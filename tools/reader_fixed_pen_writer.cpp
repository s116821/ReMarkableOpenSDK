// Fresh fixed POSIX writer image. No Qt dependency, application callbacks or
// input-device open/read/ioctl. The parent passes one already-qualified writer FD.
#include "reader_fixed_pen_holder.h"
#include <dirent.h>
#include <linux/input.h>
#include <sys/stat.h>
#include <sys/sysmacros.h>
#include <time.h>
#include <limits>
#ifndef READER_FIXED_PEN_WRITER_OWNED_TESTING
#if !defined(__arm__) || !defined(__ARM_PCS_VFP) || __BYTE_ORDER__!=__ORDER_LITTLE_ENDIAN__
#error Native fixed writer requires ARM32 little-endian hard-float.
#endif
static_assert(sizeof(void*)==4&&sizeof(input_event)==16&&alignof(input_event)==4);
static_assert(offsetof(input_event,type)==8&&offsetof(input_event,code)==10&&offsetof(input_event,value)==12);
#endif
namespace {
using namespace reader_fixed_pen_holder;
#ifdef READER_FIXED_PEN_WRITER_OWNED_TESTING
// Explicit host-owned file model, preserving the measured 16-byte kernel wire.
struct KernelRecord {std::uint32_t seconds=0,micros=0;std::uint16_t type=0,code=0;std::int32_t value=0;};
constexpr bool owned=true;
#else
using KernelRecord=input_event;
constexpr bool owned=false;
#endif
static_assert(sizeof(KernelRecord)==16&&offsetof(KernelRecord,type)==8&&offsetof(KernelRecord,code)==10&&offsetof(KernelRecord,value)==12);
static_assert(sizeof(detail::ChildRecord)==36&&alignof(detail::ChildRecord)==4);
bool number(const char *s,std::uint64_t&n)noexcept {
    if(!s||!*s||(*s=='0'&&s[1]))return false;
    n=0;unsigned count=0;
    for(;*s;++s){if(++count>20||*s<'0'||*s>'9')return false;const auto d=std::uint64_t(*s-'0');if(n>(UINT64_MAX-d)/10)return false;n=n*10+d;}
    return true;
}
std::uint64_t milliseconds()noexcept {
    timespec t{};if(::clock_gettime(CLOCK_BOOTTIME,&t)||t.tv_sec<0||t.tv_nsec<0||t.tv_nsec>=1000000000)return 0;
    const auto part=std::uint64_t(t.tv_nsec)/1000000;
    if(std::uint64_t(t.tv_sec)>(UINT64_MAX-part)/1000)return 0;
    return std::uint64_t(t.tv_sec)*1000+part;
}
unsigned threads()noexcept {
    DIR *d=::opendir("/proc/self/task");if(!d)return 0;
    unsigned count=0;errno=0;while(const auto *e=::readdir(d))if(e->d_name[0]!='.')++count;
    const int error=errno;return ::closedir(d)==0&&!error?count:0;
}
bool descriptors()noexcept {
    DIR *d=::opendir("/proc/self/fd");if(!d)return false;const int own=::dirfd(d);bool okay=true;errno=0;
    while(const auto *e=::readdir(d)){
        if(e->d_name[0]=='.')continue;
        std::uint64_t n=0;if(!number(e->d_name,n)||n>std::uint64_t(std::numeric_limits<int>::max())||(n>=7&&int(n)!=own))okay=false;
    }
    const int error=errno;return ::closedir(d)==0&&!error&&okay;
}
bool roles()noexcept {
    for(int fd:{3,4,5,6}){
        const int flags=::fcntl(fd,F_GETFL),closeFlags=::fcntl(fd,F_GETFD);struct stat st{};
        if(flags<0||closeFlags<0||(closeFlags&FD_CLOEXEC)||!(flags&O_NONBLOCK)||::fstat(fd,&st))return false;
        if((flags&O_ACCMODE)!=((fd==3||fd==6)?O_WRONLY:O_RDONLY))return false;
        if(fd!=3&&!S_ISFIFO(st.st_mode))return false;
    }
    return descriptors();
}
class WriterIO {
public:
    WriterIO(pid_t parent,std::uint64_t start,std::uint64_t device,std::uint64_t inode,std::uint64_t rdev)
        :parent_(parent),start_(start),device_(device),inode_(inode),rdev_(rdev){}
    bool afterForkChild()noexcept{return true;} // already a fresh exec image
    std::uint64_t nowMs()const noexcept{return milliseconds();}
    bool childAllowed()const noexcept {
        struct stat st{};
        if(::getppid()!=parent_||detail::processStart(parent_)!=start_||::fstat(3,&st)||std::uint64_t(st.st_dev)!=device_||
           std::uint64_t(st.st_ino)!=inode_||std::uint64_t(st.st_rdev)!=rdev_)return false;
#ifdef READER_FIXED_PEN_WRITER_OWNED_TESTING
        return (S_ISREG(st.st_mode)||S_ISFIFO(st.st_mode))&&st.st_uid==::geteuid();
#else
        struct stat named{};return S_ISCHR(st.st_mode)&&st.st_uid==0&&major(st.st_rdev)==13&&minor(st.st_rdev)==65&&
            !::lstat("/dev/input/event1",&named)&&S_ISCHR(named.st_mode)&&named.st_uid==0&&named.st_dev==st.st_dev&&named.st_ino==st.st_ino&&named.st_rdev==st.st_rdev;
#endif
    }
    reader_fixed_pen::Write emitWriter(const Event *events,std::size_t count)noexcept {
        if(count<1||count>7)return {-1,EINVAL};
        std::array<KernelRecord,7> records{};
        for(std::size_t i=0;i<count;++i){records[i].type=events[i].type;records[i].code=events[i].code;records[i].value=events[i].value;}
        errno=0;const auto n=::write(3,records.data(),count*sizeof(KernelRecord));return {n,n<0?errno:0};
    }
private:
    pid_t parent_;std::uint64_t start_,device_,inode_,rdev_;
};
int profile()noexcept {
    const auto count=threads();const auto pid=::getpid();const auto start=detail::processStart(pid);if(!count||!start)return 2;
    char bytes[512];const int n=std::snprintf(bytes,sizeof bytes,
        "{\"schema\":\"fixed-pen-writer-profile/1\",\"pid\":\"%ld\",\"start\":\"%llu\",\"threads\":%u,\"kernel_record_bytes\":%zu,\"type_offset\":%zu,\"code_offset\":%zu,\"value_offset\":%zu,\"owned_model\":%s,\"input_io\":false}\n",
        static_cast<long>(pid),static_cast<unsigned long long>(start),count,sizeof(KernelRecord),offsetof(KernelRecord,type),offsetof(KernelRecord,code),offsetof(KernelRecord,value),owned?"true":"false");
    return n>0&&std::size_t(n)<sizeof bytes&&::write(STDOUT_FILENO,bytes,std::size_t(n))==n?0:2;
}
}
int main(int argc,char **argv) {
    if(argc==2&&!std::strcmp(argv[1],"--profile"))return profile(); // no device FD inspection/access
    if(argc!=8||std::strcmp(argv[1],"--writer")||::geteuid()!=0)return 2;
    for(const char *name:{"LD_PRELOAD","LD_BIND_NOW","LD_LIBRARY_PATH","XOVI_ROOT"})if(::getenv(name))return 2;
    std::uint64_t values[6]{};for(unsigned i=0;i<6;++i)if(!number(argv[i+2],values[i]))return 2;
    if(values[0]<=1||values[0]>std::uint64_t(std::numeric_limits<pid_t>::max())||!values[1]||!values[2]||threads()!=1||!roles())return 2;
    const auto budget=reader_fixed_pen::reserve(values[2]);const auto now=milliseconds();
    if(!budget.valid()||!now||now<budget.began||now>=budget.strokeEnd)return 2;
    WriterIO io(pid_t(values[0]),values[1],values[3],values[4],values[5]);if(!io.childAllowed())return 2;
    detail::child(io,4,5,6,pid_t(values[0]),budget);
}
