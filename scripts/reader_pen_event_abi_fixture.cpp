// Compile/emulator-only ABI discriminator. This program opens no device.
#include <linux/input.h>
#include <sys/time.h>
#include <cstddef>
#include <cstdio>
#include <type_traits>
static_assert(sizeof(void*) == 4, "fixed ARM32 profile");
static_assert(__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__, "little endian");
static_assert(sizeof(input_event) == 16, "kernel input record size");
static_assert(alignof(input_event) == 4, "kernel input record alignment");
static_assert(offsetof(input_event,type) == 8, "type offset");
static_assert(offsetof(input_event,code) == 10, "code offset");
static_assert(offsetof(input_event,value) == 12, "value offset");
static_assert(std::is_standard_layout<input_event>::value, "record layout");
static_assert(EV_SYN == 0 && EV_KEY == 1 && EV_ABS == 3 && SYN_REPORT == 0,
              "event constants");
static_assert(ABS_X == 0 && ABS_Y == 1 && ABS_PRESSURE == 24 && ABS_DISTANCE == 25,
              "axis constants");
static_assert(BTN_TOOL_PEN == 320 && BTN_TOOL_RUBBER == 321 && BTN_TOUCH == 330,
              "key constants");
int main() {
    input_event e{};
    e.type = EV_KEY; e.code = BTN_TOUCH; e.value = 0;
    const auto *b = reinterpret_cast<const unsigned char*>(&e);
    std::printf("input_event=%zu align=%zu offsets=%zu/%zu/%zu timeval=%zu bytes=",
        sizeof(e), alignof(input_event), offsetof(input_event,type),
        offsetof(input_event,code), offsetof(input_event,value), sizeof(timeval));
    for (std::size_t i=0;i<sizeof(e);++i) std::printf("%02x",unsigned(b[i]));
    std::puts(" no_device_open=true");
}
