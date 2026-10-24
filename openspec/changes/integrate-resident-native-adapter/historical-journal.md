# Historical journal compatibility

The protocol v2 live endpoint remains strict: v1 is not a negotiated transport
version. Existing consumer journals nevertheless retain canonical v1 requests and
responses. Upgrading the SDK must not rewrite those bytes, recalculate a different
request identity, drop prior operations or clear an unresolved reservation.

Add a separate read-only HistoricalFrame decoder for a complete length-prefixed
canonical frame. It accepts only known revisions 1 and 2, applies the corresponding
tag grammar, retains the exact original bytes and exposes their original SHA256
alongside immutable decoded fields and revision. The shared internal codec may
take a private revision argument; normal Frame encode/decode remain v2 only. No
public legacy encoder or historical transport send path is added.

A consumer verifies stored digests against historical bytes and response
correlation against the original request digest, never a re-encoded v2 frame.
Request/response revisions must match. Existing JSON records stay byte-identical
within their stored frame/digest fields when later records are appended. Malformed
or unsupported evidence refuses loading without overwriting anything.

History can supply original source/target facts to a new read-only reconciliation
request with fresh connection/operation identity. It cannot replay the old effect,
settle uncertainty, grant native authority or remove a conversation reservation.
Selection reservation policy and product binding decisions remain separate work.

Verify canonical retained v1 vectors, v1 request/response digest correlation,
strict live rejection, v1 rejection of v2-only tags, malformed prefixes/trailing
bytes, unknown revisions, mixed revisions, exact-byte retention and consumer
reservation survival across upgrade. Validate current v2 behavior independently.

Source basis: current v1 consumer journal, the SDK v1-to-v2 codec diff and original
retained owned wire vectors. No deployed journal is assumed; compatibility is
required before changing the consumer pin.
