// Actual owned pipe/signal controls; no tablet, UI, native producer or input.
#define READER_CAPTURE_READBACK_OWNED_HELPERS_ONLY
#include "reader_identity_capture_readback.cpp"
#include <sys/wait.h>
#include <sys/ioctl.h>
#include <chrono>
#include <stdexcept>
#include <cstring>
using namespace capture_process_readback;
static void need(bool value,const char *why){if(!value)throw std::runtime_error(why);}
static int reap(pid_t pid){int status=0;for(int n=0;n<600;++n){const auto found=::waitpid(pid,&status,WNOHANG);if(found==pid)return status;need(found==0||errno==EINTR,"wait");::usleep(5000);}::kill(pid,SIGKILL);::waitpid(pid,&status,0);throw std::runtime_error("owned child timeout");}
static void fill(int fd){need(::fcntl(fd,F_SETFL,::fcntl(fd,F_GETFL)|O_NONBLOCK)==0,"fill flags");char b[4096]{};while(::write(fd,b,sizeof b)>0){}need(errno==EAGAIN||errno==EWOULDBLOCK,"full pipe");}
int main(int argc,char **argv){try{
 const bool signalTest=argc==1;need(argc==1||(argc==2&&!std::strcmp(argv[1],"--skip-signal-test")),"owned mode");
 const QByteArray frame(200000,'x');unsigned cases=0;
 {int p[2];need(::pipe(p)==0,"pipe");const pid_t child=::fork();need(child>=0,"fork");
  if(!child){::close(p[1]);char b[37];unsigned total=0,sum=0;ssize_t n;while((n=::read(p[0],b,sizeof b))>0){total+=unsigned(n);for(ssize_t i=0;i<n;++i)sum+=unsigned(static_cast<unsigned char>(b[i]));}::_Exit(n==0&&total==200000&&sum==24000000?0:90);}
  ::close(p[0]);unsigned calls=0;auto permit=[&]{++calls;return true;};const bool good=writeFrameBounded(p[1],frame,permit);::close(p[1]);const int status=reap(child);need(good&&calls>=2&&WIFEXITED(status)&&WEXITSTATUS(status)==0,"whole original output across partial writes");++cases;}
 {int p[2];need(::pipe(p)==0,"full pipe");fill(p[1]);int before=0,after=0;need(::ioctl(p[0],FIONREAD,&before)==0,"before");unsigned calls=0;auto permit=[&]{return ++calls<4;};need(!writeFrameBounded(p[1],frame,permit),"revocation");need(::ioctl(p[0],FIONREAD,&after)==0&&before==after&&calls==4,"no new bytes after full/revoke");::close(p[0]);::close(p[1]);++cases;}
 {int p[2];need(::pipe(p)==0,"closed reader");::close(p[0]);struct sigaction old{},ignore{};ignore.sa_handler=SIG_IGN;need(!::sigemptyset(&ignore.sa_mask)&&!::sigaction(SIGPIPE,&ignore,&old),"ignore broken pipe");auto permit=[]{return true;};const bool good=writeFrameBounded(p[1],frame,permit);need(!::sigaction(SIGPIPE,&old,nullptr),"restore signal");::close(p[1]);need(!good,"closed peer refusal");++cases;}
 if(signalTest){int p[2];need(::pipe(p)==0,"alarm pipe");fill(p[1]);const pid_t child=::fork();need(child>=0,"alarm fork");
  if(!child){sigset_t blocked{};struct sigaction ignored{};ignored.sa_handler=SIG_IGN;if(::sigemptyset(&blocked)||::sigaddset(&blocked,SIGALRM)||::sigprocmask(SIG_BLOCK,&blocked,nullptr)||::sigemptyset(&ignored.sa_mask)||::sigaction(SIGALRM,&ignored,nullptr)||!armCollectionAlarm())::_Exit(90);::alarm(1);auto permit=[]{return true;};const bool good=writeFrameBounded(p[1],frame,permit);::_Exit(good?91:92);}
  const int status=reap(child);::close(p[0]);::close(p[1]);need(WIFSIGNALED(status)&&WTERMSIG(status)==SIGALRM,"inherited ignored/blocked alarm reset and unblocked");++cases;}
 std::printf("PASS owned capture output cases=%u native_execution=false alarm_kernel_test=%s\n",cases,signalTest?"host":"skipped-arm-qemu");return 0;
}catch(const std::exception&e){std::fprintf(stderr,"FAIL capture output %s\n",e.what());return 1;}}

