#!/bin/sh
# Owned GUI stack/cleanup model only. No device, installation or native backend.
set -eu
root=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
work=${READER_RETIREMENT_OUTPUT:?fresh output directory required}
test ! -e "$work"
mkdir -p "$work"
if [ -n "${QT_PROBE_SDK_ENV:-}" ]; then
    . "$QT_PROBE_SDK_ENV"
    moc="$OECORE_NATIVE_SYSROOT/usr/libexec/moc"
else
    moc=${QT_HOST_MOC:-/usr/lib/qt6/libexec/moc}
fi
"$moc" "$root/reader_request_retirement.h" -o "$work/moc_reader_request_retirement.cpp"
"$moc" "$root/reader_request_retirement_fixture.cpp" -o "$work/reader_request_retirement_fixture.moc"
sanitize=
if [ "${READER_RETIREMENT_SANITIZERS:-0}" = 1 ]; then sanitize="-fsanitize=address,undefined -fno-omit-frame-pointer -no-pie"; fi
${CXX:-c++} $sanitize -std=c++17 -Wall -Wextra -Werror -O2 -fPIC -pthread -I"$work" -I"$root" \
    "$root/reader_request_retirement_fixture.cpp" "$work/moc_reader_request_retirement.cpp" \
    $(pkg-config --cflags --libs Qt6Core Qt6Gui Qt6Qml Qt6Quick) -o "$work/fixture"
if [ -n "${QT_PROBE_BINARY_OUT:-}" ]; then cp "$work/fixture" "$QT_PROBE_BINARY_OUT"; fi
if [ -n "${QT_PROBE_SDK_ENV:-}" ]; then
    timeout --kill-after=1s 60s "$OECORE_NATIVE_SYSROOT/usr/bin/qemu-arm" -L "$SDKTARGETSYSROOT" \
        -E LANG=C.UTF-8 -E QT_QPA_PLATFORM=offscreen \
        -E QT_PLUGIN_PATH="$SDKTARGETSYSROOT/usr/lib/plugins" "$work/fixture"
else
    QT_QPA_PLATFORM=offscreen ASAN_OPTIONS=detect_leaks=0:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 timeout --kill-after=1s 60s "$work/fixture"
fi
