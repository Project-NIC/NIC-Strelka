<div align="center">

# The record

**What a Belka writes on its card, to the byte.**

↑ [NIC-Strelka](README.md)

</div>

---

## The file

Each session is one file on the card's own file system, FAT32 or exFAT as the card comes, named
by the session's number and the Belka's ([`PRINCIPLE.md`](PRINCIPLE.md), *The memory*). The file is
a run of blocks of 512 B, a sector of the card each, every number in them little-endian, every
block ending in a CRC-32 over the bytes before it — IEEE's, as zlib's `crc32` and ZIP's, so that any
tool checks it; the main core computes it, some 30 µs a block:

```
[the session's head] [the set's table]  D D D … I  D D D … I  …  [the end]
```

- **The session's head**, one block: who, when, how fast, the ranges, the calibration, below.
- **The set's table**, one block or more: what the user filled in the application — where each
  Belka sits, the animal, the notes — the same in every Belka's file.
- **D, a data block**: the sensor's steps, below.
- **I, an information block**, after every 120 steps — three data blocks, at any rate, so that
  it takes the same share of every record: what the clock and the radio did for each of those
  steps, the temperature and the cell, below.
- **The end**, one block: why the record ended and how many steps it holds.

## The session's head

The file's first block, written at the start:

| bytes | the field | what for |
|---|---|---|
| 0–3 | `STRK` | the mark of a Strelka record |
| 4 | the format's version u8 | how a reader reads the rest |
| 5 | the kind u8, 0x01 | the head |
| 6–7 | the session u16 | Povodok's number, the same on every Belka of one examination |
| 8 | the Belka's number u8 | 1–63, its slot in the set |
| 9–16 | the chip's ID u64 | the unit itself, for its calibration and its repairs |
| 17–19 | the firmware u8 × 3 | major, minor, patch |
| 20–23 | the firmware's build | the short hash of its source, the very code that wrote the record |
| 24–25 | the rate into memory u16, Hz | a step's time |
| 26 | N u8 | the samples a step, 1 raw |
| 27 | the accelerometer's range u8, ±g | the steps turned into g |
| 28–29 | the gyro's range u16, ±°/s | the steps turned into °/s |
| 30–33 | the period of the first step u32 | when the Belka began to record |
| 34–41 | the date and time u64, µs since 1970, UTC | from the phone, 0 unknown |
| 42–45 | the period of that time u32 | a period's number turned into the date and time |
| 46–47 | the phone's time zone i16, minutes from UTC | the examination's local time |
| 48 | the clock u8 | 0 steered by the SYNC, 1 the fallback, the steps off the grid |
| 49–60 | the accelerometer's calibration i16 × 6 | its raw readings with x up and down, y up and down, z up and down |
| 61–66 | the gyro's scale u16 × 3 | x, y, z from the turns between the six positions, 32 768 for 1,000 |
| 67–72 | the gyro's zero i16 × 3 | x, y, z at rest, warm, raw |
| 73–74 | the calibration's temperature i16, 0,01 °C | |
| 75–78 | the calibration's time u32, s since 1970 | how old it is |
| 79–110 | the Belka's place on the animal, 32 B | as the user wrote it, UTF-8, zeros after |
| 111–142 | the file's own name, 32 B | as first written, zeros after |
| 143–507 | free, 365 B | zeros |
| 508–511 | CRC-32 | |

**The calibration** is made on the table before every examination, after the warm-up, the units
turned onto each face of their boxes and left still a moment on each: the accelerometer's
readings with each axis up and down give its zero, their middle, and its scale, half their span;
the gyro's readings at rest give its zero, and its turns between the positions, against the
angle the accelerometer saw, give its scale. The application computes it from the live view and
writes it into each Belka; the Belka stores the raw readings and computes nothing.

## A data block

Whole bytes, no bits:

| bytes | the field | what for |
|---|---|---|
| 0 | the kind u8, 0x02 | a data block told from an information block |
| 1 | the steps in it u8 | 40, fewer in the session's last block |
| 2–3 | the session u16 | a block known even out of its file |
| 4–7 | the block's number u32 | one count for the session's data and information blocks, the first data block 0; a gap is a block lost |
| 8–11 | the period of its first step u32 | the set's common time, the same instant on every Belka |
| 12 | the block's place in the period u8 | from 4 kHz more blocks than one fall in a period, 16 at 32 kHz; which of them |
| 13 | the steps missing u8 | their six values −32 768 |
| 14 | the steps with the accelerometer out of range u8 | at ±32 g |
| 15 | the steps with the gyro out of range u8 | at ±4 000 °/s |
| 16–19 | the first step's offset i32, ns | where its data-ready fell, less its place on the grid: the clock's check, and the fallback's ground |
| 20–27 | free, 8 B | zeros |
| 28–507 | 40 steps × 12 B | the accelerometer's x, y, z, then the gyro's x, y, z, i16 — the samples, or the means of N |
| 508–511 | CRC-32 | |

The head is 28 B, the steps 480 B: a block covers 0,8 s at 50 Hz, 0,4 s at 100 Hz.

## An information block

After the three data blocks it speaks for, 120 steps:

| bytes | the field | what for |
|---|---|---|
| 0 | the kind u8, 0x05 | an information block told from a data block |
| 1 | the steps it speaks for u8 | 120, fewer in the session's last |
| 2–3 | the session u16 | as in a data block |
| 4–7 | the block's number u32 | the same count as the data blocks', the next number after the three it speaks for |
| 8–11 | the period of its first step u32 | the set's common time |
| 12–13 | the sensor's temperature i16, 0,01 °C | the gyro's bias followed as the unit warms on the animal |
| 14–15 | the cell u16, mV | |
| 16–27 | free, 12 B | zeros |
| 28–507 | 120 steps × 4 B | each step's data-ready offset i16, ns; the SYNC's signal i8, dBm, −128 none heard; a byte, its high nibble the Povodok whose SYNC was taken, 0–3, F none, its low nibble the SYNCs heard in the period, 0–4 |
| 508–511 | CRC-32 | |

The offset is each step's own; the signal and the SYNCs are its period's, the same for the steps of
one period above 50 Hz. The entry's offset reaches ±32 µs, the clock's check under the SYNC; under
the fallback, with the steps off the grid, the entries saturate, and the steps are put onto the grid
from the data blocks' first offsets, which are whole.

## The set's table

Right after the head, as many blocks as it needs: what the user filled in the application, as
CSV — plain text, opened by any spreadsheet and read by eye — the same in every Belka's file, so
that any one record says where every unit of the set sat:

| bytes | the field | what for |
|---|---|---|
| 0 | the kind u8, 0x04 | the set's table |
| 1 | the part u8 | 0 first; a longer table goes on in the next blocks |
| 2–3 | the session u16 | |
| 4–5 | the table's whole length u16, B | where the text ends in its last part |
| 6–507 | 502 B of the text | the last part padded with zeros |
| 508–511 | CRC-32 | |

**The text**: UTF-8, its fields parted by a semicolon, so that a spreadsheet set to a decimal
comma opens it as it is; a field holding a semicolon, a quote or a new line goes in quotes, as
RFC 4180 has it. First the animal, a line a field, its name and its value; then the Belkas, a
line each under a line naming the columns. The session's settings are the head's, not here; the
fields themselves are the application's to offer:

```
druh;kůň
zařazení;sportovní, parkur
jméno;Bára
hmotnost;520 kg
výška;165 cm
anamnéza;"Od jara kulhá vpravo vpředu, víc na tvrdém.
Klusána na rovině a na kruhu."
belka;cip;misto
1;0123456789ABCDEF;hlava, nánosník
2;0123456789ABCDF0;kohoutek
3;0123456789ABCDF1;křížová kost
```

## The end

The file's last block, written when the session ends; a pause of ten seconds without a SYNC
writes none, the record going on in the same session and the gap seen in the periods:

| bytes | the field | what for |
|---|---|---|
| 0 | the kind u8, 0x03 | the end |
| 1 | why u8 | 1 the SYNC's state said stop, 2 switched off by magnet A, 3 the cell low, 4 the card full, 5 a fault, 6 switched off by the application ([`COMMANDS.md`](COMMANDS.md)) |
| 2–3 | the session u16 | |
| 4–7 | the steps u32 | the record's span, a check that the file is whole |
| 8–11 | the blocks u32 | the same, in blocks |
| 12–15 | the period of the last step u32 | when it ended, in the set's time |
| 16–19 | the blocks dropped u32 | lost when the card paused longer than its buffer bears; 0 as a rule |
| 20–507 | free | zeros |
| 508–511 | CRC-32 | |

A file without its end — the cell pulled out — is read to its last block whose CRC holds; its
length is saved every 3 s, so 3 s at most are lost.
