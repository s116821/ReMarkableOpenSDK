#pragma once
// Synchronous research composition. No file publication or native authority.
#include "reader_selected_view_probe.h"
#include "visual_capture_core.h"
#include <atomic>
#include <chrono>
#include <memory>

#if defined(READER_IDENTITY_CAPTURE_TESTING) && !defined(VISUAL_CAPTURE_TESTING)
#error Identity capture fixture hooks require the visual capture fixture boundary.
#endif

namespace reader_identity_capture {
using Guard = std::function<bool()>;
using Clock = std::chrono::steady_clock;
struct Expected {
    QString documentId, pageId;
    int pageIndex = -1, pageCount = -1;
    // SDK composition runs its complete-order check within each borrowed source
    // callback. Empty is retained only for the imported identity-only controls.
    std::function<bool(QObject *, QObject *, QObject *, const QVariant &, const reader_selected::Observation &, const Guard &)> sourceCheck = {};
    bool valid() const {
        static const QRegularExpression canonical(QStringLiteral("^[0-9a-f]{8}(-[0-9a-f]{4}){3}-[0-9a-f]{12}$"));
        const auto id = [&](const QString &value) {
            return value.size() == 36 && canonical.match(value).hasMatch() &&
                value != QStringLiteral("00000000-0000-0000-0000-000000000000");
        };
        // Deliberate SDK extension: 16 originals plus one inserted page.
        // Native qualification is separate from this representation bound.
        return id(documentId) && id(pageId) && pageCount >= 1 && pageCount <= 17 &&
            pageIndex >= 0 && pageIndex < pageCount;
    }
};
struct Claim { std::atomic<bool> consumed{false}; };
struct Result {
    bool captured = false, attempted = false, unknown = false;
    QString refusal;
    reader_selected::Observation before, after;
    int width = 0, height = 0;
    qint64 storage = 0, encoded = 0;
    double dpr = 0;
    QByteArray sha;
    static constexpr bool nativeAuthority = false, renderAuthority = false, durableAuthority = false;
};
#ifdef READER_IDENTITY_CAPTURE_TESTING
struct TestBoundary {
    using Sampler = std::function<reader_selected::Observation(QQuickWindow *, const Guard &, int,
        const reader_selected::ValidatedSourceAction &)>;
    Sampler sample;
    std::function<QImage(QQuickWindow *)> grab;
    std::function<Clock::time_point()> now;
};
#endif
namespace detail {
inline thread_local bool running = false, reentered = false;
struct Gate {
    bool entered = false;
    Gate() { if (running) reentered = true; else { running = true; reentered = false; entered = true; } }
    ~Gate() { if (entered) running = false; }
};
inline bool matches(const reader_selected::Observation &s, const Expected &e) {
    return s.observed && s.mapCoherent && s.creationJoinObserved && s.ready && s.refusal.isEmpty() &&
        s.documentId == e.documentId && s.pageId == e.pageId && s.pageIndex == e.pageIndex && s.pageCount == e.pageCount &&
        s.status >= 0 && s.pageStatus >= 0 && !s.loading && !s.pageError && !s.documentError &&
        !s.awaitingTiles && !s.pendingTiles && !s.working &&
        !s.nativeAuthority && !s.renderAuthority && !s.durableAuthority;
}
inline bool same(const reader_selected::Observation &a, const reader_selected::Observation &b) {
    return a.refusal == b.refusal && a.documentId == b.documentId && a.pageId == b.pageId &&
        a.pageIndex == b.pageIndex && a.pageCount == b.pageCount && a.status == b.status && a.pageStatus == b.pageStatus &&
        a.observed == b.observed && a.mapCoherent == b.mapCoherent && a.creationJoinObserved == b.creationJoinObserved &&
        a.ready == b.ready && a.loading == b.loading && a.pageError == b.pageError && a.documentError == b.documentError &&
        a.awaitingTiles == b.awaitingTiles && a.pendingTiles == b.pendingTiles && a.working == b.working &&
        a.nativeAuthority == b.nativeAuthority && a.renderAuthority == b.renderAuthority && a.durableAuthority == b.durableAuthority;
}
}

// The caller owns a fresh Sink/FD for this synchronous call. No borrowed guard,
// QVariant, capture context or native object address survives in Result.
inline Result run(QQuickWindow *inputWindow, const Expected &inputExpected, visual_capture::Sink &inputSink,
                  const std::shared_ptr<Claim> &inputClaim, const Guard &inputPermit, int budgetMs = 20000
#ifdef READER_IDENTITY_CAPTURE_TESTING
                  , const TestBoundary &inputTest = {}
#endif
) {
    Result result;
    detail::Gate gate;
    const auto refuse = [&](const char *why) {
        result.captured = false; result.unknown = result.attempted;
        result.refusal = QString::fromLatin1(why); return result;
    };
    if (!gate.entered) return refuse("reentered");
    const QPointer<QGuiApplication> app(qobject_cast<QGuiApplication *>(QCoreApplication::instance()));
    const QPointer<QQuickWindow> window(inputWindow);
    const QPointer<visual_capture::Sink> sink(&inputSink);
    if (!app || QCoreApplication::closingDown() || QThread::currentThread() != app->thread() ||
        !window || window->thread() != app->thread() || sink->thread() != app->thread()) return refuse("gui");
    const QPointer<QQuickItem> root(window->contentItem());
    const QPointer<QQmlEngine> engine(qmlEngine(window));
    try {
        const Expected expected = inputExpected;
        const Guard permission = inputPermit;
        const auto claim = inputClaim;
#ifdef READER_IDENTITY_CAPTURE_TESTING
        const TestBoundary test = inputTest;
#endif
        const auto now = [&] {
#ifdef READER_IDENTITY_CAPTURE_TESTING
            if (test.now) return test.now();
#endif
            return Clock::now();
        };
        if (!expected.valid() || !permission || !claim || budgetMs < 1 || budgetMs > 20000) return refuse("request");
        const auto deadline = now() + std::chrono::milliseconds(budgetMs);
        bool sticky = false;
        const auto alive = [&] {
            return !detail::reentered && app && window && root && engine && sink && !QCoreApplication::closingDown() &&
                QThread::currentThread() == app->thread() && window->thread() == app->thread() &&
                root->thread() == app->thread() && engine->thread() == app->thread() && sink->thread() == app->thread() &&
                QGuiApplication::focusWindow() == window && window->isVisible() && window->isActive() &&
                window->contentItem() == root && qmlEngine(window) == engine;
        };
        const auto allowed = [&] {
            if (sticky) return false;
            try {
                if (now() >= deadline || !alive() || !permission() || !alive() || now() >= deadline || !alive()) sticky = true;
            } catch (...) { sticky = true; }
            return !sticky;
        };
        const auto remaining = [&] {
            return qMax(0, qMin(budgetMs, int(std::chrono::duration_cast<std::chrono::milliseconds>(deadline - now()).count())));
        };
        const auto sample = [&](const reader_selected::ValidatedSourceAction &action) {
            const int budget = remaining();
            if (!allowed() || budget < 1) return reader_selected::Observation{};
#ifdef READER_IDENTITY_CAPTURE_TESTING
            if (test.sample) return test.sample(window, allowed, budget, action);
#endif
            return reader_selected::observeCreationJoin(window, allowed, budget, {}, action);
        };
        if (!allowed() || sink->count() != 0 || claim->consumed.load(std::memory_order_acquire)) return refuse("before-sample");
        QPointer<QObject> reader, document, controller;
        const auto ownersAlive = [&] {
            if (!app || !reader || !document || !controller || !engine ||
                reader->thread() != app->thread() || document->thread() != app->thread() || controller->thread() != app->thread()) return false;
            for (const auto &owner : {reader, document, controller}) {
                auto *associated = qmlEngine(owner);
                if (!owner || (associated && associated != engine)) return false;
            }
            return true;
        };
        reader_selected::Observation firstAction, secondAction;
        unsigned firstCalls = 0, secondCalls = 0;
        bool firstOkay = false, secondOkay = false;
        result.before = sample([&](QObject *r, QObject *d, QObject *c, const QVariant &actualId,
                                  const reader_selected::Observation &observation, const Guard &borrowed) {
            ++firstCalls;
            if (firstCalls != 1) { sticky = true; return; }
            reader = r; document = d; controller = c;
            firstAction = observation;
            const auto live = [&] {
                if (!allowed() || !ownersAlive() || !borrowed) return false;
                bool valid = false;
                try { valid = borrowed(); } catch (...) { sticky = true; return false; }
                if (!valid) sticky = true;
                return valid && allowed() && ownersAlive();
            };
            if (!detail::matches(observation, expected) || !live()) return;
            if (expected.sourceCheck && (!expected.sourceCheck(r, d, c, actualId, observation, live) || !live())) return;
            bool unused = false;
            if (!claim->consumed.compare_exchange_strong(unused, true, std::memory_order_acq_rel)) { sticky = true; return; }
            result.attempted = true;
            unsigned before = 0, after = 0;
            const auto observer = [&](visual_capture::SamplePhase phase, QPointer<QQuickWindow> capturedWindow,
                                      QPointer<QQuickItem> capturedRoot, const Guard &capturePermit) {
                if (!live() || !capturePermit || capturedWindow != window || capturedRoot != root) { sticky = true; return false; }
                if (phase == visual_capture::SamplePhase::BeforeGrab) {
                    if (++before != 1 || after != 0) sticky = true;
                } else if (phase == visual_capture::SamplePhase::AfterGrab) {
                    if (++after != 1 || before != 1) sticky = true;
                } else sticky = true;
                return !sticky && capturePermit() && live();
            };
            auto captured = visual_capture::captureObserved(live, *sink, observer
#ifdef VISUAL_CAPTURE_TESTING
#ifdef READER_IDENTITY_CAPTURE_TESTING
                , test.grab
#endif
#endif
            );
            const auto clear = qScopeGuard([&] { captured.context = {}; });
            if (!captured.context || !captured.context() || !live() || before != 1 || after != 1 ||
                qstrcmp(captured.stage, "visual-image-observed") != 0) return;
            result.width = captured.width; result.height = captured.height;
            result.storage = captured.storage; result.encoded = captured.encoded;
            result.dpr = captured.dpr; result.sha = captured.sha;
            firstOkay = captured.context() && live();
        });
        // The first Sample and all captured borrowed functions have now unwound.
        if (!allowed() || !ownersAlive() || firstCalls != 1 || !firstOkay || !detail::matches(result.before, expected) ||
            !detail::same(result.before, firstAction)) return refuse("capture-or-first-join");
        result.after = sample([&](QObject *r, QObject *d, QObject *c, const QVariant &actualId,
                                 const reader_selected::Observation &observation, const Guard &borrowed) {
            ++secondCalls;
            if (secondCalls != 1) { sticky = true; return; }
            secondAction = observation;
            if (!allowed() || !ownersAlive() || reader != r || document != d || controller != c ||
                !detail::matches(observation, expected) || !borrowed) return;
            bool valid = false;
            try { valid = borrowed(); } catch (...) { sticky = true; return; }
            if (!valid) sticky = true;
            if (!valid || !allowed() || !ownersAlive() || reader != r || document != d || controller != c) return;
            if (valid && expected.sourceCheck &&
                (!expected.sourceCheck(reader, document, controller, actualId, observation, borrowed) || !allowed() || !ownersAlive())) return;
            secondOkay = valid && allowed() && ownersAlive() && reader == r && document == d && controller == c;
        });
        if (!allowed() || !ownersAlive() || secondCalls != 1 || !secondOkay || !detail::matches(result.after, expected) ||
            !detail::same(result.after, secondAction)) return refuse("fresh-join");
        result.captured = true;
        return result;
    } catch (...) { return refuse("exception"); }
}
} // namespace reader_identity_capture
