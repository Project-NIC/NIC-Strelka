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
own fragments, signed. Every number in a CBOR body is CBOR's own; SMP's header is big-endian, as
Zephyr writes it; the fragment's count over ESB, as every number of Strelka's own, little-endian.

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
most, header and body, Zephyr's default for a network transport — a body of 2 040 B — and a unit
tells its own in the system group's *mcumgr parameters*. SMP's versions 0 and 1 are both taken,
each answered in its own, so that Zephyr's own tools speak to a unit as well.

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
| 8/0 a file's download — `name`, `off` → `off`, `data`, and `len` when `off` is 0 | read | a Belka | **the record**, from a given byte ([`PRINCIPLE.md`](PRINCIPLE.md), *The download*); on Kryska the sessions as files all the same, `_2` for a number again ([`KRYSKA.md`](KRYSKA.md)) |
| 8/0 a file's upload — `name`, `off`, `data`, and `len` when `off` is 0 | write | a Kryska of the NOR way on its cable | `STRELKA.SET` alone, **the set given** as a Belka's disk takes it, where that disk would be open ([`chapters/kryska-memory.md`](chapters/kryska-memory.md)); nothing else is written |
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
| 0 | **info** | read | A → P; A → B | — | `chip` b(8) the chip's ID; `n` u the number, a Belka's 1–63, a Povodok's 0–3; `who` u, 0 a Povodok, 2 a Belka; `kind` u, a Belka's 0 a Belka, 1 Kryska, 2 Zvjozdočka, Kryska by its build, Zvjozdočka by its ICM; `fw` [u, u, u]; `build` b(4); `sensor` u WHO_AM_I, 0x70 the LSM6DSV16X, 0x73 the LSM6DSV320X, 0x44 the ICM-42686-P or 0x47 the ICM-42688-P, a Povodok's 0; `card` {`in` bool, `mb` u, `free` u MB, `fs` u, 0 none, 1 FAT32, 2 exFAT}; `cell` {`pct` u, `mv` u}; `temp` i, 0,01 °C; `state` u and `fault` u ([`PRINCIPLE.md`](PRINCIPLE.md), *A Belka's state*); `cal` u, the calibration's time, s since 1970, 0 none; a Povodok's `label` u, the set's own number, 1–9999, absent when it has none |
| 1 | **session** | write | A → P; P → every B | `s` u the session, 16 bits; `rate` u Hz, the sensor's rate on the set's grid — 200 to 2 000 on the hoof's LSM6DSV320X, a faster one refused with 3 —; `n` u the samples a step, 1–10 and 1 from 4 kHz on, the rate into memory `rate` / `n`, the head saying both; `acc` u ±g, 2, 4, 8, 16 or 32, or on the hoof's LSM6DSV320X 64, 128, 256 or 320 — a Belka refuses a range its sensor has not ([`FIRMWARE.md`](FIRMWARE.md), *The sensor and its clock*), with 3, and then does not record at the SYNC's word, its fault 12 in its state and the answer in 64/14's `rc` once the frames go over ESB; `hg` u ±g, absent or 0 by default: on the hoof's LSM6DSV320X both of its accelerometer channels kept, the low-g at `acc`, 16 at most, and the high-g beside it at `hg`, 32, 64, 128, 256 or 320, in the high-g blocks ([`RECORD.md`](RECORD.md), *A high-g block*) — a Belka of another sensor keeps its low-g alone; `gyro` u ±°/s, 125, 250, 500, 1 000, 2 000 or 4 000, whole degrees on purpose ([`WHY.md`](WHY.md), *The architecture*), refused as `acc` where the sensor has it not; `live` u the live view's S0 code, 0, 1, 2, 4 or 5, never 3 or 6, which a Belka sends of itself ([`PRINCIPLE.md`](PRINCIPLE.md), *The LIVE*); `stop` u the seconds without a SYNC that pause the record, 10 by default ([`FIRMWARE.md`](FIRMWARE.md), *A session*); `off` u the seconds without a SYNC that switch a Belka not recording off, 600 by default, and three times it a paused record's end (*The power*); `t` u µs since 1970, UTC, 0 unknown; `tz` i minutes from UTC; `net` bool, false by default: the set gathered by the network, every Povodok on one 5 GHz network with the application and each sent this session, the first sending no MISS ([`PRINCIPLE.md`](PRINCIPLE.md), *The three ways of gathering*); `units` b(8) the units chosen, bit n unit n, little-endian, and `kinds` b(16) their kinds, unit n's at bits 2n and 2n + 1, as 64/0's `kind` — both absent, every unit chosen, as the tools send it; a unit without its bit neither records at the SYNC's word nor sends its LIVE, as at rest; the units and their kinds are read from the set's table in every file, 18 sending it before ([`RECORD.md`](RECORD.md), *The set's table*); and **from Povodok to the Belkas** `p` u the period of the instant `t` | refused with 6, a bad state, while the set records; a Belka given it stays awake ten minutes, its sensor warming before the calibration ([`FIRMWARE.md`](FIRMWARE.md), *A session*) |
| 2 | **sessions** | read | A → B | `from` u the first entry's index | `list` [{`s` u, `file` t, `steps` u, `blocks` u, `p0` u the first step's period, `end` u the reason, 0 unfinished, `bytes` u}], `more` bool |
| 3 | **cal** | write, read | A → B | `acc` [6 i] the raw readings, x up and down, y, z; `gscale` [3 u], 32 768 for 1,000; `gzero` [3 i]; `range` [2 u] the ranges it was made at, g and °/s; `temp` i, 0,01 °C; `t` u, s since 1970; on the hoof's LSM6DSV320X calibrated with both channels, `hg` [6 i] the high-g channel's raw readings, as `acc`, and `hgrange` u the range they were made at, both absent where it was not — the head's groups' calibration ([`RECORD.md`](RECORD.md), *The session's head*); written while the Belka records, it goes into the next file's head | the read returns the same |
| 4 | **blink** | write | A → B, P | `s` u seconds of blinking, 0 stops it | the light orange, fast blinks ([`FIRMWARE.md`](FIRMWARE.md)) |
| 5 | **card** | write | A → B | `do` u, 1 every session erased, 2 the card formatted, 3 the disk opened for writing until the cable leaves ([`FIRMWARE.md`](FIRMWARE.md), *USB*); only right after the lock written again (*The lock*). On Kryska 1 and 2 alike erase the whole memory, its writer running it while the unit says *erasing*; 3 lets `STRELKA.SET` in on the serial port ([`KRYSKA.md`](KRYSKA.md)) | refused with 10, busy, while the Belka records or gives its record; 3 refused with 6, a bad state, off the cable |
| 6 | **off** | write | A → B, P | — | a Belka recording closes its file, its end saying *switched off by the application*, stops sending and lets its hold go; Povodok recording first stops the set, five SYNCs saying so, then switches off |
| 7 | **forget** | write | A → B, P | — only right after the lock written again (*The lock*); a Belka recording refuses it with 6 | a Belka forgets the set's address, key and channel, its number, its calibration and the gauge's learned values, keeps its records and listens on the search's channel; Povodok forgets the set, its network and the password, the build's own again |
| 8 | **key** | write | P → a new B, on the search's channel and address | `pub` b(32) Povodok's public key, X25519 | `pub` b(32) the Belka's |
| 9 | **join** | write | P → a new B | `n` u 1–63; `addr` b(4) the set's address, little-endian, as the HERE's chip; `ch` u the channel, 0–38; `key` b(16) the set's key under AES-CCM with the agreed key, `nonce` b(13), `mic` b(4) | after its answer the Belka moves to `addr` and `ch` |
| 10 | **record** | write | A → P | `run` bool | bit 15 of the SYNC's state; the start refused with 6 while the set is on ESB — a download running, or its SYNCs not back yet (*When the frames go*) |
| 11 | **live** | write | A → P | `live` u the S0 code | the SYNC's S0; the live view on refused with 6 while the set is on ESB, as 64/10 |
| 12 | **search** | write, read | A → P | the write: `run` bool, the CALLs on 2480 MHz; refused with 6, a bad state, while the set records, since the first sends CALLs in the SYNCs' place, all but the state's period's, once a second | the read: `found` [{`chip` b(8), `rssi` i dBm, `ago` u s}] |
| 13 | **take** | write | A → P | `chip` b(8), `n` u 1–63, one unit's alone in the set whatever its kind | Povodok does 8 and 9 to that Belka; 5, not found, if it does not hear it; 3 if another chip holds the number. A Belka given the set by its cable needs no taking: a number heard on the set's address and not taken, Povodok asks it for 64/0 over ESB and admits it once the answer comes, its fragments' MIC proving the set's key; 13 names the chip of one taken over the radio |
| 14 | **set** | read | A → P | — | `belkas` [{`n` u, `chip` b(8), `ago` u periods since its last LIVE, `by` u, 0 heard by the first, 1–3 passed by that Povodok, `state` u, `fault` u, `rc` u the last 64/1's answer Povodok had from it, absent none yet, `esb` u the Povodok it downloads with, from its item 8, absent unknown}]; `povodoks` u a bit for each heard; `ch` u the set's channel; `spare` u |
| 15 | **drop** | write | A → P | `n` u | Povodok stops waiting for the Belka; the Belka forgets by 7 |
| 16 | **auth** | read, write | A → P, first on every connection | the read: — ; the write: `pub` b(32) the application's public key, X25519; `mac` b(32) | the read: `nonce` b(16) and `pub` b(32) Povodok's own public key, the same on every connection, which the application pins; the write: 0, and the connection is open — until then every other command on it answers 11, access denied |
| 17 | **password** | write | A → P | `pub` b(32) and `mac` b(32) as in 16, with a fresh nonce; `new` b(32) the new password's SHA-256 | |
| 18 | **table** | write | A → P; P → every B | `off` u, `len` u the whole text's length, `csv` t a piece of it, 1 900 B at most | a Belka writes it after the head of every file ([`RECORD.md`](RECORD.md), *The set's table*) |
| 19 | **give** | write | A → P | `pub` b(32) and `mac` b(32) as in 16, with a fresh nonce; `n` u the Povodok's number, 0–3; `addr` b(4) the set's address, little-endian, as in 64/9; `ch` u the channel, 0–38; `key` b(16) the set's key under AES-CCM with the first 16 B of the secret that write's lock agreed, `nonce` b(13), `mic` b(4); `label` u the set's own number, 1–9999, the user's, absent for none | Povodok keeps the set and its number and starts again; refused with 6, a bad state, while the set records or at work ([`FIRMWARE.md`](FIRMWARE.md), *What a unit keeps*) |
| 20 | **work** | write | A → P | — | Povodok leaves its service mode until it is switched off and on: Bluetooth LE off, the record and the live view allowed, and from then on 64/7, 64/12, 64/13, 64/15, 64/17 and 64/19 refused with 6, and to a Belka 64/5, 64/7, 64/8 and 64/9, never bridged — the calibration, 64/3 and 64/22, passes at work too; until it, 64/10 `run` true and 64/11 a live view refused with 6 ([`FIRMWARE.md`](FIRMWARE.md), *The service mode*) |
| 21 | **fetch** | write | A → P | `s` u the session; `belkas` [u] the Belkas, absent every one taken; `from` [u] each one's first byte, as many as `belkas`, absent 0 | Povodok goes round the Belkas over ESB, from each in turn a frame of her file's download, 8/0, `/SD:/S` the session in five digits `B` her number in two `.STRK`, and passes every answer on as it comes on TCP port **1401**: her number u8, the frame's length u16 and the frame as she sent it; her number with the length 0, she is done without her file whole — with another Povodok, gone, or her file missing; the number 0 with the length 0, the download over (*When the frames go*, below). Refused with 6 while the set records or shows its live view, with 10 while a download runs |
| 22 | **calibrate** | write, read | A → B | the write: `face` u 0–5, z up, z down, x up, x down, y up, y down, each the face taken last and one, or 0; with face 0 also `acc` u g and `gyro` u °/s, the ranges as in 1, `hg` u g, absent or 0 none, the high-g channel's range, the hoof's LSM6DSV320X calibrating it from the same faces beside the low-g, a Belka of another sensor refusing it with 3, and `t` u, s since 1970 — face 0 starts the calibration again, another out of turn or while one is measured refused with 3; the read: — | the write: its samples from then on, the answer at once; the read, the face's answer once its 5 s are over: `face` u, `got` u, 0 the face taken, 1 the unit moved, 2 another face, 3 measured still; on the sixth taken the Belka has computed the calibration and kept it, the read carrying 3's fields too ([`FIRMWARE.md`](FIRMWARE.md), *Calibration*). The write refused with 6 while the Belka records |
| 23 | **diag** | read | A → P | `from` u the first Belka's index | what Povodok knows already: `belkas` [{`n` u; `rssi` i dBm, its last LIVE at this Povodok, absent none; `ago` u periods since its last LIVE; its items as last heard, each absent unknown — `cell` u %, `temp` i °C, `mv` u, `free` u % the card's, `dropped` u blocks, `sync` i dBm the SYNCs' mean signal, `missed` u SYNCs in the last second, `offset` i ns the clock's, `out` [u, u] the steps out of range, the accelerometer's and the gyro's, `ch` u the channel measured, `noise` i dBm its noise}], 8 a page; `more` bool; with `from` 0 also `ch` u the set's channel, `spare` u, `margin` [4 u] each Povodok's, 0–31 by 2 dB, `noise` [4 [40 i]] each Povodok's last on each channel and `bnoise` [40 i] the Belkas', dBm, −128 not measured yet. Read once a second in the service mode and at work, the live view on or off ([`APP.md`](APP.md), *The menus*, 5) |
| 24 | **move** | write | A → P | `ch` u the channel, 0–38 | the first Povodok moves the set to it as it moves it on its own — said in two SYNCs, every unit moving in the same period ([`PRINCIPLE.md`](PRINCIPLE.md), *The channels*); 3 for the set's own channel, the search's, or a Povodok not the first; refused with 6 while the set records |

**The lock**, 64/16: Povodok keeps the SHA-256 of the application's password, twelve characters
at least, and its own long-term X25519 key, made at its first start and made anew by its factory
settings. On every connection the two agree a secret by X25519 — Povodok gives its own public key
with the nonce, the application a fresh one of its own with the write — and the application writes
`mac` = HMAC-SHA256 with the password's hash as the key over the nonce followed by the SHA-256 of
the secret, 32 B, so that the password never crosses the lead or the air, and whoever only listens
cannot try passwords on what he heard: without the secret there is nothing to compare. The
application pins Povodok's key at the first joining, by its address, best made on the cable, and
shows it as a fingerprint, the first 8 B of its SHA-256; another key it refuses before the MAC goes
out, taken only at the user's word after Povodok's factory settings, so that whoever answers as
Povodok without its key gets no MAC to try passwords on. A wrong MAC is answered
after a second, the next after two, then four, up to 32 s, counted over every connection of the
unit and set back by a right one; that connection alone waits, the others going on, and the third
wrong on one connection closes it, so that guessing at a unit costs the guesser its time. The pair
agreed is the connection's own: two locks in progress on two connections never cross. The same for
the password's change, 64/17, with a fresh nonce. **The password for each act**: 64/5, the card's,
and 64/7, the forgetting — what takes a card's records, opens its disk or lets a unit go — pass only
as the next frame after the lock written again on the connection, 64/16's read and write, the
password asked of the user again; every other frame spends that, and without it they answer 11. A
unit comes from the factory, and from its factory settings, with the firmware's own password — a
setting of the build, `strelka1` as built here, shorter than twelve — which the application asks to
change at its first session.
**The taking**, 64/8 and 64/9: the two agree a key by X25519, the chip's CRACEN doing it; the agreed
key is the first 16 B of the SHA-256 of the shared secret, and under it, by AES-CCM, the set's key
reaches the Belka once. **On the serial port of a Kryska of the NOR way there is no lock**: every
frame passes, whoever holds the cable holding the unit, as whoever holds a magnet does
([`chapters/kryska-memory.md`](chapters/kryska-memory.md)).

## A Belka through Povodok: a port each

SMP has no field for whom a frame is meant. Povodok listens on TCP port **1337**, SMP's usual, for
itself, and on **1337 + n** for the Belka numbered n, 1338–1400: a frame from that port goes over
ESB to that Belka and its answer comes back the same way, so that the application and any tool of
SMP's talk to a Belka as to anything else, and a download's frames go out on **1401** (64/21); a
Belka Povodok does not hear answers with group 64's error 1, the bridged target out of reach; a
Belka that took the frame and never answered, within 3 s — a Belka asleep is woken in its state's
period first ([`FIRMWARE.md`](FIRMWARE.md), *The power*) — is answered with 4, a timeout, and the
connection reads on. The frames on a connection follow one another, each its header and `length`
bytes, nothing between; every connection begins with the lock. Joined, the application reads 64/14
once a second, so that Povodok knows the phone is there and the phone that Povodok answers. A
connection that sends no frame for two minutes, or whose peer will not take an answer within a
second, is closed, and a peer gone without a word is found by TCP's keep-alive, so that the eight
connections' slots come free. The live view goes apart, over UDP ([`PHONE.md`](PHONE.md)), and stops
when the phone has been silent 3 s.

## The frame over ESB, between Povodok and a Belka

An ESB payload carries 252 B at most, a frame of SMP up to 2 048 B: the frame goes in fragments,
each signed by the set's key, a count in it against a replay (*The lock*, above). The count is the
sender's, from 0 at the taking: a Povodok's one for each Belka, a Belka's one for every Povodok, a
receiver holding the last it took from each sender; it outlives a start: a unit saves its count
every 1 024, and starting again sends from the saved one plus 1 024, so that no count is sent twice;
what it takes starts at the count it had taken when it last saved, so that the other side, which did
not start again, is heard at once — the fragments sent since that saving, under 1 024, may each be
replayed once before the next start, each still one of a kind by its MIC and count.

| byte | the field | what |
|---|---|---|
| 0 | bits 7–6 the **fragment**: 0 a whole frame, 1 the first, 2 a middle one, 3 the last; bits 5–0 its **index**, 0–63 | |
| 1–4 | the **count** u32 | against a replay: the sender's, a Povodok's one for each Belka, a Belka's one for every Povodok, from 0 at the taking; a fragment with a count not above the last taken from that sender is dropped |
| 5 … | the frame's bytes | the header and the CBOR, or a piece of them, 243 B at most |
| the last 4 | the **MIC** b(4) | AES-CCM's tag with the set's key over every byte before it; the nonce 13 B: the set's address 4 B, the sender's who 1 B, the count 4 B, the fragment's index 1 B, the receiver's who 1 B, two zeros — the receiver in it, so that a fragment meant for one Belka is nothing to another |

<!-- calc:commands-esb -->
A fragment carries 243 B of a frame, the ESB payload of 252 B less its head of 5 B and the MIC of 4
B; a frame of 2 048 B goes in 9 fragments, each acknowledged by the radio, and a Belka asked for its
record gives 198 kB/s.
<!-- /calc -->

**When the frames go.** Only while the set neither records nor shows its live view. A frame waiting
for a Belka gives the set over to ESB: the SYNCs carry bit 14 of the set's state for two of the
state's periods, so that every Belka hears it, asleep or not, and every Povodok repeats it; then
they pause, and ESB has the radio whole, every Povodok joined to the phone on a channel of its own,
given in its SYNC while bit 14 is set — the first the set's, another the quietest it measured of its
quarter of the set's channels, its number and every fourth, never the set's — and every Belka on the
channel of the Povodok she downloads with (*The download* in [`PRINCIPLE.md`](PRINCIPLE.md)). A
download is asked once, by 64/21 to every Povodok joined, the same Belkas to each: each goes round
them, a frame of the file's download from each in turn, and those that answer on its channel are its
own — one that has not answered its first request three times, ESB's repeats spent each time, is
with another Povodok and left to it, one that has answered is asked again until her file is out.
Povodok speaks to one Belka at a time, its request out fragment by fragment, then her answer back, a
fragment going again until the radio acknowledges it; a frame from a port, it keeps 4 waiting and
lets one go unanswered after 3 s (*A Belka through Povodok*, above). ESB goes as Nordic made it, its
acknowledgements and repeats. Once a minute the first leaves ESB for two of the state's periods and
sends its SYNC, bit 14 in it, on the set's channel, for the units back on the SYNCs. Every unit
stays on ESB while packets come to it, and 10 s with none bring it back to the SYNCs, a Belka
listening in every period until one is heard and then as ever, asleep and switched off after 10
minutes without one; a Povodok back from a channel of its own stays silent on the set's until the
first's SYNC, 10 minutes at most ([`FIRMWARE.md`](FIRMWARE.md), *The power*).

The check of the MIC runs in the chip's cryptographic hardware; what it costs a fragment is measured
on the kit, and if it tells, it lowers the download's pace alone — the SYNCs are the frame's own
packets and take nothing from it.
