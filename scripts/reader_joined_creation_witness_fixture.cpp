// Original modeled Qt objects. These fixtures do not qualify native creation.
#include "reader_joined_creation_witness.h"
#include <QCoreApplication>
#include <QTextStream>
#include <memory>
#include <stdexcept>

namespace entry {
struct Id {
    QString text;
    bool operator==(const Id &other) const { return text==other.text; }
    bool operator!=(const Id &other) const { return !(*this==other); }
};
}
Q_DECLARE_METATYPE(entry::Id)

class QmlDocumentWrapper : public QObject {
    Q_OBJECT
    Q_PROPERTY(entry::Id id READ id CONSTANT)
    Q_PROPERTY(int pageCount READ pageCount NOTIFY pageCountChanged)
public:
    entry::Id key{QStringLiteral("10000000-0000-0000-0000-000000000100")};
    QStringList pages;
    QString pageTemplate=QStringLiteral("owned-template");
    mutable std::function<void()> onRead;
    bool badRoundtrip=false, wrongCount=false;
    explicit QmlDocumentWrapper(QObject *parent=nullptr):QObject(parent) {
        for(int i=1;i<=8;++i) pages.append(QStringLiteral("10000000-0000-0000-0000-%1").arg(i,12,10,QLatin1Char('0')));
    }
    entry::Id id() const { read(); return key; }
    int pageCount() const { read(); return pages.size()+(wrongCount?1:0); }
    Q_INVOKABLE QString idForPage(int index) const { read(); return index>=0 && index<pages.size()?pages[index]:QString(); }
    Q_INVOKABLE int pageForId(QString page) const { read(); return badRoundtrip?42:pages.indexOf(page); }
    Q_INVOKABLE QString templateForPage(int) const { read(); return pageTemplate; }
    void read() const { if(onRead) { auto f=std::move(onRead); onRead={}; f(); } }
signals:
    void pageCountChanged();
    void pageMapChanged();
};
namespace xofm { namespace libs { namespace library {
class DocumentController : public QObject {
    Q_OBJECT
public:
    enum Mode { Async, Sync, None, False, Throw, Duplicate } mode=Async;
    int calls=0,index=-1;
    entry::Id lastId;
    QString lastTemplate,target;
    QSizeF size;
    QJSValue retained;
    std::function<void()> onCall;
    Q_INVOKABLE bool addPageWithTemplateAndPageSize(entry::Id id,int insertion,QString pageTemplate,QSizeF paper,QJSValue callback,QString targetId) {
        ++calls;lastId=id;index=insertion;lastTemplate=pageTemplate;size=paper;target=targetId;retained=callback;
        const auto action=onCall;
        if(action)action();
        if(mode==Async)QTimer::singleShot(0,this,[callback]() mutable { callback.call(); });
        else if(mode!=None) { callback.call(); if(mode==Duplicate)callback.call(); }
        if(mode==Throw)throw std::runtime_error("owned throw");
        return mode!=False;
    }
};
}}}

#ifndef READER_JOINED_CREATION_OWNED_CLASSES_ONLY
namespace {
using reader_joined_creation::Witness;
using reader_joined_creation::Request;
using Controller=xofm::libs::library::DocumentController;
int checks=0;
void require(bool condition,const char *name) { if(!condition)qFatal("FAIL %s",name);++checks; }
void pump(int ms=20) {
    QElapsedTimer elapsed;elapsed.start();
    do { QCoreApplication::processEvents(QEventLoop::AllEvents,1);QThread::msleep(1); } while(elapsed.elapsed()<ms);
}
Request requestFor(const QmlDocumentWrapper &doc) {
    Request r;r.documentId=doc.key.text;r.expectedPages=doc.pages;r.sourceIndex=4;r.sourcePageId=doc.pages[4];
    r.targetPageId=QStringLiteral("10000000-0000-0000-0000-000000000009");return r;
}
reader_selected::Observation beforeFor(const Request &r) {
    reader_selected::Observation o;o.documentId=r.documentId;o.pageId=r.sourcePageId;o.pageIndex=r.sourceIndex;o.pageCount=r.expectedPages.size();
    o.observed=o.creationJoinObserved=o.mapCoherent=o.ready=true;
    o.loading=o.pageError=o.documentError=o.awaitingTiles=o.pendingTiles=o.working=false;return o;
}
struct Fixture {
    QQmlEngine engine;
    QmlDocumentWrapper doc;
    Controller controller;
    Request request=requestFor(doc);
    reader_selected::Observation before=beforeFor(request);
    Witness witness;
    int finished=0;
    Fixture() { require(witness.prepare(&engine),"prepare");QObject::connect(&witness,&Witness::finished,[this]{++finished;}); }
    bool submit(const std::function<bool()> &guard=[] {return true;},int budget=100) {
        return witness.submit(&controller,&doc,QVariant::fromValue(doc.key),before,request,guard,budget);
    }
};
}
int main(int argc,char **argv) {
    QGuiApplication app(argc,argv);
    qRegisterMetaType<entry::Id>("entry::Id");
    QMetaType::registerConverter<entry::Id,QString>([](const entry::Id &id){return id.text;});
    {
        Fixture f;require(f.request.valid(),"valid request");require(f.submit(),"async invocation returned");require(f.finished==0,"no sync finished");pump();
        const auto r=f.witness.snapshot();require(r.finished && r.attempted && r.returnKnown && r.accepted && r.callbackObserved && !r.unknown && !r.duplicate && r.callbackCount==1,"async positive");
        require(f.controller.calls==1 && f.controller.lastId==f.doc.key && f.controller.index==5 && f.controller.lastTemplate==f.doc.pageTemplate && f.controller.target==f.request.targetPageId && f.controller.size==QSizeF(1404,1872),"exact arguments");
        require(!f.submit() && f.controller.calls==1 && f.finished==1,"no retry");
        f.controller.retained.call();pump();require(f.witness.snapshot().callbackCount==1 && f.finished==1,"frozen late callback facts");
    }
    for(auto mode:{Controller::Sync,Controller::False,Controller::Throw,Controller::Duplicate,Controller::None}) {
        Fixture f;f.controller.mode=mode;
        f.controller.onCall=[&]{QCoreApplication::processEvents();require(f.finished==0,"no completion on invoke stack");};
        const bool invoked=f.submit([]{return true;},30);require(invoked==(mode!=Controller::Throw),"return mechanics");
        require(f.finished==0,"finish queued after unwind");pump(45);auto r=f.witness.snapshot();
        require(r.finished && r.attempted && f.finished==1 && f.controller.calls==1,"mode terminal once");
        if(mode==Controller::Sync)require(r.accepted && r.returnKnown && r.callbackObserved && !r.unknown,"sync positive");
        if(mode==Controller::False)require(r.returnKnown && !r.accepted && r.callbackObserved && r.unknown,"false distinct from callback");
        if(mode==Controller::Throw)require(!r.returnKnown && r.callbackObserved && r.unknown,"throw after effect unknown");
        if(mode==Controller::Duplicate)require(r.duplicate && r.callbackCount==2 && r.unknown,"duplicate refusal");
        if(mode==Controller::None)require(r.returnKnown && r.accepted && !r.callbackObserved && r.unknown && r.refusal=="deadline","no callback deadline");
    }
    for(int variant=0;variant<11;++variant) {
        Fixture f;
        if(variant==0)f.request.targetPageId=f.request.sourcePageId;
        if(variant==1)f.request.expectedPages[0]=f.request.expectedPages[1];
        if(variant==2)f.request.paperSize=QSizeF(-1,1872);
        if(variant==3)f.before.ready=false;
        if(variant==4)f.doc.pages.swapItemsAt(0,1);
        if(variant==5)f.doc.badRoundtrip=true;
        if(variant==6)f.doc.wrongCount=true;
        if(variant==7)f.doc.key.text=QStringLiteral("10000000-0000-0000-0000-000000000101");
        if(variant==8)f.doc.pageTemplate=QString(1025,QLatin1Char('a'));
        if(variant==9)f.request.targetPageId=QStringLiteral("not-an-id");
        if(variant==10)f.request.sourceIndex=-1;
        require(!f.submit(),"invalid preflight refuses");pump();auto r=f.witness.snapshot();
        require(r.finished && !r.attempted && !r.unknown && f.controller.calls==0,"no mutation on preflight error");
    }
    {
        Fixture f;require(!f.submit([]{return false;}),"cancel guard");pump();require(!f.witness.snapshot().attempted && !f.witness.snapshot().unknown,"guard no effect");
    }
    {
        Fixture f;bool once=false;
        require(!f.submit([&]{if(!once){once=true;f.witness.cancel();pump(2);}return true;}),"cancel reentry");pump();require(f.finished==1 && f.controller.calls==0,"cancel reentry once");
    }
    {
        Fixture f;bool once=false;
        require(!f.submit([&]{if(!once){once=true;require(!f.submit(),"nested submit refused");pump(2);}return true;}),"nested guard cancels original");pump();require(f.finished==1 && f.controller.calls==0,"nested submit no mutation");
    }
    {
        Fixture f;require(!f.submit([]{QThread::msleep(3);return true;},2),"cooperative expired guard");pump();require(!f.witness.snapshot().attempted,"deadline before effect");
    }
    {
        Fixture f;f.controller.mode=Controller::None;require(f.submit(),"cancel after accepted");f.witness.cancel();pump();
        const auto r=f.witness.snapshot();require(r.canceled && r.unknown && r.returnKnown && r.accepted && !r.callbackObserved,"cancel pending effect unknown");
        f.controller.retained.call();pump();require(f.finished==1 && !f.witness.snapshot().callbackObserved,"retained callback after cancel safe");
    }
    {
        QQmlEngine engine;QmlDocumentWrapper doc;Controller c;c.mode=Controller::None;
        auto w=std::make_unique<Witness>();require(w->prepare(&engine),"destroy fixture prepare");auto req=requestFor(doc);auto before=beforeFor(req);
        require(w->submit(&c,&doc,QVariant::fromValue(doc.key),before,req,[]{return true;},100),"destroy fixture submit");
        const QJSValue callback=c.retained;w.reset();require(!callback.call().isError(),"late JS after bridge destruction safe");pump();
    }
    {
        QQmlEngine engine;QmlDocumentWrapper doc;Controller c;auto *w=new Witness;QPointer<Witness> weak(w);require(w->prepare(&engine),"guard deletion prepare");auto req=requestFor(doc);auto before=beforeFor(req);
        require(!w->submit(&c,&doc,QVariant::fromValue(doc.key),before,req,[&]{delete w;return true;},100) && !weak && c.calls==0,"guard deletion safe");
    }
    {
        Fixture f;bool once=false;f.doc.onRead=[&]{if(!once){once=true;f.witness.cancel("getter-cancel");pump(2);}};
        require(!f.submit(),"getter cancel no invoke");pump();require(f.controller.calls==0 && f.finished==1,"getter cancel terminal");
    }
    {
        Fixture f;QVariant wrong=QStringLiteral("wrong");require(!f.witness.submit(&f.controller,&f.doc,wrong,f.before,f.request,[]{return true;},100),"actual metatype refused");pump();require(f.controller.calls==0,"wrong type no calls");
    }
    {
        Fixture f;auto engine=std::make_unique<QQmlEngine>();Witness w;require(w.prepare(engine.get()),"engine death prepare");engine.reset();pump();require(w.snapshot().finished && w.snapshot().canceled,"engine death disarmed");
    }
    {
        Fixture f;QQmlEngine foreign;
        QQmlEngine::setContextForObject(&f.controller,foreign.rootContext());
        require(!f.submit(),"foreign engine refused");pump();require(f.controller.calls==0 && !f.witness.snapshot().attempted,"foreign no effect");
    }
    {
        Fixture f;QObject wrong;
        require(!f.witness.submit(&wrong,&f.doc,QVariant::fromValue(f.doc.key),f.before,f.request,[]{return true;},100),"foreign controller refused");pump();require(!f.witness.snapshot().attempted,"foreign controller no effect");
    }
    {
        Fixture f;f.witness.cancel();pump();require(f.finished==1 && !f.submit() && f.controller.calls==0,"prestart cancel consumes");
    }
    for(int budget:{0,30001}) {
        Fixture f;require(!f.submit([]{return true;},budget),"budget bounds");pump();require(f.controller.calls==0 && !f.witness.snapshot().attempted,"invalid budget no effect");
    }
    {
        QQmlEngine engine;QmlDocumentWrapper doc;Controller c;auto *w=new Witness;QPointer<Witness> weak(w);require(w->prepare(&engine),"getter destruction prepare");auto req=requestFor(doc);auto before=beforeFor(req);
        doc.onRead=[&]{delete w;};
        require(!w->submit(&c,&doc,QVariant::fromValue(doc.key),before,req,[]{return true;},100) && !weak && c.calls==0,"getter destruction safe");
    }
    {
        QQmlEngine engine;QmlDocumentWrapper doc;Controller c;c.mode=Controller::Sync;auto *w=new Witness;QPointer<Witness> weak(w);require(w->prepare(&engine),"invoke destruction prepare");auto req=requestFor(doc);auto before=beforeFor(req);
        c.onCall=[&]{delete w;};
        require(!w->submit(&c,&doc,QVariant::fromValue(doc.key),before,req,[]{return true;},100) && !weak && c.calls==1,"invoke destruction safe");
        require(!c.retained.call().isError(),"retained after in-call destruction safe");
    }
    {
        Fixture f;bool active=true;int guardCalls=0;
        require(f.submit([&]{require(active,"guard only synchronous");++guardCalls;return true;}),"guard lifetime submit");active=false;const int count=guardCalls;pump();
        require(f.witness.snapshot().callbackObserved && guardCalls==count,"no async borrowed guard retention");
    }
    QTextStream(stdout)<<"PASS joined creation owned checks="<<checks<<"; no native qualification\n";
    return 0;
}
#endif
#include "reader_joined_creation_witness_fixture.moc"
