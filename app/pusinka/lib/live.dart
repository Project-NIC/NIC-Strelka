/// The live view (APP.md, What it must do, 6; the sketches app/mockups/en/measurement-b-overview,
/// -c-curves and -d-numbers): the Povodoks' messages joined by the Belka's number and the period
/// (LiveSeries); the overview, each unit by its place's code, when it was last heard, shown in the
/// curves or in the numbers by a touch; the curves, those units on one time axis over the window
/// set, a loss a gap; the numbers, each one's maximum, minimum and range in a stride over the last
/// strides set, and left against right by the template's pairs. The record stopped
/// here; out of the service mode by a touch, the live view being the work's. Saved on the phone at
/// a touch (APP.md, The live view saved): what comes from every Povodok, joined a period a message,
/// after the session's and the units' SMP frames, into a file of the data manager's.
library;

import 'dart:async';
import 'dart:io';

import 'package:material_ui/material_ui.dart';
import 'package:strelka/strelka.dart';

import 'app_settings.dart';
import 'data.dart';
import 'l10n/app_localizations.dart';
import 'main.dart';

class LiveScreen extends StatefulWidget {
  const LiveScreen(
      {super.key,
      this.client,
      this.host,
      this.accG = 16,
      this.hgG = 0,
      this.saveHead,
      this.messages,
      this.datagrams,
      this.dir});

  final SmpClient? client;      // Povodok's
  final String? host;           // its address: its messages the lead's
  final int accG;               // the session's accelerometer range
  final int hgG;                // the high-g channel's, 0 none

  /// The session's and the units' SMP frames for the file; none, nothing to save.
  final LiveSaveHead? saveHead;

  /// For the tests: the messages, or the datagrams, without a socket, and the folder.
  final Stream<LiveMessage>? messages;
  final Stream<Datagram>? datagrams;
  final Directory? dir;

  @override
  State<LiveScreen> createState() => _LiveScreenState();
}

class _LiveScreenState extends State<LiveScreen> {
  late final LiveSeries _series = LiveSeries(accG: widget.accG, hgG: widget.hgG);
  LiveReceiver? _receiver;
  StreamSubscription<Object>? _live;
  LiveSaver? _saver;
  DateTime? _savedFrom;
  String? _savedPath;
  StreamSubscription<Map<String, Object?>>? _watch;
  Timer? _paint;
  bool _lost = false;
  String? _trouble;
  final Set<int> _inNumbers = {};

  @override
  void initState() {
    super.initState();
    unawaited(_listen());
    // 64/14 once a second: Povodok keeps the live view going; no answer for the time set is the
    // connection lost (COMMANDS.md)
    _watch = widget.client
        ?.watchSet(lost: Duration(seconds: settings.value.lostS))
        .listen((_) => _lost = false, onError: (Object _) => _lost = true);
    _paint = Timer.periodic(const Duration(milliseconds: 100), (_) {
      if (mounted) setState(() {});
    });
  }

  @override
  void dispose() {
    _paint?.cancel();
    unawaited(_watch?.cancel());
    unawaited(_live?.cancel());
    _receiver?.close();
    final saver = _saver, path = _savedPath;
    if (saver != null && path != null) {
      // left saving: the working file closed and made into its C3D all the same
      unawaited(saver.close().then((_) => finishLive(File(path))).then((_) {}, onError: (_) {}));
    }
    super.dispose();
  }

  Future<void> _listen() async {
    try {
      final messages = widget.messages;
      if (messages != null) {
        _live = messages.listen(_series.add);
        return;
      }
      var datagrams = widget.datagrams;
      if (datagrams == null) {
        final receiver = await LiveReceiver.bind();
        _receiver = receiver;
        datagrams = receiver.datagrams;
      }
      _live = datagrams.listen(_take);
    } on SocketException catch (e) {
      if (mounted) setState(() => _trouble = 'UDP $livePort: ${e.message}');
    }
  }

  /// A datagram shown, and saved when the user said so; what is not a message dropped.
  void _take(Datagram d) {
    final LiveMessage m;
    try {
      m = parseLive(d.data);
    } on FormatException {
      return;
    }
    _series.add(m);
    _saver?.add(d.data, lead: widget.host == null || d.address.address == widget.host);
  }

  /// Saving begun, or ended, by a touch.
  Future<void> _save() async {
    final saver = _saver;
    if (saver != null) {
      setState(() => _saver = null);
      try {
        await saver.close();
        final c3d = await finishLive(File(_savedPath!));      // the working file into the C3D
        if (mounted) {
          ScaffoldMessenger.of(context)
              .showSnackBar(SnackBar(content: Text(c3d.path.split('/').last)));
        }
      } on Object catch (e) {
        if (mounted) setState(() => _trouble = '$e');
      }
      return;
    }
    try {
      final dir = widget.dir ?? await phoneDir();
      await dir.create(recursive: true);
      final now = DateTime.now();
      final s = widget.saveHead!.session['s'] as int? ?? 0;
      final path = '${dir.path}/${liveName(s, now)}';
      final next = await LiveSaver.create(path, widget.saveHead!);
      if (mounted) {
        setState(() {
          _saver = next;
          _savedFrom = now;
          _savedPath = path;
        });
      } else {
        await next.close();
      }
    } on Object catch (e) {
      if (mounted) setState(() => _trouble = '$e');
    }
  }

  String _saving(AppLocalizations t) {
    final s = DateTime.now().difference(_savedFrom!).inSeconds;
    return t.liveSaving('${s ~/ 60}:${'${s % 60}'.padLeft(2, '0')}',
        (_saver!.bytes / 1e6).toStringAsFixed(1));
  }

  Future<void> _write(int cmd, [Map<String, Object?>? body]) async {
    try {
      await widget.client?.write(groupStrelka, cmd, body: body);
      if (cmd == Strelka.record) recording.value = false;
    } on Exception catch (e) {
      if (mounted) setState(() => _trouble = '$e');
    }
  }

  String _code(int n) => '${settings.value.places[n] ?? '--'} $n';

  List<UnitSeries> get _units => _series.units.values.toList()..sort((a, b) => a.n - b.n);

  @override
  Widget build(BuildContext context) {
    final t = AppLocalizations.of(context);
    return DefaultTabController(
      length: 3,
      child: Scaffold(
        appBar: AppBar(
          title: Text(t.measLive),
          actions: [
            if (widget.saveHead != null)
              TextButton(
                  onPressed: _save,
                  child: Text(_saver == null ? t.liveSave : t.liveSaveStop,
                      style: TextStyle(color: _saver == null ? null : const Color(0xFFFF5555)))),
            if (recording.value)
              TextButton(onPressed: () => _write(Strelka.record, {'run': false}),
                  child: Text(t.actStop)),
            if (widget.client != null)
              TextButton(onPressed: () => _write(Strelka.work), child: Text(t.setToWork)),
          ],
          bottom: TabBar(tabs: [
            Tab(text: t.menuOverview),
            Tab(text: t.liveCurves),
            Tab(text: t.liveNumbers),
          ]),
        ),
        body: TabBarView(children: [_overview(t), _curves(t), _numbers(t)]),
      ),
    );
  }

  Widget _overview(AppLocalizations t) => ListView(children: [
        if (_saver != null)
          ListTile(title: Text(_saving(t), style: const TextStyle(color: Look.yellow))),
        if (_lost)
          ListTile(
              title: Text(t.setLost, style: TextStyle(color: Theme.of(context).colorScheme.error))),
        if (_trouble != null)
          ListTile(
              title: Text(_trouble!, style: TextStyle(color: Theme.of(context).colorScheme.error))),
        if (_series.units.isEmpty) ListTile(title: Text(t.setNoLive)),
        for (final u in _units)
          ListTile(
            dense: true,
            title: Text(_code(u.n), style: const TextStyle(color: Look.white, fontSize: 18)),
            subtitle: Text(t.diagHeard(((_series.period - u.lastPeriod) / periodsAS).toStringAsFixed(1))),
            trailing: OutlinedButton(
              onPressed: () => setState(() =>
                  _inNumbers.contains(u.n) ? _inNumbers.remove(u.n) : _inNumbers.add(u.n)),
              child: Text(_inNumbers.contains(u.n) ? t.liveNumbers : t.liveCurves,
                  style: TextStyle(
                      color: _inNumbers.contains(u.n) ? Look.lightCyan : const Color(0xFFFF55FF))),
            ),
          ),
      ]);

  Widget _curves(AppLocalizations t) {
    final lanes = [for (final u in _units) if (!_inNumbers.contains(u.n)) u];
    final quats = lanes.isNotEmpty && lanes.first.quaternions;
    return Column(crossAxisAlignment: CrossAxisAlignment.start, children: [
      Padding(
        padding: const EdgeInsets.all(8),
        child: Text('${quats ? t.liveTilt : t.liveAccel} · ${settings.value.curvesS} s',
            style: const TextStyle(color: Look.yellow)),
      ),
      Expanded(
        child: CustomPaint(
          size: Size.infinite,
          painter: CurvesPainter(
              lanes: lanes,
              labels: [for (final u in lanes) _code(u.n)],
              windowS: settings.value.curvesS),
        ),
      ),
    ]);
  }

  Widget _numbers(AppLocalizations t) {
    final strides = settings.value.numbersStrides;
    final numbers = {
      for (final u in _units)
        if (_inNumbers.contains(u.n)) u.n: strideNumbers(u.values, u.rateHz, strides)
    };
    final digits = _units.isNotEmpty && _units.first.quaternions ? 1 : 2;
    String f(double v) => v.toStringAsFixed(digits);
    // the template's pairs of left and right, both shown in the numbers
    final byCode = {for (final n in numbers.keys) settings.value.places[n]: n};
    final pairs = [
      for (final (l, r) in placeTemplate(settings.value.template).pairs)
        if (byCode.containsKey(l) && byCode.containsKey(r)) (l, r, byCode[l]!, byCode[r]!)
    ];
    const head = TextStyle(color: Look.yellow);
    return ListView(padding: const EdgeInsets.all(8), children: [
      Text(t.liveLastStrides(strides), style: head),
      Table(children: [
        TableRow(children: [
          const Text('', style: head),
          const Text('max', style: head),
          const Text('min', style: head),
          Text(t.liveRange, style: head),
          Text(t.liveStrides, style: head),
        ]),
        for (final e in numbers.entries)
          TableRow(children: [
            Text(_code(e.key), style: const TextStyle(color: Look.white)),
            ...switch (e.value) {
              null => [Text(t.liveNoRhythm), const Text(''), const Text(''), const Text('')],
              final s => [
                  Text(f(s.max)),
                  Text(f(s.min)),
                  Text(f(s.range)),
                  Text('${s.strides} · ${s.periodS.toStringAsFixed(2)} s'),
                ],
            },
          ]),
      ]),
      if (pairs.isNotEmpty) ...[
        const Divider(),
        Text(t.liveLeftRight, style: head),
        Table(children: [
          for (final (l, r, nl, nr) in pairs)
            TableRow(children: [
              Text('$l / $r', style: const TextStyle(color: Look.white)),
              Text(numbers[nl] == null ? '' : f(numbers[nl]!.range)),
              Text(numbers[nr] == null ? '' : f(numbers[nr]!.range)),
              Text(numbers[nl] == null || numbers[nr] == null
                  ? ''
                  : '${t.liveDifference} '
                      '${leftAgainstRight(numbers[nl]!.range, numbers[nr]!.range).toStringAsFixed(0)} %'),
            ]),
        ]),
      ],
    ]);
  }
}

/// The units' curves, a lane each on one time axis over the window, each lane scaled to its own
/// values; a loss breaks the line.
class CurvesPainter extends CustomPainter {
  CurvesPainter({required this.lanes, required this.labels, required this.windowS});

  final List<UnitSeries> lanes;
  final List<String> labels;
  final int windowS;

  @override
  void paint(Canvas canvas, Size size) {
    if (lanes.isEmpty) return;
    final h = size.height / lanes.length;
    final line = Paint()
      ..color = const Color(0xFFFF55FF)
      ..strokeWidth = 2
      ..style = PaintingStyle.stroke;
    for (var i = 0; i < lanes.length; i++) {
      final u = lanes[i];
      final n = windowS * u.rateHz;
      final v = u.values.length > n ? u.values.sublist(u.values.length - n) : u.values;
      final finite = v.where((e) => !e.isNaN);
      if (finite.isEmpty) continue;
      var lo = finite.reduce((a, b) => a < b ? a : b), hi = finite.reduce((a, b) => a > b ? a : b);
      if (hi - lo < 1e-3) {
        lo -= 0.5;
        hi += 0.5;
      }
      final top = i * h, x0 = size.width - v.length * size.width / n;
      final path = Path();
      var pen = false;
      for (var k = 0; k < v.length; k++) {
        if (v[k].isNaN) {
          pen = false;
          continue;
        }
        final x = x0 + k * size.width / n;
        final y = top + h * 0.9 - (v[k] - lo) / (hi - lo) * h * 0.8;
        pen ? path.lineTo(x, y) : path.moveTo(x, y);
        pen = true;
      }
      canvas.drawPath(path, line);
      final label = TextPainter(
          text: TextSpan(text: labels[i], style: const TextStyle(color: Look.white, fontSize: 16)),
          textDirection: TextDirection.ltr)
        ..layout();
      label.paint(canvas, Offset(4, top + 2));
    }
  }

  @override
  bool shouldRepaint(CurvesPainter old) => true;
}
