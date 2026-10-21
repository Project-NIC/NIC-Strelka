<div align="center">

# The commands

**What the application, Povodok and the Belkas say to each other by SMP, to the byte.**

↑ [NIC-Strelka](README.md)

</div>

---

The commands, the settings, the downloads and the firmware go by SMP, the protocol of Zephyr's
MCUmgr: its header, its own groups and its error codes as Zephyr has them ([the SMP protocol
specification](https://docs.zephyrproject.org/latest/services/device_mgmt/smp_protocol.html);
[the handlers](https://docs.zephyrproject.org/latest/services/device_mgmt/mcumgr_handlers.html)),
and Strelka's own group, 64, the first number Zephyr leaves to a user. Over the lead or Wi-Fi the
frames ride on TCP ([`PHONE.md`](PHONE.md)); between Povodok and the Belkas on ESB, in Strelka's
own fragments, signed. Every number in a CBOR body is CBOR's own; the frames on the wire are
little-endian nowhere here — SMP's header is big-endian, as Zephyr writes it.

## The SMP header, 8 B, then the body as a CBOR map

| byte | the field | what |
|---|---|---|
| 0 | bits 2–0 **op**: 0 a read, 1 its answer, 2 a write, 3 its answer; bits 4–3 the **version**, 1; bits 7–5 zero | |
| 1 | the flags | zero, reserved |
| 2–3 | the body's length u16 | the CBOR bytes that follow |
| 4–5 | the group u16 | 0 the system, 1 the images, 8 the files, **64 Strelka** |
| 6 | the sequence u8 | the answer carries the same |
| 7 | the command u8 | its number within the group |

An answer is an empty map on success, or the fields the table below names; on failure `rc` u,
Zephyr's code bare in the map, as SMP's second version gives it (`mgmt_defines.h`; `smp.c`): 0 all
well, 1 unknown, 2 no memory, 3 a bad value, 4 a timeout, 5 not found, 6 a bad state, 7 too large, 8
unsupported, 9 corrupt, 10 **busy**, 11 **access denied**, 12 and 13 a version too old or too new; a
group's own code in `err: {group, rc}` — Strelka's, group 64: 1 **the bridged target out of reach**;
the files' group's its own (`fs_mgmt.h`), 3 the file not found among them. A frame is 2 048 B at
most, Zephyr's default for a network transport, and a unit tells its own in the system group's
*mcumgr parameters*.

## The standard groups taken as they are

| group/command | op | to | for |
|---|---|---|---|
| 0/0 echo (`d`: text) | write | a Belka, Povodok | a ping |
| 0/5 reset | write | a Belka, Povodok | a restart, after a firmware |
| 0/6 mcumgr parameters → `buf_size`, `buf_count` | read | a Belka, Povodok | the largest frame the unit takes |
| 0/7 OS and application info | read | a Belka, Povodok | Zephyr's and the SDK's versions; Strelka's own is in 64/0 |
| 0/8 bootloader info | read | a Belka, Povodok | MCUboot and its slots |
| 1/0 the images' state — slot, version, hash, pending, confirmed | read | a Belka, Povodok | before and after a firmware |
| 1/1 an image's upload — `image`, `len`, `off`, `sha`, `data`, `upgrade` | write | a Belka, Povodok | the firmware, in pieces ([`FIRMWARE.md`](FIRMWARE.md)) |
| 1/5 a slot erased | write | a Belka, Povodok | |
| 8/0 a file's download — `name`, `off` → `off`, `data`, and `len` when `off` is 0 | read | a Belka | **the record**, from a given byte ([`PRINCIPLE.md`](PRINCIPLE.md), *The download*) |
| 8/1 a file's status — `name` → `len` | read | a Belka | the length before a download |
| 8/2 a file's hash — `name`, `type`, `off`, `len` | read | a Belka | a download checked; the blocks carry their own CRC-32 besides |

The settings group, 3, is not used: it reads any key, and the set's key is not for reading. The
date and time do not go by the system group's command 4 but in the session, 64/1, since Povodok
adds the period of that instant to them.

## Strelka's group, 64

A is the application, P Povodok, B a Belka; u an unsigned integer, i a signed one, b(n) a byte
string of n, t a text, [ ] an array, { } a map. The values are the pages': the rates, ranges and
the live view's codes of [`PRINCIPLE.md`](PRINCIPLE.md), the calibration's fields and the end's
reasons of [`RECORD.md`](RECORD.md); only the keys are new.

| id | the command | op | from → to | the request | the answer |
|---|---|---|---|---|---|
| 0 | **info** | read | A → P; A → B | — | `chip` b(8) the chip's ID; `n` u the number, a Belka's 1–63, a Povodok's 0–3; `who` u, 0 a Povodok, 2 a Belka; `fw` [u, u, u]; `build` b(4); `sensor` u WHO_AM_I, 0x44, 0x47 or 0xDB, a Povodok's 0; `card` {`in` bool, `mb` u, `free` u MB, `fs` u, 0 none, 1 FAT32, 2 exFAT}; `cell` {`pct` u, `mv` u}; `temp` i, 0,01 °C; `state` u and `fault` u ([`PRINCIPLE.md`](PRINCIPLE.md), *A Belka's state*); `cal` u, the calibration's time, s since 1970, 0 none |
| 1 | **session** | write | A → P; P → every B | `s` u the session, 16 bits; `rate` u Hz; `n` u the samples a step; `acc` u ±g, 16 or 32; `gyro` u ±°/s, 2 000 or 4 000; `live` u the live view's S0 code, 0, 1, 2, 4 or 5; `stop` u the seconds without a SYNC that stop the record, 10 by default; `t` u µs since 1970, UTC, 0 unknown; `tz` i minutes from UTC; and **from Povodok to the Belkas** `p` u the period of the instant `t` | — |
| 2 | **sessions** | read | A → B | `from` u the first entry's index | `list` [{`s` u, `file` t, `steps` u, `blocks` u, `p0` u the first step's period, `end` u the reason, 0 unfinished, `bytes` u}], `more` bool |
| 3 | **cal** | write, read | A → B | `acc` [6 i] the raw readings, x up and down, y, z; `gscale` [3 u], 32 768 for 1,000; `gzero` [3 i]; `temp` i, 0,01 °C; `t` u, s since 1970 — the head's fields ([`RECORD.md`](RECORD.md)) | the read returns the same |
| 4 | **blink** | write | A → B, P | `s` u seconds of blinking, 0 stops it | the light orange, fast blinks ([`FIRMWARE.md`](FIRMWARE.md)) |
| 5 | **card** | write | A → B | `do` u, 1 every session erased, 2 the card formatted | refused with 10, busy, while the Belka records |
| 6 | **off** | write | A → B, P | — | a Belka recording closes its file, its end saying *switched off by the application*, stops sending and lets its hold go; Povodok recording first stops the set, five SYNCs saying so, then switches off |
| 7 | **forget** | write | A → B, P | — | a Belka forgets the set's address, key and channel, its number, its calibration and the gauge's learned values, keeps its records and listens on the search's channel; Povodok forgets the set and its network |
| 8 | **key** | write | P → a new B, on the search's channel and address | `pub` b(32) Povodok's public key, X25519 | `pub` b(32) the Belka's |
| 9 | **join** | write | P → a new B | `n` u 1–63; `addr` b(4) the set's address; `ch` u the channel, 0–38; `key` b(16) the set's key under AES-CCM with the agreed key, `nonce` b(13), `mic` b(4) | after its answer the Belka moves to `addr` and `ch` |
| 10 | **record** | write | A → P | `run` bool | bit 15 of the SYNC's state |
| 11 | **live** | write | A → P | `live` u the S0 code | the SYNC's S0 |
| 12 | **search** | write, read | A → P | the write: `run` bool, the CALLs on 2480 MHz; refused with 6, a bad state, while the set records, since the first sends CALLs in the SYNCs' place | the read: `found` [{`chip` b(8), `rssi` i dBm, `ago` u s}] |
| 13 | **take** | write | A → P | `chip` b(8), `n` u 1–63 | Povodok does 8 and 9 to that Belka; 5, not found, if it does not hear it |
| 14 | **set** | read | A → P | — | `belkas` [{`n` u, `chip` b(8), `ago` u periods since its last LIVE, `by` u, 0 heard by the first, 1–3 passed by that Povodok, `state` u, `fault` u}]; `povodoks` u a bit for each heard; `ch` u the set's channel; `spare` u |
| 15 | **drop** | write | A → P | `n` u | Povodok stops waiting for the Belka; the Belka forgets by 7 |
| 16 | **auth** | read, write | A → P, first on every connection | the read: — ; the write: `pub` b(32) the application's public key, X25519; `mac` b(32) | the read: `nonce` b(16) and `pub` b(32) Povodok's public key for this connection; the write: 0, and the connection is open — until then every other command on it answers 11, access denied |
| 17 | **password** | write | A → P | `pub` b(32) and `mac` b(32) as in 16, with a fresh nonce; `new` b(32) the new password's SHA-256 | |
| 18 | **table** | write | A → P; P → every B | `off` u, `len` u the whole text's length, `csv` t a piece of it, 1 900 B at most | a Belka writes it after the head of every file ([`RECORD.md`](RECORD.md), *The set's table*) |

**The lock**, 64/16: Povodok keeps the SHA-256 of the application's password, eight characters at
least. On every connection Povodok and the application agree a secret by X25519 — Povodok gives its
public key with the nonce, the application its own with the write — and the application writes `mac`
= HMAC-SHA256 with that hash as the key over the nonce followed by the SHA-256 of the secret, 32 B,
so that the password never crosses the lead or the air, and whoever listens cannot try passwords on
what he heard: without the secret there is nothing to compare. The same for the password's change,
64/17, with a fresh nonce. A unit comes from the factory, and from its factory settings, with the
firmware's own password — a setting of the build, `strelka1` as built here — which the application
changes at its first session. **The taking**, 64/8 and 64/9: the two agree a key by X25519, the
chip's CRACEN doing it; the agreed key is the first 16 B of the SHA-256 of the shared secret, and
under it, by AES-CCM, the set's key reaches the Belka once.

## A Belka through Povodok: a port each

SMP has no field for whom a frame is meant. Povodok listens on TCP port **1337**, SMP's usual, for
itself, and on **1337 + n** for the Belka numbered n, 1338–1400: a frame from that port goes over
ESB to that Belka and its answer comes back the same way, so that the application and any tool of
SMP's talk to a Belka as to anything else; a Belka Povodok does not hear answers with group 64's
error 1, the bridged target out of reach. The frames on a connection follow one another, each its
header and `length` bytes, nothing between; every connection begins with the lock. The live view
goes apart, over UDP ([`PHONE.md`](PHONE.md)).

## The frame over ESB, between Povodok and a Belka

An ESB payload carries 252 B at most, a frame of SMP up to 2 048 B: the frame goes in fragments,
each signed by the set's key, a count in it against a replay (the README, *The lock*). The count is
one a direction and a Belka, from 0 at the taking, and outlives a start: a unit saves its count
every 1 024, and starting again takes the saved one plus 1 024 as its next and as the floor under
which it takes none, so that no count is sent twice and none under the last is taken.

| byte | the field | what |
|---|---|---|
| 0 | bits 7–6 the **fragment**: 0 a whole frame, 1 the first, 2 a middle one, 3 the last; bits 5–0 its **index**, 0–63 | |
| 1–4 | the **count** u32 | against a replay: one count a direction and a Belka, from 0 at the taking; a fragment with a count not above the last is dropped |
| 5 … | the frame's bytes | the header and the CBOR, or a piece of them, 243 B at most |
| the last 4 | the **MIC** b(4) | AES-CCM's tag with the set's key over every byte before it; the nonce 13 B: the set's address 4 B, the sender's who 1 B, the count 4 B, the fragment's index 1 B, the receiver's who 1 B, two zeros — the receiver in it, so that a fragment meant for one Belka is nothing to another |

<!-- calc:commands-esb -->
A fragment carries 243 B of a frame, the ESB payload of 252 B less its head of 5 B and the MIC of 4
B; a frame of 2 048 B goes in 9 fragments, each acknowledged by the radio, and a Belka asked for its
record gives 185 kB/s.
<!-- /calc -->

The check of the MIC runs in the chip's cryptographic hardware; what it costs a fragment is measured
on the kit, and if it tells, it lowers the download's pace alone — the SYNCs are the frame's own
packets and take nothing from it.
