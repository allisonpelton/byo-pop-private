# fullcap.py <title part> <out.png> <seconds> [screen|window] [crop x y w h]: waits for the game's window (SDL's class, the title part in its
# title), then <seconds> after that captures its client area as a PNG: 'screen' (the default) as the desktop shows it, BitBlt from the screen's
# DC, which reads a window that has had a Direct3D swap chain (the visible desktop only, at night: CLAUDE.md); 'window' by PrintWindow, which
# reads a GDI window on rundesk's desktop and answers black for a window that has had a swap chain. A crop (client pixels) writes only that
# part, at 1:1. Prints the client size and the number of distinct colours (a black capture has one).
import importlib.util, os, struct, sys, time, zlib

here = os.path.dirname(os.path.abspath(__file__))
spec = importlib.util.spec_from_file_location('wincap', os.path.join(here, 'wincap.py'))
wincap = importlib.util.module_from_spec(spec)
spec.loader.exec_module(wincap)


def write_png(path, w, h, bgra, stride_w):
    raw = bytearray()
    for y in range(h):
        row = bgra[y * stride_w * 4:(y * stride_w + w) * 4]
        line = bytearray(w * 3)
        line[0::3] = row[2::4]
        line[1::3] = row[1::4]
        line[2::3] = row[0::4]
        raw += b'\0' + line

    def chunk(t, d):
        c = struct.pack('>I', len(d)) + t + d
        return c + struct.pack('>I', zlib.crc32(t + d) & 0xffffffff)
    with open(path, 'wb') as f:
        f.write(b'\x89PNG\r\n\x1a\n' + chunk(b'IHDR', struct.pack('>IIBBBBB', w, h, 8, 2, 0, 0, 0)) + chunk(b'IDAT', zlib.compress(bytes(raw), 6)) + chunk(b'IEND', b''))


def main():
    part, out, seconds = sys.argv[1], sys.argv[2], float(sys.argv[3])
    how = sys.argv[4] if len(sys.argv) > 4 else 'screen'
    crop = [int(v) for v in sys.argv[6:10]] if len(sys.argv) > 9 and sys.argv[5] == 'crop' else None
    t0 = time.time()
    hwnd = None
    while hwnd is None:
        hwnd = wincap.find(part)
        if hwnd is None:
            if time.time() - t0 > 120:
                sys.exit('no window')
            time.sleep(0.2)
    time.sleep(seconds)
    if how == 'screen':
        cw, chh, buf = wincap.capture_screen(hwnd)
        ox, oy, stride_w = 0, 0, cw
    else:
        ok, cw, chh, buf, ww, ox, oy = wincap.capture(hwnd)
        stride_w = ww
    # the client area as BGRA rows
    rows = bytearray()
    for y in range(chh):
        o = ((y + oy) * stride_w + ox) * 4
        rows += buf[o:o + cw * 4]
    x0, y0, w, h = (0, 0, cw, chh) if crop is None else crop
    sub = bytearray()
    for y in range(y0, y0 + h):
        sub += rows[(y * cw + x0) * 4:(y * cw + x0 + w) * 4]
    write_png(out, w, h, bytes(sub), w)
    colours = len(set(bytes(sub[i:i + 4]) for i in range(0, min(len(sub), 4 * 200000), 4)))
    print('client %dx%d, wrote %dx%d, %d distinct colours in the first 200000 pixels' % (cw, chh, w, h, colours))


if __name__ == '__main__':
    main()
