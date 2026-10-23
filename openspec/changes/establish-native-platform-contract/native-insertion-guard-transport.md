# Fixed diagnostic handoff

Wrap the [Coordinator](native-insertion-guard-entry.md) in a separate nonce-bound
file entry. Retain existing diagnostics and their spent packets unchanged. The
new wrapper accepts one request only under the current PID/start identity and
owned private root; no filename or ready marker supplies native authority.

Use the existing owner/attempt.identity/selected-ready/selected-request/result
conventions, no-follow fixed-size reads, root/file identity checks and exclusive
pending-file hard-link publication. Cancellation/restoration suppresses normal
result publication. Request consumption, nonce and deadline cannot be renewed.

The total setup window remains90s and the original request window30s. Freeze
one steady-clock deadline at consumption, clamp to setup end and pass that
absolute deadline through automatic fixture opening, the5s queued opportunity
and Coordinator. The Coordinator must never receive a fresh30s window afterward.

Open only the fixed disposable fixture through the already reviewed ordinary
legacy composition. No menus, new human countdown or post-insertion navigation
is needed for this negative witness. Forward permission remains bound to the
same process/window/engine and private root. Deletion or application shutdown
cancels the coordinator without closing retained native owners.

Emit a bounded closed JSON result with diagnostic stage/refusal, original
identity/clocks, navigation and Coordinator facts. Encode full unsigned64 job
IDs as canonical decimal strings, never floating point. Report native/render/
durable authority false. Positive diagnostic interpretation requires the exact
Core/worker/ordinary-callback conjunction and fresh publication permission.

The candidate startup environment uses LD_BIND_NOW=1 only for that temporary
process, plus a fully reviewed actor/recovery packet. Do not change global
systemd environment. Before any tablet action, independently review exact
source/build/provider/import/init-array/packet and original absolute recovery
clocks. Preserve all failures and complete device readback/restoration evidence.

Source basis: existing diagnostic file protocol, current Coordinator/negative
contracts and exact native comparator-binding requirement. This is precode for
one fixed research transport, not the product IPC protocol.

## Fixed startup composition

The separately compiled startup file requires a frozen nonce, document/order,
source/wrong index and target UUID. Reject fixture-testing macros at compilation.
Before installing the native pair, recheck the owned root and fixed owner,
actor/start/activation/attempt markers, actual PID/start identity and absence of
restore/cancel. The launcher already writes attempt.identity before exec; do not
invent another process identity or use a marker as proof of recovery authority.
The independent host packet review remains responsible for proving the actual
actor, policy, absolute recovery window and provider closure.

Install through the exact-build facilities in an actual pre-main constructor.
At ordinary QCoreApplication startup, queue one GUI entry. Supply conditional
caller assertions only for the exact profile/callsite, scoped Core registration,
probed worker thread, retained native owners and qualified final-lock path.
Those assertions are not newly measured global facts. Any missing startup,
rechecked admission, entry scheduling or configuration terminates the candidate
with119 and uses the already independent recovery actor. Keep all code and
native owner records resident; no destructor or dlclose is introduced.
