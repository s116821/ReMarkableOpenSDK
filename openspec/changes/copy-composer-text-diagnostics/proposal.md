# Change: Copy bounded composer text diagnostics

## Why
Typed scene delivery can attach document diagnostics, but cannot yet return the
exact committed text of an explicit range. A finite read adapter lets downstream
integration inspect real document content without manufacturing composer ownership.

## What Changes
- Add an optional copied range request to the existing Attachment/Delivery path.
- Copy at most 4,096 UTF-16 code units inside the admitted native document borrow,
  retaining native revision observations before and after the read.
- Preserve NUL, NBSP and native paragraph separators; refuse unsupported structure,
  composition, invalid bounds, split surrogate endpoints or unsettled publication.
- Add focused owned Qt controls after design acceptance, then synchronize the
  canonical spec and archive this same change after implementation acceptance.

## Impact
This publication is precode only. Integration-owner review is required before
implementation. Existing Attachment, Observer and typed owner retention remain
the lifetime and dispatch mechanisms. No new dispatcher, owner framework,
runtime selection, native activation, text mutation or transport endpoint.

The result is historical diagnostic data. Caller-supplied range, native identity
scalars and equal revisions do not grant owned range, active page/visit, semantic
Submit, edit lease, conditional clear or native owner retirement authority.
