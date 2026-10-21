"""The live view's message, a message a period over UDP from the first Povodok to the phone
(PHONE.md, The live view): built and read here, byte for byte the firmware's
(firmware/common/live_msg.c), with the quaternion's packing of PRINCIPLE.md — the smallest three,
2 + 3 × 10 bits — for the application and the tests. Povodok sends it to the address the
application's connection on port 1337 came from, to UDP port 1337 there.

    import live_view
    with live_view.Receiver(port=1337) as r:          # bound on every address
        msg = r.next()                                # or None after the timeout
        for belka in msg["belkas"]:
            print(belka["n"], belka["steps"])        # quaternions as (w, x, y, z), raw as tuples
"""

import math
import socket
import struct

HEAD = struct.Struct("<BBHIHBB4HB")
ENTRY = struct.Struct("<BBBH")
STEP = struct.Struct("<hhhhhh")
KIND, VERSION = 1, 1
MAX_BYTES = 1400
LIVE_OFF, QUAT_50, QUAT_100, RAW_50, RAW_100 = 0, 1, 2, 4, 5
STEPS = {LIVE_OFF: 0, QUAT_50: 1, QUAT_100: 2, RAW_50: 1, RAW_100: 2}
STEP_BYTES = {LIVE_OFF: 0, QUAT_50: 4, QUAT_100: 4, RAW_50: 12, RAW_100: 12}
ROOT_HALF = 0.70710678
SCALE = 511.0


def body_bytes(code):
    return STEPS[code] * STEP_BYTES[code]


def pack_quat(q):
    """w, x, y, z of unit length into 4 B: the largest component, kept positive, named by 2 bits
    and left out, the other three as ±511 for ±1/√2."""
    big = max(range(4), key=lambda i: abs(q[i]))
    sign = -1.0 if q[big] < 0 else 1.0
    v = big << 30
    shift = 20
    for i in range(4):
        if i == big:
            continue
        n = int(round(q[i] * sign / ROOT_HALF * SCALE))
        n = max(-511, min(511, n))
        v |= (n & 0x3FF) << shift
        shift -= 10
    return v


def unpack_quat(v):
    big = v >> 30
    q = [0.0] * 4
    total = 0.0
    shift = 20
    for i in range(4):
        if i == big:
            continue
        n = (v >> shift) & 0x3FF
        if n & 0x200:
            n -= 0x400
        q[i] = n / SCALE * ROOT_HALF
        total += q[i] * q[i]
        shift -= 10
    q[big] = math.sqrt(1.0 - total) if total < 1.0 else 0.0
    return tuple(q)


def build(session, period, state, s0, povodoks, povodok_states, belkas):
    """One message: `belkas` a list of (n, s0, by, state, steps), steps a list of quaternion
    tuples or raw 6-tuples as the entry's S0 says; raises ValueError when it does not fit."""
    out = bytearray(HEAD.pack(KIND, VERSION, session, period, state, s0, povodoks,
                              *povodok_states, len(belkas)))
    for n, b_s0, by, b_state, steps in belkas:
        code = b_s0 >> 4
        if len(steps) != STEPS[code]:
            raise ValueError("Belka %d: %d steps for code %d" % (n, len(steps), code))
        out += ENTRY.pack(n, b_s0, by, b_state)
        for step in steps:
            if STEP_BYTES[code] == 4:
                out += struct.pack("<I", pack_quat(step) if isinstance(step, tuple) else step)
            else:
                out += STEP.pack(*step)
    if len(out) > MAX_BYTES:
        raise ValueError("a message of %d B" % len(out))
    return bytes(out)


def split(session, period, state, s0, povodoks, povodok_states, belkas):
    """A period's Belkas in one message or two of the same head."""
    messages, part = [], []
    for entry in belkas:
        try:
            build(session, period, state, s0, povodoks, povodok_states, part + [entry])
            part.append(entry)
        except ValueError:
            messages.append(build(session, period, state, s0, povodoks, povodok_states, part))
            part = [entry]
    messages.append(build(session, period, state, s0, povodoks, povodok_states, part))
    return messages


def parse(data, quats_as_tuples=True):
    """A message into a dict: the head's fields and 'belkas', each with n, s0, by, state, steps."""
    kind, version, session, period, state, s0, povodoks, p0, p1, p2, p3, count = \
        HEAD.unpack_from(data)
    if kind != KIND or version != VERSION:
        raise ValueError("not a live view message")
    msg = dict(session=session, period=period, state=state, s0=s0, live=s0 >> 4, item=s0 & 0xF,
               povodoks=povodoks, povodok_states=(p0, p1, p2, p3), belkas=[])
    at = HEAD.size
    for _ in range(count):
        n, b_s0, by, b_state = ENTRY.unpack_from(data, at)
        at += ENTRY.size
        code = b_s0 >> 4
        if code not in STEPS or n < 1 or n > 63:
            raise ValueError("a malformed entry")
        steps = []
        for _ in range(STEPS[code]):
            if STEP_BYTES[code] == 4:
                v = struct.unpack_from("<I", data, at)[0]
                steps.append(unpack_quat(v) if quats_as_tuples else v)
            else:
                steps.append(STEP.unpack_from(data, at))
            at += STEP_BYTES[code]
        msg["belkas"].append(dict(n=n, s0=b_s0, live=code, item=b_s0 & 0xF, by=by,
                                  state=b_state, steps=steps))
    if at != len(data):
        raise ValueError("%d bytes left" % (len(data) - at))
    return msg


class Receiver:
    """The messages as they come, on UDP port 1337 by default."""

    def __init__(self, port=1337, host="", timeout=1.0):
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        self.sock.bind((host, port))
        self.sock.settimeout(timeout)

    def next(self):
        """The next message parsed, or None after the timeout."""
        try:
            data, _ = self.sock.recvfrom(2048)
        except socket.timeout:
            return None
        return parse(data)

    def close(self):
        self.sock.close()

    def __enter__(self):
        return self

    def __exit__(self, *exc):
        self.close()
