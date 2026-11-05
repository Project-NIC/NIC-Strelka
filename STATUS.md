<div align="center">

# Status

**Beta 0.8, 2 November 2026:** the whole design on paper, the commands by SMP to the byte, the
firmware's three projects built against the SDK and reviewed against the pages, the application
begun in Flutter with its protocol in Dart held to a simulated Povodok, nothing run on hardware: the
nRF54LM20A on Fanstel's BM20F module in the Belka and on its WM02C20C, with Wi-Fi, in Povodok; the
record on the user's own microSD card; one to four Povodoks, the first gathering the whole set and
the phone joining it alone, by USB or Wi-Fi; Strelka's own radio frame of six packets, the settings,
the commands, the download and the firmware by ESB with SMP; 40 channels measured by everyone, the
set moving before the losses come; up to 32 Belkas, the firmware holding 63, the live view up to
100 Hz; a LiFePO4 cell; the calibration; the set's table in every record; the boards as netlists for
EasyEDA; the record's tools and the C3D converter; since 0.6 the link's logic, the live view's
messages, the orientation filter, the phone's network path and the lock, written and checked on the
host, and a Povodok simulated in Python for the application; since 0.7 the 32 MHz crystal held
through the record, and one land for three sensors, ST's LSM6DSV16X for the classic build, ST's
LSM6DSV320X to ±320 g for the hoof, TDK's ICM-42686-P for the industrial; since 0.75 the sensor read
by the chip's second core, the FLPR, at every rate, three ways of gathering, the set given by the
cable, a Belka asleep with the live view off and its disk read-only in a set; since 0.8 ESB's turns
by Nordic's way, `esb_init()` and `esb_disable()` every period, and the boards described for
drawing, each unit on its own — the Belka, [Kryska](KRYSKA.md), the small Belka on Fanstel's BM20M
with NOR flash for its record, and Povodok — on Nordic's PMIC, the nPM1300, on Kryska the nPM1304 or
the nPM1300: the charge, ship mode as the off, magnet A on its SHPHLD, the light on its LEDs and
Nordic's fuel gauge, the firmware on it built for the three and reviewed twice; and the current
computed by the bytes written. **The drawn boards come with 0.9**, once the questions only the
development kit and the makers can answer are answered.

↑ [NIC-Strelka](README.md)

</div>

---

This is a concept at the design stage. Every block has its page, every packet and every block of
the record is written down to the byte, and the figures are computed by scripts from their inputs
with sources; no board has been drawn and no part has been bought. What is here comes from four
kinds of source, and each page says which:

| kind | what it means | where it shows |
|---|---|---|
| **from a datasheet** | a figure read in the part's own document, cited by document, table or section | most numbers in [`PARTS.md`](PARTS.md) and [`BOARD.md`](BOARD.md) |
| **computed on paper** | a figure derived from the inputs in [`calc/params.py`](calc/params.py) by a script, checked on every push | the tables and lines between `calc:` markers in the pages |
| **an estimate** | a figure with no source yet, marked *assumed* | the processor's and the card's duties, the SYNC's error, SMP's share of a frame, the dock's hub, the card's size |
| **awaiting measurement** | what only a built unit can tell | the list at the end of this page |

## What is closed

- the purpose, the requirements and the architecture ([README](README.md)): up to 32 Belkas and
  one to four Povodoks, the first commanding and gathering the set; the SYNCs and the Belkas in
  fixed slots, no Belka relaying anything; the phone joined to the first Povodok alone; no
  computation in the unit,
- the inside link to the byte ([`PRINCIPLE.md`](PRINCIPLE.md)): six packets of one head, the
  set's state and the Belka's, the channels measured and the move, the power by EN 300 328, the
  lock,
- the physics of every block, computed ([`PRINCIPLE.md`](PRINCIPLE.md)): the period's timetable,
  the card, the download by ESB, by the cable and in the dock, the current, the reach, the clock
  loop simulated, the sensor's noise against the motion,
- the record on the card to the byte ([`RECORD.md`](RECORD.md)),
- the parts, each with the requirement it meets ([`PARTS.md`](PARTS.md)): Fanstel's BM20F, BM20M and
  WM02C20C, ST's LSM6DSV16X or LSM6DSV320X or TDK's ICM-42686-P on one land, a microSD slot,
  Winbond's NOR flash for Kryska, Nordic's nPM1300 PMIC, Kryska's the nPM1304 or the nPM1300, a
  LiFePO4 cell for the Belka, a five-pin magnetic contact carrying USB, a Hammond 1551 box,
- Materinka, the base boards of the three units, one core the same on each, as nets, pin by pin,
  every pin checked against the sheets ([`BOARD.md`](BOARD.md)), and each as a netlist for EasyEDA,
  `board/belka.kicad_sch`, `kryska.kicad_sch` and `povodok.kicad_sch`, by `board/schematic.py`,
- the firmware, described ([`FIRMWARE.md`](FIRMWARE.md)): the Belka's and Povodok's projects and
  the FLPR's, Kryska's the Belka's built for its board,
- the commands by SMP to the byte ([`COMMANDS.md`](COMMANDS.md)),
- the phone's interface to the byte ([`PHONE.md`](PHONE.md)), what the application must do
  ([`APP.md`](APP.md)) and its base in Flutter, the protocol in Dart held to the simulator
  ([`app/`](app/)),
- the radio rules ([`REGULATIONS.md`](REGULATIONS.md)),
- a first skeleton of the lameness measures, checked on synthetic trots
  ([`ANALYSIS.md`](ANALYSIS.md)).

## What is open

- **the schematic**, to be drawn from [`BOARD.md`](BOARD.md); the board is not drawn, and is drawn
  for 0.9 alone, once the kit and the makers have answered,
- **the lock**: a recorded nonce and its MAC can be tried against a dictionary by whoever answers
  as Povodok, which the application does not check ([`COMMANDS.md`](COMMANDS.md), *The lock*);
  Povodok proving itself, or a password-authenticated exchange, is to be chosen,
- **Povodok's ports**, one a Belka, 64 at once: kept, or opened on demand, as the kit's memory
  says; and the MIC's cost a fragment, measured on the kit, lowering the download's pace if it
  tells,
- **Kryska's contact**, the Belka's pair for now, a smaller one to be found with its box,
- **the USB PHY's supply**: the module's VBUS fed from the nPM1300's VBUSOUT, which Nordic names
  for sensing, and the PHY's current unknown — asked, or measured, with a guard of its own as the
  fallback,
- **the firmware's code:** the protocol's and the record's code, the inside link's logic on both
  sides and SMP with Strelka's group 64 are written and checked on the host, the link against a
  simulated set of 32 Belkas and 4 Povodoks; the Belka, Kryska and Povodok build against the SDK
  with MCUboot ([`firmware/README.md`](firmware/README.md)); the radio under the link and SMP over
  ESB are not written, and nothing has run on hardware; the LSM6DSV16X's reading is written beside
  the ICMs', its ODR-triggered mode by ST's AN5763, its axes the ICM's on the shared land by both
  sheets; the LSM6DSV320X's, the hoof's, on its own clock, its samples drawn onto the set's grid by
  their stamps; the three ways of gathering — by the set's radio, one station, by the network — the
  network's in the link's logic and the whole set's simulation, the application's joining of the
  Povodoks' messages still to write; the set given by the cable, a file on a Belka's disk and 64/19
  to a Povodok; a Belka asleep with the live view off, off by itself with every Povodok gone; the
  sensor read by the FLPR, its program merged into the Belka's image and signed as one, the main
  core's reading kept as the fallback; a Belka's disk read-only on the cable while it is in a set,
  opened by magnet B or by the application's word with the password given again, by a patch to
  Zephyr's disk class; ESB's turns with the frame by Nordic's way, `esb_init()` and `esb_disable()`
  every period, the Belka's MPSL never started, and Povodok's released by `bt_disable()`, the
  clock's DPPI channels through `nrfx_gppi` — in the code and built, the turns themselves in
  `shared/link.h` as its shape, not yet as code; the PMIC through the SDK's drivers — the charge,
  ship mode as the off, the cable's wake and idle, magnet A on its SHPHLD, the light on its LEDs,
  Nordic's fuel gauge with the SDK sample's cell model — built; Kryska's record raw on its flash,
  its writer waiting on both dies, the sessions found backwards and read as files, the set and
  the download by SMP on its serial port — built, checked on a simulated memory on the host,
- **on the development kits and the first boards:** ESB on the nRF54LM20A as the primary transmitter
  — as the receiver it has run on Nordic's kit beside an nRF54L15 — and its turns with Strelka's
  frame, the times of `esb_init()` and `esb_disable()` measured; SMP over TCP; phones taking Povodok
  as a USB network adapter, and the USB device of three classes at once; the sensor read by the
  FLPR, the RISC-V coprocessor; the hoof's sensor drawn onto the grid from its own clock; the
  sensor's clock steered a tick at a time, which TDK says the ICM takes; the SYNC stamped through
  the DPPI's bridge, some 125 ns of variation by Nordic's word, in Constant Latency mode; the
  disk's write-protection as Windows, macOS, Linux and Android show it,
- **Kryska**, the small Belka ([`KRYSKA.md`](KRYSKA.md)): what it must do and the way taken — 400 Hz
  in ordinary use and 4 kHz at the most, its memory on the board recording into erased room, the
  record raw on it and its sessions as files, the BM20M, Nordic's nPM1304 or nPM1300 with the
  buck filtered — written; its board to be drawn first, then the cell and the box; the buck
  perhaps forced to PWM while the radio sends, Nordic's way with its ripple, as well as the
  filter, not in its place; a disk made up of the record for a computer's file manager and the
  dock, until which its sessions leave by SMP on the serial port,
- **the dock**, the charging station with its USB hub,
- **a C3D file** opened in a gait laboratory's own software,
- **from the radio rules:** that a Povodok's PASS is no "převáděč" in the text's sense; the
  finished unit's own assessment under EN 300 328 for Strelka's frame and ESB at every power, the
  BM20F's reports covering Bluetooth LE and 802.15.4 alone; the WM02C20C's reports, Fanstel's
  WT02C40C's, expected in the second quarter of 2027 ([`REGULATIONS.md`](REGULATIONS.md)),
- **the units on the body**: the two Korabls, the Belka family's and Kryska's, and how each is held
  — strap, glue or boot, the box empty or potted — what research has used read in its papers, no
  study found comparing them ([`PRINCIPLE.md`](PRINCIPLE.md), *The unit on the body*),
- **the gallop**: the classic's ±16 g clips at the cannon, the hoof's ±320 g at the hardest strikes
  on turf, some 2 % of the strides ([`PRINCIPLE.md`](PRINCIPLE.md), *The sensor's noise*) — whether
  a build for the gallop is wanted,
- the magnetic contact's drawing and its seal; the cell's connector matched to its lead.

## Awaiting measurement

- the sensor under CLKIN's one-tick steps, and the loop's lock on two units,
- the buck's hysteretic bursts' rate on Kryska at a light load, under the filter's corner,
- the USB PHY's current on VBUS, and VBUSOUT's drop under it,
- Povodok's +3V3 at the Wi-Fi's bursts, U5 near its 300 mA, and how long a burst lasts,
- the crystal's wander on the animal, assumed 1 ppm a minute,
- the body's shadow on a cow, and the reach,
- the e.i.r.p., and at the full power the power in any 1 MHz,
- the current of a built unit, low and high,
- the cell's model for the PMIC's fuel gauge, made on the bench by Nordic's nPM PowerUP,
- the SYNC's stamp through the DPPI's bridge, its mean delay and its jitter,
- the busy times of the cards users have, against the SD standard's longest, and the sustained rate
  on the 8 MHz bus.

## What anyone can check

The pages cite their sources, so a reader with the datasheets can check any number; the scripts
under [`calc/`](calc/) compute every table from [`calc/params.py`](calc/params.py), and `python3
calc/check.py` says whether the pages still match them. The analysis under [`analysis/`](analysis/)
runs with its tests; the record's reader under [`tools/`](tools/) builds, reads and exports
[`RECORD.md`](RECORD.md)'s record and runs with its tests; the firmware's common code is compiled
on the host and held to the same record, `python3 tools/test_firmware.py`. A wrong number, a
misread datasheet or a broken link goes into
[Issues](https://github.com/Project-NIC/NIC-Strelka/issues); a question, an
alternative or "we tried that and it failed" goes into
[Discussions](https://github.com/Project-NIC/NIC-Strelka/discussions), where [`WHY.md`](WHY.md)
grows from. The design is published as it is, the software under the MIT licence and the hardware
under the CERN-OHL-P v2, for whoever finds it useful; nothing is sold.
