# Design

The operator selects an existing SSH alias; a strict host-token grammar prevents options or shell fragments. SSH uses BatchMode, IdentitiesOnly, strict known-host verification, disabled forwarding and a connect timeout. A fixed command prints a narrow key/value record; no arbitrary remote command is accepted.

The Linux host bounds output to 16 KiB while reading, applies a 30-second outer deadline, suppresses remote diagnostics and kills/waits for its own SSH process on refusal. Parsing accepts only exact keys and formats. Runtime executable and Qt Core hashes are sampled twice and must agree; the xochitl PID must remain the same and its executable must be the expected path. These sampled checks are non-atomic, not lifetime or source authority.

Only RM1/ARMv7 with the observed framebuffer family can produce a report. Firmware/Qt fingerprints are observations, never compatibility allowlists. Storage records total, free and unreserved available blocks separately; 0 available does not imply 0 free. Framebuffer virtual size is allocation geometry, not logical page or input geometry.

Reports contain no credentials, SSH alias, serial number, document names/content or proprietary binary bytes. Native operations always remain unsupported and source_authority=false. Mock test reports remain distinct from actual tablet evidence. File output is atomic after full validation, so failure preserves prior evidence.

Tests cover missing/duplicate/unknown fields, identity changes, unsupported models, malformed hashes/numbers, reserved-space accounting, injected host tokens, output limits/deadline and existing-output preservation. Actual RM1 run plus a normal repeated observation verifies transport and read-only report behavior. Native page/source/capture qualification is a subsequent scope requiring adapter design, independent review and model-specific real-device evidence; it is not a dependency for this collector.
