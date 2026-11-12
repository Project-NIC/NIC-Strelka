import 'dart:async';
import 'dart:io';
import 'dart:math' as math;

import 'package:cupertino_ui/cupertino_ui.dart' show GlobalCupertinoLocalizations;
import 'package:flutter_localizations/flutter_localizations.dart' show GlobalWidgetsLocalizations;
import 'package:flutter_test/flutter_test.dart';
import 'package:material_ui/material_ui.dart';
import 'package:pusinka/app_settings.dart';
import 'package:pusinka/l10n/app_localizations.dart';
import 'package:pusinka/live.dart';
import 'package:strelka/strelka.dart';

Widget localized(Widget child) => MaterialApp(
      locale: const Locale('en'),
      localizationsDelegates: const [
        AppLocalizations.delegate,
        GlobalMaterialLocalizations.delegate,
        GlobalWidgetsLocalizations.delegate,
        GlobalCupertinoLocalizations.delegate,
      ],
      supportedLocales: AppLocalizations.supportedLocales,
      home: child,
    );

/// A unit swinging [amp] g at 1.25 Hz, raw steps at 100 Hz, ±16 g.
LiveBelka belka(int n, int period, double amp) => LiveBelka(n: n, s0: 0, by: 0, state: 4, raws: [
      for (var j = 0; j < 2; j++)
        RawStep([
          0,
          0,
          ((1 + amp * math.sin(2 * math.pi * 1.25 * (period * 2 + j) / 100)) * 2048).round(),
        ], const [0, 0, 0]),
    ]);

/// The live view from messages made up: the units by their places' codes, moved into the numbers
/// by a touch, their strides' ranges and the left fore against the right.
/// The files' work runs in real time, outside the test's clock, and takes its own while the whole
/// suite runs at once: waited for until [done] holds, five seconds at the most.
Future<void> settle(WidgetTester tester, bool Function() done) async {
  for (var i = 0; i < 100 && !done(); i++) {
    await tester.runAsync(() => Future<void>.delayed(const Duration(milliseconds: 50)));
    await tester.pump();
  }
}

void main() {
  tearDown(() => settings.value = AppSettings());

  testWidgets('the overview, the curves, the numbers and left against right', (tester) async {
    settings.value = AppSettings()..places = {1: 'LF', 2: 'RF'};
    final messages = StreamController<LiveMessage>();
    await tester.pumpWidget(localized(LiveScreen(messages: messages.stream)));
    for (var p = 0; p < 50 * 12; p++) {
      messages.add(LiveMessage(
          session: 3, period: p, state: 0, s0: 0, povodoks: 1, povodokStates: const [0],
          belkas: [belka(1, p, 0.33), belka(2, p, 0.3)]));
    }
    await tester.pump(const Duration(milliseconds: 200));
    expect(find.text('LF 1'), findsOneWidget);
    expect(find.text('RF 2'), findsOneWidget);
    await tester.tap(find.byType(OutlinedButton).at(0));     // unit 1 into the numbers
    await tester.tap(find.byType(OutlinedButton).at(1));     // and unit 2
    await tester.pump(const Duration(milliseconds: 200));
    await tester.tap(find.widgetWithText(Tab, 'Numbers'));    // the numbers' tab
    await tester.pumpAndSettle(const Duration(milliseconds: 100));
    expect(find.text('the last 10 strides'), findsOneWidget);
    expect(find.text('0.65'), findsWidgets);                 // the left fore's range, g
    expect(find.text('10 · 0.80 s'), findsNWidgets(2));      // ten strides of 0.8 s each
    expect(find.text('LF / RF'), findsOneWidget);
    expect(find.text('difference 10 %'), findsOneWidget);
    await messages.close();
    await tester.pumpWidget(const SizedBox());
  });

  testWidgets('saved at a touch: the SMP frames first, then the messages, the lead\'s head kept',
      (tester) async {
    final dir = (await tester.runAsync(() => Directory.systemTemp.createTemp('pusinka_live')))!;
    final datagrams = StreamController<Datagram>();
    final head = LiveSaveHead(
        session: {'s': 9, 'n': 10, 'acc': 16, 'gyro': 2000, 'live': LiveCode.raw100.code},
        table: tableBodies(
            'kind;horse\nnote;Praha: prase\nbelka;chip;code;place\n1;F0;LF;left fore\n'),
        units: [(1, {'n': 1, 'kind': kindBelka, 'sensor': 0x70}, <String, Object?>{})]);
    await tester.pumpWidget(localized(
        LiveScreen(host: '192.168.7.1', saveHead: head, datagrams: datagrams.stream, dir: dir)));
    await tester.tap(find.text('Save'));
    await settle(tester, () => find.text('Stop saving').evaluate().isNotEmpty);
    expect(find.text('Stop saving'), findsOneWidget);
    for (var p = 0; p < 10; p++) {
      final m = buildLive(LiveMessage(
          session: 9, period: p, state: 0, s0: LiveCode.raw100.code << 4, povodoks: 1,
          povodokStates: const [0, 0, 0, 0],
          belkas: [
            LiveBelka(n: 1, s0: LiveCode.raw100.code << 4, by: 0, state: 0, raws: [
              for (var j = 0; j < 2; j++) RawStep([0, 0, 2048], [p, j, 0]),
            ]),
          ]));
      datagrams.add(Datagram(m, InternetAddress('192.168.7.1'), 1337));
    }
    await tester.pump(const Duration(milliseconds: 200));
    await tester.tap(find.text('Stop saving'));
    await settle(tester, () {
      final left = dir.listSync().whereType<File>().toList();
      return left.length == 1 && left.first.path.endsWith('.c3d');
    });
    final files = (await tester.runAsync(() => dir.list().toList()))!.whereType<File>().toList();
    // the working file gone into its C3D, named by the date, the session and the note
    expect(files.length, 1);
    expect(files.first.path, matches(RegExp(r'/\d{4}-\d\d-\d\d_\d{4}_S9_Praha- prase_live\.c3d$')));
    final c3d = (await tester.runAsync(() => files.first.readAsBytes()))!;
    final c = readC3d(c3d);
    expect(c.rows.length, 20);                                   // 10 periods of 2 steps
    expect(c.labels.first, 'B1_AX');
    final csv = StringBuffer();
    c3dToCsv(c3d, csv);
    expect(csv.toString().split('\n')[1], startsWith('0,0.000000,0.000000,0.000000,0.999424,'));
    await datagrams.close();
    await tester.pumpWidget(const SizedBox());
    await tester.runAsync(() => dir.delete(recursive: true));
  });
}
