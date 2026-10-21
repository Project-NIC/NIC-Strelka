"""The board's schematic as a netlist for EasyEDA: every part of BOARD.md with its reference, its
value and its LCSC code, every pin on its net as a global label — written as a KiCad 7 schematic,
which EasyEDA Pro imports (File → Import → KiCad). It is a netlist drawn, not a drawing to look
at: the parts stand in rows, the labels at their pins; whoever draws the schematic replaces the
boxes by the library's symbols and arranges them. The parts and nets are nets.py's, the pins of
the small parts BOARD.md's.

    python3 board/schematic.py           write board/strelka.kicad_sch
    python3 board/schematic.py --check   exit 1 if the file differs from what the script writes
"""

import os
import sys
import uuid

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import nets  # noqa: E402

OUT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "strelka.kicad_sch")
NS = uuid.UUID("2b5e0f8e-5a2c-4a1b-9f3e-1c2d3e4f5a6b")
PITCH = 2.54
PIN = 2.54
FONT = "(effects (font (size 1.27 1.27)))"
HIDDEN = "(effects (font (size 1.27 1.27)) hide)"

# --- the parts' values and LCSC codes, from BOARD.md's bill of materials ------------------------
LCSC = {
    "U2": ("onsemi NCP361SNT1G", "C233714"), "U3": ("TI BQ25170DSGR", "C5208201"),
    "U4": ("Analog Devices MAX17055ETB+T", "C2976422"), "U5": ("TI TPS7A2030PDQNR", "C48581094"),
    "U6": ("TI TPS7A2030PDQNR", "C48581094"), "U7": ("TDK ICM-42686-P", "C19191783"),
    "U8": ("TI DRV5032ZEDBZR", "C266118"), "U9": ("TI DRV5032ZEDBZR", "C266118"),
    "U10": ("TI TPS62933DRLR", "C3200405"), "U11": ("TI LM66200DRLR", "C3235556"),
    "Q1": ("onsemi BSS138", "C52895"), "Q2": ("onsemi BSS138", "C52895"), "Q3": ("onsemi BSS138", "C52895"),
    "D1": ("SMF12CA", "C123801"), "D2": ("TI TPD2E2U06DCKR", "C1855726"),
    "D3": ("Lite-On LTST-C195KGJRKT", "C125104"), "D4": ("CJ BAT54", "C8590"),
    "L1": ("Sunlord WPN201610H2R2MT 2,2 µH", "C97005"), "FB1": ("Murata BLM18PG121SN1D", "C14709"),
    "R1": ("Ralec RLM10FTSMR020 20 mΩ", "C163095"),
    "J1": ("Promax NF2347-05B", ""), "J2": ("JST S3B-PH-SM4-TB", "C265101"),
    "J3": ("Megastar ZX-TF-281.6J", "C7419839"), "U1": ("Fanstel BM20F / WM02C20C", ""),
    "TC1": ("Tag-Connect TC2030-NL", ""), "TP1": ("test pad", ""), "TP2": ("test pad", ""),
    "TP3": ("test pad", ""),
}
RES = {"100 kΩ": "C25741", "1 MΩ": "C26083", "10 MΩ": "C7250", "47 kΩ": "C25792", "12 kΩ": "C25752",
       "10 kΩ": "C25744", "1,0 kΩ": "C11702"}
CAP = {"100 µF": "C15008", "1 µF": "C28323", "100 nF": "C307331", "470 nF": "C1623", "10 µF": "C13585",
       "22 µF": "C45783", "10 nF": "C15195", "1 nF": "C1523"}
BELKA_ONLY = {"U6", "U7", "J3", "FB1", "R11", "R12", "R13", "R14", "R15", "R16", "C12", "C19", "C20",
              "C21", "C22", "C23", "C24", "C25", "C26", "C27"}
POVODOK_ONLY = {"Q3"}


def fitted(ref):
    return "Belka" if ref in BELKA_ONLY else "Povodok" if ref in POVODOK_ONLY else "both"


def net_name(net):
    return None if net is None else net.replace("−", "-")


# --- the parts as (ref, value, lcsc, [(pin number, pin name, net)]) ------------------------------
def ic_pins(part):
    """nets.py's IC: its pads split one a pin, the pseudo-pins of free GPIO left out."""
    ref, name, left, right = part
    pins = []
    for num, pname, net in left + right:
        if num in ("P1.04–P1.14", "rest"):
            continue
        for n in num.split(","):
            pins.append((n.strip(), pname, net_name(net)))
    return pins


def parts():
    out = []
    for part in (nets.U1, nets.U2, nets.U10, nets.U3, nets.U11, nets.U4, nets.U5, nets.U6,
                 nets.U7, nets.J3):
        ref = part[0]
        value, code = LCSC.get(ref, (part[1], ""))
        out.append((ref, value if ref != "U1" else LCSC["U1"][0], code, ic_pins(part)))
    led = {}
    for title, group in nets.GROUPS:
        for kind, ref, value, a, b in group:
            a, b = net_name(a), net_name(b)
            short = value.split(",")[0].split(" —")[0]
            if kind in ("cap", "res", "ind", "bead", "tvs2"):
                code = (CAP if kind == "cap" else RES if kind == "res" else {}).get(short.split(" 1 %")[0], "")
                if ref in LCSC:
                    short, code = LCSC[ref]
                out.append((ref, short, code, [("1", "1", a), ("2", "2", b)]))
            elif kind == "diode":
                out.append((ref, LCSC[ref][0], LCSC[ref][1], [("1", "A", a), ("2", "K", b)]))
            elif kind == "led":
                led[ref] = (a, b)
            elif kind == "fet":
                g = {"Q1": "HOLD", "Q2": "+3V0", "Q3": "VBUCK"}[ref]
                s = {"Q1": "GND", "Q2": "MAG_A", "Q3": "GND"}[ref]
                out.append((ref, LCSC[ref][0], LCSC[ref][1], [("1", "G", g), ("2", "S", s), ("3", "D", a)]))
            elif kind == "sw":
                vcc = "BAT" if ref == "U8" else "+3V0"
                out.append((ref, LCSC[ref][0], LCSC[ref][1], [("1", "VCC", vcc), ("2", "OUT", a), ("3", "GND", "GND")]))
            elif kind == "esd":
                out.append((ref, LCSC[ref][0], LCSC[ref][1], [("1", "IO1", "USB_DP"), ("2", "IO2", "USB_DM"), ("3", "GND", "GND")]))
            elif kind == "conn" and ref == "J1":
                out.append((ref, LCSC[ref][0], "", [("1", "VBUS", "VBUS"), ("2", "D+", "USB_DP"), ("3", "GND", "GND"), ("4", "D-", "USB_DM"), ("5", "VBUS", "VBUS")]))
            elif kind == "conn" and ref == "J2":
                out.append((ref, LCSC[ref][0], LCSC[ref][1], [("1", "BAT", "BAT"), ("2", "BATT-", "BATT-"), ("3", "NTC", "NTC")]))
            elif kind == "pad" and ref.startswith("TP"):
                out.append((ref, "test pad", "", [("1", value, a)]))
    # the SWD pads as one part, TC1
    swd = [g for t, g in nets.GROUPS if t.startswith("SWD")][0]
    out.append(("TC1", LCSC["TC1"][0], "", [(num, value, net_name(a)) for kind, num, value, a, b in swd]))
    # the two-colour LED, one part of four pads: 1 and 2 the anodes, 3 the green cathode, 4 the red
    out.append(("D3", LCSC["D3"][0], LCSC["D3"][1],
                [("1", "A green", led["D3b"][0]), ("2", "A red", led["D3a"][0]),
                 ("3", "K green", led["D3b"][1]), ("4", "K red", led["D3a"][1])]))
    return out


# --- KiCad 7 S-expressions -----------------------------------------------------------------------
def uid(*key):
    return str(uuid.uuid5(NS, ":".join(str(k) for k in key)))


def q(s):
    return '"' + s.replace("\\", "\\\\").replace('"', '\\"') + '"'


def f(x):
    return f"{x:.2f}".rstrip("0").rstrip(".") if x != int(x) else str(int(x))


def symbol_def(ref, value, pins):
    """A box with its pins, half of them on each side, the longest pin name setting the width."""
    n = len(pins)
    left, right = pins[:(n + 1) // 2], pins[(n + 1) // 2:]
    rows = max(len(left), len(right), 1)
    longest = max((len(p[1]) for p in pins), default=4)
    half_w = max(7.62, round((longest * 1.3 + 4) / PITCH) * PITCH / 2 + PITCH)
    top = (rows - 1) / 2 * PITCH
    half_h = top + PITCH
    out = [f'(symbol {q("Strelka:" + ref)} (pin_names (offset 1.016)) (in_bom yes) (on_board yes)',
           f'  (property "Reference" {q(ref.rstrip("0123456789"))} (at 0 {f(half_h + 1.27)} 0) {FONT})',
           f'  (property "Value" {q(value)} (at 0 {f(-half_h - 1.27)} 0) {FONT})',
           f'  (property "Footprint" "" (at 0 0 0) {HIDDEN})',
           f'  (property "Datasheet" "" (at 0 0 0) {HIDDEN})',
           f'  (symbol {q(ref + "_0_1")}',
           f'    (rectangle (start {f(-half_w)} {f(half_h)}) (end {f(half_w)} {f(-half_h)})'
           f' (stroke (width 0.254) (type default)) (fill (type background)))',
           '  )', f'  (symbol {q(ref + "_1_1")}']
    places = {}
    for i, (num, name, net) in enumerate(left):
        y = top - i * PITCH
        out.append(f'    (pin passive line (at {f(-half_w - PIN)} {f(y)} 0) (length {f(PIN)})'
                   f' (name {q(name)} {FONT}) (number {q(num)} {FONT}))')
        places[num] = (-half_w - PIN, y, "left")
    for i, (num, name, net) in enumerate(right):
        y = top - i * PITCH
        out.append(f'    (pin passive line (at {f(half_w + PIN)} {f(y)} 180) (length {f(PIN)})'
                   f' (name {q(name)} {FONT}) (number {q(num)} {FONT}))')
        places[num] = (half_w + PIN, y, "right")
    out += ['  )', ')']
    return "\n".join("  " + l for l in out), places, half_w, half_h


def instance(ref, value, code, pins, x, y, places, half_h, root):
    out = [f'(symbol (lib_id {q("Strelka:" + ref)}) (at {f(x)} {f(y)} 0) (unit 1) (in_bom yes)'
           f' (on_board yes) (dnp no) (fields_autoplaced)',
           f'  (uuid {q(uid("sym", ref))})',
           f'  (property "Reference" {q(ref)} (at {f(x)} {f(y - half_h - 1.27)} 0) {FONT})',
           f'  (property "Value" {q(value)} (at {f(x)} {f(y + half_h + 1.27)} 0) {FONT})',
           f'  (property "Footprint" "" (at {f(x)} {f(y)} 0) {HIDDEN})',
           f'  (property "Datasheet" "" (at {f(x)} {f(y)} 0) {HIDDEN})',
           f'  (property "LCSC" {q(code)} (at {f(x)} {f(y)} 0) {HIDDEN})',
           f'  (property "Fitted" {q(fitted(ref))} (at {f(x)} {f(y)} 0) {HIDDEN})']
    labels = []
    for num, name, net in pins:
        out.append(f'  (pin {q(num)} (uuid {q(uid("pin", ref, num))}))')
        px, py, side = places[num]
        lx, ly = x + px, y - py
        if net is None:
            labels.append(f'(no_connect (at {f(lx)} {f(ly)}) (uuid {q(uid("nc", ref, num))}))')
        else:
            angle, justify = (180, "right") if side == "left" else (0, "left")
            labels.append(f'(global_label {q(net)} (shape input) (at {f(lx)} {f(ly)} {angle})'
                          f' (fields_autoplaced) (effects (font (size 1.27 1.27)) (justify {justify}))'
                          f' (uuid {q(uid("lbl", ref, num))})'
                          f' (property "Intersheetrefs" "${{INTERSHEET_REFS}}" (at {f(lx)} {f(ly)} 0) {HIDDEN}))')
    out.append(f'  (instances (project "strelka" (path {q("/" + root)} (reference {q(ref)}) (unit 1))))')
    out.append(')')
    return "\n".join("  " + l for l in out), labels


def draw():
    root = uid("root")
    all_parts = parts()
    defs, insts, labels = [], [], []
    x, y, col_w = 60.0, 60.0, 0.0
    for ref, value, code, pins in all_parts:
        d, places, half_w, half_h = symbol_def(ref, value, pins)
        defs.append(d)
        if y + 2 * half_h + 20 > 560:
            x += col_w + 70.0
            y, col_w = 60.0, 0.0
        yc = y + half_h
        inst, lbls = instance(ref, value, code, pins, x + half_w + 25.0, yc, places, half_h, root)
        insts.append(inst)
        labels += lbls
        y += 2 * half_h + 25.0
        col_w = max(col_w, 2 * half_w + 50.0)
    out = ['(kicad_sch (version 20230121) (generator strelka)', f'  (uuid {q(root)})', '  (paper "A0")',
           '  (title_block (title "NIC-Strelka — the board as a netlist, from BOARD.md by board/schematic.py")'
           ' (comment 1 "Not a drawing: every pin on its net by a global label; for import into EasyEDA.")'
           ' (comment 2 "CERN-OHL-P v2. The property Fitted says Belka, Povodok or both; BOARD.md\'s bill decides."))',
           '  (lib_symbols']
    out += defs
    out.append('  )')
    out += insts
    out += ["  " + l for l in labels]
    out.append('  (sheet_instances (path "/" (page "1")))')
    out.append(')')
    return "\n".join(out) + "\n"


if __name__ == "__main__":
    text = draw()
    if "--check" in sys.argv:
        have = open(OUT, encoding="utf-8").read() if os.path.exists(OUT) else ""
        if have != text:
            print("board/strelka.kicad_sch differs from board/schematic.py")
            sys.exit(1)
        sys.exit(0)
    open(OUT, "w", encoding="utf-8").write(text)
    n_parts = len(parts())
    n_pins = sum(len(p[3]) for p in parts())
    print(f"wrote {OUT}: {n_parts} parts, {n_pins} pins")
