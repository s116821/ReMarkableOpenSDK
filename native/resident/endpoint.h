#pragma once
// Linux/Qt endpoint only. Native profile selection and executor are separate.
#include "wire.h"
#include <QCoreApplication>
#include <QFile>
#include <QLocalSocket>
#include <QPointer>
#include <QSocketNotifier>
#include <QThread>
#include <QTimer>
#include <QUuid>
#include <atomic>
#include <chrono>
#include <functional>
#include <memory>
#include <limits>
#include <cerrno>
#include <climits>
#include <cstring>
#if defined(RSDK_ENDPOINT_DIAGNOSTICS_V1) || defined(RSDK_DEVELOPMENT_PRE_ENTER_DIAGNOSTIC_V1) || defined(RSDK_PRE_ENTER_MARKER_TESTING)
#include <cstdio>
#endif
#include <fcntl.h>
#include <sys/random.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/un.h>
#include <unistd.h>

namespace remarkable::resident {
inline bool sameSocketObject(const struct stat &a,const struct stat &b) {
    return S_ISSOCK(a.st_mode)&&S_ISSOCK(b.st_mode)&&a.st_dev==b.st_dev&&a.st_ino==b.st_ino&&a.st_uid==b.st_uid;
}
// Pin the already identified inode before chmod. A pathname replacement must
// neither redirect the permission change nor become the cleanup identity.
inline bool restrictSocket(int root,const struct stat &bound,struct stat &restricted,const char *leaf="sdk.sock") {
    const int fd=::openat(root,leaf,O_PATH|O_NOFOLLOW|O_CLOEXEC);if(fd<0)return false;
    struct stat pinned{},named{};
    bool ok=!::fstat(fd,&pinned)&&sameSocketObject(bound,pinned);
    const auto descriptor=QByteArray("/proc/self/fd/")+QByteArray::number(fd);
    if(ok)ok=!::chmod(descriptor.constData(),0600)&&!::fstat(fd,&pinned)&&sameSocketObject(bound,pinned)&&
        (pinned.st_mode&07777)==0600&&!::fstatat(root,leaf,&named,AT_SYMLINK_NOFOLLOW)&&
        sameSocketObject(pinned,named)&&pinned.st_mode==named.st_mode;
    ::close(fd);if(ok)restricted=pinned;return ok;
}
// The public name is a listening boundary, not merely a successful bind. Linux
// atomic no-replace publication preserves any unexpected final entry.
inline bool publishListeningSocket(int root,int listener,const struct stat &bound) {
    int accepting=0;socklen_t size=sizeof accepting;struct stat named{};
    if(::getsockopt(listener,SOL_SOCKET,SO_ACCEPTCONN,&accepting,&size)||size!=sizeof accepting||!accepting||
       ::fstatat(root,"sdk.pending",&named,AT_SYMLINK_NOFOLLOW)||!sameSocketObject(bound,named)||
       (named.st_mode&07777)!=0600)return false;
    if(::syscall(SYS_renameat2,root,"sdk.pending",root,"sdk.sock",1U /* RENAME_NOREPLACE */))return false;
    return !::fstatat(root,"sdk.sock",&named,AT_SYMLINK_NOFOLLOW)&&sameSocketObject(bound,named)&&
        (named.st_mode&07777)==0600;
}
inline quint64 processStart(pid_t pid) {
    QFile file(QStringLiteral("/proc/%1/stat").arg(pid));
    if(!file.open(QIODevice::ReadOnly))return 0;
    const auto bytes=file.read(4096);if(!file.atEnd())return 0;
    const auto end=bytes.lastIndexOf(')');if(end<0)return 0;
    const auto fields=bytes.mid(end+1).simplified().split(' ');
    if(fields.size()<20)return 0;
    bool ok=false;const auto value=fields.at(19).toULongLong(&ok);return ok?value:0;
}
inline Id bootIdentity() {
    QFile file(QStringLiteral("/proc/sys/kernel/random/boot_id"));
    if(!file.open(QIODevice::ReadOnly))return {};
    const auto bytes=file.read(64).trimmed();if(bytes.size()!=36)return {};
    const auto value=QUuid(QString::fromLatin1(bytes)).toRfc4122();return validId(value)?value:Id{};
}
inline Id freshNonce() {
    Id value(16,0);qsizetype done=0;
    for(unsigned attempt=0;attempt<8&&done<16;++attempt) {
        const auto count=::getrandom(value.data()+done,std::size_t(16-done),GRND_NONBLOCK);
        if(count>0)done+=count;else if(count<0&&errno==EINTR)continue;else return {};
    }
    return done==16&&validId(value)?value:Id{};
}
struct EndpointConfig {
    QString directory; // Existing owned mode0700 directory; no automatic creation.
    QByteArray runtime; // Exact approved artifact/profile plus actual self incarnation.
    uid_t peerUid=0;
    pid_t peerPid=0;
    quint64 peerStart=0;
    int operationBudgetMs=30000;
    // Bootstrap can pin the directory it consumed admission from. Ordinary
    // owned transport fixtures may omit this additional correspondence check.
    bool pinDirectory=false;
    dev_t directoryDevice=0;
    ino_t directoryInode=0;
};
// Plain lifetime gate; no native owner is reclaimed by this object. A worker may
// inspect active/deadline but must report through its existing guarded Qt bridge.
class RequestLease final {
    friend class Endpoint;
    std::chrono::steady_clock::time_point deadline_;
    const std::shared_ptr<std::atomic<bool>> retired_=std::make_shared<std::atomic<bool>>(false);
    Frame request_;
    QByteArray digest_;
    RequestLease(Frame frame,int budget):deadline_(std::chrono::steady_clock::now()+std::chrono::milliseconds(budget)),
        request_(std::move(frame)),digest_(fingerprint(request_)) {}
public:
    bool active()const{return !retired_->load(std::memory_order_acquire)&&std::chrono::steady_clock::now()<deadline_;}
    const Frame &request()const{return request_;}
    const QByteArray &digest()const{return digest_;}
    std::chrono::steady_clock::time_point deadline()const{return deadline_;}
    std::shared_ptr<const std::atomic<bool>> retirement()const{return retired_;}
};
class Endpoint final:public QObject {
public:
    using Lease=std::shared_ptr<const RequestLease>;
    // GUI-thread internal backend; may finish synchronously or retain a plain lease.
    using Submit=std::function<void(Lease)>;
    Endpoint(EndpointConfig config,Submit submit,QObject *parent=nullptr):QObject(parent),config_(std::move(config)),submit_(std::move(submit)) {
        timer_.setParent(this);timer_.setSingleShot(true);timer_.setTimerType(Qt::PreciseTimer);
        connect(&timer_,&QTimer::timeout,this,[this]{retireConnection(pending_?"timer-request":outputDeadline_?"timer-output":ready_?"timer-idle":"timer-hello");});
    }
    ~Endpoint()override {
        // No backend callbacks/destruction of native owners from shutdown.
        stopped_=true;retireConnection("endpoint-destroy");
        if(notifier_)notifier_->setEnabled(false);
        if(listener_>=0)::close(listener_);
        if(root_>=0) {
            struct stat named{};
            for(const char *leaf:{"sdk.pending","sdk.sock"})
                if(socketOwned_&&!::fstatat(root_,leaf,&named,AT_SYMLINK_NOFOLLOW)&&same(named,socketStat_))::unlinkat(root_,leaf,0);
            ::close(root_);
        }
    }
    bool start() {
        if(started_||!gui())return false;
        started_=true;
        if(config_.peerPid<=0||!config_.peerStart||config_.peerUid!=::geteuid()||config_.operationBudgetMs<1||config_.operationBudgetMs>30000||
           config_.runtime.size()!=140||config_.runtime.mid(96,16)!=bootIdentity()||
           qFromBigEndian<quint32>(reinterpret_cast<const uchar*>(config_.runtime.constData()+112))!=quint32(::getpid())||
           qFromBigEndian<quint64>(reinterpret_cast<const uchar*>(config_.runtime.constData()+116))!=processStart(::getpid()))return false;
        Frame shape;shape.runtime=config_.runtime;shape.client=Id(16,1);shape.server=Id(16,0);shape.operation=Id(16,2);
        if(encode(shape).isEmpty())return false;
        const auto directory=QFile::encodeName(config_.directory);
        if(directory.isEmpty()||!directory.startsWith('/')||directory.contains('\0'))return false;
        root_=::open(directory.constData(),O_RDONLY|O_DIRECTORY|O_NOFOLLOW|O_CLOEXEC);
        if(root_<0||::fstat(root_,&rootStat_)||!directoryCurrent()||
           (config_.pinDirectory&&(rootStat_.st_dev!=config_.directoryDevice||rootStat_.st_ino!=config_.directoryInode)))return false;
        struct stat existing{};
        if(!::fstatat(root_,"sdk.sock",&existing,AT_SYMLINK_NOFOLLOW)||errno!=ENOENT)return false;
        if(!::fstatat(root_,"sdk.pending",&existing,AT_SYMLINK_NOFOLLOW)||errno!=ENOENT)return false;
        const auto visible=directory+"/sdk.pending";
        if(visible.size()>=qsizetype(sizeof(sockaddr_un::sun_path)))return false;
        listener_=::socket(AF_UNIX,SOCK_STREAM|SOCK_NONBLOCK|SOCK_CLOEXEC,0);if(listener_<0)return false;
        // Bind through the retained directory, never through a replaceable path.
        const auto name=QByteArray("/proc/self/fd/")+QByteArray::number(root_)+"/sdk.pending";
        sockaddr_un address{};address.sun_family=AF_UNIX;
        if(name.size()>=qsizetype(sizeof address.sun_path))return false;
        std::memcpy(address.sun_path,name.constData(),std::size_t(name.size()));
        if(::bind(listener_,reinterpret_cast<sockaddr*>(&address),sizeof address))return false;
        if(::fstatat(root_,"sdk.pending",&socketStat_,AT_SYMLINK_NOFOLLOW)||!S_ISSOCK(socketStat_.st_mode)||socketStat_.st_uid!=::geteuid())return false;
        socketOwned_=true;
        struct stat restricted{};
        if(!restrictSocket(root_,socketStat_,restricted,"sdk.pending"))return false;
        socketStat_=restricted;
        if(!directoryCurrent()||::listen(listener_,1)||!publishListeningSocket(root_,listener_,socketStat_)||!directoryCurrent())return false;
        notifier_=new QSocketNotifier(listener_,QSocketNotifier::Read,this);
        connect(notifier_,&QSocketNotifier::activated,this,[this]{acceptOne();});
        return true;
    }
#if defined(RSDK_DEVELOPMENT_PRE_ENTER_DIAGNOSTIC_V1) || defined(RSDK_PRE_ENTER_MARKER_TESTING)
    // Called only after the dispatcher owns the capture handle.
    bool notePreEnterArmed(const Lease &lease) {
        if(!gui()||!pending_||lease.get()!=pending_.get()||
           lease->request().kind!=PreEnterRequest)return false;
        const auto fail=[this]{retireConnection("pre-enter-marker-refused");return false;};
        if(preEnterMarkerAttempted_)return fail();
        preEnterMarkerAttempted_=true;
        if(!lease->active()||!peerCurrent()||!preEnterStderrCurrent())return fail();
        const auto &request=lease->request();
        const auto *runtime=reinterpret_cast<const uchar *>(request.runtime.constData());
        char activation[33]{},operation[33]{},digest[65]{};
        markerHex(activation,runtime+124,16);
        markerHex(operation,reinterpret_cast<const uchar *>(request.operation.constData()),16);
        markerHex(digest,reinterpret_cast<const uchar *>(lease->digest().constData()),32);
        using namespace std::chrono;
        const auto sample=duration_cast<nanoseconds>(steady_clock::now().time_since_epoch()).count();
        const auto deadline=duration_cast<nanoseconds>(lease->deadline().time_since_epoch()).count();
        if(sample<0||deadline<=sample)return fail();
        char line[384];
        const auto length=std::snprintf(line,sizeof line,
            "RSDK_PRE_ENTER_ARMED_V1 pid=%u start_ticks=%llu activation=%s sequence=%llu operation=%s request_sha256=%s sample_ns=%llu deadline_ns=%llu\n",
            unsigned(qFromBigEndian<quint32>(runtime+112)),
            static_cast<unsigned long long>(qFromBigEndian<quint64>(runtime+116)),activation,
            static_cast<unsigned long long>(request.sequence),operation,digest,
            static_cast<unsigned long long>(sample),static_cast<unsigned long long>(deadline));
        if(length<=0||length>=int(sizeof line)||!lease->active())return fail();
#ifdef RSDK_PRE_ENTER_MARKER_TESTING
        const auto written=preEnterWrite_(STDERR_FILENO,line,std::size_t(length));
#else
        const auto written=::write(STDERR_FILENO,line,std::size_t(length));
#endif
        if(written!=length||!lease->active()||!peerCurrent())return fail();
        return true;
    }
#endif
    // GUI-only. Completion invalidates publication/getter authority immediately;
    // releasing this plain lease says nothing about physical native completion.
    bool finish(const Lease &lease,Frame result) {
        if(!gui()||!pending_||lease.get()!=pending_.get()||!lease->active()||!peerCurrent())return false;
        if(!validPreEnterReply(lease->request(),result)){retireConnection("finish-wrong-text-reply");return false;}
#ifdef RSDK_DEVELOPMENT_READER_FOCUS_V1
        if(!validReaderFocusReply(lease->request(),result)){retireConnection("finish-wrong-reader-focus-reply");return false;}
#endif
        result.runtime=config_.runtime;result.client=client_;result.server=server_;
        result.sequence=lease->request().sequence;result.operation=lease->request().operation;
        result.message=3;result.requestDigest=lease->digest();
        const auto bytes=encode(result);
        // Encoding/validation can consume the remaining budget while the Qt
        // timer is unable to run on this stack.
        if(bytes.isEmpty()||!lease->active()){retireConnection("finish-invalid-or-expired");return false;}
        outputDeadline_=lease->deadline();
        pending_->retired_->store(true,std::memory_order_release);pending_.reset();
        if(!send(bytes)){retireConnection("finish-send-refused");return false;}
        if(!refreshOutput())return false;
        return true;
    }
private:
    EndpointConfig config_;
    Submit submit_;
    int root_=-1,listener_=-1;
    struct stat rootStat_{},socketStat_{};
    bool started_=false,stopped_=false,socketOwned_=false,ready_=false,reading_=false;
    QSocketNotifier *notifier_=nullptr;
    QPointer<QLocalSocket> peer_;
    QTimer timer_;
    Decoder decoder_;
    Id client_,server_;
    quint64 sequence_=0,connectionGeneration_=0;
    std::shared_ptr<RequestLease> pending_;
    std::optional<std::chrono::steady_clock::time_point> outputDeadline_;
#if defined(RSDK_DEVELOPMENT_PRE_ENTER_DIAGNOSTIC_V1) || defined(RSDK_PRE_ENTER_MARKER_TESTING)
    bool preEnterMarkerAttempted_=false;
#ifdef RSDK_PRE_ENTER_MARKER_TESTING
    friend struct PreEnterMarkerTestAccess;
    ssize_t (*preEnterWrite_)(int,const void *,std::size_t)=::write;
#endif
    static void markerHex(char *out,const uchar *in,std::size_t size) noexcept {
        constexpr char hex[]="0123456789abcdef";
        for(std::size_t n=0;n<size;++n) {
            out[2*n]=hex[in[n]>>4];out[2*n+1]=hex[in[n]&15];
        }
    }
    bool preEnterStderrCurrent()const noexcept {
        struct stat descriptor{},named{};
        return !::fstat(STDERR_FILENO,&descriptor)&&S_ISREG(descriptor.st_mode)&&
            (descriptor.st_mode&07777)==0600&&descriptor.st_uid==config_.peerUid&&descriptor.st_nlink==1&&
            !::fstatat(root_,"stderr.log",&named,AT_SYMLINK_NOFOLLOW)&&named.st_nlink==1&&same(descriptor,named);
    }
#endif
    bool gui()const{return QCoreApplication::instance()&&!QCoreApplication::closingDown()&&QThread::currentThread()==thread()&&thread()==QCoreApplication::instance()->thread();}
    static bool same(const struct stat&a,const struct stat&b){return a.st_dev==b.st_dev&&a.st_ino==b.st_ino&&a.st_uid==b.st_uid&&a.st_mode==b.st_mode;}
    bool directoryCurrent()const {
        struct stat current{},named{};const auto path=QFile::encodeName(config_.directory);
        return root_>=0&&!::fstat(root_,&current)&&S_ISDIR(current.st_mode)&&(current.st_mode&07777)==0700&&current.st_uid==::geteuid()&&
            same(rootStat_,current)&&!::lstat(path.constData(),&named)&&same(current,named);
    }
    bool credentials(int fd)const {
        struct ucred peer{};socklen_t length=sizeof peer;
        return !::getsockopt(fd,SOL_SOCKET,SO_PEERCRED,&peer,&length)&&length==sizeof peer&&peer.uid==config_.peerUid&&
            peer.pid==config_.peerPid&&processStart(peer.pid)==config_.peerStart;
    }
    bool peerCurrent()const {
        struct stat named{};
        return !stopped_&&peer_&&peer_->state()==QLocalSocket::ConnectedState&&directoryCurrent()&&
            !::fstatat(root_,"sdk.sock",&named,AT_SYMLINK_NOFOLLOW)&&same(named,socketStat_)&&credentials(int(peer_->socketDescriptor()));
    }
#ifdef RSDK_ENDPOINT_DIAGNOSTICS_V1
    unsigned traceEvents_=0;
#endif
    // No request bodies, native pointers or file paths. At most64 stack-bounded
    // writes to the supervisor-owned stderr file; no new timers or retry paths.
    void trace(const char *event,int code=0) noexcept {
#ifdef RSDK_ENDPOINT_DIAGNOSTICS_V1
        if(traceEvents_>=64)return;
        ++traceEvents_;
        const auto now=std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
        char line[256];
        const int length=std::snprintf(line,sizeof line,"RSDK_ENDPOINT_V1 t_ms=%lld pid=%ld generation=%llu event=%s code=%d peer=%d ready=%d reading=%d pending=%d buffered=%lld output=%lld\n",
            static_cast<long long>(now),static_cast<long>(::getpid()),static_cast<unsigned long long>(connectionGeneration_),event,code,peer_?int(peer_->state()):-1,
            int(ready_),int(reading_),int(bool(pending_)),peer_?static_cast<long long>(peer_->bytesAvailable()):0LL,peer_?static_cast<long long>(peer_->bytesToWrite()):0LL);
        if(length>0&&length<int(sizeof line))(void)::write(STDERR_FILENO,line,std::size_t(length));
#else
        (void)event;(void)code;
#endif
    }
    void retireConnection(const char *reason,int code=0) {
        trace(reason,code);
        timer_.stop();if(pending_)pending_->retired_->store(true,std::memory_order_release);pending_.reset();
        outputDeadline_.reset();
        ready_=false;decoder_.close();client_.clear();server_.clear();
        auto *socket=peer_.data();peer_=nullptr;
        if(socket){socket->abort();socket->deleteLater();}
    }
    void acceptOne() {
        if(stopped_||!gui())return;
        const int fd=::accept4(listener_,nullptr,nullptr,SOCK_NONBLOCK|SOCK_CLOEXEC);if(fd<0)return;
        // A backend may run a nested event loop. Do not install a replacement
        // until the original read/submit stack has completely unwound.
        if(!directoryCurrent()){trace("reject-directory");::close(fd);return;}
        if(!credentials(fd)){trace("reject-credentials");::close(fd);return;}
        // Qt state notification can lag the kernel stream shutdown. Probe only
        // a completely idle old transport; one nonconsuming, nonblocking byte
        // distinguishes EOF from an open/blocked/data/error socket. No retry.
        // Native effect debt/quarantine lives in its executor and is untouched.
        if(peer_) {
            const unsigned blocked=(reading_?1U:0U)|(pending_?2U:0U)|(outputDeadline_?4U:0U)|
                (!decoder_.idle()?8U:0U)|(peer_->bytesAvailable()!=0?16U:0U)|(peer_->bytesToWrite()!=0?32U:0U);
            trace("old-peer-idle-guards",int(blocked));
            if(!blocked) {
                const auto descriptor=peer_->socketDescriptor();
                if(peer_->state()==QLocalSocket::UnconnectedState&&descriptor<0) {
                    // Qt has completed closure and no descriptor remains to peek.
                    // This does not reinterpret EBADF on a nonnegative fd.
                    retireConnection("retire-qt-closed-idle-peer");
                } else {
                    unsigned char byte=0;ssize_t observed=-1;int error=EBADF;
                    if(descriptor>=0&&descriptor<=INT_MAX) {
                        observed=::recv(int(descriptor),&byte,1,MSG_PEEK|MSG_DONTWAIT);
                        error=observed<0?errno:0;
                    }
                    // code0=EOF; code1=data; negative code is refusal errno.
                    trace("old-peer-peek",observed<0?-error:int(observed));
                    if(observed==0)retireConnection("retire-eof-idle-peer");
                }
            } else trace("old-peer-peek-skipped",int(blocked));
        }
        if(peer_){trace("reject-existing-peer");::close(fd);return;}
        if(reading_){trace("reject-reading-stack");::close(fd);return;}
        if(connectionGeneration_==std::numeric_limits<quint64>::max()){trace("reject-generation-exhausted");::close(fd);return;}
        auto *socket=new QLocalSocket(this);
        if(!socket->setSocketDescriptor(fd)){trace("reject-socket-descriptor");::close(fd);delete socket;return;}
        peer_=socket;decoder_=Decoder{};sequence_=0;ready_=false;socket->setReadBufferSize(MaxFrame+4);
        ++connectionGeneration_;
        trace("accepted");
        const QPointer<QLocalSocket> original(socket);const auto generation=connectionGeneration_;
        connect(socket,&QLocalSocket::readyRead,this,[this,original,generation]{if(original&&peer_==original&&connectionGeneration_==generation)readAvailable();},Qt::QueuedConnection);
        connect(socket,&QLocalSocket::disconnected,this,[this,original,generation]{if(original&&peer_==original&&connectionGeneration_==generation)retireConnection("peer-disconnected");},Qt::QueuedConnection);
        connect(socket,&QLocalSocket::errorOccurred,this,[this,original,generation]{if(original&&peer_==original&&connectionGeneration_==generation)retireConnection("peer-error",int(original->error()));},Qt::QueuedConnection);
        connect(socket,&QLocalSocket::bytesWritten,this,[this,original,generation]{if(original&&peer_==original&&connectionGeneration_==generation&&outputDeadline_)refreshOutput();},Qt::QueuedConnection);
        timer_.start(5000);readAvailable();
    }
    bool send(const QByteArray &bytes) {
        return peerCurrent()&&!bytes.isEmpty()&&bytes.size()<=MaxCaptureFrame+4&&peer_->bytesToWrite()==0&&peer_->write(bytes)==bytes.size();
    }
    bool refreshOutput() {
        if(!outputDeadline_)return true;
        const auto now=std::chrono::steady_clock::now();
        if(!peerCurrent()||now>=*outputDeadline_){retireConnection("output-expired-or-peer");return false;}
        if(peer_->bytesToWrite()==0){outputDeadline_.reset();timer_.start(30000);}
        else {
            const auto left=std::chrono::duration_cast<std::chrono::milliseconds>(*outputDeadline_-now).count();
            timer_.start(int(qMax(qint64(1),qint64(left))));
        }
        return true;
    }
    void readAvailable() {
        if(reading_||!peerCurrent())return;
        reading_=true;const QPointer<Endpoint> self(this);const QPointer<QLocalSocket> original(peer_);const auto generation=connectionGeneration_;
        // One bounded read per callback. Qt queues further readiness; no wait APIs.
        auto bytes=peer_->read(MaxFrame+4);qsizetype used=0;
        while(self&&original&&peer_==original&&connectionGeneration_==generation&&used<bytes.size()) {
            auto decoded=decoder_.feed(bytes.mid(used));used+=decoded.consumed;
            if(decoded.error||(!decoded.consumed&&!decoded.frame)){retireConnection("decode-refused");break;}
            if(decoded.frame)acceptFrame(*decoded.frame);
        }
        if(!self)return;
        reading_=false;
        if(original&&peer_==original&&connectionGeneration_==generation&&original->bytesAvailable()>0)
            QTimer::singleShot(0,this,[this,original,generation]{if(original&&peer_==original&&connectionGeneration_==generation)readAvailable();});
    }
    void acceptFrame(const Frame &frame) {
        if(!refreshOutput())return;
        if(!peerCurrent()||frame.runtime!=config_.runtime){retireConnection("frame-peer-or-runtime");return;}
        if(!ready_) {
            if(frame.message!=0){retireConnection("expected-hello");return;}
            trace("hello");
            client_=frame.client;server_=freshNonce();if(!validId(server_)){retireConnection("nonce-unavailable");return;}
            auto reply=frame;reply.message=1;reply.server=server_;
            if(!send(encode(reply))){retireConnection("ready-send-refused");return;}
            ready_=true;trace("ready");timer_.start(30000);return;
        }
        if(frame.message!=2||frame.client!=client_||frame.server!=server_||pending_||outputDeadline_||sequence_==std::numeric_limits<quint64>::max()||
           frame.sequence!=sequence_+1){retireConnection("request-scope-or-sequence");return;}
        sequence_=frame.sequence;pending_=std::shared_ptr<RequestLease>(new RequestLease(frame,config_.operationBudgetMs));
        if(!pending_->active()){retireConnection("request-expired");return;}
        timer_.start(config_.operationBudgetMs);
        if(!submit_){Frame unsupported;unsupported.kind=3;unsupported.rejection=0;finish(pending_,unsupported);return;}
        auto submit=submit_;const Lease lease=pending_;
        // No endpoint access after invoking a possibly reentrant internal backend.
        const QPointer<Endpoint> self(this);const QPointer<QLocalSocket> original(peer_);const auto generation=connectionGeneration_;
        try {submit(lease);} catch(...) {if(self&&original&&self->peer_==original&&self->connectionGeneration_==generation)self->retireConnection("backend-exception");}
    }
};
}
