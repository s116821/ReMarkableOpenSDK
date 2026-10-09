#!/bin/sh
# Host-only exact ARM checkpoints, real GDB memory/register/breakpoint operations.
# Run in a network-none, read-only-root vendor container with writable /out.
set -eu
. /opt/codex/rm2/5.8.203/environment-setup-cortexa7hf-neon-remarkable-linux-gnueabi
export TMPDIR=/out
cd /src
$CXX -std=c++17 -O2 -g -Wall -Wextra -Werror -no-pie -pthread \
 tools/input_normalization_owned_fixture.cpp tools/input_normalization_owned_fixture.S \
 -o /out/input-normalization-owned-fixture
debugger=/opt/codex/rm2/5.8.203/sysroots/x86_64-codexsdk-linux/usr/bin/arm-remarkable-linux-gnueabi/arm-remarkable-linux-gnueabi-gdb
for mode in identity rotate nonfinite; do
 mkdir /out/$mode
 qemu-arm -L "$SDKTARGETSYSROOT" -g 1234 /out/input-normalization-owned-fixture "$mode" - > /out/$mode/fixture.stdout 2> /out/$mode/fixture.stderr &
 fixture_pid=$!
 # Independent HOST fixture watchdog only; no tablet, stock service or ptrace claim.
 (sleep 8; kill -KILL "$fixture_pid" 2>/dev/null || true) &
 watchdog_pid=$!
 SDK_NORMALIZATION_QEMU_PID="$fixture_pid" "$debugger" -nx -nh -batch -ex 'set confirm off' -ex 'set auto-load off' \
  -ex 'set debuginfod enabled off' -ex 'set auto-solib-add off' \
  -ex 'set osabi GNU/Linux' -ex 'set architecture arm' \
  -ex "set sysroot $SDKTARGETSYSROOT" -ex 'set exec-file-mismatch off' \
  -ex 'set remotetimeout 2' -ex 'target remote :1234' \
  -ex 'source /src/tools/input_normalization_gdb.py' \
  -ex "normalization-host-fixture /out/$mode" \
  /out/input-normalization-owned-fixture > /out/$mode/gdb.stdout 2> /out/$mode/gdb.stderr &
 debugger_pid=$!
 (sleep 9; kill -KILL "$debugger_pid" 2>/dev/null || true) &
 debugger_watchdog=$!
 if ! wait "$debugger_pid"; then
  kill -KILL "$fixture_pid" 2>/dev/null || true
  exit 1
 fi
 wait "$fixture_pid"
 kill "$watchdog_pid" "$debugger_watchdog" 2>/dev/null || true
 grep -q NORMALIZATION_HOST_FIXTURE_MEASURED /out/$mode/gdb.stdout
 python3 - "$mode" <<'PY'
import base64,hashlib,json,math,pathlib,struct,sys
mode=sys.argv[1]; files=sorted(pathlib.Path('/out',mode).glob('acquisition-*.json'))
receipts=pathlib.Path('/out',mode,'normalization.receipts').read_bytes()
assert files and sum(p.stat().st_size for p in files)+len(receipts)<=8192
assert receipts[:8]==b'NORMR001' and len(receipts)==8+36*len(files)
for ordinal,p in enumerate(files):
 count,digest=struct.unpack_from('<I32s',receipts,8+36*ordinal)
 assert count==p.stat().st_size and digest==hashlib.sha256(p.read_bytes()).digest()
entries=[json.loads(p.read_bytes())['entry'] for p in files]
result=entries[-1]['result']; assert result['status']=='measured' and not result['authority']
assert result['requested_bytes']==result['acquired_bytes']==286 and result['successful_stops']==2
pre=result['values']['pre']; post=result['values']['post']
if mode=='nonfinite': assert not pre[0]['finite'] and not post[0]['finite']
else:
 x,y=164/1403,1396/1871
 assert abs(pre[0]['value']-x)<1e-12 and abs(pre[1]['value']-y)<1e-12
 expected=(x,y) if mode=='identity' else (1-y,x)
 assert all(abs(p['value']-e)<1e-12 for p,e in zip(post,expected))
for entry in entries:
 if 'acquired' in entry: assert len(base64.b64decode(entry['base64']))==entry['acquired']
print('PASS owned ARM/GDB '+mode+'; host-only, no target-server qualification')
PY
done
