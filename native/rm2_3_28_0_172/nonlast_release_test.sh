#!/bin/sh
# Owned controls only; no native addresses/control objects are accessed.
set -eu
root=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
work=${NONLAST_RELEASE_OUTPUT:?fresh output directory required}
test ! -e "$work"
mkdir -p "$work"
if [ -n "${QT_PROBE_SDK_ENV:-}" ]; then . "$QT_PROBE_SDK_ENV"; fi
sanitizers=
if [ "${NONLAST_RELEASE_SANITIZERS:-0}" = 1 ]; then sanitizers='-fsanitize=address,undefined -fno-omit-frame-pointer -no-pie'; fi
${CXX:-c++} -std=c++17 -Wall -Wextra -Werror -O2 -g $sanitizers \
    "$root/nonlast_release_test.cpp" -pthread -o "$work/nonlast-test"
if [ -n "${QT_PROBE_SDK_ENV:-}" ]; then
    timeout --kill-after=1s 30s "$OECORE_NATIVE_SYSROOT/usr/bin/qemu-arm" -L "$SDKTARGETSYSROOT" "$work/nonlast-test"
    printf '%s\n' '#include "nonlast_release.h"' \
        'extern "C" __attribute__((noinline)) unsigned owned_release_entry(std::int32_t *strong) noexcept {' \
        'return static_cast<unsigned>(remarkable::rm2_3_28_0_172::owner_release::decrementPinnedStrong(strong));}' > "$work/atomic.cpp"
    $CXX -std=c++17 -marm -O2 -fPIC -fno-exceptions -fno-rtti -I"$root" -c "$work/atomic.cpp" -o "$work/atomic.o"
    $OBJDUMP -d "$work/atomic.o" > "$work/atomic.asm"
    $NM --undefined-only "$work/atomic.o" > "$work/atomic.undefined"
    test ! -s "$work/atomic.undefined"
    for instruction in ldrex strex dmb; do grep -Eq "[[:space:]]$instruction(eq|ne)?[[:space:]]" "$work/atomic.asm"; done
    if grep -Eq '[[:space:]]blx?(eq|ne|cs|cc|mi|pl|vs|vc|hi|ls|ge|lt|gt|le|al)?[[:space:]]' "$work/atomic.asm"; then exit 1; fi
    sha256sum "$work/atomic.o" "$work/atomic.asm" > "$work/atomic.sha256"
else
    timeout --kill-after=1s 30s "$work/nonlast-test"
fi
