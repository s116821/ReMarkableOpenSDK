#pragma once
// Internal RM1 inputs only. This is not a public profile/admission framework.
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>

namespace remarkable::rm1_observation::detail {
enum class ProviderRole { Core, Qml, Quick, Gui, Network, Libc, Cxx, Gcc, Count };
struct ProviderPin { const char *sha256=nullptr; std::size_t bytes=0; };
enum class SymbolKind : std::uint8_t { Object=1, Function=2 };
enum class Consumer : std::uint8_t { Adapter, Main, Count };
struct BindingSite {
    Consumer consumer=Consumer::Count;
    std::uint32_t offset=0,dynsymIndex=0;
    std::uint16_t versionIndex=0; // Exact consumer versym, including hidden bit.
    std::uint8_t relocation=0; // Exact reviewed ABS32(2)/GLOB_DAT(21)/JUMP_SLOT(22).
    std::uint32_t addend=0; // Original REL word; never inferred from live memory.
};
struct SymbolPin {
    const char *name=nullptr,*version=nullptr;
    ProviderRole provider=ProviderRole::Count;
    std::uint32_t value=0,size=0,dynsymIndex=0;
    std::uint16_t versionIndex=0; // Exact provider versym, not consumer's index.
    SymbolKind kind=SymbolKind::Function;
    std::uint8_t binding=1,visibility=0; // STB_GLOBAL/STV_DEFAULT initially.
    std::uint32_t alignment=1; // Data alignment only; functions use ARM/Thumb state.
    bool writable=false; // Expected effective OBJECT mapping permission.
    const BindingSite *sites=nullptr;
    std::size_t siteCount=0;
};
struct ArtifactSelfPin {
    const char *name=nullptr;
    std::uint32_t value=0,size=0,dynsymIndex=0;
    SymbolKind kind=SymbolKind::Function;
    std::uint8_t binding=2,visibility=0; // Only observed WEAK/default definitions.
    std::uint16_t versionIndex=1;
    std::uint32_t alignment=1;
    bool writable=false;
    const BindingSite *sites=nullptr;
    std::size_t siteCount=0;
};
struct OptionalCrtNull {
    const char *name=nullptr;
    std::uint32_t dynsymIndex=0;
    const BindingSite *sites=nullptr;
    std::size_t siteCount=0;
};
inline int optionalCrtName(const char *name) noexcept {
    if(!name)return -1;
    if(std::strcmp(name,"_ITM_registerTMCloneTable")==0)return 0;
    if(std::strcmp(name,"_ITM_deregisterTMCloneTable")==0)return 1;
    if(std::strcmp(name,"__gmon_start__")==0)return 2;
    return -1;
}
inline bool optionalCrtShape(const OptionalCrtNull &pin) noexcept {
    const auto name=optionalCrtName(pin.name);
    if(name<0||!pin.dynsymIndex||pin.dynsymIndex>=65536||!pin.sites||pin.siteCount!=(name==2?2u:1u))return false;
    unsigned glob=0,jump=0;
    for(std::size_t i=0;i<pin.siteCount;++i) {
        const auto &s=pin.sites[i];
        if(s.consumer!=Consumer::Adapter||!s.offset||s.offset%4||s.dynsymIndex!=pin.dynsymIndex||
           s.versionIndex!=1||s.addend)return false;
        if(s.relocation==21)++glob;
        else if(s.relocation==22&&name==2)++jump;
        else return false;
        for(std::size_t j=0;j<i;++j)if(s.offset==pin.sites[j].offset)return false;
    }
    return glob==1&&jump==unsigned(name==2);
}
inline bool bindingForm(const SymbolPin &s,const BindingSite &site) noexcept {
    if(s.kind!=SymbolKind::Function&&s.kind!=SymbolKind::Object)return false;
    if(site.relocation==21||site.relocation==22)
        return site.addend==0&&(s.kind==SymbolKind::Function||site.relocation==21);
    if(site.relocation!=2)return false;
    if(s.kind==SymbolKind::Function)return site.addend==0;
    if(s.kind!=SymbolKind::Object||s.size<4)return false;
    if(site.addend==0)return true;
    return site.addend==8&&s.provider==ProviderRole::Cxx&&s.size==44&&s.name&&s.version&&
        std::strcmp(s.version,"CXXABI_1.3")==0&&
        (std::strcmp(s.name,"_ZTVN10__cxxabiv117__class_type_infoE")==0||
         std::strcmp(s.name,"_ZTVN10__cxxabiv120__si_class_type_infoE")==0);
}
inline bool bindingForm(const ArtifactSelfPin &s,const BindingSite &site) noexcept {
    return site.consumer==Consumer::Adapter&&site.addend==0&&
        ((site.relocation==2&&(s.kind==SymbolKind::Function||(s.kind==SymbolKind::Object&&s.size>=4)))||
         (site.relocation==21&&s.kind==SymbolKind::Object)||(site.relocation==22&&s.kind==SymbolKind::Function));
}
struct Profile {
    const char *manifestSha256=nullptr,*executableSha256=nullptr,*qtVersion=nullptr;
    std::size_t executableBytes=0;
    std::array<ProviderPin,std::size_t(ProviderRole::Count)> providers{};
    const SymbolPin *symbols=nullptr;
    std::size_t symbolCount=0;
    const ArtifactSelfPin *selfSymbols=nullptr;
    std::size_t selfSymbolCount=0;
    const OptionalCrtNull *optionalCrt=nullptr;
    std::size_t optionalCrtCount=0;
};
inline bool digest(const char *s) noexcept {
    if(!s||std::strlen(s)!=64)return false;
    bool nonzero=false;
    for(unsigned i=0;i<64;++i){if(!((s[i]>='0'&&s[i]<='9')||(s[i]>='a'&&s[i]<='f')))return false;nonzero=nonzero||s[i]!='0';}
    return nonzero;
}
inline bool complete(const Profile &p) noexcept {
    if(!digest(p.manifestSha256)||!digest(p.executableSha256)||!p.qtVersion||!*p.qtVersion||
       !p.executableBytes||p.executableBytes>64*1024*1024||!p.symbols||!p.symbolCount||p.symbolCount>4096)return false;
    std::array<bool,std::size_t(ProviderRole::Count)> represented{};
    for(const auto &pin:p.providers)if(!digest(pin.sha256)||!pin.bytes||pin.bytes>64*1024*1024)return false;
    std::size_t totalSites=0;
    for(std::size_t i=0;i<p.symbolCount;++i){
        const auto &s=p.symbols[i];const auto role=std::size_t(s.provider);
        if(!s.name||!*s.name||!s.version||!*s.version||role>=represented.size()||!s.value||!s.size||
           !s.dynsymIndex||s.dynsymIndex>=65536||(s.versionIndex&0x7fff)<2||s.binding!=1||s.visibility!=0||
           (s.kind!=SymbolKind::Function&&s.kind!=SymbolKind::Object)||!s.alignment||
           (s.alignment&(s.alignment-1))||s.alignment>4096||!s.sites||!s.siteCount||s.siteCount>256)return false;
        if(totalSites>8192-s.siteCount)return false;
        totalSites+=s.siteCount;
        for(std::size_t j=0;j<i;++j)if(std::strcmp(s.name,p.symbols[j].name)==0&&
            std::strcmp(s.version,p.symbols[j].version)==0)return false;
        for(std::size_t j=0;j<s.siteCount;++j){
            const auto &site=s.sites[j];
            if(site.consumer>=Consumer::Count||!site.offset||site.offset%4||!site.dynsymIndex||
               site.dynsymIndex>=65536||(site.versionIndex&0x7fff)<2||
               !bindingForm(s,site))return false;
            for(std::size_t k=0;k<=i;++k){
                const auto &other=p.symbols[k];
                for(std::size_t n=0;n<(k==i?j:other.siteCount);++n)
                    if(site.consumer==other.sites[n].consumer&&site.offset==other.sites[n].offset)return false;
            }
        }
        represented[role]=true;
    }
    if(p.selfSymbolCount>4096||bool(p.selfSymbols)!=bool(p.selfSymbolCount))return false;
    for(std::size_t i=0;i<p.selfSymbolCount;++i) {
        const auto &s=p.selfSymbols[i];
        if(!s.name||!*s.name||!s.value||!s.size||!s.dynsymIndex||s.dynsymIndex>=65536||
           s.binding!=2||s.visibility!=0||s.versionIndex!=1||
           (s.kind!=SymbolKind::Function&&s.kind!=SymbolKind::Object)||!s.alignment||
           (s.alignment&(s.alignment-1))||s.alignment>4096||!s.sites||!s.siteCount||s.siteCount>256||
           totalSites>8192-s.siteCount)return false;
        totalSites+=s.siteCount;
        for(std::size_t j=0;j<p.symbolCount;++j)if(std::strcmp(s.name,p.symbols[j].name)==0)return false;
        for(std::size_t j=0;j<i;++j)if(std::strcmp(s.name,p.selfSymbols[j].name)==0)return false;
        for(std::size_t j=0;j<s.siteCount;++j) {
            const auto &site=s.sites[j];
            if(!site.offset||site.offset%4||!site.dynsymIndex||site.dynsymIndex>=65536||
               site.versionIndex!=1||!bindingForm(s,site))return false;
            for(std::size_t k=0;k<p.symbolCount;++k)for(std::size_t n=0;n<p.symbols[k].siteCount;++n)
                if(site.consumer==p.symbols[k].sites[n].consumer&&site.offset==p.symbols[k].sites[n].offset)return false;
            for(std::size_t k=0;k<=i;++k)for(std::size_t n=0;n<(k==i?j:p.selfSymbols[k].siteCount);++n)
                if(site.offset==p.selfSymbols[k].sites[n].offset)return false;
        }
    }
    if(bool(p.optionalCrt)!=bool(p.optionalCrtCount)||(p.optionalCrtCount&&p.optionalCrtCount!=3))return false;
    std::array<bool,3> crtNames{};
    for(std::size_t i=0;i<p.optionalCrtCount;++i) {
        const auto &pin=p.optionalCrt[i];
        if(!optionalCrtShape(pin)||totalSites>8192-pin.siteCount)return false;
        totalSites+=pin.siteCount;const auto name=std::size_t(optionalCrtName(pin.name));
        if(crtNames[name])return false;
        crtNames[name]=true;
        for(std::size_t k=0;k<p.symbolCount;++k)if(std::strcmp(pin.name,p.symbols[k].name)==0)return false;
        for(std::size_t k=0;k<p.selfSymbolCount;++k)if(std::strcmp(pin.name,p.selfSymbols[k].name)==0)return false;
        for(std::size_t j=0;j<pin.siteCount;++j) {
            const auto &site=pin.sites[j];
            for(std::size_t k=0;k<p.symbolCount;++k)for(std::size_t n=0;n<p.symbols[k].siteCount;++n)
                if(site.consumer==p.symbols[k].sites[n].consumer&&site.offset==p.symbols[k].sites[n].offset)return false;
            for(std::size_t k=0;k<p.selfSymbolCount;++k)for(std::size_t n=0;n<p.selfSymbols[k].siteCount;++n)
                if(site.offset==p.selfSymbols[k].sites[n].offset)return false;
            for(std::size_t k=0;k<i;++k)for(std::size_t n=0;n<p.optionalCrt[k].siteCount;++n)
                if(site.offset==p.optionalCrt[k].sites[n].offset)return false;
        }
    }
    for(bool present:represented)if(!present)return false;
    return true;
}
// There is NO approved RM1 observation profile. Supplying an environment variable,
// test table or peer's hashes cannot select one. A future real table is a separate
// evidence-backed source/review change, not a build-time bypass.
inline const Profile *reviewedProfile() noexcept { return nullptr; }
}
