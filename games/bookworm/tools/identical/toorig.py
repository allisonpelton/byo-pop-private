# toorig.py <port script> [delta]: prints an origrun script for the original from a port -script file (the bot's
# -autoplaylog). A port line "K command args" is read before update K's widgets are updated; the original reads an input
# posted "at N" before update N+1, so a line becomes "at K+delta", delta -1 by default. Keys go by their virtual-key
# code; a run of "char" lines at one update becomes one "text" (SPACE a space); screenshot lines are dropped, and a quit
# line ends the script with origrun's quit (the window asked to close at once, so the game saves as a closed window
# does).
import sys

KEYS = {'RETURN': 13, 'ESCAPE': 27, 'TAB': 9, 'BACK': 8, 'SPACE': 32, 'DELETE': 46, 'LEFT': 37, 'UP': 38, 'RIGHT': 39,
        'DOWN': 40, 'CONTROL': 17, 'SHIFT': 16}

src = sys.argv[1]
delta = int(sys.argv[2]) if len(sys.argv) > 2 else -1
out = []
text_at, text = None, ''


def flush():
    global text_at, text
    if text_at is not None:
        out.append('at %d text %s' % (text_at, text))
    text_at, text = None, ''


for line in open(src):
    line = line.split('#')[0].strip()
    if not line:
        continue
    k, cmd, *rest = line.split(None, 2)
    u = int(k) + delta
    arg = rest[0] if rest else ''
    if cmd == 'char':
        if text_at is not None and text_at != u:
            flush()
        text_at = u
        text += ' ' if arg == 'SPACE' else arg[0]
        continue
    flush()
    if cmd == 'quit':
        out.append('at %d move 0 0' % u)   # wait for the update, then close
        out.append('quit')
        break
    if cmd in ('screenshot', 'screenshotfull'):
        continue
    if cmd in ('key', 'keydown', 'keyup'):
        arg = str(int(arg)) if arg.isdigit() else str(KEYS[arg])
    out.append('at %d %s %s' % (u, cmd, arg))
flush()
print('\n'.join(out))
