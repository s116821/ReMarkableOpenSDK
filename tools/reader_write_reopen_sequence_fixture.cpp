// Actual queued Qt/Files/PNG/owned selected joins; native route/closure/actor modeled.
#define READER_WRITE_REOPEN_TESTING
#define READER_IDENTITY_CAPTURE_TESTING
#define VISUAL_CAPTURE_TESTING
#define READER_FIXED_PEN_HANDOFF_TESTING
#include "reader_write_reopen_sequence.h"
#include "reader_capture_files.h"
#define main selected_fixture_uninvoked_main
#include "reader_selected_view_fixture.cpp"
#undef main
#include <QFile>
#include <QJsonDocument>
#include <QTemporaryDir>
#include <QEventLoop>
#include <cstdio>
#include <stdexcept>

namespace {
namespace seq=reader_write_reopen;
unsigned cases=0,checks=0;
void need(bool yes,const char *why){++checks;if(!yes)throw std::runtime_error(why);}
QString uuid(int i){return QStringLiteral("00000000-0000-4000-8000-%1").arg(i,12,16,QLatin1Char('0'));}
QByteArray bytes(const QString &path){QFile f(path);need(f.open(QIODevice::ReadOnly),"frozen/owned input opens");return f.readAll();}
struct Owned {
    Fixture f{false,true,"property int ownedCloseCalls: 0\nfunction close() { ownedCloseCalls += 1; document = null; }\n"};
    Library library;DocumentController creation;reader_identity_capture::Expected expected;
    Owned(){
        f.document.pages={uuid(1),uuid(2),uuid(3)};f.device->setProperty("page",1);f.controller.setPageId(uuid(2));
        emit f.document.pageMapChanged();emit f.document.pageCountChanged();
        creation.setLibrary(&library);library.result=&f.document;f.engine.rootContext()->setContextProperty("externalCreation",&creation);
        f.window.resize(160,120);QCoreApplication::processEvents();expected={f.document.key,uuid(2),1,3};
        const auto joined=reader_selected::observeCreationJoin(&f.window,[]{return true;},20000);
        need(joined.observed&&joined.ready&&joined.creationJoinObserved,"actual owned selected-creation join baseline");
    }
};
struct Assets {
    QByteArray positive,canceled,positiveRaw,canceledRaw;
    reader_fixed_pen_result::Expected expected,canceledExpected;
    reader_fixed_pen_result::Completion completion,canceledCompletion;
    explicit Assets(const QString &folder){
        const auto load=[&](const char *name,reader_fixed_pen_result::Expected &e,reader_fixed_pen_result::Completion &c,QByteArray &wire,QByteArray &raw){
            const auto path=folder+'/'+QString::fromLatin1(name);
            wire=bytes(path+".json");raw=bytes(path+".raw");const auto facts=QJsonDocument::fromJson(bytes(path+".expected.json")).object();
            need(facts.value("external_completion_modeled").toBool()&&facts.value("candidate_identity_model")=="live-owned-parent","frozen expectation explicitly modeled");
            e={facts.value("nonce").toString(),facts.value("boot_id").toString(),facts.value("candidate_pid").toString(),facts.value("candidate_start").toString(),facts.value("holder_pid").toString(),facts.value("holder_start").toString(),facts.value("setup_end_ms").toString().toULongLong(),facts.value("request_ms").toString().toULongLong()};
            c={true,true,true,true,true,0,e.holderPid,e.holderStart,facts.value("completion_observed_ms").toString().toULongLong(),facts.value("completion_now_ms").toString().toULongLong(),wire};
        };
        load("positive",expected,completion,positive,positiveRaw);load("canceled",canceledExpected,canceledCompletion,canceled,canceledRaw);
        reader_fixed_pen_result::Claim claim;need(reader_fixed_pen_result::consume(positive,positiveRaw,expected,completion,claim).accepted,"untouched actual FIFO producer passes current consumer");
    }
};
enum class Fault {None,NavRefuse,ReopenRefuse,NoRetire,NoPair,WrongClaim,UnknownCapture,WrongSource,
    PublishRefuse,PenRefuse,PenPending,WrongHolder,WrongRequest,WrongBoot,FutureCompletion,ClosurePending,
    CancelQueued,CancelCapture,DeleteCapture,DeleteReader,FocusChange,ClockExpire,ClockRegress,
    DeletePen,ThrowCapture,ThrowPen,DuplicateHandoff,DuplicateClose,FinalContext,FinalAdmission,FinalCancel,FinalQuit,TimerExpire,DeleteReady,DeleteInitialClock,DeleteAdmission,DeleteReport,DeleteCancel};
class ModelIO final:public seq::IO,public std::enable_shared_from_this<ModelIO> {
public:
    Assets assets;std::unique_ptr<Owned> owned=std::make_unique<Owned>();QTemporaryDir filesRoot;
    QPointer<seq::Sequence> sequence;Fault fault;quint64 clock;bool admittedValue=true,reported=false,ink=false,thirdReturned=false;
    unsigned navigation=0,captures=0,handoffs=0,goWrites=0,pens=0,closes=0,closures=0,cancels=0,reports=0,grabs=0;
    unsigned clocksAfterThird=0,admissionsAfterThird=0,allClocks=0;quint64 setupOverride=0;QElapsedTimer wall;seq::Result result;QStringList events;std::array<QString,3> folders;
    std::array<QByteArray,3> pngs,scalars;std::unique_ptr<QQuickWindow> other;
    ModelIO(const QString &assetsFolder,Fault f):assets(assetsFolder),fault(f),clock(assets.expected.requestMs){need(filesRoot.isValid(),"owned phase directory");wall.start();}
    quint64 nowMs()noexcept override{
        if(++allClocks==1&&fault==Fault::DeleteInitialClock&&sequence)delete sequence.data();
        if(thirdReturned&&++clocksAfterThird==3&&fault==Fault::FinalContext)owned.reset();
        return clock+(fault==Fault::TimerExpire?quint64(wall.elapsed()):0);
    }
    bool admitted()noexcept override{
        if(fault==Fault::DeleteAdmission&&sequence)delete sequence.data();
        if(thirdReturned&&++admissionsAfterThird==2){
            if(fault==Fault::FinalAdmission)admittedValue=false;
            if(fault==Fault::FinalCancel&&sequence)sequence->cancel();
            if(fault==Fault::FinalQuit)QMetaObject::invokeMethod(QCoreApplication::instance(),"aboutToQuit",Qt::DirectConnection);
        }
        return admittedValue;
    }
    seq::HolderReady holderReady()noexcept override{auto e=assets.expected;e.requestMs=0;if(setupOverride)e.setupEnd=setupOverride;if(fault==Fault::DeleteReady&&sequence)delete sequence.data();return {true,e};}
    seq::Capture capture(int index,QQuickWindow *window,const reader_identity_capture::Expected &expected,
                         const std::shared_ptr<reader_identity_capture::Claim> &claim,const seq::Guard &permit,int budget)override{
        ++captures;events.append(QStringLiteral("capture%1").arg(index));
        if(fault==Fault::ThrowCapture)throw std::runtime_error("owned capture");
        if(index==0&&fault==Fault::CancelCapture&&sequence)sequence->cancel();
        if(index==0&&fault==Fault::DeleteCapture&&sequence)delete sequence.data();
        if(index==0&&fault==Fault::DeleteReader&&owned){delete owned->f.reader;owned->f.reader=nullptr;}
        if(index==0&&fault==Fault::FocusChange&&owned){other=std::make_unique<QQuickWindow>();QQmlEngine::setContextForObject(other.get(),owned->f.engine.rootContext());other->show();other->requestActivate();QCoreApplication::processEvents();}
        if(index==0&&fault==Fault::ClockExpire)clock=assets.expected.setupEnd;
        if(index==0&&fault==Fault::ClockRegress)--clock;
        if(index==0&&fault==Fault::WrongSource&&owned)owned->f.controller.setPageId(uuid(1));
        seq::Capture out;folders[std::size_t(index)]=filesRoot.path()+QStringLiteral("/phase%1").arg(index);const auto path=QFile::encodeName(folders[std::size_t(index)]);
        need(::mkdir(path.constData(),0700)==0,"fresh one-use phase folder");
        const int root=::open(path.constData(),O_RDONLY|O_DIRECTORY|O_NOFOLLOW|O_CLOEXEC);need(root>=0,"owned phase root opens");
        const auto files=reader_capture_files::Files::create(root,permit);
        if(files){
            const auto opened=files->openImage(permit);
            if(opened.ok){
                {visual_capture::Sink sink(files->imageFd());reader_identity_capture::TestBoundary hooks;
                    hooks.grab=[&](QQuickWindow *){++grabs;QImage image(32,24,QImage::Format_ARGB32);image.fill(Qt::white);if(ink)for(int x=4;x<=20;++x)image.setPixelColor(x,12,Qt::black);return image;};
                    const auto selectedClaim=(index==0&&fault==Fault::WrongClaim)?std::make_shared<reader_identity_capture::Claim>():claim;
                    out.facts=reader_identity_capture::run(window,expected,sink,selectedClaim,permit,budget,hooks);
                    if(out.facts.captured)need(out.facts.encoded==sink.count()&&out.facts.sha==sink.digest(),"actual Sink encoder bytes/hash");
                }
                if(out.facts.captured){
                    const auto sealed=files->sealImage(out.facts.encoded,permit);const auto committed=sealed.ok?files->commitImage(permit):reader_capture_files::Result{};
                    if(committed.committed){
                        const auto scalar=QJsonDocument(QJsonObject{{"owned_phase",index},{"image_sha256",QString::fromLatin1(out.facts.sha)}}).toJson(QJsonDocument::Compact)+'\n';
                        const auto published=files->publishScalar(scalar,reader_capture_files::ScalarKind::AcceptedPair,permit);out.pairCommitted=published.committed;
                        if(out.pairCommitted){
                            pngs[std::size_t(index)]=bytes(folders[std::size_t(index)]+"/visual-image.png");scalars[std::size_t(index)]=bytes(folders[std::size_t(index)]+"/selected-result");
                            out.scalarSha256=QCryptographicHash::hash(scalars[std::size_t(index)],QCryptographicHash::Sha256).toHex();
                            QImage decoded;need(decoded.loadFromData(pngs[std::size_t(index)],"PNG")&&decoded.size()==QSize(32,24),"actual complete phase PNG decode");
                        }
                    }
                }
            }
            const auto retired=files->retire();out.retired=retired.ok&&retired.attempted&&!retired.committed;need(files->imageFd()==-1,"phase writer retired before return");
        }
        need(::close(root)==0,"phase caller root checked close before advance");
        if(index==0&&fault==Fault::NoRetire)out.retired=false;
        if(index==0&&fault==Fault::NoPair)out.pairCommitted=false;
        if(index==0&&fault==Fault::UnknownCapture){out.facts.unknown=true;out.facts.captured=false;out.facts.refusal="owned-unknown";}
        if(index==0&&fault==Fault::CancelQueued){const auto pin=sequence;QTimer::singleShot(0,QCoreApplication::instance(),[pin]{if(pin)pin->cancel();});}
        if(index==2)thirdReturned=true;
        return out;
    }
    bool publishGo(const QByteArray &request)override{
        ++goWrites;events.append("publish-go");const auto path=QFile::encodeName(filesRoot.path()+"/pen-go.request");
        const int fd=::open(path.constData(),O_WRONLY|O_CREAT|O_EXCL|O_NOFOLLOW|O_CLOEXEC,0600);need(fd>=0,"one actual owned GO file");
        const auto n=::write(fd,request.constData(),std::size_t(request.size()));need(n==request.size()&&::close(fd)==0,"owned GO bytes written once");
        need(bytes(QString::fromLocal8Bit(path))==request,"owned original GO byte readback");return fault!=Fault::PublishRefuse&&fault!=Fault::DeleteCancel;
    }
    seq::Pen pen(const reader_fixed_pen_handoff::Result &handoff)override{
        ++pens;events.append("pen");
        if(fault==Fault::ThrowPen)throw std::runtime_error("owned pen");
        if(fault==Fault::DeletePen&&sequence){delete sequence.data();need(handoff.published,"handoff argument retained after facade deletion");return {};}
        if(fault==Fault::PenPending||pens==1)return {};
        const bool refused=fault==Fault::PenRefuse;reader_fixed_pen_result::Claim claim;
        auto evidence=reader_fixed_pen_result::consume(refused?assets.canceled:assets.positive,refused?assets.canceledRaw:assets.positiveRaw,
            refused?assets.canceledExpected:assets.expected,refused?assets.canceledCompletion:assets.completion,claim);
        clock=assets.completion.nowMs+1;
        if(fault==Fault::WrongHolder)evidence.binding.holderPid="2147483647";
        if(fault==Fault::WrongRequest)--evidence.binding.requestMs;
        if(fault==Fault::WrongBoot)evidence.binding.boot="aaaaaaaa-bbbb-cccc-dddd-eeeeeeeeeeee";
        if(fault==Fault::FutureCompletion)evidence.completedAtMs=clock+1;
        if(evidence.accepted)ink=true;
        return {evidence.accepted?seq::PenState::Accepted:seq::PenState::Refused,evidence};
    }
    void cancelPen()noexcept override{++cancels;events.append("cancel-pen");if(fault==Fault::DeleteCancel&&sequence)delete sequence.data();}
    void report(const seq::Result &r)noexcept override{++reports;result=r;reported=true;events.append("report");if(fault==Fault::DeleteReport&&sequence)delete sequence.data();}
    seq::TestBoundary boundary(){
        const auto io=shared_from_this();seq::TestBoundary t;t.navigationWait=fault==Fault::TimerExpire?750:1;t.penWait=0;t.closureWait=0;
        t.navigate=[io](QQuickWindow *window,const reader_selected::FixtureNavigation &c,const seq::Guard &permit,int){
            const auto keep=io;++keep->navigation;keep->events.append("navigate");reader_selected::NavigationObservation r;
            if(!permit())return r;
            need(keep->owned&&window==&keep->owned->f.window&&c.documentId==keep->owned->expected.documentId&&c.pageIndex==keep->owned->expected.pageIndex&&c.legacyComposition,"one fixed modeled ordinary route");
            r.attempted=true;r.dispatched=keep->fault!=Fault::NavRefuse&&!(keep->fault==Fault::ReopenRefuse&&keep->navigation==2);
            if(!r.dispatched)r.refusal="owned-route-refusal";
            else if(keep->navigation==2)need(keep->owned->f.reader->setProperty("document",QVariant::fromValue<QObject *>(&keep->owned->f.document)),"modeled ordinary reopen restores owned document binding");
            return r;
        };
        t.handoff=[io](QQuickWindow *w,const reader_fixed_pen_handoff::Config &c,const std::shared_ptr<reader_fixed_pen_handoff::Claim> &claim,const seq::Guard &permit,const std::function<bool(const QByteArray &)> &publish,int budget){
            const auto keep=io;++keep->handoffs;keep->clock=keep->assets.expected.requestMs;
            reader_fixed_pen_handoff::TestBoundary hooks;hooks.clock=[keep]{return keep->clock;};
            const auto r=reader_fixed_pen_handoff::run(w,c,claim,permit,publish,budget,hooks);
            if(keep->fault==Fault::DuplicateHandoff){const auto duplicate=reader_fixed_pen_handoff::run(w,c,claim,permit,publish,budget,hooks);need(!duplicate.attempted&&duplicate.refusal=="claim-spent","same real handoff claim cannot duplicate publication");}
            return r;
        };
        t.close=[io](QQuickWindow *w,const reader_ordinary_close::Expected &e,const std::shared_ptr<reader_ordinary_close::Claim> &claim,const seq::Guard &permit,int budget){
            const auto keep=io;++keep->closes;keep->events.append("close");const auto r=reader_ordinary_close::run(w,e,claim,permit,budget);
            if(keep->fault==Fault::DuplicateClose){const auto duplicate=reader_ordinary_close::run(w,e,claim,permit,budget);need(!duplicate.attempted&&duplicate.refusal=="claim-spent","same real close claim cannot invoke twice");}return r;
        };
        t.closed=[io](QQuickWindow *,const seq::Guard &permit,int){
            const auto keep=io;++keep->closures;keep->events.append("closed");reader_closed_view::Result r;
            if(!permit()||keep->fault==Fault::ClosurePending||keep->closures==1)return r;
            // Native Loader/composition is explicitly modeled; owned QML close
            // itself genuinely nulled its document property on the prior turn.
            need(keep->owned&&keep->owned->f.reader->property("document").value<QObject *>()==nullptr,"real owned close nulled document before modeled closure");
            r.observed=r.compositionObserved=r.loaderReady=r.documentNull=true;return r;
        };
        return t;
    }
};
struct Run {
    std::shared_ptr<ModelIO> io;QPointer<seq::Sequence> sequence;
    Run(QGuiApplication &app,const QString &assets,Fault fault){io=std::make_shared<ModelIO>(assets,fault);const auto &b=io->assets.expected;
        if(fault==Fault::TimerExpire)io->setupOverride=io->clock+500;
        seq::Config c{io->owned->expected,b.nonce.toLatin1(),b.candidatePid.toLatin1(),b.candidateStart.toLatin1(),io->setupOverride?io->setupOverride:b.setupEnd};
        sequence=new seq::Sequence(&app,c,io,nullptr,io->boundary());io->sequence=sequence;
    }
    ~Run(){if(sequence)delete sequence.data();}
    void pump(){QElapsedTimer elapsed;elapsed.start();while(sequence&&!io->reported&&elapsed.elapsed()<8000){QCoreApplication::sendPostedEvents();QCoreApplication::processEvents(QEventLoop::AllEvents,5);QThread::msleep(1);}need(io->reported||!sequence,"queued sequence reaches a terminal/deleted state");QCoreApplication::processEvents();}
};
void scenario(QGuiApplication &app,const QString &assets,Fault fault){
    const auto only=qEnvironmentVariable("READER_WRITE_REOPEN_ONLY");if(!only.isEmpty()&&only.toInt()!=int(fault))return;
    ++cases;Run r(app,assets,fault);const bool started=r.sequence->start();
    if(fault==Fault::DeleteReady||fault==Fault::DeleteInitialClock||fault==Fault::DeleteAdmission){need(!started&&!r.sequence&&!r.io->reported&&r.io->navigation==0,"startup callout deletion prevents all queued effects/report");return;}
    need(started,"owned sequence starts");need(!r.sequence->start(),"whole sequence start one-use");r.pump();
    const auto &io=*r.io;
    if(fault==Fault::DeleteCapture||fault==Fault::DeletePen||fault==Fault::DeleteCancel){need(!r.sequence&&!io.reports,"deleted sequence cannot publish a result");need(io.goWrites<=1&&io.navigation==1,"deletion cannot reopen or resend");return;}
    need(io.reports==1&&io.result.finished,"exactly one terminal report");
    const bool success=fault==Fault::None||fault==Fault::DuplicateHandoff||fault==Fault::DuplicateClose||fault==Fault::DeleteReport;
    if(io.result.observed!=success)std::fprintf(stderr,"fault=%d refusal=%s nav=%u captures=%u go=%u close=%u\n",int(fault),qPrintable(io.result.refusal),io.navigation,io.captures,io.goWrites,io.closes);
    need(io.result.observed==success,"only complete bound sequence observed");
    need(io.navigation<=2&&io.handoffs<=1&&io.goWrites<=1&&io.closes<=1&&io.captures<=3,"all effect paths bounded once");
    if(success){
        need(io.navigation==2&&io.captures==3&&io.handoffs==1&&io.goWrites==1&&io.closes==1&&io.pens==2&&io.closures==2,"complete queued route/capture/pen/close/reopen path");
        need(io.result.pen.accepted&&io.result.pen.replay.strokeObserved&&io.result.closed.observed,"untouched actual producer trace plus modeled independent completion");
        for(const auto &capture:io.result.captures)need(capture.facts.captured&&capture.retired&&capture.pairCommitted,"each real capture pair retired before advance");
        need(io.pngs[0]!=io.pngs[1]&&io.pngs[1]==io.pngs[2],"known modeled after/reopen pixels preserved in actual PNGs");
        need(io.owned->f.reader->property("ownedCloseCalls").toInt()==1,"actual generated QML close slot invoked once");
    }else need(!io.result.refusal.isEmpty(),"failure explicit");
    if(fault==Fault::NoRetire||fault==Fault::NoPair||fault==Fault::WrongClaim||fault==Fault::UnknownCapture||fault==Fault::WrongSource||fault==Fault::CancelQueued||fault==Fault::CancelCapture||fault==Fault::DeleteReader||fault==Fault::FocusChange||fault==Fault::ClockExpire||fault==Fault::ClockRegress||fault==Fault::ThrowCapture||fault==Fault::TimerExpire)
        need(io.handoffs==0&&io.goWrites==0&&io.closes==0,"pre-write/capture refusal cannot handoff or close");
    if(fault==Fault::PublishRefuse||fault==Fault::PenRefuse||fault==Fault::PenPending||fault==Fault::WrongHolder||fault==Fault::WrongRequest||fault==Fault::WrongBoot||fault==Fault::FutureCompletion||fault==Fault::ThrowPen)
        need(io.closes==0&&io.captures==1&&io.cancels==1,"unaccepted pen never advances, requests independent cancel once");
    if(fault==Fault::ClosurePending)need(io.navigation==1&&io.closes==1,"unknown closure cannot reopen");
}
}
int main(int argc,char **argv){QGuiApplication app(argc,argv);try{
    need(argc==2,"one original frozen producer asset directory");
    qmlRegisterType<QObject>("QtQml",2,0,"QtObject");qmlRegisterModule("QtQml.WorkerScript",2,0);qmlRegisterModule("QtQml.Models",2,0);
    qmlRegisterModule("QtQml.WorkerScript",QT_VERSION_MAJOR,QT_VERSION_MINOR);qmlRegisterModule("QtQml.Models",QT_VERSION_MAJOR,QT_VERSION_MINOR);
    qmlRegisterType<QObject>("OwnedProbe",1,0,"QtObject");qmlRegisterType<QQuickItem>("OwnedProbe",1,0,"Item");qmlRegisterType<FocusFixture>("OwnedProbe",1,0,"FocusScope");qmlRegisterType<SceneView>("OwnedProbe",1,0,"SceneFixture");registerCreationFixture();
    const auto assets=QString::fromLocal8Bit(argv[1]);
    for(Fault fault:{Fault::None,Fault::NavRefuse,Fault::ReopenRefuse,Fault::NoRetire,Fault::NoPair,Fault::WrongClaim,Fault::UnknownCapture,Fault::WrongSource,Fault::PublishRefuse,Fault::PenRefuse,Fault::PenPending,Fault::WrongHolder,Fault::WrongRequest,Fault::WrongBoot,Fault::FutureCompletion,Fault::ClosurePending,Fault::CancelQueued,Fault::CancelCapture,Fault::DeleteCapture,Fault::DeleteReader,Fault::FocusChange,Fault::ClockExpire,Fault::ClockRegress,Fault::DeletePen,Fault::ThrowCapture,Fault::ThrowPen,Fault::DuplicateHandoff,Fault::DuplicateClose,Fault::FinalContext,Fault::FinalAdmission,Fault::FinalCancel,Fault::FinalQuit,Fault::TimerExpire,Fault::DeleteReady,Fault::DeleteInitialClock,Fault::DeleteAdmission,Fault::DeleteReport,Fault::DeleteCancel})scenario(app,assets,fault);
    std::printf("PASS write reopen sequence cases=%u checks=%u actual_queued_qt_files_png_join=true native_route_closure_actor_modeled=true native_execution=false\n",cases,checks);return 0;
}catch(const std::exception &e){std::fprintf(stderr,"FAIL write reopen sequence %s cases=%u checks=%u\n",e.what(),cases,checks);return 1;}}
