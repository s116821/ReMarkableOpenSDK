# Bind a key event to the existing Scene read

## Why
The native Scene key-action representation discards QKeyEvent repeat/count and modifier provenance before the existing pre-newline hook. A later independent Scene read cannot reconstruct those facts. The first composer increment must carry the actual receiver's event into the existing one-shot Scene request without claiming Submit or edit authority.

## What Changes
- Add a synchronous selected-input-receiver join using the existing Reader/document/page/controller/worker validation and exact Scene focus.
- Add an opt-in, one-use Qt event filter which copies bounded key facts and enqueues the existing typed Scene read during that join, before forwarding stock input.
- Carry one original absolute deadline through admission and the native callback; cancel on subsequent input or receiver/focus identity changes.
- Use a snapshot-only read without a persistent document observer for the key path; revoke through an atomic latch without blocking stock input. Publish key facts only after the selected join produced an actual native attempt.
- Add a dedicated private typed-read retirement path after exact same-worker/full-ID completion, closed and drained borrowing, and a later non-reentrant GUI turn. Preserve unknown/last-owner quarantine and the generic Task exclusion.
- Preserve all stock key behavior, native owner retention when proof is incomplete, and unsupported-profile refusal. No runtime selection, product shortcut grammar or draft clearing is introduced.

## Impact
SDK native composer and selected-view helper source and owned fixtures. The parent pre-enter diagnostic change remains incomplete. Buddy submission policy and central consumer specifications require separate coordination; this increment is not a usable composer or v1 completion.
