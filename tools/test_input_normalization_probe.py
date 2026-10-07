"""Host-only failure and persistence tests through the fixed policy entrypoint."""
import base64
import json
import struct
import tempfile
import unittest
from pathlib import Path
from input_normalization_probe import Evidence, FixedProbe, Refused, PRE_WORD, POST_WORD


class OwnedAdapter:
    def __init__(self):
        self.phase, self.detached, self.resumes, self.reads = "pre", False, 0, []
        self.h, self.c, self.sp = 0x30000, 0x40000, 0x50000
        self.addresses = {"pre": 0x10000, "post": 0x10010, "caller": 0x20000}
        self.blocks = {
            0x10000: PRE_WORD, 0x10010: POST_WORD,
            self.h + 0x40: struct.pack("<i", 0),
            self.c: struct.pack("<iiiiiB", 1, 164, 1396, 8, 100, 1),
            self.h + 0x58: struct.pack("<iiii", 0, 1403, 0, 1871),
            self.h + 0x90: struct.pack("<9dH", 1, 0, 0, 0, 1, 0, 0, 0, 1, 0),
            self.sp + 0x20: struct.pack("<dd", 164 / 1403, 1396 / 1871),
            self.sp + 0xAC: struct.pack("<I", self.addresses["caller"]),
        }
        self.failure = None

    def readable(self, address, length):
        return address in self.blocks

    def read_memory(self, address, length):
        self.reads.append((address, length))
        if self.failure == "partial" and address == self.c:
            return self.blocks[address][:5]
        if self.failure == "read" and address == self.c:
            raise OSError("fixture error")
        if self.failure == "readback" and address == 0x10000 and self.resumes == 2:
            return b"bad!"
        return self.blocks[address]

    def context(self):
        return {"generation": "owned-generation", "thread": 2 if self.failure == "thread" and self.phase == "post" else 1,
                "stopped": True, "inferior": 1}

    def register(self, name):
        r = {"pc": self.addresses[self.phase], "cpsr": 0x60000010,
             "sp": self.sp, "r4": self.c, "r5": self.h, "r6": self.h + 0x90}
        if self.failure == "thumb":
            r["cpsr"] |= 0x20
        if self.failure == "frame" and self.phase == "post":
            r["sp"] += 4
        return r[name]

    def existing_breakpoints(self):
        return self.failure == "existing"

    def breakpoint(self, address, thread=None):
        return (address, thread)

    def delete_breakpoint(self, bp):
        pass

    def ready(self):
        pass

    def resume_to(self, bp):
        self.resumes += 1
        self.phase = "pre" if self.resumes == 1 else "post"
        return self.failure not in ("signal", "missing-post") or (self.failure == "missing-post" and self.resumes == 1)

    def detach(self):
        self.detached = True


class ProbeTests(unittest.TestCase):
    def run_case(self, adapter, clock=None):
        with tempfile.TemporaryDirectory() as directory:
            evidence = Evidence(directory)
            probe = FixedProbe(adapter, evidence, adapter.addresses, "owned-generation", **({"clock": clock} if clock else {}))
            try:
                result = probe.run()
            except Refused:
                result = None
            files = sorted(Path(directory).glob("acquisition-*.json"))
            saved = [json.loads(p.read_bytes())["entry"] for p in files]
            self.assertLessEqual(sum(p.stat().st_size for p in files), 8192)
            self.assertLessEqual(sum(e.get("requested", 0) for e in saved), 1024)
            for e in saved:
                if "acquired" in e:
                    self.assertEqual(len(base64.b64decode(e["base64"], validate=True)), e["acquired"])
            return probe, result, saved

    def test_actual_policy_good_and_nonfinite_lossless(self):
        for values in ((0.1169, 0.7461), (float("nan"), float("inf"))):
            a = OwnedAdapter()
            a.blocks[a.sp + 0x20] = struct.pack("<dd", *values)
            p, result, saved = self.run_case(a)
            self.assertEqual(result["status"], "measured")
            self.assertFalse(result["authority"])
            self.assertEqual((p.requested, p.acquired), (286, 286))
            self.assertEqual(result["successful_stops"], 2)
            self.assertTrue(a.detached)
            self.assertEqual([base64.b64decode(e["base64"]) for e in saved if e.get("name") == "fractions"],
                             [a.blocks[a.sp + 0x20]] * 2)

    def test_failures_never_detach_or_retry(self):
        for failure in ("existing", "partial", "read", "thumb", "frame", "thread", "signal", "missing-post", "readback"):
            with self.subTest(failure=failure):
                a = OwnedAdapter(); a.failure = failure
                p, result, saved = self.run_case(a)
                self.assertIsNone(result)
                self.assertFalse(a.detached)
                self.assertLessEqual(a.resumes, 2)
                self.assertEqual(p.requested, sum(e.get("requested", 0) for e in saved))
                self.assertGreaterEqual(p.requested, sum(length for _, length in a.reads))
                if failure in ("partial", "read"):
                    entry = next(e for e in saved if e.get("name") == "contact")
                    self.assertEqual(entry["acquired"], 5 if failure == "partial" else 0)
                    self.assertEqual(entry["requested"], 21)
                    self.assertEqual(a.reads.count((a.c, 21)), 1)

    def test_first_contact_refusal_no_xy_sampling(self):
        for contact in ((2, 164, 1396, 1), (1, 164, 1396, 8), (1, 0, 1871, 1)):
            a = OwnedAdapter()
            ident, x, y, state = contact
            a.blocks[a.c] = struct.pack("<iiiiiB", ident, x, y, 8, 100, state)
            _, result, _ = self.run_case(a)
            self.assertEqual(result is not None, ident == 1 and state == 1)
            self.assertEqual(a.resumes, 2 if result else 1)

    def test_generation_opcode_and_deadline_refusal(self):
        a = OwnedAdapter(); a.blocks[0x10000] = b"bad!"
        _, result, _ = self.run_case(a); self.assertIsNone(result); self.assertEqual(a.resumes, 0)
        a = OwnedAdapter(); a.context = lambda: {"generation": "reused", "thread": 1, "stopped": True}
        _, result, _ = self.run_case(a); self.assertIsNone(result); self.assertEqual(a.resumes, 1)
        times = iter((0, 5))
        a = OwnedAdapter(); _, result, _ = self.run_case(a, lambda: next(times))
        self.assertIsNone(result); self.assertEqual(a.reads, [])

    def test_claim_and_persistence_failure_spent(self):
        with tempfile.TemporaryDirectory() as directory:
            e = Evidence(directory)
            with self.assertRaises(FileExistsError):
                Evidence(directory)
            Path(directory, "acquisition-001.json").mkdir()
            a = OwnedAdapter()
            with self.assertRaises(OSError):
                FixedProbe(a, e, a.addresses, "owned-generation").run()
            self.assertTrue(Path(directory, "normalization.claim").exists())
            self.assertEqual(a.reads, [(0x10000, 4)])
            self.assertFalse(a.detached)

    def test_completion_persistence_failure_prevents_release(self):
        class BlockCompletion(Evidence):
            def append(self, entry):
                if entry.get("phase") == "release-ready":
                    # Actual filesystem failure at the last write, after both stops.
                    (self.root / ("acquisition-%03d.json" % (len(self.entries) + 1))).mkdir()
                return super().append(entry)

        with tempfile.TemporaryDirectory() as directory:
            e, a = BlockCompletion(directory), OwnedAdapter()
            p = FixedProbe(a, e, a.addresses, "owned-generation")
            with self.assertRaises(OSError):
                p.run()
            self.assertEqual((p.successful_stops, p.requested), (2, 286))
            self.assertFalse(a.detached)
            self.assertTrue((Path(directory) / "normalization.claim").exists())
            self.assertLessEqual(sum(f.stat().st_size for f in Path(directory).glob("acquisition-*.json") if f.is_file()), 8192)

    def test_exhausted_caps_do_not_retry_or_resume(self):
        with tempfile.TemporaryDirectory() as directory:
            e, a = Evidence(directory), OwnedAdapter()
            e.written = 8192
            p = FixedProbe(a, e, a.addresses, "owned-generation")
            with self.assertRaisesRegex(Refused, "raw evidence cap"):
                p.run()
            self.assertEqual(a.reads, [(0x10000, 4)])
            self.assertEqual(a.resumes, 0)
            self.assertFalse(a.detached)
        with tempfile.TemporaryDirectory() as directory:
            e, a = Evidence(directory), OwnedAdapter()
            p = FixedProbe(a, e, a.addresses, "owned-generation")
            p.requested = 1024
            with self.assertRaisesRegex(Refused, "request cap"):
                p.run()
            self.assertEqual((a.reads, a.resumes, a.detached), ([], 0, False))

    def test_register_failure_keeps_unknown_evidence(self):
        a = OwnedAdapter()
        original = a.register
        def fail_register(name):
            if name == "sp":
                raise OSError("owned register failure")
            return original(name)
        a.register = fail_register
        _, result, saved = self.run_case(a)
        self.assertIsNone(result)
        self.assertFalse(a.detached)
        self.assertEqual(a.resumes, 1)
        self.assertIn({"phase": "pre", "register": "sp", "status": "unknown", "base64": ""}, saved)


if __name__ == "__main__":
    unittest.main()
