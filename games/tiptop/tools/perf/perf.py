#!/usr/bin/env python
"""perf.py: load time, memory and processor time of TipTop, the original against the port (GAME_NOTES.md, "Performance").

  perf.py port --exe build/native/tiptop.exe --label after [--seconds 60] [--out DIR] [--modsdir DIR]
  perf.py orig --label original [--seconds 60] [--out DIR]
  perf.py portscript --exe ... --label after [--seconds 30] [--window-from 8]   (the port without a commands or real-input file)

Both run in real time (the port without -fast, the original on the wall clock), invisibly and silently: the port under
rundesk, the original through tools/origrun (which puts it under rundesk itself), with only the patches the original needs
to play as the registered game and to leave out the MIDI music (build/tt/perf/tiptop-perf.txt, made by patches.sh from the
identical-play setup's list); `orig --paced 16` also makes its main loop sleep 16 ms a pass, as the wait for a 60 Hz screen's
vertical blank would (the hidden desktop's DirectDraw does not wait, so the unpatched original spins).

Load time: from the game process's creation to the moment the title screen's load bar is full (the bar's loaded count,
read from the process's memory every 2 ms, equals its total): the same variable in both, the original's at 0x489eb4 and
0x489eb8, the port's found through the exe's symbols (llvm-nm) and the module's base address.

The scripted minute: the first clicks of phase two's tour 2 (games/tiptop/tools/shots/tour2.txt), the same real clicks for
both: the title's button, the menu's Play, the mode's button on the new game screen (--mode, regular by default), the tips'
button, then one click on a column every --every seconds, --count of them. A regular game piles up and ends about 33 s after its
first click however slowly the columns are clicked; the strategy mode (--mode strategy --every 2.4 --count 25) waits for the
player and mostly lasts the minute. It starts when the title accepts its first click: the port's driver clicks the moment the load bar is full;
the original's script waits --orig-load seconds (the load time that `orig` measured in an earlier run, plus a margin), since a
click that came before the bar was full is lost and a second one would land on the menu. The window measured is the 60 seconds
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
SCREEN_GAMEOVER = 10

k32 = ctypes.WinDLL("kernel32", use_last_error=True)
psapi = ctypes.WinDLL("psapi", use_last_error=True)
k32.OpenProcess.restype = W.HANDLE
k32.OpenProcess.argtypes = [W.DWORD, W.BOOL, W.DWORD]
k32.ReadProcessMemory.argtypes = [W.HANDLE, ctypes.c_void_p, ctypes.c_void_p, ctypes.c_size_t, ctypes.POINTER(ctypes.c_size_t)]
k32.QueryFullProcessImageNameW.argtypes = [W.HANDLE, W.DWORD, W.LPWSTR, ctypes.POINTER(W.DWORD)]
k32.CloseHandle.argtypes = [W.HANDLE]
k32.CreateToolhelp32Snapshot.restype = W.HANDLE
k32.CreateToolhelp32Snapshot.argtypes = [W.DWORD, W.DWORD]
k32.OpenThread.restype = W.HANDLE
k32.OpenThread.argtypes = [W.DWORD, W.BOOL, W.DWORD]
k32.QueryThreadCycleTime.argtypes = [W.HANDLE, ctypes.POINTER(ctypes.c_uint64)]
k32.Thread32First.argtypes = [W.HANDLE, ctypes.c_void_p]
k32.Thread32Next.argtypes = [W.HANDLE, ctypes.c_void_p]
k32.Process32FirstW.argtypes = [W.HANDLE, ctypes.c_void_p]
k32.Process32NextW.argtypes = [W.HANDLE, ctypes.c_void_p]
psapi.EnumProcessModulesEx.argtypes = [W.HANDLE, ctypes.POINTER(ctypes.c_void_p), W.DWORD, ctypes.POINTER(W.DWORD), W.DWORD]


class PE32(ctypes.Structure):
    _fields_ = [("dwSize", W.DWORD), ("cntUsage", W.DWORD), ("th32ProcessID", W.DWORD), ("th32DefaultHeapID", ctypes.c_size_t),
                ("th32ModuleID", W.DWORD), ("cntThreads", W.DWORD), ("th32ParentProcessID", W.DWORD),
                ("pcPriClassBase", ctypes.c_long), ("dwFlags", W.DWORD), ("szExeFile", ctypes.c_wchar * 260)]


class TE32(ctypes.Structure):
    _fields_ = [("dwSize", W.DWORD), ("cntUsage", W.DWORD), ("th32ThreadID", W.DWORD), ("th32OwnerProcessID", W.DWORD),
                ("tpBasePri", W.LONG), ("tpDeltaPri", W.LONG), ("dwFlags", W.DWORD)]


def thread_ids(pid):
    snap = k32.CreateToolhelp32Snapshot(4, 0)
    e = TE32()
    e.dwSize = ctypes.sizeof(e)
    out = []
    ok = k32.Thread32First(snap, ctypes.byref(e))
    while ok:
        if e.th32OwnerProcessID == pid:
            out.append(e.th32ThreadID)
        ok = k32.Thread32Next(snap, ctypes.byref(e))
    k32.CloseHandle(snap)
    return out


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
        self.threads = {}
        self.h = k32.OpenProcess(0x1000 | 0x0400 | 0x0010 | 0x0001, False, pid)  # limited + query + vm read + terminate
        if not self.h:
            raise OSError("OpenProcess %d: %d" % (pid, ctypes.get_last_error()))
        c, e, kt, ut = W.FILETIME(), W.FILETIME(), W.FILETIME(), W.FILETIME()
        k32.GetProcessTimes(self.h, ctypes.byref(c), ctypes.byref(e), ctypes.byref(kt), ctypes.byref(ut))
        self.created = filetime_epoch(c)

    def base(self, exe_name):
        """The load address of the module with the exe's name. Right after the process starts the list can be empty or hold
        only the 64-bit system modules of WOW64 (not the exe), so it is read again until the exe is in it."""
        psapi.GetModuleFileNameExW.argtypes = [W.HANDLE, ctypes.c_void_p, W.LPWSTR, W.DWORD]
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

    def thread_cycles(self):
        """{thread id: cycles used so far}, exact (QueryThreadCycleTime) and read without stopping any thread."""
        out = {}
        for tid in thread_ids(self.pid):
            h = self.threads.get(tid)
            if h is None:
                h = k32.OpenThread(0x0040 | 0x0800, False, tid)  # query information, query limited information
                if not h:
                    continue
                self.threads[tid] = h
            c = ctypes.c_uint64()
            if k32.QueryThreadCycleTime(h, ctypes.byref(c)):
                out[tid] = c.value
        return out

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
        self.bar_at = None         # seconds since creation, the bar created (its total is set)
        self.loaded_at = None      # seconds since creation, the bar full
        self.game_at = None        # the game screen first seen (originals: read from memory)
        self.over_at = None        # the game over screen first seen after it
        self.last_mem = 0.0
        self.last_threads = 0.0
        self.thread_samples = []   # (seconds since creation, {thread id: cycles})

    def run(self):
        while not self.stop_flag:
            now = time.time()
            t = now - self.proc.created
            if self.loaded_at is None:
                a, b = self.proc.read32(self.loaded_addr), self.proc.read32(self.count_addr)
                if b and self.bar_at is None:
                    self.bar_at = t
                if a is not None and b and a == b:
                    self.loaded_at = t
            if self.screen_addr:
                scr = self.proc.read32(self.screen_addr)
                if self.game_at is None and scr == SCREEN_GAME:
                    self.game_at = t
                if self.game_at is not None and self.over_at is None and scr == SCREEN_GAMEOVER:
                    self.over_at = t
            if now - self.last_mem >= 0.1:
                self.last_mem = now
                m = self.proc.memory()
                self.samples.append((t, m.WorkingSetSize, m.PrivateUsage, self.proc.cpu()))
            if now - self.last_threads >= 0.25:
                self.last_threads = now
                self.thread_samples.append((t, self.proc.thread_cycles()))
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
STEPS = [(0.0, 186, 288), (1.02, 165, 88), (2.04, 232, 185), (3.61, 500, 388)]  # the title's button, Play, Regular, the tips' button
FIRST_COLUMN = 4.62
PACE = {"every": 0.61, "count": 100}


MODE_BUTTON = {"regular": (232, 185), "strategy": (145, 260), "speed": (240, 325)}


def schedule():
    if PACE.get("menu"):
        # --menu: the title's button, then the main menu stays up, or (--menu-click X,Y) one more click on it 1.5 s later
        return [STEPS[0]] + ([(1.5,) + tuple(int(v) for v in PACE["menu_click"].split(","))] if PACE.get("menu_click") else [])
    sched = [(w, x, y) if (x, y) != (232, 185) else (w,) + MODE_BUTTON[PACE.get("mode", "regular")] for (w, x, y) in STEPS]
    for i in range(PACE["count"]):
        sched.append((FIRST_COLUMN + i * PACE["every"], COLUMNS[i % len(COLUMNS)], 234))
    return sched


def summarize(s, label, window):
    MB = 1 << 20
    res = {"label": label, "bar_created_s": s.bar_at, "loaded_s": s.loaded_at, "game_at_s": s.game_at, "over_at_s": s.over_at}
    if s.bar_at is not None and s.loaded_at is not None:
        res["bar_fill_s"] = round(s.loaded_at - s.bar_at, 3)
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

    def threads_between(lo, hi):
        """The cycles each thread used from lo to hi, in millions, largest first (the main thread is the lowest id)."""
        a = [x for x in s.thread_samples if x[0] <= lo]
        b = [x for x in s.thread_samples if x[0] <= hi]
        if not a or not b:
            return None
        first, last = a[-1][1], b[-1][1]
        rows = sorted(((last[tid] - first.get(tid, 0)) / 1e6, tid) for tid in last)
        main = min(last) if last else None
        return {"main_thread_id": main, "mcycles": [(round(c), tid) for c, tid in reversed(rows[-6:])],
                "total_mcycles": round(sum(c for c, _ in rows))}

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
        res["window"]["threads"] = threads_between(lo, hi)
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
        base = proc.base(realplay.EXE.name)
        s = Sampler(proc, symbol_addr(realplay.EXE, base, "gLoadBarLoaded"), symbol_addr(realplay.EXE, base, "gLoadBarCount"))
        s.start()
        # the first click of the minute goes the moment the bar is full; the title's button ignores it before that
        while s.loaded_at is None:
            time.sleep(0.002)
        t0 = time.time()
        events = schedule()
        i, last_poll = 0, 0.0
        game_at = over_at = None
        st = {}
        while True:
            now = time.time()
            if game_at is not None and (over_at is not None or now - proc.created >= game_at + args.seconds):
                break
            if i < len(events) and now >= t0 + events[i][0]:
                (_, x, y) = events[i]
                i += 1
                if over_at is None:
                    g.send(["click game %d %d" % (x, y)])
                continue
            if now - last_poll >= 1.0 and now >= t0 + 3.0:
                st = g.state()
                last_poll = time.time()
                if game_at is None and st["screen"] == "board":
                    game_at = time.time() - proc.created
                    s.game_at = game_at
                if game_at is not None and over_at is None and (st.get("over") or st["screen"] == "gameover"):
                    over_at = time.time() - proc.created
                    s.over_at = over_at
                continue
            if i >= len(events) and game_at is None and now > t0 + 90:
                break
            time.sleep(0.005)
        st = g.state()
        final = {"screen": st["screen"], "score": st.get("score"), "level": st.get("level"), "over": st.get("over")}
        s.stop_flag = True
        s.join()
        window = (game_at, min(game_at + args.seconds, over_at if over_at else 1e9)) if game_at else None
        res = summarize(s, args.label, window)
        res["final_state"] = final
        res["others_before"] = mine_before
        res["others_after"] = other_game_processes({pid, g.p.pid})
        res["updates"] = st.get("updates")
        return res
    finally:
        g.close()


def run_port_script(args):
    """The same scripted minute through -script (the clicks at fixed update numbers, 100 a second in real time), with no commands
    file and no real-input file: the framework polls those every update (a stat() of the commands file, about 340,000 cycles),
    a cost a player's copy never pays. The window is a stretch of wall time from the process's creation (--from, --seconds),
    chosen inside the minute the script plays (the game screen is up from about 6 s)."""
    out = Path(args.out)
    if out.exists():
        import shutil
        shutil.rmtree(out)
    (out / "user").mkdir(parents=True)
    exe = Path(args.exe).resolve()
    lines = []
    for (when, x, y) in schedule():
        # the steps' times are the driver's (the bar full at 0); the script's updates start at the process, the bar is full by about
        # update 100, so the first click is at update 102 as in tour 2
        lines.append("%d click %d %d" % (102 + int(when * 100), x, y))
    last = int(lines[-1].split()[0])
    if args.menu:
        last = 102 + int((args.window_from + args.seconds + 2) * 100)
    # the state where the window starts and ends, to see what the window held
    lines.append("%d state" % (102 + int(args.window_from * 100) + 100))
    lines.append("%d state" % (102 + int((args.window_from + args.seconds) * 100) - 100))
    for u in range(500, last, 500):  # a look at the game every 5 seconds
        lines.append("%d state" % u)
    lines.sort(key=lambda l: int(l.split()[0]))
    lines.append("%d state" % (last + 20))
    lines.append("%d quit" % (last + 40))
    script = out / "script.txt"
    script.write_text(chr(10).join(lines) + chr(10))
    cmd = [str(ROOT / "build/native/rundesk.exe"), str(exe), "-gamedir=" + str(ROOT / "assets/tiptop"), "-userdir=" + str(out / "user"),
           "-seed=1", "-script=" + str(script)]
    if args.modsdir:
        cmd.append("-modsdir=" + str(Path(args.modsdir).resolve()))
    cmd += args.port_arg
    mine_before = other_game_processes(set())
    logf = open(out / "log.txt", "wb")
    p = subprocess.Popen(cmd, stdout=logf, stderr=subprocess.STDOUT, cwd=str(out))
    try:
        pid = wait_for_game(p.pid, exe.name)
        proc = Proc(pid)
        s = Sampler(proc, 0, 0)
        s.start()
        lo, hi = args.window_from, args.window_from + args.seconds
        while time.time() - proc.created < hi + 1 and p.poll() is None:
            time.sleep(0.05)
        s.stop_flag = True
        s.join()
        res = summarize(s, args.label, (lo, hi))
        res["others_before"] = mine_before
        res["others_after"] = other_game_processes({pid, p.pid})
        res["script_updates"] = last
        states = []
        for l in (out / "log.txt").read_text(errors="replace").splitlines():
            if l.startswith("STATE "):
                st = json.loads(l[6:])
                states.append({k: st.get(k) for k in ("screen", "mode", "over", "updates", "level", "score")})
        res["states"] = states
        return res
    finally:
        for (cpid, cname) in descendants(p.pid):
            try:
                Proc(cpid).terminate()
            except OSError:
                pass
        if p.poll() is None:
            p.kill()
        logf.close()


def run_orig(args):
    out = Path(args.out)
    out.mkdir(parents=True, exist_ok=True)
    patches = ROOT / "build/tt/perf/tiptop-perf.txt"
    if args.paced:
        # a pass of the main loop sleeps args.paced ms, as the wait for a 60 Hz screen's vertical blank would (the hidden desktop's
        # DirectDraw does not wait for one, so the unpatched original spins); the loop's own Sleep(50) for an inactive window goes
        paced = ROOT / ("build/tt/perf/tiptop-perf-paced%d.txt" % args.paced)
        line = ("test TipTop.exe va:0x4128dc 0fb605f086480085c074086a32ff1520c14600 ff05f0cf48006a%02xff1520c146009090909090 "
                "each pass sleeps %d ms, as a 60 Hz screen's vertical blank would\n" % (args.paced, args.paced))
        paced.write_text(patches.read_text() + line)
        patches = paced
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
    if args.probe:
        lines = ["# probe: only wait for the load", "wait 25000", "quit"]
    script.write_text("\n".join(lines) + "\n")
    cmd = [str(ORIGRUN), "--game", "tiptop", "--run", STAGE_RUN, "--stage-root", STAGE_ROOT, "--patches", str(patches), "--seed", "1",
           "--clock", "real", "--cursor", "scripted", "--sound", "stub", "--timeout", str(int(args.orig_load + FIRST_COLUMN + 61 + args.seconds + 40)),
           "--script", str(script)]
    mine_before = other_game_processes(set())
    logf = open(out / "origrun.log", "wb")
    p = subprocess.Popen(cmd, stdout=logf, stderr=subprocess.STDOUT, cwd=str(ROOT))
    owned = []
    try:
        pid = wait_for_game(p.pid, "TipTop.exe", 120)
        proc = Proc(pid)
        owned.append(proc)
        for (cpid, par, name) in processes():
            if cpid == pid and par:
                owned.append(Proc(par))  # the rundesk origrun started the game under
        s = Sampler(proc, 0x489eb4, 0x489eb8, 0x4887ec)
        s.start()
        while p.poll() is None and s.over_at is None and (s.game_at is None or time.time() - proc.created < s.game_at + args.seconds) and not (args.menu and time.time() - proc.created > args.window_from + args.seconds + 1):
            if args.probe and s.loaded_at is not None and time.time() - proc.created > s.loaded_at + 2:
                break
            time.sleep(0.05)
        final = {"screen": proc.read32(0x4887ec)}
        s.stop_flag = True
        s.join()
        window = (s.game_at, min(s.game_at + args.seconds, s.over_at if s.over_at else 1e9)) if s.game_at else None
        if args.menu:
            window = (args.window_from, args.window_from + args.seconds)
        res = summarize(s, args.label, window)
        res["final_state"] = final
        res["others_before"] = mine_before
        res["others_after"] = other_game_processes({pid, p.pid})
        return res
    finally:
        # the script's quit only asks the window to close, and the game answers with its quit question, so the game and
        # the rundesk origrun put it under (both started by this run, found as its descendants) are ended here
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
    ap.add_argument("kind", choices=["port", "orig", "portscript"])
    ap.add_argument("--exe", default=str(ROOT / "build/native/tiptop.exe"))
    ap.add_argument("--label", default=None)
    ap.add_argument("--out", default=None)
    ap.add_argument("--seconds", type=float, default=60)
    ap.add_argument("--modsdir", default=None)
    ap.add_argument("--every", type=float, default=0.61, help="seconds between the clicks on a column")
    ap.add_argument("--count", type=int, default=100, help="how many column clicks the script holds")
    ap.add_argument("--mode", default="regular", choices=["regular", "strategy", "speed"], help="the mode the scripted minute plays")
    ap.add_argument("--paced", type=int, default=0, help="orig: the original's main loop sleeps this many ms a pass (16 for a 60 Hz screen)")
    ap.add_argument("--window-from", type=float, default=8.0, help="portscript: the window starts this many seconds after the process's creation")
    ap.add_argument("--port-arg", action="append", default=[], help="portscript: an extra argument of the port (-soundfont=<no such file> leaves the music out, as the original's is in these runs)")
    ap.add_argument("--menu", action="store_true", help="only the title's button: the window is the main menu left alone (orig and portscript; --window-from, --seconds from the process's creation)")
    ap.add_argument("--menu-click", default=None, help="with --menu: X,Y of one more click on the main menu (265,157 the options, 245,243 the instructions)")
    ap.add_argument("--probe", action="store_true", help="orig: only measure the load, play nothing")
    ap.add_argument("--orig-load", type=float, default=8.0, help="seconds the original's script waits before its first click")
    args = ap.parse_args()
    import faulthandler
    faulthandler.dump_traceback_later(int(args.seconds + 200), exit=True)  # a run that hangs says where, then ends
    args.label = args.label or args.kind
    PACE["every"], PACE["count"], PACE["mode"], PACE["menu"], PACE["menu_click"] = args.every, args.count, args.mode, args.menu, args.menu_click
    args.out = args.out or str(ROOT / "build/tt/perf" / args.label)
    res = {"port": run_port, "orig": run_orig, "portscript": run_port_script}[args.kind](args)
    Path(args.out).mkdir(parents=True, exist_ok=True)
    Path(args.out, "result.json").write_text(json.dumps(res, indent=1))
    print("RESULT " + json.dumps(res))


main()
