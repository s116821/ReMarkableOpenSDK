#pragma once
// Separate one-request entry. No injection mechanism or stock restoration claim.
#include "reader_selected_view_probe.h"
#include "reader_fixture_navigation.h"
#include "reader_navigation_ownership.h"
#include "reader_worker_callback_witness.h"
#include "reader_native_worker_bridge.h"
#include "reader_joined_creation_witness.h"
#include "reader_identity_capture.h"
#include "reader_capture_files.h"
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
    enum class Diagnostic { SelectedPage, CreationSourceJoin, NavigateCreationSourceJoin, NavigationOwnership, NavigateCreationWorkerCallback, NavigateJoinedCreation, NavigateIdentityCapture };
    Entry(QGuiApplication *app, QString path, QByteArray nonce, QObject *parent = nullptr,
          Diagnostic diagnostic = Diagnostic::SelectedPage, FixtureNavigation navigation = {}, reader_joined_creation::Request creation = {},
          reader_identity_capture::Expected capture = {})
        : QObject(parent ? parent : app), app_(app), path_(std::move(path)), nonce_(std::move(nonce)),
          diagnostic_(diagnostic), navigation_(std::move(navigation)), creationRequest_(std::move(creation)), captureExpected_(std::move(capture)) {
        if (captureMode()) captureClaim_ = std::make_shared<reader_identity_capture::Claim>();
        elapsed_.start();
    }
    ~Entry() override {
        if (creation_) creation_->cancel(QStringLiteral("entry-destroyed"));
        creation_.reset();
        if (worker_) worker_->cancel(QStringLiteral("entry-destroyed"));
        worker_.reset(); // Disarm callbacks before native owners can release.
        workerPrepared_.reset(); workerBridge_.reset();
        if (root_ >= 0) ::close(root_);
    }
    bool start() {
        if (started_) return false;
        started_ = true;
        if (!app_ || QThread::currentThread() != app_->thread() || thread() != app_->thread() || nonce_.size() != 32)
            return terminal_ = true, false;
        if ((diagnostic_ != Diagnostic::SelectedPage && diagnostic_ != Diagnostic::CreationSourceJoin &&
             diagnostic_ != Diagnostic::NavigateCreationSourceJoin && diagnostic_ != Diagnostic::NavigationOwnership &&
             diagnostic_ != Diagnostic::NavigateCreationWorkerCallback && diagnostic_ != Diagnostic::NavigateJoinedCreation &&
             diagnostic_ != Diagnostic::NavigateIdentityCapture) ||
            (navigates() && !navigation_.valid()) || ((workerMode() || creationMode()) && !navigation_.legacyComposition) ||
            (creationMode() && (!creationRequest_.valid() || creationRequest_.documentId != navigation_.documentId ||
                creationRequest_.sourceIndex != navigation_.pageIndex)) ||
            (captureMode() && (!navigation_.legacyComposition || !captureExpected_.valid() ||
                captureExpected_.documentId != navigation_.documentId || captureExpected_.pageIndex != navigation_.pageIndex)))
            return terminal_ = true, false;
        for (char c : nonce_) if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f'))) return terminal_ = true, false;
        identity_ = selfIdentity();
        if (identity_.isEmpty()) return terminal_ = true, false;
        root_ = ::open(path_.toLocal8Bit().constData(), O_RDONLY | O_DIRECTORY | O_NOFOLLOW | O_CLOEXEC);
        if (root_ < 0 || ::fstat(root_, &rootStat_) != 0 || !S_ISDIR(rootStat_.st_mode) ||
            (rootStat_.st_mode & 07777) != 0700 || rootStat_.st_uid != ::geteuid() || !trusted())
            return terminal_ = true, false;
        if (!absent("selected-ready") || !absent("selected-result") || !absent(".pending-ready") || !absent(".pending-result"))
            return terminal_ = true, false;
        if (captureMode() && (!absent("visual-image.png") || !absent(".pending-image"))) return terminal_ = true, false;
        if (captureMode()) {
            const QPointer<Entry> weak(this);
            auto files = reader_capture_files::Files::create(root_, [weak] { return weak && weak->permit(); });
            if (!weak) return false;
            if (!files) return terminal_ = true, false;
            captureFiles_ = std::move(files);
        }
        connect(&watcher_, &QFileSystemWatcher::directoryChanged, this, [this] { queue(); });
        connect(&timer_, &QTimer::timeout, this, [this] { finish("setup-expired"); });
        connect(&navigationTimer_, &QTimer::timeout, this, [this] { afterNavigation(); });
        connect(&creationTimer_, &QTimer::timeout, this, [this] { afterCreationOpportunity(); });
        creationTimer_.setSingleShot(true); creationTimer_.setTimerType(Qt::PreciseTimer);
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
    void workerSubmitForTest(std::function<qulonglong(QObject *, const std::function<void()> &)> hook) { workerSubmitTest_ = std::move(hook); }
    using CreationSubmitTest = std::function<reader_joined_creation::Result(QObject *, QObject *, const QVariant &,
        const Observation &, const reader_joined_creation::Request &, const std::function<bool()> &)>;
    void creationSubmitForTest(CreationSubmitTest hook) { creationSubmitTest_ = std::move(hook); }
    bool creationPendingForTest() const { return creationTimer_.isActive(); }
    void afterCreationForTest() { creationTimer_.stop(); afterCreationOpportunity(); }
    void creationWirePaddingForTest(QString padding) { creationWirePaddingTest_ = padding.left(1024); }
    using CaptureRunTest = std::function<reader_identity_capture::Result(QQuickWindow *, const reader_identity_capture::Expected &,
        visual_capture::Sink &, const std::shared_ptr<reader_identity_capture::Claim> &, const std::function<bool()> &, int)>;
    void captureRunForTest(CaptureRunTest hook) { captureRunTest_ = std::move(hook); }
    void capturePublicationForTest(std::function<void()> hook) { capturePublicationTest_ = std::move(hook); }
    reader_identity_capture::Result captureResultForTest() const { return captureResult_; }
#endif
private:
    enum class CreationPhase { Initial, BeforeNext, AfterNext };
    bool workerMode() const { return diagnostic_ == Diagnostic::NavigateCreationWorkerCallback; }
    bool creationMode() const { return diagnostic_ == Diagnostic::NavigateJoinedCreation; }
    bool captureMode() const { return diagnostic_ == Diagnostic::NavigateIdentityCapture; }
    bool navigates() const { return diagnostic_ == Diagnostic::NavigateCreationSourceJoin || workerMode() || creationMode() || captureMode(); }
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
        const int cap = QByteArray(name) == "selected-result" ?
            diagnostic_ == Diagnostic::NavigationOwnership ? 16384 : creationMode() ? 4096 : 2048 : 2048;
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
        creationTimer_.stop();
        if (creation_) creation_->cancel(QStringLiteral("entry-disarmed"));
        if (worker_) worker_->cancel(QStringLiteral("entry-disarmed"));
        const auto paths = watcher_.directories();
        if (!paths.isEmpty()) watcher_.removePaths(paths);
    }
    void finish(QString stage, Observation observation = {}, bool allowResult = true) {
        if (captureMode()) { finishCapture(std::move(stage), allowResult, std::move(observation.refusal)); return; }
        if (terminal_) return;
        disarm();
        if (!allowResult) suppressed_ = true;
        if (sampling_) {
            if (pendingStage_.isEmpty() || stage == QStringLiteral("setup-expired")) pendingStage_ = std::move(stage);
            return; // Never publish or destroy an entry on its sample stack.
        }
        if (workerMode() && worker_) {
            const auto current = worker_->snapshot();
            if (current.finished || !workerResult_.finished) workerResult_ = current;
        }
        if (creationMode() && creation_) {
#ifdef READER_SELECTED_TESTING
            if (!creationSubmitTest_)
#endif
            {
            const auto current = creation_->snapshot();
            if (current.finished || !creationResult_.finished) creationResult_ = current;
            }
        }
        // Native owner release may run a deleter/notifications. It belongs on
        // this GUI path with callbacks disarmed and publication deferred.
        if (workerPrepared_) {
            sampling_ = true;
            const QPointer<Entry> weak(this);
            auto releasing = std::move(workerPrepared_);
            const bool releaseOk = releasing->close(); releasing.reset();
            if (!weak) return;
            workerReleaseOk_ = releaseOk;
            sampling_ = false;
            if (!workerReleaseOk_) {
                workerFailure_ = QStringLiteral("native-release-unknown");
                workerResult_.deliveryObserved = false;
                stage = QStringLiteral("worker-witness-refused");
            }
        }
        terminal_ = true;
        if (suppressed_ || !trusted()) return;
        if (!pendingStage_.isEmpty()) { stage = pendingStage_; observation = {}; ownership_ = {}; }
        if (now() >= SetupWindowMs) { stage = QStringLiteral("setup-expired"); observation = {}; ownership_ = {}; }
        if (workerMode() && operationEnd_ >= 0 && now() >= operationEnd_) {
            stage = QStringLiteral("worker-witness-refused"); observation = {}; observation.refusal = QStringLiteral("deadline");
            workerResult_.deliveryObserved = false;
        }
        if (creationMode() && operationEnd_ >= 0 && now() >= operationEnd_) {
            stage = QStringLiteral("creation-refused"); observation = {}; observation.refusal = QStringLiteral("deadline");
        }
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
        if (navigates()) {
            json.insert(QStringLiteral("diagnostic"), QStringLiteral("navigate-creation-source-join"));
            json.insert(QStringLiteral("navigation_attempted"), navigationAttempted_);
            json.insert(QStringLiteral("navigation_dispatched"), navigationDispatched_);
            json.insert(QStringLiteral("creation_join_observed"), observation.creationJoinObserved);
            if (navigation_.legacyComposition) json.insert(QStringLiteral("navigation_scope"), QStringLiteral("legacy-main-composition"));
        }
        if (workerMode()) {
            json.insert(QStringLiteral("diagnostic"), QStringLiteral("worker-callback-witness"));
            json.insert(QStringLiteral("observation_phase"), QStringLiteral("before-worker-submit"));
            json.insert(QStringLiteral("worker_prepared"), workerPreparedOnce_);
            json.insert(QStringLiteral("worker_finished"), workerResult_.finished);
            json.insert(QStringLiteral("worker_submission_returned"), workerResult_.submissionReturned);
            json.insert(QStringLiteral("worker_native_attempted"), workerSubmission_.attempted);
            json.insert(QStringLiteral("worker_native_accepted"), workerSubmission_.accepted);
            json.insert(QStringLiteral("worker_native_unknown"), workerSubmission_.unknown);
            json.insert(QStringLiteral("worker_job_id"), workerResult_.jobId);
            json.insert(QStringLiteral("worker_matching_completion"), workerResult_.matchingCompletion);
            json.insert(QStringLiteral("worker_delivery_observed"), workerResult_.deliveryObserved);
            json.insert(QStringLiteral("worker_canceled"), workerResult_.canceled);
            json.insert(QStringLiteral("worker_entered_count"), int(workerResult_.enteredCount));
            json.insert(QStringLiteral("worker_returning_count"), int(workerResult_.returningCount));
            json.insert(QStringLiteral("worker_off_gui_count"), int(workerResult_.offGuiCount));
            json.insert(QStringLiteral("worker_canceled_count"), int(workerResult_.canceledCount));
            json.insert(QStringLiteral("worker_completion_count"), int(workerResult_.matchingCompletionCount));
            json.insert(QStringLiteral("worker_refusal"), workerFailure_.isEmpty() ? workerResult_.refusal : workerFailure_);
            json.insert(QStringLiteral("worker_owners_released"), workerReleaseOk_);
        }
        if (creationMode()) {
            json.insert(QStringLiteral("diagnostic"), QStringLiteral("joined-creation-selection"));
            json.insert(QStringLiteral("observation_phase"), observation.observed ?
                creationTargetAlreadySelected_ ? QStringLiteral("after-create") : QStringLiteral("after-next") : QStringLiteral("unqualified"));
            json.insert(QStringLiteral("creation_target_id"), creationRequest_.targetPageId);
            json.insert(QStringLiteral("creation_source_page_id"), creationRequest_.sourcePageId);
            json.insert(QStringLiteral("creation_source_index"), creationRequest_.sourceIndex);
            json.insert(QStringLiteral("creation_expected_count"), creationRequest_.expectedPages.size());
            json.insert(QStringLiteral("creation_before_observed"), creationBefore_.observed);
            json.insert(QStringLiteral("creation_before_ready"), creationBefore_.ready);
            json.insert(QStringLiteral("creation_before_join"), creationBefore_.creationJoinObserved);
            json.insert(QStringLiteral("creation_before_count"), creationBefore_.pageCount);
            json.insert(QStringLiteral("creation_attempted"), creationResult_.attempted);
            json.insert(QStringLiteral("creation_return_known"), creationResult_.returnKnown);
            json.insert(QStringLiteral("creation_accepted"), creationResult_.accepted);
            json.insert(QStringLiteral("creation_unknown"), creationResult_.unknown);
            json.insert(QStringLiteral("creation_callback_observed"), creationResult_.callbackObserved);
            json.insert(QStringLiteral("creation_duplicate"), creationResult_.duplicate);
            json.insert(QStringLiteral("creation_canceled"), creationResult_.canceled);
            json.insert(QStringLiteral("creation_callback_count"), int(creationResult_.callbackCount));
            json.insert(QStringLiteral("creation_finished"), creationResult_.finished);
            json.insert(QStringLiteral("creation_refusal"), creationFailure_.isEmpty() ? creationResult_.refusal : creationFailure_);
            json.insert(QStringLiteral("creation_order_observed"), creationOrderObserved_);
            json.insert(QStringLiteral("creation_source_still_selected"), creationSourceStillSelected_);
            json.insert(QStringLiteral("creation_target_already_selected"), creationTargetAlreadySelected_);
            json.insert(QStringLiteral("creation_next_attempted"), creationNextAttempted_);
            json.insert(QStringLiteral("creation_next_dispatched"), creationNextDispatched_);
            json.insert(QStringLiteral("creation_next_unknown"), creationNextUnknown_);
            json.insert(QStringLiteral("effect_requires_reconciliation"), creationResult_.attempted);
#ifdef READER_SELECTED_TESTING
            if (!creationWirePaddingTest_.isEmpty()) json.insert(QStringLiteral("owned_test_padding"), creationWirePaddingTest_);
#endif
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
        if (bytes.size() > (diagnostic_ == Diagnostic::NavigationOwnership ? 16384 : creationMode() ? 4096 : 2048)) return;
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
        operationEnd_ = qMin<qint64>(SetupWindowMs, sampleBegin_ + ((creationMode() || captureMode()) ? 30000 : MaxObservationMs));
        if (operationEnd_ <= now()) { finish("setup-expired"); return; }
        if (diagnostic_ == Diagnostic::NavigationOwnership) { sampleOwnership(window); return; }
        if (!navigates()) { sample(window); return; }
        const QPointer<Entry> weak(this);
        operationEngine_ = window ? qmlEngine(window) : nullptr;
        if (captureMode()) { captureWindow_ = window; captureRoot_ = window ? window->contentItem() : nullptr; }
        sampling_ = true;
        const auto navigation = openFixture(window, navigation_, [weak] {
            return weak && weak->permit() && weak->now() < weak->operationEnd_;
        },
            int(qMin<qint64>(MaxObservationMs, operationEnd_ - now())));
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
        const int navigationDelay = navigation_.legacyComposition ? LegacyNavigationDelayMs : NavigationDelayMs;
        if (operationEnd_ - now() <= navigationDelay) {
            Observation r; r.refusal = QStringLiteral("deadline"); sampleEnd_ = now(); finish("observation-refused", r); return;
        }
        navigationPending_ = true; navigationTimer_.start(navigationDelay);
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
        if (captureMode()) { sampleCapture(window); return; }
        if (creationMode()) { sampleCreation(window); return; }
        const qint64 remaining = operationEnd_ - now();
        if (remaining <= 0) { Observation r; r.refusal = QStringLiteral("deadline"); sampleEnd_ = now(); finish("observation-refused", r); return; }
        const int budget = int(remaining);
        const QPointer<Entry> weak(this);
        sampling_ = true;
#ifdef READER_SELECTED_TESTING
        ++samples_;
#endif
        const auto allowed = [weak] {
            return weak && weak->permit() && (!weak->navigates() || weak->now() < weak->operationEnd_);
        };
        ValidatedWorkerAction workerAction;
        if (workerMode()) {
            worker_ = std::make_unique<WorkerCallbackWitness>(this);
            connect(worker_.get(), &WorkerCallbackWitness::finished, this, [weak] {
                if (weak) weak->workerFinished();
            }, Qt::QueuedConnection);
            workerAction = [weak](QObject *creation, const QVariant &id, QObject *source,
                    const Observation &before, const std::function<bool()> &guard) {
                if (weak) weak->submitWorker(creation, id, source, before, guard);
            };
        }
        Observation observation = diagnostic_ == Diagnostic::SelectedPage
            ? observe(window, allowed, budget) : observeCreationJoin(window, allowed, budget, std::move(workerAction));
        if (!weak) return;
        sampling_ = false; sampleEnd_ = now();
        if (!permit()) {
            finishStopped();
            return;
        }
        if (navigates() && (now() >= operationEnd_ ||
            (observation.observed && (observation.documentId != navigation_.documentId || observation.pageIndex != navigation_.pageIndex)))) {
            const bool late = now() >= operationEnd_;
            observation = {}; observation.refusal = late ? QStringLiteral("deadline") : QStringLiteral("navigation-target-not-observed");
        }
        if (workerMode()) {
            workerBefore_ = observation;
            if (workerSubmission_.accepted && worker_ && workerFailure_.isEmpty() && observation.observed && observation.ready) {
                // finished is queued and may already be pending. Never resample.
                if (worker_->snapshot().finished) workerFinished();
                return;
            }
            if (workerFailure_.isEmpty()) workerFailure_ = observation.refusal.isEmpty()
                ? QStringLiteral("worker-source-unobserved") : observation.refusal;
            if (worker_) workerResult_ = worker_->snapshot();
            finish(QStringLiteral("worker-witness-refused"), observation); return;
        }
        finish(observation.observed ? QStringLiteral("observed") : QStringLiteral("observation-refused"), observation);
    }
    void submitWorker(QObject *creation, const QVariant &id, QObject *source, const Observation &before,
                      const std::function<bool()> &guard) {
        if (!worker_ || !guard || !guard() || !before.observed || !before.creationJoinObserved || !before.ready ||
            before.documentId != navigation_.documentId || before.pageIndex != navigation_.pageIndex) {
            workerFailure_ = QStringLiteral("worker-target-refused"); return;
        }
        QMetaMethod completion; int matches = 0;
        const QMetaObject *meta = source ? source->metaObject() : nullptr;
        if (!meta || meta->methodCount() > 512) { workerFailure_ = QStringLiteral("worker-completion-contract"); return; }
        for (int i = 0; i < meta->methodCount(); ++i) {
            const auto method = meta->method(i);
            if (method.name() == "jobCompleted") { ++matches; completion = method; }
        }
        if (matches != 1 || completion.methodType() != QMetaMethod::Signal || completion.parameterCount() != 1 ||
            completion.parameterMetaType(0) != QMetaType::fromType<qulonglong>() ||
            completion.returnMetaType() != QMetaType::fromType<void>() ||
            qstrcmp(completion.enclosingMetaObject()->className(), "DocumentWorker") != 0 || !guard()) {
            workerFailure_ = QStringLiteral("worker-completion-contract"); return;
        }
        const QPointer<Entry> weak(this);
#ifdef READER_SELECTED_TESTING
        const auto testSubmit = workerSubmitTest_;
        if (!testSubmit) {
#endif
            auto loading = reader_worker_native::loadPinned(guard);
            if (!weak) return;
            if (!loading.bridge) { workerFailure_ = loading.refusal; return; }
            workerBridge_ = std::move(loading.bridge);
            auto preparing = workerBridge_->prepare(creation, id, source, guard);
            if (!weak) return;
            if (!preparing.prepared) { workerFailure_ = preparing.refusal; workerSubmission_.unknown = preparing.unknown; return; }
            workerPrepared_ = std::move(preparing.prepared); workerPreparedOnce_ = true;
#ifdef READER_SELECTED_TESTING
        } else {
            workerPreparedOnce_ = true;
            workerReleaseOk_ = true; // Owned model has no native leases; adapter tested separately.
        }
#endif
        const int remaining = int(operationEnd_ - now());
        if (remaining < 1 || !guard() || !weak) { if (weak) workerFailure_ = QStringLiteral("deadline"); return; }
        worker_->start(source, completion, [weak, &guard
#ifdef READER_SELECTED_TESTING
                , testSubmit, source
#endif
                ](const std::function<void()> &callback) -> qulonglong {
            if (!weak || !guard()) return 0;
#ifdef READER_SELECTED_TESTING
            if (testSubmit) {
                weak->workerSubmission_.attempted = true;
                try {
                    const auto job = testSubmit(source, callback);
                    if (weak) { weak->workerSubmission_.jobId = job; weak->workerSubmission_.accepted = job != 0; }
                    return job;
                } catch (...) { if (weak) weak->workerSubmission_.unknown = true; throw; }
            }
#endif
            const auto submitted = weak->workerPrepared_->submit(callback, guard);
            if (weak) weak->workerSubmission_ = submitted;
            return submitted.accepted ? submitted.jobId : 0;
        }, remaining);
        // The guarded function borrows Sample's stack and is destroyed by start
        // before returning. Only the witness's atomic closure is queued natively.
        if (weak && !workerSubmission_.accepted && workerFailure_.isEmpty())
            workerFailure_ = workerSubmission_.refusal.isEmpty() ? QStringLiteral("worker-submission-refused") : workerSubmission_.refusal;
    }
    void creationFinished() {
        if (terminal_ || disarmed_ || sampling_ || !creationInitialUnwound_ ||
            creationPhase_ != CreationPhase::Initial || creationPhasePending_) return;
        if (!permit()) { finishStopped(); return; }
#ifdef READER_SELECTED_TESTING
        if (!creationSubmitTest_)
#endif
            if (creation_) creationResult_ = creation_->snapshot();
        if (!creationResult_.finished) return;
        if (!creationResult_.attempted || !creationResult_.returnKnown || !creationResult_.accepted ||
            creationResult_.unknown || !creationResult_.callbackObserved || creationResult_.duplicate ||
            creationResult_.canceled || creationResult_.callbackCount != 1 || !creationResult_.refusal.isEmpty()) {
            creationFailure_ = creationResult_.refusal.isEmpty() ? QStringLiteral("creation-completion-refused") : creationResult_.refusal;
            finish(QStringLiteral("creation-refused")); return;
        }
        scheduleCreationPhase(CreationPhase::BeforeNext);
    }
    void scheduleCreationPhase(CreationPhase phase) {
        if (!permit()) { finishStopped(); return; }
        if (operationEnd_ - now() <= 2000) {
            creationFailure_ = QStringLiteral("deadline"); finish(QStringLiteral("creation-refused")); return;
        }
        creationPhase_ = phase; creationPhasePending_ = true; creationTimer_.start(2000);
    }
    void afterCreationOpportunity() {
        if (terminal_ || disarmed_ || sampling_ || !creationPhasePending_) return;
        creationPhasePending_ = false;
        if (!permit()) { finishStopped(); return; }
        QPointer<QQuickWindow> window = qobject_cast<QQuickWindow *>(QGuiApplication::focusWindow());
        if (!window || !operationEngine_ || qmlEngine(window) != operationEngine_ || window != creationWindow_) {
            creationFailure_ = QStringLiteral("creation-context-changed"); finish(QStringLiteral("creation-refused")); return;
        }
        sampleCreation(window);
    }
    void creationNext(QObject *reader, QObject *document, const Observation &before,
                      const std::function<bool()> &guard) {
        const QPointer<Entry> weak(this);
        const auto allowed = [&] { return weak && guard && guard() && weak; };
        auto refuse = [&] (const char *why) { if (weak) weak->creationFailure_ = QString::fromLatin1(why); };
        if (!allowed()) { refuse("creation-source-disarmed"); return; }
        const bool sourceSelected = before.pageId == creationRequest_.sourcePageId && before.pageIndex == creationRequest_.sourceIndex;
        const bool targetSelected = before.pageId == creationRequest_.targetPageId && before.pageIndex == creationRequest_.sourceIndex + 1;
        if (!before.observed || !before.ready || !before.creationJoinObserved ||
            before.documentId != creationRequest_.documentId || (!sourceSelected && !targetSelected) ||
            before.pageCount != creationRequest_.expectedPages.size() + 1 ||
            creationRequest_.sourceIndex + 1 >= before.pageCount || creationNextAttempted_) {
            refuse("creation-source-not-selected"); return;
        }
        QStringList expected = creationRequest_.expectedPages;
        expected.insert(creationRequest_.sourceIndex + 1, creationRequest_.targetPageId);
        QPointer<QObject> heldReader(reader), heldDocument(document);
        for (int i = 0; i < expected.size(); ++i) {
            if (!allowed() || !heldDocument || !heldReader) { refuse("creation-order-disarmed"); return; }
            QString id; int roundtrip = -1;
            const bool read = QMetaObject::invokeMethod(heldDocument, "idForPage", Qt::DirectConnection,
                Q_RETURN_ARG(QString, id), Q_ARG(int, i));
            if (!allowed() || !heldDocument || !heldReader || !read || id != expected[i]) { refuse("creation-order-refused"); return; }
            const bool reverse = QMetaObject::invokeMethod(heldDocument, "pageForId", Qt::DirectConnection,
                Q_RETURN_ARG(int, roundtrip), Q_ARG(QString, id));
            if (!allowed() || !heldDocument || !heldReader || !reverse || roundtrip != i) { refuse("creation-order-refused"); return; }
        }
        if (!allowed() || !heldReader || !heldDocument) { refuse("creation-order-disarmed"); return; }
        creationOrderObserved_ = true;
        if (targetSelected) { creationTargetAlreadySelected_ = true; return; }
        const QMetaObject *meta = heldReader->metaObject();
        if (!meta || meta->methodCount() > 512) { refuse("creation-next-contract"); return; }
        QMetaMethod next; int matches = 0;
        for (int i = 0; i < meta->methodCount(); ++i) {
            const auto method = meta->method(i);
            if (method.name() == "moveForward") { ++matches; next = method; }
        }
        if (matches != 1 || next.parameterCount() != 0 || next.methodType() == QMetaMethod::Signal ||
            (next.returnMetaType() != QMetaType::fromType<void>() && next.returnMetaType() != QMetaType::fromType<QVariant>()) ||
            !allowed() || !heldReader || !heldDocument) { refuse("creation-next-contract"); return; }
        creationOrderObserved_ = true; creationSourceStillSelected_ = true;
        creationNextAttempted_ = true; // Consume before invoking; never inspect the old epoch afterward.
        try {
            const bool dispatched = next.invoke(heldReader, Qt::DirectConnection);
            if (weak) { weak->creationNextDispatched_ = dispatched; weak->creationNextUnknown_ = !dispatched; }
        } catch (...) {
            if (weak) { weak->creationNextUnknown_ = true; weak->creationFailure_ = QStringLiteral("creation-next-threw"); }
        }
    }
    void sampleCreation(QQuickWindow *window) {
        const QPointer<Entry> weak(this);
        if (creationPhase_ == CreationPhase::Initial && !creationWindow_) {
            creationWindow_ = window;
            connect(creationWindow_, &QObject::destroyed, this, [weak] {
                if (weak && !weak->terminal_) {
                    weak->creationFailure_ = QStringLiteral("creation-context-changed");
                    weak->finish(QStringLiteral("creation-refused"));
                }
            });
        }
        const qint64 remaining = operationEnd_ - now();
        if (remaining < 1) { creationFailure_ = QStringLiteral("deadline"); finish(QStringLiteral("creation-refused")); return; }
        const auto allowed = [weak] {
            return weak && weak->permit() && weak->creationWindow_ && QGuiApplication::focusWindow() == weak->creationWindow_ &&
                qmlEngine(weak->creationWindow_) == weak->operationEngine_ && weak->now() < weak->operationEnd_;
        };
        if (creationPhase_ == CreationPhase::Initial && !creation_) {
            creation_ = std::make_unique<reader_joined_creation::Witness>(this);
            connect(creation_.get(), &reader_joined_creation::Witness::finished, this, [weak] {
                if (weak) weak->creationFinished();
            }, Qt::QueuedConnection);
#ifdef READER_SELECTED_TESTING
            if (!creationSubmitTest_)
#endif
            {
                const bool prepared = creation_->prepare(operationEngine_);
                if (!weak) return;
                if (!prepared) { creationFailure_ = QStringLiteral("creation-callback-prepare-refused"); finish(QStringLiteral("creation-refused")); return; }
            }
        }
        ValidatedSourceAction action;
        if (creationPhase_ == CreationPhase::Initial) {
            action = [weak](QObject *, QObject *document, QObject *controller, const QVariant &id,
                    const Observation &before, const std::function<bool()> &guard) {
                if (!weak) return;
                weak->creationBefore_ = before;
#ifdef READER_SELECTED_TESTING
                const auto test = weak->creationSubmitTest_;
                if (test) {
                    const auto result = test(controller, document, id, before, weak->creationRequest_, guard);
                    if (weak) weak->creationResult_ = result;
                    return;
                }
#endif
                weak->creation_->submit(controller, document, id, before, weak->creationRequest_, guard,
                    int(weak->operationEnd_ - weak->now()));
                if (weak) weak->creationResult_ = weak->creation_->snapshot();
            };
        } else if (creationPhase_ == CreationPhase::BeforeNext) {
            action = [weak](QObject *reader, QObject *document, QObject *, const QVariant &,
                    const Observation &before, const std::function<bool()> &guard) {
                if (weak) weak->creationNext(reader, document, before, guard);
            };
        }
        sampling_ = true;
#ifdef READER_SELECTED_TESTING
        ++samples_;
#endif
        auto observation = observeCreationJoin(window, allowed, qMin<int>(MaxObservationMs, int(operationEnd_ - now())), {}, std::move(action));
        if (!weak) return;
        sampling_ = false; sampleEnd_ = now();
        if (!permit()) { finishStopped(); return; }
        if (now() >= operationEnd_ || !observation.observed || !observation.ready || !observation.creationJoinObserved ||
            observation.documentId != creationRequest_.documentId || !creationFailure_.isEmpty()) {
            if (creationFailure_.isEmpty()) creationFailure_ = now() >= operationEnd_ ? QStringLiteral("deadline") :
                observation.refusal.isEmpty() ? QStringLiteral("creation-source-unobserved") : observation.refusal;
            finish(QStringLiteral("creation-refused")); return;
        }
        if (creationPhase_ == CreationPhase::Initial) {
            creationInitialUnwound_ = true;
            if (observation.pageId != creationRequest_.sourcePageId || observation.pageIndex != creationRequest_.sourceIndex ||
                observation.pageCount != creationRequest_.expectedPages.size()) {
                creationFailure_ = QStringLiteral("creation-source-mismatch"); finish(QStringLiteral("creation-refused")); return;
            }
            creationFinished(); return;
        }
        if (creationPhase_ == CreationPhase::BeforeNext) {
            if (creationTargetAlreadySelected_ && creationOrderObserved_ && !creationNextAttempted_) {
                finish(QStringLiteral("creation-selected-observed"), observation); return;
            }
            if (!creationNextDispatched_ || creationNextUnknown_ || !creationOrderObserved_ || !creationSourceStillSelected_) {
                creationFailure_ = QStringLiteral("creation-next-refused"); finish(QStringLiteral("creation-refused")); return;
            }
            scheduleCreationPhase(CreationPhase::AfterNext); return;
        }
        if (observation.pageId != creationRequest_.targetPageId || observation.pageIndex != creationRequest_.sourceIndex + 1 ||
            observation.pageCount != creationRequest_.expectedPages.size() + 1) {
            creationFailure_ = QStringLiteral("creation-target-not-selected"); finish(QStringLiteral("creation-refused")); return;
        }
        finish(QStringLiteral("creation-selected-observed"), observation);
    }
    bool captureContext() const {
        return app_ && !QCoreApplication::closingDown() && QThread::currentThread() == app_->thread() &&
            captureWindow_ && captureRoot_ && operationEngine_ &&
            captureWindow_->thread() == app_->thread() && captureRoot_->thread() == app_->thread() &&
            operationEngine_->thread() == app_->thread() && QGuiApplication::focusWindow() == captureWindow_ &&
            captureWindow_->isActive() && captureWindow_->isVisible() && captureWindow_->contentItem() == captureRoot_ &&
            qmlEngine(captureWindow_) == operationEngine_;
    }
    // Separate publication phase; observation permit() remains retired.
    bool capturePublicationAllowed() const {
        const QPointer<const Entry> weak(this);
        try {
#ifdef READER_SELECTED_TESTING
            const auto test = capturePublicationTest_;
            if (test) test();
            if (!weak) return false;
#endif
            return weak && weak->capturePublishing_ && weak->terminal_ && !weak->suppressed_ && weak->consumed_ && weak->captureContext() &&
                weak->now() < SetupWindowMs && weak->operationEnd_ > 0 && weak->now() < weak->operationEnd_ && weak->trusted();
        } catch (...) { return false; }
    }
    static QJsonObject captureObservation(const Observation &r) {
        return {{"observed",r.observed},{"map_coherent",r.mapCoherent},{"creation_join",r.creationJoinObserved},{"ready",r.ready},
            {"document_id",r.documentId},{"page_id",r.pageId},{"index",r.pageIndex},{"count",r.pageCount},
            {"status",r.status},{"page_status",r.pageStatus},{"loading",r.loading},{"page_error",r.pageError},
            {"document_error",r.documentError},{"awaiting_tiles",r.awaitingTiles},{"pending_tiles",r.pendingTiles},
            {"working",r.working},{"refusal",r.refusal}};
    }
    bool captureResultValid() const {
        const auto &r = captureResult_;
        if (!r.captured || !r.attempted || r.unknown || !r.refusal.isEmpty() || !captureClaim_ || !captureClaim_->consumed.load() ||
            !reader_identity_capture::detail::matches(r.before, captureExpected_) ||
            !reader_identity_capture::detail::matches(r.after, captureExpected_) ||
            r.width <= 0 || r.width > 1872 || r.height <= 0 || r.height > 1872 ||
            qint64(r.width) * r.height > visual_capture::PixelCap || r.storage <= 0 || r.storage > visual_capture::StorageCap ||
            !std::isfinite(r.dpr) || r.dpr <= 0 || r.encoded <= 0 || r.encoded > visual_capture::PngCap ||
            r.encoded != captureOutputBytes_ || r.sha.size() != 64 || r.sha != captureOutputSha_) return false;
        for (char c : r.sha) if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f'))) return false;
        return true;
    }
    void finishCapture(QString stage, bool allowResult, QString failure = {}) {
        // Shutdown suppression remains sticky during terminal publication too.
        if (!allowResult) suppressed_ = true;
        if (terminal_) return;
        const QPointer<Entry> weak(this);
        disarm();
        if (!weak) return;
        if (sampling_) {
            if (pendingStage_.isEmpty() || stage == QStringLiteral("setup-expired")) pendingStage_ = std::move(stage);
            return;
        }
        terminal_ = true;
        capturePublishing_ = true;
        const auto files = captureFiles_; // Own duplicated directory/FD across Entry destruction.
        const auto admitted = [weak] { return weak && weak->capturePublicationAllowed(); };
        if (!files || !admitted()) { if (weak) { weak->capturePublishing_ = false; weak->captureFiles_.reset(); } return; }
        if (!pendingStage_.isEmpty()) { stage = pendingStage_; if (failure.isEmpty()) failure = QStringLiteral("observation-disarmed"); }
        if (captureResult_.captured && !captureResultValid()) { stage = QStringLiteral("helper-result-refused"); failure = QStringLiteral("helper-result-contract"); }
        reader_capture_files::Result image;
        if (pendingStage_.isEmpty() && captureResultValid() && captureSeal_.ok && failure.isEmpty()) {
            image = files->commitImage(admitted);
            if (!weak) return;
            if (!image.ok) { stage = QStringLiteral("image-publication-refused"); failure = image.refusal; }
        }
        if (!admitted()) { if (weak) { weak->capturePublishing_ = false; weak->captureFiles_.reset(); } return; }
        const bool accepted = navigationAttempted_ && navigationDispatched_ && captureResultValid() && captureSeal_.ok &&
            image.committed && pendingStage_.isEmpty() && failure.isEmpty();
        const auto identity = identity_.trimmed().split(' ');
        if (identity.size() != 2) { capturePublishing_ = false; captureFiles_.reset(); return; }
        if (!accepted && failure.isEmpty()) failure = captureFailure_.isEmpty() ? captureResult_.refusal : captureFailure_;
        QJsonObject body{{"schema",1},{"diagnostic",QStringLiteral("identity-capture")},{"nonce",QString::fromLatin1(nonce_)},
            {"pid",QString::fromLatin1(identity[0])},{"start",QString::fromLatin1(identity[1])},
            {"stage",accepted ? QStringLiteral("identity-image-observed") : stage},{"refusal",failure},
            {"accepted",accepted},{"attempted",captureResult_.attempted},{"captured",captureResult_.captured},{"unknown",captureResult_.unknown},
            {"navigation_attempted",navigationAttempted_},{"navigation_dispatched",navigationDispatched_},
            {"navigation_scope",QStringLiteral("legacy-main-composition")},
            {"expected",QJsonObject{{"document_id",captureExpected_.documentId},{"page_id",captureExpected_.pageId},
                {"index",captureExpected_.pageIndex},{"count",captureExpected_.pageCount}}},
            {"before",captureObservation(captureResult_.before)},{"after",captureObservation(captureResult_.after)},
            {"sample_begin_ms",sampleBegin_},{"sample_end_ms",sampleEnd_},{"capture_begin_ms",captureBegin_},{"capture_end_ms",captureEnd_},
            {"publication_ms",now()},{"operation_end_ms",operationEnd_},{"setup_limit_ms",SetupWindowMs},
            {"image_committed",image.committed},{"image_linked",image.linked},{"image",QStringLiteral("visual-image.png")},
            {"width",captureResult_.width},{"height",captureResult_.height},{"storage_bytes",captureResult_.storage},
            {"dpr",captureResult_.dpr},{"encoded_bytes",captureResult_.encoded},{"output_bytes",captureOutputBytes_},
            {"sha256",QString::fromLatin1(captureResult_.sha)},{"acquisition",QStringLiteral("QQuickWindow::grabWindow")},
            {"coordinates",QStringLiteral("full-window")},{"identity_semantics",QStringLiteral("sampled-before-around-after")},
            {"content_correspondence",QStringLiteral("unverified")},{"physical_freshness",QStringLiteral("unverified")},
            {"native_authority",false},{"render_authority",false},{"durable_authority",false},{"atomic_authority",false}};
        const auto bytes = QJsonDocument(body).toJson(QJsonDocument::Compact) + '\n';
        files->publishScalar(bytes, accepted ? reader_capture_files::ScalarKind::AcceptedPair : reader_capture_files::ScalarKind::Diagnostic, admitted);
        if (weak) { weak->capturePublishing_ = false; weak->captureFiles_.reset(); }
    }
    void sampleCapture(QQuickWindow *window) {
        const QPointer<Entry> weak(this);
        const auto files = captureFiles_;
        const auto expected = captureExpected_;
        const auto claim = captureClaim_;
        const auto admitted = [weak] {
            return weak && weak->permit() && weak && weak->captureContext() && weak->now() < weak->operationEnd_;
        };
        if (!files || window != captureWindow_ || !admitted()) { if (weak) weak->finish(QStringLiteral("capture-context-refused")); return; }
        sampling_ = true;
        const auto opened = files->openImage(admitted);
        if (!weak) return;
        if (!opened.ok) { sampling_ = false; captureFailure_ = opened.refusal; sampleEnd_ = now(); finish(QStringLiteral("image-open-refused")); return; }
        reader_identity_capture::Result result;
        qint64 output = 0;
        QByteArray outputSha;
        {
            visual_capture::Sink sink(files->imageFd()); // Files owns FD; local Sink dies before sealing.
            if (!weak) return;
            captureBegin_ = now();
            const int budget = int(qMax<qint64>(0, qMin<qint64>(20000, operationEnd_ - now())));
            try {
#ifdef READER_SELECTED_TESTING
                const auto test = captureRunTest_;
                if (test) result = test(window, expected, sink, claim, admitted, budget);
                else
#endif
                    result = reader_identity_capture::run(window, expected, sink, claim, admitted, budget);
            } catch (...) {
                result.attempted = claim && claim->consumed.load(); result.unknown = result.attempted;
                result.refusal = QStringLiteral("capture-exception");
            }
            output = sink.count();
            outputSha = sink.digest();
        }
        if (!weak) return;
        captureEnd_ = sampleEnd_ = now();
        captureResult_ = std::move(result); captureOutputBytes_ = output; captureOutputSha_ = std::move(outputSha);
        const auto sealed = files->sealImage(output, admitted); // Closes even an invalid/partial output FD.
        if (!weak) return;
        captureSeal_ = sealed;
        if (!sealed.ok && captureResult_.captured) captureFailure_ = sealed.refusal;
        sampling_ = false;
        finish(captureResult_.captured && sealed.ok ? QStringLiteral("identity-image-observed") : QStringLiteral("identity-image-refused"));
    }
    void workerFinished() {
        if (terminal_ || disarmed_ || !worker_ || sampling_) return;
        if (!permit()) { finishStopped(); return; }
        workerResult_ = worker_->snapshot(); sampleEnd_ = now();
        if (!workerResult_.finished) return;
        const bool ok = workerSubmission_.accepted && workerBefore_.observed && workerBefore_.ready &&
            workerBefore_.creationJoinObserved && workerResult_.deliveryObserved && workerFailure_.isEmpty() && now() < operationEnd_;
        if (!ok && workerFailure_.isEmpty()) workerFailure_ = now() >= operationEnd_
            ? QStringLiteral("deadline") : workerResult_.refusal;
        finish(ok ? QStringLiteral("worker-witness-observed") : QStringLiteral("worker-witness-refused"), workerBefore_);
    }
    QPointer<QGuiApplication> app_;
    QString path_, pendingStage_;
    QByteArray nonce_, identity_;
    Diagnostic diagnostic_;
    FixtureNavigation navigation_;
    NavigationOwnership ownership_;
    QPointer<QQmlEngine> operationEngine_;
    std::unique_ptr<WorkerCallbackWitness> worker_;
    std::unique_ptr<reader_worker_native::Bridge> workerBridge_;
    std::unique_ptr<reader_worker_native::Prepared> workerPrepared_;
    reader_worker_native::Submission workerSubmission_;
    reader_joined_creation::Request creationRequest_;
    reader_identity_capture::Expected captureExpected_;
    reader_identity_capture::Result captureResult_;
    reader_capture_files::Result captureSeal_;
    std::shared_ptr<reader_capture_files::Files> captureFiles_;
    std::shared_ptr<reader_identity_capture::Claim> captureClaim_;
    QPointer<QQuickWindow> captureWindow_;
    QPointer<QQuickItem> captureRoot_;
    QString captureFailure_;
    QByteArray captureOutputSha_;
    qint64 captureBegin_ = -1, captureEnd_ = -1, captureOutputBytes_ = 0;
    bool capturePublishing_ = false;
    reader_joined_creation::Result creationResult_;
    std::unique_ptr<reader_joined_creation::Witness> creation_;
    QPointer<QQuickWindow> creationWindow_;
    Observation creationBefore_;
    QString creationFailure_;
    CreationPhase creationPhase_ = CreationPhase::Initial;
    bool creationOrderObserved_ = false, creationSourceStillSelected_ = false;
    bool creationTargetAlreadySelected_ = false;
    bool creationNextAttempted_ = false, creationNextDispatched_ = false, creationNextUnknown_ = false;
    bool creationPhasePending_ = false, creationInitialUnwound_ = false;
    WorkerCallbackResult workerResult_;
    Observation workerBefore_;
    QString workerFailure_;
    bool workerPreparedOnce_ = false, workerReleaseOk_ = false;
    int root_ = -1, checks_ = 0;
    struct stat rootStat_{}, requestStat_{};
    QElapsedTimer elapsed_;
    QFileSystemWatcher watcher_;
    QTimer timer_;
    QTimer navigationTimer_;
    QTimer creationTimer_;
    static constexpr int NavigationDelayMs = 2000;
    static constexpr int LegacyNavigationDelayMs = 5000;
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
    std::function<qulonglong(QObject *, const std::function<void()> &)> workerSubmitTest_;
    CreationSubmitTest creationSubmitTest_;
    QString creationWirePaddingTest_;
    CaptureRunTest captureRunTest_;
    std::function<void()> capturePublicationTest_;
#endif
};
} // namespace reader_selected
