import 'package:cupertino_ui/cupertino_ui.dart' show GlobalCupertinoLocalizations;
import 'package:flutter_localizations/flutter_localizations.dart' show GlobalWidgetsLocalizations;
import 'package:flutter_test/flutter_test.dart';
import 'package:material_ui/material_ui.dart';
import 'package:pusinka/diagnostics.dart';
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

/// The diagnostics from a reading of 64/23 made up: a unit heard with every item, one not
/// heard, the band with this Povodok's noise and the Belkas' on the set's channel.
void main() {
  testWidgets('the band and the units as 64/23 gives them, the unknown left out', (tester) async {
    var reads = 0;
    Future<SetDiag> read() async {
      reads++;
      return SetDiag([
        UnitDiag.fromMap({
          'n': 5, 'rssi': -61, 'ago': 10, 'cell': 87, 'temp': 25, 'mv': 3291, 'free': 90,
          'dropped': 1, 'sync': -58, 'missed': 2, 'offset': -1234, 'out': [3, 0], 'ch': 7,
          'noise': -93,
        }),
        UnitDiag.fromMap({'n': 9, 'ago': 300}),
      ], {
        'ch': 7, 'spare': 23, 'margin': [24, 31, 31, 31],
        'noise': [
          [for (var c = 0; c < 40; c++) c == 11 ? -71 : -95],
          for (var k = 1; k < 4; k++) [for (var c = 0; c < 40; c++) -128],
        ],
        'bnoise': [for (var c = 0; c < 40; c++) c == 7 ? -93 : -128],
      });
    }

    await tester.pumpWidget(localized(DiagnosticsScreen(read: read)));
    await tester.pump();
    expect(find.text('the set 7 · spare 23'), findsOneWidget);
    expect(find.text('Povodok 1: margin 48 dB'), findsOneWidget);
    expect(find.textContaining('-61 dBm · heard 0.2 s ago'), findsOneWidget);
    expect(find.textContaining('SYNC -58 dBm, 2 missed · clock -1234 ns · out of range 3/0'),
        findsOneWidget);
    expect(find.textContaining('card 90 % free, 1 dropped'), findsOneWidget);
    expect(find.textContaining('not heard'), findsOneWidget);
    await tester.pump(const Duration(seconds: 1));
    expect(reads, 2);                                     // once a second
    await tester.pumpWidget(const SizedBox());           // the timer stopped with the screen
  });

  test("a channel's noise, the loudest any measurer saw; none measured, null", () {
    final d = SetDiag([], {
      'noise': [
        [-95, -128],
        [-90, -128]
      ],
      'bnoise': [-99, -128],
    });
    expect(d.channelNoise(0), -90);
    expect(d.channelNoise(1), isNull);
  });

  test('the quietest channel offered for a move, never the set\'s own nor one unmeasured', () {
    final d = SetDiag([], {
      'ch': 2,
      'noise': [
        [-80, -128, -99, -91]
      ],
      'bnoise': [-128, -128, -128, -128],
    });
    expect(quietestChannel(d, except: d.ch), 3);
  });
}
