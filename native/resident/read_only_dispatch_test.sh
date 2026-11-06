#!/bin/sh
# Host Qt regression only. Does not load a native adapter or contact a tablet.
set -eu
out=${READ_ONLY_DISPATCH_TEST_OUTPUT:?fresh output directory required}
test ! -e "$out"
root=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
if [ -n "${QT_PROBE_SDK_ENV:-}" ]; then
    . "$QT_PROBE_SDK_ENV"
    moc="$OECORE_NATIVE_SYSROOT/usr/libexec/moc"
else
    moc=${QT_HOST_MOC:-/usr/lib/qt6/libexec/moc}
fi
pkg-config --print-errors --exists Qt6Core Qt6Gui Qt6Network Qt6Qml Qt6Quick
test -x "$moc"
${CXX:-c++} -std=c++17 -Wall -Wextra -Werror -fPIC -pthread -fsyntax-only \
    "$root/../rm1_observation/bootstrap.cpp" \
    $(pkg-config --cflags Qt6Core Qt6Gui Qt6Network Qt6Qml Qt6Quick)
mkdir "$out"
"$moc" "$root/../rm2_3_28_0_172/reader_selected_view_probe.h" -o "$out/moc_reader_selected_view_probe.cpp"
${CXX:-c++} -std=c++17 -Wall -Wextra -Werror -O1 -g -fPIC -pthread \
    "$root/read_only_dispatch_test.cpp" "$root/../rm1_observation/admission.cpp" \
    "$out/moc_reader_selected_view_probe.cpp" \
    $(pkg-config --cflags --libs Qt6Core Qt6Gui Qt6Network Qt6Qml Qt6Quick) -ldl \
    -o "$out/read-only-dispatch-test"
if [ -n "${QT_PROBE_SDK_ENV:-}" ]; then
    timeout --kill-after=1s 20s "$OECORE_NATIVE_SYSROOT/usr/bin/qemu-arm" -L "$SDKTARGETSYSROOT" \
        -E LANG=C.UTF-8 -E QT_QPA_PLATFORM=offscreen -E QT_PLUGIN_PATH="$SDKTARGETSYSROOT/usr/lib/plugins" \
        "$out/read-only-dispatch-test"
else
    QT_QPA_PLATFORM=offscreen timeout --kill-after=1s 20s "$out/read-only-dispatch-test"
fi
