#ifndef RM1_INPUT_GEOMETRY_H
#define RM1_INPUT_GEOMETRY_H
#include <linux/input.h>
#include <stdint.h>
#include <string.h>

#define RM1_AXIS_COUNT 8
static const unsigned rm1_axes[RM1_AXIS_COUNT] = {
    ABS_MT_SLOT, ABS_MT_POSITION_X, ABS_MT_POSITION_Y, ABS_MT_TRACKING_ID,
    ABS_MT_PRESSURE, ABS_MT_TOUCH_MAJOR, ABS_MT_TOUCH_MINOR, ABS_MT_ORIENTATION
};
struct rm1_axis {
    int present;
    struct input_absinfo bounds;
};
struct rm1_snapshot {
    char name[128];
    unsigned char events[(EV_MAX + 8) / 8];
    unsigned char absolute[(ABS_MAX + 8) / 8];
    struct rm1_axis axes[RM1_AXIS_COUNT];
};
static inline int rm1_bit(const unsigned char *bits, unsigned code) {
    return (bits[code / 8] >> (code % 8)) & 1;
}
static inline int rm1_valid(const struct rm1_snapshot *s) {
    if (memcmp(s->name, "cyttsp5_mt\0", sizeof("cyttsp5_mt")) != 0 ||
        !rm1_bit(s->events, EV_SYN) || !rm1_bit(s->events, EV_ABS)) return 0;
    for (unsigned i = 0; i < RM1_AXIS_COUNT; ++i) {
        const struct rm1_axis *a = &s->axes[i];
        if (a->present != rm1_bit(s->absolute, rm1_axes[i])) return 0;
        if (i < 4 && !a->present) return 0;
        if (a->present && (a->bounds.minimum > a->bounds.maximum ||
            ((i == 1 || i == 2) && a->bounds.minimum == a->bounds.maximum) ||
            a->bounds.fuzz < 0 || a->bounds.flat < 0 || a->bounds.resolution < 0)) return 0;
    }
    return 1;
}
static inline int rm1_same(const struct rm1_snapshot *a, const struct rm1_snapshot *b) {
    if (!rm1_valid(a) || !rm1_valid(b) ||
        memcmp(a->name, b->name, sizeof(a->name)) ||
        memcmp(a->events, b->events, sizeof(a->events)) ||
        memcmp(a->absolute, b->absolute, sizeof(a->absolute))) return 0;
    for (unsigned i = 0; i < RM1_AXIS_COUNT; ++i) {
        const struct input_absinfo *x = &a->axes[i].bounds, *y = &b->axes[i].bounds;
        if (a->axes[i].present != b->axes[i].present || x->minimum != y->minimum ||
            x->maximum != y->maximum || x->fuzz != y->fuzz || x->flat != y->flat ||
            x->resolution != y->resolution) return 0;
    }
    return 1;
}
#endif
