// Portable owned control flow only. No native hook, addresses or Task proof.
#define READER_INSERTION_LIFECYCLE_TESTING
#include "reader_insertion_guard_hooks.h"
#include <QCoreApplication>
#include <condition_variable>
#include <cstdio>
#include <stdexcept>
using namespace reader_insertion_hooks;
namespace reader_insertion_hooks::testing {
struct Lifecycle {
    static Core *make(Outer outer) {
        auto *core=Core::create({});core->originalOuter_=outer;
        core->phase_.store(Core::Phase::Verified);return core;
    }
    static void destroy(Core *core){delete core;}
    static bool retire(Core &core,const Core::Token &token){return core.retirePhysically(token);}
    static void invoke(Core &core,const Binding &b,const QUuid &target) {
        core.outer(b.outerReturnPC,b.worker,b.index,b.optional,b.sizePresent,b.size,b.sourceIndex,b.inherit,target);
    }
};
}
namespace {
unsigned checks=0;
void check(bool value){++checks;if(!value){std::fprintf(stderr,"core lifetime check %u failed\n",checks);std::abort();}}
std::mutex mutex;std::condition_variable cv;
bool entered=false,continue_=false,throw_=false;
void ownedOuter(void*,int,const void*,Word,QSizeF,int,Word,const QUuid&) {
    std::unique_lock<std::mutex> lock(mutex);entered=true;cv.notify_all();cv.wait(lock,[]{return continue_;});
    if(throw_)throw std::runtime_error("owned unwind");
}
Registration registration(std::thread::id thread) {
    Registration r;
    const QString source=QStringLiteral("11111111-1111-4111-8111-111111111111"),right=QStringLiteral("22222222-2222-4222-8222-222222222222"),
        wrong=QStringLiteral("33333333-3333-4333-8333-333333333333"),target=QStringLiteral("44444444-4444-4444-8444-444444444444");
    r.target=QUuid(target);r.frozen=reader_insertion_predicate::freeze({1,5,8,source,right,wrong,target}).frozen;
    r.binding={reinterpret_cast<void*>(0x1000),reinterpret_cast<void*>(0x2000),thread,1,2,2,1,nullptr,0,0,QSizeF(1,1)};
    r.residentNativeOwners=reinterpret_cast<void*>(0x3000);r.deadline=std::chrono::steady_clock::now()+std::chrono::seconds(30);
    return r;
}
}
int main(int argc,char **argv) {
    QCoreApplication app(argc,argv);
    auto *core=testing::Lifecycle::make(ownedOuter),*foreign=testing::Lifecycle::make(ownedOuter);
    Core::Token stale;
    for(unsigned n=0;n<200;++n) {
        bool ready=false,start=false;std::thread::id threadId;Binding binding;QUuid target;
        entered=false;continue_=false;throw_=n%2;
        std::thread worker([&] {
            {std::unique_lock<std::mutex> lock(mutex);threadId=std::this_thread::get_id();ready=true;cv.notify_all();cv.wait(lock,[&]{return start;});}
            try{testing::Lifecycle::invoke(*core,binding,target);}catch(const std::runtime_error&){}
        });
        {std::unique_lock<std::mutex> lock(mutex);cv.wait(lock,[&]{return ready;});}
        auto r=registration(threadId);binding=r.binding;target=r.target;
        auto token=core->registerRequest(std::move(r));check(bool(token));
        check(!core->registerRequest(registration(threadId)));
        check(!foreign->cancel(token)&&!foreign->snapshot(token).terminal&&!testing::Lifecycle::retire(*foreign,token));
        {std::lock_guard<std::mutex> lock(mutex);start=true;cv.notify_all();}
        {std::unique_lock<std::mutex> lock(mutex);cv.wait(lock,[]{return entered;});}
        check(!core->snapshot(token).terminal&&!testing::Lifecycle::retire(*core,token));
        check(!core->cancel(stale)&&!core->snapshot(stale).terminal&&!testing::Lifecycle::retire(*core,stale));
        check(core->cancel(token));
        {std::lock_guard<std::mutex> lock(mutex);continue_=true;cv.notify_all();}
        worker.join();
        check(core->snapshot(token).terminal&&core->snapshot(token).canceled);
        check(testing::Lifecycle::retire(*core,token));
        check(!core->cancel(token)&&!core->snapshot(token).terminal&&!testing::Lifecycle::retire(*core,token));
        stale=token;
    }
    testing::Lifecycle::destroy(core);testing::Lifecycle::destroy(foreign);
    std::printf("{\"passed\":true,\"checks\":%u,\"native_hook\":false}\n",checks);
}
