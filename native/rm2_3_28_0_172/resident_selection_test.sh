#!/bin/sh
set -eu
out=${RESIDENT_SELECTION_OUTPUT:?fresh output directory required}
test ! -e "$out"
mkdir -p "$out"
root=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
if [ -n "${QT_PROBE_SDK_ENV:-}" ]; then
    . "$QT_PROBE_SDK_ENV"
    moc="$OECORE_NATIVE_SYSROOT/usr/libexec/moc"
else moc=${QT_HOST_MOC:-/usr/lib/qt6/libexec/moc}; fi
${CXX:-c++} --version > "$out/compiler.txt"
pkg-config --modversion Qt6Core >> "$out/compiler.txt"
for name in reader_selected_view_probe reader_joined_creation_witness; do
    "$moc" "$root/$name.h" -o "$out/moc_$name.cpp"
done
"$moc" "$root/qt_qml_creation_bridge.h" -o "$out/qt_qml_creation_bridge.moc"
for name in reader_insertion_post_order_fixture resident_selection_test; do
    "$moc" "$root/$name.cpp" -o "$out/$name.moc"
done
${CXX:-c++} -std=c++17 -Wall -Wextra -Werror -O1 -g -fPIC -pthread -DRESIDENT_SELECTION_TESTING -I"$root" -I"$out" \
    "$root/resident_selection_test.cpp" "$root/qt_qml_creation_bridge.cpp" \
    "$out/moc_reader_selected_view_probe.cpp" "$out/moc_reader_joined_creation_witness.cpp" \
    $(pkg-config --cflags --libs Qt6Core Qt6Gui Qt6Qml Qt6Quick) -o "$out/selection-test"
sha256sum "$root/resident_selection"* "$root/reader_insertion_post_order_fixture.cpp" "$root/qt_qml_creation_bridge.cpp" "$root"/*.h "$root/../resident"/*.h "$out"/moc_*.cpp "$out"/*.moc > "$out/inputs.sha256"
sha256sum "$out/selection-test" > "$out/artifacts.sha256"
if [ -n "${QT_PROBE_SDK_ENV:-}" ]; then
    timeout --kill-after=1s 40s "$OECORE_NATIVE_SYSROOT/usr/bin/qemu-arm" -L "$SDKTARGETSYSROOT" \
        -E LANG=C.UTF-8 -E QT_QPA_PLATFORM=offscreen -E QT_PLUGIN_PATH="$SDKTARGETSYSROOT/usr/lib/plugins" "$out/selection-test"
else QT_QPA_PLATFORM=offscreen timeout --kill-after=1s 40s "$out/selection-test"; fi
