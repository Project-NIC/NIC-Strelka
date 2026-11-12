# The application's base

Pušinka ([`APP.md`](../APP.md)) is to run on Android, iPhone, Windows, macOS and Linux from one
code, so its base is written in Flutter, whose one code base builds for all five
([Flutter, Multi-platform](https://flutter.dev/multi-platform)). Two packages:

| package | what | checked by |
|---|---|---|
| [`strelka/`](strelka/) | the protocol in Dart alone, no Flutter in it: SMP over TCP with the lock ([`COMMANDS.md`](../COMMANDS.md)), the live view's messages over UDP ([`PHONE.md`](../PHONE.md)), the live view saved and made into a C3D ([`APP.md`](../APP.md), *The live view saved*); a new set drawn and given to a Povodok by 64/19, its key sealed by AES-CCM, and its `STRELKA.SET`; the firmware's images fetched, checked and given to a unit by MCUmgr's image group | `dart analyze`, `dart test`: the tests hold it to `tools/smp.py`, `tools/live_view.py` and `tools/strelka_live.py` by their vectors, and drive the Povodok simulator, `tools/povodok_sim.py`, through the lock, the record, a Belka's port and the live view |
| [`pusinka/`](pusinka/) | the application: Povodok joined through the lock, its own key pinned, the password kept in the phone's keystore, then the six menus along the bottom in the look "Volkov" — the set's overview and a unit's page; the calibration, the six faces asked of every unit; the measurement's choice with the synchronisation and the start, and the live view, its overview, curves and numbers, left against right, saved at a touch into a C3D; the data manager, the download into the cards' own files and one C3D, a C3D into CSV, the files named by the date, the session and the user's note; the diagnostics, the band and the units as Povodok knows them, the set moved by hand; the settings, a new set drawn on the map of the band and given to each Povodok, its file saved for a Belka on its cable, the firmware fetched and every unit updated, the language of the seven, the measurement's, the places' templates, new units, the password, Povodok switched off, About; the texts a table a language, `lib/l10n/app_<lang>.arb` | `flutter analyze`, `flutter test`: widget tests of every menu, the first joining against the simulator, the seven languages and the settings' rules |

Beside them, [`mockups/`](mockups/): the screens' sketches of [`APP.md`](../APP.md), *The menus* —
each menu's HTML, its texts a table a language, and `render.js`, which draws them as pictures by
Playwright's Chromium, `node app/mockups/render.js en`, the pictures into `mockups/en/`.

The Python tools stay the reference: what the Dart does, they did first, and the simulator holds
both to the same bytes.

## The checks

    cd app/strelka && dart pub get && dart analyze --fatal-infos && dart test
    cd app/pusinka && flutter pub get && flutter analyze --fatal-infos && flutter test

The simulator's test wants Python 3 with `cryptography` and `cbor2`, as `tools/` does. The
packages used: `cbor` for SMP's bodies ([pub.dev](https://pub.dev/packages/cbor)) and
`cryptography` for X25519, SHA-256 and HMAC in Dart alone
([pub.dev](https://pub.dev/packages/cryptography)); in the application `flutter_secure_storage` for
the phone's keystore ([pub.dev](https://pub.dev/packages/flutter_secure_storage)), and Material's
and Cupertino's widgets and texts from `material_ui` and `cupertino_ui`, where Flutter has moved
them out of its SDK ([pub.dev](https://pub.dev/packages/material_ui),
[pub.dev](https://pub.dev/packages/cupertino_ui)): Flutter 3.47 at the least.

## To run it

The platform folders are not kept here; Flutter makes them, with an id of the maker's own:

    cd app/pusinka
    flutter create --platforms=android,ios,windows,macos,linux --org <id> .
    flutter run -d linux

Or `windows`, `macos`, a phone plugged in. Not the web: a browser has no TCP and UDP sockets of
its own. Povodok on its cable is at 192.168.7.1 ([`PHONE.md`](../PHONE.md)), the password from
the factory `strelka1` ([`COMMANDS.md`](../COMMANDS.md), *The lock*).

## What is not here yet

- The animal's drawing in the live view's overview, as [`APP.md`](../APP.md) has it and its sketch
  in [`mockups/`](mockups/) shows.
- The firmware's images themselves: the update is written, fetched from
  [`firmware/images/`](../firmware/images/), but no image is there until the first signed by the
  maker's key.
- `STRELKA.SET` written onto a Belka's disk from the phone: the new set's file is saved on the
  phone, in its folder `Strelka/set/`, and copied onto the disk by hand until a plugin opens a
  Belka's disk over USB from the phone, chosen with a phone and a Belka on the desk.
- What Flutter does not carry itself and each platform does its own way ([`APP.md`](../APP.md),
  *The link*): on Android the wired network asked for and the sockets bound to it; on iPhone the
  wired interface and the local network's permission; Bluetooth LE for the Wi-Fi's name and
  password in Povodok's first minutes. Each is a plugin from pub.dev, or a few lines of the
  platform's own code called from Dart.
- Nothing has run against a Povodok: there is none yet. The simulator is what it was run
  against, and the Flutter screens have been analysed and tested as widgets, built for no device.
