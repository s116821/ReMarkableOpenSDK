# Capture owner diagnostic source checkpoint

This additive source implements the reviewed `capture-owner-diagnostics.md`
contract. It remains default-off development instrumentation. Independent source
review, exact artifact selection, target qualification and integrated navigation
remain separate gates.

The existing owner traversal and compound predicate order are retained. Optional
sinks record only the original reached counters, literal predicate groups and
fixed observer roles/members. The failure latch samples the clock after the
original failed evaluation, before the unchanged owner-refused completion. It
does not claim an earlier getter timestamp. Deferred publication serializes the
latched scalar record under retained root/generation/closure guards, using the
existing exclusive bounded file writer. Later getters cannot replace the record.

Source validation on 2026-10-07 used the vendor Qt 6.10.3 ARM SDK under QEMU:

- `qt_capture_observation_test.sh`: 42 cases passed, including 17 diagnostic
  cases for initial/deadline progress, failed discovery, zero candidates,
  unavailable window, observer metadata, final revalidation, foreign output,
  closure and generation replacement, discovery progress loss, final discovery active-owner loss, nested reentrancy with/without queued event dispatch, candidate/topology bounds and ambiguity.
- Diagnostic fixtures checked the 29-field record, mode 0600, immutable failure
  time, original callback shape/stage, unchanged getter counts, and no capture
  image/completion authority. A sink/no-sink discovery comparison also checked
  getter and progress counts.
- `qt_page_facts_entry_test.sh`: normal exit zero; 32 entry cases, 21 refusal
  format cases, 17 input observation cases and the original 25 capture cases
  passed. These are synthetic source checks, not real-tablet evidence.

Every emitted diagnostic fixture compares the complete 29-field expected record, including every null. Nested queued failure is latched before the later outer failure; callback delivery remains held until Scope exits. Finite branches beyond the explicitly exercised diagnostic fixtures still need
independent review of their instrumentation and decoder matrix. No new nonce,
artifact or native trial is selected by this checkpoint.
