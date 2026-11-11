#!/usr/bin/env python3
"""Phase 2 helpers: one list of steps -> the original's origrun script and the port's -script, with screenshots at the same updates.

A step is (update, kind, args...). The update is the original's update counter (`at <update> ...`); the port's line runs one number
later (tsp.PORT_SHIFT: an origrun input at counter N is read by update N+1, a port script line N runs before update N, and a shot
the same: origrun's `at N shot` shows the frame that update N drew, the port's line N+1 saves the frame that update N drew).
The port's shots are its own test command `shot`, which reads the screen back without drawing again (the framework's `screenshot`
draws every widget once more, and Board::Draw and MapScreen::Draw each ease something a step per draw, so it put the port a step
ahead of the original with every shot); it needs -drawevery, which chain.sh passes.
Kinds: click x y [k], rclick x y, move x y, down x y, up x y, key vk, type text (one character an update), text1 c,
shot name (shots/<name>.png in the original, <dir>/port/<name>.png in the port), raw 'orig line' 'port line'.
"""
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.normpath(os.path.join(HERE, "..", "..", "..", ".."))
sys.path.insert(0, os.path.join(HERE, "..", "phase1"))
from tsp import PORT_SHIFT  # noqa: E402

VK = {"enter": 13, "esc": 27, "space": 32, "back": 8, "tab": 9, "left": 37, "up": 38, "right": 39, "down": 40, "shift": 16,
      "delete": 46}


class Tour:
    def __init__(self, name):
        self.name = name
        self.steps = []
        self.dir = os.path.join(ROOT, "build", "ts-p2", name)

    # inputs
    def click(self, u, x, y, k=None):
        self.steps.append((u, "click", x, y, k))

    def rclick(self, u, x, y):
        self.steps.append((u, "rclick", x, y))

    def move(self, u, x, y):
        self.steps.append((u, "move", x, y))

    def down(self, u, x, y):
        self.steps.append((u, "down", x, y))

    def up(self, u, x, y):
        self.steps.append((u, "up", x, y))

    def key(self, u, vk):
        self.steps.append((u, "key", VK.get(vk, vk) if isinstance(vk, str) else vk))

    def type(self, u, text):
        for i, c in enumerate(text):
            self.steps.append((u + i, "text1", c))

    def shot(self, u, name):
        self.steps.append((u, "shot", name))

    def raw(self, u, orig, port):
        self.steps.append((u, "raw", orig, port))

    def peek(self, u, *exprs):
        self.steps.append((u, "peek", list(exprs)))

    # output
    def last(self):
        return max(s[0] for s in self.steps)

    def write(self, quit_at=None):
        os.makedirs(self.dir, exist_ok=True)
        q = quit_at if quit_at is not None else self.last() + 20
        port_dir = os.path.join(self.dir, "port").replace("\\", "/")
        pd = port_dir
        # a Windows-style absolute path for the port (git bash's /c/... is not understood by the exe)
        if pd.startswith("/c/"):
            pd = "C:/" + pd[3:]
        orig, port = [], []
        # at the same update a shot comes before the inputs: origrun's `at N shot` shows the frame update N drew (before the input of N is
        # read by update N+1), and the port's line N+1 draws what the lines before it left (the port's inputs of the same line run first)
        for st in sorted(self.steps, key=lambda s: (s[0], s[1] != "shot")):
            u, kind = st[0], st[1]
            pu = u + PORT_SHIFT
            if kind == "click":
                k = f" {st[4]}" if st[4] else ""
                orig.append(f"at {u} click {st[2]} {st[3]}{k}")
                port.append(f"{pu} click {st[2]} {st[3]}")
            elif kind == "rclick":
                orig.append(f"at {u} rclick {st[2]} {st[3]}")
                port.append(f"{pu} rclick {st[2]} {st[3]}")
            elif kind in ("move", "down", "up"):
                orig.append(f"at {u} {kind} {st[2]} {st[3]}")
                port.append(f"{pu} {kind} {st[2]} {st[3]}")
            elif kind == "key":
                orig.append(f"at {u} key {st[2]}")
                port.append(f"{pu} key {st[2]}")
            elif kind == "text1":
                c = st[2]
                if c == " ":
                    orig.append(f"at {u} key 32")
                    port.append(f"{pu} char SPACE")
                else:
                    orig.append(f"at {u} text {c}")
                    port.append(f"{pu} char {c}")
            elif kind == "shot":
                orig.append(f"at {u} shot {st[2]}")
                port.append(f"{pu} shot {pd}/{st[2]}.png")
            elif kind == "raw":
                orig.append(f"at {u} {st[2]}" if st[2] else "")
                port.append(f"{pu} {st[3]}" if st[3] else "")
            elif kind == "peek":
                orig.append(f"at {u} peek " + " ".join(st[2]))
        orig.append("quit")
        port.append(f"{q + PORT_SHIFT} quit")
        open(os.path.join(self.dir, "orig.txt"), "w", newline="\n").write("\n".join(l for l in orig if l) + "\n")
        open(os.path.join(self.dir, "port.txt"), "w", newline="\n").write("\n".join(l for l in port if l) + "\n")
        return self.dir
