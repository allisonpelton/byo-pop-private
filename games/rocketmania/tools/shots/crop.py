# crop.py <tour> <shot> x y w h [out.png]: the original's and the port's best frame of a shot side by side, enlarged twice, with the
# difference (tripled) at the right, for a box that shotdiff lists as faint (it writes no crop for those). Uses build/ph2/<tour>/orig and
# build/ph2/<tour>all/best, and the client crop of shotdiff (origrun's window frame cut away).
import sys, os
import numpy as np
from PIL import Image
tour, shot, x, y, w, h = sys.argv[1], sys.argv[2], *[int(v) for v in sys.argv[3:7]]
out = sys.argv[7] if len(sys.argv) > 7 else 'build/ph2/crop_%s.png' % shot
o = Image.open('build/ph2/%s/orig/%s.png' % (tour, shot)).convert('RGB')
p = Image.open('build/ph2/%sall/best/%s.png' % (tour, shot)).convert('RGB')
if o.size == (646, 509):
    o = o.crop((3, 26, 643, 506))
a = np.asarray(o.crop((x, y, x + w, y + h))).astype(int)
b = np.asarray(p.crop((x, y, x + w, y + h))).astype(int)
d = np.clip(np.abs(a - b).max(axis=2) * 3, 0, 255).astype('uint8')
d = np.stack([d, d, d], axis=2)
row = np.concatenate([a.astype('uint8'), b.astype('uint8'), d], axis=1)
Image.fromarray(row).resize((row.shape[1] * 2, row.shape[0] * 2), Image.NEAREST).save(out)
print(out)
