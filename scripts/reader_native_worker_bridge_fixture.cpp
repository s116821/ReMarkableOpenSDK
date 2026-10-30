// Owned mock machine-ABI fixtures. Never calls the sealed native addresses.
#define READER_WORKER_NATIVE_TESTING
#include "reader_native_worker_bridge.h"
#include <QDebug>
#include <stdexcept>
#include <thread>

struct NativeId { std::uint32_t words[3] = {17, 23, 31}; };
Q_DECLARE_METATYPE(NativeId)
struct WrongId {
    static inline int copies = 0;
    std::uint32_t words[3] = {};
    WrongId() = default;
    WrongId(const WrongId &other) { std::memcpy(words, other.words, sizeof(words)); ++copies; }
};
Q_DECLARE_TYPEINFO(WrongId, Q_RELOCATABLE_TYPE);
Q_DECLARE_METATYPE(WrongId)
namespace xofm::libs::library {
class DocumentController final : public QObject { Q_OBJECT };
}
class DocumentWorker final : public QThread { Q_OBJECT };
using namespace reader_worker_native;
namespace {
int checks = 0;
void require(bool value, const char *message) {
    ++checks;
    if (!value) qFatal("native bridge fixture: %s", message);
}
struct Scenario {
    int document = 0, documentControl = 0, workerControl = 0;
    int resolves = 0, acquires = 0, requests = 0, docReleases = 0, workerReleases = 0;
    bool docEmpty = false, workerEmpty = false, docAlias = false, wrongWorker = false;
    bool resolveThrow = false, acquireThrow = false, requestThrow = false;
    qulonglong jobId = 0x1234567800000001ULL;
    DocumentWorker worker, otherWorker;
    xofm::libs::library::DocumentController controller;
    std::function<void()> resolveHook, acquireHook, requestHook, releaseHook;
};
Scenario *active = nullptr;
void writeOwner(void *out, void *raw, void *control) {
    const std::array<std::uintptr_t, 2> value{{reinterpret_cast<std::uintptr_t>(raw), reinterpret_cast<std::uintptr_t>(control)}};
    std::memcpy(out, value.data(), sizeof(value));
}
void resolve(void *out, void *controller, const void *id) {
    auto &s = *active; ++s.resolves;
    require(controller == &s.controller, "resolver receiver");
    require(static_cast<const NativeId *>(id)->words[0] == 17, "actual native ID storage");
    if (s.resolveThrow) throw std::runtime_error("resolver");
    if (s.resolveHook) s.resolveHook();
    writeOwner(out, s.docEmpty || s.docAlias ? nullptr : &s.document,
               s.docEmpty ? nullptr : &s.documentControl);
}
void acquire(void *out, void *document, int mode) {
    auto &s = *active; ++s.acquires;
    require(document == &s.document && mode == 1, "existing-only acquisition ABI");
    if (s.acquireThrow) throw std::runtime_error("acquire");
    if (s.acquireHook) s.acquireHook();
    writeOwner(out, s.workerEmpty ? nullptr : (s.wrongWorker ? &s.otherWorker : &s.worker),
               s.workerEmpty ? nullptr : &s.workerControl);
}
qulonglong request(void *worker, const std::function<void()> &callback) {
    auto &s = *active; ++s.requests;
    require(worker == &s.worker, "request complete-object receiver");
    if (s.requestThrow) throw std::runtime_error("request");
    if (s.requestHook) s.requestHook();
    callback();
    return s.jobId;
}
void release(void *control) {
    auto &s = *active;
    require(QThread::currentThread() == QCoreApplication::instance()->thread(), "GUI release");
    if (control == &s.documentControl) ++s.docReleases;
    else if (control == &s.workerControl) ++s.workerReleases;
    else require(false, "release receives control block, not pair/raw");
    if (s.releaseHook) s.releaseHook();
}
Loading bridge() {
    const auto type = QMetaType::fromType<NativeId>();
    return testing::load({resolve, acquire, request, release, type.iface(), &DocumentWorker::staticMetaObject});
}
Preparation prepare(Bridge &b, Scenario &s, const Permit &permit = [] { return true; }) {
    return b.prepare(&s.controller, QVariant::fromValue(NativeId{}), &s.worker, permit);
}
}

int main(int argc, char **argv) {
    QCoreApplication app(argc, argv);
    {
        Scenario s; active = &s;
        require(!loadPinned([] { return true; }).bridge, "owned executable is unsupported by real loader");
        require(s.resolves == 0, "real loader never reaches mock/native calls");
    }
    {
        Scenario s; active = &s; auto loaded = bridge(); auto p = prepare(*loaded.bridge, s);
        require(bool(p.prepared) && p.prepared->worker() == &s.worker, "strong matching preparation");
        int callbacks = 0;
        auto submitted = p.prepared->submit([&] { ++callbacks; }, [] { return true; });
        require(submitted.accepted && submitted.attempted && !submitted.unknown &&
                submitted.jobId == s.jobId && callbacks == 1, "one full 64-bit submission");
        require(!p.prepared->submit([] {}, [] { return true; }).attempted && s.requests == 1, "never resubmit");
        require(p.prepared->close() && p.prepared->close(), "idempotent close");
        p.prepared.reset();
        require(s.docReleases == 1 && s.workerReleases == 1, "exact owner releases");
    }
    for (int mode = 0; mode < 4; ++mode) {
        Scenario s; active = &s; auto loaded = bridge();
        s.docEmpty = mode == 0; s.docAlias = mode == 1; s.workerEmpty = mode == 2; s.wrongWorker = mode == 3;
        auto p = prepare(*loaded.bridge, s);
        require(!p.prepared && !p.unknown && s.requests == 0, "lease/equality refusal before request");
        require(s.docReleases == (mode == 0 ? 0 : 1), "alias or normal document control released");
        require(s.workerReleases == (mode == 3 ? 1 : 0), "wrong matching worker control released");
    }
    for (int mode = 0; mode < 2; ++mode) {
        Scenario s; active = &s; auto loaded = bridge(); s.resolveThrow = mode == 0; s.acquireThrow = mode == 1;
        auto p = prepare(*loaded.bridge, s);
        require(!p.prepared && p.unknown && s.requests == 0, "throw is unknown, no submission");
        require(s.docReleases == mode && s.workerReleases == 0, "only fully returned owner released");
        const int oldResolves = s.resolves;
        require(!prepare(*loaded.bridge, s).prepared && s.resolves == oldResolves, "preparation unknown cannot retry adapter");
    }
    {
        Scenario s; active = &s; auto loaded = bridge(); int permits = 0;
        auto p = prepare(*loaded.bridge, s, [&] { return ++permits < 2; });
        require(!p.prepared && s.resolves == 1 && s.acquires == 0 && s.docReleases == 1, "post-resolver cancellation");
    }
    {
        Scenario s; active = &s; auto loaded = bridge(); bool nestedRefused = false;
        auto p = prepare(*loaded.bridge, s, [&] {
            nestedRefused = !prepare(*loaded.bridge, s).prepared; return true;
        });
        require(!p.prepared && nestedRefused && s.resolves == 0, "prepare permit reentry poisons outer operation");
    }
    {
        Scenario s; active = &s; auto loaded = bridge(); auto p = prepare(*loaded.bridge, s);
        auto submitted = p.prepared->submit([] {}, [] { return false; });
        require(!submitted.attempted && !submitted.unknown && s.requests == 0, "pre-request cancel");
        require(!p.prepared->submit([] {}, [] { return true; }).attempted, "canceled preparation cannot retry");
    }
    for (int mode = 0; mode < 2; ++mode) {
        Scenario s; active = &s; auto loaded = bridge(); auto p = prepare(*loaded.bridge, s);
        s.requestThrow = mode == 0; s.jobId = mode == 1 ? 0 : s.jobId;
        auto submitted = p.prepared->submit([] {}, [] { return true; });
        require(submitted.attempted && !submitted.accepted && submitted.unknown, "uncertain native request consumes attempt");
        require(!p.prepared->submit([] {}, [] { return true; }).attempted && s.requests == 1, "uncertain never retry");
    }
    {
        Scenario s; active = &s; auto loaded = bridge(); auto p = prepare(*loaded.bridge, s);
        bool oldEpoch = true; int permits = 0;
        s.requestHook = [&] { oldEpoch = false; };
        const auto submitted = p.prepared->submit([] {}, [&] { ++permits; return oldEpoch; });
        require(submitted.accepted && submitted.jobId == s.jobId && permits == 1,
                "intentional enqueue invalidation never rereads old source epoch");
    }
    {
        Scenario s; active = &s; auto loaded = bridge(); auto p = prepare(*loaded.bridge, s);
        Prepared *borrowed = p.prepared.get();
        const auto submitted = borrowed->submit([] {}, [&] { p.prepared.reset(); return true; });
        require(!submitted.attempted && s.docReleases == 1 && s.workerReleases == 1,
                "permit deletion retains state until safe unwind");
    }
    {
        Scenario s; active = &s; auto loaded = bridge(); auto p = prepare(*loaded.bridge, s);
        bool nested = false;
        s.requestHook = [&] { nested = !p.prepared->submit([] {}, [] { return true; }).attempted; };
        const auto submitted = p.prepared->submit([] {}, [] { return true; });
        require(nested && submitted.attempted && submitted.unknown && !submitted.accepted && s.requests == 1,
                "native reentry never causes second call or accepted receipt");
    }
    {
        Scenario s; active = &s; auto loaded = bridge(); auto p = prepare(*loaded.bridge, s);
        bool closed = true; QObject *worker = &s.worker;
        std::thread wrong([&] { closed = p.prepared->close(); worker = p.prepared->worker(); }); wrong.join();
        require(!closed && !worker && s.docReleases == 0 && s.workerReleases == 0, "wrong-thread close does not invoke teardown");
        require(p.prepared->close() && s.docReleases == 1 && s.workerReleases == 1, "later GUI close releases");
    }
    {
        Scenario s; active = &s; auto loaded = bridge(); auto p = prepare(*loaded.bridge, s);
        s.releaseHook = [] { throw std::runtime_error("release outcome unknown"); };
        require(!p.prepared->close() && !p.prepared->close(), "release uncertainty remains terminal");
        require(s.docReleases == 1 && s.workerReleases == 1, "uncertain release is never repeated");
    }
    {
        Scenario s; active = &s; auto loaded = bridge(); auto p = prepare(*loaded.bridge, s);
        s.requestHook = [&] { require(!p.prepared->close(), "nested close defers teardown"); };
        const auto submitted = p.prepared->submit([] {}, [] { return true; });
        require(submitted.unknown && s.docReleases == 0 && s.workerReleases == 0, "no release on request stack");
        require(p.prepared->close() && s.docReleases == 1 && s.workerReleases == 1, "release after request stack unwinds");
    }
    {
        Scenario s; active = &s; auto loaded = bridge();
        auto p = loaded.bridge->prepare(&s.controller, QVariant(QStringLiteral("not native ID")), &s.worker, [] { return true; });
        require(!p.prepared && s.resolves == 0, "wrong metatype never reinterpreted");
        QObject foreign;
        loaded = bridge();
        p = loaded.bridge->prepare(&s.controller, QVariant::fromValue(NativeId{}), &foreign, [] { return true; });
        require(!p.prepared && s.resolves == 0, "wrong native worker ancestry");
    }
    {
        Scenario s; active = &s; auto loaded = bridge();
        const QVariant wrong = QVariant::fromValue(WrongId{});
        const int beforeControlCopy = WrongId::copies;
        const QVariant controlCopy = wrong;
        require(WrongId::copies > beforeControlCopy && controlCopy.metaType() == wrong.metaType(),
                "owned wrong-ID positive control exercises a QVariant copy hook");
        const int copies = WrongId::copies;
        const auto p = loaded.bridge->prepare(&s.controller, wrong, &s.worker, [] { return true; });
        require(!p.prepared && WrongId::copies == copies && s.resolves == 0,
                "wrong same-size metatype rejected before its copy hook");
    }
    qInfo() << "native worker bridge owned fixture PASS" << checks << "checks; no target execution";
    return 0;
}
#include "reader_native_worker_bridge_fixture.moc"
