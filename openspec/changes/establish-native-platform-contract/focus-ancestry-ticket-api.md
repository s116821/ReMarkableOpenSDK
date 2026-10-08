# Retained-owner C++ ownership and API checkpoint

Status: proposed concrete signatures for Main/Astra review before implementation.
Authority: SDK `a4e8c8d0bef9ff884085262e5362b868d0bcbbf2` and independently accepted
consumer Docs `6763bfb5796758c710ec26b89047e87b396c1e61`. This is a design checkpoint,
not compiled code or runtime proof. The owning [focus contract](focus-ancestry-discovery.md)
still governs every selection, lifetime, refusal and qualification gate.

## Header boundary and sealed construction

Add a private implementation header `tools/qt_retained_owner_ticket.h`, included
by qt_page_facts.h and qt_page_facts_entry.h. It includes qt_page_owner.h, QObject,
QPointer and memory. Forward-declare FactsEntry and PageFactsSession. No header
depends on a complete FactsEntry type: record entry identity is QPointer<QObject>;
the sole typed bridge is defined inside the complete FactsEntry class. This avoids
an inline session reference to an incomplete entry or a missing definition in
ordinary standalone facts builds. No QObject parent owns the record or session.

Proposed internal declarations (method bodies remain subject to source review):

```cpp
class FactsEntry;
class PageFactsSession;

class RetainedOwnerRecord final {
    friend class FactsEntry;
    friend class RetainedOwnerTicket;
    enum class Boundary { EntryProgress, SessionOwner, DeliveryProgress, InvalidateOnly };
    using Bridge = bool (*)(RetainedOwnerRecord &, Boundary);
    RetainedOwnerRecord(QPointer<QObject> entry, quint64 generation,
                        PageOwner owner, QPointer<QQuickItem> root,
                        QPointer<QQuickItem> anchor, quint64 captureEpoch,
                        Bridge bridge);
    RetainedOwnerRecord(const RetainedOwnerRecord &) = delete;
    RetainedOwnerRecord &operator=(const RetainedOwnerRecord &) = delete;
    QPointer<QObject> entry_;
    const quint64 generation_;
    const PageOwner owner_;
    const QPointer<QQuickItem> root_, anchor_;
    const quint64 captureEpoch_;
    const Bridge bridge_;
    bool revoked_ = false;
    bool validating_ = false;
public:
    ~RetainedOwnerRecord() = default;
};

class RetainedOwnerTicket final {
    friend class FactsEntry;
    friend class PageFactsSession;
    RetainedOwnerTicket(std::weak_ptr<RetainedOwnerRecord> record,
                        quint64 generation, PageOwner owner);
    RetainedOwnerTicket(const RetainedOwnerTicket &) = delete;
    RetainedOwnerTicket &operator=(const RetainedOwnerTicket &) = delete;
    RetainedOwnerTicket(RetainedOwnerTicket &&) = delete;
    RetainedOwnerTicket &operator=(RetainedOwnerTicket &&) = delete;
    bool entryProgress();
    bool sessionOwner();
    bool deliveryProgress();
    bool validate(RetainedOwnerRecord::Boundary boundary);
    std::weak_ptr<RetainedOwnerRecord> record_;
    const quint64 generation_;
    const PageOwner owner_;
public:
    ~RetainedOwnerTicket() = default;
};
```

FactsEntry is the only factory friend. Private constructors accept a fixed static
bridge only from that factory; there is no public injected PageOwner, Bridge,
validation lambda, factory token, setter or subclass. No bridge captures raw this.
Public destructors allow standard unique/shared destruction but grant no authority.
Construct with explicit new inside the friend factory (make_unique/make_shared
cannot acquire private constructor access). Weak ticket locking retains record
storage temporarily, never entry lifetime or authority. Generation is a process-
local monotonically allocated identity with overflow refusal, not a wire field.

FactsEntry members and private methods:

```cpp
std::shared_ptr<RetainedOwnerRecord> retainedRecord_;
std::unique_ptr<PageFactsSession> makeRetainedReader();
static bool validateRetainedRecord(RetainedOwnerRecord &record,
                                  RetainedOwnerRecord::Boundary boundary);
void revokeRetainedOwner(); // idempotent, no getters
```

The private PageFactsSession overload is friend-accessible only to FactsEntry:

```cpp
friend class FactsEntry;
PageFactsSession(QQmlEngine *engine, PageFactsConfig config,
                 std::unique_ptr<RetainedOwnerTicket> ticket,
                 std::function<void(PageFactsResult)> completed);
std::unique_ptr<RetainedOwnerTicket> retainedTicket_;
```

Keep the current public engine/config/progress/completed constructor byte-for-byte
in behavior for all unselected callers. The selected overload accepts no progress
lambda and never calls findPageOwner. A null retained ticket refuses; it cannot
fall back to the public constructor. The ticket is created once after selected
complete-chain unique-owner capture and only transferred after the unchanged facts
request/positive visual binding gate. Its owner must equal the capture owner.

## Fixed bridge and reentrant scopes

Ticket validate locks the weak record; checks generation, nonrevocation, weak entry
and exact ticket/record owner QPointers; refuses if validating_ is already true.
Nested validation first revokes the record, then invokes the fixed bridge with
InvalidateOnly. This operation bypasses normal nonrevocation admission solely to
propagate the already latched invalidation to a surviving entry. It never invokes
validate(), a normal boundary, progress, current(), captureAllowedChecked(),
activeOwner(), endpoint sampling or any native getter. Resolve weak entry on the
GUI thread, immediately enter FactsEntry::Scope, check the entry's exact record
identity/generation and selected mode, then call invalidateCapture and latch
record revocation; return false in every case. Missing/mismatched entry returns
false without dereference or granting authority. InvalidateOnly is private and
fixed in the sealed factory bridge, not a caller-supplied operation or public API.
No nested call can clear invalidation, revocation, validating_, captureChecking_,
session checking_ or an earlier failure. Only the original outer RAII latch clears
its validating_ on unwind; existing owner/session checking flags remain governed
by their original outer scopes.

For the three normal validation boundaries, the static FactsEntry bridge resolves
the guarded QObject via qobject_cast<FactsEntry *>
only after weak availability/thread checks. It immediately enters the existing
FactsEntry::Scope before member access. It verifies selected mode, record identity
equals entry.retainedRecord_, generation, nonrevocation, context/root/attempt/
closure, captured epoch and retained anchor/root equality. Endpoint mismatch
invalidates through the existing captureInvalid_/epoch path. Original checks and
progress precedence remain unchanged; check again after getters. No chain rebuild.
The session caller holds CallScope before any ticket validation.

Boundary methods are private fixed operations, not a configurable policy:

- EntryProgress replaces only the selected constructor's existing progress lambda:
  original readCurrent plus captureAllowedChecked with one original activeOwner
  evaluation, guarded anchor/root checks and post-getter sticky/context checks.
- SessionOwner replaces the one activeOwner evaluation currently between current
  checks in allowed(): validate original owner plus retained record/anchor/root,
  evaluate activeOwner exactly once, then original post-getter progress/context.
- DeliveryProgress replaces the existing queued session progress call and preserves
  its deadline/cancellation/epoch boundary. It grants no new operation after success.

The exact getter schedule must be reviewed with implementation. In selected mode,
do not call sessionOwner as an extra check in addition to the existing activeOwner
site, or call entryProgress redundantly to manufacture provenance. Original
unselected current/allowed/delivery getter evaluation counts remain unchanged.
New root/anchor checks are selected guard observations and are explicitly counted
in fixture expectations. Owner/document predicates are never memoized past their
original validation boundaries. Null weak pointers are checked after every call
that can reenter; the bridge must not use a saved raw entry pointer after its scope
has unwound.

## Session admission, observer installation and completion

Selected begin enters CallScope, uses the unchanged config/current admission and
clock initialization, obtains the already validated exact ticket owner, and
skips candidate discovery. Required facts observers remain mandatory. Bracket
installation with ticket checks and retain the original metadata failure stage
when metadata fails and ticket checks pass; ticket failure wins according to the
first latched result. No helper read begins before all observers are installed.

Selected current/allowed/queued delivery use the fixed ticket boundaries above.
Ticket failure latches facts-retained-owner-refused in PageFactsResult with no
facts unless an earlier terminal result already exists. Existing helper/metadata/
final stages remain when ticket validation passed; a terminal result cannot be
rewritten by a later ticket failure. The new reader stage reaches the unchanged
27-field refusal only under explicit selected gating in both sanitization and
serialization, with had-facts=false/reader-result/outer read-refused. It can never
be paired with outer delivery-refused. Callback fields remain unchanged.

Replace the selected completion lambda's unguarded raw this with a captured
QPointer<FactsEntry>. Resolve it, enter Scope, then use the current reader-result
handling. If missing, return without publication or dereference. Ordinary mode
callback behavior remains unchanged. Session CallScope and entry Scope prevent
completion-triggered release during nested stacks; external caller still obeys
the existing requirement to retain entry through completion. The entry destructor
cannot be made safe for an arbitrary externally forced synchronous delete inside
a getter; that remains unsupported caller behavior, not a promised lifetime lease.

Guard revocation occurs immediately on terminal refusal, cancel, close and
destruction. Entry provisional observed success preserves record and focus-event
guard until both final binding checks/callback publication finish; then revoke
before reader reset and external completion callback. A final conversion to refusal
revokes immediately. Session provisional finish alone does not revoke entry guard.
No successful finish renews clocks or authorizes another facts read. Record signal
connections are entry-context owned and removed only after deferred scope unwind.

Required tests include standalone ordinary facts builds (no entry definition),
private constructor access/no injected bridge API, weak entry/record expiry,
generation/owner mismatch, repeated transfer, nested validation, entry callback
absence, cancel/close during getters, original getter counts, observer failures,
provisional success through both publication checks, final conversion to refusal,
exact inner/outer refusal tuple, and ordinary mode regression coverage. The code
checkpoint must prove these concrete declarations work under the vendor toolchain;
this document does not claim that proof. Native profile qualification remains open.
