"""inlinedead.py <folder>...: member functions defined inline in a header whose name no other line of the folders'
sources uses (README, "Unneeded code"). An inline function nothing calls is never compiled, so the linker's list of
discarded functions (gcdead.py) cannot show it. Names are matched as words, so a name shared with anything else hides a
function: the output is a list to read, not a verdict, and virtual overrides are left out.

    python -I games/bookwormadv/tools/coverage/inlinedead.py games/bookwormadv/{src,jefflib,luaapp,lualib,popanim}
"""
import collections
import glob
import os
import re
import sys

INLINE = re.compile(r'^\s*(?:static\s+|virtual\s+|inline\s+|constexpr\s+)*[\w:<>*&, ]+?[\s*&](\w+)\s*\([^;{]*\)\s*'
                    r'(?:const\s*)?(?:noexcept\s*)?\{')
STATEMENTS = ('return', 'if', 'else', 'for', 'while', '//')


def main():
    files = [f for d in sys.argv[1:] for f in glob.glob(os.path.join(d, '**', '*.*'), recursive=True)
             if f.endswith(('.h', '.cpp', '.c', '.inl'))]
    text = {f: open(f, encoding='utf-8', errors='replace').read() for f in files}
    uses = collections.Counter()
    for t in text.values():
        uses.update(re.findall(r'\b\w+\b', t))
    for f in files:
        if not f.endswith('.h'):
            continue
        for n, line in enumerate(text[f].split('\n'), 1):
            m = INLINE.match(line)
            if m and 'override' not in line and not line.lstrip().startswith(STATEMENTS) and uses[m.group(1)] <= 1:
                print('%s:%d: %s' % (f.replace(os.sep, '/'), n, line.strip()[:110]))


if __name__ == '__main__':
    main()
