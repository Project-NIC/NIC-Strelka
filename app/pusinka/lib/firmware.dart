/// The firmware (APP.md, What it must do, 13; The menus, 6; the sketch
/// app/mockups/en/settings-e-firmware.png): the signed images fetched from the public repository's
/// folder and kept on the phone, checked; the units of the set with the version each runs, a bar
/// and its state; all updated by SMP through Povodok one unit at a time, the Belkas first and
/// Povodok last, which then starts again and is joined again; a unit off takes it when it is on.
/// The screens have run on no device.
library;

import 'dart:async';
import 'dart:io';
import 'dart:typed_data';

import 'package:material_ui/material_ui.dart';
import 'package:strelka/strelka.dart';

import 'data.dart';
import 'l10n/app_localizations.dart';
import 'main.dart';

/// A unit in the table: Povodok, n null, or a Belka by its number; its 64/0 and its update's state.
class FirmwareRow {
  FirmwareRow(this.n, this.info, {this.reachable = true});

  final int? n;
  final Map<String, Object?> info;
  final bool reachable;
  String state = '';
  double? progress;
}

enum RowState { waiting, taking, done, current, off, failed }

class FirmwareScreen extends StatefulWidget {
  const FirmwareScreen(
      {super.key,
      this.client,
      this.host = '',
      this.password = '',
      this.dir,
      this.source,
      this.loadRows,
      this.update});

  final SmpClient? client;      // Povodok's
  final String host, password;

  /// For the tests: the phone's folder, the folder fetched from, the units, the update.
  final Directory? dir;
  final Uri? source;
  final Future<List<FirmwareRow>> Function()? loadRows;
  final Future<void> Function(int? n, Uint8List image, void Function(int, int) progress)? update;

  @override
  State<FirmwareScreen> createState() => _FirmwareScreenState();
}

class _FirmwareScreenState extends State<FirmwareScreen> {
  FirmwareSet? _fw;
  List<FirmwareRow>? _rows;
  final Map<FirmwareRow, RowState> _state = {};
  bool _busy = false;
  String? _note, _trouble;

  @override
  void initState() {
    super.initState();
    unawaited(_load());
  }

  Future<Directory> _dir() async =>
      widget.dir ?? Directory('${(await phoneDir()).path}/firmware');

  Future<void> _load() async {
    try {
      final fw = await keptFirmware(await _dir());
      final rows = await (widget.loadRows ?? _readRows)();
      if (!mounted) return;
      setState(() {
        _fw = fw;
        _rows = rows;
        _mark();
      });
    } on Exception catch (e) {
      if (mounted) setState(() => _trouble = '$e');
    }
  }

  /// The units: the Belkas of the set by their ports, then Povodok itself.
  Future<List<FirmwareRow>> _readRows() async {
    final c = widget.client!;
    final set = await c.read(groupStrelka, Strelka.set);
    final belkas = [
      for (final b in set['belkas'] as List<Object?>? ?? const [])
        (b as Map<Object?, Object?>)['n'] as int
    ]..sort();
    final units = await unitsOnSet(widget.host, widget.password, belkas);
    return [
      for (final u in units) FirmwareRow(u.n, u.info, reachable: u.reachable),
      FirmwareRow(null, await c.read(groupStrelka, Strelka.info)),
    ];
  }

  void _mark() {
    for (final r in _rows ?? const <FirmwareRow>[]) {
      _state[r] = !r.reachable
          ? RowState.off
          : _fw != null && fwOf(r.info) == _fw!.version
              ? RowState.current
              : _state[r] == RowState.done
                  ? RowState.done
                  : RowState.waiting;
    }
  }

  Future<void> _fetch() async {
    final t = AppLocalizations.of(context);
    setState(() {
      _busy = true;
      _trouble = _note = null;
    });
    try {
      final fw = await fetchFirmware(await _dir(), source: widget.source);
      if (!mounted) return;
      setState(() {
        if (fw == null) {
          _note = t.fwNotPublished;
        } else {
          _fw = fw;
          _note = t.fwFetched(fw.version);
        }
        _mark();
      });
    } on Exception catch (e) {
      if (mounted) setState(() => _trouble = '$e');
    } finally {
      if (mounted) setState(() => _busy = false);
    }
  }

  Future<void> _give(FirmwareRow r, Uint8List image, void Function(int, int) progress) async {
    if (widget.update != null) return widget.update!(r.n, image, progress);
    final c = await SmpClient.connect(widget.host,
        port: smpPort + (r.n ?? 0), password: widget.password);
    try {
      await updateUnit(c, image, onProgress: progress);
    } finally {
      c.close();
    }
  }

  /// Every unit not on the version, one at a time: the Belkas, then Povodok.
  Future<void> _updateAll() async {
    final t = AppLocalizations.of(context);
    final fw = _fw;
    if (fw == null) return;
    setState(() {
      _busy = true;
      _trouble = _note = null;
    });
    for (final r in _rows ?? const <FirmwareRow>[]) {
      if (_state[r] != RowState.waiting && _state[r] != RowState.failed) continue;
      final image = imageFor(fw, r.info);
      if (image == null) continue;
      setState(() => _state[r] = RowState.taking);
      try {
        await _give(r, image, (done, total) {
          if (mounted) setState(() => r.progress = done / total);
        });
        if (mounted) setState(() => _state[r] = RowState.done);
      } on Exception catch (e) {
        if (mounted) {
          setState(() {
            _state[r] = RowState.failed;
            _trouble = '${_name(r)}: $e';
          });
        }
      }
    }
    if (!mounted) return;
    setState(() {
      _busy = false;
      if (_rows!.any((r) => r.n == null && _state[r] == RowState.done)) _note = t.fwPovodokAgain;
    });
  }

  String _name(FirmwareRow r) => r.n == null
      ? 'P${(r.info['n'] as int? ?? 0) + 1}'
      : '${kindLetter(r.info['kind'] as int? ?? kindBelka)}${r.n}';

  String _stateText(AppLocalizations t, RowState s) => switch (s) {
        RowState.waiting => t.fwWaiting,
        RowState.taking => t.fwTaking,
        RowState.done => t.fwDone,
        RowState.current => t.fwCurrent,
        RowState.off => t.fwOff,
        RowState.failed => t.fwFailed,
      };

  @override
  Widget build(BuildContext context) {
    final t = AppLocalizations.of(context);
    final fw = _fw;
    final rows = _rows;
    final waiting = _state.values.any((s) => s == RowState.waiting || s == RowState.failed);
    return Scaffold(
      appBar: AppBar(title: Text(t.fwTitle)),
      body: ListView(padding: const EdgeInsets.all(12), children: [
        Text(fw == null ? t.fwNone : t.fwReady(fw.version),
            style: const TextStyle(color: Look.yellow)),
        const SizedBox(height: 8),
        Wrap(spacing: 8, children: [
          OutlinedButton(onPressed: _busy ? null : _fetch, child: Text(t.fwFetch)),
          FilledButton(
              onPressed: _busy || fw == null || !waiting ? null : _updateAll,
              child: Text(t.fwUpdateAll)),
        ]),
        const SizedBox(height: 8),
        if (rows == null && _trouble == null)
          const Center(child: CircularProgressIndicator())
        else
          for (final r in rows ?? const <FirmwareRow>[])
            ListTile(
              dense: true,
              leading: Text(_name(r), style: const TextStyle(color: Look.yellow)),
              title: Text(fwOf(r.info), style: const TextStyle(color: Look.white)),
              subtitle: _state[r] == RowState.taking
                  ? LinearProgressIndicator(value: r.progress)
                  : null,
              trailing: Text(_stateText(t, _state[r] ?? RowState.waiting)),
            ),
        const SizedBox(height: 8),
        Text(t.fwHow, style: const TextStyle(fontSize: 12)),
        if (_note != null)
          Padding(
            padding: const EdgeInsets.only(top: 8),
            child: Text(_note!, style: const TextStyle(color: Color(0xFF55FF55))),
          ),
        if (_trouble != null)
          Padding(
            padding: const EdgeInsets.only(top: 8),
            child: Text(_trouble!, style: TextStyle(color: Theme.of(context).colorScheme.error)),
          ),
      ]),
    );
  }
}
