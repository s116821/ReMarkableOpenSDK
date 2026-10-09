# Bounded observational capability report

## Scope and API, October 8

Implement only task 3.1's absent observational Rust query. Existing identity/evidence and task 3.2 model behavior remain unchanged. Main reports no current Buddy integration requirement; this is the independent SDK contract already proposed in design.md. No native qualification, release manifest generator or consumer integration is added.

Add `Platform::capabilities(&self) -> CapabilityReport`, with a conservative default. Report getters expose `contract_revision()` (`remarkable-open-sdk/semantic-contract/1`), `profile()` (`DeviceProfile::UnknownNative` or `SyntheticModel`), `profile_revision()` (None for unknown; `remarkable-open-sdk/synthetic-model/1` for the model) and `status(key)`.

Keys are `observe_page`, `capture`, `navigate`, `create_after/client-selected`, `create_after/native-assigned`, `reconcile_creation`, and `acquire_after_creation`. Status is only `Unsupported(UnsupportedReason)` or `SyntheticOnly`. There is deliberately no native-supported variant while no qualified adapter exists. Unknown keys return CapabilityNotImplemented. These revisions describe semantic/profile schemas, not the package/app version or Git tag. No release/version machinery changes.

The native default reports unknown profile and no profile revision. UnqualifiedPlatform reports known operations unsupported with NoNativeAdapter; a third-party Platform using the default reports CapabilityNotImplemented. No fingerprint/version input or constructor can promote support. The model reports observation and historical reconciliation synthetic; creation/navigation/handoff are synthetic only while guard enforcement is enabled, otherwise UnqualifiedMechanism. Capture remains CapabilityNotImplemented because MockPlatform does not implement acquisition. Both allocation modes describe modeled behavior; native-assigned operation still requires its fixture-provided ID and does not promise durable correlation.

Reports are immutable snapshots of implementation availability, not current page identity, readiness, operation admission or authority. A report never creates/reconstructs a guard, dispatches an operation, consumes a request/fault/native ID, or changes historical evidence. Call-time validations continue to decide each outcome. No provider/model/firmware identity is invented for an unknown profile.

## Verification and delivery

- Test unknown native/default profiles, unsupported known and unknown keys.
- Test synthetic operation distinctions, absent capture and both allocation modes; repeated queries leave page state, injected execution fault and assigned ID available to the real modeled operation.
- Test a retained report cannot override later guard refusal, and synthetic discovery cannot enable UnqualifiedPlatform.
- Run focused Rust tests, format and strict Clippy; validate the additive SDK specification. Obtain Main semantic compatibility review on exact source/spec freeze. No native build/device test.

Keep task 3.1 partial: native profile/qualification/release manifest remains unfinished. Keep the overall change unarchived.
