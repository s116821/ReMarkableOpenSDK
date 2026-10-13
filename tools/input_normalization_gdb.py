"""GDB adapter; no attach, connect, or target action occurs on import.

The native runtime entrypoint remains source-only pending the disposable-server
amendment. The explicit host command below requires the recorded owned QEMU PID.
"""
import os
import sys
import time
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parent))
import gdb
from input_normalization_probe import Evidence, FixedProbe, Refused

SOURCE_ONLY = True


class GdbAdapter:
    def __init__(self, evidence, generation, ranges):
        self.evidence, self.generation, self.ranges = evidence, generation, ranges
        self.inferior = gdb.selected_inferior()
        self.inferior_num, self.pid = self.inferior.num, self.inferior.pid
        self.last_stop, self.announce, self.event_failure = None, False, False
        gdb.events.stop.connect(self.on_stop)
        gdb.events.cont.connect(self.on_continue)

    def close(self):
        gdb.events.stop.disconnect(self.on_stop)
        gdb.events.cont.disconnect(self.on_continue)

    def on_stop(self, event):
        self.last_stop = event

    def on_continue(self, event):
        if self.announce:
            self.announce = False
            try:
                self.evidence.append({"phase": "ready", "armed_resumed": True,
                                      "generation": self.generation})
            except Exception:
                # GDB may swallow event-handler exceptions. Retain failure for
                # the command and do not announce readiness. Host watchdog
                # still owns termination if the next stop never arrives.
                self.event_failure = True
                return
            # Bounded marker only, never raw target data.
            gdb.write("NORMALIZATION_ARMED_RESUMED\n")

    def existing_breakpoints(self):
        return bool(gdb.breakpoints())

    def register(self, name):
        return int(gdb.selected_frame().read_register(name))

    def context(self):
        inferior, thread = gdb.selected_inferior(), gdb.selected_thread()
        if inferior.num != self.inferior_num or inferior.pid != self.pid or thread is None:
            raise Refused("Attached inferior changed")
        return {"generation": self.generation, "pid": inferior.pid,
                "inferior": inferior.num, "thread": thread.global_num,
                "stopped": thread.is_stopped()}

    def readable(self, address, length):
        return type(address) is int and 0 < address < 2**32 and any(
            start <= address and address + length <= end for start, end in self.ranges)

    def read_memory(self, address, length):
        return self.inferior.read_memory(address, length).tobytes()

    def breakpoint(self, address, thread=None):
        bp = gdb.Breakpoint("*0x%x" % address, internal=True)
        if thread is not None:
            bp.thread = thread
        return bp

    def delete_breakpoint(self, bp):
        bp.delete()

    def ready(self):
        self.announce = True  # Published only from the actual continue event.

    def resume_to(self, bp):
        self.last_stop = None
        # No event handler changes breakpoints or resumes the inferior.
        gdb.execute("continue", to_string=False)
        if self.event_failure:
            raise Refused("Continue-event persistence failed")
        return isinstance(self.last_stop, gdb.BreakpointEvent) and tuple(self.last_stop.breakpoints) == (bp,)

    def detach(self):
        gdb.execute("detach", to_string=False)


class OwnedFixtureCommand(gdb.Command):
    """Host test only: already-connected owned QEMU PID, fixed fixture symbols."""
    def __init__(self):
        super().__init__("normalization-host-fixture", gdb.COMMAND_USER)

    def invoke(self, argument, from_tty):
        inferior = gdb.selected_inferior()
        filename = gdb.current_progspace().filename
        expected_pid = os.environ.get("SDK_NORMALIZATION_QEMU_PID", "")
        if not expected_pid.isdecimal() or inferior.pid != int(expected_pid) or filename is None or Path(filename).name != "input-normalization-owned-fixture":
            raise gdb.GdbError("Host owned QEMU fixture identity refused")
        evidence = Evidence(argument)
        addresses = {phase: int(gdb.parse_and_eval("&" + symbol)) for phase, symbol in
                     (("pre", "fixture_pre"), ("post", "fixture_post"), ("caller", "fixture_current_return"))}
        generation = "host-owned-qemu-pid" + expected_pid
        adapter = GdbAdapter(evidence, generation, ((0x10000, 2**32),))
        try:
            result = FixedProbe(adapter, evidence, addresses, generation).run()
            evidence.status = "measured"
            evidence.append({"result": result})
            gdb.write("NORMALIZATION_HOST_FIXTURE_MEASURED\n")
        except Exception:
            # This branch also never detach/resume/quit. An independent host test
            # watchdog kills the owned fixture/container. Native mode is absent.
            gdb.write("NORMALIZATION_RECOVERY_REQUIRED\n", gdb.STDERR)
            while True:
                time.sleep(0.05)
        finally:
            adapter.close()


OwnedFixtureCommand()  # Registration only. No runtime/native command is exposed.
