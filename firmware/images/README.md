# The firmware's images

The signed images the units take, for Pušinka to fetch from this folder of the public repository
and give to the units through Povodok ([`APP.md`](../../APP.md), *What it must do*, 13;
[`FIRMWARE.md`](../../FIRMWARE.md), *The firmware's own updates*). The folder holds the version
the repository's one commit is, and no other.

| file | what |
|---|---|
| `manifest.json` | the version, and for each image its file, its size and its SHA-256 — the image's own, not the gzipped file's |
| `belka.bin.gz` | the Belka's image, the FLPR's program inside it (`firmware/flpr/merge.py`) |
| `zvjozdocka.bin.gz` | Zvjozdočka's, her entry in the manifest naming the Belka's file while her image is the Belka's own |
| `kryska.bin.gz` | Kryska's |
| `povodok.bin.gz` | Povodok's |

Each is MCUboot's signed image, gzipped; its header carries the version, which the application
holds to the manifest's, and the unit checks its signature before it swaps. They are written by
`tools/strelka_images.py` from the three builds:

    python3 tools/strelka_images.py firmware/images belka=<belka_flpr.signed.bin> \
        [zvjozdocka=<its signed.bin>] kryska=<zephyr.signed.bin> povodok=<zephyr.signed.bin>

**Signed by the maker's key, never by MCUboot's development key**, which is public: a build without
a key of its own is signed by that one and stays out of this folder. Until the first images signed
so, the folder holds this page alone, and the application says that none is published.
