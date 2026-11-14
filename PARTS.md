<div align="center">

# The parts

**The types, packages and processors, each naming the requirement it meets.**

↑ [NIC-Strelka](README.md)

</div>

---

The board is built for JLCPCB's assembly, its basic parts first, then its preferred extended
parts, which economic assembly loads without the feeders' fee the other extended parts bear
(JLCPCB's PCB assembly FAQ, 6 and 7); a value missing from them is made of basic parts in parallel,
which also lowers the series resistance and inductance; a capacitor is chosen by what it keeps at
its working voltage ([`BOARD.md`](BOARD.md), *The capacitors*). *Basic*, *preferred* and
*extended* are JLCPCB's assembly classes.

## Three boards, one core

The three units' base boards, Materinka ([`BOARD.md`](BOARD.md)), share their core — the contact and
its guards, the PMIC, the magnets' switches, the light, the cell's connector, the SWD pads — and
differ by the module, the rails, the sensor and the memory. B is the Belka, K Kryska, P Povodok.

| block | part | package | at JLCPCB | what it meets | B | K | P |
|---|---|---|---|---|---|---|---|
| radio and processor | Fanstel BM20F, the nRF54LM20A with a high-performance PCB antenna | LGA-90, 10 × 20,8 mm, 15,2 at the antenna | Fanstel | the set's own radio at 2,4 GHz, Strelka's frame and ESB, and USB | ✓ | — | — |
| radio and processor, small | Fanstel BM20M, the same chip with a small PCB antenna | LGA-90 on the same land, 10,0 × 14,0 mm | Fanstel | Kryska's board as small as can be ([`KRYSKA.md`](KRYSKA.md)) | — | ✓ | — |
| radio, processor and Wi-Fi | Fanstel WM02C20C, the nRF54LM20A and the nRF7002 in one module, two chip antennas | LGA-90 on the same land, 10,0 × 24,0 mm | Fanstel | Wi-Fi 6 to the phone or the computer | — | — | ✓ |
| motion sensor, one land | ST LSM6DSV16X, ±16 g, the classic build; ST LSM6DSV320X, ±16 and ±320 g, the hoof; TDK ICM-42686-P, ±32 g, the industrial, Zvjozdočka | LGA-14, 2,5 × 3 mm | extended (C5267406; C26633007; C19191783) | the common time on pin 9; the hoof's drawn onto the grid; raw data up to 32 kHz on the ICM | ✓ | LSM6DSV320X | — |
| memory | the user's own microSD card in SPI mode, in Megastar's ZX-TF-281.6J slot | SMD, 14,4 × 13,6 mm | extended (C7419839) | the whole record, a card of up to 2 TB | ✓ | — | — |
| memory on the board | CS CSNP1GCR01-BOW, an SD NAND of 1 Gbit, an SD card's NAND and controller in one package | LGA-8, 8 × 6 mm | extended (C2691593) | 128 MB served as the Belka's card, the same files and firmware | — | ✓ | — |
| PMIC | Nordic nPM1300, on every board | QFN32, 5 × 5 mm | extended (C7501206) | the charge, the power path, the off in ship mode, the cell measured for the fuel gauge, the module's VBUS guarded, the light | ✓ | ✓ | ✓ |
| supply | TI TPS7A2030, 3,0 V; TI TPS7A2033, 3,3 V; 300 mA, 6,5 µA of their own | X2SON-4, 1 × 1 mm | extended (C5220164; C2871641) | +3V0 for the Belka and +3V0_SD for its card; +3V3 for Povodok's Wi-Fi | two at 3,0 V | — | one at 3,3 V |
| Kryska's supply | the PMIC's BUCK2 at 3,0 V, and an LC filter after it: two Sunlord WPN201610H2R2MT, 2,2 µH, and 100 µF | 2016, 2,0 × 1,6 mm | extended (C97005) | 3,0 V from the cell, the buck's ripple kept off the module and the sensor | — | ✓ | — |
| the sensor's filter | ferrite bead Murata BLM18PG121SN1D, 120 Ω at 100 MHz, 50 mΩ | 0603 | basic (C14709) | the rail's ripple kept off the sensor | ✓ | ✓ | — |
| light | Lite-On LTST-C195KGJRKT, red and green, on the PMIC's LED drivers | 1,6 × 1,5 mm | extended (C125104) | the charge shown by the PMIC itself, the rest by the firmware | ✓ | ✓ | ✓ |
| the magnets' switches | TI DRV5032ZEDBZR, two, Hall switches of open drain, ±63 mT at most | SOT-23 | extended (C266118) | the unit switched on and worked by a strong magnet through the closed box | ✓ | ✓ | ✓ |
| the cell's connector | JST S3B-PH-SM4-TB, 3 pins at 2 mm, 2 A, 100 V, −25 to 85 °C (JST's PH drawing) | SMD, side entry | extended (C265101) | BAT, GND and the cell's NTC | ✓ | ✓ | ✓ |
| the inlet's guard | SMF12CA, a two-way TVS; Hongjiacheng's H5VUT2U on the USB pair | SOD-123FL; SOT-23 | preferred (C19077507, C20615824) | a surge and a discharge; the processor's VBUS guarded by the PMIC | ✓ | ✓ | ✓ |
| capacitors, resistors | 100 µF 6,3 V 1206; 10 µF 50 V 1206; 2,2 µF 16 V 0603; 1 µF 50 V 0805; 100 nF and 1 nF 50 V 0402; 150 kΩ 0603; 100 kΩ, 10 kΩ, 1,0 kΩ and 0 Ω 0402 | — | basic | what each keeps at its voltage; 3,0 V from VSET2 | ✓ | ✓ | ✓ |
| cell | the Belka: LTOPOW LFP103042, LiFePO4, 3,2 V, 720 mAh; Kryska: Li-poly or LiFePO4; Povodok: Li-ion or Li-poly; each with its protection board and a 10 kΩ NTC | the Belka's a pouch of 10 × 30 × 42 mm; the others chosen with their boxes | not there | the Belka's fifteen minutes a charge and far more; the others as their boxes hold | ✓ | ✓ | ✓ |
| box | Hammond 1551GFLGY, ABS, IP54, with flanges, glued shut; a hole over the light, filled with clear resin; Ugoljok's built sturdier, of a laminate or the like | 50 × 35 × 20 mm | not there | water, mud and a kick; the strap on the flanges | ✓ | its own, by its board, sturdy as standard | by its board |
| inlet | a five-pin magnetic pogo pair, Promax NF2347-05B in the lid and NC2347-05B on the cable, 1 A, wired symmetric | 27,2 × 8,0 × 7,2 mm | not there | the charge either way round, and USB | ✓ | for now | ✓ |
| SWD | Tag-Connect's TC2030-NL pads, for its TC2030-CTX-NL cable | pads only | copper | the first firmware | ✓ | ✓ | ✓ |

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
  from 127 kHz to 8 Mbit/s (11.19.1): the sensor and the memory, the card or the flash, go there,
  each with its clock on one of P3's two clock pins (table 83).
- +8 dBm at most in the CSP packages, +7 dBm in the QFN (11.14.4); −95,5 dBm at 1 Mbit/s and
  −93 dBm at 2 Mbit/s for packets of 37 B or less (11.14.5); the budget at the default power is
  computed in [`PRINCIPLE.md`](PRINCIPLE.md), *The reach*.

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
  cell lies nearer; what the reach reckoned in [`PRINCIPLE.md`](PRINCIPLE.md) has in hand at 1 m
  is in its table, and a built unit measures it.
- Approvals for the series: FCC ID X8WBM20C, CE and RCM compliant, TELEC R201-260622, KCC
  R-C-F8A-BM20C, NCC CCAL26Y11416T2 for the BM20F, ISED 4100A-BM20C (the specification v1.00);
  tested −40 to +125 °C ([Fanstel](https://www.fanstel.com/bm20cnrf54lm20modules)).
- The BM20BF is the same with the nRF54LM20B, its accelerator left idle; the same firmware runs
  on either.

**Kryska: Fanstel's BM20M**, the series' small PCB antenna on the same land, 10,0 × 14,0 mm: the
same chip, crystals and approvals, the antenna of the series' least range, 110 m between two
modules at 1 Mbit/s in Fanstel's mean against the BM20F's 501 m at 1 Mbit/s, estimated, and 358 m
at 2 Mbit/s, where a
small animal keeps Povodok within metres ([`KRYSKA.md`](KRYSKA.md)).

**Povodok: Fanstel's WM02C20C**, the nRF54LM20A and Nordic's nRF7002 in one module, on the
BM20 series' land. From its product specification, draft 0.82, July 2026, and the BM20 series'
pin table, column WM20:

- **Its pads are the BM20C's**, "footprint compatible" with the series, but for P1.04–P1.14,
  which control the nRF7002 inside the module and are left unconnected on its pads.
- **Wi-Fi 6** by the nRF7002: IEEE 802.11 a/b/g/n/ac/ax, 2,4 and 5 GHz, one spatial stream,
  20 MHz channels, 64-QAM (MCS7), 86 Mbit/s on air; WPA3; the transmitter +5 to +19 dBm.
- **One supply, VDD 1,7–3,5 V**, the nRF7002 fed from it inside; the nRF7002 draws up to
  260 mA sending at its full power on 5 GHz (its product specification), 270 mA at the peak
  Fanstel measured, some 283 mA with the module's processor and radio (`BOARD.md`, *Povodok*),
  which the +3V3 LDO of 300 mA, limited at 360–730 mA, carries, as on Fanstel's own boards; the
  module switches the nRF7002's supply off inside, Toshiba's TCK127BG, under the processor's P1.05.
- 10,0 × 24,0 mm by the draft, Fanstel's product page giving 10,0 × 25,2 and 14,0 mm at the
  antennas; two chip antennas. The same module comes with two U.FL, the WM02E20E, or with pads
  for two outside antennas, the WM02P20P.
- Its approvals are not yet listed; it is sampled, the WT02C40C's reports to come for it, likely in
  the second quarter of 2027 ([`REGULATIONS.md`](REGULATIONS.md)). **Povodok keeps the WM02C20C**,
  the design's own: its two chip antennas inside a compact box, enough for the set's reach and a
  phone's Wi-Fi. Fanstel's certification to come covers the WM02E20E as well, with its external
  antennas (Fanstel's answer), on the same land and the same pins: **for whoever
  builds a Povodok for a long reach**, in a larger box — a dipole such as Fanstel's ANT000 on the
  set's radio, with which the BM20E radiates +14,37 dBm from +8,37 conducted and reaches 490 m at
  2 Mbit/s against the BM20F's 358 m (the BM20C specification v1.00, the model summaries and the
  range table), the set's radio then kept at 10 dBm e.i.r.p. by its power; the Wi-Fi on the
  dual-band ANT036 that Fanstel's WM02E is certified with, 1,52 dBi at 2,4 GHz and 3,13 dBi at
  5 GHz (the WM02 specification v1.00). Another antenna, a directional one too, only as the rules
  allow: one the module's maker names, and the e.i.r.p. kept under the band's limit by the power
  ([`REGULATIONS.md`](REGULATIONS.md), *What the design keeps to*, 3 and 4).

The nRF52833 and the nRF52840 are the fallbacks, with full-speed USB; the other modules and
chips weighed are in [`WHY.md`](WHY.md).

## The supply — Nordic's PMIC, and an LDO or a buck

**Nordic's nPM1300**, U2 on every board, the charge, the power path and the off in one chip, a
QFN32 of 5 × 5 mm, Kryska's too ([`KRYSKA.md`](KRYSKA.md)). From its product specification, v1.1
(4490_483), its pins and their wiring in [`BOARD.md`](BOARD.md), *U2*:

- **A linear charger for Li-ion, Li-poly and LiFePO4**: LiFePO4 to 3,50–3,65 V, Li-ion to
  4,00–4,45 V, in steps of 50 mV, ±1 %; 32–800 mA in steps of 2 mA; JEITA's bands by the cell's
  NTC, 10, 47 or 100 kΩ; the charge ended at 10 or 20 % of its current and started again under
  95 % of its end voltage (6.2). **Off from power-on** until the firmware enables it: a firmware
  that never runs leaves the cell uncharged, never overcharged.
- **The power path** (6.2.10): on the cable the board runs on VBUS and the cell rests once full;
  the cell feeds VSYS through 160 mΩ, up to 1 A by its default limit; VBUS limited to 100 mA from
  every start until the firmware raises it, 1,5 A at most (6.1.1).
- **Ship mode, the off**, 370 nA: the cell parted from the board, left by a low on SHPHLD — magnet
  A — or by the cable (7.4); 0,8 µA of its own running with no buck loaded (table 4).
- **VBUSOUT**, the processor's USB supply guarded against over- and undervoltage, "for nRF5x
  series devices" (6.1.5), in the place of a guard of its own.
- **The fuel gauge**: the cell's voltage, current and temperature measured by the PMIC, its state
  made of them by Nordic's fuel gauge in the SDK, "comparable to Coulomb counters at a
  significantly lower power consumption" (the introduction); "the battery model parameters can be
  created from the nPM PowerUP application", the cell modelled over its temperatures (7.1.7).
- **Two bucks of 200 mA**, 3,6 MHz, 1,0–3,3 V, the first voltage set by a resistor (6.3), and two
  load switches or LDOs of 100 or 50 mA: unused on the Belka and Povodok, BUCK2 Kryska's supply.
- **Three LED drivers**, 5 mA sinks, two lit by the charger itself from reset, its charge and its
  error (6.5); five GPIO, one the interrupt to the processor (6.6); a TWI to the processor.

**TI's TPS7A2030 and TPS7A2033**, LDOs of 3,0 and 3,3 V from VSYS: **U5** on the Belka, +3V0 for
the module, the sensor, magnet B's switch and the PMIC's VDDIO, and **U6** for its card alone,
switched by the processor; **U5** on Povodok at 3,3 V, +3V3 for the WM02C20C. From the TPS7A20's
data sheet, SBVS338H:

- 300 mA, limited at 360–730 mA; **6,5 µA of its own**, 8,5 µA at most at 25 °C and 10 µA at
  −40 to 85 °C; 0,07 µA disabled; in dropout 140 mV at most at 300 mA in its X2SON; an input of
  1,6–6,0 V, which VSYS's 5 V on the cable keeps within; an output of 0,47–200 µF effective;
  switched off, the output discharged through 150 Ω; started in 750–1 150 µs; its EN held low
  inside by 500 kΩ until it is driven.
- **The X2SON of 1 × 1 mm**: four pads at 0,65 mm in its corners and the thermal pad between
  them, joined to GND on the top layer beside pin 2, no via; TI allows it floating as well.
- **Why 3,0 V on the Belka**: the BM20F bears 1,7–3,6 V (its specification, the pin table) and the
  LiFePO4 cell gives 3,1–3,5 V while it is used; an LDO drops 140 mV at most, so +3V0 holds
  3,0 V down to some 3,15 V of the cell. **Why 3,3 V on Povodok**: Fanstel advises it for its
  combos, the module bearing 1,7–3,5 V and its nRF7002 asking 2,9 V at the least, and a Li-ion
  cell gives 3,5–4,0 V between some 20 and 80 % of its charge.
- **Why an LDO, not the cell straight**: on the cable VSYS carries VBUS, some 5 V, past the
  modules' 3,6 and 3,5 V (the nPM1300's specification, 6.1.1), and a full Li-ion cell 4,2 V.
- **Why an LDO on Povodok, not a buck**: from 3,6–4,0 V to 3,3 V an LDO keeps 82–92 %, a buck
  some 90 % — a few per cent of the run — where a buck switching beside two radios asks a filter
  and some 500 mA for the Wi-Fi's peaks of 260 mA; the PMIC's own bucks give 200 mA.
- **The unit asleep**, the radio awake for one period a second, the sensor powered down and the
  processor idle between ([`FIRMWARE.md`](FIRMWARE.md), *The power*): a Belka some 22–30 µA
  ([`PRINCIPLE.md`](PRINCIPLE.md), *The current*), of which the board's own, U5, U6 off, the PMIC
  and the two Hall switches, some 10,6 µA ([`BOARD.md`](BOARD.md), *The Belka*); Povodok's Wi-Fi off
  behind the module's own switch, Toshiba's TCK127BG, 3 nA (its sheet, the standby current). Fanstel
  measured its BM20E and BM20XE at 6,1–7,3 µA idle (the BM20C specification, *Power consumption*),
  the mode not given.
- **The unit off**, in ship mode: some 2 µA from the cell on every board — the PMIC's 370 nA and
  magnet A's switch, 1,6 µA; the cell's own self-discharge is the limit then.

**Kryska's supply**, the PMIC's BUCK2 at 3,0 V from power-on, set by 150 kΩ on VSET2, and an LC
filter after it, the buck's own inductor again and the module's 100 µF ([`BOARD.md`](BOARD.md),
*Kryska*; [`KRYSKA.md`](KRYSKA.md)).

## The cells

**The Belka's cell is charged by its PMIC to 3,50 V at 300 mA**: at 3,50 V some 5 % of the capacity
stays unused, 93,2 % reached against 98,3 % at 3,61 V (TI's SLUAAR1, table 2-1), for the cell's
life; the charge stopped out of the cell's 0–45 °C by its NTC; the cell discharged to 3,1 V, where
the firmware switches the unit off, the cell's own protection board cutting it at 2,3 V. **Kryska's
cell**, Li-poly or LiFePO4, and **Povodok's**, Li-ion or Li-poly, are chosen with their boxes, each
with its protection board and a 10 kΩ NTC of B 3 380 K between 25 and 50 °C, which the PMIC reads
(its specification, table 12) ([`KRYSKA.md`](KRYSKA.md)).

**LTOPOW's LFP103042**, a LiFePO4 pouch, from its product page
([LTOPOW](https://lithium-lifepo4-battery.com/lifepo4-battery-lfp103042-720mah-3-2v/)): 3,2 V, 720
mAh, 2,3 Wh, 10 × 30 × 42 mm; 2,3–3,65 V; charged to 3,65 V ±50 mV, here to 3,50 V; the standard
charge 360 mA, 300 mA here, 700 mA the most; discharged at 360 mA, 700 mA the most, against the
Belka's card's 100 mA at the most and its run's few milliamperes (PRINCIPLE.md, The current); cut
off at 2,3 V by its own board; 2 000 cycles and more; charged at 0–45 °C, discharged at −10–80 °C;
its protection board in every one, a lead on a JST PH plug and an NTC fitted on request, asked of
the 103AT's curve, 10 kΩ and B 3 435 K between 25 and 85 °C, B 3 380 K between 25 and 50 °C (the
nPM1300's table 12), pin 1 the positive lead; 5–10 pieces the least order. Its box is Koroljov's
([`KOROLJOV.md`](KOROLJOV.md)). The page's weights are loose (14 g for this
cell, 19 g for the smaller LFP903035): the cell's own sheet decides.

**The Belka's cells that may be fitted**, LTOPOW's other LiFePO4 pouches, are listed with the box
each asks in [`KOROLJOV.md`](KOROLJOV.md), *The Korabls*.

## The motion sensor — one land, three builds

One land takes three sensors, the pins they share and the two they do not in
[`BOARD.md`](BOARD.md), U7; the firmware tells them by WHO_AM_I and offers the ranges the chip has.
The land is the Belka's and Kryska's, Kryska's fitted with the LSM6DSV320X alone, one build for
every examination ([`KRYSKA.md`](KRYSKA.md)).

| build | sensor | for |
|---|---|---|
| **the classic** | ST's LSM6DSV16X | the gait of a dog, a horse or a cow: ±16 g, the quieter gyro |
| **the hoof**, Ugoljok | ST's LSM6DSV320X | a hoof's strike: a second accelerometer to ±320 g beside the ±16 g one, research's own range for the impact (*the hoof's sensor*, below) |
| **the industrial**, Zvjozdočka | TDK's ICM-42686-P | raw data at 4–32 kHz, read by the FLPR ([`FIRMWARE.md`](FIRMWARE.md)); ±32 g |
| **an option** | TDK's ICM-42688-P | the ICM-42686-P's land and registers with ±16 g and a quieter gyro |

| | LSM6DSV16X | LSM6DSV320X | ICM-42686-P |
|---|---|---|---|
| sheet | DS13510 Rev 4 | DS14623 Rev 3 | DS-000639 v1.0 |
| ranges, at most | ±16 g, ±4 000 °/s | ±16 g, and ±32–320 g on its high-g channel; ±250–4 000 °/s (table 3) | ±32 g, ±4 000 °/s |
| gyro's noise | 2,8 mdps/√Hz, high-performance (table 3) | 3,8 mdps/√Hz (table 3) | 5,3 mdps/√Hz (table 1) |
| accelerometer's noise | 60 µg/√Hz, high-performance (table 3) | 60 µg/√Hz to ±16 g; 1 000 µg/√Hz on the high-g channel (table 3) | 70 µg/√Hz (table 2) |
| six axes | 0,65 mA, high-performance (table 4) | 0,67 mA, and 0,225 mA more with the high-g channel (table 4) | 0,70 mA, low-noise |
| the common time, on pin 9 | the ODR-triggered mode: the samples aligned in frequency and phase to a reference's edges, 8–510 a reference period in steps of two, the reference 40 ms at the shortest from 200 Hz up (6.6; AN5763 Rev 4, 3.3, table 15) | none with the high-g channel on: the ODR-triggered mode takes the low-g channel and the gyro alone (6.1.2, 6.1.5, table 21); its own clock, the samples drawn onto the grid by their stamps | CLKIN, 31–50 kHz; the rates scale by f / 32 kHz (4.10, 12.5) |
| the gyro's step | 70 mdps at ±2 000 °/s, 140 at ±4 000 (table 3): its 32 768 steps reach past the range | the LSM6DSV16X's (table 3) | the range / 32 768 (table 1) |
| fastest rate | 4 kHz on the common time, the ODR-triggered mode's ODRsel 3 200 Hz + 33 % (DT0155 Rev 2); 7,68 kHz on its own clock (table 3) | 7,68 kHz on its own clock, the high-g channel from 480 Hz (table 149); 2 kHz on the grid | 32 kHz |
| SPI | 10 MHz at most, modes 0 and 3 (table 7; 5.1); read at 8 MHz | 10 MHz at most (table 6); read at 8 MHz | read at 8 MHz |
| supply | Vdd 1,71–3,6 V, Vdd_IO 1,08–3,6 V (table 4) | the same (table 4) | VDD and VDDIO 1,71–3,6 V |
| WHO_AM_I | 0x70 at 0x0F (9.13) | 0x73 at 0x0F | 0x44 at 0x75 (14.58) |

- **The common time on pin 9**, wired on every board and unused now, every sensor on its own clock.
  The ICM-42686-P takes CLKIN at 32 kHz from the 16 MHz peripheral clock, which gives the 1 kHz rate
  exactly; the LSM6DSV16X takes an edge every second period, 25 Hz from the same timer, and 40
  samples to it, 1 kHz. The LSM6DSV16X takes any reference within ±33 % of its rate and locks within
  4 of its periods (AN5763 Rev 4, 3.3), far beyond the loop's 30 ppm; the ICM's sheet sets no limit
  on CLKIN's jitter, and TDK answers that it takes such steps with no issue on its samples or noise
  while CLKIN keeps the sheet's range
  ([zephyr.hal_tdk#40](https://github.com/tdk-invn-oss/zephyr.hal_tdk/issues/40)): a sample confirms
  it, the fallback kept ([`PRINCIPLE.md`](PRINCIPLE.md)).
- **The hoof's sensor, the LSM6DSV320X**, is the LSM6DSV16X's package, pins and register map with a
  high-g channel beside (DS14623 Rev 3, table 2; figure 31): the classic build's fitting, R24 left
  empty. Its high-g channel reads ±32, 64, 128, 256 or 320 g, 10,417 mg a step at ±320 g, so that
  its 32 768 steps make 341 g (table 3); its zero ±1,5 g before the calibration, moving ±2 mg a
  degree. A session's range of ±16 g takes its low-g channel, a wider one the high-g; either way the
  sensor runs on its own clock, since the high-g channel asks the high-performance or high-accuracy
  mode and takes no reference (6.1.2, table 21), at 7,68 kHz, its fastest, the grid 2 kHz at the
  most, and its samples filtered onto the set's grid by their stamps, as every sensor's
  ([`FIRMWARE.md`](FIRMWARE.md), *The sensor and its clock*). LCSC C26633007, extended.
- **The ICM-42688-P** stays an option on the ICM's fitting: the -42686-P's land and registers,
  ±16 g and ±2 000 °/s, its gyro 2,8 mdps/√Hz, 0,88 mA in six axes (DS-000347 v1.9, tables 1 and
  3); the firmware reads it by WHO_AM_I, 0x47 (14.58), and R24 is fitted with it as with the
  -42686-P.
- **The supply:** a π, 100 nF, the basic ferrite bead, then 2 × 1 µF, 2 × 100 nF and 1 nF; VDDIO 100
  nF. TDK asks 0,1 µF and 2,2 µF on VDD and 10 nF on VDDIO (DS-000639, 4.3, table 11); ST asks 100
  nF at each supply pin (DS13510 and DS14623, table 2). The bead's drop is in
  [`BOARD.md`](BOARD.md), *FB1*.

## The memory — the Belka's card, Kryska's SD NAND

**Kryska records into an SD NAND on its board**, CS's CSNP1GCR01-BOW, 128 MB, an SD card's NAND and
controller in one package, served as the Belka's card in SPI mode, its bad blocks and its ECC its
own: 2,7–3,6 V, −30 to +85 °C, 150–200 µA standing, 15–25 mA at work (its sheet, V1.1, 1.2; 3; 4),
at 400 Hz in ordinary use and 2 kHz at the most ([`KRYSKA.md`](KRYSKA.md)); its other way, NOR
flash, is a closed chapter ([`chapters/kryska-memory.md`](chapters/kryska-memory.md)). **The
Belka records to a microSD card in SPI mode**, the user's own: the board asks nothing of it
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

**One two-colour LED, Lite-On's LTST-C195KGJRKT**, the part LCSC sells as C125104: red, 639 nm at
the peak, and green, 574 nm; 2,0 V typical and 2,4 V at the most at 20 mA; 25 and 35 mcd typical at
20 mA, 15 and 20 at the least; −55 to 85 °C; two chips on four pads, the anodes 1 and 2 (Lite-On's
sheet for it, BNS-OD-C131/A4). Each colour is sunk by one of the PMIC's LED drivers through a
resistor of 1,5 kΩ, some 1 mA from the cell ([`BOARD.md`](BOARD.md), *D3*): the red by LED0, lit by
the charger's error from power-on, the green by LED1, lit while it charges, so that the charge shows
with the processor asleep; the firmware takes either for its own patterns ([`BOARD.md`](BOARD.md),
*U2*). At a twentieth of the sheet's 20 mA, seen indoors, faint in the sun. Of the same size:
XINGLIGHT's XL-1615SURSYGC, common anode, 570 and 620 nm (C965838).

## The magnets' switches

**Two TI DRV5032ZE**, Hall switches of open drain (SLVSDC7H): both poles, operating from ±33 to
±63 mT and released from ±30 to ±58 mT, so that a strong magnet held at the mark works them and
a stray one does not; the field sampled at 20 Hz, 13,3–37, so a magnet is seen within 75 ms;
1,6 µA at 3 V, 3,5 µA at most. They sit low in the box, at its two opposite sides, each under a
mark, some 20 mm or more from the contact in the lid, whose magnets could otherwise close them.
**Magnet A switches the unit on**, its switch on the cell itself and its output on the PMIC's
SHPHLD, which leaves ship mode on a hold of 96 ms; the processor reads it through the PMIC, and
switches the unit off. **Magnet B** is a second input, its output held up by the processor pin's
own pull-up.

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
guard. Behind it the TVS, the ESD guard and the PMIC, which guards the processor's VBUS
([`BOARD.md`](BOARD.md), *The core*).

## SWD

**Tag-Connect's TC2030-NL footprint**: six pads at 1,27 mm and three alignment holes, no part
on the board, for the TC2030-CTX-NL cable, whose other end is ARM's 10-pin Cortex connector on
the development kit's Debug out. SWD takes the first firmware and serves the bench; after it the
firmware comes over USB through the contact.
