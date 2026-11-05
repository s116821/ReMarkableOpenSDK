#pragma once
// Original, opt-in Qt diagnostic. No native resolver, dispatch or composer lease.
#include <QMetaObject>
#include <QObject>
#include <QTextDocument>
#include <QThread>
#include <array>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <memory>
#include <mutex>
#include <thread>
#include <typeinfo>

namespace remarkable::composer_diagnostic {
using Clock = std::chrono::steady_clock;
enum class Event { Attached, ContentsChange, ContentsChanged, Destroyed };
enum class End { Open, Canceled, Deadline, Capacity, Contention, WrongThread,
                 Destroyed, ConnectFailed };
struct Record {
    Event event = Event::Attached;
    std::int64_t elapsedNanoseconds = 0;
    std::uintptr_t emitterThread = 0; // Diagnostic OS/Qt identifier, not a lease.
    unsigned recorderDepth = 0;      // This recorder only, not native stack depth.
    unsigned concurrentEntries = 0;
    bool revisionPresent = false;
    int revision = 0;
    int position = -1, removed = -1, added = -1;
};
struct Snapshot {
    static constexpr std::size_t Capacity = 256;
    std::array<Record, Capacity> records{};
    std::size_t count = 0;
    std::uintptr_t documentAddress = 0; // Never dereferenced by this value.
    End end = End::Open;
    unsigned activeCallbacks = 0;
    bool available = false;
    // Even a closed snapshot with zero active callbacks is NOT an unload permit.
};
static_assert(sizeof(Snapshot) < 65536, "Diagnostic storage must stay bounded");

class Observer final {
    struct State {
        const std::thread::id owner = std::this_thread::get_id();
        const Clock::time_point started = Clock::now();
        const Clock::time_point deadline;
        const std::uintptr_t address;
        const std::size_t limit;
        std::atomic<End> end{End::Open};
        std::atomic<unsigned> active{0};
        std::mutex mutex;
        std::array<Record, Snapshot::Capacity> records{};
        std::size_t count = 0;
        State(QTextDocument &doc, std::chrono::milliseconds budget, std::size_t cap)
            : deadline(started + budget),
              address(reinterpret_cast<std::uintptr_t>(&doc)), limit(cap) {}
        void close(End reason) noexcept {
            End open = End::Open;
            end.compare_exchange_strong(open, reason, std::memory_order_acq_rel);
        }
        void expire() noexcept {
            if (Clock::now() >= deadline) close(End::Deadline);
        }
    };
    struct Entry {
        State &state;
        Entry *previous;
        unsigned depth = 1, concurrent;
        static inline thread_local Entry *top = nullptr;
        explicit Entry(State &s) noexcept : state(s), previous(top),
            concurrent(s.active.fetch_add(1, std::memory_order_acq_rel) + 1) {
            for (auto *p = previous; p; p = p->previous)
                if (&p->state == &state) ++depth;
            top = this;
        }
        ~Entry() { top = previous; state.active.fetch_sub(1, std::memory_order_release); }
    };
    std::shared_ptr<State> state_;
    std::array<QMetaObject::Connection, 3> connections_{};
    explicit Observer(std::shared_ptr<State> state) : state_(std::move(state)) {}

    static void record(const std::shared_ptr<State> &s, QTextDocument *doc,
                       Event event, int position = -1, int removed = -1, int added = -1) {
        Entry entry(*s);
        s->expire();
        if (s->end.load(std::memory_order_acquire) != End::Open) return;
        // A direct connection can run on an unexpected emitting thread.
        // Never touch QTextDocument on that path, including its thread() getter.
        const bool owner = std::this_thread::get_id() == s->owner;
        Record value;
        value.event = event;
        value.elapsedNanoseconds =
            std::chrono::duration_cast<std::chrono::nanoseconds>(Clock::now() - s->started).count();
        value.emitterThread = reinterpret_cast<std::uintptr_t>(QThread::currentThreadId());
        value.recorderDepth = entry.depth;
        value.concurrentEntries = entry.concurrent;
        value.position = position; value.removed = removed; value.added = added;
        // destroyed() is emitted from QObject's destructor: derived state is gone.
        const bool readable = owner && doc && event != Event::Destroyed &&
                              doc->thread() == QThread::currentThread();
        if (readable) { value.revision = doc->revision(); value.revisionPresent = true; }
        std::unique_lock<std::mutex> lock(s->mutex, std::try_to_lock);
        if (!lock.owns_lock()) { s->close(End::Contention); return; }
        s->expire();
        if (s->end.load(std::memory_order_acquire) != End::Open) return;
        if (s->count == s->limit) { s->close(End::Capacity); return; }
        s->records[s->count++] = value;
        if (event == Event::Destroyed) s->close(End::Destroyed);
        else if (!readable) s->close(End::WrongThread);
        else if (s->count == s->limit) s->close(End::Capacity);
    }
    void disconnect() noexcept {
        for (auto &connection : connections_) {
            QObject::disconnect(connection);
            connection = {};
        }
    }
public:
    Observer(const Observer &) = delete;
    Observer &operator=(const Observer &) = delete;
    // Handle methods/destruction must be serialized by its caller. Native signal
    // delivery may be concurrent. No QObject pointer is touched by stop/snapshot.
    ~Observer() { stop(); }

    // Caller guarantees a live object for this entire call, on its owning thread.
    // Exact type avoids arbitrary subclass connectNotify/disconnectNotify hooks.
    // Neither a pointer nor this successful attachment proves native provenance.
    static std::unique_ptr<Observer> attach(QTextDocument &doc,
            std::chrono::milliseconds budget = std::chrono::milliseconds(120000),
            std::size_t maximumRecords = Snapshot::Capacity) {
        if (budget.count() < 1 || budget > std::chrono::milliseconds(120000) ||
            maximumRecords < 1 || maximumRecords > Snapshot::Capacity ||
            doc.thread() != QThread::currentThread() || typeid(doc) != typeid(QTextDocument))
            return {};
        auto state = std::make_shared<State>(doc, budget, maximumRecords);
        auto result = std::unique_ptr<Observer>(new Observer(state));
        // Context is the sender itself: Qt disconnects it on sender destruction.
        // Captured shared state, never 'this', survives an entered Qt functor.
        result->connections_[0] = QObject::connect(&doc, &QTextDocument::contentsChange, &doc,
            [state, sender = &doc](int pos, int removed, int added) {
                record(state, sender, Event::ContentsChange, pos, removed, added);
            }, Qt::DirectConnection);
        result->connections_[1] = QObject::connect(&doc, &QTextDocument::contentsChanged, &doc,
            [state, sender = &doc] { record(state, sender, Event::ContentsChanged); },
            Qt::DirectConnection);
        result->connections_[2] = QObject::connect(&doc, &QObject::destroyed, &doc,
            [state] { record(state, nullptr, Event::Destroyed); }, Qt::DirectConnection);
        for (const auto &connection : result->connections_)
            if (!connection) {
                state->close(End::ConnectFailed); result->disconnect(); return result;
            }
        record(state, &doc, Event::Attached);
        return result;
    }
    void stop() noexcept {
        state_->close(End::Canceled);
        disconnect(); // Does not assert that all preempted Qt activations returned.
    }
    // Caller/supervisor polling removes connections after idle expiry. No timer,
    // worker dispatch or background thread is created by this observer.
    void poll() noexcept {
        state_->expire();
        if (state_->end.load(std::memory_order_acquire) != End::Open) disconnect();
    }
    Snapshot snapshot() const {
        const auto s = state_;
        s->expire();
        Snapshot result;
        result.documentAddress = s->address;
        result.end = s->end.load(std::memory_order_acquire);
        result.activeCallbacks = s->active.load(std::memory_order_acquire);
        std::unique_lock<std::mutex> lock(s->mutex, std::try_to_lock);
        if (!lock.owns_lock()) return result;
        result.records = s->records;
        result.count = s->count;
        result.end = s->end.load(std::memory_order_acquire);
        result.activeCallbacks = s->active.load(std::memory_order_acquire);
        result.available = true;
        return result;
    }
};
} // namespace remarkable::composer_diagnostic
