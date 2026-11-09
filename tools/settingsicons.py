"""Draws the Settings sections' icons for Desktop 95 (web/settingsicons.js), our own drawings in the 1995 manner: 32 by 32
with a 16 by 16 version of each, in the 16-colour palette, outlined in black. Each is drawn from a few shapes at its own size
(no smoothing), then written as SVG of one rectangle per run of a colour in a row.

python tools/settingsicons.py            writes web/settingsicons.js
python tools/settingsicons.py --preview  also writes build/settingsicons.png, every icon at 4 times its size
"""
import os, sys
from PIL import Image, ImageDraw

K, MAROON, GREEN, OLIVE, NAVY, PURPLE, TEAL, GREY = "#000000", "#800000", "#008000", "#808000", "#000080", "#800080", "#008080", "#808080"
SILVER, RED, LIME, YELLOW, BLUE, FUCHSIA, AQUA, WHITE = "#c0c0c0", "#ff0000", "#00ff00", "#ffff00", "#0000ff", "#ff00ff", "#00ffff", "#ffffff"


def canvas(n):
    im = Image.new("RGBA", (n, n), (0, 0, 0, 0))
    return im, ImageDraw.Draw(im)


def S(v, n):
    """a coordinate drawn for 32 pixels, at size n"""
    return int(round(v * n / 32))


def box(d, n, x0, y0, x1, y1, fill, outline=K):
    d.rectangle([S(x0, n), S(y0, n), S(x1, n) - 1, S(y1, n) - 1], fill=fill, outline=outline)


def oval(d, n, x0, y0, x1, y1, fill, outline=K):
    d.ellipse([S(x0, n), S(y0, n), S(x1, n) - 1, S(y1, n) - 1], fill=fill, outline=outline)


def poly(d, n, pts, fill, outline=K):
    d.polygon([(S(x, n), S(y, n)) for x, y in pts], fill=fill, outline=outline)


def line(d, n, pts, fill=K):
    d.line([(S(x, n), S(y, n)) for x, y in pts], fill=fill, width=1)


# ---- the icons ----
def players(n):
    im, d = canvas(n)
    # the one behind: green
    oval(d, n, 15, 3, 27, 15, YELLOW)
    box(d, n, 13, 15, 30, 27, GREEN)
    # the one in front: blue
    oval(d, n, 4, 7, 16, 19, YELLOW)
    box(d, n, 2, 19, 19, 31, BLUE)
    return im


def saves(n):
    im, d = canvas(n)
    # a floppy disk: its shutter at the top, its label below
    poly(d, n, [(3, 3), (26, 3), (29, 6), (29, 29), (3, 29)], NAVY)
    box(d, n, 9, 3, 23, 12, SILVER)
    box(d, n, 18, 5, 21, 10, NAVY, NAVY)
    box(d, n, 7, 17, 25, 29, WHITE)
    if n >= 32:
        line(d, n, [(10, 21), (22, 21)], GREY)
        line(d, n, [(10, 24), (19, 24)], GREY)
    return im


def sound(n):
    im, d = canvas(n)
    box(d, n, 3, 11, 10, 21, SILVER)
    poly(d, n, [(10, 11), (18, 4), (18, 28), (10, 21)], GREY)
    for r in ((22, 13, 24, 19), (25, 9, 27, 23), (28, 6, 30, 26)):
        if n < 32 and r[0] == 28: continue
        box(d, n, *r, NAVY, NAVY)
    return im


def look(n):
    im, d = canvas(n)
    # a painter's palette, its thumb hole cut out
    oval(d, n, 2, 5, 30, 27, OLIVE)
    oval(d, n, 4, 7, 28, 25, YELLOW, YELLOW)
    oval(d, n, 17, 17, 23, 23, (0, 0, 0, 0))
    for (x, y, c) in ((7, 10, RED), (13, 8, BLUE), (19, 9, LIME), (24, 12, FUCHSIA), (8, 17, AQUA)):
        box(d, n, x, y, x + 4, y + 4, c)
    return im


def overlays(n):
    im, d = canvas(n)
    # a screen with a little chart and a clock over the picture
    box(d, n, 2, 3, 30, 24, SILVER)
    box(d, n, 5, 6, 27, 21, NAVY)
    box(d, n, 12, 24, 20, 28, GREY)
    box(d, n, 7, 28, 25, 31, SILVER)
    box(d, n, 7, 14, 10, 19, LIME, LIME)
    box(d, n, 11, 11, 14, 19, YELLOW, YELLOW)
    box(d, n, 15, 8, 18, 19, RED, RED)
    if n >= 32:
        oval(d, n, 19, 8, 26, 15, WHITE)
        line(d, n, [(22, 9), (22, 11), (24, 11)])
    return im


def inputs(n):
    im, d = canvas(n)
    # a game pad: its body, the cross at the left, two buttons at the right
    box(d, n, 2, 10, 30, 24, SILVER)
    box(d, n, 2, 18, 9, 28, SILVER)
    box(d, n, 23, 18, 30, 28, SILVER)
    box(d, n, 3, 17, 29, 23, SILVER, SILVER)
    box(d, n, 7, 13, 10, 22, K, K)
    box(d, n, 4, 16, 13, 19, K, K)
    box(d, n, 20, 15, 23, 18, RED)
    box(d, n, 24, 12, 27, 15, BLUE)
    return im


def start(n):
    im, d = canvas(n)
    # a flag on its pole
    box(d, n, 5, 2, 8, 31, GREY)
    poly(d, n, [(8, 3), (28, 3), (23, 9), (28, 15), (8, 15)], RED)
    box(d, n, 2, 28, 12, 31, OLIVE)
    return im


def game(n):
    im, d = canvas(n)
    # a cog
    import math
    pts = []
    for i in range(16):
        a = i * math.pi / 8 + math.pi / 16
        r = 14 if i % 2 == 0 else 10
        for da in (-0.22, 0.22):
            pts.append((16 + r * math.cos(a + da), 16 + r * math.sin(a + da)))
    poly(d, n, pts, SILVER)
    oval(d, n, 11, 11, 21, 21, GREY)
    oval(d, n, 13, 13, 19, 19, WHITE)
    return im


def page(n):
    im, d = canvas(n)
    # a page with its corner turned down, lines of text
    poly(d, n, [(6, 2), (21, 2), (27, 8), (27, 30), (6, 30)], WHITE)
    poly(d, n, [(21, 2), (21, 8), (27, 8)], SILVER)
    for y in (12, 16, 20, 24):
        if n < 32 and y == 24: continue
        line(d, n, [(9, y), (23, y)], NAVY)
    return im


def chat(n):
    im, d = canvas(n)
    # two speech balloons
    box(d, n, 2, 4, 22, 17, WHITE)
    poly(d, n, [(6, 16), (6, 22), (12, 16)], WHITE)
    line(d, n, [(7, 16), (11, 16)], WHITE)
    box(d, n, 12, 13, 30, 25, YELLOW)
    poly(d, n, [(24, 24), (26, 30), (20, 24)], YELLOW)
    line(d, n, [(21, 24), (24, 24)], YELLOW)
    if n >= 32:
        line(d, n, [(5, 8), (18, 8)], GREY)
        line(d, n, [(5, 11), (14, 11)], GREY)
        line(d, n, [(15, 17), (27, 17)], OLIVE)
        line(d, n, [(15, 20), (24, 20)], OLIVE)
    return im


ICONS = [("players", players), ("saves", saves), ("sound", sound), ("look", look), ("overlays", overlays), ("input", inputs),
         ("start", start), ("game", game), ("page", page), ("chat", chat)]


def svg(im):
    n = im.size[0]
    px = im.load()
    rects = []
    for y in range(n):
        x = 0
        while x < n:
            r, g, b, a = px[x, y]
            if a < 128:
                x += 1
                continue
            x1 = x
            while x1 < n and px[x1, y] == px[x, y]:
                x1 += 1
            rects.append("<rect x='%d' y='%d' width='%d' height='1' fill='#%02x%02x%02x'/>" % (x, y, x1 - x, r, g, b))
            x = x1
    return ("<svg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 %d %d' width='%d' height='%d' shape-rendering='crispEdges' "
            "aria-hidden='true' focusable='false'>%s</svg>" % (n, n, n, n, "".join(rects)))


def main():
    root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    out = ["// The Settings sections' icons for Desktop 95, drawn by tools/settingsicons.py (do not edit by hand): 32 by 32 and",
           "// 16 by 16, our own drawings in the 1995 manner. ByopopSettingsIcons[section] = { big: svg, small: svg }.",
           "var ByopopSettingsIcons = {"]
    sheet = Image.new("RGBA", (len(ICONS) * 140, 214), (0, 128, 128, 255))
    for i, (name, fn) in enumerate(ICONS):
        big, small = fn(32), fn(16)
        out.append('  %s: { big: "%s",\n    small: "%s" },' % (name, svg(big), svg(small)))
        sheet.alpha_composite(big.resize((128, 128), Image.NEAREST), (i * 140 + 6, 6))
        sheet.alpha_composite(small.resize((64, 64), Image.NEAREST), (i * 140 + 38, 144))
    out[-1] = out[-1].rstrip(",")
    out.append("};")
    with open(os.path.join(root, "web", "settingsicons.js"), "w", encoding="utf-8", newline="\n") as f:
        f.write("\n".join(out) + "\n")
    if "--preview" in sys.argv:
        os.makedirs(os.path.join(root, "build"), exist_ok=True)
        sheet.save(os.path.join(root, "build", "settingsicons.png"))
    print("wrote web/settingsicons.js,", len(ICONS), "icons")


main()
