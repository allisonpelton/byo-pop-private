# pngmatch.py <shot.png> <dir with c*.png>: which of a folder's screenshots (c<update>.png) a shot equals pixel for pixel, with the
# differing-pixel count of every one
import glob, importlib.util, os, re, sys

here = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', 'display')
spec = importlib.util.spec_from_file_location('wincap', os.path.join(here, 'wincap.py'))
wincap = importlib.util.module_from_spec(spec)
spec.loader.exec_module(wincap)

w, h, ch, px = wincap.read_png(sys.argv[1])
rows = []
for path in sorted(glob.glob(os.path.join(sys.argv[2], 'c*.png')), key=lambda p: int(re.findall(r'c(\d+)\.png', p)[0])):
    w2, h2, ch2, px2 = wincap.read_png(path)
    diff = sum(1 for i in range(w * h) if px[i * ch:i * ch + 3] != px2[i * ch2:i * ch2 + 3])
    rows.append((re.findall(r'c(\d+)\.png', path)[0], diff))
print(sys.argv[1] + ': ' + ' '.join('%s=%d' % r for r in rows))
print('equal to: ' + (', '.join(u for u, d in rows if d == 0) or 'none'))
