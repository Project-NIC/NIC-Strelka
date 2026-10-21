<div align="center">

<img src="NIC-Strelka.jpg" width="420"/>

# NIC-Strelka

[English](README.md) · **Čeština** · [Русский](README.ru.md)

**Pohyb zvířete pro veterinární vyšetření:** malé jednotky připevněné na nohách, trupu a hlavě psa,
koně nebo krávy zaznamenávají, jak se zvíře hýbe, všechny na jednom společném čase, takže jde každou
nohu, trup a hlavu položit vedle sebe, krok po kroku. Živě v telefonu veterináře, kam dosáhne rádio,
a celý záznam potom.

**Náhrada za levné sady senzorů, které se dnes prodávají** — za jejich cenu nebo níž, pokud to
půjde, v každém případě lepší.

</div>

> **Beta 0.7:** návrh na papíře, firmware sestavený proti SDK, aplikace začatá ve Flutteru, nic
> ještě neběželo — jedna deska pro Belku i Povodok: nRF54LM20A na modulu Fanstel BM20F, pro Povodok
> s nRF7002 na modulu WM02C20C na stejných ploškách; karta microSD; USB na pětipinovém kontaktu;
> článek LiFePO4; kalibrace, tabulka sady v každém záznamu, živý náhled až 100 Hz, záznamy jako disk
> a záznamy celé sady jedním kabelem; co je uzavřené, co otevřené a co řekne až postavený kus, je ve
> [stavu projektu](STATUS.md) (anglicky). Ostatní stránky jsou anglicky.

> **Rádiová pravidla — přečtěte si je před zapnutím, ať jste kdekoli.** Strelka vysílá na 2,4 GHz a
> Povodok navíc na Wi-Fi 5 GHz, jako klient: přístupový bod, ke kterému se připojí — hotspot
> telefonu, notebook, síť budovy — volí kanál a drží se pravidel země, kde běží, takže ho nastavte
> na tu zemi a na 5 GHz. Co říká Česká republika a Evropská unie, citované z jejich textů, je v
> [`REGULATIONS.md`](REGULATIONS.md); jinde se pravidla liší a přesná jsou ta národního úřadu — v
> Česku ČTÚ.

---

## Co to je

Sada stejných jednotek, jedna až dvaatřicet, připevněných na zvíře suchým zipem, na popruhu nebo na
botě na kopyto. Každá nese pohybový senzor, rádio s procesorem, paměť na celé vyšetření a článek
nejméně na patnáct minut. Povodok, krabička sama pro sebe, je řídí: posílá čas, sbírá, co si telefon
žádá, a předává to dál — jeden až čtyři Povodoky v sadě, kolem zvířete, kvůli dosahu, zpravidla tři.

**Všechno na zvířeti.** Pohyb měří jen jednotky na těle: žádná kamera, žádný chodník, nic
postaveného kolem zvířete. **Jedna sada pro každé zvíře:** pes, kůň, kráva; sada se přizpůsobí
počtem jednotek, ne stavbou pro jeden druh. **Jen pohyb:** Strelka nesleduje, kam zvíře jde.

## Části

| část | co to je |
|---|---|
| [**Belka**](PARTS.md) | jednotka připevněná na zvířeti, se svým senzorem a kartou, na desce, kterou sdílí s Povodokem |
| [**Povodok**](NAMES.md) | velitel sady, krabička sama pro sebe, jeden až čtyři v sadě kvůli dosahu, první velí: posílá SYNC, sbírá, co si telefon žádá, a předává to telefonu nebo počítači, po USB nebo Wi-Fi; tatáž deska s modulem Wi-Fi, bez senzoru a karty |
| [**Pušinka**](APP.md) | aplikace v telefonu veterináře; její základ v [`app/`](app/), časem možná celé aplikace |
| **nabíjecí stanice** | nabíjecí stanice pro celou sadu s rozbočovačem USB uvnitř: jednotky se v ní nabíjejí a každá Belka v ní je disk, počítač je zkopíruje všechny najednou |
| **Budka** | krabička, ve které Belka bydlí: utěsněné pouzdro, okénko pro světlo, uchycení |

Proč psi a co znamenají jména, je ve [jménech](NAMES.md) (anglicky).

## Co existuje

Systémy, které měří pohybující se zvíře inerciálními senzory na těle:

- **Levné sady — to, co Strelka nahrazuje.** Bezdrátové jednotky MPU-9250 prodávané po čtyřech v
  sadě, typu WitMotion WT901BLECL: Bluetooth LE 5.0, výstup až 200 Hz, asi 48 $ za jednotku
  ([RobotShop](https://www.robotshop.com/products/witmotion-witmotion-wt901blecl-mpu9250-high-precision-9-axis-gyroscope-accelerometer-magnetometer-3-axis-bluetooth-ahrs-imu-sensor-arduino)),
  nejvýš čtyři najednou
  ([Desertcart](https://desertcart.lk/products/511015272-bluetooth-5-0-accelerometer-inclinometer-wt-9011-dcl-mpu-9250-high-precision-9-axis-gyroscope-angle-xy-0-2-accuracy-magnetometer-with-kalman-filter-low-power-3-axis-ahrs-imu-sensor-for-arduino)).
- **Kůň.** Equinosis Lameness Locator, prodávaný veterinářům: senzory na hlavě, pánvi a pravé hrudní
  končetině, data proudí do tabletu, zatímco kůň klusá ([University of
  Adelaide](https://www.adelaide.edu.au/vet/ua/media/528/Client-Brochure_2019-web.pdf), [Horse
  Sport](https://horsesport.com/magazine/business/pinpointing-lameness-with-technology/)).
  EquiMoves, výzkumný systém: až osm synchronizovaných senzorů na záhlaví, kohoutku, křížové kosti,
  hrudní kosti a na každé noze ([Bosch et al., *Sensors* 2018](https://doi.org/10.3390/s18030850)).
- **Pes.** Žádný systém prodávaný pro psy jsme nenašli. Lameness Locator s upraveným softwarem byl
  vyzkoušen na klusajících psech ([Stenman, SLU 2019](https://stud.epsilon.slu.se/14611/));
  GaitKeeper je výzkumný systém
  ([PMC5335924](https://www.ncbi.nlm.nih.gov/pmc/articles/PMC5335924/)).
- **Kráva.** Jen výzkum: akcelerometry na pánevních končetinách na čtyřech farmách ([*Animal*
  2015](https://resolve.cambridge.org/core/journals/animal/article/abs/lameness-detection-via-legmounted-accelerometers-on-dairy-cows-on-four-commercial-farms/2B43F96D169CE6C73F559435AE828915)),
  devět senzorů na těle krav s vyvolaným kulháním na hrudní končetinu ([Leclercq et al., *Animal*
  2025](https://pub.epsilon.slu.se/id/document/20449105)).

## Jak víme, že to funguje

Na stejném zvířeti ve stejnou chvíli ukáže Strelka totéž, co systém prodávaný veterinářům — to je
míra toho, zda funguje, ne jeho cena. U koně je to takový systém; u psa a krávy, kde se žádný
neprodává, výzkumný systém nebo změny, které uvádějí jeho publikované studie.

## Co musí umět

Rozsahy a řády; přesné hodnoty přijdou se součástkami.

| | požadavek |
|---|---|
| jednotek v sadě | 1 až 32 na jednom zvířeti; firmware jich unese 63; a jeden až čtyři Povodoky |
| zvířata | pes, kůň, kráva — tatáž sada |
| do paměti jednotky | asi 100 Hz, podle sezení až několik kHz |
| živě | 50 nebo 100 Hz na jednotku, natočení jako kvaterniony nebo surové kroky, podle sezení; co rádio zmešká, je v paměti jednotky |
| společný čas jednotek | hluboko pod jedním vzorkem: mikrosekundy |
| záznam | celé vyšetření, v každé jednotce; paměť ho neomezuje |
| jedno nabití | nejméně 15 min |
| dosah | kolem těla zvířete, až čtyři Povodoky kolem něj, Belku stačí slyšet kterýkoli z nich; k telefonu nebo počítači přes první Povodok, kabelem USB nebo Wi-Fi na 5 GHz |
| velikost | krabička od sirek nebo větší |
| hmotnost | bez stanovené meze; vyplyne ze součástek |
| uchycení | na popruhu nebo na botě na kopyto, suchým zipem |
| podmínky | uvnitř i venku; voda, bláto a kopnutí; mezi zvířaty čištění dezinfekcí |
| obrazovka veterináře | telefon, Android nebo iPhone, nebo počítač |
| cena | jako levné sady nebo níž — asi 48 $ za jednotku, čtyři v sadě, před clem a poštovným — a v každém případě lepší |

## Jak je to postavené

Bloky a to, co je spojuje; součástky jsou v [`PARTS.md`](PARTS.md).

```
  Pušinka, v telefonu nebo počítači
        │  USB na magnetickém kontaktu, nebo Wi-Fi na 5 GHz:
        │  Povodok jako klient hotspotu telefonu, notebooku nebo sítě budovy
  Povodok 1 … 4, vodítka — první velí
        │  vlastní rádio Strelky na 2,4 GHz: SYNC každého Povodoku ve vlastním slotu;
        │  Belka vysílá ve vlastním slotu, mezi SYNCy na vlastní hodiny
  ┌─────┼──────┬────────┐
Belka Belka  Belka …  Belka       každá slyší kterýkoli Povodok; žádná nic nepřeposílá
                                  (až 32 jednotek v sadě)
```

**Belka** je jedna deska ve své Budce, utěsněné krabičce:

| blok | co dělá | s procesorem ho spojuje |
|---|---|---|
| rádio a procesor, jeden čip | slyší SYNC, svůj slot, smyčka hodin, záznam | — |
| pohybový senzor | gyro a akcelerometr; vzorky uložené, jak přijdou, nebo sečtené po N, N podle sezení | sériová sběrnice; jeho vstup hodin, který dělá procesor; jeho linka data-ready |
| paměť | celý záznam vyšetření | sériová sběrnice |
| nabíječka, napájení a měřič | nabíjí článek z kontaktů a dělá napájení, nastavené jen odpory; měřič počítá stav článku | dva stavové piny nabíječky; měřič na I²C |
| článek | nejméně 15 min na nabití | — |
| nabíjecí vstup | pětipinový magnetický kontakt v krabičce, zapojený souměrně: napájení na obou krajních pinech, zem uprostřed, USB procesoru mezi nimi | — |
| magnety a světlo | zapnutí, vypnutí, stav článku, tovární nastavení; světlo dvoubarevné, červené a zelené | magnet A na spínač desky a na procesor, magnet B na procesor; světlo na pinech procesoru |

**Povodok** je krabička sama pro sebe, také utěsněná:

| blok | co dělá | s procesorem ho spojuje |
|---|---|---|
| rádio a procesor, jeden čip, jako u Belky | SYNC, sloty, příkazy, most k telefonu | — |
| Wi-Fi na 5 GHz | klient přístupového bodu telefonu, notebooku nebo budovy | sériová sběrnice |
| nabíječka, napájení a měřič | jako u Belky; kabel Povodok zapíná | dva stavové piny nabíječky; měřič na I²C |
| článek a vstup | jako u Belky: magnetický kontakt, nabíjení a USB | — |
| magnety a světlo | zapnutí, vypnutí, stav článku, start a stop | magnet A na spínač desky a na procesor, magnet B na procesor; světlo na pinech procesoru |

**Role.** Velí jen Povodoky, první z nich; každá Belka je tatáž deska. Jeden až čtyři Povodoky v
sadě, kvůli dosahu, každý posílá svůj SYNC z místa, kde stojí, a slyší Belky; druhý zapnutý se
připojí jako další SYNC. Povodok jezdí v ruce na kabelu USB; na stropě stáje nebo ordinace, napájený
ze sítě, po Wi-Fi; nebo na krku či hlavě zvířete, kde nahrává bez telefonu a spouští a zastavuje ho
jeho magnet B. Zapíná ho jeho kabel — zapojený, nebo když se zapne počítač na jeho druhém konci — a
pět minut po odpojení kabelu se sám vypne; vypne ho i nabídka aplikace, po zadání hesla. Belku
zapíná jen její magnet; její kabel ji jen nabíjí.

**Uvnitř sady: SYNCy Povodoků a sloty Belek.** Každý Povodok posílá svůj SYNC v pevném slotu, vždy
čtyři sloty, slot chybějícího Povodoku mlčí; SYNC nese číslo periody, živý náhled, zda záznam běží —
start a stop jsou tento stav, ne příkaz — a náhradní kanál. Každá Belka změří za periodu jeden ze 40
kanálů, sada pracuje na prvních 39 a 2480 MHz je vyhrazený hledání, a hlásí jeho šum; první Povodok
přesune sadu na náhradní kanál dřív, než přijdou ztráty, a přesun odpočítá přes dvě periody. Co
posílá první Povodok, ostatní opakují; když vypadne, převezme to další. Belka si bere čas z
kteréhokoli SYNCu, který slyší, a potřebuje ho jen čas od času, její hodiny mezitím drží. Každá
Belka má pevný slot podle svého čísla z přiřazení, všechny sloty stejně dlouhé: živý náhled a její
stav, posílané v každé periodě, ať SYNC slyšela, nebo ne. Žádná Belka nic nepřeposílá ani znovu
neposílá a co se zmešká, leží v paměti Belky. Deset sekund bez SYNCu zastaví záznam, nastavení v
Pušince, i vysílání Belky; ta dál poslouchá, a když zase přijde SYNC, že záznam běží, pokračuje ve
stejném sezení a mezera je vidět v číslech jejích bloků. **Vlastní rámec Strelky nese jen
drobnosti**, v šesti paketech se stejnou hlavičkou — SYNC, LIVE, MISS a PASS, jimiž ostatní Povodoky
předají prvnímu, co mu chybí, a CALL a HERE hledání nových jednotek; všechno ostatní, nastavení,
příkazy, stahování a firmware, jde vlastním protokolem výrobce: Nordic Enhanced ShockBurst, ESB, na
tomtéž rádiu, a na něm SMP, protokol MCUmgr ze Zephyru; Povodok je most mezi ním a USB nebo Wi-Fi.
**Nová Belka**, nebo Belka po továrním nastavení, poslouchá na kanálu a adrese pevně daných ve
firmwaru, stejných pro každou sadu; Povodok ji tam najde, zapíše jí po SMP její číslo, adresu, klíč
a kanál sady a ona se na ně přesune.

**K telefonu nebo počítači jen přes první Povodok:** kabel USB na magnetickém kontaktu, nebo Wi-Fi
na 5 GHz, kde se Povodok připojí k přístupovému bodu na kterémkoli kanálu, který ten používá; 2,4
GHz odmítá kvůli vlastnímu rádiu sady. Bluetooth LE jen v prvních minutách po zapnutí Povodoku, aby
přijal jméno a heslo sítě, pak je vypnuté. **Po obojím TCP/IP:** na kabelu je Povodok síťový adaptér
(CDC-NCM z USB), takže po obojím běží totéž a aplikace má jednu cestu; SMP po TCP pro příkazy,
stahování a firmware; živý náhled po UDP, zpráva za periodu s celou sadou, jeden LIVE od každé
Belky, sebraný prvním Povodokem z toho, co slyšel sám, a z PASSů ostatních; ztracená perioda zůstane
ztracená. Jedno spojení, jedna adresa: žádná hvězda Povodoků, žádný switch, a jakýkoli telefon.
Rozhraní je popsané po bajtech pro toho, kdo píše aplikaci, v [`PHONE.md`](PHONE.md) a
[`COMMANDS.md`](COMMANDS.md). Rádiová
pravidla jsou v [`REGULATIONS.md`](REGULATIONS.md).

**Zámek** chrání sadu před tím, aby ji ovládal cizí; data, pohyb zvířete, se neskrývají. Heslo
aplikace, nejméně osm znaků, se ověřuje výzvou a odpovědí, takže nikdy nejde kabelem ani vzduchem
čitelné; na Wi-Fi drží cizí stranou heslo samotné sítě. Mezi Povodokem a Belkami je každý paket SMP
podepsaný 128bitovým klíčem sady, značkou AES-CCM z hardwaru čipu, s počítadlem proti opakování, a
stejně tak každý SYNC, týmž klíčem a svou periodou: nikdo kromě sady nespustí ani nezastaví záznam,
nepřesune sadu na jiný kanál, nezmění nastavení ani nesmaže kartu. Ostatní pakety jdou otevřeně.
Klíč sady dostane nová Belka jednou, při přiřazení, zašifrovaný klíčem, na kterém se obě strany
nejdřív dohodnou přes ECDH, a to počítá CRACEN čipu. Firmware se spustí, jen když je podepsaný
výrobcem, ať má cizí jakýkoli klíč. Proti rušení žádný klíč nepomůže: sada se přesune z obsazeného
kanálu a co rádio zmešká, zůstane na kartách.

**Formát záznamu** je vlastní Strelky, bloky se svými časovými razítky, popsaný po bajtech, sezení
jako soubor na vlastním FAT32 nebo exFAT karty; Strelka dává převodníky do C3D, formátu laboratoří
chůze ([c3dr](https://docs.ropensci.org/c3dr/articles/c3dr.html)), a do CSV.

**Nabíjení.** Kabel z kteréhokoli portu USB, notebooku nebo nabíječky telefonu, k pětipinovému
magnetickému kontaktu v krabičce, u Belky i Povodoku stejnému: napájení na obou krajních pinech a
zem uprostřed, takže nabíjí při kterémkoli otočení, a mezi nimi USB procesoru — na počítači je
zapnutá Belka disk se svými záznamy a sériový port pro práci na stole a bere firmware přes zavaděč
bez otevírání krabičky.

**Na stole**, nebo ještě na zvířeti, vydá celá sada každý záznam po rádiu přes ESB a první Povodok,
Belku po Belce; když jsou ostatní Povodoky připojené po Wi-Fi jen pro stahování, jde každá Belka po
sekundách tomu Povodoku, který ji slyší nejlíp, telefon je rozděluje a další sekunda navazuje tam,
kde minulá skončila, takže Belku, která se vzdaluje od jednoho Povodoku, převezme jiný; nebo je
každá Belka na svém kabelu, zapnutá, disk se svými záznamy; nebo jde celá sada do nabíjecí stanice,
která ji nabije, a počítač zkopíruje disky všech Belek najednou: hodina záznamu při 100 Hz za
sekundy, při 1 kHz asi za minutu.

**Firmware** přichází také po rádiu, po SMP z aplikace přes Povodok, na stole; první se nahraje, než
se krabička zavře.

**Čas.** Každá Belka řídí hodiny svého senzoru, tik po tiku, až její vzorky padnou na mřížku
Povodoku; příchod SYNCu i každý vzorek dostanou razítko v hardwaru. Všechny jednotky vzorkují ve
stejných okamžicích, na mikrosekundy. **Sezení** čísluje Povodok a datuje telefon, datum a čas
svázané s číslem periody, obojí dané každé Belce s nastavením před startem, takže záznamy jednoho
vyšetření nesou stejné číslo a každý blok svůj čas; bez telefonu vlastní počítadlo Povodoku a
poslední čas, který mu telefon dal, nebo „neznámý“, a aplikace ho doplní při stahování.

**Záznam bez výpočtů.** Belka ukládá vzorky svého senzoru surové nebo sečtené a nic jiného: žádné
spektrum, žádnou míru. Jen pro živý náhled počítá své natočení, jako kvaterniony z akcelerometru a
gyroskopu, nebo posílá surové kroky, jak sezení řekne. Každá analýza je věc aplikace nebo počítače,
ze záznamu, takže každé použití — kulhající kůň, skákající pes, klepání stroje — si vezme metodu,
kterou potřebuje.

**Kde rádio selže** — velké tělo mezi Povodokem a Belkou — je slyšet jiný Povodok, z druhé strany.
Kde není žádný, má živý náhled mezery, hodiny drží svou poslední opravu a záznam je v Belce celý,
dokud ho deset sekund bez SYNCu nezastaví; znovu slyšený SYNC ho naváže ve stejném sezení.

## V ruce

Žádné tlačítko: dva magnety na značkách krabičky, A a B, a dvoubarevné světlo. Držení se vyhodnotí,
až když magnet odejde, podle toho, jak dlouho byl u značky, aby dlouhé držení nezapůsobilo dřív; na
každé jednotce stejně, natištěno na krabičce.

| uděláš | jednotka |
|---|---|
| magnet A na značku, držet, dokud se neukáže světlo, asi půl sekundy, na kabelu čtyři | se zapne; magnet, který jen projel kolem, nic neudělá, jednotka se ještě před světlem zase vypne |
| magnet A, krátké přiložení, jednotka zapnutá | ukáže článek: zelená čtyři bliknutí 76–100 %, oranžová tři 51–75 %, oranžová dvě 26–50 %, červená jedno 11–25 %, červená rychlé blikání 10 % a méně |
| magnet A, držet 6 s | se po puštění vypne: záznam uzavřený, světlo zhasne, jak magnet odchází |
| magnet B na Povodoku, držet 3 s, sada připravená | sada začne měřit; světlo zelené na 2 s |
| magnet B na Povodoku, držet 6 s, sada měří | sada přestane; světlo červené na 2 s |
| oba magnety, držet 10 s, jednotka zapnutá i vypnutá | tovární nastavení: světlo v 10 s oranžové na 2 s, a když oba odejdou, sada je zapomenuta, kalibrace také, záznamy zůstanou, jednotka se znovu spustí |

Na Belce magnet B sám nic nedělá; držení B pod 3 s nic nedělá, nad 10 s také ne. Aplikace umí totéž
svými příkazy: start a stop, tovární nastavení a bliknutí, aby se jednotka našla, oranžové a rychlé.
Co světlo říká: zelené pomalé blikání, v sadě, čeká; zelené rychlé blikání, měří; zelená a červená
střídavě, Povodok; červené blikání, slabý článek, pod 20 %; zelené blikání na nabíječi, nabíjí;
zelená stálá, nabito; oranžová stálá, chyba nabíječe; červená stálá, chyba; oranžové rychlé blikání,
hledaná aplikací ([`FIRMWARE.md`](FIRMWARE.md), *The magnets and the light*).

## Soubory

Stránky jsou anglicky.

| soubor | co |
|---|---|
| [`STATUS.md`](STATUS.md) | co je uzavřené, co otevřené, co řekne až postavený kus |
| [`REGULATIONS.md`](REGULATIONS.md) | rádiová pravidla České republiky a EU, citovaná z jejich textů |
| [`NAMES.md`](NAMES.md) | psi a co znamenají jména |
| [`PRINCIPLE.md`](PRINCIPLE.md) | fyzika každého bloku a hrubý výpočet |
| [`RECORD.md`](RECORD.md) | co Belka zapisuje na kartu, po bajtech |
| [`PARTS.md`](PARTS.md) | součástky, každá s požadavkem, který splňuje |
| [`FIRMWARE.md`](FIRMWARE.md) | co dělá firmware Belky a Povodoku, každý sám |
| [`APP.md`](APP.md) | co musí umět Pušinka, aplikace v telefonu |
| [`PHONE.md`](PHONE.md) | co si říká první Povodok s telefonem, po bajtech |
| [`COMMANDS.md`](COMMANDS.md) | co si říkají aplikace, Povodok a Belky přes SMP, po bajtech |
| [`BOARD.md`](BOARD.md) | deska Belky a Povodoku — schéma jako spoje, hodnoty, seznam součástek |
| [`board/`](board/) | součástky a jejich sítě vypsané jako seznam, `nets.svg`, skriptem `nets.py`, a jako netlist pro EasyEDA, `strelka.kicad_sch`, skriptem `schematic.py`; schéma se teprve nakreslí |
| [`calc/`](calc/) | skripty za každým číslem, jejich vstupy se zdroji v `params.py` a `check.py` |
| [`firmware/`](firmware/) | kód firmwaru: dva projekty nRF Connect SDK, Belka a Povodok, jejich společný kód a kód protokolu, kontrolovaný na hostiteli; oba se proti SDK sestaví, na hardwaru zatím nic neběželo |
| [`tools/`](tools/) | záznam sestavený, přečtený a vyvedený do CSV, `strelka_record.py`, a do C3D, `strelka_c3d.py`, s testy — reference k [`RECORD.md`](RECORD.md); společný kód firmwaru proti ní, `test_firmware.py`; Povodok simulovaný pro psaní aplikace, `povodok_sim.py`, a jeho klient `smp.py` |
| [`app/`](app/) | základ aplikace ve Flutteru: protokol v samotném Dartu, `strelka/`, držený na nástrojích a řídící simulátor; první obrazovky Pušinky, `pusinka/`, zanalyzované a otestované jako widgety, na žádném zařízení zatím nespuštěné |
| [`ANALYSIS.md`](ANALYSIS.md) | míry kulhání, první kostra podle publikovaných metod |
| [`analysis/`](analysis/) | kód kostry a jeho kontroly na syntetických klusech |
| [`WHY.md`](WHY.md) | co bylo zváženo a odloženo, a proč |
| [`CONTRIBUTING.md`](CONTRIBUTING.md) | jak se změna dostane dovnitř: forma stránek, kontroly, co pomůže nejvíc |

---

Software pod licencí MIT, viz [LICENSE](LICENSE); návrh hardwaru, stránky a obrázky pod CERN-OHL-P
v2, viz [LICENSE-HW](LICENSE-HW).

## Poděkování

AI asistentovi Claude (Anthropic) za velkou pomoc s tímto projektem.

★ Viva La Resistánce ★
