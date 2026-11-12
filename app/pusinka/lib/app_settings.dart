/// Pušinka's settings, each with its default (APP.md, The menus, The settings), kept in the phone
/// as one JSON file beside the C3Ds: the calibration's validity among them, which the
/// measurement's choice holds every unit to. Changed outside the measurement alone.
library;

import 'dart:convert';
import 'dart:io';

import 'package:flutter/foundation.dart';
import 'package:path_provider/path_provider.dart';

enum CalValidity { everyMeasurement, twoHours, day, thirtyDays, unlimited }

enum LightWhileMeasuring { usual, faultsAlone, off }

class AppSettings {
  CalValidity calValidity = CalValidity.thirtyDays;
  int tempApartC = 10;               // a unit this far from its calibration's temperature, warned of
  int curvesS = 5;
  int numbersStrides = 10;
  int lostS = 3;                     // no answer to 64/14 for this long: the connection lost
  bool fullPower = false;            // +5 dBm, or the full power within the rules
  LightWhileMeasuring light = LightWhileMeasuring.usual;
  int nextSession = 1;               // the number the next session is given
  int lastStart = 0;                 // the last start, s since 1970
  Map<int, String> places = {};      // each unit's place's code, the set's table's
  String template = 'horse';         // the places' template: horse, dog, person
  String animalName = '';            // the animal's name, into the set's table
  String note = '';                  // the user's note, 64 characters, into the table and the names

  Map<String, Object?> toJson() => {
        'calValidity': calValidity.name,
        'tempApartC': tempApartC,
        'curvesS': curvesS,
        'numbersStrides': numbersStrides,
        'lostS': lostS,
        'fullPower': fullPower,
        'light': light.name,
        'nextSession': nextSession,
        'lastStart': lastStart,
        'places': {for (final e in places.entries) '${e.key}': e.value},
        'template': template,
        'animalName': animalName,
        'note': note,
      };

  static AppSettings fromJson(Map<String, Object?> j) {
    T byName<T extends Enum>(List<T> values, Object? name, T fallback) =>
        values.where((v) => v.name == name).firstOrNull ?? fallback;
    return AppSettings()
      ..calValidity = byName(CalValidity.values, j['calValidity'], CalValidity.thirtyDays)
      ..tempApartC = j['tempApartC'] as int? ?? 10
      ..curvesS = j['curvesS'] as int? ?? 5
      ..numbersStrides = j['numbersStrides'] as int? ?? 10
      ..lostS = j['lostS'] as int? ?? 3
      ..fullPower = j['fullPower'] as bool? ?? false
      ..light = byName(LightWhileMeasuring.values, j['light'], LightWhileMeasuring.usual)
      ..nextSession = j['nextSession'] as int? ?? 1
      ..lastStart = j['lastStart'] as int? ?? 0
      ..places = {
        for (final e in (j['places'] as Map<String, Object?>? ?? const {}).entries)
          int.parse(e.key): e.value as String
      }
      ..template = j['template'] as String? ?? 'horse'
      ..animalName = j['animalName'] as String? ?? ''
      ..note = j['note'] as String? ?? '';
  }

  static Future<File> _file() async =>
      File('${(await getApplicationDocumentsDirectory()).path}/Strelka/settings.json');

  /// The settings kept, or the defaults: none kept yet, or no storage — a test.
  static Future<AppSettings> load() async {
    try {
      final f = await _file();
      return fromJson(jsonDecode(await f.readAsString()) as Map<String, Object?>);
    } on Object {
      return AppSettings();
    }
  }

  Future<void> save() async {
    try {
      final f = await _file();
      await f.parent.create(recursive: true);
      await f.writeAsString(jsonEncode(toJson()));
    } on Object {
      // no storage: kept for this run alone
    }
  }

  /// A unit's calibration, taken at [calTime], s since 1970, valid [now] by this setting.
  bool calibrationValid(int calTime, int now) {
    if (calTime == 0) return false;
    return switch (calValidity) {
      CalValidity.everyMeasurement => calTime > lastStart,
      CalValidity.twoHours => now - calTime <= 2 * 3600,
      CalValidity.day => now - calTime <= 24 * 3600,
      CalValidity.thirtyDays => now - calTime <= 30 * 24 * 3600,
      CalValidity.unlimited => true,
    };
  }
}

/// The settings in force, and the set's measuring: no setting changes while it records.
final settings = ValueNotifier<AppSettings>(AppSettings());
final recording = ValueNotifier<bool>(false);

/// A change of the settings: made, shown at once and kept.
void changeSettings(void Function(AppSettings s) change) {
  final s = AppSettings.fromJson(settings.value.toJson());
  change(s);
  settings.value = s;
  s.save();
}

int nowS() => DateTime.now().millisecondsSinceEpoch ~/ 1000;
