import 'dart:typed_data';

import 'package:flutter_test/flutter_test.dart';
import 'package:pusinka/data.dart';
import 'package:strelka/strelka.dart';

/// A head block of version 4 (RECORD.md): session 44, rate 100 Hz; the units 1, 3 and 7, 7 a
/// Kryska, from the set's table after it.
RecordHead head() {
  final b = Uint8List(blockBytes);
  b.setAll(0, 'STRK'.codeUnits);
  b[4] = 4;                                  // the version
  b[5] = kindHead;
  ByteData.sublistView(b)
    ..setUint16(6, 44, Endian.little)
    ..setUint16(20, 100, Endian.little);
  b[8] = 1;
  final h = RecordHead.parse(b)!;
  h.units.addAll({1: kindBelka, 3: kindBelka, 7: kindKryska});
  return h;
}

SessionOnSet session(List<int> on, {int bytes = 1000000}) {
  final s = SessionOnSet(44);
  for (final n in on) {
    s.records[n] = RecordOnBelka(n, 6000, 1000, bytes ~/ on.length);
  }
  return s;
}

void main() {
  test('a session described: the rate, how long, its units by kind, how much', () {
    expect(describeSession(session([1, 3, 7]), head()), '100 Hz · 1:00 · B1 B3 K7 · 1.0 MB');
  });

  test("a session's state: on the phone, a unit to switch on, too large, or ready", () {
    expect(sessionState(session([1, 3, 7]), head(), {'2026-11-07_1240_S44_Bára_card.c3d'}),
        SessionState.onPhone);
    expect(sessionState(session([1, 3, 7]), head(), {'2026-11-07_1240_S44_live.c3d'}),
        SessionState.ready);                                         // live is not the cards'
    expect(sessionState(session([1, 3]), head(), {}), SessionState.switchOn);   // K7 off
    expect(sessionState(session([1, 3, 7], bytes: 2100000000), head(), {}), SessionState.large);
    expect(sessionState(session([1, 3, 7]), head(), {}), SessionState.ready);
    expect(sessionState(session([1]), null, {}), SessionState.ready);           // a head unknown
  });

  test("a file's name: the date and the time, the session, the note made safe and cut to 64", () {
    final t = DateTime(2026, 11, 10, 17, 5);
    expect(recordBase(t, 44, ''), '2026-11-10_1705_S44');
    expect(recordBase(t, 44, ' Praha: prase/zlomená noha? '),
        '2026-11-10_1705_S44_Praha- prase-zlomená noha-');
    expect(recordBase(t, 7, 'x' * 80), '2026-11-10_1705_S7_${'x' * 64}');
    expect(cardOnPhone({'2026-11-10_1705_S44_card.c3d'}, 4), isFalse);
    expect(cardOnPhone({'2026-11-10_1705_S44_card.c3d'}, 44), isTrue);
  });
}
