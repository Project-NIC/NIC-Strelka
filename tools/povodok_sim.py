"""Černuška: a Povodok and its set simulated on a computer, for the application to be written against
before a board exists: SMP over TCP as COMMANDS.md and PHONE.md have it — port 1337 Povodok's own,
1337 + n the Belka numbered n, every connection beginning with the lock; Strelka's group 64 whole,
the system group's echo, parameters and information, the images' state, and on a Belka's ports
the files' group, giving a record built by strelka_record.py, so that a download is checked to
the byte; and the live view's messages a period over UDP (live_view.py), a made-up motion in them,
to the address the application's connection came from. What is simulated here is the talk, not
the radio: the Belkas answer at once, their records are made up, the time is the computer's.

    python3 tools/povodok_sim.py                  # 127.0.0.1, ports 1337 and 1338–1341, four Belkas
    python3 tools/povodok_sim.py --host 0.0.0.0 --belkas 8 --password strelka1

The password is strelka1 unless given; the factory one of the firmware is the same.
"""

import argparse
import hashlib
import hmac
import math
import os
import socket
import socketserver
import struct
import sys
import threading
import time

import cbor2
from cryptography.hazmat.primitives import serialization
from cryptography.hazmat.primitives.asymmetric.x25519 import X25519PrivateKey, X25519PublicKey

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import live_view  # noqa: E402
import smp  # noqa: E402
import strelka_cal  # noqa: E402
import strelka_images  # noqa: E402
import strelka_live as sl  # noqa: E402
import strelka_record as rec  # noqa: E402
import test_cal  # noqa: E402

FW = (0, 8, 0)
BUILD = b"\x00\x00\x00\x00"
PERIOD_S = 0.02
LIVE_QUIET_S = 3                   # the live view stops after the phone is this long silent
FS_ERR_FILE_NOT_FOUND = 3          # Zephyr's fs_mgmt.h
FS_ERR_OFFSET_LARGER = 12
DL_CHUNK = 1024                    # bytes a download answer carries, under a frame of 2 048
FETCH_FRAME_S = 0.005              # the simulator's pace in a download, a frame each 5 ms
ACC_RANGES = (2, 4, 8, 16, 32, 64, 128, 256, 320)   # 64/1's: every range a sensor of the set has
GYRO_RANGES = (125, 250, 500, 1000, 2000, 4000)    # whole degrees a second
BELKA_STATE = {"recording": 0, "alone": 1, "searching": 2, "paused": 3, "waiting": 4,
               "calibrating": 5, "giving": 6, "firmware": 7, "erasing": 8}


class Images:
    """A unit's two image slots as MCUmgr's image group shows them (Zephyr's img_mgmt): the image
    running, its version; one uploaded into the second slot by 1/1, piece by piece from its offset;
    marked for a test by 1/0 with its hash; taken at the next reset, 0/5 (FIRMWARE.md, The
    firmware's own updates). The firmware confirms the test once it has started whole."""

    def __init__(self):
        self.fw = FW
        self.slot = bytearray()
        self.length = 0
        self.sha = None
        self.pending = False

    def whole(self):
        return self.length and len(self.slot) == self.length

    def state(self):
        out = [dict(image=0, slot=0, version="%d.%d.%d" % self.fw, hash=hashlib.sha256(
            ("running %d.%d.%d" % self.fw).encode()).digest(), bootable=True, pending=False,
            confirmed=True, active=True, permanent=False)]
        if self.whole():
            out.append(dict(image=0, slot=1, version=strelka_images.version_of(bytes(self.slot)),
                            hash=hashlib.sha256(self.slot).digest(), bootable=True,
                            pending=self.pending, confirmed=False, active=False, permanent=False))
        return {"images": out, "splitStatus": 0}

    def command(self, read, cmd, body):
        if cmd == 0 and read:
            return self.state()
        if cmd == 0 and not read:                    # a test of the second slot, by its hash
            if not self.whole() or body.get("hash") != hashlib.sha256(self.slot).digest():
                raise smp.SmpError(smp.NOENT)
            self.pending = True
            return self.state()
        if cmd == 1 and not read:                    # an upload's piece
            off, data = body.get("off", 0), bytes(body.get("data", b""))
            if off == 0:
                if "len" not in body:
                    raise smp.SmpError(smp.INVAL)
                self.slot, self.length, self.sha = bytearray(), body["len"], body.get("sha")
                self.pending = False
            if off != len(self.slot):
                return {"off": len(self.slot)}       # where to go on from
            self.slot += data
            if len(self.slot) > self.length:
                raise smp.SmpError(smp.INVAL)
            out = {"off": len(self.slot)}
            if self.whole() and self.sha is not None:
                out["match"] = hashlib.sha256(self.slot).digest() == self.sha
            return out
        raise smp.SmpError(smp.NOTSUP)

    def reset(self):
        """0/5: a test pending, the second slot's image runs, the firmware confirming it."""
        if self.pending and self.whole():
            self.fw = tuple(int(x) for x in strelka_images.version_of(bytes(self.slot)).split("."))
            self.slot, self.length, self.pending = bytearray(), 0, False


def chosen(cfg):
    """The session's units from 64/1, each unit's number and kind; None, every unit chosen."""
    if "units" not in cfg:
        return None
    return rec.units_of(cfg["units"], cfg.get("kinds", bytes(16)))


class Belka:
    def __init__(self, n, chip):
        self.n, self.chip = n, chip
        self.state, self.fault = BELKA_STATE["waiting"], 0
        self.cell_pct, self.cell_mv, self.temp_centi = 80 + n % 15, 3250 + 3 * n, 2300 + 10 * n
        self.card_mb, self.card_free_mb = 30436, 30436 - 12 * n
        self.cal = dict(acc=[0] * 6, gscale=[32768] * 3, gzero=[0] * 3, range=[0, 0], temp=0, t=0,
                        hg=[0] * 6, hgrange=0)
        self.calibrating = None         # 64/22 under way: face 0's ask, the faces taken, the last
        self.files = {}                 # name → (session dict, bytes)
        self.last_live = time.monotonic()
        self.reachable = True
        self.table = b""
        self.blink_until = 0.0
        self.sensor = 0x44              # the ICM-42686-P: by the firmware's rule a Zvjozdočka
        self.rc = None                  # its answer to the last 64/1 Povodok had, 64/14's rc
        self.images = Images()          # its image slots, MCUmgr's group 1

    @property
    def kind(self):
        """64/0's `kind` as the firmware tells it: by the ICM a Zvjozdočka, else a Belka."""
        return 2 if self.sensor in (0x44, 0x47) else 0

    def record(self, session, cfg, first_period, steps=130, dropped=0, why=1):
        """A session's file as a Belka would write it: the head of version 4 with its groups, the
        table, the data and information blocks, with 64/1's `hg` on the hoof's sensor its
        high-g blocks, the end — strelka_record.py's composer."""
        name = "S%05dB%02d.STRK" % (session, self.n)
        who, hg = self.sensor, cfg.get("hg", 0) if self.sensor == 0x73 else 0
        c = self.cal
        groups = rec.imu_groups(cfg["acc"], sl.acc_step_ng(who, cfg["acc"]), cfg["gyro"],
                                sl.gyro_step_ndps(who, cfg["gyro"]), acc_cal=c["acc"],
                                gyro_zero=c["gzero"], gyro_scale=c["gscale"],
                                cal_range=tuple(c["range"][:2]), high_g=who == 0x73 and cfg["acc"] > 16,
                                hg=dict(range=hg, step_ng=sl.acc_step_ng(who, hg),
                                        cal=c.get("hg", [0] * 6), cal_range=c.get("hgrange", 0))
                                if hg else None)
        head = rec.head_block(session, self.n, self.chip, fw=self.images.fw, rate_hz=cfg["rate"] // cfg["n"],
                              n=cfg["n"], time_us=cfg.get("t", 0), time_period=first_period,
                              tz_min=cfg.get("tz", 0), sensor=sl.SENSORS.get(who, ""),
                              cal_temp=c["temp"], cal_time=c["t"], groups=groups)
        blocks = rec.compose(
            head, self.table.decode("utf-8", "replace") if self.table else None, first_period,
            steps, lambda k: ((k * 7 + self.n) % 300, -((k * 5) % 300), 16384, 0, 100 * (k % 3), 0),
            lambda k: (k % 5 - 2, -60 - k % 7, 0, 2),
            hg_of=(lambda k: ((k * 3) % 300, 0, 96)) if hg else None,
            temp_centi=self.temp_centi, cell_mv=self.cell_mv, why=why, dropped=dropped)
        number = rec.parse_end(blocks[-1])["blocks"]
        data = b"".join(blocks)
        self.files[name] = (dict(s=session, file=name, steps=steps, blocks=number, p0=first_period,
                                 end=why, bytes=len(data)), data)
        return name


class Povodok:
    """The set's commander as the application sees it over one address."""

    def __init__(self, belkas=4, password=b"strelka1", number=0):
        self.number = number
        self.chip = 0xF0F0F0F0F0F00000 + number
        self.password_hash = self.factory_hash = hashlib.sha256(password).digest()
        self.identity = X25519PrivateKey.generate()   # its own long-term key, the application pins
        self.belkas = {n: Belka(n, 0x1000 + 0x101 * n) for n in range(1, belkas + 1)}
        self.lock = threading.Lock()
        self.record_on, self.live, self.searching = False, 0, False
        self.face_s = 5.0                # a face's samples in 64/22, shorter for the tests
        self.fetching = False           # 64/21's download running
        self.streams = []               # the connections on the download's port
        self.stream_lock = threading.Lock()
        self.service = True             # the service mode until 64/20 (FIRMWARE.md)
        self.label = 0                  # the set's own number, 0 none
        self.channel, self.spare = 7, 23
        self.session = dict(s=1, rate=1000, n=10, acc=16, gyro=2000, live=0, stop=10, t=0, tz=0)
        self.table = bytearray()
        self.table_len = 0
        self.found = {}                 # chip → (rssi, found at)
        self.images = Images()          # Povodok's own image slots
        self.new_belkas = [0xABCDEF0000000001, 0xABCDEF0000000002]   # heard when searching
        self.search_started = 0.0
        self.period0 = time.monotonic()
        self.record_started = None
        self.on_set_changed = lambda: None
        self.phone = None               # (address, port) the live view goes to: the last
                                        # connection on Povodok's port that passed the lock
        self.phone_seen = 0.0           # when a frame last came from it: the live view goes
                                        # while it is under LIVE_QUIET_S (COMMANDS.md, 64/14)

    def period(self):
        return int((time.monotonic() - self.period0) / PERIOD_S)

    # ---- the application's commands, group 64, to Povodok itself ----
    def info(self):
        return dict(chip=self.chip.to_bytes(8, "little"), n=self.number, who=0,
                    fw=list(self.images.fw),
                    build=BUILD, sensor=0, card=dict({"in": False}, mb=0, free=0, fs=0),
                    cell=dict(pct=0, mv=0), temp=0, state=0 if self.record_on else 4, fault=0,
                    cal=0, **({"label": self.label} if self.label else {}))

    def set_session(self, body):
        if self.record_on:
            raise smp.SmpError(smp.BAD_STATE)       # not while the set records
        for key in ("s", "rate", "n", "acc", "gyro", "live"):
            if key not in body:
                raise smp.SmpError(smp.INVAL)
        if body["acc"] not in ACC_RANGES or body["gyro"] not in GYRO_RANGES or \
                body["live"] not in (0, 1, 2, 4, 5) or not 1 <= body["n"] <= 10 or \
                (body["rate"] >= 4000 and body["n"] != 1) or body["rate"] < 1 or \
                body.get("stop", 10) < 1 or body.get("off", 600) < 1 or \
                ("kinds" in body and "units" not in body) or \
                len(body.get("units", bytes(8))) != 8 or len(body.get("kinds", bytes(16))) != 16:
            raise smp.SmpError(smp.INVAL)
        self.session = dict(body)
        for b in self.belkas.values():
            b.rc = 0 if b.reachable else None     # passed to each Belka, its answer kept
        self.session.setdefault("stop", 10)
        self.session.setdefault("off", 600)
        self.live = body["live"]
        return {}

    def aside(self, n):
        """A Belka not among the session's units: it neither records nor sends the live view."""
        units = chosen(self.session)
        return units is not None and n not in units

    def record(self, run):
        if run and not self.record_on:
            self.record_on = True
            self.record_started = self.period() + 1
            for b in self.belkas.values():
                if not self.aside(b.n):
                    b.state = BELKA_STATE["recording"]
        elif not run and self.record_on:
            self.record_on = False
            for b in self.belkas.values():
                if b.state == BELKA_STATE["recording"]:
                    b.state = BELKA_STATE["waiting"]
                    b.record(self.session["s"], self.session, self.record_started)
            self.session["s"] = (self.session["s"] + 1) & 0xFFFF
        return {}

    def search(self, run):
        if run and not self.searching:
            self.search_started = time.monotonic()
        self.searching = run
        return {}

    def found_list(self):
        if self.searching:
            for i, chip in enumerate(self.new_belkas):
                if chip not in self.found and all(b.chip != chip for b in self.belkas.values()):
                    self.found[chip] = (-60 - 7 * i, time.monotonic())
        now = time.monotonic()
        return {"found": [dict(chip=c.to_bytes(8, "little"), rssi=r, ago=int(now - at))
                          for c, (r, at) in self.found.items()]}

    def take(self, chip, n):
        if n < 1 or n > 63 or chip is None:
            raise smp.SmpError(smp.INVAL)
        if n in self.belkas and self.belkas[n].chip != chip:
            raise smp.SmpError(smp.INVAL)              # a number is one unit's, of any kind
        if chip not in self.found:
            raise smp.SmpError(smp.NOENT)
        self.belkas[n] = Belka(n, chip)
        if self.record_on and not self.aside(n):
            self.belkas[n].state = BELKA_STATE["recording"]
        del self.found[chip]
        self.on_set_changed()
        return {}

    def set_view(self):
        now = time.monotonic()
        return {"belkas": [dict(n=b.n, chip=b.chip.to_bytes(8, "little"),
                                # a Belka in reach is heard once a second at the least
                                ago=int((now - b.last_live) / PERIOD_S) % 50 if b.reachable else 500,
                                by=2 if b.n % 3 == 0 else 0, state=b.state, fault=b.fault,
                                **({} if b.rc is None else {"rc": b.rc}))
                           for b in sorted(self.belkas.values(), key=lambda b: b.n)],
                "povodoks": 1 << self.number, "ch": self.channel, "spare": self.spare}

    def diag(self, start):
        """64/23: what Povodok knows of the Belkas and of the band, 8 Belkas a page; made up, the
        noise of the band this Povodok's own alone, the Belkas' on the set's channel."""
        now, lines = time.monotonic(), []
        for b in sorted(self.belkas.values(), key=lambda b: b.n):
            if not b.reachable:
                lines.append(dict(n=b.n, ago=500))
                continue
            lines.append(dict(n=b.n, rssi=-55 - b.n, ago=int((now - b.last_live) / PERIOD_S) % 50,
                              cell=b.cell_pct, temp=b.temp_centi // 100, mv=b.cell_mv,
                              free=100 * b.card_free_mb // b.card_mb, dropped=0, sync=-57 - b.n,
                              missed=b.n % 2, offset=12 * b.n, out=[0, 0], ch=self.channel,
                              noise=-94))
        answer = {"belkas": lines[start:start + 8], "more": start + 8 < len(lines)}
        if start == 0:
            own = [-97 + (c * 7) % 9 if c != 11 else -71 for c in range(40)]   # Wi-Fi on 11
            answer.update(ch=self.channel, spare=self.spare,
                          margin=[24 if k == self.number else 31 for k in range(4)],
                          noise=[own if k == self.number else [-128] * 40 for k in range(4)],
                          bnoise=[-94 if c == self.channel else -128 for c in range(40)])
        return answer

    def drop(self, n):
        if n not in self.belkas:
            raise smp.SmpError(smp.NOENT)
        del self.belkas[n]
        self.on_set_changed()
        return {}

    def start_fetch(self, body):
        """64/21: the session from the Belkas given, every one taken when none is, each from its
        byte in `from`; going round them as Povodok does, a frame of the file's download from the
        one furthest behind, every answer out on the download's port (COMMANDS.md)."""
        s = body["s"]
        belkas = list(body.get("belkas", sorted(self.belkas)))
        start = list(body.get("from", [0] * len(belkas)))
        if len(start) != len(belkas) or any(not 1 <= n <= 63 for n in belkas):
            raise smp.SmpError(smp.INVAL)
        if self.record_on or self.live:
            raise smp.SmpError(smp.BAD_STATE)          # ESB only while nothing is recorded
        if self.fetching:
            raise smp.SmpError(smp.BUSY)
        self.fetching = True
        threading.Thread(target=self.fetch_loop, args=(s, belkas, start), daemon=True).start()
        return {}

    def fetch_loop(self, s, belkas, start):
        off = dict(zip(belkas, start))
        done = {}
        seq = 0
        while len(done) < len(belkas):
            with self.stream_lock:
                if not self.streams:
                    break                              # the phone gone: the download let go
            n = min((b for b in belkas if b not in done), key=lambda b: off[b])
            b = self.belkas.get(n)
            name = "S%05dB%02d.STRK" % (s, n)
            if b is None or not b.reachable or name not in b.files:
                done[n] = True
                self.stream_out(n, b"")                # done without her file whole
                continue
            data = b.files[name][1]
            body = {"off": off[n], "data": data[off[n]:off[n] + DL_CHUNK]}
            if off[n] == 0:
                body["len"] = len(data)
            frame = smp.frame(smp.OP_READ_RSP, smp.GROUP_FS, seq & 0xFF, 0, body)
            seq += 1
            off[n] += len(body["data"])
            self.stream_out(n, frame)
            if off[n] >= len(data):
                done[n] = True
            time.sleep(FETCH_FRAME_S)
        self.stream_out(0, b"")                        # the download over
        self.fetching = False

    def stream_out(self, n, frame):
        record = struct.pack("<BH", n, len(frame)) + frame
        with self.stream_lock:
            for c in list(self.streams):
                try:
                    c.sendall(record)
                except OSError:
                    self.streams.remove(c)

    def table_piece(self, body):
        off, length, csv = body["off"], body["len"], body["csv"].encode("utf-8")
        if len(csv) > 1900 or off + len(csv) > length:
            raise smp.SmpError(smp.INVAL)
        if off == 0:
            self.table = bytearray(length)
            self.table_len = length
        self.table[off:off + len(csv)] = csv
        if off + len(csv) == length:
            for b in self.belkas.values():
                b.table = bytes(self.table)
        return {}


    def live_messages(self, period):
        """This period's live view: every Belka in the set with a made-up step or two, as the
        live view's code says — a slow turn as quaternions, a trot as raw steps."""
        code = self.live
        if code == 0 and period % 50 != 0:
            return []
        t = period / 50.0
        belkas = []
        for b in sorted(self.belkas.values(), key=lambda b: b.n):
            if not b.reachable or (self.aside(b.n) and period % 50 != 0):
                continue                 # a Belka aside: once a second, as at rest
            steps = []
            for k in range(live_view.STEPS[code]):
                tt = t + k / (100.0 if code in (2, 5) else 50.0)
                if live_view.STEP_BYTES[code] == 4:
                    a = 0.3 * math.sin(2 * math.pi * 0.5 * tt + b.n)
                    steps.append((math.cos(a / 2), math.sin(a / 2), 0.0, 0.0))
                else:
                    g = 32768 // self.session["acc"]      # a g in steps at the range
                    steps.append((int(0.2 * g * math.sin(2 * math.pi * 2.5 * tt + b.n)), 0, g,
                                  int(3000 * math.sin(2 * math.pi * 2.5 * tt)), 0, 0))
            item = 8 if period % 3000 >= 7 else 1 + period % 3000
            state = (b.n, -90 + b.n % 7) if item == 8 else (b.cell_pct, 20)
            belkas.append((b.n, (code << 4) | item, 2 if b.n % 3 == 0 else 0,
                           (state[0] << 8) | (state[1] & 0xFF), steps))
        set_state = (0x8000 if self.record_on else 0) | (20 << 8) | self.spare
        return live_view.split(self.session["s"], period, set_state, (code << 4) | 8, 1,
                               (set_state, 0, 0, 0), belkas)


def as_bool(v):
    """A CBOR boolean, as the C takes it; anything else is a bad value."""
    if not isinstance(v, bool):
        raise smp.SmpError(smp.INVAL)
    return v


class Connection(socketserver.BaseRequestHandler):
    """One TCP connection: the stream cut into frames, the lock, the answers."""

    def handle(self):
        stream = smp.Stream()
        self.open, self.nonce, self.fresh = False, None, False
        self.request.settimeout(600)
        try:
            while True:
                data = self.request.recv(4096)
                if not data:
                    return
                for fr in stream.feed(data):
                    answer = self.answer(fr)
                    if self.open and self.server.belka_n is None:
                        self.server.sim.phone_seen = time.monotonic()
                    if answer:
                        self.request.sendall(answer)
        except (ConnectionError, socket.timeout, ValueError):
            return

    def answer(self, fr):
        op, version, _, length, group, seq, cmd = smp.parse_header(fr)
        if op not in (smp.OP_READ, smp.OP_WRITE):
            return b""
        rsp_op = smp.OP_READ_RSP if op == smp.OP_READ else smp.OP_WRITE_RSP
        if version > smp.VERSION:                       # 0 and 1 both taken, as the units do
            return smp.frame(rsp_op, group, seq, cmd, {"rc": smp.TOO_NEW}, version)
        try:
            body = cbor2.loads(fr[8:8 + length]) if length else {}
            if not isinstance(body, dict):
                raise smp.SmpError(smp.INVAL)
        except (ValueError, cbor2.CBORDecodeError):
            return smp.frame(rsp_op, group, seq, cmd, {"rc": smp.INVAL})
        try:
            if not (group == smp.GROUP_STRELKA and cmd == smp.AUTH):
                # 64/5 and 64/7 only as the next frame after the lock written again, which every
                # other frame spends (COMMANDS.md, The lock)
                fresh, self.fresh = self.fresh, False
                if not self.open or (group == smp.GROUP_STRELKA and
                                     cmd in (smp.CARD, smp.FORGET) and not fresh):
                    raise smp.SmpError(smp.ACCESS)
            result = self.dispatch(op, group, cmd, body)
        except smp.SmpError as e:
            result = {"rc": e.rc} if e.group is None else {"err": {"group": e.group, "rc": e.rc}}
        except (KeyError, TypeError, ValueError):
            result = {"rc": smp.INVAL}
        return smp.frame(rsp_op, group, seq, cmd, result)

    def dispatch(self, op, group, cmd, body):
        sim, belka = self.server.sim, self.server.belka_n
        read = op == smp.OP_READ
        if group == smp.GROUP_STRELKA and cmd == smp.AUTH:
            if read:
                self.nonce = os.urandom(16)
                self.key = sim.identity              # Povodok's own key, on every port
                pub = self.key.public_key().public_bytes(serialization.Encoding.Raw,
                                                         serialization.PublicFormat.Raw)
                return {"nonce": self.nonce, "pub": pub}
            self.fresh = False
            self.check_mac(body)
            self.open = self.fresh = True
            if belka is None:
                sim.phone = (self.client_address[0], self.server.server_address[1])
            return {}
        if not sim.service and group == smp.GROUP_STRELKA and (
                (belka is None and cmd in (smp.FORGET, smp.SEARCH, smp.TAKE, smp.DROP,
                                           smp.PASSWORD, smp.GIVE) and op == smp.OP_WRITE) or
                (belka is not None and cmd in (smp.CARD, smp.FORGET, smp.KEY, smp.JOIN))):
            raise smp.SmpError(smp.BAD_STATE)           # a setting: the service mode's alone
        if belka is not None:
            b = sim.belkas.get(belka)
            if b is None or not b.reachable:
                raise smp.SmpError(smp.STRELKA_UNREACHABLE, smp.GROUP_STRELKA)
            with sim.lock:
                return self.belka_command(b, op, group, cmd, body)
        with sim.lock:
            return self.povodok_command(op, group, cmd, body)

    def check_mac(self, body):
        """The lock's MAC over the nonce and the X25519 secret of this connection."""
        nonce, self.nonce = self.nonce, None
        key, self.key = getattr(self, "key", None), None
        mac, pub = body.get("mac"), body.get("pub")
        if not isinstance(mac, bytes) or len(mac) != 32 or not isinstance(pub, bytes) or \
                len(pub) != 32:
            raise smp.SmpError(smp.INVAL)
        if nonce is None or key is None:
            raise smp.SmpError(smp.ACCESS)
        secret = hashlib.sha256(key.exchange(X25519PublicKey.from_public_bytes(pub))).digest()
        want = hmac.new(self.server.sim.password_hash, nonce + secret, "sha256").digest()
        if not hmac.compare_digest(want, mac):
            raise smp.SmpError(smp.ACCESS)
        return secret

    def os_command(self, read, cmd, body, who, images=None):
        if cmd == 0 and not read:
            return {"r": body["d"]}
        if cmd == 5 and not read:
            if images is not None:
                images.reset()                  # started again: a test pending taken
            return {}
        if cmd == 6 and read:
            return {"buf_size": 2048, "buf_count": 4}
        if cmd == 7 and read:
            return {"output": "Zephyr 4.4.0 nRF Connect SDK v3.4.0 NIC-Strelka %s simulated" % who}
        if cmd == 8 and read:
            return {"bootloader": "MCUboot"} if "query" not in body else {"mode": 1}
        raise smp.SmpError(smp.NOTSUP)


    def povodok_command(self, op, group, cmd, body):
        sim, read = self.server.sim, op == smp.OP_READ
        if group == smp.GROUP_OS:
            return self.os_command(read, cmd, body, "Povodok", sim.images)
        if group == smp.GROUP_IMG:
            return sim.images.command(read, cmd, body)
        if group != smp.GROUP_STRELKA:
            raise smp.SmpError(smp.NOTSUP)
        if cmd == smp.INFO and read:
            return sim.info()
        if cmd == smp.SESSION and not read:
            return sim.set_session(body)
        if cmd == smp.BLINK and not read:
            return {}
        if cmd == smp.OFF and not read:
            sim.record(False)
            return {}
        if cmd == smp.FORGET and not read:
            sim.belkas.clear()
            sim.password_hash = sim.factory_hash        # the factory settings
            sim.identity = X25519PrivateKey.generate()  # and a key of its own anew
            sim.on_set_changed()
            return {}
        if cmd == smp.GIVE and not read:
            from cryptography.hazmat.primitives.ciphers.aead import AESCCM
            secret = self.check_mac(body)
            n, addr, ch = body["n"], body["addr"], body["ch"]
            if not 0 <= n <= 3 or not isinstance(addr, bytes) or len(addr) != 4 or \
                    not 0 <= ch <= 38:
                raise smp.SmpError(smp.INVAL)
            try:
                key = AESCCM(secret[:16], tag_length=4).decrypt(body["nonce"],
                                                                body["key"] + body["mic"], None)
            except Exception:
                raise smp.SmpError(smp.ACCESS)
            label = body.get("label", 0)
            if not 0 <= label <= 9999:
                raise smp.SmpError(smp.INVAL)
            sim.given = dict(n=n, address=int.from_bytes(addr, "little"), channel=ch, key=key)
            sim.label = label
            return {}
        if cmd == smp.WORK and not read:
            sim.service = False                         # Bluetooth LE off, the settings refused
            return {}
        if cmd == smp.RECORD and not read:
            if sim.service and as_bool(body["run"]):
                raise smp.SmpError(smp.BAD_STATE)       # the record is the work's: 64/20 first
            return sim.record(as_bool(body["run"]))
        if cmd == smp.LIVE and not read:
            if body["live"] not in (0, 1, 2, 4, 5):
                raise smp.SmpError(smp.INVAL)
            if sim.service and body["live"]:
                raise smp.SmpError(smp.BAD_STATE)       # the live view is the work's
            sim.live = body["live"]
            return {}
        if cmd == smp.SEARCH:
            if not read and sim.record_on and as_bool(body["run"]):
                raise smp.SmpError(smp.BAD_STATE)    # searching, the first sends no SYNCs
            return sim.found_list() if read else sim.search(as_bool(body["run"]))
        if cmd == smp.TAKE and not read:
            chip = body["chip"]
            if not isinstance(chip, bytes) or len(chip) != 8:
                raise smp.SmpError(smp.INVAL)
            return sim.take(int.from_bytes(chip, "little"), body["n"])
        if cmd == smp.SET and read:
            return sim.set_view()
        if cmd == smp.DIAG and read:
            return sim.diag(body.get("from", 0))
        if cmd == smp.MOVE and not read:
            ch = body.get("ch")
            if not isinstance(ch, int) or not 0 <= ch < 39 or ch == sim.channel:
                raise smp.SmpError(smp.INVAL)
            if sim.record_on:
                raise smp.SmpError(smp.BAD_STATE)       # at rest alone
            sim.spare, sim.channel = sim.channel, ch
            return {}
        if cmd == smp.DROP and not read:
            return sim.drop(body["n"])
        if cmd == smp.PASSWORD and not read:
            self.check_mac(body)
            new = body["new"]
            if not isinstance(new, bytes) or len(new) != 32:
                raise smp.SmpError(smp.INVAL)
            sim.password_hash = new
            return {}
        if cmd == smp.TABLE and not read:
            return sim.table_piece(body)
        if cmd == smp.FETCH and not read:
            return sim.start_fetch(body)
        raise smp.SmpError(smp.NOTSUP)

    def belka_command(self, b, op, group, cmd, body):
        read = op == smp.OP_READ
        if group == smp.GROUP_OS:
            return self.os_command(read, cmd, body, "Belka %d" % b.n, b.images)
        if group == smp.GROUP_IMG:
            return b.images.command(read, cmd, body)
        if group == smp.GROUP_FS:
            return self.fs_command(b, read, cmd, body)
        if group != smp.GROUP_STRELKA:
            raise smp.SmpError(smp.NOTSUP)
        if cmd == smp.INFO and read:
            return dict(chip=b.chip.to_bytes(8, "little"), n=b.n, who=2, kind=b.kind,
                        fw=list(b.images.fw),
                        build=BUILD, sensor=b.sensor,
                        card=dict({"in": True}, mb=b.card_mb, free=b.card_free_mb, fs=2),
                        cell=dict(pct=b.cell_pct, mv=b.cell_mv), temp=b.temp_centi,
                        state=b.state, fault=b.fault, cal=b.cal["t"])
        if cmd == smp.SESSIONS and read:
            entries = [info for info, _ in sorted(b.files.values(), key=lambda f: f[0]["s"])]
            start = body.get("from", 0)
            page = entries[start:start + 16]
            return {"list": page, "more": start + 16 < len(entries)}
        if cmd == smp.CAL:
            if read:
                c = dict(b.cal)
                if not any(c.get("hg", ())) and not c.get("hgrange"):
                    c.pop("hg", None)            # no high-g channel kept: 64/3 as before
                    c.pop("hgrange", None)
                return c
            for key, n in (("acc", 6), ("gscale", 3), ("gzero", 3)):
                if len(body[key]) != n:
                    raise smp.SmpError(smp.INVAL)
            b.cal = dict(acc=list(body["acc"]), gscale=list(body["gscale"]),
                         gzero=list(body["gzero"]), range=list(body.get("range", [0, 0])),
                         temp=body["temp"], t=body["t"], hg=list(body.get("hg", [0] * 6)),
                         hgrange=body.get("hgrange", 0))
            return {}
        if cmd == smp.CALIBRATE:
            return self.calibrate(b, read, body)
        if cmd == smp.BLINK and not read:
            b.blink_until = time.monotonic() + body["s"]
            return {}
        if cmd == smp.CARD and not read:
            if b.state == BELKA_STATE["recording"]:
                raise smp.SmpError(smp.BUSY)
            if body["do"] not in (1, 2, 3):
                raise smp.SmpError(smp.INVAL)
            if body["do"] == 3:
                raise smp.SmpError(smp.BAD_STATE)        # no simulated Belka is on its cable
            b.files.clear()
            return {}
        if cmd == smp.OFF and not read:
            b.reachable = False
            return {}
        if cmd == smp.FORGET and not read:
            del self.server.sim.belkas[b.n]
            self.server.sim.on_set_changed()
            return {}
        if cmd == smp.TABLE and not read:
            return self.server.sim.table_piece(body)
        raise smp.SmpError(smp.NOTSUP)

    def calibrate(self, b, read, body):
        """64/22 as a Belka answers it, the unit lying as tools/test_cal.py's, every face taken
        once its time is over; after the sixth the calibration strelka_cal.py computes, kept."""
        run, now = b.calibrating, time.monotonic()
        if not read:
            if b.state == BELKA_STATE["recording"]:
                raise smp.SmpError(smp.BAD_STATE)
            face = body.get("face")
            if face == 0:
                if not all(k in body for k in ("acc", "gyro", "t")):
                    raise smp.SmpError(smp.INVAL)
                b.calibrating = dict(q0=body, taken=0, face=0, since=now, got=3)
                return {}
            if run is None or face != run["taken"] or run["got"] == 3:
                raise smp.SmpError(smp.INVAL)
            run.update(face=face, since=now, got=3)
            return {}
        if run is None:
            raise smp.SmpError(smp.BAD_STATE)
        if run["got"] == 3 and now - run["since"] >= self.server.sim.face_s:
            run["got"], run["taken"] = 0, run["taken"] + 1
            if run["taken"] == 6:
                faces, turns = test_cal.unit()
                got = strelka_cal.calibrate(faces, turns, run["q0"]["acc"], run["q0"]["gyro"],
                                            test_cal.RATE)
                b.cal = dict(got, range=[run["q0"]["acc"], run["q0"]["gyro"]], temp=b.temp_centi,
                             t=run["q0"]["t"])
        answer = dict(face=run["face"], got=run["got"])
        if run["taken"] == 6 and run["face"] == 5:
            answer.update(b.cal)
        return answer

    def fs_command(self, b, read, cmd, body):
        """Zephyr's files group as fs_mgmt answers it: 8/0 the download by offsets, 8/1 the
        length, 8/2 the SHA-256; its own codes in err with group 8."""
        name = body.get("name", "")
        if name.startswith("/SD:/"):
            name = name[5:]
        if name not in b.files:
            raise smp.SmpError(FS_ERR_FILE_NOT_FOUND, smp.GROUP_FS)
        data = b.files[name][1]
        if cmd == 0 and read:
            off = body.get("off", 0)
            if off > len(data):
                raise smp.SmpError(FS_ERR_OFFSET_LARGER, smp.GROUP_FS)
            rsp = {"off": off, "data": data[off:off + DL_CHUNK]}
            if off == 0:
                rsp["len"] = len(data)
            return rsp
        if cmd == 1 and read:
            return {"len": len(data)}
        if cmd == 2 and read:
            off, length = body.get("off", 0), body.get("len", len(data))
            piece = data[off:off + length]
            return {"type": "sha256", "off": off, "len": len(piece),
                    "output": hashlib.sha256(piece).digest()}
        raise smp.SmpError(smp.NOTSUP)


class StreamConnection(socketserver.BaseRequestHandler):
    """The download's port, 1337 + 64: a connection takes every frame 64/21 brings."""

    def handle(self):
        sim = self.server.sim
        with sim.stream_lock:
            sim.streams.append(self.request)
        try:
            while self.request.recv(64):
                pass
        except OSError:
            pass
        with sim.stream_lock:
            if self.request in sim.streams:
                sim.streams.remove(self.request)


class StreamServer(socketserver.ThreadingTCPServer):
    allow_reuse_address = True
    daemon_threads = True

    def __init__(self, address, sim):
        self.sim = sim
        super().__init__(address, StreamConnection)


class Server(socketserver.ThreadingTCPServer):
    allow_reuse_address = True
    daemon_threads = True

    def __init__(self, address, sim, belka_n):
        self.sim, self.belka_n = sim, belka_n
        super().__init__(address, Connection)


class Simulator:
    """Povodok's port and one for each Belka in the set, opened and closed as the set changes;
    the live view's messages a period over UDP to the phone that passed the lock, to the port
    number Povodok's own TCP port has."""

    def __init__(self, host="127.0.0.1", base=smp.PORT, belkas=4, password=b"strelka1"):
        self.host, self.base = host, base
        self.sim = Povodok(belkas, password)
        self.servers = {}
        self.threads = {}
        self.sim.on_set_changed = self.sync_ports
        self.open_port(None)
        self.sync_ports()
        self.stream = StreamServer((host, base + smp.FETCH_PORT - smp.PORT), self.sim)
        threading.Thread(target=self.stream.serve_forever, daemon=True).start()
        self.running = True
        self.udp = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.live_thread = threading.Thread(target=self.live_loop, daemon=True)
        self.live_thread.start()

    def live_loop(self):
        last = -1
        while self.running:
            period = self.sim.period()
            if period != last:
                last = period
                phone = self.sim.phone
                if phone is not None and not self.sim.service and \
                        time.monotonic() - self.sim.phone_seen <= LIVE_QUIET_S:
                    with self.sim.lock:
                        messages = self.sim.live_messages(period)
                    for m in messages:
                        try:
                            self.udp.sendto(m, phone)
                        except OSError:
                            pass
            time.sleep(0.005)

    def open_port(self, belka_n):
        port = self.base + (belka_n or 0)
        server = Server((self.host, port), self.sim, belka_n)
        thread = threading.Thread(target=server.serve_forever, daemon=True)
        thread.start()
        self.servers[belka_n] = server
        self.threads[belka_n] = thread

    def sync_ports(self):
        wanted = set(self.sim.belkas)
        for n in list(self.servers):
            if n is not None and n not in wanted:
                self.servers.pop(n).shutdown()
        for n in wanted - set(self.servers):
            self.open_port(n)

    def close(self):
        self.running = False
        self.udp.close()
        self.stream.shutdown()
        self.stream.server_close()
        for server in list(self.servers.values()):
            server.shutdown()
            server.server_close()
        self.servers.clear()


def main():
    parser = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    parser.add_argument("--host", default="127.0.0.1")
    parser.add_argument("--port", type=int, default=smp.PORT, help="Povodok's; the Belkas' follow")
    parser.add_argument("--belkas", type=int, default=4)
    parser.add_argument("--password", default="strelka1")
    parser.add_argument("--face-s", type=float, default=5.0, help="a face's time in 64/22")
    args = parser.parse_args()
    sim = Simulator(args.host, args.port, args.belkas, args.password.encode("utf-8"))
    sim.sim.face_s = args.face_s
    print("Povodok on %s:%d, the Belkas 1–%d on %d–%d; the password %r" %
          (args.host, args.port, args.belkas, args.port + 1, args.port + args.belkas,
           args.password))
    try:
        while True:
            time.sleep(1)
    except KeyboardInterrupt:
        sim.close()


if __name__ == "__main__":
    main()
