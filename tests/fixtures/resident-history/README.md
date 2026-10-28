These two original owned protocol-v1 vectors were retained from this task's
`work/rem25-resident-local/wire-r2` corpus before protocol v2 was implemented.
They are not produced by relabeling the current encoder's output.

- `v1-insertion.bin`: original `03.bin`, SHA256
  `ba17908eb61a97542847ca09df1d6854b552296d4b77f65ee8de496b3a7ffea7`.
- `v1-insertion-observed.bin`: original `06.bin`, SHA256
  `d6c1ed7391a1a1c8fffb34d0d347669cc4bcaaf0b1d91db617966d443e2753f4`.

Both include the four-byte length prefix. The reply retains the original
request digest. All identities are synthetic fixture values; no device data,
credentials or native artifacts are included.

Original protocol-v2 vectors retained from the pre-upgrade
`work/rem25-resident-local/wire-selection-r1` corpus:

- `v2-insertion.bin`: original `03.bin`, SHA256
  `57ff674e05d62b08a8504835616b592a75f8ef4a247d5b160246fe6a90138a68`.
- `v2-insertion-observed.bin`: original `08.bin`, SHA256
  `b78d2e77ad8c84f681d6487aea19468fd9c8193af8ab0d29c23cb87b7796192f`.
- `v2-selection.bin`: original `05.bin`, SHA256
  `134f640a9c84ce6436082fd151a740b0b737c5db99205d2ada3762ddda8913b2`.

The insertion response retains its original request digest. These are copied
original bytes, not revision-3 output with relabeled version fields.

Original protocol-v3 vectors retained from the pre-upgrade
`work/rem25-resident-local/wire-capture-r2` corpus (owned synthetic pixels):

- `v3-capture-request.bin`: original `18.bin`, SHA256
  `c318472f83b0d960d1ab7d03d480475a7def22c5dbe1aa2b82717d0344c22601`.
- `v3-capture-response.bin`: original `19.bin`, SHA256
  `e3ad798001d3a4f32e8cc1a9d651c0bf26b12e282351a95713fb96f26c4facae`.
- `v3-insertion.bin`: original `03.bin`, SHA256
  `a281b9d592dfd1a301f0a3540b3a9ddf3103b3dd0ab9092ec77b5f8293ddca14`.

The capture response retains the original request digest and exceeds the ordinary
control-frame bound. These are copied original bytes, not relabeled v4 output.
