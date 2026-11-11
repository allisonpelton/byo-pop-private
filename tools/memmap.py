#!/usr/bin/env python3
"""memmap.py: what a running process's memory is made of, read from its memory map.

  python -I tools/memmap.py <pid> [--json FILE] [--top N]

It walks the process's address space with VirtualQueryEx and totals the committed regions by kind:

  images    executables and libraries, by module (the game's exe, the runtime, SDL, the graphics driver's libraries ...)
  mapped    files mapped as data (fonts, locale tables) and sections shared with other processes
  stacks    the threads' stacks (an allocation with a guard page)
  private   everything else the process allocated for itself: its heaps, the large blocks an allocator takes straight
            from the system, and what libraries and the graphics driver allocate in the process. "heapStarts" is the
            part that lies in the allocations the process's heaps begin in (read from its heap list); a heap's later
            segments and its large blocks are not marked as such by the system, so they count under "other".

It prints one summary line and the same numbers as JSON (to FILE, or to stdout after the line). Sizes are bytes of
committed address space. Images and mapped files are shared with other processes until written to, so the process's
private bytes (printed beside them, from the system's own counter) are about stacks + private plus the image pages it
has changed.

It only reads: it opens the process to query it and to read the two words of its heap list, injects nothing and
suspends nothing, so it works on a port and on an original running under origrun alike (give it the game's process id;
tools/gamecount.py lists the game processes). Windows only; run it with a 64-bit Python.
"""
import argparse, ctypes, ctypes.wintypes as W, json, os, sys

PROCESS_QUERY_INFORMATION = 0x0400
PROCESS_VM_READ = 0x0010
MEM_COMMIT = 0x1000
MEM_RESERVE = 0x2000
MEM_IMAGE = 0x1000000
MEM_MAPPED = 0x40000
MEM_PRIVATE = 0x20000
PAGE_GUARD = 0x100


class MEMORY_BASIC_INFORMATION(ctypes.Structure):
    _fields_ = [('BaseAddress', ctypes.c_void_p), ('AllocationBase', ctypes.c_void_p), ('AllocationProtect', W.DWORD),
                ('PartitionId', W.WORD), ('RegionSize', ctypes.c_size_t), ('State', W.DWORD), ('Protect', W.DWORD),
                ('Type', W.DWORD)]


class PROCESS_MEMORY_COUNTERS_EX(ctypes.Structure):
    _fields_ = [('cb', W.DWORD), ('PageFaultCount', W.DWORD), ('PeakWorkingSetSize', ctypes.c_size_t),
                ('WorkingSetSize', ctypes.c_size_t), ('QuotaPeakPagedPoolUsage', ctypes.c_size_t),
                ('QuotaPagedPoolUsage', ctypes.c_size_t), ('QuotaPeakNonPagedPoolUsage', ctypes.c_size_t),
                ('QuotaNonPagedPoolUsage', ctypes.c_size_t), ('PagefileUsage', ctypes.c_size_t),
                ('PeakPagefileUsage', ctypes.c_size_t), ('PrivateUsage', ctypes.c_size_t)]


class PROCESS_BASIC_INFORMATION(ctypes.Structure):
    _fields_ = [('ExitStatus', ctypes.c_void_p), ('PebBaseAddress', ctypes.c_void_p), ('AffinityMask', ctypes.c_void_p),
                ('BasePriority', ctypes.c_void_p), ('UniqueProcessId', ctypes.c_void_p),
                ('InheritedFromUniqueProcessId', ctypes.c_void_p)]


def read(k32, handle, address, size):
    buf = ctypes.create_string_buffer(size)
    got = ctypes.c_size_t(0)
    if not k32.ReadProcessMemory(handle, ctypes.c_void_p(address), buf, size, ctypes.byref(got)) or got.value != size:
        return None
    return buf.raw


def heap_starts(k32, ntdll, handle):
    """The base addresses of the process's heaps, from the heap list in its environment block (two reads)."""
    wow = W.BOOL(0)
    k32.IsWow64Process(handle, ctypes.byref(wow))
    if wow.value:
        peb = ctypes.c_void_p(0)        # ProcessWow64Information: the 32-bit environment block
        if ntdll.NtQueryInformationProcess(handle, 26, ctypes.byref(peb), ctypes.sizeof(peb), None) != 0 or not peb.value:
            return set()
        count_at, list_at, word = peb.value + 0x88, peb.value + 0x90, 4
    else:
        info = PROCESS_BASIC_INFORMATION()
        if ntdll.NtQueryInformationProcess(handle, 0, ctypes.byref(info), ctypes.sizeof(info), None) != 0 or not info.PebBaseAddress:
            return set()
        count_at, list_at, word = info.PebBaseAddress + 0xe8, info.PebBaseAddress + 0xf0, 8
    raw = read(k32, handle, count_at, 4)
    ptr = read(k32, handle, list_at, word)
    if raw is None or ptr is None:
        return set()
    count = int.from_bytes(raw, 'little')
    table = read(k32, handle, int.from_bytes(ptr, 'little'), count * word) if 0 < count < 4096 else None
    if table is None:
        return set()
    return {int.from_bytes(table[i * word:(i + 1) * word], 'little') for i in range(count)}


def walk(pid):
    k32 = ctypes.WinDLL('kernel32', use_last_error=True)
    ntdll = ctypes.WinDLL('ntdll')
    k32.OpenProcess.restype = W.HANDLE
    k32.OpenProcess.argtypes = [W.DWORD, W.BOOL, W.DWORD]
    k32.VirtualQueryEx.restype = ctypes.c_size_t
    k32.VirtualQueryEx.argtypes = [W.HANDLE, ctypes.c_void_p, ctypes.POINTER(MEMORY_BASIC_INFORMATION), ctypes.c_size_t]
    k32.ReadProcessMemory.argtypes = [W.HANDLE, ctypes.c_void_p, ctypes.c_void_p, ctypes.c_size_t, ctypes.POINTER(ctypes.c_size_t)]
    k32.K32GetMappedFileNameW.argtypes = [W.HANDLE, ctypes.c_void_p, W.LPWSTR, W.DWORD]
    k32.K32GetProcessMemoryInfo.argtypes = [W.HANDLE, ctypes.POINTER(PROCESS_MEMORY_COUNTERS_EX), W.DWORD]
    k32.K32GetProcessImageFileNameW.argtypes = [W.HANDLE, W.LPWSTR, W.DWORD]
    k32.IsWow64Process.argtypes = [W.HANDLE, ctypes.POINTER(W.BOOL)]
    k32.CloseHandle.argtypes = [W.HANDLE]
    ntdll.NtQueryInformationProcess.argtypes = [W.HANDLE, ctypes.c_int, ctypes.c_void_p, ctypes.c_ulong, ctypes.c_void_p]
    handle = k32.OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, False, pid)
    if not handle:
        raise OSError('cannot open process %d (error %d)' % (pid, ctypes.get_last_error()))
    try:
        name = ctypes.create_unicode_buffer(1024)
        exe = os.path.basename(name.value) if k32.K32GetProcessImageFileNameW(handle, name, 1024) else ''
        heaps = heap_starts(k32, ntdll, handle)
        regions = []            # (allocation base, size, type, protect, file name)
        mbi = MEMORY_BASIC_INFORMATION()
        address = 0
        reserved = 0
        while k32.VirtualQueryEx(handle, ctypes.c_void_p(address), ctypes.byref(mbi), ctypes.sizeof(mbi)):
            base = mbi.BaseAddress or 0
            if mbi.State == MEM_COMMIT:
                file_name = ''
                if mbi.Type in (MEM_IMAGE, MEM_MAPPED) and k32.K32GetMappedFileNameW(handle, ctypes.c_void_p(base), name, 1024):
                    file_name = os.path.basename(name.value)
                regions.append((mbi.AllocationBase or 0, mbi.RegionSize, mbi.Type, mbi.Protect, file_name))
            elif mbi.State == MEM_RESERVE:
                reserved += mbi.RegionSize
            address = base + mbi.RegionSize
            if address >= 0x7fffffff0000:
                break
        counters = PROCESS_MEMORY_COUNTERS_EX()
        counters.cb = ctypes.sizeof(counters)
        k32.K32GetProcessMemoryInfo(handle, ctypes.byref(counters), counters.cb)
    finally:
        k32.CloseHandle(handle)

    stack_allocations = {r[0] for r in regions if r[2] == MEM_PRIVATE and r[3] & PAGE_GUARD}
    images, files = {}, {}
    sections = stacks = heap_bytes = other = 0
    for allocation, size, kind, protect, file_name in regions:
        if kind == MEM_IMAGE:
            images[file_name or '(unnamed)'] = images.get(file_name or '(unnamed)', 0) + size
        elif kind == MEM_MAPPED:
            if file_name:
                files[file_name] = files.get(file_name, 0) + size
            else:
                sections += size
        elif allocation in stack_allocations:
            stacks += size
        elif allocation in heaps:
            heap_bytes += size
        else:
            other += size
    return {
        'pid': pid, 'exe': exe,
        'images': {'total': sum(images.values()), 'modules': dict(sorted(images.items(), key=lambda t: -t[1]))},
        'mapped': {'total': sum(files.values()) + sections, 'sections': sections, 'files': dict(sorted(files.items(), key=lambda t: -t[1]))},
        'stacks': stacks,
        'private': {'total': heap_bytes + other, 'heapStarts': heap_bytes, 'other': other},
        'reserved': reserved,
        'process': {'private': counters.PrivateUsage, 'workingSet': counters.WorkingSetSize},
    }


def main():
    ap = argparse.ArgumentParser(description=__doc__.split('\n')[0])
    ap.add_argument('pid', type=int)
    ap.add_argument('--json', metavar='FILE', help='write the JSON there instead of printing it')
    ap.add_argument('--top', type=int, default=5, help='how many of the largest modules the summary line names (default 5)')
    args = ap.parse_args()
    if sys.platform != 'win32':
        sys.exit('memmap.py reads a Windows process')
    if ctypes.sizeof(ctypes.c_void_p) != 8:
        sys.exit('memmap.py needs a 64-bit Python')
    try:
        m = walk(args.pid)
    except OSError as e:
        sys.exit(str(e))
    mb = lambda n: '%.1f' % (n / 1048576.0)
    top = ', '.join('%s %s' % (k, mb(v)) for k, v in list(m['images']['modules'].items())[:args.top])
    print('memmap %d %s: images %s MB in %d modules (%s), mapped %s MB, stacks %s MB, private %s MB (%s in the heaps\' first allocations); '
          'the process\'s private bytes %s MB, working set %s MB' % (
              m['pid'], m['exe'], mb(m['images']['total']), len(m['images']['modules']), top, mb(m['mapped']['total']), mb(m['stacks']),
              mb(m['private']['total']), mb(m['private']['heapStarts']), mb(m['process']['private']), mb(m['process']['workingSet'])))
    text = json.dumps(m, indent=1)
    if args.json:
        open(args.json, 'w').write(text + '\n')
    else:
        print(text)


if __name__ == '__main__':
    main()
