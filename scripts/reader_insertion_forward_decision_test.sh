#!/bin/sh
set -eu
src=${1:?source folder}
out=${2:?output folder}
mkdir -p "$out"
g++ -std=c++17 -Wall -Wextra -Werror -O1 -pthread "$src/reader_insertion_forward_decision_fixture.cpp" -o "$out/decision-host"
"$out/decision-host"
. /opt/codex/rm2/5.8.203/environment-setup-cortexa7hf-neon-remarkable-linux-gnueabi
$CXX -std=c++17 -Wall -Wextra -Werror -O1 -pthread "$src/reader_insertion_forward_decision_fixture.cpp" -o "$out/decision-arm"
qemu-arm -L "$SDKTARGETSYSROOT" "$out/decision-arm"
