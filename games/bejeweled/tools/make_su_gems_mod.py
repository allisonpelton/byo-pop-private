#!/usr/bin/env python3
"""Generate the "Steven Universe Gems" mod (fan art): the gems redrawn as gems from Steven Universe.

    python games/bejeweled/tools/make_su_gems_mod.py [--out <mods folder, default games/bejeweled/mods>]
                                                     [--colors 7|11] [--no-title] [--preview <png>] [--check]

games/bejeweled/docs/CODE_MODS_DESIGN.md 4.17. Each gem keeps its Bejeweled colour, so play reads the same:
0 yellow Yellow Diamond (a rhombus with a rhombus table), 1 white Pearl (a smooth egg), 2 blue Sapphire (round, a
triangle table), 3 red Ruby (round, a square table), 4 purple Amethyst (round, a hexagon table), 5 orange Jasper (a
kite of four facets), 6 green Peridot (a rounded triangle, point down), and for stage 8's colours 7 Rose Quartz
(round, a pentagon table), 8 Lapis Lazuli (a teardrop), 9 Bismuth (a square stepped in rainbow rings) and 10 Garnet
(round, an octagon table).

The drawings are static pictures in the show's style (the user, 2026-10-26: recognisably the show's gems, no
animation): flat colours, no glints, a rim of the gem's darkest colour, and the facets as flat wedges around a
table. Each wedge takes one of the gem's tones by the direction it faces, lit from the upper left. Everything is
new 2D drawing from polygons, after the show's designs; this script reads no file (no show images, no game data).
Every spin frame of a colour is the same picture, and its glow row is the same gem lit up.

Output: games/bejeweled/mods/su-gems (git-ignored, and the script is left out of the public snapshot: fan art of a
TV show's designs), a whole images/gemsheet6@4x.png (the game makes its 1x pixels by box downsampling) with 11
colours by default, so stage 8's extra colours (more than 7, board.gemColors) show these gems too; --colors 7 writes
only the seven. Unless --no-title, a @4x patch draws the same gems over the five of the title picture. --check prints
each gem's extent and fails when one leaves the cell's sampled area (x 2..50, y 0..48).
"""
import argparse
import math
import os

import numpy as np
from PIL import Image, ImageDraw

from gemsheet import (ANCHOR, CELL, FRAMES, SCALE, TITLE_BOXES, TITLE_COVER, TITLE_GLOW, TITLE_HAZE, TITLE_NOISE,
                      TITLE_RECT, TITLE_STONE, TITLE_TOP, downsample, hull2d, polygon_distance, rect_distance, save,
                      set_title_patch, smoothstep, to_image, write_mod_json, write_preview)

SS = 4                       # supersampling per output pixel
UNIT = 21.0                  # 1x pixels per unit: a round gem's outer radius
LIGHT = np.array([-0.6, -0.8])   # the light comes from the upper left (y down)
GLOW_LIFT = 0.3              # the glow row: every colour this far toward white (the rim half as far)


def rgb(s):
    return np.array([int(s[i:i + 2], 16) for i in (1, 3, 5)], dtype=np.float64) / 255.0


def mix(a, b, t):
    return a + (b - a) * t


def circle(r, cx=0.0, cy=0.0, n=192):
    return np.array([(cx + r * math.cos(2 * math.pi * k / n), cy + r * math.sin(2 * math.pi * k / n)) for k in range(n)])


def ellipse(rx, ry, cx=0.0, cy=0.0, n=192):
    return np.array([(cx + rx * math.cos(2 * math.pi * k / n), cy + ry * math.sin(2 * math.pi * k / n)) for k in range(n)])


def regular(n, r, phase):
    """n vertices on radius r, the first at `phase` degrees (y down: -90 is straight up)."""
    return np.array([(r * math.cos(math.radians(phase + 360.0 * k / n)), r * math.sin(math.radians(phase + 360.0 * k / n)))
                     for k in range(n)])


def rounded(core, r, arc=16):
    """The convex polygon `core` grown by r: its edges pushed out by r and its corners rounded (a Minkowski sum)."""
    core = np.asarray(core, dtype=np.float64)
    if r <= 0:
        return core
    c = core.mean(0)
    out = []
    n = len(core)
    for i in range(n):
        p0, p1, p2 = core[i - 1], core[i], core[(i + 1) % n]
        normals = []
        for a, b in ((p0, p1), (p1, p2)):
            nv = np.array([b[1] - a[1], a[0] - b[0]])
            nv /= np.linalg.norm(nv)
            if nv @ ((a + b) / 2 - c) < 0:
                nv = -nv
            normals.append(math.atan2(nv[1], nv[0]))
        a0, a1 = normals
        da = (a1 - a0 + math.pi) % (2 * math.pi) - math.pi     # the short way round a convex corner
        for k in range(arc + 1):
            a = a0 + da * k / arc
            out.append((p1[0] + r * math.cos(a), p1[1] + r * math.sin(a)))
    return np.array(out)


def teardrop(apex_y, cx, cy, r):
    """A teardrop, point up: the hull of the apex (cx, apex_y) and a circle."""
    return hull2d(np.vstack([circle(r, cx, cy), [[cx, apex_y]]]))


def tone_for(direction, tones):
    """The tone a wedge facing `direction` takes: the first lit straight on, the last turned away."""
    d = np.asarray(direction, dtype=np.float64)
    d = d / max(np.linalg.norm(d), 1e-9)
    lit = float(d @ (LIGHT / np.linalg.norm(LIGHT)))
    return tones[int(round((1 - lit) / 2 * (len(tones) - 1)))]


def wedges(centre, rays, table, tones, outer=None):
    """The facets between a table and the body's edge: wedge i runs from table edge i out between the rays from
    `centre` through rays[i] and rays[i + 1] (clipped to the body when drawn). table is a polygon with a vertex on
    each ray, or None for a point table at the centre; outer, a polygon with a vertex on each ray, ends the wedges
    there instead of at the body's edge."""
    centre = np.asarray(centre, dtype=np.float64)
    rays = np.asarray(rays, dtype=np.float64)
    inner = np.repeat(centre[None], len(rays), 0) if table is None else np.asarray(table, dtype=np.float64)
    out = []
    for i in range(len(rays)):
        j = (i + 1) % len(rays)
        far_i = centre + (rays[i] - centre) * 8 if outer is None else outer[i]
        far_j = centre + (rays[j] - centre) * 8 if outer is None else outer[j]
        mid = (rays[i] + rays[j]) / 2 - centre
        out.append((np.array([inner[i], inner[j], far_j, far_i]), tone_for(mid, tones)))
    return out


class Gem:
    """A drawing: the rim (outline polygon) in `rim`, the body polygon over it, then `parts`, each (polygon, colour)
    clipped to the body, in order."""

    def __init__(self, name, outline, body, rim, fill, parts, tint):
        self.name = name
        self.outline = np.asarray(outline, dtype=np.float64)
        self.body = np.asarray(body, dtype=np.float64)
        self.rim = rim
        self.fill = fill
        self.parts = parts
        self.tint = tint
        lo, hi = self.outline.min(0), self.outline.max(0)
        self.centre = (lo + hi) / 2           # of the bounding box, which lands on the cell's anchor
        self.extent = hi - lo


def round_gem(name, sides, phase, table_r, tones, table, rim, tint=None):
    """The show's round faceted gems: a rim, then wedges around a regular table."""
    t = regular(sides, table_r, phase)
    parts = wedges((0, 0), t, t, [rgb(c) for c in tones]) + [(t, rgb(table))]
    return Gem(name, circle(1.0), circle(0.9), rgb(rim), rgb(tones[1]), parts, rgb(tint or tones[1]))


def su_gems(colors):
    gems = []
    # 0 Yellow Diamond: a tall rhombus, a rhombus table, a dark gold rim
    body = np.array([(0, -0.98), (0.6, 0), (0, 0.98), (-0.6, 0)])
    table = body * 0.42
    tones = [rgb(c) for c in ("#FFF7C4", "#FFE34F", "#F4C21A", "#C8940B")]
    gems.append(Gem("Yellow Diamond", rounded(body, 0.09), body, rgb("#6E5200"), tones[1],
                    wedges((0, 0), table, table, tones) + [(table, rgb("#FFEB8A"))], tones[1]))
    # 1 Pearl: a smooth egg, white, a black rim, a pale pink crescent down its left side
    inner = ellipse(0.66, 0.88)
    gems.append(Gem("Pearl", ellipse(0.75, 0.97), inner, rgb("#16121A"), rgb("#F7DCEB"),
                    [(ellipse(0.54, 0.86, 0.13, -0.01), rgb("#FFFDF9"))], rgb("#FFF4FA")))
    # 2 Sapphire: round, a triangle table point up
    gems.append(round_gem("Sapphire", 3, -90, 0.6, ("#3F7BFF", "#2B58E2", "#1F3FBC", "#172E98"), "#62A0FF",
                          "#0A196B"))
    # 3 Ruby: round, a square table; the lit wedge goes orange
    gems.append(round_gem("Ruby", 4, 45, 0.6, ("#FFB067", "#FF5B3D", "#E1293F", "#AE132E"), "#FF8371", "#850A24"))
    # 4 Amethyst: round, a hexagon table pointed left and right
    gems.append(round_gem("Amethyst", 6, 0, 0.52, ("#C86EFF", "#A63FF2", "#8A22DB", "#5E0FA2"), "#DA8CFF",
                          "#3B0A63"))
    # 5 Jasper: a kite of four facets, lit orange, red, tan and brown, a near-black rim
    kite = np.array([(0, -0.98), (0.56, 0.12), (0, 0.98), (-0.56, 0.12)])
    tones = [rgb(c) for c in ("#FF9C4C", "#EE5530", "#C9966A", "#8C5937")]
    gems.append(Gem("Jasper", rounded(kite, 0.09), kite, rgb("#2A130A"), tones[1],
                    wedges((0, 0.12), kite, None, tones), rgb("#F0703A")))
    # 6 Peridot: a rounded triangle point down, bright green, a dark green rim, a pale yellow highlight
    core = np.array([(-0.7, -0.42), (0.7, -0.42), (0, 0.6)])
    gems.append(Gem("Peridot", rounded(core, 0.33), rounded(core, 0.22), rgb("#1B6E33"), rgb("#3DEB5C"),
                    [(rounded(core * 0.55 + [0, -0.06], 0.12), rgb("#5BF577")),
                     (ellipse(0.22, 0.1, 0.3, -0.36), rgb("#ECF8A6"))], rgb("#3DEB5C")))
    if colors > 7:
        # 7 Rose Quartz: round, a pentagon table point up
        gems.append(round_gem("Rose Quartz", 5, -90, 0.52, ("#FFDAEC", "#FFA4CD", "#FF86BB", "#F2689F"), "#FFB4D7",
                              "#C42A5E"))
        # 8 Lapis Lazuli: a teardrop point up, deep blue, a lighter teardrop table to the upper left
        gems.append(Gem("Lapis Lazuli", teardrop(-1.0, 0, 0.3, 0.7), teardrop(-0.86, 0, 0.3, 0.6), rgb("#071A5C"),
                        rgb("#1C55D6"), [(teardrop(-0.52, -0.1, 0.24, 0.34), rgb("#4C86FF"))], rgb("#2E6BF0")))
        # 9 Bismuth: a square in a mauve setting, stepped in rainbow rings that light from the upper left
        parts = []
        sizes = (0.62, 0.47, 0.33, 0.19)
        hues = [rgb(c) for c in ("#4C7BFF", "#3FD8E6", "#79F28A", "#F1F27C")]
        for k, (h, hue) in enumerate(zip(sizes, hues)):
            ring = regular(4, h * math.sqrt(2), 45)
            tones = [mix(hue, np.ones(3), 0.45), mix(hue, np.ones(3), 0.12), mix(hue, np.zeros(3), 0.12),
                     mix(hue, np.zeros(3), 0.3)]
            inner = regular(4, sizes[k + 1] * math.sqrt(2), 45) if k + 1 < len(sizes) else None
            parts += [(ring, tones[1])]
            if inner is not None:
                parts += wedges((0, 0), ring, inner, tones, outer=ring)
        parts.append((regular(4, 0.08 * math.sqrt(2), 45), rgb("#FFE3F6")))
        square = regular(4, 0.76 * math.sqrt(2), 45)
        gems.append(Gem("Bismuth", rounded(square, 0.12), rounded(square, 0.04), rgb("#4E3350"), rgb("#B48CB6"),
                        parts, rgb("#9AD6E0")))
        # 10 Garnet: round, an octagon table, deep reds
        gems.append(round_gem("Garnet", 8, 22.5, 0.5, ("#EC6A80", "#C42B4B", "#9A193B", "#6C0E2A"), "#D84560",
                              "#3B0615"))
    return gems


def render(gem, size, upx, origin, lift=0.0):
    """The (size x size) RGBA float image of `gem` with its bounding-box centre at `origin` (output pixels) and upx
    output pixels per unit; lift moves its colours toward white."""
    w = size * SS

    def mask(poly):
        im = Image.new("L", (w, w), 0)
        pts = [((origin[0] + (x - gem.centre[0]) * upx) * SS, (origin[1] + (y - gem.centre[1]) * upx) * SS)
               for x, y in poly]
        ImageDraw.Draw(im).polygon(pts, fill=255)
        return np.asarray(im) > 127

    white = np.ones(3)
    img = np.zeros((w, w, 4))
    outline = mask(gem.outline)
    img[outline, :3] = mix(gem.rim, white, lift * 0.5)
    img[outline, 3] = 1
    body = mask(gem.body)
    img[body, :3] = mix(gem.fill, white, lift)
    for poly, colour in gem.parts:
        m = mask(poly) & body
        img[m, :3] = mix(colour, white, lift)
    return downsample(img, SS)


def render_cell(gem, lift=0.0, scale=SCALE):
    img = render(gem, CELL * scale, UNIT * scale, (ANCHOR[0] * scale, ANCHOR[1] * scale), lift)
    # the game samples x 2..50, y 0..48 of a cell: keep the gutter empty
    img[:, :2 * scale] = 0
    img[:, 51 * scale:] = 0
    img[49 * scale:] = 0
    return img


def make_sheet(gems, scale=SCALE):
    """The gem sheet (colour c: spin row 2c, glow row 2c + 1), every frame of a row the same picture."""
    c = CELL * scale
    sheet = np.zeros((2 * len(gems) * c, FRAMES * c, 4))
    for gi, gem in enumerate(gems):
        rest, lit = render_cell(gem, 0.0, scale), render_cell(gem, GLOW_LIFT, scale)
        for f in range(FRAMES):
            sheet[2 * gi * c:(2 * gi + 1) * c, f * c:(f + 1) * c] = rest
            sheet[(2 * gi + 1) * c:(2 * gi + 2) * c, f * c:(f + 1) * c] = lit
        print(f"  {gem.name}")
    return to_image(sheet)


def check(gems):
    ok = True
    for gem in gems:
        x0 = ANCHOR[0] - gem.extent[0] / 2 * UNIT
        y0 = ANCHOR[1] - gem.extent[1] / 2 * UNIT
        x1, y1 = x0 + gem.extent[0] * UNIT, y0 + gem.extent[1] * UNIT
        inside = x0 >= 2 and x1 <= 50 and y0 >= 0 and y1 <= 48
        ok &= inside
        print(f"{gem.name:15s} x {x0:5.2f}..{x1:5.2f}  y {y0:5.2f}..{y1:5.2f}{'' if inside else '  OUTSIDE'}")
    return ok


def render_title(gems, scale=SCALE):
    """The title patch over the five gems of the title picture (gemsheet.TITLE_BOXES): the slate background with
    the old glows' haze, each new gem in its old gem's box with a glow of its colour, opaque around the gems and
    fading out beyond (as gemsheet.render_title does for the ray-cast gems)."""
    x0, y0, w, h = TITLE_RECT
    pw, ph = w * scale, h * scale
    X, Y = np.meshgrid(x0 + (np.arange(pw) + 0.5) / scale, y0 + (np.arange(ph) + 0.5) / scale)
    rng = np.random.default_rng(1)
    grain = rng.normal(0.0, TITLE_NOISE / 255.0, (h, w)).astype(np.float32)
    grain = np.asarray(Image.fromarray(grain, "F").resize((pw, ph), Image.BILINEAR), dtype=np.float64)
    colour = np.array(TITLE_STONE, dtype=np.float64)[None, None, :] / 255.0 + grain[..., None]
    (kh, sh), (kc, sc), (kw, sw) = TITLE_GLOW
    haze = np.array(TITLE_HAZE)
    near = np.full(X.shape, np.inf)
    layers = []
    for ci, (cx, cy), (bw, bh) in TITLE_BOXES:
        gem = gems[ci]
        upx = min(bw / gem.extent[0], bh / gem.extent[1]) * 0.92     # title pixels per unit
        hull = hull2d(gem.outline)
        poly = np.stack([cx + (hull[:, 0] - gem.centre[0]) * upx, cy + (hull[:, 1] - gem.centre[1]) * upx], 1)
        dist = polygon_distance(poly, X, Y)
        near = np.minimum(near, dist)
        out = np.clip(dist, 0, None)
        colour += ((kh * np.exp(-out / sh))[..., None] * haze + (kc * np.exp(-out / sc))[..., None] * gem.tint
                   + (kw * np.exp(-out / sw))[..., None])
        size = int(math.ceil(max(gem.extent) * upx * scale)) + 8
        tx, ty = int(math.floor((cx - x0) * scale - size / 2)), int(math.floor((cy - y0) * scale - size / 2))
        origin = ((cx - x0) * scale - tx, (cy - y0) * scale - ty)
        layers.append((render(gem, size, upx * scale, origin), tx, ty))
        print(f"  title {gem.name}")
    for _, centre, box in TITLE_BOXES:
        near = np.minimum(near, rect_distance((centre, box), X, Y))
    colour = np.clip(colour, 0, 1)
    for img, tx, ty in layers:
        sx0, sy0 = max(0, -tx), max(0, -ty)
        dx0, dy0 = max(0, tx), max(0, ty)
        cw, chh = min(img.shape[1] - sx0, pw - dx0), min(img.shape[0] - sy0, ph - dy0)
        if cw <= 0 or chh <= 0:
            continue
        src = img[sy0:sy0 + chh, sx0:sx0 + cw]
        dst = colour[dy0:dy0 + chh, dx0:dx0 + cw]
        dst[:] = dst * (1 - src[..., 3:4]) + src[..., :3] * src[..., 3:4]
    m0, m1 = TITLE_COVER
    cover = (1 - smoothstep(m0, m0 + m1, near)) * smoothstep(*TITLE_TOP, Y)
    return to_image(np.dstack([colour, cover]))


def main():
    here = os.path.dirname(os.path.abspath(__file__))
    ap = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    ap.add_argument("--out", default=os.path.join(here, "..", "mods"))
    ap.add_argument("--colors", type=int, choices=(7, 11), default=11, help="gem colours in the sheet")
    ap.add_argument("--no-title", action="store_true", help="leave the title screen's gems as they are")
    ap.add_argument("--preview", help="also write a preview PNG (the sheet on a dark background, @4x and 1x)")
    ap.add_argument("--check", action="store_true", help="only check that every gem stays inside its cell")
    args = ap.parse_args()
    gems = su_gems(args.colors)
    if args.check:
        raise SystemExit(0 if check(gems) else 1)
    if not check(gems):
        raise SystemExit("a gem leaves its cell")
    mod = os.path.join(os.path.abspath(args.out), "su-gems")
    sheet_rel = "images/gemsheet6@4x.png"
    title_rel = "patches/su-title@4x.png"
    print("drawing")
    sheet = make_sheet(gems)
    assert sheet.size == (FRAMES * CELL * SCALE, 2 * len(gems) * CELL * SCALE)
    save(sheet, mod, sheet_rel)
    patches = []
    set_title_patch(mod, title_rel, patches, args.no_title, lambda: render_title(gems))
    data = {
        "id": "su-gems",
        "name": "Steven Universe Gems (fan art)",
        "version": "2.0",
        "author": "byo-pop",
        "description": "Fan art: the gems redrawn as gems from Steven Universe, as the show draws them: flat colours, "
                       "a dark rim and wedge facets around a table (new drawings; nothing from the show is used). "
                       "Yellow Diamond, Pearl, Sapphire, Ruby, Amethyst, Jasper and Peridot keep the gems' colours"
                       + (", and Rose Quartz, Lapis Lazuli, Bismuth and Garnet play the extra colours"
                          if len(gems) > 7 else "")
                       + ". Static pictures, HD (@4x)"
                       + ("." if args.no_title else "; the same gems on the title screen."),
    }
    if patches:
        data["imagePatches"] = patches
    write_mod_json(mod, data)
    print(f"wrote {mod}")
    if args.preview:
        write_preview(sheet, args.preview)
        print(f"preview {args.preview}")


if __name__ == "__main__":
    main()
