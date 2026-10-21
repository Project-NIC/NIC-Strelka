"""NIC-Strelka — a Belka's record, read, written and exported.

The record is RECORD.md's: a session is one file of blocks of 512 B — the session's head, the set's
table, data blocks of 40 steps with an information block after every 120 steps, and the end — every
number little-endian, every block ending in a CRC-32 (IEEE, zlib's) over the 508 bytes before it.
This reads such a file, lists it, writes it as CSV, and builds its blocks, so that the firmware and
the application have one reference for both directions.

    python3 tools/strelka_record.py session.bin            the head, the table, the blocks, the end
    python3 tools/strelka_record.py session.bin out.csv    the steps as CSV
"""

import csv
import io
import struct
import sys
import zlib

BLOCK = 512
STEPS = 40                       # steps in a data block
INFO_STEPS = 120                 # steps an information block speaks for: three data blocks
STEP_BYTES = 12
PERIOD_S = 0.020
KIND_HEAD, KIND_DATA, KIND_END, KIND_TABLE, KIND_INFO = 1, 2, 3, 4, 5
MISSING = (-32768,) * 6          # a step lost: its six values
NO_SIGNAL = -128                 # the SYNC's signal when none was heard
END_WHY = {0: "unfinished", 1: "the SYNC's state said stop", 2: "switched off by magnet A",
           3: "the cell low", 4: "the card full", 5: "a fault",
           6: "switched off by the application"}

HEAD = struct.Struct("<4sBBHBQ3s4sHBBHIQIhB6h3H3hhI32s32s")   # 143 B, the rest free
TABLE = struct.Struct("<BBHH")                                 # 6 B, then 502 B of text
TABLE_TEXT = 502
DATA = struct.Struct("<BBHIIBBBBi")                            # 20 B, 8 free, the steps at 28
INFO = struct.Struct("<BBHIIhH")                               # 16 B, 12 free, the entries at 28
INFO_ENTRY = struct.Struct("<hbB")                             # 4 B a step
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


def head_block(session, belka, chip, fw=(0, 0, 0), build=b"\x00" * 4, rate_hz=100, n=1,
               acc_g=32, gyro_dps=4000, first_period=0, time_us=0, time_period=0, tz_min=0,
               clock=0, acc_cal=(0,) * 6, gyro_scale=(32768,) * 3, gyro_zero=(0,) * 3,
               cal_temp=0, cal_time=0, place="", name="", version=1):
    """The session's head (RECORD.md, The session's head)."""
    return seal(HEAD.pack(b"STRK", version, KIND_HEAD, session, belka, chip, bytes(fw), build,
                          rate_hz, n, acc_g, gyro_dps, first_period, time_us, time_period, tz_min,
                          clock, *acc_cal, *gyro_scale, *gyro_zero, cal_temp, cal_time,
                          place.encode("utf-8")[:32], name.encode("utf-8")[:32]))


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


def end_block(session, why, steps, blocks, last_period, dropped=0):
    return seal(END.pack(KIND_END, why, session, steps, blocks, last_period, dropped))


# --- the blocks, read --------------------------------------------------------------------------

def parse_head(block):
    f = HEAD.unpack_from(block)
    (mark, version, kind, session, belka, chip, fw, build, rate, n, acc, gyro, first_period,
     time_us, time_period, tz, clock) = f[:17]
    if mark != b"STRK" or kind != KIND_HEAD:
        return None
    acc_cal, gyro_scale, gyro_zero = f[17:23], f[23:26], f[26:29]
    cal_temp, cal_time, place, name = f[29:33]
    return dict(version=version, session=session, belka=belka, chip=chip,
                fw=f"{fw[0]}.{fw[1]}.{fw[2]}", build=build.hex(), rate_hz=rate, n=n, acc_g=acc,
                gyro_dps=gyro, first_period=first_period, time_us=time_us,
                time_period=time_period, tz_min=tz, clock=clock, acc_cal=acc_cal,
                gyro_scale=gyro_scale, gyro_zero=gyro_zero, cal_temp=cal_temp, cal_time=cal_time,
                place=place.rstrip(b"\x00").decode("utf-8", "replace"),
                name=name.rstrip(b"\x00").decode("utf-8", "replace"))


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
    their numbers, end (None if the file is cut off), the block numbers missing, and how many
    blocks failed their CRC. Blocks whose CRC fails are skipped, as a reader must after a power
    cut (RECORD.md, The end)."""
    head = end = None
    tables, blocks = [], []
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
            end = parse_end(block)
    if head is None:
        raise ValueError("no head: not a Strelka record")
    blocks.sort(key=lambda b: b["number"])
    numbers = [b["number"] for b in blocks]
    gaps = sorted(set(range(numbers[0], numbers[-1] + 1)) - set(numbers)) if numbers else []
    text = table_text(tables)
    return dict(head=head, table=text, rows=table_rows(text),
                data=[b for b in blocks if b["kind"] == KIND_DATA],
                info=[b for b in blocks if b["kind"] == KIND_INFO], end=end, gaps=gaps, bad=bad)


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
    the gyro's zero and scale. None where the head carries no calibration."""
    if not head["cal_time"]:
        return None
    c = head["acc_cal"]
    zero = [(c[2 * i] + c[2 * i + 1]) / 2 for i in range(3)]
    per_g = [(c[2 * i] - c[2 * i + 1]) / 2 or 1.0 for i in range(3)]
    return dict(acc_zero=zero, acc_per_g=per_g, gyro_zero=head["gyro_zero"],
                gyro_scale=[s / 32768 or 1.0 for s in head["gyro_scale"]])


def convert(head, raw, cal=None):
    """Six raw i16 into g and °/s: by the ranges, and by the calibration where there is one; a sum
    of N is stored as its mean, so N changes nothing here (APP.md, The conversion)."""
    acc = head["acc_g"] / 32768
    gyro = head["gyro_dps"] / 32768
    if cal is None:
        return [v * acc for v in raw[:3]] + [v * gyro for v in raw[3:]]
    a = [(raw[i] - cal["acc_zero"][i]) / cal["acc_per_g"][i] for i in range(3)]
    g = [(raw[3 + i] - cal["gyro_zero"][i]) * gyro / cal["gyro_scale"][i] for i in range(3)]
    return a + g


def info_of(session):
    """For each grid index covered by an information block, its entry."""
    head = session["head"]
    out = {}
    for b in session["info"]:
        start = b["period"] * steps_a_period(head)
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
    microseconds where the head knows the time, the six values in g and °/s, and the information
    block's offset, signal, Povodok and SYNCs for it; a lost step is no row."""
    head = session["head"]
    cal = calibration(head) if calibrated else None
    rate = head["rate_hz"]
    info = info_of(session)
    out.write("step,period,t_s,utc_us,ax_g,ay_g,az_g,gx_dps,gy_dps,gz_dps,"
              "offset_ns,sync_dbm,povodok,syncs\n")
    t0 = head["time_period"] * PERIOD_S
    spp = steps_a_period(head)
    for k, b, j, raw in steps(session):
        if tuple(raw) == MISSING:
            continue
        t = k / rate
        utc = round(head["time_us"] + (t - t0) * 1e6) if head["time_us"] else ""
        v = convert(head, raw, cal)
        e = info.get(k, {})
        row = [k, k // spp, f"{t:.6f}", utc] + [f"{x:.6f}" for x in v[:3]] + \
              [f"{x:.4f}" for x in v[3:]] + \
              [e.get("offset_ns", ""), "" if e.get("signal_dbm") is None else e["signal_dbm"],
               "" if e.get("povodok") is None else e["povodok"], e.get("syncs", "")]
        out.write(",".join(str(x) for x in row) + "\n")


def describe(session):
    h = session["head"]
    lines = [f"session {h['session']}, Belka {h['belka']}, chip {h['chip']:016x}, firmware "
             f"{h['fw']} ({h['build']}), format {h['version']}",
             f"  {h['rate_hz']} Hz into memory, N {h['n']}, ±{h['acc_g']} g, ±{h['gyro_dps']} °/s, "
             f"the clock {'steered' if h['clock'] == 0 else 'the fallback'}",
             f"  first period {h['first_period']}; time {h['time_us'] or 'unknown'} µs at period "
             f"{h['time_period']}, zone {h['tz_min']:+d} min",
             f"  calibration {'none' if not h['cal_time'] else 'at ' + str(h['cal_time'])}; "
             f"place '{h['place']}'; file '{h['name']}'",
             f"  table: {len(session['rows'])} rows" if session["table"] is not None
             else "  table: missing",
             f"  {len(session['data'])} data blocks, {len(session['info'])} information blocks, "
             f"{len(session['gaps'])} missing, {session['bad']} with a bad CRC"]
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
