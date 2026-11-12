"""NIC-Strelka — the set given to a Belka by its cable: STRELKA.SET, written on the Belka's disk
and taken by the Belka when the cable leaves (firmware/common/setfile.h; FIRMWARE.md, What a unit
keeps). The application writes it so; this is the reference, held to the firmware's reader by
tools/test_firmware.py.

    python3 tools/strelka_set.py A1B2C3D4 7 0102030405060708090A0B0C0D0E0F10 5 > STRELKA.SET
"""

import sys

NAME = "STRELKA.SET"
MAX_BYTES = 512                    # the most a unit reads of it (firmware/common/setfile.h)


def set_file(address, channel, key, number):
    """The file's text: the set's address u32, its channel 0-38, its key of 16 B, the Belka's
    number 1-63."""
    assert 0 < address <= 0xFFFFFFFF and 0 <= channel <= 38 and len(key) == 16
    assert 1 <= number <= 63
    return ("# NIC-Strelka: the set of this Belka, taken when its cable leaves, then wiped\n"
            f"set = {address:08X}\nchannel = {channel}\nkey = {key.hex().upper()}\n"
            f"number = {number}\n")


def parse(text):
    """The file read back: a dict of address, channel, key, number; ValueError on anything else."""
    if len(text.encode()) > MAX_BYTES:
        raise ValueError("longer than a unit reads")
    got = {}
    for line in text.splitlines():
        line = line.strip()
        if not line or line.startswith("#"):
            continue
        name, _, value = line.partition("=")
        name, value = name.strip(), value.strip()
        if not _ or name in got or name not in ("set", "channel", "key", "number"):
            raise ValueError(f"the line {line!r}")
        got[name] = value
    if len(got) != 4:
        raise ValueError("a key missing")
    # exactly as the firmware reads it (common/setfile.c): 8 and 32 hex digits, 1-3 decimal
    # digits, nothing else — no 0x, no sign, no spaces
    import re
    if not re.fullmatch(r"[0-9A-Fa-f]{8}", got["set"]) or \
            not re.fullmatch(r"[0-9A-Fa-f]{32}", got["key"]) or \
            not re.fullmatch(r"[0-9]{1,3}", got["channel"]) or \
            not re.fullmatch(r"[0-9]{1,3}", got["number"]):
        raise ValueError("a value malformed")
    address, channel = int(got["set"], 16), int(got["channel"])
    key, number = bytes.fromhex(got["key"]), int(got["number"])
    if not address or not 0 <= channel <= 38 or not 1 <= number <= 63:
        raise ValueError("a value out of range")
    return dict(address=address, channel=channel, key=key, number=number)


if __name__ == "__main__":
    if len(sys.argv) != 5:
        sys.exit(__doc__)
    sys.stdout.write(set_file(int(sys.argv[1], 16), int(sys.argv[2]), bytes.fromhex(sys.argv[3]),
                              int(sys.argv[4])))
