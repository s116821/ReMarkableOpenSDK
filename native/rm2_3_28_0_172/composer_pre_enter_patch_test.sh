#!/bin/sh
# Raw Linux capsule in owned children; no native builder or activation.
set -eu
root=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
work=${COMPOSER_PATCH_OUTPUT:?fresh output required}
test ! -e "$work"
mkdir -p "$work"
arch=
if [ -n "${QT_PROBE_SDK_ENV:-}" ]; then . "$QT_PROBE_SDK_ENV"; arch=-marm; fi
${CXX:-c++} -std=c++17 -Wall -Wextra -Werror -O1 -g -pthread \
    $arch -DCOMPOSER_PATCH_OWNED_TESTING "$root/composer_pre_enter_patch_owned.cpp" -o "$work/capsule-test"
printf '%s\n' '#include "composer_pre_enter_patch_private.h"' > "$work/private.cpp"
${CXX:-c++} -std=c++17 -Wall -Wextra -Werror $arch -I"$root" \
    -c "$work/private.cpp" -o "$work/private-production.o"
if [ -n "${QT_PROBE_SDK_ENV:-}" ]; then
    ${OBJDUMP} -d "$work/capsule-test" > "$work/capsule-disassembly.txt"
    timeout --kill-after=1s 40s "$OECORE_NATIVE_SYSROOT/usr/bin/qemu-arm" -L "$SDKTARGETSYSROOT" "$work/capsule-test"
else
    objdump -d "$work/capsule-test" > "$work/capsule-disassembly.txt"
    timeout --kill-after=1s 40s "$work/capsule-test"
fi
