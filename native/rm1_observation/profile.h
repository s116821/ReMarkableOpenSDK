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
    std::uint8_t relocation=0; // Reviewed R_ARM_GLOB_DAT(21)/JUMP_SLOT(22) only.
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
struct Profile {
    const char *manifestSha256=nullptr,*executableSha256=nullptr,*qtVersion=nullptr;
    std::size_t executableBytes=0;
    std::array<ProviderPin,std::size_t(ProviderRole::Count)> providers{};
    const SymbolPin *symbols=nullptr;
    std::size_t symbolCount=0;
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
               (site.relocation!=21&&site.relocation!=22)||
               (s.kind==SymbolKind::Object&&site.relocation!=21))return false;
            for(std::size_t k=0;k<=i;++k){
                const auto &other=p.symbols[k];
                for(std::size_t n=0;n<(k==i?j:other.siteCount);++n)
                    if(site.consumer==other.sites[n].consumer&&site.offset==other.sites[n].offset)return false;
            }
        }
        represented[role]=true;
    }
    for(bool present:represented)if(!present)return false;
    return true;
}
// There is NO approved RM1 observation profile. Supplying an environment variable,
// test table or peer's hashes cannot select one. A future real table is a separate
// evidence-backed source/review change, not a build-time bypass.
inline const Profile *reviewedProfile() noexcept { return nullptr; }
}
