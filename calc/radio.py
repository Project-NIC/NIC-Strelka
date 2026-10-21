"""The inside link: the Povodoks' SYNCs in four fixed slots, then a fixed slot for each Belka's
number, every slot as long as the longest LIVE."""

import math
import textwrap

from params import (BELKAS, BODY_MAX_BYTES, CHANNELS, LIVE_MSG_EACH_BYTES, LIVE_MSG_HEAD_BYTES,
                    SET_CHANNELS, UDP_MAX_BYTES, CRC_BYTES, LIVE_HZ, MEASURE_S, MISS_BYTES,
                    PASS_EACH_BYTES, PACKET_HEAD_BYTES, PERIOD_S,
                    PHY_BIT_S, POVODOKS, QUATERNION_BYTES, SEARCH_ID_BYTES, SEARCH_NUMBERS,
                    SLOT_GAP_S, STEP_BYTES, SYNC_BODY_BYTES, SYNC_GUARD_S, TURN_S, USB_BIT_S,
                    WIFI_BIT_S)

CONTENTS = (("quaternions", QUATERNION_BYTES), ("raw", STEP_BYTES))


def air_s(packet_bytes):
    return packet_bytes * 8 / PHY_BIT_S


def packet_bytes(body_bytes):
    return PACKET_HEAD_BYTES + body_bytes + CRC_BYTES


def sync_bytes():
    return packet_bytes(SYNC_BODY_BYTES)


def syncs_s():
    """The guard and the four SYNCs' slots, a SYNC and the turn to the next each."""
    return SYNC_GUARD_S + POVODOKS * (air_s(sync_bytes()) + TURN_S)


def steps_a_period(hz):
    return round(hz * PERIOD_S)


def live_bytes(hz, step):
    """A Belka's LIVE: the steps of the last period, its state in the head."""
    return packet_bytes(steps_a_period(hz) * step)


def slot_s():
    """One length for every slot: the longest LIVE, raw at the highest rate, and the gap."""
    return air_s(live_bytes(max(LIVE_HZ), STEP_BYTES)) + SLOT_GAP_S


def rows():
    out = [("the state alone", packet_bytes(0))]
    for name, step in CONTENTS:
        for hz in LIVE_HZ:
            out.append((f"{name} {hz} Hz", live_bytes(hz, step)))
    return out


def ms(seconds):
    return f"{seconds * 1e3:.1f} ms".replace(".", ",")


def miss_s():
    """The first Povodok's MISS after the last slot, and the turn after it."""
    return air_s(packet_bytes(MISS_BYTES)) + TURN_S


def pass_window_s():
    """What is left between the MISS and the period's measurement, shared by the other Povodoks."""
    slots = max(BELKAS) * slot_s()
    return (PERIOD_S - syncs_s() - slots - miss_s() - MEASURE_S) / (POVODOKS - 1)


def passed_a_window(each):
    """The missed Belkas one Povodok passes in its window, each `each` bytes, in packets of the
    body's most, a turn before each packet."""
    left, n = pass_window_s(), 0
    while True:
        k = min(BODY_MAX_BYTES // each, math.floor(
            ((left - TURN_S) * PHY_BIT_S / 8 - PACKET_HEAD_BYTES - CRC_BYTES) / each))
        if k <= 0:
            return n
        left -= TURN_S + air_s(packet_bytes(k * each))
        n += k


def table_packets():
    longest = max(LIVE_HZ)
    each = PASS_EACH_BYTES + steps_a_period(longest) * STEP_BYTES
    rows = (
        ("SYNC", "the set's", f"a signature of {SYNC_BODY_BYTES} B", f"{sync_bytes()} B"),
        ("LIVE", "the Belka's",
         f"the steps of the last whole period, 0–{steps_a_period(longest) * STEP_BYTES} B",
         f"{packet_bytes(0)}–{live_bytes(longest, STEP_BYTES)} B"),
        ("MISS", "the set's", f"a bit for each Belka missed, {MISS_BYTES} B",
         f"{packet_bytes(MISS_BYTES)} B"),
        ("PASS", "the set's", f"each missed Belka's number, S0, state and steps, up to {each} B "
         f"a Belka, {BODY_MAX_BYTES} B at most", f"up to {packet_bytes(BODY_MAX_BYTES)} B"),
        ("CALL", "a SYNC's, a new Belka not reading it", "a SYNC's", f"{sync_bytes()} B"),
        ("HERE", "0", f"the chip's ID, {SEARCH_ID_BYTES} B", f"{packet_bytes(SEARCH_ID_BYTES)} B"),
    )
    lines = ["| the packet | its state | its body | all |", "|---|---|---|---|"]
    lines += [f"| {name} | {state} | {body} | {size} |" for name, state, body, size in rows]
    return "\n".join(lines)


def table_live():
    lines = ["| the live view a Belka | its packet | on the air |", "|---|---|---|"]
    for name, nbytes in rows():
        lines.append(f"| {name} | {nbytes} B | {air_s(nbytes) * 1e6:.0f} µs |")
    return "\n".join(lines)


def line_slots():
    n = max(BELKAS)
    raw = live_bytes(max(LIVE_HZ), STEP_BYTES)
    text = (f"Every slot is {slot_s() * 1e6:.0f} µs: the raw steps at {max(LIVE_HZ)} Hz, {raw} B "
            f"and {air_s(raw) * 1e6:.0f} µs on the air, and the gap of "
            f"{SLOT_GAP_S * 1e6:.0f} µs; a shorter packet goes in the same slot. The SYNCs and "
            f"{n} slots take {ms(syncs_s() + n * slot_s())} of the {PERIOD_S * 1e3:.0f} ms period, "
            f"whatever the live view; "
            f"{math.floor((PERIOD_S - syncs_s()) / slot_s())} slots would fit.")
    return textwrap.fill(text, 100, break_on_hyphens=False)


def line_syncs():
    text = (f"A SYNC is {sync_bytes()} B, its signature of {SYNC_BODY_BYTES} B among them, "
            f"{air_s(sync_bytes()) * 1e6:.0f} µs on the air; with "
            f"{TURN_S * 1e6:.0f} µs to turn from receiving to sending, Povodok k sends at "
            f"{SYNC_GUARD_S * 1e6:.0f} + k × {(air_s(sync_bytes()) + TURN_S) * 1e6:.0f} µs, and the "
            f"guard and the slots of {POVODOKS} Povodoks take {syncs_s() * 1e6:.0f} µs of the "
            f"{PERIOD_S * 1e3:.0f} ms period.")
    return textwrap.fill(text, 100)


def line_pass():
    raw = PASS_EACH_BYTES + steps_a_period(max(LIVE_HZ)) * STEP_BYTES
    quat = PASS_EACH_BYTES + steps_a_period(max(LIVE_HZ)) * QUATERNION_BYTES
    n, small = min(BELKAS), passed_a_window(raw)
    three, two = (POVODOKS - 1) * small, (POVODOKS - 2) * small
    if two >= n:
        tail = f"every one of {n} Belkas even when the first hears none."
    else:
        tail = (f"every one of {n} Belkas with three, {two} of them with two, when the first hears "
                f"none.")
    text = (f"After the {max(BELKAS)} slots, the MISS takes {miss_s() * 1e6:.0f} µs; the "
            f"{POVODOKS - 1} other Povodoks then have "
            f"{pass_window_s() * 1e3:.2f} ms each".replace(".", ",") +
            f" and the measurement the period's last {MEASURE_S * 1e3:.1f} ms".replace(".", ",") +
            f". A Povodok passes {small} missed Belkas in its window at raw {max(LIVE_HZ)} Hz, "
            f"{raw} B each, {passed_a_window(quat)} as quaternions: three Povodoks besides the "
            f"first pass {three} a period, two {two} — " + tail)
    return textwrap.fill(text, 100, break_on_hyphens=False)


def line_message():
    n, hz = min(BELKAS), max(LIVE_HZ)
    each = LIVE_MSG_EACH_BYTES + steps_a_period(hz) * STEP_BYTES
    size = LIVE_MSG_HEAD_BYTES + n * each
    fit = (UDP_MAX_BYTES - LIVE_MSG_HEAD_BYTES) // each
    rate = size / PERIOD_S
    text = (f"With {n} Belkas raw at {hz} Hz, {each} B each, a message is {size} B, "
            f"{rate / 1e3:.0f} kB/s, {rate * 8 / 1e6:.2f} Mbit/s; up to {fit} Belkas go in one "
            f"message of {UDP_MAX_BYTES} B at most, more in two of the same head, the phone "
            f"joining them by the period.").replace(f"{rate * 8 / 1e6:.2f}",
                                                    f"{rate * 8 / 1e6:.2f}".replace(".", ","))
    return textwrap.fill(text.replace(f"{UDP_MAX_BYTES} B", f"{UDP_MAX_BYTES:,} B".replace(",", " ")),
                         100, break_on_hyphens=False)


def line_channels():
    n = min(BELKAS)
    periods = math.ceil(CHANNELS / n)
    scan = f"{2 * (SET_CHANNELS - 1) * PERIOD_S:.1f}".replace(".", ",")
    text = (f"{n} Belkas, each measuring a channel a period, go through the {CHANNELS} in "
            f"{periods} periods, {periods * PERIOD_S * 1e3:.0f} ms; a Belka that lost the set "
            f"listens a period on each of the set's other {SET_CHANNELS - 1}, the set's own "
            f"between each, all in {scan}\u00a0s.")
    return textwrap.fill(text, 100).replace("\u00a0", " ")


def line_search():
    window = air_s(packet_bytes(SEARCH_ID_BYTES)) + SLOT_GAP_S
    windows = math.floor((PERIOD_S - syncs_s()) / window)
    periods = math.ceil(SEARCH_NUMBERS / windows)
    n = min(BELKAS)
    clash = 1.0
    for k in range(n):
        clash *= 1 - k / SEARCH_NUMBERS
    text = (f"A HERE is {packet_bytes(SEARCH_ID_BYTES)} B, its chip's ID whole; "
            f"with the gap a window of {window * 1e6:.0f} µs, {windows} after the CALL in a "
            f"period, so that its number, one of {SEARCH_NUMBERS:,}, falls in one of {periods} "
            f"periods: every new Belka is heard once in {periods * PERIOD_S:.1f} s. Two of {n} draw "
            f"the same number {100 * (1 - clash):.1f} % of the time, and draw again.")
    text = text.replace(f"{SEARCH_NUMBERS:,}", f"{SEARCH_NUMBERS:,}".replace(",", " "))
    text = text.replace(f"{periods * PERIOD_S:.1f} s", f"{periods * PERIOD_S:.1f} s".replace(".", ","))
    text = text.replace(f"{100 * (1 - clash):.1f} %", f"{100 * (1 - clash):.1f} %".replace(".", ","))
    return textwrap.fill(text, 100, break_on_hyphens=False)


def line_out():
    """What the whole set's live view asks of the link to the phone or the computer."""
    n = min(BELKAS)
    hz = max(LIVE_HZ)
    raw = n * hz * STEP_BYTES
    quat = n * hz * QUATERNION_BYTES
    text = (f"Out of Povodok, {n} Belkas at {hz} Hz send {quat / 1e3:.0f} kB/s as quaternions and "
            f"{raw / 1e3:.0f} kB/s raw, {raw * 8 / 1e6:.2f} Mbit/s — under a hundredth of the "
            f"Wi-Fi's {WIFI_BIT_S / 1e6:.0f} Mbit/s on the air and of USB's "
            f"{USB_BIT_S / 1e6:.0f} Mbit/s.").replace(f"{raw * 8 / 1e6:.2f}", f"{raw * 8 / 1e6:.2f}".replace(".", ","))
    return textwrap.fill(text, 100)


if __name__ == "__main__":
    print(line_syncs())
    print()
    print(table_live())
    print()
    print(line_slots())
    print(line_out())
