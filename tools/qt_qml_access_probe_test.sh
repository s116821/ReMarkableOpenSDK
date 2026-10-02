#!/bin/sh
set -eu
if [ -n "${QT_PROBE_SDK_ENV:-}" ]; then
  . "$QT_PROBE_SDK_ENV"
fi
compiler=${CXX:-c++}
work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT
$compiler -std=c++17 -Wall -Wextra -Werror -O2 -fPIC \
  tools/qt_qml_access_probe_fixture.cpp $(pkg-config --cflags --libs Qt6Qml Qt6Gui) -o "$work/fixture"
$compiler -std=c++17 -Wall -Wextra -Werror -O2 -fPIC -shared -fvisibility=hidden -Wl,-z,defs \
  '-DQT_PROBE_NONCE="0123456789abcdef0123456789abcdef"' \
  tools/qt_qml_access_probe.cpp $(pkg-config --cflags --libs Qt6Qml Qt6Gui) -o "$work/probe.so"
export QT_QPA_PLATFORM=offscreen
for case in diagnostics classifier compiler-errors live major-version existing late absent conflict multiple window-cap engine-lost cap deadline nested-deadline cancel; do
  if [ -n "${QT_PROBE_SDK_ENV:-}" ]; then
    /opt/codex/rm2/5.8.203/sysroots/x86_64-codexsdk-linux/usr/bin/qemu-arm \
      -L "$SDKTARGETSYSROOT" -E LANG=C.UTF-8 -E QT_QPA_PLATFORM=offscreen \
      -E QT_PLUGIN_PATH="$SDKTARGETSYSROOT/usr/lib/plugins" \
      -E QML_IMPORT_PATH="$SDKTARGETSYSROOT/usr/lib/qml" "$work/fixture" "$case"
  else
    "$work/fixture" "$case"
  fi
done
