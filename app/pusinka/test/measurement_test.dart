import 'package:cupertino_ui/cupertino_ui.dart' show GlobalCupertinoLocalizations;
import 'package:flutter_localizations/flutter_localizations.dart' show GlobalWidgetsLocalizations;
import 'package:flutter_test/flutter_test.dart';
import 'package:material_ui/material_ui.dart';
import 'package:pusinka/app_settings.dart';
import 'package:pusinka/l10n/app_localizations.dart';
import 'package:pusinka/main.dart';
import 'package:pusinka/measurement.dart';
import 'package:strelka/strelka.dart';

Widget localized(Widget child, {Locale locale = const Locale('en')}) => MaterialApp(
      locale: locale,
      localizationsDelegates: const [
        AppLocalizations.delegate,
        GlobalMaterialLocalizations.delegate,
        GlobalWidgetsLocalizations.delegate,
        GlobalCupertinoLocalizations.delegate,
      ],
      supportedLocales: AppLocalizations.supportedLocales,
      home: Scaffold(body: child),
    );

void main() {
  const day = 24 * 3600;
  const now = 1762600000;

  tearDown(() {
    settings.value = AppSettings();
    recording.value = false;
  });

  test('a calibration valid by the setting: none never, every measurement after the last start',
      () {
    final s = AppSettings();
    expect(s.calibrationValid(0, now), isFalse);
    expect(s.calibrationValid(now - 29 * day, now), isTrue);       // 30 days, the default
    expect(s.calibrationValid(now - 31 * day, now), isFalse);
    s.calValidity = CalValidity.twoHours;
    expect(s.calibrationValid(now - 3600, now), isTrue);
    expect(s.calibrationValid(now - 3 * 3600, now), isFalse);
    s
      ..calValidity = CalValidity.everyMeasurement
      ..lastStart = now - 600;
    expect(s.calibrationValid(now - 300, now), isTrue);            // calibrated since
    expect(s.calibrationValid(now - 900, now), isFalse);           // before the last start
    s.calValidity = CalValidity.unlimited;
    expect(s.calibrationValid(1, now), isTrue);
  });

  test('the settings kept as JSON come back the same', () {
    final s = AppSettings()
      ..calValidity = CalValidity.day
      ..tempApartC = 15
      ..light = LightWhileMeasuring.faultsAlone
      ..nextSession = 45
      ..places = {1: 'HL', 7: 'LP'};
    final back = AppSettings.fromJson(s.toJson());
    expect(back.toJson(), s.toJson());
    expect(AppSettings.fromJson(const {}).toJson(), AppSettings().toJson());
  });

  test("a tile's state: out of reach, no calibration, expired, chosen or free", () {
    final s = AppSettings();
    const fresh = now - day;
    expect(tileState(const UnitOnSet(1, reachable: false), {}, s, now), TileState.unreachable);
    expect(tileState(const UnitOnSet(2), {}, s, now), TileState.noCalibration);
    expect(tileState(const UnitOnSet(3, calTime: now - 90 * day), {}, s, now), TileState.expired);
    expect(tileState(const UnitOnSet(4, calTime: fresh), {4}, s, now), TileState.chosen);
    expect(tileState(const UnitOnSet(5, calTime: fresh), {4}, s, now), TileState.free);
  });

  testWidgets("a tile: the place's code, the kind and number, why it cannot be chosen",
      (tester) async {
    settings.value = AppSettings()..places = {7: 'LF'};
    await tester.pumpWidget(localized(Row(children: [
      UnitTile(
          unit: const UnitOnSet(7, kind: kindKryska, calTime: now),
          state: TileState.chosen,
          onTap: () {}),
      UnitTile(
          unit: const UnitOnSet(12, kind: kindZvjozdocka),
          state: TileState.noCalibration,
          onTap: () {}),
    ])));
    expect(find.text('LF'), findsOneWidget);
    expect(find.text('K7'), findsOneWidget);
    expect(find.text('Z12'), findsOneWidget);
    expect(find.text('no calibration'), findsOneWidget);
  });

  testWidgets('the measurement settings in Czech, changed, and closed while the set records',
      (tester) async {
    await tester.pumpWidget(localized(const SettingsScreen(), locale: const Locale('cs')));
    await tester.scrollUntilVisible(find.text('30 dní'), 200);
    expect(find.text('Měření a kalibrace'), findsOneWidget);
    expect(find.text('30 dní'), findsOneWidget);
    await tester.tap(find.text('30 dní'));
    await tester.pumpAndSettle();
    await tester.tap(find.text('2 h').last);
    await tester.pumpAndSettle();
    expect(settings.value.calValidity, CalValidity.twoHours);

    recording.value = true;
    await tester.pump();
    final menus = tester.widgetList<DropdownButton<Object?>>(find.byWidgetPredicate(
        (w) => w is DropdownButton));
    expect(menus.every((d) => d.onChanged == null), isTrue);
  });
}
