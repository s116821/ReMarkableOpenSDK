// Original owned startup/install component; no native address resolution.
#include "reader_insertion_guard_hooks.h"
#include "owned_api.h"
#include <QCoreApplication>
#include <dobby.h>
#include <cstdio>
#include <dlfcn.h>
#include <cstring>
#include <cstdlib>
#include <dirent.h>
static_assert(sizeof(void*)==4 && sizeof(QString)==12 && sizeof(QUuid)==16 && sizeof(QSizeF)==16);
#if !defined(__arm__) || defined(__thumb__) || !defined(__ARM_PCS_VFP)
#error This owned study requires ARM mode and the hard-float ARM calling convention.
#endif
namespace {
using namespace reader_insertion_hooks;
bool map(void*,std::uintptr_t at,Mapping &m) {
    FILE *f=std::fopen("/proc/self/maps","r");if(!f)return false;
    char line[512],perms[8];unsigned long begin=0,end=0;bool found=false;
    while(std::fgets(line,sizeof line,f))if(std::sscanf(line,"%lx-%lx %7s",&begin,&end,perms)==3&&at>=begin&&at<end){
        m={begin,end,perms[0]=='r',perms[1]=='w',perms[2]=='x'};found=true;break;
    }
    std::fclose(f);
    if(found)std::fprintf(stderr,"MAP %lx %lx-%lx R%dW%dX%d\n",static_cast<unsigned long>(at),static_cast<unsigned long>(m.begin),static_cast<unsigned long>(m.end),m.readable,m.writable,m.executable);
    return found;
}
bool read(void*,std::uintptr_t at,unsigned char *bytes,std::size_t n) {
    std::memcpy(bytes,reinterpret_cast<void*>(at),n);
    std::fprintf(stderr,"READ %lx %zu ",static_cast<unsigned long>(at),n);
    for(std::size_t i=0;i<n;++i)std::fprintf(stderr,"%02x",bytes[i]);
    std::fprintf(stderr,"\n");return true;
}
int hook(void*,void *target,void *replacement,void **original){return DobbyHook(target,replacement,original);}
void stopped(void*,const char *why){std::fprintf(stderr,"LOADER_FAILSTOP %s\n",why);}
}
extern "C" void owned_install_from_constructor() {
    unsigned tasks=0;
    if(DIR *d=::opendir("/proc/self/task")){while(auto *entry=::readdir(d))if(entry->d_name[0]>='0'&&entry->d_name[0]<='9')++tasks;::closedir(d);}
    std::fprintf(stderr,"DSO_INSTALL_BEGIN app=%d process_tasks=%u\n",QCoreApplication::instance()!=nullptr,tasks);
    if(QCoreApplication::instance()!=nullptr)std::_Exit(119);
    auto note=reinterpret_cast<Note>(::dlsym(RTLD_DEFAULT,"owned_loader_note"));
    void *outer=::dlsym(RTLD_DEFAULT,"owned_loader_outer");
    void *inner=::dlsym(RTLD_DEFAULT,"owned_loader_inner");
    if(!note||!outer||!inner){std::fprintf(stderr,"LOADER_FAILSTOP missing-owned-export\n");std::_Exit(119);}
    if(!note(1))std::_Exit(119);
    Install p;p.outer=outer;p.inner=inner;
    p.outerBytes={{0xf0,0x4f,0x2d,0xe9,0x00,0x40,0xa0,0xe1}};
    p.innerBytes={{0xf0,0x4f,0x2d,0xe9,0x03,0x50,0xa0,0xe1}};
    if(const char *mode=std::getenv("OWNED_LOADER_FAIL"))if(std::strcmp(mode,"bad-entry")==0)p.innerBytes[0]^=1;
    auto *core=Core::create({nullptr,hook,read,map,stopped});
    core->install(p);
    if(!note(2))std::_Exit(119);
    std::fprintf(stderr,"DSO_INSTALL_VERIFIED app=%d no_registration=1\n",QCoreApplication::instance()!=nullptr);
}
