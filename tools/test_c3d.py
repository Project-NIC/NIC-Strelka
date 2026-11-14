"""The C3D converter on records made here: two Belkas' files of one session, one a period late,
laid side by side on the grid, written, read back with the c3d package (pip install c3d) and
compared step by step; and a long record folded into frames."""

import io
import os
import sys
import tempfile

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import strelka_record as rec  # noqa: E402
import strelka_c3d as c3  # noqa: E402
from test_record import make_session  # noqa: E402


def read_back(path):
    import c3d

    with open(path, "rb") as f:
        r = c3d.Reader(f)
        frames = list(r.read_frames())
        labels = [l.strip() for l in r.analog_labels]
        return r, frames, labels


def test_two_units():
    a = rec.read(make_session(session=1, belka=3, steps=45, first_period=1000))
    b = rec.read(make_session(session=1, belka=4, steps=40, first_period=1001))  # a period late
    out = io.BytesIO()
    r = c3.write(out, [a, b])
    # B4 began a period, two steps, after B3: its 40 steps end at B3's step 42
    assert r["frames"] == 45 and r["per_frame"] == 1 and r["channels"] == 12, r
    path = os.path.join(tempfile.mkdtemp(), "two.c3d")
    open(path, "wb").write(out.getvalue())
    reader, frames, labels = read_back(path)
    assert labels[:6] == ["B3_AX", "B3_AY", "B3_AZ", "B3_GX", "B3_GY", "B3_GZ"], labels
    assert labels[6] == "B4_AX"
    assert reader.analog_used == 12 and reader.analog_rate == 100.0
    assert len(frames) == 45, len(frames)
    # frame 2 of B3: ax = 2 raw at ±16 g, az 16384 = 8 g, gz = 100 - 200 raw
    i, points, analog = frames[2]
    assert points.shape[0] == 0
    assert abs(analog[0][0] - 2 * 16 / 32768) < 1e-6
    assert abs(analog[2][0] - 8.0) < 1e-5
    assert abs(analog[5][0] + 100 * 2000 / 32768) < 1e-3
    # B4's frame 2 is its own step 0, a period late: ax = 0; its frame 1 is before it began
    assert analog[6][0] == 0.0 and abs(analog[6 + 2][0] - 8.0) < 1e-5
    assert frames[1][2][6 + 2][0] == 0.0
    assert abs(frames[3][2][6][0] - 1 * 16 / 32768) < 1e-6
    # B4 ends at frame 41: zeros after
    assert frames[41][2][6 + 2][0] != 0.0 and frames[42][2][6 + 2][0] == 0.0
    print("ok two units, one a period late")


KINDS = "kind;horse\nbelka;chip;code;place;kind\n3;0;LF;left fore;Z\n4;0;RF;right fore;K\n"


def test_high_g():
    """A unit keeping both channels: its HX, HY, HZ after its six, in g, by the high-g step; the
    group STRELKA its range, step and calibration; the raw steps back still its six."""
    a = rec.read(make_session(session=1, belka=3, steps=40, hg=True, table=KINDS))
    b = rec.read(make_session(session=1, belka=4, steps=40, table=KINDS))
    out = io.BytesIO()
    c3.write(out, [a, b])
    c = c3.read(out.getvalue())
    assert c["labels"][:9] == ["Z3_AX", "Z3_AY", "Z3_AZ", "Z3_GX", "Z3_GY", "Z3_GZ", "Z3_HX",
                               "Z3_HY", "Z3_HZ"] and c["labels"][9] == "K4_AX", c["labels"]
    assert abs(c["rows"][7][6] - 7 * 10417000e-9) < 1e-6 and abs(c["rows"][7][8] - 96 * 0.010417) < 1e-5
    assert c["params"][("STRELKA", "HG_RANGE")] == [320, 0]
    assert c3.raw_back(out.getvalue())["Z3"][7] == (7, -7, 16384, 0, 700, 100 - 700)
    csv = io.StringIO()
    c3.to_csv(out.getvalue(), csv)
    assert csv.getvalue().split("\n")[0].endswith("K4_GZ,Z3_lost,K4_lost")
    print("ok the high-g channel's three after a unit's six, in g")


def test_lost_and_kinds():
    """A step lost is zeros in the C3D; the channels named by the units' kinds in the set's
    table."""
    a = rec.read(make_session(session=1, belka=3, steps=40, lose=(5,), table=KINDS))
    b = rec.read(make_session(session=1, belka=4, steps=40, table=KINDS))
    out = io.BytesIO()
    c3.write(out, [a, b])
    path = os.path.join(tempfile.mkdtemp(), "lost.c3d")
    open(path, "wb").write(out.getvalue())
    reader, frames, labels = read_back(path)
    assert labels[0] == "Z3_AX" and labels[6] == "K4_AX", labels
    assert all(frames[5][2][c][0] == 0.0 for c in range(6))           # Z3's step 5 lost
    assert abs(frames[5][2][6 + 2][0] - 8.0) < 1e-5                    # K4's there
    assert abs(frames[4][2][2][0] - 8.0) < 1e-5 and abs(frames[6][2][2][0] - 8.0) < 1e-5
    print("ok a step lost zeros, the channels named by the units' kinds")


def test_csv():
    """The C3D read back and written as CSV: a row a step, a unit's lost step zeros and 1."""
    a = rec.read(make_session(session=1, belka=3, steps=40, lose=(5,), table=KINDS))
    b = rec.read(make_session(session=1, belka=4, steps=40, first_period=1001, table=KINDS))
    out = io.BytesIO()
    c3.write(out, [a, b])
    c = c3.read(out.getvalue())
    assert c["labels"][0] == "Z3_AX" and c["steps"] == 42 and c["rate"] == 100.0, c["steps"]
    csv = io.StringIO()
    c3.to_csv(out.getvalue(), csv)
    rows = csv.getvalue().strip().split("\n")
    head = rows[0].split(",")
    assert head[:3] == ["step", "t_s", "Z3_AX"] and head[-2:] == ["Z3_lost", "K4_lost"], head
    assert len(rows) == 1 + 42
    r5 = rows[1 + 5].split(",")
    assert r5[-2:] == ["1", "0"] and all(float(x) == 0 for x in r5[2:8]), r5   # Z3's step 5 lost
    r0 = rows[1].split(",")
    assert r0[-1] == "1" and r0[-2] == "0"           # K4 a period late: its first steps zeros
    assert rows[1 + 3].split(",")[-1] == "0"
    print("ok the C3D read back and written as CSV, a lost step zeros and 1")


def test_folded():
    s = rec.read(make_session(session=2, steps=70000, rate_hz=1000, table=""))
    # ax = k runs past 16 bits in make_session; only the frames' count and rates are checked
    out = io.BytesIO()
    r = c3.write(out, [s])
    assert r["per_frame"] == 2 and r["frames"] == 35000, r
    path = os.path.join(tempfile.mkdtemp(), "long.c3d")
    open(path, "wb").write(out.getvalue())
    reader, frames, labels = read_back(path)
    assert reader.point_rate == 500.0 and reader.analog_rate == 1000.0
    assert len(frames) == 35000
    i, points, analog = frames[1]
    assert analog.shape == (6, 2), analog.shape
    print("ok folded")


def test_ezc3d():
    """The same two files read by a second reader, ezc3d (pip install ezc3d), the one many
    laboratories' tools are built on; skipped where it is not installed."""
    try:
        import ezc3d
    except ImportError:
        print("skipped ezc3d, not installed")
        return
    a = rec.read(make_session(session=1, belka=3, steps=45, first_period=1000))
    b = rec.read(make_session(session=1, belka=4, steps=40, first_period=1001))
    out = io.BytesIO()
    c3.write(out, [a, b])
    path = os.path.join(tempfile.mkdtemp(), "two.c3d")
    open(path, "wb").write(out.getvalue())
    c = ezc3d.c3d(path)
    p, an = c["parameters"], c["data"]["analogs"][0]
    assert p["ANALOG"]["RATE"]["value"][0] == 100.0 and p["ANALOG"]["USED"]["value"][0] == 12
    assert list(p["ANALOG"]["LABELS"]["value"])[:6] == ["B3_AX", "B3_AY", "B3_AZ", "B3_GX", "B3_GY",
                                                       "B3_GZ"]
    assert list(p["ANALOG"]["UNITS"]["value"])[:6] == ["g", "g", "g", "dps", "dps", "dps"]
    assert an.shape == (12, 45), an.shape
    assert abs(an[0][2] - 2 * 16 / 32768) < 1e-6 and abs(an[2][2] - 8.0) < 1e-5
    assert abs(an[5][2] + 100 * 2000 / 32768) < 1e-3
    assert an[8][1] == 0.0 and abs(an[8][2] - 8.0) < 1e-5
    # a long record folded two steps a frame: every analog sample whole and in its order
    s = rec.read(make_session(session=2, steps=70000, rate_hz=1000, table=""))
    out = io.BytesIO()
    c3.write(out, [s])
    path = os.path.join(tempfile.mkdtemp(), "long.c3d")
    open(path, "wb").write(out.getvalue())
    c = ezc3d.c3d(path)
    p, an = c["parameters"], c["data"]["analogs"][0]
    assert p["POINT"]["RATE"]["value"][0] == 500.0 and p["ANALOG"]["RATE"]["value"][0] == 1000.0
    assert an.shape == (6, 70000), an.shape
    for k in (0, 1, 2, 299, 300, 69999):            # ax = k mod 300 raw at ±16 g
        assert abs(an[0][k] - (k % 300) * 16 / 32768) < 1e-6, (k, an[0][k])
    print("ok read by ezc3d, the folded record too")


def test_main():
    """The command line: two units' files in, one C3D out, the same bytes write() makes."""
    d = tempfile.mkdtemp()
    paths = []
    for belka, first in ((3, 1000), (4, 1001)):
        path = os.path.join(d, f"S00001B{belka:02d}.STRK")
        open(path, "wb").write(make_session(session=1, belka=belka, steps=40, first_period=first))
        paths.append(path)
    out = os.path.join(d, "both.c3d")
    assert c3.main(["strelka_c3d.py", out, *paths]) == 0
    want = io.BytesIO()
    c3.write(want, [rec.read(open(p, "rb").read()) for p in paths])
    assert open(out, "rb").read() == want.getvalue(), "the command line's file differs"
    assert c3.main(["strelka_c3d.py"]) == 2
    print("ok the command line, two files in, the C3D write() makes")


def test_raw_back():
    """The raw steps computed back from the C3D's values to the count, by the steps and the
    calibration its group STRELKA carries: a unit calibrated, one not, a step lost."""
    a = rec.read(make_session(session=1, belka=3, steps=130, cal=True, lose=(7,)))
    b = rec.read(make_session(session=1, belka=4, steps=130, first_period=1001))
    out = io.BytesIO()
    c3.write(out, [a, b])
    c = c3.read(out.getvalue())
    assert c["params"][("STRELKA", "SOURCE")] == "card"
    assert c["params"][("STRELKA", "CAL_GSCALE")][:3] == [32768.0, 32768.0, 16384.0]
    back = c3.raw_back(out.getvalue())
    first = c["first"]
    for label, s in (("B3", a), ("B4", b)):
        want = [None] * c["steps"]
        for k, blk, j, raw in rec.steps(s):
            want[k - first] = None if tuple(raw) == rec.MISSING else tuple(raw)
        assert back[label] == want, label
    assert back["B3"][7] is None and back["B3"][8] is not None
    print("ok the raw steps back from the C3D to the count, calibrated and not")


if __name__ == "__main__":
    test_main()
    test_two_units()
    test_lost_and_kinds()
    test_csv()
    test_folded()
    test_ezc3d()
    test_raw_back()
    test_high_g()
    print("C3D OK")
