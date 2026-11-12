import 'dart:io';

import 'package:flutter_test/flutter_test.dart';
import 'package:material_ui/material_ui.dart';
import 'package:pusinka/main.dart';
import 'package:strelka/strelka.dart';

void main() {
  testWidgets('the first screen asks the address and the password', (tester) async {
    await tester.pumpWidget(const PusinkaApp());
    expect(find.text("Povodok's address"), findsOneWidget);
    expect(find.text('Password'), findsOneWidget);
    expect(find.widgetWithText(FilledButton, 'Join'), findsOneWidget);
  });

  testWidgets('no Povodok at the address is said, not thrown', (tester) async {
    await tester.pumpWidget(const PusinkaApp());
    await tester.enterText(find.byType(TextField).first, '127.0.0.1');
    // real sockets: the refusal comes from the address itself, not from a timeout of fake time
    await tester.runAsync(() async {
      await tester.tap(find.widgetWithText(FilledButton, 'Join'));
      await Future<void>.delayed(const Duration(seconds: 1));
    });
    await tester.pump();
    expect(find.textContaining('does not answer:'), findsOneWidget);
  });

  testWidgets("joined with the factory password: Povodok's key kept, a new password asked at once",
      (tester) async {
    late Process sim;
    await tester.runAsync(() async {
      sim = await Process.start('python3', [
        '${Directory.current.path}/../../tools/povodok_sim.py', '--host', '127.0.0.1',
        '--port', '$smpPort', '--belkas', '1'
      ]);
      for (var i = 0; i < 50; i++) {
        try {
          (await Socket.connect('127.0.0.1', smpPort)).destroy();
          break;
        } on SocketException {
          await Future<void>.delayed(const Duration(milliseconds: 100));
        }
      }
    });
    await tester.pumpWidget(const PusinkaApp());
    await tester.enterText(find.byType(TextField).first, '127.0.0.1');
    await tester.enterText(find.byType(TextField).last, 'strelka1');
    await tester.runAsync(() async {
      await tester.tap(find.widgetWithText(FilledButton, 'Join'));
      await Future<void>.delayed(const Duration(seconds: 1));
    });
    await tester.pump();
    await tester.pump();
    expect(SmpClient.pins['127.0.0.1'], isNotNull);         // pinned at the first joining
    expect(find.text('The factory password: give a new one, 12 characters at least.'),
        findsOneWidget);
    sim.kill();
  });
}

