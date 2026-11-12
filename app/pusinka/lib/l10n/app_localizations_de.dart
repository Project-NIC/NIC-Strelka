// ignore: unused_import
import 'package:intl/intl.dart' as intl;
import 'app_localizations.dart';

// ignore_for_file: type=lint

/// The translations for German (`de`).
class AppLocalizationsDe extends AppLocalizations {
  AppLocalizationsDe([String locale = 'de']) : super(locale);

  @override
  String get menuOverview => 'Übersicht';

  @override
  String get menuCalibration => 'Kalibrierung';

  @override
  String get menuMeasurement => 'Messung';

  @override
  String get menuData => 'Daten';

  @override
  String get menuDiagnostics => 'Diagnose';

  @override
  String get menuSettings => 'Einstellungen';

  @override
  String get joinAddress => 'Adresse des Povodok';

  @override
  String get joinPassword => 'Passwort';

  @override
  String get joinButton => 'Verbinden';

  @override
  String get joinJoining => 'Verbinde…';

  @override
  String get joinRefused => 'Der Povodok hat das Passwort abgelehnt.';

  @override
  String joinNoAnswer(String message) {
    return 'Der Povodok antwortet nicht: $message';
  }

  @override
  String get joinTimeout => 'Der Povodok hat nicht rechtzeitig geantwortet.';

  @override
  String get settingsLanguage => 'Sprache';

  @override
  String get settingsAbout => 'Über';

  @override
  String get aboutPicture => 'Pušinka, Strelkas Welpe, als Zeichnung.';

  @override
  String get aboutOpen =>
      'Offen, für jeden zum Nachbauen; nichts wird verkauft.';

  @override
  String get overviewUnits => 'Einheiten des Sets';

  @override
  String get overviewNone => 'Noch keine Einheit gehört.';

  @override
  String measSession(int s) {
    return 'Sitzung $s';
  }

  @override
  String get measChoose => 'Einheiten: durch Antippen wählen';

  @override
  String get measNote => 'Notiz: Ort, Tier, was geschah';

  @override
  String get measNone => 'Noch keine Einheit des Sets gehört.';

  @override
  String measChosen(int chosen, int all) {
    return 'Gewählt $chosen von $all';
  }

  @override
  String get measRate => 'Rate';

  @override
  String get measAcc => 'Beschleunigungssensor';

  @override
  String get measGyro => 'Gyroskop';

  @override
  String get measHg => 'High-g-Kanal';

  @override
  String get measHgOff => 'aus';

  @override
  String get measLive => 'Live-Ansicht';

  @override
  String get liveOff => 'aus';

  @override
  String get liveQuat50 => 'Quaternionen 50 Hz';

  @override
  String get liveQuat100 => 'Quaternionen 100 Hz';

  @override
  String get liveRaw50 => 'Rohschritte 50 Hz';

  @override
  String get liveRaw100 => 'Rohschritte 100 Hz';

  @override
  String get measSyncFirst => 'Synchro: die Sitzung noch nicht gesendet';

  @override
  String measInStep(int n, int all) {
    return 'Synchro: $n von $all gewählten im Takt';
  }

  @override
  String get measSync => 'Synchro';

  @override
  String get measStart => 'START';

  @override
  String get measLiveOnly => 'Live';

  @override
  String get measNoCal => 'keine Kalibrierung';

  @override
  String get measExpired => 'Kalibrierung abgelaufen';

  @override
  String get measUnreachable => 'außer Reichweite';

  @override
  String measTempApart(int deg) {
    return '$deg °C von der Kalibrierung';
  }

  @override
  String get setMeasurement => 'Messung und Kalibrierung';

  @override
  String get setCalValid => 'Kalibrierung gültig';

  @override
  String get calEvery => 'jede Messung';

  @override
  String get cal2h => '2 h';

  @override
  String get calDay => 'ein Tag';

  @override
  String get cal30 => '30 Tage';

  @override
  String get calUnlimited => 'ohne Grenze';

  @override
  String get setTempApart => 'warnen bei einer Temperatur abweichend um';

  @override
  String get setCurves => 'Fenster der Kurven';

  @override
  String get setNumbers => 'die Zahlen der letzten';

  @override
  String strides(int n) {
    return '$n Schritte';
  }

  @override
  String get setLost => 'Die Verbindung zu Povodok ist verloren';

  @override
  String get setPower => 'Sendeleistung';

  @override
  String get powerFull => 'voll, im Rahmen der Regeln';

  @override
  String get setLight => 'Licht beim Messen';

  @override
  String get lightUsual => 'wie üblich';

  @override
  String get lightFaults => 'nur Fehler';

  @override
  String get lightOff => 'aus';

  @override
  String get setToCome =>
      'an die Einheiten gesendet, sobald ihre Firmware es kann';

  @override
  String get setLocked => 'Nur außerhalb der Messung zu ändern.';

  @override
  String get dataPhone => 'Auf dem Telefon';

  @override
  String get dataUnits => 'Auf den Einheiten';

  @override
  String get dataNoFiles => 'Noch nichts auf dem Telefon.';

  @override
  String get dataNoSessions => 'Keine Sitzung auf den Einheiten.';

  @override
  String get dataGot => 'auf dem Telefon ✓';

  @override
  String dataSwitchOn(String units) {
    return '$units einschalten';
  }

  @override
  String get dataLarge => 'groß: Dock oder Kabel';

  @override
  String get dataReady => 'bereit';

  @override
  String get dataToCsv => 'Als CSV';

  @override
  String get dataToC3d => 'Als C3D';

  @override
  String dataAsk(int n) {
    return 'Von $n Einheit(en) in ein C3D herunterladen?';
  }

  @override
  String get dataCancel => 'Abbrechen';

  @override
  String get dataDownload => 'Herunterladen';

  @override
  String get ovPovodoks => 'Povodoks';

  @override
  String get ovToPhone => 'zum Telefon';

  @override
  String get ovHeard => 'gehört';

  @override
  String get ovNotHeard => 'nicht gehört';

  @override
  String get ovCode => 'Code';

  @override
  String get ovUnit => 'Einheit';

  @override
  String get ovState => 'Zustand';

  @override
  String get ovCell => 'Akku';

  @override
  String get ovCard => 'Karte';

  @override
  String get ovHeardAgo => 'gehört';

  @override
  String get ovFault => 'Fehler';

  @override
  String get ovFlash => 'Flash';

  @override
  String get stRecording => 'nimmt auf';

  @override
  String get stAlone => 'nimmt allein auf';

  @override
  String get stSearching => 'nimmt auf, sucht';

  @override
  String get stPaused => 'Pause';

  @override
  String get stWaiting => 'wartet';

  @override
  String get stCalibrating => 'kalibriert';

  @override
  String get stGiving => 'gibt ihre Aufnahme';

  @override
  String get stFirmware => 'erhält Firmware';

  @override
  String get stErasing => 'löscht';

  @override
  String get fNoCard => 'keine Karte';

  @override
  String get fCardFull => 'Karte voll';

  @override
  String get fWrite => 'Schreiben fehlgeschlagen';

  @override
  String get fFs => 'Dateisystem unlesbar';

  @override
  String get fDropped => 'Blöcke verloren';

  @override
  String get fSensor => 'Sensor stumm';

  @override
  String get fClock => 'Uhr auf Reserve';

  @override
  String get fCellLow => 'Akku schwach';

  @override
  String get fCharger => 'Ladegerätfehler';

  @override
  String get fPmic => 'PMIC stumm';

  @override
  String get fNoCal => 'keine Kalibrierung';

  @override
  String get fRefused => 'Einstellungen abgelehnt';

  @override
  String get uKind => 'Art, Nummer';

  @override
  String get uChip => 'Chip';

  @override
  String get uSensor => 'Sensor';

  @override
  String get uFirmware => 'Firmware';

  @override
  String get uTemp => 'Temperatur';

  @override
  String get uCal => 'Kalibrierung';

  @override
  String get uWith => 'Mit ihr';

  @override
  String get uBlink => '10 s blinken';

  @override
  String get uErase => 'Karte löschen… (Passwort)';

  @override
  String get uForget => 'Vergessen… (Passwort)';

  @override
  String get uOff => 'Ausschalten';

  @override
  String get uPassword => 'Das Passwort noch einmal';

  @override
  String get uForgotten =>
      'Vergessen verlässt die Einheit das Set und behält ihre Aufnahmen.';

  @override
  String get uDone => 'Erledigt.';

  @override
  String get uNotNow =>
      'Nicht jetzt: das Set nimmt auf, oder der Povodok ist im Betrieb.';

  @override
  String calOnto(String face) {
    return 'Alle Einheiten auf die Seite $face, dann Bestätigen antippen.';
  }

  @override
  String calTurn(String axis, String face) {
    return 'Alle Einheiten um ihre $axis-Achse auf die Seite $face drehen, dann Bestätigen antippen.';
  }

  @override
  String get calConfirm => 'Bestätigen';

  @override
  String get calMeasuring => 'Stillhalten: 5 s…';

  @override
  String get calTaken => 'ruhig ✓';

  @override
  String get calMoved => 'bewegt ✗, nochmal';

  @override
  String get calOtherFace => 'andere Seite ✗, nochmal';

  @override
  String get calDone => 'Fertig: Jede Einheit behält ihre neue Kalibrierung.';

  @override
  String calAge(int days) {
    String _temp0 = intl.Intl.pluralLogic(
      days,
      locale: localeName,
      other: '$days Tage',
      one: '1 Tag',
      zero: 'heute',
    );
    return '$_temp0';
  }

  @override
  String get dlStarting => 'Startet…';

  @override
  String dlDone(String path) {
    return 'Fertig: $path';
  }

  @override
  String dlOf(int got, int all) {
    return '$got von $all kB';
  }

  @override
  String dlLeft(int s) {
    return 'noch $s s';
  }

  @override
  String get dlLeftOut => 'ausgelassen: nicht erreicht, oder ihre Datei fehlt';

  @override
  String get actStop => 'Stopp';

  @override
  String get liveSave => 'Speichern';

  @override
  String get liveSaveStop => 'Speichern beenden';

  @override
  String liveSaving(Object time, Object mb) {
    return 'speichert $time · $mb MB';
  }

  @override
  String get actContinue => 'Weiter';

  @override
  String get setToWork => 'An die Arbeit';

  @override
  String get setNoLive => 'Noch keine Live-Ansicht';

  @override
  String get diagBand => 'Das Band: Rauschen je Kanal, dBm';

  @override
  String diagSet(Object ch) {
    return 'Set $ch';
  }

  @override
  String diagSpare(Object ch) {
    return 'Reserve $ch';
  }

  @override
  String diagMargin(Object k, Object db) {
    return 'Povodok $k: Abstand $db dB';
  }

  @override
  String get diagUnits => 'Einheiten';

  @override
  String diagHeard(Object s) {
    return 'gehört vor $s s';
  }

  @override
  String get diagNotHeard => 'nicht gehört';

  @override
  String diagSync(Object dbm, Object missed) {
    return 'SYNC $dbm dBm, $missed verpasst';
  }

  @override
  String diagClock(Object ns) {
    return 'Uhr $ns ns';
  }

  @override
  String diagOut(Object acc, Object gyro) {
    return 'außerhalb des Bereichs $acc/$gyro';
  }

  @override
  String diagCell(Object pct) {
    return 'Akku $pct %';
  }

  @override
  String diagCard(Object free, Object dropped) {
    return 'Karte $free % frei, $dropped verworfen';
  }

  @override
  String get liveCurves => 'Kurven';

  @override
  String get liveNumbers => 'Zahlen';

  @override
  String get liveAccel => 'vertikale Beschleunigung, g';

  @override
  String get liveTilt => 'Neigung zur Senkrechten, °';

  @override
  String get liveRange => 'Spanne';

  @override
  String get liveStrides => 'Schritte';

  @override
  String liveLastStrides(Object n) {
    return 'die letzten $n Schritte';
  }

  @override
  String get liveNoRhythm => 'noch kein Schritt';

  @override
  String get liveLeftRight => 'Links gegen rechts';

  @override
  String get liveDifference => 'Unterschied';

  @override
  String get placesTitle => 'Vorlagen der Orte';

  @override
  String get placesTemplate => 'Vorlage';

  @override
  String get placesAnimal => 'Name des Tieres';

  @override
  String get placesNote =>
      'Der Code zeigt die Einheit überall; die Tabelle des Sets geht mit der Sitzung in die Datei jeder Einheit.';

  @override
  String get animalHorse => 'Pferd';

  @override
  String get animalDog => 'Hund';

  @override
  String get animalPerson => 'Mensch';

  @override
  String get placeHead => 'Kopf';

  @override
  String get placeWithers => 'Widerrist';

  @override
  String get placeThoracic => 'Brustwirbelsäule';

  @override
  String get placeLumbar => 'Lendenwirbelsäule';

  @override
  String get placePelvis => 'Becken';

  @override
  String get placeLeftFore => 'linkes Vorderbein';

  @override
  String get placeRightFore => 'rechtes Vorderbein';

  @override
  String get placeLeftHind => 'linkes Hinterbein';

  @override
  String get placeRightHind => 'rechtes Hinterbein';

  @override
  String get placeLeftShoulder => 'linke Schulter';

  @override
  String get placeRightShoulder => 'rechte Schulter';

  @override
  String get placeLeftElbow => 'linker Ellbogen';

  @override
  String get placeRightElbow => 'rechter Ellbogen';

  @override
  String get placeLeftHip => 'linke Hüfte';

  @override
  String get placeRightHip => 'rechte Hüfte';

  @override
  String get animalCow => 'Kuh';

  @override
  String get animalCat => 'Katze';

  @override
  String get setNewUnits => 'Neue Einheiten';

  @override
  String get fwTitle => 'Firmware';

  @override
  String fwReady(String version) {
    return 'Firmware $version auf dem Telefon, signiert und geprüft';
  }

  @override
  String get fwNone => 'Noch keine Firmware auf dem Telefon.';

  @override
  String get fwFetch => 'Von GitHub holen';

  @override
  String fwFetched(String version) {
    return 'Geholt: $version';
  }

  @override
  String get fwNotPublished => 'Noch keine Firmware veröffentlicht.';

  @override
  String get fwUpdateAll => 'Alle aktualisieren';

  @override
  String get fwWaiting => 'wartet';

  @override
  String get fwTaking => 'nimmt auf';

  @override
  String get fwDone => 'fertig ✓';

  @override
  String get fwCurrent => 'aktuell ✓';

  @override
  String get fwOff => 'aus: später';

  @override
  String get fwFailed => 'fehlgeschlagen, erneut';

  @override
  String get fwHow =>
      'Per SMP über den Povodok, eine Einheit nach der anderen, der Povodok zuletzt; jede Einheit prüft die Signatur des Abbilds und behält das alte, sollte das neue nicht starten; eine ausgeschaltete erhält es, wenn sie an ist.';

  @override
  String get fwPovodokAgain =>
      'Der Povodok startet mit der neuen Firmware neu: erneut verbinden.';

  @override
  String get setNewSet => 'Ein neues Set';

  @override
  String get nsIntro =>
      'Ein neues Set — seine Adresse, sein Schlüssel und sein Kanal — dem Povodok gegeben, der darin neu beginnt. Die Einheiten kommen dann durch die Suche hinzu, eine Belka auch durch die Datei am Kabel.';

  @override
  String get nsChannel => 'Kanal';

  @override
  String get nsNotMeasured => 'nicht gemessen';

  @override
  String get nsQuietest => 'der leiseste';

  @override
  String get nsLabel => 'Die eigene Nummer des Sets, 1–9999, falls eine';

  @override
  String get nsDraw => 'Ein neues Set auslosen';

  @override
  String nsDrawn(String address, Object ch) {
    return 'Das Set: $address, Kanal $ch';
  }

  @override
  String get nsPovodok => 'Die Nummer dieses Povodok im Set';

  @override
  String get nsPassword => 'Das Passwort des Povodok, erneut';

  @override
  String get nsGive => 'Diesem Povodok geben';

  @override
  String get nsAsk =>
      'Der Povodok verlässt sein Set für das neue und beginnt neu; die Einheiten des alten Sets bleiben draußen, bis sie das neue bekommen.';

  @override
  String get nsGiven =>
      'Der Povodok übernimmt das neue Set und beginnt neu: erneut verbinden, dann nach den Einheiten suchen.';

  @override
  String get nsFileFor => 'STRELKA.SET für die Belka Nummer';

  @override
  String get nsSave => 'Speichern';

  @override
  String nsSaved(String path) {
    return 'Gespeichert: $path — auf die Platte der Belka am Kabel kopiert, wird sie übernommen, wenn das Kabel geht.';
  }

  @override
  String get newTitle => 'Neue Einheiten: die Suche';

  @override
  String get newSearching =>
      'Povodok ruft; eine neue Einheit antwortet in ihrem Fenster.';

  @override
  String get newNone => 'Noch keine neue Einheit gehört.';

  @override
  String newTake(Object n) {
    return 'Aufnehmen als $n';
  }

  @override
  String newTaken(Object n) {
    return 'aufgenommen als $n';
  }

  @override
  String get newCable =>
      'Eine Belka am Kabel bekommt das Set schneller, die Datei STRELKA.SET auf ihrem Laufwerk.';

  @override
  String get pwTitle => 'Passwort';

  @override
  String get pwOld => 'Das jetzige Passwort';

  @override
  String get pwNew => 'Das neue Passwort, mindestens 12 Zeichen';

  @override
  String get pwAgain => 'Das neue Passwort noch einmal';

  @override
  String get pwShort => 'Mindestens 12 Zeichen.';

  @override
  String get pwMismatch => 'Die beiden stimmen nicht überein.';

  @override
  String get pwChange => 'Ändern';

  @override
  String get pwChanged => 'Passwort geändert: mit dem neuen erneut verbinden.';

  @override
  String get offTitle => 'Povodok ausschalten';

  @override
  String get offAsk =>
      'Povodok ausschalten? Bei einer Aufnahme stoppt es zuerst das Set.';

  @override
  String keyNew(Object fp) {
    return 'Povodoks Schlüssel gespeichert: $fp';
  }

  @override
  String get keyChangedTitle => 'Ein anderer Schlüssel';

  @override
  String keyChanged(Object now, Object old) {
    return 'Povodok antwortet mit einem anderen Schlüssel ($now) als dem gespeicherten ($old). Nach seinen Werkseinstellungen ist das richtig; sonst antwortet jemand anderes. Den neuen Schlüssel nehmen?';
  }

  @override
  String get keyTake => 'Nehmen';

  @override
  String get keyFingerprint => 'Povodoks Schlüssel';

  @override
  String get pwFactory =>
      'Das Werkspasswort: ein neues eingeben, mindestens 12 Zeichen.';

  @override
  String diagMove(Object ch) {
    return 'Das Set auf $ch verlegen';
  }
}
