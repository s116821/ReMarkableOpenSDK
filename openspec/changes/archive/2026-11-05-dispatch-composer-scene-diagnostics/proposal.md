# Change: Dispatch typed scene diagnostics

## Why
The accepted document attachment needs delivery through the real scene request; the generic worker callback has a different ABI.

## What Changes
- Add a distinct typed request0x6d6ce4 adapter using the reviewed resident document/worker owner slot and accepted scene attachment.
- Preserve one-use admission, original deadline, callback-copy lifetime, cancellation and unknown enqueue results.
- Retain native owners after any typed request attempt; the generic task's retirement proof cannot retire a different typed callback.
- Exercise actual adapter submission against owned typed request fakes on host and ARM.

## Impact
No native bootstrap/runtime selection, tablet execution, new binary research, native clear or draft authority. This is actual source wiring to the known native request. Root owns integration and runtime scheduling.
