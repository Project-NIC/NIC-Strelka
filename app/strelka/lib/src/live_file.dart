/// The live view saved on the phone (APP.md, The live view saved), tools/strelka_live.py's file
/// byte for byte: 8 B, "STRELKA" and the version; then records, each its kind u8, its unit u8 and
/// its length u16, little-endian, and its bytes unchanged — 1 a live view's message (unit 0), 2 an
/// SMP frame with Povodok (its number 0–3), 3 an SMP frame with a Belka (1–63). First the SMP
/// frames the export needs: 64/1 the session as it was sent, 64/18 the set's table as it was sent,
/// and every chosen unit's answers to 64/0 and 64/3, read before the start while SMP still goes
/// over ESB; then the messages, a period's in one, joined from every Povodok where the set is
/// gathered by the network. The export: CSV always, the raw steps as C3D too, in g and °/s by the
/// session's ranges and each unit's calibration, as a record from the card is converted.
///
///     final head = LiveSaveHead(session: body, table: tableBodies(csv), units: answers);
///     final saver = await LiveSaver.create(path, head);
///     await for (final d in receiver.datagrams) {
///       saver.add(d.data, lead: d.address.address == povodok);
///     }
///     await saver.close();
///     final saved = readSavedLive(await File(path).readAsBytes());
///     liveToCsv(saved, sink);
library;

import 'dart:io';
import 'dart:typed_data';

import 'c3d.dart';
import 'live_view.dart';
import 'places.dart';
import 'record.dart';
import 'smp.dart';

const String liveFileMark = 'STRELKA';
const int liveFileVersion = 1;
const int recordLive = 1, recordSmpPovodok = 2, recordSmpBelka = 3;
const int _group = 64;
const int _session = 1, _info = 0, _cal = 3, _table = 18;
const int _opReadRsp = 1, _opWrite = 2;
const int _hold = 2; // periods a period stays open for the other Povodoks' messages
const int _head = 21;

/// The sensors by their WHO_AM_I (64/0's `sensor`), their names, and the LSMs' steps: the
/// accelerometer's in µg by its range, the gyro's in µdps (DS13510 Rev 4 and DS14623 Rev 3,
/// table 3) — firmware/common/imu_tables.c's, as tools/strelka_live.py has them. An ICM's step is
/// the range / 32 768 (DS-000347 and DS-000639, tables 1 and 2), and so is an LSM's in a range its
/// table has not.
const Map<int, String> sensorNames = {
  0x47: 'ICM-42688-P',
  0xDB: 'ICM-42688-P',
  0x44: 'ICM-42686-P',
  0x70: 'LSM6DSV16X',
  0x73: 'LSM6DSV320X',
};
const Map<int, int> _lsmAccUg = {
  2: 61, 4: 122, 8: 244, 16: 488, 32: 976, 64: 1952, 128: 3904, 256: 7808, 320: 10417, //
};
const Map<int, int> _lsmGyroUdps = {
  125: 4375, 250: 8750, 500: 17500, 1000: 35000, 2000: 70000, 4000: 140000, //
};

bool _lsm(int whoami) => whoami == 0x70 || whoami == 0x73;

const List<int> _accAll = [2, 4, 8, 16, 32, 64, 128, 256, 320];
const List<int> _gyroAll = [125, 250, 500, 1000, 2000, 4000];

/// The accelerometer's ranges, ±g, a sensor has by its WHO_AM_I, 64/0's `sensor`: ±2 to ±16 g on
/// the LSM6DSV16X and the ICM-42688-P, to ±32 g on the ICM-42686-P, to ±320 g on the LSM6DSV320X,
/// above ±16 g its high-g channel alone (DS13510 Rev 4 and DS14623 Rev 3, table 3; DS-000347 and
/// DS-000639, table 2); every one 64/1 takes where the sensor is not known.
List<int> accRangesOf(int? whoami) => switch (whoami) {
      0x70 || 0x47 || 0xDB => const [2, 4, 8, 16],
      0x44 => const [2, 4, 8, 16, 32],
      _ => _accAll,
    };

/// The gyro's ranges, ±°/s, in whole degrees: from ±125 to ±4 000 on the LSM6DSV16X and the
/// ICM-42686-P, from ±250 on the LSM6DSV320X, to ±2 000 on the ICM-42688-P (the same tables, the
/// ICMs' table 1) — the ICMs' ±62,5 and below are no whole degrees, and 64/1 takes none of them.
List<int> gyroRangesOf(int? whoami) => switch (whoami) {
      0x73 => const [250, 500, 1000, 2000, 4000],
      0x47 || 0xDB => const [125, 250, 500, 1000, 2000],
      _ => _gyroAll,
    };

/// The ranges every one of [sensors] has, by [of]: what a session of those units may ask.
List<int> rangesOfAll(Iterable<int?> sensors, List<int> Function(int?) of) {
  var out = of(null);
  for (final s in sensors) {
    final mine = of(s);
    out = [for (final r in out) if (mine.contains(r)) r];
  }
  return out;
}

int accStepNg(int whoami, int accG) => _lsm(whoami) && _lsmAccUg.containsKey(accG)
    ? _lsmAccUg[accG]! * 1000
    : (accG * 1000000000 + 16384) ~/ 32768;

int gyroStepNdps(int whoami, int gyroDps) => _lsm(whoami) && _lsmGyroUdps.containsKey(gyroDps)
    ? _lsmGyroUdps[gyroDps]! * 1000
    : (gyroDps * 1000000000 + 16384) ~/ 32768;

Uint8List liveFileHead() => Uint8List.fromList([...liveFileMark.codeUnits, liveFileVersion]);

Uint8List liveRecord(int kind, int unit, List<int> data) {
  final h = ByteData(4)
    ..setUint8(0, kind)
    ..setUint8(1, unit)
    ..setUint16(2, data.length, Endian.little);
  return Uint8List.fromList([...h.buffer.asUint8List(), ...data]);
}

/// The SMP frames first in the file: 64/1's body as it was sent, the table's pieces of 64/18 as
/// they were sent, and each unit's answers to 64/0 and 64/3.
Uint8List liveSmpRecords(
  Map<String, Object?> session,
  List<Map<String, Object?>> tablePieces,
  List<(int, Map<String, Object?>, Map<String, Object?>)> units,
) {
  final out = BytesBuilder(copy: false)
    ..add(liveRecord(recordSmpPovodok, 0, smpFrame(_opWrite, _group, 0, _session, body: session)));
  for (final piece in tablePieces) {
    out.add(liveRecord(recordSmpPovodok, 0, smpFrame(_opWrite, _group, 0, _table, body: piece)));
  }
  for (final (n, info, cal) in units) {
    out
      ..add(liveRecord(recordSmpBelka, n, smpFrame(_opReadRsp, _group, 0, _info, body: info)))
      ..add(liveRecord(recordSmpBelka, n, smpFrame(_opReadRsp, _group, 0, _cal, body: cal)));
  }
  return out.takeBytes();
}

class _Period {
  _Period(this.head, this.lead);

  Uint8List head;
  bool lead;
  final Map<int, List<Uint8List>> belkas = {};
}

/// The messages of every Povodok into one a period, as they come, their bytes kept: the head from
/// the lead Povodok's message, or the first to come where it sent none; each Belka's entry once.
/// The copies of an entry are compared, its `by` aside: the same kept once, differing ones settled
/// by the larger part, and with none larger than half the step is taken as lost — the card keeps
/// the record. A period is written once a message two periods later has come, or at [finish]; a
/// message of a period already written is left out. Each period goes out as Povodok would send it,
/// in one message or in two of the same head (tools/live_view.py, split).
class LiveJoiner {
  LiveJoiner(this.emit);

  final void Function(Uint8List message) emit;
  final Map<int, _Period> _open = {};
  int _done = -1;

  /// Entries left out, their copies differing.
  int disagreed = 0;

  void add(Uint8List data, {bool lead = true}) {
    final m = parseLive(data);
    final p = m.period;
    if (p <= _done) return;
    final slot = _open.putIfAbsent(
      p,
      () => _Period(Uint8List.fromList(data.sublist(0, _head)), lead),
    );
    if (lead && !slot.lead) {
      slot
        ..head = Uint8List.fromList(data.sublist(0, _head))
        ..lead = true;
    }
    var at = _head;
    for (final b in m.belkas) {
      final len = 5 + b.code.steps * b.code.stepBytes;
      slot.belkas.putIfAbsent(b.n, () => []).add(Uint8List.fromList(data.sublist(at, at + len)));
      at += len;
    }
    for (final q in _open.keys.toList()..sort()) {
      if (q + _hold <= p) _out(q);
    }
  }

  void finish() {
    for (final q in _open.keys.toList()..sort()) {
      _out(q);
    }
  }

  void _out(int p) {
    final slot = _open.remove(p)!;
    if (p > _done) _done = p;
    final kept = <Uint8List>[];
    for (final n in slot.belkas.keys.toList()..sort()) {
      final copies = slot.belkas[n]!;
      final keys = [
        for (final c in copies) String.fromCharCodes([...c.sublist(0, 2), ...c.sublist(3)]),
      ];
      final counts = <String, int>{};
      for (final k in keys) {
        counts[k] = (counts[k] ?? 0) + 1;
      }
      final best = counts.entries.reduce((a, b) => b.value > a.value ? b : a);
      if (keys.length > 1 && best.value * 2 <= keys.length) {
        disagreed++;
        continue;
      }
      kept.add(copies[keys.indexOf(best.key)]);
    }
    var part = <Uint8List>[];
    var size = _head;
    void send() {
      final head = Uint8List.fromList(slot.head)..[20] = part.length;
      emit(Uint8List.fromList([...head, for (final e in part) ...e]));
    }

    for (final e in kept) {
      if (part.isNotEmpty && size + e.length > liveMaxBytes) {
        send();
        part = [];
        size = _head;
      }
      part.add(e);
      size += e.length;
    }
    send();
  }
}

/// What the file's SMP frames hold: 64/1's body and 64/18's as they were sent, and each chosen
/// unit's number and answers to 64/0 and 64/3, its calibration's empty where it has none.
class LiveSaveHead {
  const LiveSaveHead({required this.session, required this.table, required this.units});

  final Map<String, Object?> session;
  final List<Map<String, Object?>> table;
  final List<(int, Map<String, Object?>, Map<String, Object?>)> units;
}

/// The file written as the live view comes: its head and the SMP frames at once, then the messages
/// through a [LiveJoiner].
class LiveSaver {
  LiveSaver._(this._sink) {
    _joiner = LiveJoiner((m) => _write(liveRecord(recordLive, 0, m)));
  }

  final IOSink _sink;
  late final LiveJoiner _joiner;
  int _bytes = 0;

  /// Bytes written so far.
  int get bytes => _bytes;

  static Future<LiveSaver> create(String path, LiveSaveHead head) async {
    final s = LiveSaver._(File(path).openWrite());
    s
      .._write(liveFileHead())
      .._write(liveSmpRecords(head.session, head.table, head.units));
    return s;
  }

  void _write(Uint8List bytes) {
    _bytes += bytes.length;
    _sink.add(bytes);
  }

  /// A message as it came; [lead] when from the Povodok the application is joined to.
  void add(Uint8List message, {bool lead = true}) => _joiner.add(message, lead: lead);

  Future<void> close() async {
    _joiner.finish();
    await _sink.flush();
    await _sink.close();
  }
}

/// A saved live view read: the session (64/1's body), the table's text, each unit's info and
/// calibration by its number, and the messages in their order.
class SavedLive {
  Map<String, Object?> session = {};
  String table = '';
  final Map<int, Map<String, Object?>> info = {}, cal = {};
  final List<LiveMessage> messages = [];
}

SavedLive readSavedLive(Uint8List data) {
  if (data.length < 8 ||
      String.fromCharCodes(data.sublist(0, 7)) != liveFileMark ||
      data[7] != liveFileVersion) {
    throw const FormatException('not a saved live view');
  }
  final out = SavedLive();
  final pieces = <int, String>{};
  final b = ByteData.sublistView(data);
  var at = 8;
  while (at + 4 <= data.length) {
    final kind = data[at], unit = data[at + 1], len = b.getUint16(at + 2, Endian.little);
    final body = Uint8List.sublistView(data, at + 4, at + 4 + len);
    at += 4 + len;
    if (kind == recordLive) {
      out.messages.add(parseLive(body));
      continue;
    }
    final h = SmpHeader.parse(body);
    if (h.group != _group) continue;
    final value = h.length == 0
        ? <String, Object?>{}
        : decodeCborMap(Uint8List.sublistView(body, 8, 8 + h.length));
    if (kind == recordSmpPovodok && h.cmd == _session) {
      out.session = value;
    } else if (kind == recordSmpPovodok && h.cmd == _table) {
      pieces[value['off'] as int] = value['csv'] as String;
    } else if (kind == recordSmpBelka && h.cmd == _info) {
      out.info[unit] = value;
    } else if (kind == recordSmpBelka && h.cmd == _cal) {
      out.cal[unit] = value;
    }
  }
  out.table = [for (final k in pieces.keys.toList()..sort()) pieces[k]!].join();
  return out;
}

Map<int, String> _places(String table) {
  final out = <int, String>{};
  List<String>? cols;
  for (final r in tableRows(table)) {
    if (r.isNotEmpty && r.first == 'belka') {
      cols = r;
    } else if (cols != null && r.isNotEmpty && RegExp(r'^\d+$').hasMatch(r.first)) {
      final i = cols.indexOf('place');
      out[int.parse(r.first)] = i >= 0 && i < r.length ? r[i] : '';
    }
  }
  return out;
}

List<int> _ints(Object? v, int count) =>
    v is List ? [for (final x in v) x as int] : List<int>.filled(count, 0);

/// Every unit the file names or its messages carry, by number, as a record's head for the
/// conversion and the C3D (tools/strelka_live.py, heads).
Map<int, RecordHead> savedHeads(SavedLive saved) {
  final s = saved.session;
  final acc = s['acc'] as int? ?? 16, gyro = s['gyro'] as int? ?? 2000;
  final numbers = {...saved.info.keys, for (final m in saved.messages) ...m.belkas.map((b) => b.n)};
  final at = _places(saved.table);
  final kinds = {for (final n in numbers) n: saved.info[n]?['kind'] as int? ?? 0};
  final (code, _) = savedSteps(saved);
  final out = <int, RecordHead>{};
  for (final n in numbers.toList()..sort()) {
    final info = saved.info[n] ?? const {}, cal = saved.cal[n] ?? const {};
    final who = info['sensor'] as int? ?? 0;
    final chip = info['chip'];
    final range = _ints(cal['range'], 2);
    out[n] = RecordHead.of(
      session: s['s'] as int? ?? 0,
      belka: n,
      chip: chip is Uint8List && chip.length == 8
          ? ByteData.sublistView(chip).getUint64(0, Endian.little)
          : 0,
      rateHz: _rateOf(code),
      n: s['n'] as int? ?? 1,
      accG: acc,
      gyroDps: gyro,
      place: at[n] ?? '',
      units: kinds,
      sensorName: sensorNames[who] ?? '',
      accStepNg: accStepNg(who, acc),
      gyroStepNdps: gyroStepNdps(who, gyro),
      accCal: _ints(cal['acc'], 6),
      gyroScale: _ints(cal['gscale'], 3),
      gyroZero: _ints(cal['gzero'], 3),
      calTime: cal['t'] as int? ?? 0,
      calAccG: range[0],
      calGyroDps: range[1],
      calTemp: cal['temp'] as int? ?? 0,
      // both channels kept: the high-g channel's range, step and calibration
      hgRange: s['hg'] as int? ?? 0,
      hgStepNg: (s['hg'] as int? ?? 0) == 0 ? 0 : accStepNg(who, s['hg'] as int),
      hgCal: _ints(cal['hg'], 6),
      hgCalRange: cal['hgrange'] as int? ?? 0,
    );
  }
  return out;
}

int _rateOf(LiveCode? code) => code == LiveCode.quat100 || code == LiveCode.raw100 ? 100 : 50;

/// Each Belka's steps on the set's grid at the live view's rate, index = period × the steps a
/// period + j: the live code, and by number the steps, a [Quat] or a [RawStep] each; the code the
/// low-g one's, a period of 3 or 6 among them counted as 4 or 5 (tools/strelka_live.py, steps).
(LiveCode?, Map<int, Map<int, Object>>) savedSteps(SavedLive saved) {
  LiveCode? code;
  final out = <int, Map<int, Object>>{};
  for (final m in saved.messages) {
    for (final b in m.belkas) {
      if (b.code == LiveCode.off) continue;
      code ??= b.code.lowG;
      final spp = b.code.steps;
      final steps = b.code.quaternions ? b.quats : b.raws;
      for (var j = 0; j < steps.length; j++) {
        out.putIfAbsent(b.n, () => {})[m.period * spp + j] = steps[j];
      }
    }
  }
  return (code, out);
}

/// The C3D, as the download's, its SOURCE `live`: the raw steps in g and °/s, or the quaternions
/// as QW, QX, QY, QZ; a step lost zeros (tools/strelka_live.py, to_c3d).
Uint8List liveToC3d(SavedLive saved) {
  final (code, by) = savedSteps(saved);
  final quaternions = code == LiveCode.quat50 || code == LiveCode.quat100;
  final heads = savedHeads(saved);
  List<double> raw(RecordHead h, RawStep r) {
    // calibrated by the Belka (PRINCIPLE.md, The LIVE): by the sensor's steps alone, the
    // accelerometer's three by the high-g channel's step where the code said so
    final v = h.convert([...r.a, ...r.g], calibrated: false);
    return r.highG && h.hasHg ? [for (final x in r.a) x * h.hgStepNg * 1e-9, ...v.skip(3)] : v;
  }
  final all = by.values.expand((d) => d.keys);
  final first = all.isEmpty ? 0 : all.reduce((a, b) => a < b ? a : b);
  final data = <Map<int, List<double>>>[
    for (final MapEntry(key: n, value: h) in heads.entries)
      {
        for (final e in (by[n] ?? const {}).entries)
          e.key - first: switch (e.value) {
            Quat q => [q.w, q.x, q.y, q.z],
            RawStep r => raw(h, r),
            _ => const [],
          },
      },
  ];
  final width = quaternions ? 4 : 6;
  // the live view's one accelerometer a step, in g whichever channel: no HX, HY, HZ
  for (final h in heads.values) {
    h
      ..hgRange = 0
      ..hgStepNg = 0
      ..hgCal = const [0, 0, 0, 0, 0, 0]
      ..hgCalRange = 0;
  }
  final steps = data
      .map((d) => d.isEmpty ? 0 : d.keys.reduce((a, b) => a > b ? a : b) + 1)
      .fold(0, (a, b) => a > b ? a : b);
  var perFrame = (steps / 65535).ceil();
  if (perFrame < 1) perFrame = 1;
  final frames = steps > 0 ? (steps / perFrame).ceil() : 1;
  final out = BytesBuilder(copy: false)
    ..add(c3dHead(heads.values.toList(),
        rate: _rateOf(code),
        first: first,
        steps: steps,
        perFrame: perFrame,
        frames: frames,
        source: 'live',
        quaternions: quaternions));
  final zero = List<double>.filled(width, 0);
  final body = ByteData(4 * width * data.length * perFrame * frames);
  var at = 0;
  for (var k = 0; k < frames * perFrame; k++) {
    for (final d in data) {
      for (final v in d[k] ?? zero) {
        body.setFloat32(at, v, Endian.little);
        at += 4;
      }
    }
  }
  out.add(body.buffer.asUint8List());
  final pad = (512 - out.length % 512) % 512;
  out.add(Uint8List(pad));
  return out.takeBytes();
}

/// CSV by way of the C3D, as the download's CSV is (tools/strelka_live.py, to_csv).
void liveToCsv(SavedLive saved, StringSink out) => c3dToCsv(liveToC3d(saved), out);

/// The note the user gave the session, the set's table's line `note`; empty where it has none.
String savedNote(SavedLive saved) => tableNote(saved.table);
