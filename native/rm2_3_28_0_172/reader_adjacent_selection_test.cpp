// Owned Qt objects only; this does not instantiate a native Reader or executor.
#define main retained_order_tests
#include "reader_insertion_post_order_fixture.cpp"
#undef main
#include "reader_adjacent_selection.h"
namespace nav=reader_insertion_target_acquisition;
using remarkable::rm2_3_28_0_172::dispatchAdjacent;
using remarkable::rm2_3_28_0_172::observeOrder;
class SelectionReader:public QObject {
    Q_OBJECT
public:
    unsigned calls=0;
    std::function<void()> invoked;
    Q_INVOKABLE void moveForward(){++calls;auto call=invoked;if(call)call();}
};
class VariantSelectionReader:public QObject {
    Q_OBJECT
public:
    unsigned calls=0;
    Q_INVOKABLE QVariant moveForward(){++calls;return QStringLiteral("ignored historical return");}
};
int main(int argc,char **argv) {
    QGuiApplication app(argc,argv);qRegisterMetaType<entry::Id>("entry::Id");
    QMetaType::registerConverter<entry::Id,QString>([](const entry::Id &id){return id.text;});
    const auto read=[](Fixture &f){return observeOrder(f.engine.get(),f.doc.get(),f.id,f.joined,[]{return true;});};
    const auto adjacent=[](const remarkable::resident::Facts &facts){
        const auto at=std::find(facts.order.begin(),facts.order.end(),facts.page);
        check(at!=facts.order.end()&&at+1!=facts.order.end(),"fixture successor");return *(at+1);
    };
    {
        Fixture f;SelectionReader reader;auto order=read(f);auto claim=std::make_shared<nav::Claim>();
        const auto method=nav::inspect(f.engine.get(),&reader,[]{return true;});check(method.admitted,"owned method admitted");
        const auto result=dispatchAdjacent(f.engine.get(),&reader,f.doc.get(),order.facts,adjacent(order.facts),order,method,claim,[]{return true;});
        check(result.admitted&&result.attempted&&result.dispatched&&!result.unknown&&reader.calls==1,"exactly one owned call");
        check(!dispatchAdjacent(f.engine.get(),&reader,f.doc.get(),order.facts,adjacent(order.facts),order,method,claim,[]{return true;}).attempted&&reader.calls==1,"claim cannot repeat");
    }
    {
        Fixture f;QObject reader;auto order=read(f);auto claim=std::make_shared<nav::Claim>();
        const auto result=dispatchAdjacent(f.engine.get(),&reader,f.doc.get(),order.facts,order.facts.page,order,{},claim,[]{return true;});
        check(result.admitted&&result.alreadySelected&&!result.attempted&&!claim->consumed,"current target needs zero method calls");
    }
    for(int fault=0;fault<10;++fault) {
        Fixture f;SelectionReader reader;auto order=read(f);auto expected=order.facts;auto target=adjacent(expected);
        auto method=nav::inspect(f.engine.get(),&reader,[]{return true;});auto claim=std::make_shared<nav::Claim>();
        if(fault==0)target=expected.order.front();
        if(fault==1)target=QUuid(pageId(88)).toRfc4122();
        if(fault==2)expected.document=QUuid(pageId(88)).toRfc4122();
        if(fault==3)order.facts.page=target;
        if(fault==4)std::swap(order.facts.order[0],order.facts.order[1]);
        if(fault==5)order.observed=false;
        if(fault==6)order.refusal=QStringLiteral("refused");
        if(fault==7)method.index=-1;
        if(fault==8)claim->consumed=true;
        const auto result=dispatchAdjacent(f.engine.get(),&reader,f.doc.get(),expected,target,order,method,claim,[fault]{return fault!=9;});
        check(!result.admitted&&!result.attempted&&reader.calls==0,"invalid precondition has zero effects");
    }
    for(int loss=0;loss<4;++loss) {
        Fixture f;auto reader=std::make_unique<SelectionReader>();auto order=read(f);auto claim=std::make_shared<nav::Claim>();
        const auto method=nav::inspect(f.engine.get(),reader.get(),[]{return true;});bool first=true;
        const auto guard=[&]{if(first){first=false;if(loss==0)reader.reset();if(loss==1)f.doc.reset();if(loss==2)f.engine.reset();if(loss==3)throw std::runtime_error("guard");}return true;};
        const auto result=dispatchAdjacent(f.engine.get(),reader.get(),f.doc.get(),order.facts,adjacent(order.facts),order,method,claim,guard);
        check(!result.attempted&&!claim->consumed,"deleted input or throwing permission refuses before effect");
    }
    for(int after=0;after<3;++after) {
        Fixture f;auto reader=std::make_unique<SelectionReader>();auto order=read(f);auto claim=std::make_shared<nav::Claim>();
        const auto method=nav::inspect(f.engine.get(),reader.get(),[]{return true;});bool epoch=true;
        reader->invoked=[&]{epoch=false;if(after==0)reader.reset();if(after==1)f.doc.reset();if(after==2)throw std::runtime_error("invoke");};
        const auto result=dispatchAdjacent(f.engine.get(),reader.get(),f.doc.get(),order.facts,adjacent(order.facts),order,method,claim,[&]{check(epoch,"old guard must not run after invoke");return true;});
        check(result.attempted&&claim->consumed,"invocation consumes claim despite epoch loss");
        check(after==2?result.unknown:result.dispatched,"exception unknown; successful dispatch still only historical");
    }
    {
        Fixture f;SelectionReader reader;auto order=read(f);auto claim=std::make_shared<nav::Claim>();
        const auto method=nav::inspect(f.engine.get(),&reader,[]{return true;});bool nested=false;
        const auto guard=[&]{if(!nested){nested=true;check(!dispatchAdjacent(f.engine.get(),&reader,f.doc.get(),order.facts,adjacent(order.facts),order,method,claim,[]{return true;}).attempted,"nested call refuses");}return true;};
        check(!dispatchAdjacent(f.engine.get(),&reader,f.doc.get(),order.facts,adjacent(order.facts),order,method,claim,guard).attempted&&reader.calls==0,"reentry poisons outer before effect");
    }
    {
        Fixture f;SelectionReader reader;QQmlEngine other;auto order=read(f);auto claim=std::make_shared<nav::Claim>();
        const auto method=nav::inspect(f.engine.get(),&reader,[]{return true;});QQmlEngine::setContextForObject(&reader,other.rootContext());
        check(!dispatchAdjacent(f.engine.get(),&reader,f.doc.get(),order.facts,adjacent(order.facts),order,method,claim,[]{return true;}).attempted&&reader.calls==0,"foreign engine refuses");
    }
    {
        Fixture f;VariantSelectionReader reader;auto order=read(f);auto claim=std::make_shared<nav::Claim>();
        const auto method=nav::inspect(f.engine.get(),&reader,[]{return true;});
        const auto result=dispatchAdjacent(f.engine.get(),&reader,f.doc.get(),order.facts,adjacent(order.facts),order,method,claim,[]{return true;});
        check(method.returnTypeId==QMetaType::QVariant&&result.dispatched&&reader.calls==1,"QVariant invocation uses owned ignored return storage");
    }
    for(int count : {17,18}) {
        Fixture f;SelectionReader reader;auto order=read(f);order.facts.order.clear();
        for(int index=1;index<=count;++index)order.facts.order.push_back(QUuid(pageId(index)).toRfc4122());
        order.facts.page=order.facts.order[0];auto claim=std::make_shared<nav::Claim>();
        const auto method=nav::inspect(f.engine.get(),&reader,[]{return true;});
        const auto result=dispatchAdjacent(f.engine.get(),&reader,f.doc.get(),order.facts,order.facts.order[1],order,method,claim,[]{return true;});
        check(count==17?result.dispatched&&reader.calls==1:!result.attempted&&reader.calls==0,"inner-step bound admits17 refuses18 even with supplied observed facts");
    }
    {
        Fixture f;SelectionReader reader;
        struct Inputs {
            remarkable::resident::Facts expected;
            remarkable::resident::Id target;
            remarkable::rm2_3_28_0_172::OrderObservation order;
            nav::MethodInfo method;
            std::shared_ptr<nav::Claim> claim;
            std::function<bool()> guard;
        };
        auto inputs=std::make_unique<Inputs>();inputs->order=read(f);inputs->expected=inputs->order.facts;
        inputs->target=adjacent(inputs->expected);inputs->method=nav::inspect(f.engine.get(),&reader,[]{return true;});
        inputs->claim=std::make_shared<nav::Claim>();auto retained=inputs->claim;
        inputs->guard=[&]{inputs.reset();return true;};
        const auto result=dispatchAdjacent(f.engine.get(),&reader,f.doc.get(),inputs->expected,inputs->target,inputs->order,inputs->method,inputs->claim,inputs->guard);
        check(!inputs&&result.dispatched&&retained->consumed&&reader.calls==1,"copied inputs survive owner destruction inside permission");
    }
    QTextStream(stdout)<<"PASS owned adjacent inner-step checks="<<checks<<"; no selection/native/write authority\n";
    return 0;
}
#include "reader_adjacent_selection_test.moc"
