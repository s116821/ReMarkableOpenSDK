// Original owned host and entry veneers. No firmware bodies or registration.
#include "owned_api.h"
#include <QCoreApplication>
#include <QThread>
#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <thread>
struct Counters {unsigned stage=0,mainEntered=0,appCreated=0,workerStarted=0,outerCalls=0,innerCalls=0,checks=0;bool offGui=false;};
static Counters c; // Trivial zero initialization: callable before host constructors.
static std::thread::id gui;
static int workerToken=0,mapToken=0,optionalToken=0;
static void check(bool v,const char *why){++c.checks;if(!v){std::fprintf(stderr,"FAILED %s\n",why);std::abort();}}
extern "C" bool owned_loader_note(unsigned stage) {
    if(c.mainEntered||c.appCreated||c.workerStarted||stage!=c.stage+1){std::fprintf(stderr,"PREMAIN_ORDER_FAILED\n");return false;}
    c.stage=stage;std::fprintf(stderr,"HOST_NOTE stage=%u main=0 app=0 owned_workers=0\n",stage);return true;
}
extern "C" void owned_loader_outer_body(void *w,int i,const void *o,Word p,QSizeF z,int s,Word h,const QUuid &id) {
    ++c.outerCalls;c.offGui=std::this_thread::get_id()!=gui;
    check(w==&workerToken&&i==3&&o==&optionalToken&&p==0x7f123401&&z==QSizeF(1404.25,1872.5)&&s==2&&h==0x7e123400,"transparent outer full carriers");
    check(id==QUuid(QStringLiteral("11111111-1111-4111-8111-111111111111")),"transparent outer UUID reference");
    const QString target=id.toString(QUuid::WithoutBraces),templ=QStringLiteral("owned-template");
    owned_loader_inner(&mapToken,i,target,templ,-123456789,0x7f123456);
}
extern "C" void owned_loader_inner_body(void *m,int i,const QString &id,const QString &t,std::int32_t a,std::int32_t b) {
    ++c.innerCalls;
    check(m==&mapToken&&i==3&&id==QStringLiteral("11111111-1111-4111-8111-111111111111")&&t==QStringLiteral("owned-template")&&a==-123456789&&b==0x7f123456,"transparent inner full words/references");
}
asm(R"(
.syntax unified
.arch armv7-a
.fpu neon-vfpv4
.arm
.text
.align 2
.global owned_loader_outer
.type owned_loader_outer,%function
owned_loader_outer:
.fnstart
 push {r4-r11,lr}
.save {r4-r11,lr}
 mov r4,r0
 vpush {d8-d9}
.vsave {d8-d9}
 sub sp,sp,#12
.pad #12
 ldr r12,[sp,#64]
 str r12,[sp,#0]
 ldr r12,[sp,#68]
 str r12,[sp,#4]
 ldr r12,[sp,#72]
 str r12,[sp,#8]
 bl owned_loader_outer_body
 add sp,sp,#12
 vpop {d8-d9}
 pop {r4-r11,pc}
.fnend
.size owned_loader_outer,.-owned_loader_outer
.align 2
.global owned_loader_inner
.type owned_loader_inner,%function
owned_loader_inner:
.fnstart
 push {r4-r11,lr}
.save {r4-r11,lr}
 mov r5,r3
 vpush {d8-d9}
.vsave {d8-d9}
 sub sp,sp,#12
.pad #12
 ldr r12,[sp,#64]
 str r12,[sp,#0]
 ldr r12,[sp,#68]
 str r12,[sp,#4]
 bl owned_loader_inner_body
 add sp,sp,#12
 vpop {d8-d9}
 pop {r4-r11,pc}
.fnend
.size owned_loader_inner,.-owned_loader_inner
.text
)");
class Worker final:public QThread {void run()override{
    ++c.workerStarted;
    const QUuid id(QStringLiteral("11111111-1111-4111-8111-111111111111"));
    owned_loader_outer(&workerToken,3,&optionalToken,0x7f123401,QSizeF(1404.25,1872.5),2,0x7e123400,id);
}};
int main(int argc,char **argv) {
    c.mainEntered=1;std::fprintf(stderr,"HOST_MAIN stage=%u\n",c.stage);
    check(c.stage==2,"installation completed before main");
    check(c.appCreated==0&&c.workerStarted==0,"no owned application/worker before main");
    QCoreApplication app(argc,argv);c.appCreated=1;gui=std::this_thread::get_id();
    Worker thread;thread.start();thread.wait();
    check(c.outerCalls==1&&c.innerCalls==1&&c.workerStarted==1&&c.offGui,"one transparent off-GUI original pair");
    std::printf("{\"passed\":true,\"checks\":%u,\"pre_main_install\":true,\"outer_calls\":%u,\"inner_calls\":%u,\"no_registration\":true,\"native_qualification\":false}\n",c.checks,c.outerCalls,c.innerCalls);
}
