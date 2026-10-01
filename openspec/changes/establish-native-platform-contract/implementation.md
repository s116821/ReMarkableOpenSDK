# Experimental implementation scope

The experimental Rust crate is version 0.0.0 with publishing disabled. It is an original semantic state-machine model, not a native adapter, stable public release or complete SDK contract. No production capability is advertised.

Implemented prototype pieces:

- Validated canonical UUID/PageKey and separate opaque device/adapter/session/visit/revision/input observation scope.
- Immutable request objects and complete-request equality for operation correlation. This is not yet a canonical serialized cryptographic fingerprint.
- Explicit unsupported, canceled-before-dispatch, proven no-mutation rejection (before dispatch or at executor), committed and indeterminate outcomes.
- Synthetic receipts, historical reconciliation/current observation separation, and no blind replay in the in-memory mock.
- Mock race injection between dispatch/execution, separate native-assigned allocation simulation, and unsupported native default.
- Explicit Unsupported versus UnknownIdentity observation failures, nonnil UUID validation, and tests separating ambiguous/unsettled/no-open-document cases from valid native identity without a consumer binding. Capture batch design is in capture-contract.md; native capture remains unimplemented.

Outstanding requirements remain unchecked in tasks.md: capability/version/profile manifest; full capture/navigation evidence and APIs; deadline/session lifetime details; canonical request encoding and digest; restart-persistent native correlation; actual adapter serialization; template/paper semantics; device/runtime/provider qualification; consumer integration; SDK licensing/release; native acceptance. The mock's instance counter is a process-local test identity, not a proposed persistent device identity scheme. Its in-memory receipt map does not establish durability or recovery across process restart. Evidence origin exposes only Synthetic at this stage, so callers cannot construct a native-qualified receipt through the prototype.

Synthetic test scenarios cover input/order change between dispatch and execution, two devices sharing UUIDs, recreated adapter scope, same operation with changed request, native-assigned commit with lost reply, historical receipt after target removal, unsupported/canceled non-mutation, and invalid UUID text. These prove properties of the model only. Actual runtime access stays with the designated tablet operator.

Independent design acceptance: root and consumer owner reviewed e63010ef4dd91fc180658750f2de78c0734910c9. The initial model and subsequent bounded synthetic capture implementation received independent review; this does not complete the remaining delivery gates. Native-assigned allocation cannot enter the consumer path without the separately coordinated journal amendment.

## Bounded synthetic capture checkpoint

The accepted capture contract is 49171a079f82958d65481dbedb0d246207704256. Independent ordinary-code review accepted candidate e0175f7483d7c3d4bea2490b15fd24a13cc34f34 (fff34f3 plus e0175f7), integrated without source changes. Sixteen tests and one compile-fail doctest passed, together with formatting and strict Clippy in all-features and no-default-features modes on Rust 1.98.1. Rust 1.88 is a declared floor, not a tested MSRV.

Capture retains private immutable encoded PNG bytes, SHA-256 digests, dimensions, pixel-edge transforms, valid source regions, parent/crop/resize lineage and synthetic scope/clock/render assertions. The model checks bounded headers and decoded pixels against the parent-derived crop/resize while preserving the supplied encoding. Unknown identity cannot carry an unsupported-capability reason. Native capture defaults to Unsupported; only the feature-gated synthetic model creates batches.

These checks do not establish aggregate process-memory bounds, actual runtime acquisition/freshness/cancellation, durable storage, a canonical evidence wire format, Reader integration or hardware qualification. Observation evidence export remains an outstanding consumer API contract; consumers must not serialize Debug output or invent unavailable scope identifiers.

Direct dependencies are image =0.25.10 (PNG only, default features disabled) and sha2 =0.10.9, each MIT OR Apache-2.0. Cargo.lock pins the transitive graph. Reviewed dependency metadata declares permissive license alternatives, but SDK license selection, dependency/publication policy and release notices remain open under task 1.4. The crate remains publish=false; no third-party code is relicensed by this work.
