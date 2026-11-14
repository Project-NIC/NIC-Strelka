/// The measurement's choice (APP.md, What it must do, 5; The menus, 3; the sketch
/// app/mockups/en/measurement-a-choice.png): the units of the set as tiles, each its place's
/// code, its kind and number, chosen by a touch — one without a valid calibration never; the
/// session's settings; the synchronisation, the session sent with the units chosen and their kinds
/// (64/1), each one shown in step once it has taken it and Povodok hears it (64/14); the start only
/// then, and the live view after it.
library;

import 'dart:async';

import 'package:material_ui/material_ui.dart';
import 'package:strelka/strelka.dart';

import 'app_settings.dart';
import 'data.dart';
import 'l10n/app_localizations.dart';
import 'live.dart';
import 'places.dart';
import 'main.dart';

class MeasurementScreen extends StatefulWidget {
  const MeasurementScreen(
      {super.key,
      required this.host,
      required this.password,
      required this.client,
      required this.info});

  final String host, password;
  final SmpClient client;
  final Map<String, Object?> info;

  @override
  State<MeasurementScreen> createState() => _MeasurementScreenState();
}

/// What a tile shows of its unit: chosen, free to be, or not to be chosen and why.
enum TileState { chosen, free, noCalibration, expired, unreachable }

TileState tileState(UnitOnSet u, Set<int> chosen, AppSettings s, int now) {
  if (!u.reachable) return TileState.unreachable;
  if (u.calTime == 0) return TileState.noCalibration;
  if (!s.calibrationValid(u.calTime, now)) return TileState.expired;
  return chosen.contains(u.n) ? TileState.chosen : TileState.free;
}

class _MeasurementScreenState extends State<MeasurementScreen> {
  List<UnitOnSet>? _units;
  final Map<int, int> _chips = {};       // each unit's chip, for the set's table
  final Set<int> _chosen = {};
  int _rate = 100, _acc = 16, _gyro = 2000, _hg = 0;

  /// The samples a step: ten at 100 Hz and below, so the sensor runs at 500 or 1 000 Hz on the
  /// set's grid and each step their mean; one above (COMMANDS.md, 64/1).
  int get _n => _rate <= 100 ? 10 : 1;
  LiveCode _live = LiveCode.quat50;
  StreamSubscription<Map<String, Object?>>? _watch;
  Map<String, Object?>? _view;
  bool _sent = false;
  LiveSaveHead? _saveHead;               // the session as sent, for the live view saved
  final _note = TextEditingController(text: settings.value.note);
  String? _trouble;

  @override
  void initState() {
    super.initState();
    unawaited(_load());
    _watch = widget.client
        .watchSet(lost: Duration(seconds: settings.value.lostS))
        .listen((v) => setState(() => _view = v), onError: (Object _) {});
  }

  @override
  void dispose() {
    unawaited(_watch?.cancel());
    _note.dispose();
    super.dispose();
  }

  Future<void> _load() async {
    try {
      final set = await widget.client.read(groupStrelka, Strelka.set);
      final belkas = <int>[];
      for (final b in set['belkas'] as List<Object?>? ?? const []) {
        final m = b as Map<Object?, Object?>;
        final n = m['n'] as int;
        belkas.add(n);
        final chip = m['chip'];
        if (chip is List<int>) {
          var v = 0;
          for (var i = chip.length - 1; i >= 0; i--) {
            v = (v << 8) | chip[i];
          }
          _chips[n] = v;
        }
      }
      belkas.sort();
      final units = await unitsOnSet(widget.host, widget.password, belkas);
      if (mounted) setState(() => _units = units);
    } on Exception catch (e) {
      if (mounted) setState(() => _trouble = '$e');
    }
  }

  void _touch(UnitOnSet u) {
    final st = tileState(u, _chosen, settings.value, nowS());
    if (st != TileState.chosen && st != TileState.free) return;
    setState(() {
      st == TileState.chosen ? _chosen.remove(u.n) : _chosen.add(u.n);
      _sent = false;                     // another choice: the synchronisation again
    });
  }

  Future<void> _synchronise() async {
    final units = {for (final u in _units ?? const <UnitOnSet>[]) u.n: u};
    final now = DateTime.now();
    try {
      // the set's table first, the animal and the units' places, into every unit's file
      final table = tableOf(settings.value, {for (final n in _chosen) n: _chips[n] ?? 0},
          kinds: {for (final n in _chosen) n: units[n]?.kind ?? kindBelka});
      await writeTable(widget.client, table);
      final session = {
        's': settings.value.nextSession,
        // 64/1's rate is the sensor's on the set's grid, the rate into memory times n
        'rate': _rate * _n,
        'n': _n,
        'acc': _acc,
        'gyro': _gyro,
        if (_hg != 0) 'hg': _hg,
        'live': _live.code,
        't': now.microsecondsSinceEpoch,
        'tz': now.timeZoneOffset.inMinutes,
        ...sessionUnits({for (final n in _chosen) n: units[n]?.kind ?? kindBelka}),
      };
      await widget.client.write(groupStrelka, Strelka.session, body: session);
      // what the live view saved needs, the units' 64/0 and 64/3 read before the start
      _saveHead = LiveSaveHead(session: session, table: tableBodies(table), units: [
        for (final n in _chosen.toList()..sort())
          if (units[n] != null) (n, units[n]!.info, units[n]!.cal),
      ]);
      if (mounted) setState(() => _sent = true);
    } on Exception catch (e) {
      if (mounted) setState(() => _trouble = '$e');
    }
  }

  int get _inStep => _view == null ? 0 : inStep(_view!, _chosen);

  /// The chosen units' sensors by 64/0's `sensor`; every unit's while none is chosen.
  List<int?> get _sensors => [
        for (final u in _units ?? const <UnitOnSet>[])
          if (_chosen.isEmpty || _chosen.contains(u.n)) u.info['sensor'] as int?
      ];

  List<int> get _accRanges => rangesOfAll(_sensors, accRangesOf);
  List<int> get _gyroRanges => rangesOfAll(_sensors, gyroRangesOf);

  /// The high-g channel's ranges where a chosen unit has the LSM6DSV320X (hgRangesFor).
  List<int> get _hgRanges => hgRangesFor(_sensors);

  /// The settings kept within what the chosen units have: a range one has not back to the nearest
  /// below, the default where none.
  void _fit() {
    int fit(int v, List<int> have, int otherwise) => have.contains(v)
        ? v
        : have.lastWhere((r) => r < v,
            orElse: () => have.contains(otherwise) ? otherwise : have.first);
    if (!_hgRanges.contains(_hg)) _hg = 0;
    _acc = fit(_acc, [for (final g in _accRanges) if (_hg == 0 || g <= 16) g], 16);
    _gyro = fit(_gyro, _gyroRanges, 2000);
  }

  bool get _canStart => _sent && _chosen.isNotEmpty && _inStep == _chosen.length;

  Future<void> _start() async {
    try {
      try {
        await widget.client.write(groupStrelka, Strelka.work);   // out of the service mode
      } on SmpError {
        // at work already
      }
      await widget.client.write(groupStrelka, Strelka.record, body: {'run': true});
      changeSettings((s) {
        s.lastStart = nowS();
        s.nextSession = (s.nextSession + 1) & 0xFFFF;
      });
      recording.value = true;
      if (!mounted) return;
      await _toLive();
    } on Exception catch (e) {
      if (mounted) setState(() => _trouble = '$e');
    }
  }

  Future<void> _toLive() => Navigator.of(context).push(MaterialPageRoute<void>(
      builder: (_) => LiveScreen(
          client: widget.client,
          host: widget.host,
          accG: _acc,
          hgG: _hg,
          saveHead: _saveHead)));

  @override
  Widget build(BuildContext context) {
    final t = AppLocalizations.of(context);
    final s = settings.value;
    final units = _units;
    _fit();
    final now = nowS();
    final usable = units?.where((u) {
          final st = tileState(u, _chosen, s, now);
          return st == TileState.chosen || st == TileState.free;
        }).length ??
        0;
    return Scaffold(
      appBar: AppBar(title: Text('${t.menuMeasurement} · ${t.measSession(s.nextSession)}')),
      body: ListView(padding: const EdgeInsets.all(12), children: [
        Text(t.measChoose, style: const TextStyle(color: Look.yellow)),
        const SizedBox(height: 8),
        if (units == null)
          const Center(child: CircularProgressIndicator())
        else if (units.isEmpty)
          Text(t.measNone)
        else
          Wrap(spacing: 8, runSpacing: 8, children: [
            for (final u in units) UnitTile(unit: u, state: tileState(u, _chosen, s, now),
                onTap: () => _touch(u)),
          ]),
        const SizedBox(height: 6),
        Text(t.measChosen(_chosen.length, usable), style: const TextStyle(color: Look.yellow)),
        const Divider(),
        _choice<int>(t.measRate, _rate, {for (final r in [50, 100, 200, 500, 1000]) r: '$r Hz'},
            (v) => _rate = v),
        // the ranges every chosen unit's sensor has; both channels with the low-g at ±16 g at most
        _choice<int>(t.measAcc, _acc,
            {for (final g in _accRanges) if (_hg == 0 || g <= 16) g: '±$g g'}, (v) => _acc = v),
        _choice<int>(t.measGyro, _gyro, {for (final d in _gyroRanges) d: '±$d °/s'},
            (v) => _gyro = v),
        // both channels: the hoof's high-g kept beside, the other units the low-g alone
        if (_hgRanges.isNotEmpty)
          _choice<int>(t.measHg, _hg, {
            0: t.measHgOff,
            for (final g in _hgRanges) g: '±$g g',
          }, (v) {
            _hg = v;
            if (v != 0 && _acc > 16) _acc = 16;
          }),
        _choice<LiveCode>(t.measLive, _live, {
          LiveCode.off: t.liveOff,
          LiveCode.quat50: t.liveQuat50,
          LiveCode.quat100: t.liveQuat100,
          LiveCode.raw50: t.liveRaw50,
          LiveCode.raw100: t.liveRaw100,
        }, (v) => _live = v),
        // the user's note, into the set's table and every file's name (APP.md, The names)
        ListTile(
          dense: true,
          title: TextField(
            controller: _note,
            maxLength: noteMost,
            decoration: InputDecoration(labelText: t.measNote),
            onChanged: (v) {
              changeSettings((s) => s.note = v);
              setState(() => _sent = false);
            },
          ),
        ),
        const Divider(),
        Container(
          color: Look.black,
          padding: const EdgeInsets.all(10),
          child: Text(
              _sent ? t.measInStep(_inStep, _chosen.length) : t.measSyncFirst,
              style: TextStyle(color: _canStart ? const Color(0xFF55FF55) : Look.yellow)),
        ),
        if (_trouble != null)
          Padding(
            padding: const EdgeInsets.only(top: 8),
            child: Text(_trouble!, style: TextStyle(color: Theme.of(context).colorScheme.error)),
          ),
        const SizedBox(height: 12),
        Row(children: [
          Expanded(child: FilledButton(onPressed: _toLive, child: Text(t.measLiveOnly))),
          const SizedBox(width: 8),
          Expanded(
              child: FilledButton(
                  onPressed: _chosen.isEmpty ? null : _synchronise, child: Text(t.measSync))),
          const SizedBox(width: 8),
          Expanded(
              child: FilledButton(
            style: FilledButton.styleFrom(backgroundColor: const Color(0xFF00AA00)),
            onPressed: _canStart ? _start : null,
            child: Text(t.measStart),
          )),
        ]),
      ]),
    );
  }

  Widget _choice<T>(String label, T value, Map<T, String> options, void Function(T) set) =>
      ListTile(
        dense: true,
        title: Text(label),
        trailing: DropdownButton<T>(
          value: value,
          dropdownColor: Look.blue,
          items: [
            for (final o in options.entries)
              DropdownMenuItem(value: o.key, child: Text(o.value, style: const TextStyle(color: Look.yellow)))
          ],
          onChanged: (v) => setState(() {
            if (v != null) set(v);
            _sent = false;               // other settings: the synchronisation again
          }),
        ),
      );
}

/// A unit's tile: its place's code large, its kind and number, and why it cannot be chosen.
class UnitTile extends StatelessWidget {
  const UnitTile({super.key, required this.unit, required this.state, required this.onTap});

  final UnitOnSet unit;
  final TileState state;
  final VoidCallback onTap;

  @override
  Widget build(BuildContext context) {
    final t = AppLocalizations.of(context);
    final code = settings.value.places[unit.n] ?? '--';
    final (bg, fg) = switch (state) {
      TileState.chosen => (const Color(0xFF00AA00), Look.black),
      TileState.free => (const Color(0xFFAA0000), Look.white),
      _ => (const Color(0xFF555555), const Color(0xFFAAAAAA)),
    };
    final apart = (unit.tempCenti - unit.calTempCenti).abs() / 100;
    final note = switch (state) {
      TileState.noCalibration => t.measNoCal,
      TileState.expired => t.measExpired,
      TileState.unreachable => t.measUnreachable,
      _ => unit.calTime != 0 && apart > settings.value.tempApartC ? t.measTempApart(apart.round()) : '',
    };
    return InkWell(
      onTap: onTap,
      child: Container(
        width: 120,
        height: 92,
        color: bg,
        padding: const EdgeInsets.all(4),
        child: Column(mainAxisAlignment: MainAxisAlignment.center, children: [
          Text(code, style: TextStyle(color: fg, fontSize: 26, fontWeight: FontWeight.bold)),
          Text(unit.name, style: TextStyle(color: fg)),
          if (note.isNotEmpty)
            Text(note,
                maxLines: 1,
                overflow: TextOverflow.ellipsis,
                style: TextStyle(color: state == TileState.free ? Look.yellow : fg, fontSize: 11)),
        ]),
      ),
    );
  }
}
