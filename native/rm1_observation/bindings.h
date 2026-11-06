#pragma once
// Private RM1 metadata/range mechanics. Never issues or selects Admission.
#include "profile.h"
#include <algorithm>
#include <cstring>
#include <elf.h>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace remarkable::rm1_observation::detail {
struct BindingError final : std::runtime_error { BindingError():std::runtime_error("RM1 binding refused"){} };
inline void bindingNeed(bool ok) {if(!ok)throw BindingError();}
inline bool add32(std::uint32_t a,std::uint32_t b,std::uint32_t &out) {
    if(b>UINT32_MAX-a)return false;
    out=a+b;return true;
}
struct FunctionExtent {std::uint32_t callable=0,code=0,end=0;};
template<class Pin>inline bool functionExtent(std::uint32_t base,const Pin &pin,FunctionExtent &out) {
    if(pin.kind!=SymbolKind::Function||!pin.value||!pin.size||base%4)return false;
    if(!add32(base,pin.value,out.callable)||!add32(base,pin.value&~1u,out.code)||
       !add32(out.code,pin.size,out.end))return false;
    return out.code%((pin.value&1)?2:4)==0;
}
inline bool slotMatches(std::uint32_t actual,std::uint32_t expected) {return actual&&actual==expected;}
template<class Pin>inline bool relocationWord(std::uint32_t base,const Pin &pin,
                                            const BindingSite &site,std::uint32_t &out) {
    if(!bindingForm(pin,site))return false;
    if(pin.kind==SymbolKind::Function) {
        FunctionExtent extent;if(!functionExtent(base,pin,extent))return false;
        out=extent.callable;return true; // Only A0, including exact callable state.
    }
    if(pin.kind!=SymbolKind::Object||!pin.alignment||(pin.alignment&(pin.alignment-1)))return false;
    std::uint32_t start=0,end=0;
    if(!add32(base,pin.value,start)||!add32(start,pin.size,end)||start%pin.alignment)return false;
    if(site.relocation==2&&(pin.size<4||site.addend>pin.size-4))return false;
    if(!add32(start,site.addend,out))return false;
    return site.relocation!=2||(out%4==0&&out%pin.alignment==0);
}

// Bounded passive parser for the exact ELF32 ARM files already hash-verified by File.
// It checks the table/entry actually referenced, not a name-only readelf match.
class BindingImage {
    std::vector<unsigned char> bytes_;
    Elf32_Ehdr header_{};
    std::vector<Elf32_Shdr> sections_;
    std::size_t dynsym_=0,versym_=0;
    std::vector<Elf32_Phdr> loads_;
    template<class T>T get(std::size_t at)const {
        bindingNeed(at<=bytes_.size()&&sizeof(T)<=bytes_.size()-at);
        T value{};std::memcpy(&value,bytes_.data()+at,sizeof value);return value;
    }
    void sectionRange(const Elf32_Shdr &s,std::size_t at,std::size_t n)const {
        bindingNeed(at<=s.sh_size&&n<=s.sh_size-at&&s.sh_offset<=bytes_.size()&&
                    s.sh_size<=bytes_.size()-s.sh_offset);
    }
    template<class T>T entry(const Elf32_Shdr &s,std::size_t at)const {
        sectionRange(s,at,sizeof(T));return get<T>(std::size_t(s.sh_offset)+at);
    }
    std::string string(std::size_t section,std::size_t at)const {
        bindingNeed(section<sections_.size());const auto &s=sections_[section];
        bindingNeed(s.sh_type==SHT_STRTAB&&s.sh_size<=4*1024*1024);sectionRange(s,at,1);
        const char *p=reinterpret_cast<const char*>(bytes_.data()+s.sh_offset+at);
        const auto *end=static_cast<const char*>(std::memchr(p,0,s.sh_size-at));bindingNeed(end);
        return std::string(p,end);
    }
    Elf32_Sym symbol(std::size_t i)const {
        const auto &s=sections_[dynsym_];bindingNeed(i<s.sh_size/sizeof(Elf32_Sym));
        return entry<Elf32_Sym>(s,i*sizeof(Elf32_Sym));
    }
    std::uint16_t versionIndex(std::size_t i)const {
        bindingNeed(i<sections_[dynsym_].sh_size/sizeof(Elf32_Sym));
        return entry<Elf32_Half>(sections_[versym_],i*sizeof(Elf32_Half));
    }
    std::string versionName(std::uint16_t raw,bool definition)const {
        const auto wanted=raw&0x7fff;bindingNeed(wanted>=2);std::string found;unsigned matches=0;
        for(const auto &s:sections_)if(s.sh_type==(definition?SHT_GNU_verdef:SHT_GNU_verneed)) {
            bindingNeed(s.sh_link==sections_[dynsym_].sh_link&&s.sh_info&&s.sh_info<=4096);
            std::size_t at=0;
            for(std::size_t record=0;record<s.sh_info;++record) {
                std::uint32_t next=0;
                if(definition) {
                    const auto d=entry<Elf32_Verdef>(s,at);
                    bindingNeed(d.vd_version==VER_DEF_CURRENT&&d.vd_cnt&&d.vd_cnt<=256&&d.vd_aux>=sizeof d);
                    sectionRange(s,at,d.vd_aux);std::size_t aux=at+d.vd_aux;
                    for(unsigned n=0;n<d.vd_cnt;++n) {
                        const auto a=entry<Elf32_Verdaux>(s,aux);const auto name=string(s.sh_link,a.vda_name);
                        if(n==0&&(d.vd_ndx&0x7fff)==wanted){found=name;++matches;}
                        if(n+1<d.vd_cnt){bindingNeed(a.vda_next>=sizeof a);sectionRange(s,aux,a.vda_next);aux+=a.vda_next;}
                        else bindingNeed(a.vda_next==0);
                    }next=d.vd_next;
                }else {
                    const auto d=entry<Elf32_Verneed>(s,at);
                    bindingNeed(d.vn_version==VER_NEED_CURRENT&&d.vn_cnt&&d.vn_cnt<=256&&d.vn_aux>=sizeof d);
                    bindingNeed(!string(s.sh_link,d.vn_file).empty());sectionRange(s,at,d.vn_aux);std::size_t aux=at+d.vn_aux;
                    for(unsigned n=0;n<d.vn_cnt;++n) {
                        const auto a=entry<Elf32_Vernaux>(s,aux);const auto name=string(s.sh_link,a.vna_name);
                        if((a.vna_other&0x7fff)==wanted){found=name;++matches;}
                        if(n+1<d.vn_cnt){bindingNeed(a.vna_next>=sizeof a);sectionRange(s,aux,a.vna_next);aux+=a.vna_next;}
                        else bindingNeed(a.vna_next==0);
                    }next=d.vn_next;
                }
                if(record+1<s.sh_info){bindingNeed(next>=(definition?sizeof(Elf32_Verdef):sizeof(Elf32_Verneed)));sectionRange(s,at,next);at+=next;}
                else bindingNeed(next==0);
            }
        }bindingNeed(matches==1&&!found.empty());return found;
    }
    void allocatedTable(const Elf32_Shdr &s)const {
        bindingNeed((s.sh_flags&SHF_ALLOC)&&s.sh_addr&&s.sh_type!=SHT_NOBITS);sectionRange(s,0,s.sh_size);
        unsigned matches=0;
        for(const auto &p:loads_)if(s.sh_addr>=p.p_vaddr&&s.sh_addr-p.p_vaddr<=p.p_filesz&&
            s.sh_size<=p.p_filesz-(s.sh_addr-p.p_vaddr)&&s.sh_offset>=p.p_offset&&
            s.sh_offset-p.p_offset==s.sh_addr-p.p_vaddr)++matches;
        bindingNeed(matches==1);
    }
    void dynamicTables()const {
        std::vector<Elf32_Dyn> entries;unsigned dynamicCount=0;
        for(std::size_t i=0;i<header_.e_phnum;++i) {
            const auto p=get<Elf32_Phdr>(std::size_t(header_.e_phoff)+i*sizeof(Elf32_Phdr));
            if(p.p_type!=PT_DYNAMIC)continue;
            bindingNeed(++dynamicCount==1&&p.p_filesz&&p.p_filesz<=65536&&p.p_filesz%sizeof(Elf32_Dyn)==0&&
                p.p_filesz<=p.p_memsz&&p.p_offset<=bytes_.size()&&p.p_filesz<=bytes_.size()-p.p_offset);
            // Join the file parser to the same virtual segment the loader sees.
            // A second full or partial memory view is ambiguous, even when
            // one candidate has the expected offset translation.
            std::uint32_t fileEnd=0,memoryEnd=0;
            bindingNeed(add32(p.p_vaddr,p.p_filesz,fileEnd)&&add32(p.p_vaddr,p.p_memsz,memoryEnd));
            unsigned backing=0;
            for(const auto &load:loads_) {
                std::uint32_t loadFileEnd=0,loadMemoryEnd=0,offset=0;
                bindingNeed(add32(load.p_vaddr,load.p_filesz,loadFileEnd)&&
                    add32(load.p_vaddr,load.p_memsz,loadMemoryEnd));
                if(memoryEnd<=load.p_vaddr||p.p_vaddr>=loadMemoryEnd)continue;
                bindingNeed((load.p_flags&PF_R)&&load.p_vaddr<=p.p_vaddr&&fileEnd<=loadFileEnd&&
                    memoryEnd<=loadMemoryEnd&&add32(load.p_offset,p.p_vaddr-load.p_vaddr,offset)&&offset==p.p_offset);
                ++backing;
            }
            bindingNeed(backing==1);
            bool terminated=false;
            for(std::size_t j=0;j<p.p_filesz;j+=sizeof(Elf32_Dyn)) {
                const auto d=get<Elf32_Dyn>(std::size_t(p.p_offset)+j);
                if(d.d_tag==DT_NULL){terminated=true;break;}entries.push_back(d);
            }bindingNeed(terminated);
        }bindingNeed(dynamicCount==1);
        const auto tag=[&](Elf32_Sword wanted,bool required=false) {
            unsigned count=0;std::uint32_t value=0;
            for(const auto &d:entries)if(d.d_tag==wanted){++count;value=d.d_un.d_val;}
            bindingNeed(count<=1&&(!required||count==1));return value;
        };
        const auto &symbols=sections_[dynsym_];const auto &strings=sections_[symbols.sh_link];
        allocatedTable(symbols);allocatedTable(strings);allocatedTable(sections_[versym_]);
        bindingNeed(tag(DT_SYMTAB,true)==symbols.sh_addr&&tag(DT_SYMENT,true)==sizeof(Elf32_Sym)&&
            tag(DT_STRTAB,true)==strings.sh_addr&&tag(DT_STRSZ,true)==strings.sh_size&&
            tag(DT_VERSYM,true)==sections_[versym_].sh_addr&&!tag(DT_RELA)&&!tag(DT_RELASZ));
        const auto rel=tag(DT_REL),relSize=tag(DT_RELSZ),plt=tag(DT_JMPREL),pltSize=tag(DT_PLTRELSZ);
        bindingNeed(bool(rel)==bool(relSize)&&bool(plt)==bool(pltSize)&&
            (!rel||tag(DT_RELENT,true)==sizeof(Elf32_Rel))&&(!plt||tag(DT_PLTREL,true)==DT_REL));
        unsigned relMatches=0,pltMatches=0,defMatches=0,needMatches=0;
        for(const auto &section:sections_) {
            if(section.sh_type==SHT_REL&&section.sh_size) {
                allocatedTable(section);
                if(rel&&section.sh_addr==rel&&section.sh_size==relSize)++relMatches;
                else if(plt&&section.sh_addr==plt&&section.sh_size==pltSize)++pltMatches;
                else bindingNeed(false);
            }
            if(section.sh_type==SHT_GNU_verdef) {
                allocatedTable(section);
                bindingNeed(tag(DT_VERDEF,true)==section.sh_addr&&tag(DT_VERDEFNUM,true)==section.sh_info);++defMatches;
            }
            if(section.sh_type==SHT_GNU_verneed) {
                allocatedTable(section);
                bindingNeed(tag(DT_VERNEED,true)==section.sh_addr&&tag(DT_VERNEEDNUM,true)==section.sh_info);++needMatches;
            }
        }
        bindingNeed(relMatches==unsigned(bool(rel))&&pltMatches==unsigned(bool(plt))&&
            defMatches==unsigned(bool(tag(DT_VERDEF)))&&needMatches==unsigned(bool(tag(DT_VERNEED))));
    }
    template<class Visit>void relocations(const Visit &visit)const {
        for(const auto &s:sections_) {
            bindingNeed(s.sh_type!=SHT_RELA||!s.sh_size);
            if(s.sh_type!=SHT_REL)continue;
            bindingNeed(s.sh_link==dynsym_&&s.sh_entsize==sizeof(Elf32_Rel)&&s.sh_size%sizeof(Elf32_Rel)==0&&s.sh_size<=4*1024*1024);
            for(std::size_t at=0;at<s.sh_size;at+=sizeof(Elf32_Rel)) {
                const auto r=entry<Elf32_Rel>(s,at);const auto i=ELF32_R_SYM(r.r_info);
                const auto sym=symbol(i);visit(r,i,string(sections_[dynsym_].sh_link,sym.st_name));
            }
        }
    }
public:
    explicit BindingImage(std::vector<unsigned char> bytes):bytes_(std::move(bytes)) {
        bindingNeed(bytes_.size()<=64*1024*1024);header_=get<Elf32_Ehdr>(0);
        bindingNeed(std::memcmp(header_.e_ident,ELFMAG,SELFMAG)==0&&header_.e_ident[EI_CLASS]==ELFCLASS32&&
          header_.e_ident[EI_DATA]==ELFDATA2LSB&&header_.e_machine==EM_ARM&&header_.e_flags==0x05000400&&
          (header_.e_type==ET_DYN||header_.e_type==ET_EXEC)&&header_.e_shentsize==sizeof(Elf32_Shdr)&&
          header_.e_shnum>0&&header_.e_shnum<=256&&header_.e_phentsize==sizeof(Elf32_Phdr)&&header_.e_phnum&&header_.e_phnum<=64);
        bindingNeed(header_.e_ident[EI_VERSION]==EV_CURRENT&&header_.e_version==EV_CURRENT&&header_.e_ehsize==sizeof(Elf32_Ehdr)&&
            header_.e_shoff<=bytes_.size()&&std::size_t(header_.e_shnum)*sizeof(Elf32_Shdr)<=bytes_.size()-header_.e_shoff&&
            header_.e_phoff<=bytes_.size()&&std::size_t(header_.e_phnum)*sizeof(Elf32_Phdr)<=bytes_.size()-header_.e_phoff);
        for(std::size_t i=0;i<header_.e_shnum;++i) {
            const auto s=get<Elf32_Shdr>(std::size_t(header_.e_shoff)+i*sizeof(Elf32_Shdr));
            sections_.push_back(s);
            if(s.sh_type==SHT_DYNSYM){bindingNeed(!dynsym_&&i);dynsym_=i;}
            if(s.sh_type==SHT_GNU_versym){bindingNeed(!versym_&&i);versym_=i;}
        }
        bindingNeed(dynsym_&&versym_);const auto &d=sections_[dynsym_];const auto &v=sections_[versym_];
        bindingNeed(d.sh_link<sections_.size()&&d.sh_entsize==sizeof(Elf32_Sym)&&d.sh_size&&
            d.sh_size%sizeof(Elf32_Sym)==0&&d.sh_size<=1024*1024&&v.sh_link==dynsym_&&
            v.sh_entsize==sizeof(Elf32_Half)&&v.sh_size==d.sh_size/sizeof(Elf32_Sym)*sizeof(Elf32_Half));
        sectionRange(d,0,d.sh_size);sectionRange(v,0,v.sh_size);
        for(std::size_t i=0;i<header_.e_phnum;++i) {
            const auto p=get<Elf32_Phdr>(std::size_t(header_.e_phoff)+i*sizeof(Elf32_Phdr));
            if(p.p_type==PT_LOAD){bindingNeed(p.p_filesz<=p.p_memsz&&p.p_offset<=bytes_.size()&&p.p_filesz<=bytes_.size()-p.p_offset);loads_.push_back(p);}
        }
        dynamicTables();
    }
    void definition(const SymbolPin &pin)const {
        bindingNeed(header_.e_type==ET_DYN);const auto s=symbol(pin.dynsymIndex);
        bindingNeed(string(sections_[dynsym_].sh_link,s.st_name)==pin.name&&s.st_shndx>SHN_UNDEF&&s.st_shndx<sections_.size()&&
            ELF32_ST_TYPE(s.st_info)==std::uint8_t(pin.kind)&&ELF32_ST_BIND(s.st_info)==pin.binding&&pin.binding==STB_GLOBAL&&
            s.st_other==pin.visibility&&pin.visibility==STV_DEFAULT&&s.st_value==pin.value&&s.st_size==pin.size&&pin.size&&
            versionIndex(pin.dynsymIndex)==pin.versionIndex&&versionName(pin.versionIndex,true)==pin.version);
        unsigned matches=0;
        for(std::size_t i=0;i<sections_[dynsym_].sh_size/sizeof(Elf32_Sym);++i)
            if(string(sections_[dynsym_].sh_link,symbol(i).st_name)==pin.name&&versionIndex(i)==pin.versionIndex)++matches;
        bindingNeed(matches==1);
    }
    void definition(const ArtifactSelfPin &pin)const {
        bindingNeed(header_.e_type==ET_DYN);const auto s=symbol(pin.dynsymIndex);
        bindingNeed(pin.name&&string(sections_[dynsym_].sh_link,s.st_name)==pin.name&&
            s.st_shndx>SHN_UNDEF&&s.st_shndx<sections_.size()&&
            ELF32_ST_TYPE(s.st_info)==std::uint8_t(pin.kind)&&ELF32_ST_BIND(s.st_info)==STB_WEAK&&
            pin.binding==STB_WEAK&&s.st_other==STV_DEFAULT&&pin.visibility==STV_DEFAULT&&
            s.st_value==pin.value&&s.st_size==pin.size&&pin.size&&pin.versionIndex==1&&versionIndex(pin.dynsymIndex)==1);
        unsigned matches=0;
        for(std::size_t i=0;i<sections_[dynsym_].sh_size/sizeof(Elf32_Sym);++i)
            if(string(sections_[dynsym_].sh_link,symbol(i).st_name)==pin.name&&versionIndex(i)==1)++matches;
        bindingNeed(matches==1);
    }
    bool hasCopy(const char *name)const {
        bool copy=false;relocations([&](const Elf32_Rel &r,std::size_t,const std::string &n){
            if(n==name&&ELF32_R_TYPE(r.r_info)==R_ARM_COPY)copy=true;
        });return copy;
    }
    std::uint32_t fileWord(std::uint32_t value)const {
        bindingNeed(value&&value%4==0);std::uint32_t end=0;bindingNeed(add32(value,4,end));
        unsigned matches=0;std::uint32_t result=0;
        for(const auto &p:loads_) {
            std::uint32_t memoryEnd=0,fileEnd=0;
            bindingNeed(add32(p.p_vaddr,p.p_memsz,memoryEnd)&&add32(p.p_vaddr,p.p_filesz,fileEnd));
            if(value>=memoryEnd||p.p_vaddr>=end)continue;
            bindingNeed((p.p_flags&PF_R)&&!(p.p_flags&PF_X)&&p.p_vaddr<=value&&end<=fileEnd);
            result=get<Elf32_Word>(std::size_t(p.p_offset)+value-p.p_vaddr);++matches;
        }bindingNeed(matches==1);return result;
    }
    void sites(const SymbolPin &pin,Consumer consumer)const {
        bindingNeed(pin.sites&&pin.siteCount);
        std::vector<bool> seen(pin.siteCount,false);
        relocations([&](const Elf32_Rel &r,std::size_t i,const std::string &name){
            if(name!=pin.name)return;
            const auto sym=symbol(i);const auto type=ELF32_R_TYPE(r.r_info);
            bindingNeed((type==R_ARM_ABS32||type==R_ARM_GLOB_DAT||type==R_ARM_JUMP_SLOT)&&
                r.r_offset&&r.r_offset%4==0&&
                sym.st_shndx==SHN_UNDEF&&ELF32_ST_BIND(sym.st_info)==STB_GLOBAL&&sym.st_other==STV_DEFAULT&&
                (ELF32_ST_TYPE(sym.st_info)==STT_NOTYPE||ELF32_ST_TYPE(sym.st_info)==std::uint8_t(pin.kind)));
            std::size_t match=pin.siteCount;
            for(std::size_t j=0;j<pin.siteCount;++j)if(pin.sites[j].consumer==consumer&&pin.sites[j].offset==r.r_offset) {
                bindingNeed(match==pin.siteCount);match=j;
            }
            bindingNeed(match<pin.siteCount&&!seen[match]);const auto &expected=pin.sites[match];
            bindingNeed(bindingForm(pin,expected)&&expected.relocation==type&&expected.dynsymIndex==i&&expected.versionIndex==versionIndex(i)&&
                versionName(expected.versionIndex,false)==pin.version);
            // REL JUMP_SLOT has implicit addend zero even though its initial word is a PLT stub.
            if(type!=R_ARM_JUMP_SLOT)bindingNeed(fileWord(r.r_offset)==expected.addend);
            else (void)fileWord(r.r_offset); // PLT initializer is not an ABS32 addend.
            seen[match]=true;
        });
        for(std::size_t i=0;i<pin.siteCount;++i)if(pin.sites[i].consumer==consumer)bindingNeed(seen[i]);
    }
    void sites(const ArtifactSelfPin &pin,Consumer consumer)const {
        bindingNeed(consumer==Consumer::Adapter&&pin.sites&&pin.siteCount);definition(pin);
        std::vector<bool> seen(pin.siteCount,false);
        relocations([&](const Elf32_Rel &r,std::size_t i,const std::string &name){
            if(name!=pin.name)return;
            std::size_t match=pin.siteCount;
            for(std::size_t j=0;j<pin.siteCount;++j)if(pin.sites[j].consumer==consumer&&pin.sites[j].offset==r.r_offset) {
                bindingNeed(match==pin.siteCount);match=j;
            }
            bindingNeed(match<pin.siteCount&&!seen[match]);const auto &expected=pin.sites[match];
            bindingNeed(bindingForm(pin,expected)&&expected.dynsymIndex==i&&i==pin.dynsymIndex&&
                expected.versionIndex==1&&versionIndex(i)==1&&expected.relocation==ELF32_R_TYPE(r.r_info));
            if(expected.relocation!=R_ARM_JUMP_SLOT)bindingNeed(fileWord(r.r_offset)==0);
            else (void)fileWord(r.r_offset);
            seen[match]=true;
        });
        for(std::size_t i=0;i<pin.siteCount;++i)bindingNeed(seen[i]);
    }
};

struct BindingMap {
    std::uintptr_t begin=0,end=0,offset=0;
    unsigned major=0,minor=0;unsigned long long inode=0;
    bool r=false,w=false,x=false;
};
// File-backed bytes, the file-backed last-page tail and anonymous BSS are distinct.
// No object contents are read or compared. The caller has verified loaded phdrs.
template<class Pin,class Backed>bool dataCovered(const std::vector<Elf32_Phdr> &ph,std::uint32_t base,
    const Pin &pin,const std::vector<BindingMap> &maps,std::uint32_t page,const Backed &backed) {
    std::uint32_t begin=0,end=0;if(pin.kind!=SymbolKind::Object||!pin.size||!pin.alignment||
       (pin.alignment&(pin.alignment-1))||!page||(page&(page-1))||!add32(base,pin.value,begin)||
       begin%pin.alignment||!add32(begin,pin.size,end))return false;
    const Elf32_Phdr *segment=nullptr;std::uint32_t start=0,fileEnd=0;
    for(const auto &p:ph)if(p.p_type==PT_LOAD&&(p.p_flags&PF_R)&&!(p.p_flags&PF_X)) {
        std::uint32_t s=0,e=0,f=0;
        if(p.p_filesz>p.p_memsz||!add32(base,p.p_vaddr,s)||!add32(s,p.p_memsz,e)||!add32(s,p.p_filesz,f))return false;
        if(s<=begin&&end<=e){if(segment)return false;segment=&p;start=s;fileEnd=f;}
    }
    if(!segment||(pin.writable&&!(segment->p_flags&PF_W))||segment->p_vaddr%page!=segment->p_offset%page)return false;
    std::uint32_t lastPage=0;if(!add32(fileEnd,(page-fileEnd%page)%page,lastPage))return false;
    for(std::uint32_t at=begin;at<end;) {
        const BindingMap *mapping=nullptr;
        for(const auto &m:maps)if(m.begin<=at&&at<m.end){if(mapping)return false;mapping=&m;}
        if(!mapping||!mapping->r||mapping->x||mapping->w!=pin.writable)return false;
        const auto stop=std::min<std::uintptr_t>(end,mapping->end);
        if(stop<=at||stop>UINT32_MAX)return false;
        if(at<fileEnd||at<lastPage) {
            std::uint32_t offset=0;if(!add32(segment->p_offset,at-start,offset)||!backed(*mapping,at,offset))return false;
            const auto boundary=at<fileEnd?fileEnd:lastPage;
            at=std::uint32_t(std::min<std::uintptr_t>(stop,boundary));
        }else {
            if(mapping->inode||mapping->major||mapping->minor||mapping->offset)return false;
            at=std::uint32_t(stop);
        }
    }return true;
}
}
