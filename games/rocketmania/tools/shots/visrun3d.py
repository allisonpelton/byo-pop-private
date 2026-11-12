"""Runs one of the screen tours (tools/shots/<tour>.txt) with the original on the user's visible desktop, for what Windows Sandbox cannot
give: the original's 3D mode (docs/ORIGRUN.md, "Hardware acceleration"; the overnight rule of CLAUDE.md, 22:00 to 08:00, allows it).
Made from Typer Shark's tools/phase2/visrun3d.py and Bookworm Adventures' tools/pilot/visrun.py, on the same terms:

    python visrun3d.py <tour> [--3d on|off] [--seed N] [--base N] [--shape client|sandbox]

The game is a copy under C:/byo-og/rm3d/game (a short path, as the other staged copies are: the original builds absolute paths in a small
buffer) made from the staged original's clean exe (RocketManiaGame.exe.origrun-unpatched) with the two patches of
tools/origrun/patches/rocketmania.txt, the run's seed and the PNG loader's buffer, and origrun's silent stand-in as its dsound.dll. It runs on
the real clock and starts with SW_SHOWNOACTIVATE, so it never takes the focus. Only the game's own window is touched: input is posted to it
(never the user's cursor or keyboard), the window is kept topmost, a shot is refused while another window overlaps it, and the picture is the
window's own (PrintWindow), never the screen. The game's GetCursorPos reads origrun's scripted point (the stand-in's cursor hook, which the
user allowed on 2026-11-03), written here into the same shared block origrun makes. The game's registry key (HKCU\\Software\\PopCap\\RocketMania,
where the stand-in's registry map puts HKLM\\SOFTWARE\\PopCap\\RocketMania) is backed up first, started from Is3D, ScreenMode and
HintsDisabled=0 (the hint dialogs on, as the host coverage runs had them), and put back afterwards.

A tour line "<update> <command>" is read as the 2D tours are (make.py): the original takes it at update-1, and a screenshot is a "shot" at
update-1. The tours' first update (40000, 300000, ...) stands for "the menu has long been up": it is moved to --base (default: the update
counter 25 s after the window came, plus 200) and every other line moves with it. Each step "at N <cmd>" waits for the game's update counter
(app+0x3f0) to reach N, holds the game (NtSuspendProcess), posts the input and lets it go, so the input is read by update C+1 (C is the
counter at the hold). The real clock gives no exact N, so every step is written with the update C it landed at, in <tour>/vis3d.steps.tsv,
and mkport3d.py makes the port's script from those. A shot waits until the game has drawn its last update (the count of updates since the
last draw, app+0x3d8, is 0), holds it and has the window draw itself.
"""
import ctypes
import ctypes.wintypes as wt
import mmap
import glob
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

APP = 0x4f6460                           # the app is a static object
COUNTER, PENDING, DDI, HWND = 0x3f0, 0x3d8, 0x2ec, 0x2cc
FLAG3D = 0x10                            # the DDInterface's mIs3D
KEY = r"Software\PopCap\RocketMania"
SOURCE = "C:/byo-og/rm/game"
GAME = "C:/byo-og/rm3d/game"
BACKUP = "C:/byo-og/rm3d/registry-backup"
EXE = "RocketManiaGame.exe"
# tools/origrun/patches/rocketmania.txt: the seed (SRand(GetTickCount()) takes the run's seed) and the PNG loader's buffer size
PATCHES = ["va:0x46d331 ff15f8414c00 b8%s90", "va:0x473216 e88ba100000faf45f4c1e002 8b45ec0faf45f4c1e0026690"]

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

    def counter(self):
        return self.rd(APP + COUNTER)

    def pending(self):
        return self.rd(APP + PENDING)

    def flag3d(self):
        dd = self.rd(APP + DDI)
        return self.rd(dd + FLAG3D, 1) if dd else None

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


def other_game_windows():
    """Visible windows of other games on the user's desktop (an original SexyApp game's window class is MainWindow, a port's SDL_app): another session's desktop run, which
    this one must not share the screen with (its window overlaps, its load disturbs a measurement of its own)."""
    found = []

    @ctypes.WINFUNCTYPE(wt.BOOL, wt.HWND, wt.LPARAM)
    def cb(hw, lp):
        if u32.IsWindowVisible(hw):
            buf = ctypes.create_unicode_buffer(64)
            u32.GetClassNameW(hw, buf, 64)
            if buf.value in ("MainWindow", "SDL_app"):
                found.append(hw)
        return True
    u32.EnumWindows(cb, 0)
    return found


def wait_for_desktop(log, quiet_s=180):
    """Waits until no other game's window has been on the desktop for quiet_s seconds (a loop of runs leaves gaps of a minute or so between
    its games); gives up (SystemExit 4) after 07:30, so that the run never reaches 08:00."""
    t0, said, clear_since = time.time(), 0, None
    while clear_since is None or time.time() - clear_since < quiet_s:
        now = time.localtime()
        if 7.5 <= now.tm_hour + now.tm_min / 60.0 < 22:
            raise SystemExit("visrun3d: the desktop stayed taken by another game's window until %02d:%02d" % (now.tm_hour, now.tm_min))
        if other_game_windows():
            clear_since = None
            if time.time() - t0 >= said * 300:
                print("waiting for the desktop: another game's window is up (%d min)" % ((time.time() - t0) / 60), file=log, flush=True)
                said += 1
        elif clear_since is None:
            clear_since = time.time()
        time.sleep(5)


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
        x, y = int(args[0]), int(args[1])
        cur.point(cx + x, cy + y)         # the point the game's GetCursorPos reads, before the message
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


def wait_counter(g, n, log, timeout=180):
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


def prepare(seed):
    """A fresh copy of the staged original under C:/byo-og/rm3d/game: its clean exe with the two patches, no saves, origrun's stand-in
    (the staged copy's dsound.dll: the stand-in that the last origrun run there placed)."""
    if os.path.isdir(GAME):
        shutil.rmtree(GAME)
    shutil.copytree(SOURCE, GAME)
    standin = os.path.join(ROOT, "build", "origrun-bin", "dsound.dll")   # the stand-in of the latest origrun build
    if os.path.exists(standin):
        shutil.copyfile(standin, os.path.join(GAME, "dsound.dll"))
    unpatched = os.path.join(GAME, EXE + ".origrun-unpatched")
    if os.path.exists(unpatched):
        shutil.copyfile(unpatched, os.path.join(GAME, EXE))
    for p in glob.glob(os.path.join(GAME, "Players", "save?.dat")) + glob.glob(os.path.join(GAME, "Players", "hiscores.dat")):
        os.remove(p)   # the saved games (savec.dat, ...: a copy of the stage carries the ones its last run left) and the score file
    seed_hex = struct.pack("<I", seed).hex()
    for p in PATCHES:
        apply_patch(os.path.join(GAME, EXE), p.replace("%s", seed_hex) if "%s" in p else p)


def stage_registry(threed):
    if os.path.exists(BACKUP):
        raise SystemExit("visrun3d: %s exists: an earlier run was not restored" % BACKUP)
    os.makedirs(BACKUP)
    had = subprocess.run(["reg", "export", "HKCU\\" + KEY, os.path.join(BACKUP, "rocketmania.reg"), "/y"], capture_output=True)
    open(os.path.join(BACKUP, "had.txt"), "w").write("1" if had.returncode == 0 else "0")
    subprocess.run(["reg", "delete", "HKCU\\" + KEY, "/f"], capture_output=True)
    with winreg.CreateKey(winreg.HKEY_CURRENT_USER, KEY) as k:
        winreg.SetValueEx(k, "Is3D", 0, winreg.REG_DWORD, 1 if threed else 0)
        winreg.SetValueEx(k, "ScreenMode", 0, winreg.REG_DWORD, 0)
        winreg.SetValueEx(k, "HintsDisabled", 0, winreg.REG_DWORD, 0)
        for kv in filter(None, os.environ.get("RM_EXTRA", "").split(",")):   # more values, "Name=value,...": the hint flags of the tour hf1
            winreg.SetValueEx(k, kv.split("=")[0], 0, winreg.REG_DWORD, int(kv.split("=")[1]))


def restore_registry():
    subprocess.run(["reg", "delete", "HKCU\\" + KEY, "/f"], capture_output=True)
    if open(os.path.join(BACKUP, "had.txt")).read() == "1":
        subprocess.run(["reg", "import", os.path.join(BACKUP, "rocketmania.reg")], check=True, capture_output=True)
    shutil.rmtree(BACKUP)


def read_tour(path):
    """The tour's lines as (update, command, args) the way make.py's origrun side reads them: taken at update-1, a screenshot a shot, a
    char a text."""
    lines = []
    for l in open(path):
        l = l.split("#")[0].strip()
        if not l:
            continue
        k, cmd, *rest = l.split(None, 2)
        if cmd == "quit":
            continue
        cmd = {"screenshot": "shot", "char": "text"}.get(cmd, cmd)
        lines.append((int(k) - 1, cmd, rest[0].split() if rest else []))
    return lines


def main():
    a = sys.argv[1:]
    threed, shape, seed, base = True, "client", 1, None
    for opt in ("--3d", "--shape", "--seed", "--base"):
        if opt in a:
            i = a.index(opt)
            if opt == "--3d":
                threed = a[i + 1] == "on"
            elif opt == "--seed":
                seed = int(a[i + 1])
            elif opt == "--base":
                base = int(a[i + 1])
            else:
                shape = a[i + 1]
            del a[i:i + 2]
    name = a[0]
    tour = read_tour(os.path.join(HERE, name + ".txt"))
    tour_dir = os.path.join(ROOT, "build", "ph2", name)
    out = os.path.join(tour_dir, "orig3d" if threed else "orig2d")
    if os.path.isdir(out):
        shutil.rmtree(out)
    os.makedirs(out)
    prefix = os.path.join(tour_dir, "vis3d" if threed else "vis2d")
    log = open(prefix + ".log", "w")
    landed = open(os.path.join(out, "landed.tsv"), "w")
    steps = open(prefix + ".steps.tsv", "w")
    wait_for_desktop(log)
    os.makedirs(os.path.dirname(BACKUP), exist_ok=True)
    prepare(seed)
    stage_registry(threed)
    run_id = uuid.uuid4().hex[:12]
    cur = Cursor(run_id)
    env = dict(os.environ)
    env.update({"ORIGRUN_DIR": "C:/byo-og/rm3d/logs", "ORIGRUN_SOUND": "stub", "ORIGRUN_REGMAP": "popcap",
                "ORIGRUN_CURSOR": "scripted", "ORIGRUN_CURSOR_ID": run_id})
    os.makedirs(env["ORIGRUN_DIR"], exist_ok=True)
    g = None
    try:
        g = Game(os.path.join(GAME, EXE), env)
        flags = set()
        last_input, refused = -1, 0
        t0 = time.time()
        while not g.window() and time.time() - t0 < 90 and g.p.poll() is None:
            time.sleep(0.05)
        if not g.window():
            print("no window", file=log, flush=True)
            return
        t_window = time.time()
        print("window after %.1f s" % (time.time() - t0), file=log, flush=True)
        post_input(g, cur, "focus", ["regain"])       # the game believes it has the focus, as the only window of a sandbox did
        if base is None:
            while time.time() - t_window < 25 and g.p.poll() is None:
                time.sleep(0.2)
            base = (g.counter() or 0) + 200
        shift = base - tour[0][0]
        print("tour update %d is update %d here (shift %d)" % (tour[0][0], base, shift), file=log, flush=True)
        for n0, cmd, args in tour:
            n = n0 + shift
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
                    refused = 0
                    steps.write("%d\t%d\tshot\t%s\t%.3f\n" % (n, c, args[0], time.time() - t_window))
                else:
                    refused += 1
                    if refused >= 3:   # the desktop is taken (another game's window, or the user's): the run is lost, end it at once
                        print("end: three shots in a row refused", file=log, flush=True)
                        raise SystemExit(4)
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
        restore_registry()
        print("restored the registry key", file=log, flush=True)


if __name__ == "__main__":
    main()
