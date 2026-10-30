#pragma once
#include "visual_capture_core.h"
#include "reader_metadata_census.h"
#include <QFileSystemWatcher>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTimer>
#include <QElapsedTimer>
#include <cerrno>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

namespace visual_capture {
inline bool decimal(const QByteArray &s) {
    if (s.isEmpty() || s.size() > 20 || s[0] == '0') return false;
    for (char c : s) if (c < '0' || c > '9') return false;
    bool ok = false; s.toULongLong(&ok); return ok;
}
inline QByteArray selfIdentity() {
    const int fd = ::open("/proc/self/stat", O_RDONLY | O_NOFOLLOW | O_CLOEXEC);
    if (fd < 0) return {};
    char buffer[4097]; const ssize_t n = ::read(fd, buffer, sizeof buffer);
    const bool closed = ::close(fd) == 0;
    if (!closed || n <= 0 || n > 4096) return {};
    const QByteArray stat(buffer, n);
    const int end = stat.lastIndexOf(')');
    if (end < 0) return {};
    const auto fields = stat.mid(end + 1).simplified().split(' ');
    if (fields.size() <= 19 || !decimal(fields[19])) return {};
    return QByteArray::number(::getpid()) + ' ' + fields[19] + '\n';
}
enum class FrameSignal { AfterFrameEnd, FrameSwapped };
inline QJsonObject readerSnapshot(const reader_metadata::Result &r) {
    return {{"stage", QString::fromLatin1(r.stage)}, {"application_thread", r.applicationThread},
        {"current_window", r.currentWindow}, {"owner_links", r.ownerLinks}, {"metadata", r.metadata},
        {"page_string_domain", r.pageStringDomain}, {"visited", r.visited},
        {"helper_count", r.helpers}, {"container_count", r.containers}};
}
inline QByteArray readerStructureBytes(const QByteArray &nonce, const QByteArray &identity, const QByteArray &sha,
                                      const reader_metadata::Result &before, const reader_metadata::Result &after) {
    const auto id = identity.trimmed().split(' ');
    if (id.size() != 2 || sha.size() != 64) return {};
    const QJsonObject json{{"schema", 1}, {"nonce", QString::fromLatin1(nonce)},
        {"pid", QString::fromLatin1(id[0])}, {"start", QString::fromLatin1(id[1])},
        {"stage", "capture-window-census"}, {"scope", "retained-capture-window-root"},
        {"png_sha256", QString::fromLatin1(sha)}, {"before_grab", readerSnapshot(before)},
        {"after_grab", readerSnapshot(after)}, {"native_authority", false}};
    return QJsonDocument(json).toJson(QJsonDocument::Compact) + '\n';
}
class Entry final : public QObject {
public:
    Entry(QGuiApplication *app, QString path, QByteArray nonce, QObject *parent = nullptr,
          bool passiveFrame = false, bool bindingWitness = false,
          FrameSignal frameSignal = FrameSignal::AfterFrameEnd, bool readerStructure = false)
        : QObject(parent ? parent : app), app_(app), path_(std::move(path)), nonce_(std::move(nonce)),
          frameMode_(passiveFrame), witnessMode_(bindingWitness), frameSignal_(frameSignal), readerMode_(readerStructure) {
        elapsed_.start();
    }
    ~Entry() override { if (root_ >= 0) ::close(root_); }
    bool start() {
        QPointer<Entry> self(this);
        if (started_) return false;
        started_ = true;
        if (witnessMode_ && !frameMode_) return terminal_ = true, false;
        if (readerMode_ && (!frameMode_ || !witnessMode_ || frameSignal_ != FrameSignal::FrameSwapped))
            return terminal_ = true, false;
        if ((frameSignal_ != FrameSignal::AfterFrameEnd && frameSignal_ != FrameSignal::FrameSwapped) ||
            (frameSignal_ == FrameSignal::FrameSwapped && !frameMode_)) return terminal_ = true, false;
        if (!app_ || QThread::currentThread() != app_->thread() || thread() != app_->thread() || nonce_.size() != 32)
            return terminal_ = true, false;
        for (char c : nonce_) if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f')))
            return terminal_ = true, false;
        identity_ = selfIdentity();
        if (identity_.isEmpty()) return terminal_ = true, false;
        root_ = ::open(path_.toLocal8Bit().constData(), O_RDONLY | O_DIRECTORY | O_NOFOLLOW | O_CLOEXEC);
        if (root_ < 0 || ::fstat(root_, &rootStat_) != 0 || !S_ISDIR(rootStat_.st_mode) ||
            (rootStat_.st_mode & 07777) != 0700 || rootStat_.st_uid != ::geteuid() || !trusted())
            return terminal_ = true, false;
        if (!absent("visual-ready") || !absent("visual-result") ||
            !absent(".pending-ready") || !absent(".pending-result") ||
            !absent("visual-image.png") || !absent(".pending-image")) return terminal_ = true, false;
        if (frameMode_ && (!absent("visual-frame-ready") || !absent(".pending-frame-ready")))
            return terminal_ = true, false;
        if (witnessMode_ && (!absent("visual-binding-status") || !absent(".pending-binding-status") ||
            !absent("visual-window-bound") || !absent(".pending-window-bound"))) return terminal_ = true, false;
        if (readerMode_ && (!absent("visual-reader-structure") || !absent(".pending-reader-structure")))
            return terminal_ = true, false;
        connect(&watcher_, &QFileSystemWatcher::directoryChanged, this, [this] { queue(); });
        connect(&timer_, &QTimer::timeout, this, [this] { Result r; r.stage = "setup-expired"; finish(r); });
        connect(app_, &QCoreApplication::aboutToQuit, this, [this] { stop(); });
        if (!watcher_.addPath(path_)) return terminal_ = true, false;
        timer_.setSingleShot(true); timer_.setTimerType(Qt::PreciseTimer);
        const qint64 remaining = 60000 - now();
        if (remaining <= 0) { stop(); return false; }
        timer_.start(int(remaining));
        const bool ready = publish("visual-ready", ".pending-ready", nonce_ + ' ' + identity_, {});
        if (!self) return false;
        if (!ready) { stop(); return false; }
        if (frameMode_) {
            focusConnection_ = connect(app_, &QGuiApplication::focusWindowChanged, this,
                [this](QWindow *) { bindFrameWindow(); }, Qt::QueuedConnection);
            bindFrameWindow();
            if (!self) return false;
        }
        queue();
        return true;
    }
    bool terminal() const { return terminal_; }
#ifdef VISUAL_CAPTURE_TESTING
    void grabForTest(std::function<QImage(QQuickWindow *)> grab) { grab_ = std::move(grab); }
    void expireForTest() { testOffset_ = 60000; queue(); }
    void postLinkForTest(std::function<void()> hook) { postLink_ = std::move(hook); }
    void readerSampleForTest(std::function<void(SamplePhase)> hook) { readerSampleHook_ = std::move(hook); }
    void readerPermitForTest(std::function<void()> hook) { readerPermitHook_ = std::move(hook); }
#endif
private:
    QByteArray readerBytes(const QByteArray &sha) const {
        if (!readerBeforeDone_ || !readerAfterDone_ || sha.size() != 64) return {};
        return readerStructureBytes(nonce_, identity_, sha, readerBefore_, readerAfter_);
    }
    WindowObserver readerObserver() {
        QPointer<Entry> self(this);
        return [self](SamplePhase phase, QPointer<QQuickWindow> window, QPointer<QQuickItem> root,
                      const std::function<bool()> &context) {
            if (!self || !context || !context() || !self) return false;
            if (window != self->frameWindow_ || root != self->frameRoot_) return false;
#ifdef VISUAL_CAPTURE_TESTING
            const auto hook = self->readerSampleHook_;
            if (hook) hook(phase);
            if (!self) return false;
#endif
            const auto sample = reader_metadata::censusWindow(window, root, context);
            if (!self || !context() || !self) return false;
            if (phase == SamplePhase::BeforeGrab) {
                if (self->readerBeforeDone_ || self->readerAfterDone_) return false;
                self->readerBefore_ = sample; self->readerBeforeDone_ = true;
            } else if (phase == SamplePhase::AfterGrab) {
                if (!self->readerBeforeDone_ || self->readerAfterDone_) return false;
                self->readerAfter_ = sample; self->readerAfterDone_ = true;
            } else return false;
            return true;
        };
    }
    bool readReaderExact() const {
        if (!readerPublished_ || readerMarker_.isEmpty() || readerMarker_.size() > 1024 ||
            !absent(".pending-reader-structure")) return false;
        const int fd = ::openat(root_, "visual-reader-structure", O_RDONLY | O_NOFOLLOW | O_CLOEXEC | O_NONBLOCK);
        if (fd < 0) return false;
        struct stat before{}, after{};
        char bytes[1025];
        bool ok = ::fstat(fd, &before) == 0 && S_ISREG(before.st_mode) &&
            before.st_uid == rootStat_.st_uid && (before.st_mode & 07777) == 0600 &&
            before.st_nlink == 1 && before.st_size == readerMarker_.size() &&
            before.st_dev == readerStat_.st_dev && before.st_ino == readerStat_.st_ino &&
            fileBound("visual-reader-structure", readerStat_, 1);
        const ssize_t n = ok ? ::read(fd, bytes, sizeof bytes) : -1;
        ok = ok && n == readerMarker_.size() && QByteArray(bytes, n) == readerMarker_ &&
            ::fstat(fd, &after) == 0 && sameFile(before, after);
        const bool closed = ::close(fd) == 0;
        return ok && closed;
    }
    bool guardedContext(const std::function<bool()> &input) const {
        QPointer<Entry> self(const_cast<Entry *>(this));
        const auto context = input;
        if (!trusted()) return false;
        bool valid = false;
        try { valid = !context || context(); }
        catch (...) { return false; }
        return self && valid && self->trusted();
    }
    bool guardedCommit(const char *name, const char *pending, const struct stat &original,
                       const std::function<bool()> &context) const {
        QPointer<Entry> self(const_cast<Entry *>(this));
        if (!guardedContext(context) || !self || !self->fileBound(pending, original, 1)) return false;
        if (::linkat(self->root_, pending, self->root_, name, 0) != 0) return false;
#ifdef VISUAL_CAPTURE_TESTING
        const auto hook = self->postLink_;
        if (hook && QByteArray(name) != "visual-ready") {
            try { hook(); }
            catch (...) { return false; }
        }
        if (!self) return false;
#endif
        if (!self->guardedContext(context) || !self || !self->fileBound(pending, original, 2) ||
            !self->fileBound(name, original, 2)) return false;
        return ::unlinkat(self->root_, pending, 0) == 0;
    }
    bool guardedPublish(const char *name, const char *pending, const QByteArray &bytes,
                        const std::function<bool()> &context, struct stat *published = nullptr) const {
        QPointer<Entry> self(const_cast<Entry *>(this));
        if (bytes.isEmpty() || bytes.size() > 1024 || !guardedContext(context) || !self) return false;
        const int fd = ::openat(self->root_, pending, O_WRONLY | O_CREAT | O_EXCL | O_NOFOLLOW | O_CLOEXEC, 0600);
        if (fd < 0) return false;
        struct stat original{}, final{};
        const bool fileOk = ::fstat(fd, &original) == 0 && S_ISREG(original.st_mode) &&
            original.st_uid == self->rootStat_.st_uid && (original.st_mode & 07777) == 0600 &&
            original.st_nlink == 1 && original.st_size == 0;
        const ssize_t n = fileOk ? ::write(fd, bytes.constData(), size_t(bytes.size())) : -1;
        const bool bound = ::fstat(fd, &final) == 0 && final.st_dev == original.st_dev &&
            final.st_ino == original.st_ino && final.st_size == bytes.size();
        const bool closed = ::close(fd) == 0;
        const bool committed = n == bytes.size() && bound && closed && self && self->guardedCommit(name, pending, final, context);
        if (committed && published) *published = final;
        return committed;
    }
    QByteArray bindingBytes(const char *stage, bool bound) const {
        const auto id = identity_.trimmed().split(' ');
        const QJsonObject json{{"schema", 1}, {"nonce", QString::fromLatin1(nonce_)},
            {"pid", QString::fromLatin1(id[0])}, {"start", QString::fromLatin1(id[1])},
            {"stage", QString::fromLatin1(stage)}, {"window_bound", bound},
            {"frame_observer_installed", bound}, {"native_authority", false}};
        return QJsonDocument(json).toJson(QJsonDocument::Compact) + '\n';
    }
    bool initialStatus(bool initial, const char *stage) {
        if (!witnessMode_ || !initial) return true;
        const auto bytes = bindingBytes(stage, false);
        return bytes.size() <= 512 && publish("visual-binding-status", ".pending-binding-status", bytes,
            [this] { return !terminal_ && now() < 60000 && trusted(); });
    }
    bool frameContext() const {
        if (terminal_ || now() >= 60000 || !trusted() || !frameWindow_ || !frameEngine_ || !frameRoot_) return false;
        const bool ok = QThread::currentThread() == app_->thread() && frameEngine_->thread() == app_->thread() &&
            frameWindow_->thread() == app_->thread() && QGuiApplication::focusWindow() == frameWindow_ &&
            frameWindow_->isVisible() && frameWindow_->isActive() && qmlEngine(frameWindow_) == frameEngine_ &&
            frameWindow_->contentItem() == frameRoot_;
        return !terminal_ && now() < 60000 && trusted() && frameWindow_ && frameEngine_ && frameRoot_ && ok;
    }
    void bindFrameWindow() {
        QPointer<Entry> self(this);
        if (terminal_ || frameBound_) return;
        if (now() >= 60000 || !trusted()) { stop(); return; }
        const bool initial = !initialBindingAttempt_;
        initialBindingAttempt_ = true;
        const auto raw = QGuiApplication::allWindows();
        if (raw.size() > 16) { initialStatus(initial, "binding-rejected"); if (self) self->stop(); return; }
        QList<QPointer<QWindow>> windows;
        for (auto *w : raw) windows.append(w);
        QPointer<QQmlEngine> engine;
        for (const auto &w : windows) {
            if (!trusted() || !w || w->thread() != app_->thread()) { initialStatus(initial, "binding-rejected"); if (self) self->stop(); return; }
            QPointer<QQmlEngine> e = qmlEngine(w);
            if (!trusted() || !w || (e && engine && e != engine)) { initialStatus(initial, "binding-rejected"); if (self) self->stop(); return; }
            if (e) engine = e;
        }
        QPointer<QQuickWindow> window = qobject_cast<QQuickWindow *>(QGuiApplication::focusWindow());
        if (!window || !engine || !window->isVisible() || !window->isActive()) {
            const bool status = initialStatus(initial, "waiting-first-eligible-window");
            if (self && !status) self->stop();
            return;
        }
        frameWindow_ = window; frameEngine_ = engine; frameRoot_ = window->contentItem();
        if (!frameContext()) { initialStatus(initial, "binding-rejected"); if (self) self->stop(); return; }
        frameBound_ = true;
        QObject::disconnect(focusConnection_);
        const auto selectedSignal = frameSignal_ == FrameSignal::FrameSwapped ?
            &QQuickWindow::frameSwapped : &QQuickWindow::afterFrameEnd;
        frameConnection_ = connect(window, selectedSignal, this, [this] {
            QPointer<Entry> weak(this);
            if (terminal_ || frameSeen_) return;
            if (witnessMode_ && !boundWitnessPublished_) { stop(); return; }
            if (!frameContext()) { stop(); return; }
            // A queued frame delivery must not outrun notification of an
            // already-created request. Presence before marker admission consumes
            // the attempt even when the directory watcher has not run yet.
            if (!absent("visual-request")) {
                consumed_ = true;
                Result r; r.stage = "frame-not-admitted"; finish(r); return;
            }
            frameSeen_ = true;
            QObject::disconnect(frameConnection_);
            const auto id = identity_.trimmed().split(' ');
            const QJsonObject marker{{"schema", 1}, {"nonce", QString::fromLatin1(nonce_)},
                {"pid", QString::fromLatin1(id[0])}, {"start", QString::fromLatin1(id[1])},
                {"stage", "frame-stage-observed"}, {"signal", frameSignal_ == FrameSignal::FrameSwapped ? "frameSwapped" : "afterFrameEnd"},
                {"gui_delivery_observed", true}, {"native_authority", false}};
            frameMarker_ = QJsonDocument(marker).toJson(QJsonDocument::Compact) + '\n';
            const bool published = frameMarker_.size() <= 512 && publish("visual-frame-ready", ".pending-frame-ready", frameMarker_,
                [this] { return frameContext() && absent("visual-request"); });
            if (weak && !published) weak->stop();
        }, Qt::QueuedConnection);
        destructionConnection_ = connect(window, &QObject::destroyed, this, [this] { stop(); });
        if (witnessMode_) {
            if (!frameConnection_) { frameBound_ = false; initialStatus(initial, "binding-rejected"); if (self) self->stop(); return; }
            boundWitness_ = bindingBytes("window-bound-observer-installed", true);
            const bool published = boundWitness_.size() <= 512 && publish("visual-window-bound", ".pending-window-bound", boundWitness_,
                [this] { return frameContext(); });
            if (!self) return;
            if (!published) { stop(); return; }
            boundWitnessPublished_ = true;
        }
    }
    bool imagePermit() const {
        return !terminal_ && now() < 60000 && trusted() && (!frameMode_ ||
            (frameSeen_ && frameContext() && absent(".pending-frame-ready") &&
             !frameMarker_.isEmpty() && readExact("visual-frame-ready", frameMarker_, 512))) &&
            (!witnessMode_ || (boundWitnessPublished_ && absent(".pending-binding-status") && absent(".pending-window-bound") &&
                readExact("visual-window-bound", boundWitness_, 512)));
    }
    qint64 now() const {
#ifdef VISUAL_CAPTURE_TESTING
        return elapsed_.elapsed() + testOffset_;
#else
        return elapsed_.elapsed();
#endif
    }
    bool absent(const char *name) const {
        struct stat st{};
        if (::fstatat(root_, name, &st, AT_SYMLINK_NOFOLLOW) == 0) return false;
        return errno == ENOENT;
    }
    static bool sameFile(const struct stat &a, const struct stat &b) {
        return a.st_dev == b.st_dev && a.st_ino == b.st_ino && a.st_mode == b.st_mode &&
            a.st_uid == b.st_uid && a.st_nlink == b.st_nlink && a.st_size == b.st_size &&
            a.st_mtim.tv_sec == b.st_mtim.tv_sec && a.st_mtim.tv_nsec == b.st_mtim.tv_nsec &&
            a.st_ctim.tv_sec == b.st_ctim.tv_sec && a.st_ctim.tv_nsec == b.st_ctim.tv_nsec;
    }
    bool readExact(const char *name, const QByteArray &expected, int cap) const {
        if (cap < 0 || cap > 512 || expected.size() > cap) return false;
        const int fd = ::openat(root_, name, O_RDONLY | O_NOFOLLOW | O_CLOEXEC | O_NONBLOCK);
        if (fd < 0) return false;
        struct stat before{}, after{};
        bool ok = ::fstat(fd, &before) == 0 && S_ISREG(before.st_mode) &&
            (before.st_mode & 07777) == 0600 && before.st_uid == rootStat_.st_uid && before.st_nlink == 1 &&
            before.st_size == expected.size() && before.st_size <= cap;
        char bytes[513];
        const ssize_t n = ok ? ::read(fd, bytes, size_t(cap + 1)) : -1;
        ok = ok && n == expected.size() && QByteArray(bytes, n) == expected &&
            ::fstat(fd, &after) == 0 && sameFile(before, after);
        const bool closed = ::close(fd) == 0;
        return ok && closed;
    }
    bool trusted() const {
        if (root_ < 0 || !app_ || QCoreApplication::closingDown()) return false;
        const int fresh = ::open(path_.toLocal8Bit().constData(), O_RDONLY | O_DIRECTORY | O_NOFOLLOW | O_CLOEXEC);
        if (fresh < 0) return false;
        struct stat current{};
        const bool same = ::fstat(fresh, &current) == 0 && current.st_dev == rootStat_.st_dev &&
            current.st_ino == rootStat_.st_ino && current.st_uid == rootStat_.st_uid &&
            (current.st_mode & 07777) == 0700;
        const bool closed = ::close(fresh) == 0;
        return same && closed && absent("visual-cancel") && absent("restore.claim") &&
            readExact("owner", nonce_, 32) && readExact("attempt.identity", identity_, 42);
    }
    bool fileBound(const char *name, const struct stat &original, nlink_t links) const {
        struct stat st{};
        return ::fstatat(root_, name, &st, AT_SYMLINK_NOFOLLOW) == 0 && S_ISREG(st.st_mode) &&
            st.st_dev == original.st_dev && st.st_ino == original.st_ino && st.st_uid == rootStat_.st_uid &&
            (st.st_mode & 07777) == 0600 && st.st_nlink == links && st.st_size == original.st_size &&
            st.st_mtim.tv_sec == original.st_mtim.tv_sec && st.st_mtim.tv_nsec == original.st_mtim.tv_nsec;
    }
    bool commit(const char *name, const char *pending, const struct stat &original,
                const std::function<bool()> &context) const {
        if (readerMode_) return guardedCommit(name, pending, original, context);
        const auto guard = [&] { return trusted() && (!context || context()); };
        if (!guard() || !fileBound(pending, original, 1)) return false;
        if (::linkat(root_, pending, root_, name, 0) != 0) return false;
#ifdef VISUAL_CAPTURE_TESTING
        if (postLink_ && QByteArray(name) != "visual-ready") postLink_();
#endif
        if (!guard() || !fileBound(pending, original, 2) || !fileBound(name, original, 2)) return false;
        return ::unlinkat(root_, pending, 0) == 0;
    }
    bool publish(const char *name, const char *pending, const QByteArray &bytes,
                 const std::function<bool()> &context) const {
        if (readerMode_) return guardedPublish(name, pending, bytes, context);
        if (bytes.isEmpty() || bytes.size() > 1024 || !trusted() || (context && !context())) return false;
        const int fd = ::openat(root_, pending, O_WRONLY | O_CREAT | O_EXCL | O_NOFOLLOW | O_CLOEXEC, 0600);
        if (fd < 0) return false;
        struct stat original{}, final{};
        const bool fileOk = ::fstat(fd, &original) == 0 && S_ISREG(original.st_mode) &&
            original.st_uid == rootStat_.st_uid && (original.st_mode & 07777) == 0600 &&
            original.st_nlink == 1 && original.st_size == 0;
        const ssize_t n = fileOk ? ::write(fd, bytes.constData(), size_t(bytes.size())) : -1;
        const bool bound = ::fstat(fd, &final) == 0 && final.st_dev == original.st_dev &&
            final.st_ino == original.st_ino && final.st_size == bytes.size();
        const bool closed = ::close(fd) == 0;
        return n == bytes.size() && bound && closed && commit(name, pending, final, context);
    }
    Result image() {
        QPointer<Entry> self(this);
        Result r; r.stage = "image-output-refused";
        const int fd = ::openat(root_, ".pending-image", O_WRONLY | O_CREAT | O_EXCL | O_NOFOLLOW | O_CLOEXEC, 0600);
        if (fd < 0) return r;
        struct stat original{}, final{};
        const bool good = ::fstat(fd, &original) == 0 && S_ISREG(original.st_mode) &&
            original.st_uid == rootStat_.st_uid && (original.st_mode & 07777) == 0600 &&
            original.st_nlink == 1 && original.st_size == 0;
        Sink sink(fd);
        if (good) {
            if (readerMode_) {
                try {
                const auto permit = [self] {
                    if (!self) return false;
#ifdef VISUAL_CAPTURE_TESTING
                    const auto hook = self->readerPermitHook_;
                    if (hook) hook();
                    if (!self) return false;
#endif
                    const bool ok = self->imagePermit();
                    return self && ok;
                };
                const auto observer = readerObserver();
#ifdef VISUAL_CAPTURE_TESTING
                const auto grab = grab_;
                r = captureObserved(permit, sink, observer, grab);
#else
                r = captureObserved(permit, sink, observer);
#endif
                } catch (...) {
                    // Only the new synchronous callback path: finish local FD
                    // cleanup below even if a test grab/callback throws or dies.
                    r = {}; r.stage = "reader-observation-refused";
                }
            } else {
#ifdef VISUAL_CAPTURE_TESTING
            r = capture([this] { return imagePermit(); }, sink, grab_);
#else
            r = capture([this] { return imagePermit(); }, sink);
#endif
            }
        }
        const bool bound = ::fstat(fd, &final) == 0 && final.st_dev == original.st_dev &&
            final.st_ino == original.st_ino && final.st_size == sink.count();
        sink.close();
        const bool closed = ::close(fd) == 0;
        if (!self) return r;
        if (QByteArray(r.stage) != "visual-image-observed") return r;
        const bool valid = sink.valid() && bound && closed && r.context && r.context();
        if (!self) return r;
        if (!valid || !self->commit("visual-image.png", ".pending-image", final, r.context))
            r.stage = "image-publication-refused";
        return r;
    }
    void stop() {
        terminal_ = true; timer_.stop();
        QObject::disconnect(focusConnection_);
        QObject::disconnect(frameConnection_);
        QObject::disconnect(destructionConnection_);
        const auto paths = watcher_.directories();
        if (!paths.isEmpty()) watcher_.removePaths(paths);
    }
    void finish(Result r) {
        QPointer<Entry> self(this);
        if (terminal_ || finishing_) return;
        finishing_ = true;
        if (!trusted()) { stop(); return; }
        if (now() >= 60000) r.stage = "setup-expired";
        const bool success = QByteArray(r.stage) == "visual-image-observed";
        const auto context = r.context;
        const bool valid = !success || (context && context());
        if (!self) return;
        if (!valid) { stop(); return; }
        std::function<bool()> finalContext = success ? context : std::function<bool()>{};
        if (success && readerMode_) {
            const auto bytes = readerBytes(r.sha);
            struct stat publishedStat{};
            const bool published = !bytes.isEmpty() && bytes.size() <= 1024 &&
                guardedPublish("visual-reader-structure", ".pending-reader-structure", bytes, context, &publishedStat);
            if (!self) return;
            if (!published) { stop(); return; }
            readerMarker_ = bytes; readerStat_ = publishedStat; readerPublished_ = true;
            finalContext = [self, context] {
                if (!self || !context || !context() || !self) return false;
                return self->readReaderExact();
            };
        }
        const auto identity = identity_.trimmed().split(' ');
        const QJsonObject json{{"schema", 1}, {"nonce", QString::fromLatin1(nonce_)},
            {"pid", QString::fromLatin1(identity[0])}, {"start", QString::fromLatin1(identity[1])},
            {"stage", QString::fromLatin1(r.stage)}, {"application_thread", r.applicationThread},
            {"current_window", r.currentWindow}, {"width", r.width}, {"height", r.height},
            {"storage_bytes", double(r.storage)}, {"encoded_bytes", double(r.encoded)},
            {"device_pixel_ratio", std::isfinite(r.dpr) ? r.dpr : 0},
            {"png_sha256", QString::fromLatin1(r.sha)}, {"native_authority", false}};
        publish("visual-result", ".pending-result", QJsonDocument(json).toJson(QJsonDocument::Compact) + '\n',
                finalContext);
        if (self) self->stop();
    }
    void queue() {
        if (terminal_ || queued_) return;
        queued_ = true;
        QPointer<Entry> weak(this);
        if (!QMetaObject::invokeMethod(this, [weak] {
            if (!weak || weak->terminal_) return;
            weak->queued_ = false;
            weak->check();
        }, Qt::QueuedConnection)) stop();
    }
    void check() {
        QPointer<Entry> self(this);
        if (terminal_ || consumed_) return;
        if (!trusted()) { stop(); return; }
        Result r;
        if (now() >= 60000) { r.stage = "setup-expired"; finish(r); return; }
        if (++checks_ > 32) { r.stage = "request-check-cap"; finish(r); return; }
        if (absent("visual-request")) return;
        if (!readExact("visual-request", nonce_ + ' ' + identity_, 75)) {
            r.stage = "request-refused"; finish(r); return;
        }
        consumed_ = true;
        if (frameMode_ && !imagePermit()) { r.stage = "frame-not-admitted"; finish(r); return; }
        r = image();
        if (self) self->finish(r);
    }
    QPointer<QGuiApplication> app_;
    QString path_;
    QByteArray nonce_, identity_;
    int root_ = -1, checks_ = 0;
    struct stat rootStat_{};
    QElapsedTimer elapsed_;
    QFileSystemWatcher watcher_;
    QTimer timer_;
    bool started_ = false, terminal_ = false, queued_ = false, consumed_ = false, finishing_ = false;
    bool frameMode_ = false, frameBound_ = false, frameSeen_ = false;
    bool witnessMode_ = false, initialBindingAttempt_ = false, boundWitnessPublished_ = false;
    const FrameSignal frameSignal_;
    const bool readerMode_;
    reader_metadata::Result readerBefore_, readerAfter_;
    bool readerBeforeDone_ = false, readerAfterDone_ = false, readerPublished_ = false;
    QByteArray readerMarker_;
    struct stat readerStat_{};
    QByteArray boundWitness_;
    QPointer<QQuickWindow> frameWindow_;
    QPointer<QQmlEngine> frameEngine_;
    QPointer<QQuickItem> frameRoot_;
    QByteArray frameMarker_;
    QMetaObject::Connection focusConnection_, frameConnection_, destructionConnection_;
#ifdef VISUAL_CAPTURE_TESTING
    qint64 testOffset_ = 0;
    std::function<void()> postLink_;
    std::function<void(SamplePhase)> readerSampleHook_;
    std::function<void()> readerPermitHook_;
    std::function<QImage(QQuickWindow *)> grab_;
#endif
};
}
