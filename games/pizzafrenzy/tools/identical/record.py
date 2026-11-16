# record.py <playbymove driver.txt> [last update]: prints the bot's input of a playbymove run as a -realinput file
# ("<update> <command> ..."). playbymove runs the port held (-hold): the game moves on only by its "wait N" lines, so a
# line's update is the sum of the waits sent before it. The game's own test commands (state, move, goto, ...) are left
# out (the bot's "move" query is the word alone, a move of the mouse has a point); a "quit" line ends the file at the
# last update (or the one given).
import sys

INPUT = ('move', 'click', 'down', 'up', 'rclick', 'key', 'type', 'keydown', 'keyup')

update = 0
lines = []
for raw in open(sys.argv[1], encoding='utf-8', errors='replace'):
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
