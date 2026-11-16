# record.py <playbymove driver.txt> [last update] [--live <-recordlive file>]: prints the bot's input of a playbymove
# run as a -realinput file ("<update> <command> ...") ending with a "quit" line.
#
# --live: the framework's own record of the run (-recordlive), each input at the update that ran it; its input lines
# are the file's, its comments (the bot's state and move queries, its own commands, a failed line) are left out, and
# the quit comes at its last update (or the one given). This is the file a replay follows the bot's game by.
#
# Without it (a run before -recordlive): playbymove runs the port held (-hold), where the game moves on by its "wait N"
# lines and by a frame for each round of queries, so a line's update is taken as the frame the last state reply gave
# plus the waits sent after it. The game's own test commands (state, move, goto, ...) are left out (the bot's "move"
# query is the word alone, a move of the mouse has a point). A held input takes effect a little after that update,
# so a long replay made this way leaves the bot's game (IDENTICAL_PLAY.md, "The held run and its replay").
import os
import re
import sys

INPUT = ('move', 'click', 'down', 'up', 'rclick', 'key', 'type', 'keydown', 'keyup')
FRAME = re.compile(r'"frame":(\d+)')

args = sys.argv[1:]
live = None
if '--live' in args:
    at = args.index('--live')
    live = args[at + 1]
    del args[at:at + 2]
given_last = int(args[1]) if len(args) > 1 else None

if live is not None and os.path.exists(live):
    last = 0
    for raw in open(live, encoding='utf-8', errors='replace'):
        line = raw.rstrip('\r\n')
        words = line.lstrip('# ').split(' ', 1)
        if words and words[0].isdigit():
            last = max(last, int(words[0]))
        if not line or line.startswith('#'):
            continue
        if given_last is None or int(words[0]) <= given_last:
            print(line)
    print('%d quit' % (given_last if given_last is not None else last))
    sys.exit(0)

update = 0
lines = []
for raw in open(args[0], encoding='utf-8', errors='replace'):
    if raw.startswith('< STATE '):
        frame = FRAME.search(raw)
        if frame:
            update = int(frame.group(1))
        continue
    if not raw.startswith('> '):
        continue
    words = raw[2:].rstrip('\n').split(' ')
    if words[0] == 'wait':
        update += int(words[1])
    elif words[0] in INPUT and len(words) > 1:
        lines.append('%d %s' % (update, ' '.join(words)))
last = given_last if given_last is not None else update
for line in lines:
    if int(line.split(' ', 1)[0]) <= last:
        print(line)
print('%d quit' % last)
