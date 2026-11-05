#pragma once
// Original one-shot public pthread adapter. No native offsets or owner grant.
#include <pthread.h>
#include <atomic>
#include <cerrno>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <limits>

namespace reader_insertion_native_profile {class Facilities;}
namespace reader_insertion_job_snapshot {
struct Bounds { std::uintptr_t begin=0,end=0; };
struct Config {
    pthread_mutex_t *mutex=nullptr;
    const std::uint32_t *words=nullptr; // Immutable address of low word, then high word.
    Bounds mutexStorage,wordStorage;
    int (*tryLock)(pthread_mutex_t *)=nullptr;
    int (*unlock)(pthread_mutex_t *)=nullptr;
    // Retained for source compatibility with the initial research interface.
    // Deliberately ignored: no reporter may run with uncertain lock ownership.
    void *fatalContext=nullptr;
    void (*fatal)(void *,const char *)=nullptr;
};
class Snapshot final {
    const Config config_;
    std::atomic<bool> consumed_{false};
    explicit Snapshot(const Config &c):config_(c) {}
    ~Snapshot()=default; // Resident context; no native mutex/owner destruction.
    friend class reader_insertion_native_profile::Facilities;
    static bool extent(std::uintptr_t p,std::size_t size,std::size_t alignment,
                       Bounds storage,std::uintptr_t &end) {
        if(!p || p%alignment || !size || p>std::numeric_limits<std::uintptr_t>::max()-size ||
           storage.begin>=storage.end) return false;
        end=p+size;
        return storage.begin<=p && end<=storage.end;
    }
    [[noreturn]] static void stop() noexcept {
        // Ownership may remain acquired. No reporter, logger, cleanup callback,
        // attempted repair/unlock or C++ shutdown unwinding is permitted here.
        std::_Exit(119);
    }
public:
    // The ranges are caller assertions, not mapped-memory/lifetime proof.
    // Actual owners, normal-mutex kind and resolved C ABI providers are required.
    static Snapshot *create(const Config &c) {
        const auto mutex=reinterpret_cast<std::uintptr_t>(c.mutex);
        const auto words=reinterpret_cast<std::uintptr_t>(c.words);
        std::uintptr_t mutexEnd=0,wordsEnd=0;
        if(!c.tryLock || !c.unlock ||
           !extent(mutex,sizeof(pthread_mutex_t),alignof(pthread_mutex_t),c.mutexStorage,mutexEnd) ||
           !extent(words,2*sizeof(std::uint32_t),alignof(std::uint32_t),c.wordStorage,wordsEnd) ||
           (mutex<wordsEnd && words<mutexEnd)) return nullptr;
        return new Snapshot(c);
    }
    bool read(std::uint64_t &id) noexcept {
        id=0;
        if(consumed_.exchange(true,std::memory_order_acq_rel))return false;
        int acquired=0;
        try {acquired=config_.tryLock(config_.mutex);}catch(...){stop();}
        if(acquired==EBUSY)return false;
        if(acquired!=0)stop();
        std::uint32_t pair[2];
        // The entire successful owned interval is this exact eight-byte copy
        // and the already-resolved unlock. No reader/log/resolver callback.
        std::memcpy(pair,config_.words,sizeof pair);
        int released=0;
        try {released=config_.unlock(config_.mutex);}catch(...){stop();}
        if(released!=0)stop();
        const auto value=std::uint64_t(pair[0])|(std::uint64_t(pair[1])<<32);
        if(value==0)return false;
        id=value;return true;
    }
    static bool attribute(void *context,std::uint64_t &id) noexcept {
        id=0;
        return context && static_cast<Snapshot *>(context)->read(id);
    }
};
}
