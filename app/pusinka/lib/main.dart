/// Pušinka at its base (APP.md): the first screen joins one Povodok by its address and the
/// password, through the lock; the second shows what Povodok says of itself, starts and stops
/// the record, and lists the Belkas as the live view brings them. The menus of APP.md follow
/// from here, one screen a task. Analysed and tested as widgets; run on no device yet.
library;

import 'dart:async';
import 'dart:io';
import 'dart:typed_data';

import 'package:flutter/material.dart';
import 'package:strelka/strelka.dart';

void main() => runApp(const PusinkaApp());

class PusinkaApp extends StatelessWidget {
  const PusinkaApp({super.key});

  @override
  Widget build(BuildContext context) => MaterialApp(
        title: 'Pušinka',
        theme: ThemeData(colorSchemeSeed: Colors.teal),
        home: const JoinScreen(),
      );
}

/// The first screen: Povodok's address — 192.168.7.1 on its cable (PHONE.md) — and the
/// password; the lock passed, the set is shown.
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

  Future<void> _join() async {
    setState(() {
      _joining = true;
      _trouble = null;
    });
    try {
      final client = await SmpClient.connect(_host.text.trim(), password: _password.text);
      final info = await client.read(groupStrelka, Strelka.info);
      if (!mounted) {
        client.close();
        return;
      }
      await Navigator.of(context).push(MaterialPageRoute<void>(
        builder: (_) => SetScreen(client: client, info: info),
      ));
    } on SmpError catch (e) {
      setState(() => _trouble = e.rc == Rc.access ? 'Povodok refused the password.' : '$e');
    } on SocketException catch (e) {
      setState(() => _trouble = 'Povodok does not answer: ${e.message}');
    } on TimeoutException {
      setState(() => _trouble = 'Povodok does not answer in time.');
    } finally {
      if (mounted) setState(() => _joining = false);
    }
  }

  @override
  Widget build(BuildContext context) => Scaffold(
        appBar: AppBar(title: const Text('Pušinka')),
        body: Padding(
          padding: const EdgeInsets.all(24),
          child: Column(
            crossAxisAlignment: CrossAxisAlignment.stretch,
            children: [
              TextField(
                controller: _host,
                decoration: const InputDecoration(labelText: "Povodok's address"),
                keyboardType: TextInputType.url,
              ),
              const SizedBox(height: 12),
              TextField(
                controller: _password,
                decoration: const InputDecoration(labelText: 'Password'),
                obscureText: true,
                onSubmitted: (_) => _joining ? null : _join(),
              ),
              const SizedBox(height: 24),
              FilledButton(
                onPressed: _joining ? null : _join,
                child: Text(_joining ? 'Joining…' : 'Join'),
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

/// The set: Povodok as 64/0 describes it, the record's start and stop, and the Belkas as the
/// live view's messages bring them, a line a Belka.
class SetScreen extends StatefulWidget {
  const SetScreen({super.key, required this.client, required this.info});

  final SmpClient client;
  final Map<String, Object?> info;

  @override
  State<SetScreen> createState() => _SetScreenState();
}

class _SetScreenState extends State<SetScreen> {
  LiveReceiver? _receiver;
  StreamSubscription<LiveMessage>? _live;
  LiveMessage? _latest;
  String? _trouble;

  @override
  void initState() {
    super.initState();
    unawaited(_listen());
  }

  @override
  void dispose() {
    unawaited(_live?.cancel());
    _receiver?.close();
    widget.client.close();
    super.dispose();
  }

  Future<void> _listen() async {
    try {
      final receiver = await LiveReceiver.bind();
      _receiver = receiver;
      _live = receiver.messages.listen((m) => setState(() => _latest = m));
    } on SocketException catch (e) {
      setState(() => _trouble = 'The live view cannot listen on UDP $livePort: ${e.message}');
    }
  }

  Future<void> _record(bool run) async {
    try {
      await widget.client.write(groupStrelka, Strelka.record, body: {'run': run});
    } on Exception catch (e) {
      if (!mounted) return;
      ScaffoldMessenger.of(context).showSnackBar(SnackBar(content: Text('$e')));
    }
  }

  static String _hex(Object? bytes) => bytes is Uint8List
      ? bytes.map((b) => b.toRadixString(16).padLeft(2, '0')).join()
      : '?';

  @override
  Widget build(BuildContext context) {
    final info = widget.info;
    final m = _latest;
    final fw = info['fw'];
    return Scaffold(
      appBar: AppBar(title: const Text('The set')),
      body: ListView(
        children: [
          ListTile(
            title: Text('Povodok ${info['n']}'),
            subtitle: Text('chip ${_hex(info['chip'])} · firmware '
                '${fw is List<Object?> ? fw.join('.') : '?'} · state ${info['state']}'),
          ),
          Padding(
            padding: const EdgeInsets.symmetric(horizontal: 16),
            child: Row(
              children: [
                FilledButton(onPressed: () => _record(true), child: const Text('Record')),
                const SizedBox(width: 12),
                OutlinedButton(onPressed: () => _record(false), child: const Text('Stop')),
              ],
            ),
          ),
          if (_trouble != null)
            ListTile(
              title: Text(_trouble!, style: TextStyle(color: Theme.of(context).colorScheme.error)),
            ),
          if (m == null)
            const ListTile(title: Text('No live view yet'))
          else ...[
            ListTile(
              title: Text('Session ${m.session}, period ${m.period}'),
              subtitle: Text('set state 0x${m.state.toRadixString(16)} · '
                  '${m.povodoks} Povodok(s) · live ${m.code.name}, item ${m.item}'),
            ),
            for (final b in m.belkas)
              ListTile(
                leading: CircleAvatar(child: Text('${b.n}')),
                title: Text(b.quats.isNotEmpty
                    ? '${b.quats.first}'
                    : b.raws.isNotEmpty
                        ? '${b.raws.first}'
                        : 'no steps'),
                subtitle: Text('item ${b.item} · state 0x${b.state.toRadixString(16)} · '
                    'heard by Povodok ${b.by}'),
              ),
          ],
        ],
      ),
    );
  }
}
