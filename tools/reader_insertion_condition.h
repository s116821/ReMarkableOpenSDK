#pragma once
// Research-only passive facts. No native resolver, hook, ownership or forwarding.
#include <QString>
#include <cstdint>
#include <limits>
#include <memory>

namespace reader_insertion_condition {
using Address=std::uint32_t;
constexpr unsigned MaxActiveRecords=16,MaxTreeNodes=64;
constexpr Address RecordStride=8;
enum class Refusal {
    None,InputRange,InputId,InputCorrespondence,Admission,MissingCall,
    AddressRange,EffectiveIndex,InitialRead,Span,Count,SearchMissing,
    SearchPointer,SearchPosition,TargetMismatch,InitialTreeRead,TreeHeader,
    FindResult,TargetPresent,FinalRead,SpanChanged,FinalTreeRead,TreeChanged,Reentered
};
struct Config {
    int sourceIndex=-1,pageCount=-1;
    QString source,target,right;
};
class Frozen;
struct Freezing {std::unique_ptr<const Frozen> frozen;Refusal refusal=Refusal::None;};
Freezing freeze(const Config &);
class Frozen final {
    int source_,count_;
    QString sourceId_,targetId_,rightId_;
    explicit Frozen(const Config &c):source_(c.sourceIndex),count_(c.pageCount),sourceId_(c.source),targetId_(c.target),rightId_(c.right) {}
    friend Freezing freeze(const Config &);
public:
    int sourceIndex() const{return source_;}
    int pageCount() const{return count_;}
    const QString &source() const{return sourceId_;}
    const QString &target() const{return targetId_;}
    const QString &right() const{return rightId_;}
    bool hasRight() const{return source_+1<count_;}
};
namespace detail {
inline bool canonical(const QString &id) {
    if(id.size()!=36)return false;
    for(int i=0;i!=36;++i) {
        const auto c=id.at(i).unicode();const bool dash=i==8||i==13||i==18||i==23;
        if(dash?c!='-':!((c>='0'&&c<='9')||(c>='a'&&c<='f')))return false;
    }
    return true;
}
inline bool add(Address a,Address b,Address &out) {
    if(a>std::numeric_limits<Address>::max()-b)return false;
    out=a+b;return true;
}
inline bool aligned(Address a){return a!=0&&a%4==0;}
inline thread_local bool reading=false,reentered=false;
struct Gate {
    bool entered=false;
    Gate(){if(reading)reentered=true;else{reading=true;reentered=false;entered=true;}}
    ~Gate(){if(entered)reading=false;}
};
}
inline Freezing freeze(const Config &c) {
    if(c.pageCount<1||c.pageCount>int(MaxActiveRecords)||c.sourceIndex<0||c.sourceIndex>=c.pageCount)
        return {nullptr,Refusal::InputRange};
    const bool middle=c.sourceIndex+1<c.pageCount;
    if(!detail::canonical(c.source)||!detail::canonical(c.target)||
       (middle?!detail::canonical(c.right):!c.right.isEmpty()))return {nullptr,Refusal::InputId};
    if(c.source==c.target||(middle&&(c.right==c.source||c.right==c.target)))return {nullptr,Refusal::InputCorrespondence};
    return {std::unique_ptr<const Frozen>(new Frozen(c)),Refusal::None};
}
// These remain assertions supplied by the separately qualified caller.
struct CallerAdmission {
    bool profileAndProviders=false,originalCallsite=false,operationScope=false;
    bool workerThread=false,nativeOwners=false,finalLock=false;
};
struct Words {Address begin=0,end=0;};
// Owned output carrier; NEVER reinterpret it as the noncontiguous native fields.
struct Tree {Address root=0;std::uint32_t count=0;};
struct Calls {
    bool (*readWords)(Address firstWord,Words &)=nullptr;
    Address (*search)(Address orderedReceiver,const QString &)=nullptr;
    bool (*equal)(const QString &,const QString &)=nullptr;
    // treeReceiver is map+96; qualified reader fills root from +8,count from +20.
    bool (*readTree)(Address treeReceiver,Tree &)=nullptr;
    Address (*find)(Address treeReceiver,const QString &)=nullptr;
};
struct Frame {Address worker=0,map=0;int effectiveIndex=-1;const QString *effectiveTarget=nullptr;};
struct Result {
    bool matched=false;
    Refusal refusal=Refusal::None;
    unsigned wordReads=0,treeReads=0,searches=0,equalities=0,finds=0,count=0,treeCount=0;
    int sourcePosition=-1,rightPosition=-1;
    bool sourceAtIndex=false,rightAdjacent=false,endSource=false,targetMatches=false;
    bool targetAbsent=false,spanUnchanged=false,treeUnchanged=false;
    static constexpr bool nativeAuthority=false,insertionPermission=false;
};
// No QString construction/copy/detach, allocation, GUI, I/O or locking here.
// Native callback exceptions propagate; this does not recover corruption/unwind faults.
inline Result observe(const Frozen &f,const Frame &frame,const Calls &calls,const CallerAdmission &a) {
    Result result;detail::Gate gate;
    if(!gate.entered){result.refusal=Refusal::Reentered;return result;}
    auto fail=[&](Refusal why){result.refusal=why;return result;};
    if(!a.profileAndProviders||!a.originalCallsite||!a.operationScope||!a.workerThread||!a.nativeOwners||!a.finalLock)
        return fail(Refusal::Admission);
    if(!calls.readWords||!calls.search||!calls.equal||!calls.readTree||!calls.find||!frame.effectiveTarget)
        return fail(Refusal::MissingCall);
    Address map=0,receiver=0,first=0,spanEnd=0,tree=0,sentinel=0,rootField=0,countField=0,headerEnd=0;
    if(!detail::aligned(frame.worker)||!detail::aligned(frame.map)||!detail::add(frame.worker,200,map)||map!=frame.map||
       !detail::add(map,80,receiver)||!detail::add(map,84,first)||!detail::add(map,92,spanEnd)||
       !detail::add(map,96,tree)||!detail::add(map,100,sentinel)||!detail::add(map,104,rootField)||
       !detail::add(map,116,countField)||!detail::add(map,120,headerEnd))return fail(Refusal::AddressRange);
    (void)spanEnd;(void)rootField;(void)countField;(void)headerEnd;
    if(frame.effectiveIndex!=f.sourceIndex()+1)return fail(Refusal::EffectiveIndex);
    Words before;++result.wordReads;
    if(!calls.readWords(first,before))return fail(Refusal::InitialRead);
    if(detail::reentered)return fail(Refusal::Reentered);
    if(!detail::aligned(before.begin)||!detail::aligned(before.end)||before.end<=before.begin||
       before.end-before.begin>MaxActiveRecords*RecordStride||(before.end-before.begin)%RecordStride)
        return fail(Refusal::Span);
    result.count=(before.end-before.begin)/RecordStride;
    if(result.count!=unsigned(f.pageCount()))return fail(Refusal::Count);
    Address expected=0;
    if(!detail::add(before.begin,Address(f.sourceIndex())*RecordStride,expected)||expected>=before.end)return fail(Refusal::Count);
    ++result.searches;const Address source=calls.search(receiver,f.source());
    if(detail::reentered)return fail(Refusal::Reentered);
    if(source<before.begin||source>before.end||(source-before.begin)%RecordStride)return fail(Refusal::SearchPointer);
    if(source==before.end)return fail(Refusal::SearchMissing);
    result.sourcePosition=int((source-before.begin)/RecordStride);
    if(source!=expected)return fail(Refusal::SearchPosition);
    result.sourceAtIndex=true;
    if(f.hasRight()) {
        if(!detail::add(expected,RecordStride,expected)||expected>=before.end)return fail(Refusal::Count);
        ++result.searches;const Address right=calls.search(receiver,f.right());
        if(detail::reentered)return fail(Refusal::Reentered);
        if(right<before.begin||right>before.end||(right-before.begin)%RecordStride)return fail(Refusal::SearchPointer);
        if(right==before.end)return fail(Refusal::SearchMissing);
        result.rightPosition=int((right-before.begin)/RecordStride);
        if(right!=expected)return fail(Refusal::SearchPosition);
        result.rightAdjacent=true;
    } else {
        result.endSource=frame.effectiveIndex==int(result.count);
        if(!result.endSource)return fail(Refusal::EffectiveIndex);
    }
    ++result.equalities;result.targetMatches=calls.equal(*frame.effectiveTarget,f.target());
    if(detail::reentered)return fail(Refusal::Reentered);
    if(!result.targetMatches)return fail(Refusal::TargetMismatch);
    Tree beforeTree;++result.treeReads;
    if(!calls.readTree(tree,beforeTree))return fail(Refusal::InitialTreeRead);
    if(detail::reentered)return fail(Refusal::Reentered);
    result.treeCount=beforeTree.count;
    if(!detail::aligned(beforeTree.root)||beforeTree.root==sentinel||beforeTree.count<result.count||beforeTree.count>MaxTreeNodes)
        return fail(Refusal::TreeHeader);
    ++result.finds;const Address found=calls.find(tree,f.target());
    if(detail::reentered)return fail(Refusal::Reentered);
    if(found!=sentinel)return fail(!detail::aligned(found)?Refusal::FindResult:Refusal::TargetPresent);
    result.targetAbsent=true;
    Words after;++result.wordReads;
    if(!calls.readWords(first,after))return fail(Refusal::FinalRead);
    if(detail::reentered)return fail(Refusal::Reentered);
    if(before.begin!=after.begin||before.end!=after.end)return fail(Refusal::SpanChanged);
    result.spanUnchanged=true;
    Tree afterTree;++result.treeReads;
    if(!calls.readTree(tree,afterTree))return fail(Refusal::FinalTreeRead);
    if(detail::reentered)return fail(Refusal::Reentered);
    if(beforeTree.root!=afterTree.root||beforeTree.count!=afterTree.count)return fail(Refusal::TreeChanged);
    result.treeUnchanged=true;result.matched=true;return result;
}
}
