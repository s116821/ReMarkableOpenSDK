# Bounded endpoint lifecycle observation

Candidate8e62675 reached normal Reader library startup, then its explicit visual
reconnect failed with ConnectionReset before any capture API invocation. Retained
B phase/error records establish that boundary, not the server branch. Endpoint
source has separate stale-peer/read-stack/credentials rejection and fixed Hello
and operation timer retirement. Queued GUI callbacks can affect ordering; no
specific cause is claimed from elapsed time alone.

Build the explicit observation-endpoint-diagnostics-v1 variant with unchanged
mutation-disabled profile bytes and all original peer, directory, protocol and
deadline checks. Bound direct stderr output to64 fixed-field records of at most
255 bytes. Record accept reason, Hello/Ready, timer phase and retirement branch;
never record payload/pointers or alter timing/retry/takeover. Preserve source and
build artifacts for the next separately selected native candidate. Build only,
no local unit/lint gate and no native qualification or product capability claim.

## Observed stale disconnected peer and narrow retirement

Candidate d7ecb0 retained generation1 after Ready, then logged
reject-existing-peer with QLocalSocket::UnconnectedState, ready=true,
reading=false, pending=false and empty input/output buffers. This identifies
an old disconnected Qt transport retained before its queued callback, not a
Hello timeout or capture API failure.

Before accepting an authenticated replacement, retire that old transport only
when disconnected, outside its read/submit stack, without pending lease/output
deadline, with an idle unpoisoned decoder and empty buffers. Use the existing
retirement path; retain executor effect debt/quarantine unchanged. Connected,
partial, active or pending old states still refuse the new fd. All queued old
socket callbacks, read continuations and backend exception continuations bind
both their original QPointer and nonwrapping connection generation. No takeover,
retry, timeout extension or native owner reclamation is introduced. Build the
trace-fix artifact separately, then qualify the next fresh native candidate.

## Kernel EOF correction after8449

Candidate8449 retained the same otherwise idle peer with Qt ConnectedState;
the earlier Qt UnconnectedState requirement therefore missed delayed state
notification. Check an old peer only after every existing idle invariant passes,
then call recv(descriptor, one_byte, MSG_PEEK|MSG_DONTWAIT) once. Only return0 is
terminal proof for this AF_UNIX stream; positive data, EAGAIN/EWOULDBLOCK,
EINTR/other errors or invalid descriptors refuse replacement. No bytes are
consumed and no descriptor flags, clocks or retry rules change. The prior
Qt-state-only retirement is superseded, not treated as native success.

Trace old-peer-idle-guards as a bit mask:1 reading,2 pending,4 output deadline,
8 nonidle/poisoned decoder,16 Qt buffered input,32 queued output. Blocked guards
skip recv. old-peer-peek code0 is EOF,1 data, negative errno is refusal; -EBADF
also covers unavailable descriptors. Retire-eof-idle-peer records the admitted
terminal path. Original socket/generation guards and executor debt remain.
Linux recv(2) documents EOF0 for orderly stream shutdown, MSG_PEEK without data
removal and per-call MSG_DONTWAIT: https://man7.org/linux/man-pages/man2/recv.2.html

## Observed terminal-proof union after692

Candidate692846 logged all idle guards clear, Qt UnconnectedState and an
unavailable descriptor (-EBADF probe). Qt had already completed closure; this
is distinct from an error returned on a still-nonnegative descriptor. Combine
both observed terminal states under the same idle guards: either Qt Unconnected
AND socketDescriptor<0, or a valid descriptor whose one-byte nonblocking peek
returns EOF0. The former logs retire-qt-closed-idle-peer; the latter retains
retire-eof-idle-peer. A valid-fd EBADF/other error, EAGAIN or data still refuses.
All callback/generation guards, native debt and original clocks remain unchanged.
