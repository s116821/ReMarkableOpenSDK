#!/bin/sh
# Owned entry fixture plus startup compile. No device or stock application use.
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
"$moc" tools/reader_selected_view_fixture.cpp -o "$work/reader_selected_view_fixture.moc"
${CXX:-c++} -std=c++17 -Wall -Wextra -Werror -O2 tools/reader_selected_request_publish.cpp -o "$work/publisher"
chmod 700 "$work/publisher"
${CXX:-c++} -std=c++17 -Wall -Wextra -Werror -O2 -fPIC -I"$work" -Itools \
    tools/reader_selected_view_entry_fixture.cpp "$work/moc_reader_selected_view_probe.cpp" \
    $(pkg-config --cflags --libs Qt6Qml Qt6Gui Qt6Quick) -o "$work/fixture"
# Test nonce only. This startup library is compiled, not injected or installed.
${CXX:-c++} -std=c++17 -Wall -Wextra -Werror -O2 -fPIC -shared -fvisibility=hidden -Wl,-z,defs -Itools \
    '-DREADER_SELECTED_NONCE="aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"' \
    tools/reader_selected_view_startup.cpp "$work/moc_reader_selected_view_probe.cpp" \
    $(pkg-config --cflags --libs Qt6Qml Qt6Gui Qt6Quick) -o "$work/test-only-startup.so"
if [ -n "${QT_PROBE_SDK_ENV:-}" ]; then
    "$OECORE_NATIVE_SYSROOT/usr/bin/qemu-arm" -L "$SDKTARGETSYSROOT" \
        -E LANG=C.UTF-8 -E QT_QPA_PLATFORM=offscreen -E QT_QUICK_BACKEND=software \
        -E QT_PLUGIN_PATH="$SDKTARGETSYSROOT/usr/lib/plugins" \
        -E QML_IMPORT_PATH="$SDKTARGETSYSROOT/usr/lib/qml" \
        -E READER_SELECTED_HELPER_QEMU="$OECORE_NATIVE_SYSROOT/usr/bin/qemu-arm" \
        -E READER_SELECTED_HELPER_SYSROOT="$SDKTARGETSYSROOT" "$work/fixture" "$work/publisher"
else
    QT_QPA_PLATFORM=offscreen QT_QUICK_BACKEND=software "$work/fixture" "$work/publisher"
fi
