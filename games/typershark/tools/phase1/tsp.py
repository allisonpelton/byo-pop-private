#!/usr/bin/env python3
"""Phase 1 helpers: build the original's origrun script and the port's -script from one list of steps, and the peek
expressions that read the original's Board (offsets from games/typershark/src/Board.h and the entity headers).

A step is (update, kind, args...): ('text', 'abc') types characters, ('click', x, y[, k]), ('key', vk), ('peek', [exprs]).
The port's script has the same inputs at the same update numbers (one `char` line per character, one update apart when
`spread` is true; the original's `text` posts them all at that update, so the generator spreads them the same way for both).
"""
import sys

# an expression is an ADDRESS: "[X]" is the pointer stored at address X; the value read is the int at the final address.
# BRD is the address of the app's Board pointer; board(off) the address of a Board field, player(off) of a Player field.
BRD = "[0x565380]+0x634"


def ind(base, off):
    """the int at (value of base + off); base is itself an expression whose value is a pointer"""
    return f"[{base}]+0x{off:x}" if off >= 0 else f"[{base}]-0x{-off:x}"


def board(off):
    return ind(BRD, off)


def player(off):
    return ind(board(0xd68), off)


def app(off):
    """a field of the app (gApp at 0x565380)"""
    return f"[0x565380]+0x{off:x}"


def spread_text(update, text):
    return [(update + i, 'text1', c) for i, c in enumerate(text)]


def orig_script(steps, peeks_per_line=14):
    """steps: list of (update, kind, ...). Returns the origrun script text."""
    lines = []
    for st in sorted(steps, key=lambda s: s[0]):
        u, kind = st[0], st[1]
        if kind in ('text', 'text1'):
            if st[2] == ' ':
                lines.append(f"at {u} key 32")     # origrun's `text` cannot type a space (the line is trimmed); a posted key is translated to WM_CHAR by the game's pump
            else:
                lines.append(f"at {u} text {st[2]}")
        elif kind == 'click':
            k = f" {st[4]}" if len(st) > 4 else ""
            lines.append(f"at {u} click {st[2]} {st[3]}{k}")
        elif kind == 'key':
            lines.append(f"at {u} key {st[2]}")
        elif kind == 'peek':
            ex = list(st[2])
            for i in range(0, len(ex), peeks_per_line):
                lines.append(f"at {u} peek " + " ".join(ex[i:i + peeks_per_line]))
        else:
            raise ValueError(kind)
    lines.append("quit")
    return "\n".join(lines) + "\n"


PORT_SHIFT = 1     # an origrun `at N` input is posted when the counter reads N and is read by update N+1; the port's -script line
                   # `N` runs before update N: the port's input lines are shifted by one so that both reach the same update


def port_script(steps, extra=()):
    """the same inputs as -script lines `<update> <command>` (shifted by PORT_SHIFT)"""
    lines = []
    for st in sorted(steps, key=lambda s: s[0]):
        u, kind = st[0] + PORT_SHIFT, st[1]
        if kind in ('text', 'text1'):
            for i, c in enumerate(st[2]):
                lines.append(f"{u} char {'SPACE' if c == ' ' else c}")
        elif kind == 'click':
            lines.append(f"{u} click {st[2]} {st[3]}")      # a held click (k) is not scripted
        elif kind == 'key':
            lines.append(f"{u} key {st[2]}")
        elif kind == 'peek':
            pass
        else:
            raise ValueError(kind)
    lines += list(extra)
    return "\n".join(lines) + "\n"
