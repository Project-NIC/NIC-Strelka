"""The live view saved on the phone (APP.md, The live view saved): a working file of the messages as
Povodok sends them (PHONE.md), a period's in one, joined from every Povodok where the set is
gathered by the network, after the SMP frames its export needs; and the C3D made of it, as the
download's, its SOURCE `live`: the raw steps, calibrated by the Belka, in g and °/s by the sensor's
steps (tools/strelka_record.py, convert), or the quaternions.
Plain Python, no numpy.

    python3 tools/strelka_live.py S00007-20261110-170512.live         what it holds
    python3 tools/strelka_live.py --csv S00007-20261110-170512.live out.csv
    python3 tools/strelka_live.py --c3d S00007-20261110-170512.live out.c3d

The file: 8 B, "STRELKA" and the version; then records, each its kind u8, its unit u8 and its length
u16, little-endian, and its bytes unchanged — 1 a live view's message (unit 0), 2 an SMP frame with
Povodok (its number 0–3), 3 an SMP frame with a Belka (1–63). The SMP frames come first: 64/1 the
session as it was sent, 64/18 the set's table as it was sent, and every chosen unit's answers to
64/0 and 64/3, read before the start while SMP still goes over ESB.
"""

import os
import struct
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import cbor2  # noqa: E402
import live_view as lv  # noqa: E402
import smp  # noqa: E402
import strelka_c3d as c3  # noqa: E402
import strelka_record as rec  # noqa: E402

MARK = b"STRELKA"
VERSION = 1
RECORD = struct.Struct("<BBH")
LIVE, SMP_POVODOK, SMP_BELKA = 1, 2, 3
GROUP = 64
SESSION, INFO, CAL, TABLE = 1, 0, 3, 18
OP_READ_RSP, OP_WRITE = 1, 2
HOLD = 2                         # periods a period stays open for the other Povodoks' messages

# The sensors by their WHO_AM_I (64/0's `sensor`; firmware/common/imu_tables.h), their names, and
# the LSMs' steps: the accelerometer's in µg a step by its range, the low-g channel and the
# LSM6DSV320X's high-g one (DS13510 Rev 4 and DS14623 Rev 3, table 3), the gyro's in µdps
# (DS13510 Rev 4, table 3; DS14623 Rev 3, table 3) — firmware/common/imu_tables.c's, which
# tools/test_live.py holds them to. An ICM's step is the range / 32 768 (DS-000347 and DS-000639,
# tables 1 and 2), and so is an LSM's in a range its table has not.
SENSORS = {0x47: "ICM-42688-P", 0xDB: "ICM-42688-P", 0x44: "ICM-42686-P",
           0x70: "LSM6DSV16X", 0x73: "LSM6DSV320X"}
LSM = (0x70, 0x73)
LSM_ACC_UG = {2: 61, 4: 122, 8: 244, 16: 488, 32: 976, 64: 1952, 128: 3904, 256: 7808,
              320: 10417}
LSM_GYRO_UDPS = {125: 4375, 250: 8750, 500: 17500, 1000: 35000, 2000: 70000, 4000: 140000}


def acc_step_ng(whoami, acc_g):
    if whoami in LSM and acc_g in LSM_ACC_UG:
        return LSM_ACC_UG[acc_g] * 1000
    return (acc_g * 1000000000 + 16384) // 32768


def gyro_step_ndps(whoami, gyro_dps):
    if whoami in LSM and gyro_dps in LSM_GYRO_UDPS:
        return LSM_GYRO_UDPS[gyro_dps] * 1000
    return (gyro_dps * 1000000000 + 16384) // 32768


def file_head():
    return MARK + bytes([VERSION])


def record(kind, unit, data):
    return RECORD.pack(kind, unit, len(data)) + bytes(data)


def smp_records(session, table_pieces, units):
    """The SMP frames first in the file: 64/1's body as it was sent, the table's pieces of 64/18 as
    they were sent, and for each unit (n, its 64/0 answer, its 64/3 answer) the two answers."""
    out = record(SMP_POVODOK, 0, smp.frame(OP_WRITE, GROUP, 0, SESSION, session))
    for piece in table_pieces:
        out += record(SMP_POVODOK, 0, smp.frame(OP_WRITE, GROUP, 0, TABLE, piece))
    for n, info, cal in units:
        out += record(SMP_BELKA, n, smp.frame(OP_READ_RSP, GROUP, 0, INFO, info))
        out += record(SMP_BELKA, n, smp.frame(OP_READ_RSP, GROUP, 0, CAL, cal))
    return out


class Joiner:
    """The messages of every Povodok into one a period, as they come: the head from the lead
    Povodok's message, or the first to come where it sent none; each Belka's entry once. The copies
    of an entry are compared, its `by` aside: the same kept once, differing ones settled by the
    larger part, and with none larger than half the step is taken as lost — the card keeps the
    record. A period is written once a message two periods later has come, or at the end; a message
    of a period already written is left out. Each period goes out as Povodok would send it, in one
    message or in two of the same head (live_view.split)."""

    def __init__(self, emit):
        self.emit = emit                  # called with each message's bytes
        self.open = {}                    # period: [head, lead, {n: [entries]}]
        self.done = -1
        self.disagreed = 0                # entries left out, their copies differing

    def add(self, data, lead=True):
        m = lv.parse(data, quats_as_tuples=False)
        p = m["period"]
        if p <= self.done:
            return
        slot = self.open.setdefault(p, [m, lead, {}])
        if lead and not slot[1]:
            slot[0], slot[1] = m, True
        for b in m["belkas"]:
            slot[2].setdefault(b["n"], []).append(b)
        for q in sorted(self.open):
            if q + HOLD <= p:
                self._out(q)

    def finish(self):
        for q in sorted(self.open):
            self._out(q)

    def _out(self, p):
        head, _, belkas = self.open.pop(p)
        self.done = max(self.done, p)
        kept = []
        for n in sorted(belkas):
            copies = belkas[n]
            keys = [(b["s0"], b["state"], tuple(b["steps"])) for b in copies]
            best = max(set(keys), key=keys.count)
            if keys.count(best) * 2 <= len(keys) and len(keys) > 1:
                self.disagreed += 1
                continue
            b = copies[keys.index(best)]
            kept.append((b["n"], b["s0"], b["by"], b["state"], b["steps"]))
        for msg in lv.split(head["session"], p, head["state"], head["s0"], head["povodoks"],
                            head["povodok_states"], kept):
            self.emit(msg)


def read(data):
    """The file into its parts: the session (64/1's body), the table's text, each unit's info and
    calibration by its number, and the live view's messages in their order, parsed."""
    if data[:7] != MARK or data[7] != VERSION:
        raise ValueError("not a saved live view")
    out = dict(session=None, table="", info={}, cal={}, messages=[])
    pieces = {}
    at = 8
    while at < len(data):
        kind, unit, length = RECORD.unpack_from(data, at)
        body = data[at + RECORD.size:at + RECORD.size + length]
        at += RECORD.size + length
        if kind == LIVE:
            out["messages"].append(lv.parse(body, quats_as_tuples=False))
            continue
        op, _, _, size, group, _, cmd = smp.parse_header(body[:8])
        if group != GROUP:
            continue
        value = cbor2.loads(body[8:8 + size]) if size else {}
        if kind == SMP_POVODOK and cmd == SESSION:
            out["session"] = value
        elif kind == SMP_POVODOK and cmd == TABLE:
            pieces[value["off"]] = value["csv"]
        elif kind == SMP_BELKA and cmd == INFO:
            out["info"][unit] = value
        elif kind == SMP_BELKA and cmd == CAL:
            out["cal"][unit] = value
    out["table"] = "".join(pieces[k] for k in sorted(pieces))
    return out


def places(table):
    """Each Belka's place by the set's table, its column `place` (RECORD.md, The set's table)."""
    rows, out, cols = rec.table_rows(table), {}, None
    for r in rows:
        if r[:1] == ["belka"]:
            cols = r
        elif cols and r and r[0].isdigit():
            out[int(r[0])] = dict(zip(cols, r)).get("place", "")
    return out


def heads(saved):
    """Every unit the file names or its messages carry, as a record's head for the conversion and
    the C3D: the session's ranges and N (64/1), its kind, chip and sensor (64/0), its calibration
    (64/3), its place (the table); the sensor's own rate and the time not known, zeros; with the
    session's `hg`, both channels kept, the high-g channel's range, step and calibration, `hg`."""
    s = saved["session"] or {}
    acc, gyro = s.get("acc", 16), s.get("gyro", 2000)
    numbers = set(saved["info"]) | {b["n"] for m in saved["messages"] for b in m["belkas"]}
    at = places(saved["table"])
    kinds = {n: saved["info"].get(n, {}).get("kind", 0) for n in numbers}
    out = {}
    for n in sorted(numbers):
        info, cal = saved["info"].get(n, {}), saved["cal"].get(n, {})
        who = info.get("sensor", 0)
        acc_cal = cal.get("acc", [0] * 6)
        out[n] = dict(session=s.get("s", 0), belka=n, chip=int.from_bytes(info.get("chip", b""),
                      "little"), place=at.get(n, ""), n=s.get("n", 1), acc_g=acc, gyro_dps=gyro,
                      time_us=0, time_period=0, units=kinds, acc_cal=acc_cal,
                      gyro_scale=cal.get("gscale", [0] * 3), gyro_zero=cal.get("gzero", [0] * 3),
                      cal_time=cal.get("t", 0), cal_range=tuple(cal.get("range", (0, 0))),
                      cal_temp=cal.get("temp", 0),
                      sensor=dict(name=SENSORS.get(who, ""), sensor_hz=0, acc_range=acc,
                                  gyro_range=gyro, acc_step_ng=acc_step_ng(who, acc),
                                  gyro_step_ndps=gyro_step_ndps(who, gyro)),
                      hg=None if not s.get("hg") else dict(
                          range=s["hg"], step_ng=acc_step_ng(who, s["hg"]),
                          full=round(acc_step_ng(who, s["hg"]) * 32768 / 1e9),
                          cal=tuple(cal.get("hg", (0,) * 6)), cal_range=cal.get("hgrange", 0)))
    return out


def steps(saved):
    """Each Belka's steps on the set's grid at the live view's rate, index = period × the steps a
    period + j: (the live code, {n: {index: the step}}), a quaternion as (w, x, y, z), a raw step as
    its six i16 and whether its accelerometer's three are the high-g channel's, codes 3 and 6; the
    code the low-g one's, 4 or 5, a period of 3 or 6 among them counted as 4 or 5."""
    code, out = None, {}
    for m in saved["messages"]:
        for b in m["belkas"]:
            if b["live"] not in (lv.QUAT_50, lv.QUAT_100) + lv.RAW_CODES:
                continue
            live = {lv.RAW_50_HG: lv.RAW_50, lv.RAW_100_HG: lv.RAW_100}.get(b["live"], b["live"])
            code = code or live
            spp = lv.STEPS[b["live"]]
            for j, step in enumerate(b["steps"]):
                v = lv.unpack_quat(step) if lv.STEP_BYTES[b["live"]] == 4 else \
                    (step, b["live"] in lv.HG_CODES)
                out.setdefault(b["n"], {})[m["period"] * spp + j] = v
    return code, out


def rate_of(code):
    return 100 if code in (lv.QUAT_100, lv.RAW_100, lv.RAW_100_HG) else 50


def live_convert(h, step):
    """A live raw step into g and °/s by the sensor's steps alone, the Belka having calibrated it:
    its accelerometer's three by the high-g channel's step where the code said so."""
    raw, high_g = step
    v = rec.convert(h, raw)
    if high_g and h.get("hg"):
        v = [x * h["hg"]["step_ng"] * 1e-9 for x in raw[:3]] + v[3:]
    return v


def to_c3d(saved, out):
    """The C3D, as the download's (tools/strelka_c3d.py), its SOURCE `live`: the raw steps in g and
    °/s by the sensor's steps alone, the Belka having calibrated them (PRINCIPLE.md, The LIVE), or
    the quaternions as QW, QX, QY, QZ; a step lost zeros. The calibration rides in the C3D all the
    same, for the reader to see."""
    code, by = steps(saved)
    hs = heads(saved)
    first = min((k for d in by.values() for k in d), default=0)
    quaternions = code in (lv.QUAT_50, lv.QUAT_100)
    units = []
    for n, h in hs.items():
        d = by.get(n, {})
        # the live view's one accelerometer a step, in g whichever channel: no HX, HY, HZ
        units.append((dict(h, hg=None), {k - first: list(v) if quaternions else live_convert(h, v)
                                         for k, v in d.items()}))
    return c3.write_units(out, units, rate_of(code), first, "live", quaternions)


def to_csv(saved, out):
    """CSV by way of the C3D, as the download's CSV is (tools/strelka_c3d.py, to_csv)."""
    import io
    buf = io.BytesIO()
    to_c3d(saved, buf)
    c3.to_csv(buf.getvalue(), out)


def describe(saved):
    s = saved["session"] or {}
    code, by = steps(saved)
    lines = [f"session {s.get('s', '?')}, {len(saved['messages'])} message(s), the live view "
             f"{'raw' if code in lv.RAW_CODES else 'quaternions'} at {rate_of(code)} Hz"]
    for n, h in heads(saved).items():
        d = by.get(n, {})
        lines.append(f"  {c3.kind_letter(h)}{n} {h['sensor']['name'] or '?'} {h['place']!r}: "
                     f"{len(d)} step(s), calibrated {'yes' if h['cal_time'] else 'no'}")
    return "\n".join(lines)


def main(argv):
    if len(argv) == 2:
        print(describe(read(open(argv[1], "rb").read())))
        return 0
    if len(argv) == 4 and argv[1] in ("--csv", "--c3d"):
        saved = read(open(argv[2], "rb").read())
        if argv[1] == "--csv":
            with open(argv[3], "w") as out:
                to_csv(saved, out)
        else:
            with open(argv[3], "wb") as out:
                to_c3d(saved, out)
        return 0
    print(__doc__)
    return 2


if __name__ == "__main__":
    sys.exit(main(sys.argv))
