"""The board's parts and their nets, drawn as SVG from BOARD.md — not a schematic.

    python3 board/nets.py           write board/nets.svg
    python3 board/nets.py --check   exit 1 if the file differs from what the script draws

Every part and every net below is BOARD.md's; a pin's net is written at its stub, no wire is
drawn. It is a description of the parts, a help for whoever draws the schematic; the schematic
itself is not drawn. The parts stand in the order of BOARD.md.
"""

import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
OUT = os.path.join(HERE, "nets.svg")

W, H = 1760, 2024
PIN = 22          # pin pitch
STUB = 36         # a pin's stub
FONT = "font-family='DejaVu Sans, Arial, sans-serif'"

# --- the ICs and the slot: (ref, name, left pins, right pins); a pin is (number, name, net) ---
U1 = ("U1", "Fanstel BM20F or WM02C20C, the nRF54LM20A module", [
    ("B8,C7,C8", "VDD", "+3V0"),
    ("H7", "VBUS", "VBUS_MOD"),
    ("G7", "D+", "USB_DP"),
    ("G6", "D−", "USB_DM"),
    ("J3", "SWDIO", "SWDIO"),
    ("K3", "SWDCLK", "SWDCLK"),
    ("G2", "/Reset", "NRESET"),
    ("A1,A9,B1,B7,B9,C1,D1", "GND", "GND"),
    ("E1,F1,G1,H1,H8,J1,K1", "GND", "GND"),
    ("P1.04–P1.14", "the combo's inside", None),
    ("rest", "free GPIO", None),
], [
    ("C3", "P3.04", "IMU_SCK"),
    ("H2", "P3.05", "IMU_SDI"),
    ("C9", "P3.07", "IMU_SDO"),
    ("D7", "P3.02", "IMU_CS"),
    ("E6", "P3.01", "IMU_INT1"),
    ("K5", "P3.00", "IMU_CLKIN"),
    ("A7", "P3.03", "SD_CLK"),
    ("D4", "P3.08", "SD_DI"),
    ("K7", "P3.09", "SD_DO"),
    ("K2", "P3.10", "SD_CS"),
    ("D8", "P3.06", "SD_EN"),
    ("C2", "P0.04", "SCL"),
    ("E7", "P0.03", "SDA"),
    ("A5", "P0.00", "CHG_STAT"),
    ("D2", "P0.01", "CHG_PG"),
    ("D9", "P0.05", "MAG_A"),
    ("A6", "P0.02", "MAG_B"),
    ("H6", "P1.23", "HOLD"),
    ("J5", "P1.30", "LED_G"),
    ("K9", "P1.31", "LED_R"),
])

U2 = ("U2", "onsemi NCP361SNT1G, the USB supply's guard", [
    ("1", "IN", "VBUS"),
    ("2", "GND", "GND"),
    ("3", "EN", "GND"),
], [
    ("5", "OUT", "VBUS_MOD"),
    ("4", "FLAG", None),
])

U10 = ("U10", "TI TPS62933, the buck, 3,93 V from the cable", [
    ("3", "VIN", "VBUS"),
    ("2", "EN", None),
    ("1", "RT", "GND"),
    ("7", "SS", "SS"),
    ("4", "GND", "GND"),
], [
    ("5", "SW", "SW"),
    ("6", "BST", "BST"),
    ("8", "FB", "FB"),
])

U3 = ("U3", "TI BQ25170, the charger, linear; 3,50 V, 300 mA by resistors", [
    ("1", "IN", "VBUCK"),
    ("2", "ISET", "ISET"),
    ("3", "TS", "NTC"),
    ("4", "GND", "GND"),
], [
    ("8", "OUT", "BAT"),
    ("7", "VSET", "VSET"),
    ("6", "/PG", "CHG_PG"),
    ("5", "STAT", "CHG_STAT"),
    ("pad", "EP", "GND"),
])

U11 = ("U11", "TI LM66200, two ideal diodes to SYS; off while ON is high", [
    ("3", "VIN1", "VBUCK"),
    ("6", "VIN2", "BAT"),
    ("4", "ON", "ON"),
    ("1,5", "GND", "GND"),
], [
    ("2,7", "VOUT", "SYS"),
    ("8", "ST", None),
])

U4 = ("U4", "Analog Devices MAX17055, the fuel gauge", [
    ("1", "N.C.", None),
    ("2", "AIN", "BATT−"),
    ("3", "SCL", "SCL"),
    ("4", "SDA", "SDA"),
    ("5", "CSN", "GND"),
], [
    ("10", "BATT", "BAT"),
    ("9", "THRM", None),
    ("8", "ALRT", "BATT−"),
    ("7", "REG", "REG"),
    ("6", "CSP", "BATT−"),
    ("pad", "EP", "BATT−"),
])

U5 = ("U5", "TI TPS7A2030, LDO 3,0 V, +3V0", [
    ("4", "IN", "SYS"),
    ("2", "GND", "GND"),
    ("3", "EN", "SYS"),
], [
    ("1", "OUT", "+3V0"),
    ("5", "pad", "GND"),
])

U6 = ("U6", "TI TPS7A2030, LDO 3,0 V, +3V0_SD — the Belka alone", [
    ("4", "IN", "SYS"),
    ("2", "GND", "GND"),
    ("3", "EN", "SD_EN"),
], [
    ("1", "OUT", "+3V0_SD"),
    ("5", "pad", "GND"),
])

U7 = ("U7", "TDK ICM-42686-P or ICM-42688-P — the Belka alone", [
    ("1", "AP_SDO", "IMU_SDO"),
    ("2", "RESV", "GND"),
    ("3", "RESV", "GND"),
    ("4", "INT1", "IMU_INT1"),
    ("5", "VDDIO", "+3V0"),
    ("6", "GND", "GND"),
    ("7", "RESV", "GND"),
], [
    ("8", "VDD", "+3V0_IMU"),
    ("9", "INT2/FSYNC/CLKIN", "IMU_CLKIN"),
    ("10", "RESV", "GND"),
    ("11", "RESV", "GND"),
    ("12", "AP_CS", "IMU_CS"),
    ("13", "AP_SCLK", "IMU_SCK"),
    ("14", "AP_SDI", "IMU_SDI"),
])

J3 = ("J3", "Megastar ZX-TF-281.6J, the microSD slot — the Belka alone", [
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

# --- the two-terminal parts: (kind, ref, value, net a, net b) grouped by rail ---------------
# (B): fitted on the Belka alone.
GROUPS = [
    ("VBUS — the inlet, and the buck's input", [
        ("conn", "J1", "5-pin magnetic contact: 1 VBUS · 2 D+ · 3 GND · 4 D− · 5 VBUS", None, None),
        ("tvs2", "D1", "SMF12CA, two-way, 12 V stand-off", "GND", "VBUS"),
        ("esd", "D2", "TPD2E2U06DCKR: 1–2 on D+, D−, 3 GND", "USB_DP/DM", "GND"),
        ("cap", "C1", "1 µF 50 V 0805, at U2", "VBUS", "GND"),
        ("cap", "C2", "100 nF 0402, at U2", "VBUS", "GND"),
        ("cap", "C30", "10 µF 50 V 1206, at U10", "VBUS", "GND"),
        ("cap", "C31", "100 nF 0402, at U10", "VBUS", "GND"),
        ("pad", "TP3", "VBUS", "VBUS", None),
    ]),
    ("VBUCK — the buck's output, the charger's input", [
        ("ind", "L1", "2,2 µH, WPN201610H2R2MT", "SW", "VBUCK"),
        ("cap", "C34", "100 nF 0402", "BST", "SW"),
        ("res", "R4", "47 kΩ 1 %, the feedback's top", "VBUCK", "FB"),
        ("res", "R5", "12 kΩ 1 % — 3,93 V", "FB", "GND"),
        ("cap", "C35", "10 nF 0402, soft start", "SS", "GND"),
        ("cap", "C32", "22 µF 25 V 0805", "VBUCK", "GND"),
        ("cap", "C36", "22 µF 25 V 0805", "VBUCK", "GND"),
        ("cap", "C9", "100 nF 0402", "VBUCK", "GND"),
        ("cap", "C8", "1 µF 50 V 0805, at U3's IN", "VBUCK", "GND"),
        ("res", "R2", "1,0 kΩ 1 % — 300 mA", "ISET", "GND"),
        ("res", "R3", "100 kΩ 1 % — 3,50 V, LiFePO4", "VSET", "GND"),
    ]),
    ("SYS — the ideal diodes' output, the LDOs' input", [
        ("cap", "C7", "100 µF 6,3 V 1206", "SYS", "GND"),
        ("cap", "C10", "1 µF 50 V 0805, at the LDOs' inputs", "SYS", "GND"),
        ("cap", "C11", "100 nF 0402, at U5", "SYS", "GND"),
        ("cap", "C12", "100 nF 0402, at U6 (B)", "SYS", "GND"),
        ("pad", "TP1", "SYS", "SYS", None),
    ]),
    ("BAT — the cell's plus", [
        ("conn", "J2", "JST S3B-PH-SM4-TB: BAT · BATT− · NTC", None, None),
        ("cap", "C33", "1 µF 50 V 0805, at U3's OUT", "BAT", "GND"),
        ("cap", "C13", "100 nF 0402, at U4's BATT", "BAT", "BATT−"),
    ]),
    ("ON — the board's switch: on by magnet A, held by the processor", [
        ("res", "R20", "1 MΩ 1 % 0402", "BAT", "ON_R"),
        ("res", "R21", "1 MΩ 1 % 0402 — ON held high, the board off", "ON_R", "ON"),
        ("sw", "U8", "DRV5032ZE, magnet A: 1 VCC BAT · 2 OUT · 3 GND", "HALL_A", "GND"),
        ("cap", "C28", "100 nF 0402, at U8's VCC", "BAT", "GND"),
        ("diode", "D4", "BAT54, Schottky: magnet A pulls ON down", "ON", "HALL_A"),
        ("fet", "Q1", "BSS138, D ON, S GND, G HOLD — the hold", "ON", "GND"),
        ("res", "R22", "10 MΩ 1 % 0603, Q1's gate", "HOLD", "GND"),
        ("cap", "C37", "100 nF 0402 — Q1 held through a reset", "HOLD", "GND"),
        ("fet", "Q2", "BSS138, D HALL_A, S MAG_A, G +3V0 — the reading", "HALL_A", "MAG_A"),
        ("fet", "Q3", "BSS138, D ON, S GND, G VBUCK — Povodok alone", "ON", "GND"),
    ]),
    ("BATT− — the cell's minus, the gauge's side", [
        ("res", "R1", "20 mΩ 1 % 0805, the sense resistor", "BATT−", "GND"),
        ("cap", "C14", "470 nF 25 V 0603, at REG", "REG", "BATT−"),
    ]),
    ("+3V0 — U5's rail", [
        ("cap", "C15", "1 µF 50 V 0805, at U5", "+3V0", "GND"),
        ("cap", "C16", "100 nF 0402, at U5", "+3V0", "GND"),
        ("cap", "C17", "100 µF 6,3 V 1206, at U1", "+3V0", "GND"),
        ("cap", "C18", "100 nF 0402, at U1", "+3V0", "GND"),
        ("cap", "C27", "100 nF 0402, at U7 VDDIO (B)", "+3V0", "GND"),
        ("cap", "C29", "100 nF 0402, at U9", "+3V0", "GND"),
        ("res", "R6", "10 kΩ", "+3V0", "CHG_STAT"),
        ("res", "R7", "100 kΩ", "+3V0", "CHG_PG"),
        ("res", "R8", "10 kΩ", "+3V0", "SCL"),
        ("res", "R9", "10 kΩ", "+3V0", "SDA"),
        ("res", "R11", "100 kΩ (B)", "+3V0", "IMU_CS"),
        ("sw", "U9", "DRV5032ZE, magnet B: 1 VCC +3V0 · 2 OUT · 3 GND", "MAG_B", "GND"),
        ("led", "D3a", "LTST-C195KGJRKT red, pads 2→4", "+3V0", "LED_RK"),
        ("res", "R18", "1,0 kΩ", "LED_RK", "LED_R"),
        ("led", "D3b", "LTST-C195KGJRKT green, pads 1→3", "+3V0", "LED_GK"),
        ("res", "R17", "1,0 kΩ", "LED_GK", "LED_G"),
    ]),
    ("VBUS_MOD — the processor's USB supply", [
        ("cap", "C4", "1 µF 50 V 0805, at U2", "VBUS_MOD", "GND"),
        ("cap", "C5", "1 µF 50 V 0805, at H7", "VBUS_MOD", "GND"),
        ("cap", "C6", "100 nF 0402, at H7", "VBUS_MOD", "GND"),
    ]),
    ("SWD — Tag-Connect's TC2030-NL pads", [
        ("pad", "1", "VCC", "+3V0", None),
        ("pad", "2", "SWDIO", "SWDIO", None),
        ("pad", "3", "nRESET", "NRESET", None),
        ("pad", "4", "SWCLK", "SWDCLK", None),
        ("pad", "5", "GND", "GND", None),
        ("pad", "6", "SWO, open", None, None),
    ]),
    ("+3V0_SD — U6's rail, the card (B)", [
        ("cap", "C19", "1 µF 50 V 0805, at J3", "+3V0_SD", "GND"),
        ("cap", "C20", "100 nF 0402, at J3", "+3V0_SD", "GND"),
        ("res", "R12", "100 kΩ", "+3V0_SD", "SD_DAT2"),
        ("res", "R13", "100 kΩ", "+3V0_SD", "SD_CS"),
        ("res", "R14", "100 kΩ", "+3V0_SD", "SD_DI"),
        ("res", "R15", "100 kΩ", "+3V0_SD", "SD_DO"),
        ("res", "R16", "100 kΩ", "+3V0_SD", "SD_DAT1"),
        ("pad", "TP2", "+3V0_SD", "+3V0_SD", None),
    ]),
    ("+3V0_IMU — the sensor's supply, a π (B)", [
        ("cap", "C21", "100 nF 0402, before the bead", "+3V0", "GND"),
        ("bead", "FB1", "BLM18PG121SN1D, 120 Ω at 100 MHz", "+3V0", "+3V0_IMU"),
        ("cap", "C22", "1 µF 50 V 0805", "+3V0_IMU", "GND"),
        ("cap", "C23", "1 µF 50 V 0805", "+3V0_IMU", "GND"),
        ("cap", "C24", "100 nF 0402", "+3V0_IMU", "GND"),
        ("cap", "C25", "100 nF 0402", "+3V0_IMU", "GND"),
        ("cap", "C26", "1 nF 50 V 0402, nearest VDD", "+3V0_IMU", "GND"),
    ]),
]

# the groups' columns, by their index in GROUPS
COLUMNS = [[0, 1, 5, 7], [2, 3, 4, 10], [6, 9, 8]]


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


def draw():
    out = [f"<svg xmlns='http://www.w3.org/2000/svg' width='{W}' height='{H}' viewBox='0 0 {W} {H}'>",
           f"<rect width='{W}' height='{H}' fill='#fff'/>",
           text(30, 40, "NIC-Strelka — the board's parts and their nets, from BOARD.md (not a schematic)", 20, "start", "bold"),
           text(30, 62, "Flat: every pin ends in its net's name; a red cross is a pin left unconnected. "
                        "One board, two fittings: (B) and \"the Belka alone\" mark the parts Povodok goes without. "
                        "A buck and a charger set by resistors; U11, the switch, on by magnet A; the cell on J2. Drawn by board/nets.py.", 11)]
    u1, h1 = ic(130, 110, 300, U1)
    out.append(u1)
    y = 110
    for part in (U2, U10, U3, U4):
        g, h = ic(700, y, 260, part)
        out.append(g)
        y += h + 60
    y2 = 110
    for part in (U11, U5, U6, U7, J3):
        g, h = ic(1270, y2, 300, part)
        out.append(g)
        y2 += h + 60
    y0 = max(110 + h1, y, y2) + 30
    col_x = [30, 620, 1210]
    for c, idx in enumerate(COLUMNS):
        cy = y0
        for i in idx:
            title, parts = GROUPS[i]
            g, gh = group(col_x[c], cy + 16, 540, title, parts)
            out.append(g)
            cy += gh + 44
    out.append(text(30, H - 16, "Licence: CERN-OHL-P v2. The values and the parts: BOARD.md; the sources: PARTS.md.", 10))
    out.append("</svg>")
    return "\n".join(out) + "\n"


if __name__ == "__main__":
    svg = draw()
    if "--check" in sys.argv:
        have = open(OUT, encoding="utf-8").read() if os.path.exists(OUT) else ""
        if have != svg:
            print("board/nets.svg differs from board/nets.py")
            sys.exit(1)
        sys.exit(0)
    open(OUT, "w", encoding="utf-8").write(svg)
    print(f"wrote {OUT}")
