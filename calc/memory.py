"""The memory: how long the user's card holds the record, what its bus carries, and the buffer in
RAM its pauses ask."""

import textwrap

from params import (BLOCK_BYTES, CARD_BIT_S, CARD_BUFFER_BYTES, CARD_BUSY_S, CARD_BYTES, DATA_HEAD,
                    FILE_PART_BYTES, INFO_A_STEP_BYTES, INFO_EVERY_STEPS, KRYSKA_FLASH_BYTES,
                    KRYSKA_PAGE_BYTES, KRYSKA_PAGE_S, KRYSKA_SESSIONS_MAX, KRYSKA_TOP_HZ,
                    KRYSKA_USUAL_HZ, RAM_BYTES, RATES_HZ, STEP_BYTES, SYNC_FILE_S)

HEAD_BYTES = sum(size for _, size in DATA_HEAD)
KRYSKA_BOTH_MOST_HZ = 2000     # both of the hoof's channels on Kryska (firmware: card_both_most_hz)
RAW_FROM_HZ = 4000


def steps_a_block():
    return (BLOCK_BYTES - HEAD_BYTES) // STEP_BYTES


def bytes_a_step():
    """The record's bytes a step: its data block's share, and its information block's."""
    return BLOCK_BYTES / steps_a_block() + BLOCK_BYTES / INFO_EVERY_STEPS


def bytes_a_second(hz, both=False):
    """The record's bytes a second; with both of the hoof's accelerometer channels kept, two
    high-g blocks more for every 120 steps (RECORD.md, A high-g block)."""
    return hz * (bytes_a_step() + (2 * BLOCK_BYTES / INFO_EVERY_STEPS if both else 0))


def fmt(seconds):
    if seconds >= 10 * 86400:
        return f"{seconds / 86400:.0f} days"
    if seconds >= 2 * 86400:
        return f"{seconds / 86400:.1f} days".replace(".", ",")
    if seconds >= 3600:
        return f"{seconds / 3600:.1f} h".replace(".", ",")
    if seconds >= 600:
        return f"{seconds / 60:.0f} min"
    return f"{seconds:.0f} s"


def rate(hz):
    text = f"{hz:,} Hz".replace(",", " ") if hz < 1000 else f"{hz / 1000:g} kHz"
    return text + (", raw" if hz >= RAW_FROM_HZ else "")


def table_hold():
    rows = [f"| into memory | {CARD_BYTES / 1e9:.0f} GB holds |", "|---|---|"]
    for hz in RATES_HZ:
        rows.append(f"| {rate(hz)} | {fmt(CARD_BYTES / bytes_a_second(hz))} |")
    return "\n".join(rows)


def line_rate():
    top = bytes_a_second(max(RATES_HZ))
    bus = CARD_BIT_S / 8
    kind, busy = CARD_BUSY_S[-1]
    covered_hz = CARD_BUFFER_BYTES / busy / bytes_a_step()
    held = CARD_BUFFER_BYTES / top
    text = (f"The card's bus, {CARD_BIT_S / 1e6:.0f} MHz, carries some {bus / 1e6:.1f} MB/s against "
            f"the record's {top / 1e3:.0f} kB/s at the sensor's {max(RATES_HZ) / 1000:g} kHz raw. "
            f"The card pauses at the end of a write, busy at most "
            + " and ".join(f"{b * 1e3:.0f} ms for {k}" for k, b in CARD_BUSY_S)
            + f"; the record waits meanwhile in a buffer of {CARD_BUFFER_BYTES // 1024} KB of the "
            f"processor's {RAM_BYTES / 1e3:.0f} KB, which bears {kind}'s longest pause up to some "
            f"{covered_hz / 1000:.0f} kHz and {held * 1e3:.0f} ms at {max(RATES_HZ) / 1000:g} kHz; "
            f"a longer pause drops a block, its gap seen in the blocks' numbers.")
    return textwrap.fill(text.replace(f"{bus / 1e6:.1f}", f"{bus / 1e6:.1f}".replace(".", ",")),
                         100, break_on_hyphens=False)


def line_block():
    n = steps_a_block()
    free = dict(DATA_HEAD).get("free", 0)
    share = 100 * (1 - STEP_BYTES / bytes_a_step())
    crc = dict(DATA_HEAD)["a CRC-32"]
    text = (f"A data block is {BLOCK_BYTES} B, a sector of the card: a head of {HEAD_BYTES - crc} B, "
            f"{free} B of it free, then {n} steps of {STEP_BYTES} B and a CRC-32; after every "
            f"{INFO_EVERY_STEPS} steps, {INFO_EVERY_STEPS // n} data blocks at any rate, an "
            f"information block of {BLOCK_BYTES} B, {INFO_A_STEP_BYTES} B for each step "
            f"([`RECORD.md`](RECORD.md)). The heads and the information blocks take "
            f"{share:.0f} % of the record at every rate; the tables here count them.")
    return textwrap.fill(text, 100, break_on_hyphens=False)


def line_files():
    top = max(RATES_HZ)
    held = FILE_PART_BYTES / bytes_a_second(top)
    text = (f"Each session is a file on the card's own file system: FAT32 on a card of up to 32 GB "
            f"and exFAT above, as the SD standard has them formatted, both read by FatFs in Zephyr; "
            f"named by the session's number and the Belka's. A file stops under 2 GiB and the record goes on in "
            f"its next part ([`RECORD.md`](RECORD.md), *The file*): a part holds {fmt(held)} of "
            f"the raw {top / 1000:g} kHz, and any examination at the lower rates; "
            f"its room is taken in one run at the start and its length saved every "
            f"{SYNC_FILE_S} s, so that a power cut loses {SYNC_FILE_S} s at most. The card is "
            f"formatted only at the application's word.")
    return textwrap.fill(text, 100, break_on_hyphens=False)


def kryska_program_share(hz, longest=False, both=False):
    """The share of the time Kryska's flash spends programming its pages at a rate."""
    pages = bytes_a_second(hz, both) / KRYSKA_PAGE_BYTES
    return pages * KRYSKA_PAGE_S[1 if longest else 0]


def line_kryska():
    """What Kryska's SD NAND holds in ordinary use and at its most, with both of the hoof's channels
    a third less (RECORD.md, A high-g block)."""
    usual, top = KRYSKA_USUAL_HZ, KRYSKA_TOP_HZ
    text = (f"Its memory holds {KRYSKA_FLASH_BYTES / 2**20:.0f} MB: "
            f"{fmt(KRYSKA_FLASH_BYTES / bytes_a_second(usual))} of the record at {usual} Hz, "
            f"{fmt(KRYSKA_FLASH_BYTES / bytes_a_second(top))} at {top / 1000:g} kHz, the record's "
            f"bytes and its file the Belka's ([`RECORD.md`](RECORD.md)); with both of the hoof's "
            f"accelerometer channels kept a third less, "
            f"{fmt(KRYSKA_FLASH_BYTES / bytes_a_second(usual, True))} and "
            f"{fmt(KRYSKA_FLASH_BYTES / bytes_a_second(top, True))}.")
    return textwrap.fill(text, 100, break_on_hyphens=False)


def line_kryska_nor():
    """Kryska's other way, NOR flash (chapters/kryska-memory.md): what it holds in ordinary use and
    at its most, 4 kHz as that way stood, and how hard the flash works at the most."""
    usual, top = KRYSKA_USUAL_HZ, 4000
    text = (f"Its memory holds {KRYSKA_FLASH_BYTES / 2**20:.0f} MB: "
            f"{fmt(KRYSKA_FLASH_BYTES / bytes_a_second(usual))} of the record at {usual} Hz, "
            f"{fmt(KRYSKA_FLASH_BYTES / bytes_a_second(top))} at {top / 1000:g} kHz, the record's "
            f"bytes a step and its heads the Belka's ([`RECORD.md`](../RECORD.md)). At "
            f"{top / 1000:g} kHz the flash programs its pages {kryska_program_share(top):.0%} of "
            f"the time at the sheet's typical {KRYSKA_PAGE_S[0] * 1e3:g} ms a page, "
            f"{kryska_program_share(top, True):.0%} at its longest, "
            f"{KRYSKA_PAGE_S[1] * 1e3:g} ms. With both of the hoof's accelerometer channels kept, "
            f"a third less: {fmt(KRYSKA_FLASH_BYTES / bytes_a_second(usual, True))} at {usual} Hz, "
            f"{fmt(KRYSKA_FLASH_BYTES / bytes_a_second(top, True))} at {top / 1000:g} kHz, the "
            f"pages programmed {kryska_program_share(top, both=True):.0%} of the time, "
            f"{kryska_program_share(top, True, True):.0%} at the longest — more than the flash "
            f"has; so Kryska keeps both up to {KRYSKA_BOTH_MOST_HZ / 1000:g} kHz, "
            f"{kryska_program_share(KRYSKA_BOTH_MOST_HZ, True, True):.0%} there at the longest, "
            f"and refuses them above it.").replace(".7 ms", ",7 ms").replace(".5 ms", ",5 ms")
    # the per cent kept with its figure: a no-break space while the lines are cut
    return textwrap.fill(text.replace("%", "\u00a0%"), 100,
                         break_on_hyphens=False).replace("\u00a0", " ")


def line_kryska_sessions():
    """The memory in blocks, the list's cap and the shortest session that fills the memory at
    the cap, in ordinary use."""
    blocks = KRYSKA_FLASH_BYTES // BLOCK_BYTES
    shortest = KRYSKA_FLASH_BYTES / KRYSKA_SESSIONS_MAX / bytes_a_second(KRYSKA_USUAL_HZ)
    text = (f"The memory is {blocks:,} blocks; its list of sessions holds "
            f"{KRYSKA_SESSIONS_MAX:,}, the newest, which over {KRYSKA_FLASH_BYTES / 2**20:.0f} MB "
            f"is a session of {shortest:.0f} s at {KRYSKA_USUAL_HZ} Hz at the least — the older "
            f"ones wait for the erase.").replace(",0", " 0").replace(",1", " 1")
    return textwrap.fill(text, 100, break_on_hyphens=False)


if __name__ == "__main__":
    print(table_hold())
    print()
    print(line_rate())
    print(line_block())
    print(line_files())
