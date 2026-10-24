#!/bin/sh
set -eu
out=${ORDER_OBSERVATION_OUTPUT:?fresh output directory required}
mkdir -p "$out"
root=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
if [ -n "${QT_PROBE_SDK_ENV:-}" ]; then
    . "$QT_PROBE_SDK_ENV"
    moc="$OECORE_NATIVE_SYSROOT/usr/libexec/moc"
else
    moc=${QT_HOST_MOC:-/usr/lib/qt6/libexec/moc}
fi
for name in reader_selected_view_probe reader_joined_creation_witness; do
    "$moc" "$root/$name.h" -o "$out/moc_$name.cpp"
done
"$moc" "$root/qt_qml_creation_bridge.h" -o "$out/qt_qml_creation_bridge.moc"
"$moc" "$root/reader_insertion_post_order_fixture.cpp" -o "$out/reader_insertion_post_order_fixture.moc"
sanitize=
if [ "${ORDER_OBSERVATION_SANITIZERS:-0}" = 1 ]; then sanitize='-fsanitize=address,undefined -fno-omit-frame-pointer'; fi
${CXX:-c++} -std=c++17 -Wall -Wextra -Werror -O1 -g -fPIC -pthread $sanitize -I"$root" -I"$out" \
    "$root/order_observation_test.cpp" "$root/qt_qml_creation_bridge.cpp" \
    "$out/moc_reader_selected_view_probe.cpp" "$out/moc_reader_joined_creation_witness.cpp" \
    $(pkg-config --cflags --libs Qt6Core Qt6Gui Qt6Qml Qt6Quick) -o "$out/order-observation-test"
printf '#include "read_only_backend.h"\n' > "$out/production.cpp"
${CXX:-c++} -std=c++17 -Wall -Wextra -Werror -fPIC -pthread -I"$root" \
    $(pkg-config --cflags Qt6Core Qt6Gui Qt6Qml Qt6Quick Qt6Network) -c "$out/production.cpp" -o "$out/production.o"
if [ -n "${QT_PROBE_SDK_ENV:-}" ]; then
    timeout --kill-after=1s 40s "$OECORE_NATIVE_SYSROOT/usr/bin/qemu-arm" -L "$SDKTARGETSYSROOT" \
        -E LANG=C.UTF-8 -E QT_QPA_PLATFORM=offscreen -E QT_PLUGIN_PATH="$SDKTARGETSYSROOT/usr/lib/plugins" "$out/order-observation-test"
else
    QT_QPA_PLATFORM=offscreen ASAN_OPTIONS=detect_leaks=0:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 \
        timeout --kill-after=1s 40s "$out/order-observation-test"
fi
