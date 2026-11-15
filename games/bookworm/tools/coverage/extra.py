# extra.py <bot log> <extras file> <shift> <cap> <shots dir>: prints the port -script of a coverage run that adds lines to the
# bot's play (run.sh extra): the bot's input shifted <shift> updates later (the original loads for that long), without its
# last "quit"; the extras' "<update> <command>" lines as they are (in that shifted timeline) and its "+<n> <command>" lines
# n updates after the cap's update; "screenshot NAME" lines write <shots dir>/NAME.png; then a quit. A line's command may
# start with @port or @orig (the original's dialogs lie 14 px lower or higher than the port's when it has no 3D row): the
# line is then only for that side, <side> (the sixth argument).
import sys

bot, extras, shift, cap, shots, side = sys.argv[1:7]
shift, cap = int(shift), int(cap)
lines = []
for l in open(bot):
    l = l.rstrip()
    if l and not l.startswith('#') and l.split(None, 2)[1] != 'quit':
        k, cmd = l.split(None, 1)
        lines.append((int(k) + shift, cmd))
last = cap + shift
for l in open(extras):
    l = l.split('#')[0].strip()
    if not l:
        continue
    k, cmd = l.split(None, 1)
    if cmd.startswith('@'):
        tag, cmd = cmd.split(None, 1)
        if tag[1:] != side:
            continue
    u = last + int(k[1:]) if k.startswith('+') else int(k)
    if cmd.startswith('screenshot '):
        cmd = 'screenshot %s/%s.png' % (shots, cmd.split()[1])
    lines.append((u, cmd))
lines.sort(key=lambda t: t[0])
end = lines[-1][0] + 40
for u, cmd in lines:
    print('%d %s' % (u, cmd))
print('%d quit' % end)
