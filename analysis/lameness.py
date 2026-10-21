"""A first skeleton of the lameness measures, after the published methods (see ANALYSIS.md).

The input is the vertical acceleration of a trunk unit — head, withers or pelvis — in the world
frame, gravity removed, sampled evenly; and the start of every stride, taken from a limb unit.
The output, per stride: the vertical displacement, MinDiff and MaxDiff, and the ratio of the
first harmonic to the second. Nothing here is validated; it is where the work starts.
"""

import numpy as np


def displacement(acc, fs, f_low):
    """Vertical displacement from vertical acceleration, integrated twice in the frequency
    domain; everything below f_low, the drift, is dropped. acc in m/s², fs and f_low in Hz."""
    n = len(acc)
    spec = np.fft.rfft(acc - np.mean(acc))
    f = np.fft.rfftfreq(n, 1 / fs)
    w = 2 * np.pi * f
    out = np.zeros_like(spec)
    keep = f >= f_low
    out[keep] = -spec[keep] / (w[keep] ** 2)
    return np.fft.irfft(out, n)


def stride_frequency(disp, fs, f_min=0.3, f_max=3.0):
    """The stride frequency from the displacement's spectrum: a sound trot moves the trunk twice
    a stride, so the strongest line is twice the stride frequency."""
    spec = np.abs(np.fft.rfft(disp - np.mean(disp)))
    f = np.fft.rfftfreq(len(disp), 1 / fs)
    band = (f >= 2 * f_min) & (f <= 2 * f_max)
    return f[band][np.argmax(spec[band])] / 2


def strides(starts, disp):
    """The displacement cut into strides at the given start indices (from a limb unit)."""
    return [disp[a:b] for a, b in zip(starts[:-1], starts[1:]) if b > a + 4]


def min_max_diff(stride):
    """MinDiff and MaxDiff of one stride: the difference between the two minima and between the
    two maxima of the vertical displacement, first half minus second half, in the input's unit."""
    half = len(stride) // 2
    first, second = stride[:half], stride[half:]
    return first.min() - second.min(), first.max() - second.max()


def harmonic_ratio(stride):
    """The amplitude of the first harmonic over the second within one stride: zero for a
    perfectly symmetric trot, growing with the asymmetry."""
    spec = np.abs(np.fft.rfft(stride - np.mean(stride)))
    return spec[1] / spec[2] if spec[2] > 0 else float("inf")


def measures(acc, fs, starts):
    """MinDiff, MaxDiff and the harmonic ratio, each as the median over the strides."""
    f_stride = 1 / np.median(np.diff(starts)) * fs
    disp = displacement(acc, fs, f_low=0.5 * f_stride)
    rows = [(*min_max_diff(s), harmonic_ratio(s)) for s in strides(starts, disp)]
    rows = np.array(rows)
    return {"min_diff": float(np.median(rows[:, 0])),
            "max_diff": float(np.median(rows[:, 1])),
            "harmonic_ratio": float(np.median(rows[:, 2])),
            "strides": len(rows)}
