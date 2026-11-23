## 1. Implement the scalar-only typed diagnostic

- [x] 1.1 Add immutable exclusive selection and explicit historical result state.
- [x] 1.2 Capture and compare the borrow-local identity and complete scalar tuple.
- [x] 1.3 Document ownership, deadline, refusal and historical semantics.

## 2. Validate and close the source change

- [x] 2.1 Extend the owned Attachment and Delivery fixtures and record their results.
- [x] 2.2 Sync and archive this completed source change in the child PR.

## Validation

- Attachment and Delivery passed 8 and 6 owned fixture groups respectively on
  Qt 6.4.2 with ASan/UBSan and on retained ARM Qt 6.10.3 in QEMU. Both lanes
  compiled the production headers. The ARM lane used owned synthetic Scene
  storage and a real Qt document for the native layout read.
- The existing exact text-copy suite passed its 6 owned groups with ASan/UBSan.
- Live profile admission and tablet execution remain separate.
