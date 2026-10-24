#!/bin/sh
# Owned model only; no private target call, target ELF loading or device contact.
set -eu
work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT
if [ -n "${QT_PROBE_SDK_ENV:-}" ]; then
    . "$QT_PROBE_SDK_ENV"
    moc="$OECORE_NATIVE_SYSROOT/usr/libexec/moc"
else
    moc=${QT_HOST_MOC:-/usr/lib/qt6/libexec/moc}
fi
"$moc" tools/reader_worker_owner_quarantine.h -o "$work/moc_quarantine.cpp"
"$moc" tools/reader_worker_owner_quarantine_fixture.cpp -o "$work/reader_worker_owner_quarantine_fixture.moc"
${CXX:-c++} -std=c++17 -Wall -Wextra -Werror -O2 -fPIC -pthread ${READER_QUARANTINE_SANITIZE_FLAGS:-} -I"$work" -Itools \
    tools/reader_worker_owner_quarantine_fixture.cpp "$work/moc_quarantine.cpp" \
    $(pkg-config --cflags --libs Qt6Core Qt6Gui) -o "$work/fixture"
if [ -n "${QT_PROBE_BINARY_OUT:-}" ]; then cp "$work/fixture" "$QT_PROBE_BINARY_OUT"; fi
if [ -n "${QT_PROBE_SDK_ENV:-}" ]; then
    "$OECORE_NATIVE_SYSROOT/usr/bin/qemu-arm" -L "$SDKTARGETSYSROOT" \
        -E LANG=C.UTF-8 -E QT_QPA_PLATFORM=offscreen \
        -E QT_PLUGIN_PATH="$SDKTARGETSYSROOT/usr/lib/plugins" "$work/fixture"
else
    QT_QPA_PLATFORM=offscreen "$work/fixture"
fi
