# Observe joined Reader focus through the resident diagnostic transport

## Why

The accepted synchronous Reader join can classify the active focus leaf. The
resident Observe reply carries page and order facts. Add an opt-in historical
focus receipt so the same authenticated GUI owner can report that classification
for one request.

## What Changes

- Add one bodyless development request and one single-byte enum receipt to
  revision 4 of the existing resident protocol.
- Run the accepted sampler once inside the existing selected Reader join using
  the original request's permit and deadline.
- Keep the receipt separate from ordinary observations, capabilities, currentness
  and native composer authority.

## Capabilities

### Modified capabilities

- `resident-native-adapter` gains a private development focus diagnostic.

## Impact

Precode for independent review. Baseline SDK
`146d78c9eded3dcfbfe84d55ca2de353e1996dff` and the REM-61 accepted Reader overlay
SHA-256 `81232f41fb880c34d23036ab7eb8252f4c7fee8626a2686a5d71cc6e5d2b62e0`.
Implementation belongs in the existing native dispatcher and paired SDK codecs.
The accepted candidate and ordinary build inputs are protected. The native
diagnostic requires its own exact opt-in artifact review.
