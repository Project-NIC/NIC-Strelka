"""The Belka's one image for MCUboot, the FLPR's program inside it (FIRMWARE.md, The sensor on the
FLPR), as Nordic's guide asks (the SDK's nrf54l_signing_with_payload.rst): both programs built
apart, their hex files merged, the whole signed as one, by the same imgtool command and key the
Belka's sysbuild used, read from its build.

    python3 firmware/flpr/merge.py <build-belka> <build-flpr>

writes <build-belka>/belka_flpr.signed.hex, to flash after MCUboot's merged.hex, and
belka_flpr.signed.bin, the update over USB or the radio. It checks first that the Belka's code ends
below the FLPR's partition and that the FLPR's lies inside it.
"""

import os
import re
import shlex
import subprocess
import sys

from intelhex import IntelHex

FLPR_START, FLPR_END = 0xE0000, 0xF8000      # belka/boards/flpr.overlay, flpr's overlay


def sign_command(build_belka):
    """The imgtool command sysbuild ran on the Belka's zephyr.hex, from its build.ninja."""
    ninja = open(os.path.join(build_belka, "belka", "build.ninja"), encoding="utf-8").read()
    # one command of several on the line, joined by &&: the one that signs zephyr.hex
    m = re.search(r"(\S*imgtool\.py) sign ((?:(?!&&).)*?) "
                  r"(\S*zephyr\.hex) (\S*zephyr\.signed\.hex)", ninja)
    if not m:
        sys.exit("no imgtool sign command for zephyr.hex in the Belka's build.ninja")
    return m.group(1), shlex.split(m.group(2))


def main(build_belka, build_flpr):
    app = IntelHex(os.path.join(build_belka, "belka", "zephyr", "zephyr.hex"))
    flpr = IntelHex(os.path.join(build_flpr, "zephyr", "zephyr.hex"))
    if app.maxaddr() >= FLPR_START:
        sys.exit(f"the Belka's code reaches 0x{app.maxaddr():x}, into the FLPR's partition")
    if flpr.minaddr() < FLPR_START or flpr.maxaddr() >= FLPR_END:
        sys.exit(f"the FLPR's code 0x{flpr.minaddr():x}-0x{flpr.maxaddr():x} is outside its "
                 f"partition 0x{FLPR_START:x}-0x{FLPR_END:x}")
    merged = os.path.join(build_belka, "belka_flpr.hex")
    flpr.start_addr = None          # the entry point is the Belka's; the FLPR's, its launcher's
    app.merge(flpr, overlap="error")
    app.write_hex_file(merged)
    imgtool, args = sign_command(build_belka)
    py = sys.executable
    for ext in ("hex", "bin"):
        out = os.path.join(build_belka, f"belka_flpr.signed.{ext}")
        subprocess.check_call([py, imgtool, "sign", *args, merged, out])
        print(f"wrote {out}")


if __name__ == "__main__":
    if len(sys.argv) != 3:
        sys.exit(__doc__)
    main(sys.argv[1], sys.argv[2])
