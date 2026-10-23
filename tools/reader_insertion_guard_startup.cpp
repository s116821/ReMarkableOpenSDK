// Fixed research startup only. The independent actor/packet is a prerequisite.
#if defined(READER_SELECTED_TESTING) || defined(READER_WORKER_NATIVE_TESTING) || defined(READER_INSERTION_GUARD_ENTRY_TESTING) || defined(READER_INSERTION_GUARD_TRANSPORT_TESTING)
#error Fixture boundaries must not enter the startup artifact.
#endif
#include "reader_insertion_guard_transport.h"
#include "reader_insertion_guard_native_profile.h"
#include "reader_insertion_child_environment.h"
#include <atomic>
#include <ctime>
#if !defined(READER_INSERTION_NONCE) || !defined(READER_INSERTION_DOCUMENT_ID) || !defined(READER_INSERTION_SOURCE_INDEX) || !defined(READER_INSERTION_WRONG_INDEX) || !defined(READER_INSERTION_TARGET_ID) || !defined(READER_INSERTION_EXPECTED_PAGES)
#error A separately frozen nonce, document/order, source/wrong indexes and target are required.
#endif
#if defined(READER_INSERTION_POST_REJECT_PROBE) && READER_INSERTION_POST_REJECT_PROBE != 1
#error The selected post-rejection probe value must be exactly one.
#endif
#if defined(READER_INSERTION_FORWARD) && READER_INSERTION_FORWARD != 1
#error The selected conditional-forward value must be exactly one.
#endif
#if defined(READER_INSERTION_FORWARD) && defined(READER_INSERTION_POST_REJECT_PROBE)
#error Conditional forwarding and the post-rejection probe are separate experiments.
#endif
#if defined(READER_INSERTION_SELECT_TARGET) && (READER_INSERTION_SELECT_TARGET != 1 || !defined(READER_INSERTION_FORWARD))
#error Target acquisition requires the exact conditional-forward selector.
#endif
namespace {
#ifdef READER_INSERTION_SELECT_TARGET
constexpr bool wantTargetAcquisition=true;
#else
constexpr bool wantTargetAcquisition=false;
#endif
#ifdef READER_INSERTION_FORWARD
constexpr bool wantConditionalForward=true;
static_assert(READER_INSERTION_WRONG_INDEX==-1,"Conditional mode cannot use a deliberately wrong source");
#else
constexpr bool wantConditionalForward=false;
#endif
#ifdef READER_INSERTION_POST_REJECT_PROBE
constexpr bool wantPostRejectProbe=true;
#else
constexpr bool wantPostRejectProbe=false;
#endif
constexpr char nonce[]=READER_INSERTION_NONCE;
constexpr char rootPath[]="/run/rmb-qt-probe-" READER_INSERTION_NONCE;
constexpr bool validNonce() {
    if(sizeof nonce!=33)return false;
    for(unsigned i=0;i<32;++i)if(!((nonce[i]>='0'&&nonce[i]<='9')||(nonce[i]>='a'&&nonce[i]<='f')))return false;
    return true;
}
static_assert(validNonce(),"Frozen nonce must be32 lowercase hexadecimal characters");
std::atomic<reader_insertion_native_profile::Facilities*> facilities{nullptr};
std::atomic_flag scheduled=ATOMIC_FLAG_INIT;
[[noreturn]] void stop() noexcept {std::_Exit(119);}
bool same(const struct stat &a,const struct stat &b) {
    return a.st_dev==b.st_dev&&a.st_ino==b.st_ino&&a.st_mode==b.st_mode&&a.st_uid==b.st_uid&&a.st_size==b.st_size&&a.st_nlink==b.st_nlink&&
        a.st_mtim.tv_sec==b.st_mtim.tv_sec&&a.st_mtim.tv_nsec==b.st_mtim.tv_nsec&&a.st_ctim.tv_sec==b.st_ctim.tv_sec&&a.st_ctim.tv_nsec==b.st_ctim.tv_nsec;
}
bool readPrivate(int root,const char *name,QByteArray &bytes) {
    const int fd=::openat(root,name,O_RDONLY|O_NOFOLLOW|O_NONBLOCK|O_CLOEXEC);if(fd<0)return false;
    struct stat before{},after{},named{};char buffer[128];
    const bool regular=!::fstat(fd,&before)&&S_ISREG(before.st_mode)&&(before.st_mode&07777)==0600&&before.st_uid==0&&before.st_nlink==1&&before.st_size>0&&before.st_size<128;
    const ssize_t count=regular?::read(fd,buffer,sizeof buffer):-1;
    const bool valid=regular&&count==before.st_size&&!::fstat(fd,&after)&&same(before,after)&&!::fstatat(root,name,&named,AT_SYMLINK_NOFOLLOW)&&same(after,named);
    const bool closed=::close(fd)==0;if(valid&&closed)bytes=QByteArray(buffer,count);return valid&&closed;
}
bool admitted() {
    if(::geteuid()!=0)return false;
    const int root=::open(rootPath,O_RDONLY|O_DIRECTORY|O_NOFOLLOW|O_CLOEXEC);if(root<0)return false;
    struct stat before{},after{},named{};
    bool valid=!::fstat(root,&before)&&S_ISDIR(before.st_mode)&&(before.st_mode&07777)==0700&&before.st_uid==0;
    const auto exact=[&](const char *name,const QByteArray &expected){QByteArray bytes;return readPrivate(root,name,bytes)&&bytes==expected;};
    const auto absent=[&](const char *name){struct stat s{};return ::fstatat(root,name,&s,AT_SYMLINK_NOFOLLOW)&&errno==ENOENT;};
    const QByteArray token(nonce);
    valid=valid&&exact("owner",token)&&exact("attempt.identity",reader_selected::selfIdentity());
    for(const char *name:{"actor.claim","actor.ready","activation.claim","candidate-start.claim","start.request","attempt.claim"})valid=valid&&exact(name,token+'\n');
    for(const char *name:{"restore.claim","selected-cancel","stop.request"})valid=valid&&absent(name);
    QByteArray deadline;
    valid=valid&&readPrivate(root,"deadline",deadline)&&deadline.endsWith('\n');
    const auto parts=deadline.chopped(1).split(' ');
    if(valid&&parts.size()==2&&reader_selected::decimal(parts[0])&&reader_selected::decimal(parts[1])) {
        const auto begin=parts[0].toULongLong(),end=parts[1].toULongLong();struct timespec now{};
        valid=begin<=UINT64_MAX-360&&end==begin+360&&::clock_gettime(CLOCK_BOOTTIME,&now)==0&&now.tv_sec>=0&&
            std::uint64_t(now.tv_sec)>=begin&&std::uint64_t(now.tv_sec)<end&&end-std::uint64_t(now.tv_sec)>185;
    }else valid=false;
    valid=valid&&!::fstat(root,&after)&&after.st_dev==before.st_dev&&after.st_ino==before.st_ino&&after.st_uid==0&&(after.st_mode&07777)==0700&&
        !::lstat(rootPath,&named)&&named.st_dev==before.st_dev&&named.st_ino==before.st_ino&&named.st_mode==before.st_mode&&named.st_uid==0;
    const bool closed=::close(root)==0;return valid&&closed;
}
__attribute__((constructor)) void install() {
    try {
        if(!admitted())stop();
        auto *value=reader_insertion_native_profile::installBeforeMain(wantConditionalForward);
        if(!value||!value->core||!value->baseAdmission.profileAndProviders||!value->applicationAbsent||!admitted())stop();
        if(wantConditionalForward&&(!value->conditionQualified||!value->baseConditionAdmission.profileAndProviders))stop();
        // Qualification has consumed LD_BIND_NOW and verified eager bindings.
        // Keep this DSO resident, but do not inject it into future exec helpers.
        if(!reader_child_environment::clearInheritedInjection())stop();
        facilities.store(value,std::memory_order_release);
    }catch(...){stop();}
}
void startup() {
    if(scheduled.test_and_set(std::memory_order_acq_rel))stop();
    auto *app=QCoreApplication::instance();if(!app||!facilities.load(std::memory_order_acquire)||!admitted())stop();
    if(!QMetaObject::invokeMethod(app,[app]{
        try {
            auto *gui=qobject_cast<QGuiApplication*>(app);auto *f=facilities.load(std::memory_order_acquire);
            if(!gui||!f||QThread::currentThread()!=gui->thread()||!admitted())stop();
            reader_insertion_transport::Config config;config.nonce=nonce;
            config.coordinator.wantPostRejectProbe=wantPostRejectProbe;
            config.coordinator.wantConditionalForward=wantConditionalForward;
            config.coordinator.wantTargetAcquisition=wantTargetAcquisition;
            config.navigation={QStringLiteral(READER_INSERTION_DOCUMENT_ID),READER_INSERTION_SOURCE_INDEX,true};
            auto &request=config.coordinator.request;
            request.documentId=QStringLiteral(READER_INSERTION_DOCUMENT_ID);request.sourceIndex=READER_INSERTION_SOURCE_INDEX;
            request.targetPageId=QStringLiteral(READER_INSERTION_TARGET_ID);
            request.expectedPages=QStringLiteral(READER_INSERTION_EXPECTED_PAGES).split(QLatin1Char(','),Qt::KeepEmptyParts);
            request.sourcePageId=request.expectedPages.value(request.sourceIndex);config.coordinator.wrongSourceIndex=READER_INSERTION_WRONG_INDEX;
            if(!config.valid())stop();
            reader_insertion_entry::Runtime runtime;runtime.core=f->core;runtime.calls=f->passiveCalls;runtime.admission=f->baseAdmission;
            // Conditional assertions for this exact composition. Core enforces
            // PCs/scope/thread, Coordinator retains owners, profile pins lock path.
            runtime.admission.originalCallsite=true;runtime.admission.operationScope=true;runtime.admission.workerThread=true;
            runtime.admission.nativeOwners=true;runtime.admission.finalLock=true;
            if(wantConditionalForward) {
                runtime.conditionAdmission=f->baseConditionAdmission;
                runtime.conditionAdmission.originalCallsite=true;runtime.conditionAdmission.operationScope=true;
                runtime.conditionAdmission.workerThread=true;runtime.conditionAdmission.nativeOwners=true;runtime.conditionAdmission.finalLock=true;
            }
            runtime.outerReturnPC=f->outerReturnPC;runtime.innerReturnPC=f->innerReturnPC;
            runtime.makeAttribution=[f](QObject *worker){const auto a=f->attributionParts(worker,wantPostRejectProbe,wantConditionalForward);return reader_insertion_entry::Attribution{a.map,a.context,a.read64,a.postRejectProbe,a.conditionCalls};};
            QJsonObject facts{{"profile_qualified",f->baseAdmission.profileAndProviders},{"application_absent_at_install",f->applicationAbsent},
                {"tasks_at_install",int(f->observedTaskCount)},{"bind_now",true},{"native_authority",false}};
            if(wantConditionalForward)facts.insert("condition_qualified",f->conditionQualified);
            auto *entry=new reader_insertion_transport::Entry(gui,QString::fromLatin1(rootPath),std::move(config),std::move(runtime),facts);
            if(!entry->start())stop();
        }catch(...){stop();}
    },Qt::QueuedConnection))stop();
}
}
Q_COREAPP_STARTUP_FUNCTION(startup)
