#pragma once
// Internal RM1 inputs only. This is not a public profile/admission framework.
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>

namespace remarkable::rm1_observation::detail {
enum class ProviderRole { Core, Qml, Quick, Gui, Network, Libc, Cxx, Gcc, Count };
struct ProviderPin { const char *sha256=nullptr; std::size_t bytes=0; };
struct SymbolPin {
    const char *name=nullptr,*version=nullptr;
    ProviderRole provider=ProviderRole::Count;
    std::uint32_t value=0,size=0;
    bool adapterBinding=false,mainBinding=false;
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
    for(std::size_t i=0;i<p.symbolCount;++i){
        const auto &s=p.symbols[i];const auto role=std::size_t(s.provider);
        if(!s.name||!*s.name||!s.version||!*s.version||role>=represented.size()||!s.value||!s.size||
           (!s.adapterBinding&&!s.mainBinding))return false;
        for(std::size_t j=0;j<i;++j)if(std::strcmp(s.name,p.symbols[j].name)==0)return false;
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
