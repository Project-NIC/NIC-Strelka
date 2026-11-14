<div align="center">

# Kryska

**The small Belka: for small animals, a small dog's head too.**

↑ [NIC-Strelka](README.md)

</div>

---

Kryska measures as a Belka does, speaks Strelka's frame and ESB with SMP as a Belka does, runs the
Belka's firmware, and joins the same set as Belkas and Povodoks ([`PRINCIPLE.md`](PRINCIPLE.md)).
Kryska takes the ordinary examinations; the Belka stays the full unit, for the large animals and the
raw rates, its large cell, its card and 32 kHz. Kryska gives up the high rates and the card's slot
for a board as small as can be made. The name is one of the two rats that flew with Belka and
Strelka ([`NAMES.md`](NAMES.md)).

## The order of the design

1. **The board first**: the module at one edge, its antenna over the edge or over board with no
   copper on either layer, and nothing over it (Fanstel; [`PARTS.md`](PARTS.md), *The Belka*);
   the rest of the parts beside the module's body, on either side of the board.
2. **Then the cell**, as large as the board leaves outside the antenna.
3. **Then the box**, a Korabl of Kryska's own size round the two, built sturdy as standard
   ([`KOROLJOV.md`](KOROLJOV.md)).

Its weight follows from the board and the cell, and is not set before them.

## What it must do

- **It records into memory at 400 Hz in ordinary use, at 2 kHz at the most**, any of the Belka's
  rates up to it, its record the Belka's ([`RECORD.md`](RECORD.md)); the live view as the
  Belka's.
- **Its memory is on the board**, an SD NAND of 128 MB soldered where a Belka has its card's slot:
  the Belka's card to the firmware, its file system, its record and its disk on the cable the
  card's (*The memory*, below). The NOR flash it first had, the record raw on it, is written whole
  as Kryska's other way ([`chapters/kryska-memory.md`](chapters/kryska-memory.md)).

<!-- calc:kryska-memory -->
Its memory holds 128 MB: 5,5 h of the record at 400 Hz, 1,1 h at 2 kHz, the record's bytes and its
file the Belka's ([`RECORD.md`](RECORD.md)); with both of the hoof's accelerometer channels kept a
third less, 3,6 h and 44 min.
<!-- /calc -->

- **It bears shocks as it is built, with no build of its own for them**: the force a shock puts on
  a unit's parts and its fastening grows with its mass, F = m · a, so a small, light unit holds
  with a simple sturdy Korabl where a Belka for large shocks, Ugoljok, asks a build of its own; with
  its LSM6DSV320X, ±320 g, every Kryska serves the shocks' examinations too, as long as its memory
  lasts.
- **USB on its contact for everything a Belka's carries**: the charge, the record on its disk, the
  commands, the set as `STRELKA.SET` on the disk and the firmware, as a Belka's
  ([`FIRMWARE.md`](FIRMWARE.md), *USB — the unit on the cable*).
- **It runs and charges on the cable at once**, so that a download goes on while it charges; the
  load before the charge.
- **Off is a true off**: switched on by magnet A held, switched off by the firmware, as a Belka.
  **The cable wakes it but does not switch it on**: woken, it shows the charge on its light for
  some seconds and sleeps again, charging; magnet A held then switches it on.
- **Its cell from a known maker**, with its protection board and an NTC: Li-poly, or LiFePO4 if a
  cell of the size is to be had, the two weighed once the board and its box are drawn — the cell
  itself chosen then, by what the box holds. Li-poly charged to about 80 %, 4,0 V, LiFePO4 to
  3,50 V, the nPM1300's lowest end; used down to about 20 %, the firmware closing the record and
  switching it off there. Charged only between 0 and 45 °C by its NTC, at 32 mA, the nPM1300's
  least, so a cell of some 64 mAh and more, the charge within half its capacity an hour, as its own
  sheet allows; with room round it in the box, which no animal bites through.
- **The same as the Belka's**: the sensor's land ([`BOARD.md`](BOARD.md), *U7*), the record, the
  frame and the firmware. **The land fitted with ST's LSM6DSV320X**, the hoof's sensor, ±16 g on its
  low-g channel and up to ±320 g on its high-g one, so that one build serves every examination: it
  runs on its own clock at 7,68 kHz, its fastest, its samples filtered onto the set's grid, 2 kHz at
  the most ([`PARTS.md`](PARTS.md), *The motion sensor*;
  [`FIRMWARE.md`](FIRMWARE.md), *The sensor and its clock*). One Kryska, no other build.

## The way taken

- **The radio: Fanstel's BM20M**, 10,0 × 14,0 mm, the series' small PCB antenna on the Belka's
  land, the same chip, the series' least range ([`PARTS.md`](PARTS.md), *The radio and the
  processor*, Fanstel's figures), where a small animal keeps Povodok within metres.
- **The supply: Nordic's nPM1300**, the PMIC of every unit, QFN32 of 5 × 5 mm: a charger, two
  bucks, two load switches or LDOs, ship mode and a fuel gauge in the host's software in one chip
  ([nPM1300](https://www.nordicsemi.com/Products/nPM1300)); LCSC lists it, C7501206, so JLCPCB
  fits it, one part and one firmware for every board. Its figures are its product specification's,
  v1.1:
  - its dynamic power path feeds the board from the cable and charges the cell from what is left,
    the battery isolated once full (6.2);
  - BUCK2 gives the module 3,0 V from power-on, 150 kΩ on VSET2, no firmware asked (6.3);
  - ship mode, 370 nA, is the off: SHPHLD held low for 96 ms, magnet A's Hall switch to ground,
    or the cable's VBUS, leaves it, and the firmware enters it; woken by the cable alone, the
    firmware shows the charge, enables it and puts the processor to sleep, the charger running on
    (the electrical specification; the ship-hold button's timing, 96 ms by default);
  - the charger is off from a reset until the firmware enables it, 32 mA by default; 32–800 mA in
    2 mA steps and an end voltage of 3,50–4,45 V by its registers; by the cell's NTC it stops under
    0 °C, charges at half its current up to 10 °C and at its end voltage lowered from 45 °C, and
    stops over 60 °C, the firmware pausing it at 45 °C instead, the cell's limit (JEITA, the warm
    threshold's pause by register; 6.2): a firmware that never starts leaves the cell uncharged.
- **The light from the PMIC**: the two-colour LED on two of its LED drivers, each through a resistor
  of 1,5 kΩ, some 1 mA from the cell, both the charger's from the PMIC's reset — the green lit while
  it charges, the red for its fault — while the processor sleeps, and both taken by the firmware for
  the Belka's patterns
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
- **The memory: CS's CSNP1GCR01-BOW**, an SD NAND of 1 Gbit, NAND flash and an SD controller in
  one package, its bad blocks and its ECC its own; LGA-8 of 8 × 6 × 0,75 mm, LCSC C2691593; beside
  the module's body, never under its antenna. From its sheet, V1.1 of January 2021: the SD
  specification 2.0, SPI mode the card's; 2,7–3,6 V; −30 to +85 °C; 150–200 µA standing, 15–25 mA
  at work (1.2; 3; 4). Chosen over NOR flash for one firmware and one way with the Belka
  ([`chapters/kryska-memory.md`](chapters/kryska-memory.md)).

## The memory

The SD NAND is the Belka's card to the firmware, on the card's pins and its supply, U6, switched
off between the records ([`BOARD.md`](BOARD.md), *Kryska*): Zephyr's SD stack in SPI mode at
8 MHz, FatFs, a session a file, its room taken in one run and a file a power cut left cut at the
next start, the disk on the cable read alone while the unit is in a set, `STRELKA.SET` taken from
it — all the Belka's ([`FIRMWARE.md`](FIRMWARE.md), *The record*; *USB — the unit on the cable*).
The memory is formatted whole by the application's command, 64/5, at the user's word with the
password, as a Belka's card; nothing else erases anything, and no file is deleted alone.
