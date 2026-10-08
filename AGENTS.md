# SDK contribution instructions

Read README.md, CONTRIBUTING.md and openspec/README.md. SDK public API, native behavior, device/firmware adapters, compatibility and release specifications belong here. Buddy product, conversation, storage and Manager integration specifications belong in ReMarkableBuddiesDocs. Reference exact cross-repository contract revisions; do not duplicate them.

Use the complete OpenSpec proposal/design/tasks/deltas, implementation, verification, canonical sync and archive lifecycle. Do not archive unfinished research or claim a plan is a shipping feature. Review full current issue descriptions and timestamped comments when available; public requirements must suffice without private tools.

Keep capability queries independent of product logic. Unknown firmware/model combinations fail closed for operations requiring qualification. Keep mocks explicit and distinguish host, simulator, live-model and native evidence. Prefer robust direct/native mechanisms; the October 1 decision permits supervised, session-scoped lazy XOVI as a candidate where needed, not an already selected or qualified dependency. Require stock cold boot, bounded activation/health/rollback, recovery independent of the modified UI including supervisor failure, and fresh source validation after restart. Buddy owns trigger/supervisor orchestration; SDK owns runtime capability and adapter contracts. No hard-coded hook addresses as public API, autonomous menu automation, persistent boot injection or per-conversation UI restart workaround. Read the active runtime-mechanism comparison before changing lifecycle design.

Preserve source documents, unknown metadata and account state. No tablet access follows from cloning, compiling or simulating. Coordinate one tablet owner and give advance notice before changes. Do not incidentally upgrade firmware, pair or enable personal sync. Keep proprietary firmware and secrets out of Git and published artifacts.

Use linked SDK and consumer PRs for cross-boundary changes, with exact revisions and merge order. PR bodies start with `# Summary` and concise bullets; detailed evidence and bot responses belong in comments. Independent review and required CI precede coordinated squash merge. Do not change repository settings or release the ecosystem's 1.0 from this work.

## Documentation layout

Put reusable documentation in `docs/`: research and prior art, technical findings,
reference guides, tool usage and experiment lessons. Keep genuine OpenSpec
change-specific proposals, designs, tasks, requirement deltas and delivery
checkpoints in the standard `openspec/` structure; link to reusable knowledge
rather than duplicate its canonical body. Root README/CONTRIBUTING/AGENTS,
license/security files and conventional tool-discovery files (GitHub templates,
skills) are exceptions. Test assets remain fixtures, not documentation by default.
Preserve historical evidence and distinguish proposals, host/simulator checks and
native qualification when reorganizing; do not sweep unrelated archives.
