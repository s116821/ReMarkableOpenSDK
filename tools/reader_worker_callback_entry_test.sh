#!/bin/sh
# Owned Entry integration only, with a test-only Submit adapter; no native calls.
set -eu
work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT
if [ -n "${QT_PROBE_SDK_ENV:-}" ]; then
    . "$QT_PROBE_SDK_ENV"
    moc="$OECORE_NATIVE_SYSROOT/usr/libexec/moc"
else
    moc=${QT_HOST_MOC:-/usr/lib/qt6/libexec/moc}
fi
"$moc" tools/reader_selected_view_probe.h -o "$work/moc_reader_selected_view_probe.cpp"
"$moc" tools/reader_worker_callback_witness.h -o "$work/moc_reader_worker_callback_witness.cpp"
"$moc" tools/reader_joined_creation_witness.h -o "$work/moc_reader_joined_creation_witness.cpp"
"$moc" tools/qt_qml_creation_bridge.h -o "$work/qt_qml_creation_bridge.moc"
for source in reader_selected_view_fixture reader_fixture_navigation_fixture reader_navigation_ownership_fixture reader_worker_callback_entry_fixture; do
    "$moc" "tools/$source.cpp" -o "$work/$source.moc"
done
${CXX:-c++} -std=c++17 -Wall -Wextra -Werror -O2 -fPIC -pthread -I"$work" -Itools \
    tools/reader_worker_callback_entry_fixture.cpp "$work/moc_reader_selected_view_probe.cpp" "$work/moc_reader_worker_callback_witness.cpp" "$work/moc_reader_joined_creation_witness.cpp" tools/qt_qml_creation_bridge.cpp \
    $(pkg-config --cflags --libs Qt6Qml Qt6Gui Qt6Quick) -ldl -o "$work/fixture"
if [ -n "${QT_PROBE_SDK_ENV:-}" ]; then
    "$OECORE_NATIVE_SYSROOT/usr/bin/qemu-arm" -L "$SDKTARGETSYSROOT" \
        -E LANG=C.UTF-8 -E QT_QPA_PLATFORM=offscreen -E QT_QUICK_BACKEND=software \
        -E QT_PLUGIN_PATH="$SDKTARGETSYSROOT/usr/lib/plugins" -E QML_IMPORT_PATH="$SDKTARGETSYSROOT/usr/lib/qml" "$work/fixture"
else
    QT_QPA_PLATFORM=offscreen QT_QUICK_BACKEND=software "$work/fixture"
fi
