#!/bin/sh
# Prepare exact owned inputs with reader_write_reopen_admission_prepare.py.
set -eu
export LANG=C.UTF-8
mode=$1
version=$2
input=$3
output=$4
mkdir -p "$output"
cd "$input"
if test "$mode" = arm; then
    . /opt/codex/rm2/5.8.203/environment-setup-cortexa7hf-neon-remarkable-linux-gnueabi
    compiler=$CXX
else compiler=g++; fi
old=0;test "$version" != old || old=1
flags='-std=c++17 -O1 -g -Wall -Wextra -Werror -Wno-psabi -DQT_FORCE_ASSERTS -fPIC'
if test "$mode" = asan; then flags="$flags -fsanitize=address,undefined -fno-omit-frame-pointer -no-pie"; fi
$compiler $flags -DFIXTURE_OLD="$old" -I. -I"$input/$version" reader_write_reopen_admission_fixture.cpp $(pkg-config --cflags --libs Qt6Core) -o "$output/fixture"
if test "$mode" = arm; then
    qemu-arm -L "$SDKTARGETSYSROOT" "$output/fixture" > "$output/stdout" 2> "$output/stderr"
else
    ASAN_OPTIONS=detect_leaks=0 "$output/fixture" > "$output/stdout" 2> "$output/stderr"
fi
