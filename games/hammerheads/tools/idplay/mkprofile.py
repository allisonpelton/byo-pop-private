#!/usr/bin/env python3
"""A player's file for the looks scenarios that need a player with a past (SCREENS.md): Tough Cookie and Marathon open, restart points in
Classic and Tough Cookie (so that a mode's button offers the restart-point screen), some trophies won. The file is the game's own
(`users/1.xml`, `users/directory.txt`: the format both the original and the port read, `ProfileFile.cpp`), made here so that both start
from the same player. Nothing of it is a game file.

    python mkprofile.py <name> [out folder]      # name: rich (default folder build/profiles/<name>)
"""
import hashlib
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.normpath(os.path.join(HERE, '..', '..', '..', '..'))

# the numbers of a restart point as RestartPoint::ToString prints them: coins, health, health capacity, hammer level, hammer on fire,
# the mode, the level (the game takes up at the next one), the bonus rounds, then twenty more counts
def point(mode, level, coins, health, capacity, hammer, fire, bonus):
    counts = [0] * 20
    values = [coins, health, capacity, hammer, fire, mode, level, bonus] + counts
    return ';'.join('%f' % v for v in values)


PROFILES = {
    'rich': {
        'trophies': '110101001011010100110',
        'classic': [point(0, 4, 150, 5, 5, 1, 0, 0), point(0, 9, 320, 4, 6, 2, 0, 1), point(0, 14, 540, 3.5, 6, 2, 1, 2), point(0, 19, 880, 5, 7, 3, 0, 3)],
        'tough': [point(1, 3, 90, 4, 5, 1, 0, 0), point(1, 8, 260, 3, 5, 2, 0, 1)],
    },
}


def hashed(states):
    return hashlib.md5(''.join(states).encode()).hexdigest().upper()


def make(name, out):
    spec = PROFILES[name]
    users = os.path.join(out, 'users')
    os.makedirs(users, exist_ok=True)
    lines = ['<?xml version="1.0" encoding="UTF-8" ?>', '<user version="7" name="Test" uuid="1">',
             '    <iscurrent>true</iscurrent>', '    <showhelpscreen>true</showhelpscreen>',
             '    <hascompletedclassic>true</hascompletedclassic>', '    <cookieexplanationshown>false</cookieexplanationshown>',
             '    <trophyflags>%s</trophyflags>' % spec['trophies']]
    for tag, key in (('resurrectsnormal', 'classic'), ('resurrectstough', 'tough')):
        lines.append('    <%s hash="%s">' % (tag, hashed(spec[key])))
        lines += ['        <resurrect state="%s" />' % s for s in spec[key]]
        lines.append('    </%s>' % tag)
    lines.append('</user>')
    open(os.path.join(users, '1.xml'), 'w', newline='').write('\r\n'.join(lines) + '\r\n')
    open(os.path.join(users, 'directory.txt'), 'w', newline='').write('1\r\n')
    return users


if __name__ == '__main__':
    name = sys.argv[1] if len(sys.argv) > 1 else 'rich'
    out = sys.argv[2] if len(sys.argv) > 2 else os.path.join(ROOT, 'build', 'profiles', name)
    print(make(name, out))
