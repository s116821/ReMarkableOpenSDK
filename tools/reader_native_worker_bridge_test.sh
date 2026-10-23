#!/bin/sh
# Owned ABI mocks only; pinned loader must reject this fixture executable.
set -eu
work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT
if [ -n "${QT_PROBE_SDK_ENV:-}" ]; then
    . "$QT_PROBE_SDK_ENV"
    moc="$OECORE_NATIVE_SYSROOT/usr/libexec/moc"
else
    moc=${QT_HOST_MOC:-/usr/lib/qt6/libexec/moc}
fi
"$moc" tools/reader_native_worker_bridge_fixture.cpp -o "$work/reader_native_worker_bridge_fixture.moc"
printf '%s\n' '#include "reader_native_worker_bridge.h"' > "$work/production-header.cpp"
${CXX:-c++} -std=c++17 -Wall -Wextra -Werror -O2 -fPIC ${READER_NATIVE_SANITIZE_FLAGS:-} -Itools \
    $(pkg-config --cflags Qt6Core) -c "$work/production-header.cpp" -o "$work/production-header.o"
${CXX:-c++} -std=c++17 -Wall -Wextra -Werror -O2 -fPIC ${READER_NATIVE_SANITIZE_FLAGS:-} -I"$work" -Itools \
    tools/reader_native_worker_bridge_fixture.cpp $(pkg-config --cflags --libs Qt6Core) -ldl -pthread -o "$work/fixture"
if [ -n "${QT_PROBE_BINARY_OUT:-}" ]; then cp "$work/fixture" "$QT_PROBE_BINARY_OUT"; fi
if [ -n "${QT_PROBE_SDK_ENV:-}" ]; then
    "$OECORE_NATIVE_SYSROOT/usr/bin/qemu-arm" -L "$SDKTARGETSYSROOT" -E LANG=C.UTF-8 "$work/fixture"
else
    "$work/fixture"
fi
