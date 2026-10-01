# Experimental implementation scope

The initial dependency-free Rust crate is version0.0.0 with publishing disabled. It is an original semantic state-machine model, not a native adapter, stable public release or complete SDK contract. No production capability is advertised.

Implemented prototype pieces:

- Validated canonical UUID/PageKey and separate opaque device/adapter/session/visit/revision/input observation scope.
- Immutable request objects and complete-request equality for operation correlation. This is not yet a canonical serialized cryptographic fingerprint.
- Explicit unsupported, canceled-before-dispatch, proven no-mutation rejection (before dispatch or at executor), committed and indeterminate outcomes.
- Synthetic receipts, historical reconciliation/current observation separation, and no blind replay in the in-memory mock.
- Mock race injection between dispatch/execution, separate native-assigned allocation simulation, and unsupported native default.
- Explicit Unsupported versus UnknownIdentity observation failures, nonnil UUID validation, and tests separating ambiguous/unsettled/no-open-document cases from valid native identity without a consumer binding. Capture batch design is in capture-contract.md; native capture remains unimplemented.

Outstanding requirements remain unchecked in tasks.md: capability/version/profile manifest; full capture/navigation evidence and APIs; deadline/session lifetime details; canonical request encoding and digest; restart-persistent native correlation; actual adapter serialization; template/paper semantics; device/runtime/provider qualification; consumer integration; SDK licensing/release; native acceptance. The mock's instance counter is a process-local test identity, not a proposed persistent device identity scheme. Its in-memory receipt map does not establish durability or recovery across process restart. Evidence origin exposes only Synthetic at this stage, so callers cannot construct a native-qualified receipt through the prototype.

Synthetic test scenarios cover input/order change between dispatch and execution, two devices sharing UUIDs, recreated adapter scope, same operation with changed request, native-assigned commit with lost reply, historical receipt after target removal, unsupported/canceled non-mutation, and invalid UUID text. These prove properties of the model only. Actual runtime access stays with the designated tablet operator.

Independent design acceptance: root and consumer owner reviewed e63010ef4dd91fc180658750f2de78c0734910c9. Implementation still requires independent review. Native-assigned allocation cannot enter the consumer path without the separately coordinated journal amendment.
