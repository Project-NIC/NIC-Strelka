"""A schematic as drawn, held against Materinka's netlist: the netlist EasyEDA exports from the
drawn schematic — KiCad's, Altium's (Protel), PADS's or Allegro's (Telesis) form, whichever it
offers — read, and every net compared by what it joins, not by its name, with the board's nets as
board/schematic.py writes them from BOARD.md. Said: each pin missing, each pin on a net it should
not be on, each net split in two or two nets joined, each part missing or extra; a net named
otherwise only noted. It catches the drawing's slips, not the design's: the design is BOARD.md's.

    python3 board/netcheck.py <board> <netlist>    belka, zvjozdocka, kryska or povodok; exit 1 on
                                                   any difference
    python3 board/netcheck.py --selftest           the four forms written from the board's own
                                                   nets, read back, and slips made in them found

A part's exposed pad or shell, here `pad` or `shell`, may carry any number in the drawing's
footprint (33, EP, 9–12, …): a pin of that part the board does not name, on the net the pad's or
shell's is, is taken for it.
"""

import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import schematic  # noqa: E402

PSEUDO = ("PAD", "SHELL")


def expected(board):
    """The board's pins: {(ref, pin): net}, a pin on no net left out, refs upper-case."""
    pins = {}
    for ref, value, lcsc, plist in schematic.parts(board):
        for num, name, net in plist:
            if net:
                pins[(ref.upper(), str(num).upper())] = net
    return pins


# --- the four forms ---------------------------------------------------------------------------
def read_kicad(text):
    nets = {}
    for chunk in re.split(r"\(net\s+\(code", text)[1:]:
        m = re.search(r'\(name\s+"([^"]*)"\)', chunk)
        if not m:
            continue
        nodes = re.findall(r'\(node\s+\(ref\s+"?([^"\s)]+)"?\)\s+\(pin\s+"?([^"\s)]+)"?\)', chunk)
        nets[m.group(1).lstrip("/")] = {(r.upper(), p.upper()) for r, p in nodes}
    return nets


def read_protel(text):
    nets = {}
    for m in re.finditer(r"\(\s*\n?([^\n()]*)\n(.*?)\)", text, re.S):
        name = m.group(1).strip()
        pins = set()
        for line in m.group(2).split():
            if "-" in line:
                r, p = line.rsplit("-", 1)
                pins.add((r.upper(), p.upper()))
        if name:
            nets[name] = pins
    return nets


def read_pads(text):
    nets, name = {}, None
    body = text.split("*NET*", 1)[1] if "*NET*" in text else text
    for line in body.splitlines():
        line = line.strip()
        if line.upper().startswith("*SIGNAL*"):
            name = line.split(None, 1)[1].split()[0]
            nets[name] = set()
        elif line.startswith("*"):
            name = None if line.upper().startswith("*END") else name
        elif name is not None:
            for tok in line.split():
                if "." in tok:
                    r, p = tok.rsplit(".", 1)
                    nets[name].add((r.upper(), p.upper()))
    return nets


def read_telesis(text):
    nets = {}
    body = text.split("$NETS", 1)[1].split("$END", 1)[0]
    for entry in re.split(r"(?<!,)\n(?=\S)", body.strip()):
        entry = entry.replace(",\n", " ").replace(",", " ")
        if ";" not in entry:
            continue
        name, rest = entry.split(";", 1)
        name = name.strip().strip("'")
        nets[name] = {(r.upper(), p.upper()) for r, p in
                      (t.rsplit(".", 1) for t in rest.split() if "." in t)}
    return nets


def read(text):
    """A netlist's nets, {name: {(ref, pin)}}, in whichever of the four forms it comes."""
    if "(export" in text and "(nets" in text:
        return read_kicad(text)
    if "*NET*" in text or "*SIGNAL*" in text:
        return read_pads(text)
    if "$NETS" in text:
        return read_telesis(text)
    return read_protel(text)


# --- the comparison -----------------------------------------------------------------------------
def compare(board, nets):
    """What differs, as lines; none when the drawing joins what the board joins."""
    want = expected(board)
    by_net = {}
    for key, net in want.items():
        by_net.setdefault(net, set()).add(key)
    refs_want = {r for r, _ in want}
    have = {}                                   # (ref, pin) -> the drawing's net
    for name, pins in nets.items():
        for key in pins:
            have[key] = name
    refs_have = {r for r, _ in have}
    said = []
    for r in sorted(refs_want - refs_have):
        said.append(f"part missing: {r}")
    for r in sorted(refs_have - refs_want):
        said.append(f"part not on the board: {r}")

    # a pad or shell named otherwise: a pin the board does not name, on the pseudo-pin's net
    named = {}
    for (r, p) in want:
        named.setdefault(r, set()).add(p)
    for (r, p), name in list(have.items()):
        if r in named and p not in named[r]:
            for ps in PSEUDO:
                if ps in named[r] and (r, ps) in want:
                    other = {k for k in nets[name]}
                    mates = by_net[want[(r, ps)]] - {(r, ps)}
                    if other & mates:
                        have.pop((r, p))
                        have[(r, ps)] = name
                        nets[name] = (nets[name] - {(r, p)}) | {(r, ps)}
                        break

    # pins the board joins that the drawing leaves on no net
    for key, net in sorted(want.items()):
        if key[0] in refs_have and key not in have:
            said.append(f"{key[0]} pin {key[1]}: on no net, the board has it on {net}")
    # each net of the drawing: the board's nets it joins
    home = {}                                   # the board's net -> the drawing's nets it is on
    for name, pins in sorted(nets.items()):
        mine = {}
        for key in pins:
            if key in want:
                mine.setdefault(want[key], set()).add(key)
            elif key[0] in refs_want:
                said.append(f"{key[0]} pin {key[1]}: on {name}, a pin the board leaves open")
        if not mine:
            continue
        main_net = max(mine, key=lambda n: (len(mine[n]), n))
        home.setdefault(main_net, set()).add(name)
        for net, keys in sorted(mine.items()):
            if net == main_net:
                continue
            if len(keys) <= 2 and len(by_net[net]) > len(keys):
                for r, p in sorted(keys):
                    said.append(f"{r} pin {p}: on {main_net}'s net, the board has it on {net}")
            else:
                said.append(f"{main_net} and {net} joined in the drawing's net {name}")
    # each net of the board: on one net of the drawing, not split
    for net, names in sorted(home.items()):
        if len(names) > 1:
            said.append(f"{net}: split in {len(names)} — " + ", ".join(sorted(names)))
    return sorted(set(said))


def notes(board, nets):
    """Nets the drawing names otherwise: a note, not a difference."""
    want = expected(board)
    out = []
    for name, pins in nets.items():
        mine = {want.get(k) for k in pins} - {None}
        if len(mine) == 1:
            n = mine.pop()
            if n.upper() != name.upper():
                out.append(f"{n} is called {name} in the drawing")
    return sorted(out)


# --- the self-test: the four forms written from the board's own nets ----------------------------
def write(form, nets):
    if form == "kicad":
        body = "".join(f'  (net (code "{i}") (name "/{n}")\n' +
                       "".join(f'    (node (ref "{r}") (pin "{p}"))\n' for r, p in sorted(k)) + "  )\n"
                       for i, (n, k) in enumerate(sorted(nets.items()), 1))
        return f"(export (version \"E\")\n (nets\n{body} )\n)\n"
    if form == "protel":
        return "".join(f"(\n{n}\n" + "".join(f"{r}-{p}\n" for r, p in sorted(k)) + ")\n"
                       for n, k in sorted(nets.items()))
    if form == "pads":
        return "*PADS-PCB*\n*NET*\n" + "".join(
            f"*SIGNAL* {n}\n" + " ".join(f"{r}.{p}" for r, p in sorted(k)) + "\n"
            for n, k in sorted(nets.items())) + "*END*\n"
    return "$NETS\n" + "".join(
        f"'{n}' ; " + " ,\n  ".join(f"{r}.{p}" for r, p in sorted(k)) + "\n"
        for n, k in sorted(nets.items())) + "$END\n"


def selftest():
    for board in ("belka", "zvjozdocka", "kryska", "povodok"):
        want = expected(board)
        nets = {}
        for key, net in want.items():
            nets.setdefault(net, set()).add(key)
        for form in ("kicad", "protel", "pads", "telesis"):
            got = read(write(form, nets))
            assert compare(board, got) == [], (board, form, compare(board, got)[:3])
    # the slips: a pin moved, two nets joined, a pin dropped, a part missing, the pad renamed
    want = expected("kryska")
    nets = {}
    for key, net in want.items():
        nets.setdefault(net, set()).add(key)
    slip = {n: set(k) for n, k in nets.items()}
    slip["SD_CS"].discard(("U3", "2"))
    slip["SD_DI"].add(("U3", "2"))
    said = compare("kryska", read(write("pads", slip)))
    assert said == ["U3 pin 2: on SD_DI's net, the board has it on SD_CS"], said
    slip = {n: set(k) for n, k in nets.items()}
    slip["SDA"] |= slip.pop("SCL")
    said = compare("kryska", read(write("kicad", slip)))
    assert said == ["SCL and SDA joined in the drawing's net SDA"] or \
        said == ["SDA and SCL joined in the drawing's net SDA"], said
    slip = {n: {k for k in ks if k[0] != "U9"} for n, ks in nets.items()}
    assert compare("kryska", read(write("protel", slip))) == ["part missing: U9"]
    slip = {n: {(r, "33" if (r, p) == ("U2", "PAD") else p) for r, p in ks} for n, ks in nets.items()}
    assert compare("kryska", read(write("telesis", slip))) == [], "the pad as 33"
    slip = {n: set(k) for n, k in nets.items()}
    slip["GND"].add(("U7", "10"))
    said = compare("kryska", read(write("pads", slip)))
    assert said == ["U7 pin 10: on GND, a pin the board leaves open"], said
    slip = {n: set(k) for n, k in nets.items()}
    slip["VBUS_B"] = {k for k in slip["VBUS"] if k[0] in ("J1", "D1")}
    slip["VBUS"] -= slip["VBUS_B"]
    said = compare("kryska", read(write("protel", slip)))
    assert said == ["VBUS: split in 2 — VBUS, VBUS_B"], said
    print("NETCHECK OK: four forms read back clean on the four boards; a pin moved, two nets "
          "joined, a net split, a part missing, an open pin tied and the pad numbered 33 all told")


def main(argv):
    if len(argv) == 2 and argv[1] == "--selftest":
        selftest()
        return 0
    if len(argv) != 3 or argv[1] not in ("belka", "zvjozdocka", "kryska", "povodok"):
        print(__doc__)
        return 2
    nets = read(open(argv[2], encoding="utf-8", errors="replace").read())
    said = compare(argv[1], nets)
    for line in notes(argv[1], nets):
        print("note: " + line)
    for line in said:
        print(line)
    print(f"{len(nets)} nets read; {'no difference' if not said else f'{len(said)} differences'}")
    return 1 if said else 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
