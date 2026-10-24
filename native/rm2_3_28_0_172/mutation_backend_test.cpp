// Real authenticated local endpoint + dispatcher, owned executor boundary.
#include "mutation_backend.h"
#include <QTemporaryDir>
#include <QElapsedTimer>
#include <cstdio>
using namespace remarkable;
using namespace remarkable::rm2_3_28_0_172;
namespace {
unsigned checks=0;
void check(bool v,const char*why){++checks;if(!v)qFatal("backend %u: %s",checks,why);}
void pump(int ms=5){QElapsedTimer t;t.start();do{QCoreApplication::processEvents(QEventLoop::AllEvents,1);QThread::msleep(1);}while(t.elapsed()<ms);}
template<class F>void until(F predicate){QElapsedTimer t;t.start();while(!predicate()&&t.elapsed()<2000)pump();check(predicate(),"bounded delivery");}
resident::EndpointConfig endpointConfig(const QString &directory){
    resident::EndpointConfig c;c.directory=directory;c.peerUid=::geteuid();c.peerPid=::getpid();c.peerStart=resident::processStart(c.peerPid);c.operationBudgetMs=300;
    resident::detail::Writer w;w.bytes=QByteArray(32,1)+QByteArray(32,2)+QByteArray(32,3)+resident::bootIdentity();
    w.number<quint32>(quint32(c.peerPid));w.number<quint64>(c.peerStart);w.bytes+=QByteArray(16,4);c.runtime=w.bytes;return c;
}
resident::Frame receive(QLocalSocket &socket){resident::Decoder decoder;std::optional<resident::Frame> frame;
    until([&]{if(socket.bytesAvailable()){auto r=decoder.feed(socket.read(resident::MaxFrame+4));check(!r.error,"valid response");if(r.frame)frame=r.frame;}return bool(frame);});return *frame;
}
resident::Frame request(){
    resident::Frame r;r.message=2;r.kind=1;r.operation=QByteArray(16,7);r.facts.document=QByteArray(16,8);
    r.facts.order={QByteArray(16,9),QByteArray(16,10),QByteArray(16,11)};r.facts.page=r.facts.order[1];r.target=QByteArray(16,12);return r;
}
reader_insertion_entry::Result good(const reader_insertion_entry::Config &config){
    reader_insertion_entry::Result r;r.finished=r.observed=r.postOrder.observed=true;
    r.postOrder.ids=config.request.expectedPages;r.postOrder.ids.insert(config.request.sourceIndex+1,config.request.targetPageId);return r;
}
}
int main(int argc,char **argv){
    QGuiApplication app(argc,argv);QQmlEngine engine;QQuickWindow window;
    QQmlEngine::setContextForObject(&window,engine.rootContext());window.show();window.requestActivate();pump();
    const auto original=request();const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(1);
    const auto config=mutationConfig(original,deadline);check(bool(config),"valid historical config");
    check(config->absoluteDeadline==deadline&&config->wantConditionalForward&&!config->wantTargetAcquisition&&config->request.sourceIndex==1,"original deadline and exact source");
    check(mutationReply(original,good(*config)).kind==1,"exact observed after-order");
    for(unsigned mode=0;mode<9;++mode){auto r=good(*config);
        if(mode==0)r.unknown=true;
        if(mode==1)r.reconciliationRequired=true;
        if(mode==2)r.canceled=true;
        if(mode==3)r.postOrder.observed=false;
        if(mode==4)r.postOrder.ids.swapItemsAt(0,1);
        if(mode==5)r.postOrder.ids.removeLast();
        if(mode==6)r.finished=false;
        if(mode==7)r.refusal=QStringLiteral("late-proof-refused");
        if(mode==8)r.postOrder.ids[1]=r.postOrder.ids[0];
        check(mutationReply(original,r).kind==4,"bad evidence never success or safe retry");
    }
    reader_insertion_entry::Result refusal;refusal.finished=true;
    check(mutationReply(original,refusal).kind==3&&mutationReply(original,refusal).rejection==2,"known executor refusal");
    refusal.creation.attempted=true;check(mutationReply(original,refusal).kind==4,"attempt cannot become safe retry");
    refusal.creation.attempted=false;refusal.core.forwardCommitted=true;check(mutationReply(original,refusal).kind==4,"committed forward cannot become safe retry");
    for(unsigned mode=0;mode<5;++mode){auto r=original;
        if(mode==0)r.operation={};
        if(mode==1)r.target=r.facts.page;
        if(mode==2)r.facts.page=QByteArray(16,99);
        if(mode==3)r.kind=2;
        if(mode==4){r.facts.order.clear();for(int i=1;i<=17;++i)r.facts.order.push_back(QByteArray(16,char(i)));r.facts.page=r.facts.order[0];r.target=QByteArray(16,80);}
        check(!mutationConfig(r,deadline),"invalid or unsupported request refuses conversion");
    }
    for(const QByteArray mode:{"normal","unknown","before-dispatch","executor-refusal","capacity","deadline","disconnect","destroy"}){
        QTemporaryDir directory;check(directory.isValid(),"owned directory");auto c=endpointConfig(directory.path());
        unsigned starts=0;reader_insertion_entry::Config frozen;QByteArray nonce;reader_insertion_entry::Permit permit;
        reader_worker_native::ResidentMutation::Completion complete;
        auto *backend=new MutationBackend([&](QQuickWindow*w,reader_insertion_entry::Config cfg,QByteArray n,reader_insertion_entry::Permit p,auto done){
            ++starts;check(w==&window&&p(),"fresh window and live lease");frozen=std::move(cfg);nonce=std::move(n);permit=std::move(p);complete=std::move(done);return mode!="before-dispatch";
        },[&]{return mode=="capacity";});
        QPointer<MutationBackend> guarded(backend);std::chrono::steady_clock::time_point leaseDeadline;
        resident::Endpoint server(c,[&](auto lease){leaseDeadline=lease->deadline();if(guarded)guarded->submit(std::move(lease));});
        check(backend->attach(&server)&&server.start(),"endpoint attached and admitted");QLocalSocket peer;
        peer.connectToServer(c.directory+"/sdk.sock");until([&]{return peer.state()==QLocalSocket::ConnectedState;});
        resident::Frame hello;hello.runtime=c.runtime;hello.client=QByteArray(16,5);hello.server=QByteArray(16,0);hello.operation=QByteArray(16,6);
        check(peer.write(resident::encode(hello))>0,"hello sent");const auto ready=receive(peer);
        auto call=original;call.runtime=ready.runtime;call.client=ready.client;call.server=ready.server;call.sequence=1;
        check(peer.write(resident::encode(call))>0,"mutation sent");
        if(mode=="capacity"){
            const auto reply=receive(peer);check(reply.kind==3&&reply.rejection==4&&!starts,"busy refuses before executor");
        }else {
            until([&]{return starts==1;});check(nonce==call.operation.toHex()&&frozen.absoluteDeadline==leaseDeadline,"exact nonce and immutable deadline");
            check(frozen.request.documentId==config->request.documentId&&frozen.request.expectedPages==config->request.expectedPages&&frozen.request.targetPageId==config->request.targetPageId,"original historical facts retained");
            if(mode=="deadline"||mode=="disconnect"||mode=="destroy"){
                if(mode=="deadline")pump(350);
                if(mode=="disconnect"){peer.abort();pump();}
                if(mode=="destroy"){delete backend;backend=nullptr;}
                check(!permit(),"lost scope revokes action authority");complete(good(frozen));pump();check(!peer.bytesAvailable(),"late completion not published");
            }else {
                if(mode!="before-dispatch"){
                    auto r=good(frozen);if(mode=="unknown")r.unknown=true;
                    if(mode=="executor-refusal"){r={};r.finished=true;}
                    complete(r);
                }
                const auto reply=receive(peer);check(reply.requestDigest==resident::fingerprint(call)&&reply.operation==call.operation&&reply.sequence==call.sequence,"response exact request correlation");
                if(mode=="normal")check(reply.kind==1&&reply.target==call.target&&reply.facts.order.size()==4,"observed reply");
                if(mode=="unknown")check(reply.kind==4,"uncertain reply");
                if(mode=="before-dispatch")check(reply.kind==3&&reply.rejection==1,"not consumed reply");
                if(mode=="executor-refusal")check(reply.kind==3&&reply.rejection==2,"executor pre-mutation reply");
                check(!permit(),"publication consumes lease");
            }
        }
        delete backend;complete={};permit={};
    }
    std::printf("resident mutation backend: %u checks PASS\n",checks);
}
