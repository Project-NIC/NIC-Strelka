"""The six faces' calibration (tools/strelka_cal.py) on a unit made up here: its accelerometer's
zeros and gains off, its gyro's zero and scale off, the faces still with noise, the turns about
one axis each; what it computes against what was put in. The same unit lies in the Povodok
simulator's Belkas (64/22) and before the firmware's own computation (tools/test_firmware.py)."""

import math
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import strelka_cal as cal  # noqa: E402

ACC_G, GYRO_DPS, RATE = 16, 2000, 100
PER_G, PER_DPS = 32768 / ACC_G, 32768 / GYRO_DPS
ZERO = (31, -22, 47)                       # the accelerometer's zeros, raw
GAIN = (1.012, 0.991, 1.004)               # its gains against the nominal
GZERO = (6, -13, 4)                        # the gyro's zero, raw
GSCALE = (1.021, 0.987, 1.008)             # its scale against the nominal


def noise(seed):
    """A small deterministic noise, ±3 raw steps: the same in every run and in Dart."""
    state = [seed]

    def next_():
        state[0] = (state[0] * 1103515245 + 12345) % 2 ** 31
        return state[0] % 7 - 3
    return next_


def face_steps(face, n=200, seed=1):
    r = noise(seed)
    axis, sign = cal.AXIS[cal.FACES[face][0]], cal.FACES[face][1]
    steps = []
    for _ in range(n):
        acc = [ZERO[i] + (sign * GAIN[i] * PER_G if i == axis else 0) + r() for i in range(3)]
        gyro = [GZERO[i] + r() for i in range(3)]
        steps.append([round(v) for v in acc + gyro])
    return steps


def turn_steps(axis, degrees, seconds=1.0, seed=7):
    """A turn about one axis at an even rate; the accelerometer's values do not count in it."""
    r = noise(seed)
    n = int(seconds * RATE)
    rate = degrees / seconds
    return [[0, 0, 0] + [round(GZERO[i] + (GSCALE[i] * PER_DPS * rate if i == axis else 0) + r())
                         for i in range(3)] for _ in range(n)]


def unit():
    faces = [face_steps(f, seed=f + 1) for f in range(6)]
    # Z up → Z down about x, 180°; → X up about y, 90°; → X down about z, 180°; → Y up about z,
    # 90°; → Y down about x, 180°
    turns = [turn_steps(0, 180, seed=11), turn_steps(1, 90, seed=12), turn_steps(2, 180, seed=13),
             turn_steps(2, 90, seed=14), turn_steps(0, 180, seed=15)]
    return faces, turns


def main():
    faces, turns = unit()
    got = cal.calibrate(faces, turns, ACC_G, GYRO_DPS, RATE)
    for i in range(3):
        up, down = got["acc"][2 * i], got["acc"][2 * i + 1]
        assert abs((up + down) / 2 - ZERO[i]) <= 1, (i, up, down)
        assert abs((up - down) / 2 - GAIN[i] * PER_G) <= 1, (i, up, down)
        assert abs(got["gzero"][i] - GZERO[i]) <= 1, got["gzero"]
        assert abs(got["gscale"][i] / 32768 - GSCALE[i]) < 0.002, got["gscale"]
    print("ok the accelerometer's zeros and gains, the gyro's zero and scale found")
    assert cal.face_problem(faces[0], 0, ACC_G, GYRO_DPS) is None
    assert cal.face_problem(faces[0], 2, ACC_G, GYRO_DPS) == "face"          # Z up, not X up
    moved = [s[:] for s in faces[0]]
    for k in range(0, len(moved), 2):
        moved[k][0] += int(0.1 * PER_G)                                         # 0.1 g shaking
    assert cal.face_problem(moved, 0, ACC_G, GYRO_DPS) == "moved"
    print("ok a face still and asked, a wrong face, a unit moved")
    # an axis no turn measured keeps 1,000
    one = cal.calibrate(faces, turns[:1], ACC_G, GYRO_DPS, RATE)
    assert one["gscale"][1] == 32768 and one["gscale"][2] == 32768 and one["gscale"][0] != 32768
    print("ok an axis without its turn keeps 1,000")
    # the high-g channel from the same faces, at ±320 g some 96 steps a g, its zero off by 1.5 g
    hg_zero, hg_per_g = (144, -50, 20), 96
    hg_faces = []
    for f, (name, sign) in enumerate(cal.FACES):
        i = cal.AXIS[name]
        hg_faces.append([[hg_zero[a] + (sign * hg_per_g if a == i else 0) + (k % 3 - 1)
                          for a in range(3)] for k in range(200)])
    both = cal.calibrate(faces, turns, ACC_G, GYRO_DPS, RATE, hg_faces=hg_faces)
    assert both["acc"] == got["acc"] and both["hg"] == [144 + 96, 144 - 96, -50 + 96, -50 - 96,
                                                        20 + 96, 20 - 96], both["hg"]
    print("ok the high-g channel's six readings from the same faces at once")
    print("CAL OK")


if __name__ == "__main__":
    main()
