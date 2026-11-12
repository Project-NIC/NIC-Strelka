"""The current a unit draws from its cell, rough: a low and a high case from the parts' figures,
the radio's share from the period's timetable, the memory's from the bytes the record writes. The
LDOs pass the current on, so the cell gives what the board draws; Kryska's buck is taken as one,
which errs high by what it gains. The measurement on a built unit decides."""

import textwrap

from memory import bytes_a_second, kryska_program_share
from params import (AWAKE_S, BELKAS, BOARD_MA, CARD_BIT_S, CARD_BUSY_FACTOR, CARD_IDLE_MA,
                    CARD_WRITE_MA, CELL_MAH, FLASH_IDLE_MA, FLASH_PROG_MA, KRYSKA_PER_MAH,
                    KRYSKA_TOP_HZ, KRYSKA_USUAL_HZ, PER_MAH, RAW_HZ, USUAL_HZ, CPU_DUTY, CPU_IDLE_MA, CPU_MA,
                    HFXO_MA, IMU_MA, IMU_PD_MA, LED_ASLEEP_DUTY, LED_DUTY, LED_MA, LIVE_HZ, LIVE_OFF_EVERY_S, PERIOD_S, QUATERNION_BYTES, RAMP_S,
                    RX_MA, SDNAND_IDLE_MA, SDNAND_MA, STEP_BYTES, SYNC_GUARD_S, TX_MA, WIFI_BIT_S,
                    WIFI_MA)
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


def card_ma(hz, case):
    """The card: idle, and writing while the bus carries the record's bytes, low, or twice as
    long, high, the card programming as long as the bus took."""
    i = 0 if case == "low" else 1
    duty = min(1.0, bytes_a_second(hz) / (CARD_BIT_S / 8) * CARD_BUSY_FACTOR[i])
    return CARD_IDLE_MA[i] + duty * (CARD_WRITE_MA[i] - CARD_IDLE_MA[i])


def flash_ma(hz, case, both=False):
    """Kryska's other way, NOR flash: in standby, and programming its pages for their share of the
    time, at the sheet's typical page and current, low, its longest and most, high."""
    i = 0 if case == "low" else 1
    share = min(1.0, kryska_program_share(hz, longest=i == 1, both=both))
    return FLASH_IDLE_MA[i] + share * (FLASH_PROG_MA[i] - FLASH_IDLE_MA[i])


def sdnand_ma(hz, case, both=False):
    """Kryska's SD NAND: standing while a record runs, U6 switching it off between, and at work
    while the bus carries the record's bytes, low, or twice as long, high, as the Belka's card."""
    i = 0 if case == "low" else 1
    duty = min(1.0, bytes_a_second(hz, both) / (CARD_BIT_S / 8) * CARD_BUSY_FACTOR[i])
    return SDNAND_IDLE_MA[i] + duty * (SDNAND_MA[i] - SDNAND_IDLE_MA[i])


def current_ma(unit, case, hz=0):
    i = 0 if case == "low" else 1
    common = CPU_MA * CPU_DUTY[i] + HFXO_MA[i] + BOARD_MA + LED_MA * LED_DUTY[i]
    if unit == "Belka":
        return common + IMU_MA + card_ma(hz, case) + belka_radio_ma(case)
    if unit == "Kryska":
        return common + IMU_MA + sdnand_ma(hz, case) + belka_radio_ma(case)
    wifi = wifi_ma(case) if unit.endswith("on") else 0
    return common + povodok_radio_ma(case) + wifi


def hz_name(hz):
    return f"{hz // 1000} kHz" if hz >= 1000 else f"{hz} Hz"


# the rows: a name, the unit, its rate into memory, its cell's mAh and whether that is a
# reckoning per so many mAh, the cell not chosen
ROWS = (
    (f"Belka, {hz_name(USUAL_HZ)} into its card", "Belka", USUAL_HZ, CELL_MAH, False),
    ("Belka, 1 kHz into its card", "Belka", 1000, CELL_MAH, False),
    (f"Zvjozdočka, {hz_name(RAW_HZ)} raw", "Belka", RAW_HZ, CELL_MAH, False),
    (f"Kryska, {hz_name(KRYSKA_USUAL_HZ)} into its SD NAND", "Kryska", KRYSKA_USUAL_HZ,
     KRYSKA_PER_MAH, True),
    (f"Kryska, {hz_name(KRYSKA_TOP_HZ)}", "Kryska", KRYSKA_TOP_HZ, KRYSKA_PER_MAH, True),
    ("Povodok, Wi-Fi off", "Povodok, Wi-Fi off", 0, PER_MAH, True),
    ("Povodok, Wi-Fi on", "Povodok, Wi-Fi on", 0, PER_MAH, True),
)


def fmt_ma(ma):
    return (f"{ma:.0f} mA" if ma >= 10 else f"{ma:.1f} mA").replace(".", ",")


def table():
    rows = ["| unit | current, low | current, high | the cell lasts |", "|---|---|---|---|"]
    for name, unit, hz, mah, per in ROWS:
        lo, hi = current_ma(unit, "low", hz), current_ma(unit, "high", hz)
        short, long_ = f"{mah / hi:.0f} h", f"{mah / lo:.0f} h"
        lasts = short if short == long_ else f"{short} to {long_}"
        if per:
            lasts += f" for each {mah:,} mAh".replace(",", " ")
        rows.append(f"| {name} | {fmt_ma(lo)} | {fmt_ma(hi)} | {lasts} |")
    return "\n".join(rows)


def kryska_memories():
    """Kryska's two ways side by side (chapters/kryska-memory.md): its memory's own current while
    a record runs, the NOR flash's and the SD NAND's, low to high, and the hours each 100 mAh lasts
    with the rest of the unit — at the usual rate, the highest, and with both of the hoof's
    channels."""
    rows = ["| a record | NOR flash, its own | SD NAND, its own | the unit, NOR | the unit, SD NAND |",
            "|---|---|---|---|---|"]
    for hz, both in ((KRYSKA_USUAL_HZ, False), (KRYSKA_TOP_HZ, False), (KRYSKA_USUAL_HZ, True),
                     (2000, True)):
        rest = {c: current_ma("Kryska", c, hz) - sdnand_ma(hz, c) for c in ("low", "high")}
        cells = []
        for mem in (flash_ma, sdnand_ma):
            cells.append(f"{fmt_ma(mem(hz, 'low', both))} to {fmt_ma(mem(hz, 'high', both))}")
        for mem in (flash_ma, sdnand_ma):
            lo, hi = (rest[c] + mem(hz, c, both) for c in ("low", "high"))
            cells.append(f"{KRYSKA_PER_MAH / hi:.0f}–{KRYSKA_PER_MAH / lo:.0f} h")
        name = hz_name(hz) + (", both channels" if both else "")
        rows.append(f"| {name} | " + " | ".join(cells) + " |")
    return "\n".join(rows)


def belka_asleep_ma(case):
    """Asleep (FIRMWARE.md, The power): the live view off, nothing recorded — the radio awake
    for the state's period once a second, a SYNC heard and the state sent; the sensor powered
    down, the card's supply off, the processor idle between; low dark, high a short blink."""
    i = 0 if case == "low" else 1
    radio = ((RAMP_S + SYNC_GUARD_S + air_s(sync_bytes())) * RX_MA +
             (RAMP_S + air_s(packet_bytes(0))) * TX_MA) / LIVE_OFF_EVERY_S
    awake = AWAKE_S[i] * (CPU_MA + HFXO_MA[i]) / LIVE_OFF_EVERY_S
    return CPU_IDLE_MA + BOARD_MA + IMU_PD_MA + radio + awake + LED_MA * LED_ASLEEP_DUTY[i]


def line_asleep():
    lo, hi = belka_asleep_ma("low"), belka_asleep_ma("high")
    return textwrap.fill(f"Asleep, a Belka draws {lo * 1e3:.0f}–{hi * 1e3:.0f} µA: the cell's "
                         f"{CELL_MAH} mAh would last {CELL_MAH / hi / 24 / 30:.0f} months and more, "
                         f"its own self-discharge the limit; the board's LDOs, PMIC and Hall "
                         f"switches {BOARD_MA * 1e3:.0f} µA of it.", 100,
                         break_on_hyphens=False)


def line_radio():
    lo, hi = belka_radio_ma("low"), belka_radio_ma("high")
    return textwrap.fill(f"The Belka's radio, from the timetable, draws {lo * 1e3:.0f}–"
                         f"{hi * 1e3:.0f} µA on average; the processor, the card and the sensor "
                         f"draw the rest.", 100)


if __name__ == "__main__":
    print(table())
    print(line_radio())
