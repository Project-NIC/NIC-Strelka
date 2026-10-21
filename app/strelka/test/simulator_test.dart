@TestOn('vm')
library;

import 'dart:io';
import 'dart:typed_data';

import 'package:strelka/strelka.dart';
import 'package:test/test.dart';

/// The Povodok simulator of tools/povodok_sim.py driven as the application will drive it: the
/// lock, the system's answers, the record, a Belka through its port, the live view's messages
/// to the address the lock was passed from. Python 3 with cryptography and cbor2 is wanted.

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

  test('without the lock access is denied; SMP version 0 is too old', () async {
    final c = await SmpClient.connect(host, port: base);
    await expectLater(c.read(groupStrelka, Strelka.info), code(Rc.access));
    await expectLater(c.request(opRead, groupStrelka, Strelka.info, version: 0), code(Rc.tooOld));
    c.close();
  });

  test('the record started and stopped; a Belka through its own port', () async {
    final c = await SmpClient.connect(host, port: base, password: password);
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
