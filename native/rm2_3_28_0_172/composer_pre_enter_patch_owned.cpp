// Owned Linux children only. No target process or production admission builder.
#include "composer_pre_enter_patch_private.h"
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <future>
#include <thread>
#include <atomic>
#include <new>
#include <sys/wait.h>
#include <unistd.h>
namespace p=remarkable::composer_pre_enter::detail::patch;
namespace d=remarkable::composer_pre_enter::detail;
static std::atomic<unsigned> allocations{0};
__attribute__((noinline)) void *operator new(std::size_t n) {
    if(auto *m=std::malloc(n?n:1)) {allocations.fetch_add(1);return m;}
    throw std::bad_alloc();
}
__attribute__((noinline)) void operator delete(void *m) noexcept {std::free(m);}
__attribute__((noinline)) void operator delete(void *m,std::size_t) noexcept {std::free(m);}
static volatile sig_atomic_t signalsSeen=0;
static volatile sig_atomic_t signalWord=0,signalCommitted=0;
static std::atomic<std::atomic<unsigned> *> handlerCommit{nullptr};
static_assert(decltype(handlerCommit)::is_always_lock_free);
static void handler(int) {
    signalWord=*reinterpret_cast<volatile sig_atomic_t *>(d::Site);
    if(auto *flag=handlerCommit.load(std::memory_order_relaxed))
        signalCommitted=static_cast<sig_atomic_t>(flag->load(std::memory_order_acquire));
    ++signalsSeen;
}
namespace remarkable::composer_pre_enter::detail::patch {
struct OwnedAccess {
    static std::unique_ptr<Prepared> prepare(Fault fault,bool signal) {
        auto c=std::unique_ptr<Prepared>(new Prepared);
        c->pageSize_=4096; c->page_=Site&~std::uintptr_t(4095);
        c->veneer_=c->page_+0x100; c->shim_=c->page_+0x200;
        c->maps_=static_cast<int>(raw(__NR_openat,AT_FDCWD,address("/proc/self/maps"),O_RDONLY|O_CLOEXEC));
        c->statusFd_=static_cast<int>(raw(__NR_openat,AT_FDCWD,address("/proc/self/status"),O_RDONLY|O_CLOEXEC));
        c->tasks_=static_cast<int>(raw(__NR_openat,AT_FDCWD,address("/proc/self/task"),O_RDONLY|O_DIRECTORY|O_CLOEXEC));
        assert(c->maps_>=0&&c->statusFd_>=0&&c->tasks_>=0);
        assert(branch(Site,c->veneer_,c->encoded_));
        assert(now(c->deadline_)); c->deadline_.seconds+=5;
        assert(c->readAll(c->maps_,c->baseline_.data(),c->baseline_.size(),c->baselineSize_));
        // This constructor is compiled only for the owned ELF/map fixture. There
        // is intentionally no production builder that treats these observations
        // as proof of target-kernel or initial-load constructor provenance.
        c->fault_=fault; c->pendingSignal_=signal?SIGUSR1:0; c->ready_=true;
        return c;
    }
    static Outcome run(Prepared &c) {return c.run();}
    static long threads(Prepared &c) {return c.threadCount();}
    static std::uint32_t expected(const Prepared &c) {return c.encoded_;}
    static std::atomic<unsigned> *commit(Prepared &c) {return &c.committed_;}
};
}
static void mapping(std::uintptr_t at) {
    void *m=::mmap(reinterpret_cast<void *>(at),4096,PROT_READ|PROT_WRITE,
                   MAP_PRIVATE|MAP_ANONYMOUS,-1,0);
    if(m!=reinterpret_cast<void *>(at))std::_Exit(77); // Never overwrite another map.
}
static int child(p::Fault fault,bool signal,bool extra,bool mismatch=false) {
    const auto page=d::Site&~std::uintptr_t(4095);
    mapping(page);
    const auto originalPage=d::Original&~std::uintptr_t(4095);
    mapping(originalPage);
    assert(::mprotect(reinterpret_cast<void *>(originalPage),4096,PROT_READ|PROT_EXEC)==0);
    *reinterpret_cast<std::uint32_t *>(d::Site)=d::OriginalWord;
    auto *v=reinterpret_cast<std::uint32_t *>(page+0x100);
    v[0]=d::VeneerLoad;v[1]=static_cast<std::uint32_t>(page+0x200);
    if(!p::cache(reinterpret_cast<void *>(page),4096)) {
        std::fprintf(stderr,"owned preparation: raw ARM cache leaf does not satisfy the native zero-return contract; refusing before any patch interval\n");
        return 78;
    }
    assert(::mprotect(reinterpret_cast<void *>(page),4096,PROT_READ|PROT_EXEC)==0);
    struct sigaction sa{}; sa.sa_handler=handler;
    assert(sigemptyset(&sa.sa_mask)==0&&sigaction(SIGUSR1,&sa,nullptr)==0);
    std::promise<void> acquired,release;
    auto ready=acquired.get_future();auto leave=release.get_future();
    std::thread peer;
    if(extra) {
        peer=std::thread([&]{acquired.set_value();assert(leave.wait_for(std::chrono::seconds(5))==std::future_status::ready);});
        assert(ready.wait_for(std::chrono::seconds(5))==std::future_status::ready);
    }
    auto capsule=p::OwnedAccess::prepare(fault,signal);
    struct ClearHandlerPointer {
        ~ClearHandlerPointer() {handlerCommit.store(nullptr,std::memory_order_release);}
    } clearHandlerPointer;
    handlerCommit.store(p::OwnedAccess::commit(*capsule),std::memory_order_release);
    const auto initialThreads=p::OwnedAccess::threads(*capsule);
    if(mismatch) {
        assert(::mprotect(reinterpret_cast<void *>(page),4096,PROT_READ|PROT_WRITE)==0);
        *reinterpret_cast<std::uint32_t *>(d::Site)=0;
        assert(::mprotect(reinterpret_cast<void *>(page),4096,PROT_READ|PROT_EXEC)==0);
    }
    std::uint64_t before=0,after=0;
    assert(p::raw(__NR_rt_sigprocmask,SIG_SETMASK,0,p::address(&before),8)==0);
    const auto allocationCount=allocations.load();
    const auto outcome=p::OwnedAccess::run(*capsule);
    assert(allocations.load()==allocationCount);
    assert(p::raw(__NR_rt_sigprocmask,SIG_SETMASK,0,p::address(&after),8)==0&&before==after);
    if(extra) {release.set_value();peer.join();}
    const auto word=*reinterpret_cast<volatile std::uint32_t *>(d::Site);
    if(initialThreads!=1) {
        assert(outcome==p::Outcome::ExtraThread&&word==(mismatch?0:d::OriginalWord));
        return extra?0:78; // Qemu host helper threads cannot be waved away.
    }
    if(mismatch) {assert(outcome==p::Outcome::Refused&&word==0);return 0;}
    if(fault==p::Fault::None) assert(outcome==p::Outcome::Installed&&word==p::OwnedAccess::expected(*capsule));
    else if(fault==p::Fault::BeforeWrite) assert(outcome==p::Outcome::Refused&&word==d::OriginalWord);
    else assert(outcome==p::Outcome::RolledBack&&word==d::OriginalWord);
    if(signal) assert(signalsSeen==1&&static_cast<std::uint32_t>(signalWord)==word&&
        signalCommitted==(outcome==p::Outcome::Installed?1:0));
    assert(p::OwnedAccess::run(*capsule)==p::Outcome::Unprepared);
    return 0;
}
static int run(p::Fault f,bool signal=false,bool extra=false,bool mismatch=false,int expected=0) {
    const auto pid=::fork();assert(pid>=0);
    if(!pid)std::_Exit(child(f,signal,extra,mismatch));
    int status=0;assert(waitpid(pid,&status,0)==pid&&WIFEXITED(status));
    const int result=WEXITSTATUS(status);
    if(result==78)return 78;
    assert(result==expected);return 0;
}
int main() {
    if(run(p::Fault::None,true)==78) {
        std::puts("owned capsule: raw-cache ABI or real task observation refuses this environment; no patch was installed");
        return 78;
    }
    assert(run(p::Fault::None,false,true)==0);
    assert(run(p::Fault::None,false,false,true)==0);
    for(auto f:{p::Fault::BeforeWrite,p::Fault::AfterWrite,p::Fault::Cache,p::Fault::Protection})
        assert(run(f,true)==0);
    assert(run(p::Fault::RollbackFatal,false,false,false,119)==0);
    assert(run(p::Fault::MaskFatal,false,false,false,119)==0);
    assert(run(p::Fault::ExclusionFatal,false,false,false,119)==0);
    std::puts("owned Linux capsule: pending signal/coherent release, extra-thread refusal, original-word refusal, cache/protection rollback and fail-closed children passed");
}
