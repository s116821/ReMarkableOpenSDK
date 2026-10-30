// Exact extracted startup functions; owned root and explicit identity fault model.
// No startup constructor, UI, native module, device, manager, or writer is run.
#include "reader_capture_files.h"
#include <QByteArray>
#include <QCoreApplication>
#include <QFile>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <cstdint>
#include <sys/wait.h>
#include <sys/resource.h>
#include <unistd.h>

static const char *rootPath=nullptr;
static constexpr char nonce[]="aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";
static bool identityFault=false;
static QByteArray identityBytes;
#include "admission.inc"

static bool writeFile(const char *name,const QByteArray &bytes) {
    QFile file(QString::fromLocal8Bit(rootPath)+'/'+QString::fromLatin1(name));
    if(!file.open(QIODevice::WriteOnly|QIODevice::Truncate))return false;
    const bool okay=file.write(bytes)==bytes.size();file.close();return okay;
}
static bool unlinkFile(const char *name){return ::unlink((QByteArray(rootPath)+'/'+name).constData())==0;}
static void setup() {
    char *p=::strdup("/tmp/write-reopen-admission-owned-XXXXXX");
    if(!p||!::mkdtemp(p))::_exit(80);
    rootPath=p;::umask(0077);
    if(!writeFile("owner",nonce)||!writeFile("attempt.identity",reader_selected::selfIdentity()))::_exit(81);
    for(const char *name:{"actor.claim","actor.ready","activation.claim","candidate-start.claim","start.request","attempt.claim"})
        if(!writeFile(name,QByteArray(nonce)+'\n'))::_exit(82);
    timespec at{};if(::clock_gettime(CLOCK_BOOTTIME,&at)||at.tv_sec<0)::_exit(83);
    if(!writeFile("deadline",QByteArray::number(at.tv_sec)+' '+QByteArray::number(at.tv_sec+360)+'\n'))::_exit(84);
}
struct Case {const char *name;bool oldAbort;bool oldValue;bool fixedValue;};
static const Case cases[]={
    {"valid",false,true,true},{"restore",true,false,false},{"cancel",true,false,false},{"stop",true,false,false},
    {"missing-claim",true,false,false},{"bad-claim",true,false,false},{"owner-mismatch",true,false,false},
    {"missing-deadline",true,false,false},{"empty-deadline",true,false,false},{"deadline-no-lf",false,false,false},
    {"root-mode",true,false,false},{"malformed-deadline",false,false,false},
    {"identity-empty-admission",true,false,false},{"identity-empty-controller",true,false,false},
    {"identity-no-lf-controller",false,true,false},{"identity-valid-controller",false,true,true}
};
static bool runCase(const char *name) {
    setup();
    if(!std::strcmp(name,"restore")){if(!writeFile("restore.claim",QByteArray(nonce)+'\n'))::_exit(85);}
    if(!std::strcmp(name,"cancel")){if(!writeFile("selected-cancel",QByteArray(nonce)+'\n'))::_exit(85);}
    if(!std::strcmp(name,"stop")){if(!writeFile("stop.request",QByteArray(nonce)+'\n'))::_exit(85);}
    if(!std::strcmp(name,"missing-claim")){if(!unlinkFile("actor.ready"))::_exit(85);}
    if(!std::strcmp(name,"bad-claim")){if(!writeFile("activation.claim","wrong\n"))::_exit(85);}
    if(!std::strcmp(name,"owner-mismatch")){if(!writeFile("owner","wrong"))::_exit(85);}
    if(!std::strcmp(name,"missing-deadline")){if(!unlinkFile("deadline"))::_exit(85);}
    if(!std::strcmp(name,"empty-deadline")){if(!writeFile("deadline",{}))::_exit(85);}
    if(!std::strcmp(name,"deadline-no-lf")){if(!writeFile("deadline","1 361"))::_exit(85);}
    if(!std::strcmp(name,"root-mode")){if(::chmod(rootPath,0755))::_exit(85);}
    if(!std::strcmp(name,"malformed-deadline")){if(!writeFile("deadline","broken\n"))::_exit(85);}
    if(!std::strcmp(name,"identity-empty-admission")){identityFault=true;identityBytes.clear();}
    if(std::strstr(name,"-controller")) {
        identityFault=true;identityBytes=std::strstr(name,"empty")?QByteArray():QByteArray("123 456");
        if(std::strstr(name,"valid"))identityBytes+='\n';
        return controllerIdentityFragment(QByteArray(nonce)+" 1 361 10 100\n");
    }
    return actorAdmission();
}
int main() {
    rlimit core{0,0};if(::setrlimit(RLIMIT_CORE,&core))return 1;
    unsigned checked=0,aborted=0;
    for(const auto &c:cases){
        const auto pid=::fork();if(pid<0)return 2;
        if(!pid){const bool answer=runCase(c.name);::_exit(answer?10:11);}
        int status=0;if(::waitpid(pid,&status,0)!=pid)return 3;
        const bool signalAbort=WIFSIGNALED(status)&&WTERMSIG(status)==SIGABRT;
        const bool expectedAbort=FIXTURE_OLD&&c.oldAbort;
        const bool expected=FIXTURE_OLD?c.oldValue:c.fixedValue;
        const bool okay=expectedAbort?signalAbort:WIFEXITED(status)&&WEXITSTATUS(status)==(expected?10:11);
        std::printf("%s status=%d abort=%d pass=%d\n",c.name,status,int(signalAbort),int(okay));
        if(!okay)return 4;
        ++checked;if(signalAbort)++aborted;
    }
    std::printf("owned_admission cases=%u observed_aborts=%u old=%d qt=%s\n",checked,aborted,FIXTURE_OLD,qVersion());
    return 0;
}
