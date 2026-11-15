# pngdiff.py <a.png> <b.png>: the number of pixels that differ between two screenshots of one size (and the first difference)
import importlib.util, os, sys

here = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', 'display')
spec = importlib.util.spec_from_file_location('wincap', os.path.join(here, 'wincap.py'))
wincap = importlib.util.module_from_spec(spec)
spec.loader.exec_module(wincap)

aw, ah, ach, apx = wincap.read_png(sys.argv[1])
bw, bh, bch, bpx = wincap.read_png(sys.argv[2])
if (aw, ah) != (bw, bh):
    print('sizes differ: %dx%d against %dx%d' % (aw, ah, bw, bh))
    sys.exit(1)
diff, first = 0, None
for i in range(aw * ah):
    a = apx[i * ach:i * ach + 3]
    b = bpx[i * bch:i * bch + 3]
    if a != b:
        diff += 1
        if first is None:
            first = (i % aw, i // aw)
print('%d of %d pixels differ%s' % (diff, aw * ah, (', first at %s' % (first,)) if first else ''))
