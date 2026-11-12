/// The download as the application runs it (APP.md, The download; COMMANDS.md, 64/21): the
/// sessions read from every Belka by 64/2 and gathered by their number, then one session asked of
/// Povodok once, its frames read from the download's port as Povodok passes them on — a Belka's
/// number u8, the frame's length u16, the frame as she sent it; a length 0 her done without her
/// file whole, the number 0 with a length 0 the download over — and written into one C3D as they
/// come (c3d.dart). No file but the C3D: the records stay on the Belkas' cards.
library;

import 'dart:async';
import 'dart:io';
import 'dart:typed_data';

import 'c3d.dart';
import 'record.dart';
import 'smp.dart';

/// One record off the download's port.
class FetchRecord {
  const FetchRecord(this.belka, this.frame);

  final int belka;
  final Uint8List frame;

  /// The download over.
  bool get over => belka == 0;

  /// The Belka done without her file whole: with another Povodok, gone, or her file missing.
  bool get gone => belka != 0 && frame.isEmpty;
}

/// The records as they come on [socket], cut from its byte stream.
Stream<FetchRecord> fetchRecords(Stream<Uint8List> socket) async* {
  final pending = BytesBuilder(copy: false);
  await for (final chunk in socket) {
    pending.add(chunk);
    var all = pending.takeBytes();
    var at = 0;
    while (all.length - at >= 3) {
      final len = all[at + 1] | all[at + 2] << 8;
      if (all.length - at < 3 + len) break;
      final r = FetchRecord(all[at], Uint8List.fromList(all.sublist(at + 3, at + 3 + len)));
      at += 3 + len;
      yield r;
      if (r.over) return;
    }
    if (at < all.length) pending.add(Uint8List.sublistView(all, at));
    all = Uint8List(0);
  }
}

/// A frame of 8/0's answer: the offset, the data, the file's length with offset 0.
class FilePiece {
  const FilePiece(this.off, this.data, this.length);

  final int off;
  final Uint8List data;
  final int? length;

  /// Throws [SmpError] for an answer with a code, [FormatException] for one malformed.
  static FilePiece parse(Uint8List frame) {
    final h = SmpHeader.parse(frame);
    if (h.group != groupFs || h.cmd != 0 || h.op != opReadRsp) {
      throw const FormatException('not a file download\'s answer');
    }
    final body = smpAnswer(decodeCborMap(
        Uint8List.sublistView(frame, smpHeaderBytes, smpHeaderBytes + h.length)));
    return FilePiece(body['off'] as int, body['data'] as Uint8List, body['len'] as int?);
  }
}

/// A session as the Belkas hold it: its number and each Belka's file of it, by 64/2.
class SessionOnSet {
  SessionOnSet(this.s);

  final int s;
  final Map<int, RecordOnBelka> records = {};

  int get bytes => records.values.fold(0, (a, r) => a + r.bytes);
  List<int> get belkas => records.keys.toList()..sort();
}

/// The units a session needs, by the session's units in a head, not found on the set: to be switched
/// on before its download, or left out at the user's word (APP.md, The download).
List<int> unitsMissing(SessionOnSet session, RecordHead? head) =>
    [for (final n in head?.units.keys ?? const <int>[]) if (!session.records.containsKey(n)) n]
      ..sort();

/// Every session on the Belkas numbered [belkas], read through their ports and gathered by the
/// session's number, the newest first; a Belka out of reach left out.
Future<List<SessionOnSet>> sessionsOnSet(String host, String password, List<int> belkas,
    {int port = smpPort}) async {
  final bySession = <int, SessionOnSet>{};
  for (final n in belkas) {
    SmpClient? c;
    try {
      c = await SmpClient.connect(host, port: port + n, password: password);
      for (final e in await c.sessions()) {
        final s = e['s'] as int;
        (bySession[s] ??= SessionOnSet(s)).records[n] =
            RecordOnBelka(n, e['steps'] as int, e['p0'] as int, e['bytes'] as int);
      }
    } on SmpError {
      // out of reach: left out of the list
    } on SocketException {
      // the same
    } finally {
      c?.close();
    }
  }
  return bySession.values.toList()..sort((a, b) => b.s.compareTo(a.s));
}

/// How a download stands, for its screen.
class DownloadProgress {
  DownloadProgress(this.session) : got = {for (final n in session.belkas) n: 0};

  final SessionOnSet session;
  final Map<int, int> got;
  final Set<int> gone = {};
  bool over = false;

  int get bytes => got.values.fold(0, (a, b) => a + b);
}

/// One session's download into one C3D written to [sink], kept across a break: [run] asks
/// Povodok by 64/21 from each Belka's byte reached, so that a download broken off — the network
/// lost, [cancel] — goes on into the same C3D at the next [run] while the application stands.
class SessionDownload {
  SessionDownload(this.session, void Function(Uint8List bytes) sink, {this.onPiece})
      : progress = DownloadProgress(session),
        _composer = C3dComposer(session.records.values.toList(), sink);

  final SessionOnSet session;
  final DownloadProgress progress;
  final C3dComposer _composer;

  /// Each piece of a Belka's file as it comes, at its offset: the card's file kept as it is, STRK,
  /// beside the C3D (APP.md, The download).
  final void Function(int belka, int off, Uint8List data)? onPiece;

  /// The set's table and the first head, once they have come: the file's name is made of them.
  String? get table => _composer.table;
  RecordHead? get head => _composer.head;
  Socket? _stream;
  bool _cancelled = false;

  /// The download broken off; [run] again goes on.
  void cancel() {
    _cancelled = true;
    _stream?.destroy();
  }

  /// Until the download is over or broken off: true when it is over, the C3D whole.
  Future<bool> run(String host, String password,
      {int port = smpPort, void Function(DownloadProgress p)? onProgress}) async {
    if (progress.over) return true;
    _cancelled = false;
    final stream = _stream = await Socket.connect(host, port + fetchPortOffset);
    final povodok = await SmpClient.connect(host, port: port, password: password);
    try {
      final left = [for (final n in session.belkas) if (!progress.gone.contains(n)) n];
      final resumed = left.any((n) => progress.got[n]! > 0);
      await povodok.fetch(session.s,
          belkas: left, from: resumed ? [for (final n in left) progress.got[n]!] : null);
      await for (final r in fetchRecords(stream)) {
        if (r.over) {
          _composer.finish();
          progress.over = true;
          break;
        }
        if (r.gone) {
          _composer.gone(r.belka);
          progress.gone.add(r.belka);
        } else {
          final piece = FilePiece.parse(r.frame);
          onPiece?.call(r.belka, piece.off, piece.data);
          _composer.add(r.belka, piece.off, piece.data);
          progress.got[r.belka] = piece.off + piece.data.length;
        }
        onProgress?.call(progress);
      }
    } on SocketException {
      if (!_cancelled) rethrow;
    } finally {
      povodok.close();
      stream.destroy();
      _stream = null;
    }
    onProgress?.call(progress);
    return progress.over;
  }
}

/// Session [session] downloaded through Povodok at [host] into one C3D written to [sink], at one
/// go (SessionDownload).
Future<DownloadProgress> downloadSession(String host, String password, SessionOnSet session,
    void Function(Uint8List bytes) sink,
    {int port = smpPort, void Function(DownloadProgress p)? onProgress}) async {
  final d = SessionDownload(session, sink);
  await d.run(host, password, port: port, onProgress: onProgress);
  return d.progress;
}

/// A session's head, for the list: one Belka's file read by 8/0 from its start until the head has
/// its place and the session's units from the set's table, the blocks after it (RECORD.md).
Future<RecordHead?> sessionHead(String host, String password, int belka, int s,
    {int port = smpPort}) async {
  final c = await SmpClient.connect(host, port: port + belka, password: password);
  try {
    final name = '/SD:/S${'$s'.padLeft(5, '0')}B${'$belka'.padLeft(2, '0')}.STRK';
    final reader = RecordReader((_) {});
    var off = 0;
    while (!reader.settled && off < _headMost) {
      final rsp = await c.read(groupFs, 0, body: {'name': name, 'off': off});
      final data = rsp['data'] as Uint8List;
      if (data.isEmpty) break;
      reader.add(data);
      off += data.length;
      if (off == data.length && reader.head == null && off >= blockBytes) break; // not a record
    }
    return reader.head;
  } finally {
    c.close();
  }
}

const int _headMost = 64 * blockBytes; // the head and a table of 63 blocks at most read for it

/// The records' own reading, for a test or a file already on the phone: each Belka's whole file
/// through the composer, as a download would bring it.
void composeFiles(Map<int, Uint8List> files, List<RecordOnBelka> records,
    void Function(Uint8List bytes) sink) {
  final c = C3dComposer(records, sink);
  for (final e in files.entries) {
    for (var at = 0; at < e.value.length; at += 1024) {
      final end = at + 1024 < e.value.length ? at + 1024 : e.value.length;
      c.add(e.key, at, Uint8List.sublistView(e.value, at, end));
    }
  }
  c.finish();
}

