# Ordinary document close: owned helper result

October 23, 2026. A separate synchronous research helper now exercises the
[source-backed document-close action](../research/ordinary-reader-close-source-boundary.md).
Host, vendor ARM/QEMU and host ASAN+UBSAN each passed32 cases/39 checks with empty
stderr. Actual Qt/QML methods execute; native Reader discovery and identity joins
are explicitly modeled. No tablet close or native save/reopen occurred.

The [helper](../../tools/reader_ordinary_close.h) consumes a shared one-use claim
before callouts. It requires a fresh ready document/page/index/count join, uses
only the callback-local selected Reader, and checks the public zero-argument
close slot with QVariant return declared on that exact reader metaobject.
It marks attempted before direct invocation. Invocation is the terminal action;
it never consults the old reader or borrowed source guard after the call.

The result retains copied pre-action facts, claim/method/attempt/dispatch facts
and an explicit reconciliation requirement after every attempt. A successful
invoke is not UI-closure or storage-completion evidence. A non-preemptible method
overrun remains attempted and reports its exceeded original budget. A fresh
closure/reopen observation and full fixture preservation remain separate work.
See the [change-specific contract](../../openspec/changes/establish-native-platform-contract/ordinary-reader-close.md).

The [owned fixture](../../tools/reader_ordinary_close_fixture.cpp) invokes an
actual generated QML function on an owned registered QQuickItem. It also covers
wrong identity/readiness, missing/wrong/inherited methods, cancellation, invalid
configuration, real wrong-thread entry, duplicate/nested claims and callbacks,
receiver deletion before and during dispatch, exceptions and deadline overrun.
The production selected-view probe is unchanged. This fixture does not supply
the real application's method availability or runtime identity/lifetime.

Initial compilation refused a misleading-indentation warning. The next run
refused the fixture's incorrectly initialized modeled-ready facts. The following
runs exposed a real declaration mistake: generated QML functions are public
slots with QVariant return in these inspected Qt builds, while the initial
helper required QMetaMethod::Method. That gate is repaired to Slot. Host QtQuick
import also lacked QtQml.WorkerScript; the final test uses its own registered
QQuickItem module, exercising actual QML generation without installing packages.
All earlier raw failures remain preserved, separate from the final pass.

Independent review of the first31/38 pass then found stale method metadata could
survive a final permission/source callout. An owned two-stable-meta-table control
reproduced one attempted close before the fix. The final helper rechecks the
reader's exact metaobject after its final source guard and before effects; the
same control now records zero attempts/calls on all three platforms. The earlier
pass and reproduced failure remain separate from the repaired32/39 result.

Run [reader_ordinary_close_test.sh](../../tools/reader_ordinary_close_test.sh)
against immutable input files and a fresh output directory, selecting
`READER_ORDINARY_CLOSE_MODE=host`, `arm` or `asan`. The pinned SDK image is
`sha256:416c7a7be0038156797b0892f031f352b841d1921fae83f712d0a272e4724618`.
ARM uses the matching vendor environment and qemu-arm/sysroot. The final original
outputs are retained locally as ordinary-close-owned-r7. Header SHA-256
`20e2978c627d40a99a1d600d1ef1963641da917571acc636d948cf2a0c0331f0`,
fixture `657b5a81db30a1c231d37c8922224f7e4db293e6b8fe0074437685f8ce9f3a8a`.
Independent source/original-evidence review passed70 checks, receipt SHA-256
`3b80a2345fbc4ba93fe443a50acef8da4f8a5fc27c5058b3d3979363f5e179fe`.
The reviewer inspected and rehashed the owner's original executions rather than
rerunning them. Private raw records have no public download link.

Separate host and vendor-ARM compile-only translation units also pass with the
test macro disabled, checking the production header signature. Receipt SHA-256
`14a3b64dd5889d3ccf0aca2910e2c4a6d3000764ed4a0d0231fb85fc282c74e3`.
These object builds are not a linked native candidate or another runtime test.

Source basis: current helper/fixture and actual original host/ARM/sanitizer
outputs; the native join is a test seam, not observed stock behavior.
