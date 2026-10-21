/// SMP over TCP as Strelka speaks it (COMMANDS.md): the header of 8 B, big-endian, then a CBOR
/// map; the frames on a connection one after another, one request in flight; the lock first on
/// every connection.
///
///     final c = await SmpClient.connect('192.168.7.1', password: 'strelka1'); // the lock passed
///     print(await c.read(groupStrelka, Strelka.info));
///     await c.write(groupStrelka, Strelka.record, body: {'run': true});
///     final belka = await SmpClient.connect('192.168.7.1', port: smpPort + 3,
///         password: 'strelka1');                                   // the Belka numbered 3
///
/// The lock agrees a secret by X25519 on every connection and answers the nonce with an HMAC
/// over the nonce and the secret, so that a listener has nothing to try passwords on
/// (COMMANDS.md, The lock). An answer is its map; a Zephyr code comes back as an [SmpError]
/// with the code, a group's own with the group too. The standard codes (mgmt_defines.h): 0 all
/// well, 1 unknown, 2 no memory, 3 a bad value, 4 a timeout, 5 not found, 6 a bad state, 7 too
/// large, 8 unsupported, 9 corrupt, 10 busy, 11 access denied, 12 and 13 a version too old or
/// too new. Strelka's group 64: 1 the bridged target out of reach.
library;

import 'dart:async';
import 'dart:convert';
import 'dart:io';
import 'dart:typed_data';

import 'package:cbor/cbor.dart';
import 'package:cryptography/cryptography.dart';

const int smpHeaderBytes = 8;
const int smpVersion = 1;
const int smpFrameMax = 2048;
const int smpPort = 1337;

const int opRead = 0;
const int opReadRsp = 1;
const int opWrite = 2;
const int opWriteRsp = 3;

const int groupOs = 0;
const int groupImg = 1;
const int groupFs = 8;
const int groupStrelka = 64;

/// Strelka's group, 64: the commands by their id (COMMANDS.md).
abstract final class Strelka {
  static const int info = 0;
  static const int session = 1;
  static const int sessions = 2;
  static const int cal = 3;
  static const int blink = 4;
  static const int card = 5;
  static const int off = 6;
  static const int forget = 7;
  static const int key = 8;
  static const int join = 9;
  static const int record = 10;
  static const int live = 11;
  static const int search = 12;
  static const int take = 13;
  static const int set = 14;
  static const int drop = 15;
  static const int auth = 16;
  static const int password = 17;
  static const int table = 18;

  /// The group's own code: the bridged target out of reach.
  static const int unreachable = 1;

  static const Map<int, String> words = {unreachable: 'the bridged target out of reach'};
}

/// The standard codes (mgmt_defines.h).
abstract final class Rc {
  static const int ok = 0;
  static const int unknown = 1;
  static const int noMem = 2;
  static const int inval = 3;
  static const int timeout = 4;
  static const int noEnt = 5;
  static const int badState = 6;
  static const int tooLarge = 7;
  static const int notSup = 8;
  static const int corrupt = 9;
  static const int busy = 10;
  static const int access = 11;
  static const int tooOld = 12;
  static const int tooNew = 13;

  static const Map<int, String> words = {
    ok: 'all well',
    unknown: 'unknown',
    noMem: 'no memory',
    inval: 'a bad value',
    timeout: 'a timeout',
    noEnt: 'not found',
    badState: 'a bad state',
    tooLarge: 'too large',
    notSup: 'unsupported',
    corrupt: 'corrupt',
    busy: 'busy',
    access: 'access denied',
    tooOld: 'a version too old',
    tooNew: 'a version too new',
  };
}

/// An answer's code other than 0: Zephyr's own without a group, a group's with it.
class SmpError implements Exception {
  const SmpError(this.rc, [this.group]);

  final int rc;
  final int? group;

  @override
  String toString() {
    if (group == null) return 'SMP: ${Rc.words[rc] ?? 'code $rc'}';
    if (group == groupStrelka) return 'SMP: group 64: ${Strelka.words[rc] ?? 'code $rc'}';
    return 'SMP: group $group: code $rc';
  }
}

/// The 8 B before every body.
class SmpHeader {
  const SmpHeader({
    required this.op,
    this.version = smpVersion,
    this.flags = 0,
    required this.length,
    required this.group,
    required this.seq,
    required this.cmd,
  });

  factory SmpHeader.parse(Uint8List data) {
    if (data.length < smpHeaderBytes) throw const FormatException('a header of fewer than 8 B');
    final b = ByteData.sublistView(data, 0, smpHeaderBytes);
    final opVer = b.getUint8(0);
    return SmpHeader(
      op: opVer & 7,
      version: opVer >> 3,
      flags: b.getUint8(1),
      length: b.getUint16(2),
      group: b.getUint16(4),
      seq: b.getUint8(6),
      cmd: b.getUint8(7),
    );
  }

  final int op;
  final int version;
  final int flags;
  final int length;
  final int group;
  final int seq;
  final int cmd;

  Uint8List pack() {
    final out = ByteData(smpHeaderBytes)
      ..setUint8(0, (op & 7) | (version << 3))
      ..setUint8(1, flags)
      ..setUint16(2, length)
      ..setUint16(4, group)
      ..setUint8(6, seq)
      ..setUint8(7, cmd);
    return out.buffer.asUint8List();
  }
}

/// A frame: the header and the CBOR body; [body] a map, or bytes already encoded.
Uint8List smpFrame(int op, int group, int seq, int cmd,
    {Object? body, int version = smpVersion}) {
  final Uint8List bytes = switch (body) {
    null => Uint8List(0),
    Uint8List b => b,
    Map<String, Object?> m => encodeCbor(m),
    _ => throw ArgumentError('a body is a map or bytes'),
  };
  final head = SmpHeader(
    op: op,
    version: version,
    length: bytes.length,
    group: group,
    seq: seq,
    cmd: cmd,
  ).pack();
  return Uint8List.fromList([...head, ...bytes]);
}

/// A map into CBOR: text keys, ints, bools, text, bytes as [Uint8List], lists and maps in it.
Uint8List encodeCbor(Map<String, Object?> map) => Uint8List.fromList(cbor.encode(_toCbor(map)));

CborValue _toCbor(Object? v) => switch (v) {
      null => const CborNull(),
      CborValue c => c,
      bool b => CborBool(b),
      int i => CborSmallInt(i),
      BigInt i => CborInt(i),
      double d => CborFloat(d),
      String s => CborString(s),
      Uint8List b => CborBytes(b),
      List<Object?> l => CborList([for (final e in l) _toCbor(e)]),
      Map<Object?, Object?> m =>
        CborMap({for (final e in m.entries) _toCbor(e.key): _toCbor(e.value)}),
      _ => throw ArgumentError('cannot encode ${v.runtimeType}'),
    };

/// A CBOR body into a map with text keys: bytes come as [Uint8List], ints as [int], the rest as
/// Dart has it; a [FormatException] when the body is not a map.
Map<String, Object?> decodeCborMap(Uint8List bytes) {
  final v = _fromCbor(cbor.decode(bytes));
  if (v is! Map<Object?, Object?>) throw const FormatException('a body that is not a map');
  return {for (final e in v.entries) e.key.toString(): e.value};
}

Object? _fromCbor(CborValue v) => switch (v) {
      CborNull() => null,
      CborBool b => b.value,
      CborInt i => i.toInt(),
      CborFloat f => f.value,
      CborString s => s.toString(),
      CborBytes b => Uint8List.fromList(b.bytes),
      CborList l => [for (final e in l) _fromCbor(e)],
      CborMap m => {for (final e in m.entries) _fromCbor(e.key): _fromCbor(e.value)},
      _ => v.toObject(),
    };

/// The map of an answer, or the [SmpError] it carries.
Map<String, Object?> smpAnswer(Map<String, Object?> body) {
  final rc = body['rc'];
  if (rc is int && rc != 0) throw SmpError(rc);
  final err = body['err'];
  if (err is Map<Object?, Object?>) {
    final erc = err['rc'];
    if (erc is int && erc != 0) throw SmpError(erc, err['group'] as int?);
  }
  return body;
}

/// A TCP stream cut into frames, as the firmware's common/smp.c does it.
class SmpStream {
  Uint8List _buf = Uint8List(0);

  /// The whole frames arrived so far, header included; a header saying more than [smpFrameMax]
  /// is a [FormatException], and the connection is to be closed.
  List<Uint8List> feed(Uint8List data) {
    _buf = _buf.isEmpty ? data : Uint8List.fromList([..._buf, ...data]);
    final frames = <Uint8List>[];
    var at = 0;
    while (_buf.length - at >= smpHeaderBytes) {
      final length = ByteData.sublistView(_buf, at, at + smpHeaderBytes).getUint16(2);
      if (length > smpFrameMax) throw FormatException('a frame of $length B');
      if (_buf.length - at < smpHeaderBytes + length) break;
      frames.add(Uint8List.sublistView(_buf, at, at + smpHeaderBytes + length));
      at += smpHeaderBytes + length;
    }
    if (at > 0) _buf = Uint8List.sublistView(_buf, at);
    return frames;
  }
}

/// A fresh X25519 pair for one connection: its public key, and the SHA-256 of the secret agreed
/// with the peer's public key — the lock's secret.
class Agreed {
  const Agreed(this.pub, this.secret);

  final Uint8List pub;
  final Uint8List secret;
}

Future<Agreed> agree(List<int> peerPub) async {
  final x = X25519();
  final pair = await x.newKeyPair();
  final pub = await pair.extractPublicKey();
  final shared = await x.sharedSecretKey(
    keyPair: pair,
    remotePublicKey: SimplePublicKey(peerPub, type: KeyPairType.x25519),
  );
  final secret = await Sha256().hash(await shared.extractBytes());
  return Agreed(Uint8List.fromList(pub.bytes), Uint8List.fromList(secret.bytes));
}

/// What the lock wants: HMAC-SHA256 with the password's SHA-256 as the key over the nonce and
/// the secret agreed for the connection.
Future<Uint8List> macFor(List<int> password, List<int> nonce, List<int> secret) async {
  final key = await Sha256().hash(password);
  final mac = await Hmac.sha256().calculateMac([...nonce, ...secret], secretKey: SecretKey(key.bytes));
  return Uint8List.fromList(mac.bytes);
}

class _Pending {
  _Pending(this.op, this.group, this.seq, this.cmd);

  final int op;
  final int group;
  final int seq;
  final int cmd;
  final Completer<Map<String, Object?>> completer = Completer();
}

/// One connection: the lock passed in [connect] when a password is given; the requests one
/// after another, each answered before the next goes.
class SmpClient {
  SmpClient._(this._socket) {
    _socket.listen(
      _onData,
      onError: _fail,
      onDone: () => _fail(const SocketException('the connection closed')),
      cancelOnError: true,
    );
  }

  final Socket _socket;
  final SmpStream _stream = SmpStream();
  int _seq = 0;
  _Pending? _pending;
  Future<void> _chain = Future<void>.value();
  Object? _dead;

  /// The address this connection comes from, where Povodok sends the live view.
  InternetAddress get localAddress => _socket.address;

  static Future<SmpClient> connect(
    String host, {
    int port = smpPort,
    String? password,
    Duration timeout = const Duration(seconds: 5),
  }) async {
    final socket = await Socket.connect(host, port, timeout: timeout);
    socket.setOption(SocketOption.tcpNoDelay, true);
    final client = SmpClient._(socket);
    if (password != null) {
      try {
        await client.unlock(password);
      } catch (_) {
        client.close();
        rethrow;
      }
    }
    return client;
  }

  void close() => _socket.destroy();

  /// One request, its answer's map back, an [SmpError] thrown for a code.
  Future<Map<String, Object?>> request(
    int op,
    int group,
    int cmd, {
    Map<String, Object?>? body,
    int version = smpVersion,
    Duration timeout = const Duration(seconds: 10),
  }) {
    final done = _chain.then((_) => _send(op, group, cmd, body, version, timeout));
    _chain = done.then<void>((_) {}, onError: (Object _) {});
    return done;
  }

  Future<Map<String, Object?>> read(int group, int cmd, {Map<String, Object?>? body}) =>
      request(opRead, group, cmd, body: body);

  Future<Map<String, Object?>> write(int group, int cmd, {Map<String, Object?>? body}) =>
      request(opWrite, group, cmd, body: body);

  /// 64/16: the nonce and Povodok's public key read, the MAC written with our own.
  Future<void> unlock(String password) async {
    final challenge = await read(groupStrelka, Strelka.auth);
    final agreed = await agree(challenge['pub'] as Uint8List);
    final mac = await macFor(utf8.encode(password), challenge['nonce'] as Uint8List, agreed.secret);
    await write(groupStrelka, Strelka.auth, body: {'pub': agreed.pub, 'mac': mac});
  }

  /// 64/17: the old password proved by a fresh nonce's MAC, the new one's SHA-256 set.
  Future<void> changePassword(String old, String fresh) async {
    final challenge = await read(groupStrelka, Strelka.auth);
    final agreed = await agree(challenge['pub'] as Uint8List);
    final mac = await macFor(utf8.encode(old), challenge['nonce'] as Uint8List, agreed.secret);
    final hash = await Sha256().hash(utf8.encode(fresh));
    await write(groupStrelka, Strelka.password, body: {
      'pub': agreed.pub,
      'mac': mac,
      'new': Uint8List.fromList(hash.bytes),
    });
  }

  /// 8/0: a file whole, from byte 0 on, as Zephyr's file group gives it.
  Future<Uint8List> download(String name) async {
    final out = BytesBuilder(copy: false);
    var off = 0;
    int? length;
    while (length == null || off < length) {
      final rsp = await read(groupFs, 0, body: {'name': name, 'off': off});
      final data = rsp['data'] as Uint8List;
      if (off == 0) length = rsp['len'] as int;
      out.add(data);
      off = (rsp['off'] as int) + data.length;
      if (data.isEmpty) break;
    }
    return out.takeBytes();
  }

  Future<Map<String, Object?>> _send(
    int op,
    int group,
    int cmd,
    Map<String, Object?>? body,
    int version,
    Duration timeout,
  ) async {
    final dead = _dead;
    if (dead != null) throw dead;
    final seq = _seq;
    _seq = (_seq + 1) & 0xFF;
    final pending = _Pending(op + 1, group, seq, cmd);
    _pending = pending;
    try {
      _socket.add(smpFrame(op, group, seq, cmd, body: body, version: version));
      final answer = await pending.completer.future.timeout(timeout);
      return smpAnswer(answer);
    } finally {
      _pending = null;
    }
  }

  void _onData(Uint8List data) {
    final List<Uint8List> frames;
    try {
      frames = _stream.feed(data);
    } on FormatException catch (e) {
      _fail(e);
      _socket.destroy();
      return;
    }
    for (final f in frames) {
      final h = SmpHeader.parse(f);
      final p = _pending;
      if (p == null ||
          p.completer.isCompleted ||
          h.seq != p.seq ||
          h.group != p.group ||
          h.cmd != p.cmd ||
          h.op != p.op) {
        continue;
      }
      try {
        p.completer.complete(h.length == 0
            ? <String, Object?>{}
            : decodeCborMap(Uint8List.sublistView(f, smpHeaderBytes, smpHeaderBytes + h.length)));
      } catch (e, s) {
        p.completer.completeError(e, s);
      }
    }
  }

  void _fail(Object error, [StackTrace? stack]) {
    _dead ??= error;
    final p = _pending;
    if (p != null && !p.completer.isCompleted) {
      p.completer.completeError(error, stack ?? StackTrace.current);
    }
  }
}
