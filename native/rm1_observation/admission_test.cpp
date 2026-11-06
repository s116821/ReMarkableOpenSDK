// Owned profile-shape controls only. None of these tables can issue Admission.
#include "admission.h"
#include "profile.h"
#include <cassert>
#include <cstdio>
#include <type_traits>
#ifdef NDEBUG
#error Admission fixture assertions must remain enabled.
#endif
using namespace remarkable::rm1_observation;
using namespace remarkable::rm1_observation::detail;
static_assert(!std::is_default_constructible_v<Admission>);
static_assert(!std::is_copy_constructible_v<Admission>);
static_assert(!std::is_copy_assignable_v<Admission>);
static_assert(!std::is_destructible_v<Admission>);
int main() {
    assert(reviewedProfile()==nullptr);
    const char *hash="1111111111111111111111111111111111111111111111111111111111111111";
    const std::array<const char*,8> names{{"core","qml","quick","gui","network","libc","cxx","gcc"}};
    std::array<SymbolPin,8> symbols;
    std::array<BindingSite,8> sites;
    for(std::size_t i=0;i<symbols.size();++i) {
        sites[i]={Consumer::Adapter,std::uint32_t(4+i*4),1,2,22};
        auto &s=symbols[i];s.name=names[i];s.version="fixture-only";s.provider=ProviderRole(i);
        s.value=4;s.size=4;s.dynsymIndex=1;s.versionIndex=2;s.sites=&sites[i];s.siteCount=1;
    }
    Profile p;p.manifestSha256=hash;p.executableSha256=hash;p.qtVersion="fixture-only";p.executableBytes=128;
    for(auto &pin:p.providers)pin={hash,128};
    p.symbols=symbols.data();p.symbolCount=symbols.size();
    assert(complete(p)); // Schema mechanics, NOT runtime/provider compatibility.
    for(std::size_t i=0;i<p.providers.size();++i){auto broken=p;broken.providers[i]={};assert(!complete(broken));}
    for(std::size_t i=0;i<symbols.size();++i){p.symbolCount=i;assert(!complete(p));}
    p.symbolCount=symbols.size();
    for(int fault=0;fault<10;++fault){
        auto broken=p;
        if(fault==0)broken.manifestSha256=nullptr;
        if(fault==1)broken.executableSha256="0000000000000000000000000000000000000000000000000000000000000000";
        if(fault==2)broken.executableSha256="FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF";
        if(fault==3)broken.qtVersion="";
        if(fault==4)broken.executableBytes=0;
        if(fault==5)broken.executableBytes=64*1024*1024+1;
        if(fault==6)broken.symbolCount=4097;
        if(fault==7)broken.symbols=nullptr;
        if(fault==8)broken.providers[0].bytes=0;
        if(fault==9)broken.providers[0].bytes=64*1024*1024+1;
        assert(!complete(broken));
    }
    for(int fault=0;fault<16;++fault){
        const auto saved=symbols[0];
        if(fault==0)symbols[0].name="";
        if(fault==1)symbols[0].name=symbols[1].name;
        if(fault==2)symbols[0].version="";
        if(fault==3)symbols[0].provider=ProviderRole::Count;
        if(fault==4)symbols[0].value=0;
        if(fault==5)symbols[0].size=0;
        if(fault==6)symbols[0].siteCount=0;
        if(fault==7)symbols[0].dynsymIndex=0;
        if(fault==8)symbols[0].versionIndex=1;
        if(fault==9)symbols[0].kind=SymbolKind(99);
        if(fault==10)symbols[0].binding=2;
        if(fault==11)symbols[0].visibility=2;
        if(fault==12)symbols[0].alignment=3;
        if(fault==13)symbols[0].sites=nullptr;
        if(fault==14)symbols[0].siteCount=257;
        if(fault==15)symbols[0].kind=SymbolKind::Object;
        assert(!complete(p));symbols[0]=saved;
    }
    for(int fault=0;fault<7;++fault) {
        const auto saved=sites[0];
        if(fault==0)sites[0].consumer=Consumer::Count;
        if(fault==1)sites[0].offset=3;
        if(fault==2)sites[0].offset=sites[1].offset;
        if(fault==3)sites[0].dynsymIndex=0;
        if(fault==4)sites[0].versionIndex=1;
        if(fault==5)sites[0].relocation=20; // COPY cannot be selected by schema.
        if(fault==6)sites[0].relocation=2; // Observed ABS32 awaits separate rules.
        assert(!complete(p));sites[0]=saved;
    }
    assert(reviewedProfile()==nullptr);
    std::puts("PASS sealed admission traits and 49 incomplete-profile controls; no RM1 admission or target facts");
}
