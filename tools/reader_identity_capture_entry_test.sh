#!/bin/sh
# Owned files/navigation and explicitly injected capture producer. No device.
set -eu
root=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
temporary=
out=${READER_IDENTITY_CAPTURE_ENTRY_OUTPUT:-}
if [ -z "$out" ]; then out=$(mktemp -d); temporary=$out; fi
trap 'if [ -n "$temporary" ]; then rm -rf "$temporary"; fi' EXIT
mkdir -p "$out"
if [ -n "${QT_PROBE_SDK_ENV:-}" ]; then
 . "$QT_PROBE_SDK_ENV"
 moc="$OECORE_NATIVE_SYSROOT/usr/libexec/moc"
else
 moc=${QT_HOST_MOC:-/usr/lib/qt6/libexec/moc}
fi
flags='-std=c++17 -Wall -Wextra -Werror -O1 -g -fPIC -pthread'
if [ "${READER_IDENTITY_CAPTURE_ENTRY_SANITIZERS:-0}" = 1 ]; then flags="$flags -fsanitize=address,undefined -fno-omit-frame-pointer"; fi
${CXX:-c++} --version > "$out/compiler.txt"
pkg-config --modversion Qt6Core >> "$out/compiler.txt"
for name in reader_selected_view_probe reader_worker_callback_witness reader_joined_creation_witness; do
 "$moc" "$root/$name.h" -o "$out/moc_$name.cpp"
done
for name in reader_selected_view_fixture reader_fixture_navigation_fixture reader_navigation_ownership_fixture; do
 "$moc" "$root/$name.cpp" -o "$out/$name.moc"
done
"$moc" "$root/qt_qml_creation_bridge.h" -o "$out/qt_qml_creation_bridge.moc"
printf '#include "reader_selected_view_entry.h"\nstatic_assert(!reader_identity_capture::Result::nativeAuthority);\n' > "$out/header.cpp"
${CXX:-c++} $flags -I"$root" -I"$out" $(pkg-config --cflags Qt6Core Qt6Gui Qt6Qml Qt6Quick) -c "$out/header.cpp" -o "$out/header.o"
common="$out/moc_reader_selected_view_probe.cpp $out/moc_reader_worker_callback_witness.cpp $out/moc_reader_joined_creation_witness.cpp"
${CXX:-c++} $flags -I"$root" -I"$out" "$root/reader_identity_capture_entry_fixture.cpp" "$root/qt_qml_creation_bridge.cpp" $common \
 $(pkg-config --cflags --libs Qt6Core Qt6Gui Qt6Qml Qt6Quick) -ldl -o "$out/fixture"
# Preserve the existing fixture source and assertion body. Only its main symbol
# and implicit main return are adapted in a generated owned environment wrapper.
cat > "$out/legacy-wrapper.cpp" <<'CPP'
#define READER_ENTRY_OWNED_HELPERS_ONLY
#include "reader_selected_view_entry_fixture.cpp"
#undef READER_ENTRY_OWNED_HELPERS_ONLY
CPP
sed -n '/^#ifndef READER_ENTRY_OWNED_HELPERS_ONLY$/,/^#endif$/p' "$root/reader_selected_view_entry_fixture.cpp" | \
 sed '1d;$d' | sed '1s/int main/int legacyEntryMain/' | sed '$i\    return 0;' >> "$out/legacy-wrapper.cpp"
cat >> "$out/legacy-wrapper.cpp" <<'CPP'
int main(int argc,char **argv) {
 qmlRegisterType<QObject>("QtQml",2,0,"QtObject");
 qmlRegisterModule("QtQml.WorkerScript",QT_VERSION_MAJOR,QT_VERSION_MINOR);
 qmlRegisterModule("QtQml.Models",QT_VERSION_MAJOR,QT_VERSION_MINOR);
 return legacyEntryMain(argc,argv);
}
CPP
${CXX:-c++} $flags -I"$root" -I"$out" "$out/legacy-wrapper.cpp" "$root/qt_qml_creation_bridge.cpp" $common \
 $(pkg-config --cflags --libs Qt6Core Qt6Gui Qt6Qml Qt6Quick) -ldl -o "$out/legacy-fixture"
${CXX:-c++} -std=c++17 -Wall -Wextra -Werror -O2 "$root/reader_selected_request_publish.cpp" -o "$out/publisher"
chmod 700 "$out/publisher"
if [ -n "${QT_PROBE_SDK_ENV:-}" ]; then
 "$READELF" -h -A "$out/fixture" > "$out/elf.txt"
 # This exact startup variant is compiled/linked only and never loaded.
 ${CXX:-c++} $flags -shared -fvisibility=hidden -Wl,-z,defs -I"$root" -I"$out" \
  '-DREADER_CAPTURE_NONCE="cccccccccccccccccccccccccccccccc"' \
  '-DREADER_CAPTURE_DOCUMENT_ID="10000000-0000-0000-0000-000000000001"' \
  '-DREADER_CAPTURE_PAGE_ID="00000000-0000-4000-8000-000000000002"' \
  -DREADER_CAPTURE_PAGE_INDEX=1 -DREADER_CAPTURE_PAGE_COUNT=3 \
  "$root/reader_identity_capture_startup.cpp" "$root/qt_qml_creation_bridge.cpp" $common \
  $(pkg-config --cflags --libs Qt6Core Qt6Gui Qt6Qml Qt6Quick) -ldl -o "$out/test-only-startup.so"
 timeout --kill-after=1s 100s "$OECORE_NATIVE_SYSROOT/usr/bin/qemu-arm" -L "$SDKTARGETSYSROOT" \
  -E LANG=C.UTF-8 -E QT_QPA_PLATFORM=offscreen -E QT_QUICK_BACKEND=software \
  -E READER_IDENTITY_CAPTURE_ENTRY_EXPORT="$out/serializer-inputs" \
  -E QT_PLUGIN_PATH="$SDKTARGETSYSROOT/usr/lib/plugins" "$out/fixture"
 timeout --kill-after=1s 150s "$OECORE_NATIVE_SYSROOT/usr/bin/qemu-arm" -L "$SDKTARGETSYSROOT" \
  -E LANG=C.UTF-8 -E QT_QPA_PLATFORM=offscreen -E QT_QUICK_BACKEND=software \
  -E QT_PLUGIN_PATH="$SDKTARGETSYSROOT/usr/lib/plugins" \
  -E READER_SELECTED_HELPER_QEMU="$OECORE_NATIVE_SYSROOT/usr/bin/qemu-arm" \
  -E READER_SELECTED_HELPER_SYSROOT="$SDKTARGETSYSROOT" "$out/legacy-fixture" "$out/publisher"
else
 READER_IDENTITY_CAPTURE_ENTRY_EXPORT="$out/serializer-inputs" QT_QPA_PLATFORM=offscreen QT_QUICK_BACKEND=software ASAN_OPTIONS=detect_leaks=0:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 \
  timeout --kill-after=1s 100s "$out/fixture"
 QT_QPA_PLATFORM=offscreen QT_QUICK_BACKEND=software ASAN_OPTIONS=detect_leaks=0:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 \
  timeout --kill-after=1s 150s "$out/legacy-fixture" "$out/publisher"
fi
sha256sum "$out/fixture" "$out/header.o" "$out/legacy-fixture" "$out/publisher" > "$out/artifact-hashes.txt"
