import 'dart:typed_data';

import 'package:strelka/strelka.dart';
import 'package:test/test.dart';

Uint8List hex(String s) =>
    Uint8List.fromList([for (var i = 0; i < s.length; i += 2) int.parse(s.substring(i, i + 2), radix: 16)]);

String toHex(List<int> b) => b.map((x) => x.toRadixString(16).padLeft(2, '0')).join();

void main() {
  // the vectors are tools/live_view.py's, the Python reference
  test('quaternions packed as live_view.py packs them', () {
    expect(packQuat(const Quat(1, 0, 0, 0)), 0);
    expect(packQuat(const Quat(0.5, 0.5, 0.5, 0.5)), 378905961);
    expect(packQuat(const Quat(0, -1, 0, 0)), 1073741824);
    final n = 1 / 0.9746794344808963; // |(0.9, 0.1, -0.3, 0.2)| = sqrt(0.95)
    expect(packQuat(Quat(0.9 * n, 0.1 * n, -0.3 * n, 0.2 * n)), 78416020);
    final q = unpackQuat(78416020);
    expect(q.w, closeTo(0.923689, 1e-6));
    expect(q.x, closeTo(0.102399, 1e-6));
    expect(q.y, closeTo(-0.307197, 1e-6));
    expect(q.z, closeTo(0.204798, 1e-6));
    expect(unpackQuat(1073741824).x, closeTo(1.0, 1e-9));
  });

  test('a message of two Belkas, quaternions, as live_view.py builds it', () {
    const bytes = '0101070040e2010017801801178000000000000002011100145000000000031802a903919c8c04';
    final m = parseLive(hex(bytes));
    expect([m.session, m.period, m.state, m.s0, m.povodoks], [7, 123456, 0x8017, 0x18, 1]);
    expect(m.povodokStates, [0x8017, 0, 0, 0]);
    expect(m.code, LiveCode.quat50);
    expect(m.item, 8);
    expect(m.belkas.length, 2);
    final first = m.belkas[0];
    expect([first.n, first.s0, first.by, first.state], [1, 0x11, 0, (80 << 8) | 20]);
    expect(first.item, 1);
    expect(first.quats.single.w, closeTo(1.0, 1e-9));
    final second = m.belkas[1];
    expect([second.n, second.by, second.state], [3, 2, (3 << 8) | (-87 & 0xFF)]);
    expect(second.quats.single.y, closeTo(-217 / 511 * 0.70710678, 1e-6)); // packed unnormalised
    expect(toHex(buildLive(m)), bytes);
  });

  test('a message of one Belka, raw steps', () {
    const bytes = '0101070001000000000048010000000000000000010241000000640038ff0040b80b0000fbff';
    final m = parseLive(hex(bytes));
    expect(m.code, LiveCode.raw50);
    final b = m.belkas.single;
    expect([b.n, b.code, b.item, b.by, b.state], [2, LiveCode.raw50, 1, 0, 0]);
    expect(b.raws.single.a, [100, -200, 16384]);
    expect(b.raws.single.g, [3000, 0, -5]);
    expect(toHex(buildLive(m)), bytes);
  });

  test('what is not a message', () {
    expect(() => parseLive(hex('0201')), throwsFormatException);
    expect(() => parseLive(hex('020107004000000000004801000000000000000000')), throwsFormatException);
    expect(() => parseLive(hex('010107004000000000004801000000000000000001')), throwsFormatException);
    expect(() => parseLive(hex('01010700400000000000480100000000000000000000')), throwsFormatException);
  });

  test('too many steps for the code, or too large', () {
    const belka = LiveBelka(n: 1, s0: 0x11, by: 0, state: 0, quats: [Quat(1, 0, 0, 0), Quat(1, 0, 0, 0)]);
    expect(
        () => buildLive(const LiveMessage(
            session: 1, period: 1, state: 0, s0: 0x18, povodoks: 1, povodokStates: [0, 0, 0, 0], belkas: [belka])),
        throwsArgumentError);
  });
}
