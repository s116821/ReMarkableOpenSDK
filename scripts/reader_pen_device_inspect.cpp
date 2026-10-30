// Read-only RM2 profile query. No read/write/ioctl mutation of input events.
#include <linux/input.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/sysmacros.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <array>
#include <cstddef>
#include <cerrno>
#include <cstdio>
#include <cstring>

static_assert(sizeof(void *)==4 && sizeof(input_event)==16 && alignof(input_event)==4,
              "vendor ARM32 event ABI required");
static_assert(__BYTE_ORDER__==__ORDER_LITTLE_ENDIAN__,"vendor byte order");
static_assert(offsetof(input_event,type)==8 && offsetof(input_event,code)==10 && offsetof(input_event,value)==12,"vendor field offsets");
namespace {
constexpr char Device[]="/dev/input/event1";
constexpr char Name[]="Wacom I2C Digitizer";
struct Holder {int fd=-1;~Holder(){if(fd>=0)::close(fd);}};
bool same(const struct stat &a,const struct stat &b) {
    return S_ISCHR(a.st_mode) && S_ISCHR(b.st_mode) && a.st_rdev==b.st_rdev &&
        a.st_dev==b.st_dev && a.st_ino==b.st_ino && a.st_uid==0 && b.st_uid==0 &&
        major(a.st_rdev)==13 && minor(a.st_rdev)==65;
}
int no(const char *why) {std::printf("{\"schema\":\"rm2-pen-inspect/1\",\"complete\":false,\"reason\":\"%s\",\"event_reads\":0,\"event_writes\":0}\n",why);return 1;}
bool bit(const unsigned char *bits,unsigned n) {return (bits[n/8]&(1u<<(n%8)))!=0;}
}
int main(int argc,char **argv) {
    if(argc!=2 || std::strcmp(argv[1],"--inspect"))return no("arguments");
    struct sigaction action{};action.sa_handler=SIG_DFL;sigemptyset(&action.sa_mask);
    sigset_t unblocked;sigemptyset(&unblocked);sigaddset(&unblocked,SIGALRM);
    if(sigaction(SIGALRM,&action,nullptr) || sigprocmask(SIG_UNBLOCK,&unblocked,nullptr))return no("alarm-setup");
    alarm(3); // Independent intrinsic lifetime bound; never changes stock services.
    Holder h;h.fd=::open(Device,O_RDONLY|O_NONBLOCK|O_CLOEXEC|O_NOFOLLOW);
    if(h.fd<0)return no("open-readonly");
    struct stat before{},path{},after{};
    if(fstat(h.fd,&before) || lstat(Device,&path) || !same(before,path))return no("device-identity");
    input_id id{},idAfter{};char name[128]{},nameAfter[128]{};
    if(ioctl(h.fd,EVIOCGID,&id) || ioctl(h.fd,EVIOCGNAME(sizeof(name)),name)!=int(sizeof(Name)) ||
       std::memcmp(name,Name,sizeof(Name)) || id.bustype!=0x18 || id.vendor!=0x2d1f || id.product!=0x95 || id.version!=0x1231)return no("profile");
    std::array<unsigned char,(EV_MAX+8)/8> ev{};
    std::array<unsigned char,(KEY_MAX+8)/8> key{},pressed{},pressedAfter{};
    std::array<unsigned char,(ABS_MAX+8)/8> abs{};
    if(ioctl(h.fd,EVIOCGBIT(0,ev.size()),ev.data())!=int(ev.size()) ||
       ioctl(h.fd,EVIOCGBIT(EV_KEY,key.size()),key.data())!=int(key.size()) ||
       ioctl(h.fd,EVIOCGBIT(EV_ABS,abs.size()),abs.data())!=int(abs.size()) ||
       ioctl(h.fd,EVIOCGKEY(pressed.size()),pressed.data())!=int(pressed.size()))return no("capabilities");
    for(unsigned n=0;n<=EV_MAX;++n)if(bit(ev.data(),n)!=(n==EV_SYN||n==EV_KEY||n==EV_ABS))return no("event-profile");
    for(unsigned n=0;n<=KEY_MAX;++n)if(bit(key.data(),n)!=(n==BTN_TOOL_PEN||n==BTN_TOOL_RUBBER||n==BTN_TOUCH||n==BTN_STYLUS||n==BTN_STYLUS2))return no("key-profile");
    for(unsigned n=0;n<=ABS_MAX;++n)if(bit(abs.data(),n)!=(n==ABS_X||n==ABS_Y||n==ABS_PRESSURE||n==ABS_DISTANCE||n==ABS_TILT_X||n==ABS_TILT_Y))return no("axis-profile");
    constexpr std::array<int,6> codes{ABS_X,ABS_Y,ABS_PRESSURE,ABS_DISTANCE,ABS_TILT_X,ABS_TILT_Y};
    std::array<input_absinfo,6> axes{},axesAfter{};
    for(std::size_t i=0;i<codes.size();++i)if(ioctl(h.fd,EVIOCGABS(codes[i]),&axes[i]))return no("axis-query");
    if(ioctl(h.fd,EVIOCGKEY(pressedAfter.size()),pressedAfter.data())!=int(pressedAfter.size()) || pressed!=pressedAfter)return no("keys-changed");
    for(std::size_t i=0;i<codes.size();++i)if(ioctl(h.fd,EVIOCGABS(codes[i]),&axesAfter[i]) ||
       std::memcmp(&axes[i],&axesAfter[i],sizeof(input_absinfo)))return no("axis-changed");
    if(fstat(h.fd,&after) || lstat(Device,&path) || !same(before,after) || !same(after,path) ||
       ioctl(h.fd,EVIOCGID,&idAfter) || std::memcmp(&id,&idAfter,sizeof(id)) ||
       ioctl(h.fd,EVIOCGNAME(sizeof(nameAfter)),nameAfter)!=int(sizeof(Name)) || std::memcmp(name,nameAfter,sizeof(Name)))return no("profile-changed");
    bool released=true;for(unsigned n=0;n<=KEY_MAX;++n)released=released&&!bit(pressed.data(),n);
    released=released&&axes[2].value==0;
    const int fd=h.fd;h.fd=-1; // Never retry close, including an interrupted close.
    if(::close(fd))return no("close-readonly");
    std::printf("{\"schema\":\"rm2-pen-inspect/1\",\"complete\":true,\"name\":\"Wacom I2C Digitizer\",\"id\":[%u,%u,%u,%u],\"rdev\":[%u,%u],\"inode\":%llu,\"released_snapshot\":%s,\"axes\":[",id.bustype,id.vendor,id.product,id.version,major(before.st_rdev),minor(before.st_rdev),static_cast<unsigned long long>(before.st_ino),released?"true":"false");
    for(std::size_t i=0;i<codes.size();++i) {
        const auto &a=axes[i];std::printf("%s{\"code\":%d,\"value\":%d,\"min\":%d,\"max\":%d,\"fuzz\":%d,\"flat\":%d,\"resolution\":%d}",i?",":"",codes[i],a.value,a.minimum,a.maximum,a.fuzz,a.flat,a.resolution);
    }
    std::printf("],\"event_bytes\":%zu,\"event_reads\":0,\"event_writes\":0,\"queue_observed\":false,\"native_authority\":false}\n",sizeof(input_event));
    return 0;
}
