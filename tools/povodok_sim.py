"""A Povodok and its set simulated on a computer, for the application to be written against
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
import strelka_record as rec  # noqa: E402

FW = (0, 7, 0)
BUILD = b"\x00\x00\x00\x00"
PERIOD_S = 0.02
FS_ERR_FILE_NOT_FOUND = 3          # Zephyr's fs_mgmt.h
FS_ERR_OFFSET_LARGER = 12
DL_CHUNK = 1024                    # bytes a download answer carries, under a frame of 2 048
BELKA_STATE = {"recording": 0, "alone": 1, "searching": 2, "paused": 3, "waiting": 4,
               "calibrating": 5, "giving": 6, "firmware": 7, "erasing": 8}


class Belka:
    def __init__(self, n, chip):
        self.n, self.chip = n, chip
        self.state, self.fault = BELKA_STATE["waiting"], 0
        self.cell_pct, self.cell_mv, self.temp_centi = 80 + n % 15, 3250 + 3 * n, 2300 + 10 * n
        self.card_mb, self.card_free_mb = 30436, 30436 - 12 * n
        self.cal = dict(acc=[0] * 6, gscale=[32768] * 3, gzero=[0] * 3, temp=0, t=0)
        self.files = {}                 # name → (session dict, bytes)
        self.last_live = time.monotonic()
        self.reachable = True
        self.table = b""
        self.blink_until = 0.0

    def record(self, session, cfg, first_period, steps=130, dropped=0, why=1):
        """A session's file as a Belka would write it: the head, the table, the data and
        information blocks, the end — strelka_record.py's builders."""
        name = "S%05dB%02d.STRK" % (session, self.n)
        spp = rec.steps_a_period(dict(rate_hz=cfg["rate"] // cfg["n"]))
        blocks = [rec.head_block(session, self.n, self.chip, fw=FW, build=BUILD,
                                 rate_hz=cfg["rate"] // cfg["n"], n=cfg["n"], acc_g=cfg["acc"],
                                 gyro_dps=cfg["gyro"], first_period=first_period,
                                 time_us=cfg.get("t", 0), time_period=first_period,
                                 tz_min=cfg.get("tz", 0), acc_cal=self.cal["acc"],
                                 gyro_scale=self.cal["gscale"], gyro_zero=self.cal["gzero"],
                                 cal_temp=self.cal["temp"], cal_time=self.cal["t"], name=name)]
        if self.table:
            blocks += rec.table_blocks(session, self.table.decode("utf-8", "replace"))
        number = 0
        for first in range(0, steps, rec.STEPS):
            count = min(rec.STEPS, steps - first)
            vals = [((k * 7 + self.n) % 300, -((k * 5) % 300), 16384, 0, 100 * (k % 3), 0)
                    for k in range(first, first + count)]
            blocks.append(rec.data_block(session, number, first_period + first // spp, vals,
                                         place=(first % spp) // rec.STEPS))
            number += 1
            if (first + count) % rec.INFO_STEPS == 0 or first + count == steps:
                start = (first + count - 1) // rec.INFO_STEPS * rec.INFO_STEPS
                entries = [(k % 5 - 2, -60 - k % 7, 0, 2) for k in range(start, first + count)]
                blocks.append(rec.info_block(session, number, first_period + start // spp,
                                             entries, temp_centi=self.temp_centi,
                                             cell_mv=self.cell_mv))
                number += 1
        blocks.append(rec.end_block(session, why, steps, number, first_period + (steps - 1) // spp,
                                    dropped=dropped))
        data = b"".join(blocks)
        self.files[name] = (dict(s=session, file=name, steps=steps, blocks=number, p0=first_period,
                                 end=why, bytes=len(data)), data)
        return name


class Povodok:
    """The set's commander as the application sees it over one address."""

    def __init__(self, belkas=4, password=b"strelka1", number=0):
        self.number = number
        self.chip = 0xF0F0F0F0F0F00000 + number
        self.password_hash = hashlib.sha256(password).digest()
        self.belkas = {n: Belka(n, 0x1000 + 0x101 * n) for n in range(1, belkas + 1)}
        self.lock = threading.Lock()
        self.record_on, self.live, self.searching = False, 0, False
        self.channel, self.spare = 7, 23
        self.session = dict(s=1, rate=1000, n=10, acc=16, gyro=2000, live=0, stop=10, t=0, tz=0)
        self.table = bytearray()
        self.table_len = 0
        self.found = {}                 # chip → (rssi, found at)
        self.new_belkas = [0xABCDEF0000000001, 0xABCDEF0000000002]   # heard when searching
        self.search_started = 0.0
        self.period0 = time.monotonic()
        self.record_started = None
        self.on_set_changed = lambda: None
        self.phone = None               # (address, port) the live view goes to: the last
                                        # connection on Povodok's port that passed the lock

    def period(self):
        return int((time.monotonic() - self.period0) / PERIOD_S)

    # ---- the application's commands, group 64, to Povodok itself ----
    def info(self):
        return dict(chip=self.chip.to_bytes(8, "little"), n=self.number, who=0, fw=list(FW),
                    build=BUILD, sensor=0, card=dict({"in": False}, mb=0, free=0, fs=0),
                    cell=dict(pct=0, mv=0), temp=0, state=0 if self.record_on else 4, fault=0,
                    cal=0)

    def set_session(self, body):
        for key in ("s", "rate", "n", "acc", "gyro", "live"):
            if key not in body:
                raise smp.SmpError(smp.INVAL)
        if body["acc"] not in (16, 32) or body["gyro"] not in (2000, 4000) or \
                body["live"] not in (0, 1, 2, 4, 5) or body["n"] < 1 or body["rate"] < 1 or \
                body.get("stop", 10) < 1:
            raise smp.SmpError(smp.INVAL)
        self.session = dict(body)
        self.session.setdefault("stop", 10)
        self.live = body["live"]
        return {}

    def record(self, run):
        if run and not self.record_on:
            self.record_on = True
            self.record_started = self.period() + 1
            for b in self.belkas.values():
                b.state = BELKA_STATE["recording"]
        elif not run and self.record_on:
            self.record_on = False
            for b in self.belkas.values():
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
        if chip not in self.found:
            raise smp.SmpError(smp.NOENT)
        self.belkas[n] = Belka(n, chip)
        if self.record_on:
            self.belkas[n].state = BELKA_STATE["recording"]
        del self.found[chip]
        self.on_set_changed()
        return {}

    def set_view(self):
        now = time.monotonic()
        return {"belkas": [dict(n=b.n, chip=b.chip.to_bytes(8, "little"),
                                ago=int((now - b.last_live) / PERIOD_S) if b.reachable else 500,
                                by=2 if b.n % 3 == 0 else 0, state=b.state, fault=b.fault)
                           for b in sorted(self.belkas.values(), key=lambda b: b.n)],
                "povodoks": 1 << self.number, "ch": self.channel, "spare": self.spare}

    def drop(self, n):
        if n not in self.belkas:
            raise smp.SmpError(smp.NOENT)
        del self.belkas[n]
        self.on_set_changed()
        return {}

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
            if not b.reachable:
                continue
            steps = []
            for k in range(live_view.STEPS[code]):
                tt = t + k / (100.0 if code in (2, 5) else 50.0)
                if live_view.STEP_BYTES[code] == 4:
                    a = 0.3 * math.sin(2 * math.pi * 0.5 * tt + b.n)
                    steps.append((math.cos(a / 2), math.sin(a / 2), 0.0, 0.0))
                else:
                    g = 16384 if self.session["acc"] == 16 else 8192
                    steps.append((int(0.2 * g * math.sin(2 * math.pi * 2.5 * tt + b.n)), 0, g,
                                  int(3000 * math.sin(2 * math.pi * 2.5 * tt)), 0, 0))
            item = 8 if period % 3000 >= 7 else 1 + period % 3000
            state = (b.n, -90 + b.n % 7) if item == 8 else (b.cell_pct, 20)
            belkas.append((b.n, (code << 4) | item, 2 if b.n % 3 == 0 else 0,
                           (state[0] << 8) | (state[1] & 0xFF), steps))
        set_state = (0x8000 if self.record_on else 0) | (20 << 10) | self.spare
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
        self.open, self.nonce = False, None
        self.request.settimeout(600)
        try:
            while True:
                data = self.request.recv(4096)
                if not data:
                    return
                for fr in stream.feed(data):
                    answer = self.answer(fr)
                    if answer:
                        self.request.sendall(answer)
        except (ConnectionError, socket.timeout, ValueError):
            return

    def answer(self, fr):
        op, version, _, length, group, seq, cmd = smp.parse_header(fr)
        if op not in (smp.OP_READ, smp.OP_WRITE):
            return b""
        rsp_op = smp.OP_READ_RSP if op == smp.OP_READ else smp.OP_WRITE_RSP
        if version != smp.VERSION:
            return smp.frame(rsp_op, group, seq, cmd,
                             {"rc": smp.TOO_OLD if version < smp.VERSION else smp.TOO_NEW})
        try:
            body = cbor2.loads(fr[8:8 + length]) if length else {}
            if not isinstance(body, dict):
                raise smp.SmpError(smp.INVAL)
        except (ValueError, cbor2.CBORDecodeError):
            return smp.frame(rsp_op, group, seq, cmd, {"rc": smp.INVAL})
        try:
            if not self.open and not (group == smp.GROUP_STRELKA and cmd == smp.AUTH):
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
                self.key = X25519PrivateKey.generate()
                pub = self.key.public_key().public_bytes(serialization.Encoding.Raw,
                                                         serialization.PublicFormat.Raw)
                return {"nonce": self.nonce, "pub": pub}
            self.check_mac(body)
            self.open = True
            if belka is None:
                sim.phone = (self.client_address[0], self.server.server_address[1])
            return {}
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

    def os_command(self, read, cmd, body, who):
        if cmd == 0 and not read:
            return {"r": body["d"]}
        if cmd == 5 and not read:
            return {}
        if cmd == 6 and read:
            return {"buf_size": 2048, "buf_count": 4}
        if cmd == 7 and read:
            return {"output": "Zephyr 4.4.0 nRF Connect SDK v3.4.0 NIC-Strelka %s simulated" % who}
        if cmd == 8 and read:
            return {"bootloader": "MCUboot"} if "query" not in body else {"mode": 1}
        raise smp.SmpError(smp.NOTSUP)

    def img_command(self, read, cmd, body):
        if cmd == 0 and read:
            return {"images": [dict(image=0, slot=0, version="%d.%d.%d" % FW,
                                    hash=bytes(32), bootable=True, pending=False,
                                    confirmed=True, active=True, permanent=False)],
                    "splitStatus": 0}
        if cmd == 1 and not read:
            return {"off": body.get("off", 0) + len(body.get("data", b""))}
        raise smp.SmpError(smp.NOTSUP)

    def povodok_command(self, op, group, cmd, body):
        sim, read = self.server.sim, op == smp.OP_READ
        if group == smp.GROUP_OS:
            return self.os_command(read, cmd, body, "Povodok")
        if group == smp.GROUP_IMG:
            return self.img_command(read, cmd, body)
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
            sim.on_set_changed()
            return {}
        if cmd == smp.RECORD and not read:
            return sim.record(as_bool(body["run"]))
        if cmd == smp.LIVE and not read:
            if body["live"] not in (0, 1, 2, 4, 5):
                raise smp.SmpError(smp.INVAL)
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
        raise smp.SmpError(smp.NOTSUP)

    def belka_command(self, b, op, group, cmd, body):
        read = op == smp.OP_READ
        if group == smp.GROUP_OS:
            return self.os_command(read, cmd, body, "Belka %d" % b.n)
        if group == smp.GROUP_IMG:
            return self.img_command(read, cmd, body)
        if group == smp.GROUP_FS:
            return self.fs_command(b, read, cmd, body)
        if group != smp.GROUP_STRELKA:
            raise smp.SmpError(smp.NOTSUP)
        if cmd == smp.INFO and read:
            return dict(chip=b.chip.to_bytes(8, "little"), n=b.n, who=2, fw=list(FW),
                        build=BUILD, sensor=0x44,
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
                return dict(b.cal)
            for key, n in (("acc", 6), ("gscale", 3), ("gzero", 3)):
                if len(body[key]) != n:
                    raise smp.SmpError(smp.INVAL)
            b.cal = dict(acc=list(body["acc"]), gscale=list(body["gscale"]),
                         gzero=list(body["gzero"]), temp=body["temp"], t=body["t"])
            return {}
        if cmd == smp.BLINK and not read:
            b.blink_until = time.monotonic() + body["s"]
            return {}
        if cmd == smp.CARD and not read:
            if b.state == BELKA_STATE["recording"]:
                raise smp.SmpError(smp.BUSY)
            if body["do"] not in (1, 2):
                raise smp.SmpError(smp.INVAL)
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
                if phone is not None:
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
    args = parser.parse_args()
    sim = Simulator(args.host, args.port, args.belkas, args.password.encode("utf-8"))
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
