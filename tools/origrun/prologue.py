"""Whole-instruction prologues for origrun's --count (docs/ORIGRUN.md, "Counting cycles inside the original's functions").

    python -I tools/origrun/prologue.py <exe> <label>=<address> ...      suggest each function's line
    python -I tools/origrun/prologue.py <exe> --check <function list>    check a list against the exe

The stand-in's counters replace a function's first instructions with a jump (5 bytes) and run a copy of them from a stub, so the
copy must be whole instructions, 5 bytes or more, and none of them may be a relative branch or call (the copy would branch from
the wrong place). A list line is `label address len=N`, N the length of that run of instructions; origrun takes the bytes from
the exe itself, so a list holds addresses and lengths and none of the game. The address is the exe's virtual address.

The instructions are read with llvm-objdump from the llvm-mingw under E:/retools (the toolchain tools/origrun/build.sh uses).
Exit status 1 when an entry cannot be used.
"""
import glob
import re
import struct
import subprocess
import sys

MIN_LEN = 5
MAX_LEN = 32
RELATIVE = re.compile(r'^(call|jmp|j[a-z]+|loop[a-z]*|jecxz|jcxz)\s+(0x[0-9a-f]+|-?\d+)(\s|$)', re.I)


def objdump():
    found = sorted(glob.glob('E:/retools/llvm-mingw-*-ucrt-x86_64/bin/llvm-objdump.exe'))
    if not found:
        sys.exit('llvm-objdump not found under E:/retools/llvm-mingw-*')
    return found[-1]


def instructions(exe, address):
    """[(address, length, text)] of the instructions from address, enough to cover MAX_LEN bytes."""
    out = subprocess.run([objdump(), '-d', '--no-show-raw-insn', '-M', 'intel', '--start-address=0x%x' % address,
                          '--stop-address=0x%x' % (address + MAX_LEN + 16), exe], capture_output=True, text=True).stdout
    rows = []
    for line in out.splitlines():
        m = re.match(r'\s*([0-9a-f]+):\s+(.*\S)\s*$', line)
        if m:
            rows.append((int(m.group(1), 16), m.group(2)))
    return [(a, rows[i + 1][0] - a, t) for i, (a, t) in enumerate(rows[:-1])]


def choose(exe, address, length=None):
    """(length, texts, problem): the run of whole instructions from address, the shortest of at least MIN_LEN bytes unless a length is given."""
    run, total = [], 0
    for a, n, text in instructions(exe, address):
        if a != address + total:
            return None, run, 'the disassembly lost its place at 0x%x' % a
        if RELATIVE.match(text) or text.startswith(('ret', 'int3', 'jmp ', 'iret')):
            return None, run, 'the instruction at 0x%x is %r: a relative branch, call or return cannot be copied' % (a, text)
        run.append(text)
        total += n
        if length is None and total >= MIN_LEN:
            return total, run, None
        if length is not None and total == length:
            return (total, run, None) if total >= MIN_LEN else (None, run, 'len=%d is under %d bytes' % (length, MIN_LEN))
        if length is not None and total > length:
            return None, run, 'len=%d falls inside an instruction (%r ends at +%d)' % (length, text, total)
        if total > MAX_LEN:
            break
    return None, run, 'no run of whole instructions of %d to %d bytes' % (MIN_LEN, MAX_LEN)


def main():
    if len(sys.argv) < 3:
        sys.exit(__doc__)
    exe = sys.argv[1]
    entries = []
    if sys.argv[2] == '--check':
        for line in open(sys.argv[3]):
            line = line.split('#')[0].strip()
            if not line:
                continue
            f = line.split()
            length = next((int(x[4:]) for x in f[2:] if x.startswith('len=')), None)
            entries.append((f[0], int(f[1], 16), length))
    else:
        for arg in sys.argv[2:]:
            label, address = arg.split('=')
            entries.append((label, int(address, 16), None))
    bad = 0
    for label, address, length in entries:
        n, run, problem = choose(exe, address, length)
        if problem:
            bad += 1
            print('%-10s 0x%08x  NOT USABLE: %s' % (label, address, problem))
        else:
            print('%-10s 0x%08x  len=%d    # %s' % (label, address, n, '; '.join(run)))
    sys.exit(1 if bad else 0)


main()
