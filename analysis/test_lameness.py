"""Checks of the skeleton on synthetic trots: a sound one, and one with a lame half-stride."""

import os
import sys

import numpy as np

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import lameness  # noqa: E402

FS = 100.0
F_STRIDE = 1.5       # strides a second, a trot
SECONDS = 20


def trot(a1):
    """Vertical displacement: two bounces a stride (the second harmonic, 30 mm) and an
    asymmetric part (the first harmonic, a1 mm); its acceleration, and the strides' starts."""
    t = np.arange(int(SECONDS * FS)) / FS
    w = 2 * np.pi * F_STRIDE
    disp = 0.030 * np.cos(2 * w * t) + a1 * np.cos(w * t)
    acc = -0.030 * (2 * w) ** 2 * np.cos(2 * w * t) - a1 * w ** 2 * np.cos(w * t)
    starts = np.arange(0, len(t), int(FS / F_STRIDE))
    return disp, acc, starts


def test_displacement():
    disp, acc, _ = trot(0.005)
    got = lameness.displacement(acc, FS, f_low=0.5 * F_STRIDE)
    assert np.max(np.abs(got - disp)) < 1e-3, "the integration misses by a millimetre or more"


def test_sound():
    _, acc, starts = trot(0.0)
    m = lameness.measures(acc, FS, starts)
    assert abs(m["min_diff"]) < 1e-3 and abs(m["max_diff"]) < 1e-3, m
    assert m["harmonic_ratio"] < 0.05, m


def test_lame():
    _, acc, starts = trot(0.005)
    m = lameness.measures(acc, FS, starts)
    # an asymmetry in phase with the maxima shows in MaxDiff and leaves MinDiff near zero
    assert max(abs(m["min_diff"]), abs(m["max_diff"])) > 5e-3, m
    assert abs(m["harmonic_ratio"] - 0.005 / 0.030) < 0.02, m


def test_stride_frequency():
    disp, _, _ = trot(0.0)
    assert abs(lameness.stride_frequency(disp, FS) - F_STRIDE) < 0.1


if __name__ == "__main__":
    for name, fn in list(globals().items()):
        if name.startswith("test_"):
            fn()
            print("ok", name)
