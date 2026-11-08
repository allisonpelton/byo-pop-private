#!/usr/bin/env python3
"""Shows the arguments pushed before calls whose decompilation lost them (a one-byte bool or an int that the export's
signature drops): for each CALL in a function whose target name contains one of the words, the PUSH/MOV/LEA lines before it.

    python games/bejeweled2/tools/callargs.py 5d9b7b SetDrawMode SetColorizeImages
"""
import os
import re
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))


def main():
    addr, words = sys.argv[1], sys.argv[2:]
    out = subprocess.run([sys.executable, os.path.join(HERE, 'dec.py'), '-a', addr], capture_output=True, text=True,
                         encoding='utf-8', errors='replace').stdout.splitlines()
    for i, line in enumerate(out):
        if 'CALL' in line and any(w in line for w in words):
            j = i - 1
            pre = []
            while j > 0 and len(pre) < 4 and ('PUSH' in out[j] or 'MOV' in out[j] or 'LEA' in out[j] or 'XOR' in out[j]):
                pre.append(out[j].strip())
                j -= 1
            print(line.strip().split(';')[0].strip(), line.split(';')[-1].strip(), '<-', ' | '.join(reversed([p.split('  ', 1)[-1] for p in pre])))


main()
