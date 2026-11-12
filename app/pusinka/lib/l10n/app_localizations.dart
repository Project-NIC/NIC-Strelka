import 'dart:async';

import 'package:flutter/foundation.dart';
import 'package:flutter/widgets.dart';
import 'package:flutter_localizations/flutter_localizations.dart';
import 'package:intl/intl.dart' as intl;

import 'app_localizations_cs.dart';
import 'app_localizations_de.dart';
import 'app_localizations_en.dart';
import 'app_localizations_es.dart';
import 'app_localizations_fr.dart';
import 'app_localizations_pt.dart';
import 'app_localizations_ru.dart';

// ignore_for_file: type=lint

/// Callers can lookup localized strings with an instance of AppLocalizations
/// returned by `AppLocalizations.of(context)`.
///
/// Applications need to include `AppLocalizations.delegate()` in their app's
/// `localizationDelegates` list, and the locales they support in the app's
/// `supportedLocales` list. For example:
///
/// ```dart
/// import 'l10n/app_localizations.dart';
///
/// return MaterialApp(
///   localizationsDelegates: AppLocalizations.localizationsDelegates,
///   supportedLocales: AppLocalizations.supportedLocales,
///   home: MyApplicationHome(),
/// );
/// ```
///
/// ## Update pubspec.yaml
///
/// Please make sure to update your pubspec.yaml to include the following
/// packages:
///
/// ```yaml
/// dependencies:
///   # Internationalization support.
///   flutter_localizations:
///     sdk: flutter
///   intl: any # Use the pinned version from flutter_localizations
///
///   # Rest of dependencies
/// ```
///
/// ## iOS Applications
///
/// iOS applications define key application metadata, including supported
/// locales, in an Info.plist file that is built into the application bundle.
/// To configure the locales supported by your app, you’ll need to edit this
/// file.
///
/// First, open your project’s ios/Runner.xcworkspace Xcode workspace file.
/// Then, in the Project Navigator, open the Info.plist file under the Runner
/// project’s Runner folder.
///
/// Next, select the Information Property List item, select Add Item from the
/// Editor menu, then select Localizations from the pop-up menu.
///
/// Select and expand the newly-created Localizations item then, for each
/// locale your application supports, add a new item and select the locale
/// you wish to add from the pop-up menu in the Value field. This list should
/// be consistent with the languages listed in the AppLocalizations.supportedLocales
/// property.
abstract class AppLocalizations {
  AppLocalizations(String locale)
    : localeName = intl.Intl.canonicalizedLocale(locale.toString());

  final String localeName;

  static AppLocalizations of(BuildContext context) {
    return Localizations.of<AppLocalizations>(context, AppLocalizations)!;
  }

  static const LocalizationsDelegate<AppLocalizations> delegate =
      _AppLocalizationsDelegate();

  /// A list of this localizations delegate along with the default localizations
  /// delegates.
  ///
  /// Returns a list of localizations delegates containing this delegate along with
  /// GlobalMaterialLocalizations.delegate, GlobalCupertinoLocalizations.delegate,
  /// and GlobalWidgetsLocalizations.delegate.
  ///
  /// Additional delegates can be added by appending to this list in
  /// MaterialApp. This list does not have to be used at all if a custom list
  /// of delegates is preferred or required.
  static const List<LocalizationsDelegate<dynamic>> localizationsDelegates =
      <LocalizationsDelegate<dynamic>>[
        delegate,
        GlobalMaterialLocalizations.delegate,
        GlobalCupertinoLocalizations.delegate,
        GlobalWidgetsLocalizations.delegate,
      ];

  /// A list of this localizations delegate's supported locales.
  static const List<Locale> supportedLocales = <Locale>[
    Locale('cs'),
    Locale('de'),
    Locale('en'),
    Locale('es'),
    Locale('fr'),
    Locale('pt'),
    Locale('ru'),
  ];

  /// No description provided for @menuOverview.
  ///
  /// In en, this message translates to:
  /// **'Overview'**
  String get menuOverview;

  /// No description provided for @menuCalibration.
  ///
  /// In en, this message translates to:
  /// **'Calibration'**
  String get menuCalibration;

  /// No description provided for @menuMeasurement.
  ///
  /// In en, this message translates to:
  /// **'Measurement'**
  String get menuMeasurement;

  /// No description provided for @menuData.
  ///
  /// In en, this message translates to:
  /// **'Data'**
  String get menuData;

  /// No description provided for @menuDiagnostics.
  ///
  /// In en, this message translates to:
  /// **'Diagnostics'**
  String get menuDiagnostics;

  /// No description provided for @menuSettings.
  ///
  /// In en, this message translates to:
  /// **'Settings'**
  String get menuSettings;

  /// No description provided for @joinAddress.
  ///
  /// In en, this message translates to:
  /// **'Povodok\'s address'**
  String get joinAddress;

  /// No description provided for @joinPassword.
  ///
  /// In en, this message translates to:
  /// **'Password'**
  String get joinPassword;

  /// No description provided for @joinButton.
  ///
  /// In en, this message translates to:
  /// **'Join'**
  String get joinButton;

  /// No description provided for @joinJoining.
  ///
  /// In en, this message translates to:
  /// **'Joining…'**
  String get joinJoining;

  /// No description provided for @joinRefused.
  ///
  /// In en, this message translates to:
  /// **'Povodok refused the password.'**
  String get joinRefused;

  /// No description provided for @joinNoAnswer.
  ///
  /// In en, this message translates to:
  /// **'Povodok does not answer: {message}'**
  String joinNoAnswer(String message);

  /// No description provided for @joinTimeout.
  ///
  /// In en, this message translates to:
  /// **'Povodok does not answer in time.'**
  String get joinTimeout;

  /// No description provided for @settingsLanguage.
  ///
  /// In en, this message translates to:
  /// **'Language'**
  String get settingsLanguage;

  /// No description provided for @settingsAbout.
  ///
  /// In en, this message translates to:
  /// **'About'**
  String get settingsAbout;

  /// No description provided for @aboutPicture.
  ///
  /// In en, this message translates to:
  /// **'Pušinka, Strelka\'s puppy, drawn.'**
  String get aboutPicture;

  /// No description provided for @aboutOpen.
  ///
  /// In en, this message translates to:
  /// **'Open, for anyone to build; nothing is sold.'**
  String get aboutOpen;

  /// No description provided for @overviewUnits.
  ///
  /// In en, this message translates to:
  /// **'Units of the set'**
  String get overviewUnits;

  /// No description provided for @overviewNone.
  ///
  /// In en, this message translates to:
  /// **'No unit heard yet.'**
  String get overviewNone;

  /// No description provided for @measSession.
  ///
  /// In en, this message translates to:
  /// **'Session {s}'**
  String measSession(int s);

  /// No description provided for @measChoose.
  ///
  /// In en, this message translates to:
  /// **'Units: choose by a touch'**
  String get measChoose;

  /// No description provided for @measNote.
  ///
  /// In en, this message translates to:
  /// **'Note: the place, the animal, what happened'**
  String get measNote;

  /// No description provided for @measNone.
  ///
  /// In en, this message translates to:
  /// **'No unit of the set heard yet.'**
  String get measNone;

  /// No description provided for @measChosen.
  ///
  /// In en, this message translates to:
  /// **'Chosen {chosen} of {all}'**
  String measChosen(int chosen, int all);

  /// No description provided for @measRate.
  ///
  /// In en, this message translates to:
  /// **'Rate'**
  String get measRate;

  /// No description provided for @measAcc.
  ///
  /// In en, this message translates to:
  /// **'Accelerometer'**
  String get measAcc;

  /// No description provided for @measGyro.
  ///
  /// In en, this message translates to:
  /// **'Gyro'**
  String get measGyro;

  /// No description provided for @measHg.
  ///
  /// In en, this message translates to:
  /// **'High-g channel'**
  String get measHg;

  /// No description provided for @measHgOff.
  ///
  /// In en, this message translates to:
  /// **'off'**
  String get measHgOff;

  /// No description provided for @measLive.
  ///
  /// In en, this message translates to:
  /// **'Live view'**
  String get measLive;

  /// No description provided for @liveOff.
  ///
  /// In en, this message translates to:
  /// **'off'**
  String get liveOff;

  /// No description provided for @liveQuat50.
  ///
  /// In en, this message translates to:
  /// **'quaternions 50 Hz'**
  String get liveQuat50;

  /// No description provided for @liveQuat100.
  ///
  /// In en, this message translates to:
  /// **'quaternions 100 Hz'**
  String get liveQuat100;

  /// No description provided for @liveRaw50.
  ///
  /// In en, this message translates to:
  /// **'raw steps 50 Hz'**
  String get liveRaw50;

  /// No description provided for @liveRaw100.
  ///
  /// In en, this message translates to:
  /// **'raw steps 100 Hz'**
  String get liveRaw100;

  /// No description provided for @measSyncFirst.
  ///
  /// In en, this message translates to:
  /// **'Synchro: the session not sent yet'**
  String get measSyncFirst;

  /// No description provided for @measInStep.
  ///
  /// In en, this message translates to:
  /// **'Synchro: {n} of {all} chosen in step'**
  String measInStep(int n, int all);

  /// No description provided for @measSync.
  ///
  /// In en, this message translates to:
  /// **'Synchro'**
  String get measSync;

  /// No description provided for @measStart.
  ///
  /// In en, this message translates to:
  /// **'START'**
  String get measStart;

  /// No description provided for @measLiveOnly.
  ///
  /// In en, this message translates to:
  /// **'Live'**
  String get measLiveOnly;

  /// No description provided for @measNoCal.
  ///
  /// In en, this message translates to:
  /// **'no calibration'**
  String get measNoCal;

  /// No description provided for @measExpired.
  ///
  /// In en, this message translates to:
  /// **'calibration expired'**
  String get measExpired;

  /// No description provided for @measUnreachable.
  ///
  /// In en, this message translates to:
  /// **'out of reach'**
  String get measUnreachable;

  /// No description provided for @measTempApart.
  ///
  /// In en, this message translates to:
  /// **'{deg} °C from its calibration'**
  String measTempApart(int deg);

  /// No description provided for @setMeasurement.
  ///
  /// In en, this message translates to:
  /// **'Measurement and calibration'**
  String get setMeasurement;

  /// No description provided for @setCalValid.
  ///
  /// In en, this message translates to:
  /// **'the calibration valid'**
  String get setCalValid;

  /// No description provided for @calEvery.
  ///
  /// In en, this message translates to:
  /// **'every measurement'**
  String get calEvery;

  /// No description provided for @cal2h.
  ///
  /// In en, this message translates to:
  /// **'2 h'**
  String get cal2h;

  /// No description provided for @calDay.
  ///
  /// In en, this message translates to:
  /// **'a day'**
  String get calDay;

  /// No description provided for @cal30.
  ///
  /// In en, this message translates to:
  /// **'30 days'**
  String get cal30;

  /// No description provided for @calUnlimited.
  ///
  /// In en, this message translates to:
  /// **'without a limit'**
  String get calUnlimited;

  /// No description provided for @setTempApart.
  ///
  /// In en, this message translates to:
  /// **'warned of a temperature apart by'**
  String get setTempApart;

  /// No description provided for @setCurves.
  ///
  /// In en, this message translates to:
  /// **'the curves\' window'**
  String get setCurves;

  /// No description provided for @setNumbers.
  ///
  /// In en, this message translates to:
  /// **'the numbers over the last'**
  String get setNumbers;

  /// No description provided for @strides.
  ///
  /// In en, this message translates to:
  /// **'{n} strides'**
  String strides(int n);

  /// No description provided for @setLost.
  ///
  /// In en, this message translates to:
  /// **'The connection to Povodok is lost'**
  String get setLost;

  /// No description provided for @setPower.
  ///
  /// In en, this message translates to:
  /// **'the radio\'s power'**
  String get setPower;

  /// No description provided for @powerFull.
  ///
  /// In en, this message translates to:
  /// **'full, within the rules'**
  String get powerFull;

  /// No description provided for @setLight.
  ///
  /// In en, this message translates to:
  /// **'the light while measuring'**
  String get setLight;

  /// No description provided for @lightUsual.
  ///
  /// In en, this message translates to:
  /// **'as usual'**
  String get lightUsual;

  /// No description provided for @lightFaults.
  ///
  /// In en, this message translates to:
  /// **'faults alone'**
  String get lightFaults;

  /// No description provided for @lightOff.
  ///
  /// In en, this message translates to:
  /// **'off'**
  String get lightOff;

  /// No description provided for @setToCome.
  ///
  /// In en, this message translates to:
  /// **'sent to the units once their firmware has it'**
  String get setToCome;

  /// No description provided for @setLocked.
  ///
  /// In en, this message translates to:
  /// **'Changed outside the measurement alone.'**
  String get setLocked;

  /// No description provided for @dataPhone.
  ///
  /// In en, this message translates to:
  /// **'On the phone'**
  String get dataPhone;

  /// No description provided for @dataUnits.
  ///
  /// In en, this message translates to:
  /// **'On the units'**
  String get dataUnits;

  /// No description provided for @dataNoFiles.
  ///
  /// In en, this message translates to:
  /// **'Nothing on the phone yet.'**
  String get dataNoFiles;

  /// No description provided for @dataNoSessions.
  ///
  /// In en, this message translates to:
  /// **'No session on the units.'**
  String get dataNoSessions;

  /// No description provided for @dataGot.
  ///
  /// In en, this message translates to:
  /// **'on the phone ✓'**
  String get dataGot;

  /// No description provided for @dataSwitchOn.
  ///
  /// In en, this message translates to:
  /// **'switch on {units}'**
  String dataSwitchOn(String units);

  /// No description provided for @dataLarge.
  ///
  /// In en, this message translates to:
  /// **'large: dock or cable'**
  String get dataLarge;

  /// No description provided for @dataReady.
  ///
  /// In en, this message translates to:
  /// **'ready'**
  String get dataReady;

  /// No description provided for @dataToCsv.
  ///
  /// In en, this message translates to:
  /// **'To CSV'**
  String get dataToCsv;

  /// No description provided for @dataToC3d.
  ///
  /// In en, this message translates to:
  /// **'To C3D'**
  String get dataToC3d;

  /// No description provided for @dataAsk.
  ///
  /// In en, this message translates to:
  /// **'Download it from {n} unit(s) into one C3D?'**
  String dataAsk(int n);

  /// No description provided for @dataCancel.
  ///
  /// In en, this message translates to:
  /// **'Cancel'**
  String get dataCancel;

  /// No description provided for @dataDownload.
  ///
  /// In en, this message translates to:
  /// **'Download'**
  String get dataDownload;

  /// No description provided for @ovPovodoks.
  ///
  /// In en, this message translates to:
  /// **'Povodoks'**
  String get ovPovodoks;

  /// No description provided for @ovToPhone.
  ///
  /// In en, this message translates to:
  /// **'to the phone'**
  String get ovToPhone;

  /// No description provided for @ovHeard.
  ///
  /// In en, this message translates to:
  /// **'heard'**
  String get ovHeard;

  /// No description provided for @ovNotHeard.
  ///
  /// In en, this message translates to:
  /// **'not heard'**
  String get ovNotHeard;

  /// No description provided for @ovCode.
  ///
  /// In en, this message translates to:
  /// **'code'**
  String get ovCode;

  /// No description provided for @ovUnit.
  ///
  /// In en, this message translates to:
  /// **'unit'**
  String get ovUnit;

  /// No description provided for @ovState.
  ///
  /// In en, this message translates to:
  /// **'state'**
  String get ovState;

  /// No description provided for @ovCell.
  ///
  /// In en, this message translates to:
  /// **'cell'**
  String get ovCell;

  /// No description provided for @ovCard.
  ///
  /// In en, this message translates to:
  /// **'card'**
  String get ovCard;

  /// No description provided for @ovHeardAgo.
  ///
  /// In en, this message translates to:
  /// **'heard'**
  String get ovHeardAgo;

  /// No description provided for @ovFault.
  ///
  /// In en, this message translates to:
  /// **'fault'**
  String get ovFault;

  /// No description provided for @ovFlash.
  ///
  /// In en, this message translates to:
  /// **'flash'**
  String get ovFlash;

  /// No description provided for @stRecording.
  ///
  /// In en, this message translates to:
  /// **'recording'**
  String get stRecording;

  /// No description provided for @stAlone.
  ///
  /// In en, this message translates to:
  /// **'recording alone'**
  String get stAlone;

  /// No description provided for @stSearching.
  ///
  /// In en, this message translates to:
  /// **'recording, seeking'**
  String get stSearching;

  /// No description provided for @stPaused.
  ///
  /// In en, this message translates to:
  /// **'paused'**
  String get stPaused;

  /// No description provided for @stWaiting.
  ///
  /// In en, this message translates to:
  /// **'waiting'**
  String get stWaiting;

  /// No description provided for @stCalibrating.
  ///
  /// In en, this message translates to:
  /// **'calibrating'**
  String get stCalibrating;

  /// No description provided for @stGiving.
  ///
  /// In en, this message translates to:
  /// **'giving its record'**
  String get stGiving;

  /// No description provided for @stFirmware.
  ///
  /// In en, this message translates to:
  /// **'taking firmware'**
  String get stFirmware;

  /// No description provided for @stErasing.
  ///
  /// In en, this message translates to:
  /// **'erasing'**
  String get stErasing;

  /// No description provided for @fNoCard.
  ///
  /// In en, this message translates to:
  /// **'no card'**
  String get fNoCard;

  /// No description provided for @fCardFull.
  ///
  /// In en, this message translates to:
  /// **'card full'**
  String get fCardFull;

  /// No description provided for @fWrite.
  ///
  /// In en, this message translates to:
  /// **'card write failed'**
  String get fWrite;

  /// No description provided for @fFs.
  ///
  /// In en, this message translates to:
  /// **'file system unreadable'**
  String get fFs;

  /// No description provided for @fDropped.
  ///
  /// In en, this message translates to:
  /// **'blocks dropped'**
  String get fDropped;

  /// No description provided for @fSensor.
  ///
  /// In en, this message translates to:
  /// **'sensor silent'**
  String get fSensor;

  /// No description provided for @fClock.
  ///
  /// In en, this message translates to:
  /// **'clock on its fallback'**
  String get fClock;

  /// No description provided for @fCellLow.
  ///
  /// In en, this message translates to:
  /// **'cell low'**
  String get fCellLow;

  /// No description provided for @fCharger.
  ///
  /// In en, this message translates to:
  /// **'charger fault'**
  String get fCharger;

  /// No description provided for @fPmic.
  ///
  /// In en, this message translates to:
  /// **'PMIC silent'**
  String get fPmic;

  /// No description provided for @fNoCal.
  ///
  /// In en, this message translates to:
  /// **'no calibration'**
  String get fNoCal;

  /// No description provided for @fRefused.
  ///
  /// In en, this message translates to:
  /// **'settings refused'**
  String get fRefused;

  /// No description provided for @uKind.
  ///
  /// In en, this message translates to:
  /// **'kind, number'**
  String get uKind;

  /// No description provided for @uChip.
  ///
  /// In en, this message translates to:
  /// **'chip'**
  String get uChip;

  /// No description provided for @uSensor.
  ///
  /// In en, this message translates to:
  /// **'sensor'**
  String get uSensor;

  /// No description provided for @uFirmware.
  ///
  /// In en, this message translates to:
  /// **'firmware'**
  String get uFirmware;

  /// No description provided for @uTemp.
  ///
  /// In en, this message translates to:
  /// **'temperature'**
  String get uTemp;

  /// No description provided for @uCal.
  ///
  /// In en, this message translates to:
  /// **'calibration'**
  String get uCal;

  /// No description provided for @uWith.
  ///
  /// In en, this message translates to:
  /// **'With it'**
  String get uWith;

  /// No description provided for @uBlink.
  ///
  /// In en, this message translates to:
  /// **'Blink 10 s'**
  String get uBlink;

  /// No description provided for @uErase.
  ///
  /// In en, this message translates to:
  /// **'Erase the card… (password)'**
  String get uErase;

  /// No description provided for @uForget.
  ///
  /// In en, this message translates to:
  /// **'Forget… (password)'**
  String get uForget;

  /// No description provided for @uOff.
  ///
  /// In en, this message translates to:
  /// **'Switch off'**
  String get uOff;

  /// No description provided for @uPassword.
  ///
  /// In en, this message translates to:
  /// **'The password again'**
  String get uPassword;

  /// No description provided for @uForgotten.
  ///
  /// In en, this message translates to:
  /// **'Forgotten, the unit leaves the set and keeps its records.'**
  String get uForgotten;

  /// No description provided for @uDone.
  ///
  /// In en, this message translates to:
  /// **'Done.'**
  String get uDone;

  /// No description provided for @uNotNow.
  ///
  /// In en, this message translates to:
  /// **'Not now: while the set records, or Povodok at work.'**
  String get uNotNow;

  /// No description provided for @calOnto.
  ///
  /// In en, this message translates to:
  /// **'Every unit onto the face {face}, then touch Confirm.'**
  String calOnto(String face);

  /// No description provided for @calTurn.
  ///
  /// In en, this message translates to:
  /// **'Turn every unit about its {axis} axis onto the face {face}, then touch Confirm.'**
  String calTurn(String axis, String face);

  /// No description provided for @calConfirm.
  ///
  /// In en, this message translates to:
  /// **'Confirm'**
  String get calConfirm;

  /// No description provided for @calMeasuring.
  ///
  /// In en, this message translates to:
  /// **'Hold still: 5 s…'**
  String get calMeasuring;

  /// No description provided for @calTaken.
  ///
  /// In en, this message translates to:
  /// **'still ✓'**
  String get calTaken;

  /// No description provided for @calMoved.
  ///
  /// In en, this message translates to:
  /// **'moved ✗, again'**
  String get calMoved;

  /// No description provided for @calOtherFace.
  ///
  /// In en, this message translates to:
  /// **'another face ✗, again'**
  String get calOtherFace;

  /// No description provided for @calDone.
  ///
  /// In en, this message translates to:
  /// **'Done: every unit keeps its new calibration.'**
  String get calDone;

  /// No description provided for @calAge.
  ///
  /// In en, this message translates to:
  /// **'{days, plural, =0{today} =1{1 day} other{{days} days}}'**
  String calAge(int days);

  /// No description provided for @dlStarting.
  ///
  /// In en, this message translates to:
  /// **'Starting…'**
  String get dlStarting;

  /// No description provided for @dlDone.
  ///
  /// In en, this message translates to:
  /// **'Done: {path}'**
  String dlDone(String path);

  /// No description provided for @dlOf.
  ///
  /// In en, this message translates to:
  /// **'{got} of {all} kB'**
  String dlOf(int got, int all);

  /// No description provided for @dlLeft.
  ///
  /// In en, this message translates to:
  /// **'{s} s left'**
  String dlLeft(int s);

  /// No description provided for @dlLeftOut.
  ///
  /// In en, this message translates to:
  /// **'left out: not reached, or its file missing'**
  String get dlLeftOut;

  /// No description provided for @actStop.
  ///
  /// In en, this message translates to:
  /// **'Stop'**
  String get actStop;

  /// No description provided for @liveSave.
  ///
  /// In en, this message translates to:
  /// **'Save'**
  String get liveSave;

  /// No description provided for @liveSaveStop.
  ///
  /// In en, this message translates to:
  /// **'Stop saving'**
  String get liveSaveStop;

  /// No description provided for @liveSaving.
  ///
  /// In en, this message translates to:
  /// **'saving {time} · {mb} MB'**
  String liveSaving(Object time, Object mb);

  /// No description provided for @actContinue.
  ///
  /// In en, this message translates to:
  /// **'Continue'**
  String get actContinue;

  /// No description provided for @setToWork.
  ///
  /// In en, this message translates to:
  /// **'To work'**
  String get setToWork;

  /// No description provided for @setNoLive.
  ///
  /// In en, this message translates to:
  /// **'No live view yet'**
  String get setNoLive;

  /// No description provided for @diagBand.
  ///
  /// In en, this message translates to:
  /// **'The band: noise by channel, dBm'**
  String get diagBand;

  /// No description provided for @diagSet.
  ///
  /// In en, this message translates to:
  /// **'the set {ch}'**
  String diagSet(Object ch);

  /// No description provided for @diagSpare.
  ///
  /// In en, this message translates to:
  /// **'spare {ch}'**
  String diagSpare(Object ch);

  /// No description provided for @diagMargin.
  ///
  /// In en, this message translates to:
  /// **'Povodok {k}: margin {db} dB'**
  String diagMargin(Object k, Object db);

  /// No description provided for @diagUnits.
  ///
  /// In en, this message translates to:
  /// **'Units'**
  String get diagUnits;

  /// No description provided for @diagHeard.
  ///
  /// In en, this message translates to:
  /// **'heard {s} s ago'**
  String diagHeard(Object s);

  /// No description provided for @diagNotHeard.
  ///
  /// In en, this message translates to:
  /// **'not heard'**
  String get diagNotHeard;

  /// No description provided for @diagSync.
  ///
  /// In en, this message translates to:
  /// **'SYNC {dbm} dBm, {missed} missed'**
  String diagSync(Object dbm, Object missed);

  /// No description provided for @diagClock.
  ///
  /// In en, this message translates to:
  /// **'clock {ns} ns'**
  String diagClock(Object ns);

  /// No description provided for @diagOut.
  ///
  /// In en, this message translates to:
  /// **'out of range {acc}/{gyro}'**
  String diagOut(Object acc, Object gyro);

  /// No description provided for @diagCell.
  ///
  /// In en, this message translates to:
  /// **'cell {pct} %'**
  String diagCell(Object pct);

  /// No description provided for @diagCard.
  ///
  /// In en, this message translates to:
  /// **'card {free} % free, {dropped} dropped'**
  String diagCard(Object free, Object dropped);

  /// No description provided for @liveCurves.
  ///
  /// In en, this message translates to:
  /// **'Curves'**
  String get liveCurves;

  /// No description provided for @liveNumbers.
  ///
  /// In en, this message translates to:
  /// **'Numbers'**
  String get liveNumbers;

  /// No description provided for @liveAccel.
  ///
  /// In en, this message translates to:
  /// **'vertical acceleration, g'**
  String get liveAccel;

  /// No description provided for @liveTilt.
  ///
  /// In en, this message translates to:
  /// **'tilt from the vertical, °'**
  String get liveTilt;

  /// No description provided for @liveRange.
  ///
  /// In en, this message translates to:
  /// **'range'**
  String get liveRange;

  /// No description provided for @liveStrides.
  ///
  /// In en, this message translates to:
  /// **'strides'**
  String get liveStrides;

  /// No description provided for @liveLastStrides.
  ///
  /// In en, this message translates to:
  /// **'the last {n} strides'**
  String liveLastStrides(Object n);

  /// No description provided for @liveNoRhythm.
  ///
  /// In en, this message translates to:
  /// **'no stride yet'**
  String get liveNoRhythm;

  /// No description provided for @liveLeftRight.
  ///
  /// In en, this message translates to:
  /// **'Left against right'**
  String get liveLeftRight;

  /// No description provided for @liveDifference.
  ///
  /// In en, this message translates to:
  /// **'difference'**
  String get liveDifference;

  /// No description provided for @placesTitle.
  ///
  /// In en, this message translates to:
  /// **'Places\' templates'**
  String get placesTitle;

  /// No description provided for @placesTemplate.
  ///
  /// In en, this message translates to:
  /// **'Template'**
  String get placesTemplate;

  /// No description provided for @placesAnimal.
  ///
  /// In en, this message translates to:
  /// **'The animal\'s name'**
  String get placesAnimal;

  /// No description provided for @placesNote.
  ///
  /// In en, this message translates to:
  /// **'The code shows the unit everywhere; the set\'s table goes with the session into every unit\'s file.'**
  String get placesNote;

  /// No description provided for @animalHorse.
  ///
  /// In en, this message translates to:
  /// **'horse'**
  String get animalHorse;

  /// No description provided for @animalDog.
  ///
  /// In en, this message translates to:
  /// **'dog'**
  String get animalDog;

  /// No description provided for @animalPerson.
  ///
  /// In en, this message translates to:
  /// **'person'**
  String get animalPerson;

  /// No description provided for @placeHead.
  ///
  /// In en, this message translates to:
  /// **'head'**
  String get placeHead;

  /// No description provided for @placeWithers.
  ///
  /// In en, this message translates to:
  /// **'withers'**
  String get placeWithers;

  /// No description provided for @placeThoracic.
  ///
  /// In en, this message translates to:
  /// **'thoracic spine'**
  String get placeThoracic;

  /// No description provided for @placeLumbar.
  ///
  /// In en, this message translates to:
  /// **'lumbar spine'**
  String get placeLumbar;

  /// No description provided for @placePelvis.
  ///
  /// In en, this message translates to:
  /// **'pelvis'**
  String get placePelvis;

  /// No description provided for @placeLeftFore.
  ///
  /// In en, this message translates to:
  /// **'left fore'**
  String get placeLeftFore;

  /// No description provided for @placeRightFore.
  ///
  /// In en, this message translates to:
  /// **'right fore'**
  String get placeRightFore;

  /// No description provided for @placeLeftHind.
  ///
  /// In en, this message translates to:
  /// **'left hind'**
  String get placeLeftHind;

  /// No description provided for @placeRightHind.
  ///
  /// In en, this message translates to:
  /// **'right hind'**
  String get placeRightHind;

  /// No description provided for @placeLeftShoulder.
  ///
  /// In en, this message translates to:
  /// **'left shoulder'**
  String get placeLeftShoulder;

  /// No description provided for @placeRightShoulder.
  ///
  /// In en, this message translates to:
  /// **'right shoulder'**
  String get placeRightShoulder;

  /// No description provided for @placeLeftElbow.
  ///
  /// In en, this message translates to:
  /// **'left elbow'**
  String get placeLeftElbow;

  /// No description provided for @placeRightElbow.
  ///
  /// In en, this message translates to:
  /// **'right elbow'**
  String get placeRightElbow;

  /// No description provided for @placeLeftHip.
  ///
  /// In en, this message translates to:
  /// **'left hip'**
  String get placeLeftHip;

  /// No description provided for @placeRightHip.
  ///
  /// In en, this message translates to:
  /// **'right hip'**
  String get placeRightHip;

  /// No description provided for @animalCow.
  ///
  /// In en, this message translates to:
  /// **'cow'**
  String get animalCow;

  /// No description provided for @animalCat.
  ///
  /// In en, this message translates to:
  /// **'cat'**
  String get animalCat;

  /// No description provided for @setNewUnits.
  ///
  /// In en, this message translates to:
  /// **'New units'**
  String get setNewUnits;

  /// No description provided for @fwTitle.
  ///
  /// In en, this message translates to:
  /// **'Firmware'**
  String get fwTitle;

  /// No description provided for @fwReady.
  ///
  /// In en, this message translates to:
  /// **'Firmware {version} on the phone, signed and checked'**
  String fwReady(String version);

  /// No description provided for @fwNone.
  ///
  /// In en, this message translates to:
  /// **'No firmware on the phone yet.'**
  String get fwNone;

  /// No description provided for @fwFetch.
  ///
  /// In en, this message translates to:
  /// **'Fetch from GitHub'**
  String get fwFetch;

  /// No description provided for @fwFetched.
  ///
  /// In en, this message translates to:
  /// **'Fetched: {version}'**
  String fwFetched(String version);

  /// No description provided for @fwNotPublished.
  ///
  /// In en, this message translates to:
  /// **'No firmware published yet.'**
  String get fwNotPublished;

  /// No description provided for @fwUpdateAll.
  ///
  /// In en, this message translates to:
  /// **'Update all'**
  String get fwUpdateAll;

  /// No description provided for @fwWaiting.
  ///
  /// In en, this message translates to:
  /// **'waiting'**
  String get fwWaiting;

  /// No description provided for @fwTaking.
  ///
  /// In en, this message translates to:
  /// **'taking it'**
  String get fwTaking;

  /// No description provided for @fwDone.
  ///
  /// In en, this message translates to:
  /// **'done ✓'**
  String get fwDone;

  /// No description provided for @fwCurrent.
  ///
  /// In en, this message translates to:
  /// **'up to date ✓'**
  String get fwCurrent;

  /// No description provided for @fwOff.
  ///
  /// In en, this message translates to:
  /// **'off: later'**
  String get fwOff;

  /// No description provided for @fwFailed.
  ///
  /// In en, this message translates to:
  /// **'failed, again'**
  String get fwFailed;

  /// No description provided for @fwHow.
  ///
  /// In en, this message translates to:
  /// **'By SMP through Povodok, one unit at a time, Povodok last; each unit checks the image\'s signature and keeps the old one should the new not start; a unit off takes it when it is on.'**
  String get fwHow;

  /// No description provided for @fwPovodokAgain.
  ///
  /// In en, this message translates to:
  /// **'Povodok starts again with the new firmware: join it again.'**
  String get fwPovodokAgain;

  /// No description provided for @setNewSet.
  ///
  /// In en, this message translates to:
  /// **'A new set'**
  String get setNewSet;

  /// No description provided for @nsIntro.
  ///
  /// In en, this message translates to:
  /// **'A new set — its address, key and channel — given to Povodok, which starts again in it. The units then come in by the search, a Belka also by the file on its cable.'**
  String get nsIntro;

  /// No description provided for @nsChannel.
  ///
  /// In en, this message translates to:
  /// **'Channel'**
  String get nsChannel;

  /// No description provided for @nsNotMeasured.
  ///
  /// In en, this message translates to:
  /// **'not measured'**
  String get nsNotMeasured;

  /// No description provided for @nsQuietest.
  ///
  /// In en, this message translates to:
  /// **'the quietest'**
  String get nsQuietest;

  /// No description provided for @nsLabel.
  ///
  /// In en, this message translates to:
  /// **'The set\'s own number, 1–9999, if any'**
  String get nsLabel;

  /// No description provided for @nsDraw.
  ///
  /// In en, this message translates to:
  /// **'Draw a new set'**
  String get nsDraw;

  /// No description provided for @nsDrawn.
  ///
  /// In en, this message translates to:
  /// **'The set: {address}, channel {ch}'**
  String nsDrawn(String address, Object ch);

  /// No description provided for @nsPovodok.
  ///
  /// In en, this message translates to:
  /// **'This Povodok\'s number in the set'**
  String get nsPovodok;

  /// No description provided for @nsPassword.
  ///
  /// In en, this message translates to:
  /// **'Povodok\'s password, again'**
  String get nsPassword;

  /// No description provided for @nsGive.
  ///
  /// In en, this message translates to:
  /// **'Give it to this Povodok'**
  String get nsGive;

  /// No description provided for @nsAsk.
  ///
  /// In en, this message translates to:
  /// **'Povodok leaves its set for the new one and starts again; the units of the old set stay out until they are given the new one.'**
  String get nsAsk;

  /// No description provided for @nsGiven.
  ///
  /// In en, this message translates to:
  /// **'Povodok takes the new set and starts again: join it again, then search for the units.'**
  String get nsGiven;

  /// No description provided for @nsFileFor.
  ///
  /// In en, this message translates to:
  /// **'STRELKA.SET for the Belka numbered'**
  String get nsFileFor;

  /// No description provided for @nsSave.
  ///
  /// In en, this message translates to:
  /// **'Save'**
  String get nsSave;

  /// No description provided for @nsSaved.
  ///
  /// In en, this message translates to:
  /// **'Saved: {path} — copied onto the Belka\'s disk on its cable, it is taken when the cable leaves.'**
  String nsSaved(String path);

  /// No description provided for @newTitle.
  ///
  /// In en, this message translates to:
  /// **'New units: the search'**
  String get newTitle;

  /// No description provided for @newSearching.
  ///
  /// In en, this message translates to:
  /// **'Povodok calls; a new unit answers in its window.'**
  String get newSearching;

  /// No description provided for @newNone.
  ///
  /// In en, this message translates to:
  /// **'No new unit heard yet.'**
  String get newNone;

  /// No description provided for @newTake.
  ///
  /// In en, this message translates to:
  /// **'Take as {n}'**
  String newTake(Object n);

  /// No description provided for @newTaken.
  ///
  /// In en, this message translates to:
  /// **'taken as {n}'**
  String newTaken(Object n);

  /// No description provided for @newCable.
  ///
  /// In en, this message translates to:
  /// **'A Belka on its cable takes the set faster, the file STRELKA.SET on its disk.'**
  String get newCable;

  /// No description provided for @pwTitle.
  ///
  /// In en, this message translates to:
  /// **'Password'**
  String get pwTitle;

  /// No description provided for @pwOld.
  ///
  /// In en, this message translates to:
  /// **'The password now'**
  String get pwOld;

  /// No description provided for @pwNew.
  ///
  /// In en, this message translates to:
  /// **'The new password, 12 characters at least'**
  String get pwNew;

  /// No description provided for @pwAgain.
  ///
  /// In en, this message translates to:
  /// **'The new password again'**
  String get pwAgain;

  /// No description provided for @pwShort.
  ///
  /// In en, this message translates to:
  /// **'At least 12 characters.'**
  String get pwShort;

  /// No description provided for @pwMismatch.
  ///
  /// In en, this message translates to:
  /// **'The two do not match.'**
  String get pwMismatch;

  /// No description provided for @pwChange.
  ///
  /// In en, this message translates to:
  /// **'Change'**
  String get pwChange;

  /// No description provided for @pwChanged.
  ///
  /// In en, this message translates to:
  /// **'The password changed: join again with it.'**
  String get pwChanged;

  /// No description provided for @offTitle.
  ///
  /// In en, this message translates to:
  /// **'Switch Povodok off'**
  String get offTitle;

  /// No description provided for @offAsk.
  ///
  /// In en, this message translates to:
  /// **'Switch Povodok off? Recording, it stops the set first.'**
  String get offAsk;

  /// No description provided for @keyNew.
  ///
  /// In en, this message translates to:
  /// **'Povodok\'s key kept: {fp}'**
  String keyNew(Object fp);

  /// No description provided for @keyChangedTitle.
  ///
  /// In en, this message translates to:
  /// **'Another key'**
  String get keyChangedTitle;

  /// No description provided for @keyChanged.
  ///
  /// In en, this message translates to:
  /// **'Povodok answers with another key ({now}) than the one kept ({old}). After its factory settings that is right; otherwise someone else answers. Take the new key?'**
  String keyChanged(Object now, Object old);

  /// No description provided for @keyTake.
  ///
  /// In en, this message translates to:
  /// **'Take it'**
  String get keyTake;

  /// No description provided for @keyFingerprint.
  ///
  /// In en, this message translates to:
  /// **'Povodok\'s key'**
  String get keyFingerprint;

  /// No description provided for @pwFactory.
  ///
  /// In en, this message translates to:
  /// **'The factory password: give a new one, 12 characters at least.'**
  String get pwFactory;

  /// No description provided for @diagMove.
  ///
  /// In en, this message translates to:
  /// **'Move the set to {ch}'**
  String diagMove(Object ch);
}

class _AppLocalizationsDelegate
    extends LocalizationsDelegate<AppLocalizations> {
  const _AppLocalizationsDelegate();

  @override
  Future<AppLocalizations> load(Locale locale) {
    return SynchronousFuture<AppLocalizations>(lookupAppLocalizations(locale));
  }

  @override
  bool isSupported(Locale locale) => <String>[
    'cs',
    'de',
    'en',
    'es',
    'fr',
    'pt',
    'ru',
  ].contains(locale.languageCode);

  @override
  bool shouldReload(_AppLocalizationsDelegate old) => false;
}

AppLocalizations lookupAppLocalizations(Locale locale) {
  // Lookup logic when only language code is specified.
  switch (locale.languageCode) {
    case 'cs':
      return AppLocalizationsCs();
    case 'de':
      return AppLocalizationsDe();
    case 'en':
      return AppLocalizationsEn();
    case 'es':
      return AppLocalizationsEs();
    case 'fr':
      return AppLocalizationsFr();
    case 'pt':
      return AppLocalizationsPt();
    case 'ru':
      return AppLocalizationsRu();
  }

  throw FlutterError(
    'AppLocalizations.delegate failed to load unsupported locale "$locale". This is likely '
    'an issue with the localizations generation tool. Please file an issue '
    'on GitHub with a reproducible sample app and the gen-l10n configuration '
    'that was used.',
  );
}
