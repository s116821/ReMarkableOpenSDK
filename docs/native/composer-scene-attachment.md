# Native scene document attachment

This opt-in diagnostic joins one borrowed native Scene to its exact root text
item's embedded QTextDocument. It adds no dispatch endpoint, runtime selection,
draft lease, Submit or clear operation. The caller must supply the callback
through the actual typed native scene request and keep the code loaded.

## Admission and borrow

Call Profile::load outside a signal or scene callback. It requires ARM32
little-endian hard-float, Qt6.10.3, a zero-bias main image whose load segments cover
the fixed profile addresses, and exact executable-mapping/backing-file hashes:

| Provider | SHA256 |
| --- | --- |
| Xochitl 3.28.0.172 | 071d85beef3ef2d4cc0e11002140b27b82a2cc04a2ed740a5669f591069b77df |
| QtGui | 93fe582cc61673342ca49e12306d7f689860016582fa46ce135ffa280a972839 |
| QtCore | 43b0e210d64e59b534490d78c4c82cc1d2958999b0969aa0f77e11a082704e5d |
| libstdc++ | 826f225815cb21bbfb5290dacc4bfdc21f913fc0f0194d6b8869948c4467de01 |

Critical mapped request/revision instructions, callback metatype and vtable RTTI
are checked too. These checks assume a trusted process and stable loaded
providers; they do not authenticate arbitrary pointers or defend against
same-process memory patching. This profile is deliberately not wired into the
resident bootstrap or capability selection.

Prepare fixes an absolute deadline of at most120 seconds. callback() returns
std::function<bool(Scene*)>; copies retain attachment state. The exact native
request borrows a live Scene synchronously on its scene-owner thread. No scene,
item or document owner pointer is retained after that borrow. The observer's Qt
connections retain recorder storage and auto-disconnect on QObject destruction.

| Retained native evidence | Join/check |
| --- | --- |
| Focus callback0x4b8ef0 and resolver0xb42234 | Scene participant+0x26c; root/control+0x190/+0x194; participant scene back-reference+8 and focus ID+0x10/+0x14 |
| SceneTextItem constructor0xb73f6c | Primary/secondary vptr0x133e700/0x133e758; native ID+0x10/+0x14; embedded document+0x1ec |
| Constructor and setComposeRange0xb70b9c | Compose start+0x260 must be-1, length+0x264 and virtual extension+0x268 must be0 |
| IM begin/end0xb4bdac/0xb4d230 | Participant operation scope+0x30, IM scope+0x34 and nested count+0x44 must all be0 |
| QtGui revision0x2a7af4 and begin-edit helper0x2a6e7c | QObject d-pointer+4, document revision d+0x68, edit depth d+0x74 |
| Callback metatype0x1513188; request0x6d6ce4; Core case0x1b | Exact callback value size16/alignment4, synchronous borrowed Scene, false return avoids damage flush |

The retained SDK compiles QTextDocument to8 bytes on ARM, separately from the
16-byte callback value. The join never calls the native resolver, dynamic_cast
or native shared-pointer acquire/release. A nonzero control-block field is only
a required structural check; it is not a newly acquired lease.

Qt affinity is checked before revision access or connection creation. An open Qt
edit block refuses. Native transaction and compose fields are sampled once;
their absence does not establish durable IME quiescence, absence of QTextLayout
preedit, current visible page/visit, owned range or future writer serialization.

## Closure and observation

At most one native invocation and recorder are admitted. A duplicate closes the
session; late callbacks after cancellation never inspect their argument.
Every callback returns false. Caller-side stop waits for any admitted attachment
and recorder publication to settle, then disconnects; the record prefix is frozen
when stop returns. The native callback only try-locks the attachment mutex.
Retirement can wait for the observer's short publication section; Qt emitter
callbacks themselves never wait on that section.

Serialize handle methods and destruction. Saved callback copies may run
concurrently. Do not reenter stop from the native join. snapshot uses a try-lock:
an unavailable snapshot is not proof of settlement. retired is true only after
recorder retirement settled under the attachment mutex; attached means a
snapshot of an open recorder, not ongoing native authority. Poll through snapshot
to remove connections on idle expiry. The observer uses the original absolute
deadline without refreshing it after dispatch.

Logical closure is not callback-copy destruction, native-owner release, or
permission to unload the payload. Keep code resident until the external native
delivery/lifetime owner establishes retirement. Recorder signal-coalescing,
thread and revision limits remain in
[the observer contract](composer-document-diagnostics.md).

## Verification boundary

Focused real-Qt lifecycle controls exercise retained callback copies, duplicate
delivery, exceptions, destruction, original deadline propagation, and
deterministic cancellation during join and after recorder creation. The ARM
control uses owned synthetic Scene/item storage plus a real embedded
QTextDocument to execute the exact join and refusal matrix.

These controls neither invoke Xochitl nor prove that a real focused item accepts
attachment. Actual scene delivery, loaded-profile acceptance, Qt thread affinity,
native callback nesting and live writer behavior remain tablet qualification.
Root owns that integration and scheduling.
