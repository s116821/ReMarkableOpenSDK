#pragma once
// Fixed private-file backend; original actor/unit/provider admission is separate.
#include "reader_write_reopen_sequence.h"
#include "reader_write_reopen_files.h"
#include "reader_capture_files.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <array>

namespace reader_write_reopen {
class NativeFiles final:public IO {
    struct Control {
        int fd=-1;const char *name=nullptr;struct stat stat{};QByteArray bytes;
        ~Control(){if(fd>=0)::close(fd);}
        Control()=default;Control(const Control&)=delete;Control&operator=(const Control&)=delete;
        bool open(const std::shared_ptr<reader_write_reopen_files::Directory> &directory,const char *file){
            if(fd>=0||!directory||!directory->bound())return false;
            Control proposed;proposed.name=file;
            proposed.fd=::openat(directory->borrowedFd(),file,O_RDONLY|O_NOFOLLOW|O_NONBLOCK|O_CLOEXEC);
            const auto &s=proposed.stat;
            if(proposed.fd<0||::fstat(proposed.fd,&proposed.stat)||!S_ISREG(s.st_mode)||(s.st_mode&07777)!=0600||s.st_uid!=::geteuid()||s.st_nlink!=1||s.st_size<1||s.st_size>256)return false;
            char data[257];const auto count=::pread(proposed.fd,data,sizeof data,0);
            if(count!=s.st_size)return false;
            proposed.bytes=QByteArray(data,int(count));if(!proposed.unchanged(directory))return false;
            name=file;stat=s;bytes=proposed.bytes;fd=std::exchange(proposed.fd,-1);return true;
        }
        bool unchanged(const std::shared_ptr<reader_write_reopen_files::Directory> &directory)const{
            if(fd<0||!directory||!directory->bound())return false;
            struct stat current{},named{};char data[257];
            return !::fstat(fd,&current)&&reader_write_reopen_files::same(current,stat)&&
                !::fstatat(directory->borrowedFd(),name,&named,AT_SYMLINK_NOFOLLOW)&&reader_write_reopen_files::same(current,named)&&
                ::pread(fd,data,sizeof data,0)==bytes.size()&&QByteArray(data,bytes.size())==bytes;
        }
    };
public:
    static std::shared_ptr<NativeFiles> create(const Config &config,const QByteArray &boot){
        if(::geteuid()!=0||!config.target.valid()||config.nonce.size()!=32||boot.size()!=36)return {};
        for(char c:config.nonce)if(!((c>='a'&&c<='f')||(c>='0'&&c<='9')))return {};
        if(!reader_fixed_pen_result::detail::pid(QString::fromLatin1(config.pid))||!reader_fixed_pen_result::detail::decimal(QString::fromLatin1(config.start))||
           config.pid!=QByteArray::number(::getpid()))return {};
        auto self=std::shared_ptr<NativeFiles>(new NativeFiles(config,boot));
        self->directory_=reader_write_reopen_files::Directory::open(QStringLiteral("/run/rmb-qt-probe-")+QString::fromLatin1(config.nonce));
        if(!self->directory_)return {};
        const char *names[]={"owner","attempt.identity","deadline","candidate.window"};
        for(unsigned i=0;i<4;++i)if(!self->controls_[i].open(self->directory_,names[i]))return {};
        if(self->controls_[0].bytes!=config.nonce||self->controls_[1].bytes!=config.pid+' '+config.start+'\n')return {};
        const auto deadline=self->controls_[2].bytes,window=self->controls_[3].bytes;
        if(!deadline.endsWith('\n')||!window.endsWith('\n'))return {};
        const auto d=deadline.chopped(1).split(' '),w=window.chopped(1).split(' ');
        quint64 armed=0,end=0,wa=0,we=0,begin=0,ends=0;
        const auto number=[](const QByteArray &b,quint64 &n){return reader_fixed_pen_result::detail::decimal(QString::fromLatin1(b),&n);};
        if(d.size()!=2||w.size()!=5||w[0]!=config.nonce||!number(d[0],armed)||!number(d[1],end)||end<armed||end-armed!=360||
           !number(w[1],wa)||!number(w[2],we)||!number(w[3],begin)||!number(w[4],ends)||wa!=armed||we!=end||begin<armed||ends<begin||ends-begin!=90||
           end<185||ends>end-185||ends>std::numeric_limits<quint64>::max()/1000||ends*1000!=config.setupEnd)return {};
        self->began_=begin*1000;
        return self->admitted()?self:std::shared_ptr<NativeFiles>{};
    }
    quint64 nowMs()noexcept override{return reader_fixed_pen_handoff::bootMs();}
    bool admitted()noexcept override{
        try{
            const auto before=nowMs();if(!before||before<began_||before>=config_.setupEnd||!controls())return false;
            for(const char *n:{"stop.request","restore.claim","selected-cancel","pen-cancel"})if(!directory_->absent(n))return false;
            if(proc("/proc/sys/kernel/random/boot_id",64)!=boot_+'\n'||processStart(::getpid())!=config_.start)return false;
            const auto after=nowMs();return after>=before&&after<config_.setupEnd;
        }catch(...){return false;}
    }
    HolderReady holderReady()noexcept override{
        HolderReady r;
        try{
            if(!admitted())return r;
            if(!holderPinned_){
                if(!ready_.open(directory_,"pen-holder.ready"))return r;
                const auto raw=ready_.bytes;
                if(!raw.endsWith('\n'))return r;
                const auto p=raw.chopped(1).split(' ');
                if(p.size()!=7||p[0]!=config_.nonce||p[1]!=config_.pid||p[2]!=config_.start||p[3]!=boot_||p[6]!=QByteArray::number(config_.setupEnd)||
                   !reader_fixed_pen_result::detail::pid(QString::fromLatin1(p[4]))||!reader_fixed_pen_result::detail::decimal(QString::fromLatin1(p[5]))||p[4]==config_.pid)return r;
                holder_={QString::fromLatin1(config_.nonce),QString::fromLatin1(boot_),QString::fromLatin1(config_.pid),QString::fromLatin1(config_.start),
                    QString::fromLatin1(p[4]),QString::fromLatin1(p[5]),config_.setupEnd,0};
                holderPinned_=true;
            }
            if(!ready_.unchanged(directory_)||processStart(pid_t(holder_.holderPid.toLongLong()))!=holder_.holderStart.toLatin1())return r;
            const auto group=proc("/proc/"+holder_.holderPid.toLatin1()+"/cgroup",4096);
            const auto suffix=QByteArray(":/system.slice/rmb-qt-pen-")+config_.nonce+".service";
            unsigned matching=0;
            for(const auto &line:group.split('\n'))if(line.endsWith(suffix))++matching;
            if(matching!=1||group.contains("xochitl")||group.contains("-actor.service")||!admitted())return r;
            r.observed=true;r.binding=holder_;return r;
        }catch(...){return r;}
    }
    Capture capture(int index,QQuickWindow *window,const reader_identity_capture::Expected &target,
                    const std::shared_ptr<reader_identity_capture::Claim> &claim,const Guard &permit,int budget)override{
        Capture evidence;
        if(index<0||index>2||captureUsed_[std::size_t(index)])return evidence;
        captureUsed_[std::size_t(index)]=true;
        const auto directory=directory_;
        if(!admitted()||!directory||!permit||!permit())return evidence;
        const char *names[]={"capture-before","capture-ink","capture-reopen"};
        const int root=directory->makeCapture(names[index]);if(root<0)return evidence;
        auto files=reader_capture_files::Files::create(root,permit);
        auto closeRoot=qScopeGuard([&]{::close(root);});
        if(!files)return evidence;
        auto retire=qScopeGuard([&]{files->retire();});
        if(!files->openImage(permit).ok)return evidence;
        {
            visual_capture::Sink sink(files->imageFd());
            evidence.facts=reader_identity_capture::run(window,target,sink,claim,permit,budget);
        }
        const auto &facts=evidence.facts;
        if(!facts.captured||facts.unknown||!facts.refusal.isEmpty()||!files->sealImage(facts.encoded,permit).ok||!files->commitImage(permit).committed)return evidence;
        const QJsonObject scalar{{"schema","write-reopen-capture/1"},{"nonce",QString::fromLatin1(config_.nonce)},{"phase",index},
            {"candidate_pid",QString::fromLatin1(config_.pid)},{"candidate_start",QString::fromLatin1(config_.start)},
            {"document_id",target.documentId},{"page_id",target.pageId},{"page_index",target.pageIndex},{"page_count",target.pageCount},
            {"png_sha256",QString::fromLatin1(facts.sha)},{"png_bytes",facts.encoded},{"width",facts.width},{"height",facts.height},{"dpr",facts.dpr},
            {"before_ready",facts.before.ready},{"after_ready",facts.after.ready},{"native_authority",false},{"render_authority",false},{"durable_authority",false}};
        const auto bytes=QJsonDocument(scalar).toJson(QJsonDocument::Compact)+'\n';
        const auto published=files->publishScalar(bytes,reader_capture_files::ScalarKind::AcceptedPair,permit);
        evidence.pairCommitted=published.committed;
        evidence.scalarSha256=QCryptographicHash::hash(bytes,QCryptographicHash::Sha256).toHex();
        const bool retired=files->retire().ok;
        const bool closed=::close(root)==0;
        closeRoot.dismiss();retire.dismiss();evidence.retired=retired&&closed;
        return evidence;
    }
    bool publishGo(const QByteArray &bytes)override{
        if(goUsed_)return false;
        goUsed_=true;
        if(!admitted()||!holderReady().observed)return false;
        const auto published=directory_->publish("pen-go.request",".pending-pen-go",bytes);
        return published.committed&&admitted();
    }
    Pen pen(const reader_fixed_pen_handoff::Result &handoff)override{
        Pen r;
        if(penUsed_){r.state=PenState::Refused;return r;}
        if(!admitted()||!holderPinned_){r.state=PenState::Refused;return r;}
        if(!directory_->absent("pen-diagnostic.json")){penUsed_=true;r.state=PenState::Refused;return r;}
        const auto ack=directory_->read("pen-holder.exited",256);
        if(!ack.observed){if(!ack.missing){penUsed_=true;r.state=PenState::Refused;}return r;}
        penUsed_=true;r.state=PenState::Refused;
        if(!ack.bytes.endsWith('\n'))return r;
        const auto p=ack.bytes.chopped(1).split(' ');
        quint64 completed=0;
        if(p.size()!=7||p[0]!=config_.nonce||p[1]!=config_.pid||p[2]!=config_.start||p[3]!=holder_.holderPid.toLatin1()||p[4]!=holder_.holderStart.toLatin1()||p[5]!="0"||
           !reader_fixed_pen_result::detail::decimal(QString::fromLatin1(p[6]),&completed)||processStart(pid_t(holder_.holderPid.toLongLong()))==holder_.holderStart.toLatin1())return r;
        const auto scalar=directory_->read("pen-result.json",4096),trace=directory_->read("pen-events.raw",131072),stdoutBytes=directory_->read("pen-holder.stdout",4096);
        if(!scalar.observed||!trace.observed||!stdoutBytes.observed||!directory_->absent(".pending-pen-result")||!directory_->absent(".pending-pen-exited")||!admitted())return r;
        auto binding=holder_;binding.requestMs=handoff.requestMs;
        reader_fixed_pen_result::Completion completion{true,true,true,true,true,0,holder_.holderPid,holder_.holderStart,completed,nowMs(),stdoutBytes.bytes};
        r.facts=reader_fixed_pen_result::consume(scalar.bytes,trace.bytes,binding,completion,penClaim_);
        if(r.facts.accepted&&admitted())r.state=PenState::Accepted;
        return r;
    }
    void cancelPen()noexcept override{
        if(cancelUsed_)return;
        cancelUsed_=true;
        try{if(controls())directory_->publish("pen-cancel",".pending-pen-cancel",config_.nonce+' '+config_.pid+' '+config_.start+'\n');}catch(...){}
    }
    void report(const Result &r)noexcept override{
        if(reportUsed_)return;
        reportUsed_=true;
        try{
            if(!controls()||(r.observed&&!admitted()))return;
            const QJsonObject scalar{{"schema","write-reopen-sequence/1"},{"nonce",QString::fromLatin1(config_.nonce)},
                {"candidate_pid",QString::fromLatin1(config_.pid)},{"candidate_start",QString::fromLatin1(config_.start)},
                {"document_id",config_.target.documentId},{"page_id",config_.target.pageId},{"page_index",config_.target.pageIndex},{"page_count",config_.target.pageCount},
                {"observed",r.observed},{"refusal",r.refusal},{"began_ms",QString::number(r.began)},{"finished_ms",QString::number(r.finishedMs)},
                {"setup_end_ms",QString::number(config_.setupEnd)},{"reconciliation_required",r.reconciliationRequired},{"pen_cancel_requested",r.penCancelRequested},
                {"pen_result_sha256",QString::fromLatin1(r.pen.resultSha256)},{"pen_trace_sha256",QString::fromLatin1(r.pen.traceSha256)},
                {"blank_pair_sha256",QString::fromLatin1(r.captures[0].scalarSha256)},{"ink_pair_sha256",QString::fromLatin1(r.captures[1].scalarSha256)},
                {"reopened_pair_sha256",QString::fromLatin1(r.captures[2].scalarSha256)},
                {"close_dispatched",r.close.dispatched},{"closure_observed",r.closed.observed},{"reopen_dispatched",r.reopened.dispatched},
                {"pen_polls",int(r.penPolls)},{"closure_polls",int(r.closurePolls)},
                {"native_authority",false},{"render_authority",false},{"durable_authority",false},{"atomic_authority",false}};
            const auto bytes=QJsonDocument(scalar).toJson(QJsonDocument::Compact)+'\n';
            const auto published=directory_->publish("selected-result",".pending-result",bytes);
            if(!published.committed||(r.observed&&!admitted()))return;
            const auto completed=nowMs();
            if(!completed||(r.observed&&completed>=config_.setupEnd))return;
            // A linked scalar alone is forensic. Collection additionally requires
            // this original-byte/identity/clock-bound completion publication.
            const auto completion=config_.nonce+' '+config_.pid+' '+config_.start+' '+
                QCryptographicHash::hash(bytes,QCryptographicHash::Sha256).toHex()+' '+QByteArray::number(completed)+' '+QByteArray::number(config_.setupEnd)+'\n';
            directory_->publish("selected-complete",".pending-complete",completion);
        }catch(...){}
    }
    bool readyForRequest(){return admitted()&&holderReady().observed;}
    bool publishReady(){return readyForRequest()&&directory_->publish("selected-ready",".pending-ready",config_.nonce+' '+config_.pid+' '+config_.start+'\n').committed;}
    reader_write_reopen_files::Read request(){return directory_->read("selected-request",128);}
    bool consumeRequest(){
        if(requestUsed_)return false;
        requestUsed_=true;
        if(!admitted()||!directory_->absent(".pending-request")||!request_.open(directory_,"selected-request")||
           request_.bytes!=config_.nonce+' '+config_.pid+' '+config_.start+'\n')return false;
        requestPinned_=true;return admitted();
    }
private:
    NativeFiles(Config config,QByteArray boot):config_(std::move(config)),boot_(std::move(boot)){}
    bool controls()const{
        if(!directory_||!directory_->bound())return false;
        for(const auto &control:controls_)if(!control.unchanged(directory_))return false;
        if(requestPinned_&&!request_.unchanged(directory_))return false;
        return true;
    }
    static QByteArray proc(const QByteArray &path,int cap){
        const int fd=::open(path.constData(),O_RDONLY|O_NOFOLLOW|O_NONBLOCK|O_CLOEXEC);if(fd<0)return {};
        QByteArray bytes(cap+1,0);const auto n=::read(fd,bytes.data(),size_t(bytes.size()));const bool closed=::close(fd)==0;
        if(!closed||n<1||n>cap)return {};
        bytes.resize(int(n));return bytes;
    }
    static QByteArray processStart(pid_t pid){
        const auto bytes=proc("/proc/"+QByteArray::number(pid)+"/stat",4096);const int end=bytes.lastIndexOf(')');
        if(end<1)return {};
        const auto fields=bytes.mid(end+2).split(' ');
        if(fields.size()<20||fields[0]=="Z"||fields[0]=="X"||fields[0]=="x"||!reader_fixed_pen_result::detail::decimal(QString::fromLatin1(fields[19])))return {};
        return fields[19];
    }
    Config config_;QByteArray boot_;quint64 began_=0;
    std::shared_ptr<reader_write_reopen_files::Directory> directory_;
    std::array<Control,4> controls_;Control ready_,request_;
    reader_fixed_pen_result::Expected holder_;reader_fixed_pen_result::Claim penClaim_;
    std::array<bool,3> captureUsed_{{false,false,false}};
    bool holderPinned_=false,goUsed_=false,penUsed_=false,cancelUsed_=false,reportUsed_=false,requestUsed_=false,requestPinned_=false;
};
} // namespace reader_write_reopen
