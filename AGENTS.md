# SDK contribution instructions

Read README.md, CONTRIBUTING.md and openspec/README.md. SDK public API, native behavior, device/firmware adapters, compatibility and release specifications belong here. Buddy product, conversation, storage and Manager integration specifications belong in ReMarkableBuddiesDocs. Reference exact cross-repository contract revisions; do not duplicate them.

Use the complete OpenSpec proposal/design/tasks/deltas, implementation, verification, canonical sync and archive lifecycle. Do not archive unfinished research or claim a plan is a shipping feature. Review full current issue descriptions and timestamped comments when available; public requirements must suffice without private tools.

Keep capability queries independent of product logic. Unknown firmware/model combinations fail closed for operations requiring qualification. Keep mocks explicit and distinguish host, simulator, live-model and native evidence. No XOVI production dependency, hard-coded hook addresses as public API, autonomous menu automation or per-conversation UI restart workaround.

Preserve source documents, unknown metadata and account state. No tablet access follows from cloning, compiling or simulating. Coordinate one tablet owner and give advance notice before changes. Do not incidentally upgrade firmware, pair or enable personal sync. Keep proprietary firmware and secrets out of Git and published artifacts.

Use linked SDK and consumer PRs for cross-boundary changes, with exact revisions and merge order. PR bodies start with `# Summary` and concise bullets; detailed evidence and bot responses belong in comments. Independent review and required CI precede coordinated squash merge. Do not change repository settings or release the ecosystem's 1.0 from this work.
