"""Runs one of the phase-2 scripts (build/ts-p2/<name>/orig.txt, made by p2lib.py) with the original on the user's visible desktop, for what
Windows Sandbox cannot give: the original's 3D mode (docs/ORIGRUN.md, "Hardware acceleration"; the user's "Yes, both routes", 2026-11-11).
A visrun for Typer Shark, made from Bookworm Adventures' tools/pilot/visrun.py on the same terms:

    python visrun3d.py <name> [--3d on|off] [--game <dir>] [--shape sandbox|client] [--users <dir>] [--reg <file.reg>] [--patch "va:0x.. <expected> <replacement>"]

--users puts that folder's files over the clean users folder (origrun's STAGE), --reg imports a registry preset of the original's (an
HKLM\\SOFTWARE\\PopCap export, as phase 2's profiles/<x>/orig.reg: read here as HKCU\\Software\\PopCap, where the registry map puts it) after
Is3D and ScreenMode are set, and --patch changes bytes of the copy's exe (checked against the expected bytes first; a copy that has the
replacement already is left as it is), for the medal tours (GAME_NOTES section 18).

The game is a copy under build/ts-3d/game (made by prepare()), with origrun's silent stand-in as its dsound.dll; it runs on the real clock
and starts with SW_SHOWNOACTIVATE, so it never takes the focus. Only the game's own window is touched: input is posted to it (never the
user's cursor or keyboard), the window is kept topmost, a shot is refused while another window overlaps it, and the picture is the window's
own (PrintWindow), never the screen. The game's GetCursorPos reads origrun's scripted point (the stand-in's cursor hook, which the user
allowed on 2026-11-03), written here into the same shared block origrun makes. The game's registry key (HKCU\\Software\\PopCap\\TyperShark,
where the stand-in's registry map, allowed on 2026-11-10, puts HKLM\\SOFTWARE\\PopCap\\TyperShark) is backed up first, started from Is3D and
ScreenMode, and put back afterwards.

Each script line "at N <cmd>" waits for the game's update counter (app+0x4b8) to reach N, holds the game (NtSuspendProcess), posts the input
and lets it go, so the input is read by update C+1 (C is the counter at the hold). The real clock gives no exact N, so every step is written
with the update C it landed at, in <name>/steps.tsv, and port.txt for the port is made from those, one input ahead as p2lib does it. A shot waits
until the game has drawn its last update (the pending-draw count, app+0x4a0, is 0), holds it and has the window draw itself.
"""
import ctypes
import ctypes.wintypes as wt
import mmap
import os
import shutil
import struct
import subprocess
import sys
import threading
import time
import uuid
import winreg

from PIL import Image

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.normpath(os.path.join(HERE, "..", "..", "..", ".."))
sys.path.insert(0, HERE)

APP, COUNTER, PENDING, FLAG3D = 0x565380, 0x4b8, 0x4a0, (0x334, 0x10)
KEY = r"Software\PopCap\TyperShark"
GAME = os.path.join(ROOT, "build", "ts-3d", "game")
SOURCE = os.path.join(ROOT, "build", "origrun", "typershark-p2", "game")
USERS_CLEAN = os.path.join(ROOT, "build", "ts-p1", "fixtures", "users-clean")
WRAPPER_FILES = ("DDraw.dll", "D3DImm.dll", "dgVoodoo.conf", "origrun-wrapper.txt")

k32 = ctypes.WinDLL("kernel32", use_last_error=True)
u32 = ctypes.WinDLL("user32", use_last_error=True)
g32 = ctypes.WinDLL("gdi32", use_last_error=True)
ntdll = ctypes.WinDLL("ntdll")
k32.OpenProcess.restype = wt.HANDLE
u32.GetDC.restype = wt.HDC
g32.CreateCompatibleDC.restype = wt.HDC
g32.CreateCompatibleBitmap.restype = wt.HBITMAP
g32.SelectObject.restype = wt.HGDIOBJ
g32.CreateCompatibleDC.argtypes = [wt.HDC]
g32.CreateCompatibleBitmap.argtypes = [wt.HDC, ctypes.c_int, ctypes.c_int]
g32.SelectObject.argtypes = [wt.HDC, wt.HGDIOBJ]
g32.GetDIBits.argtypes = [wt.HDC, wt.HBITMAP, ctypes.c_uint, ctypes.c_uint, ctypes.c_void_p, ctypes.c_void_p, ctypes.c_uint]
g32.DeleteObject.argtypes = [wt.HGDIOBJ]
g32.DeleteDC.argtypes = [wt.HDC]
u32.ReleaseDC.argtypes = [wt.HWND, wt.HDC]
u32.PostMessageW.argtypes = [wt.HWND, ctypes.c_uint, wt.WPARAM, wt.LPARAM]
u32.PrintWindow.argtypes = [wt.HWND, wt.HDC, ctypes.c_uint]
u32.GetWindow.restype = wt.HWND
u32.GetWindow.argtypes = [wt.HWND, ctypes.c_uint]
u32.SetWindowPos.argtypes = [wt.HWND, wt.HWND, ctypes.c_int, ctypes.c_int, ctypes.c_int, ctypes.c_int, ctypes.c_uint]


class CurShared(ctypes.Structure):
    """tools/origrun/cursor.h, the block the stand-in's GetCursorPos reads (CUR_MAGIC 'CURS', CUR_VERSION 2)."""
    _fields_ = [("magic", ctypes.c_uint32), ("version", ctypes.c_uint32), ("size", ctypes.c_uint32), ("set", ctypes.c_int32),
                ("pos", ctypes.c_int64), ("posts", ctypes.c_int32), ("installed", ctypes.c_int32), ("reads", ctypes.c_int32),
                ("realReads", ctypes.c_int32), ("keysSet", ctypes.c_int32), ("keyPosts", ctypes.c_int32),
                ("keysInstalled", ctypes.c_int32), ("keyReads", ctypes.c_int32), ("keyRealReads", ctypes.c_int32),
                ("keys", ctypes.c_uint8 * 256)]


class Cursor:
    def __init__(self, run_id):
        self.name = "Local\\origrun-cursor-" + run_id
        self.mm = mmap.mmap(-1, ctypes.sizeof(CurShared), tagname=self.name)
        self.blk = CurShared.from_buffer(self.mm)
        self.blk.magic, self.blk.version, self.blk.size = 0x52535543, 2, ctypes.sizeof(CurShared)

    def point(self, x, y):
        self.blk.pos = (y << 32) | (x & 0xffffffff)
        self.blk.set = 1
        self.blk.posts += 1

    def stats(self):
        return "installed %d, reads %d, real reads %d" % (self.blk.installed, self.blk.reads, self.blk.realReads)


class Game:
    def __init__(self, exe, env):
        si = subprocess.STARTUPINFO()
        si.dwFlags = 1          # STARTF_USESHOWWINDOW: the game's first ShowWindow takes this, so the window never takes the focus
        si.wShowWindow = 4      # SW_SHOWNOACTIVATE
        self.p = subprocess.Popen([exe], cwd=os.path.dirname(exe), env=env, startupinfo=si)
        self.h = k32.OpenProcess(0x0010 | 0x0400 | 0x0800, False, self.p.pid)   # VM_READ, QUERY_INFO, SUSPEND_RESUME
        self.hwnd = None

    def rd(self, addr, size=4):
        v = ctypes.c_uint32()
        n = ctypes.c_size_t()
        if not addr or not k32.ReadProcessMemory(self.h, ctypes.c_void_p(addr), ctypes.byref(v), size, ctypes.byref(n)):
            return None
        return v.value & (0xff if size == 1 else 0xffffffff)

    def app(self):
        return self.rd(APP)

    def counter(self):
        a = self.app()
        return self.rd(a + COUNTER) if a else None

    def pending(self):
        a = self.app()
        return self.rd(a + PENDING) if a else None

    def flag3d(self):
        a = self.app()
        dd = self.rd(a + FLAG3D[0]) if a else None
        return self.rd(dd + FLAG3D[1], 1) if dd else None

    def hold(self):
        ntdll.NtSuspendProcess(self.h)

    def release(self):
        ntdll.NtResumeProcess(self.h)

    def window(self):
        if self.hwnd and u32.IsWindow(self.hwnd):
            return self.hwnd
        found = []

        @ctypes.WINFUNCTYPE(wt.BOOL, wt.HWND, wt.LPARAM)
        def cb(hw, lp):
            pid = wt.DWORD()
            u32.GetWindowThreadProcessId(hw, ctypes.byref(pid))
            r = wt.RECT()
            u32.GetClientRect(hw, ctypes.byref(r))
            if pid.value == self.p.pid and u32.IsWindowVisible(hw) and r.right == 640 and r.bottom == 480:
                found.append(hw)
            return True
        u32.EnumWindows(cb, 0)
        self.hwnd = found[0] if found else None
        if self.hwnd:
            # topmost, not activated: nothing of the user's can come over it while it is captured
            u32.SetWindowPos(self.hwnd, wt.HWND(-1), 0, 0, 0, 0, 0x0001 | 0x0002 | 0x0010)
        return self.hwnd


def client_rect(hw):
    r = wt.RECT()
    u32.GetClientRect(hw, ctypes.byref(r))
    pt = wt.POINT(0, 0)
    u32.ClientToScreen(hw, ctypes.byref(pt))
    return pt.x, pt.y, r.right, r.bottom


def overlapped(hw):
    """A visible window above the game's in the z-order that covers part of its client area, or None."""
    x, y, w, h = client_rect(hw)
    o = u32.GetWindow(hw, 3)   # GW_HWNDPREV
    while o:
        if u32.IsWindowVisible(o) and not u32.IsIconic(o):
            r = wt.RECT()
            u32.GetWindowRect(o, ctypes.byref(r))
            if r.left < x + w and r.right > x and r.top < y + h and r.bottom > y and r.right > r.left and r.bottom > r.top:
                buf = ctypes.create_unicode_buffer(256)
                u32.GetClassNameW(o, buf, 256)
                return buf.value
        o = u32.GetWindow(o, 3)
    return None


def grab(hw, g, shape):
    """The game's window as the window itself renders it (PrintWindow, PW_RENDERFULLCONTENT), never the screen. PrintWindow needs the
    game's thread to answer, so it is called from a helper thread and the game released a moment after: the picture shows the held
    update. Returns None if PrintWindow fails or does not return within 3 s. shape "sandbox" puts the client at (3, 26) of a 646x509
    picture, as origrun's sandbox shots are (shotdiff cuts that away); "client" is the 640x480 client alone."""
    r = wt.RECT()
    u32.GetWindowRect(hw, ctypes.byref(r))
    w, h = r.right - r.left, r.bottom - r.top
    cx, cy, cw, ch = client_rect(hw)
    sdc = u32.GetDC(None)
    mdc = g32.CreateCompatibleDC(sdc)
    bmp = g32.CreateCompatibleBitmap(sdc, w, h)
    u32.ReleaseDC(None, sdc)
    old = g32.SelectObject(mdc, bmp)
    done = []
    t = threading.Thread(target=lambda: done.append(u32.PrintWindow(hw, mdc, 2)), daemon=True)
    t.start()
    t0 = time.perf_counter()
    while time.perf_counter() - t0 < 0.0005:
        pass
    g.release()
    t.join(3)
    if t.is_alive() or not done or not done[0]:
        return None
    buf = ctypes.create_string_buffer(w * h * 4)

    class BMI(ctypes.Structure):
        _fields_ = [("biSize", wt.DWORD), ("biWidth", wt.LONG), ("biHeight", wt.LONG), ("biPlanes", wt.WORD),
                    ("biBitCount", wt.WORD), ("biCompression", wt.DWORD), ("biSizeImage", wt.DWORD),
                    ("biXPelsPerMeter", wt.LONG), ("biYPelsPerMeter", wt.LONG), ("biClrUsed", wt.DWORD),
                    ("biClrImportant", wt.DWORD), ("pad", wt.DWORD * 4)]
    bmi = BMI()
    bmi.biSize = 40
    bmi.biWidth, bmi.biHeight, bmi.biPlanes, bmi.biBitCount = w, -h, 1, 32
    g32.GetDIBits(mdc, bmp, 0, h, buf, ctypes.byref(bmi), 0)
    g32.SelectObject(mdc, old)
    g32.DeleteObject(bmp)
    g32.DeleteDC(mdc)
    ox, oy = cx - r.left, cy - r.top
    im = Image.frombuffer("RGBA", (w, h), buf, "raw", "BGRA", 0, 1).convert("RGB").crop((ox, oy, ox + 640, oy + 480))
    if shape == "client":
        return im
    canvas = Image.new("RGB", (646, 509))
    canvas.paste(im, (3, 26))
    return canvas


def xy(x, y):
    return (y << 16) | (x & 0xffff)


def post_input(g, cur, cmd, args):
    hw = g.window()
    cx, cy, _, _ = client_rect(hw)
    if cmd in ("move", "click", "rclick", "down", "up"):
        if cmd != "up" or len(args) >= 2:
            x, y = int(args[0]), int(args[1])
            cur.point(cx + x, cy + y)         # the point the game's GetCursorPos reads, before the message
        else:
            x = y = 0
        lp = xy(x, y)
        if cmd == "move":
            u32.PostMessageW(hw, 0x0200, 0, lp)
        elif cmd == "click":
            u32.PostMessageW(hw, 0x0200, 0, lp)
            u32.PostMessageW(hw, 0x0201, 1, lp)
            u32.PostMessageW(hw, 0x0202, 0, lp)
        elif cmd == "rclick":
            u32.PostMessageW(hw, 0x0200, 0, lp)
            u32.PostMessageW(hw, 0x0204, 2, lp)
            u32.PostMessageW(hw, 0x0205, 0, lp)
        elif cmd == "down":
            u32.PostMessageW(hw, 0x0200, 0, lp)
            u32.PostMessageW(hw, 0x0201, 1, lp)
        elif cmd == "up":
            u32.PostMessageW(hw, 0x0202, 0, lp)
    elif cmd == "focus":
        if args[0] == "regain":
            u32.PostMessageW(hw, 0x001C, 1, 0)   # WM_ACTIVATEAPP
            u32.PostMessageW(hw, 0x0006, 1, 0)   # WM_ACTIVATE
            u32.PostMessageW(hw, 0x0007, 0, 0)   # WM_SETFOCUS
        else:
            u32.PostMessageW(hw, 0x001C, 0, 0)
            u32.PostMessageW(hw, 0x0006, 0, 0)
            u32.PostMessageW(hw, 0x0008, 0, 0)   # WM_KILLFOCUS
    elif cmd == "key":
        vk = int(args[0])
        sc = u32.MapVirtualKeyW(vk, 0)
        u32.PostMessageW(hw, 0x0100, vk, 1 | (sc << 16))
        u32.PostMessageW(hw, 0x0101, vk, 1 | (sc << 16) | (1 << 30) | (1 << 31))
    elif cmd == "text":
        for ch in " ".join(args):
            u32.PostMessageW(hw, 0x0102, ord(ch), 1)
    else:
        raise SystemExit("visrun3d: cannot do %s" % cmd)


def wait_counter(g, n, log, timeout=120):
    """Waits for the update counter to reach n: it sleeps while far from n and spins near it, so that an update that comes in a burst
    with the next one is still seen."""
    t0 = time.time()
    while time.time() - t0 < timeout:
        c = g.counter()
        if c is not None and c >= n:
            return c
        if g.p.poll() is not None:
            return None
        if c is None or n - c > 8:
            time.sleep(0.004)
    print("counter %d not reached in %d s" % (n, timeout), file=log, flush=True)
    return None


def shot(g, out, name, landed, log, shape):
    hw = g.window()
    o = overlapped(hw)
    if o:
        print("shot %s refused: a window (%s) overlaps the game" % (name, o), file=log, flush=True)
        return None
    for _ in range(400):
        if g.pending() == 0:
            g.hold()
            if g.pending() == 0:
                break
            g.release()
    else:
        print("shot %s refused: no drawn update in 400 tries" % name, file=log, flush=True)
        return None
    c = g.counter()
    im = grab(hw, g, shape)   # releases the game
    if im is None:
        print("shot %s refused: PrintWindow failed or did not return" % name, file=log, flush=True)
        return None
    path = os.path.join(out, name + ".png")
    os.makedirs(os.path.dirname(path), exist_ok=True)
    im.save(path)
    landed.write("%s\t%s\tdrawn\n" % (name, c))
    landed.flush()
    return c


def prepare(threed, users=None, patch=None):
    """A fresh copy of the staged original under build/ts-3d/game: no dgVoodoo files, the clean users folder (with the files of `users`
    over it), origrun's stand-in, and the exe's bytes changed by `patch` ("va:0x.. <expected> <replacement>")."""
    if os.path.isdir(GAME):
        shutil.rmtree(GAME)
    shutil.copytree(SOURCE, GAME, ignore=shutil.ignore_patterns("users", "logs", "crash.txt", "*.log", *WRAPPER_FILES))
    shutil.copytree(USERS_CLEAN, os.path.join(GAME, "users"))
    if users:
        shutil.copytree(users, os.path.join(GAME, "users"), dirs_exist_ok=True)
    if patch:
        apply_patch(os.path.join(GAME, "TyperSharkGame.exe"), patch)


def apply_patch(exe, spec):
    """One patch of origrun's list, `va:0x<address> <expected hex> <replacement hex>`, by the PE's section table."""
    where, expected, replacement = spec.split()
    va = int(where.split(":", 1)[1], 16)
    expected, replacement = bytes.fromhex(expected), bytes.fromhex(replacement)
    data = bytearray(open(exe, "rb").read())
    pe = struct.unpack_from("<I", data, 0x3c)[0]
    count, opt = struct.unpack_from("<H", data, pe + 6)[0], struct.unpack_from("<H", data, pe + 20)[0]
    base = struct.unpack_from("<I", data, pe + 24 + 28)[0]
    for i in range(count):
        _, size, rva, raw_size, raw = struct.unpack_from("<8sIIII", data, pe + 24 + opt + 40 * i)
        if rva <= va - base < rva + max(size, raw_size):
            off = raw + va - base - rva
            break
    else:
        raise SystemExit("visrun3d: %s is in no section" % where)
    have = bytes(data[off:off + len(expected)])
    if have == replacement:
        return
    if have != expected:
        raise SystemExit("visrun3d: %s has %s, expected %s" % (where, have.hex(), expected.hex()))
    data[off:off + len(replacement)] = replacement
    open(exe, "wb").write(data)


def import_registry(backup, reg):
    """The original's registry preset (UTF-16, HKLM\\SOFTWARE\\PopCap) imported as HKCU\\Software\\PopCap, where the registry map puts it."""
    text = open(reg, "rb").read().decode("utf-16")
    text = text.replace("HKEY_LOCAL_MACHINE\\SOFTWARE\\PopCap", "HKEY_CURRENT_USER\\Software\\PopCap")
    conv = os.path.join(backup, "preset.reg")
    open(conv, "wb").write(b"\xff\xfe" + text.encode("utf-16-le"))
    subprocess.run(["reg", "import", conv], check=True, capture_output=True)


def stage_registry(backup, threed):
    if os.path.exists(backup):
        raise SystemExit("visrun3d: %s exists: an earlier run was not restored" % backup)
    os.makedirs(backup)
    had = subprocess.run(["reg", "export", "HKCU\\" + KEY, os.path.join(backup, "typershark.reg"), "/y"], capture_output=True)
    open(os.path.join(backup, "had.txt"), "w").write("1" if had.returncode == 0 else "0")
    subprocess.run(["reg", "delete", "HKCU\\" + KEY, "/f"], capture_output=True)
    with winreg.CreateKey(winreg.HKEY_CURRENT_USER, KEY) as k:
        winreg.SetValueEx(k, "Is3D", 0, winreg.REG_DWORD, 1 if threed else 0)
        winreg.SetValueEx(k, "ScreenMode", 0, winreg.REG_DWORD, 0)


def restore_registry(backup):
    subprocess.run(["reg", "delete", "HKCU\\" + KEY, "/f"], capture_output=True)
    if open(os.path.join(backup, "had.txt")).read() == "1":
        subprocess.run(["reg", "import", os.path.join(backup, "typershark.reg")], check=True, capture_output=True)
    shutil.rmtree(backup)


def main():
    a = sys.argv[1:]
    threed, shape = True, "sandbox"
    users = reg = patch = None
    for opt in ("--3d", "--shape", "--users", "--reg", "--patch"):
        if opt in a:
            i = a.index(opt)
            if opt == "--3d":
                threed = a[i + 1] == "on"
            elif opt == "--users":
                users = os.path.abspath(a[i + 1])
            elif opt == "--reg":
                reg = os.path.abspath(a[i + 1])
            elif opt == "--patch":
                patch = a[i + 1]
            else:
                shape = a[i + 1]
            del a[i:i + 2]
    name = a[0]
    tour_dir = os.path.join(ROOT, "build", "ts-p2", name)
    script = os.path.join(tour_dir, "orig.txt")
    out = os.path.join(tour_dir, "orig3d" if threed else "orig2d")
    if os.path.isdir(out):
        shutil.rmtree(out)
    os.makedirs(out)
    base = os.path.join(tour_dir, "vis3d" if threed else "vis2d")
    log = open(base + ".log", "w")
    landed = open(os.path.join(out, "landed.tsv"), "w")
    steps = open(base + ".steps.tsv", "w")
    backup = os.path.join(ROOT, "build", "ts-3d", "registry-backup")
    prepare(threed, users, patch)
    stage_registry(backup, threed)
    if reg:
        import_registry(backup, reg)
    run_id = uuid.uuid4().hex[:12]
    cur = Cursor(run_id)
    env = dict(os.environ)
    env.update({"ORIGRUN_DIR": os.path.join(ROOT, "build", "ts-3d", "logs"), "ORIGRUN_SOUND": "stub", "ORIGRUN_REGMAP": "popcap",
                "ORIGRUN_CURSOR": "scripted", "ORIGRUN_CURSOR_ID": run_id})
    os.makedirs(env["ORIGRUN_DIR"], exist_ok=True)
    g = None
    try:
        g = Game(os.path.join(GAME, "TyperSharkGame.exe"), env)
        flags = set()
        last_input = -1
        t0 = time.time()
        while not g.window() and time.time() - t0 < 90 and g.p.poll() is None:
            time.sleep(0.05)
        if not g.window():
            print("no window", file=log, flush=True)
            return
        t_window = time.time()
        print("window after %.1f s" % (time.time() - t0), file=log, flush=True)
        post_input(g, cur, "focus", ["regain"])       # the game believes it has the focus, as the only window of a sandbox did
        for line in open(script):
            w = line.split()
            if not w:
                continue
            if w[0] == "quit":
                break
            if w[0] != "at" or w[2] in ("peek",):
                print("skipped: %s" % line.strip(), file=log)
                continue
            n, cmd, args = int(w[1]), w[2], w[3:]
            if cmd == "shot":
                # an input is read by the update after the one it was posted at, and the game may redraw at once when the input made a
                # widget dirty, with no update between: a shot at the input's own update would show the result of an input that the
                # port reads one update later. The shot waits for the update after it.
                n = max(n, last_input + 1)
            if wait_counter(g, n, log) is None:
                break
            f = g.flag3d()
            if f not in flags:
                flags.add(f)
                print("update %d: 3D flag %s" % (n, f), file=log, flush=True)
            if cmd == "shot":
                c = shot(g, out, args[0], landed, log, shape)
                if c is not None:
                    steps.write("%d\t%d\tshot\t%s\t%.3f\n" % (n, c, args[0], time.time() - t_window))
            else:
                g.hold()
                c = g.counter()
                try:
                    post_input(g, cur, cmd, args)
                finally:
                    g.release()
                last_input = c
                steps.write("%d\t%d\t%s\t%s\t%.3f\n" % (n, c, cmd, " ".join(args), time.time() - t_window))
            steps.flush()
        print("end: update %s, 3D flags seen %s; cursor: %s" % (g.counter(), sorted(flags, key=str), cur.stats()), file=log, flush=True)
    finally:
        if g and g.p.poll() is None:
            hw = g.window()
            if hw:
                u32.PostMessageW(hw, 0x0010, 0, 0)
            try:
                g.p.wait(20)
            except subprocess.TimeoutExpired:
                g.p.kill()
                g.p.wait(10)
                print("killed", file=log)
        time.sleep(0.5)
        restore_registry(backup)
        print("restored the registry key", file=log, flush=True)


if __name__ == "__main__":
    main()
