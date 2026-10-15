# Design

Base actual main d9be3eb3c8cd997fc33caf817a8ac2ac3f08723b after SDK16. Move docs/reference/dependency-license-inventory.json to tools/dependency-license-inventory.json with exact mode/type/blob; only its checker path changes. No inventory data or license/lock changes. Existing immutable historical links remain valid.

Replace current docs extglob exceptions in sdk-paths.yml with the same supported broad docs/** ignore used by the selected PSR config. Add the existing selected release/README.md named guidance exclusion so the two maintained configurations agree. Unknown paths and nested source Markdown remain application relevant. Reuse the actual pinned dorny bundle controls; prove old docs path is docs-only, new tool input is application relevant, named release guidance is docs-only and mixed/rename behavior remains. No custom matcher.

Run only affected package controls and path fixtures plus strict specification checks. Diagnose both paths through actual upstream PSR public parser in a disposable local repository using the existing selected configuration. This validates path relevance, not initial-release policy or source title enforcement. No production tag/version calculation, publisher, SDK/native matrix or new dependency is introduced. Preserve every unrelated main entry and coordinate this small input move before semantic metadata activation.
