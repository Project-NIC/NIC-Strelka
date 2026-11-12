"""Strelka's records to C3D, the gait laboratories' format: the units' six channels as analog
channels on one time base, a frame a step; nothing but the data in the record (RECORD.md). Plain
Python, no numpy.

    python3 tools/strelka_c3d.py out.c3d S00007B01.STRK [S00007B02.STRK …]
    python3 tools/strelka_c3d.py --csv S00007.c3d S00007.csv

The second, the C3D's steps as CSV, as the application exports it on the phone (APP.md, The
export): a row a step, its grid index and time, every channel, and a column `lost` a unit, 1 where
its six values are all zero — a step the unit has not, never a reading.

One file a Belka, the files of one session: they must share the session's number and the rate into
memory, and they are laid side by side by the set's grid — the period's number, the block's place
and the step's — so that a Belka a period early or late lines up all the same (PRINCIPLE.md). The
channels are in g and °/s, by the ranges and by each Belka's calibration where its head carries
one, as tools/strelka_record.py converts them; a step lost is zeros, every value, never a reading,
as in the CSV, and so is a step outside a unit's record. C3D's frame count is 16 bits, so a long
record is folded: several steps a frame, ANALOG:RATE the steps' rate, POINT:RATE the frames'. A
channel is named by its unit's kind and number, B3_AX, K7_GZ, by the session's units in the heads;
the group STRELKA carries the heads: SOURCE `card` for a record's, `live` for the live view saved
(tools/strelka_live.py, whose quaternions are QW, QX, QY, QZ); each unit's sensor's steps and its
calibration as the head has them, raw, so that the raw steps can be computed back from the values,
to the count (raw_back).
"""

import math
import os
import struct
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import strelka_record as rec  # noqa: E402

AXES = ("AX", "AY", "AZ", "GX", "GY", "GZ")
UNITS = ("g", "g", "g", "dps", "dps", "dps")
QUATS = ("QW", "QX", "QY", "QZ")
HG_AXES = ("HX", "HY", "HZ")
MAX_FRAMES = 65535
PROCESSOR_INTEL = 84


def channels(session, first):
    """The session's steps as six lists of physical values, index = the step's place on the set's
    grid less `first`, the grid index the set's frames start at."""
    head = session["head"]
    cal = rec.calibration(head)
    out = {}
    for k, blk, j, raw in rec.steps(session):
        if tuple(raw) != rec.MISSING:        # a step lost left out: zeros
            out[k - first] = rec.convert(head, raw, cal)
    if head.get("hg"):                       # the high-g channel's three after the six
        hcal = rec.hg_calibration(head)
        hg = dict(rec.hg_steps(session))
        for i in list(out):
            v = hg.get(i + first, rec.HG_MISSING)
            out[i] = out[i] + ([0.0] * 3 if tuple(v) == rec.HG_MISSING
                               else rec.hg_convert(head, v, hcal))
    return out


def kind_letter(head):
    """The unit's kind by the session's units in its head: B, K or Z; B when the head knows none."""
    return rec.KINDS.get(head.get("units", {}).get(head["belka"], 0), "B")


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


def write(out, sessions, source="card"):
    if not sessions:
        raise SystemExit("no session")
    rates = {s["head"]["rate_hz"] for s in sessions}
    numbers = {s["head"]["session"] for s in sessions}
    if len(rates) != 1 or len(numbers) != 1:
        raise SystemExit("the files differ in their session or their rate: not one session's")
    rate = rates.pop()
    first = first_index(sessions)
    return write_units(out, [(s["head"], channels(s, first)) for s in sessions], rate, first,
                       source)


def write_units(out, units, rate, first, source="card", quaternions=False):
    """The C3D of units given as (head, {index: six values, or a quaternion's four}), index the
    step's place on the set's grid less `first`: the records' heads, or a saved live view's
    (tools/strelka_live.py)."""
    heads = [h for h, _ in units]
    data = [d for _, d in units]
    def axes_of(h):
        if quaternions:
            return QUATS, ("1",) * 4
        return (AXES + HG_AXES, UNITS + ("g",) * 3) if h.get("hg") else (AXES, UNITS)
    widths = [len(axes_of(h)[0]) for h in heads]
    steps = max((max(d) + 1 if d else 0) for d in data)
    per_frame = max(1, math.ceil(steps / MAX_FRAMES))
    frames = math.ceil(steps / per_frame) if steps else 1
    n_ch = sum(widths)
    labels = [f"{kind_letter(h)}{h['belka']}_{a}" for h in heads for a in axes_of(h)[0]]
    units_ = [u for h in heads for u in axes_of(h)[1]]

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
    p.strings("ANALOG", "UNITS", units_, 4)
    p.float32("ANALOG", "RATE", float(rate))
    p.string("ANALOG", "FORMAT", "SIGNED")
    p.int16("ANALOG", "BITS", 16)
    p.group("STRELKA", "the records' heads")
    p.uint16("STRELKA", "SESSION", heads[0]["session"])
    p.int16s("STRELKA", "BELKA", [h["belka"] for h in heads])
    p.strings("STRELKA", "CHIP", [f"{h['chip']:016X}" for h in heads], 16)
    p.strings("STRELKA", "PLACE", [h["place"] for h in heads], 32)
    p.int16s("STRELKA", "N", [h["n"] for h in heads])
    p.int16s("STRELKA", "SENSOR_HZ", [(h.get("sensor") or {}).get("sensor_hz", 0)
                                      for h in heads])
    p.int16s("STRELKA", "ACC_G", [h["acc_g"] for h in heads])
    p.int16s("STRELKA", "GYRO_DPS", [h["gyro_dps"] for h in heads])
    p.strings("STRELKA", "SENSOR", [(h.get("sensor") or {}).get("name", "")
                                    for h in heads], 16)
    p.strings("STRELKA", "TIME_US", [str(h["time_us"]) for h in heads], 20)
    p.strings("STRELKA", "TIME_PERIOD", [str(h["time_period"]) for h in heads], 12)
    p.string("STRELKA", "FIRST_GRID_INDEX", str(first))
    p.int16("STRELKA", "STEPS_PER_FRAME", per_frame)
    p.string("STRELKA", "STEPS", str(steps))
    p.string("STRELKA", "SOURCE", source)
    sensors = [h.get("sensor") or {} for h in heads]
    p.int16s("STRELKA", "ACC_RANGE", [sn.get("acc_range") or h["acc_g"]
                                      for sn, h in zip(sensors, heads)])
    p.int16s("STRELKA", "GYRO_RANGE", [sn.get("gyro_range") or h["gyro_dps"]
                                       for sn, h in zip(sensors, heads)])
    p.strings("STRELKA", "ACC_STEP_NG", [str(sn.get("acc_step_ng", 0)) for sn in sensors], 12)
    p.strings("STRELKA", "GYRO_STEP_NDPS", [str(sn.get("gyro_step_ndps", 0)) for sn in sensors], 12)
    p.strings("STRELKA", "CAL_TIME", [str(h["cal_time"]) for h in heads], 12)
    p.int16s("STRELKA", "CAL_ACC", [v for h in heads for v in h["acc_cal"]])
    p.floats("STRELKA", "CAL_GSCALE", [float(v) for h in heads for v in h["gyro_scale"]])
    p.int16s("STRELKA", "CAL_GZERO", [v for h in heads for v in h["gyro_zero"]])
    p.int16s("STRELKA", "CAL_RANGE", [v for h in heads for v in h.get("cal_range", (0, 0))])
    p.int16s("STRELKA", "CAL_TEMP", [h.get("cal_temp", 0) for h in heads])
    p.int16s("STRELKA", "HG_RANGE", [(h.get("hg") or {}).get("range", 0) for h in heads])
    p.strings("STRELKA", "HG_STEP_NG", [str((h.get("hg") or {}).get("step_ng", 0)) for h in heads],
              12)
    p.int16s("STRELKA", "CAL_HG", [v for h in heads for v in (h.get("hg") or {}).get("cal", (0,) * 6)])
    p.int16s("STRELKA", "CAL_HG_RANGE", [(h.get("hg") or {}).get("cal_range", 0) for h in heads])
    # POINT:DATA_START needs the section's own size: packed to measure, again with the figure in,
    # until the block count holds (the parameter itself may push it over a block's edge)
    data_start = 2 + len(p.pack()) // 512
    while True:
        p.int16("POINT", "DATA_START", data_start)
        params = p.pack()
        if data_start == 2 + len(params) // 512:
            break
        data_start = 2 + len(params) // 512

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
    zeros = [[0.0] * w for w in widths]
    for f in range(frames):
        for sub in range(per_frame):
            k = f * per_frame + sub
            for d, w, zero in zip(data, widths, zeros):
                body += struct.pack(f"<{w}f", *d.get(k, zero))
    body += b"\x00" * ((512 - len(body) % 512) % 512)
    out.write(bytes(header) + params + bytes(body))
    return dict(frames=frames, per_frame=per_frame, channels=n_ch, steps=steps)


def read(data):
    """A C3D this file wrote, read back: its labels, rate, first grid index and steps, and every
    step's channels — the parameters by C3D's own records, the analog samples as float32."""
    params, groups = {}, {}
    pos = 512 * (data[0] - 1) + 4
    records = []
    while True:
        nlen, gid = struct.unpack_from("<bb", data, pos)
        if nlen == 0:
            break
        name = data[pos + 2:pos + 2 + abs(nlen)].decode()
        at = pos + 2 + abs(nlen)
        (offset,) = struct.unpack_from("<h", data, at)
        records.append((gid, name, at + 2))
        if gid < 0:
            groups[-gid] = name
        if offset == 0:
            break
        pos = at + offset
    for gid, name, at in records:
        if gid < 0:
            continue
        kind, ndims = struct.unpack_from("<bB", data, at)
        dims = list(data[at + 2:at + 2 + ndims])
        body = at + 2 + ndims
        size = abs(kind)
        for d in dims:
            size *= d
        raw = data[body:body + size]
        if kind == -1:
            width = dims[0] if dims else len(raw)
            value = [raw[i:i + width].decode().rstrip() for i in range(0, len(raw), width)] \
                if len(dims) > 1 else raw.decode().rstrip()
        elif kind == 2:
            value = list(struct.unpack(f"<{size // 2}h", raw))
        else:
            value = list(struct.unpack(f"<{size // 4}f", raw))
        params[(groups[gid], name)] = value
    labels = params[("ANALOG", "LABELS")]
    n_ch = len(labels)
    per_frame = struct.unpack_from("<H", data, 18)[0]
    start = 512 * (struct.unpack_from("<H", data, 16)[0] - 1)
    steps = int(params[("STRELKA", "STEPS")])
    flat = struct.unpack_from(f"<{steps * n_ch}f", data, start)
    rows = [flat[k * n_ch:(k + 1) * n_ch] for k in range(steps)]
    return dict(labels=labels, rate=params[("ANALOG", "RATE")][0],
                first=int(params[("STRELKA", "FIRST_GRID_INDEX")]), steps=steps,
                per_frame=per_frame, rows=rows, params=params)


def unit_channels(labels):
    """The units in the order of the channels, each (its label, the first channel, how many)."""
    out = []
    for i, label in enumerate(labels):
        unit = label.rsplit("_", 1)[0]
        if out and out[-1][0] == unit:
            out[-1][2] += 1
        else:
            out.append([unit, i, 1])
    return out


def to_csv(data, out):
    """The C3D's steps as CSV: the grid index, the time from the first step, every channel — g and
    a quaternion's components to six places, °/s to four — and a unit's `lost`, 1 where its values
    are all zero."""
    c = read(data)
    units = unit_channels(c["labels"])
    out.write(",".join(["step", "t_s"] + c["labels"] + [u + "_lost" for u, _, _ in units]) + "\n")
    for k, row in enumerate(c["rows"]):
        values = [f"{v:.4f}" if c["labels"][i].rsplit("_", 1)[1].startswith("G") else f"{v:.6f}"
                  for i, v in enumerate(row)]
        lost = [str(int(all(v == 0.0 for v in row[at:at + n]))) for _, at, n in units]
        out.write(",".join([str(c["first"] + k), f"{k / c['rate']:.6f}"] + values + lost) + "\n")


def raw_back(data):
    """The raw steps computed back from a C3D of raw steps, by the ranges, the sensor's steps and
    the calibration its group STRELKA carries — the inverse of tools/strelka_record.py's convert:
    for each unit by its label, a step's six i16, or None for a step lost. From a card's C3D the
    sensor's own; from a live view's, the steps as the Belka sent them, calibrated."""
    c = read(data)
    p = lambda name: c["params"][("STRELKA", name)]  # noqa: E731
    out = {}
    for u, (label, at, _) in enumerate(unit_channels(c["labels"])):
        def of(name, n):
            return p(name)[n * u:n * u + n]
        sensor = dict(acc_range=p("ACC_RANGE")[u], gyro_range=p("GYRO_RANGE")[u],
                      acc_step_ng=int(p("ACC_STEP_NG")[u]),
                      gyro_step_ndps=int(p("GYRO_STEP_NDPS")[u]))
        head = dict(acc_g=p("ACC_G")[u], gyro_dps=p("GYRO_DPS")[u],
                    cal_time=int(p("CAL_TIME")[u]), acc_cal=of("CAL_ACC", 6),
                    gyro_scale=of("CAL_GSCALE", 3), gyro_zero=of("CAL_GZERO", 3),
                    cal_range=tuple(of("CAL_RANGE", 2)), sensor=sensor)
        # the live view's steps the Belka calibrated, converted by the steps alone: back to them
        cal = None if c["params"][("STRELKA", "SOURCE")] == "live" else rec.calibration(head)
        acc, gyro = head["acc_g"] / 32768, head["gyro_dps"] / 32768
        if sensor["acc_step_ng"] and sensor["gyro_step_ndps"]:
            acc, gyro = sensor["acc_step_ng"] * 1e-9, sensor["gyro_step_ndps"] * 1e-9
        steps = []
        for row in c["rows"]:
            v = row[at:at + 6]
            if all(x == 0.0 for x in v):
                steps.append(None)
            elif cal is None:
                steps.append(tuple(round(x / acc) for x in v[:3]) +
                             tuple(round(x / gyro) for x in v[3:]))
            else:
                steps.append(
                    tuple(round(v[i] * cal["acc_per_g"][i] + cal["acc_zero"][i])
                          for i in range(3)) +
                    tuple(round(v[3 + i] * cal["gyro_scale"][i] / gyro + cal["gyro_zero"][i])
                          for i in range(3)))
        out[label] = steps
    return out


def main(argv):
    if len(argv) == 4 and argv[1] == "--csv":
        with open(argv[3], "w") as out:
            to_csv(open(argv[2], "rb").read(), out)
        return 0
    if len(argv) < 3:
        print(__doc__)
        return 2
    sessions = []
    for path in argv[2:]:
        sessions.append(rec.read(open(path, "rb").read()))
    with open(argv[1], "wb") as out:
        r = write(out, sessions)
    print(f"{argv[1]}: {len(sessions)} unit(s), {r['channels']} channels, {r['steps']} steps "
          f"in {r['frames']} frames of {r['per_frame']}")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
