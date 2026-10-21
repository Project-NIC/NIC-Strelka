<div align="center">

# Status

**Beta 0.7, 21 October 2026:** the whole design on paper, the commands by SMP to the byte, the
firmware's two projects built against the SDK and reviewed against the pages, the application begun
in Flutter with its protocol in Dart held to a simulated Povodok, nothing run on hardware: the
nRF54LM20A on Fanstel's BM20F module in the Belka and on its WM02C20C, with Wi-Fi, in Povodok; the
record on the user's own microSD card; one to four Povodoks, the first gathering the whole set and
the phone joining it alone, by USB or Wi-Fi; Strelka's own radio frame of six packets, the settings,
the commands, the download and the firmware by ESB with SMP; 40 channels measured by everyone, the
set moving before the losses come; up to 32 Belkas, the firmware holding 63, the live view up to 100
Hz; a LiFePO4 cell; the calibration; the set's table in every record; the board as a netlist for
EasyEDA; the record's tools and the C3D converter; since 0.6 the link's logic, the live view's
messages, the orientation filter, the gauge, the charger, the phone's network path and the lock,
written and checked on the host, and a Povodok simulated in Python for the application.

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
| **computed on paper** | a figure derived from the inputs in [`calc/params.py`](calc/params.py) by a script, checked on every push | the tables of [`PRINCIPLE.md`](PRINCIPLE.md) and [`PHONE.md`](PHONE.md) |
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
- the parts, each with the requirement it meets ([`PARTS.md`](PARTS.md)): Fanstel's BM20F and
  WM02C20C, the ICM-42686-P or the ICM-42688-P on one land, a microSD slot, the BQ25170 charger,
  the MAX17055 gauge, a LiFePO4 cell, a five-pin magnetic contact carrying USB, a Hammond 1551
  box,
- the board as nets, pin by pin, every pin checked against the sheets ([`BOARD.md`](BOARD.md)),
  and as a netlist for EasyEDA, `board/strelka.kicad_sch`, drawn by `board/schematic.py`,
- the firmware, described ([`FIRMWARE.md`](FIRMWARE.md)): two projects, the Belka and Povodok,
- the commands by SMP to the byte ([`COMMANDS.md`](COMMANDS.md)),
- the phone's interface to the byte ([`PHONE.md`](PHONE.md)), what the application must do
  ([`APP.md`](APP.md)) and its base in Flutter, the protocol in Dart held to the simulator
  ([`app/`](app/)),
- the radio rules ([`REGULATIONS.md`](REGULATIONS.md)),
- a first skeleton of the lameness measures, checked on synthetic trots
  ([`ANALYSIS.md`](ANALYSIS.md)).

## What is open

- **the schematic**, to be drawn from [`BOARD.md`](BOARD.md); the board is not drawn,
- **the firmware's code:** the protocol's and the record's code, the inside link's logic on both
  sides and SMP with Strelka's group 64 are written and checked on the host, the link against a
  simulated set of 32 Belkas and 4 Povodoks; the two projects build against the SDK with MCUboot
  ([`firmware/README.md`](firmware/README.md)); the radio under the link and SMP over ESB are not
  written, and nothing has run on hardware,
- **on the development kit and the first boards:** ESB on the nRF54LM20A and its turns with
  Strelka's frame; SMP over TCP; phones taking Povodok as a USB network adapter, and the USB device
  of three classes at once; the raw rates read by the RISC-V coprocessor; the sensor's clock steered
  a tick at a time; the SYNC stamped through the DPPI's bridge,

- **the USB disk's write-protection**: Zephyr's disk class cannot refuse a write, so a Belka on the
  cable shows its card writable while it does not record; a change to the class, or upstream,
- **the dock**, the charging station with its USB hub,
- **Povodok fitted with the sensor and the card too**, on the same board,
- **a C3D file** opened in a gait laboratory's own software,
- **from the radio rules:** that a Povodok's PASS is no "převáděč" in the text's sense, the
  WM02C20C's certificate for 5 GHz, and the finished unit's own assessment under EN 300 328 at the
  full power ([`REGULATIONS.md`](REGULATIONS.md)),
- the magnetic contact's drawing and its seal; the cell's connector matched to its lead.

## Awaiting measurement

- the sensor under CLKIN's one-tick steps, and the loop's lock on two units,
- the crystal's wander on the animal, assumed 1 ppm a minute,
- the body's shadow on a cow, and the reach,
- the e.i.r.p., and at the full power the power in any 1 MHz,
- the current of a built unit, low and high,
- the cell's model for the gauge, characterised for it by the gauge's maker or on the bench,
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
