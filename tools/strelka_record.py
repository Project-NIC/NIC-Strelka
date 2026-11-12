"""NIC-Strelka — a Belka's record, read, written and exported.

The record is RECORD.md's: a session is one file of blocks of 512 B — the session's head with its
table of the data's groups, the set's table, data blocks of 40 steps with an information block after
every 120 steps and, where the high-g channel is kept, its two blocks of 60 steps after that, and the
end — every number little-endian, every block ending in a CRC-32 (IEEE, zlib's) over the 508 bytes
before it.
This reads such a file, lists it, writes it as CSV, and builds its blocks, so that the firmware and
the application have one reference for both directions.

    python3 tools/strelka_record.py session.bin            the head, the table, the blocks, the end
    python3 tools/strelka_record.py session.bin out.csv    the steps as CSV
"""

import csv
import math
import io
import struct
import sys
import zlib

BLOCK = 512
STEPS = 40                       # steps in a data block
INFO_STEPS = 120                 # steps an information block speaks for: three data blocks
STEP_BYTES = 12
PERIOD_S = 0.020
HG_STEPS = 60                    # steps in a high-g block: two to a cycle of 120
KIND_HEAD, KIND_DATA, KIND_END, KIND_TABLE, KIND_INFO, KIND_HG = 1, 2, 3, 4, 5, 6
VERSION = 4
MISSING = (-32768,) * 6          # a step lost: its six values
CLOCKS = {0: "steered", 1: "the fallback", 2: "the sensor's own, drawn onto the grid"}
NO_SIGNAL = -128                 # the SYNC's signal when none was heard
END_WHY = {0: "unfinished", 1: "the SYNC's state said stop", 2: "switched off by magnet A",
           3: "the cell low", 4: "the card full", 5: "a fault",
           6: "switched off by the application", 7: "the set silent, every Povodok gone"}

HEAD = struct.Struct("<4sBBHBQ3sHBBQIb16shIB4x")                # the fixed part, 64 B
GROUP = struct.Struct("<BBBB8s4sIHH6H")                          # a group of the data, 36 B
GROUPS_AT, GROUPS_MAX = 64, 12
TYPE_I16, TYPE_I8, TYPE_U8 = 1, 2, 3
TYPE_FORMAT = {TYPE_I16: "h", TYPE_I8: "b", TYPE_U8: "B"}
KINDS = {0: "B", 1: "K", 2: "Z"}  # a Belka, Kryska, Zvjozdočka: 64/0's `kind`, the table's
SENSOR_KEYS = ("id", "name", "sheet", "acc_channel", "acc_range", "acc_step_ng", "gyro_range",
               "gyro_step_ndps", "sensor_hz")
TABLE = struct.Struct("<BBHH")                                 # 6 B, then 502 B of text
TABLE_TEXT = 502
DATA = struct.Struct("<BBHIIBBBBi")                            # 20 B, 8 free, the steps at 28
INFO = struct.Struct("<BBHIIhH")                               # 16 B, 12 free, the entries at 28
INFO_ENTRY = struct.Struct("<hbB")                             # 4 B a step
HG = struct.Struct("<BBHIIBBB")                                # 15 B, the steps at 28
HG_STEP = struct.Struct("<3h")
HG_MISSING = (-32768,) * 3
END = struct.Struct("<BBHIIII")                                # 20 B, the rest free
STEP = struct.Struct("<6h")
PAYLOAD = 28                     # where a data or information block's entries begin


# --- the blocks, built -------------------------------------------------------------------------

def seal(body):
    """A block from its body: padded to 508 B with zeros, the CRC-32 after."""
    assert len(body) <= BLOCK - 4, len(body)
    body = body.ljust(BLOCK - 4, b"\x00")
    return body + struct.pack("<I", zlib.crc32(body))


def block_ok(block):
    return len(block) == BLOCK and zlib.crc32(block[:-4]) == struct.unpack("<I", block[-4:])[0]


def head_block(session, belka, chip, fw=(0, 0, 0), rate_hz=100, n=1, clock=0, time_us=0,
               time_period=0, tz_min=0, sensor="", cal_temp=0, cal_time=0, groups=()):
    """The session's head, version 4 (RECORD.md, The session's head): the fixed part, then the
    table of the data's groups, each a dict of kind, first, count, type, name, unit, step (nano-
    units), range, cal_range and cal (six 16-bit words); the time zone kept in quarter hours."""
    assert len(groups) <= GROUPS_MAX
    body = HEAD.pack(b"STRK", VERSION, KIND_HEAD, session, belka, chip, bytes(fw), rate_hz, n,
                     clock, time_us, time_period, int(tz_min / 15),
                     sensor.encode("ascii")[:16], cal_temp, cal_time, len(groups))
    for g in groups:
        body += GROUP.pack(g["kind"], g["first"], g["count"], g["type"],
                           g["name"].encode("ascii")[:8], g["unit"].encode("ascii")[:4],
                           g.get("step", 0), g.get("range", 0), g.get("cal_range", 0),
                           *[v & 0xFFFF for v in g.get("cal", (0,) * 6)])
    return seal(body)


def imu_groups(acc_range, acc_step_ng, gyro_range, gyro_step_ndps, acc_cal=(0,) * 6,
               gyro_zero=(0,) * 3, gyro_scale=(32768,) * 3, cal_range=(0, 0), high_g=False,
               hg=None):
    """A Belka's groups (RECORD.md, The groups): the accelerometer and the gyro in the data blocks
    — the accelerometer named ACC_HG when it is the high-g channel's, high_g — each step's offset,
    SYNC and radio in the information blocks, and with hg, a dict of range, step_ng, cal and
    cal_range, the high-g channel in its own blocks."""
    out = [dict(kind=KIND_DATA, first=0, count=3, type=TYPE_I16, name="ACC_HG" if high_g else "ACC",
                unit="g", step=acc_step_ng, range=acc_range, cal_range=cal_range[0],
                cal=tuple(acc_cal)),
           dict(kind=KIND_DATA, first=3, count=3, type=TYPE_I16, name="GYRO", unit="dps",
                step=gyro_step_ndps, range=gyro_range, cal_range=cal_range[1],
                cal=tuple(gyro_zero) + tuple(gyro_scale)),
           dict(kind=KIND_INFO, first=0, count=1, type=TYPE_I16, name="OFFSET", unit="ns",
                step=10 ** 9),
           dict(kind=KIND_INFO, first=1, count=1, type=TYPE_I8, name="SIGNAL", unit="dBm",
                step=10 ** 9),
           dict(kind=KIND_INFO, first=2, count=1, type=TYPE_U8, name="RADIO", unit="", step=0)]
    if hg:
        out.append(dict(kind=KIND_HG, first=0, count=3, type=TYPE_I16, name="ACC_HG", unit="g",
                        step=hg["step_ng"], range=hg["range"], cal_range=hg.get("cal_range", 0),
                        cal=tuple(hg.get("cal", (0,) * 6))))
    return out


def units_bytes(units):
    """The session's units as 64/1 carries them: a bit a unit, then 2 bits each for its kind
    (COMMANDS.md, 64/1)."""
    mask, kinds = 0, 0
    for n, kind in units.items():
        mask |= 1 << n
        kinds |= kind << (2 * n)
    return mask.to_bytes(8, "little"), kinds.to_bytes(16, "little")


def units_of(mask, kinds):
    """The session's units read back from 64/1: each unit's number and kind."""
    m, k = int.from_bytes(mask, "little"), int.from_bytes(kinds, "little")
    return {n: (k >> (2 * n)) & 3 for n in range(64) if m >> n & 1}


def table_blocks(session, text):
    """The set's table, CSV text, in as many blocks as it needs (RECORD.md, The set's table)."""
    raw = text.encode("utf-8")
    parts = [raw[i:i + TABLE_TEXT] for i in range(0, len(raw), TABLE_TEXT)] or [b""]
    return [seal(TABLE.pack(KIND_TABLE, i, session, len(raw)) + part)
            for i, part in enumerate(parts)]


def data_block(session, number, period, steps, place=0, offset_ns=0, acc_out=0, gyro_out=0):
    """A data block: up to 40 steps of six i16, a lost one as MISSING (RECORD.md, A data block)."""
    assert 0 < len(steps) <= STEPS
    missing = sum(1 for s in steps if tuple(s) == MISSING)
    body = DATA.pack(KIND_DATA, len(steps), session, number, period, place, missing, acc_out,
                     gyro_out, offset_ns)
    body = body.ljust(PAYLOAD, b"\x00") + b"".join(STEP.pack(*s) for s in steps)
    return seal(body)


def info_block(session, number, period, entries, temp_centi=0, cell_mv=0):
    """An information block: for each of up to 120 steps (offset_ns, signal_dbm, povodok, syncs),
    the Povodok 0-3 or None, the SYNCs heard 0-4 (RECORD.md, An information block)."""
    assert 0 < len(entries) <= INFO_STEPS
    body = INFO.pack(KIND_INFO, len(entries), session, number, period, temp_centi, cell_mv)
    body = body.ljust(PAYLOAD, b"\x00")
    for offset_ns, signal, povodok, syncs in entries:
        who = ((0xF if povodok is None else povodok & 0xF) << 4) | (syncs & 0xF)
        body += INFO_ENTRY.pack(offset_ns, NO_SIGNAL if signal is None else signal, who)
    return seal(body)


def hg_block(session, number, period, place, half, steps):
    """A high-g block: up to 60 steps of the high-g channel's x, y, z, the cycle of 120 steps its
    data block numbered `number` begins, at that block's period and place, its first or second
    half; it takes no number of its own (RECORD.md, A high-g block)."""
    assert 0 < len(steps) <= HG_STEPS
    out = sum(1 for v in steps if tuple(v) != HG_MISSING and (32767 in v or -32768 in v))
    body = HG.pack(KIND_HG, len(steps), session, number, period, place, half, min(out, 255))
    return seal(body.ljust(PAYLOAD, b"\x00") + b"".join(HG_STEP.pack(*v) for v in steps))


def compose(head, table, first_period, steps, step_of, entry_of, hg_of=None,
            offset_of=lambda first: 0, temp_centi=0, cell_mv=0, why=1, dropped=0):
    """A session's file as a Belka composes it (firmware/common/session.c): the head given, the
    set's table, data blocks of 40 steps, after every 120 steps their information block and, with
    hg_of, the high-g channel's two blocks of 60, and the end. step_of(k) step k's six, MISSING a
    lost one; entry_of(k) its (offset_ns, signal, povodok, syncs); hg_of(k) its high-g three;
    offset_of(first) a data block's first step's offset. A list of the blocks."""
    h = parse_head(head)
    session, spp = h["session"], steps_a_period(h)
    blocks = [head] + (table_blocks(session, table) if table is not None else [])
    number = 0
    cycle_number = 0
    for first in range(0, steps, STEPS):
        n = min(STEPS, steps - first)
        if first % INFO_STEPS == 0:
            cycle_number = number
        blocks.append(data_block(session, number, first_period + first // spp,
                                 [step_of(k) for k in range(first, first + n)],
                                 place=(first % spp) // STEPS, offset_ns=offset_of(first)))
        number += 1
        if (first + n) % INFO_STEPS == 0 or first + n == steps:
            start = (first + n - 1) // INFO_STEPS * INFO_STEPS
            blocks.append(info_block(session, number, first_period + start // spp,
                                     [entry_of(k) for k in range(start, first + n)],
                                     temp_centi=temp_centi, cell_mv=cell_mv))
            number += 1
            if hg_of is not None:
                for half in (0, 1):
                    a = start + half * HG_STEPS
                    b = min(a + HG_STEPS, first + n)
                    if a < b:
                        blocks.append(hg_block(session, cycle_number,
                                               first_period + start // spp,
                                               (start % spp) // STEPS, half,
                                               [hg_of(k) for k in range(a, b)]))
    blocks.append(end_block(session, why, steps, number, first_period + (steps - 1) // spp,
                            dropped=dropped))
    return blocks


def end_block(session, why, steps, blocks, last_period, dropped=0):
    return seal(END.pack(KIND_END, why, session, steps, blocks, last_period, dropped))


# --- the blocks, read --------------------------------------------------------------------------

def _signed(v):
    return v - 0x10000 if v >= 0x8000 else v


def parse_head(block):
    """The head, version 4, into its fields, its groups, and what the tools take of them: the
    accelerometer and the gyro of the data blocks, their ranges, steps and calibration, and the
    high-g channel's where the record keeps it (`hg`); the place and the session's units filled
    from the set's table by read()."""
    (mark, version, kind, session, belka, chip, fw, rate, n, clock, time_us, time_period, tz_q,
     sensor, cal_temp, cal_time, count) = HEAD.unpack_from(block)
    if mark != b"STRK" or kind != KIND_HEAD:
        return None
    if version != VERSION:
        raise ValueError(f"the record's format {version}: this reads version {VERSION}")
    groups = []
    for i in range(min(count, GROUPS_MAX)):
        (g_kind, first, cnt, typ, name, unit, step, rng, cal_range, *cal) = GROUP.unpack_from(
            block, GROUPS_AT + i * GROUP.size)
        groups.append(dict(kind=g_kind, first=first, count=cnt, type=typ,
                           name=name.rstrip(b"\x00").decode("ascii", "replace"),
                           unit=unit.rstrip(b"\x00").decode("ascii", "replace"), step=step,
                           range=rng, cal_range=cal_range, cal=tuple(cal)))
    data = [g for g in groups if g["kind"] == KIND_DATA]
    acc = next((g for g in data if g["unit"] == "g"), None) or dict(
        name="ACC", step=0, range=0, cal_range=0, cal=(0,) * 6)
    gyro = next((g for g in data if g["unit"] == "dps"), None) or dict(
        step=0, range=0, cal_range=0, cal=(0,) * 3 + (32768,) * 3)
    full = lambda g: round(g["step"] * 32768 / 1e9) if g["step"] else g["range"]  # noqa: E731
    hgg = next((g for g in groups if g["kind"] == KIND_HG), None)
    hg = None if hgg is None else dict(
        range=hgg["range"], step_ng=hgg["step"], full=full(hgg), cal_range=hgg["cal_range"],
        cal=tuple(_signed(v) for v in hgg["cal"]))
    sensor_name = sensor.rstrip(b"\x00").decode("ascii", "replace")
    return dict(version=version, session=session, belka=belka, chip=chip,
                fw=f"{fw[0]}.{fw[1]}.{fw[2]}", rate_hz=rate, n=n, clock=clock, time_us=time_us,
                time_period=time_period, tz_min=tz_q * 15, cal_temp=cal_temp, cal_time=cal_time,
                groups=groups, acc_g=full(acc), gyro_dps=full(gyro),
                acc_cal=tuple(_signed(v) for v in acc["cal"]),
                gyro_zero=tuple(_signed(v) for v in gyro["cal"][:3]),
                gyro_scale=tuple(gyro["cal"][3:]),
                cal_range=(acc["cal_range"], gyro["cal_range"]),
                sensor=dict(id=0, name=sensor_name, sheet="",
                            acc_channel=1 if acc["name"] == "ACC_HG" else 0,
                            acc_range=acc["range"], acc_step_ng=acc["step"],
                            gyro_range=gyro["range"], gyro_step_ndps=gyro["step"], sensor_hz=0),
                hg=hg, place="", units={})


def parse_table(block):
    kind, part, session, length = TABLE.unpack_from(block)
    return dict(part=part, session=session, length=length,
                bytes=block[TABLE.size:TABLE.size + TABLE_TEXT])


def parse_data(block):
    kind, n, session, number, period, place, missing, acc_out, gyro_out, offset = \
        DATA.unpack_from(block)
    steps = [STEP.unpack_from(block, PAYLOAD + i * STEP_BYTES) for i in range(n)]
    return dict(kind=kind, session=session, number=number, period=period, place=place,
                missing=missing, acc_out=acc_out, gyro_out=gyro_out, offset_ns=offset,
                steps=steps)


def parse_info(block):
    kind, n, session, number, period, temp, cell = INFO.unpack_from(block)
    entries = []
    for i in range(n):
        offset, signal, who = INFO_ENTRY.unpack_from(block, PAYLOAD + i * INFO_ENTRY.size)
        entries.append(dict(offset_ns=offset, signal_dbm=None if signal == NO_SIGNAL else signal,
                            povodok=None if who >> 4 == 0xF else who >> 4, syncs=who & 0xF))
    return dict(kind=kind, session=session, number=number, period=period, temp_centi=temp,
                cell_mv=cell, entries=entries)


def parse_hg(block):
    kind, n, session, number, period, place, half, out = HG.unpack_from(block)
    steps = [HG_STEP.unpack_from(block, PAYLOAD + i * HG_STEP.size) for i in range(n)]
    return dict(kind=kind, session=session, number=number, period=period, place=place, half=half,
                out=out, steps=steps)


def parse_end(block):
    kind, why, session, steps, blocks, last_period, dropped = END.unpack_from(block)
    return dict(why=why, why_text=END_WHY.get(why, f"{why}"), session=session, steps=steps,
                blocks=blocks, last_period=last_period, dropped=dropped)


def table_text(parts):
    """The set's table from its parts: the text, or None where a part is missing."""
    if not parts:
        return None
    length = parts[0]["length"]
    by_part = {p["part"]: p["bytes"] for p in parts}
    n = max(1, -(-length // TABLE_TEXT))
    if any(i not in by_part for i in range(n)):
        return None
    return b"".join(by_part[i] for i in range(n))[:length].decode("utf-8", "replace")


def table_rows(text):
    """The table's lines as CSV rows, semicolons parting the fields (RECORD.md)."""
    return list(csv.reader(io.StringIO(text), delimiter=";")) if text else []


def read(data):
    """One session's file: head, table (its text and rows), data and info blocks in the order of
    their numbers, end (None if the file is cut off), the block numbers missing, how many blocks
    failed their CRC, and how many were another session's. Blocks whose CRC fails are skipped, as
    a reader must after a power cut, and so are whole blocks of another session, which an earlier
    file left in the room the file took at its start (RECORD.md, The end)."""
    head = None
    tables, blocks, ends, hgs = [], [], [], []
    bad = 0
    for at in range(0, len(data) - BLOCK + 1, BLOCK):
        block = data[at:at + BLOCK]
        if not block_ok(block):
            bad += 1
            continue
        if block[:4] == b"STRK":
            head = head or parse_head(block)
            continue
        kind = block[0]
        if kind == KIND_TABLE:
            tables.append(parse_table(block))
        elif kind == KIND_DATA:
            blocks.append(parse_data(block))
        elif kind == KIND_INFO:
            blocks.append(parse_info(block))
        elif kind == KIND_END:
            ends.append(parse_end(block))
        elif kind == KIND_HG:
            hgs.append(parse_hg(block))
    if head is None:
        raise ValueError("no head: not a Strelka record")
    own = head["session"]
    foreign = sum(1 for b in tables + blocks + ends + hgs if b["session"] != own)
    tables = [b for b in tables if b["session"] == own]
    hgs = [b for b in hgs if b["session"] == own]
    blocks = [b for b in blocks if b["session"] == own]
    own_ends = [b for b in ends if b["session"] == own]
    end = own_ends[0] if own_ends else None         # the file's own; a later one another's
    blocks.sort(key=lambda b: b["number"])
    numbers = [b["number"] for b in blocks]
    # the first data block is 0 (RECORD.md): blocks lost before the first kept count too
    gaps = sorted(set(range(0, numbers[-1] + 1)) - set(numbers)) if numbers else []
    text = table_text(tables)
    rows = table_rows(text)
    head["units"], place = table_units(rows)
    head["place"] = place.get(head["belka"], "")
    return dict(head=head, table=text, rows=rows,
                data=[b for b in blocks if b["kind"] == KIND_DATA],
                info=[b for b in blocks if b["kind"] == KIND_INFO], hg=hgs, end=end, gaps=gaps,
                bad=bad, foreign=foreign)


def table_units(rows):
    """The session's units from the set's table, each one's kind by its letter in the column
    `kind`, B where none; and each one's place (RECORD.md, The set's table)."""
    letters = {v: k for k, v in KINDS.items()}
    cols, units, place = None, {}, {}
    for r in rows:
        if r[:1] == ["belka"]:
            cols = r
        elif cols and r and r[0].isdigit():
            row = dict(zip(cols, r))
            units[int(r[0])] = letters.get(row.get("kind", "B"), 0)
            place[int(r[0])] = row.get("place", "")
    return units, place


# --- the steps on the set's grid ---------------------------------------------------------------

def steps_a_period(head):
    """How many steps fall in one period at the record's rate: 1 at 50 Hz, 640 at 32 kHz."""
    return max(1, round(head["rate_hz"] * PERIOD_S))


def grid_index(head, block, j):
    """Step j of a data block on the set's grid: the steps since period 0 (PRINCIPLE.md, The clock
    loop) — the period of the block's first step, the block's place in the period, then j."""
    return block["period"] * steps_a_period(head) + block["place"] * STEPS + j


def calibration(head):
    """From the head's raw readings: the accelerometer's zero and its raw units a g, each axis;
    the gyro's zero and scale. None where the head carries no calibration. Made in other ranges
    than the session's, the readings are taken by the nominal steps: times the calibration's
    range over the session's (RECORD.md)."""
    if not head["cal_time"]:
        return None
    sn = head.get("sensor") or {}
    ka, kg = (cal / (sn.get(key) or own) if cal else 1.0 for cal, key, own in
              ((head.get("cal_range", (0, 0))[0], "acc_range", head["acc_g"]),
               (head.get("cal_range", (0, 0))[1], "gyro_range", head["gyro_dps"])))
    c = [v * ka for v in head["acc_cal"]]
    zero = [(c[2 * i] + c[2 * i + 1]) / 2 for i in range(3)]
    per_g = [(c[2 * i] - c[2 * i + 1]) / 2 or 1.0 for i in range(3)]
    return dict(acc_zero=zero, acc_per_g=per_g, gyro_zero=[v * kg for v in head["gyro_zero"]],
                gyro_scale=[s / 32768 or 1.0 for s in head["gyro_scale"]])


def convert(head, raw, cal=None):
    """Six raw i16 into g and °/s: by the ranges, and by the calibration where there is one; a sum
    of N is stored as its mean, so N changes nothing here (APP.md, The conversion)."""
    acc = head["acc_g"] / 32768
    gyro = head["gyro_dps"] / 32768
    sn = head.get("sensor")
    if sn and sn["acc_step_ng"] and sn["gyro_step_ndps"]:
        acc = sn["acc_step_ng"] * 1e-9          # the sheet's own step, where the head gives it
        gyro = sn["gyro_step_ndps"] * 1e-9
    if cal is None:
        return [v * acc for v in raw[:3]] + [v * gyro for v in raw[3:]]
    a = [(raw[i] - cal["acc_zero"][i]) / cal["acc_per_g"][i] for i in range(3)]
    g = [(raw[3 + i] - cal["gyro_zero"][i]) * gyro / cal["gyro_scale"][i] for i in range(3)]
    return a + g


def hg_steps(session):
    """Every high-g step as (grid index, its x, y, z); a lost one as HG_MISSING. A block's first
    step is its cycle's first, by the period and the place it carries, and its half's 60 after."""
    spp = steps_a_period(session["head"])
    for b in session["hg"]:
        start = b["period"] * spp + b["place"] * STEPS + b["half"] * HG_STEPS
        for j, raw in enumerate(b["steps"]):
            yield start + j, raw


def hg_calibration(head):
    """The high-g channel's zero and raw units a g, each axis, from its six readings; None where
    the record keeps no high-g channel or no calibration. Made at another range, by the nominal
    steps."""
    hg = head.get("hg")
    if not hg or not head["cal_time"] or not any(hg["cal"]):
        return None
    k = hg["cal_range"] / hg["range"] if hg["cal_range"] and hg["range"] else 1.0
    c = [v * k for v in hg["cal"]]
    return dict(zero=[(c[2 * i] + c[2 * i + 1]) / 2 for i in range(3)],
                per_g=[(c[2 * i] - c[2 * i + 1]) / 2 or 1.0 for i in range(3)])


def hg_convert(head, raw, cal=None):
    """The high-g channel's three i16 into g: by its step, or by its calibration."""
    if cal is None:
        step = head["hg"]["step_ng"] * 1e-9 if head["hg"]["step_ng"] else head["hg"]["full"] / 32768
        return [v * step for v in raw]
    return [(raw[i] - cal["zero"][i]) / cal["per_g"][i] for i in range(3)]


def _f32(x):
    return struct.unpack("<f", struct.pack("<f", x))[0]


def live_gain(head, high_g=False):
    """The calibration as a zero and a gain an axis, float32 as the Belka keeps them
    (firmware/common/cal.c, cal_gain_init): a step put into the sensor's own steps as an ideal
    sensor would give it — the live view's, sent raw or turned into a quaternion (PRINCIPLE.md, The
    LIVE). high_g: the accelerometer's three the high-g channel's, by its own calibration and
    step, for the live view's codes 3 and 6. No calibration: zeros and ones."""
    zero, gain = [0.0] * 6, [1.0] * 6
    cal = calibration(head)
    if cal is None:
        return zero, gain
    step_g = ((head.get("sensor") or {}).get("acc_step_ng") or 0) * 1e-9
    acc_zero, acc_per_g = cal["acc_zero"], cal["acc_per_g"]
    if high_g:
        hcal = hg_calibration(head)
        step_g = head["hg"]["step_ng"] * 1e-9
        acc_zero, acc_per_g = (hcal["zero"], hcal["per_g"]) if hcal else ([0.0] * 3, None)
    for i in range(3):
        if acc_per_g is not None:
            zero[i] = _f32(acc_zero[i])
            gain[i] = _f32(1.0 / (acc_per_g[i] * step_g)) if step_g > 0 else 1.0
        zero[3 + i] = _f32(cal["gyro_zero"][i])
        gain[3 + i] = _f32(1.0 / cal["gyro_scale"][i])
    return zero, gain


def live_step(gain, raw):
    """A step calibrated as the Belka sends it in the live view (firmware/common/cal.c,
    cal_gain_step): float32 throughout, rounded half up, held within ±32 767; a step lost kept."""
    if tuple(raw) == MISSING:
        return tuple(raw)
    zero, g = gain
    out = []
    for i in range(6):
        y = math.floor(_f32(_f32(_f32(raw[i] - zero[i]) * g[i]) + 0.5))
        out.append(max(-32767, min(32767, y)))
    return tuple(out)


def info_of(session):
    """For each grid index covered by an information block, its entry. The block numbered N
    speaks for the data blocks N-3, N-2 and N-1 (RECORD.md); its first step is the first of the
    block N-3, found on the grid by that block's period and place — the information block's own
    period is its first step's, which above 2 kHz is not the period's first step."""
    head = session["head"]
    by_number = {b["number"]: b for b in session["data"]}
    out = {}
    for b in session["info"]:
        start = None
        for back in (3, 2, 1):
            d = by_number.get(b["number"] - back)
            if d is not None:
                start = grid_index(head, d, 0) - (3 - back) * STEPS
                break
        if start is None:
            start = b["period"] * steps_a_period(head)      # no data block kept: the period's
        for j, e in enumerate(b["entries"]):
            out[start + j] = e
    return out


def steps(session):
    """Every step as (grid index, data block, j, raw six); a lost step as MISSING."""
    head = session["head"]
    for b in session["data"]:
        for j, raw in enumerate(b["steps"]):
            yield grid_index(head, b, j), b, j, raw


def to_csv(session, out, calibrated=True):
    """One row a step: its grid index, its own period, its time in seconds from period 0 and as UTC
    microseconds where the head knows the time, the six values in g and °/s, the high-g channel's
    three in g where the record keeps it, and the information
    block's offset, signal, Povodok and SYNCs for it, and `lost`: 1 for a step the unit never had,
    its values zeros, never a reading, so that the rows run evenly and the gap shows (APP.md, The
    export). A block skipped for its CRC has no rows: the gap is in the grid index."""
    head = session["head"]
    cal = calibration(head) if calibrated else None
    hcal = hg_calibration(head) if calibrated else None
    hg = dict(hg_steps(session)) if head.get("hg") else None
    rate = head["rate_hz"]
    info = info_of(session)
    out.write("step,period,t_s,utc_us,ax_g,ay_g,az_g,gx_dps,gy_dps,gz_dps,"
              + ("hx_g,hy_g,hz_g," if hg is not None else "") +
              "offset_ns,sync_dbm,povodok,syncs,lost\n")
    t0 = head["time_period"] * PERIOD_S
    spp = steps_a_period(head)
    for k, b, j, raw in steps(session):
        lost = tuple(raw) == MISSING
        v = (0.0,) * 6 if lost else convert(head, raw, cal)
        t = k / rate
        utc = round(head["time_us"] + (t - t0) * 1e6) if head["time_us"] else ""
        e = info.get(k, {})
        values = [f"{x:.6f}" for x in v[:3]] + [f"{x:.4f}" for x in v[3:]]
        if hg is not None:
            hv = hg.get(k, HG_MISSING)
            values += [f"{x:.6f}" for x in ((0.0,) * 3 if tuple(hv) == HG_MISSING
                                            else hg_convert(head, hv, hcal))]
        row = [k, k // spp, f"{t:.6f}", utc] + values + \
              [e.get("offset_ns", ""), "" if e.get("signal_dbm") is None else e["signal_dbm"],
               "" if e.get("povodok") is None else e["povodok"], e.get("syncs", ""), int(lost)]
        out.write(",".join(str(x) for x in row) + "\n")


def describe(session):
    h = session["head"]
    groups = ", ".join(f"{g['name']} ({g['unit'] or '-'}, ±{g['range']})" if g["range"] else
                       g["name"] for g in h["groups"])
    lines = [f"session {h['session']}, Belka {h['belka']}, chip {h['chip']:016x}, firmware "
             f"{h['fw']}, format {h['version']}",
             f"  {h['rate_hz']} Hz into memory, N {h['n']}, the clock "
             f"{CLOCKS.get(h['clock'], h['clock'])}, the sensor {h['sensor']['name'] or '?'}",
             f"  groups: {groups}",
             f"  time {h['time_us'] or 'unknown'} µs at period {h['time_period']}, zone "
             f"{h['tz_min']:+d} min",
             f"  calibration {'none' if not h['cal_time'] else 'at ' + str(h['cal_time'])}; "
             f"place '{h['place']}'",
             f"  table: {len(session['rows'])} rows" if session["table"] is not None
             else "  table: missing",
             f"  {len(session['data'])} data blocks, {len(session['info'])} information blocks, "
             f"{len(session['hg'])} high-g blocks, {len(session['gaps'])} missing, "
             f"{session['bad']} with a bad CRC"]
    e = session["end"]
    lines.append(f"  end: {e['why_text']}, {e['steps']} steps, {e['blocks']} blocks, last period "
                 f"{e['last_period']}, {e['dropped']} dropped" if e else "  end: none, cut off")
    return "\n".join(lines)


def main(argv):
    if len(argv) not in (2, 3):
        print(__doc__)
        return 2
    session = read(open(argv[1], "rb").read())
    if len(argv) == 2:
        print(describe(session))
        if session["table"]:
            print(session["table"].rstrip())
        return 0
    with open(argv[2], "w", encoding="utf-8", newline="") as f:
        to_csv(session, f)
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
