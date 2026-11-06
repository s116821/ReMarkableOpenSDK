#!/bin/sh
# Owned ABI fakes. The private retirement test gate proves no native quiescence.
set -eu
root=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
work=${COMPOSER_DELIVERY_OUTPUT:?fresh output directory required}
test ! -e "$work"
mkdir -p "$work"
if [ -n "${QT_PROBE_SDK_ENV:-}" ]; then
    . "$QT_PROBE_SDK_ENV"
    moc="$OECORE_NATIVE_SYSROOT/usr/libexec/moc"
else
    moc=${QT_HOST_MOC:-/usr/lib/qt6/libexec/moc}
fi
sanitizers=
if [ "${COMPOSER_DELIVERY_SANITIZERS:-0}" = 1 ]; then sanitizers='-fsanitize=address,undefined -fno-omit-frame-pointer -no-pie'; fi
"$moc" "$root/composer_scene_delivery_test.cpp" -o "$work/composer_scene_delivery_test.moc"
${CXX:-c++} -std=c++17 -Wall -Wextra -Werror -O1 -g -fPIC -pthread $sanitizers -I"$work" \
    "$root/composer_scene_delivery_test.cpp" $(pkg-config --cflags --libs Qt6Core Qt6Gui) -ldl -o "$work/delivery-test"
printf '%s\n' '#include "composer_scene_delivery.h"' > "$work/production.cpp"
${CXX:-c++} -std=c++17 -Wall -Wextra -Werror -fPIC -pthread -I"$root" \
    $(pkg-config --cflags Qt6Core Qt6Gui) -c "$work/production.cpp" -o "$work/production.o"
if [ -n "${QT_PROBE_SDK_ENV:-}" ]; then
    timeout --kill-after=1s 40s "$OECORE_NATIVE_SYSROOT/usr/bin/qemu-arm" -L "$SDKTARGETSYSROOT" -E QT_QPA_PLATFORM=offscreen -E QT_PLUGIN_PATH="$SDKTARGETSYSROOT/usr/lib/plugins" "$work/delivery-test"
else
    QT_QPA_PLATFORM=offscreen ASAN_OPTIONS=detect_leaks=0:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 timeout --kill-after=1s 40s "$work/delivery-test"
fi
