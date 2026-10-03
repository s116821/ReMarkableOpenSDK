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
"$moc" tools/qt_qml_creation_bridge.h -o "$work/qt_qml_creation_bridge.moc"
"$moc" tools/qt_qml_creation_fixture.cpp -o "$work/qt_qml_creation_fixture.moc"
"$moc" tools/qt_qml_access_probe_metadata_fixture.cpp -o "$work/qt_qml_access_probe_metadata_fixture.moc"
$compiler -std=c++17 -Wall -Wextra -Werror -O2 -fPIC -I"$work" \
  tools/qt_qml_access_probe_metadata_fixture.cpp tools/qt_qml_creation_bridge.cpp $(pkg-config --cflags --libs Qt6Qml Qt6Gui Qt6Quick) -o "$work/metadata-fixture"
$compiler -std=c++17 -Wall -Wextra -Werror -O2 -fPIC -I"$work" \
  tools/qt_qml_access_probe_fixture.cpp tools/qt_qml_creation_bridge.cpp $(pkg-config --cflags --libs Qt6Qml Qt6Gui Qt6Quick) -o "$work/fixture"
$compiler -std=c++17 -Wall -Wextra -Werror -O2 -fPIC -I"$work" -shared -fvisibility=hidden -Wl,-z,defs \
  '-DQT_PROBE_NONCE="0123456789abcdef0123456789abcdef"' \
  tools/qt_qml_access_probe.cpp tools/qt_qml_creation_bridge.cpp $(pkg-config --cflags --libs Qt6Qml Qt6Gui Qt6Quick) -o "$work/probe.so"
$compiler -std=c++17 -Wall -Wextra -Werror -O2 -fPIC -I"$work" \
  tools/qt_qml_creation_fixture.cpp tools/qt_qml_creation_bridge.cpp $(pkg-config --cflags --libs Qt6Qml Qt6Gui Qt6Quick) -o "$work/creation-fixture"
cat > "$work/creation_config.h" <<'CONFIG'
inline qml_access::CreationConfig creationConfig() {
  return {QStringLiteral("00000000-0000-4000-8000-000000000001"),
    {QStringLiteral("00000000-0000-4000-8000-000000000002"), QStringLiteral("00000000-0000-4000-8000-000000000003"),
     QStringLiteral("00000000-0000-4000-8000-000000000004"), QStringLiteral("00000000-0000-4000-8000-000000000005"),
     QStringLiteral("00000000-0000-4000-8000-000000000006")}, true};
}
CONFIG
$compiler -std=c++17 -Wall -Wextra -Werror -O2 -fPIC -I"$work" -shared -fvisibility=hidden -Wl,-z,defs \
  '-DQT_PROBE_NONCE="0123456789abcdef0123456789abcdef"' '-DQT_PROBE_CREATION_CONFIG="creation_config.h"' \
  tools/qt_qml_access_probe.cpp tools/qt_qml_creation_bridge.cpp $(pkg-config --cflags --libs Qt6Qml Qt6Gui Qt6Quick) -o "$work/creation-probe.so"
export QT_QPA_PLATFORM=offscreen
navigation_cases="navigation-good navigation-namespace navigation-other-pointer navigation-empty navigation-missing navigation-nonpointer navigation-fake-focus navigation-split navigation-multiple navigation-mixed navigation-wrong-kind navigation-wrong-slot navigation-wrong-arity navigation-wrong-parameters navigation-candidate-boundary navigation-candidate-cap navigation-unreadable navigation-opaque navigation-lexical navigation-positive-loss navigation-positive-deadline navigation-probe-good navigation-probe-absent navigation-probe-incompatible navigation-probe-mixed navigation-probe-refusal navigation-probe-navigation-cap navigation-probe-cancel navigation-probe-deadline navigation-probe-controller-lost navigation-probe-engine-lost navigation-probe-ambiguous"
for case in $navigation_cases metadata-good metadata-multiple metadata-mismatch metadata-empty metadata-wrong-return metadata-node-cap metadata-node-boundary metadata-depth-cap metadata-depth-boundary metadata-candidate-cap metadata-candidate-boundary metadata-superclass-cap metadata-superclass-boundary metadata-window-cap metadata-unknown-engine metadata-other-engine metadata-controller-lost metadata-item-lost metadata-window-lost metadata-thread metadata-deadline metadata-mid-deadline metadata-probe-good metadata-probe-refusal metadata-probe-cancel metadata-probe-deadline metadata-probe-controller-lost metadata-probe-engine-lost metadata-probe-ambiguous; do
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

for case in async sync duplicate burst repeat enum missing identity type exporting status count page0 later-page reorder roundtrip template empty-template getter-throw template-throw lookup-throw native-throw false-async false-no-callback no-callback unknown-return preclaim-cancel reentrant-cancel nested-timeout bad-config method-missing foreign-bridge entry-absent library-absent id-read-throw id-string-throw type-read-throw status-read-throw count-read-throw index-getter-throw marker-privacy; do
  if [ -n "${QT_PROBE_SDK_ENV:-}" ]; then
    /opt/codex/rm2/5.8.203/sysroots/x86_64-codexsdk-linux/usr/bin/qemu-arm \
      -L "$SDKTARGETSYSROOT" -E LANG=C.UTF-8 -E QT_QPA_PLATFORM=offscreen \
      -E QT_PLUGIN_PATH="$SDKTARGETSYSROOT/usr/lib/plugins" \
      -E QML_IMPORT_PATH="$SDKTARGETSYSROOT/usr/lib/qml" "$work/creation-fixture" "$case"
  else
    "$work/creation-fixture" "$case"
  fi
done
