#!/usr/bin/env python3
"""The parts the gem-sheet generators share (make_bej2_gems_mod.py, make_su_gems_mod.py; split out of the first).

A gem is a convex solid given as half-spaces n.x <= d in model units (an 84-unit cell, centre at 0), with its
corners (`verts`) and a rest orientation (`r0`): an object with the attributes n, d, verts and r0, such as either
generator's Gem. This module has
  * geometry: the planes of rings, mirrored crowns, convex hulls and support planes around a point sample;
  * the ray caster (cast): per supersampled pixel, the silhouette distance and the entering facet of a perspective
    ray, which each generator's own render() shades, then finish() turns into an RGBA tile;
  * the gem sheet's layout (images/gemsheet6: per colour a spin row and a glow row of FRAMES cells of CELL px):
    render_cell, make_sheet, and the game's 1x copy (box_down);
  * the title screen: the five gems under the logo (render_title), placed over the original's;
  * writing a mod (save, write_mod_json, set_title_patch, write_preview).
"""
import json
import math
import itertools
import os

import numpy as np
from PIL import Image

CELL = 52            # game cell (1x)
SCALE = 4            # patch pixel density (@4x)
FRAMES = 15
SS = 3               # supersampling per @4x pixel
UNIT_PX = 0.6        # 1x pixels per model unit (models are built in an 84-unit cell, centre at 0)
ANCHOR = (26.75, 24.75)   # 1x cell point where the centre of the rest pose's bounding box lands
EYE = 650.0          # eye distance from the gem centre (mild perspective)
SPIN_STEP = 12.0     # degrees per spin frame
GLOW_STEP = 24.0     # degrees the extra glow light moves per frame
PHI = (1 + 5 ** 0.5) / 2


def unit(v):
    v = np.asarray(v, dtype=np.float64)
    return v / np.linalg.norm(v, axis=-1, keepdims=True)


def smoothstep(e0, e1, x):
    t = np.clip((x - e0) / (e1 - e0), 0.0, 1.0)
    return t * t * (3 - 2 * t)


def rot_y(deg):
    t = math.radians(deg)
    c, s = math.cos(t), math.sin(t)
    return np.array([[c, 0, s], [0, 1, 0], [-s, 0, c]])


def rot_x(deg):
    """Positive: the top turns toward the viewer."""
    t = math.radians(deg)
    c, s = math.cos(t), math.sin(t)
    return np.array([[1, 0, 0], [0, c, -s], [0, s, c]])


def rot_z(deg):
    """Positive: counter-clockwise on screen."""
    t = math.radians(deg)
    c, s = math.cos(t), math.sin(t)
    return np.array([[c, -s, 0], [s, c, 0], [0, 0, 1]])


# --- geometry ----------------------------------------------------------------------------------------
# Model space: x right, y up, z toward the viewer, units of the 84-unit cell. A solid is a list of
# half-spaces n.x <= d with unit outward normals n.

def plane(normal, point):
    n = unit(normal)
    return n, float(n @ np.asarray(point, dtype=np.float64))


def ring(count, phase, tilt, radius, height, axis="z"):
    """`count` planes around an axis. Normal k tilts `tilt` degrees away from the axis toward azimuth
    phase + 360k/count; the plane passes through the point `radius` out from the axis in that azimuth,
    `height` along the axis. Axis z: azimuth from +x toward +y. Axis y: azimuth from +z toward +x."""
    out = []
    t = math.radians(tilt)
    for k in range(count):
        a = math.radians(phase + 360.0 * k / count)
        if axis == "z":
            n = (math.sin(t) * math.cos(a), math.sin(t) * math.sin(a), math.cos(t))
            p = (radius * math.cos(a), radius * math.sin(a), height)
        else:
            n = (math.sin(t) * math.sin(a), math.cos(t), math.sin(t) * math.cos(a))
            p = (radius * math.sin(a), height, radius * math.cos(a))
        out.append(plane(n, p))
    return out


def both_sides(planes_front):
    """A front crown ring plus its mirror image through z = 0 (the back of a plate-like gem)."""
    out = list(planes_front)
    for n, d in planes_front:
        out.append((n * np.array([1, 1, -1]), d))
    return out


def hull_planes(points):
    """Half-spaces of the convex hull of a point set (brute force; fine for <= 60 points)."""
    p = np.asarray(points, dtype=np.float64)
    idx = np.array(list(itertools.combinations(range(len(p)), 3)))
    a, b, c = p[idx[:, 0]], p[idx[:, 1]], p[idx[:, 2]]
    nrm = np.cross(b - a, c - a)
    ln = np.linalg.norm(nrm, axis=1)
    ok = ln > 1e-9
    nrm, a = nrm[ok] / ln[ok, None], a[ok]
    d = (nrm * a).sum(1)
    s = p @ nrm.T - d
    tol = 1e-6 * np.abs(p).max()
    above, below = (s > tol).any(0), (s < -tol).any(0)
    face = ~(above & below)
    nrm, d = nrm[face], d[face]
    flip = above[face]
    nrm[flip] *= -1
    d[flip] *= -1
    seen, out = set(), []
    for n, dd in zip(nrm, d):
        key = tuple(np.round(np.append(n, dd / np.abs(p).max()), 5))
        if key not in seen:
            seen.add(key)
            out.append((n, float(dd)))
    return out


def support_planes(points, count):
    """Half-spaces around a dense point sample of a smooth convex solid: for `count` directions spread
    evenly over the sphere (plus the axes and the diagonals of the xy plane), the plane touching the
    points from outside. Returns the planes and the touching points (the solid's vertices for its
    silhouettes). The points must look the same after a half turn about the vertical axis,
    (x, y, z) -> (-x, y, -z). The directions are a spiral over the front half (z > 0) plus its half
    turn, and the touching points come with their half turns too (ties on flat parts pick any one), so
    the back is exactly the front turned and the spin loop is seamless (180 degrees looks like 0)."""
    half = count // 2
    k = np.arange(half) + 0.5
    z = 1 - k / half
    phi = k * math.pi * (3 - math.sqrt(5))
    r = np.sqrt(1 - z * z)
    front = np.stack([r * np.cos(phi), r * np.sin(phi), z], 1)
    dirs = [front, front * [-1.0, 1.0, -1.0]]
    dirs.append(np.array([(1, 0, 0), (-1, 0, 0), (0, 1, 0), (0, -1, 0), (0, 0, 1), (0, 0, -1)], dtype=np.float64))
    dirs.append(unit(np.array([(1, 1, 0), (1, -1, 0), (-1, 1, 0), (-1, -1, 0)], dtype=np.float64)))
    dirs = np.concatenate(dirs)
    p = np.asarray(points, dtype=np.float64)
    s = p @ dirs.T
    top = s.argmax(0)
    planes = [(n, float(dd)) for n, dd in zip(dirs, s[top, np.arange(len(dirs))])]
    touch = p[np.unique(top)]
    return planes, np.unique(np.concatenate([touch, touch * [-1.0, 1.0, -1.0]]), axis=0)


def solid_vertices(n, d):
    """Corners of the solid {x : n.x <= d}: feasible intersections of plane triples."""
    idx = np.array(list(itertools.combinations(range(len(n)), 3)))
    a = n[idx]
    det = np.linalg.det(a)
    ok = np.abs(det) > 1e-9
    x = np.linalg.solve(a[ok], d[idx[ok]][..., None])[..., 0]
    inside = (x @ n.T <= d + 1e-6).all(1)
    return x[inside]


def hull2d(pts):
    """Convex hull (counter-clockwise, y up) by the monotone chain."""
    pts = sorted(set(map(tuple, np.round(pts, 9))))
    if len(pts) < 3:
        return np.array(pts)

    def half(seq):
        h = []
        for p in seq:
            while len(h) >= 2 and ((h[-1][0] - h[-2][0]) * (p[1] - h[-2][1])
                                   - (h[-1][1] - h[-2][1]) * (p[0] - h[-2][0])) <= 0:
                h.pop()
            h.append(p)
        return h

    lower, upper = half(pts), half(reversed(pts))
    return np.array(lower[:-1] + upper[:-1])


def profile_plane(count, phase, r1, z1, r2, z2):
    """A ring of crown planes through the profile points (r1, z1) and (r2, z2) (distance from the
    axis, height); r1 > r2."""
    tilt = math.degrees(math.atan2(z2 - z1, r1 - r2))
    return ring(count, phase, tilt, r1, z1)


def project(gem, rot):
    v = gem.verts @ rot.T
    s = EYE / (EYE - v[:, 2])
    return hull2d(v[:, :2] * s[:, None])


def orientation(gem, spin=0.0, pose=None):
    """Model-to-view rotation: the rest orientation, the spin about the vertical axis, then `pose`."""
    rot = rot_y(spin) @ gem.r0
    return rot if pose is None else pose @ rot


def bbox_centre(gem, spin=0.0, pose=None):
    h = project(gem, orientation(gem, spin, pose))
    return (h[:, 0].min() + h[:, 0].max()) / 2, (h[:, 1].min() + h[:, 1].max()) / 2


# --- ray caster --------------------------------------------------------------------------------------------

class Cast:
    """The rays of one render (cast): the supersampled pixels of the silhouette's bounding box (rows, cols of the
    n_px x n_px output; sx, sy their model-space image-plane points, row-major), `dist` the distance inside the
    silhouette (model units, negative outside), `inside`, and for the rays that hit the solid (`hit`): `t_hit`,
    the entering facet `face`, the view-space facet planes (nv, d), the ray directions `dirs` from `eye`."""

    def __init__(self, **kw):
        self.__dict__.update(kw)


def cast(gem, rot, size, unit_px, origin, ss):
    """The rays of `gem` in view orientation `rot` for a (size x size) output (see render functions: unit_px
    output pixels per model unit, `origin` the output pixel of the model's image-plane point (0, 0)), or None when
    the silhouette misses the output."""
    nv = gem.n @ rot.T
    d = gem.d
    hull = project(gem, rot)
    # edge lines of the silhouette (counter-clockwise, y up): inside distance = min over edges
    e0, e1 = hull, np.roll(hull, -1, axis=0)
    en = np.stack([e1[:, 1] - e0[:, 1], -(e1[:, 0] - e0[:, 0])], 1)
    en /= np.linalg.norm(en, axis=1, keepdims=True)
    en = -en  # inward
    ec = (en * e0).sum(1)

    n_px = size * ss
    px = (np.arange(n_px) + 0.5) / ss
    mx = (px - origin[0]) / unit_px
    my = (origin[1] - px) / unit_px
    # restrict the work to the silhouette's bounding box
    x0, x1 = hull[:, 0].min() - 1, hull[:, 0].max() + 1
    y0, y1 = hull[:, 1].min() - 1, hull[:, 1].max() + 1
    cols = np.nonzero((mx >= x0) & (mx <= x1))[0]
    rows = np.nonzero((my >= y0) & (my <= y1))[0]
    if len(cols) == 0 or len(rows) == 0:
        return None
    gx, gy = np.meshgrid(mx[cols], my[rows])
    sx, sy = gx.ravel(), gy.ravel()
    dist = (np.stack([sx, sy], 1) @ en.T - ec).min(1)
    inside = dist > 0

    eye = np.array([0.0, 0.0, EYE])
    dirs = unit(np.stack([sx, sy, np.full_like(sx, -EYE)], 1))
    num = d - nv @ eye
    face = np.zeros(len(sx), dtype=int)
    t_hit = np.full(len(sx), -np.inf)
    t_out = np.full(len(sx), np.inf)
    rays = np.nonzero(inside)[0]
    step = max(1, 4000000 // len(d))          # rays per chunk (solids of many planes)
    for c0 in range(0, len(rays), step):
        r = rays[c0:c0 + step]
        denom = dirs[r] @ nv.T
        with np.errstate(divide="ignore", invalid="ignore"):
            t = num / denom
        t_in = np.where(denom < 0, t, -np.inf)
        face[r] = t_in.argmax(1)
        t_hit[r] = t_in.max(1)
        t_out[r] = np.where(denom > 0, t, np.inf).min(1)
    hit = inside & (t_hit < t_out) & np.isfinite(t_hit)
    return Cast(n_px=n_px, rows=rows, cols=cols, shape=gx.shape, sx=sx, sy=sy, dist=dist, inside=inside, hit=hit,
                t_hit=t_hit, face=face, nv=nv, d=d, dirs=dirs, eye=eye)


def empty(size, ss):
    """The render of a gem outside its output."""
    return downsample(np.zeros((size * ss, size * ss, 4)), ss)


def finish(c, rgb, ss, alpha=None):
    """The (size x size) RGBA image of a cast whose pixels got the colours rgb (one row per pixel of c): alpha is
    `inside` unless given."""
    tile = np.zeros((len(c.rows), len(c.cols), 4))
    tile[..., :3] = rgb.reshape(len(c.rows), len(c.cols), 3)
    tile[..., 3] = (c.inside if alpha is None else alpha).reshape(len(c.rows), len(c.cols))
    out = np.zeros((c.n_px, c.n_px, 4))
    out[c.rows[0]:c.rows[-1] + 1, c.cols[0]:c.cols[-1] + 1] = tile
    return downsample(out, ss)


def downsample(img, ss):
    """Premultiplied box filter ss x ss -> 1, back to straight alpha with RGB = 0 under alpha 0."""
    h, w = img.shape[0] // ss, img.shape[1] // ss
    pm = img.copy()
    pm[..., :3] *= pm[..., 3:4]
    pm = pm.reshape(h, ss, w, ss, 4).mean(axis=(1, 3))
    a = pm[..., 3:4]
    pm[..., :3] = np.where(a > 0, pm[..., :3] / np.maximum(a, 1e-9), 0)
    return pm


def to_image(img):
    return Image.fromarray(np.clip(np.round(img * 255), 0, 255).astype(np.uint8), "RGBA")


def gem_origin(gem, cell_px, unit_px, anchor):
    """Output-pixel origin so the rest pose's bounding-box centre lands on `anchor` (cell pixels)."""
    bx, by = bbox_centre(gem)
    return anchor[0] - bx * unit_px, anchor[1] + by * unit_px


def render_cell(render, gem, spin=0.0, glow=None, scale=SCALE, ss=SS):
    """One cell of the sheet: render(gem, spin, size, unit_px, origin, ss=, glow=) with the rest pose's bounding-box
    centre on ANCHOR."""
    unit_px = UNIT_PX * scale
    origin = gem_origin(gem, CELL * scale, unit_px, (ANCHOR[0] * scale, ANCHOR[1] * scale))
    img = render(gem, spin, CELL * scale, unit_px, origin, ss=ss, glow=glow)
    # the game samples x 2..50, y 0..48 of a cell: keep the gutter empty
    img[:, :2 * scale] = 0
    img[:, 51 * scale:] = 0
    img[49 * scale:] = 0
    return img


def make_sheet(render, gems, scale=SCALE, ss=SS, log=None):
    """The gem sheet of `gems` (colour c: spin row 2c, glow row 2c + 1) at `scale`, a PIL image."""
    c = CELL * scale
    sheet = np.zeros((2 * len(gems) * c, FRAMES * c, 4))
    for gi, gem in enumerate(gems):
        for f in range(FRAMES):
            sheet[2 * gi * c:(2 * gi + 1) * c, f * c:(f + 1) * c] = render_cell(render, gem, spin=f * SPIN_STEP,
                                                                                 scale=scale, ss=ss)
            sheet[(2 * gi + 1) * c:(2 * gi + 2) * c, f * c:(f + 1) * c] = render_cell(
                render, gem, glow=90.0 + f * GLOW_STEP, scale=scale, ss=ss)
        if log:
            log(f"  {gem.name}")
    return to_image(sheet)


def box_down(img, k):
    """Premultiplied box downsample of a PIL RGBA image by an integer factor (the game's own 1x copy)."""
    a = np.asarray(img, dtype=np.float64) / 255.0
    return to_image(downsample(a, k))


# --- title screen -------------------------------------------------------------------------------------
# The title picture (images/titlescreen6, 640 x 480) has five Bejeweled Deluxe gems under the logo: blue,
# green, purple, orange and red, each with a soft glow. The title patch draws the new gems over them, at
# their places and sizes, on a backdrop of the slate background and new glows; the patch fades out
# around the gems (alpha), so the old gems are covered and the picture continues around them. Nothing
# is read from the picture: places, sizes and colours below are hand-set constants.
TITLE_TARGET = "images/titlescreen6"
TITLE_RECT = (160, 146, 292, 120)        # x, y, width, height of the patch (title pixels)
# the old gems, back to front: colour index, the centre and size of the bounding box (title pixels; the outline of
# make_bej2_gems_mod.py's gem placed there, which matches the old gem's)
TITLE_BOXES = (
    (2, (211.0, 204.5), (68.27, 69.06)),
    (6, (261.5, 219.0), (60.95, 60.95)),
    (5, (357.5, 219.5), (67.23, 69.32)),
    (3, (402.75, 199.5), (62.22, 62.22)),
    (4, (310.0, 188.5), (66.68, 65.7)),
)
TITLE_STONE = (46, 52, 66)               # the slate background (its mean colour) ...
TITLE_NOISE = 1.5                        # ... and its grain (standard deviation, levels)
# glows, at d title pixels outside a gem: a wide warm haze (colour TITLE_HAZE), the gem's colour and a
# white rim, each k * exp(-d / s), given as (k, s) pairs
TITLE_HAZE = (1.0, 0.2, 0.0)
TITLE_GLOW = ((0.15, 10.0), (0.25, 6.0), (0.3, 2.0))
TITLE_COVER = (3.0, 7.0)                 # the patch is opaque up to 3 px outside the gems, gone at 10
TITLE_TOP = (147.0, 152.0)               # ... and fades out from y = 152 up to 147 (the logo's shadow)


def polygon_distance(poly, X, Y):
    """Signed distance (positive outside, mitred corners) from points X, Y to a convex polygon."""
    c = poly.mean(0)
    out = np.full(X.shape, -np.inf)
    for p0, p1 in zip(poly, np.roll(poly, -1, axis=0)):
        n = np.array([p1[1] - p0[1], p0[0] - p1[0]])
        ln = np.linalg.norm(n)
        if ln < 1e-9:
            continue
        n /= ln
        if n @ (c - p0) > 0:
            n = -n
        out = np.maximum(out, (X - p0[0]) * n[0] + (Y - p0[1]) * n[1])
    return out


def rect_distance(box, X, Y):
    """Signed distance (positive outside, mitred corners) from points X, Y to an axis-aligned box
    ((cx, cy), (w, h))."""
    (cx, cy), (w, h) = box
    return np.maximum(np.abs(X - cx) - w / 2, np.abs(Y - cy) - h / 2)


def render_title(render, gems, places, tint, scale=SCALE, ss=SS, log=None, boxes=()):
    """The title patch: an RGBA image of TITLE_RECT at `scale` (see TITLE_TARGET). places: per title gem, back to
    front, (colour index, centre of the bounding box in title pixels, title pixels per model unit, spin, tilt (top
    toward the viewer), roll (counter-clockwise), degrees); gems[colour index] is drawn there by render (see
    render_cell), with glows in tint(gem) (an RGB array in 0..1). boxes: further areas the patch covers, ((cx, cy),
    (w, h)) in title pixels (the old gems' boxes, when the new gems do not cover them)."""
    x0, y0, w, h = TITLE_RECT
    pw, ph = w * scale, h * scale
    X, Y = np.meshgrid(x0 + (np.arange(pw) + 0.5) / scale, y0 + (np.arange(ph) + 0.5) / scale)
    rng = np.random.default_rng(1)
    grain = rng.normal(0.0, TITLE_NOISE / 255.0, (h, w)).astype(np.float32)
    grain = np.asarray(Image.fromarray(grain, "F").resize((pw, ph), Image.BILINEAR), dtype=np.float64)
    rgb = np.array(TITLE_STONE, dtype=np.float64)[None, None, :] / 255.0 + grain[..., None]
    (kh, sh), (kc, sc), (kw, sw) = TITLE_GLOW
    haze = np.array(TITLE_HAZE)
    near = np.full(X.shape, np.inf)
    layers = []
    for ci, (cx, cy), upx, spin, tilt, roll in places:
        gem = gems[ci]
        pose = rot_z(roll) @ rot_x(tilt)
        hull = project(gem, orientation(gem, spin, pose))
        bx, by = (hull.min(0) + hull.max(0)) / 2
        ox, oy = cx - bx * upx, cy + by * upx                  # title position of the model's origin
        dist = polygon_distance(np.stack([ox + hull[:, 0] * upx, oy - hull[:, 1] * upx], 1), X, Y)
        near = np.minimum(near, dist)
        out = np.clip(dist, 0, None)
        rgb += ((kh * np.exp(-out / sh))[..., None] * haze + (kc * np.exp(-out / sc))[..., None] * tint(gem)
                + (kw * np.exp(-out / sw))[..., None])
        # the gem itself, rendered into a square tile around its origin
        size = int(math.ceil(np.abs(hull).max() * 2 * upx * scale)) + 8
        opx, opy = (ox - x0) * scale, (oy - y0) * scale
        tx, ty = int(math.floor(opx - size / 2)), int(math.floor(opy - size / 2))
        layers.append((render(gem, spin, size, upx * scale, (opx - tx, opy - ty), ss=ss, pose=pose), tx, ty))
        if log:
            log(f"  title {gem.name}")
    for box in boxes:
        near = np.minimum(near, rect_distance(box, X, Y))
    rgb = np.clip(rgb, 0, 1)
    for img, tx, ty in layers:
        # paste the tile, clipped to the patch
        sx0, sy0 = max(0, -tx), max(0, -ty)
        dx0, dy0 = max(0, tx), max(0, ty)
        cw, chh = min(img.shape[1] - sx0, pw - dx0), min(img.shape[0] - sy0, ph - dy0)
        if cw <= 0 or chh <= 0:
            continue
        src = img[sy0:sy0 + chh, sx0:sx0 + cw]
        dst = rgb[dy0:dy0 + chh, dx0:dx0 + cw]
        dst[:] = dst * (1 - src[..., 3:4]) + src[..., :3] * src[..., 3:4]
    m0, m1 = TITLE_COVER
    cover = (1 - smoothstep(m0, m0 + m1, near)) * smoothstep(*TITLE_TOP, Y)
    return to_image(np.dstack([rgb, cover]))


# --- writing the mod --------------------------------------------------------------------------------------

def save(img, mod, rel):
    """Save a PIL image as <mod>/<rel> (folders made)."""
    path = os.path.join(mod, *rel.split("/"))
    os.makedirs(os.path.dirname(path), exist_ok=True)
    img.save(path, optimize=True)
    return path


def set_title_patch(mod, rel, patches, skip, make):
    """The title patch <mod>/<rel> from make() and its imagePatches entry, or (skip) no title patch."""
    path = os.path.join(mod, *rel.split("/"))
    if skip:
        if os.path.exists(path):
            os.remove(path)
        return
    save(make(), mod, rel)
    patches.append({"target": TITLE_TARGET, "image": rel, "x": TITLE_RECT[0], "y": TITLE_RECT[1], "mode": "over"})


def write_mod_json(mod, data):
    with open(os.path.join(mod, "mod.json"), "w", encoding="utf-8") as f:
        json.dump(data, f, indent=2)
        f.write("\n")


def write_preview(sheet, path, scale=SCALE):
    """A preview PNG: the sheet on a dark background, then its 1x copy at twice the size."""
    one = box_down(sheet, scale)
    bg = Image.new("RGBA", (sheet.width, sheet.height + one.height * 2), (34, 30, 48, 255))
    bg.alpha_composite(sheet)
    bg.alpha_composite(one.resize((one.width * 2, one.height * 2), Image.NEAREST), (0, sheet.height))
    bg.convert("RGB").save(path)
