# ReMarkableOpenSDK

An independent, unofficial reMarkable hardware and native-runtime SDK, intended for ReMarkableBuddies and unrelated applications.

**Status: active research and contract design. No native adapter, supported firmware matrix, shipping library or page-creation capability is implemented here yet.**

The first change is [establish-native-platform-contract](openspec/changes/establish-native-platform-contract/proposal.md). It targets a capability-oriented boundary for page identity, evidence-bound capture, navigation and safe native page creation. RM2/ARMv7 is the first qualification target. Paper Pro/AArch64 is an adapter target whose native qualification remains later work; cross-builds do not imply hardware support.

SDK specifications and workflow live in this repository. [ReMarkableBuddiesDocs](https://github.com/s116821/ReMarkableBuddiesDocs) owns Buddy and Manager product/integration requirements and references exact SDK contract revisions. SDK definitions are not duplicated there.

Buddy consumes the SDK at build time. Manager selects a compatible Buddy release artifact, without separately installing an SDK runtime. The SDK must remain independently consumable and mockable.

XOVI is research prior art only and is excluded as a production dependency. Proprietary firmware binaries and reconstructed proprietary source must not be committed. Interface discovery is not proof of safe native operation.

Read [contributor guidance](CONTRIBUTING.md), the [workflow](openspec/README.md), and the active design before implementation.
