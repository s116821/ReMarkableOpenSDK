#pragma once
// Private fixed-profile patch. The constructor owns launch admission.
#include "composer_pre_enter_arm.h"
#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <csignal>
#include <fcntl.h>
#include <memory>
#include <asm/unistd.h>
#include <linux/magic.h>
#include <sys/mman.h>
#include <sys/vfs.h>
#include <time.h>

namespace remarkable::composer_pre_enter::detail { class Route; }
namespace remarkable::composer_pre_enter::detail::patch {
// Raw leaves: no libc errno, cancellation wrapper, lazy binding or allocation.
inline long raw(long n,long x=0,long y=0,long z=0,long u=0,long v=0,long w=0) noexcept {
#if defined(__arm__) && !defined(__thumb__)
    register long a0 asm("r0")=x,a1 asm("r1")=y,a2 asm("r2")=z;
    register long a3 asm("r3")=u,a4 asm("r4")=v,a5 asm("r5")=w,number asm("r7")=n;
    asm volatile("svc 0" : "+r"(a0) : "r"(a1),"r"(a2),"r"(a3),"r"(a4),"r"(a5),"r"(number) : "memory","cc");
    return a0;
#elif defined(__x86_64__)
    register long a3 asm("r10")=u,a4 asm("r8")=v,a5 asm("r9")=w;
    long result;
    asm volatile("syscall" : "=a"(result) : "a"(n),"D"(x),"S"(y),"d"(z),"r"(a3),"r"(a4),"r"(a5) : "rcx","r11","memory","cc");
    return result;
#else
#error Unsupported owned/private patch syscall architecture.
#endif
}
inline long address(const void *p) noexcept { return reinterpret_cast<long>(p); }
struct BootTime { std::int64_t seconds=0,nanoseconds=0; };
inline bool bootNow(BootTime &t) noexcept {
#if defined(__arm__)
    const bool okay=raw(__NR_clock_gettime64,CLOCK_BOOTTIME,address(&t))==0;
#else
    const bool okay=raw(__NR_clock_gettime,CLOCK_BOOTTIME,address(&t))==0;
#endif
    return okay&&t.seconds>=0&&t.nanoseconds>=0&&t.nanoseconds<1000000000;
}
inline bool cache(void *p,std::size_t n) noexcept {
#if defined(__arm__)
    return raw(0xf0002,address(p),address(p)+static_cast<long>(n),0)==0;
#else
    (void)p; (void)n; asm volatile("" ::: "memory"); return true;
#endif
}
[[noreturn]] inline void fatal() noexcept {
    raw(__NR_exit_group,119); __builtin_trap();
}
enum class Outcome { Unprepared, Refused, ExtraThread, Installed, RolledBack };
#ifdef COMPOSER_PATCH_OWNED_TESTING
enum class Fault { None, Expired, BeforeWrite, AfterWrite, Cache, Protection, RollbackFatal, MaskFatal, ExclusionFatal };
#endif

class Prepared final {
    friend class StartupBuilder;
#ifdef COMPOSER_PATCH_OWNED_TESTING
    friend struct OwnedAccess;
    Fault fault_=Fault::None;
    int pendingSignal_=0;
#endif
    static constexpr std::size_t MapLimit=65536,StatusLimit=16384;
    std::array<char,MapLimit> baseline_{},current_{};
    std::array<char,StatusLimit> status_{};
    alignas(8) std::array<unsigned char,4096> dents_{};
    std::size_t baselineSize_=0,pageSize_=0;
    std::uintptr_t page_=0,veneer_=0,shim_=0;
    bool ownsVeneer_=false;
    int maps_=-1,statusFd_=-1,tasks_=-1;
    std::uint32_t encoded_=0;
    std::uint64_t oldMask_=0;
    // Absolute CLOCK_BOOTTIME, normalized from the original startup expiry.
    BootTime deadline_{};
    bool ready_=false,used_=false;
    std::atomic<unsigned> committed_{0}; // Local patch fact, never consumer authority.
    Route *route_=nullptr;
    std::atomic<Route *> *routeSlot_=nullptr;
    static_assert(std::atomic<unsigned>::is_always_lock_free);
    Prepared()=default;
    static bool same(const char *a,const char *b,std::size_t n) noexcept {
        // Volatile byte loads avoid compiler substitution with a libc memcmp.
        for(std::size_t i=0;i<n;++i)
            if(static_cast<const volatile char *>(a)[i]!=static_cast<const volatile char *>(b)[i])return false;
        return true;
    }
    static bool readAll(int fd,char *out,std::size_t capacity,std::size_t &size) noexcept {
        if(raw(__NR_lseek,fd,0,0)<0)return false;
        size=0;
        for(unsigned reads=0;reads<32;++reads) {
            char last=0;
            const auto n=raw(__NR_read,fd,address(size<capacity?out+size:&last),
                             static_cast<long>(size<capacity?capacity-size:1));
            if(n<0)return false;
            if(!n)return true;
            if(size==capacity)return false;
            size+=static_cast<std::size_t>(n);
        }
        return false;
    }
    bool mapsMatch() noexcept {
        std::size_t n=0;
        return readAll(maps_,current_.data(),current_.size(),n)&&n==baselineSize_&&
            same(baseline_.data(),current_.data(),n);
    }
    bool executable(std::uintptr_t at,std::size_t length) const noexcept {
        if(at>UINTPTR_MAX-length)return false;
        auto hex=[&](std::size_t &i,std::uintptr_t &value) noexcept {
            value=0; bool any=false;
            for(;i<baselineSize_;++i) {
                const auto c=baseline_[i]; unsigned digit=0;
                if(c>='0'&&c<='9')digit=unsigned(c-'0');
                else if(c>='a'&&c<='f')digit=unsigned(c-'a'+10);
                else break;
                if(value>(UINTPTR_MAX-digit)/16)return false;
                value=value*16+digit; any=true;
            }
            return any;
        };
        for(std::size_t i=0;i<baselineSize_;) {
            std::uintptr_t start=0,end=0;
            if(!hex(i,start)||i>=baselineSize_||baseline_[i++]!='-'||!hex(i,end))return false;
            if(i+5>=baselineSize_||baseline_[i++]!=' ')return false;
            if(start<=at&&at+length<=end)
                return baseline_[i]=='r'&&baseline_[i+1]=='-'&&baseline_[i+2]=='x'&&baseline_[i+3]=='p';
            while(i<baselineSize_&&baseline_[i++]!='\n'){}
        }
        return false;
    }
    long threadCount() noexcept {
        std::size_t n=0;
        if(!readAll(statusFd_,status_.data(),status_.size(),n))return -1;
        long found=-1;
        constexpr char label[]="Threads:";
        for(std::size_t i=0;i+sizeof(label)-1<n;++i) {
            if(i&&status_[i-1]!='\n')continue;
            if(!same(status_.data()+i,label,sizeof(label)-1))continue;
            if(found!=-1)return -1;
            auto k=i+sizeof(label)-1;
            while(k<n&&(status_[k]==' '||status_[k]=='\t'))++k;
            long value=0; bool digit=false;
            while(k<n&&status_[k]>='0'&&status_[k]<='9') {
                digit=true; if(value>100000)return -1;
                value=value*10+status_[k++]-'0';
            }
            if(!digit||k==n||status_[k]!='\n')return -1;
            found=value;
        }
        return found;
    }
    bool onlyTask(long tid) noexcept {
        if(raw(__NR_lseek,tasks_,0,0)<0)return false;
        unsigned count=0;
        for(unsigned reads=0;reads<16;++reads) {
            const auto n=raw(__NR_getdents64,tasks_,address(dents_.data()),dents_.size());
            if(n<0)return false;
            if(!n)return count==1;
            for(std::size_t at=0;at<static_cast<std::size_t>(n);) {
                if(static_cast<std::size_t>(n)-at<20)return false;
                const auto len=unsigned(dents_[at+16])|(unsigned(dents_[at+17])<<8);
                if(len<20||len>static_cast<std::size_t>(n)-at)return false;
                std::size_t k=at+19,end=at+len;
                if(dents_[k]=='.') {
                    ++k; if(k<end&&dents_[k]=='.')++k;
                    if(k>=end||dents_[k]!=0)return false;
                } else {
                    long id=0; bool digit=false;
                    for(;k<end&&dents_[k];++k) {
                        if(dents_[k]<'0'||dents_[k]>'9'||id>214748364)return false;
                        digit=true; id=id*10+dents_[k]-'0';
                    }
                    if(!digit||k==end||id!=tid||++count!=1)return false;
                }
                at=end;
            }
        }
        return false;
    }
    bool inTime() noexcept {
        BootTime t{}; return bootNow(t)&&(t.seconds<deadline_.seconds||
            (t.seconds==deadline_.seconds&&t.nanoseconds<deadline_.nanoseconds));
    }
    bool protect(int flags) noexcept {return raw(__NR_mprotect,page_,pageSize_,flags)==0;}
    static std::uint32_t siteWord() noexcept {return *reinterpret_cast<volatile std::uint32_t *>(Site);}
    bool restoreMask() noexcept {return raw(__NR_rt_sigprocmask,SIG_SETMASK,address(&oldMask_),0,8)==0;}
    bool rollback() noexcept {
        if(siteWord()!=encoded_)return false; // Never overwrite unexpected instructions.
        if(!protect(PROT_READ|PROT_WRITE))return false;
        *reinterpret_cast<volatile std::uint32_t *>(Site)=OriginalWord;
        return cache(reinterpret_cast<void *>(Site),4)&&protect(PROT_READ|PROT_EXEC)&&
            siteWord()==OriginalWord&&mapsMatch();
    }
    Outcome run() noexcept {
        if(!ready_||used_)return Outcome::Unprepared;
        used_=true;
        if(!inTime())return Outcome::Refused;
        const std::uint64_t blocked=~std::uint64_t(0)&~(std::uint64_t(1)<<(SIGKILL-1))&
            ~(std::uint64_t(1)<<(SIGSTOP-1));
        if(raw(__NR_rt_sigprocmask,SIG_SETMASK,address(&blocked),address(&oldMask_),8)!=0)
            return Outcome::Refused;
        std::uint64_t effective=0;
        auto refuse=[&](Outcome why) noexcept {if(!restoreMask())fatal();return why;};
        if(raw(__NR_rt_sigprocmask,SIG_SETMASK,0,address(&effective),8)!=0||effective!=blocked)
            return refuse(Outcome::Refused);
        const auto tid=raw(__NR_gettid);
        if(tid<=0||tid!=raw(__NR_getpid))return refuse(Outcome::Refused);
        const auto threads=threadCount();
        if(threads<1)return refuse(Outcome::Refused);
        if(threads!=1)return refuse(Outcome::ExtraThread);
        if(!onlyTask(tid)||threadCount()!=1||!mapsMatch())
            return refuse(Outcome::Refused);
        std::uint32_t expected=0;
        if(!pageSize_||(pageSize_&(pageSize_-1))||page_!=(Site&~(pageSize_-1))||
           !branch(Site,veneer_,expected)||expected!=encoded_||(shim_&3)||shim_>UINT32_MAX||
           !executable(Site,4)||!executable(Original,4)||!executable(veneer_,8)||!executable(shim_,4)||
           (reinterpret_cast<std::uintptr_t>(&cache)&~(pageSize_-1))==page_)
            return refuse(Outcome::Refused);
        if(*reinterpret_cast<volatile std::uint32_t *>(veneer_)!=VeneerLoad||
           *reinterpret_cast<volatile std::uint32_t *>(veneer_+4)!=shim_||siteWord()!=OriginalWord||
           (routeSlot_&&routeSlot_->load(std::memory_order_acquire)))
            return refuse(Outcome::Refused);
#ifdef COMPOSER_PATCH_OWNED_TESTING
        if(pendingSignal_&&raw(__NR_tgkill,tid,tid,pendingSignal_)!=0)fatal();
        if(fault_==Fault::BeforeWrite)return refuse(Outcome::Refused);
#endif
        if(!inTime()||!protect(PROT_READ|PROT_WRITE))return refuse(Outcome::Refused);
        *reinterpret_cast<volatile std::uint32_t *>(Site)=encoded_;
        bool flushed=false,readonly=false;
#ifdef COMPOSER_PATCH_OWNED_TESTING
        if(fault_!=Fault::AfterWrite&&fault_!=Fault::Cache&&fault_!=Fault::RollbackFatal)
#endif
            flushed=cache(reinterpret_cast<void *>(Site),4);
#ifdef COMPOSER_PATCH_OWNED_TESTING
        if(fault_!=Fault::Protection)
#endif
            if(flushed)readonly=protect(PROT_READ|PROT_EXEC);
        bool okay=flushed&&readonly;
        std::uint64_t finalMask=0;
        if(threadCount()!=1||raw(__NR_rt_sigprocmask,SIG_SETMASK,0,address(&finalMask),8)!=0||
           finalMask!=blocked)fatal(); // Lost exclusion never authorizes rollback.
#ifdef COMPOSER_PATCH_OWNED_TESTING
        if(fault_==Fault::ExclusionFatal)fatal();
#endif
        if(siteWord()!=encoded_)fatal();
        if(okay&&!mapsMatch())fatal();
        okay=okay&&inTime();
        if(!okay) {
#ifdef COMPOSER_PATCH_OWNED_TESTING
            if(fault_==Fault::RollbackFatal)fatal();
#endif
            if(!rollback())fatal();
            return refuse(Outcome::RolledBack);
        }
#ifdef COMPOSER_PATCH_OWNED_TESTING
        if(fault_==Fault::MaskFatal)fatal();
#endif
        // The caller's prepared route must already be resident/coherent. Pending
        // handlers may run as soon as this exact mask restoration takes effect.
        if(routeSlot_)routeSlot_->store(route_,std::memory_order_release);
        committed_.store(1,std::memory_order_release);
        if(!restoreMask())fatal();
        return Outcome::Installed;
    }
public:
    Prepared(const Prepared &)=delete;
    Prepared &operator=(const Prepared &)=delete;
    ~Prepared() { // Outside exclusion; never restores code or destroys a route.
        if(ownsVeneer_&&!committed_.load(std::memory_order_acquire))
            ::munmap(reinterpret_cast<void *>(veneer_),pageSize_);
        if(maps_>=0)raw(__NR_close,maps_);
        if(statusFd_>=0)raw(__NR_close,statusFd_);
        if(tasks_>=0)raw(__NR_close,tasks_);
    }
};

class ConstructorStartup;
class StartupBuilder final {
    friend class ConstructorStartup;
#ifdef COMPOSER_PATCH_OWNED_TESTING
    friend struct OwnedAccess;
#endif
    static bool procFile(int fd) noexcept {
        struct statfs fs{};
        return fd>=0&&::fstatfs(fd,&fs)==0&&fs.f_type==PROC_SUPER_MAGIC;
    }
    static std::unique_ptr<Prepared> prepare(std::size_t pageSize,std::uintptr_t shim,
                                            std::uint64_t expiryBootMs) {
        if(pageSize<8||(pageSize&(pageSize-1))||pageSize>65536||
           !shim||(shim&3)||shim>UINT32_MAX||!expiryBootMs)return {};
        auto c=std::unique_ptr<Prepared>(new Prepared);
        c->pageSize_=pageSize;
        c->page_=Site&~(pageSize-1);
        c->shim_=shim;
        c->deadline_={static_cast<std::int64_t>(expiryBootMs/1000),
                      static_cast<std::int64_t>(expiryBootMs%1000)*1000000};
        if(!c->inTime())return {};
        c->maps_=::open("/proc/self/maps",O_RDONLY|O_CLOEXEC);
        c->statusFd_=::open("/proc/self/status",O_RDONLY|O_CLOEXEC);
        c->tasks_=::open("/proc/self/task",O_RDONLY|O_DIRECTORY|O_CLOEXEC);
        if(!procFile(c->maps_)||!procFile(c->statusFd_)||!procFile(c->tasks_))return {};
        if(!c->readAll(c->maps_,c->baseline_.data(),c->baseline_.size(),c->baselineSize_)||
           !c->executable(c->page_,pageSize)||!c->executable(Original,4)||
           !c->executable(shim,4)||siteWordChanged())return {};

        // Hints never replace a live mapping. Keep only a returned, in-range page.
        for(unsigned i=1;i<=31&&!c->ownsVeneer_;++i) {
            const auto hint=c->page_+i*std::uintptr_t(1U<<20);
            void *mapped=::mmap(reinterpret_cast<void *>(hint),pageSize,PROT_READ|PROT_WRITE,
                                MAP_PRIVATE|MAP_ANONYMOUS,-1,0);
            if(mapped==MAP_FAILED)continue;
            const auto actual=reinterpret_cast<std::uintptr_t>(mapped);
            if(!actual||!branch(Site,actual,c->encoded_)) {
                ::munmap(mapped,pageSize);
                continue;
            }
            c->veneer_=actual;
            c->ownsVeneer_=true;
        }
        if(!c->ownsVeneer_)return {};
        auto *words=reinterpret_cast<std::uint32_t *>(c->veneer_);
        words[0]=VeneerLoad;
        words[1]=static_cast<std::uint32_t>(shim);
        if(!cache(words,8)||::mprotect(words,pageSize,PROT_READ|PROT_EXEC)||
           !c->readAll(c->maps_,c->baseline_.data(),c->baseline_.size(),c->baselineSize_)||
           !c->executable(c->veneer_,pageSize)||!c->inTime())return {};
        c->ready_=true;
        return c;
    }
    static bool siteWordChanged() noexcept {return Prepared::siteWord()!=OriginalWord;}
    static Outcome install(Prepared &capsule,Route &route,std::atomic<Route *> &slot) noexcept {
        capsule.route_=&route;
        capsule.routeSlot_=&slot;
        return capsule.run();
    }
};
} // namespace remarkable::composer_pre_enter::detail::patch
