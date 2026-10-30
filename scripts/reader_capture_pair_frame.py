"""Strict length-frame decoder for archived capture reader output.

A parsed frame validates bytes only. Live route, target/provider/process/recovery
and original clock brackets must be verified separately by the exact operator.
"""
from dataclasses import dataclass
import hashlib
import re

from reader_identity_capture_consumer import (
    Expected, Refused, SCALAR_CAP, PNG_CAP, canonical_id, require,
    validate_positive_bytes,
)

END = b"\nRMB_CAPTURE_PAIR_END\n"
FRAME_CAP = SCALAR_CAP + PNG_CAP + 256


@dataclass(frozen=True)
class ParsedPair:
    scalar: bytes
    image: bytes
    evidence: dict


def parse_frame(raw, expected, expected_boot):
    require(type(raw) is bytes and 0 < len(raw) <= FRAME_CAP, "frame-size-type")
    require(type(expected) is Expected, "frame-expected-type")
    expected.validate()
    require(canonical_id(expected_boot), "frame-expected-boot")
    end = raw.find(b"\n", 0, 256)
    require(end > 0, "frame-header")
    try:
        parts = raw[:end].decode("ascii", errors="strict").split(" ")
    except UnicodeError as error:
        raise Refused("frame-header-ascii") from error
    require(len(parts) == 7 and parts[0] == "RMB_CAPTURE_PAIR_V1", "frame-purpose")
    require(tuple(parts[1:5]) == (expected.nonce, expected.pid, expected.start, expected_boot), "frame-binding")
    require(all(re.fullmatch(r"[1-9][0-9]{0,6}", value) is not None for value in parts[5:]), "frame-length-format")
    scalar_size, image_size = map(int, parts[5:])
    require(scalar_size <= SCALAR_CAP and image_size <= PNG_CAP, "frame-length-quota")
    begin = end + 1
    limit = begin + scalar_size + image_size
    require(limit + len(END) == len(raw) and raw[limit:] == END, "frame-complete")
    scalar = raw[begin:begin + scalar_size]
    image = raw[begin + scalar_size:limit]
    evidence = validate_positive_bytes(scalar, image, expected)
    evidence = dict(evidence, frame_sha256=hashlib.sha256(raw).hexdigest(),
                    frame="RMB_CAPTURE_PAIR_V1", boot_id=expected_boot)
    # Never upgrade the byte consumer's unverified external admission here.
    require(evidence["external_admission"] == "unverified", "frame-authority")
    return ParsedPair(scalar, image, evidence)
