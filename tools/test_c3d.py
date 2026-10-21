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


if __name__ == "__main__":
    test_two_units()
    test_folded()
    print("C3D OK")
