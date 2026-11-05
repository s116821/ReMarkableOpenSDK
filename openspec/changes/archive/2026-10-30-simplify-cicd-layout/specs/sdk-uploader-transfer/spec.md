## REMOVED Requirements

### Requirement: Qualified uploader transfer binds independent provenance
**Reason**: The immutable upstream distributed Action is consumed directly; there is no project-built uploader artifact to transfer.
**Migration**: Keep trusted SDK candidate artifact IDs and digests; remove only uploader artifact outputs/downloads and their bespoke verification.

### Requirement: Changed transferred tool refuses before execution
**Reason**: Project-specific uploader transfer and modified-copy qualification are removed with the locally rebuilt tool.
**Migration**: Pin the maintained Action to the reviewed upstream commit and retain SDK-owned release identity and byte-verification checks.
