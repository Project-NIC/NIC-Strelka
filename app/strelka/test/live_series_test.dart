import 'dart:math' as math;

import 'package:strelka/strelka.dart';
import 'package:test/test.dart';

/// The live view gathered: two Povodoks' messages joined, a loss a gap, the vertical acceleration
/// from raw steps, the tilt from quaternions, the stride found and its numbers.

LiveMessage message(int period, List<LiveBelka> belkas, {int session = 7}) => LiveMessage(
    session: session, period: period, state: 0, s0: 0, povodoks: 1, povodokStates: const [0],
    belkas: belkas);

/// A unit at rest but for a vertical swing of [amp] g at [hz], z up, at 100 Hz, ±16 g; [half]
/// g more at half the rate, a trunk's two beats in a stride unlike.
List<RawStep> raws(int period, {double amp = 0.3, double hz = 1.25, double half = 0}) => [
      for (var j = 0; j < 2; j++)
        RawStep([
          0,
          0,
          ((1 +
                      amp * math.sin(2 * math.pi * hz * (period * 2 + j) / 100) +
                      half * math.sin(math.pi * hz * (period * 2 + j) / 100)) *
                  2048)
              .round(),
        ], const [0, 0, 0]),
    ];

void main() {
  test('two Povodoks the same period: kept once; a period missed: a gap', () {
    final s = LiveSeries();
    final b = LiveBelka(n: 3, s0: 0, by: 0, state: 4, raws: raws(10));
    s.add(message(10, [b]));
    s.add(message(10, [b]));                                 // from the other Povodok
    s.add(message(12, [LiveBelka(n: 3, s0: 0, by: 1, state: 4, raws: raws(12))]));
    final u = s.units[3]!;
    expect(u.rateHz, 100);
    expect(u.values.length, 6);
    expect(u.values.sublist(2, 4).every((v) => v.isNaN), isTrue);
    expect(u.values.first.abs(), lessThan(0.01));            // at rest: 1 g along gravity, less 1 g
  });

  test('the tilt from the vertical, the heading free', () {
    expect(tilt(const Quat(1, 0, 0, 0)), closeTo(0, 1e-9));
    final h = math.sqrt(0.5);
    expect(tilt(Quat(h, h, 0, 0)), closeTo(90, 1e-9));       // about x by 90°
    expect(tilt(Quat(h, 0, 0, h)), closeTo(0, 1e-9));        // about z alone: no tilt
  });

  test("the stride by its period, a limb's one beat, a trunk's two unlike; a loss not counted", () {
    final s = LiveSeries();
    for (var p = 0; p < 50 * 12; p++) {
      if (p >= 500 && p < 505) continue;                     // 0.1 s lost
      s.add(message(p, [
        LiveBelka(n: 1, s0: 0, by: 0, state: 4, raws: raws(p)),
        LiveBelka(n: 2, s0: 0, by: 0, state: 4, raws: raws(p, half: 0.08)),
      ]));
    }
    final limb = s.units[1]!, trunk = s.units[2]!;
    expect(stridePeriod(limb.values, 100)! / 100, closeTo(0.8, 0.03));
    expect(stridePeriod(trunk.values, 100)! / 100, closeTo(1.6, 0.03));
    final n = strideNumbers(limb.values, limb.rateHz, 10)!;
    expect(n.range, closeTo(0.6, 0.05));
    expect(n.strides, 9);                                    // the stride with the loss left out
    expect(leftAgainstRight(0.55, 0.5), closeTo(9.52, 0.01));
  });

  test('no rhythm, no numbers', () {
    expect(strideNumbers(List.filled(1000, 0.0), 100, 10), isNull);
  });
}
