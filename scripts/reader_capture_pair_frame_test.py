"""Length-frame controls; optionally consume original C++ owned frame exports."""
import argparse
from dataclasses import replace
import hashlib
import json
from pathlib import Path
import unittest

from reader_capture_pair_frame import END, parse_frame
from reader_identity_capture_consumer import Expected, Refused
from reader_identity_capture_consumer_test import positive, encode


BOOT = "01234567-89ab-4cde-8f01-23456789abcd"


def frame(scalar, image, expected, boot=BOOT):
    return ("RMB_CAPTURE_PAIR_V1 " + " ".join((expected.nonce, expected.pid, expected.start, boot,
                str(len(scalar)), str(len(image)))) + "\n").encode() + scalar + image + END


class FrameTest(unittest.TestCase):
    def setUp(self):
        self.body, self.image, self.expected = positive()
        self.scalar = encode(self.body)
        self.raw = frame(self.scalar, self.image, self.expected)

    def test_original_positive_bytes(self):
        result = parse_frame(self.raw, self.expected, BOOT)
        self.assertEqual(result.scalar, self.scalar)
        self.assertEqual(result.image, self.image)
        self.assertEqual(result.evidence["frame_sha256"], hashlib.sha256(self.raw).hexdigest())
        self.assertEqual(result.evidence["external_admission"], "unverified")
        self.assertFalse(result.evidence["native_authority"])

    def test_header_and_binding_refusals(self):
        header = self.raw[:self.raw.index(b"\n")]
        for bad in (b"", b"OTHER " + header, header.replace(b"V1", b"V2"), header + b" extra",
                    header.replace(b" ", b"  ", 1), header.replace(self.expected.nonce.encode(), b"d" * 32),
                    header.replace(BOOT.encode(), b"01234567-89AB-4cde-8f01-23456789abcd"),
                    header.replace(b"101 ", b"0101 "), header + b"\xff"):
            with self.subTest(header=bad[:40]), self.assertRaises(Refused):
                parse_frame(bad + b"\n" + self.scalar + self.image + END, self.expected, BOOT)
        with self.assertRaises(Refused):
            parse_frame(self.raw, replace(self.expected, start="123457"), BOOT)
        with self.assertRaises(Refused):
            parse_frame(self.raw, self.expected, "00000000-0000-0000-0000-000000000000")

    def test_lengths_and_exact_terminal_frame(self):
        parts = self.raw[:self.raw.index(b"\n")].split(b" ")
        for index in (5, 6):
            for value in (b"0", b"-1", b"+1", b"01", b"1.0", b"1e3", b"1048577", b"9" * 80, b"1"):
                changed = list(parts); changed[index] = value
                with self.subTest(index=index, value=value), self.assertRaises(Refused):
                    parse_frame(b" ".join(changed) + b"\n" + self.scalar + self.image + END, self.expected, BOOT)
        for value in (self.raw[:-1], self.raw + b"\n", self.raw + self.raw,
                      self.raw.replace(END, b"\nOTHER_END\n"), b"x" * 256 + b"\n" + self.raw):
            with self.subTest(length=len(value)), self.assertRaises(Refused):
                parse_frame(value, self.expected, BOOT)

    def test_semantic_and_image_refusal_after_valid_framing(self):
        body = dict(self.body, accepted=False, refusal="deadline")
        with self.assertRaisesRegex(Refused, "diagnostic-refused"):
            parse_frame(frame(encode(body), self.image, self.expected), self.expected, BOOT)
        with self.assertRaisesRegex(Refused, "image-sha256"):
            parse_frame(frame(self.scalar, self.image[:-1] + b"x", self.expected), self.expected, BOOT)
        scalar = self.scalar.replace(b'"accepted":true', b'"accepted":false,"accepted":true')
        with self.assertRaisesRegex(Refused, "duplicate-json-key"):
            parse_frame(frame(scalar, self.image, self.expected), self.expected, BOOT)


def check_cpp_export(path):
    root = Path(path)
    facts = json.loads((root / "frame-facts.json").read_bytes())
    expected = Expected(**{key: facts[key] for key in Expected.__dataclass_fields__})
    raw = (root / "original-frame.bin").read_bytes()
    result = parse_frame(raw, expected, facts["boot"])
    return dict(source="owned-cpp-frame-format-only", **result.evidence)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--cpp-export", action="append", default=[])
    args = parser.parse_args()
    outcome = unittest.TextTestRunner(verbosity=2).run(unittest.defaultTestLoader.loadTestsFromTestCase(FrameTest))
    if not outcome.wasSuccessful():
        raise SystemExit(1)
    records = [check_cpp_export(path) for path in args.cpp_export]
    print(json.dumps(dict(synthetic_methods=outcome.testsRun, actual_cpp_frames=records,
                          native_execution=False, live_admission_qualified=False), indent=2))
