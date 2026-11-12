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
tool checks it; the main core computes it ([`FIRMWARE.md`](FIRMWARE.md), *The record*):

```
[the session's head] [the set's table]  D D D I  D D D I  …  [the end]
                                        D D D I H H          the hoof's both channels
```

- **The session's head**, one block: who, when, how fast, and the groups of the data — each one's
  name, unit, step, range and calibration — below.
- **The set's table**, one block or more: what the user filled in the application — where each
  Belka sits, the units of the session and their kinds, the animal, the notes — the same in every
  Belka's file.
- **D, a data block**: the sensor's steps, below.
- **I, an information block**, after every 120 steps — three data blocks, at any rate, so that
  it takes the same share of every record: what the clock and the radio did for each of those
  steps, the temperature and the cell, below.
- **H, a high-g block**, two after the information block where the session keeps the hoof's
  high-g channel beside the low-g: its 120 steps, 60 in each, below.
- **The end**, one block: why the record ended and how many steps it holds.

**The rhythm is fixed**: D D D I for every 120 steps, and whatever more a session keeps comes after
the I of the same 120 — so that a cycle is read alone, and a reader that knows not a kind of block
skips it by its kind.

A file stops under 2 GiB and the record goes on in its next part, `S00007B03_2.STRK`, begun with
the head again ([`FIRMWARE.md`](FIRMWARE.md), *The record*). Kryska's SD NAND is the same card,
soldered: the same files ([`KRYSKA.md`](KRYSKA.md), *The memory*).

## The session's head

The file's first block, written at the start: a fixed part of 64 B, then the table of the data's
groups.

| bytes | the field | what for |
|---|---|---|
| 0–3 | `STRK` | the mark of a Strelka record |
| 4 | the format's version u8, 4 | how a reader reads the rest |
| 5 | the kind u8, 0x01 | the head |
| 6–7 | the session u16 | Povodok's number, the same on every Belka of one examination |
| 8 | the Belka's number u8 | 1–63, its slot in the set |
| 9–16 | the chip's ID u64 | the unit itself, for its calibration and its repairs |
| 17–19 | the firmware u8 × 3 | major, minor, patch |
| 20–21 | the rate into memory u16, Hz | a step's time |
| 22 | N u8 | the samples a step, 1 raw |
| 23 | the clock u8 | 0 steered by the SYNC, the way kept in the code; 1 the fallback, the steps off the grid; 2 the sensor on its own clock, its samples filtered onto the grid, every sensor now ([`FIRMWARE.md`](FIRMWARE.md), *The sensor and its clock*) |
| 24–31 | the date and time u64, µs since 1970, UTC | from the phone, 0 unknown |
| 32–35 | the period of that time u32 | a period's number turned into the date and time |
| 36 | the phone's time zone i8, quarter hours from UTC | the examination's local time; a zone off the whole hour kept whole, as 3GPP TS 23.040 keeps it in its time stamps (9.2.3.11) |
| 37–52 | the sensor's name, 16 B | ASCII, zeros after: `LSM6DSV320X` |
| 53–54 | the calibration's temperature i16, 0,01 °C | |
| 55–58 | the calibration's time u32, s since 1970 | how old it is; 0 none |
| 59 | the groups u8 | how many follow, 12 at most |
| 60–63 | free, 4 B | zeros |
| 64–495 | the groups, 36 B each | what the blocks hold, below |
| 496–507 | free | zeros |
| 508–511 | CRC-32 | |

**A group** is a run of values in every step of one kind of block — the accelerometer's three in a
data block, the SYNC's signal in an information block — described whole, so that the record is
read without a table of chips, in ten years or in fifty:

| bytes | the field | what for |
|---|---|---|
| 0 | the block's kind u8 | where its values lie: 2 a data block, 5 an information block, 6 a high-g block |
| 1 | the first u8 | its first value's place in a step, counted in values |
| 2 | the count u8 | its values |
| 3 | the type u8 | 1 i16, 2 i8, 3 u8 |
| 4–11 | the name, 8 B | ASCII, zeros after |
| 12–15 | the unit, 4 B | ASCII, zeros after; none for a byte of fields |
| 16–19 | the step u32, nano-units | a value times it in the unit: the sensor's sheet's own, 488 000 nano-g at ±16 g on an LSM, 10 417 000 at ±320 g; 10⁹ for a value counted in whole units, 0 for a byte of fields |
| 20–21 | the range u16, as set | what the sheet promises: 320 at ±320 g, though 32 768 steps make 341 |
| 22–23 | the calibration's range u16 | the range it was made at; 0 the session's own |
| 24–35 | the calibration, 6 × 16 bits | an accelerometer's raw readings with x up and down, y up and down, z up and down, i16; a gyro's zero at rest, x, y, z, i16, then its scale, x, y, z, u16, 32 768 for 1,000; zeros, none |

A Belka's groups, in this order:

| the name | the block | the first, the count | the type | the unit | what |
|---|---|---|---|---|---|
| `ACC` | 2 | 0, 3 | i16 | g | the accelerometer's x, y, z; `ACC_HG` when the session's range is the LSM6DSV320X's high-g channel alone |
| `GYRO` | 2 | 3, 3 | i16 | dps | the gyro's x, y, z |
| `OFFSET` | 5 | 0, 1 | i16 | ns | each step's data-ready offset |
| `SIGNAL` | 5 | 1, 1 | i8 | dBm | the SYNC's signal, −128 none heard |
| `RADIO` | 5 | 2, 1 | u8 | — | the Povodok whose SYNC was taken, the high nibble, and the SYNCs heard, the low |
| `ACC_HG` | 6 | 0, 3 | i16 | g | the hoof's high-g channel beside the low-g, both kept |

A reader finds a group by its name and its unit and turns a value into the unit by the group's
step alone; one it knows not, it skips. A channel a later Belka adds is a group more, the format
the same — EDF, the format of clinical recordings, describes each of its signals so, its label,
its physical dimension and its range in the file's own header
([EDF](https://www.edfplus.info/specs/edf.html)). The units of the session, their kinds and each
Belka's place are in the set's table, the first step's period in the first data block.

**The calibration** is made on the table before every examination, after the warm-up, the units
turned onto each face of their boxes and left still a moment on each: the accelerometer's
readings with each axis up and down give its zero, their middle, and its scale, half their span;
the gyro's readings at rest give its zero, and its turns between the positions, against the
angle the accelerometer saw, give its scale. The Belka computes it itself, face by face as the
application asks (64/22), and keeps the raw readings, each group its own: the hoof's high-g
channel is calibrated from the same six faces at once, beside the low-g. A session in ranges other
than the calibration's takes them by the nominal steps: the readings times the calibration's range
over the session's.

## A data block

Whole bytes, no bits:

| bytes | the field | what for |
|---|---|---|
| 0 | the kind u8, 0x02 | a data block told from an information block |
| 1 | the steps in it u8 | 40, fewer in the session's last block |
| 2–3 | the session u16 | a block known even out of its file |
| 4–7 | the block's number u32 | one count for the session's data and information blocks, the high-g blocks taking none, the first data block 0; a gap is a block lost |
| 8–11 | the period of its first step u32 | the set's common time, the same instant on every Belka |
| 12 | the block's place in the period u8 | from 4 kHz more blocks than one fall in a period, 16 at 32 kHz; which of them |
| 13 | the steps missing u8 | their six values −32 768 |
| 14 | the steps with the accelerometer out of range u8 | at the session's range, ±2 g to ±320 g; with both channels, the low-g's |
| 15 | the steps with the gyro out of range u8 | at the full scale |
| 16–19 | the first step's offset i32, ns | where its data-ready fell, less its place on the grid: the clock's check, and the fallback's ground |
| 20–27 | free, 8 B | zeros |
| 28–507 | 40 steps × 12 B | the accelerometer's x, y, z, then the gyro's x, y, z, i16 — the samples, or the means of N |
| 508–511 | CRC-32 | |

The head is 28 B, the steps 480 B: a block covers 0,8 s at 50 Hz, 0,4 s at 100 Hz. The x, y, z are
the sensor's own, the same on every build: both sensors' axes coincide on the board's land, +Z out
of the sensor's top, +1 g on the axis pointing up, a turn positive by the right-hand rule
([`BOARD.md`](BOARD.md), *U7*).

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

## A high-g block

Where the session keeps both of the hoof's accelerometer channels, the low-g in the data blocks and
the high-g beside it ([`COMMANDS.md`](COMMANDS.md), 64/1's `hg`), two of these follow each
information block, the first half of its 120 steps and the second. They take no number of their
own: each carries the number of its cycle's first data block.

| bytes | the field | what for |
|---|---|---|
| 0 | the kind u8, 0x06 | a high-g block |
| 1 | the steps in it u8 | 60, fewer in the session's last |
| 2–3 | the session u16 | as in a data block |
| 4–7 | the number of its cycle's first data block u32 | the cycle it belongs to, a block lost known by it |
| 8–11 | the period of the cycle's first step u32 | the set's common time |
| 12 | that step's place in the period u8 | as a data block's |
| 13 | the half u8 | 0 the cycle's first 60 steps, 1 the second |
| 14 | the steps out of range u8 | at the high-g channel's range |
| 15–27 | free, 13 B | zeros |
| 28–387 | 60 steps × 6 B | the high-g channel's x, y, z, i16, its group `ACC_HG`; a step lost all three −32 768 |
| 388–507 | free | zeros |
| 508–511 | CRC-32 | |

A hoof's record is half as large again with both channels: six blocks for 120 steps where it was
four.

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
line each under a line naming the columns: its number, its chip, its place's code and name, and
its kind — `B` a Belka, `K` Kryska, `Z` Zvjozdočka, `B` where the column is none; the Belkas listed
are the session's units. The session's settings are the head's, not here; the fields themselves
are the application's to offer. The fields' names, the kind, the places' codes
and their names are English in every language, as research has them; what the user wrote stays as
written:

```
kind;horse
use;sport, show jumping
name;Bára
weight;520 kg
height;165 cm
history;"Od jara kulhá vpravo vpředu, víc na tvrdém.
Klusána na rovině a na kruhu."
belka;chip;code;place;kind
1;0123456789ABCDEF;HD;head;B
2;0123456789ABCDF0;WI;withers;B
3;0123456789ABCDF1;PE;pelvis;K
```

## The end

The file's last block, written when the session ends; a pause of ten seconds without a SYNC
writes none, the record going on in the same session and the gap seen in the periods:

| bytes | the field | what for |
|---|---|---|
| 0 | the kind u8, 0x03 | the end |
| 1 | why u8 | 1 the SYNC's state said stop, 2 switched off by magnet A, 3 the cell low, 4 the card full, 5 a fault, 6 switched off by the application ([`COMMANDS.md`](COMMANDS.md)), 7 the set silent: paused, and every Povodok gone for three times the switching-off's setting ([`FIRMWARE.md`](FIRMWARE.md), *The power*) |
| 2–3 | the session u16 | |
| 4–7 | the steps u32 | the record's span, a check that the file is whole |
| 8–11 | the blocks u32 | the same, in blocks |
| 12–15 | the period of the last step u32 | when it ended, in the set's time |
| 16–19 | the blocks dropped u32 | lost when the card paused longer than its buffer bears; 0 as a rule |
| 20–507 | free | zeros |
| 508–511 | CRC-32 | |

A file without its end — the cell pulled out — keeps the room it took at its start
([`FIRMWARE.md`](FIRMWARE.md), *The record*): it is read to its last block whose CRC holds, and a
block of another session, which an earlier file left in that room, is skipped by its session.
What is written is saved to the card every 3 s, so 3 s at most are lost, and the Belka cuts such a
file to its last block of its own when it next starts.
