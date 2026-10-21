<div align="center">

# The principle

**Each block's physics and a rough calculation.**

↑ [NIC-Strelka](README.md)

</div>

---

Every figure in a table here is computed by a script in [`calc/`](calc/) from the inputs in
[`calc/params.py`](calc/params.py), each with its source or marked as assumed;
`python3 calc/check.py --write` writes the tables, `python3 calc/check.py` checks that they still
match. No part is chosen on this page.

## The inside link — the Povodoks' SYNCs, the Belkas' slots

Strelka's own frame carries the small things alone, in six packets. The settings, the commands,
the download and the firmware go by the maker's own, ESB with SMP on it (*The download*, below),
and so does the taking of a new Belka — its number, the set's address and key.

| the packet | what | sent by | when |
|---|---|---|---|
| **SYNC** | the set's time and state | a Povodok | at each period's start, in its fixed slot |
| **LIVE** | the steps and the Belka's state | a Belka | in its slot, every period |
| **MISS** | "these I missed", the Belkas the first Povodok did not hear in the period | the first Povodok | after the slots |
| **PASS** | the missed Belkas' LIVEs, handed to the first Povodok | the other Povodoks | each in its window after the MISS |
| **CALL** | "is a new Belka here?" | a Povodok | searching for new Belkas |
| **HERE** | "here I am", the new Belka's chip ID | a new Belka | in its window after a CALL |

Every packet has one shape, in bytes:

```
preamble 2 | address 4 | who 1 | length 1 | S0 1 | period 4 | state 2 | body | CRC 3
```

| the field | what |
|---|---|
| preamble | 0xAAAA or 0x5555, the radio's own, two bytes with PCNF0.PLEN set to 16 bits — at 2 Mbit/s the radio's default is one |
| address | the set's, heard by the set alone — the radio's base of 3 B and its prefix; or the search's, 0x52E6B438, the same in every firmware, for a new Belka |
| who | bits 7–6 the sender, 0 a Povodok, 2 a Belka, 1 and 3 free; bits 5–0 its number, a Povodok's 0–3, a Belka's 1–63, 0 a new Belka; the radio's S0 field |
| length | the period, the state and the body, 255 B at most, so that the radio finds the CRC; the radio's LENGTH field |
| S0 | what the packet carries, two nibbles, below; the radio's S1 field, 8 bits |
| period | the period's number, the set's common time, the same 4 B the record carries — 2,7 years of periods before it wraps |
| state | from a Povodok the set's, from a Belka its own, below |
| body | by the packet, below |
| CRC | the radio's |

| S0 | the high nibble | the low nibble |
|---|---|---|
| from a Povodok | the live view: 0 off, 1–2 quaternions at 50, 100 Hz, 4–5 raw at 50, 100 Hz, 3 and 6 free; 15 (0xF) a CALL, 14 (0xE) a MISS, 12 (0xC) a PASS | the item of the Belkas' state asked; in a CALL, the attempt, 0–15; in a MISS or a PASS, 0 |
| from a Belka | its steps: 0 none, 1–2 quaternions, 4–5 raw, as above; 13 (0xD) a HERE | the item its state carries |

**The set's state**, in a SYNC: the record and the move the first Povodok's, the margin and the
measurement the sending Povodok's own. Its bits read two ways, by the move: in an even period the
spare channel and the margin, in an odd one the Povodok's measurement, and during a move the move.

| bits | the move 0, 1, 2 | the move 3, the measurement |
|---|---|---|
| 15 | the record: 1 runs, 0 not | the record |
| 14–10 | the margin: the Belkas' signals over the channel's noise at this Povodok, 0–31 by 2 dB | the noise on the channel measured, −100 to −38 dBm by 2 dB |
| 9–8 | free | free |
| 7–6 | the move: 0 none, 2 in two periods, 1 the next period | 3 |
| 5–0 | the spare channel, 0–38, where a move goes | the channel measured, 0–39 |

**A Belka's state**, in its LIVE: one item a period, the one the SYNC's S0 asks, named in the LIVE's
own S0. The SYNC asks item 8 in every period but once a minute, when it asks items 1–7 in seven
periods in a row — seven of the periods the Belkas send in, with the live view off one a second: the
first Povodok holds every Belka's whole state once a minute and the channels all the time (*The
channels*, below). Trouble first: a Belka whose state or fault changes sends item 6 in its next
LIVE, whatever is asked.

| item | the state's 2 B |
|---|---|
| 0 | nothing, zeros |
| 1 | the cell u8, %; the temperature i8, °C |
| 2 | the cell u16, mV |
| 3 | the card's free room u8, %; the blocks dropped u8 |
| 4 | the SYNCs' mean signal i8, dBm; the SYNCs missed in the last second u8 |
| 5 | the clock's offset i16, ns |
| 6 | the state u8; the fault u8, below |
| 7 | the steps out of range since the start: the accelerometer's u8, the gyro's u8 |
| 8 | the channel measured u8, 0–39; its noise i8, dBm |
| 9–15 | free |

| the state | | the fault | |
|---|---|---|---|
| 0 | recording, all well | 0 | none |
| 1 | recording without a SYNC, on its own clock | 1 | no card |
| 2 | recording, looking for the set's channel | 2 | the card full |
| 3 | paused after ten seconds without a SYNC, listening | 3 | a write failed |
| 4 | in the set, waiting | 4 | the card's file system unreadable |
| 5 | calibrating | 5 | blocks dropped |
| 6 | giving its record | 6 | the sensor silent |
| 7 | taking a firmware | 7 | the sensor's clock not steered, the fallback running |
| 8 | erasing or formatting the card | 8 | the cell under 20 % |
| | | 9 | the charger's fault |
| | | 10 | the gauge silent |
| | | 11 | no calibration |

<!-- calc:radio-packets -->
| the packet | its state | its body | all |
|---|---|---|---|
| SYNC | the set's | a signature of 4 B | 22 B |
| LIVE | the Belka's | the steps of the last whole period, 0–24 B | 18–42 B |
| MISS | the set's | a bit for each Belka missed, 8 B | 26 B |
| PASS | the set's | each missed Belka's number, S0, state and steps, up to 28 B a Belka, 249 B at most | up to 267 B |
| CALL | a SYNC's, a new Belka not reading it | a SYNC's | 22 B |
| HERE | 0 | the chip's ID, 8 B | 26 B |
<!-- /calc -->

**The SYNC's signature** is AES-CCM's tag of 4 B with the set's key over the packet's head, who to
the state, the nonce the set's address, who, the period, S0 and four zeros — the fragment's nonce
with the period in the count's place and no receiver, the SYNC being everyone's
([`COMMANDS.md`](COMMANDS.md), *The frame over ESB*); a Belka drops a SYNC whose signature fails, so
that no one forges the set's time or its state.

**A period of 20 ms, the SYNCs first.** Up to four Povodoks stand round the animal, each sending its
SYNC in a fixed slot of its own at the period's start — four slots always, a missing Povodok's
silent. The first Povodok's clock is the set's; the others lock on its SYNC and send theirs from
their own clocks, the content the first one's, a period later; what the first sends to the Belkas,
the others repeat in their own slots. When the first is not heard for a second, the one with the
lowest number takes over — at the switching on too: a Povodok listens a second before its first
SYNC, taking the set's time and state from whoever sends them, so that the lowest number present
stands up as the first and a Povodok coming back does not break the set's time. **The record is a
state**, sent in every SYNC, not a command — a Belka records from the period after the first SYNC
heard saying it runs, and stops after five heard saying it does not, or after ten seconds with none.
A Belka a period early or late lines up all the same, by the period number in its blocks. After the
SYNCs come the Belkas' slots, then the MISS and the PASSes (*The first Povodok gathers the set*,
below), and at the period's end every Povodok measures a channel (*The channels*, below).

<!-- calc:radio-syncs -->
A SYNC is 22 B, its signature of 4 B among them, 88 µs on the air; with 50 µs to turn from receiving
to sending, Povodok k sends at 30 + k × 138 µs, and the guard and the slots of 4 Povodoks take 582
µs of the 20 ms period.
<!-- /calc -->

**The channels.** The band holds 40 channels, 2402–2480 MHz by 2 MHz, as wide as a packet at 2
Mbit/s takes, numbered 0–39; the set works on one of the first 39, the last, 2480 MHz, the search's
alone (*The search for new Belkas*, below). Everyone measures all 40. Every Belka measures one more
in each period, outside its slot and the SYNCs — its number and the period's, modulo 40 — and sends
the channel and its noise as an item of its state. Every Povodok measures one at the period's end,
going through the 40 in turn from ten times its number, and sends it in its next odd period's SYNC.
No one changes anything on its own: the first Povodok keeps the map — its own measurements, the
other Povodoks' and their margins from their SYNCs, the Belkas' from their LIVEs and the PASSes —
and decides alone, the phone only showing it. It names a spare channel in the SYNC — the channel
with the least noise anyone measured, neither the set's nor the search's — and moves the set to it
before the losses come, when the margin at any Povodok has stood under 10 dB for a second; the move
counted down in the state, 2 and then 1, so that every Belka moves in the same period, having heard
it in any of eight SYNCs, four Povodoks' in two periods; a new spare is named after. A Belka that
heard none of them, hearing no SYNC after the move, looks for the set on the spare channel every
other period; failing that, after a second without a SYNC, it listens a period on each of the set's
other channels in turn, the set's own between each — where another Povodok stands up as the first
after the same second — its clock and its record running on meanwhile; a Povodok that lost the first
tries the spare the same way, and takes over on the set's channel after the second:

<!-- calc:radio-channels -->
32 Belkas, each measuring a channel a period, go through the 40 in 2 periods, 40 ms; a Belka that
lost the set listens a period on each of the set's other 38, the set's own between each, all in
1,5 s.
<!-- /calc -->

**A Belka takes its time from any SYNC it hears** — the SYNC names its Povodok, so the Belka knows
where the period began — and needs one now and then, its clock holding between (*The clock loop*,
below). It sends its LIVE in its own slot, by its number from the taking, in every period, a SYNC
heard or not: its clock holds the slot, and a Povodok may hear the Belka when the Belka does not
hear it. No Belka relays anything and none sends again: what no Povodok hears lies in the Belka's
memory and comes afterwards, on the table. Ten seconds without a SYNC stop the record, a setting,
and the Belka's sending; it listens on, and a SYNC saying the record runs takes it up again in the
same session. Ten seconds leave its clock within microseconds of the set's (*The clock loop*), far
inside the 36 µs between two slots.

**The LIVE**'s body is the steps of the last whole period, 1 or 2 at 50 or 100 Hz, each a quaternion
of 4 B or a raw step of 12 B; with the live view off, the Belka's state alone. The quaternion is
"the smallest three": the largest of its four components, kept positive, is named by 2 bits and left
out, the other three go in 10 bits each, ±1/√2 as ±511, an error under 0,2° a component. **The slots
are fixed:** slot k for the Belka numbered k, 1–63, a missing number's slot silent — nothing
compacted, nothing sent to the Belkas to set it — and every slot as long as the longest LIVE, so
that nothing moves when the live view changes:

<!-- calc:radio-live -->
| the live view a Belka | its packet | on the air |
|---|---|---|
| the state alone | 18 B | 72 µs |
| quaternions 50 Hz | 22 B | 88 µs |
| quaternions 100 Hz | 26 B | 104 µs |
| raw 50 Hz | 30 B | 120 µs |
| raw 100 Hz | 42 B | 168 µs |
<!-- /calc -->

<!-- calc:radio-slots -->
Every slot is 204 µs: the raw steps at 100 Hz, 42 B and 168 µs on the air, and the gap of 36 µs; a
shorter packet goes in the same slot. The SYNCs and 63 slots take 13,4 ms of the 20 ms period,
whatever the live view; 95 slots would fit.
<!-- /calc -->

**The first Povodok gathers the set.** After the slots it sends the MISS, a bit for each Belka it
did not hear; the other Povodoks then PASS it those they heard, in fixed windows by their numbers,
each Belka's number, its LIVE's S0, its state and its steps, a Povodok listening to the windows
before its own and leaving out what was passed already. The first keeps one LIVE of each Belka and
period and passes the set on to the phone or the computer, which joins it alone — by USB or Wi-Fi,
one address, any phone. A Povodok the first does not hear passes it nothing, as it gives it no SYNC;
three Povodoks round the animal, up to 30 m apart, hear each other in the open and every Belka from
one side or another, the measurement deciding (*The reach*, below); four are the most:

<!-- calc:radio-pass -->
After the 63 slots, the MISS takes 154 µs; the 3 other Povodoks then have 2,07 ms each and the
measurement the period's last 0,2 ms. A Povodok passes 16 missed Belkas in its window at raw 100 Hz,
28 B each, 38 as quaternions: three Povodoks besides the first pass 48 a period, two 32 — every one
of 32 Belkas even when the first hears none.
<!-- /calc -->

It is little to pass on:

<!-- calc:radio-out -->
Out of Povodok, 32 Belkas at 100 Hz send 13 kB/s as quaternions and 38 kB/s raw, 0,31 Mbit/s — under
a hundredth of the Wi-Fi's 86 Mbit/s on the air and of USB's 480 Mbit/s.
<!-- /calc -->

**The search for new Belkas.** A Belka new or after its factory settings listens on 2480 MHz and the
search's address, fixed in the firmware; Povodok, searching, sends its CALL there in every period,
the attempt, 0–15, in its S0. The Belka draws a number from its chip's ID and the attempt — the low
16 bits of SplitMix64's number attempt + 1, the ID its seed, the generator's own gamma; a CRC, being
linear, would give two IDs that drew one number together the same number at every attempt — which
names a period of the round and a window in it, and sends its HERE in that window, counted from the
CALL's arrival, as TDMA has it; on 2480 MHz, which no set works on, it meets no set's packets. Two
drawing the same number collide; Povodok raises the attempt and they draw again. Then Povodok writes
the found Belka its number, the set's address, key and channel by SMP, and it moves to them.

<!-- calc:radio-search -->
A HERE is 26 B, its chip's ID whole; with the gap a window of 140 µs, 138 after the CALL in a
period, so that its number, one of 65 536, falls in one of 475 periods: every new Belka is heard
once in 9,5 s. Two of 32 draw the same number 0,8 % of the time, and draw again.
<!-- /calc -->

From the nRF54LM20A's sheet (datasheet v1.0): 2 Mbit/s on air; the packet's fields in the order the
radio sends them, S0, LENGTH, S1, the address a base and a prefix (8.17.1, 8.17.2); from receiving
to sending 50 µs, the receiver off in 1 µs and the fast ramp-up 40 µs (11.14.8), the rest a margin.
The packet's shape, the guard and the slots' gap are the design's (`calc/params.py`). The search's
draw is SplitMix64's — the mixing function and the golden-ratio gamma of Steele, Lea and Flood,
"Fast splittable pseudorandom number generators", OOPSLA 2014, as Java's SplittableRandom carries
them.

## The memory — the user's card

The record goes to the user's own microSD card in SPI mode, a block of 512 B to a sector, written
in runs; the card keeps its own wear and its bad blocks. The table counts a card of 8 GB; SPI mode
reads a card of up to 2 TB (the SD Association's Physical Layer Simplified Specification 9.10, 3),
and a card twice the size holds twice as long.

<!-- calc:memory-files -->
Each session is a file on the card's own file system: FAT32 on a card of up to 32 GB and exFAT
above, as the SD standard has them formatted, both read by FatFs in Zephyr; named by the session's
number and the Belka's. A file on FAT32 holds 4 GB, 2,2 h of the raw 32 kHz, and any examination at
the lower rates; its room is taken in one run at the start and its length saved every 3 s, so that a
power cut loses 3 s at most. The card is formatted only at the application's word.
<!-- /calc -->

<!-- calc:memory-hold -->
| into memory | 8 GB holds |
|---|---|
| 50 Hz | 109 days |
| 100 Hz | 54 days |
| 200 Hz | 27 days |
| 400 Hz | 14 days |
| 500 Hz | 11 days |
| 1 kHz | 5,4 days |
| 2 kHz | 2,7 days |
| 4 kHz, raw | 32,6 h |
| 8 kHz, raw | 16,3 h |
| 16 kHz, raw | 8,1 h |
| 32 kHz, raw | 4,1 h |
<!-- /calc -->

<!-- calc:memory-rate -->
The card's bus, 8 MHz, carries some 1,0 MB/s against the record's 546 kB/s at the sensor's 32 kHz
raw. The card pauses at the end of a write, busy at most 250 ms for SDHC and 500 ms for SDXC; the
record waits meanwhile in a buffer of 128 KB of the processor's 512 KB, which bears SDXC's longest
pause up to some 15 kHz and 240 ms at 32 kHz; a longer pause drops a block, its gap seen in the
blocks' numbers.
<!-- /calc -->

The card's bus and its pauses are from ATP's industrial microSD sheet and the SD specification 9.10,
4.6.2.2; a sustained rate is measured on a built unit with the user's card, not read from the sheet.
That a file grown as it is written makes a card pause for up to a second now and then, and one laid
out beforehand for tens of milliseconds, is the loggers' authors' finding
([SdFat](https://github.com/greiman/SdFat), its Teensy logger; [PJRC's forum,
2017](https://forum.pjrc.com/threads/45331-uSD-card-write-time-and-coding-around-it)).

<!-- calc:memory-block -->
A data block is 512 B, a sector of the card: a head of 28 B, 8 B of it free, then 40 steps of 12 B
and a CRC-32; after every 120 steps, 3 data blocks at any rate, an information block of 512 B, 4 B
for each step ([`RECORD.md`](RECORD.md)). The heads and the information blocks take 30 % of the
record at every rate; the tables here count them.
<!-- /calc -->

## The download

The record stopped, on the table or still on the animal: the Belkas give their records over the
radio by Nordic's ESB, its own acknowledgements and repeats in the radio, with SMP on it, which asks
for a file from a given byte ([`COMMANDS.md`](COMMANDS.md)). The periods go on, the SYNCs first;
once a second a period is the Belkas' state, each in its slot, so that every Povodok hears which
Belka it hears and how well. The first Povodok downloads them Belka after Belka and passes them on
by USB or Wi-Fi, both far faster than the set's radio; when the other Povodoks are joined to the
phone too, over Wi-Fi, for the download alone, the phone gives each Belka's next second to the
Povodok that heard it best, the next second taking up from the byte where the last left off — for
the reach, not the pace, as they share the set's one channel. Or each Belka on its own cable,
switched on, is a disk; there the card's bus is the limit, USB's 480 Mbit/s far above it. Or the
whole set goes into the dock, the charging station with a USB hub, every Belka a disk and a computer
copying them all at once, the hub's share the limit, taken as 35 MB/s, the way of research sensors'
docks (APDM's Opal, Axivity's AX6).

<!-- calc:download-pace -->
An ESB packet of 252 B with its acknowledgement and the turns takes 1 188 µs; 16 go in a period
after the SYNCs, a period in 50 kept for the Belkas' state, each carrying 243 B of a frame
([`COMMANDS.md`](COMMANDS.md)), the SMP frames' own bytes counted: 185 kB/s from the Belka being
asked, a second's worth some 2 minutes of its record at 100 Hz.
<!-- /calc -->

ESB's payload of up to 252 B a packet: nRF Connect SDK, Enhanced ShockBurst,
`CONFIG_ESB_MAX_PAYLOAD_LENGTH`; the packet's head, the acknowledgement and the SMP frames' share
assumed (`calc/params.py`); the first boards decide.

<!-- calc:download -->
| the set's record | over the radio, ESB | a Belka on its cable | the set in the dock, at once |
|---|---|---|---|
| 32 Belkas × 15 min at 100 Hz, 49 MB | 4 min | 1,5 s | 1,5 s |
| 32 Belkas × 1 h at 100 Hz, 197 MB | 18 min | 6,1 s | 6,1 s |
| 32 Belkas × 1 h at 1 kHz, 2,0 GB | 3,0 h | 1 min | 1 min |
<!-- /calc -->

## The current — rough

A low and a high case from the parts' figures, the radio's share from the timetable above; the
processor's and the card's duties are assumed, and the measurement on a built unit decides. A
charge must last 15 min. The two LDOs pass the current on, so the cell gives what the board draws
and a few µA of their own (`PARTS.md`, *The supply*). Povodok's Wi-Fi is counted with its receiver
on throughout, the nRF7002's power saving left out; on its cable Povodok runs on the cable, its
cell resting.

<!-- calc:power -->
| unit | current, low | current, high | the cell lasts |
|---|---|---|---|
| Belka | 2,3 mA | 6,6 mA | 108 h to 317 h |
| Povodok, Wi-Fi off | 4,7 mA | 6,9 mA | 105 h to 153 h |
| Povodok, Wi-Fi on | 61 mA | 64 mA | 11 h to 12 h |
<!-- /calc -->

<!-- calc:power-radio -->
The Belka's radio, from the timetable, draws 27–216 µA on average; the processor, the card and the
sensor draw the rest.
<!-- /calc -->

## The reach — rough

Nothing extra is built for the reach but more Povodoks: up to four round the animal, a Belka heard
by any of them — no long-range mode, no relays, no stronger radio. A Povodok rides on the animal,
in the hand, or stays off it — on a stand, a ceiling — where it sees the Belkas best. To the phone
or the computer, Wi-Fi at 5 GHz or a USB lead: the access point's reach, not the set's.

**The power.** The radio sends +5 dBm by default, under the 10 dBm e.i.r.p. below which the norm for
the 2,4 GHz band, EN 300 328, asks nothing of a radio's share of the time nor that it listen before
it sends. The full +8 dBm is a setting, for the far case: the set then keeps the norm's terms for
equipment that does not adapt — no sequence of sending longer than 10 ms, a gap after each as long
and 3,5 ms at least, its use of the medium under 10 % — which a measurement's timetable keeps, and a
download, a Belka sending most of the time, does not: the download goes at the default power alone.
The channel move is the set's own, against the interference, not the norm's.

<!-- calc:link-power -->
By default the radio sends +5 dBm, with the antenna some +9,2 dBm e.i.r.p., under the 10 dBm above
which EN 300 328 asks adaptivity or limits on a radio that does not adapt; the budget 98 dB against
−93 dBm received. The full +8 dBm, +12,2 dBm e.i.r.p., adds 3 dB.
<!-- /calc -->

<!-- calc:link-mu -->
At the full power, 17 mW e.i.r.p., a Belka measuring sends 168 µs a period, a duty cycle of 0,84 %
and a medium utilisation of 0,14 %, and a Povodok 2,16 ms at most, 10,8 % and 1,8 % — under the 10 %
the norm allows equipment that does not adapt. The norm's test counts a silence under 3,5 ms into
the sequence, so the last Povodok's PASS window and its next SYNC are one Tx-sequence of 2,8 ms,
under the 10 ms allowed, with the rest of the period silent after it; the duty cycles are what the
maker declares. A Belka giving its record by ESB sends 84 % of the time, 14 %, so a download goes at
the default power alone.
<!-- /calc -->

The loss in free space at 2,44 GHz against the module's budget at 2 Mbit/s and the default power;
and what is left when a body stands across the path — a person takes up to 40 dB at 2,45 GHz
([Cotton, McKernan and Scanlon](https://research.ucc.ie/en/publications/received-signal-characteristics-of-outdoor-body-to-body-communica/)),
and a path straight through a body is lost — about 120 dB over 30 cm, as cited by
[Wang, Yang, Zhang and Jiang](https://arxiv.org/pdf/1801.09224) — so the wave creeps round it, and
where it cannot, a Povodok on the other side hears the Belka. A cow's trunk is not measured
anywhere found; a Belka on a cow decides.

<!-- calc:link -->
| the link | free space | left of the budget | left behind a body |
|---|---|---|---|
| a Belka to a Povodok round a dog, 1 m | 40 dB | 58 dB | 18 dB |
| a Belka to a Povodok round a cow, 3 m | 50 dB | 48 dB | 8 dB |
| a Belka to a Povodok on a stable's ceiling, 5 m | 54 dB | 44 dB | 4 dB |
| a Belka to a Povodok in the hand, a horse trotted away, 30 m | 70 dB | 28 dB | −12 dB |
<!-- /calc -->

## The clock loop — rough

Each Belka steers its sensor's clock to the set's SYNCs. CLKIN, 32 kHz, is 500 ticks of the 16 MHz
peripheral clock, which the module's 32 MHz crystal drives (nRF54LM20A datasheet v1.0, 3.3); one
period made a tick longer or shorter, 62,5 ns, moves the grid by that much, so a correction of 30
ppm is one tick every 67 of CLKIN's periods. The SYNC's arrival is stamped in hardware, a timer
captured through DPPI; the SYNC names its Povodok, so the Belka takes off that Povodok's place in
the period. Its error is taken as 0,5 µs, a bit at 2 Mbit/s, a Povodok's own lock on the first's
within it. The loop: the first second of SYNCs measures the frequency; then a PI loop of 0,1 Hz,
damped at 0,707, keeps the phase; a period without a SYNC holds the frequency. Simulated in
`calc/clock.py`, inputs in `calc/params.py`:

<!-- calc:clock -->
| the case | how far a Belka's samples lie from the set's grid |
|---|---|
| free-running, the crystals 40 ppm apart | 2,4 ms a minute |
| locked within 1 µs, from the first SYNC | 1,0 s |
| locked, 10 % of the periods without a SYNC | 0,07 µs RMS, 0,21 µs at most |
| 1 s without a SYNC | 0,16 µs at most |
| 5 s without a SYNC | 0,59 µs at most |
| 10 s without a SYNC | 1,5 µs at most |
<!-- /calc -->

A sample lasts 10 ms at 100 Hz and 1 ms at the sensor's 1 kHz. Locked, the units agree within a
fraction of a microsecond; ten seconds without a SYNC, when the record stops, still leave them
within microseconds, the crystal's wander deciding it — assumed at 1 ppm a minute while the unit
warms on the animal. Not modelled: how the sensor's own PLL follows a period one tick longer;
its sheet sets no limit on CLKIN's jitter (DS-000347, 4.10), so a sample decides.

**The fallback**, should the sensor not bear the tick's steps: CLKIN straight from the crystal, 500
ticks every period, steady and with no steps; every sample stamped in hardware as now, and the
samples put onto the set's grid by computation — interpolated at the sensor's 1 kHz, where the
gait's content, below some 20 Hz, loses nothing that matters. The same serves a sensor with no CLKIN
at all, left on its own clock, ±1 % in its PLL mode (DS-000347, 4.10). A sample cannot be fired one
at a time instead: the gyro needs 30 ms from its enable to its drive being ready and is kept on 45
ms at least, the accelerometer 10 ms from sleep (DS-000347, tables 1 and 2, and 14.36, PWR_MGMT0).

## The sensor's noise against the motion — rough

The motion to be seen is millimetres: the trunk's vertical movement, its two lowest and two
highest points compared stride by stride; one low-cost IMU agreed with an established system to
±6,6 mm for MinDiff and ±4,3 mm for MaxDiff ([`ANALYSIS.md`](ANALYSIS.md)). What the sensor's
noise leaves in that displacement, a stride: the accelerometer's white noise integrated twice
from half the stride's frequency up to half the record's rate, and the gyro's tilt, wandering
for 10 s, turning a generous 1 g sideways into the vertical (`calc/noise.py`):

<!-- calc:noise -->
| the motion | its stride | the accelerometer's noise | the gyro's tilt, leaked |
|---|---|---|---|
| a cow at the walk | 0,8 Hz | 0,04 mm | 0,11 mm |
| a horse at the trot | 1,5 Hz | 0,02 mm | 0,03 mm |
| a dog at the trot | 2,5 Hz | 0,01 mm | 0,01 mm |
<!-- /calc -->

**The noise is a hundredth of what is measured, or less**, with either sensor; the second sensor
on the same unit could not have bought anything here. What limits the measure is elsewhere: the
integration and the orientation filter, and the unit moving on the skin and the strap.

**The range limits before the noise, on a limb.** The accelerometer reads ±16 g and the gyro ±2 000
°/s on the ICM-42688-P, ±32 g and ±4 000 °/s on the ICM-42686-P (DS-000639, 3.1, 3.2). Research
units on a horse's distal limb and hoof carry a second accelerometer of ±200 or ±400 g beside the
±16 g one, for the hoof's impact ([Bragança et al.,
2017](https://pmc.ncbi.nlm.nih.gov/articles/PMC5484301); [Tijssen et al.,
2020](https://pmc.ncbi.nlm.nih.gov/articles/PMC7269263)), so a Belka on a hoof boot clips at the
impact, at 16 g or at 32 g. The analysis takes from a limb unit only the moment of the foot's
contact, which a clipped impact still marks; the trunk's units stay well inside the range.
