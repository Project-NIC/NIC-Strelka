"""Strelka's records to C3D, the gait laboratories' format: the units' six channels as analog
channels on one time base, a frame a step; nothing but the data in the record (RECORD.md). Plain
Python, no numpy.

    python3 tools/strelka_c3d.py out.c3d S00007B01.STRK [S00007B02.STRK …]

One file a Belka, the files of one session: they must share the session's number and the rate into
memory, and they are laid side by side by the set's grid — the period's number, the block's place
and the step's — so that a Belka a period early or late lines up all the same (PRINCIPLE.md). The
channels are in g and °/s, by the ranges and by each Belka's calibration where its head carries
one, as tools/strelka_record.py converts them; a step lost is zeros in its frame. C3D's frame count
is 16 bits, so a long record is folded: several steps a frame, ANALOG:RATE the steps' rate,
POINT:RATE the frames'. The group STRELKA carries the heads.
"""

import math
import os
import struct
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import strelka_record as rec  # noqa: E402

AXES = ("AX", "AY", "AZ", "GX", "GY", "GZ")
UNITS = ("g", "g", "g", "dps", "dps", "dps")
MAX_FRAMES = 65535
PROCESSOR_INTEL = 84


def channels(session, first):
    """The session's steps as six lists of physical values, index = the step's place on the set's
    grid less `first`, the grid index the set's frames start at."""
    head = session["head"]
    cal = rec.calibration(head)
    out = {}
    for k, blk, j, raw in rec.steps(session):
        if tuple(raw) == rec.MISSING:
            continue                         # a step lost: zeros in its frame
        out[k - first] = rec.convert(head, raw, cal)
    return out


def first_index(sessions):
    """Where the set's frames start: the earliest step on the grid among the sessions."""
    firsts = []
    for s in sessions:
        for k, blk, j, raw in rec.steps(s):
            firsts.append(k)
            break
    return min(firsts) if firsts else 0


class Params:
    """The parameter section: groups and parameters packed as C3D wants them."""

    def __init__(self):
        self.records = []
        self.ids = {}

    def group(self, name, desc=""):
        gid = -(len(self.ids) + 1)
        self.ids[name] = -gid
        self.records.append(self._record(gid, name, b"", desc))

    def param(self, group, name, kind, dims, data, desc=""):
        gid = self.ids[group]
        body = struct.pack("<b", kind) + struct.pack("<B", len(dims)) + bytes(dims) + data
        self.records.append(self._record(gid, name, body, desc))

    @staticmethod
    def _record(gid, name, body, desc):
        name_b = name.encode()
        desc_b = desc.encode()
        tail = body + struct.pack("<B", len(desc_b)) + desc_b
        head = struct.pack("<bb", len(name_b), gid) + name_b
        return head, tail

    def int16(self, group, name, value, desc=""):
        self.param(group, name, 2, [], struct.pack("<h", value), desc)

    def uint16(self, group, name, value, desc=""):
        """Stored as a 16-bit integer and read unsigned, as FRAMES is by the readers."""
        self.param(group, name, 2, [], struct.pack("<H", value), desc)

    def int16s(self, group, name, values, desc=""):
        self.param(group, name, 2, [len(values)], struct.pack(f"<{len(values)}h", *values), desc)

    def float32(self, group, name, value, desc=""):
        self.param(group, name, 4, [], struct.pack("<f", value), desc)

    def floats(self, group, name, values, desc=""):
        self.param(group, name, 4, [len(values)], struct.pack(f"<{len(values)}f", *values), desc)

    def strings(self, group, name, values, width, desc=""):
        data = b"".join(v.encode().ljust(width)[:width] for v in values)
        self.param(group, name, -1, [width, len(values)], data, desc)

    def string(self, group, name, value, desc=""):
        self.param(group, name, -1, [len(value)], value.encode(), desc)

    def pack(self):
        out = bytearray(struct.pack("<BBBB", 1, 80, 0, PROCESSOR_INTEL))
        for i, (head, tail) in enumerate(self.records):
            last = i == len(self.records) - 1
            offset = 0 if last else 2 + len(tail)
            out += head + struct.pack("<h", offset) + tail
        blocks = (len(out) + 511) // 512
        out[2] = blocks
        return bytes(out) + b"\x00" * (blocks * 512 - len(out))


def write(out, sessions):
    if not sessions:
        raise SystemExit("no session")
    rates = {s["head"]["rate_hz"] for s in sessions}
    numbers = {s["head"]["session"] for s in sessions}
    if len(rates) != 1 or len(numbers) != 1:
        raise SystemExit("the files differ in their session or their rate: not one session's")
    rate = rates.pop()
    first = first_index(sessions)
    data = [channels(s, first) for s in sessions]
    steps = max((max(d) + 1 if d else 0) for d in data)
    per_frame = max(1, math.ceil(steps / MAX_FRAMES))
    frames = math.ceil(steps / per_frame) if steps else 1
    n_ch = 6 * len(sessions)
    labels = [f"B{s['head']['belka']}_{a}" for s in sessions for a in AXES]
    units = [u for _ in sessions for u in UNITS]

    p = Params()
    p.group("POINT", "no 3D points: the units' channels are analog")
    p.int16("POINT", "USED", 0)
    p.uint16("POINT", "FRAMES", frames)
    p.float32("POINT", "SCALE", -1.0)
    p.float32("POINT", "RATE", rate / per_frame)
    p.strings("POINT", "LABELS", [], 4)
    p.strings("POINT", "DESCRIPTIONS", [], 4)
    p.strings("POINT", "UNITS", ["mm"], 4)
    p.group("ANALOG", "the units' accelerometers and gyros")
    p.int16("ANALOG", "USED", n_ch)
    p.strings("ANALOG", "LABELS", labels, 16)
    p.strings("ANALOG", "DESCRIPTIONS", labels, 16)
    p.floats("ANALOG", "SCALE", [1.0] * n_ch)
    p.int16s("ANALOG", "OFFSET", [0] * n_ch)
    p.float32("ANALOG", "GEN_SCALE", 1.0)
    p.strings("ANALOG", "UNITS", units, 4)
    p.float32("ANALOG", "RATE", float(rate))
    p.string("ANALOG", "FORMAT", "SIGNED")
    p.int16("ANALOG", "BITS", 16)
    p.group("STRELKA", "the records' heads")
    p.int16("STRELKA", "SESSION", sessions[0]["head"]["session"])
    p.int16s("STRELKA", "BELKA", [s["head"]["belka"] for s in sessions])
    p.strings("STRELKA", "CHIP", [f"{s['head']['chip']:016X}" for s in sessions], 16)
    p.strings("STRELKA", "PLACE", [s["head"]["place"] for s in sessions], 32)
    p.int16s("STRELKA", "N", [s["head"]["n"] for s in sessions])
    p.int16s("STRELKA", "ACC_G", [s["head"]["acc_g"] for s in sessions])
    p.int16s("STRELKA", "GYRO_DPS", [s["head"]["gyro_dps"] for s in sessions])
    p.strings("STRELKA", "TIME_US", [str(s["head"]["time_us"]) for s in sessions], 20)
    p.strings("STRELKA", "TIME_PERIOD", [str(s["head"]["time_period"]) for s in sessions], 12)
    p.string("STRELKA", "FIRST_GRID_INDEX", str(first))
    p.int16("STRELKA", "STEPS_PER_FRAME", per_frame)
    p.int16("STRELKA", "STEPS", min(steps, 32767))
    # POINT:DATA_START needs the section's own size: packed once to measure, then with it
    data_start = 2 + len(p.pack()) // 512
    p.int16("POINT", "DATA_START", data_start)
    params = p.pack()
    assert data_start == 2 + len(params) // 512

    header = bytearray(512)
    struct.pack_into("<BB", header, 0, 2, 80)
    struct.pack_into("<H", header, 2, 0)                       # 3D points
    struct.pack_into("<H", header, 4, n_ch * per_frame)        # analog measurements a frame
    struct.pack_into("<HH", header, 6, 1, frames)
    struct.pack_into("<H", header, 10, 0)
    struct.pack_into("<f", header, 12, -1.0)
    struct.pack_into("<H", header, 16, data_start)
    struct.pack_into("<H", header, 18, per_frame)
    struct.pack_into("<f", header, 20, rate / per_frame)
    struct.pack_into("<H", header, 298, 12345)                 # the labels' key, no labels

    body = bytearray()
    zero = [0.0] * 6
    for f in range(frames):
        for sub in range(per_frame):
            k = f * per_frame + sub
            for d in data:
                body += struct.pack("<6f", *d.get(k, zero))
    body += b"\x00" * ((512 - len(body) % 512) % 512)
    out.write(bytes(header) + params + bytes(body))
    return dict(frames=frames, per_frame=per_frame, channels=n_ch, steps=steps)


def main(argv):
    if len(argv) < 3:
        print(__doc__)
        return 2
    sessions = []
    for path in argv[2:]:
        sessions += rec.read(open(path, "rb").read())
    with open(argv[1], "wb") as out:
        r = write(out, sessions)
    print(f"{argv[1]}: {len(sessions)} unit(s), {r['channels']} channels, {r['steps']} steps "
          f"in {r['frames']} frames of {r['per_frame']}")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
