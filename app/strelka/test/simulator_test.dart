@TestOn('vm')
library;

import 'dart:io';
import 'dart:typed_data';

import 'package:strelka/strelka.dart';
import 'package:test/test.dart';

/// The Povodok simulator of tools/povodok_sim.py driven as the application will drive it: the
/// lock, the system's answers, the record, a Belka through its port, its card erased with the
/// password given again, the live view's messages to the address the lock was passed from. Python 3 with cryptography and cbor2 is wanted.

const int base = 2337;
const String password = 'strelka1';
const String host = '127.0.0.1';

Future<Process> startSimulator() async {
  final tools = '${Directory.current.path}/../../tools';
  final sim = await Process.start(
      'python3', ['$tools/povodok_sim.py', '--host', host, '--port', '$base', '--belkas', '4']);
  for (var i = 0; i < 50; i++) {
    try {
      final s = await Socket.connect(host, base, timeout: const Duration(seconds: 1));
      s.destroy();
      return sim;
    } on SocketException {
      await Future<void>.delayed(const Duration(milliseconds: 100));
    }
  }
  sim.kill();
  throw StateError('the simulator did not come up');
}

Matcher code(int rc) => throwsA(isA<SmpError>().having((e) => e.rc, 'rc', rc));

void main() {
  late Process sim;
  setUpAll(() async {
    sim = await startSimulator();
  });
  tearDownAll(() {
    sim.kill();
  });

  test('the lock: a wrong password refused, the right one passed', () async {
    await expectLater(SmpClient.connect(host, port: base, password: 'wrong-one'), code(Rc.access));
    final c = await SmpClient.connect(host, port: base, password: password);
    final info = await c.read(groupStrelka, Strelka.info);
    expect((info['chip'] as Uint8List).length, 8);
    expect(info['n'], 0);
    expect(info['state'], 4);
    expect(info['fw'], isA<List<Object?>>());
    c.close();
  });

  test('without the lock access is denied; SMP version 0 taken as 1 is', () async {
    final c = await SmpClient.connect(host, port: base);
    await expectLater(c.read(groupStrelka, Strelka.info), code(Rc.access));
    await expectLater(c.request(opRead, groupStrelka, Strelka.info, version: 0), code(Rc.access));
    c.close();
    final d = await SmpClient.connect(host, port: base, password: password);
    final info = await d.request(opRead, groupStrelka, Strelka.info, version: 0);
    expect(info['n'], 0);
    d.close();
  });

  test("a Belka's card erased only with the password given again", () async {
    final b = await SmpClient.connect(host, port: base + 3, password: password);
    expect((await b.read(groupStrelka, Strelka.info))['n'], 3);   // the connection's lock spent
    await expectLater(b.write(groupStrelka, Strelka.card, body: {'do': 1}), code(Rc.access));
    await expectLater(b.afresh(password, Strelka.card, body: {'do': 3}), code(Rc.badState));
    await b.afresh(password, Strelka.card, body: {'do': 1});
    await expectLater(b.write(groupStrelka, Strelka.card, body: {'do': 1}), code(Rc.access));
    final sessions = await b.read(groupStrelka, Strelka.sessions, body: {'from': 0});
    expect(sessions['list'], isEmpty);
    b.close();
  });

  test('the record refused in the service mode, started and stopped at work; a Belka through '
      'its own port', () async {
    final c = await SmpClient.connect(host, port: base, password: password);
    await expectLater(c.write(groupStrelka, Strelka.record, body: {'run': true}), code(Rc.badState));
    await c.write(groupStrelka, Strelka.work); // the settings closed, from here on at work
    await c.write(groupStrelka, Strelka.record, body: {'run': true});
    expect((await c.read(groupStrelka, Strelka.info))['state'], 0);
    await c.write(groupStrelka, Strelka.record, body: {'run': false});
    expect((await c.read(groupStrelka, Strelka.info))['state'], 4);
    final b = await SmpClient.connect(host, port: base + 2, password: password);
    final info = await b.read(groupStrelka, Strelka.info);
    expect(info['n'], 2);
    expect(info['who'], 2);
    final sessions = await b.read(groupStrelka, Strelka.sessions, body: {'from': 0});
    expect(sessions['list'], isNotEmpty);
    b.close();
    c.close();
  });

  test('64/14 read once a second keeps the live view going; a silent phone loses it', () async {
    final c = await SmpClient.connect(host, port: base, password: password);
    final views = await c.watchSet().take(2).toList().timeout(const Duration(seconds: 5));
    expect(views.length, 2);
    expect(views.last['belkas'], isA<List<Object?>>());
    final r = await LiveReceiver.bind(port: base);
    final got = <LiveMessage>[];
    final sub = r.messages.listen(got.add);
    await c.write(groupStrelka, Strelka.live, body: {'live': 1});
    await Future<void>.delayed(const Duration(milliseconds: 500));
    expect(got, isNotEmpty);
    await Future<void>.delayed(const Duration(milliseconds: 3500));    // the phone silent
    got.clear();
    await Future<void>.delayed(const Duration(milliseconds: 500));
    expect(got, isEmpty);
    await c.read(groupStrelka, Strelka.set);                             // back
    await Future<void>.delayed(const Duration(milliseconds: 500));
    expect(got, isNotEmpty);
    await sub.cancel();
    r.close();
    c.close();
  });

  test("a Belka's record downloaded whole, and again from a byte", () async {
    final b = await SmpClient.connect(host, port: base + 2, password: password);
    final list = (await b.read(groupStrelka, Strelka.sessions))['list'] as List<Object?>;
    final file = (list.first as Map<Object?, Object?>)['file'] as String;
    final whole = await b.download(file);
    final rest = await b.download(file, from: 1000);
    expect(rest, whole.sublist(1000));
    b.close();
  });

  test('the live view comes to the address the lock was passed from', () async {
    final r = await LiveReceiver.bind(port: base);
    final c = await SmpClient.connect(host, port: base, password: password);
    await c.write(groupStrelka, Strelka.session, body: {
      's': 3,
      'rate': 1000,
      'n': 10,
      'acc': 16,
      'gyro': 2000,
      'live': 1,
      'stop': 10,
      't': 0,
      'tz': 0,
    });
    final m = await r.messages.first.timeout(const Duration(seconds: 5));
    expect(m.session, 3);
    expect(m.code, LiveCode.quat50);
    expect(m.belkas.length, 4);
    for (final b in m.belkas) {
      final q = b.quats.single;
      expect(q.w * q.w + q.x * q.x + q.y * q.y + q.z * q.z, closeTo(1.0, 0.01));
    }
    r.close();
    c.close();
  });
}
