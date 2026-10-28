#pragma once
// Owned historical PNG evidence. No operational PageObservation or write lease.
#include "reader_identity_capture.h"
#include "order_observation.h"

namespace remarkable::rm2_3_28_0_172 {
namespace source_capture_detail {
inline thread_local bool running = false, reentered = false;
struct Gate {
    bool entered = false;
    Gate() { if (running) reentered = true; else { running = true; reentered = false; entered = true; } }
    ~Gate() { if (entered) running = false; }
};
}
struct SourceCapture {
    reader_identity_capture::Result identity;
    resident::Facts before, after;
    QByteArray png;
    bool captured() const { return identity.captured && !png.isEmpty(); }
};
inline bool sameFacts(const resident::Facts &a, const resident::Facts &b) {
    return a.document == b.document && a.page == b.page && a.order == b.order;
}
inline resident::Frame captureReply(SourceCapture captured) {
    resident::Frame response;response.kind=3;response.rejection=3;
    if(!captured.captured())return response;
    response.facts=std::move(captured.before);
    auto &image=response.capture;image.after=std::move(captured.after);
    image.width=quint32(captured.identity.width);image.height=quint32(captured.identity.height);
    std::memcpy(&image.dprBits,&captured.identity.dpr,sizeof image.dprBits);
    image.sha256=QByteArray::fromHex(captured.identity.sha);image.png=std::move(captured.png);
    if(resident::validCapture(response.facts,image))response.kind=7;
    else response.capture={};
    return response;
}

// All intermediate output and borrowed-source callbacks are confined to this
// synchronous call. Only a completely admitted result can expose PNG bytes.
inline SourceCapture captureSource(QQuickWindow *inputWindow, const resident::Facts &inputExpected,
                                   const reader_identity_capture::Guard &inputPermit, int budgetMs
#ifdef READER_IDENTITY_CAPTURE_TESTING
                                   , const reader_identity_capture::TestBoundary &test = {}
#endif
) {
    using namespace reader_identity_capture;
    SourceCapture result;
    source_capture_detail::Gate gate;
    const auto refuse = [&](const char *why) {
        result.identity.captured = false;
        result.identity.unknown = result.identity.attempted;
        result.identity.refusal = QString::fromLatin1(why);
        result.png.clear();
        return result;
    };
    if (!gate.entered) return refuse("source-reentered");
    try {
        if (budgetMs < 1 || budgetMs > 20000) return refuse("source-budget");
        const auto deadline = Clock::now() + std::chrono::milliseconds(budgetMs);
        const auto expected = inputExpected;
        const auto permit = inputPermit;
        const QPointer<QGuiApplication> app(qobject_cast<QGuiApplication *>(QCoreApplication::instance()));
        if (!app || QCoreApplication::closingDown() || QThread::currentThread() != app->thread())
            return refuse("source-gui");
        const QPointer<QQuickWindow> window(inputWindow);
        if (!window || window->thread() != app->thread()) return refuse("source-window");
        const QPointer<QQmlEngine> engine(window ? qmlEngine(window) : nullptr);
        const QPointer<QQuickItem> root(window->contentItem());
        QPointer<QObject> readerOwner, documentOwner, controllerOwner;
        bool ownersPinned = false;
        if (!resident::validFacts(expected) || expected.order.size() > 17 || !permit || !window || !engine)
            return refuse("source-request");
        bool lost = false;
        const Guard current = [&] {
            if (lost) return false;
            const auto context = [&] {
                if (source_capture_detail::reentered || !app || !window || !engine || !root ||
                    QCoreApplication::closingDown()) return false;
                if (ownersPinned) {
                    for (const auto &owner : {readerOwner, documentOwner, controllerOwner}) {
                        if (!owner || owner->thread() != app->thread()) return false;
                        const auto *associated = qmlEngine(owner);
                        if (!owner || (associated && associated != engine)) return false;
                    }
                }
                return
                    QThread::currentThread() == app->thread() && window->thread() == app->thread() &&
                    root->thread() == app->thread() && window->contentItem() == root &&
                    engine->thread() == app->thread() && qmlEngine(window) == engine &&
                    QGuiApplication::focusWindow() == window && window->isVisible() && window->isActive();
            };
            if (Clock::now() >= deadline || !context() || !permit() ||
                !context() || Clock::now() >= deadline) lost = true;
            return !lost;
        };
        const auto at = std::find(expected.order.begin(), expected.order.end(), expected.page);
        Expected identity{QUuid::fromRfc4122(expected.document).toString(QUuid::WithoutBraces),
                          QUuid::fromRfc4122(expected.page).toString(QUuid::WithoutBraces),
                          int(at - expected.order.begin()), int(expected.order.size())};
        unsigned samples = 0;
        identity.sourceCheck = [&](QObject *reader, QObject *document, QObject *controller, const QVariant &actualId,
                                   const reader_selected::Observation &joined, const Guard &borrowed) {
            // Pin callback owners before current() can invoke caller code.
            const QPointer<QObject> joinedReader(reader), joinedDocument(document), joinedController(controller);
            if (!ownersPinned) {
                readerOwner = joinedReader; documentOwner = joinedDocument; controllerOwner = joinedController;
                ownersPinned = true;
            } else if (readerOwner != joinedReader || documentOwner != joinedDocument || controllerOwner != joinedController) {
                lost = true; return false;
            }
            if (!current() || samples >= 2) { lost = true; return false; }
            const Guard joinedPermit = [&] { return current() && borrowed && borrowed() && current(); };
            auto order = observeOrder(engine, joinedDocument, actualId, joined, joinedPermit);
            if (!order.observed || !sameFacts(order.facts, expected) || !joinedPermit()) {
                lost = true; return false;
            }
            if (samples++ == 0) result.before = std::move(order.facts);
            else result.after = std::move(order.facts);
            return current();
        };
        visual_capture::Sink sink;
        const auto claim = std::make_shared<Claim>();
        result.identity = run(window, identity, sink, claim, current, budgetMs
#ifdef READER_IDENTITY_CAPTURE_TESTING
                              , test
#endif
        );
        // The outer gate and QObject/root pins outlive the inner identity run.
        // No borrowed source callback is retained, including during publication.
        if (!result.identity.captured || samples != 2 || !current()) return refuse("source-capture");
        auto bytes = sink.takeOwned();
        if (bytes.isEmpty() || bytes.size() != result.identity.encoded ||
            QCryptographicHash::hash(bytes, QCryptographicHash::Sha256).toHex() != result.identity.sha || !current())
            return refuse("source-publication");
        result.png = std::move(bytes);
        return result;
    } catch (...) { return refuse("source-exception"); }
}
}
