#pragma once
// Owned research policy only. IO supplies real provenance or explicit models.
#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <limits>

namespace reader_fixed_pen {
struct Event {std::uint16_t type=0,code=0;std::int32_t value=0;};
struct Frame {std::array<Event,7> events{};std::size_t count=0;unsigned delayMs=0;};
constexpr Frame frame(std::initializer_list<Event> events,unsigned delay) {
    if(events.size()>Frame{}.events.size())return {}; // Explicit invalid, never truncate/overrun.
    Frame result{};result.delayMs=delay;
    for(const auto &event:events)result.events[result.count++]=event;
    return result;
}
constexpr Frame releaseFrame() {return frame({{3,24,0},{3,25,100},{1,330,0},{1,320,0},{0,0,0}},10);}
constexpr std::array<Frame,56> strokeFrames() {
    std::array<Frame,56> result{};
    result[0]=releaseFrame();
    result[1]=frame({{3,0,11138},{3,1,7371},{1,320,1},{1,330,0},{3,24,0},{3,25,100},{0,0,0}},10);
    result[2]=frame({{1,320,1},{1,330,1},{3,24,2630},{3,25,0},{0,0,0}},10);
    for(int i=1;i<=52;++i)result[std::size_t(i+2)]=frame({{3,0,11138+(-82*i)/52},{3,1,7371+(245*i)/52},{0,0,0}},1);
    result[55]=releaseFrame();return result;
}
constexpr std::size_t recordCount() {std::size_t count=0;for(const auto &f:strokeFrames())count+=f.count;return count;}
static_assert(recordCount()==178,"fixed source-derived record count");
constexpr std::size_t KernelRecordBytes=16;
struct Budget {
    std::uint64_t began=0,strokeEnd=0,recoveryEnd=0;
    bool valid() const {return began<=std::numeric_limits<std::uint64_t>::max()-3000 && strokeEnd==began+1000 && recoveryEnd==began+3000;}
};
inline Budget reserve(std::uint64_t began) {
    if(began>std::numeric_limits<std::uint64_t>::max()-3000)return {};
    return {began,began+1000,began+3000};
}
struct Write {std::ptrdiff_t bytes=-1;int error=0;};
enum class Refusal {None,Budget,Clock,Context,WriteError,WriteZero,WriteShort,WriteOversize,Pause,Exception,NotArmed,Spent,Device,ReleaseObservation,InvalidFrame};
struct Dispatch {
    bool complete=false,attempted=false,contactPossible=false,unknown=false;
    std::size_t calls=0,frames=0,acceptedRecords=0;
    Refusal refusal=Refusal::None;
};
class DispatchClaim final {
public:
    DispatchClaim()=default;
    DispatchClaim(const DispatchClaim&)=delete;DispatchClaim&operator=(const DispatchClaim&)=delete;
    DispatchClaim(DispatchClaim&&)=delete;DispatchClaim&operator=(DispatchClaim&&)=delete;
    bool spent()const noexcept{return spent_.load(std::memory_order_acquire);}
    bool consume()noexcept{return !spent_.exchange(true,std::memory_order_acq_rel);}
private:
    static_assert(std::atomic<bool>::is_always_lock_free,"claim admission must not call out or lock");
    std::atomic<bool> spent_{false};
};
// IO: nowMs(), allowed(), emit(Event*,count)->Write, pauseMs(unsigned)->bool.
// emit must construct actual vendor records; success bytes are count*16.
// One claim must survive every retry/reentrant call. A Spent result does not
// erase the original caller's possible-dispatch/contact facts. Claims are not
// device/nonce authority, cross-process admission or a replacement for recovery.
template<class IO> Dispatch dispatch(IO &io,const Budget &budget,DispatchClaim &claim) noexcept {
    Dispatch result;
    if(!claim.consume()){result.refusal=Refusal::Spent;return result;}
    const auto refuse=[&](Refusal why){result.refusal=why;result.unknown=result.attempted;return result;};
    try {
        if(!budget.valid())return refuse(Refusal::Budget);
        auto last=budget.began;
        const auto permit=[&]() {
            const auto now=io.nowMs();
            if(now<last){result.refusal=Refusal::Clock;return false;}
            last=now;
            if(now>=budget.strokeEnd){result.refusal=Refusal::Budget;return false;}
            if(!io.allowed()){result.refusal=Refusal::Context;return false;}
            // IO permission is a callout: it cannot spend the remaining budget
            // or regress the clock and then authorize a write.
            const auto after=io.nowMs();
            if(after<last){result.refusal=Refusal::Clock;return false;}
            last=after;
            if(after>=budget.strokeEnd){result.refusal=Refusal::Budget;return false;}
            return true;
        };
        const auto frames=strokeFrames();
        for(std::size_t index=0;index<frames.size();++index) {
            if(!permit())return refuse(result.refusal);
            const auto &f=frames[index];const auto expected=std::ptrdiff_t(f.count*KernelRecordBytes);
            if(!f.count || f.count>f.events.size())return refuse(Refusal::InvalidFrame);
            result.attempted=true;result.contactPossible=result.contactPossible||index>=2;++result.calls;
            const auto written=io.emit(f.events.data(),f.count);
            if(written.bytes<0 || written.error)return refuse(Refusal::WriteError);
            if(!written.bytes)return refuse(Refusal::WriteZero);
            if(written.bytes>expected)return refuse(Refusal::WriteOversize);
            if(written.bytes%std::ptrdiff_t(KernelRecordBytes)==0)result.acceptedRecords+=std::size_t(written.bytes)/KernelRecordBytes;
            if(written.bytes!=expected)return refuse(Refusal::WriteShort);
            ++result.frames;
            if(!permit())return refuse(result.refusal);
            if(!io.pauseMs(f.delayMs))return refuse(Refusal::Pause);
            if(!permit())return refuse(result.refusal);
        }
        result.complete=true;return result;
    } catch(...) {return refuse(Refusal::Exception);}
}
struct Release {
    // unknown is possible release-dispatch ambiguity, not "contact unresolved".
    // An armed release with observed=false never authorizes close/reopen, even
    // when no release write was attempted and unknown consequently stays false.
    bool attempted=false,written=false,observed=false,unknown=false;
    Refusal refusal=Refusal::None;
};
class Recovery final {
public:
    explicit Recovery(Budget budget):budget_(budget) {}
    Recovery(const Recovery&)=delete;Recovery &operator=(const Recovery&)=delete;
    Recovery(Recovery&&)=delete;Recovery &operator=(Recovery&&)=delete;
    // IO is the independently retained holder: identityValid(), released(),
    // observerEmpty(), nowMs(), emit(...), confirmReleased(originalDeadline).
    template<class IO> bool arm(IO &io) noexcept {
        if(armAttempted_ || spent_ || !budget_.valid())return false;
        armAttempted_=true; // Before every callout; failed/reentrant arming is not renewed.
        try {
            const auto now=io.nowMs();
            if(now<budget_.began || now>=budget_.strokeEnd || !io.identityValid() || !io.released() || !io.observerEmpty())return false;
            const auto after=io.nowMs();
            if(spent_ || after<now || after>=budget_.strokeEnd)return false;
            armed_=true;return true;
        }catch(...){return false;}
    }
    template<class IO> Release release(IO &io) noexcept {
        Release result;
        if(spent_){result.refusal=Refusal::Spent;return result;}
        spent_=true; // Reentry or exceptions can never grant a second release.
        if(!armed_){result.refusal=Refusal::NotArmed;return result;}
        try {
            auto now=io.nowMs();
            if(now<budget_.began || now>=budget_.recoveryEnd){result.refusal=Refusal::Budget;return result;}
            if(!io.identityValid()){result.refusal=Refusal::Device;return result;}
            const auto checked=io.nowMs();
            if(checked<now || checked>=budget_.recoveryEnd){result.refusal=Refusal::Budget;return result;}
            const auto f=releaseFrame();result.attempted=true;
            const auto written=io.emit(f.events.data(),f.count);
            if(written.bytes!=std::ptrdiff_t(f.count*KernelRecordBytes) || written.error){result.unknown=true;result.refusal=Refusal::WriteError;return result;}
            result.written=true;
            const auto afterEmit=io.nowMs();
            if(afterEmit<checked || afterEmit>=budget_.recoveryEnd){result.unknown=true;result.refusal=Refusal::Budget;return result;}
            result.observed=io.confirmReleased(budget_.recoveryEnd);
            now=io.nowMs();
            if(now<afterEmit || now>=budget_.recoveryEnd)result.observed=false;
            if(result.observed) {
                const bool identity=io.identityValid();
                const auto final=io.nowMs();
                if(!identity || final<now || final>=budget_.recoveryEnd)result.observed=false;
            }
            if(!result.observed){result.unknown=true;result.refusal=Refusal::ReleaseObservation;}
            return result;
        }catch(...){result.unknown=result.attempted;result.refusal=Refusal::Exception;return result;}
    }
private:
    const Budget budget_;bool armed_=false,spent_=false,armAttempted_=false;
};
} // namespace reader_fixed_pen
