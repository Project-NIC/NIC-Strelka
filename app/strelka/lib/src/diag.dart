/// The diagnostics as Povodok gives them (COMMANDS.md, 64/23; APP.md, The menus, 5): what it
/// knows already of each Belka — its signal, when it was last heard, its items as last heard —
/// and of the band, each channel's noise as the Povodoks and the Belkas measured it. Read once a
/// second, in the service mode and at work, the live view on or off.
library;

import 'smp.dart';

/// A Belka as Povodok knows it; a value null while unknown.
class UnitDiag {
  UnitDiag.fromMap(Map<Object?, Object?> m)
      : n = m['n'] as int,
        rssi = m['rssi'] as int?,
        ago = m['ago'] as int? ?? 0,
        cell = m['cell'] as int?,
        temp = m['temp'] as int?,
        mv = m['mv'] as int?,
        free = m['free'] as int?,
        dropped = m['dropped'] as int?,
        sync = m['sync'] as int?,
        missed = m['missed'] as int?,
        offset = m['offset'] as int?,
        out = (m['out'] as List<Object?>?)?.cast<int>(),
        ch = m['ch'] as int?,
        noise = m['noise'] as int?;

  final int n, ago;
  final int? rssi, cell, temp, mv, free, dropped, sync, missed, offset, ch, noise;

  /// The steps out of range since the start: the accelerometer's and the gyro's.
  final List<int>? out;

  /// Seconds since its last LIVE, 50 periods a second.
  double get agoS => ago / 50;
}

/// The band and the Belkas, one reading.
class SetDiag {
  SetDiag(this.units, Map<String, Object?> first)
      : ch = first['ch'] as int? ?? 0,
        spare = first['spare'] as int? ?? 0,
        margin = (first['margin'] as List<Object?>? ?? const []).cast<int>(),
        noise = [
          for (final l in first['noise'] as List<Object?>? ?? const [])
            (l as List<Object?>).cast<int>()
        ],
        bnoise = (first['bnoise'] as List<Object?>? ?? const []).cast<int>();

  final List<UnitDiag> units;
  final int ch, spare;

  /// Each Povodok's margin, 0–31 by 2 dB.
  final List<int> margin;

  /// Each Povodok's noise on each channel and the Belkas', dBm; −128 not measured.
  final List<List<int>> noise;
  final List<int> bnoise;

  static const int notMeasured = -128;

  /// A channel's noise, the loudest any measurer saw; null not measured yet.
  int? channelNoise(int c) {
    int? most;
    for (final l in [...noise, bnoise]) {
      if (c < l.length && l[c] != notMeasured && (most == null || l[c] > most)) most = l[c];
    }
    return most;
  }
}

/// One reading of 64/23, its pages gathered.
Future<SetDiag> readDiag(SmpClient c) async {
  final units = <UnitDiag>[];
  Map<String, Object?>? first;
  for (var from = 0;; from += 8) {
    final page = await c.read(groupStrelka, Strelka.diag, body: {'from': from});
    first ??= page;
    for (final b in page['belkas'] as List<Object?>? ?? const []) {
      units.add(UnitDiag.fromMap(b as Map<Object?, Object?>));
    }
    if (page['more'] != true) break;
  }
  return SetDiag(units, first);
}
