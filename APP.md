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

1. **The link.** It joins the set's first Povodok, the lowest number present: by USB, or by Wi-Fi at
   5 GHz — a phone's hotspot, a laptop, a building's network; or, the set gathered by the network,
   every Povodok on one 5 GHz network ([`PRINCIPLE.md`](PRINCIPLE.md), *The three ways of
   gathering*). It gives Povodok the network's name and password over Bluetooth LE in Povodok's
   service mode, and sends 64/20 once the set is made up, so that it records and shows the live view
   ([`FIRMWARE.md`](FIRMWARE.md), *The service mode*); it shows a Povodok as 1–4 and the set by its
   own number. On Android it asks for the wired network and binds its sockets to it, since a link
   without the internet never becomes the phone's default; on iPhone it asks for the wired interface
   and declares its use of the local network. The password, twelve characters at least, is entered
   once, when the set is first joined, and kept in the phone's own keystore; from then on the
   application locks every unit by it itself, as headphones paired once — a challenge and its answer
   ([`COMMANDS.md`](COMMANDS.md), *The lock*). Povodok's own key is pinned at the first joining and
   shown in the settings as a fingerprint; another key is refused, taken only at the user's word
   after Povodok's factory settings. The factory password, shorter, is changed at once. It asks
   again only before the erasing, the formatting, the disk opened for writing and a unit's
   forgetting. A unit off while the password is changed keeps the old one: it is given its factory
   settings and joined again, its records kept. Joined, it reads 64/14 once a second, and with no
   answer for 3 s shows the connection lost.
2. **The set.** A map: the first Povodok, the other Povodoks it hears, and the Belkas. For each
   Belka who gave it — the first heard it itself, or Povodok 1–3 passed it — its state once a
   minute — the cell, the temperature, the card, the SYNC's signal, the clock, the steps out of
   range — and its fault at once. The map of the band from the Povodoks' and the Belkas'
   measurements, the set's channel and its spare ([`PRINCIPLE.md`](PRINCIPLE.md), *The
   channels*).
3. **The set made up, and new Belkas.** It draws a new set — its address, key and channel, by the
   phone's secure random source, the address never the search's — keeps it in the phone's keystore
   and gives it with their numbers to every unit on the table: a Belka by its cable, `STRELKA.SET`
   on its disk — a new Belka's, or one opened by 64/5 `do` 3 — a Povodok by 64/19
   ([`FIRMWARE.md`](FIRMWARE.md), *What a unit keeps*); or it starts the search — the CALL and the
   HERE — shows the Belkas found by their chip IDs, gives them their numbers, 1–63, and writes them
   the set's address, key and channel. A number is one unit's alone in the set, whatever its kind
   ([`PRINCIPLE.md`](PRINCIPLE.md), *The inside link*). The set's channel is chosen at its making,
   the quietest offered from the map of the band, or the user's; at rest it is moved by hand from
   the diagnostics (64/24).
4. **The animal and the places.** The set's table: the animal and each unit's place, a code of two
   letters and its name. The codes come from a template by the animal — for a horse, a cow, a dog or
   a cat LF, RF, LH, RH the limbs, HD the head, WI the withers, PE the pelvis, TS and LS the
   thoracic and the lumbar spine; for a person LS, RS, LE, RE, LH, RH the shoulders, the elbows, the
   hips by side — with the template's pairs of left and right, LF and RF, LH and RH. The codes and
   the names are English in the record, in every language, as research has them; the application
   shows the code with the place's name in the user's language. The code shows the unit everywhere:
   on its tile, by its curves and by its numbers. Sent into every unit as CSV with the session
   ([`RECORD.md`](RECORD.md), *The set's table*).
5. **The session.** Its settings: the rate into memory, 50 Hz to 32 kHz, 100 Hz by default; the
   sensor's ranges for the whole session, those every chosen unit's chip has — from ±2 g and ±125
   °/s, ±16 g and ±2 000 °/s by default — and the hoof's high-g channel kept beside the low-g at ±16
   g at most, off by default or at ±32 g to ±320 g, offered where a chosen unit has the LSM6DSV320X
   (64/1's `hg`); the live view, 50 or 100 Hz, quaternions or the raw steps; and the seconds without
   a SYNC that stop the record, 10 by default. Before the start every Belka is given the session's
   number, the date and time and the time zone. The session's units: a tile each, the kind's letter
   and the number — B a Belka, K Kryska, Z Zvjozdočka — chosen by a touch, one not calibrated never
   (7); a unit not chosen neither records nor sends the live view, staying as at rest. **The
   synchronisation**: each chosen unit shown as having taken the session and heard by Povodok,
   64/14's `rc` and `ago`; the start only then. The start and the stop. The card is never
   overwritten nor erased without the user's word, asked again: its erasing, its formatting, its
   disk opened for writing and a unit's forgetting each ask the password again and write the lock
   anew just before ([`COMMANDS.md`](COMMANDS.md), *The lock*).
6. **The live view**, landscape, on four screens: **the choice**, the tiles, each with its place's
   code, its kind and its number, LF · K7; **the overview**, the units on a drawing of a few lines,
   placed and moved by a touch, or a list of their codes alone, the drawing not needed; **the
   curves**, the units chosen, large, on one time axis, a gap shown as a gap; **the numbers**, for
   each unit by its code the maximum, the minimum and the range in a stride, and the difference of
   left against right by the template's pairs. Each unit is shown in the curves or in the numbers,
   as the user chooses. What they show, a first take for the animal to judge: from raw steps the
   vertical acceleration, the acceleration along gravity as its slow mean finds it, less 1 g; from
   quaternions the tilt from the vertical; a stride the signal's own period by its
   autocorrelation, a trunk's two unlike beats one stride; the pairs by the places' codes, L…
   against R…. Gathered by the network, the Povodoks' messages are joined by the Belka's
   number and the period, one of each kept. **Saved on the phone** at a touch, *Save*: what it
   receives, quaternions or raw steps at 50 or 100 Hz, unchanged, a file a session (*The live view
   saved*), made into a C3D, calibrated, when the saving stops. For ordinary work no unit
   need record; a unit's own record on its card or flash stays, at a higher rate and with nothing
   lost, the backup and the research's. The quaternions' heading is free, with no magnetometer
   ([`WHY.md`](WHY.md), *The architecture*), so it is set to zero at a run's start, the animal
   standing.
7. **The calibration**, the first thing after joining and the measurement's condition: a unit
   without one cannot be chosen (*The menus*, 3). In the service mode or at work, the set not
   recording and the live view off, so that it is made anew between two measurements; on the table
   after the warm-up, six faces: every unit turned onto the same face, confirmed by a touch, and
   each asked by 64/22, in the measurement's ranges, the hoof's LSM6DSV320X its high-g channel
   beside at ±32 g, its finest, a g some thousand steps (DS14623 Rev 3, table 3); the Belka takes 5
   s still, its answer read then, and a unit moved or lying on another face is named and asked again
   alone. Between the faces the application shows the turn to make. After the sixth the Belka
   computes the calibration and keeps it ([`RECORD.md`](RECORD.md), *The session's head*); the
   application needs only its time and temperature (64/0 `cal`, 64/3), no live view. Each
   calibration's age is shown; one older than the setting allows asks a new one, and a unit far
   warmer or colder than at its calibration is warned of (*The menus*, *The settings*).
8. **The download**, in the data manager, one session at a time, the set at rest. On the right the
   sessions on the units, read from each by 64/2 and gathered by their number; on the left what is
   on the phone. For each session the date and the rate from one unit's head and the units it needs,
   from the set's table after it, the ones off named — "switch K7 on" — and without them a download
   only at the user's word. The application asks every joined Povodok once by 64/21 with the same
   units; each downloads on its own channel the units that chose it, always the one furthest behind,
   and passes the frames on port 1401 ([`COMMANDS.md`](COMMANDS.md), 64/21). The application writes
   them as they come into two at once: each unit's file as it is on its card, STRK, and one C3D of
   them all, the heads in its group STRELKA; a C3D can be made again from the STRK at any time
   (`tools/strelka_c3d.py`). Both named as *The names* has it once the download is over. Its screen:
   a bar a unit, the pace, the time left, a unit left out named; the stop, and the going on from the
   bytes reached. A large session goes by the dock or the cable, a unit a disk, a computer copying
   every disk at once. A date and time missing are filled in.
9. **The conversion.** The steps into g and °/s by the step each group of the file's head gives,
   with the calibration it carries (`RECORD.md`, *The session's head*), a step a mean of N whatever
   N; the gyro's bias followed by the temperature in the information blocks; the vertical from
   gravity. The live view's raw steps, calibrated by the Belka ([`PRINCIPLE.md`](PRINCIPLE.md), *The
   LIVE*), by the sensor's own steps alone, the Belka's sensor told by `sensor` in its info
   ([`COMMANDS.md`](COMMANDS.md); the steps in [`PARTS.md`](PARTS.md)).
10. **The time.** The steps lie on the set's grid; where the clock ran on its fallback, the
    application puts them onto the grid by the offsets in the information blocks
    ([`PRINCIPLE.md`](PRINCIPLE.md), *The clock loop*).
11. **The export.** The C3D, the gait laboratories' format, made by the download (8), and the CSV
    from it on the phone; the record as it is from the cards, a unit on its cable or the set in the
    dock. A step the unit has not is zeros, every value, in the C3D and the CSV alike — never a
    reading, the gravity always there — so that the rows and the frames run evenly and the gap
    shows; the CSV marks it in its column `lost` as well (`tools/strelka_record.py`), the C3D
    carries N and the heads in its group STRELKA (`tools/strelka_c3d.py`): SOURCE `card`, or `live`
    for the live view saved, and each unit's sensor's steps and its calibration as its head has
    them, raw, the high-g channel's too where the record keeps it, its three channels HX, HY, HZ
    after the six, so that the raw steps can be computed back from the
    values to the count. The CSV is made of a C3D, either. Three kinds of file and no more: STRK,
    the card's own; C3D; CSV, a table.
12. **The analysis**, in the application, never in a Belka: the measures of
    [`ANALYSIS.md`](ANALYSIS.md), with [`analysis/`](analysis/) as the reference; its users add
    the methods they need.
13. **The firmware.** A new image, signed by its maker, fetched from the public repository's
    folder [`firmware/images/`](firmware/images/) — gzipped, beside a manifest of the version, the
    sizes and the SHA-256 — while the phone is online, kept on it and checked by its size, its hash
    and its header's version; then given by SMP through the first Povodok to the Belkas, one at a
    time, and to the Povodoks, the one joined last, which starts again and is joined again: each
    image into the unit's second slot by MCUmgr's image group, marked for a test, the unit started
    again. A unit off takes it when it is on.

## The menus

One screen a task, the main menu a bar of six along the bottom, 1–6. The look, "Volkov", of the
DOS file commanders: a blue ground, light monospaced letters, the sixteen colours of their text
mode, two panels side by side wherever something is chosen — for the contrast, read in the sun
and in a stable alike; the system's own monospaced letters, no font bundled.

1. **The overview** — the Povodoks, the units on and off, their cells, cards and signals.
2. **The calibration** — the six faces, each unit's calibration and its age (*What it must do*,
   7).
3. **The measurement** — the units' tiles, the place's code, the kind's letter and the number,
   chosen by a touch; the session's settings, the animal and the places, the user's notes; the live
   view, a record running or not, landscape on four screens; the synchronisation, the start and the
   stop (4, 5, 6).
4. **The data manager** — on the left what is on the phone, on the right what is on the units;
   the download; the export to C3D and CSV; the erasing and the formatting, under the password
   (8, 11).
5. **The diagnostics** — read from Povodok by 64/23 once a second, in the service mode and at
   work, the live view on or off: the map of the band, the set's channel and its spare, each
   Povodok's margin; each unit's signal and when it was last heard, its SYNCs' signal and those
   missed, its clock, its steps out of range, its cell and card (2).
6. **The settings** — the language, the password, the Wi-Fi for Povodok, the set made up and new
   units, the firmware, Povodok switched off (1, 3, 13).

**The sketches**, an idea and not the finished look, a picture a screen in
[`app/mockups/`](app/mockups/), drawn again by its `render.js`: [the
joining](app/mockups/en/join.png); 1 [the overview](app/mockups/en/overview.png) and [a unit's
page](app/mockups/en/unit.png); 2 [the calibration](app/mockups/en/calibration.png); 3 the
measurement's [choice](app/mockups/en/measurement-a-choice.png),
[overview](app/mockups/en/measurement-b-overview.png),
[curves](app/mockups/en/measurement-c-curves.png) and
[numbers](app/mockups/en/measurement-d-numbers.png); 4 the data manager's
[list](app/mockups/en/data-a-list.png), [download](app/mockups/en/data-b-download.png) and
[formatting](app/mockups/en/data-c-format.png); 5 [the diagnostics](app/mockups/en/diagnostics.png);
6 the settings' [language](app/mockups/en/settings-a-language.png),
[About](app/mockups/en/settings-b-about.png), [new units](app/mockups/en/settings-c-new-units.png),
[places' templates](app/mockups/en/settings-d-templates.png)
[firmware](app/mockups/en/settings-e-firmware.png) and [the measurement's
settings](app/mockups/en/settings-f-measurement.png).

**The settings**, each with its default, so that most users never open them:

| setting | the choices | by default |
|---|---|---|
| the calibration valid | every measurement, 2 h, a day, 30 days, without a limit | 30 days |
| warned of a temperature apart from the calibration's | by more than the degrees set | 10 °C |
| the curves' window | | 5 s |
| the numbers over the last | | 10 strides |
| the connection shown lost after | | 3 s |
| the radio's power | within what the rules allow ([`REGULATIONS.md`](REGULATIONS.md)) | +5 dBm |
| the light while measuring | as usual, faults alone, off | as usual |

Settings, as the factory settings, are taken outside the measurement alone: never while the set
records. Fixed, the same on every set: the cell's end voltage, the password's twelve characters at
least, the magnets' holds ([`FIRMWARE.md`](FIRMWARE.md), *The magnets and the light*) and the
radio's timing, which every unit of a set must share.

**The languages**: a table of the texts a language — English, Czech, Russian, Spanish, Brazilian
Portuguese, German, French; a new language a new table, the code unchanged. The main messages
alone, translated by machine key by key, for a speaker to correct; numbers, units and the technical
lines the same in every language. No help: each screen says what to do.

**About**: the version and Pušinka, Strelka's puppy, drawn.

## The live view saved

The phone keeps the live view's messages as Povodok sends them ([`PHONE.md`](PHONE.md)) in a
working file, as Xsens's MT Manager keeps its sensor's messages in its log and turns them into
other formats only afterwards ([MT Manager](https://mtidocs.xsens.com/overview-mt-manager)); when
the saving stops, the working file is made into a C3D and goes. The working file,
`S00007-20261110-170512.live`, by the session and the time the saving began: 8 B, `STRELKA` in
ASCII and the version, 1; then records, little-endian:

| bytes | the field | what |
|---|---|---|
| 0 | the kind u8 | 1 a live view's message; 2 an SMP frame with Povodok; 3 an SMP frame with a Belka |
| 1 | the unit u8 | 0 for a message; for an SMP frame Povodok's number, 0–3, or the Belka's, 1–63 |
| 2–3 | the length u16 | the bytes that follow |
| 4– | the bytes | unchanged: the message, or the SMP frame's head of 8 B and its CBOR body |

First the SMP frames the C3D needs: 64/1, the session as it was sent, its ranges, N and the
live view's code; 64/18, the set's table as it was sent; and every chosen unit's answers to 64/0
and 64/3, read before the start, while SMP still goes over ESB. Then a message a period. Gathered
by the network, the phone takes the messages of every Povodok and joins them by the Belka and the
period: the head from the lead Povodok's message, or from the first to come where it sent none;
each Belka's entry once. The copies of an entry are compared, its byte 2 aside, which names the
Povodok that passed it: the same are kept once, differing ones are settled by the larger part,
and with none larger than half the step is taken as lost — the unit's own record stays the backup.
A period is written once a message two periods later has come, and one later still is left out; a
period lost is not in the file. Written by `LiveSaver` in `app/strelka`, `tools/strelka_live.py`
the reference. A working file left by a stop the application never reached is made into its C3D
in the data manager by a touch.

The C3D is the download's (*What it must do*, 11), its SOURCE `live`: the raw steps in g and °/s, or
the quaternions as QW, QX, QY, QZ; a step lost is zeros. The live view is calibrated by the Belka,
raw steps and quaternions alike ([`PRINCIPLE.md`](PRINCIPLE.md), *The LIVE*), so its raw steps are
converted by the ranges of 64/1 and the sensor's own step by its `sensor` in 64/0
([`PARTS.md`](PARTS.md)) alone, a period of S0's 3 or 6 by the high-g channel's step, 64/1's `hg`;
the calibration of 64/3 rides in the C3D for the reader to see. A record from the cards is the
original, calibrated by the phone (*What it must do*, 9): each source calibrated in one place.

## The names

A file is named by the date and the time its record began — the date first, year, month and day,
so that a list sorted by name is sorted by time, and a name, unlike a file's date, survives a copy
and an archive unpacked — then the session, the user's note, and where it came from:

    2026-11-10_1705_S44_Praha prase_card.c3d
    2026-11-10_1705_S44_Praha prase_card_S00044B03.STRK
    2026-11-10_1705_S44_Praha prase_live.c3d

The note is the user's, at most 64 characters, written in the measurement's choice and sent in the
set's table, the line `note`, so that the card's every file carries it; what Windows forbids in a
name (`\ / : * ? " < > |`) is made `-`. A record from the cards takes its head's date and time;
the live view saved, the time the saving began.

## The base

The base is in [`app/`](app/), written in Flutter, so that one code runs on Android, iPhone,
Windows, macOS and Linux ([Flutter, Multi-platform](https://flutter.dev/multi-platform)); a
browser is not among them, having no TCP and UDP sockets of its own. Two packages:

- **`strelka`**, Dart alone, no Flutter in it: SMP over TCP with the lock
  ([`COMMANDS.md`](COMMANDS.md)) and the live view's messages over UDP
  ([`PHONE.md`](PHONE.md)), held to the Python tools' bytes and driving the Povodok simulator
  ([`tools/povodok_sim.py`](tools/povodok_sim.py)) as the application will drive Povodok.
- **`pusinka`**, the application itself: Povodok joined through the lock, then the six menus along
  the bottom in the commanders' look — the overview, the Povodoks and every unit's state, fault,
  cell, temperature and card, and a unit's page with its blink, its card erased and its forgetting
  under the password asked again; the measurement's choice, the units as tiles held to their
  calibration's validity, the session sent with them, each shown in step, the start only then, the
  live view after it, its overview, curves and numbers, left against right, the record stopped
  there, what it receives saved on the phone at a touch into a C3D; the data manager, what is on the
  phone and the sessions on the units, each with a unit to switch on, too large for the radio or
  ready, the download into the cards' own files and one C3D, a C3D turned into CSV, the files named
  by the date, the session and the user's note; the calibration, the six faces asked of every unit
  by 64/22, the turn shown between them, a unit moved asked again alone, each one's age; the
  settings' language, the measurement's and the calibration's settings kept in the phone, the
  places' templates — horse, cow, dog, cat, person, the codes English, the names in the user's
  language — each unit given its code and the set's table sent with the session by 64/18, new units
  found by the search and taken, the password changed, twelve characters at least, Povodok switched
  off, and About; the diagnostics, the band's noise by channel and each unit as Povodok knows it,
  read by 64/23 once a second, the set moved by hand to the quietest channel by 64/24 — its texts a
  table a language, the seven, `lib/l10n`.

What Flutter does not carry itself, each platform does its own way: the wired network on Android
and iPhone (*The link*) and Bluetooth LE for the Wi-Fi's name and password — a plugin each, or a
few lines of the platform's own code called from Dart. The Python tools stay the reference: what
the Dart does, they did first.

## What it does not do

- **Where the animal goes**: no position and no map of the ground — the set's map is the radio's;
  Strelka records only the motion ([`README.md`](README.md)).
- **The animal in 3D**: no skeleton, no tendons drawn moving. A phone shows what can be read on it,
  the curves and the numbers in a stride, left against right; the details of a 3D model no one would
  see on it. The record is open, C3D: whoever wants it whole in 3D, on a screen four metres wide,
  opens it in the tools of biomechanics — [Mokka](https://biomechanical-toolkit.github.io/mokka/),
  [ezc3d](https://github.com/pyomeca/ezc3d) in Python, commercial suites such as Visual3D — or
  builds a system of their own.
