#pragma once
#include <QGuiApplication>
#include <QPointer>
#include <QQmlEngine>
#include <QQuickWindow>
#include <QQuickItem>
#include <QThread>
#include <QImage>
#include <QIODevice>
#include <QCryptographicHash>
#include <cmath>
#include <functional>
#include <memory>
#include <unistd.h>

namespace visual_capture {
constexpr qint64 PngCap = 1048576, StorageCap = 10513152, PixelCap = 2628288;
class Sink final : public QIODevice {
public:
    explicit Sink(int fd) : fd_(fd) { open(QIODevice::WriteOnly | QIODevice::Unbuffered); }
    bool isSequential() const override { return true; }
    bool valid() const { return good_ && bytes_ > 0; }
    qint64 count() const { return bytes_; }
    QByteArray digest() const { return hash_.result().toHex(); }
protected:
    qint64 readData(char *, qint64) override { return -1; }
    qint64 writeData(const char *data, qint64 n) override {
        if (!good_ || n < 0 || n > PngCap - bytes_) { good_ = false; return -1; }
        if (!n) return 0;
        const auto written = ::write(fd_, data, size_t(n));
        if (written != n) { good_ = false; return -1; }
        hash_.addData(QByteArrayView(data, n)); bytes_ += n;
        return n;
    }
private:
    int fd_;
    qint64 bytes_ = 0;
    bool good_ = true;
    QCryptographicHash hash_{QCryptographicHash::Sha256};
};
struct Result {
    const char *stage = "context-refused";
    bool applicationThread = false, currentWindow = false;
    int width = 0, height = 0;
    qint64 storage = 0, encoded = 0;
    double dpr = 0;
    QByteArray sha;
    std::function<bool()> context;
};
enum class SamplePhase { BeforeGrab, AfterGrab };
using WindowObserver = std::function<bool(SamplePhase, QPointer<QQuickWindow>, QPointer<QQuickItem>,
                                         const std::function<bool()> &)>;
namespace detail {
inline Result captureImpl(const std::function<bool()> &inputPermit, Sink &sink,
                          const WindowObserver &inputObserver, bool observed
#ifdef VISUAL_CAPTURE_TESTING
    , const std::function<QImage(QQuickWindow *)> &grab = {}
#endif
) {
    Result r;
    const auto refused = observed ? std::make_shared<bool>(false) : std::shared_ptr<bool>{};
    const std::function<bool()> permit = observed ? std::function<bool()>([inputPermit, refused] {
        if (*refused) return false;
        bool ok = false;
        try { ok = inputPermit && inputPermit(); }
        catch (...) { *refused = true; return false; }
        if (!ok) *refused = true;
        return ok;
    }) : inputPermit;
    const auto observer = inputObserver;
    if (observed && (!inputPermit || !observer)) { r.stage = "reader-observation-refused"; return r; }
    QPointer<QGuiApplication> app = qobject_cast<QGuiApplication *>(QCoreApplication::instance());
    if (!app || app->thread() != QThread::currentThread() || !permit()) return r;
    r.applicationThread = true;
    const auto raw = QGuiApplication::allWindows();
    if (raw.size() > 16) { r.stage = "window-cap"; return r; }
    QList<QPointer<QWindow>> windows;
    for (auto *w : raw) windows.append(w);
    QPointer<QQmlEngine> engine;
    for (const auto &w : windows) {
        if (!permit() || !w || w->thread() != app->thread()) return r;
        QPointer<QQmlEngine> e = qmlEngine(w);
        if (!permit() || !w) return r;
        if (e && engine && e != engine) { r.stage = "engine-ambiguous"; return r; }
        if (e) engine = e;
    }
    QPointer<QQuickWindow> window = qobject_cast<QQuickWindow *>(QGuiApplication::focusWindow());
    QPointer<QQuickItem> root = window ? window->contentItem() : nullptr;
    r.context = [app, engine, window, root, permit, observed, refused] {
        const auto retained = [&] {
            return app && !QCoreApplication::closingDown() && engine && window && root &&
                QThread::currentThread() == app->thread() && engine->thread() == app->thread() &&
                window->thread() == app->thread() && root->thread() == app->thread() &&
                QGuiApplication::focusWindow() == window && window->isVisible() && window->isActive() &&
                qmlEngine(window) == engine && window->contentItem() == root;
        };
        if (observed) {
            if (*refused) return false;
            if (!retained() || !permit() || !retained()) { *refused = true; return false; }
            return true;
        }
        if (!permit() || !app || QCoreApplication::closingDown() || !engine || !window || !root) return false;
        const bool ok = QThread::currentThread() == app->thread() && engine->thread() == app->thread() &&
            window->thread() == app->thread() && QGuiApplication::focusWindow() == window &&
            window->isVisible() && window->isActive() && qmlEngine(window) == engine &&
            window->contentItem() == root;
        return permit() && app && engine && window && root && ok;
    };
    if (!r.context()) return r;
    r.currentWindow = true;
    const auto sample = [&](SamplePhase phase) {
        if (!r.context()) return false;
        bool completed = false;
        try { completed = observer(phase, window, root, r.context); }
        catch (...) { *refused = true; return false; }
        if (!completed || !r.context()) { *refused = true; return false; }
        return true;
    };
    if (observed && !sample(SamplePhase::BeforeGrab)) { r.stage = "reader-observation-refused"; return r; }
    QImage image;
#ifdef VISUAL_CAPTURE_TESTING
    image = grab ? grab(window) : window->grabWindow();
#else
    image = window->grabWindow();
#endif
    if (!r.context()) { r.stage = "context-lost"; return r; }
    if (image.isNull()) { r.stage = "image-null"; return r; }
    const int width = image.width(), height = image.height();
    const qint64 storage = image.sizeInBytes();
    const double dpr = image.devicePixelRatio();
    if (width <= 0 || height <= 0 || width > 1872 || height > 1872 ||
        qint64(width) * height > PixelCap || storage <= 0 || storage > StorageCap ||
        !std::isfinite(dpr) || dpr <= 0) { r.stage = "image-quota"; return r; }
    r.width = width; r.height = height; r.storage = storage; r.dpr = dpr;
    if (observed && !sample(SamplePhase::AfterGrab)) { r.stage = "reader-observation-refused"; return r; }
    if (!r.context()) { r.stage = "context-lost"; return r; }
    const bool encoded = image.save(&sink, "PNG");
    if (!r.context()) { r.stage = "context-lost"; return r; }
    if (!encoded || !sink.valid()) { r.stage = "encode-refused"; return r; }
    r.encoded = sink.count(); r.sha = sink.digest();
    r.stage = "visual-image-observed";
    return r;
}
} // namespace detail
inline Result capture(const std::function<bool()> &permit, Sink &sink
#ifdef VISUAL_CAPTURE_TESTING
    , const std::function<QImage(QQuickWindow *)> &grab = {}
#endif
) {
    return detail::captureImpl(permit, sink, {}, false
#ifdef VISUAL_CAPTURE_TESTING
        , grab
#endif
    );
}
inline Result captureObserved(const std::function<bool()> &permit, Sink &sink, const WindowObserver &observer
#ifdef VISUAL_CAPTURE_TESTING
    , const std::function<QImage(QQuickWindow *)> &grab = {}
#endif
) {
    return detail::captureImpl(permit, sink, observer, true
#ifdef VISUAL_CAPTURE_TESTING
        , grab
#endif
    );
}
}
