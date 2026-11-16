"""Prints the bytes before each call to a function inside another function, to read which `this` a call uses.

    python -I callctx.py <function address> <callee address> [bytes before, default 16]
"""
import json
import os
import struct
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import pe

ROOT = pe.ROOT


def function_range(address):
    for line in open(os.path.join(ROOT, "re", "peggle", "deluxe", "gh", "functions.jsonl")):
        f = json.loads(line)
        if int(f.get("address") or f.get("addr"), 16) == address:
            return address, f.get("size") or f.get("length")
    raise SystemExit("function not found")


def main():
    func = int(sys.argv[1], 16)
    callee = int(sys.argv[2], 16)
    before = int(sys.argv[3]) if len(sys.argv) > 3 else 16
    start, size = function_range(func)
    image = pe.Image()
    code = image.bytes(start, size)
    for i in range(len(code) - 4):
        if code[i] == 0xE8:
            target = start + i + 5 + struct.unpack_from("<i", code, i + 1)[0]
            if target == callee:
                lo = max(0, i - before)
                print("%08x: %s | call" % (start + i, code[lo:i].hex(" ")))


main()
