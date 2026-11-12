@TestOn('vm')
library;

import 'dart:io';
import 'dart:math';

import 'package:strelka/strelka.dart';
import 'package:test/test.dart';

/// The set made up by the application held to the Python tools: the key sealed as
/// `cryptography`'s AESCCM seals it, the file as tools/strelka_set.py writes it; a set drawn, kept
/// and read back; the quietest channel of the map. Python 3 with cryptography is wanted.
void main() {
  test('the key sealed as AES-CCM seals it, tools/smp.py\'s 64/19', () async {
    final key = [for (var i = 0; i < 16; i++) 0xA0 + i];
    final nonce = [for (var i = 0; i < 13; i++) 3 * i + 1];
    final text = [for (var i = 0; i < 16; i++) i];
    const script = '''
import sys
from cryptography.hazmat.primitives.ciphers.aead import AESCCM
k, n, t = (bytes.fromhex(a) for a in sys.argv[1:4])
print(AESCCM(k, tag_length=4).encrypt(n, t, None).hex())
''';
    String hex(List<int> b) => [for (final x in b) x.toRadixString(16).padLeft(2, '0')].join();
    final r = await Process.run('python3', ['-c', script, hex(key), hex(nonce), hex(text)]);
    expect(r.exitCode, 0, reason: '${r.stderr}');
    final (sealed, mic) = await sealKey(key, nonce, text);
    expect(hex([...sealed, ...mic]), (r.stdout as String).trim());
  });

  test('STRELKA.SET as tools/strelka_set.py writes it', () async {
    final set = NewSet(0xA1B2C3D4, [for (var i = 1; i <= 16; i++) i], 7);
    final tools = '${Directory.current.path}/../../tools';
    final r = await Process.run(
        'python3', ['$tools/strelka_set.py', 'A1B2C3D4', '7', set.keyHex, '5']);
    expect(r.exitCode, 0, reason: '${r.stderr}');
    expect(set.fileFor(5), r.stdout);
    expect(() => set.fileFor(64), throwsArgumentError);
  });

  test('a set drawn: never zero nor the search\'s address, kept and read back the same', () {
    final set = NewSet.draw(channel: 12, label: 77, random: Random(4));
    expect(set.address != 0 && set.address != searchAddress, isTrue);
    expect(set.key.length, 16);
    final again = NewSet.fromJson(set.toJson());
    expect([again.address, again.channel, again.label, again.keyHex],
        [set.address, 12, 77, set.keyHex]);
    expect(() => NewSet(searchAddress, List.filled(16, 0), 1), throwsArgumentError);
    expect(() => NewSet(1, List.filled(16, 0), 39), throwsArgumentError);
    expect(() => NewSet(1, List.filled(16, 0), 1, label: 10000), throwsArgumentError);
  });

  test('the quietest channel: the least noise anyone measured, none while nothing is', () {
    List<int> band(Map<int, int> at) =>
        [for (var c = 0; c < 40; c++) at[c] ?? SetDiag.notMeasured];
    final d = SetDiag(const [], {
      'noise': [band({3: -90, 7: -97, 9: -97}), band({7: -80})],
      'bnoise': band({9: -99, 39: -110}),
    });
    expect(quietestChannel(d), 9);          // 7 loud at the second Povodok; 39 the search's
    expect(quietestChannel(SetDiag(const [], {})), isNull);
  });
}
