# fromlog.py <bot log> [extra tour lines file]: a tour (make.py's format) from the -autoplay bot's -autoplaylog,
# written with -autoplayshots=shots/: its screenshot lines become named shots, every dialog the bot answers gets a
# shot at the update of its answer (taken before the click), a run of "char" lines at one update becomes one "text",
# and the rest is kept as it is.
import os, re, sys
out = []
pending = None      # a dialog to shoot at the next input line
text_at, text = None, ''


def flush():
    global text_at, text
    if text_at is not None:
        out.append('%d text %s' % (text_at, text))
    text_at, text = None, ''


n = 0
for line in open(sys.argv[1]):
    line = line.rstrip('\n')
    m = re.match(r'# update \d+: click the OK of dialog (\d+) \((.*)\)', line)
    if m:
        n += 1
        pending = 'dialog%d-%s' % (n, re.sub(r'[^a-z]+', '', m.group(2).lower()))
        continue
    line = line.split('#')[0].strip()
    if not line:
        continue
    k, cmd, *rest = line.split(None, 2)
    arg = rest[0] if rest else ''
    if cmd == 'char':
        if text_at is not None and text_at != int(k):
            flush()
        text_at = int(k)
        text += ' ' if arg == 'SPACE' else arg
        continue
    flush()
    if pending is not None:
        out.append('%s screenshot %s' % (k, pending))
        pending = None
    if cmd == 'screenshot':
        out.append('%s screenshot %s' % (k, os.path.splitext(os.path.basename(arg))[0]))
        continue
    out.append(' '.join([k, cmd] + ([arg] if arg else [])))
flush()
if len(sys.argv) > 2:
    out += [l.strip() for l in open(sys.argv[2]) if l.strip() and not l.startswith('#')]
print('# made by fromlog.py from %s' % os.path.basename(sys.argv[1]))
print('\n'.join(out))
