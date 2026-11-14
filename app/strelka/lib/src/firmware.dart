/// The firmware's update (APP.md, What it must do, 13; FIRMWARE.md, The firmware's own updates):
/// the signed images fetched from the public repository's folder `firmware/images/` — gzipped,
/// beside a manifest of their version, sizes and SHA-256 (tools/strelka_images.py) — kept on the
/// phone, each checked by its size, its hash and its MCUboot header; then given to each unit by
/// MCUmgr's image group, piece by piece into its second slot, marked for a test and the unit
/// started again, the firmware confirming the new image once its start is whole.
library;

import 'dart:convert';
import 'dart:io';
import 'dart:typed_data';

import 'package:cryptography/cryptography.dart';

import 'smp.dart';

/// Where the images lie: the public repository's folder, its one commit the version's.
final Uri firmwareSource =
    Uri.parse('https://raw.githubusercontent.com/Project-NIC/NIC-Strelka/main/firmware/images/');

/// The manifest's name in the folder and on the phone.
const String manifestName = 'manifest.json';

/// The image kinds: the Belka's; Zvjozdočka's, today the Belka's own; Kryska's; Povodok's.
const List<String> imageKinds = ['belka', 'zvjozdocka', 'kryska', 'povodok'];

/// The image a unit takes by its 64/0: Povodok's, Kryska's, Zvjozdočka's or the Belka's
/// (COMMANDS.md, info's `who` and `kind`; tools/strelka_images.py, kind_of).
String imageKindOf(Map<String, Object?> info) {
  if (info['who'] == 0) return 'povodok';
  return switch (info['kind']) { 1 => 'kryska', 2 => 'zvjozdocka', _ => 'belka' };
}

/// The image for a unit by its 64/0 in [fw]; Zvjozdočka the Belka's where the manifest has none of
/// hers.
Uint8List? imageFor(FirmwareSet fw, Map<String, Object?> info) {
  final kind = imageKindOf(info);
  return fw.images[kind] ?? (kind == 'zvjozdocka' ? fw.images['belka'] : null);
}

/// A unit's firmware as 64/0 gives it, major.minor.patch; empty unknown.
String fwOf(Map<String, Object?> info) =>
    (info['fw'] as List<Object?>? ?? const []).map((v) => '$v').join('.');

const int _imageMagic = 0x96F3B83D; // MCUboot's image header (bootutil/image.h, IMAGE_MAGIC)

/// An MCUboot image's version from its header, major.minor.revision; null for no such image.
String? imageVersion(Uint8List image) {
  if (image.length < 32) return null;
  final d = ByteData.sublistView(image);
  if (d.getUint32(0, Endian.little) != _imageMagic) return null;
  return '${image[20]}.${image[21]}.${d.getUint16(22, Endian.little)}';
}

/// One image in the manifest: its file in the folder, and the image's own size and SHA-256.
class ManifestImage {
  const ManifestImage(this.file, this.size, this.sha256);

  final String file;
  final int size;
  final String sha256;
}

/// The manifest: the version the images carry, and each kind's image.
class FirmwareManifest {
  FirmwareManifest(this.version, this.images);

  factory FirmwareManifest.parse(String json) {
    final m = jsonDecode(json) as Map<String, Object?>;
    final images = <String, ManifestImage>{};
    for (final e in (m['images'] as Map<String, Object?>? ?? const {}).entries) {
      final i = e.value as Map<String, Object?>;
      images[e.key] = ManifestImage(i['file'] as String, i['size'] as int, i['sha256'] as String);
    }
    return FirmwareManifest(m['version'] as String, images);
  }

  final String version;
  final Map<String, ManifestImage> images;
}

/// Why an image was refused: a size, a hash or a header other than the manifest says.
class FirmwareMismatch implements Exception {
  const FirmwareMismatch(this.kind, this.why);

  final String kind, why;

  @override
  String toString() => 'the $kind image: $why';
}

/// The firmware kept on the phone: the manifest and each kind's image, checked.
class FirmwareSet {
  FirmwareSet(this.manifest, this.images);

  final FirmwareManifest manifest;
  final Map<String, Uint8List> images;

  String get version => manifest.version;
}

String _hex(List<int> b) => [for (final x in b) x.toRadixString(16).padLeft(2, '0')].join();

/// [image] held to the manifest's entry for [kind]: its size, its SHA-256 and its header's version.
Future<void> checkImage(FirmwareManifest m, String kind, Uint8List image) async {
  final e = m.images[kind]!;
  if (image.length != e.size) {
    throw FirmwareMismatch(kind, '${image.length} B, not ${e.size}');
  }
  final sha = _hex((await Sha256().hash(image)).bytes);
  if (sha != e.sha256.toLowerCase()) throw FirmwareMismatch(kind, 'its SHA-256 other');
  final v = imageVersion(image);
  if (v != m.version) throw FirmwareMismatch(kind, 'version $v, not ${m.version}');
}

/// The folder's manifest and images fetched from [source] — GitHub's raw files by default — each
/// gunzipped and checked, then kept in [dir]: the manifest last, so that a fetch cut short leaves
/// the last whole one. Null where the folder holds no manifest yet.
Future<FirmwareSet?> fetchFirmware(Directory dir, {Uri? source, HttpClient? http}) async {
  final from = source ?? firmwareSource;
  final client = http ?? HttpClient();
  Future<Uint8List?> get(String name) async {
    final rq = await client.getUrl(from.resolve(name));
    final rs = await rq.close();
    final body = BytesBuilder(copy: false);
    await rs.forEach(body.add);
    if (rs.statusCode == HttpStatus.notFound) return null;
    if (rs.statusCode != HttpStatus.ok) throw HttpException('$name: ${rs.statusCode}');
    return body.takeBytes();
  }

  try {
    final raw = await get(manifestName);
    if (raw == null) return null;
    final m = FirmwareManifest.parse(utf8.decode(raw));
    final images = <String, Uint8List>{};
    final files = <String, Uint8List>{};            // a file two kinds share fetched once
    for (final e in m.images.entries) {
      var image = files[e.value.file];
      if (image == null) {
        final gz = await get(e.value.file);
        if (gz == null) throw FirmwareMismatch(e.key, '${e.value.file} missing');
        image = files[e.value.file] = Uint8List.fromList(gzip.decode(gz));
      }
      await checkImage(m, e.key, image);
      images[e.key] = image;
    }
    await dir.create(recursive: true);
    for (final e in images.entries) {
      await File('${dir.path}/${e.key}.bin').writeAsBytes(e.value, flush: true);
    }
    await File('${dir.path}/$manifestName').writeAsBytes(raw, flush: true);
    return FirmwareSet(m, images);
  } finally {
    if (http == null) client.close();
  }
}

/// The firmware kept in [dir], each image checked again; null where none is.
Future<FirmwareSet?> keptFirmware(Directory dir) async {
  final f = File('${dir.path}/$manifestName');
  if (!await f.exists()) return null;
  final m = FirmwareManifest.parse(await f.readAsString());
  final images = <String, Uint8List>{};
  for (final kind in m.images.keys) {
    final image = await File('${dir.path}/$kind.bin').readAsBytes();
    await checkImage(m, kind, image);
    images[kind] = image;
  }
  return FirmwareSet(m, images);
}

/// The data a piece of an upload carries: under an SMP frame through Povodok to a Belka.
const int uploadPiece = 512;

/// [image] into the unit's second slot by MCUmgr's image group, 1/1, piece by piece from the offset
/// the unit gives, its length and SHA-256 with the first; then marked for a test by its hash, 1/0,
/// and the unit started again, 0/5 — the new image runs and confirms itself at a whole start, or
/// MCUboot takes the old one back at the next (FIRMWARE.md, The firmware's own updates).
/// [onProgress] the bytes the unit has.
Future<void> updateUnit(SmpClient c, Uint8List image,
    {void Function(int done, int total)? onProgress}) async {
  final sha = Uint8List.fromList((await Sha256().hash(image)).bytes);
  var off = 0;
  while (off < image.length) {
    final end = off + uploadPiece < image.length ? off + uploadPiece : image.length;
    final rsp = await c.write(groupImg, 1, body: {
      if (off == 0) ...{'image': 0, 'len': image.length, 'sha': sha},
      'off': off,
      'data': Uint8List.sublistView(image, off, end),
    });
    final next = rsp['off'] as int;
    if (next == off) throw const SmpError(Rc.corrupt);   // no progress: the unit takes it not
    off = next;
    onProgress?.call(off, image.length);
  }
  final state = await c.read(groupImg, 0);
  Uint8List? hash;
  for (final s in state['images'] as List<Object?>? ?? const []) {
    final m = s as Map<Object?, Object?>;
    if (m['slot'] == 1) hash = m['hash'] as Uint8List?;
  }
  if (hash == null) throw const SmpError(Rc.noEnt);
  await c.write(groupImg, 0, body: {'hash': hash, 'confirm': false});
  try {
    await c.write(groupOs, 5);
  } on Exception {
    // it went to restart before it answered
  }
}
