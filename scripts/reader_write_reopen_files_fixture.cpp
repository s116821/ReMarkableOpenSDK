#include "reader_write_reopen_files.h"
#include <QCoreApplication>
#include <QTemporaryDir>
#include <cstdio>
#include <cstdlib>

using reader_write_reopen_files::Directory;
namespace {
unsigned checks=0;
void require(bool ok,const char *what){++checks;if(!ok){std::fprintf(stderr,"FAIL %s\n",what);std::exit(1);}}
struct Area {
 QTemporaryDir temporary;QString path;
 Area(){require(temporary.isValid(),"temp");path=temporary.path()+"/root";require(::mkdir(QFile::encodeName(path).constData(),0700)==0,"mkdir");}
 QByteArray file(const char *n)const{return QFile::encodeName(path+'/'+QString::fromLatin1(n));}
 std::shared_ptr<Directory> open()const{return Directory::open(path);}
 void put(const char *n,const QByteArray &b,mode_t mode=0600){int fd=::open(file(n),O_CREAT|O_EXCL|O_WRONLY|O_CLOEXEC,mode);require(fd>=0,"put-open");require(::write(fd,b.constData(),size_t(b.size()))==b.size(),"put-write");require(::close(fd)==0,"put-close");}
};
}
int main(int argc,char **argv){
 QCoreApplication app(argc,argv);::umask(0077);
 {
  Area a;auto d=a.open();require(bool(d)&&d->bound(),"root-bound");require(d->read("missing",10).missing,"missing");
  auto p=d->publish("result",".pending-result","original\n");require(p.attempted&&p.linked&&p.committed,"publish");
  auto r=d->read("result",9);require(r.observed&&r.bytes=="original\n","read-original");require(d->absent(".pending-result"),"pending-gone");
  require(!d->publish("result",".pending-result","overwrite").attempted,"no-overwrite");require(!d->read("result",8).observed,"read-cap");
  const int owned=d->makeCapture("capture");require(owned>=0,"capture-open");struct stat s{};require(!::fstat(owned,&s)&&S_ISDIR(s.st_mode)&&(s.st_mode&0777)==0700,"capture-mode");require(::close(owned)==0,"capture-close");require(d->makeCapture("capture")<0,"capture-no-reuse");
  const int borrowed=d->borrowedFd();require(d->retire(),"retire");require(!d->bound()&&d->borrowedFd()==-1,"retired-state");errno=0;require(::fcntl(borrowed,F_GETFD)==-1&&errno==EBADF,"retired-fd");require(!d->retire(),"no-double-retire");require(!d->read("result",9).observed&&!d->publish("later",".pending-later","x").attempted,"no-after-retire");
  QFile f(a.file("result"));require(f.open(QIODevice::ReadOnly)&&f.readAll()=="original\n","retire-keeps-evidence");
 }
 {
  Area a;auto d=a.open();a.put(".pending-result","partial");require(!d->publish("result",".pending-result","x").attempted,"pending-refuses");require(d->absent("result"),"no-result-from-partial");
  a.put("empty",{});require(!d->read("empty",0).observed&&d->read("empty",0,true).observed,"empty-explicit");
  a.put("large",QByteArray(131073,'x'));require(!d->read("large",131072).observed&&!d->read("large",131073).observed,"absolute-cap");
  require(!d->publish("oversize",".pending-big",QByteArray(4097,'x')).attempted,"publish-cap");
  for(const char *bad:{"",".","..","../escape","UPPER","a/b","a b"}){require(!d->read(bad,32).observed&&!d->publish(bad,".pending","x").attempted&&!d->absent(bad)&&d->makeCapture(bad)<0,"invalid-name");}
  require(!d->publish("same","same","x").attempted,"same-name");require(!d->publish("empty-write",".pending-empty",{}).attempted,"empty-publish");
 }
 {
  Area a;auto d=a.open();a.put("regular","data");require(!::symlink("regular",a.file("symbolic")),"symlink");require(!d->read("symbolic",32).observed,"no-follow-file");
  require(!::link(a.file("regular"),a.file("hardlink")),"hardlink");require(!d->read("regular",32).observed&&!d->read("hardlink",32).observed,"nlink-reject");
  require(!::mkfifo(a.file("fifo"),0600),"fifo");require(!d->read("fifo",32).observed,"fifo-nonblocking-refusal");
  require(!::mkdir(a.file("dir"),0700),"directory");require(!d->read("dir",32).observed,"directory-not-file");
  a.put("mode","data");require(!::chmod(a.file("mode"),0644),"chmod-file");require(!d->read("mode",32).observed,"file-mode-refusal");
 }
 {
  Area a;auto d=a.open();require(!::chmod(QFile::encodeName(a.path),0755),"chmod-root");require(!d->bound()&&!d->publish("x",".pending-x","x").attempted,"root-mode-refusal");require(!a.open(),"open-root-mode-refusal");
 }
 {
  Area a;auto d=a.open();const auto old=a.path+"-old";require(!::rename(QFile::encodeName(a.path),QFile::encodeName(old)),"rename-root");require(!::mkdir(QFile::encodeName(a.path),0700),"replace-root");require(!d->bound()&&!d->read("x",32).observed&&!d->publish("x",".pending-x","x").attempted,"root-replaced-refusal");
 }
 {
  Area a;const auto link=a.path+"-link";require(!::symlink(QFile::encodeName(a.path),QFile::encodeName(link)),"root-symlink");require(!Directory::open(link),"no-follow-root");
 }
 std::printf("write-reopen files owned: %u checks; no device actions\n",checks);
}
