<div align="center">

# The parts

**The types, packages and processors, each naming the requirement it meets.**

↑ [NIC-Strelka](README.md)

</div>

---

The board is built for JLCPCB's assembly, its basic parts first; a value missing from them is
made of basic parts in parallel, which also lowers the series resistance and inductance; a
capacitor is chosen by what it keeps at its working voltage ([`BOARD.md`](BOARD.md), *The
capacitors*). *Basic* and *extended* are JLCPCB's assembly classes.

## One board, two fittings

The Belka and Povodok are one board: the Belka with its sensor and its card, Povodok with the
combo module that adds Wi-Fi, without the sensor and the card. B is the Belka, P is Povodok.

| block | part | package | at JLCPCB | what it meets | B | P |
|---|---|---|---|---|---|---|
| radio and processor | Fanstel BM20F, the nRF54LM20A with a high-performance PCB antenna | LGA-90, 10 × 20,8 mm, 15,2 at the antenna | Fanstel | the set's own radio at 2,4 GHz, Strelka's frame and ESB, and USB | ✓ | — |
| radio, processor and Wi-Fi | Fanstel WM02C20C, the nRF54LM20A and the nRF7002 in one module, two chip antennas | LGA-90 on the same pads, 10,0 × 24,0 mm | Fanstel | Wi-Fi 6 to the phone or the computer | — | ✓ |
| motion sensor, one land for two | TDK ICM-42686-P, ±32 g, or ICM-42688-P, ±16 g | LGA-14, 2,5 × 3 mm | extended (C19191783; C1850418) | CLKIN for the common time, raw data up to 32 kHz | ✓ | — |
| memory | the user's own microSD card in SPI mode, in Megastar's ZX-TF-281.6J slot | SMD, 14,4 × 13,6 mm | extended (C7419839) | the whole record, a card of up to 2 TB | ✓ | — |
| charger | TI BQ25170, linear, set by resistors; fed by TI's TPS62933, a buck at 1,2 MHz with Sunlord's 2,2 µH WPN201610H2R2MT, 3,93 V from the cable; TI's LM66200, two ideal diodes in one, to SYS, the board's switch, with two onsemi BSS138, a third on Povodok, and CJ's BAT54 | WSON-8, 2 × 2 mm; SOT-583, 1,6 × 2,1 mm; 2016; SOT-583; SOT-23 | extended (C5208201; C3200405; C97005; C3235556; C52895; C8590) | LiFePO4 to 3,50 V at 300 mA, nothing written to it; on the cable the board runs on the buck, the cell resting; some 0,25 W lost; stopped under 0 °C and over 45 °C by the cell's NTC; switched on by magnet A, on Povodok by the cable too, and off by the processor, some 9 µA off | ✓ | ✓ |
| fuel gauge | Analog Devices MAX17055, with Ralec's RLM10FTSMR020, 20 mΩ | TDFN-10, 2 × 2,5 mm; 0805 | extended (C2976422; C163095) | the cell's state by counting its charge, LiFePO4's model | ✓ | ✓ |
| supply | TI TPS7A2030, two, 3,0 V, 300 mA, 6,5 µA of its own | X2SON-4, 1 × 1 mm | extended (C48581094) | +3V0 for the module and the rest; +3V0_SD for the card, the second on the Belka alone | ✓ | one |
| the sensor's filter | ferrite bead Murata BLM18PG121SN1D, 120 Ω at 100 MHz, 50 mΩ | 0603 | basic (C14709) | the rail's ripple kept off the sensor | ✓ | — |
| light | Lite-On LTST-C195KGJRKT, red and green, with two 1 kΩ | 1,6 × 1,5 mm | extended (C125104) | two colours from the processor's pins | ✓ | ✓ |
| the magnets' switches | TI DRV5032ZEDBZR, two, Hall switches of open drain, ±63 mT at most | SOT-23 | extended (C266118) | the unit switched on and worked by a strong magnet through the closed box | ✓ | ✓ |
| the cell's connector | JST S3B-PH-SM4-TB, 3 pins at 2 mm, 2 A, 100 V, −25 to 85 °C (JST's PH drawing) | SMD, side entry | extended (C265101) | BAT, BATT− and the cell's NTC | ✓ | ✓ |
| the inlet's guard | SMF12CA, a two-way TVS; TI's TPD2E2U06DCKR on the USB pair; onsemi's NCP361 before the processor's VBUS | SOD-123FL; SC-70; TSOP-5 | extended (C123801, C1855726, C233714) | a surge, a discharge, and the processor's VBUS kept under 6 V | ✓ | ✓ |
| capacitors, resistors | 100 µF 6,3 V 1206; 10 µF 50 V 1206; 22 µF 25 V 0805; 1 µF 50 V 0805; 470 nF 25 V 0603; 100 nF, 10 nF and 1 nF 50 V 0402; 1 MΩ, 100 kΩ, 47 kΩ, 12 kΩ, 10 kΩ and 1,0 kΩ 0402; 10 MΩ 0603 | — | basic | what each keeps at its voltage; 3,50 V and 3,93 V exactly | ✓ | ✓ |
| cell | LTOPOW LFP103042, LiFePO4, 3,2 V, 720 mAh, with its protection board and NTC | pouch, 10 × 30 × 42 mm | not there | fifteen minutes a charge and far more; fits the box | ✓ | ✓ |
| box | Hammond 1551GFLGY, ABS, IP54, with flanges, glued shut; a hole over the light, filled with clear resin | 50 × 35 × 20 mm | not there | water, mud and a kick; the strap on the flanges | ✓ | ✓ |
| inlet | a five-pin magnetic pogo pair, Promax NF2347-05B in the lid and NC2347-05B on the cable, 1 A, wired symmetric | 27,2 × 8,0 × 7,2 mm | not there | the charge either way round, and USB | ✓ | ✓ |
| SWD | Tag-Connect's TC2030-NL pads, for its TC2030-CTX-NL cable | pads only | copper | the first firmware | ✓ | ✓ |

## The radio and the processor

**Nordic's nRF54LM20A**: Nordic's own proprietary 2,4 GHz mode for the set, at 2 Mbit/s —
Bluetooth's 2M on air — and Bluetooth LE, Povodok's alone in its first minutes, on one chip, with
**high-speed USB**, 480 Mbit/s;
an Arm Cortex-M33 at 128 MHz with 2 MB of non-volatile memory and 512 KB of RAM; 3,3 mA receiving
and 5,0 mA sending at 0 dBm on 3 V; −40 to 85 °C
([Nordic](https://www.nordicsemi.com/Products/nRF54LM20A)). From its datasheet, v1.0
(nRF54LM20A and nRF54LM20B, 4539_001):

- **The nRF54LM20B is the same chip with an AI accelerator**, Nordic's Axon NPU, pin for pin
  the same (table 7); Strelka needs no accelerator, so the A is the part. The **nRF54LM20-DK**
  carries the B and builds for the A as an emulated target.
- **The packages decide the pins** (table 7): QFN52 with 32 GPIO, CSP61 with 40, CSP98 with 66;
  port P3 whole only on the CSP98 (10.1.6) — the package inside both modules.
- VDD 1,7–3,6 V on the chip's own DC/DC (11.15.1); the 32 MHz crystal within ±40 ppm in all for
  Bluetooth LE, ±60 ppm for the proprietary mode (11.10.1), its load capacitors inside the chip;
  **no 32,768 kHz crystal is used** — the module carries one, never started.
- **USB** (8.27.1): D+ and D− are dedicated pins, **not 5 V tolerant**; "The data line does not
  need serial resistors"; a 200 Ω 1 % resistor from TXRTUNE to ground tunes the driver's 45 Ω;
  VBUS, 3,67–5,5 V (11.27.1), 6 V at the most (13), feeds the chip's own 3,3 V regulator for the
  signalling, decoupled at DECUSB — TXRTUNE's resistor and DECUSB's capacitor inside the module.
  Nordic asks protection against a discharge and an overvoltage at a connector exposed to them
  (its reference circuitry's note): the inlet's guard, below.
- SPIM20, SPIM21 and SPIM22, the SPIs of the peripheral domain, take any pins of P1 or P3 and clock
  from 127 kHz to 8 Mbit/s (11.19.1): the sensor and the card go there, each with its clock on one
  of P3's two clock pins (table 83).
- +8 dBm at most in the CSP packages, +7 dBm in the QFN (11.14.4); −95,5 dBm at 1 Mbit/s
  (11.14.5): a budget of 103 dB before the module's antenna.

**The Belka: Fanstel's BM20F**. The radio's layout, a PCB antenna of its own, both crystals, the
DC/DC inductors, DECUSB's capacitor, TXRTUNE's resistor and the approvals come with it. From the
series' product specification, version 1.00, October 2026
([Fanstel](https://www.fanstel.com/s/BM20C-Product-Specifications-cgkb.pdf)), and Fanstel's
libraries for it, 29 July 2026:

- **90 LGA pads, 64 of the CSP98's 66 GPIO brought out** — all but P1.20 and P1.21, which
  carry the module's 32,768 kHz crystal (its pin map) — with D+, D− and VBUS, SWDIO, SWDCLK
  and the reset; the pads used are named in [`BOARD.md`](BOARD.md). Every module of the series
  lies on the same land, the antenna alone differing: the BM20M with a PCB trace, 10,0 × 14,0
  mm; the BM20C with a chip, 10,0 × 16,2; the BM20E with a U.FL, 10,0 × 15,0; the BM20F with the
  high-performance PCB antenna; the BM20P with pads for an outside one, 10,0 × 10,5 (its model
  summaries).
- 10,0 × 20,8 × 2,0 mm, 15,2 mm wide at the antenna: the body with the pads 10 × 9 mm, the
  antenna's part 15,2 × 11,8 mm beyond it, standing out 5,2 mm on one side (the library's
  outline).
- **The evenest antenna of the series round the unit.** Fanstel measures the range between two
  modules as the mean of twelve turns 30° apart: 358 m at 2 Mbit/s for the BM20F, against 189 m
  at 1 Mbit/s for the chip-antenna BM20C and 110 m for the small PCB-antenna BM20M; the BM20C
  and the BM20F radiate the same +12,6 dBm at their best, +8,37 dBm conducted, so the BM20F's
  lead, some 8 dB on the mean, is its evenness at the angles between.
- VDD 1,7–3,6 V, "free of AC ripple" (the notes); VBUS 4,4–5,5 V.
- **The carrier within ±20 ppm from −40 to 105 °C** with the chip's load capacitance set to
  14,5 pF, measured on ten samples; ±50 ppm is the Bluetooth SIG's limit.
- **Mounting**: the body may lie on the ground plane, the antenna area must hang over the
  board's edge or over board without copper or traces, not in the middle of a board, and
  "all external metal at least 30 mm from the antenna area" for the best range — in Belka the
  cell lies nearer; the reach reckoned in [`PRINCIPLE.md`](PRINCIPLE.md) has 23 dB in hand at
  1 m, and a built unit measures it.
- Approvals for the series: FCC ID X8WBM20C, CE and RCM compliant, TELEC R201-260622, KCC
  R-C-F8A-BM20C, NCC CCAL26Y11416T2 for the BM20F; ISED pending; tested −40 to +125 °C
  ([Fanstel](https://www.fanstel.com/bm20cnrf54lm20modules)).
- The BM20BF is the same with the nRF54LM20B, its accelerator left idle; the same firmware runs
  on either.

**Povodok: Fanstel's WM02C20C**, the nRF54LM20A and Nordic's nRF7002 in one module, on the
BM20 series' land. From its product specification, draft 0.82, July 2026, and the BM20 series'
pin table, column WM20:

- **Its pads are the BM20C's**, "footprint compatible" with the series, but for P1.04–P1.14,
  which control the nRF7002 inside the module and are left unconnected on its pads.
- **Wi-Fi 6** by the nRF7002: IEEE 802.11 a/b/g/n/ac/ax, 2,4 and 5 GHz, one spatial stream,
  20 MHz channels, 64-QAM (MCS7), 86 Mbit/s on air; WPA3; the transmitter +5 to +19 dBm.
- **One supply, VDD 1,7–3,5 V**, the nRF7002 fed from it inside; the nRF7002 draws up to
  260 mA sending at its full power on 5 GHz (its product specification), some 290 mA with the
  processor's share, which the +3V0 LDO of 300 mA, limited at 360–730 mA, carries; the module
  switches the nRF7002's supply off inside, Toshiba's TCK127BG, under the processor's P1.05.
- 10,0 × 24,0 mm by the draft, Fanstel's product page giving 10,0 × 25,2 and 14,0 mm at the
  antennas; two chip antennas. The same module comes with two U.FL, the WM02E20E, or with pads
  for two outside antennas, the WM02P20P.
- Its approvals are not yet listed; it is sampled.

The nRF52833 and the nRF52840 are the fallbacks, with full-speed USB; the other modules and
chips weighed are in [`WHY.md`](WHY.md).

## The supply — two LDOs at 3,0 V from SYS

Two TI TPS7A2030 LDOs feed the board from SYS — the cell without the cable, the buck's 3,93 V
with it, through two ideal diodes in one, the board's switch; the only switching regulator is
that buck, which switches while the cable is in:
**U5, +3V0**, always on, for the module, the sensor, magnet B's switch and the light; **U6,
+3V0_SD**, switched by the processor, for the card alone, fitted on the Belka. From the
TPS7A20's data sheet, SBVS338H:

- 300 mA, limited at 360–730 mA; **6,5 µA of its own**, 8,5 µA at most at 25 °C and 10 µA at
  −40 to 85 °C; 0,07 µA disabled; in dropout 140 mV at most at 300 mA in its X2SON; an output of
  0,47–200 µF effective; switched off, the output discharged through 150 Ω; started in
  750–1 150 µs; its EN held low inside by 500 kΩ until it is driven.
- **The X2SON of 1 × 1 mm**: four pads at 0,65 mm in its corners and the thermal pad between
  them, joined to GND on the top layer beside pin 2, no via; TI allows it floating as well.
- **Why 3,0 V**: Povodok's nRF7002 asks 2,9 V at least, the BM20F bears 1,7–3,6 V and the
  WM02C20C 1,7–3,5 V (their specifications, the pin tables) and the LiFePO4 cell gives 3,1–3,5 V
  while it is used. At
  some 290 mA, the Wi-Fi's peak and the processor's, the LDO drops 140 mV at most, so +3V0 holds
  3,0 V down to some 3,15 V of the cell; on the cable SYS carries the buck's 3,93 V. A rail of
  3,3 V, which Fanstel advises for its combos, the cell cannot hold through an LDO.
- **Why an LDO, not the cell straight**: at the top of its charge the cell passes the combo
  module's 3,5 V — the charger's 3,50 V with its spread, its output guard at 103–105 %, up to 3,68 V
  (SLUSDJ8A, VOUT_OVP) — and on the cable SYS carries the buck's 3,93 V and its ripple. From
  3,1–3,5 V to 3,0 V an LDO keeps 86–97 % of the power.
- **The unit asleep**, the processor in System OFF: the Belka some 28,6 µA — the module 0,7 µA
  (the nRF54LM20A's datasheet, IOFF0), U5 6,5 µA, the sensor asleep 7,5 µA (DS-000639), the gauge
  hibernating 7 µA, the two Hall switches 3,2 µA, the charger and its switch some 3,7 µA;
  Povodok some 21 µA, its Wi-Fi off behind the module's own switch, Toshiba's TCK127BG, 3 nA (its
  sheet, the standby current). Fanstel measured its BM20E and BM20XE at 6,1–7,3 µA idle (the
  BM20C specification, *Power consumption*), the mode not given.
- **The unit off**, its switch open: some 9 µA from the cell, the Belka and Povodok alike — the
  gauge hibernating 7 µA, magnet A's switch 1,6 µA, the charger 0,35 µA and the switch 0,05 µA;
  720 mAh would last some nine years on it, by the arithmetic alone.

## The charger, the gauge and the cell

**The charge, set by resistors alone** — nothing written to it, so that it charges right with
the processor dead or its firmware wrong. Three parts and the gauge:

- **TI's BQ25170** (SLUSDJ8A), a linear charger for one Li-ion or LiFePO4 cell, 2 × 2 mm: **3,50 V
  for LiFePO4 by 100 kΩ on VSET** (table 7-1), ±0,5 % — 82 kΩ would give 3,60 V and 62 kΩ 3,70 V;
  at 3,50 V some 5 % of the capacity stays unused, 93,2 % reached against 98,3 % at 3,61 V (TI's
  SLUAAR1, table 2-1), for the cell's life; **300 mA by 1,0 kΩ on ISET**, 270–330 mA,
  under the cell's standard 360 mA; the charge ended at 10 % of it and started again 175–225 mV
  under (VRECHG); the cell's NTC straight on TS, the charge stopped under about 0 °C and over
  about 45 °C, the cell's own window; 0,35 µA from the cell without the cable (IQ_OUT); its input
  bears 30 V and stops above 6,6–6,9 V. Its output carries the cell alone, so the charge ends on
  the cell's own current.
- **TI's TPS62933** (SLUSEA4D), a buck of 3,8–30 V in, 32 V at most, 3 A, SOT-583: **3,93 V** by
  47 kΩ over 12 kΩ, at 1,2 MHz with 2,2 µH; some 95 % at these loads (figure 8-19). It runs on the
  cable's power alone. **The charger fed from 3,93 V loses (3,93 − 3,3) V × 0,3 A, some 0,19 W**,
  where from 5 V it would lose 0,54 W; with the buck's some 0,06 W, some 0,25 W in all.
- **TI's LM66200** (SLVSG04), two ideal diodes in one SOT-583, 40 mΩ, 48 mΩ at most at 3,3 V:
  the higher of the buck and the cell feeds SYS, a current back into either blocked. **On the
  cable the board runs on the buck and the cell rests**: Povodok's Wi-Fi does not draw on it, and
  in the cold the board runs on while the charge waits. Off the cable the cell takes over in some
  6,2 µs (tSW). **Its ON pin turns both off**, and so it is the board's switch: held high by
  2,0 MΩ from the cell, pulled down by magnet A's Hall switch through a BAT54, held down by the
  processor through a BSS138; a second BSS138 lets the processor read magnet A, parting the two
  while the processor is dead. On Povodok a third BSS138, its gate on the buck's output, starts it
  when the cable comes — plugged in, or a computer switched on — and it switches itself off five
  minutes after the cable leaves. The charge goes on with the board off.
- **From the cell without the cable**: running, some 3,7 µA for the chain — the charger's
  0,35 µA, the LM66200's 1,35 µA, 1,65 µA in ON's pull-up and 0,3 µA in the hold's; off, the
  charger's 0,35 µA and the LM66200's 0,05 µA (6.5, at 25 °C).
- The cell is discharged to 3,1 V: the firmware, by the gauge, switches the board off there; the
  cell's own protection board cuts it at 2,3 V. A flat cell is taken up at 16 mA under 1,2 V
  and at 60 mA under 2,0 V (VBAT_SHORT, VBAT_LOWV, for LiFePO4).

**Analog Devices' MAX17055**, a fuel gauge with ModelGauge m5 EZ (its data sheet):

- "Supports Li+ and Variants Including LiFePO4": **ModelID 6, "for lithium iron-phosphate"**, set by
  the firmware — "since LiFePO4 is a challenging chemistry, custom characterization is usually
  recommended" (ModelCfg): the EZ model serves until the cell is characterised for the gauge.
- **The current counted on a 20 mΩ resistor in the cell's minus**, Ralec's RLM10FTSMR020, 1 %, ±100
  ppm/°C, 1/2 W, MnCu (Ralec's RLM series sheet): ±2,56 A of range, ±75 µA of offset, 5,8 mV at 290
  mA, 1,7 mW in it; the gauge's ground is the cell's minus, and the board's ground the resistor's
  other end.
- 7 µA in hibernation, 12 µA at most; the temperature by its own die; I²C at up to 400 kHz.
- **A LiFePO4 curve is flat**, so the cell's state is known by the counted charge, not by the
  voltage alone.

**LTOPOW's LFP103042**, a LiFePO4 pouch, from its product page
([LTOPOW](https://lithium-lifepo4-battery.com/lifepo4-battery-lfp103042-720mah-3-2v/)): 3,2 V, 720
mAh, 2,3 Wh, 10 × 30 × 42 mm; 2,3–3,65 V; charged to 3,65 V ±50 mV, here to 3,50 V; the standard
charge 360 mA, 300 mA here, 700 mA the most; discharged at 360 mA, 700 mA the most, against the
unit's 64 mA at the worst, Povodok with the Wi-Fi on (PRINCIPLE.md, The current); cut off at 2,3 V
by its own board; 2 000 cycles and more; charged at 0–45 °C, discharged at −10–80 °C; its protection
board in every one, a lead on a JST PH plug and an NTC fitted on request, asked of the 103AT's
curve, 10 kΩ and B 3 435 K, pin 1 the positive lead; 5–10 pieces the least order. It fits the 1551G,
10 mm thick in its 20 mm. The page's weights are loose (14 g for this cell, 19 g for the smaller
LFP903035): the cell's own sheet decides.

**The cells that may be fitted**, all LTOPOW's LiFePO4 pouches from the same list, each with its
protection board and the same charger settings; the hours scale with the capacity. The box is
the Hammond 1551 of the next size up where the cell outgrows the 1551G: the 1551H is 60 × 35 ×
20 mm, the 1551K 80 × 40 × 20; whether a cell fits each is read from Hammond's drawing before it
is ordered.

| cell | thickness × width × length, mm | capacity | energy | the box |
|---|---|---|---|---|
| LFP503035 | 5 × 30 × 35 | 300 mAh | 0,96 Wh | 1551G |
| LFP903035 | 9 × 30 × 35 | 600 mAh | 1,92 Wh | 1551G |
| **LFP103042**, chosen | 10 × 30 × 42 | 720 mAh | 2,3 Wh | 1551G |
| LFP603050 | 6 × 30 × 50 | 540 mAh | 1,7 Wh | 1551H |
| LFP103450 | 10 × 34 × 50 | 1 000 mAh | 3,2 Wh | 1551K, 34 mm wide |
| LFP703060 | 7 × 30 × 60 | 900 mAh | 2,9 Wh | 1551K |
| LFP802560 | 8 × 25 × 60 | 900 mAh | 2,9 Wh | 1551K |
| LFP652562 | 6,5 × 25 × 62 | 600 mAh | 1,9 Wh | 1551K |
| LFP604060 | 6 × 40 × 60 | 1 100 mAh | 3,5 Wh | a box of its own, 40 mm wide |
| LFP143060 | 14 × 30 × 60 | 1 800 mAh | 5,8 Wh | 1551K |
| LFP904062 | 9 × 40 × 62 | 1 500 mAh | 4,8 Wh | a box of its own, 40 mm wide |

## The motion sensor — one land for two

The ICM-42686-P and the ICM-42688-P are the same chip with two range tables — the same land, the
same registers, the same CLKIN (DS-000639 v1.0; DS-000347 rev. 1.2). The firmware reads WHO_AM_I
and offers the ranges the chip has; nothing on the board changes. **The ICM-42686-P is the
default**, ±32 g, the more universal for the animals; the ICM-42688-P, ±16 g, has the quieter
gyro. The ICM-42688-V, the -P with TDK's fusion library and WHO_AM_I 0xDB, is read as a -P should
one be fitted.

| | ICM-42688-P | ICM-42686-P |
|---|---|---|
| sheet | DS-000347 rev. 1.2 | DS-000639 v1.0 |
| ranges, at most | ±16 g, ±2 000 °/s | ±32 g, ±4 000 °/s |
| gyro's noise | 2,8 mdps/√Hz | 5,3 mdps/√Hz |
| accelerometer's noise | 65–70 µg/√Hz | 70 µg/√Hz |
| six axes, low-noise | 0,88 mA | 0,70 mA |
| CLKIN | 31–50 kHz; the rates scale by f / 32 kHz (4.10, 12.5) | the same |
| fastest rate | 32 kHz | 32 kHz |
| supply | VDD and VDDIO 1,71–3,6 V | the same |

- **CLKIN at 32 kHz** from the 16 MHz peripheral clock gives the 1 kHz rate exactly; the sheet
  sets no limit on CLKIN's jitter, so the tick's steps are tried on a sample, the fallback ready
  ([`PRINCIPLE.md`](PRINCIPLE.md)).
- **The supply:** a π, 100 nF, the basic ferrite bead, then 2 × 1 µF, 2 × 100 nF and 1 nF; VDDIO
  100 nF. TDK asks 0,1 µF and 2,2 µF on VDD and 10 nF on VDDIO (DS-000639, 4.3, table 11). The
  bead drops some 35 µV at the sensor's 0,70 mA.

## The memory — the user's microSD card

The record goes to a **microSD card in SPI mode**, the user's own: the board asks nothing of it
beyond the SD standard; an industrial card is the user's choice, not the design's. From the SD
Association's Physical Layer Simplified Specification 9.10 and ATP's industrial microSD sheet,
which gives the electrical tables the simplified specification leaves out:

- **In SPI mode a card of up to 2 TB is read**: SDSC to 2 GB, SDHC to 32 GB, SDXC to 2 TB (3);
  "SPI Mode is not supported by SDUC cards" (7).
- **The bus**: SPIM20 at up to 8 Mbit/s (11.19.1), the card itself bearing 25 MHz in its default
  speed (ATP) — some 1 MB/s, against the record's some 0,55 MB/s of raw data at 32 kHz; the card's
  own pauses while it writes are taken by a buffer in RAM.
- **Up to 100 mA** in SPI mode, averaged over a second (ATP, 4.1–4.2); the card on its own LDO,
  switched off while it is not used, its lines low then (6.4.1.5).
- An 8 GB card holds some 4 h of raw data at 32 kHz, some 5 days at 1 kHz
  ([`PRINCIPLE.md`](PRINCIPLE.md), *The memory*).

**Megastar's ZX-TF-281.6J**, the slot: a lid sliding to LOCK in a steel shell, 14,40 × 13,60 ×
1,5 mm, eight contacts at 1,1 mm, 1 A, 50 mΩ at most, 5 000 cycles, −30 to 80 °C (its drawing);
no card-detect switch.

## The light

**One two-colour LED, Lite-On's LTST-C195KGJRKT**: red 631 nm and the classic green 571 nm, both
AlInGaP, 2,0 V typical and 2,4 V at most at 20 mA, 35 and 25 mcd at 20 mA, −55 to 85 °C, two
chips apart on four pads (Lite-On's sheet, BNS-OD-C131/A4). Each colour is sunk by a processor
pin on high drive through 1 kΩ, some 1,1 mA, some 1–7 mcd by the bin: seen by day, not lighting a
room at night. Of the same size: XINGLIGHT's XL-1615SURSYGC, common anode, 570 and 620 nm
(C965838).

## The magnets' switches

**Two TI DRV5032ZE**, Hall switches of open drain (SLVSDC7H): both poles, operating from ±33 to
±63 mT and released from ±30 to ±58 mT, so that a strong magnet held at the mark works them and
a stray one does not; the field sampled at 20 Hz, 13,3–37, so a magnet is seen within 75 ms;
1,6 µA at 3 V, 3,5 µA at most. They sit low in the box, at its two opposite sides, each under a
mark, some 20 mm or more from the contact in the lid, whose magnets could otherwise close them.
**Magnet A switches the unit on**, its switch on the cell itself and its output on the
LM66200's ON; the processor reads it too, and switches the unit off. **Magnet B** is a second
input, its output held up by the processor pin's own pull-up.

## The inlet

**A five-pin magnetic pogo pair**, Promax's NC2347-05B and NF2347-05B, 1 A, from its waterproof
series ([Promax](https://promaxpogopin.com/magnetic-connectors/waterproof/)): the NF2347-05B glued
into the box's lid, the NC2347-05B on the cable. From the NC2347-05B's drawing: DC 12 V, 1 A at
most, 50 mΩ a contact, five pins at 2,2 mm, 1,0 mm of working stroke at 80 gf, 10 000 cycles, −25 to
85 °C, gold over nickel on brass, N35 magnets; the maker asks that the supply come on only once the
pair is mated, against arcing, which a USB charger does not do, so the pads' wear at 5 V and under
0,5 A is watched on the first units. Wired symmetric — VBUS on both outer pins, ground in the
middle, the processor's USB pair between ([`BOARD.md`](BOARD.md), J1) — so that a plug turned round
swaps D+ with D− alone: it charges either way and harms nothing, and the inlet needs no reverse
guard. Behind it the TVS, the ESD guard and the USB supply's guard ([`BOARD.md`](BOARD.md), *The
supply*).

## SWD

**Tag-Connect's TC2030-NL footprint**: six pads at 1,27 mm and three alignment holes, no part
on the board, for the TC2030-CTX-NL cable, whose other end is ARM's 10-pin Cortex connector on
the development kit's Debug out. SWD takes the first firmware and serves the bench; after it the
firmware comes over USB through the contact.
