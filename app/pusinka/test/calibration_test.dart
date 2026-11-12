import 'package:cupertino_ui/cupertino_ui.dart' show GlobalCupertinoLocalizations;
import 'package:flutter_localizations/flutter_localizations.dart' show GlobalWidgetsLocalizations;
import 'package:flutter_test/flutter_test.dart';
import 'package:material_ui/material_ui.dart';
import 'package:pusinka/app_settings.dart';
import 'package:pusinka/calibration.dart';
import 'package:pusinka/l10n/app_localizations.dart';
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
      home: Scaffold(body: child),
    );

/// The six faces as Pušinka leads them, the Belkas' answers made up: one moved on the first face
/// asked again alone, the turn shown between the faces, the ranges asked with face 0, the units'
/// calibrations read again after the sixth.
void main() {
  testWidgets('six faces, a unit moved asked again alone, the calibrations read again after',
      (tester) async {
    final now = nowS();
    var loads = 0;
    final asked = <(int, int, int, int, int)>[];
    Future<List<UnitOnSet>> load() async {
      loads++;
      final t = loads == 1 ? 0 : now;
      return [
        UnitOnSet(1, calTime: t),
        UnitOnSet(2,
            kind: kindKryska,
            calTime: loads == 1 ? now - 3 * 86400 : now,
            info: const {'sensor': lsm320}),          // the hoof's: its high-g beside
        const UnitOnSet(3, reachable: false),
      ];
    }

    Future<FaceAnswer> ask(int n, int face, int acc, int gyro, int hg) async {
      asked.add((n, face, acc, gyro, hg));
      final moved = n == 2 && face == 0 && asked.where((a) => a.$1 == 2).length == 1;
      return FaceAnswer(moved ? FaceGot.moved : FaceGot.taken);
    }

    await tester.pumpWidget(localized(
        CalibrationScreen(host: '', password: '', loadUnits: load, askFace: ask)));
    await tester.pump();
    expect(find.text('Every unit onto the face Z ↑, then touch Confirm.'), findsOneWidget);
    expect(find.text('3 days'), findsOneWidget);
    await tester.tap(find.text('Confirm'));
    await tester.pump();
    expect(find.text('moved ✗, again'), findsOneWidget);
    expect(asked, [(1, 0, 16, 2000, 0), (2, 0, 16, 2000, 32)]);   // not the unit out of reach
    await tester.tap(find.text('Confirm'));
    await tester.pump();
    expect(asked.last, (2, 0, 16, 2000, 32));
    expect(asked.length, 3);                                  // the moved one alone
    expect(find.text('Turn every unit about its X axis onto the face Z ↓, then touch Confirm.'),
        findsOneWidget);
    for (var face = 1; face < 6; face++) {
      await tester.tap(find.text('Confirm'));
      await tester.pump();
      await tester.pump();
    }
    expect(find.text('Done: every unit keeps its new calibration.'), findsOneWidget);
    expect(loads, 2);
    expect(find.text('today'), findsNWidgets(2));
  });
}
