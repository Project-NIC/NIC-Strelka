import 'package:cupertino_ui/cupertino_ui.dart' show GlobalCupertinoLocalizations;
import 'package:flutter_localizations/flutter_localizations.dart' show GlobalWidgetsLocalizations;
import 'package:flutter_test/flutter_test.dart';
import 'package:material_ui/material_ui.dart';
import 'package:pusinka/l10n/app_localizations.dart';
import 'package:pusinka/main.dart';

/// A screen under the app's own localizations, in the language given.
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

void main() {
  tearDown(() => language.value = null);

  testWidgets('every language has its table, the seven of APP.md', (tester) async {
    expect(AppLocalizations.supportedLocales.map((l) => l.languageCode).toSet(),
        {'en', 'cs', 'ru', 'es', 'pt', 'de', 'fr'});
    expect(languages.map((l) => l.$1.languageCode).toSet(),
        AppLocalizations.supportedLocales.map((l) => l.languageCode).toSet());
  });

  testWidgets('the first screen in Czech when Czech is chosen', (tester) async {
    language.value = const Locale('cs');
    await tester.pumpWidget(const PusinkaApp());
    await tester.pumpAndSettle();
    expect(find.text('Adresa Povodoku'), findsOneWidget);
    expect(find.widgetWithText(FilledButton, 'Připojit'), findsOneWidget);
  });

  testWidgets('the bar of six: each key its number and name, a touch chooses', (tester) async {
    var chosen = -1;
    await tester.pumpWidget(localized(Builder(
        builder: (context) => Scaffold(
            bottomNavigationBar: MenuBar6(
                current: 2,
                names: menuNames(AppLocalizations.of(context)),
                onChosen: (i) => chosen = i))),
        locale: const Locale('de')));
    for (final name in ['Übersicht', 'Kalibrierung', 'Messung', 'Daten', 'Diagnose', 'Einstellungen']) {
      expect(find.text(name), findsOneWidget);
    }
    expect(find.text('6'), findsOneWidget);
    await tester.tap(find.text('Daten'));
    expect(chosen, 3);
  });

  testWidgets('the settings choose the language; About shows Pušinka drawn', (tester) async {
    await tester.pumpWidget(localized(const SettingsScreen()));
    expect(find.text('Français'), findsOneWidget);
    await tester.tap(find.text('Français'));
    expect(language.value, const Locale('fr'));
    await tester.scrollUntilVisible(find.textContaining("Strelka's puppy"), 200);
    expect(find.textContaining("Strelka's puppy"), findsOneWidget);
  });
}
