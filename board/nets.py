"""Materinka, the base boards — the Belka's, Zvjozdočka's, Kryska's and Povodok's — their parts and nets, drawn as
SVG from BOARD.md: not a schematic.

    python3 board/nets.py           write board/nets-belka.svg, nets-kryska.svg, nets-povodok.svg
    python3 board/nets.py --check   exit 1 if a file differs from what the script draws

Every part and every net below is BOARD.md's; a pin's net is written at its stub, no wire is
drawn. It is a description of the parts, a help for whoever draws the schematic; the schematic
itself is not drawn. The parts stand in the order of BOARD.md, each with the boards it is on.
"""

import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
PIN = 22          # pin pitch
STUB = 36         # a pin's stub
FONT = "font-family='DejaVu Sans, Arial, sans-serif'"

B, Z, K, P = "belka", "zvjozdocka", "kryska", "povodok"
BOARDS = (B, Z, K, P)
NAME = {B: "the Belka", Z: "Zvjozdočka", K: "Kryska", P: "Povodok"}
RAIL = {B: "+3V0", Z: "+3V0", K: "+3V0", P: "+3V3"}
MODULE = {B: "Fanstel BM20F", Z: "Fanstel BM20F", K: "Fanstel BM20M", P: "Fanstel WM02C20C"}


def base(b):
    """Zvjozdočka is the Belka's board, TDK's ICM-42686-P fitted on its land and R24 with it."""
    return B if b == Z else b


# --- the ICs and the slot: (ref, name, left pins, right pins); a pin is (number, name, net) ---
def u1(b):
    left = [
        ("B8,C7,C8", "VDD", RAIL[b]),
        ("H7", "VBUS", "VBUS_MOD"),
        ("G7", "D+", "USB_DP"),
        ("G6", "D−", "USB_DM"),
        ("J3", "SWDIO", "SWDIO"),
        ("K3", "SWDCLK", "SWDCLK"),
        ("G2", "/Reset", "NRESET"),
        ("A1,A9,B1,B7,B9,C1,D1", "GND", "GND"),
        ("E1,F1,G1,H1,H8,J1,K1", "GND", "GND"),
        ("P1.04–P1.14", "the combo's inside" if b == P else "free GPIO", None),
        ("rest", "free GPIO", None),
    ]
    right = []
    if base(b) in (B, K):
        right += [("C3", "P3.04", "IMU_SCK"), ("H2", "P3.05", "IMU_SDI"), ("C9", "P3.07", "IMU_SDO"),
                  ("D7", "P3.02", "IMU_CS"), ("E6", "P3.01", "IMU_INT1"), ("K5", "P3.00", "IMU_CLKIN")]
    if base(b) in (B, K):        # the Belka's card, Kryska's SD NAND: the same pins
        right += [("A7", "P3.03", "SD_CLK"), ("D4", "P3.08", "SD_DI"), ("K7", "P3.09", "SD_DO"),
                  ("K2", "P3.10", "SD_CS"), ("D8", "P3.06", "SD_EN")]
    right += [("C2", "P0.04", "SCL"), ("E7", "P0.03", "SDA"), ("A5", "P0.00", "PMIC_INT"),
              ("A6", "P0.02", "MAG_B")]
    combo = ", the nRF54LM20A and the nRF7002" if b == P else ", the nRF54LM20A module"
    return ("U1", MODULE[b] + combo, left, right)


def u2(b):
    k = b == K
    left = [
        ("21", "VBUS", "VBUS"),
        ("19", "VBAT", "BAT"),
        ("18", "NTC", "NTC"),
        ("15", "SHPHLD", "HALL_A"),
        ("12", "VDDIO", RAIL[b]),
        ("13", "SDA", "SDA"),
        ("14", "SCL", "SCL"),
        ("7", "GPIO0", "PMIC_INT"),
        ("8", "GPIO1", None),
        ("9", "GPIO2", None),
        ("10", "GPIO3", None),
        ("11", "GPIO4", None),
        ("23", "CC1", None),
        ("24", "CC2", None),
        ("17", "VSET1", "GND"),
        ("16", "VSET2", "VSET2" if k else "GND"),
    ]
    right = [
        ("22", "VBUSOUT", "VBUS_MOD"),
        ("20", "VSYS", "VSYS"),
        ("4", "PVDD", "VSYS"),
        ("1", "VOUT1", "VSYS"),
        ("32", "VOUT2", "BUCK2" if k else "VSYS"),
        ("3", "SW1", None),
        ("5", "SW2", "SW2" if k else None),
        ("2", "PVSS1", "GND"),
        ("6", "PVSS2", "GND"),
        ("25", "LED0", "LED_R"),
        ("26", "LED1", "LED_G"),
        ("27", "LED2", None),
        ("28", "LSIN1", "GND"),
        ("30", "LSIN2", "GND"),
        ("29", "LSOUT1", "GND"),
        ("31", "LSOUT2", "GND"),
        ("pad", "AVSS", "GND"),
    ]
    name = ("Nordic nPM1300, the PMIC; BUCK2 at 3,0 V" if k
            else "Nordic nPM1300, the PMIC; its bucks unused")
    return ("U2", name, left, right)


U3 = ("U3", "CS CSNP1GCR01-BOW, SD NAND of 1 Gbit, in SPI mode — Kryska", [
    ("1", "SDD2, reserved", "SD_DAT2"),
    ("2", "CD/SDD3, CS", "SD_CS"),
    ("3", "SCLK", "SD_CLK"),
    ("4", "VSS", "GND"),
], [
    ("8", "VCC", "+3V0_SD"),
    ("7", "SDD1, reserved", "SD_DAT1"),
    ("6", "SDD0, DO", "SD_DO"),
    ("5", "CMD, DI", "SD_DI"),
])


def u5(b):
    name = ("TI TPS7A2033, LDO 3,3 V, +3V3" if b == P else "TI TPS7A2030, LDO 3,0 V, +3V0")
    return ("U5", name, [
        ("4", "IN", "VSYS"),
        ("2", "GND", "GND"),
        ("3", "EN", "VSYS"),
    ], [
        ("1", "OUT", RAIL[b]),
        ("5", "pad", "GND"),
    ])


U6 = ("U6", "TI TPS7A2030, LDO 3,0 V, +3V0_SD — the Belka and Kryska", [
    ("4", "IN", "VSYS"),
    ("2", "GND", "GND"),
    ("3", "EN", "SD_EN"),
], [
    ("1", "OUT", "+3V0_SD"),
    ("5", "pad", "GND"),
])


def u7(b):
    name = {B: "ST LSM6DSV16X, Ugoljok's LSM6DSV320X; the land takes Zvjozdočka's ICM",
            Z: "TDK ICM-42686-P, the ICM-42688-P an option; the Belka's land"}.get(b, "ST LSM6DSV320X")
    return ("U7", name, [
        ("1", "SDO", "IMU_SDO"),
        ("2", "SDx / RESV", "GND"),
        ("3", "SCx / RESV", "GND"),
        ("4", "INT1", "IMU_INT1"),
        ("5", "VDDIO", "+3V0"),
        ("6", "GND", "GND"),
        ("7", "GND / RESV", "GND"),
    ], [
        ("8", "VDD", "+3V0_IMU"),
        ("9", "INT2 / CLKIN", "IMU_PIN9"),
        ("10", "OCS_Aux / RESV, open", None),
        ("11", "SDO_Aux / RESV" if b in (B, Z) else "SDO_Aux, open",
         "IMU_PIN11" if b in (B, Z) else None),
        ("12", "CS", "IMU_CS"),
        ("13", "SCL / SCLK", "IMU_SCK"),
        ("14", "SDA / SDI", "IMU_SDI"),
    ])


J3 = ("J3", "Megastar ZX-TF-281.6J, the microSD slot — the Belka", [
    ("1", "DAT2", "SD_DAT2"),
    ("2", "CD/DAT3, CS", "SD_CS"),
    ("3", "CMD, DI", "SD_DI"),
    ("4", "VDD", "+3V0_SD"),
], [
    ("5", "CLK", "SD_CLK"),
    ("6", "VSS", "GND"),
    ("7", "DAT0, DO", "SD_DO"),
    ("8", "DAT1", "SD_DAT1"),
    ("shell", "", "GND"),
])


def ics(b):
    """The board's ICs and its slot, in BOARD.md's order."""
    out, b0 = [u1(b), u2(b)], base(b)
    if b0 == K:
        out.append(U3)
    if b0 in (B, P):
        out.append(u5(b))
    if b0 in (B, K):
        out.append(U6)
    if b0 in (B, K):
        out.append(u7(b))
    if b0 == B:
        out.append(J3)
    return out


# --- the two-terminal parts: (kind, ref, value, net a, net b, boards) grouped by rail ----------
ALL = BOARDS
R = "RAIL"        # the board's rail, +3V0 or +3V3
GROUPS = [
    ("VBUS — the inlet", [
        ("conn", "J1", "5-pin magnetic contact: 1 VBUS · 2 D+ · 3 GND · 4 D− · 5 VBUS", None, None, ALL),
        ("tvs2", "D1", "SMF12CA, two-way, 12 V stand-off", "GND", "VBUS", ALL),
        ("esd", "D2", "H5VUT2U: 1–2 on D+, D−, 3 GND", "USB_DP/DM", "GND", ALL),
        ("cap", "C1", "1 µF 50 V 0805, at U2's VBUS", "VBUS", "GND", ALL),
        ("cap", "C2", "100 nF 0402, at U2's VBUS", "VBUS", "GND", ALL),
        ("pad", "TP3", "VBUS", "VBUS", None, ALL),
    ]),
    ("VBUS_MOD — U2's VBUSOUT, the processor's USB supply", [
        ("cap", "C3", "1 µF 50 V 0805, at U2's VBUSOUT", "VBUS_MOD", "GND", ALL),
        ("cap", "C4", "10 µF 50 V 1206, at U1's H7", "VBUS_MOD", "GND", ALL),
    ]),
    ("VSYS — the power path's output, and the light", [
        ("cap", "C5", "10 µF 50 V 1206, at U2's VSYS and PVDD", "VSYS", "GND", ALL),
        ("cap", "C6", "100 nF 0402, at U2's VSYS", "VSYS", "GND", ALL),
        ("cap", "C26", "10 µF 50 V 1206, at U2's PVDD, to PVSS2", "VSYS", "GND", (K,)),
        ("cap", "C13", "1 µF 50 V 0805, at U5's input", "VSYS", "GND", (B, P)),
        ("cap", "C16", "1 µF 50 V 0805, at U6's input", "VSYS", "GND", (B, K)),
        ("led", "D3a", "LTST-C195KGJRKT red, pads 2→4", "VSYS", "LED_RK", ALL),
        ("led", "D3b", "LTST-C195KGJRKT green, pads 1→3", "VSYS", "LED_GK", ALL),
        ("res", "R17", "1,5 kΩ — the red's current, some 1 mA", "LED_RK", "LED_R", ALL),
        ("res", "R18", "1,5 kΩ — the green's current, some 1 mA", "LED_GK", "LED_G", ALL),
        ("pad", "TP1", "VSYS", "VSYS", None, ALL),
    ]),
    ("BAT — the cell's plus, and magnet A", [
        ("conn", "J2", "JST S3B-PH-SM4-TB: BAT · GND · NTC", None, None, (B, P)),
        ("conn", "J2", "JST SH, 1,0 mm, SM03B-SRSS-TB or a like one: BAT · GND · NTC", None, None,
         (K,)),
        ("cap", "C7", "2,2 µF 16 V 0603, at U2's VBAT", "BAT", "GND", ALL),
        ("sw", "U8", "DRV5032ZE, magnet A: 1 VCC BAT · 2 OUT · 3 GND", "HALL_A", "GND", ALL),
        ("cap", "C9", "100 nF 0402, at U8's VCC", "BAT", "GND", ALL),
    ]),
    ("BUCK2 — Kryska's buck and its filter", [
        ("res", "R3", "150 kΩ 1 % 0603 — 3,0 V from power-on", "VSET2", "GND", (K,)),
        ("ind", "L1", "2,2 µH, WPN201610H2R2MT, the buck's", "SW2", "BUCK2", (K,)),
        ("cap", "C27", "10 µF 50 V 1206, the buck's output", "BUCK2", "GND", (K,)),
        ("ind", "L2", "2,2 µH, WPN201610H2R2MT, the filter's", "BUCK2", "+3V0", (K,)),
    ]),
    ("RAIL — the board's rail: U5's on the Belka and Povodok, the filter's on Kryska", [
        ("cap", "C14", "1 µF 50 V 0805, at U5", R, "GND", (B, P)),
        ("cap", "C15", "100 nF 0402, at U5", R, "GND", (B, P)),
        ("cap", "C11", "100 µF 6,3 V 1206, at U1", R, "GND", ALL),
        ("cap", "C12", "100 nF 0402, at U1", R, "GND", ALL),
        ("cap", "C8", "100 nF 0402, at U2's VDDIO", R, "GND", ALL),
        ("cap", "C10", "100 nF 0402, at U9", R, "GND", ALL),
        ("cap", "C25", "100 nF 0402, at U7's VDDIO", R, "GND", (B, K)),
        ("res", "R8", "10 kΩ", R, "SCL", ALL),
        ("res", "R9", "10 kΩ", R, "SDA", ALL),
        ("res", "R11", "100 kΩ", R, "IMU_CS", (B, K)),
        ("sw", "U9", "DRV5032ZE, magnet B: 1 VCC rail · 2 OUT · 3 GND", "MAG_B", "GND", ALL),
        ("pad", "TP2", "+3V0", R, None, (K,)),
    ]),
    ("SWD — Tag-Connect's TC2030-NL pads", [
        ("pad", "1", "VCC", R, None, ALL),
        ("pad", "2", "SWDIO", "SWDIO", None, ALL),
        ("pad", "3", "nRESET", "NRESET", None, ALL),
        ("pad", "4", "SWCLK", "SWDCLK", None, ALL),
        ("pad", "5", "GND", "GND", None, ALL),
        ("pad", "6", "SWO, open", None, None, ALL),
    ]),
    ("+3V0_SD — U6's rail, the Belka's card, Kryska's SD NAND", [
        ("cap", "C17", "1 µF 50 V 0805, at J3", "+3V0_SD", "GND", (B,)),
        ("cap", "C29", "2,2 µF 16 V 0603, at U3, the SD NAND's own", "+3V0_SD", "GND", (K,)),
        ("cap", "C18", "100 nF 0402, at J3 or U3", "+3V0_SD", "GND", (B, K)),
        ("res", "R12", "100 kΩ", "+3V0_SD", "SD_DAT2", (B, K)),
        ("res", "R13", "100 kΩ", "+3V0_SD", "SD_CS", (B, K)),
        ("res", "R14", "100 kΩ", "+3V0_SD", "SD_DI", (B, K)),
        ("res", "R15", "100 kΩ", "+3V0_SD", "SD_DO", (B, K)),
        ("res", "R16", "100 kΩ", "+3V0_SD", "SD_DAT1", (B, K)),
        ("pad", "TP2", "+3V0_SD", "+3V0_SD", None, (B,)),
    ]),
    ("+3V0_IMU — the sensor's supply, a π, and its resistors", [
        ("cap", "C19", "100 nF 0402, before the bead — Zvjozdočka's", "+3V0", "GND", (B,)),
        ("bead", "FB1", "BLM18PG121SN1D, 120 Ω at 100 MHz", "+3V0", "+3V0_IMU", (B, K)),
        ("cap", "C20", "1 µF 50 V 0805, at VDD", "+3V0_IMU", "GND", (B, K)),
        ("cap", "C21", "1 µF 50 V 0805 — Zvjozdočka's", "+3V0_IMU", "GND", (B,)),
        ("cap", "C22", "100 nF 0402", "+3V0_IMU", "GND", (B, K)),
        ("cap", "C23", "100 nF 0402 — Zvjozdočka's", "+3V0_IMU", "GND", (B,)),
        ("cap", "C24", "1 nF 50 V 0402, nearest VDD — Zvjozdočka's", "+3V0_IMU", "GND", (B,)),
        ("res", "R23", "1,0 kΩ, pin 9's guard", "IMU_CLKIN", "IMU_PIN9", (B, K)),
        ("res", "R24", "0 Ω — an ICM alone, Zvjozdočka's", "IMU_PIN11", "GND", (B,)),
    ]),
]


def groups(b):
    """The board's groups: each part on it, the rail named; the empty groups left out."""
    out = []
    for title, parts in GROUPS:
        mine = [(kind, ref, value, RAIL[b] if a == R else a, RAIL[b] if c == R else c)
                for kind, ref, value, a, c, on in parts if base(b) in on]
        if mine:
            out.append((title.replace("RAIL", RAIL[b], 1), mine))
    return out


def esc(s):
    return s.replace("&", "&amp;").replace("<", "&lt;").replace(">", "&gt;")


def text(x, y, s, size=11, anchor="start", weight="normal", fill="#111"):
    return (f"<text x='{x}' y='{y}' font-size='{size}' text-anchor='{anchor}' "
            f"font-weight='{weight}' fill='{fill}' {FONT}>{esc(s)}</text>")


def net_label(x, y, net, side):
    """A net's name at a stub's end; an open pin gets a cross."""
    if net is None:
        return (f"<line x1='{x - 4}' y1='{y - 4}' x2='{x + 4}' y2='{y + 4}' stroke='#b00' stroke-width='1.5'/>"
                f"<line x1='{x - 4}' y1='{y + 4}' x2='{x + 4}' y2='{y - 4}' stroke='#b00' stroke-width='1.5'/>")
    if side == "left":
        return text(x - 6, y + 4, net, 11, "end", "bold", "#0a4")
    return text(x + 6, y + 4, net, 11, "start", "bold", "#0a4")


def ic(x, y, w, part):
    ref, name, left, right = part
    rows = max(len(left), len(right))
    h = rows * PIN + 30
    out = [f"<rect x='{x}' y='{y}' width='{w}' height='{h}' fill='#fff' stroke='#222' stroke-width='1.5'/>",
           text(x - STUB, y - 8, f"{ref} — {name}", 12, "start", "bold")]
    for i, (num, pname, net) in enumerate(left):
        py = y + 20 + i * PIN
        out.append(f"<line x1='{x - STUB}' y1='{py}' x2='{x}' y2='{py}' stroke='#222' stroke-width='1.2'/>")
        out.append(text(x + 6, py + 4, f"{num}  {pname}".strip(), 10))
        out.append(net_label(x - STUB, py, net, "left"))
    for i, (num, pname, net) in enumerate(right):
        py = y + 20 + i * PIN
        out.append(f"<line x1='{x + w}' y1='{py}' x2='{x + w + STUB}' y2='{py}' stroke='#222' stroke-width='1.2'/>")
        out.append(text(x + w - 6, py + 4, f"{pname}  {num}".strip(), 10, "end"))
        out.append(net_label(x + w + STUB, py, net, "right"))
    return "\n".join(out), h


def symbol(kind, x, y):
    """A two-terminal symbol centred at (x, y), 40 wide, its leads from x-30 to x+30."""
    s = "stroke='#222' stroke-width='1.5' fill='none'"
    lead = f"<line x1='{x - 30}' y1='{y}' x2='{x - 10}' y2='{y}' {s}/><line x1='{x + 10}' y1='{y}' x2='{x + 30}' y2='{y}' {s}/>"
    if kind == "cap":
        return lead + f"<line x1='{x - 3}' y1='{y - 9}' x2='{x - 3}' y2='{y + 9}' {s}/><line x1='{x + 3}' y1='{y - 9}' x2='{x + 3}' y2='{y + 9}' {s}/>".replace("x1='{x - 10}'", "")
    if kind in ("res",):
        return lead + f"<rect x='{x - 10}' y='{y - 5}' width='20' height='10' {s}/>"
    if kind == "ntc":
        return lead + f"<rect x='{x - 10}' y='{y - 5}' width='20' height='10' {s}/><line x1='{x - 12}' y1='{y + 9}' x2='{x + 12}' y2='{y - 9}' {s}/>"
    if kind == "ind":
        return lead + f"<path d='M{x - 10},{y} a5,5 0 0 1 10,0 a5,5 0 0 1 10,0' {s}/>"
    if kind == "bead":
        return lead + f"<rect x='{x - 10}' y='{y - 4}' width='20' height='8' fill='#222'/>"
    if kind == "tvs2":
        return lead + (f"<polygon points='{x - 10},{y - 7} {x - 10},{y + 7} {x},{y}' {s}/>"
                       f"<polygon points='{x + 10},{y - 7} {x + 10},{y + 7} {x},{y}' {s}/>"
                       f"<line x1='{x}' y1='{y - 8}' x2='{x}' y2='{y + 8}' {s}/>")
    if kind == "fet":
        return lead + (f"<line x1='{x - 10}' y1='{y}' x2='{x - 10}' y2='{y - 6}' {s}/>"
                       f"<line x1='{x + 10}' y1='{y}' x2='{x + 10}' y2='{y - 6}' {s}/>"
                       f"<line x1='{x - 12}' y1='{y - 6}' x2='{x + 12}' y2='{y - 6}' {s}/>"
                       f"<line x1='{x - 12}' y1='{y - 11}' x2='{x + 12}' y2='{y - 11}' {s}/>"
                       f"<line x1='{x}' y1='{y - 11}' x2='{x}' y2='{y - 16}' {s}/>")
    if kind in ("diode", "tvs", "led", "esd"):
        tri = f"<polygon points='{x - 8},{y - 8} {x - 8},{y + 8} {x + 6},{y}' {s}/><line x1='{x + 6}' y1='{y - 8}' x2='{x + 6}' y2='{y + 8}' {s}/>"
        if kind == "tvs":
            tri += f"<line x1='{x + 6}' y1='{y - 8}' x2='{x + 2}' y2='{y - 8}' {s}/><line x1='{x + 6}' y1='{y + 8}' x2='{x + 10}' y2='{y + 8}' {s}/>"
        if kind == "led":
            tri += f"<line x1='{x - 2}' y1='{y - 10}' x2='{x + 3}' y2='{y - 15}' {s}/><line x1='{x + 3}' y1='{y - 10}' x2='{x + 8}' y2='{y - 15}' {s}/>"
        if kind == "esd":
            tri += f"<line x1='{x + 6}' y1='{y - 8}' x2='{x + 2}' y2='{y - 8}' {s}/><line x1='{x + 6}' y1='{y + 8}' x2='{x + 10}' y2='{y + 8}' {s}/>"
        return lead + tri
    if kind == "sw":
        return lead + f"<line x1='{x - 10}' y1='{y}' x2='{x + 8}' y2='{y - 9}' {s}/><circle cx='{x - 10}' cy='{y}' r='2' fill='#222'/><circle cx='{x + 10}' cy='{y}' r='2' fill='#222'/>"
    if kind == "pad":
        return f"<line x1='{x - 30}' y1='{y}' x2='{x - 6}' y2='{y}' {s}/><circle cx='{x}' cy='{y}' r='6' {s}/>"
    if kind == "conn":
        return f"<rect x='{x - 30}' y='{y - 10}' width='60' height='20' {s}/>"
    return lead


def group(x, y, w, title, parts):
    out = [text(x, y - 6, title, 12, "start", "bold")]
    out.append(f"<rect x='{x}' y='{y}' width='{w}' height='{len(parts) * 26 + 10}' fill='#fafafa' stroke='#999' stroke-width='1'/>")
    for i, (kind, ref, value, a, b) in enumerate(parts):
        py = y + 18 + i * 26
        sx = x + 150
        out.append(symbol(kind, sx, py))
        if a:
            out.append(text(sx - 34, py + 4, a, 10, "end", "bold", "#0a4"))
        if b:
            out.append(text(sx + 34, py + 4, b, 10, "start", "bold", "#0a4"))
        out.append(text(x + 8, py + 4, ref, 10, "start", "bold"))
        out.append(text(sx + (120 if b else 40), py + 4, value, 10))
    return "\n".join(out), len(parts) * 26 + 10


def draw(b):
    gs = groups(b)
    parts = ics(b)
    out = []
    u1, h1 = ic(130, 110, 300, parts[0])
    out.append(u1)
    mid, right = parts[1:3], parts[3:]
    y = 110
    for part in mid:
        g, h = ic(720, y, 270, part)
        out.append(g)
        y += h + 60
    y2 = 110
    for part in right:
        g, h = ic(1290, y2, 300, part)
        out.append(g)
        y2 += h + 60
    y0 = max(110 + h1, y, y2) + 30
    col_x = [30, 620, 1210]
    heights = [len(p) * 26 + 10 + 44 for t, p in gs]
    cols, cy = [[], [], []], [0, 0, 0]
    for i, h in enumerate(heights):       # each group into the shortest column so far
        c = cy.index(min(cy))
        cols[c].append(i)
        cy[c] += h
    for c, idx in enumerate(cols):
        yy = y0
        for i in idx:
            title, ps = gs[i]
            g, gh = group(col_x[c], yy + 16, 540, title, ps)
            out.append(g)
            yy += gh + 44
    w, h = 1760, y0 + max(cy) + 60
    top = [f"<svg xmlns='http://www.w3.org/2000/svg' width='{w}' height='{h}' viewBox='0 0 {w} {h}'>",
           f"<rect width='{w}' height='{h}' fill='#fff'/>",
           text(30, 40, f"NIC-Strelka — Materinka, {NAME[b]}'s board: its parts and their nets, from "
                        "BOARD.md (not a schematic)", 20, "start", "bold"),
           text(30, 62, "Flat: every pin ends in its net's name; a red cross is a pin left unconnected. "
                        "The core — the contact, U2 the PMIC, the magnets, the light, the cell — the "
                        "same on the three boards. Drawn by board/nets.py.", 11)]
    out.append(text(30, h - 16, "Licence: CERN-OHL-P v2. The values and the parts: BOARD.md; the "
                                "sources: PARTS.md.", 10))
    out.append("</svg>")
    return "\n".join(top + out) + "\n"


if __name__ == "__main__":
    bad = False
    for b in BOARDS:
        path = os.path.join(HERE, f"nets-{b}.svg")
        svg = draw(b)
        if "--check" in sys.argv:
            have = open(path, encoding="utf-8").read() if os.path.exists(path) else ""
            if have != svg:
                print(f"board/nets-{b}.svg differs from board/nets.py")
                bad = True
        else:
            open(path, "w", encoding="utf-8").write(svg)
            print(f"wrote {path}")
    sys.exit(1 if bad else 0)
