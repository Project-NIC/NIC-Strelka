"""SMP over TCP as Strelka speaks it (COMMANDS.md): the header of 8 B, big-endian, then a CBOR
map; the frames on a connection one after another; the lock first on every connection. A client
for the application's side and for the tests, and what the Povodok simulator shares with it.

    import smp
    with smp.Client("192.168.7.1", 1337, password="strelka1") as c:   # the lock passed
        print(c.read(smp.GROUP_STRELKA, smp.INFO))
        c.write(smp.GROUP_STRELKA, smp.RECORD, {"run": True})
        belka = smp.Client("192.168.7.1", 1337 + 3, password="strelka1")  # the Belka numbered 3

The lock agrees a secret by X25519 on every connection (the cryptography library does it) and
answers the nonce with an HMAC over the nonce and the secret. An answer is the CBOR map; a Zephyr code comes back as SmpError(rc), a group's own as
SmpError(rc, group). Standard codes (mgmt_defines.h): 0 all well, 1 unknown, 2 no memory, 3 a bad
value, 4 a timeout, 5 not found, 6 a bad state, 7 too large, 8 unsupported, 9 corrupt, 10 busy,
11 access denied, 12 and 13 a version too old or too new. Strelka's group 64: 1 the bridged
target out of reach.
"""

import hashlib
import hmac
import socket
import struct

import cbor2

HEADER = struct.Struct(">BBHHBB")
VERSION = 1
FRAME_MAX = 2048

OP_READ, OP_READ_RSP, OP_WRITE, OP_WRITE_RSP = 0, 1, 2, 3
GROUP_OS, GROUP_IMG, GROUP_FS, GROUP_STRELKA = 0, 1, 8, 64
PORT = 1337

# Strelka's group, 64
(INFO, SESSION, SESSIONS, CAL, BLINK, CARD, OFF, FORGET, KEY, JOIN, RECORD, LIVE, SEARCH, TAKE,
 SET, DROP, AUTH, PASSWORD, TABLE) = range(19)

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
            if length > FRAME_MAX:
                raise ValueError("a frame of %d B" % length)
            if len(self.buf) < 8 + length:
                break
            frames.append(self.buf[:8 + length])
            self.buf = self.buf[8 + length:]
        return frames


class Client:
    """One connection: the lock passed in __init__ when a password is given."""

    def __init__(self, host, port=PORT, password=None, timeout=5.0):
        self.sock = socket.create_connection((host, port), timeout=timeout)
        self.stream = Stream()
        self.seq = 0
        if password is not None:
            self.unlock(password)

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

    def unlock(self, password):
        """64/16: the nonce read, the MAC written."""
        if isinstance(password, str):
            password = password.encode("utf-8")
        challenge = self.read(GROUP_STRELKA, AUTH)
        pub, secret = agree(challenge["pub"])
        self.write(GROUP_STRELKA, AUTH, {"pub": pub, "mac": mac_for(password, challenge["nonce"],
                                                                     secret)})

    def change_password(self, old, new):
        """64/17: the old proved by a fresh nonce's MAC, the new one's SHA-256 set."""
        old = old.encode("utf-8") if isinstance(old, str) else old
        new = new.encode("utf-8") if isinstance(new, str) else new
        challenge = self.read(GROUP_STRELKA, AUTH)
        pub, secret = agree(challenge["pub"])
        self.write(GROUP_STRELKA, PASSWORD, {"pub": pub,
                                             "mac": mac_for(old, challenge["nonce"], secret),
                                             "new": hashlib.sha256(new).digest()})

    def download(self, name, chunk_hint=None):
        """8/0: a file whole, from byte 0 on, as Zephyr's file group gives it."""
        off, data, length = 0, bytearray(), None
        while length is None or off < length:
            rsp = self.read(GROUP_FS, 0, {"name": name, "off": off})
            if off == 0:
                length = rsp["len"]
            data += rsp["data"]
            off = rsp["off"] + len(rsp["data"])
            if not rsp["data"]:
                break
        return bytes(data)
