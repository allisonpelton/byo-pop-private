#!/usr/bin/env python3
"""Writes the synthetic JPEGs in tools/jpegtest/data/ (Pillow, its bundled libjpeg(-turbo) as the encoder only: the
expected pixels are not Pillow's but libjpeg 6b's, in jpegtest.cpp). The picture is made here, not taken from a game:
smooth ramps, hard coloured edges and a little noise, 67x45 (odd, not a multiple of the 8 or 16 pixel blocks), so the
chroma upsampling is exercised at the edges, the DCT blocks at every quantiser step and the right/bottom padding.
Run: python tools/jpegtest/make_vectors.py   (rewrites the files; jpegtest -print then prints the new hashes)."""
import io, os, random
from PIL import Image

W, H = 67, 45
random.seed(20261107)
img = Image.new("RGB", (W, H))
px = img.load()
for y in range(H):
    for x in range(W):
        r = int(255 * x / (W - 1))
        g = int(255 * y / (H - 1))
        b = 128 + int(100 * ((x // 9 + y // 7) % 2)) - 50
        if 20 <= x < 40 and 10 <= y < 30:      # a hard-edged coloured block
            r, g, b = 250, 20, 30 + (x - 20) * 8
        if 45 <= x < 60 and y > 25:
            r, g, b = 10, 240, 20
        n = random.randint(-9, 9)
        px[x, y] = tuple(max(0, min(255, c + n)) for c in (r, g, b))

out = os.path.join(os.path.dirname(os.path.abspath(__file__)), "data")
os.makedirs(out, exist_ok=True)

def save(name, im, **kw):
    path = os.path.join(out, name + ".jpg")
    im.save(path, "JPEG", **kw)
    return open(path, "rb").read()

save("gray", img.convert("L"), quality=80)
save("c444", img, quality=85, subsampling=0)
save("c422", img, quality=85, subsampling=1)
save("c420", img, quality=85, subsampling=2)
save("c420_rst", img, quality=75, subsampling=2, restart_marker_blocks=3)
save("c420_prog", img, quality=75, subsampling=2, progressive=True)
save("gray_prog", img.convert("L"), quality=60, progressive=True)
save("cmyk", img.convert("CMYK"), quality=80)
# cut off in the middle of the entropy-coded data: libjpeg supplies the missing EOI and finishes the picture
d = open(os.path.join(out, "c420.jpg"), "rb").read()
open(os.path.join(out, "c420_cut.jpg"), "wb").write(d[:len(d) * 2 // 3])
# damaged: a header with a bad marker segment length
bad = bytearray(d)
bad[2:6] = b"\xff\xc0\x00\x02"
open(os.path.join(out, "bad_header.jpg"), "wb").write(bytes(bad))
print("wrote", sorted(os.listdir(out)))
