// Original owned layout and failure controls; no native object or private offsets.
#include "reader_insertion_job_snapshot.h"
#include <cstdio>
#include <stdexcept>
#include <string>
using namespace reader_insertion_job_snapshot;
static_assert(sizeof(pthread_mutex_t)==24 && alignof(pthread_mutex_t)==4,"this ARM public pthread ABI");
static unsigned checks=0,tries=0,unlocks=0;
static std::string mode;
static void check(bool value,const char *why){++checks;if(!value){std::fprintf(stderr,"FAILED %s\n",why);std::abort();}}
struct Owned {pthread_mutex_t mutex;std::uint32_t words[2];};
static int tryLock(pthread_mutex_t *mutex) {
    ++tries;
    if(mode=="try-error")return EINVAL;
    if(mode=="try-throw")throw std::runtime_error("owned try failure");
    const int result=::pthread_mutex_trylock(mutex);
    if(mode=="owner-dead")return result==0?EOWNERDEAD:result;
    if(mode=="throw-after-acquire"&&result==0)throw std::runtime_error("owned acquired uncertainty");
    return result;
}
static int unlock(pthread_mutex_t *mutex) {
    ++unlocks;
    if(mode=="unlock-error")return EPERM;
    if(mode=="unlock-throw")throw std::runtime_error("owned unlock uncertainty");
    const int result=::pthread_mutex_unlock(mutex);
    if(mode=="throw-after-unlock")throw std::runtime_error("owned released uncertainty");
    return result;
}
static void fatal(void*,const char *why) {
    std::fprintf(stderr,"FORBIDDEN_FATAL_REPORTER %s\n",why);
    if(mode=="fatal-throw")throw std::runtime_error("owned fatal reporter");
}
static Bounds bounds(const void *p,std::size_t size){const auto b=reinterpret_cast<std::uintptr_t>(p);return {b,b+size};}
static Config config(Owned &o) {return {&o.mutex,o.words,bounds(&o.mutex,sizeof o.mutex),bounds(o.words,sizeof o.words),tryLock,unlock,nullptr,fatal};}
struct Busy {
    Owned *owned;
    pthread_mutex_t gate=PTHREAD_MUTEX_INITIALIZER;
    pthread_cond_t signal=PTHREAD_COND_INITIALIZER;
    bool locked=false,release=false;
};
static void *holdOther(void *context) {
    auto &b=*static_cast<Busy*>(context);
    ::pthread_mutex_lock(&b.owned->mutex);
    ::pthread_mutex_lock(&b.gate);b.locked=true;::pthread_cond_signal(&b.signal);
    while(!b.release)::pthread_cond_wait(&b.signal,&b.gate);
    ::pthread_mutex_unlock(&b.gate);::pthread_mutex_unlock(&b.owned->mutex);return nullptr;
}
int main(int argc,char **argv) {
    check(argc==2,"one scenario");mode=argv[1];
    Owned owned;check(::pthread_mutex_init(&owned.mutex,nullptr)==0,"actual default mutex init");
    owned.words[0]=0x76543210;owned.words[1]=0xfedcba98;
    if(mode=="range") {
        const auto valid=config(owned);
        check(Snapshot::create(valid)!=nullptr,"valid public layout");
        auto invalid=[&](Config candidate){check(Snapshot::create(candidate)==nullptr,"invalid config refused without calls");};
        Config c=valid;c.mutex=nullptr;invalid(c);
        c=valid;c.words=nullptr;invalid(c);
        c=valid;c.mutex=reinterpret_cast<pthread_mutex_t*>(reinterpret_cast<std::uintptr_t>(c.mutex)+1);invalid(c);
        c=valid;c.words=reinterpret_cast<const std::uint32_t*>(reinterpret_cast<std::uintptr_t>(c.words)+1);invalid(c);
        c=valid;--c.mutexStorage.end;invalid(c);
        c=valid;--c.wordStorage.end;invalid(c);
        c=valid;c.wordStorage.begin=c.wordStorage.end;invalid(c);
        c=valid;c.mutexStorage.begin+=4;invalid(c);
        c=valid;c.tryLock=nullptr;invalid(c);
        c=valid;c.unlock=nullptr;invalid(c);
        c=valid;c.words=reinterpret_cast<const std::uint32_t*>(&owned.mutex);c.wordStorage=bounds(&owned.mutex,8);invalid(c);
        c=valid;c.words=reinterpret_cast<const std::uint32_t*>(UINTPTR_MAX-3);c.wordStorage={1,UINTPTR_MAX};invalid(c);
        c=valid;c.mutex=reinterpret_cast<pthread_mutex_t*>(UINTPTR_MAX-3);c.mutexStorage={1,UINTPTR_MAX};invalid(c);
        check(tries==0&&unlocks==0,"configuration never touches mutex");
        std::uint64_t id=99;check(!Snapshot::attribute(nullptr,id)&&id==0,"null callback context refused");
    } else {
        Busy busy;busy.owned=&owned;pthread_t holder{};
        if(mode=="busy") {
            check(::pthread_create(&holder,nullptr,holdOther,&busy)==0,"real other pthread");
            ::pthread_mutex_lock(&busy.gate);while(!busy.locked)::pthread_cond_wait(&busy.signal,&busy.gate);::pthread_mutex_unlock(&busy.gate);
        }
        if(mode=="zero")owned.words[0]=owned.words[1]=0;
        Config c=config(owned);
        // The fatal reporter controls use an unexpected code, without ownership.
        if(mode=="fatal-return"||mode=="fatal-throw")c.tryLock=+[](pthread_mutex_t*)->int{++tries;return EINVAL;};
        auto *snapshot=Snapshot::create(c);check(snapshot!=nullptr,"frozen context created");
        if(mode!="success"&&mode!="zero"&&mode!="busy")
            std::fprintf(stderr,"OWNED_FAILURE_CONTROL %s before snapshot; actual failure reason not observed\n",mode.c_str());
        std::uint64_t id=99;
        const bool result=Snapshot::attribute(snapshot,id);
        const bool expected=mode=="success";
        check(result==expected,"snapshot result");
        check(id==(expected?UINT64_C(0xfedcba9876543210):0),"full unsigned64 or cleared output");
        check(tries==1&&unlocks==(mode=="busy"?0U:1U),"one try and exactly one successful unlock");
        if(mode=="busy") {
            ::pthread_mutex_lock(&busy.gate);busy.release=true;::pthread_cond_signal(&busy.signal);::pthread_mutex_unlock(&busy.gate);
            check(::pthread_join(holder,nullptr)==0,"other owner completed");
        }
        check(::pthread_mutex_trylock(&owned.mutex)==0,"released before helper return");::pthread_mutex_unlock(&owned.mutex);
        id=99;check(!snapshot->read(id)&&id==0&&tries==1,"no retry after any first outcome");
        check(::pthread_cond_destroy(&busy.signal)==0&&::pthread_mutex_destroy(&busy.gate)==0,"owned gate cleanup");
    }
    check(::pthread_mutex_destroy(&owned.mutex)==0,"owned native-free cleanup");
    std::printf("{\"passed\":true,\"scenario\":\"%s\",\"checks\":%u,\"tries\":%u,\"unlocks\":%u,\"native_qualification\":false}\n",mode.c_str(),checks,tries,unlocks);
}
