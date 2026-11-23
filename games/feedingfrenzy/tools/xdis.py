"""Disassembles a range of the exe like dis.sh and annotates what the decompiler loses: the float and double constants a
floating-point instruction reads, the strings a pushed address points at, and immediates that are floats.

Usage: python -I games/feedingfrenzy/tools/xdis.py <start> <stop>   (hex addresses; start must be a function start)
"""
import importlib.util
import os
import re
import struct
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
SPEC = importlib.util.spec_from_file_location("fn", os.path.join(HERE, "fn.py"))
FN = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(FN)


def read_float(va):
    return struct.unpack('<f', FN.read_exe(va, 4))[0]


def read_double(va):
    return struct.unpack('<d', FN.read_exe(va, 8))[0]


def read_string(va):
    data = FN.read_exe(va, 48)
    if data is None:
        return None
    data = data.split(bytes([0]))[0]
    if len(data) >= 2 and all(32 <= c < 127 or c in (9, 10, 13) for c in data):
        return data.decode('ascii')
    return None


def fmt(value):
    if value != value or abs(value) > 1e9:
        return None
    return repr(round(value, 6))


def annotate(line):
    note = ''
    m = re.search(r'(dword|qword) ptr \[0x(4[0-9a-f]{5})\]', line)
    if m and re.search(r'\bf(ld|add|sub|subr|mul|div|divr|comp|icom|ucom)', line):
        va = int(m.group(2), 16)
        value = read_float(va) if m.group(1) == 'dword' else read_double(va)
        note = fmt(value) or ''
    m = re.search(r'push\s+0x(4[0-9a-f]{5})\s*$', line)
    if m and not note:
        text = read_string(int(m.group(1), 16))
        if text is not None:
            note = '"%s"' % text
    m = re.search(r'(push|mov.*,)\s+0x([0-9a-f]{8})\s*$', line)
    if m and not note:
        bits = int(m.group(2), 16)
        if 0x30000000 <= bits < 0x47000000 or 0xb0000000 <= bits < 0xc7000000:
            note = fmt(struct.unpack('<f', struct.pack('<I', bits))[0]) or ''
    return line + ('    ; ' + note if note else '')


def main():
    start, stop = sys.argv[1], sys.argv[2]
    root = subprocess.check_output(['git', 'rev-parse', '--show-toplevel'], text=True).strip()
    objdump = sorted(__import__('glob').glob('E:/retools/llvm-mingw-*/bin/llvm-objdump.exe'))[0]
    exe = os.path.join(root, 're', 'feedingfrenzy', 'bin', 'FeedingFrenzy.exe')
    out = subprocess.check_output([objdump, '-d', '-M', 'intel', '--no-show-raw-insn',
                                   '--start-address=0x' + start, '--stop-address=0x' + stop, exe], text=True)
    for raw in out.splitlines():
        line = re.sub(r'\s+', ' ', raw.strip())
        line = re.sub(r' <[^>]*>', '', line)
        if re.match(r'^[0-9a-f]+:', line):
            if line.endswith('int3'):
                continue
            print(annotate(line))


if __name__ == '__main__':
    main()
