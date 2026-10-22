#!/bin/sh
# Entirely owned fixtures; no target access, native startup or installation.
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
"$moc" tools/reader_fixture_navigation_fixture.cpp -o "$work/reader_fixture_navigation_fixture.moc"
"$moc" tools/reader_navigation_ownership_fixture.cpp -o "$work/reader_navigation_ownership_fixture.moc"
${CXX:-c++} -std=c++17 -Wall -Wextra -Werror -O2 -fPIC -I"$work" -Itools \
    tools/reader_navigation_ownership_fixture.cpp \
    $(pkg-config --cflags --libs Qt6Qml Qt6Gui Qt6Quick) -o "$work/fixture"
if [ -n "${QT_PROBE_BINARY_OUT:-}" ]; then cp "$work/fixture" "$QT_PROBE_BINARY_OUT"; fi
if [ -n "${QT_PROBE_SDK_ENV:-}" ]; then
    "$OECORE_NATIVE_SYSROOT/usr/bin/qemu-arm" -L "$SDKTARGETSYSROOT" \
        -E LANG=C.UTF-8 -E QT_QPA_PLATFORM=offscreen -E QT_QUICK_BACKEND=software \
        -E QT_PLUGIN_PATH="$SDKTARGETSYSROOT/usr/lib/plugins" \
        -E QML_IMPORT_PATH="$SDKTARGETSYSROOT/usr/lib/qml" "$work/fixture"
else
    QT_QPA_PLATFORM=offscreen QT_QUICK_BACKEND=software "$work/fixture"
fi
