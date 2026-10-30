#!/bin/sh
# Owned callback queues only; no native ABI, device, installation or startup.
set -eu
work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT
if [ -n "${QT_PROBE_SDK_ENV:-}" ]; then
    . "$QT_PROBE_SDK_ENV"
    moc="$OECORE_NATIVE_SYSROOT/usr/libexec/moc"
else
    moc=${QT_HOST_MOC:-/usr/lib/qt6/libexec/moc}
fi
"$moc" scripts/reader_worker_callback_witness.h -o "$work/moc_reader_worker_callback_witness.cpp"
"$moc" scripts/reader_worker_callback_witness_fixture.cpp -o "$work/reader_worker_callback_witness_fixture.moc"
${CXX:-c++} -std=c++17 -Wall -Wextra -Werror -O2 -fPIC -pthread -I"$work" -Iscripts \
    scripts/reader_worker_callback_witness_fixture.cpp "$work/moc_reader_worker_callback_witness.cpp" \
    $(pkg-config --cflags --libs Qt6Gui Qt6Core) -o "$work/fixture"
if [ -n "${QT_PROBE_BINARY_OUT:-}" ]; then cp "$work/fixture" "$QT_PROBE_BINARY_OUT"; fi
if [ -n "${QT_PROBE_SDK_ENV:-}" ]; then
    "$OECORE_NATIVE_SYSROOT/usr/bin/qemu-arm" -L "$SDKTARGETSYSROOT" \
        -E LANG=C.UTF-8 -E QT_QPA_PLATFORM=offscreen \
        -E QT_PLUGIN_PATH="$SDKTARGETSYSROOT/usr/lib/plugins" "$work/fixture"
else
    QT_QPA_PLATFORM=offscreen "$work/fixture"
fi
