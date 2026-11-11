# Builds the site's text font, web/fonts/byopop-sans.woff ("Byopop Sans"), a derivative of W95FA by Alina Sava under the SIL
# Open Font License 1.1 (web/licenses/byopop-sans.txt; docs/WEB_PLAYER.md, "Desktop 95", "Its text").
#
# W95FA draws its glyphs on a grid of 12.5 pixels to the em and gives them advances that are not whole pixels, so no CSS size
# shows it sharp with even spacing. This script takes its glyphs as the pixels they are on that grid and writes them as a new
# font whose em is 11 pixels (so "font-size: 11px" is the size the 1995 desktop's text had, 13 pixels from ascent to descent),
# every pixel a square of 100 units, with advances in whole pixels by one rule: to the end of the glyph's ink (its left bearing,
# mostly one pixel, is the gap between letters), with one more pixel for the capitals and marks in WIDER; the digits all as wide
# as the widest; the space 3. To these it adds the characters the site uses that W95FA
# lacks, drawn here by hand on the same grid (EXTRA below): our own drawings.
#
#   python tools/web/pixelfont.py            writes web/fonts/byopop-sans.woff
#   python tools/web/pixelfont.py --check    builds in memory and says whether the file in the repo is the same
#
# Needs Pillow (to read the pixels of W95FA's WOFF2) and fontTools; WOFF, not WOFF2, since that needs no Brotli module.
import io, os, sys
from PIL import ImageFont
from fontTools.fontBuilder import FontBuilder
from fontTools.pens.ttGlyphPen import TTGlyphPen

HERE = os.path.dirname(os.path.abspath(__file__))
SOURCE = os.path.join(HERE, "w95fa.woff2")   # W95FA 1.002, the Latin subset of @fontsource/win95fa 5.3.0, unchanged
OUT = os.path.join(HERE, "..", "..", "web", "fonts", "byopop-sans.woff")
GRID = 12.5          # W95FA's pixels to the em
PX = 100             # one pixel in font units; the em is 11 pixels
ASCENT, DESCENT = 11, 2

# The glyphs that get one more pixel of advance than the rule gives: the advance the 1995 system font gave them (the user,
# 2026-11-11: "Match widths"), taken as numbers, as a metric-compatible font takes another's widths; no shape is changed.
WIDER = "BDEGHIMNOQRSTU!.:"

# The characters W95FA lacks, drawn here: the advance in pixels, the row of the first line counted down from the top of the
# 13-pixel cell (row 10 stands on the baseline), and the lines, '#' a pixel.
EXTRA = {
    "`": (3, 2, [".#", "..#"]),
    "·": (3, 7, [".#"]),
    "×": (7, 5, [".#...#", "..#.#", "...#", "..#.#", ".#...#"]),
    "–": (6, 7, ["######"]),
    "—": (11, 7, ["###########"]),
    "…": (9, 10, [".#..#..#"]),
    "←": (9, 5, ["...#", "..#", ".#######", "..#", "...#"]),
    "→": (9, 5, [".....#", "......#", ".#######", "......#", ".....#"]),
    "↑": (7, 3, ["...#", "..###", ".#.#.#", "...#", "...#", "...#", "...#"]),
    "↓": (7, 3, ["...#", "...#", "...#", "...#", ".#.#.#", "..###", "...#"]),
    "¼": (10, 2, [".#....#", "##...#", ".#...#", ".#..#", "....#..#", "...#..##", "...#.#.#", "..#..####", "..#....#"]),
    "½": (10, 2, [".#....#", "##...#", ".#...#", ".#..#", "....#.##", "...#.#..#", "...#...#", "..#...#", "..#..####"]),
    "¾": (10, 2, ["###...#", "..#..#", ".#...#", "..#.#", "##..#..#", "...#..##", "...#.#.#", "..#..####", "..#....#"]),
    "©": (11, 2, ["...#####", "..#.....#", ".#..###..#", ".#.#.....#", ".#.#.....#", ".#.#.....#", ".#..###..#", "..#.....#", "...#####"]),
    "™": (11, 2, [".###.#...#", "..#..##.##", "..#..#.#.#", "..#..#...#"]),
}


def source_glyphs():
    """W95FA's glyphs as pixels: {char: (left bearing, set of (x, row))}, row counted down from the top of the cell."""
    f = ImageFont.truetype(SOURCE, GRID)
    def pixels(ch):
        m = f.getmask(ch, mode="1", anchor="ls")
        bb = f.getbbox(ch, mode="1", anchor="ls")
        return frozenset((x + bb[0], y + bb[1] + ASCENT) for y in range(m.size[1]) for x in range(m.size[0]) if m.getpixel((x, y)))
    missing = pixels("￿")
    out = {}
    for code in list(range(33, 127)) + list(range(161, 0x250)) + [0x2018, 0x2019, 0x201A, 0x201C, 0x201D, 0x201E, 0x2022, 0x20AC]:
        ch = chr(code)
        px = pixels(ch)
        if not px or px == missing:
            continue
        out[ch] = px
    return out


def build():
    glyphs = {}   # char -> (advance, pixels)
    for ch, px in source_glyphs().items():
        # the advance: to one pixel past the ink (a glyph's own left bearing, mostly one pixel, is the gap before it)
        glyphs[ch] = (max(x for x, y in px) + 1, px)
    for ch in WIDER:
        glyphs[ch] = (glyphs[ch][0] + 1, glyphs[ch][1])
    digits = max(glyphs[d][0] for d in "0123456789")
    for d in "0123456789":
        glyphs[d] = (digits, glyphs[d][1])
    for ch, (adv, top, rows) in EXTRA.items():
        glyphs[ch] = (adv, frozenset((x, top + r) for r, line in enumerate(rows) for x, c in enumerate(line) if c == "#"))
    glyphs[" "] = (3, frozenset())
    glyphs[" "] = glyphs[" "]

    order = [".notdef"] + ["u%04X" % ord(ch) for ch in sorted(glyphs)]
    cmap = {ord(ch): "u%04X" % ord(ch) for ch in glyphs}
    fb = FontBuilder(11 * PX, isTTF=True)
    fb.setupGlyphOrder(order)
    fb.setupCharacterMap(cmap)
    drawn, metrics = {}, {}

    def draw(px):
        pen = TTGlyphPen(None)
        rows = {}
        for x, y in px:
            rows.setdefault(y, []).append(x)
        for y, xs in sorted(rows.items()):
            xs.sort()
            start = prev = xs[0]
            runs = []
            for x in xs[1:]:
                if x != prev + 1:
                    runs.append((start, prev)); start = x
                prev = x
            runs.append((start, prev))
            top, bottom = (ASCENT - y) * PX, (ASCENT - y - 1) * PX
            for a, b in runs:
                pen.moveTo((a * PX, bottom)); pen.lineTo((a * PX, top)); pen.lineTo(((b + 1) * PX, top)); pen.lineTo(((b + 1) * PX, bottom)); pen.closePath()
        return pen.glyph()

    # .notdef: a hollow box
    box = frozenset([(x, y) for x in range(1, 6) for y in (2, 10)] + [(x, y) for x in (1, 5) for y in range(2, 11)])
    drawn[".notdef"] = draw(box); metrics[".notdef"] = (7 * PX, PX)
    for ch, (adv, px) in glyphs.items():
        name = "u%04X" % ord(ch)
        drawn[name] = draw(px)
        metrics[name] = (adv * PX, (min(x for x, y in px) * PX) if px else 0)
    fb.setupGlyf(drawn)
    fb.setupHorizontalMetrics(metrics)
    fb.setupHorizontalHeader(ascent=ASCENT * PX, descent=-DESCENT * PX)
    fb.setupNameTable({"familyName": "Byopop Sans", "styleName": "Regular",
                       "copyright": "Glyphs from W95FA, Copyright (c) Alina Sava; changes and added glyphs by the byo-pop project.",
                       "licenseDescription": "This Font Software is licensed under the SIL Open Font License, Version 1.1.",
                       "licenseInfoURL": "https://openfontlicense.org"})
    fb.setupOS2(sTypoAscender=ASCENT * PX, sTypoDescender=-DESCENT * PX, sTypoLineGap=0, usWinAscent=ASCENT * PX, usWinDescent=DESCENT * PX,
                sxHeight=6 * PX, sCapHeight=9 * PX)
    fb.setupPost()
    # the same bytes for the same glyphs: no time of building in the file
    fb.font["head"].created = fb.font["head"].modified = 0
    fb.font.flavor = "woff"
    buf = io.BytesIO()
    fb.save(buf)
    return buf.getvalue(), glyphs


def main():
    data, glyphs = build()
    if "--check" in sys.argv:
        same = os.path.exists(OUT) and open(OUT, "rb").read() == data
        print("pixelfont: %d glyphs, %d bytes, %s" % (len(glyphs), len(data), "the file is up to date" if same else "THE FILE DIFFERS"))
        sys.exit(0 if same else 1)
    with open(OUT, "wb") as f:
        f.write(data)
    print("pixelfont: wrote %s, %d glyphs, %d bytes" % (os.path.relpath(OUT), len(glyphs), len(data)))


if __name__ == "__main__":
    main()
