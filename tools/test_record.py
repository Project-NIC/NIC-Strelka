"""The record tool on a record made here, block by block as RECORD.md has it: the head of version 4
and its groups read back, the set's table with the session's units, the steps on the grid at 100 Hz and at 32 kHz, a step lost, an
information block's entries, a block with a bad CRC skipped and a missing block counted, the end,
and the conversion by the ranges and by the six positions' calibration."""

import io
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import strelka_record as rec  # noqa: E402

TABLE = ("kind;horse\nname;Bára\nnote;\"Od jara kulhá vpravo vpředu.\nKlusána na kruhu.\"\n"
         "belka;chip;code;place;kind\n3;1122334455667788;HD;hlava, nánosník;B\n"
         "2;0123456789ABCDF0;WI;kohoutek;K\n")
ACC_STEP, GYRO_STEP = 488281, 61035156          # an ICM's at ±16 g and ±2 000 °/s: the range / 32 768
HG = dict(range=320, step_ng=10417000)


def make_session(session=7, belka=3, steps=130, rate_hz=100, first_period=1000, time_us=0,
                 cal=False, table=TABLE, lose=(), bad_block=None, drop_block=None, hg=False):
    """A session of `steps` steps, m = k mod 300: ax = m, ay = -m, az = 16384 (8 g at ±16 g),
    gx = 0, gy = 100 m, gz = 100 - 100 m, and with hg the high-g channel's (m, 2m, 96); the
    information block after every 120 steps, the high-g blocks after it; the end — composed by
    strelka_record.compose, as the Belka composes it."""
    acc_cal = (16384, -16384, 16384, -16384, 16384 + 100, -16384 + 100) if cal else (0,) * 6
    hg_cal = (96, -96, 96, -96, 100, -92) if cal else (0,) * 6
    groups = rec.imu_groups(16, ACC_STEP, 2000, GYRO_STEP, acc_cal=acc_cal,
                            gyro_zero=(0, 0, 100) if cal else (0, 0, 0),
                            gyro_scale=(32768, 32768, 16384) if cal else (32768,) * 3,
                            hg=dict(HG, cal=hg_cal) if hg else None)
    head = rec.head_block(session, belka, 0x1122334455667788, fw=(0, 5, 0), rate_hz=rate_hz, n=10,
                          time_us=time_us, time_period=first_period, tz_min=120,
                          sensor="LSM6DSV320X" if hg else "ICM-42686-P", cal_temp=2550,
                          cal_time=1_760_000_000 if cal else 0, groups=groups)

    def step_of(k):
        m = k % 300                         # the values kept within 16 bits for long records
        return rec.MISSING if k in lose else (m, -m, 16384, 0, 100 * m, 100 - 100 * m)

    blocks = rec.compose(head, table, first_period, steps, step_of,
                         lambda k: (k % 7 - 3, -60 - k % 5, k % 4, 1 + k % 4),
                         hg_of=(lambda k: rec.HG_MISSING if k in lose else (k % 300, 2 * (k % 300), 96))
                         if hg else None, offset_of=lambda first: -70, temp_centi=2512, cell_mv=3290)
    lead = 1 + len(rec.table_blocks(session, table))
    body = blocks[lead:-1]
    if bad_block is not None:
        body[bad_block] = body[bad_block][:100] + b"\xff" + body[bad_block][101:]
    if drop_block is not None:
        del body[drop_block]
    return b"".join(blocks[:lead] + body + blocks[-1:])


def test_round_trip():
    s = rec.read(make_session(time_us=1_760_000_000_000_000))
    h = s["head"]
    assert h["session"] == 7 and h["belka"] == 3 and h["fw"] == "0.5.0" and h["version"] == 4
    assert h["rate_hz"] == 100 and h["n"] == 10 and h["acc_g"] == 16 and h["gyro_dps"] == 2000
    assert h["place"] == "hlava, nánosník" and h["tz_min"] == 120 and h["hg"] is None
    assert h["units"] == {3: 0, 2: 1}                  # the session's units from the set's table
    assert s["table"] == TABLE and s["rows"][2] == ["note", "Od jara kulhá vpravo vpředu.\nKlusána na kruhu."]
    assert s["rows"][5] == ["2", "0123456789ABCDF0", "WI", "kohoutek", "K"]
    assert len(s["data"]) == 4 and len(s["info"]) == 2 and s["gaps"] == [] and s["bad"] == 0
    assert s["end"]["why_text"] == "the SYNC's state said stop" and s["end"]["steps"] == 130
    assert s["end"]["last_period"] == 1000 + 129 // 2
    out = io.StringIO()
    rec.to_csv(s, out)
    rows = out.getvalue().strip().split("\n")
    assert len(rows) == 1 + 130, len(rows)
    k, period, t, utc, ax, ay, az, gx, gy, gz, off, dbm, pov, syncs, lost = rows[1 + 3].split(",")
    assert k == str(1000 * 2 + 3) and period == "1001" and t == f"{(2003) / 100:.6f}"
    assert utc == str(1_760_000_000_000_000 + 3 * 10_000)
    assert abs(float(ax) - 3 * 16 / 32768) < 1e-6 and abs(float(az) - 8.0) < 1e-5   # the step in ng
    assert abs(float(gy) - 300 * 2000 / 32768) < 1e-3
    assert off == str(3 % 7 - 3) and dbm == str(-60 - 3 % 5) and pov == "3" and syncs == "4" and \
        lost == "0"
    print("ok the round trip, the table, the grid and the information block")


def test_lost_and_bad():
    s = rec.read(make_session(lose=(5, 6), bad_block=1, drop_block=3))
    assert s["bad"] == 1 and s["gaps"] == [1, 3], (s["bad"], s["gaps"])
    assert s["data"][0]["missing"] == 2
    out = io.StringIO()
    rec.to_csv(s, out)
    rows = out.getvalue().strip().split("\n")
    # 130 steps less the data block skipped for its CRC; the two lost kept, zeros and `lost` 1;
    # the dropped block was an information block, no step of it
    assert len(rows) == 1 + 130 - 40, len(rows)
    lost = [r.split(",") for r in rows[1:] if r.endswith(",1")]
    assert len(lost) == 2 and all(float(x) == 0 for x in lost[0][4:10]), lost
    print("ok a step lost, a bad block skipped, a missing block counted")


def test_raw_rate_places():
    s = rec.read(make_session(steps=1300, rate_hz=32000, first_period=5))
    b = s["data"][17]                      # steps 680-719: period 6, the second block in it
    assert b["period"] == 6 and b["place"] == 1
    assert rec.grid_index(s["head"], b, 0) == 6 * 640 + 40
    assert len(s["info"]) == 11 and s["info"][0]["period"] == 5 and s["info"][6]["period"] == 6
    print("ok the places in a period at 32 kHz")


def test_calibration():
    s = rec.read(make_session(cal=True, steps=40))
    cal = rec.calibration(s["head"])
    assert cal["acc_zero"] == [0, 0, 100] and cal["acc_per_g"] == [16384, 16384, 16384]
    assert cal["gyro_scale"][2] == 0.5 and cal["gyro_zero"][2] == 100
    v = rec.convert(s["head"], (16384, 0, 16484, 0, 0, 100 + 328), cal)
    assert abs(v[0] - 1.0) < 1e-9 and abs(v[2] - 1.0) < 1e-9
    assert abs(v[5] - 328 * 2000 / 32768 / 0.5) < 1e-6
    assert abs(rec.convert(s["head"], (16384, 0, 0, 0, 0, 328))[0] - 8.0) < 1e-5
    # made at ±8 g and ±1000 °/s, the session at ±16 g and ±2000 °/s: the readings by the
    # nominal steps, half
    other = dict(s["head"], cal_range=(8, 1000))
    cal = rec.calibration(other)
    assert cal["acc_per_g"] == [8192, 8192, 8192] and cal["gyro_zero"][2] == 50
    print("ok the conversion by the ranges and by the calibration, made in other ranges too")


def test_head():
    """Version 4, byte by byte where it matters: the fixed part, the time zone in quarter hours, the
    sensor's name, the groups' table — the hoof's LSM6DSV320X with its high-g channel — read back
    whole, and the conversion by the sheet's own step: 30 720 steps at ±320 g are 320 g."""
    groups = rec.imu_groups(16, 488000, 4000, 140000000, cal_range=(16, 4000),
                            hg=dict(range=320, step_ng=10417000, cal=(96, -96, 97, -95, 98, -94),
                                    cal_range=320))
    head = rec.head_block(1, 2, 3, fw=(0, 9, 0), rate_hz=200, n=1, clock=2, time_us=5,
                          time_period=6, tz_min=-90, sensor="LSM6DSV320X", cal_temp=2400,
                          cal_time=7, groups=groups)
    assert head[0:6] == b"STRK\x04\x01" and head[22] == 1 and head[23] == 2
    assert head[36] == (256 - 6) and head[37:48] == b"LSM6DSV320X" and head[59] == 6
    assert head[64:68] == bytes([2, 0, 3, 1]) and head[68:71] == b"ACC" and head[76:77] == b"g"
    assert head[80:84] == (488000).to_bytes(4, "little") and head[84:86] == (16).to_bytes(2, "little")
    g6 = 64 + 5 * 36                                     # the sixth group: the high-g blocks'
    assert head[g6] == 6 and head[g6 + 4:g6 + 10] == b"ACC_HG"
    assert head[g6 + 24:g6 + 26] == (96).to_bytes(2, "little")
    assert head[g6 + 26:g6 + 28] == (-96 & 0xFFFF).to_bytes(2, "little")
    assert head[64 + 6 * 36:508] == bytes(508 - 64 - 6 * 36)
    h = rec.parse_head(head)
    assert h["tz_min"] == -90 and h["clock"] == 2 and h["sensor"]["name"] == "LSM6DSV320X"
    assert [g["name"] for g in h["groups"]] == ["ACC", "GYRO", "OFFSET", "SIGNAL", "RADIO", "ACC_HG"]
    assert h["hg"] == dict(range=320, step_ng=10417000, full=341, cal_range=320,
                           cal=(96, -96, 97, -95, 98, -94))
    assert h["cal_range"] == (16, 4000) and h["sensor"]["acc_channel"] == 0
    assert abs(rec.hg_convert(h, (30720, 0, 0))[0] - 320.01) < 0.01
    cal = rec.hg_calibration(dict(h, cal_time=7))
    assert cal["zero"] == [0, 1, 2] and cal["per_g"] == [96, 96, 96]
    # the high-g channel alone, in the data blocks: its accelerometer named ACC_HG
    alone = rec.parse_head(rec.head_block(1, 2, 3, groups=rec.imu_groups(
        320, 10417000, 4000, 140000000, high_g=True)))
    assert alone["sensor"]["acc_channel"] == 1 and alone["acc_g"] == 341 and alone["hg"] is None
    try:
        rec.parse_head(head[:4] + b"\x03" + head[5:])
        raise AssertionError("a version 3 head read")
    except ValueError:
        pass
    print("ok the head of version 4: the fixed part, the zone in quarter hours, the groups' table")


def test_high_g():
    """The hoof's both channels: after every information block the high-g channel's two blocks of
    60 steps, no number of their own; their steps on the data's grid, lost ones lost; the CSV with
    the high-g channel by its step, or by its calibration."""
    s = rec.read(make_session(steps=130, hg=True, lose=(5,)))
    assert len(s["hg"]) == 3 and s["gaps"] == [] and s["end"]["blocks"] == 6
    assert [(b["number"], b["half"], len(b["steps"])) for b in s["hg"]] == [(0, 0, 60), (0, 1, 60),
                                                                           (4, 0, 10)]
    grid = dict(rec.hg_steps(s))
    data = {k: raw for k, b, j, raw in rec.steps(s)}
    assert sorted(grid) == sorted(data) and grid[2000 + 7] == (7, 14, 96)
    assert grid[2000 + 5] == rec.HG_MISSING and grid[2000 + 125] == (125, 250, 96)
    out = io.StringIO()
    rec.to_csv(s, out)
    rows = out.getvalue().strip().split("\n")
    assert rows[0].split(",")[10:13] == ["hx_g", "hy_g", "hz_g"]
    hx = float(rows[1 + 7].split(",")[10])
    assert abs(hx - 7 * 10417000e-9) < 1e-6
    c = rec.read(make_session(steps=40, hg=True, cal=True))
    out = io.StringIO()
    rec.to_csv(c, out)
    hz = float(out.getvalue().strip().split("\n")[1].split(",")[12])
    assert abs(hz - (96 - 4) / 96) < 1e-6, hz             # (raw − zero) / its g, zero 4, 96 a g
    print("ok the high-g channel's blocks after the information block, on the grid, in the CSV")


def test_end_cut_off():
    data = make_session(steps=40)
    s = rec.read(data[:-rec.BLOCK])
    assert s["end"] is None and len(s["data"]) == 1
    print("ok a file without its end")


def test_room_left_by_a_power_cut():
    """A file cut off in the room it took at its start: after its own blocks, another session's
    whole blocks with their CRCs right, then zeros. The other session's are skipped, not read as
    this one's (RECORD.md, The end)."""
    own = make_session(session=7, steps=130)[:-rec.BLOCK]          # its end never written
    old = make_session(session=5, steps=200)
    room = own + old[4 * rec.BLOCK:] + bytes(8 * rec.BLOCK)        # an earlier file's blocks, then none
    s = rec.read(room)
    whole = rec.read(make_session(session=7, steps=130))
    assert s["end"] is None and s["head"]["session"] == 7
    assert len(s["data"]) == len(whole["data"]) and len(s["info"]) == len(whole["info"])
    assert s["gaps"] == [] and s["foreign"] > 0 and s["bad"] == 8, (s["foreign"], s["bad"])
    print("ok the room after a power cut: another session's blocks skipped")


if __name__ == "__main__":
    test_round_trip()
    test_lost_and_bad()
    test_raw_rate_places()
    test_calibration()
    test_head()
    test_high_g()
    test_end_cut_off()
    test_room_left_by_a_power_cut()
    print("RECORD OK")
