#!/bin/sh
# Actual owned Qt/file lifetimes; selected source join and clock modeled.
set -eu
src=${1:?frozen source folder};out=${2:?fresh output folder}
mkdir -p "$out";mkdir -m 700 /tmp/reader-handoff-runtime
XDG_RUNTIME_DIR=/tmp/reader-handoff-runtime;export XDG_RUNTIME_DIR
mode=${READER_FIXED_PEN_HANDOFF_MODE:-host}
flags='-std=c++17 -Wall -Wextra -Werror -Wno-psabi -O1 -g -fPIC -pthread -fno-omit-frame-pointer'
moc=/usr/lib/qt6/libexec/moc
case "$mode" in
 host) compiler=g++;;
 asan) compiler=g++;flags="$flags -fsanitize=address,undefined";;
 arm) . /opt/codex/rm2/5.8.203/environment-setup-cortexa7hf-neon-remarkable-linux-gnueabi;compiler=$CXX;moc="$OECORE_NATIVE_SYSROOT/usr/libexec/moc";;
 *) exit 90;;
esac
$compiler --version > "$out/compiler.txt"
"$moc" "$src/reader_selected_view_probe.h" -o "$out/moc_reader_selected_view_probe.cpp"
$compiler $flags -I"$src" "$src/reader_fixed_pen_handoff_fixture.cpp" "$out/moc_reader_selected_view_probe.cpp" $(pkg-config --cflags --libs Qt6Core Qt6Gui Qt6Qml Qt6Quick) -o "$out/fixture"
if [ "$mode" = arm ]; then
 $READELF -h -A "$out/fixture" > "$out/elf.txt"
 timeout --kill-after=1s 60s "$OECORE_NATIVE_SYSROOT/usr/bin/qemu-arm" -L "$SDKTARGETSYSROOT" -E LANG=C.UTF-8 -E QT_QPA_PLATFORM=offscreen -E QT_QUICK_BACKEND=software -E QT_PLUGIN_PATH="$SDKTARGETSYSROOT/usr/lib/plugins" "$out/fixture"
else
 QT_QPA_PLATFORM=offscreen QT_QUICK_BACKEND=software ASAN_OPTIONS=detect_leaks=0:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 timeout --kill-after=1s 60s "$out/fixture"
fi
sha256sum "$out/fixture" > "$out/artifact.sha256"
