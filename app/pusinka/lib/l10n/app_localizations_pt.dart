// ignore: unused_import
import 'package:intl/intl.dart' as intl;
import 'app_localizations.dart';

// ignore_for_file: type=lint

/// The translations for Portuguese (`pt`).
class AppLocalizationsPt extends AppLocalizations {
  AppLocalizationsPt([String locale = 'pt']) : super(locale);

  @override
  String get menuOverview => 'Visão geral';

  @override
  String get menuCalibration => 'Calibração';

  @override
  String get menuMeasurement => 'Medição';

  @override
  String get menuData => 'Dados';

  @override
  String get menuDiagnostics => 'Diagnóstico';

  @override
  String get menuSettings => 'Configurações';

  @override
  String get joinAddress => 'Endereço do Povodok';

  @override
  String get joinPassword => 'Senha';

  @override
  String get joinButton => 'Conectar';

  @override
  String get joinJoining => 'Conectando…';

  @override
  String get joinRefused => 'O Povodok recusou a senha.';

  @override
  String joinNoAnswer(String message) {
    return 'O Povodok não responde: $message';
  }

  @override
  String get joinTimeout => 'O Povodok não respondeu a tempo.';

  @override
  String get settingsLanguage => 'Idioma';

  @override
  String get settingsAbout => 'Sobre';

  @override
  String get aboutPicture => 'Pušinka, a filhote de Strelka, em desenho.';

  @override
  String get aboutOpen => 'Aberto, para qualquer um montar; nada é vendido.';

  @override
  String get overviewUnits => 'Unidades do conjunto';

  @override
  String get overviewNone => 'Nenhuma unidade ouvida ainda.';

  @override
  String measSession(int s) {
    return 'Sessão $s';
  }

  @override
  String get measChoose => 'Unidades: escolha com um toque';

  @override
  String get measNote => 'Nota: o lugar, o animal, o que aconteceu';

  @override
  String get measNone => 'Nenhuma unidade do conjunto ouvida ainda.';

  @override
  String measChosen(int chosen, int all) {
    return 'Escolhidas $chosen de $all';
  }

  @override
  String get measRate => 'Frequência';

  @override
  String get measAcc => 'Acelerômetro';

  @override
  String get measGyro => 'Giroscópio';

  @override
  String get measHg => 'Canal de alto g';

  @override
  String get measHgOff => 'desligado';

  @override
  String get measLive => 'Visão ao vivo';

  @override
  String get liveOff => 'desligada';

  @override
  String get liveQuat50 => 'quatérnios 50 Hz';

  @override
  String get liveQuat100 => 'quatérnios 100 Hz';

  @override
  String get liveRaw50 => 'passos brutos 50 Hz';

  @override
  String get liveRaw100 => 'passos brutos 100 Hz';

  @override
  String get measSyncFirst => 'Sincronia: a sessão ainda não foi enviada';

  @override
  String measInStep(int n, int all) {
    return 'Sincronia: $n de $all escolhidas em sincronia';
  }

  @override
  String get measSync => 'Sincronia';

  @override
  String get measStart => 'INICIAR';

  @override
  String get measLiveOnly => 'Ao vivo';

  @override
  String get measNoCal => 'sem calibração';

  @override
  String get measExpired => 'calibração vencida';

  @override
  String get measUnreachable => 'fora de alcance';

  @override
  String measTempApart(int deg) {
    return '$deg °C da calibração';
  }

  @override
  String get setMeasurement => 'Medição e calibração';

  @override
  String get setCalValid => 'validade da calibração';

  @override
  String get calEvery => 'cada medição';

  @override
  String get cal2h => '2 h';

  @override
  String get calDay => 'um dia';

  @override
  String get cal30 => '30 dias';

  @override
  String get calUnlimited => 'sem limite';

  @override
  String get setTempApart => 'avisar de temperatura diferente em';

  @override
  String get setCurves => 'janela das curvas';

  @override
  String get setNumbers => 'os números dos últimos';

  @override
  String strides(int n) {
    return '$n passos';
  }

  @override
  String get setLost => 'A conexão com o Povodok foi perdida';

  @override
  String get setPower => 'potência do rádio';

  @override
  String get powerFull => 'plena, dentro das regras';

  @override
  String get setLight => 'luz durante a medição';

  @override
  String get lightUsual => 'normal';

  @override
  String get lightFaults => 'só falhas';

  @override
  String get lightOff => 'desligada';

  @override
  String get setToCome =>
      'será enviado às unidades quando o firmware delas permitir';

  @override
  String get setLocked => 'Muda-se só fora da medição.';

  @override
  String get dataPhone => 'No telefone';

  @override
  String get dataUnits => 'Nas unidades';

  @override
  String get dataNoFiles => 'Ainda não há nada no telefone.';

  @override
  String get dataNoSessions => 'Nenhuma sessão nas unidades.';

  @override
  String get dataGot => 'no telefone ✓';

  @override
  String dataSwitchOn(String units) {
    return 'ligue $units';
  }

  @override
  String get dataLarge => 'grande: base ou cabo';

  @override
  String get dataReady => 'pronta';

  @override
  String get dataToCsv => 'Para CSV';

  @override
  String get dataToC3d => 'Para C3D';

  @override
  String dataAsk(int n) {
    return 'Baixar de $n unidade(s) num só C3D?';
  }

  @override
  String get dataCancel => 'Cancelar';

  @override
  String get dataDownload => 'Baixar';

  @override
  String get ovPovodoks => 'Povodoks';

  @override
  String get ovToPhone => 'ao telefone';

  @override
  String get ovHeard => 'ouvido';

  @override
  String get ovNotHeard => 'não ouvido';

  @override
  String get ovCode => 'código';

  @override
  String get ovUnit => 'unidade';

  @override
  String get ovState => 'estado';

  @override
  String get ovCell => 'bateria';

  @override
  String get ovCard => 'cartão';

  @override
  String get ovHeardAgo => 'ouvida';

  @override
  String get ovFault => 'falha';

  @override
  String get ovFlash => 'flash';

  @override
  String get stRecording => 'gravando';

  @override
  String get stAlone => 'gravando sozinha';

  @override
  String get stSearching => 'gravando, buscando';

  @override
  String get stPaused => 'pausada';

  @override
  String get stWaiting => 'aguardando';

  @override
  String get stCalibrating => 'calibrando';

  @override
  String get stGiving => 'entregando o registro';

  @override
  String get stFirmware => 'recebendo firmware';

  @override
  String get stErasing => 'apagando';

  @override
  String get fNoCard => 'sem cartão';

  @override
  String get fCardFull => 'cartão cheio';

  @override
  String get fWrite => 'falha na gravação';

  @override
  String get fFs => 'sistema de arquivos ilegível';

  @override
  String get fDropped => 'blocos perdidos';

  @override
  String get fSensor => 'sensor mudo';

  @override
  String get fClock => 'relógio de reserva';

  @override
  String get fCellLow => 'bateria fraca';

  @override
  String get fCharger => 'falha do carregador';

  @override
  String get fPmic => 'PMIC mudo';

  @override
  String get fNoCal => 'sem calibração';

  @override
  String get fRefused => 'configurações recusadas';

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
  String get uCal => 'calibração';

  @override
  String get uWith => 'Com ela';

  @override
  String get uBlink => 'Piscar 10 s';

  @override
  String get uErase => 'Apagar o cartão… (senha)';

  @override
  String get uForget => 'Esquecer… (senha)';

  @override
  String get uOff => 'Desligar';

  @override
  String get uPassword => 'A senha de novo';

  @override
  String get uForgotten =>
      'Esquecida, a unidade sai do conjunto e guarda seus registros.';

  @override
  String get uDone => 'Feito.';

  @override
  String get uNotNow =>
      'Agora não: o conjunto grava, ou o Povodok está em trabalho.';

  @override
  String calOnto(String face) {
    return 'Todas as unidades na face $face, depois toque em Confirmar.';
  }

  @override
  String calTurn(String axis, String face) {
    return 'Gire todas as unidades em torno do eixo $axis até a face $face, depois toque em Confirmar.';
  }

  @override
  String get calConfirm => 'Confirmar';

  @override
  String get calMeasuring => 'Paradas: 5 s…';

  @override
  String get calTaken => 'parada ✓';

  @override
  String get calMoved => 'mexeu ✗, de novo';

  @override
  String get calOtherFace => 'outra face ✗, de novo';

  @override
  String get calDone => 'Pronto: cada unidade guarda sua nova calibração.';

  @override
  String calAge(int days) {
    String _temp0 = intl.Intl.pluralLogic(
      days,
      locale: localeName,
      other: '$days dias',
      one: '1 dia',
      zero: 'hoje',
    );
    return '$_temp0';
  }

  @override
  String get dlStarting => 'Iniciando…';

  @override
  String dlDone(String path) {
    return 'Pronto: $path';
  }

  @override
  String dlOf(int got, int all) {
    return '$got de $all kB';
  }

  @override
  String dlLeft(int s) {
    return 'faltam $s s';
  }

  @override
  String get dlLeftOut =>
      'deixada de fora: não alcançada, ou falta seu arquivo';

  @override
  String get actStop => 'Parar';

  @override
  String get liveSave => 'Salvar';

  @override
  String get liveSaveStop => 'Parar de salvar';

  @override
  String liveSaving(Object time, Object mb) {
    return 'salvando $time · $mb MB';
  }

  @override
  String get actContinue => 'Continuar';

  @override
  String get setToWork => 'Ao trabalho';

  @override
  String get setNoLive => 'Ainda sem visão ao vivo';

  @override
  String get diagBand => 'A banda: ruído por canal, dBm';

  @override
  String diagSet(Object ch) {
    return 'o conjunto $ch';
  }

  @override
  String diagSpare(Object ch) {
    return 'reserva $ch';
  }

  @override
  String diagMargin(Object k, Object db) {
    return 'Povodok $k: margem $db dB';
  }

  @override
  String get diagUnits => 'Unidades';

  @override
  String diagHeard(Object s) {
    return 'ouvida há $s s';
  }

  @override
  String get diagNotHeard => 'não ouvida';

  @override
  String diagSync(Object dbm, Object missed) {
    return 'SYNC $dbm dBm, $missed perdidos';
  }

  @override
  String diagClock(Object ns) {
    return 'relógio $ns ns';
  }

  @override
  String diagOut(Object acc, Object gyro) {
    return 'fora da faixa $acc/$gyro';
  }

  @override
  String diagCell(Object pct) {
    return 'bateria $pct %';
  }

  @override
  String diagCard(Object free, Object dropped) {
    return 'cartão $free % livre, $dropped descartados';
  }

  @override
  String get liveCurves => 'Curvas';

  @override
  String get liveNumbers => 'Números';

  @override
  String get liveAccel => 'aceleração vertical, g';

  @override
  String get liveTilt => 'inclinação em relação à vertical, °';

  @override
  String get liveRange => 'amplitude';

  @override
  String get liveStrides => 'passadas';

  @override
  String liveLastStrides(Object n) {
    return 'as últimas $n passadas';
  }

  @override
  String get liveNoRhythm => 'ainda sem passada';

  @override
  String get liveLeftRight => 'Esquerda contra direita';

  @override
  String get liveDifference => 'diferença';

  @override
  String get placesTitle => 'Modelos de locais';

  @override
  String get placesTemplate => 'Modelo';

  @override
  String get placesAnimal => 'Nome do animal';

  @override
  String get placesNote =>
      'O código mostra a unidade em todo lugar; a tabela do conjunto vai com a sessão para o arquivo de cada unidade.';

  @override
  String get animalHorse => 'cavalo';

  @override
  String get animalDog => 'cão';

  @override
  String get animalPerson => 'pessoa';

  @override
  String get placeHead => 'cabeça';

  @override
  String get placeWithers => 'cernelha';

  @override
  String get placeThoracic => 'coluna torácica';

  @override
  String get placeLumbar => 'coluna lombar';

  @override
  String get placePelvis => 'pelve';

  @override
  String get placeLeftFore => 'anterior esquerdo';

  @override
  String get placeRightFore => 'anterior direito';

  @override
  String get placeLeftHind => 'posterior esquerdo';

  @override
  String get placeRightHind => 'posterior direito';

  @override
  String get placeLeftShoulder => 'ombro esquerdo';

  @override
  String get placeRightShoulder => 'ombro direito';

  @override
  String get placeLeftElbow => 'cotovelo esquerdo';

  @override
  String get placeRightElbow => 'cotovelo direito';

  @override
  String get placeLeftHip => 'quadril esquerdo';

  @override
  String get placeRightHip => 'quadril direito';

  @override
  String get animalCow => 'vaca';

  @override
  String get animalCat => 'gato';

  @override
  String get setNewUnits => 'Unidades novas';

  @override
  String get fwTitle => 'Firmware';

  @override
  String fwReady(String version) {
    return 'Firmware $version no telefone, assinado e verificado';
  }

  @override
  String get fwNone => 'Ainda não há firmware no telefone.';

  @override
  String get fwFetch => 'Baixar do GitHub';

  @override
  String fwFetched(String version) {
    return 'Baixado: $version';
  }

  @override
  String get fwNotPublished => 'Ainda não há firmware publicado.';

  @override
  String get fwUpdateAll => 'Atualizar tudo';

  @override
  String get fwWaiting => 'aguardando';

  @override
  String get fwTaking => 'recebendo';

  @override
  String get fwDone => 'feito ✓';

  @override
  String get fwCurrent => 'atualizado ✓';

  @override
  String get fwOff => 'desligada: depois';

  @override
  String get fwFailed => 'falhou, de novo';

  @override
  String get fwHow =>
      'Por SMP através do Povodok, uma unidade de cada vez, o Povodok por último; cada unidade verifica a assinatura da imagem e mantém a antiga se a nova não arrancar; uma desligada recebe-a quando estiver ligada.';

  @override
  String get fwPovodokAgain =>
      'O Povodok recomeça com o firmware novo: junte-se de novo.';

  @override
  String get setNewSet => 'Um novo conjunto';

  @override
  String get nsIntro =>
      'Um novo conjunto — seu endereço, sua chave e seu canal — dado ao Povodok, que recomeça nele. As unidades entram depois pela busca, uma Belka também pelo arquivo no seu cabo.';

  @override
  String get nsChannel => 'Canal';

  @override
  String get nsNotMeasured => 'não medido';

  @override
  String get nsQuietest => 'o mais silencioso';

  @override
  String get nsLabel => 'O número próprio do conjunto, 1–9999, se houver';

  @override
  String get nsDraw => 'Sortear um novo conjunto';

  @override
  String nsDrawn(String address, Object ch) {
    return 'O conjunto: $address, canal $ch';
  }

  @override
  String get nsPovodok => 'O número deste Povodok no conjunto';

  @override
  String get nsPassword => 'A senha do Povodok, de novo';

  @override
  String get nsGive => 'Dá-lo a este Povodok';

  @override
  String get nsAsk =>
      'O Povodok deixa seu conjunto pelo novo e recomeça; as unidades do antigo ficam de fora até receberem o novo.';

  @override
  String get nsGiven =>
      'O Povodok assume o novo conjunto e recomeça: junte-se de novo e procure as unidades.';

  @override
  String get nsFileFor => 'STRELKA.SET para a Belka número';

  @override
  String get nsSave => 'Salvar';

  @override
  String nsSaved(String path) {
    return 'Salvo: $path — copiado para o disco da Belka no cabo, é tomado quando o cabo sai.';
  }

  @override
  String get newTitle => 'Unidades novas: a busca';

  @override
  String get newSearching =>
      'O Povodok chama; uma unidade nova responde na sua janela.';

  @override
  String get newNone => 'Nenhuma unidade nova ouvida ainda.';

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
      'Uma Belka no cabo recebe o conjunto mais rápido, o arquivo STRELKA.SET no seu disco.';

  @override
  String get pwTitle => 'Senha';

  @override
  String get pwOld => 'A senha atual';

  @override
  String get pwNew => 'A nova senha, pelo menos 12 caracteres';

  @override
  String get pwAgain => 'A nova senha de novo';

  @override
  String get pwShort => 'Pelo menos 12 caracteres.';

  @override
  String get pwMismatch => 'As duas não coincidem.';

  @override
  String get pwChange => 'Alterar';

  @override
  String get pwChanged => 'Senha alterada: conecte de novo com a nova.';

  @override
  String get offTitle => 'Desligar o Povodok';

  @override
  String get offAsk =>
      'Desligar o Povodok? Se estiver gravando, ele para o conjunto primeiro.';

  @override
  String keyNew(Object fp) {
    return 'Chave do Povodok guardada: $fp';
  }

  @override
  String get keyChangedTitle => 'Outra chave';

  @override
  String keyChanged(Object now, Object old) {
    return 'O Povodok responde com outra chave ($now) que não a guardada ($old). Após suas configurações de fábrica está certo; senão, outro responde. Aceitar a nova chave?';
  }

  @override
  String get keyTake => 'Aceitar';

  @override
  String get keyFingerprint => 'Chave do Povodok';

  @override
  String get pwFactory =>
      'A senha de fábrica: dê uma nova, pelo menos 12 caracteres.';

  @override
  String diagMove(Object ch) {
    return 'Mover o conjunto para $ch';
  }
}
