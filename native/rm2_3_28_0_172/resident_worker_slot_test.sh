#!/bin/sh
# Owned ABI fakes. The private retirement test gate proves no native quiescence.
set -eu
root=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
work=${RESIDENT_SLOT_OUTPUT:?fresh output directory required}
test ! -e "$work"
mkdir -p "$work"
if [ -n "${QT_PROBE_SDK_ENV:-}" ]; then
    . "$QT_PROBE_SDK_ENV"
    moc="$OECORE_NATIVE_SYSROOT/usr/libexec/moc"
else
    moc=${QT_HOST_MOC:-/usr/lib/qt6/libexec/moc}
fi
sanitizers=
if [ "${RESIDENT_SLOT_SANITIZERS:-0}" = 1 ]; then sanitizers='-fsanitize=address,undefined -fno-omit-frame-pointer -no-pie'; fi
"$moc" "$root/resident_worker_slot_test.cpp" -o "$work/resident_worker_slot_test.moc"
${CXX:-c++} -std=c++17 -Wall -Wextra -Werror -O1 -g -fPIC -pthread $sanitizers -I"$work" \
    "$root/resident_worker_slot_test.cpp" $(pkg-config --cflags --libs Qt6Core Qt6Gui) -o "$work/slot-test"
printf '%s\n' '#include "resident_worker_slot.h"' > "$work/production.cpp"
${CXX:-c++} -std=c++17 -Wall -Wextra -Werror -fPIC -pthread -I"$root" \
    $(pkg-config --cflags Qt6Core Qt6Gui) -c "$work/production.cpp" -o "$work/production.o"
if [ -n "${QT_PROBE_SDK_ENV:-}" ]; then
    timeout --kill-after=1s 40s "$OECORE_NATIVE_SYSROOT/usr/bin/qemu-arm" -L "$SDKTARGETSYSROOT" "$work/slot-test"
else
    ASAN_OPTIONS=halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 timeout --kill-after=1s 40s "$work/slot-test"
fi
