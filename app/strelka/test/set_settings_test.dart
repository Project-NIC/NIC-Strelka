@TestOn('vm')
library;

import 'dart:io';

import 'package:strelka/strelka.dart';
import 'package:test/test.dart';

/// The set's own settings against tools/povodok_sim.py in its service mode: the search finding a
/// new Belka and the take giving it its number; the password changed, the old one refused after.

const int base = 3437;
const String host = '127.0.0.1';

void main() {
  late Process sim;
  setUpAll(() async {
    final tools = '${Directory.current.path}/../../tools';
    sim = await Process.start('python3',
        ['$tools/povodok_sim.py', '--host', host, '--port', '$base', '--belkas', '2']);
    for (var i = 0; i < 50; i++) {
      try {
        (await Socket.connect(host, base, timeout: const Duration(seconds: 1))).destroy();
        return;
      } on SocketException {
        await Future<void>.delayed(const Duration(milliseconds: 100));
      }
    }
  });
  tearDownAll(() => sim.kill());

  test('a new Belka found by the search and taken with its number', () async {
    final c = await SmpClient.connect(host, port: base, password: 'strelka1');
    await c.write(groupStrelka, Strelka.search, body: {'run': true});
    List<Object?> found = const [];
    for (var i = 0; i < 30 && found.isEmpty; i++) {
      await Future<void>.delayed(const Duration(milliseconds: 100));
      found = (await c.read(groupStrelka, Strelka.search))['found'] as List<Object?>;
    }
    expect(found, isNotEmpty);
    final chip = (found.first as Map<Object?, Object?>)['chip'];
    await c.write(groupStrelka, Strelka.take, body: {'chip': chip, 'n': 7});
    await c.write(groupStrelka, Strelka.search, body: {'run': false});
    final set = await c.read(groupStrelka, Strelka.set);
    expect([for (final b in set['belkas'] as List<Object?>) (b as Map<Object?, Object?>)['n']],
        contains(7));
    c.close();
  });

  test("Povodok's own key: the same on every port, its fingerprint, another refused", () async {
    final c = await SmpClient.connect(host, port: base, password: 'strelka1');
    final key = c.unitKey!;
    c.close();
    final b1 = await SmpClient.connect(host, port: base + 1, password: 'strelka1', pinned: key);
    expect(b1.unitKey, key);
    b1.close();
    expect(await keyFingerprint(key), matches(RegExp(r'^([0-9A-F]{4} ){3}[0-9A-F]{4}$')));
    await expectLater(
        SmpClient.connect(host, port: base, password: 'strelka1', pinned: List.filled(32, 0)),
        throwsA(isA<KeyMismatch>()));
  });

  test('the password changed: the new one passes the lock, the old one is refused', () async {
    final c = await SmpClient.connect(host, port: base, password: 'strelka1');
    await c.changePassword('strelka1', 'twelve chars at least');
    c.close();
    (await SmpClient.connect(host, port: base, password: 'twelve chars at least')).close();
    await expectLater(SmpClient.connect(host, port: base, password: 'strelka1'),
        throwsA(isA<SmpError>().having((e) => e.rc, 'rc', Rc.access)));
  });
}
