import 'dart:convert';
import 'dart:io';
import 'dart:typed_data';

import 'package:cryptography/cryptography.dart';
import 'package:cupertino_ui/cupertino_ui.dart' show GlobalCupertinoLocalizations;
import 'package:flutter_localizations/flutter_localizations.dart' show GlobalWidgetsLocalizations;
import 'package:flutter_test/flutter_test.dart';
import 'package:material_ui/material_ui.dart';
import 'package:pusinka/firmware.dart';
import 'package:pusinka/l10n/app_localizations.dart';

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

/// An image with MCUboot's header, version 0.8.6, and a body (tools/strelka_images.py, fake_image).
Uint8List image(int size) {
  final b = Uint8List(32 + size);
  ByteData.sublistView(b)
    ..setUint32(0, 0x96F3B83D, Endian.little)
    ..setUint16(8, 32, Endian.little)
    ..setUint32(12, size, Endian.little);
  b[20] = 0;
  b[21] = 8;
  b[22] = 6;
  for (var i = 0; i < size; i++) {
    b[32 + i] = (0x5A + i) & 0xFF;
  }
  return b;
}

/// The firmware's screen: the version kept on the phone, the units each with its own and its
/// state — waiting, up to date, off — and all updated one at a time, the Belkas first and Povodok
/// last, Povodok then joined again.
void main() {
  late Directory dir;

  setUpAll(() async {
    dir = await Directory.systemTemp.createTemp('pusinka_fw');
    final images = <String, Object?>{};
    for (final kind in ['belka', 'kryska', 'povodok']) {
      final b = image(2000 + kind.length);
      await File('${dir.path}/$kind.bin').writeAsBytes(b);
      final sha = (await Sha256().hash(b)).bytes;
      images[kind] = {
        'file': '$kind.bin.gz',
        'size': b.length,
        'sha256': [for (final x in sha) x.toRadixString(16).padLeft(2, '0')].join(),
      };
    }
    await File('${dir.path}/manifest.json')
        .writeAsString(jsonEncode({'version': '0.8.6', 'images': images}));
  });

  tearDownAll(() => dir.delete(recursive: true));

  testWidgets('the units, their versions and states; all updated, Povodok last', (tester) async {
    final given = <(int?, int)>[];
    await tester.runAsync(() async {
      await tester.pumpWidget(localized(FirmwareScreen(
        dir: dir,
        loadRows: () async => [
          FirmwareRow(1, {'who': 2, 'kind': 0, 'fw': [0, 8, 5]}),
          FirmwareRow(7, {'who': 2, 'kind': 1, 'fw': [0, 8, 6]}),
          FirmwareRow(3, const {}, reachable: false),
          FirmwareRow(null, {'who': 0, 'n': 0, 'fw': [0, 8, 5]}),
        ],
        update: (n, image, progress) async {
          given.add((n, image.length));
          progress(image.length, image.length);
        },
      )));
      await Future<void>.delayed(const Duration(milliseconds: 300));
    });
    await tester.pump();
    expect(find.text('Firmware 0.8.6 on the phone, signed and checked'), findsOneWidget);
    expect(find.text('waiting'), findsNWidgets(2));
    expect(find.text('up to date ✓'), findsOneWidget);
    expect(find.text('off: later'), findsOneWidget);
    expect(find.text('K7'), findsOneWidget);
    expect(find.text('P1'), findsOneWidget);
    await tester.runAsync(() async {
      await tester.tap(find.text('Update all'));
      await Future<void>.delayed(const Duration(milliseconds: 100));
    });
    await tester.pump();
    expect(given, [(1, 2037), (null, 2039)]);      // the Belka's image, then Povodok's
    expect(find.text('done ✓'), findsNWidgets(2));
    expect(find.text('Povodok starts again with the new firmware: join it again.'), findsOneWidget);
  });
}
