/// A Belka's record as RECORD.md has it, read block by block as it comes in: blocks of 512 B, the
/// session's head, the set's table, data blocks of 40 steps with an information block after every
/// 120 and, where the record keeps the high-g channel, its two blocks of 60 after that, the end —
/// every number little-endian, every block ending in a CRC-32 over the 508 bytes before it. What
/// the application needs of it to write the C3D: the head, the place and the units from the table,
/// and the steps on the set's grid, converted as tools/strelka_record.py converts them, which this
/// follows.
library;

import 'dart:convert';
import 'dart:typed_data';

import 'places.dart';

const int blockBytes = 512;
const int stepsABlock = 40;
const double periodS = 0.020;
const int kindHead = 1;
const int kindData = 2;
const int kindEnd = 3;
const int kindTable = 4;
const int kindInfo = 5;
const int kindHg = 6; // the high-g channel's blocks, two after each information block
const int hgSteps = 60;
const int recordVersion = 4;
const int _groupsAt = 64, _groupBytes = 36, _groupsMax = 12;
const int _payload = 28; // where a data block's steps begin
const int _missing = -32768; // a step lost: every value
const int _tableText = 502; // a table block's text, after its 6 B (RECORD.md)
const int kindBelka = 0; // 64/0's `kind`, the head's kinds
const int kindKryska = 1;
const int kindZvjozdocka = 2;

/// A unit's kind as a letter: B a Belka, K Kryska, Z Zvjozdočka.
String kindLetter(int kind) => const {kindKryska: 'K', kindZvjozdocka: 'Z'}[kind] ?? 'B';

final Uint32List _crcTable = () {
  final t = Uint32List(256);
  for (var n = 0; n < 256; n++) {
    var c = n;
    for (var k = 0; k < 8; k++) {
      c = (c & 1) != 0 ? 0xEDB88320 ^ (c >> 1) : c >> 1;
    }
    t[n] = c;
  }
  return t;
}();

/// CRC-32, IEEE, zlib's.
int crc32(Uint8List data, [int start = 0, int? end]) {
  var c = 0xFFFFFFFF;
  for (var i = start; i < (end ?? data.length); i++) {
    c = _crcTable[(c ^ data[i]) & 0xFF] ^ (c >> 8);
  }
  return c ^ 0xFFFFFFFF;
}

bool blockOk(Uint8List b) =>
    b.length == blockBytes &&
    crc32(b, 0, blockBytes - 4) == ByteData.sublistView(b).getUint32(blockBytes - 4, Endian.little);

String _text(Uint8List b, int at, int len) {
  var end = at;
  while (end < at + len && b[end] != 0) {
    end++;
  }
  return utf8.decode(b.sublist(at, end), allowMalformed: true); // UTF-8, as the head keeps it
}

/// A group of the data, as the head's table has it (RECORD.md, The groups): the block kind it lies
/// in, its first value in the step and how many, their type, its name and unit, a value's step in
/// nano-units, its range, the calibration's range and the group's own six 16-bit words.
class RecordGroup {
  RecordGroup(this.kind, this.first, this.count, this.type, this.name, this.unit, this.step,
      this.range, this.calRange, this.cal);

  factory RecordGroup.parse(Uint8List b, int at) {
    final d = ByteData.sublistView(b);
    return RecordGroup(b[at], b[at + 1], b[at + 2], b[at + 3], _text(b, at + 4, 8),
        _text(b, at + 12, 4), d.getUint32(at + 16, Endian.little),
        d.getUint16(at + 20, Endian.little), d.getUint16(at + 22, Endian.little),
        [for (var i = 0; i < 6; i++) d.getUint16(at + 24 + 2 * i, Endian.little)]);
  }

  final int kind, first, count, type, step, range, calRange;
  final String name, unit;
  final List<int> cal;

  /// What 32 768 steps make: the step times 32 768, or the range where no step is given.
  int get full => step != 0 ? (step * 32768 / 1e9).round() : range;
}

int _signed(int v) => v >= 0x8000 ? v - 0x10000 : v;

/// The session's head, version 4 (RECORD.md, The session's head): its fixed part and its groups,
/// and what the tools take of them — the accelerometer and the gyro of the data blocks, the high-g
/// channel's where the record keeps it; the place and the units from the set's table, filled by
/// the RecordReader.
class RecordHead {
  RecordHead._(Uint8List b) : _d = ByteData.sublistView(b) {
    version = b[4];
    session = _d.getUint16(6, Endian.little);
    belka = b[8];
    chip = _d.getUint64(9, Endian.little);
    rateHz = _d.getUint16(20, Endian.little);
    n = b[22];
    clock = b[23];
    timeUs = _d.getUint64(24, Endian.little);
    timePeriod = _d.getUint32(32, Endian.little);
    tzMin = _d.getInt8(36) * 15;
    sensorName = _text(b, 37, 16);
    calTemp = _d.getInt16(53, Endian.little);
    calTime = _d.getUint32(55, Endian.little);
    final count = b[59] < _groupsMax ? b[59] : _groupsMax;
    groups.addAll(
        [for (var i = 0; i < count; i++) RecordGroup.parse(b, _groupsAt + i * _groupBytes)]);
    final acc = groups.where((g) => g.kind == kindData && g.unit == 'g').firstOrNull;
    final gyro = groups.where((g) => g.kind == kindData && g.unit == 'dps').firstOrNull;
    accRange = acc?.range ?? 0;
    accStepNg = acc?.step ?? 0;
    accG = acc?.full ?? 0;
    calAccG = acc?.calRange ?? 0;
    accCal = [for (final v in acc?.cal ?? const [0, 0, 0, 0, 0, 0]) _signed(v)];
    gyroRange = gyro?.range ?? 0;
    gyroStepNdps = gyro?.step ?? 0;
    gyroDps = gyro?.full ?? 0;
    calGyroDps = gyro?.calRange ?? 0;
    final gcal = gyro?.cal ?? const [0, 0, 0, 32768, 32768, 32768];
    gyroZero = [for (final v in gcal.take(3)) _signed(v)];
    gyroScale = [...gcal.skip(3)];
    final hg = groups.where((g) => g.kind == kindHg).firstOrNull;
    if (hg != null) {
      hgRange = hg.range;
      hgStepNg = hg.step;
      hgCalRange = hg.calRange;
      hgCal = [for (final v in hg.cal) _signed(v)];
    }
  }

  /// A head for steps that came another way than a record, the live view saved (live_file.dart):
  /// the fields given, the time and the sensor's own rate unknown, zeros.
  RecordHead.of({
    required this.session,
    required this.belka,
    required this.chip,
    required this.rateHz,
    required this.n,
    required this.accG,
    required this.gyroDps,
    required this.place,
    required Map<int, int> units,
    this.sensorName = '',
    this.hgRange = 0,
    this.hgStepNg = 0,
    this.hgCal = const [0, 0, 0, 0, 0, 0],
    this.hgCalRange = 0,
    this.accStepNg = 0,
    this.gyroStepNdps = 0,
    this.accCal = const [0, 0, 0, 0, 0, 0],
    this.gyroScale = const [0, 0, 0],
    this.gyroZero = const [0, 0, 0],
    this.calTime = 0,
    this.calTemp = 0,
    this.calAccG = 0,
    this.calGyroDps = 0,
  }) : _d = ByteData(0) {
    version = recordVersion;
    timeUs = 0;
    timePeriod = 0;
    accRange = accG;
    gyroRange = gyroDps;
    this.units.addAll(units);
  }

  final ByteData _d;
  late final int version, session, belka, chip, rateHz, n, accG, gyroDps, timeUs, timePeriod;

  /// How the time was kept (0 SYNC, 1 the fallback, 2 the sensor's own), the zone in minutes.
  int clock = 0, tzMin = 0;

  /// The head's groups, in their order.
  final List<RecordGroup> groups = [];

  /// The high-g channel kept in its own blocks: its range, step, six readings and the range of
  /// its calibration; hgRange 0 none.
  int hgRange = 0, hgStepNg = 0, hgCalRange = 0;
  List<int> hgCal = const [0, 0, 0, 0, 0, 0];
  bool get hasHg => hgRange != 0;
  late final List<int> accCal, gyroScale, gyroZero;
  late final int calTime;

  /// The calibration's temperature, 0,01 °C.
  int calTemp = 0;

  /// The unit's place, from the set's table.
  String place = '';
  String sensorName = '';
  int accStepNg = 0, gyroStepNdps = 0, sensorHz = 0, accRange = 0, gyroRange = 0;

  /// The ranges the calibration was made at, ±g and ±°/s; 0 the session's own.
  int calAccG = 0, calGyroDps = 0;

  /// The session's units, each one's number and kind, from the set's table; empty when unknown.
  final Map<int, int> units = {};

  /// The head of a block, version 4, or null when it is none.
  static RecordHead? parse(Uint8List b) => b.length == blockBytes &&
          String.fromCharCodes(b.sublist(0, 4)) == 'STRK' &&
          b[5] == kindHead &&
          b[4] == recordVersion
      ? RecordHead._(b)
      : null;

  /// The high-g channel's three in g: by its step, or by its own calibration
  /// (tools/strelka_record.py, hg_convert and hg_calibration).
  List<double> hgConvert(List<int> raw) {
    if (calTime == 0 || hgCal.every((v) => v == 0)) {
      final step = hgStepNg != 0 ? hgStepNg * 1e-9 : (hgRange / 32768);
      return [for (final v in raw) v * step];
    }
    final k = hgCalRange != 0 && hgRange != 0 ? hgCalRange / hgRange : 1.0;
    final c = [for (final v in hgCal) v * k];
    return [
      for (var i = 0; i < 3; i++)
        (raw[i] - (c[2 * i] + c[2 * i + 1]) / 2) /
            ((c[2 * i] - c[2 * i + 1]) / 2 == 0 ? 1.0 : (c[2 * i] - c[2 * i + 1]) / 2),
    ];
  }

  /// Steps in one period at the record's rate: 1 at 50 Hz, 640 at 32 kHz.
  int get stepsAPeriod {
    final s = (rateHz * periodS).round();
    return s < 1 ? 1 : s;
  }

  /// Six raw values into g and °/s: by the ranges, by the sensor's own steps where the head gives
  /// them, and by the calibration where there is one and [calibrated] is left true — false for the
  /// live view's steps, which the Belka calibrated (tools/strelka_record.py, convert).
  List<double> convert(List<int> raw, {bool calibrated = true}) {
    var acc = accG / 32768;
    var gyro = gyroDps / 32768;
    if (accStepNg != 0 && gyroStepNdps != 0) {
      acc = accStepNg * 1e-9;
      gyro = gyroStepNdps * 1e-9;
    }
    if (calTime == 0 || !calibrated) {
      return [for (var i = 0; i < 3; i++) raw[i] * acc, for (var i = 3; i < 6; i++) raw[i] * gyro];
    }
    // made in other ranges than the session's: the readings by the nominal steps (RECORD.md)
    final ka = calAccG == 0 ? 1.0 : calAccG / (accRange != 0 ? accRange : accG);
    final kg = calGyroDps == 0 ? 1.0 : calGyroDps / (gyroRange != 0 ? gyroRange : gyroDps);
    final c = [for (final v in accCal) v * ka];
    final zero = [for (var i = 0; i < 3; i++) (c[2 * i] + c[2 * i + 1]) / 2];
    final perG = [
      for (var i = 0; i < 3; i++)
        ((c[2 * i] - c[2 * i + 1]) / 2) == 0 ? 1.0 : (c[2 * i] - c[2 * i + 1]) / 2
    ];
    final scale = [for (final s in gyroScale) s / 32768 == 0 ? 1.0 : s / 32768];
    return [
      for (var i = 0; i < 3; i++) (raw[i] - zero[i]) / perG[i],
      for (var i = 0; i < 3; i++) (raw[3 + i] - gyroZero[i] * kg) * gyro / scale[i],
    ];
  }
}

/// A data block's steps on the set's grid: the first step's grid index and the raw six of each,
/// null for a step lost.
class DataBlock {
  DataBlock(this.session, this.number, this.firstIndex, this.steps);

  final int session, number, firstIndex;
  final List<List<int>?> steps;

  static DataBlock? parse(Uint8List b, int stepsAPeriod) {
    if (b[0] != kindData) return null;
    final d = ByteData.sublistView(b);
    final n = b[1];
    final period = d.getUint32(8, Endian.little);
    final place = b[12];
    final steps = <List<int>?>[];
    for (var i = 0; i < n; i++) {
      final raw = [for (var a = 0; a < 6; a++) d.getInt16(_payload + 12 * i + 2 * a, Endian.little)];
      steps.add(raw.every((v) => v == _missing) ? null : raw);
    }
    return DataBlock(d.getUint16(2, Endian.little), d.getUint32(4, Endian.little),
        period * stepsAPeriod + place * stepsABlock, steps);
  }
}

/// A high-g block's steps on the set's grid (RECORD.md, A high-g block): its cycle's first data
/// block's number, the first step's grid index — the cycle's first by the period and the place it
/// carries, and its half's 60 after — and the raw three of each, null for a step lost.
class HgBlock {
  HgBlock(this.session, this.number, this.firstIndex, this.steps);

  final int session, number, firstIndex;
  final List<List<int>?> steps;

  static HgBlock? parse(Uint8List b, int stepsAPeriod) {
    if (b[0] != kindHg) return null;
    final d = ByteData.sublistView(b);
    final n = b[1] < hgSteps ? b[1] : hgSteps;
    final steps = <List<int>?>[];
    for (var i = 0; i < n; i++) {
      final raw = [for (var a = 0; a < 3; a++) d.getInt16(_payload + 6 * i + 2 * a, Endian.little)];
      steps.add(raw.every((v) => v == _missing) ? null : raw);
    }
    return HgBlock(d.getUint16(2, Endian.little), d.getUint32(4, Endian.little),
        d.getUint32(8, Endian.little) * stepsAPeriod + b[12] * stepsABlock + b[13] * hgSteps,
        steps);
  }
}

/// A record coming in by its bytes, in order: whole blocks read as they complete, the head kept,
/// the place and the units put into it from the set's table, the data and the high-g blocks handed
/// on; a block failing its CRC or of another session skipped, as a reader must (RECORD.md, The
/// end).
class RecordReader {
  RecordReader(this.onData, {this.onHg});

  final void Function(DataBlock block) onData;
  final void Function(HgBlock block)? onHg;
  final BytesBuilder _pending = BytesBuilder(copy: false);
  final Map<int, Uint8List> _table = {};
  int _tableBytes = -1;
  bool _tableRead = false, _past = false;
  RecordHead? head;

  /// The head is whole: its table read into it, or a block after the table's place come, or none.
  bool get settled => head != null && (_tableRead || _past);

  /// The set's table once its blocks have all come (RECORD.md, The set's table); null till then,
  /// and for a record that has none.
  String? get table {
    if (_tableBytes < 0) return null;
    final all = BytesBuilder(copy: false);
    for (var i = 0; all.length < _tableBytes; i++) {
      final part = _table[i];
      if (part == null) return null;
      all.add(part);
    }
    return utf8.decode(all.takeBytes().sublist(0, _tableBytes), allowMalformed: true);
  }

  void add(Uint8List bytes) {
    _pending.add(bytes);
    if (_pending.length < blockBytes) return;
    final all = _pending.takeBytes();
    var at = 0;
    for (; at + blockBytes <= all.length; at += blockBytes) {
      _block(Uint8List.sublistView(all, at, at + blockBytes));
    }
    if (at < all.length) _pending.add(Uint8List.sublistView(all, at));
  }

  void _block(Uint8List b) {
    if (!blockOk(b)) return;
    final h = RecordHead.parse(b);
    if (h != null) {
      head ??= h;
      return;
    }
    final own = head;
    if (own == null) return;
    final d = ByteData.sublistView(b);
    if (d.getUint16(2, Endian.little) != own.session) return;
    if (b[0] == kindTable) {
      _tableBytes = d.getUint16(4, Endian.little);
      _table[b[1]] = Uint8List.fromList(b.sublist(6, 6 + _tableText));
      final text = _tableRead ? null : table;
      if (text != null) _units(own, text);
      return;
    }
    _past = true;
    final data = DataBlock.parse(b, own.stepsAPeriod);
    if (data != null) onData(data);
    final hg = onHg == null ? null : HgBlock.parse(b, own.stepsAPeriod);
    if (hg != null) onHg!(hg);
  }

  /// The session's units and the unit's place from the table's rows under the line naming the
  /// columns, each unit's kind by its letter, B where none (tools/strelka_record.py, table_units).
  void _units(RecordHead own, String text) {
    _tableRead = true;
    List<String>? cols;
    for (final r in tableRows(text)) {
      if (r.isNotEmpty && r[0] == 'belka') {
        cols = r;
      } else if (cols != null && r.isNotEmpty && RegExp(r'^[0-9]+$').hasMatch(r[0])) {
        final row = {for (var i = 0; i < r.length && i < cols.length; i++) cols[i]: r[i]};
        final n = int.parse(r[0]);
        own.units[n] =
            const {'K': kindKryska, 'Z': kindZvjozdocka}[row['kind'] ?? 'B'] ?? kindBelka;
        if (n == own.belka) own.place = row['place'] ?? '';
      }
    }
  }
}
