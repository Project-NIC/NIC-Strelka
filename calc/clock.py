"""The clock loop, rough: one Belka's sensor clock steered to the Povodoks' SYNCs, simulated.

Each period the Belka measures how far its grid lies from the set's — with the stamp's error, and
not at all in a period whose SYNCs it missed — and a PI loop lengthens or shortens CLKIN's periods
by whole ticks. The first second of SYNCs measures the frequency before the loop takes over."""

import math
import random

from params import (ACQUIRE_S, CRYSTAL_PPM, GAPS_S, LOOP_HZ, PERIOD_S, SEED, SYNC_ERR_S, SYNC_LOST,
                    TICK_S, WANDER_PPM_MIN)

RUN_S = 90.0      # one run
START_S = 5.0     # the record starts here, the loop locked
GAP_AT_S = 30.0   # a gap without a SYNC starts here


def simulate(gap_s=0.0):
    """Return the Belka's grid error against the set's, a value a period, in seconds."""
    rng = random.Random(SEED)
    ts = PERIOD_S
    wn = 2 * math.pi * LOOP_HZ
    kp, ki = 2 * 0.707 * wn * ts, (wn * ts) ** 2
    y0 = 2 * CRYSTAL_PPM * 1e-6              # two crystals at the opposite ends of their range
    wander = WANDER_PPM_MIN * 1e-6 / 60      # a second
    err = rng.gauss(0, SYNC_ERR_S)         # the first SYNC sets the phase
    integ = carry = 0.0
    seen = []
    out = []
    for k in range(int(RUN_S / ts)):
        t = k * ts
        heard = rng.random() >= SYNC_LOST and not (GAP_AT_S <= t < GAP_AT_S + gap_s)
        meas = err + rng.gauss(0, SYNC_ERR_S) if heard else None
        if t < ACQUIRE_S:                     # measure the frequency first, steer nothing
            if meas is not None:
                seen.append((t, meas))
            want = 0.0
        else:
            if seen:                          # the line through the first second's SYNCs
                n = len(seen)
                mt = sum(s for s, _ in seen) / n
                mm = sum(m for _, m in seen) / n
                slope = (sum((s - mt) * (m - mm) for s, m in seen)
                         / sum((s - mt) ** 2 for s, _ in seen))
                integ = slope * ts
                err -= mm + slope * (t - mt)  # one step of the phase, many ticks at once
                seen = []
                meas = None
            if meas is not None:
                integ += ki * meas
                want = -(kp * meas + integ)
            else:
                want = -integ                 # no SYNC: hold the frequency
        ticks = round((want + carry) / TICK_S)
        carry += want - ticks * TICK_S
        err += (y0 + wander * t) * ts + ticks * TICK_S
        out.append((t, err))
    return out


def us(seconds):
    v = seconds * 1e6
    digits = 2 if v < 1 else 1 if v < 10 else 0
    return f"{v:.{digits}f}".replace(".", ",") + " µs"


def lock_s(run, within=1e-6):
    """The time after which the error stays within a microsecond."""
    last_out = max((t for t, e in run if abs(e) >= within), default=0.0)
    return last_out + PERIOD_S


def table():
    run = simulate()
    locked = [abs(e) for t, e in run if t >= START_S]
    lock = lock_s([(t, e) for t, e in run if t < GAP_AT_S])
    rms = math.sqrt(sum(e * e for e in locked) / len(locked))
    free = 2 * CRYSTAL_PPM * 1e-6 * 60
    rows = ["| the case | how far a Belka's samples lie from the set's grid |", "|---|---|",
            f"| free-running, the crystals {2 * CRYSTAL_PPM} ppm apart | "
            f"{free * 1e3:.1f} ms a minute |".replace(".", ","),
            f"| locked within 1 µs, from the first SYNC | {lock:.1f} s |".replace(".", ","),
            f"| locked, {SYNC_LOST * 100:.0f} % of the periods without a SYNC | {us(rms)} RMS, "
            f"{us(max(locked))} at most |"]
    for gap in GAPS_S:
        run = simulate(gap)
        worst = max(abs(e) for t, e in run if GAP_AT_S <= t < GAP_AT_S + gap + 1)
        rows.append(f"| {gap} s without a SYNC | {us(worst)} at most |")
    return "\n".join(rows)


if __name__ == "__main__":
    print(table())
