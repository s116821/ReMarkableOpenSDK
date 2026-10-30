#!/bin/sh
# Actual host Linux fork/exec. No ARM exec/binfmt assumption, device or unload.
set -eu
ulimit -c 0
out=${CHILD_ENV_OUTPUT:?set CHILD_ENV_OUTPUT to a fresh writable directory}
root=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
tools=$(CDPATH= cd -- "$root/../.." && pwd)
# Flat immutable exports may place helper next to the five owned files.
if [ ! -f "$tools/reader_insertion_child_environment.h" ]; then tools=$root; fi
mkdir -p "$out"
test -z "${LD_PRELOAD+x}" && test -z "${LD_BIND_NOW+x}"
compiler=${CXX:-c++}
"$compiler" --version > "$out/compiler.txt"
uname -s -m > "$out/kernel.txt"
flags='-std=c++17 -Wall -Wextra -Werror -O1 -g -fPIC -fno-gnu-unique'
"$compiler" $flags -I"$tools" -fvisibility=hidden -shared "$root/preload.cpp" -Wl,-z,now -Wl,-z,defs -o "$out/preload.so"
"$compiler" $flags -I"$tools" "$root/parent.cpp" -ldl -o "$out/parent"
"$compiler" $flags "$root/child.cpp" -ldl -o "$out/child"
readelf -W -h -d -s "$out/preload.so" > "$out/preload-elf.txt"
readelf -W -h "$out/parent" "$out/child" > "$out/host-elf.txt"
sha256sum "$out/preload.so" "$out/parent" "$out/child" > "$out/artifact-hashes.txt"
for mode in inherit parent-clear constructor-clear already-clear custom-env; do
    timeout --kill-after=1s 10s sh -c '
      export OWNED_EXPECTED_PARENT_PID=$$ OWNED_CLEAR_MODE="$3"
      export LD_PRELOAD="$1" LD_BIND_NOW=1
      exec "$2" "$3" "$4" "$1"
    ' sh "$out/preload.so" "$out/parent" "$mode" "$out/child" > "$out/$mode.stdout" 2> "$out/$mode.stderr"
    cat "$out/$mode.stdout"
done
timeout --kill-after=1s 10s "$out/parent" bare-controls "$out/child" "$out/preload.so" > "$out/bare-controls.stdout" 2> "$out/bare-controls.stderr"
cat "$out/bare-controls.stdout"
test -z "${LD_PRELOAD+x}" && test -z "${LD_BIND_NOW+x}"
printf 'PASS builder environment unchanged; actual host exec only\n'
if [ -n "${CHILD_ENV_ARM_SDK_ENV:-}" ]; then
    (
        . "$CHILD_ENV_ARM_SDK_ENV"
        printf '#include "reader_insertion_child_environment.h"\nbool owned_compile_only(){return reader_child_environment::clearInheritedInjection();}\n' > "$out/helper-compile.cpp"
        $CXX -std=c++17 -Wall -Wextra -Werror -fPIC -I"$tools" -c "$out/helper-compile.cpp" -o "$out/arm-helper.o"
        $READELF -W -h -A "$out/arm-helper.o" > "$out/arm-helper-elf.txt"
        printf 'PASS ARM helper compile-only; no ARM exec\n'
    )
fi
