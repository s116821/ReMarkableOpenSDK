// Explicit one-use supervised observation only; no autonomous boot activation.
#if defined(READER_SELECTED_TESTING) || defined(READER_WORKER_NATIVE_TESTING) || defined(READER_RESIDENT_TESTING) || defined(RESIDENT_WORKER_SLOT_TESTING) || defined(READER_INSERTION_LIFECYCLE_TESTING) || defined(READER_INSERTION_GUARD_ENTRY_TESTING) || defined(RESIDENT_MUTATION_TESTING) || defined(RESIDENT_MUTATION_BACKEND_TESTING) || defined(RESIDENT_SELECTION_TESTING) || defined(READER_IDENTITY_CAPTURE_TESTING) || defined(VISUAL_CAPTURE_TESTING)
#error Fixture hooks must not enter the observation artifact.
#endif
#include "read_only_backend.h"
#include "profile.h"
#include "../resident/activation.h"
#include "../resident/loaded_artifact.h"
#include "../rm2_3_28_0_172/reader_insertion_child_environment.h"
#include <atomic>
#include <cstdlib>

namespace {
remarkable::resident::Activation *activation=nullptr;
const remarkable::rm1_observation::Admission *admission=nullptr;
std::atomic_flag scheduled=ATOMIC_FLAG_INIT;
[[noreturn]] void refuse() noexcept {std::_Exit(119);}
__attribute__((noinline)) void artifactAnchor() {}
__attribute__((constructor(200))) void initialize() {
    try {
        if(QCoreApplication::instance()||activation||admission||::geteuid()!=0)refuse();
        const auto *profile=remarkable::rm1_observation::detail::reviewedProfile();
        if(!profile||!remarkable::rm1_observation::detail::complete(*profile))refuse();
        const char *directory=std::getenv("RSDK_ACTIVATION_DIRECTORY");
        if(!directory||!*directory)refuse();
        const QString root=QString::fromLocal8Bit(directory);
        if(::unsetenv("RSDK_ACTIVATION_DIRECTORY"))refuse();
        remarkable::resident::ArtifactDigests hashes;
        hashes.profile=QByteArray::fromHex(profile->manifestSha256);
        hashes.executable=QByteArray::fromHex(profile->executableSha256);
        hashes.adapter=remarkable::resident::loadedArtifactDigest(reinterpret_cast<const void*>(&artifactAnchor));
        auto claimed=remarkable::resident::Activation::claim(root,hashes);
        if(!claimed)refuse();
        admission=remarkable::rm1_observation::admitBeforeMain(*claimed);
        if(!admission||!admission->current()||!reader_child_environment::clearInheritedInjection())refuse();
        activation=claimed.release(); // Admission and Activation remain resident.
    }catch(...){refuse();}
}
void startup() {
    if(scheduled.test_and_set(std::memory_order_relaxed))return;
    auto *app=QCoreApplication::instance();
    if(!app||!activation||!admission||!admission->current())refuse();
    if(!QMetaObject::invokeMethod(app,[app]{
        try {
            auto *gui=qobject_cast<QGuiApplication*>(app);
            if(!gui||QThread::currentThread()!=gui->thread()||!admission->current())refuse();
            auto *backend=new remarkable::rm1_observation::ReadOnlyBackend(admission,gui);
            const QPointer<remarkable::rm1_observation::ReadOnlyBackend> guarded(backend);
            auto *endpoint=new remarkable::resident::Endpoint(activation->config(),[guarded](auto lease){
                if(guarded)guarded->submit(std::move(lease));
            },gui);
            if(!backend->attach(endpoint)||!endpoint->start())refuse();
        }catch(...){refuse();}
    },Qt::QueuedConnection))refuse();
}
Q_COREAPP_STARTUP_FUNCTION(startup)
}
