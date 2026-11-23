# Builds the 2000 skin's text font, web/fonts/byopop-sans-2000.woff ("Byopop Sans 2000"), and its bold,
# web/fonts/byopop-sans-2000-bold.woff: a pixel font in the manner of Tahoma at 8 points, which Windows 2000 brought in place of MS Sans Serif (docs/SKINS.md, "Windows 2000"). Every glyph is
# drawn here by hand on Byopop Sans's grid (tools/web/pixelfont.py): an em of 11 pixels, a cell of 13 rows, row 10 standing
# on the baseline. Tahoma's manner is in its numbers: capitals and ascenders 8 pixels tall where Byopop Sans's are 9, the
# x-height 6, narrower capitals, and each character's advance at 11 pixels to the em, taken as numbers from Tahoma's own
# table of them (its hdmx), as a metric-compatible font takes another's widths. No shape is taken from it.
#
#   python tools/web/pixelfont2000.py            writes both files
#   python tools/web/pixelfont2000.py --check    builds in memory and says whether the files in the repo are the same
#   python tools/web/pixelfont2000.py --show "text" [--bold]    prints the text as pixels
#
# Both faces are the project's own drawing, dedicated to the public domain under CC0 1.0 (the user, 2026-11-23: "CC0";
# web/licenses/byopop-sans-2000.txt).
#
# The printable ASCII is drawn, and the Latin-1 letters with accents are built from it (COMPOSED). A character not here is
# drawn by the next font in the skin's list, Byopop Sans, whose baseline and x-height are the same.
import os, sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import pixelfont  # noqa: E402  (font_bytes, the shared writer)

OUT = os.path.join(HERE, "..", "..", "web", "fonts", "byopop-sans-2000.woff")
OUT_BOLD = os.path.join(HERE, "..", "..", "web", "fonts", "byopop-sans-2000-bold.woff")

# Each glyph: its advance in pixels, the row of its first line counted down from the top of the cell, and its lines, '#' a
# pixel. Column 0 is the gap before the glyph, and its ink ends at the advance's last column, the next glyph's column 0
# being the gap after it. Capitals and digits stand on rows 3 to 10, the x-height is rows 5 to 10, descenders rows 11 and 12.
GLYPHS = {
    "!": (4, 3, ["..#", "..#", "..#", "..#", "..#", "..#", "", "..#"]),
    '"': (4, 3, [".#.#", ".#.#"]),
    "#": (8, 3, ["..#..#", "..#..#", ".######", "..#..#", "..#..#", ".######", "..#..#", "..#..#"]),
    "$": (6, 2, ["...#", "..####", ".#.#", ".#.#", "..###", "...#.#", "...#.#", ".####", "...#"]),
    "%": (11, 3, ["..##....#", ".#..#..#", ".#..#..#", "..##..#", ".....#.##", "....#.#..#", "....#.#..#", "...#...##"]),
    "&": (7, 3, ["..##", ".#..#", ".#..#", "..##..#", ".#..#.#", ".#...#", ".#...##", "..###.#"]),
    "'": (2, 3, [".#", ".#"]),
    "(": (4, 2, ["...#", "..#", "..#", ".#", ".#", ".#", ".#", "..#", "..#", "...#"]),
    ")": (4, 2, [".#", "..#", "..#", "...#", "...#", "...#", "...#", "..#", "..#", ".#"]),
    "*": (6, 3, ["...#", ".#.#.#", "..###", ".#.#.#", "...#"]),
    "+": (8, 4, ["....#", "....#", "....#", ".#######", "....#", "....#", "....#"]),
    ",": (4, 9, ["..#", "..#", ".#"]),
    "-": (4, 7, [".###"]),
    ".": (4, 10, ["..#"]),
    "/": (4, 2, ["...#", "...#", "...#", "..#", "..#", "..#", "..#", ".#", ".#", ".#"]),
    "0": (6, 3, ["..###", ".#...#", ".#...#", ".#...#", ".#...#", ".#...#", ".#...#", "..###"]),
    "1": (6, 3, ["...#", ".###", "...#", "...#", "...#", "...#", "...#", ".#####"]),
    "2": (6, 3, ["..###", ".#...#", ".....#", "....#", "...#", "..#", ".#", ".#####"]),
    "3": (6, 3, ["..###", ".#...#", ".....#", "...##", ".....#", ".....#", ".#...#", "..###"]),
    "4": (6, 3, ["....#", "...##", "..#.#", "..#.#", ".#..#", ".#####", "....#", "....#"]),
    "5": (6, 3, [".#####", ".#", ".####", ".....#", ".....#", ".....#", ".#...#", "..###"]),
    "6": (6, 3, ["...##", "..#", ".#", ".####", ".#...#", ".#...#", ".#...#", "..###"]),
    "7": (6, 3, [".#####", ".....#", "....#", "....#", "...#", "...#", "..#", "..#"]),
    "8": (6, 3, ["..###", ".#...#", ".#...#", "..###", ".#...#", ".#...#", ".#...#", "..###"]),
    "9": (6, 3, ["..###", ".#...#", ".#...#", ".#...#", "..####", ".....#", "....#", ".##"]),
    ":": (4, 5, ["..#", "", "", "", "", "..#"]),
    ";": (4, 5, ["..#", "", "", "", "", "..#", ".#"]),
    "<": (8, 4, ["......#", "....##", "..##", ".#", "..##", "....##", "......#"]),
    "=": (8, 6, [".#######", "", ".#######"]),
    ">": (8, 4, [".#", "..##", "....##", "......#", "....##", "..##", ".#"]),
    "?": (5, 3, [".###", "....#", "....#", "...#", "..#", "..#", "", "..#"]),
    "@": (10, 3, ["...#####", "..#.....#", ".#..###.#", ".#.#..#.#", ".#.#..#.#", ".#.#..#.#", ".#..##.##", "..#", "...#####"]),
    "A": (7, 3, ["...##", "...##", "..#..#", "..#..#", ".#....#", ".######", ".#....#", ".#....#"]),
    "B": (6, 3, [".####", ".#...#", ".#...#", ".####", ".#...#", ".#...#", ".#...#", ".####"]),
    "C": (7, 3, ["...###", "..#...#", ".#", ".#", ".#", ".#", "..#...#", "...###"]),
    "D": (7, 3, [".####", ".#...#", ".#....#", ".#....#", ".#....#", ".#....#", ".#...#", ".####"]),
    "E": (6, 3, [".#####", ".#", ".#", ".####", ".#", ".#", ".#", ".#####"]),
    "F": (6, 3, [".#####", ".#", ".#", ".####", ".#", ".#", ".#", ".#"]),
    "G": (7, 3, ["...###", "..#...#", ".#", ".#", ".#..###", ".#....#", "..#...#", "...####"]),
    "H": (7, 3, [".#....#", ".#....#", ".#....#", ".######", ".#....#", ".#....#", ".#....#", ".#....#"]),
    "I": (4, 3, [".###", "..#", "..#", "..#", "..#", "..#", "..#", ".###"]),
    "J": (5, 3, ["....#", "....#", "....#", "....#", "....#", "....#", ".#..#", "..##"]),
    "K": (6, 3, [".#...#", ".#..#", ".#.#", ".##", ".#.#", ".#..#", ".#...#", ".#...#"]),
    "L": (5, 3, [".#", ".#", ".#", ".#", ".#", ".#", ".#", ".####"]),
    "M": (8, 3, [".##...##", ".##...##", ".#.#.#.#", ".#.#.#.#", ".#..#..#", ".#..#..#", ".#.....#", ".#.....#"]),
    "N": (7, 3, [".##...#", ".#.#..#", ".#.#..#", ".#..#.#", ".#..#.#", ".#...##", ".#...##", ".#....#"]),
    "O": (8, 3, ["...###", "..#...#", ".#.....#", ".#.....#", ".#.....#", ".#.....#", "..#...#", "...###"]),
    "P": (6, 3, [".####", ".#...#", ".#...#", ".####", ".#", ".#", ".#", ".#"]),
    "Q": (8, 3, ["...###", "..#...#", ".#.....#", ".#.....#", ".#.....#", ".#.....#", "..#...#", "...###", ".....##"]),
    "R": (7, 3, [".####", ".#...#", ".#...#", ".####", ".#..#", ".#...#", ".#...#", ".#....#"]),
    "S": (6, 3, ["..####", ".#", ".#", "..###", ".....#", ".....#", ".....#", ".####"]),
    "T": (6, 3, [".#####", "...#", "...#", "...#", "...#", "...#", "...#", "...#"]),
    "U": (7, 3, [".#....#", ".#....#", ".#....#", ".#....#", ".#....#", ".#....#", ".#....#", "..####"]),
    "V": (6, 3, [".#...#", ".#...#", ".#...#", "..#.#", "..#.#", "..#.#", "...#", "...#"]),
    "W": (10, 3, [".#...#...#", ".#...#...#", ".#...#...#", "..#.#.#.#", "..#.#.#.#", "..#.#.#.#", "...#...#", "...#...#"]),
    "X": (6, 3, [".#...#", ".#...#", "..#.#", "...#", "...#", "..#.#", ".#...#", ".#...#"]),
    "Y": (6, 3, [".#...#", ".#...#", "..#.#", "...#", "...#", "...#", "...#", "...#"]),
    "Z": (6, 3, [".#####", ".....#", "....#", "...#", "..#", ".#", ".#", ".#####"]),
    "[": (4, 2, [".##", ".#", ".#", ".#", ".#", ".#", ".#", ".#", ".#", ".##"]),
    "\\": (4, 2, [".#", ".#", ".#", "..#", "..#", "..#", "..#", "...#", "...#", "...#"]),
    "]": (4, 2, [".##", "..#", "..#", "..#", "..#", "..#", "..#", "..#", "..#", ".##"]),
    "^": (8, 3, ["....#", "...#.#", "..#...#", ".#.....#"]),
    "_": (6, 12, ["######"]),
    "`": (6, 3, ["..#", "...#"]),
    "a": (6, 5, [".####", ".....#", "..####", ".#...#", ".#..##", "..##.#"]),
    "b": (6, 3, [".#", ".#", ".#.##", ".##..#", ".#...#", ".#...#", ".##..#", ".#.##"]),
    "c": (5, 5, ["..###", ".#", ".#", ".#", ".#", "..###"]),
    "d": (6, 3, [".....#", ".....#", "..##.#", ".#..##", ".#...#", ".#...#", ".#..##", "..##.#"]),
    "e": (6, 5, ["..###", ".#...#", ".#####", ".#", ".#", "..####"]),
    "f": (4, 3, ["..##", "..#", ".###", "..#", "..#", "..#", "..#", "..#"]),
    "g": (6, 5, ["..##.#", ".#..##", ".#...#", ".#...#", ".#..##", "..##.#", ".....#", "..###"]),
    "h": (6, 3, [".#", ".#", ".#.##", ".##..#", ".#...#", ".#...#", ".#...#", ".#...#"]),
    "i": (2, 3, [".#", "", ".#", ".#", ".#", ".#", ".#", ".#"]),
    "j": (3, 3, ["..#", "", "..#", "..#", "..#", "..#", "..#", "..#", "..#", ".#"]),
    "k": (5, 3, [".#", ".#", ".#..#", ".#.#", ".##", ".#.#", ".#..#", ".#..#"]),
    "l": (2, 3, [".#", ".#", ".#", ".#", ".#", ".#", ".#", ".#"]),
    "m": (8, 5, [".#.##.##", ".##.##.#", ".#..#..#", ".#..#..#", ".#..#..#", ".#..#..#"]),
    "n": (6, 5, [".#.##", ".##..#", ".#...#", ".#...#", ".#...#", ".#...#"]),
    "o": (6, 5, ["..###", ".#...#", ".#...#", ".#...#", ".#...#", "..###"]),
    "p": (6, 5, [".#.##", ".##..#", ".#...#", ".#...#", ".##..#", ".#.##", ".#", ".#"]),
    "q": (6, 5, ["..##.#", ".#..##", ".#...#", ".#...#", ".#..##", "..##.#", ".....#", ".....#"]),
    "r": (4, 5, [".#.#", ".##", ".#", ".#", ".#", ".#"]),
    "s": (5, 5, ["..###", ".#", "..##", "....#", "....#", ".###"]),
    "t": (4, 4, [".#", ".###", ".#", ".#", ".#", ".#", "..##"]),
    "u": (6, 5, [".#...#", ".#...#", ".#...#", ".#...#", ".#..##", "..##.#"]),
    "v": (6, 5, [".#...#", ".#...#", "..#.#", "..#.#", "...#", "...#"]),
    "w": (8, 5, [".#.....#", ".#..#..#", "..#.#.#", "..#.#.#", "...#.#", "...#.#"]),
    "x": (6, 5, [".#...#", "..#.#", "...#", "...#", "..#.#", ".#...#"]),
    "y": (6, 5, [".#...#", ".#...#", "..#.#", "..#.#", "...#", "...#", "..#", ".#"]),
    "z": (5, 5, [".####", "....#", "...#", "..#", ".#", ".####"]),
    "{": (5, 2, ["...##", "..#", "..#", "..#", ".#", "..#", "..#", "..#", "..#", "...##"]),
    "|": (4, 2, ["..#"] * 11),
    "}": (5, 2, [".##", "...#", "...#", "...#", "....#", "...#", "...#", "...#", "...#", ".##"]),
    "~": (8, 6, ["..##...#", ".#..###"]),
}

# The Latin-1 letters with accents: the base letter (an i without its dot), and the mark centred over its ink, on rows 1 and
# 2 over a capital and rows 2 and 3 over a small letter (the cedilla under the baseline).
MARKS = {
    "grave": ["#.", ".#"], "acute": [".#", "#."], "circumflex": [".#.", "#.#"], "tilde": [".#.#", "#.#."],
    "diaeresis": ["", "#.#"], "ring": [".#.", "#.#", ".#."], "cedilla": [".#", "##"],
}
COMPOSED = [
    ("grave", "AEIOU", "ÀÈÌÒÙ"), ("acute", "AEIOUY", "ÁÉÍÓÚÝ"), ("circumflex", "AEIOU", "ÂÊÎÔÛ"), ("tilde", "ANO", "ÃÑÕ"),
    ("diaeresis", "AEIOU", "ÄËÏÖÜ"), ("ring", "A", "Å"), ("cedilla", "C", "Ç"),
    ("grave", "aeiou", "àèìòù"), ("acute", "aeiouy", "áéíóúý"), ("circumflex", "aeiou", "âêîôû"), ("tilde", "ano", "ãñõ"),
    ("diaeresis", "aeiouy", "äëïöüÿ"), ("ring", "a", "å"), ("cedilla", "c", "ç"),
]


# The bold, which 2000 drew its title bars, the Start button and default buttons in. Its advances are Tahoma Bold's at 11
# pixels to the em, taken as numbers as the regular's are, for the printable ASCII from "!" to "~".
BOLD_ADV = dict(zip("".join(chr(c) for c in range(33, 127)),
                    [3, 6, 9, 7, 13, 9, 3, 5, 5, 7, 9, 3, 5, 3, 6] + [7] * 10 + [3, 3, 9, 9, 9, 6, 10] +
                    [8, 7, 7, 8, 6, 6, 8, 8, 5, 6, 7, 6, 10, 7, 8, 7, 8, 8, 7, 7, 8, 7, 11, 7, 7, 7] + [5, 6, 5, 9, 7, 6] +
                    [7, 7, 6, 7, 7, 4, 7, 7, 3, 4, 7, 3, 11, 7, 7, 7, 7, 5, 6, 5, 7, 7, 9, 7, 7, 6] + [7, 7, 7, 9]))
# Most bold glyphs are the regular's struck twice, a pixel apart, which makes every stem two pixels wide and the advance one
# more, as Tahoma Bold's mostly is. Where its advance is not one more than the regular's, these are drawn by hand:
BOLD = {
    "!": (3, 3, [".##", ".##", ".##", ".##", ".##", ".##", "", ".##"]),
    '"': (6, 3, [".##.##", ".##.##"]),
    ",": (3, 9, [".##", ".##", ".#"]),
    ".": (3, 9, [".##", ".##"]),
    ":": (3, 5, [".##", ".##", "", "", ".##", ".##"]),
    ";": (3, 5, [".##", ".##", "", "", ".##", ".##", ".#"]),
    "C": (7, 3, ["..####", ".##..##", ".##", ".##", ".##", ".##", ".##..##", "..####"]),
    "E": (6, 3, [".#####", ".##", ".##", ".####", ".##", ".##", ".##", ".#####"]),
    "F": (6, 3, [".#####", ".##", ".##", ".####", ".##", ".##", ".##", ".##"]),
    "M": (10, 3, [".###...###", ".###...###", ".##.#.#.##", ".##.#.#.##", ".##..#..##", ".##..#..##", ".##.....##", ".##.....##"]),
    "N": (7, 3, [".##..##", ".###.##", ".###.##", ".###.##", ".##.###", ".##.###", ".##.###", ".##..##"]),
    "O": (8, 3, ["..#####", ".##...##", ".##...##", ".##...##", ".##...##", ".##...##", ".##...##", "..#####"]),
    "Q": (8, 3, ["..#####", ".##...##", ".##...##", ".##...##", ".##...##", ".##...##", ".##...##", "..#####", ".....###"]),
    "f": (4, 3, ["..##", ".##", ".###", ".##", ".##", ".##", ".##", ".##"]),
    "k": (7, 3, [".##", ".##", ".##..##", ".##.##", ".####", ".##.##", ".##..##", ".##..##"]),
    "m": (11, 5, [".##.###.###", ".###.###.##", ".##..##..##", ".##..##..##", ".##..##..##", ".##..##..##"]),
}
# drawn as the regular, at Tahoma Bold's advance: the at sign and the grave accent, as wide in bold
BOLD_AS_REGULAR = "@`"
# struck twice and set in the middle of a wider advance
BOLD_CENTRED = "%&/\\{|}"

# The Latin-1 letters with accents: the base letter (an i without its dot), and the mark centred over its ink, on rows 1 and
# 2 over a capital and rows 2 and 3 over a small letter (the cedilla under the baseline). The bold's grave and acute are
# struck twice too.
MARKS = {
    "grave": ["#.", ".#"], "acute": [".#", "#."], "circumflex": [".#.", "#.#"], "tilde": [".#.#", "#.#."],
    "diaeresis": ["", "#.#"], "ring": [".#.", "#.#", ".#."], "cedilla": [".#", "##"],
}
BOLD_MARKS = dict(MARKS, grave=["##.", ".##"], acute=[".##", "##."])
COMPOSED = [
    ("grave", "AEIOU", "ÀÈÌÒÙ"), ("acute", "AEIOUY", "ÁÉÍÓÚÝ"), ("circumflex", "AEIOU", "ÂÊÎÔÛ"), ("tilde", "ANO", "ÃÑÕ"),
    ("diaeresis", "AEIOU", "ÄËÏÖÜ"), ("ring", "A", "Å"), ("cedilla", "C", "Ç"),
    ("grave", "aeiou", "àèìòù"), ("acute", "aeiouy", "áéíóúý"), ("circumflex", "aeiou", "âêîôû"), ("tilde", "ano", "ãñõ"),
    ("diaeresis", "aeiouy", "äëïöüÿ"), ("ring", "a", "å"), ("cedilla", "c", "ç"),
]


def pixels(top, rows):
    return frozenset((x, top + r) for r, line in enumerate(rows) for x, c in enumerate(line) if c == "#")


def checked(ch, adv, px, mark=False):
    # the ink inside the advance, column 0 left as the gap (the underscore alone runs the whole advance, and a mark over a
    # narrow letter, as ì's and î's, may reach into the gaps on either side above the x-height)
    assert max(x for x, y in px) < adv + (1 if mark else 0), ch
    assert ch == "_" or min(x for x, y in px) >= (0 if mark else 1), ch
    return adv, px


def glyphs(bold=False):
    out = {}
    for ch, (adv, top, rows) in GLYPHS.items():
        px = pixels(top, rows)
        if bold:
            if ch in BOLD:
                adv, top, rows = BOLD[ch]
                px = pixels(top, rows)
            elif ch in BOLD_AS_REGULAR:
                adv = BOLD_ADV[ch]
            else:
                shift = (BOLD_ADV[ch] - adv - 1) // 2 if ch in BOLD_CENTRED else 0
                assert ch in BOLD_CENTRED or BOLD_ADV[ch] == adv + 1, ch
                px = frozenset((x + shift + d, y) for x, y in px for d in (0, 1))
                adv = BOLD_ADV[ch]
            assert adv == BOLD_ADV[ch], ch
        out[ch] = checked(ch, adv, px)
    out[" "] = (3, frozenset())
    out["\xa0"] = out[" "]
    marks = BOLD_MARKS if bold else MARKS
    for mark, bases, made in COMPOSED:
        for b, ch in zip(bases, made):
            adv, px = out[b]
            if b == "i":
                px = frozenset(p for p in px if p[1] >= 5)   # dotless
            xs = [x for x, y in px]
            rows = marks[mark]
            w = max(len(r) for r in rows)
            left = (min(xs) + max(xs) + 1 - w) // 2
            if mark == "cedilla":
                top = 11
            elif b.isupper():
                top = 3 - len(rows) - (0 if mark == "ring" else 1)
            else:
                top = 5 - len(rows) - 1
            out[ch] = checked(ch, adv, px | frozenset((left + x, top + y) for x, y in pixels(0, rows)), mark=True)
    return out


def main():
    faces = []
    for bold, path in ((False, OUT), (True, OUT_BOLD)):
        gl = glyphs(bold)
        faces.append((path, gl, pixelfont.font_bytes(gl, "Byopop Sans 2000", "Drawn by the byo-pop project.", 8, licence="cc0", bold=bold)))
    if "--check" in sys.argv:
        ok = True
        for path, gl, data in faces:
            same = os.path.exists(path) and open(path, "rb").read() == data
            ok = ok and same
            print("pixelfont2000: %s, %d glyphs, %d bytes, %s" % (os.path.basename(path), len(gl), len(data),
                                                                 "the file is up to date" if same else "THE FILE DIFFERS"))
        sys.exit(0 if ok else 1)
    if "--show" in sys.argv:
        text = sys.argv[sys.argv.index("--show") + 1]
        gl = faces[1 if "--bold" in sys.argv else 0][1]
        for y in range(13):
            line = ""
            for ch in text:
                adv, px = gl.get(ch, (3, frozenset()))
                line += "".join("#" if (x, y) in px else "." for x in range(adv))
            print("%2d %s" % (y, line))
        return
    for path, gl, data in faces:
        with open(path, "wb") as f:
            f.write(data)
        print("pixelfont2000: wrote %s, %d glyphs, %d bytes" % (os.path.relpath(path), len(gl), len(data)))


if __name__ == "__main__":
    main()
