"""The Povodok simulator driven as the application will drive it, through smp.py: the lock, the
system group, the session and the table, the record and the set, a Belka's port — its sessions
and its record downloaded by the files' group and read back by strelka_record.py to the byte —
the search and the taking, a drop, the password changed, a Belka out of reach.

    python3 tools/test_povodok_sim.py
"""

import hashlib
import os
import random
import socket
import sys
import time

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import povodok_sim  # noqa: E402
import smp  # noqa: E402
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
            info["fw"] == [0, 7, 0] and info["card"]["in"] is False
        print("ok the lock, Povodok's info")

        # the system group, as mcumgr's tools ask it
        assert c.write(smp.GROUP_OS, 0, {"d": "hi"}) == {"r": "hi"}
        assert c.read(smp.GROUP_OS, 6)["buf_size"] == 2048
        assert "Zephyr" in c.read(smp.GROUP_OS, 7, {"format": "a"})["output"]
        assert c.read(smp.GROUP_IMG, 0)["images"][0]["version"] == "0.7.0"
        expect(smp.NOTSUP, c.write, G, 19, {})
        print("ok the system and the images groups")

        # the session and the table
        expect(smp.INVAL, c.write, G, smp.SESSION, dict(s=3, rate=1000, n=10, acc=16, gyro=2000,
                                                        live=3))
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

        # the record: on, the set recording; off, every Belka with a file
        c.write(G, smp.RECORD, {"run": True})
        view = c.read(G, smp.SET)
        assert [b["n"] for b in view["belkas"]] == [1, 2, 3, 4] and \
            all(b["state"] == 0 for b in view["belkas"]) and view["ch"] == 7
        time.sleep(0.05)
        c.write(G, smp.RECORD, {"run": False})
        view = c.read(G, smp.SET)
        assert all(b["state"] == 4 for b in view["belkas"])
        print("ok the record on and off, the set's view")

        # a Belka's port: its info, its sessions, its record downloaded and read back
        b2 = smp.Client("127.0.0.1", base + 2, password=PASSWORD)
        info = b2.read(G, smp.INFO)
        assert info["who"] == 2 and info["n"] == 2 and info["sensor"] == 0x44
        sessions = b2.read(G, smp.SESSIONS, {"from": 0})
        assert len(sessions["list"]) == 1 and not sessions["more"]
        entry = sessions["list"][0]
        assert entry["s"] == 3 and entry["steps"] == 130 and entry["end"] == 1 and \
            entry["file"] == "S00003B02.STRK"
        assert b2.read(smp.GROUP_FS, 1, {"name": entry["file"]})["len"] == entry["bytes"]
        data = b2.download(entry["file"])
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
                   gzero=[5, -12, 3], temp=2550, t=1759999000)
        b2.write(G, smp.CAL, cal)
        assert b2.read(G, smp.CAL) == cal
        b2.write(G, smp.BLINK, {"s": 3})
        b2.write(G, smp.CARD, {"do": 1})
        assert b2.read(G, smp.SESSIONS)["list"] == []
        expect(smp.NOTSUP, b2.write, G, smp.RECORD, {"run": True})   # Povodok's, not a Belka's
        print("ok the calibration, the card erased")

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
        print("ok the live view's messages over UDP")

        # a Belka switched off is out of reach; the password changed
        b2.write(G, smp.OFF)
        expect(smp.STRELKA_UNREACHABLE, b2.read, G, smp.INFO, group=G)
        c.change_password(PASSWORD, "novy-klic-2026")
        expect(smp.ACCESS, smp.Client, "127.0.0.1", base, PASSWORD)
        d = smp.Client("127.0.0.1", base, password="novy-klic-2026")
        assert d.read(G, smp.INFO)["who"] == 0
        d.close()
        b2.close()
        c.close()
        print("ok a Belka out of reach, the password changed")
        print("POVODOK SIM OK")
    finally:
        sim.close()


if __name__ == "__main__":
    main()
