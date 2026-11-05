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

> **Beta 0.8:** návrh na papíře, desky popsané pro kreslení, každá jednotka zvlášť — Belka,
> [Kryska](KRYSKA.md), malá Belka, a Povodok — firmware sestavený proti SDK pro každou, střídání ESB
> podle Nordicu, aplikace začatá ve Flutteru; na hardwaru zatím nic neběželo. Nakreslené desky
> přijdou s verzí 0.9. Co je uzavřené, co otevřené a co řekne až postavený kus, je ve [stavu
> projektu](STATUS.md) (anglicky). Ostatní stránky jsou anglicky. Tagy ani vydání záměrně nejsou:
> repozitář drží jediný commit, poslední verzi, nahrazovaný, jak práce pokračuje.

> **Rádiová pravidla — přečtěte si je před zapnutím.** Strelka vysílá na 2,4 GHz a Povodok na Wi-Fi
> 5 GHz jako klient: přístupový bod, ke kterému se připojí, nastavte na svou zemi a na 5 GHz.
> Pravidla České republiky a EU, citovaná, jsou v [`REGULATIONS.md`](REGULATIONS.md); jinde platí ta
> národního úřadu.

---

## Co to je

Sada stejných jednotek, jedna až dvaatřicet, připevněných na zvíře suchým zipem, na popruhu nebo na
botě na kopyto. Každá nese pohybový senzor, rádio s procesorem, kartu na celé vyšetření a článek
nejméně na patnáct minut. Jeden až čtyři Povodoky kolem zvířete dávají sadě čas a předávají, co
měří, telefonu nebo počítači.

**Všechno na zvířeti:** žádná kamera, žádný chodník. **Jedna sada pro každé zvíře**, přizpůsobená
počtem jednotek. **Jen pohyb:** Strelka nesleduje, kam zvíře jde.

## Části

| část | co to je |
|---|---|
| [**Belka**](PARTS.md) | jednotka připevněná na zvířeti, se svým senzorem a kartou, na své základní desce, [Materince](BOARD.md), jejíž jádro sdílí každá jednotka; pro velké rázy, na kopytě, s LSM6DSV320X a pevnější krabičkou, Ugoljok; pro průmysl, se surovými daty ICM-42686-P až do 32 kHz, Zvjozdočka |
| [**Povodok**](NAMES.md) | velitel sady, jeden až čtyři v sadě kvůli dosahu: posílá čas, sbírá, co si telefon žádá, a předává to dál, po USB nebo Wi-Fi; jeho vlastní Materinka s modulem Wi-Fi, bez senzoru a karty |
| [**Kryska**](KRYSKA.md) | malá Belka, pro malá zvířata a běžná vyšetření: paměť na své desce, odolná už v základu |
| [**Pušinka**](APP.md) | aplikace v telefonu veterináře nebo v počítači; její základ v [`app/`](app/) |
| [**Stykovka**](NAMES.md) | cokoliv, k čemu se jednotka připojí nabíjet a přes co jde její USB dál: kabel, nebo stanice pro celou sadu s rozbočovačem USB, počítač zkopíruje všechny Belky najednou |
| **Korabl** | krabička jednotky, každé ve své velikosti: utěsněné pouzdro, okénko pro světlo, uchycení |
| **Semjorka** | kufr, který nese celou sadu |

Proč psi a co znamenají jména, je ve [jménech](NAMES.md) (anglicky).

## Jak to funguje

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

- **Jeden čas pro celou sadu.** Každý Povodok posílá svůj SYNC každých 20 ms; každá Belka řídí
  hodiny svého senzoru tik po tiku, až její vzorky padnou na mřížku sady, takže všechny jednotky
  vzorkují ve stejných okamžicích, na mikrosekundy.
- **Záznam celý v každé Belce**, na její vlastní kartě microSD, ve vlastním formátu Strelky popsaném
  po bajtech ([`RECORD.md`](RECORD.md)), s převodníky do C3D a CSV; co rádio zmešká, zůstane na
  kartě. Belka nepočítá nic kromě svého natočení pro živý náhled: každá analýza je věc aplikace
  nebo počítače.
- **Živý náhled** 50 nebo 100 Hz na jednotku, jako kvaterniony nebo surové kroky: sebraný prvním
  Povodokem a poslaný dál kabelem USB nebo po Wi-Fi, nebo každý Povodok v jedné síti Wi-Fi posílá,
  co slyší ([`PRINCIPLE.md`](PRINCIPLE.md)).
- **Zámek.** Heslo aplikace se ověřuje výzvou a odpovědí a nikdy nejde kabelem ani vzduchem; každý
  paket mezi Povodokem a Belkami i každý SYNC je podepsaný klíčem sady; firmware se spustí, jen když
  je podepsaný výrobcem.
- **Stahování** po rádiu; nebo každá Belka na svém kabelu jako disk, v sadě jen ke čtení; nebo celá
  sada ve Stykovce s rozbočovačem USB, počítač zkopíruje všechny disky najednou.
- **Žádné tlačítko:** dva magnety, A a B, a dvoubarevné světlo — zapnutí, vypnutí, stav článku,
  start a stop ([`FIRMWARE.md`](FIRMWARE.md), *The magnets and the light*).

Deska a součástky jsou v [`BOARD.md`](BOARD.md) a [`PARTS.md`](PARTS.md), firmware ve
[`FIRMWARE.md`](FIRMWARE.md), strana aplikace po bajtech v [`PHONE.md`](PHONE.md) a
[`COMMANDS.md`](COMMANDS.md) (anglicky).

## Co existuje

<details>
<summary>Systémy, které měří pohybující se zvíře inerciálními senzory na těle: levné sady, pro koně,
pro psy, pro krávy</summary>

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

</details>

**Jak víme, že to funguje:** na stejném zvířeti ve stejnou chvíli ukáže Strelka totéž, co systém
prodávaný veterinářům. U koně je to takový systém; u psa a krávy, kde se žádný neprodává, výzkumný
systém nebo změny, které uvádějí jeho publikované studie.

## Co musí umět

<details>
<summary>1 až 32 jednotek na jakémkoli zvířeti, asi 100 Hz do paměti a až několik kHz, živý náhled
50 nebo 100 Hz, jeden čas na mikrosekundy, nejméně 15 min na nabití</summary>

Rozsahy a řády; přesné hodnoty přijdou se součástkami.

| | požadavek |
|---|---|
| jednotek v sadě | 1 až 32 na jednom zvířeti; firmware jich unese 63; a jeden až čtyři Povodoky |
| zvířata | pes, kůň, kráva — tatáž sada |
| do paměti jednotky | asi 100 Hz, podle sezení až několik kHz |
| živě | 50 nebo 100 Hz na jednotku, natočení jako kvaterniony nebo surové kroky, podle sezení; co rádio zmešká, je v paměti jednotky |
| společný čas jednotek | hluboko pod jedním vzorkem: mikrosekundy |
| záznam | celé vyšetření, v každé jednotce; karta Belky ho neomezuje, 128 MB Krysky stačí asi na 5,5 h při 400 Hz |
| jedno nabití | nejméně 15 min |
| dosah | kolem těla zvířete, až čtyři Povodoky kolem něj, Belku stačí slyšet kterýkoli z nich; k telefonu nebo počítači kabelem USB nebo Wi-Fi na 5 GHz |
| velikost | krabička od sirek nebo větší |
| hmotnost | bez stanovené meze; vyplyne ze součástek |
| uchycení | na popruhu nebo na botě na kopyto, suchým zipem |
| podmínky | uvnitř i venku; voda, bláto a kopnutí; mezi zvířaty čištění dezinfekcí |
| obrazovka veterináře | telefon, Android nebo iPhone, nebo počítač |
| cena | jako levné sady nebo níž — asi 48 $ za jednotku, čtyři v sadě, před clem a poštovným — a v každém případě lepší |

</details>

## Soubory

<details>
<summary>Stránky, deska, firmware, nástroje a aplikace; stránky jsou anglicky</summary>

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
| [`BOARD.md`](BOARD.md) | Materinka, základní desky tří jednotek — schéma jako spoje, hodnoty, seznam součástek |
| [`board/`](board/) | součástky a sítě jako seznam a jako netlist pro EasyEDA; bloková schémata; skripty, které je kreslí |
| [`calc/`](calc/) | skripty za každým číslem, jejich vstupy se zdroji v `params.py` a `check.py` |
| [`firmware/`](firmware/) | dva projekty nRF Connect SDK, Belka a Povodok, a jejich společný kód, kontrolovaný na hostiteli |
| [`tools/`](tools/) | nástroje záznamu, do CSV a C3D; testy firmwaru; Černuška, simulovaný Povodok; soubor sady |
| [`app/`](app/) | základ aplikace ve Flutteru: protokol v Dartu a první obrazovky Pušinky |
| [`ANALYSIS.md`](ANALYSIS.md) | míry kulhání, první kostra podle publikovaných metod |
| [`analysis/`](analysis/) | kód kostry a jeho kontroly na syntetických klusech |
| [`WHY.md`](WHY.md) | co bylo zváženo a odloženo, a proč |
| [`CONTRIBUTING.md`](CONTRIBUTING.md) | jak se změna dostane dovnitř: forma stránek, kontroly, co pomůže nejvíc |

</details>

---

Software pod licencí MIT, viz [LICENSE](LICENSE); návrh hardwaru, stránky a obrázky pod CERN-OHL-P
v2, viz [LICENSE-HW](LICENSE-HW).

## Poděkování

AI asistentovi Claude (Anthropic) za velkou pomoc s tímto projektem.

★ Viva La Resistánce ★
