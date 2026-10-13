# Why
The release-tool branch has no SDK crate, so accepted automatic selection still reports historical source. Integrating the owner-reviewed immutable source enables real current-candidate compiler, consumer and source-stage checks without altering SDK behavior.

# Scope
Mechanically combine CIa79355e and owner00fac00 sources with source blobs preserved, union gitignore exclusions, then verify current-candidate selection and existing host/MSRV/ARM/consumer/package gates on that exact candidate. No SDK runtime/API edits or native/source shipping claims; retain unfinished release/advisory/publication/main gates.
