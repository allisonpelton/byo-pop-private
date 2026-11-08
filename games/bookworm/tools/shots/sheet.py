# sheet.py <out.png> <scale> <png>...: a contact sheet of shots (4 across, names under each), to check where a tour's
# clicks land before it is run on the original.
import os, sys
from PIL import Image, ImageDraw
out, scale, files = sys.argv[1], float(sys.argv[2]), sys.argv[3:]
ims = [Image.open(f).convert('RGB') for f in files]
w, h = int(ims[0].width * scale), int(ims[0].height * scale)
cols = min(4, len(ims)); rows = (len(ims) + cols - 1) // cols
sheet = Image.new('RGB', (cols * w, rows * (h + 14)), 'white')
d = ImageDraw.Draw(sheet)
for i, (f, im) in enumerate(zip(files, ims)):
    x, y = (i % cols) * w, (i // cols) * (h + 14)
    sheet.paste(im.resize((w, h)), (x, y))
    d.text((x + 2, y + h + 1), os.path.basename(f), fill='black')
sheet.save(out)
