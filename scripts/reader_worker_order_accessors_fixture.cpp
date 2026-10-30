// Owned function-pointer models only; never invokes the proprietary accessors.
#define READER_WORKER_ORDER_TESTING
#include "reader_worker_order_accessors.h"
#include <iostream>
#include <stdexcept>
#include <vector>
#include <climits>

namespace {
using namespace reader_worker_order;
int checks = 0;
void check(bool value, const char *name) {
    ++checks;
    if (!value) throw std::runtime_error(name);
}
const std::array<QString, 3> ids{{
    QStringLiteral("10000000-0000-0000-0000-000000000001"),
    QStringLiteral("20000000-0000-0000-0000-000000000002"),
    QStringLiteral("30000000-0000-0000-0000-000000000003")}};
struct Model {
    std::array<QString, 3> values = ids;
    std::array<int, 3> inverse{{4, 5, 6}};
    std::vector<std::pair<char, int>> calls;
    QObject *expected = nullptr;
    int throwAt = -1;
    std::function<void()> hook;
};
Model *model = nullptr;
QString fakeIdAt(QObject *worker, int index) {
    if (!model || worker != model->expected) throw std::runtime_error("wrong leased receiver");
    model->calls.emplace_back('i', index);
    if (int(model->calls.size()) == model->throwAt) throw std::system_error(std::make_error_code(std::errc::invalid_argument));
    if (model->hook) { auto hook = std::move(model->hook); hook(); }
    return index >= 4 && index <= 6 ? model->values[size_t(index - 4)] : QString();
}
int fakeIndexFor(QObject *worker, const QString &id) {
    if (!model || worker != model->expected) throw std::runtime_error("wrong inverse receiver");
    int position = -1;
    for (size_t i = 0; i < ids.size(); ++i) if (id == ids[i]) position = int(i);
    model->calls.emplace_back('x', position);
    if (int(model->calls.size()) == model->throwAt) throw std::system_error(std::make_error_code(std::errc::invalid_argument));
    return position < 0 ? -1 : model->inverse[size_t(position)];
}
Result run(Accessors &accessors, const Frozen &frozen, Model &value, QObject *worker) {
    model = &value;
    Result result;
    std::thread thread([&] { result = accessors.read(worker, frozen); });
    thread.join(); model = nullptr; return result;
}
}
int main(int argc, char **argv) {
    QCoreApplication app(argc, argv);
    try {
        auto frozen = freeze(5, 9, ids); check(bool(frozen.frozen), "valid frozen inputs");
        check(frozen.frozen->indexes() == std::array<int, 3>{{4,5,6}}, "fixed adjacent indexes");
        for (size_t i = 0; i < 3; ++i) check(QString::fromLatin1(frozen.frozen->expectedIds()[i].data()) == ids[i], "frozen canonical bytes");
        for (auto pair : std::array<std::pair<int,int>, 9>{{{-1,9},{0,9},{8,9},{9,9},{INT_MAX,9},{5,-1},{1,2},{INT_MIN,INT_MAX},{INT_MAX-1,INT_MAX}}})
            check(!freeze(pair.first, pair.second, ids).frozen, "invalid source/count range");
        check(bool(freeze(INT_MAX-2, INT_MAX, ids).frozen), "upper safe fixed indices");
        auto bad = ids;
        for (const QString &value : {QString(),QStringLiteral("bad"),QStringLiteral("10000000-0000-0000-0000-00000000000G"),QStringLiteral("10000000_0000-0000-0000-000000000001"),QStringLiteral("10000000-0000-0000-0000-00000000000\u0100")}) {
            bad = ids; bad[1] = value; check(!freeze(5,9,bad).frozen, "invalid input canonical UUID");
        }
        bad=ids;bad[2]=ids[0];check(!freeze(5,9,bad).frozen,"duplicate IDs rejected");
        bool foreignFrozen = true;
        std::thread foreign([&]{foreignFrozen=bool(freeze(5,9,ids).frozen);});foreign.join();
        check(!foreignFrozen,"freeze requires GUI thread");
        auto loaded=testing::load(&fakeIdAt,&fakeIndexFor);check(bool(loaded.accessors),"owned typed function pointers");
        check(!testing::load(nullptr,&fakeIndexFor).accessors,"missing owned first function");
        check(!testing::load(&fakeIdAt,nullptr).accessors,"missing owned second function");
        QObject worker;Model value;value.expected=&worker;
        check(loaded.accessors->read(&worker,*frozen.frozen).refusal==Refusal::GuiThread,"read rejects cached GUI thread");
        check(value.calls.empty(),"GUI refusal invokes no accessor");
        auto result=run(*loaded.accessors,*frozen.frozen,value,&worker);
        check(result.observed && result.calls==6 && result.refusal==Refusal::None,"six-read positive");
        check(value.calls==std::vector<std::pair<char,int>>{{'i',4},{'x',0},{'i',5},{'x',1},{'i',6},{'x',2}},"exact fixed call sequence");
        for(size_t i=0;i<3;++i) {
            check(result.entries[i].index==int(i)+4 && result.entries[i].inverse==int(i)+4,"inverse roundtrip");
            check(QString::fromLatin1(result.entries[i].id.data())==ids[i] && result.entries[i].id[36]=='\0',"owned scalar UUID output");
        }
        value.calls.clear();result=run(*loaded.accessors,*frozen.frozen,value,nullptr);
        check(!result.observed && result.refusal==Refusal::NullWorker && !result.calls && value.calls.empty(),"null receiver no call");
        for(size_t fault=0;fault<3;++fault) {
            for(int kind=0;kind<4;++kind) {
                value=Model{};value.expected=&worker;
                value.values[fault]=kind==0?QString():kind==1?QStringLiteral("bad"):kind==2?QStringLiteral("40000000-0000-0000-0000-000000000004"):QStringLiteral("10000000-0000-0000-0000-00000000000A");
                result=run(*loaded.accessors,*frozen.frozen,value,&worker);
                check(!result.observed && result.calls==2*fault+1 && value.calls.size()==result.calls,"ID failure stops at fixed call");
                check(result.refusal==(kind==0?Refusal::EmptyId:kind==2?Refusal::IdMismatch:Refusal::InvalidId),"ID failure classification");
            }
            for(int index : {-1,0,1000000}) {
                value=Model{};value.expected=&worker;value.inverse[fault]=index;
                result=run(*loaded.accessors,*frozen.frozen,value,&worker);
                check(!result.observed && result.refusal==Refusal::InverseMismatch && result.calls==2*fault+2,"inverse failure never follows returned index");
                check(value.calls.size()==result.calls,"no retry after inverse mismatch");
            }
        }
        for(int at=1;at<=6;++at) {
            value=Model{};value.expected=&worker;value.throwAt=at;
            result=run(*loaded.accessors,*frozen.frozen,value,&worker);
            check(!result.observed && result.refusal==Refusal::AccessorException && result.calls==unsigned(at),"typed accessor exception stops once");
            check(value.calls.size()==result.calls,"no retry after exception");
        }
        value=Model{};value.expected=&worker;Result nested;
        value.hook=[&]{nested=loaded.accessors->read(&worker,*frozen.frozen);};
        result=run(*loaded.accessors,*frozen.frozen,value,&worker);
        check(nested.refusal==Refusal::Reentered && !nested.calls,"nested read refused before accessor");
        check(result.refusal==Refusal::Reentered && result.calls==1 && !result.observed,"outer reentry invalidated");
        value=Model{};value.expected=&worker;result=run(*loaded.accessors,*frozen.frozen,value,&worker);
        check(result.observed,"read gate resets after failed independent call");
        check(!loadPinned([]{return false;}).accessors,"GUI load canceled");
        check(!loadPinned([]()->bool{throw std::runtime_error("cancel");}).accessors,"GUI permit exception");
        bool reentered=false;
        auto loading=loadPinned([&]{if(!reentered){reentered=true;check(!loadPinned([]{return true;}).accessors,"nested loader refused");}return true;});
        check(!loading.accessors && !loading.nativeBridge,"outer loader reentry refused");
        loading=loadPinned([]{return true;});
        check(!loading.accessors && !loading.nativeBridge && !loading.refusal.isEmpty(),"real fixture image rejected before private calls");
        bool loadedForeign=true;
        std::thread loader([&]{loadedForeign=bool(loadPinned([]{return true;}).accessors);});loader.join();
        check(!loadedForeign,"loader requires GUI thread");
        std::cout<<"PASS owned worker-order accessor checks="<<checks<<"; native private calls=0\n";
    } catch(const std::exception &error) {std::cerr<<"FAIL "<<error.what()<<" after "<<checks<<" checks\n";return 1;}
    return 0;
}
