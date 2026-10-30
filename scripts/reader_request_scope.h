#pragma once
// Optional GUI-only request lifetime; no QObject, native closure or IPC state.
#include <functional>
#include <memory>
#include <utility>

namespace reader_request_scope {
struct Lease { virtual ~Lease() noexcept = default; };
class Scope;
class Binding final {
public:
    using Acquire = std::function<std::unique_ptr<Lease>()>;
    using Accept = std::function<bool()>;
    Binding() = default; // Explicit legacy behavior: no request fence selected.
    Binding(Acquire acquire, Accept accept)
        : hooks_(std::make_shared<const Hooks>(std::move(acquire),std::move(accept))) {}
    bool bound() const noexcept { return bool(hooks_); }
private:
    friend class Scope;
    struct Hooks {
        Acquire acquire;
        Accept accept;
        Hooks(Acquire a,Accept b):acquire(std::move(a)),accept(std::move(b)) {}
    };
    std::shared_ptr<const Hooks> hooks_;
};
// Declare before all reentrant locals. Adapter hooks are GUI-only, nonreentrant
// internals with noexcept destruction, not arbitrary application callbacks.
// The caller also frames full construction/start/delete expressions externally.
class Scope final {
public:
    explicit Scope(const Binding &binding):Scope(binding.hooks_) {}
    Scope(const Scope &)=delete;
    Scope &operator=(const Scope &)=delete;
    Scope(Scope &&)=delete;
    Scope &operator=(Scope &&)=delete;
    bool entered() const noexcept { return !hooks_ || bool(lease_); }
    bool active() const noexcept {
        if(!hooks_)return true;
        if(!lease_ || !hooks_->accept)return false;
        try { return hooks_->accept(); } catch(...) { return false; }
    }
private:
    // Own the argument before acquire(): deleting the source Binding cannot
    // invalidate the later hooks_ initialization. No borrowed source is reread.
    explicit Scope(std::shared_ptr<const Binding::Hooks> hooks)
        : lease_(acquire(hooks)),hooks_(std::move(hooks)) {}
    static std::unique_ptr<Lease> acquire(const std::shared_ptr<const Binding::Hooks> &hooks) noexcept {
        if(!hooks || !hooks->acquire || !hooks->accept)return {};
        try { return hooks->acquire(); } catch(...) { return {}; }
    }
    std::unique_ptr<Lease> lease_; // FIRST: destroyed LAST, after Hooks captures.
    std::shared_ptr<const Binding::Hooks> hooks_;
};
}
