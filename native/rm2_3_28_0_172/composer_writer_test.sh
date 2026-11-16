#!/bin/sh
# Owned nodes/actions only; no stock process or device execution.
set -eu
root=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
out=${COMPOSER_WRITER_OUTPUT:?fresh output directory required}
test ! -e "$out" && test ! -L "$out"
mkdir -p "$out"
if [ -n "${QT_PROBE_SDK_ENV:-}" ]; then . "$QT_PROBE_SDK_ENV"; fi
if [ -n "${QT_PROBE_SDK_ENV:-}" ]; then moc="$OECORE_NATIVE_SYSROOT/usr/libexec/moc"; else moc=${QT_HOST_MOC:-/usr/lib/qt6/libexec/moc}; fi
version=$(pkg-config --modversion Qt6Gui)
includes=$(pkg-config --variable=includedir Qt6Gui)
if [ ! -d "$includes/QtGui/$version" ] && [ -n "${SDKTARGETSYSROOT:-}" ]; then includes="$SDKTARGETSYSROOT$includes"; fi
private="-DCOMPOSER_TEXT_PRIVATE_QT -I$includes/QtCore/$version -I$includes/QtCore/$version/QtCore -I$includes/QtGui/$version -I$includes/QtGui/$version/QtGui"
flags='-std=c++17 -Wall -Wextra -Werror -fPIC -no-pie -pthread'
if [ "${COMPOSER_WRITER_SANITIZERS:-0}" = 1 ]; then flags="$flags -fsanitize=address,undefined -fno-omit-frame-pointer"; fi
${CXX:-c++} $flags $private -DCOMPOSER_TEXT_TESTING -DCOMPOSER_SCENE_TESTING "$root/composer_writer_test.cpp" \
    $(pkg-config --cflags --libs Qt6Core Qt6Gui) -ldl -o "$out/writer-test"
printf '%s\n' '#include "composer_native_action.h"' '#include "composer_anchor_walk.h"' '#include "composer_plain_edit.h"' '#include "composer_native_insert.h"' '#include "composer_owned_edit.h"' > "$out/production.cpp"
${CXX:-c++} $flags $private -I"$root" $(pkg-config --cflags Qt6Core Qt6Gui) \
    -c "$out/production.cpp" -o "$out/production.o"
printf '%s\n' '#include "mutation_backend.h"' > "$out/issuer.cpp"
${CXX:-c++} $flags $private -DRSDK_DEVELOPMENT_COMPOSER_V1 -I"$root" \
    $(pkg-config --cflags Qt6Core Qt6Gui Qt6Network Qt6Qml Qt6Quick) \
    -c "$out/issuer.cpp" -o "$out/issuer.o"
for header in composer_snapshot_retirement reader_request_retirement reader_selected_view_probe; do
    "$moc" "$root/$header.h" -o "$out/moc_$header.cpp"
done
${CXX:-c++} $flags $private "$root/composer_issuer_test.cpp" "$out"/moc_*.cpp \
    $(pkg-config --cflags --libs Qt6Core Qt6Gui Qt6Network Qt6Qml Qt6Quick) -ldl -o "$out/issuer-test"
if [ -n "${QT_PROBE_SDK_ENV:-}" ]; then
    timeout --kill-after=1s 40s "$OECORE_NATIVE_SYSROOT/usr/bin/qemu-arm" -L "$SDKTARGETSYSROOT" -E QT_QPA_PLATFORM=offscreen -E QT_PLUGIN_PATH="$SDKTARGETSYSROOT/usr/lib/plugins" "$out/writer-test"
    timeout --kill-after=1s 40s "$OECORE_NATIVE_SYSROOT/usr/bin/qemu-arm" -L "$SDKTARGETSYSROOT" -E QT_QPA_PLATFORM=offscreen -E QT_PLUGIN_PATH="$SDKTARGETSYSROOT/usr/lib/plugins" "$out/issuer-test"
else
    QT_QPA_PLATFORM=offscreen ASAN_OPTIONS=detect_leaks=0:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 timeout --kill-after=1s 40s "$out/writer-test"
    QT_QPA_PLATFORM=offscreen ASAN_OPTIONS=detect_leaks=0:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 timeout --kill-after=1s 40s "$out/issuer-test"
fi
