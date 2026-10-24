#define main retained_post_order_tests
#include "reader_insertion_post_order_fixture.cpp"
#undef main
#include "resident_selection.h"
namespace resident=remarkable::resident;
class OwnedSelectionReader:public QObject {
    Q_OBJECT
public:
    unsigned *calls=nullptr;std::function<void()> invoke;
    Q_INVOKABLE void moveForward(){++*calls;auto call=invoke;if(call)call();}
};
namespace remarkable::rm2_3_28_0_172::testing {
struct SelectionAccess {
    static auto *gate(){return ResidentSelection::ownedGate();}
    static void destroyGate(ResidentEffectGate *g){ResidentSelection::destroyOwnedGate(g);}
    static auto *make(ResidentEffectGate &g,ResidentSelection::Join join){return new ResidentSelection(g,std::move(join));}
    static void destroy(ResidentSelection *value){delete value;}
    static bool active(ResidentSelection &value){return bool(value.active_);}
};
}
static void pump(int ms=3){QElapsedTimer timer;timer.start();do{QCoreApplication::processEvents(QEventLoop::AllEvents,1);QThread::msleep(1);}while(timer.elapsed()<ms);}
int main(int argc,char **argv){
    QGuiApplication app(argc,argv);qRegisterMetaType<entry::Id>("entry::Id");
    QMetaType::registerConverter<entry::Id,QString>([](const entry::Id &id){return id.text;});
    using namespace remarkable::rm2_3_28_0_172;using A=testing::SelectionAccess;
    check(!ResidentSelection::create(nullptr),"unqualified production factory stays closed");
    for(const QString mode : {"zero","next","wait","wrong-target","order-change","cancel","delete-reader","replace-reader","foreign-document","throw","deadline","duplicate","pre-cancel","stale-source","pre-wait-timeout","post-wait-timeout",
            "nested-getter-zero","nested-getter-next","nested-invoke","refused-zero","refused-before","refused-after","deadline-zero","deadline-after"}) {
        Fixture f;QQuickWindow window;QQmlEngine::setContextForObject(&window,f.engine->rootContext());
        window.show();window.requestActivate();pump();check(QGuiApplication::focusWindow()==&window,"owned focused window");
        auto reader=std::make_unique<OwnedSelectionReader>();unsigned calls=0,joins=0,delivered=0;reader->calls=&calls;
        bool permitted=mode!="pre-cancel",sample=false,captureDestroyed=false;
        std::function<void()> duplicate;bool reentered=false;
        auto *gate=A::gate();resident::Frame result;QObject foreignDocument;
        auto join=[&](QQuickWindow *actual,const std::function<bool()> &permit,int budget,const reader_selected::ValidatedSourceAction &action){
            check(actual==&window&&budget>0&&budget<=reader_selected::MaxObservationMs,"original window and bounded remaining budget");++joins;
            if((mode=="wait"&&joins>1&&joins<4)||mode=="pre-wait-timeout"||(mode=="post-wait-timeout"&&joins>1)){reader_selected::Observation waiting;waiting.refusal="worker-source-unready";return waiting;}
            sample=true;const auto unwind=qScopeGuard([&]{sample=false;});
            const auto guard=[&]{check(sample,"borrowed guard stays in Sample");return permit();};
            duplicate=[&]{check(sample&&!reentered,"reentrant callback remains synchronous");reentered=true;action(reader.get(),f.doc.get(),nullptr,f.id,f.joined,guard);};
            const auto clearDuplicate=qScopeGuard([&]{duplicate={};});
            action(reader.get(),mode=="foreign-document"&&joins>1?&foreignDocument:f.doc.get(),nullptr,f.id,f.joined,guard);
            if(mode=="duplicate"&&joins==1)action(reader.get(),f.doc.get(),nullptr,f.id,f.joined,guard);
            pump();check(delivered==0,"completion waits for Sample unwind, including nested loop");
            if((mode=="refused-zero"||mode=="refused-before"||mode=="deadline-zero")||
               ((mode=="refused-after"||mode=="deadline-after")&&joins>1)){
                reader_selected::Observation refused;refused.refusal=mode.startsWith("deadline-")?"deadline":"creation-preparation-refused";return refused;
            }
            return f.joined;
        };
        auto *executor=A::make(*gate,join),*other=A::make(*gate,join);
        const auto order=observeOrder(f.engine.get(),f.doc.get(),f.id,f.joined,[]{return true;});check(order.observed,"fixture initial full order");
        resident::Frame request;request.message=2;request.kind=3;request.operation=QByteArray(16,1);request.facts=order.facts;
        const auto at=std::find(request.facts.order.begin(),request.facts.order.end(),request.facts.page);
        check(at+1!=request.facts.order.end(),"fixture next page");request.target=mode=="zero"||mode.endsWith("-zero")?request.facts.page:*(at+1);
        f.doc->onRead=[&](const QString&,unsigned){if(mode.startsWith("nested-getter")&&duplicate&&!reentered)duplicate();};
        if(mode=="stale-source"){request.facts.page=request.facts.order[0];request.target=request.facts.order[1];}
        reader->invoke=[&]{
            if(mode=="nested-invoke")duplicate();
            if(mode!="wrong-target"){++f.joined.pageIndex;f.joined.pageId=f.doc->pages[f.joined.pageIndex];}
            if(mode=="order-change")f.doc->pages.swapItemsAt(0,1);
            if(mode=="cancel")permitted=false;
            if(mode=="delete-reader")reader.reset();
            if(mode=="replace-reader"){reader=std::make_unique<OwnedSelectionReader>();reader->calls=&calls;}
            if(mode=="deadline")QThread::msleep(100);
            if(mode=="throw")throw std::runtime_error("owned transition");
        };
        auto capture=std::shared_ptr<int>(new int(0),[&](int *value){captureDestroyed=true;check(!sample&&gate->busy()&&other->busy(),"capture destruction retains shared gate after Sample");pump();check(gate->busy()&&other->busy(),"nested capture destruction retains gate");delete value;});
        const auto end=std::chrono::steady_clock::now()+std::chrono::milliseconds(mode=="deadline"?80:mode.endsWith("wait-timeout")?180:1500);
        check(executor->start(&window,request,end,[&]{return permitted;},[&,capture](const resident::Frame &reply){
            ++delivered;result=reply;check(!sample&&gate->busy()&&other->busy(),"publication outside Sample under shared gate");
            check(!other->start(&window,request,end,[]{return true;},[](const resident::Frame&){}),"replacement cannot enter completion callback");
        }),"owned request consumed");capture.reset();
        QElapsedTimer wait;wait.start();while(!delivered&&wait.elapsed()<2200)pump();pump();
        check(delivered==1&&captureDestroyed&&!A::active(*executor),"one completion and bounded request retirement");
        const bool positive=mode=="zero"||mode=="next"||mode=="wait";
        const bool noEffect=mode=="pre-cancel"||mode=="stale-source"||mode=="pre-wait-timeout"||mode.startsWith("nested-getter")||mode=="refused-zero"||mode=="deadline-zero";
        if(mode.startsWith("nested-"))check(reentered,"regression actually entered nested callback");
        check(result.kind==(positive?5:noEffect?3:4),"distinct observed/refused/unknown result");
        check(calls==(mode=="zero"||noEffect?0u:1u),"zero or exactly one invocation");
        check(gate->busy()==(!positive&&!noEffect),"only known outcome releases capacity");
        if(positive){check(result.facts.document==request.facts.document&&result.facts.page==request.target&&result.facts.order==request.facts.order,"positive uses exact fresh post-order target");if(mode!="zero")check(joins>=2,"adjacent invocation requires fresh join");}
        A::destroy(executor);check(other->busy()==(!positive&&!noEffect),"executor destruction cannot clear uncertain transition");
        A::destroy(other);A::destroyGate(gate);
    }
    QTextStream(stdout)<<"PASS resident selection owned checks="<<checks<<"; native qualification disabled\n";
    return 0;
}
#include "resident_selection_test.moc"
