# The application's base

Pušinka ([`APP.md`](../APP.md)) is to run on Android, iPhone, Windows, macOS and Linux from one
code, so its base is written in Flutter, whose one code base builds for all five
([Flutter, Multi-platform](https://flutter.dev/multi-platform)). Two packages:

| package | what | checked by |
|---|---|---|
| [`strelka/`](strelka/) | the protocol in Dart alone, no Flutter in it: SMP over TCP with the lock ([`COMMANDS.md`](../COMMANDS.md)), the live view's messages over UDP ([`PHONE.md`](../PHONE.md)) | `dart analyze`, `dart test`: the tests hold it to `tools/smp.py` and `tools/live_view.py` by their vectors, and drive the Povodok simulator, `tools/povodok_sim.py`, through the lock, the record, a Belka's port and the live view |
| [`pusinka/`](pusinka/) | the application: a screen joining Povodok through the lock, a screen with the set and its live view | `flutter analyze`, `flutter test`: a widget test of the first screen |

The Python tools stay the reference: what the Dart does, they did first, and the simulator holds
both to the same bytes.

## The checks

    cd app/strelka && dart pub get && dart analyze --fatal-infos && dart test
    cd app/pusinka && flutter pub get && flutter analyze --fatal-infos && flutter test

The simulator's test wants Python 3 with `cryptography` and `cbor2`, as `tools/` does. The
packages used: `cbor` for SMP's bodies ([pub.dev](https://pub.dev/packages/cbor)) and
`cryptography` for X25519, SHA-256 and HMAC in Dart alone
([pub.dev](https://pub.dev/packages/cryptography)).

## To run it

The platform folders are not kept here; Flutter makes them, with an id of the maker's own:

    cd app/pusinka
    flutter create --platforms=android,ios,windows,macos,linux --org <id> .
    flutter run -d linux

Or `windows`, `macos`, a phone plugged in. Not the web: a browser has no TCP and UDP sockets of
its own. Povodok on its cable is at 192.168.7.1 ([`PHONE.md`](../PHONE.md)), the password from
the factory `strelka1` ([`COMMANDS.md`](../COMMANDS.md), *The lock*).

## What is not here yet

- The menus of [`APP.md`](../APP.md) beyond the first two screens: the Belkas, the calibration,
  the live curves, the record's settings, the download, the export, the settings.
- What Flutter does not carry itself and each platform does its own way ([`APP.md`](../APP.md),
  *The link*): on Android the wired network asked for and the sockets bound to it; on iPhone the
  wired interface and the local network's permission; Bluetooth LE for the Wi-Fi's name and
  password in Povodok's first minutes. Each is a plugin from pub.dev, or a few lines of the
  platform's own code called from Dart.
- Nothing has run against a Povodok: there is none yet. The simulator is what it was run
  against, and the Flutter screens have been analysed and tested as widgets, built for no device.
