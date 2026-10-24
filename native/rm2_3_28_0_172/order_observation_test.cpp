// Reuse the handoff's owned Qt wrappers; no proprietary runtime or native calls.
#define main retained_post_order_tests
#include "reader_insertion_post_order_fixture.cpp"
#undef main
#include "order_observation.h"
using remarkable::rm2_3_28_0_172::observeOrder;
int main(int argc,char **argv) {
    QGuiApplication app(argc,argv);qRegisterMetaType<entry::Id>("entry::Id");
    QMetaType::registerConverter<entry::Id,QString>([](const entry::Id &id){return id.text;});
    const auto run=[](Fixture &f,const std::function<bool()> &permit=[] {return true;}) {
        return observeOrder(f.engine.get(),f.doc.get(),f.id,f.joined,permit);
    };
    for(int size=1;size<=17;++size) {
        Fixture f;f.doc->pages.clear();for(int i=1;i<=size;++i)f.doc->pages.append(pageId(i));
        f.joined.pageCount=size;f.joined.pageIndex=size/2;f.joined.pageId=f.doc->pages[size/2];
        const auto result=run(f);check(result.observed&&result.refusal.isEmpty(),"bounded full order observed");
        check(result.facts.order.size()==std::size_t(size)&&result.facts.page==QUuid(f.joined.pageId).toRfc4122(),"actual selection retained");
        for(int i=0;i<size;++i)check(result.facts.order[i]==QUuid(f.doc->pages[i]).toRfc4122(),"all IDs retained");
    }
    for(int fault=0;fault<10;++fault) {
        Fixture f;
        if(fault==0)f.joined.pageCount=18;
        if(fault==1)f.joined.pageCount=0;
        if(fault==2)f.joined.pageIndex=-1;
        if(fault==3)f.joined.ready=false;
        if(fault==4)f.joined.creationJoinObserved=false;
        if(fault==5)f.joined.documentId="invalid";
        if(fault==6)f.joined.pageId="invalid";
        if(fault==7)f.joined.loading=true;
        if(fault==8)f.joined.pendingTiles=true;
        if(fault==9)f.id=QStringLiteral("wrong type");
        const auto result=run(f);check(!result.observed&&result.facts.order.empty(),"invalid selection has no wire facts");
        check(f.doc->reads==0,"invalid selection performs no getters");
    }
    for(int fault=0;fault<7;++fault) {
        Fixture f;
        if(fault==0)f.doc->pages[1]=f.doc->pages[0];
        if(fault==1)f.doc->pages[1]="invalid";
        if(fault==2)f.doc->badInverse=true;
        if(fault==3)f.joined.pageId=pageId(88);
        if(fault==4)f.doc->pages.append(pageId(88));
        if(fault==5)f.doc->onRead=[&](const QString&,unsigned n){if(n==21)f.doc->pages.swapItemsAt(0,1);};
        if(fault==6)f.doc->key.text=pageId(88);
        const auto result=run(f);check(!result.observed&&result.facts.order.empty(),"changed identity/order returns no wire facts");
    }
    unsigned calls=0;{Fixture f;check(run(f).observed,"baseline");calls=f.doc->reads;}
    for(unsigned stop=1;stop<=calls;++stop) {
        Fixture f;bool permitted=true;f.doc->onRead=[&](const QString&,unsigned n){if(n==stop)permitted=false;};
        const auto result=run(f,[&]{return permitted;});check(!result.observed&&result.facts.order.empty(),"loss at every getter refuses");
        check(f.doc->reads==stop,"no subsequent getter after permit loss");
    }
    for(int kind=0;kind<3;++kind) {
        Fixture f;
        if(kind==0)f.doc=std::make_unique<WrongCount>();
        if(kind==1)f.doc=std::make_unique<MutableId>();
        if(kind==2)f.doc=std::make_unique<ShadowMethod>();
        check(!run(f).observed&&f.doc->reads==0,"shadowed metadata refused before getters");
    }
    {
        Fixture f;bool nested=false;
        f.doc->onRead=[&](const QString&,unsigned){if(!nested){nested=true;check(!run(f).observed,"nested read refused");}};
        check(!run(f).observed,"nested read invalidates outer sample");
    }
    {
        Fixture f;f.doc->onRead=[&](const QString&,unsigned n){if(n==4)f.doc.reset();};
        check(!run(f).observed,"deleting wrapper in getter refuses without borrowed access");
    }
    {
        Fixture f;QQmlEngine foreign;QQmlEngine::setContextForObject(f.doc.get(),foreign.rootContext());
        check(!run(f).observed&&f.doc->reads==0,"foreign engine refuses");
    }
    QTextStream(stdout)<<"PASS SDK owned full-order observation checks="<<checks<<"; no native authority\n";
}
