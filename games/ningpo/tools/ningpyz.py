#!/usr/bin/env python3
"""Read Ning Po Mahjong Deluxe's data archive (ningpo.pyz).

The archive is an ordinary ZIP file (PKZIP 2.0, no encryption, no comment): 235 entries, stored or deflated,
with bare names and no folders ("tileset_1.tga", "classic.nml", "theme_np.ogg"). The game reads it with the
minizip 0.15 and zlib 1.1.3 linked into Ningpo.exe and looks names up without regard to case. It holds:

    *.tga   148 Truevision Targa images, uncompressed and bottom-up: 130 of 32 bits (8 of alpha), 16 of 16 bits
                (1-5-5-5, the top bit the alpha), 2 of 24 bits
    *.jpg    58 JPEG images (the backgrounds, the screens, the layout previews)
    *.png     1 PNG, never loaded (carefulonemoveleft_np.tga is the one the game uses)
    *.nml    18 layouts: one tile a line, "x,y,z" in half-tile units
    *.ogg     7 Ogg Vorbis (the music and most sounds)
    *.wav     3 PCM WAV (the clicks)

Usage:
    ningpyz.py list <ningpo.pyz>
    ningpyz.py extract <ningpo.pyz> <out dir> [name ...]     # all files, or only those named
    ningpyz.py cat <ningpo.pyz> <name>                       # one file to stdout
"""
import os
import sys
import zipfile


def find(z, name):
    for info in z.infolist():
        if info.filename.lower() == name.lower():
            return info
    sys.exit(f"{name}: not in the archive")


def main():
    if len(sys.argv) < 3 or sys.argv[1] not in ("list", "extract", "cat"):
        sys.exit(__doc__)
    z = zipfile.ZipFile(sys.argv[2])
    if sys.argv[1] == "list":
        for info in z.infolist():
            method = "deflate" if info.compress_type == zipfile.ZIP_DEFLATED else "stored"
            print(f"{info.file_size:9d} {info.compress_size:9d} {method:7s} {info.filename}")
    elif sys.argv[1] == "extract":
        out = sys.argv[3]
        names = sys.argv[4:]
        os.makedirs(out, exist_ok=True)
        infos = [find(z, n) for n in names] if names else z.infolist()
        for info in infos:
            with open(os.path.join(out, os.path.basename(info.filename)), "wb") as f:
                f.write(z.read(info))
        print(f"extracted {len(infos)} files into {out}")
    else:
        sys.stdout.buffer.write(z.read(find(z, sys.argv[3])))


if __name__ == "__main__":
    main()
