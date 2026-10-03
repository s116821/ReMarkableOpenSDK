"""Owned synthetic observations; these tests do not qualify tablet operations."""
import contextlib
import importlib.util
import io
import json
import os
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch

spec = importlib.util.spec_from_file_location('baseline', Path(__file__).with_name('collect_rm1_baseline.py'))
baseline = importlib.util.module_from_spec(spec)
spec.loader.exec_module(baseline)


def fixture(**changes):
    values = dict(model='reMarkable 1.0', arch='armv7l', firmware='3.28.0.172',
        pid_before='321', exe_before='/usr/bin/xochitl', xochitl_before='a' * 64,
        qt_version='6.10.3', qt_before='b' * 64, root_storage='100 5 0 1024',
        home_storage='1000 900 850 4096', framebuffer='mxc_epdc_fb',
        virtual_size='1408,3840', bits_per_pixel='16', xochitl_after='a' * 64,
        qt_after='b' * 64, pid_after='321', exe_after='/usr/bin/xochitl')
    values.update(changes)
    return ''.join(f'{k}\t{v}\n' for k, v in values.items()).encode()


class ParserTests(unittest.TestCase):
    def test_report_is_unqualified_and_separates_reserved_space(self):
        report = baseline.parse(fixture())
        self.assertEqual(report['qualification']['native_operations'], 'unsupported')
        self.assertFalse(report['qualification']['source_authority'])
        self.assertEqual(report['storage']['root']['reserved_free_bytes'], 5120)
        self.assertEqual(report['storage']['root']['available_bytes'], 0)
        self.assertIsNone(report['framebuffer']['logical_geometry'])
        self.assertNotIn('pid', json.dumps(report))

    def test_missing_duplicate_unknown_and_extra_fields_refuse(self):
        raw = fixture()
        for bad in (raw.split(b'\n', 1)[1], raw + b'model\treMarkable 1.0\n',
                    raw + b'DeveloperPassword\tSECRET\n', raw + b'garbage\n'):
            with self.subTest(bad=bad[-30:]), self.assertRaises(baseline.Refusal):
                baseline.parse(bad)

    def test_identity_changes_and_wrong_models_refuse(self):
        for changes in ({'model': 'reMarkable 2.0'}, {'arch': 'aarch64'},
                        {'xochitl_after': 'c' * 64}, {'qt_after': 'd' * 64},
                        {'pid_after': '322'}, {'exe_after': '/tmp/other'},
                        {'pid_before': '321 322'}, {'framebuffer': 'other'}):
            with self.subTest(changes=changes), self.assertRaises(baseline.Refusal):
                baseline.parse(fixture(**changes))

    def test_malformed_observations_refuse_without_echo(self):
        for changes in ({'firmware': 'SECRET'}, {'qt_version': 'invalid'},
                        {'xochitl_before': 'A' * 64}, {'virtual_size': '0,3840'},
                        {'virtual_size': '1408,3840,1'}, {'bits_per_pixel': '32'},
                        {'root_storage': '100 101 0 1024'},
                        {'home_storage': '100 90 91 4096'},
                        {'root_storage': '100 5 0 512'},
                        {'pid_before': '-1'}, {'virtual_size': '999999,1'}):
            with self.subTest(changes=changes), self.assertRaises(baseline.Refusal) as error:
                baseline.parse(fixture(**changes))
            self.assertNotIn('SECRET', str(error.exception))

    def test_output_and_encoding_limits(self):
        for raw in (b'x' * (baseline.MAX_OUTPUT + 1), fixture() + b'\xff'):
            with self.assertRaises(baseline.Refusal):
                baseline.parse(raw)


class TransportTests(unittest.TestCase):
    def script(self, directory, body):
        path = Path(directory) / 'owned-transport'
        path.write_text('#!/usr/bin/python3\n' + body)
        path.chmod(0o700)
        return str(path)

    def test_host_option_and_command_injection_refused_before_spawn(self):
        with patch.object(baseline.subprocess, 'Popen') as spawn:
            for host in ('-oProxyCommand=bad', 'host;bad', 'user@host', '', 'a\nb'):
                with self.assertRaises(baseline.Refusal):
                    baseline.transport(host)
            spawn.assert_not_called()

    def test_nonzero_transport_output_is_not_reported(self):
        with tempfile.TemporaryDirectory() as directory:
            command = self.script(directory, "import sys\nprint('SECRET')\nsys.exit(1)\n")
            with self.assertRaises(baseline.Refusal) as error:
                baseline.transport('fixture', command=command)
            self.assertNotIn('SECRET', str(error.exception))

    def test_stream_limit_and_deadline_terminate_owned_transport(self):
        for body in ("import os\nwhile True: os.write(1, b'x'*4096)\n",
                     "import time\ntime.sleep(60)\n"):
            with self.subTest(body=body), tempfile.TemporaryDirectory() as directory:
                command = self.script(directory, body)
                created = []
                original = baseline.subprocess.Popen
                def spawn(*args, **kwargs):
                    result = original(*args, **kwargs)
                    created.append(result)
                    return result
                with patch.object(baseline.subprocess, 'Popen', side_effect=spawn):
                    with self.assertRaises(baseline.Refusal):
                        baseline.transport('fixture', deadline=.15, command=command)
                self.assertIsNotNone(created[0].poll())

    def test_ssh_options_and_fixed_command(self):
        with tempfile.TemporaryDirectory() as directory:
            args_path = Path(directory) / 'args.json'
            command = self.script(directory,
                'import json,sys\n' + f'open({str(args_path)!r}, "w").write(json.dumps(sys.argv[1:]))\n' +
                f'sys.stdout.buffer.write({fixture()!r})\n')
            self.assertEqual(baseline.transport('fixture', command=command), fixture())
            args = json.loads(args_path.read_text())
            for option in ('BatchMode=yes', 'IdentitiesOnly=yes', 'StrictHostKeyChecking=yes',
                           'ForwardAgent=no', 'ClearAllForwardings=yes'):
                self.assertIn(option, args)
            self.assertEqual(args[-1], baseline.REMOTE_SCRIPT)


class OutputTests(unittest.TestCase):
    def test_atomic_output_replaces_validated_report(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / 'report.json'
            path.write_text('previous')
            baseline.write_report(path, baseline.parse(fixture()))
            self.assertEqual(json.loads(path.read_text())['model'], 'rm1')
            self.assertEqual(os.stat(path).st_mode & 0o777, 0o600)
            self.assertEqual(len(list(Path(directory).iterdir())), 1)

    def test_failed_parse_leaves_output_unchanged_and_no_secret_diagnostic(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / 'report.json'
            path.write_text('previous')
            with patch('sys.argv', ['collector', '--ssh-host', 'fixture', '--output', str(path)]), \
                 patch.object(baseline, 'transport', return_value=b'DeveloperPassword\tSECRET\n'), \
                 contextlib.redirect_stderr(io.StringIO()) as error:
                with self.assertRaises(SystemExit):
                    baseline.main()
            self.assertEqual(path.read_text(), 'previous')
            self.assertNotIn('SECRET', error.getvalue())

if __name__ == '__main__':
    unittest.main()
