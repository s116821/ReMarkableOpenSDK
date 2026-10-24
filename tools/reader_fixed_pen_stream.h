#pragma once
// Owned record-state policy. No kernel, device, clock, queue or routing authority.
#include "reader_fixed_stroke_policy.h"
#include <cstdint>

namespace reader_fixed_pen_stream {
using Event=reader_fixed_pen::Event;
struct Snapshot {
    int x=0,y=0,pressure=0,distance=100,tiltX=0,tiltY=0;
    bool pen=false,touch=false,rubber=false,stylus=false,stylus2=false;
    bool valid()const noexcept {
        return x>=0&&x<=20966&&y>=0&&y<=15725&&pressure>=0&&pressure<=4095&&
            distance>=0&&distance<=255&&tiltX>=-9000&&tiltX<=9000&&tiltY>=-9000&&tiltY<=9000;
    }
};
inline bool same(const Snapshot&a,const Snapshot&b)noexcept {
    return a.x==b.x&&a.y==b.y&&a.pressure==b.pressure&&a.distance==b.distance&&
        a.tiltX==b.tiltX&&a.tiltY==b.tiltY&&a.pen==b.pen&&a.touch==b.touch&&
        a.rubber==b.rubber&&a.stylus==b.stylus&&a.stylus2==b.stylus2;
}
inline bool released(const Snapshot&s)noexcept {
    return s.valid()&&!s.pen&&!s.touch&&!s.rubber&&!s.stylus&&!s.stylus2&&s.pressure==0;
}
enum class Refusal {None,Initial,Quota,Dropped,Unknown,Range,Conflict,RepeatedContact,Partial,Held,SnapshotMismatch,Finalized};
struct Report {
    bool valid=false,finalized=false,partialFrame=false,rawContactPossible=false;
    bool completeContact=false,finalReleased=false,startEndpointObserved=false,endEndpointObserved=false,strokeObserved=false;
    std::uint32_t records=0,frames=0,contactFrames=0,contactIntervals=0;
    Snapshot state{};Refusal refusal=Refusal::None;
    static constexpr bool nativeAuthority=false,releaseAuthority=false,renderAuthority=false,durableAuthority=false;
};
class Stream final {
public:
    static constexpr std::uint32_t MaxRecords=8192;
    explicit Stream(Snapshot initial)noexcept {
        r_.state=initial;r_.rawContactPossible=initial.touch||initial.pressure>0||initial.rubber;
        r_.valid=released(initial)&&initial.tiltX==0&&initial.tiltY==0;
        if(!r_.valid)r_.refusal=Refusal::Initial;
    }
    Stream(const Stream&)=delete;Stream&operator=(const Stream&)=delete;
    Stream(Stream&&)=delete;Stream&operator=(Stream&&)=delete;
    bool accept(Event e)noexcept {
        // Even a rejected/over-quota raw contact attempt must not disappear.
        if((e.type==1&&(e.code==330||e.code==321)&&e.value>0)||
           (e.type==3&&e.code==24&&e.value>0)||(e.type==0&&e.code==3))r_.rawContactPossible=true;
        if(!r_.valid)return false;
        if(r_.finalized)return fail(Refusal::Finalized);
        if(r_.records==MaxRecords)return fail(Refusal::Quota);
        ++r_.records;
        if(e.type==0) {
            if(e.code==3)return fail(Refusal::Dropped);
            if(e.code!=0)return fail(Refusal::Unknown);
            if(e.value!=0)return fail(Refusal::Range);
            const auto&s=r_.state;
            if((s.touch&&(!s.pen||s.pressure!=2630||s.distance!=0))||(!s.touch&&s.pressure!=0))return fail(Refusal::Conflict);
            if(s.touch&&(s.x<11056||s.x>11138||s.y<7371||s.y>7616))return fail(Refusal::Range);
            if(s.touch) {
                if(!contact_) {
                    if(r_.contactIntervals)return fail(Refusal::RepeatedContact);
                    ++r_.contactIntervals;
                }
                ++r_.contactFrames;r_.completeContact=true;
                r_.startEndpointObserved=r_.startEndpointObserved||(s.x==11138&&s.y==7371);
                r_.endEndpointObserved=r_.endEndpointObserved||(s.x==11056&&s.y==7616);
            }
            if(contact_&&!s.touch)contactClosed_=true;
            contact_=s.touch;++r_.frames;r_.partialFrame=false;return true;
        }
        r_.partialFrame=true;
        if(e.type==1) {
            if(e.code!=320&&e.code!=330)return fail(Refusal::Unknown);
            if(e.value!=0&&e.value!=1)return fail(Refusal::Range);
            if(e.code==320)r_.state.pen=e.value!=0;
            else {
                if(e.value&&!r_.state.touch) {if(touchEnded_)return fail(Refusal::RepeatedContact);touchSeen_=true;}
                if(!e.value&&r_.state.touch&&touchSeen_)touchEnded_=true;
                r_.state.touch=e.value!=0;
            }
            return true;
        }
        if(e.type!=3)return fail(Refusal::Unknown);
        switch(e.code) {
        case 0:if(e.value<11056||e.value>11138)return fail(Refusal::Range);r_.state.x=e.value;break;
        case 1:if(e.value<7371||e.value>7616)return fail(Refusal::Range);r_.state.y=e.value;break;
        case 24:
            if(e.value!=0&&e.value!=2630)return fail(Refusal::Range);
            if(e.value&&!r_.state.pressure) {if(pressureEnded_)return fail(Refusal::RepeatedContact);pressureSeen_=true;}
            if(!e.value&&r_.state.pressure&&pressureSeen_)pressureEnded_=true;
            r_.state.pressure=e.value;break;
        case 25:if(e.value!=0&&e.value!=100)return fail(Refusal::Range);r_.state.distance=e.value;break;
        default:return fail(Refusal::Unknown);
        }
        return true;
    }
    // One finalization attempt, not a live ioctl or permission. The caller must
    // independently establish raw-read framing, same device, clocks and quiet queue.
    bool releasedMatches(const Snapshot&snapshot)noexcept {
        if(!r_.valid)return false;
        if(r_.finalized)return fail(Refusal::Finalized);
        r_.finalized=true;
        if(r_.partialFrame)return fail(Refusal::Partial);
        if(!released(r_.state)||!released(snapshot))return fail(Refusal::Held);
        if(!same(r_.state,snapshot))return fail(Refusal::SnapshotMismatch);
        return true;
    }
    Report report()const noexcept {
        auto result=r_;result.finalReleased=result.valid&&!result.partialFrame&&released(result.state);
        result.strokeObserved=result.valid&&result.finalized&&result.finalReleased&&contactClosed_&&
            result.contactIntervals==1&&result.completeContact&&result.startEndpointObserved&&result.endEndpointObserved;
        return result;
    }
private:
    bool fail(Refusal why)noexcept {if(r_.valid){r_.valid=false;r_.refusal=why;}return false;}
    Report r_{};bool contact_=false,contactClosed_=false;
    bool touchSeen_=false,touchEnded_=false,pressureSeen_=false,pressureEnded_=false;
};
} // namespace reader_fixed_pen_stream
