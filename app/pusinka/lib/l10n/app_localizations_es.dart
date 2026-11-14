// ignore: unused_import
import 'package:intl/intl.dart' as intl;
import 'app_localizations.dart';

// ignore_for_file: type=lint

/// The translations for Spanish Castilian (`es`).
class AppLocalizationsEs extends AppLocalizations {
  AppLocalizationsEs([String locale = 'es']) : super(locale);

  @override
  String get menuOverview => 'Resumen';

  @override
  String get menuCalibration => 'Calibración';

  @override
  String get menuMeasurement => 'Medición';

  @override
  String get menuData => 'Datos';

  @override
  String get menuDiagnostics => 'Diagnóstico';

  @override
  String get menuSettings => 'Ajustes';

  @override
  String get joinAddress => 'Dirección del Povodok';

  @override
  String get joinPassword => 'Contraseña';

  @override
  String get joinButton => 'Conectar';

  @override
  String get joinJoining => 'Conectando…';

  @override
  String get joinRefused => 'El Povodok rechazó la contraseña.';

  @override
  String joinNoAnswer(String message) {
    return 'El Povodok no responde: $message';
  }

  @override
  String get joinTimeout => 'El Povodok no respondió a tiempo.';

  @override
  String get settingsLanguage => 'Idioma';

  @override
  String get settingsAbout => 'Acerca de';

  @override
  String get aboutPicture => 'Pušinka, la cachorra de Strelka, en dibujo.';

  @override
  String get aboutOpen =>
      'Abierto, para que cualquiera lo construya; no se vende nada.';

  @override
  String get overviewUnits => 'Unidades del equipo';

  @override
  String get overviewNone => 'Aún no se oye ninguna unidad.';

  @override
  String measSession(int s) {
    return 'Sesión $s';
  }

  @override
  String get measChoose => 'Unidades: elige con un toque';

  @override
  String get measNote => 'Nota: el lugar, el animal, qué pasó';

  @override
  String get measNone => 'Aún no se oye ninguna unidad del equipo.';

  @override
  String measChosen(int chosen, int all) {
    return 'Elegidas $chosen de $all';
  }

  @override
  String get measRate => 'Frecuencia';

  @override
  String get measAcc => 'Acelerómetro';

  @override
  String get measGyro => 'Giroscopio';

  @override
  String get measHg => 'Canal de alta g';

  @override
  String get measHgOff => 'apagado';

  @override
  String get measLive => 'Vista en vivo';

  @override
  String get liveOff => 'apagada';

  @override
  String get liveQuat50 => 'cuaterniones 50 Hz';

  @override
  String get liveQuat100 => 'cuaterniones 100 Hz';

  @override
  String get liveRaw50 => 'pasos en bruto 50 Hz';

  @override
  String get liveRaw100 => 'pasos en bruto 100 Hz';

  @override
  String get measSyncFirst => 'Sincronía: la sesión aún no se ha enviado';

  @override
  String measInStep(int n, int all) {
    return 'Sincronía: $n de $all elegidas en sincronía';
  }

  @override
  String get measSync => 'Sincronía';

  @override
  String get measStart => 'INICIAR';

  @override
  String get measLiveOnly => 'En vivo';

  @override
  String get measNoCal => 'sin calibración';

  @override
  String get measExpired => 'calibración caducada';

  @override
  String get measUnreachable => 'fuera de alcance';

  @override
  String measTempApart(int deg) {
    return '$deg °C de su calibración';
  }

  @override
  String get setMeasurement => 'Medición y calibración';

  @override
  String get setCalValid => 'validez de la calibración';

  @override
  String get calEvery => 'cada medición';

  @override
  String get cal2h => '2 h';

  @override
  String get calDay => 'un día';

  @override
  String get cal30 => '30 días';

  @override
  String get calUnlimited => 'sin límite';

  @override
  String get setTempApart => 'avisar de una temperatura distinta en';

  @override
  String get setCurves => 'ventana de las curvas';

  @override
  String get setNumbers => 'los números de los últimos';

  @override
  String strides(int n) {
    return '$n pasos';
  }

  @override
  String get setLost => 'Se perdió la conexión con Povodok';

  @override
  String get setPower => 'potencia de la radio';

  @override
  String get powerFull => 'plena, dentro de las normas';

  @override
  String get setLight => 'luz durante la medición';

  @override
  String get lightUsual => 'normal';

  @override
  String get lightFaults => 'solo fallos';

  @override
  String get lightOff => 'apagada';

  @override
  String get setToCome =>
      'se enviará a las unidades cuando su firmware lo admita';

  @override
  String get setLocked => 'Se cambia solo fuera de la medición.';

  @override
  String get dataPhone => 'En el teléfono';

  @override
  String get dataUnits => 'En las unidades';

  @override
  String get dataNoFiles => 'Aún no hay nada en el teléfono.';

  @override
  String get dataNoSessions => 'No hay sesiones en las unidades.';

  @override
  String get dataGot => 'en el teléfono ✓';

  @override
  String dataSwitchOn(String units) {
    return 'enciende $units';
  }

  @override
  String get dataLarge => 'grande: base o cable';

  @override
  String get dataReady => 'lista';

  @override
  String get dataToCsv => 'A CSV';

  @override
  String get dataToC3d => 'A C3D';

  @override
  String dataAsk(int n) {
    return '¿Descargarla de $n unidad(es) en un solo C3D?';
  }

  @override
  String get dataCancel => 'Cancelar';

  @override
  String get dataDownload => 'Descargar';

  @override
  String get ovPovodoks => 'Povodoks';

  @override
  String get ovToPhone => 'al teléfono';

  @override
  String get ovHeard => 'se oye';

  @override
  String get ovNotHeard => 'no se oye';

  @override
  String get ovCode => 'código';

  @override
  String get ovUnit => 'unidad';

  @override
  String get ovState => 'estado';

  @override
  String get ovCell => 'batería';

  @override
  String get ovCard => 'tarjeta';

  @override
  String get ovHeardAgo => 'oída';

  @override
  String get ovFault => 'fallo';

  @override
  String get ovFlash => 'flash';

  @override
  String get stRecording => 'grabando';

  @override
  String get stAlone => 'grabando sola';

  @override
  String get stSearching => 'grabando, buscando';

  @override
  String get stPaused => 'en pausa';

  @override
  String get stWaiting => 'esperando';

  @override
  String get stCalibrating => 'calibrando';

  @override
  String get stGiving => 'dando su registro';

  @override
  String get stFirmware => 'recibiendo firmware';

  @override
  String get stErasing => 'borrando';

  @override
  String get fNoCard => 'sin tarjeta';

  @override
  String get fCardFull => 'tarjeta llena';

  @override
  String get fWrite => 'falló la escritura';

  @override
  String get fFs => 'sistema de archivos ilegible';

  @override
  String get fDropped => 'bloques perdidos';

  @override
  String get fSensor => 'sensor mudo';

  @override
  String get fClock => 'reloj de reserva';

  @override
  String get fCellLow => 'batería baja';

  @override
  String get fCharger => 'fallo del cargador';

  @override
  String get fPmic => 'PMIC mudo';

  @override
  String get fNoCal => 'sin calibración';

  @override
  String get fRefused => 'ajustes rechazados';

  @override
  String get uKind => 'tipo, número';

  @override
  String get uChip => 'chip';

  @override
  String get uSensor => 'sensor';

  @override
  String get uFirmware => 'firmware';

  @override
  String get uTemp => 'temperatura';

  @override
  String get uCal => 'calibración';

  @override
  String get uWith => 'Con ella';

  @override
  String get uBlink => 'Parpadear 10 s';

  @override
  String get uErase => 'Borrar la tarjeta… (contraseña)';

  @override
  String get uForget => 'Olvidar… (contraseña)';

  @override
  String get uOff => 'Apagar';

  @override
  String get uPassword => 'La contraseña otra vez';

  @override
  String get uForgotten =>
      'Olvidada, la unidad deja el equipo y conserva sus registros.';

  @override
  String get uDone => 'Hecho.';

  @override
  String get uNotNow =>
      'Ahora no: el equipo graba, o el Povodok está en marcha.';

  @override
  String calOnto(String face) {
    return 'Todas las unidades sobre la cara $face, luego toca Confirmar.';
  }

  @override
  String calTurn(String axis, String face) {
    return 'Gira todas las unidades sobre su eje $axis hasta la cara $face, luego toca Confirmar.';
  }

  @override
  String get calConfirm => 'Confirmar';

  @override
  String get calMeasuring => 'Quietas: 5 s…';

  @override
  String get calTaken => 'quieta ✓';

  @override
  String get calMoved => 'se movió ✗, otra vez';

  @override
  String get calOtherFace => 'otra cara ✗, otra vez';

  @override
  String get calDone => 'Listo: cada unidad guarda su nueva calibración.';

  @override
  String calAge(int days) {
    String _temp0 = intl.Intl.pluralLogic(
      days,
      locale: localeName,
      other: '$days días',
      one: '1 día',
      zero: 'hoy',
    );
    return '$_temp0';
  }

  @override
  String get dlStarting => 'Empezando…';

  @override
  String dlDone(String path) {
    return 'Listo: $path';
  }

  @override
  String dlOf(int got, int all) {
    return '$got de $all kB';
  }

  @override
  String dlLeft(int s) {
    return 'quedan $s s';
  }

  @override
  String get dlLeftOut => 'omitida: no alcanzada, o falta su archivo';

  @override
  String get actStop => 'Detener';

  @override
  String get liveSave => 'Guardar';

  @override
  String get liveSaveStop => 'Dejar de guardar';

  @override
  String liveSaving(Object time, Object mb) {
    return 'guardando $time · $mb MB';
  }

  @override
  String get actContinue => 'Continuar';

  @override
  String get setToWork => 'A trabajar';

  @override
  String get setNoLive => 'Aún no hay vista en vivo';

  @override
  String get diagBand => 'La banda: ruido por canal, dBm';

  @override
  String diagSet(Object ch) {
    return 'el conjunto $ch';
  }

  @override
  String diagSpare(Object ch) {
    return 'reserva $ch';
  }

  @override
  String diagMargin(Object k, Object db) {
    return 'Povodok $k: margen $db dB';
  }

  @override
  String get diagUnits => 'Unidades';

  @override
  String diagHeard(Object s) {
    return 'oída hace $s s';
  }

  @override
  String get diagNotHeard => 'no oída';

  @override
  String diagSync(Object dbm, Object missed) {
    return 'SYNC $dbm dBm, $missed perdidos';
  }

  @override
  String diagClock(Object ns) {
    return 'reloj $ns ns';
  }

  @override
  String diagOut(Object acc, Object gyro) {
    return 'fuera de rango $acc/$gyro';
  }

  @override
  String diagCell(Object pct) {
    return 'batería $pct %';
  }

  @override
  String diagCard(Object free, Object dropped) {
    return 'tarjeta $free % libre, $dropped descartados';
  }

  @override
  String get liveCurves => 'Curvas';

  @override
  String get liveNumbers => 'Números';

  @override
  String get liveAccel => 'aceleración vertical, g';

  @override
  String get liveTilt => 'inclinación respecto a la vertical, °';

  @override
  String get liveRange => 'rango';

  @override
  String get liveStrides => 'zancadas';

  @override
  String liveLastStrides(Object n) {
    return 'las últimas $n zancadas';
  }

  @override
  String get liveNoRhythm => 'aún sin zancada';

  @override
  String get liveLeftRight => 'Izquierda frente a derecha';

  @override
  String get liveDifference => 'diferencia';

  @override
  String get placesTitle => 'Plantillas de lugares';

  @override
  String get placesTemplate => 'Plantilla';

  @override
  String get placesAnimal => 'Nombre del animal';

  @override
  String get placesNote =>
      'El código muestra la unidad en todas partes; la tabla del conjunto va con la sesión al archivo de cada unidad.';

  @override
  String get animalHorse => 'caballo';

  @override
  String get animalDog => 'perro';

  @override
  String get animalPerson => 'persona';

  @override
  String get placeHead => 'cabeza';

  @override
  String get placeWithers => 'cruz';

  @override
  String get placeThoracic => 'columna torácica';

  @override
  String get placeLumbar => 'columna lumbar';

  @override
  String get placePelvis => 'pelvis';

  @override
  String get placeLeftFore => 'anterior izquierda';

  @override
  String get placeRightFore => 'anterior derecha';

  @override
  String get placeLeftHind => 'posterior izquierda';

  @override
  String get placeRightHind => 'posterior derecha';

  @override
  String get placeLeftShoulder => 'hombro izquierdo';

  @override
  String get placeRightShoulder => 'hombro derecho';

  @override
  String get placeLeftElbow => 'codo izquierdo';

  @override
  String get placeRightElbow => 'codo derecho';

  @override
  String get placeLeftHip => 'cadera izquierda';

  @override
  String get placeRightHip => 'cadera derecha';

  @override
  String get animalCow => 'vaca';

  @override
  String get animalCat => 'gato';

  @override
  String get setNewUnits => 'Unidades nuevas';

  @override
  String get fwTitle => 'Firmware';

  @override
  String fwReady(String version) {
    return 'Firmware $version en el teléfono, firmado y comprobado';
  }

  @override
  String get fwNone => 'Aún no hay firmware en el teléfono.';

  @override
  String get fwFetch => 'Descargar de GitHub';

  @override
  String fwFetched(String version) {
    return 'Descargado: $version';
  }

  @override
  String get fwNotPublished => 'Aún no hay firmware publicado.';

  @override
  String get fwUpdateAll => 'Actualizar todo';

  @override
  String get fwWaiting => 'en espera';

  @override
  String get fwTaking => 'recibiendo';

  @override
  String get fwDone => 'hecho ✓';

  @override
  String get fwCurrent => 'al día ✓';

  @override
  String get fwOff => 'apagada: más tarde';

  @override
  String get fwFailed => 'falló, otra vez';

  @override
  String get fwHow =>
      'Por SMP a través del Povodok, una unidad tras otra, el Povodok el último; cada unidad comprueba la firma de la imagen y conserva la antigua si la nueva no arranca; una apagada la recibe cuando esté encendida.';

  @override
  String get fwPovodokAgain =>
      'El Povodok vuelve a arrancar con el firmware nuevo: únete otra vez.';

  @override
  String get setNewSet => 'Un equipo nuevo';

  @override
  String get nsIntro =>
      'Un equipo nuevo — su dirección, su clave y su canal — dado al Povodok, que vuelve a empezar en él. Las unidades entran luego por la búsqueda, una Belka también por el archivo en su cable.';

  @override
  String get nsChannel => 'Canal';

  @override
  String get nsNotMeasured => 'sin medir';

  @override
  String get nsQuietest => 'el más tranquilo';

  @override
  String get nsLabel => 'El número propio del equipo, 1–9999, si lo hay';

  @override
  String get nsDraw => 'Sortear un equipo nuevo';

  @override
  String nsDrawn(String address, Object ch) {
    return 'El equipo: $address, canal $ch';
  }

  @override
  String get nsPovodok => 'El número de este Povodok en el equipo';

  @override
  String get nsPassword => 'La contraseña del Povodok, otra vez';

  @override
  String get nsGive => 'Darlo a este Povodok';

  @override
  String get nsAsk =>
      'El Povodok deja su equipo por el nuevo y vuelve a empezar; las unidades del antiguo quedan fuera hasta recibir el nuevo.';

  @override
  String get nsGiven =>
      'El Povodok toma el equipo nuevo y vuelve a empezar: únete otra vez y busca las unidades.';

  @override
  String get nsFileFor => 'STRELKA.SET para la Belka número';

  @override
  String get nsSave => 'Guardar';

  @override
  String nsSaved(String path) {
    return 'Guardado: $path — copiado al disco de la Belka en su cable, se toma al quitar el cable.';
  }

  @override
  String get newTitle => 'Unidades nuevas: la búsqueda';

  @override
  String get newSearching =>
      'Povodok llama; una unidad nueva responde en su ventana.';

  @override
  String get newNone => 'Aún no se oye ninguna unidad nueva.';

  @override
  String newTake(Object n) {
    return 'Admitir como $n';
  }

  @override
  String newTaken(Object n) {
    return 'admitida como $n';
  }

  @override
  String get newCable =>
      'Una Belka con su cable recibe el conjunto más rápido, el archivo STRELKA.SET en su disco.';

  @override
  String get pwTitle => 'Contraseña';

  @override
  String get pwOld => 'La contraseña actual';

  @override
  String get pwNew => 'La contraseña nueva, al menos 12 caracteres';

  @override
  String get pwAgain => 'La contraseña nueva otra vez';

  @override
  String get pwShort => 'Al menos 12 caracteres.';

  @override
  String get pwMismatch => 'No coinciden.';

  @override
  String get pwChange => 'Cambiar';

  @override
  String get pwChanged =>
      'Contraseña cambiada: vuelve a conectar con la nueva.';

  @override
  String get offTitle => 'Apagar Povodok';

  @override
  String get offAsk =>
      '¿Apagar Povodok? Si graba, primero detiene el conjunto.';

  @override
  String keyNew(Object fp) {
    return 'Clave de Povodok guardada: $fp';
  }

  @override
  String get keyChangedTitle => 'Otra clave';

  @override
  String keyChanged(Object now, Object old) {
    return 'Povodok responde con otra clave ($now) distinta de la guardada ($old). Tras sus ajustes de fábrica es correcto; si no, responde otro. ¿Tomar la clave nueva?';
  }

  @override
  String get keyTake => 'Tomarla';

  @override
  String get keyFingerprint => 'Clave de Povodok';

  @override
  String get pwFactory =>
      'La contraseña de fábrica: da una nueva, al menos 12 caracteres.';

  @override
  String diagMove(Object ch) {
    return 'Mover el conjunto al $ch';
  }
}
