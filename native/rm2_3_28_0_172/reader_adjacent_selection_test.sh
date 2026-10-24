#!/bin/sh
set -eu
out=${ADJACENT_SELECTION_OUTPUT:?fresh output directory required}
test ! -e "$out"
mkdir -p "$out"
root=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
if [ -n "${QT_PROBE_SDK_ENV:-}" ]; then
    . "$QT_PROBE_SDK_ENV"
    moc="$OECORE_NATIVE_SYSROOT/usr/libexec/moc"
else moc=${QT_HOST_MOC:-/usr/lib/qt6/libexec/moc}; fi
for name in reader_selected_view_probe reader_joined_creation_witness; do
    "$moc" "$root/$name.h" -o "$out/moc_$name.cpp"
done
"$moc" "$root/qt_qml_creation_bridge.h" -o "$out/qt_qml_creation_bridge.moc"
for name in reader_insertion_post_order_fixture reader_adjacent_selection_test; do
    "$moc" "$root/$name.cpp" -o "$out/$name.moc"
done
${CXX:-c++} -std=c++17 -Wall -Wextra -Werror -O1 -g -fPIC -pthread -I"$root" -I"$out" \
    "$root/reader_adjacent_selection_test.cpp" "$root/qt_qml_creation_bridge.cpp" \
    "$out/moc_reader_selected_view_probe.cpp" "$out/moc_reader_joined_creation_witness.cpp" \
    $(pkg-config --cflags --libs Qt6Core Qt6Gui Qt6Qml Qt6Quick) -o "$out/adjacent-selection-test"
if [ -n "${QT_PROBE_SDK_ENV:-}" ]; then
    timeout --kill-after=1s 40s "$OECORE_NATIVE_SYSROOT/usr/bin/qemu-arm" -L "$SDKTARGETSYSROOT" \
        -E LANG=C.UTF-8 -E QT_QPA_PLATFORM=offscreen -E QT_PLUGIN_PATH="$SDKTARGETSYSROOT/usr/lib/plugins" "$out/adjacent-selection-test"
else QT_QPA_PLATFORM=offscreen timeout --kill-after=1s 40s "$out/adjacent-selection-test"; fi
