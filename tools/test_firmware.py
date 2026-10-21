"""The firmware's common code checked on the host: firmware/common/ is compiled with gcc together
with each test under firmware/tests/, against a software AES for the chip's cryptography, and
run — the packets' sizes and the SYNC's signature, the record's blocks, the steps on the grid,
the fragments over ESB, the clock loop against a simulated crystal, the inside link's logic on
each side and a whole set of 32 Belkas and 4 Povodoks simulated with the radio's losses for some
three minutes. Two of the tests leave files
for this script to hold against Python: the sessions the C composer writes, byte for byte against
tools/strelka_record.py's blocks, and the AES-CCM tags against the cryptography library's.

    python3 tools/test_firmware.py
"""

import glob
import importlib.util
import os
import struct
import subprocess
import sys
import tempfile

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
COMMON = os.path.join(ROOT, "firmware", "common")
TESTS = os.path.join(ROOT, "firmware", "tests")
sys.path.insert(0, os.path.join(ROOT, "tools"))
import strelka_record as rec  # noqa: E402

CFLAGS = ["-std=c11", "-O1", "-Wall", "-Wextra", "-Werror", "-I", COMMON, "-I", TESTS]


def zcbor_dir():
    """zcbor's C sources, the same 0.9.1 the SDK carries: from the Python package of that version
    (pip install zcbor==0.9.1), found without importing it, which a newer cbor2 breaks."""
    spec = importlib.util.find_spec("zcbor")
    if spec is None or not spec.submodule_search_locations:
        sys.exit("zcbor's C sources are wanted: pip install zcbor==0.9.1")
    return list(spec.submodule_search_locations)[0]
TABLE = ("druh;kůň\njméno;Bára\nanamnéza;\"Od jara kulhá vpravo vpředu.\nKlusána na kruhu.\"\n"
         "belka;cip;misto\n1;0123456789ABCDEF;hlava, nánosník\n"
         "2;0123456789ABCDF0;kohoutek\n")


def zcbor_objects(tmp):
    """zcbor compiled once, with its own warnings left alone."""
    zc = zcbor_dir()
    objects = []
    for part in ("common", "decode", "encode"):
        obj = os.path.join(tmp, f"zcbor_{part}.o")
        subprocess.check_call(["gcc", "-std=c11", "-O1", "-w", "-I", os.path.join(zc, "include"),
                               "-c", os.path.join(zc, "src", f"zcbor_{part}.c"), "-o", obj])
        objects.append(obj)
    return objects


def build(tmp, name, zcbor):
    exe = os.path.join(tmp, name)
    sources = sorted(glob.glob(os.path.join(COMMON, "*.c")))
    subprocess.check_call(["gcc", *CFLAGS, "-I", os.path.join(zcbor_dir(), "include"),
                           os.path.join(TESTS, name + ".c"), os.path.join(TESTS, "ccm_soft.c"),
                           os.path.join(TESTS, "sha256_soft.c"), os.path.join(TESTS, "x25519_soft.c"),
                           *sources, *zcbor, "-lm", "-o", exe])
    return exe


def expected_session(lose=()):
    """What test_record.c composes, built with the Python builders: 130 steps at 100 Hz, N 10,
    from period 1000, m = k mod 300, the information block after every 120 steps, the end."""
    session, belka, steps, first_period = 7, 3, 130, 1000
    blocks = [rec.head_block(session, belka, 0x1122334455667788, fw=(0, 5, 0),
                             build=b"\x01\x02\x03\x04", rate_hz=100, n=10, acc_g=16,
                             gyro_dps=2000, first_period=first_period, time_period=first_period,
                             tz_min=120, cal_temp=2550, place="hlava, nánosník",
                             name=f"S{session:05d}B{belka:02d}.STRK")]
    blocks += rec.table_blocks(session, TABLE)
    spp = rec.steps_a_period(dict(rate_hz=100))
    number = 0
    for first in range(0, steps, rec.STEPS):
        n = min(rec.STEPS, steps - first)
        vals = []
        for k in range(first, first + n):
            m = k % 300
            vals.append(rec.MISSING if k in lose else (m, -m, 16384, 0, 100 * m, 100 - 100 * m))
        blocks.append(rec.data_block(session, number, first_period + first // spp, vals,
                                     place=(first % spp) // rec.STEPS, offset_ns=first % 7 - 3))
        number += 1
        if (first + n) % rec.INFO_STEPS == 0 or first + n == steps:
            start = (first + n - 1) // rec.INFO_STEPS * rec.INFO_STEPS
            entries = [(k % 7 - 3, -60 - k % 5, k % 4, 1 + k % 4) for k in range(start, first + n)]
            blocks.append(rec.info_block(session, number, first_period + start // spp, entries,
                                         temp_centi=2512, cell_mv=3290))
            number += 1
    blocks.append(rec.end_block(session, 1, steps, number, first_period + (steps - 1) // spp))
    return b"".join(blocks)


def check_record(tmp, exe):
    whole, lost = os.path.join(tmp, "whole.bin"), os.path.join(tmp, "lost.bin")
    subprocess.check_call([exe, whole, lost])
    got = open(whole, "rb").read()
    want = expected_session()
    assert len(got) == len(want) == 512 * (1 + 1 + 4 + 2 + 1), (len(got), len(want))
    for i in range(0, len(want), 512):
        assert got[i:i + 512] == want[i:i + 512], f"block {i // 512} differs from Python's"
    got = open(lost, "rb").read()
    assert got == expected_session(lose=(5, 77)), "the session with lost steps differs"
    s = rec.read(got)
    assert s["end"]["steps"] == 130 and s["data"][0]["missing"] == 1 and \
        s["data"][1]["missing"] == 1 and s["gaps"] == [] and s["bad"] == 0
    assert sum(1 for k, b, j, raw in rec.steps(s) if tuple(raw) == rec.MISSING) == 2
    print("ok the C composer's sessions, byte for byte Python's")


def check_frag(tmp, exe):
    from cryptography.hazmat.primitives.ciphers.aead import AESCCM
    path = os.path.join(tmp, "tags.txt")
    subprocess.check_call([exe, path])
    n = 0
    for line in open(path):
        parts = line.split()
        key, nonce = bytes.fromhex(parts[0]), bytes.fromhex(parts[1])
        ccm = AESCCM(key, tag_length=4)
        if parts[2].startswith("enc:"):
            plain = bytes.fromhex(parts[2][4:])
            assert ccm.encrypt(nonce, plain, None) == bytes.fromhex(parts[3]), "an encryption"
        else:
            data, tag = (bytes.fromhex(parts[2]), bytes.fromhex(parts[3])) if len(parts) == 4 \
                else (b"", bytes.fromhex(parts[2]))
            assert ccm.encrypt(nonce, b"", data) == tag, f"a tag over {len(data)} B"
        n += 1
    assert n == 7, n
    print(f"ok {n} AES-CCM tags, the C software's and Python's the same")


# ---- SMP and Strelka's group 64 (COMMANDS.md) --------------------------------------------------

PASSWORD = b"strelka1"
NEW_PASSWORD = b"novy-klic-2026"


def smp_frame(op, group, seq, cmd, body=None, version=1):
    """A request: the 8 B header, big-endian, then the CBOR map — or raw bytes for a bad body."""
    import cbor2
    body = b"" if body is None else body if isinstance(body, bytes) else cbor2.dumps(body)
    return struct.pack(">BBHHBB", op | (version << 3), 0, len(body), group, seq, cmd) + body


def test_random(n):
    """The random bytes tests/sha256_soft.c gives, the same each run, so that the nonces are known
    before the C runs and the MACs can be built here."""
    x = 0x243F6A8885A308D3
    out = bytearray()
    for _ in range(n):
        x ^= (x << 13) & 0xFFFFFFFFFFFFFFFF
        x ^= x >> 7
        x ^= (x << 17) & 0xFFFFFFFFFFFFFFFF
        out.append((x >> 24) & 0xFF)
    return bytes(out)


def chip_bytes(value):
    return value.to_bytes(8, "little")


def x25519_pair(private_bytes):
    from cryptography.hazmat.primitives.asymmetric.x25519 import X25519PrivateKey
    from cryptography.hazmat.primitives import serialization
    key = X25519PrivateKey.from_private_bytes(private_bytes)
    pub = key.public_key().public_bytes(serialization.Encoding.Raw,
                                        serialization.PublicFormat.Raw)
    return key, pub


def lock_mac(password_hash, nonce, app_key, povodok_pub):
    """The lock's MAC (COMMANDS.md, The lock): HMAC-SHA256 with the password's hash over the
    nonce and the SHA-256 of the X25519 secret."""
    import hashlib
    import hmac
    from cryptography.hazmat.primitives.asymmetric.x25519 import X25519PublicKey
    secret = hashlib.sha256(app_key.exchange(X25519PublicKey.from_public_bytes(povodok_pub))).digest()
    return hmac.new(password_hash, nonce + secret, "sha256").digest()


def smp_requests():
    """The 33 requests test_smp.c expects, in order, with the answers wanted. The C's nonces and
    its X25519 private keys come from the tests' random stream, 16 then 32 B a reading, so the
    answers are known here before it runs."""
    import hashlib
    read, write = 0, 2
    stream = test_random(3 * 48)
    nonces = [stream[48 * i:48 * i + 16] for i in range(3)]
    povodok_pubs = [x25519_pair(stream[48 * i + 16:48 * i + 48])[1] for i in range(3)]
    app_keys = [x25519_pair(bytes([0x40 + i]) * 32) for i in range(3)]
    old_hash = hashlib.sha256(PASSWORD).digest()
    new_hash = hashlib.sha256(NEW_PASSWORD).digest()
    cal = dict(acc=[16380, -16390, 16385, -16395, 16390, -16400], gscale=[32768, 32750, 32700],
               gzero=[5, -12, 3], temp=2550, t=1759999000)
    want_info = dict(chip=chip_bytes(0x0102030405060708), n=5, who=2, fw=[0, 7, 0],
                     build=b"\xde\xad\xbe\xef", sensor=0x44,
                     card=dict({"in": True}, mb=30436, free=29000, fs=2),
                     cell=dict(pct=87, mv=3291), temp=2512, state=4, fault=0, cal=1760000000)
    want_sessions = {"list": [dict(s=7, file="S00007B05.STRK", steps=130, blocks=9, p0=1000, end=1,
                                   bytes=4608),
                              dict(s=8, file="S00008B05.STRK", steps=1, blocks=4, p0=5000, end=0,
                                   bytes=2048)], "more": True}
    want_found = {"found": [dict(chip=chip_bytes(0x1122334455667788), rssi=-67, ago=3),
                            dict(chip=chip_bytes(0xA1A2A3A4A5A6A7A8), rssi=-80, ago=12)]}
    want_set = {"belkas": [dict(n=1, chip=chip_bytes(0x0101), ago=0, by=0, state=0, fault=0),
                           dict(n=2, chip=chip_bytes(0x0202), ago=3, by=2, state=1, fault=8)],
                "povodoks": 5, "ch": 23, "spare": 7}
    chip = bytes(range(1, 9))
    rows = [
        (read, 64, 0, {}, {"rc": 11}),                                   # before the lock
        (read, 64, 16, {}, {"nonce": nonces[0], "pub": povodok_pubs[0]}),
        (write, 64, 16, {"pub": app_keys[0][1], "mac": bytes(32)}, {"rc": 11}),  # a wrong MAC
        (read, 64, 16, {}, {"nonce": nonces[1], "pub": povodok_pubs[1]}),
        (write, 64, 16, {"pub": app_keys[1][1],
                         "mac": lock_mac(old_hash, nonces[1], app_keys[1][0], povodok_pubs[1])}, {}),
        (read, 64, 0, {}, want_info),
        (write, 64, 1, dict(s=7, rate=1000, n=10, acc=16, gyro=2000, live=1, stop=15,
                            t=1760000000000000, tz=120, p=123456), {}),
        (read, 64, 2, {"from": 2}, want_sessions),
        (write, 64, 3, cal, {}),
        (read, 64, 3, {}, cal),
        (write, 64, 4, {"s": 5}, {}),
        (write, 64, 5, {"do": 1}, {"rc": 10}),                           # busy, recording
        (write, 64, 6, {}, {}),
        (write, 64, 7, {}, {}),
        (write, 64, 8, {"pub": bytes(range(0x40, 0x60))}, {"pub": bytes(range(0x80, 0xA0))}),
        (write, 64, 9, dict(n=12, addr=b"\xd4\xc3\xb2\xa1", ch=7, key=bytes(range(1, 17)),
                            nonce=bytes(12) + b"\x6c", mic=b"\x00\x00\x00\xff"), {}),
        (write, 64, 10, {"run": True}, {}),
        (write, 64, 11, {"live": 4}, {}),
        (write, 64, 12, {"run": True}, {}),
        (read, 64, 12, {}, want_found),
        (write, 64, 13, {"chip": chip, "n": 34}, {}),
        (write, 64, 13, {"chip": chip, "n": 33}, {"rc": 5}),             # not heard
        (read, 64, 14, {}, want_set),
        (write, 64, 15, {"n": 3}, {}),
        (write, 64, 15, {"n": 9}, {"err": {"group": 64, "rc": 1}}),      # out of reach
        (read, 64, 16, {}, {"nonce": nonces[2], "pub": povodok_pubs[2]}),
        (write, 64, 17, {"pub": app_keys[2][1],
                         "mac": lock_mac(old_hash, nonces[2], app_keys[2][0], povodok_pubs[2]),
                         "new": new_hash}, {}),
        (write, 64, 18, {"off": 10, "len": 40, "csv": "druh;kůň\n"}, {}),
        (write, 64, 19, {}, {"rc": 8}),                                  # no such command
        (read, 64, 4, {}, {"rc": 8}),                                    # blink is a write
        (write, 64, 4, b"\xff\xff", {"rc": 3}),                          # not CBOR
        (write, 0, 0, {"d": "hi"}, {"rc": 8}),                           # MCUmgr's, not here
    ]
    frames = [smp_frame(op, group, seq, cmd, body) for seq, (op, group, cmd, body, _) in
              enumerate(rows)]
    frames.append(smp_frame(read, 64, len(rows), 0, {}, version=0))    # SMP's first version
    rows.append((read, 64, 0, {}, {"rc": 12}))
    return frames, rows


def check_smp(tmp, exe):
    import cbor2
    import hashlib
    import hmac
    requests, answers, hashes = (os.path.join(tmp, n) for n in ("req.bin", "ans.bin", "sha.txt"))
    frames, rows = smp_requests()
    open(requests, "wb").write(b"".join(frames))
    subprocess.check_call([exe, requests, answers, hashes])
    data = open(answers, "rb").read()
    at = 0
    for seq, (op, group, cmd, _, want) in enumerate(rows):
        op_ver, flags, length, got_group, got_seq, got_cmd = struct.unpack(">BBHHBB", data[at:at + 8])
        body = cbor2.loads(data[at + 8:at + 8 + length])
        at += 8 + length
        assert op_ver == (op + 1) | (1 << 3) and flags == 0 and got_group == group and \
            got_seq == seq and got_cmd == cmd, f"answer {seq}: the header"
        assert body == want, f"answer {seq} to {group}/{cmd}: {body!r}, wanted {want!r}"
    assert at == len(data), "bytes left after the answers"
    n = 0
    for line in open(hashes, encoding="utf-8"):
        text, digests = line.rstrip("\n").split("|")
        sha, mac = digests.split()
        assert hashlib.sha256(text.encode()).hexdigest() == sha, f"SHA-256 of {text!r}"
        assert hmac.new(b"key", text.encode(), "sha256").hexdigest() == mac, f"HMAC of {text!r}"
        n += 1
    assert n == 4
    print(f"ok {len(rows)} SMP requests from Python answered as COMMANDS.md says; the hashes "
          f"hashlib's")


def check_live(tmp, exe):
    """The live view's messages the C built, read by live_view.py and built again the same; the
    C's packed quaternions unpacked here within the packing's step."""
    import live_view
    path = os.path.join(tmp, "live.txt")
    subprocess.check_call([exe, path])
    messages, quats = [], 0
    for line in open(path):
        what, *rest = line.split()
        if what == "msg":
            data = bytes.fromhex(rest[0])
            msg = live_view.parse(data)
            again = live_view.build(msg["session"], msg["period"], msg["state"], msg["s0"],
                                    msg["povodoks"], msg["povodok_states"],
                                    [(b["n"], b["s0"], b["by"], b["state"], b["steps"])
                                     for b in msg["belkas"]])
            assert again == data, "the message built here differs from the C's"
            messages.append(msg)
        else:
            v = int(rest[0], 16)
            q = live_view.unpack_quat(v)
            want = [float(x) for x in rest[1:5]]
            assert all(abs(abs(a) - abs(b)) < 0.003 for a, b in zip(q, want)), (q, want)
            assert live_view.pack_quat(tuple(want)) == v, "packed here differently"
            quats += 1
    assert len(messages) == 4 and quats == 5
    assert [b["n"] for b in messages[0]["belkas"]] == [1, 2, 3] and \
        messages[0]["belkas"][1]["by"] == 2 and messages[0]["live"] == 5 and \
        len(messages[0]["belkas"][0]["steps"]) == 2
    assert len(messages[1]["belkas"]) == 47 and len(messages[2]["belkas"]) == 1 and \
        messages[1]["period"] == messages[2]["period"]
    assert messages[3]["belkas"][0]["steps"] == [] and messages[3]["belkas"][0]["item"] == 1
    print("ok the live view's messages, Python's byte for byte the C's; the quaternions unpacked")


def main():
    tmp = tempfile.mkdtemp()
    zcbor = zcbor_objects(tmp)
    for name in ("test_record", "test_steps", "test_frame", "test_frag", "test_loop",
                 "test_belka_link", "test_povodok_link", "test_world", "test_smp", "test_live",
                 "test_orient"):
        exe = build(tmp, name, zcbor)
        if name == "test_record":
            check_record(tmp, exe)
        elif name == "test_frag":
            check_frag(tmp, exe)
        elif name == "test_smp":
            check_smp(tmp, exe)
        elif name == "test_live":
            check_live(tmp, exe)
        else:
            subprocess.check_call([exe])
    print("FIRMWARE OK")


if __name__ == "__main__":
    main()
