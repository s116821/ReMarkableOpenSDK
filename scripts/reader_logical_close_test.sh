#!/bin/sh
set -eu
if [ -n "${QT_PROBE_SDK_ENV:-}" ]; then . "$QT_PROBE_SDK_ENV"; fi
compiler=${CXX:-c++}
work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT
if [ -n "${QT_PROBE_SDK_ENV:-}" ]; then
    moc="$OECORE_NATIVE_SYSROOT/usr/libexec/moc"
else
    moc="$(pkg-config --variable=libexecdir Qt6Core)/moc"
fi
for name in qt_page_facts qt_page_facts_entry; do
 "$moc" -DQT_FACTS_PRETOKEN_DIAGNOSTIC=1 "scripts/$name.h" -o "$work/$name.moc"
done
"$moc" scripts/qt_page_facts_fixture.cpp -o "$work/qt_page_facts_fixture.moc"
$compiler -std=c++17 -Wall -Wextra -Werror -O2 -fPIC -DQT_FACTS_PRETOKEN_DIAGNOSTIC=1 \
 -I"$work" -Iscripts scripts/reader_logical_close_fixture.cpp scripts/qt_page_facts_entry.cpp scripts/qt_page_facts.cpp \
 $(pkg-config --cflags --libs Qt6Quick Qt6Gui Qt6Core Qt6Qml) -o "$work/fixture"
if [ -n "${QT_PROBE_BINARY_OUT:-}" ]; then cp "$work/fixture" "$QT_PROBE_BINARY_OUT"; fi
if [ -n "${QT_PROBE_SDK_ENV:-}" ]; then
    "$OECORE_NATIVE_SYSROOT/usr/bin/qemu-arm" -L "$SDKTARGETSYSROOT" \
     -E LANG=C.UTF-8 -E QT_QPA_PLATFORM=offscreen -E QT_QUICK_BACKEND=software \
     -E QT_PLUGIN_PATH="$SDKTARGETSYSROOT/usr/lib/plugins" \
     -E QML_IMPORT_PATH="$SDKTARGETSYSROOT/usr/lib/qml" "$work/fixture"
else
    QT_QPA_PLATFORM=offscreen QT_QUICK_BACKEND=software "$work/fixture"
fi
