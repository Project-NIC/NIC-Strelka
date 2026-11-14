/// The units of a set as the measurement's choice needs them (APP.md, What it must do, 5 and 7):
/// each one's kind and calibration, read through its port by 64/0 and 64/3, and how many of the
/// units chosen are in step — the session taken, 64/14's `rc` 0, and heard by Povodok within two
/// seconds, its `ago` (COMMANDS.md). The start waits for all of them.
library;

import 'dart:io';

import 'record.dart';
import 'smp.dart';

/// A unit of the set: its number, kind, calibration's time and temperatures; unreachable, none.
class UnitOnSet {
  const UnitOnSet(
    this.n, {
    this.kind = kindBelka,
    this.calTime = 0,
    this.tempCenti = 0,
    this.calTempCenti = 0,
    this.reachable = true,
    this.info = const {},
    this.cal = const {},
  });

  final int n, kind;

  /// Its answers to 64/0 and 64/3 as they came, for the live view saved (live_file.dart); the
  /// calibration's empty where it has none.
  final Map<String, Object?> info, cal;

  /// The calibration's time, s since 1970; 0 none.
  final int calTime;
  final int tempCenti, calTempCenti;
  final bool reachable;

  String get name => '${kindLetter(kind)}$n';
}

/// The units numbered [belkas], each read through its own port: 64/0 for its kind, its
/// temperature and its calibration's time, 64/3 for the calibration's temperature.
Future<List<UnitOnSet>> unitsOnSet(
  String host,
  String password,
  List<int> belkas, {
  int port = smpPort,
}) async {
  final out = <UnitOnSet>[];
  for (final n in belkas) {
    SmpClient? c;
    try {
      c = await SmpClient.connect(host, port: port + n, password: password);
      final info = await c.read(groupStrelka, Strelka.info);
      final calTime = info['cal'] as int? ?? 0;
      var cal = const <String, Object?>{};
      if (calTime != 0) {
        cal = await c.read(groupStrelka, Strelka.cal);
      }
      out.add(
        UnitOnSet(
          n,
          kind: info['kind'] as int? ?? kindBelka,
          calTime: calTime,
          tempCenti: info['temp'] as int? ?? 0,
          calTempCenti: cal['temp'] as int? ?? 0,
          info: info,
          cal: cal,
        ),
      );
    } on SmpError {
      out.add(UnitOnSet(n, reachable: false));
    } on SocketException {
      out.add(UnitOnSet(n, reachable: false));
    } finally {
      c?.close();
    }
  }
  return out;
}

/// 64/22's `got`: the face taken, the unit moved, lying on another face, measured still.
enum FaceGot { taken, moved, otherFace, measuring }

/// A face's answer; after the sixth taken, the calibration the Belka keeps, 64/3's fields.
class FaceAnswer {
  const FaceAnswer(this.got, [this.cal]);

  final FaceGot got;
  final Map<String, Object?>? cal;
}

/// The LSM6DSV320X's WHO_AM_I, 64/0's `sensor`: the hoof's sensor, the one with a high-g channel.
const int lsm320 = 0x73;

/// The high-g channel's ranges, ±g, a session of units with [sensors] — each by 64/0's `sensor`,
/// null unknown — may keep beside the low-g: offered where one has the LSM6DSV320X, the others
/// keeping their low-g alone.
List<int> hgRangesFor(Iterable<int?> sensors) {
  if (!sensors.contains(lsm320)) return const [];
  return const [32, 64, 128, 256, 320];
}

/// The range the high-g channel is calibrated at, ±g: its finest, 1 g some 1 000 steps
/// (DS14623 Rev 3, table 3); a session at another range takes it by the nominal steps.
const int hgCalRange = 32;

/// One of the six faces asked of a unit through its port (COMMANDS.md, 64/22; FIRMWARE.md,
/// Calibration): face 0 with the measurement's ranges and the time, and [hg], the high-g channel's
/// range where it is calibrated beside, 0 not; the answer read once the Belka's [wait] is over,
/// again while it measures still.
Future<FaceAnswer> calibrateFace(
  SmpClient c,
  int face, {
  int acc = 16,
  int gyro = 2000,
  int hg = 0,
  int? t,
  Duration wait = const Duration(seconds: 5),
  Duration every = const Duration(milliseconds: 500),
}) async {
  await c.write(groupStrelka, Strelka.calibrate, body: {
    'face': face,
    if (face == 0) ...{
      'acc': acc,
      'gyro': gyro,
      if (hg != 0) 'hg': hg,
      't': t ?? DateTime.now().millisecondsSinceEpoch ~/ 1000,
    },
  });
  await Future<void>.delayed(wait);
  for (;;) {
    final a = await c.read(groupStrelka, Strelka.calibrate);
    final got = a['got'] as int? ?? FaceGot.measuring.index;
    if (got < 0 || got >= FaceGot.values.length) throw const SmpError(Rc.unknown);
    if (got != FaceGot.measuring.index) {
      return FaceAnswer(FaceGot.values[got], a.containsKey('acc') ? a : null);
    }
    await Future<void>.delayed(every);
  }
}

/// Periods heard within for a unit in step: two seconds of 50.
const int inStepPeriods = 100;

/// How many of [chosen] are in step in a 64/14 answer: the session taken and heard of late.
int inStep(Map<String, Object?> setView, Iterable<int> chosen) {
  final want = chosen.toSet();
  var n = 0;
  for (final b in setView['belkas'] as List<Object?>? ?? const []) {
    final m = b as Map<Object?, Object?>;
    if (want.contains(m['n']) && m['rc'] == 0 && (m['ago'] as int? ?? 1 << 30) < inStepPeriods) {
      n++;
    }
  }
  return n;
}
