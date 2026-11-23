"""Draws the site's apps' icons for Desktop 95 (web/appicons.js): every window that is not a game (All games, the Library,
Setup, Settings, Music, Sounds, Splits, the chat, System Information or About, Performance, the Trophy Cabinet, Fonts, the
Gallery, Guides, and a game page's Controls, Game files, Mods and Replays), our own drawings in the 1995 manner: 32 by 32 with a
16 by 16 version of each, in the 16-colour palette, outlined in black, as tools/settingsicons.py draws the Settings
sections'. Each is drawn from a few shapes at its own size (no smoothing), then written as SVG of one rectangle per run of a
colour in a row. web/skins.js draws the one picture wherever the window's picture goes (its desktop icon, title bar, task bar
button, the Pop menu and Alt+Tab).

python tools/appicons.py            writes web/appicons.js
python tools/appicons.py --preview  also writes build/appicons.png, every icon at 4 times its size
"""
import os, sys
from PIL import Image, ImageDraw

K, MAROON, GREEN, OLIVE, NAVY, PURPLE, TEAL, GREY = "#000000", "#800000", "#008000", "#808000", "#000080", "#800080", "#008080", "#808080"
SILVER, RED, LIME, YELLOW, BLUE, FUCHSIA, AQUA, WHITE = "#c0c0c0", "#ff0000", "#00ff00", "#ffff00", "#0000ff", "#ff00ff", "#00ffff", "#ffffff"
CLEAR = (0, 0, 0, 0)


def canvas(n):
    im = Image.new("RGBA", (n, n), CLEAR)
    return im, ImageDraw.Draw(im)


def S(v, n):
    """a coordinate drawn for 32 pixels, at size n"""
    return int(round(v * n / 32))


def box(d, n, x0, y0, x1, y1, fill, outline=K):
    """a rectangle, at least a pixel across at the small size"""
    a, b, c, e = S(x0, n), S(y0, n), S(x1, n) - 1, S(y1, n) - 1
    d.rectangle([a, b, max(a, c), max(b, e)], fill=fill, outline=outline)


def oval(d, n, x0, y0, x1, y1, fill, outline=K):
    d.ellipse([S(x0, n), S(y0, n), S(x1, n) - 1, S(y1, n) - 1], fill=fill, outline=outline)


def poly(d, n, pts, fill, outline=K):
    d.polygon([(S(x, n), S(y, n)) for x, y in pts], fill=fill, outline=outline)


def line(d, n, pts, fill=K):
    d.line([(S(x, n), S(y, n)) for x, y in pts], fill=fill, width=1)


def dot(d, n, x, y, fill):
    """one pixel of the drawing's own size at (x, y) in its own coordinates"""
    d.point((x, y), fill=fill)


# ---- the icons ----
def games(n):
    # All games: four of the listing's caps, each in its own colour with the cap's ring
    im, d = canvas(n)
    for x, y, c, ring in ((2, 2, RED, MAROON), (16, 2, BLUE, NAVY), (2, 16, LIME, GREEN), (16, 16, YELLOW, OLIVE)):
        oval(d, n, x, y, x + 14, y + 14, c)
        if n >= 32:
            oval(d, n, x + 3, y + 3, x + 11, y + 11, None, ring)
    return im


def library(n):
    # the Library: books on a shelf, the last one leaning
    im, d = canvas(n)
    for x0, x1, top, c in ((3, 8, 7, MAROON), (8, 13, 4, NAVY), (13, 17, 9, GREEN), (17, 22, 5, PURPLE)):
        # (at 16 the books touch without a black line between them, which would leave only black)
        box(d, n, x0, top, x1, 26, c, K if n >= 32 else c)
        if n >= 32:
            line(d, n, [(x0 + 1, top + 3), (x1 - 2, top + 3)], YELLOW)
            line(d, n, [(x0 + 1, 22), (x1 - 2, 22)], YELLOW)
    poly(d, n, [(22, 26), (27, 8), (31, 9), (26, 26)], TEAL)
    box(d, n, 1, 26, 31, 30, OLIVE)
    return im


def setup(n):
    # Setup: an open box, a green arrow going into it
    im, d = canvas(n)
    poly(d, n, [(4, 15), (1, 10), (9, 10), (12, 15)], OLIVE)
    poly(d, n, [(20, 15), (23, 10), (31, 10), (28, 15)], OLIVE)
    box(d, n, 4, 15, 28, 30, YELLOW)
    if n >= 32:
        line(d, n, [(5, 16), (26, 16)], OLIVE)
        box(d, n, 8, 21, 16, 26, WHITE)
    box(d, n, 13, 1, 19, 9, LIME)
    poly(d, n, [(9, 9), (23, 9), (16, 17)], LIME)
    return im


def settings(n):
    # Settings: a panel of three sliders, each set at its own height
    im, d = canvas(n)
    box(d, n, 2, 2, 30, 30, SILVER)
    if n >= 32:
        line(d, n, [(3, 3), (28, 3)], WHITE)
        line(d, n, [(3, 3), (3, 28)], WHITE)
    for x, knob, c in ((8, 17, NAVY), (16, 7, RED), (24, 20, GREEN)):
        box(d, n, x - 1, 6, x + 1, 27, K, K)
        box(d, n, x - 4, knob, x + 4, knob + 6, c)
    return im


def music(n):
    # Music: two notes joined by their beam, in front of a silver disc (which keeps them seen on a dark desktop)
    im, d = canvas(n)
    oval(d, n, 0, 1, 30, 31, SILVER)
    if n >= 32:
        oval(d, n, 4, 5, 26, 27, None, WHITE)
    oval(d, n, 12, 13, 18, 19, CLEAR)
    box(d, n, 14, 8, 17, 26, K, K)
    box(d, n, 28, 5, 31, 23, K, K)
    poly(d, n, [(14, 8), (31, 4), (31, 10), (14, 14)], K, K)
    oval(d, n, 6, 21, 17, 30, NAVY)
    oval(d, n, 20, 18, 31, 27, NAVY)
    return im


def sounds(n):
    # Sounds: a board of six pads, each a sound
    im, d = canvas(n)
    box(d, n, 1, 5, 31, 28, SILVER)
    for i, c in enumerate((RED, YELLOW, LIME, BLUE, AQUA, FUCHSIA)):
        x, y = 4 + (i % 3) * 9, 8 + (i // 3) * 9
        box(d, n, x, y, x + 7, y + 7, c)
    return im


def splits(n):
    # Splits: a stopwatch, its crown on top
    im, d = canvas(n)
    box(d, n, 13, 1, 19, 5, SILVER)
    box(d, n, 15, 4, 17, 7, GREY)
    poly(d, n, [(24, 5), (27, 8), (25, 10), (22, 7)], SILVER)
    oval(d, n, 3, 6, 29, 32, WHITE)
    if n >= 32:
        for x, y in ((15, 8), (15, 28), (5, 18), (25, 18)):
            box(d, n, x, y, x + 2, y + 2, K, K)
    line(d, n, [(16, 19), (16, 11)])
    line(d, n, [(16, 19), (22, 19)])
    line(d, n, [(16, 19), (11, 25)], RED)
    return im


def chat(n):
    # the chat: one balloon of someone typing, three dots in it
    im, d = canvas(n)
    box(d, n, 1, 4, 31, 22, AQUA)
    poly(d, n, [(7, 21), (5, 29), (14, 21)], AQUA)
    line(d, n, [(8, 21), (13, 21)], AQUA)
    for x in (7, 14, 21):
        box(d, n, x, 11, x + 4, 15, K, K)
    return im


def about(n):
    # System Information (About on a game's page): the computer itself, a tower with its drives and its light
    im, d = canvas(n)
    box(d, n, 8, 1, 24, 31, SILVER)
    if n >= 32:
        line(d, n, [(9, 2), (22, 2)], WHITE)
        line(d, n, [(9, 2), (9, 29)], WHITE)
    box(d, n, 11, 5, 21, 9, GREY)
    box(d, n, 11, 11, 21, 15, GREY)
    if n >= 32:
        line(d, n, [(13, 13), (19, 13)], K)
        line(d, n, [(11, 20), (20, 20)], GREY)
        line(d, n, [(11, 22), (20, 22)], GREY)
    box(d, n, 18, 25, 21, 28, LIME)
    return im


def perf(n):
    # Performance: a line chart on squared paper
    im, d = canvas(n)
    box(d, n, 1, 3, 31, 29, WHITE)
    if n >= 32:
        for v in (8, 15, 22):
            line(d, n, [(v, 4), (v, 27)], SILVER)
        for v in (9, 15, 21):
            line(d, n, [(2, v), (29, v)], SILVER)
    pts = [(3, 25), (9, 17), (14, 21), (20, 10), (24, 14), (29, 5)]
    line(d, n, pts, RED)
    if n >= 32:
        line(d, n, [(x, y + 1) for x, y in pts], RED)
    return im


def trophies(n):
    # the Trophy Cabinet: a cup on its base
    im, d = canvas(n)
    oval(d, n, 1, 5, 11, 15, None)
    oval(d, n, 21, 5, 31, 15, None)
    if n >= 32:
        oval(d, n, 2, 6, 10, 14, None, YELLOW)
        oval(d, n, 22, 6, 30, 14, None, YELLOW)
    poly(d, n, [(6, 3), (26, 3), (24, 14), (19, 19), (13, 19), (8, 14)], YELLOW)
    if n >= 32:
        line(d, n, [(10, 5), (11, 13)], WHITE)
        line(d, n, [(22, 5), (21, 14)], OLIVE)
    box(d, n, 14, 19, 18, 24, OLIVE)
    box(d, n, 9, 24, 23, 27, YELLOW)
    box(d, n, 7, 27, 25, 31, MAROON)
    return im


def fontview(n):
    # Fonts: a letter A with its serifs, drawn as its outline (white inside, which shows on any desktop)
    im, d = canvas(n)
    poly(d, n, [(13, 2), (19, 2), (27, 26), (30, 26), (30, 30), (19, 30), (19, 26), (21, 26), (19, 21), (13, 21), (11, 26),
                (13, 26), (13, 30), (2, 30), (2, 26), (5, 26)], WHITE)
    poly(d, n, [(13, 17), (19, 17), (16, 8)], CLEAR, K)
    return im


def gallery(n):
    # the Gallery: a picture in a gold frame, a landscape with its sun
    im, d = canvas(n)
    box(d, n, 1, 4, 31, 29, YELLOW)
    if n >= 32:
        line(d, n, [(2, 27), (29, 27)], OLIVE)
        line(d, n, [(29, 5), (29, 27)], OLIVE)
    box(d, n, 5, 8, 27, 25, AQUA)
    poly(d, n, [(6, 23), (12, 15), (16, 19), (20, 14), (26, 23)], GREEN, GREEN)
    box(d, n, 6, 22, 26, 24, GREEN, GREEN)
    oval(d, n, 8, 10, 13, 15, YELLOW, YELLOW if n < 32 else OLIVE)
    return im


def guides(n):
    # Guides: a closed book, a question mark on its cover
    im, d = canvas(n)
    box(d, n, 7, 27, 28, 31, WHITE)
    box(d, n, 4, 1, 26, 29, NAVY)
    box(d, n, 4, 1, 8, 29, BLUE)
    if n >= 32:
        for r in ((13, 6, 21, 9), (20, 8, 23, 14), (16, 13, 21, 16), (16, 15, 19, 20), (16, 22, 19, 25)):
            box(d, n, *r, YELLOW, YELLOW)
    else:
        for x, y in ((7, 3), (8, 3), (9, 3), (10, 4), (10, 5), (9, 6), (8, 7), (8, 8), (8, 10)):
            dot(d, n, x, y, YELLOW)
    return im


def controls(n):
    # a game's Controls: a keyboard
    im, d = canvas(n)
    box(d, n, 1, 10, 31, 27, SILVER)
    if n >= 32:
        for y in (13, 17):
            for x in range(4, 28, 4):
                box(d, n, x, y, x + 3, y + 3, WHITE)
        box(d, n, 4, 21, 8, 24, WHITE)
        box(d, n, 9, 21, 23, 24, WHITE)
        box(d, n, 24, 21, 28, 24, WHITE)
    else:
        for y in (7, 9):
            for x in range(2, 14, 2):
                dot(d, n, x, y, K)
        line(d, n, [(4, 11), (11, 11)], K)
    return im


def files(n):
    # a game's Game files: a folder with its pages showing
    im, d = canvas(n)
    poly(d, n, [(2, 6), (11, 6), (14, 9), (29, 9), (29, 29), (2, 29)], OLIVE)
    box(d, n, 6, 4, 22, 18, WHITE)
    if n >= 32:
        line(d, n, [(8, 7), (19, 7)], GREY)
        line(d, n, [(8, 10), (17, 10)], GREY)
    poly(d, n, [(1, 13), (27, 13), (31, 29), (2, 29)], YELLOW)
    return im


def mods(n):
    # a game's Mods: a wrench over a screwdriver
    im, d = canvas(n)
    # the screwdriver under it: its shaft from the top left, its red handle at the bottom right
    poly(d, n, [(3, 5), (5, 3), (19, 17), (17, 19)], GREY)
    poly(d, n, [(16, 20), (20, 16), (30, 26), (26, 30)], RED)
    poly(d, n, [(3, 24), (19, 8), (23, 12), (7, 28)], SILVER)
    oval(d, n, 16, 2, 30, 16, SILVER)
    poly(d, n, [(22, 1), (31, 1), (31, 10), (24, 9)], CLEAR, CLEAR)
    return im


def replays(n):
    # a game's Replays: a video cassette, its two reels showing through its window
    im, d = canvas(n)
    box(d, n, 1, 7, 31, 26, K, K)
    box(d, n, 5, 9, 27, 14, WHITE, WHITE if n < 32 else K)
    box(d, n, 7, 16, 25, 23, GREY)
    for x in (8, 18):
        oval(d, n, x, 16, x + 7, 23, WHITE)
        if n >= 32:
            box(d, n, x + 3, 19, x + 5, 21, K, K)
    return im


ICONS = [("games", games), ("library", library), ("setup", setup), ("settings", settings), ("music", music), ("sounds", sounds),
         ("splits", splits), ("chat", chat), ("about", about), ("perf", perf), ("trophies", trophies), ("fontview", fontview),
         ("gallery", gallery), ("guides", guides), ("controls", controls), ("files", files), ("mods", mods),
         ("replays", replays)]


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
    out = ["// The site's apps' icons for Desktop 95, drawn by tools/appicons.py (do not edit by hand): 32 by 32 and 16 by 16, our",
           "// own drawings in the 1995 manner. ByopopAppIcons[app] = { big: svg, small: svg }; web/skins.js maps each window to its app.",
           "var ByopopAppIcons = {"]
    sheet = Image.new("RGBA", (len(ICONS) * 140, 214), (0, 128, 128, 255))
    for i, (name, fn) in enumerate(ICONS):
        big, small = fn(32), fn(16)
        out.append('  %s: { big: "%s",\n    small: "%s" },' % (name, svg(big), svg(small)))
        sheet.alpha_composite(big.resize((128, 128), Image.NEAREST), (i * 140 + 6, 6))
        sheet.alpha_composite(small.resize((64, 64), Image.NEAREST), (i * 140 + 38, 144))
    out[-1] = out[-1].rstrip(",")
    out.append("};")
    out.append('if (typeof module !== "undefined" && module.exports) module.exports = ByopopAppIcons;')
    with open(os.path.join(root, "web", "appicons.js"), "w", encoding="utf-8", newline="\n") as f:
        f.write("\n".join(out) + "\n")
    if "--preview" in sys.argv:
        os.makedirs(os.path.join(root, "build"), exist_ok=True)
        sheet.save(os.path.join(root, "build", "appicons.png"))
    print("wrote web/appicons.js,", len(ICONS), "icons")


main()
