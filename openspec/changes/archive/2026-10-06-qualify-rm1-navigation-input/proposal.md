# RM1 read-only navigation input metadata

REM-49, SCRAPPY-DOO. Follow the accepted shared logical navigation plan at SDK
fb68f56456ce753c905277d9a9208f651747c615; do not duplicate its navigation API.
Native opening is deferred. RM1 gesture mapping needs actual RM1 input/orientation
and source/target evidence independently of page creation.

This delivery implements a small read-only Linux input geometry probe. It reports fixed
RM1 model/touch identity and axis bounds; no event reads, device grabs, writes or
native operations. This supplies missing kernel-axis facts without installing
another framework. Its acceptance is bounded metadata/refusal behavior, scoped
source review, host/ARM-emulator verification and honestly attributed own-device
observations. It does not qualify navigation or a production-distributed binary.

Gesture mapping, native source/target continuity and completion remain unfinished
in the separate active `qualify-rm1-logical-navigation` change. That change needs
the shared native capability, independent real-profile qualification, an own RM1
fixture and per-orientation hardware proof. This scope split adds no navigation
implementation and does not archive any unfinished work. No historical collector
or release scope changes.
