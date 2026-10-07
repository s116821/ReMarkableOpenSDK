"""Fixed development measurement policy. Importing this module performs no I/O.

No native capability is exported. Target transport and recovery are consumer gates.
"""
import base64
import hashlib
import json
import math
import os
import struct
import time
from pathlib import Path

PLUGIN_SHA256 = "678574220af960704c8f2b622f3e9208f5a0548dad2f636f3f13ead330c312ec"
QTGUI_SHA256 = "93fe582cc61673342ca49e12306d7f689860016582fa46ce135ffa280a972839"
PRE_OFFSET, POST_OFFSET, CALLER_OFFSET = 0x1563C, 0x15664, 0x17E5C
PRE_WORD, POST_WORD = bytes.fromhex("0100000a"), bytes.fromhex("6c009de5")
MAX_REQUESTED, MAX_RAW, DEADLINE = 1024, 8192, 5.0
REGISTERS = ("pc", "cpsr", "sp", "r4", "r5", "r6")
# Contiguous blocks only. A failed read never falls back to smaller requests.
BLOCKS = (("slot", "r5", 0x40, 4), ("contact", "r4", 0, 21),
          ("bounds", "r5", 0x58, 16), ("matrix", "r5", 0x90, 74),
          ("fractions", "sp", 0x20, 16), ("caller", "sp", 0xAC, 4))


class Refused(RuntimeError):
    pass


def encode(value):
    return json.dumps(value, sort_keys=True, separators=(",", ":"),
                      ensure_ascii=True, allow_nan=False).encode("ascii")


class Evidence:
    """Exclusive claim and lossless bounded readback, including failed requests."""
    def __init__(self, directory):
        self.root = Path(directory)
        if not self.root.is_dir() or self.root.is_symlink():
            raise Refused("Private evidence directory refused")
        with (self.root / "normalization.claim").open("xb"):
            pass
        self.entries, self.status = [], "unknown"
        # Fixed-width durable receipts: ordinal is the record position, followed
        # by little-endian byte count and SHA256. No self-hash or raw duplication.
        self.receipts = b"NORMR001"
        self.receipt_path = self.root / "normalization.receipts"
        with self.receipt_path.open("xb") as stream:
            stream.write(self.receipts)
            stream.flush()
            os.fsync(stream.fileno())
        if self.receipt_path.read_bytes() != self.receipts:
            raise Refused("Private receipt readback mismatch")
        self.written = len(self.receipts)

    def save(self):
        raw = encode({"status": self.status, "entry": self.entries[-1],
                      "authority": False})
        if self.written + len(raw) + 36 > MAX_RAW:
            raise Refused("Private raw evidence cap")
        # The claim is spent even when writing or readback fails. Retain old files.
        name = "acquisition-%03d.json" % len(self.entries)
        destination = self.root / name
        with destination.open("xb") as stream:
            stream.write(raw)
            stream.flush()
            os.fsync(stream.fileno())
        saved = destination.read_bytes()
        if saved != raw:
            raise Refused("Private byte readback mismatch")
        digest = hashlib.sha256(saved).digest()
        receipt = struct.pack("<I32s", len(saved), digest)
        with self.receipt_path.open("ab") as stream:
            if stream.tell() != len(self.receipts):
                raise Refused("Private receipt length mismatch")
            stream.write(receipt)
            stream.flush()
            os.fsync(stream.fileno())
        expected = self.receipts + receipt
        if self.receipt_path.read_bytes() != expected:
            raise Refused("Private receipt readback mismatch")
        self.receipts = expected
        self.written += len(raw) + len(receipt)
        return {"path": name, "bytes": len(raw),
                "sha256": digest.hex()}

    def append(self, entry):
        self.entries.append(entry)
        return self.save()


class FixedProbe:
    """One same-call pair through an injected, reviewed transport adapter."""
    def __init__(self, adapter, evidence, addresses, generation, clock=time.monotonic):
        self.io, self.evidence, self.addresses = adapter, evidence, addresses
        self.generation, self.clock = generation, clock
        self.started, self.requested, self.acquired = clock(), 0, 0
        self.successful_stops = 0

    def check_time(self):
        if self.clock() - self.started >= DEADLINE:
            raise Refused("Debugger subdeadline")

    def read(self, phase, name, address, length):
        self.check_time()
        # Charge BEFORE the request; errors and overlapping reads are not refunded.
        self.requested += length
        if self.requested > MAX_REQUESTED:
            raise Refused("Explicit target-memory request cap")
        entry = {"phase": phase, "name": name, "address": address,
                 "requested": length, "acquired": 0, "base64": "",
                 "status": "unknown", "requested_total": self.requested}
        try:
            if not self.io.readable(address, length):
                raise Refused("Reviewed readable range refused")
            data = bytes(self.io.read_memory(address, length))
            entry.update(acquired=len(data), base64=base64.b64encode(data).decode("ascii"),
                         status="complete" if len(data) == length else "partial")
            self.acquired += len(data)
        except Exception:
            # No recoverable bytes from an exception are invented.
            self.evidence.append(entry)
            raise Refused("Target read failed") from None
        self.evidence.append(entry)  # BEFORE interpreting even partial bytes.
        self.check_time()
        if len(data) != length:
            raise Refused("Partial target read")
        return data

    def snapshot(self, phase):
        self.check_time()
        context = self.io.context()
        registers = {}
        for name in REGISTERS:
            try:
                value = self.io.register(name)
            except Exception:
                self.evidence.append({"phase": phase, "register": name,
                                      "status": "unknown", "base64": ""})
                raise Refused("Register read failed") from None
            if type(value) is not int or not -(2**31) <= value < 2**32:
                raise Refused("ARM register width")
            registers[name] = value & 0xFFFFFFFF
            self.evidence.append({"phase": phase, "register": name,
                                  "base64": base64.b64encode(struct.pack("<I", registers[name])).decode("ascii")})
        self.evidence.append({"phase": phase, "context": context})
        if context.get("generation") != self.generation or not context.get("stopped"):
            raise Refused("Stopped generation mismatch")
        if registers["cpsr"] & 0x20:
            raise Refused("Wrong ARM state")
        if registers["pc"] != self.addresses[phase]:
            raise Refused("Unexpected stop address")
        if registers["r6"] != registers["r5"] + 0x90:
            raise Refused("Handler matrix pointer mismatch")
        blocks = {name: self.read(phase, name, registers[base] + offset, length)
                  for name, base, offset, length in BLOCKS}
        # These conditions select the first hit, never its X/Y or computed result.
        if struct.unpack_from("<i", blocks["contact"])[0] != 1 or blocks["contact"][20] != 1:
            raise Refused("Unexpected first contact id/state")
        if struct.unpack("<I", blocks["caller"])[0] != self.addresses["caller"]:
            raise Refused("Wrong current-contact caller")
        self.successful_stops += 1
        return context, registers, blocks

    def run(self):
        pre, post = self.addresses["pre"], self.addresses["post"]
        if self.io.existing_breakpoints():
            raise Refused("Existing debugger breakpoints")
        if self.read("check", "pre-word", pre, 4) != PRE_WORD:
            raise Refused("PRE instruction mismatch")
        if self.read("check", "post-word", post, 4) != POST_WORD:
            raise Refused("POST instruction mismatch")
        bp = self.io.breakpoint(pre)
        self.io.ready()
        if not self.io.resume_to(bp):
            raise Refused("Unexpected PRE stop")
        before = self.snapshot("pre")
        self.io.delete_breakpoint(bp)
        bp = self.io.breakpoint(post, before[0]["thread"])
        if not self.io.resume_to(bp):
            raise Refused("Unexpected POST stop")
        after = self.snapshot("post")
        if before[0] != after[0] or any(before[1][r] != after[1][r] for r in ("sp", "r4", "r5", "r6")):
            raise Refused("Same-call frame/thread mismatch")
        if before[2]["caller"] != after[2]["caller"]:
            raise Refused("Same-call caller mismatch")
        # Matrix cached type may change. All bytes are retained, not forced equal.
        self.io.delete_breakpoint(bp)
        if self.read("readback", "pre-word", pre, 4) != PRE_WORD or self.read("readback", "post-word", post, 4) != POST_WORD:
            raise Refused("Breakpoint word restoration mismatch")
        result = {"status": "measured", "authority": False,
                  "requested_bytes": self.requested, "acquired_bytes": self.acquired,
                  "successful_stops": self.successful_stops, "values": {}}
        for phase, snapshot in (("pre", before), ("post", after)):
            values = struct.unpack("<dd", snapshot[2]["fractions"])
            result["values"][phase] = [{"finite": math.isfinite(v),
                                       "value": v if math.isfinite(v) else None}
                                      for v in values]
        self.evidence.append({"phase": "release-ready", "completion": result})
        self.check_time()
        self.io.detach()
        self.check_time()
        return result
