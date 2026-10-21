"""The current a unit draws from its cell, rough: a low and a high case from the parts' figures,
the radio's share from the period's timetable. The LDOs pass the current on, so the cell gives
what the board draws. The measurement on a built unit decides."""

import textwrap

from params import (BELKAS, BOARD_MA, CARD_MA, CELL_MAH, CPU_DUTY, CPU_MA, IMU_MA, LED_DUTY,
                    LED_MA, LIVE_HZ, LIVE_OFF_EVERY_S, PERIOD_S, QUATERNION_BYTES, RAMP_S, RX_MA,
                    STEP_BYTES, SYNC_GUARD_S, TX_MA, WIFI_BIT_S, WIFI_MA)
from radio import air_s, live_bytes, packet_bytes, pass_window_s, sync_bytes, syncs_s


def belka_radio_ma(case):
    """Low: the first SYNC heard, the live view off, the state alone once a second. High: all
    four SYNCs' slots listened through, raw steps at the highest rate every period."""
    if case == "low":
        rx = RAMP_S + SYNC_GUARD_S + air_s(sync_bytes())
        tx = (RAMP_S + air_s(packet_bytes(0))) * PERIOD_S / LIVE_OFF_EVERY_S
    else:
        rx = RAMP_S + syncs_s()
        tx = RAMP_S + air_s(live_bytes(max(LIVE_HZ), STEP_BYTES))
    return (rx * RX_MA + tx * TX_MA) / PERIOD_S


def povodok_radio_ma(case):
    """The receiver on all the time; its own SYNC sent once a period, and high, a whole PASS
    window sent too."""
    send = RAMP_S + air_s(sync_bytes()) + (pass_window_s() if case == "high" else 0)
    return RX_MA + send * (TX_MA - RX_MA) / PERIOD_S


def wifi_ma(case):
    """The receiver on throughout, power saving not counted; the set's live view sent on, the
    quaternions low, the raw steps high."""
    step = QUATERNION_BYTES if case == "low" else STEP_BYTES
    sent = min(BELKAS) * max(LIVE_HZ) * step * 8 / WIFI_BIT_S
    return WIFI_MA[0] + sent * (WIFI_MA[1] - WIFI_MA[0])


def current_ma(unit, case):
    i = 0 if case == "low" else 1
    common = CPU_MA * CPU_DUTY[i] + BOARD_MA + LED_MA * LED_DUTY[i]
    if unit == "Belka":
        return common + IMU_MA + CARD_MA[i] + belka_radio_ma(case)
    wifi = wifi_ma(case) if unit.endswith("on") else 0
    return common + povodok_radio_ma(case) + wifi


UNITS = ("Belka", "Povodok, Wi-Fi off", "Povodok, Wi-Fi on")


def fmt_ma(ma):
    return (f"{ma:.0f} mA" if ma >= 10 else f"{ma:.1f} mA").replace(".", ",")


def table():
    rows = ["| unit | current, low | current, high | the cell lasts |", "|---|---|---|---|"]
    for unit in UNITS:
        lo, hi = current_ma(unit, "low"), current_ma(unit, "high")
        rows.append(f"| {unit} | {fmt_ma(lo)} | {fmt_ma(hi)} | "
                    f"{CELL_MAH / hi:.0f} h to {CELL_MAH / lo:.0f} h |")
    return "\n".join(rows)


def line_radio():
    lo, hi = belka_radio_ma("low"), belka_radio_ma("high")
    return textwrap.fill(f"The Belka's radio, from the timetable, draws {lo * 1e3:.0f}–"
                         f"{hi * 1e3:.0f} µA on average; the processor, the card and the sensor "
                         f"draw the rest.", 100)


if __name__ == "__main__":
    print(table())
    print(line_radio())
