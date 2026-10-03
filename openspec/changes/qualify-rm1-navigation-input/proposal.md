# RM1 navigation input qualification

REM-49, SCRAPPY-DOO. Follow the accepted shared logical navigation plan at SDK
fb68f56456ce753c905277d9a9208f651747c615; do not duplicate its navigation API.
Native opening is deferred. RM1 gesture mapping needs actual RM1 input/orientation
and source/target evidence independently of page creation.

First implement a small read-only Linux input geometry probe. It reports fixed
RM1 model/touch identity and axis bounds; no event reads, device grabs, writes or
native operations. This supplies missing kernel-axis facts without installing
another framework. Next stages need the shared API implementation, independent
probe/source review, an own RM1 test fixture and per-orientation hardware proof.
No historical collector/release scope is changed or archived by this work.
