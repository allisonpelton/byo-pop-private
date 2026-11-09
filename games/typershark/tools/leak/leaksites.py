"""leaksites.py: reads the leak_<tag>.sites files the counting build writes (src/testhooks/LeakCount.cpp, docs/LEAKS.md).

  leaksites.py <exe> show <file> [-n N]          the live blocks by allocation site, largest count first, frames as function names
  leaksites.py <exe> diff <fileA> <fileB> [-n N] the sites whose live count grew from A to B (a leak per repeat), then those that shrank
  leaksites.py <exe> total <file> [<file> ...]   the live blocks and bytes of each file (the sum of its sites)

<exe> is the counting build's typershark.exe (a frame is module+rva; the executable's rva is looked up in the output of nm,
at the preferred image base 0x400000; frames of the DLLs are shown as module+rva). A frame that is a test or counter function
is left out when the site has other frames.
"""
import bisect
import os
import re
import subprocess
import sys

NM = r'E:\retools\llvm-mingw-20261006-ucrt-x86_64\bin\llvm-nm.exe'
SYMBOLIZER = NM.replace('llvm-nm', 'llvm-symbolizer')
IMAGE_BASE = 0x400000


def load_symbols(exe, min_addr=IMAGE_BASE):
    out = subprocess.run([NM, '-n', '--demangle', '--defined-only', exe], capture_output=True, text=True).stdout
    addrs, names = [], []
    for line in out.splitlines():
        m = re.match(r'([0-9a-fA-F]{8}) ([A-Za-z]) (.*)$', line)
        if not m or m.group(2) not in 'TtWw':
            continue
        a = int(m.group(1), 16)
        if a < min_addr:
            continue
        addrs.append(a)
        names.append(m.group(3))
    return addrs, names


def parse_sites(path):
    """the live blocks by allocation stack; with LEAK_HEAP=1 the H lines instead (blocks of the process heap that
    neither new nor malloc made: the DLLs' own, found by hooking ntdll when TS_LEAK_HEAPHOOK=1 is set for the run)"""
    sites = {}
    kind = 'H' if os.environ.get('LEAK_HEAP') else None
    for line in open(path):
        p = line.split()
        if kind:
            if len(p) < 4 or p[0] != 'H':
                continue
            p = p[1:]
        elif len(p) < 3 or p[0] in ('U', 'S', 'H'):
            continue
        live, nbytes, frames = int(p[0]), int(p[1]), tuple(p[2:])
        a = sites.get(frames, (0, 0))
        sites[frames] = (a[0] + live, a[1] + nbytes)
    return sites


_cache = {}
_exe = [None]


def symbolize(exe, a):
    """the innermost inline frames of the address as 'function file:line' (or None when the exe has no line tables)"""
    if a in _cache:
        return _cache[a]
    r = subprocess.run([SYMBOLIZER, '--obj=' + exe, '--inlines', '--demangle', '--basenames', '0x%x' % a], capture_output=True, text=True).stdout
    rows = [l for l in r.splitlines() if l.strip()]
    out = []
    for i in range(0, len(rows) - 1, 2):
        fn = re.sub(r'\(.*$', '', rows[i]) if '(' in rows[i] and not rows[i].startswith('operator') else rows[i]
        loc = rows[i + 1]
        if loc.startswith('??'):
            continue
        out.append('%s %s' % (fn[:70], re.sub(r':\d+$', '', loc)))
    _cache[a] = out or None
    return _cache[a]


def parse_untracked(path):
    """the heap blocks the counter does not know (SDL2.dll's, for one), by size: {size: count}"""
    out = {}
    for line in open(path):
        p = line.split()
        if len(p) == 3 and p[0] == 'U':
            out[int(p[1])] = int(p[2])
    return out


_dll = {}


def dll_symbols(exe, mod):
    """the exported symbols of a DLL beside the exe: (image base, addresses, names); SDL2.dll has the SDL_* functions"""
    if mod not in _dll:
        path = os.path.join(os.path.dirname(os.path.abspath(exe)), mod)
        base = 0
        if os.path.exists(path):
            o = subprocess.run([NM.replace('llvm-nm', 'llvm-objdump'), '-p', path], capture_output=True, text=True).stdout
            m = re.search(r'ImageBase\s+([0-9a-fA-F]+)', o)
            base = int(m.group(1), 16) if m else 0
            addrs, names = load_symbols(path, 0)
        else:
            addrs, names = [], []
        _dll[mod] = (base, addrs, names)
    return _dll[mod]


def name_of(frame, syms):
    mod, _, rva = frame.rpartition('+')
    if mod.lower().endswith('.dll') and mod.lower() != 'ntdll.dll' and _exe[0]:
        base, addrs, names = dll_symbols(_exe[0], mod)
        a = int(rva, 16) + base
        i = bisect.bisect_right(addrs, a) - 1
        if i >= 0 and a - addrs[i] < 0x4000:
            return '%s!%s+0x%x' % (mod, names[i], a - addrs[i])
    if mod.lower() == 'typershark.exe' and _exe[0] and os.environ.get('LEAK_LINES'):
        inl = symbolize(_exe[0], int(rva, 16) + IMAGE_BASE)
        if inl:
            return ' / '.join(inl[:3])
    if mod.lower() == 'typershark.exe':
        addrs, names = syms
        a = int(rva, 16) + IMAGE_BASE
        i = bisect.bisect_right(addrs, a) - 1
        if i >= 0:
            n = names[i]
            n = re.sub(r'\(.*$', '', n) if '(' in n and not n.startswith('operator') else n
            return '%s+0x%x' % (n[:90], a - addrs[i])
    return frame


def describe(frames, syms, depth=5):
    names = [name_of(f, syms) for f in frames]
    keep = [n for n in names if not n.startswith('Sexy::LeakCount') and 'operator new' not in n and 'anonymous namespace' not in n and '__wrap_' not in n and not n.startswith('operator delete')]
    keep = keep or names
    return ' < '.join(keep[:depth])


def main():
    exe, mode = sys.argv[1], sys.argv[2]
    args = sys.argv[3:]
    n = 25
    if '-n' in args:
        i = args.index('-n')
        n = int(args[i + 1])
        del args[i:i + 2]
    syms = load_symbols(exe)
    _exe[0] = exe
    if mode == 'show':
        sites = parse_sites(args[0])
        for frames, (live, nbytes) in sorted(sites.items(), key=lambda kv: -kv[1][0])[:n]:
            print('%6d blocks %9d bytes  %s' % (live, nbytes, describe(frames, syms)))
    elif mode == 'diff':
        a, b = parse_sites(args[0]), parse_sites(args[1])
        rows = []
        for frames in set(a) | set(b):
            la, ba = a.get(frames, (0, 0))
            lb, bb = b.get(frames, (0, 0))
            if lb != la:
                rows.append((lb - la, bb - ba, frames))
        grew = sorted([r for r in rows if r[0] > 0], key=lambda r: -r[0])
        shrank = sorted([r for r in rows if r[0] < 0], key=lambda r: r[0])
        print('GREW: %d sites, %d blocks, %d bytes' % (len(grew), sum(r[0] for r in grew), sum(r[1] for r in grew)))
        for d, db, frames in grew[:n]:
            print('  +%-5d blocks %+9d bytes  %s' % (d, db, describe(frames, syms)))
        print('SHRANK: %d sites, %d blocks' % (len(shrank), sum(r[0] for r in shrank)))
        for d, db, frames in shrank[:max(5, n // 3)]:
            print('  %-6d blocks %+9d bytes  %s' % (d, db, describe(frames, syms)))
    elif mode == 'udiff':
        a, b = parse_untracked(args[0]), parse_untracked(args[1])
        rows = sorted(((b.get(k, 0) - a.get(k, 0), k) for k in set(a) | set(b) if b.get(k, 0) != a.get(k, 0)), reverse=True)
        print('untracked blocks: %d -> %d' % (sum(a.values()), sum(b.values())))
        for d, k in rows[:n]:
            print('  size <=%6d: %+d' % (k, d))
    elif mode == 'total':
        for f in args:
            s = parse_sites(f)
            print('%s: %d blocks %d bytes in %d sites' % (os.path.basename(f), sum(v[0] for v in s.values()), sum(v[1] for v in s.values()), len(s)))


if __name__ == '__main__':
    main()
