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
"$moc" scripts/insertion_notification_order_fixture.cpp -o "$work/insertion_notification_order_fixture.moc"
$compiler -std=c++17 -Wall -Wextra -Werror -O2 -fPIC -I"$work" \
    scripts/insertion_notification_order_fixture.cpp $(pkg-config --cflags --libs Qt6Core) -o "$work/fixture"
if [ -n "${QT_PROBE_BINARY_OUT:-}" ]; then cp "$work/fixture" "$QT_PROBE_BINARY_OUT"; fi
if [ -n "${QT_PROBE_SDK_ENV:-}" ]; then
    "$OECORE_NATIVE_SYSROOT/usr/bin/qemu-arm" -L "$SDKTARGETSYSROOT" "$work/fixture"
else
    "$work/fixture"
fi
