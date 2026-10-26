#!/usr/bin/env python3
"""Compact decread.py's output further for reading: drops MSVC 6 std::string / strlen / exception-frame idioms
(keeps the string literals, shown as S"...", and the calls and numbers), then removes the empty blocks left behind.

    python games/mummymaze/tools/decread.py 4092a0 | python games/mummymaze/tools/skim.py
"""
import re, sys

DROP = [
    r'^\s*\w+ = 0xffffffff;$',
    r'^\s*\w+ = ~\w+ - 1;$',
    r'std::length_error::ctor_or_dtor\(\);',
    r'FUN_00401b40\(',
    r'^\s*\w+ = FUN_004030a0\(\);$',
    r'^\s*\w+ = string_grow\(',
    r'^\s*string_eos\(',
    r"^\s*string_tidy\(.*,'\\[0x01]+'\);$",
    r'^\s*(Exception|STRFREE|STRDEL|STRINIT)',
    r'\bcVar\d+ = \*\(?[pi]?\w+ ?\+? ?-?\d*\)?;$',
    r'^\s*((?:pcVar|pvVar|puVar|pIVar|pBVar)\d*) =  \+ 1;$',
    r'^\s*(pcVar|puVar|pvVar|pIVar)\d+ = (pcVar|puVar|pvVar)\w+;$',
    r"^\s*\w+\[\w+\] = '\\0';$",
    r'^\s*(pcVar|pvVar|puVar|pIVar|pBVar)\d* = (pcVar|pvVar|puVar|pIVar|pBVar)\w*;$',
]
DROP = [re.compile(d) for d in DROP]
STRLOOP_START = re.compile(r'^\s*do \{$')
STRLOOP_BRK = re.compile(r'^\s*if \(\w+ == 0\) break;$')
STR = re.compile(r'"((?:[^"\\]|\\.)*)"/\*@([0-9a-f]{8})\*/')


def skim(text):
    lines = text.split('\n')
    out = []
    i = 0
    while i < len(lines):
        ln = lines[i]
        if STRLOOP_START.match(ln) and i + 1 < len(lines) and STRLOOP_BRK.match(lines[i + 1]):
            j = i
            while j < len(lines) and not re.match(r"^\s*\} while \(", lines[j]):
                j += 1
            i = j + 1
            continue
        if re.search(r'strlen\(', ln) and STR.search(ln) and ('string_assign' not in ln):
            m = STR.search(ln)
            out.append('  S"%s"@%s' % (m.group(1), m.group(2)))
            i += 1
            continue
        if any(d.search(ln) for d in DROP) and 'return' not in ln:
            i += 1
            continue
        ln = STR.sub(lambda m: 'S"%s"@%s' % (m.group(1), m.group(2)), ln)
        out.append(ln)
        i += 1
    t = '\n'.join(out)
    # empty blocks left behind
    for _ in range(6):
        t2 = re.sub(r'\n[ ]*(?:if|while) \([^\n]*\) \{\n[ ]*\}(?=\n)', '', t)
        t2 = re.sub(r'\n[ ]*\} else \{\n[ ]*\}', '\n  }', t2)
        t2 = re.sub(r'\n[ ]*else \{\n[ ]*\}', '', t2)
        if t2 == t:
            break
        t = t2
    return t


if __name__ == '__main__':
    sys.stdout.write(skim(sys.stdin.read()))
