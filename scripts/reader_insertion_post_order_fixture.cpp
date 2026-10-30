// Original public-Qt models; no native object, map, ABI or insertion qualification.
#include "reader_insertion_post_order.h"
#include <QTextStream>
#include <memory>
#include <stdexcept>
#include <thread>

namespace entry {
struct Id {
    QString text;
    bool operator==(const Id &other)const{return text==other.text;}
    bool operator!=(const Id &other)const{return !(*this==other);}
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
    bool badInverse=false;
    mutable unsigned reads=0;
    mutable std::function<void(const QString&,unsigned)> onRead;
    entry::Id id()const{const auto value=key;read(QStringLiteral("id"));return value;}
    int pageCount()const{const int value=pages.size();read(QStringLiteral("count"));return value;}
    Q_INVOKABLE QString idForPage(int index)const {
        const QString value=index>=0&&index<pages.size()?pages[index]:QString();read(QStringLiteral("forward"));return value;
    }
    Q_INVOKABLE int pageForId(QString page)const {
        const int value=badInverse?-2:pages.indexOf(page);read(QStringLiteral("inverse"));return value;
    }
    void read(const QString &kind)const {
        const unsigned ordinal=++reads;const auto action=onRead;
        if(action)action(kind,ordinal); // No member access after a deleting callback.
    }
signals:
    void pageCountChanged();
};
class WrongCount final:public QmlDocumentWrapper {
    Q_OBJECT
    Q_PROPERTY(QString pageCount READ textCount CONSTANT)
public:QString textCount()const{return QStringLiteral("9");}
};
class MutableId final:public QmlDocumentWrapper {
    Q_OBJECT
    Q_PROPERTY(entry::Id id READ mutableId NOTIFY idChanged)
public:entry::Id mutableId()const{return key;}
signals:void idChanged();
};
class ShadowMethod final:public QmlDocumentWrapper {
    Q_OBJECT
public:Q_INVOKABLE QString idForPage(int index)const{return pages.value(index);}
};

namespace {
using reader_insertion_post_order::Result;
using reader_insertion_post_order::observe;
using reader_joined_creation::Request;
unsigned checks=0,scenarios=0;
void check(bool yes,const char *name){++checks;if(!yes)qFatal("FAIL %s",name);}
QString pageId(int index){return QStringLiteral("20000000-0000-0000-0000-%1").arg(index,12,10,QLatin1Char('0'));}
struct Fixture {
    std::unique_ptr<QQmlEngine> engine=std::make_unique<QQmlEngine>();
    std::unique_ptr<QmlDocumentWrapper> doc=std::make_unique<QmlDocumentWrapper>();
    Request request;
    reader_selected::Observation joined;
    QVariant id;
    Fixture(int count=8,int source=4,bool target=false){configure(count,source,target);}
    void configure(int count,int source,bool target=false) {
        request.documentId=doc->key.text;request.sourceIndex=source;
        for(int i=1;i<=count;++i)request.expectedPages.append(pageId(i));
        request.sourcePageId=request.expectedPages[source];request.targetPageId=pageId(99);
        doc->pages=request.expectedPages;doc->pages.insert(source+1,request.targetPageId);
        id=QVariant::fromValue(doc->key);
        joined.observed=joined.creationJoinObserved=joined.mapCoherent=joined.ready=true;
        joined.loading=joined.pageError=joined.documentError=joined.awaitingTiles=joined.pendingTiles=joined.working=false;
        joined.documentId=request.documentId;joined.pageCount=count+1;
        joined.pageId=target?request.targetPageId:request.sourcePageId;joined.pageIndex=source+(target?1:0);
    }
    Result run(const std::function<bool()> &guard=[] {return true;}) {
        ++scenarios;return observe(engine.get(),doc.get(),id,request,joined,guard);
    }
};
void refused(const Result &r,const char *label) {
    check(!r.observed&&!r.refusal.isEmpty(),label);
    check(r.ids.size()<=17&&r.calls<=39,"bounded partial result");
    check(!r.nativeAuthority&&!r.renderAuthority&&!r.durableAuthority,"no authority flags");
}
}
int main(int argc,char **argv) {
    QGuiApplication app(argc,argv);
    qRegisterMetaType<entry::Id>("entry::Id");
    QMetaType::registerConverter<entry::Id,QString>([](const entry::Id &id){return id.text;});
    for(const auto &input:{std::array<int,3>{8,4,0},{8,4,1},{8,7,0},{8,7,1},{1,0,0},{1,0,1},{16,15,0},{16,8,1}}) {
        Fixture f(input[0],input[1],input[2]);const auto r=f.run();
        check(r.observed&&r.refusal.isEmpty(),"middle/end/single/cap complete post-order");
        check(r.targetSelected==bool(input[2])&&r.currentPageId==f.joined.pageId&&r.currentPageIndex==f.joined.pageIndex,"selection facts distinct");
        check(r.ids==f.doc->pages&&r.pageCount==input[0]+1,"entire expected order retained");
        check(r.calls==unsigned(2*(input[0]+1)+5)&&f.doc->reads==r.calls,"exact bounded public call count");
        check(!r.nativeAuthority&&!r.renderAuthority&&!r.durableAuthority,"positive observation grants no authority");
    }
    for(int fault=0;fault<10;++fault) {
        Fixture f;
        if(fault==0)f.request.expectedPages[0]=f.request.expectedPages[1];
        if(fault==1)f.request.expectedPages[0]=QStringLiteral("bad");
        if(fault==2)f.request.sourceIndex=-1;
        if(fault==3)f.request.targetPageId=f.request.sourcePageId;
        if(fault==4)f.request.sourcePageId=pageId(98);
        if(fault==5)f.request.documentId=QStringLiteral("bad");
        if(fault==6)f.request.paperSize=QSizeF(-1,1);
        if(fault==7)f.request.expectedPages.clear();
        if(fault==8){f.request.expectedPages.append(pageId(77));f.request.expectedPages.append(pageId(78));for(int n=10;n<17;++n)f.request.expectedPages.append(pageId(n));}
        if(fault==9)f.request.targetPageId=QStringLiteral("00000000-0000-0000-0000-000000000000");
        const auto r=f.run();refused(r,"malformed request refuses");check(!r.calls&&!f.doc->reads,"request before any getter");
    }
    for(int fault=0;fault<17;++fault) {
        Fixture f;
        bool *flags[]={&f.joined.observed,&f.joined.creationJoinObserved,&f.joined.mapCoherent,&f.joined.ready};
        bool *errors[]={&f.joined.loading,&f.joined.pageError,&f.joined.documentError,&f.joined.awaitingTiles,&f.joined.pendingTiles,&f.joined.working};
        if(fault<4)*flags[fault]=false;
        else if(fault<10)*errors[fault-4]=true;
        else if(fault==10)f.joined.documentId=pageId(88);
        else if(fault==11)f.joined.pageCount=8;
        else if(fault==12)f.joined.pageId=pageId(88);
        else if(fault==13)f.joined.pageIndex=5;
        else if(fault==14){f.joined.pageId=f.request.targetPageId;f.joined.pageIndex=4;}
        else if(fault==15)f.joined.pageIndex=-1;
        else f.joined.pageCount=10;
        const auto r=f.run();refused(r,"fresh joined prerequisites required");check(!r.calls&&!f.doc->reads,"joined refusal no getters");
    }
    for(int fault=0;fault<8;++fault) {
        Fixture f;
        if(fault==0)f.doc->pages.swapItemsAt(0,1);
        if(fault==1)f.doc->pages.swapItemsAt(5,6);
        if(fault==2)f.doc->pages[0]=pageId(77);
        if(fault==3)f.doc->pages[0]=f.request.targetPageId;
        if(fault==4)f.doc->pages.removeLast();
        if(fault==5)f.doc->pages.append(pageId(77));
        if(fault==6)f.doc->pages[0]=QStringLiteral("bad");
        if(fault==7)f.doc->badInverse=true;
        const auto r=f.run();refused(r,"reorder/loss/substitution/duplicate/count/inverse refuses");
        check(r.pageCount>=0,"initial actual count retained");
    }
    for(int fault=0;fault<6;++fault) {
        Fixture f;
        if(fault==0)f.id=QVariant();
        if(fault==1)f.id=QStringLiteral("wrong");
        if(fault==2)f.id=QVariant::fromValue(f.doc.get());
        if(fault==3)f.id=QVariant::fromValue(entry::Id{pageId(88)});
        if(fault==4){f.doc->key.text=pageId(88);f.id=QVariant::fromValue(f.doc->key);}
        if(fault==5)f.doc->onRead=[&](const QString&,unsigned n){if(n==23)f.doc->key.text=pageId(88);};
        const auto r=f.run();
        if(fault==5)check(r.observed,"constant getter value captured before owned mutation is not an atomic snapshot");
        else refused(r,"typed actual id/text mismatch refuses");
    }
    for(int fault=0;fault<3;++fault) {
        Fixture f;
        if(fault==0)f.doc=std::make_unique<WrongCount>();
        if(fault==1)f.doc=std::make_unique<MutableId>();
        if(fault==2)f.doc=std::make_unique<ShadowMethod>();
        const auto r=f.run();refused(r,"shadowed contract refused");check(!r.calls&&!f.doc->reads,"metadata refusal before getters");
    }
    {
        Fixture f;QObject unrelated;const auto r=observe(f.engine.get(),&unrelated,f.id,f.request,f.joined,[]{return true;});++scenarios;
        refused(r,"wrong native class refused");check(!r.calls,"wrong class no public data calls");
    }
    {
        Fixture f;QQmlEngine foreign;QQmlEngine::setContextForObject(f.doc.get(),foreign.rootContext());
        const auto r=f.run();refused(r,"foreign associated engine refused");check(!f.doc->reads,"foreign engine before getter");
    }
    {
        Fixture f;bool permit=true;
        f.doc->onRead=[&](const QString &kind,unsigned n){if(kind==QStringLiteral("forward")&&n==4)f.doc->pages.append(pageId(77));};
        const auto r=f.run([&]{return permit;});refused(r,"final count catches appended page");check(r.ids.size()==9&&r.refusal==QStringLiteral("page-count-changed"),"full partial facts preserved on final count mismatch");
    }
    {
        Fixture f;f.doc->onRead=[&](const QString &kind,unsigned n){if(kind==QStringLiteral("inverse")&&n==21)f.doc->key.text=pageId(88);};
        const auto r=f.run();refused(r,"final constant id recheck");check(r.refusal==QStringLiteral("document-id-changed"),"id change reason exact");
    }
    for(unsigned stop=1;stop<=23;++stop) {
        Fixture f;bool permit=true;f.doc->onRead=[&](const QString&,unsigned n){if(n==stop)permit=false;};
        const auto r=f.run([&]{return permit;});refused(r,"guard loss after each property/evaluation/method call");
        check(f.doc->reads==stop&&r.calls==stop,"no call after failed post-call guard");
    }
    {
        Fixture f;const auto r=f.run([]{return false;});refused(r,"initial canceled guard");check(!r.calls,"no call under canceled guard");
    }
    {
        Fixture f;const auto r=f.run([]()->bool{throw std::runtime_error("guard");});refused(r,"guard exception contained");check(r.refusal==QStringLiteral("exception")&&!r.calls,"guard exception before getter");
    }
    {
        Fixture f;f.doc->onRead=[](const QString&,unsigned){throw std::runtime_error("getter");};
        const auto r=f.run();refused(r,"getter exception contained");check(r.calls==1,"throwing getter exactly once");
        f.doc->onRead={};check(f.run().observed,"exception unwinds reentry gate");
    }
    {
        Fixture f;bool nested=false;
        const auto r=f.run([&]{if(!nested){nested=true;const auto inner=f.run();refused(inner,"nested observe refused");}return true;});
        refused(r,"reentry poisons outer");check(!r.calls,"reentry no getter");check(f.run().observed,"next independent observe allowed");
    }
    {
        Fixture f;bool nested=false;
        f.doc->onRead=[&](const QString&,unsigned){if(!nested){nested=true;refused(f.run(),"getter reentry refused");}};
        const auto r=f.run();refused(r,"getter reentry poisons outer");check(r.calls==1,"no getters after reentry");
    }
    for(int what=0;what<4;++what) {
        Fixture f;bool first=true;
        const auto r=f.run([&]{if(first){first=false;if(what==0)f.doc.reset();if(what==1)f.engine.reset();if(what==2)QQmlEngine::setContextForObject(f.doc.get(),nullptr);if(what==3)throw std::runtime_error("deadline");}return what!=2;});
        refused(r,"guard deletion/expired permission safe");check(!r.calls,"deleted/expired initial objects no calls");
    }
    for(int what=0;what<2;++what) {
        Fixture f;f.doc->onRead=[&](const QString&,unsigned){if(what==0)f.doc.reset();else f.engine.reset();};
        const auto r=f.run();refused(r,"getter object/engine destruction safe");check(r.calls==1,"no call after getter destruction");
    }
    {
        Fixture f;auto owner=std::make_unique<std::array<int,1>>();bool once=false;
        struct Inputs {Request request;reader_selected::Observation joined;QVariant id;std::function<bool()> guard;};
        auto inputs=std::make_unique<Inputs>();inputs->request=f.request;inputs->joined=f.joined;inputs->id=f.id;
        inputs->guard=[&]{if(!once){once=true;inputs.reset();owner.reset();}return true;};
        ++scenarios;const auto r=observe(f.engine.get(),f.doc.get(),inputs->id,inputs->request,inputs->joined,inputs->guard);
        check(r.observed&&!inputs&&!owner,"owned input and permission copies survive caller destruction");
    }
    {
        Fixture f;bool active=true;unsigned calls=0;
        const auto r=f.run([&]{check(active,"borrowed guard only synchronous");++calls;return true;});
        check(r.observed&&calls>0,"guard used synchronously");active=false;const unsigned before=calls;QCoreApplication::processEvents();
        check(calls==before,"guard not retained for queued work");
    }
    {
        Fixture f;Result result;std::thread foreign([&]{result=f.run();});foreign.join();
        refused(result,"non-GUI caller refused");check(!result.calls&&!f.doc->reads,"wrong thread no getters");
    }
    {
        Fixture f;QThread worker;worker.start();f.doc->moveToThread(&worker);const auto r=f.run();
        refused(r,"foreign document affinity refused");check(!f.doc->reads,"foreign affinity no getters");
        QMetaObject::invokeMethod(f.doc.get(),[&]{f.doc->moveToThread(app.thread());},Qt::BlockingQueuedConnection);worker.quit();worker.wait();
    }
    QTextStream(stdout)<<"PASS owned GUI post-order scenarios="<<scenarios<<" checks="<<checks<<"; native/render/durable authority=0\n";
}
#include "reader_insertion_post_order_fixture.moc"
