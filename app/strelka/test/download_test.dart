@TestOn('vm')
library;

import 'dart:convert';
import 'dart:io';
import 'dart:typed_data';

import 'package:strelka/strelka.dart';
import 'package:test/test.dart';

/// The download as the application runs it, against tools/povodok_sim.py: a session recorded,
/// found on every Belka by 64/2 and gathered, asked of Povodok once by 64/21, its frames off the
/// download's port into one C3D — the same bytes, to the last, as tools/strelka_c3d.py writes from
/// the Belkas' files fetched one by one. Python 3 with cryptography and cbor2 is wanted.

const int base = 3337;
const String password = 'strelka1';
const String host = '127.0.0.1';

Future<Process> startSimulator() async {
  final tools = '${Directory.current.path}/../../tools';
  final sim = await Process.start(
      'python3', ['$tools/povodok_sim.py', '--host', host, '--port', '$base', '--belkas', '4',
      '--face-s', '0.05']);
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

void main() {
  late Process sim;
  setUpAll(() async {
    sim = await startSimulator();
  });
  tearDownAll(() {
    sim.kill();
  });

  test("the units' kinds and calibrations read; the chosen in step once the session is taken "
      '(in the service mode, first)', () async {
    final b2 = await SmpClient.connect(host, port: base + 2, password: password);
    await b2.write(groupStrelka, Strelka.cal, body: {
      'acc': [16380, -16390, 16385, -16395, 16390, -16400],
      'gscale': [32768, 32750, 32700],
      'gzero': [5, -12, 3],
      'temp': 2550,
      't': 1760000000,
    });
    b2.close();
    final units = await unitsOnSet(host, password, [1, 2, 3, 4, 9], port: base);
    expect(units.map((u) => u.name), ['Z1', 'Z2', 'Z3', 'Z4', 'B9']);
    expect(units[1].calTime, 1760000000);
    expect(units[1].calTempCenti, 2550);
    expect(units[0].calTime, 0);
    expect(units[4].reachable, isFalse);                   // no Belka 9 in the set

    final c = await SmpClient.connect(host, port: base, password: password);
    await c.write(groupStrelka, Strelka.session, body: {
      's': 46, 'rate': 100, 'n': 1, 'acc': 16, 'gyro': 2000, 'live': 0,
      ...sessionUnits({2: kindZvjozdocka, 4: kindZvjozdocka}),
    });
    expect(inStep(await c.read(groupStrelka, Strelka.set), [2, 4]), 2);
    await c.write(groupStrelka, Strelka.session, // every unit again
        body: {'s': 47, 'rate': 100, 'n': 1, 'acc': 16, 'gyro': 2000, 'live': 0});
    c.close();
  });

  test('the calibration asked face by face (64/22), the Belka computing it, its time then read',
      () async {
    final b3 = await SmpClient.connect(host, port: base + 3, password: password);
    const wait = Duration(milliseconds: 100);
    await expectLater(b3.write(groupStrelka, Strelka.calibrate, body: {'face': 2}),
        throwsA(isA<SmpError>().having((e) => e.rc, 'rc', Rc.inval)));     // out of turn
    FaceAnswer? last;
    for (var face = 0; face < 6; face++) {
      last = await calibrateFace(b3, face, acc: 16, gyro: 2000, t: 1761000000, wait: wait);
      expect(last.got, FaceGot.taken);
    }
    b3.close();
    expect(last!.cal!['range'], [16, 2000]);
    expect((last.cal!['acc'] as List).length, 6);
    final units = await unitsOnSet(host, password, [3], port: base);
    expect(units.single.calTime, 1761000000);
  });

  test('the diagnostics: what Povodok knows of the Belkas and the band, read by 64/23', () async {
    final c = await SmpClient.connect(host, port: base, password: password);
    final d = await readDiag(c);
    c.close();
    expect(d.units.map((u) => u.n), [1, 2, 3, 4]);
    expect(d.units.first.rssi, isNotNull);
    expect(d.units.first.out, [0, 0]);
    expect(d.noise.length, 4);
    expect(d.channelNoise(11), -71);                        // the simulator's Wi-Fi
    expect(d.channelNoise(d.ch), isNotNull);
  });

  test('a session found on every Belka, downloaded through Povodok into the C3D of the tools',
      () async {
    final c = await SmpClient.connect(host, port: base, password: password);
    await c.write(groupStrelka, Strelka.work);
    // the set's table, by 64/18, into every Belka's file
    await writeTable(c, setTable(animal: [('kind', 'horse'), ('name', 'Bára')],
        belkas: [(1, 0x1101, 'LF', 'left fore'), (2, 0x1202, 'RF', 'right fore')]));
    await c.write(groupStrelka, Strelka.session,
        body: {'s': 42, 'rate': 100, 'n': 1, 'acc': 16, 'gyro': 2000, 'live': 0});
    await c.write(groupStrelka, Strelka.record, body: {'run': true});
    await Future<void>.delayed(const Duration(milliseconds: 300));
    await c.write(groupStrelka, Strelka.record, body: {'run': false});
    c.close();

    final sessions = await sessionsOnSet(host, password, [1, 2, 3, 4], port: base);
    final s42 = sessions.firstWhere((s) => s.s == 42);
    expect(s42.belkas, [1, 2, 3, 4]);

    final c3d = BytesBuilder(copy: false);
    var updates = 0;
    final progress = await downloadSession(host, password, s42, c3d.add,
        port: base, onProgress: (_) => updates++);
    expect(progress.over, isTrue);
    expect(progress.gone, isEmpty);
    expect(progress.bytes, s42.bytes);
    expect(updates, greaterThan(4));
    final ours = c3d.takeBytes();

    // the reference: each file fetched by 8/0, tools/strelka_c3d.py over them
    final dir = await Directory.systemTemp.createTemp('strelka_c3d');
    final files = <int, Uint8List>{};
    final paths = <String>[];
    for (final n in s42.belkas) {
      final b = await SmpClient.connect(host, port: base + n, password: password);
      final name = 'S${'42'.padLeft(5, '0')}B${'$n'.padLeft(2, '0')}.STRK';
      files[n] = await b.download(name);
      b.close();
      expect(utf8.decode(files[n]!, allowMalformed: true), contains('2;0000000000001202;RF;right fore'));
      final path = '${dir.path}/$name';
      await File(path).writeAsBytes(files[n]!);
      paths.add(path);
    }
    final tools = '${Directory.current.path}/../../tools';
    final out = '${dir.path}/ref.c3d';
    final r = await Process.run('python3', ['$tools/strelka_c3d.py', out, ...paths]);
    expect(r.exitCode, 0, reason: '${r.stdout}${r.stderr}');
    final ref = await File(out).readAsBytes();
    expect(ours.length, ref.length);
    expect(ours, ref);

    // and the files on the phone through the same composer
    final again = BytesBuilder(copy: false);
    composeFiles(files, s42.records.values.toList(), again.add);
    expect(again.takeBytes(), ref);
    await dir.delete(recursive: true);
  });

  test('a download broken off goes on into the same C3D and the same card files; the list shows '
      "the session's head",
      () async {
    final c = await SmpClient.connect(host, port: base, password: password);
    await c.write(groupStrelka, Strelka.session,
        body: {'s': 43, 'rate': 100, 'n': 1, 'acc': 32, 'gyro': 4000, 'live': 0});
    await c.write(groupStrelka, Strelka.record, body: {'run': true});
    await Future<void>.delayed(const Duration(milliseconds: 300));
    await c.write(groupStrelka, Strelka.record, body: {'run': false});
    c.close();
    final s43 = (await sessionsOnSet(host, password, [1, 2, 3, 4], port: base))
        .firstWhere((s) => s.s == 43);
    final head = await sessionHead(host, password, 2, 43, port: base);
    expect(head!.session, 43);
    expect(head.belka, 2);
    expect(head.accG, 32);
    expect(head.gyroDps, 4000);
    expect(head.rateHz, 100);

    final c3d = BytesBuilder(copy: false);
    final card = <int, List<int>>{};                    // each Belka's file, as STRK beside it
    final d = SessionDownload(s43, c3d.add, onPiece: (n, off, data) {
      final f = card.putIfAbsent(n, () => <int>[]);
      if (f.length < off + data.length) f.addAll(List.filled(off + data.length - f.length, 0));
      f.setRange(off, off + data.length, data);
    });
    var seen = 0;
    final first = await d.run(host, password, port: base, onProgress: (_) {
      if (++seen == 5) d.cancel();
    });
    expect(first, isFalse);
    expect(d.progress.bytes, inInclusiveRange(1, s43.bytes - 1));
    await Future<void>.delayed(const Duration(milliseconds: 200));   // Povodok lets it go
    expect(await d.run(host, password, port: base), isTrue);
    expect(d.progress.bytes, s43.bytes);

    final files = <int, Uint8List>{};
    for (final n in s43.belkas) {
      final b = await SmpClient.connect(host, port: base + n, password: password);
      files[n] = await b.download('S00043B${'$n'.padLeft(2, '0')}.STRK');
      b.close();
    }
    final ref = BytesBuilder(copy: false);
    composeFiles(files, s43.records.values.toList(), ref.add);
    expect(c3d.takeBytes(), ref.takeBytes());
    for (final n in s43.belkas) {
      expect(card[n], files[n], reason: 'Belka $n');
    }
    expect(d.head!.session, 43);
    final reader = RecordReader((_) {})..add(files[s43.belkas.first]!);
    expect(d.table, reader.table);
  });

  test("the session's units: the chosen record, the heads say who, a missing one named", () async {
    final c = await SmpClient.connect(host, port: base, password: password);
    // the units' kinds in the set's table (RECORD.md, The set's table)
    await writeTable(c, setTable(animal: [('kind', 'horse')], belkas: [
      (1, 0x1101, 'LF', 'left fore'),
      (3, 0x1303, 'RF', 'right fore')
    ], kinds: {1: 'Z', 3: 'K'}));
    await c.write(groupStrelka, Strelka.session, body: {
      's': 44, 'rate': 100, 'n': 1, 'acc': 16, 'gyro': 2000, 'live': 0,
      ...sessionUnits({1: kindZvjozdocka, 3: kindKryska}),
    });
    await c.write(groupStrelka, Strelka.record, body: {'run': true});
    await Future<void>.delayed(const Duration(milliseconds: 300));
    await c.write(groupStrelka, Strelka.record, body: {'run': false});
    await c.write(groupStrelka, Strelka.session, // every unit again
        body: {'s': 45, 'rate': 100, 'n': 1, 'acc': 16, 'gyro': 2000, 'live': 0});
    c.close();
    final s44 = (await sessionsOnSet(host, password, [1, 2, 3, 4], port: base))
        .firstWhere((s) => s.s == 44);
    expect(s44.belkas, [1, 3]);
    final head = await sessionHead(host, password, 1, 44, port: base);
    expect(head!.units, {1: kindZvjozdocka, 3: kindKryska});
    expect(head.place, 'left fore');
    expect(unitsMissing(s44, head), isEmpty);
    expect(unitsMissing(SessionOnSet(44)..records[1] = s44.records[1]!, head), [3]);
    final c3d = BytesBuilder(copy: false);
    expect((await downloadSession(host, password, s44, c3d.add, port: base)).over, isTrue);
    final text = String.fromCharCodes(c3d.takeBytes());
    expect(text.contains('Z1_AX') && text.contains('K3_GZ') && !text.contains('B1_AX'), isTrue);
  });

  test("a step lost is zeros, the channels named by the units' kinds, and the CSV, as strelka_c3d.py",
      () async {
    final dir = await Directory.systemTemp.createTemp('strelka_lost');
    final tools = '${Directory.current.path}/../../tools';
    const script = '''
import sys
sys.path.insert(0, sys.argv[1])
from test_record import make_session
import strelka_c3d as c3, strelka_record as rec
table = "kind;horse\\nbelka;chip;code;place;kind\\n3;0;LF;left fore;Z\\n4;0;RF;right fore;K\\n"
d = sys.argv[2]
a = make_session(session=1, belka=3, steps=40, lose=(5, 6), table=table)
b = make_session(session=1, belka=4, steps=40, first_period=1001, table=table)
open(d + "/a", "wb").write(a)
open(d + "/b", "wb").write(b)
c3.write(open(d + "/ref.c3d", "wb"), [rec.read(a), rec.read(b)])
c3.to_csv(open(d + "/ref.c3d", "rb").read(), open(d + "/ref.csv", "w"))
''';
    final r = await Process.run('python3', ['-c', script, tools, dir.path]);
    expect(r.exitCode, 0, reason: '${r.stdout}${r.stderr}');
    final a = await File('${dir.path}/a').readAsBytes();
    final b = await File('${dir.path}/b').readAsBytes();
    final ours = BytesBuilder(copy: false);
    composeFiles({3: a, 4: b},
        [RecordOnBelka(3, 40, 1000, a.length), RecordOnBelka(4, 40, 1001, b.length)], ours.add);
    final c3d = ours.takeBytes();
    expect(c3d, await File('${dir.path}/ref.c3d').readAsBytes());
    // and as CSV, the lost steps zeros and 1, as strelka_c3d.py writes it
    final csv = StringBuffer();
    c3dToCsv(c3d, csv);
    expect(csv.toString(), await File('${dir.path}/ref.csv').readAsString());
    expect(csv.toString().split('\n')[1 + 5], endsWith(',1,0'));
    await dir.delete(recursive: true);
  });

  test('the high-g channel beside: its HX, HY, HZ and its calibration, as strelka_c3d.py',
      () async {
    final dir = await Directory.systemTemp.createTemp('strelka_hg');
    final tools = '${Directory.current.path}/../../tools';
    const script = '''
import sys
sys.path.insert(0, sys.argv[1])
from test_record import make_session
import strelka_c3d as c3, strelka_record as rec
d = sys.argv[2]
a = make_session(session=2, belka=3, steps=250, lose=(5, 130), cal=True, hg=True)
b = make_session(session=2, belka=4, steps=200, first_period=1001)
open(d + "/a", "wb").write(a)
open(d + "/b", "wb").write(b)
c3.write(open(d + "/ref.c3d", "wb"), [rec.read(a), rec.read(b)])
c3.to_csv(open(d + "/ref.c3d", "rb").read(), open(d + "/ref.csv", "w"))
''';
    final r = await Process.run('python3', ['-c', script, tools, dir.path]);
    expect(r.exitCode, 0, reason: '${r.stdout}${r.stderr}');
    final a = await File('${dir.path}/a').readAsBytes();
    final b = await File('${dir.path}/b').readAsBytes();
    final head = (RecordReader((_) {})..add(a)).head!;
    expect(head.hasHg && head.hgRange == 320 && head.groups.length == 6, isTrue);
    expect(head.tzMin, 120);
    final ours = BytesBuilder(copy: false);
    composeFiles({3: a, 4: b},
        [RecordOnBelka(3, 250, 1000, a.length), RecordOnBelka(4, 200, 1001, b.length)], ours.add);
    final c3d = ours.takeBytes();
    expect(c3d, await File('${dir.path}/ref.c3d').readAsBytes());
    final csv = StringBuffer();
    c3dToCsv(c3d, csv);
    expect(csv.toString(), await File('${dir.path}/ref.csv').readAsString());
    expect(csv.toString().split('\n').first, contains('B3_HX,B3_HY,B3_HZ,B4_AX'));
    await dir.delete(recursive: true);
  });

  test('64/21 refused while the set records', () async {
    final c = await SmpClient.connect(host, port: base, password: password);
    await c.write(groupStrelka, Strelka.record, body: {'run': true});
    await expectLater(c.fetch(42),
        throwsA(isA<SmpError>().having((e) => e.rc, 'rc', Rc.badState)));
    await c.write(groupStrelka, Strelka.record, body: {'run': false});
    c.close();
  });
}
