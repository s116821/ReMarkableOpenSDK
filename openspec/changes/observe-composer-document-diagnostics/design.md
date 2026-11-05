# Design

## Boundary
Use supported Qt DirectConnection signals contentsChange, contentsChanged and
QObject::destroyed. Installation accepts a live exact QTextDocument on its owning
thread; the caller supplies that lifetime and native provenance. No native pointer
resolver, instruction patch, custom worker enqueue or std::shared_ptr native-owner
operation is introduced. A document address is diagnostic correspondence only.

The exact retained callback metatype is std::function<bool(Scene*)>, not the
delivered std::function<void()> bridge. A Scene* is borrowed only for that native
invocation; neither queued callback storage nor a job ID owns the Scene after return.
Native attachment requires an actual worker-scene/document binding and loaded-profile
validation; this change does not fabricate one.

## Recorder and retirement
Fixed storage holds at most 256 small records and an absolute deadline at most
120 seconds from attachment. Callbacks use no disk/network I/O, event pumping,
mutation, arbitrary user callbacks or text conversion. Try-lock contention is a
terminal diagnostic failure rather than blocking the emitting thread. Revision is
read only on the installation thread with matching QObject affinity; other threads
record a mismatch without reading the document.

Connections own only shared recorder state and a borrowed sender used during its
direct notification. No callback captures the observer handle. Cancellation marks
the state closed before disconnecting. Already-entered Qt functors retain their
state; the handle can disappear without invalidating them. Destruction notifications
never read QTextDocument members. Stop/disconnect is logical retirement, not proof
that every preempted Qt activation has unwound or that payload code may be unloaded.
The containing module must remain loaded until the surrounding runtime guarantees
no future calls. Qt owns document deletion; the recorder never deletes it.

Deadline gates every callback and snapshot/poll. An idle document requires the caller
to poll or destroy the handle for connection removal; no hidden event loop, timer or
background thread is installed. This does not replace independent runtime recovery.

## Meaning and limitations
Records describe observer callback entry, not the whole native call stack. Direct
signals can be coalesced, blocked or emitted by reentrant Qt processing. No complete
edit count, closed native IME transaction, owner incarnation, exact draft, owned
range, active visit, input origin, Submit event or clear authority follows.
Host fixtures exercise the real Qt signal/lifetime machinery, not Xochitl.
