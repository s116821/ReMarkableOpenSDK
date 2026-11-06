# Change: Capture one borrowed text diagnostic before stock Enter insertion

## Why
Queued scene diagnostics cannot establish the text present immediately before
an accepted stock Enter edit. The retained RM2 path has a narrower boundary:
the newline callsite already holds the focused text item's native strong owner.
A synchronous bounded copy there can preserve historical pre-edit data while
stock editing continues once.

## What Changes
- Add one prepared, cancelable, deadline-bound diagnostic capture at the exact
  ordinary Enter newline callsite in the admitted RM2 profile.
- Share the existing private join/copy/revalidation logic with Attachment, adding
  correspondence to the stock owner pair borrowed by this invocation.
- Implement an ARM callsite bridge with a compiled C++ forwarding shim, explicit
  register/unwind requirements and a guarded installation seam.
- Validate the real bridge and patch route in an owned ARM executable, then
  qualify selected native behavior through the existing runtime workflow.
- Keep unsuccessful capture independent of stock editing: preserve original
  arguments, one stock invocation and original exception propagation.

## Capabilities
### New Capabilities
- `composer-pre-enter-diagnostics`: one historical text copy before stock LF
  insertion, with exact-build, owner, composition and lifetime admission.

### Modified Capabilities
None. Existing queued Attachment/Delivery contracts retain their admission paths.
Private read code may be factored without changing their public behavior.

## Impact
Baseline: actual SDK main d107c2bf778ce35682a6f47e5250f5599d3d7c7c,
released as v0.6.0. Proposed implementation belongs under
`native/rm2_3_28_0_172/`, with reuse of current text-copy and resident lifetime
controls. No generic recorder, public raw-native-pointer API or new owner system.

This change does not issue semantic Submit, owned-range/current-visit, conditional
clear or retirement authority. List unindent remains stock behavior outside the
LF capture site. Runtime hook installation requires an established quiescent
installation boundary; source and owned tests alone cannot enable it.

This initial commit prepares the formal change and implementation plan only.
Implementation, checks, spec sync and archival stay in the same eventual change
PR. Keep implementation tasks open until their actual evidence exists.
