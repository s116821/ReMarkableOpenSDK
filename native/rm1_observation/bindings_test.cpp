// Owned mechanics only. ARM fixture files are parsed, never loaded/executed.
#include "bindings.h"
#include <cassert>
#include <cstdio>
#include <fstream>
#include <iterator>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/sysmacros.h>
#include <unistd.h>
#ifdef NDEBUG
#error Binding fixture assertions must remain enabled.
#endif
using namespace remarkable::rm1_observation::detail;
using Bytes=std::vector<unsigned char>;
template<class T>T read(const Bytes &b,std::size_t at) {
    assert(at<=b.size()&&sizeof(T)<=b.size()-at);T value{};std::memcpy(&value,b.data()+at,sizeof value);return value;
}
template<class T>void write(Bytes &b,std::size_t at,const T &value) {
    assert(at<=b.size()&&sizeof(T)<=b.size()-at);std::memcpy(b.data()+at,&value,sizeof value);
}
Bytes contents(const char *path) {
    std::ifstream f(path,std::ios::binary);assert(f);return Bytes(std::istreambuf_iterator<char>(f),{});
}
template<class F>void refuses(const F &f) {bool failed=false;try{f();}catch(const BindingError&){failed=true;}assert(failed);}
struct Fixture {
    Bytes bytes;Elf32_Ehdr header;std::vector<Elf32_Shdr> sections;std::size_t dynsym=0,versym=0;
    explicit Fixture(const char *path):bytes(contents(path)),header(read<Elf32_Ehdr>(bytes,0)) {
        for(std::size_t i=0;i<header.e_shnum;++i) {
            sections.push_back(read<Elf32_Shdr>(bytes,header.e_shoff+i*sizeof(Elf32_Shdr)));
            if(sections.back().sh_type==SHT_DYNSYM)dynsym=i;
            if(sections.back().sh_type==SHT_GNU_versym)versym=i;
        }assert(dynsym&&versym);
    }
    std::string name(const Elf32_Sym &s)const {
        const auto &str=sections[sections[dynsym].sh_link];return reinterpret_cast<const char*>(bytes.data()+str.sh_offset+s.st_name);
    }
    std::size_t index(const char *wanted)const {
        const auto &s=sections[dynsym];
        for(std::size_t i=0;i<s.sh_size/sizeof(Elf32_Sym);++i)if(name(read<Elf32_Sym>(bytes,s.sh_offset+i*sizeof(Elf32_Sym)))==wanted)return i;
        assert(false);return 0;
    }
    SymbolPin pin(const char *name_,SymbolKind kind)const {
        SymbolPin p;p.name=name_;p.version="RM49_FIXTURE_1";p.provider=ProviderRole::Core;p.kind=kind;
        p.dynsymIndex=std::uint32_t(index(name_));const auto s=read<Elf32_Sym>(bytes,sections[dynsym].sh_offset+p.dynsymIndex*sizeof(Elf32_Sym));
        p.value=s.st_value;p.size=s.st_size;p.versionIndex=read<Elf32_Half>(bytes,sections[versym].sh_offset+p.dynsymIndex*2);return p;
    }
    ArtifactSelfPin selfPin(const char *name_,SymbolKind kind)const {
        const auto p=pin(name_,kind);ArtifactSelfPin self;
        self.name=p.name;self.value=p.value;self.size=p.size;self.dynsymIndex=p.dynsymIndex;
        self.kind=kind;self.versionIndex=p.versionIndex;self.alignment=kind==SymbolKind::Object?4:1;
        self.writable=kind==SymbolKind::Object;return self;
    }
    std::vector<BindingSite> sites(const char *wanted)const {
        std::vector<BindingSite> result;const auto i=index(wanted);
        for(const auto &s:sections)if(s.sh_type==SHT_REL)for(std::size_t at=0;at<s.sh_size;at+=sizeof(Elf32_Rel)) {
            const auto r=read<Elf32_Rel>(bytes,s.sh_offset+at);
            if(ELF32_R_SYM(r.r_info)==i)result.push_back({Consumer::Adapter,r.r_offset,std::uint32_t(i),
                read<Elf32_Half>(bytes,sections[versym].sh_offset+i*2),std::uint8_t(ELF32_R_TYPE(r.r_info)),
                ELF32_R_TYPE(r.r_info)==R_ARM_ABS32?BindingImage(bytes).fileWord(r.r_offset):0});
        }assert(!result.empty());return result;
    }
};
void absSelfControls(const Fixture &provider,const Fixture &consumer) {
    BindingImage definitions(provider.bytes),bindings(consumer.bytes);
    unsigned positive=0,negative=0;
    for(const auto *name:{"arm_fn","thumb_fn","owned_data","_ZTVN10__cxxabiv117__class_type_infoE",
                         "_ZTVN10__cxxabiv120__si_class_type_infoE"}) {
        const bool function=std::strcmp(name,"arm_fn")==0||std::strcmp(name,"thumb_fn")==0;
        auto pin=provider.pin(name,function?SymbolKind::Function:SymbolKind::Object);
        const bool vtable=std::strncmp(name,"_ZTV",4)==0;
        if(vtable){pin.provider=ProviderRole::Cxx;pin.version="CXXABI_1.3";assert(pin.size==44);}
        if(!function)pin.alignment=4;
        auto sites=consumer.sites(name);pin.sites=sites.data();pin.siteCount=sites.size();
        definitions.definition(pin);bindings.sites(pin,Consumer::Adapter);
        unsigned absolute=0;
        for(const auto &site:sites)if(site.relocation==R_ARM_ABS32) {
            ++absolute;assert(site.addend==(vtable?8u:0u));
            std::uint32_t expected=0;assert(relocationWord(0x10000000,pin,site,expected));
            assert(expected==0x10000000+pin.value+site.addend);
            assert(slotMatches(expected,expected)&&!slotMatches(expected^1u,expected));
            auto bad=site;bad.addend=vtable?4:1;assert(!relocationWord(0x10000000,pin,bad,expected));++negative;
        }
        assert(absolute);++positive;
        for(int fault=0;fault<7;++fault) {
            auto broken=pin;auto changed=sites;
            if(fault==0)++changed[0].offset;
            if(fault==1)changed.push_back(changed[0]);
            if(fault==2)++changed[0].addend;
            if(fault==3)changed[0].versionIndex^=0x8000;
            if(fault==4)changed[0].relocation=R_ARM_COPY;
            if(fault==5)changed[0].dynsymIndex=0;
            if(fault==6)broken.version="wrong";
            broken.sites=changed.data();broken.siteCount=changed.size();
            refuses([&]{bindings.sites(broken,Consumer::Adapter);});++negative;
        }
    }
    for(const auto *name:{"self_arm","self_thumb","self_data"}) {
        const bool object=std::strcmp(name,"self_data")==0;
        auto pin=consumer.selfPin(name,object?SymbolKind::Object:SymbolKind::Function);
        auto sites=consumer.sites(name);pin.sites=sites.data();pin.siteCount=sites.size();
        bindings.definition(pin);bindings.sites(pin,Consumer::Adapter);assert(pin.versionIndex==1);++positive;
        bool absolute=false,other=false;
        for(const auto &site:sites) {
            absolute=absolute||site.relocation==R_ARM_ABS32;other=other||site.relocation==(object?R_ARM_GLOB_DAT:R_ARM_JUMP_SLOT);
            std::uint32_t own=0,otherImage=0;
            assert(relocationWord(0x10000000,pin,site,own)&&relocationWord(0x20000000,pin,site,otherImage));
            assert(slotMatches(own,own)&&!slotMatches(otherImage,own));
            if(!object)assert(bool(own&1)==(std::strcmp(name,"self_thumb")==0));
        }assert(absolute&&other);
        for(int fault=0;fault<11;++fault) {
            auto bad=pin;auto changed=sites;
            if(fault==0)bad.binding=STB_GLOBAL;
            if(fault==1)bad.versionIndex=2;
            if(fault==2)bad.visibility=STV_HIDDEN;
            if(fault==3)++bad.value;
            if(fault==4)++bad.size;
            if(fault==5)bad.kind=object?SymbolKind::Function:SymbolKind::Object;
            if(fault==6)changed[0].consumer=Consumer::Main;
            if(fault==7)changed[0].addend=8;
            if(fault==8)changed.push_back(changed[0]);
            if(fault==9)changed[0].dynsymIndex=0;
            if(fault==10)changed[0].versionIndex=2;
            bad.sites=changed.data();bad.siteCount=changed.size();
            refuses([&]{bindings.sites(bad,Consumer::Adapter);});++negative;
        }
        for(int fault=0;fault<5;++fault) {
            auto bytes=consumer.bytes;
            const auto at=consumer.sections[consumer.dynsym].sh_offset+pin.dynsymIndex*sizeof(Elf32_Sym);
            auto symbol=read<Elf32_Sym>(bytes,at);
            if(fault==0)symbol.st_shndx=SHN_UNDEF;
            if(fault==1)symbol.st_shndx=SHN_ABS;
            if(fault==2)symbol.st_info=ELF32_ST_INFO(STB_GLOBAL,ELF32_ST_TYPE(symbol.st_info));
            if(fault==3)symbol.st_other=STV_HIDDEN;
            if(fault==4)symbol.st_size=0;
            write(bytes,at,symbol);
            refuses([&]{BindingImage image(bytes);image.definition(pin);});++negative;
        }
    }
    const auto places=consumer.sites("arm_fn");
    const auto place=std::find_if(places.begin(),places.end(),[](const BindingSite &s){return s.relocation==R_ARM_ABS32;});
    assert(place!=places.end());assert(bindings.fileWord(place->offset)==0);
    refuses([&]{bindings.fileWord(place->offset+1);});++negative;
    refuses([&]{bindings.fileWord(UINT32_MAX-3);});++negative;
    Elf32_Phdr backing{};std::size_t spare=0,backingAt=0;
    for(std::size_t i=0;i<consumer.header.e_phnum;++i) {
        const auto at=consumer.header.e_phoff+i*sizeof(Elf32_Phdr),p=at;
        const auto ph=read<Elf32_Phdr>(consumer.bytes,p);
        if(ph.p_type==PT_GNU_STACK)spare=at;
        if(ph.p_type==PT_LOAD&&ph.p_vaddr<=place->offset&&place->offset+4<=ph.p_vaddr+ph.p_filesz){backing=ph;backingAt=at;}
    }assert(spare&&backingAt);
    for(unsigned fault=0;fault<4;++fault) {
        auto bytes=consumer.bytes;auto ph=backing;
        if(fault==0){ph.p_flags|=PF_X;write(bytes,backingAt,ph);}
        else {
            ph.p_vaddr=place->offset+(fault==2?2:0);ph.p_offset=backing.p_offset+ph.p_vaddr-backing.p_vaddr;
            ph.p_memsz=fault==2?2:4;ph.p_filesz=fault==3?0:ph.p_memsz;ph.p_align=1;
            write(bytes,spare,ph);
        }
        // Metadata remains parseable: reject the actual executable, duplicate,
        // partial or zero-fill competing four-byte place, not a malformed header.
        BindingImage mutated(bytes);refuses([&]{mutated.fileWord(place->offset);});++negative;
    }
    std::printf("PASS emitted ABS32/provider and weak artifact-self exact sites: %u positive, %u refusals; never loaded\n",positive,negative);
}
void emittedControls(char **paths) {
    Fixture provider(paths[0]),consumer(paths[1]),main(paths[2]);
    BindingImage definitions(provider.bytes),bindings(consumer.bytes),executable(main.bytes);
    absSelfControls(provider,consumer);
    unsigned positive=0,negative=0;
    for(const auto *name:{"arm_fn","thumb_fn","owned_data","owned_bss"}) {
        auto pin=provider.pin(name,std::strncmp(name,"owned_",6)==0?SymbolKind::Object:SymbolKind::Function);
        auto sites=consumer.sites(name);pin.sites=sites.data();pin.siteCount=sites.size();
        definitions.definition(pin);bindings.sites(pin,Consumer::Adapter);++positive;
        if(pin.kind==SymbolKind::Function)assert(bool(pin.value&1)==(std::strcmp(name,"thumb_fn")==0));
        for(int fault=0;fault<9;++fault) {
            auto broken=pin;
            if(fault==0)broken.name="missing";
            if(fault==1)broken.version="other-version";
            if(fault==2)broken.value^=1;
            if(fault==3)++broken.size;
            if(fault==4)broken.kind=pin.kind==SymbolKind::Object?SymbolKind::Function:SymbolKind::Object;
            if(fault==5)broken.versionIndex^=0x8000;
            if(fault==6)broken.dynsymIndex=0;
            if(fault==7)broken.binding=STB_WEAK;
            if(fault==8)broken.visibility=STV_HIDDEN;
            refuses([&]{definitions.definition(broken);});++negative;
        }
        for(int fault=0;fault<8;++fault) {
            auto brokenSites=sites;auto broken=pin;
            if(fault==0)brokenSites[0].offset+=4;
            if(fault==1)brokenSites[0].dynsymIndex=0;
            if(fault==2)brokenSites[0].versionIndex^=0x8000;
            if(fault==3)brokenSites[0].relocation=R_ARM_COPY;
            if(fault==4)brokenSites[0].consumer=Consumer::Main;
            if(fault==5)brokenSites.push_back(brokenSites[0]);
            if(fault==6)broken.version="other-version";
            if(fault==7)broken.kind=pin.kind==SymbolKind::Object?SymbolKind::Function:SymbolKind::Object;
            broken.sites=brokenSites.data();broken.siteCount=brokenSites.size();
            refuses([&]{bindings.sites(broken,Consumer::Adapter);});++negative;
        }
        auto truncated=pin;truncated.siteCount=0;refuses([&]{bindings.sites(truncated,Consumer::Adapter);});++negative;
        if(pin.kind==SymbolKind::Object) {
            assert(executable.hasCopy(name));++positive;
            refuses([&]{executable.sites(pin,Consumer::Main);});++negative;
        }else assert(!executable.hasCopy(name));
    }
    // The dynamic file bytes must describe the same range the loader maps.
    std::size_t dynamicIndex=consumer.header.e_phnum,loadIndex=dynamicIndex,spareIndex=dynamicIndex;
    Elf32_Phdr dynamic{},load{};
    for(std::size_t i=0;i<consumer.header.e_phnum;++i) {
        const auto p=read<Elf32_Phdr>(consumer.bytes,consumer.header.e_phoff+i*sizeof(Elf32_Phdr));
        if(p.p_type==PT_DYNAMIC){dynamicIndex=i;dynamic=p;}
        if(p.p_type==PT_GNU_STACK)spareIndex=i;
    }
    assert(dynamicIndex<consumer.header.e_phnum&&spareIndex<consumer.header.e_phnum);
    for(std::size_t i=0;i<consumer.header.e_phnum;++i) {
        const auto p=read<Elf32_Phdr>(consumer.bytes,consumer.header.e_phoff+i*sizeof(Elf32_Phdr));
        if(p.p_type==PT_LOAD&&p.p_vaddr<=dynamic.p_vaddr&&dynamic.p_vaddr-p.p_vaddr+dynamic.p_filesz<=p.p_filesz){loadIndex=i;load=p;}
    }assert(loadIndex<consumer.header.e_phnum);
    for(int fault=0;fault<13;++fault) {
        auto b=consumer.bytes;auto d=dynamic;auto l=load;
        if(fault==0)d.p_vaddr+=4; // Reviewer's minimal counterexample: same file bytes, different virtual view.
        if(fault==1)d.p_offset+=sizeof(Elf32_Dyn); // Skip an unneeded DT_NEEDED; required tags still parse.
        if(fault==2)d.p_vaddr=0x70000000; // No loaded file-backed range.
        if(fault==3)d.p_filesz=d.p_memsz+sizeof(Elf32_Dyn);
        if(fault==4)d.p_memsz=d.p_filesz-sizeof(Elf32_Dyn);
        if(fault==5)d.p_vaddr=UINT32_MAX-7; // File virtual end overflows.
        if(fault==6)d.p_memsz=load.p_memsz-(dynamic.p_vaddr-load.p_vaddr)+8; // File fits; memory extent escapes.
        if(fault==7)write(b,consumer.header.e_phoff+spareIndex*sizeof(Elf32_Phdr),load); // Two full backing views.
        if(fault==8) {
            auto overlap=load;overlap.p_vaddr=dynamic.p_vaddr+8;overlap.p_offset=dynamic.p_offset+8;
            overlap.p_filesz=8;overlap.p_memsz=8;
            write(b,consumer.header.e_phoff+spareIndex*sizeof(Elf32_Phdr),overlap); // Partial competing view.
        }
        if(fault==9)l.p_filesz=(dynamic.p_vaddr-load.p_vaddr)+dynamic.p_filesz-8;
        if(fault==10){d.p_vaddr=UINT32_MAX-d.p_filesz;d.p_memsz=d.p_filesz+8;} // Memory end overflows.
        if(fault==11)l.p_vaddr=UINT32_MAX-7; // Backing LOAD virtual extent overflows.
        if(fault==12) {
            auto zeroFill=load;zeroFill.p_vaddr=dynamic.p_vaddr;zeroFill.p_offset=dynamic.p_offset;
            zeroFill.p_filesz=0;zeroFill.p_memsz=dynamic.p_memsz;
            write(b,consumer.header.e_phoff+spareIndex*sizeof(Elf32_Phdr),zeroFill); // Competing zero-fill view.
        }
        write(b,consumer.header.e_phoff+dynamicIndex*sizeof(Elf32_Phdr),d);
        if(fault==9||fault==11)write(b,consumer.header.e_phoff+loadIndex*sizeof(Elf32_Phdr),l);
        refuses([&]{BindingImage image(b);});++negative;
    }
    std::puts("PASS PT_DYNAMIC file/virtual/load correspondence: 13 passive refusal controls");
    // Mutate actual compiler-produced tables, preserving all other inputs.
    for(int fault=0;fault<10;++fault) {
        auto b=consumer.bytes;
        if(fault==0){auto e=consumer.header;e.e_shoff=UINT32_MAX;write(b,0,e);}
        if(fault==1){auto s=consumer.sections[consumer.versym];s.sh_link=0;write(b,consumer.header.e_shoff+consumer.versym*sizeof(Elf32_Shdr),s);}
        if(fault==2){auto s=consumer.sections[consumer.dynsym];s.sh_size+=1;write(b,consumer.header.e_shoff+consumer.dynsym*sizeof(Elf32_Shdr),s);}
        if(fault==8){auto s=consumer.sections[consumer.dynsym];s.sh_offset+=4;write(b,consumer.header.e_shoff+consumer.dynsym*sizeof(Elf32_Shdr),s);}
        if(fault==9)for(std::size_t i=0;i<consumer.header.e_phnum;++i) {
            const auto p=read<Elf32_Phdr>(b,consumer.header.e_phoff+i*sizeof(Elf32_Phdr));
            if(p.p_type==PT_DYNAMIC)for(std::size_t at=0;at<p.p_filesz;at+=sizeof(Elf32_Dyn)) {
                auto d=read<Elf32_Dyn>(b,p.p_offset+at);
                if(d.d_tag==DT_SYMTAB){d.d_un.d_val+=4;write(b,p.p_offset+at,d);}
            }
        }
        for(std::size_t i=0;i<consumer.sections.size();++i) {
            auto s=consumer.sections[i];
            if(s.sh_type==SHT_GNU_verneed) {
                if(fault==3){auto v=read<Elf32_Verneed>(b,s.sh_offset);v.vn_aux=UINT32_MAX;write(b,s.sh_offset,v);}
                if(fault==4){auto v=read<Elf32_Verneed>(b,s.sh_offset);auto a=read<Elf32_Vernaux>(b,s.sh_offset+v.vn_aux);a.vna_name=UINT32_MAX;write(b,s.sh_offset+v.vn_aux,a);}
            }
            if(s.sh_type==SHT_REL) {
                if(fault==5){s.sh_link=0;write(b,consumer.header.e_shoff+i*sizeof(Elf32_Shdr),s);}
                if(fault==6){s.sh_type=SHT_RELA;write(b,consumer.header.e_shoff+i*sizeof(Elf32_Shdr),s);}
                if(fault==7)for(std::size_t at=0;at<s.sh_size;at+=sizeof(Elf32_Rel)) {
                    auto r=read<Elf32_Rel>(b,s.sh_offset+at);r.r_info=(UINT32_C(0xffffff)<<8)|R_ARM_GLOB_DAT;write(b,s.sh_offset+at,r);
                }
            }
        }
        auto pin=provider.pin("owned_data",SymbolKind::Object);auto sites=consumer.sites(pin.name);pin.sites=sites.data();pin.siteCount=sites.size();
        refuses([&]{BindingImage image(b);image.sites(pin,Consumer::Adapter);});++negative;
    }
    // Nonzero GLOB_DAT addend is read from the actual file-backed relocation place.
    auto pin=provider.pin("owned_data",SymbolKind::Object);auto sites=consumer.sites(pin.name);pin.sites=sites.data();pin.siteCount=sites.size();
    auto b=consumer.bytes;bool changed=false;
    for(std::size_t i=0;i<consumer.header.e_phnum;++i) {
        const auto p=read<Elf32_Phdr>(b,consumer.header.e_phoff+i*sizeof(Elf32_Phdr));
        if(p.p_type==PT_LOAD&&sites[0].offset>=p.p_vaddr&&sites[0].offset-p.p_vaddr<p.p_filesz) {
            write(b,p.p_offset+sites[0].offset-p.p_vaddr,UINT32_C(4));changed=true;
        }
    }assert(changed);refuses([&]{BindingImage image(b);image.sites(pin,Consumer::Adapter);});++negative;
    std::printf("PASS emitted ARM/Thumb/OBJECT definitions and COPY refusal: %u positive, %u negative controls; no loading\n",positive,negative);
}
void diagnosticControls(const char *path) {
    Fixture diagnostic(path);BindingImage image(diagnostic.bytes);
    for(const auto *name:{"strcmp","__aeabi_atexit","_ZN16QCoreApplication4selfE"}) {
        const bool object=std::strcmp(name,"_ZN16QCoreApplication4selfE")==0;
        auto pin=diagnostic.pin(name,object?SymbolKind::Object:SymbolKind::Function);
        pin.version=object?"Qt_6":(std::strcmp(name,"strcmp")==0?"GLIBC_2.4":"CXXABI_ARM_1.3.3");
        auto sites=diagnostic.sites(name);pin.sites=sites.data();pin.siteCount=sites.size();
        image.sites(pin,Consumer::Adapter);
    }
    auto pin=diagnostic.pin("_ZN7QObject16staticMetaObjectE",SymbolKind::Object);pin.version="Qt_6";
    auto sites=diagnostic.sites(pin.name);pin.sites=sites.data();pin.siteCount=sites.size();
    refuses([&]{image.sites(pin,Consumer::Adapter);});
    auto weak=diagnostic.pin("_ITM_registerTMCloneTable",SymbolKind::Function);weak.version="not-versioned";
    auto weakSites=diagnostic.sites(weak.name);weak.sites=weakSites.data();weak.siteCount=weakSites.size();
    refuses([&]{image.sites(weak,Consumer::Adapter);});
    std::puts("PASS actual optimized diagnostic: FUNC/versioned OBJECT sites; incomplete ABS32 definition and unreviewed CRT refuse");
}
void savedLibcControls(const char *path) {
    Fixture libc(path);BindingImage image(libc.bytes);
    for(const auto *name:{"memchr","strcmp","strlen"}) {
        auto pin=libc.pin(name,SymbolKind::Function);pin.version="GLIBC_2.4";
        assert(pin.value&1);image.definition(pin);FunctionExtent extent;
        assert(functionExtent(0x10000000,pin,extent)&&extent.callable==(0x10000000+pin.value)&&
            extent.code==(extent.callable&~1u));
        auto changed=pin;changed.versionIndex^=0x8000;refuses([&]{image.definition(changed);});
    }
    std::puts("PASS passive saved libc: exact versioned Thumb definitions; never loaded");
}
void addressControls() {
    SymbolPin pin;pin.kind=SymbolKind::Function;pin.value=0x101;pin.size=8;
    FunctionExtent extent;assert(functionExtent(0x1000,pin,extent));assert(extent.callable==0x1101&&extent.code==0x1100&&extent.end==0x1108);
    assert(slotMatches(0x1101,extent.callable));assert(!slotMatches(0x1100,extent.callable));
    pin.value=0x100;assert(functionExtent(0x1000,pin,extent)&&extent.callable==extent.code);
    pin.value=0x102;assert(!functionExtent(0x1000,pin,extent));
    pin.value=0xfffffff1;assert(!functionExtent(0x1000,pin,extent));
    pin.value=0xfffffff9;pin.size=8;assert(!functionExtent(0,pin,extent));
    pin.kind=SymbolKind::Object;assert(!functionExtent(0,pin,extent));assert(!slotMatches(0,0));
    SymbolPin vtable;vtable.name="_ZTVN10__cxxabiv117__class_type_infoE";vtable.version="CXXABI_1.3";
    vtable.provider=ProviderRole::Cxx;vtable.kind=SymbolKind::Object;vtable.value=0x100;vtable.size=44;vtable.alignment=4;
    BindingSite absolute{Consumer::Adapter,0x200,1,2,R_ARM_ABS32,8};std::uint32_t word=0;
    assert(relocationWord(0x1000,vtable,absolute,word)&&word==0x1108);
    for(int fault=0;fault<11;++fault) {
        auto broken=vtable;auto site=absolute;std::uint32_t base=0x1000;
        if(fault==0)broken.name="another-vtable";
        if(fault==1)broken.version="another-version";
        if(fault==2)broken.provider=ProviderRole::Core;
        if(fault==3)broken.size=43;
        if(fault==4)broken.kind=SymbolKind::Function;
        if(fault==5)site.addend=4;
        if(fault==6)site.addend=40; // Within44 but not the observed addend.
        if(fault==7)site.addend=UINT32_MAX;
        if(fault==8)broken.value++;
        if(fault==9)base=UINT32_MAX-0x80;
        if(fault==10)broken.alignment=3;
        assert(!relocationWord(base,broken,site,word));
    }
    absolute.addend=0;vtable.size=3;assert(!relocationWord(0x1000,vtable,absolute,word));
    vtable.size=UINT32_MAX;assert(!relocationWord(0x1000,vtable,absolute,word));
    pin.kind=SymbolKind::Function;pin.value=0x101;pin.size=8;
    assert(relocationWord(0x1000,pin,absolute,word)&&word==0x1101&&!slotMatches(word&~1u,word));
    absolute.addend=1;assert(!relocationWord(0x1000,pin,absolute,word));
    absolute.addend=0;pin.value=0xfffffff9;assert(!relocationWord(0,pin,absolute,word));
    std::puts("PASS tagged pointer/code-extent, alignment/overflow and resolved-word controls");
}
void mappedControls() {
    const long pageLong=::sysconf(_SC_PAGESIZE);assert(pageLong>0);const auto page=std::uint32_t(pageLong);
    char name[]="/tmp/rm1-binding-map-XXXXXX";const int fd=::mkstemp(name);assert(fd>=0);assert(::unlink(name)==0);
    assert(::ftruncate(fd,page*2)==0);struct stat st{};assert(::fstat(fd,&st)==0);
    int flags=MAP_PRIVATE|MAP_ANONYMOUS;
#ifdef MAP_32BIT
    flags|=MAP_32BIT;
#endif
    void *reservation=::mmap(nullptr,page*4,PROT_NONE,flags,-1,0);assert(reservation!=MAP_FAILED);
    const auto base64=reinterpret_cast<std::uintptr_t>(reservation);
    if(base64>UINT32_MAX-page*4) {assert(::munmap(reservation,page*4)==0);assert(::close(fd)==0);std::puts("SKIP 32-bit host mapping controls: no low address allocation");return;}
    assert(::mmap(reservation,page*2,PROT_READ|PROT_WRITE,MAP_SHARED|MAP_FIXED,fd,0)==reservation);
    auto *tail=static_cast<char*>(reservation)+page*2;
    assert(::mmap(tail,page*2,PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANONYMOUS|MAP_FIXED,-1,0)==tail);
    const auto base=std::uint32_t(base64);Elf32_Phdr ph{};ph.p_type=PT_LOAD;ph.p_flags=PF_R|PF_W;ph.p_filesz=page+page/2;ph.p_memsz=page*4;
    std::vector<Elf32_Phdr> segments{ph};
    std::vector<BindingMap> maps{{base,base+page*2,0,unsigned(major(st.st_dev)),unsigned(minor(st.st_dev)),static_cast<unsigned long long>(st.st_ino),true,true,false},
        {base+page*2,base+page*4,0,0,0,0,true,true,false}};
    const auto backed=[&](const BindingMap &m,std::uint32_t at,std::uint32_t offset){return m.inode==static_cast<unsigned long long>(st.st_ino)&&m.major==major(st.st_dev)&&m.minor==minor(st.st_dev)&&m.offset+at-m.begin==offset;};
    SymbolPin p;p.kind=SymbolKind::Object;p.value=page+128;p.size=page*2;p.alignment=4;p.writable=true;
    assert(dataCovered(segments,base,p,maps,page,backed));
    auto *slot=static_cast<std::uint32_t*>(reservation);std::memcpy(slot,&p.value,4);
    assert(slotMatches(*slot,p.value));*slot=base;assert(!slotMatches(*slot,p.value));
    tail[0]=37;assert(dataCovered(segments,base,p,maps,page,backed)); // Mutable contents are not identity.
    SymbolPin vtable;vtable.name="_ZTVN10__cxxabiv117__class_type_infoE";vtable.version="CXXABI_1.3";
    vtable.provider=ProviderRole::Cxx;vtable.kind=SymbolKind::Object;vtable.value=128;vtable.size=44;vtable.alignment=4;vtable.writable=true;
    BindingSite absolute{Consumer::Adapter,4,1,2,R_ARM_ABS32,8};std::uint32_t expected=0;
    assert(dataCovered(segments,base,vtable,maps,page,backed)&&relocationWord(base,vtable,absolute,expected));
    *slot=expected;assert(slotMatches(*slot,expected));*slot=expected^1u;assert(!slotMatches(*slot,expected));
    *slot=expected+page;assert(!slotMatches(*slot,expected)); // Another image/base is not self/provider correspondence.
    auto escaping=vtable;escaping.value=page*4-8;
    assert(!dataCovered(segments,base,escaping,maps,page,backed)); // Interior target cannot replace full object extent.
    for(int fault=0;fault<10;++fault) {
        auto changed=maps;auto pin=p;auto loads=segments;
        if(fault==0)changed[0].inode++;
        if(fault==1)changed[0].offset=page;
        if(fault==2)changed[1].inode=st.st_ino;
        if(fault==3)changed[1].begin+=4;
        if(fault==4)changed[1].x=true;
        if(fault==5)changed[0].w=false;
        if(fault==6)pin.value++;
        if(fault==7)pin.size=page*4;
        if(fault==8)loads.push_back(ph);
        if(fault==9)loads[0].p_filesz=page*5;
        assert(!dataCovered(loads,base,pin,changed,page,backed));
    }
    // Read-only/RELRO data still uses the verified file backing, not object bytes.
    p.value=128;p.size=16;p.writable=false;maps[0].w=false;
    assert(::mprotect(reservation,page*2,PROT_READ)==0);assert(dataCovered(segments,base,p,maps,page,backed));
    assert(::munmap(reservation,page*4)==0);assert(::close(fd)==0);
    std::puts("PASS real owned low mappings: file/last-page/anonymous zero-fill and RELRO data; 10 mismatch refusals");
}
int main(int argc,char **argv) {
    assert(reviewedProfile()==nullptr);addressControls();mappedControls();
    if(argc>=4&&argc<=6)emittedControls(argv+1);else {assert(argc==1);std::puts("NOTE emitted ARM-file controls require the three explicitly generated fixture paths");}
    if(argc>=5)diagnosticControls(argv[4]);
    if(argc==6)savedLibcControls(argv[5]);
    assert(reviewedProfile()==nullptr);
}
