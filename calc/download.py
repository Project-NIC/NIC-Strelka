"""The download: by Nordic's ESB with SMP on it, through the first Povodok, the others joined over
Wi-Fi for the reach; a Belka on its own cable, a disk; and the whole set in the dock, every Belka a
disk on one hub, copied at once."""

import math
import textwrap

from memory import bytes_a_second
from params import (BELKAS, CARD_BIT_S, DEFAULT_HZ, DOCK_BYTE_S, ESB_ACK_BYTES, ESB_FRAG_HEAD_BYTES,
                    ESB_HEAD_BYTES, ESB_MIC_BYTES, ESB_PAYLOAD_BYTES, EXAMS_S, PERIOD_S,
                    SMP_FRAME_BYTES, SMP_HEAD_BYTES, STATE_EVERY, TURN_S)
from radio import air_s, syncs_s


def packet_s():
    """One ESB packet of the most payload, its acknowledgement and the two turns."""
    return air_s(ESB_HEAD_BYTES + ESB_PAYLOAD_BYTES) + air_s(ESB_ACK_BYTES) + 2 * TURN_S


def packets_a_period():
    return math.floor((PERIOD_S - syncs_s()) / packet_s())


def frame_bytes_a_fragment():
    """What of an SMP frame one ESB payload carries: less Strelka's fragment head and the MIC."""
    return ESB_PAYLOAD_BYTES - ESB_FRAG_HEAD_BYTES - ESB_MIC_BYTES


def fragments_a_frame():
    return math.ceil(SMP_FRAME_BYTES / frame_bytes_a_fragment())


def pace():
    """The record's bytes a second from the Belka being asked: the periods after the SYNCs, less
    the state's period once a second, less the fragments' heads and MICs and the SMP frames' own
    bytes."""
    payload = packets_a_period() * frame_bytes_a_fragment() / PERIOD_S
    payload *= (STATE_EVERY - 1) / STATE_EVERY
    return payload * SMP_FRAME_BYTES / (SMP_FRAME_BYTES + SMP_HEAD_BYTES)


def fmt(seconds):
    if seconds >= 3600:
        hours = seconds / 3600
        return f"{hours:.0f} h" if hours == int(hours) else f"{hours:.1f} h".replace(".", ",")
    if seconds >= 60:
        return f"{seconds / 60:.0f} min"
    if seconds >= 10:
        return f"{seconds:.0f} s"
    return f"{seconds:.1f} s".replace(".", ",")


def records():
    n = min(BELKAS)
    out = [(n, exam, DEFAULT_HZ, f"{DEFAULT_HZ} Hz") for exam in EXAMS_S]
    out.append((n, max(EXAMS_S), 10 * DEFAULT_HZ, f"{10 * DEFAULT_HZ / 1000:g} kHz"))
    return out


def table():
    head = ("| the set's record | over the radio, ESB | a Belka on its cable "
            "| the set in the dock, at once |")
    rows = [head, "|---|---|---|---|"]
    for n, exam, hz, rate in records():
        one = bytes_a_second(hz) * exam
        total = n * one
        size = f"{total / 1e9:.1f} GB".replace(".", ",") if total >= 1e9 else f"{total / 1e6:.0f} MB"
        card = one / (CARD_BIT_S / 8)
        rows.append(f"| {n} Belkas × {fmt(exam)} at {rate}, {size} | {fmt(total / pace())} | "
                    f"{fmt(card)} | {fmt(max(card, total / DOCK_BYTE_S))} |")
    return "\n".join(rows)


def line_frame():
    text = (f"A fragment carries {frame_bytes_a_fragment()} B of a frame, the ESB payload of "
            f"{ESB_PAYLOAD_BYTES} B less its head of {ESB_FRAG_HEAD_BYTES} B and the MIC of "
            f"{ESB_MIC_BYTES} B; a frame of {SMP_FRAME_BYTES:,} B goes in {fragments_a_frame()} "
            f"fragments, each acknowledged by the radio, and a Belka asked for its record gives "
            f"{pace() / 1e3:.0f} kB/s.").replace(f"{SMP_FRAME_BYTES:,}", f"{SMP_FRAME_BYTES:,}".replace(",", " "))
    return textwrap.fill(text, 100, break_on_hyphens=False)


def line_pace():
    text = (f"An ESB packet of {ESB_PAYLOAD_BYTES} B with its acknowledgement and the turns takes "
            f"{packet_s() * 1e6:,.0f} µs; {packets_a_period()} go in a period after the SYNCs, a period "
            f"in {STATE_EVERY} kept for the Belkas' state, each carrying {frame_bytes_a_fragment()} B of "
            f"a frame ([`COMMANDS.md`](COMMANDS.md)), the SMP frames' own bytes counted: "
            f"{pace() / 1e3:.0f} kB/s from the Belka being asked, a second's worth some "
            f"{pace() / bytes_a_second(DEFAULT_HZ) / 60:.0f} minutes of its record at {DEFAULT_HZ} Hz.")
    return textwrap.fill(text.replace("1,188", "1 188"), 100, break_on_hyphens=False)


if __name__ == "__main__":
    print(line_pace())
    print()
    print(table())
