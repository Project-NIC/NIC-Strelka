"""tools/smp.py's framing on the serial port held to Zephyr's SMP over console (serial.h and
serial_util.c): a frame of 2 000 B goes in lines of 127 characters at most, the first opened by
06 09, the rest by 04 14, the length and the CRC-16 round it, and comes back whole through the
console's own lines, in pieces of any size; a line with a broken CRC or a cut frame is dropped.

    python3 tools/test_smp.py
"""

import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import smp  # noqa: E402


def main():
    packet = smp.frame(smp.OP_READ, smp.GROUP_FS, 7, 0, {"name": "/SD:/S00007B03.STRK", "off": 0})
    big = smp.frame(smp.OP_READ_RSP, smp.GROUP_FS, 7, 0, {"off": 0, "data": bytes(range(256)) * 7,
                                                           "len": 123456})
    assert smp.crc16_itu_t(b"123456789") == 0x31C3, "CRC-16 ITU-T's check value"
    for frame in (packet, big):
        lines = smp.serial_lines(frame)
        for line in lines.split(b"\n")[:-1]:
            assert len(line) + 1 <= smp.SERIAL_LINE_MAX, len(line)
        assert lines.startswith(smp.SERIAL_PKT) and lines.count(smp.SERIAL_PKT) == 1
        assert lines.count(b"\n" + smp.SERIAL_FRAG) == len(lines.split(b"\n")) - 2
        stream = smp.SerialStream()
        got = []
        noise = b"[00:00:01.000,000] <inf> flash_rec: the memory: 3 sessions\r\n"
        data = noise + lines + noise + lines
        for i in range(0, len(data), 61):
            got += stream.feed(data[i:i + 61])
        assert got == [frame, frame], (len(got), [len(g) for g in got])
    print("ok a frame in lines of 127, back whole through the console's lines, in pieces")

    lines = smp.serial_lines(packet)
    broken = bytearray(lines)
    broken[5] ^= 0x01
    stream = smp.SerialStream()
    assert stream.feed(bytes(broken)) == [] and stream.feed(lines) == [packet]
    cut = smp.serial_lines(big).split(b"\n")[0] + b"\n"
    assert stream.feed(cut) == [] and stream.feed(lines) == [packet]
    print("ok a broken frame and a cut one dropped, the next taken")
    print("SMP OK")


if __name__ == "__main__":
    main()
