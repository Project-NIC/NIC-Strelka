<div align="center">

# Kryska's memory

**A closed chapter: NOR flash or an SD NAND, both ways written whole, the SD NAND taken.**

↑ [NIC-Strelka](../README.md) · [Kryska](../KRYSKA.md) · [the chapters](README.md)

</div>

---

Kryska's memory is on its board, no card's slot. It was first NOR flash, Winbond's W25Q01JV, the
record raw on it and its own writer; then an SD NAND, an SD card's NAND and controller in one
package, soldered in the slot's place. Both ways are written here whole, so that either can be
built; **Kryska takes the SD NAND**, and the NOR way stays as it was left: its board, its firmware
built, its record on the flash tested on the host, its disk for a computer still to come.

## The two side by side

| | NOR flash, Winbond's W25Q01JV | SD NAND, CS's CSNP1GCR01-BOW |
|---|---|---|
| what | NOR flash of 1 Gbit, two dies of 512 Mbit | NAND flash of 1 Gbit and an SD controller, its bad blocks and ECC its own |
| the package | WSON-8, 8 × 6 mm | LGA-8, 8 × 6 × 0,75 mm |
| holds | 128 MB | 128 MB |
| works | −40 to +85 °C | −30 to +85 °C |
| writes | a page of 256 B in 0,7 ms, 3,5 ms at the longest | the bus's 8 Mbit/s in SPI mode, the controller's own pace behind it |
| erases | a block of 64 KB in 150 ms, 2 s at the longest; the whole chip 200 s, 1 000 s at the longest | its own, inside; a format as a card's |
| draws | 20–25 mA programming, 1 µA in power-down | 15–25 mA at work, 150–200 µA standing, switched off by U6 between the records |
| to the firmware | a writer of its own, the record raw, a file system of its own for the sessions | the Belka's card: Zephyr's SD stack, FatFs, the USB disk |
| to a computer | SMP on the serial port; a disk made up of the record still to come | a disk on the cable, as a Belka's |

The NOR flash's figures are its sheet's, Rev E of March 2024 (1; 7.4; 7.6); the SD NAND's its own,
V1.1 of January 2021 (1.1–1.2; 3; 4).

**The current, rough**, by the model of [`PRINCIPLE.md`](../PRINCIPLE.md), *The current*: the
memory's own while a record runs, and the hours each 100 mAh lasts with the rest of the unit — the
SD NAND switched on for the record, the NOR flash programming its pages for their share of the
time. Preliminary, the kit measures it:

<!-- calc:kryska-memories -->
| a record | NOR flash, its own | SD NAND, its own | the unit, NOR | the unit, SD NAND |
|---|---|---|---|---|
| 400 Hz | 0,4 mA to 2,5 mA | 0,3 mA to 0,5 mA | 16–37 h | 23–39 h |
| 2 kHz | 1,9 mA to 12 mA | 0,7 mA to 1,9 mA | 6–24 h | 18–34 h |
| 400 Hz, both channels | 0,6 mA to 3,6 mA | 0,3 mA to 0,7 mA | 13–35 h | 22–39 h |
| 2 kHz, both channels | 2,8 mA to 18 mA | 0,9 mA to 2,7 mA | 5–20 h | 15–31 h |
<!-- /calc -->

The SD NAND's standing current is paid only while a record runs, U6 switching it off between, as
the Belka's card; its work is short, the bus carrying the record's bytes in a few per cent of the
time, where the NOR flash programs its small pages for long. So the SD NAND draws less while it
records, not more.

## Why the SD NAND

- **One firmware and one board's memory with the Belka.** The SD NAND is the Belka's card to the
  firmware, on the card's pins and its switched supply: the same driver, the same file system, the
  same room taken for a file in one run and cut after a power cut, the same disk on the cable, the
  same `STRELKA.SET` ([`FIRMWARE.md`](../FIRMWARE.md), *The record*; *USB*). What Kryska alone
  carried — its writer, its own file system for the sessions, the download by SMP on the serial
  port, the disk still to come — is not needed.
- **A memory with a file system of its own.** A computer reads Kryska as it reads a Belka; the NOR
  flash shows its record only through a disk made up of it, whose tables Kryska would compute as
  they are read, a file system holding the record raw being too slow on NOR: it erases as it
  writes ([`WHY.md`](../WHY.md), *The parts*).
- **A format in seconds, not minutes.** The NOR flash is erased whole in 200 s, 1 000 s at the
  longest, the unit to stay on throughout; the SD NAND is formatted as a card is.
- **Fewer ways to go wrong.** A memory written only into room erased beforehand, a long erase to
  finish, sessions found by reading backwards, the record leaving by a serial port: each is a step
  a user can get wrong, and code to keep; the card's way is one the Belka already has.

What it costs, accepted: the standing current while it is on, paid only during a record; −30 °C at
the coldest, not −40; U6, R12–R16, C16, C18 and C29 more on Kryska's board, the Belka's own parts
([`BOARD.md`](../BOARD.md), *Kryska*).

## The NOR way, whole

For whoever builds Kryska with NOR flash: its board, its firmware and its record, as they were
left. The firmware builds as the Belka's with Kryska's NOR overlay and configuration:

    west build -b nrf54lm20dk/nrf54lm20a/cpuapp --sysbuild firmware/belka -- \
        -Dbelka_EXTRA_CONF_FILE=kryska_nor.conf \
        -Dbelka_EXTRA_DTC_OVERLAY_FILE=boards/kryska_nor.overlay

`firmware/belka/src/flash_rec.c`, the writer behind the card's interface, and
`firmware/common/flashrec.c`, the sessions found on the flash, checked on a simulated memory by
`tools/test_firmware.py`. On the board, the SD NAND's place: U3 the W25Q01JV, R25–R27 and C28 in
place of U6, R12–R16, C16, C18 and C29, the card's nets FL_CS, FL_DO, FL_DI and FL_CLK on the same
pins, its supply +3V0 itself.

- **The record raw on the flash, no file system**: the blocks of [`RECORD.md`](../RECORD.md), each
  session's head first, written one after another from where the last session ended, by
  Kryska's own writer; a file system erases as it writes, which at 4 kHz the flash cannot keep
  up with ([`WHY.md`](../WHY.md), *The parts*). Every block carries its session, its number and its
  CRC, so a power cut leaves the blocks before it whole, read as the card's are. The whole memory
  is erased by a chip erase, 200 s and 1 000 s at the longest (the W25Q01JV's sheet, 7.6), so
  after a download on the cable or in the dock. How it lies in the memory, how the sessions are
  found and read, and how they leave by the cable: *The record on the flash*, below.

<!-- calc:kryska-nor-memory -->
Its memory holds 128 MB: 5,5 h of the record at 400 Hz, 33 min at 4 kHz, the record's bytes a step
and its heads the Belka's ([`RECORD.md`](../RECORD.md)). At 4 kHz the flash programs its pages 19 %
of the time at the sheet's typical 0,7 ms a page, 93 % at its longest, 3,5 ms. With both of the
hoof's accelerometer channels kept, a third less: 3,6 h at 400 Hz, 22 min at 4 kHz, the pages
programmed 28 % of the time, 140 % at the longest — more than the flash has; so Kryska keeps both up
to 2 kHz, 70 % there at the longest, and refuses them above it.
<!-- /calc -->

- **The memory: Winbond's W25Q01JV**, NOR flash of 1 Gbit at 3 V, WSON-8 of 8 × 6 mm, LCSC
  C6926235 ([DigiKey](https://azcus.digikey.com/en/products/detail/winbond-electronics/W25Q01JVZEIQ/12143374)),
  on Zephyr's SPI NOR driver; beside the module's body, never under its antenna. From its sheet,
  Rev E of March 2024: 2,7–3,6 V; 20 mA programming a page, 25 mA at most; 1 µA in power-down,
  40 µA at most to 85 °C (7.4); a page of 256 B in 0,7 ms, 3,5 ms at the longest, a block of 64 KB
  erased in 150 ms, 2 s at the longest (7.6); 100 000 erases (1). Erased before the record, it
  only programs while recording. **Two dies of 512 Mbit in one package**, each with its own BUSY
  and SUS bits, the status read giving the active die's alone, chosen by Software Die Select, C2h
  (its sheet, 1 and 6; 6.3.3); the power-down, B9h, and its release, ABh, are commands both dies
  take (6.3.3), and a chip erase erases both (6.4.31). Linux's driver therefore selects each
  die in turn and reads its status before the next command, some 30 µs a die, the dies observed
  up to 200 µs apart, and writing as fast as without it
  ([linux-mtd, 2025](https://lkml.indiana.edu/2501.1/04470.html)); Zephyr's SPI NOR driver in
  v3.4.0 has no die select, so Kryska's own writer waits on every die the same way (*The record
  on the flash*, below).

### U3, the flash, on the board

**Winbond's W25Q01JVZEIQ**, NOR flash of 1 Gbit, WSON-8 of 8 × 6 mm (C6926235), beside the
module's body, never under its antenna, on SPIM20, the card's pins on the Belka. The pads by its
sheet, Rev E, 2.2:

| pad | name | net |
|---|---|---|
| 1 | /CS | FL_CS, P3.10; R25 100 kΩ to +3V0 |
| 2 | DO (IO1) | FL_DO, P3.09 |
| 3 | /WP (IO2) | FL_WP, R26 10 kΩ to +3V0 |
| 4 | GND | GND |
| 5 | DI (IO0) | FL_DI, P3.08 |
| 6 | CLK | FL_CLK, P3.03, a clock pin |
| 7 | /HOLD (IO3) | FL_HOLD, R27 10 kΩ to +3V0 |
| 8 | VCC | +3V0; C28 100 nF |

**R25** holds /CS high while the processor is held in reset or programmed: "The /CS input must
track the VCC supply level at power-up and power-down … If needed a pull-up resistor on the /CS
pin can be used" (3.1). **R26 and R27** hold /WP and /HOLD high: the IQ part has its Quad Enable
bit fixed at 1, "the Quad IO2 and IO3 pins are enabled, and /HOLD function is disabled" (6.1.9;
7.6), the chip driving them only in the quad instructions, which the firmware does not use on
SPIM20; a resistor, not a track, so that a quad instruction sent by mistake meets no short. Its
supply is +3V0 itself, no switch: 1 µA in power-down ([`KRYSKA.md`](../KRYSKA.md)).

### The record on the flash

The record is raw on the flash, no file system: the blocks of [`RECORD.md`](../RECORD.md) as a card
holds them, 512 B each, written one after another from the memory's first byte into room erased
beforehand, each session right after the last — its head, the set's table, the data and the
information blocks, its end. A block is two pages of the flash, programmed one after the other.

<!-- calc:kryska-sessions -->
The memory is 262 144 blocks; its list of sessions holds 1 024, the newest, which over 128 MB is a
session of 19 s at 400 Hz at the least — the older ones wait for the erase.
<!-- /calc -->

- **An erased block reads FFh throughout.** The written blocks are one run from block 0, so the
  first erased block is found at the start by halving, 18 reads of a block, and the next session
  begins there. A record runs until the memory is full, its end saying *the card full*, and the
  light shows the fault, as on a Belka ([`FIRMWARE.md`](../FIRMWARE.md), *The record*).
- **The power gone in the middle of a page** leaves a block that is not erased and whose CRC does
  not hold: it is passed over, the next session begins after it, and the session it cut off is
  read to its last whole block, as a Belka's file is; nothing is written at the next start. Such
  a page may hold bits weakly programmed: a block reading FFh throughout is written over by the
  next session, and at the worst that one block is lost.
- **The sessions are found backwards** from the first erased block, at the start: a session's
  last block is its end, which counts its data and information blocks, the dropped among them
  ([`RECORD.md`](../RECORD.md), *The end*), or, the power gone before the end, a data or information
  block, which carries its number; the head is where they say, checked by its mark and its
  session, and found block by block where the count was wrong. Some three reads a session. The
  list is kept in memory as sessions are added.
- **The sessions are files**, read alone, on a file system of Kryska's own registered in Zephyr and
  mounted where a Belka's card is, `/SD:`, named as the card's, `S00007B03.STRK`; since nothing
  here is overwritten, a session of the same number and Belka again is `S00007B03_2.STRK`, `_3`
  and on. So the download, 8/0–8/2, the sessions' list, 64/2, and the application see a Belka's
  card ([`COMMANDS.md`](../COMMANDS.md)). A file is opened for the download only while no record
  runs; a record begun while a download reads goes on beside it, the reads and the writes taking
  turns on the flash.
- **The whole memory is erased** by the application's command, 64/5, erased or formatted alike:
  the chip erase, run by the writer while the unit says *erasing* and refuses a record, after a
  download on the cable or in the dock. Nothing else erases anything.
- **The writer** is the Belka's behind the card's interface (`firmware/belka/src/flash_rec.c`): the
  blocks through the same buffer of 128 KB and a thread of its own below the sensor and the
  radio, each block programmed through Zephyr's SPI NOR driver, which waits for the die the page
  is in, the status read following the instruction before it (the sheet, 6.3.2). What the driver
  has not, the writer sends over the same SPI: after the chip erase, which the driver awaits on
  one die alone, each die selected by C2h and its BUSY read until both are idle, as Linux's driver
  does (linux-mtd, January 2025), and the same before the power-down, B9h, which both dies take,
  sent once nothing has used the memory for three seconds, 1 µA (7.4); the release, ABh, and 3 µs
  before anything uses it again (7.6, tRES1). The sessions found on a simulated memory — closed
  ones, one cut off by the power, blocks dropped, a page half written, the same number again —
  are checked on the host, `firmware/common/flashrec.c` by `tools/test_firmware.py`.
- **On the cable, SMP on the serial port** carries what a Belka's disk carries: the download by 8/0,
  and the set given as the same `STRELKA.SET` a Belka takes from its disk, written by 8/0's upload
  into the record's file system — allowed where a Belka's disk would be open, on a new Kryska or one
  opened by 64/5 `do` 3 — and taken as from the disk: read, kept, wiped and deleted, the light green
  or red for 2 s ([`FIRMWARE.md`](../FIRMWARE.md), *USB* and *What a unit keeps*). The frames go in
  Zephyr's SMP over console framing, beside the console's own lines, which a client passes over;
  there is no lock on the cable — whoever holds the cable holds the unit, as whoever holds a magnet
  does. A phone has no serial port: the set and the download by the cable are a computer's,
  `tools/smp.py`, and the phone's way is the radio. A disk made up of the record, for a computer's
  file manager and the dock, is still to come, should this way be built.

Within this way, weighed and left:

- **A file system on Kryska's flash, FAT or LittleFS.** Either erases as it writes, a sector of
  4 KB at a time, 50 ms and 400 ms at the longest (the W25Q01JV's sheet, 7.6). At 4 kHz the
  record's some 68 KB a second would ask some 17 erases a second, some 0,85 s of erasing beside
  0,19 s of programming its pages; and FAT writes its table again at every save, at the card's
  3 s 1 200 times an hour on one sector, whose 100 000 erases (the sheet's features) would last
  some 83 h of recording. The record goes raw into memory erased beforehand (above).
- **An index of the sessions on Kryska's flash**, entries programmed into a block kept for it as
  each session ends. The sessions are found backwards from the first erased block by the counts
  the blocks carry, some three reads a session, and an index would be programmed into a page
  written before, which NOR takes but which there is no need to lean on.

## Another weighed: SLC NAND

Winbond's W25M02GV, two dies of SLC NAND of 1 Gbit in the same WSON-8 of 8 × 6 mm, 256 MB, −40 to
+85 °C, programming a page of 2 KB in 0,25 ms, 0,7 ms at the longest, and erasing a block of 128 KB
in 2 ms, 10 ms at the longest, its ECC and its table of bad blocks on the chip (its sheet, Rev I of
May 2021, 1; 9.2; 9.6). Fast, and twice the room; but Zephyr has no NAND driver, its bad blocks are
the writer's to pass over, and a computer would still see it only through a disk made up of it — the
NOR way's work again, with a driver more.
