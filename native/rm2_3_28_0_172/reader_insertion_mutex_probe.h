#pragma once
// One owned/qualified nonblocking mutex witness. No owner, loader or capability grant.
#include <pthread.h>
#include <atomic>
#include <cstdint>
#include <cstdlib>
#include <limits>

namespace reader_insertion_native_profile {class Facilities;}
namespace reader_insertion_mutex_probe {
struct Storage { std::uintptr_t begin = 0, end = 0; };
struct Config {
    pthread_mutex_t *mutex = nullptr;
    Storage storage;
    int (*tryLock)(pthread_mutex_t *) = nullptr;
    int (*unlock)(pthread_mutex_t *) = nullptr;
};
struct Result {
    bool attempted = false, acquired = false, released = false;
    int tryResult = -1, unlockResult = -1;
};
inline bool successful(const Result &r) noexcept {
    return r.attempted && r.acquired && r.released && r.tryResult == 0 && r.unlockResult == 0;
}
class Probe final {
    Config config_;
    std::atomic<bool> consumed_{false};
    explicit Probe(Config config) : config_(config) {}
    ~Probe() = default; // Process-resident; does not own or destroy the mutex.
    friend class reader_insertion_native_profile::Facilities;
public:
    static Probe *create(Config config) {
        const auto address = reinterpret_cast<std::uintptr_t>(config.mutex);
        if (!address || address % alignof(pthread_mutex_t) || !config.storage.begin ||
            config.storage.begin % alignof(pthread_mutex_t) || config.storage.end <= config.storage.begin ||
            address < config.storage.begin || address > std::numeric_limits<std::uintptr_t>::max() - sizeof(pthread_mutex_t) ||
            address + sizeof(pthread_mutex_t) > config.storage.end || !config.tryLock || !config.unlock ||
            config.tryLock == config.unlock) return nullptr;
        // Public pinned Linux/vendor pthread header layout, not a private Qt type.
        if (config.mutex->__data.__kind != 0) return nullptr;
        return new Probe(config);
    }
    Result once() noexcept {
        if (consumed_.exchange(true, std::memory_order_acq_rel)) std::_Exit(119);
        int tried = -1, unlocked = -1;
        try {
            tried = config_.tryLock(config_.mutex);
            if (tried != 0) std::_Exit(119);
            // No reporter, clock, atomic or other call between acquisition/unlock.
            unlocked = config_.unlock(config_.mutex);
            if (unlocked != 0) std::_Exit(119);
        } catch (...) { std::_Exit(119); }
        return {true, true, true, tried, unlocked};
    }
};
}
