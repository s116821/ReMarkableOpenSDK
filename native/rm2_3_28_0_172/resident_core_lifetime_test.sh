#!/bin/sh
set -eu
root=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
out=${RESIDENT_CORE_OUTPUT:?fresh output directory required}
test ! -e "$out"
mkdir -p "$out"
sanitize=
if [ "${RESIDENT_CORE_SANITIZERS:-0}" = 1 ]; then sanitize='-fsanitize=address,undefined -fno-omit-frame-pointer -no-pie'; fi
${CXX:-c++} -std=c++17 -Wall -Wextra -Werror -O1 -g -fPIC -pthread $sanitize \
    "$root/resident_core_lifetime_test.cpp" $(pkg-config --cflags --libs Qt6Core) -o "$out/core-test"
ASAN_OPTIONS=halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 timeout --kill-after=1s 40s "$out/core-test"
