#pragma once
#include <QGuiApplication>
#include <QThread>
#include <memory>

namespace reader_worker_native {class ResidentMutation;namespace testing {struct MutationAccess;}}
namespace remarkable::rm2_3_28_0_172 {
class ResidentSelection;
// A single process-resident effect reservation shared across executor facades.
// Dropping a token or a connection cannot release it. Only the owning executor's
// proven no-effect/retired path may release the exact private token. Read-only
// observation never acquires or clears this gate.
class ResidentEffectGate final {
    struct State {};
public:
    class Token final {
        std::weak_ptr<State> state_;
        explicit Token(const std::shared_ptr<State> &state):state_(state){}
        friend class ResidentEffectGate;
    public:
        Token()=default;
        explicit operator bool()const{return !state_.expired();}
    };
    static ResidentEffectGate &process(){static auto *gate=new ResidentEffectGate;return *gate;}
    bool busy()const{return !gui()||bool(active_);}
private:
    std::shared_ptr<State> active_;
    ResidentEffectGate()=default;
    ~ResidentEffectGate()=default;
    ResidentEffectGate(const ResidentEffectGate&)=delete;
    ResidentEffectGate &operator=(const ResidentEffectGate&)=delete;
    friend class reader_worker_native::ResidentMutation;
    friend class ResidentSelection;
#ifdef RESIDENT_MUTATION_TESTING
    friend struct reader_worker_native::testing::MutationAccess;
#endif
    static bool gui(){
        auto *app=qobject_cast<QGuiApplication*>(QCoreApplication::instance());
        return app&&!QCoreApplication::closingDown()&&QThread::currentThread()==app->thread();
    }
    Token acquire(){
        if(busy())return {};
        active_=std::make_shared<State>();return Token(active_);
    }
    bool release(const Token &token){
        if(!gui())return false;
        const auto state=token.state_.lock();
        if(!state||state!=active_)return false;
        active_.reset();return true;
    }
};
}
