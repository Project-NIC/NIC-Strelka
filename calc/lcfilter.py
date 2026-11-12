"""Kryska's LC filter after the buck, L2 and C11 (BOARD.md, Kryska): its corner, impedance, Q and
peak, and its drop at the SD NAND's current — computed here, typed nowhere."""

import math
import textwrap

from link import dec
from params import FILTER_C_F, FILTER_L_H, FILTER_R_OHM, SDNAND_MA


def figures():
    corner = 1 / (2 * math.pi * math.sqrt(FILTER_L_H * FILTER_C_F))
    z = math.sqrt(FILTER_L_H / FILTER_C_F)
    q = z / FILTER_R_OHM
    at_corner = 20 * math.log10(q)                      # |H| = Q at the corner, unloaded
    peak = 20 * math.log10(q / math.sqrt(1 - 1 / (4 * q * q))) if q > 2 ** -0.5 else 0.0
    drop = FILTER_R_OHM * SDNAND_MA[1] * 1e-3
    return corner, z, q, at_corner, peak, drop


def line_filter():
    corner, z, q, at_corner, peak, drop = figures()
    text = (f"C11 keeps some {dec(FILTER_C_F * 1e6, '.1f')} µF at 3,0 V. The corner, 1 / (2π √(L × "
            f"C)), is some {corner / 1e3:.0f} kHz; the impedance √(L / C) {dec(z, '.2f')} Ω; the "
            f"Q with L2's {FILTER_R_OHM * 1e3:.0f} mΩ {dec(q, '.1f')}, so {dec(at_corner, '.1f')} dB "
            f"at the corner and a peak of {dec(peak, '.1f')} dB just under it, falling 40 dB a "
            f"decade above. Outside the buck's loop, L2 drops some {drop * 1e3:.0f} mV at the "
            f"SD NAND's {SDNAND_MA[1]} mA.")
    return textwrap.fill(text, 100, break_on_hyphens=False)


if __name__ == "__main__":
    print(line_filter())
