"""The live view saved (tools/strelka_live.py) checked: the sensors' steps against the firmware's
own table (firmware/common/imu_tables.c, compiled with gcc), the messages of one Povodok kept byte
for byte, those of several joined with their copies compared, the file read back, and its C3D — the
raw steps as the download's, read again by ezc3d where it is installed and computed back to the
count, the quaternions as four channels, and the CSV of each, a step lost zeros and `lost` 1.

    python3 tools/test_live.py
"""

import io
import math
import os
import subprocess
import sys
import tempfile

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import live_view as lv  # noqa: E402
import strelka_c3d as c3  # noqa: E402
import strelka_live as sl  # noqa: E402
import strelka_record as rec  # noqa: E402

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
COMMON = os.path.join(ROOT, "firmware", "common")
KINDS = {1: 0x47, 2: 0x44, 3: 0x70, 4: 0x73}       # imu_tables.h's kinds by their WHO_AM_I
ACC = (2, 4, 8, 16, 32, 64, 128, 256, 320)
GYRO = (125, 250, 500, 1000, 2000, 4000)

STEPS_C = r"""
#include <stdio.h>
#include "imu_tables.h"
int main(void)
{
	static const unsigned acc[] = { %s }, gyro[] = { %s };
	for (unsigned k = 1; k <= 4; k++) {
		for (unsigned i = 0; i < sizeof(acc) / sizeof(acc[0]); i++)
			printf("%%u a %%u %%u\n", k, acc[i], (unsigned)imu_acc_step_ng_of(k, acc[i]));
		for (unsigned i = 0; i < sizeof(gyro) / sizeof(gyro[0]); i++)
			printf("%%u g %%u %%u\n", k, gyro[i], (unsigned)imu_gyro_step_ndps_of(k, gyro[i]));
	}
	return 0;
}
"""


def test_steps():
    with tempfile.TemporaryDirectory() as tmp:
        src, exe = os.path.join(tmp, "steps.c"), os.path.join(tmp, "steps")
        with open(src, "w") as f:
            f.write(STEPS_C % (", ".join(map(str, ACC)), ", ".join(map(str, GYRO))))
        subprocess.check_call(["gcc", "-std=c11", "-w", "-I", COMMON, src,
                               os.path.join(COMMON, "imu_tables.c"), "-o", exe])
        lines = subprocess.check_output([exe]).decode().split()
    for i in range(0, len(lines), 4):
        k, what, rng, step = int(lines[i]), lines[i + 1], int(lines[i + 2]), int(lines[i + 3])
        mine = (sl.acc_step_ng if what == "a" else sl.gyro_step_ndps)(KINDS[k], rng)
        assert mine == step, (k, what, rng, mine, step)
    print("ok the sensors' steps the firmware's, %d of them" % (len(lines) // 4))


def raw(n, k):
    return tuple((n * 1000 + k * 7 + a * 101) % 30000 - 15000 for a in range(6))


def step(n, k, code):
    """Belka n's step k: raw, or a quaternion packed."""
    if lv.STEP_BYTES[code] == 12:
        return raw(n, k)
    a = n * 0.1 + k * 0.01
    return lv.pack_quat((math.cos(a), 0.6 * math.sin(a), 0.8 * math.sin(a), 0.0))


def message(period, belkas, code=lv.RAW_100, by=0, alter=()):
    """A period's message: each Belka in `belkas` with her steps, `alter` those whose steps
    differ in this copy. The head's S0 the session's code, a period of 3 or 6 its entries' alone."""
    s0 = (code << 4) | 8
    session_s0 = ({lv.RAW_50_HG: lv.RAW_50, lv.RAW_100_HG: lv.RAW_100}.get(code, code) << 4) | 8
    entries = []
    for n in belkas:
        spp = lv.STEPS[code]
        steps = [step(n, period * spp + j, code) for j in range(spp)]
        if n in alter:
            steps[0] = tuple(v + 1 for v in steps[0]) if spp and lv.STEP_BYTES[code] == 12 \
                else steps[0] ^ 1
        entries.append((n, s0, by, 0x1100 + n, steps))
    return lv.split(7, period, 0x8000, session_s0, 0b0111, (0, 0, 0, 0), entries)


def test_one_povodok():
    """The lead alone: every message kept as it came, two of a period's 50 Belkas as two."""
    sent, kept = [], []
    j = sl.Joiner(kept.append)
    for p in range(1000, 1006):
        for m in message(p, range(1, 51) if p == 1003 else (3, 5, 9)):
            sent.append(m)
            j.add(m)
    j.finish()
    assert kept == sent and len(sent) == 7, (len(kept), len(sent))
    print("ok one Povodok: %d messages kept byte for byte, 50 Belkas in two" % len(sent))


def test_several():
    """Three Povodoks: the copies joined, one kept a Belka; two copies differing lost; three with
    two alike the two's; the head the lead's though it came last."""
    kept = []
    j = sl.Joiner(kept.append)
    p = 2000
    j.add(message(p, (5, 7, 9), by=2)[0], lead=False)                # Povodok 2: 5, 7, 9
    j.add(message(p, (7, 9, 11), by=3, alter=(7,))[0], lead=False)  # Povodok 3: 7, 9, 11
    j.add(message(p, (3, 5, 9, 11), by=0, alter=(9,))[0], lead=True)
    j.add(message(p + 2, (3,))[0])
    j.finish()
    first = lv.parse(kept[0], quats_as_tuples=False)
    got = {b["n"]: b for b in first["belkas"]}
    assert first["povodoks"] == 0b0111 and first["period"] == p
    assert sorted(got) == [3, 5, 9, 11], sorted(got)            # 7: two copies differing
    assert got[3]["by"] == 0 and got[5]["by"] == 2                 # the first of the copies alike
    assert got[9]["steps"][0] == raw(9, p * 2) and got[9]["by"] == 2   # two of three alike
    assert got[11]["steps"][0] == raw(11, p * 2) and got[11]["by"] == 3
    assert j.disagreed == 1
    late = len(kept)
    j.add(message(p, (13,))[0])                                     # a period already written
    assert len(kept) == late
    print("ok several Povodoks: the copies compared, one kept, a disagreement lost")


SESSION = dict(s=7, rate=1000, n=10, acc=16, gyro=4000, live=lv.RAW_100, stop=10, off=600,
               t=0, tz=60, net=False)
TABLE = ("kind;horse\nbelka;chip;code;place\n3;F0F0F0F0F0F00003;LF;left fore\n"
         "5;F0F0F0F0F0F00005;RF;right fore\n")


def info(n, whoami, kind=0):
    return dict(chip=(0xF0F0F0F0F0F00000 + n).to_bytes(8, "little"), n=n, who=2, kind=kind,
                sensor=whoami, state=0, fault=0, cal=1762700000)


CAL = dict(acc=[2100, -2000, 2060, -2030, 2000, -2090], gscale=[33000, 32700, 32768],
           gzero=[12, -7, 3], range=[16, 2000], temp=2510, t=1762700000)


def saved_file(code, periods, lose=()):
    out = bytearray(sl.file_head())
    pieces = [dict(off=0, len=len(TABLE), csv=TABLE)]
    out += sl.smp_records(dict(SESSION, live=code), pieces,
                          [(3, info(3, 0x70), CAL), (5, info(5, 0x44, kind=1), {})])
    j = sl.Joiner(lambda m: out.extend(sl.record(sl.LIVE, 0, m)))
    for p in periods:
        if p not in lose:
            for m in message(p, (3, 5), code=code):
                j.add(m)
    j.finish()
    return bytes(out)


def test_file_and_c3d():
    data = saved_file(lv.RAW_100, range(500, 510), lose=(504,))
    saved = sl.read(data)
    assert saved["session"]["s"] == 7 and saved["table"] == TABLE
    assert saved["info"][5]["kind"] == 1 and saved["cal"][3]["gscale"] == CAL["gscale"]
    assert len(saved["messages"]) == 9
    hs = sl.heads(saved)
    assert hs[3]["place"] == "left fore" and hs[3]["sensor"]["gyro_step_ndps"] == 140000000
    assert hs[5]["sensor"]["gyro_step_ndps"] == 122070313 and c3.kind_letter(hs[5]) == "K"
    buf = io.BytesIO()
    r = sl.to_c3d(saved, buf)
    assert r["steps"] == 20 and r["channels"] == 12, r
    c = c3.read(buf.getvalue())
    assert c["labels"][:7] == ["B3_AX", "B3_AY", "B3_AZ", "B3_GX", "B3_GY", "B3_GZ", "K5_AX"]
    assert c["first"] == 1000 and c["rate"] == 100
    assert c["params"][("STRELKA", "SOURCE")] == "live"
    assert c3.raw_back(buf.getvalue())["B3"][1] == raw(3, 1001)     # the raw steps back
    want = rec.convert(hs[3], raw(3, 1001))          # calibrated by the Belka: the steps alone
    assert all(abs(a - b) < 1e-4 for a, b in zip(c["rows"][1][:6], want)), (c["rows"][1], want)
    assert all(v == 0.0 for v in c["rows"][8] + c["rows"][9])    # period 504 lost
    csv = io.StringIO()
    sl.to_csv(saved, csv)
    lines = csv.getvalue().splitlines()
    assert lines[0].endswith("B3_lost,K5_lost") and lines[9].endswith(",1,1"), lines[9]
    assert lines[2].endswith(",0,0") and lines[2].startswith("1001,0.010000,")
    try:
        import ezc3d
    except ImportError:
        print("ok the file read, its C3D and CSV; ezc3d not installed, not read by it")
        return
    with tempfile.TemporaryDirectory() as tmp:
        path = os.path.join(tmp, "live.c3d")
        with open(path, "wb") as f:
            f.write(buf.getvalue())
        e = ezc3d.c3d(path)
        assert e["parameters"]["ANALOG"]["LABELS"]["value"][6] == "K5_AX"
        assert e["data"]["analogs"].shape[1:] == (12, 20), e["data"]["analogs"].shape
    print("ok the file read, the raw steps as the download's C3D, read by ezc3d, and its CSV")


def test_quaternions():
    """The quaternions as the C3D's QW, QX, QY, QZ, its SOURCE `live`, and its CSV."""
    import struct
    saved = sl.read(saved_file(lv.QUAT_50, range(300, 304), lose=(302,)))
    buf = io.BytesIO()
    sl.to_c3d(saved, buf)
    c = c3.read(buf.getvalue())
    assert c["labels"] == ["B3_QW", "B3_QX", "B3_QY", "B3_QZ", "K5_QW", "K5_QX", "K5_QY", "K5_QZ"]
    assert c["params"][("STRELKA", "SOURCE")] == "live" and c["rate"] == 50
    csv = io.StringIO()
    sl.to_csv(saved, csv)
    lines = csv.getvalue().splitlines()
    assert lines[0] == ("step,t_s,B3_QW,B3_QX,B3_QY,B3_QZ,K5_QW,K5_QX,K5_QY,K5_QZ,"
                        "B3_lost,K5_lost"), lines[0]
    assert len(lines) == 5 and lines[3] == "302,0.040000," + ",".join(["0.000000"] * 8) + ",1,1"
    f32 = lambda v: struct.unpack("<f", struct.pack("<f", v))[0]  # noqa: E731
    q = tuple(f32(v) for v in lv.unpack_quat(step(3, 300, lv.QUAT_50)))
    assert lines[1].startswith("300,0.000000,%.6f,%.6f,%.6f,%.6f," % q), (lines[1], q)
    print("ok the quaternions as the C3D's four channels, its SOURCE live, and its CSV")


def test_high_g_codes():
    """Both channels kept: a period the low-g clipped in comes with code 6, its accelerometer the
    high-g channel's, converted by that channel's step; the C3D's AX the same channel in g, either."""
    out = bytearray(sl.file_head())
    out += sl.smp_records(dict(SESSION, live=lv.RAW_100, acc=16, hg=320), [],
                          [(3, info(3, 0x73, kind=1), {})])
    j = sl.Joiner(lambda m: out.extend(sl.record(sl.LIVE, 0, m)))
    for p, code in ((500, lv.RAW_100), (501, lv.RAW_100_HG)):
        for m in message(p, (3,), code=code):
            j.add(m)
    j.finish()
    saved = sl.read(bytes(out))
    hs = sl.heads(saved)
    assert hs[3]["hg"]["step_ng"] == 10417000 and hs[3]["sensor"]["acc_step_ng"] == 488000
    buf = io.BytesIO()
    sl.to_c3d(saved, buf)
    c = c3.read(buf.getvalue())
    assert c["labels"] == ["K3_AX", "K3_AY", "K3_AZ", "K3_GX", "K3_GY", "K3_GZ"], c["labels"]
    assert abs(c["rows"][0][0] - raw(3, 1000)[0] * 488000e-9) < 1e-5
    assert abs(c["rows"][2][0] - raw(3, 1002)[0] * 10417000e-9) < 1e-4
    assert c["rate"] == 100
    print("ok the live view's codes 3 and 6: the high-g channel's periods by its own step")


def main():
    test_steps()
    test_one_povodok()
    test_several()
    test_file_and_c3d()
    test_quaternions()
    test_high_g_codes()
    print("LIVE SAVED OK")


if __name__ == "__main__":
    main()
