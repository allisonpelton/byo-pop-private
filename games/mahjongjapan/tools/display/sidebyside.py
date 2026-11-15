# sidebyside.py <left.png> <right.png> <out.png> <x> <y> <w> <h> <magnify>: the same part (x, y, w, h) of two captures of one screen, each repeated
# <magnify> times each way (nearest, so that no pixel is blended), the left one beside the right one with a white line between them.
import importlib.util, os, sys

here = os.path.dirname(os.path.abspath(__file__))


def load(name):
    spec = importlib.util.spec_from_file_location(name, os.path.join(here, name + '.py'))
    mod = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(mod)
    return mod


wincap = load('wincap')
fullcap = load('fullcap')


def crop(path, x, y, w, h):
    iw, ih, ch, px = wincap.read_png(path)
    rows = []
    for yy in range(y, y + h):
        rows.append(px[(yy * iw + x) * ch:(yy * iw + x + w) * ch])
    return rows, ch


def magnify(rows, ch, n):
    out = []
    for row in rows:
        line = bytearray()
        for i in range(0, len(row), ch):
            line += row[i:i + 3] * n
        for _ in range(n):
            out.append(bytes(line))
    return out


def main():
    left, right, out = sys.argv[1], sys.argv[2], sys.argv[3]
    x, y, w, h, n = [int(v) for v in sys.argv[4:9]]
    a, cha = crop(left, x, y, w, h)
    b, chb = crop(right, x, y, w, h)
    a = magnify(a, cha, n)
    b = magnify(b, chb, n)
    bar = bytes([255, 255, 255]) * 6
    bgra = bytearray()
    for ra, rb in zip(a, b):
        line = ra + bar + rb
        for i in range(0, len(line), 3):
            bgra += bytes([line[i + 2], line[i + 1], line[i], 255])
    total_w = len(a[0]) // 3 + 6 + len(b[0]) // 3
    fullcap.write_png(out, total_w, len(a), bytes(bgra), total_w)
    print('wrote %s: %dx%d, left %s, right %s' % (out, total_w, len(a), os.path.basename(left), os.path.basename(right)))


if __name__ == '__main__':
    main()
