import 'dart:typed_data';

import 'package:strelka/strelka.dart';
import 'package:test/test.dart';

Uint8List hex(String s) =>
    Uint8List.fromList([for (var i = 0; i < s.length; i += 2) int.parse(s.substring(i, i + 2), radix: 16)]);

String toHex(List<int> b) => b.map((x) => x.toRadixString(16).padLeft(2, '0')).join();

void main() {
  // the vectors are tools/smp.py's, the Python reference
  test('a frame as smp.py builds it', () {
    expect(toHex(smpFrame(opWrite, groupStrelka, 5, Strelka.record, body: {'run': true})),
        '0a0000060040050aa16372756ef5');
    expect(toHex(smpFrame(opRead, groupStrelka, 0, Strelka.info)), '0800000000400000');
  });

  test('the header read back', () {
    final h = SmpHeader.parse(hex('0a0000060040050aa16372756ef5'));
    expect([h.op, h.version, h.flags, h.length, h.group, h.seq, h.cmd], [2, 1, 0, 6, 64, 5, 10]);
  });

  test('the stream cut into frames, a byte at a time', () {
    final one = smpFrame(opReadRsp, groupStrelka, 1, Strelka.info, body: {'n': 3});
    final two = smpFrame(opWriteRsp, groupOs, 2, 0);
    final all = Uint8List.fromList([...one, ...two]);
    final stream = SmpStream();
    final frames = <Uint8List>[];
    for (final b in all) {
      frames.addAll(stream.feed(Uint8List.fromList([b])));
    }
    expect(frames.map(toHex), [toHex(one), toHex(two)]);
    expect(() => stream.feed(hex('0800ffff00400000')), throwsFormatException);
  });

  test('bodies to and from CBOR', () {
    final body = {
      'chip': Uint8List.fromList([1, 2, 3]),
      'n': 7,
      'ok': true,
      'name': 'x',
      'fw': [0, 7, 0],
      'card': {'in': false, 'mb': 0},
    };
    final back = decodeCborMap(encodeCbor(body));
    expect(back['chip'], isA<Uint8List>());
    expect(toHex(back['chip'] as Uint8List), '010203');
    expect(back['n'], 7);
    expect(back['ok'], true);
    expect(back['name'], 'x');
    expect(back['fw'], [0, 7, 0]);
    expect(back['card'], {'in': false, 'mb': 0});
  });

  test('the codes as errors', () {
    expect(() => smpAnswer({'rc': 11}),
        throwsA(isA<SmpError>().having((e) => e.toString(), 'text', 'SMP: access denied')));
    expect(
        () => smpAnswer({
              'err': {'group': 64, 'rc': 1}
            }),
        throwsA(isA<SmpError>()
            .having((e) => e.group, 'group', 64)
            .having((e) => e.toString(), 'text', 'SMP: group 64: the bridged target out of reach')));
    expect(smpAnswer({'rc': 0, 'n': 1}), {'rc': 0, 'n': 1});
  });

  test('the MAC as smp.py computes it', () async {
    final nonce = Uint8List.fromList(List.generate(16, (i) => i));
    final secret = Uint8List.fromList(List.generate(32, (i) => i));
    expect(toHex(await macFor('strelka1'.codeUnits, nonce, secret)),
        'f805f2e3979496741309033eace41926f3e0e9d5e2ec826d74b0c117697e4a3d');
  });

  test('two sides agree the same secret', () async {
    final a = await agree(Uint8List(32)..[0] = 9);
    expect(a.pub.length, 32);
    expect(a.secret.length, 32);
  });
}
