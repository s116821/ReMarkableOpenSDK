# SDK executable release inputs

## Purpose
Keep executable package-check data visible to maintained release and CI filters while preserving documentation exclusions.

## Requirements

### Requirement: Executable release inputs remain visible to maintained classifiers
The unchanged dependency-license inventory SHALL live at a normal tool-input path, be consumed by package checks there, and remain application relevant without special documentation exceptions or a private classifier.

#### Scenario: Tool input
- **WHEN** the tool inventory changes
- **THEN** maintained CI and semantic path filtering recognize an application input and the package checker verifies the same original inventory bytes and current lock/notices.

#### Scenario: Guidance remains documentation
- **WHEN** only the former documentation location or named release README guidance changes
- **THEN** maintained path filtering treats it as documentation while unknown/source, mixed and rename inputs retain conservative admission.
