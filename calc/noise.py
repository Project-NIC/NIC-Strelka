"""The sensor's noise against the motion to be seen, rough: what the accelerometer's white noise
leaves in the vertical displacement after the double integration, from half the stride's
frequency up to half the record's rate; and what the gyro's wandering tilt leaks into it."""

import math

from params import (ACC_NOISE_G, DEFAULT_HZ, GYRO_NOISE_DPS, SIDEWAYS_G, STRIDES_HZ,
                    TILT_S)

G = 9.80665


def displacement_noise_m(stride_hz):
    f1, f2 = stride_hz / 2, DEFAULT_HZ / 2
    density = ACC_NOISE_G * G                     # m/s² a √Hz
    return density / (2 * math.pi) ** 2 * math.sqrt((f1 ** -3 - f2 ** -3) / 3)


def tilt_leak_m(stride_hz):
    tilt = math.radians(GYRO_NOISE_DPS * math.sqrt(TILT_S))
    return SIDEWAYS_G * G * tilt / (2 * math.pi * stride_hz) ** 2


def mm(metres):
    return f"{metres * 1e3:.2f}".replace(".", ",") + " mm"


def table():
    rows = ["| the motion | its stride | the accelerometer's noise | the gyro's tilt, leaked |",
            "|---|---|---|---|"]
    for name, hz in STRIDES_HZ:
        stride = f"{hz:g}".replace(".", ",")
        rows.append(f"| {name} | {stride} Hz | {mm(displacement_noise_m(hz))} | "
                    f"{mm(tilt_leak_m(hz))} |")
    return "\n".join(rows)


if __name__ == "__main__":
    print(table())
