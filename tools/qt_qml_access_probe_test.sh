#!/bin/sh
set -eu
if [ -n "${QT_PROBE_SDK_ENV:-}" ]; then
  . "$QT_PROBE_SDK_ENV"
fi
compiler=${CXX:-c++}
work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT
if [ -n "${QT_PROBE_SDK_ENV:-}" ]; then
  moc="$OECORE_NATIVE_SYSROOT/usr/libexec/moc"
else
  moc="$(pkg-config --variable=libexecdir Qt6Core)/moc"
fi
"$moc" tools/qt_qml_access_probe_metadata_fixture.cpp -o "$work/qt_qml_access_probe_metadata_fixture.moc"
$compiler -std=c++17 -Wall -Wextra -Werror -O2 -fPIC -I"$work" \
  tools/qt_qml_access_probe_metadata_fixture.cpp $(pkg-config --cflags --libs Qt6Qml Qt6Gui Qt6Quick) -o "$work/metadata-fixture"
$compiler -std=c++17 -Wall -Wextra -Werror -O2 -fPIC \
  tools/qt_qml_access_probe_fixture.cpp $(pkg-config --cflags --libs Qt6Qml Qt6Gui Qt6Quick) -o "$work/fixture"
$compiler -std=c++17 -Wall -Wextra -Werror -O2 -fPIC -shared -fvisibility=hidden -Wl,-z,defs \
  '-DQT_PROBE_NONCE="0123456789abcdef0123456789abcdef"' \
  tools/qt_qml_access_probe.cpp $(pkg-config --cflags --libs Qt6Qml Qt6Gui Qt6Quick) -o "$work/probe.so"
export QT_QPA_PLATFORM=offscreen
for case in metadata-good metadata-multiple metadata-mismatch metadata-empty metadata-wrong-return metadata-node-cap metadata-node-boundary metadata-depth-cap metadata-depth-boundary metadata-candidate-cap metadata-candidate-boundary metadata-superclass-cap metadata-superclass-boundary metadata-window-cap metadata-unknown-engine metadata-other-engine metadata-controller-lost metadata-item-lost metadata-window-lost metadata-thread metadata-deadline metadata-mid-deadline metadata-probe-good metadata-probe-refusal metadata-probe-cancel metadata-probe-deadline metadata-probe-controller-lost metadata-probe-engine-lost metadata-probe-ambiguous; do
  if [ -n "${QT_PROBE_SDK_ENV:-}" ]; then
    /opt/codex/rm2/5.8.203/sysroots/x86_64-codexsdk-linux/usr/bin/qemu-arm \
      -L "$SDKTARGETSYSROOT" -E LANG=C.UTF-8 -E QT_QPA_PLATFORM=offscreen \
      -E QT_PLUGIN_PATH="$SDKTARGETSYSROOT/usr/lib/plugins" \
      -E QML_IMPORT_PATH="$SDKTARGETSYSROOT/usr/lib/qml" "$work/metadata-fixture" "$case"
  else
    "$work/metadata-fixture" "$case"
  fi
done
for case in diagnostics classifier compiler-errors live major-version existing late absent retry-late retry-no-event retry-stale retry-changed retry-ambiguous retry-cap wrong-error conflict multiple window-cap engine-lost cap deadline nested-deadline cancel root-preexisting root-signal root-duplicate root-wait root-failed root-cap root-unsupported root-lost root-engine-lost root-changed root-ambiguous root-queued-readiness-deadline root-cancel root-wrong-thread root-late-witness root-stale-failure root-cancel-queued retry-delayed retry-cancel retry-engine-lost phase-delayed-ready phase-late-ready phase-boundary phase-ready-changed phase-ready-ambiguous; do
  if [ -n "${QT_PROBE_SDK_ENV:-}" ]; then
    /opt/codex/rm2/5.8.203/sysroots/x86_64-codexsdk-linux/usr/bin/qemu-arm \
      -L "$SDKTARGETSYSROOT" -E LANG=C.UTF-8 -E QT_QPA_PLATFORM=offscreen \
      -E QT_PLUGIN_PATH="$SDKTARGETSYSROOT/usr/lib/plugins" \
      -E QML_IMPORT_PATH="$SDKTARGETSYSROOT/usr/lib/qml" "$work/fixture" "$case"
  else
    "$work/fixture" "$case"
  fi
done
