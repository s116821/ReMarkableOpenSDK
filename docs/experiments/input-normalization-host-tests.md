# Source-only normalization collector tests

These tools implement the fixed same-call measurement policy and an original owned
ARM fixture. They expose no native runtime command and perform no attachment on
import. They do not implement the proposed disposable-server lifecycle amendment,
complete debugger text ledger, native provider/generation preparation or consumer
recovery guard. The original standard GNU server remains held from native use.

Run the policy tests with Python3:

```text
python tools/test_input_normalization_probe.py
```

Run the ARM integration script only in the pinned vendor container with network
disabled, a read-only root and SDK checkout mounted read-only at `/src`. Mount a
fresh task-private output directory at `/out`; execute
`/bin/sh /src/tools/test_input_normalization_arm.sh`. Image identity:
`sha256:416c7a7be0038156797b0892f031f352b841d1921fae83f712d0a272e4724618`.
The script compiles original fixture code, connects vendor GDB14.2 to owned QEMU
inside that isolated container and checks identity/rotation/nonfinite raw doubles,
two same-call capture stops and condition-flag preservation. Host watchdogs bound
fixture/debugger failure. This does not test actual ptrace, the Windows15.2 client,
SSH, kernel EXITKILL or tablet recovery; none may be inferred from a passing run.

The successful host policy currently requests286 memory bytes. This is not a native
total: explicit server ledger capture/restoration will require shared accounting
before native implementation. GDB internal protocol/decoding accesses are separate
bookkeeping. Raw private evidence is capped at8192 aggregate serialized bytes;
each acquired memory/register entry is persisted before semantic classification,
and its byte count/SHA256 is retained in an append-only fixed-width receipt index.
The index is fsynced and read back before returning from each write, including
release-ready completion. Its bytes count against the same8192 cap; no receipt
self-hash or duplicate raw snapshot is introduced. Completion is persisted before
host-fixture detach. Failed/partial reads are
charged without retry; failure never detaches through the collector success path.
The host fixture command holds on failure until its independent watchdog terminates
the owned processes. This host cleanup is not the proposed target recovery guard.

Source basis: current SDK measurement/lifecycle proposals and the local host tests.
No native authority, coordinate fix or successful device trial is claimed.
