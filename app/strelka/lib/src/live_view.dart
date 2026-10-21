/// The live view's message, a message a period over UDP from the first Povodok to the phone
/// (PHONE.md, The live view): read and built here byte for byte as the firmware has it
/// (firmware/common/live_msg.c), with the quaternion's packing of PRINCIPLE.md — the smallest
/// three, 2 + 3 × 10 bits. Povodok sends it to the address the application's connection on port
/// 1337 came from, to UDP port 1337 there.
///
///     final r = await LiveReceiver.bind();                 // UDP 1337 on every address
///     await for (final msg in r.messages) {
///       for (final b in msg.belkas) {
///         print('${b.n} ${b.quats}');                      // or b.raws, as the code says
///       }
///     }
library;

import 'dart:async';
import 'dart:io';
import 'dart:math' as math;
import 'dart:typed_data';

const int liveKind = 1;
const int liveVersion = 1;
const int liveMaxBytes = 1400;
const int livePort = 1337;

const int _headBytes = 21;
const int _entryBytes = 5;
const double _rootHalf = 0.70710678;
const double _scale = 511.0;

/// The live view's code, the high nibble of S0: what a step is and how many come a period.
enum LiveCode {
  off(0, 0, 0),
  quat50(1, 1, 4),
  quat100(2, 2, 4),
  raw50(4, 1, 12),
  raw100(5, 2, 12);

  const LiveCode(this.code, this.steps, this.stepBytes);

  final int code;
  final int steps;
  final int stepBytes;

  bool get quaternions => stepBytes == 4;

  static LiveCode? of(int code) {
    for (final c in values) {
      if (c.code == code) return c;
    }
    return null;
  }
}

/// A unit quaternion, w first.
class Quat {
  const Quat(this.w, this.x, this.y, this.z);

  final double w;
  final double x;
  final double y;
  final double z;

  List<double> get list => [w, x, y, z];

  @override
  String toString() =>
      '(${w.toStringAsFixed(4)}, ${x.toStringAsFixed(4)}, ${y.toStringAsFixed(4)}, '
      '${z.toStringAsFixed(4)})';
}

/// A raw step: the accelerometer's three readings and the gyro's three, as the record has them.
class RawStep {
  const RawStep(this.a, this.g);

  final List<int> a;
  final List<int> g;

  @override
  String toString() => 'a$a g$g';
}

/// One Belka's entry: its number, its S0 (the code and the item), who heard it (0 the first
/// Povodok itself, else the Povodok's number), its state's two bytes, and its steps — in
/// [quats] or in [raws] as the code says, the other list empty.
class LiveBelka {
  const LiveBelka({
    required this.n,
    required this.s0,
    required this.by,
    required this.state,
    this.quats = const [],
    this.raws = const [],
  });

  final int n;
  final int s0;
  final int by;
  final int state;
  final List<Quat> quats;
  final List<RawStep> raws;

  LiveCode get code => LiveCode.of(s0 >> 4)!;
  int get item => s0 & 0xF;
}

/// One message: the head's fields and the Belkas in it.
class LiveMessage {
  const LiveMessage({
    required this.session,
    required this.period,
    required this.state,
    required this.s0,
    required this.povodoks,
    required this.povodokStates,
    required this.belkas,
  });

  final int session;
  final int period;
  final int state;
  final int s0;
  final int povodoks;
  final List<int> povodokStates;
  final List<LiveBelka> belkas;

  LiveCode get code => LiveCode.of(s0 >> 4)!;
  int get item => s0 & 0xF;
}

/// A unit quaternion into 4 B: the largest component, kept positive, named by 2 bits and left
/// out, the other three as ±511 for ±1/√2.
int packQuat(Quat q) {
  final c = q.list;
  var big = 0;
  for (var i = 1; i < 4; i++) {
    if (c[i].abs() > c[big].abs()) big = i;
  }
  final sign = c[big] < 0 ? -1.0 : 1.0;
  var v = big << 30;
  var shift = 20;
  for (var i = 0; i < 4; i++) {
    if (i == big) continue;
    final n = (c[i] * sign / _rootHalf * _scale).round().clamp(-511, 511);
    v |= (n & 0x3FF) << shift;
    shift -= 10;
  }
  return v;
}

/// The 4 B back into a unit quaternion, the left-out component from the other three.
Quat unpackQuat(int v) {
  final big = v >> 30;
  final q = List<double>.filled(4, 0.0);
  var total = 0.0;
  var shift = 20;
  for (var i = 0; i < 4; i++) {
    if (i == big) continue;
    var n = (v >> shift) & 0x3FF;
    if (n & 0x200 != 0) n -= 0x400;
    q[i] = n / _scale * _rootHalf;
    total += q[i] * q[i];
    shift -= 10;
  }
  q[big] = total < 1.0 ? math.sqrt(1.0 - total) : 0.0;
  return Quat(q[0], q[1], q[2], q[3]);
}

/// A message read; a [FormatException] when it is not one.
LiveMessage parseLive(Uint8List data) {
  if (data.length < _headBytes) throw const FormatException('shorter than a head');
  final b = ByteData.sublistView(data);
  if (b.getUint8(0) != liveKind || b.getUint8(1) != liveVersion) {
    throw const FormatException('not a live view message');
  }
  final count = b.getUint8(20);
  final belkas = <LiveBelka>[];
  var at = _headBytes;
  for (var i = 0; i < count; i++) {
    if (data.length - at < _entryBytes) throw const FormatException('an entry cut off');
    final n = b.getUint8(at);
    final s0 = b.getUint8(at + 1);
    final by = b.getUint8(at + 2);
    final state = b.getUint16(at + 3, Endian.little);
    at += _entryBytes;
    final code = LiveCode.of(s0 >> 4);
    if (code == null || n < 1 || n > 63) throw const FormatException('a malformed entry');
    if (data.length - at < code.steps * code.stepBytes) {
      throw const FormatException('steps cut off');
    }
    final quats = <Quat>[];
    final raws = <RawStep>[];
    for (var k = 0; k < code.steps; k++) {
      if (code.quaternions) {
        quats.add(unpackQuat(b.getUint32(at, Endian.little)));
      } else {
        raws.add(RawStep(
          [for (var j = 0; j < 3; j++) b.getInt16(at + 2 * j, Endian.little)],
          [for (var j = 3; j < 6; j++) b.getInt16(at + 2 * j, Endian.little)],
        ));
      }
      at += code.stepBytes;
    }
    belkas.add(LiveBelka(n: n, s0: s0, by: by, state: state, quats: quats, raws: raws));
  }
  if (at != data.length) throw FormatException('${data.length - at} bytes left');
  return LiveMessage(
    session: b.getUint16(2, Endian.little),
    period: b.getUint32(4, Endian.little),
    state: b.getUint16(8, Endian.little),
    s0: b.getUint8(10),
    povodoks: b.getUint8(11),
    povodokStates: [for (var j = 0; j < 4; j++) b.getUint16(12 + 2 * j, Endian.little)],
    belkas: belkas,
  );
}

/// A message built, as the simulator and the tests need it; an [ArgumentError] when the steps
/// do not match an entry's code or the whole does not fit in [liveMaxBytes].
Uint8List buildLive(LiveMessage m) {
  final out = BytesBuilder(copy: false);
  final head = ByteData(_headBytes)
    ..setUint8(0, liveKind)
    ..setUint8(1, liveVersion)
    ..setUint16(2, m.session, Endian.little)
    ..setUint32(4, m.period, Endian.little)
    ..setUint16(8, m.state, Endian.little)
    ..setUint8(10, m.s0)
    ..setUint8(11, m.povodoks)
    ..setUint8(20, m.belkas.length);
  for (var j = 0; j < 4; j++) {
    head.setUint16(12 + 2 * j, m.povodokStates[j], Endian.little);
  }
  out.add(head.buffer.asUint8List());
  for (final e in m.belkas) {
    final code = e.code;
    final steps = code.quaternions ? e.quats.length : e.raws.length;
    if (steps != code.steps) {
      throw ArgumentError('Belka ${e.n}: $steps steps for code ${code.code}');
    }
    final entry = ByteData(_entryBytes + code.steps * code.stepBytes)
      ..setUint8(0, e.n)
      ..setUint8(1, e.s0)
      ..setUint8(2, e.by)
      ..setUint16(3, e.state, Endian.little);
    var at = _entryBytes;
    for (var k = 0; k < code.steps; k++) {
      if (code.quaternions) {
        entry.setUint32(at, packQuat(e.quats[k]), Endian.little);
      } else {
        for (var j = 0; j < 3; j++) {
          entry.setInt16(at + 2 * j, e.raws[k].a[j], Endian.little);
          entry.setInt16(at + 6 + 2 * j, e.raws[k].g[j], Endian.little);
        }
      }
      at += code.stepBytes;
    }
    out.add(entry.buffer.asUint8List());
  }
  if (out.length > liveMaxBytes) throw ArgumentError('a message of ${out.length} B');
  return out.takeBytes();
}

/// The messages as they come, on UDP port 1337 by default, on every address of the device.
class LiveReceiver {
  LiveReceiver._(this._socket);

  final RawDatagramSocket _socket;

  static Future<LiveReceiver> bind({int port = livePort, InternetAddress? address}) async {
    final socket =
        await RawDatagramSocket.bind(address ?? InternetAddress.anyIPv4, port, reuseAddress: true);
    return LiveReceiver._(socket);
  }

  /// The port bound, for the one connection's answer the receiver listens for.
  int get port => _socket.port;

  /// The messages parsed, one stream for the receiver's life; what is not a message is dropped.
  late final Stream<LiveMessage> messages = _messages();

  Stream<LiveMessage> _messages() async* {
    await for (final event in _socket) {
      if (event != RawSocketEvent.read) continue;
      final datagram = _socket.receive();
      if (datagram == null) continue;
      try {
        yield parseLive(datagram.data);
      } on FormatException {
        continue;
      }
    }
  }

  void close() => _socket.close();
}
