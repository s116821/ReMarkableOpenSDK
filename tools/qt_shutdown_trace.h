#pragma once
// Development diagnostic only. No page facts, rendering requests or shutdown action.
#include <QByteArray>
#include <QList>
#include <atomic>
#include <cstdio>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <time.h>
#include <unistd.h>

namespace shutdown_trace {
enum class Event { Startup, Window, BeforeRender, AfterRender, AboutToQuit,
                   WindowDestroyed, ApplicationDestroyed, LateBeforeRender, LateAfterRender,
                   EntryInstalled };
inline const char *name(Event e) {
    switch(e) {
    case Event::Startup:return "startup"; case Event::Window:return "window";
    case Event::BeforeRender:return "before-render"; case Event::AfterRender:return "after-render";
    case Event::AboutToQuit:return "about-to-quit";
    case Event::WindowDestroyed:return "window-destroyed";
    case Event::ApplicationDestroyed:return "application-destroyed";
    case Event::LateBeforeRender:return "late-before-render";
    case Event::LateAfterRender:return "late-after-render";
    case Event::EntryInstalled:return "entry-installed";
    }
    return "invalid";
}
inline QByteArray readOwned(int dir,const char *path,int cap) {
    const int fd=::openat(dir,path,O_RDONLY|O_NOFOLLOW|O_CLOEXEC);
    if(fd<0)return {};
    struct stat s{}; QByteArray out(cap,0);
    const bool valid=::fstat(fd,&s)==0 && S_ISREG(s.st_mode) && s.st_uid==0 &&
        (s.st_mode&0777)==0600 && s.st_nlink==1 && s.st_size>0 && s.st_size<cap;
    const ssize_t n=valid ? ::read(fd,out.data(),size_t(cap)):-1;
    ::close(fd);
    if(n<=0 || n!=s.st_size)return {};
    out.resize(int(n));return out;
}
inline QByteArray processStart() {
    int fd=::open("/proc/self/stat",O_RDONLY|O_CLOEXEC);if(fd<0)return {};
    char b[2048];const ssize_t n=::read(fd,b,sizeof b);::close(fd);
    if(n<=0 || n>=ssize_t(sizeof b))return {};
    const QByteArray text(b,int(n));const int end=text.lastIndexOf(')');if(end<0)return {};
    const auto v=text.mid(end+2).simplified().split(' ').value(19);
    if(v.isEmpty())return {};
    for(char c:v)if(c<'0'||c>'9')return {};
    return v;
}
// Intentionally process-resident: callbacks never access a destructed recorder.
// Do not use this object as a completion fence or production lifetime owner.
struct Recorder {
    int fd=-1; unsigned pid=0; unsigned long long start=0;
    std::atomic_flag writing=ATOMIC_FLAG_INIT;
    std::atomic<unsigned> dropped{0}, frames{0};
    std::atomic<bool> failed{false},quit{false},lateBefore{false},lateAfter{false};
    unsigned sequence=0;
    static Recorder *open(const char *directory,const char *nonce) {
        if(::geteuid()!=0)return nullptr;
        const int dir=::open(directory,O_RDONLY|O_DIRECTORY|O_NOFOLLOW|O_CLOEXEC);
        if(dir<0)return nullptr;
        struct stat s{};const auto startText=processStart();bool ok=false;
        const auto startValue=startText.toULongLong(&ok);
        const bool valid=::fstat(dir,&s)==0 && S_ISDIR(s.st_mode) && s.st_uid==0 &&
            (s.st_mode&0777)==0700 && ok && startValue>0 &&
            readOwned(dir,"owner",64)==QByteArray(nonce) &&
            readOwned(dir,"attempt.identity",128)==QByteArray::number(::getpid())+' '+startText+'\n';
        const int out=valid ? ::openat(dir,"shutdown-trace.log",O_WRONLY|O_APPEND|O_CREAT|O_EXCL|O_NOFOLLOW|O_CLOEXEC,0600):-1;
        ::close(dir);if(out<0)return nullptr;
        auto *r=new Recorder;r->fd=out;r->pid=unsigned(::getpid());r->start=startValue;return r;
    }
    void record(Event e) {
        if(failed.load(std::memory_order_relaxed))return;
        if(writing.test_and_set(std::memory_order_acquire)){dropped.fetch_add(1);return;}
        // 32 normal frame pairs + one pair after quit + bounded lifecycle events.
        if(sequence>=96){failed=true;writing.clear(std::memory_order_release);return;}
        timespec ts{};char bytes[192];
        if(::clock_gettime(CLOCK_MONOTONIC,&ts)!=0){failed=true;writing.clear(std::memory_order_release);return;}
        const auto ns=static_cast<unsigned long long>(ts.tv_sec)*1000000000ULL+static_cast<unsigned long long>(ts.tv_nsec);
        const int length=std::snprintf(bytes,sizeof bytes,"v1 %u %s %llu %ld %u %llu %u %u %u\n",
            ++sequence,name(e),ns,::syscall(SYS_gettid),pid,start,unsigned(quit.load()),dropped.load(),frames.load());
        if(length<=0 || size_t(length)>=sizeof bytes || ::write(fd,bytes,size_t(length))!=length)failed=true;
        writing.clear(std::memory_order_release);
    }
    void beforeRender() {
        const unsigned frame=frames.fetch_add(1)+1;
        if(frame<=32)record(Event::BeforeRender);
        if(quit.load() && !lateBefore.exchange(true))record(Event::LateBeforeRender);
    }
    void afterRender() {
        const unsigned frame=frames.load();
        if(frame>0 && frame<=32)record(Event::AfterRender);
        if(quit.load() && !lateAfter.exchange(true))record(Event::LateAfterRender);
    }
    void aboutToQuit() {quit=true;record(Event::AboutToQuit);}
};
}
