"""The reach, rough: the loss in free space against the module's budget, and what is left when
a body stands in the way."""

import math
import textwrap

from params import (ANTENNA_DB, BODY_DB, BUDGET_DB, EIRP_FREE_DBM, ESB_HEAD_BYTES,
                    ESB_PAYLOAD_BYTES, FREQ_HZ, LIVE_HZ, MEASURE_S, MU_MAX, PERIOD_S, POVODOKS,
                    REACH_M, RX_SENS_DBM, STEP_BYTES, SYNC_GUARD_S, TURN_S, TX_DBM, TX_FULL_DBM,
                    TX_GAP_MIN_S, TX_SEQ_MAX_S)
from radio import air_s, live_bytes, pass_window_s, sync_bytes
from download import packets_a_period

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
    text = (f"By default the radio sends {dec(TX_DBM, '+d')} dBm, with the antenna some "
            f"{dec(eirp, '+.1f')} dBm e.i.r.p., under the {EIRP_FREE_DBM} dBm above which EN 300 "
            f"328 asks adaptivity or limits on a radio that does not adapt; the budget {BUDGET_DB} dB against {dec(RX_SENS_DBM, 'd')} dBm "
            f"received. The full {dec(TX_FULL_DBM, '+d')} dBm, "
            f"{dec(TX_FULL_DBM + ANTENNA_DB, '+.1f')} dBm e.i.r.p., adds "
            f"{TX_FULL_DBM - TX_DBM} dB.")
    return textwrap.fill(text, 100, break_on_hyphens=False)


def line_mu():
    full_mw = 10 ** ((TX_FULL_DBM + ANTENNA_DB) / 10)
    belka = air_s(live_bytes(max(LIVE_HZ), STEP_BYTES))
    povodok = air_s(sync_bytes()) + pass_window_s()
    esb = packets_a_period() * air_s(ESB_HEAD_BYTES + ESB_PAYLOAD_BYTES)
    # the last Povodok's PASS window and its next SYNC, one Tx-sequence as the norm's test counts
    # a silence under 3,5 ms into it (EN 300 328 V2.2.2, 5.4.2.2.1.3)
    window_start = PERIOD_S - MEASURE_S - pass_window_s()
    sync_end = PERIOD_S + SYNC_GUARD_S + (POVODOKS - 1) * (air_s(sync_bytes()) + TURN_S) \
        + air_s(sync_bytes())
    sequence = sync_end - window_start
    mu = lambda on: full_mw / 100 * on / PERIOD_S * 100
    dc = lambda on: on / PERIOD_S * 100
    text = (f"At the full power, {full_mw:.0f} mW e.i.r.p., a Belka measuring sends "
            f"{belka * 1e6:.0f} µs a period, a duty cycle of {dec(dc(belka), '.2f')} % and a medium "
            f"utilisation of {dec(mu(belka), '.2f')} %, and a Povodok {dec(povodok * 1e3, '.2f')} ms "
            f"at most, {dec(dc(povodok), '.1f')} % and {dec(mu(povodok), '.1f')} % — under the "
            f"{MU_MAX * 100:.0f} % the norm allows equipment that does not adapt. The norm's test "
            f"counts a silence under {dec(TX_GAP_MIN_S * 1e3, '.1f')} ms into the sequence, so the "
            f"last Povodok's PASS window and its next SYNC are one Tx-sequence of "
            f"{dec(sequence * 1e3, '.1f')} ms, under the {TX_SEQ_MAX_S * 1e3:.0f} ms allowed, with "
            f"the rest of the period silent after it; the duty cycles are what the maker declares. "
            f"A Belka giving its record by ESB sends {esb / PERIOD_S * 100:.0f} % of the time, "
            f"{dec(mu(esb), '.0f')} %, so a download goes at the default power alone.")
    return textwrap.fill(text, 100, break_on_hyphens=False)


if __name__ == "__main__":
    print(table())
