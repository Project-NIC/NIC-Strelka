/// The overview (APP.md, What it must do, 2; The menus, 1; the sketches
/// app/mockups/en/overview.png and unit.png): the Povodoks, the joined one and those it hears; every
/// unit of the set, its place's code, kind and number, its state and fault as Povodok has them by
/// 64/14 once a second, its cell, temperature and card as it says by 64/0 through its port; a
/// touch on a row opens the unit's page: what it says of itself, and a blink, the card erased and
/// the unit forgotten under the password asked again, the unit switched off.
library;

import 'dart:async';
import 'dart:typed_data';

import 'package:material_ui/material_ui.dart';
import 'package:strelka/strelka.dart';

import 'app_settings.dart';
import 'l10n/app_localizations.dart';
import 'main.dart';

String stateName(AppLocalizations t, int? s) => switch (s) {
      0 => t.stRecording,
      1 => t.stAlone,
      2 => t.stSearching,
      3 => t.stPaused,
      4 => t.stWaiting,
      5 => t.stCalibrating,
      6 => t.stGiving,
      7 => t.stFirmware,
      8 => t.stErasing,
      _ => '${s ?? '?'}',
    };

String faultName(AppLocalizations t, int? f) => switch (f) {
      null || 0 => '',
      1 => t.fNoCard,
      2 => t.fCardFull,
      3 => t.fWrite,
      4 => t.fFs,
      5 => t.fDropped,
      6 => t.fSensor,
      7 => t.fClock,
      8 => t.fCellLow,
      9 => t.fCharger,
      10 => t.fPmic,
      11 => t.fNoCal,
      12 => t.fRefused,
      _ => '$f',
    };

/// The sensor by its WHO_AM_I (COMMANDS.md, 64/0).
String sensorName(int? id) => switch (id) {
      0x70 => 'LSM6DSV16X',
      0x73 => 'LSM6DSV320X',
      0x44 => 'ICM-42686-P',
      0x47 => 'ICM-42688-P',
      _ => '?',
    };

/// Bytes as hex: a chip's ID little-endian as it comes, shown the other way round.
String hexBytes(Object? bytes, {bool reversed = true}) {
  if (bytes is! Uint8List) return '?';
  final order = reversed ? bytes.reversed : bytes;
  return order.map((b) => b.toRadixString(16).padLeft(2, '0')).join().toUpperCase();
}

class OverviewScreen extends StatefulWidget {
  const OverviewScreen(
      {super.key,
      required this.host,
      required this.password,
      required this.client,
      required this.info});

  final String host, password;
  final SmpClient client;
  final Map<String, Object?> info;

  @override
  State<OverviewScreen> createState() => _OverviewScreenState();
}

class _OverviewScreenState extends State<OverviewScreen> {
  StreamSubscription<Map<String, Object?>>? _watch;
  Map<String, Object?>? _view;
  final Map<int, Map<String, Object?>?> _infos = {};   // 64/0 of each unit; null out of reach

  @override
  void initState() {
    super.initState();
    _watch = widget.client.watchSet(lost: Duration(seconds: settings.value.lostS)).listen((v) {
      if (!mounted) return;
      setState(() => _view = v);
      if (_infos.isEmpty) unawaited(_readInfos());
    }, onError: (Object _) {});
  }

  @override
  void dispose() {
    unawaited(_watch?.cancel());
    super.dispose();
  }

  List<Map<Object?, Object?>> get _units => [
        for (final b in _view?['belkas'] as List<Object?>? ?? const []) b as Map<Object?, Object?>
      ]..sort((a, b) => (a['n'] as int).compareTo(b['n'] as int));

  Future<void> _readInfos() async {
    for (final u in _units) {
      final n = u['n'] as int;
      _infos[n] = null;
      try {
        final c = await SmpClient.connect(widget.host, port: smpPort + n, password: widget.password);
        try {
          _infos[n] = await c.read(groupStrelka, Strelka.info);
        } finally {
          c.close();
        }
      } on Object {
        // out of reach: its row says what 64/14 knows
      }
      if (mounted) setState(() {});
    }
  }

  @override
  Widget build(BuildContext context) {
    final t = AppLocalizations.of(context);
    final heard = _view?['povodoks'] as int? ?? 0;
    final mine = widget.info['n'] as int? ?? 0;
    return Scaffold(
      appBar: AppBar(title: Text(t.menuOverview), actions: [
        IconButton(
            icon: const Icon(Icons.refresh),
            onPressed: () {
              _infos.clear();
              unawaited(_readInfos());
            }),
      ]),
      body: ListView(children: [
        ListTile(title: Text(t.ovPovodoks, style: const TextStyle(color: Look.white))),
        Wrap(spacing: 16, children: [
          for (var p = 0; p < 4; p++)
            Padding(
              padding: const EdgeInsets.symmetric(horizontal: 16),
              child: Text(
                  'P${p + 1} ${p == mine ? t.ovToPhone : heard >> p & 1 != 0 ? t.ovHeard : t.ovNotHeard}',
                  style: TextStyle(
                      color: p == mine
                          ? const Color(0xFF55FF55)
                          : heard >> p & 1 != 0
                              ? Look.white
                              : const Color(0xFFAAAAAA))),
            ),
        ]),
        const Divider(),
        ListTile(title: Text(t.overviewUnits, style: const TextStyle(color: Look.white))),
        if (_view == null)
          const Center(child: CircularProgressIndicator())
        else if (_units.isEmpty)
          ListTile(title: Text(t.overviewNone))
        else
          SingleChildScrollView(
            scrollDirection: Axis.horizontal,
            child: DataTable(
              columnSpacing: 18,
              showCheckboxColumn: false,
              columns: [
                for (final h in [t.ovCode, t.ovUnit, t.ovState, t.ovCell, '°C', t.ovCard, t.ovHeardAgo, t.ovFault])
                  DataColumn(label: Text(h, style: const TextStyle(color: Look.white))),
              ],
              rows: [for (final u in _units) _row(t, u)],
            ),
          ),
      ]),
    );
  }

  DataRow _row(AppLocalizations t, Map<Object?, Object?> u) {
    final n = u['n'] as int;
    final info = _infos[n];
    final card = info?['card'] as Map<Object?, Object?>?;
    final cell = info?['cell'] as Map<Object?, Object?>?;
    final kind = info?['kind'] as int? ?? kindBelka;
    final fault = faultName(t, u['fault'] as int?);
    return DataRow(
      onSelectChanged: (_) => Navigator.of(context).push(MaterialPageRoute<void>(
          builder: (_) => UnitScreen(host: widget.host, password: widget.password, n: n))),
      cells: [
        DataCell(Text(settings.value.places[n] ?? '--',
            style: const TextStyle(color: Look.yellow, fontWeight: FontWeight.bold))),
        DataCell(Text('${kindLetter(kind)}$n')),
        DataCell(Text(stateName(t, u['state'] as int?))),
        DataCell(Text(cell == null ? '' : '${cell['pct']} %')),
        DataCell(Text(info == null ? '' : ((info['temp'] as int? ?? 0) / 100).toStringAsFixed(1))),
        DataCell(Text(card == null
            ? ''
            : card['in'] == true
                ? '${card['free']} MB'
                : kind == kindKryska
                    ? t.ovFlash
                    : t.fNoCard)),
        DataCell(Text('${((u['ago'] as int? ?? 0) / 50).round()} s')),
        DataCell(Text(fault, style: const TextStyle(color: Color(0xFFFF5555)))),
      ],
    );
  }
}

/// A unit's own page: what it says of itself, and what can be done with it.
class UnitScreen extends StatefulWidget {
  const UnitScreen({super.key, required this.host, required this.password, required this.n});

  final String host, password;
  final int n;

  @override
  State<UnitScreen> createState() => _UnitScreenState();
}

class _UnitScreenState extends State<UnitScreen> {
  Map<String, Object?>? _info;
  String? _said;

  @override
  void initState() {
    super.initState();
    unawaited(_load());
  }

  Future<T> _with<T>(Future<T> Function(SmpClient c) act) async {
    final c = await SmpClient.connect(widget.host, port: smpPort + widget.n, password: widget.password);
    try {
      return await act(c);
    } finally {
      c.close();
    }
  }

  Future<void> _load() async {
    try {
      final info = await _with((c) => c.read(groupStrelka, Strelka.info));
      if (mounted) setState(() => _info = info);
    } on Object catch (e) {
      if (mounted) setState(() => _said = '$e');
    }
  }

  Future<void> _act(Future<void> Function(SmpClient c) act) async {
    final t = AppLocalizations.of(context);
    try {
      await _with(act);
      if (mounted) setState(() => _said = t.uDone);
    } on SmpError catch (e) {
      if (mounted) setState(() => _said = e.rc == Rc.badState ? t.uNotNow : '$e');
    } on Object catch (e) {
      if (mounted) setState(() => _said = '$e');
    }
  }

  /// The password asked again, for the card's erasing and the forgetting (COMMANDS.md, The lock).
  Future<String?> _askPassword() {
    final t = AppLocalizations.of(context);
    final field = TextEditingController();
    return showDialog<String>(
      context: context,
      builder: (c) => AlertDialog(
        backgroundColor: const Color(0xFFAA0000),
        title: Text(t.uPassword),
        content: TextField(controller: field, obscureText: true, autofocus: true),
        actions: [
          TextButton(onPressed: () => Navigator.pop(c), child: Text(t.dataCancel)),
          FilledButton(onPressed: () => Navigator.pop(c, field.text), child: const Text('OK')),
        ],
      ),
    );
  }

  Future<void> _underPassword(int cmd, Map<String, Object?>? body) async {
    final pw = await _askPassword();
    if (pw == null || pw.isEmpty) return;
    await _act((c) => c.afresh(pw, cmd, body: body));
  }

  @override
  Widget build(BuildContext context) {
    final t = AppLocalizations.of(context);
    final i = _info;
    final kind = i?['kind'] as int? ?? kindBelka;
    final card = i?['card'] as Map<Object?, Object?>?;
    final cell = i?['cell'] as Map<Object?, Object?>?;
    final fw = i?['fw'];
    final cal = i?['cal'] as int? ?? 0;
    final calText = cal == 0
        ? t.fNoCal
        : '${DateTime.fromMillisecondsSinceEpoch(cal * 1000).toLocal().toString().substring(0, 10)}'
            '${settings.value.calibrationValid(cal, nowS()) ? '' : ' · ${t.measExpired}'}';
    final rows = i == null
        ? const <(String, String)>[]
        : [
            (t.uKind, '${const ['Belka', 'Kryska', 'Zvjozdočka', '?'][kind & 3]}, ${widget.n}'),
            (t.uChip, hexBytes(i['chip'])),
            (t.uSensor, sensorName(i['sensor'] as int?)),
            (t.uFirmware, '${fw is List<Object?> ? fw.join('.') : '?'} (${hexBytes(i['build'], reversed: false)})'),
            (t.ovCell, cell == null ? '' : '${cell['pct']} % · ${cell['mv']} mV'),
            (t.uTemp, '${((i['temp'] as int? ?? 0) / 100).toStringAsFixed(1)} °C'),
            (
              t.ovCard,
              card == null
                  ? ''
                  : card['in'] == true
                      ? '${card['free']} / ${card['mb']} MB'
                      : kind == kindKryska
                          ? t.ovFlash
                          : t.fNoCard
            ),
            (t.uCal, calText),
            (t.ovState, stateName(t, i['state'] as int?)),
            (t.ovFault, faultName(t, i['fault'] as int?)),
          ];
    return Scaffold(
      appBar: AppBar(title: Text('${t.menuOverview} · ${kindLetter(kind)}${widget.n}')),
      body: ListView(padding: const EdgeInsets.all(12), children: [
        for (final (k, v) in rows)
          ListTile(dense: true, title: Text(k), trailing: Text(v, style: const TextStyle(color: Look.white))),
        if (i == null && _said == null) const Center(child: CircularProgressIndicator()),
        const Divider(),
        Text(t.uWith, style: const TextStyle(color: Look.white)),
        const SizedBox(height: 8),
        Wrap(spacing: 8, runSpacing: 8, children: [
          FilledButton(
              onPressed: () => _act((c) => c.write(groupStrelka, Strelka.blink, body: {'s': 10})),
              child: Text(t.uBlink)),
          FilledButton(
              style: FilledButton.styleFrom(backgroundColor: const Color(0xFFAA0000)),
              onPressed: recording.value ? null : () => _underPassword(Strelka.card, {'do': 1}),
              child: Text(t.uErase)),
          FilledButton(
              style: FilledButton.styleFrom(backgroundColor: const Color(0xFFAA0000)),
              onPressed: recording.value ? null : () => _underPassword(Strelka.forget, null),
              child: Text(t.uForget)),
          FilledButton(
              onPressed: () => _act((c) => c.write(groupStrelka, Strelka.off)),
              child: Text(t.uOff)),
        ]),
        const SizedBox(height: 8),
        Text(t.uForgotten, style: const TextStyle(fontSize: 12)),
        if (_said != null) Padding(padding: const EdgeInsets.only(top: 12), child: Text(_said!)),
      ]),
    );
  }
}
