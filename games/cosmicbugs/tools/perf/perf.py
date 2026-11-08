#!/usr/bin/env python
"""perf.py: load time, memory and processor time of Cosmic Bugs, the original against the port (README.md, "Performance").

  build/native/rundesk.exe python games/cosmicbugs/tools/perf/perf.py port [--exe build/native/cosmicbugs.exe] [--label port] [--out DIR]
  python games/cosmicbugs/tools/perf/perf.py orig [--label original] [--out DIR]

Both run in real time (the port without -fast, the original on the wall clock), invisibly and silently, windowed, on a fresh
save: the port under rundesk (so start this script under it), the original through tools/origrun (which puts it under rundesk
itself, so start `orig` directly), unpatched (the game is the full version already). The same timeline for both, in the
game's own updates (its step counter, gFrame, one per millisecond: 0x460a1c in the original, the port's symbol, read from
the process's memory):

  load       from the process's creation to the first update (the counter leaves 1: the game's loading is done and its
             main loop runs); Cosmic Bugs loads everything before its first frame, in one go
  title      idle on the title (the logo's two seconds are over by update 2162), updates 3000..12000
  board      the title's Start Game at update 13000, the instructions page's Play at 14500 (a fresh save shows the page),
             then the shooter's first board with no input (the enemies move, the ship waits), updates 16000..46000

Memory (peak working set, peak private bytes) and processor time (user + kernel seconds) are sampled every 20 ms over the whole
run and over each window; a window's processor time is the difference between the samples at its two counter values, divided
by their wall time. The other game processes on the machine are counted at the start and the end (they share the cores).
The port's two input files (-commands and -realinput, which the test channel polls) are on in its run, to click with.
"""
import argparse
import ctypes
import ctypes.wintypes as W
import json
import os
import subprocess
import sys
import threading
import time
from pathlib import Path

ROOT = Path(subprocess.check_output(["git", "rev-parse", "--show-toplevel"], text=True).strip()).resolve()
NM = "E:/retools/llvm-mingw-20261006-ucrt-x86_64/bin/llvm-nm.exe"
ORIGRUN = ROOT / "build/origrun-bin/origrun.exe"
ORIG_FRAME = 0x460a1c
WINDOWS = {"title": (3000, 12000), "board": (16000, 46000)}
CLICKS = [(13000, 175, 313), (14500, 400, 563)]  # Start Game, the instructions page's Play (the game's updates, the centres)
END = 47000
CONFIG = '<CP_CONFIG\n    fullscreen="0"\n    desktopResolution="0"\n    wave="1"\n    music="1"\n/>\n'

k32 = ctypes.WinDLL("kernel32", use_last_error=True)
psapi = ctypes.WinDLL("psapi", use_last_error=True)
k32.OpenProcess.restype = W.HANDLE
k32.OpenProcess.argtypes = [W.DWORD, W.BOOL, W.DWORD]
k32.ReadProcessMemory.argtypes = [W.HANDLE, ctypes.c_void_p, ctypes.c_void_p, ctypes.c_size_t, ctypes.POINTER(ctypes.c_size_t)]
k32.QueryFullProcessImageNameW.argtypes = [W.HANDLE, W.DWORD, W.LPWSTR, ctypes.POINTER(W.DWORD)]
k32.CloseHandle.argtypes = [W.HANDLE]
k32.CreateToolhelp32Snapshot.restype = W.HANDLE
k32.CreateToolhelp32Snapshot.argtypes = [W.DWORD, W.DWORD]
k32.Process32FirstW.argtypes = [W.HANDLE, ctypes.c_void_p]
k32.Process32NextW.argtypes = [W.HANDLE, ctypes.c_void_p]
psapi.EnumProcessModulesEx.argtypes = [W.HANDLE, ctypes.POINTER(ctypes.c_void_p), W.DWORD, ctypes.POINTER(W.DWORD), W.DWORD]
psapi.GetModuleFileNameExW.argtypes = [W.HANDLE, ctypes.c_void_p, W.LPWSTR, W.DWORD]


class PE32(ctypes.Structure):
    _fields_ = [("dwSize", W.DWORD), ("cntUsage", W.DWORD), ("th32ProcessID", W.DWORD), ("th32DefaultHeapID", ctypes.c_size_t),
                ("th32ModuleID", W.DWORD), ("cntThreads", W.DWORD), ("th32ParentProcessID", W.DWORD),
                ("pcPriClassBase", ctypes.c_long), ("dwFlags", W.DWORD), ("szExeFile", ctypes.c_wchar * 260)]


class PMC(ctypes.Structure):
    _fields_ = [("cb", W.DWORD), ("PageFaultCount", W.DWORD)] + [(n, ctypes.c_size_t) for n in (
        "PeakWorkingSetSize", "WorkingSetSize", "QuotaPeakPagedPoolUsage", "QuotaPagedPoolUsage",
        "QuotaPeakNonPagedPoolUsage", "QuotaNonPagedPoolUsage", "PagefileUsage", "PeakPagefileUsage", "PrivateUsage")]


def processes():
    snap = k32.CreateToolhelp32Snapshot(2, 0)
    out = []
    e = PE32()
    e.dwSize = ctypes.sizeof(PE32)
    ok = k32.Process32FirstW(snap, ctypes.byref(e))
    while ok:
        out.append((e.th32ProcessID, e.th32ParentProcessID, e.szExeFile))
        ok = k32.Process32NextW(snap, ctypes.byref(e))
    k32.CloseHandle(snap)
    return out


def descendants(pid):
    procs = processes()
    found, todo = [], [pid]
    while todo:
        p = todo.pop()
        for (c, par, name) in procs:
            if par == p and c != p:
                found.append((c, name))
                todo.append(c)
    return found


def image_path(pid):
    h = k32.OpenProcess(0x1000, False, pid)
    if not h:
        return ""
    buf = ctypes.create_unicode_buffer(520)
    n = W.DWORD(520)
    k32.QueryFullProcessImageNameW(h, 0, buf, ctypes.byref(n))
    k32.CloseHandle(h)
    return buf.value.lower()


def other_game_processes(mine):
    """The game processes on the machine that are not this run's: executables of the repository's builds and of the staged originals."""
    skip = {"rundesk.exe", "origrun.exe", "cmake.exe", "ninja.exe", "python.exe", "bash.exe", "lsprobe.exe"}
    keys = ("\\build\\native", "\\build\\baseline", "\\byo-og\\", "\\build\\origrun\\", "\\build\\web")
    n = 0
    for (pid, _, name) in processes():
        if pid in mine or name.lower() in skip:
            continue
        p = image_path(pid)
        if p and any(k in p for k in keys):
            n += 1
    return n


def filetime_epoch(ft):
    return (((ft.dwHighDateTime << 32) | ft.dwLowDateTime) / 1e7) - 11644473600.0


class Proc:
    def __init__(self, pid):
        self.pid = pid
        self.h = k32.OpenProcess(0x1000 | 0x0400 | 0x0010 | 0x0001, False, pid)  # limited + query + vm read + terminate
        if not self.h:
            raise OSError("OpenProcess %d: %d" % (pid, ctypes.get_last_error()))
        c, e, kt, ut = W.FILETIME(), W.FILETIME(), W.FILETIME(), W.FILETIME()
        k32.GetProcessTimes(self.h, ctypes.byref(c), ctypes.byref(e), ctypes.byref(kt), ctypes.byref(ut))
        self.created = filetime_epoch(c)

    def base(self, exe_name):
        """The load address of the module with the exe's name (the module list can be empty right after the process starts)"""
        for _ in range(1000):
            mods = (ctypes.c_void_p * 1024)()
            need = W.DWORD()
            if psapi.EnumProcessModulesEx(self.h, mods, ctypes.sizeof(mods), ctypes.byref(need), 3):
                for i in range(min(need.value // ctypes.sizeof(ctypes.c_void_p), 1024)):
                    if not mods[i]:
                        continue
                    buf = ctypes.create_unicode_buffer(520)
                    psapi.GetModuleFileNameExW(self.h, ctypes.c_void_p(mods[i]), buf, 520)
                    if os.path.basename(buf.value).lower() == exe_name.lower():
                        return mods[i]
            time.sleep(0.01)
        raise OSError("%s not in the module list of %d" % (exe_name, self.pid))

    def terminate(self):
        k32.TerminateProcess(self.h, 1)

    def read32(self, addr):
        buf = ctypes.c_uint32()
        n = ctypes.c_size_t()
        if not k32.ReadProcessMemory(self.h, ctypes.c_void_p(addr), ctypes.byref(buf), 4, ctypes.byref(n)):
            return None
        return buf.value

    def memory(self):
        m = PMC()
        m.cb = ctypes.sizeof(PMC)
        psapi.GetProcessMemoryInfo(self.h, ctypes.byref(m), m.cb)
        return m

    def cpu(self):
        c, e, kt, ut = W.FILETIME(), W.FILETIME(), W.FILETIME(), W.FILETIME()
        k32.GetProcessTimes(self.h, ctypes.byref(c), ctypes.byref(e), ctypes.byref(kt), ctypes.byref(ut))
        f = lambda t: ((t.dwHighDateTime << 32) | t.dwLowDateTime) / 1e7
        return f(kt) + f(ut)


class Sampler(threading.Thread):
    """Every 20 ms: the game's update counter, working set, private bytes and processor time"""

    def __init__(self, proc, frame_addr):
        super().__init__(daemon=True)
        self.proc, self.frame_addr = proc, frame_addr
        self.stop_flag = False
        self.samples = []       # (seconds since creation, counter, working set, private bytes, cpu seconds)
        self.frame = 0
        self.loaded_at = None   # seconds since creation, the counter first above 1

    def run(self):
        while not self.stop_flag:
            t = time.time() - self.proc.created
            f = self.proc.read32(self.frame_addr)
            if f is not None:
                self.frame = f
                if self.loaded_at is None and f >= 2:
                    self.loaded_at = t
            m = self.proc.memory()
            self.samples.append((t, self.frame, m.WorkingSetSize, m.PrivateUsage, self.proc.cpu()))
            time.sleep(0.02)


def symbol_addr(exe, base, name_part):
    out = subprocess.check_output([NM, str(exe)], text=True)
    for line in out.splitlines():
        parts = line.split()
        if len(parts) == 3 and parts[2] == name_part:
            return int(parts[0], 16) - 0x400000 + base
    raise RuntimeError("no symbol " + name_part)


def wait_for_game(parent_pid, exe_name, timeout=60):
    end = time.time() + timeout
    while time.time() < end:
        for (pid, name) in descendants(parent_pid):
            if name.lower() == exe_name.lower():
                return pid
        time.sleep(0.01)
    raise TimeoutError("no " + exe_name)


def summarize(s, label):
    MB = 1 << 20
    smp = s.samples
    res = {"label": label, "loaded_s": None if s.loaded_at is None else round(s.loaded_at, 3)}

    def at_counter(n):
        for x in smp:
            if x[1] >= n:
                return x
        return None

    if smp:
        res["whole_run"] = {"peak_working_set_mb": round(max(x[2] for x in smp) / MB, 1),
                            "peak_private_mb": round(max(x[3] for x in smp) / MB, 1), "cpu_s": round(smp[-1][4], 2),
                            "seconds": round(smp[-1][0], 1), "last_counter": smp[-1][1]}
    if s.loaded_at is not None:
        sel = [x for x in smp if x[0] <= s.loaded_at]
        res["load"] = {"peak_working_set_mb": round(max(x[2] for x in sel) / MB, 1), "peak_private_mb": round(max(x[3] for x in sel) / MB, 1),
                       "cpu_s": round(sel[-1][4], 2)}
    for name, (lo, hi) in WINDOWS.items():
        a, b = at_counter(lo), at_counter(hi)
        if a is None or b is None:
            res[name] = None
            continue
        sel = [x for x in smp if a[0] <= x[0] <= b[0]]
        wall = b[0] - a[0]
        res[name] = {"updates": b[1] - a[1], "seconds": round(wall, 2), "cpu_s": round(b[4] - a[4], 3),
                     "cpu_percent_of_a_core": round(100 * (b[4] - a[4]) / wall, 1),
                     "peak_working_set_mb": round(max(x[2] for x in sel) / MB, 1), "peak_private_mb": round(max(x[3] for x in sel) / MB, 1)}
    return res


def run_port(args):
    sys.path.insert(0, str(ROOT / "games/cosmicbugs/tools/play"))
    import drive
    drive.T0 = time.time()
    exe = Path(args.exe).resolve()
    out = Path(args.out)
    (out / "user").mkdir(parents=True, exist_ok=True)
    (out / "user" / "cp.cfg").write_text(CONFIG)
    before = other_game_processes(set())
    g = drive.Game(str(out), str(exe), fast=False)
    try:
        proc = Proc(g.proc.pid)
        base = proc.base(exe.name)
        s = Sampler(proc, symbol_addr(exe, base, "__ZN2cb6gFrameE"))
        s.start()
        todo = list(CLICKS)
        checks = []
        if args.verify:
            todo += [(6000, None, None), (30000, None, None)]
            todo.sort()
        up = None
        t0 = time.time()
        while s.frame < END and time.time() - t0 < 240 and g.proc.poll() is None:
            if up is not None and s.frame >= up[0]:
                g.ask("up game %d %d" % (up[1], up[2]))
                up = None
            elif up is None and todo and s.frame >= todo[0][0]:
                (_, x, y) = todo.pop(0)
                if x is None:
                    checks.append("update %d: %s" % (s.frame, g.state()["screen"]))
                    continue
                g.ask("down game %d %d" % (x, y))
                up = (s.frame + 30, x, y)
            time.sleep(0.005)
        st = g.state()
        final = {"screen": st["screen"], "level": st.get("level"), "lives": st.get("lives"), "shooter": st.get("shooter"), "updates": st["updates"]}
        s.stop_flag = True
        s.join()
        res = summarize(s, args.label)
        res["final_state"] = final
        res["checks"] = checks
        res["others_before"] = before
        res["others_after"] = other_game_processes({proc.pid})
        return res
    finally:
        g.close()


def run_orig(args):
    out = Path(args.out)
    out.mkdir(parents=True, exist_ok=True)
    run = "cbperf"
    game = ROOT / "build/origrun" / run / "game"
    if not game.is_dir():
        subprocess.run([str(ORIGRUN), "--game", "cosmicbugs", "--run", run, "--copy-only"], cwd=str(ROOT), stdout=subprocess.DEVNULL,
                       stderr=subprocess.STDOUT, check=True)
    for leftover in ("cb.dat",):
        if (game / leftover).exists():
            (game / leftover).unlink()
    (game / "cp.cfg").write_text(CONFIG)
    script = out / "orig-script.txt"
    lines = ["# generated by perf.py: the game's updates (--counter) time the clicks"]
    lines += ["at %d click %d %d 30" % c for c in CLICKS]
    script.write_text("\n".join(lines) + "\n")
    cmd = [str(ORIGRUN), "--exe", str(game / "CosmicBugs.exe"), "--run", run, "--script", str(script), "--cursor", "scripted",
           "--counter", hex(ORIG_FRAME), "--update-ms", "1", "--clock", "real", "--timeout", "240"]
    if args.verify:  # the kind of the current level (0 a board, 1 the title), read from the game's memory at each change
        cmd += ["--watch", "[0x473138]+0x8"]
    if args.paced:
        # the main loop (0x41ee60) draws and presents with Present(1), which waits for the display's refresh; the hidden desktop's
        # DirectDraw does not wait, so the unpatched original spins. With --paced N the loop's timing of the Present call
        # (a TimeMs call and a subtraction, 11 bytes at 0x41f063, whose result nothing reads) is a SleepEx(N, 0), as a 60 Hz
        # screen's wait would make the pass last N ms
        patches = out / "orig-paced.txt"
        patches.write_text("test CosmicBugs.exe va:0x41f063 e8089e00002b051c656e00 6a006a%02xff156081450090 "
                           "each pass of the main loop sleeps %d ms, as the wait for a 60 Hz screen's refresh would\n" % (args.paced, args.paced))
        cmd += ["--patches", str(patches)]
    before = other_game_processes(set())
    logf = open(out / "origrun.log", "wb")
    p = subprocess.Popen(cmd, stdout=logf, stderr=subprocess.STDOUT, cwd=str(ROOT))
    owned = []
    try:
        pid = wait_for_game(p.pid, "CosmicBugs.exe", 120)
        proc = Proc(pid)
        owned.append(proc)
        for (cpid, par, name) in processes():
            if cpid == pid and par:
                try:
                    owned.append(Proc(par))  # the rundesk origrun started the game under
                except OSError:
                    pass
        s = Sampler(proc, ORIG_FRAME)
        s.start()
        t0 = time.time()
        while p.poll() is None and s.frame < END + 200 and time.time() - t0 < 240:
            time.sleep(0.05)
        final = {"updates": proc.read32(ORIG_FRAME)}
        s.stop_flag = True
        s.join()
        res = summarize(s, args.label)
        res["final_state"] = final
        if args.verify:
            res["checks"] = [ln.strip() for ln in (out / "origrun.log").read_text(errors="replace").splitlines() if "watch 1:" in ln][:14]
        res["others_before"] = before
        res["others_after"] = other_game_processes({pid, p.pid})
        return res
    finally:
        for pr in owned:
            pr.terminate()
        for (cpid, cname) in descendants(p.pid):
            try:
                Proc(cpid).terminate()
            except OSError:
                pass
        if p.poll() is None:
            try:
                p.wait(timeout=60)
            except subprocess.TimeoutExpired:
                p.kill()
        logf.close()


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("kind", choices=["port", "orig"])
    ap.add_argument("--exe", default=str(ROOT / "build/native/cosmicbugs.exe"))
    ap.add_argument("--label", default=None)
    ap.add_argument("--paced", type=int, default=0, help="orig: the original's main loop sleeps this many ms a pass (16 for a 60 Hz screen)")
    ap.add_argument("--verify", action="store_true", help="check that the clicks reached the board (the original's level kind watched, the port's screen read at update 6000 and 30000); not for a measured run")
    ap.add_argument("--out", default=None)
    args = ap.parse_args()
    import faulthandler
    faulthandler.dump_traceback_later(400, exit=True)  # a run that hangs says where, then ends
    args.label = args.label or args.kind
    args.out = args.out or str(ROOT / "build/cbperf" / args.label)
    res = run_port(args) if args.kind == "port" else run_orig(args)
    Path(args.out).mkdir(parents=True, exist_ok=True)
    Path(args.out, "result.json").write_text(json.dumps(res, indent=1))
    print("RESULT " + json.dumps(res))


main()
