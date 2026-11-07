// Explicit one-use supervisor admission only. No autonomous boot installation.
#if defined(READER_SELECTED_TESTING) || defined(READER_WORKER_NATIVE_TESTING) || defined(READER_RESIDENT_TESTING) || defined(RESIDENT_WORKER_SLOT_TESTING) || defined(READER_INSERTION_LIFECYCLE_TESTING) || defined(READER_INSERTION_GUARD_ENTRY_TESTING) || defined(RESIDENT_MUTATION_TESTING) || defined(RESIDENT_MUTATION_BACKEND_TESTING) || defined(RESIDENT_SELECTION_TESTING) || defined(READER_IDENTITY_CAPTURE_TESTING) || defined(VISUAL_CAPTURE_TESTING)
#error Fixture hooks must not enter the native artifact.
#endif
#if defined(COMPOSER_PRE_ENTER_TESTING) || defined(COMPOSER_PATCH_OWNED_TESTING)
#error Fixture hooks must not enter the native artifact.
#endif
#ifndef RSDK_PROFILE_SHA256
#error Build must bind the exact profile manifest digest.
#endif
#include "mutation_backend.h"
#include "resident_worker_slot.h"
#include "reader_insertion_child_environment.h"
#include "composer_pre_enter.h"
#include "composer_pre_enter_patch_private.h"
#include "../resident/activation.h"
#include "../resident/loaded_artifact.h"
#include <atomic>
#include <cstdlib>
#include <cerrno>
#include <sys/stat.h>

namespace remarkable::composer_pre_enter::detail::patch {
// This type is defined only at the authenticated constructor callsite.
class ConstructorStartup final {
public:
    static bool trustedLoader(const void *anchor) noexcept;
    static Outcome install(resident::Activation &activation,
                           const reader_insertion_native_profile::Facilities &facilities);
};
bool ConstructorStartup::trustedLoader(const void *anchor) noexcept {
    struct stat preload{};
    if(::lstat("/etc/ld.so.preload",&preload)==0||errno!=ENOENT)return false;
    const char *selected=std::getenv("LD_PRELOAD");
    Dl_info info{};
    if(!selected||*selected!='/'||std::strpbrk(selected," :\t\r\n")||
       !::dladdr(anchor,&info)||!info.dli_fname||std::strcmp(selected,info.dli_fname))return false;
    for(char **entry=::environ;*entry;++entry) {
        if(!std::strncmp(*entry,"LD_",3)&&std::strncmp(*entry,"LD_PRELOAD=",11)&&
           std::strncmp(*entry,"LD_BIND_NOW=",12))return false;
    }
    return !std::getenv("GLIBC_TUNABLES");
}
Outcome ConstructorStartup::install(resident::Activation &activation,
                                    const reader_insertion_native_profile::Facilities &facilities) {
    const auto expiry=activation.captureStartupExpiryBootMs();
    const long pageSize=::sysconf(_SC_PAGESIZE);
    if(!expiry||pageSize<=0||Route::current()||!facilities.composerPatchSite(pageSize))
        return Outcome::Refused;
    if(!composer_scene::Profile::load()||!pinPreparedRouteCode()||
       activation.captureStartupExpiryBootMs()!=expiry)return Outcome::Refused;
    // The constructor installs a dormant route. A capture request and its
    // independent observation deadline belong to a later qualified profile.
    auto *route=new Route;
    try {
        auto capsule=StartupBuilder::prepare(pageSize,
            reinterpret_cast<std::uintptr_t>(&remarkable_pre_enter_shim),expiry);
        const auto result=capsule ? StartupBuilder::install(*capsule,*route,Route::current_)
                                  : Outcome::Refused;
        if(result!=Outcome::Installed)delete route;
        return result;
    }catch(...) {
        delete route;
        throw;
    }
}
}

namespace {
remarkable::resident::Activation *activation=nullptr;
reader_insertion_native_profile::Facilities *facilities=nullptr;
std::atomic_flag scheduled=ATOMIC_FLAG_INIT;
[[noreturn]] void refuse()noexcept{std::_Exit(119);}
__attribute__((noinline)) void artifactAnchor(){}
__attribute__((constructor(200))) void initialize() {
    try {
        if(QCoreApplication::instance()||activation||facilities||::geteuid()!=0)refuse();
        const char *directory=std::getenv("RSDK_ACTIVATION_DIRECTORY");
        if(!directory||!*directory)refuse();
        const QString root=QString::fromLocal8Bit(directory);
        if(::unsetenv("RSDK_ACTIVATION_DIRECTORY"))refuse();
        remarkable::resident::ArtifactDigests hashes;
        hashes.profile=QByteArray::fromHex(RSDK_PROFILE_SHA256);
        hashes.executable=QByteArray::fromHex("071d85beef3ef2d4cc0e11002140b27b82a2cc04a2ed740a5669f591069b77df");
        hashes.adapter=remarkable::resident::loadedArtifactDigest(reinterpret_cast<const void*>(&artifactAnchor));
        auto admitted=remarkable::resident::Activation::claim(root,hashes);
        if(!admitted)refuse();
        namespace patch=remarkable::composer_pre_enter::detail::patch;
        using remarkable::composer_pre_enter::detail::StartupSelected;
        if constexpr(StartupSelected)
            if(!patch::ConstructorStartup::trustedLoader(reinterpret_cast<const void*>(&artifactAnchor)))refuse();
        // The original loader independently verifies executable/provider hashes,
        // mapped code, eager bindings and pre-main conditions before any hook.
#if defined(RSDK_INSTALLED_READER_INSERT_V1) || defined(RSDK_DEVELOPMENT_EFFECT_PROBE_V1)
        facilities=reader_insertion_native_profile::installBeforeMain(true);
#else
        facilities=reader_insertion_native_profile::installBeforeMain(false);
#endif
        if(!facilities||!admitted->directoryCurrent()||!reader_child_environment::clearInheritedInjection())refuse();
        if constexpr(StartupSelected)
            if(patch::ConstructorStartup::install(*admitted,*facilities)!=patch::Outcome::Installed)refuse();
        activation=admitted.release(); // One process-resident admission/directory.
    }catch(...){refuse();}
}
void startup() {
    if(scheduled.test_and_set(std::memory_order_relaxed))return;
    auto *app=QCoreApplication::instance();
    if(!activation||!facilities||!app)refuse();
    if(!QMetaObject::invokeMethod(app,[app]{
        try {
            auto *gui=qobject_cast<QGuiApplication*>(app);
            if(!gui||QThread::currentThread()!=gui->thread()||!activation->directoryCurrent())refuse();
            // Ordinary installBeforeMain(false) keeps mutation unavailable. The source
            // dispatcher cannot manufacture conditional-profile admission.
            auto *backend=new remarkable::rm2_3_28_0_172::MutationBackend(facilities,gui);
            const QPointer<remarkable::rm2_3_28_0_172::MutationBackend> guarded(backend);
            auto *endpoint=new remarkable::resident::Endpoint(activation->config(),[guarded](auto lease){
                if(guarded)guarded->submit(std::move(lease));
            },gui);
            if(!backend->attach(endpoint)||!endpoint->start())refuse();
        }catch(...){refuse();}
    },Qt::QueuedConnection))refuse();
}
}
Q_COREAPP_STARTUP_FUNCTION(startup)
