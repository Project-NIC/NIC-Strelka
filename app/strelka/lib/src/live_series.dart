/// The live view's steps gathered for the curves and the numbers (APP.md, What it must do, 6):
/// the Povodoks' messages joined by the Belka's number and the period, one of each kept; each
/// Belka's signal kept over the last seconds, a loss a gap. Raw steps give the vertical
/// acceleration, the acceleration along gravity as its slow mean finds it, less 1 g; quaternions
/// the tilt from the vertical, the heading being free. A stride is the signal's own period, found
/// by its autocorrelation; the numbers are each stride's maximum, minimum and range, averaged over
/// the last strides, a stride with a loss not counted. A first take, for a person to judge on the
/// animal.
library;

import 'dart:math' as math;

import 'live_view.dart';

/// Periods a second (PRINCIPLE.md).
const int periodsAS = 50;

/// One Belka's signal: a value a step, NaN a step lost.
class UnitSeries {
  UnitSeries(this.n);

  final int n;
  bool quaternions = false;
  int rateHz = 0;

  /// The step index of values' first: the period times the steps a period, and the step in it.
  int first = 0;
  final List<double> values = [];
  int state = 0;

  /// The last period heard, for when it was.
  int lastPeriod = -1;
  List<double>? _gravity;

  int get end => first + values.length;
}

class LiveSeries {
  LiveSeries({this.keepS = 30, this.accG = 16, this.hgG = 0});

  /// Seconds kept: enough for the numbers' strides.
  final int keepS;

  /// The session's accelerometer range, ±g: what 32 768 steps make.
  int accG;

  /// The high-g channel's range, ±g, for the steps of codes 3 and 6; 0 none.
  int hgG;

  final Map<int, UnitSeries> units = {};

  /// The latest period of any message.
  int period = -1;
  int session = -1;

  /// A message in: each Belka's steps placed by the period, a period already had skipped, the
  /// steps between lost filled as gaps.
  void add(LiveMessage m) {
    if (m.session != session) {
      units.clear();
      session = m.session;
      period = -1;
    }
    if (m.period > period) period = m.period;
    for (final b in m.belkas) {
      final quats = b.quats.isNotEmpty;
      final steps = quats ? b.quats.length : b.raws.length;
      if (steps == 0) continue;
      final u = units.putIfAbsent(b.n, () => UnitSeries(b.n));
      u.state = b.state;
      final rate = steps * periodsAS;
      if (u.rateHz != rate || u.quaternions != quats) {
        u
          ..rateHz = rate
          ..quaternions = quats
          ..values.clear()
          ..first = m.period * steps
          .._gravity = null;
      }
      final at = m.period * steps;
      if (at < u.end) continue;                    // had already, from another Povodok
      if (u.values.isEmpty) u.first = at;
      while (u.end < at) {
        u.values.add(double.nan);
      }
      for (var j = 0; j < steps; j++) {
        u.values.add(quats ? tilt(b.quats[j]) : _vertical(u, b.raws[j]));
      }
      u.lastPeriod = m.period;
      final over = u.values.length - keepS * u.rateHz;
      if (over > 0) {
        u.values.removeRange(0, over);
        u.first += over;
      }
    }
  }

  double _vertical(UnitSeries u, RawStep s) {
    final range = s.highG && hgG != 0 ? hgG : accG;
    final a = [for (final v in s.a) v * range / 32768];
    final g = u._gravity ??= List.of(a);
    final k = 1 / u.rateHz;                         // the slow mean over about a second
    for (var i = 0; i < 3; i++) {
      g[i] += (a[i] - g[i]) * k;
    }
    final norm = math.sqrt(g[0] * g[0] + g[1] * g[1] + g[2] * g[2]);
    if (norm == 0) return double.nan;
    return (a[0] * g[0] + a[1] * g[1] + a[2] * g[2]) / norm - norm;
  }
}

/// The tilt from the vertical, degrees: the angle of the unit's z axis to the vertical, the same
/// whichever way the quaternion turns.
double tilt(Quat q) {
  final c = (1 - 2 * (q.x * q.x + q.y * q.y)).clamp(-1.0, 1.0);
  return math.acos(c) * 180 / math.pi;
}

/// A stride's numbers, averaged over the strides counted.
class StrideNumbers {
  const StrideNumbers(this.max, this.min, this.range, this.strides, this.periodS);

  final double max, min, range;
  final int strides;
  final double periodS;
}

/// The stride's length in steps, by the autocorrelation over the last [lookS] seconds, between
/// 0.25 and 2 s: the first peak nearly as strong as the strongest; the one at twice its lag taken
/// if clearly stronger, a trunk's two beats in a stride, unlike each other, being one stride. Null
/// with no rhythm.
int? stridePeriod(List<double> v, int rateHz, {double lookS = 10}) {
  final n = math.min(v.length, (lookS * rateHz).round());
  final x = v.sublist(v.length - n);
  final finite = x.where((e) => !e.isNaN).toList();
  if (finite.length < rateHz) return null;
  final mean = finite.reduce((a, b) => a + b) / finite.length;
  double r(int k) {
    var num = 0.0, den = 0.0;
    for (var i = 0; i + k < x.length; i++) {
      final a = x[i], b = x[i + k];
      if (a.isNaN || b.isNaN) continue;
      num += (a - mean) * (b - mean);
      den += (a - mean) * (a - mean);
    }
    return den == 0 ? 0 : num / den;
  }

  final lo = (0.25 * rateHz).round(), hi = math.min((2 * rateHz).round(), x.length ~/ 2);
  if (hi <= lo + 1) return null;
  final rs = [for (var k = 0; k <= hi; k++) k < lo ? 0.0 : r(k)];
  final peaks = [
    for (var k = lo + 1; k < hi; k++)
      if (rs[k] > rs[k - 1] && rs[k] >= rs[k + 1] && rs[k] >= 0.3) k
  ];
  if (peaks.isEmpty) return null;
  final strongest = peaks.map((k) => rs[k]).reduce(math.max);
  final first = peaks.firstWhere((k) => rs[k] >= 0.8 * strongest);
  final twice = 2 * first;
  if (twice + 2 < hi) {
    var near = twice;
    for (var k = twice - 2; k <= twice + 2; k++) {
      if (rs[k] > rs[near]) near = k;
    }
    if (rs[near] > rs[first] + 0.05) return near;
  }
  return first;
}

/// The last [strides] strides' maximum, minimum and range, back from the newest step, a stride
/// with a gap not counted; null with no rhythm or no whole stride.
StrideNumbers? strideNumbers(List<double> v, int rateHz, int strides) {
  final p = stridePeriod(v, rateHz);
  if (p == null) return null;
  var mx = 0.0, mn = 0.0, rg = 0.0, counted = 0;
  for (var i = 0; i < strides; i++) {
    final end = v.length - i * p, start = end - p;
    if (start < 0) break;
    final s = v.sublist(start, end);
    if (s.any((e) => e.isNaN)) continue;
    final hi = s.reduce(math.max), lo = s.reduce(math.min);
    mx += hi;
    mn += lo;
    rg += hi - lo;
    counted++;
  }
  if (counted == 0) return null;
  return StrideNumbers(mx / counted, mn / counted, rg / counted, counted, p / rateHz);
}

/// Left against right: the difference of the ranges, % of their mean; positive the left more.
double leftAgainstRight(double left, double right) {
  final mean = (left + right) / 2;
  return mean == 0 ? 0 : (left - right) / mean * 100;
}
