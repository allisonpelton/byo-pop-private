"""What a running game's private memory is made of, at moments after its start (docs/MEMORY.md, the Mahjong Escapes' memory): for the
original through origrun (--attach <path part>, as perfmin.py) or a port (-- <exe> <args>, run it under rundesk). At each of the
given seconds after the process was created it reads the process's memory map (tools/memmap.py's walk) and prints: the totals by kind,
the private regions of 256 KB or more (their sizes and protection, the largest first), the sections shared with the system (a
DirectDraw surface in system memory is one) and the private bytes. Everything goes to <out.json>.

  python -I games/mahjongjapan/tools/profile/origmem.py <out.json> <t1,t2,...> --attach <path part>
  python -I games/mahjongjapan/tools/profile/origmem.py <out.json> <t1,t2,...> -- <exe> <args...>
"""
import ctypes
import ctypes.wintypes as W
import json
import os
import subprocess
import sys
import time

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..', '..', '..', 'tools'))
import memmap  # noqa: E402

k32 = ctypes.WinDLL('kernel32', use_last_error=True)
k32.OpenProcess.restype = W.HANDLE
k32.CreateToolhelp32Snapshot.restype = W.HANDLE
PROCESS_QUERY_LIMITED, PROCESS_QUERY, PROCESS_VM_READ = 0x1000, 0x400, 0x10
MB = 1 << 20


class PROCESSENTRY32W(ctypes.Structure):
    _fields_ = [('dwSize', W.DWORD), ('cntUsage', W.DWORD), ('th32ProcessID', W.DWORD), ('th32DefaultHeapID', ctypes.c_size_t),
                ('th32ModuleID', W.DWORD), ('cntThreads', W.DWORD), ('th32ParentProcessID', W.DWORD), ('pcPriClassBase', W.LONG),
                ('dwFlags', W.DWORD), ('szExeFile', ctypes.c_wchar * 260)]


def find(part):
    """The id of a process whose exe path contains part (lower case, forward slashes), or 0."""
    snap = k32.CreateToolhelp32Snapshot(0x2, 0)
    pe = PROCESSENTRY32W()
    pe.dwSize = ctypes.sizeof(pe)
    found = 0
    ok = k32.Process32FirstW(snap, ctypes.byref(pe))
    while ok and not found:
        if pe.szExeFile.lower().endswith('.exe') and pe.th32ProcessID:
            h = k32.OpenProcess(PROCESS_QUERY_LIMITED, False, pe.th32ProcessID)
            if h:
                buf = ctypes.create_unicode_buffer(1024)
                n = W.DWORD(1024)
                if k32.QueryFullProcessImageNameW(h, 0, buf, ctypes.byref(n)) and part in buf.value.lower().replace('\\', '/'):
                    found = pe.th32ProcessID
                k32.CloseHandle(h)
        ok = k32.Process32NextW(snap, ctypes.byref(pe))
    k32.CloseHandle(snap)
    return found


def filetime(t):
    return ((t.dwHighDateTime << 32) | t.dwLowDateTime) / 1e7


def created_at(h):
    a, b, c, d = W.FILETIME(), W.FILETIME(), W.FILETIME(), W.FILETIME()
    k32.GetProcessTimes(h, ctypes.byref(a), ctypes.byref(b), ctypes.byref(c), ctypes.byref(d))
    return filetime(a)


def now():
    t = W.FILETIME()
    k32.GetSystemTimeAsFileTime(ctypes.byref(t))
    return filetime(t)


def regions_of(pid):
    """The committed regions as (base, size, type, protect, allocation base)."""
    k32.VirtualQueryEx.restype = ctypes.c_size_t
    k32.VirtualQueryEx.argtypes = [W.HANDLE, ctypes.c_void_p, ctypes.POINTER(memmap.MEMORY_BASIC_INFORMATION), ctypes.c_size_t]
    h = k32.OpenProcess(PROCESS_QUERY | PROCESS_VM_READ, False, pid)
    out = []
    mbi = memmap.MEMORY_BASIC_INFORMATION()
    address = 0
    while k32.VirtualQueryEx(h, ctypes.c_void_p(address), ctypes.byref(mbi), ctypes.sizeof(mbi)):
        base = mbi.BaseAddress or 0
        if mbi.State == memmap.MEM_COMMIT:
            out.append((base, mbi.RegionSize, mbi.Type, mbi.Protect, mbi.AllocationBase or 0))
        address = base + mbi.RegionSize
        if address >= 0x7fffffff0000:
            break
    k32.CloseHandle(h)
    return out


def snapshot(pid, t):
    walked = memmap.walk(pid)
    regions = regions_of(pid)
    private = [r for r in regions if r[2] == memmap.MEM_PRIVATE and not (r[3] & memmap.PAGE_GUARD)]
    big = sorted((r for r in private if r[1] >= 256 * 1024), key=lambda r: -r[1])
    sections = sorted((r for r in regions if r[2] == memmap.MEM_MAPPED), key=lambda r: -r[1])
    snap = {
        't': t, 'privateBytes': walked['process']['private'], 'workingSet': walked['process']['workingSet'],
        'images': walked['images']['total'], 'mapped': walked['mapped']['total'], 'stacks': walked['stacks'],
        'privateRegions': walked['private']['total'],
        'big': [[r[1], hex(r[3])] for r in big[:40]],
        'bigTotal': sum(r[1] for r in big), 'smallTotal': sum(r[1] for r in private if r[1] < 256 * 1024),
        'sections': [[r[1], hex(r[3])] for r in sections if r[1] >= 256 * 1024][:20],
    }
    print('t=%.1f s: private bytes %.1f MB, working set %.1f MB; images %.1f, mapped %.1f, stacks %.1f, private regions %.1f '
          '(%.1f in %d regions of 256 KB or more, %.1f in the rest)' % (
              t, snap['privateBytes'] / MB, snap['workingSet'] / MB, snap['images'] / MB, snap['mapped'] / MB, snap['stacks'] / MB,
              snap['privateRegions'] / MB, snap['bigTotal'] / MB, len(big), snap['smallTotal'] / MB))
    print('   largest private regions (KB):', ' '.join('%d' % (r[1] // 1024) for r in big[:24]))
    print('   mapped sections of 256 KB or more (KB):', ' '.join('%d' % (r[1] // 1024) for r in sections if r[1] >= 256 * 1024))
    sys.stdout.flush()
    return snap


def main():
    out = sys.argv[1]
    times = [float(x) for x in sys.argv[2].split(',')]
    proc = None
    if sys.argv[3] == '--attach':
        part = sys.argv[4].lower().replace('\\', '/')
        pid = 0
        while not pid:
            pid = find(part)
            time.sleep(0.005)
    else:
        proc = subprocess.Popen(sys.argv[4:], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        pid = proc.pid
    h = k32.OpenProcess(PROCESS_QUERY_LIMITED, False, pid)
    started = created_at(h)
    snaps = []
    for t in times:
        while now() - started < t:
            time.sleep(0.02)
        try:
            snaps.append(snapshot(pid, now() - started))
        except OSError as e:
            print('t=%.1f: %s' % (t, e))
            break
    json.dump(snaps, open(out, 'w'), indent=1)
    if proc is not None:
        proc.terminate()


main()
