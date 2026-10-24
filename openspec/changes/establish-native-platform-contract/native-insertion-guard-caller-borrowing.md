# Ordinary caller-borrowed carrier policy

This separately refines the [runtime contract](native-insertion-guard-runtime-core.md)
after inspecting the ordinary explicit-size Task producer. The historical owned
exact-pointer controls remain evidence for that mode, not a native pointer-
identity requirement. No native implementation or action is selected here.

## Justified values and dynamic fields

The ordinary explicit-size entry constructs a present QSizeF and an absent
source/inherit pair. Task, queued request and dispatcher preserve these values.
The numeric word in the absent source pair is not established as zero, minus
one or the GUI source index. Its engagement byte is zero. The dispatcher builds
the optional template argument and UUID value on its own worker stack, so their
addresses cannot be frozen by the GUI before submission.

Add an explicit policy choice, leaving exact owned binding the default. The new
caller-borrowed explicit-size policy requires:

- The same fresh UUID value recognition, complete worker/incarnation, actual
  worker thread, exact original outer return PC, unique scope and insertion
  index checks. A recognized mismatch cannot become transparent work.
- Frozen size-present word exactly one, inherit word exactly zero and an exact
  finite positive bounded QSizeF from the actual public invocation. Actual
  scalar words must match those qualified values without truncation. A missing
  size or present inheritance refuses before the original.
- A nonnull, four-byte-aligned actual optional pointer, preserved opaquely for
  the original dynamic call. A null frozen optional pointer indicates that no
  GUI pointer equality is asserted. Do not construct, copy, decode or declare
  a native private optional type. Alignment is only a refusal check; provenance
  supplies normal native stack-object lifetime.
- Preserve the original numeric source word without comparing it to a frozen
  GUI integer or interpreting it as page identity. Its source/inherit engagement
  is zero on this route. The final source/order check comes from the separate
  under-lock predicate and frozen page identities.

Reject malformed policy configuration before registration, including a claimed
GUI-known optional address or flags inconsistent with this explicit-size route.
Do not add a broad ignore-all-arguments option. Preserve all actual carrier bits
when invoking the original; exact owned mode retains its prior behavior.

## Owned verification and separate native gate

Extend only the focused hook controls. Check that caller-borrowed mode accepts
different valid dynamic optional addresses and unrelated opaque numeric source
words while forwarding their exact bits. Null/misaligned optional pointers,
wrong size/flags, wrong outer caller/worker/thread/index and malformed policy
registration must refuse without forwarding tracked insertion. Keep the
negative-only barrier, cancellation, terminal publication and original cleanup
unchanged. Preserve the exact earlier source/results before changing this mode.

These controls do not establish original optional content, template identity,
the native ABI/provider/owner facts, loader ordering or actual Task correlation.
Do not extend this policy to absent-size/inheritance routes or accepted native
insertion without their own evidence. A passing owned result does not authorize
page creation or make the full native witness implemented.

Source basis: retained explicit-size producer, Task/request copying and dispatcher
machine-location evidence, original handler branch use, and the owned hook core.
The policy is proposed original interoperability logic; original C++ private
aggregate types are not claimed to have been recovered.
