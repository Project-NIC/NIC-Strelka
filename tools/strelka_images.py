"""NIC-Strelka — the firmware's images for the application: the signed images MCUboot takes — the
Belka's with the FLPR's program inside it (firmware/flpr/merge.py); Zvjozdočka's, today the
Belka's own, its entry the Belka's file then; Kryska's; Povodok's — gzipped into one folder beside a
manifest, firmware/images/ in the public repository, where Pušinka fetches them (APP.md, What it
must do, 13; firmware/images/README.md).

    python3 tools/strelka_images.py <folder> belka=<signed.bin> zvjozdocka=<signed.bin> \
        kryska=<signed.bin> povodok=<signed.bin>

manifest.json: the version the images carry in their MCUboot headers, and for each kind its gzipped
file and the image's own size and SHA-256 — the image's, not the file's — so that the application
checks what it will send; Zvjozdočka given none, or the Belka's very image, takes the Belka's file.
The images are signed by the maker's key; the development key MCUboot builds with when none is
given is public, and its images are never put there.
"""

import gzip
import hashlib
import json
import os
import struct
import sys

KINDS = ("belka", "zvjozdocka", "kryska", "povodok")
MAGIC = 0x96F3B83D                        # MCUboot's image header (bootutil/image.h, IMAGE_MAGIC)
# magic, load address, the header's size, the protected TLVs' size, the image's size, flags, then
# the version: major, minor, revision, build
HEADER = struct.Struct("<IIHHIIBBHI")
MANIFEST = "manifest.json"


def version_of(image):
    """An MCUboot image's version, major.minor.revision, from its header."""
    magic, _, _, _, _, _, major, minor, revision, _ = HEADER.unpack_from(image)
    if magic != MAGIC:
        raise ValueError("not an MCUboot image")
    return "%d.%d.%d" % (major, minor, revision)


def kind_of(info):
    """The image a unit takes by its 64/0: Povodok's, Kryska's, Zvjozdočka's or the Belka's
    (COMMANDS.md, info's `who` and `kind`)."""
    if info.get("who") == 0:
        return "povodok"
    return {1: "kryska", 2: "zvjozdocka"}.get(info.get("kind"), "belka")


def fake_image(version, size=4096, fill=0x5A):
    """An image with MCUboot's header and a body, unsigned: the tests' and the simulator's."""
    major, minor, revision = (int(x) for x in version.split("."))
    head = HEADER.pack(MAGIC, 0, 32, 0, size, 0, major, minor, revision, 0).ljust(32, b"\x00")
    return head + bytes((fill + i) & 0xFF for i in range(size))


def write(folder, images):
    """The folder of {kind: image}: each gzipped, the manifest beside them; every image one version."""
    versions = {version_of(b) for b in images.values()}
    if len(versions) != 1:
        raise ValueError("the images carry different versions: %s" % sorted(versions))
    os.makedirs(folder, exist_ok=True)
    manifest = {"version": versions.pop(), "images": {}}
    if "belka" in images and images.get("zvjozdocka", images["belka"]) == images["belka"]:
        images = dict(images, zvjozdocka=images["belka"])     # the Belka's, one file for both
    files = {}
    for kind in KINDS:
        if kind not in images:
            continue
        data = images[kind]
        name = files.get(data) or kind + ".bin.gz"
        if data not in files:
            files[data] = name
            with open(os.path.join(folder, name), "wb") as f:
                f.write(gzip.compress(data, 9, mtime=0))  # the same bytes for the same image
        manifest["images"][kind] = dict(file=name, size=len(data),
                                        sha256=hashlib.sha256(data).hexdigest())
    with open(os.path.join(folder, MANIFEST), "w") as f:
        json.dump(manifest, f, indent=2)
        f.write("\n")
    return manifest


def main(argv):
    if len(argv) < 3:
        raise SystemExit(__doc__)
    images = {}
    for arg in argv[2:]:
        kind, _, path = arg.partition("=")
        if kind not in KINDS:
            raise SystemExit("one of %s=<signed.bin>" % ", ".join(KINDS))
        with open(path, "rb") as f:
            images[kind] = f.read()
    print(json.dumps(write(argv[1], images), indent=2))


if __name__ == "__main__":
    main(sys.argv)
