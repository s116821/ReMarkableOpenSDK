/* Metadata only: no event read, grab, injection, native operation or version. */
#define _GNU_SOURCE
#include "rm1_input_geometry.h"
#include <fcntl.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/sysmacros.h>
#include <unistd.h>

static int model(void) {
    char value[32] = {0};
    int fd = open("/sys/devices/soc0/machine", O_RDONLY | O_CLOEXEC | O_NOFOLLOW);
    if (fd < 0) return 0;
    ssize_t n = read(fd, value, sizeof(value));
    close(fd);
    return n == (ssize_t)(sizeof("reMarkable 1.0\n") - 1) &&
        memcmp(value, "reMarkable 1.0\n", sizeof("reMarkable 1.0\n") - 1) == 0;
}
static int snapshot(int fd, struct rm1_snapshot *s) {
    memset(s, 0, sizeof(*s));
    if (ioctl(fd, EVIOCGNAME(sizeof(s->name)), s->name) < 0 ||
        ioctl(fd, EVIOCGBIT(0, sizeof(s->events)), s->events) < 0 ||
        ioctl(fd, EVIOCGBIT(EV_ABS, sizeof(s->absolute)), s->absolute) < 0) return 0;
    for (unsigned i = 0; i < RM1_AXIS_COUNT; ++i) {
        s->axes[i].present = rm1_bit(s->absolute, rm1_axes[i]);
        if (s->axes[i].present && ioctl(fd, EVIOCGABS(rm1_axes[i]), &s->axes[i].bounds) < 0) return 0;
        s->axes[i].bounds.value = 0; /* Never retain/report current touch coordinates. */
    }
    return rm1_valid(s);
}
int main(int argc, char **argv) {
    (void)argv;
    alarm(3); /* Default termination; no hard bound on kernel-side ioctl waits. */
    if (argc != 1 || !model()) return 1;
    int fd = open("/dev/input/event2", O_RDONLY | O_NONBLOCK | O_CLOEXEC | O_NOFOLLOW);
    if (fd < 0) return 1;
    struct stat before, after;
    struct rm1_snapshot a, b;
    int valid = fstat(fd, &before) == 0 && S_ISCHR(before.st_mode) &&
        major(before.st_rdev) == 13 && minor(before.st_rdev) == 66 &&
        snapshot(fd, &a) && snapshot(fd, &b) && fstat(fd, &after) == 0 &&
        before.st_ino == after.st_ino && before.st_rdev == after.st_rdev &&
        rm1_same(&a, &b) && model();
    close(fd);
    if (!valid) return 1;
    char output[2048];
    int n = snprintf(output, sizeof(output),
        "{\"schema\":1,\"model\":\"reMarkable 1.0\",\"touch\":\"cyttsp5_mt\","
        "\"native_navigation\":\"unsupported\",\"orientation\":\"unqualified\",\"axes\":[");
    for (unsigned i = 0; i < RM1_AXIS_COUNT; ++i) {
        if (n < 0 || n >= (int)sizeof(output)) return 1;
        const struct input_absinfo *x = &a.axes[i].bounds;
        n += snprintf(output + n, sizeof(output) - (size_t)n,
            "%s{\"code\":%u,\"present\":%s,\"min\":%d,\"max\":%d,\"fuzz\":%d,\"flat\":%d,\"resolution\":%d}",
            i ? "," : "", rm1_axes[i], a.axes[i].present ? "true" : "false",
            x->minimum, x->maximum, x->fuzz, x->flat, x->resolution);
    }
    if (n < 0 || n > (int)sizeof(output) - 4) return 1;
    memcpy(output + n, "]}\n", 3); n += 3;
    return fwrite(output, 1, (size_t)n, stdout) == (size_t)n && fflush(stdout) == 0 ? 0 : 1;
}
