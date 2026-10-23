# Original owned capture pair

The three files in `case-1` are exact exports from the October23,2026 final R4
host Entry fixture. The producer was injected and wrote a known24x16 PNG into
the real Sink. These are synthetic owned bytes, not tablet content or native
readiness facts. The scalar uses the owned model's17/31 readiness enums.

`selected-result` was copied without JSON reserialization; `expectation.json`
records its original size/hash and separate synthetic request/identity facts.
The consumer validates original bytes only and leaves external live admission
unverified. Neither this example nor a successful decode establishes native,
render, durable, atomic, page-content or physical freshness authority.

See the [owned checkpoint](../../../docs/experiments/identity-capture-entry-owned-result.md)
for source/evidence identities and the reproduced lifecycle/publication defects.

Run from the repository root with an existing Python/Pillow environment:

    python -B tools/reader_identity_capture_consumer_test.py --entry-exports tests/fixtures/identity-capture-pair
