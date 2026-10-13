# SFTP session identity and bounded cleanup

Status: October 13, 2026 source findings and operational recommendations. No
device qualification or watchdog implementation is claimed. The active
[passive visual observation method](../../openspec/changes/establish-native-platform-contract/passive-visual-sftp-method.md)
at `91f0973` retains its schema, access method, manifest and caps. These findings
address its existing identity and outside-supervisor questions; they add no
gate or dependency on the separate document-relationship research.

## Verified source and saved evidence

The existing vendor SDK 5.8.203 ARM sysroot contains `sftp-server` with SHA-256
`6b0a9c32f66e9acde3025c1038600aed607f0543967fe41c047a3e76f7a82516`,
matching Main's retained installed-file receipt. Local static inspection tied
the native READ dispatch to a seek using the supplied offset, followed by one
read. The positive result sends DATA using the returned length; the inspected
success path does not retry to fill a short read. This supports the host's
short-read refusal. It neither identifies a running child nor establishes
process-memory access or a completion deadline. The binary was read as data,
not executed.

Private evidence identifier `sftp-session-identity-cleanup-source-20261013`,
SHA-256 `d5f65f5ddaf3deed7c00359bd9e3d18ce87a8f407d8bd097f5a1c1b5ef46dd72`,
records the binary/trace correspondence. It has no public download link; raw
disassembly, addresses and runtime receipts remain private. The
[upstream OpenSSH READ handler](https://github.com/openssh/openssh-portable/blob/V_9_9_P2/sftp-server.c)
provides supporting semantics, not proof of the installed binary's version.

[Dropbear 2025.88](https://github.com/mkj/dropbear/blob/DROPBEAR_2025.88/src/svr-chansession.c)
maps the SFTP subsystem request to a configured executable path, then permits
forced-command overrides. Thus defaults and INIT/VERSION responses do not
identify the executable that actually serves a connection. The retained stock
configuration uses per-connection Dropbear, not an established OpenSSH daemon
configuration. A missing `sshd_config` does not establish absent SFTP service.

## Recommended same-session attribution

Hold the owned standard SFTP session after INIT/VERSION, without file OPEN,
READ or extension requests. Retain the local SSH process handle/creation identity
and its exact established TCP tuple. Saved unit names include both endpoints;
match that client port to one fresh Dropbear unit, then use its MainPID and
ControlGroup to inspect only bounded related child identities. Corroborate the
serving child's PID/start identity, parent chain or cgroup membership, and
executable inode/hash through its process executable reference. Recheck while
the same connection remains held. The independent metadata connection has a
different client port and must be excluded.

This is a recommendation: current unit/cgroup visibility and a unique child
association remain live assumptions. Ambiguity or truncated metadata cannot be
repaired by selecting a global `pidof` result. A completed preparatory handshake
identifies only its own child, not a later observer fork. Any later instance
binding must fit the already fixed observation bounds; preparation must not
silently substitute for it.

## Recommended host ownership; remote limits

An outside Windows supervisor can own the observer and inherited SSH children
through a [Job Object](https://learn.microsoft.com/en-us/windows/win32/procthread/job-objects):
create the observer suspended, assign it before resume, enable kill-on-job-close,
and allow neither breakaway nor inheritance of the supervisor's job handle.
Failed assignment must refuse before connection. Arm the absolute deadline
before release; terminate only the owned job, then check retained process
handles and the job's active-process count with a bounded wait. Record actual
elapsed time and overruns. This design is recommended, not implemented or
tested here, and is not a hard real-time guarantee. It covers gaps that an
in-process timer cannot cover, including its own stalled cleanup or final
output. [Process.Kill(true) followed by WaitForExit](https://learn.microsoft.com/en-us/dotnet/api/system.diagnostics.process.kill?view=net-9.0)
does not by itself confirm descendant termination.

Remote cleanup needs separate observation after closing owned stdin and bounded
client shutdown. Check the recorded remote process identities and exact cgroup;
distinguish gone, live, zombie, reused PID and unobservable. The saved unit uses
`KillMode=process`; [systemd documents](https://github.com/systemd/systemd-stable/blob/v255.21/man/systemd.kill.xml)
that this mode targets only the main process. An inactive unit therefore does
not prove child cleanup. No target signal or service restart is proposed.

An INIT-only cleanup result applies to that idle session. Host cutoff cannot
guarantee that a future server blocked in a kernel read consumes EOF or releases
resources by a deadline. Report unknown where unobserved; do not convert the
host deadline into a remote cleanup claim or a new blanket pause.
