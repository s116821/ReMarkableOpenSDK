// Production bridge against an OWNED ELF, at the exact retained instruction VAs.
#include "composer_pre_enter.h"
#include <QGuiApplication>
#include <QTextCursor>
#include <array>
#include <cassert>
#include <cstdio>
#include <future>
#include <memory>
#include <thread>
#include <vector>
#include <sys/mman.h>
#include <unistd.h>
using namespace remarkable;
namespace pe=composer_pre_enter;
using pe::detail::StockPair;
namespace remarkable::composer_pre_enter::detail {
struct OwnedTestAccess {
    static void publishDormant() {
        // Independent owned scenarios reuse the patched callsite while idle.
        // Production publishes one route and never replaces it.
        Route::current_.store(new Route,std::memory_order_release);
    }
    static State *state() { auto *r=Route::current(); return r?r->state():nullptr; }
};
}
namespace remarkable::composer_pre_enter {
struct TestAccess {
    static std::unique_ptr<Capture> arm(composer_text::ReadRange range={0,3},
            std::chrono::milliseconds budget=std::chrono::seconds(5)) {
        return Capture::armRequest(range,budget);
    }
    static std::shared_ptr<detail::State> state(Capture &c) { return c.state_; }
    static std::unique_ptr<Capture> armUntil(composer_text::Clock::time_point deadline,
            std::shared_ptr<const std::atomic<bool>> retirement,bool &occupied) {
        return Capture::armUntil({0,3},deadline,std::move(retirement),occupied);
    }
};
}
extern "C" void owned_enter(void *,const StockPair *,const QString *);
extern "C" int owned_abi_enter(void *,const StockPair *,const QString *);
extern "C" std::uint32_t owned_site,owned_veneer[];

struct Item {
    alignas(8) std::array<unsigned char,0x300> storage{};
    QTextDocument &doc() { return *reinterpret_cast<QTextDocument *>(storage.data()+0x1ec); }
    Item() { new (storage.data()+0x1ec) QTextDocument; }
    ~Item() { doc().~QTextDocument(); }
};
static unsigned calls=0,cleanups=0;
static int mode=0;
static void *expectedParticipant;
static const StockPair *expectedOwner;
static const QString *expectedText;
static void *fixtureScene;
static std::atomic<bool> *fixtureRetirement;
static bool fixtureDeadlineWaited;
struct Marker { ~Marker() { ++cleanups; } };

extern "C" __attribute__((noinline,section(".stock_original")))
void stock_original(void *p,const StockPair *o,const QString *s) {
    std::uintptr_t stack=0; asm volatile("mov %0, sp" : "=r"(stack));
    assert((stack&7)==0);
    ++calls;
    assert(p==expectedParticipant && o==expectedOwner && s==expectedText);
    if (mode==1) throw 72;
    if (mode==2) { mode=0; owned_enter(p,o,s); }
    if (mode==3) pe::detail::OwnedTestAccess::state()->canceled.store(true);
    if (mode==4) fixtureRetirement->store(true,std::memory_order_release);
    auto &doc=*reinterpret_cast<QTextDocument *>(o->item+0x1ec);
    QTextCursor cursor(&doc); cursor.movePosition(QTextCursor::End); cursor.insertText(*s);
}

static void put(void *base,std::size_t offset,std::uintptr_t value) {
    assert(value<=UINT32_MAX); auto v=static_cast<std::uint32_t>(value);
    std::memcpy(static_cast<unsigned char *>(base)+offset,&v,4);
}
static void writable(void *address,bool write) {
    const auto page=static_cast<std::uintptr_t>(sysconf(_SC_PAGESIZE));
    const auto start=reinterpret_cast<std::uintptr_t>(address)&~(page-1);
    assert(mprotect(reinterpret_cast<void *>(start),page,
                    PROT_READ|(write?PROT_WRITE:PROT_EXEC))==0);
}
// Test-only installer owns this ELF before any fixture thread/route executes.
// It offers no production token, pointer installer or caller-set safety flag.
static void installOwned() {
    assert(reinterpret_cast<std::uintptr_t>(&owned_site)==pe::detail::Site);
    assert(reinterpret_cast<std::uintptr_t>(&stock_original)==pe::detail::Original);
    assert(owned_site==pe::detail::OriginalWord);
    std::uint32_t bl=0;
    assert(pe::detail::branch(pe::detail::Site,reinterpret_cast<std::uintptr_t>(owned_veneer),bl));
    const auto shim=reinterpret_cast<std::uintptr_t>(&pe::detail::remarkable_pre_enter_shim);
    assert(!(shim&3) && shim<=UINT32_MAX);
    writable(owned_veneer,true);
    owned_veneer[0]=pe::detail::VeneerLoad; owned_veneer[1]=static_cast<std::uint32_t>(shim);
    __builtin___clear_cache(reinterpret_cast<char *>(owned_veneer),reinterpret_cast<char *>(owned_veneer+2));
    writable(owned_veneer,false);
    writable(&owned_site,true); owned_site=bl;
    __builtin___clear_cache(reinterpret_cast<char *>(&owned_site),reinterpret_cast<char *>(&owned_site+1));
    writable(&owned_site,false);
    assert(owned_site==bl);
}

int main(int argc,char **argv) {
    QGuiApplication app(argc,argv);
    static_assert(sizeof(StockPair)==8);
    std::uint32_t word=0;
    assert(pe::detail::branch(pe::detail::Site,pe::detail::Original,word));
    assert(word==pe::detail::OriginalWord);
    assert(!pe::detail::branch(pe::detail::Site,pe::detail::Site+33554440,word));
    assert(!pe::detail::branch(pe::detail::Site,pe::detail::Original+1,word));
    static_assert(!pe::detail::StartupSelected);
    assert(pe::detail::pinPreparedRouteCode());
    assert(!pe::TestAccess::arm()); // Uninstalled or disabled profile.
    installOwned();
    pe::detail::OwnedTestAccess::publishDormant();
    assert(!pe::TestAccess::arm({-1,3}));
    assert(!pe::TestAccess::arm({0,3},std::chrono::milliseconds(0)));
    alignas(8) std::array<unsigned char,0x280> scene{};
    alignas(8) std::array<unsigned char,0x60> participant{};
    fixtureScene=scene.data();
    auto owner=std::make_shared<Item>();
    StockPair pair{};
    static_assert(sizeof(owner)==sizeof(pair));
    std::memcpy(&pair,&owner,sizeof(pair)); // Existing toolchain's owned shared_ptr ABI.
    assert(pair.item==reinterpret_cast<std::uintptr_t>(owner.get()));
    const auto item=reinterpret_cast<std::uintptr_t>(owner.get());
    put(scene.data(),0x190,item); put(scene.data(),0x194,pair.control);
    put(scene.data(),0x26c,reinterpret_cast<std::uintptr_t>(participant.data()));
    put(participant.data(),8,reinterpret_cast<std::uintptr_t>(scene.data()));
    put(owner->storage.data(),0,0x133e700); put(owner->storage.data(),8,0x133e758);
    put(owner->storage.data(),0x260,UINT32_MAX);
    const QString lf=QStringLiteral("\n");
    expectedParticipant=participant.data(); expectedOwner=&pair; expectedText=&lf;
    // State storage is owned and retained for all route entries. No native owner
    // or fixture callback is substituted by the production shim.
    std::vector<std::unique_ptr<pe::Capture>> handles;
    auto prepare=[&]() -> pe::detail::State & {
        owner->doc().setPlainText(QStringLiteral("abc"));
        pe::detail::OwnedTestAccess::publishDormant();
        handles.push_back(pe::TestAccess::arm());
        assert(handles.back());
        return *pe::TestAccess::state(*handles.back());
    };
    auto call=[&]() {
        Marker marker; const auto before=calls; const auto repeated=mode==2;
        assert(owned_abi_enter(participant.data(),&pair,&lf)==1);
        assert(calls==before+(repeated?2U:1U));
    };
    auto &normal=prepare(); call();
    assert(normal.result.status==composer_text::Status::Copied && normal.result.count==3);
    assert(normal.result.units[0]=='a' && owner->doc().toPlainText()==QStringLiteral("abc\n"));
    assert(calls==1 && owner.use_count()==1);
    call(); assert(calls==2 && normal.result.count==3); // consumed one-shot
    assert(!pe::TestAccess::arm());
    pe::detail::OwnedTestAccess::publishDormant();
    owner->doc().setPlainText(QStringLiteral("abc"));
    call();
    assert(!pe::detail::OwnedTestAccess::state()&&owner->doc().toPlainText()==QStringLiteral("abc\n"));
    auto &nesting=prepare(); put(participant.data(),0x44,1); call();
    assert(nesting.result.status==composer_text::Status::Refused); put(participant.data(),0x44,0);
    auto &qtBlock=prepare(); QTextCursor block(&owner->doc()); block.beginEditBlock(); call();
    assert(qtBlock.result.status==composer_text::Status::Refused); block.endEditBlock();
    auto &preedit=prepare(); auto b=owner->doc().begin();
    b.layout()->setPreeditArea(0,QStringLiteral("x")); call();
    assert(preedit.result.status==composer_text::Status::Refused);
    b.layout()->setPreeditArea(-1,QString());
    auto &wrongOwner=prepare(); put(scene.data(),0x194,pair.control+4); call();
    assert(wrongOwner.result.status==composer_text::Status::Refused); put(scene.data(),0x194,pair.control);
    auto &drift=prepare(); drift.beforeRecheck=+[] { put(fixtureScene,0x194,0); }; call();
    assert(drift.result.failure==composer_text::Failure::Join && !drift.result.count);
    put(scene.data(),0x194,pair.control);
    auto &captureThrow=prepare(); captureThrow.beforeRead=+[] { throw 19; }; call();
    assert(captureThrow.result.failure==composer_text::Failure::Exception);
    auto &originalThrow=prepare(); mode=1; auto beforeCalls=calls, beforeCleanup=cleanups;
    try { call(); assert(false); } catch (int value) { assert(value==72); }
    assert(calls==beforeCalls+1 && cleanups==beforeCleanup+1 && originalThrow.originalThrew.load());
    assert(originalThrow.result.status==composer_text::Status::Refused && owner.use_count()==1);
    mode=0;
    auto &canceled=prepare(); canceled.canceled.store(true); call();
    assert(canceled.result.failure==composer_text::Failure::Canceled);
    pe::detail::OwnedTestAccess::publishDormant();
    auto expired=pe::TestAccess::arm({0,3},std::chrono::milliseconds(1));
    assert(expired);
    std::this_thread::sleep_until(pe::TestAccess::state(*expired)->deadline);
    call();
    assert(expired->snapshot().text.failure==composer_text::Failure::Deadline);
    assert(!pe::TestAccess::arm());
    auto &during=prepare(); mode=3; call(); mode=0;
    assert(during.result.failure==composer_text::Failure::Canceled);
    auto &nested=prepare(); mode=2; beforeCalls=calls; call();
    assert(calls==beforeCalls+2 && nested.result.status==composer_text::Status::Copied);
    assert(owner->doc().toPlainText()==QStringLiteral("abc\n\n"));
    pe::detail::OwnedTestAccess::publishDormant();
    auto observed=pe::TestAccess::arm();
    assert(observed);
    owner->doc().setPlainText(QStringLiteral("abc"));
    auto observedState=pe::TestAccess::state(*observed);
    // Publication contention is a real dropped candidate, not a blocking stock
    // call or accepted partial snapshot.
    // A mutex cannot be try_locked by its owning thread. Keep Qt/native entry
    // on this thread and let a distinct bounded holder supply real contention.
    // The only executable patch was installed before any fixture thread started.
    std::promise<void> acquired,release;
    auto acquiredFuture=acquired.get_future();
    auto releaseFuture=release.get_future();
    const auto entryThread=std::this_thread::get_id();
    std::thread holder([&,observedState] {
        assert(std::this_thread::get_id()!=entryThread);
        std::unique_lock<std::mutex> lock(observedState->mutex);
        acquired.set_value();
        assert(releaseFuture.wait_for(std::chrono::seconds(5))==std::future_status::ready);
    });
    assert(acquiredFuture.wait_for(std::chrono::seconds(5))==std::future_status::ready);
    call();
    release.set_value();
    holder.join();
    auto snapshot=observed->snapshot();
    assert(snapshot.available && snapshot.finished && snapshot.originalReturned);
    assert(snapshot.text.status==composer_text::Status::Refused && !snapshot.text.count);
    pe::detail::OwnedTestAccess::publishDormant();
    auto canceledHandle=pe::TestAccess::arm();
    assert(canceledHandle);
    canceledHandle->cancel(); call();
    assert(canceledHandle->snapshot().text.failure==composer_text::Failure::Canceled);
    assert(!pe::TestAccess::arm());
    pe::detail::OwnedTestAccess::publishDormant();
    auto gone=pe::TestAccess::arm();
    assert(gone);
    std::weak_ptr<pe::detail::State> weak=pe::TestAccess::state(*gone);
    gone.reset(); assert(!weak.expired()&&!pe::TestAccess::arm());
    call(); // Canceled handle, retained route.
    assert(weak.lock()->result.failure==composer_text::Failure::Canceled);

    pe::detail::OwnedTestAccess::publishDormant();
    owner->doc().setPlainText(QStringLiteral("abc"));
    std::array<std::unique_ptr<pe::Capture>,2> competing;
    std::promise<void> start;
    auto ready=start.get_future().share();
    std::thread first([&]{ready.wait(); competing[0]=pe::TestAccess::arm({0,1});});
    std::thread second([&]{ready.wait(); competing[1]=pe::TestAccess::arm({1,1});});
    start.set_value();
    first.join(); second.join();
    assert(bool(competing[0])!=bool(competing[1]));
    const auto winner=competing[0] ? 0U : 1U;
    assert(!pe::TestAccess::arm());
    call(); call();
    const auto won=competing[winner]->snapshot();
    assert(won.finished&&won.originalReturned&&won.text.count==1&&won.text.units[0]=='a'+winner);
    assert(owner->doc().toPlainText()==QStringLiteral("abc\n\n"));
    competing[winner].reset();
    assert(!pe::TestAccess::arm()&&!weak.expired());
    pe::detail::OwnedTestAccess::publishDormant();
    auto retired=std::make_shared<std::atomic<bool>>(true);
    const auto originalDeadline=composer_text::Clock::now()+std::chrono::seconds(5);
    bool occupied=false;
    assert(!pe::TestAccess::armUntil(originalDeadline,retired,occupied)&&!occupied);
    auto live=std::make_shared<std::atomic<bool>>(false);
    assert(!pe::TestAccess::armUntil(composer_text::Clock::now(),live,occupied)&&!occupied);
    auto leased=pe::TestAccess::armUntil(originalDeadline,live,occupied);
    assert(leased&&pe::TestAccess::state(*leased)->deadline==originalDeadline);
    owner->doc().setPlainText(QStringLiteral("abc"));
    fixtureRetirement=live.get();
    pe::TestAccess::state(*leased)->beforeRecheck=+[] {
        fixtureRetirement->store(true,std::memory_order_release);
    };
    call();
    assert(live->load()&&leased->snapshot().text.failure==composer_text::Failure::Canceled);
    assert(!leased->snapshot().text.count);
    leased.reset();
    assert(!pe::TestAccess::armUntil(originalDeadline,live,occupied)&&occupied);
    pe::detail::OwnedTestAccess::publishDormant();
    auto lateRetirement=std::make_shared<std::atomic<bool>>(false);
    fixtureRetirement=lateRetirement.get();
    auto publishing=pe::TestAccess::armUntil(originalDeadline,lateRetirement,occupied);
    assert(publishing);
    owner->doc().setPlainText(QStringLiteral("abc"));
    mode=4;call();mode=0;
    assert(lateRetirement->load()&&publishing->snapshot().text.failure==composer_text::Failure::Canceled);
    assert(!publishing->snapshot().text.count);
    fixtureRetirement=nullptr;
    pe::detail::OwnedTestAccess::publishDormant();
    const auto shortDeadline=composer_text::Clock::now()+std::chrono::milliseconds(100);
    auto expiring=pe::TestAccess::armUntil(shortDeadline,std::make_shared<std::atomic<bool>>(false),occupied);
    assert(expiring);
    owner->doc().setPlainText(QStringLiteral("abc"));
    pe::TestAccess::state(*expiring)->beforeRecheck=+[] {
        fixtureDeadlineWaited=true;
        std::this_thread::sleep_until(pe::detail::OwnedTestAccess::state()->deadline);
    };
    call();
    assert(fixtureDeadlineWaited&&expiring->snapshot().text.failure==composer_text::Failure::Deadline);
    assert(!expiring->snapshot().text.count);
    std::puts("owned ARM lease: original absolute deadline, in-copy expiry and retirement, and pre-publication retirement passed");
    std::puts("owned ARM arming: dormant and uninstalled routes, competing requests, expiry, cancellation, and retained one-shot state passed");
    std::puts("owned ARM patched BL -> near veneer -> compiled shim: copy, once, refusal, cancellation, reentry and original unwind passed");
}
