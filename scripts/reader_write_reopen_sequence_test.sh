#!/bin/sh
# No native backend. Original FIFO trace + real owned Qt/capture/file lifecycle.
set -eu
src=${1:?frozen source folder};assets=${2:?frozen producer asset folder};out=${3:?fresh output folder}
mode=${READER_WRITE_REOPEN_MODE:-host}
flags='-std=c++17 -O1 -g -Wall -Wextra -Werror -fPIC -pthread -fno-omit-frame-pointer'
case "$mode" in
 host) compiler=g++;moc=/usr/lib/qt6/libexec/moc;;
 asan) compiler=g++;moc=/usr/lib/qt6/libexec/moc;flags="$flags -fsanitize=address,undefined";;
 arm) . /opt/codex/rm2/5.8.203/environment-setup-cortexa7hf-neon-remarkable-linux-gnueabi;compiler=$CXX;moc="$OECORE_NATIVE_SYSROOT/usr/libexec/moc";;
 *) exit 90;;
esac
mkdir -p "$out"
mkdir -m 700 /tmp/owned-qt-runtime
export XDG_RUNTIME_DIR=/tmp/owned-qt-runtime
$compiler --version > "$out/compiler.txt"
pkg-config --modversion Qt6Core >> "$out/compiler.txt"
sha256sum "$src"/*.h "$src"/reader_selected_view_fixture.cpp "$src"/reader_write_reopen_sequence_fixture.cpp "$src"/reader_write_reopen_sequence_test.sh > "$out/input-hashes.txt"
(cd "$assets";sha256sum -c asset-hashes.txt) > "$out/asset-before.txt"
"$moc" "$src/reader_selected_view_probe.h" -o "$out/moc_reader_selected_view_probe.cpp"
"$moc" "$src/reader_selected_view_fixture.cpp" -o "$out/reader_selected_view_fixture.moc"
printf '#include "reader_write_reopen_sequence.h"\nstatic_assert(!reader_write_reopen::Result::nativeAuthority);\n' > "$out/production-header.cpp"
$compiler $flags -I"$src" -I"$out" $(pkg-config --cflags Qt6Core Qt6Gui Qt6Qml Qt6Quick) -c "$out/production-header.cpp" -o "$out/production-header.o"
$compiler $flags -I"$src" -I"$out" "$src/reader_write_reopen_sequence_fixture.cpp" "$out/moc_reader_selected_view_probe.cpp" $(pkg-config --cflags --libs Qt6Core Qt6Gui Qt6Qml Qt6Quick) -o "$out/fixture"
if [ "$mode" = arm ];then
 $READELF -h -A "$out/fixture" > "$out/elf.txt"
 timeout --kill-after=1s 120s "$OECORE_NATIVE_SYSROOT/usr/bin/qemu-arm" -L "$SDKTARGETSYSROOT" -E LANG=C.UTF-8 -E QT_QPA_PLATFORM=offscreen -E QT_QUICK_BACKEND=software -E QT_PLUGIN_PATH="$SDKTARGETSYSROOT/usr/lib/plugins" "$out/fixture" "$assets"
else
 QT_QPA_PLATFORM=offscreen QT_QUICK_BACKEND=software ASAN_OPTIONS=detect_leaks=0:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 timeout --kill-after=1s 120s "$out/fixture" "$assets"
fi
(cd "$assets";sha256sum -c asset-hashes.txt) > "$out/asset-after.txt"
sha256sum "$out/fixture" "$out/production-header.o" > "$out/artifact-hashes.txt"
sha256sum -c "$out/input-hashes.txt" > "$out/input-recheck.txt"
