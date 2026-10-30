# Design

The release workflow uses `svenstaro/upload-release-action` at the existing exact
upstream revision, whose tree contains its distributed entrypoint. It no longer
rebuilds that Action using a project-selected dependency graph. Historical audits
and generated bundle identities remain historical evidence and are not credited
to the directly consumed distribution.

The Action receives the original release ID and explicit no-overwrite/draft
options. Project helpers still verify the original ID, remote tag and complete
published bytes before finalization. The uploader is no longer a build artifact;
only SDK candidate bytes cross the Build/Integration/Release custody boundary.

Keep build logic needed to generate a versioned package from a versionless source
manifest, test external consumers and verify the finite multi-target inventory.
Do not add a general CI framework. Ordinary job setup and orchestration stays in
the workflow. Existing package and source refusal tests are substantive SDK tests,
whereas re-executing upstream Action implementations is redundant maintenance.

The new directories have the same responsibilities with updated relative roots.
Generated candidate `release/` directories and Cargo target release profiles are
artifact formats, not repository directories, and remain unchanged. Historical
OpenSpec archives and source-accounting records keep their original paths/revisions.

After reviewed cleanup delivery, resync every open SDK PR with its owner at a safe
source-only point. Never change a running build's source or frozen native inputs.
Audit unique unmerged scope and actual blockers so old CI definitions cannot return
and already landed work is not repeated.
