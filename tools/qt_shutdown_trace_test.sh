#!/bin/sh
set -eu
# Use the pinned RM2 SDK container with source read-only, /out writable and
# ephemeral /run,/tmp. These all-zero fixture roots are never a selected packet.
. /opt/codex/rm2/5.8.203/environment-setup-cortexa7hf-neon-remarkable-linux-gnueabi
cd /src
$CXX -std=c++17 -Wall -Wextra -Werror -O2 -fPIC -shared -Wl,-z,defs \
 '-DQT_SHUTDOWN_NONCE="00000000000000000000000000000000"' \
 tools/qt_shutdown_trace_startup.cpp $(pkg-config --cflags --libs Qt6Quick Qt6Gui Qt6Core) -o /out/shutdown-production-compile-only.so
$CXX -std=c++17 -Wall -Wextra -Werror -O2 -fPIC tools/qt_shutdown_trace_fixture.cpp \
 $(pkg-config --cflags --libs Qt6Quick Qt6Gui Qt6Core) -o /out/shutdown-owned-fixture
"$OECORE_NATIVE_SYSROOT/usr/bin/qemu-arm" -L "$SDKTARGETSYSROOT" \
 -E QT_QPA_PLATFORM=offscreen -E QT_QUICK_BACKEND=software \
 -E QT_PLUGIN_PATH="$SDKTARGETSYSROOT/usr/lib/plugins" /out/shutdown-owned-fixture
