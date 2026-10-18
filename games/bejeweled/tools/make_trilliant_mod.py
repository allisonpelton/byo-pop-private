#!/usr/bin/env python3
"""Generate the "Mines Trilliant" mod: replaces the orange gem with a trilliant-cut gem whose table
(its largest facet) is the rounded triangle of the Colorado School of Mines legacy triangle mark.

    python games/bejeweled/tools/make_trilliant_mod.py [--out <mods folder, default games/bejeweled/mods>]
                                                       [--preview <png>]

Shape: the mark is an equilateral Reuleaux triangle pointing up. Measured from the official artwork
(brand.mines.edu, 1082 px wide), normalized to width 1 with the apex at the origin and y down:
  * the left side is an arc centred on the bottom-right corner (0.4998, 0.8600), radius 0.9958,
    the right side mirrors it (centre (-0.4998, 0.8600));
  * the bottom is an arc centred just below the apex (0, 0.0162), radius 0.9870;
  * the two bottom corners are cut by short vertical flats at x = +-0.5 from y 0.8447 to 0.8678;
  * the apex is a sharp 120 degree point.
(least-squares circle fits; mean error about 1 px on the 1082 px source)

Only geometry is reproduced; no logo artwork is copied. The output is an @2x image patch for the two
orange-gem rows of images/gemsheet6 (row 10 = spin animation, row 11 = glint animation, 15 frames each).
"""
import argparse
import json
import math
import os

from PIL import Image, ImageDraw, ImageFilter

CELL = 52          # game cell size (1x)
SCALE = 2          # patch pixel density (@2x)
SS = 4             # supersampling factor for anti-aliasing
FRAMES = 15
ORANGE_ROW = 5 * 2  # gem 5 (orange) uses rows 10 and 11

# --- Mines legacy-triangle outline (normalized) ---------------------------------------------------
SIDE_CENTER_X = 0.4998
SIDE_CENTER_Y = 0.8600
SIDE_RADIUS = 0.9958
BOTTOM_CENTER_Y = 0.0162
BOTTOM_RADIUS = 0.9870
FLAT_TOP = 0.8447
FLAT_BOTTOM = 0.8678
CENTER = (0.0, 0.5714)  # centroid of the three Reuleaux vertices; facets scale about this point


def arc(cx, cy, r, p0, p1, n):
    """Points along the circle (cx,cy,r) from p0 to p1 (both on the circle), shortest direction."""
    a0 = math.atan2(p0[1] - cy, p0[0] - cx)
    a1 = math.atan2(p1[1] - cy, p1[0] - cx)
    d = (a1 - a0 + math.pi) % (2 * math.pi) - math.pi
    return [(cx + r * math.cos(a0 + d * i / n), cy + r * math.sin(a0 + d * i / n)) for i in range(n + 1)]


def x_on_side(y, sign):
    """x of the side arc at height y; sign=+1 right side (centre on the left), -1 left side."""
    return sign * (math.sqrt(SIDE_RADIUS ** 2 - (y - SIDE_CENTER_Y) ** 2) - SIDE_CENTER_X)


def mines_sides(n=48):
    """The outline as three sides (right, bottom, left), each a list of points, corners included.
    Corner flats are folded into the bottom side so each side starts/ends on a corner."""
    apex = (0.0, 0.0)
    r_top = (x_on_side(FLAT_TOP, +1), FLAT_TOP)
    l_top = (x_on_side(FLAT_TOP, -1), FLAT_TOP)
    right = arc(-SIDE_CENTER_X, SIDE_CENTER_Y, SIDE_RADIUS, apex, r_top, n)
    left = arc(SIDE_CENTER_X, SIDE_CENTER_Y, SIDE_RADIUS, l_top, apex, n)
    by = BOTTOM_CENTER_Y + math.sqrt(BOTTOM_RADIUS ** 2 - 0.25)
    bottom_arc = arc(0.0, BOTTOM_CENTER_Y, BOTTOM_RADIUS, (0.5, by), (-0.5, by), n)
    bottom = [r_top, (0.5, FLAT_BOTTOM)] + bottom_arc[1:-1] + [(-0.5, FLAT_BOTTOM), l_top]
    return [right, bottom, left]


def resample(poly, n):
    """Resample a polyline to n+1 points evenly spaced by arc length."""
    seg = [math.dist(poly[i], poly[i + 1]) for i in range(len(poly) - 1)]
    total = sum(seg)
    out, acc, j = [], 0.0, 0
    for k in range(n + 1):
        target = total * k / n
        while j < len(seg) - 1 and acc + seg[j] < target:
            acc += seg[j]
            j += 1
        t = 0.0 if seg[j] == 0 else min(1.0, (target - acc) / seg[j])
        a, b = poly[j], poly[j + 1]
        out.append((a[0] + (b[0] - a[0]) * t, a[1] + (b[1] - a[1]) * t))
    return out


def scaled(points, s):
    return [(CENTER[0] + (x - CENTER[0]) * s, CENTER[1] + (y - CENTER[1]) * s) for x, y in points]


# --- facets ----------------------------------------------------------------------------------------
TABLE_SCALE = 0.60   # table size relative to the girdle (it stays the largest facet)
STEPS = 60           # samples per side


def build_facets():
    """Returns (girdle polygon, table polygon, crown facets [(polygon, tilt_degrees)]).

    Each of the three sides between the table edge t(u) and the girdle edge g(u) is cut into five
    crown facets (u = 0 and 1 are the corners):
        upper-girdle  t(0)   g(0..1/3)
        star          t(0..1/2)  g(1/3)
        bezel         t(1/2) g(1/3..2/3)
        star          t(1/2..1)  g(2/3)
        upper-girdle  t(1)   g(2/3..1)
    Curved edges keep their arc samples so the outline stays a Reuleaux triangle.
    """
    girdle_sides = [resample(s, STEPS) for s in mines_sides()]
    table_sides = [scaled(s, TABLE_SCALE) for s in girdle_sides]
    third, half = STEPS // 3, STEPS // 2
    crown = []
    for g, t in zip(girdle_sides, table_sides):
        crown.append((g[0:third + 1] + [t[0]], 42))
        crown.append((t[0:half + 1] + [g[third]], 30))
        crown.append((g[third:2 * third + 1] + [t[half]], 36))
        crown.append((t[half:STEPS + 1] + [g[2 * third]], 30))
        crown.append((g[2 * third:STEPS + 1] + [t[STEPS]], 42))
    girdle = [p for s in girdle_sides for p in s[:-1]]
    table = [p for s in table_sides for p in s[:-1]]
    return girdle, table, crown


GIRDLE, TABLE, CROWN = build_facets()

# --- shading -------------------------------------------------------------------------------------
PALETTE = [  # intensity -> colour (orange gem)
    (0.00, (70, 20, 0)),
    (0.30, (160, 58, 0)),
    (0.55, (236, 112, 10)),
    (0.78, (255, 172, 52)),
    (0.92, (255, 222, 140)),
    (1.00, (255, 250, 230)),
]


def shade(i):
    i = max(0.0, min(1.0, i))
    for (a, ca), (b, cb) in zip(PALETTE, PALETTE[1:]):
        if i <= b:
            t = (i - a) / (b - a)
            return tuple(int(ca[k] + (cb[k] - ca[k]) * t) for k in range(3))
    return PALETTE[-1][1]


def normalize(v):
    n = math.sqrt(sum(c * c for c in v))
    return tuple(c / n for c in v)


def facet_normal(poly, tilt_deg):
    cx = sum(p[0] for p in poly) / len(poly) - CENTER[0]
    cy = sum(p[1] for p in poly) / len(poly) - CENTER[1]
    d = math.hypot(cx, cy) or 1.0
    t = math.radians(tilt_deg)
    return (cx / d * math.sin(t), cy / d * math.sin(t), math.cos(t))


def rotate_y(n, ang):
    return (n[0] * math.cos(ang) + n[2] * math.sin(ang), n[1], -n[0] * math.sin(ang) + n[2] * math.cos(ang))


def intensity(n, light):
    view = (0.0, 0.0, 1.0)
    half = normalize((light[0] + view[0], light[1] + view[1], light[2] + view[2]))
    diff = max(0.0, sum(a * b for a, b in zip(n, light)))
    spec = max(0.0, sum(a * b for a, b in zip(n, half))) ** 28
    return 0.18 + 0.62 * diff + 0.55 * spec


BASE_LIGHT = normalize((-0.45, -0.65, 0.62))


def light_at(azimuth_deg):
    a = math.radians(azimuth_deg)
    x, y, z = BASE_LIGHT
    return normalize((x * math.cos(a) - y * math.sin(a), x * math.sin(a) + y * math.cos(a), z))


# --- rendering -----------------------------------------------------------------------------------
GEM_WIDTH = 46 / CELL   # fraction of the cell the girdle spans (the original gems span ~44-46 px)


def render_cell(spin=0.0, light=BASE_LIGHT, glint=None):
    """One @2x cell. spin: rotation about the vertical axis in radians (0..pi)."""
    size = CELL * SCALE * SS
    img = Image.new("RGBA", (size, size), (0, 0, 0, 0))
    squash = math.cos(spin)
    width = max(0.10, abs(squash))
    mirror = -1 if squash < 0 else 1
    view_spin = spin if squash >= 0 else math.pi - spin
    gem_px = size * GEM_WIDTH
    ox = size / 2
    oy = (size - gem_px * (BOTTOM_CENTER_Y + BOTTOM_RADIUS)) / 2

    def to_px(p):
        return (ox + p[0] * gem_px * width * mirror, oy + p[1] * gem_px)

    # soft drop shadow
    shadow = Image.new("RGBA", (size, size), (0, 0, 0, 0))
    off = size * 0.035
    ImageDraw.Draw(shadow).polygon([(x + off * 0.6, y + off) for x, y in map(to_px, GIRDLE)], fill=(0, 0, 0, 120))
    img.alpha_composite(shadow.filter(ImageFilter.GaussianBlur(size * 0.018)))

    d = ImageDraw.Draw(img)
    edge_w = max(1, int(size * 0.006))

    def n_view(n):
        n = (n[0] * mirror, n[1], n[2])
        return rotate_y(n, view_spin * mirror)

    # crown facets
    for poly, tilt in CROWN:
        n = n_view(facet_normal(poly, tilt))
        col = shade(intensity(n, light))
        d.polygon([to_px(p) for p in poly], fill=col + (255,))

    # table: flat facet with depth cues (pavilion reflections seen through it)
    tn = n_view((0.0, 0.0, 1.0))
    ti = intensity(tn, light)
    table_px = [to_px(p) for p in TABLE]
    d.polygon(table_px, fill=shade(ti - 0.05) + (255,))
    inner = Image.new("RGBA", (size, size), (0, 0, 0, 0))
    idraw = ImageDraw.Draw(inner)
    culet = to_px(CENTER)
    corners = [to_px(scaled([p], TABLE_SCALE)[0]) for p in ((0.0, 0.0), (0.5, 0.856), (-0.5, 0.856))]
    for i, c in enumerate(corners):
        nxt = corners[(i + 1) % 3]
        tri = [culet, c, nxt]
        k = (i + int(round(view_spin * 3 / math.pi))) % 3
        idraw.polygon(tri, fill=shade(ti - 0.18 + 0.12 * k) + (70,))
    for c in corners:
        idraw.line([culet, c], fill=shade(ti + 0.25) + (110,), width=edge_w)
    mask = Image.new("L", (size, size), 0)
    ImageDraw.Draw(mask).polygon(table_px, fill=255)
    clipped = Image.new("RGBA", (size, size), (0, 0, 0, 0))
    clipped.paste(inner, (0, 0), mask)
    img.alpha_composite(clipped)

    # facet edges catch light; table outline crisp so the Mines shape reads clearly
    for poly, _ in CROWN:
        d.line([to_px(p) for p in poly] + [to_px(poly[0])], fill=shade(0.86) + (150,), width=edge_w)
    d.line(table_px + [table_px[0]], fill=shade(0.97) + (230,), width=edge_w * 2)
    girdle_px = [to_px(p) for p in GIRDLE]
    d.line(girdle_px + [girdle_px[0]], fill=(80, 26, 0, 255), width=edge_w * 2)

    if glint is not None:
        # a soft band of light sweeping across the stone, clipped to the girdle
        band = Image.new("RGBA", (size, size), (0, 0, 0, 0))
        x = -size * 0.45 + glint * size * 1.9
        ImageDraw.Draw(band).polygon(
            [(x, 0), (x + size * 0.16, 0), (x - size * 0.55 + size * 0.16, size), (x - size * 0.55, size)],
            fill=(255, 255, 255, 110))
        band = band.filter(ImageFilter.GaussianBlur(size * 0.025))
        gmask = Image.new("L", (size, size), 0)
        ImageDraw.Draw(gmask).polygon(girdle_px, fill=255)
        clip = Image.new("RGBA", (size, size), (0, 0, 0, 0))
        clip.paste(band, (0, 0), gmask)
        img.alpha_composite(clip)
        # twinkle on the apex region at the middle of the sweep
        tw = max(0.0, 1.0 - abs(glint - 0.5) * 4)
        if tw > 0:
            sx, sy = to_px((0.0, 0.16))
            r = size * 0.09 * tw
            star = Image.new("RGBA", (size, size), (0, 0, 0, 0))
            sd = ImageDraw.Draw(star)
            sd.polygon([(sx, sy - r), (sx + r * 0.16, sy), (sx, sy + r), (sx - r * 0.16, sy)], fill=(255, 255, 255, 235))
            sd.polygon([(sx - r, sy), (sx, sy - r * 0.16), (sx + r, sy), (sx, sy + r * 0.16)], fill=(255, 255, 255, 235))
            img.alpha_composite(star.filter(ImageFilter.GaussianBlur(size * 0.004)))

    return img.resize((CELL * SCALE, CELL * SCALE), Image.LANCZOS)


def make_patch(path):
    w, h = FRAMES * CELL * SCALE, 2 * CELL * SCALE
    sheet = Image.new("RGBA", (w, h), (0, 0, 0, 0))
    for f in range(FRAMES):
        spin = math.pi * f / FRAMES
        sheet.alpha_composite(render_cell(spin=spin, light=light_at(0)), (f * CELL * SCALE, 0))
        sheet.alpha_composite(render_cell(light=light_at(-40 + 80 * f / (FRAMES - 1)), glint=f / (FRAMES - 1)),
                              (f * CELL * SCALE, CELL * SCALE))
    os.makedirs(os.path.dirname(path), exist_ok=True)
    sheet.save(path, optimize=True)
    return sheet


def main():
    here = os.path.dirname(os.path.abspath(__file__))
    ap = argparse.ArgumentParser()
    ap.add_argument("--out", default=os.path.join(here, "..", "mods"))
    ap.add_argument("--preview", help="also write a preview PNG (patch rows on a dark background, 1x and 2x)")
    args = ap.parse_args()
    mod = os.path.join(os.path.abspath(args.out), "mines-trilliant")
    patch_rel = "patches/orange-trilliant@2x.png"
    sheet = make_patch(os.path.join(mod, *patch_rel.split("/")))
    with open(os.path.join(mod, "mod.json"), "w", encoding="utf-8") as f:
        json.dump({
            "id": "mines-trilliant",
            "name": "Mines Trilliant",
            "version": "1.0",
            "author": "byo-pop",
            "description": "Replaces the orange gem with a trilliant cut whose table is the Colorado School of "
                           "Mines legacy triangle (a Reuleaux triangle). HD (@2x) art; the other gems are untouched.",
            "imagePatches": [
                {"target": "images/gemsheet6", "image": patch_rel, "x": 0, "y": ORANGE_ROW * CELL, "mode": "replace"}
            ],
        }, f, indent=2)
        f.write("\n")
    print(f"wrote {mod}")
    if args.preview:
        bg = Image.new("RGBA", sheet.size, (34, 30, 48, 255))
        bg.alpha_composite(sheet)
        bg.convert("RGB").save(args.preview)
        print(f"preview {args.preview}")


if __name__ == "__main__":
    main()
