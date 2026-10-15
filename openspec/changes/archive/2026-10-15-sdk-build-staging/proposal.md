# Change: Extract nonpublishing SDK build staging

## Why
The SDK is now incrementally merged on main. Its selected release candidate contains independently useful, already reviewed source/version staging logic that can land without the cumulative experimental SDK or a publisher.

## What Changes
Extract the exact selected stage_source.py from72136ed and its bounded source-admission/portable controls. Keep official existing-tag admission separate from explicitly unofficial candidate staging. Run only these Python/Git controls on Linux/Windows. No maintained bump engine, tagger, release API or native adapter is introduced.

## Impact
No runtime/Cargo/package/consumer/workflow-owner edits overlap capturePR12. Distribution/crate verification and full release composition retain their exact prior evidence and separate remaining qualification; this increment verifies staging admission only.
