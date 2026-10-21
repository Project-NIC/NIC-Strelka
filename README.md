<div align="center">

<img src="NIC-Strelka.jpg" width="420"/>

# NIC-Strelka

**English** · [Čeština](README.cs.md) · [Русский](README.ru.md)

**The motion of an animal, for a veterinary examination:** small units fastened on the legs,
the trunk and the head of a dog, a horse or a cow record how it moves, every unit on one common
time, so that each leg, the trunk and the head can be laid side by side, step by step. Live on the
vet's phone wherever the radio reaches, and the whole record afterwards.

**To replace the cheap sensor sets sold today** — at their price or below if it can be, better in
any case.

</div>

> **Beta 0.7:** a design on paper, the firmware built against the SDK, the application begun in
> Flutter, nothing has run yet — one board for the Belka and Povodok: the nRF54LM20A on Fanstel's
> BM20F module, with the nRF7002 on its WM02C20C on the same pads for Povodok; a microSD card; USB
> on the five-pin contact; a LiFePO4 cell; the calibration, the set's table in every record, the
> live view up to 100 Hz, the records as a disk and the whole set's over one cable; what is closed,
> what is open and what only a built unit can tell is under [Status](STATUS.md).

> **The radio rules — read before switching on, wherever you are.** Strelka transmits on 2.4 GHz,
> and Povodok on 5 GHz Wi-Fi too, as a client: the access point it joins — a phone's hotspot, a
> laptop, a building's network — chooses the channel and keeps to the rules of the country it
> runs in, so set it to that country and to 5 GHz. What the Czech Republic and the European Union
> say, quoted from their texts, is in [`REGULATIONS.md`](REGULATIONS.md); elsewhere the rules
> differ, and the exact ones are the national office's — in the Czech Republic ČTÚ.

---

## What it is

A set of identical units, one to thirty-two, fastened on the animal by hook and loop, on a strap
or a hoof boot. Each carries a motion sensor, a radio with its processor, a memory for the whole
examination and a cell for fifteen minutes at least. Povodok, a box of their own, commands them: it
sends the time, gathers what the phone asks for and passes it on — one to four Povodoks in a set,
round the animal, for the reach, three as a rule.

**Everything on the animal.** The motion is measured by the units on the body alone: no camera,
no walkway, nothing set up around the animal. **One set for any animal:** a dog, a horse, a cow;
the set is fitted by the number of units it carries, not built for one species. **Only the
motion:** Strelka does not track where the animal goes.

## The parts

| part | what it is |
|---|---|
| [**Belka**](PARTS.md) | the unit fastened on the animal, with its sensor and its card, on the board it shares with Povodok |
| [**Povodok**](NAMES.md) | the set's commander, a box of its own, one to four in a set for the reach, the first commanding: it sends the SYNC, gathers what the phone asks for and passes it to the phone or the computer, by USB or Wi-Fi; the same board with the Wi-Fi module, without the sensor and the card |
| [**Pušinka**](APP.md) | the application on the vet's phone; its base in [`app/`](app/), in time perhaps whole applications |
| **the dock** | a charging station for the whole set with a USB hub inside: the units charge in it, and every Belka in it is a disk, a computer copying them all at once |
| **Budka** | the box a Belka lives in: the sealed case, its window for the light, its fastening |

Why the dogs, and what the names mean, is under [Names](NAMES.md).

## What exists

Systems that measure a moving animal with inertial sensors on its body:

- **The cheap sets — what Strelka replaces.** Wireless MPU-9250 units sold four to a set, of the
  WitMotion WT901BLECL kind: Bluetooth LE 5.0, an output of up to 200 Hz, about $48 a unit
  ([RobotShop](https://www.robotshop.com/products/witmotion-witmotion-wt901blecl-mpu9250-high-precision-9-axis-gyroscope-accelerometer-magnetometer-3-axis-bluetooth-ahrs-imu-sensor-arduino)),
  at most four at once
  ([Desertcart](https://desertcart.lk/products/511015272-bluetooth-5-0-accelerometer-inclinometer-wt-9011-dcl-mpu-9250-high-precision-9-axis-gyroscope-angle-xy-0-2-accuracy-magnetometer-with-kalman-filter-low-power-3-axis-ahrs-imu-sensor-for-arduino)).
- **Horse.** The Equinosis Lameness Locator, sold to vets: sensors on the head, the pelvis and the
  right forelimb, the data streamed to a tablet while the horse trots
  ([University of Adelaide](https://www.adelaide.edu.au/vet/ua/media/528/Client-Brochure_2019-web.pdf),
  [Horse Sport](https://horsesport.com/magazine/business/pinpointing-lameness-with-technology/)).
  EquiMoves, a research system: up to eight synchronised sensors on the poll, the withers, the
  sacrum, the sternum and each leg
  ([Bosch et al., *Sensors* 2018](https://doi.org/10.3390/s18030850)).
- **Dog.** No system sold for dogs was found. The Lameness Locator with modified software was
  tried on trotting dogs ([Stenman, SLU 2019](https://stud.epsilon.slu.se/14611/)); GaitKeeper is
  a research system ([PMC5335924](https://www.ncbi.nlm.nih.gov/pmc/articles/PMC5335924/)).
- **Cow.** Research only: accelerometers on the hind legs on four farms
  ([*Animal* 2015](https://resolve.cambridge.org/core/journals/animal/article/abs/lameness-detection-via-legmounted-accelerometers-on-dairy-cows-on-four-commercial-farms/2B43F96D169CE6C73F559435AE828915)),
  nine sensors on the body of cows with an induced forelimb lameness
  ([Leclercq et al., *Animal* 2025](https://pub.epsilon.slu.se/id/document/20449105)).

## How we know that it works

On the same animal at the same time, Strelka shows what a system sold to vets shows — the measure of
whether it works, not of its price. For a horse that is such a system; for a dog and a cow, where
none is sold, a research system or the changes its published studies report.

## What it must do

Ranges and orders of magnitude; the exact values come with the parts.

| | requirement |
|---|---|
| units in a set | 1 to 32, on one animal; the firmware holds 63; and one to four Povodoks |
| animals | a dog, a horse, a cow — the same set |
| into a unit's memory | about 100 Hz, chosen per session up to a few kHz |
| live | 50 or 100 Hz a unit, the turn as quaternions or the raw steps, chosen per session; what the radio misses is in the unit's memory |
| the units' common time | far below one sample: microseconds |
| the record | the whole examination, in every unit; the memory does not limit it |
| one charge | 15 min at least |
| reach | round the animal's body, up to four Povodoks round it, a Belka heard by any of them; to the phone or the computer through the first Povodok, by a USB lead or Wi-Fi at 5 GHz |
| size | a matchbox or larger |
| weight | no limit set; it follows from the parts |
| fastening | on a strap or a hoof boot, by hook and loop |
| conditions | indoors and out; water, mud and a kick; cleaned with disinfectant between animals |
| the vet's screen | a phone, Android or iPhone, or a computer |
| price | the cheap sets' or below — about $48 a unit, four to a set, before the duty and the post — and better in any case |

## How it is built

The blocks and what joins them; the parts are in [`PARTS.md`](PARTS.md).

```
  Pušinka, on a phone or a computer
        │  USB on the magnetic contact, or Wi-Fi at 5 GHz:
        │  Povodok a client of a phone's hotspot, a laptop or a building's network
  Povodok 1 … 4, the leashes — the first commands
        │  Strelka's own 2.4 GHz radio: each Povodok's SYNC in a slot of its own;
        │  a Belka sends in a slot of its own, on its own clock between the SYNCs
  ┌─────┼──────┬────────┐
Belka Belka  Belka …  Belka       each hears any of the Povodoks; none relays
                                  (up to 32 units in a set)
```

**Belka** is one board in its Budka, a sealed box:

| block | what it does | joined to the processor by |
|---|---|---|
| the radio and the processor, one chip | the SYNC heard, its slot, the clock loop, the record | — |
| the motion sensor | gyro and accelerometer; its samples stored as they come, or summed N to one, N chosen per session | a serial bus; its clock input, made by the processor; its data-ready line |
| the memory | the whole record of the examination | a serial bus |
| the charger, the supply and the gauge | charges the cell from the contacts and makes the supply, set by resistors alone; the gauge counts the cell's state | the charger's two status pins; the gauge on I²C |
| the cell | 15 min at least a charge | — |
| the charging inlet | a five-pin magnetic contact in the box, wired symmetric: power on both outer pins, ground in the middle, the processor's USB between | — |
| the magnets and the light | on, off, the cell's state, the factory settings; the light two-coloured, red and green | magnet A to the board's switch and to the processor, magnet B to the processor; the light on the processor's pins |

**Povodok** is a box of its own, sealed too:

| block | what it does | joined to the processor by |
|---|---|---|
| the radio and the processor, one chip, the Belka's | the SYNC, the slots, the commands, the bridge to the phone | — |
| Wi-Fi at 5 GHz | a client of a phone's, a laptop's or a building's access point | a serial bus |
| the charger, the supply and the gauge | the Belka's; the cable switches Povodok on | the charger's two status pins; the gauge on I²C |
| the cell and the inlet | the Belka's: the magnetic contact, the charge and USB | — |
| the magnets and the light | on, off, the cell's state, the start and the stop | magnet A to the board's switch and to the processor, magnet B to the processor; the light on the processor's pins |

**The roles.** The Povodoks alone command, the first of them; every Belka is the same board. One
to four Povodoks in a set, for the reach, each sending its own SYNC from where it stands and
hearing the Belkas; a second switched on joins as the next SYNC. Povodok rides in the hand
on a USB lead; on the ceiling of a stable or a surgery, fed from the mains, over Wi-Fi; or on the
animal's neck or head, recording without a phone, started and stopped by its magnet B. Its
cable switches it on — plugged in, or the computer at its other end switched on — and five
minutes after the cable leaves it switches itself off; the application's menu, after the
password, switches it off too. A Belka is switched on by its magnet alone; its cable only
charges it.

**Inside the set, the Povodoks' SYNCs and the Belkas' slots.** Each Povodok sends its SYNC in a
fixed slot of its own, four slots always, a missing Povodok's silent; the SYNC carries the period's
number, the live view, whether the record runs — the start and the stop are that state, not a
command — and the spare channel. Every Belka measures one of the 40 channels a period, the set
working on the first 39 and 2480 MHz kept for the search, and reports its noise; the first Povodok
moves the set to the spare channel before the losses come, the move counted down over two periods.
What the first Povodok sends, the others repeat; when it drops out, the next takes over. A Belka
takes its time from any SYNC it hears, and needs one now and then, its clock holding between. Each
Belka has a fixed slot by its number from the pairing, all slots of one length: the live view and
its state, sent in every period, a SYNC heard or not. No Belka relays anything nor sends anything
again, and what is missed lies in the Belka's memory. Ten seconds without a SYNC stop the record, a
setting in Pušinka, and the Belka's sending; it listens on, and when a SYNC comes again saying the
record runs, it goes on in the same session, the gap seen in its blocks' numbers. **Strelka's own
frame carries only the small things**, in six packets, each with the same head — the SYNC, the LIVE,
the MISS and the PASS by which the other Povodoks hand the first what it missed, and the CALL and
the HERE of the search for new units; everything else, the settings, the commands, the download and
the firmware, goes by the maker's own: Nordic's Enhanced ShockBurst, ESB, on the same radio, with
SMP on it, the protocol of Zephyr's MCUmgr; Povodok a bridge between it and USB or Wi-Fi. **A new
Belka**, or one after the factory settings, listens on a channel and an address fixed in the
firmware, the same for every set; Povodok finds it there, writes it its number, the set's address,
key and channel by SMP, and it moves to them.

**To the phone or the computer, through the first Povodok alone:** a USB lead on the magnetic
contact, or Wi-Fi at 5 GHz, Povodok joining the access point on whatever channel it uses; 2.4 GHz is
refused, for the set's own radio. Bluetooth LE only in the first minutes after Povodok is switched
on, to receive the network's name and password, then off. **Over either, TCP/IP:** on the lead
Povodok is a network adapter (USB's CDC-NCM), so that the same runs over both and the application
has one way; SMP over TCP for the commands, the downloads and the firmware; the live view over UDP,
a message a period with the whole set in it, one LIVE of each Belka, gathered by the first Povodok
from its own hearing and the others' PASSes; a period lost left lost. One link, one address: no star
of Povodoks, no switch, and any phone. The interface is written down to the byte for whoever writes
the application, in [`PHONE.md`](PHONE.md) and [`COMMANDS.md`](COMMANDS.md). The radio rules are in
[`REGULATIONS.md`](REGULATIONS.md).

**The lock** keeps strangers from commanding the set; the data, an animal's motion, are not hidden.
The application's password, eight characters at least, is checked by a challenge and its answer, so
that it never crosses the lead or the air readable; on Wi-Fi the network's own password keeps
strangers off it. Between Povodok and the Belkas every SMP packet is signed by the set's 128-bit
key, AES-CCM's tag from the chip's own hardware, a count in it against a replay, and so is every
SYNC, by the same key and its period: no one but the set starts or stops the record, moves the set
to another channel, changes a setting or erases a card. The other packets go in the clear. The set's
key reaches a new Belka once, at its taking, encrypted under a key the two agree by ECDH first, the
chip's CRACEN doing it. A firmware boots only signed by its maker, whatever key a stranger holds.
Against jamming no key helps: the set moves off a channel taken, and what the radio misses stays on
the cards.

**The record's format** is Strelka's own, blocks with their time stamps, written down to the byte, a
session a file on the card's own FAT32 or exFAT; Strelka gives converters to C3D, the format of the
gait laboratories ([c3dr](https://docs.ropensci.org/c3dr/articles/c3dr.html)), and to CSV.

**The charge.** A cable from any USB port, a notebook's or a phone charger's, to a five-pin
magnetic contact in the box, Belka's and Povodok's alike: power on both outer pins and ground in
the middle, so that it charges either way round, and between them the processor's USB — on a
computer a Belka, switched on, is a disk with its records, a serial port for the bench, and takes
a firmware through the boot loader without opening the box.

**On the table**, or still on the animal, a whole set gives every record over the radio by ESB
through the first Povodok, Belka after Belka; with the other Povodoks joined over Wi-Fi for the
download alone, a second at a time each Belka goes to the Povodok that hears it best, the phone
dealing them out and the next second taking up where the last left off, so that a Belka moving away
from one Povodok is taken by another; or each Belka on its cable, switched on, is a disk with its
records; or the whole set goes into the dock, which charges it, and a computer copies every Belka's
disk at once, seconds for an hour's record at 100 Hz and about a minute at 1 kHz.

**The firmware** comes over the radio too, by SMP from the application through Povodok, on the
table; the first is put in before the box is closed.

**The time.** Each Belka steers its sensor's clock, a tick at a time, until its samples fall on
Povodok's grid; the SYNC's arrival and every sample are stamped in hardware. Every unit samples at
the same instants, to microseconds. **The session** is numbered by Povodok and dated by the phone,
the date and time tied to a period's number, both given to every Belka with the settings before
the start, so that the records of one examination carry the same number and every block its
time; without a phone, Povodok's own count and the last time the phone gave it, or "unknown", the
application filling it in at the download.

**The record without computation**. A Belka stores its sensor's samples raw or summed, and
nothing else: no spectrum, no measure. For the live view alone it computes its turn, as
quaternions from its accelerometer and gyroscope, or sends the raw steps, as the session says.
Every analysis is the application's or a computer's, from the record, so that each use — a lame
horse, a jumping dog, a machine's knock — takes the method it needs.

**Where the radio fails** — a large body between a Povodok and a Belka — another Povodok, on the
other side, is heard. Where none is, the live view has gaps, the clock keeps its last correction,
and the record is whole in the Belka until ten seconds without a SYNC stop it; a SYNC heard
again takes it up in the same session.

## In the hand

No button: two magnets at the box's marks, A and B, and the two-coloured light. A hold acts when the
magnet is taken away, by how long it was held, so that a long hold does not act early; the same on
every unit, printed on the box.

| you do | the unit |
|---|---|
| magnet A at its mark, held until the light shows, half a second or so, four on the cable | switches on; a magnet brushed past does nothing, the unit going off again before its light |
| magnet A, a short hold, the unit on | shows the cell: green four blinks 76–100 %, orange three 51–75 %, orange two 26–50 %, red one 11–25 %, red fast blinks 10 % and less |
| magnet A, held 6 s | switches off when let go: the record closed, the light dark as the magnet leaves |
| magnet B on Povodok, held 3 s, the set prepared | the set starts measuring; the light green for 2 s |
| magnet B on Povodok, held 6 s, the set measuring | the set stops; the light red for 2 s |
| both magnets, held 10 s, the unit on or off | the factory settings: the light orange for 2 s at 10 s, and when both are let go the set is forgotten, the calibration too, the records kept, the unit starts again |

On a Belka magnet B alone does nothing; a hold of B under 3 s does nothing, over 10 s neither. The
application does the same by its commands: the start and the stop, the factory settings, and a blink
to find a unit, orange and fast. What the light says: green slow blinks, in a set, waiting; green
fast blinks, measuring; green and red in turn, Povodok; red blinks, the cell low, under 20 %; green
blinks on the charger, charging; green steady, charged; orange steady, the charger's fault; red
steady, a fault; orange fast blinks, sought by the application ([`FIRMWARE.md`](FIRMWARE.md), *The
magnets and the light*).

## Files

| file | what |
|---|---|
| [`STATUS.md`](STATUS.md) | what is closed, what is open, what only a built unit can tell |
| [`REGULATIONS.md`](REGULATIONS.md) | the radio rules, the Czech Republic's and the EU's, quoted from their texts |
| [`NAMES.md`](NAMES.md) | the dogs, and what the names mean |
| [`PRINCIPLE.md`](PRINCIPLE.md) | each block's physics and a rough calculation |
| [`RECORD.md`](RECORD.md) | what a Belka writes on its card, to the byte |
| [`PARTS.md`](PARTS.md) | the parts, each with the requirement it meets |
| [`FIRMWARE.md`](FIRMWARE.md) | what the Belka's and Povodok's firmware do, each on its own |
| [`APP.md`](APP.md) | what Pušinka, the phone's application, must do |
| [`PHONE.md`](PHONE.md) | what the first Povodok and the phone say, to the byte |
| [`COMMANDS.md`](COMMANDS.md) | what the application, Povodok and the Belkas say to each other by SMP, to the byte |
| [`BOARD.md`](BOARD.md) | the board of the Belka and Povodok — the schematic as nets, the values, the bill of materials |
| [`board/`](board/) | the parts and their nets drawn as a list, `nets.svg`, by `nets.py`, and as a netlist for EasyEDA, `strelka.kicad_sch`, by `schematic.py`; the schematic is to be drawn |
| [`calc/`](calc/) | the scripts behind every figure, their inputs with sources in `params.py`, and `check.py` |
| [`firmware/`](firmware/) | the firmware's code: two nRF Connect SDK projects, the Belka and Povodok, the code they share and the protocol's, checked on the host; both build against the SDK, nothing has run on hardware |
| [`tools/`](tools/) | the record built, read and exported to CSV, `strelka_record.py`, and to C3D, `strelka_c3d.py`, with their tests — the reference for [`RECORD.md`](RECORD.md); the firmware's common code held to it, `test_firmware.py`; a Povodok simulated for the application to be written against, `povodok_sim.py`, and its client, `smp.py` |
| [`app/`](app/) | the application's base in Flutter: the protocol in Dart alone, `strelka/`, held to the tools and driving the simulator; the first screens of Pušinka, `pusinka/`, analysed and tested as widgets, run on no device yet |
| [`ANALYSIS.md`](ANALYSIS.md) | the lameness measures, a first skeleton after the published methods |
| [`analysis/`](analysis/) | the skeleton's code and its checks on synthetic trots |
| [`WHY.md`](WHY.md) | what was considered and set aside, and why |

---

The software under the MIT licence, see [LICENSE](LICENSE); the hardware design, the pages and the
pictures, under the CERN-OHL-P v2, see [LICENSE-HW](LICENSE-HW).

## Acknowledgements

To the AI assistant Claude (Anthropic), for its great help with this project.

★ Viva La Resistánce ★
