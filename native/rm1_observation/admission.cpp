// RM1 observation-only admission. File/ELF/mapping/export checks adapted from
// the existing exact RM2 loader, without hooks or private owner/worker layouts.
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include "admission.h"
#include "profile.h"
#include "bindings.h"
#include "../resident/activation.h"
#include "../resident/loaded_artifact.h"
#include <QCoreApplication>
#include <QCryptographicHash>
#include <QSizeF>
#include <algorithm>
#include <array>
#include <atomic>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <dlfcn.h>
#include <elf.h>
#include <fcntl.h>
#include <link.h>
#include <memory>
#include <optional>
#include <string>
#include <sys/stat.h>
#include <sys/sysmacros.h>
#include <unistd.h>
#include <vector>

namespace remarkable::rm1_observation {
namespace {
std::atomic<bool> attempted{false};
#if defined(__arm__) && !defined(__thumb__) && defined(__ARM_PCS_VFP) && __BYTE_ORDER__==__ORDER_LITTLE_ENDIAN__
[[noreturn]] void die() noexcept {std::_Exit(119);}
void need(bool value) {if(!value)die();}
bool add(std::uintptr_t a,std::size_t n,std::uintptr_t &end) {
    if(a>UINTPTR_MAX-n)return false;
    end=a+n;return true;
}
using Map=detail::BindingMap;
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
    return a.st_dev==b.st_dev&&a.st_ino==b.st_ino&&a.st_size==b.st_size&&a.st_mode==b.st_mode&&a.st_uid==b.st_uid&&
        a.st_mtim.tv_sec==b.st_mtim.tv_sec&&a.st_mtim.tv_nsec==b.st_mtim.tv_nsec&&
        a.st_ctim.tv_sec==b.st_ctim.tv_sec&&a.st_ctim.tv_nsec==b.st_ctim.tv_nsec;
}
class File {
    int fd_=-1;struct stat identity_{};
    mutable std::unique_ptr<detail::BindingImage> bindings_;
public:
    Elf32_Ehdr header{};std::vector<Elf32_Phdr> ph;
    explicit File(const char *path,const char *hash,std::size_t exactSize=0) {
        fd_=::open(path,O_RDONLY|O_CLOEXEC);need(fd_>=0&&::fstat(fd_,&identity_)==0&&S_ISREG(identity_.st_mode));
        need(identity_.st_uid==::geteuid()&&!(identity_.st_mode&0022));
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
    const detail::BindingImage &bindings()const {
        if(!bindings_) {
            std::vector<unsigned char> bytes(std::size_t(identity_.st_size));
            read(0,bytes.data(),bytes.size());stable();
            bindings_=std::make_unique<detail::BindingImage>(std::move(bytes));
        }return *bindings_;
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
struct Provider {Module module;std::string path;};
Provider provider(void *symbol,const char *hash,const std::vector<Map> &all) {
    Dl_info info{};need(symbol&&::dladdr(symbol,&info)&&info.dli_fname&&info.dli_fbase);
    File file(info.dli_fname,hash);need(file.header.e_type==ET_DYN);
    Provider p{loaded(reinterpret_cast<std::uintptr_t>(info.dli_fbase)),info.dli_fname};code(file,p.module,all);return p;
}
void *version(const char *name,const char *version_) {void *p=::dlvsym(RTLD_DEFAULT,name,version_);need(p);return p;}
void exactExport(const File &file,const Provider &p,const std::vector<Map> &all,
                 void *address_,const detail::SymbolPin &pin) {
    file.bindings().definition(pin);
    const auto resolved=reinterpret_cast<std::uintptr_t>(address_);
    need(p.module.base<=UINT32_MAX&&resolved<=UINT32_MAX);
    if(pin.kind==detail::SymbolKind::Function) {
        detail::FunctionExtent extent;
        need(detail::functionExtent(std::uint32_t(p.module.base),pin,extent)&&resolved==extent.callable&&
             inLoad(p.module,extent.code,pin.size,true)&&fileRange(file,p.module,all,extent.code,pin.size));
    }else {
        std::uint32_t expected=0;const auto page=::sysconf(_SC_PAGESIZE);
        need(page>0&&std::uint64_t(page)<=UINT32_MAX&&detail::add32(std::uint32_t(p.module.base),pin.value,expected)&&resolved==expected&&
            detail::dataCovered(file.ph,std::uint32_t(p.module.base),pin,all,std::uint32_t(page),
                [&](const Map &m,std::uint32_t at,std::uint32_t offset){return file.backed(m,at,offset);}));
    }
}

void ownDynamicFlags(const Module &module,const std::vector<Map> &all) {
    bool now=false,nodelete=false,terminated=false;
    for(const auto &p:module.ph)if(p.p_type==PT_DYNAMIC) {
        need(!terminated&&p.p_memsz&&p.p_memsz%sizeof(Elf32_Dyn)==0&&p.p_memsz<=65536);
        std::uintptr_t at=0;need(add(module.base,p.p_vaddr,at)&&cover(all,at,p.p_memsz,false));
        const auto *d=reinterpret_cast<const Elf32_Dyn*>(at);
        for(std::size_t i=0;i<p.p_memsz/sizeof(Elf32_Dyn);++i) {
            if(d[i].d_tag==DT_NULL){terminated=true;break;}
            now=now||d[i].d_tag==DT_BIND_NOW||(d[i].d_tag==DT_FLAGS&&(d[i].d_un.d_val&DF_BIND_NOW))||
                (d[i].d_tag==DT_FLAGS_1&&(d[i].d_un.d_val&DF_1_NOW));
            nodelete=nodelete||(d[i].d_tag==DT_FLAGS_1&&(d[i].d_un.d_val&DF_1_NODELETE));
        }
    }
    need(terminated&&now&&nodelete);
}
void boundSlots(const File &file,const Module &module,const std::vector<Map> &all,
                const detail::SymbolPin &pin,detail::Consumer consumer,void *resolved) {
    file.bindings().sites(pin,consumer);
    const auto expected=reinterpret_cast<std::uintptr_t>(resolved);need(expected<=UINT32_MAX);
    for(std::size_t i=0;i<pin.siteCount;++i)if(pin.sites[i].consumer==consumer) {
        std::uintptr_t at=0;need(add(module.base,pin.sites[i].offset,at)&&fileRange(file,module,all,at,4));
        const auto *m=cover(all,at,4,false);need(m&&!m->x);
        std::uint32_t actual=0;std::memcpy(&actual,reinterpret_cast<const void*>(at),4);
        need(detail::slotMatches(actual,std::uint32_t(expected)));
    }
}

#endif
}

bool Admission::current() const noexcept {
    return activation_&&::getpid()==process_&&resident::processStart(process_)==start_&&activation_->directoryCurrent();
}

const Admission *admitBeforeMain(const resident::Activation &activation) noexcept {
    if(attempted.exchange(true,std::memory_order_acq_rel))return nullptr;
    const auto *profile=detail::reviewedProfile();
    // This gate is deliberately closed until real missing target evidence is
    // reviewed in source. A caller/peer/test table cannot mint an Admission.
    if(!profile||!detail::complete(*profile))return nullptr;
#if defined(__arm__) && !defined(__thumb__) && defined(__ARM_PCS_VFP) && __BYTE_ORDER__==__ORDER_LITTLE_ENDIAN__
    try {
        static_assert(sizeof(void*)==4&&sizeof(int)==4&&sizeof(qreal)==8&&sizeof(QSizeF)==16&&sizeof(QString)==12&&sizeof(QUuid)==16);
        need(::geteuid()==0&&!QCoreApplication::instance()&&activation.directoryCurrent());
        const auto &runtime=activation.config().runtime;
        need(runtime.size()==140&&runtime.left(32)==QByteArray::fromHex(profile->manifestSha256)&&
             runtime.mid(32,32)==QByteArray::fromHex(profile->executableSha256));
        need(std::strcmp(qVersion(),profile->qtVersion)==0);
        const char *now=std::getenv("LD_BIND_NOW");need(now&&std::strcmp(now,"1")==0);
        const auto all=maps();
        File main("/proc/self/exe",profile->executableSha256,profile->executableBytes);
        main.staticMainFacts();const auto mainModule=loaded(0);code(main,mainModule,all);
        const auto anchor=reinterpret_cast<const void*>(&admitBeforeMain);
        const auto adapterDigest=resident::loadedArtifactDigest(anchor);
        need(adapterDigest.size()==32&&adapterDigest==runtime.mid(64,32));
        Dl_info info{};need(::dladdr(anchor,&info)&&info.dli_fname&&info.dli_fbase);
        const auto adapterHash=adapterDigest.toHex();File adapter(info.dli_fname,adapterHash.constData());
        need(adapter.header.e_type==ET_DYN);const auto adapterModule=loaded(reinterpret_cast<std::uintptr_t>(info.dli_fbase));
        code(adapter,adapterModule,all);ownDynamicFlags(adapterModule,all);
        std::array<Provider,std::size_t(detail::ProviderRole::Count)> supplied{};
        std::array<std::unique_ptr<File>,std::size_t(detail::ProviderRole::Count)> providerFiles{};
        for(std::size_t i=0;i<profile->symbolCount;++i) {
            const auto &pin=profile->symbols[i];const auto role=std::size_t(pin.provider);
            const auto &filePin=profile->providers[role];
            // COPY provenance needs actual loaded source-scope evidence, not dladdr
            // on effective ET_EXEC storage. No such evidence exists: refuse it.
            if(pin.kind==detail::SymbolKind::Object)need(!main.bindings().hasCopy(pin.name));
            void *resolved=version(pin.name,pin.version);
            if(!providerFiles[role]) {
                supplied[role]=provider(resolved,filePin.sha256,all);
                providerFiles[role]=std::make_unique<File>(supplied[role].path.c_str(),filePin.sha256,filePin.bytes);
            }
            exactExport(*providerFiles[role],supplied[role],all,resolved,pin);
            boundSlots(adapter,adapterModule,all,pin,detail::Consumer::Adapter,resolved);
            boundSlots(main,mainModule,all,pin,detail::Consumer::Main,resolved);
        }
        for(const auto &file:providerFiles){need(bool(file));file->stable();}
        main.stable();adapter.stable();
        need(!QCoreApplication::instance()&&activation.directoryCurrent());
        const auto process=::getpid();const auto start=resident::processStart(process);need(start!=0);
        return new Admission(&activation,process,start);
    }catch(...){die();}
#else
    (void)activation;
    return nullptr;
#endif
}
}
