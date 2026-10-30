#!/bin/sh
# Host-only owned dlopen fixture. The ARM native artifact is never loaded here.
set -eu
root=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
work=${LOADED_ARTIFACT_TEST_OUTPUT:?fresh output directory required}
test ! -e "$work"
mkdir -p "$work"
printf '%s\n' 'extern "C" void owned_artifact_anchor() {}' > "$work/owned.cpp"
${CXX:-c++} -shared -fPIC "$work/owned.cpp" -o "$work/owned.so"
${CXX:-c++} -std=c++17 -Wall -Wextra -Werror -O1 -g -fPIC \
    -fsanitize=address,undefined -fno-omit-frame-pointer -no-pie \
    "$root/loaded_artifact_test.cpp" $(pkg-config --cflags --libs Qt6Core) -ldl -o "$work/artifact-test"
timeout --kill-after=1s 20s "$work/artifact-test" "$work/owned.so"
