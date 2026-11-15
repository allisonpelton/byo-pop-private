# wincap.py <title part> <png the game wrote> [<out.txt>]: waits for the game's screenshot file, then captures the game window's
# client area as the system shows it (PrintWindow with PW_RENDERFULLCONTENT, on the desktop this runs on, so run it under rundesk
# beside the game), and compares its pixels with the screenshot's. Prints one line: the sizes and how many pixels differ.
import ctypes, ctypes.wintypes as W, os, struct, sys, time, zlib
user32 = ctypes.WinDLL('user32', use_last_error=True)
gdi32 = ctypes.WinDLL('gdi32', use_last_error=True)
P = ctypes.c_void_p
for f, args, res in ((user32.GetDC, [P], P), (user32.ReleaseDC, [P, P], ctypes.c_int), (user32.PrintWindow, [P, P, ctypes.c_uint], W.BOOL),
                     (user32.GetWindowRect, [P, P], W.BOOL), (user32.GetClientRect, [P, P], W.BOOL), (user32.ClientToScreen, [P, P], W.BOOL),
                     (user32.IsWindowVisible, [P], W.BOOL), (user32.GetWindowTextLengthW, [P], ctypes.c_int),
                     (user32.GetWindowTextW, [P, P, ctypes.c_int], ctypes.c_int),
                     (gdi32.CreateCompatibleDC, [P], P), (gdi32.CreateCompatibleBitmap, [P, ctypes.c_int, ctypes.c_int], P),
                     (gdi32.SelectObject, [P, P], P), (gdi32.DeleteObject, [P], W.BOOL), (gdi32.DeleteDC, [P], W.BOOL),
                     (gdi32.GetDIBits, [P, P, ctypes.c_uint, ctypes.c_uint, P, P, ctypes.c_uint], ctypes.c_int),
                     (gdi32.BitBlt, [P, ctypes.c_int, ctypes.c_int, ctypes.c_int, ctypes.c_int, P, ctypes.c_int, ctypes.c_int, W.DWORD], W.BOOL),
                     (user32.GetClassNameW, [P, P, ctypes.c_int], ctypes.c_int)):
    f.argtypes = args
    f.restype = res
EnumProc = ctypes.WINFUNCTYPE(W.BOOL, P, P)


def find(part):
    found = []
    def cb(h, _):
        if user32.IsWindowVisible(h) or True:
            n = user32.GetWindowTextLengthW(h)
            if n:
                b = ctypes.create_unicode_buffer(n + 1)
                user32.GetWindowTextW(h, b, n + 1)
                c = ctypes.create_unicode_buffer(64)
                user32.GetClassNameW(h, c, 64)
                # the game's own window (SDL's class), not another window that has the name in its title
                if part.lower() in b.value.lower() and c.value == 'SDL_app':
                    found.append(h)
        return True
    user32.EnumWindows(EnumProc(cb), 0)
    return found[0] if found else None


def read_png(path):
    d = open(path, 'rb').read()
    pos, idat, w, h = 8, b'', 0, 0
    while pos < len(d):
        n, t = struct.unpack('>I4s', d[pos:pos + 8])
        c = d[pos + 8:pos + 8 + n]
        if t == b'IHDR':
            w, h = struct.unpack('>II', c[:8])
            assert c[8] == 8 and c[9] in (2, 6) and c[12] == 0
            ch = 3 if c[9] == 2 else 4
        elif t == b'IDAT':
            idat += c
        pos += 12 + n
    raw = zlib.decompress(idat)
    stride = w * ch
    out = bytearray()
    prev = bytearray(stride)
    p = 0
    for y in range(h):
        f = raw[p]; line = bytearray(raw[p + 1:p + 1 + stride]); p += 1 + stride
        for i in range(stride):
            a = line[i - ch] if i >= ch else 0
            b = prev[i]
            c2 = prev[i - ch] if i >= ch else 0
            if f == 1: line[i] = (line[i] + a) & 255
            elif f == 2: line[i] = (line[i] + b) & 255
            elif f == 3: line[i] = (line[i] + ((a + b) >> 1)) & 255
            elif f == 4:
                pa, pb, pc = abs(b - c2), abs(a - c2), abs(a + b - 2 * c2)
                pr = a if pa <= pb and pa <= pc else (b if pb <= pc else c2)
                line[i] = (line[i] + pr) & 255
        out += line; prev = line
    return w, h, ch, bytes(out)


class BIH(ctypes.Structure):
    _fields_ = [('biSize', W.DWORD), ('biWidth', W.LONG), ('biHeight', W.LONG), ('biPlanes', W.WORD), ('biBitCount', W.WORD),
                ('biCompression', W.DWORD), ('biSizeImage', W.DWORD), ('biXPelsPerMeter', W.LONG), ('biYPelsPerMeter', W.LONG),
                ('biClrUsed', W.DWORD), ('biClrImportant', W.DWORD)]


def capture(hwnd):
    r = W.RECT(); user32.GetWindowRect(hwnd, ctypes.byref(r))
    cr = W.RECT(); user32.GetClientRect(hwnd, ctypes.byref(cr))
    pt = W.POINT(0, 0); user32.ClientToScreen(hwnd, ctypes.byref(pt))
    ww, wh = r.right - r.left, r.bottom - r.top
    hdc = user32.GetDC(None)
    mdc = gdi32.CreateCompatibleDC(hdc)
    bmp = gdi32.CreateCompatibleBitmap(hdc, ww, wh)
    gdi32.SelectObject(mdc, bmp)
    ok = user32.PrintWindow(hwnd, mdc, 2)
    bi = BIH(); bi.biSize = ctypes.sizeof(BIH); bi.biWidth = ww; bi.biHeight = -wh; bi.biPlanes = 1; bi.biBitCount = 32
    buf = (ctypes.c_ubyte * (ww * wh * 4))()
    gdi32.GetDIBits(mdc, bmp, 0, wh, buf, ctypes.byref(bi), 0)
    gdi32.DeleteObject(bmp); gdi32.DeleteDC(mdc); user32.ReleaseDC(None, hdc)
    ox, oy = pt.x - r.left, pt.y - r.top
    cw, chh = cr.right - cr.left, cr.bottom - cr.top
    return ok, cw, chh, bytes(buf), ww, ox, oy


def capture_screen(hwnd):
    # the pixels of the client area as the desktop shows them (BitBlt from the screen's DC): PrintWindow cannot read a window
    # that has had a Direct3D swap chain, and answers black
    cr = W.RECT(); user32.GetClientRect(hwnd, ctypes.byref(cr))
    pt = W.POINT(0, 0); user32.ClientToScreen(hwnd, ctypes.byref(pt))
    cw, chh = cr.right - cr.left, cr.bottom - cr.top
    hdc = user32.GetDC(None)
    mdc = gdi32.CreateCompatibleDC(hdc)
    bmp = gdi32.CreateCompatibleBitmap(hdc, cw, chh)
    gdi32.SelectObject(mdc, bmp)
    gdi32.BitBlt(mdc, 0, 0, cw, chh, hdc, pt.x, pt.y, 0x00CC0020 | 0x40000000)
    bi = BIH(); bi.biSize = ctypes.sizeof(BIH); bi.biWidth = cw; bi.biHeight = -chh; bi.biPlanes = 1; bi.biBitCount = 32
    buf = (ctypes.c_ubyte * (cw * chh * 4))()
    gdi32.GetDIBits(mdc, bmp, 0, chh, buf, ctypes.byref(bi), 0)
    gdi32.DeleteObject(bmp); gdi32.DeleteDC(mdc); user32.ReleaseDC(None, hdc)
    return cw, chh, bytes(buf)


def compare(buf, ww, ox, oy, cw, chh, w, h, ch, px):
    diff, first = 0, None
    n = cw // w if (cw % w == 0 and chh == (cw // w) * h and cw >= w) else 1  # a window n times the screenshot: each pixel repeated
    for y in range(min(h * n, chh)):
        for x in range(min(w * n, cw)):
            o = ((y + oy) * ww + x + ox) * 4
            s = ((y // n) * w + x // n) * ch
            if (buf[o + 2], buf[o + 1], buf[o]) != (px[s], px[s + 1], px[s + 2]):
                diff += 1
                if first is None:
                    first = (x, y, tuple(buf[o:o + 3][::-1]), tuple(px[s:s + 3]))
    return diff, first


def main():
    part, png = sys.argv[1], sys.argv[2]
    t0 = time.time()
    while not os.path.exists(png):
        if time.time() - t0 > 120:
            sys.exit('no screenshot')
        time.sleep(0.2)
    time.sleep(0.7)
    hwnd = find(part)
    if not hwnd:
        sys.exit('no window')
    ok, cw, chh, buf, ww, ox, oy = capture(hwnd)
    sw, sh, sbuf = capture_screen(hwnd)
    w, h, ch, px = read_png(png)
    diff, first = compare(buf, ww, ox, oy, cw, chh, w, h, ch, px)
    sdiff, sfirst = compare(sbuf, sw, 0, 0, sw, sh, w, h, ch, px)
    line = 'client %dx%d, screenshot %dx%d, PrintWindow %s: %d pixels differ%s; screen BitBlt: %d differ%s' % (
        cw, chh, w, h, 'ok' if ok else 'FAILED', diff, (', first at %s' % (first,)) if first else '', sdiff,
        (', first at %s' % (sfirst,)) if sfirst else '')
    print(line)
    if len(sys.argv) > 3:
        open(sys.argv[3], 'w').write(line + '\n')


if __name__ == '__main__':
    main()
