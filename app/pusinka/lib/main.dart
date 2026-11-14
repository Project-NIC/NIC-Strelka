/// Pušinka at its base (APP.md): the first screen joins one Povodok by its address and the
/// password, through the lock; then the six menus along the bottom, one screen a task — the
/// overview, the calibration, the measurement, the data manager, the diagnostics, the settings
/// (APP.md, The menus; the sketches in app/mockups). Built: the overview of the set, the
/// calibration, the measurement's choice and its live view, the data manager's download into one
/// C3D, the diagnostics, the settings' language and About. The look is the DOS commanders'; the
/// texts are a table a language, lib/l10n. Analysed and tested as widgets; run on no device yet.
library;

import 'dart:async';
import 'dart:io';
import 'dart:typed_data';

import 'package:cupertino_ui/cupertino_ui.dart' show GlobalCupertinoLocalizations;
import 'package:flutter_localizations/flutter_localizations.dart' show GlobalWidgetsLocalizations;
import 'package:material_ui/material_ui.dart';
import 'package:path_provider/path_provider.dart';
import 'package:strelka/strelka.dart';

import 'app_settings.dart';
import 'calibration.dart';
import 'data.dart';
import 'diagnostics.dart';
import 'firmware.dart';
import 'keys.dart';
import 'l10n/app_localizations.dart';
import 'measurement.dart';
import 'overview.dart';
import 'places.dart';
import 'set_settings.dart';

Future<void> main() async {
  WidgetsFlutterBinding.ensureInitialized();
  settings.value = await AppSettings.load();
  runApp(const PusinkaApp());
}

/// The commanders' colours, the look "Volkov" (APP.md, The menus).
abstract final class Look {
  static const blue = Color(0xFF0000AA);
  static const cyan = Color(0xFF00AAAA);
  static const lightCyan = Color(0xFF55FFFF);
  static const yellow = Color(0xFFFFFF55);
  static const black = Color(0xFF000000);
  static const white = Color(0xFFFFFFFF);

  static ThemeData theme() {
    final base = ThemeData(
      brightness: Brightness.dark,
      colorScheme: const ColorScheme.dark(
          primary: cyan, onPrimary: black, secondary: yellow, surface: blue, onSurface: lightCyan),
      scaffoldBackgroundColor: blue,
      fontFamily: 'monospace',
      fontFamilyFallback: const ['Menlo', 'Consolas', 'DejaVu Sans Mono', 'Courier New'],
    );
    return base.copyWith(
      appBarTheme: const AppBarTheme(backgroundColor: cyan, foregroundColor: black),
      filledButtonTheme: FilledButtonThemeData(
          style: FilledButton.styleFrom(
              backgroundColor: cyan, foregroundColor: black, shape: const RoundedRectangleBorder())),
    );
  }
}

/// The languages of the texts' tables, each named in its own (APP.md, The languages).
const languages = <(Locale, String)>[
  (Locale('en'), 'English'),
  (Locale('cs'), 'Čeština'),
  (Locale('ru'), 'Русский'),
  (Locale('es'), 'Español'),
  (Locale('pt'), 'Português (Brasil)'),
  (Locale('de'), 'Deutsch'),
  (Locale('fr'), 'Français'),
];

/// The language chosen in the settings; none, the phone's own.
final language = ValueNotifier<Locale?>(null);

class PusinkaApp extends StatelessWidget {
  const PusinkaApp({super.key});

  @override
  Widget build(BuildContext context) => ValueListenableBuilder<Locale?>(
        valueListenable: language,
        builder: (context, locale, _) => MaterialApp(
          title: 'Pušinka',
          theme: Look.theme(),
          locale: locale,
          localizationsDelegates: const [
            AppLocalizations.delegate,
            GlobalMaterialLocalizations.delegate,
            GlobalWidgetsLocalizations.delegate,
            GlobalCupertinoLocalizations.delegate,
          ],
          supportedLocales: AppLocalizations.supportedLocales,
          home: const JoinScreen(),
        ),
      );
}

/// The first screen: Povodok's address — 192.168.7.1 on its cable (PHONE.md) — and the
/// password; the lock passed, the menus.
class JoinScreen extends StatefulWidget {
  const JoinScreen({super.key});

  @override
  State<JoinScreen> createState() => _JoinScreenState();
}

class _JoinScreenState extends State<JoinScreen> {
  final _host = TextEditingController(text: '192.168.7.1');
  final _password = TextEditingController();
  String? _trouble;
  bool _joining = false;

  @override
  void dispose() {
    _host.dispose();
    _password.dispose();
    super.dispose();
  }

  @override
  void initState() {
    super.initState();
    unawaited(Keys.password(_host.text).then((pw) {
      if (pw != null && mounted && _password.text.isEmpty) _password.text = pw;
    }));
  }

  /// Another key than the one kept: shown both, taken only at the user's word.
  Future<bool> _takeNewKey(List<int>? old, List<int> now) async {
    final t = AppLocalizations.of(context);
    final was = old == null ? '?' : await keyFingerprint(old);
    final is_ = await keyFingerprint(now);
    if (!mounted) return false;
    return await showDialog<bool>(
          context: context,
          builder: (c) => AlertDialog(
            title: Text(t.keyChangedTitle),
            content: Text(t.keyChanged(is_, was)),
            actions: [
              TextButton(onPressed: () => Navigator.pop(c, false), child: Text(t.dataCancel)),
              FilledButton(onPressed: () => Navigator.pop(c, true), child: Text(t.keyTake)),
            ],
          ),
        ) ==
        true;
  }

  Future<void> _join() async {
    final t = AppLocalizations.of(context);
    final host = _host.text.trim();
    setState(() {
      _joining = true;
      _trouble = null;
    });
    try {
      final pw = _password.text.isNotEmpty ? _password.text : await Keys.password(host) ?? '';
      var pinned = await Keys.pinned(host);
      SmpClient client;
      try {
        client = await SmpClient.connect(host, password: pw, pinned: pinned);
      } on KeyMismatch catch (e) {
        if (!await _takeNewKey(pinned, e.met)) return;
        pinned = e.met;
        await Keys.pin(host, e.met);
        client = await SmpClient.connect(host, password: pw, pinned: pinned);
      }
      final key = client.unitKey!;
      if (pinned == null) {
        await Keys.pin(host, key);                     // the first joining: kept from now on
        final fp = await keyFingerprint(key);
        if (mounted) ScaffoldMessenger.of(context).showSnackBar(SnackBar(content: Text(t.keyNew(fp))));
      }
      SmpClient.pins[host] = key;
      await Keys.setPassword(host, pw);
      final info = await client.read(groupStrelka, Strelka.info);
      if (!mounted) {
        client.close();
        return;
      }
      await Navigator.of(context).push(MaterialPageRoute<void>(
        builder: (_) => MenuShell(host: host, password: pw, client: client, info: info),
      ));
    } on Exception catch (e) {
      // whatever went wrong is said on the screen, never thrown — and only while it stands
      if (!mounted) return;
      setState(() => _trouble = switch (e) {
            SmpError(rc: Rc.access) => t.joinRefused,
            SocketException(:final message) => t.joinNoAnswer(message),
            TimeoutException() => t.joinTimeout,
            _ => '$e',
          });
    } finally {
      if (mounted) setState(() => _joining = false);
    }
  }

  @override
  Widget build(BuildContext context) {
    final t = AppLocalizations.of(context);
    return Scaffold(
      appBar: AppBar(title: const Text('Pušinka')),
      body: Padding(
        padding: const EdgeInsets.all(24),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.stretch,
          children: [
            TextField(
              controller: _host,
              decoration: InputDecoration(labelText: t.joinAddress),
              keyboardType: TextInputType.url,
            ),
            const SizedBox(height: 12),
            TextField(
              controller: _password,
              decoration: InputDecoration(labelText: t.joinPassword),
              obscureText: true,
              onSubmitted: (_) => _joining ? null : _join(),
            ),
            const SizedBox(height: 24),
            FilledButton(
              onPressed: _joining ? null : _join,
              child: Text(_joining ? t.joinJoining : t.joinButton),
            ),
            if (_trouble != null) ...[
              const SizedBox(height: 16),
              Text(_trouble!, style: TextStyle(color: Theme.of(context).colorScheme.error)),
            ],
          ],
        ),
      ),
    );
  }
}

/// Joined: the six menus along the bottom, a bar of 1–6 as the commanders' keys; the connection
/// is the menus', closed with them.
class MenuShell extends StatefulWidget {
  const MenuShell(
      {super.key,
      required this.host,
      required this.password,
      required this.client,
      required this.info});

  final String host, password;
  final SmpClient client;
  final Map<String, Object?> info;

  @override
  State<MenuShell> createState() => _MenuShellState();
}

class _MenuShellState extends State<MenuShell> {
  int _menu = 0;

  @override
  void initState() {
    super.initState();
    // the factory password, shorter than twelve: a new one asked at once (APP.md, What it must
    // do, 1)
    if (widget.password.length < 12) {
      WidgetsBinding.instance.addPostFrameCallback((_) {
        if (mounted) {
          unawaited(changePasswordDialog(context, widget.client,
              host: widget.host, oldPassword: widget.password, factory: true));
        }
      });
    }
  }

  @override
  void dispose() {
    widget.client.close();
    super.dispose();
  }

  Widget _screen(AppLocalizations t) => switch (_menu) {
        0 => OverviewScreen(
            host: widget.host, password: widget.password, client: widget.client, info: widget.info),
        1 => CalibrationScreen(host: widget.host, password: widget.password, client: widget.client),
        2 => MeasurementScreen(
            host: widget.host, password: widget.password, client: widget.client, info: widget.info),
        3 => DataScreen(host: widget.host, password: widget.password, client: widget.client),
        4 => DiagnosticsScreen(client: widget.client),
        _ => SettingsScreen(client: widget.client, host: widget.host, password: widget.password),
      };

  @override
  Widget build(BuildContext context) {
    final t = AppLocalizations.of(context);
    return Scaffold(
      body: _screen(t),
      bottomNavigationBar: SafeArea(
        child: MenuBar6(current: _menu, names: menuNames(t), onChosen: (i) => setState(() => _menu = i)),
      ),
    );
  }
}

List<String> menuNames(AppLocalizations t) => [
      t.menuOverview,
      t.menuCalibration,
      t.menuMeasurement,
      t.menuData,
      t.menuDiagnostics,
      t.menuSettings
    ];

/// The bar along the bottom: a key a menu, its number white on black, its name black on cyan,
/// the current one on yellow.
class MenuBar6 extends StatelessWidget {
  const MenuBar6({super.key, required this.current, required this.names, required this.onChosen});

  final int current;
  final List<String> names;
  final ValueChanged<int> onChosen;

  @override
  Widget build(BuildContext context) => ColoredBox(
        color: Look.black,
        child: Row(children: [
          for (var i = 0; i < names.length; i++)
            Expanded(
              child: InkWell(
                onTap: () => onChosen(i),
                child: Padding(
                  padding: const EdgeInsets.symmetric(vertical: 6),
                  child: Row(children: [
                    Padding(
                      padding: const EdgeInsets.symmetric(horizontal: 4),
                      child: Text('${i + 1}', style: const TextStyle(color: Look.white)),
                    ),
                    Expanded(
                      child: Container(
                        color: i == current ? Look.yellow : Look.cyan,
                        padding: const EdgeInsets.symmetric(horizontal: 4, vertical: 8),
                        margin: const EdgeInsets.only(right: 3),
                        child: Text(names[i],
                            maxLines: 1,
                            overflow: TextOverflow.ellipsis,
                            style: const TextStyle(color: Look.black)),
                      ),
                    ),
                  ]),
                ),
              ),
            ),
        ]),
      );
}

/// The settings (APP.md, The menus, 6): the language of the seven, the set's own, the
/// measurement's, the places' templates and About.
class SettingsScreen extends StatelessWidget {
  const SettingsScreen({super.key, this.client, this.host = '', this.password = ''});

  final SmpClient? client;      // Povodok's, for the set's units
  final String host;            // its address: its key and the password kept by it
  final String password;        // the units' ports joined with it, the firmware's update

  @override
  Widget build(BuildContext context) => ValueListenableBuilder<bool>(
      valueListenable: recording,
      builder: (context, measuring, _) => ValueListenableBuilder<AppSettings>(
          valueListenable: settings, builder: (context, s, _) => _build(context, s, measuring)));

  /// A setting's row: its choices, each named; none chosen while the set records.
  Widget _row<T>(String label, T value, Map<T, String> options, void Function(AppSettings, T) set,
          bool measuring, {String? note}) =>
      ListTile(
        dense: true,
        title: Text(label),
        subtitle: note == null ? null : Text(note, style: const TextStyle(fontSize: 11)),
        trailing: DropdownButton<T>(
          value: value,
          dropdownColor: Look.blue,
          items: [
            for (final o in options.entries)
              DropdownMenuItem(
                  value: o.key, child: Text(o.value, style: const TextStyle(color: Look.yellow)))
          ],
          onChanged: measuring ? null : (v) => v == null ? null : changeSettings((s) => set(s, v)),
        ),
      );

  Widget _build(BuildContext context, AppSettings s, bool measuring) {
    final t = AppLocalizations.of(context);
    final now = Localizations.localeOf(context).languageCode;
    return Scaffold(
      appBar: AppBar(title: Text(t.menuSettings)),
      body: ListView(padding: const EdgeInsets.all(16), children: [
        Text(t.settingsLanguage, style: const TextStyle(color: Look.white)),
        for (final (locale, name) in languages)
          ListTile(
            leading: Text(locale.languageCode == now ? '(•)' : '( )',
                style: const TextStyle(color: Look.yellow)),
            title: Text(name),
            onTap: () => language.value = locale,
          ),
        const SizedBox(height: 24),
        if (client != null) ...[
          ListTile(
            title: Text(t.setNewSet, style: const TextStyle(color: Look.white)),
            trailing: const Text('▶', style: TextStyle(color: Look.yellow)),
            enabled: !measuring,
            onTap: () => Navigator.of(context).push(
                MaterialPageRoute<void>(builder: (_) => NewSetScreen(client: client))),
          ),
          ListTile(
            title: Text(t.setNewUnits, style: const TextStyle(color: Look.white)),
            trailing: const Text('▶', style: TextStyle(color: Look.yellow)),
            enabled: !measuring,
            onTap: () => Navigator.of(context).push(
                MaterialPageRoute<void>(builder: (_) => NewUnitsScreen(client: client!))),
          ),
          ListTile(
            title: Text(t.pwTitle, style: const TextStyle(color: Look.white)),
            trailing: const Text('▶', style: TextStyle(color: Look.yellow)),
            enabled: !measuring,
            onTap: () => changePasswordDialog(context, client!, host: host),
          ),
          if (SmpClient.pins[host] != null)
            ListTile(
              title: Text(t.keyFingerprint, style: const TextStyle(color: Look.white)),
              subtitle: FutureBuilder<String>(
                  future: keyFingerprint(SmpClient.pins[host]!),
                  builder: (_, f) => Text(f.data ?? '')),
            ),
          ListTile(
            title: Text(t.fwTitle, style: const TextStyle(color: Look.white)),
            trailing: const Text('▶', style: TextStyle(color: Look.yellow)),
            enabled: !measuring,
            onTap: () => Navigator.of(context).push(MaterialPageRoute<void>(
                builder: (_) => FirmwareScreen(client: client, host: host, password: password))),
          ),
          ListTile(
            title: Text(t.offTitle, style: const TextStyle(color: Look.white)),
            trailing: const Text('▶', style: TextStyle(color: Look.yellow)),
            onTap: () => povodokOffDialog(context, client!),
          ),
        ],
        ListTile(
          title: Text(t.placesTitle, style: const TextStyle(color: Look.white)),
          subtitle: Text(animalName(t, s.template)),
          trailing: const Text('▶', style: TextStyle(color: Look.yellow)),
          onTap: () => Navigator.of(context)
              .push(MaterialPageRoute<void>(builder: (_) => PlacesScreen(client: client))),
        ),
        const SizedBox(height: 24),
        Text(t.setMeasurement, style: const TextStyle(color: Look.white)),
        _row<CalValidity>(t.setCalValid, s.calValidity, {
          CalValidity.everyMeasurement: t.calEvery,
          CalValidity.twoHours: t.cal2h,
          CalValidity.day: t.calDay,
          CalValidity.thirtyDays: t.cal30,
          CalValidity.unlimited: t.calUnlimited,
        }, (s, v) => s.calValidity = v, measuring),
        _row<int>(t.setTempApart, s.tempApartC, {for (final c in [5, 10, 15, 20]) c: '$c °C'},
            (s, v) => s.tempApartC = v, measuring),
        _row<int>(t.setCurves, s.curvesS, {for (final c in [2, 5, 10, 30]) c: '$c s'},
            (s, v) => s.curvesS = v, measuring),
        _row<int>(t.setNumbers, s.numbersStrides, {for (final c in [5, 10, 20, 50]) c: t.strides(c)},
            (s, v) => s.numbersStrides = v, measuring),
        _row<int>(t.setLost, s.lostS, {for (final c in [2, 3, 5, 10]) c: '$c s'},
            (s, v) => s.lostS = v, measuring),
        _row<bool>(t.setPower, s.fullPower, {false: '+5 dBm', true: t.powerFull},
            (s, v) => s.fullPower = v, measuring, note: t.setToCome),
        _row<LightWhileMeasuring>(t.setLight, s.light, {
          LightWhileMeasuring.usual: t.lightUsual,
          LightWhileMeasuring.faultsAlone: t.lightFaults,
          LightWhileMeasuring.off: t.lightOff,
        }, (s, v) => s.light = v, measuring, note: t.setToCome),
        Text(t.setLocked, style: const TextStyle(fontSize: 12)),
        const SizedBox(height: 24),
        Text(t.settingsAbout, style: const TextStyle(color: Look.white)),
        const SizedBox(height: 8),
        const Text('Pušinka 0.1 · NIC-Strelka 0.86', style: TextStyle(color: Look.white)),
        const SizedBox(height: 8),
        Image.asset('assets/pusinka.jpg', height: 240, alignment: Alignment.centerLeft),
        const SizedBox(height: 4),
        Text(t.aboutPicture),
        const SizedBox(height: 8),
        Text(t.aboutOpen),
      ]),
    );
  }
}

/// One session coming in: each Belka's share, the pace, the Belkas left out, the C3D written as
/// the frames come; broken off, it goes on from where it stopped (SessionDownload).
class DownloadScreen extends StatefulWidget {
  const DownloadScreen(
      {super.key, required this.host, required this.password, required this.session});

  final String host, password;
  final SessionOnSet session;

  @override
  State<DownloadScreen> createState() => _DownloadScreenState();
}

class _DownloadScreenState extends State<DownloadScreen> {
  IOSink? _sink;
  String? _path, _dir;
  final Map<int, RandomAccessFile> _card = {};    // each unit's card file, STRK, as it comes
  SessionDownload? _download;
  final Stopwatch _clock = Stopwatch();
  bool _running = false;
  String? _trouble;

  @override
  void initState() {
    super.initState();
    unawaited(_start());
  }

  @override
  void dispose() {
    _download?.cancel();
    unawaited(_sink?.close());
    for (final f in _card.values) {
      f.closeSync();
    }
    super.dispose();
  }

  String _strk(int n) => 'S${'${widget.session.s}'.padLeft(5, '0')}B${'$n'.padLeft(2, '0')}.STRK';

  /// A piece of a unit's card file, written where it lies in it: the card's file as it is.
  void _piece(int n, int off, Uint8List data) {
    final f = _card.putIfAbsent(
        n, () => File('$_dir/${_strk(n)}.part').openSync(mode: FileMode.write));
    f
      ..setPositionSync(off)
      ..writeFromSync(data);
  }

  /// The download over: the C3D and the card's files named by the session's date and time and the
  /// user's note (APP.md, The names).
  Future<void> _name() async {
    final d = _download!;
    await _sink?.close();
    _sink = null;
    for (final f in _card.values) {
      f.closeSync();
    }
    final head = d.head;
    final t = head != null && head.timeUs > 0
        ? DateTime.fromMicrosecondsSinceEpoch(head.timeUs, isUtc: true).toLocal()
        : DateTime.now();
    final base = recordBase(t, widget.session.s, tableNote(d.table ?? ''));
    final c3d = await File(_path!).rename('$_dir/${base}_card.c3d');
    for (final n in _card.keys) {
      await File('$_dir/${_strk(n)}.part').rename('$_dir/${base}_card_${_strk(n)}');
    }
    _card.clear();
    if (mounted) setState(() => _path = c3d.path);
  }

  Future<void> _start() async {
    try {
      final dir = Directory('${(await getApplicationDocumentsDirectory()).path}/Strelka');
      await dir.create(recursive: true);
      final path = '${dir.path}/S${'${widget.session.s}'.padLeft(5, '0')}.c3d.part';
      final sink = File(path).openWrite();
      _sink = sink;
      _path = path;
      _dir = dir.path;
      _download = SessionDownload(widget.session, sink.add, onPiece: _piece);
      await _run();
    } on Exception catch (e) {
      if (mounted) setState(() => _trouble = '$e');
    }
  }

  Future<void> _run() async {
    final d = _download;
    if (d == null) return;
    setState(() {
      _running = true;
      _trouble = null;
    });
    _clock.start();
    try {
      final over = await d.run(widget.host, widget.password, onProgress: (_) {
        if (mounted) setState(() {});
      });
      if (over) await _name();
    } on Exception catch (e) {
      if (mounted) {
        setState(() => _trouble = switch (e) {
              SmpError(rc: Rc.badState) => 'The set is recording or showing its live view.',
              SmpError(rc: Rc.busy) => 'Povodok is still busy: continue in a moment.',
              SocketException(:final message) => 'Povodok does not answer: $message',
              _ => '$e',
            });
      }
    } finally {
      _clock.stop();
      if (mounted) setState(() => _running = false);
    }
  }

  @override
  Widget build(BuildContext context) {
    final t = AppLocalizations.of(context);
    final d = _download;
    final p = d?.progress;
    final total = widget.session.bytes;
    final secs = _clock.elapsedMilliseconds / 1000;
    final pace = p != null && secs > 0 ? p.bytes / secs / 1000 : 0.0;
    final left = pace > 0 && p != null ? (total - p.bytes) / 1000 / pace : null;
    return Scaffold(
      appBar: AppBar(title: Text(t.measSession(widget.session.s))),
      body: ListView(
        padding: const EdgeInsets.all(16),
        children: [
          LinearProgressIndicator(value: p == null || total == 0 ? null : p.bytes / total),
          const SizedBox(height: 8),
          Text(p == null
              ? t.dlStarting
              : p.over
                  ? t.dlDone(_path ?? '')
                  : '${t.dlOf((p.bytes / 1000).round(), (total / 1000).round())} · '
                      '${pace.toStringAsFixed(0)} kB/s'
                      '${left != null ? ' · ${t.dlLeft(left.round())}' : ''}'),
          if (_trouble != null)
            Padding(
              padding: const EdgeInsets.only(top: 8),
              child: Text(_trouble!, style: TextStyle(color: Theme.of(context).colorScheme.error)),
            ),
          const SizedBox(height: 8),
          if (p != null)
            for (final n in widget.session.belkas)
              ListTile(
                dense: true,
                leading: CircleAvatar(child: Text('$n')),
                title: LinearProgressIndicator(
                    value: (p.got[n] ?? 0) / (widget.session.records[n]!.bytes.clamp(1, 1 << 62))),
                subtitle: Text(p.gone.contains(n)
                    ? t.dlLeftOut
                    : t.dlOf(((p.got[n] ?? 0) / 1000).round(),
                        (widget.session.records[n]!.bytes / 1000).round())),
              ),
          const SizedBox(height: 16),
          if (p != null && !p.over)
            _running
                ? OutlinedButton(onPressed: () => d?.cancel(), child: Text(t.actStop))
                : FilledButton(onPressed: _run, child: Text(t.actContinue)),
        ],
      ),
    );
  }
}
