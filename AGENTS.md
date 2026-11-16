# Agent contribution entry

Read [README.md](README.md) and the applicable specifications and proposals in
[openspec/](openspec/) before changing SDK behavior. SDK specifications and their
change lifecycle remain in this repository.

## Development validation

Follow the shared [development validation policy](https://github.com/s116821/RemarkableBuddiesDocs/blob/main/CONTRIBUTING.md#development-validation).
Build and exercise the changed SDK behavior through the relevant end-to-end workflow
on the authorized development tablet when available. Use local emulation or simulation
only while another operator uses the tablet or when you lack authorized hardware. Keep
automated component and regression suites in existing CI. Source review and formatting
may proceed locally.

## Development hygiene

Before building, collecting test or release outputs, handing off, or finishing
work, read and apply the shared
[development-hygiene skill](https://github.com/s116821/RemarkableBuddiesDocs/blob/main/.agents/skills/development-hygiene/SKILL.md).
Its ownership, cleanup, evidence-preservation and retention-exception rules apply
to SDK work by every agent on local and remote development machines. Use that
shared policy, including its manual workflow, as the hygiene reference.
