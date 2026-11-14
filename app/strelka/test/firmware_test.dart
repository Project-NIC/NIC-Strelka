@TestOn('vm')
library;

import 'dart:io';
import 'dart:typed_data';

import 'package:strelka/strelka.dart';
import 'package:test/test.dart';

/// The firmware's update held to the Python tools: a folder made by tools/strelka_images.py served
/// as GitHub would serve it, fetched, gunzipped, checked and kept; a tampered image refused; then
/// an image given to the simulated Povodok and a Belka through its port, tested and taken at the
/// reset, 64/0 saying the new version. Python 3 with cryptography and cbor2 is wanted.

const int base = 3537;
const String password = 'strelka1';
const String host = '127.0.0.1';

const String script = r'''
import sys
sys.path.insert(0, sys.argv[1])
import strelka_images as si
image = {k: si.fake_image("0.8.6", 5000 + 100 * i) for i, k in enumerate(si.KINDS)}
image["zvjozdocka"] = image["belka"]            # today the Belka's: one file for both
si.write(sys.argv[2], image)
''';

void main() {
  late Directory folder, phone;
  late HttpServer server;
  late Process sim;
  final tools = '${Directory.current.path}/../../tools';

  setUpAll(() async {
    folder = await Directory.systemTemp.createTemp('strelka_images');
    phone = await Directory.systemTemp.createTemp('strelka_phone');
    final r = await Process.run('python3', ['-c', script, tools, folder.path]);
    expect(r.exitCode, 0, reason: '${r.stderr}');
    server = await HttpServer.bind(host, 0);
    server.listen((rq) async {                  // the folder as GitHub's raw files serve it
      final f = File('${folder.path}${rq.uri.path}');
      if (await f.exists()) {
        rq.response.add(await f.readAsBytes());
      } else {
        rq.response.statusCode = HttpStatus.notFound;
      }
      await rq.response.close();
    });
    sim = await Process.start(
        'python3', ['$tools/povodok_sim.py', '--host', host, '--port', '$base', '--belkas', '2']);
    for (var i = 0; i < 50; i++) {
      try {
        (await Socket.connect(host, base, timeout: const Duration(seconds: 1))).destroy();
        return;
      } on SocketException {
        await Future<void>.delayed(const Duration(milliseconds: 100));
      }
    }
    throw StateError('the simulator did not come up');
  });

  tearDownAll(() async {
    sim.kill();
    await server.close(force: true);
    await folder.delete(recursive: true);
    await phone.delete(recursive: true);
  });

  Uri source() => Uri.parse('http://$host:${server.port}/');

  test('the folder fetched, each image gunzipped and checked, kept and read back', () async {
    final got = await fetchFirmware(phone, source: source());
    expect(got!.version, '0.8.6');
    expect(got.images.keys.toSet(), imageKinds.toSet());
    expect(imageVersion(got.images['kryska']!), '0.8.6');
    expect(got.images['povodok']!.length, 5332);      // the header's 32 B and the body
    expect(got.images['zvjozdocka'], got.images['belka']);
    final kept = await keptFirmware(phone);
    expect(kept!.images['belka'], got.images['belka']);
    expect(imageKindOf({'who': 2, 'kind': 1}), 'kryska');
    expect(imageKindOf({'who': 2, 'kind': 2}), 'zvjozdocka');
    expect(imageKindOf({'who': 2, 'kind': 0}), 'belka');
    expect(imageKindOf({'who': 0}), 'povodok');
  });

  test('a folder without a manifest is none; an image other than the manifest says, refused',
      () async {
    final empty = await Directory.systemTemp.createTemp('strelka_none');
    expect(await fetchFirmware(empty, source: source().resolve('nothing/')), isNull);
    await empty.delete(recursive: true);
    final kept = await keptFirmware(phone);
    final bad = Uint8List.fromList(kept!.images['belka']!)..[100] ^= 1;
    await expectLater(checkImage(kept.manifest, 'belka', bad), throwsA(isA<FirmwareMismatch>()));
    expect(imageVersion(Uint8List(64)), isNull);
  });

  test('an image given to Povodok and to a Belka through its port: tested, taken at the reset',
      () async {
    final fw = (await keptFirmware(phone))!;
    for (final port in [base + 2, base]) {          // the Belka first, Povodok last
      final c = await SmpClient.connect(host, port: port, password: password);
      final info = await c.read(groupStrelka, Strelka.info);
      expect(fwOf(info), '0.8.0');
      final seen = <int>[];
      final image = imageFor(fw, info)!;           // the simulator's Belkas are Zvjozdočkas
      await updateUnit(c, image, onProgress: (done, _) => seen.add(done));
      expect(seen.last, image.length);
      expect(fwOf(await c.read(groupStrelka, Strelka.info)), '0.8.6');
      c.close();
    }
  });
}
