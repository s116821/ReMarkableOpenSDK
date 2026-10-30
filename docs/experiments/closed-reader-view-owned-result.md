# Observing a closed Reader through its known UI composition

The new [observer](../../scripts/reader_closed_view.h) reports a sampled closed
document state through the known legacy MainView composition. A missing Reader,
hidden Loader, loading state or inactive document flag alone is insufficient.
This source/owned result does not qualify a native close or acknowledge saved ink.

It first uses a separate read-only branch of the existing legacy composition
census. The branch never selects or calls the navigator's open method. Within
that guarded callback, a fresh MainView lexical scope must bind root to the
selected main and documentView to its Loader. The Loader must be synchronous,
Ready by its declared enum, hidden, and retain a nonnull item with a typed null
document pointer and documentLoaded false. MainView documentViewActive must also
be false. Objects, metadata, parent/window/engine relationships and notifying
properties remain guarded while getters execute. No borrowed context or source
guard escapes the callback.

An actual owned QML test exposed that a component root's qmlContext can identify
its instantiation scope. The observer therefore checks the root and its bounded
immediate visual children's context ancestry for the exact MainView lexical
bindings. It refuses if those bindings cannot be established; it does not broaden
the search to arbitrary missing or hidden Reader objects.

A separate counterexample reproduced a final-callout bug: after the observer's
watchers unwound, a further provider getter could change document state without
invalidating its accepted observation. Original R4 host/ARM/sanitizer controls
reported post_read_provider_call=1 and failed. The repaired read branch performs
its final composition guard inside the callback while those watchers live, then
performs only the non-provider context/deadline checks. Original R5 controls
report post_read_provider_call=0 and pass25 scenarios/177 checks in each mode,
with identical stdout and empty stderr.

The tests use actual owned Qt/QML contexts, scoped IDs, focused windows, signals,
getters and deletion/reentry. Loader/native provider types and document state are
explicit models. Controls include missing/loading/error state, asynchronous or
visible Loader, nonnull document, loaded document, wrong parent/route, notified
ABA, cancellation, overrun, deletion during getters, returned-item loss before a
permission callback, and callback exceptions. Existing generic navigation passes
on host and ARM; existing legacy composition navigation passes on ARM. The latter
two compiler streams retain GCC psABI notes, distinct from passing test stdout.

Observer SHA-256:
`3494a0bbefee8b55e4dbca15a5aaaa0451817fd96ae3bdd71f527dd8a3e7202f`.
Navigation header:
`319c022c1a7cb7294c52346fa6e8fa383dc8eb7fb40c079b7cc6471de0438e0a`.
Independent71-check source/frozen-artifact review receipt:
`dfb531ae0822a73d9d64a69c1f8abfc2d8e335afed0644041d0a62582a58b245`.
Private original matrices, earlier fixture failures and regression streams have
no public download links. R1 lacked registration of the owned visual pointer
type; R2 exposed the lexical-scope assumption. Neither is native evidence.

Callers must require observed, compositionObserved and empty refusal together.
This is a sampled UI state, without a storage fence, render acknowledgement,
global ownership exclusion, safe unload or native resource-drain guarantee.
The [ordinary close dispatch](ordinary-reader-close-owned-result.md), this
closure observation, target ink, persisted file changes and fresh reopen remain
separate facts in the [planned sequence](../../openspec/changes/establish-native-platform-contract/write-reopen-sequence.md).

Source basis: original owned implementation and QML, actual saved host/vendor
ARM-QEMU/sanitizer artifacts and streams, and independent source/original-byte
review. Relationship to future native persistence testing is an explicit plan.
