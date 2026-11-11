r"""Turns the players' part of the port's registry file (registry.ini, the emulated HKEY_CURRENT_USER) into a .reg file that
origrun imports into the original's sandbox (`--reg`), so that both games start with the same player. The original keeps its
settings under HKEY_LOCAL_MACHINE (SexyAppFramework reads HKLM first), the port's emulation under HKEY_CURRENT_USER.
  ini_to_reg.py <registry.ini> <out.reg>
Only the sections below ...\Users and the version value SGFVersion are written (the settings are each game's own)."""
import re
import sys


def reg_value(text):
    kind, _, value = text.partition(':')
    if kind == 'dword':
        return 'dword:%08x' % (int(value) & 0xffffffff)
    if kind == 'sz':
        return '"%s"' % value.replace('\\', '\\\\').replace('"', '\\"')
    raise SystemExit('ini_to_reg.py: value type %s is not handled' % kind)


def main():
    out = ['Windows Registry Editor Version 5.00', '']
    keep = False
    root = False
    for line in open(sys.argv[1], encoding='utf-8'):
        line = line.rstrip('\r\n')
        m = re.match(r'^\[(.*)\]$', line)
        if m:
            keep = re.search(r'\\Users(\\|$)', m.group(1)) is not None
            root = re.search(r'\\AstroPop$', m.group(1)) is not None
            if keep or root:
                out.append('[HKEY_LOCAL_MACHINE\\%s]' % m.group(1))
        elif (keep or root) and '=' in line:
            name, _, value = line.partition('=')
            # Init deletes every player when SGFVersion is missing or below 15, so that value goes with them
            if keep or name == 'SGFVersion':
                out.append('"%s"=%s' % (name, reg_value(value)))
        elif (keep or root) and not line:
            out.append('')
    with open(sys.argv[2], 'w', encoding='utf-16', newline='') as f:
        f.write('\r\n'.join(out) + '\r\n')


if __name__ == '__main__':
    main()
