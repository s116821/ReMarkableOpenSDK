# SDK OpenSpec workflow

This repository owns the SDK's API, capability, adapter, native behavior, compatibility and release contracts. Canonical implemented specifications will live in `specs/`; active proposals live in `changes/`; completed deliveries move to `changes/archive/`. No canonical implemented specification exists at bootstrap.

1. Read the current public requirements and full available issue/comment chronology. Write proposal, design, tasks and requirement deltas before implementation.
2. Implement and update tasks against observed evidence. Keep speculative mechanism choices and hardware limitations explicit.
3. Verify code against each scenario, including failure/interruption cases, and obtain independent review. Coordinate exact consumer revisions and required CI.
4. Sync the accepted, implemented delta into canonical `specs/`, preserving unrelated requirements.
5. Archive only completed changes in the coordinated delivery. Never archive an unfinished investigation or native validation gate.

Validate using `openspec validate --all --strict --no-interactive`. Equivalent manual review remains supported. Structural validation is not implementation verification.

ReMarkableBuddiesDocs owns product behavior and references this SDK contract/version. Neither repository silently overrides or duplicates the other's contract. Current instructions supersede earlier ecosystem guidance that placed every specification in Docs.

Reusable SDK research, reference guides and tooling knowledge live under
[`docs/`](../docs/README.md). Genuine change-specific plans, design amendments and
implementation/verification checkpoints remain here in their standard change
structure. Link to their canonical bodies from the knowledge index; do not move
workflow artifacts solely because they are Markdown or archive unfinished gates.
