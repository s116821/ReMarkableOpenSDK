#pragma once
#include <QtGlobal>
#include <functional>
#include <limits>
#include <time.h>

namespace qml_access {
// Development permission only. CLOCK_BOOTTIME includes suspend, like /proc/uptime.
class CreationDeadline final {
public:
    using Now = std::function<qint64()>;
    static qint64 boottimeMs() {
        struct timespec value{};
        if (::clock_gettime(CLOCK_BOOTTIME, &value) != 0 || value.tv_sec < 0 ||
            value.tv_nsec < 0 || value.tv_nsec >= 1000000000 ||
            value.tv_sec > (std::numeric_limits<qint64>::max() - 999) / 1000) return -1;
        return static_cast<qint64>(value.tv_sec) * 1000 + value.tv_nsec / 1000000;
    }
    explicit CreationDeadline(Now now = boottimeMs) : now_(std::move(now)) {}
    bool startSetup(qint64 durationMs) {
        if (started_) return false;
        started_ = true;
        const qint64 now = now_();
        if (!canAdd(now, durationMs)) return false;
        setup_ = now + durationMs;
        return true;
    }
    bool accept(qint64 accessMs) {
        if (!started_ || access_ > 0) return false;
        const qint64 now = now_();
        if (now < 0 || now >= setup_ || !canAdd(now, accessMs)) return false;
        access_ = now + accessMs;
        return true;
    }
    qint64 nowMs() const { return now_(); }
    bool open() const { return remainingMs() > 0; }
    qint64 remainingMs() const {
        const qint64 now = now_();
        const qint64 deadline = access_ > 0 ? access_ : setup_;
        return started_ && now >= 0 && now < deadline ? deadline - now : 0;
    }
    qint64 setupDeadlineMs() const { return setup_; }
private:
    static bool canAdd(qint64 now, qint64 duration) {
        return now >= 0 && duration > 0 && now <= std::numeric_limits<qint64>::max() - duration;
    }
    Now now_;
    qint64 setup_ = 0, access_ = 0;
    bool started_ = false;
};
}
