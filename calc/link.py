"""The reach, rough: the loss in free space against the module's budget, and what is left when
a body stands in the way."""

import math
import textwrap

from params import (ANTENNA_DB, BODY_DB, BUDGET_DB, EIRP_FREE_DBM, ESB_HEAD_BYTES,
                    ESB_PAYLOAD_BYTES, FREQ_HZ, LIVE_HZ, MEASURE_S, MU_MAX, PERIOD_S, POVODOKS,
                    REACH_M, RX_SENS_DBM, STEP_BYTES, SYNC_GUARD_S, TURN_S, TX_DBM, TX_FULL_DBM,
                    TX_GAP_MIN_S, TX_SEQ_MAX_S)
from radio import air_s, live_bytes, pass_window_s, sync_bytes
from download import packets_a_second

C = 299_792_458


def free_space_db(metres):
    return 20 * math.log10(4 * math.pi * metres * FREQ_HZ / C)


def table():
    rows = ["| the link | free space | left of the budget | left behind a body |",
            "|---|---|---|---|"]
    for name, metres in REACH_M:
        left = BUDGET_DB - free_space_db(metres)
        behind = f"{left - BODY_DB:.0f}".replace("-", "−")
        rows.append(f"| {name}, {metres} m | {free_space_db(metres):.0f} dB | {left:.0f} dB | "
                    f"{behind} dB |")
    return "\n".join(rows)


def dec(value, fmt):
    return format(value, fmt).replace(".", ",").replace("-", "−")


def line_power():
    eirp = TX_DBM + ANTENNA_DB
    text = (f"By default the radio sends {dec(TX_DBM, '+d')} dBm, with the BM20F's antenna some "
            f"{dec(eirp, '+.1f')} dBm e.i.r.p., under the {EIRP_FREE_DBM} dBm above which EN 300 "
            f"328 asks adaptivity or limits on a radio that does not adapt; the budget {dec(BUDGET_DB, '.1f')} dB against {dec(RX_SENS_DBM, '.1f')} dBm "
            f"received. The full {dec(TX_FULL_DBM, '+d')} dBm, "
            f"{dec(TX_FULL_DBM + ANTENNA_DB, '+.1f')} dBm e.i.r.p., adds "
            f"{TX_FULL_DBM - TX_DBM} dB. Kryska's BM20M radiates some 6 dB less than the BM20F "
            f"from the same power (+6,65 dBm against +12,61 from +8,37, the BM20C specification "
            f"v1.00, the model summaries), its e.i.r.p. the further under, its reach the shorter; "
            f"the WM02C20C's own is not yet published.")
    return textwrap.fill(text, 100, break_on_hyphens=False)


def line_mu():
    full_mw = 10 ** ((TX_FULL_DBM + ANTENNA_DB) / 10)
    belka = air_s(live_bytes(max(LIVE_HZ), STEP_BYTES))
    povodok = air_s(sync_bytes()) + pass_window_s()
    esb = packets_a_second() * PERIOD_S * air_s(ESB_HEAD_BYTES + ESB_PAYLOAD_BYTES)
    # a Povodok's PASS window and its next SYNC are one Tx-sequence where the silence between
    # them is under 3,5 ms, as the norm's test counts it (EN 300 328 V2.2.2, 5.4.2.2.1.3): the
    # longest over the Povodoks that pass, the windows in the numbers' order before the
    # measurement at the period's end
    sequence, longest = 0.0, 0
    for j in range(1, POVODOKS):
        start = PERIOD_S - MEASURE_S - (POVODOKS - j) * pass_window_s()
        sync_start = PERIOD_S + SYNC_GUARD_S + j * (air_s(sync_bytes()) + TURN_S)
        gap = sync_start - (start + pass_window_s())
        one = sync_start + air_s(sync_bytes()) - start if gap < TX_GAP_MIN_S else pass_window_s()
        if one > sequence:
            sequence, longest = one, j
    ordinal = {1: "second", 2: "third", 3: "fourth"}[longest]
    mu = lambda on: full_mw / 100 * on / PERIOD_S * 100
    dc = lambda on: on / PERIOD_S * 100
    text = (f"At the full power, {full_mw:.0f} mW e.i.r.p., a Belka measuring sends "
            f"{belka * 1e6:.0f} µs a period, a duty cycle of {dec(dc(belka), '.2f')} % and a medium "
            f"utilisation of {dec(mu(belka), '.2f')} %, and a Povodok {dec(povodok * 1e3, '.2f')} ms "
            f"at most, {dec(dc(povodok), '.1f')} % and {dec(mu(povodok), '.1f')} % — under the "
            f"{MU_MAX * 100:.0f} % the norm allows equipment that does not adapt. The norm's test "
            f"counts a silence under {dec(TX_GAP_MIN_S * 1e3, '.1f')} ms into the sequence, so a "
            f"Povodok's PASS window and its next SYNC may be one Tx-sequence: the longest the "
            f"{ordinal} Povodok's, {dec(sequence * 1e3, '.1f')} ms with what lies between them, "
            f"under the {TX_SEQ_MAX_S * 1e3:.0f} ms allowed, with "
            f"the rest of the period silent after it; the duty cycles are what the maker declares. "
            f"A Belka giving its record by ESB sends {esb / PERIOD_S * 100:.0f} % of the time, "
            f"{dec(mu(esb), '.0f')} %, so a download goes at the default power alone.")
    return textwrap.fill(text, 100, break_on_hyphens=False)


if __name__ == "__main__":
    print(table())
