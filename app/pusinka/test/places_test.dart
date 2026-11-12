import 'package:cupertino_ui/cupertino_ui.dart' show GlobalCupertinoLocalizations;
import 'package:flutter_localizations/flutter_localizations.dart' show GlobalWidgetsLocalizations;
import 'package:flutter_test/flutter_test.dart';
import 'package:material_ui/material_ui.dart';
import 'package:pusinka/app_settings.dart';
import 'package:pusinka/l10n/app_localizations.dart';
import 'package:pusinka/places.dart';

Widget localized(Widget child, {Locale locale = const Locale('en')}) => MaterialApp(
      locale: locale,
      localizationsDelegates: const [
        AppLocalizations.delegate,
        GlobalMaterialLocalizations.delegate,
        GlobalWidgetsLocalizations.delegate,
        GlobalCupertinoLocalizations.delegate,
      ],
      supportedLocales: AppLocalizations.supportedLocales,
      home: child,
    );

/// The places given by a touch from the template, a place once; the set's table from them.
void main() {
  tearDown(() => settings.value = AppSettings());

  testWidgets('a unit given its place, the same place taken from another; the table', (tester) async {
    await tester.pumpWidget(localized(PlacesScreen(loadUnits: () async => [1, 2, 3])));
    await tester.pump();
    Future<void> give(int row, String item) async {
      await tester.tap(find.byType(DropdownButton<String>).at(row + 1));
      await tester.pumpAndSettle();
      await tester.tap(find.text(item).last);
      await tester.pumpAndSettle();
    }

    await give(0, 'LF  left fore');
    expect(settings.value.places, {1: 'LF'});
    await give(1, 'LF  left fore');                          // unit 2 takes it from unit 1
    expect(settings.value.places, {2: 'LF'});
    await give(0, 'RF  right fore');
    expect(settings.value.places, {1: 'RF', 2: 'LF'});
    expect(tableOf(settings.value, {1: 0x1101, 2: 0x1202}, kinds: {2: 2}),
        'kind;horse\nbelka;chip;code;place;kind\n1;0000000000001101;RF;right fore;B\n'
        '2;0000000000001202;LF;left fore;Z\n');
  });

  testWidgets('a Czech user: the English code with the Czech name; the table English', (tester) async {
    await tester.pumpWidget(
        localized(PlacesScreen(loadUnits: () async => [4]), locale: const Locale('cs')));
    await tester.pump();
    await tester.tap(find.byType(DropdownButton<String>).at(1));
    await tester.pumpAndSettle();
    await tester.tap(find.text('LH  levá zadní').last);
    await tester.pumpAndSettle();
    expect(settings.value.places, {4: 'LH'});
    changeSettings((s) => s.animalName = 'Bára');
    expect(tableOf(settings.value, {4: 0x1404}),
        'kind;horse\nname;Bára\nbelka;chip;code;place;kind\n4;0000000000001404;LH;left hind;B\n');
  });
}
