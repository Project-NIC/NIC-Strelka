<div align="center">

# Pušinka, the application

**What the phone's application must do.**

↑ [NIC-Strelka](README.md)

</div>

---

Pušinka is written by its users; Strelka gives them this list, the interface to the byte
([`PHONE.md`](PHONE.md)), the record's format ([`RECORD.md`](RECORD.md)) and a base to start
from (*The base*, below). Android, iPhone and a computer alike.

## What it must do

1. **The link.** It joins the set's first Povodok, the lowest number present: by USB, or by Wi-Fi
   at 5 GHz — a phone's hotspot, a laptop, a building's network; or, the set gathered by the
   network, every Povodok on one 5 GHz network ([`PRINCIPLE.md`](PRINCIPLE.md), *The three ways of
   gathering*). It gives Povodok the network's name
   and password over Bluetooth LE in Povodok's service mode, and sends 64/20 once the set is made
   up, so that it records and shows the live view ([`FIRMWARE.md`](FIRMWARE.md), *The service
   mode*); it shows a Povodok as 1–4 and the set by its own number. On Android it
   asks for the wired network and binds its sockets to it, since a link without the internet never
   becomes the phone's default; on iPhone it asks for the wired interface and declares its use of
   the local network. Its own password, twelve characters at least, is checked by a challenge and
   its answer ([`COMMANDS.md`](COMMANDS.md), *The lock*). Joined, it reads 64/14 once a second, and
   with no answer for 3 s shows the connection lost.
2. **The set.** A map: the first Povodok, the other Povodoks it hears, and the Belkas. For each
   Belka who gave it — the first heard it itself, or Povodok 1–3 passed it — its state once a
   minute — the cell, the temperature, the card, the SYNC's signal, the clock, the steps out of
   range — and its fault at once. The map of the band from the Povodoks' and the Belkas'
   measurements, the set's channel and its spare ([`PRINCIPLE.md`](PRINCIPLE.md), *The
   channels*).
3. **The set made up, and new Belkas.** It draws a new set — its address, key and channel — and
   gives it with their numbers to every unit on the table: a Belka by its cable, `STRELKA.SET` on
   its disk — a new Belka's, or one opened by 64/5 `do` 3 — a Povodok by 64/19
   ([`FIRMWARE.md`](FIRMWARE.md), *What a unit keeps*); or it starts
   the search — the CALL and the HERE — shows the Belkas found by their chip IDs, gives them their
   numbers, 1–63, and writes them the set's address, key and channel.
4. **The animal and the places.** The set's table: the animal and each Belka's place in the user's
   own words, 32 characters; kept as the user's template and sent into every Belka as CSV
   ([`RECORD.md`](RECORD.md), *The set's table*).
5. **The session.** Its settings: the rate into memory, 50 Hz to 32 kHz, 100 Hz by default; the
   sensor's ranges; the live view, 50 or 100 Hz, quaternions or the raw steps; and the seconds
   without a SYNC that stop the record, 10 by default. Before the start every Belka is given the
   session's number, the date and time and the time zone. The start and the stop. The card is
   never overwritten nor erased without the user's word, asked again: its erasing, its formatting,
   its disk opened for writing and a unit's forgetting each ask the password again and write the
   lock anew just before ([`COMMANDS.md`](COMMANDS.md), *The lock*).
6. **The live view.** Curves from any Belkas on one time axis; a gap shown as a gap. Gathered by
   the network, the Povodoks' messages are joined by the Belka's number and the period, one of
   each kept. **Kept on the phone** at the user's word: what it receives,
   quaternions or raw steps at 50 or 100 Hz, each Belka's by the period's number, a gap as a gap,
   a file a session, exported as CSV and the raw steps as C3D too. For ordinary work no unit need
   record; a unit's own record on its card or flash stays, at a higher rate and with nothing lost,
   the backup and the research's. The quaternions' heading is free, with no magnetometer
   ([`WHY.md`](WHY.md), *The architecture*), so it is set to zero at a run's start, the animal
   standing.
7. **The calibration.** On the table after the warm-up, six faces: every Belka turned onto the same
   face, confirmed, left still a moment, the next face. The application computes it from the live
   view and writes each Belka its raw readings ([`RECORD.md`](RECORD.md), *The session's head*); a
   Belka moved during a step is named and the step repeated. Each calibration's age is shown.
8. **The download.** Over the radio through the first Povodok, Belka after Belka; a download cut
   off goes on from the byte where it stopped. When the other Povodoks are joined too, over
   Wi-Fi, for the download alone, the application gives each Belka's next second to the Povodok
   that hears it best, the next second taking up from the byte where the last left off — for the
   reach, not the pace: they share the set's one channel. Or a Belka on its cable is a disk, or the
   whole set goes into the dock, a computer copying every disk at once. A date and time missing
   are filled in.
9. **The conversion.** The steps into g and °/s by the samples a step and the full scales the
   file's head carries, with the calibration it carries; the gyro's bias followed by the
   temperature in the information blocks; the vertical from gravity. The live view's raw steps by
   the same full scale, the Belka's sensor told by `sensor` in its info
   ([`COMMANDS.md`](COMMANDS.md); the steps in [`PARTS.md`](PARTS.md)).
10. **The time.** The steps lie on the set's grid; where the clock ran on its fallback, the
    application puts them onto the grid by the offsets in the information blocks
    ([`PRINCIPLE.md`](PRINCIPLE.md), *The clock loop*).
11. **The export.** To C3D, the gait laboratories' format, to CSV, and the record as it is. A step
    the unit never had is the step before it again, so that the rows and the frames run evenly;
    the CSV marks it in its column `lost` (`tools/strelka_record.py`), the C3D carries N and the
    sensor's own rate in its group STRELKA (`tools/strelka_c3d.py`).
12. **The analysis**, in the application, never in a Belka: the measures of
    [`ANALYSIS.md`](ANALYSIS.md), with [`analysis/`](analysis/) as the reference; its users add
    the methods they need.
13. **The firmware.** A new image, signed by its maker, by SMP through the first Povodok to the
    Belkas and the Povodoks.

## The menus

One screen a task:

- **The set** — the map of the set and of the band.
- **The Belkas** — a row a Belka: its number, place, state; the search for new ones; *erase*.
- **The calibration** — the warm-up's time, each Belka's calibration and its age, the six faces.
- **Live** — any Belkas chosen, their curves on one time axis; kept on the phone at a touch.
- **The record** — the session's settings, the start and the stop, the user's notes.
- **The download** — the sessions found on the Belkas, their progress, the files kept.
- **The export** — C3D, CSV, the record as it is.
- **The settings** — the Wi-Fi for Povodok, the password, the firmware, Povodok switched off.

## The base

The base is in [`app/`](app/), written in Flutter, so that one code runs on Android, iPhone,
Windows, macOS and Linux ([Flutter, Multi-platform](https://flutter.dev/multi-platform)); a
browser is not among them, having no TCP and UDP sockets of its own. Two packages:

- **`strelka`**, Dart alone, no Flutter in it: SMP over TCP with the lock
  ([`COMMANDS.md`](COMMANDS.md)) and the live view's messages over UDP
  ([`PHONE.md`](PHONE.md)), held to the Python tools' bytes and driving the Povodok simulator
  ([`tools/povodok_sim.py`](tools/povodok_sim.py)) as the application will drive Povodok.
- **`pusinka`**, the application itself: the screens of *The menus*, from the first — Povodok
  joined through the lock, the set and its live view shown.

What Flutter does not carry itself, each platform does its own way: the wired network on Android
and iPhone (*The link*) and Bluetooth LE for the Wi-Fi's name and password — a plugin each, or a
few lines of the platform's own code called from Dart. The Python tools stay the reference: what
the Dart does, they did first.

## What it does not do

- **Where the animal goes**: no position and no map of the ground — the set's map is the radio's;
  Strelka records only the motion ([`README.md`](README.md)).
