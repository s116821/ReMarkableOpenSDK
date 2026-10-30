// Original owned integer-address models. No native/private layout is emulated.
#include "reader_insertion_guard_predicate.h"
#include <cstdlib>
#include <new>
#include <iostream>
#include <stdexcept>
#include <thread>
#include <climits>

namespace { thread_local bool allocationWatch = false; thread_local unsigned allocations = 0; }
void *operator new(std::size_t n) {
    if (allocationWatch) ++allocations;
    if (void *p = std::malloc(n ? n : 1)) return p;
    throw std::bad_alloc();
}
void *operator new[](std::size_t n) { return ::operator new(n); }
void operator delete(void *p) noexcept { std::free(p); }
void operator delete[](void *p) noexcept { std::free(p); }
void operator delete(void *p, std::size_t) noexcept { std::free(p); }
void operator delete[](void *p, std::size_t) noexcept { std::free(p); }

namespace {
using namespace reader_insertion_predicate;
unsigned checks = 0;
void check(bool b, const char *label) { ++checks; if (!b) throw std::runtime_error(label); }
const std::array<QString, 16> ids{{
    QStringLiteral("10000000-0000-0000-0000-000000000000"),
    QStringLiteral("10000000-0000-0000-0000-000000000001"),
    QStringLiteral("10000000-0000-0000-0000-000000000002"),
    QStringLiteral("10000000-0000-0000-0000-000000000003"),
    QStringLiteral("10000000-0000-0000-0000-000000000004"),
    QStringLiteral("10000000-0000-0000-0000-000000000005"),
    QStringLiteral("10000000-0000-0000-0000-000000000006"),
    QStringLiteral("10000000-0000-0000-0000-000000000007"),
    QStringLiteral("10000000-0000-0000-0000-000000000008"),
    QStringLiteral("10000000-0000-0000-0000-000000000009"),
    QStringLiteral("10000000-0000-0000-0000-00000000000a"),
    QStringLiteral("10000000-0000-0000-0000-00000000000b"),
    QStringLiteral("10000000-0000-0000-0000-00000000000c"),
    QStringLiteral("10000000-0000-0000-0000-00000000000d"),
    QStringLiteral("10000000-0000-0000-0000-00000000000e"),
    QStringLiteral("10000000-0000-0000-0000-00000000000f")}};
const QString target = QStringLiteral("f0000000-0000-0000-0000-000000000001");
const QString collision = QStringLiteral("f0000000-0000-0000-0000-000000000002");
Config config(int source = 4, int wrong = 2, int count = 9) {
    return {source, wrong, count, ids[size_t(source)], ids[size_t(source + 1)], ids[size_t(wrong)], target};
}
const CallerAdmission admitted{true, true, true, true, true, true};
const Frame ordinary{0x1000, 0x10c8, 5, &target};
struct OwnedFailure {};
struct Model {
    Words initial{0x4004, 0x404c}, final = initial;
    std::array<QString, 16> values = ids;
    const Frozen *frozen = nullptr;
    Frame frame = ordinary;
    unsigned reads = 0, searches = 0, equalities = 0, recordComparisons = 0, stageAllocations = 0;
    std::array<int, 3> order{{-1, -1, -1}};
    bool interfaceCorrect = true;
    int failRead = 0, forceAt = -1, reenterAt = 0, throwAt = 0;
    Address forced = 0;
    bool nestedRefused = false;
};
thread_local Model *model = nullptr;
bool readWords(Address, Words &);
Address search(Address, const QString &);
bool equal(const QString &, const QString &);
const Calls calls{&readWords, &search, &equal};
void reenter(int point) {
    if (model->reenterAt != point) return;
    const Result nested = observe(*model->frozen, model->frame, calls, admitted);
    model->nestedRefused = nested.refusal == Refusal::Reentered && !nested.searches && !nested.wordReads;
}
bool readWords(Address word, Words &out) {
    Model &m = *model; ++m.reads;
    m.interfaceCorrect = m.interfaceCorrect && word == m.frame.map + 84;
    reenter(m.reads == 1 ? 1 : 4);
    if (m.throwAt == (m.reads == 1 ? 1 : 4)) throw OwnedFailure{};
    if (int(m.reads) == m.failRead) return false;
    out = m.reads == 1 ? m.initial : m.final; return true;
}
Address search(Address receiver, const QString &id) {
    Model &m = *model;
    const std::array<const QString *, 3> expected{{&m.frozen->realSource(), &m.frozen->rightNeighbor(), &m.frozen->wrongSource()}};
    const unsigned at = m.searches++;
    m.interfaceCorrect = m.interfaceCorrect && receiver == m.frame.map + 80 && at < 3 && &id == expected[at];
    if (at < 3) m.order[at] = int(at);
    reenter(2);
    if (m.throwAt == 2) throw OwnedFailure{};
    if (int(at) == m.forceAt) return m.forced;
    const unsigned n = (m.initial.end - m.initial.begin) / 8;
    for (unsigned i = 0; i != n; ++i) {
        ++m.recordComparisons;
        if (m.values[i] == id) return m.initial.begin + 8 * i;
    }
    return m.initial.end;
}
bool equal(const QString &a, const QString &b) {
    Model &m = *model; ++m.equalities;
    m.interfaceCorrect = m.interfaceCorrect && &a == m.frame.effectiveTarget && &b == &m.frozen->requestedTarget();
    reenter(3);
    if (m.throwAt == 3) throw OwnedFailure{};
    return a == b;
}
struct Stage {
    Model *previous; bool previousWatch; unsigned before; Model &current;
    explicit Stage(Model &m) : previous(model), previousWatch(allocationWatch), before(allocations), current(m) {
        model = &m; allocationWatch = true;
    }
    ~Stage() { current.stageAllocations = allocations - before; allocationWatch = previousWatch; model = previous; }
};
Result run(Model &m, const Frozen &f, Frame frame = ordinary, Calls c = calls, CallerAdmission a = admitted) {
    m.frozen = &f; m.frame = frame;
    Stage stage(m); return observe(f, frame, c, a);
}
void noSearch(const Frozen &f, Model m, Frame frame, Refusal refusal) {
    const auto r = run(m, f, frame);
    check(r.refusal == refusal && !r.observedMismatch, "pre-search refusal classification");
    check(!m.searches && !m.equalities && !m.stageAllocations, "pre-search control calls no search/equality/new");
    check(!r.insertionPermission && !r.nativeAuthority, "negative result has no authority");
}
}
int main() {
    try {
        auto freezing = freeze(config()); check(bool(freezing.frozen), "valid frozen tuple");
        const Frozen &f = *freezing.frozen;
        const auto data = std::array<const QChar *, 4>{{f.realSource().constData(), f.rightNeighbor().constData(), f.wrongSource().constData(), f.requestedTarget().constData()}};
        Model m; auto r = run(m, f);
        check(r.observedMismatch && r.refusal == Refusal::None, "owned mismatch witness");
        check(r.realAtSource && r.rightAdjacent && r.wrongAtFrozenPosition && r.targetMatches && r.spanUnchanged, "all scalar evidence present");
        check(r.positions == std::array<int,3>{{4,5,2}} && r.count == 9, "same span exact positions");
        check(m.reads == 2 && m.searches == 3 && m.equalities == 1 && m.interfaceCorrect, "exact injected argument sequence");
        check(m.order == std::array<int,3>{{0,1,2}} && m.recordComparisons <= 48, "only frozen searches bounded");
        check(m.stageAllocations == 0 && !r.insertionPermission && !r.nativeAuthority, "positive mismatch is passive and never insertion permission");
        check(data == std::array<const QChar *,4>{{f.realSource().constData(),f.rightNeighbor().constData(),f.wrongSource().constData(),f.requestedTarget().constData()}}, "frozen buffers not detached");
        for (int n : {-1,0,1,17,INT_MAX}) { auto c=config();c.pageCount=n;check(!freeze(c).frozen,"invalid frozen count"); }
        for (int n : {INT_MIN,-1,8,9,INT_MAX}) { auto c=config();c.sourceIndex=n;check(!freeze(c).frozen,"invalid source range"); }
        for (int n : {INT_MIN,-1,4,9,INT_MAX}) { auto c=config();c.wrongIndex=n;check(!freeze(c).frozen,"invalid wrong source range"); }
        for (int role=0;role!=4;++role) for (const QString &bad : {QString(),QStringLiteral("bad"),QStringLiteral("10000000_0000-0000-0000-000000000001"),QStringLiteral("10000000-0000-0000-0000-00000000000A"),QStringLiteral("10000000-0000-0000-0000-00000000000\u0100")}) {
            auto c=config();std::array<QString *,4> fields{{&c.realSource,&c.rightNeighbor,&c.wrongSource,&c.requestedTarget}};
            *fields[size_t(role)]=bad;check(freeze(c).refusal==Refusal::InputId,"freeze validates all canonical values");
        }
        for(int kind=0;kind!=6;++kind) {
            auto c=config();
            if(kind==0)c.wrongSource=c.realSource;
            if(kind==1)c.rightNeighbor=c.realSource;
            if(kind==2)c.wrongSource=c.rightNeighbor;
            if(kind==3)c.requestedTarget=c.realSource;
            if(kind==4)c.requestedTarget=c.rightNeighbor;
            if(kind==5)c.requestedTarget=c.wrongSource;
            check(freeze(c).refusal==Refusal::InputCorrespondence,"freeze correspondence and fresh target");
        }
        auto sameRight=freeze(config(4,5));check(bool(sameRight.frozen),"wrong equals right allowed at right position");
        m=Model{};r=run(m,*sameRight.frozen);check(r.observedMismatch && r.positions==std::array<int,3>{{4,5,5}},"same right/wrong still exactly three searches");
        auto wrongAtRight=config();wrongAtRight.wrongIndex=5;check(!freeze(wrongAtRight).frozen,"right position with unequal wrong ID refused");
        for(unsigned i=0;i!=6;++i) {
            CallerAdmission a=admitted;
            std::array<bool *,6> flags{{&a.profileAndProviders,&a.originalCallsite,&a.operationScope,&a.workerThread,&a.nativeOwners,&a.finalLock}};
            *flags[i]=false;m=Model{};r=run(m,f,ordinary,calls,a);
            check(r.refusal==Refusal::Admission && !m.reads && !m.searches && !m.equalities,"every caller admission is required before reads");
        }
        for(int i=0;i!=4;++i) {
            Calls c=calls;Frame frame=ordinary;
            if(i==0)c.readWords=nullptr;
            if(i==1)c.search=nullptr;
            if(i==2)c.equal=nullptr;
            if(i==3)frame.effectiveTarget=nullptr;
            m=Model{};r=run(m,f,frame,c);check(r.refusal==Refusal::MissingCall && !m.reads,"missing calls/target no reads");
        }
        for(const Frame &frame : {Frame{0,0,5,&target},Frame{0x1001,0x10c9,5,&target},Frame{0x1000,0,5,&target},Frame{0x1000,0x10cc,5,&target},Frame{0xffffff80,0x48,5,&target},Frame{0xffffff00,0xffffffc8,5,&target}})
            noSearch(f,Model{},frame,Refusal::AddressRange);
        for(int index : {-1,4,6,9,INT_MIN,INT_MAX}) { auto frame=ordinary;frame.effectiveIndex=index;noSearch(f,Model{},frame,Refusal::EffectiveIndex); }
        m=Model{};m.failRead=1;noSearch(f,m,ordinary,Refusal::InitialRead);
        for(const Words &words : {Words{0,0},Words{0x4004,0},Words{0x404c,0x4004},Words{0x4004,0x4004},Words{0x4005,0x404d},Words{0x4004,0x404d},Words{0x4004,0x4008},Words{0x4004,0x408c},Words{0xfffffffc,0x44}}) {
            m=Model{};m.initial=words;noSearch(f,m,ordinary,Refusal::Span);
        }
        for(Address span : {8u,64u,128u}) {m=Model{};m.initial.end=m.initial.begin+span;noSearch(f,m,ordinary,Refusal::Count);}
        auto top=freeze(config(14,15,16));check(bool(top.frozen),"upper count and source valid");
        m=Model{};m.initial.end=m.initial.begin+128;m.final=m.initial;auto frame=ordinary;frame.effectiveIndex=15;
        r=run(m,*top.frozen,frame);check(r.observedMismatch && m.searches==3 && m.recordComparisons==47,"cap16 bounds worst modeled fixed searches");
        check(m.equalities==1 && !m.stageAllocations,"cap16 no new calls/allocation");
        m=Model{};m.initial={0xffffffb4,0xfffffffc};m.final=m.initial;r=run(m,f);
        check(r.observedMismatch && r.positions==std::array<int,3>{{4,5,2}},"near uint32 end valid checked span");
        for(int at=0;at!=3;++at) for(int fault=0;fault!=5;++fault) {
            m=Model{};m.forceAt=at;
            m.forced=fault==0?m.initial.end:fault==1?m.initial.begin-8:fault==2?m.initial.end+8:fault==3?m.initial.begin+4:m.initial.begin;
            r=run(m,f);
            const Refusal expected=fault==0?Refusal::SearchMissing:fault==4?Refusal::SearchPosition:Refusal::SearchPointer;
            check(r.refusal==expected && !r.observedMismatch,"returned pointer refusal");
            check(m.searches==unsigned(at+1) && !m.equalities && m.reads==1 && !m.stageAllocations,"returned fault stops without following pointer");
        }
        for(int position : {4,5,2}) {
            m=Model{};m.values[size_t(position)]=collision;r=run(m,f);
            check(r.refusal==Refusal::SearchMissing && !r.observedMismatch && !m.equalities,"missing frozen subject stops without substitute");
        }
        m=Model{};std::swap(m.values[4],m.values[2]);r=run(m,f);
        check(r.refusal==Refusal::SearchPosition && !r.realAtSource && !r.observedMismatch,"unexpected wrong source at real index not positive");
        m=Model{};frame=ordinary;frame.effectiveTarget=&collision;r=run(m,f,frame);
        check(r.refusal==Refusal::TargetMismatch && r.searches==3 && r.equalities==1 && r.wordReads==1 && !r.observedMismatch,"generated target mismatch no final/read grant");
        for(int which=0;which!=3;++which) {
            m=Model{};
            if(which==0)m.failRead=2;
            if(which==1)m.final.begin+=8;
            if(which==2)m.final.end+=8;
            r=run(m,f);check(r.refusal==(which==0?Refusal::FinalRead:Refusal::SpanChanged) && !r.observedMismatch,"late span failure cannot be positive");
            check(m.reads==2 && m.searches==3 && m.equalities==1 && !m.stageAllocations,"late failure has fixed bounded call count");
        }
        for(int at=1;at!=5;++at) {
            m=Model{};m.reenterAt=at;r=run(m,f);
            check(r.refusal==Refusal::Reentered && m.nestedRefused && !r.observedMismatch,"reentry invalidates outer read");
            check(!m.stageAllocations && m.searches<=3 && m.equalities<=1,"reentry stays bounded");
            m=Model{};r=run(m,f);check(r.observedMismatch,"reentry gate resets for separate owned control");
        }
        for(int at=1;at!=5;++at) {
            m=Model{};m.throwAt=at;bool propagated=false;
            try { (void)run(m,f); } catch(const OwnedFailure &) { propagated=true; }
            check(propagated,"owned unexpected exception propagates not false refusal");
            m=Model{};r=run(m,f);check(r.observedMismatch,"plain RAII read gate resets after owned throw");
        }
        m=Model{};std::thread worker([&]{r=run(m,f);});worker.join();
        check(r.observedMismatch && !m.stageAllocations && m.interfaceCorrect,"actual owned thread uses passive model interfaces");
        std::cout << "{\"passed\":true,\"checks\":" << checks << ",\"max_searches\":3,\"max_records\":16,\"native_calls\":0,\"insertion_permission\":false}\n";
        return 0;
    } catch(const std::exception &e) { std::cerr << "owned predicate failure: " << e.what() << '\n'; return 1; }
}
