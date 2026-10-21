# The firmware's code

What the firmware does is in [`../FIRMWARE.md`](../FIRMWARE.md); here is what of it is written,
how it is checked, and what the development kit still decides. Two nRF Connect SDK projects on
Zephyr, the Belka and Povodok, share the radio's code and the protocol's; MCUboot stands beside
them.

## The layout

| folder | what |
|---|---|
| [`common/`](common/) | the protocol and the pure logic, plain C11 without Zephyr, compiled on the host for the tests and into both units: `protocol.h`, every packet, block, header and fragment to the byte, its sizes asserted; `crc32`; `record`, the blocks built; `session`, a file composed as the steps come; `steps`, the samples summed by their numbers on the grid; `frame`, the six packets built and read, the SYNC's signature, the search's draw; `timetable`, the period in microseconds; `frag`, an SMP frame in fragments over ESB and back; `quat`, a turn in 4 B; `loop`, the clock loop's arithmetic; `link_rules.h`, the link's rules as numbers of periods; `belka_link` and `povodok_link`, what each side of the inside link says and answers period by period — the record from the SYNC's state, the LIVE and its items, the MISS and the PASSes, the map and the move, the takeover, the search; `smp`, SMP's header, a TCP stream cut into frames, the lock on a connection; `strelka_mgmt`, Strelka's group 64 over zcbor, each command's request read and its answer written, the unit's own doings behind `strelka_ops`; `live_msg`, the live view's message to the phone from the first Povodok's gathering, one or two a period; `orient`, the Belka's one computation, its turn as a quaternion by Mahony's complementary filter; `crypto.h`, what is asked of the chip's cryptography |
| [`shared/`](shared/) | the Zephyr code both units share: `clock`, CLKIN and the stamps on the timers; `crypto_psa`, CRACEN through PSA; `light`, `hold`, `magnet`, `usb`; `gauge`, the MAX17055 set the guide's way for the LiFePO4 cell, what it learns kept; `charger`, the BQ25170's two pins, its fault's blink told from a level; `strelka_mgmt_zephyr`, group 64 registered with MCUmgr; `smp_tcp`, SMP over TCP on Povodok — port 1337 its own, 1337 + n a Belka's, the lock on every connection, a transport of MCUmgr's; `live_udp`, the live view's messages to the phone's address; `phone_net`, the lead's network with DHCP for the phone and the Wi-Fi joined with the stored name and password; `ble_prov`, the name and the password over Bluetooth LE in the first minutes; `link.h`, the radio's interface, no code yet; `common.conf`, the Kconfig both share; `vbusgate/`, Strelka's module built into MCUboot — the hold set first, the serial recovery skipped without VBUS |
| [`belka/`](belka/) | the Belka's project: CMake, `prj.conf`, sysbuild with MCUboot, the overlay with the pins of `FIRMWARE.md`, the sensor's driver, the card's writer behind its buffer, `main` |
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
  a new Belka's window after a CALL;
- a Povodok's side against scripted frames: the first after a second's listening, its even and
  odd SYNCs, the items once a minute in a row, the MISS and the gathering, another's repeat a
  period later and its PASS of what the MISS names, the takeover and the yield, the move decided
  from the margin, the search's CALLs and the HEREs found;
- **the whole set simulated** (`tests/test_world.c`): 32 Belkas and 4 Povodoks for some three
  minutes of periods, each Povodok hearing its side's Belkas well and the far ones poorly, every
  loss drawn at random — the gathering held every period against what went over the air, exactly;
  the start, the items, the move with a deaf Belka, the first dying and coming back with the set's
  time unbroken, eleven seconds of silence and the pause, a Belka deaf through two moves, the
  search with a collision, the stop;
- **SMP and group 64 against Python** (`tests/test_smp.c`): 33 requests built by Python's
  `cbor2` — the lock refused and passed, every command of the group, the codes for a wrong value,
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
  the calibration; a rocking of ±20° at 1 Hz followed within 3°; the heading from the gyro alone.

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

**Both projects build against nRF Connect SDK v3.4.0** — Zephyr 4.4.0, Zephyr SDK 1.0.1 — with
sysbuild, MCUboot and the signed update package beside the application, for the target
`nrf54lm20dk/nrf54lm20a/cpuapp`; the build was made on a Linux host without the kit, so **nothing
has run on hardware**. What the first build taught, now in the files: the frame's head is in the
radio's own order; MCUboot's recovery over CDC ACM wants the one CDC node and no console of its own;
the kit's console UART is SERIAL20, the card's SPIM20's instance, so on the kit the console is the
USB port alone; ESB's FEM-only cut of MPSL leaves two callbacks unresolved on the nRF54LM20A, so ESB
runs with dynamic interrupts and MPSL whole, which Povodok's Bluetooth LE needs anyway; the
nRF7002's node wants every band's power named.

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
| Belka | 236 kB of 975 | 216 kB of 511, the card's buffer of 128 kB in it |
| Povodok | 617 kB of 975 | 320 kB of 511, the network's 128 contexts for a port a Belka and the eight connections' frames in it — the kit decides whether so many ports are kept or opened on demand |
| MCUboot, each | 89 kB of 968 | 38 kB |

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
the SYNCs, refused now; SMP's first version is refused alike everywhere. Open from it: the lock's
design lets a recorded nonce and its MAC be tried offline against a dictionary.

**Not written yet**, in the order the kit asks for them:

1. **the link** on the radio, `shared/link.h`: Strelka's frame and ESB taking turns in one period
   — the first thing tried on the kit, since no one documents ESB sharing a radio with a frame of
   one's own (`STATUS.md`); the SYNC's stamp through the DPPI's bridge, its delay measured; the
   frames carried for `common/belka_link.c` and `common/povodok_link.c`, whose logic is written;
2. SMP over ESB: the fragments into MCUmgr on both sides, a transport of its own like
   `shared/smp_tcp.c`; the taking's exchange, 64/8 and 64/9, from Povodok's side; the session
   and the table forwarded to every Belka — group 64 and the TCP server are written, and a
   Belka's port answers "out of reach" until the fragments go;
3. the settings' handlers: the set's address, key, channel and the number, the calibration, the
   gauge's learned values, the fragments' counts saved every 1 024; the channel saved again at
   every move, so that a unit switched on later finds the set; the factory settings;
4. the raw rates on the RISC-V coprocessor, the FIFO the fallback (`FIRMWARE.md`, Reading the
   sensor);
5. the set's map's measurements on the radio (the live view over UDP, the lead's network, the
   Wi-Fi and the name and password over Bluetooth LE are written, unrun);
6. the file's room taken in one run at the start, FatFs's `f_expand`, which Zephyr's file API
   does not reach.

## Building

nRF Connect SDK v3.4.0 (`FIRMWARE.md`, The platform), set up as it was for the first build, on
Linux with Python 3.12 or newer, cmake, ninja, `dtc` and `gperf`:

```
python3 -m venv ncs-venv && . ncs-venv/bin/activate && pip install west
west init -m https://github.com/nrfconnect/sdk-nrf --mr v3.4.0 ncs && cd ncs
west update --narrow -o=--depth=1
pip install -r zephyr/scripts/requirements-base.txt -r nrf/scripts/requirements-build.txt "cbor2<5.7"
```

The Zephyr SDK 1.0.1: its minimal bundle unpacked, the Arm toolchain's tarball unpacked **inside its
`gnu/` folder**, then `./setup.sh -c`; `ZEPHYR_SDK_INSTALL_DIR` names it. Then MCUboot's patch,
once, into the workspace's `bootloader/mcuboot` (`patches/patches.yml` says what it is), and the
builds, from the workspace:

```
west patch apply -l <repo>/firmware/patches/patches.yml -b <repo>/firmware/patches
```


```
west build -b nrf54lm20dk/nrf54lm20a/cpuapp --sysbuild -d build-belka <repo>/firmware/belka
west build -b nrf54lm20dk/nrf54lm20a/cpuapp --sysbuild -d build-povodok <repo>/firmware/povodok
```

Without a key named, sysbuild signs the images with MCUboot's own development key and says so;
the maker's key, which is not in the repository, goes in as `SB_CONFIG_BOOT_SIGNATURE_KEY_FILE`
on the command line. The module's overlay differs from the kit's in one block, the crystal's load
capacitance, commented out in the overlays.
