#define COMPOSER_OBSERVER_TESTING
#include "composer_document_observer.h"
#include <QGuiApplication>
#include <QTextCursor>
#include <cassert>
#include <future>
#include <functional>
#include <iostream>
using namespace remarkable::composer_diagnostic;
using namespace std::chrono_literals;
// Check the discovered value ABI only. No native entry is called.
class Scene;
using SceneCallback = std::function<bool(Scene *)>;
#if defined(__arm__) && defined(__ARM_PCS_VFP)
static_assert(sizeof(SceneCallback) == 16 && alignof(SceneCallback) == 4);
static_assert(sizeof(QString) == 12);
#endif
static int lastRevision(const Snapshot &s) {
    assert(s.available);
    for (std::size_t i = s.count; i; --i)
        if (s.records[i-1].revisionPresent) return s.records[i-1].revision;
    assert(false); return 0;
}
static void unicodeAndUndo() {
    QTextDocument doc;
    const auto original = QString::fromUtf16(u"PREFIX\nNBSP:\u00a0 Cafe\u0301 \U0001f642 \u4e2d \u0628\nHISTORY");
    doc.setPlainText(original);
    doc.setModified(false);
    const auto raw = doc.toRawText();
    const auto revision = doc.revision();
    const bool undo = doc.isUndoAvailable();
    auto observer = Observer::attach(doc);
    assert(observer);
    assert(doc.toRawText() == raw && doc.revision() == revision);
    assert(!doc.isModified() && doc.isUndoAvailable() == undo);
    assert(lastRevision(observer->snapshot()) == revision);
    QTextCursor cursor(&doc);
    cursor.movePosition(QTextCursor::End);
    cursor.beginEditBlock(); cursor.insertText(QStringLiteral("X")); cursor.endEditBlock();
    const auto edited = doc.toRawText();
    const auto editedRevision = doc.revision();
    assert(edited != raw && editedRevision != revision);
    assert(lastRevision(observer->snapshot()) == editedRevision);
    doc.undo();
    const auto undoneRevision = doc.revision();
    assert(doc.toRawText() == raw && undoneRevision != revision);
    assert(lastRevision(observer->snapshot()) == undoneRevision);
    doc.redo();
    assert(doc.toRawText() == edited && doc.revision() != editedRevision);
    auto result = observer->snapshot();
    assert(lastRevision(result) == doc.revision());
    for (std::size_t i = 0; i < result.count; ++i) {
        assert(result.records[i].revisionPresent);
        assert(result.records[i].recorderDepth == 1);
        assert(result.records[i].concurrentEntries == 1);
    }
    observer->stop();
    const auto closed = observer->snapshot();
    assert(closed.end == End::Canceled);
    doc.undo();
    const auto after = observer->snapshot();
    assert(after.count == closed.count && after.end == End::Canceled);
}
static void limitsAndExpiry() {
    QTextDocument doc;
    assert(!Observer::attach(doc, 0ms));
    assert(!Observer::attach(doc, 120001ms));
    assert(!Observer::attach(doc, 1s, 257));
    assert(!Observer::attach(doc, 1s, 0));
    auto bounded = Observer::attach(doc, 1s, 2);
    assert(bounded);
    doc.contentsChanged(); // Explicit Qt notification control; no text mutation.
    assert(bounded->snapshot().end == End::Capacity);
    for (int i = 0; i < 1000; ++i) doc.contentsChanged();
    assert(bounded->snapshot().count == 2);
    bounded->poll();
    auto expired = Observer::attach(doc, 1ms);
    std::this_thread::sleep_for(3ms);
    expired->poll();
    auto final = expired->snapshot();
    assert(final.end == End::Deadline && final.count == 1);
    doc.contentsChanged();
    assert(expired->snapshot().count == 1);
}
static void lifetime() {
    auto doc = std::make_unique<QTextDocument>();
    auto observer = Observer::attach(*doc);
    doc.reset();
    auto result = observer->snapshot();
    assert(result.end == End::Destroyed);
    assert(result.records[result.count-1].event == Event::Destroyed);
    assert(!result.records[result.count-1].revisionPresent);
    observer->stop();
    assert(observer->snapshot().end == End::Destroyed);

    QTextDocument another;
    std::unique_ptr<Observer> retiring;
    auto first = QObject::connect(&another, &QTextDocument::contentsChanged, &another,
        [&retiring] { retiring.reset(); }, Qt::DirectConnection);
    retiring = Observer::attach(another);
    another.contentsChanged(); // Earlier slot retires it within Qt activation.
    assert(!retiring);
    QObject::disconnect(first);
}
static void wrongThread() {
    QTextDocument doc;
    auto observer = Observer::attach(doc);
    std::thread wrong([&] {
        assert(!Observer::attach(doc));
        doc.contentsChanged(); // Signal delivery only; no foreign-thread text edit.
    });
    wrong.join();
    auto result = observer->snapshot();
    assert(result.end == End::WrongThread);
    assert(result.count == 2 && !result.records[1].revisionPresent);
    assert(result.records[1].emitterThread != result.records[0].emitterThread);
    struct Subclass : QTextDocument {};
    Subclass subclass;
    assert(!Observer::attach(subclass));
}
static void coalescingAndReentry() {
    QTextDocument doc;
    auto observer = Observer::attach(doc);
    const auto first = observer->snapshot();
    doc.blockSignals(true);
    doc.setPlainText(QStringLiteral("changed with signals blocked"));
    doc.blockSignals(false);
    assert(observer->snapshot().count == first.count);
    assert(doc.revision() != lastRevision(first)); // Silence is not currentness.
    bool nesting = false;
    const auto nested = QObject::connect(&doc, &QTextDocument::contentsChanged, &doc, [&] {
        if (!nesting) { nesting = true; doc.contentsChanged(); nesting = false; }
    }, Qt::DirectConnection);
    doc.contentsChanged();
    auto result = observer->snapshot();
    assert(result.count == first.count + 2);
    // Our first callback returned before another slot nested the signal.
    // This depth must not be mislabeled as whole native signal/worker nesting.
    assert(result.records[result.count-1].recorderDepth == 1);
    QObject::disconnect(nested);
}
static void cancelWhileEmitting() {
    std::promise<std::unique_ptr<Observer>> installed;
    auto result = installed.get_future();
    std::atomic<bool> go{false}, done{false};
    std::thread emitter([&] {
        QTextDocument doc;
        installed.set_value(Observer::attach(doc));
        while (!go.load(std::memory_order_acquire)) std::this_thread::yield();
        for (int i = 0; i < 20000; ++i) doc.contentsChanged();
        while (!done.load(std::memory_order_acquire)) std::this_thread::yield();
        for (int i = 0; i < 100; ++i) doc.contentsChanged();
    });
    auto observer = result.get();
    assert(observer);
    go.store(true, std::memory_order_release);
    observer->stop();
    observer.reset(); // Qt functors, not this handle, own any entered state.
    done.store(true, std::memory_order_release);
    emitter.join();
}

namespace remarkable::composer_diagnostic::testing {
struct Access {
    static void hooks(Observer &observer, void *context,
                      void (*publish)(void *), void (*contended)(void *)) {
        observer.state_->hookContext = context;
        observer.state_->beforePublish = publish;
        observer.state_->closeContended = contended;
    }
    static End terminal(const Observer &observer) {
        return observer.state_->end.load(std::memory_order_acquire);
    }
};
}
static void closeVersusAdmittedPublication() {
    struct Phases {
        std::promise<void> atPublish, closeBlocked, releasePublish, start, afterStop;
        std::shared_future<void> release = releasePublish.get_future().share();
    } phases;
    auto atPublish = phases.atPublish.get_future();
    auto closeBlocked = phases.closeBlocked.get_future();
    auto start = phases.start.get_future();
    auto afterStop = phases.afterStop.get_future();
    std::promise<std::unique_ptr<Observer>> installed;
    auto ready = installed.get_future();
    std::thread emitter([&] {
        QTextDocument doc;
        auto observer = Observer::attach(doc);
        testing::Access::hooks(*observer, &phases,
            [](void *p) {
                auto &phase = *static_cast<Phases *>(p);
                phase.atPublish.set_value();
                phase.release.wait(); // Paused after final gate, before append.
            },
            [](void *p) { static_cast<Phases *>(p)->closeBlocked.set_value(); });
        installed.set_value(std::move(observer));
        start.wait();
        doc.contentsChanged();
        afterStop.wait();
        for (int i = 0; i < 100; ++i) doc.contentsChanged();
    });
    auto observer = ready.get();
    phases.start.set_value();
    assert(atPublish.wait_for(5s) == std::future_status::ready);
    std::atomic<bool> stopReturned{false};
    std::thread closer([&] { observer->stop(); stopReturned.store(true); });
    // The real try_lock failed while publication is paused. No timing guess:
    // terminal closure cannot happen until that admitted publication releases.
    assert(closeBlocked.wait_for(5s) == std::future_status::ready);
    assert(!stopReturned.load());
    assert(testing::Access::terminal(*observer) == End::Open);
    phases.releasePublish.set_value();
    closer.join();
    const auto frozen = observer->snapshot();
    assert(frozen.available && frozen.end == End::Canceled && frozen.count == 2);
    phases.afterStop.set_value();
    emitter.join();
    const auto final = observer->snapshot();
    assert(final.available && final.end == End::Canceled && final.count == frozen.count);
    assert(final.records[1].revisionPresent);
}

int main(int argc, char **argv) {
    QGuiApplication app(argc, argv);
    unicodeAndUndo();
    limitsAndExpiry();
    lifetime();
    wrongThread();
    coalescingAndReentry();
    cancelWhileEmitting();
    closeVersusAdmittedPublication();
    std::cout << "composer document observer: 7 Qt scenarios passed; Qt "
              << qVersion() << "; native runtime unqualified\n";
}
