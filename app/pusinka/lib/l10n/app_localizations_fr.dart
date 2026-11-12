// ignore: unused_import
import 'package:intl/intl.dart' as intl;
import 'app_localizations.dart';

// ignore_for_file: type=lint

/// The translations for French (`fr`).
class AppLocalizationsFr extends AppLocalizations {
  AppLocalizationsFr([String locale = 'fr']) : super(locale);

  @override
  String get menuOverview => 'Aperçu';

  @override
  String get menuCalibration => 'Étalonnage';

  @override
  String get menuMeasurement => 'Mesure';

  @override
  String get menuData => 'Données';

  @override
  String get menuDiagnostics => 'Diagnostic';

  @override
  String get menuSettings => 'Réglages';

  @override
  String get joinAddress => 'Adresse du Povodok';

  @override
  String get joinPassword => 'Mot de passe';

  @override
  String get joinButton => 'Se connecter';

  @override
  String get joinJoining => 'Connexion…';

  @override
  String get joinRefused => 'Le Povodok a refusé le mot de passe.';

  @override
  String joinNoAnswer(String message) {
    return 'Le Povodok ne répond pas : $message';
  }

  @override
  String get joinTimeout => 'Le Povodok n’a pas répondu à temps.';

  @override
  String get settingsLanguage => 'Langue';

  @override
  String get settingsAbout => 'À propos';

  @override
  String get aboutPicture => 'Pušinka, le chiot de Strelka, en dessin.';

  @override
  String get aboutOpen =>
      'Ouvert, chacun peut le construire ; rien n’est vendu.';

  @override
  String get overviewUnits => 'Unités de l\'ensemble';

  @override
  String get overviewNone => 'Aucune unité entendue pour l’instant.';

  @override
  String measSession(int s) {
    return 'Séance $s';
  }

  @override
  String get measChoose => 'Unités : choisir d’un toucher';

  @override
  String get measNote => 'Note : le lieu, l’animal, ce qui s’est passé';

  @override
  String get measNone => 'Aucune unité de l’ensemble entendue pour l’instant.';

  @override
  String measChosen(int chosen, int all) {
    return 'Choisies $chosen sur $all';
  }

  @override
  String get measRate => 'Fréquence';

  @override
  String get measAcc => 'Accéléromètre';

  @override
  String get measGyro => 'Gyroscope';

  @override
  String get measHg => 'Canal haut g';

  @override
  String get measHgOff => 'éteint';

  @override
  String get measLive => 'Vue en direct';

  @override
  String get liveOff => 'éteinte';

  @override
  String get liveQuat50 => 'quaternions 50 Hz';

  @override
  String get liveQuat100 => 'quaternions 100 Hz';

  @override
  String get liveRaw50 => 'pas bruts 50 Hz';

  @override
  String get liveRaw100 => 'pas bruts 100 Hz';

  @override
  String get measSyncFirst => 'Synchro : la séance pas encore envoyée';

  @override
  String measInStep(int n, int all) {
    return 'Synchro : $n sur $all choisies en phase';
  }

  @override
  String get measSync => 'Synchro';

  @override
  String get measStart => 'DÉMARRER';

  @override
  String get measLiveOnly => 'Direct';

  @override
  String get measNoCal => 'sans étalonnage';

  @override
  String get measExpired => 'étalonnage périmé';

  @override
  String get measUnreachable => 'hors de portée';

  @override
  String measTempApart(int deg) {
    return '$deg °C de son étalonnage';
  }

  @override
  String get setMeasurement => 'Mesure et étalonnage';

  @override
  String get setCalValid => 'validité de l’étalonnage';

  @override
  String get calEvery => 'chaque mesure';

  @override
  String get cal2h => '2 h';

  @override
  String get calDay => 'un jour';

  @override
  String get cal30 => '30 jours';

  @override
  String get calUnlimited => 'sans limite';

  @override
  String get setTempApart => 'avertir d’une température différente de';

  @override
  String get setCurves => 'fenêtre des courbes';

  @override
  String get setNumbers => 'les nombres des derniers';

  @override
  String strides(int n) {
    return '$n pas';
  }

  @override
  String get setLost => 'La connexion à Povodok est perdue';

  @override
  String get setPower => 'puissance radio';

  @override
  String get powerFull => 'pleine, dans les règles';

  @override
  String get setLight => 'lumière pendant la mesure';

  @override
  String get lightUsual => 'comme d’habitude';

  @override
  String get lightFaults => 'pannes seules';

  @override
  String get lightOff => 'éteinte';

  @override
  String get setToCome =>
      'envoyé aux unités dès que leur firmware le permettra';

  @override
  String get setLocked => 'Ne se change qu’hors de la mesure.';

  @override
  String get dataPhone => 'Sur le téléphone';

  @override
  String get dataUnits => 'Sur les unités';

  @override
  String get dataNoFiles => 'Rien sur le téléphone pour l’instant.';

  @override
  String get dataNoSessions => 'Aucune séance sur les unités.';

  @override
  String get dataGot => 'sur le téléphone ✓';

  @override
  String dataSwitchOn(String units) {
    return 'allume $units';
  }

  @override
  String get dataLarge => 'grande : socle ou câble';

  @override
  String get dataReady => 'prête';

  @override
  String get dataToCsv => 'En CSV';

  @override
  String get dataToC3d => 'En C3D';

  @override
  String dataAsk(int n) {
    return 'La télécharger de $n unité(s) en un seul C3D ?';
  }

  @override
  String get dataCancel => 'Annuler';

  @override
  String get dataDownload => 'Télécharger';

  @override
  String get ovPovodoks => 'Povodoks';

  @override
  String get ovToPhone => 'au téléphone';

  @override
  String get ovHeard => 'entendu';

  @override
  String get ovNotHeard => 'pas entendu';

  @override
  String get ovCode => 'code';

  @override
  String get ovUnit => 'unité';

  @override
  String get ovState => 'état';

  @override
  String get ovCell => 'batterie';

  @override
  String get ovCard => 'carte';

  @override
  String get ovHeardAgo => 'entendue';

  @override
  String get ovFault => 'panne';

  @override
  String get ovFlash => 'flash';

  @override
  String get stRecording => 'enregistre';

  @override
  String get stAlone => 'enregistre seule';

  @override
  String get stSearching => 'enregistre, cherche';

  @override
  String get stPaused => 'en pause';

  @override
  String get stWaiting => 'attend';

  @override
  String get stCalibrating => 's’étalonne';

  @override
  String get stGiving => 'donne son enregistrement';

  @override
  String get stFirmware => 'reçoit le firmware';

  @override
  String get stErasing => 'efface';

  @override
  String get fNoCard => 'pas de carte';

  @override
  String get fCardFull => 'carte pleine';

  @override
  String get fWrite => 'écriture ratée';

  @override
  String get fFs => 'système de fichiers illisible';

  @override
  String get fDropped => 'blocs perdus';

  @override
  String get fSensor => 'capteur muet';

  @override
  String get fClock => 'horloge de secours';

  @override
  String get fCellLow => 'batterie faible';

  @override
  String get fCharger => 'panne du chargeur';

  @override
  String get fPmic => 'PMIC muet';

  @override
  String get fNoCal => 'sans étalonnage';

  @override
  String get fRefused => 'réglages refusés';

  @override
  String get uKind => 'type, numéro';

  @override
  String get uChip => 'puce';

  @override
  String get uSensor => 'capteur';

  @override
  String get uFirmware => 'firmware';

  @override
  String get uTemp => 'température';

  @override
  String get uCal => 'étalonnage';

  @override
  String get uWith => 'Avec elle';

  @override
  String get uBlink => 'Clignoter 10 s';

  @override
  String get uErase => 'Effacer la carte… (mot de passe)';

  @override
  String get uForget => 'Oublier… (mot de passe)';

  @override
  String get uOff => 'Éteindre';

  @override
  String get uPassword => 'Le mot de passe encore';

  @override
  String get uForgotten =>
      'Oubliée, l’unité quitte l’ensemble et garde ses enregistrements.';

  @override
  String get uDone => 'Fait.';

  @override
  String get uNotNow =>
      'Pas maintenant : l’ensemble enregistre, ou le Povodok est au travail.';

  @override
  String calOnto(String face) {
    return 'Toutes les unités sur la face $face, puis touchez Confirmer.';
  }

  @override
  String calTurn(String axis, String face) {
    return 'Tournez toutes les unités autour de leur axe $axis sur la face $face, puis touchez Confirmer.';
  }

  @override
  String get calConfirm => 'Confirmer';

  @override
  String get calMeasuring => 'Immobiles : 5 s…';

  @override
  String get calTaken => 'immobile ✓';

  @override
  String get calMoved => 'bougée ✗, encore';

  @override
  String get calOtherFace => 'autre face ✗, encore';

  @override
  String get calDone => 'Terminé : chaque unité garde son nouvel étalonnage.';

  @override
  String calAge(int days) {
    String _temp0 = intl.Intl.pluralLogic(
      days,
      locale: localeName,
      other: '$days jours',
      one: '1 jour',
      zero: 'aujourd’hui',
    );
    return '$_temp0';
  }

  @override
  String get dlStarting => 'Démarrage…';

  @override
  String dlDone(String path) {
    return 'Terminé : $path';
  }

  @override
  String dlOf(int got, int all) {
    return '$got sur $all ko';
  }

  @override
  String dlLeft(int s) {
    return 'encore $s s';
  }

  @override
  String get dlLeftOut => 'omise : non atteinte, ou son fichier manque';

  @override
  String get actStop => 'Arrêter';

  @override
  String get liveSave => 'Enregistrer';

  @override
  String get liveSaveStop => 'Arrêter l\'enregistrement';

  @override
  String liveSaving(Object time, Object mb) {
    return 'enregistrement $time · $mb Mo';
  }

  @override
  String get actContinue => 'Continuer';

  @override
  String get setToWork => 'Au travail';

  @override
  String get setNoLive => 'Pas encore de vue en direct';

  @override
  String get diagBand => 'La bande : bruit par canal, dBm';

  @override
  String diagSet(Object ch) {
    return 'l’ensemble $ch';
  }

  @override
  String diagSpare(Object ch) {
    return 'réserve $ch';
  }

  @override
  String diagMargin(Object k, Object db) {
    return 'Povodok $k : marge $db dB';
  }

  @override
  String get diagUnits => 'Unités';

  @override
  String diagHeard(Object s) {
    return 'entendue il y a $s s';
  }

  @override
  String get diagNotHeard => 'pas entendue';

  @override
  String diagSync(Object dbm, Object missed) {
    return 'SYNC $dbm dBm, $missed manqués';
  }

  @override
  String diagClock(Object ns) {
    return 'horloge $ns ns';
  }

  @override
  String diagOut(Object acc, Object gyro) {
    return 'hors plage $acc/$gyro';
  }

  @override
  String diagCell(Object pct) {
    return 'batterie $pct %';
  }

  @override
  String diagCard(Object free, Object dropped) {
    return 'carte $free % libre, $dropped perdus';
  }

  @override
  String get liveCurves => 'Courbes';

  @override
  String get liveNumbers => 'Nombres';

  @override
  String get liveAccel => 'accélération verticale, g';

  @override
  String get liveTilt => 'inclinaison sur la verticale, °';

  @override
  String get liveRange => 'amplitude';

  @override
  String get liveStrides => 'foulées';

  @override
  String liveLastStrides(Object n) {
    return 'les $n dernières foulées';
  }

  @override
  String get liveNoRhythm => 'pas encore de foulée';

  @override
  String get liveLeftRight => 'Gauche contre droite';

  @override
  String get liveDifference => 'différence';

  @override
  String get placesTitle => 'Modèles des emplacements';

  @override
  String get placesTemplate => 'Modèle';

  @override
  String get placesAnimal => 'Nom de l’animal';

  @override
  String get placesNote =>
      'Le code montre l’unité partout ; la table de l’ensemble part avec la session dans le fichier de chaque unité.';

  @override
  String get animalHorse => 'cheval';

  @override
  String get animalDog => 'chien';

  @override
  String get animalPerson => 'personne';

  @override
  String get placeHead => 'tête';

  @override
  String get placeWithers => 'garrot';

  @override
  String get placeThoracic => 'rachis thoracique';

  @override
  String get placeLumbar => 'rachis lombaire';

  @override
  String get placePelvis => 'bassin';

  @override
  String get placeLeftFore => 'antérieur gauche';

  @override
  String get placeRightFore => 'antérieur droit';

  @override
  String get placeLeftHind => 'postérieur gauche';

  @override
  String get placeRightHind => 'postérieur droit';

  @override
  String get placeLeftShoulder => 'épaule gauche';

  @override
  String get placeRightShoulder => 'épaule droite';

  @override
  String get placeLeftElbow => 'coude gauche';

  @override
  String get placeRightElbow => 'coude droit';

  @override
  String get placeLeftHip => 'hanche gauche';

  @override
  String get placeRightHip => 'hanche droite';

  @override
  String get animalCow => 'vache';

  @override
  String get animalCat => 'chat';

  @override
  String get setNewUnits => 'Nouvelles unités';

  @override
  String get fwTitle => 'Micrologiciel';

  @override
  String fwReady(String version) {
    return 'Micrologiciel $version sur le téléphone, signé et vérifié';
  }

  @override
  String get fwNone => 'Pas encore de micrologiciel sur le téléphone.';

  @override
  String get fwFetch => 'Récupérer sur GitHub';

  @override
  String fwFetched(String version) {
    return 'Récupéré : $version';
  }

  @override
  String get fwNotPublished => 'Aucun micrologiciel publié pour l’instant.';

  @override
  String get fwUpdateAll => 'Tout mettre à jour';

  @override
  String get fwWaiting => 'en attente';

  @override
  String get fwTaking => 'le reçoit';

  @override
  String get fwDone => 'fait ✓';

  @override
  String get fwCurrent => 'à jour ✓';

  @override
  String get fwOff => 'éteinte : plus tard';

  @override
  String get fwFailed => 'échec, à nouveau';

  @override
  String get fwHow =>
      'Par SMP via le Povodok, une unité après l’autre, le Povodok en dernier ; chaque unité vérifie la signature de l’image et garde l’ancienne si la nouvelle ne démarre pas ; une unité éteinte la reçoit une fois allumée.';

  @override
  String get fwPovodokAgain =>
      'Le Povodok redémarre avec le nouveau micrologiciel : rejoignez-le à nouveau.';

  @override
  String get setNewSet => 'Un nouvel ensemble';

  @override
  String get nsIntro =>
      'Un nouvel ensemble — son adresse, sa clé et son canal — donné au Povodok, qui redémarre dedans. Les unités y entrent ensuite par la recherche, une Belka aussi par le fichier sur son câble.';

  @override
  String get nsChannel => 'Canal';

  @override
  String get nsNotMeasured => 'non mesuré';

  @override
  String get nsQuietest => 'le plus calme';

  @override
  String get nsLabel => 'Le numéro propre de l’ensemble, 1–9999, s’il en a un';

  @override
  String get nsDraw => 'Tirer un nouvel ensemble';

  @override
  String nsDrawn(String address, Object ch) {
    return 'L’ensemble : $address, canal $ch';
  }

  @override
  String get nsPovodok => 'Le numéro de ce Povodok dans l’ensemble';

  @override
  String get nsPassword => 'Le mot de passe du Povodok, à nouveau';

  @override
  String get nsGive => 'Le donner à ce Povodok';

  @override
  String get nsAsk =>
      'Le Povodok quitte son ensemble pour le nouveau et redémarre ; les unités de l’ancien restent dehors jusqu’à recevoir le nouveau.';

  @override
  String get nsGiven =>
      'Le Povodok prend le nouvel ensemble et redémarre : rejoignez-le à nouveau, puis cherchez les unités.';

  @override
  String get nsFileFor => 'STRELKA.SET pour la Belka numéro';

  @override
  String get nsSave => 'Enregistrer';

  @override
  String nsSaved(String path) {
    return 'Enregistré : $path — copié sur le disque de la Belka sur son câble, il est pris quand le câble part.';
  }

  @override
  String get newTitle => 'Nouvelles unités : la recherche';

  @override
  String get newSearching =>
      'Povodok appelle ; une nouvelle unité répond dans sa fenêtre.';

  @override
  String get newNone => 'Aucune nouvelle unité entendue.';

  @override
  String newTake(Object n) {
    return 'Admettre comme $n';
  }

  @override
  String newTaken(Object n) {
    return 'admise comme $n';
  }

  @override
  String get newCable =>
      'Une Belka sur son câble reçoit l’ensemble plus vite, le fichier STRELKA.SET sur son disque.';

  @override
  String get pwTitle => 'Mot de passe';

  @override
  String get pwOld => 'Le mot de passe actuel';

  @override
  String get pwNew => 'Le nouveau mot de passe, 12 caractères au moins';

  @override
  String get pwAgain => 'Le nouveau mot de passe encore';

  @override
  String get pwShort => '12 caractères au moins.';

  @override
  String get pwMismatch => 'Les deux ne correspondent pas.';

  @override
  String get pwChange => 'Changer';

  @override
  String get pwChanged =>
      'Mot de passe changé : reconnectez-vous avec le nouveau.';

  @override
  String get offTitle => 'Éteindre Povodok';

  @override
  String get offAsk =>
      'Éteindre Povodok ? S’il enregistre, il arrête d’abord l’ensemble.';

  @override
  String keyNew(Object fp) {
    return 'Clé de Povodok gardée : $fp';
  }

  @override
  String get keyChangedTitle => 'Une autre clé';

  @override
  String keyChanged(Object now, Object old) {
    return 'Povodok répond avec une autre clé ($now) que celle gardée ($old). Après ses réglages d’usine c’est juste ; sinon quelqu’un d’autre répond. Prendre la nouvelle clé ?';
  }

  @override
  String get keyTake => 'La prendre';

  @override
  String get keyFingerprint => 'Clé de Povodok';

  @override
  String get pwFactory =>
      'Le mot de passe d’usine : donnez-en un nouveau, 12 caractères au moins.';

  @override
  String diagMove(Object ch) {
    return 'Déplacer l’ensemble sur $ch';
  }
}
