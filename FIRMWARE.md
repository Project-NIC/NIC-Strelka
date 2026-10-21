<div align="center">

# The firmware

**What the Belka's and Povodok's firmware do, each on its own.**

↑ [NIC-Strelka](README.md)

</div>

---

The Belka's and Povodok's firmware are two projects in [`firmware/`](firmware/): the Belka
measures, writes its card and sends; Povodok commands the set, gathers it and talks to the phone.
The radio's code — Strelka's own frame, ESB and SMP — they share. What the units say to each other
over the radio is in [`PRINCIPLE.md`](PRINCIPLE.md), the record in [`RECORD.md`](RECORD.md), the
phone in [`PHONE.md`](PHONE.md); here is what the firmware does on its own.

## The principle — no computation in the unit

A Belka reads its sensor, stamps the samples, stores them and sends them, and nothing else. The
samples are kept as the sensor gives them, or summed N to one, N chosen per session; the rate into
memory runs from 50 Hz to 32 kHz, raw from 4 kHz. A sum is no filter and no model — the gyro's sum
is the turn over that time, the accelerometer's the change of velocity. **A sum is stored as its
mean**, the sum divided by N, in the sensor's own 16 bits: a sample's noise, some 1–1,6 mg at 1 kHz
(70 µg/√Hz over the filter's band), is larger than the step the division loses, 0,49 mg at ±16 g and
0,98 mg at ±32 g. The file's head carries N and the ranges, so the application converts. The one
computation in a Belka is the live view's: its turn as quaternions from its accelerometer and gyro —
the gyro integrated step by step, its tilt held to gravity and its bias learned by Mahony's
complementary filter (Mahony, Hamel and Pflimlin, IEEE Transactions on Automatic Control 53(5),
2008), the heading free, no magnetometer — or the raw steps, as the session says; the record carries
none of it. No other filter, no spectrum, no measure runs in the unit; every analysis is the
application's or a computer's, from the record, so that each use takes the method it needs — the
lameness measures in [`analysis/`](analysis/) are one method among any.

## The platform

- **nRF Connect SDK**, Nordic's SDK on Zephyr, which carries the nRF54LM20A and its development
  kit from v3.1.1 and supports them from v3.3.0 — Bluetooth LE, ESB, Wi-Fi with the nRF7002,
  MCUboot's DFU, the high-speed USB — v3.4.0 with long-term support
  ([Nordic](https://nrfconnectdocs.nordicsemi.com/ncs/latest/nrf/releases_and_maturity/releases/release-notes-3.3.0.html));
  Fanstel asks v3.3.0 or newer for the BM20 series (its specification v1.00).
  The firmware is built for the **nRF54LM20-DK** as the target `nrf54lm20dk/nrf54lm20a/cpuapp`,
  the nRF54LM20A emulated on the kit's nRF54LM20B
  ([Zephyr](https://docs.zephyrproject.org/latest/boards/nordic/nrf54lm20dk/doc/index.html)),
  the same chip as the Belka's and Povodok's modules.
- **MCUboot**, the boot loader, for the images that come over the radio and over USB.
- **The Belka:** Strelka's own frame and ESB on the one radio, taking turns within the period.
- **Povodok:** the same, and the Wi-Fi, the nRF7002, at 5 GHz, clear of the set's band; Bluetooth LE
  in its first minutes after it is switched on alone, to receive the network's name and password.
- **On the first boards:** ESB on the nRF54LM20A and its turns with Strelka's frame — ESB takes the
  radio for itself alone, and no one documents it sharing one with a frame of one's own: the first
  thing tried on the development kit. Bluetooth LE and ESB clashed on the nRF54L series under the
  SDK's v3.0.0 ([Nordic
  DevZone](https://devzone.nordicsemi.com/f/nordic-q-a/123777/esb-and-ble-concurrent-issues-on-nrf54l-series/546350));
  v3.3.0 lets ESB share MPSL's timer and run in its timeslots, experimentally. Povodok's Bluetooth
  LE, in its first minutes, is switched off before ESB and the frame take the radio, Nordic's own
  order, and sends at the default +5 dBm.

## A session

1. **Switched on.** A Belka by magnet A, held 1 s; Povodok by its cable or by magnet A.
2. **A new Belka**, or one after the factory settings, listens on 2480 MHz and the search's
   address. The application starts the search: Povodok sends its CALLs, the Belka its HERE in its
   window; then by SMP the two agree a key by ECDH, and the Belka is given the set's key, its
   number, the set's address and channel, and moves to them.
3. **The phone** joins one Povodok, by USB or Wi-Fi, which becomes the first; its password is
   checked by a challenge and its answer.
4. **The session** is set by SMP: the rate, N, the ranges, the live view, the seconds without a
   SYNC that stop the record; the session's number, the date and time and the time zone. Without a
   phone Povodok gives the number, and the time is the last the phone gave it, or "unknown". The
   sensors start, and with them **the warm-up**: half a minute to a minute untouched, the live
   view already showing.
5. **The calibration**, six faces, led by the application (*Calibration*, below).
6. **The set's table** goes from the application through Povodok to every Belka by SMP; each Belka
   writes it right after the head of every file.
7. **On the animal**: the live view and the set's map; nothing is recorded yet.
8. **The start.** The application, or magnet B on Povodok, sets the SYNC's state to "the record
   runs"; a Belka records from the period after the first SYNC heard saying so. The sensor already
   runs — the gyro needs 30 ms to its drive being ready and is kept on 45 ms at least (DS-000347,
   table 1 and 14.36) — and the clock has locked, within a second of the first SYNC.
9. **Live**, every Belka sends its LIVE in its slot; the first Povodok gathers the set — the MISS
   and the PASSes — and passes it to the phone.
10. **The stop.** The SYNC says "the record does not run": a Belka stops after five such SYNCs and
    writes its file's end. Ten seconds without a SYNC are a pause alone, no end written; a SYNC
    saying the record runs takes it up again in the same session.
11. **The download**: over the radio through the first Povodok, the others joined over Wi-Fi for
    the reach; or a Belka on its cable as a disk; or the set in the dock.

## The inside link

What the units say to each other is in [`PRINCIPLE.md`](PRINCIPLE.md), *The inside link*. The
firmware adds:

- **Every send and every receive is started by a timer in hardware**, through the DPPI, no processor
  in the path; the software only makes the packet ready before. A packet whose instant has passed
  is dropped, never sent late into another's slot.
- **The SYNC's arrival is stamped by a timer in hardware**, for the clock's loop.
- **A Belka sends its LIVE in its slot in every period**, a SYNC heard or not; after ten seconds
  without one it stops sending and listens.
- **During a download** the radio turns within the period between Strelka's frame and ESB; what goes
  by SMP, in which frames and how a Belka is reached through Povodok is in
  [`COMMANDS.md`](COMMANDS.md).

## The sensor and its clock

- **CLKIN**, 32 kHz, is made by a timer of the 16 MHz peripheral clock on a pin, 500 ticks a
  period; the sensor's data-ready pin is captured by another timer in hardware. A PI loop lengthens
  or shortens one period by a tick, one in 67 periods for 30 ppm — the loop and its figures in
  [`PRINCIPLE.md`](PRINCIPLE.md).
- **The set's grid is the first Povodok's timer**: period p's SYNC leaves at its period 0 plus p
  times 20 ms, to the tick, the send started in hardware; the software only wakes 2 ms before to
  make it ready, on a work queue of its own above every thread, so that no other work holds it
  back. A receiver's delay from the packet's end to its END event is measured on the kit and
  taken out once, and what wanders in it from period to period is measured there too.
- **The fallback**, should the sensor not bear a period a tick longer: CLKIN straight, 500 ticks
  every period, and the samples put onto the grid by the application from the offsets in the
  information blocks.
- **The ICM-42688-P and the ICM-42686-P** share one land and one register map; the firmware
  reads WHO_AM_I at 0x75 — 0x47 the ICM-42688-P (DS-000347, 14.58), 0x44 the ICM-42686-P
  (DS-000639, 14.58), 0xDB the ICM-42688-V, taken as a -P (DS-000439, 14.58) — and offers the
  phone the ranges that chip has, ±16 g and ±2 000 °/s or ±32 g and ±4 000 °/s, nothing else
  differing.
- **The sensor's pins at reset:** INT1 and pin 9 wake as open-drain outputs, active low
  (INT_CONFIG, reset 0x00), so the data-ready is set push-pull and active high before it is
  captured, and CLKIN is driven only once the sensor is set to take it — RTC_MODE in INTF_CONFIG1
  and PIN9_FUNCTION in INTF_CONFIG5 (DS-000639, 12.7); INT_ASYNC_RESET
  in INT_CONFIG1 is cleared as well, as the sheet asks, and the pulse set to 8 µs with no
  de-assert hold, which the sheet requires at 4 kHz and above (INT_TPULSE_DURATION,
  INT_TDEASSERT_DISABLE).

## Reading the sensor

- **At a gait's rates**, up to some 2 kHz, the main core reads the data registers at every
  data-ready, 14 B from TEMP_DATA1 at 0x1D through the gyro's z, one burst over SPI at 8 MHz, some
  20 µs (DS-000347, the register map). The stamp is taken in the data-ready's interrupt and queued,
  so that every sample keeps its own; the registers hold the newest sample alone, so a reader
  lagging a sample behind pairs a stamp with the sample after it — counted, and seen in the record
  as a repeated step dropped; the FIFO alone would prevent it.
- **At the raw rates, 4–32 kHz**, the chip's RISC-V coprocessor, the FLPR, reads every sample at its
  data-ready, stamps it and lays it in RAM shared with the main core, which builds the blocks and
  writes the card: one core reads, the other writes and sends. The bus bears it — 14 B at 8 MHz take
  14 µs of every 31 µs at 32 kHz — and every sample keeps its own stamp. The SDK, v3.4, carries the
  FLPR of the nRF54LM20A as experimental, its board file tested with GPIO and counters alone. **The
  fallback**, should the coprocessor not prove itself on the kit: the sensor's FIFO, 2 048 B
  (DS-000347, 6.3), emptied in bursts every millisecond by a timer, the data-ready captured in
  hardware all the while, its interrupt off; the temperature from each packet's one byte, 64 times
  the register's scale (DS-000347, 6.1 and 14.6).
- **Every sample by its number on the set's grid.** A sample's data-ready stamp, against period 0,
  gives its number — the samples stand on the grid, steered there, so the nearest grid point is
  theirs; in a FIFO's burst the newest is numbered by its stamp and the rest counted back, their
  offsets in the information block counted with them. Step k of the record sums the N samples
  numbered from the first sample of the period the record began in, plus k × N: every Belka's step
  k the same N instants, whenever its sensor began. A sample lost leaves its step the mean of the
  rest; a step lost is marked, its six values −32 768, and counted in its block's steps missing
  ([`RECORD.md`](RECORD.md)).

## The pins

The pins are the nRF54LM20A's, from its datasheet v1.0: the pin table (10.1.6, table 83), the
clock pins marked there, the rules that a peripheral uses the pins of its own power domain and a
serial peripheral the pins of one port (10.1; 8.9), and the ports' speeds (table 41); the
module's pads that carry them from the BM20 series' specification, version 1.00, the pin table;
the board's nets from [`BOARD.md`](BOARD.md). On the development kit none of them is taken by its
debugger, its buttons, its LEDs or its crystal (the kit's guide v0.7.0, tables 1–4 and 10).

**Both units:**

| signal | the nRF54LM20A | the module's pad | why there |
|---|---|---|---|
| the gauge's SCL, SDA | P0.04, P0.03 | C2, E7 | TWIM30 in the low-power domain; SCL on a clock pin |
| the charger's STAT, /PG | P0.00, P0.01 | A5, D2 | inputs, held up by R6 and R7 |
| magnet A, through Q2 | P0.05 | D9 | an input, the processor's own pull-up on it |
| magnet B | P0.02 | A6 | an input, the processor's own pull-up on it |
| HOLD | P1.23 | H6 | an output, set by the bootloader's first instructions |
| the light, green and red | P1.30, P1.31 | J5, K9 | outputs on high drive, sinking the cathodes |
| USB: D+, D−, VBUS | dedicated | G7, G6, H7 | the contact's pair; VBUS from U2 (8.27.1) |
| SWDIO, SWDCLK, nRESET | dedicated | J3, K3, G2 | Tag-Connect's pads inside the box |

**The Belka alone**, the BM20F:

| signal | the nRF54LM20A | the module's pad | why there |
|---|---|---|---|
| the sensor's SCK | P3.04 | C3 | SPIM21 in the peripheral domain, on a clock pin; P3 runs at 8 MHz at most, the sensor read at 8 MHz |
| the sensor's SDI, the processor's SDO | P3.05 | H2 | SPIM21 |
| the sensor's SDO, the processor's SDI | P3.07 | C9 | SPIM21 |
| the sensor's CS | P3.02 | D7 | SPIM21; R11 holds it high until the processor runs |
| the sensor's data-ready, INT1 | P3.01 | E6 | GPIOTE20, which serves P1 and P3 (the instances' table), to a timer of the same domain, captured in hardware |
| CLKIN, to the sensor's pin 9 | P3.00 | K5 | made by a timer of the same domain through GPIOTE20 |
| the card's CLK, DI, DO, CS | P3.03, P3.08, P3.09, P3.10 | A7, D4, K7, K2 | SPIM20 in the peripheral domain, CLK on P3's other clock pin; the card's identification at 400 kHz, then 8 Mbit/s (11.19.1) |
| the card's supply, U6's EN | P3.06 | D8 | an output; U6 holds it low by 500 kΩ until it is driven |

P1.04–P1.14 are free on the BM20F and given a pull.

**Povodok alone**, the WM02C20C, its nRF7002 on pins inside the module, as Fanstel's example
wires it (the BM20C series' specification, the pin table and its Wi-Fi overlay):

| signal | the nRF54LM20A | why there |
|---|---|---|
| the Wi-Fi's MOSI, MISO, SCK, CS | P1.06, P1.07, P1.11, P1.10 | SPIM22 in the peripheral domain, 8 MHz |
| the Wi-Fi's IRQ, BUCK-EN | P1.14, P1.04 | the nRF7002's interrupt and its buck |
| the Wi-Fi's IOVDD | P1.05 | the module's TCK127BG, the Wi-Fi off at 3 nA |
| coexistence: status 0, request, grant, status 1 | P1.08, P1.09, P1.12, P1.13 | the radio and the Wi-Fi sharing the air |

The kit's own Wi-Fi shield, the nRF7002-EB II, sits on other pins of the nRF54LM20 DK, so the
module's overlay repins it to these — the nRF54L15 DK's pinout, which Fanstel copied.

The sensor's and the card's pads, P3.00–P3.10, have nothing fitted on Povodok and are given a pull.

- **The 32 MHz crystal's load capacitance is set to 14,5 pF** in the chip, Fanstel's setting
  for ±20 ppm on the carrier from −40 to 105 °C (the series' specification, *Crystal*); the
  development kit keeps its own 15 pF. The module's 32,768 kHz crystal is never started.
- **Left free:** every other GPIO, given a pull. **Left unconnected:** P1.01 and P1.02, the NFC
  pins from reset (8.9.1).
- **The SYNC's arrival** is an event of the radio's domain; it reaches the timer of the peripheral
  domain through the DPPI's bridge, PPIB11 to PPIB21, with a delay the datasheet does not promise
  constant across the domains (6.1) — the kit measures it, in Constant Latency mode if it must, and
  the loop takes out its mean; the capture keeps off what MPSL and ESB reserve, TIMER10, TIMER20 and
  channel 0 of DPPIC10, DPPIC20, PPIB11 and PPIB21.

## The record

The record on the card is [`RECORD.md`](RECORD.md): files on FAT32 or exFAT, blocks of 512 B, each
ending in a CRC-32. The firmware adds:

- **FatFs in Zephyr**; a session a file, its room taken in one run at the start and its length
  saved every 3 s, so that a power cut loses 3 s at most.
- **Through a buffer of 128 KB in RAM.** The sensor's side puts each finished block into it at
  once, and a writer of its own, below the sensor and the radio, takes the blocks onto the card in
  their order, so that neither the sensor nor the radio ever waits for the card. Should the buffer
  fill all the same — the card busy longer than it bears — a block is dropped, never one written
  late; the blocks dropped are counted in the file's end and in the Belka's state.
- **The CRC-32 by the main core**, some 30 µs a block.
- **The card full**: the record ends, its end saying *the card full*, and the light shows a fault —
  unless the user has allowed overwriting in the application, when as many of the oldest sessions
  are deleted as the room asks.
- **The card erased or formatted** only by the application's command, at the user's word, asked
  again; refused while the Belka records. Nothing else deletes anything.
- **The list of sessions** is read from the card's directory; the Belka keeps none of its own.

## Calibration

- **Six faces after the warm-up**, led by the application: from the live view it computes, and
  writes into each Belka by SMP, the raw readings — the accelerometer with each axis up and down,
  the gyro's scale from the turns between the positions, the gyro's zero at rest, warm, the
  temperature and the time. The Belka keeps them across power-ons and puts them into the head of
  every file ([`RECORD.md`](RECORD.md), *The session's head*); it computes nothing itself.
- **The warming on the animal.** The gyro's zero moves with temperature; the sensor's temperature is
  in every information block, and the application follows the zero by it.
- **The conversion is the application's.**

## USB — the unit on the cable

On the cable a unit shows to the computer over the contact's pair at high speed, 480 Mbit/s:

- **A Belka is a disk, read-only**: its card as it is, with its sessions' files. The computer reads
  them; nothing is written to the card nor erased by it — the card is erased by the application's
  command alone. The Belka is switched on by its magnet, and the disk shows only while it does not
  record, the Belka letting its file system go to the host for that time and taking it back when a
  record begins, the disk gone from the host then; the card's bus sets the pace, some 1 MB/s.
  Zephyr's disk class cannot yet refuse a write, so the host is asked not to write, until the class
  can say so itself ([`STATUS.md`](STATUS.md)).
- **Povodok is a network adapter**, USB's CDC-NCM, and TCP/IP runs over it to the application as it
  does over Wi-Fi ([`PHONE.md`](PHONE.md)).
- **Both have a serial port**, the console on the bench and the door of MCUboot's serial recovery,
  in the SDK over USB from v3.3.0 (`CONFIG_BOOT_SERIAL_CDC_ACM`): with VBUS present at power-on —
  the chip's VBUS detector says so — MCUboot brings the port up and listens three seconds for a
  command, a firmware image coming in through it; without VBUS it boots at once. The SDK's own wait
  is unconditional (`BOOT_SERIAL_WAIT_FOR_DFU`) and has no hook, so the VBUS condition is a patch to
  it kept with the firmware (`firmware/patches/`); and the USB driver's own wait for VBUS, which the
  SDK leaves unbounded, is bounded, so that neither MCUboot nor the application can stand still
  without a cable. A unit sealed in its box takes its firmware so, through the contact. A phone
  offers no serial port to an application, so for it the network adapter alone counts
  ([`PHONE.md`](PHONE.md)).

The USB block runs only while VBUS is present (8.27.1); off the cable it is dark and costs nothing.

## The charger and the supply

The supply is set by its resistors, nothing written to it ([`BOARD.md`](BOARD.md), *The
supply*); the firmware holds the board on, reads the charger and sets the gauge once.

- **The hold.** The bootloader's first instructions set P1.23, HOLD, high, and the firmware keeps
  it so while the unit is on; driven low, it switches the board off once magnet A is away too.
  Off is the switch open, some 9 µA from the cell. Through a reset the pins stand high-impedance
  until they are set again (the nRF54LM20A datasheet, 8.9.7), and C37 holds Q1 for 0,69 s at
  least, so that the bootloader takes the hold back before it goes; a hung firmware is reset by
  the chip's watchdog and stays on.
- **Povodok and its cable.** The cable switches Povodok on through Q3 ([`BOARD.md`](BOARD.md),
  *The switch*): plugged in, or a computer switched on, it starts as by magnet A, and it shows on
  the computer's USB. When the cable leaves, Povodok runs on its cell and lets HOLD go 5 min
  later, unless the cable has come back. Switched off while the cable is in — magnet A's hold or
  the application's *off* — it cannot open the switch, which Q3 holds: it lets HOLD go, darkens
  and sleeps in System OFF on the cable's power, magnet A its one wake, and is off when the cable
  leaves; a cable put back starts it. A port that keeps its 5 V with the computer off neither
  starts nor stops it. The Belka is not switched on by its cable: off, it only charges, and the
  computer sees nothing until magnet A switches it on.
- **The charger**, TI's BQ25170, is read on two pins (SLUSDJ8A, the pin table): STAT on P0.00,
  low while it charges, high when the charge is done or not running, blinking at 1 Hz on a
  fault; /PG on P0.01, low while the cable gives it power.
- **The gauge**, ADI's MAX17055, on TWIM30, SCL P0.04 and SDA P0.03, is set the way of ADI's
  MAX17055 Software Implementation Guide (UG6365, rev. 2, steps 1–4, the EZ configuration), and
  only when it has lost its settings — the cell joined — which the POR bit of its Status register,
  00h bit 1, says. The firmware waits until DNR in FStat, 3Dh bit 0, is clear, looking every
  10 ms; takes the gauge out of hibernation — HibCFG, BAh, kept, then 0x90 into 60h, 0 into BAh
  and 0 into 60h; writes the six registers below in their order; waits until the gauge clears
  Refresh in ModelCfg, looking every 10 ms; puts HibCFG back; and clears POR, reading it back.

| register | value | what |
|---|---|---|
| DesignCap, 18h | 0x0B40 | 720 mAh, 0,25 mAh a step: 5,0 µVh over R1's 20 mΩ (the data sheet, the register units) |
| dQAcc, 45h | 0x005A | DesignCap ÷ 32, 90 (the guide, 3.1) |
| IChgTerm, 1Eh | 0x0180 | 30 mA, where the BQ25170 ends the charge, 10 % of its 300 mA; 78,125 µA a step, 1,5625 µV over 20 mΩ |
| VEmpty, 3Ah | 0x9B53 | VE 3,10 V, the empty under load, 0 %; VR 3,32 V, over which the empty is watched again, under the cell's 3,50 V of charge — the default 3,88 V, made for Li-ion, a LiFePO4 cell never reaches (the data sheet, table 2) |
| dPAcc, 46h | 0x0563 | dQAcc × 44 138 ÷ DesignCap, 1 379, the guide's for a charge under 4,275 V (3.1) |
| ModelCfg, DBh | 0x8060 | Refresh with ModelID 6, "for lithium iron-phosphate" (the data sheet, table 3); the guide's 0x8000 is ModelID 0 |

  **What the gauge learns is kept** (the guide, 4.5–4.11). Each time bit 6 of Cycles, 17h, turns —
  every 64 % of the cell's charge passed — the firmware copies RCOMP0 (38h), TempCo (39h),
  FullCapRep (10h), Cycles and FullCapNom (23h), 10 B, into the module's own memory. After a POR,
  once the gauge is set as above, it writes RCOMP0, TempCo and FullCapNom back; waits 350 ms;
  writes MixCap (0Fh), MixSOC (0Dh) × FullCapNom ÷ 25 600, then FullCapRep, then dPAcc, 0x0C80,
  and dQAcc, FullCapNom ÷ 16; waits 350 ms; and writes Cycles back — each write read back. The
  factory settings forget them, so that a new cell on J2 starts clean.

  After that the gauge runs alone, hibernating at a low current and counting the charge with the
  board off as well; nothing puts it in shutdown. The firmware reads RepSOC (06h), the cell's
  per cent, 20 % its low mark, for the light and the unit's state, and VCell (09h).
- **The cell empty**: at 0 %, 3,10 V under load, the record is closed, its end saying *the cell
  low*, and the board switched off.

## The magnets and the light

No button: two magnets and the two-coloured light, red and green, both lit at once for orange.
Magnet A, at its mark on the box, switches the unit on through its Hall switch on the cell
([`BOARD.md`](BOARD.md), *The switch*), and the processor reads it while the unit runs; magnet B,
at the box's other end, is read while the unit runs. A hold acts when it is let go, by how long
it was held, so that a hold of 6 s does not stop at 3 s; while both magnets are held, neither's
own hold counts. The holds and the lights are fixed, the same on every unit and on the box's
label.

| magnet A | what it does |
|---|---|
| off, put on | the board starts at once, in hardware; the firmware, once it runs — some 0,3 s, and 3 s more on a cable, while MCUboot waits for a command — keeps it on while the magnet is still there, and lets go if it has gone: a magnet brushed past does nothing |
| off, held until the light shows | on |
| on, a short hold | shows the cell's state, below |
| on, held 6 s | off, when let go: the record closed, the hold let go, the board dark as the magnet leaves |

| magnet B | what it does |
|---|---|
| on Povodok, the set in preparation, held 3 s | the set starts measuring; the light green for 2 s |
| on Povodok, the set measuring, held 6 s | the set stops; the light red for 2 s |

A hold of B between 3 and 6 s starts, between 6 and 10 s stops, each only in its own state; on a
Belka magnet B alone does nothing.

| both magnets | what they do |
|---|---|
| held together 10 s, the unit on or off | the factory settings: the light orange for 2 s at 10 s, and when the magnets are let go the set forgotten, the settings back to their first values, the calibration and the gauge's learned values forgotten, the records kept, the unit started again |

The factory settings also by the application's command, which asks the user again.

| a short hold of A shows | the cell, of its capacity |
|---|---|
| green, four blinks | 76–100 % |
| orange, three blinks | 51–75 % |
| orange, two blinks | 26–50 % |
| red, one blink | 11–25 % |
| red, fast blinks | 10 % and less |

| the light | means |
|---|---|
| green, slow blinks | in a set, waiting |
| green, fast blinks | measuring |
| green and red in turn | Povodok |
| red blinks | the cell is low, under 20 % — on the charger too, while it charges |
| green blinks, on the charger | charging, the cell at 20 % or more |
| green steady, on the charger | charged |
| orange steady, on the charger | the charger's fault, its STAT blinking |
| red steady | a fault |
| orange, fast blinks | sought by the application, *blink* ([`COMMANDS.md`](COMMANDS.md)) |

In a set the set's light comes first; the charge shows outside a set. With the unit off the light
is dark; the charge goes on all the same.

## The firmware's own updates

MCUboot keeps two images and boots only one signed by its maker. A new one comes from the
application by SMP over TCP to Povodok, and from Povodok by ESB to every Belka, Zephyr's MCUmgr
protocol ([Zephyr](https://docs.zephyrproject.org/latest/services/device_mgmt/mcumgr.html)); a
Belka writes it into the second slot and swaps at its next start when it is whole and its signature
holds. Povodok takes its own straight over TCP. The first image goes in over SWD, through the pads
inside the box, before it is closed; after that MCUboot's serial recovery takes one through the
contact's USB port as well (*USB*, above).

## The code

The firmware is written in [`firmware/`](firmware/): two nRF Connect SDK projects on Zephyr, the
Belka and Povodok, sharing the radio's code and the protocol's, MCUboot beside them. Every packet,
block, header and fragment is one header, `firmware/common/protocol.h`, its sizes asserted; the
protocol's code and the record's are plain C, compiled on the host and run by
[`tools/test_firmware.py`](tools/test_firmware.py), which holds the record the C writes byte for
byte to the one [`tools/strelka_record.py`](tools/strelka_record.py) builds — the reader of the
record, which also writes CSV; [`tools/strelka_c3d.py`](tools/strelka_c3d.py) turns it into C3D,
lining up one session's Belkas by the period's number. Both projects build against the SDK, v3.4.0,
with MCUboot; nothing has run on hardware. What is written, what is not, and what the kit decides
first is in [`firmware/README.md`](firmware/README.md).
