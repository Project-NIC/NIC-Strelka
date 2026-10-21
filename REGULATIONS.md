# The radio rules

> **Read before switching on, wherever you are.** Strelka transmits on 2.4 GHz, every unit, and
> Povodok on 5 GHz Wi-Fi too, as a client. The access point it joins — a phone's hotspot, a
> laptop, a building's network — chooses the channel and keeps to the rules of the country it
> runs in, indoors or out, as for any other client: set it to that country and to 5 GHz. The
> rules below are the Czech Republic's and the European Union's, quoted from their texts;
> elsewhere they differ. The exact rules are the national office's — in the Czech Republic ČTÚ,
> [ctu.gov.cz](https://ctu.gov.cz) — and an AI assistant will help find and read them; the
> office's text decides.

## What Strelka sends

| radio | sent by | band | at most | as |
|---|---|---|---|---|
| Strelka's own radio | every Belka, Povodok | 2400–2483,5 MHz | by default +5 dBm out, some 8 mW e.i.r.p.; the full power, a setting, some 17 mW | a non-specific short-range device; at the full power a wideband data transmission device |
| Bluetooth LE, only to give Povodok its Wi-Fi network, in the first minutes after it is switched on | Povodok | 2400–2483,5 MHz | 10 mW e.i.r.p., sent at the default +5 dBm | a non-specific short-range device |
| Wi-Fi, a client only | Povodok | 5 GHz, the channel the access point chooses | what that channel allows, the access point's word | a wireless access system (WAS/RLAN) |

## What the design keeps to

1. **Povodok is a Wi-Fi client, never an access point.** The access point chooses the channel and
   answers for its clients: "Za škodlivou interferenci způsobenou klientskou stanicí (v tzv. režimu
   Slave) zodpovídá provozovatel přidruženého přístupového bodu" (VO-R/12/11.2021-11, čl. 1
   odst. 1 písm. q).
2. **Povodok joins the access point on whatever 5 GHz channel it uses**, as a laptop or a phone
   does, and takes the country from the access point's beacon (IEEE 802.11d). Whether a channel
   may be used outdoors, and the radar detection, are the access point's: in the EU, 5150–5250
   MHz is open indoors and, for equipment not fixed outdoors, outside (2022/179, Table 1, note 1;
   VO-R/12, b1); 5250–5350 MHz indoors only; 5470–5725 MHz indoors and out, with the access
   point's radar detection. Only 2.4 GHz is refused: it would deafen Strelka's own radio.
3. **Under 10 mW e.i.r.p. on 2.4 GHz by default** — the EU's limit for a non-specific short-range
   device there (2006/771/EC, row 57a); the Czech Republic allows 25 mW (VO-R/10/05.2025-5, čl. 3,
   pásmo j). +5 dBm out and the BM20F's antenna, +12,61 dBm radiated from +8,37 dBm out (its
   specification v1.00), make some +9,2 dBm e.i.r.p., 8 mW; under 10 dBm e.i.r.p. EN 300 328 asks
   neither adaptivity nor the limits on a duty cycle and a medium utilisation (V2.2.2, 4.3.2.4.1,
   4.3.2.5.1, 4.3.2.6.1). **The full +8 dBm**, some 17 mW, is a setting: within the Czech Republic's
   25 mW for a short-range device; in the EU a wideband data transmission device, 100 mW and 10
   mW/MHz (2006/771/EC, row 57c), the set keeping EN 300 328's terms for equipment that does not
   adapt — a Tx-sequence of 10 ms at most, a Tx-gap after it as long and 3,5 ms at least, a medium
   utilisation of 10 % at most (4.3.2.4.3, 4.3.2.5.3), the norm's test counting a silence under 3,5
   ms into the sequence (5.4.2.2.1.3) — which a measurement's timetable keeps and a download does
   not, so a download goes at the default power ([`PRINCIPLE.md`](PRINCIPLE.md), *The reach*). The
   module's own report under EN 300 328 (Fanstel's ISL-26LR0132E328BLE, August 2026) covers
   Bluetooth LE at 7 dBm as adaptive equipment under 10 dBm e.i.r.p.; the full power, non-adaptive
   above it, is assessed on the finished unit, its duty cycles declared (5.4.1).
4. **The built-in antennas alone, no amplifier** (VO-R/10, čl. 2 odst. 2).
5. **In a road vehicle Povodok's Wi-Fi is limited to 40 mW**, and none of Strelka is for a drone
   (VO-R/12, čl. 1 odst. 1 písm. g; 2022/179, Table 1).

## Elsewhere

Outside the EU, from the Linux kernel's regulatory database alone (wireless-regdb 2026.09.03, the
national texts unread): the United States allow 5150–5250 MHz outdoors; Japan has 5170–5250 MHz at
20 dBm and no 5725–5875 MHz. What is still open from the rules is in [`STATUS.md`](STATUS.md).

## The texts

### The Czech Republic

**VO-R/12/11.2021-11** — the general authorisation for wideband data transmission, Telekomunikační
věstník 11/2021, 12 November 2021, in force from 1 December 2021; the Czech text binds:

| ozn. | kmitočtové pásmo | maximální vyzářený výkon | maximální spektrální hustota e.i.r.p. | další podmínky |
|---|---|---|---|---|
| a | 2400,0–2483,5 MHz | 100 mW e.i.r.p. | 10 mW/1 MHz; 100 mW/100 kHz (FHSS) | systémy s technikou DSSS nebo OFDM; FHSS |
| b1 | 5150–5250 MHz | 200 mW e.i.r.p. | 10 mW/MHz | viz odst. 1 písm. g |
| b2 | 5250–5350 MHz | 200 mW e.i.r.p. | 10 mW/MHz | pouze pro použití uvnitř budovy |
| b3 | 5470–5725 MHz | 1 W e.i.r.p. | 50 mW/MHz | — |
| b4 | 5725–5850 MHz | 1 W e.i.r.p. | 50 mW/MHz | pouze v oblastech podle odst. 1 písm. j |

> čl. 1 odst. 1 písm. f) v pásmech b2, b3, b5, b6 a d1 až d4, jakož i v případě venkovního užití
> v pásmech b1 a b4 musí být použity techniky přístupu ke spektru a zmírnění rušení, které
> poskytují přinejmenším rovnocenný účinek jako techniky popsané v harmonizovaných normách.
>
> písm. g) stanice v pásmu b1 instalované ve vozidlech a ve vlacích s průměrným útlumem stěn
> vagonu < 12 dB mohou mít maximální vyzářený výkon pouze 40 mW e.i.r.p. V úseku 5150–5170 MHz
> nemohou být provozovány stanice využívané bezpilotními systémy (UAS – Unmanned Aircraft
> Systems);
>
> písm. h) stanice v pásmu b4, které jsou umístěny vně budovy, mohou být pouze pevně instalované;
>
> písm. q) stanice nesmí způsobovat škodlivou interferenci stanicím radiokomunikačních služeb,
> které využívají rádiové kmitočty podle Plánu přidělení kmitočtových pásem, a nemají nárok na
> ochranu před škodlivou interferencí způsobenou těmito stanicemi. Za škodlivou interferenci
> způsobenou klientskou stanicí (v tzv. režimu Slave) zodpovídá provozovatel přidruženého
> přístupového bodu (v tzv. režimu Master);


**VO-R/10/05.2025-5** — the general authorisation for short-range devices, Telekomunikační věstník
5/2025, 20 May 2025, the newest found:

> čl. 2 odst. 2 Zařízení lze provozovat pouze s vestavěnou anténou nebo s anténou, kterou stanoví
> výrobce. Zařízení nesmí být provozována s přídavnými zesilovači vysokofrekvenčního výkonu
> a s převáděči.
>
> čl. 3 odst. 1 Podle článku 3 je možné provozovat všechny druhy zařízení splňující technické
> podmínky stanovené pro dané kmitočtové pásmo, bez ohledu na jejich použití nebo účel. Typická
> využití zahrnují například telemetrii, dálkové řízení, poplašné systémy či přenos dat obecně.

| ozn. | kmitočtové pásmo | vyzářený výkon | harmonizovaná norma |
|---|---|---|---|
| j | 2400–2483,5 MHz | 25 mW e.i.r.p.; další podmínky podle odst. 9, the ISM note | — |
| k | 5725–5875 MHz | 25 mW e.i.r.p. | ČSN ETSI EN 300 440 |

### The European Union

**Commission Implementing Decision (EU) 2022/179**, its Annex as replaced by **(EU) 2022/2307**:

> Table 1 — 5 150-5 250 MHz. Permissible operation: Indoor use, including installations inside
> road vehicles, trains and aircraft, and limited outdoor use (note 1). Use by unmanned aircraft
> systems ("UAS") is limited to within the 5 170-5 250 MHz band. Maximum mean equivalent
> isotropically radiated power (e.i.r.p.) for in-band emissions: 200 mW. Exceptions: 40 mW
> maximum mean e.i.r.p. applies for installations inside train carriages with an attenuation loss
> on average of less than 12 dB; 40 mW maximum mean e.i.r.p. applies for installations inside
> road vehicles. Maximum mean e.i.r.p. density for in-band emissions: 10 mW/MHz in any 1 MHz
> band.
>
> Note 1: If used outdoors, equipment shall not be attached to a fixed outdoor antenna, fixed
> infrastructure or to the external body of road vehicles.
>
> Table 2 — 5 250-5 350 MHz. Permissible operation: Indoor use: inside buildings only.
> Installations in road vehicles, trains and aircraft are not permitted (note 2). Outdoor use is
> not permitted.
>
> Table 3 — 5 470-5 725 MHz. Permissible operation: Indoor and outdoor use. […] Mitigation
> techniques to be used: Transmitter power control (TPC) and dynamic frequency selection (DFS).

**Commission Decision 2006/771/EC** on short-range devices, its Annex as replaced by
**Implementing Decision (EU) 2025/105**:

| row | band | category | limit |
|---|---|---|---|
| 57a | 2 400-2 483,5 MHz | non-specific short-range devices | 10 mW e.i.r.p. |
| 57c | 2 400-2 483,5 MHz | wideband data transmission devices | 100 mW e.i.r.p.; 10 mW/MHz when other than frequency hopping; techniques to access spectrum and mitigate interference apply |
| 61 | 5 725-5 875 MHz | non-specific short-range devices | 25 mW e.i.r.p. |

### Sources

- ČTÚ, VO-R/12/11.2021-11, Telekomunikační věstník 11/2021:
  [portal.gov.cz](https://portal.gov.cz/vestniky/a9qaats/967099241.pdf); the office's
  unofficial English translation:
  [ctu.gov.cz](https://ctu.gov.cz/en/sites/default/files/obsah/stranky/74784/soubory/vo-r-12-11.2021-11enfin.pdf).
- ČTÚ, VO-R/10/05.2025-5, Telekomunikační věstník 5/2025:
  [gov.cz](https://gov.cz/vestniky/a9qaats/1531028547.pdf).
- [Implementing Decision (EU) 2022/179](https://eur-lex.europa.eu/eli/dec_impl/2022/179/oj) and
  [(EU) 2022/2307](https://eur-lex.europa.eu/eli/dec_impl/2022/2307/oj); Decision 2006/771/EC and
  [Implementing Decision (EU) 2025/105](https://eur-lex.europa.eu/eli/dec_impl/2025/105/oj).
- ETSI EN 300 328 V2.2.2 (2019-07), wideband transmission systems in the 2,4 GHz band:
  [etsi.org](https://www.etsi.org/deliver/etsi_en/300300_300399/300328/02.02.02_60/en_300328v020202p.pdf).
- [wireless-regdb](https://git.kernel.org/pub/scm/linux/kernel/git/wens/wireless-regdb.git),
  release 2026.09.03, `db.txt` — a good-faith compilation, not law; its Czech entry still cites
  the authorisation of 2010.
