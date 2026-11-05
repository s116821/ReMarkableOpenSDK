#!/bin/sh
set -eu
out=${COMPOSER_OBSERVER_OUTPUT:?fresh output directory required}
test ! -e "$out" && test ! -L "$out"
mkdir -p "$out"
root=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
if [ -n "${QT_PROBE_SDK_ENV:-}" ]; then . "$QT_PROBE_SDK_ENV"; fi
sanitize=
if [ "${COMPOSER_OBSERVER_SANITIZERS:-0}" = 1 ]; then
    sanitize='-fsanitize=address,undefined -fno-omit-frame-pointer -no-pie'
fi
${CXX:-c++} -std=c++17 -Wall -Wextra -Werror -O1 -g -fPIC -pthread $sanitize \
    "$root/composer_document_observer_test.cpp" \
    $(pkg-config --cflags --libs Qt6Core Qt6Gui) -o "$out/observer-test"
printf '#include "composer_document_observer.h"\n' > "$out/production.cpp"
${CXX:-c++} -std=c++17 -Wall -Wextra -Werror -fPIC -pthread -I"$root" \
    $(pkg-config --cflags Qt6Core Qt6Gui) -c "$out/production.cpp" -o "$out/production.o"
if [ -n "${QT_PROBE_SDK_ENV:-}" ]; then
    timeout --kill-after=1s 20s "$OECORE_NATIVE_SYSROOT/usr/bin/qemu-arm" -L "$SDKTARGETSYSROOT" \
        -E LANG=C.UTF-8 -E QT_QPA_PLATFORM=offscreen \
        -E QT_PLUGIN_PATH="$SDKTARGETSYSROOT/usr/lib/plugins" "$out/observer-test"
else
    QT_QPA_PLATFORM=offscreen ASAN_OPTIONS=detect_leaks=0:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 \
        timeout --kill-after=1s 20s "$out/observer-test"
fi
