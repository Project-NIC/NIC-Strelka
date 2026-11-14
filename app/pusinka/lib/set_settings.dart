/// The set's own settings (APP.md, What it must do, 1 and 3; The menus, 6; the sketch
/// app/mockups/en/settings-c-new-units.png): a new set drawn and given to Povodok (64/19), and its
/// file for a Belka on its cable; new units found by the search (64/12) and taken with their
/// numbers (64/13); the password changed (64/17), twelve characters at least, the menus joined
/// again with it; Povodok switched off (64/6). Never while the set records.
library;

import 'dart:async';
import 'dart:io';

import 'package:material_ui/material_ui.dart';
import 'package:strelka/strelka.dart';

import 'data.dart';
import 'keys.dart';
import 'l10n/app_localizations.dart';
import 'main.dart';
import 'overview.dart';

/// A new set (APP.md, What it must do, 3): its channel chosen on the map of the band, the quietest
/// offered; drawn and kept in the phone's keystore, so that each Povodok is given the same in turn,
/// its number 0–3; given by 64/19 with the password asked again, Povodok starting again in it; and
/// `STRELKA.SET` saved for a Belka on its cable, by its number.
class NewSetScreen extends StatefulWidget {
  const NewSetScreen(
      {super.key, this.client, this.loadDiag, this.give, this.dir, this.keep = Keys.setNewSet,
      this.kept = Keys.newSet});

  final SmpClient? client;

  /// For the tests: the map of the band, the giving, the folder and the keystore without Povodok.
  final Future<SetDiag> Function()? loadDiag;
  final Future<void> Function(String password, int n, NewSet set)? give;
  final Directory? dir;
  final Future<void> Function(String json) keep;
  final Future<String?> Function() kept;

  @override
  State<NewSetScreen> createState() => _NewSetScreenState();
}

class _NewSetScreenState extends State<NewSetScreen> {
  SetDiag? _diag;
  int _channel = 0, _n = 0, _number = 1;
  int? _quietest;
  NewSet? _set;
  final _label = TextEditingController(), _password = TextEditingController();
  String? _trouble, _note;
  bool _busy = false;

  @override
  void initState() {
    super.initState();
    unawaited(_load());
  }

  @override
  void dispose() {
    _label.dispose();
    _password.dispose();
    super.dispose();
  }

  Future<void> _load() async {
    try {
      final json = await widget.kept();
      final d = await (widget.loadDiag ?? () => readDiag(widget.client!))();
      if (!mounted) return;
      setState(() {
        _diag = d;
        _quietest = quietestChannel(d);
        _channel = _quietest ?? (d.ch < setChannels ? d.ch : 0);
        if (json != null) {
          _set = NewSet.fromJson(json);
          _channel = _set!.channel;
        }
      });
    } on Exception catch (e) {
      if (mounted) setState(() => _trouble = '$e');
    }
  }

  Future<void> _draw() async {
    final label = int.tryParse(_label.text.trim()) ?? 0;
    if (label < 0 || label > labelMost) return setState(() => _trouble = t.nsLabel);
    final set = NewSet.draw(channel: _channel, label: label);
    await widget.keep(set.toJson());
    setState(() {
      _set = set;
      _trouble = _note = null;
    });
  }

  AppLocalizations get t => AppLocalizations.of(context);

  Future<void> _give() async {
    final set = _set;
    if (set == null) return;
    final ok = await showDialog<bool>(
      context: context,
      builder: (c) => AlertDialog(
        title: Text(t.nsGive),
        content: Text(t.nsAsk),
        actions: [
          TextButton(onPressed: () => Navigator.pop(c, false), child: Text(t.dataCancel)),
          FilledButton(onPressed: () => Navigator.pop(c, true), child: Text(t.nsGive)),
        ],
      ),
    );
    if (ok != true || !mounted) return;
    setState(() => _busy = true);
    try {
      await (widget.give ?? (pw, n, s) => giveSet(widget.client!, pw, n, s))(
          _password.text, _n, set);
      if (!mounted) return;
      setState(() => _note = t.nsGiven);
    } on Exception catch (e) {
      if (mounted) {
        setState(() => _trouble = e is SmpError && e.rc == Rc.access ? t.joinRefused : '$e');
      }
    } finally {
      if (mounted) setState(() => _busy = false);
    }
  }

  Future<void> _save() async {
    final set = _set;
    if (set == null) return;
    try {
      final root = widget.dir ?? await phoneDir();
      final f = File('${root.path}/set/B${'$_number'.padLeft(2, '0')}/$setFileName');
      await f.parent.create(recursive: true);
      await f.writeAsString(set.fileFor(_number));
      if (mounted) setState(() => _note = t.nsSaved(f.path));
    } on Exception catch (e) {
      if (mounted) setState(() => _trouble = '$e');
    }
  }

  String _noiseOf(int c) {
    final n = _diag?.channelNoise(c);
    final mark = c == _quietest ? ' · ${t.nsQuietest}' : '';
    return n == null ? '$c · ${t.nsNotMeasured}' : '$c · $n dBm$mark';
  }

  @override
  Widget build(BuildContext context) {
    final set = _set;
    return Scaffold(
      appBar: AppBar(title: Text(t.setNewSet)),
      body: ListView(padding: const EdgeInsets.all(12), children: [
        Text(t.nsIntro, style: const TextStyle(color: Look.yellow)),
        if (_diag == null && _trouble == null)
          const Padding(
              padding: EdgeInsets.all(16), child: Center(child: CircularProgressIndicator())),
        if (_diag != null) ...[
          ListTile(
            dense: true,
            title: Text(t.nsChannel),
            trailing: DropdownButton<int>(
              value: _channel,
              dropdownColor: Look.blue,
              items: [
                for (var c = 0; c < setChannels; c++)
                  DropdownMenuItem(
                      value: c,
                      child: Text(_noiseOf(c), style: const TextStyle(color: Look.yellow))),
              ],
              onChanged: (v) => setState(() => _channel = v ?? _channel),
            ),
          ),
          ListTile(
            dense: true,
            title: TextField(
              controller: _label,
              keyboardType: TextInputType.number,
              decoration: InputDecoration(labelText: t.nsLabel),
            ),
          ),
          FilledButton(onPressed: _busy ? null : _draw, child: Text(t.nsDraw)),
        ],
        if (set != null) ...[
          const Divider(),
          Text(t.nsDrawn(set.addressHex, set.channel), style: const TextStyle(color: Look.white)),
          ListTile(
            dense: true,
            title: Text(t.nsPovodok),
            trailing: DropdownButton<int>(
              value: _n,
              dropdownColor: Look.blue,
              items: [
                for (var n = 0; n < 4; n++)
                  DropdownMenuItem(
                      value: n, child: Text('$n', style: const TextStyle(color: Look.yellow))),
              ],
              onChanged: (v) => setState(() => _n = v ?? _n),
            ),
          ),
          ListTile(
            dense: true,
            title: TextField(
              controller: _password,
              obscureText: true,
              decoration: InputDecoration(labelText: t.nsPassword),
            ),
          ),
          FilledButton(onPressed: _busy ? null : _give, child: Text(t.nsGive)),
          const Divider(),
          ListTile(
            dense: true,
            title: Text(t.nsFileFor),
            trailing: DropdownButton<int>(
              value: _number,
              dropdownColor: Look.blue,
              items: [
                for (var n = 1; n <= 63; n++)
                  DropdownMenuItem(
                      value: n, child: Text('$n', style: const TextStyle(color: Look.yellow))),
              ],
              onChanged: (v) => setState(() => _number = v ?? _number),
            ),
          ),
          OutlinedButton(onPressed: _save, child: Text(t.nsSave)),
        ],
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

/// The search: Povodok calls on 2480 MHz, the new units answering are listed by their chips, each
/// taken with a number of 1–63 not yet in the set; the search stopped as the screen closes.
class NewUnitsScreen extends StatefulWidget {
  const NewUnitsScreen({super.key, required this.client});

  final SmpClient client;

  @override
  State<NewUnitsScreen> createState() => _NewUnitsScreenState();
}

class _NewUnitsScreenState extends State<NewUnitsScreen> {
  List<Map<Object?, Object?>> _found = const [];
  final Set<int> _numbers = {};          // taken in the set, or here
  final Map<String, int> _taken = {};    // a chip's hex, its number given
  Timer? _every;
  String? _trouble;
  bool _searching = false;

  @override
  void initState() {
    super.initState();
    unawaited(_start());
  }

  @override
  void dispose() {
    _every?.cancel();
    if (_searching) {
      unawaited(widget.client
          .write(groupStrelka, Strelka.search, body: {'run': false})
          .then((_) {}, onError: (Object _) {}));
    }
    super.dispose();
  }

  Future<void> _start() async {
    try {
      final set = await widget.client.read(groupStrelka, Strelka.set);
      for (final b in set['belkas'] as List<Object?>? ?? const []) {
        _numbers.add((b as Map<Object?, Object?>)['n'] as int);
      }
      await widget.client.write(groupStrelka, Strelka.search, body: {'run': true});
      _searching = true;
      _every = Timer.periodic(const Duration(seconds: 1), (_) => unawaited(_read()));
      await _read();
    } on Exception catch (e) {
      if (mounted) setState(() => _trouble = '$e');
    }
  }

  Future<void> _read() async {
    try {
      final r = await widget.client.read(groupStrelka, Strelka.search);
      if (mounted) {
        setState(() => _found = [
              for (final f in r['found'] as List<Object?>? ?? const []) f as Map<Object?, Object?>
            ]);
      }
    } on Exception catch (e) {
      if (mounted) setState(() => _trouble = '$e');
    }
  }

  int get _nextFree {
    for (var n = 1; n <= 63; n++) {
      if (!_numbers.contains(n)) return n;
    }
    return 0;
  }

  Future<void> _take(List<int> chip) async {
    final n = _nextFree;
    if (n == 0) return;
    try {
      await widget.client.write(groupStrelka, Strelka.take, body: {'chip': chip, 'n': n});
      setState(() {
        _numbers.add(n);
        _taken[hexBytes(chip)] = n;
      });
    } on Exception catch (e) {
      if (mounted) setState(() => _trouble = '$e');
    }
  }

  @override
  Widget build(BuildContext context) {
    final t = AppLocalizations.of(context);
    return Scaffold(
      appBar: AppBar(title: Text(t.newTitle)),
      body: ListView(padding: const EdgeInsets.all(12), children: [
        Text(t.newSearching, style: const TextStyle(color: Look.yellow)),
        const SizedBox(height: 8),
        if (_found.isEmpty && _trouble == null) Text(t.newNone),
        for (final f in _found)
          ListTile(
            dense: true,
            title: Text(hexBytes(f['chip']), style: const TextStyle(color: Look.white)),
            subtitle: Text('${f['rssi']} dBm · ${f['ago']} s'),
            trailing: switch (_taken[hexBytes(f['chip'])]) {
              final n? => Text(t.newTaken(n), style: const TextStyle(color: Color(0xFF55FF55))),
              null => FilledButton(
                  onPressed: _nextFree == 0 ? null : () => _take((f['chip'] as List).cast<int>()),
                  child: Text(t.newTake(_nextFree))),
            },
          ),
        const SizedBox(height: 8),
        Text(t.newCable, style: const TextStyle(fontSize: 12)),
        if (_trouble != null)
          Padding(
            padding: const EdgeInsets.only(top: 8),
            child: Text(_trouble!, style: TextStyle(color: Theme.of(context).colorScheme.error)),
          ),
      ]),
    );
  }
}

/// What a new password must be, or null when it will do: twelve characters at least, the two
/// alike.
String? passwordTrouble(AppLocalizations t, String fresh, String again) {
  if (fresh.length < 12) return t.pwShort;
  if (fresh != again) return t.pwMismatch;
  return null;
}

/// The password changed by 64/17, the old one proved, the new one kept in the phone's keystore for
/// [host]; then the menus joined again with it. [factory]: the unit's own password of the build,
/// shorter than twelve, asked to be changed at the first joining.
Future<void> changePasswordDialog(BuildContext context, SmpClient client,
    {String host = '', String oldPassword = '', bool factory = false}) async {
  final t = AppLocalizations.of(context);
  final old = TextEditingController(text: oldPassword),
      fresh = TextEditingController(),
      again = TextEditingController();
  String? trouble;
  final done = await showDialog<bool>(
    context: context,
    builder: (c) => StatefulBuilder(
      builder: (c, set) => AlertDialog(
        title: Text(t.pwTitle),
        content: Column(mainAxisSize: MainAxisSize.min, children: [
          if (factory) Text(t.pwFactory),
          TextField(controller: old, obscureText: true, decoration: InputDecoration(labelText: t.pwOld)),
          TextField(controller: fresh, obscureText: true, decoration: InputDecoration(labelText: t.pwNew)),
          TextField(
              controller: again, obscureText: true, decoration: InputDecoration(labelText: t.pwAgain)),
          if (trouble != null)
            Padding(
              padding: const EdgeInsets.only(top: 8),
              child: Text(trouble!, style: TextStyle(color: Theme.of(c).colorScheme.error)),
            ),
        ]),
        actions: [
          TextButton(onPressed: () => Navigator.pop(c, false), child: Text(t.dataCancel)),
          FilledButton(
            onPressed: () async {
              final why = passwordTrouble(t, fresh.text, again.text);
              if (why != null) return set(() => trouble = why);
              try {
                await client.changePassword(old.text, fresh.text);
                if (host.isNotEmpty) await Keys.setPassword(host, fresh.text);
                if (c.mounted) Navigator.pop(c, true);
              } on Exception catch (e) {
                set(() => trouble = e is SmpError && e.rc == Rc.access ? t.joinRefused : '$e');
              }
            },
            child: Text(t.pwChange),
          ),
        ],
      ),
    ),
  );
  old.dispose();
  fresh.dispose();
  again.dispose();
  if (done == true && context.mounted) {
    ScaffoldMessenger.of(context).showSnackBar(SnackBar(content: Text(t.pwChanged)));
    Navigator.of(context).popUntil((r) => r.isFirst);   // joined again with the new one
  }
}

/// Povodok switched off by 64/6, asked first: recording, it stops the set first.
Future<void> povodokOffDialog(BuildContext context, SmpClient client) async {
  final t = AppLocalizations.of(context);
  final ok = await showDialog<bool>(
    context: context,
    builder: (c) => AlertDialog(
      title: Text(t.offTitle),
      content: Text(t.offAsk),
      actions: [
        TextButton(onPressed: () => Navigator.pop(c, false), child: Text(t.dataCancel)),
        FilledButton(onPressed: () => Navigator.pop(c, true), child: Text(t.offTitle)),
      ],
    ),
  );
  if (ok != true || !context.mounted) return;
  try {
    await client.write(groupStrelka, Strelka.off);
  } on Exception {
    // it went off before it answered
  }
  if (context.mounted) Navigator.of(context).popUntil((r) => r.isFirst);
}
