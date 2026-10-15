#!/bin/sh
# New reader-structure source/owned controls only; no startup shared artifact.
set -eu
. "$QT_PROBE_SDK_ENV"
if [ -n "${VISUAL_READER_EVIDENCE_DIR:-}" ]; then
 work="$VISUAL_READER_EVIDENCE_DIR/work"
 mkdir "$work"
else
 work=$(mktemp -d)
 trap 'rm -rf "$work"' EXIT
fi
$CXX --version > "$work/compiler-version.txt"
pkg-config --cflags --libs Qt6Qml Qt6Gui Qt6Quick > "$work/qt-flags.txt"
"$OECORE_NATIVE_SYSROOT/usr/libexec/moc" tools/visual_reader_structure_fixture.cpp -o "$work/visual_reader_structure_fixture.moc" \
 > "$work/moc.stdout" 2> "$work/moc.stderr"
$CXX -std=c++17 -Wall -Wextra -Werror -O2 -fPIC -I"$work" tools/visual_reader_structure_fixture.cpp \
 $(pkg-config --cflags --libs Qt6Qml Qt6Gui Qt6Quick) -o "$work/fixture" \
 > "$work/compile.stdout" 2> "$work/compile.stderr"
$READELF -h -d -V "$work/fixture" > "$work/fixture.elf.txt"
sha256sum "$work/fixture" > "$work/fixture.sha256.txt"
base='-DVISUAL_CAPTURE_PASSIVE_FRAME_READY=1 -DVISUAL_CAPTURE_BINDING_WITNESS=1 -DVISUAL_CAPTURE_FRAME_SWAPPED=1'
for profile in default reader; do
 flags=''
 if [ "$profile" = reader ]; then flags="$base -DVISUAL_CAPTURE_READER_STRUCTURE=1"; fi
 $CXX -std=c++17 -Wall -Wextra -Werror $flags '-DVISUAL_CAPTURE_NONCE="aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"' \
  $(pkg-config --cflags Qt6Qml Qt6Gui Qt6Quick) -fsyntax-only tools/visual_capture_startup.cpp \
  > "$work/$profile-syntax.stdout" 2> "$work/$profile-syntax.stderr"
done
for refusal in zero two passive witness swapped; do
 case "$refusal" in
  zero) flags="$base -DVISUAL_CAPTURE_READER_STRUCTURE=0";;
  two) flags="$base -DVISUAL_CAPTURE_READER_STRUCTURE=2";;
  passive) flags='-DVISUAL_CAPTURE_READER_STRUCTURE=1 -DVISUAL_CAPTURE_BINDING_WITNESS=1 -DVISUAL_CAPTURE_FRAME_SWAPPED=1';;
  witness) flags='-DVISUAL_CAPTURE_READER_STRUCTURE=1 -DVISUAL_CAPTURE_PASSIVE_FRAME_READY=1 -DVISUAL_CAPTURE_FRAME_SWAPPED=1';;
  swapped) flags='-DVISUAL_CAPTURE_READER_STRUCTURE=1 -DVISUAL_CAPTURE_PASSIVE_FRAME_READY=1 -DVISUAL_CAPTURE_BINDING_WITNESS=1';;
 esac
 if $CXX -std=c++17 $flags '-DVISUAL_CAPTURE_NONCE="aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"' \
  $(pkg-config --cflags Qt6Qml Qt6Gui Qt6Quick) -fsyntax-only tools/visual_capture_startup.cpp \
  > "$work/refuse-$refusal.stdout" 2> "$work/refuse-$refusal.stderr"; then exit 1; fi
 grep -q 'Reader structure requires explicit value 1' "$work/refuse-$refusal.stderr"
done
if $CXX -std=c++17 $base -DVISUAL_CAPTURE_READER_STRUCTURE=1 -DVISUAL_CAPTURE_TESTING \
 '-DVISUAL_CAPTURE_NONCE="aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"' $(pkg-config --cflags Qt6Qml Qt6Gui Qt6Quick) \
 -fsyntax-only tools/visual_capture_startup.cpp > "$work/test-fence.stdout" 2> "$work/test-fence.stderr"; then exit 1; fi
grep -q 'Fixture-only clock/publication hooks must not enter' "$work/test-fence.stderr"
"$OECORE_NATIVE_SYSROOT/usr/bin/qemu-arm" -L "$SDKTARGETSYSROOT" \
 -E LANG=C.UTF-8 -E QT_QPA_PLATFORM=offscreen -E QT_QUICK_BACKEND=software \
 -E QT_PLUGIN_PATH="$SDKTARGETSYSROOT/usr/lib/plugins" -E QML_IMPORT_PATH="$SDKTARGETSYSROOT/usr/lib/qml" \
 "$work/fixture" > "$work/control.stdout" 2> "$work/control.stderr"
cat "$work/control.stdout"
