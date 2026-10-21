<div align="center">

# The board

**One board for the Belka and Povodok: the schematic as nets, pin by pin, the values and the
bill of materials.**

↑ [NIC-Strelka](README.md)

</div>

---

One board, fitted two ways. **The Belka** carries Fanstel's BM20F, the nRF54LM20A module, with
the motion sensor U7 and the microSD card J3. **Povodok** carries Fanstel's WM02C20C — the
nRF54LM20A and the nRF7002 in one module — on the same pads, with U6, U7, J3 and their parts not
fitted; the bill of materials marks each part. The firmware is built twice from one source: the
Belka's with the sensor and the card, Povodok's with the Wi-Fi.

The schematic is written as nets, part by part, with its values and its bill of materials;
[`board/nets.svg`](board/nets.svg), by `board/nets.py`, lists the same parts and nets pin by pin,
and [`board/strelka.kicad_sch`](board/strelka.kicad_sch), by `board/schematic.py`, carries them as a
netlist in KiCad's form, every part with its reference, value and LCSC code and every pin on its
net, for EasyEDA to import. The schematic itself is to be drawn, in EasyEDA for JLCPCB's assembly,
the parts by their LCSC codes; the board is not drawn.

## The rails

| net | from | to | capacitors |
|---|---|---|---|
| VBUS | J1 pins 1 and 5, the contact | D1, the TVS; U2 IN (1); U10 VIN (3); TP3 | C1 1 µF and C2 100 nF at U2; C30 10 µF and C31 100 nF at U10 |
| VBUS_MOD | U2 OUT (5) | U1 VBUS (H7), the processor's USB supply | C4 1 µF at U2; C5 1 µF and C6 100 nF at H7 |
| VBUCK | L1 from U10 SW (5), 3,93 V while the cable is in | U3 IN (1); U11 VIN1 (3); R4, the feedback's top; Q3's gate, on Povodok | C32 and C36 22 µF and C9 100 nF at L1; C8 1 µF at U3's IN |
| SYS | U11 VOUT (2, 7), the two ideal diodes' common output | U5 IN (4) and EN (3); U6 IN (4); TP1 | C7 100 µF; C10 1 µF at the LDOs' inputs; C11 100 nF at U5; C12 100 nF at U6 |
| BAT | the cell's plus through J2 | U3 OUT (8); U11 VIN2 (6); U4 BATT (10); U8 VCC (1); R20, the top of ON's pull-up | C33 1 µF at U3's OUT; C13 100 nF at U4, to BATT−; C28 100 nF at U8 |
| BATT− | the cell's minus through J2 | U4 CSP (6), its pad, AIN (2) and ALRT (8); R1, its cell side; the cell's NTC | C14 470 nF from U4 REG (7); C13 from BAT |
| +3V0 | U5 OUT (1) | U1 VDD (B8, C7, C8); FB1; U7 VDDIO (5); U9 VCC (1); Q2's gate; D3's anodes; R6, R8, R9, R11; the SWD pad 1 | C15 1 µF and C16 100 nF at U5; C17 100 µF and C18 100 nF at U1; C27 100 nF at U7's VDDIO; C29 100 nF at U9 |
| +3V0_SD | U6 OUT (1), its EN (3) from P3.06, held low inside it by 500 kΩ | J3 VDD (4); R12–R16; TP2 | C19 1 µF and C20 100 nF at J3 |
| +3V0_IMU | FB1, the ferrite bead, from +3V0 | U7 VDD (8) | C21 100 nF at FB1's input; C22, C23 1 µF, C24, C25 100 nF and C26 1 nF at U7 |
| GND | one plane | every ground below; U4 CSN (5) and R1's load side | — |

Two LDOs of 3,0 V: U5, always on, feeds the module, the sensor, magnet B's switch and the light;
U6, switched by the processor, feeds the card alone. **The supply is set by resistors alone**,
nothing written to it: U10, a buck that runs only while the cable is in, makes VBUCK, 3,93 V,
from the cable; U3, a linear charger, charges the cell from VBUCK; U11, two ideal diodes in one,
passes to SYS whichever of VBUCK and the cell is higher — VBUCK while the cable is in, so that
the board runs on the cable and the cell rests, the cell without it. **U11 is the board's
switch**: magnet A turns it on, and on Povodok the cable too; the processor holds it on and lets
it go to switch the board off; the charge goes on without it. U10 is the board's one switching
converter. The cell's minus is BATT−, parted from the board's ground by the gauge's sense
resistor R1, the one tie between the two, so that R1 carries the cell's current alone, in or out.

## U1 — the module

The pads and the nRF54LM20A's pins they carry are from Fanstel's BM20C series specification,
version 1.00, the pin table, each checked in its BM20 and WM20 columns, so that the wiring holds
for the BM20F and the WM02C20C alike; the WM02C20C's own specification, draft 0.82, gives its
footprint as compatible with the BM20C series. The crystals, the antenna, the DC/DC inductors and
the chip's decoupling are inside the module.

| pad | pin | net | to |
|---|---|---|---|
| B8, C7, C8 | VDD | +3V0 | C17 100 µF and C18 100 nF at the pads |
| H7 | VBUS | VBUS_MOD | U2 OUT |
| G7, G6 | D+, D− | USB_DP, USB_DM | J1 pins 2 and 3, straight; D2 at the contact; a 90 Ω pair, short and straight, nothing in series (the nRF54LM20A datasheet v1.0, 8.27.1) |
| J3, K3 | SWDIO, SWDCLK | SWDIO, SWDCLK | the SWD pads 2 and 4 |
| G2 | /Reset | NRESET | the SWD pad 3; its pull-up inside the chip |
| C3 | P3.04 | IMU_SCK | U7 pin 13; a clock pin (table 83) |
| H2 | P3.05 | IMU_SDI | U7 pin 14 |
| C9 | P3.07 | IMU_SDO | U7 pin 1 |
| D7 | P3.02 | IMU_CS | U7 pin 12; R11 100 kΩ to +3V0 |
| E6 | P3.01 | IMU_INT1 | U7 pin 4 |
| K5 | P3.00 | IMU_CLKIN | U7 pin 9 |
| A7 | P3.03 | SD_CLK | J3 pin 5; a clock pin (table 83) |
| D4 | P3.08 | SD_DI | J3 pin 3 |
| K7 | P3.09 | SD_DO | J3 pin 7 |
| K2 | P3.10 | SD_CS | J3 pin 2 |
| D8 | P3.06 | SD_EN | U6 EN (3), held low inside U6 by 500 kΩ |
| C2 | P0.04 | SCL | U4 pin 3; R8 10 kΩ to +3V0 |
| E7 | P0.03 | SDA | U4 pin 4; R9 10 kΩ to +3V0 |
| A5 | P0.00 | CHG_STAT | U3 pin 5; R6 10 kΩ to +3V0 |
| D2 | P0.01 | CHG_PG | U3 pin 6; R7 100 kΩ to +3V0; low while the cable is in, wakes the chip from System OFF |
| D9 | P0.05 | MAG_A | Q2's source, U8's output through Q2; wakes the chip from System OFF |
| A6 | P0.02 | MAG_B | U9 pin 2 |
| H6 | P1.23 | HOLD | Q1's gate, R22 10 MΩ and C37 100 nF to GND; high, it holds the board on |
| J5 | P1.30 | LED_G | D3 pad 3, the green cathode, through R17 1 kΩ |
| K9 | P1.31 | LED_R | D3 pad 4, the red cathode, through R18 1 kΩ |
| A1, A9, B1, B7, B9, C1, D1, E1, F1, G1, H1, H8, J1, K1 | GND | GND | each to the plane |

43 pads of 90 are joined. **Left open:** P1.04–P1.14, unconnected on the WM02C20C, whose
nRF7002 they control inside the module, and free on the BM20F; A2, which feeds the power
amplifier of the BM20NE and BM20XE alone; NFC (C4, A8) and every other GPIO, each given a pull
by the firmware.

**Why the pins are where they are**: a peripheral takes its pins on one port (8.9.3); SCK and SCL
sit on clock pins (tables 79 and 83) — P3 has two, P3.04 for the sensor's clock and P3.03 for the
card's; the magnets are on P0, since P2's pins cannot wake the chip from System OFF (table 41); the
sensor and the card are on SPIM21 and SPIM20 in the peripheral domain, whose clock runs from 127
kHz, under the 400 kHz a card asks at its identification, up to 8 Mbit/s (11.19.1); none of the pins
clashes with what Nordic's development kit uses for itself (its guide v0.7.0, tables 1–4 and 10).

**The antenna**: the module sits at the board's edge, its antenna over a place with no copper on
either layer (Fanstel). That end of the board takes the largest of the modules on this land: the
WM02C20C's 24,0 mm, the BM20F's 15,2 mm width at its antenna.

## The supply

**J1, the contact**, Promax's five-pin magnetic pair, 1 A: the NF2347-05B glued into the lid, 27,2 ×
8,0 × 7,2 mm, the NC2347-05B on the cable, 26,7 × 7,5 × 7,2 mm
([Promax](https://promaxpogopin.com/magnetic-connectors/waterproof/)), its five pins at 2,2 mm, pin
1 at the mark on its face (the NC2347-05B's drawing). Wired symmetric: pin 1 VBUS, 2 D+, 3 GND, 4
D−, 5 VBUS, so that a plug put on turned round meets VBUS with VBUS and GND with GND and swaps D+
with D− alone: the charge either way, USB the right way round, nothing harmed, and no reverse guard
at the inlet.

**D1, the TVS at the inlet**, from VBUS to GND: SMF12CA, two-way, standing off 12 V either
way, breaking down at 13,3–14,7 V, clamping 19,9 V at 10,1 A — under the 21 V U2's input bears
(onsemi NCP361/D, maximum ratings) and the 32 V of U10's VIN (SLUSEA4D, 7.1).

**D2, the ESD guard on the USB pair**, at the contact's pads 2 and 4: TI's TPD2E2U06DCKR in SC-70 —
pins 1 and 2 to D+ and D−, 3 to GND — standing off 5,5 V, 1,5 pF a line, made for USB 2.0's
480 Mbit/s (TI SLLSEG9C).

**U2, the guard of the processor's USB supply**, onsemi NCP361SNT1G, TSOP-5: 1 IN to VBUS, 2 GND,
3 EN to GND — enabled for good, 4 FLAG unconnected, 5 OUT to VBUS_MOD. It cuts its output above
5,43–5,9 V (OVLO) and asks 1 µF at IN and at OUT (NCP361/D, the pin table): C1 and C4.

**U10, the buck**, TI TPS62933DRLR, SOT-583 of 1,6 × 2,1 mm, 3,8–30 V in, 32 V at most
(SLUSEA4D, the pin table and 7.1): 3 VIN to VBUS, C30 10 µF and C31 100 nF; 5 SW to L1; 6 BST,
C34 100 nF to SW; 8 FB, the middle of R4 47 kΩ from VBUCK and R5 12 kΩ to GND; 1 RT to GND, for
1,2 MHz; 7 SS, C35 10 nF, some 1,5 ms of soft start, 6,8 nF the least; 2 EN open, enabled by its
own pull-up — it bears 6 V alone, so it is not tied to VIN; 4 GND to the plane. **3,93 V** =
0,8 V × (1 + 47 / 12), the reference ±1 %. TI's divider has 10 kΩ at the foot (9.3.4); 12 kΩ, a
basic part, gives 3,93 V with a basic 47 kΩ. It runs only on the cable's power and draws nothing
from the cell; in light load it skips pulses (PFM). Some 95 % from 6 V to 3,3 V at 0,2–0,5 A with
2,2 µH at 1,2 MHz (figure 8-19), so some 0,06 W lost while the cell takes 300 mA. The least
off-time, 140 ns, and its 98 % duty at the most hold VBUCK near 3,9 V down to some 4,75 V of USB
(9.3.9).

**L1, the buck's inductor**, Sunlord WPN201610H2R2MT, 2,2 µH, 2,0 × 1,6 mm, 170 mΩ, rated 1,45 and
2,15 A (LCSC's listing, C97005); TI's pairing for 1,2 MHz is 2,2 µH with 10–30 µF effective at
the output (table 10-2): C32 and C36, 22 µF each. The peak at 5 V, 300 mA charging and Povodok's
290 mA, some 0,76 A with 0,32 A of ripple; at 18 V on VBUS some 1,2 A.

**U3, the charger**, TI BQ25170DSGR, WSON-8 of 2 × 2 mm, linear, set by resistors alone
(SLUSDJ8A, the pin table): 1 IN to VBUCK, C8 1 µF; 8 OUT to BAT, C33 1 µF; 7 VSET, R3 100 kΩ to
GND — **3,50 V** for a LiFePO4 cell (table 7-1), ±0,5 %; 2 ISET, R2 1,0 kΩ to GND — **300 mA**,
300 AΩ / 1,0 kΩ, 270–330 mA (KISET); 3 TS, the cell's NTC through J2 to BATT−, biased by 38 µA
inside; 5 STAT, open drain, to P0.00 with R6 10 kΩ to +3V0, low while it charges, blinking on a
fault; 6 /PG, open drain, to P0.01 with R7 100 kΩ to +3V0, low while VBUCK is there; 4 GND and
the pad to the plane through vias. Its output carries the cell alone, the board's load being on
SYS behind U11, so that the charge ends on the cell's own current, at 30 mA, 10 % of ISET, and
starts again 175–225 mV under 3,50 V (VRECHG). **From VBUCK it loses (3,93 − VBAT) × 0,3 A**, some
0,19 W with the cell at 3,3 V, against 0,54 W from 5 V. It charges with VBUCK at least 95–175 mV
over the cell (VSLEEPZ); 425 mV of dropout at 500 mA (VDO) leave it 3,5 V and more from 3,93 V at
300 mA. A cell under 2,0 V takes 20 %, 60 mA; under 1,2 V, 16 mA (VBAT_LOWV, VBAT_SHORT, both for
LiFePO4). Its temperature by the NTC alone: stopped under about 0 °C, again from about 4 °C; and
over about 45 °C, again under about 40 °C (VCOLD, VHOT) — the cell's own 0–45 °C. Its timers:
30 minutes of pre-charge, 10 hours of fast charge (tPRECHG, tSAFETY). Its input stops above
6,6–6,9 V and bears 30 V; 0,35 µA from the cell without the cable, 0,6 µA at most (IQ_OUT).

**U11, the ideal diodes**, TI LM66200DRLR, two in one SOT-583 of 1,6 × 2,1 mm, 2,5 A each, 40 mΩ,
48 mΩ at most at 3,3 V and 25 °C, 55 mΩ at most to 85 °C (SLVSG04, the pin table and 6.5): 3 VIN1
to VBUCK, 6 VIN2 to BAT, 2 and 7 VOUT to SYS, 4 ON to ON, 1 and 5 GND to the plane, 8 ST
unconnected, its open drain unused. It passes the higher of its inputs and blocks a current back
into either (7.6.1; the features): on the cable VBUCK, some 0,4 V over the cell, and the cell
rests; when the cable leaves, the cell takes over in some 6,2 µs (tSW), C7 holding SYS. An input
falling faster than 1 V in 10 µs is caught only with the other at 2,5 V or more (7.4); the cell
stands over 3,1 V whenever the board runs. **ON low turns it on, ON high turns both channels off**
(the pin table): the threshold 0,8–1,2 V, 1,0 V typical, the pin's leakage 0,1 µA at most, no
pull of its own (6.5). Turned on, it waits some 1,2 ms and brings SYS up in some 1,3 ms (tD, tSS),
some 0,3 A into C7 (8.2.2). From the cell, on, 1,35 µA; off, 0,05 µA, 1,3 µA at most to 85 °C
(IQ,VIN2; ISD,VIN2, at 25 °C).

**The switch: on by magnet A, off by the processor.** ON is held high by R20 and R21, 1 MΩ each
in series from BAT, 2,0 MΩ of two basic parts, so that the board stands off by itself.

- **On**: U8, magnet A's Hall switch, sits on BAT and pulls ON down through D4, a Schottky diode,
  its anode on ON, its cathode on HALL_A, U8's output: 0,3 V at most from U8 at 1 mA (SLVSDC7H,
  6.5) and 0,24 V at most from D4 at 0,1 mA (CJ's BAT54 sheet), some 0,54 V at the most against
  the 0,8 V that turns U11 on for certain; some 1,7 µA flows. U8 sees a magnet within 75 ms; U11
  and U5 start in some 4 ms more; then the bootloader sets HOLD.
- **Held**: the firmware sets P1.23, HOLD, high from the bootloader's first instructions; Q1,
  onsemi's BSS138, its drain on ON, its source on GND, holds ON down. D4 then stands reversed, and
  HALL_A stays U8's alone.
- **Read**: Q2, a second BSS138, passes HALL_A to P0.05: its drain on HALL_A, its source on P0.05,
  its gate on +3V0, 1,5 V over its threshold at least (0,8–1,5 V at 1 mA; onsemi's BSS138 sheet).
  U8 pulls P0.05 down to some 0,3 V against the processor's pull-up, under its 0,9 V (VIL,
  0,3 × VDD; the nRF54LM20A datasheet, 11.7.1); without a magnet the pull-up holds it high, D4's
  leakage, 2 µA at most at 25 V, costing it some 0,03 V.
- **Off**: the firmware drives HOLD low; ON rises once magnet A is gone too, and the board is off,
  the charge going on from VBUCK.
- **The cable, on Povodok alone**: Q3, a third BSS138, its drain on ON, its source on GND, its gate
  on VBUCK. The cable's 3,93 V turns it on, 2,4 V over its threshold at least, and the board
  starts as by the magnet; when the cable leaves, its gate falls with VBUCK through R4 and R5.
  While the cable is in, Q3 holds ON down, so the firmware cannot open the switch and sleeps
  instead ([`FIRMWARE.md`](FIRMWARE.md)). Not fitted on the Belka, which the cable only charges.
- **Why the transistors**: a dead processor's pins sit on its protection diodes to a rail at 0 V
  — a pin bears VDD + 0,3 V at most (the datasheet, table 94) — so that a pin joined to ON or to
  HALL_A would pull it down and start the board. Q1's gate and Q2's channel, both off at 0 V, part
  the processor from them; Q1 also turns the pin's high into the low that U11 runs on.
- **Through a reset** the pins stand high-impedance until the firmware sets them (8.9.7; table 22):
  R22, 10 MΩ, and C37, 100 nF, hold Q1's gate over its 1,5 V for 0,69 s at least, the pin
  charging C37 at 1–4 mA (standard drive, 11.7.1) in some 0,1 ms.
- **Off, the leakage on ON**: U11's 0,1 µA, U8's 0,1 µA (IOZ), Q1's and Q2's 0,1 µA each at 30 V
  (onsemi's BSS138 sheet), 0,4 µA at most, 0,8 V on 2,0 MΩ: ON stands at 1,5 V at least with the
  cell at its 2,3 V cut-off; on Povodok Q3's 0,1 µA more, 1,3 V at least — over U11's 1,2 V
  either way.

**From the cell without the cable**: the board running, U3's 0,35 µA, U11's 1,35 µA, 1,65 µA in
R20 and R21 at 3,3 V and 0,3 µA in R22, **some 3,7 µA** for the supply and the switch; the board
off, U3's 0,35 µA, U11's 0,05 µA, U8's 1,6 µA and the gauge hibernating, 7 µA, **some 9 µA** in
all.

For the firmware: the supply is set by its resistors; HOLD set high from the bootloader's first
instructions and driven low to switch the board off; on Povodok, the cable gone, HOLD kept 5 min;
STAT and /PG read; the gauge written after the cell is joined.

**U4, the fuel gauge**, Analog Devices' MAX17055ETB+T, TDFN-10 of 2 × 2,5 mm: 1 N.C.; 2 AIN to CSP,
unused; 3 SCL to P0.04; 4 SDA to P0.03; 5 CSN to GND, the sense resistor's load side; 6 CSP, the
IC's ground, to BATT−, the resistor's cell side; 7 REG, C14 470 nF to CSP; 8 ALRT to CSP, unused; 9
THRM unconnected, the temperature taken by the die's own sensor; 10 BATT to BAT, C13 100 nF to CSP;
the pad to CSP (MAX17055 data sheet, the pin description). **R1**, the sense resistor, Ralec
RLM10FTSMR020, 20 mΩ 1 %, ±100 ppm/°C, 1/2 W, 0805 (Ralec's RLM series sheet), between the cell's
minus and the board's ground, its two ends taken to CSP and CSN by their own tracks: ±2,56 A of
range, ±75 µA of offset, 5,8 mV at 290 mA. The firmware sets ModelID 6, "for lithium iron-phosphate"
(the data sheet, ModelCfg).

**U5 and U6, the LDOs**, TI TPS7A2030PDQNR, 3,0 V, X2SON of 1 × 1 mm: 1 OUT, 2 GND, 3 EN, 4 IN,
5 the thermal pad, joined to GND on the top layer beside pin 2, no via (SBVS338H, the pin
functions). U5 has IN and EN on SYS and runs always; U6 has IN on SYS and EN from P3.06, held
low by 500 kΩ inside it until the processor drives it. Each gives 300 mA, limited at
360–730 mA; 6,5 µA of its own, 8,5 µA at most at 25 °C, 0,07 µA disabled; in dropout 140 mV at
most at 300 mA, so +3V0 holds 3,0 V down to some 3,15 V on SYS at Povodok's peak of some
290 mA, the Wi-Fi's and the processor's. Disabled, each discharges its output through 150 Ω;
each starts in 750–1 150 µs (tSTR).

**J2, the cell**, JST S3B-PH-SM4-TB, three pins: BAT, BATT− and NTC, in the order of the cell's
lead.

## U7 — the motion sensor

**TDK ICM-42686-P**, LGA-14 of 2,5 × 3 mm; the ICM-42688-P on the same land, the firmware telling
them by WHO_AM_I. Fitted on the Belka alone. The pins (DS-000639, table 10):

| pin | name | net |
|---|---|---|
| 1 | AP_SDO | IMU_SDO, P3.07 |
| 4 | INT1 | IMU_INT1, P3.01 |
| 5 | VDDIO | +3V0, C27 100 nF |
| 6 | GND | GND |
| 8 | VDD | +3V0_IMU |
| 9 | INT2 / FSYNC / CLKIN | IMU_CLKIN, P3.00 |
| 12 | AP_CS | IMU_CS, P3.02; R11 100 kΩ to +3V0 |
| 13 | AP_SCLK | IMU_SCK, P3.04 |
| 14 | AP_SDI | IMU_SDI, P3.05 |
| 2, 3, 7, 10, 11 | RESV | GND — 2, 3 and 10 "No Connect or Connect to GND", 7 and 11 "Connect to GND" |

**FB1**, Murata's BLM18PG121SN1D, 120 Ω at 100 MHz, 50 mΩ — some 35 µV at the sensor's 0,70 mA —
from +3V0 to +3V0_IMU, with C21 100 nF at its input and C22, C23 1 µF, C24, C25 100 nF and C26
1 nF at the sensor's VDD, against TDK's 2,2 µF and 0,1 µF (table 11). **R11** keeps the sensor off
the bus while the processor is held in reset or programmed; at 100 kΩ it draws some 15 µA on
average while the sensor is read at 32 kHz, its CS low some half of the time.

For the firmware: INT1 is an open-drain output after reset (INT_CONFIG, reset 0x00), set to
push-pull before the data-ready is awaited; pin 9 is INT2 after reset (INTF_CONFIG5,
PIN9_FUNCTION 00), and the clock is driven onto it only once it is set to CLKIN.

## J3 — the card

**Megastar's ZX-TF-281.6J**, a microSD slot with a lid sliding to lock in a steel shell, eight
contacts and four shell pads, no card-detect switch (its drawing). Fitted on the Belka alone, with
U6, R12–R16, C12, C19 and C20. The card works in SPI mode; the contacts by ATP's microSD
sheet, table 2-0:

| pin | the card's | net |
|---|---|---|
| 1 | DAT2, unused | R12 100 kΩ to +3V0_SD |
| 2 | CD/DAT3 — CS | SD_CS, P3.10; R13 100 kΩ to +3V0_SD |
| 3 | CMD — DI | SD_DI, P3.08; R14 100 kΩ to +3V0_SD |
| 4 | VDD | +3V0_SD; C19 1 µF and C20 100 nF |
| 5 | CLK | SD_CLK, P3.03, a clock pin |
| 6 | VSS | GND |
| 7 | DAT0 — DO | SD_DO, P3.09; R15 100 kΩ to +3V0_SD |
| 8 | DAT1, unused | R16 100 kΩ to +3V0_SD |
| shell | — | GND |

**The pull-ups**, 10–100 kΩ on CMD and DAT "to prevent bus floating" (the bus's line load, as
ATP's sheet gives it, table 4-4; the SD Association's simplified specification leaves that table
out), on +3V0_SD, not +3V0: "during power down, DAT, CMD, and CLK should be disconnected or driven
to logical 0" (the Physical Layer Simplified Specification 9.10, 6.4.1.5), so that a card switched
off is not fed through its lines; they draw nothing then. **U6 sits beside the slot**, its C19 and
C20 at pin 4. The card in SPI mode draws up to 100 mA, averaged over a second (ATP, 4.1–4.2).
U6 starts in 550 µs, within the ramp of 0,1–35 ms the card asks (6.4.1.4); switched off, it
takes 1,1 µF below 0,5 V in some 0,25 ms against the 1 ms asked (6.4.1.3).

For the firmware: P3.03, P3.08 and P3.10 low or disconnected before EN falls; the card kept off
for 10 ms, its own capacitance unknown; after EN rises, 1 ms, then 74 clocks with CS high, then
CMD0 with CS low for SPI mode (6.4.1.1); the card known by its answer, the slot having no switch.
No ESD parts on the card's lines: the slot sits inside the closed box. In SPI mode a card of up
to 2 TB is read, SDSC, SDHC and SDXC — "SPI Mode is not supported by SDUC cards" (7).

## The rest

- **U8 and U9, the magnets' switches**, TI DRV5032ZE, Hall switches of open drain, SOT-23: pin 1
  VCC, 2 OUT, 3 GND (SLVSDC7H, table 5-1), 100 nF at each VCC (TI: "at least 0.1 µF"). **U8,
  magnet A**, VCC on BAT with C28, so that it works with the board off; its OUT on HALL_A, through
  D4 onto ON and through Q2 to P0.05, which wakes the chip from System OFF (*The supply*). **U9,
  magnet B**, VCC on +3V0 with C29, its OUT to P0.02. Each OUT is held up by the processor pin's
  own pull-up, 12–16 kΩ (the nRF54LM20A datasheet, 11.7.1), some 0,2 mA only while a magnet is
  held; U8's, with the board off, by R20 and R21 through D4. Both poles, operating from ±33 to
  ±63 mT and released from ±30 to ±58 mT (6.6): a strong magnet held at the mark, not a stray
  one. The field sampled at 20 Hz (13,3–37), a magnet seen within 75 ms; 1,6 µA each at 3 V,
  3,5 µA at most. They sit low in the box at its two opposite sides, each under a mark, some 20 mm
  or more from the contact in the lid, whose magnets could otherwise close them.
- **D3, the light**, Lite-On's LTST-C195KGJRKT, red and green in 1,6 × 1,5 mm: pads 1 and 2, the
  anodes, on +3V0; pad 3, the green cathode, through R17 1 kΩ to P1.30; pad 4, the red cathode,
  through R18 1 kΩ to P1.31 (Lite-On's sheet, the polarity drawing). The pins sink on high drive,
  some 1,1 mA a colour: seen by day, not lighting a room at night.
- **The SWD pads**, Tag-Connect's TC2030-NL footprint for its TC2030-CTX-NL cable: six pads at
  1,27 mm and three alignment holes, no part: 1 +3V0, 2 SWDIO (U1 J3), 3 NRESET (U1 G2), 4 SWDCLK
  (U1 K3), 5 GND, 6 unconnected, SWO (Tag-Connect's TC2030-CTX sheet). No paste on its pads, no
  track or via in its keep-out (the TC2030-IDC-NL footprint's notes). The cable's other end,
  ARM's 10-pin Cortex connector, goes on the development kit's Debug out, P13, which programs an
  outside board when the 10-pin's pin 3 is grounded (the kit's guide, 3.3, table 12). SWD serves
  the first programming and the bench; after it the firmware comes over USB through the contact.
- **TP1–TP3, the test pads**, bare copper for a probe: TP1 SYS, TP2 +3V0_SD, TP3 VBUS.

## The capacitors

Each is chosen by the capacitance it keeps at the highest voltage it sees: a ceramic capacitor
loses part of it under a DC voltage, the more the smaller its case and the lower its rating. The
losses are read from Samsung's DC-bias curves at 25 °C, on each part's page at
[weblib.samsungsem.com](https://weblib.samsungsem.com/mlcc/mlcc-ec-data-sheet.do?partNumber=CL31A107MQHNNNE);
every part is a basic one at JLCPCB.

| part | refs | the highest voltage | kept |
|---|---|---|---|
| 100 µF 6,3 V X5R 1206, Samsung CL31A107MQHNNNE | C7 SYS; C17 the module's VDD | 4,0 V, VBUCK's at the most; 3,0 V | —; 49,2 µF; 1,6 mΩ at 100 kHz |
| 1 µF 50 V X7R 0805, Samsung CL21B105KBFNNNE | C1, C4, C5, C8, C10, C15, C19, C22, C23, C33 | 5,9 V on VBUS_MOD, U2's cut-off | 98 % at 5 V |
| 100 nF 50 V X7R 0402, Samsung CL05B104KB54PNC | C2, C6, C9, C11–C13, C16, C18, C20, C21, C24, C25, C27–C29, C31, C34, C37 | 5,5 V on VBUS | 91 % at 5,5 V |
| 470 nF 25 V X7R 0603, Samsung CL10B474KA8NNNC | C14, the gauge's REG | 1,8 V | the value Maxim asks, "0.47 μF" |
| 1 nF 50 V X7R 0402 | C26, nearest the sensor's VDD | 3,0 V | 1 nF |
| 10 µF 50 V X5R 1206, Samsung CL31A106KBHNNNE | C30, U10's VIN | 5,5 V | — |
| 22 µF 25 V X5R 0805, Samsung CL21A226MAQNNNE | C32, C36, U10's output | 4,0 V | — |
| 10 nF 50 V X7R 0402, Samsung CL05B103KB5NNNC | C35, U10's SS | 0,8 V | — |

The 100 µF in 1206 keeps half its value, and has no leakage, which a polymer capacitor of the same
value has on an always-on rail. The TPS7A2030 asks 1 µF at its input and an effective
0,47–200 µF at its output, of 100 mΩ at most (SBVS338H, the recommended operating conditions):
+3V0 carries some 104 µF, some 53 µF kept; +3V0_SD carries C19, C20 and the card's own. U10
asks 10 µF and 0,1 µF at VIN, 100 nF at BST, 6,8 nF at least at SS and 10–30 µF effective at its
output at 1,2 MHz (SLUSEA4D, the pin table, 10.2.2.9 and table 10-2); U3 asks 1 µF at IN and at
OUT (SLUSDJ8A, 6.3); U11, 1 µF at each input (SLVSG04, 9) and 0,1 µF at least at its output for
its soft start (7.3.2), which C8, C32, C33 and C7 give. U2 asks 1 µF at IN and OUT: C1 and C4.
**A 100 nF in 0402 beside every larger capacitor, nearest the pin**: the smaller case has the
smaller series inductance and keeps the impedance down where the larger one has turned inductive.
Each has its own via to the ground plane.

## The bill of materials

B is the Belka, P is Povodok: ✓ fitted, — not fitted. At JLCPCB, *basic* or *extended*.

| ref | part | value | package | at JLCPCB | B | P |
|---|---|---|---|---|---|---|
| U1 | Fanstel BM20F; Fanstel WM02C20C | the nRF54LM20A module; with the nRF7002 | LGA-90, 10 × 20,8 mm, 15,2 at the antenna; 10,0 × 24,0 mm | Fanstel | BM20F | WM02C20C |
| U2 | onsemi NCP361SNT1G | the USB supply's guard | TSOP-5 | extended, C233714 | ✓ | ✓ |
| U3 | TI BQ25170DSGR | the charger, linear; 3,50 V, 300 mA by resistors | WSON-8, 2 × 2 mm | extended, C5208201 | ✓ | ✓ |
| U4 | Analog Devices MAX17055ETB+T | the fuel gauge | TDFN-10, 2 × 2,5 mm | extended, C2976422 | ✓ | ✓ |
| U5 | TI TPS7A2030PDQNR | LDO 3,0 V, +3V0 | X2SON-4, 1 × 1 mm | extended, C48581094 | ✓ | ✓ |
| U6 | TI TPS7A2030PDQNR | LDO 3,0 V, +3V0_SD | X2SON-4, 1 × 1 mm | extended, C48581094 | ✓ | — |
| U7 | TDK ICM-42686-P | the motion sensor | LGA-14, 2,5 × 3 mm | extended, C19191783 | ✓ | — |
| U8, U9 | TI DRV5032ZEDBZR | Hall switch, ±63 mT at most | SOT-23 | extended, C266118 | ✓ | ✓ |
| U10 | TI TPS62933DRLR | the buck, 3,93 V from the cable | SOT-583, 1,6 × 2,1 mm | extended, C3200405 | ✓ | ✓ |
| U11 | TI LM66200DRLR | two ideal diodes, VBUCK or the cell to SYS; the board's switch | SOT-583, 1,6 × 2,1 mm | extended, C3235556 | ✓ | ✓ |
| Q1, Q2 | onsemi BSS138 | N-MOSFET, 50 V; the hold, magnet A's reading | SOT-23 | extended, C52895 | ✓ | ✓ |
| Q3 | onsemi BSS138 | N-MOSFET, 50 V; the cable's start | SOT-23 | extended, C52895 | — | ✓ |
| D1 | SMF12CA | two-way TVS, 12 V | SOD-123FL | extended, C123801 | ✓ | ✓ |
| D2 | TI TPD2E2U06DCKR | ESD on the USB pair | SC-70-3 | extended, C1855726 | ✓ | ✓ |
| D3 | Lite-On LTST-C195KGJRKT | red and green | 1,6 × 1,5 mm | extended, C125104 | ✓ | ✓ |
| D4 | CJ BAT54 | Schottky, 30 V; magnet A onto ON | SOT-23 | extended, C8590 | ✓ | ✓ |
| L1 | Sunlord WPN201610H2R2MT | 2,2 µH, 170 mΩ | 2016, 2,0 × 1,6 mm | extended, C97005 | ✓ | ✓ |
| FB1 | Murata BLM18PG121SN1D | 120 Ω at 100 MHz | 0603 | basic, C14709 | ✓ | — |
| R1 | Ralec RLM10FTSMR020 | 20 mΩ 1 % | 0805 | extended, C163095 | ✓ | ✓ |
| R3, R7 | resistor | 100 kΩ 1 % | 0402 | basic, C25741 | ✓ | ✓ |
| R20, R21 | resistor | 1 MΩ 1 % | 0402 | basic, C26083 | ✓ | ✓ |
| R22 | resistor | 10 MΩ 1 % | 0603 | basic, C7250 | ✓ | ✓ |
| R4 | resistor | 47 kΩ 1 % | 0402 | basic, C25792 | ✓ | ✓ |
| R5 | resistor | 12 kΩ 1 % | 0402 | basic, C25752 | ✓ | ✓ |
| R11, R12–R16 | resistor | 100 kΩ 1 % | 0402 | basic, C25741 | ✓ | — |
| R6, R8, R9 | resistor | 10 kΩ 1 % | 0402 | basic, C25744 | ✓ | ✓ |
| R2, R17, R18 | resistor | 1,0 kΩ 1 % | 0402 | basic, C11702 | ✓ | ✓ |
| C7, C17 | capacitor | 100 µF 6,3 V X5R | 1206 | basic, C15008 | ✓ | ✓ |
| C1, C4, C5, C8, C10, C15, C33 | capacitor | 1 µF 50 V X7R | 0805 | basic, C28323 | ✓ | ✓ |
| C19, C22, C23 | capacitor | 1 µF 50 V X7R | 0805 | basic, C28323 | ✓ | — |
| C2, C6, C9, C11, C13, C16, C18, C28, C29, C31, C34, C37 | capacitor | 100 nF 50 V X7R | 0402 | basic, C307331 | ✓ | ✓ |
| C12, C20, C21, C24, C25, C27 | capacitor | 100 nF 50 V X7R | 0402 | basic, C307331 | ✓ | — |
| C14 | capacitor | 470 nF 25 V X7R | 0603 | basic, C1623 | ✓ | ✓ |
| C30 | capacitor | 10 µF 50 V X5R | 1206 | basic, C13585 | ✓ | ✓ |
| C32, C36 | capacitor | 22 µF 25 V X5R | 0805 | basic, C45783 | ✓ | ✓ |
| C35 | capacitor | 10 nF 50 V X7R | 0402 | basic, C15195 | ✓ | ✓ |
| C26 | capacitor | 1 nF 50 V X7R | 0402 | basic, C1523 | ✓ | — |
| J1 | Promax NF2347-05B; NC2347-05B on the cable | the five-pin magnetic contact, in the lid | — | bought elsewhere | ✓ | ✓ |
| J2 | JST S3B-PH-SM4-TB | the cell, 3 pins, 2 mm | SMD, right angle | extended, C265101 | ✓ | ✓ |
| J3 | Megastar ZX-TF-281.6J | microSD slot | SMD | extended, C7419839 | ✓ | — |
| — | LTOPOW LFP103042 | the LiFePO4 cell, 720 mAh, with its NTC | 10 × 30 × 42 mm | bought elsewhere | ✓ | ✓ |
| TC1; TP1–TP3 | Tag-Connect TC2030-NL; test pads | pads | — | copper only | ✓ | ✓ |

**The Belka**: 76 parts placed, of 33 kinds, 16 of them basic. **Povodok**: 57 parts placed, of
29 kinds, 14 of them basic; the module aside in both.
