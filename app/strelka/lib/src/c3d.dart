/// The set's session as one C3D, written as the Belkas' records come in (APP.md, The download): a
/// frame a step, each Belka's six channels side by side on the set's grid, the high-g channel's
/// three after them where her record keeps it, the heads in the group
/// STRELKA — tools/strelka_c3d.py's file, byte for byte, written in one pass. The frame count is
/// known before the first byte comes, from each record's steps and first period that 64/2 gives
/// (COMMANDS.md); a frame goes out once every Belka has its steps or is done. A step lost is zeros,
/// every value, never a reading (APP.md, The export); a Belka gone has zeros from where she
/// stopped. A channel is named by its unit's kind and number, B3_AX, K7_GZ, by the session's units
/// in the heads.
library;

import 'dart:convert';
import 'dart:typed_data';

import 'record.dart';

const List<String> _axes = ['AX', 'AY', 'AZ', 'GX', 'GY', 'GZ'];
const List<String> _units = ['g', 'g', 'g', 'dps', 'dps', 'dps'];
const List<String> _quats = ['QW', 'QX', 'QY', 'QZ'];
const List<String> _hgAxes = ['HX', 'HY', 'HZ'];
const int _maxFrames = 65535;
const int _processorIntel = 84;

/// What 64/2 says of a Belka's file of the session: her number, its steps and its first period.
class RecordOnBelka {
  const RecordOnBelka(this.belka, this.steps, this.firstPeriod, this.bytes);

  final int belka, steps, firstPeriod, bytes;
}

/// The parameter section, packed as C3D wants it (tools/strelka_c3d.py, Params).
class _Params {
  final List<(Uint8List, Uint8List)> _records = [];
  final Map<String, int> _ids = {};

  void group(String name, [String desc = '']) {
    final gid = -(_ids.length + 1);
    _ids[name] = -gid;
    _records.add(_record(gid, name, Uint8List(0), desc));
  }

  void _param(String group, String name, int kind, List<int> dims, List<int> data,
      [String desc = '']) {
    final body = BytesBuilder()
      ..addByte(kind & 0xFF)
      ..addByte(dims.length)
      ..add(dims)
      ..add(data);
    _records.add(_record(_ids[group]!, name, body.takeBytes(), desc));
  }

  static (Uint8List, Uint8List) _record(int gid, String name, Uint8List body, String desc) {
    final n = utf8.encode(name), d = utf8.encode(desc);
    final tail = BytesBuilder()
      ..add(body)
      ..addByte(d.length)
      ..add(d);
    final head = BytesBuilder()
      ..addByte(n.length)
      ..addByte(gid & 0xFF)
      ..add(n);
    return (head.takeBytes(), tail.takeBytes());
  }

  static Uint8List _i16(List<int> v) {
    final b = ByteData(2 * v.length);
    for (var i = 0; i < v.length; i++) {
      b.setInt16(2 * i, v[i], Endian.little);
    }
    return b.buffer.asUint8List();
  }

  static Uint8List _f32(List<double> v) {
    final b = ByteData(4 * v.length);
    for (var i = 0; i < v.length; i++) {
      b.setFloat32(4 * i, v[i], Endian.little);
    }
    return b.buffer.asUint8List();
  }

  void int16(String g, String n, int v) => _param(g, n, 2, [], _i16([v]));
  void uint16(String g, String n, int v) => _param(g, n, 2, [], _i16([v.toSigned(16)]));
  void int16s(String g, String n, List<int> v) => _param(g, n, 2, [v.length], _i16(v));
  void float32(String g, String n, double v) => _param(g, n, 4, [], _f32([v]));
  void floats(String g, String n, List<double> v) => _param(g, n, 4, [v.length], _f32(v));

  void strings(String g, String n, List<String> values, int width) {
    final data = <int>[];
    for (final v in values) {
      final e = utf8.encode(v);
      data.addAll([...e.take(width), for (var i = e.length; i < width; i++) 0x20]);
    }
    _param(g, n, -1, [width, values.length], data);
  }

  void string(String g, String n, String v) {
    final e = utf8.encode(v);
    _param(g, n, -1, [e.length], e);
  }

  Uint8List pack() {
    final out = BytesBuilder()..add([1, 80, 0, _processorIntel]);
    for (var i = 0; i < _records.length; i++) {
      final (head, tail) = _records[i];
      final offset = i == _records.length - 1 ? 0 : 2 + tail.length;
      out
        ..add(head)
        ..add(_i16([offset]))
        ..add(tail);
    }
    final bytes = out.takeBytes();
    final blocks = (bytes.length + 511) ~/ 512;
    bytes[2] = blocks;
    return Uint8List(blocks * 512)..setAll(0, bytes);
  }
}

/// A chip's ID as tools/strelka_c3d.py writes it: 16 hex digits, the top bit too.
String _hex16(int chip) =>
    BigInt.from(chip).toUnsigned(64).toRadixString(16).toUpperCase().padLeft(16, '0');

class _Belka {
  _Belka(this.on) : reader = null;

  final RecordOnBelka on;
  RecordReader? reader;
  int expected = 0; // the next byte awaited
  int covered = -1; // the last step index on the frames' grid given
  int hgCovered = -1; // the same for the high-g channel's blocks
  bool done = false;
  final Map<int, List<double>> steps = {};
  final Map<int, List<double>> hg = {};

  bool get hasHg => reader!.head!.hasHg;
  int get width => hasHg ? 9 : 6;
  int get through => hasHg && hgCovered < covered ? hgCovered : covered;
}

/// The C3D's header and parameters for the units of [heads] side by side, [steps] steps from the
/// grid index [first] at [rate] Hz in [frames] frames of [perFrame] (tools/strelka_c3d.py,
/// write_units): the frames follow, each unit's six float32 a step and the high-g channel's three
/// where its head keeps it, or a quaternion's four. The group STRELKA carries the heads, [source]
/// `card` or `live`, and each unit's sensor's steps and calibration as its head has them, raw.
Uint8List c3dHead(List<RecordHead> heads,
    {required int rate,
    required int first,
    required int steps,
    required int perFrame,
    required int frames,
    String source = 'card',
    bool quaternions = false}) {
    List<String> axesOf(RecordHead h) =>
        quaternions ? _quats : (h.hasHg ? [..._axes, ..._hgAxes] : _axes);
    List<String> unitsOf(RecordHead h) =>
        quaternions ? const ['1', '1', '1', '1'] : (h.hasHg ? [..._units, 'g', 'g', 'g'] : _units);
    final nCh = heads.fold(0, (n, h) => n + axesOf(h).length);
    final labels = [
      for (final h in heads)
        for (final a in axesOf(h)) '${kindLetter(h.units[h.belka] ?? kindBelka)}${h.belka}_$a'
    ];
    final p = _Params()
      ..group('POINT', 'no 3D points: the units\' channels are analog')
      ..int16('POINT', 'USED', 0)
      ..uint16('POINT', 'FRAMES', frames)
      ..float32('POINT', 'SCALE', -1.0)
      ..float32('POINT', 'RATE', rate / perFrame)
      ..strings('POINT', 'LABELS', [], 4)
      ..strings('POINT', 'DESCRIPTIONS', [], 4)
      ..strings('POINT', 'UNITS', ['mm'], 4)
      ..group('ANALOG', 'the units\' accelerometers and gyros')
      ..int16('ANALOG', 'USED', nCh)
      ..strings('ANALOG', 'LABELS', labels, 16)
      ..strings('ANALOG', 'DESCRIPTIONS', labels, 16)
      ..floats('ANALOG', 'SCALE', List.filled(nCh, 1.0))
      ..int16s('ANALOG', 'OFFSET', List.filled(nCh, 0))
      ..float32('ANALOG', 'GEN_SCALE', 1.0)
      ..strings('ANALOG', 'UNITS', [for (final h in heads) ...unitsOf(h)], 4)
      ..float32('ANALOG', 'RATE', rate.toDouble())
      ..string('ANALOG', 'FORMAT', 'SIGNED')
      ..int16('ANALOG', 'BITS', 16)
      ..group('STRELKA', 'the records\' heads')
      ..uint16('STRELKA', 'SESSION', heads.first.session)
      ..int16s('STRELKA', 'BELKA', [for (final h in heads) h.belka])
      ..strings('STRELKA', 'CHIP',
          [for (final h in heads) _hex16(h.chip)], 16)
      ..strings('STRELKA', 'PLACE', [for (final h in heads) h.place], 32)
      ..int16s('STRELKA', 'N', [for (final h in heads) h.n])
      ..int16s('STRELKA', 'SENSOR_HZ', [for (final h in heads) h.sensorHz])
      ..int16s('STRELKA', 'ACC_G', [for (final h in heads) h.accG])
      ..int16s('STRELKA', 'GYRO_DPS', [for (final h in heads) h.gyroDps])
      ..strings('STRELKA', 'SENSOR', [for (final h in heads) h.sensorName], 16)
      ..strings('STRELKA', 'TIME_US', [for (final h in heads) '${h.timeUs}'], 20)
      ..strings('STRELKA', 'TIME_PERIOD', [for (final h in heads) '${h.timePeriod}'], 12)
      ..string('STRELKA', 'FIRST_GRID_INDEX', '$first')
      ..int16('STRELKA', 'STEPS_PER_FRAME', perFrame)
      ..string('STRELKA', 'STEPS', '$steps')
      ..string('STRELKA', 'SOURCE', source)
      ..int16s('STRELKA', 'ACC_RANGE',
          [for (final h in heads) h.accRange != 0 ? h.accRange : h.accG])
      ..int16s('STRELKA', 'GYRO_RANGE',
          [for (final h in heads) h.gyroRange != 0 ? h.gyroRange : h.gyroDps])
      ..strings('STRELKA', 'ACC_STEP_NG', [for (final h in heads) '${h.accStepNg}'], 12)
      ..strings('STRELKA', 'GYRO_STEP_NDPS', [for (final h in heads) '${h.gyroStepNdps}'], 12)
      ..strings('STRELKA', 'CAL_TIME', [for (final h in heads) '${h.calTime}'], 12)
      ..int16s('STRELKA', 'CAL_ACC', [for (final h in heads) ...h.accCal])
      ..floats('STRELKA', 'CAL_GSCALE',
          [for (final h in heads) ...h.gyroScale.map((v) => v.toDouble())])
      ..int16s('STRELKA', 'CAL_GZERO', [for (final h in heads) ...h.gyroZero])
      ..int16s('STRELKA', 'CAL_RANGE', [for (final h in heads) ...[h.calAccG, h.calGyroDps]])
      ..int16s('STRELKA', 'CAL_TEMP', [for (final h in heads) h.calTemp])
      ..int16s('STRELKA', 'HG_RANGE', [for (final h in heads) h.hgRange])
      ..strings('STRELKA', 'HG_STEP_NG', [for (final h in heads) '${h.hgStepNg}'], 12)
      ..int16s('STRELKA', 'CAL_HG', [for (final h in heads) ...h.hgCal])
      ..int16s('STRELKA', 'CAL_HG_RANGE', [for (final h in heads) h.hgCalRange]);
    // POINT:DATA_START needs the section's own size, as tools/strelka_c3d.py finds it
    var dataStart = 2 + p.pack().length ~/ 512;
    Uint8List params;
    while (true) {
      p.int16('POINT', 'DATA_START', dataStart);
      params = p.pack();
      if (dataStart == 2 + params.length ~/ 512) break;
      dataStart = 2 + params.length ~/ 512;
    }
    final header = ByteData(512)
      ..setUint8(0, 2)
      ..setUint8(1, 80)
      ..setUint16(2, 0, Endian.little)
      ..setUint16(4, nCh * perFrame, Endian.little)
      ..setUint16(6, 1, Endian.little)
      ..setUint16(8, frames, Endian.little)
      ..setUint16(10, 0, Endian.little)
      ..setFloat32(12, -1.0, Endian.little)
      ..setUint16(16, dataStart, Endian.little)
      ..setUint16(18, perFrame, Endian.little)
      ..setFloat32(20, rate / perFrame, Endian.little)
      ..setUint16(298, 12345, Endian.little);
  return Uint8List.fromList([...header.buffer.asUint8List(), ...params]);
}

/// The C3D composed as the records come: [add] each piece of a Belka's file at its offset, [gone]
/// when she is done without it whole, [finish] at the download's end. What it writes goes to
/// [sink] in order, a header and the parameters once every Belka's head has come, the frames as
/// they are whole.
class C3dComposer {
  C3dComposer(List<RecordOnBelka> records, this.sink, {this.source = 'card'})
      : _belkas = {for (final r in records) r.belka: _Belka(r)} {
    for (final b in _belkas.values) {
      b.reader = RecordReader((block) => _data(b, block), onHg: (block) => _hg(b, block));
    }
  }

  final void Function(Uint8List bytes) sink;
  final String source;
  final Map<int, _Belka> _belkas;

  /// The set's table, from the first record by number that has brought it whole; null till then.
  String? get table {
    for (final n in _belkas.keys.toList()..sort()) {
      final t = _belkas[n]!.reader?.table;
      if (t != null) return t;
    }
    return null;
  }

  /// The first head by number that has come, for the file's name; null till one has.
  RecordHead? get head {
    for (final n in _belkas.keys.toList()..sort()) {
      final h = _belkas[n]!.reader?.head;
      if (h != null) return h;
    }
    return null;
  }
  List<_Belka> _in = []; // in the file, by number: those whose head came
  int _first = 0, _steps = 0, _perFrame = 1, _frames = 1, _next = 0;
  bool _headed = false;
  int _written = 0;

  /// Frames written so far, and in all once the header is out.
  int get framesWritten => _next;
  int get frames => _frames;

  /// A piece of Belka [belka]'s file from byte [off]; a piece not at the byte awaited is dropped.
  void add(int belka, int off, Uint8List data) {
    final b = _belkas[belka];
    if (b == null || b.done || off != b.expected) return;
    b.expected += data.length;
    b.reader!.add(data);
    if (b.expected >= b.on.bytes || data.isEmpty) _end(b);
    _flush();
  }

  /// Belka [belka] done without her file whole: zeros from where she stopped.
  void gone(int belka) {
    final b = _belkas[belka];
    if (b != null && !b.done) _end(b);
    _flush();
  }

  /// The download over: every Belka not done taken as gone, the frames out to the end.
  void finish() {
    for (final b in _belkas.values) {
      if (!b.done) _end(b);
    }
    _flush();
    final pad = (512 - _written % 512) % 512;
    if (pad > 0) _out(Uint8List(pad));
  }

  void _end(_Belka b) {
    b.done = true;
    b.covered = 1 << 62;
    b.hgCovered = 1 << 62;
  }

  void _data(_Belka b, DataBlock block) {
    final h = b.reader!.head!;
    for (var j = 0; j < block.steps.length; j++) {
      final raw = block.steps[j];
      final k = block.firstIndex + j - _firstOr(h);
      if (raw != null) b.steps[k] = h.convert(raw); // a step lost left out: zeros
      b.covered = k;
    }
  }

  void _hg(_Belka b, HgBlock block) {
    final h = b.reader!.head!;
    for (var j = 0; j < block.steps.length; j++) {
      final raw = block.steps[j];
      final k = block.firstIndex + j - _firstOr(h);
      if (raw != null) b.hg[k] = h.hgConvert(raw); // lost: zeros
      if (k > b.hgCovered) b.hgCovered = k;
    }
  }

  // before the header, the grid's first is not yet known: steps kept by the record's own first
  int _firstOr(RecordHead h) => _headed ? _first : 0;

  void _flush() {
    if (!_headed) {
      // the place and the units from the table: each head settled, or its Belka done
      if (_belkas.values.any((b) => !b.done && !b.reader!.settled)) return;
      _header();
    }
    final need = _in.isEmpty ? 0 : _in.map((b) => b.through).reduce((a, c) => a < c ? a : c);
    final zero = List<double>.filled(9, 0);
    final width = _in.fold(0, (n, b) => n + b.width);
    while (_next < _frames && (_next + 1) * _perFrame - 1 <= need) {
      final out = ByteData(4 * width * _perFrame);
      var at = 0;
      for (var sub = 0; sub < _perFrame; sub++) {
        final k = _next * _perFrame + sub;
        for (final b in _in) {
          final six = b.steps.remove(k);
          final hg = b.hg.remove(k);
          // the high-g channel's three only beside a step the data blocks have (strelka_c3d.py)
          final v = six == null ? zero : [...six, ...?(b.hasHg ? (hg ?? zero) : null)];
          for (var a = 0; a < b.width; a++) {
            out.setFloat32(at, v[a], Endian.little);
            at += 4;
          }
        }
      }
      _out(out.buffer.asUint8List());
      _next++;
    }
  }

  void _header() {
    _headed = true;
    _in = (_belkas.values.where((b) => b.reader!.head != null).toList()
      ..sort((a, c) => a.on.belka.compareTo(c.on.belka)));
    if (_in.isEmpty) {
      _frames = 0;
      return;
    }
    final heads = [for (final b in _in) b.reader!.head!];
    final rate = heads.first.rateHz;
    final spp = heads.first.stepsAPeriod;
    _first = _in.map((b) => b.on.firstPeriod * spp).reduce((a, c) => a < c ? a : c);
    final ends = _in.map((b) => b.on.firstPeriod * spp + b.on.steps).reduce((a, c) => a > c ? a : c);
    _steps = ends - _first;
    _perFrame = _steps > 0 ? (_steps / _maxFrames).ceil() : 1;
    if (_perFrame < 1) _perFrame = 1;
    _frames = _steps > 0 ? (_steps / _perFrame).ceil() : 1;
    // the steps already in, kept by the records' own first, shifted onto the frames' grid
    for (final b in _in) {
      final moved = {for (final e in b.steps.entries) e.key - _first: e.value};
      b.steps
        ..clear()
        ..addAll(moved);
      final movedHg = {for (final e in b.hg.entries) e.key - _first: e.value};
      b.hg
        ..clear()
        ..addAll(movedHg);
      if (!b.done) {
        b.covered -= _first;
        if (b.hgCovered >= 0) b.hgCovered -= _first;
      }
    }
    _out(c3dHead(heads,
        rate: rate,
        first: _first,
        steps: _steps,
        perFrame: _perFrame,
        frames: _frames,
        source: source));
  }

  void _out(Uint8List bytes) {
    _written += bytes.length;
    sink(bytes);
  }
}

/// A C3D this package or tools/strelka_c3d.py wrote, read back (tools/strelka_c3d.py, read): its
/// channels' labels, the steps' rate, the first grid index, and every step's channels.
class C3dRead {
  C3dRead(this.labels, this.rate, this.first, this.rows);

  final List<String> labels;
  final double rate;
  final int first;
  final List<List<double>> rows;
}

C3dRead readC3d(Uint8List data) {
  final d = ByteData.sublistView(data);
  final groups = <int, String>{};
  final records = <(int, String, int)>[];
  var pos = 512 * (data[0] - 1) + 4;
  while (true) {
    final nlen = d.getInt8(pos), gid = d.getInt8(pos + 1);
    if (nlen == 0) break;
    final name = utf8.decode(data.sublist(pos + 2, pos + 2 + nlen.abs()));
    final at = pos + 2 + nlen.abs();
    final offset = d.getInt16(at, Endian.little);
    records.add((gid, name, at + 2));
    if (gid < 0) groups[-gid] = name;
    if (offset == 0) break;
    pos = at + offset;
  }
  final params = <String, Object>{};
  for (final (gid, name, at) in records) {
    if (gid < 0) continue;
    final kind = d.getInt8(at), ndims = data[at + 1];
    final dims = data.sublist(at + 2, at + 2 + ndims);
    final body = at + 2 + ndims;
    var size = kind.abs();
    for (final x in dims) {
      size *= x;
    }
    final raw = data.sublist(body, body + size);
    final Object value;
    if (kind == -1) {
      final width = dims.isEmpty ? raw.length : dims[0];
      value = dims.length > 1
          ? [
              for (var i = 0; i < raw.length; i += width)
                utf8.decode(raw.sublist(i, i + width)).trimRight()
            ]
          : utf8.decode(raw).trimRight();
    } else if (kind == 2) {
      value = [for (var i = 0; i < size; i += 2) ByteData.sublistView(raw).getInt16(i, Endian.little)];
    } else {
      value = [
        for (var i = 0; i < size; i += 4) ByteData.sublistView(raw).getFloat32(i, Endian.little)
      ];
    }
    params['${groups[gid]}:$name'] = value;
  }
  final labels = (params['ANALOG:LABELS']! as List<Object?>).cast<String>();
  final steps = int.parse(params['STRELKA:STEPS']! as String);
  final start = 512 * (d.getUint16(16, Endian.little) - 1);
  final n = labels.length;
  final rows = [
    for (var k = 0; k < steps; k++)
      [for (var c = 0; c < n; c++) d.getFloat32(start + 4 * (k * n + c), Endian.little)]
  ];
  return C3dRead(labels, (params['ANALOG:RATE']! as List<Object?>).first! as double,
      int.parse(params['STRELKA:FIRST_GRID_INDEX']! as String), rows);
}

/// [v] to [places] decimals as Python's format writes it: the double's exact value rounded,
/// a tie to the even digit — 0.0078125 is 0.007812 — and the sign kept, -0.000000 too.
String fixedAsPython(double v, int places) {
  final bits = ByteData(8)..setFloat64(0, v);
  final hi = bits.getUint32(0), lo = bits.getUint32(4);
  final negative = hi >> 31 == 1;
  final exp = (hi >> 20) & 0x7FF;
  var mant = BigInt.from(hi & 0xFFFFF) << 32 | BigInt.from(lo);
  var e = exp - 1075;
  if (exp == 0) {
    e = -1074;                             // subnormal
  } else {
    mant += BigInt.one << 52;
  }
  final scale = BigInt.from(10).pow(places);
  BigInt q;
  if (e >= 0) {
    q = (mant << e) * scale;
  } else {
    final num = mant * scale, den = BigInt.one << -e;
    q = num ~/ den;
    final r2 = (num - q * den) * BigInt.two;
    if (r2 > den || (r2 == den && q.isOdd)) q += BigInt.one;
  }
  final digits = q.toString().padLeft(places + 1, '0');
  final whole = digits.substring(0, digits.length - places);
  return '${negative ? '-' : ''}$whole${places > 0 ? '.${digits.substring(whole.length)}' : ''}';
}

/// The C3D's steps as CSV, as tools/strelka_c3d.py writes them (APP.md, The export): the grid
/// index, the time from the first step, every channel — g to six places, °/s to four — and a
/// unit's `lost`, 1 where its six values are all zero, a step the unit has not.
void c3dToCsv(Uint8List data, StringSink out) {
  final c = readC3d(data);
  // the units in the channels' order, each its label, its first channel and how many
  final units = <(String, int, int)>[];
  for (var i = 0; i < c.labels.length; i++) {
    final u = c.labels[i].substring(0, c.labels[i].lastIndexOf('_'));
    if (units.isNotEmpty && units.last.$1 == u) {
      units.last = (u, units.last.$2, units.last.$3 + 1);
    } else {
      units.add((u, i, 1));
    }
  }
  out.write('${['step', 't_s', ...c.labels, for (final u in units) '${u.$1}_lost'].join(',')}\n');
  for (var k = 0; k < c.rows.length; k++) {
    final row = c.rows[k];
    final values = [
      for (var i = 0; i < row.length; i++)
        fixedAsPython(row[i], c.labels[i].split('_').last.startsWith('G') ? 4 : 6)
    ];
    final lost = [
      for (final (_, at, n) in units)
        row.sublist(at, at + n).every((v) => v == 0.0) ? '1' : '0'
    ];
    out.write('${[
      '${c.first + k}',
      fixedAsPython(k / c.rate, 6),
      ...values,
      ...lost
    ].join(',')}\n');
  }
}
