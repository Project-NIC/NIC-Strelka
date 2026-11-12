/// The calibration (APP.md, What it must do, 7; The menus, 2; the sketch
/// app/mockups/en/calibration.png): on the table after the warm-up, every unit of the set onto the
/// same face, confirmed by a touch, each asked by 64/22; the Belka takes 5 s still and answers,
/// a unit moved or on another face named and asked again; between the faces the turn to make.
/// After the sixth each Belka computes its calibration and keeps it; each one's age shown.
library;

import 'dart:async';

import 'package:material_ui/material_ui.dart';
import 'package:strelka/strelka.dart';

import 'app_settings.dart';
import 'l10n/app_localizations.dart';
import 'main.dart';

/// The six faces in their order, 64/22's `face`, and the axis turned about onto each from the
/// one before: a turn about one axis alone measures its gyro (FIRMWARE.md, Calibration).
const List<String> calFaceNames = ['Z ↑', 'Z ↓', 'X ↑', 'X ↓', 'Y ↑', 'Y ↓'];
const List<String> calTurnAxes = ['', 'X', 'Y', 'Z', 'Z', 'X'];

/// A face asked of the unit numbered n, in the ranges given, hg the high-g channel's or 0; its
/// answer.
typedef FaceAsker = Future<FaceAnswer> Function(int n, int face, int acc, int gyro, int hg);

class CalibrationScreen extends StatefulWidget {
  const CalibrationScreen(
      {super.key,
      required this.host,
      required this.password,
      this.client,
      this.loadUnits,
      this.askFace});

  final String host, password;
  final SmpClient? client;      // Povodok's, for the set's units

  /// For the tests: the units and the faces' answers without a set.
  final Future<List<UnitOnSet>> Function()? loadUnits;
  final FaceAsker? askFace;

  @override
  State<CalibrationScreen> createState() => _CalibrationScreenState();
}

class _CalibrationScreenState extends State<CalibrationScreen> {
  List<UnitOnSet>? _units;
  final Map<int, SmpClient> _clients = {};
  int _face = 0, _acc = 16, _gyro = 2000;
  final Set<int> _taken = {};
  final Map<int, FaceGot> _got = {};
  bool _busy = false, _done = false;
  String? _trouble;

  @override
  void initState() {
    super.initState();
    unawaited(_load());
  }

  @override
  void dispose() {
    _closeAll();
    super.dispose();
  }

  void _closeAll() {
    for (final c in _clients.values) {
      c.close();
    }
    _clients.clear();
  }

  Future<List<UnitOnSet>> _readUnits() async {
    final set = await widget.client!.read(groupStrelka, Strelka.set);
    final belkas = [
      for (final b in set['belkas'] as List<Object?>? ?? const [])
        (b as Map<Object?, Object?>)['n'] as int
    ]..sort();
    return unitsOnSet(widget.host, widget.password, belkas);
  }

  Future<void> _load() async {
    try {
      final units = await (widget.loadUnits ?? _readUnits)();
      if (mounted) setState(() => _units = units);
    } on Exception catch (e) {
      if (mounted) setState(() => _trouble = '$e');
    }
  }

  Future<FaceAnswer> _ask(int n, int face, int acc, int gyro, int hg) async {
    final c = _clients[n] ??=
        await SmpClient.connect(widget.host, port: smpPort + n, password: widget.password);
    return calibrateFace(c, face, acc: acc, gyro: gyro, hg: hg);
  }

  /// The hoof's sensor's high-g channel calibrated beside, at its finest range (64/22's hg).
  int _hgOf(int n) =>
      _units?.where((u) => u.n == n).firstOrNull?.info['sensor'] == lsm320 ? hgCalRange : 0;

  List<UnitOnSet> get _reachable => [for (final u in _units ?? const <UnitOnSet>[]) if (u.reachable) u];

  /// The face asked of every unit not yet on it; all taken, the next face, after the sixth done.
  Future<void> _confirm() async {
    final ask = widget.askFace ?? _ask;
    final pending = [for (final u in _reachable) if (!_taken.contains(u.n)) u.n];
    setState(() {
      _busy = true;
      _trouble = null;
    });
    final answers = await Future.wait([
      for (final n in pending)
        ask(n, _face, _acc, _gyro, _hgOf(n)).then<(int, FaceAnswer?, Object?)>((a) => (n, a, null),
            onError: (Object e) => (n, null, e)),
    ]);
    if (!mounted) return;
    setState(() {
      for (final (n, a, e) in answers) {
        if (a == null) {
          _trouble = '${unitName(n)}: $e';
          _got.remove(n);
          continue;
        }
        _got[n] = a.got;
        if (a.got == FaceGot.taken) _taken.add(n);
      }
      _busy = false;
      if (_reachable.every((u) => _taken.contains(u.n))) {
        if (_face == calFaceNames.length - 1) {
          _done = true;
        } else {
          _face++;
          _taken.clear();
          _got.clear();
        }
      }
    });
    if (_done) {
      _closeAll();
      await _load();                     // each unit's calibration's time, now
    }
  }

  String unitName(int n) {
    for (final u in _units ?? const <UnitOnSet>[]) {
      if (u.n == n) return u.name;
    }
    return '$n';
  }

  String _note(AppLocalizations t, int n) => switch (_got[n]) {
        FaceGot.taken => t.calTaken,
        FaceGot.moved => t.calMoved,
        FaceGot.otherFace => t.calOtherFace,
        _ => '',
      };

  String _age(AppLocalizations t, UnitOnSet u, int now) {
    if (!u.reachable) return t.measUnreachable;
    if (u.calTime == 0) return t.measNoCal;
    final age = t.calAge((now - u.calTime) ~/ 86400);
    return settings.value.calibrationValid(u.calTime, now) ? age : '$age · ${t.measExpired}';
  }

  @override
  Widget build(BuildContext context) {
    final t = AppLocalizations.of(context);
    final units = _units;
    final now = nowS();
    final started = _face > 0 || _taken.isNotEmpty;
    final what = _done
        ? t.calDone
        : _face == 0
            ? t.calOnto(calFaceNames[0])
            : t.calTurn(calTurnAxes[_face], calFaceNames[_face]);
    return Scaffold(
      appBar: AppBar(title: Text(t.menuCalibration)),
      body: ListView(padding: const EdgeInsets.all(12), children: [
        Wrap(spacing: 8, runSpacing: 8, children: [
          for (var i = 0; i < calFaceNames.length; i++)
            Container(
              padding: const EdgeInsets.symmetric(horizontal: 10, vertical: 4),
              color: i < _face || _done
                  ? const Color(0xFF00AA00)
                  : i == _face
                      ? Look.yellow
                      : Look.black,
              child: Text('${i + 1}. ${calFaceNames[i]}',
                  style: TextStyle(color: i <= _face || _done ? Look.black : Look.lightCyan)),
            ),
        ]),
        const SizedBox(height: 12),
        Text(what, style: const TextStyle(color: Look.yellow)),
        if (_busy) ...[
          const SizedBox(height: 8),
          Text(t.calMeasuring),
          const LinearProgressIndicator(),
        ],
        const Divider(),
        _range(t.measAcc, _acc, {16: '±16 g', 32: '±32 g'}, (v) => _acc = v, !started),
        _range(t.measGyro, _gyro, {2000: '±2000 °/s', 4000: '±4000 °/s'}, (v) => _gyro = v,
            !started),
        const Divider(),
        if (units == null && _trouble == null)
          const Center(child: CircularProgressIndicator())
        else if (units != null && units.isEmpty)
          Text(t.measNone)
        else if (units != null)
          for (final u in units)
            ListTile(
              dense: true,
              title: Text('${settings.value.places[u.n] ?? '--'}  ${u.name}',
                  style: const TextStyle(color: Look.white)),
              subtitle: Text(_age(t, u, now)),
              trailing: Text(_note(t, u.n),
                  style: TextStyle(
                      color: _got[u.n] == FaceGot.taken ? const Color(0xFF55FF55) : Look.yellow)),
            ),
        if (_trouble != null)
          Padding(
            padding: const EdgeInsets.only(top: 8),
            child: Text(_trouble!, style: TextStyle(color: Theme.of(context).colorScheme.error)),
          ),
        const SizedBox(height: 12),
        FilledButton(
          onPressed: _busy || _done || _reachable.isEmpty ? null : _confirm,
          child: Text(t.calConfirm),
        ),
      ]),
    );
  }

  Widget _range(String label, int value, Map<int, String> options, void Function(int) set,
          bool enabled) =>
      ListTile(
        dense: true,
        title: Text(label),
        trailing: DropdownButton<int>(
          value: value,
          dropdownColor: Look.blue,
          items: [
            for (final o in options.entries)
              DropdownMenuItem(
                  value: o.key, child: Text(o.value, style: const TextStyle(color: Look.yellow)))
          ],
          onChanged: enabled ? (v) => setState(() => set(v ?? value)) : null,
        ),
      );
}
