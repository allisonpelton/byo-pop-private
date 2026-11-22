"""sheet.py <out.png> <cols> <scale> files... : a contact sheet of shots, labelled with their file names"""
import sys
from PIL import Image, ImageDraw
out, cols, sc = sys.argv[1], int(sys.argv[2]), float(sys.argv[3])
fs = sys.argv[4:]
ims = [Image.open(f).convert('RGB') for f in fs]
w = int(ims[0].width * sc); h = int(ims[0].height * sc)
rows = (len(ims) + cols - 1) // cols
sheet = Image.new('RGB', (cols * w, rows * (h + 14)), (40, 40, 40))
d = ImageDraw.Draw(sheet)
for i, (f, im) in enumerate(zip(fs, ims)):
    x, y = (i % cols) * w, (i // cols) * (h + 14)
    sheet.paste(im.resize((w, h)), (x, y + 14))
    d.text((x + 2, y + 1), f.split('/')[-1], fill=(255, 255, 0))
sheet.save(out)
