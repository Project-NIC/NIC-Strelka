/// The data manager (APP.md, What it must do, 8 and 11; The menus, 4; the sketch
/// app/mockups/en/data-a-list.png): what is on the phone — the C3Ds downloaded and their CSVs, a
/// C3D turned into CSV by a touch, a lost step zeros and 1 in its unit's `lost`; the cards' own
/// files beside them; a live view's working file left by a stop not reached made into its C3D by a
/// touch (APP.md, The live view saved) — and the sessions on the units, gathered by their number,
/// each with what it needs: on the phone already, a unit to switch on first, too large for the
/// radio, or ready; a touch downloads it.
library;

import 'dart:async';
import 'dart:io';

import 'package:material_ui/material_ui.dart';
import 'package:path_provider/path_provider.dart';
import 'package:strelka/strelka.dart';

import 'l10n/app_localizations.dart';
import 'main.dart';

/// A session larger than this goes by the dock or the cable rather than the radio.
const int largeBytes = 1000000000;

/// The longest note in a name (APP.md, The names).
const int noteMost = 64;

/// A record's name without its end (APP.md, The names): the date and the time it began, the
/// session, and the user's note, what Windows forbids in a name (\ / : * ? " < > |) and control
/// characters made '-', at most 64 characters — 2026-11-10_1705_S44_Praha prase. Its ends: `_card`
/// and `_live` before `.c3d`, `_card_S00044B03.STRK` for a card's own file.
String recordBase(DateTime t, int s, String note) {
  String two(int v) => '$v'.padLeft(2, '0');
  final clean = String.fromCharCodes(note.runes.map((r) => r < 32 || '\\/:*?"<>|'.contains(
          String.fromCharCode(r)) ? 0x2D : r).take(noteMost)).trim();
  return '${t.year}-${two(t.month)}-${two(t.day)}_${two(t.hour)}${two(t.minute)}_S$s'
      '${clean.isEmpty ? '' : '_$clean'}';
}

/// Whether session [s]'s C3D from the cards is among [names].
bool cardOnPhone(Set<String> names, int s) =>
    names.any((n) => RegExp('_S$s(_.*)?_card\\.c3d\$').hasMatch(n));

/// A live view saved: the session, the date and the time it began, S00007-20261110-170512.live.
String liveName(int s, DateTime t) {
  String two(int v) => '$v'.padLeft(2, '0');
  return 'S${'$s'.padLeft(5, '0')}-${t.year}${two(t.month)}${two(t.day)}-'
      '${two(t.hour)}${two(t.minute)}${two(t.second)}.live';
}

/// A live view's working file made into its C3D beside it (APP.md, The live view saved), named
/// by the time the saving began, the session and the note; the working file then gone.
Future<File> finishLive(File live) async {
  final saved = readSavedLive(await live.readAsBytes());
  final m = RegExp(r'S(\d+)-(\d{4})(\d{2})(\d{2})-(\d{2})(\d{2})(\d{2})\.live$')
      .firstMatch(live.path);
  int at(int i) => int.parse(m![i]!);
  final t = m == null
      ? await live.lastModified()
      : DateTime(at(2), at(3), at(4), at(5), at(6), at(7));
  final s = saved.session['s'] as int? ?? (m == null ? 0 : at(1));
  final out = File('${live.parent.path}/${recordBase(t, s, savedNote(saved))}_live.c3d');
  await out.writeAsBytes(liveToC3d(saved));
  await live.delete();
  return out;
}

/// Where the phone keeps what it downloads and saves.
Future<Directory> phoneDir() async =>
    Directory('${(await getApplicationDocumentsDirectory()).path}/Strelka');

/// When, the rate, how long, which units, how much (APP.md, The download).
String describeSession(SessionOnSet s, RecordHead? head) {
  final parts = <String>[];
  if (head != null && head.timeUs > 0) {
    final t = DateTime.fromMicrosecondsSinceEpoch(head.timeUs, isUtc: true).toLocal();
    String two(int v) => '$v'.padLeft(2, '0');
    parts.add('${t.year}-${two(t.month)}-${two(t.day)} ${two(t.hour)}:${two(t.minute)}');
  }
  if (head != null) {
    parts.add('${head.rateHz} Hz');
    final steps = s.records.values.map((r) => r.steps).fold(0, (a, b) => a > b ? a : b);
    final secs = head.rateHz > 0 ? steps ~/ head.rateHz : 0;
    parts.add('${secs ~/ 60}:${'${secs % 60}'.padLeft(2, '0')}');
  }
  parts.add([
    for (final n in s.belkas) '${kindLetter(head?.units[n] ?? kindBelka)}$n',
  ].join(' '));
  final mb = s.bytes / 1e6;
  parts.add(mb >= 1000 ? '${(mb / 1000).toStringAsFixed(1)} GB' : '${mb.toStringAsFixed(1)} MB');
  return parts.join(' · ');
}

enum SessionState { onPhone, switchOn, large, ready }

SessionState sessionState(SessionOnSet s, RecordHead? head, Set<String> onPhone) {
  if (cardOnPhone(onPhone, s.s)) return SessionState.onPhone;
  if (unitsMissing(s, head).isNotEmpty) return SessionState.switchOn;
  if (s.bytes > largeBytes) return SessionState.large;
  return SessionState.ready;
}

class DataScreen extends StatefulWidget {
  const DataScreen({super.key, required this.host, required this.password, required this.client});

  final String host, password;
  final SmpClient client;

  @override
  State<DataScreen> createState() => _DataScreenState();
}

class _DataScreenState extends State<DataScreen> {
  List<File> _files = const [];
  List<SessionOnSet>? _sessions;
  final Map<int, RecordHead?> _heads = {};
  String? _trouble;

  @override
  void initState() {
    super.initState();
    unawaited(_loadFiles());
    unawaited(_loadSessions());
  }

  Future<void> _loadFiles() async {
    try {
      final dir = await phoneDir();
      final files = !await dir.exists()
          ? <File>[]
          : (await dir.list().toList())
              .whereType<File>()
              .where((f) => RegExp(r'\.(c3d|csv|live|STRK)$').hasMatch(f.path))
              .toList();
      files.sort((a, b) => b.path.compareTo(a.path));
      if (mounted) setState(() => _files = files);
    } on Object {
      // no storage: nothing on the phone to show
    }
  }

  Future<void> _loadSessions() async {
    try {
      final set = await widget.client.read(groupStrelka, Strelka.set);
      final belkas = [
        for (final b in set['belkas'] as List<Object?>? ?? const [])
          (b as Map<Object?, Object?>)['n'] as int
      ]..sort();
      final sessions = await sessionsOnSet(widget.host, widget.password, belkas);
      if (!mounted) return;
      setState(() => _sessions = sessions);
      for (final s in sessions) {
        RecordHead? head;
        try {
          head = await sessionHead(widget.host, widget.password, s.belkas.first, s.s);
        } on Exception {
          head = null;
        }
        if (!mounted) return;
        setState(() => _heads[s.s] = head);
      }
    } on Exception catch (e) {
      if (mounted) setState(() => _trouble = '$e');
    }
  }

  String _name(File f) => f.path.split('/').last;

  Future<void> _toCsv(File c3d) async {
    try {
      final out = StringBuffer();
      c3dToCsv(await c3d.readAsBytes(), out);
      await File(c3d.path.replaceFirst(RegExp(r'\.c3d$'), '.csv')).writeAsString(out.toString());
      await _loadFiles();
    } on Object catch (e) {
      if (mounted) setState(() => _trouble = '$e');
    }
  }

  /// A working file the live view left, the application stopped before its C3D, made into it.
  Future<void> _finish(File live) async {
    try {
      await finishLive(live);
      await _loadFiles();
    } on Object catch (e) {
      if (mounted) setState(() => _trouble = '$e');
    }
  }

  Future<void> _download(SessionOnSet s) async {
    final t = AppLocalizations.of(context);
    final ok = await showDialog<bool>(
      context: context,
      builder: (c) => AlertDialog(
        title: Text(t.measSession(s.s)),
        content: Text(t.dataAsk(s.belkas.length)),
        actions: [
          TextButton(onPressed: () => Navigator.pop(c, false), child: Text(t.dataCancel)),
          FilledButton(onPressed: () => Navigator.pop(c, true), child: Text(t.dataDownload)),
        ],
      ),
    );
    if (ok != true || !mounted) return;
    await Navigator.of(context).push(MaterialPageRoute<void>(
        builder: (_) => DownloadScreen(host: widget.host, password: widget.password, session: s)));
    await _loadFiles();
  }

  @override
  Widget build(BuildContext context) {
    final t = AppLocalizations.of(context);
    final names = {for (final f in _files) _name(f)};
    final sessions = _sessions;
    return Scaffold(
      appBar: AppBar(title: Text(t.menuData)),
      body: ListView(children: [
        ListTile(title: Text(t.dataPhone, style: const TextStyle(color: Look.white))),
        if (_files.isEmpty) ListTile(dense: true, title: Text(t.dataNoFiles)),
        for (final f in _files)
          ListTile(
            dense: true,
            title: Text(_name(f)),
            subtitle: Text('${(f.lengthSync() / 1e6).toStringAsFixed(1)} MB'),
            trailing: _name(f).endsWith('.live')
                ? FilledButton(onPressed: () => _finish(f), child: Text(t.dataToC3d))
                : _name(f).endsWith('.c3d') &&
                        !names.contains(_name(f).replaceFirst(RegExp(r'\.c3d$'), '.csv'))
                    ? FilledButton(onPressed: () => _toCsv(f), child: Text(t.dataToCsv))
                    : null,
          ),
        const Divider(),
        ListTile(title: Text(t.dataUnits, style: const TextStyle(color: Look.white))),
        if (_trouble != null)
          ListTile(
              title: Text(_trouble!, style: TextStyle(color: Theme.of(context).colorScheme.error))),
        if (sessions == null && _trouble == null)
          const Center(child: CircularProgressIndicator())
        else if (sessions != null && sessions.isEmpty)
          ListTile(dense: true, title: Text(t.dataNoSessions))
        else if (sessions != null)
          for (final s in sessions)
            ListTile(
              title: Text(t.measSession(s.s), style: const TextStyle(color: Look.yellow)),
              subtitle: Text(describeSession(s, _heads[s.s])),
              trailing: Text(switch (sessionState(s, _heads[s.s], names)) {
                SessionState.onPhone => t.dataGot,
                SessionState.switchOn => t.dataSwitchOn(unitsMissing(s, _heads[s.s])
                    .map((n) => '${kindLetter(_heads[s.s]?.units[n] ?? kindBelka)}$n')
                    .join(' ')),
                SessionState.large => t.dataLarge,
                SessionState.ready => t.dataReady,
              }),
              onTap: () => _download(s),
            ),
      ]),
    );
  }
}
