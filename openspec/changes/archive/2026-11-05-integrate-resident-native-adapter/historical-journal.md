# Historical journal compatibility

The protocol v3 live endpoint remains strict: v1/v2 are not negotiated transport
versions. Existing consumer journals nevertheless retain canonical v1/v2 requests and
responses. Upgrading the SDK must not rewrite those bytes, recalculate a different
request identity, drop prior operations or clear an unresolved reservation.

Add a separate read-only HistoricalFrame decoder for a complete length-prefixed
canonical frame. It accepts only known revisions 1, 2 and 3, applies the corresponding
tag grammar, retains the exact original bytes and exposes their original SHA256
alongside immutable decoded fields and revision. The shared internal codec may
take a private revision argument; normal Frame encode/decode remain v3 only. No
public legacy encoder or historical transport send path is added.

A consumer verifies stored digests against historical bytes and response
correlation against the original request digest, never a re-encoded current frame.
Request/response revisions must match. Existing JSON records stay byte-identical
within their stored frame/digest fields when later records are appended. Malformed
or unsupported evidence refuses loading without overwriting anything.

History can supply original source/target facts to a new read-only reconciliation
request with fresh connection/operation identity. It cannot replay the old effect,
settle uncertainty, grant native authority or remove a conversation reservation.
Selection reservation policy and product binding decisions remain separate work.

Verify canonical retained v1/v2 vectors, original request/response digest correlation,
strict live rejection, v1 rejection of v2-only tags, v1/v2 rejection of capture tags, malformed prefixes/trailing
bytes, unknown revisions, mixed revisions, exact-byte retention and consumer
reservation survival across upgrade. Validate current v3 behavior independently.
Historical v1/v2 payloads retain their 64 KiB limit; the capture allowance applies
only to revision 3 Captured replies.

Source basis: retained consumer journal requirements, the SDK codec changes and original
retained owned wire vectors. No deployed journal is assumed; compatibility is
required before changing the consumer pin.
