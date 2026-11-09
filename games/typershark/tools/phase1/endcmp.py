#!/usr/bin/env python3
"""endcmp.py <tag> [--orig-log NAME] : compare the state the original and the port END a run with (P1b item 5): the registry values
(the original's `reg export` kept by runorig.sh as <log>.reg, the port's registry.ini in its -userdir) and the files of the users folder
(byte for byte). Reads build/ts-p1/<tag>.orig.json's sibling logs: the original's last pass is <tag>.p2.orig.log (or <tag>.t.orig.log
for a tutor run); the port's folder is build/ts-p1/u_<tag>.   Prints one line per difference and a verdict line."""
import os, re, sys
HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.normpath(os.path.join(HERE, '..', '..', '..', '..'))
OUT = os.path.join(ROOT, 'build', 'ts-p1')


def read_reg(path):
    """original: a `reg export` file (UTF-16) -> {(key, name): (kind, value)}"""
    raw = open(path, 'rb').read()
    txt = raw.decode('utf-16') if raw[:2] in (b'\xff\xfe', b'\xfe\xff') else raw.decode('latin1')
    out = {}
    key = None
    for line in txt.replace('\r', '').split('\n'):
        m = re.match(r'\[HKEY_CURRENT_USER[\\](.*)\]$', line)
        if m:
            key = m.group(1).lower()
            continue
        m = re.match(r'"(.*)"=(.*)$', line)
        if m and key is not None:
            name, val = m.group(1), m.group(2)
            if val.startswith('dword:'):
                out[(key, name)] = ('dword', int(val[6:], 16))
            elif val.startswith('"'):
                out[(key, name)] = ('sz', val[1:-1].replace(chr(92) * 2, chr(92)).replace(chr(92) + '"', '"'))
            else:
                out[(key, name)] = ('other', val)
    return out


def read_ini(path):
    out = {}
    key = None
    for line in open(path, errors='replace'):
        line = line.rstrip('\r\n')
        m = re.match(r'\[(.*)\]$', line)
        if m:
            key = m.group(1).lower()
            continue
        if '=' in line and key is not None and not line.startswith(';'):
            name, val = line.split('=', 1)
            if val.startswith('dword:'):
                v = int(val[6:])
                out[(key, name)] = ('dword', v & 0xffffffff)
            elif val.startswith('sz:'):
                out[(key, name)] = ('sz', val[3:])
            else:
                out[(key, name)] = ('other', val)
    return out


def main():
    tag = sys.argv[1]
    ologs = [a for a in (tag + '.p2.orig.log', tag + '.t.orig.log', tag + '.orig.log') if os.path.isdir(os.path.join(OUT, a + '.users'))]
    if not ologs:
        print(f'{tag}: no end state of the original ({tag}.p2.orig.log.users)')
        return
    olog = os.path.join(OUT, ologs[0])
    # the original keeps no registry key when the run could not write one (an unelevated run: see PHASE1.md, item 5)
    oreg = read_reg(olog + '.reg') if os.path.exists(olog + '.reg') else {}
    if not oreg:
        print(f'{tag}: the original left no registry values (registry not compared)')
    pdir = os.path.join(OUT, 'u_' + tag)
    preg = read_ini(os.path.join(pdir, 'registry.ini'))
    ndiff = 0
    ignore = {'timesexecuted', 'timesplayed', 'test'}      # run counters of the executable, not of a game
    for k in sorted((set(oreg) | set(preg)) if oreg else set()):
        if k[1].lower() in ignore:
            continue
        a, b = oreg.get(k), preg.get(k)
        if a is None or b is None:
            ndiff += 1
            print(f'registry {k[0]} / {k[1]}: orig {a} port {b}')
        elif a != b:
            ndiff += 1
            print(f'registry {k[0]} / {k[1]}: orig {a} port {b}')
    ofiles = os.path.join(olog + '.users')
    pfiles = os.path.join(pdir, 'users')
    names = sorted(set(os.listdir(ofiles) if os.path.isdir(ofiles) else []) | set(os.listdir(pfiles) if os.path.isdir(pfiles) else []))
    for n in names:
        fo, fp = os.path.join(ofiles, n), os.path.join(pfiles, n)
        if not os.path.exists(fo) or not os.path.exists(fp):
            ndiff += 1
            print(f'file {n}: orig {"present" if os.path.exists(fo) else "absent"} port {"present" if os.path.exists(fp) else "absent"}')
            continue
        a, b = open(fo, 'rb').read(), open(fp, 'rb').read()
        if a != b:
            ndiff += 1
            first = next((i for i in range(min(len(a), len(b))) if a[i] != b[i]), min(len(a), len(b)))
            nd = sum(1 for i in range(min(len(a), len(b))) if a[i] != b[i])
            print(f'file {n}: sizes {len(a)} / {len(b)}, first byte that differs at {first}, {nd} bytes differ')
    print(f'{tag}: end state: {len(oreg)} registry values (original), {len(preg)} (port), {len(names)} files, {ndiff} differences')


if __name__ == '__main__':
    main()
