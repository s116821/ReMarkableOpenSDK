// Real private-file admission in an isolated container /run. No holder or tablet.
#include "reader_write_reopen_backend.h"
#include <QCoreApplication>
#include <QFile>
#include <QDir>
#include <cstdio>
#include <cstdlib>
namespace {
unsigned checks=0,serial=0;
void require(bool b,const char *s){++checks;if(!b){std::fprintf(stderr,"FAIL %s\n",s);std::exit(1);}}
QByteArray read(const QByteArray &p){QFile f(QString::fromLatin1(p));require(f.open(QIODevice::ReadOnly),"fixture-read");return f.readAll();}
struct Area {
 reader_write_reopen::Config config;QByteArray root,boot;
 Area(){
  config.nonce=QByteArray::number(++serial,16).rightJustified(32,'0');config.pid=QByteArray::number(::getpid());
  const auto stat=read("/proc/self/stat");config.start=stat.mid(stat.lastIndexOf(')')+2).split(' ')[19];
  boot=read("/proc/sys/kernel/random/boot_id").trimmed();const auto second=reader_fixed_pen_handoff::bootMs()/1000;
  config.setupEnd=(second+90)*1000;config.target={"11111111-1111-4111-8111-111111111111","22222222-2222-4222-8222-222222222222",6,12};
  root="/run/rmb-qt-probe-"+config.nonce;require(!::mkdir(root,0700),"mkdir-root");
  put("owner",config.nonce);put("attempt.identity",config.pid+' '+config.start+'\n');
  put("deadline",QByteArray::number(second-1)+' '+QByteArray::number(second+359)+'\n');
  put("candidate.window",config.nonce+' '+QByteArray::number(second-1)+' '+QByteArray::number(second+359)+' '+QByteArray::number(second)+' '+QByteArray::number(second+90)+'\n');
 }
 QByteArray path(const char *n)const{return root+'/'+n;}
 void put(const char *n,const QByteArray &b){int fd=::open(path(n),O_WRONLY|O_CREAT|O_EXCL|O_CLOEXEC,0600);require(fd>=0,"put-open");require(::write(fd,b.constData(),size_t(b.size()))==b.size(),"put-write");require(!::close(fd),"put-close");}
 std::shared_ptr<reader_write_reopen::NativeFiles> open(){return reader_write_reopen::NativeFiles::create(config,boot);}
};
}
int main(int argc,char **argv){
 QCoreApplication app(argc,argv);::umask(0077);require(::geteuid()==0,"container-root");
 {
  const QByteArray nonce(32,'a'),path="/system.slice/rmb-qt-pen-"+nonce+".service";
  const auto v1="1:name=systemd:"+path+'\n',v2="0::"+path+'\n';
  require(reader_write_reopen::native_files_detail::holderCgroup(v1+v2,nonce)&&reader_write_reopen::native_files_detail::holderCgroup(v2+v1,nonce),"hybrid-same-service");
  require(!reader_write_reopen::native_files_detail::holderCgroup(v1,nonce)&&!reader_write_reopen::native_files_detail::holderCgroup(v2,nonce),"missing-hierarchy");
  require(!reader_write_reopen::native_files_detail::holderCgroup(v1+v1,nonce),"duplicate-hierarchy");
  require(!reader_write_reopen::native_files_detail::holderCgroup(v1+"0::/system.slice/xochitl.service\n",nonce),"ui-coupling");
  require(!reader_write_reopen::native_files_detail::holderCgroup(v1+"0::/system.slice/rmb-qt-insertion-a-actor.service\n",nonce),"actor-coupling");
  require(!reader_write_reopen::native_files_detail::holderCgroup(v1+v2+v2,nonce)&&!reader_write_reopen::native_files_detail::holderCgroup((v1+v2).chopped(1),nonce),"extra-or-incomplete");
 }
 {
  Area a;auto io=a.open();require(bool(io)&&io->admitted(),"valid-admission");
  require(!io->holderReady().observed&&!io->readyForRequest()&&!io->publishReady(),"missing-holder-refuses");
  require(io->request().missing,"missing-request");a.put("selected-request",a.config.nonce+' '+a.config.pid+' '+a.config.start+'\n');require(io->request().observed,"request-read");
  require(io->consumeRequest()&&!io->consumeRequest(),"request-once");
  require(!io->publishGo("unbound"),"no-go-without-holder");require(::access(a.path("pen-go.request"),F_OK)!=0,"no-go-file");
  reader_write_reopen::Result result;result.finished=true;result.refusal="owned-no-holder";io->report(result);
  auto scalar=read(a.path("selected-result"));require(scalar.contains("owned-no-holder"),"refusal-record");
  const auto completion=read(a.path("selected-complete")).split(' ');require(completion.size()==6&&completion[3]==QCryptographicHash::hash(scalar,QCryptographicHash::Sha256).toHex(),"completion-original-hash");
  io->report({});require(read(a.path("selected-result"))==scalar,"report-once");
  io->cancelPen();require(read(a.path("pen-cancel"))==a.config.nonce+' '+a.config.pid+' '+a.config.start+'\n',"cancel-bound");require(!io->admitted(),"cancel-revokes");io->cancelPen();
 }
 for(const char *marker:{"stop.request","restore.claim","selected-cancel","pen-cancel"}){Area a;a.put(marker,"x");require(!a.open(),"marker-refuses-create");}
 {Area a;auto io=a.open();require(bool(io),"control-before");require(!::chmod(a.path("owner"),0644),"mutate-mode");require(!io->admitted(),"control-mode-drift");}
 {Area a;auto io=a.open();require(bool(io),"replace-before");require(!::rename(a.path("owner"),a.path("owner-old")),"control-rename");a.put("owner",a.config.nonce);require(!io->admitted(),"control-inode-drift");}
 {Area a;a.config.start="999999999";require(!a.open(),"process-start-binding");}
 {Area a;a.boot="aaaaaaaa-aaaa-4aaa-8aaa-aaaaaaaaaaaa";require(!a.open(),"boot-binding");}
 {Area a;a.config.setupEnd+=1;require(!a.open(),"window-end-binding");}
 {Area a;auto io=a.open();require(bool(io),"bad-ready-before");a.put("pen-holder.ready","wrong\n");require(!io->holderReady().observed&&!io->publishReady(),"bad-ready-refusal");}
 {Area a;auto io=a.open();require(bool(io),"linked-ready-before");a.put("pen-holder.ready","publication-in-progress\n");require(!::link(a.path("pen-holder.ready"),a.path(".pending-pen-ready")),"ready-two-links");const auto count=QDir("/proc/self/fd").entryList(QDir::AllEntries|QDir::NoDotAndDotDot).size();for(unsigned i=0;i<10;++i)require(!io->holderReady().observed,"linked-ready-refused");require(QDir("/proc/self/fd").entryList(QDir::AllEntries|QDir::NoDotAndDotDot).size()==count,"failed-ready-does-not-pin-fd");}
 {Area a;auto io=a.open();require(bool(io),"request-binding-before");a.put("selected-request",a.config.nonce+' '+a.config.pid+' '+a.config.start+'\n');require(io->consumeRequest(),"consume-request");require(!::rename(a.path("selected-request"),a.path("selected-request-old")),"request-rename");a.put("selected-request",a.config.nonce+' '+a.config.pid+' '+a.config.start+'\n');require(!io->admitted(),"request-replacement-revokes");}
 {Area a;auto io=a.open();require(bool(io),"pending-request-before");a.put("selected-request",a.config.nonce+' '+a.config.pid+' '+a.config.start+'\n');a.put(".pending-request","partial");require(!io->consumeRequest(),"pending-request-refuses");}
 std::printf("write-reopen backend owned: %u checks; actual private files, no holder/device actions\n",checks);
}
