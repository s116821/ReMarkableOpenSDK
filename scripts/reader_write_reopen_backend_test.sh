#!/bin/sh
set -eu
src=${1:?frozen source};out=${2:?fresh output}
mode=${READER_BACKEND_MODE:-host}
mkdir -p "$out"
flags='-std=c++17 -O1 -g -Wall -Wextra -Werror -Wno-psabi -fPIC -pthread -fno-omit-frame-pointer'
case "$mode" in
host) compiler=g++;moc=/usr/lib/qt6/libexec/moc;;
asan) compiler=g++;moc=/usr/lib/qt6/libexec/moc;flags="$flags -fsanitize=address,undefined";;
arm) . /opt/codex/rm2/5.8.203/environment-setup-cortexa7hf-neon-remarkable-linux-gnueabi;compiler=$CXX;moc="$OECORE_NATIVE_SYSROOT/usr/libexec/moc";;
*) exit 90;;
esac
$compiler --version > "$out/compiler.txt"
pkg-config --modversion Qt6Core >> "$out/compiler.txt"
sha256sum "$src"/* > "$out/input-hashes.txt"
"$moc" "$src/reader_selected_view_probe.h" -o "$out/moc_probe.cpp"
$compiler $flags -I"$src" "$src/reader_write_reopen_backend_fixture.cpp" "$out/moc_probe.cpp" $(pkg-config --cflags --libs Qt6Core Qt6Gui Qt6Qml Qt6Quick) -o "$out/fixture"
if [ "$mode" = arm ];then
 $READELF -h -A "$out/fixture" > "$out/elf.txt"
 timeout --kill-after=1s 30s "$OECORE_NATIVE_SYSROOT/usr/bin/qemu-arm" -L "$SDKTARGETSYSROOT" -E LANG=C.UTF-8 "$out/fixture"
else
 ASAN_OPTIONS=detect_leaks=0:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 timeout --kill-after=1s 30s "$out/fixture"
fi
sha256sum "$out/fixture" > "$out/artifact.sha256"
sha256sum -c "$out/input-hashes.txt" > "$out/input-recheck.txt"
