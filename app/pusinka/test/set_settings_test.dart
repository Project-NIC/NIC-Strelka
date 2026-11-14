import 'dart:io';

import 'package:cupertino_ui/cupertino_ui.dart' show GlobalCupertinoLocalizations;
import 'package:flutter_localizations/flutter_localizations.dart' show GlobalWidgetsLocalizations;
import 'package:flutter_test/flutter_test.dart';
import 'package:material_ui/material_ui.dart';
import 'package:pusinka/l10n/app_localizations.dart';
import 'package:pusinka/set_settings.dart';
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

/// The new password: twelve characters at least, typed twice alike. A new set: the quietest
/// channel offered, drawn and kept, given to Povodok with the password again, its file saved.
void main() {
  test('a new password too short, two unlike, or good', () {
    final t = lookupAppLocalizations(const Locale('en'));
    expect(passwordTrouble(t, 'strelka2', 'strelka2'), 'At least 12 characters.');
    expect(passwordTrouble(t, 'twelve chars at least', 'twelve chars at last'),
        'The two do not match.');
    expect(passwordTrouble(t, 'twelve chars at least', 'twelve chars at least'), isNull);
  });

  testWidgets('a new set: the quietest channel, drawn and kept, given, its file saved',
      (tester) async {
    List<int> band(Map<int, int> at) =>
        [for (var c = 0; c < 40; c++) at[c] ?? SetDiag.notMeasured];
    final dir = Directory.systemTemp.createTempSync('pusinka_set');
    String? kept;
    final given = <(String, int, NewSet)>[];
    await tester.pumpWidget(localized(NewSetScreen(
      loadDiag: () async => SetDiag(const [], {
        'ch': 5,
        'noise': [band({5: -70, 9: -99, 12: -90})],
      }),
      give: (pw, n, set) async => given.add((pw, n, set)),
      dir: dir,
      keep: (json) async => kept = json,
      kept: () async => null,
    )));
    await tester.pumpAndSettle();
    expect(find.text('9 · -99 dBm · the quietest'), findsOneWidget);
    await tester.tap(find.text('Draw a new set'));
    await tester.pumpAndSettle();
    final set = NewSet.fromJson(kept!);
    expect(set.channel, 9);
    expect(find.text('The set: ${set.addressHex}, channel 9'), findsOneWidget);
    await tester.enterText(find.widgetWithText(TextField, "Povodok's password, again"), 'pw-again');
    await tester.ensureVisible(find.widgetWithText(FilledButton, 'Give it to this Povodok'));
    await tester.tap(find.widgetWithText(FilledButton, 'Give it to this Povodok'));
    await tester.pumpAndSettle();
    await tester.tap(find.widgetWithText(FilledButton, 'Give it to this Povodok').last);
    await tester.pumpAndSettle();
    expect(given.single.$1, 'pw-again');
    expect([given.single.$2, given.single.$3.address], [0, set.address]);
    expect(find.textContaining('Povodok takes the new set'), findsOneWidget);
    await tester.ensureVisible(find.text('Save'));
    await tester.runAsync(() async {
      await tester.tap(find.text('Save'));
      await Future<void>.delayed(const Duration(milliseconds: 200));
    });
    await tester.pumpAndSettle();
    expect(File('${dir.path}/set/B01/STRELKA.SET').readAsStringSync(), set.fileFor(1));
    dir.deleteSync(recursive: true);
  });
}
