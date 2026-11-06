"""Real host compiler/readelf and fail-closed preflight; no target or Qt proof."""
import importlib.util
import os
from pathlib import Path
import subprocess
import tempfile
import unittest

ROOT=Path(__file__).resolve().parent
spec=importlib.util.spec_from_file_location('audit',ROOT/'audit_elf.py')
audit=importlib.util.module_from_spec(spec)
spec.loader.exec_module(audit)

class Contract(unittest.TestCase):
    def run_command(self,args,**kw):
        return subprocess.run(args,capture_output=True,text=True,timeout=30,**kw)

    def test_profile_policy_and_seal(self):
        with tempfile.TemporaryDirectory(prefix='rm1-policy-') as work:
            exe=Path(work)/'policy'
            built=self.run_command([os.environ.get('RSDK_HOST_CXX','c++'),'-std=c++17','-Wall','-Wextra','-Werror',str(ROOT/'admission_test.cpp'),'-o',str(exe)])
            self.assertEqual(built.returncode,0,built.stderr)
            ran=self.run_command([str(exe)])
            self.assertEqual(ran.returncode,0,ran.stderr)
            self.assertIn('no RM1 admission',ran.stdout)

    def test_direct_constructor_is_inaccessible(self):
        with tempfile.TemporaryDirectory(prefix='rm1-seal-') as work:
            source=Path(work)/'forgery.cpp'
            source.write_text('#include "admission.h"\nint main(){auto *a=new remarkable::rm1_observation::Admission(nullptr,1,1);(void)a;}\n')
            ran=self.run_command([os.environ.get('RSDK_HOST_CXX','c++'),'-std=c++17','-fsyntax-only','-I',str(ROOT),str(source)])
            self.assertNotEqual(ran.returncode,0)
            self.assertIn('private',ran.stderr)

    def test_unqualified_build_never_sources_sdk_or_creates_output(self):
        with tempfile.TemporaryDirectory(prefix='rm1-build-') as work:
            work=Path(work);marker=work/'sdk-sourced';sdk=work/'sdk-env.sh';out=work/'target-output'
            sdk.write_text('touch "'+str(marker)+'"\n')
            environment=dict(os.environ,QT_PROBE_SDK_ENV=str(sdk),RSDK_NATIVE_OUTPUT=str(out),
                             RSDK_RM1_PROFILE_MANIFEST=str(work/'peer-profile.json'),RSDK_PROFILE_SHA256='1'*64)
            for arguments in [[],['--check-inputs']]:
                ran=self.run_command(['sh',str(ROOT/'build.sh'),*arguments],env=environment)
                self.assertEqual(ran.returncode,78,ran.stderr)
                self.assertIn('no reviewed RM1 observation profile',ran.stderr)
                self.assertFalse(marker.exists())
                self.assertFalse(out.exists())

    def test_actual_host_elf_reports_forbidden_dependency(self):
        with tempfile.TemporaryDirectory(prefix='rm1-elf-') as work:
            work=Path(work);source=work/'fixture.cpp';library=work/'fixture.so'
            for symbol in ['owned_fixture','DobbyHook']:
                source.write_text('extern "C" int '+symbol+'(){return 0;}\n')
                built=self.run_command([os.environ.get('RSDK_HOST_CXX','c++'),'-shared','-fPIC','-Wl,-z,now,-z,nodelete',str(source),'-o',str(library)])
                self.assertEqual(built.returncode,0,built.stderr)
                report=self.run_command(['readelf','-W','-h','-A','-d','-s',str(library)])
                self.assertEqual(report.returncode,0,report.stderr)
                failures=audit.issues(report.stdout)
                self.assertIn('missing ARM',failures) # host fixture, never ARM proof
                self.assertNotIn('missing eager binding',failures)
                self.assertNotIn('missing resident lifetime',failures)
                self.assertEqual('forbidden native dependency Dobby' in failures,symbol=='DobbyHook')

    def test_symbol_names_cannot_fake_dynamic_flags(self):
        with tempfile.TemporaryDirectory(prefix='rm1-elf-flags-') as work:
            work=Path(work);source=work/'fixture.cpp';library=work/'fixture.so'
            source.write_text('extern "C" int NOW(){return 0;}\nextern "C" int NODELETE(){return 0;}\n')
            built=self.run_command([os.environ.get('RSDK_HOST_CXX','c++'),'-shared','-fPIC',str(source),'-o',str(library)])
            self.assertEqual(built.returncode,0,built.stderr)
            report=self.run_command(['readelf','-W','-h','-A','-d','-s',str(library)])
            self.assertEqual(report.returncode,0,report.stderr)
            failures=audit.issues(report.stdout)
            self.assertIn('missing eager binding',failures)
            self.assertIn('missing resident lifetime',failures)

if __name__=='__main__':unittest.main()
