"""Materinka in block diagrams, drawn as SVG from BOARD.md — the supply and the signals of the three
boards, their core the same — and the processor's two cores, who does what, from FIRMWARE.md.

    python3 board/blocks.py           write board/blocks-supply.svg, blocks-signals.svg, blocks-cores.svg
    python3 board/blocks.py --check   exit 1 if any file differs from what the script draws

A picture for the reader, not a schematic: each block is a part of BOARD.md by its reference, each
line a net of it by its name. The parts and nets themselves are BOARD.md's and nets.py's; this
script only places them.
"""

import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
FONT = "font-family='DejaVu Sans, Arial, sans-serif'"
INK, SOFT, FILL, BELKA = "#1f2328", "#57606a", "#f6f8fa", "#eef4fb"
RAIL = {"VBUS": "#b42318", "BAT": "#067647", "VSYS": "#175cd3", "+3V0": "#6941c6",
        "+3V0_SD": "#6941c6", "+3V3": "#6941c6", "VBUS_MOD": "#b42318", "SHPHLD": "#57606a"}

def esc(s):
    return s.replace("&", "&amp;").replace("<", "&lt;").replace(">", "&gt;")


def text(x, y, s, size=13, anchor="middle", weight="normal", fill=INK):
    return (f"<text x='{x}' y='{y}' font-size='{size}' text-anchor='{anchor}' "
            f"font-weight='{weight}' fill='{fill}' {FONT}>{esc(s)}</text>")


def box(x, y, w, h, title, *lines, belka=False):
    """A block: its reference and name in bold, a line or two below; dashed if not on every board."""
    dash = " stroke-dasharray='6 4'" if belka else ""
    out = [f"<rect x='{x}' y='{y}' width='{w}' height='{h}' rx='8' fill='{BELKA if belka else FILL}' "
           f"stroke='{INK}' stroke-width='1.4'{dash}/>"]
    n = 1 + len(lines)
    y0 = y + h / 2 - (n - 1) * 9 + 5
    out.append(text(x + w / 2, y0, title, 14, weight="bold"))
    for i, s in enumerate(lines):
        out.append(text(x + w / 2, y0 + 18 * (i + 1), s, 12, fill=SOFT))
    return "\n".join(out)


def line(points, net=None, label=None, at=0, dx=0, dy=-6, arrow=True, anchor="middle"):
    """A net as a polyline from the first point to the last, its name written by segment `at`."""
    colour = RAIL.get(net, INK)
    pts = " ".join(f"{x},{y}" for x, y in points)
    head = f" marker-end='url(#arrow{colour[1:]})'" if arrow else ""
    out = [f"<polyline points='{pts}' fill='none' stroke='{colour}' stroke-width='2'{head}/>"]
    if label:
        (x1, y1), (x2, y2) = points[at], points[at + 1]
        out.append(text((x1 + x2) / 2 + dx, (y1 + y2) / 2 + dy, label, 12, anchor, "bold", colour))
    return "\n".join(out)


def head(w, h, title):
    return "\n".join([
        f"<svg xmlns='http://www.w3.org/2000/svg' width='{w}' height='{h}' viewBox='0 0 {w} {h}'>",
        "<defs>" + "".join(
            f"<marker id='arrow{c[1:]}' viewBox='0 0 10 10' refX='9' refY='5' markerWidth='7' "
            f"markerHeight='7' orient='auto-start-reverse'><path d='M0,0 L10,5 L0,10 z' "
            f"fill='{c}'/></marker>" for c in sorted(set(RAIL.values()) | {INK})) + "</defs>",
        f"<rect width='{w}' height='{h}' fill='#ffffff'/>",
        text(20, 30, title, 17, "start", "bold"),
    ])


def foot(w, h, note, more=None, ref="The parts, the nets and their values: BOARD.md."):
    legend = [f"<rect x='20' y='{h - 44}' width='26' height='14' rx='3' fill='{BELKA}' "
              f"stroke='{INK}' stroke-dasharray='6 4'/>",
              text(54, h - 33, note, 12, "start", fill=SOFT)] if note else []
    return "\n".join(legend + [
        text(20, h - 56, more, 12, "start", fill=SOFT) if more else "",
        text(20, h - 12, "Licence: CERN-OHL-P v2. " + ref, 10, "start", fill=SOFT),
        "</svg>",
    ])

def supply():
    w, h = 1000, 760
    out = [head(w, h, "Materinka — the supply: from the contact to the rails, the core the same on "
                      "the three boards")]
    out += [
        box(20, 60, 150, 70, "J1", "the contact,", "the cable's 5 V"),
        box(190, 160, 110, 44, "D1", "the TVS"),
        box(330, 50, 320, 120, "U2, the PMIC", "Nordic's nPM1300; Kryska's", "nPM1304 or nPM1300:",
            "the charge, the power path, the off"),
        box(820, 60, 160, 70, "J2", "the cell, with", "its NTC"),
        box(820, 180, 160, 60, "U8", "magnet A, on", "the cell"),
        box(20, 250, 220, 56, "U1's VBUS", "the module's USB supply"),
        box(560, 230, 200, 56, "D3, the light", "on U2's LED0, LED1"),
        text(170, 388, "the Belka", 14, weight="bold"),
        text(500, 388, "Kryska", 14, weight="bold"),
        text(830, 388, "Povodok", 14, weight="bold"),
        box(30, 400, 130, 66, "U5", "LDO, 3,0 V,", "always on"),
        box(180, 400, 130, 66, "U6", "LDO, 3,0 V, on by", "the processor"),
        box(360, 400, 280, 66, "BUCK2, in U2", "3,0 V by R3 on VSET2; L1;", "L2 and C11, the filter"),
        box(690, 400, 280, 66, "U5", "LDO, 3,3 V, always on"),
        box(30, 560, 130, 96, "U1, the BM20F", "U7, the sensor;", "U9, U2's VDDIO,", "the pull-ups"),
        box(180, 560, 130, 96, "J3", "the microSD card"),
        box(360, 560, 280, 96, "U1, the BM20M", "U7, the sensor; U3, the flash;", "U9, U2's VDDIO,",
            "the pull-ups"),
        box(690, 560, 280, 96, "U1, the WM02C20C", "the nRF7002 inside;", "U9, U2's VDDIO,",
            "the pull-ups"),
    ]
    out += [
        line([(170, 95), (330, 95)], "VBUS", "VBUS"),
        line([(245, 95), (245, 160)], "VBUS", arrow=False),
        line([(820, 95), (650, 95)], "BAT", "BAT"),
        line([(900, 130), (900, 180)], "BAT", arrow=False),
        line([(820, 210), (600, 210), (600, 170)], "SHPHLD", "SHPHLD", dy=-6),
        line([(380, 170), (380, 278), (240, 278)], "VBUS_MOD", "VBUS_MOD", at=0, dx=-8, dy=4,
             anchor="end"),
        line([(480, 170), (480, 340)], "VSYS", "VSYS", dx=24, dy=4, arrow=False),
        line([(95, 340), (830, 340)], "VSYS", arrow=False),
        line([(660, 340), (660, 286)], "VSYS"),
        line([(95, 340), (95, 400)], "VSYS"),
        line([(245, 340), (245, 400)], "VSYS"),
        line([(500, 340), (500, 400)], "VSYS"),
        line([(830, 340), (830, 400)], "VSYS"),
        line([(95, 466), (95, 560)], "+3V0", "+3V0", dx=26, dy=4),
        line([(245, 466), (245, 560)], "+3V0_SD", "+3V0_SD", dx=36, dy=4),
        line([(500, 466), (500, 560)], "+3V0", "+3V0", dx=26, dy=4),
        line([(830, 466), (830, 560)], "+3V3", "+3V3", dx=26, dy=4),
    ]
    out.append(foot(w, h, None, "Below VSYS each board its own; U1's VBUS is U2's VBUSOUT, "
                                "guarded against over- and undervoltage."))
    return "\n".join(out) + "\n"


def signals():
    w, h = 1000, 600
    out = [head(w, h, "Materinka — the signals: what the processor talks to")]
    out.append(box(400, 50, 200, 470, "U1", "the module:", "the nRF54LM20A,", "the BM20F, the BM20M,",
                   "or the WM02C20C,", "the nRF7002 inside", "",
                   "the sensor read by", "its second core, the FLPR"))
    left = [
        (80, box(20, 50, 220, 60, "J1", "the contact, through D2"), "USB 2.0, 480 Mbit/s"),
        (250, box(20, 210, 220, 80, "U7", "the sensor: the Belka's", "and Kryska's", belka=True),
         "SPI, INT1, pin 9"),
        (375, box(20, 345, 220, 60, "J3", "the card, the Belka's", belka=True), "SPI, SD_EN to U6"),
        (465, box(20, 435, 220, 60, "U3", "the flash, Kryska's", belka=True), "SPI"),
    ]
    out += [
        box(760, 40, 220, 60, "U8", "magnet A"),
        box(760, 140, 220, 80, "U2", "the PMIC: the charge,", "the off, the gauge's measures"),
        box(760, 260, 220, 60, "D3", "the light, red and green"),
        box(760, 360, 220, 60, "U9", "magnet B"),
        line([(600, 180), (760, 180)], None, "I²C, PMIC_INT", arrow=False),
        line([(870, 100), (870, 140)], None, "SHPHLD", dx=8, dy=4, anchor="start", arrow=False),
        line([(870, 220), (870, 260)], None, "LED0, LED1", dx=8, dy=4, anchor="start", arrow=False),
        line([(600, 390), (760, 390)], None, "MAG_B", arrow=False),
    ]
    for y, b, label in left:
        out += [b, line([(240, y), (400, y)], None, label, arrow=False)]
    out.append(foot(w, h, "not on every board",
                    "Beside: the SWD pads, Tag-Connect's TC2030-NL, for the first firmware."))
    return "\n".join(out) + "\n"


def cores():
    """The processor's two cores and what each drives (FIRMWARE.md, The sensor on the FLPR)."""
    w, h = 1000, 680
    out = [head(w, h, "Strelka — the processor's two cores: who does what")]
    out += [
        box(380, 60, 240, 110, "Core B, the FLPR", "RISC-V, its own 96 KB of RAM",
            "the sensor alone: found, set,", "every sample read"),
        box(40, 60, 220, 110, "U7", "the sensor: LSM6DSV16X,", "LSM6DSV320X or ICM-42686-P",
            belka=True),
        box(40, 215, 220, 80, "TIMER22", "the data-ready's stamp,", "captured in hardware"),
        box(380, 235, 240, 100, "the ring, in RAM both see", "its head of 32 B: the commands,",
            "the state, the temperature;", "1 024 samples, each stamped"),
        box(330, 400, 340, 170, "Core A, the main one", "Arm Cortex-M33",
            "the set's time: the SYNC, the clock's loop,", "the sensor's clock on pin 9;",
            "the steps on the grid, the turn, the record;", "the commands, the gauge, the light"),
        box(40, 400, 220, 70, "the radio", "Strelka's frame and ESB"),
        box(40, 500, 220, 70, "J1, USB", "the disk, MCUboot's recovery"),
        box(740, 390, 220, 55, "J3 or U3", "the card or the flash", belka=True),
        box(740, 460, 220, 55, "U2, U9", "the PMIC, magnet B"),
        box(740, 530, 220, 55, "the nRF7002", "the Wi-Fi, Povodok alone"),
        line([(260, 115), (380, 115)], None, "SPIM21", arrow=False),
        line([(150, 170), (150, 215)], None, "INT1", dx=8, dy=4, anchor="start"),
        line([(260, 255), (320, 255), (320, 150), (380, 150)], None, "the stamp", at=1,
             dx=-8, dy=4, anchor="end", arrow=False),
        line([(500, 170), (500, 235)], None, "the samples in", dx=8, dy=4, anchor="start"),
        line([(500, 335), (500, 400)], None, "out every millisecond; the commands", dx=8,
             dy=4, anchor="start", arrow=False),
        line([(260, 435), (330, 435)], None, arrow=False),
        line([(260, 535), (330, 535)], None, arrow=False),
        line([(670, 417), (740, 417)], None, "SPIM20", arrow=False),
        line([(670, 487), (740, 487)], None, "I²C, pins", arrow=False),
        line([(670, 557), (740, 557)], None, "SPIM22", arrow=False),
    ]
    out.append(foot(w, h, "not on every board",
                    "Either core can drive any of the buses, one at a time, by turns agreed in "
                    "software.", "Who does what: FIRMWARE.md, The sensor on the FLPR."))
    return "\n".join(out) + "\n"


FILES = {"blocks-supply.svg": supply, "blocks-signals.svg": signals, "blocks-cores.svg": cores}

if __name__ == "__main__":
    bad = False
    for name, draw in FILES.items():
        path = os.path.join(HERE, name)
        svg = draw()
        if "--check" in sys.argv:
            have = open(path, encoding="utf-8").read() if os.path.exists(path) else ""
            if have != svg:
                print(f"board/{name} differs from board/blocks.py")
                bad = True
        else:
            open(path, "w", encoding="utf-8").write(svg)
            print(f"wrote {path}")
    sys.exit(1 if bad else 0)
