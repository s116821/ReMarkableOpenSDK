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
#include <fcntl.h>
#include <sys/random.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <unistd.h>

namespace remarkable::resident {
inline bool sameSocketObject(const struct stat &a,const struct stat &b) {
    return S_ISSOCK(a.st_mode)&&S_ISSOCK(b.st_mode)&&a.st_dev==b.st_dev&&a.st_ino==b.st_ino&&a.st_uid==b.st_uid;
}
// Pin the already identified inode before chmod. A pathname replacement must
// neither redirect the permission change nor become the cleanup identity.
inline bool restrictSocket(int root,const struct stat &bound,struct stat &restricted) {
    const int fd=::openat(root,"sdk.sock",O_PATH|O_NOFOLLOW|O_CLOEXEC);if(fd<0)return false;
    struct stat pinned{},named{};
    bool ok=!::fstat(fd,&pinned)&&sameSocketObject(bound,pinned);
    const auto descriptor=QByteArray("/proc/self/fd/")+QByteArray::number(fd);
    if(ok)ok=!::chmod(descriptor.constData(),0600)&&!::fstat(fd,&pinned)&&sameSocketObject(bound,pinned)&&
        (pinned.st_mode&07777)==0600&&!::fstatat(root,"sdk.sock",&named,AT_SYMLINK_NOFOLLOW)&&
        sameSocketObject(pinned,named)&&pinned.st_mode==named.st_mode;
    ::close(fd);if(ok)restricted=pinned;return ok;
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
};
// Plain lifetime gate; no native owner is reclaimed by this object. A worker may
// inspect active/deadline but must report through its existing guarded Qt bridge.
class RequestLease final {
    friend class Endpoint;
    std::atomic<bool> active_{true};
    std::chrono::steady_clock::time_point deadline_;
    Frame request_;
    QByteArray digest_;
    RequestLease(Frame frame,int budget):deadline_(std::chrono::steady_clock::now()+std::chrono::milliseconds(budget)),
        request_(std::move(frame)),digest_(fingerprint(request_)) {}
public:
    bool active()const{return active_.load(std::memory_order_acquire)&&std::chrono::steady_clock::now()<deadline_;}
    const Frame &request()const{return request_;}
    const QByteArray &digest()const{return digest_;}
};
class Endpoint final:public QObject {
public:
    using Lease=std::shared_ptr<const RequestLease>;
    // GUI-thread internal backend; may finish synchronously or retain a plain lease.
    using Submit=std::function<void(Lease)>;
    Endpoint(EndpointConfig config,Submit submit,QObject *parent=nullptr):QObject(parent),config_(std::move(config)),submit_(std::move(submit)) {
        timer_.setParent(this);timer_.setSingleShot(true);timer_.setTimerType(Qt::PreciseTimer);
        connect(&timer_,&QTimer::timeout,this,[this]{retireConnection();});
    }
    ~Endpoint()override {
        // No backend callbacks/destruction of native owners from shutdown.
        stopped_=true;retireConnection();
        if(notifier_)notifier_->setEnabled(false);
        if(listener_>=0)::close(listener_);
        if(root_>=0) {
            struct stat named{};
            if(socketOwned_&&!::fstatat(root_,"sdk.sock",&named,AT_SYMLINK_NOFOLLOW)&&same(named,socketStat_))::unlinkat(root_,"sdk.sock",0);
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
        if(root_<0||::fstat(root_,&rootStat_)||!directoryCurrent())return false;
        struct stat existing{};
        if(!::fstatat(root_,"sdk.sock",&existing,AT_SYMLINK_NOFOLLOW)||errno!=ENOENT)return false;
        const auto visible=directory+"/sdk.sock";
        if(visible.size()>=qsizetype(sizeof(sockaddr_un::sun_path)))return false;
        listener_=::socket(AF_UNIX,SOCK_STREAM|SOCK_NONBLOCK|SOCK_CLOEXEC,0);if(listener_<0)return false;
        // Bind through the retained directory, never through a replaceable path.
        const auto name=QByteArray("/proc/self/fd/")+QByteArray::number(root_)+"/sdk.sock";
        sockaddr_un address{};address.sun_family=AF_UNIX;
        if(name.size()>=qsizetype(sizeof address.sun_path))return false;
        std::memcpy(address.sun_path,name.constData(),std::size_t(name.size()));
        if(::bind(listener_,reinterpret_cast<sockaddr*>(&address),sizeof address))return false;
        if(::fstatat(root_,"sdk.sock",&socketStat_,AT_SYMLINK_NOFOLLOW)||!S_ISSOCK(socketStat_.st_mode)||socketStat_.st_uid!=::geteuid())return false;
        socketOwned_=true;
        struct stat restricted{};
        if(!restrictSocket(root_,socketStat_,restricted))return false;
        socketStat_=restricted;
        if(!directoryCurrent()||::listen(listener_,1))return false;
        notifier_=new QSocketNotifier(listener_,QSocketNotifier::Read,this);
        connect(notifier_,&QSocketNotifier::activated,this,[this]{acceptOne();});
        return true;
    }
    // GUI-only. Completion invalidates publication/getter authority immediately;
    // releasing this plain lease says nothing about physical native completion.
    bool finish(const Lease &lease,Frame result) {
        if(!gui()||!pending_||lease.get()!=pending_.get()||!lease->active()||!peerCurrent())return false;
        result.runtime=config_.runtime;result.client=client_;result.server=server_;
        result.sequence=lease->request().sequence;result.operation=lease->request().operation;
        result.message=3;result.requestDigest=lease->digest();
        const auto bytes=encode(result);
        // Encoding/validation can consume the remaining budget while the Qt
        // timer is unable to run on this stack.
        if(bytes.isEmpty()||!lease->active()){retireConnection();return false;}
        pending_->active_.store(false,std::memory_order_release);pending_.reset();
        if(!send(bytes)){retireConnection();return false;}
        timer_.start(30000);return true;
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
    quint64 sequence_=0;
    std::shared_ptr<RequestLease> pending_;
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
    void retireConnection() {
        timer_.stop();if(pending_)pending_->active_.store(false,std::memory_order_release);pending_.reset();
        ready_=false;decoder_.close();client_.clear();server_.clear();
        auto *socket=peer_.data();peer_=nullptr;
        if(socket){socket->abort();socket->deleteLater();}
    }
    void acceptOne() {
        if(stopped_||!gui())return;
        const int fd=::accept4(listener_,nullptr,nullptr,SOCK_NONBLOCK|SOCK_CLOEXEC);if(fd<0)return;
        // A backend may run a nested event loop. Do not install a replacement
        // until the original read/submit stack has completely unwound.
        if(peer_||reading_||!directoryCurrent()||!credentials(fd)){::close(fd);return;}
        auto *socket=new QLocalSocket(this);
        if(!socket->setSocketDescriptor(fd)){::close(fd);delete socket;return;}
        peer_=socket;decoder_=Decoder{};sequence_=0;ready_=false;socket->setReadBufferSize(MaxFrame+4);
        const QPointer<QLocalSocket> original(socket);
        connect(socket,&QLocalSocket::readyRead,this,[this,original]{if(original&&peer_==original)readAvailable();},Qt::QueuedConnection);
        connect(socket,&QLocalSocket::disconnected,this,[this,original]{if(original&&peer_==original)retireConnection();},Qt::QueuedConnection);
        connect(socket,&QLocalSocket::errorOccurred,this,[this,original]{if(original&&peer_==original)retireConnection();},Qt::QueuedConnection);
        timer_.start(5000);readAvailable();
    }
    bool send(const QByteArray &bytes) {
        return peerCurrent()&&!bytes.isEmpty()&&bytes.size()<=MaxFrame+4&&peer_->bytesToWrite()==0&&peer_->write(bytes)==bytes.size();
    }
    void readAvailable() {
        if(reading_||!peerCurrent())return;
        reading_=true;const QPointer<Endpoint> self(this);const QPointer<QLocalSocket> original(peer_);
        // One bounded read per callback. Qt queues further readiness; no wait APIs.
        auto bytes=peer_->read(MaxFrame+4);qsizetype used=0;
        while(self&&original&&peer_==original&&used<bytes.size()) {
            auto decoded=decoder_.feed(bytes.mid(used));used+=decoded.consumed;
            if(decoded.error||(!decoded.consumed&&!decoded.frame)){retireConnection();break;}
            if(decoded.frame)acceptFrame(*decoded.frame);
        }
        if(!self)return;
        reading_=false;
        if(original&&peer_==original&&original->bytesAvailable()>0)
            QTimer::singleShot(0,this,[this,original]{if(original&&peer_==original)readAvailable();});
    }
    void acceptFrame(const Frame &frame) {
        if(!peerCurrent()||frame.runtime!=config_.runtime){retireConnection();return;}
        if(!ready_) {
            if(frame.message!=0){retireConnection();return;}
            client_=frame.client;server_=freshNonce();if(!validId(server_)){retireConnection();return;}
            auto reply=frame;reply.message=1;reply.server=server_;
            if(!send(encode(reply))){retireConnection();return;}
            ready_=true;timer_.start(30000);return;
        }
        if(frame.message!=2||frame.client!=client_||frame.server!=server_||pending_||sequence_==std::numeric_limits<quint64>::max()||
           frame.sequence!=sequence_+1){retireConnection();return;}
        sequence_=frame.sequence;pending_=std::shared_ptr<RequestLease>(new RequestLease(frame,config_.operationBudgetMs));
        if(!pending_->active()){retireConnection();return;}
        timer_.start(config_.operationBudgetMs);
        if(!submit_){Frame unsupported;unsupported.kind=3;unsupported.rejection=0;finish(pending_,unsupported);return;}
        auto submit=submit_;const Lease lease=pending_;
        // No endpoint access after invoking a possibly reentrant internal backend.
        const QPointer<Endpoint> self(this);const QPointer<QLocalSocket> original(peer_);
        try {submit(lease);} catch(...) {if(self&&original&&self->peer_==original)self->retireConnection();}
    }
};
}
