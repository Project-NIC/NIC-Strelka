// ignore: unused_import
import 'package:intl/intl.dart' as intl;
import 'app_localizations.dart';

// ignore_for_file: type=lint

/// The translations for Russian (`ru`).
class AppLocalizationsRu extends AppLocalizations {
  AppLocalizationsRu([String locale = 'ru']) : super(locale);

  @override
  String get menuOverview => 'Обзор';

  @override
  String get menuCalibration => 'Калибровка';

  @override
  String get menuMeasurement => 'Измерение';

  @override
  String get menuData => 'Данные';

  @override
  String get menuDiagnostics => 'Диагностика';

  @override
  String get menuSettings => 'Настройки';

  @override
  String get joinAddress => 'Адрес Povodok';

  @override
  String get joinPassword => 'Пароль';

  @override
  String get joinButton => 'Подключить';

  @override
  String get joinJoining => 'Подключение…';

  @override
  String get joinRefused => 'Povodok отклонил пароль.';

  @override
  String joinNoAnswer(String message) {
    return 'Povodok не отвечает: $message';
  }

  @override
  String get joinTimeout => 'Povodok не ответил вовремя.';

  @override
  String get settingsLanguage => 'Язык';

  @override
  String get settingsAbout => 'О программе';

  @override
  String get aboutPicture => 'Pušinka, щенок Стрелки, рисунок.';

  @override
  String get aboutOpen => 'Открыто, каждый может собрать; ничего не продаётся.';

  @override
  String get overviewUnits => 'Устройства набора';

  @override
  String get overviewNone => 'Пока не слышно ни одного устройства.';

  @override
  String measSession(int s) {
    return 'Сеанс $s';
  }

  @override
  String get measChoose => 'Устройства: выбор касанием';

  @override
  String get measNote => 'Заметка: место, животное, что случилось';

  @override
  String get measNone => 'Пока не слышно ни одного устройства набора.';

  @override
  String measChosen(int chosen, int all) {
    return 'Выбрано $chosen из $all';
  }

  @override
  String get measRate => 'Частота';

  @override
  String get measAcc => 'Акселерометр';

  @override
  String get measGyro => 'Гироскоп';

  @override
  String get measHg => 'Канал больших g';

  @override
  String get measHgOff => 'выкл.';

  @override
  String get measLive => 'Живой просмотр';

  @override
  String get liveOff => 'выкл.';

  @override
  String get liveQuat50 => 'кватернионы 50 Гц';

  @override
  String get liveQuat100 => 'кватернионы 100 Гц';

  @override
  String get liveRaw50 => 'сырые шаги 50 Гц';

  @override
  String get liveRaw100 => 'сырые шаги 100 Гц';

  @override
  String get measSyncFirst => 'Синхро: сеанс ещё не отправлен';

  @override
  String measInStep(int n, int all) {
    return 'Синхро: $n из $all выбранных синхронны';
  }

  @override
  String get measSync => 'Синхро';

  @override
  String get measStart => 'СТАРТ';

  @override
  String get measLiveOnly => 'Live';

  @override
  String get measNoCal => 'без калибровки';

  @override
  String get measExpired => 'калибровка устарела';

  @override
  String get measUnreachable => 'вне досягаемости';

  @override
  String measTempApart(int deg) {
    return '$deg °C от калибровки';
  }

  @override
  String get setMeasurement => 'Измерение и калибровка';

  @override
  String get setCalValid => 'срок калибровки';

  @override
  String get calEvery => 'каждое измерение';

  @override
  String get cal2h => '2 ч';

  @override
  String get calDay => 'сутки';

  @override
  String get cal30 => '30 дней';

  @override
  String get calUnlimited => 'без ограничения';

  @override
  String get setTempApart => 'предупреждать при разнице температуры в';

  @override
  String get setCurves => 'окно кривых';

  @override
  String get setNumbers => 'числа за последние';

  @override
  String strides(int n) {
    return '$n шагов';
  }

  @override
  String get setLost => 'Связь с Povodok потеряна';

  @override
  String get setPower => 'мощность передатчика';

  @override
  String get powerFull => 'полная, в пределах правил';

  @override
  String get setLight => 'свет при измерении';

  @override
  String get lightUsual => 'как обычно';

  @override
  String get lightFaults => 'только неисправности';

  @override
  String get lightOff => 'выкл.';

  @override
  String get setToCome =>
      'будет передано устройствам, когда их прошивка это сможет';

  @override
  String get setLocked => 'Меняется только вне измерения.';

  @override
  String get dataPhone => 'В телефоне';

  @override
  String get dataUnits => 'На устройствах';

  @override
  String get dataNoFiles => 'В телефоне пока ничего нет.';

  @override
  String get dataNoSessions => 'На устройствах нет сеансов.';

  @override
  String get dataGot => 'в телефоне ✓';

  @override
  String dataSwitchOn(String units) {
    return 'включи $units';
  }

  @override
  String get dataLarge => 'большой: док или кабель';

  @override
  String get dataReady => 'готово';

  @override
  String get dataToCsv => 'В CSV';

  @override
  String get dataToC3d => 'В C3D';

  @override
  String dataAsk(int n) {
    return 'Загрузить с устройств ($n) в один C3D?';
  }

  @override
  String get dataCancel => 'Отмена';

  @override
  String get dataDownload => 'Загрузить';

  @override
  String get ovPovodoks => 'Povodok';

  @override
  String get ovToPhone => 'к телефону';

  @override
  String get ovHeard => 'слышно';

  @override
  String get ovNotHeard => 'не слышно';

  @override
  String get ovCode => 'код';

  @override
  String get ovUnit => 'устройство';

  @override
  String get ovState => 'состояние';

  @override
  String get ovCell => 'батарея';

  @override
  String get ovCard => 'карта';

  @override
  String get ovHeardAgo => 'слышно';

  @override
  String get ovFault => 'неисправность';

  @override
  String get ovFlash => 'flash';

  @override
  String get stRecording => 'запись';

  @override
  String get stAlone => 'запись автономно';

  @override
  String get stSearching => 'запись, поиск';

  @override
  String get stPaused => 'пауза';

  @override
  String get stWaiting => 'ожидание';

  @override
  String get stCalibrating => 'калибровка';

  @override
  String get stGiving => 'отдаёт запись';

  @override
  String get stFirmware => 'получает прошивку';

  @override
  String get stErasing => 'стирание';

  @override
  String get fNoCard => 'нет карты';

  @override
  String get fCardFull => 'карта заполнена';

  @override
  String get fWrite => 'ошибка записи на карту';

  @override
  String get fFs => 'файловая система нечитаема';

  @override
  String get fDropped => 'блоки потеряны';

  @override
  String get fSensor => 'датчик молчит';

  @override
  String get fClock => 'часы на резерве';

  @override
  String get fCellLow => 'батарея разряжена';

  @override
  String get fCharger => 'неисправность зарядки';

  @override
  String get fPmic => 'PMIC молчит';

  @override
  String get fNoCal => 'без калибровки';

  @override
  String get fRefused => 'настройки отклонены';

  @override
  String get uKind => 'тип, номер';

  @override
  String get uChip => 'чип';

  @override
  String get uSensor => 'датчик';

  @override
  String get uFirmware => 'прошивка';

  @override
  String get uTemp => 'температура';

  @override
  String get uCal => 'калибровка';

  @override
  String get uWith => 'С устройством';

  @override
  String get uBlink => 'Мигнуть 10 с';

  @override
  String get uErase => 'Стереть карту… (пароль)';

  @override
  String get uForget => 'Забыть… (пароль)';

  @override
  String get uOff => 'Выключить';

  @override
  String get uPassword => 'Пароль ещё раз';

  @override
  String get uForgotten =>
      'Забытое устройство покидает набор, записи остаются.';

  @override
  String get uDone => 'Готово.';

  @override
  String get uNotNow => 'Не сейчас: набор записывает или Povodok в работе.';

  @override
  String calOnto(String face) {
    return 'Все устройства на сторону $face, затем нажмите «Подтвердить».';
  }

  @override
  String calTurn(String axis, String face) {
    return 'Поверните все устройства вокруг оси $axis на сторону $face, затем нажмите «Подтвердить».';
  }

  @override
  String get calConfirm => 'Подтвердить';

  @override
  String get calMeasuring => 'Не двигать: 5 с…';

  @override
  String get calTaken => 'в покое ✓';

  @override
  String get calMoved => 'сдвинулось ✗, ещё раз';

  @override
  String get calOtherFace => 'другая сторона ✗, ещё раз';

  @override
  String get calDone => 'Готово: каждое устройство хранит новую калибровку.';

  @override
  String calAge(int days) {
    String _temp0 = intl.Intl.pluralLogic(
      days,
      locale: localeName,
      other: '$days дня',
      many: '$days дней',
      few: '$days дня',
      one: '$days день',
      zero: 'сегодня',
    );
    return '$_temp0';
  }

  @override
  String get dlStarting => 'Запуск…';

  @override
  String dlDone(String path) {
    return 'Готово: $path';
  }

  @override
  String dlOf(int got, int all) {
    return '$got из $all кБ';
  }

  @override
  String dlLeft(int s) {
    return 'осталось $s с';
  }

  @override
  String get dlLeftOut => 'пропущено: недоступно, или нет его файла';

  @override
  String get actStop => 'Стоп';

  @override
  String get liveSave => 'Сохранить';

  @override
  String get liveSaveStop => 'Остановить сохранение';

  @override
  String liveSaving(Object time, Object mb) {
    return 'сохраняется $time · $mb МБ';
  }

  @override
  String get actContinue => 'Продолжить';

  @override
  String get setToWork => 'В работу';

  @override
  String get setNoLive => 'Живого просмотра пока нет';

  @override
  String get diagBand => 'Диапазон: шум по каналам, дБм';

  @override
  String diagSet(Object ch) {
    return 'комплект $ch';
  }

  @override
  String diagSpare(Object ch) {
    return 'запасной $ch';
  }

  @override
  String diagMargin(Object k, Object db) {
    return 'Povodok $k: запас $db дБ';
  }

  @override
  String get diagUnits => 'Устройства';

  @override
  String diagHeard(Object s) {
    return 'слышно $s с назад';
  }

  @override
  String get diagNotHeard => 'не слышно';

  @override
  String diagSync(Object dbm, Object missed) {
    return 'SYNC $dbm дБм, пропущено $missed';
  }

  @override
  String diagClock(Object ns) {
    return 'часы $ns нс';
  }

  @override
  String diagOut(Object acc, Object gyro) {
    return 'вне диапазона $acc/$gyro';
  }

  @override
  String diagCell(Object pct) {
    return 'батарея $pct %';
  }

  @override
  String diagCard(Object free, Object dropped) {
    return 'карта $free % свободно, отброшено $dropped';
  }

  @override
  String get liveCurves => 'Кривые';

  @override
  String get liveNumbers => 'Числа';

  @override
  String get liveAccel => 'вертикальное ускорение, g';

  @override
  String get liveTilt => 'наклон от вертикали, °';

  @override
  String get liveRange => 'размах';

  @override
  String get liveStrides => 'шаги';

  @override
  String liveLastStrides(Object n) {
    return 'последние $n шагов';
  }

  @override
  String get liveNoRhythm => 'шагов пока нет';

  @override
  String get liveLeftRight => 'Левая против правой';

  @override
  String get liveDifference => 'разница';

  @override
  String get placesTitle => 'Шаблоны мест';

  @override
  String get placesTemplate => 'Шаблон';

  @override
  String get placesAnimal => 'Кличка животного';

  @override
  String get placesNote =>
      'Код показывает устройство везде; таблица комплекта уходит с сеансом в файл каждого устройства.';

  @override
  String get animalHorse => 'лошадь';

  @override
  String get animalDog => 'собака';

  @override
  String get animalPerson => 'человек';

  @override
  String get placeHead => 'голова';

  @override
  String get placeWithers => 'холка';

  @override
  String get placeThoracic => 'грудной отдел позвоночника';

  @override
  String get placeLumbar => 'поясничный отдел позвоночника';

  @override
  String get placePelvis => 'таз';

  @override
  String get placeLeftFore => 'левая передняя';

  @override
  String get placeRightFore => 'правая передняя';

  @override
  String get placeLeftHind => 'левая задняя';

  @override
  String get placeRightHind => 'правая задняя';

  @override
  String get placeLeftShoulder => 'левое плечо';

  @override
  String get placeRightShoulder => 'правое плечо';

  @override
  String get placeLeftElbow => 'левый локоть';

  @override
  String get placeRightElbow => 'правый локоть';

  @override
  String get placeLeftHip => 'левое бедро';

  @override
  String get placeRightHip => 'правое бедро';

  @override
  String get animalCow => 'корова';

  @override
  String get animalCat => 'кошка';

  @override
  String get setNewUnits => 'Новые устройства';

  @override
  String get fwTitle => 'Прошивка';

  @override
  String fwReady(String version) {
    return 'Прошивка $version в телефоне, подписана и проверена';
  }

  @override
  String get fwNone => 'В телефоне пока нет прошивки.';

  @override
  String get fwFetch => 'Скачать с GitHub';

  @override
  String fwFetched(String version) {
    return 'Скачано: $version';
  }

  @override
  String get fwNotPublished => 'Прошивка пока не опубликована.';

  @override
  String get fwUpdateAll => 'Обновить все';

  @override
  String get fwWaiting => 'ждёт';

  @override
  String get fwTaking => 'принимает';

  @override
  String get fwDone => 'готово ✓';

  @override
  String get fwCurrent => 'актуально ✓';

  @override
  String get fwOff => 'выключено: позже';

  @override
  String get fwFailed => 'не удалось, ещё раз';

  @override
  String get fwHow =>
      'По SMP через Povodok, по одному устройству, Povodok последним; каждое устройство проверяет подпись образа и оставляет старый, если новый не запустится; выключенное получит его, когда будет включено.';

  @override
  String get fwPovodokAgain =>
      'Povodok перезапускается с новой прошивкой: подключитесь снова.';

  @override
  String get setNewSet => 'Новый набор';

  @override
  String get nsIntro =>
      'Новый набор — его адрес, ключ и канал — передаётся Povodok, который перезапускается в нём. Устройства затем добавляются поиском, Belka также файлом по кабелю.';

  @override
  String get nsChannel => 'Канал';

  @override
  String get nsNotMeasured => 'не измерен';

  @override
  String get nsQuietest => 'самый тихий';

  @override
  String get nsLabel => 'Собственный номер набора, 1–9999, если есть';

  @override
  String get nsDraw => 'Создать новый набор';

  @override
  String nsDrawn(String address, Object ch) {
    return 'Набор: $address, канал $ch';
  }

  @override
  String get nsPovodok => 'Номер этого Povodok в наборе';

  @override
  String get nsPassword => 'Пароль Povodok, ещё раз';

  @override
  String get nsGive => 'Передать этому Povodok';

  @override
  String get nsAsk =>
      'Povodok переходит в новый набор и перезапускается; устройства старого набора остаются вне, пока не получат новый.';

  @override
  String get nsGiven =>
      'Povodok принял новый набор и перезапускается: подключитесь снова и найдите устройства.';

  @override
  String get nsFileFor => 'STRELKA.SET для Belka номер';

  @override
  String get nsSave => 'Сохранить';

  @override
  String nsSaved(String path) {
    return 'Сохранено: $path — скопируйте на диск Belka на кабеле, файл примется при отключении кабеля.';
  }

  @override
  String get newTitle => 'Новые устройства: поиск';

  @override
  String get newSearching =>
      'Povodok вызывает; новое устройство отвечает в своём окне.';

  @override
  String get newNone => 'Новых устройств пока не слышно.';

  @override
  String newTake(Object n) {
    return 'Принять как $n';
  }

  @override
  String newTaken(Object n) {
    return 'принято как $n';
  }

  @override
  String get newCable =>
      'Belka на кабеле получает комплект быстрее — файл STRELKA.SET на её диске.';

  @override
  String get pwTitle => 'Пароль';

  @override
  String get pwOld => 'Текущий пароль';

  @override
  String get pwNew => 'Новый пароль, не менее 12 символов';

  @override
  String get pwAgain => 'Новый пароль ещё раз';

  @override
  String get pwShort => 'Не менее 12 символов.';

  @override
  String get pwMismatch => 'Пароли не совпадают.';

  @override
  String get pwChange => 'Сменить';

  @override
  String get pwChanged => 'Пароль сменён: подключитесь снова с новым.';

  @override
  String get offTitle => 'Выключить Povodok';

  @override
  String get offAsk =>
      'Выключить Povodok? Если идёт запись, сначала останавливает комплект.';

  @override
  String keyNew(Object fp) {
    return 'Ключ Povodok сохранён: $fp';
  }

  @override
  String get keyChangedTitle => 'Другой ключ';

  @override
  String keyChanged(Object now, Object old) {
    return 'Povodok отвечает другим ключом ($now), а не сохранённым ($old). После сброса к заводским настройкам это верно; иначе отвечает кто-то другой. Принять новый ключ?';
  }

  @override
  String get keyTake => 'Принять';

  @override
  String get keyFingerprint => 'Ключ Povodok';

  @override
  String get pwFactory =>
      'Заводской пароль: задайте новый, не менее 12 символов.';

  @override
  String diagMove(Object ch) {
    return 'Перевести комплект на $ch';
  }
}
