// ignore: unused_import
import 'package:intl/intl.dart' as intl;
import 'app_localizations.dart';

// ignore_for_file: type=lint

/// The translations for Czech (`cs`).
class AppLocalizationsCs extends AppLocalizations {
  AppLocalizationsCs([String locale = 'cs']) : super(locale);

  @override
  String get menuOverview => 'Přehled';

  @override
  String get menuCalibration => 'Kalibrace';

  @override
  String get menuMeasurement => 'Měření';

  @override
  String get menuData => 'Data';

  @override
  String get menuDiagnostics => 'Diagnostika';

  @override
  String get menuSettings => 'Nastavení';

  @override
  String get joinAddress => 'Adresa Povodoku';

  @override
  String get joinPassword => 'Heslo';

  @override
  String get joinButton => 'Připojit';

  @override
  String get joinJoining => 'Připojuji…';

  @override
  String get joinRefused => 'Povodok heslo odmítl.';

  @override
  String joinNoAnswer(String message) {
    return 'Povodok neodpovídá: $message';
  }

  @override
  String get joinTimeout => 'Povodok neodpověděl včas.';

  @override
  String get settingsLanguage => 'Jazyk';

  @override
  String get settingsAbout => 'O programu';

  @override
  String get aboutPicture => 'Pušinka, Strelčino štěně, kreslená.';

  @override
  String get aboutOpen =>
      'Otevřené, pro každého k postavení; nic se neprodává.';

  @override
  String get overviewUnits => 'Jednotky sady';

  @override
  String get overviewNone => 'Zatím není slyšet žádná jednotka.';

  @override
  String measSession(int s) {
    return 'Sezení $s';
  }

  @override
  String get measChoose => 'Jednotky: vyber ťuknutím';

  @override
  String get measNote => 'Poznámka: místo, zvíře, co se stalo';

  @override
  String get measNone => 'Zatím není slyšet žádná jednotka sady.';

  @override
  String measChosen(int chosen, int all) {
    return 'Vybráno $chosen z $all';
  }

  @override
  String get measRate => 'Frekvence';

  @override
  String get measAcc => 'Akcelerometr';

  @override
  String get measGyro => 'Gyroskop';

  @override
  String get measHg => 'Kanál velkého g';

  @override
  String get measHgOff => 'vypnutý';

  @override
  String get measLive => 'Živý pohled';

  @override
  String get liveOff => 'vypnutý';

  @override
  String get liveQuat50 => 'kvaterniony 50 Hz';

  @override
  String get liveQuat100 => 'kvaterniony 100 Hz';

  @override
  String get liveRaw50 => 'surové kroky 50 Hz';

  @override
  String get liveRaw100 => 'surové kroky 100 Hz';

  @override
  String get measSyncFirst => 'Synchro: sezení ještě neodešlo';

  @override
  String measInStep(int n, int all) {
    return 'Synchro: $n z $all vybraných v synchru';
  }

  @override
  String get measSync => 'Synchro';

  @override
  String get measStart => 'START';

  @override
  String get measLiveOnly => 'Live';

  @override
  String get measNoCal => 'bez kalibrace';

  @override
  String get measExpired => 'kalibrace propadlá';

  @override
  String get measUnreachable => 'mimo dosah';

  @override
  String measTempApart(int deg) {
    return '$deg °C od kalibrace';
  }

  @override
  String get setMeasurement => 'Měření a kalibrace';

  @override
  String get setCalValid => 'platnost kalibrace';

  @override
  String get calEvery => 'každé měření';

  @override
  String get cal2h => '2 h';

  @override
  String get calDay => 'den';

  @override
  String get cal30 => '30 dní';

  @override
  String get calUnlimited => 'bez limitu';

  @override
  String get setTempApart => 'varovat při teplotě jiné o';

  @override
  String get setCurves => 'okno křivek';

  @override
  String get setNumbers => 'čísla za posledních';

  @override
  String strides(int n) {
    return '$n kroků';
  }

  @override
  String get setLost => 'Spojení s Povodokem je ztracené';

  @override
  String get setPower => 'výkon vysílače';

  @override
  String get powerFull => 'plný, v mezích pravidel';

  @override
  String get setLight => 'světlo při měření';

  @override
  String get lightUsual => 'normálně';

  @override
  String get lightFaults => 'jen poruchy';

  @override
  String get lightOff => 'vypnuté';

  @override
  String get setToCome => 'jednotkám se pošle, až to jejich firmware bude umět';

  @override
  String get setLocked => 'Mění se jen mimo měření.';

  @override
  String get dataPhone => 'V telefonu';

  @override
  String get dataUnits => 'Na jednotkách';

  @override
  String get dataNoFiles => 'V telefonu zatím nic není.';

  @override
  String get dataNoSessions => 'Na jednotkách není žádné sezení.';

  @override
  String get dataGot => 'v telefonu ✓';

  @override
  String dataSwitchOn(String units) {
    return 'zapni $units';
  }

  @override
  String get dataLarge => 'velké: dok nebo kabel';

  @override
  String get dataReady => 'připraveno';

  @override
  String get dataToCsv => 'Do CSV';

  @override
  String get dataToC3d => 'Do C3D';

  @override
  String dataAsk(int n) {
    return 'Stáhnout z $n jednotek do jednoho C3D?';
  }

  @override
  String get dataCancel => 'Zrušit';

  @override
  String get dataDownload => 'Stáhnout';

  @override
  String get ovPovodoks => 'Povodoky';

  @override
  String get ovToPhone => 'k telefonu';

  @override
  String get ovHeard => 'slyšet';

  @override
  String get ovNotHeard => 'neslyšet';

  @override
  String get ovCode => 'kód';

  @override
  String get ovUnit => 'jednotka';

  @override
  String get ovState => 'stav';

  @override
  String get ovCell => 'baterie';

  @override
  String get ovCard => 'karta';

  @override
  String get ovHeardAgo => 'slyšet';

  @override
  String get ovFault => 'porucha';

  @override
  String get ovFlash => 'flash';

  @override
  String get stRecording => 'nahrává';

  @override
  String get stAlone => 'nahrává sama';

  @override
  String get stSearching => 'nahrává, hledá';

  @override
  String get stPaused => 'pauza';

  @override
  String get stWaiting => 'čeká';

  @override
  String get stCalibrating => 'kalibruje';

  @override
  String get stGiving => 'dává záznam';

  @override
  String get stFirmware => 'přijímá firmware';

  @override
  String get stErasing => 'maže';

  @override
  String get fNoCard => 'chybí karta';

  @override
  String get fCardFull => 'karta plná';

  @override
  String get fWrite => 'zápis na kartu selhal';

  @override
  String get fFs => 'souborový systém nečitelný';

  @override
  String get fDropped => 'zahozené bloky';

  @override
  String get fSensor => 'snímač mlčí';

  @override
  String get fClock => 'hodiny na záloze';

  @override
  String get fCellLow => 'slabá baterie';

  @override
  String get fCharger => 'porucha nabíječky';

  @override
  String get fPmic => 'PMIC mlčí';

  @override
  String get fNoCal => 'bez kalibrace';

  @override
  String get fRefused => 'nastavení odmítnuto';

  @override
  String get uKind => 'druh, číslo';

  @override
  String get uChip => 'čip';

  @override
  String get uSensor => 'snímač';

  @override
  String get uFirmware => 'firmware';

  @override
  String get uTemp => 'teplota';

  @override
  String get uCal => 'kalibrace';

  @override
  String get uWith => 'S jednotkou';

  @override
  String get uBlink => 'Bliknout 10 s';

  @override
  String get uErase => 'Smazat kartu… (heslo)';

  @override
  String get uForget => 'Zapomenout… (heslo)';

  @override
  String get uOff => 'Vypnout';

  @override
  String get uPassword => 'Heslo znovu';

  @override
  String get uForgotten =>
      'Zapomenutá jednotka opustí sadu a záznamy si nechá.';

  @override
  String get uDone => 'Hotovo.';

  @override
  String get uNotNow => 'Teď ne: sada nahrává, nebo je Povodok v práci.';

  @override
  String calOnto(String face) {
    return 'Všechny jednotky na stranu $face, pak ťukni Potvrdit.';
  }

  @override
  String calTurn(String axis, String face) {
    return 'Otoč všechny jednotky kolem osy $axis na stranu $face, pak ťukni Potvrdit.';
  }

  @override
  String get calConfirm => 'Potvrdit';

  @override
  String get calMeasuring => 'Nehýbat: 5 s…';

  @override
  String get calTaken => 'v klidu ✓';

  @override
  String get calMoved => 'pohnula se ✗, znovu';

  @override
  String get calOtherFace => 'jiná strana ✗, znovu';

  @override
  String get calDone => 'Hotovo: každá jednotka má novou kalibraci.';

  @override
  String calAge(int days) {
    String _temp0 = intl.Intl.pluralLogic(
      days,
      locale: localeName,
      other: '$days dní',
      few: '$days dny',
      one: '1 den',
      zero: 'dnes',
    );
    return '$_temp0';
  }

  @override
  String get dlStarting => 'Začíná se…';

  @override
  String dlDone(String path) {
    return 'Hotovo: $path';
  }

  @override
  String dlOf(int got, int all) {
    return '$got z $all kB';
  }

  @override
  String dlLeft(int s) {
    return 'zbývá $s s';
  }

  @override
  String get dlLeftOut => 'vynechána: nedostupná, nebo chybí její soubor';

  @override
  String get actStop => 'Zastavit';

  @override
  String get liveSave => 'Uložit';

  @override
  String get liveSaveStop => 'Zastavit ukládání';

  @override
  String liveSaving(Object time, Object mb) {
    return 'ukládá se $time · $mb MB';
  }

  @override
  String get actContinue => 'Pokračovat';

  @override
  String get setToWork => 'Do práce';

  @override
  String get setNoLive => 'Zatím žádný živý pohled';

  @override
  String get diagBand => 'Pásmo: šum po kanálech, dBm';

  @override
  String diagSet(Object ch) {
    return 'sada $ch';
  }

  @override
  String diagSpare(Object ch) {
    return 'náhradní $ch';
  }

  @override
  String diagMargin(Object k, Object db) {
    return 'Povodok $k: rezerva $db dB';
  }

  @override
  String get diagUnits => 'Jednotky';

  @override
  String diagHeard(Object s) {
    return 'slyšena před $s s';
  }

  @override
  String get diagNotHeard => 'neslyšena';

  @override
  String diagSync(Object dbm, Object missed) {
    return 'SYNC $dbm dBm, $missed vypadlo';
  }

  @override
  String diagClock(Object ns) {
    return 'hodiny $ns ns';
  }

  @override
  String diagOut(Object acc, Object gyro) {
    return 'mimo rozsah $acc/$gyro';
  }

  @override
  String diagCell(Object pct) {
    return 'baterie $pct %';
  }

  @override
  String diagCard(Object free, Object dropped) {
    return 'karta $free % volno, $dropped zahozeno';
  }

  @override
  String get liveCurves => 'Křivky';

  @override
  String get liveNumbers => 'Čísla';

  @override
  String get liveAccel => 'svislé zrychlení, g';

  @override
  String get liveTilt => 'náklon od svislice, °';

  @override
  String get liveRange => 'rozsah';

  @override
  String get liveStrides => 'kroky';

  @override
  String liveLastStrides(Object n) {
    return 'posledních $n kroků';
  }

  @override
  String get liveNoRhythm => 'zatím žádný krok';

  @override
  String get liveLeftRight => 'Levá proti pravé';

  @override
  String get liveDifference => 'rozdíl';

  @override
  String get placesTitle => 'Šablony míst';

  @override
  String get placesTemplate => 'Šablona';

  @override
  String get placesAnimal => 'Jméno zvířete';

  @override
  String get placesNote =>
      'Kód ukazuje jednotku všude; tabulka sady jde se sezením do souboru každé jednotky.';

  @override
  String get animalHorse => 'kůň';

  @override
  String get animalDog => 'pes';

  @override
  String get animalPerson => 'člověk';

  @override
  String get placeHead => 'hlava';

  @override
  String get placeWithers => 'kohoutek';

  @override
  String get placeThoracic => 'hrudní páteř';

  @override
  String get placeLumbar => 'bederní páteř';

  @override
  String get placePelvis => 'pánev';

  @override
  String get placeLeftFore => 'levá přední';

  @override
  String get placeRightFore => 'pravá přední';

  @override
  String get placeLeftHind => 'levá zadní';

  @override
  String get placeRightHind => 'pravá zadní';

  @override
  String get placeLeftShoulder => 'levé rameno';

  @override
  String get placeRightShoulder => 'pravé rameno';

  @override
  String get placeLeftElbow => 'levý loket';

  @override
  String get placeRightElbow => 'pravý loket';

  @override
  String get placeLeftHip => 'levá kyčel';

  @override
  String get placeRightHip => 'pravá kyčel';

  @override
  String get animalCow => 'kráva';

  @override
  String get animalCat => 'kočka';

  @override
  String get setNewUnits => 'Nové jednotky';

  @override
  String get fwTitle => 'Firmware';

  @override
  String fwReady(String version) {
    return 'Firmware $version v telefonu, podepsaný a zkontrolovaný';
  }

  @override
  String get fwNone => 'V telefonu zatím žádný firmware.';

  @override
  String get fwFetch => 'Stáhnout z GitHubu';

  @override
  String fwFetched(String version) {
    return 'Staženo: $version';
  }

  @override
  String get fwNotPublished => 'Zatím není zveřejněný žádný firmware.';

  @override
  String get fwUpdateAll => 'Aktualizovat vše';

  @override
  String get fwWaiting => 'čeká';

  @override
  String get fwTaking => 'přijímá';

  @override
  String get fwDone => 'hotovo ✓';

  @override
  String get fwCurrent => 'aktuální ✓';

  @override
  String get fwOff => 'vypnutá: později';

  @override
  String get fwFailed => 'nepovedlo se, znovu';

  @override
  String get fwHow =>
      'Přes SMP skrze Povodok, jedna jednotka po druhé, Povodok poslední; každá jednotka ověří podpis obrazu a nechá si starý, kdyby nový nenaběhl; vypnutá ho dostane, až bude zapnutá.';

  @override
  String get fwPovodokAgain =>
      'Povodok startuje znovu s novým firmwarem: připoj se k němu znovu.';

  @override
  String get setNewSet => 'Nová sada';

  @override
  String get nsIntro =>
      'Nová sada — její adresa, klíč a kanál — předaná Povodoku, který v ní začne znovu. Jednotky pak přibudou hledáním, Belka také souborem na kabelu.';

  @override
  String get nsChannel => 'Kanál';

  @override
  String get nsNotMeasured => 'neměřen';

  @override
  String get nsQuietest => 'nejtišší';

  @override
  String get nsLabel => 'Vlastní číslo sady, 1–9999, je-li';

  @override
  String get nsDraw => 'Vylosovat novou sadu';

  @override
  String nsDrawn(String address, Object ch) {
    return 'Sada: $address, kanál $ch';
  }

  @override
  String get nsPovodok => 'Číslo tohoto Povodoku v sadě';

  @override
  String get nsPassword => 'Heslo Povodoku, znovu';

  @override
  String get nsGive => 'Předat tomuto Povodoku';

  @override
  String get nsAsk =>
      'Povodok opustí svou sadu pro novou a začne znovu; jednotky staré sady zůstanou mimo, dokud nedostanou novou.';

  @override
  String get nsGiven =>
      'Povodok přijal novou sadu a začíná znovu: připoj se k němu znovu a pak hledej jednotky.';

  @override
  String get nsFileFor => 'STRELKA.SET pro Belku číslo';

  @override
  String get nsSave => 'Uložit';

  @override
  String nsSaved(String path) {
    return 'Uloženo: $path — zkopírovaný na disk Belky na kabelu se převezme, když kabel odpojíš.';
  }

  @override
  String get newTitle => 'Nové jednotky: hledání';

  @override
  String get newSearching => 'Povodok volá; nová jednotka odpoví ve svém okně.';

  @override
  String get newNone => 'Zatím žádná nová jednotka.';

  @override
  String newTake(Object n) {
    return 'Přijmout jako $n';
  }

  @override
  String newTaken(Object n) {
    return 'přijata jako $n';
  }

  @override
  String get newCable =>
      'Belka na kabelu dostane sadu rychleji, soubor STRELKA.SET na jejím disku.';

  @override
  String get pwTitle => 'Heslo';

  @override
  String get pwOld => 'Současné heslo';

  @override
  String get pwNew => 'Nové heslo, aspoň 12 znaků';

  @override
  String get pwAgain => 'Nové heslo znovu';

  @override
  String get pwShort => 'Aspoň 12 znaků.';

  @override
  String get pwMismatch => 'Hesla se neshodují.';

  @override
  String get pwChange => 'Změnit';

  @override
  String get pwChanged => 'Heslo změněno: připoj se znovu s novým.';

  @override
  String get offTitle => 'Vypnout Povodok';

  @override
  String get offAsk =>
      'Vypnout Povodok? Když se nahrává, nejdřív zastaví sadu.';

  @override
  String keyNew(Object fp) {
    return 'Klíč Povodoku uložen: $fp';
  }

  @override
  String get keyChangedTitle => 'Jiný klíč';

  @override
  String keyChanged(Object now, Object old) {
    return 'Povodok odpovídá jiným klíčem ($now) než uloženým ($old). Po jeho továrním nastavení je to v pořádku; jinak odpovídá někdo jiný. Vzít nový klíč?';
  }

  @override
  String get keyTake => 'Vzít';

  @override
  String get keyFingerprint => 'Klíč Povodoku';

  @override
  String get pwFactory => 'Tovární heslo: zadej nové, aspoň 12 znaků.';

  @override
  String diagMove(Object ch) {
    return 'Přesunout sadu na $ch';
  }
}
