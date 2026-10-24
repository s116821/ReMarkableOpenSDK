// ARM-only, fixed research writer. No QCoreApplication or threads before fork.
#include "reader_fixed_pen_holder_wire.h"
#include <QCryptographicHash>
#include <dirent.h>
#include <linux/input.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/sysmacros.h>
#include <time.h>
#include <limits>

#ifdef READER_FIXED_PEN_HOLDER_TESTING
#error Test fencing seam must never compile into native executable.
#endif
#if !defined(__arm__) || !defined(__ARM_PCS_VFP) || __BYTE_ORDER__!=__ORDER_LITTLE_ENDIAN__
#error Native fixed holder requires ARM32 little-endian hard-float vendor ABI.
#endif
static_assert(sizeof(void*)==4&&sizeof(input_event)==16&&alignof(input_event)==4);
static_assert(offsetof(input_event,type)==8&&offsetof(input_event,code)==10&&offsetof(input_event,value)==12);
namespace {
using namespace reader_fixed_pen_holder;
using Fd=reader_fixed_pen_holder::detail::Fd;
constexpr char Device[]="/dev/input/event1",DeviceName[]="Wacom I2C Digitizer";
constexpr std::uint64_t TraceCap=131072;
std::uint64_t milliseconds()noexcept {
    timespec t{};
    if(::clock_gettime(CLOCK_BOOTTIME,&t)||t.tv_sec<0||t.tv_nsec<0||t.tv_nsec>=1000000000)return 0;
    const auto part=std::uint64_t(t.tv_nsec)/1000000;
    if(std::uint64_t(t.tv_sec)>(std::numeric_limits<std::uint64_t>::max()-part)/1000)return 0;
    return std::uint64_t(t.tv_sec)*1000+part;
}
bool number(const QByteArray&b,std::uint64_t&n,bool zero=false) {
    if(b.isEmpty()||b.size()>20||(b.size()>1&&b[0]=='0'))return false;
    n=0;
    for(char c:b){if(c<'0'||c>'9'||n>(std::numeric_limits<std::uint64_t>::max()-std::uint64_t(c-'0'))/10)return false;n=n*10+std::uint64_t(c-'0');}
    return zero||n!=0;
}
bool nonce(const QByteArray&b){if(b.size()!=32)return false;for(char c:b)if(!((c>='0'&&c<='9')||(c>='a'&&c<='f')))return false;return true;}
bool boot(const QByteArray&b){if(b.size()!=36)return false;for(int i=0;i<36;++i){if(i==8||i==13||i==18||i==23){if(b[i]!='-')return false;}else if(!((b[i]>='0'&&b[i]<='9')||(b[i]>='a'&&b[i]<='f')))return false;}return true;}
bool same(const struct stat&a,const struct stat&b,bool times=true)noexcept {
    return a.st_dev==b.st_dev&&a.st_ino==b.st_ino&&a.st_mode==b.st_mode&&a.st_uid==b.st_uid&&a.st_gid==b.st_gid&&
        a.st_nlink==b.st_nlink&&a.st_size==b.st_size&&(!times||(a.st_mtim.tv_sec==b.st_mtim.tv_sec&&a.st_mtim.tv_nsec==b.st_mtim.tv_nsec&&a.st_ctim.tv_sec==b.st_ctim.tv_sec&&a.st_ctim.tv_nsec==b.st_ctim.tv_nsec));
}
bool sameData(const struct stat&a,const struct stat&b)noexcept {
    return a.st_dev==b.st_dev&&a.st_ino==b.st_ino&&a.st_mode==b.st_mode&&a.st_uid==b.st_uid&&a.st_gid==b.st_gid&&a.st_size==b.st_size&&
        a.st_mtim.tv_sec==b.st_mtim.tv_sec&&a.st_mtim.tv_nsec==b.st_mtim.tv_nsec;
}
bool privateFile(const struct stat&s,off_t cap)noexcept{return S_ISREG(s.st_mode)&&(s.st_mode&07777)==0600&&s.st_uid==0&&s.st_nlink==1&&s.st_size>=0&&s.st_size<=cap;}
QByteArray proc(const QByteArray&path,int cap=4096) {
    Fd fd(::open(path.constData(),O_RDONLY|O_CLOEXEC|O_NOFOLLOW|O_NONBLOCK));if(fd.value<0)return {};
    QByteArray bytes(cap+1,0);const auto n=::read(fd.value,bytes.data(),std::size_t(bytes.size()));
    if(n<1||n>cap||!fd.close())return {};
    bytes.resize(int(n));return bytes;
}
bool singleThreaded()noexcept {
    DIR*d=::opendir("/proc/self/task");if(!d)return false;unsigned count=0;errno=0;
    while(const auto*e=::readdir(d))if(e->d_name[0]!='.')++count;
    const int error=errno;return ::closedir(d)==0&&!error&&count==1;
}
struct Control {
    Fd fd;QByteArray bytes;struct stat stat{};const char*name=nullptr;
    bool open(int root,const char*n) {
        name=n;fd.value=::openat(root,n,O_RDONLY|O_NOFOLLOW|O_NONBLOCK|O_CLOEXEC);fd.closed=fd.value<0;
        if(fd.value<0||::fstat(fd.value,&stat)||!privateFile(stat,256))return false;
        char data[257];const auto count=::pread(fd.value,data,sizeof data,0);
        if(count!=stat.st_size)return false;
        bytes=QByteArray(data,int(count));return unchanged(root);
    }
    bool unchanged(int root)const {
        struct stat a{},b{};char data[257];
        return fd.value>=0&&!::fstat(fd.value,&a)&&same(a,stat)&&!::fstatat(root,name,&b,AT_SYMLINK_NOFOLLOW)&&same(a,b)&&
            ::pread(fd.value,data,sizeof data,0)==bytes.size()&&QByteArray(data,bytes.size())==bytes;
    }
};
class Context {
public:
    explicit Context(Binding&b):b_(b),path_("/run/rmb-qt-probe-"+b.nonce){}
    bool start() {
        root_.value=::open(path_.constData(),O_RDONLY|O_DIRECTORY|O_NOFOLLOW|O_CLOEXEC);root_.closed=root_.value<0;
        if(root_.value<0||::fstat(root_.value,&stat_)||!S_ISDIR(stat_.st_mode)||stat_.st_uid||((stat_.st_mode&07777)!=0700))return false;
        if(!owner_.open(root_.value,"owner")||owner_.bytes!=b_.nonce||!attempt_.open(root_.value,"attempt.identity")||
           attempt_.bytes!=QByteArray::number(b_.candidatePid)+' '+QByteArray::number(b_.candidateStart)+'\n'||
           !deadline_.open(root_.value,"deadline")||!window_.open(root_.value,"candidate.window"))return false;
        auto d=deadline_.bytes.split(' '),w=window_.bytes.split(' ');std::uint64_t armed=0,end=0,wa=0,we=0,begin=0,ends=0;
        if(d.size()!=2||!d[1].endsWith('\n')||!number(d[0],armed)||!number(d[1].chopped(1),end)||end<armed||end-armed!=360||
           w.size()!=5||w[0]!=b_.nonce||!w[4].endsWith('\n')||!number(w[1],wa)||!number(w[2],we)||!number(w[3],begin)||!number(w[4].chopped(1),ends)||
           wa!=armed||we!=end||begin<armed||ends<begin||ends-begin!=90||end<185||ends>end-185||ends>std::numeric_limits<std::uint64_t>::max()/1000||
           b_.setupEnd!=ends*1000)return false;
        began_=begin*1000;return context();
    }
    bool basic()const {
        struct stat path{};return root_.value>=0&&!::lstat(path_.constData(),&path)&&S_ISDIR(path.st_mode)&&
            path.st_dev==stat_.st_dev&&path.st_ino==stat_.st_ino&&path.st_uid==0&&(path.st_mode&07777)==0700&&
            owner_.unchanged(root_.value)&&attempt_.unchanged(root_.value)&&deadline_.unchanged(root_.value)&&window_.unchanged(root_.value)&&
            (!claim_.name||claim_.unchanged(root_.value))&&(!b_.goClaimed||!go_.name||go_.unchanged(root_.value));
    }
    bool context()const {
        if(!basic())return false;
        const auto now=milliseconds();if(!now||now<began_||now>=b_.setupEnd)return false;
        for(const char*n:{"pen-cancel","stop.request","selected-cancel","restore.claim"})if(!absent(n))return false;
        if(proc("/proc/sys/kernel/random/boot_id",64)!=b_.boot+'\n')return false;
        const auto stat=proc("/proc/"+QByteArray::number(b_.candidatePid)+"/stat");const int end=stat.lastIndexOf(')');
        if(end<1)return false;
        const auto fields=stat.mid(end+2).split(' ');
        if(fields.size()<20||fields[0]=="Z"||fields[0]=="X"||fields[0]=="x"||fields[19]!=QByteArray::number(b_.candidateStart))return false;
        const auto after=milliseconds();return after>=now&&after<b_.setupEnd;
    }
    bool absent(const char*n)const {struct stat s{};return ::fstatat(root_.value,n,&s,AT_SYMLINK_NOFOLLOW)&&errno==ENOENT;}
    bool claim(const QByteArray&bytes) {
        if(!basic())return false;
        Fd fd(::openat(root_.value,"pen-holder.claim",O_WRONLY|O_CREAT|O_EXCL|O_NOFOLLOW|O_CLOEXEC,0600));
        if(fd.value<0||::write(fd.value,bytes.constData(),std::size_t(bytes.size()))!=bytes.size())return false;
        struct stat written{},named{};return !::fstat(fd.value,&written)&&privateFile(written,256)&&claim_.open(root_.value,"pen-holder.claim")&&
            claim_.bytes==bytes&&fd.close()&&!::fstatat(root_.value,"pen-holder.claim",&named,AT_SYMLINK_NOFOLLOW)&&same(written,named)&&basic();
    }
    int takeGo() {
        if(!context())return -1;
        if(absent("pen-go.request"))return 0;
        b_.goClaimed=true; // One holder claim + sticky in-process consumption, before parse.
        if(!go_.open(root_.value,"pen-go.request"))return -1;
        const auto prefix=b_.nonce+' '+QByteArray::number(b_.candidatePid)+' '+QByteArray::number(b_.candidateStart)+' ';
        if(!go_.bytes.startsWith(prefix)||!go_.bytes.endsWith('\n')||!number(go_.bytes.mid(prefix.size()).chopped(1),b_.requestMs))return -1;
        b_.beganMs=milliseconds();
        return b_.beganMs>=b_.requestMs&&b_.beganMs-b_.requestMs<=250&&b_.beganMs<=std::numeric_limits<std::uint64_t>::max()-3000&&
            b_.beganMs+3000<b_.setupEnd&&context()?1:-1;
    }
    bool publish(const char*name,const char*pending,const QByteArray&bytes,bool positive) {
        const auto permit=[&]{return basic()&&(!positive||(context()&&milliseconds()<b_.beganMs+3000));};
        if(bytes.isEmpty()||bytes.size()>4096||!permit()||!absent(name)||!absent(pending))return false;
        Fd fd(::openat(root_.value,pending,O_WRONLY|O_CREAT|O_EXCL|O_NOFOLLOW|O_CLOEXEC,0600));if(fd.value<0)return false;
        if(::write(fd.value,bytes.constData(),std::size_t(bytes.size()))!=bytes.size())return false;
        struct stat sealed{},pinned{},named{},linked{};
        if(::fstat(fd.value,&sealed)||!privateFile(sealed,4096)||sealed.st_size!=bytes.size())return false;
        Fd pin(::openat(root_.value,pending,O_PATH|O_NOFOLLOW|O_CLOEXEC));
        if(pin.value<0||::fstat(pin.value,&pinned)||!same(sealed,pinned)||!fd.close()||!permit()||
           ::fstatat(root_.value,pending,&named,AT_SYMLINK_NOFOLLOW)||!same(sealed,named)||!absent(name))return false;
        if(::linkat(root_.value,pending,root_.value,name,0)||!permit())return false;
        if(::fstat(pin.value,&pinned)||::fstatat(root_.value,pending,&named,AT_SYMLINK_NOFOLLOW)||
           ::fstatat(root_.value,name,&linked,AT_SYMLINK_NOFOLLOW)||!same(pinned,named)||!same(pinned,linked)||pinned.st_nlink!=2||
           !sameData(sealed,pinned)||!permit())return false;
        if(::unlinkat(root_.value,pending,0)||::fstat(pin.value,&pinned)||::fstatat(root_.value,name,&named,AT_SYMLINK_NOFOLLOW)||
           !same(pinned,named)||!sameData(sealed,pinned)||pinned.st_nlink!=1||!absent(pending)||!permit())return false;
        return pin.close(); // Failed post-link admission deliberately preserves forensic partials.
    }
    int root()const noexcept{return root_.value;}
    bool closeAll()noexcept {bool ok=go_.fd.close();ok=claim_.fd.close()&&ok;ok=window_.fd.close()&&ok;ok=deadline_.fd.close()&&ok;ok=attempt_.fd.close()&&ok;ok=owner_.fd.close()&&ok;return root_.close()&&ok;}
private:
    Binding&b_;QByteArray path_;Fd root_;struct stat stat_{};Control owner_,attempt_,deadline_,window_,go_,claim_;std::uint64_t began_=0;
};
bool bit(const unsigned char*b,unsigned n)noexcept{return (b[n/8]&(1u<<(n%8)))!=0;}
bool query(int fd,Snapshot&out)noexcept {
    input_id id{};char name[128]{};
    if(::ioctl(fd,EVIOCGID,&id)||id.bustype!=0x18||id.vendor!=0x2d1f||id.product!=0x95||id.version!=0x1231||
       ::ioctl(fd,EVIOCGNAME(sizeof name),name)!=int(sizeof DeviceName)||std::memcmp(name,DeviceName,sizeof DeviceName))return false;
    std::array<unsigned char,(EV_MAX+8)/8> events{};std::array<unsigned char,(KEY_MAX+8)/8> keys{},pressed{};std::array<unsigned char,(ABS_MAX+8)/8> axes{};
    if(::ioctl(fd,EVIOCGBIT(0,events.size()),events.data())!=int(events.size())||::ioctl(fd,EVIOCGBIT(EV_KEY,keys.size()),keys.data())!=int(keys.size())||
       ::ioctl(fd,EVIOCGBIT(EV_ABS,axes.size()),axes.data())!=int(axes.size())||::ioctl(fd,EVIOCGKEY(pressed.size()),pressed.data())!=int(pressed.size()))return false;
    for(unsigned n=0;n<events.size()*8;++n)if(bit(events.data(),n)!=(n==EV_SYN||n==EV_KEY||n==EV_ABS))return false;
    for(unsigned n=0;n<keys.size()*8;++n) {
        const bool known=n==BTN_TOOL_PEN||n==BTN_TOOL_RUBBER||n==BTN_TOUCH||n==BTN_STYLUS||n==BTN_STYLUS2;
        if(bit(keys.data(),n)!=known||(!known&&bit(pressed.data(),n)))return false;
    }
    for(unsigned n=0;n<axes.size()*8;++n)if(bit(axes.data(),n)!=(n==ABS_X||n==ABS_Y||n==ABS_PRESSURE||n==ABS_DISTANCE||n==ABS_TILT_X||n==ABS_TILT_Y))return false;
    constexpr int codes[]={0,1,24,25,26,27},low[]={0,0,0,0,-9000,-9000},high[]={20966,15725,4095,255,9000,9000},resolution[]={100,100,0,0,0,0};
    int values[6]{};
    for(unsigned i=0;i<6;++i){input_absinfo a{};if(::ioctl(fd,EVIOCGABS(codes[i]),&a)||a.minimum!=low[i]||a.maximum!=high[i]||a.fuzz||a.flat||a.resolution!=resolution[i]||a.value<low[i]||a.value>high[i])return false;values[i]=a.value;}
    out.x=values[0];out.y=values[1];out.pressure=values[2];out.distance=values[3];out.tiltX=values[4];out.tiltY=values[5];
    out.pen=bit(pressed.data(),BTN_TOOL_PEN);out.touch=bit(pressed.data(),BTN_TOUCH);out.rubber=bit(pressed.data(),BTN_TOOL_RUBBER);
    out.stylus=bit(pressed.data(),BTN_STYLUS);out.stylus2=bit(pressed.data(),BTN_STYLUS2);return true;
}
class NativeIO {
public:
    explicit NativeIO(Context&context):context_(context){}
    ~NativeIO(){for(int*fd:{&observer_,&writer_,&recovery_,&trace_})if(*fd>=0)::close(*fd);}
    bool prepare() {
        observer_=::open(Device,O_RDONLY|O_NOFOLLOW|O_NONBLOCK|O_CLOEXEC);
        writer_=::open(Device,O_WRONLY|O_NOFOLLOW|O_NONBLOCK|O_CLOEXEC);
        recovery_=::open(Device,O_WRONLY|O_NOFOLLOW|O_NONBLOCK|O_CLOEXEC);
        if(observer_<0||writer_<0||recovery_<0||::fstat(observer_,&device_)||!identityValid())return false;
        trace_=::openat(context_.root(),"pen-events.raw",O_RDWR|O_CREAT|O_EXCL|O_NOFOLLOW|O_CLOEXEC,0600);if(trace_<0)return false;
        Snapshot a{},b{};return snapshot(a)&&snapshot(b)&&reader_fixed_pen_stream::same(a,b)&&reader_fixed_pen_stream::released(a)&&!a.tiltX&&!a.tiltY&&observerEmpty();
    }
    std::uint64_t nowMs()const noexcept{return milliseconds();}
    bool context()const{return context_.context();}
    bool childAllowed()const noexcept{return true;} // Parent cancellation pipe + original clock gate every child frame.
    bool snapshot(Snapshot&s)noexcept {if(!identityValid()||!query(observer_,s))return false;last_=s;if(!haveInitial_){initial_=s;haveInitial_=true;}return true;}
    bool releasedState()noexcept {Snapshot s{};return snapshot(s)&&reader_fixed_pen_stream::same(s,initial_)&&reader_fixed_pen_stream::released(s);}
    bool identityValid()noexcept {
        struct stat path{};if(::lstat(Device,&path))return false;
        const int descriptors[]={observer_,writer_,recovery_};
        for(unsigned i=0;i<3;++i) {
            const int fd=descriptors[i];if(i==1&&writer_<0&&parentWriterClosed_)continue;
            struct stat s{};if(fd<0||::fstat(fd,&s)||!S_ISCHR(s.st_mode)||s.st_uid||s.st_dev!=device_.st_dev||s.st_ino!=device_.st_ino||
                s.st_rdev!=device_.st_rdev||s.st_dev!=path.st_dev||s.st_ino!=path.st_ino||s.st_rdev!=path.st_rdev||!S_ISCHR(path.st_mode)||path.st_uid||
                major(s.st_rdev)!=13||minor(s.st_rdev)!=65)return false;
        }
        Snapshot ignored{};identity_=query(observer_,ignored)&&(!snapshotMatch_||reader_fixed_pen_stream::same(ignored,last_));return identity_;
    }
    bool observerEmpty()const noexcept {pollfd p{observer_,POLLIN,0};return ::poll(&p,1,0)==0;}
    bool afterForkChild()noexcept {
        bool ok=closeFd(observer_);ok=closeFd(recovery_)&&ok;ok=closeFd(trace_)&&ok;return context_.closeAll()&&ok;
    }
    bool afterForkParent()noexcept{parentWriterClosed_=closeFd(writer_);return parentWriterClosed_&&singleThreaded();}
    reader_fixed_pen::Write emitWriter(const Event*e,std::size_t n)noexcept{return emitRecords(writer_,e,n);}
    reader_fixed_pen::Write emitRecovery(const Event*e,std::size_t n)noexcept{return emitRecords(recovery_,e,n);}
    bool observe(Stream&stream)noexcept {
        std::array<input_event,256> events{};const ssize_t n=::read(observer_,events.data(),sizeof events);
        if(n<0)return (errno==EAGAIN||errno==EWOULDBLOCK)?true:poison();
        if(n==0)return poison();
        lastRead_=nowMs();
        if(traceBytes_+std::uint64_t(n)>TraceCap||::write(trace_,events.data(),std::size_t(n))!=n)return poison();
        traceBytes_+=std::uint64_t(n);if(n%ssize_t(sizeof(input_event)))return poison();
        for(ssize_t i=0;i<n/ssize_t(sizeof(input_event));++i)if(!stream.accept({events[std::size_t(i)].type,events[std::size_t(i)].code,events[std::size_t(i)].value}))return poison();
        return rawGood_;
    }
    bool waitParent(unsigned ms)const noexcept {pollfd p{observer_,POLLIN,0};return ::poll(&p,1,int(ms))>=0&&!(p.revents&(POLLERR|POLLHUP|POLLNVAL));}
    bool confirmReleased(Stream&stream,std::uint64_t end)noexcept {
        if(!rawGood_||!stream.report().valid)return false;
        lastRead_=nowMs();auto last=lastRead_;
        while(true) {
            const auto at=nowMs();if(at<last||at>=end)return false;last=at;
            if(!observe(stream))return false;
            const auto after=nowMs();if(after<at||after>=end)return false;
            if(after>=lastRead_&&after-lastRead_>=20) {
                quiet_=true;Snapshot a{},b{};
                if(!snapshot(a)||!snapshot(b)||!reader_fixed_pen_stream::same(a,b)||!observerEmpty()||!identityValid())return false;
                snapshotMatch_=reader_fixed_pen_stream::same(stream.report().state,b);
                return snapshotMatch_&&stream.releasedMatches(b);
            }
            if(!waitParent(2))return false;
        }
    }
    Snapshot finalSnapshot()const noexcept{return last_;}
    bool quietConfirmed()const noexcept{return quiet_;}
    bool snapshotMatched()const noexcept{return snapshotMatch_;}
    bool finish() {
        bool okay=true;
        if(trace_>=0) {
            struct stat s{},named{};QCryptographicHash hash(QCryptographicHash::Sha256);std::array<char,4096> bytes{};
            bool valid=!::fstat(trace_,&s)&&privateFile(s,off_t(TraceCap));std::uint64_t offset=0;
            while(valid&&offset<std::uint64_t(s.st_size)) {const auto amount=std::size_t(qMin<std::uint64_t>(bytes.size(),std::uint64_t(s.st_size)-offset));const auto n=::pread(trace_,bytes.data(),amount,off_t(offset));if(n!=ssize_t(amount)){valid=false;break;}hash.addData(QByteArrayView(bytes.data(),n));offset+=std::uint64_t(n);}
            struct stat after{};valid=valid&&!::fstat(trace_,&after)&&same(s,after)&&!::fstatat(context_.root(),"pen-events.raw",&named,AT_SYMLINK_NOFOLLOW)&&same(s,named);
            evidence_.traceBytes=valid?std::uint64_t(s.st_size):traceBytes_;evidence_.traceSha256=valid?hash.result().toHex():QByteArray{};
            Fd pin(::openat(context_.root(),"pen-events.raw",O_PATH|O_NOFOLLOW|O_CLOEXEC));struct stat pinned{};
            valid=valid&&pin.value>=0&&!::fstat(pin.value,&pinned)&&same(s,pinned);
            traceClosed_=closeFd(trace_);valid=valid&&traceClosed_&&!::fstatat(context_.root(),"pen-events.raw",&after,AT_SYMLINK_NOFOLLOW)&&same(s,after);
            traceClosed_=pin.close()&&traceClosed_;
            evidence_.traceComplete=rawGood_&&valid&&evidence_.traceBytes==traceBytes_;okay=valid&&okay;
        }else {traceClosed_=true;evidence_.traceComplete=false;}
        bool closed=closeFd(observer_);closed=closeFd(writer_)&&closed;closed=closeFd(recovery_)&&closed;deviceClosed_=closed;
        evidence_.quiet=quiet_;evidence_.snapshotMatch=snapshotMatch_;evidence_.descriptorIdentity=identity_;
        return okay&&closed;
    }
    bool deviceFdsClosed()const noexcept{return deviceClosed_;}
    bool traceClosed()const noexcept{return traceClosed_;}
    Evidence evidence()const{return evidence_;}
private:
    static bool closeFd(int&fd)noexcept {if(fd<0)return true;const int original=fd;fd=-1;return ::close(original)==0;}
    static reader_fixed_pen::Write emitRecords(int fd,const Event*e,std::size_t count)noexcept {
        if(fd<0||count<1||count>7)return {-1,EINVAL};
        std::array<input_event,7> records{};
        for(std::size_t i=0;i<count;++i){records[i].type=e[i].type;records[i].code=e[i].code;records[i].value=e[i].value;}
        errno=0;const auto n=::write(fd,records.data(),count*sizeof(input_event));return {n,n<0?errno:0};
    }
    bool poison()noexcept{rawGood_=false;return false;}
    Context&context_;int observer_=-1,writer_=-1,recovery_=-1,trace_=-1;struct stat device_{};
    Snapshot initial_{},last_{};bool haveInitial_=false,identity_=false,rawGood_=true,quiet_=false,snapshotMatch_=false,deviceClosed_=false,traceClosed_=false,parentWriterClosed_=false;
    std::uint64_t traceBytes_=0,lastRead_=0;Evidence evidence_{};
};
int failure()noexcept {const char message[]="fixed pen holder admission failed\n";const auto n=::write(STDERR_FILENO,message,sizeof message-1);(void)n;return 2;}
}
int main(int argc,char**argv) {
    if(argc!=7||std::strcmp(argv[1],"--wait")||::geteuid()!=0||std::strcmp(qVersion(),"6.10.3"))return failure();
    for(const char*n:{"LD_PRELOAD","LD_BIND_NOW","LD_LIBRARY_PATH","XOVI_ROOT"})if(::getenv(n))return failure();
    Binding binding;binding.nonce=argv[2];binding.boot=argv[5];
    if(!nonce(binding.nonce)||!boot(binding.boot)||!number(argv[3],binding.candidatePid)||binding.candidatePid>std::uint64_t(std::numeric_limits<pid_t>::max())||
       !number(argv[4],binding.candidateStart)||!number(argv[6],binding.setupEnd))return failure();
    const auto at=milliseconds();const auto group=proc("/proc/self/cgroup");
    if(!at||binding.setupEnd<=at||binding.setupEnd-at>90000||group.isEmpty()||group.contains("xochitl")||!singleThreaded())return failure();
    struct sigaction ignore{};ignore.sa_handler=SIG_IGN;if(::sigemptyset(&ignore.sa_mask)||::sigaction(SIGPIPE,&ignore,nullptr))return failure();
    struct sigaction childDisposition{};childDisposition.sa_handler=SIG_DFL;
    if(::sigemptyset(&childDisposition.sa_mask)||::sigaction(SIGCHLD,&childDisposition,nullptr))return failure();
    binding.holderPid=std::uint64_t(::getpid());binding.holderStart=detail::processStart(::getpid());if(!binding.holderStart)return failure();
    Context context(binding);if(!context.start())return failure();
    const auto ready=binding.nonce+' '+QByteArray::number(binding.candidatePid)+' '+QByteArray::number(binding.candidateStart)+' '+binding.boot+' '+
        QByteArray::number(binding.holderPid)+' '+QByteArray::number(binding.holderStart)+' '+QByteArray::number(binding.setupEnd)+'\n';
    if(!context.claim(ready))return failure();
    NativeIO io(context);Result result;
    if(!io.prepare()){io.finish();result.refusal="device-profile";result.finishedMs=milliseconds();result.deviceFdsClosed=io.deviceFdsClosed();result.traceClosed=io.traceClosed();}
    else if(!context.publish("pen-holder.ready",".pending-pen-ready",ready,false))return failure();
    else {
        int go=0;while((go=context.takeGo())==0){pollfd none{};if(::poll(&none,0,2)<0)break;}
        if(go==1&&singleThreaded())result=run(io,reader_fixed_pen::reserve(binding.beganMs));
        else {io.finish();result.refusal="go-admission";result.finishedMs=milliseconds();result.deviceFdsClosed=io.deviceFdsClosed();result.traceClosed=io.traceClosed();}
    }
    QByteArray bytes;
    if(result.diagnostic) {
        bytes=diagnosticWire(binding,result);
        if(!context.publish("pen-diagnostic.json",".pending-pen-diagnostic",bytes,false))return 119;
        const auto n=::write(STDOUT_FILENO,bytes.constData(),std::size_t(bytes.size()));(void)n;
        // No late recovery or normal result. Retain owners while reaping only,
        // under original setup end. An uninterruptible child remains an outer
        // actor/backstop hazard, not a successful release or OS recovery proof.
        while(milliseconds()&&milliseconds()<binding.setupEnd){int status=0;const auto p=::waitpid(result.childPid,&status,WNOHANG);if(p==result.childPid||p<0)break;pollfd none{};if(::poll(&none,0,2)<0)break;}
        io.finish();context.closeAll();return 119;
    }
    bytes=normalWire(binding,result,io.evidence());
    if(!context.publish("pen-result.json",".pending-pen-result",bytes,result.accepted))return 1;
    if(::write(STDOUT_FILENO,bytes.constData(),std::size_t(bytes.size()))!=bytes.size())return 1;
    const bool admitted=!result.accepted||(context.context()&&milliseconds()<binding.beganMs+3000);
    if(!context.closeAll()||!admitted)return 1;
    return result.accepted?0:1;
}
