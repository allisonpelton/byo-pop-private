#!/usr/bin/env python3
"""Which levels hold which kinds of object: for each level of each list the config names (Journey's `level_file_relaxed`,
Flood's and Survival's `level_file_game`, Trick Shot's `level_file_trick`), its number as the game numbers it and the
types of the objects in its file (and `action:<name>` for each action its events run). A coverage scenario picks the levels it starts at from this.

    python games/venice/tools/coverage/levelfeat.py [type ...]     # the levels holding every type or action given, or the counts of all
"""
import collections
import os
import re
import sys

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..', '..', '..'))
ASSETS = os.path.join(ROOT, 'assets', 'venice')


def levels():
    config = open(os.path.join(ASSETS, 'config.xml'), encoding='latin1').read()
    lists = collections.OrderedDict((kind, []) for kind in ('relaxed', 'game', 'trick'))
    for found in re.finditer(r'<level_file_(relaxed|game|trick)\s+name="([^"]+)"', config):
        lists[found.group(1)].append(found.group(2).replace(chr(92), '/'))
    result = []
    for kind, files in lists.items():
        for number, name in enumerate(files, 1):
            text = open(os.path.join(ASSETS, name), encoding='latin1').read()
            features = set(re.findall(r'type="([^"]+)"', text))
            features.update('action:' + action for action in re.findall(r'action="([^"]+)"', text))
            result.append((kind, number, name, features, text))
    return result


def main():
    wanted = set(sys.argv[1:])
    rows = levels()
    if not wanted:
        counts = collections.Counter()
        for _, _, _, types, _ in rows:
            counts.update(types)
        for name, count in counts.most_common():
            print('%5d %s' % (count, name))
        return 0
    for kind, number, name, types, _ in rows:
        if wanted <= types:
            print('%-8s %3d %s' % (kind, number, name))
    return 0


if __name__ == '__main__':
    sys.exit(main())
