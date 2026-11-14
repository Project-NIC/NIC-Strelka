"""The calibration from the six faces, as a Belka computes it (FIRMWARE.md, Calibration; COMMANDS.md,
64/22), the reference its code is tested against: from the raw steps — each face held still, the
turns between them — the head's raw readings (RECORD.md, The session's head): the accelerometer
with each axis up and down, the gyro's zero at rest, the gyro's scale from the turns. Plain Python.

The faces in their order: Z up, Z down, X up, X down, Y up, Y down. A face is the raw steps while
the unit lay still on it, six i16 each; a turn the raw steps between one face and the next.

    calibrate(faces, turns, acc_g, gyro_dps, rate_hz, hg_faces=None) -> dict(acc, gscale, gzero[, hg])

- The accelerometer: each axis's mean on the face that has it up, then on the one that has it down
  — the head's acc[2i] and acc[2i+1], from which a reader takes the zero, their mean, and the g,
  half their difference.
- The gyro's zero: its mean over the six faces, at rest.
- The gyro's scale, an axis's 32 768 for 1,000: a turn about one axis alone — nine tenths of the
  turn's integrated rate on it — measures that axis: its angle integrated by the sensor's nominal
  step against the angle between gravity, by the accelerometer just calibrated, on the face before
  and on the face after. The turns of an axis averaged; an axis no turn measured keeps 1,000.

A face is still when no axis of the accelerometer spreads by more than 0.02 g and the gyro, less
its mean, by more than 2 °/s, its standard deviation; and it is the face asked when the axis
asked carries most of the gravity, with its sign. Numbers are rounded half up, alike in Dart.
"""

import math

FACES = (("z", +1), ("z", -1), ("x", +1), ("x", -1), ("y", +1), ("y", -1))
AXIS = {"x": 0, "y": 1, "z": 2}
STILL_G = 0.02
STILL_DPS = 2.0
ONE_AXIS = 0.9


def round_half_up(x):
    return math.floor(x + 0.5)


def mean(values):
    return sum(values) / len(values)


def spread(values):
    m = mean(values)
    return math.sqrt(sum((v - m) ** 2 for v in values) / len(values))


def face_problem(steps, face, acc_g, gyro_dps):
    """None when the steps lie still on the face asked; else what is wrong: 'moved' or 'face'."""
    per_g = 32768 / acc_g
    per_dps = 32768 / gyro_dps
    cols = list(zip(*steps))
    if any(spread(cols[i]) > STILL_G * per_g for i in range(3)) or \
            any(spread(cols[3 + i]) > STILL_DPS * per_dps for i in range(3)):
        return "moved"
    axis, sign = AXIS[FACES[face][0]], FACES[face][1]
    m = [mean(cols[i]) / per_g for i in range(3)]
    if m[axis] * sign < 0.8 or any(abs(m[i]) > 0.3 for i in range(3) if i != axis):
        return "face"
    return None


def calibrate_hg(hg_faces):
    """The high-g channel's six readings from the same faces, read at once with the rest: each
    axis's mean on the face that has it up, then on the one that has it down, as the
    accelerometer's (RECORD.md, The groups)."""
    out = [0] * 6
    for f, (name, sign) in enumerate(FACES):
        i = AXIS[name]
        out[2 * i + (0 if sign > 0 else 1)] = round_half_up(mean([s[i] for s in hg_faces[f]]))
    return out


def calibrate(faces, turns, acc_g, gyro_dps, rate_hz, hg_faces=None):
    means = [[mean(col) for col in zip(*steps)] for steps in faces]
    acc = [0] * 6
    for f, (name, sign) in enumerate(FACES):
        i = AXIS[name]
        acc[2 * i + (0 if sign > 0 else 1)] = round_half_up(means[f][i])
    gzero = [round_half_up(mean([m[3 + i] for m in means])) for i in range(3)]
    zero = [(acc[2 * i] + acc[2 * i + 1]) / 2 for i in range(3)]
    per_g = [(acc[2 * i] - acc[2 * i + 1]) / 2 or 1.0 for i in range(3)]
    gravity = [[(m[i] - zero[i]) / per_g[i] for i in range(3)] for m in means]
    step_dps = gyro_dps / 32768
    estimates = [[], [], []]
    for k, steps in enumerate(turns):
        theta = [sum(s[3 + i] - gzero[i] for s in steps) / rate_hz * step_dps for i in range(3)]
        norm = math.sqrt(sum(t * t for t in theta))
        a, b = gravity[k], gravity[k + 1]
        cos = sum(x * y for x, y in zip(a, b)) / math.sqrt(
            sum(x * x for x in a) * sum(y * y for y in b))
        angle = math.degrees(math.acos(max(-1.0, min(1.0, cos))))
        if norm == 0 or angle < 45:
            continue
        for i in range(3):
            if abs(theta[i]) >= ONE_AXIS * norm:
                estimates[i].append(abs(theta[i]) / angle)
    gscale = [round_half_up(mean(e) * 32768) if e else 32768 for e in estimates]
    out = dict(acc=acc, gscale=gscale, gzero=gzero)
    if hg_faces is not None:
        out["hg"] = calibrate_hg(hg_faces)
    return out
