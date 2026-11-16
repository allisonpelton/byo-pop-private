# record.py <playbymove driver.txt> [last update]: prints the bot's input of a playbymove run as a -realinput file
# ("<update> <command> ..."). playbymove runs the port held (-hold): the game moves on by its "wait N" lines and by a
# frame for each round of queries, so a line's update is the frame the last state reply gave plus the waits sent after
# it. The game's own test commands (state, move, goto, ...) are left out (the bot's "move" query is the word alone, a
# move of the mouse has a point); a "quit" line ends the file at the last update (or the one given).
import re
import sys

INPUT = ('move', 'click', 'down', 'up', 'rclick', 'key', 'type', 'keydown', 'keyup')
FRAME = re.compile(r'"frame":(\d+)')

update = 0
lines = []
for raw in open(sys.argv[1], encoding='utf-8', errors='replace'):
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
last = int(sys.argv[2]) if len(sys.argv) > 2 else update
for line in lines:
    if int(line.split(' ', 1)[0]) <= last:
        print(line)
print('%d quit' % last)
