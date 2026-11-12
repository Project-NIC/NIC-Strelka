import 'dart:typed_data';

import 'package:flutter_test/flutter_test.dart';
import 'package:material_ui/material_ui.dart';
import 'package:pusinka/l10n/app_localizations.dart';
import 'package:pusinka/overview.dart';

void main() {
  test("a unit's state and fault named, in Czech and in English, a number unknown as it is", () {
    final cs = lookupAppLocalizations(const Locale('cs'));
    final en = lookupAppLocalizations(const Locale('en'));
    expect(stateName(cs, 4), 'čeká');
    expect(stateName(en, 0), 'recording');
    expect(stateName(en, 42), '42');
    expect(faultName(cs, 2), 'karta plná');
    expect(faultName(en, 0), '');
    expect(faultName(en, 11), 'no calibration');
  });

  test("the sensor by its WHO_AM_I; a chip's ID shown the other way round, a build as it comes", () {
    expect(sensorName(0x44), 'ICM-42686-P');
    expect(sensorName(0x73), 'LSM6DSV320X');
    expect(sensorName(0), '?');
    final chip = Uint8List.fromList([0x02, 0x6A, 0x7F, 0x3B, 0xA4, 0x09, 0x1E, 0x7C]);
    expect(hexBytes(chip), '7C1E09A43B7F6A02');
    expect(hexBytes(Uint8List.fromList([0x3f, 0x9c, 0x2e, 0x1a]), reversed: false), '3F9C2E1A');
  });
}
