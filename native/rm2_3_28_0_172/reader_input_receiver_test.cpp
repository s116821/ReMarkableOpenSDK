#define main selected_view_baseline_main
#include "reader_selected_view_fixture.cpp"
#undef main
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
    QTextStream(stdout)<<"PASS selected input receiver: exact native-view join, focus, mismatch and ABA refusals\n";
}
#include "reader_input_receiver_test.moc"
