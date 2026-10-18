#!/usr/bin/env python3
"""Generate example mods with original (procedurally drawn) art.

    python games/bejeweled/tools/make_example_mods.py [--out <mods folder, default games/bejeweled/mods>]

Creates, in games/bejeweled/mods/:
  example-crystal-gems/   HD (@2x) replacement for images/gemsheet6 with new gem art
  example-voice-lines/    text-only mod: changes on-screen messages via string overrides
  example-relaxed/        gameplay mod: tunables (cheaper hints) -> separate high-score table

The gem sheet layout matches the game's: 15 columns x 14 rows of 52x52 cells; gem g uses row 2g
(spin animation) and row 2g+1 (glint animation). At @2x each cell is 104x104.
"""
import argparse
import json
import math
import os

from PIL import Image, ImageDraw, ImageFilter

CELL = 52
SCALE = 2          # @2x output
SS = 4             # supersampling for anti-aliasing
FRAMES = 15

# (name, base colour, highlight colour, shape)
GEMS = [
    ("yellow", (232, 196, 20), (255, 248, 170), "rhombus"),
    ("white", (196, 204, 214), (255, 255, 255), "round"),
    ("blue", (24, 132, 230), (170, 230, 255), "brilliant"),
    ("red", (214, 28, 40), (255, 170, 160), "square"),
    ("purple", (186, 48, 210), (250, 180, 255), "triangle"),
    ("orange", (236, 120, 16), (255, 214, 140), "hexagon"),
    ("green", (30, 176, 64), (180, 255, 170), "octagon"),
]


def shape_points(shape, r):
    """Outline of a gem centred at (0,0) with radius r."""
    if shape == "rhombus":
        return [(0, -r), (r * 0.8, 0), (0, r), (-r * 0.8, 0)]
    if shape == "round":
        return [(r * math.cos(a), r * math.sin(a)) for a in [i * math.pi / 8 + math.pi / 16 for i in range(16)]]
    if shape == "brilliant":
        return [(-r, -r * 0.35), (-r * 0.55, -r * 0.8), (r * 0.55, -r * 0.8), (r, -r * 0.35), (0, r * 0.9)]
    if shape == "square":
        k = r * 0.82
        c = r * 0.18
        return [(-k + c, -k), (k - c, -k), (k, -k + c), (k, k - c), (k - c, k), (-k + c, k), (-k, k - c), (-k, -k + c)]
    if shape == "triangle":
        return [(0, -r), (r * 0.92, r * 0.72), (-r * 0.92, r * 0.72)]
    if shape == "hexagon":
        return [(r * math.cos(a), r * math.sin(a)) for a in [i * math.pi / 3 for i in range(6)]]
    if shape == "octagon":
        return [(r * 0.95 * math.cos(a), r * math.sin(a)) for a in [i * math.pi / 4 + math.pi / 8 for i in range(8)]]
    raise ValueError(shape)


def lerp(a, b, t):
    return tuple(int(a[i] + (b[i] - a[i]) * t) for i in range(3))


def draw_gem(base, hi, shape, squash=1.0, glint=None):
    """Draw one cell at full output resolution (CELL*SCALE) with alpha."""
    size = CELL * SCALE * SS
    img = Image.new("RGBA", (size, size), (0, 0, 0, 0))
    d = ImageDraw.Draw(img)
    cx = cy = size / 2
    r = size * 0.42
    pts = [(cx + x * squash, cy + y) for x, y in shape_points(shape, r)]

    # drop shadow
    shadow = Image.new("RGBA", (size, size), (0, 0, 0, 0))
    ImageDraw.Draw(shadow).polygon([(x + size * 0.03, y + size * 0.04) for x, y in pts], fill=(0, 0, 0, 110))
    shadow = shadow.filter(ImageFilter.GaussianBlur(size * 0.02))
    img.alpha_composite(shadow)

    # body: facets as triangles from an off-centre table point, shaded by facet normal
    table = (cx - r * 0.12 * squash, cy - r * 0.18)
    dark = lerp(base, (0, 0, 0), 0.45)
    for i in range(len(pts)):
        p0, p1 = pts[i], pts[(i + 1) % len(pts)]
        mx, my = (p0[0] + p1[0]) / 2 - cx, (p0[1] + p1[1]) / 2 - cy
        light = 0.5 - 0.5 * (mx * 0.6 + my * 0.8) / (math.hypot(mx, my) + 1e-6)  # light from top-left
        col = lerp(dark, hi, 0.15 + 0.7 * light)
        d.polygon([table, p0, p1], fill=col + (255,))
    # inner table facet
    inner = [(table[0] + (x - cx) * 0.45, table[1] + (y - cy) * 0.45) for x, y in pts]
    d.polygon(inner, fill=lerp(base, hi, 0.55) + (255,))
    # rim
    d.line(pts + [pts[0]], fill=lerp(base, (0, 0, 0), 0.6) + (255,), width=int(size * 0.015))

    # specular sparkle
    sx, sy = cx - r * 0.38 * squash, cy - r * 0.45
    sr = size * 0.045
    d.ellipse([sx - sr, sy - sr, sx + sr, sy + sr], fill=(255, 255, 255, 230))

    if glint is not None:
        # a soft diagonal band of light sweeping across the gem
        band = Image.new("RGBA", (size, size), (0, 0, 0, 0))
        bd = ImageDraw.Draw(band)
        x = -size * 0.4 + glint * size * 1.8
        bd.polygon([(x, 0), (x + size * 0.18, 0), (x - size * 0.6 + size * 0.18, size), (x - size * 0.6, size)],
                   fill=(255, 255, 255, 120))
        band = band.filter(ImageFilter.GaussianBlur(size * 0.03))
        mask = Image.new("L", (size, size), 0)
        ImageDraw.Draw(mask).polygon(pts, fill=255)
        clipped = Image.new("RGBA", (size, size), (0, 0, 0, 0))
        clipped.paste(band, (0, 0), mask)
        img.alpha_composite(clipped)

    return img.resize((CELL * SCALE, CELL * SCALE), Image.LANCZOS)


def make_gemsheet(path):
    w, h = FRAMES * CELL * SCALE, len(GEMS) * 2 * CELL * SCALE
    sheet = Image.new("RGBA", (w, h), (0, 0, 0, 0))
    for g, (name, base, hi, shape) in enumerate(GEMS):
        for f in range(FRAMES):
            squash = max(0.12, abs(math.cos(math.pi * f / FRAMES)))
            spin = draw_gem(base, hi, shape, squash=squash)
            sheet.alpha_composite(spin, (f * CELL * SCALE, (2 * g) * CELL * SCALE))
            glint = draw_gem(base, hi, shape, glint=f / (FRAMES - 1))
            sheet.alpha_composite(glint, (f * CELL * SCALE, (2 * g + 1) * CELL * SCALE))
        print(f"  gem {g} ({name}) done")
    os.makedirs(os.path.dirname(path), exist_ok=True)
    sheet.save(path, optimize=True)


def write_json(path, data):
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, "w", encoding="utf-8") as f:
        json.dump(data, f, indent=2)
        f.write("\n")


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--out", default=os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "mods"))
    args = ap.parse_args()
    out = os.path.abspath(args.out)

    gems = os.path.join(out, "example-crystal-gems")
    write_json(os.path.join(gems, "mod.json"), {
        "id": "example-crystal-gems",
        "name": "Crystal Gems HD",
        "version": "1.0",
        "author": "byo-pop examples",
        "description": "Replaces the gem sprites with new double-resolution art (images/gemsheet6@2x.png).",
    })
    print("drawing gem sheet...")
    make_gemsheet(os.path.join(gems, "images", "gemsheet6@2x.png"))

    voice = os.path.join(out, "example-voice-lines")
    write_json(os.path.join(voice, "mod.json"), {
        "id": "example-voice-lines",
        "name": "New Messages",
        "version": "1.0",
        "author": "byo-pop examples",
        "description": "Changes a few on-screen messages using string overrides.",
        "strings": {
            "EXCELLENT": "BRILLIANT!",
            "NO_MOVES": "OUT OF MOVES!",
            "GO": "LET'S GO!",
            "TIME_UP": "TIME'S UP!",
        },
    })

    relaxed = os.path.join(out, "example-relaxed")
    write_json(os.path.join(relaxed, "mod.json"), {
        "id": "example-relaxed",
        "name": "Relaxed Hints",
        "version": "1.0",
        "author": "byo-pop examples",
        "description": "Hints cost 10 points instead of 50 (per level multiplier). Uses its own high-score table.",
        "tunables": {"scoring": {"hintPenalty": 10}},
    })
    print(f"example mods written to {out}")


if __name__ == "__main__":
    main()
