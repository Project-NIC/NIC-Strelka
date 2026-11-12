"""SMP over TCP as Strelka speaks it (COMMANDS.md): the header of 8 B, big-endian, then a CBOR
map; the frames on a connection one after another; the lock first on every connection. A client
for the application's side and for the tests, and what the Povodok simulator shares with it.

    import smp
    with smp.Client("192.168.7.1", 1337, password="strelka1") as c:   # the lock passed
        print(c.read(smp.GROUP_STRELKA, smp.INFO))
        c.write(smp.GROUP_STRELKA, smp.RECORD, {"run": True})
        belka = smp.Client("192.168.7.1", 1337 + 3, password="strelka1")  # the Belka numbered 3

And SMP on Kryska's serial port on the cable, in Zephyr's "SMP over console" framing: each frame
base64 in lines of 127 characters at most, the first opening with 06 09, the next with 04 14, a
length of two bytes before the frame and a CRC-16 after it, the console's own lines between them
passed over; no lock on the cable, the hand on the unit being the key (FIRMWARE.md, USB):

    with smp.SerialClient("/dev/ttyACM0") as k:
        k.give_set("STRELKA.SET")                               # the set, as a Belka's disk takes it
        data = k.download("/SD:/S00007B03.STRK")

The lock agrees a secret by X25519 on every connection (the cryptography library does it) and
answers the nonce with an HMAC over the nonce and the secret. An answer is the CBOR map; a Zephyr code comes back as SmpError(rc), a group's own as
SmpError(rc, group). Standard codes (mgmt_defines.h): 0 all well, 1 unknown, 2 no memory, 3 a bad
value, 4 a timeout, 5 not found, 6 a bad state, 7 too large, 8 unsupported, 9 corrupt, 10 busy,
11 access denied, 12 and 13 a version too old or too new. Strelka's group 64: 1 the bridged
target out of reach.
"""

import base64
import hashlib
import hmac
import os
import socket
import struct

import cbor2

HEADER = struct.Struct(">BBHHBB")
VERSION = 1
FRAME_MAX = 2048

# SMP over console (Zephyr's mgmt/mcumgr/transport/serial.h and serial_util.c)
SERIAL_PKT, SERIAL_FRAG = b"\x06\x09", b"\x04\x14"
SERIAL_LINE_MAX = 127                     # the two bytes opening, the base64, the newline
SERIAL_RAW_A_LINE = (SERIAL_LINE_MAX - 3) // 4 * 3   # 93 raw bytes a line

OP_READ, OP_READ_RSP, OP_WRITE, OP_WRITE_RSP = 0, 1, 2, 3
GROUP_OS, GROUP_IMG, GROUP_FS, GROUP_STRELKA = 0, 1, 8, 64
PORT = 1337

# Strelka's group, 64
(INFO, SESSION, SESSIONS, CAL, BLINK, CARD, OFF, FORGET, KEY, JOIN, RECORD, LIVE, SEARCH, TAKE,
 SET, DROP, AUTH, PASSWORD, TABLE, GIVE, WORK, FETCH, CALIBRATE, DIAG, MOVE) = range(25)
FETCH_PORT = PORT + 64                    # a download's frames out, 64/21 (COMMANDS.md)

# the codes
(OK, UNKNOWN, NOMEM, INVAL, TIMEOUT, NOENT, BAD_STATE, TOO_LARGE, NOTSUP, CORRUPT, BUSY, ACCESS,
 TOO_OLD, TOO_NEW) = range(14)
STRELKA_UNREACHABLE = 1

ERRORS = {0: "all well", 1: "unknown", 2: "no memory", 3: "a bad value", 4: "a timeout",
          5: "not found", 6: "a bad state", 7: "too large", 8: "unsupported", 9: "corrupt",
          10: "busy", 11: "access denied", 12: "a version too old", 13: "a version too new"}
STRELKA_ERRORS = {1: "the bridged target out of reach"}


class SmpError(Exception):
    def __init__(self, rc, group=None):
        self.rc, self.group = rc, group
        if group is None:
            what = ERRORS.get(rc, "code %d" % rc)
        elif group == GROUP_STRELKA:
            what = "group 64: " + STRELKA_ERRORS.get(rc, "code %d" % rc)
        else:
            what = "group %d: code %d" % (group, rc)
        super().__init__(what)


def frame(op, group, seq, cmd, body=None, version=VERSION):
    """A frame: the header and the CBOR body; `body` a dict, or bytes already encoded."""
    if body is None:
        body = b""
    elif not isinstance(body, bytes):
        body = cbor2.dumps(body)
    return HEADER.pack((op & 7) | (version << 3), 0, len(body), group, seq, cmd) + body


def parse_header(data):
    """op, version, flags, length, group, seq, cmd of the 8 B given."""
    op_ver, flags, length, group, seq, cmd = HEADER.unpack(data[:8])
    return op_ver & 7, op_ver >> 3, flags, length, group, seq, cmd


def agree(peer_pub):
    """A fresh X25519 pair for this connection: its public key, and the SHA-256 of the secret
    agreed with the peer's public key — the lock's secret (COMMANDS.md, The lock)."""
    from cryptography.hazmat.primitives import serialization
    from cryptography.hazmat.primitives.asymmetric.x25519 import X25519PrivateKey, X25519PublicKey
    key = X25519PrivateKey.generate()
    pub = key.public_key().public_bytes(serialization.Encoding.Raw, serialization.PublicFormat.Raw)
    secret = hashlib.sha256(key.exchange(X25519PublicKey.from_public_bytes(peer_pub))).digest()
    return pub, secret


def mac_for(password, nonce, secret):
    """What the lock wants: HMAC-SHA256 with the password's SHA-256 as the key over the nonce and
    the secret agreed for the connection, so that a listener has nothing to try passwords on."""
    return hmac.new(hashlib.sha256(password).digest(), nonce + secret, "sha256").digest()


def answer_body(body):
    """The map of an answer, or the SmpError it carries."""
    if "rc" in body and body["rc"] != 0:
        raise SmpError(body["rc"])
    if "err" in body and body["err"].get("rc", 0) != 0:
        raise SmpError(body["err"]["rc"], body["err"].get("group"))
    return body


def crc16_itu_t(data, crc=0):
    """CRC-16 as Zephyr's crc16_itu_t: the polynomial 0x1021, no reflection, no final xor."""
    for byte in data:
        crc ^= byte << 8
        for _ in range(8):
            crc = ((crc << 1) ^ 0x1021) & 0xFFFF if crc & 0x8000 else (crc << 1) & 0xFFFF
    return crc


def serial_lines(packet):
    """A frame on the serial port: the length of the frame and its CRC, big-endian, round it;
    the bytes in pieces of 93, each base64 on a line of its own, the first opened by 06 09, the
    rest by 04 14, every line ended by a newline."""
    raw = struct.pack(">H", len(packet) + 2) + packet + struct.pack(">H", crc16_itu_t(packet))
    out = b""
    for i in range(0, len(raw), SERIAL_RAW_A_LINE):
        out += (SERIAL_PKT if i == 0 else SERIAL_FRAG) + \
            base64.b64encode(raw[i:i + SERIAL_RAW_A_LINE]) + b"\n"
    return out


class SerialStream:
    """The serial port's bytes cut into frames: a line opened by 06 09 begins one, lines opened by
    04 14 go on with it, any other line is the console's and passed over; a frame whose CRC fails
    or whose length does not add up is dropped."""

    def __init__(self):
        self.buf = b""
        self.raw = b""
        self.want = None

    def feed(self, data):
        self.buf += data
        frames = []
        while b"\n" in self.buf:
            line, self.buf = self.buf.split(b"\n", 1)
            line = line.rstrip(b"\r")
            if line[:2] == SERIAL_PKT:
                self.raw, self.want = b"", None
            elif line[:2] != SERIAL_FRAG or self.want is None and not self.raw:
                continue
            try:
                self.raw += base64.b64decode(line[2:], validate=True)
            except ValueError:
                self.raw, self.want = b"", None
                continue
            if self.want is None and len(self.raw) >= 2:
                self.want = struct.unpack(">H", self.raw[:2])[0]
                self.raw = self.raw[2:]
            if self.want is not None and len(self.raw) >= self.want:
                body = self.raw[:self.want]
                self.raw, self.want = b"", None
                if len(body) >= 2 and crc16_itu_t(body) == 0:
                    frames.append(body[:-2])
        return frames


class Stream:
    """A TCP stream cut into frames, as common/smp.c does it."""

    def __init__(self):
        self.buf = b""

    def feed(self, data):
        """The whole frames arrived so far, header included; a header saying more than
        FRAME_MAX raises ValueError and the connection is to be closed."""
        self.buf += data
        frames = []
        while len(self.buf) >= 8:
            length = HEADER.unpack(self.buf[:8])[2]
            if 8 + length > FRAME_MAX:
                raise ValueError("a frame of %d B" % (8 + length))
            if len(self.buf) < 8 + length:
                break
            frames.append(self.buf[:8 + length])
            self.buf = self.buf[8 + length:]
        return frames


class Client:
    """One connection: the lock passed in __init__ when a password is given."""

    def __init__(self, host, port=PORT, password=None, timeout=5.0, pinned=None):
        self.sock = socket.create_connection((host, port), timeout=timeout)
        self.stream = Stream()
        self.seq = 0
        self.unit_key = None
        if password is not None:
            self.unlock(password, pinned)

    def close(self):
        self.sock.close()

    def __enter__(self):
        return self

    def __exit__(self, *exc):
        self.close()

    def request(self, op, group, cmd, body=None, version=VERSION):
        """One request, its answer's map back, an error raised."""
        seq = self.seq
        self.seq = (self.seq + 1) & 0xFF
        self.sock.sendall(frame(op, group, seq, cmd, body, version))
        while True:
            data = self.sock.recv(4096)
            if not data:
                raise ConnectionError("the connection closed")
            for got in self.stream.feed(data):
                r_op, _, _, length, r_group, r_seq, r_cmd = parse_header(got)
                if r_seq != seq or r_group != group or r_cmd != cmd or r_op != op + 1:
                    continue
                return answer_body(cbor2.loads(got[8:8 + length]) if length else {})

    def read(self, group, cmd, body=None):
        return self.request(OP_READ, group, cmd, body)

    def write(self, group, cmd, body=None):
        return self.request(OP_WRITE, group, cmd, body)

    def unlock(self, password, pinned=None):
        """64/16: the nonce read, the MAC written. With [pinned], the unit's own key the
        application kept: another key refused before the MAC goes out (COMMANDS.md, The lock).
        The key met is left in `unit_key`."""
        if isinstance(password, str):
            password = password.encode("utf-8")
        challenge = self.read(GROUP_STRELKA, AUTH)
        self.unit_key = challenge["pub"]
        if pinned is not None and challenge["pub"] != pinned:
            raise ValueError("another key than the one pinned: not the Povodok joined before")
        pub, secret = agree(challenge["pub"])
        self.write(GROUP_STRELKA, AUTH, {"pub": pub, "mac": mac_for(password, challenge["nonce"],
                                                                     secret)})

    def afresh(self, password, cmd, body=None):
        """64/5 or 64/7: the lock written again just before, the password asked again
        (COMMANDS.md, The lock)."""
        self.unlock(password)
        return self.write(GROUP_STRELKA, cmd, body)

    def change_password(self, old, new):
        """64/17: the old proved by a fresh nonce's MAC, the new one's SHA-256 set."""
        old = old.encode("utf-8") if isinstance(old, str) else old
        new = new.encode("utf-8") if isinstance(new, str) else new
        challenge = self.read(GROUP_STRELKA, AUTH)
        pub, secret = agree(challenge["pub"])
        self.write(GROUP_STRELKA, PASSWORD, {"pub": pub,
                                             "mac": mac_for(old, challenge["nonce"], secret),
                                             "new": hashlib.sha256(new).digest()})

    def give(self, password, n, address, channel, key, label=0):
        """64/19: the set given to a Povodok — its number 0–3, the set's address u32, channel and
        key of 16 B, and the set's own number 1–9999 if it has one — under a fresh lock of its
        own, the key sealed by AES-CCM with the first 16 B of the agreed secret (COMMANDS.md)."""
        from cryptography.hazmat.primitives.ciphers.aead import AESCCM
        password = password.encode("utf-8") if isinstance(password, str) else password
        challenge = self.read(GROUP_STRELKA, AUTH)
        pub, secret = agree(challenge["pub"])
        nonce = os.urandom(13)
        sealed = AESCCM(secret[:16], tag_length=4).encrypt(nonce, key, None)
        return self.write(GROUP_STRELKA, GIVE, {
            "pub": pub, "mac": mac_for(password, challenge["nonce"], secret), "n": n,
            "addr": address.to_bytes(4, "little"), "ch": channel, "key": sealed[:16],
            "nonce": nonce, "mic": sealed[16:], **({"label": label} if label else {})})

    def upload(self, name, data):
        """8/0 written: a file given whole, in pieces the frame holds, as Zephyr's file group
        takes it — on Kryska's NOR way STRELKA.SET alone, the set given by the cable
        (chapters/kryska-memory.md)."""
        off, piece = 0, 256
        while off < len(data) or off == 0:
            body = {"name": name, "off": off, "data": data[off:off + piece]}
            if off == 0:
                body["len"] = len(data)
            rsp = self.write(GROUP_FS, 0, body)
            off = rsp.get("off", off + len(body["data"]))
            if not data:
                break

    def give_set(self, path):
        """The set file, tools/strelka_set.py's, given to a Kryska on its cable."""
        with open(path, "rb") as f:
            self.upload("/SD:/STRELKA.SET", f.read())

    def download(self, name, start=0):
        """8/0: a file from byte `start` on, the whole from 0, as Zephyr's file group gives it; a
        download broken off goes on from the byte it had, the length then from 8/1, since 8/0
        says it at offset 0 alone (APP.md, COMMANDS.md)."""
        off, data, length = start, bytearray(), None
        if start:
            length = self.read(GROUP_FS, 1, {"name": name})["len"]
        while length is None or off < length:
            rsp = self.read(GROUP_FS, 0, {"name": name, "off": off})
            if off == 0:
                length = rsp["len"]
            data += rsp["data"]
            off = rsp["off"] + len(rsp["data"])
            if not rsp["data"]:
                break
        return bytes(data)


class SerialClient(Client):
    """SMP on a unit's serial port on the cable, Kryska's: the frames in Zephyr's console
    framing, no lock. The device is opened raw through pyserial when it is there, else as a
    file, a CDC ACM port needing no speed set."""

    def __init__(self, device, timeout=5.0):
        self.timeout = timeout
        try:
            import serial
            self.port = serial.Serial(device, 115200, timeout=timeout)
            self.fd = None
        except ImportError:
            self.port = None
            self.fd = os.open(device, os.O_RDWR | os.O_NOCTTY)
            try:
                import termios
                import tty
                tty.setraw(self.fd)
                attrs = termios.tcgetattr(self.fd)
                attrs[6][termios.VMIN], attrs[6][termios.VTIME] = 0, max(1, int(timeout * 10))
                termios.tcsetattr(self.fd, termios.TCSANOW, attrs)
            except (ImportError, OSError):
                pass
        self.stream = SerialStream()
        self.seq = 0

    def close(self):
        if self.port is not None:
            self.port.close()
        elif self.fd is not None:
            os.close(self.fd)
            self.fd = None

    def _send(self, data):
        if self.port is not None:
            self.port.write(data)
        else:
            os.write(self.fd, data)

    def _recv(self):
        if self.port is not None:
            first = self.port.read(1)        # waits for the first byte, the timeout at most
            return first + self.port.read(self.port.in_waiting) if first else b""
        return os.read(self.fd, 4096)

    def request(self, op, group, cmd, body=None, version=VERSION):
        seq = self.seq
        self.seq = (self.seq + 1) & 0xFF
        self._send(serial_lines(frame(op, group, seq, cmd, body, version)))
        while True:
            data = self._recv()
            if not data:
                raise TimeoutError("no answer on the serial port")
            for got in self.stream.feed(data):
                if len(got) < 8:
                    continue
                r_op, _, _, length, r_group, r_seq, r_cmd = parse_header(got)
                if r_seq != seq or r_group != group or r_cmd != cmd or r_op != op + 1:
                    continue
                return answer_body(cbor2.loads(got[8:8 + length]) if length else {})
