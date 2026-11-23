#define main selected_view_baseline_main
#include "reader_selected_view_fixture.cpp"
#undef main
#include "composer_key_delivery.h"
namespace remarkable::composer_scene::testing {
struct KeyAccess {
    static auto make(QQuickItem *receiver, KeyDelivery::Acquire acquire) {
        return KeyDelivery::make(receiver,std::chrono::seconds(10),std::move(acquire));
    }
};
}
class DocumentWorker final : public QThread { Q_OBJECT };
int main(int argc,char **argv) {
    QGuiApplication app(argc,argv);
    qmlRegisterType<QObject>("OwnedProbe",1,0,"QtObject");
    qmlRegisterType<QQuickItem>("OwnedProbe",1,0,"Item");
    qmlRegisterType<FocusFixture>("OwnedProbe",1,0,"FocusScope");
    qmlRegisterType<SceneView>("OwnedProbe",1,0,"SceneFixture");
    registerCreationFixture();
    Fixture f;
    Library library; DocumentController creation; DocumentWorker worker;
    creation.setLibrary(&library); library.result=&f.document;
    f.controller.workerValue=&worker;
    f.current->forceActiveFocus();
    int calls=0;
    const auto action=[&](QObject *c,const QVariant &,QObject *w,const reader_selected::Observation &r,
            const std::function<bool()> &guard) {
        require(c==&creation && w==&worker && r.ready && guard(),"actual existing worker join inside input guard");
        ++calls;
    };
    const auto observe=[&](QQuickItem *receiver) {
        return reader_selected::observeInputReceiver(&f.window,&creation,receiver,[]{return true;},1000,action);
    };
    require(observe(f.current).observed && calls==1,"exact selected focused receiver dispatches once");
    QQuickItem child(f.current),other(f.device);
    refused(observe(&child),"child is not the actual SceneView receiver");
    refused(observe(&other),"unselected receiver refused");
    refused(observe(nullptr),"null receiver refused");
    child.forceActiveFocus();
    refused(observe(f.current),"focused child does not impersonate exact receiver");
    f.current->forceActiveFocus();
    library.onReadyRead=[&]{other.forceActiveFocus(); f.current->forceActiveFocus();};
    refused(observe(f.current),"focus ABA while joining refuses");
    require(calls==1,"all invalid contexts suppress worker action");
    require(observe(f.current).observed && calls==2,"fresh exact receiver recovers");
    {
        // Same signal/property surface as the selected SceneView, but not the
        // actual selected receiver. A failed join must publish no private key.
        SceneView unselected(f.device);
        int refusedJoins=0;
        auto ingress=remarkable::composer_scene::testing::KeyAccess::make(&unselected,
            [&](QQuickItem *target,remarkable::composer_scene::Clock::time_point,
                    const std::shared_ptr<std::atomic<bool>> &,const reader_worker_native::Permit &permit) {
                const auto result=reader_selected::observeInputReceiver(&f.window,&creation,target,permit,1000,action);
                refused(result,"unselected event receiver fails the real join");
                ++refusedJoins;
                return std::unique_ptr<remarkable::composer_scene::Delivery>{};
            });
        require(bool(ingress),"matching surface can be armed without disclosure authority");
        QKeyEvent event(QEvent::KeyPress,Qt::Key_A,Qt::NoModifier,QStringLiteral("unrelated private text"));
        QCoreApplication::sendEvent(&unselected,&event);
        const auto receipt=ingress->snapshot();
        require(refusedJoins==1 && calls==2 && !receipt.key && !receipt.scene.submission.attempted &&
            receipt.refusal==QStringLiteral("key-source-refused"),"refused source exposes no key facts or text");
    }
    QTextStream(stdout)<<"PASS selected input receiver: exact native-view join, focus, mismatch and ABA refusals\n";
}
#include "reader_input_receiver_test.moc"
