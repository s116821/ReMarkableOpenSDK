#define RSDK_DEVELOPMENT_COMPOSER_V1
#define COMPOSER_ISSUER_TESTING
#define COMPOSER_DELIVERY_TESTING
#define RESIDENT_WORKER_SLOT_TESTING
#define RESIDENT_MUTATION_TESTING
#include "composer_issuer.h"
#include <cassert>
#include <iostream>

using Slot=reader_worker_native::ResidentSlot;
using Kind=reader_insertion_native_profile::NativeOwnerKind;
using Released=reader_insertion_native_profile::OwnerReleaseResult;
static unsigned releases=0;
static bool retainLast=false;
static Released releaseOwner(Kind kind,std::uintptr_t,std::uintptr_t) {
    ++releases;
    if(!retainLast)assert(kind==(releases==1?Kind::Worker:Kind::Document));
    return retainLast?Released::RefusedIdentity:Released::Released;
}
namespace reader_worker_native::testing {
struct SlotAccess {
    static Slot *make(){auto *s=new Slot({},nullptr);s->testRelease_=releaseOwner;return s;}
    static Slot::Token prepare(Slot &s,bool unknown) {
        auto v=std::make_shared<Slot::State>();v->preparing=false;v->ready=true;v->unknown=unknown;
        v->document.words={1,2};v->document.ownership=Slot::Ownership::Owned;
        v->worker.words={3,4};v->worker.ownership=Slot::Ownership::Owned;
        s.current_=v;return Slot::Token(v);
    }
    static void destroy(Slot *s){delete s;}
};
}
namespace remarkable::composer_scene::testing {
struct DeliveryAccess {
    static auto retirement(composer_text::Clock::time_point deadline){return SnapshotRetirement::createWriter(deadline,true);}
    static auto binding(const std::shared_ptr<SnapshotRetirement> &r){return r->binding_;}
};
}
namespace remarkable::composer_writer::testing {
struct IssuerAccess {
    static resident::Endpoint::Lease lease(quint8 kind) {
        resident::Frame frame;frame.message=2;frame.kind=kind;
        return std::shared_ptr<resident::RequestLease>(new resident::RequestLease(frame,30000));
    }
    static void run() {
        using Clock=composer_text::Clock;
        auto composer=lease(15),ordinary=lease(0);
        assert(composer->deadline()-Clock::now()<=std::chrono::seconds(20));
        assert(ordinary->deadline()-Clock::now()>std::chrono::seconds(29));
        for(unsigned mode=0;mode<4;++mode) {
            auto *slot=reader_worker_native::testing::SlotAccess::make();
            {
                Issuer issuer(slot);releases=0;retainLast=mode==3;
                auto visit=std::make_shared<Issuer::Visit>();issuer.visit_=visit;
                visit->revoked->store(true);issuer.retireIdleVisit();assert(!issuer.visit_);
                auto active=std::make_shared<Issuer::Active>();active->lease=lease(15);
                active->deadline=active->lease->deadline();active->visit=std::make_shared<Issuer::Visit>();
                active->awaiting=true;active->keyCaptured=true;
                active->retirement=composer_scene::testing::DeliveryAccess::retirement(active->deadline);
                issuer.active_=active;issuer.visit_=active->visit;
                if(mode) {
                    active->prepared=true;
                    active->preparedToken=reader_worker_native::testing::SlotAccess::prepare(*slot,mode==2);
                }
                {
                    const reader_request_scope::Scope frame(composer_scene::testing::DeliveryAccess::binding(active->retirement));
                    assert(frame.active());
                    // Captured key with no delivery (including expired/refused
                    // admission) must enter no-send retirement, never hang.
                    issuer.maintain();assert(active->finishing);
                    for(int i=0;i<5;++i){QCoreApplication::processEvents();issuer.maintain();}
                    assert(issuer.active_&&releases==0);
                }
                for(int i=0;i<20&&issuer.active_;++i){QCoreApplication::processEvents();issuer.maintain();}
                assert(!issuer.active_);
                if(mode<2){assert(!slot->occupied()&&!issuer.quarantined_);assert(releases==(mode?2U:0U));}
                else {assert(slot->occupied()&&issuer.quarantined_);assert(releases==(mode==2?0U:2U));}
                issuer.visit_->revoked->store(true);issuer.retireIdleVisit();assert(!issuer.visit_);
            }
            reader_worker_native::testing::SlotAccess::destroy(slot);
        }
    }
};
}
int main(int argc,char **argv) {
    QGuiApplication app(argc,argv);
    remarkable::composer_writer::testing::IssuerAccess::run();
    std::cout<<"PASS issuer original deadline, revoked visit, captured-key no-send, later-frame disposal and unknown/last quarantine\n";
}
