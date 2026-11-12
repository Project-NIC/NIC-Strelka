"""The Povodok simulator driven as the application will drive it, through smp.py: the lock, the
system group, the session and the table, the record and the set, a Belka's port — its sessions
and its record downloaded by the files' group and read back by strelka_record.py to the byte —
the search and the taking, a drop, the password changed, a Belka out of reach.

    python3 tools/test_povodok_sim.py
"""

import gzip
import hashlib
import os
import random
import socket
import sys
import tempfile
import time

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import povodok_sim  # noqa: E402
import smp  # noqa: E402
import strelka_images  # noqa: E402
import strelka_record as rec  # noqa: E402

PASSWORD = "strelka1"


def start():
    for _ in range(20):
        base = random.randint(20000, 60000)
        try:
            return povodok_sim.Simulator("127.0.0.1", base, belkas=4, password=PASSWORD.encode()), base
        except OSError:
            continue
    raise SystemExit("no free ports")


def expect(code, fn, *args, group=None):
    try:
        fn(*args)
    except smp.SmpError as e:
        assert e.rc == code and e.group == group, f"{e.rc}/{e.group}, wanted {code}/{group}"
        return
    raise AssertionError(f"no error, wanted {code}")


def main():
    sim, base = start()
    G = smp.GROUP_STRELKA
    try:
        # the lock: a wrong password refused, a command before the lock refused
        c = smp.Client("127.0.0.1", base)
        expect(smp.ACCESS, c.read, G, smp.INFO)
        expect(smp.ACCESS, c.unlock, "wrong-one")
        c.unlock(PASSWORD)
        info = c.read(G, smp.INFO)
        assert info["who"] == 0 and info["n"] == 0 and len(info["chip"]) == 8 and \
            info["fw"] == [0, 8, 0] and info["card"]["in"] is False
        print("ok the lock, Povodok's info")

        # the system group, as mcumgr's tools ask it
        assert c.write(smp.GROUP_OS, 0, {"d": "hi"}) == {"r": "hi"}
        assert c.read(smp.GROUP_OS, 6)["buf_size"] == 2048
        assert "Zephyr" in c.read(smp.GROUP_OS, 7, {"format": "a"})["output"]
        assert c.read(smp.GROUP_IMG, 0)["images"][0]["version"] == "0.8.0"
        print("ok the system and the images groups")

        # a new image into Povodok's second slot, piece by piece, tested and taken at the reset;
        # the folder Pušinka fetches it from, its manifest by tools/strelka_images.py
        image = strelka_images.fake_image("0.8.6", 3000)
        folder = tempfile.mkdtemp()
        manifest = strelka_images.write(folder, {"belka": image, "kryska": image, "povodok": image})
        assert manifest["version"] == "0.8.6" and manifest["images"]["povodok"]["size"] == 3032
        with gzip.open(os.path.join(folder, manifest["images"]["povodok"]["file"])) as f:
            assert f.read() == image
        assert manifest["images"]["zvjozdocka"]["file"] == "belka.bin.gz"   # the Belka's, today
        assert strelka_images.kind_of(dict(who=2, kind=1)) == "kryska"
        assert strelka_images.kind_of(dict(who=2, kind=2)) == "zvjozdocka"
        assert strelka_images.kind_of(dict(who=0)) == "povodok"
        off = 0
        while off < len(image):
            body = {"off": off, "data": image[off:off + 1024]}
            if off == 0:
                body.update(len=len(image), sha=hashlib.sha256(image).digest())
            rsp = c.write(smp.GROUP_IMG, 1, body)
            off = rsp["off"]
        assert rsp["match"]
        slot = c.read(smp.GROUP_IMG, 0)["images"][1]
        assert slot["version"] == "0.8.6" and not slot["pending"]
        expect(smp.NOENT, c.write, smp.GROUP_IMG, 0, {"hash": bytes(32), "confirm": False})
        assert c.write(smp.GROUP_IMG, 0, {"hash": slot["hash"], "confirm": False})["images"][1]["pending"]
        c.write(smp.GROUP_OS, 5)
        assert c.read(G, smp.INFO)["fw"] == [0, 8, 6]
        print("ok a new image uploaded, tested and taken at the reset; its folder and manifest")

        # 64/19: the set given to Povodok under a fresh lock, the key sealed
        c.give(PASSWORD, 2, 0xA1B2C3D4, 7, bytes(range(16)), label=12)
        assert sim.sim.given == dict(n=2, address=0xA1B2C3D4, channel=7, key=bytes(range(16)))
        assert c.read(G, smp.INFO)["label"] == 12      # the set's own number, the user's
        expect(smp.ACCESS, c.write, G, smp.GIVE, dict(pub=bytes(32), mac=bytes(32), n=0,
                                                      addr=bytes(4), ch=0, key=bytes(16),
                                                      nonce=bytes(13), mic=bytes(4)))
        print("ok the set given to Povodok, 64/19")

        # the session and the table
        expect(smp.INVAL, c.write, G, smp.SESSION, dict(s=3, rate=1000, n=10, acc=16, gyro=2000,
                                                        live=3))
        expect(smp.INVAL, c.write, G, smp.SESSION, dict(s=3, rate=100, n=10, acc=2, gyro=62,
                                                        live=0))     # no whole degrees
        c.write(G, smp.SESSION, dict(s=3, rate=100, n=10, acc=2, gyro=125, live=0))  # the finest
        c.write(G, smp.SESSION, dict(s=3, rate=1000, n=10, acc=16, gyro=2000, live=1, stop=10,
                                     t=1760000000000000, tz=120))
        table = "druh;kůň\njméno;Bára\nbelka;cip;misto\n1;1101;hlava\n2;1202;kohoutek\n"
        text = table.encode("utf-8")
        half = len(text) // 2
        c.write(G, smp.TABLE, {"off": 0, "len": len(text), "csv": text[:half].decode("utf-8", "ignore")})
        # the pieces are text: cut at a character's edge
        first = text[:half].decode("utf-8", "ignore").encode("utf-8")
        c.write(G, smp.TABLE, {"off": 0, "len": len(text), "csv": first.decode("utf-8")})
        c.write(G, smp.TABLE, {"off": len(first), "len": len(text),
                               "csv": text[len(first):].decode("utf-8")})
        assert sim.sim.belkas[1].table == text
        print("ok the session and the table in pieces")

        # the record: refused in the service mode; at work on, the set recording; off, every
        # Belka with a file; then switched off and on, the service mode again
        expect(smp.BAD_STATE, c.write, G, smp.RECORD, {"run": True})
        c.write(G, smp.WORK)
        c.write(G, smp.RECORD, {"run": True})
        view = c.read(G, smp.SET)
        assert [b["n"] for b in view["belkas"]] == [1, 2, 3, 4] and \
            all(b["state"] == 0 for b in view["belkas"]) and view["ch"] == 7
        time.sleep(0.05)
        c.write(G, smp.RECORD, {"run": False})
        view = c.read(G, smp.SET)
        assert all(b["state"] == 4 for b in view["belkas"])
        sim.sim.service = True                          # switched off and on
        # Povodok's own key: the same on every connection and on a Belka's port, another refused
        key = c.unit_key
        with smp.Client("127.0.0.1", base + 2, password=PASSWORD, pinned=key) as again:
            assert again.unit_key == key
        try:
            smp.Client("127.0.0.1", base, password=PASSWORD, pinned=bytes(32))
            raise AssertionError("a key other than the pinned one taken")
        except ValueError:
            pass
        # 64/23: what Povodok knows, 8 Belkas a page, the band with the first
        diag = c.read(G, smp.DIAG)
        assert [b["n"] for b in diag["belkas"]] == [1, 2, 3, 4] and not diag["more"]
        assert len(diag["noise"]) == 4 and len(diag["bnoise"]) == 40 and "rssi" in diag["belkas"][0]
        assert c.read(G, smp.DIAG, {"from": 8}) == {"belkas": [], "more": False}
        # 64/24: the set moved by hand at rest; the search's channel refused
        c.write(G, smp.MOVE, {"ch": 25})
        assert c.read(G, smp.SET)["ch"] == 25
        expect(smp.INVAL, c.write, G, smp.MOVE, {"ch": 39})
        print("ok the record on and off, the set's view, the diagnostics")

        # a Belka's port: its info, its sessions, its record downloaded and read back
        b2 = smp.Client("127.0.0.1", base + 2, password=PASSWORD)
        info = b2.read(G, smp.INFO)
        assert info["who"] == 2 and info["n"] == 2 and info["sensor"] == 0x44 and info["kind"] == 2
        sessions = b2.read(G, smp.SESSIONS, {"from": 0})
        assert len(sessions["list"]) == 1 and not sessions["more"]
        entry = sessions["list"][0]
        assert entry["s"] == 3 and entry["steps"] == 130 and entry["end"] == 1 and \
            entry["file"] == "S00003B02.STRK"
        assert b2.read(smp.GROUP_FS, 1, {"name": entry["file"]})["len"] == entry["bytes"]
        data = b2.download(entry["file"])
        assert b2.download(entry["file"], start=1000) == data[1000:], "the download from a byte"
        assert len(data) == entry["bytes"]
        digest = b2.read(smp.GROUP_FS, 2, {"name": entry["file"], "type": "sha256"})
        assert digest["output"] == hashlib.sha256(data).digest()
        expect(povodok_sim.FS_ERR_FILE_NOT_FOUND, b2.read, smp.GROUP_FS, 1, {"name": "none"},
               group=smp.GROUP_FS)
        s = rec.read(data)
        assert s["head"]["belka"] == 2 and s["head"]["session"] == 3 and s["bad"] == 0 and \
            s["end"]["steps"] == 130 and s["table"] == table and s["gaps"] == []
        print("ok a Belka's sessions, its record downloaded and read back to the byte")

        # the calibration, the card, a blink
        cal = dict(acc=[16380, -16390, 16385, -16395, 16390, -16400], gscale=[32768, 32750, 32700],
                   gzero=[5, -12, 3], range=[16, 2000], temp=2550, t=1759999000)
        b2.write(G, smp.CAL, cal)
        assert b2.read(G, smp.CAL) == cal
        # 64/22: face by face, each answered once its time is over, out of turn refused; the
        # sixth computes and keeps the calibration, as tools/strelka_cal.py does
        sim.sim.face_s = 0.05
        expect(smp.INVAL, b2.write, G, smp.CALIBRATE, {"face": 0})          # no ranges
        b2.write(G, smp.CALIBRATE, dict(face=0, acc=16, gyro=2000, t=1760000000))
        assert b2.read(G, smp.CALIBRATE) == {"face": 0, "got": 3}          # measuring still
        expect(smp.INVAL, b2.write, G, smp.CALIBRATE, {"face": 1})          # before its answer
        time.sleep(0.1)
        assert b2.read(G, smp.CALIBRATE) == {"face": 0, "got": 0}
        expect(smp.INVAL, b2.write, G, smp.CALIBRATE, {"face": 3})          # out of turn
        for face in range(1, 6):
            b2.write(G, smp.CALIBRATE, {"face": face})
            time.sleep(0.1)
            last = b2.read(G, smp.CALIBRATE)
        want = povodok_sim.strelka_cal.calibrate(*povodok_sim.test_cal.unit(), 16, 2000, 100)
        assert last["got"] == 0 and last["acc"] == want["acc"] and last["range"] == [16, 2000]
        assert b2.read(G, smp.CAL)["t"] == 1760000000
        b2.write(G, smp.CAL, cal)
        b2.write(G, smp.BLINK, {"s": 3})
        expect(smp.ACCESS, b2.write, G, smp.CARD, {"do": 1})     # no lock just before
        b2.afresh(PASSWORD, smp.CARD, {"do": 1})
        expect(smp.ACCESS, b2.write, G, smp.CARD, {"do": 1})     # that lock spent
        expect(smp.BAD_STATE, b2.afresh, PASSWORD, smp.CARD, {"do": 3})   # not on its cable
        assert b2.read(G, smp.SESSIONS)["list"] == []
        expect(smp.NOTSUP, b2.write, G, smp.RECORD, {"run": True})   # Povodok's, not a Belka's
        print("ok the calibration written and computed face by face, the card erased with the password "
              "given again")

        # the session's units: 2 and 4 chosen, 4 a Kryska; 1 and 3 aside, recording nothing
        units, kinds = rec.units_bytes({2: 2, 4: 1})
        expect(smp.INVAL, c.write, G, smp.SESSION, dict(s=9, rate=1000, n=10, acc=16, gyro=2000,
                                                        live=0, kinds=kinds))
        table = "kind;horse\nbelka;chip;code;place;kind\n2;0;LF;left fore;Z\n4;0;RF;right fore;K\n"
        c.write(G, smp.TABLE, {"off": 0, "len": len(table), "csv": table})   # the units' kinds
        c.write(G, smp.SESSION, dict(s=9, rate=1000, n=10, acc=16, gyro=2000, live=0,
                                     units=units, kinds=kinds))
        c.write(G, smp.WORK)
        c.write(G, smp.RECORD, {"run": True})
        states = {b["n"]: b["state"] for b in c.read(G, smp.SET)["belkas"]}
        assert states == {1: 4, 2: 0, 3: 4, 4: 0}, states
        c.write(G, smp.RECORD, {"run": False})
        entry = b2.read(G, smp.SESSIONS)["list"][0]
        assert entry["s"] == 9
        assert rec.read(b2.download(entry["file"]))["head"]["units"] == {2: 2, 4: 1}
        b1 = smp.Client("127.0.0.1", base + 1, password=PASSWORD)
        assert all(e["s"] != 9 for e in b1.read(G, smp.SESSIONS)["list"])
        b1.close()
        c.write(G, smp.SESSION, dict(s=10, rate=1000, n=10, acc=16, gyro=2000, live=1, stop=10,
                                     t=1760000000000000, tz=120))      # every unit again
        sim.sim.service = True
        print("ok the session's units: the chosen record, the others aside, the heads say who")

        # the search and the taking: a new Belka, its port opened; a drop closes it
        c.write(G, smp.SEARCH, {"run": True})
        found = c.read(G, smp.SEARCH)["found"]
        assert len(found) == 2 and found[0]["rssi"] == -60
        expect(smp.NOENT, c.write, G, smp.TAKE, {"chip": bytes(8), "n": 5})
        c.write(G, smp.TAKE, {"chip": found[0]["chip"], "n": 5})
        c.write(G, smp.SEARCH, {"run": False})
        assert [b["n"] for b in c.read(G, smp.SET)["belkas"]] == [1, 2, 3, 4, 5]
        b5 = smp.Client("127.0.0.1", base + 5, password=PASSWORD)
        assert b5.read(G, smp.INFO)["chip"] == found[0]["chip"]
        b5.close()
        c.write(G, smp.DROP, {"n": 5})
        assert [b["n"] for b in c.read(G, smp.SET)["belkas"]] == [1, 2, 3, 4]
        try:
            smp.Client("127.0.0.1", base + 5, timeout=1.0)
            raise AssertionError("the dropped Belka's port still open")
        except (ConnectionRefusedError, socket.timeout, OSError):
            pass
        print("ok the search, the taking, the drop")

        # the live view over UDP: a message a period to the port Povodok's own has, the Belkas in
        # it with their steps, the set's state the record's
        import live_view
        with live_view.Receiver(port=base, host="127.0.0.1", timeout=2.0) as r:
            c.write(G, smp.WORK)
            c.write(G, smp.RECORD, {"run": True})
            c.write(G, smp.LIVE, {"live": 5})
            got = [r.next() for _ in range(12)]
            got = [m for m in got if m is not None and m["live"] == 5]
            assert len(got) >= 6, "the live view's messages did not come"
            periods = [m["period"] for m in got]
            assert periods == sorted(periods) and len(set(periods)) == len(periods)
            m = got[-1]
            assert m["state"] & 0x8000 and [b["n"] for b in m["belkas"]] == [1, 2, 3, 4] and \
                all(len(b["steps"]) == 2 and len(b["steps"][0]) == 6 for b in m["belkas"])
            c.write(G, smp.LIVE, {"live": 1})
            got = [r.next() for _ in range(6)]
            m = [m for m in got if m is not None and m["live"] == 1][-1]
            assert all(len(b["steps"]) == 1 and len(b["steps"][0]) == 4 for b in m["belkas"])
            c.write(G, smp.RECORD, {"run": False})
            # the phone silent past 3 s: the live view stops; 64/14 read, it goes on
            time.sleep(3.3)
            while r.next() is not None:
                pass
            assert r.next() is None, "the live view went on to a silent phone"
            c.read(G, smp.SET)
            assert r.next() is not None, "the live view did not come back after 64/14"
        sim.sim.service = True                          # switched off and on
        print("ok the live view's messages over UDP, stopped by the phone's silence")

        # a Belka switched off is out of reach; the password changed
        b2.write(G, smp.OFF)
        expect(smp.STRELKA_UNREACHABLE, b2.read, G, smp.INFO, group=G)
        c.change_password(PASSWORD, "novy-klic-2026")
        expect(smp.ACCESS, smp.Client, "127.0.0.1", base, PASSWORD)
        d = smp.Client("127.0.0.1", base, password="novy-klic-2026")
        assert d.read(G, smp.INFO)["who"] == 0
        # 64/20: out of the service mode — the settings refused with 6, the record still runs
        d.write(G, smp.WORK)
        expect(smp.BAD_STATE, d.write, G, smp.SEARCH, {"run": True})
        expect(smp.BAD_STATE, d.change_password, "novy-klic-2026", "jiny-klic-2026")
        d.write(G, smp.RECORD, {"run": True})
        d.write(G, smp.RECORD, {"run": False})
        d.close()
        b2.close()
        c.close()
        print("ok a Belka out of reach, the password changed, then at work: the settings refused")
        print("POVODOK SIM OK")
    finally:
        sim.close()


if __name__ == "__main__":
    main()
