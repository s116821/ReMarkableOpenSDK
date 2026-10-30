// Real owned pthread mutexes only. No native receiver, profile or hook.
#include "reader_insertion_mutex_probe.h"
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <stdexcept>

using namespace reader_insertion_mutex_probe;
namespace {
int checks=0, tries=0, unlocks=0;
void check(bool yes,const char *what) { if(!yes){std::fprintf(stderr,"FAIL %s\n",what);std::abort();}++checks; }
int tried(pthread_mutex_t *m) { ++tries; return ::pthread_mutex_trylock(m); }
int unlocked(pthread_mutex_t *m) { ++unlocks;check(tries==1,"unlock immediately follows sole successful try");return ::pthread_mutex_unlock(m); }
int badUnlock(pthread_mutex_t *m) { (void)::pthread_mutex_unlock(m);return EPERM; }
int throwingTry(pthread_mutex_t*) {throw std::runtime_error("owned try");}
int throwingUnlock(pthread_mutex_t *m) {(void)::pthread_mutex_unlock(m);throw std::runtime_error("owned unlock");}
Config config(pthread_mutex_t &mutex) {
    const auto begin=reinterpret_cast<std::uintptr_t>(&mutex);
    return {&mutex,{begin,begin+sizeof mutex},tried,unlocked};
}
}
int main(int argc,char **argv) {
    check(argc==2,"one fixed owned mode");
    const char *mode=argv[1];
    pthread_mutex_t mutex=PTHREAD_MUTEX_INITIALIZER;
    Config c=config(mutex);
    if(!std::strcmp(mode,"invalid")) {
        for(unsigned i=0;i!=11;++i) {
            Config bad=c;
            switch(i) {
            case 0:bad.mutex=nullptr;break;
            case 1:bad.mutex=reinterpret_cast<pthread_mutex_t*>(c.storage.begin+1);break;
            case 2:bad.storage.begin=0;break;
            case 3:++bad.storage.begin;break;
            case 4:bad.storage.end=c.storage.begin;break;
            case 5:--bad.storage.end;break;
            case 6:bad.storage.begin=c.storage.end;break;
            case 7:bad.tryLock=nullptr;break;
            case 8:bad.unlock=nullptr;break;
            case 9:bad.unlock=bad.tryLock;break;
            case 10:bad.mutex=reinterpret_cast<pthread_mutex_t*>(~std::uintptr_t(0)&~(std::uintptr_t(alignof(pthread_mutex_t))-1));
                bad.storage={reinterpret_cast<std::uintptr_t>(bad.mutex),~std::uintptr_t(0)};break;
            }
            check(Probe::create(bad)==nullptr,"malformed bounded config rejected before callbacks");
            check(tries==0&&unlocks==0,"invalid config does not invoke callbacks");
        }
        pthread_mutexattr_t attr;check(::pthread_mutexattr_init(&attr)==0,"owned attr init");
        check(::pthread_mutexattr_settype(&attr,PTHREAD_MUTEX_RECURSIVE)==0,"owned recursive attr");
        pthread_mutex_t recursive;check(::pthread_mutex_init(&recursive,&attr)==0,"owned recursive mutex");
        check(Probe::create(config(recursive))==nullptr,"recursive kind rejected");
        check(::pthread_mutex_destroy(&recursive)==0&&::pthread_mutexattr_destroy(&attr)==0,"owned recursive cleanup");
    } else {
        if(!std::strcmp(mode,"unlock-failure"))c.unlock=badUnlock;
        if(!std::strcmp(mode,"try-throws"))c.tryLock=throwingTry;
        if(!std::strcmp(mode,"unlock-throws"))c.unlock=throwingUnlock;
        Probe *probe=Probe::create(c);check(probe!=nullptr,"qualified owned default mutex");
        if(!std::strcmp(mode,"busy"))check(::pthread_mutex_lock(&mutex)==0,"owned original retains lock");
        const Result result=probe->once();
        check(successful(result)&&result.tryResult==0&&result.unlockResult==0,"complete successful facts only after unlock");
        check(tries==1&&unlocks==1,"exact one try/unlock");
        check(::pthread_mutex_trylock(&mutex)==0,"mutex reusable after successful witness");
        check(::pthread_mutex_unlock(&mutex)==0,"owned verification releases mutex");
        if(!std::strcmp(mode,"reuse"))probe->once();
    }
    check(::pthread_mutex_destroy(&mutex)==0,"owned default mutex cleanup");
    std::printf("PASS mode=%s checks=%d no_native_qualification=1\n",mode,checks);
}
