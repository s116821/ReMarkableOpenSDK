# RM1 historical observation source

This is an unqualified source increment. No approved RM1 observation profile or
native library/profile pair exists. `profile.h::reviewedProfile()` returns null;
neither build environment, peer admission nor owned test tables can replace it.
`sh native/rm1_observation/build.sh --check-inputs` exits78 before sourcing an SDK
or creating target output. It compiles only a small host preflight in an owned,
automatically retired temporary directory. It does not install or activate code.

## Remaining concrete inputs

The three private xochitl/Qt Core/libc copies are not a complete observation
profile. A separately reviewed source change must provide the exact profile
manifest digest, executable identity/size, matching Qt version, required provider
identities/sizes for Core, QML, Quick, GUI, Network and the C/C++ runtimes, and
actual required export/version/code and main/adapter eager-binding evidence.
Profile-shape checks only reject incomplete declarations; they do not establish
that an export list covers the real linked artifact's ABI-sensitive dependency
closure. That coverage needs independent review of the actual build and target.

The target build also needs an independently reviewed RM1 hard-float ARM SDK
environment, matching Qt development libraries/pkg-config metadata, native moc,
target C++ compiler and readelf. Set `QT_PROBE_SDK_ENV`,
`RSDK_RM1_PROFILE_MANIFEST` and a nonexistent `RSDK_NATIVE_OUTPUT` only after those
inputs/profile have been selected. A compiler target or RM2 sysroot alone does
not supply the missing evidence. No toolchain download or device operation is
part of this source increment.

## Code and runtime boundary

The RM1 build compiles only admission/bootstrap and the existing observation
Epoch moc. It reuses the historical Reader/order/capture helpers unchanged, through
the private shared dispatcher. It does not compile/link mutation executors,
worker ownership, creation bridge, insertion hooks or Dobby. Required Qt/runtime
libraries are dynamically linked; proprietary libraries are never bundled.
`audit_elf.py` checks the emitted report's architecture, eager binding, NODELETE,
required Qt dependencies and forbidden symbols/dependencies. This is an artifact
boundary check, not native qualification or complete ABI verification.

The real verifier retains the existing exact loader's bounded file/ELF/mapping/
mapped-code/provider/export/slot checks without its hook installation or private
owner pin tables. It requires pre-main application absence, exact claimed
runtime correspondence, actual stable adapter mapping, eager binding/NODELETE
and unchanged retained directory. An immutable process-resident Admission is
issued only there; it rechecks process incarnation/directory during requests.
The current missing profile refuses before any of those paths can admit work.

Bootstrap reuses one-use Activation, consumed marker retention, actual artifact
hashing, inherited injection-environment clearing and queued GUI startup. The
only exposed commands are historical Observe, presence reconciliation and the
existing source/visual captures. These cannot grant navigation, write authority
or a live PageObservation. A refused/uncertain mutation is never cleared by an
observation. Bootstrap failure exits119; observation-only does not imply harmless
payload loading. No automatic boot installation, retry, unload or normal restart.

RM2 keeps its original Facilities gate and delegates the same commands to shared
mechanics. Its approved manifests/pins/build recipe are untouched. Source
extraction may change a rebuilt RM2 binary; do not replace the approved pair or
silently update digests. Root must review any resulting RM2 artifact change.

## Validation

Run `python3 -B native/rm1_observation/build_contract_test.py -v` for existing-host
compiler seal/profile-shape controls, real host-ELF forbidden-dependency checks
and refusal before an SDK script/output can be touched. These fixtures cannot
issue an Admission and provide no target facts.

With an existing Qt toolchain, run `native/resident/read_only_dispatch_test.sh`
with a fresh `READ_ONLY_DISPATCH_TEST_OUTPUT` to exercise real socket rejection
without original RM2 Facilities and rejection of null RM1 admission. It also
checks that arbitrary callers cannot construct the shared dispatcher. Optional
`QT_PROBE_SDK_ENV` uses the existing SDK moc/compiler and QEMU for these owned
models, as the existing runners do; it supplies no actual RM1 evidence. This
runner checks for Qt before creating output; a missing Qt dependency is not a
test pass.

With an existing matching Qt toolchain, run the existing `order_observation_test.sh`
and `reader_source_capture_test.sh`/`reader_identity_capture_test.sh`,
and resident activation/endpoint/loaded-artifact controls into fresh owned output.
The shared header is production-compiled by the existing order runner; its
existing reply tests retain the RM2-qualified name through an alias. These checks
are still host/model evidence, not RM1 runtime proof. Actual target compilation,
provider/startup/Reader/metatype/capture/recovery qualification and all nine native
acceptance tasks remain open. Native's REM-61 observer/callback/recorder paths are
unchanged and are not a second investigation in this lane.
