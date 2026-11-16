#!/bin/sh
# Owned Qt events and native request fakes. No stock process/device invocation.
set -eu
root=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
out=${COMPOSER_KEY_OUTPUT:?fresh output directory required}
test ! -e "$out" && test ! -L "$out"
mkdir -p "$out"
if [ -n "${QT_PROBE_SDK_ENV:-}" ]; then
    . "$QT_PROBE_SDK_ENV"
    moc="$OECORE_NATIVE_SYSROOT/usr/libexec/moc"
else
    moc=${QT_HOST_MOC:-/usr/lib/qt6/libexec/moc}
fi
modules='Qt6Core Qt6Gui Qt6Qml Qt6Quick'
flags='-std=c++17 -Wall -Wextra -Werror -fPIC -no-pie -pthread'
if [ "${COMPOSER_KEY_SANITIZERS:-0}" = 1 ]; then flags="$flags -fsanitize=address,undefined -fno-omit-frame-pointer"; fi
version=$(pkg-config --modversion Qt6Gui)
includes=$(pkg-config --variable=includedir Qt6Gui)
if [ ! -d "$includes/QtGui/$version" ] && [ -n "${SDKTARGETSYSROOT:-}" ]; then includes="$SDKTARGETSYSROOT$includes"; fi
test -r "$includes/QtGui/$version/QtGui/private/qtextdocument_p.h"
private="-DCOMPOSER_TEXT_PRIVATE_QT -I$includes/QtCore/$version -I$includes/QtCore/$version/QtCore -I$includes/QtGui/$version -I$includes/QtGui/$version/QtGui"
for header in reader_selected_view_probe composer_key_delivery composer_snapshot_retirement reader_request_retirement; do
    "$moc" $(pkg-config --cflags $modules) "$root/$header.h" -o "$out/moc_$header.cpp"
done
for source in composer_scene_delivery_test composer_key_delivery_test reader_selected_view_fixture reader_input_receiver_test; do
    "$moc" $(pkg-config --cflags $modules) "$root/$source.cpp" -o "$out/$source.moc"
done
${CXX:-c++} $flags $private -DCOMPOSER_SCENE_TESTING= -DCOMPOSER_DELIVERY_TESTING= -DCOMPOSER_TEXT_TESTING= \
    -DRESIDENT_WORKER_SLOT_TESTING= -DCOMPOSER_KEY_DELIVERY_TESTING= -I"$out" "$root/composer_key_delivery_test.cpp" \
    "$out/moc_composer_key_delivery.cpp" "$out/moc_reader_selected_view_probe.cpp" \
    "$out/moc_composer_snapshot_retirement.cpp" "$out/moc_reader_request_retirement.cpp" \
    $(pkg-config --cflags --libs $modules) -ldl -o "$out/key-test"
${CXX:-c++} $flags $private -DCOMPOSER_KEY_DELIVERY_TESTING= -I"$out" "$root/reader_input_receiver_test.cpp" \
    "$out/moc_reader_selected_view_probe.cpp" "$out/moc_composer_key_delivery.cpp" \
    "$out/moc_composer_snapshot_retirement.cpp" "$out/moc_reader_request_retirement.cpp" $(pkg-config --cflags --libs $modules) -ldl -o "$out/receiver-test"
printf '%s\n' '#include "composer_key_delivery.h"' > "$out/production.cpp"
${CXX:-c++} $flags -I"$root" $(pkg-config --cflags $modules) -c "$out/production.cpp" -o "$out/production.o"
for binary in key-test receiver-test; do
    if [ -n "${QT_PROBE_SDK_ENV:-}" ]; then
        timeout --kill-after=1s 40s "$OECORE_NATIVE_SYSROOT/usr/bin/qemu-arm" -L "$SDKTARGETSYSROOT" \
            -E QT_QPA_PLATFORM=offscreen -E QT_QUICK_BACKEND=software -E QT_PLUGIN_PATH="$SDKTARGETSYSROOT/usr/lib/plugins" "$out/$binary"
    else
        QT_QPA_PLATFORM=offscreen QT_QUICK_BACKEND=software ASAN_OPTIONS=detect_leaks=0:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 timeout --kill-after=1s 40s "$out/$binary"
    fi
done
