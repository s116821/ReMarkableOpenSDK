#include "carriers.h"
#define STANDALONE __attribute__((noinline, noclone, noipa))
struct CompileObservation {
    OwnedWorker *worker;
    OwnedMap *map;
    const OwnedOptionalQString *optional;
    const QUuid *uuid;
    const QString *target, *pageTemplate;
    std::int32_t index, source, inner5, inner6;
    std::uint8_t sizePresent, inherit;
    double width, height;
};
volatile CompileObservation observed;
extern "C" STANDALONE void compile_outer_callee(OwnedWorker *worker, std::int32_t index,
    const OwnedOptionalQString &optional, std::uint8_t sizePresent, QSizeF size,
    std::int32_t source, std::uint8_t inherit, const QUuid &uuid) {
    observed.worker=worker; observed.index=index; observed.optional=&optional;
    observed.sizePresent=sizePresent; observed.width=size.width(); observed.height=size.height();
    observed.source=source; observed.inherit=inherit; observed.uuid=&uuid;
}
extern "C" STANDALONE void compile_outer_caller(OwnedWorker *worker,
    const OwnedOptionalQString *optional, const QUuid *uuid) {
    compile_outer_callee(worker,0x13579,*optional,0x7f,QSizeF(1404.25,1872.5),0x2468,0x5b,*uuid);
}
extern "C" STANDALONE void compile_inner_callee(OwnedMap *map, std::int32_t index,
    const QString &target,const QString &pageTemplate,std::int32_t fifth,std::int32_t sixth) {
    observed.map=map; observed.index=index; observed.target=&target;
    observed.pageTemplate=&pageTemplate; observed.inner5=fifth; observed.inner6=sixth;
}
extern "C" STANDALONE void compile_inner_caller(OwnedMap *map, const QString *target,
    const QString *pageTemplate) {
    compile_inner_callee(map,7,*target,*pageTemplate,-1,0);
    compile_inner_callee(map,19,*pageTemplate,*target,-123456789,0x7f123456);
}
