// Original record models only. No evdev, kernel queue, timing or document IO.
#include "reader_fixed_pen_stream.h"
#include <algorithm>
#include <cstdio>
#include <stdexcept>
#include <type_traits>
#include <vector>

namespace {
using namespace reader_fixed_pen_stream;
unsigned cases=0,checks=0;
void require(bool value,const char*why){++checks;if(!value)throw std::runtime_error(why);}
template<class F>void scenario(F f){++cases;f();}
using Trace=std::vector<Event>;
constexpr Event syn{0,0,0};
Snapshot initial(){return {};}
Trace full(){Trace t;for(const auto&f:reader_fixed_pen::strokeFrames())for(std::size_t i=0;i<f.count;++i)t.push_back(f.events[i]);return t;}
void apply(Snapshot&s,Event e){
    if(e.type==1){if(e.code==320)s.pen=e.value;else if(e.code==330)s.touch=e.value;}
    else if(e.type==3)switch(e.code){case 0:s.x=e.value;break;case 1:s.y=e.value;break;case 24:s.pressure=e.value;break;case 25:s.distance=e.value;break;default:break;}
}
Snapshot ending(const Trace&t,Snapshot s=initial()){for(auto e:t)apply(s,e);return s;}
void feed(Stream&s,const Trace&t){for(auto e:t)require(s.accept(e),"valid trace record accepted");}
Trace contact(bool start=true,bool end=true){
    const int x=start?11138:11100,y=start?7371:7400;
    Trace t{{3,0,x},{3,1,y},{1,320,1},{1,330,1},{3,24,2630},{3,25,0},syn};
    if(end){t.push_back({3,0,11056});t.push_back({3,1,7616});t.push_back(syn);}
    const auto r=reader_fixed_pen::releaseFrame();for(std::size_t i=0;i<r.count;++i)t.push_back(r.events[i]);return t;
}
void rejected(Event e,Refusal why,bool contactPossible=false){Stream s(initial());require(!s.accept(e),"invalid event rejected");auto r=s.report();require(!r.valid&&r.refusal==why,"specific first refusal");if(contactPossible)require(r.rawContactPossible,"raw possibility survives malformed record");const auto count=r.records;require(!s.accept(syn)&&s.report().records==count&&s.report().refusal==why,"first failure sticky no processing");require(!s.releasedMatches(initial())&&!s.report().strokeObserved,"failure cannot finalize into release/stroke");}
}
int main(){
    static_assert(!std::is_copy_constructible<Stream>::value&&!std::is_move_constructible<Stream>::value,"stream observations cannot be reset by copy/move");
    static_assert(!Report::nativeAuthority&&!Report::releaseAuthority&&!Report::renderAuthority&&!Report::durableAuthority);
    try{
        const Trace original=full();require(original.size()==178,"published original carrier count");
        scenario([&]{Stream s(initial());feed(s,original);const auto before=s.report();require(before.valid&&before.records==178&&before.frames==56&&before.contactFrames==53&&before.contactIntervals==1,"full trace counts");require(before.completeContact&&before.rawContactPossible&&before.finalReleased&&before.startEndpointObserved&&before.endEndpointObserved&&!before.strokeObserved,"pre-finalization facts are separate");require(s.releasedMatches(ending(original)),"full matching fresh snapshot model");const auto r=s.report();require(r.strokeObserved&&r.finalized&&!r.partialFrame,"full fixed endpoint/contact conjunction");});
        scenario([&]{Stream s(initial());require(s.releasedMatches(initial()),"empty stream can match modeled released snapshot");const auto r=s.report();require(!r.strokeObserved&&!r.rawContactPossible&&!r.completeContact&&r.frames==0,"empty stream never stroke");});
        for(int distance:{0,100,255})scenario([&]{auto a=initial();a.distance=distance;Stream s(a);require(s.releasedMatches(a)&&!s.report().strokeObserved,"initial native distance range not manufactured event");});
        scenario([&]{Stream s(initial());Trace release;const auto f=reader_fixed_pen::releaseFrame();for(std::size_t i=0;i<f.count;++i)release.push_back(f.events[i]);feed(s,release);require(s.releasedMatches(ending(release)),"release-only fresh state model");require(!s.report().strokeObserved&&!s.report().rawContactPossible&&s.report().frames==1,"release-only not contact/stroke");});
        scenario([&]{Snapshot state=initial();Trace filtered,frame;for(auto e:original){if(e.type==0){if(!frame.empty()){filtered.insert(filtered.end(),frame.begin(),frame.end());filtered.push_back(e);frame.clear();}}else{auto next=state;apply(next,e);if(!same(next,state)){frame.push_back(e);state=next;}}}Stream s(initial());feed(s,filtered);require(filtered.size()<178&&s.releasedMatches(state)&&s.report().strokeObserved,"unchanged values/empty frames filtered without requiring178");});
        scenario([&]{Trace redundant;for(auto e:original){redundant.push_back(e);redundant.push_back(e);}Stream s(initial());feed(s,redundant);require(s.releasedMatches(ending(original))&&s.report().strokeObserved&&s.report().contactIntervals==1,"redundant values and SYNs do not create contact intervals");});
        // Each cut models arbitrary read-batch boundaries. Only SYN records commit frames.
        for(std::size_t cut=1;cut<original.size();++cut)scenario([&]{Stream s(initial());for(std::size_t i=0;i<cut;++i)require(s.accept(original[i]),"prefix accepts");const auto r=s.report();require(r.frames==std::uint32_t(std::count_if(original.begin(),original.begin()+std::ptrdiff_t(cut),[](Event e){return e.type==0;})),"split batch cannot invent SYN");require(r.partialFrame==(original[cut-1].type!=0),"split batch retains partial frame");for(std::size_t i=cut;i<original.size();++i)require(s.accept(original[i]),"suffix accepts");require(s.releasedMatches(ending(original))&&s.report().strokeObserved,"all batch cuts preserve full result");});
        for(int field=0;field<14;++field)scenario([&]{auto a=initial();switch(field){case 0:a.x=-1;break;case 1:a.x=20967;break;case 2:a.y=-1;break;case 3:a.y=15726;break;case 4:a.pressure=1;break;case 5:a.distance=-1;break;case 6:a.distance=256;break;case 7:a.tiltX=1;break;case 8:a.tiltY=-1;break;case 9:a.pen=true;break;case 10:a.touch=true;break;case 11:a.rubber=true;break;case 12:a.stylus=true;break;default:a.stylus2=true;}Stream s(a);require(!s.report().valid&&s.report().refusal==Refusal::Initial,"bad initial state refused");require(!s.accept(syn)&&!s.releasedMatches(initial()),"bad initial cannot revive");if(a.touch||a.rubber||a.pressure>0)require(s.report().rawContactPossible,"bad initial raw contact retained");});
        for(Event e:Trace{{3,0,11055},{3,0,11139},{3,1,7370},{3,1,7617},{3,24,-1},{3,24,1},{3,24,4095},{3,25,-1},{3,25,1},{3,25,255},{1,320,-1},{1,320,2},{1,330,-1},{1,330,2},{0,0,1}})scenario([&]{rejected(e,Refusal::Range,(e.type==3&&e.code==24&&e.value>0)||(e.type==1&&e.code==330&&e.value>0));});
        for(Event e:Trace{{1,321,0},{1,321,1},{1,331,0},{1,332,1},{3,26,0},{3,26,1},{3,27,0},{0,1,0},{0,2,0},{4,4,0},{65535,65535,1}})scenario([&]{rejected(e,Refusal::Unknown,e.type==1&&e.code==321&&e.value>0);});
        scenario([&]{rejected({0,3,0},Refusal::Dropped,true);});
        for(int variant=0;variant<5;++variant)scenario([&]{Stream s(initial());Trace t{{3,0,11138},{3,1,7371}};if(variant!=0)t.push_back({1,320,1});if(variant!=1)t.push_back({1,330,1});if(variant!=2)t.push_back({3,24,2630});if(variant!=3)t.push_back({3,25,0});if(variant==4)t.push_back({1,320,0});feed(s,t);require(!s.accept(syn)&&s.report().refusal==Refusal::Conflict&&s.report().rawContactPossible,"incoherent complete contact frame refused");});
        scenario([&]{Stream s(initial());feed(s,{{1,320,1},{1,330,1},{3,24,2630},{3,25,0}});require(!s.accept(syn)&&s.report().refusal==Refusal::Range,"contact cannot inherit outside-rectangle initial XY");});
        scenario([&]{Stream s(initial());feed(s,{{1,330,1},{1,330,0}});require(!s.accept({1,330,1})&&s.report().refusal==Refusal::RepeatedContact&&s.report().rawContactPossible,"raw down-up-down before SYN refused");});
        scenario([&]{Stream s(initial());feed(s,{{3,24,2630},{3,24,0}});require(!s.accept({3,24,2630})&&s.report().refusal==Refusal::RepeatedContact&&s.report().rawContactPossible,"raw pressure pulse repeat before SYN refused");});
        scenario([&]{Stream s(initial());auto t=contact();feed(s,t);require(!s.accept({1,330,1})&&s.report().refusal==Refusal::RepeatedContact,"second complete contact cannot restart");});
        for(bool start:{false,true})for(bool end:{false,true})scenario([&]{Stream s(initial());auto t=contact(start,end);feed(s,t);require(s.releasedMatches(ending(t)),"single interval fresh released model");require(s.report().strokeObserved==(start&&end),"both contact endpoints required for stroke fact");});
        scenario([&]{Stream s(initial());Trace t{{3,0,11138},{3,1,7371},{1,320,1},{1,330,1},{3,24,2630},{3,25,0},syn};feed(s,t);auto held=ending(t);require(!s.releasedMatches(held)&&s.report().refusal==Refusal::Held&&s.report().completeContact,"held contact not released despite matching snapshot");});
        scenario([&]{Stream s(initial());feed(s,{{1,330,1}});require(!s.releasedMatches(initial())&&s.report().refusal==Refusal::Partial&&s.report().rawContactPossible,"partial frame cannot finalize");require(!s.accept(syn)&&s.report().refusal==Refusal::Partial,"late SYN cannot repair failed finalization");});
        scenario([&]{Stream s(initial());feed(s,{{1,330,1},{3,24,2630}});const auto f=reader_fixed_pen::releaseFrame();Trace t;for(std::size_t i=0;i<f.count;++i)t.push_back(f.events[i]);feed(s,t);require(s.releasedMatches(ending(t))&&s.report().rawContactPossible&&!s.report().completeContact&&!s.report().strokeObserved,"merged raw contact/release is not complete contact proof");});
        for(int field=0;field<11;++field)scenario([&]{Stream s(initial());feed(s,original);auto a=ending(original);switch(field){case 0:--a.x;break;case 1:--a.y;break;case 2:a.pressure=1;break;case 3:a.distance=0;break;case 4:a.tiltX=1;break;case 5:a.tiltY=1;break;case 6:a.pen=true;break;case 7:a.touch=true;break;case 8:a.rubber=true;break;case 9:a.stylus=true;break;default:a.stylus2=true;}require(!s.releasedMatches(a)&&!s.report().strokeObserved,"fresh snapshot conflict cannot prove stroke release");require(!s.releasedMatches(ending(original)),"matching second snapshot cannot retry failed finalization");});
        scenario([&]{Stream s(initial());for(unsigned i=0;i<Stream::MaxRecords;++i)require(s.accept(syn),"quota exact boundary accepted");require(s.report().records==8192&&s.releasedMatches(initial()),"quota exact empty/release state may finalize");});
        scenario([&]{Stream s(initial());for(unsigned i=0;i<Stream::MaxRecords;++i)require(s.accept(syn),"quota prefix accepted");require(!s.accept({1,330,1})&&s.report().refusal==Refusal::Quota&&s.report().records==8192&&s.report().rawContactPossible,"quota overflow raw contact remains visible");});
        scenario([&]{Stream s(initial());require(s.releasedMatches(initial()),"first empty finalization");require(!s.releasedMatches(initial())&&s.report().refusal==Refusal::Finalized,"second finalization is not a retry");});
        scenario([&]{Stream s(initial());require(s.releasedMatches(initial()),"finalize before extra event");require(!s.accept({1,330,1})&&s.report().refusal==Refusal::Finalized&&s.report().rawContactPossible,"post-finalization input poisons rather than revives");});
        std::printf("PASS fixed pen stream scenarios=%u checks=%u records_cap=8192 modeled_kernel=true native_io=false\n",cases,checks);return 0;
    }catch(const std::exception&e){std::fprintf(stderr,"FAIL fixed pen stream %s scenarios=%u checks=%u\n",e.what(),cases,checks);return 1;}
}
