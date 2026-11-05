<div align="center">

# Kryska

**The small Belka: for small animals, a small dog's head too.**

↑ [NIC-Strelka](README.md)

</div>

---

Kryska measures as a Belka does, speaks Strelka's frame and ESB with SMP as a Belka does, runs the
Belka's firmware, and joins the same set as Belkas and Povodoks ([`PRINCIPLE.md`](PRINCIPLE.md)).
Kryska takes the ordinary examinations; the Belka stays the full unit, for the large animals and
the raw rates, its large cell, its card and 32 kHz. Kryska gives up the high rates and the card for
a board as small as can be made. The name is one of the two rats that flew with Belka and Strelka
([`NAMES.md`](NAMES.md)).

## The order of the design

1. **The board first**: the module at one edge, its antenna over the edge or over board with no
   copper on either layer, and nothing over it (Fanstel; [`PARTS.md`](PARTS.md), *The Belka*);
   the rest of the parts beside the module's body, on either side of the board.
2. **Then the cell**, as large as the board leaves outside the antenna.
3. **Then the box**, a Korabl of Kryska's own size round the two, built sturdy as standard.

Its weight follows from the board and the cell, and is not set before them.

## What it must do

- **It records into memory at 400 Hz in ordinary use, at 4 kHz at the most**, any of the Belka's
  rates up to it, its record the Belka's ([`RECORD.md`](RECORD.md)); the live view as the
  Belka's.
- **Its memory is on the board**, NOR flash of 128 MB at the most, **and records only into room
  erased beforehand**: the memory is erased after its download, at the application's word, and a
  record runs until the memory is full and stops there, never waiting on an erase; how long a
  record may run is the room the user erased, not a card's that is written over.
- **The record raw on the flash, no file system**: the blocks of [`RECORD.md`](RECORD.md), each
  session's head first, written one after another from where the last session ended, by
  Kryska's own writer; a file system erases as it writes, which at 4 kHz the flash cannot keep
  up with ([`WHY.md`](WHY.md), *The parts*). Every block carries its session, its number and its
  CRC, so a power cut leaves the blocks before it whole, read as the card's are. The whole memory
  is erased by a chip erase, 200 s and 1 000 s at the longest (the W25Q01JV's sheet, 7.6), so
  after a download on the cable or in the dock. How it lies in the memory, how the sessions are
  found and read, and how they leave by the cable: *The record on the flash*, below.

<!-- calc:kryska-memory -->
Its memory holds 128 MB: 5,5 h of the record at 400 Hz, 33 min at 4 kHz, the record's bytes a step
and its heads the Belka's ([`RECORD.md`](RECORD.md)). At 4 kHz the flash programs its pages 19 % of
the time at the sheet's typical 0,7 ms a page, 93 % at its longest, 3,5 ms.
<!-- /calc -->

- **It bears shocks as it is built, with no build of its own for them**: the force a shock puts on
  a unit's parts and its fastening grows with its mass, F = m · a, so a small, light unit holds
  with a simple sturdy Korabl where a Belka for large shocks, Ugoljok, asks a build of its own; with
  its LSM6DSV320X, ±320 g, every Kryska serves the shocks' examinations too, as long as its memory
  lasts.
- **USB on its contact for everything a Belka's carries**: the charge, the record, the commands,
  the set and the firmware — the record and the set by SMP on its serial port, a disk made up of
  the record still to come ([`FIRMWARE.md`](FIRMWARE.md), *USB — the unit on the cable*; *The
  record on the flash*, below).
- **It runs and charges on the cable at once**, so that a download goes on while it charges; the
  load before the charge.
- **Off is a true off**: switched on by magnet A held, switched off by the firmware, as a Belka.
  **The cable wakes it but does not switch it on**: woken, it shows the charge on its light for
  some seconds and sleeps again, charging; magnet A held then switches it on.
- **Its cell from a known maker**, with its protection board and an NTC: Li-poly, or LiFePO4 if a
  cell of the size is to be had, the two weighed once the board and its box are drawn — the cell
  itself chosen then, by what the box holds. Li-poly charged to about 80 %, 4,0 V, LiFePO4 to 3,6 V,
  the nPM1304's lowest end, 3,50 V on the nPM1300; used down to about 20 %, the firmware closing the
  record and switching it off there. Charged only between 0 and 45 °C by its NTC, at a tenth of its
  capacity an hour, the charge's length not counted; with room round it in the box, which no animal
  bites through.
- **The same as the Belka's**: the sensor's land ([`BOARD.md`](BOARD.md), *U7*), the record, the
  frame and the firmware. **The land fitted with ST's LSM6DSV320X**, the hoof's sensor, ±16 g on
  its low-g channel and up to ±320 g on its high-g one, so that one build serves every
  examination: it runs on its own clock, up to 7,68 kHz, its samples drawn onto the set's grid,
  4 kHz at the most ([`PARTS.md`](PARTS.md), *The motion sensor*). One Kryska, no other build.

## The way taken

- **The radio: Fanstel's BM20M**, 10,0 × 14,0 mm, the series' small PCB antenna on the Belka's
  land, the same chip, the series' least range ([`PARTS.md`](PARTS.md), *The radio and the
  processor*, Fanstel's figures), where a small animal keeps Povodok within metres.
- **The supply: Nordic's nPM1304, or its nPM1300**, pin for pin the same in QFN32 of 5 × 5 mm
  (Nordic's pin tables of both), a charger, two bucks, two load switches or LDOs, ship mode and a
  fuel gauge in the host's software in one chip
  ([nPM1304](https://www.nordicsemi.com/Products/nPM1304),
  [nPM1300](https://www.nordicsemi.com/Products/nPM1300)): the one JLCPCB has when a run is
  ordered fitted to the whole run, the firmware built for it. The nPM1304 charges at 4–100 mA, for
  the small cell; the nPM1300 at 32–800 mA, a tenth of the capacity an hour from a cell of
  320 mAh up, faster below; LCSC lists the nPM1300, C7501206, and not yet the nPM1304. The figures
  below are the nPM1304's, the nPM1300's alike but for its charge and its end voltage of
  3,50–3,65 or 4,00–4,45 V:
  - its dynamic power path feeds the board from the cable and charges the cell from what is left,
    the battery isolated once full
    ([charger](https://docs.nordicsemi.com/r/bundle/ps_npm1304/page/chapters/charger.html));
  - BUCK2 gives the module 3,0 V from power-on, 150 kΩ on VSET2, no firmware asked
    ([buck](https://docs.nordicsemi.com/r/bundle/ps_npm1304/page/chapters/core_components/buck/doc/frontpage.html));
  - ship mode, 370 nA, is the off: SHPHLD held low for 96 ms, magnet A's Hall switch to ground,
    or the cable's VBUS, leaves it, and the firmware enters it; woken by the cable alone, the
    firmware shows the charge, enables it and puts the processor to sleep, the charger running on
    ([ship mode](https://docs.nordicsemi.com/r/bundle/ps_npm1304/page/chapters/core_components/ship/doc/ship-mode.html));
  - the charger is off from power-on until the firmware enables it, its reset values 4 mA and
    4,0 V; 4–100 mA and 3,6–4,65 V in 50 mV steps by its registers; by the cell's NTC it stops
    under 0 °C and charges at half its current up to 10 °C, and its hot stop, 60 °C from reset, the
    firmware sets to the cell's 45 °C (JEITA, its limits set between −20 and 60 °C; the charger's
    page): a firmware that never starts leaves the cell uncharged, never charged past 4,0 V.
- **The light from the PMIC**: the two-colour LED on two of its LED drivers, 5 mA sinks, no
  resistors, both the charger's from the PMIC's reset — the green lit while it charges, the red for
  its fault — while the processor sleeps, and both taken by the firmware for the Belka's patterns
  ([LEDDRV](https://docs.nordicsemi.com/r/bundle/ps_npm1300/page/chapters/core_components/leddrv/doc/frontpage.html)).
- **The contact**: the Belka's five-pin magnetic pair, USB whole on it — the charge, the record,
  the commands and the firmware.
- **The buck filtered**: in its hysteretic mode, the one at a Kryska's load, it leaves 50 mV of
  ripple, 5 mV in PWM at 4,0 mA more (the buck's page; the product overview, table 2), and the
  module asks a supply free of ripple (Fanstel). On the cell, in the automatic mode, the ripple
  comes of the PMIC's references refreshed: "output capacitance can help slightly, but it won't
  get rid of" it, and Nordic's way is PWM for the time that needs it (DevZone
  [128960](https://devzone.nordicsemi.com/f/nordic-q-a/128960/npm1300-large-vsys-ldo-ripple-in-vbat-mode-with-buck-auto-pfm-but-disappears-in-forced-pwm/570690)).
  The sensor samples all the time, and PWM all the time costs the 4,0 mA: an LC filter after the
  buck feeds the module and the sensor, whatever the radio does, from parts the board already
  carries: the buck's own inductor again, Sunlord's WPN201610H2R2MT (C97005), and a ceramic of
  100 µF in 1206 — its corner, its Q and its peak computed in [`BOARD.md`](BOARD.md), *Kryska*.
  The buck's bursts at a light load come
  below 3,6 MHz, at a rate Nordic does not give: under the corner they pass the filter, and the
  kit measures it ([`STATUS.md`](STATUS.md), *Awaiting measurement*).
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

## The record on the flash

The record is raw on the flash, no file system: the blocks of [`RECORD.md`](RECORD.md) as a card
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
  light shows the fault, as on a Belka ([`FIRMWARE.md`](FIRMWARE.md), *The record*).
- **The power gone in the middle of a page** leaves a block that is not erased and whose CRC does
  not hold: it is passed over, the next session begins after it, and the session it cut off is
  read to its last whole block, as a Belka's file is; nothing is written at the next start. Such
  a page may hold bits weakly programmed: a block reading FFh throughout is written over by the
  next session, and at the worst that one block is lost.
- **The sessions are found backwards** from the first erased block, at the start: a session's
  last block is its end, which counts its data and information blocks, the dropped among them
  ([`RECORD.md`](RECORD.md), *The end*), or, the power gone before the end, a data or information
  block, which carries its number; the head is where they say, checked by its mark and its
  session, and found block by block where the count was wrong. Some three reads a session. The
  list is kept in memory as sessions are added.
- **The sessions are files**, read alone, on a file system of Kryska's own registered in Zephyr and
  mounted where a Belka's card is, `/SD:`, named as the card's, `S00007B03.STRK`; since nothing
  here is overwritten, a session of the same number and Belka again is `S00007B03_2.STRK`, `_3`
  and on. So the download, 8/0–8/2, the sessions' list, 64/2, and the application see a Belka's
  card ([`COMMANDS.md`](COMMANDS.md)). A file is opened for the download only while no record
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
- **On the cable, SMP on the serial port** carries what a Belka's disk carries: the download by
  8/0, and the set given as the same `STRELKA.SET` a Belka takes from its disk, written by 8/0's
  upload into the record's file system — allowed where a Belka's disk would be open, on a new
  Kryska or one opened by magnet B held 3 s or by 64/5 `do` 3 — and taken as from the disk: read,
  kept, wiped and deleted, the light green or red for 2 s ([`FIRMWARE.md`](FIRMWARE.md), *USB* and
  *What a unit keeps*). The frames go in Zephyr's SMP over console framing, beside the console's
  own lines, which a client passes over; there is no lock on the cable — whoever holds the cable
  holds the unit, as whoever holds a magnet does. A phone has no serial port: the set and the
  download by the cable are a computer's, `tools/smp.py`, and the phone's way is the radio. A disk
  made up of the record, for a computer's file manager and the dock, is still to come
  ([`STATUS.md`](STATUS.md)).
