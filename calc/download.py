"""The download: by Nordic's ESB with SMP on it, through the first Povodok, the others joined over
Wi-Fi for the reach; a Belka on its own cable, a disk; and the whole set in the dock, every Belka a
disk on one hub, copied at once."""

import math
import textwrap

from memory import bytes_a_second
from params import (BELKAS, CARD_BIT_S, DEFAULT_HZ, DOCK_BYTE_S, ESB_ACK_BYTES,
                    ESB_FRAG_HEAD_BYTES, ESB_HEAD_BYTES, ESB_MIC_BYTES, ESB_PAYLOAD_BYTES, EXAMS_S,
                    RETRANSMIT_STEP_S, SMP_FRAME_BYTES, SMP_HEAD_BYTES, TURN_S)
from radio import air_s


def packet_s():
    """One ESB packet of the most payload, its acknowledgement and the two turns."""
    return air_s(ESB_HEAD_BYTES + ESB_PAYLOAD_BYTES) + air_s(ESB_ACK_BYTES) + 2 * TURN_S


def packets_a_second():
    """The download's packets: the SYNCs paused, ESB has the radio whole, one after another."""
    return 1 / packet_s()


def retransmit_delay_s():
    """ESB's retransmit_delay: above one packet with its acknowledgement, in whole steps."""
    return math.floor(packet_s() / RETRANSMIT_STEP_S + 1) * RETRANSMIT_STEP_S


def frame_bytes_a_fragment():
    """What of an SMP frame one ESB payload carries: less Strelka's fragment head and the MIC."""
    return ESB_PAYLOAD_BYTES - ESB_FRAG_HEAD_BYTES - ESB_MIC_BYTES


def fragments_a_frame():
    return math.ceil(SMP_FRAME_BYTES / frame_bytes_a_fragment())


def pace():
    """The record's bytes a second from the Belka being asked: ESB's packets, less the fragments'
    heads and MICs and the SMP frames' own bytes."""
    payload = packets_a_second() * frame_bytes_a_fragment()
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
            f"{packet_s() * 1e6:,.0f} µs; with the SYNCs paused {packets_a_second():.0f} go in a "
            f"second, each carrying {frame_bytes_a_fragment()} B of "
            f"a frame ([`COMMANDS.md`](COMMANDS.md)), the SMP frames' own bytes counted: "
            f"{pace() / 1e3:.0f} kB/s from the Belka being asked, a second's worth some "
            f"{pace() / bytes_a_second(DEFAULT_HZ) / 60:.0f} minutes of its record at {DEFAULT_HZ} Hz; "
            f"through two to four Povodoks joined to the phone, each on a channel of its own, as "
            f"many times that.")
    return textwrap.fill(text.replace("1,188", "1 188"), 100, break_on_hyphens=False)


def us(seconds):
    """Microseconds, the space of thousands kept whole through the wrapping."""
    return f"{seconds * 1e6:,.0f}".replace(",", "\0")


def line_turns():
    text = (f"ESB's `retransmit_delay` is {us(retransmit_delay_s())} µs, above one packet of "
            f"{us(packet_s())} µs with its acknowledgement.")
    return textwrap.fill(text, 100, break_on_hyphens=False).replace("\0", " ")


if __name__ == "__main__":
    print(line_pace())
    print()
    print(table())
