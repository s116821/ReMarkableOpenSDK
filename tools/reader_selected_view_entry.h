#pragma once
// Separate one-request entry. No injection mechanism or stock restoration claim.
#include "reader_selected_view_probe.h"
#include "reader_fixture_navigation.h"
#include "reader_navigation_ownership.h"
#include <QFileSystemWatcher>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTimer>
#include <cerrno>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

namespace reader_selected {
inline constexpr qint64 SetupWindowMs = 90000;
inline bool decimal(const QByteArray &s) {
    if (s.isEmpty() || s.size() > 20 || s[0] == '0') return false;
    for (char c : s) if (c < '0' || c > '9') return false;
    bool ok = false; s.toULongLong(&ok); return ok;
}
inline QByteArray selfIdentity() {
    const int fd = ::open("/proc/self/stat", O_RDONLY | O_NOFOLLOW | O_CLOEXEC);
    if (fd < 0) return {};
    char bytes[4097]; const ssize_t n = ::read(fd, bytes, sizeof bytes);
    const bool closed = ::close(fd) == 0;
    if (!closed || n <= 0 || n > 4096) return {};
    const QByteArray stat(bytes, n);
    const int end = stat.lastIndexOf(')');
    if (end < 0) return {};
    const auto fields = stat.mid(end + 1).simplified().split(' ');
    if (fields.size() <= 19 || !decimal(fields[19])) return {};
    return QByteArray::number(::getpid()) + ' ' + fields[19] + '\n';
}

class Entry final : public QObject {
public:
    enum class Diagnostic { SelectedPage, CreationSourceJoin, NavigateCreationSourceJoin, NavigationOwnership };
    Entry(QGuiApplication *app, QString path, QByteArray nonce, QObject *parent = nullptr,
          Diagnostic diagnostic = Diagnostic::SelectedPage, FixtureNavigation navigation = {})
        : QObject(parent ? parent : app), app_(app), path_(std::move(path)), nonce_(std::move(nonce)),
          diagnostic_(diagnostic), navigation_(std::move(navigation)) { elapsed_.start(); }
    ~Entry() override { if (root_ >= 0) ::close(root_); }
    bool start() {
        if (started_) return false;
        started_ = true;
        if (!app_ || QThread::currentThread() != app_->thread() || thread() != app_->thread() || nonce_.size() != 32)
            return terminal_ = true, false;
        if ((diagnostic_ != Diagnostic::SelectedPage && diagnostic_ != Diagnostic::CreationSourceJoin &&
             diagnostic_ != Diagnostic::NavigateCreationSourceJoin && diagnostic_ != Diagnostic::NavigationOwnership) ||
            (diagnostic_ == Diagnostic::NavigateCreationSourceJoin && !navigation_.valid())) return terminal_ = true, false;
        for (char c : nonce_) if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f'))) return terminal_ = true, false;
        identity_ = selfIdentity();
        if (identity_.isEmpty()) return terminal_ = true, false;
        root_ = ::open(path_.toLocal8Bit().constData(), O_RDONLY | O_DIRECTORY | O_NOFOLLOW | O_CLOEXEC);
        if (root_ < 0 || ::fstat(root_, &rootStat_) != 0 || !S_ISDIR(rootStat_.st_mode) ||
            (rootStat_.st_mode & 07777) != 0700 || rootStat_.st_uid != ::geteuid() || !trusted())
            return terminal_ = true, false;
        if (!absent("selected-ready") || !absent("selected-result") || !absent(".pending-ready") || !absent(".pending-result"))
            return terminal_ = true, false;
        connect(&watcher_, &QFileSystemWatcher::directoryChanged, this, [this] { queue(); });
        connect(&timer_, &QTimer::timeout, this, [this] { finish("setup-expired"); });
        connect(&navigationTimer_, &QTimer::timeout, this, [this] { afterNavigation(); });
        navigationTimer_.setSingleShot(true); navigationTimer_.setTimerType(Qt::PreciseTimer);
        connect(app_, &QCoreApplication::aboutToQuit, this, [this] { finish("application-closing", {}, false); });
        if (!watcher_.addPath(path_)) return terminal_ = true, false;
        timer_.setSingleShot(true); timer_.setTimerType(Qt::PreciseTimer);
        const qint64 remaining = SetupWindowMs - now();
        if (remaining <= 0) { finish("setup-expired"); return false; }
        timer_.start(int(remaining));
        if (!publish("selected-ready", ".pending-ready", nonce_ + ' ' + identity_)) {
            finish("ready-publication-refused", {}, false); return false;
        }
        queue(); return true;
    }
    bool terminal() const { return terminal_; }
#ifdef READER_SELECTED_TESTING
    void expireForTest() { offset_ = SetupWindowMs; finish("setup-expired"); }
    void checkForTest() { queue(); }
    bool samplingForTest() const { return sampling_; }
    bool disarmedForTest() const { return disarmed_; }
    int samplesForTest() const { return samples_; }
    bool navigationPendingForTest() const { return navigationPending_; }
    void afterNavigationForTest() { afterNavigation(); }
    void advanceForTest(qint64 milliseconds) { offset_ += milliseconds; }
    void postOwnershipForTest(std::function<void(NavigationOwnership &)> hook) { postOwnership_ = std::move(hook); }
    void postLinkForTest(std::function<void()> hook) { postLink_ = std::move(hook); }
    void preLinkForTest(std::function<void()> hook) { preLink_ = std::move(hook); }
#endif
private:
    qint64 now() const {
#ifdef READER_SELECTED_TESTING
        return elapsed_.elapsed() + offset_;
#else
        return elapsed_.elapsed();
#endif
    }
    static bool sameFile(const struct stat &a, const struct stat &b) {
        return a.st_dev == b.st_dev && a.st_ino == b.st_ino && a.st_mode == b.st_mode &&
            a.st_uid == b.st_uid && a.st_nlink == b.st_nlink && a.st_size == b.st_size &&
            a.st_mtim.tv_sec == b.st_mtim.tv_sec && a.st_mtim.tv_nsec == b.st_mtim.tv_nsec &&
            a.st_ctim.tv_sec == b.st_ctim.tv_sec && a.st_ctim.tv_nsec == b.st_ctim.tv_nsec;
    }
    bool absent(const char *name) const {
        struct stat st{};
        if (::fstatat(root_, name, &st, AT_SYMLINK_NOFOLLOW) == 0) return false;
        return errno == ENOENT;
    }
    bool readExact(const char *name, const QByteArray &expected, int cap, struct stat *identity = nullptr) const {
        if (cap < 0 || cap > 75 || expected.size() > cap) return false;
        const int fd = ::openat(root_, name, O_RDONLY | O_NOFOLLOW | O_CLOEXEC | O_NONBLOCK);
        if (fd < 0) return false;
        struct stat before{}, after{}, named{};
        bool ok = ::fstat(fd, &before) == 0 && S_ISREG(before.st_mode) &&
            (before.st_mode & 07777) == 0600 && before.st_uid == rootStat_.st_uid && before.st_nlink == 1 &&
            before.st_size == expected.size();
        char bytes[76]; const ssize_t n = ok ? ::read(fd, bytes, size_t(cap + 1)) : -1;
        ok = ok && n == expected.size() && QByteArray(bytes, n) == expected &&
            ::fstat(fd, &after) == 0 && sameFile(before, after) &&
            ::fstatat(root_, name, &named, AT_SYMLINK_NOFOLLOW) == 0 && sameFile(after, named);
        const bool closed = ::close(fd) == 0;
        if (ok && closed && identity) *identity = after;
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
        if (!same || !closed || !absent("selected-cancel") || !absent("restore.claim") ||
            !readExact("owner", nonce_, 32) || !readExact("attempt.identity", identity_, 42)) return false;
        if (consumed_) {
            struct stat request{};
            return readExact("selected-request", nonce_ + ' ' + identity_, 75, &request) && sameFile(requestStat_, request);
        }
        return true;
    }
    bool publish(const char *name, const char *pending, const QByteArray &bytes, bool requireDeadline = true) const {
        const auto admitted = [&] { return trusted() && (!requireDeadline || now() < SetupWindowMs); };
        const int cap = diagnostic_ == Diagnostic::NavigationOwnership && QByteArray(name) == "selected-result" ? 16384 : 2048;
        if (bytes.isEmpty() || bytes.size() > cap || !admitted()) return false;
        const int fd = ::openat(root_, pending, O_WRONLY | O_CREAT | O_EXCL | O_NOFOLLOW | O_CLOEXEC, 0600);
        if (fd < 0) return false;
        struct stat st{};
        const bool valid = ::fstat(fd, &st) == 0 && S_ISREG(st.st_mode) && st.st_uid == rootStat_.st_uid &&
            (st.st_mode & 07777) == 0600 && st.st_nlink == 1 && st.st_size == 0;
        const ssize_t n = valid ? ::write(fd, bytes.constData(), size_t(bytes.size())) : -1;
        const bool closed = ::close(fd) == 0;
        if (n != bytes.size() || !closed || !admitted()) return false;
#ifdef READER_SELECTED_TESTING
        if (preLink_ && QByteArray(name) == "selected-result") preLink_();
#endif
        if (!admitted()) return false;
        if (::linkat(root_, pending, root_, name, 0) != 0) return false;
#ifdef READER_SELECTED_TESTING
        if (postLink_ && QByteArray(name) == "selected-result") postLink_();
#endif
        // A post-link trust failure leaves pending+nlink2: unknown, not a valid
        // receipt. Consumers must independently bracket this sampled commit;
        // this is not an atomic cancellation or persistence fence.
        if (!admitted() || ::unlinkat(root_, pending, 0) != 0) return false;
        return true;
    }
    void disarm() {
        disarmed_ = true; timer_.stop(); navigationTimer_.stop(); navigationPending_ = false;
        const auto paths = watcher_.directories();
        if (!paths.isEmpty()) watcher_.removePaths(paths);
    }
    void finish(QString stage, Observation observation = {}, bool allowResult = true) {
        if (terminal_) return;
        disarm();
        if (!allowResult) suppressed_ = true;
        if (sampling_) {
            if (pendingStage_.isEmpty() || stage == QStringLiteral("setup-expired")) pendingStage_ = std::move(stage);
            return; // Never publish or destroy an entry on its sample stack.
        }
        terminal_ = true;
        if (suppressed_ || !trusted()) return;
        if (!pendingStage_.isEmpty()) { stage = pendingStage_; observation = {}; ownership_ = {}; }
        if (now() >= SetupWindowMs) { stage = QStringLiteral("setup-expired"); observation = {}; ownership_ = {}; }
        if (observation.documentId.size() > 36 || observation.pageId.toUtf8().size() > 256 || observation.refusal.size() > 128) {
            stage = QStringLiteral("observation-cap"); observation = {};
        }
        const auto identity = identity_.trimmed().split(' ');
        if (identity.size() != 2) return;
        QJsonObject json{{"schema", 1}, {"nonce", QString::fromLatin1(nonce_)},
            {"pid", QString::fromLatin1(identity[0])}, {"start", QString::fromLatin1(identity[1])},
            {"stage", stage}, {"refusal", observation.refusal}, {"observed", observation.observed},
            {"document_id", observation.documentId}, {"page_id", observation.pageId},
            {"page_index", observation.pageIndex}, {"page_count", observation.pageCount},
            {"map_coherent", observation.mapCoherent}, {"status", observation.status}, {"page_status", observation.pageStatus},
            {"loading", observation.loading}, {"page_error", observation.pageError}, {"document_error", observation.documentError},
            {"awaiting_tiles", observation.awaitingTiles}, {"pending_tiles", observation.pendingTiles}, {"working", observation.working},
            {"ready", observation.ready}, {"sample_begin_ms", sampleBegin_}, {"sample_end_ms", sampleEnd_},
            {"native_authority", false}, {"render_authority", false}, {"durable_authority", false}};
        if (diagnostic_ == Diagnostic::CreationSourceJoin) {
            json.insert(QStringLiteral("diagnostic"), QStringLiteral("creation-source-join"));
            json.insert(QStringLiteral("creation_join_observed"), observation.creationJoinObserved);
        }
        if (diagnostic_ == Diagnostic::NavigateCreationSourceJoin) {
            json.insert(QStringLiteral("diagnostic"), QStringLiteral("navigate-creation-source-join"));
            json.insert(QStringLiteral("navigation_attempted"), navigationAttempted_);
            json.insert(QStringLiteral("navigation_dispatched"), navigationDispatched_);
            json.insert(QStringLiteral("creation_join_observed"), observation.creationJoinObserved);
        }
        if (diagnostic_ == Diagnostic::NavigationOwnership) {
            json.insert(QStringLiteral("diagnostic"), QStringLiteral("navigation-ownership"));
            json.insert(QStringLiteral("census_complete"), ownership_.complete);
            json.insert(QStringLiteral("visited"), ownership_.visited);
            json.insert(QStringLiteral("owners"), ownership_.owners);
        }
        QByteArray bytes = QJsonDocument(json).toJson(QJsonDocument::Compact) + '\n';
        if (diagnostic_ == Diagnostic::NavigationOwnership && bytes.size() > 16384) {
            json.insert(QStringLiteral("owners"), QJsonArray());
            json.insert(QStringLiteral("census_complete"), false);
            json.insert(QStringLiteral("stage"), QStringLiteral("observation-refused"));
            json.insert(QStringLiteral("refusal"), QStringLiteral("ownership-wire-cap"));
            bytes = QJsonDocument(json).toJson(QJsonDocument::Compact) + '\n';
        }
        if (bytes.size() > (diagnostic_ == Diagnostic::NavigationOwnership ? 16384 : 2048)) return;
        publish("selected-result", ".pending-result", bytes, stage == QStringLiteral("setup-expired") ? false : true);
    }
    bool permit() {
        if (terminal_ || disarmed_) return false;
        if (!trusted()) { finish("trust-lost", {}, false); return false; }
        if (now() >= SetupWindowMs) { finish("setup-expired"); return false; }
        return true;
    }
    void queue() {
        if (terminal_ || disarmed_ || queued_) return;
        queued_ = true;
        const QPointer<Entry> weak(this);
        if (!QMetaObject::invokeMethod(this, [weak] {
            if (!weak || weak->terminal_ || weak->disarmed_) return;
            weak->queued_ = false; weak->check();
        }, Qt::QueuedConnection)) finish("queue-refused", {}, false);
    }
    void check() {
        if (!permit()) return;
        // Nested watcher delivery checks cancellation/trust through permit(),
        // but can neither repeat nor finish the synchronous sample early.
        if (sampling_ || consumed_) return;
        if (++checks_ > 32) { finish("request-check-cap"); return; }
        if (absent("selected-request")) return;
        if (!readExact("selected-request", nonce_ + ' ' + identity_, 75, &requestStat_)) {
            finish("request-refused"); return;
        }
        consumed_ = true;
        if (!permit()) return;
        // The first native/window selection occurs only after exact admission.
        QPointer<QQuickWindow> window = qobject_cast<QQuickWindow *>(QGuiApplication::focusWindow());
        sampleBegin_ = now();
        operationEnd_ = qMin<qint64>(SetupWindowMs, sampleBegin_ + MaxObservationMs);
        if (operationEnd_ <= now()) { finish("setup-expired"); return; }
        if (diagnostic_ == Diagnostic::NavigationOwnership) { sampleOwnership(window); return; }
        if (diagnostic_ != Diagnostic::NavigateCreationSourceJoin) { sample(window); return; }
        const QPointer<Entry> weak(this);
        operationEngine_ = window ? qmlEngine(window) : nullptr;
        sampling_ = true;
        const auto navigation = openFixture(window, navigation_, [weak] {
            return weak && weak->permit() && weak->now() < weak->operationEnd_;
        },
            int(operationEnd_ - now()));
        if (!weak) return;
        navigationAttempted_ = navigation.attempted; navigationDispatched_ = navigation.dispatched;
        sampling_ = false;
        if (!permit()) { finishStopped(); return; }
        if (now() >= operationEnd_) {
            Observation r; r.refusal = QStringLiteral("deadline"); sampleEnd_ = now(); finish("observation-refused", r); return;
        }
        if (!navigation.dispatched || !navigation.refusal.isEmpty()) {
            Observation r; r.refusal = navigation.refusal.isEmpty() ? QStringLiteral("navigation-dispatch-refused") : navigation.refusal;
            sampleEnd_ = now(); finish("observation-refused", r); return;
        }
        // One deferred fresh sample, not an event pump or readiness/reopen loop.
        if (operationEnd_ - now() <= NavigationDelayMs) {
            Observation r; r.refusal = QStringLiteral("deadline"); sampleEnd_ = now(); finish("observation-refused", r); return;
        }
        navigationPending_ = true; navigationTimer_.start(NavigationDelayMs);
    }
    void finishStopped() {
        if (!terminal_) finish(pendingStage_.isEmpty() ? QStringLiteral("sample-disarmed") : pendingStage_, {}, !suppressed_);
    }
    void sampleOwnership(QQuickWindow *window) {
        const QPointer<Entry> weak(this);
        sampling_ = true;
#ifdef READER_SELECTED_TESTING
        ++samples_;
#endif
        auto result = inspectNavigationOwnership(window, [weak] {
            return weak && weak->permit() && weak->now() < weak->operationEnd_;
        }, int(operationEnd_ - now()));
        if (!weak) return;
        sampling_ = false; sampleEnd_ = now();
        if (!permit()) { finishStopped(); return; }
        if (now() >= operationEnd_) { result = {}; result.refusal = QStringLiteral("deadline"); }
        ownership_ = std::move(result);
#ifdef READER_SELECTED_TESTING
        if (postOwnership_) postOwnership_(ownership_);
#endif
        Observation r; r.refusal = ownership_.refusal;
        finish(ownership_.complete ? QStringLiteral("ownership-observed") : QStringLiteral("observation-refused"), r);
    }
    void afterNavigation() {
        if (terminal_ || disarmed_ || !navigationPending_ || sampling_) return;
        navigationPending_ = false; navigationTimer_.stop();
        if (!permit()) { finishStopped(); return; }
        QPointer<QQuickWindow> window = qobject_cast<QQuickWindow *>(QGuiApplication::focusWindow());
        if (!window || !operationEngine_ || qmlEngine(window) != operationEngine_) {
            Observation r; r.refusal = QStringLiteral("navigation-context-changed"); sampleEnd_ = now(); finish("observation-refused", r); return;
        }
        sample(window);
    }
    void sample(QQuickWindow *window) {
        const qint64 remaining = operationEnd_ - now();
        if (remaining <= 0) { Observation r; r.refusal = QStringLiteral("deadline"); sampleEnd_ = now(); finish("observation-refused", r); return; }
        const int budget = int(remaining);
        const QPointer<Entry> weak(this);
        sampling_ = true;
#ifdef READER_SELECTED_TESTING
        ++samples_;
#endif
        const auto allowed = [weak] {
            return weak && weak->permit() && (weak->diagnostic_ != Diagnostic::NavigateCreationSourceJoin || weak->now() < weak->operationEnd_);
        };
        Observation observation = diagnostic_ == Diagnostic::SelectedPage
            ? observe(window, allowed, budget) : observeCreationJoin(window, allowed, budget);
        if (!weak) return;
        sampling_ = false; sampleEnd_ = now();
        if (!permit()) {
            finishStopped();
            return;
        }
        if (diagnostic_ == Diagnostic::NavigateCreationSourceJoin && (now() >= operationEnd_ ||
            (observation.observed && (observation.documentId != navigation_.documentId || observation.pageIndex != navigation_.pageIndex)))) {
            const bool late = now() >= operationEnd_;
            observation = {}; observation.refusal = late ? QStringLiteral("deadline") : QStringLiteral("navigation-target-not-observed");
        }
        finish(observation.observed ? QStringLiteral("observed") : QStringLiteral("observation-refused"), observation);
    }
    QPointer<QGuiApplication> app_;
    QString path_, pendingStage_;
    QByteArray nonce_, identity_;
    Diagnostic diagnostic_;
    FixtureNavigation navigation_;
    NavigationOwnership ownership_;
    QPointer<QQmlEngine> operationEngine_;
    int root_ = -1, checks_ = 0;
    struct stat rootStat_{}, requestStat_{};
    QElapsedTimer elapsed_;
    QFileSystemWatcher watcher_;
    QTimer timer_;
    QTimer navigationTimer_;
    static constexpr int NavigationDelayMs = 2000;
    qint64 sampleBegin_ = -1, sampleEnd_ = -1;
    qint64 operationEnd_ = -1;
    bool started_ = false, terminal_ = false, queued_ = false, consumed_ = false;
    bool sampling_ = false, disarmed_ = false, suppressed_ = false;
    bool navigationPending_ = false, navigationAttempted_ = false, navigationDispatched_ = false;
#ifdef READER_SELECTED_TESTING
    qint64 offset_ = 0;
    int samples_ = 0;
    std::function<void()> postLink_;
    std::function<void()> preLink_;
    std::function<void(NavigationOwnership &)> postOwnership_;
#endif
};
} // namespace reader_selected
