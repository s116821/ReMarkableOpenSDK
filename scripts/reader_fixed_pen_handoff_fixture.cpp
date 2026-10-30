// Real owned Qt/callout lifetimes and files; source identity/clock explicitly modeled.
#define READER_FIXED_PEN_HANDOFF_TESTING
#include "reader_fixed_pen_handoff.h"
#include <QTemporaryDir>
#include <QFile>
#include <cstdio>
#include <stdexcept>
#include <fcntl.h>
#include <unistd.h>
namespace {
using namespace reader_fixed_pen_handoff;
unsigned cases=0,checks=0;
void require(bool b,const char *s){++checks;if(!b)throw std::runtime_error(s);}
template<class F>void scenario(F f){++cases;f();}
Config config(){return {{"11111111-2222-3333-4444-555555555555","22222222-3333-4444-5555-666666666666",6,12},QByteArray(32,'a'),"123","456",20000};}
reader_selected::Observation facts(){const auto e=config().target;reader_selected::Observation o;o.documentId=e.documentId;o.pageId=e.pageId;o.pageIndex=e.pageIndex;o.pageCount=e.pageCount;o.status=o.pageStatus=1;o.observed=o.mapCoherent=o.creationJoinObserved=o.ready=true;o.loading=o.pageError=o.documentError=o.awaitingTiles=o.pendingTiles=o.working=false;return o;}
struct Seam {
    QQuickWindow window;QPointer<QObject> reader=new QObject(&window);
    reader_selected::Observation observed=facts();Guard guard=[]{return true;};
    quint64 now=10000;unsigned repeats=1,writes=0,samples=0;QByteArray request;
    std::function<void()> onWrite;
    TestBoundary boundary(){return {[this](QQuickWindow *,const Guard &,int,const reader_selected::ValidatedSourceAction &action){++samples;for(unsigned n=0;n<repeats;++n)action(reader,nullptr,nullptr,{},observed,guard);return observed;},[this]{return now;}};}
    Result run(const std::shared_ptr<Claim> &claim=std::make_shared<Claim>(),Guard permit=[]{return true;},Config c=config(),int budget=1000){
        return reader_fixed_pen_handoff::run(&window,c,claim,permit,[&](const QByteArray &b){++writes;request=b;const auto f=onWrite;if(f)f();return true;},budget,boundary());
    }
};
}
int main(int argc,char **argv){QGuiApplication app(argc,argv);try{
    scenario([]{Seam s;auto claim=std::make_shared<Claim>();const auto r=s.run(claim);require(r.claimed&&r.matched&&r.attempted&&r.published&&r.reconciliationRequired&&r.refusal.isEmpty(),"fresh source one handoff");require(r.requestMs==10000&&r.originalRequest==QByteArray(32,'a')+" 123 456 10000\n"&&s.writes==1&&s.request==r.originalRequest,"exact original cross-process request");require(!s.run(claim).attempted&&s.writes==1,"same claim spent");});
    for(int field=0;field<10;++field)scenario([=]{Seam s;switch(field){case 0:s.observed.documentId="bad";break;case 1:s.observed.pageId="bad";break;case 2:s.observed.pageIndex=5;break;case 3:s.observed.pageCount=11;break;case 4:s.observed.ready=false;break;case 5:s.observed.creationJoinObserved=false;break;case 6:s.observed.mapCoherent=false;break;case 7:s.observed.working=true;break;case 8:s.observed.loading=true;break;default:s.observed.nativeAuthority=true;}const auto r=s.run();require(!r.attempted&&!s.writes,"source mismatch never publishes");});
    scenario([]{Seam s;s.guard=[&]{delete s.reader.data();return true;};const auto r=s.run();require(!r.attempted&&!s.writes&&!s.reader,"source guard deletion before dispatch");});
    scenario([]{Seam s;unsigned n=0;s.guard=[&]{return ++n<2;};const auto r=s.run();require(!r.attempted&&!s.writes,"final source guard required");});
    scenario([]{Seam s;s.onWrite=[&]{delete s.reader.data();};const auto r=s.run();require(r.attempted&&r.published&&r.reconciliationRequired&&!s.reader&&s.writes==1,"terminal publication can invalidate Reader");});
    scenario([]{Seam s;s.onWrite=[] {throw std::runtime_error("owned");};const auto r=s.run();require(r.attempted&&r.reconciliationRequired&&r.refusal=="exception"&&s.writes==1,"publisher exception keeps possible effect");});
    scenario([]{Seam s;s.onWrite=[&]{s.now=20000;};const auto r=s.run();require(r.attempted&&r.published&&r.reconciliationRequired&&r.refusal=="deadline-after-publication","postpublication deadline remains unknown effect");});
    scenario([]{Seam s;const auto r=s.run(std::make_shared<Claim>(),[&]{s.now=20000;return true;});require(!r.attempted&&!s.samples&&!s.writes,"permission cannot spend original deadline then authorize");});
    scenario([]{Seam s;const auto r=s.run(std::make_shared<Claim>(),[&]{s.now=9999;return true;});require(!r.attempted&&!s.writes,"regressed clock refuses");});
    for(quint64 remaining:{quint64(0),quint64(3000),quint64(3250)})scenario([=]{Seam s;auto c=config();c.setupEnd=s.now+remaining;const auto r=s.run(std::make_shared<Claim>(),[]{return true;},c);require(!r.attempted&&!s.writes,"no full original recovery and handoff reserve");});
    scenario([]{Seam s;s.repeats=2;const auto r=s.run();require(r.attempted&&r.published&&r.refusal=="source-callback-repeat"&&s.writes==1,"duplicate modeled callback no duplicate handoff");});
    scenario([]{Seam s;auto claim=std::make_shared<Claim>();Result nested;bool once=false;const auto r=s.run(claim,[&]{if(!once){once=true;nested=s.run(claim);}return true;});require(r.published&&s.writes==1&&!nested.attempted&&nested.refusal=="claim-spent","reentrant permission cannot duplicate");});
    scenario([]{Seam s;auto claim=std::make_shared<Claim>();require(!s.run(claim,[]{return false;}).attempted,"initial cancel");require(!s.run(claim).attempted&&!s.writes,"cancel cannot renew claim");});
    scenario([]{Seam s;const auto r=s.run(std::make_shared<Claim>(),[]()->bool{throw std::runtime_error("owned");});require(!r.attempted&&r.refusal=="exception"&&!s.writes,"permission exception no effect");});
    scenario([]{Seam s;QTemporaryDir dir;require(dir.isValid(),"real owned private directory");const auto path=QFile::encodeName(dir.path()+"/pen-go.request");auto claim=std::make_shared<Claim>();const auto r=reader_fixed_pen_handoff::run(&s.window,config(),claim,[]{return true;},[&](const QByteArray &b){int fd=::open(path.constData(),O_WRONLY|O_CREAT|O_EXCL|O_NOFOLLOW|O_CLOEXEC,0600);if(fd<0)return false;const auto n=::write(fd,b.constData(),std::size_t(b.size()));const bool closed=::close(fd)==0;return n==b.size()&&closed;},1000,s.boundary());require(r.published,"real owned original request file");QFile file(QString::fromLocal8Bit(path));require(file.open(QIODevice::ReadOnly)&&file.readAll()==r.originalRequest,"readback original cross-process bytes");require(::open(path.constData(),O_WRONLY|O_CREAT|O_EXCL|O_CLOEXEC,0600)<0,"existing request cannot be overwritten");});
    scenario([]{Seam s;const auto r=reader_fixed_pen_handoff::run(&s.window,config(),std::make_shared<Claim>(),[]{return true;},[](const QByteArray &){return false;},1000,s.boundary());require(r.attempted&&!r.published&&r.reconciliationRequired&&r.refusal=="publication-unknown","failed publication never licenses resend");});
    std::printf("PASS fixed pen handoff cases=%u checks=%u native_join_modeled=true clock_modeled=true native_execution=false\n",cases,checks);return 0;
}catch(const std::exception &e){std::fprintf(stderr,"FAIL fixed pen handoff %s cases=%u checks=%u\n",e.what(),cases,checks);return 1;}}
