# Public QML access development probe

This original, development-only experiment adapts the public window/engine/QML
access concept from [rm-librarian 7d0fe086](https://github.com/rmitchellscott/rm-librarian/blob/7d0fe08678203c129b9d848419da2a7e96a2abe5/src/main.cpp#L41-L113).
No community CRUD, generic invocation helpers, persistent raw caches or endless
polling are copied. SDK license/publication decisions remain open.

The app-context startup callback queues one probe. It inspects at most sixteen
existing windows per attempt, accepts one distinct existing engine on the app
thread, and otherwise retries only after coalesced window Show/Expose/focus
events. Eight acquisition attempts are permitted. Once selected, the engine is
never reacquired. The five-second elapsed deadline can only refuse; a timer is
never evidence of readiness. Public component Ready/Error status transitions
queue processing; Loading waits within the same deadline. There is one create.

The fixed helper imports QtQml and versionless xofm.libs.library, binds a typed QtObject
reference to DocumentController, and exposes its own availability boolean.
C++ reads that boolean only. It does not retrieve the controller pointer, read
native controller properties, call controller/page methods or access documents
or account files. QML resolution **may invoke the existing registration functor,
associate the singleton with an engine and set CppOwnership**. Import caches and
that association are not undone by helper cleanup. This is not a no-factory or
pure read-only operation. Access proves neither displayed source nor page identity.

Only the owned helper/component/filter/timer/probe are cleaned up; weak engine
guards and post-call checks do not pin native lifetimes. Supported GUI lifecycle
is assumed. Cancellation during setData/create defers destruction until those
calls unwind. The DSO must remain loaded until process exit. Qt allocation,
imports and synchronous calls have no hard execution-time bound; the elapsed
deadline refuses late completion, while the independently reviewed operator
provides external recovery.

The operator precreates an owned 0700 `/run/rmb-qt-probe-<nonce>` directory.
`callback.json` is one exclusive, no-follow 0600 open and one write below 256
bytes. It contains fixed nonce/stage and scalar application_thread,
engine_thread, helper_available, controller_available booleans. It contains no
raw pointers or document strings. Existing receipts are preserved; partial writes
are invalid. A fresh frozen 32-character lowercase hexadecimal QT_PROBE_NONCE
is required for the independently built target artifact. The script nonce is
synthetic and must never be reused on hardware.

The original e017ccc/5ce development run reached `component-error` after engine
acquisition; its saved receipt did not disclose a compiler reason. Removing the
explicit 1.0 module version matches the pinned community import. A version
mismatch remains a hypothesis, not an established explanation of that result.
On a later component compilation failure, the classifier inspects at most eight
errors with at most 256 UTF-16 code units per description. It emits only fixed
`module-version`, `module-missing`, `type-missing`, `property-type` or
`component-error` stages. Unknown, mixed or oversized diagnostics use the generic
stage. It never emits error descriptions or URLs. These English vendor-Qt
patterns are categorical hints, not a general parser or root-cause proof. Qt
still owns/materializes the error list; this limit bounds our inspection only.

Run `sh tools/qt_qml_access_probe_test.sh` with Qt development/runtime plugins,
or set QT_PROBE_SDK_ENV to the firmware-matched SDK environment file to compile
and run owned ARM fixtures under qemu with the SDK offscreen/QML plugins.
Fixtures cover fixed-category/unknown/mixed/count/description boundaries, actual
owned compiler errors, versionless major-version selection,
immediate/same-engine/late readiness, absent registration,
conflicting and multiple engines, window cap, engine teardown, event cap,
deadline refusal (including a deliberately nested owned singleton callback) and
application cancellation. The nested fixture registration factory is confined
to the owned application; the payload has no such registration. These are synthetic fixtures,
not hardware or production qualification. OpenSpec task 2.5 and native page
creation/recovery gates remain open pending exact source/artifact/operator
review and coordinated evidence.
