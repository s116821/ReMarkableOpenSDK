#pragma once
#include <cstdint>
#include <sys/types.h>

namespace remarkable::resident { class Activation; }
namespace remarkable::rm1_observation {
class Admission;
// At most one attempt, before QCoreApplication. Never installs hooks.
const Admission *admitBeforeMain(const resident::Activation &) noexcept;
class Admission final {
    const resident::Activation *activation_;
    pid_t process_;
    std::uint64_t start_;
    Admission(const resident::Activation *a,pid_t p,std::uint64_t start)
        :activation_(a),process_(p),start_(start) {}
    ~Admission()=default; // Explicitly process-resident, like the Activation.
    friend const Admission *admitBeforeMain(const resident::Activation &) noexcept;
public:
    Admission(const Admission &)=delete;
    Admission &operator=(const Admission &)=delete;
    bool current() const noexcept;
};
}
