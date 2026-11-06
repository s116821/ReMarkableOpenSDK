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
    for(std::size_t i=0;i<symbols.size();++i)symbols[i]={names[i],"fixture-only",ProviderRole(i),4,4,true,false};
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
    for(int fault=0;fault<7;++fault){
        const auto saved=symbols[0];
        if(fault==0)symbols[0].name="";
        if(fault==1)symbols[0].name=symbols[1].name;
        if(fault==2)symbols[0].version="";
        if(fault==3)symbols[0].provider=ProviderRole::Count;
        if(fault==4)symbols[0].value=0;
        if(fault==5)symbols[0].size=0;
        if(fault==6)symbols[0].adapterBinding=false;
        assert(!complete(p));symbols[0]=saved;
    }
    assert(reviewedProfile()==nullptr);
    std::puts("PASS sealed admission traits and 33 incomplete-profile controls; no RM1 admission or target facts");
}
