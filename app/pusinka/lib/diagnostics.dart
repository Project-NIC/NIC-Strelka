/// The diagnostics (APP.md, The menus, 5; What it must do, 2; the sketch
/// app/mockups/en/diagnostics.png): read from Povodok by 64/23 once a second, in the service mode
/// and at work, the live view on or off — the band, each channel's noise as the Povodoks and the
/// units measured it, the set's channel and its spare, each Povodok's margin; each unit's signal at
/// Povodok and when it was last heard, its SYNCs' signal and those missed, its clock, its steps out
/// of range, its cell and card, as its items last said.
library;

import 'dart:async';

import 'package:material_ui/material_ui.dart';
import 'package:strelka/strelka.dart';

import 'app_settings.dart';
import 'l10n/app_localizations.dart';
import 'main.dart';

class DiagnosticsScreen extends StatefulWidget {
  const DiagnosticsScreen({super.key, this.client, this.read});

  final SmpClient? client;      // Povodok's

  /// For the tests: a reading without a set.
  final Future<SetDiag> Function()? read;

  @override
  State<DiagnosticsScreen> createState() => _DiagnosticsScreenState();
}

class _DiagnosticsScreenState extends State<DiagnosticsScreen> {
  SetDiag? _diag;
  int? _moveTo;                          // the channel chosen to move to; null the quietest
  String? _trouble;
  Timer? _every;
  bool _reading = false;

  @override
  void initState() {
    super.initState();
    unawaited(_read());
    _every = Timer.periodic(const Duration(seconds: 1), (_) => unawaited(_read()));
  }

  @override
  void dispose() {
    _every?.cancel();
    super.dispose();
  }

  Future<void> _read() async {
    if (_reading) return;
    _reading = true;
    try {
      final d = await (widget.read ?? () => readDiag(widget.client!))();
      if (mounted) {
        setState(() {
          _diag = d;
          _trouble = null;
        });
      }
    } on Exception catch (e) {
      if (mounted) setState(() => _trouble = '$e');
    } finally {
      _reading = false;
    }
  }

  /// What a unit's items said, the unknown left out.
  String _unit(AppLocalizations t, UnitDiag u) => [
        if (u.sync != null) t.diagSync(u.sync!, u.missed ?? 0),
        if (u.offset != null) t.diagClock(u.offset!),
        if (u.out != null && u.out!.length == 2) t.diagOut(u.out![0], u.out![1]),
        if (u.cell != null) t.diagCell(u.cell!),
        if (u.mv != null) '${u.mv} mV',
        if (u.temp != null) '${u.temp} °C',
        if (u.free != null) t.diagCard(u.free!, u.dropped ?? 0),
        if (u.ch != null && u.noise != null) 'ch ${u.ch}: ${u.noise} dBm',
      ].join(' · ');

  /// The set moved by hand, 64/24, at rest alone: the quietest offered, another chosen.
  Widget _move(AppLocalizations t, SetDiag d) {
    final to = _moveTo ?? quietestChannel(d, except: d.ch);   // not its own
    return Row(children: [
      DropdownButton<int>(
        value: to,
        dropdownColor: Look.blue,
        items: [
          for (var c = 0; c < 39; c++)
            if (c != d.ch)
              DropdownMenuItem(
                  value: c, child: Text('$c', style: const TextStyle(color: Look.yellow))),
        ],
        onChanged: (v) => setState(() => _moveTo = v),
      ),
      const SizedBox(width: 8),
      FilledButton(
        onPressed: to == null || recording.value
            ? null
            : () async {
                try {
                  await widget.client!.write(groupStrelka, Strelka.move, body: {'ch': to});
                  setState(() => _moveTo = null);
                } on Exception catch (e) {
                  if (mounted) setState(() => _trouble = '$e');
                }
              },
        child: Text(t.diagMove(to ?? 0)),
      ),
    ]);
  }

  @override
  Widget build(BuildContext context) {
    final t = AppLocalizations.of(context);
    final d = _diag;
    return Scaffold(
      appBar: AppBar(title: Text(t.menuDiagnostics)),
      body: ListView(padding: const EdgeInsets.all(12), children: [
        Text(t.diagBand, style: const TextStyle(color: Look.yellow)),
        const SizedBox(height: 8),
        if (d == null && _trouble == null) const Center(child: CircularProgressIndicator()),
        if (d != null) ...[
          SizedBox(height: 110, child: BandMap(diag: d)),
          const SizedBox(height: 4),
          Text('${t.diagSet(d.ch)} · ${t.diagSpare(d.spare)}'),
          if (widget.client != null) _move(t, d),
          for (var k = 0; k < d.margin.length; k++)
            if (k < d.noise.length && d.noise[k].any((v) => v != SetDiag.notMeasured))
              Text(t.diagMargin(k + 1, d.margin[k] * 2)),
          const Divider(),
          Text(t.diagUnits, style: const TextStyle(color: Look.yellow)),
          for (final u in d.units)
            ListTile(
              dense: true,
              title: Text(
                  '${settings.value.places[u.n] ?? '--'}  ${u.n}'
                  '${u.rssi != null ? ' · ${u.rssi} dBm · ${t.diagHeard(u.agoS.toStringAsFixed(1))}' : ' · ${t.diagNotHeard}'}',
                  style: const TextStyle(color: Look.white)),
              subtitle: Text(_unit(t, u)),
            ),
        ],
        if (_trouble != null)
          Padding(
            padding: const EdgeInsets.only(top: 8),
            child: Text(_trouble!, style: TextStyle(color: Theme.of(context).colorScheme.error)),
          ),
      ]),
    );
  }
}

/// The 40 channels as bars, the louder the taller, −110 to −40 dBm: the set's channel green, its
/// spare yellow, one not measured yet a line.
class BandMap extends StatelessWidget {
  const BandMap({super.key, required this.diag});

  final SetDiag diag;

  @override
  Widget build(BuildContext context) => LayoutBuilder(
        builder: (context, box) => Row(crossAxisAlignment: CrossAxisAlignment.end, children: [
          for (var c = 0; c < 40; c++)
            Expanded(
              child: Container(
                margin: const EdgeInsets.symmetric(horizontal: 1),
                height: switch (diag.channelNoise(c)) {
                  null => 2.0,
                  final n => box.maxHeight * ((n + 110) / 70).clamp(0.05, 1.0),
                },
                color: c == diag.ch
                    ? const Color(0xFF55FF55)
                    : c == diag.spare
                        ? Look.yellow
                        : Look.lightCyan,
              ),
            ),
        ]),
      );
}
