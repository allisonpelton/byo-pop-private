#!/usr/bin/env python3
"""A contact sheet of screenshots, for finding coordinates in the port's pictures: sheet.py <out.png> <scale> <cols> <png>..."""
import sys
from PIL import Image, ImageDraw
out, scale, cols = sys.argv[1], float(sys.argv[2]), int(sys.argv[3])
ims = [(p, Image.open(p).convert("RGB")) for p in sys.argv[4:]]
w, h = int(640 * scale), int(480 * scale)
rows = (len(ims) + cols - 1) // cols
sheet = Image.new("RGB", (cols * w, rows * (h + 14)), (255, 255, 255))
d = ImageDraw.Draw(sheet)
for i, (p, im) in enumerate(ims):
    x, y = (i % cols) * w, (i // cols) * (h + 14)
    sheet.paste(im.crop((0, 0, 640, 480)).resize((w, h)), (x, y + 14))
    d.text((x + 2, y + 1), p.replace("\\", "/").split("/")[-1], fill=(0, 0, 0))
sheet.save(out)
