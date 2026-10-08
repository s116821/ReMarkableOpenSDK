#pragma once
#include "qt_page_facts.h"
#include "qt_page_facts_refusal.h"
#include <QFileSystemWatcher>
#include <QTimer>
#include <QEvent>
#include <memory>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include "qt_input_observation.h"
#include <QCryptographicHash>

namespace qml_access {
struct FactsEntryConfig {
    QString nonce, directory;
    PageFactsConfig facts;
    int setupBudgetMs=20000;
    bool developmentSetup120=false;
    bool developmentInputObservation=false;
    bool developmentCaptureObservation=false;
    QString setupSelection;
    bool setupValid() const {
        return developmentSetup120 ? setupBudgetMs==120000 && setupSelection==
            (developmentInputObservation ? QStringLiteral("main-dev-input-observation-120s"):QStringLiteral("main-dev-facts-120s")) :
            setupBudgetMs>=1 && setupBudgetMs<=20000 && setupSelection.isEmpty();
    }
    QByteArray setupProfile() const {
        return developmentInputObservation ? QByteArray("main-dev-input-observation-120s"):
            developmentSetup120 ? QByteArray("main-dev-facts-120s") : QByteArray("default-dev-20s");
    }
};
struct FactsEntryResult {
    QString stage;
    bool observed=false, outputPublished=false;
};
// Fixed read-only development entry. Caller retains this parentless object
// through completion. No controller, native mutator or general command endpoint.
class FactsEntry final : public QObject {
    Q_OBJECT
    friend struct InputObservationFixtureAccess;
    friend struct CaptureObservationFixtureAccess;
public:
    FactsEntry(QGuiApplication *app, FactsEntryConfig config,
               std::function<void(FactsEntryResult)> completed,
               std::function<qint64()> clock={})
        : app_(app), config_(std::move(config)), completed_(std::move(completed)), clock_(std::move(clock)) {}
    ~FactsEntry() override {
        if (app_) app_->removeEventFilter(this);
        reader_.reset();
        for (int fd:{captureTokenFd_,capturePngFd_,captureCompleteFd_}) if (fd>=0) ::close(fd);
        if (root_>=0) ::close(root_);
    }
    void start() {
        const Scope scope(this);
        if (started_) { cancel(); return; }
        started_=true; elapsed_.start(); origin_=clock_ ? clock_() : 0;
        if (!app_ || app_->thread()!=thread() || QThread::currentThread()!=thread() ||
            !QRegularExpression(QStringLiteral("^[0-9a-f]{32}$")).match(config_.nonce).hasMatch() ||
            !config_.facts.valid() || !config_.setupValid() || !completed_ ||
            (config_.developmentInputObservation && (!config_.developmentSetup120 || config_.facts.budgetMs!=5000))) {
            finish("facts-entry-config-refused"); return;
        }
        if (config_.developmentCaptureObservation && (!config_.developmentSetup120 || config_.facts.budgetMs!=5000 || config_.developmentInputObservation)) {
            finish("capture-observation-config-refused"); return;
        }
        root_=::open(config_.directory.toUtf8().constData(),O_RDONLY|O_DIRECTORY|O_NOFOLLOW|O_CLOEXEC);
        if (root_<0 || ::fstat(root_,&rootStat_)!=0 || !directoryMode(rootStat_) || !rootCurrent()) {
            finish("facts-entry-directory-refused"); return;
        }
        if (exists("facts-waiting") || exists("facts-request") || exists("facts-request.tmp") ||
            exists("callback.json") || exists("diagnostics.json") || exists("refusal.json") ||
            (config_.developmentInputObservation && (exists("input-observation-ready") || exists("input-observation-end") ||
             exists("input-observation-end.tmp") || exists("input-observation-complete.json") || exists("input-window.png")))) { finish("facts-entry-stale-refused"); return; }
        process_=QByteArray::number(::getpid());
        if (config_.developmentCaptureObservation && (exists("capture-observation-request") || exists("capture-observation-request.tmp") ||
            exists("capture-observation-complete.json") || exists("capture-window.png") || exists("input-observation-end") || exists("input-observation-end.tmp"))) {
            finish("capture-observation-stale-refused"); return;
        }
        processStart_=processStart();
        if (processStart_.isEmpty() || readFile("attempt.identity",128)!=process_+' '+processStart_+'\n') {
            finish("facts-entry-generation-refused"); return;
        }
        connect(app_,&QCoreApplication::aboutToQuit,this,&FactsEntry::cancel);
        connect(&watcher_,&QFileSystemWatcher::directoryChanged,this,[this]{queueRequest();});
        if (!watcher_.addPath(config_.directory)) { finish("facts-entry-watch-refused"); return; }
        app_->installEventFilter(this);
        timer_.setSingleShot(true); timer_.setTimerType(Qt::PreciseTimer);
        connect(&timer_,&QTimer::timeout,this,&FactsEntry::checkDeadline);
        timer_.start(config_.setupBudgetMs);
        queueBootstrap();
    }
    static QByteArray processStart() {
        const int fd=::open("/proc/self/stat",O_RDONLY|O_CLOEXEC);
        if (fd<0) return {};
        char bytes[2048]; const ssize_t length=::read(fd,bytes,sizeof bytes); ::close(fd);
        if (length<=0 || length>=ssize_t(sizeof bytes)) return {};
        const QByteArray text(bytes,int(length)); const int end=text.lastIndexOf(')');
        if (end<0) return {};
        const auto fields=text.mid(end+2).simplified().split(' ');
        const QByteArray value=fields.value(19); // suffix starts at stat field 3
        for (const char digit:value) if (digit<'0' || digit>'9') return {};
        return value;
    }
public slots:
    void cancel() { closed_=true; if (!reading_) finish("facts-entry-canceled"); }
    void checkDeadline() {
        const Scope scope(this);
        if (done_) return;
        const qint64 deadline=captureReading_ ? qMin(qint64(config_.setupBudgetMs),captureAcceptedAt_+5000) :
            acceptedAt_<0 ? config_.setupBudgetMs : acceptedAt_+config_.facts.budgetMs;
        if (now()>=deadline) {
            closed_=true;
            if (!reading_) finish(acceptedAt_<0 ? "facts-entry-setup-deadline" : "facts-entry-access-deadline");
        } else timer_.start(int(deadline-now()));
    }
    void invalidateCapture() { captureInvalid_=true; ++captureEpoch_; }
    bool captureAllowed() {
        const Scope scope(this);
        if (captureChecking_) { invalidateCapture(); return false; }
        captureChecking_=true;
        const bool valid=!captureInvalid_ && context() && captureLife() && activeOwner(captureOwner_,engine_);
        captureChecking_=false;
        return valid && !captureInvalid_ && context() && captureLife() && captureOwner_.window && captureOwner_.receiver &&
            captureOwner_.scene && captureOwner_.document && captureOwner_.window->thread()==thread() &&
            captureOwner_.receiver->thread()==thread() && captureOwner_.scene->thread()==thread() && captureOwner_.document->thread()==thread() &&
            (captureReading_ ? now()<qMin(qint64(config_.setupBudgetMs),captureAcceptedAt_+5000) :
             acceptedAt_<0 ? now()<config_.setupBudgetMs : now()<acceptedAt_+config_.facts.budgetMs);
    }
protected:
    bool eventFilter(QObject *object,QEvent *event) override {
        if (config_.developmentCaptureObservation && captureWatching_) {
            switch (event->type()) {
            case QEvent::TouchBegin: case QEvent::TouchUpdate: case QEvent::TouchEnd: case QEvent::TouchCancel:
            case QEvent::MouseButtonPress: case QEvent::MouseButtonRelease: case QEvent::MouseButtonDblClick: case QEvent::MouseMove:
            case QEvent::TabletPress: case QEvent::TabletMove: case QEvent::TabletRelease:
            case QEvent::Wheel: case QEvent::KeyPress: case QEvent::KeyRelease: invalidateCapture(); break;
            default: break;
            }
        }
        if (config_.developmentInputObservation && observationReady_ && !done_)
            observation_.observe(object,event,observationWindow_,now());
        if (!done_ && !engine_ && qobject_cast<QWindow *>(object) &&
            (event->type()==QEvent::Show || event->type()==QEvent::Expose || event->type()==QEvent::FocusIn))
            queueBootstrap();
        return false;
    }
private:
    struct Scope {
        FactsEntry *entry;
        explicit Scope(FactsEntry *value):entry(value){++entry->depth_;}
        ~Scope(){if (--entry->depth_==0) entry->queueCompletion();}
    };
    qint64 now() const { return clock_ ? clock_()-origin_ : elapsed_.elapsed(); }
    static bool directoryMode(const struct stat &st) {
        return S_ISDIR(st.st_mode) && st.st_uid==::geteuid() && (st.st_mode&0777)==0700;
    }
    bool exists(const char *name) const {
        struct stat st{};
        return ::fstatat(root_,name,&st,AT_SYMLINK_NOFOLLOW)==0 || errno!=ENOENT;
    }
    QByteArray readFile(const char *name,int cap) const {
        const int fd=::openat(root_,name,O_RDONLY|O_NOFOLLOW|O_NONBLOCK|O_CLOEXEC);
        if (fd<0) return {};
        struct stat before{},after{};
        QByteArray value(cap+1,Qt::Uninitialized);
        const bool metadata=::fstat(fd,&before)==0 && S_ISREG(before.st_mode) && before.st_uid==::geteuid() &&
            (before.st_mode&0777)==0600 && before.st_size>0 && before.st_size<=cap;
        const ssize_t length=metadata ? ::read(fd,value.data(),size_t(value.size())) : -1;
        const bool stable=::fstat(fd,&after)==0 && before.st_dev==after.st_dev && before.st_ino==after.st_ino &&
            before.st_size==after.st_size && before.st_mode==after.st_mode && before.st_uid==after.st_uid;
        ::close(fd);
        if (!metadata || !stable || length!=before.st_size || length>cap) return {};
        value.resize(int(length)); return value;
    }
    bool rootCurrent() const {
        if (root_<0) return false;
        struct stat held{},path{};
        const int fresh=::open(config_.directory.toUtf8().constData(),O_RDONLY|O_DIRECTORY|O_NOFOLLOW|O_CLOEXEC);
        if (fresh<0) return false;
        const bool valid=::fstat(root_,&held)==0 && ::fstat(fresh,&path)==0 && directoryMode(held) && directoryMode(path) &&
            held.st_dev==rootStat_.st_dev && held.st_ino==rootStat_.st_ino && path.st_dev==held.st_dev && path.st_ino==held.st_ino;
        ::close(fresh);
        return valid && readFile("owner",32)==config_.nonce.toLatin1();
    }
    bool context() const {
        return app_ && QThread::currentThread()==thread() && app_->thread()==thread() &&
            engine_ && engine_->thread()==thread();
    }
    bool live() const {
        return !closed_ && !done_ && rootCurrent() && !exists("entry.closed") && !exists("restore.claim") &&
            readFile("attempt.identity",128)==process_+' '+processStart_+'\n';
    }
    bool readCurrent() const {
        return !done_ && deliveryCurrent();
    }
    bool deliveryCurrent() const {
        return acceptedAt_>=0 && context() && !closed_ && rootCurrent() && !exists("entry.closed") &&
            !exists("restore.claim") && readFile("attempt.identity",128)==process_+' '+processStart_+'\n' &&
            now()<acceptedAt_+config_.facts.budgetMs;
    }
    QByteArray identity(const char *stage) const {
        return config_.nonce.toLatin1()+' '+process_+' '+processStart_+' '+
            QByteArray::number(qulonglong(rootStat_.st_dev))+' '+QByteArray::number(qulonglong(rootStat_.st_ino))+' '+stage;
    }
    bool writeFile(const char *name,const QByteArray &bytes) const {
        if (bytes.isEmpty() || bytes.size()>8192 || !rootCurrent()) return false;
        const int fd=::openat(root_,name,O_WRONLY|O_CREAT|O_EXCL|O_NOFOLLOW|O_CLOEXEC,0600);
        if (fd<0) return false;
        const ssize_t length=::write(fd,bytes.constData(),size_t(bytes.size())); ::close(fd);
        return length==bytes.size() && rootCurrent(); // partial output never retried
    }
    void queueBootstrap() {
        if (done_ || bootstrapQueued_ || engine_) return;
        bootstrapQueued_=true;
        QMetaObject::invokeMethod(this,[this]{
            const Scope scope(this); bootstrapQueued_=false;
            if (done_) return;
            if (!live() || now()>=config_.setupBudgetMs) { finish("facts-entry-bootstrap-refused"); return; }
            if (++bootstrapAttempts_>8) { finish("facts-entry-bootstrap-cap"); return; }
            const auto windows=QGuiApplication::allWindows();
            if (windows.size()>16) { finish("facts-entry-window-cap"); return; }
            QPointer<QQmlEngine> selected;
            for (QWindow *window:windows) {
                QQmlEngine *candidate=qmlEngine(window);
                if (candidate && selected && selected!=candidate) { finish("facts-entry-engine-ambiguous"); return; }
                if (candidate) selected=candidate;
            }
            if (!selected) return; // bounded readiness events only, no document scan
            engine_=selected;
            if (!context()) { finish("facts-entry-engine-refused"); return; }
            connect(engine_,&QObject::destroyed,this,&FactsEntry::cancel);
            if (config_.developmentInputObservation) {
                const auto positiveDecimal=[](const QByteArray &value){
                    if(value.isEmpty() || value.size()>20 || value[0]=='0')return false;
                    for(char digit:value)if(digit<'0' || digit>'9')return false;
                    bool ok=false;value.toULongLong(&ok);return ok;
                };
                if(!positiveDecimal(process_) || !positiveDecimal(processStart_) ||
                   !positiveDecimal(QByteArray::number(qulonglong(rootStat_.st_dev))) ||
                   !positiveDecimal(QByteArray::number(qulonglong(rootStat_.st_ino)))) {
                    finish("input-observation-identity-width-refused");return;
                }
                auto *window=qobject_cast<QQuickWindow *>(QGuiApplication::focusWindow());
                if (!window || !window->isVisible() || !window->isActive() || window->thread()!=thread() || qmlEngine(window)!=engine_) {
                    finish("input-observation-window-refused");return;
                }
                observationWindow_=window;observation_=InputObservationStore{};observationReady_=true;
                const auto ready=identity("waiting-input-observation")+' '+QByteArray::number(now())+" 120000 "+config_.setupProfile()+'\n';
                if (!observationCurrent() || now()>=config_.setupBudgetMs || ready.size()>256 || !writeFile("input-observation-ready",ready)) {
                    finish("input-observation-ready-refused");return;
                }
                queueRequest();return;
            }
            const QByteArray waiting=identity("waiting-facts")+' '+QByteArray::number(now())+' '+
                QByteArray::number(config_.setupBudgetMs)+' '+config_.setupProfile()+'\n';
            if (!live() || now()>=config_.setupBudgetMs || !writeFile("facts-waiting",waiting)) {
                finish("facts-entry-waiting-refused"); return;
            }
            queueRequest();
        },Qt::QueuedConnection);
    }
    void queueRequest() {
        if (done_ || requestQueued_ || consumed_ || !engine_) return;
        requestQueued_=true;
        QMetaObject::invokeMethod(this,[this]{
            const Scope scope(this); requestQueued_=false;
            if (done_ || consumed_) return;
            if (config_.developmentInputObservation) { acceptObservationEnd();return; }
            if (config_.developmentCaptureObservation && !acceptCaptureRequest()) return;
            if (!context() || !live() || now()>=config_.setupBudgetMs) { finish("facts-entry-request-context-refused"); return; }
            if (!exists("facts-request")) return;
            if (config_.developmentCaptureObservation && !captureBindingsCurrent()) { finish("capture-observation-facts-binding-refused");return; }
            consumed_=true; // every existing token consumes the one admission
            if (readFile("facts-request",128)!=identity("read-facts")+' '+QByteArray::number(config_.setupBudgetMs)+' '+
                config_.setupProfile()+'\n' || !context() || !live() || now()>=config_.setupBudgetMs) {
                finish("facts-entry-request-refused"); return;
            }
            acceptedAt_=now();
            timer_.start(config_.facts.budgetMs);
            QMetaObject::invokeMethod(this,[this]{startRead();},Qt::QueuedConnection);
        },Qt::QueuedConnection);
    }
    bool captureLife() const {
        return (!done_ || result_.observed) && !closed_ && rootCurrent() && !exists("entry.closed") && !exists("restore.claim") &&
            readFile("attempt.identity",128)==process_+' '+processStart_+'\n';
    }
    bool captureSignal(QObject *object,const char *signature) {
        if (!object) return false;
        const int signal=object->metaObject()->indexOfSignal(signature),slot=metaObject()->indexOfSlot("invalidateCapture()");
        if (signal<0 || slot<0 || object->metaObject()->method(signal).returnMetaType()!=QMetaType::fromType<void>()) return false;
        return bool(QObject::connect(object,object->metaObject()->method(signal),this,metaObject()->method(slot)));
    }
    bool captureObservers() {
        for (const auto *signal:{"pageCountChanged(int,int)","pageMapChanged()","pageAdded(int)","pagesAdded(QList<int>)",
            "pageMoved(int,int)","pagesMoved()","pagesRemoved()","redirectionPageMapChanged()","pageUpdated(int)","documentMetadataChanged()","orientationChanged()"})
            if (!captureSignal(captureOwner_.document,signal)) return false;
        for (const auto *signal:{"pageIdChanged()","documentWrapperChanged()","workerChanged()","viewportChanged()"})
            if (!captureSignal(captureOwner_.scene,signal)) return false;
        for (const auto *name:{"document","currentPage","currentPageId","drawingAreaFocused"}) {
            const int index=captureOwner_.receiver->metaObject()->indexOfProperty(name);
            if (index<0) return false;
            const auto property=captureOwner_.receiver->metaObject()->property(index);
            if (!property.hasNotifySignal() || !captureSignal(captureOwner_.receiver,property.notifySignal().methodSignature().constData())) return false;
        }
        connect(captureOwner_.window,&QWindow::activeChanged,this,&FactsEntry::invalidateCapture);
        connect(captureOwner_.window,&QWindow::visibleChanged,this,&FactsEntry::invalidateCapture);
        connect(captureOwner_.window,&QWindow::widthChanged,this,&FactsEntry::invalidateCapture);
        connect(captureOwner_.window,&QWindow::heightChanged,this,&FactsEntry::invalidateCapture);
        connect(captureOwner_.window,&QWindow::screenChanged,this,&FactsEntry::invalidateCapture);
        for (const auto &item:{captureOwner_.receiver,captureOwner_.scene}) {
            connect(item,&QQuickItem::visibleChanged,this,&FactsEntry::invalidateCapture);
            connect(item,&QQuickItem::enabledChanged,this,&FactsEntry::invalidateCapture);
            connect(item,&QQuickItem::activeFocusChanged,this,&FactsEntry::invalidateCapture);
            connect(item,&QQuickItem::windowChanged,this,&FactsEntry::invalidateCapture);
            connect(item,&QQuickItem::parentChanged,this,&FactsEntry::invalidateCapture);
        }
        for (QObject *object:{static_cast<QObject *>(engine_.data()),static_cast<QObject *>(captureOwner_.window.data()),
            static_cast<QObject *>(captureOwner_.receiver.data()),static_cast<QObject *>(captureOwner_.scene.data()),captureOwner_.document.data()})
            connect(object,&QObject::destroyed,this,&FactsEntry::invalidateCapture);
        captureWatching_=true;return true;
    }
    bool captureIdentity() {
        QVariant result;
        if (!captureAllowed() || !captureHelper_ || !QMetaObject::invokeMethod(captureHelper_,"readIdentity",Qt::DirectConnection,Q_RETURN_ARG(QVariant,result)) ||
            !captureAllowed() || result.metaType()!=QMetaType::fromType<QString>()) return false;
        const auto bytes=result.toString().toUtf8();
        if (bytes.size()>1024 || bytes.startsWith('!')) return false;
        const auto json=QJsonDocument::fromJson(bytes);const auto object=json.object();
        const auto doc=object.value("document").toString(),page=object.value("page").toString();
        const int index=object.value("index").toInt(-1);
        if (!json.isObject() || object.size()!=3 || !PageFactsConfig::canonical(doc) || !PageFactsConfig::canonical(page) || doc!=config_.facts.documentId ||
            index<0 || index>=config_.facts.expectedOrder.size() || page!=config_.facts.expectedOrder[index] || !captureAllowed()) return false;
        if (captureIndex_>=0) return captureIndex_==index && captureDocument_==doc && capturePage_==page;
        captureIndex_=index;captureDocument_=doc;capturePage_=page;return true;
    }
    bool captureTokenCurrent() const {
        struct stat named{},temporary{};
        const bool hasTemporary=::fstatat(root_,"capture-observation-request.tmp",&temporary,AT_SYMLINK_NOFOLLOW)==0;
        const int temporaryError=errno;
        if (captureTemporaryForFixture_) captureTemporaryForFixture_();
        if (hasTemporary && captureTemporaryReleased_) return false;
        if (!hasTemporary && temporaryError==ENOENT) captureTemporaryReleased_=true;
        // The held final inode's bytes below also validate an identical temporary
        // alias. Reopening the alias can race its legitimate publisher unlink.
        const bool temporaryCurrent=hasTemporary ? S_ISREG(temporary.st_mode) && temporary.st_uid==::geteuid() && (temporary.st_mode&0777)==0600 &&
            temporary.st_dev==captureTokenStat_.st_dev && temporary.st_ino==captureTokenStat_.st_ino : temporaryError==ENOENT;
        return ::fstatat(root_,"capture-observation-request",&named,AT_SYMLINK_NOFOLLOW)==0 &&
            named.st_dev==captureTokenStat_.st_dev && named.st_ino==captureTokenStat_.st_ino &&
            readFile("capture-observation-request",256)==captureToken_ && temporaryCurrent &&
            !exists("input-observation-end") && !exists("input-observation-end.tmp");
    }
    bool captureFileCurrent(const char *name,int fd) const {
        struct stat held{},named{};
        return fd>=0 && ::fstat(fd,&held)==0 && ::fstatat(root_,name,&named,AT_SYMLINK_NOFOLLOW)==0 &&
            S_ISREG(held.st_mode) && held.st_uid==::geteuid() && (held.st_mode&0777)==0600 &&
            held.st_dev==named.st_dev && held.st_ino==named.st_ino && held.st_mode==named.st_mode && held.st_size==named.st_size;
    }
    bool captureBindingsCurrent() {
        const Scope scope(this);
        if (!captureDone_ || !captureAllowed() || !captureTokenCurrent() || !captureFileCurrent("capture-window.png",capturePngFd_) ||
            !captureFileCurrent("capture-observation-complete.json",captureCompleteFd_) || readFile("capture-observation-complete.json",8192)!=captureComplete_) return false;
        const auto png=readFile("capture-window.png",8388608);
        return !png.isEmpty() && QCryptographicHash::hash(png,QCryptographicHash::Sha256).toHex()==capturePngHash_ && captureIdentity() && captureAllowed();
    }
    void withdrawCaptureCompletion() {
        if (rootCurrent() && captureFileCurrent("capture-observation-complete.json",captureCompleteFd_))
            ::unlinkat(root_,"capture-observation-complete.json",0);
    }
    bool acceptCaptureRequest() {
        if (exists("input-observation-end") || exists("input-observation-end.tmp") || (!captureDone_ && (exists("facts-request") || exists("facts-request.tmp")))) {
            finish("capture-observation-purpose-refused");return false;
        }
        if (captureConsumed_) {
            if (!captureTokenCurrent()) finish("capture-observation-replaced-refused");
            return captureDone_ && !done_;
        }
        if (!exists("capture-observation-request")) return false;
        captureConsumed_=true;
        captureToken_=identity("capture-observation")+" 120000 main-dev-facts-120s\n";
        captureTokenFd_=::openat(root_,"capture-observation-request",O_RDONLY|O_NOFOLLOW|O_NONBLOCK|O_CLOEXEC);
        if (!context() || !live() || now()>=config_.setupBudgetMs || readFile("capture-observation-request",256)!=captureToken_ ||
            captureTokenFd_<0 || ::fstat(captureTokenFd_,&captureTokenStat_)!=0 || !captureTokenCurrent()) {
            finish("capture-observation-request-refused");return false;
        }
        captureAcceptedAt_=now();captureReading_=true;
        timer_.start(int(qMin(qint64(config_.setupBudgetMs),captureAcceptedAt_+5000)-now()));
        QMetaObject::invokeMethod(this,[this]{captureOwnerWindow();},Qt::QueuedConnection);return false;
    }
    void captureOwnerWindow() {
        const Scope scope(this);
        const auto progress=[this]{return context() && live() && !captureInvalid_ && captureTokenCurrent() &&
            now()<qMin(qint64(config_.setupBudgetMs),captureAcceptedAt_+5000);};
        if (!progress() || QByteArrayView(findPageOwner(engine_,progress,captureOwner_))!=QByteArrayView("open-owner-observed") ||
            !captureObservers() || !captureAllowed()) { finish("capture-observation-owner-refused");return; }
        QQmlComponent component(engine_,this);
        component.setData(R"QML(import QtQml
QtObject {
 property QtObject bridge:null
 property QtObject receiver:null
 property QtObject scene:null
 property QtObject document:null
 function readIdentity() {
  function ok(){return bridge && bridge.captureAllowed() && receiver && scene && document;}
  if(!ok()) return "!context";
  try {
   const doc=String(document.id); if(!ok()) return "!context";
   const index=receiver.currentPage; if(!ok()) return "!context";
   const page=scene.pageId; if(!ok()) return "!context";
   const alias=receiver.currentPageId; if(!ok() || !Number.isInteger(index) || typeof page!=="string" || alias!==page) return "!identity";
   return JSON.stringify({document:doc,page:page,index:index});
  } catch(ignored){return "!exception";}
 }
})QML",QUrl(QStringLiteral("qrc:/OwnedCaptureIdentity.qml")));
        if (component.status()!=QQmlComponent::Ready || !captureAllowed()) { finish("capture-observation-helper-refused");return; }
        captureHelper_=component.create();
        if (!captureHelper_ || !captureAllowed()) { finish("capture-observation-helper-refused");return; }
        QQmlEngine::setObjectOwnership(captureHelper_,QQmlEngine::CppOwnership);captureHelper_->setParent(this);
        captureHelper_->setProperty("bridge",QVariant::fromValue<QObject *>(this));
        captureHelper_->setProperty("receiver",QVariant::fromValue<QObject *>(captureOwner_.receiver));
        captureHelper_->setProperty("scene",QVariant::fromValue<QObject *>(captureOwner_.scene));
        captureHelper_->setProperty("document",QVariant::fromValue<QObject *>(captureOwner_.document));
        const auto epoch=captureEpoch_;const qint64 baseline=now();
        if (!captureIdentity() || epoch!=captureEpoch_) { finish("capture-observation-identity-refused");return; }
        const int width=captureOwner_.window->width(),height=captureOwner_.window->height();const double dpr=captureOwner_.window->devicePixelRatio();
        if (width<=0 || height<=0 || !std::isfinite(dpr) || dpr<=0 || double(width)*height*dpr*dpr>4194304 || !captureAllowed()) { finish("capture-observation-image-cap-refused");return; }
        const auto grabStart=now();reading_=true;
        const auto acquired=captureGrabForFixture_ ? captureGrabForFixture_() : captureOwner_.window->grabWindow();
        reading_=false;
        if (acquired.isNull() || qint64(acquired.width())*acquired.height()>4194304 || acquired.sizeInBytes()>16777216 ||
            acquired.width()!=qRound(width*dpr) || acquired.height()!=qRound(height*dpr)) {
            finish("capture-observation-acquisition-refused");return;
        }
        // Own the returned pixels before any post-grab native getter can alter a
        // shallow/external framebuffer alias. This is a copy, not another grab.
        const QImage image=acquired.copy();
        const auto grabEnd=now();
        if (image.isNull() || !captureAllowed() || !captureIdentity() || epoch!=captureEpoch_) {
            finish("capture-observation-acquisition-refused");return;
        }
        const auto postRead=now();
        const int fd=::openat(root_,"capture-window.png",O_WRONLY|O_CREAT|O_EXCL|O_NOFOLLOW|O_CLOEXEC,0600);capturePngFd_=fd;
        if (fd<0) { finish("capture-observation-image-output-refused");return; }
        QCryptographicHash encodedHash(QCryptographicHash::Sha256);
        InputImageOutput output(fd,&encodedHash);QImageWriter writer(&output,"png");const bool saved=writer.write(image);const auto pngBytes=output.bytes();
        const auto png=readFile("capture-window.png",8388608);
        capturePngHash_=encodedHash.result().toHex();
        if (!saved || !captureFileCurrent("capture-window.png",capturePngFd_) || png.size()!=pngBytes || QCryptographicHash::hash(png,QCryptographicHash::Sha256).toHex()!=capturePngHash_ || !captureAllowed() || epoch!=captureEpoch_) { finish("capture-observation-image-output-unknown");return; }
        QJsonObject object{{"kind","development-capture-observation"},{"version",1},{"nonce",config_.nonce},
            {"attempt_pid",QString::fromLatin1(process_)},{"attempt_start",QString::fromLatin1(processStart_)},
            {"root_device",QString::number(qulonglong(rootStat_.st_dev))},{"root_inode",QString::number(qulonglong(rootStat_.st_ino))},
            {"setup_profile","main-dev-facts-120s"},{"setup_budget_ms",120000},{"capture_budget_ms",5000},{"accepted_ms",captureAcceptedAt_},
            {"baseline_ms",baseline},{"grab_start_ms",grabStart},{"grab_end_ms",grabEnd},{"post_read_ms",postRead},
            {"document_id",captureDocument_},{"page_id",capturePage_},{"page_index",captureIndex_},
            {"begin_epoch",QString::number(epoch)},{"end_epoch",QString::number(captureEpoch_)},{"width",width},{"height",height},{"dpr",dpr},
            {"image_width",image.width()},{"image_height",image.height()},{"png_bytes",pngBytes},{"png_sha256",QString::fromLatin1(capturePngHash_)},
            {"image_status","available"},{"gui_callback_completed",true},{"scope_current",true},{"atomic_snapshot",false},
            {"native_authority",false},{"render_authority",false},{"ui_acknowledged",false},{"observed_order",false}};
        QMetaObject::invokeMethod(this,[this,object,epoch]() mutable {
            const Scope completionScope(this);
            if (!captureAllowed() || !captureIdentity() || epoch!=captureEpoch_ || !captureTokenCurrent() || exists("facts-request") || exists("facts-request.tmp")) { finish("capture-observation-completion-refused");return; }
            object["completed_ms"]=now();captureComplete_=QJsonDocument(object).toJson(QJsonDocument::Compact);
            captureCompleteFd_=::openat(root_,"capture-observation-complete.json",O_WRONLY|O_CREAT|O_EXCL|O_NOFOLLOW|O_CLOEXEC,0600);
            if (captureComplete_.isEmpty() || captureComplete_.size()>8192 || captureCompleteFd_<0 ||
                ::write(captureCompleteFd_,captureComplete_.constData(),size_t(captureComplete_.size()))!=captureComplete_.size() ||
                !captureFileCurrent("capture-observation-complete.json",captureCompleteFd_) || !captureAllowed() || epoch!=captureEpoch_) { withdrawCaptureCompletion();finish("capture-observation-completion-unknown");return; }
            captureDone_=true;
            if (!captureBindingsCurrent()) { withdrawCaptureCompletion();finish("capture-observation-completion-binding-refused");return; }
            captureReading_=false;
            timer_.start(int(config_.setupBudgetMs-now()));queueRequest();
        },Qt::QueuedConnection);
    }
    bool observationCurrent() const {
        return context() && live() && observationWindow_ && observationWindow_->thread()==thread() &&
            observationWindow_->isVisible() && observationWindow_->isActive() &&
            QGuiApplication::focusWindow()==observationWindow_ && qmlEngine(observationWindow_)==engine_;
    }
    void acceptObservationEnd() {
        if (!observationCurrent() || now()>=config_.setupBudgetMs) { finish("input-observation-end-context-refused");return; }
        if (exists("facts-request") || exists("facts-request.tmp")) { finish("input-observation-purpose-refused");return; }
        if (!exists("input-observation-end")) return;
        consumed_=true;
        const auto expected=identity("end-input-observation")+" 120000 "+config_.setupProfile()+'\n';
        if (expected.size()>256 || readFile("input-observation-end",256)!=expected ||
            !observationCurrent() || now()>=config_.setupBudgetMs) { finish("input-observation-end-refused");return; }
        observation_.sealed=true;acceptedAt_=now();timer_.start(config_.facts.budgetMs);
        QMetaObject::invokeMethod(this,[this]{captureObservation();},Qt::QueuedConnection);
    }
    void captureObservation() {
        const Scope scope(this);
        const auto current=[&]{return observationCurrent() && now()<acceptedAt_+config_.facts.budgetMs;};
        if (!current()) { finish("input-observation-capture-context-refused");return; }
        const auto width=observationWindow_->width(),height=observationWindow_->height();
        const double dpr=observationWindow_->devicePixelRatio();
        if (width<=0 || height<=0 || !std::isfinite(dpr) || dpr<=0 || double(width)*height*dpr*dpr>4194304) {
            finish("input-observation-image-cap-refused");return;
        }
        const qint64 grabStart=now();reading_=true;
        const QImage image=observationWindow_->grabWindow();
        reading_=false;const qint64 grabEnd=now();
        if (!current()) { finish("input-observation-capture-unknown");return; }
        QString imageStatus=QStringLiteral("unsupported-empty");
        qint64 pngBytes=0;
        if (!image.isNull()) {
            if (qint64(image.width())*image.height()>4194304 || image.sizeInBytes()>16777216) {
                finish("input-observation-image-cap-refused");return;
            }
            const int fd=::openat(root_,"input-window.png",O_WRONLY|O_CREAT|O_EXCL|O_NOFOLLOW|O_CLOEXEC,0600);
            if (fd<0) { finish("input-observation-image-output-refused");return; }
            InputImageOutput output(fd);QImageWriter writer(&output,"png");
            const bool saved=writer.write(image);pngBytes=output.bytes();::close(fd);
            if (!saved || !current()) { finish("input-observation-image-output-unknown");return; }
            imageStatus=QStringLiteral("available");
        }
        auto object=observation_.json();
        object["evidence_profile"]=QStringLiteral("device-frames-v1");
        object["kind"]=QStringLiteral("development-input-observation");object["nonce"]=config_.nonce;
        object["attempt_pid"]=QString::fromLatin1(process_);object["attempt_start"]=QString::fromLatin1(processStart_);
        object["root_device"]=QString::number(qulonglong(rootStat_.st_dev));object["root_inode"]=QString::number(qulonglong(rootStat_.st_ino));
        object["accepted_ms"]=acceptedAt_;object["seal_ms"]=acceptedAt_;object["grab_start_ms"]=grabStart;
        object["grab_end_ms"]=grabEnd;
        object["application_thread"]=true;object["engine_thread"]=true;object["scope_current"]=true;
        object["width"]=width;object["height"]=height;object["dpr"]=dpr;
        object["image_status"]=imageStatus;object["image_width"]=image.width();object["image_height"]=image.height();object["png_bytes"]=pngBytes;
        object["native_authority"]=false;object["render_authority"]=false;object["ui_acknowledged"]=false;
        // Publish at a separate queued boundary after this callback unwinds.
        // The original accepted-end budget and retained window still govern.
        QMetaObject::invokeMethod(this,[this,object]() mutable {
            const Scope completionScope(this);
            const auto current=[&]{return observationCurrent() && now()<acceptedAt_+config_.facts.budgetMs;};
            if (!current()) { finish("input-observation-completion-context-refused");return; }
            object["completed_ms"]=now();object["gui_callback_completed"]=true;
            const auto bytes=inputCompletionBytes(object);
            if (bytes.isEmpty() || !current() || !writeFile("input-observation-complete.json",bytes) || !current()) {
                if (rootCurrent()) ::unlinkat(root_,"input-observation-complete.json",0);
                finish("input-observation-completion-unknown");return;
            }
            finish("input-observation-completed");
        },Qt::QueuedConnection);
    }
    void startRead() {
        const Scope scope(this);
        if (config_.developmentInputObservation) { finish("input-observation-facts-refused");return; }
        if (!readCurrent() || (config_.developmentCaptureObservation && !captureBindingsCurrent())) { finish("facts-entry-dispatch-refused"); return; }
        reading_=true;
        reader_=std::make_unique<PageFactsSession>(engine_,config_.facts,[this]{return readCurrent() && (!config_.developmentCaptureObservation || captureAllowed());},
            [this](PageFactsResult result){
                const Scope callbackScope(this);
                reading_=false;
                readerStage_=factsRefusalReaderStage(result.stage); readerHadFacts_=bool(result.facts);
                if (!result.facts || !readCurrent()) {
                    refusalPending_=true; finish("facts-entry-read-refused"); return;
                }
                const auto &facts=*result.facts;
                if (config_.developmentCaptureObservation && (!captureBindingsCurrent() || facts.documentId!=captureDocument_ ||
                    facts.currentPageId!=capturePage_ || facts.currentIndex!=captureIndex_)) {
                    finish("capture-observation-facts-agreement-refused"); return;
                }
                QJsonArray order; for (const auto &id:facts.order) order.append(id);
                QJsonObject json{{"kind","development-observed-facts"},{"nonce",config_.nonce},
                    {"attempt_pid",QString::fromLatin1(process_)},{"attempt_start",QString::fromLatin1(processStart_)},
                    {"required_metadata_validated",true},{"required_connections_installed",true},{"document_id",facts.documentId},
                    {"development_setup_opt_in",config_.developmentSetup120},{"setup_budget_ms",config_.setupBudgetMs},
                    {"setup_selection",QString::fromLatin1(config_.setupProfile())},
                    {"current_page_id",facts.currentPageId},{"current_index",facts.currentIndex},{"order",order},
                    {"instance",QString::number(facts.localInstance)},{"begin_epoch",QString::number(facts.beginEpoch)},
                    {"end_epoch",QString::number(facts.endEpoch)},{"begin_ms",facts.beginMs},{"end_ms",facts.endMs},
                    {"request_accepted_ms",acceptedAt_},{"delivered_ms",now()},
                    {"atomic_snapshot",false},{"native_authority",false},{"render_authority",false}};
                const QByteArray bytes=QJsonDocument(json).toJson(QJsonDocument::Compact);
                if (!readCurrent() || (config_.developmentCaptureObservation && !captureBindingsCurrent()) || !writeFile("diagnostics.json",bytes) ||
                    !readCurrent() || (config_.developmentCaptureObservation && !captureBindingsCurrent())) {
                    finish("facts-entry-output-refused"); return;
                }
                result_.observed=true;
                finish("facts-observed-no-change-during-read");
            });
        reader_->begin();
    }
    void finish(const char *stage) {
        if (done_) return;
        done_=true; result_.stage=QString::fromLatin1(stage); timer_.stop();
        if (root_>=0 && rootCurrent()) ::unlinkat(root_,"facts-waiting",0);
        observation_.sealed=true;
        if (config_.developmentInputObservation && root_>=0 && rootCurrent()) ::unlinkat(root_,"input-observation-ready",0);
        queueCompletion();
    }
    void queueCompletion() {
        if (!done_ || depth_ || completionQueued_ || reading_) return;
        completionQueued_=true;
        QMetaObject::invokeMethod(this,[this]{
            // Another queued boundary cannot renew the accepted-request budget.
            if (result_.observed && (!deliveryCurrent() || (config_.developmentCaptureObservation && !captureBindingsCurrent()))) {
                result_.observed=false; result_.stage=QStringLiteral("facts-entry-delivery-refused");
                refusalPending_=true; refusalCompletionBoundary_=true;
            }
            // Refusal-only evidence, before the public callback. This samples
            // non-getter entry checks at recording time, not an atomic history
            // or a reconstruction of the reader's first failure. No retry.
            if (refusalPending_ && config_.developmentSetup120 && config_.setupValid() && config_.facts.budgetMs==5000) {
                FactsRefusalSample sample;
                sample.nonce=config_.nonce; sample.process=QString::fromLatin1(process_);
                sample.processStart=QString::fromLatin1(processStart_); sample.readerStage=readerStage_;
                sample.readerHadFacts=readerHadFacts_; sample.completionBoundary=refusalCompletionBoundary_;
                sample.sampledMs=now(); sample.acceptedMs=acceptedAt_;
                sample.contextCurrent=context(); sample.rootCurrent=rootCurrent();
                sample.closureAbsent=!closed_ && !exists("entry.closed") && !exists("restore.claim");
                sample.identityCurrent=readFile("attempt.identity",128)==process_+' '+processStart_+'\n';
                sample.withinAcceptedDeadline=acceptedAt_>=0 && sample.sampledMs<acceptedAt_+config_.facts.budgetMs;
                // Failure/partial I/O stays unknown and cannot promote success.
                (void)writeFile("refusal.json",factsRefusalBytes(sample));
            }
            QJsonObject callback{{"nonce",config_.nonce},{"stage",result_.stage},
                {"application_thread",app_ && QThread::currentThread()==app_->thread()},
                {"engine_thread",context()}};
            result_.outputPublished=writeFile("callback.json",QJsonDocument(callback).toJson(QJsonDocument::Compact));
            // Syscalls are not preemptible. Never retain late success as a final
            // usable callback; partial/unknown output grants no authority.
            if (result_.observed && (!deliveryCurrent() || (config_.developmentCaptureObservation && !captureBindingsCurrent()))) {
                ::unlinkat(root_,"callback.json",0);
                result_.observed=false; result_.outputPublished=false;
                result_.stage=QStringLiteral("facts-entry-output-unknown");
            }
            reader_.reset();
            auto callbackFunction=std::move(completed_); auto result=std::move(result_);
            if (callbackFunction) callbackFunction(std::move(result));
        },Qt::QueuedConnection);
    }
    QPointer<QGuiApplication> app_;
    QPointer<QQmlEngine> engine_;
    QPointer<QQuickWindow> observationWindow_;
    PageOwner captureOwner_;
    std::function<QImage()> captureGrabForFixture_;
    std::function<void()> captureTemporaryForFixture_;
    QPointer<QObject> captureHelper_;
    QByteArray captureToken_,captureComplete_,capturePngHash_;
    struct stat captureTokenStat_{};
    int captureTokenFd_=-1,capturePngFd_=-1,captureCompleteFd_=-1;
    mutable bool captureTemporaryReleased_=false;
    QString captureDocument_,capturePage_;
    int captureIndex_=-1;
    quint64 captureEpoch_=1;
    qint64 captureAcceptedAt_=-1;
    bool captureConsumed_=false,captureReading_=false,captureDone_=false,captureInvalid_=false,captureChecking_=false,captureWatching_=false;
    InputObservationStore observation_;
    bool observationReady_=false;
    FactsEntryConfig config_;
    std::function<void(FactsEntryResult)> completed_;
    std::function<qint64()> clock_;
    QElapsedTimer elapsed_;
    QFileSystemWatcher watcher_;
    QTimer timer_;
    std::unique_ptr<PageFactsSession> reader_;
    FactsEntryResult result_;
    struct stat rootStat_{};
    QByteArray process_,processStart_;
    QString readerStage_;
    qint64 origin_=0,acceptedAt_=-1;
    int root_=-1,depth_=0,bootstrapAttempts_=0;
    bool started_=false,closed_=false,done_=false,reading_=false,consumed_=false;
    bool bootstrapQueued_=false,requestQueued_=false,completionQueued_=false;
    bool refusalPending_=false,refusalCompletionBoundary_=false,readerHadFacts_=false;
};
}
