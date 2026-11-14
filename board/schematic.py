"""Materinka's four schematics as netlists for EasyEDA: every part of each board in BOARD.md with
its reference, its value and its LCSC code, every pin on its net as a global label — written as a
KiCad 7 schematic, which EasyEDA Pro imports (File → Import → KiCad). It is a netlist drawn, not
a drawing to look at: the parts stand in rows, the labels at their pins; whoever draws the
schematic replaces the boxes by the library's symbols and arranges them. The parts and nets are
nets.py's, the pins of the small parts BOARD.md's.

    python3 board/schematic.py           write board/belka.kicad_sch, zvjozdocka.kicad_sch,
                                         kryska.kicad_sch, povodok.kicad_sch
    python3 board/schematic.py --check   exit 1 if a file differs from what the script writes
"""

import os
import sys
import uuid

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import nets  # noqa: E402

HERE = os.path.dirname(os.path.abspath(__file__))
NS = uuid.UUID("2b5e0f8e-5a2c-4a1b-9f3e-1c2d3e4f5a6b")
PITCH = 2.54
PIN = 2.54
FONT = "(effects (font (size 1.27 1.27)))"
HIDDEN = "(effects (font (size 1.27 1.27)) hide)"
B, Z, K, P = nets.B, nets.Z, nets.K, nets.P

# --- the parts' values and LCSC codes, from BOARD.md's bill of materials ------------------------
LCSC = {
    "U3": ("CS CSNP1GCR01-BOW", "C2691593"), "U6": ("TI TPS7A2030PDQNR", "C5220164"),
    "U8": ("TI DRV5032ZEDBZR", "C266118"), "U9": ("TI DRV5032ZEDBZR", "C266118"),
    "D1": ("Hongjiacheng SMF12CA", "C19077507"), "D2": ("Hongjiacheng H5VUT2U", "C20615824"),
    "D3": ("Lite-On LTST-C195KGJRKT", "C125104"),
    "L1": ("Sunlord WPN201610H2R2MT 2,2 µH", "C97005"), "L2": ("Sunlord WPN201610H2R2MT 2,2 µH", "C97005"),
    "FB1": ("Murata BLM18PG121SN1D", "C14709"),
    "J1": ("Promax NF2347-05B", ""),
    "J3": ("Megastar ZX-TF-281.6J", "C7419839"),
    "TC1": ("Tag-Connect TC2030-NL", ""), "TP1": ("test pad", ""), "TP2": ("test pad", ""),
    "TP3": ("test pad", ""),
}
BY_BOARD = {
    "U1": {B: ("Fanstel BM20F", ""), Z: ("Fanstel BM20F", ""), K: ("Fanstel BM20M", ""), P: ("Fanstel WM02C20C", "")},
    "U2": {B: ("Nordic nPM1300-QEAA", "C7501206"), Z: ("Nordic nPM1300-QEAA", "C7501206"),
           K: ("Nordic nPM1300-QEAA", "C7501206"),
           P: ("Nordic nPM1300-QEAA", "C7501206")},
    "J2": {B: ("JST S3B-PH-SM4-TB", "C265101"), Z: ("JST S3B-PH-SM4-TB", "C265101"), K: ("JST SM03B-SRSS-TB or a like SH, 1,0 mm", ""),
           P: ("JST S3B-PH-SM4-TB", "C265101")},
    "U5": {B: ("TI TPS7A2030PDQNR", "C5220164"), Z: ("TI TPS7A2030PDQNR", "C5220164"), P: ("TI TPS7A2033PDQNR", "C2871641")},
    "U7": {B: ("ST LSM6DSV16X", "C5267406"),
           Z: ("TDK ICM-42686-P", "C19191783"),
           K: ("ST LSM6DSV320X", "C26633007")},
}
RES = {"100 kΩ": "C25741", "150 kΩ": "C22807", "10 kΩ": "C25744", "1,0 kΩ": "C11702", "0 Ω": "C17168",
       "1,5 kΩ": "C25867"}
CAP = {"100 µF": "C15008", "10 µF": "C13585", "2,2 µF": "C23630", "1 µF": "C28323", "100 nF": "C307331",
       "1 nF": "C1523"}
ICM_ONLY = {"R24", "C19", "C21", "C23", "C24"}   # fitted on Zvjozdočka alone, her ICM's
BASIC = set(RES.values()) | set(CAP.values()) | {"C14709"}
PREFERRED = {"C19077507", "C20615824"}


def code(ref, b):
    return BY_BOARD[ref][b] if ref in BY_BOARD else LCSC[ref]


def passive(kind, value):
    """A passive's value as BOARD.md's bill gives it, and its code: the value before its note
    (" — …", ", at …"), the code of the longest value in the table it begins with."""
    short = value.split(" —")[0].split(", ")[0]
    table = CAP if kind == "cap" else RES if kind == "res" else {}
    keys = [k for k in table if short == k or short.startswith(k + " ")]
    return short, table[max(keys, key=len)] if keys else ""


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


def parts(b):
    out = []
    for part in nets.ics(b):
        ref = part[0]
        value, lcsc = code(ref, b) if (ref in LCSC or ref in BY_BOARD) else (part[1], "")
        out.append((ref, value, lcsc, ic_pins(part)))
    rail = net_name(nets.RAIL[b])
    led, swd = {}, []
    for title, group in nets.groups(b):
        for kind, ref, value, a, c in group:
            a, c = net_name(a), net_name(c)
            if title.startswith("SWD"):
                swd.append((ref, value, a))
            elif kind in ("cap", "res", "ind", "bead", "tvs2"):
                short, lcsc = passive(kind, value)
                if ref in LCSC:
                    short, lcsc = LCSC[ref]
                out.append((ref, short, lcsc, [("1", "1", a), ("2", "2", c)]))
            elif kind == "led":
                led[ref] = (a, c)
            elif kind == "sw":
                vcc = "BAT" if ref == "U8" else rail
                out.append((ref, LCSC[ref][0], LCSC[ref][1],
                            [("1", "VCC", vcc), ("2", "OUT", a), ("3", "GND", "GND")]))
            elif kind == "esd":
                out.append((ref, LCSC[ref][0], LCSC[ref][1],
                            [("1", "IO1", "USB_DP"), ("2", "IO2", "USB_DM"), ("3", "GND", "GND")]))
            elif kind == "conn" and ref == "J1":
                out.append((ref, LCSC[ref][0], "", [("1", "VBUS", "VBUS"), ("2", "D+", "USB_DP"),
                                                   ("3", "GND", "GND"), ("4", "D-", "USB_DM"),
                                                   ("5", "VBUS", "VBUS")]))
            elif kind == "conn" and ref == "J2":
                out.append((ref, code(ref, b)[0], code(ref, b)[1],
                            [("1", "BAT", "BAT"), ("2", "GND", "GND"), ("3", "NTC", "NTC")]))
            elif kind == "pad" and ref.startswith("TP"):
                out.append((ref, "test pad", "", [("1", value, a)]))
    # the SWD pads as one part, TC1
    out.append(("TC1", LCSC["TC1"][0], "", swd))
    # the two-colour LED, one part of four pads: 1 and 2 the anodes, 3 the green cathode, 4 the red
    out.append(("D3", LCSC["D3"][0], LCSC["D3"][1],
                [("1", "A green", led["D3b"][0]), ("2", "A red", led["D3a"][0]),
                 ("3", "K green", led["D3b"][1]), ("4", "K red", led["D3a"][1])]))
    return out


def counts(b):
    """What JLCPCB places on the board — every part with an LCSC code but the module, the
    contact in the lid and, on the Belka, Zvjozdočka's own — and of how many kinds, how many basic."""
    placed = [lcsc for ref, value, lcsc, pins in parts(b)
              if lcsc and ref not in ("U1",) and (ref not in ICM_ONLY or b == Z)]
    kinds = set(placed)
    return len(placed), len(kinds), len(kinds & BASIC), len(kinds & PREFERRED)


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


def instance(ref, value, code, pins, x, y, places, half_h, root, project):
    empty = ref in ICM_ONLY and project != Z
    out = [f'(symbol (lib_id {q("Strelka:" + ref)}) (at {f(x)} {f(y)} 0) (unit 1) (in_bom yes)'
           f' (on_board yes) (dnp {"yes" if empty else "no"}) (fields_autoplaced)',
           f'  (uuid {q(uid("sym", ref))})',
           f'  (property "Reference" {q(ref)} (at {f(x)} {f(y - half_h - 1.27)} 0) {FONT})',
           f'  (property "Value" {q(value)} (at {f(x)} {f(y + half_h + 1.27)} 0) {FONT})',
           f'  (property "Footprint" "" (at {f(x)} {f(y)} 0) {HIDDEN})',
           f'  (property "Datasheet" "" (at {f(x)} {f(y)} 0) {HIDDEN})',
           f'  (property "LCSC" {q(code)} (at {f(x)} {f(y)} 0) {HIDDEN})',
           f'  (property "Fitted" {q("no, Zvjozdočka\'s alone" if empty else "yes")}'
           f' (at {f(x)} {f(y)} 0) {HIDDEN})']
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
    out.append(f'  (instances (project {q(project)} (path {q("/" + root)} (reference {q(ref)}) (unit 1))))')
    out.append(')')
    return "\n".join("  " + l for l in out), labels


def draw(b):
    root = uid("root", b)
    all_parts = parts(b)
    defs, insts, labels = [], [], []
    x, y, col_w = 60.0, 60.0, 0.0
    for ref, value, code_, pins in all_parts:
        d, places, half_w, half_h = symbol_def(ref, value, pins)
        defs.append(d)
        if y + 2 * half_h + 20 > 560:
            x += col_w + 70.0
            y, col_w = 60.0, 0.0
        yc = y + half_h
        inst, lbls = instance(ref, value, code_, pins, x + half_w + 25.0, yc, places, half_h, root, b)
        insts.append(inst)
        labels += lbls
        y += 2 * half_h + 25.0
        col_w = max(col_w, 2 * half_w + 50.0)
    out = ['(kicad_sch (version 20230121) (generator strelka)', f'  (uuid {q(root)})', '  (paper "A0")',
           f'  (title_block (title "NIC-Strelka — Materinka, {nets.NAME[b]}\'s board as a netlist, from'
           ' BOARD.md by board/schematic.py")'
           ' (comment 1 "Not a drawing: every pin on its net by a global label; for import into EasyEDA.")'
           ' (comment 2 "CERN-OHL-P v2. The property Fitted says yes, or no where the part is another board\'s; BOARD.md\'s'
           ' bill decides."))',
           '  (lib_symbols']
    out += defs
    out.append('  )')
    out += insts
    out += ["  " + l for l in labels]
    out.append('  (sheet_instances (path "/" (page "1")))')
    out.append(')')
    return "\n".join(out) + "\n"


if __name__ == "__main__":
    bad = False
    for b in nets.BOARDS:
        path = os.path.join(HERE, f"{b}.kicad_sch")
        text = draw(b)
        if "--check" in sys.argv:
            have = open(path, encoding="utf-8").read() if os.path.exists(path) else ""
            if have != text:
                print(f"board/{b}.kicad_sch differs from board/schematic.py")
                bad = True
        else:
            open(path, "w", encoding="utf-8").write(text)
            n_pins = sum(len(p[3]) for p in parts(b))
            print(f"wrote {path}: {len(parts(b))} parts, {n_pins} pins; placed, kinds, basic, "
                  f"preferred: {counts(b)}")
    sys.exit(1 if bad else 0)
