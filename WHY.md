<div align="center">

# Why not

**So that nobody takes it up again without knowing.**

↑ [NIC-Strelka](README.md)

</div>

---

What was considered and set aside, and why — so that nobody takes it up again without knowing.

## The requirements

- **30 m to the phone.** Round a cow's body that asks a professional radio — a long-range mode,
  more power, a better antenna — and its price. The phone is reached through Povodok, in the vet's
  hand or on the animal.
- **Up to 40 units in a set.** 32 cover an examination of any animal; the firmware keeps room for
  63.

## The architecture

- **A Belka relaying for another — parents and children, three generations.** The data fly
  twice and a level a period late, and a body stands between two Belkas on one animal as it
  stands between a Belka and a Povodok: from one BM20F to another at their best, 1 m and 50 dB of
  body, some 20 dB are in hand, and straight through the flesh nothing. Up to four Povodoks round
  the animal, each sending its own sync and hearing the Belkas, carry the reach instead; no
  Belka sends anything but its own.
- **A flood, every Belka that hears the sync repeating it at once.** Repeats sent together
  collide, and every Belka's radio would stay on to repeat; the Povodoks' own syncs carry the
  time, and a Belka that hears none keeps its clock and records on.
- **Repeaters named by the commander, each sending a copy of the sync at its own delay.** The
  Povodoks send their own syncs in fixed slots, so nothing is named and no Belka sends for the
  time.
- **A live view above 100 Hz, a choice of rates, and a Belka sending again what the Povodoks
  missed.** The live view shows that the units sit and move as they should and that the signal
  holds; the record, at its own rate, is whole in every unit's memory and comes on the table. Raw
  steps at 200 Hz would fill the period and leave no room for the Povodoks to pass the first what
  it missed; more rates or a Belka's resend would take the air for nothing the analysis uses.
- **A live step's median, its RMS or a logarithmic sum.** The median drops a lone spike — the
  very strike that is looked for; the RMS loses the sign and the direction; a logarithmic sum is
  a soft peak with no meaning on signed values. The mean and the peak remain: the mean keeps the
  turn the gyro measures, so a model of the animal moves true; the peak shows a strike whole.
- **A mesh for the data too, every Belka repeating every packet.** Each packet would take the air
  once for every hop, every Belka's radio would stay on through every slot, and the firmware would
  time every repeat; what a far Belka sends unheard is not lost — it stays in its memory and
  comes afterwards.
- **Several commanders**, one on the head, one on each flank, passing data between them. The
  children choose their parents, so no relay is chosen by hand; one commander stays, Povodok.
- **A Belka as the commander, Vožak, talking to the phone over Bluetooth LE.** One radio shared by
  Bluetooth and the set's own periods took some 6 ms of every 20 ms and the timing's care, and
  Bluetooth was the narrowest link to the phone. One box more, Povodok, commands and reaches the
  phone by USB or Wi-Fi, and the Belka's firmware drops Bluetooth whole; Povodok keeps it for its
  first minutes alone, to receive the Wi-Fi network.
- **Povodok bridging to a commander on the animal**, so that the time would not hang on the
  handler's distance. It would be a second commander's code for one case; Povodok itself rides on
  the animal when the work is far from the phone.
- **Each Povodok to the phone, the phone keeping one of each.** The phone would join up to four
  Povodoks at once — four addresses, a star through a router or the phone's own hotspot, and none
  of it over a phone's one USB port. The first Povodok gathers the set instead, the others passing
  it what it missed in the period's own free time, and the phone joins that one.
- **The data encrypted, and TLS to the phone.** The data are an animal's motion, nothing to hide;
  what is guarded is the set itself — signatures keep a stranger from commanding it, the password
  is checked by a challenge and never sent, and the set's key reaches a new Belka under ECDH, once.
- **Povodok as a Wi-Fi access point.** A client follows its access point's channel and its
  country's rules, as every laptop does, and the access point answers for its clients
  ([`REGULATIONS.md`](REGULATIONS.md)).
- **Wi-Fi at 2.4 GHz to the phone.** It would deafen the set's own radio.
- **Every unit on its own Bluetooth link to the phone**, the way the cheap sets work. A phone
  holds a handful of links at once, seven on many Android phones, fewer on some
  ([Nordic DevZone](https://devzone.nordicsemi.com/f/nordic-q-a/77268/android-ble-issues-on-parallel-5-connection-establishments/319483)),
  and stamps time only as packets arrive.
- **Wi-Fi inside the set.** Its receiver alone draws 84 mA
  ([Espressif, ESP32-C3](https://documentation.espressif.com/esp32-c3_datasheet_en.html)) against
  3,3 mA for a Bluetooth LE receiver of the same class
  ([Nordic, nRF54LM20A](https://www.nordicsemi.com/Products/nRF54LM20A)), and a unit listens most
  of the time.
- **A charging station with a processor**, charging up to 32 Belkas in turns and passing their
  records to USB. The records go to the phone through Povodok and the firmware over the radio, so
  a cable from a USB port is enough.
- **An addressable LED, or an RGB one.** The addressable one wants a timed data line and draws
  current dark; an emerald green, InGaN at about 525 nm, wants about 3,3 V — typical for the green
  of Kingbright's APHB1608ZGSURKC ([DigiKey](https://digikey.cn/en/products/detail/kingbright/APHB1608ZGSURKC/2683994))
  — which a cell near empty does not give. Red and the classic green, about 570 nm at about 2 V,
  are enough for the state.
- **Two motion sensors a unit, back to back through the board.** Averaged, two take the white
  noise down by √2 and leave the drift they share
  ([a CubeSat IMU cluster](https://digitalcommons.usu.edu/smallsat/2014/AdvTechI/5/)); the
  displacement from an IMU is limited more by the integration and the fastening than by the noise
  ([Bosch et al., 2018](https://pmc.ncbi.nlm.nih.gov/articles/PMC5877382)).
- **Always ten samples summed to one.** A machine's knock, a jump or a study may want every
  sample, so N is chosen per session, 1 keeping them all.
- **A magnetometer.** Indoors — a clinic, a stable, steel in the floor and the walls, cables —
  the earth's field is bent: in motion laboratories the heading went up to 29° wrong
  ([de Vries et al., 2009](https://doi.org/10.1016/j.gaitpost.2008.12.004)). It serves in an
  open field and misleads where the vet works. The record needs none: the vertical comes from
  gravity, a stride's turning from the gyro, and the angles between neighbouring segments can be
  found without one ([Laidig et al.](https://arxiv.org/pdf/2002.00639)). A builder who wants it
  adds it, and every step grows by a third, 18 B for 12.
- **The application in Kotlin Multiplatform, Qt, .NET MAUI or Python.** .NET MAUI builds for
  Android, iOS, macOS and Windows, not Linux
  ([Microsoft, Supported platforms](https://learn.microsoft.com/en-us/dotnet/maui/supported-platforms));
  Kotlin Multiplatform shares the logic, the interface being Compose Multiplatform's or each
  platform's own ([JetBrains](https://www.jetbrains.com/compose-multiplatform/)); Qt is C++, with
  a licence of its own to weigh ([Qt, Licensing](https://www.qt.io/licensing)); Python runs on a
  computer, not on the vet's phone. Flutter builds all five from one code
  ([Flutter, Multi-platform](https://flutter.dev/multi-platform)), so the base is in Flutter.

## The parts

- **The nRF52832.** No Bluetooth long range, +4 dBm at most
  ([Nordic](https://www.nordicsemi.com/Products/nRF52832)).
- **The nRF52840.** It receives at 4,6 mA against the nRF54LM20A's 3,3 mA, its USB is
  full-speed, 12 Mbit/s, against high-speed, and its modules are larger, 10,5 × 15,5 mm
  ([Nordic](https://www.nordicsemi.com/Products/nRF52840); [SparkFun](https://www.sparkfun.com/products/21605)).
- **The nRF54L15, on Ezurio's BL54L15µ module.** The chip first chosen: no USB at all, 1,5 MB and
  256 KB. Set aside for the nRF54LM20A of the same family and SDK — high-speed USB, 2 MB and
  512 KB ([Nordic](https://www.nordicsemi.com/Products/nRF54LM20A)).
- **The nRF54LM20B.** The nRF54LM20A with an AI accelerator, pin for pin the same; Strelka needs
  no accelerator, so it would idle. Taken where its module, the BM20BF, is the one to be had.
- **The other nRF54LM20A modules.** ARAD's MN54LM-S20A, the smallest at 10,2 × 7,5 mm, brings
  out 30 of the 66 GPIO, so the pins would move, and its maker is new to Nordic's list
  ([ARAD](https://www.aradconn.com/product-detail/S20A-nrf54LM20A-module/)); Raytac's
  AN54LM-20A was "coming soon" in October 2026
  ([Raytac](https://www.raytac.com/product/ins.php?index_id=180)); Ezurio's BL54LM20A has an
  MHF4 connector only ([Nordic's list](https://www.nordicsemi.com/Products/nRF54LM20A/Modules)).
  Fanstel's BM20 series brings out every GPIO but its crystal's two, is approved in the US, the
  EU, Japan, Korea, Taiwan and Australia, and has a combo with the nRF7002 on the same land; it
  chose itself. **Raytac's Wi-Fi** is a pair, not a combo: its AN7002Q, the nRF7002, beside an
  nRF5340 module, two modules on the board.
- **Fanstel's BM20C, the chip antenna**, on the same land. A unit turns on the animal, and
  Fanstel's ranges, each the mean of twelve turns, put the BM20F's PCB antenna some 8 dB ahead of
  the chip at the same peak: 358 m at 2 Mbit/s against 189 m at 1 Mbit/s. The BM20M's small PCB
  antenna, 110 m, falls behind both. An antenna of the board's own, on the BM20P's pads or the
  BM20E's connector, would need its own tuning and lose the module's approvals.
- **Periodic advertising with responses**, Bluetooth 5.4's way for one device to talk with many
  without a connection
  ([Silicon Labs](https://docs.silabs.com/bluetooth/9.1.0/bluetooth-low-power-optimization/use-case-9-i-want-to-exchange-data-with-many-low-power-peripheral-devices)).
  Its timing belongs to the Bluetooth stack, and every unit would carry one for nothing.
- **Nordic's nPM1300 and a rail at 1,8 V from its buck.** Its VSYS carries VBUS unregulated, up
  to 5,5 V, while the cable is in (its Product Specification v1.1, 6.1.1), so the combo module,
  1,7–3,5 V, cannot hang on it; its bucks give 200 mA, and Povodok's nRF7002 draws 260 mA
  sending and asks 2,9 V; its fuel gauge runs in the host's software on sampled current. **With
  TI's TPS62840 beside it** in its 100 % mode: the high-side switch passes the input, 430–600 mΩ
  at 3,6 V (SLVSEC6D, 7.5), as an LDO in dropout, after the nPM1300's 160 mΩ between the cell and
  VSYS; a switching converter beside two radios, and the gauge still in software.
- **Other PMICs**, a charger and the rails in one chip: TI's **BQ25190**, whose buck-boost starts
  by itself at 4,8 V (SLUSF40A, its registers' reset values), which would burn the modules;
  ADI's **MAX77654**, 3,3 V for some 30 mA only; ADI's **MAX20345** and Qorvo's **ACT81460**,
  charging Li-ion alone; ADI's **ADP5360**, a buck-boost of some 190 mA and a gauge by voltage
  alone; NXP's **PCA9422**, starting at 5,0 V or 1,0–1,1 V; X-Powers' **AXP2101**, Li-ion alone;
  Nordic's **nPM6001**, a WLCSP; ADI's **MAX77658**, whose single-inductor buck-boost gives
  3,3 V from a cell at 3,2 V at some 30 mA (19-101207, table 12), the maker asking 0,7 V between
  input and output for buck or boost alone, which charges from 3,6 V, asks a custom model for
  LiFePO4 and an inductor beside the radios; Nordic's **nPM1304**, the nPM1300's faults with a
  charge of 100 mA at most. None found charges LiFePO4 to 3,5 V, feeds a 3 V rail of some
  300 mA and counts the charge: they are made for Li-ion.
- **TI's BQ25170 alone on the cable's 5 V, the board on its output**, as on 17 Oct. From 5 V it
  loses what an LDO would, (5,0 − 3,2) V × 0,3 A = 0,54 W at 300 mA; it ends the charge on its
  output's current, the load's with the cell's, so that a load over 30 mA never lets it end, and
  after its 10-hour timer it stays off until the cable or its TS pin is toggled (SLUSDJ8A,
  7.3.2.2–7.3.2.3). Behind a buck at 3,93 V and with the board on the ideal diodes it loses some
  0,19 W and sees the cell alone.
- **The BQ25170's kin**: TI's **BQ25170J** (SLUSEF8), the same with JEITA, but charging on at 50 %
  between 45 and 55 °C, over the cell's 45 °C; a resistor at the NTC that moved its hot stop to
  45 °C would leave the cold stop unreached — the NTC's 5,55 between 0 and 45 °C would have to
  grow to 7,71, and resistors only shrink it. **BQ25175** (SLUSDY7A), Li-ion at 4,35 V alone;
  **BQ25171-Q1** (SLUSDK0A), 3 × 3 mm, nothing gained.
- **TI's BQ25306**, the standalone buck of 18 Oct, 3 × 3 mm, set by resistors alone. No power
  path: its output charges the cell and feeds the load (SLUSDC7A, 10.1), so that on the cable the
  board runs on the cell between charges, and in the cold, the charge stopped, draws it down; no
  band of a lower current between 0 and 10 °C. Some 0,06 W lost against the present chain's 0,25.
- **Chargers set by I²C, with a power path**: TI's **BQ25628E** (SLUSFA4C), one chip of 2,5 × 3 mm,
  some 0,1 W, JEITA and a ship mode of 0,15 µA — but waking set for Li-ion, 4,20 V and 320 mA, so
  the firmware writes it at every start and a resistor holds its CE till then: set aside for a
  supply set by resistors alone, which no firmware can set wrong. TI's **BQ25640** (SLVSIK1),
  waking at 4,2 V and 2 A, made for amperes; **BQ25619E** (SLUSEC9B) and its twin in WCSP, the
  **BQ25618E**, whose 4,2 V comes back with their watchdog, 9,5 µA from the cell, 15 µA at most;
  **BQ25622E** (SLUSFA3C), the BQ25628E's twin of 3 A, waking at 1 040 mA. ADI's **MAX77884** and
  **MAX77885** (19-102050), 3,5 and 5,5 A, set over I²C.
- **Standalone switching chargers with a power path**, each keeping its selector awake on the
  cell: ADI's **LTC4040** (4040fb), LiFePO4 at 3,45–3,6 V by pins, but a backup that boosts the
  cell to SYS, 40 µA from it, 70 µA at most; ADI's **MAX77757** (19-100986), 3,6 or 3,7 V,
  30 µA from the cell, 50 µA at most, its input bearing 16 V, under the TVS's 19,9 V; ADI's
  **MAX8903** (A–Y), Li-ion at 4,1–4,35 V alone.
- **Bucks for the chain**: TI's **TPS629210** (SLVSGE0B), 3,8 V by one resistor and no bootstrap,
  but bearing 18 V, under the TVS's 19,9 V; ADI's **MAX17242** and **MAX17243** (19-7767), 42 V,
  but 5 × 5 mm, compensated outside, 3,3 or 5 V fixed. **A power mux**, TI's **TPS2116**
  (SLVSFG1A), 1,32 µA: two ideal diodes do it.
- **TI's LM66100, two in TI's ORing for continuous power**, the chain of 18 Oct: 91 mΩ each at
  3,6 V (SLVSEZ8A, 6.5), and no way to turn both off, so that the board had no true off. The
  LM66200 is the two in one, 40 mΩ, with one ON for both.
- **TI's load switches TPS22910A and TPS22912C** (SLVSB49F), 61 mΩ at 3,3 V, an enable and a
  current back blocked, 2 µA on at 3,6 V: one channel each, so two parts for VBUCK and the cell.
- **The cable switching the Belka on**, a transistor from VBUCK onto ON: it would run through
  every charge and could not be switched off with the cable in. The charge runs without it, the
  charger fed from VBUCK, and the gauge counts it hibernating. Povodok has it, Q3: under a
  ceiling, out of a magnet's reach, the cable is its switch.
- **A pulse from the cable** for Povodok, a capacitor from VBUCK into Q3's gate and a resistor to
  ground: a true off with the cable in, at two parts more, where on the cable an asleep Povodok
  draws on the cable alone.
- **The gauge shut down while the board is off**, 0,5 µA against 7 (the MAX17055 data sheet,
  IDD0, IDD1): it would not count a charge made with the board off, and a LiFePO4 cell's flat
  curve would not tell it afterwards.
- **Magnet A read through a diode**, Nexperia's low-leakage BAV170: its drop, 0,9 V at most at
  1 mA (its sheet, table 7), with the Hall switch's 0,3 V, could put the processor's pin between
  its VIL and VIH. A BSS138 passes it with none to speak of. **A third Hall switch** under mark A
  for the reading: a part and 1,6 µA more.
- **A P-MOSFET from magnet A's switch onto ON**, AOS's AO3401A, its gate on the switch's output:
  it pulls ON down only to its threshold, 0,5–1,3 V, into the LM66200's 0,8–1,2 V, and leaks up
  to 1 µA at 25 °C, 5 µA at 55 °C. The BAT54 takes ON to 0,54 V at the most.
- **The Schottky diodes of the basic parts**, SS14, SS34, 1N5819WS: parts of 1–3 A, leaking up
  to 0,3–0,5 mA at 40 V (JLCPCB's listings). The BAT54 leaks 2 µA at most at 25 V.
- **Linear chargers with a power path**, each losing what an LDO would, some 0,5 W at 300 mA from
  5 V: TI's **BQ25185** (SLUSF65B), set by resistors, 3,6 V at the least, SYS at 4,5 V, 4 µA from
  the cell; TI's **BQ25180** (SLUSE99C) and **BQ25188** (SLUSFJ3), 1,6 × 1,1 mm, 3,5 V by I²C,
  but waking at 4,20 V with the charge on, their watchdog bringing 4,2 V back after 160 s without
  I²C, no CE pin, and from the cell alone starting only above 3,08–3,46 V (VBATPOR), around a
  LiFePO4 cell's 3,2 V. ADI's **LTC4098-3.6** (409836f), a switching one made for LiFePO4: 3,6 V
  fixed, 25 µA from the cell, 35 µA at most.
- **Standalone bucks at 3,6 V**: TI's **BQ25300** (SLUSCZ2) and **BQ25308** (SLUSFD7), no
  3,50 V, and the BQ25308 refuses a cell under 2 V. ADI's **LTC4121** (4121fc): an inductor of
  some 15 µH, a shunt outside, switches of 0,8 and 0,5 Ω, 7–15 µA from the cell through its
  switch node. TI's **BQ24620**: a controller, its FETs outside.
- **Linear chargers made for LiFePO4**: Consonance's **CN3058E**, 3,6 V that its resistor can
  only raise, an input of 6,5 V at most, an SOP-8; Microchip's **MCP73123**, 3,60 V fixed, a DFN
  of 3 × 3 mm (DS22191E). Each as hot as any linear charger.
- **Chips that charge, count the charge and guard the cell**: ADI's **MAX17335**, 2,42–4,8 V, but
  LiFePO4 only after ADI's characterisation of the cell, the system on the charge source's 5 V,
  two FETs outside and a WLP at 0,5 mm (19-101884); ADI's **LTC4015**, 5 × 7 mm with all its power
  outside, 112–325 µA standing and 78 µA more for its counter, made for amperes (4015fb); ADI's
  **MAX20360**, charging Li-ion at 4,05–4,6 V; TI's **BQ25157**, 3,6–4,6 V, its ADC reading the
  charge current alone; ADI's **MAX17330** (19-100962), set once in its nonvolatile memory, but
  3,56 V at the least and 21 µA from the cell in hibernation, 42 µA at most, its FETs on.
- **Gauges that keep their settings in nonvolatile memory**, set once at production: ADI's
  **MAX17201** and **MAX17211** (19-8424), 9 µA in hibernation, the memory written 7 times at
  most, LiFePO4 only with a model of the cell made by ADI; TI's **BQ27220** (SLUSCB7A, SLUUBD4A),
  9 µA asleep, written once at 7,4 V on GPOUT, LiFePO4 only with CEDV parameters made from the
  cell's own discharges. The MAX17055 has LiFePO4's model, ModelID 6, at 7 µA.
- **A buck or a buck-boost for +3V0**, TI's **TPS63900** or a small buck in its 100 % mode. From
  3,1–3,5 V an LDO keeps 86–97 %, a buck-boost some 90 % (SLVSET3D, figures 8-7 and 8-9), and a
  buck in its 100 % mode is an LDO in dropout; either switches beside the radios on the animal,
  and the TPS63900 takes some 21 mm² with its parts (its features).
- **TI's TLV75530**, the LDO of 17 Oct: 25 µA of its own, 33 µA at most (SBVS320D), on the rail
  that never sleeps, against the TPS7A2030's 6,5 µA in the same work.
- **A voltage supervisor on U5's EN**, TI's **TPS3840PL30**, cutting +3V0 under 3,0 V: the
  firmware switches the board off at 3,1 V and the cell's own board cuts it at 2,3 V.
- **Promax's four-pin F708-04PB**, the contact of 17 Oct: VBUS and GND on its outer pins, so that
  a plug forced on turned round puts −5 V on VBUS, past the charger's −2 V and the NCP361's
  −0,3 V, and asks a reverse guard — AOS's AO3401A with two resistors, some 13 mm² and 43 mV at
  500 mA. Five pins wired symmetric leave a plug turned round harmless.
- **A rail of 3,3 V**, as Fanstel advises for its combos and feeds them on its boards: on a
  LiFePO4 cell an LDO at 3,3 V spends most of the discharge in dropout, where it no longer
  regulates. **Torex's XC6220**, 1 A and 0,2 Ω, drops its 3,0 V output by some 0,27 V for a step
  of 0,1 to 300 mA in its power-save mode (ETR0341-013, figure 9). TI's TLV757P, Diodes' AP2112,
  Richtek's RT9013 and RT9080, onsemi's NCP161 and TI's LP5912 and TLV1117LV are no lower in
  dropout than the TLV755P; TI's TLV758P was not to be had at JLCPCB.
- **ADI's MAX17262H**, the gauge with its sense resistor inside, in the cell's plus: its balls at
  0,4 mm would ask JLCPCB's Standard assembly. The MAX17055 counts on an outside 20 mΩ in the
  cell's minus and comes in a TDFN.
- **A polymer tantalum of 100 µF on SYS**, KEMET's T520A107M006ATE070: a leakage of up to 63 µA
  at 6,3 V (KEMET's T52X sheet), not given at 3–3,5 V, on a rail that is never off. A ceramic of
  100 µF in 1206 keeps half its value and leaks nothing.
- **Parts sold under the ICM-42688-P's name by Tokmas and HXY** at LCSC. They name their own
  makers, not TDK, and HXY's sheet (Huaxuanyang, for C46550687) shows another chip under the
  name: WHO_AM_I 0x6A at 0x01 and the I²C address 0x18 or 0x19, where TDK's is 0x47 at 0x75 and
  0x68 or 0x69 (DS-000347, 14.58 and 9.3); pin 9 INT2 alone, no CLKIN; the gyro's noise
  6 mdps/√Hz against 2,8; the accelerometer to 1,6 kHz and the gyro to 3,2 kHz against 32 kHz;
  1,00 mm tall; marked 7I22.
- **The ICM-45686 on the sensor's land**, cheaper and newer. Another register map, no 1 kHz rate,
  6,4 kHz at most, and its sheet does not say how its rates follow CLKIN, on which the clock
  stands; the ICM-42686-P gives the ±32 g with the same registers. **The ICM-42688-V** is the -P
  with a library, WHO_AM_I 0xDB: not a part of its own, read as a -P if fitted.
- **The proprietary mode at 4 Mbit/s.** Bluetooth's 2M is what the module's approvals cover; the
  4 Mbit/s mode is not, and 2 Mbit/s carries 40 units at 100 Hz live.
- **A Li-poly cell**, EEMB's LP403040, 460 mAh at 4,3 × 30,5 × 41 mm, 500 cycles, kept between
  20 and 80 % to last. Set aside for a LiFePO4 pouch of the same footprint, LTOPOW's LFP103042
  at 720 mAh and 2 000 cycles, 10 mm thick, used whole; the charger would take a Li-poly cell
  by other settings in the firmware. EEMB's LP753042, 1 000 mAh at 7,8 × 30,5 × 43 mm, would
  give 3,7 Wh against the LFP103042's 2,3 and a gauge by voltage alone; set aside for safety on
  an animal and some 500 cycles.
- **Motion sensors with no CLKIN** — the ICM-42670-P, the BMI270. The fractional clock needs
  CLKIN; with the fallback in [`PRINCIPLE.md`](PRINCIPLE.md), the samples put on the grid by
  computation, they could serve.
- **A waterproof USB-C socket in the box**, and a second build beside it with a two-pin magnetic
  contact. One build, one contact: the five-pin magnetic contact charges and carries USB, and
  the box keeps one opening. A two-pin contact carried no data and, symmetrical, went on
  backwards.
- **A push button.** The box is sealed; the magnets' Hall switches work the unit through it.
- **A memory on the board: the NOR flash, Winbond's W25Q256JW, and the SLC NAND, the
  W25N02KW.** The NOR holds 32 MB, some 7 h at 100 Hz, and erases a block in 0,2–2 s; the NAND
  256 MB, some 11 min of the sensor's raw 32 kHz, both at 1,8 V, while the card and the Wi-Fi ask
  3 V. A microSD card in a slot holds a whole examination of raw data, a 16 GB card some 8 h at
  32 kHz, keeps its own bad blocks and wear, and is the user's to choose
  ([`PARTS.md`](PARTS.md), *The memory*).
- **Other slots for the card.** Würth's 693072010801, a hinged lid, its drawing not to be opened;
  Hirose's DM3D-SF, push-pull, the card held by friction alone; Molex's 503398-1892, push-push,
  springing the card out. Megastar's ZX-TF-281.6J locks its lid.
- **The card's 4-bit SD mode.** The nRF54LM20A has no SD host, its only 4-bit interface the
  QSPI emulated on its RISC-V core; SPI on SPIM20 carries some 1 MB/s against the record's
  some 0,55 MB/s, and the card's own pauses while it writes are the same on either bus.
- **The sensor on the card's bus**, three pads fewer under the module. A card switched off would
  sit on a live bus, which the SD standard forbids (6.4.1.5), unless the sensor lost its wake by
  motion or the bus were used only with the card on; some cards hold DO after CS rises; the
  sensor would wait on the card's pauses. Two buses, SPIM21 and SPIM20.
- **The card on SPIM00, the 32 Mbit/s SPI on its own pins on P2.** Its core runs at 128 MHz and its
  divisor reaches 126 at most, so it clocks no slower than 1,016 MHz (the nRF54LM20A datasheet v1.0,
  the SPIM configuration table), while a card asks 100–400 kHz at its identification (the SD
  Physical Layer Simplified Specification 9.10, 6.4.1), and Nordic sends SD cards to SPIM20–30. A
  start at 1 MHz, which many cards bear and the standard does not promise, and a start clocked by
  software on the same pins were set aside: the card sits on SPIM20 at 8 Mbit/s, some 1 MB/s against
  the record's 0,55 MB/s at most.
- **Smaller packages**: the NCP361 in a 2 × 2 mm UDFN, the MAX17055 in its 1,4 × 1,5 mm WLP, for
  pads that ask vias in or round them on a dearer board; the WLP's balls at 0,4 mm, under
  Economic assembly's 0,5 mm.
- **ARM's 10-pin Cortex header for SWD**, Samtec's FTSH-105: too large for the box. Tag-Connect's
  pads take the same cable's signals with no part on the board.
- **A reset supervisor**, TI's TPS3422 or TPS3420D, cycling the supply on a long hold of magnet A:
  their fixed times, 7,5 s and 12,5 s, collide with the hold of 6 s that switches the unit off;
  a hung firmware is reset by the chip's watchdog.
- **The cell's protection on the board**, Hycon's HY2112-AB with a dual MOSFET. At the cell it
  guards the lead as well, so the cell is bought with its protection board.
- **A resistor to filter the sensor's supply.** One that filtered, 10 Ω, would drop some 7 mV at
  0,7 mA and leave the radios' frequencies to the capacitors; a ferrite bead drops some 35 µV
  and takes them.
- **A diode across VBUS against a cable put on backwards**, the M7 first. It shorts the reversed
  supply at its own forward drop, some 0,8–1 V at 0,1–1 A, past the −0,3 V the NCP361's input
  bears (NCP361/D, maximum ratings), so the chips' own diodes would share the current; a
  Schottky, 0,3–0,5 V, still passes −0,3 V at amperes. Five pins wired symmetric leave no
  reversed supply to guard ([`BOARD.md`](BOARD.md), J1).
- **ROHM's BD6047AGUL**, a charge protection IC made for a USB port: ±30 V on its input, a
  reversed plug blocked, cut off above 5,85 V, 125 mΩ, in a 1,95 mm ball package (its datasheet,
  rev. 001, 2013). It would guard a reversed plug and the processor's VBUS in one part, but neither
  JLCPCB nor LCSC carries it, so it would be bought elsewhere and sent to the assembler.
