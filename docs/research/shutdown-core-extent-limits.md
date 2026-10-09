# 0404: intended extent versus lost backing storage

October 8, 2026. Question: can the saved artifact distinguish an access outside the intended image allocation from an access inside formerly mapped storage that is no longer readable? **Not from the currently identified image metadata and mapping evidence.** Neither alternative is established, and they are not an exhaustive cause classification (for example, pointer/stride corruption remains possible).

This follows the [QImage ownership](shutdown-0404-prior-art.md) and [entrypoint comparison](shutdown-entrypoint-comparison.md) findings without repeating their source analysis or rebuilding/rerunning anything. The capability-query review freeze remains `6f46ef4`; native diagnostic freeze remains `4d98ebd`.

## Saved artifact inventory

Read-only standard-library ELF parsing rechecked private core SHA256 `37be0ff8b52434c36a97adff39aa381ab490864fd59df3c700ecd0ea89d4c34e`, 294376 bytes. It contains 255 program headers, including 252 PT_LOAD segments totaling 239702 captured bytes. All those segments have equal file and memory sizes; there is no zero-file-size segment preserving an additional omitted extent. Neither row pointer `0x6d30ef08` nor fault address `0x6d30f000` belongs to any captured segment or NT_FILE range. NT_FILE contains 301 ranges; it is not a historical anonymous-allocation ledger.

Fourteen PRSTATUS register sets are present, with all fourteen saved stack pointers covered by captured bytes. Among the fault thread's r0–r12, sp, lr and pc values, only sp points into a captured segment. This is a coverage observation, not proof that no useful indirect pointer could exist in a saved stack. No identified QImage/QImageData descriptor address, allocation base, actual bytesPerLine, height or allocation byte count has been recovered from these observations. Matching executable/Qt metadata resolves the previously recorded partial call path; it does not supply these runtime values.

The two Memfault-specific notes were also decoded locally as bounded CBOR without publishing their raw contents. Metadata records the `threads` capture strategy with `max_thread_size_kib=32`, and attached application logs. The debug note has schema-version/capture-logs fields and 129 capture log entries; a bounded search found no fault-region address, image/allocation or unmap entry. This does not turn debug logs into a complete mapping history or exclude unrelated information in captured stacks. Device identity, command metadata, application-log bodies and raw memory remain private.

## Exact missing distinction

The existing row/pixel interpretation shows a 16-byte read spanning pixels 60–63 within clipped width 1404. That excludes only the simple horizontal-right-edge explanation. Clipped width is not the backing allocation length. Establishing intended containment requires the actual image base, stride, height/row validity and allocation extent, tied to this particular source image. Computing a hypothetical base from width times four would assume the missing stride and would not establish ownership or validity.

Establishing *previously mapped* storage additionally requires an earlier mapping/allocation record for the same process and object, plus lifetime ordering. MAPERR identifies the crash-time invalid access; absent capture cannot supply the preceding map/free/unmap history. Even an ordinary mapping range is not necessarily the allocator's object boundary.

Permitted evidence that could resolve this is an already-retained exact-process image descriptor/allocation record or exact-build symbol/source information sufficient to identify such a descriptor in the saved stack, together with an existing same-process pre-fault mapping/lifetime trace. No such matching record has been identified in this bounded investigation. A read of today's stock process cannot reconstruct the old process's allocation. No new stock restart, manual action, changed capture configuration, instrumented payload or speculative fix is proposed.

## Existing early stock stop correction

Main's correction is confirmed in saved `main-reboot-logs.json`: candidate17423 crashed at 15:26:37; replacement stock17544 started at 15:26:38, logged display/update waiting and stopped threads at 15:26:39, and deactivated successfully at 15:26:40 (journal clock). This is an existing negative elapsed-time example. It lacks a matching engine/first-render/image-allocation cohort and cannot establish causation or a lifetime guarantee. Another generic stock stop would duplicate this limited evidence. Failed 0404 restoration remains failed.

Source basis: current private ELF headers/notes and coverage calculations, bounded local CBOR decoding, prior sanitized fault interpretation, and direct readback of the saved journal excerpt. The distinction remains unresolved; no fresh GDB invocation, tool patch, compilation, native run or private-data upload was performed.

## Bounded saved-stack descriptor path

Later October 8, the remaining possible indirect-stack route was checked against available exact-build anchors. Existing `astra-core-render-finding.json`, `astra-core-check-result.json` and independently retained `main-core-unwind-review.json` identify the pixel-read operation and the partial five-frame path. They do not identify a QImage/QImageData object address, a descriptor-bearing stack slot or a typed caller parameter. The first four frames remain unnamed xochitl code; the fifth is the Qt Quick renderer. No retained full proprietary disassembly or typed-locals result was found among this incident packet's debugger/core outputs.

Read-only section/symbol parsing reverified the exact saved xochitl hash `071d85beef3ef2d4cc0e11002140b27b82a2cc04a2ed740a5669f591069b77df`. The image contains `.ARM.exidx`/`.ARM.extab` and a 3497-entry `.dynsym`, but no `.symtab`, debug/type sections or separate-debug-file-link section. No defined dynamic symbol with a nonzero extent covers any of the four xochitl frame PCs (`0x66d004`, `0x68447c`, `0x65ec34`, `0x667d44`). The immediate private evidence directory contains the executable, with no matching xochitl symbol/debug sidecar identified. This is a bounded local inventory, not a claim that no symbol package exists elsewhere.

Unwind metadata explains why partial call recovery is possible; it does not identify a local variable's type or image ownership. The established fault-time row pointer cannot be substituted for an unestablished descriptor pointer. Public Qt type declarations/version correspondence alone cannot assign a vendor caller's stack slot to that type or recover optimized argument provenance.

The missing anchor is exact-build function/type/location information tying a particular captured slot or saved pointer to the actual image argument/descriptor, with fields corroborated against the faulting call. A matching symbol/source package with usable variable locations, or an already-retained typed debugger observation from this exact process, could supply that anchor; neither is currently identified. Even recovering the descriptor would still leave the separate prior-allocation-history requirement above. No arbitrary stack-value scan or inferred width/height reconstruction was performed. This permitted artifact path is concluded without a repair or new experiment; no specific human-only evidence acquisition has been identified.

## Additional bounded static argument trace

A subsequent, separately requested read-only static pass recovered an argument anchor without DWARF, superseding the earlier absence of an identified descriptor pointer. Exact executable ARM unwind entries bound the fault routine to `[0x66ce78, 0x66d144)` and immediate caller to `[0x6841b0, 0x684c00)`. Minimal instruction ranges were inspected privately using the already-installed unmodified ARM objdump; misleading nearest-export symbol labels were not treated as function names.

The fault routine retains its incoming image argument in r5 and passes it to dynamically named `QImage::height()`, `width()`, `format()` and `constScanLine(int)` imports. Fault-time r5 identifies `0x1bbb908`. The immediate caller obtains that argument from a field at offset 100 from its r5 owner pointer before the call. The callee's actual stack adjustment/save layout locates saved caller r5 at fault-sp+24 (`0x1bbb8b0`) and the saved return at fault-sp+52; the latter equals the known `0x68447c` return PC. These are instruction/frame-correlated values, not a search for plausible image dimensions.

The captured local at fault-sp holds an exclusive clipped height of 1872, derived by taking the lesser of rectangle-bottom-plus-one and `QImage::height()`. Row 16 is within that bound; the already-established pixels60–63 are within clipped width1404. This narrows away a simple vertical-bound overrun as well as the previously excluded horizontal-edge explanation, assuming the recorded scalar state and instruction path. It does not validate the image descriptor or backing allocation; clipped bounds are not allocation dimensions.

The descriptor's first word at `0x1bbb908` and the caller's field at `0x1bbb8b0+100` are both outside captured segments. Provenance therefore stops at the corroborated QImage argument and getter-derived clipping bounds. Actual backing base, bytesPerLine, allocation extent and prior release history are still unrecovered. No descriptor layout was guessed, stack scanned, lifetime fix inferred or new experiment proposed.

Private scalar receipt: `astra-core-immediate-chain-finding.json`, retained alongside the core, records the exact intervals, stack/argument anchors and absent coverage. Raw instructions and proprietary bytes remain private. The capability review freeze and native candidate remain unchanged.
