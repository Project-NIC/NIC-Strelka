@TestOn('vm')
library;

import 'dart:io';
import 'dart:typed_data';

import 'package:strelka/strelka.dart';
import 'package:test/test.dart';

/// The live view saved (APP.md, The live view saved) held to tools/strelka_live.py: the messages of
/// several Povodoks joined into the same bytes, the file [LiveSaver] writes the same as the
/// Python one, and its export, CSV and C3D, the same to the last byte. Python 3 with cbor2 is
/// wanted.

const String script = r'''
import os, sys
sys.path.insert(0, sys.argv[1])
import live_view as lv, strelka_live as sl, test_live as t
d = sys.argv[2]
def dump(name, items):
    with open(os.path.join(d, name), "wb") as f:
        for lead, m in items:
            f.write(bytes([lead]) + len(m).to_bytes(2, "little") + m)
ins = [(0, t.message(2000, (5, 7, 9), by=2)[0]),
       (0, t.message(2000, (7, 9, 11), by=3, alter=(7,))[0]),
       (1, t.message(2000, (3, 5, 9, 11), by=0, alter=(9,))[0])]
ins += [(1, m) for m in t.message(2001, range(1, 51))]
ins += [(1, t.message(2003, (3,))[0])]
out = []
j = sl.Joiner(out.append)
for lead, m in ins:
    j.add(m, lead=bool(lead))
j.finish()
dump("joined.in", ins)
dump("joined.out", [(1, m) for m in out])
for name, code, periods, lose in (("raw", lv.RAW_100, range(500, 510), (504,)),
                                  ("quat", lv.QUAT_50, range(300, 304), (302,))):
    data = t.saved_file(code, periods, lose)
    open(os.path.join(d, name + ".live"), "wb").write(data)
    dump(name + ".in", [(1, m) for p in periods if p not in lose
                        for m in t.message(p, (3, 5), code=code)])
    saved = sl.read(data)
    with open(os.path.join(d, name + ".csv"), "w") as f:
        sl.to_csv(saved, f)
    with open(os.path.join(d, name + ".c3d"), "wb") as f:
        sl.to_c3d(saved, f)
# both channels kept: period 501 the high-g channel's, code 6
out = bytearray(sl.file_head())
out += sl.smp_records(dict(t.SESSION, live=lv.RAW_100, acc=16, hg=320), [],
                      [(3, t.info(3, 0x73, kind=1), {"hg": [900, -900, 920, -880, 910, -890],
                                                     "hgrange": 32, "t": 5})])
j = sl.Joiner(lambda m: out.extend(sl.record(sl.LIVE, 0, m)))
for p, code in ((500, lv.RAW_100), (501, lv.RAW_100_HG)):
    for m in t.message(p, (3,), code=code):
        j.add(m)
j.finish()
open(os.path.join(d, "hg.live"), "wb").write(out)
saved = sl.read(bytes(out))
with open(os.path.join(d, "hg.csv"), "w") as f:
    sl.to_csv(saved, f)
with open(os.path.join(d, "hg.c3d"), "wb") as f:
    sl.to_c3d(saved, f)
''';

/// The messages a file of the script's holds, each with whether it is the lead Povodok's.
List<(bool, Uint8List)> messages(Uint8List data) {
  final out = <(bool, Uint8List)>[];
  var at = 0;
  while (at < data.length) {
    final len = data[at + 1] | data[at + 2] << 8;
    out.add((data[at] == 1, Uint8List.sublistView(data, at + 3, at + 3 + len)));
    at += 3 + len;
  }
  return out;
}

void main() {
  late Directory dir;

  setUpAll(() async {
    dir = await Directory.systemTemp.createTemp('strelka_live');
    final tools = '${Directory.current.path}/../../tools';
    final r = await Process.run('python3', ['-c', script, tools, dir.path]);
    expect(r.exitCode, 0, reason: '${r.stdout}${r.stderr}');
  });

  tearDownAll(() => dir.delete(recursive: true));

  Future<Uint8List> bytes(String name) => File('${dir.path}/$name').readAsBytes();

  test('the copies of several Povodoks joined as the Python joins them', () async {
    final out = <Uint8List>[];
    final j = LiveJoiner(out.add);
    for (final (lead, m) in messages(await bytes('joined.in'))) {
      j.add(m, lead: lead);
    }
    j.finish();
    final want = [for (final (_, m) in messages(await bytes('joined.out'))) m];
    expect(out, want);
    expect(out.length, 4); // period 2000; 2001's 50 Belkas in two; 2003
    expect(j.disagreed, 1);
    expect(parseLive(out.first).belkas.map((b) => b.n), [3, 5, 9, 11]);
  });

  test('the file written as the Python writes it, its SMP frames first', () async {
    final want = await bytes('raw.live');
    final saved = readSavedLive(want);
    expect(saved.session['s'], 7);
    expect(saved.info[5]!['kind'], kindKryska);
    final path = '${dir.path}/ours.live';
    final saver = await LiveSaver.create(
        path,
        LiveSaveHead(
            session: saved.session,
            table: tableBodies(saved.table),
            units: [for (final n in [3, 5]) (n, saved.info[n]!, saved.cal[n]!)]));
    for (final (lead, m) in messages(await bytes('raw.in'))) {
      saver.add(m, lead: lead);
    }
    await saver.close();
    expect(await File(path).readAsBytes(), want);
    expect(saver.bytes, want.length);
  });

  test('the raw steps exported as the Python does, C3D and CSV', () async {
    final saved = readSavedLive(await bytes('raw.live'));
    final heads = savedHeads(saved);
    expect(heads[3]!.place, 'left fore');
    expect(heads[3]!.gyroStepNdps, 140000000);
    expect(heads[5]!.gyroStepNdps, 122070313);
    expect(liveToC3d(saved), await bytes('raw.c3d'));
    final csv = StringBuffer();
    liveToCsv(saved, csv);
    expect(csv.toString(), await File('${dir.path}/raw.csv').readAsString());
  });

  test('codes 3 and 6: a period from the high-g channel by its own step, as the Python does',
      () async {
    final saved = readSavedLive(await bytes('hg.live'));
    final h = savedHeads(saved)[3]!;
    expect(h.hgStepNg, 10417000);
    expect(h.hgCalRange, 32);
    expect(liveToC3d(saved), await bytes('hg.c3d'));
    final csv = StringBuffer();
    liveToCsv(saved, csv);
    expect(csv.toString(), await File('${dir.path}/hg.csv').readAsString());
    expect(csv.toString().split('\n').first, startsWith('step,t_s,K3_AX,'));
  });

  test('the quaternions as the C3D\'s four channels and its CSV, as the Python does', () async {
    final saved = readSavedLive(await bytes('quat.live'));
    expect(liveToC3d(saved), await bytes('quat.c3d'));
    final csv = StringBuffer();
    liveToCsv(saved, csv);
    expect(csv.toString(), await File('${dir.path}/quat.csv').readAsString());
    expect(csv.toString().split('\n').first, startsWith('step,t_s,B3_QW,B3_QX,B3_QY,B3_QZ,'));
  });

  test("a session's ranges: those every unit's sensor has, ±2 g and ±125 °/s the finest", () {
    expect(rangesOfAll([0x70, 0x73], accRangesOf), [2, 4, 8, 16]);
    expect(rangesOfAll([0x73], accRangesOf).last, 320);
    expect(rangesOfAll([0x44, null], accRangesOf), [2, 4, 8, 16, 32]);
    expect(rangesOfAll([0x70, 0x73], gyroRangesOf), [250, 500, 1000, 2000, 4000]);
    expect(rangesOfAll([0x70, 0x47], gyroRangesOf), [125, 250, 500, 1000, 2000]);
    expect(rangesOfAll(<int?>[], gyroRangesOf).first, 125);
  });

  test('the high-g channel offered where a unit has the LSM6DSV320X', () {
    expect(hgRangesFor([0x70]), isEmpty);
    expect(hgRangesFor([0x70, lsm320]).last, 320);
    expect(hgRangesFor([null, lsm320]).first, 32);
  });

  test('the sensors\' steps: an LSM\'s by its table, an ICM\'s the range / 32 768', () {
    expect(accStepNg(0x70, 16), 488000);
    expect(accStepNg(0x73, 320), 10417000);
    expect(gyroStepNdps(0x73, 2000), 70000000);
    expect(accStepNg(0x44, 32), 976563);
    expect(gyroStepNdps(0x47, 2000), 61035156);
  });

  test('the set\'s table read as RFC 4180 has it', () {
    expect(tableRows('belka;chip;code;place\n3;F0;LF;"left; ""fore"""\r\n\n'), [
      ['belka', 'chip', 'code', 'place'],
      ['3', 'F0', 'LF', 'left; "fore"'],
      <String>[],
    ]);
  });
}
