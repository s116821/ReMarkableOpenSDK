// Original exact-build interoperability profile. Compile-only until separately
// selected by the owner of a complete native packet. No automatic constructor.
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include "reader_insertion_guard_native_profile.h"
#include <QCoreApplication>
#include <QCryptographicHash>
#include <QPointer>
#include <QThread>
#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <dirent.h>
#include <dlfcn.h>
#include <elf.h>
#include <fcntl.h>
#include <link.h>
#include <optional>
#include <string>
#include <sys/stat.h>
#include <sys/sysmacros.h>
#include <unistd.h>
#include <vector>
#include <dobby.h>

namespace reader_insertion_native_profile {
namespace {
constexpr std::uintptr_t OuterAddress=0x841728,InnerAddress=0x7b4bb4;
constexpr std::uintptr_t SearchAddress=0x7b1f60,EqualAddress=0x144d9c;
constexpr std::uintptr_t FindAddress=0x46d4a4,CompareSlot=0x14fe5a4;
constexpr std::uintptr_t WorkerMeta=0xe624d8;
constexpr std::uintptr_t ReleaseAddress=0x9d620,SingleThreadFlag=0x1516fac;
constexpr std::uintptr_t WorkerControlVptr=0x1218ebc,DocumentControlVptr=0xe829c4;
constexpr const char MainHash[]="071d85beef3ef2d4cc0e11002140b27b82a2cc04a2ed740a5669f591069b77df";
constexpr const char LibcHash[]="8d741a0db9727c97faebfd17e7f8409183c382241ff3d4c9b55a52e98d8dd832";
constexpr const char QtHash[]="43b0e210d64e59b534490d78c4c82cc1d2958999b0969aa0f77e11a082704e5d";
constexpr const char EqualSymbol[]="_ZN9QtPrivate12equalStringsE11QStringViewS0_";
constexpr const char CompareSymbol[]="_ZN9QtPrivate14compareStringsE11QStringViewS0_N2Qt15CaseSensitivityE";
[[noreturn]] void die() noexcept {std::_Exit(119);}
void need(bool value) {if(!value)die();}
bool add(std::uintptr_t a,std::size_t n,std::uintptr_t &end) {
    if(a>UINTPTR_MAX-n)return false;
    end=a+n;return true;
}
struct Map {
    std::uintptr_t begin=0,end=0,offset=0;
    unsigned major=0,minor=0;unsigned long long inode=0;
    bool r=false,w=false,x=false;
};
std::optional<std::vector<Map>> inspectMaps() {
    FILE *file=std::fopen("/proc/self/maps","r");if(!file)return {};
    struct Guard {FILE *file;~Guard(){if(file)std::fclose(file);}} guard{file};
    std::vector<Map> result;char line[4096];std::size_t bytes=0;
    while(std::fgets(line,sizeof line,file)) {
        bytes+=std::strlen(line);if(bytes>1024*1024||result.size()>=2048||!std::strchr(line,'\n'))return {};
        unsigned long b=0,e=0,o=0;char p[8];Map m;
        if(std::sscanf(line,"%lx-%lx %7s %lx %x:%x %llu",&b,&e,p,&o,&m.major,&m.minor,&m.inode)!=7)return {};
        m.begin=b;m.end=e;m.offset=o;m.r=p[0]=='r';m.w=p[1]=='w';m.x=p[2]=='x';
        if(m.begin>=m.end)return {};
        result.push_back(m);
    }
    if(std::ferror(file))return {};
    const int closed=std::fclose(file);guard.file=nullptr;
    if(closed)return {};
    return result;
}
std::vector<Map> maps(){auto value=inspectMaps();need(bool(value));return std::move(*value);}
const Map *cover(const std::vector<Map> &all,std::uintptr_t at,std::size_t n,bool code) {
    std::uintptr_t end=0;if(!at||!n||!add(at,n,end))return nullptr;
    for(const auto &m:all)if(m.begin<=at&&end<=m.end&&m.r&&(!code||(m.x&&!m.w)))return &m;
    return nullptr;
}
bool sameStat(const struct stat &a,const struct stat &b) {
    return a.st_dev==b.st_dev&&a.st_ino==b.st_ino&&a.st_size==b.st_size&&a.st_mode==b.st_mode&&
        a.st_mtim.tv_sec==b.st_mtim.tv_sec&&a.st_mtim.tv_nsec==b.st_mtim.tv_nsec&&
        a.st_ctim.tv_sec==b.st_ctim.tv_sec&&a.st_ctim.tv_nsec==b.st_ctim.tv_nsec;
}
class File {
    int fd_=-1;struct stat identity_{};
public:
    Elf32_Ehdr header{};std::vector<Elf32_Phdr> ph;
    explicit File(const char *path,const char *hash,std::size_t exactSize=0) {
        fd_=::open(path,O_RDONLY|O_CLOEXEC);need(fd_>=0&&::fstat(fd_,&identity_)==0&&S_ISREG(identity_.st_mode));
        need(identity_.st_size>0&&identity_.st_size<=64*1024*1024&&(!exactSize||std::uint64_t(identity_.st_size)==exactSize));
        QCryptographicHash digest(QCryptographicHash::Sha256);std::array<char,65536> block{};
        for(std::size_t at=0;at<std::size_t(identity_.st_size);) {
            const auto count=std::min(block.size(),std::size_t(identity_.st_size)-at);
            read(at,block.data(),count);digest.addData(QByteArrayView(block.data(),qsizetype(count)));at+=count;
        }
        need(digest.result().toHex()==hash);stable();read(0,&header,sizeof header);
        need(std::memcmp(header.e_ident,ELFMAG,SELFMAG)==0&&header.e_ident[EI_CLASS]==ELFCLASS32&&
             header.e_ident[EI_DATA]==ELFDATA2LSB&&header.e_ident[EI_VERSION]==EV_CURRENT&&
             header.e_machine==EM_ARM&&header.e_version==EV_CURRENT&&header.e_flags==0x05000400&&
             header.e_ehsize==sizeof(Elf32_Ehdr)&&header.e_phentsize==sizeof(Elf32_Phdr)&&header.e_phnum>0&&header.e_phnum<=64);
        ph.resize(header.e_phnum);read(header.e_phoff,ph.data(),ph.size()*sizeof(Elf32_Phdr));
        for(const auto &p:ph)if(p.p_type==PT_LOAD) {
            std::uintptr_t end=0;need(p.p_filesz<=p.p_memsz&&add(p.p_vaddr,p.p_memsz,end));range(p.p_offset,p.p_filesz);
        }
    }
    ~File(){if(fd_>=0)::close(fd_);}
    File(const File&)=delete;File &operator=(const File&)=delete;
    void range(std::size_t at,std::size_t n)const {need(at<=std::size_t(identity_.st_size)&&n<=std::size_t(identity_.st_size)-at);}
    void read(std::size_t at,void *out,std::size_t n)const {
        range(at,n);auto *bytes=static_cast<unsigned char*>(out);
        while(n) {const auto got=::pread(fd_,bytes,n,off_t(at));need(got>0);at+=std::size_t(got);bytes+=got;n-=std::size_t(got);}
    }
    void stable()const {struct stat now{};need(::fstat(fd_,&now)==0&&sameStat(identity_,now));}
    bool backed(const Map &m,std::uintptr_t at,std::size_t offset)const {
        std::uintptr_t fileOffset=0;
        return m.inode==static_cast<unsigned long long>(identity_.st_ino)&&m.major==major(identity_.st_dev)&&
            m.minor==minor(identity_.st_dev)&&add(m.offset,at-m.begin,fileOffset)&&fileOffset==offset;
    }
    std::vector<Elf32_Shdr> sections()const {
        need(header.e_shentsize==sizeof(Elf32_Shdr)&&header.e_shnum>0&&header.e_shnum<=256);
        std::vector<Elf32_Shdr> s(header.e_shnum);read(header.e_shoff,s.data(),s.size()*sizeof(Elf32_Shdr));return s;
    }
    struct Symbols {std::vector<Elf32_Sym> values;std::vector<char> strings;};
    Symbols symbols()const {
        const auto sections_=sections();for(const auto &s:sections_)if(s.sh_type==SHT_DYNSYM) {
            need(s.sh_entsize==sizeof(Elf32_Sym)&&s.sh_size%sizeof(Elf32_Sym)==0&&s.sh_size<=1024*1024&&s.sh_link<sections_.size());
            const auto &str=sections_[s.sh_link];need(str.sh_type==SHT_STRTAB&&str.sh_size>0&&str.sh_size<=4*1024*1024);
            Symbols result;result.values.resize(s.sh_size/sizeof(Elf32_Sym));result.strings.resize(str.sh_size);
            read(s.sh_offset,result.values.data(),s.sh_size);read(str.sh_offset,result.strings.data(),str.sh_size);return result;
        }die();
    }
    static const char *name(const Symbols &s,std::size_t i) {
        need(i<s.values.size());const auto offset=s.values[i].st_name;need(offset<s.strings.size());
        const char *name_=s.strings.data()+offset;need(std::memchr(name_,0,s.strings.size()-offset));return name_;
    }
    Elf32_Sym function(const char *wanted)const {
        const auto s=symbols();Elf32_Sym result{};bool found=false;
        for(std::size_t i=0;i<s.values.size();++i)if(std::strcmp(name(s,i),wanted)==0&&s.values[i].st_shndx!=SHN_UNDEF) {
            need(ELF32_ST_TYPE(s.values[i].st_info)==STT_FUNC);
            if(found)need(result.st_value==s.values[i].st_value&&result.st_size==s.values[i].st_size);
            result=s.values[i];found=true;
        }need(found&&result.st_size>0);return result;
    }
    std::uintptr_t jumpSlot(const char *wanted)const {
        const auto syms=symbols();const auto s=sections();std::uintptr_t result=0;
        for(const auto &section:s)if(section.sh_type==SHT_REL) {
            need(section.sh_entsize==sizeof(Elf32_Rel)&&section.sh_size%sizeof(Elf32_Rel)==0&&section.sh_size<=4*1024*1024);
            std::vector<Elf32_Rel> rel(section.sh_size/sizeof(Elf32_Rel));read(section.sh_offset,rel.data(),section.sh_size);
            for(const auto &r:rel)if(ELF32_R_TYPE(r.r_info)==R_ARM_JUMP_SLOT&&std::strcmp(name(syms,ELF32_R_SYM(r.r_info)),wanted)==0) {
                need(!result);result=r.r_offset;
            }
        }need(result&&result%4==0);return result;
    }
    void staticMainFacts()const {
        need(header.e_type==ET_EXEC);const auto s=sections();const auto syms=symbols();
        for(const auto &v:syms.values)need(ELF32_ST_TYPE(v.st_info)!=STT_GNU_IFUNC);
        for(const auto &section:s) {
            need(section.sh_type!=SHT_PREINIT_ARRAY);
            if(section.sh_type==SHT_REL) {
                need(section.sh_size%sizeof(Elf32_Rel)==0&&section.sh_size<=4*1024*1024);
                std::vector<Elf32_Rel> rel(section.sh_size/sizeof(Elf32_Rel));read(section.sh_offset,rel.data(),section.sh_size);
                for(const auto &r:rel)need(ELF32_R_TYPE(r.r_info)!=R_ARM_IRELATIVE);
            }
        }
        for(const auto &p:ph)if(p.p_type==PT_DYNAMIC) {
            need(p.p_filesz%sizeof(Elf32_Dyn)==0&&p.p_filesz<=65536);std::vector<Elf32_Dyn> d(p.p_filesz/sizeof(Elf32_Dyn));read(p.p_offset,d.data(),p.p_filesz);
            for(const auto &v:d)need(v.d_tag!=DT_PREINIT_ARRAY&&v.d_tag!=DT_PREINIT_ARRAYSZ&&v.d_tag!=DT_TEXTREL);
        }
    }
};
struct Module {std::uintptr_t base=0;std::vector<Elf32_Phdr> ph;};
Module loaded(std::uintptr_t base) {
    struct Query {std::uintptr_t base;Module result;bool found=false;} query{base,{},false};
    const auto callback=+[](dl_phdr_info *info,std::size_t,void *opaque)->int {
        auto &q=*static_cast<Query*>(opaque);if(std::uintptr_t(info->dlpi_addr)!=q.base)return 0;
        if(info->dlpi_phnum==0||info->dlpi_phnum>64)die();
        q.result.base=q.base;q.result.ph.assign(info->dlpi_phdr,info->dlpi_phdr+info->dlpi_phnum);q.found=true;return 1;
    };
    ::dl_iterate_phdr(callback,&query);need(query.found);return query.result;
}
void code(const File &file,const Module &module,const std::vector<Map> &all) {
    need(file.ph.size()==module.ph.size());
    for(std::size_t i=0;i<file.ph.size();++i)need(std::memcmp(&file.ph[i],&module.ph[i],sizeof(Elf32_Phdr))==0);
    std::array<unsigned char,4096> buffer{};
    for(const auto &p:file.ph)if(p.p_type==PT_LOAD&&(p.p_flags&PF_X)) {
        need((p.p_flags&PF_R)&&!(p.p_flags&PF_W));std::uintptr_t begin=0;need(add(module.base,p.p_vaddr,begin));
        for(std::size_t done=0;done<p.p_filesz;) {
            std::uintptr_t at=0;need(add(begin,done,at));const auto *m=cover(all,at,1,true);need(m);
            const auto n=std::min({buffer.size(),std::size_t(p.p_filesz)-done,std::size_t(m->end-at)});
            need(file.backed(*m,at,std::size_t(p.p_offset)+done));file.read(std::size_t(p.p_offset)+done,buffer.data(),n);
            need(std::memcmp(reinterpret_cast<const void*>(at),buffer.data(),n)==0);done+=n;
        }
    }file.stable();
}
bool inLoad(const Module &m,std::uintptr_t address,std::size_t n,bool executable) {
    std::uintptr_t end=0;if(!add(address,n,end))return false;
    for(const auto &p:m.ph)if(p.p_type==PT_LOAD&&(p.p_flags&PF_R)&&(!executable||(p.p_flags&PF_X))) {
        std::uintptr_t start=0,last=0;if(add(m.base,p.p_vaddr,start)&&add(start,executable?p.p_filesz:p.p_memsz,last)&&start<=address&&end<=last)return true;
    }return false;
}
bool fileRange(const File &file,const Module &m,const std::vector<Map> &all,std::uintptr_t address,std::size_t n) {
    std::uintptr_t end=0;if(!add(address,n,end))return false;
    const auto *mapping=cover(all,address,n,false);if(!mapping)return false;
    for(const auto &p:m.ph)if(p.p_type==PT_LOAD&&(p.p_flags&PF_R)) {
        std::uintptr_t start=0,last=0,offset=0;
        if(add(m.base,p.p_vaddr,start)&&add(start,p.p_filesz,last)&&start<=address&&end<=last&&
           add(p.p_offset,address-start,offset)&&file.backed(*mapping,address,offset))return true;
    }return false;
}
template<std::size_t N> std::array<unsigned char,N> pinBytes(const File &file,const Module &m,
        const std::vector<Map> &all,std::uintptr_t address) {
    need(fileRange(file,m,all,address,N));
    std::array<unsigned char,N> bytes{};
    for(const auto &p:m.ph)if(p.p_type==PT_LOAD) {
        std::uintptr_t start=0,last=0,offset=0;
        if(add(m.base,p.p_vaddr,start)&&add(start,p.p_filesz,last)&&start<=address&&address<=last&&N<=last-address&&
           add(p.p_offset,address-start,offset)) {
            file.read(offset,bytes.data(),N);
            need(std::memcmp(bytes.data(),reinterpret_cast<const void*>(address),N)==0);return bytes;
        }
    }
    die();
}
struct Provider {Module module;std::string path;};
Provider provider(void *symbol,const char *hash,const std::vector<Map> &all) {
    Dl_info info{};need(symbol&&::dladdr(symbol,&info)&&info.dli_fname&&info.dli_fbase);
    File file(info.dli_fname,hash);need(file.header.e_type==ET_DYN);
    Provider p{loaded(reinterpret_cast<std::uintptr_t>(info.dli_fbase)),info.dli_fname};code(file,p.module,all);return p;
}
void *version(const char *name,const char *version_) {void *p=::dlvsym(RTLD_DEFAULT,name,version_);need(p);return p;}
void exactExport(const File &file,const Provider &p,void *address_,const char *name,std::uint32_t expected,std::size_t size) {
    const auto symbol=file.function(name);need(symbol.st_value==expected&&(!size||symbol.st_size==size));
    std::uintptr_t at=0;need(add(p.module.base,symbol.st_value,at)&&at==reinterpret_cast<std::uintptr_t>(address_)&&at%4==0&&inLoad(p.module,at,symbol.st_size,true));
}
bool bindNowHere() {
    Dl_info info{};if(!::dladdr(reinterpret_cast<void*>(&installBeforeMain),&info))return false;
    const auto m=loaded(reinterpret_cast<std::uintptr_t>(info.dli_fbase));bool yes=false;
    for(const auto &p:m.ph)if(p.p_type==PT_DYNAMIC) {
        need(p.p_memsz%sizeof(Elf32_Dyn)==0&&p.p_memsz<=65536);std::uintptr_t at=0;need(add(m.base,p.p_vaddr,at));
        const auto all=maps();need(cover(all,at,p.p_memsz,false));
        const auto *d=reinterpret_cast<const Elf32_Dyn*>(at);
        for(std::size_t i=0;i<p.p_memsz/sizeof(Elf32_Dyn)&&d[i].d_tag!=DT_NULL;++i)
            yes=yes||d[i].d_tag==DT_BIND_NOW||(d[i].d_tag==DT_FLAGS&&(d[i].d_un.d_val&DF_BIND_NOW))||(d[i].d_tag==DT_FLAGS_1&&(d[i].d_un.d_val&DF_1_NOW));
    }return yes;
}
unsigned tasks() {DIR *d=::opendir("/proc/self/task");need(d);unsigned n=0;while(auto *e=::readdir(d))if(e->d_name[0]>='0'&&e->d_name[0]<='9')need(++n<=4096);need(::closedir(d)==0);return n;}
}

struct State {
    reader_insertion_hooks::Core *core=nullptr;
    using Search=std::uint32_t(*)(std::uint32_t,const QString&);
    using Equal=std::uint32_t(*)(const QString&,const QString&);
    // Observed AAPCS32 carriers, not a recovered original private declaration.
    using Find=std::uint32_t(*)(std::uint32_t,const QString&);
    Search search=nullptr;Equal equal=nullptr;
    Find find=nullptr;
    bool conditionSelected=false; // Immutable before resident publication.
    int(*tryLock)(pthread_mutex_t*)=nullptr;int(*unlock)(pthread_mutex_t*)=nullptr;
    Module main;
    Map metaMapping;
    std::atomic<bool> installed{false},partsUsed{false};
    AttributionParts attribution; // GUI-only, one allocation set until retirement.
    std::atomic<std::uint32_t> wordAddress{0},searchReceiver{0};
    std::atomic<std::uint32_t> treeReceiver{0};
    unsigned taskCount=0;
    pid_t ownerProcess=0;
    std::array<unsigned char,0xcc> releaseCode{};
    std::array<unsigned char,24> workerControlTable{},documentControlTable{};
    std::array<Map,4> ownerMappings{};
};
namespace {
std::atomic<State*> resident{nullptr};
std::atomic<bool> attempted{false};
bool readWords(std::uint32_t address,reader_insertion_predicate::Words &out) {
    auto *s=resident.load(std::memory_order_acquire);
    if(!s||!s->installed.load(std::memory_order_acquire)||!address||address!=s->wordAddress.load(std::memory_order_acquire))return false;
    std::uint32_t words[2];std::memcpy(words,reinterpret_cast<const void*>(std::uintptr_t(address)),8);out={words[0],words[1]};return true;
}
std::uint32_t search(std::uint32_t receiver,const QString &value) {
    auto *s=resident.load(std::memory_order_acquire);
    if(!s||!s->installed.load(std::memory_order_acquire)||!receiver||receiver!=s->searchReceiver.load(std::memory_order_acquire))return 0;
    return s->search(receiver,value);
}
bool equal(const QString &a,const QString &b) {
    auto *s=resident.load(std::memory_order_acquire);return s&&s->installed.load(std::memory_order_acquire)&&s->wordAddress.load(std::memory_order_acquire)&&s->equal(a,b)!=0;
}
bool conditionWords(std::uint32_t address,reader_insertion_condition::Words &out) {
    auto *s=resident.load(std::memory_order_acquire);
    if(!s||!s->conditionSelected||!s->installed.load(std::memory_order_acquire)||
       !s->treeReceiver.load(std::memory_order_acquire)||!address||address!=s->wordAddress.load(std::memory_order_acquire))return false;
    std::uint32_t words[2];std::memcpy(words,reinterpret_cast<const void*>(std::uintptr_t(address)),8);
    out={words[0],words[1]};return true;
}
std::uint32_t conditionSearch(std::uint32_t receiver,const QString &value) {
    auto *s=resident.load(std::memory_order_acquire);
    if(!s||!s->conditionSelected||!s->installed.load(std::memory_order_acquire)||
       !s->treeReceiver.load(std::memory_order_acquire)||!receiver||receiver!=s->searchReceiver.load(std::memory_order_acquire))return 0;
    return s->search(receiver,value);
}
bool conditionEqual(const QString &a,const QString &b) {
    auto *s=resident.load(std::memory_order_acquire);
    return s&&s->conditionSelected&&s->installed.load(std::memory_order_acquire)&&
        s->treeReceiver.load(std::memory_order_acquire)&&s->equal(a,b)!=0;
}
bool conditionTree(std::uint32_t receiver,reader_insertion_condition::Tree &out) {
    auto *s=resident.load(std::memory_order_acquire);
    if(!s||!s->conditionSelected||!s->installed.load(std::memory_order_acquire)||
       !receiver||receiver!=s->treeReceiver.load(std::memory_order_acquire))return false;
    // The owned result carrier is not the native noncontiguous header layout.
    const auto address=std::uintptr_t(receiver);
    std::uint32_t root=0,count=0;
    std::memcpy(&root,reinterpret_cast<const void*>(address+8),4);
    std::memcpy(&count,reinterpret_cast<const void*>(address+20),4);
    out={root,count};return true;
}
std::uint32_t conditionFind(std::uint32_t receiver,const QString &value) {
    auto *s=resident.load(std::memory_order_acquire);
    if(!s||!s->conditionSelected||!s->installed.load(std::memory_order_acquire)||!s->find||
       !receiver||receiver!=s->treeReceiver.load(std::memory_order_acquire))return 0;
    return s->find(receiver,value);
}
bool installRead(void*,std::uintptr_t at,unsigned char *out,std::size_t n) {std::memcpy(out,reinterpret_cast<const void*>(at),n);return true;}
bool installMap(void*,std::uintptr_t at,reader_insertion_hooks::Mapping &out) {
    const auto all=maps();const auto *m=cover(all,at,1,false);if(!m)return false;
    out={m->begin,m->end,m->r,m->w,m->x};return true;
}
int installHook(void*,void *at,void *replacement,void **original) {return DobbyHook(at,replacement,original);}
}
Facilities::Facilities(State *state):state_(state),core(state->core),passiveCalls{readWords,search,equal},
    baseAdmission{true,false,false,false,false,false},
    baseConditionAdmission{state->conditionSelected,false,false,false,false,false},conditionQualified(state->conditionSelected),
    outerReturnPC(0x6ee780),innerReturnPC(0x841988),
    observedTaskCount(state->taskCount),applicationAbsent(true) {}

Facilities *installBeforeMain(bool wantCondition) {
    try {
#if !defined(__arm__) || defined(__thumb__) || !defined(__ARM_PCS_VFP) || __BYTE_ORDER__!=__ORDER_LITTLE_ENDIAN__
        die();
#endif
        static_assert(sizeof(void*)==4&&sizeof(int)==4&&sizeof(qreal)==8&&sizeof(QSizeF)==16&&sizeof(QString)==12&&sizeof(QUuid)==16);
        static_assert(sizeof(pthread_mutex_t)==24&&alignof(pthread_mutex_t)==4&&offsetof(pthread_mutex_t,__data.__kind)==12);
        constexpr pthread_mutex_t plain=PTHREAD_MUTEX_INITIALIZER;static_assert(plain.__data.__kind==0);
        need(!attempted.exchange(true,std::memory_order_acq_rel)&&!QCoreApplication::instance());
        const char *now=std::getenv("LD_BIND_NOW");need(now&&std::strcmp(now,"1")==0&&bindNowHere());
        auto *s=new State;s->taskCount=tasks();const auto all=maps();
        File main("/proc/self/exe",MainHash,22046100);main.staticMainFacts();s->main=loaded(0);code(main,s->main,all);
        for(const auto &range:std::array<std::pair<std::uintptr_t,std::size_t>,6>{{{OuterAddress,2504},{InnerAddress,3396},{SearchAddress,480},{EqualAddress,80},{0x6ee750,48},{0x841824,548}}})
            need(inLoad(s->main,range.first,range.second,true)&&cover(all,range.first,range.second,true));
        need(fileRange(main,s->main,all,WorkerMeta,sizeof(QMetaObject)));
        s->metaMapping=*cover(all,WorkerMeta,sizeof(QMetaObject),false);
        void *try_=version("pthread_mutex_trylock","GLIBC_2.34"),*compat=version("pthread_mutex_trylock","GLIBC_2.4");
        void *unlock_=version("pthread_mutex_unlock","GLIBC_2.4"),*exit_=version("_Exit","GLIBC_2.4");need(try_==compat);
        const auto libc=provider(try_,LibcHash,all);File libcFile(libc.path.c_str(),LibcHash);
        exactExport(libcFile,libc,try_,"pthread_mutex_trylock",0x7bfd4,1864);
        exactExport(libcFile,libc,unlock_,"pthread_mutex_unlock",0x7cd8c,8);
        exactExport(libcFile,libc,exit_,"_Exit",0xb02dc,44);
        need(reinterpret_cast<void*>(&std::_Exit)==exit_);
        need(version("__libc_single_threaded","GLIBC_2.32")==reinterpret_cast<void*>(SingleThreadFlag));
        s->releaseCode=pinBytes<0xcc>(main,s->main,all,ReleaseAddress);
        s->workerControlTable=pinBytes<24>(main,s->main,all,WorkerControlVptr-8);
        s->documentControlTable=pinBytes<24>(main,s->main,all,DocumentControlVptr-8);
        const std::array<std::uintptr_t,4> ownerAddresses{{ReleaseAddress,WorkerControlVptr-8,DocumentControlVptr-8,SingleThreadFlag}};
        const std::array<std::size_t,4> ownerLengths{{0xcc,24,24,1}};
        for(std::size_t n=0;n<ownerAddresses.size();++n) {
            const auto *mapping=cover(all,ownerAddresses[n],ownerLengths[n],n==0);
            need(mapping&&inLoad(s->main,ownerAddresses[n],ownerLengths[n],n==0));s->ownerMappings[n]=*mapping;
        }
        s->ownerProcess=::getpid();
        void *qtEqual=version(EqualSymbol,"Qt_6");const auto qt=provider(qtEqual,QtHash,all);File qtFile(qt.path.c_str(),QtHash);
        const auto eqSymbol=qtFile.function(EqualSymbol);exactExport(qtFile,qt,qtEqual,EqualSymbol,eqSymbol.st_value,eqSymbol.st_size);
        const auto slot=main.jumpSlot(EqualSymbol);need(fileRange(main,s->main,all,slot,4));
        std::uint32_t bound=0;std::memcpy(&bound,reinterpret_cast<const void*>(slot),4);need(bound==reinterpret_cast<std::uintptr_t>(qtEqual));
        if(wantCondition) {
            need(inLoad(s->main,FindAddress,208,true)&&cover(all,FindAddress,208,true));
            void *qtCompare=version(CompareSymbol,"Qt_6");
            // Reuse the already hash/code-qualified Qt module; do not call it.
            exactExport(qtFile,qt,qtCompare,CompareSymbol,0x1b3748,148);
            const auto compareSlot=main.jumpSlot(CompareSymbol);
            need(compareSlot==CompareSlot&&fileRange(main,s->main,all,compareSlot,4));
            std::uint32_t compareBound=0;std::memcpy(&compareBound,reinterpret_cast<const void*>(compareSlot),4);
            need(compareBound==reinterpret_cast<std::uintptr_t>(qtCompare));
            s->find=reinterpret_cast<State::Find>(FindAddress);s->conditionSelected=true;
        }
        s->tryLock=reinterpret_cast<int(*)(pthread_mutex_t*)>(try_);s->unlock=reinterpret_cast<int(*)(pthread_mutex_t*)>(unlock_);
        s->search=reinterpret_cast<State::Search>(SearchAddress);s->equal=reinterpret_cast<State::Equal>(EqualAddress);
        main.stable();libcFile.stable();qtFile.stable();need(!QCoreApplication::instance());
        resident.store(s,std::memory_order_release);
        s->core=reader_insertion_hooks::Core::create({nullptr,installHook,installRead,installMap,nullptr});
        reader_insertion_hooks::Install plan;plan.outer=reinterpret_cast<void*>(OuterAddress);plan.inner=reinterpret_cast<void*>(InnerAddress);
        plan.outerBytes={{0xf0,0x4f,0x2d,0xe9,0x00,0x40,0xa0,0xe1}};plan.innerBytes={{0xf0,0x4f,0x2d,0xe9,0x03,0x50,0xa0,0xe1}};
        s->core->install(plan);s->installed.store(true,std::memory_order_release);return new Facilities(s);
    }catch(...){die();}
}

AttributionParts Facilities::attributionParts(QObject *worker,bool wantPostRejectProbe,bool wantCondition)const {
    try {
        if((wantCondition&&wantPostRejectProbe)||(wantCondition&&!state_->conditionSelected))return {};
        auto *app=QCoreApplication::instance();
        if(!app||QThread::currentThread()!=app->thread()||!worker||!state_->installed.load(std::memory_order_acquire)||state_->partsUsed.exchange(true,std::memory_order_acq_rel))return {};
        QPointer<QObject> guarded(worker);
        const auto all=maps();const auto *meta=cover(all,WorkerMeta,sizeof(QMetaObject),false);
        if(!inLoad(state_->main,WorkerMeta,sizeof(QMetaObject),false)||!meta)return {};
        std::uintptr_t priorOffset=0,currentOffset=0;
        if(meta->inode!=state_->metaMapping.inode||meta->major!=state_->metaMapping.major||meta->minor!=state_->metaMapping.minor||
           !add(state_->metaMapping.offset,WorkerMeta-state_->metaMapping.begin,priorOffset)||
           !add(meta->offset,WorkerMeta-meta->begin,currentOffset)||priorOffset!=currentOffset)return {};
        if(worker->metaObject()!=reinterpret_cast<const QMetaObject*>(WorkerMeta)||!guarded||worker->thread()!=app->thread())return {};
        const auto w=reinterpret_cast<std::uintptr_t>(worker);std::uintptr_t map=0,mutex=0,kind=0,words=0,end=0,first=0,receiver=0;
        if(!w||w%4||!add(w,200,map)||!add(w,592,mutex)||!add(mutex,12,kind)||!add(w,688,words)||!add(words,8,end)||
           !add(w,284,first)||!add(w,280,receiver)||!cover(all,mutex,24,false)||!cover(all,words,8,false)||!cover(all,first,8,false))return {};
        int actualKind=-1;std::memcpy(&actualKind,reinterpret_cast<const void*>(kind),4);if(actualKind!=0||!guarded)return {};
        reader_insertion_job_snapshot::Config c;c.mutex=reinterpret_cast<pthread_mutex_t*>(mutex);c.words=reinterpret_cast<const std::uint32_t*>(words);
        c.mutexStorage={mutex,mutex+24};c.wordStorage={words,end};c.tryLock=state_->tryLock;c.unlock=state_->unlock;
        auto *context=reader_insertion_job_snapshot::Snapshot::create(c);if(!context)return {};
        state_->attribution.context=context; // Retain even on later refusal.
        std::uintptr_t tree=0;
        reader_insertion_condition::Calls conditionCalls;
        if(wantCondition) {
            std::uintptr_t treeEnd=0;
            if(!add(w,296,tree)||!add(tree,24,treeEnd))return {};
            const auto *header=cover(all,tree,24,false);if(!header||header->x||!guarded)return {};
            // Only storage admission here; values are read later under the
            // caller's original final map mutex, not on this GUI stack.
            conditionCalls={conditionWords,conditionSearch,conditionEqual,conditionTree,conditionFind};
        }
        reader_insertion_mutex_probe::Probe *postRejectProbe=nullptr;
        if(wantPostRejectProbe) {
            std::uintptr_t finalMutex=0,finalKind=0,finalEnd=0;
            if(!add(w,176,finalMutex)||!add(finalMutex,12,finalKind)||!add(finalMutex,24,finalEnd))return {};
            const auto *storage=cover(all,finalMutex,24,false);if(!storage||!storage->w||storage->x)return {};
            int actualFinalKind=-1;std::memcpy(&actualFinalKind,reinterpret_cast<const void*>(finalKind),4);
            if(actualFinalKind!=0||!guarded)return {};
            reader_insertion_mutex_probe::Config probe;
            probe.mutex=reinterpret_cast<pthread_mutex_t*>(finalMutex);probe.storage={finalMutex,finalEnd};
            probe.tryLock=state_->tryLock;probe.unlock=state_->unlock;
            postRejectProbe=reader_insertion_mutex_probe::Probe::create(probe);if(!postRejectProbe)return {};
            state_->attribution.postRejectProbe=postRejectProbe;
        }
        state_->wordAddress.store(std::uint32_t(first),std::memory_order_release);state_->searchReceiver.store(std::uint32_t(receiver),std::memory_order_release);
        if(wantCondition)state_->treeReceiver.store(std::uint32_t(tree),std::memory_order_release);
        state_->attribution={reinterpret_cast<void*>(map),context,reader_insertion_job_snapshot::Snapshot::attribute,postRejectProbe,conditionCalls};
        return state_->attribution;
    }catch(...){return {};}
}

bool Facilities::retireAttribution(const AttributionParts &parts)const noexcept {
    auto *app=QCoreApplication::instance();
    if(!app||QCoreApplication::closingDown()||QThread::currentThread()!=app->thread()||
       !state_->installed.load(std::memory_order_acquire)||!state_->partsUsed.load(std::memory_order_acquire)||
       !parts||parts.context!=state_->attribution.context||parts.map!=state_->attribution.map||
       parts.read64!=state_->attribution.read64||parts.postRejectProbe!=state_->attribution.postRejectProbe)return false;
    // No native pointer is dereferenced and no native mutex/owner is destroyed.
    // Clear dispatch addresses before destroying adapter-only one-shot readers.
    state_->wordAddress.store(0,std::memory_order_release);
    state_->searchReceiver.store(0,std::memory_order_release);
    state_->treeReceiver.store(0,std::memory_order_release);
    const auto retired=state_->attribution;state_->attribution={};
    delete retired.context;delete retired.postRejectProbe;
    state_->partsUsed.store(false,std::memory_order_release);
    return true;
}

OwnerReleaseResult Facilities::tryRelinquishNonLast(NativeOwnerKind kind,std::uintptr_t raw,std::uintptr_t control)const noexcept {
    using Result=OwnerReleaseResult;
    try {
        auto *app=QCoreApplication::instance();
        if(!app||QCoreApplication::closingDown()||QThread::currentThread()!=app->thread()||
           !state_->installed.load(std::memory_order_acquire)||::getpid()!=state_->ownerProcess||
           !raw||!control||raw%4||control%4||raw>UINT32_MAX||control>UINT32_MAX-20||
           (kind!=NativeOwnerKind::Document&&kind!=NativeOwnerKind::Worker))return Result::RefusedIdentity;
        const auto observed=inspectMaps();if(!observed)return Result::RefusedIdentity;
        const auto &all=*observed;
        const std::array<std::uintptr_t,4> addresses{{ReleaseAddress,WorkerControlVptr-8,DocumentControlVptr-8,SingleThreadFlag}};
        const std::array<std::size_t,4> lengths{{0xcc,24,24,1}};
        for(std::size_t n=0;n<addresses.size();++n) {
            const auto *current=cover(all,addresses[n],lengths[n],n==0);
            const auto &prior=state_->ownerMappings[n];std::uintptr_t priorOffset=0,currentOffset=0;
            if(!current||prior.inode!=current->inode||prior.major!=current->major||prior.minor!=current->minor||
               prior.r!=current->r||prior.w!=current->w||prior.x!=current->x||
               !add(prior.offset,addresses[n]-prior.begin,priorOffset)||
               !add(current->offset,addresses[n]-current->begin,currentOffset)||priorOffset!=currentOffset)return Result::RefusedIdentity;
        }
        if(std::memcmp(state_->releaseCode.data(),reinterpret_cast<const void*>(ReleaseAddress),state_->releaseCode.size())||
           std::memcmp(state_->workerControlTable.data(),reinterpret_cast<const void*>(WorkerControlVptr-8),24)||
           std::memcmp(state_->documentControlTable.data(),reinterpret_cast<const void*>(DocumentControlVptr-8),24)||
           __atomic_load_n(reinterpret_cast<const unsigned char*>(SingleThreadFlag),__ATOMIC_ACQUIRE)!=0)return Result::RefusedIdentity;
        const auto *storage=cover(all,control,16,false);
        if(!storage||!storage->w||storage->x)return Result::RefusedIdentity;
        std::uint32_t vptr=0;std::memcpy(&vptr,reinterpret_cast<const void*>(control),4);
        if(kind==NativeOwnerKind::Document) {
            if(vptr!=DocumentControlVptr||raw!=control+16||!cover(all,raw,4,false))return Result::RefusedIdentity;
        }else {
            std::uint32_t contained=0;std::memcpy(&contained,reinterpret_cast<const void*>(control+12),4);
            if(vptr!=WorkerControlVptr||contained!=raw||!cover(all,raw,4,false))return Result::RefusedIdentity;
        }
        if(__atomic_load_n(reinterpret_cast<const std::int32_t*>(control+8),__ATOMIC_ACQUIRE)<=0)return Result::RefusedIdentity;
        // This is the final native access on success. The caller must clear its
        // local owner words, without invoking the old ordinary release helper.
        return remarkable::rm2_3_28_0_172::owner_release::decrementPinnedStrong(reinterpret_cast<std::int32_t*>(control+4));
    }catch(...){return Result::RefusedIdentity;}
}
}
