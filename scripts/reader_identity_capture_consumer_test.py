"""Original-byte consumer controls. Synthetic vectors are labeled separately.

Pass --entry-exports DIR repeatedly to also check exact owned Entry serializer
outputs. These exports use an injected capture producer, not a tablet capture.
"""
import argparse
from copy import deepcopy
from dataclasses import replace
import hashlib
import io
import json
from pathlib import Path
import struct
import unittest
import zlib

from PIL import Image
import reader_identity_capture_consumer as consumer


def encode(body):
    return json.dumps(body, separators=(",", ":"), allow_nan=False).encode() + b"\n"


def positive():
    expected = consumer.Expected("c" * 32, "101", "123456", "10000000-0000-0000-0000-000000000001", "00000000-0000-4000-8000-000000000002", 1, 3)
    stream = io.BytesIO()
    Image.new("RGBA", (24, 16), (31, 67, 101, 255)).save(stream, format="PNG")
    image = stream.getvalue()
    target = dict(document_id=expected.document_id, page_id=expected.page_id, index=1, count=3)
    observation = dict(target, observed=True, map_coherent=True, creation_join=True, ready=True,
                       status=1, page_status=1, loading=False, page_error=False, document_error=False,
                       awaiting_tiles=False, pending_tiles=False, working=False, refusal="")
    body = dict(schema=1, diagnostic="identity-capture", nonce=expected.nonce, pid=expected.pid, start=expected.start,
                stage="identity-image-observed", refusal="", accepted=True, attempted=True, captured=True, unknown=False,
                navigation_attempted=True, navigation_dispatched=True, navigation_scope="legacy-main-composition",
                expected=target, before=deepcopy(observation), after=deepcopy(observation),
                sample_begin_ms=100, sample_end_ms=6100, capture_begin_ms=5100, capture_end_ms=6100,
                publication_ms=6101, operation_end_ms=30100, setup_limit_ms=90000,
                image_committed=True, image_linked=True, image="visual-image.png", width=24, height=16,
                storage_bytes=1536, dpr=1, encoded_bytes=len(image), output_bytes=len(image),
                sha256=hashlib.sha256(image).hexdigest(), acquisition="QQuickWindow::grabWindow",
                coordinates="full-window", identity_semantics="sampled-before-around-after",
                content_correspondence="unverified", physical_freshness="unverified",
                native_authority=False, render_authority=False, durable_authority=False, atomic_authority=False)
    return body, image, expected


def chunk(kind, data):
    return struct.pack(">I", len(data)) + kind + data + struct.pack(">I", zlib.crc32(kind + data) & 0xFFFFFFFF)


def replace_idat(image, transform):
    offset, result = 8, image[:8]
    while offset < len(image):
        length = struct.unpack_from(">I", image, offset)[0]
        kind = image[offset + 4:offset + 8]
        data = image[offset + 8:offset + 8 + length]
        result += chunk(kind, transform(data)) if kind == b"IDAT" else image[offset:offset + length + 12]
        offset += length + 12
    return result


class ConsumerTest(unittest.TestCase):
    def setUp(self):
        self.body, self.image, self.expected = positive()

    def validate(self, body=None, image=None):
        return consumer.validate_positive_bytes(encode(self.body if body is None else body), self.image if image is None else image, self.expected)

    def refused(self, body=None, image=None):
        with self.assertRaises(consumer.Refused):
            self.validate(body, image)

    def test_positive_synthetic(self):
        result = self.validate()
        self.assertTrue(result["byte_pair_valid"])
        self.assertEqual(result["external_admission"], "unverified")
        self.assertFalse(any(result[k] for k in ("native_authority", "render_authority", "durable_authority", "atomic_authority")))
        self.assertEqual(len(result["decoded_rgba_sha256"]), 64)

    def test_supported_png_variants_and_fixed_profile_refusals(self):
        for mode in ("RGB", "RGBA"):
            image = io.BytesIO(); Image.new(mode, (24, 16)).save(image, format="PNG")
            raw = image.getvalue()
            for value in (raw, raw[:33] + chunk(b"PLTE", b"\0\0\0") + raw[33:]):
                body = deepcopy(self.body); body.update(encoded_bytes=len(value), output_bytes=len(value), sha256=hashlib.sha256(value).hexdigest())
                self.assertTrue(self.validate(body, value)["byte_pair_valid"])
        for mode in ("P", "1", "L", "LA"):
            image = io.BytesIO(); Image.new(mode, (24, 16)).save(image, format="PNG")
            value = image.getvalue()
            body = deepcopy(self.body); body.update(encoded_bytes=len(value), output_bytes=len(value), sha256=hashlib.sha256(value).hexdigest())
            with self.assertRaisesRegex(consumer.Refused, "png-fixed-truecolor-profile"):
                self.validate(body, value)

    def test_expected_binding_ranges(self):
        for key, value in (("pid", "1"), ("pid", str(2**31)), ("start", str(2**64)),
                           ("nonce", "C" * 32), ("document_id", "00000000-0000-0000-0000-000000000000"),
                           ("index", True), ("count", 17), ("start", "0123")):
            with self.subTest(field=key), self.assertRaises(consumer.Refused):
                consumer.validate_positive_bytes(encode(self.body), self.image, replace(self.expected, **{key: value}))

    def test_every_top_field_missing_or_added(self):
        for key in self.body:
            with self.subTest(missing=key):
                body = deepcopy(self.body); del body[key]; self.refused(body)
        body = deepcopy(self.body); body["extra"] = False; self.refused(body)

    def test_every_nested_field_missing_or_added(self):
        for name in ("expected", "before", "after"):
            for key in self.body[name]:
                with self.subTest(nested=name, missing=key):
                    body = deepcopy(self.body); del body[name][key]; self.refused(body)
            body = deepcopy(self.body); body[name]["extra"] = False; self.refused(body)

    def test_strict_numeric_and_boolean_types(self):
        for key in consumer.BOOL_KEYS:
            body = deepcopy(self.body); body[key] = int(body[key]); self.refused(body)
        for key in consumer.INT_KEYS:
            for value in (True, str(self.body[key]), float(self.body[key]), 2**53):
                body = deepcopy(self.body); body[key] = value; self.refused(body)
        for name in ("before", "after"):
            for key in consumer.OBS_BOOL_KEYS:
                body = deepcopy(self.body); body[name][key] = int(body[name][key]); self.refused(body)
            for key in ("index", "count", "status", "page_status"):
                body = deepcopy(self.body); body[name][key] = True; self.refused(body)
        for key in ("index", "count"):
            body = deepcopy(self.body); body["expected"][key] = True; self.refused(body)
        for value in (True, "1", None, 0, -1, 10**400):
            body = deepcopy(self.body); body["dpr"] = value; self.refused(body)

    def test_duplicates_unicode_and_nonfinite_original_bytes(self):
        raw = encode(self.body)
        malformed = [raw.replace(b'"accepted":true', b'"accepted":false,"accepted":true'),
                     raw.replace(b'"index":1', b'"index":0,"index":1', 1),
                     raw.replace(b'"dpr":1', b'"dpr":NaN'), raw.replace(b'"dpr":1', b'"dpr":Infinity'),
                     raw.replace(b'"dpr":1', b'"dpr":1e999'), raw.replace(b'"refusal":""', b'"refusal":"\\ud800"', 1),
                     raw.replace(b'"refusal":""', b'"refusal":"\\u0000"', 1),
                     b"\xef\xbb\xbf" + raw, raw[:-1], raw + b"{}", raw.replace(b"identity-capture", b"\xff"),
                     raw + b" " * 4096, b"[" * 2000 + b"0" + b"]" * 2000 + b"\n"]
        for value in malformed:
            with self.subTest(raw_prefix=value[:30]), self.assertRaises(consumer.Refused):
                consumer.validate_positive_bytes(value, self.image, self.expected)

    def test_positive_contract_mutations(self):
        changes = dict(schema=2, diagnostic="other", nonce="d" * 32, pid="102", start="123457",
                       accepted=False, stage="observed", refusal="deadline", attempted=False, captured=False,
                       unknown=True, navigation_attempted=False, navigation_dispatched=False, navigation_scope="other",
                       image_committed=False, image_linked=False, image="../visual-image.png", acquisition="other",
                       coordinates="page", identity_semantics="atomic", content_correspondence="verified",
                       physical_freshness="verified", native_authority=True, render_authority=True,
                       durable_authority=True, atomic_authority=True, width=1873, height=0,
                       storage_bytes=consumer.STORAGE_CAP + 1, encoded_bytes=0, output_bytes=len(self.image) + 1,
                       sha256=self.body["sha256"].upper())
        for key, value in changes.items():
            with self.subTest(field=key):
                body = deepcopy(self.body); body[key] = value; self.refused(body)

    def test_observation_and_target_mutations(self):
        for name in ("before", "after"):
            changes = dict(observed=False, map_coherent=False, creation_join=False, ready=False,
                           document_id=self.expected.page_id, page_id=self.expected.document_id, index=2, count=4,
                           status=-1, page_status=-1, loading=True, page_error=True, document_error=True,
                           awaiting_tiles=True, pending_tiles=True, working=True, refusal="changed")
            for key, value in changes.items():
                with self.subTest(observation=name, field=key):
                    body = deepcopy(self.body); body[name][key] = value; self.refused(body)
        for key, value in dict(document_id=self.expected.page_id, page_id=self.expected.document_id, index=0, count=4).items():
            body = deepcopy(self.body); body["expected"][key] = value; self.refused(body)

    def test_original_clock_mutations_and_setup_clamp(self):
        for key, value in dict(sample_begin_ms=-1, capture_begin_ms=99, capture_end_ms=2000, sample_end_ms=6102,
                               publication_ms=30100, operation_end_ms=30101, setup_limit_ms=90001).items():
            body = deepcopy(self.body); body[key] = value; self.refused(body)
        body = deepcopy(self.body); body.update(sample_begin_ms=81000, capture_begin_ms=86000,
                                              capture_end_ms=87000, sample_end_ms=87000, publication_ms=87001, operation_end_ms=90000)
        self.assertTrue(self.validate(body)["byte_pair_valid"])
        body.update(capture_begin_ms=100, capture_end_ms=20101, sample_end_ms=20101, sample_begin_ms=100, publication_ms=20102, operation_end_ms=30100)
        self.refused(body)

    def test_png_corruption_and_complete_stream(self):
        malformed = [b"", self.image[:-1], self.image + b"trailing", self.image[:20] + bytes([self.image[20] ^ 1]) + self.image[21:],
                     self.image[:-12], self.image[:8] + chunk(b"IHDR", struct.pack(">IIBBBBB", 25, 16, 8, 6, 0, 0, 0)) + self.image[33:],
                     self.image[:33] + chunk(b"acTL", struct.pack(">II", 1, 0)) + self.image[33:],
                     self.image[:-12] + chunk(b"PLTE", b"\0\0\0") + self.image[-12:],
                     self.image[:33] + chunk(b"PLTE", b"\0\0\0") * 2 + self.image[33:],
                     self.image[:33] + chunk(b"PLTE", b"\0\0") + self.image[33:],
                     self.image[:33] + chunk(b"zTXt", b"key\0\0" + zlib.compress(b"x" * 1500000)) + self.image[33:],
                     self.image[:33] + chunk(b"iTXt", b"key\0\1\0\0\0" + zlib.compress(b"x" * 1500000)) + self.image[33:],
                     self.image[:33] + chunk(b"iCCP", b"profile\0\0" + zlib.compress(b"x" * 1500000)) + self.image[33:],
                     replace_idat(self.image, lambda data: data + b"junk"),
                     replace_idat(self.image, lambda data: data + zlib.compress(b"another stream")),
                     replace_idat(self.image, lambda _: zlib.compress(b"\0" * (24 * 16 * 4 + 16 + 1))),
                     replace_idat(self.image, lambda _: zlib.compress(b"\5" + b"\0" * (24 * 16 * 4 + 15))),
                     self.image[:33] + chunk(b"ABCD", b"") + self.image[33:]]
        for image in malformed:
            with self.subTest(length=len(image)):
                # Bind a valid digest/size to malformed data to exercise decoder,
                # not just the earlier original-byte hash gate.
                body = deepcopy(self.body); body.update(encoded_bytes=len(image), output_bytes=len(image), sha256=hashlib.sha256(image).hexdigest())
                self.refused(body, image)
        self.refused(image=self.image[:-1] + b"x")


def validate_exports(directories):
    records = []
    for directory in directories:
        cases = sorted(Path(directory).glob("case-*/expectation.json"))
        if not cases:
            raise RuntimeError("no actual Entry exports at " + directory)
        for facts_path in cases:
            facts = json.loads(facts_path.read_bytes())
            expected = consumer.Expected(**{key: facts[key] for key in consumer.Expected.__dataclass_fields__})
            root = facts_path.parent
            result = consumer.validate_positive_bytes((root / "selected-result").read_bytes(), (root / "visual-image.png").read_bytes(), expected)
            records.append(dict(case=str(root), producer="injected-owned-entry", **result))
    return records


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--entry-exports", action="append", default=[])
    args = parser.parse_args()
    outcome = unittest.TextTestRunner(verbosity=2).run(unittest.defaultTestLoader.loadTestsFromTestCase(ConsumerTest))
    if not outcome.wasSuccessful():
        raise SystemExit(1)
    records = validate_exports(args.entry_exports)
    print(json.dumps(dict(synthetic_test_methods=outcome.testsRun, actual_owned_entry_pairs=len(records), pairs=records,
                          native_execution=False, live_admission_qualified=False), indent=2))
