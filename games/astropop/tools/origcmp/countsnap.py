"""The original's --count counters, read from outside by a run that does not go through origrun (visrun3d.py's, on the desktop).

origrun's `--count <list>` (docs/ORIGRUN.md, "Counting cycles inside the original's functions") makes the sound stand-in count the cycles the
game's threads spend in the functions of a list, in a block of shared memory (tools/origrun/count.h), and origrun reads the block, with every
thread's cycles, at a script's `count <label>` lines into counts.jsonl, the file tools/cyclesplit.py reports. visrun3d.py starts the staged copy
itself (the game's 3D needs the visible desktop), so this module does the same two things from Python:

    resolve_list(exe, list_file, out_file)   the function list with the first bytes of each function read out of the exe, the form the
                                             stand-in takes (origrun.c, count_resolve_list)
    Counter(process_handle, map_name)        .snapshot(label, update, out_file) appends one JSON line in origrun's format
                                             (origrun.c, count_snapshot): the parts' calls and cycles summed over the threads, the process's
                                             cycles, and every other thread with its name and the module its start address is in

The game gets two environment variables, ORIGRUN_COUNT (the resolved list's path) and ORIGRUN_COUNT_MAP (the block's name); the block exists
once the stand-in has loaded, and nothing is held while it is read, as origrun's.
"""
import ctypes
import ctypes.wintypes as wt
import json
import mmap
import struct
import time

COUNT_MAGIC = 0x4E43524F
MAXFUNC = 16
MAXTHREAD = 16
NAMELEN = 24
OFF_ERROR = 24
OFF_NAME = 144
OFF_ADDR = OFF_NAME + MAXFUNC * NAMELEN
OFF_THREAD = OFF_ADDR + MAXFUNC * 4
THREAD_SIZE = 8 + MAXFUNC * 8 * 2 + 8
SIZE = OFF_THREAD + MAXTHREAD * THREAD_SIZE

k32 = ctypes.WinDLL('kernel32', use_last_error=True)
psapi = ctypes.WinDLL('psapi', use_last_error=True)
ntdll = ctypes.WinDLL('ntdll')

k32.OpenThread.restype = wt.HANDLE
k32.OpenThread.argtypes = [wt.DWORD, wt.BOOL, wt.DWORD]
k32.CloseHandle.argtypes = [wt.HANDLE]
k32.CreateToolhelp32Snapshot.restype = wt.HANDLE
k32.CreateToolhelp32Snapshot.argtypes = [wt.DWORD, wt.DWORD]
k32.QueryThreadCycleTime.argtypes = [wt.HANDLE, ctypes.POINTER(ctypes.c_ulonglong)]
k32.QueryProcessCycleTime.argtypes = [wt.HANDLE, ctypes.POINTER(ctypes.c_ulonglong)]
k32.GetProcessId.argtypes = [wt.HANDLE]
k32.LocalFree.argtypes = [ctypes.c_void_p]
psapi.EnumProcessModulesEx.argtypes = [wt.HANDLE, ctypes.c_void_p, wt.DWORD, ctypes.POINTER(wt.DWORD), wt.DWORD]
psapi.GetModuleBaseNameA.argtypes = [wt.HANDLE, ctypes.c_void_p, ctypes.c_char_p, wt.DWORD]


class THREADENTRY32(ctypes.Structure):
    _fields_ = [('dwSize', wt.DWORD), ('cntUsage', wt.DWORD), ('th32ThreadID', wt.DWORD), ('th32OwnerProcessID', wt.DWORD),
                ('tpBasePri', wt.LONG), ('tpDeltaPri', wt.LONG), ('dwFlags', wt.DWORD)]


class MODULEINFO(ctypes.Structure):
    _fields_ = [('lpBaseOfDll', ctypes.c_void_p), ('SizeOfImage', wt.DWORD), ('EntryPoint', ctypes.c_void_p)]


psapi.GetModuleInformation.argtypes = [wt.HANDLE, ctypes.c_void_p, ctypes.POINTER(MODULEINFO), wt.DWORD]
k32.Thread32First.argtypes = [wt.HANDLE, ctypes.POINTER(THREADENTRY32)]
k32.Thread32Next.argtypes = [wt.HANDLE, ctypes.POINTER(THREADENTRY32)]
ntdll.NtQueryInformationThread.argtypes = [wt.HANDLE, ctypes.c_int, ctypes.c_void_p, wt.ULONG, ctypes.c_void_p]


def resolve_list(exe, list_file, out_file):
    """origrun.c count_resolve_list: `label addr len=N` lines become `label 0xaddr <N bytes of the exe at addr>`."""
    b = open(exe, 'rb').read()
    pe = struct.unpack_from('<I', b, 0x3c)[0]
    nsec = struct.unpack_from('<H', b, pe + 6)[0]
    opt = struct.unpack_from('<H', b, pe + 20)[0]
    base = struct.unpack_from('<I', b, pe + 24 + 28)[0]
    secs = []
    for i in range(nsec):
        o = pe + 24 + opt + 40 * i
        vsize, va, rsize, raw = struct.unpack_from('<IIII', b, o + 8)
        secs.append((va, max(vsize, rsize), raw))
    out = []
    for line in open(list_file):
        line = line.split('#')[0].split()
        if len(line) < 3 or not line[2].startswith('len='):
            continue
        label, addr, n = line[0], int(line[1], 16), int(line[2][4:])
        rva = addr - base
        off = next((raw + rva - va for va, size, raw in secs if va <= rva < va + size), None)
        if off is None or n < 5 or n > 32:
            raise SystemExit('countsnap: %s at 0x%x len=%d is not in %s' % (label, addr, n, exe))
        out.append('%s 0x%08x %s' % (label, addr, ' '.join('%02x' % c for c in b[off:off + n])))
    if not out:
        raise SystemExit('countsnap: %s lists no function' % list_file)
    open(out_file, 'w', newline='\n').write('\n'.join(out) + '\n')


class Counter:
    def __init__(self, proc, map_name):
        self.proc = proc
        self.map_name = map_name
        self.mm = None
        self.t0 = None

    def ready(self):
        """True once the stand-in has made the block and put its counters in place."""
        if self.mm is None:
            try:
                self.mm = mmap.mmap(-1, SIZE, tagname=self.map_name)
            except OSError:
                return False
        magic, nfunc = struct.unpack_from('<II', self.mm, 0)
        return magic == COUNT_MAGIC and nfunc > 0

    def error(self):
        raw = bytes(self.mm[OFF_ERROR:OFF_ERROR + 120]).split(b'\0')[0]
        return raw.decode('ascii', 'replace')

    def copy(self):
        for _ in range(500):
            a = bytes(self.mm[:SIZE])
            b = bytes(self.mm[:SIZE])
            ok = True
            for i in range(MAXTHREAD):
                s1 = struct.unpack_from('<I', a, OFF_THREAD + i * THREAD_SIZE)[0]
                s2 = struct.unpack_from('<I', b, OFF_THREAD + i * THREAD_SIZE)[0]
                if (s1 & 1) or s1 != s2:
                    ok = False
            if ok:
                return a
        return None

    def modules(self):
        mods = (ctypes.c_void_p * 512)()
        need = wt.DWORD(0)
        if not psapi.EnumProcessModulesEx(self.proc, mods, ctypes.sizeof(mods), ctypes.byref(need), 3):
            return []
        out = []
        for i in range(min(need.value // ctypes.sizeof(ctypes.c_void_p), 512)):
            mi = MODULEINFO()
            if not psapi.GetModuleInformation(self.proc, mods[i], ctypes.byref(mi), ctypes.sizeof(mi)):
                continue
            name = ctypes.create_string_buffer(64)
            if not psapi.GetModuleBaseNameA(self.proc, mods[i], name, 64):
                name.value = b'?'
            out.append((mi.lpBaseOfDll or 0, mi.SizeOfImage, name.value.decode('ascii', 'replace').lower()))
        return out

    def snapshot(self, label, update, out_file):
        raw = self.copy()
        if raw is None:
            return False
        magic, nfunc, nthread, misses, paused, read_cost = struct.unpack_from('<6I', raw, 0)
        names = [bytes(raw[OFF_NAME + k * NAMELEN:OFF_NAME + (k + 1) * NAMELEN]).split(b'\0')[0].decode('ascii', 'replace') for k in range(nfunc)]
        calls = [0] * nfunc
        cycles = [0] * nfunc
        readings = read_tsc = 0
        main_tid, main_counted = 0, -1
        for i in range(MAXTHREAD):
            o = OFF_THREAD + i * THREAD_SIZE
            seq, tid = struct.unpack_from('<II', raw, o)
            if not tid:
                continue
            c = struct.unpack_from('<%dQ' % MAXFUNC, raw, o + 8)
            y = struct.unpack_from('<%dQ' % MAXFUNC, raw, o + 8 + MAXFUNC * 8)
            tsc = struct.unpack_from('<Q', raw, o + 8 + 2 * MAXFUNC * 8)[0]
            counted = 0
            read_tsc += tsc
            for k in range(nfunc):
                calls[k] += c[k]
                cycles[k] += y[k]
                counted += y[k]
                readings += 2 * c[k]
            if counted >= main_counted:
                main_counted, main_tid = counted, tid
        if self.t0 is None:
            self.t0 = time.time()
        pid = k32.GetProcessId(self.proc)
        pc = ctypes.c_ulonglong(0)
        k32.QueryProcessCycleTime(self.proc, ctypes.byref(pc))
        mods = self.modules()
        describe = getattr(k32, 'GetThreadDescription', None)
        threads = []
        main_cycles = 0
        snap = k32.CreateToolhelp32Snapshot(4, 0)    # TH32CS_SNAPTHREAD
        if snap and snap != wt.HANDLE(-1).value:
            te = THREADENTRY32()
            te.dwSize = ctypes.sizeof(te)
            more = k32.Thread32First(snap, ctypes.byref(te))
            while more:
                if te.th32OwnerProcessID == pid:
                    th = k32.OpenThread(0x0040 | 0x0800, False, te.th32ThreadID)    # THREAD_QUERY_INFORMATION | QUERY_LIMITED
                    if th:
                        c = ctypes.c_ulonglong(0)
                        if k32.QueryThreadCycleTime(th, ctypes.byref(c)):
                            name = ''
                            if describe:
                                w = ctypes.c_wchar_p()
                                if describe(th, ctypes.byref(w)) >= 0 and w.value:
                                    name = ''.join(ch if ord(ch) < 128 and ch not in '"\\' else '?' for ch in w.value[:40])
                                    k32.LocalFree(ctypes.cast(w, ctypes.c_void_p))
                            start = ctypes.c_void_p(0)
                            text = ''
                            if ntdll.NtQueryInformationThread(th, 9, ctypes.byref(start), ctypes.sizeof(start), None) == 0:
                                a = start.value or 0
                                for base, size, mname in mods:
                                    if base <= a < base + size:
                                        text = '%s+0x%x' % (mname, a - base)
                                        break
                                if not text:
                                    text = '0x%x' % a
                            if te.th32ThreadID == main_tid:
                                main_cycles = c.value
                            else:
                                threads.append({'tid': te.th32ThreadID, 'name': name, 'start': text, 'cycles': c.value})
                        k32.CloseHandle(th)
                more = k32.Thread32Next(snap, ctypes.byref(te))
            k32.CloseHandle(snap)
        rec = {'label': label, 'update': update if update is not None else -1, 'wall_s': round(time.time() - self.t0, 6),
               'process_cycles': pc.value, 'main_thread_id': main_tid, 'main_thread_cycles': main_cycles,
               'parts': {names[k]: {'calls': calls[k], 'cycles': cycles[k]} for k in range(nfunc)},
               'counter': {'calls': readings, 'cycles_each': (read_tsc // readings) if readings else read_cost, 'cycles_in_a_row': read_cost},
               'misses': misses, 'threads': threads}
        with open(out_file, 'a', newline='\n') as f:
            f.write(json.dumps(rec, separators=(',', ':')) + '\n')
        return True
