"""Strelka's checks, run on every change: the generated tables in the pages match the scripts,
no line of a page runs past 100 characters outside tables and links, every relative link resolves,
and nothing struck from the design (`dead.py`) comes back into a page.

    python3 calc/check.py           check; exit 1 on any difference
    python3 calc/check.py --write   rewrite the generated blocks from the scripts
    python3 calc/check.py --stale   also list what the pages still to be rewritten carry
"""

import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
sys.path.insert(0, HERE)

import clock  # noqa: E402
import dead  # noqa: E402
import download  # noqa: E402
import link  # noqa: E402
import power  # noqa: E402
import memory  # noqa: E402
import noise  # noqa: E402
import radio  # noqa: E402

BLOCKS = {
    "radio-packets": radio.table_packets,
    "radio-syncs": radio.line_syncs,
    "radio-live": radio.table_live,
    "radio-slots": radio.line_slots,
    "radio-out": radio.line_out,
    "radio-pass": radio.line_pass,
    "radio-channels": radio.line_channels,
    "phone-live": radio.line_message,
    "radio-search": radio.line_search,
    "memory-hold": memory.table_hold,
    "memory-block": memory.line_block,
    "memory-rate": memory.line_rate,
    "memory-files": memory.line_files,
        "download-pace": download.line_pace,
    "commands-esb": download.line_frame,
    "download": download.table,
    "power": power.table,
    "power-radio": power.line_radio,
    "link-power": link.line_power,
    "link-mu": link.line_mu,
    "link": link.table,
    "clock": clock.table,
    "noise": noise.table,
}
PAGES = ["README.md", "README.cs.md", "README.ru.md", "STATUS.md", "NAMES.md", "WHY.md",
         "PRINCIPLE.md", "PARTS.md", "FIRMWARE.md", "APP.md", "PHONE.md", "BOARD.md",
         "ANALYSIS.md", "REGULATIONS.md", "RECORD.md", "COMMANDS.md"]
BLOCK = re.compile(r"<!-- calc:([\w-]+) -->\n(.*?)<!-- /calc -->", re.S)
LINK = re.compile(r"\]\(([^)#:]+?)(#[^)]*)?\)")


def struck(text):
    """What a page carries of the struck, as (what it was, how many times)."""
    for phrase in dead.ALLOWED:
        text = text.replace(phrase, "")
    found = []
    for pattern, what in dead.DEAD:
        n = len(re.findall(pattern, text, re.I))
        if n:
            found.append((what, n))
    return found


def dead_check():
    bad, stale = [], []
    for page in PAGES + ["firmware/README.md"]:
        if page in dead.EXEMPT:
            continue
        found = struck(open(os.path.join(ROOT, page), encoding="utf-8").read())
        if page in dead.STALE:
            if found:
                stale.append(f"{page}: " + "; ".join(f"{w} ×{n}" for w, n in found))
            else:
                bad.append(f"{page}: carries nothing struck now — take it off dead.STALE")
        else:
            bad += [f"{page}: struck, {w} ×{n}" for w, n in found]
    return bad, stale


def main(write):
    bad = []
    for page in PAGES:
        path = os.path.join(ROOT, page)
        text = open(path, encoding="utf-8").read()

        def fill(m):
            name = m.group(1)
            if name not in BLOCKS:
                bad.append(f"{page}: no script for block {name}")
                return m.group(0)
            want = BLOCKS[name]()
            if m.group(2) != want + "\n":
                bad.append(f"{page}: block {name} differs from its script")
            return f"<!-- calc:{name} -->\n{want}\n<!-- /calc -->"

        new = BLOCK.sub(fill, text)
        if write and new != text:
            open(path, "w", encoding="utf-8").write(new)
        in_code = False
        for i, line in enumerate(new.split("\n"), 1):
            if line.startswith("```"):
                in_code = not in_code
            if len(line) > 100 and not in_code and not line.startswith("|") and "http" not in line:
                bad.append(f"{page}:{i}: {len(line)} characters")
        for target, _ in LINK.findall(new):
            if not target.startswith("http") and not os.path.exists(os.path.join(ROOT, target)):
                bad.append(f"{page}: link to {target} does not resolve")
    import subprocess
    for script in ("nets.py", "schematic.py"):
        drawn = subprocess.run([sys.executable, os.path.join(ROOT, "board", script)]
                               + ([] if write else ["--check"]), capture_output=True, text=True)
        if drawn.returncode:
            bad.append(drawn.stdout.strip() or f"board/{script} failed")
    if write:
        bad = [b for b in bad if "differs" not in b]
    dead_bad, stale = dead_check()
    bad += dead_bad
    for b in bad:
        print(b)
    if stale and "--stale" in sys.argv:
        print("Still to be rewritten:")
        for s in stale:
            print("  " + s)
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main("--write" in sys.argv))
