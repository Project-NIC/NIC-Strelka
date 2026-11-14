/// The animal and the places (APP.md, What it must do, 4): the templates of the places' codes by
/// the animal, English as research has them, with their pairs of left and right; the set's table as CSV
/// (RECORD.md, The set's table), the animal a line a field, then the Belkas under a line naming the
/// columns; and the table written to Povodok in pieces by 64/18, which passes it to every Belka.
library;

import 'dart:convert';

import 'smp.dart';

/// A place: its code of two letters and its name, English both, as research has them.
class Place {
  const Place(this.code, this.name);

  final String code, name;
}

/// The places of an animal, the pairs of left and right among them. English in the record in
/// every language; the application shows each code with the name in the user's language.
class PlaceTemplate {
  const PlaceTemplate(this.animal, this.places, this.pairs);

  /// The animal, the table's kind: horse, cow, dog, cat, person.
  final String animal;
  final List<Place> places;
  final List<(String, String)> pairs;

  String? nameOf(String code) {
    for (final p in places) {
      if (p.code == code) return p.name;
    }
    return null;
  }
}

const List<Place> _quadruped = [
  Place('HD', 'head'),
  Place('WI', 'withers'),
  Place('TS', 'thoracic spine'),
  Place('LS', 'lumbar spine'),
  Place('PE', 'pelvis'),
  Place('LF', 'left fore'),
  Place('RF', 'right fore'),
  Place('LH', 'left hind'),
  Place('RH', 'right hind'),
];
const List<(String, String)> _quadrupedPairs = [('LF', 'RF'), ('LH', 'RH')];

/// The templates: a horse, a cow, a dog, a cat — the same places — and a person; the record says
/// which animal.
const List<PlaceTemplate> placeTemplates = [
  PlaceTemplate('horse', _quadruped, _quadrupedPairs),
  PlaceTemplate('cow', _quadruped, _quadrupedPairs),
  PlaceTemplate('dog', _quadruped, _quadrupedPairs),
  PlaceTemplate('cat', _quadruped, _quadrupedPairs),
  PlaceTemplate('person', [
    Place('LS', 'left shoulder'),
    Place('RS', 'right shoulder'),
    Place('LE', 'left elbow'),
    Place('RE', 'right elbow'),
    Place('LH', 'left hip'),
    Place('RH', 'right hip'),
  ], [
    ('LS', 'RS'),
    ('LE', 'RE'),
    ('LH', 'RH')
  ]),
];

/// The template of [animal], or the first.
PlaceTemplate placeTemplate(String animal) =>
    placeTemplates.firstWhere((t) => t.animal == animal, orElse: () => placeTemplates.first);

/// A field as RFC 4180 has it: in quotes when it holds a semicolon, a quote or a new line.
String csvField(String v) =>
    v.contains(RegExp('[;"\n\r]')) ? '"${v.replaceAll('"', '""')}"' : v;

/// The table's lines as fields, semicolons parting them and quotes as RFC 4180 has them
/// (tools/strelka_record.py, table_rows).
List<List<String>> tableRows(String text) {
  final rows = <List<String>>[];
  var row = <String>[], field = StringBuffer();
  var quoted = false, any = false;
  for (var i = 0; i < text.length; i++) {
    final c = text[i];
    if (quoted) {
      if (c == '"' && i + 1 < text.length && text[i + 1] == '"') {
        field.write('"');
        i++;
      } else if (c == '"') {
        quoted = false;
      } else {
        field.write(c);
      }
    } else if (c == '"') {
      quoted = true;
      any = true;
    } else if (c == ';') {
      row.add(field.toString());
      field = StringBuffer();
      any = true;
    } else if (c == '\n' || c == '\r') {
      if (c == '\r' && i + 1 < text.length && text[i + 1] == '\n') i++;
      if (any || field.isNotEmpty) row.add(field.toString());
      rows.add(row);
      row = <String>[];
      field = StringBuffer();
      any = false;
    } else {
      field.write(c);
      any = true;
    }
  }
  if (any || field.isNotEmpty) {
    row.add(field.toString());
    rows.add(row);
  }
  return rows;
}

/// The note the user gives a session, at most 64 characters, the table's line `note`
/// (APP.md, The names); empty where it has none.
String tableNote(String table) {
  for (final r in tableRows(table)) {
    if (r.length >= 2 && r[0] == 'note') return r[1];
  }
  return '';
}

/// The set's table: the animal's lines, then a line a Belka — its number, its chip, its place's
/// code and name, its kind's letter — under a line naming the columns; English, what the user wrote
/// as written. A unit's kind is B where [kinds] has it not.
String setTable({
  required List<(String, String)> animal,
  required List<(int, int, String, String)> belkas,
  Map<int, String> kinds = const {},
}) {
  final out = StringBuffer();
  for (final (field, value) in animal) {
    out.write('${csvField(field)};${csvField(value)}\n');
  }
  out.write('belka;chip;code;place;kind\n');
  for (final (n, chip, code, name) in belkas) {
    out.write('$n;${chip.toRadixString(16).toUpperCase().padLeft(16, '0')};'
        '${csvField(code)};${csvField(name)};${kinds[n] ?? 'B'}\n');
  }
  return out.toString();
}

/// The table's pieces for 64/18, at most [most] bytes of UTF-8 each, never inside a character:
/// each its byte offset and its text.
List<(int, String)> tablePieces(String csv, {int most = 1900}) {
  final pieces = <(int, String)>[];
  var off = 0, piece = StringBuffer(), bytes = 0;
  for (final r in csv.runes) {
    final ch = String.fromCharCode(r);
    final n = utf8.encode(ch).length;
    if (bytes + n > most) {
      pieces.add((off, piece.toString()));
      off += bytes;
      piece = StringBuffer();
      bytes = 0;
    }
    piece.write(ch);
    bytes += n;
  }
  if (bytes > 0 || pieces.isEmpty) pieces.add((off, piece.toString()));
  return pieces;
}

/// 64/18's bodies for the table, a piece each: its offset, the whole's length, its text.
List<Map<String, Object?>> tableBodies(String csv) {
  final len = utf8.encode(csv).length;
  return [
    for (final (off, text) in tablePieces(csv)) {'off': off, 'len': len, 'csv': text},
  ];
}

/// The table written to Povodok by 64/18, piece by piece; Povodok passes it to every Belka, which
/// writes it after the head of every file.
Future<void> writeTable(SmpClient c, String csv) async {
  for (final body in tableBodies(csv)) {
    await c.write(groupStrelka, Strelka.table, body: body);
  }
}
