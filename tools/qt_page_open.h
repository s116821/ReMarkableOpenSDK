#pragma once
#include "qt_page_owner.h"
#include <QQmlComponent>
#include <QJsonDocument>
#include <QJsonArray>
#include <QTimer>
#include <QThread>
#include <QRegularExpression>

namespace qml_access {
struct PageOpenConfig {
    QString documentId, sourcePageId, targetPageId;
    QStringList pageIds;
    bool enabled = false;
    bool valid() const {
        static const QRegularExpression uuid(QStringLiteral("^[0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12}$"));
        const QString nil = QStringLiteral("00000000-0000-0000-0000-000000000000");
        if (!enabled || documentId == nil || !uuid.match(documentId).hasMatch() || pageIds.size() != 6 ||
            !pageIds.contains(sourcePageId) || !pageIds.contains(targetPageId)) return false;
        for (int i = 0; i < pageIds.size(); ++i) {
            if (pageIds[i] == nil || !uuid.match(pageIds[i]).hasMatch()) return false;
            if (pageIds.indexOf(pageIds[i]) != i) return false;
        }
        return true;
    }
};
inline QByteArray pageOpenHelper(const PageOpenConfig &config) {
    QJsonArray ids;
    for (const auto &id : config.pageIds) ids.append(id);
    const auto literal = [](const QString &value) {
        return QJsonDocument(QJsonArray{value}).toJson(QJsonDocument::Compact) + "[0]";
    };
    return QByteArray("import QtQml\nQtObject {\n") +
        "property QtObject receiver: null\nproperty QtObject scene: null\nproperty QtObject document: null\n"
        "property QtObject bridge: null\nproperty bool entered: false\n" +
        "readonly property string expectedDocument: " + literal(config.documentId) + "\n" +
        "readonly property string source: " + literal(config.sourcePageId) + "\n" +
        "readonly property string target: " + literal(config.targetPageId) + "\n" +
        "readonly property var order: " + QJsonDocument(ids).toJson(QJsonDocument::Compact) + R"QML(
    function observe() {
        if (!bridge || !bridge.allowed() || !receiver || !scene || !document) return "context";
        try {
            if (receiver.document !== document || scene.document !== document || String(document.id) !== expectedDocument ||
                typeof document.idForPage !== "function" || typeof document.pageForId !== "function" ||
                typeof receiver.openPage !== "function" || document.pageCount !== order.length) return "identity";
            for (let i = 0; i < order.length; ++i) {
                if (!bridge.allowed() || !receiver || !scene || !document) return "context";
                if (document.idForPage(i) !== order[i] || document.pageForId(order[i]) !== i) return "mapping";
            }
            const index = receiver.currentPage;
            const page = scene.pageId;
            if (!Number.isInteger(index) || index < 0 || index >= order.length || typeof page !== "string" ||
                receiver.currentPageId !== page || document.idForPage(index) !== page || document.pageForId(page) !== index)
                return "page";
            if (!bridge.allowed()) return "context";
            return page === target ? "target" : page === source ? "source" : "page";
        } catch (ignored) { return "exception"; }
    }
    function openOnce() {
        if (entered) return "duplicate";
        entered = true;
        const state = observe();
        if (state === "target") return "already-open";
        if (state !== "source") return state;
        try {
            const index = document.pageForId(target);
            if (!Number.isInteger(index) || index < 0 || document.idForPage(index) !== target) return "mapping";
            if (observe() !== "source" || !bridge.claim()) return "context";
            receiver.openPage(index);
            return "dispatched";
        } catch (ignored) { return "exception"; }
    }
})QML";
}

// No signals are interpreted as success. Queued observations recheck the same owner.
class PageOpenSession final : public QObject {
    Q_OBJECT
public:
    PageOpenSession(QObject *parent, QQmlEngine *engine, PageOpenConfig config,
                    std::function<bool()> progress, std::function<void(const char *)> completed,
                    std::function<void(bool)> nativeCall = {})
        : QObject(parent), engine_(engine), config_(std::move(config)), progress_(std::move(progress)), completed_(std::move(completed)), nativeCall_(std::move(nativeCall)) {}
    ~PageOpenSession() override { delete helper_.data(); }
    bool attempted() const { return claimed_; }
    bool returned() const { return returned_; }
    void begin() {
        const CallScope call(nativeCall_);
        if (started_) return;
        started_ = true;
        if (!config_.valid()) { finish("open-config-refused"); return; }
        const char *result = findPageOwner(engine_, progress_, owner_);
        if (QByteArrayView(result) != QByteArrayView("open-owner-observed")) { finish(result); return; }
        for (QObject *object : {static_cast<QObject *>(engine_), static_cast<QObject *>(owner_.window),
                              static_cast<QObject *>(owner_.receiver), static_cast<QObject *>(owner_.scene), owner_.document.data()})
            connect(object, &QObject::destroyed, this, [this] { invalid_ = true; changed(); });
        for (QQuickItem *item : {owner_.receiver.data(), owner_.scene.data()}) {
            const auto ownerChange = [this] { invalid_ = true; changed(); };
            connect(item, &QQuickItem::activeFocusChanged, this, ownerChange);
            connect(item, &QQuickItem::visibleChanged, this, ownerChange);
            connect(item, &QQuickItem::enabledChanged, this, ownerChange);
            connect(item, &QQuickItem::windowChanged, this, ownerChange);
            connect(item, &QQuickItem::parentChanged, this, ownerChange);
        }
        connect(owner_.window, &QWindow::activeChanged, this, [this] {
            invalid_ = true;
            changed();
        });
        const int slot = metaObject()->indexOfSlot("pageChanged()");
        const int invalidationSlot = metaObject()->indexOfSlot("invalidate()");
        for (const char *name : {"pageIdChanged()", "documentWrapperChanged()"}) {
            const int signal = owner_.scene->metaObject()->indexOfSignal(name);
            if (signal < 0 || !QObject::connect(owner_.scene, owner_.scene->metaObject()->method(signal),
                                              this, metaObject()->method(QByteArrayView(name) == QByteArrayView("documentWrapperChanged()") ? invalidationSlot : slot))) { finish("open-observer-unavailable"); return; }
        }
        for (const char *name : {"document", "currentPage", "currentPageId"}) {
            const int index = owner_.receiver->metaObject()->indexOfProperty(name);
            const auto property = owner_.receiver->metaObject()->property(index);
            if (!property.hasNotifySignal() || !QObject::connect(owner_.receiver, property.notifySignal(),
                                                               this, metaObject()->method(QByteArrayView(name) == QByteArrayView("document") ? invalidationSlot : slot))) {
                finish("open-observer-unavailable"); return;
            }
        }
        component_ = new QQmlComponent(engine_, this);
        component_->setData(pageOpenHelper(config_), QUrl());
        if (!allowed() || component_->status() != QQmlComponent::Ready) { finish("open-helper-unavailable"); return; }
        helper_ = component_->create();
        if (!allowed() || !helper_) { finish("open-helper-unavailable"); return; }
        QQmlEngine::setObjectOwnership(helper_, QQmlEngine::CppOwnership);
        for (const auto &value : {qMakePair("receiver", static_cast<QObject *>(owner_.receiver)),
                                 qMakePair("scene", static_cast<QObject *>(owner_.scene)),
                                 qMakePair("document", owner_.document.data()), qMakePair("bridge", static_cast<QObject *>(this))}) {
            if (!allowed() || !helper_ || !helper_->setProperty(value.first, QVariant::fromValue(value.second))) {
                finish("open-helper-unavailable"); return;
            }
        }
        // Dispatch is a distinct queued turn after observation installation.
        if (!QMetaObject::invokeMethod(this, [this] {
            const CallScope call(nativeCall_);
            if (!allowed()) { finish("open-context-lost"); return; }
            QVariant result;
            if (!QMetaObject::invokeMethod(helper_, "openOnce", Qt::DirectConnection, Q_RETURN_ARG(QVariant, result))) {
                finish("open-callable-unavailable"); return;
            }
            returned_ = claimed_ && result.metaType() == QMetaType::fromType<QString>() && result.toString() == QStringLiteral("dispatched");
            if (!allowed()) { finish("open-context-lost"); return; }
            if (result.metaType() != QMetaType::fromType<QString>()) { finish("open-helper-result-refused"); return; }
            const QString value = result.toString();
            if (value != QStringLiteral("dispatched") && value != QStringLiteral("already-open")) {
                finish(refusalStage(value)); return;
            }
            dispatched_ = true;
            changed();
        }, Qt::QueuedConnection)) finish("open-dispatch-unavailable");
    }
public slots:
    bool allowed() const {
        return !done_ && !invalid_ && engine_ && QThread::currentThread() == thread() &&
            progress_() && activeOwner(owner_, engine_);
    }
    bool claim() { if (claimed_ || !allowed()) return false; claimed_ = true; return true; }
    void invalidate() { invalid_ = true; changed(); }
    void pageChanged() { if (!claimed_ && !dispatched_) invalid_ = true; changed(); }
    void changed() {
        if (done_ || queued_) return;
        queued_ = true;
        if (!QMetaObject::invokeMethod(this, [this] {
            const CallScope call(nativeCall_);
            queued_ = false;
            if (done_) return;
            if (!allowed()) { finish("open-context-lost"); return; }
            if (!dispatched_) return;
            QVariant result;
            if (!helper_ || !QMetaObject::invokeMethod(helper_, "observe", Qt::DirectConnection, Q_RETURN_ARG(QVariant, result)) ||
                result.metaType() != QMetaType::fromType<QString>() || !allowed()) { finish("open-context-lost"); return; }
            if (result.toString() == QStringLiteral("target")) finish("open-observed");
            else if (result.toString() != QStringLiteral("source")) finish(refusalStage(result.toString()));
        }, Qt::QueuedConnection)) finish("open-observation-unavailable");
    }
private:
    static const char *refusalStage(const QString &value) {
        if (value == QStringLiteral("identity")) return "open-identity-refused";
        if (value == QStringLiteral("mapping")) return "open-mapping-refused";
        if (value == QStringLiteral("page")) return "open-page-refused";
        if (value == QStringLiteral("exception")) return "open-call-exception";
        if (value == QStringLiteral("duplicate")) return "open-duplicate-refused";
        return "open-context-lost";
    }
    struct CallScope {
        const std::function<void(bool)> &callback;
        explicit CallScope(const std::function<void(bool)> &value) : callback(value) { if (callback) callback(true); }
        ~CallScope() { if (callback) callback(false); }
    };
    void finish(const char *stage) {
        if (done_) return;
        done_ = true;
        // Never tear down helper/receiver on native signal or QML call stacks.
        QTimer::singleShot(0, this, [this, stage] { completed_(stage); });
    }
    QPointer<QQmlEngine> engine_;
    PageOpenConfig config_;
    std::function<bool()> progress_;
    std::function<void(const char *)> completed_;
    std::function<void(bool)> nativeCall_;
    PageOwner owner_;
    QQmlComponent *component_ = nullptr;
    QPointer<QObject> helper_;
    bool started_ = false, invalid_ = false, done_ = false, claimed_ = false, returned_ = false, dispatched_ = false, queued_ = false;
};
}
