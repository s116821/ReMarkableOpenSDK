## 1. Protocol and observation contract
- [x] 1.1 Define a feature-gated bounded canonical frame schema and historical identity/order facts without invented revision/input epochs.
- [x] 1.2 Implement client handshake/request correlation, monotonic request sequences and sticky indeterminate mutation loss with no automatic replay.
- [ ] 1.3 Verify frame/schema faults, fragmentation, reconnect/stale replies, sequence exhaustion and actual Rust/Qt endpoint interoperability; preserve dependency-free defaults and package containment.
  - [x] 1.3a Host protocol suite: 13 integration tests plus sequence-exhaustion unit regression; default/all-feature builds, both ordinary consumer toolchains and package checks pass.
  - [ ] 1.3b Actual Rust/Qt local endpoint interoperability, peer authentication and native-side stale-request rejection.
- [ ] 1.4 Independently review the final exact implementation and record source/build evidence separately from native qualification.
  - [x] 1.4a Independent source review accepts the protocol foundation after capacity and sticky-ambiguity fixes; [checkpoint](verification.md). Final endpoint/native/consumer review remains open.

## 2. Resident native executor
- [ ] 2.1 Implement the owner-checked asynchronous Unix endpoint and exact runtime/profile handshake.
- [ ] 2.2 Integrate the measured Reader ownership observation and execution-side insertion predicate with unchanged native Task bookkeeping.
- [ ] 2.3 Implement separately verified physical callback completion and bounded owner reclamation, including canceled/late/nested delivery and repeated operations in one process.
- [ ] 2.4 Integrate read-only reconciliation and fresh conditional adjacent target acquisition; retain unknown outcomes without mutation replay.

## 3. Consumer and delivery
- [ ] 3.1 Pin the SDK in the existing Buddy/Docs candidates and integrate bindings, journal, writer/capture and Supervisor recovery.
- [ ] 3.2 Complete disposable PDF/notebook middle/end, reuse/no-duplicate, preservation, interruption, write/reopen and blank-start/Writer acceptance with appropriate simulation and exact native evidence.
- [ ] 3.3 Publish only qualified profile/capability results, pass exact-source CI/review, sync implemented specs and archive the completed coordinated change.
