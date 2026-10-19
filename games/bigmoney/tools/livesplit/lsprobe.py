#!/usr/bin/env python3
"""Read Big Money's LiveSplit block from the running game, the way the auto splitter does (Windows).

    lsprobe.py --exe <path to bigmoney.exe> --log <file> [--interval-ms 10] [--wait-seconds 60]
    lsprobe.py --check <probe log> <game log>

The first form waits for a process of that exe (started after the probe), then, like
BigMoneyDeluxe-byopop.asl: the main module's PE header from the game's memory, each writable section scanned at
4-byte steps for the magic, the game id, the layout and size, and self (0 or the hit's own address); then it
polls until the process ends: seq, the 0xB0 bytes, seq again; a copy counts when both seqs are equal and even
and the copy's own seq is the same (up to 4 tries, else "torn"). Each poll is a line
"t=<ms> ok <fields> hex=<block>" or "t=<ms> torn" (the fields as the game's -livesplitlog writes them, with
self and the clock masked), which lsdecide.py reads like a game log. Like LiveSplit, it is a 64-bit process
reading the 32-bit game.

--check: every snapshot the probe accepted must be one the game wrote (its fields equal a line of the game's
-livesplitlog), in the order the game wrote them, or the block as the exe holds it before the game's first
write (seq 0); prints how many polls, distinct snapshots and torn reads.
"""
import argparse
import ctypes
import ctypes.wintypes as wt
import struct
import sys
import time

MAGIC = bytes([0x42, 0x59, 0x4F, 0x50, 0x4F, 0x50, 0x2D, 0x4C, 0x49, 0x56, 0x45, 0x53, 0x50, 0x4C, 0x49, 0x54,
               0xF0, 0xF1, 0xF2, 0xF3])
GAME_ID = 0x58444D42
LAYOUT, SIZE = 1, 0xB0

PROCESS_QUERY_LIMITED_INFORMATION = 0x1000
PROCESS_VM_READ = 0x0010
PROCESS_QUERY_INFORMATION = 0x0400
TH32CS_SNAPPROCESS = 0x2
LIST_MODULES_32BIT = 0x01

k32 = ctypes.WinDLL("kernel32", use_last_error=True)
psapi = ctypes.WinDLL("psapi", use_last_error=True)


class PROCESSENTRY32W(ctypes.Structure):
    _fields_ = [("dwSize", wt.DWORD), ("cntUsage", wt.DWORD), ("th32ProcessID", wt.DWORD),
                ("th32DefaultHeapID", ctypes.c_size_t), ("th32ModuleID", wt.DWORD), ("cntThreads", wt.DWORD),
                ("th32ParentProcessID", wt.DWORD), ("pcPriClassBase", ctypes.c_long), ("dwFlags", wt.DWORD),
                ("szExeFile", ctypes.c_wchar * 260)]


class MODULEINFO(ctypes.Structure):
    _fields_ = [("lpBaseOfDll", ctypes.c_void_p), ("SizeOfImage", wt.DWORD), ("EntryPoint", ctypes.c_void_p)]


k32.OpenProcess.restype = wt.HANDLE
k32.CreateToolhelp32Snapshot.restype = wt.HANDLE
k32.ReadProcessMemory.argtypes = [wt.HANDLE, ctypes.c_void_p, ctypes.c_void_p, ctypes.c_size_t, ctypes.POINTER(ctypes.c_size_t)]
k32.QueryFullProcessImageNameW.argtypes = [wt.HANDLE, wt.DWORD, ctypes.c_wchar_p, ctypes.POINTER(wt.DWORD)]
psapi.EnumProcessModulesEx.argtypes = [wt.HANDLE, ctypes.POINTER(wt.HMODULE), wt.DWORD, ctypes.POINTER(wt.DWORD), wt.DWORD]
psapi.GetModuleInformation.argtypes = [wt.HANDLE, wt.HMODULE, ctypes.POINTER(MODULEINFO), wt.DWORD]


def processes():
    snap = k32.CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0)
    entry = PROCESSENTRY32W()
    entry.dwSize = ctypes.sizeof(entry)
    out = []
    ok = k32.Process32FirstW(snap, ctypes.byref(entry))
    while ok:
        out.append((entry.th32ProcessID, entry.szExeFile))
        ok = k32.Process32NextW(snap, ctypes.byref(entry))
    k32.CloseHandle(snap)
    return out


def image_path(pid):
    h = k32.OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, False, pid)
    if not h:
        return None
    buf = ctypes.create_unicode_buffer(1024)
    n = wt.DWORD(1024)
    ok = k32.QueryFullProcessImageNameW(h, 0, buf, ctypes.byref(n))
    k32.CloseHandle(h)
    return buf.value if ok else None


def find_pid(exe, seen_before):
    target = exe.replace("/", "\\").lower()
    for pid, name in processes():
        if pid in seen_before or name.lower() != "bigmoney.exe":
            continue
        path = image_path(pid)
        if path and path.lower() == target:
            return pid
    return None


def read(h, address, size):
    buf = ctypes.create_string_buffer(size)
    got = ctypes.c_size_t(0)
    if not k32.ReadProcessMemory(h, ctypes.c_void_p(address), buf, size, ctypes.byref(got)) or got.value != size:
        return None
    return buf.raw


def main_module(h):
    mods = (wt.HMODULE * 1)()
    needed = wt.DWORD(0)
    if not psapi.EnumProcessModulesEx(h, mods, ctypes.sizeof(mods), ctypes.byref(needed), LIST_MODULES_32BIT):
        return None, 0
    info = MODULEINFO()
    if not psapi.GetModuleInformation(h, mods[0], ctypes.byref(info), ctypes.sizeof(info)):
        return None, 0
    return info.lpBaseOfDll, info.SizeOfImage


def find_block(h):
    """As the script's FindBlock"""
    base, size = main_module(h)
    if not base:
        return 0
    header = read(h, base, min(0x1000, size))
    if not header or header[:2] != b"MZ":
        return 0
    pe = struct.unpack_from("<i", header, 0x3C)[0]
    if pe <= 0 or pe + 24 > len(header) or struct.unpack_from("<I", header, pe)[0] != 0x4550:
        return 0
    sections = struct.unpack_from("<H", header, pe + 6)[0]
    table = pe + 24 + struct.unpack_from("<H", header, pe + 20)[0]
    for i in range(sections):
        entry = table + i * 40
        if entry + 40 > len(header):
            break
        vsize, vaddr = struct.unpack_from("<ii", header, entry + 8)
        chars = struct.unpack_from("<I", header, entry + 36)[0]
        if not chars & 0x80000000 or vsize <= 0 or vaddr <= 0 or vaddr + vsize > size:
            continue
        data = read(h, base + vaddr, vsize)
        if not data:
            continue
        at = data.find(MAGIC)
        while at >= 0:
            if at % 4 == 0 and at + SIZE <= len(data):
                gid, layout, bsize, selfaddr = struct.unpack_from("<IHHI", data, at + 0x14)
                address = base + vaddr + at
                if gid == GAME_ID and layout >= LAYOUT and bsize >= SIZE and selfaddr in (0, address):
                    return address
            at = data.find(MAGIC, at + 1)
    return 0


def format_block(b):
    """The game's FormatLiveSplitBlock with self and the clock masked"""
    v = struct.unpack_from("<IIII", b, 0x1C)  # self seq active updates
    w = struct.unpack_from("<12I", b, 0x2C)    # flags .. deathsOther
    x = struct.unpack_from("<11i", b, 0x5C)    # tick .. skill
    key = "".join(chr(c) if 0x20 <= c < 0x7F else "-" for c in b[0x88:0x90])
    return ("seq=%u active=%u updates=%u flags=%02x board=%u go=%u/%u lc=%u/%u over=%u/%u deaths=%u,%u,%u,%u "
            "tick=%d state=%d level=%d dlevel=%d dscore=%d score=%d blc=%d gotick=%d dsmax=%d mode=%d skill=%d key=%s "
            "self=* qpc=* qpcf=*") % ((v[1], v[2], v[3]) + w + x + (key,))


def probe(exe, log, interval, wait):
    before = {pid for pid, _ in processes()}
    t0 = time.time()
    pid = None
    while pid is None:
        if time.time() - t0 > wait:
            sys.exit("lsprobe: no %s started" % exe)
        pid = find_pid(exe, before)
        time.sleep(0.05)
    h = k32.OpenProcess(PROCESS_VM_READ | PROCESS_QUERY_INFORMATION, False, pid)
    if not h:
        sys.exit("lsprobe: cannot open process %d" % pid)
    address = 0
    polls = torn = 0
    start = time.perf_counter()
    exit_code = wt.DWORD(0)
    with open(log, "w") as out:
        while True:
            if not k32.GetExitCodeProcess(h, ctypes.byref(exit_code)) or exit_code.value != 259:  # STILL_ACTIVE
                break
            t = int((time.perf_counter() - start) * 1000)
            if not address:
                address = find_block(h)
                if address:
                    out.write("# block at 0x%08x\n" % address)
            if address:
                line = None
                for _ in range(4):
                    s1, block, s2 = read(h, address + 0x20, 4), read(h, address, SIZE), read(h, address + 0x20, 4)
                    if s1 is None or block is None or s2 is None:
                        break
                    q1 = struct.unpack("<I", s1)[0]
                    if q1 == struct.unpack("<I", s2)[0] and not q1 & 1 and struct.unpack_from("<I", block, 0x20)[0] == q1:
                        line = "t=%d ok %s hex=%s" % (t, format_block(block), block.hex())
                        break
                if line is None:
                    torn += 1
                    line = "t=%d torn" % t
                out.write(line + "\n")
                polls += 1
            time.sleep(interval / 1000.0)
    k32.CloseHandle(h)
    print("lsprobe: pid %d, block 0x%08x, %d polls, %d torn" % (pid, address, polls, torn))


def check(probe_log, game_log):
    game = []
    with open(game_log, encoding="latin-1") as f:
        for line in f:
            if line.startswith("u="):
                game.append(line.split(" ", 1)[1].rsplit(" hex=", 1)[0])
    index = {}
    for i, fields in enumerate(game):
        index.setdefault(fields, []).append(i)
    polls = torn = distinct = bad = initial = 0
    last = -1
    seen = set()
    with open(probe_log, encoding="latin-1") as f:
        for line in f:
            if not line.startswith("t="):
                continue
            polls += 1
            words = line.split(" ", 2)
            if words[1] == "torn":
                torn += 1
                continue
            fields = words[2].rsplit(" hex=", 1)[0]
            if fields.startswith("seq=0 ") and last < 0:
                initial += 1
                continue
            hits = [i for i in index.get(fields, []) if i >= last]
            if not hits:
                bad += 1
                if bad <= 5:
                    print("not written by the game (or out of order): " + fields[:160])
                continue
            last = hits[0]
            if fields not in seen:
                seen.add(fields)
                distinct += 1
    print("%d polls: %d before the first write, %d distinct snapshots, %d torn, %d not the game's; the game wrote %d" % (
        polls, initial, distinct, torn, bad, len(game)))
    return 1 if bad or distinct == 0 else 0


def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("--exe")
    ap.add_argument("--log")
    ap.add_argument("--interval-ms", type=int, default=10)
    ap.add_argument("--wait-seconds", type=int, default=60)
    ap.add_argument("--check", nargs=2, metavar=("PROBE_LOG", "GAME_LOG"))
    a = ap.parse_args()
    if a.check:
        return check(*a.check)
    probe(a.exe, a.log, a.interval_ms, a.wait_seconds)
    return 0


if __name__ == "__main__":
    sys.exit(main())
