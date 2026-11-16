# Composer development wire contract

This is the frozen byte-order contract for coordinated C++ producer and Rust consumer source work. It is gated by `RSDK_DEVELOPMENT_COMPOSER_V1` / `development-composer-v1`; ordinary production startup remains unselected. The existing v4 runtime/client/server/sequence/operation/request-digest envelope and its exact peer checks remain unchanged. Decoding a message never manufactures native authority. Existing readers without this extension refuse its unknown kinds.

All integers are big-endian. `Id` is the existing nonzero 16-byte opaque ID. `Facts` is the existing document Id, page Id, u16 page-order count and that many Ids, with unchanged validity/bounds. `Revision` is i32, constrained to >=0. `Units(N)` is u16 count followed by that many u16 units, count <=N; units are preserved exactly, without newline/Unicode normalization. Empty draft text is allowed. Conversion to a product string is separately fallible.

## Requests

The listed fields follow the existing request kind byte in exact order.

| Kind | Command | Fields |
|---|---|---|
| 12 | ProvisionComposer | Facts |
| 13 | ObserveComposer | Facts, visit Id, range Id |
| 14 | ClearComposer | Facts, visit Id, range Id, expected Revision, expected Units(4096) |
| 15 | AwaitComposerKey | Facts, visit Id, range Id, u8 key-code count (1..4), that many distinct i32 Qt key codes |

Configured key codes must be >0 and <0x01ffffff. This selects events to observe, not Submit semantics; no event is consumed or rewritten. The existing maximum 20-second selected-input budget applies; no additional timeout field is encoded. The issuer must continuously count every delivered KeyPress and InputMethod event within a visit, including unselected keys. This lets a consumer distinguish consecutive keyboard actions without inventing folio/OSK origin.

## Replies

Kinds 14 Provisioned, 15 Observed, 16 Cleared and 17 KeyObserved correspond only to request kinds 12, 13, 14 and 15 respectively. Each follows the existing response digest/kind with **u8 status, u8 attempts**.

- Status0: verified provision/observation, or Applied clear after projection verification plus exact full-job completion and scoped owner retirement. It does not claim render, save, sync or durability acknowledgement.
- Status1: Refused. No native edit was attempted.
- Status2: Unknown. A native request was attempted; no retry or cleared-draft inference follows.
- Attempts bit0: native Scene request attempted. Bit1: native edit submission attempted. Bit2: exact full-ID worker completion observed. Other bits are invalid. Bit1/2 require bit0. Observe/Key replies always forbid bit1.
- Status0 requires attempts7 for Provisioned/Cleared, attempts5 for Observed/KeyObserved. Status1 permits attempts0 or5 only. Status2 permits attempts1/3/5/7 for Provisioned/Cleared, attempts1/5 for Observed/KeyObserved.
- Status1/2 terminate immediately after these two bytes: no draft, event, IDs, revision or partially issued capability is serialized.

For status0, the exact remaining field order is:

| Reply kind | Fields |
|---|---|
| 14 Provisioned | actual Facts, visit Id, range Id, Revision |
| 15 Observed | actual Facts, visit Id, range Id, Revision, Units(4096) |
| 16 Cleared | actual Facts, visit Id, range Id, before Revision, after Revision |
| 17 KeyObserved | actual Facts, visit Id, range Id, Revision, Units(4096), event Id, u64 input serial, i32 key, u16 count, u32 modifiers, u32 native scan code, u32 native virtual key, u32 native modifiers, u8 autoRepeat, u8 spontaneous, Units(64) key text |

Input serial must be nonzero; visit overflow revokes/refuses instead of wrapping. Boolean bytes must be exactly0/1. Raw modifier and native-code words are copied facts, not source-origin proof. A KeyObserved key must be in its request's selected set; count0 is preserved and cannot qualify a product shortcut that requires count1. Replies must match the original facts and supplied visit/range (except Provisioned's newly minted IDs). Generic existing Rejected replies with unsupported/not-ready/busy codes0/3/4 remain valid pre-issuer refusals; they carry no composer payload.

## Producer and consumer boundary

Native visit/range/event IDs are opaque, process/runtime-bound capabilities or event identities, never addresses. A range is minted only after real private provision plus native-ID/full-projection readback, then exact worker completion/retirement. Every use rejoins the selected document/page and same continuous visit, worker and weak embedded-document incarnation. The existing endpoint operation/digest record owns no-replay behavior; no second conversation registry is introduced. The existing Buddy BindingReceipt/Ledger owns any eventual persistence. Restart/native-ID namespace qualification remains separate.

The revision and draft are a copied same-borrow observation, not a transferable edit/currentness lease. Clear must compare the actual live revision, anchors and exact native text at the private writer boundary. A post-event read may report genuine current content without claiming that Enter caused a specific edit or revision increment. Product shortcut grammar, trimming and semantic Submit selection remain outside this protocol and are not selected here.

The initial private implementation's whole-page 4096-unit admission is an early bounded profile. It must not be described as the v1 conversation-page limit; practical full-page streaming inspection remains an explicit integration task.
