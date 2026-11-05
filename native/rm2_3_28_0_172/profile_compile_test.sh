#!/bin/sh
# Compile-only exact ARM profile. No constructor, loading, hooking or tablet I/O.
set -eu
out=${NATIVE_PROFILE_OUTPUT:?fresh output directory required}
: "${QT_PROBE_SDK_ENV:?exact ARM SDK environment required}"
: "${DOBBY_INCLUDE:?reviewed Dobby include directory required}"
mkdir -p "$out"
. "$QT_PROBE_SDK_ENV"
root=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
flags='-std=c++17 -Wall -Wextra -Werror -O1 -g -fPIC -pthread -marm -fexceptions -frtti -fno-lto -fno-gnu-unique -fvisibility=hidden -ftls-model=initial-exec'
$CXX $flags -I"$root" -I"$DOBBY_INCLUDE" $(pkg-config --cflags Qt6Core) \
    -c "$root/reader_insertion_guard_native_profile.cpp" -o "$out/profile.o"
$READELF -W -h -A "$out/profile.o" > "$out/profile-elf.txt"
sha256sum "$out/profile.o" > "$out/profile-sha256.txt"
printf 'PASS exact ARM native profile compile-only; no linked adapter or activation\n'
