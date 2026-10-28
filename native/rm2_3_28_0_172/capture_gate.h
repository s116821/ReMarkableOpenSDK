#pragma once

namespace remarkable::rm2_3_28_0_172::capture_detail {
// Shared across source and visual calls; any nested entry poisons the outer
// publication until it unwinds. Only GUI-thread calls may proceed to Qt.
inline thread_local bool running = false, reentered = false;
struct Gate {
    bool entered = false;
    Gate() { if (running) reentered = true; else { running = true; reentered = false; entered = true; } }
    ~Gate() { if (entered) running = false; }
    Gate(const Gate &) = delete;
    Gate &operator=(const Gate &) = delete;
};
}
