"""addkeys.py <origrun script> <first at> <span> <vk>...: holds each virtual key in turn for <span> updates from <first at> on (keydown,
then keyup), inside the script, whose lines stay in the order of their times. The original reads the developers' test keys 0..9 and A..D
(0x30..0x39, 0x41..0x44) while a power-up is made: the one held then is its kind (docs/README.md, Coverage check)."""
import re
import sys

path, first, span = sys.argv[1], int(sys.argv[2]), int(sys.argv[3])
keys = [int(v, 0) for v in sys.argv[4:]]
lines = [l.rstrip("\n") for l in open(path)]
for i, vk in enumerate(keys):
    t = first + i * span
    lines.append("at %d keydown %d" % (t, vk))
    lines.append("at %d keyup %d" % (t + span - 100, vk))


def when(line):
    m = re.match(r"at (\d+) ", line)
    return int(m.group(1)) if m else 10 ** 9


lines.sort(key=when)
open(path, "w").write("\n".join(lines) + "\n")
