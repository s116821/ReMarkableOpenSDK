## 1. Review

- [x] Independently review wire tags, closed values, reuse of the original
  lease and owner, artifact selection and compatibility against delivered SDK
  main. Reconcile any drift before implementation.

## 2. Implementation after acceptance

- [x] Add paired development-gated codecs and exact request/reply correlation
  while preserving ordinary encodings and limits.
- [x] Integrate the accepted source-only Reader sampler into the existing
  read-only dispatcher. Keep the opt-in artifact separate from protected
  candidates and ordinary release inputs.

## 3. Evidence and closeout

- [x] Check C++/Rust golden bytes, malformed sizes and enum values, feature-off
  behavior, ordinary reply compatibility, cross-kind rejection, stale session
  scope and cancellation before completion.
- [x] Reuse the focused Qt join fixture for readiness, wrong Reader identity,
  focus/page drift, reentry and deadline refusal. Check transport publication
  against a retired or expired lease in the owned endpoint fixture.
- [x] Record the Main-operated runtime result or the concrete admission blocker
  separately from fixture results. Sync and archive this change with its
  eventual implementation PR.
