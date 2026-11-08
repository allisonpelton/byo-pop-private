#!/usr/bin/env python
"""perf.py: load time, memory and processor time of TipTop, the original against the port (GAME_NOTES.md, "Performance").

  perf.py port --exe build/native/tiptop.exe --label after [--seconds 60] [--out DIR] [--modsdir DIR]
  perf.py orig --label original [--seconds 60] [--out DIR]

Both run in real time (the port without -fast, the original on the wall clock), invisibly and silently: the port under
rundesk, the original through tools/origrun (which puts it under rundesk itself), with only the patches the original needs
to play as the registered game and to leave out the MIDI music (build/tt/perf/tiptop-perf.txt, written by `orig`).

Load time: from the game process's creation to the moment the title screen's load bar is full (the bar's loaded count,
read from the process's memory every 2 ms, equals its total): the same variable in both, the original's at 0x489eb4 and
0x489eb8, the port's found through the exe's symbols (llvm-nm) and the module's base address.

The scripted minute: the click sequence of phase two's tour 2 (games/tiptop/tools/shots/tour2.txt), the same real clicks for
both: the title's button, the menu's Play, Regular on the new game screen, the tips' button, then one click on a column every
610 ms, 100 of them. It starts at the moment the title accepts its first click. The window measured is the 60 seconds
from the game screen's first appearance (the screen value, 0x4887ec in the original; the driver's own state in the port).
Memory (peak working set, peak private bytes) and processor time (user + kernel seconds) are sampled every 100 ms over the
whole run and over the window; the other game processes on the machine are counted at the start and the end.
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
sys.path.insert(0, str(ROOT / "games/tiptop/tools"))
NM = "E:/retools/llvm-mingw-20261006-ucrt-x86_64/bin/llvm-nm.exe"
ORIGRUN = ROOT / "build/origrun-bin/origrun.exe"
STAGE_ROOT = "C:\\byo-og"
STAGE_RUN = "ttperf"
SCREEN_GAME = 6

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
    keys = ("\\build\\native\\", "\\build\\baseline", "\\byo-og\\", "\\build\\origrun\\", "\\build\\web", "\\build\\tt\\")
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
        self.h = k32.OpenProcess(0x1000 | 0x0400 | 0x0010, False, pid)  # limited + query + vm read
        if not self.h:
            raise OSError("OpenProcess %d: %d" % (pid, ctypes.get_last_error()))
        c, e, kt, ut = W.FILETIME(), W.FILETIME(), W.FILETIME(), W.FILETIME()
        k32.GetProcessTimes(self.h, ctypes.byref(c), ctypes.byref(e), ctypes.byref(kt), ctypes.byref(ut))
        self.created = filetime_epoch(c)

    def base(self):
        # the exe is the first module; right after the process starts the list can still be empty
        for _ in range(500):
            mods = (ctypes.c_void_p * 8)()
            need = W.DWORD()
            if psapi.EnumProcessModulesEx(self.h, mods, ctypes.sizeof(mods), ctypes.byref(need), 3) and mods[0]:
                return mods[0]
            time.sleep(0.01)
        raise OSError("no module list for %d (error %d)" % (self.pid, ctypes.get_last_error()))

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
    """Samples the process every 2 ms for the load bar and the screen, every 100 ms for memory and processor time."""

    def __init__(self, proc, loaded_addr, count_addr, screen_addr=None):
        super().__init__(daemon=True)
        self.proc, self.loaded_addr, self.count_addr, self.screen_addr = proc, loaded_addr, count_addr, screen_addr
        self.stop_flag = False
        self.samples = []          # (seconds since creation, working set, private bytes, cpu seconds)
        self.loaded_at = None      # seconds since creation, the bar full
        self.game_at = None        # the game screen first seen (originals: read from memory)
        self.last_mem = 0.0

    def run(self):
        while not self.stop_flag:
            now = time.time()
            t = now - self.proc.created
            if self.loaded_at is None:
                a, b = self.proc.read32(self.loaded_addr), self.proc.read32(self.count_addr)
                if a is not None and b and a == b:
                    self.loaded_at = t
            if self.screen_addr and self.game_at is None and self.proc.read32(self.screen_addr) == SCREEN_GAME:
                self.game_at = t
            if now - self.last_mem >= 0.1:
                self.last_mem = now
                m = self.proc.memory()
                self.samples.append((t, m.WorkingSetSize, m.PrivateUsage, self.proc.cpu()))
            time.sleep(0.002)


def symbol_addr(exe, base, name_part):
    out = subprocess.check_output([NM, str(exe)], text=True)
    for line in out.splitlines():
        parts = line.split()
        if len(parts) == 3 and name_part in parts[2]:
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


# the scripted minute: the clicks of tour 2 after the tips, as columns, and its first steps (seconds after the first click)
COLUMNS = [317, 245, 389, 353, 281, 425, 353, 317, 353, 317, 317, 353, 497, 317, 281, 461, 461, 389, 461, 245, 497, 353, 353,
           353, 425, 317, 281, 317, 425, 281]
STEPS = [(0.0, 186, 288), (1.01, 186, 288), (2.02, 186, 288), (3.04, 165, 88), (4.06, 232, 185), (5.63, 500, 388)]
FIRST_COLUMN = 6.64
COLUMN_EVERY = 0.61
NCOLUMNS = 100


def schedule():
    sched = list(STEPS)
    for i in range(NCOLUMNS):
        sched.append((FIRST_COLUMN + i * COLUMN_EVERY, COLUMNS[i % len(COLUMNS)], 234))
    return sched


def summarize(s, label, window):
    MB = 1 << 20
    res = {"label": label, "loaded_s": s.loaded_at, "game_at_s": s.game_at}
    samples = s.samples

    def peak(lo, hi):
        sel = [x for x in samples if lo <= x[0] <= hi]
        if not sel:
            return None
        return {"peak_working_set_mb": round(max(x[1] for x in sel) / MB, 1), "peak_private_mb": round(max(x[2] for x in sel) / MB, 1)}

    def cpu_between(lo, hi):
        a = [x for x in samples if x[0] <= lo]
        b = [x for x in samples if x[0] <= hi]
        if not a or not b:
            return None
        return round(b[-1][3] - a[-1][3], 3)

    if samples:
        res["whole_run"] = peak(0, 1e9)
        res["whole_run"]["cpu_s"] = round(samples[-1][3], 3)
        res["whole_run"]["seconds"] = round(samples[-1][0], 1)
    if s.loaded_at is not None:
        res["load"] = peak(0, s.loaded_at)
        res["load"]["cpu_s"] = cpu_between(0, s.loaded_at)
    if window:
        lo, hi = window
        res["window"] = peak(lo, hi)
        res["window"]["cpu_s"] = cpu_between(lo, hi)
        res["window"]["seconds"] = round(hi - lo, 2)
    return res


def run_port(args):
    import realplay
    realplay.EXE = Path(args.exe).resolve()
    out = Path(args.out)
    extra = ["-modsdir=" + str(Path(args.modsdir).resolve())] if args.modsdir else []
    mine_before = other_game_processes(set())
    g = realplay.Game(out, seed=1, fast=False, extra=extra)
    try:
        pid = wait_for_game(g.p.pid, realplay.EXE.name)
        proc = Proc(pid)
        base = proc.base()
        s = Sampler(proc, symbol_addr(realplay.EXE, base, "gLoadBarLoaded"), symbol_addr(realplay.EXE, base, "gLoadBarCount"))
        s.start()
        # the first click of the minute goes the moment the bar is full; the title's button ignores it before that
        while s.loaded_at is None:
            time.sleep(0.002)
        t0 = time.time()
        sched = schedule()
        game_at = None
        for (when, x, y) in sched:
            delay = t0 + when - time.time()
            if delay > 0:
                time.sleep(delay)
            g.send(["click game %d %d" % (x, y)])
            if game_at is None and when >= FIRST_COLUMN:
                st = g.state()
                if st["screen"] == "board":
                    game_at = time.time() - proc.created
                    s.game_at = game_at
        end_at = (game_at or 0) + args.seconds
        while time.time() - proc.created < end_at:
            time.sleep(0.05)
        st = g.state()
        final = {"screen": st["screen"], "score": st.get("score"), "level": st.get("level"), "over": st.get("over")}
        s.stop_flag = True
        s.join()
        window = (game_at, game_at + args.seconds) if game_at else None
        res = summarize(s, args.label, window)
        res["final_state"] = final
        res["others_before"] = mine_before
        res["others_after"] = other_game_processes({pid, g.p.pid})
        res["updates"] = st.get("updates")
        return res
    finally:
        g.close()


def run_orig(args):
    out = Path(args.out)
    out.mkdir(parents=True, exist_ok=True)
    patches = ROOT / "build/tt/perf/tiptop-perf.txt"
    stage_cmd = [str(ORIGRUN), "--game", "tiptop", "--run", STAGE_RUN, "--stage-root", STAGE_ROOT, "--copy-only"]
    subprocess.run(stage_cmd, cwd=str(ROOT), stdout=subprocess.DEVNULL, stderr=subprocess.STDOUT, check=True)
    for leftover in ("Preferences.dat", "savegame.dat"):
        f = Path(STAGE_ROOT) / STAGE_RUN / "game" / leftover
        if f.exists():
            f.unlink()
    script = out / "orig-script.txt"
    lines = ["# generated by perf.py", "wait %d" % int(args.orig_load * 1000)]
    prev = 0.0
    for (when, x, y) in schedule():
        w = int((when - prev) * 1000)
        prev = when
        if w > 0:
            lines.append("wait %d" % w)
        lines += ["move %d %d" % (x, y), "click %d %d" % (x, y)]
    lines += ["wait %d" % int((args.seconds + 5) * 1000), "quit"]
    script.write_text("\n".join(lines) + "\n")
    cmd = [str(ORIGRUN), "--game", "tiptop", "--run", STAGE_RUN, "--stage-root", STAGE_ROOT, "--patches", str(patches), "--seed", "1",
           "--clock", "real", "--cursor", "scripted", "--sound", "stub", "--timeout", str(int(args.orig_load + 6.64 + 61 + args.seconds + 40)),
           "--script", str(script)]
    mine_before = other_game_processes(set())
    logf = open(out / "origrun.log", "wb")
    p = subprocess.Popen(cmd, stdout=logf, stderr=subprocess.STDOUT, cwd=str(ROOT))
    try:
        pid = wait_for_game(p.pid, "TipTop.exe", 120)
        proc = Proc(pid)
        s = Sampler(proc, 0x489eb4, 0x489eb8, 0x4887ec)
        s.start()
        while p.poll() is None and (s.game_at is None or time.time() - proc.created < s.game_at + args.seconds):
            time.sleep(0.05)
        final = {"screen": proc.read32(0x4887ec)}
        s.stop_flag = True
        s.join()
        window = (s.game_at, s.game_at + args.seconds) if s.game_at else None
        res = summarize(s, args.label, window)
        res["final_state"] = final
        res["others_before"] = mine_before
        res["others_after"] = other_game_processes({pid, p.pid})
        return res
    finally:
        if p.poll() is None:
            # the script ends with quit; give it a moment, then stop only our own origrun
            try:
                p.wait(timeout=args.seconds + 60)
            except subprocess.TimeoutExpired:
                p.kill()
        logf.close()


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("kind", choices=["port", "orig"])
    ap.add_argument("--exe", default=str(ROOT / "build/native/tiptop.exe"))
    ap.add_argument("--label", default=None)
    ap.add_argument("--out", default=None)
    ap.add_argument("--seconds", type=float, default=60)
    ap.add_argument("--modsdir", default=None)
    ap.add_argument("--orig-load", type=float, default=8.0, help="seconds the original's script waits before its first click")
    args = ap.parse_args()
    args.label = args.label or args.kind
    args.out = args.out or str(ROOT / "build/tt/perf" / args.label)
    res = run_port(args) if args.kind == "port" else run_orig(args)
    Path(args.out).mkdir(parents=True, exist_ok=True)
    Path(args.out, "result.json").write_text(json.dumps(res, indent=1))
    print("RESULT " + json.dumps(res))


main()
