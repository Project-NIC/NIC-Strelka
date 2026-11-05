# The firmware's code

What the firmware does is in [`../FIRMWARE.md`](../FIRMWARE.md); here is what of it is written,
how it is checked, and what the development kit still decides. Two nRF Connect SDK projects on
Zephyr, the Belka and Povodok, share the radio's code and the protocol's; Kryska is the Belka's
project built with Kryska's devicetree and configuration; MCUboot stands beside them.

## The layout

| folder | what |
|---|---|
| [`common/`](common/) | the protocol and the pure logic, plain C11 without Zephyr, compiled on the host for the tests and into both units: `protocol.h`, every packet, block, header and fragment to the byte, its sizes asserted; `crc32`; `record`, the blocks built; `session`, a file composed as the steps come; `steps`, the samples summed by their numbers on the grid; `frame`, the six packets built and read, the SYNC's signature, the search's draw; `timetable`, the period in microseconds; `frag`, an SMP frame in fragments over ESB and back; `quat`, a turn in 4 B; `loop`, the clock loop's arithmetic; `link_rules.h`, the link's rules as numbers of periods; `belka_link` and `povodok_link`, what each side of the inside link says and answers period by period — the record from the SYNC's state, the LIVE and its items, the MISS and the PASSes, the map and the move, the takeover, the search; `smp`, SMP's header, a TCP stream cut into frames, the lock on a connection; `strelka_mgmt`, Strelka's group 64 over zcbor, each command's request read and its answer written, the unit's own doings behind `strelka_ops`; `live_msg`, the live view's message to the phone from the first Povodok's gathering, one or two a period; `orient`, the Belka's one computation, its turn as a quaternion by Mahony's complementary filter; `sensor_ring.h`, the ring between the FLPR and the Belka's main core, its head of 32 B; `imu_tables`, the sensors' rates, ranges and temperatures, one home for both cores; `resample`, the hoof's sensor's samples drawn onto the set's grid from its own clock; `setfile`, the set given to a Belka by its cable, `STRELKA.SET`; `flashrec`, Kryska's sessions raw on its flash — the first erased block by halving, the sessions found backwards, their names; `crypto.h`, what is asked of the chip's cryptography |
| [`shared/`](shared/) | the Zephyr code every unit shares: `clock`, CLKIN and the stamps on the timers; `crypto_psa`, CRACEN through PSA; `pmic`, Nordic's nPM1300 through the SDK's drivers — its events on PMIC_INT, the cable, magnet A on SHPHLD, the charge's state, its LED drivers, ship mode; `power`, how a unit started and how it goes off, ship mode, or on the cable an idle of its own; `light`, the patterns on the PMIC's LEDs, the charge's light given back to the PMIC; `magnet`, A from the PMIC's events, B on a pin; `usb`; `gauge`, Nordic's nRF Fuel Gauge fed from the PMIC's charger, its state kept; `store`, what a unit keeps across power-ons, one home for both; `strelka_mgmt_zephyr`, group 64 registered with MCUmgr; `smp_tcp`, SMP over TCP on Povodok — port 1337 its own, 1337 + n a Belka's, the lock on every connection, a transport of MCUmgr's; `live_udp`, the live view's messages to the phone's address; `phone_net`, the lead's network with DHCP for the phone and the Wi-Fi joined with the stored name and password (the announcement by mDNS and DNS-SD not yet); `ble_prov`, the name and the password over Bluetooth LE in the first minutes; `link.h`, the radio's interface, no code yet; `common.conf`, the Kconfig both share; `vbusgate/`, Strelka's module built into MCUboot — the serial recovery skipped without VBUS |
| [`belka/`](belka/) | the Belka's project: CMake, `prj.conf`, sysbuild with MCUboot, the overlay with the pins of `FIRMWARE.md` and the PMIC's settings, the sensor's driver, `sensor_flpr`, the same calls answered from the ring, the card's writer behind its buffer, the file's room taken in one run and a file a power cut left cut at the start, `main`; Kryska's `boards/kryska.overlay` and `kryska.conf` laid over it, the W25Q01JV in the card's place, the PMIC's BUCK2 and SMP on the serial port, and `flash_rec`, its memory behind the card's interface: the record raw on the flash through the SPI NOR driver, both dies awaited by C2h after the chip erase and before the power-down, the sessions as read-only files of a file system of its own at `/SD:`, `STRELKA.SET` taken into it by 8/0's upload |
| [`flpr/`](flpr/) | the FLPR's program, the sensor alone: found, set at the main core's word, each sample read at its data-ready into the ring; `merge.py`, the two programs merged and signed as one |
| [`povodok/`](povodok/) | Povodok's project, the same shape: the nRF7002 repinned to the module's pins, the network adapter over USB, `main` |
| [`tests/`](tests/) | the host tests in C, a software AES-CCM, SHA-256 and HMAC standing in for the chip's |

## What is checked on the host

`python3 tools/test_firmware.py` compiles `common/` with gcc, `-Wall -Wextra -Werror`, together
with each test and runs it:

- the packets' sizes on the air, 18 to 42 B a LIVE, 22 B a SYNC, 26 a MISS or a HERE, a PASS
  filled to its last byte and read back item by item; the SYNC's signature made and checked, a
  changed period or another set's address caught; the period's timetable as `PRINCIPLE.md`
  computes it; a turn packed into 4 B within 0,003 a component;
- the record's blocks and a whole session composed in C, 130 steps with and without lost ones,
  **byte for byte what [`tools/strelka_record.py`](../tools/strelka_record.py) builds in Python**
  — the firmware and the application hold one reference;
- the samples summed into steps: two Belkas whose sensors began at different moments number
  their steps the same, a sample lost leaves the mean of the rest, a step lost is marked;
- a frame of 2 048 B in 9 fragments of 243 and back; a replay, a tampered byte, a stranger's key
  and a fragment out of order dropped; **the AES-CCM tags held against Python's `cryptography`
  library**, so that the software AES of the tests is itself checked;
- the clock loop against a simulated crystal, ±40 ppm at 1, 8 and 32 kHz, a SYNC in ten lost:
  locked within eleven seconds, the samples within 0,05 µs of the grid over the last ten;
- a Belka's side of the link against scripted SYNCs: the record from the period after the first
  SYNC saying it runs, stopped after five saying it does not, paused after ten seconds without
  any and taken up again; the LIVE every period or once a second, trouble first, the items'
  values; the move said twice and taken in the same period; the spare tried, then every channel;
  a new Belka's window after a CALL; asleep with the live view off, the radio one period in 50, and
  every Povodok gone, off after the setting, a paused record ended "the set silent";
- a Povodok's side against scripted frames: the first after a second's listening, its even and
  odd SYNCs, the items once a minute in a row, the MISS and the gathering, another's repeat a
  period later and its PASS of what the MISS names, the takeover and the yield, the move decided
  from the margin, the search's CALLs and the HEREs found;
- **the whole set simulated** (`tests/test_world.c`): 32 Belkas and 4 Povodoks for some three
  minutes of periods, each Povodok hearing its side's Belkas well and the far ones poorly, every
  loss drawn at random — the gathering held every period against what went over the air, exactly;
  the start, the items, the move with a deaf Belka, the first dying and coming back with the set's
  time unbroken, eleven seconds of silence and the pause, a Belka deaf through two moves, the
  search with a collision, the stop; and the network's gathering, no MISS and no PASS sent, each
  Povodok's own message exactly what it heard;
- **Kryska's sessions on a simulated flash** (`tests/test_flashrec.c`): the first erased block by
  halving in an empty, a part-written and a full memory; the sessions found backwards — closed
  ones by their end, one the power cut off by its last data block, one of the head alone, blocks
  dropped and counted or not; a page the power cut in the middle passed over; the same number
  again named `_2`; the list's cap;
- **SMP and group 64 against Python** (`tests/test_smp.c`): 54 requests built by Python's
  `cbor2` — the lock refused and passed, and written again for the card's command and the
  forgetting, spent by the next frame; every command of the group, the codes for a wrong value,
  a wrong op, an unknown command, a stream cut anywhere — answered by the C and read back in
  Python as `COMMANDS.md` says; the software SHA-256 and HMAC held to `hashlib`. zcbor's C sources
  come from its Python package of the SDK's version, `pip install zcbor==0.9.1`;
- **the live view's messages** (`tests/test_live.c`): a period's gathering with a Belka passed by
  another Povodok, 47 Belkas raw at 100 Hz in one message of 1 400 B and the 48th in a second,
  the state alone in 5 B — read back in Python by `tools/live_view.py` and built there again,
  byte for byte; the quaternion's packing, the smallest three, unpacked in Python within its step;
- **the turn** (`tests/test_orient.c`) against synthetic motion at 100 steps a second: a sensor
  still and tilted takes its tilt from gravity at once; 90° turned in a second integrated within
  2°; a gyro's bias of 2 °/s held under 4° and learned away within thirty seconds, or taken off by
  the calibration; a rocking of ±20° at 1 Hz followed within 3°; the heading from the gyro alone;
- **the set by the cable** (`tests/test_setfile.c`): the file `tools/strelka_set.py` writes, read
  as a Belka reads it; one written by hand, in another order and case, read too; a key missing,
  twice or out of range refused; and 64/19 in the SMP's requests, the set's key opened under the
  write's own lock;
- **the hoof's sensor onto the grid** (`tests/test_resample.c`): the LSM6DSV320X at 7 680 Hz,
  1 % fast and 1 % slow, onto a grid of 4 kHz — every point once and in order, a ramp drawn
  exactly, a sine within 3 steps; three samples lost and the points between left missing; samples
  from before period 0 left; the ring's head byte by byte and the sensors' tables, the LSM6DSV320X's
  rates, ranges and codes among them (`tests/test_ring.c`).

Beside it, **a Povodok simulated in Python** for the application to be written against before a
board exists: [`../tools/povodok_sim.py`](../tools/povodok_sim.py) listens on 1337 and on a port for
each Belka, speaks the lock, group 64 whole, the system and the images groups and, on a Belka's
ports, the files' group with a record built by `strelka_record.py`;
[`../tools/smp.py`](../tools/smp.py) is the client, [`../tools/live_view.py`](../tools/live_view.py)
the live view's receiver, the messages streamed over UDP with a made-up motion in them; `python3
tools/test_povodok_sim.py` drives the one with the other, the record downloaded and read back to the
byte.

The analysis under [`../analysis/`](../analysis/) and the record's tools under
[`../tools/`](../tools/) have their own tests; `.github/workflows/check.yml` runs them all.

## Built, not run

**The three build against nRF Connect SDK v3.4.0** — Zephyr 4.4.0, Zephyr SDK 1.0.1 — with sysbuild,
MCUboot and the signed update package beside the application, for the target
`nrf54lm20dk/nrf54lm20a/cpuapp`, the FLPR's program for `nrf54lm20dk/nrf54lm20a/cpuflpr`; the build
was made on a Linux host without the kit, so **nothing has run on hardware**. What the first build
taught, now in the files: the frame's head is in the radio's own order; MCUboot's recovery over CDC
ACM wants the one CDC node and no console of its own; the kit's console UART is SERIAL20, the card's
SPIM20's instance, so on the kit the console is the USB port alone; MPSL's FEM-only cut, the
Belka's, left two callbacks unresolved on the nRF54LM20A, since the SDK's MCUmgr reboot hook calls
`mpsl_uninit()` whenever MPSL is in — the hook given a prompt in the Belka's `Kconfig` and turned
off in its `prj.conf`, by Nordic's advice, mends it with no patch, and Povodok keeps MPSL whole for
its Bluetooth LE, released by `bt_disable()`; the nRF7002's node wants every band's power named.

**What a review against the pages then found**, read against the SDK's sources, and what was done:
the USB driver waits for VBUS without end unless told otherwise, so MCUboot and the application
would have stood still off the cable — the wait is bounded, the application enables the device on
the stack's VBUS message alone, and MCUboot's wait for a command is gated on VBUS by a patch kept in
`patches/`, since MCUboot has no hook in it and its pin hook, which the module answered before, is
never asked; the data-ready's interrupt was connected to GPIOTE20's line 0 while the application
core's interrupt group is 1, so it never ran — the pin goes through nrfx now, its channels allocated
beside Zephyr's GPIO driver, its handler on the driver's line; every sample carries the stamp taken
in the interrupt, and a lagging reader's repeat is dropped; the sensor's thread owns the record, a
stop from a magnet or the card's fault being a request it serves; a full card is a short write in
FatFs, not an error code; the Belka lets its file system go to the host on the cable and takes it
back to record; the fragment's nonce names the receiver, and the counts outlive a start; the MISS
names its sender; the first hold of magnet A is the switching-on hold, no gesture. Each file says in
its head that it is built and unrun.

| image | flash | RAM |
|---|---|---|
| Belka | 253 kB of 975, the FLPR's 23 kB at `0xE0000` beside it in the one slot — the merged image that is signed and sent spans the gap between them, some 930 kB | 83 kB of RAM_00's 256; the card's buffer of 128 kB, the ring's 17 kB and the FLPR's 96 kB in RAM_01 (`FIRMWARE.md`, The sensor on the FLPR) |
| Kryska | 222 kB of 975, the FLPR's beside it as on the Belka | 76 kB of RAM_00's 256 |
| the FLPR | 23 kB, inside the Belka's image | 28 kB of its 96 |
| Povodok | 644 kB of 975 | 318 kB of 511, the network's 128 contexts for a port a Belka and the eight connections' frames in it — the kit decides whether so many ports are kept or opened on demand |
| MCUboot, each | 87 kB of 968, the watchdog fed | 37 kB |

**What the PMIC's build taught** (step 4 of the three boards): the SDK's LED driver for the nPM1300
keeps the modes its devicetree gives it and refuses a LED not in the host's mode, so the light
writes the PMIC's LED registers itself, through the MFD, and the devicetree names no LED node — the
PMIC's reset modes, the charger's error on LED0 and its charge on LED1, stand until the firmware
takes the light; the charger's driver writes the USB input's limit as the startup limit too, so a
cable plugged in again keeps it; ship mode is asked of the regulators' node,
`regulator_parent_ship_mode()`, so each overlay names that node, with no regulator under it on the
Belka and Povodok; Kryska's build takes its devicetree and configuration through sysbuild's own
variables for the image, the card's SD stack and FatFs left out, which leaves four of the Belka's
FatFs options unmet, a warning each. The fuel gauge's cell model is the SDK sample's, a Li-ion
cell's, until the cell is profiled. The development kit has no PMIC on TWIM30's pins, P0.04 and
P0.03 — its own nPM1300 is set from nRF Connect for Desktop's Board Configurator (the kit's guide,
2.2) — so with no PMIC answering a unit starts as switched on, its light and its gauge silent,
Povodok never off by itself; on the kit the PMIC's trials take Nordic's nPM1300 EK, wired to P0.04,
P0.03 and P0.00.

**What the review of the PMIC's code found** (2 November 2026), read against the SDK's sources
and the sheets, and what was done: a restart off the cable — an update's swap, the factory
settings, a set given over the network, the watchdog — went into ship mode, as the hold's code had
before, so that an update would have been reverted for want of its confirmation; the reset's cause
keeps the unit on now, a mark in GPREGRET[1] saying the off on the cable and the start after the
factory settings. A Povodok switched on by magnet A off the cable went off after 5 min; the count
waits for a cable seen. After a soft reset the light took the PMIC's LEDs for the charger's while
they were still the host's; their modes are written at the start, and the light goes dark at once
on the way off. The factory settings could not be made with the unit off, both magnets held at its
start counting nothing; they are counted from magnet A's coming, and the start after them alone
waits for the magnets to be let go. Kryska's devicetree served the nPM1300 alone, the nPM1304
taking the same numbers as other voltages and currents; its run has an overlay of its own. The
watchdog runs on through a soft reset and MCUboot fed it not: MCUboot feeds it now, and the
firmware until main installs its own. Ship mode had no way out if the PMIC did not take it: it is
tried again each second, a cable come back restarts into the idle, and System OFF follows. Magnet
A's state was read before its events were listened to, a release between them lost; listened to
first now, and the PMIC's events left pending by the last run cleared at the start, so that one
does not hold PMIC_INT high. The fuel gauge was reached from three threads without a lock, the
PMIC's measurement too; both are under one now. The USB input's limit reached the PMIC as its
startup limit alone, 100 mA standing until a replug; it is applied at every start. The empty cell
was the gauge's 0 %, which the sample's Li-ion model would have given on a LiFePO4 cell's plateau;
the end voltage decides now, ten seconds under it. Kryska's stub named its fault at the start, its
light held red; only once a record is asked of it now. BOARD.md gave the NTC's B over 25–85 °C
where the PMIC's table, and the devicetree, take 25–50 °C.

**What a second review of the PMIC's code found** (2 November 2026), and what was done: the factory
settings kept the fuel gauge's state, the restart after them writing the old cell's state back to
the store just forgotten; that restart keeps nothing now. The idle on the cable trusted a
VBUS-removed event once seen, which a bounce of the plug can raise with the cable in, and ship mode
was written before VBUS was read; the idle reads VBUS itself now, and ship mode's tries read it
before each. A unit idling on the cable had the watchdog fed by a timer for ever, so that a hang
there was never reset, and a reset there came back switched on; the timer stops at the idle, which
feeds the watchdog from its own loop, as ship mode's tries do, and sets its mark again, and the
timer feeds for the first minute of a start at the most. The off ran on the sensor thread's 2 048 B,
with the settings' write and the log's flush, an overflow halting the processor and the watchdog's
reset bringing the unit back on; main's thread takes it now, Povodok's too, off the system work
queue. The factory settings restarted after 500 ms, their orange of 2 s cut short; they restart
after it. A record asked with no card, and always of Kryska, whose stub has none, failed with no
fault named; it names *no card* now. Magnet A's press in the idle was a flag tested and cleared in
two steps, a press between them lost; it is taken in one now. A reset by the pin or by a debugger
went into ship mode, the probe losing the target; it keeps the unit on now. The light's drive and
its hand-over to the charger ran from two threads with no lock, so that the dark on the way off
could be skipped; one now. Two are written into `FIRMWARE.md`, not changed: a PMIC that fails at the
start leaves the unit on, as on the development kit, magnet A dead and the PMIC's own 10 s the
rescue; both magnets held on past 10 s bring the PMIC's restart too, and the factory settings made
once more.

**What the simulated set found**, and what was done: the search's draw was the low 16 bits of a
CRC-32 over the ID and the attempt — a CRC is linear, so two Belkas that drew one number at one
attempt drew one number at every attempt and the collision never resolved; the draw is SplitMix64's
now, `PRINCIPLE.md` with it. A Belka that lost the set began its sweep of the channels in the very
second another Povodok stood up as the first, so the whole set went deaf for 0,8 s after every
takeover; the sweep takes the set's own channel every other period now. A Povodok switched on while
another stood as the first took over with its own time and a stopped record; it listens a second
first, taking the set's time and state. A PASS from Povodok 0 read as the receiver's own hearing.

**What a second review found** (21 October 2026), read against Zephyr's sources, and what was done:
the TCP transport's table of connections was reached from three threads with no lock, so an answer
could go to a connection that had been closed and taken by another client — the table and every send
are under one mutex now, a request's buffer carries the connection's count on its slot and an answer
to a connection gone is dropped, and a connection has one request in flight at a time, the stream
read no further until it is answered; the table's bounds check wrapped in 32 bits, so an offset near
the top let a piece be written before the buffer; the card's format named the wrong drive, FatFs
wanting `SD:`; a Belka took the set's time from the last SYNC of a period and another Povodok's
SYNC, a period behind, could undo a move, both from the taken SYNC now; a Belka heard by a first
that was not Povodok 0 read as passed; the factory settings kept the password; a Belka's measurement
passed by another Povodok never reached the map; a search while the set records would have silenced
the SYNCs, refused now; SMP's versions 0 and 1 are taken alike everywhere. What is open of it is
in `STATUS.md`.

**Not written yet**, in the order the kit asks for them:

1. **the link** on the radio, `shared/link.h`: Strelka's frame and ESB taking turns in one period
   by Nordic's way, `esb_init()` and `esb_disable()` every period, the radio handed over clean and
   ESB stopped before the cutoff (`FIRMWARE.md`, *The inside link*) — the first thing tried on the
   kit; the Belka's MPSL cut to FEM only, Povodok's released by `bt_disable()` and the clock's
   DPPI channels taken through `nrfx_gppi` are in the code and built (`FIRMWARE.md`, *The
   platform*), the SYNC's route from RADIO.END to `clock_sync_capture_task()` the link's, by
   `nrfx_gppi_conn_alloc()`; the SYNC's stamp through the DPPI's bridge, its delay measured; the
   frames carried for `common/belka_link.c` and `common/povodok_link.c`, whose logic is written;
2. SMP over ESB: the fragments into MCUmgr on both sides, a transport of its own like
   `shared/smp_tcp.c`; the taking's exchange, 64/8 and 64/9, from Povodok's side; the session
   and the table forwarded to every Belka — group 64 and the TCP server are written, and a
   Belka's port answers "out of reach" until the fragments go; the fragments' counts kept every
   1 024 with them (`COMMANDS.md`, The frame over ESB);
3. the fallback's FIFO at the raw rates, emptied by a timer — its reader `imu_read_fifo` written,
   the timer not — wanted only should the FLPR not prove itself (`FIRMWARE.md`, Reading the
   sensor);
4. the set's map's measurements on the radio (the live view over UDP, the lead's network, the
   Wi-Fi and the name and password over Bluetooth LE are written, unrun);
5. Kryska's disk on the cable, made up of the record for a computer's file manager and the dock,
   read alone; until it is, Kryska's sessions and its set go by SMP on the serial port
   (`KRYSKA.md`, The record on the flash), written and built, unrun — the first thing for the
   kit with a W25Q01JV on its SPIM20 pins: the erase on both dies, a record to the memory's end,
   the power pulled mid-page, the sessions back by `tools/smp.py`;
6. the fuel gauge's model of each cell, made by Nordic's nPM PowerUP from the cell on the bench,
   in the place of the SDK sample's (`FIRMWARE.md`, The PMIC);
7. the FLPR idle between two data-readies, woken by the data-ready's interrupt — GPIOTE20's
   second line or the VPR's event interface, the kit's to settle — its loop polling the stamp at
   the core's full speed until then (`FIRMWARE.md`, The sensor on the FLPR).

## Building

nRF Connect SDK v3.4.0 (`FIRMWARE.md`, The platform), set up as it was for the first build, on
Linux with Python 3.12 or newer, cmake, ninja, `dtc` and `gperf`:

```
python3 -m venv ncs-venv && . ncs-venv/bin/activate && pip install west
west init -m https://github.com/nrfconnect/sdk-nrf --mr v3.4.0 ncs && cd ncs
west update --narrow -o=--depth=1
pip install -r zephyr/scripts/requirements-base.txt -r nrf/scripts/requirements-build.txt "cbor2<5.7"
```

The Zephyr SDK 1.0.1: its minimal bundle unpacked, the Arm toolchain's tarball and the RISC-V one,
`riscv64-zephyr-elf`, for the FLPR, unpacked **inside its `gnu/` folder**, then `./setup.sh -c`;
`ZEPHYR_SDK_INSTALL_DIR` names it. Then the two patches, once, MCUboot's into the workspace's
`bootloader/mcuboot` and the USB disk's write-protection into `zephyr` (`patches/patches.yml` says
what each is), and the builds, from the workspace:

```
west patch -b <repo>/firmware/patches -l <repo>/firmware/patches/patches.yml apply
```

```
west build -b nrf54lm20dk/nrf54lm20a/cpuflpr --no-sysbuild -d build-flpr <repo>/firmware/flpr
west build -b nrf54lm20dk/nrf54lm20a/cpuapp --sysbuild -d build-belka <repo>/firmware/belka
python3 <repo>/firmware/flpr/merge.py build-belka build-flpr
west build -b nrf54lm20dk/nrf54lm20a/cpuapp --sysbuild -d build-povodok <repo>/firmware/povodok
west build -b nrf54lm20dk/nrf54lm20a/cpuapp --sysbuild -d build-kryska <repo>/firmware/belka -- -Dbelka_EXTRA_CONF_FILE=kryska.conf -Dbelka_EXTRA_DTC_OVERLAY_FILE=boards/kryska.overlay
python3 <repo>/firmware/flpr/merge.py build-kryska build-flpr
```

Kryska's run on the nPM1304, its overlay laid over Kryska's:

```
west build -b nrf54lm20dk/nrf54lm20a/cpuapp --sysbuild -d build-kryska <repo>/firmware/belka -- -Dbelka_EXTRA_CONF_FILE=kryska.conf "-Dbelka_EXTRA_DTC_OVERLAY_FILE=boards/kryska.overlay;boards/kryska_npm1304.overlay"
```

The Belka is two programs, the FLPR's apart, as Nordic's guide asks
(`nrf54l_signing_with_payload.rst`): `merge.py` merges them and signs the whole with the key and
the command the Belka's build used, into `build-belka/belka_flpr.signed.hex`, flashed after
MCUboot, and `belka_flpr.signed.bin`, the update. On the kit, untried:

```
west flash -d build-belka --domain mcuboot
west flash -d build-belka --domain belka --hex-file build-belka/belka_flpr.signed.hex
```

The sensor on the main core instead, the fallback, one program, flashed whole by `west flash`:

```
west build -b nrf54lm20dk/nrf54lm20a/cpuapp --sysbuild -d build-belka <repo>/firmware/belka -- -Dbelka_EXTRA_DTC_OVERLAY_FILE=<repo>/firmware/belka/boards/sensor_on_app.overlay -Dbelka_CONFIG_STRELKA_SENSOR_FLPR=n
```

**The console over RTT**, for the development kit alone: the log through the kit's own J-Link on
its DEBUGGER USB cable, so that no second cable is needed to read it, with the extra configuration
`shared/rtt.conf` added to the build, read with SEGGER's RTT Viewer or `JLinkRTTClient`; built for
the Belka, not run, `merge.py` then run on its folder as on `build-belka`:

```
west build -b nrf54lm20dk/nrf54lm20a/cpuapp --sysbuild -d build-belka-rtt <repo>/firmware/belka -- -Dbelka_EXTRA_CONF_FILE=<repo>/firmware/shared/rtt.conf
```

Without a key named, sysbuild signs the images with MCUboot's own development key and says so;
the maker's key, which is not in the repository, goes in as `SB_CONFIG_BOOT_SIGNATURE_KEY_FILE`
on the command line. The module's overlay differs from the kit's in one block, the crystal's load
capacitance, commented out in the overlays.
