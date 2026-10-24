// Explicit one-use supervisor admission only. No autonomous boot installation.
#if defined(READER_SELECTED_TESTING) || defined(READER_WORKER_NATIVE_TESTING) || defined(READER_RESIDENT_TESTING)
#error Fixture hooks must not enter the native artifact.
#endif
#ifndef RSDK_PROFILE_SHA256
#error Build must bind the exact profile manifest digest.
#endif
#include "read_only_backend.h"
#include "reader_insertion_child_environment.h"
#include "../resident/activation.h"
#include "../resident/loaded_artifact.h"
#include <atomic>
#include <cstdlib>

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
        // The original loader independently verifies executable/provider hashes,
        // mapped code, eager bindings and pre-main conditions before any hook.
        facilities=reader_insertion_native_profile::installBeforeMain(false);
        if(!facilities||!admitted->directoryCurrent()||!reader_child_environment::clearInheritedInjection())refuse();
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
            auto *backend=new remarkable::rm2_3_28_0_172::ReadOnlyBackend(facilities,gui);
            const QPointer<remarkable::rm2_3_28_0_172::ReadOnlyBackend> guarded(backend);
            auto *endpoint=new remarkable::resident::Endpoint(activation->config(),[guarded](auto lease){
                if(guarded)guarded->submit(std::move(lease));
            },gui);
            if(!backend->attach(endpoint)||!endpoint->start())refuse();
        }catch(...){refuse();}
    },Qt::QueuedConnection))refuse();
}
}
Q_COREAPP_STARTUP_FUNCTION(startup)
