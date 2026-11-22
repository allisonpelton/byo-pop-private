"""A copy of make_profile.py's player "Ali" with other progress, as a folder of the original's own save files, so that the port and the
original start a screen coverage run (SCREENS.md) from the same state with no test command (the original has none):

    python games/bejeweled2/tools/origcmp/make_gapprofile.py --base build/logs/bej2/profile --out build/logs/bej2/prof_x \
        [--puzzle N] [--solved K] [--hints MASK] [--galaxy N] [--mode-level I=V ...] [--fresh]

--puzzle N     the puzzle the player is on in "Galaxy One" (the planet is N / 5); --solved K marks the first K puzzles solved
               (default: N)
--hints MASK   mHints, bit n set = hint n turned off (the one-time boxes: 0 shows them all)
--galaxy N     mGalaxyOneSolved (80 unlocks Cognito)
--mode-level   mModeLevel[I] = V (17, 8 and 280 unlock Twilight, Hyper and Finity)
--fresh        every counter zero, no hint turned off, no secret mode (the first game of a new player)
The file's layout is Profile::WriteProfile's (Profile.cpp): magic, version, the pack, the packs' progress, eight counters, the
backdrop flag, the hint mask, the galaxy count, four mode levels."""
import argparse
import os
import shutil
import struct


def read_profile(path):
    d = open(path, 'rb').read()
    p = [0]

    def take(fmt):
        n = struct.calcsize(fmt)
        v = struct.unpack_from(fmt, d, p[0])
        p[0] += n
        return v[0]

    def string():
        n = take('<h')
        s = d[p[0]:p[0] + n].decode('latin-1')
        p[0] += n
        return s
    prof = {'magic': take('<I'), 'version': take('<I'), 'pack': string(), 'progress': {}}
    for _ in range(take('<h')):
        name = string()
        n = take('<h')
        solved = list(d[p[0]:p[0] + n])
        p[0] += n
        prof['progress'][name] = {'solved': solved, 'num': take('<h')}
    prof['stats'] = [take('<i') for _ in range(8)]
    prof['random'] = take('<B')
    prof['hints'] = take('<I')
    prof['galaxy'] = take('<i')
    prof['levels'] = [take('<i') for _ in range(4)]
    assert p[0] == len(d), 'trailing bytes'
    return prof


def write_profile(prof, path):
    out = struct.pack('<II', prof['magic'], prof['version'])

    def string(s):
        return struct.pack('<h', len(s)) + s.encode('latin-1')
    out += string(prof['pack']) + struct.pack('<h', len(prof['progress']))
    for name in sorted(prof['progress']):
        pr = prof['progress'][name]
        out += string(name) + struct.pack('<h', len(pr['solved'])) + bytes(pr['solved']) + struct.pack('<h', pr['num'])
    for v in prof['stats']:
        out += struct.pack('<i', v)
    out += struct.pack('<B', prof['random']) + struct.pack('<I', prof['hints']) + struct.pack('<i', prof['galaxy'])
    for v in prof['levels']:
        out += struct.pack('<i', v)
    os.makedirs(os.path.dirname(path), exist_ok=True)
    open(path, 'wb').write(out)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--base', required=True)
    ap.add_argument('--out', required=True)
    ap.add_argument('--puzzle', type=int)
    ap.add_argument('--solved', type=int)
    ap.add_argument('--hints', type=lambda s: int(s, 0))
    ap.add_argument('--galaxy', type=int)
    ap.add_argument('--mode-level', action='append', default=[])
    ap.add_argument('--fresh', action='store_true')
    a = ap.parse_args()
    shutil.rmtree(a.out, ignore_errors=True)
    shutil.copytree(a.base, a.out)
    path = os.path.join(a.out, 'users', 'Ali', 'profile.dat')
    prof = read_profile(path)
    if a.fresh:
        prof.update(stats=[0] * 8, hints=0, galaxy=0, levels=[0] * 4, progress={})
    if a.puzzle is not None:
        solved = a.puzzle if a.solved is None else a.solved
        prof['progress']['Galaxy One'] = {'solved': [1] * solved + [0] * (400 - solved), 'num': a.puzzle}
    if a.hints is not None:
        prof['hints'] = a.hints
    if a.galaxy is not None:
        prof['galaxy'] = a.galaxy
    for item in a.mode_level:
        i, v = item.split('=')
        prof['levels'][int(i)] = int(v)
    write_profile(prof, path)
    print(path, prof)


if __name__ == '__main__':
    main()
