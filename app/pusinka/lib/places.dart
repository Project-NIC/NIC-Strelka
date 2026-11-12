/// The animal and the places (APP.md, What it must do, 4; the sketch
/// app/mockups/en/settings-d-templates.png): the template chosen by the animal; the animal's name;
/// each unit of the set given its place by a touch, its English code shown with the place's name in
/// the user's language. The code shows the unit everywhere; the set's table, English in every
/// language, goes with the session into every unit's file (the measurement, 64/18). Never while
/// the set records.
library;

import 'dart:async';

import 'package:material_ui/material_ui.dart';
import 'package:strelka/strelka.dart';

import 'app_settings.dart';
import 'l10n/app_localizations.dart';
import 'main.dart';

class PlacesScreen extends StatefulWidget {
  const PlacesScreen({super.key, this.client, this.loadUnits});

  final SmpClient? client;      // Povodok's

  /// For the tests: the set's units without a set.
  final Future<List<int>> Function()? loadUnits;

  @override
  State<PlacesScreen> createState() => _PlacesScreenState();
}

/// The set's units by 64/14.
Future<List<int>> setUnits(SmpClient c) async {
  final set = await c.read(groupStrelka, Strelka.set);
  return [
    for (final b in set['belkas'] as List<Object?>? ?? const [])
      (b as Map<Object?, Object?>)['n'] as int
  ]..sort();
}

class _PlacesScreenState extends State<PlacesScreen> {
  List<int>? _units;
  String? _trouble;
  late final TextEditingController _name =
      TextEditingController(text: settings.value.animalName);

  @override
  void initState() {
    super.initState();
    unawaited(_load());
  }

  @override
  void dispose() {
    _name.dispose();
    super.dispose();
  }

  Future<void> _load() async {
    try {
      final units = await (widget.loadUnits ??
          () async => widget.client == null ? <int>[] : setUnits(widget.client!))();
      if (mounted) setState(() => _units = units);
    } on Exception catch (e) {
      if (mounted) setState(() => _trouble = '$e');
    }
  }

  @override
  Widget build(BuildContext context) {
    final t = AppLocalizations.of(context);
    return ValueListenableBuilder<bool>(
      valueListenable: recording,
      builder: (context, measuring, _) => ValueListenableBuilder<AppSettings>(
        valueListenable: settings,
        builder: (context, s, _) {
          final tpl = placeTemplate(s.template);
          final units = _units;
          return Scaffold(
            appBar: AppBar(title: Text(t.placesTitle)),
            body: ListView(padding: const EdgeInsets.all(12), children: [
              ListTile(
                title: Text(t.placesTemplate),
                trailing: DropdownButton<String>(
                  value: s.template,
                  dropdownColor: Look.blue,
                  items: [
                    for (final o in placeTemplates)
                      DropdownMenuItem(
                          value: o.animal,
                          child: Text(animalName(t, o.animal),
                              style: const TextStyle(color: Look.yellow)))
                  ],
                  // another template: its codes, the units' taken from it anew
                  onChanged: measuring
                      ? null
                      : (v) => changeSettings((s) => s
                        ..template = v ?? s.template
                        ..places = {}),
                ),
              ),
              ListTile(
                title: TextField(
                  controller: _name,
                  enabled: !measuring,
                  maxLength: 32,
                  decoration: InputDecoration(labelText: t.placesAnimal),
                  onChanged: (v) => changeSettings((s) => s.animalName = v),
                ),
              ),
              Text(t.placesNote, style: const TextStyle(fontSize: 12)),
              const Divider(),
              if (units == null && _trouble == null)
                const Center(child: CircularProgressIndicator())
              else if (units != null && units.isEmpty)
                Text(t.measNone)
              else if (units != null)
                for (final n in units)
                  ListTile(
                    dense: true,
                    title: Text('$n', style: const TextStyle(color: Look.white, fontSize: 18)),
                    trailing: DropdownButton<String>(
                      value: tpl.nameOf(s.places[n] ?? '') == null ? '' : s.places[n],
                      dropdownColor: Look.blue,
                      items: [
                        const DropdownMenuItem(
                            value: '', child: Text('--', style: TextStyle(color: Look.yellow))),
                        for (final p in tpl.places)
                          DropdownMenuItem(
                              value: p.code,
                              child: Text('${p.code}  ${placeName(t, p.name)}',
                                  style: const TextStyle(color: Look.yellow))),
                      ],
                      onChanged: measuring
                          ? null
                          : (v) => changeSettings((s) {
                                if (v == null || v.isEmpty) {
                                  s.places.remove(n);
                                } else {
                                  s.places.removeWhere((_, code) => code == v);   // a place once
                                  s.places[n] = v;
                                }
                              }),
                    ),
                  ),
              if (_trouble != null)
                Text(_trouble!, style: TextStyle(color: Theme.of(context).colorScheme.error)),
            ]),
          );
        },
      ),
    );
  }
}

/// The set's table from the settings (RECORD.md, The set's table), English in every language: the
/// animal, its name as written, and each of [belkas] with its chip, its place's code and name, and
/// its kind from [kinds], B where none.
String tableOf(AppSettings s, Map<int, int> belkas, {Map<int, int> kinds = const {}}) {
  final tpl = placeTemplate(s.template);
  return setTable(animal: [
    ('kind', tpl.animal),
    if (s.animalName.isNotEmpty) ('name', s.animalName),
    if (s.note.isNotEmpty) ('note', s.note),
  ], belkas: [
    for (final e in belkas.entries)
      (e.key, e.value, s.places[e.key] ?? '', tpl.nameOf(s.places[e.key] ?? '') ?? ''),
  ], kinds: {for (final e in kinds.entries) e.key: kindLetter(e.value)});
}

/// The animal in the user's language.
String animalName(AppLocalizations t, String animal) => switch (animal) {
      'cow' => t.animalCow,
      'dog' => t.animalDog,
      'cat' => t.animalCat,
      'person' => t.animalPerson,
      _ => t.animalHorse,
    };

/// A place's English name in the user's language; one not known as it is.
String placeName(AppLocalizations t, String english) => switch (english) {
      'head' => t.placeHead,
      'withers' => t.placeWithers,
      'thoracic spine' => t.placeThoracic,
      'lumbar spine' => t.placeLumbar,
      'pelvis' => t.placePelvis,
      'left fore' => t.placeLeftFore,
      'right fore' => t.placeRightFore,
      'left hind' => t.placeLeftHind,
      'right hind' => t.placeRightHind,
      'left shoulder' => t.placeLeftShoulder,
      'right shoulder' => t.placeRightShoulder,
      'left elbow' => t.placeLeftElbow,
      'right elbow' => t.placeRightElbow,
      'left hip' => t.placeLeftHip,
      'right hip' => t.placeRightHip,
      _ => english,
    };
