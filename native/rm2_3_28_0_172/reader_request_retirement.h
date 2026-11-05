#pragma once
// Original GUI-only research fence. No IPC, native cancellation, drain or unload.
#include <QByteArray>
#include <QGuiApplication>
#include <QMetaObject>
#include <QPointer>
#include <QSet>
#include <QThread>
#include <functional>
#include <limits>
#include <memory>

namespace reader_retirement {
enum class Kind { ReadOnly, Mutation };
enum class Phase { Invalid, Active, Terminal, Retiring, Retired, Failed };
enum class Effect { NoMutationAttempted, ReconciliationRequired };
class Fence;
namespace detail {
struct State {
    QPointer<Fence> owner;
    QByteArray nonce;
    quint64 generation=0;
    unsigned depth=0;
    Phase phase=Phase::Active;
    Kind kind=Kind::ReadOnly;
    bool queued=false,cleanupDone=false;
};
}
class Token {
public:
    Token()=default;
    QByteArray nonce() const { return nonce_; }
    quint64 generation() const { return generation_; }
    explicit operator bool() const { return generation_!=0; }
private:
    friend class Fence;
    std::weak_ptr<detail::State> state_;
    QByteArray nonce_;
    quint64 generation_=0;
};
struct Snapshot {
    quint64 generation=0;
    QByteArray nonce;
    Phase phase=Phase::Invalid;
    unsigned depth=0;
    bool reconciliationRequired=false;
    QString refusal;
};
// Declare FIRST in the GUI scope and destroy LAST: all potentially reentrant
// locals/parameters/capture destruction must finish before its final epilogue.
// It cannot be copied/moved into an asynchronous callback or a foreign thread.
class Frame final {
public:
    Frame(const Frame&)=delete;
    Frame& operator=(const Frame&)=delete;
    Frame(Frame&&)=delete;
    Frame& operator=(Frame&&)=delete;
    ~Frame();
    bool valid() const { return bool(state_); }
    explicit operator bool() const { return valid(); }
private:
    friend class Fence;
    explicit Frame(std::shared_ptr<detail::State> state):state_(std::move(state)) { if(state_)++state_->depth; }
    std::shared_ptr<detail::State> state_;
};
class Fence final : public QObject {
    Q_OBJECT
public:
    // This finite nonce history is an owned research-profile cap, not the
    // production duplicate/journal policy. Exhaustion fails closed until exit.
    static constexpr int MaxRequests=256;
    explicit Fence(QObject *parent=nullptr):QObject(parent?parent:QCoreApplication::instance()) {}
    ~Fence() override {
        failed_=true;notifying_=true;
        if(current_)current_->owner=nullptr;
        if(unresolved_)unresolved_->owner=nullptr;
        cleanup_={}; // Owner already invalidated before capture destructors.
    }
    Token begin(const QByteArray &nonce,Kind kind) {
        Token token;
        if(!gui() || failed_ || notifying_ || (kind!=Kind::ReadOnly && kind!=Kind::Mutation) ||
            (current_ && (current_->phase!=Phase::Retired || current_->depth)) ||
            (kind==Kind::Mutation && unresolved_) || history_.size()>=MaxRequests ||
            generation_==std::numeric_limits<quint64>::max() || !validNonce(nonce) || history_.contains(nonce))return token;
        auto state=std::make_shared<detail::State>();state->owner=this;state->nonce=nonce;state->generation=++generation_;state->kind=kind;
        current_=state;history_.insert(nonce);
        token.state_=state;token.nonce_=nonce;token.generation_=state->generation;return token;
    }
    Frame enter(const Token &token) {
        if(!gui())return Frame({});
        auto state=resolve(token);
        if(!state || state!=current_ || state->phase==Phase::Retired || state->phase==Phase::Failed || state->depth>=64)return Frame({});
        return Frame(std::move(state));
    }
    // This check precedes ANY borrowed getter/action. Frame validity only tracks
    // a stack; it is not permission once Terminal or Retiring has been reached.
    bool accepts(const Token &token) const {
        if(!gui())return false;
        auto state=resolve(token);
        return !failed_ && state && state==current_ && state->phase==Phase::Active;
    }
    bool requestRetire(const Token &token,std::function<void()> cleanup,Effect effect) {
        if(!gui()) { cleanup={};return false; }
        const auto frame=enter(token); // First local, destroyed last.
        const QPointer<Fence> weak(this);
        auto state=resolve(token);
        if(!frame || !state || state!=current_ || state->phase!=Phase::Active) { cleanup={};return false; }
        state->phase=Phase::Terminal; // Invalidate callback/action permission now.
        if(effect!=Effect::NoMutationAttempted && effect!=Effect::ReconciliationRequired) {
            cleanup={};if(weak)weak->fail(state,QStringLiteral("effect-disposition-invalid"));return false;
        }
        if(effect==Effect::ReconciliationRequired) {
            if(unresolved_ && unresolved_!=state) { cleanup={};if(weak)weak->fail(state,QStringLiteral("multiple-unresolved-effects"));return false; }
            unresolved_=state;
        }
        cleanup_=std::move(cleanup);
        cleanup={}; // No parameter capture destructor may escape this frame.
        return bool(weak);
    }
    // Trusted caller handoff after independent reconciliation of this original
    // operation; this method is NOT proof, a native authority or an SDK receipt.
    bool acknowledgeReconciled(const Token &original) {
        if(!gui())return false;
        auto state=resolve(original);
        if(failed_ || notifying_ || !state || state!=unresolved_ || state->phase!=Phase::Retired)return false;
        unresolved_.reset();return true;
    }
    Snapshot snapshot(const Token &token) const {
        if(!gui())return {token.generation_,token.nonce_,Phase::Invalid,0,false,QStringLiteral("application-thread")};
        const auto state=resolve(token);
        return state?Snapshot{state->generation,state->nonce,state->phase,state->depth,state==unresolved_,refusal_}:Snapshot{token.generation_,token.nonce_,Phase::Invalid,0,false,{}};
    }
    // Admission handoff is later than the retired notification's own stack.
    // A resident heartbeat may observe this; it must not renew any deadline.
    bool handoffReady(const Token &token) const {
        if(!gui() || failed_ || notifying_)return false;
        const auto state=resolve(token);
        return state && state==current_ && state->phase==Phase::Retired && state->depth==0;
    }
signals:
    void retired(quint64 generation,QByteArray nonce);
    void retirementFailed(quint64 generation,QByteArray nonce);
private:
    friend class Frame;
    bool gui() const {
        auto *app=qobject_cast<QGuiApplication*>(QCoreApplication::instance());
        return app && !QCoreApplication::closingDown() && thread()==app->thread() && QThread::currentThread()==app->thread();
    }
    static bool validNonce(const QByteArray &nonce) {
        if(nonce.size()!=32)return false;
        for(char c:nonce)if(!((c>='0'&&c<='9')||(c>='a'&&c<='f')))return false;
        return true;
    }
    std::shared_ptr<detail::State> resolve(const Token &token) const {
        auto state=token.state_.lock();
        return state && state->owner==this && state->generation==token.generation_ && state->nonce==token.nonce_?state:nullptr;
    }
    void leave(const std::shared_ptr<detail::State> &state) {
        if(!gui() || state->owner!=this || !state->depth)return;
        --state->depth;
        // Final non-reentrant epilogue only. Never call user cleanup or emit here.
        if(!state->depth && (state->phase==Phase::Terminal || (state->phase==Phase::Retiring && state->cleanupDone)))schedule(state);
    }
    void schedule(const std::shared_ptr<detail::State> &state) {
        if(failed_ || state!=current_ || state->queued)return;
        state->queued=true;
        const QPointer<Fence> weak(this);
        if(!QMetaObject::invokeMethod(this,[weak,state]{if(weak)weak->check(state);},Qt::QueuedConnection)) {
            state->queued=false;fail(state,QStringLiteral("retirement-queue-failed"));
        }
    }
    void check(const std::shared_ptr<detail::State> &state) {
        state->queued=false;
        if(!gui() || failed_ || state!=current_ || state->owner!=this)return;
        if(state->depth)return; // Outermost exit will schedule. No nested-loop spin.
        if(state->phase==Phase::Terminal) {
            const Frame frame(state); // Holds every cleanup/local destructor below.
            const QPointer<Fence> weak(this);
            auto cleanup=std::move(cleanup_);
            state->phase=Phase::Retiring;
            bool threw=false;
            try { if(cleanup)cleanup(); } catch(...) { threw=true; }
            cleanup={}; // Last capture may pump GUI or destroy this owner.
            if(!weak)return;
            if(threw) { weak->fail(state,QStringLiteral("cleanup-exception"));return; }
            state->cleanupDone=true;
            return; // Frame exit schedules the distinct later completion turn.
        }
        if(state->phase==Phase::Retiring && state->cleanupDone) {
            state->phase=Phase::Retired;
            const QPointer<Fence> weak(this);
            notifying_=true;emit retired(state->generation,state->nonce);
            if(weak)weak->notifying_=false; // A later admission turn, not this slot.
        }
    }
    void fail(const std::shared_ptr<detail::State> &state,QString reason) {
        failed_=true;refusal_=std::move(reason);state->phase=Phase::Failed;
        // Failure is sticky: no next request or optimistic resource reuse.
        const QPointer<Fence> weak(this);notifying_=true;emit retirementFailed(state->generation,state->nonce);
        if(weak)weak->notifying_=false;
    }
    std::shared_ptr<detail::State> current_,unresolved_;
    std::function<void()> cleanup_;
    QSet<QByteArray> history_;
    quint64 generation_=0;
    bool failed_=false,notifying_=false;
    QString refusal_;
};
inline Frame::~Frame() {
    if(!state_)return;
    const QPointer<Fence> owner=state_->owner;
    if(owner)owner->leave(state_);
    // State itself holds no cleanup function/QObject/native owner to destroy.
}
} // namespace reader_retirement
