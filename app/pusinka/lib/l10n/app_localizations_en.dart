// ignore: unused_import
import 'package:intl/intl.dart' as intl;
import 'app_localizations.dart';

// ignore_for_file: type=lint

/// The translations for English (`en`).
class AppLocalizationsEn extends AppLocalizations {
  AppLocalizationsEn([String locale = 'en']) : super(locale);

  @override
  String get menuOverview => 'Overview';

  @override
  String get menuCalibration => 'Calibration';

  @override
  String get menuMeasurement => 'Measurement';

  @override
  String get menuData => 'Data';

  @override
  String get menuDiagnostics => 'Diagnostics';

  @override
  String get menuSettings => 'Settings';

  @override
  String get joinAddress => 'Povodok\'s address';

  @override
  String get joinPassword => 'Password';

  @override
  String get joinButton => 'Join';

  @override
  String get joinJoining => 'Joining…';

  @override
  String get joinRefused => 'Povodok refused the password.';

  @override
  String joinNoAnswer(String message) {
    return 'Povodok does not answer: $message';
  }

  @override
  String get joinTimeout => 'Povodok does not answer in time.';

  @override
  String get settingsLanguage => 'Language';

  @override
  String get settingsAbout => 'About';

  @override
  String get aboutPicture => 'Pušinka, Strelka\'s puppy, drawn.';

  @override
  String get aboutOpen => 'Open, for anyone to build; nothing is sold.';

  @override
  String get overviewUnits => 'Units of the set';

  @override
  String get overviewNone => 'No unit heard yet.';

  @override
  String measSession(int s) {
    return 'Session $s';
  }

  @override
  String get measChoose => 'Units: choose by a touch';

  @override
  String get measNote => 'Note: the place, the animal, what happened';

  @override
  String get measNone => 'No unit of the set heard yet.';

  @override
  String measChosen(int chosen, int all) {
    return 'Chosen $chosen of $all';
  }

  @override
  String get measRate => 'Rate';

  @override
  String get measAcc => 'Accelerometer';

  @override
  String get measGyro => 'Gyro';

  @override
  String get measHg => 'High-g channel';

  @override
  String get measHgOff => 'off';

  @override
  String get measLive => 'Live view';

  @override
  String get liveOff => 'off';

  @override
  String get liveQuat50 => 'quaternions 50 Hz';

  @override
  String get liveQuat100 => 'quaternions 100 Hz';

  @override
  String get liveRaw50 => 'raw steps 50 Hz';

  @override
  String get liveRaw100 => 'raw steps 100 Hz';

  @override
  String get measSyncFirst => 'Synchro: the session not sent yet';

  @override
  String measInStep(int n, int all) {
    return 'Synchro: $n of $all chosen in step';
  }

  @override
  String get measSync => 'Synchro';

  @override
  String get measStart => 'START';

  @override
  String get measLiveOnly => 'Live';

  @override
  String get measNoCal => 'no calibration';

  @override
  String get measExpired => 'calibration expired';

  @override
  String get measUnreachable => 'out of reach';

  @override
  String measTempApart(int deg) {
    return '$deg °C from its calibration';
  }

  @override
  String get setMeasurement => 'Measurement and calibration';

  @override
  String get setCalValid => 'the calibration valid';

  @override
  String get calEvery => 'every measurement';

  @override
  String get cal2h => '2 h';

  @override
  String get calDay => 'a day';

  @override
  String get cal30 => '30 days';

  @override
  String get calUnlimited => 'without a limit';

  @override
  String get setTempApart => 'warned of a temperature apart by';

  @override
  String get setCurves => 'the curves\' window';

  @override
  String get setNumbers => 'the numbers over the last';

  @override
  String strides(int n) {
    return '$n strides';
  }

  @override
  String get setLost => 'The connection to Povodok is lost';

  @override
  String get setPower => 'the radio\'s power';

  @override
  String get powerFull => 'full, within the rules';

  @override
  String get setLight => 'the light while measuring';

  @override
  String get lightUsual => 'as usual';

  @override
  String get lightFaults => 'faults alone';

  @override
  String get lightOff => 'off';

  @override
  String get setToCome => 'sent to the units once their firmware has it';

  @override
  String get setLocked => 'Changed outside the measurement alone.';

  @override
  String get dataPhone => 'On the phone';

  @override
  String get dataUnits => 'On the units';

  @override
  String get dataNoFiles => 'Nothing on the phone yet.';

  @override
  String get dataNoSessions => 'No session on the units.';

  @override
  String get dataGot => 'on the phone ✓';

  @override
  String dataSwitchOn(String units) {
    return 'switch on $units';
  }

  @override
  String get dataLarge => 'large: dock or cable';

  @override
  String get dataReady => 'ready';

  @override
  String get dataToCsv => 'To CSV';

  @override
  String get dataToC3d => 'To C3D';

  @override
  String dataAsk(int n) {
    return 'Download it from $n unit(s) into one C3D?';
  }

  @override
  String get dataCancel => 'Cancel';

  @override
  String get dataDownload => 'Download';

  @override
  String get ovPovodoks => 'Povodoks';

  @override
  String get ovToPhone => 'to the phone';

  @override
  String get ovHeard => 'heard';

  @override
  String get ovNotHeard => 'not heard';

  @override
  String get ovCode => 'code';

  @override
  String get ovUnit => 'unit';

  @override
  String get ovState => 'state';

  @override
  String get ovCell => 'cell';

  @override
  String get ovCard => 'card';

  @override
  String get ovHeardAgo => 'heard';

  @override
  String get ovFault => 'fault';

  @override
  String get ovFlash => 'flash';

  @override
  String get stRecording => 'recording';

  @override
  String get stAlone => 'recording alone';

  @override
  String get stSearching => 'recording, seeking';

  @override
  String get stPaused => 'paused';

  @override
  String get stWaiting => 'waiting';

  @override
  String get stCalibrating => 'calibrating';

  @override
  String get stGiving => 'giving its record';

  @override
  String get stFirmware => 'taking firmware';

  @override
  String get stErasing => 'erasing';

  @override
  String get fNoCard => 'no card';

  @override
  String get fCardFull => 'card full';

  @override
  String get fWrite => 'card write failed';

  @override
  String get fFs => 'file system unreadable';

  @override
  String get fDropped => 'blocks dropped';

  @override
  String get fSensor => 'sensor silent';

  @override
  String get fClock => 'clock on its fallback';

  @override
  String get fCellLow => 'cell low';

  @override
  String get fCharger => 'charger fault';

  @override
  String get fPmic => 'PMIC silent';

  @override
  String get fNoCal => 'no calibration';

  @override
  String get fRefused => 'settings refused';

  @override
  String get uKind => 'kind, number';

  @override
  String get uChip => 'chip';

  @override
  String get uSensor => 'sensor';

  @override
  String get uFirmware => 'firmware';

  @override
  String get uTemp => 'temperature';

  @override
  String get uCal => 'calibration';

  @override
  String get uWith => 'With it';

  @override
  String get uBlink => 'Blink 10 s';

  @override
  String get uErase => 'Erase the card… (password)';

  @override
  String get uForget => 'Forget… (password)';

  @override
  String get uOff => 'Switch off';

  @override
  String get uPassword => 'The password again';

  @override
  String get uForgotten =>
      'Forgotten, the unit leaves the set and keeps its records.';

  @override
  String get uDone => 'Done.';

  @override
  String get uNotNow => 'Not now: while the set records, or Povodok at work.';

  @override
  String calOnto(String face) {
    return 'Every unit onto the face $face, then touch Confirm.';
  }

  @override
  String calTurn(String axis, String face) {
    return 'Turn every unit about its $axis axis onto the face $face, then touch Confirm.';
  }

  @override
  String get calConfirm => 'Confirm';

  @override
  String get calMeasuring => 'Hold still: 5 s…';

  @override
  String get calTaken => 'still ✓';

  @override
  String get calMoved => 'moved ✗, again';

  @override
  String get calOtherFace => 'another face ✗, again';

  @override
  String get calDone => 'Done: every unit keeps its new calibration.';

  @override
  String calAge(int days) {
    String _temp0 = intl.Intl.pluralLogic(
      days,
      locale: localeName,
      other: '$days days',
      one: '1 day',
      zero: 'today',
    );
    return '$_temp0';
  }

  @override
  String get dlStarting => 'Starting…';

  @override
  String dlDone(String path) {
    return 'Done: $path';
  }

  @override
  String dlOf(int got, int all) {
    return '$got of $all kB';
  }

  @override
  String dlLeft(int s) {
    return '$s s left';
  }

  @override
  String get dlLeftOut => 'left out: not reached, or its file missing';

  @override
  String get actStop => 'Stop';

  @override
  String get liveSave => 'Save';

  @override
  String get liveSaveStop => 'Stop saving';

  @override
  String liveSaving(Object time, Object mb) {
    return 'saving $time · $mb MB';
  }

  @override
  String get actContinue => 'Continue';

  @override
  String get setToWork => 'To work';

  @override
  String get setNoLive => 'No live view yet';

  @override
  String get diagBand => 'The band: noise by channel, dBm';

  @override
  String diagSet(Object ch) {
    return 'the set $ch';
  }

  @override
  String diagSpare(Object ch) {
    return 'spare $ch';
  }

  @override
  String diagMargin(Object k, Object db) {
    return 'Povodok $k: margin $db dB';
  }

  @override
  String get diagUnits => 'Units';

  @override
  String diagHeard(Object s) {
    return 'heard $s s ago';
  }

  @override
  String get diagNotHeard => 'not heard';

  @override
  String diagSync(Object dbm, Object missed) {
    return 'SYNC $dbm dBm, $missed missed';
  }

  @override
  String diagClock(Object ns) {
    return 'clock $ns ns';
  }

  @override
  String diagOut(Object acc, Object gyro) {
    return 'out of range $acc/$gyro';
  }

  @override
  String diagCell(Object pct) {
    return 'cell $pct %';
  }

  @override
  String diagCard(Object free, Object dropped) {
    return 'card $free % free, $dropped dropped';
  }

  @override
  String get liveCurves => 'Curves';

  @override
  String get liveNumbers => 'Numbers';

  @override
  String get liveAccel => 'vertical acceleration, g';

  @override
  String get liveTilt => 'tilt from the vertical, °';

  @override
  String get liveRange => 'range';

  @override
  String get liveStrides => 'strides';

  @override
  String liveLastStrides(Object n) {
    return 'the last $n strides';
  }

  @override
  String get liveNoRhythm => 'no stride yet';

  @override
  String get liveLeftRight => 'Left against right';

  @override
  String get liveDifference => 'difference';

  @override
  String get placesTitle => 'Places\' templates';

  @override
  String get placesTemplate => 'Template';

  @override
  String get placesAnimal => 'The animal\'s name';

  @override
  String get placesNote =>
      'The code shows the unit everywhere; the set\'s table goes with the session into every unit\'s file.';

  @override
  String get animalHorse => 'horse';

  @override
  String get animalDog => 'dog';

  @override
  String get animalPerson => 'person';

  @override
  String get placeHead => 'head';

  @override
  String get placeWithers => 'withers';

  @override
  String get placeThoracic => 'thoracic spine';

  @override
  String get placeLumbar => 'lumbar spine';

  @override
  String get placePelvis => 'pelvis';

  @override
  String get placeLeftFore => 'left fore';

  @override
  String get placeRightFore => 'right fore';

  @override
  String get placeLeftHind => 'left hind';

  @override
  String get placeRightHind => 'right hind';

  @override
  String get placeLeftShoulder => 'left shoulder';

  @override
  String get placeRightShoulder => 'right shoulder';

  @override
  String get placeLeftElbow => 'left elbow';

  @override
  String get placeRightElbow => 'right elbow';

  @override
  String get placeLeftHip => 'left hip';

  @override
  String get placeRightHip => 'right hip';

  @override
  String get animalCow => 'cow';

  @override
  String get animalCat => 'cat';

  @override
  String get setNewUnits => 'New units';

  @override
  String get fwTitle => 'Firmware';

  @override
  String fwReady(String version) {
    return 'Firmware $version on the phone, signed and checked';
  }

  @override
  String get fwNone => 'No firmware on the phone yet.';

  @override
  String get fwFetch => 'Fetch from GitHub';

  @override
  String fwFetched(String version) {
    return 'Fetched: $version';
  }

  @override
  String get fwNotPublished => 'No firmware published yet.';

  @override
  String get fwUpdateAll => 'Update all';

  @override
  String get fwWaiting => 'waiting';

  @override
  String get fwTaking => 'taking it';

  @override
  String get fwDone => 'done ✓';

  @override
  String get fwCurrent => 'up to date ✓';

  @override
  String get fwOff => 'off: later';

  @override
  String get fwFailed => 'failed, again';

  @override
  String get fwHow =>
      'By SMP through Povodok, one unit at a time, Povodok last; each unit checks the image\'s signature and keeps the old one should the new not start; a unit off takes it when it is on.';

  @override
  String get fwPovodokAgain =>
      'Povodok starts again with the new firmware: join it again.';

  @override
  String get setNewSet => 'A new set';

  @override
  String get nsIntro =>
      'A new set — its address, key and channel — given to Povodok, which starts again in it. The units then come in by the search, a Belka also by the file on its cable.';

  @override
  String get nsChannel => 'Channel';

  @override
  String get nsNotMeasured => 'not measured';

  @override
  String get nsQuietest => 'the quietest';

  @override
  String get nsLabel => 'The set\'s own number, 1–9999, if any';

  @override
  String get nsDraw => 'Draw a new set';

  @override
  String nsDrawn(String address, Object ch) {
    return 'The set: $address, channel $ch';
  }

  @override
  String get nsPovodok => 'This Povodok\'s number in the set';

  @override
  String get nsPassword => 'Povodok\'s password, again';

  @override
  String get nsGive => 'Give it to this Povodok';

  @override
  String get nsAsk =>
      'Povodok leaves its set for the new one and starts again; the units of the old set stay out until they are given the new one.';

  @override
  String get nsGiven =>
      'Povodok takes the new set and starts again: join it again, then search for the units.';

  @override
  String get nsFileFor => 'STRELKA.SET for the Belka numbered';

  @override
  String get nsSave => 'Save';

  @override
  String nsSaved(String path) {
    return 'Saved: $path — copied onto the Belka\'s disk on its cable, it is taken when the cable leaves.';
  }

  @override
  String get newTitle => 'New units: the search';

  @override
  String get newSearching => 'Povodok calls; a new unit answers in its window.';

  @override
  String get newNone => 'No new unit heard yet.';

  @override
  String newTake(Object n) {
    return 'Take as $n';
  }

  @override
  String newTaken(Object n) {
    return 'taken as $n';
  }

  @override
  String get newCable =>
      'A Belka on its cable takes the set faster, the file STRELKA.SET on its disk.';

  @override
  String get pwTitle => 'Password';

  @override
  String get pwOld => 'The password now';

  @override
  String get pwNew => 'The new password, 12 characters at least';

  @override
  String get pwAgain => 'The new password again';

  @override
  String get pwShort => 'At least 12 characters.';

  @override
  String get pwMismatch => 'The two do not match.';

  @override
  String get pwChange => 'Change';

  @override
  String get pwChanged => 'The password changed: join again with it.';

  @override
  String get offTitle => 'Switch Povodok off';

  @override
  String get offAsk => 'Switch Povodok off? Recording, it stops the set first.';

  @override
  String keyNew(Object fp) {
    return 'Povodok\'s key kept: $fp';
  }

  @override
  String get keyChangedTitle => 'Another key';

  @override
  String keyChanged(Object now, Object old) {
    return 'Povodok answers with another key ($now) than the one kept ($old). After its factory settings that is right; otherwise someone else answers. Take the new key?';
  }

  @override
  String get keyTake => 'Take it';

  @override
  String get keyFingerprint => 'Povodok\'s key';

  @override
  String get pwFactory =>
      'The factory password: give a new one, 12 characters at least.';

  @override
  String diagMove(Object ch) {
    return 'Move the set to $ch';
  }
}
