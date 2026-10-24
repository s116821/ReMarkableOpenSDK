"""Validate original research capture bytes; never grants live device authority.

The caller must separately collect both files under fresh process/request/restore
and private-file identity brackets. This module validates the archived byte pair,
not those external observations. It neither activates a candidate nor retries it.
"""
from dataclasses import dataclass
import hashlib
import io
import json
import math
import re
import struct
import zlib

SCALAR_CAP = 4096
PNG_CAP = 1048576
PIXEL_CAP = 2628288
STORAGE_CAP = 10513152
ID = re.compile(r"[0-9a-f]{8}(?:-[0-9a-f]{4}){3}-[0-9a-f]{12}\Z")
HEX = re.compile(r"[0-9a-f]{64}\Z")
DECIMAL = re.compile(r"[1-9][0-9]{0,19}\Z")
TOP_KEYS = frozenset("""schema diagnostic nonce pid start stage refusal accepted
attempted captured unknown navigation_attempted navigation_dispatched
navigation_scope expected before after sample_begin_ms sample_end_ms
capture_begin_ms capture_end_ms publication_ms operation_end_ms setup_limit_ms
image_committed image_linked image width height storage_bytes dpr encoded_bytes
output_bytes sha256 acquisition coordinates identity_semantics
content_correspondence physical_freshness native_authority render_authority
durable_authority atomic_authority""".split())
OBS_KEYS = frozenset("""observed map_coherent creation_join ready document_id page_id
index count status page_status loading page_error document_error awaiting_tiles
pending_tiles working refusal""".split())
EXPECTED_KEYS = frozenset(("document_id", "page_id", "index", "count"))
BOOL_KEYS = frozenset("""accepted attempted captured unknown navigation_attempted
navigation_dispatched image_committed image_linked native_authority
render_authority durable_authority atomic_authority""".split())
INT_KEYS = frozenset("""schema sample_begin_ms sample_end_ms capture_begin_ms
capture_end_ms publication_ms operation_end_ms setup_limit_ms width height
storage_bytes encoded_bytes output_bytes""".split())
OBS_BOOL_KEYS = frozenset("""observed map_coherent creation_join ready loading
page_error document_error awaiting_tiles pending_tiles working""".split())


class Refused(ValueError):
    """The original bytes cannot support a positive capture receipt."""


def require(condition, reason):
    if not condition:
        raise Refused(reason)


def canonical_id(value):
    return type(value) is str and ID.fullmatch(value) is not None and value != "00000000-0000-0000-0000-000000000000"


@dataclass(frozen=True)
class Expected:
    nonce: str
    pid: str
    start: str
    document_id: str
    page_id: str
    index: int
    count: int

    def validate(self):
        require(type(self.nonce) is str and re.fullmatch(r"[0-9a-f]{32}", self.nonce) is not None, "expected-nonce")
        require(all(type(v) is str and DECIMAL.fullmatch(v) for v in (self.pid, self.start)), "expected-incarnation")
        require(1 < int(self.pid) <= 2**31 - 1 and int(self.start) <= 2**64 - 1, "expected-incarnation-range")
        require(canonical_id(self.document_id) and canonical_id(self.page_id), "expected-identity")
        require(type(self.index) is int and type(self.count) is int and 1 <= self.count <= 16 and 0 <= self.index < self.count, "expected-index-count")


def closed_object(value, keys, reason):
    require(type(value) is dict and value.keys() == keys, reason)


def unique_object(pairs):
    result = {}
    for key, value in pairs:
        require(key not in result, "duplicate-json-key")
        result[key] = value
    return result


def finite_constant(_):
    raise Refused("nonfinite-json-constant")


def clean_strings(value):
    if type(value) is str:
        require(not any(ord(c) < 32 or 0xD800 <= ord(c) <= 0xDFFF or ord(c) == 127 for c in value), "json-string-controls")
    elif type(value) is dict:
        for key, child in value.items():
            clean_strings(key)
            clean_strings(child)


def parse_original_scalar(raw):
    require(type(raw) is bytes and 0 < len(raw) <= SCALAR_CAP, "scalar-size-type")
    require(raw.endswith(b"\n") and not raw.startswith(b"\xef\xbb\xbf"), "scalar-original-framing")
    try:
        result = json.loads(raw.decode("utf-8", errors="strict"), object_pairs_hook=unique_object, parse_constant=finite_constant)
    except (UnicodeError, json.JSONDecodeError, RecursionError, OverflowError) as error:
        raise Refused("scalar-json") from error
    closed_object(result, TOP_KEYS, "scalar-closed-schema")
    clean_strings(result)
    for key in BOOL_KEYS:
        require(type(result[key]) is bool, "scalar-bool:" + key)
    for key in INT_KEYS:
        require(type(result[key]) is int and -(2**53) < result[key] < 2**53, "scalar-int:" + key)
    for key in TOP_KEYS - BOOL_KEYS - INT_KEYS - {"expected", "before", "after", "dpr"}:
        require(type(result[key]) is str, "scalar-string:" + key)
    dpr = result["dpr"]
    require((type(dpr) is int and -(2**53) < dpr < 2**53) or
            (type(dpr) is float and math.isfinite(dpr)), "scalar-dpr")
    for name in ("before", "after"):
        observation = result[name]
        closed_object(observation, OBS_KEYS, "observation-closed-schema:" + name)
        for key in OBS_BOOL_KEYS:
            require(type(observation[key]) is bool, "observation-bool:" + key)
        for key in ("index", "count", "status", "page_status"):
            require(type(observation[key]) is int and -(2**31) <= observation[key] < 2**31, "observation-int:" + key)
        for key in ("document_id", "page_id", "refusal"):
            require(type(observation[key]) is str, "observation-string:" + key)
    closed_object(result["expected"], EXPECTED_KEYS, "expected-closed-schema")
    return result


def decode_png(raw, width, height):
    """Decode static truecolor PNG; indexed/gray/compressed metadata is refused."""
    require(type(raw) is bytes and 0 < len(raw) <= PNG_CAP and raw[:8] == b"\x89PNG\r\n\x1a\n", "png-size-signature")
    offset, index, idat, ended_idat, complete = 8, 0, False, False, False
    compressed = bytearray()
    header = None
    palette = False
    while offset < len(raw):
        require(offset + 12 <= len(raw), "png-short-chunk")
        size = struct.unpack_from(">I", raw, offset)[0]
        kind = raw[offset + 4:offset + 8]
        require(size <= PNG_CAP and offset + 12 + size <= len(raw), "png-chunk-length")
        payload = raw[offset + 8:offset + 8 + size]
        crc = struct.unpack_from(">I", raw, offset + 8 + size)[0]
        require(zlib.crc32(kind + payload) & 0xFFFFFFFF == crc, "png-crc")
        require(all(65 <= c <= 90 or 97 <= c <= 122 for c in kind) and 65 <= kind[2] <= 90, "png-chunk-type")
        require(kind not in (b"acTL", b"fcTL", b"fdAT"), "png-animation")
        require(kind not in (b"zTXt", b"iTXt", b"iCCP"), "png-compressed-ancillary-profile")
        if index == 0:
            require(kind == b"IHDR" and size == 13, "png-first-ihdr")
            require(struct.unpack_from(">II", payload) == (width, height), "png-dimensions")
            header = struct.unpack(">IIBBBBB", payload)
            require(header[3] in (2, 6) and header[2] in (8, 16), "png-fixed-truecolor-profile")
        else:
            require(kind != b"IHDR", "png-duplicate-ihdr")
        if kind == b"PLTE":
            color, depth = header[3], header[2]
            require(not palette and not idat and color in (2, 6) and 0 < size <= 768 and size % 3 == 0,
                    "png-palette-contract")
            palette = True
        if kind == b"IDAT":
            require(not ended_idat, "png-noncontiguous-idat")
            idat = True
            compressed.extend(payload)
        elif idat:
            ended_idat = True
        if kind == b"IEND":
            require(size == 0 and idat and offset + 12 == len(raw), "png-final-iend")
            complete = True
        require(kind in (b"IHDR", b"PLTE", b"IDAT", b"IEND") or kind[0] >= 97, "png-unknown-critical-chunk")
        offset += 12 + size
        index += 1
    require(complete, "png-incomplete")
    _, _, depth, color, compression, filtering, interlace = header
    channels = {2: 3, 6: 4}
    depths = {2: (8, 16), 6: (8, 16)}
    require(color in channels and depth in depths[color] and compression == filtering == 0 and interlace in (0, 1), "png-header-contract")
    passes = [(0, 0, 1, 1)] if not interlace else [(0, 0, 8, 8), (4, 0, 8, 8), (0, 4, 4, 8), (2, 0, 4, 4), (0, 2, 2, 4), (1, 0, 2, 2), (0, 1, 1, 2)]
    rows = []
    for x, y, dx, dy in passes:
        columns, lines = max(0, (width - x + dx - 1) // dx), max(0, (height - y + dy - 1) // dy)
        if columns and lines:
            rows.extend([(columns * channels[color] * depth + 7) // 8] * lines)
    expected_size = sum(length + 1 for length in rows)
    require(expected_size <= PIXEL_CAP * 8 + 1872 * 7, "png-inflate-quota")
    try:
        stream = zlib.decompressobj()
        inflated = stream.decompress(compressed, expected_size + 1)
    except zlib.error as error:
        raise Refused("png-deflate") from error
    require(len(inflated) == expected_size and stream.eof and not stream.unused_data and not stream.unconsumed_tail, "png-deflate-complete")
    offset = 0
    for length in rows:
        require(inflated[offset] <= 4, "png-filter")
        offset += length + 1
    try:
        from PIL import Image
        with Image.open(io.BytesIO(raw)) as image:
            require(image.format == "PNG" and image.size == (width, height) and getattr(image, "n_frames", 1) == 1, "png-decoder-contract")
            image.verify()
        with Image.open(io.BytesIO(raw)) as image:
            image.load()
            pixels = image.convert("RGBA").tobytes()
            require(len(pixels) == width * height * 4, "png-decoded-size")
            return hashlib.sha256(pixels).hexdigest()
    except (OSError, SyntaxError, ValueError) as error:
        if isinstance(error, Refused):
            raise
        raise Refused("png-full-decode") from error


def validate_positive_bytes(scalar, image, expected):
    """Return only byte-level evidence; external admission remains unverified."""
    require(type(expected) is Expected, "expected-type")
    expected.validate()
    body = parse_original_scalar(scalar)
    require(body["schema"] == 1 and body["diagnostic"] == "identity-capture", "schema-purpose")
    require((body["nonce"], body["pid"], body["start"]) == (expected.nonce, expected.pid, expected.start), "request-incarnation")
    require(body["expected"] == dict(document_id=expected.document_id, page_id=expected.page_id, index=expected.index, count=expected.count), "expected-target")
    # Reject bool-as-int even though Python dictionary equality would equate them.
    require(type(body["expected"]["index"]) is int and type(body["expected"]["count"]) is int, "expected-target-types")
    require(body["accepted"], "diagnostic-refused")
    require(all(body[k] for k in ("attempted", "captured", "navigation_attempted", "navigation_dispatched", "image_committed", "image_linked")) and not body["unknown"] and not body["refusal"], "positive-conjunction")
    for key, value in {"stage": "identity-image-observed", "navigation_scope": "legacy-main-composition", "image": "visual-image.png", "acquisition": "QQuickWindow::grabWindow", "coordinates": "full-window", "identity_semantics": "sampled-before-around-after", "content_correspondence": "unverified", "physical_freshness": "unverified"}.items():
        require(body[key] == value, "positive-scope:" + key)
    require(not any(body[k] for k in ("native_authority", "render_authority", "durable_authority", "atomic_authority")), "authority-escalation")
    for name in ("before", "after"):
        observation = body[name]
        require(all(observation[k] for k in ("observed", "map_coherent", "creation_join", "ready")) and not any(observation[k] for k in ("loading", "page_error", "document_error", "awaiting_tiles", "pending_tiles", "working")) and not observation["refusal"] and observation["status"] >= 0 and observation["page_status"] >= 0, "ready-observation:" + name)
        require(all(observation[k] == body["expected"][k] for k in EXPECTED_KEYS), "identity-observation:" + name)
    begin, end = body["sample_begin_ms"], body["operation_end_ms"]
    require(body["setup_limit_ms"] == 90000 and 0 <= begin < 90000 and end == min(90000, begin + 30000), "original-deadline")
    require(begin <= body["capture_begin_ms"] <= body["capture_end_ms"] == body["sample_end_ms"] <= body["publication_ms"] < end, "time-order")
    require(body["capture_end_ms"] - body["capture_begin_ms"] <= 20000, "capture-budget")
    width, height = body["width"], body["height"]
    require(0 < width <= 1872 and 0 < height <= 1872 and width * height <= PIXEL_CAP, "pixel-quota")
    require(0 < body["storage_bytes"] <= STORAGE_CAP and body["dpr"] > 0, "storage-dpr")
    require(type(image) is bytes and 0 < len(image) <= PNG_CAP and len(image) == body["encoded_bytes"] == body["output_bytes"], "encoded-size")
    require(HEX.fullmatch(body["sha256"]) is not None and hashlib.sha256(image).hexdigest() == body["sha256"], "image-sha256")
    pixels_sha = decode_png(image, width, height)
    return {"byte_pair_valid": True, "scalar_sha256": hashlib.sha256(scalar).hexdigest(), "image_sha256": body["sha256"], "decoded_rgba_sha256": pixels_sha, "width": width, "height": height, "external_admission": "unverified", "native_authority": False, "render_authority": False, "durable_authority": False, "atomic_authority": False}
