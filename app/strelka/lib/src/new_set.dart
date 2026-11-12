/// The set made up by the application (APP.md, What it must do, 3): a new set drawn — its address,
/// its key and its channel, the quietest the map of the band offers, or the user's — given to each
/// Povodok by 64/19, its key sealed under that write's own lock (COMMANDS.md), and the file a Belka
/// takes from its disk on the cable, `STRELKA.SET` (tools/strelka_set.py). The Belkas then come in
/// by the search (64/12, 64/13), or by the file.
library;

import 'dart:convert';
import 'dart:math';
import 'dart:typed_data';

import 'package:cryptography/cryptography.dart';

import 'diag.dart';
import 'smp.dart';

/// The search's address, the same in every firmware (PRINCIPLE.md, The inside link): never a
/// set's.
const int searchAddress = 0x52E6B438;

/// The set's channels, 0–38; the 40th, 2480 MHz, is the search's (PRINCIPLE.md, The channels).
const int setChannels = 39;

/// A set's own number, the user's, 1–9999; 0 none (COMMANDS.md, 64/19).
const int labelMost = 9999;

/// A set: its address, its key of 16 B, its channel, and its own number, 0 none.
class NewSet {
  NewSet(this.address, List<int> key, this.channel, {this.label = 0})
      : key = Uint8List.fromList(key) {
    if (address <= 0 || address > 0xFFFFFFFF || address == searchAddress) {
      throw ArgumentError.value(address, 'address');
    }
    if (this.key.length != 16) throw ArgumentError.value(key, 'key', '16 B');
    if (channel < 0 || channel >= setChannels) throw ArgumentError.value(channel, 'channel');
    if (label < 0 || label > labelMost) throw ArgumentError.value(label, 'label');
  }

  /// A set drawn anew by a secure random source: the address neither zero nor the search's.
  factory NewSet.draw({required int channel, int label = 0, Random? random}) {
    final r = random ?? Random.secure();
    int address;
    do {
      address = r.nextInt(1 << 32);
    } while (address == 0 || address == searchAddress);
    return NewSet(address, [for (var i = 0; i < 16; i++) r.nextInt(256)], channel, label: label);
  }

  /// The set kept between Povodoks, given to each in turn: as JSON, the key in hex.
  factory NewSet.fromJson(String json) {
    final m = jsonDecode(json) as Map<String, Object?>;
    final hex = m['key'] as String;
    return NewSet(m['address'] as int,
        [for (var i = 0; i < hex.length; i += 2) int.parse(hex.substring(i, i + 2), radix: 16)],
        m['channel'] as int,
        label: m['label'] as int? ?? 0);
  }

  final int address, channel, label;
  final Uint8List key;

  String get keyHex =>
      [for (final b in key) b.toRadixString(16).padLeft(2, '0')].join().toUpperCase();

  String get addressHex => address.toRadixString(16).toUpperCase().padLeft(8, '0');

  String toJson() =>
      jsonEncode({'address': address, 'key': keyHex, 'channel': channel, 'label': label});

  /// `STRELKA.SET` for the Belka numbered [number], 1–63, as tools/strelka_set.py writes it: taken
  /// by the Belka when its cable leaves, then wiped (FIRMWARE.md, What a unit keeps).
  String fileFor(int number) {
    if (number < 1 || number > 63) throw ArgumentError.value(number, 'number');
    return '# NIC-Strelka: the set of this Belka, taken when its cable leaves, then wiped\n'
        'set = $addressHex\nchannel = $channel\nkey = $keyHex\nnumber = $number\n';
  }
}

/// The file's name on a Belka's disk.
const String setFileName = 'STRELKA.SET';

/// The quietest of the set's channels on the map of the band: the least noise any measurer saw,
/// the lower channel on a tie, [except] left out — the set's own, for a move (64/24); null while
/// none is measured.
int? quietestChannel(SetDiag d, {int? except}) {
  int? best, bestNoise;
  for (var c = 0; c < setChannels; c++) {
    final n = d.channelNoise(c);
    if (c != except && n != null && (bestNoise == null || n < bestNoise)) {
      best = c;
      bestNoise = n;
    }
  }
  return best;
}

/// AES-CCM with a nonce of 13 B and a tag of 4 B, no associated data, the text one block of 16 B
/// (NIST SP 800-38C; RFC 3610): the key 64/19 and 64/9 seal, as tools/smp.py's `AESCCM`. The
/// cipher text and the tag. AES itself by CBC from a zero vector, a block at a time.
Future<(Uint8List, Uint8List)> sealKey(List<int> aesKey, List<int> nonce, List<int> text) async {
  if (aesKey.length != 16 || nonce.length != 13 || text.length != 16) {
    throw ArgumentError('a key of 16 B, a nonce of 13 B and a block of 16 B');
  }
  final cbc = AesCbc.with128bits(
      macAlgorithm: MacAlgorithm.empty, paddingAlgorithm: PaddingAlgorithm.zero);
  final key = SecretKey(aesKey);
  Future<Uint8List> run(List<int> data) async => Uint8List.fromList(
      (await cbc.encrypt(data, secretKey: key, nonce: Uint8List(16))).cipherText);
  // B0: the flags — the tag's 4 B as (4 − 2) / 2 in bits 5–3, the length's 2 B as 2 − 1 — then
  // the nonce and the text's length; the MAC is the CBC's last block over B0 and the text
  final mac = (await run([0x09, ...nonce, 0, 16, ...text])).sublist(16, 20);
  // the counter blocks A0 and A1: the flags 2 − 1, the nonce, the count
  final s0 = await run([0x01, ...nonce, 0, 0]);
  final s1 = await run([0x01, ...nonce, 0, 1]);
  return (
    Uint8List.fromList([for (var i = 0; i < 16; i++) text[i] ^ s1[i]]),
    Uint8List.fromList([for (var i = 0; i < 4; i++) mac[i] ^ s0[i]]),
  );
}

/// 64/19: [set] given to the Povodok of this connection as its number [n], 0–3, under a fresh lock
/// of its own, the password given again, the key sealed with the first 16 B of the secret that
/// lock agreed (COMMANDS.md). Povodok keeps the set and starts again in it.
Future<void> giveSet(SmpClient c, String password, int n, NewSet set, {Random? random}) async {
  if (n < 0 || n > 3) throw ArgumentError.value(n, 'n');
  final r = random ?? Random.secure();
  final challenge = await c.read(groupStrelka, Strelka.auth);
  final agreed = await agree(challenge['pub'] as Uint8List);
  final mac = await macFor(utf8.encode(password), challenge['nonce'] as Uint8List, agreed.secret);
  final nonce = Uint8List.fromList([for (var i = 0; i < 13; i++) r.nextInt(256)]);
  final (sealed, mic) = await sealKey(agreed.secret.sublist(0, 16), nonce, set.key);
  await c.write(groupStrelka, Strelka.give, body: {
    'pub': agreed.pub,
    'mac': mac,
    'n': n,
    'addr': Uint8List(4)..buffer.asByteData().setUint32(0, set.address, Endian.little),
    'ch': set.channel,
    'key': sealed,
    'nonce': nonce,
    'mic': mic,
    if (set.label != 0) 'label': set.label,
  });
}
