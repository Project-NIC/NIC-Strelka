"""Materinka's four boards counted: what JLCPCB places on each, of how many kinds, how many of them
basic and preferred, from board/schematic.py's parts, which are BOARD.md's."""

import os
import sys
import textwrap

sys.path.insert(0, os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), "board"))
import nets  # noqa: E402
import schematic  # noqa: E402


def line_count():
    """Each board's parts placed, their kinds, the basic and the preferred among them."""
    said = []
    for b in nets.BOARDS:
        placed, kinds, basic, preferred = schematic.counts(b)
        name = nets.NAME[b][0].upper() + nets.NAME[b][1:]
        said.append(f"**{name}**: {placed} parts placed, of {kinds} kinds, {basic} of them basic "
                    f"and {preferred} preferred")
    text = ("; ".join(said) + "; the module and the contact in the lid aside — Zvjozdočka the "
            "Belka's board, her ICM, R24 and TDK's capacitors fitted, their pads empty on the Belka.")
    return textwrap.fill(text, 100, break_on_hyphens=False)


if __name__ == "__main__":
    print(line_count())
