"""Writes games/pizzafrenzy/tools/web/replay_case.js: replay.js with a case in front (its input, its shot list, its
seed and, for a case that starts from a saved player, the port's player folder).
    python -I games/pizzafrenzy/tools/web/replay_gen.py <case dir> <shot list> <seed> [--user <port user folder>]
The case dir is tools/identical's build/identical/<case> (input.txt); --user takes the files under the folder
(build/identical/<case>/port/user) as the page's player files.
"""
import base64
import json
import os
import sys


def main():
    args = sys.argv[1:]
    user = None
    if '--user' in args:
        i = args.index('--user')
        user = args[i + 1]
        del args[i:i + 2]
    case, shot_list, seed = args[0], args[1], int(args[2])
    lines = [line.strip() for line in open(os.path.join(case, 'input.txt')) if line.strip()]
    shots = []
    for line in open(shot_list):
        words = line.split()
        if len(words) >= 2 and not line.startswith('#'):
            shots.append([int(words[0]), words[1]])
    files = {}
    if user:
        for root, _, names in os.walk(user):
            for name in names:
                path = os.path.join(root, name)
                rel = os.path.relpath(path, user).replace(os.sep, '/')
                files[rel] = base64.b64encode(open(path, 'rb').read()).decode()
    here = os.path.dirname(os.path.abspath(__file__))
    head = 'var CASE = %s;\n' % json.dumps({'seed': seed, 'lines': lines, 'shots': shots, 'files': files})
    with open(os.path.join(here, 'replay_case.js'), 'w', newline='\n') as out:
        out.write(head + open(os.path.join(here, 'replay.js')).read())
    print('%d input lines, %d shots, %d player files' % (len(lines), len(shots), len(files)))


if __name__ == '__main__':
    main()
