"""The set emulated on the host: firmware/tests/test_world.c built as a program, the logic of
belka_link.c and povodok_link.c run period by period for as many Belkas and Povodoks as asked, the
radio's losses drawn at random, and what a set never shows told — each Belka's LIVEs sent, heard
by the first Povodok itself, passed to it, held at the period's end, the PASS windows' overflow,
and with -v the first, its channel and the share held, second by second.

    python3 tools/world.py --belkas 63 --povodoks 4 --seconds 60 --live 5 -v
    python3 tools/world.py --belkas 40 --near 90 --side 50 --far 10 --seed 7

--belkas 1-63 (32), --povodoks 1-4 (4), --seconds (60), --live the live view's code 0, 1, 2, 4
or 5 (1), --near, --side, --far the share a Povodok hears of the Belkas on its own side, the next
and the far one (95, 70, 30), --seed for another draw. Wants gcc and zcbor's C sources, as
tools/test_firmware.py does.
"""

import os
import subprocess
import sys
import tempfile

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import test_firmware as tf  # noqa: E402


def main(argv):
    tmp = tempfile.mkdtemp()
    exe = tf.build(tmp, "test_world", tf.zcbor_objects(tmp), defines=("-DWORLD_SIM",))
    return subprocess.call([exe, *argv[1:]])


if __name__ == "__main__":
    sys.exit(main(sys.argv))
