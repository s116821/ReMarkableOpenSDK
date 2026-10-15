#!/bin/sh
# Focused owned-QML checks only; builds no target preload and selects no nonce.
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
"$moc" tools/qt_qml_creation_bridge.h -o "$work/qt_qml_creation_bridge.moc"
"$moc" tools/qt_page_open.h -o "$work/qt_page_open.moc"
"$moc" tools/qt_qml_creation_fixture.cpp -o "$work/qt_qml_creation_fixture.moc"
$compiler -std=c++17 -Wall -Wextra -Werror -O2 -fPIC -I"$work" \
  tools/qt_qml_creation_fixture.cpp tools/qt_qml_creation_bridge.cpp tools/qt_page_open.cpp \
  $(pkg-config --cflags --libs Qt6Qml Qt6Gui Qt6Quick) -o "$work/creation-fixture"
$compiler -std=c++17 -Wall -Wextra -Werror -O2 -fPIC \
  tools/qt_creation_deadline_fixture.cpp $(pkg-config --cflags --libs Qt6Core) -o "$work/deadline-fixture"
if [ -n "${QT_PROBE_SDK_ENV:-}" ]; then
  "$OECORE_NATIVE_SYSROOT/usr/bin/qemu-arm" -L "$SDKTARGETSYSROOT" "$work/deadline-fixture"
else
  "$work/deadline-fixture"
fi
for case in eight-source5-config dev-target-eight-source5 dev-target-eight-source5-before-expiry dev-target-eight-source5-setup-expiry dev-target-eight-source5-access-expiry dev-target-eight-source5-order dev-target-eight-source5-count dev-target-eight-source5-reverse dev-target-eight-source5-no-target-generated eight-config dev-target-eight dev-target-eight-setup-expiry dev-target-eight-access-expiry target-config dev-target-success dev-target-seven dev-target-seven-suspend dev-target-seven-clock-failure dev-target-seven-95 dev-target-seven-95-setup-expiry dev-target-seven-95-access-expiry dev-target-count dev-target-order dev-ready-success repeat; do
  if [ -n "${QT_PROBE_SDK_ENV:-}" ]; then
    "$OECORE_NATIVE_SYSROOT/usr/bin/qemu-arm" -L "$SDKTARGETSYSROOT" \
      -E LANG=C.UTF-8 -E QT_QPA_PLATFORM=offscreen \
      -E QT_PLUGIN_PATH="$SDKTARGETSYSROOT/usr/lib/plugins" \
      -E QML_IMPORT_PATH="$SDKTARGETSYSROOT/usr/lib/qml" "$work/creation-fixture" "$case"
  else
    QT_QPA_PLATFORM=offscreen "$work/creation-fixture" "$case"
  fi
done
