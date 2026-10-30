# First fixed-fixture navigation/join attempt

October 22, 2026: the separately compiled navigation/join mode accepted one
request on RM2, then returned `navigation-ambiguity` before calling the stock
navigator. No navigation, controller preparation, selected-page observation or
source comparison ran. Stock restoration and all eighteen fixture-file hashes
passed. This records a discovery limitation, not a failed stock open operation.

| Input / observation | Value |
| --- | --- |
| SDK source | `630cf159e28c5c0e2fbc15e993fd99ba3125220a` |
| Startup mode | `READER_SELECTED_OPEN_FIXTURE_JOIN`, frozen existing fixture, page index 4 |
| Payload SHA-256 | `2956172b6580cdc2e2d7762f157f720643446ba612be90232e7a4bcb99dd2b26` |
| Attempt | `4e941d31481944cd84224756a1767948`, consumed |
| Candidate PID / start | `6887` / `123528825` |
| Diagnostic | `navigate-creation-source-join` |
| Wire validation | Passed exact nonce/process, schema, file and time binding |
| Stage / refusal | `observation-refused` / `navigation-ambiguity` |
| Navigation attempted / dispatched | Both false |
| Selected observation / source join / ready | All false; no IDs returned |
| Entry begin / end | `4414` / `4432` ms, 18 ms whole call |
| Native / render / durable authority | All false |

The exact source emits this refusal on finding eligible `windowNavigator`
properties with different guarded receiver pointers. That establishes
nonunique receiver identity within the inspected eligible census. It does not
identify their owning views, prove different underlying routers, or establish
which receiver belongs to the active stock view. This early-refusal census is
not a complete inventory. A narrower source-backed selector or metadata-only
ownership diagnostic is needed; choosing the first receiver is unsupported by
this result. The separate [REA forwarding findings](fixture-navigation-source-join.md)
and earlier positive selected-page observation remain distinct evidence.

Host Qt6.4.2 and ARM Qt6.10.3/QEMU passed all three owned suites before the run.
Independent reconstruction reproduced the payload and all twenty packet files
plus the arm script. Sixty-three offline operator checks passed after fixing
inconsistent-success acceptance; the original operator and rejected vectors
remain private. The resealed operator SHA-256 is
`1af1c566bfcee44ff1c8f7115258db485b4399f726d22eaf0f312d2d1017d03b`.
The actor retained its original 360-second deadline, ninety-second setup,
twenty-second cooperative operation cap and independent recovery reserve.
One atomic request and one stop request were published, with no retry.

The actor recorded `primary_exit=0`; stock xochitl was restored as PID `7734`,
start `123531466`, matching the original executable. Fresh checks found all
three services active/running, no jobs or restart counts, no trial mappings or
runtime shadows. All eighteen fixture files were byte-identical before,
archived and after the run; both mutable metadata files were unchanged too.
The collector's final inherited assertion still expected the previous stock
PID and failed after saving these records. A separate saved-data verification
checked the actual new PID, backups, hashes and archive without repeating the
test or overwriting the failure.

The 328704-byte runtime archive SHA-256 is
`baef7a5335f38b2d913386d51fe22366b0ca237367a3fed8ebea8cc7ca22429e`.
The fixture archive retained SHA-256
`3f22dd7853c44fd54dc11ac9224d3e27639069860193b4345bfa9aaf7ec255cf`.
Only the exact owned RAM directory was removed after verifying archive identity,
the restoration marker, absent actor/candidate processes, inactive actor unit,
packet hashes and flat file-only contents.

Source basis: exact committed original interoperability code, owned Qt runs,
independent offline review/reconstruction, private bound native receipt,
actor/archive records and fresh stock/fixture readback. No rendered-page,
creation, worker serialization, lifecycle repair or durability claim follows.
