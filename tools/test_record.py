"""The record tool on a record made here, block by block as RECORD.md has it: the head read back,
the set's table over two blocks, the steps on the grid at 100 Hz and at 32 kHz, a step lost, an
information block's entries, a block with a bad CRC skipped and a missing block counted, the end,
and the conversion by the ranges and by the six positions' calibration."""

import io
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import strelka_record as rec  # noqa: E402

TABLE = ("druh;kůň\njméno;Bára\nanamnéza;\"Od jara kulhá vpravo vpředu.\nKlusána na kruhu.\"\n"
         "belka;cip;misto\n1;0123456789ABCDEF;hlava, nánosník\n2;0123456789ABCDF0;kohoutek\n")


def make_session(session=7, belka=3, steps=130, rate_hz=100, first_period=1000, time_us=0,
                 cal=False, table=TABLE, lose=(), bad_block=None, drop_block=None):
    """A session of `steps` steps, m = k mod 300: ax = m, ay = -m, az = 16384 (8 g at ±16 g),
    gx = 0, gy = 100 m, gz = 100 - 100 m; the information block after every 120 steps; the end."""
    acc_cal = (16384, -16384, 16384, -16384, 16384 + 100, -16384 + 100) if cal else (0,) * 6
    head = rec.head_block(session, belka, 0x1122334455667788, fw=(0, 5, 0), build=b"\x01\x02\x03\x04",
                          rate_hz=rate_hz, n=10, acc_g=16, gyro_dps=2000, first_period=first_period,
                          time_us=time_us, time_period=first_period, tz_min=120, acc_cal=acc_cal,
                          gyro_scale=(32768, 32768, 16384) if cal else (32768,) * 3,
                          gyro_zero=(0, 0, 100) if cal else (0, 0, 0), cal_temp=2550,
                          cal_time=1_760_000_000 if cal else 0, place="hlava, nánosník",
                          name=f"S{session:05d}B{belka:02d}.STRK")
    blocks = [head] + rec.table_blocks(session, table)
    spp = rec.steps_a_period(dict(rate_hz=rate_hz))
    number = 0
    body = []
    for first in range(0, steps, rec.STEPS):
        n = min(rec.STEPS, steps - first)
        vals = []
        for i in range(n):
            k = first + i
            m = k % 300                     # the values kept within 16 bits for long records
            vals.append(rec.MISSING if k in lose else (m, -m, 16384, 0, 100 * m, 100 - 100 * m))
        period = first_period + first // spp
        place = (first % spp) // rec.STEPS
        body.append(rec.data_block(session, number, period, vals, place=place, offset_ns=-70))
        number += 1
        if (first + n) % rec.INFO_STEPS == 0 or first + n == steps:
            start = (first + n - 1) // rec.INFO_STEPS * rec.INFO_STEPS
            entries = [(k % 7 - 3, -60 - k % 5, k % 4, 1 + k % 4) for k in range(start, first + n)]
            body.append(rec.info_block(session, number, first_period + start // spp, entries,
                                       temp_centi=2512, cell_mv=3290))
            number += 1
    if bad_block is not None:
        body[bad_block] = body[bad_block][:100] + b"\xff" + body[bad_block][101:]
    if drop_block is not None:
        del body[drop_block]
    last_period = first_period + (steps - 1) // spp
    return b"".join(blocks + body) + rec.end_block(session, 1, steps, number, last_period)


def test_round_trip():
    s = rec.read(make_session(time_us=1_760_000_000_000_000))
    h = s["head"]
    assert h["session"] == 7 and h["belka"] == 3 and h["fw"] == "0.5.0" and h["build"] == "01020304"
    assert h["rate_hz"] == 100 and h["n"] == 10 and h["acc_g"] == 16 and h["gyro_dps"] == 2000
    assert h["place"] == "hlava, nánosník" and h["name"] == "S00007B03.STRK" and h["tz_min"] == 120
    assert s["table"] == TABLE and s["rows"][2] == ["anamnéza", "Od jara kulhá vpravo vpředu.\nKlusána na kruhu."]
    assert s["rows"][5] == ["2", "0123456789ABCDF0", "kohoutek"]
    assert len(s["data"]) == 4 and len(s["info"]) == 2 and s["gaps"] == [] and s["bad"] == 0
    assert s["end"]["why_text"] == "the SYNC's state said stop" and s["end"]["steps"] == 130
    assert s["end"]["last_period"] == 1000 + 129 // 2
    out = io.StringIO()
    rec.to_csv(s, out)
    rows = out.getvalue().strip().split("\n")
    assert len(rows) == 1 + 130, len(rows)
    k, period, t, utc, ax, ay, az, gx, gy, gz, off, dbm, pov, syncs = rows[1 + 3].split(",")
    assert k == str(1000 * 2 + 3) and period == "1001" and t == f"{(2003) / 100:.6f}"
    assert utc == str(1_760_000_000_000_000 + 3 * 10_000)
    assert abs(float(ax) - 3 * 16 / 32768) < 1e-6 and abs(float(az) - 8.0) < 1e-6
    assert abs(float(gy) - 300 * 2000 / 32768) < 1e-3
    assert off == str(3 % 7 - 3) and dbm == str(-60 - 3 % 5) and pov == "3" and syncs == "4"
    print("ok the round trip, the table, the grid and the information block")


def test_lost_and_bad():
    s = rec.read(make_session(lose=(5, 6), bad_block=1, drop_block=3))
    assert s["bad"] == 1 and s["gaps"] == [1, 3], (s["bad"], s["gaps"])
    assert s["data"][0]["missing"] == 2
    out = io.StringIO()
    rec.to_csv(s, out)
    rows = out.getvalue().strip().split("\n")
    # 130 steps less the data block skipped for its CRC, less two lost; the dropped block was an
    # information block, no step of it
    assert len(rows) == 1 + 130 - 40 - 2, len(rows)
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
    assert rec.convert(s["head"], (16384, 0, 0, 0, 0, 328)) [0] == 8.0
    print("ok the conversion by the ranges and by the calibration")


def test_end_cut_off():
    data = make_session(steps=40)
    s = rec.read(data[:-rec.BLOCK])
    assert s["end"] is None and len(s["data"]) == 1
    print("ok a file without its end")


if __name__ == "__main__":
    test_round_trip()
    test_lost_and_bad()
    test_raw_rate_places()
    test_calibration()
    test_end_cut_off()
    print("RECORD OK")
