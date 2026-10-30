#include "qt_page_facts.h"
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QEventLoop>
#include <QTimer>
#include <cstdio>
#include <memory>
using namespace qml_access;
class DocumentFixture : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString id READ id CONSTANT)
    Q_PROPERTY(int pageCount READ pageCount NOTIFY countChanged)
public:
    QStringList ids;
    mutable std::function<void()> hook;
    mutable int idReads=0;
    bool reverseWrong=false;
    QString id() const { ++idReads; return QStringLiteral("00000000-0000-4000-8000-000000000001"); }
    int pageCount() const { return ids.size(); }
    Q_INVOKABLE QString idForPage(int index) const {
        const QString value=ids.value(index); const auto callback=hook;
        if (callback) callback();
        return value;
    }
    Q_INVOKABLE int pageForId(const QString &id) const { return reverseWrong ? -1 : ids.indexOf(id); }
signals:
    void countChanged();
    void pageCountChanged(int before,int after);
    void pageMapChanged();
    void pageAdded(int index);
    void pagesAdded(QList<int> indexes);
    void pageMoved(int before,int after);
    void pagesMoved();
    void pagesRemoved();
    void redirectionPageMapChanged();
    void pageUpdated(int index);
    void documentMetadataChanged();
    void orientationChanged();
};
class IncompleteDocument : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString id READ id CONSTANT)
    Q_PROPERTY(int pageCount READ pageCount CONSTANT)
public:
    QString id() const { return QStringLiteral("00000000-0000-4000-8000-000000000001"); }
    int pageCount() const { return 1; }
    Q_INVOKABLE QString idForPage(int) const { return QStringLiteral("00000000-0000-4000-8000-000000000002"); }
    Q_INVOKABLE int pageForId(const QString &) const { return 0; }
};
class IncompatibleDocument : public DocumentFixture {
    Q_OBJECT
    Q_PROPERTY(double pageCount READ incompatibleCount)
public:
    double incompatibleCount() const { return ids.size(); }
};
class WrongSignalsDocument : public IncompleteDocument {
    Q_OBJECT
signals:
    void pageCountChanged(int before,int after);
    void pageMapChanged();
    void pageAdded(int index);
    void pagesAdded(QString incompatibleIndexes);
    void pageMoved(int before,int after);
    void pagesMoved();
    void pagesRemoved();
    void redirectionPageMapChanged();
    void pageUpdated(int index);
    void documentMetadataChanged();
    void orientationChanged();
};
class SceneView : public QQuickItem {
    Q_OBJECT
    Q_PROPERTY(QObject *document MEMBER document NOTIFY documentWrapperChanged)
    Q_PROPERTY(QString pageId MEMBER pageId NOTIFY pageIdChanged)
public:
    QObject *document=nullptr;
    QString pageId;
signals:
    void pageIdChanged();
    void documentWrapperChanged();
    void workerChanged();
    void viewportChanged();
};
class OwnedReceiver : public QQuickItem {
    Q_OBJECT
    Q_PROPERTY(bool drawingAreaFocused READ focusEvidence NOTIFY focusEvidenceChanged)
public:
    OwnedReceiver() { setFlag(QQuickItem::ItemIsFocusScope); }
    std::function<bool()> evidence;
    bool focusEvidence() const { return evidence && evidence(); }
signals:
    void focusEvidenceChanged();
};
int main(int argc,char **argv) {
    QGuiApplication app(argc,argv); app.setQuitOnLastWindowClosed(false);
    if (argc!=2) return 2;
    const QByteArray mode(argv[1]);
    qmlRegisterType<SceneView>("OwnedFacts",1,0,"SceneView");
    qmlRegisterType<OwnedReceiver>("OwnedFacts",1,0,"OwnedReceiver");
    auto engine=std::make_unique<QQmlApplicationEngine>();
    auto document=std::make_unique<DocumentFixture>();
    QThread foreignDocumentThread;
    IncompleteDocument incomplete;
    IncompatibleDocument incompatible;
    WrongSignalsDocument wrongSignals;
    PageFactsConfig config{document->id(),{},256,5000};
    const int count=mode=="good-one" || mode=="metadata" || mode=="signal-shape" ? 1 : mode=="good-cap" ? 256 : 6;
    for (int i=0;i<count;++i) config.expectedOrder.append(QStringLiteral("00000000-0000-4000-8000-%1").arg(i+2,12,16,QLatin1Char('0')));
    document->ids=config.expectedOrder;
    incompatible.ids=config.expectedOrder;
    QObject *contextDocument=mode=="metadata" ? static_cast<QObject *>(&incomplete) :
        mode=="incompatible" ? static_cast<QObject *>(&incompatible) :
        mode=="signal-shape" ? static_cast<QObject *>(&wrongSignals) : document.get();
    engine->rootContext()->setContextProperty("ownedDocument",contextDocument);
    engine->rootContext()->setContextProperty("ambiguousFixture",mode=="ambiguous");
    engine->loadData(R"QML(import QtQuick
import QtQuick.Window
import OwnedFacts 1.0
Window { visible:true; width:400; height:400
    FocusScope { anchors.fill:parent
        property QtObject document:ownedDocument
        readonly property int currentPage:receiver.currentPage
        readonly property string currentPageId:scene ? scene.pageId : ""
        readonly property bool drawingAreaFocused:ambiguousFixture && scene.activeFocus
        OwnedReceiver { id:receiver; objectName:"receiver"; anchors.fill:parent; focus:true
            property QtObject document:ownedDocument
            property int currentPage:0
            property string currentPageId:scene ? scene.pageId : ""
            SceneView { id:scene; objectName:"scene"; anchors.fill:parent; focus:true
                document:receiver.document
                pageId:receiver.document ? receiver.document.idForPage(receiver.currentPage) : ""
            }
        }
    }
})QML");
    if (engine->rootObjects().size()!=1) return 3;
    auto *window=qobject_cast<QQuickWindow *>(engine->rootObjects()[0]);
    auto *receiver=window->findChild<OwnedReceiver *>("receiver");
    auto *scene=window->findChild<SceneView *>("scene");
    // The owned receiver exposes no native mutation callable at all.
    if (receiver->metaObject()->indexOfMethod("openPage(int)")>=0) return 5;
    receiver->evidence=[&]{return scene && scene->hasActiveFocus();};
    window->requestActivate(); scene->forceActiveFocus(); QCoreApplication::processEvents();
    bool progress=true, completed=false, nested=false, held=true;
    PageFactsResult result;
    if (mode=="cap-zero") config.pageCap=0;
    if (mode=="cap-large") config.pageCap=257;
    if (mode=="cap-small") config.pageCap=1;
    if (mode=="budget-zero") config.budgetMs=0;
    if (mode=="canonical") config.documentId=QStringLiteral("ABCDEF00-0000-4000-8000-000000000001");
    if (mode=="nil") config.documentId=QStringLiteral("00000000-0000-0000-0000-000000000000");
    if (mode=="duplicate") config.expectedOrder[1]=config.expectedOrder[0];
    if (mode=="empty") config.expectedOrder.clear();
    if (mode=="hidden") receiver->setVisible(false);
    if (mode=="disabled") receiver->setEnabled(false);
    if (mode=="no-window") window->hide();
    if (mode=="focus") scene->setFocus(false);
    if (mode=="document") scene->setProperty("document",QVariant::fromValue<QObject *>(&app));
    if (mode=="count") document->ids.removeLast();
    if (mode=="order") document->ids.swapItemsAt(0,1);
    if (mode=="reverse") document->reverseWrong=true;
    if (mode=="alias") receiver->setProperty("currentPageId",QStringLiteral("wrong"));
    if (mode=="page") scene->setProperty("pageId",config.expectedOrder.last());
    if (mode=="index") receiver->setProperty("currentPage",config.expectedOrder.size());
    if (mode=="cancel") progress=false;
    if (mode=="deadline" || mode=="delivery-deadline" || mode=="nested-deadline") config.budgetMs=1000;
    QQmlEngine otherEngine;
    QQuickWindow otherWindow;
    if (mode=="wrong-window") { otherWindow.show(); otherWindow.requestActivate(); QCoreApplication::processEvents(); }
    bool readReturned=false, deliveryHooked=false, threadMoved=false;
    int finalProgressCalls=0;
    std::unique_ptr<PageFactsSession> session;
    session=std::make_unique<PageFactsSession>(mode=="wrong-engine" ? &otherEngine : engine.get(),config,[&]{
        if (readReturned && !deliveryHooked && mode=="delivery-reentry") { deliveryHooked=true; session->begin(); }
        if (readReturned && !deliveryHooked && mode=="delivery-progress-destroy") { deliveryHooked=true; engine.reset(); }
        if (readReturned && !deliveryHooked && mode=="delivery-progress-document-thread") {
            deliveryHooked=true; document->moveToThread(&foreignDocumentThread); threadMoved=true;
        }
        // Second progress call in allowed(): activeOwner has already checked
        // affinity, so only the post-progress retained-context check catches it.
        if (mode=="final-progress-document-thread" && document->idReads>=2 && ++finalProgressCalls==2) {
            document->moveToThread(&foreignDocumentThread); threadMoved=true;
        }
        return progress;
    },[&](PageFactsResult value){
        if (nested) held=false;
        completed=true; result=std::move(value);
        if (mode=="release") session.reset();
    });
    bool hooked=false;
    document->hook=[&]{
        if (hooked) return;
        hooked=true;
        if (mode=="dirty") emit document->pageMapChanged();
        if (mode=="away-back") { emit document->pageUpdated(0); emit document->pageUpdated(0); }
        if (mode=="redirection") emit document->redirectionPageMapChanged();
        if (mode=="worker") emit scene->workerChanged();
        if (mode=="signal-count") emit document->pageCountChanged(6,6);
        if (mode=="signal-added") emit document->pageAdded(0);
        if (mode=="signal-added-list") emit document->pagesAdded(QList<int>{0});
        if (mode=="signal-moved") emit document->pageMoved(0,1);
        if (mode=="signal-moved-list") emit document->pagesMoved();
        if (mode=="signal-removed") emit document->pagesRemoved();
        if (mode=="signal-metadata") emit document->documentMetadataChanged();
        if (mode=="signal-orientation") emit document->orientationChanged();
        if (mode=="signal-viewport") emit scene->viewportChanged();
        if (mode=="signal-document") emit scene->documentWrapperChanged();
        if (mode=="signal-page") emit scene->pageIdChanged();
        if (mode=="getter-focus") scene->setFocus(false);
        if (mode=="getter-context") scene->setProperty("document",QVariant::fromValue<QObject *>(&app));
        if (mode=="getter-document-thread") { document->moveToThread(&foreignDocumentThread); threadMoved=true; }
        if (mode=="getter-cancel") progress=false;
        if (mode=="deadline") QThread::msleep(1010);
        if (mode=="nested") {
            nested=true; session->begin(); QCoreApplication::processEvents();
            if (completed) held=false;
            nested=false;
        }
        if (mode=="nested-cancel") {
            nested=true; progress=false; QCoreApplication::processEvents();
            if (completed) held=false;
            nested=false;
        }
        if (mode=="nested-deadline") {
            nested=true;
            QEventLoop loop; QTimer::singleShot(1010,&loop,&QEventLoop::quit); loop.exec();
            if (completed) held=false;
            nested=false;
        }
        if (mode=="destroy") { document.reset(); }
        if (mode=="scene-destroy") { auto *old=scene; scene=nullptr; delete old; }
        if (mode=="nested-destroy") {
            nested=true; auto *old=scene; scene=nullptr; delete old;
            QCoreApplication::processEvents();
            if (completed) held=false;
            nested=false;
        }
        // Intentionally unnotified A->B->A: facts can pass and are NOT atomic.
        if (mode=="silent-aba") { document->ids.swapItemsAt(0,1); document->ids.swapItemsAt(0,1); }
    };
    document->idReads=0;
    session->begin();
    readReturned=true;
    if (completed) return 4; // All delivery deferred until the native/read stack unwinds.
    if (mode=="delivery-cancel") progress=false;
    if (mode=="delivery-dirty") emit document->pagesMoved();
    if (mode=="delivery-deadline") QThread::msleep(1010);
    if (mode=="engine-destroy") engine.reset();
    if (mode=="window-destroy") delete window;
    if (mode=="delivery-document-thread") { document->moveToThread(&foreignDocumentThread); threadMoved=true; }
    QCoreApplication::processEvents();
    const bool expected=mode=="good-one" || mode=="good-six" || mode=="good-cap" || mode=="silent-aba" || mode=="release";
    const bool facts=bool(result.facts);
    bool limited=true;
    if (facts) limited=!result.facts->atomicSnapshot && !result.facts->nativeAuthority && !result.facts->renderAuthority &&
        result.facts->order==config.expectedOrder && result.facts->beginEpoch==result.facts->endEpoch &&
        result.facts->localInstance!=0 && result.facts->endMs>=result.facts->beginMs;
    const bool hookRequired=mode=="deadline" || mode.startsWith("nested") || mode=="getter-cancel" ||
        mode=="dirty" || mode=="silent-aba";
    const bool deliveryRequired=mode.startsWith("delivery-") || mode=="engine-destroy" || mode=="window-destroy";
    const bool threadRequired=mode.endsWith("document-thread");
    const bool boundary=(!hookRequired || hooked) && (!deliveryRequired || result.stage=="facts-delivery-boundary-refused") &&
        (!threadRequired || (threadMoved && document->thread()==&foreignDocumentThread));
    const bool passed=completed && facts==expected && limited && held && boundary;
    std::printf("%s: %s (%s; limited=%d; nested-held=%d; boundary=%d)\n",argv[1],passed ? "PASS" : "FAIL",qPrintable(result.stage),limited,held,boundary);
    if (document && document->thread()==&foreignDocumentThread) {
        // Restore from the owning thread before ordinary owned-fixture teardown.
        foreignDocumentThread.start();
        QMetaObject::invokeMethod(document.get(),[&]{ document->moveToThread(app.thread()); },Qt::BlockingQueuedConnection);
        foreignDocumentThread.quit(); foreignDocumentThread.wait();
    }
    return passed ? 0 : 1;
}
#include "qt_page_facts_fixture.moc"
