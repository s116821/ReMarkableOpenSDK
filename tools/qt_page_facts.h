#pragma once
#include "qt_page_owner.h"
#include <QQmlComponent>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QElapsedTimer>
#include <QRegularExpression>
#include <optional>
#include <atomic>

namespace qml_access {
struct PageFactsConfig {
    QString documentId;
    QStringList expectedOrder;
    int pageCap = 0, budgetMs = 0;
    static bool canonical(const QString &id) {
        static const QRegularExpression pattern(QStringLiteral("^[0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12}$"));
        return id != QStringLiteral("00000000-0000-0000-0000-000000000000") && pattern.match(id).hasMatch();
    }
    bool valid() const {
        if (!canonical(documentId) || pageCap < 1 || pageCap > 256 || budgetMs < 1 || budgetMs > 5000 ||
            expectedOrder.isEmpty() || expectedOrder.size() > pageCap) return false;
        for (int i=0; i<expectedOrder.size(); ++i)
            if (!canonical(expectedOrder[i]) || expectedOrder.indexOf(expectedOrder[i]) != i) return false;
        return true;
    }
};
// Local research observations only. No Rust PageObservation conversion exists.
struct ObservedFacts {
    QString documentId, currentPageId;
    QStringList order;
    int currentIndex = -1;
    quint64 localInstance = 0, beginEpoch = 0, endEpoch = 0;
    qint64 beginMs = 0, endMs = 0;
    bool atomicSnapshot = false, nativeAuthority = false, renderAuthority = false;
};
struct PageFactsResult {
    QString stage;
    std::optional<ObservedFacts> facts;
};

inline QByteArray pageFactsHelper() {
    return R"QML(import QtQml
QtObject {
    property QtObject receiver: null
    property QtObject scene: null
    property QtObject document: null
    property QtObject bridge: null
    property string expectedDocument
    property var expectedOrder
    function readFacts() {
        function ok() { return bridge && bridge.allowed() && receiver && scene && document; }
        if (!ok()) return "!context";
        try {
            const doc = String(document.id);
            if (!ok() || doc !== expectedDocument) return "!identity";
            const count = document.pageCount;
            if (!ok() || !Number.isInteger(count) || count !== expectedOrder.length) return "!count";
            const index = receiver.currentPage;
            if (!ok()) return "!context";
            const page = scene.pageId;
            if (!ok()) return "!context";
            const alias = receiver.currentPageId;
            if (!ok() || !Number.isInteger(index) || index < 0 || index >= count ||
                typeof page !== "string" || alias !== page) return "!page";
            const ids = [];
            for (let i=0; i<count; ++i) {
                const id = document.idForPage(i);
                if (!ok() || typeof id !== "string" || id !== expectedOrder[i]) return "!mapping";
                const reverse = document.pageForId(id);
                if (!ok() || reverse !== i) return "!mapping";
                ids.push(id);
            }
            if (ids[index] !== page) return "!page";
            const endDoc = String(document.id);
            if (!ok()) return "!context";
            const endCount = document.pageCount;
            if (!ok()) return "!context";
            const endIndex = receiver.currentPage;
            if (!ok()) return "!context";
            const endPage = scene.pageId;
            if (!ok()) return "!context";
            const endAlias = receiver.currentPageId;
            if (!ok() || endDoc !== doc || endCount !== count || endIndex !== index || endPage !== page || endAlias !== page)
                return "!changed";
            return JSON.stringify({documentId:doc, pageId:page, index:index, order:ids});
        } catch (ignored) { return "!exception"; }
    }
})QML";
}

// Single-use, locally owned session. No QObject parent can delete it mid-getter.
// The caller retains it until queued completion, which is deferred through every
// nested read/native-getter stack. A future native entry recipe remains separate.
class PageFactsSession final : public QObject {
    Q_OBJECT
public:
    PageFactsSession(QQmlEngine *engine, PageFactsConfig config, std::function<bool()> progress,
                     std::function<void(PageFactsResult)> completed)
        : engine_(engine), config_(std::move(config)), progress_(std::move(progress)), completed_(std::move(completed)) {
        static std::atomic<quint64> next{1}; instance_=next.fetch_add(1);
    }
    ~PageFactsSession() override { delete helper_.data(); }
    void begin() {
        const CallScope call(this);
        if (started_) { invalidate(); finish("facts-reentry-refused"); return; }
        started_=true; elapsed_.start();
        if (!config_.valid() || !current()) { finish("facts-config-or-context-refused"); return; }
        const char *ownerStage=findPageOwner(engine_, [this]{return current();}, owner_);
        if (QByteArrayView(ownerStage) != QByteArrayView("open-owner-observed")) { finish(ownerStage); return; }
        if (!installObservers() || !allowed()) { finish("facts-metadata-or-context-refused"); return; }
        const quint64 baseline=epoch_;
        const qint64 began=elapsed_.elapsed();
        QQmlComponent component(engine_, this);
        component.setData(pageFactsHelper(), QUrl(QStringLiteral("qrc:/OwnedFactsReader.qml")));
        if (component.status()!=QQmlComponent::Ready || !allowed()) { finish("facts-helper-refused"); return; }
        helper_=component.create();
        if (!helper_ || !allowed()) { finish("facts-helper-refused"); return; }
        QQmlEngine::setObjectOwnership(helper_, QQmlEngine::CppOwnership);
        helper_->setParent(this);
        helper_->setProperty("bridge", QVariant::fromValue<QObject *>(this));
        helper_->setProperty("receiver", QVariant::fromValue<QObject *>(owner_.receiver));
        helper_->setProperty("scene", QVariant::fromValue<QObject *>(owner_.scene));
        helper_->setProperty("document", QVariant::fromValue<QObject *>(owner_.document));
        helper_->setProperty("expectedDocument", config_.documentId);
        helper_->setProperty("expectedOrder", config_.expectedOrder);
        QVariant result;
        if (!allowed() || !QMetaObject::invokeMethod(helper_, "readFacts", Qt::DirectConnection, Q_RETURN_ARG(QVariant,result)) ||
            !allowed() || epoch_!=baseline || result.metaType()!=QMetaType::fromType<QString>()) { finish("facts-changed-or-context-refused"); return; }
        const QByteArray bytes=result.toString().toUtf8();
        if (bytes.startsWith('!') || bytes.size()>16384) { finish("facts-values-refused"); return; }
        const auto json=QJsonDocument::fromJson(bytes);
        const auto object=json.object();
        QStringList order;
        for (const auto &id:object.value("order").toArray()) order.append(id.toString());
        const int index=object.value("index").toInt(-1);
        if (!json.isObject() || order!=config_.expectedOrder || object.value("documentId").toString()!=config_.documentId ||
            index<0 || index>=order.size() || object.value("pageId").toString()!=order[index] ||
            !allowed() || epoch_!=baseline) { finish("facts-final-boundary-refused"); return; }
        result_.facts=ObservedFacts{config_.documentId,order[index],order,index,instance_,baseline,epoch_,began,elapsed_.elapsed(),false,false,false};
        finish("facts-observed-no-change-during-read");
    }
public slots:
    bool allowed() {
        const CallScope call(this);
        if (checking_) { invalidate(); return false; }
        checking_=true;
        const bool ownerCurrent=current() && activeOwner(owner_,engine_);
        checking_=false;
        // Do not repeat getters; their reentry may have changed epoch/lifetime/time.
        return ownerCurrent && current() && owner_.window && owner_.receiver && owner_.scene && owner_.document;
    }
    void invalidate() { invalid_=true; ++epoch_; }
private:
    struct CallScope {
        PageFactsSession *session;
        explicit CallScope(PageFactsSession *value):session(value){++session->depth_;}
        ~CallScope(){if (--session->depth_==0) session->scheduleCompletion();}
    };
    bool current() {
        if (done_ || invalid_ || !engine_ || QThread::currentThread()!=thread() || engine_->thread()!=thread() ||
            elapsed_.elapsed()>=config_.budgetMs || !progress_()) return false;
        return !done_ && !invalid_ && engine_ && engine_->thread()==thread() && elapsed_.elapsed()<config_.budgetMs;
    }
    bool connectSignal(QObject *object, const char *signature) {
        if (!object) return false;
        const int index=object->metaObject()->indexOfSignal(signature);
        const int slot=metaObject()->indexOfSlot("invalidate()");
        if (index<0 || slot<0) return false;
        const auto signal=object->metaObject()->method(index);
        if (signal.returnMetaType()!=QMetaType::fromType<void>()) return false;
        return bool(QObject::connect(object,signal,this,metaObject()->method(slot)));
    }
    bool installObservers() {
        if (!owner_.document || !propertyType(owner_.document,"pageCount",QMetaType::fromType<int>())) return false;
        const auto *meta=owner_.document->metaObject();
        for (const auto &signature:{"idForPage(int)","pageForId(QString)"}) {
            const int index=meta->indexOfMethod(signature);
            if (index<0 || meta->method(index).returnMetaType()!=
                (QByteArrayView(signature)==QByteArrayView("idForPage(int)") ? QMetaType::fromType<QString>() : QMetaType::fromType<int>())) return false;
        }
        for (const auto *signature:{"pageCountChanged(int,int)","pageMapChanged()","pageAdded(int)","pagesAdded(QList<int>)",
             "pageMoved(int,int)","pagesMoved()","pagesRemoved()","redirectionPageMapChanged()","pageUpdated(int)",
             "documentMetadataChanged()","orientationChanged()"})
            if (!connectSignal(owner_.document,signature)) return false;
        for (const auto *signature:{"pageIdChanged()","documentWrapperChanged()","workerChanged()","viewportChanged()"})
            if (!connectSignal(owner_.scene,signature)) return false;
        for (const auto *name:{"document","currentPage","currentPageId","drawingAreaFocused"}) {
            const int index=owner_.receiver->metaObject()->indexOfProperty(name);
            if (index<0) return false;
            const auto property=owner_.receiver->metaObject()->property(index);
            if (!property.hasNotifySignal() || !connectSignal(owner_.receiver,property.notifySignal().methodSignature().constData())) return false;
        }
        QObject::connect(owner_.window,&QWindow::activeChanged,this,&PageFactsSession::invalidate);
        QObject::connect(owner_.window,&QWindow::visibleChanged,this,&PageFactsSession::invalidate);
        for (const auto &item:{owner_.receiver,owner_.scene}) {
            QObject::connect(item,&QQuickItem::visibleChanged,this,&PageFactsSession::invalidate);
            QObject::connect(item,&QQuickItem::enabledChanged,this,&PageFactsSession::invalidate);
            QObject::connect(item,&QQuickItem::activeFocusChanged,this,&PageFactsSession::invalidate);
            QObject::connect(item,&QQuickItem::windowChanged,this,&PageFactsSession::invalidate);
            QObject::connect(item,&QQuickItem::parentChanged,this,&PageFactsSession::invalidate);
        }
        for (QObject *object:{static_cast<QObject *>(engine_.data()),static_cast<QObject *>(owner_.window.data()),
             static_cast<QObject *>(owner_.receiver.data()),static_cast<QObject *>(owner_.scene.data()),owner_.document.data()})
            QObject::connect(object,&QObject::destroyed,this,&PageFactsSession::invalidate);
        return true;
    }
    void finish(const char *stage) { if (done_) return; done_=true; result_.stage=QString::fromLatin1(stage); scheduleCompletion(); }
    void scheduleCompletion() {
        if (!done_ || depth_ || queued_) return;
        queued_=true;
        QMetaObject::invokeMethod(this,[this]{
            // Delivery is another cancellation/lifetime/deadline boundary. This
            // adds no new native getter and never refreshes a failed read.
            const bool deliverable = !invalid_ && engine_ && owner_.window && owner_.receiver &&
                owner_.scene && owner_.document && QThread::currentThread()==thread() && engine_->thread()==thread() &&
                elapsed_.elapsed()<config_.budgetMs && progress_();
            if (result_.facts && (!deliverable || invalid_ || !engine_ || !owner_.window ||
                !owner_.receiver || !owner_.scene || !owner_.document ||
                QThread::currentThread()!=thread() || engine_->thread()!=thread() ||
                elapsed_.elapsed()>=config_.budgetMs || epoch_!=result_.facts->endEpoch)) {
                result_.facts.reset(); result_.stage=QStringLiteral("facts-delivery-boundary-refused");
            }
            // The caller may release the retained session in its callback.
            // Keep the callback/result alive independently before that happens.
            auto callback=std::move(completed_);
            auto result=std::move(result_);
            callback(std::move(result));
        },Qt::QueuedConnection);
    }
    QPointer<QQmlEngine> engine_;
    PageFactsConfig config_;
    std::function<bool()> progress_;
    std::function<void(PageFactsResult)> completed_;
    PageOwner owner_;
    QPointer<QObject> helper_;
    QElapsedTimer elapsed_;
    PageFactsResult result_;
    quint64 instance_=0,epoch_=0;
    int depth_=0;
    bool started_=false,invalid_=false,done_=false,queued_=false,checking_=false;
};
}
