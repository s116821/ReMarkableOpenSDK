#!/bin/sh
# Owned Qt controller/callback model only; no device or native firmware calls.
set -eu
work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT
if [ -n "${QT_PROBE_SDK_ENV:-}" ]; then
    . "$QT_PROBE_SDK_ENV"
    moc="$OECORE_NATIVE_SYSROOT/usr/libexec/moc"
else
    moc=${QT_HOST_MOC:-/usr/lib/qt6/libexec/moc}
fi
for name in reader_selected_view_probe reader_joined_creation_witness; do
    "$moc" "scripts/$name.h" -o "$work/moc_$name.cpp"
done
"$moc" scripts/qt_qml_creation_bridge.h -o "$work/qt_qml_creation_bridge.moc"
"$moc" scripts/reader_joined_creation_witness_fixture.cpp -o "$work/reader_joined_creation_witness_fixture.moc"
${CXX:-c++} -std=c++17 -Wall -Wextra -Werror -O2 -fPIC -pthread -I"$work" -Iscripts \
    scripts/reader_joined_creation_witness_fixture.cpp scripts/qt_qml_creation_bridge.cpp \
    "$work/moc_reader_selected_view_probe.cpp" "$work/moc_reader_joined_creation_witness.cpp" \
    $(pkg-config --cflags --libs Qt6Core Qt6Gui Qt6Qml Qt6Quick) -o "$work/fixture"
if [ -n "${QT_PROBE_BINARY_OUT:-}" ]; then cp "$work/fixture" "$QT_PROBE_BINARY_OUT"; fi
if [ -n "${QT_PROBE_SDK_ENV:-}" ]; then
    "$OECORE_NATIVE_SYSROOT/usr/bin/qemu-arm" -L "$SDKTARGETSYSROOT" \
        -E LANG=C.UTF-8 -E QT_QPA_PLATFORM=offscreen \
        -E QT_PLUGIN_PATH="$SDKTARGETSYSROOT/usr/lib/plugins" "$work/fixture"
else
    QT_QPA_PLATFORM=offscreen "$work/fixture"
fi
