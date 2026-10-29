#!/usr/bin/env python3
"""Read-only Linux-host RM1 baseline; observation grants no native authority."""
import argparse
import datetime
import json
import os
from pathlib import Path
import re
import selectors
import subprocess
import tempfile
import time

MAX_OUTPUT = 16384
DEADLINE = 30
REMOTE_SCRIPT = r'''set -eu
emit() { printf '%s\t%s\n' "$1" "$2"; }
emit model "$(cat /sys/devices/soc0/machine)"
emit arch "$(uname -m)"
emit firmware "$(sed -n 's/^IMG_VERSION="\([^"]*\)"$/\1/p' /usr/lib/os-release)"
pid=$(pidof xochitl)
emit pid_before "$pid"
emit exe_before "$(readlink /proc/$pid/exe)"
emit xochitl_before "$(sha256sum /usr/bin/xochitl | cut -d ' ' -f 1)"
emit qt_version "$(basename "$(readlink -f /usr/lib/libQt6Core.so.6)" | sed 's/^libQt6Core.so.//')"
emit qt_before "$(sha256sum /usr/lib/libQt6Core.so.6 | cut -d ' ' -f 1)"
emit root_storage "$(stat -f -c '%b %f %a %S' /)"
emit home_storage "$(stat -f -c '%b %f %a %S' /home)"
emit framebuffer "$(cat /sys/class/graphics/fb0/name)"
emit virtual_size "$(cat /sys/class/graphics/fb0/virtual_size)"
emit bits_per_pixel "$(cat /sys/class/graphics/fb0/bits_per_pixel)"
emit xochitl_after "$(sha256sum /usr/bin/xochitl | cut -d ' ' -f 1)"
emit qt_after "$(sha256sum /usr/lib/libQt6Core.so.6 | cut -d ' ' -f 1)"
pid=$(pidof xochitl)
emit pid_after "$pid"
emit exe_after "$(readlink /proc/$pid/exe)"
'''
KEYS = frozenset(('model arch firmware pid_before exe_before xochitl_before '
    'qt_version qt_before root_storage home_storage framebuffer virtual_size '
    'bits_per_pixel xochitl_after qt_after pid_after exe_after').split())

class Refusal(Exception):
    """Messages must not interpolate device output or credentials."""


def number(text, minimum=0, maximum=(1 << 63) - 1):
    if not re.fullmatch(r'0|[1-9][0-9]{0,18}', text):
        raise Refusal('Invalid numeric observation')
    value = int(text)
    if not minimum <= value <= maximum:
        raise Refusal('Numeric observation out of bounds')
    return value


def storage(text):
    parts = text.split(' ')
    if len(parts) != 4:
        raise Refusal('Invalid storage observation')
    total, free, available, block_size = map(number, parts)
    if not 0 <= available <= free <= total or not total:
        raise Refusal('Inconsistent storage observation')
    if block_size not in (1024, 2048, 4096):
        raise Refusal('Unexpected filesystem block size')
    return dict(total_bytes=total * block_size, free_bytes=free * block_size,
                available_bytes=available * block_size,
                reserved_free_bytes=(free - available) * block_size)


def parse(raw):
    if len(raw) > MAX_OUTPUT:
        raise Refusal('Observation output exceeded limit')
    try:
        text = raw.decode('ascii')
    except UnicodeError:
        raise Refusal('Invalid observation encoding') from None
    fields = {}
    for line in text.splitlines():
        parts = line.split('\t')
        if len(parts) != 2 or parts[0] not in KEYS or parts[0] in fields:
            raise Refusal('Invalid observation fields')
        fields[parts[0]] = parts[1]
    if fields.keys() != KEYS:
        raise Refusal('Incomplete observation')
    if fields['model'] != 'reMarkable 1.0' or fields['arch'] != 'armv7l':
        raise Refusal('Unsupported model or architecture')
    if fields['framebuffer'] != 'mxc_epdc_fb':
        raise Refusal('Unsupported framebuffer family')
    for key in ('firmware', 'qt_version'):
        if not re.fullmatch(r'[0-9]{1,3}(\.[0-9]{1,4}){2,3}', fields[key]):
            raise Refusal('Invalid version observation')
    for prefix in ('xochitl', 'qt'):
        before, after = fields[prefix + '_before'], fields[prefix + '_after']
        if not re.fullmatch(r'[0-9a-f]{64}', before) or before != after:
            raise Refusal('Changed or invalid runtime fingerprint')
    if (number(fields['pid_before'], 1) != number(fields['pid_after'], 1)
        or fields['exe_before'] != '/usr/bin/xochitl'
        or fields['exe_after'] != '/usr/bin/xochitl'):
        raise Refusal('Runtime process identity changed or unexpected')
    dimensions = fields['virtual_size'].split(',')
    if len(dimensions) != 2:
        raise Refusal('Invalid framebuffer allocation')
    width, height = (number(n, 1, 65536) for n in dimensions)
    if number(fields['bits_per_pixel']) != 16:
        raise Refusal('Unsupported framebuffer format')
    return {
        'schema_version': 1,
        'observation_kind': 'read_only_platform_baseline',
        'model': 'rm1', 'architecture': fields['arch'],
        'firmware': fields['firmware'], 'qt_core_version': fields['qt_version'],
        'xochitl_sha256': fields['xochitl_before'],
        'qt_core_sha256': fields['qt_before'],
        'identity_checks': 'two_matching_samples_non_atomic',
        'storage': {'root': storage(fields['root_storage']),
                    'home': storage(fields['home_storage'])},
        'framebuffer': {'family': fields['framebuffer'], 'bits_per_pixel': 16,
                        'virtual_allocation_pixels': [width, height],
                        'logical_geometry': None},
        'qualification': {'native_operations': 'unsupported',
                          'source_authority': False,
                          'reason': 'RM1 native adapter not qualified'},
    }


def transport(host, deadline=DEADLINE, command='ssh'):
    if not re.fullmatch(r'[A-Za-z0-9][A-Za-z0-9_.-]{0,127}', host):
        raise Refusal('Invalid SSH host token')
    args = [command, '-T', '-o', 'BatchMode=yes', '-o', 'IdentitiesOnly=yes',
            '-o', 'StrictHostKeyChecking=yes', '-o', 'ForwardAgent=no',
            '-o', 'ClearAllForwardings=yes', '-o', 'ConnectTimeout=8',
            host, REMOTE_SCRIPT]
    try:
        proc = subprocess.Popen(args, stdin=subprocess.DEVNULL,
                                stdout=subprocess.PIPE, stderr=subprocess.DEVNULL)
    except OSError:
        raise Refusal('SSH transport unavailable') from None
    result = bytearray()
    end = time.monotonic() + deadline
    try:
        with selectors.DefaultSelector() as selector:
            selector.register(proc.stdout, selectors.EVENT_READ)
            while True:
                remaining = end - time.monotonic()
                if remaining <= 0:
                    raise Refusal('SSH observation deadline exceeded')
                events = selector.select(remaining)
                if not events:
                    raise Refusal('SSH observation deadline exceeded')
                data = os.read(proc.stdout.fileno(), min(4096, MAX_OUTPUT + 1 - len(result)))
                if not data:
                    break
                result.extend(data)
                if len(result) > MAX_OUTPUT:
                    raise Refusal('Observation output exceeded limit')
        try:
            status = proc.wait(timeout=max(.001, end - time.monotonic()))
        except subprocess.TimeoutExpired:
            raise Refusal('SSH observation deadline exceeded') from None
        if status:
            raise Refusal('SSH observation failed')
        return bytes(result)
    finally:
        if proc.poll() is None:
            proc.kill()
        proc.wait()
        proc.stdout.close()


def write_report(path, report):
    """Validate before calling; atomic replacement, no raw remote data persisted."""
    path = Path(path)
    temporary = None
    try:
        with tempfile.NamedTemporaryFile(mode='w', encoding='utf-8', dir=path.parent,
                                         prefix='.' + path.name + '.', delete=False) as f:
            temporary = f.name
            json.dump(report, f, indent=2, sort_keys=True)
            f.write('\n')
            f.flush()
            os.fsync(f.fileno())
        os.replace(temporary, path)
    finally:
        if temporary and os.path.exists(temporary):
            os.unlink(temporary)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--ssh-host', required=True)
    parser.add_argument('--output', required=True, type=Path)
    args = parser.parse_args()
    try:
        report = parse(transport(args.ssh_host))
        report['observed_at_utc'] = datetime.datetime.now(datetime.timezone.utc).isoformat()
        write_report(args.output, report)
    except (Refusal, OSError) as error:
        # OSError paths may include operator input; keep file diagnostics generic.
        parser.exit(2, ('Baseline refused: ' + str(error) if isinstance(error, Refusal)
                        else 'Baseline refused: output could not be saved') + '\n')
    print('RM1 baseline saved; native operations remain unsupported.')

if __name__ == '__main__':
    main()
