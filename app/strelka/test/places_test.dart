import 'dart:convert';

import 'package:strelka/strelka.dart';
import 'package:test/test.dart';

/// The places' templates and the set's table: the codes and pairs by the animal and the language,
/// the CSV as RECORD.md has it, its pieces for 64/18 never cut inside a character.
void main() {
  test('the templates: a horse, a cow, a dog, a cat the same places, a person; English codes', () {
    final horse = placeTemplate('horse');
    expect(horse.places.map((p) => p.code), ['HD', 'WI', 'TS', 'LS', 'PE', 'LF', 'RF', 'LH', 'RH']);
    expect(horse.pairs, [('LF', 'RF'), ('LH', 'RH')]);
    expect(placeTemplate('dog').places, horse.places);
    expect(placeTemplate('person').nameOf('RE'), 'right elbow');
    expect(placeTemplate('cat').places, horse.places);
    expect(placeTemplate('goat').animal, 'horse');            // none such: the first
  });

  test("the set's table as RECORD.md has it, English, the user's own words as written", () {
    final csv = setTable(animal: [
      ('kind', 'horse'),
      ('name', 'Bára'),
      ('history', 'Od jara kulhá; víc na tvrdém.'),
    ], belkas: [
      (1, 0x0123456789ABCDEF, 'HD', 'head'),
      (3, 0x0123456789ABCDF1, 'LF', 'left fore'),
    ], kinds: {3: 'K'});
    expect(csv, 'kind;horse\nname;Bára\nhistory;"Od jara kulhá; víc na tvrdém."\n'
        'belka;chip;code;place;kind\n1;0123456789ABCDEF;HD;head;B\n'
        '3;0123456789ABCDF1;LF;left fore;K\n');
  });

  test('the pieces: at most the bytes given, a character never cut, joined the whole', () {
    final csv = 'ř' * 1000;                                  // 2 000 B
    final pieces = tablePieces(csv, most: 1901);
    expect(pieces.length, 2);
    expect(utf8.encode(pieces[0].$2).length, 1900);
    expect(pieces[1].$1, 1900);
    expect(pieces.map((p) => p.$2).join(), csv);
  });
}
