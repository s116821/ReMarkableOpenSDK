# Design

Extend the current qualifier's output with provenance_sha256 after writing the qualified record. Expose this as a Checks job output along with the exact artifact ID returned by upload-artifact. Upload the already-qualified directory only after the four existing loopback controls pass, using a source/run-attempt name and seven-day retention consistent with SDK artifacts.

Extend verify(directory, expected_bundle) with an optional expected_provenance argument and corresponding CLI input. When supplied, hash the regular provenance file and compare to that externally trusted value BEFORE parsing its file inventory. Existing local same-job verification keeps its behavior. Integration always supplies the expected-provenance input from Checks, downloads by exact ID and retains the existing selected bundle pin. All directory files then remain subject to the existing complete inventory/recipe/audit verification. No copied record is treated as its own authority across jobs.

Three integration controls use actual downloaded output with explicit required fixture path/digest. The positive control verifies it; owned copies prove jointly changed harmless action metadata/provenance and altered JS payload refuse. No modified tool is executed. Only Checks builds/audits the uploader and runs the four loopback behaviors. Existing Integration verifies this transfer without Node, npm, rebuilding or credentialed execution; its retained SDK consumer/tamper checks remain unchanged. Application admission, direct predecessor success and cancellation guards stay intact.

This closes qualified tool transport only. Real remote original-ID/full-byte publication and final Release composition remain separate, with no implicit production activation.
