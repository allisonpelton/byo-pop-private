#!/usr/bin/env python3
"""Generate the "Steven Universe Gems" mod (fan art): the gems redrawn as gems from Steven Universe.

    python games/bejeweled/tools/make_su_gems_mod.py [--out <mods folder, default games/bejeweled/mods>]
                                                     [--colors 7|11] [--no-title] [--preview <png>] [--check]

games/bejeweled/docs/CODE_MODS_DESIGN.md 4.17. Each gem keeps its Bejeweled colour, so play reads the same:
0 yellow Yellow Diamond (a kite), 1 white Pearl (a smooth oval), 2 blue Sapphire (a teardrop, point up), 3 red Ruby
(a rounded square), 4 purple Amethyst (an upright oval), 5 orange Jasper (a long hexagon), 6 green Peridot (a
triangle, point down), and for stage 8's colours 7 Rose Quartz (a pentagon), 8 Lapis Lazuli (a teardrop, point
down), 9 Bismuth (an octagon of rainbow facets) and 10 Garnet (a round cut).

All art is new and procedural, after the show's well-known designs: this script reads no file (no show images, no
game data). The solids are step cuts of a convex outline (girdle, crown steps, table, the back a mirror image),
Pearl an ellipsoid; gemsheet.py casts the rays. The look is the show's flat cel shading: each facet takes one of
four tones by a quantised Lambert term from a fixed light, facets steep to the view take a darker rim tone, one hard
white glint sits where the light reflects off the gem's smooth proxy (an ellipsoid of its size), and a dark outline
runs inside the silhouette. No refraction. Pearl's tones come from its smooth normal, tinted pastel toward the rim
by the direction it faces; Bismuth's facets each take a hue of their own.

Output: games/bejeweled/mods/su-gems (git-ignored: fan art of a TV show's designs), a whole images/gemsheet6@4x.png
(the game makes its 1x pixels by box downsampling) with 11 colours by default, so stage 8's extra colours (more
than 7, board.gemColors) show these gems too; --colors 7 writes only the seven. A sheet, not a patch: a patch
cannot make the sheet taller. Unless --no-title, a @4x patch draws the same gems over the five of the title picture
(gemsheet.render_title). --check prints each gem's extent over its 15 spin frames and fails when a frame leaves the
cell's sampled area (x 2..50, y 0..48).
"""
import argparse
import colorsys
import math
import os

import numpy as np

from gemsheet import (ANCHOR, CELL, FRAMES, SCALE, SPIN_STEP, SS, TITLE_BOXES, UNIT_PX, cast, empty, finish,
                      gem_origin, make_sheet, orientation, project, render_title, save, set_title_patch,
                      solid_vertices, support_planes, unit, write_mod_json, write_preview)

LIGHT = unit((-0.6, 0.7, 1.0))     # view space, upper-left front
STEPS = (0.35, 0.6, 0.85)          # Lambert thresholds between the four tones
GLOW = (0.45, 0.35)                # hover glow: the circling light's strength and its height (z)
OUTLINE = 1.6                      # outline width, model units (about 1 px at 1x)


# --- solids ---------------------------------------------------------------------------------------------------
# Model space as gemsheet's: x right, y up, z toward the viewer, the 84-unit cell; outlines are convex polygons
# (counter-clockwise, y up), symmetric left to right, so every solid looks the same after the spin's half turn

def ngon(count, rx, ry, phase=90.0):
    a = np.radians(phase + 360.0 * np.arange(count) / count)
    return np.stack([rx * np.cos(a), ry * np.sin(a)], 1)


def teardrop(r, cy, apex, count):
    """A circle of radius r around (0, cy) and its two tangents to the apex (0, apex); a negative apex below cy
    points the drop down. The arc gets `count` points."""
    up = apex > cy
    dist = abs(apex - cy)
    half = math.degrees(math.acos(r / dist))
    toward = 90.0 if up else -90.0
    a = np.radians(toward + half + (360.0 - 2 * half) * np.arange(count) / (count - 1))
    arc = np.stack([r * np.cos(a), cy + r * np.sin(a)], 1)
    return np.concatenate([[(0.0, apex)], arc]) if up else np.concatenate([arc, [(0.0, apex)]])


def chamfered(poly, cut):
    """The polygon with every corner cut back `cut` units along both edges."""
    out = []
    for i in range(len(poly)):
        p, a, b = poly[i], poly[i - 1], poly[(i + 1) % len(poly)]
        out.append(p + unit(a - p) * cut)
        out.append(p + unit(b - p) * cut)
    return np.array(out)


def step_cut(poly, girdle, steps):
    """Planes of a step cut: a vertical girdle plane per outline edge from z -girdle to girdle, then per crown step
    (inset, z) a ring of planes from the previous edge offset and height to the edge moved `inset` inwards at z, the
    last z the table's; the back mirrors the front."""
    poly = np.asarray(poly, dtype=np.float64)
    e = np.roll(poly, -1, axis=0) - poly
    m = unit(np.stack([e[:, 1], -e[:, 0]], 1))           # outward edge normals (counter-clockwise outline)
    o = (m * poly).sum(1)
    planes = [(np.array([mx, my, 0.0]), float(oo)) for (mx, my), oo in zip(m, o)]
    crown = []
    off, z = o, girdle
    for inset, z2 in steps:
        dz, do = z2 - z, inset
        ln = math.hypot(dz, do)
        for (mx, my), ok in zip(m, off):
            crown.append((np.array([mx * dz, my * dz, do]) / ln, float((dz * ok + do * z) / ln)))
        off, z = off - inset, z2
    crown.append((np.array([0.0, 0.0, 1.0]), float(z)))
    for n, d in crown:
        planes.append((n, d))
        planes.append((n * np.array([1.0, 1.0, -1.0]), d))
    return planes


def ellipsoid_points(axes, rings=24, count=48):
    """A point sample of an ellipsoid's surface that looks the same after a half turn about y."""
    pts = [(0.0, axes[1], 0.0), (0.0, -axes[1], 0.0)]
    for i in range(1, rings):
        lat = math.pi * i / rings - math.pi / 2
        for k in range(count):
            lon = 2 * math.pi * k / count
            pts.append((axes[0] * math.cos(lat) * math.cos(lon), axes[1] * math.sin(lat),
                        axes[2] * math.cos(lat) * math.sin(lon)))
    return np.array(pts)


class Gem:
    """A convex solid (gemsheet's half-spaces) and its cel look: four tones from shadow to highlight (RGB),
    `smooth` (Pearl: shaded from the proxy ellipsoid's normal, tinted pastel), `rainbow` (Bismuth: a hue per facet).
    `axes` are the half-axes of the smooth proxy (the glint, Pearl's normal): the solid's extent unless given."""

    def __init__(self, name, planes, tones, verts=None, axes=None, smooth=False, rainbow=False, rim=0.25,
                 glint=0.993):
        self.name = name
        self.n = np.array([p[0] for p in planes])
        self.d = np.array([p[1] for p in planes])
        self.verts = solid_vertices(self.n, self.d) if verts is None else np.asarray(verts, dtype=np.float64)
        self.r0 = np.eye(3)
        self.axes = np.abs(self.verts).max(0) if axes is None else np.asarray(axes, dtype=np.float64)
        self.tones = np.array(tones, dtype=np.float64) / 255.0
        self.line = self.tones[0] * 0.3
        self.smooth = smooth
        self.rainbow = rainbow
        self.rim = rim
        self.glint = glint


def faceted(name, poly, tones, girdle=4.0, steps=((5.0, 10.0), (5.0, 15.0)), **kw):
    return Gem(name, step_cut(poly, girdle, steps), tones, **kw)


PEARL_AXES = (33.0, 24.0, 15.0)
# Pearl's pastel tints by the facing direction's azimuth about the vertical axis (model space): lilac, pink, mint
PEARL_TINTS = np.array([(196, 186, 242), (250, 196, 226), (192, 240, 226)], dtype=np.float64) / 255.0


def su_gems(count=11):
    gems = [
        faceted("Yellow Diamond", [(0, 37), (-29, 9), (0, -37), (29, 9)],
                [(186, 140, 0), (238, 196, 18), (255, 232, 84), (255, 248, 186)],
                steps=((6.0, 10.0), (6.0, 15.0))),
        Gem("Pearl", *(lambda pv: (pv[0], [(214, 196, 216), (236, 224, 238), (250, 244, 250), (255, 255, 255)],
                                   pv[1]))(support_planes(ellipsoid_points(PEARL_AXES), 700)),
            axes=PEARL_AXES, smooth=True, glint=0.996),
        faceted("Sapphire", teardrop(23.0, -11.0, 36.0, 13),
                [(22, 88, 196), (52, 140, 238), (120, 192, 255), (200, 232, 255)]),
        faceted("Ruby", chamfered(ngon(4, 33.0 * math.sqrt(2), 33.0 * math.sqrt(2), 45.0), 9.0),
                [(150, 0, 24), (214, 22, 46), (255, 84, 94), (255, 170, 170)],
                steps=((6.0, 10.0), (6.0, 15.0))),
        faceted("Amethyst", ngon(16, 26.0, 35.0, 90.0 + 360.0 / 32),
                [(108, 30, 150), (164, 72, 206), (210, 134, 240), (240, 204, 255)]),
        faceted("Jasper", ngon(6, 37.0, 26.0 / math.sin(math.radians(60)), 0.0),
                [(168, 66, 8), (226, 110, 30), (255, 160, 72), (255, 212, 150)]),
        faceted("Peridot", chamfered(ngon(3, 42.0, 42.0, -90.0), 7.0),
                [(36, 128, 18), (88, 190, 40), (158, 232, 90), (214, 250, 172)],
                steps=((6.0, 10.0), (5.0, 15.0))),
        faceted("Rose Quartz", ngon(5, 36.0, 36.0, 90.0),
                [(196, 74, 136), (238, 128, 178), (255, 180, 214), (255, 226, 240)]),
        faceted("Lapis Lazuli", teardrop(23.0, 11.0, -36.0, 13),
                [(18, 30, 120), (36, 66, 186), (84, 124, 232), (164, 190, 255)]),
        faceted("Bismuth", ngon(8, 34.0, 34.0, 90.0 + 22.5),
                [(70, 70, 70), (120, 120, 120), (175, 175, 175), (225, 225, 225)], rainbow=True),
        faceted("Garnet", ngon(20, 35.0, 35.0, 90.0 + 9.0),
                [(104, 0, 52), (164, 18, 84), (218, 60, 126), (250, 152, 192)],
                steps=((4.0, 9.0), (5.0, 13.0), (5.0, 16.0))),
    ]
    return gems[:count]


# --- cel shading ----------------------------------------------------------------------------------------------

def hsv(h, s, v):
    return np.array([colorsys.hsv_to_rgb(a, b, c) for a, b, c in zip(h, s, v)]).reshape(-1, 3)


def render(gem, spin, size, unit_px, origin, ss=SS, glow=None, pose=None):
    """Render `gem` turned `spin` degrees about the vertical axis into a (size x size) RGBA float image, as
    make_bej2_gems_mod.render does (the same arguments), in the cel look (the module comment)."""
    rot = orientation(gem, spin, pose)
    c = cast(gem, rot, size, unit_px, origin, ss)
    if c is None:
        return empty(size, ss)
    rgb = np.zeros((len(c.sx), 3))
    h = np.nonzero(c.hit)[0]
    if len(h):
        dh = c.dirs[h]
        xm = (c.eye + c.t_hit[h, None] * dh) @ rot          # model space
        fh = c.face[h]
        v = -dh
        nsm = unit(xm / gem.axes ** 2)                      # the smooth proxy's normal, model space
        ns = nsm @ rot.T
        n = ns if gem.smooth else c.nv[fh]
        light = np.clip(n @ LIGHT, 0, 1)
        if glow is not None:
            a = math.radians(glow)
            light = light + GLOW[0] * np.clip(n @ unit((math.cos(a), math.sin(a), GLOW[1])), 0, 1) ** 2
        level = np.digitize(light, STEPS)
        ndv = np.clip((n * v).sum(1), 0, 1)
        col = gem.tones[level]
        if gem.rainbow:
            # a hue per facet from its model normal, the same for a facet and its half-turned twin: round the outline
            # from the bottom to the top, each crown step a third of the wheel on
            nm = gem.n[fh]
            hue = ((np.arctan2(nm[:, 1], np.abs(nm[:, 0])) / math.pi + 0.5) * 0.85 + 0.33 * np.round(np.abs(nm[:, 2]) * 3)) % 1.0
            col = hsv(hue, np.full(len(h), 0.6), gem.tones[level].max(1))
        if gem.smooth:
            az = (np.arctan2(nsm[:, 0], nsm[:, 2]) / (2 * math.pi)) % 0.5 * 6.0      # 0..3, a half turn
            k = np.floor(az).astype(int) % 3
            t = (az - np.floor(az))[:, None]
            tint = PEARL_TINTS[k] * (1 - t) + PEARL_TINTS[(k + 1) % 3] * t
            s = (np.clip(1 - ndv, 0, 1) ** 1.5 * 0.75)[:, None]
            col = col * (1 - s) + tint * s
        rim = ndv < gem.rim
        col = np.where(rim[:, None], gem.tones[0] * 0.8, col)
        glint = (ns * unit(LIGHT + v)).sum(1) > gem.glint
        col = np.where(glint[:, None], 1.0, col)
        rgb[h] = col
    rgb = np.where((c.dist < OUTLINE)[:, None], gem.line, rgb)
    return finish(c, rgb, ss)


# --- checks -------------------------------------------------------------------------------------------------

def check(gems):
    """Each gem's extent over the spin frames in 1x cell pixels; False when one leaves x 2..50, y 0..48."""
    ok = True
    for gem in gems:
        ox, oy = gem_origin(gem, CELL, UNIT_PX, ANCHOR)
        x0 = y0 = math.inf
        x1 = y1 = -math.inf
        for f in range(FRAMES):
            hull = project(gem, orientation(gem, f * SPIN_STEP))
            x0, x1 = min(x0, ox + hull[:, 0].min() * UNIT_PX), max(x1, ox + hull[:, 0].max() * UNIT_PX)
            y0, y1 = min(y0, oy - hull[:, 1].max() * UNIT_PX), max(y1, oy - hull[:, 1].min() * UNIT_PX)
        inside = x0 >= 2 and x1 <= 50 and y0 >= 0 and y1 <= 49
        ok = ok and inside
        print(f"  {gem.name}: x {x0:.1f}..{x1:.1f}, y {y0:.1f}..{y1:.1f}{'' if inside else '  OUTSIDE THE CELL'}")
    return ok


def title_places(gems):
    """Each old title gem's place for the new gem of its colour, as large as fits the old gem's box."""
    places = []
    for ci, centre, (w, h) in TITLE_BOXES:
        hull = project(gems[ci], orientation(gems[ci]))
        span = hull.max(0) - hull.min(0)
        places.append((ci, centre, min(w / span[0], h / span[1]), 0.0, 0.0, 0.0))
    return places


def main():
    here = os.path.dirname(os.path.abspath(__file__))
    ap = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    ap.add_argument("--out", default=os.path.join(here, "..", "mods"))
    ap.add_argument("--colors", type=int, choices=(7, 11), default=11, help="gem colours in the sheet")
    ap.add_argument("--no-title", action="store_true", help="leave the title screen's gems as they are")
    ap.add_argument("--preview", help="also write a preview PNG (the sheet on a dark background, @4x and 1x)")
    ap.add_argument("--check", action="store_true", help="only check that every frame stays inside its cell")
    args = ap.parse_args()
    gems = su_gems(args.colors)
    if args.check:
        raise SystemExit(0 if check(gems) else 1)
    mod = os.path.join(os.path.abspath(args.out), "su-gems")
    sheet_rel = "images/gemsheet6@4x.png"
    title_rel = "patches/su-title@4x.png"
    print("rendering")
    sheet = make_sheet(render, gems, log=print)
    assert sheet.size == (FRAMES * CELL * SCALE, 2 * len(gems) * CELL * SCALE)
    save(sheet, mod, sheet_rel)
    patches = []
    set_title_patch(mod, title_rel, patches, args.no_title,
                    lambda: render_title(render, gems, title_places(gems), lambda gem: gem.tones[2], log=print,
                                         boxes=[(centre, size) for _, centre, size in TITLE_BOXES]))
    data = {
        "id": "su-gems",
        "name": "Steven Universe Gems (fan art)",
        "version": "1.0",
        "author": "byo-pop",
        "description": "Fan art: the gems redrawn as gems from Steven Universe in the show's flat cel shading "
                       "(new procedural art; nothing from the show is used). Yellow Diamond, Pearl, Sapphire, Ruby, "
                       "Amethyst, Jasper and Peridot keep the gems' colours"
                       + (", and Rose Quartz, Lapis Lazuli, Bismuth and Garnet play the extra colours"
                          if len(gems) > 7 else "")
                       + ". HD (@4x); spin and hover animations"
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
