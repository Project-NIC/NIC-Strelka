import 'package:flutter/material.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:pusinka/main.dart';

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
    await tester.tap(find.widgetWithText(FilledButton, 'Join'));
    await tester.pump();
    await tester.pump(const Duration(seconds: 6));
    expect(find.textContaining('does not answer'), findsOneWidget);
  });
}
