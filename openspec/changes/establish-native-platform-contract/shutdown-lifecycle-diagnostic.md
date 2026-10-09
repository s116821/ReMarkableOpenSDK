# Fresh lifecycle-only shutdown discriminator

October 8 renewed human direction authorizes continued independent source work and necessary targeted development experiments. The earlier blanket requirement to resolve the defect before any native debugging is superseded. Main alone operates RM2 and selects a fresh reviewed artifact and recovery sequence. Spent 0404/301 are never replayed. The external query remains unsent; an upstream answer is not a prerequisite. No firmware/account/sync changes.

## Question and falsifiable observations

H1: native shutdown can leave rendering able to read invalid image backing, without FactsEntry participation. H2: FactsEntry or its observer/completion lifecycle is necessary to this failure. Use a separate lifecycle-only payload, never instantiate FactsEntry or access page objects. A repeat of the same faulting operation/rendering signature without FactsEntry rejects its necessity for that occurrence. A clean stop does not prove H1 false: timing and instrumentation differ. A late render marker after aboutToQuit establishes observed Qt signal order only; it is not a native worker fence or proof of the same allocation error.

No page opening, contact, facts publication, capture request or render forcing. Main observes the first valid before-render marker, then requests the one independent recovery actor. If no such marker is observed within the original selected budget, the actor still has recovery duty; classify the discriminator as not reached. Do not spin a GUI polling loop or renew the budget. A subsequent exact-FactsEntry comparison would be separately selected only if the first result justifies it.

## SDK payload and wire

`qt_shutdown_trace_startup.cpp` registers one context-bound queued application startup callback. A process-resident recorder connects to aboutToQuit/application destruction and one naturally focused QQuickWindow's beforeRendering/afterRendering/destruction. It neither installs an event filter nor changes window state. Direct render callbacks use fixed scalar formatting and one bounded append write; their timing impact is explicit. No QML engine, page getter, controller, image allocation or image read is used.

Compile requires a frozen 32-lowercase-hex `QT_SHUTDOWN_NONCE`. Root is `/run/rmb-qt-shutdown-<nonce>`, mode0700/UID0. `owner` must be a mode0600 single-link regular file containing exactly the nonce (no newline). `attempt.identity` must contain `<pid> <start>\n` with the current process identity. The recorder exclusively creates mode0600 `shutdown-trace.log`. No native nonce is selected by fixture compilation.

Each line is exactly `v1 sequence event monotonic_ns tid pid start quit_seen dropped frames\n`, decimal scalars and fixed event names: startup, window, before-render, after-render, about-to-quit, window-destroyed, application-destroyed, late-before-render, late-after-render. Log at most32 ordinary frame pairs, the first before/after render after quit, and lifecycle events; hard limit96 records of fewer than192 bytes each. Sequence is local recorder order, not a whole-system ordering guarantee. The frame counter includes unsampled frames. Contended writes increment dropped; I/O failure latches recording off. Absence, truncation, nonzero dropped, cap/error, or incomplete destruction markers cannot establish successful shutdown. Crash traces are useful partial evidence and remain partial. The process-resident recorder avoids deletion during callbacks; it is not a production ownership model.

## Consumer recovery requirements for Main/Sol review

Before the first stock stop, pre-arm the independent singleton actor and install a task-owned runtime guard. Verify effective OnFailure empty, OnFailureJobMode selected exactly, FailureAction none, StartLimitAction none and Restart no; preserve watchdog and unrelated policy. Preserve original unit/drop-in hashes and all original effective policy. The guard contains the identified systemd OnFailure reboot route, not every possible reboot cause.

The same actor handles host-requested stop and timer/host-loss recovery. STOP under the guard, verify exact attempted PID/start and all selected cgroup members gone, then remove only owned activation/guard files, reload, start stock once and verify original policy/services/fixtures. Do not restore stock OnFailure before the attempted process is gone. The absolute deadline covers stop, gone verification and stock start; no physical retries or hidden deadline reset. Stop crash/result and final stock state are separate fields; a crash cannot be rewritten as a successful diagnostic stop even if stock is recovered. Timeout disposition must be explicitly reviewed against actual service stop/kill/watchdog bounds; do not improvise kill/reboot changes or remove protection while a candidate survives.

## Acceptance scenarios

- WHEN root/owner/attempt identity is wrong or trace already exists, THEN no recorder attaches and no existing trace is overwritten.
- WHEN render callbacks exceed the sample cap, THEN output remains bounded and first post-quit render markers remain available.
- WHEN a callback overlaps a write or an append fails, THEN telemetry is incomplete; the consumer never turns missing evidence into success.
- WHEN the first render marker is verified, THEN Main may request only the previously armed actor, preserving its original deadline and one-stop/no-retry semantics.
- WHEN candidate stop crashes, THEN retain signal/result/core evidence and distinguish failed diagnostic stop from separately verified recovered stock.

Source-only validation is limited to exact ARM compile and focused owned recorder/signal fixtures. Hardware safety is reviewed on the selected whole packet; fixture passes do not qualify native shutdown. SDK implementation, Buddy operator, Main selection, actual findings and full lifecycle closure remain separate checkpoints.
