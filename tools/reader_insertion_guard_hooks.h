#pragma once
// Original research-only negative hook core. No native resolver or ownership grant.
#include "reader_insertion_guard_predicate.h"
#include "reader_insertion_mutex_probe.h"
#include <QSizeF>
#include <QUuid>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <thread>

namespace reader_insertion_hooks {
using Word = std::uint32_t;
using Outer = void (*)(void *, int, const void *, Word, QSizeF, int, Word, const QUuid &);
using Inner = void (*)(void *, int, const QString &, const QString &, std::int32_t, std::int32_t);
struct Mapping { std::uintptr_t begin=0, end=0; bool readable=false, writable=false, executable=false; };
struct InstallIO {
    void *context=nullptr;
    int (*hook)(void *, void *, void *, void **)=nullptr;
    bool (*read)(void *, std::uintptr_t, unsigned char *, std::size_t)=nullptr;
    bool (*mapping)(void *, std::uintptr_t, Mapping &)=nullptr;
    void (*failStop)(void *, const char *)=nullptr; // Must terminate. Returning still terminates.
};
struct Install {
    void *outer=nullptr, *inner=nullptr;
    std::array<unsigned char,8> outerBytes{}, innerBytes{};
};
enum class CallerPolicy { ExactOwned, BorrowedExplicitSize };
struct Binding {
    void *worker=nullptr, *map=nullptr;
    std::thread::id thread;
    std::uintptr_t outerReturnPC=0, innerReturnPC=0;
    int index=-1, sourceIndex=-1;
    const void *optional=nullptr;
    Word sizePresent=0, inherit=0;
    QSizeF size;
    CallerPolicy policy=CallerPolicy::ExactOwned;
};
enum class Refusal { None, Binding, Duplicate, Canceled, Deadline, Attribution, Predicate, MissingInner, UnrelatedException };
struct Outcome {
    bool terminal=false, privateCaught=false, observedMismatch=false, unknown=false, interference=false;
    bool canceled=false, witness=false;
    bool attributed=false;
    std::uint64_t jobId=0;
    unsigned outerCalls=0, innerCalls=0, trackedForwards=0;
    Refusal refusal=Refusal::None;
    reader_insertion_predicate::Result predicate;
    reader_insertion_mutex_probe::Result postReject;
};
struct Registration {
    QUuid target;
    Binding binding;
    std::unique_ptr<const reader_insertion_predicate::Frozen> frozen;
    reader_insertion_predicate::Calls calls;
    // Explicit assertions supplied by qualified caller; this core does not prove them.
    reader_insertion_predicate::CallerAdmission admission;
    std::chrono::steady_clock::time_point deadline;
    void *residentNativeOwners=nullptr; // Intentionally retained, never deleted by core.
    void *attributionContext=nullptr;
    bool (*attribute)(void *,std::uint64_t &) noexcept=nullptr;
    reader_insertion_mutex_probe::Probe *postRejectProbe=nullptr;
};
class Core final {
    struct Record {
        Registration input;
        std::atomic<bool> claimed{false}, canceled{false}, terminal{false}, interference{false};
        Outcome outcome;
        explicit Record(Registration &&r):input(std::move(r)) {}
    };
    struct Scope { Core *core; Record *record; Scope *previous; bool inner=false; };
    struct Rejected { Record *record; };
    enum class Phase { Empty, Installing, Verified, Failed };
    InstallIO io_;
    std::atomic<Phase> phase_{Phase::Empty};
    std::atomic<Record *> record_{nullptr};
    std::atomic<bool> registered_{false};
    Outer originalOuter_=nullptr;
    Inner originalInner_=nullptr;
    static inline std::atomic<Core *> active_{nullptr};
    static inline thread_local Scope *scope_=nullptr;
    explicit Core(InstallIO io):io_(io) {}
    ~Core()=default; // Deliberately inaccessible: process-resident allocation only.
    [[noreturn]] void stop(const char *why) noexcept {
        phase_.store(Phase::Failed,std::memory_order_release);
        try { if(io_.failStop) io_.failStop(io_.context,why); } catch(...) {}
        std::_Exit(119);
    }
    static std::uintptr_t address(const void *p) { return reinterpret_cast<std::uintptr_t>(p); }
    bool rx(std::uintptr_t at,std::size_t count) const {
        if(!at || count==0 || at>UINTPTR_MAX-count || !io_.mapping) return false;
        Mapping m;
        return io_.mapping(io_.context,at,m) && m.begin<=at && m.end>=at+count &&
            m.readable && m.executable && !m.writable;
    }
    bool bytes(std::uintptr_t at,const unsigned char *expected,std::size_t count) const {
        std::array<unsigned char,16> actual{};
        return count<=actual.size() && rx(at,count) && io_.read &&
            io_.read(io_.context,at,actual.data(),count) && std::memcmp(actual.data(),expected,count)==0;
    }
    static std::array<unsigned char,8> jump(std::uintptr_t destination) {
        std::array<unsigned char,8> b{{0x04,0xf0,0x1f,0xe5,0,0,0,0}};
        const Word word=static_cast<Word>(destination); std::memcpy(b.data()+4,&word,4); return b;
    }
    void verifyOne(void *target,void *replacement,const std::array<unsigned char,8> &expected,void *original) {
        const auto entry=jump(address(replacement));
        if(!bytes(address(target),entry.data(),entry.size())) stop("entry-verification");
        std::array<unsigned char,16> trampoline{};
        std::memcpy(trampoline.data(),expected.data(),8);
        const auto back=jump(address(target)+8); std::memcpy(trampoline.data()+8,back.data(),8);
        if(address(original)%4 || !bytes(address(original),trampoline.data(),16)) stop("trampoline-verification");
    }
    void installOne(void *target,void *replacement,const std::array<unsigned char,8> &expected,void **original) {
        if(!io_.hook || io_.hook(io_.context,target,replacement,original)!=0 || !*original) stop("hook-result");
        verifyOne(target,replacement,expected,*original);
    }
    static bool sameSize(const QSizeF &a,const QSizeF &b) {
        return std::memcmp(&a,&b,sizeof(QSizeF))==0;
    }
    static bool validPolicy(const Binding &b) {
        switch(b.policy) {
        case CallerPolicy::ExactOwned: return true;
        case CallerPolicy::BorrowedExplicitSize:
            return !b.optional && b.sizePresent==1 && b.inherit==0 &&
                std::isfinite(b.size.width()) && std::isfinite(b.size.height()) &&
                b.size.width()>0 && b.size.height()>0 && b.size.width()<=16384 && b.size.height()<=16384;
        }
        return false;
    }
    static void finish(Record *r) { r->outcome.terminal=true; r->terminal.store(true,std::memory_order_release); }
    static Core *ready() {
        Core *c=active_.load(std::memory_order_acquire);
        if(!c) std::_Exit(119);
        if(c->phase_.load(std::memory_order_acquire)!=Phase::Verified) c->stop("early-entry");
        return c;
    }
    void outer(std::uintptr_t pc,void *worker,int index,const void *optional,Word present,QSizeF size,
               int source,Word inherit,const QUuid &target) {
        // Materialize this worker-local TLS before the original can hold its map lock.
        Scope *previous=scope_;
        Record *r=record_.load(std::memory_order_acquire);
        if(!r || target!=r->input.target) {
            if(previous) previous->record->interference.store(true,std::memory_order_release);
            struct Suspend { Scope *old; Suspend():old(scope_){scope_=nullptr;} ~Suspend(){scope_=old;} } suspend;
            originalOuter_(worker,index,optional,present,size,source,inherit,target); return;
        }
        if(r->claimed.exchange(true,std::memory_order_acq_rel)) {
            r->interference.store(true,std::memory_order_release); return;
        }
        r->outcome.outerCalls=1;
        const Binding &b=r->input.binding;
        const bool borrowed=b.policy==CallerPolicy::BorrowedExplicitSize;
        const bool optionalMatches=borrowed ? optional && address(optional)%4==0 : optional==b.optional;
        if(previous || worker!=b.worker || index!=b.index || !optionalMatches || present!=b.sizePresent ||
           !sameSize(size,b.size) || (!borrowed && source!=b.sourceIndex) || inherit!=b.inherit || pc!=b.outerReturnPC ||
           std::this_thread::get_id()!=b.thread) {
            r->outcome.refusal=Refusal::Binding; finish(r); return;
        }
        if(r->canceled.load(std::memory_order_acquire)) { r->outcome.refusal=Refusal::Canceled; finish(r); return; }
        if(std::chrono::steady_clock::now()>=r->input.deadline) { r->outcome.refusal=Refusal::Deadline; finish(r); return; }
        if(r->input.attribute) {
            std::uint64_t job=0;
            if(!r->input.attribute(r->input.attributionContext,job) || job==0) {
                r->outcome.refusal=Refusal::Attribution; finish(r); return;
            }
            r->outcome.jobId=job; r->outcome.attributed=true;
            if(r->canceled.load(std::memory_order_acquire)) { r->outcome.refusal=Refusal::Canceled; finish(r); return; }
            if(std::chrono::steady_clock::now()>=r->input.deadline) { r->outcome.refusal=Refusal::Deadline; finish(r); return; }
        }
        Scope current{this,r,previous,false}; scope_=&current;
        struct Restore { Scope *old; ~Restore(){scope_=old;} } restore{previous};
        try {
            originalOuter_(worker,index,optional,present,size,source,inherit,target);
            r->outcome.unknown=true; r->outcome.refusal=Refusal::MissingInner;
        } catch(const Rejected &rejected) {
            if(rejected.record!=r) stop("foreign-private-rejection");
            r->outcome.privateCaught=true;
            if(r->input.postRejectProbe && r->outcome.observedMismatch && r->outcome.refusal==Refusal::None &&
               !r->outcome.unknown && !r->interference.load(std::memory_order_acquire)) {
                if(r->canceled.load(std::memory_order_acquire)) r->outcome.refusal=Refusal::Canceled;
                else if(std::chrono::steady_clock::now()>=r->input.deadline) r->outcome.refusal=Refusal::Deadline;
                else {
                    r->outcome.postReject=r->input.postRejectProbe->once();
                    if(r->canceled.load(std::memory_order_acquire)) r->outcome.refusal=Refusal::Canceled;
                    else if(std::chrono::steady_clock::now()>=r->input.deadline) r->outcome.refusal=Refusal::Deadline;
                }
            }
        } catch(...) {
            r->outcome.unknown=true; r->outcome.refusal=Refusal::UnrelatedException; finish(r); throw;
        }
        finish(r);
    }
    void inner(std::uintptr_t pc,void *map,int index,const QString &target,const QString &templ,
               std::int32_t fifth,std::int32_t sixth) {
        Scope *s=scope_;
        if(!s) { originalInner_(map,index,target,templ,fifth,sixth); return; }
        Record *r=s->record;
        if(s->core!=this || s->inner || pc!=r->input.binding.innerReturnPC || map!=r->input.binding.map ||
           std::this_thread::get_id()!=r->input.binding.thread) stop("unqualified-tracked-inner");
        s->inner=true; r->outcome.innerCalls=1;
        if(r->canceled.load(std::memory_order_acquire)) r->outcome.refusal=Refusal::Canceled;
        else if(std::chrono::steady_clock::now()>=r->input.deadline) r->outcome.refusal=Refusal::Deadline;
        else {
            reader_insertion_predicate::Frame frame;
            const auto w=address(r->input.binding.worker), m=address(map);
            if(w>UINT32_MAX || m>UINT32_MAX) stop("predicate-address-width");
            frame.worker=static_cast<Word>(w); frame.map=static_cast<Word>(m);
            frame.effectiveIndex=index; frame.effectiveTarget=&target;
            r->outcome.predicate=reader_insertion_predicate::observe(*r->input.frozen,frame,r->input.calls,r->input.admission);
            r->outcome.observedMismatch=r->outcome.predicate.observedMismatch;
            if(!r->outcome.observedMismatch) r->outcome.refusal=Refusal::Predicate;
            if(r->canceled.load(std::memory_order_acquire)) r->outcome.refusal=Refusal::Canceled;
            else if(std::chrono::steady_clock::now()>=r->input.deadline) r->outcome.refusal=Refusal::Deadline;
        }
        // Unconditional negative-only barrier. There is no tracked call to originalInner_.
        throw Rejected{r};
    }
public:
    static Core *create(InstallIO io) { return new Core(io); }
    void install(const Install &plan) {
        try {
        Phase empty=Phase::Empty;
        if(!phase_.compare_exchange_strong(empty,Phase::Installing)) stop("installation-reused");
        Core *none=nullptr;
        if(!active_.compare_exchange_strong(none,this,std::memory_order_acq_rel)) stop("second-core");
        const auto o=address(plan.outer),i=address(plan.inner);
        if(sizeof(void*)!=4 || !o || !i || o%4 || i%4 || o>UINT32_MAX-8 || i>UINT32_MAX-8 || (o<i+8 && i<o+8) ||
           !bytes(o,plan.outerBytes.data(),8) || !bytes(i,plan.innerBytes.data(),8) ||
           !rx(address(reinterpret_cast<void *>(&outerEntry)),8) || !rx(address(reinterpret_cast<void *>(&innerEntry)),8))
            stop("installation-preflight");
        void *outerOriginal=nullptr,*innerOriginal=nullptr;
        installOne(plan.outer,reinterpret_cast<void *>(&outerEntry),plan.outerBytes,&outerOriginal);
        originalOuter_=reinterpret_cast<Outer>(outerOriginal);
        installOne(plan.inner,reinterpret_cast<void *>(&innerEntry),plan.innerBytes,&innerOriginal);
        originalInner_=reinterpret_cast<Inner>(innerOriginal);
        // A later installer can invalidate an earlier result. Verify the complete
        // pair again only after both mutation callbacks have returned.
        verifyOne(plan.outer,reinterpret_cast<void *>(&outerEntry),plan.outerBytes,outerOriginal);
        verifyOne(plan.inner,reinterpret_cast<void *>(&innerEntry),plan.innerBytes,innerOriginal);
        phase_.store(Phase::Verified,std::memory_order_release);
        } catch(...) { stop("installation-exception"); }
    }
    bool registerOnce(Registration &&input) {
        if(phase_.load(std::memory_order_acquire)!=Phase::Verified || input.target.isNull() || !input.frozen ||
           !input.binding.worker || !input.binding.map || !input.binding.outerReturnPC || !input.binding.innerReturnPC ||
           input.binding.thread==std::thread::id{} || !input.residentNativeOwners || !validPolicy(input.binding) ||
           input.target.toString(QUuid::WithoutBraces)!=input.frozen->requestedTarget()) return false;
        bool no=false; if(!registered_.compare_exchange_strong(no,true,std::memory_order_acq_rel)) return false;
        record_.store(new Record(std::move(input)),std::memory_order_release); return true;
    }
    void cancel() { if(auto *r=record_.load(std::memory_order_acquire)) r->canceled.store(true,std::memory_order_release); }
    Outcome snapshot() const {
        auto *r=record_.load(std::memory_order_acquire);
        if(!r || !r->terminal.load(std::memory_order_acquire)) return {};
        Outcome result=r->outcome;
        result.interference=r->interference.load(std::memory_order_acquire);
        result.canceled=r->canceled.load(std::memory_order_acquire);
        result.witness=result.privateCaught && result.observedMismatch && !result.unknown &&
            !result.interference && !result.canceled && result.refusal==Refusal::None &&
            (!r->input.postRejectProbe || reader_insertion_mutex_probe::successful(result.postReject));
        return result;
    }
    __attribute__((noinline)) static void outerEntry(void *w,int i,const void *o,Word p,QSizeF z,int s,Word h,const QUuid &u) {
        const auto pc=reinterpret_cast<std::uintptr_t>(__builtin_extract_return_addr(__builtin_return_address(0)));
        ready()->outer(pc,w,i,o,p,z,s,h,u);
    }
    __attribute__((noinline)) static void innerEntry(void *m,int i,const QString &u,const QString &t,std::int32_t a,std::int32_t b) {
        const auto pc=reinterpret_cast<std::uintptr_t>(__builtin_extract_return_addr(__builtin_return_address(0)));
        ready()->inner(pc,m,i,u,t,a,b);
    }
};
}
