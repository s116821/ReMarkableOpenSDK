// Original address/QString models, not a native tree or private ABI emulator.
#include "reader_insertion_condition.h"
#include <array>
#include <climits>
#include <cstdio>
#include <cstdlib>
#include <new>
#include <thread>

namespace {thread_local bool watch=false;thread_local unsigned news=0;}
void *operator new(std::size_t n){if(watch)++news;if(void *p=std::malloc(n?n:1))return p;throw std::bad_alloc();}
void *operator new[](std::size_t n){return ::operator new(n);}
void operator delete(void *p) noexcept{std::free(p);}
void operator delete[](void *p) noexcept{std::free(p);}
void operator delete(void *p,std::size_t) noexcept{std::free(p);}
void operator delete[](void *p,std::size_t) noexcept{std::free(p);}
namespace {
using namespace reader_insertion_condition;
unsigned checks=0,scenarios=0;
void check(bool yes,const char *what){++checks;if(!yes){std::fprintf(stderr,"FAIL %s\n",what);std::abort();}}
const std::array<QString,16> ids=[] {
    std::array<QString,16> value;
    for(unsigned i=0;i!=value.size();++i)value[i]=QStringLiteral("10000000-0000-0000-0000-%1").arg(i,12,16,QLatin1Char('0'));
    return value;
}();
const QString target=QStringLiteral("20000000-0000-0000-0000-000000000099");
const QString other=QStringLiteral("30000000-0000-0000-0000-000000000099");
const CallerAdmission admitted{true,true,true,true,true,true};
Config config(int source=4,int count=9){return {source,count,ids[size_t(source)],target,source+1<count?ids[size_t(source+1)]:QString()};}
struct Fault {};
struct Model {
    Words before{0x4004,0x404c},after=before;
    Tree firstTree{0x8000,9},lastTree=firstTree;
    std::array<QString,16> active=ids;
    std::array<QString,64> treeIds=[] {std::array<QString,64> values;for(unsigned i=0;i!=ids.size();++i)values[i]=ids[i];return values;}();
    const Frozen *frozen=nullptr;
    Frame frame;
    unsigned reads=0,treeReads=0,searches=0,equalities=0,finds=0,comparisons=0,treeComparisons=0,allocations=0;
    bool interfaces=true,nestedRefused=false;
    int readFailure=0,treeFailure=0,forceSearch=-1,reentry=0,throws=0;
    Address forcedSearch=0,forcedFind=0;
    bool forceFind=false;
};
thread_local Model *current=nullptr;
bool words(Address,Words&);Address search(Address,const QString&);bool equal(const QString&,const QString&);bool tree(Address,Tree&);Address find(Address,const QString&);
const Calls calls{words,search,equal,tree,find};
void point(int at) {
    Model &m=*current;
    if(m.reentry==at) {
        const auto r=observe(*m.frozen,m.frame,calls,admitted);
        m.nestedRefused=r.refusal==Refusal::Reentered&&!r.matched&&!r.wordReads&&!r.searches&&!r.treeReads&&!r.finds;
    }
    if(m.throws==at)throw Fault{};
}
bool words(Address receiver,Words &out) {
    auto &m=*current;++m.reads;m.interfaces=m.interfaces&&receiver==m.frame.map+84;
    point(m.reads==1?1:7);
    if(int(m.reads)==m.readFailure)return false;
    out=m.reads==1?m.before:m.after;return true;
}
Address search(Address receiver,const QString &id) {
    auto &m=*current;const unsigned at=m.searches++;
    m.interfaces=m.interfaces&&receiver==m.frame.map+80&&at<2&&&id==(at==0?&m.frozen->source():&m.frozen->right());
    point(at==0?2:3);
    if(int(at)==m.forceSearch)return m.forcedSearch;
    const unsigned count=(m.before.end-m.before.begin)/8;
    for(unsigned i=0;i<count;++i){++m.comparisons;if(m.active[i]==id)return m.before.begin+i*8;}
    return m.before.end;
}
bool equal(const QString &left,const QString &right) {
    auto &m=*current;++m.equalities;
    m.interfaces=m.interfaces&&&left==m.frame.effectiveTarget&&&right==&m.frozen->target();point(4);
    return left==right;
}
bool tree(Address receiver,Tree &out) {
    auto &m=*current;++m.treeReads;m.interfaces=m.interfaces&&receiver==m.frame.map+96;
    point(m.treeReads==1?5:8);
    if(int(m.treeReads)==m.treeFailure)return false;
    // Explicit owned result assignment. No native-header casting or memcpy.
    out=m.treeReads==1?m.firstTree:m.lastTree;return true;
}
Address find(Address receiver,const QString &id) {
    auto &m=*current;++m.finds;m.interfaces=m.interfaces&&receiver==m.frame.map+96&&&id==&m.frozen->target();point(6);
    if(m.forceFind)return m.forcedFind;
    for(unsigned i=0;i<m.firstTree.count;++i){++m.treeComparisons;if(m.treeIds[i]==id)return m.firstTree.root;}
    return receiver+4;
}
Result run(Model &m,const Frozen &f,Frame frame,const Calls &c=calls,const CallerAdmission &a=admitted) {
    m.frozen=&f;m.frame=frame;Model *old=current;const bool oldWatch=watch;const unsigned before=news;
    current=&m;watch=true;
    struct Restore {Model &m;Model *old;bool watched;unsigned before;~Restore(){m.allocations=news-before;watch=watched;current=old;}} restore{m,old,oldWatch,before};
    ++scenarios;return observe(f,frame,c,a);
}
Frame frameFor(const Frozen &f){return {0x1000,0x10c8,f.sourceIndex()+1,&target};}
Result run(Model &m,const Frozen &f){return run(m,f,frameFor(f));}
void refused(const Result &r,const Model &m,Refusal why) {
    check(!r.matched&&r.refusal==why,"specific fail-closed refusal");
    check(m.interfaces&&!m.allocations,"borrowed interfaces and no instrumented C++ allocation");
    check(r.wordReads==m.reads&&r.treeReads==m.treeReads&&r.searches==m.searches&&r.equalities==m.equalities&&r.finds==m.finds,"exact callback counters");
    check(m.reads<=2&&m.treeReads<=2&&m.searches<=2&&m.equalities<=1&&m.finds<=1,"bounded fixed callback set");
    check(!r.nativeAuthority&&!r.insertionPermission,"facts never confer authority or permission");
}
}
int main() {
    auto freezeResult=freeze(config());check(bool(freezeResult.frozen),"valid middle freeze");const Frozen &f=*freezeResult.frozen;
    const std::array<const QChar*,3> frozenData{{f.source().constData(),f.target().constData(),f.right().constData()}};
    for(auto input:{config(),config(0,1),config(8,9),config(0,2),config(14,16),config(15,16)}) {
        auto frozen=freeze(input);check(bool(frozen.frozen),"middle/end/single/cap freeze");Model m;
        m.before.end=m.before.begin+unsigned(input.pageCount)*8;m.after=m.before;
        m.firstTree.count=unsigned(input.pageCount);m.lastTree=m.firstTree;
        const auto r=run(m,*frozen.frozen);
        check(r.matched&&r.refusal==Refusal::None&&r.sourceAtIndex&&r.targetMatches&&r.targetAbsent&&r.spanUnchanged&&r.treeUnchanged,"all passive condition facts present");
        check(r.sourcePosition==input.sourceIndex&&r.count==unsigned(input.pageCount)&&r.treeCount==unsigned(input.pageCount),"exact source/count facts");
        check(input.right.isEmpty()?r.endSource&&!r.rightAdjacent&&r.rightPosition==-1:r.rightAdjacent&&!r.endSource&&r.rightPosition==input.sourceIndex+1,"middle and end evidence distinct");
        check(m.searches==unsigned(input.right.isEmpty()?1:2)&&m.reads==2&&m.treeReads==2&&m.equalities==1&&m.finds==1,"exact fixed reads and no right lookup at end");
        check(m.interfaces&&!m.allocations&&m.comparisons<=32&&m.treeComparisons<=64&&!r.nativeAuthority&&!r.insertionPermission,"normal models bounded/passive/no grant");
    }
    for(int n:{INT_MIN,-1,0,17,INT_MAX}){auto c=config();c.pageCount=n;check(!freeze(c).frozen,"bad frozen count");}
    for(int n:{INT_MIN,-1,9,INT_MAX}){auto c=config();c.sourceIndex=n;check(!freeze(c).frozen,"bad frozen index");}
    for(int field=0;field<3;++field)for(const QString &bad:{QString(),QStringLiteral("bad"),QStringLiteral("10000000_0000-0000-0000-000000000000"),QStringLiteral("10000000-0000-0000-0000-00000000000A"),QStringLiteral("10000000-0000-0000-0000-00000000000\u0100")}) {
        auto c=config();QString *fields[]={&c.source,&c.target,&c.right};*fields[field]=bad;
        check(freeze(c).refusal==Refusal::InputId,"canonical frozen IDs required");
    }
    for(int field=0;field<3;++field){auto c=config();if(field==0)c.source=c.target;if(field==1)c.right=c.source;if(field==2)c.right=c.target;check(freeze(c).refusal==Refusal::InputCorrespondence,"frozen IDs distinct");}
    {auto c=config(8,9);c.right=ids[0];check(!freeze(c).frozen,"last source must omit right");}
    {auto c=config();c.right.clear();check(!freeze(c).frozen,"middle source requires right");}
    for(int i=0;i<6;++i) {
        auto a=admitted;bool *flags[]={&a.profileAndProviders,&a.originalCallsite,&a.operationScope,&a.workerThread,&a.nativeOwners,&a.finalLock};*flags[i]=false;
        Model m;auto r=run(m,f,frameFor(f),calls,a);refused(r,m,Refusal::Admission);check(!m.reads,"admission before callback");
    }
    for(int i=0;i<6;++i) {
        auto c=calls;auto frame=frameFor(f);if(i==0)c.readWords=nullptr;if(i==1)c.search=nullptr;if(i==2)c.equal=nullptr;if(i==3)c.readTree=nullptr;if(i==4)c.find=nullptr;if(i==5)frame.effectiveTarget=nullptr;
        Model m;auto r=run(m,f,frame,c);refused(r,m,Refusal::MissingCall);check(!m.reads,"missing callback before stage");
    }
    for(const Frame &frame:{Frame{0,0,5,&target},Frame{0x1001,0x10c9,5,&target},Frame{0x1000,0x10cc,5,&target},Frame{0xffffff80,0x48,5,&target},Frame{0xffffff00,0xffffffc8,5,&target},Frame{0xfffffecc,0xffffff94,5,&target}}) {
        Model m;auto r=run(m,f,frame);refused(r,m,Refusal::AddressRange);check(!m.reads,"overflow/address faults before any callbacks");
    }
    for(int index:{INT_MIN,-1,4,6,9,INT_MAX}){Model m;auto frame=frameFor(f);frame.effectiveIndex=index;auto r=run(m,f,frame);refused(r,m,Refusal::EffectiveIndex);check(!m.reads,"bad effective index no probing");}
    for(Words span:{Words{0,0},Words{0x4004,0},Words{0x404c,0x4004},Words{0x4004,0x4004},Words{0x4005,0x404d},Words{0x4004,0x404d},Words{0x4004,0x4008},Words{0x4004,0x408c},Words{0xfffffffc,0x44}}) {
        Model m;m.before=span;auto r=run(m,f);refused(r,m,Refusal::Span);check(!m.searches&&!m.finds,"malformed span cannot search");
    }
    for(Address bytes:{8u,64u,128u}){Model m;m.before.end=m.before.begin+bytes;auto r=run(m,f);refused(r,m,Refusal::Count);check(!m.searches&&!m.finds,"count mismatch before searches");}
    for(int at=0;at<2;++at)for(int fault=0;fault<5;++fault) {
        Model m;m.forceSearch=at;m.forcedSearch=fault==0?m.before.end:fault==1?m.before.begin-8:fault==2?m.before.end+8:fault==3?m.before.begin+4:m.before.begin;
        auto r=run(m,f);refused(r,m,fault==0?Refusal::SearchMissing:fault==4?Refusal::SearchPosition:Refusal::SearchPointer);
        check(m.searches==unsigned(at+1)&&!m.equalities&&!m.treeReads&&!m.finds,"bad result never followed or substituted");
    }
    for(int position:{4,5}){Model m;m.active[size_t(position)]=other;auto r=run(m,f);refused(r,m,Refusal::SearchMissing);}
    {Model m;std::swap(m.active[4],m.active[1]);auto r=run(m,f);refused(r,m,Refusal::SearchPosition);}
    {Model m;auto frame=frameFor(f);frame.effectiveTarget=&other;auto r=run(m,f,frame);refused(r,m,Refusal::TargetMismatch);check(!m.treeReads&&!m.finds,"generated substituted target never enters tree lookup");}
    for(Tree t:{Tree{0,9},Tree{0x8001,9},Tree{0x112c,9},Tree{0x8000,0},Tree{0x8000,8},Tree{0x8000,65},Tree{0x8000,UINT32_MAX}}) {
        Model m;m.firstTree=t;auto r=run(m,f);refused(r,m,Refusal::TreeHeader);check(m.treeReads==1&&!m.finds,"bad or overcap actual tree header blocks find");
    }
    {Model m;m.firstTree.count=64;m.lastTree=m.firstTree;auto r=run(m,f);check(r.matched&&m.treeComparisons==64,"tree bound independent from active count");}
    {Model m;m.firstTree.count=12;m.lastTree=m.firstTree;m.treeIds[11]=target;auto r=run(m,f);refused(r,m,Refusal::TargetPresent);check(m.searches==2&&m.finds==1&&!r.targetAbsent,"hidden nonactive target collision refused");}
    for(Address found:{0u,1u,0x8001u,0x8000u,0x9000u}){Model m;m.forceFind=true;m.forcedFind=found;auto r=run(m,f);refused(r,m,found==0||found%4?Refusal::FindResult:Refusal::TargetPresent);}
    for(int which=0;which<2;++which){Model m;if(which==0)m.after.begin+=8;else m.after.end+=8;auto r=run(m,f);refused(r,m,Refusal::SpanChanged);check(m.treeReads==1,"changed span stops before final tree call");}
    for(int which=0;which<2;++which){Model m;if(which==0)m.lastTree.root+=4;else ++m.lastTree.count;auto r=run(m,f);refused(r,m,Refusal::TreeChanged);}
    for(int at:{1,2}){Model m;m.readFailure=at;auto r=run(m,f);refused(r,m,at==1?Refusal::InitialRead:Refusal::FinalRead);}
    for(int at:{1,2}){Model m;m.treeFailure=at;auto r=run(m,f);refused(r,m,at==1?Refusal::InitialTreeRead:Refusal::FinalTreeRead);}
    for(int at=1;at<=8;++at){Model m;m.reentry=at;auto r=run(m,f);refused(r,m,Refusal::Reentered);check(m.nestedRefused,"reentry stops nested reads and refuses outer facts");}
    for(int at=1;at<=8;++at){Model m;m.throws=at;bool thrown=false;try{run(m,f);}catch(const Fault&){thrown=true;}check(thrown,"unrelated owned callback exception propagates");Model after;check(run(after,f).matched,"exception unwinds gate without poisoning next independent read");}
    {Model m;m.before={0xffffffb4,0xfffffffc};m.after=m.before;auto r=run(m,f);check(r.matched&&r.sourcePosition==4,"checked near-uint32-limit active span");}
    {Model m;auto frame=frameFor(f);frame.worker=0xfffffe00;frame.map=0xfffffec8;auto r=run(m,f,frame);check(r.matched&&m.interfaces,"checked high receiver arithmetic remains usable");}
    check(frozenData==std::array<const QChar*,3>{{f.source().constData(),f.target().constData(),f.right().constData()}},"frozen string buffers unchanged and not detached");
    bool threadPassed=false;std::thread separate([&]{Model m;threadPassed=run(m,f).matched&&!m.allocations;});separate.join();check(threadPassed,"independent thread-local guard and original owned model");
    std::printf("PASS owned passive condition scenarios=%u checks=%u native_authority=0 insertion_permission=0\n",scenarios,checks);
}
