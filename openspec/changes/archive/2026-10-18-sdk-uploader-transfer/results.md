# Verification

Precode 33a6b18aeee55265a76869a61a5821904b24959b precedes implementation and is based on actual merged main 8de71d826e2c6782eeeca83b66b00ca11003158d.

- Existing boundary diagnostic: on an owned copy of the actual qualified uploader, changing harmless action metadata and the corresponding copied inventory hash passes the existing local inventory verifier while preserving the selected JS bundle. An independently captured original provenance digest refuses that joint mutation. No modified tool was executed or original evidence altered.
- Three focused controls against actual qualified output pass in 0.023s: valid trusted identity, joint metadata/provenance refusal, and payload mutation refusal with an unchanged trusted record. The original API lacked the new expected-provenance argument (three explicit initial TypeErrors); the new API and CLI both verify the actual qualified tool successfully.
- Checks now emits provenance_sha256, retains the already-tested directory and exposes the immutable artifact ID plus trusted provenance digest. Existing Integration downloads that exact ID and requires the trusted digest before parsing the record and verifying complete files, recipe, audit and selected bundle. Hosted emission/transfer is separately confirmed below.
- Local ten compact workflow controls pass, including exact SDK artifact/receipt assertions and new uploader artifact/provenance mapping assertions. The original fixture selected the first Integration command and failed after adding the tool-verification step; selecting the actual SDK consumer command preserves all prior receipt/source assertions. No required gate was weakened.
- CLI trusted verification, actionlint, strict specifications (20 passed, zero failed), and diff checks pass. No local tool rebuild, repeated uploader loopback suite or unchanged SDK matrix was run. The selected recipes, four loopback controls, SDK candidate build/probe, runtime/API/manifests/locks and native research remain unchanged.

## Hosted source and independent review

- Exact source f9b205708f5efacf604f2b5b466378b657c83ad8 has all five required jobs successful in run 39108045195. Direct Linux log 117618066710 confirms strict 20/0, selected source qualification and all four loopback controls, emitted provenance SHA 43dd6225177b0146a5ed60625657a4a2db03cd83c5d82ad3b71eab6c1bb30eaf and immutable tool artifact ID 12129675822 (330668 bytes).
- Tool artifact upload and download SHA256 both equal 934ea73a231785cd4395f4bd29217acee83f742ad34898cfc6a0700d9d71cf82. Integration log 117618730374 confirms exact ID download, trusted provenance value from Checks, successful explicit CLI verification and all three actual transferred-tool controls passing in 0.007s. No tool rebuild or repeated loopback suite occurs in Integration.
- Build 117618492334 and Integration identify temporary PR merge ab081280381dd9551cc8403d21f9b87c62cea302, candidate v0.1.0, SDK artifact ID 12129103159 and matching uploaded/downloaded SHA256 45dcf355a06ff32830a12fb5f2afa1f59e699597b99efd3f0697e1ca259836b7. The retained SDK consumer actually executes at that tested source and all four SDK tamper controls pass in 0.064s.
- SDK receipt SHA256 f0ff7ebf3429628142ef82f7058083fc57b256f7324f14402e8895d027615712; manifest SHA256 f5e774dd26c78fc78a159cff2f81423491ca698f316ed468d785a266298539c3.
- Full independent source review 5561964639 at exact f9b2057 was directly read. It reports no blocking finding, independently preserves all 189 unrelated baseline entries and accepts only this scope's canonical sync/archive. Remote original-ID/full-byte publication, final Release composition and native capabilities remain separate.

## Lifecycle handoff

Only this five-file change is archived. Proposal, design and delta remain byte-exact to accepted source; canonical text preserves the exact two requirements and three scenarios. All other accepted entries, including implementation, workflow, fixtures and unrelated active navigation work, remain unchanged. Strict validation, diff and preservation checks are run for this handoff; no unchanged local matrix is repeated.

Final lifecycle review/current-head checks, normal squash and actual-main proof remain pending at author handoff. This closes only qualified tool transport; no final Release job, production credentials, official tag/publication, new pipeline job, Buddy pin or native qualification is claimed.
