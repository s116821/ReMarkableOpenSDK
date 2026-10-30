# Original identity-capture pair readback

October 23, 2026 UTC. Owned-only continuation of the fixed
[capture Entry](identity-capture-entry.md). This does not select or execute a
device packet, create a request, parse an image, or add a file-IPC manager.

## One fixed synchronous collection

Provide a read-only `reader_capture_pair_read::readPair` helper, receiving a
borrowed already-open root directory descriptor, owned expected nonce/PID/start
strings, and a fresh externally composed permission. Duplicate the root descriptor
with close-on-exec before any permission callout can destroy its caller. Copy
expected strings and permission before invoking them; retain only local owners.
One thread-local entry gate must refuse nested collection and poison the outer
attempt. A false or throwing permission is sticky for that attempt. No reopening,
retry, event pumping, publication, deletion or effect follows a refusal.

Require a directory owned by the effective user with exact permission bits 0700.
Open and retain all six fixed files before collecting pair bytes: `owner`,
`attempt.identity`, `selected-ready`, `selected-request`, `selected-result`, and
`visual-image.png`. Use read-only, no-follow, nonblocking, close-on-exec descriptors.
Every file must be regular, same-owner, exactly 0600, and single-link. Reject empty
or oversized pair files: scalar at most 4096 bytes, image at most 1 MiB.

Expected nonce is exactly 32 lowercase hexadecimal bytes. Expected PID and start
are nonzero canonical unsigned decimal strings bounded by uint64. Require exact
original bytes, including the identity newline: owner is nonce; identity is
`PID SP START LF`; ready and request are `NONCE SP` followed by identity. These
checks bind declared incarnation bytes and do not establish that a process is
currently alive, that the root is still named at an expected external pathname,
or that runtime/provider/time admission remains valid.

Require absence of `.pending-ready`, `.pending-image`, `.pending-result`,
`selected-cancel`, `restore.claim`, and `stop.request`, including symlinks or
other nonregular nodes under these names. Errors other than ENOENT refuse.

## Brackets and original bytes

Snapshot retained descriptors and named entries. Freshly check permission and
the directory plus all retained descriptor/name identities before and after each
bounded read chunk and at the final boundary. Compare device, inode, type/mode,
owner, link count, size, mtime and ctime; directory entries must remain stable.
Do not compare atime, which reads may legitimately update. Exact snapshot size
and an additional EOF read are required. Retained open descriptors pin the
original inodes across unlink/recreate and caller destruction. No successful
result may expose a partial pair or borrowed descriptor/permission/context.
Explicitly destroy the copied permission while all six files and root remain
pinned, then perform a final no-callout metadata/reentry check and checked closes
before exposing bytes. A captured target destructor can itself mutate files or
reenter; a check preceding that destruction does not cover it. Do not consult a
borrowed input permission after its owner may have died. The final permission
invocation is a sampled external admission; a future wrapper must freshly
revalidate its own live/process/time authority after helper return.

Return owned scalar and image byte arrays only after all final checks pass.
Return an explicit refusal with empty byte arrays otherwise. Do not normalize,
reserialize, hash-substitute or parse those originals. JSON/PNG/schema/digest
validation remains the separate original-byte consumer's responsibility.

This is sampled file stability, not atomic content or live-process authority.
An arbitrary same-owner writer can race between observations or restore bytes
and metadata. An unrelated directory entry can also change without a distinct
directory timestamp on a coarse filesystem; this is not a complete inventory.
Root-path, process incarnation, cancellation, setup/operation
deadlines and runtime/provider authority must be supplied by the external fresh
permission. The helper never certifies those facts from an injected fixture
permission, and success leaves `liveProcessAuthority=false`.

## Owned controls

Use real local files and descriptors on host and ARM/QEMU, with sanitizer
controls where available. Verify exact original binary bytes, boundary sizes,
strict binding bytes, modes/ownership/link/type/symlink refusal, missing/stale
pending nodes, FD retention after source-FD closure and caller destruction,
replacement/rename/unlink-recreate and same-inode metadata changes, partial
reads/truncation, permission loss/throw/reentry at read stages, and FD cleanup.
Permit callbacks may mutate owned files to exercise these boundaries. Such
models establish helper behavior only; no device, native target process or
production external admission is part of this checkpoint.
