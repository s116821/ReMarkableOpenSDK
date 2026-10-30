#include "rm1_input_geometry.h"
#include <assert.h>
#include <stdio.h>

static void bit(unsigned char *p, unsigned n) { p[n / 8] |= (unsigned char)(1U << (n % 8)); }
static struct rm1_snapshot fixture(void) {
    struct rm1_snapshot s = {0};
    memcpy(s.name, "cyttsp5_mt", sizeof("cyttsp5_mt"));
    bit(s.events, EV_SYN); bit(s.events, EV_ABS);
    for (unsigned i = 0; i < RM1_AXIS_COUNT; ++i) {
        bit(s.absolute, rm1_axes[i]); s.axes[i].present = 1;
        s.axes[i].bounds.maximum = 100;
    }
    return s;
}
int main(void) {
    struct rm1_snapshot a = fixture(), b = a;
    assert(rm1_valid(&a)); assert(rm1_same(&a, &b));
    b.axes[1].bounds.value = 99; assert(rm1_same(&a, &b)); /* current input is volatile */
    b = a; b.axes[1].bounds.minimum = 101; assert(!rm1_valid(&b));
    b = a; b.axes[1].bounds.maximum = 0; assert(!rm1_valid(&b));
    b = a; b.axes[1].bounds.maximum++; assert(!rm1_same(&a, &b));
    b = a; b.axes[1].bounds.fuzz = -1; assert(!rm1_valid(&b));
    b = a; b.axes[1].bounds.flat = -1; assert(!rm1_valid(&b));
    b = a; b.axes[1].bounds.resolution = -1; assert(!rm1_valid(&b));
    b = a; b.axes[0].present = 0; assert(!rm1_valid(&b));
    b = a; b.events[0] = 0; assert(!rm1_valid(&b));
    b = a; b.name[0] = 'x'; assert(!rm1_valid(&b));
    b = a; b.name[sizeof("cyttsp5_mt") - 1] = 'x'; assert(!rm1_valid(&b));
    b = a; bit(b.absolute, ABS_X); assert(!rm1_same(&a, &b));
    a.axes[4].present = 0; a.absolute[rm1_axes[4] / 8] &= (unsigned char)~(1U << (rm1_axes[4] % 8));
    assert(rm1_valid(&a));
    b = fixture(); b.axes[0].bounds.maximum = 0; assert(rm1_valid(&b));
    puts("15 owned metadata boundaries passed; no real device accessed");
}
