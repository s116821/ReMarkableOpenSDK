# Proposed Reader capture boundary

Status: proposed consumer seam; native implementation and this refinement's independent review remain pending. This extends the reviewed creation model without claiming native capture qualification.

## Identity outcomes

`observe_page` returns a known scoped `PageObservation`, or an explicit observation failure:

- `Unsupported(reason)`: this adapter cannot perform qualified identity observation for the detected runtime/capability.
- `UnknownIdentity(reason)`: observation ran but could not establish exactly one current native page. Reasons include no document open, unsettled state, ambiguous identities, missing identifiers and insufficient ownership evidence.

Unknown is not a PageKey with nil/generated UUIDs, a filename/index surrogate, an old last-opened record or an automatically adopted page. UUID validation rejects nil document/page IDs. An operational failure never permits binding, creation, provider-source persistence or native output using a guessed owner.

Unbound is different: a known native PageKey may lack a Buddy conversation binding. That is a consumer lookup result, not SDK identity failure. The SDK does not know whether a conversation exists, whether a source is a legacy QA page, or whether product policy permits starting a new conversation. REM-37 can retain its nonnil source identity requirement unchanged. It must not construct SourceObservation on UnknownIdentity/Unsupported.

If legacy behavior still allows an identity-free diagnostic screenshot, represent it separately as unqualified diagnostic pixels. It cannot be implicitly converted into a qualified source batch or associated with a conversation. Whether a user-visible degraded flow is allowed belongs in Docs/Buddy policy; it does not relax SDK provenance or fabricate identity.

## Immutable evidence batch

A capture request carries one already-known scoped observation, an operation/capture correlation ID, monotonic deadline and cancellation context. Native adapter code brackets acquisition with fresh observations and continuous ownership/input checks; matching before/after UUIDs alone cannot hide navigation away and back. One immutable native acquisition parent produces the overview and details under the same operation/source observation. Loss of guard anywhere invalidates the whole qualified batch rather than returning partial success.

The response is either `Qualified(CapturedBatch)` or an explicit unsupported, unknown/stale ownership, canceled or capture-failed outcome. There is no native mutation in capture, so a failure does not imply page creation; existing creation indeterminacy remains a separate type.

Stable identity around capture does not establish pixel freshness: the logical page may already be B while the selected buffer still contains page A. The qualified acquisition procedure must bind the acquired logical rendered content/frame generation to the observed page, content revision and viewport state, or establish an equivalently qualified render-to-acquisition relationship. Carry that render/acquisition evidence in the batch's procedure facts. Reject stale or unbound buffers even when before/after identity matches. Recent timestamps, equal digests, repeated reads and fixed sleeps cannot substitute. This concerns logical rendered-content provenance, not waiting for a complete physical e-ink refresh. If no such procedure is qualified, native capture remains unsupported rather than assigning pixels to a plausible page.

CapturedBatch owns immutable image buffers (for example Arc<[u8]> behind private fields), their SHA-256 digests computed over those exact encoded bytes, and immutable metadata. No borrowed mutable framebuffer, mutable global last-image cache, inferred file identity or caller-supplied unverified digest is accepted as qualified evidence. Production adapters construct qualified batches internally; synthetic constructors are feature-gated and explicitly synthetic.

Required batch facts:

| Fact | Semantics |
| --- | --- |
| Source | Exact native document/page plus device/adapter instance, runtime session, visit, content and order revision evidence; opaque tokens are not globally comparable clocks |
| Procedure | Semantic capability/contract revision, adapter qualification/procedure identity and evidence origin |
| Render/acquisition binding | Qualified evidence connecting the actual buffer generation/content to the observed page/content/viewport state; not a caller-supplied timestamp assertion |
| Acquisition interval | Monotonic start/end in the observation's clock scope, plus optional wall-clock timestamp with explicit clock provenance; UTC alone is not freshness proof |
| Native acquisition parent | Exact immutable encoded native-resolution pixels, computed digest, validated MIME/encoding/nonzero decoded dimensions and acquisition/conversion procedure; retain this parent even if it is not submitted to a provider |
| Overview | Exact immutable bytes, digest, validated MIME/encoding/dimensions and explicit derivation from the native parent, including resampling/normalization procedure |
| Details | Each has exact bytes/digest/encoding/dimensions and the native acquisition parent ID/digest, parent pixel dimensions and crop rectangle plus any resampling procedure |
| Geometry | Finite invertible affine image-pixel-to-normalized-source transform, with declared coordinate convention/viewport and orientation |
| Target | Optional normalized source target, validated against the same source/transform; absence does not mean whole-document permission |

Coordinate convention: image coordinates use continuous pixel-edge units, origin at top-left; an image spans [0,width] by [0,height]. A pixel center is (x+0.5,y+0.5). Normalized source coordinates refer to the observed native page plane, origin at top-left, with the page rectangle [0,1] by [0,1]; viewport offsets, scrolling and rotation are encoded in the transform. UI chrome/padding outside that page plane is represented by a valid-source region or masked out, not mislabeled as page content. Do not clamp invalid coordinates into range silently.

Each derivative's rectangle uses checked native-parent pixel bounds, and its transform must equal the parent transform composed with crop origin and declared resampling. Overview and detail are siblings; a detail must not claim the resampled overview as its pixel parent. For an independently recaptured image, mark a separate acquisition with its own guarded interval/provenance; that is outside this initial same-acquisition contract.

Pixel lineage requires more than metadata validation. A trusted SDK image-derivation path generates overview/detail pixels and encoded bytes from the validated immutable native parent using an explicit algorithm/version/parameters, or verifies an equivalent derivation before accepting it. Arbitrary valid image bytes cannot be tagged as a crop merely by supplying a matching parent digest and plausible bounds/affine transform. Re-encoding/resampling behavior must be reproducible under the recorded procedure; a digest identifies the result bytes but does not prove their derivation.

Current Reader topology, verified in Buddy origin/main ff8ad75 `src/device/screenshot.rs`: `take_screenshot` captures a native image once, serializes native_data, normalizes/resamples the overview and publishes both only after completion. `detail_images_base64` decodes that native_data and crops three overlapping full-width strips at y=0,(h-th)/2,h-th where th=h*2/5. It does not recapture or crop from the 768x1024 overview. Preserve this exact full-resolution detail behavior and store the native parent; do not downgrade provider detail resolution to fit the API. In this path, the parent is the lossless PNG of the owned native-resolution image after the declared device pixel conversion, not the original raw framebuffer byte layout. Record that conversion accurately. A future raw pixel-plane representation would need explicit format, dimensions, stride and validated extent, and must not be mislabeled with an encoded image MIME type.

Validate byte/count/decoded-dimension limits before allocation or decoding; reject malformed images, MIME/dimension mismatches, nonfinite/singular transforms, overflow/out-of-bounds crops, missing/foreign parents, mixed owner/visit/revisions and inconsistent crop transforms. Immutable storage alone does not prove correct pixels or source ownership.

## Consumer handoff

The incremental DeviceBackend adapter maps SDK capture outcomes into existing Reader workflow results. On Qualified, the conversation domain persists the exact native parent, overview/detail batch and source facts before any provider call, then retrieves immutable stored bytes. Parent persistence is required even when only overview/details go to the provider. Storage owns its content-addressed keys and retention policy; the SDK owns native observation/capture facts. Neither layer silently recomputes a crop or re-encodes an image after the digest/source record is committed.

The SDK owns device/image semantics, not model-specific payload preparation. If a provider needs additional resizing, crop, masking or re-encoding, Buddy creates an explicitly labeled immutable derivative from retained evidence, records its algorithm/parameters, parent/geometry and digest, and persists the exact submitted bytes before dispatch. It must not label that image as the original native acquisition. Original source evidence remains retained under consumer policy. The initial consumer contract assumes existing overview/detail PNGs need no further image transformation; integration must verify that assumption (base64 transport encoding does not change the decoded image bytes). Any deviation requires explicit derivative lineage rather than an SDK provider-specific branch.

Source identity loss yields an explicit unsupported/unknown-source workflow outcome before creating the REM-37 source record/provider request. A known source without a conversation binding follows the consumer's normal binding/start policy. No raw hardware access is added to the conversation domain.

Persisted facts never confer operational authority. After provider completion or a page revisit, native output requires a newly checked device/adapter/session guard and the correct Buddy binding. `CapturedBatch`, stored SourceObservation and historical CreationReceipt are all insufficient alone. Existing native output outcomes must distinguish unsupported or rejected-without-mutation from uncertain post-dispatch effects; typed text acknowledgement is not automatically durable completion.

## Acceptance before native use

- Unknown/unsupported identity cannot construct REM-37 SourceObservation or enter the qualified capture path; known-but-unbound remains distinguishable.
- Nil UUIDs and forged identity surrogates are rejected.
- Capture rejects page change and away/back input between brackets, foreign/recreated adapter guards and mixed-frame batches.
- A stable page-B guard with a stale page-A buffer fails render/acquisition binding even though identities before/after match; no physical-panel-refresh wait is inferred.
- Original native parent/overview/details remain byte-identical across persistence and provider retrieval; wrong MIME/dimensions/digest/crop/transform/parent fail deterministically. Full-resolution details must not be derived from a downsampled overview.
- An unrelated but valid detail image with plausible parent digest, crop bounds and affine transform is rejected unless the trusted parent-pixel derivation is established.
- Rotation, scrolled viewport and resampled crop geometry are tested against fixed synthetic fixtures, with native geometry qualified separately.
- Provider/output integration tests prove source persistence precedes dispatch and persisted evidence cannot grant native rendering permission.
- Any provider-specific image transformation is an explicit immutable derivative stored before submission, preserving exact submitted bytes and required original source evidence.

This refinement requires root API review and consumer-owner agreement before native integration. It does not authorize a new device experiment or mark existing SDK tasks complete.
