#!/usr/bin/env python3
"""Generate the "Bejeweled 2 Gems" mod: the seven gems redrawn in the style of Bejeweled 2.

    python games/bejeweled/tools/make_bej2_gems_mod.py [--out <mods folder, default games/bejeweled/mods>]
                                                       [--preview <png>]

All art is new and procedural: this script reads no file at all (no Bejeweled 2 or Bejeweled Deluxe
data, no reference pixels). Each gem is a small faceted solid (a convex polyhedron given as
half-spaces) that a numpy ray caster renders with supersampling. Shapes, lights and colour ramps are
hand-tuned constants, chosen by comparing renders with Bejeweled 2 by eye; that is why the output is
a close recreation of another game's look and stays git-ignored (only this generator is tracked).

Rendering, per pixel:
  * a perspective ray from the eye hits the solid (entry = the last entering half-space);
  * a shading normal: the facet normal, optionally
      - bent toward a smooth proxy ellipsoid (curved bevels; a negative bend gives the white ball's
        shallow dimples),
      - rolled toward a neighbouring table/ring plane near their shared edge ("fillet": a rounded
        edge, which is where Bejeweled 2's hard streak along the upper-left table edge comes from),
      - or, for the red cushion, taken from a height field (crescent bevels around a pinched table,
        corner facets);
  * light: a key light from the upper-left front, a strong second light from the lower-right front,
    and "bands" of an imagined environment looked up with the reflected view ray; see-through gems
    mix in a refracted look at the back facets. The red instead looks its colour up straight from the
    normal's direction (red_shade), with hand-set terms for its rim, streak and corner facets;
  * a tone curve, measured once per gem from its own quick renders, spreads the intensities over the
    colour ramp so the ramp's stops (dark floor = 0.6 x the full colour ... highlight) land on the
    luminance quantiles they stand for (the red's own shading is already in ramp units);
  * a crisp outline (black, then a dark step into the body colour) inside the silhouette. No halo.

Output: one @4x image patch over all of images/gemsheet6 (780x728 logical, 15 x 14 cells of 52 px;
colour c uses row 2c for the spin animation and row 2c+1 for the hover glow). A patch rather than an
images/gemsheet6@4x.png file, so the gems show whatever other gem-sheet mod is enabled below it.
  spin row: frame 0 is the rest pose, frames 1..14 turn the gem about the vertical axis by 12 degrees
            each (180 degrees per cycle; every solid here is symmetric under that half turn, so the
            loop is seamless);
  glow row: the rest pose with an extra light circling the gem (24 degrees per frame, one turn).
Unless --no-title is given, a second @4x patch ("over", with alpha) draws the same gems over the five
gems of the title picture (images/titlescreen6; see TITLE_GEMS).
"""
import argparse
import itertools
import json
import math
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


# --- light ---------------------------------------------------------------------------------------------
# View space (fixed; the gem turns under the lights): x right, y up, z toward the eye.
KEY = unit((-1.0, 1.0, 1.3))      # upper-left front
FILL = unit((1.0, -1.0, 0.8))     # lower-right front ("transmission" light; a Bejeweled 2 signature)


def band(azimuth, half_az, tilt, half_tilt, strength, soft=0.4):
    """A bright area of the imagined environment, seen in reflections: reflected view rays within
    `half_az` degrees of screen azimuth `azimuth` (0 = right, 90 = up, 135 = upper-left) and within
    `half_tilt` of `tilt` degrees off the view axis. A reflection lands there from normals tilted about
    half as far, so a narrow tilt range on a rounded edge draws a hard-edged streak. `soft` is the
    fraction of each half-width used for the anti-aliased fall-off."""
    return (azimuth, half_az, tilt, half_tilt, strength, soft)


def bands_at(bands, r):
    s = np.zeros(r.shape[0])
    if not bands:
        return s
    tilt = np.degrees(np.arccos(np.clip(r[:, 2], -1, 1)))
    az = np.degrees(np.arctan2(r[:, 1], r[:, 0]))
    for a, ha, t, ht, k, soft in bands:
        da = np.abs((az - a + 180) % 360 - 180)
        w = (1 - smoothstep(ha * (1 - soft), ha, da)) * (1 - smoothstep(ht * (1 - soft), ht, np.abs(tilt - t)))
        s += k * w
    return s


def ramp_colour(ramp, i):
    """Piecewise-linear colour ramp: ramp = [(position, (r,g,b)), ...], positions ascending in 0..1."""
    pos = [p for p, _ in ramp]
    out = np.empty(i.shape + (3,), dtype=np.float64)
    for ch in range(3):
        out[..., ch] = np.interp(i, pos, [c[ch] for _, c in ramp])
    return out / 255.0


def ramp8(stops):
    """Eight colour stops at the luminance quantiles 0, .1, .25, .5, .75, .9, .97, 1."""
    return list(zip((0.0, 0.10, 0.25, 0.50, 0.75, 0.90, 0.97, 1.0), stops))


class Gem:
    """A convex solid plus its look. Shading normal = facet normal, then
      bend:   per-face blend toward a proxy ellipsoid's normal (curved facets; negative = dimples);
      fillet: within `fillet` units of a fillet plane (a table or a bevel ring), the normal rolls
              toward that plane's normal, like a rounded edge; an environment band then draws a hard
              streak along it;
      normal_fn: a custom function instead of both.
    Look parameters (keyword arguments) are described where render() uses them."""

    def __init__(self, name, planes, ramp, **kw):
        self.name = name
        self.n = np.array([p[0] for p in planes])
        self.d = np.array([p[1] for p in planes])
        verts = kw.pop("verts", None)                   # given for solids of many planes (support_planes)
        self.verts = solid_vertices(self.n, self.d) if verts is None else np.asarray(verts, dtype=np.float64)
        self.ramp = ramp
        self.r0 = kw.pop("r0", np.eye(3))       # rest orientation (applied before the spin)
        self.bend = kw.pop("bend", None)
        self.axes = np.asarray(kw.pop("axes", (36.0, 36.0, 36.0)), dtype=np.float64)
        fp = kw.pop("fillet_planes", [])      # (normal, d) or (normal, d, width)
        self.fillet_n = np.array([p[0] for p in fp]).reshape(-1, 3)
        self.fillet_d = np.array([p[1] for p in fp])
        self.fillet_w = np.array([p[2] if len(p) > 2 else kw.get("fillet", 4.0) for p in fp])
        # the solid face each fillet plane belongs to: a face never rolls toward its own plane
        self.fillet_face = np.array([next((i for i in range(len(self.d)) if np.allclose(self.n[i], q[0], atol=1e-6)
                                           and abs(self.d[i] - q[1]) < 1e-6), -1) for q in fp], dtype=int)
        self.normal_fn = kw.pop("normal_fn", None)
        p = dict(
            # key light (direction override, strength, exponent, limb darkening) and fill light
            key=None, amb=0.1, kd1=0.3, q1=1.0, limb=1.0, smooth_limb=False, kd2=0.8, q2=3.0,
            bands=(),                                    # environment bands seen in reflections
            fillet=4.0, fillet_pow=1.5, fillet_max=1.0,  # rounded edges (see above)
            # see-through: weight (x facing^refr_face), index, back-facet shading, blur (model units)
            refr=0.0, ior=1.7, refr_amb=0.4, refr_k1=0.3, refr_k2=0.3, refr_bands=(), refr_blur=0.0,
            refr_face=1.0, refr_k3=0.0, refr_dir=(1, -1, 0),
            cushion=None, shade_fn=None, look=None,      # red: profile, own shading (see red_shade)
            tone=1.0, lift=0.0,                          # tone curve blend, final offset
            outline=1.3, step=1.0, step_dark=0.2,        # outline widths (model units), step colour
            glow_k=0.35, glow_band=0.25, glow_z=0.3)     # hover glow: circling light
        p.update(kw)
        self.p = p
        self._tone = None
        self.collect = None

    TONE_AT = (0.02, 0.10, 0.25, 0.50, 0.75, 0.90, 0.97, 0.995)
    TONE_TO = (0.00, 0.10, 0.25, 0.50, 0.75, 0.90, 0.97, 1.00)

    def tone_curve(self):
        """Map raw intensities onto the colour ramp so that, over the spin frames, the ramp's stops land
        on the luminance quantiles they stand for (q02 floor ... max): measure this gem's own raw
        intensity quantiles once (quick low-res renders), then interpolate. `tone` < 1 blends toward the
        identity."""
        if self._tone is None:
            self._tone = (np.array(self.TONE_TO), np.array(self.TONE_TO))
            if self.p["tone"] == 0:
                return self._tone
            self.collect = []
            for f in range(FRAMES):
                render(self, f * SPIN_STEP, 84, 1.0, (42.0, 42.0), ss=1)
            raw = np.quantile(np.concatenate(self.collect), self.TONE_AT)
            self.collect = None
            raw = np.maximum.accumulate(raw + np.arange(len(raw)) * 1e-6)
            k = self.p["tone"]
            to = np.array(self.TONE_TO)
            self._tone = (raw * k + to * (1 - k), to)    # np.interp clamps outside the measured range
        return self._tone

    def shading_normals(self, xm, face):
        nf = self.n[face]
        if self.normal_fn is not None:
            return self.normal_fn(self, xm, face, nf)
        n = nf
        if self.bend is not None:
            proxy = unit(xm / self.axes ** 2)
            n = unit(n + self.bend[face][:, None] * (proxy - n))
        if len(self.fillet_d):
            delta = self.fillet_d[None, :] - xm @ self.fillet_n.T          # depth below each table
            delta = np.where(self.fillet_face[None, :] == face[:, None], np.inf, delta)
            j = delta.argmin(1)
            w = np.clip(1 - delta[np.arange(len(j)), j] / self.fillet_w[j], 0, 1) ** self.p["fillet_pow"]
            w = w * self.p["fillet_max"]
            n = unit(n * (1 - w[:, None]) + self.fillet_n[j] * w[:, None])
        return n


# --- the seven gems ---------------------------------------------------------------------------------

def profile_plane(count, phase, r1, z1, r2, z2):
    """A ring of crown planes through the profile points (r1, z1) and (r2, z2) (distance from the
    axis, height); r1 > r2."""
    tilt = math.degrees(math.atan2(z2 - z1, r1 - r2))
    return ring(count, phase, tilt, r1, z1)


def gem_yellow():
    """Square step cut turned 45 degrees (a diamond outline), thick plate, two bevel steps."""
    g = 26.9                    # distance of the girdle edges from the centre (half-diagonal 38)
    planes = ring(4, 45, 90, g, 0)                       # girdle
    planes += ring(4, 0, 90, 36.0, 0)                    # small flats at the four tips
    outer = profile_plane(4, 45, g, 2.0, 18.3, 13.5)
    inner = profile_plane(4, 45, 18.3, 13.5, 14.8, 17.0)
    tips = ring(4, 0, 45, 35.6, 2.0)
    table = plane((0, 0, 1), (0, 0, 17.0))
    crown = outer + inner + tips + [table]
    planes += both_sides(crown)
    ramp = ramp8([(152, 152, 0), (154, 154, 0), (174, 174, 8), (223, 223, 52), (254, 254, 68),
                  (254, 254, 105), (254, 254, 155), (254, 254, 235)])
    fil = [(q[0], q[1], 2.2) for q in inner + outer] + [(table[0], table[1], 2.0)]
    bend = np.array([0.0] * 8 + ([0.6] * 4 + [0.0] * 4 + [0.3] * 4 + [0.0]) * 2)
    return Gem("yellow", planes, ramp, bend=bend, axes=(30, 30, 30),
               fillet_planes=fil + [(q[0] * [1, 1, -1], q[1], q[2]) for q in fil], fillet_pow=1.0, fillet_max=0.5,
               key=(-1, 1, 0.8), amb=0.05, kd1=0.55, q1=1.0, limb=0.3, kd2=0.65, q2=3.0,
               bands=(band(135, 65, 98, 12, 0.5, 0.75), band(-45, 60, 50, 20, 0.15)),
               refr=0.15, ior=1.6, refr_amb=0.30, refr_k1=0.3, refr_k2=0.4, refr_blur=1.2, lift=0.06)


def gem_white():
    """Truncated icosahedron (a football), 2-fold axis vertical, facets read as shallow dimples."""
    pts = []
    base = [(0, 1, 3 * PHI), (1, 2 + PHI, 2 * PHI), (PHI, 2, PHI ** 3)]
    for b in base:
        for sx in (1, -1):
            for sy in (1, -1):
                for sz in (1, -1):
                    v = (b[0] * sx, b[1] * sy, b[2] * sz)
                    for k in range(3):
                        pts.append(v[k:] + v[:k])
    pts = np.unique(np.round(np.array(pts, dtype=np.float64), 9), axis=0)
    pts *= 35.0 / np.linalg.norm(pts[0])
    planes = hull_planes(pts)
    bend = np.full(len(planes), -1.6)
    ramp = [(0.0, (76, 76, 76)), (0.10, (97, 97, 97)), (0.25, (153, 153, 153)), (0.47, (214, 214, 214)),
            (0.62, (255, 255, 255)), (1.0, (255, 255, 255))]
    return Gem("white", planes, ramp, bend=bend, axes=(35, 35, 35), r0=rot_y(8),
               key=(-0.6, 0.6, 1.0), amb=0.05, kd1=0.9, q1=1.0, limb=1.0, smooth_limb=True, kd2=0.55, q2=2.0,
               bands=(band(135, 60, 45, 30, 0.3),), lift=-0.04, outline=1.3, step=1.0, step_dark=0.5)


def gem_blue():
    """Round brilliant seen from the side: 8-fold, wide crown, long pavilion, girdle at 3/4 height."""
    planes = [plane((0, 1, 0), (0, 38.0, 0))]                         # table (edge-on)
    planes += ring(16, 11.25, 90, 36.0, 0, axis="y")                    # girdle
    planes += ring(8, 22.5, 47, 36.0, 20.0, axis="y")                   # crown (bezel) facets
    planes += ring(8, 0, 60, 34.6, 20.0, axis="y")                      # upper-girdle facets
    pav = 90 + math.degrees(math.atan2(36.0, 50.0))                     # girdle (r 36, y 15) to culet y -35
    planes += ring(8, 22.5, pav, 36.0, 15.0, axis="y")                  # pavilion mains
    planes += ring(16, 11.25, pav - 8, 36.0, 15.0, axis="y")            # lower-girdle facets
    ramp = ramp8([(0, 65, 152), (3, 77, 173), (13, 95, 203), (26, 121, 248), (36, 162, 254),
                  (70, 175, 254), (87, 201, 254), (114, 242, 254)])
    return Gem("blue", planes, ramp, amb=0.02, kd1=0.55, q1=2.0, limb=0.5, kd2=1.0, q2=4.0,
               bands=(band(135, 60, 50, 15, 0.45), band(-45, 60, 50, 15, 0.2), band(105, 45, 95, 16, 0.35)),
               refr=0.3, ior=1.8, refr_amb=0.30, refr_k1=0.25, refr_k2=0.45,
               refr_bands=(band(135, 60, 50, 15, 0.5),), refr_blur=0.8)


def cushion_normals(gem, xm, face, nf):
    """Red cushion as a height field: a rim around the outline, and inside it the table (z = Z) and four
    corner facets. The rim rises from the girdle (z = Z - H at the outline) to the table height at
    distance Wr inside it, along z = Z - H (1 - e / Wr)^beta (e = distance from the outline: steep at the
    rim, flat where it meets the table). Wr shrinks from its mid-side width (Wtl on the top and left,
    Wbr on the bottom and right) to We at the corners along exponent pw, so the side bevels are
    crescents (a pinched table); the chamfer rims have width Wch. A corner facet slopes gently down the
    diagonal from the table's corner cut |x| + |y| = D (drop Hc at the chamfer, exponent beta_c), inside
    the rim: a triangle between the table corner and the point where two crescents meet. The girdle band
    (|z| < G) keeps curving to 90 degrees at z = 0."""
    W, C, G, Z, H = 34.0, 6.0, 10.0, 24.0, 14.0
    Wtl, Wbr, We, pw, beta, D, Hc, beta_c, Wch = gem.p["cushion"]
    x, y, z = xm[:, 0], xm[:, 1], xm[:, 2]
    # the back mirrors the front left-right as well, so a half turn shows the same face (seamless loop)
    x = np.where(z < 0, -x, x)
    ax, ay = np.abs(x), np.abs(y)
    sx, sy = np.where(x < 0, -1.0, 1.0), np.where(y < 0, -1.0, 1.0)
    r2 = math.sqrt(2.0)
    Lc = W - C
    d = (ax + ay) / r2
    dc0, dc1 = D / r2, (2 * W - C) / r2

    def rim(e, wr):
        q = np.clip(1 - e / wr, 0, 1)
        return Z - H * q ** beta, H * beta * q ** (beta - 1) / wr

    wx, wy = np.where(x < 0, Wtl, Wbr), np.where(y > 0, Wtl, Wbr)
    hx, kx = rim(W - ax, wx - (wx - We) * np.clip(ay / Lc, 0, 1) ** pw)
    hy, ky = rim(W - ay, wy - (wy - We) * np.clip(ax / Lc, 0, 1) ** pw)
    hch, kch = rim(dc1 - d, Wch)
    sc = np.clip((d - dc0) / (dc1 - dc0), 0, 1)
    kc = np.where(sc > 0, Hc * beta_c * np.maximum(sc, 1e-9) ** (beta_c - 1) / (dc1 - dc0), 0.0)
    # the rim wins wherever it has started to fall; inside it, the corner facets (flat on the table)
    j = np.stack([hx, hy, hch], 1).argmin(1)
    on_rim = np.stack([hx, hy, hch], 1).min(1) < Z - 1e-9
    j = np.where(on_rim, j, 3)
    slope = np.select([j == 0, j == 1, j == 2], [kx, ky, kch], kc)
    gx = np.select([j == 0, j == 1], [sx, 0 * sx], sx / r2)
    gy = np.select([j == 0, j == 1], [0 * sy, sy], sy / r2)
    th = np.arctan(slope)
    # the girdle band: from the rim tilt at |z| = G on to 90 degrees at z = 0 (a barrel)
    th_rim = np.arctan(H * beta / np.select([j == 0, j == 1], [wx, wy], Wch))
    on_band = np.abs(z) < G - 1e-6
    th = np.where(on_band, th_rim + (np.pi / 2 - th_rim) * (1 - np.abs(z) / G), th)
    sgn = np.where(z < 0, -1.0, 1.0)
    return unit(np.stack([sgn * gx * np.sin(th), gy * np.sin(th), sgn * np.cos(th)], axis=1))


def red_shade(gem, n, nm, xm):
    """The red's own shading, straight in colour-ramp units (0 = the dark floor ... 1 = the highlight).
    The terms look up the view-space normal n (tilt th from the view axis, screen azimuth ph); some are
    limited to parts of the cut by the face tilt thf, the tilt of the model-space normal nm from the axis
    of its own table (the back is the front turned half a turn): the tables are thf = 0, the girdle band
    90. At rest th = thf on the front. a = +1 for normals toward the upper-left ... -1 lower-right;
    b = +-1 for upper-right / lower-left. Terms (keys of the gem's `look`):
      table          level of front-facing surfaces;
      grad           (kx, ky, r): the tables brighten toward their left and top edges (at the hit point
                     xm, model space; the back's left is the front's right);
      streak         a hard bright band where a surface facing upper-left is tilted about th_s;
      sheen          (k, w, m0, m1): a broad glossy band where the bevels and the girdle face the eye
                     (view tilt within about w, face tilt m0..m1): the capsule look of the half turn;
      ul_dark        (k, t0, t1, t2): surfaces facing upper-left fall to the floor as they steepen
                     from t0 to t1 (the dark top/left rim); only the bevels (not the tables, which
                     face sideways in the half turn, nor the girdle band, t2 .. 90);
      lr_facet       a lift for gently tilted lower-right surfaces (the lit lower-right corner facet);
      lr_line        a dip where the right and bottom bevels leave the table (a dark edge line);
      diag           gently tilted surfaces facing upper-right / lower-left read dark (corner facets);
      rim            everything darkens toward the silhouette;
      cap            the brightest value (a flat, pale plateau rather than white)."""
    L = gem.p["look"]

    def tilt_dir(v):
        th = np.degrees(np.arccos(np.clip(v[:, 2], -1, 1)))
        ph = np.radians(np.degrees(np.arctan2(v[:, 1] + 0.0, v[:, 0] + 0.0)) - 135.0)
        # the azimuth means nothing on surfaces facing straight out: fade the direction terms in
        w = smoothstep(0.5, 4.0, th)
        return th, np.cos(ph) * w, np.sin(ph) * w

    th, a, b = tilt_dir(n)
    f = np.where((nm[:, 2] < 0)[:, None], nm * [-1.0, 1.0, -1.0], nm)
    thf = tilt_dir(f)[0]
    table = 1 - smoothstep(4, 20, thf)
    fx = np.where(nm[:, 2] < 0, -xm[:, 0], xm[:, 0])
    gx, gy, gr = L["grad"]
    i = L["table"] + (gx * np.clip(-fx / gr, -1, 1) + gy * np.clip(xm[:, 1] / gr, -1, 1)) * table
    # the terms for the bevels and corner facets leave the flat tables alone, which turn through their
    # tilts in the half turn
    facet = smoothstep(0.5, 2.0, thf)
    sk, ts, sw = L["streak"]
    i += sk * np.exp(-0.5 * ((th - ts) / sw) ** 2) * smoothstep(-0.1, 0.7, a) * facet
    hk, hw, h0, h1 = L["sheen"]
    i += hk * np.exp(-0.5 * (th / hw) ** 2) * smoothstep(h0, h1, thf)
    dk, d0, d1, d2 = L["ul_dark"]
    bevel = smoothstep(8.0, 18.0, thf) * (1 - smoothstep(d2, 90.0, thf))
    i -= dk * smoothstep(d0, d1, th) * smoothstep(-0.4, 0.5, a) * bevel
    fk, ft, fw = L["lr_facet"]
    i += fk * np.exp(-0.5 * ((th - ft) / fw) ** 2) * smoothstep(-0.85, -0.97, a) * facet
    ek, et, ew = L["lr_line"]
    i -= (ek * np.exp(-0.5 * ((th - et) / ew) ** 2) * smoothstep(-0.3, -0.6, a)
          * (1 - smoothstep(-0.85, -0.97, a)) * facet)
    ck, cp, c0, c1 = L["diag"]
    i -= ck * np.abs(b) ** cp * smoothstep(c0, c1, th) * (1 - smoothstep(30, 50, th)) * facet
    rk, r0, r1 = L["rim"]
    i -= rk * smoothstep(r0, r1, th)
    return np.minimum(i, L["cap"])


def loft_points(W, C, rings, count=720):
    """Outline curves of a cushion built as the convex hull of horizontal rings (a loft): ring
    (z, inset, a, p) is the square outline of half-width W with chamfered corners (C), moved `inset`
    inwards, and cut by the superellipse |x/a|^p + |y/a|^p = 1 (a = 0: no cut), which rounds it. The
    back mirrors the front. The hull of all rings is the solid (support_planes)."""
    phi = np.linspace(0, 2 * math.pi, count, endpoint=False)
    c, s = np.cos(phi), np.sin(phi)
    # edges of the outline: distance from the centre and direction of the outward normal
    edges = [(W, k * 90.0) for k in range(4)] + [((2 * W - C) / math.sqrt(2), 45.0 + k * 90.0) for k in range(4)]
    pts = []
    for z, inset, a, p in rings:
        r = np.full(count, np.inf)
        for dist, ang in edges:
            cosd = np.cos(phi - math.radians(ang))
            r = np.where(cosd > 1e-9, np.minimum(r, (dist - inset) / np.maximum(cosd, 1e-9)), r)
        if a > 0:
            r = np.minimum(r, a / (np.abs(c) ** p + np.abs(s) ** p) ** (1 / p))
        for zz in {z, -z}:
            pts.append(np.stack([r * c, r * s, np.full(count, zz)], 1))
    return np.concatenate(pts)


# the red's loft (z, inset, superellipse a, p): the girdle outline (frame 0's silhouette), its top edge
# with rounded corners, a rounder mid-bevel ring and the table, a rounded square of half-width 20
RED_LOFT = ((0.0, 0.0, 0.0, 2.0), (8.0, 0.5, 35.0, 4.0), (17.0, 7.5, 29.0, 3.0), (24.0, 14.0, 21.0, 3.0))


def gem_red(rings=RED_LOFT):
    """Thick square cushion with chamfered corners: crescent bevels around a pinched table, corner facets
    (pale upper-left, dark upper-right and lower-left, light lower-right). Its own shading (red_shade).
    The solid is a loft of rounded rings (loft_points: the outline at the girdle, rounder squares up to
    the table), so the half turn shows round, capsule-like silhouettes; the crescents and corner facets
    are in the shading normals (cushion_normals)."""
    W, c = 34.0, 6.0
    planes, verts = support_planes(loft_points(W, c, rings), 900)
    ramp = ramp8([(152, 0, 0), (178, 11, 11), (234, 36, 36), (254, 78, 78), (254, 124, 124),
                  (254, 169, 169), (254, 189, 189), (254, 241, 241)])
    look = dict(table=0.61, grad=(0.18, 0.08, 20.0), streak=(0.40, 8.0, 5.0), sheen=(0.3, 14.0, 30.0, 50.0),
                ul_dark=(0.62, 22.0, 55.0, 72.0), lr_facet=(0.22, 13.0, 6.0), lr_line=(0.25, 7.0, 4.0),
                diag=(0.42, 4, 4.0, 10.0), rim=(0.6, 45.0, 80.0), cap=0.97)
    return Gem("red", planes, ramp, verts=verts, normal_fn=cushion_normals, shade_fn=red_shade, look=look,
               tone=0.0,
               # rims 16 (top, left) / 12 (bottom, right) wide at mid-side, 7 at the corners, chamfer rims 6;
               # table corner cut |x| + |y| = 28, corner facets drop 6 (exponent 1.3)
               cushion=(16.0, 12.0, 7.0, 1.5, 2.0, 28.0, 6.0, 1.3, 6.0))


def gem_purple():
    """Triangular step cut, apex up, taller than equilateral."""
    side_n = unit((-67.0, 32.0, 0.0))
    planes = [plane(side_n, (-34, -34, 0)), plane(side_n * [-1, 1, 1], (34, -34, 0)),
              plane((0, -1, 0), (0, -34, 0)), plane((0, 1, 0), (0, 33, 0))]
    crown = []
    for n2, p_girdle, p_table in ((side_n, (-34, -34), (-16, -20)), (side_n * [-1, 1, 1], (34, -34), (16, -20)),
                                  (np.array([0.0, -1.0, 0.0]), (0, -34), (0, -20))):
        n2 = np.asarray(n2, dtype=np.float64)
        dg = n2[0] * p_girdle[0] + n2[1] * p_girdle[1]
        dt = n2[0] * p_table[0] + n2[1] * p_table[1]
        tilt = math.degrees(math.atan2(dg - dt, 10.0))
        nn = np.array([n2[0] * math.sin(math.radians(tilt)), n2[1] * math.sin(math.radians(tilt)),
                       math.cos(math.radians(tilt))])
        crown.append(plane(nn, (n2[0] * dg, n2[1] * dg, 7.0)))
    crown.append(plane((0, math.sin(math.radians(64)), math.cos(math.radians(64))), (0, 33.0, 7.0)))
    table = plane((0, 0, 1), (0, 0, 17.0))
    planes += both_sides(crown + [table])
    bend = np.array([0.0] * 4 + [0.3, 0.3, 0.3, 0.2, 0.0] * 2)
    ramp = ramp8([(152, 0, 152), (152, 0, 152), (163, 4, 163), (204, 25, 204), (254, 46, 254),
                  (254, 82, 254), (254, 115, 254), (254, 141, 254)])
    return Gem("purple", planes, ramp, bend=bend, axes=(26, 30, 26),
               fillet_planes=[table, (table[0] * [1, 1, -1], table[1])], fillet=4.0,
               key=(-1, 0.8, 0.6), amb=0.05, kd1=0.85, q1=2.0, limb=0.4, kd2=1.1, q2=4.0,
               bands=(band(0, 180, 0, 16, 0.2), band(135, 60, 48, 12, 0.35), band(-45, 60, 50, 15, 0.2)),
               refr=0.2, ior=1.6, refr_blur=1.2)


def gem_orange():
    """Regular icosahedron (a d20), 2-fold axes along x, y and z: pointy-top hexagon at rest."""
    u = 19.6
    pts = []
    for a in (1, -1):
        for b in (1, -1):
            pts += [(a * u, 0, b * PHI * u), (a * PHI * u, b * u, 0), (0, a * PHI * u, b * u)]
    planes = hull_planes(pts)
    ramp = [(0.0, (152, 66, 0)), (0.12, (178, 79, 4)), (0.28, (227, 103, 13)), (0.50, (254, 151, 41)),
            (0.72, (254, 195, 62)), (0.86, (254, 243, 116)), (0.95, (254, 254, 131)), (1.0, (254, 254, 167))]
    return Gem("orange", planes, ramp, key=(-0.3, 1.0, 0.9), amb=0.02, kd1=0.85, q1=2.0, limb=0.4, kd2=0.85, q2=5.0,
               bands=(band(120, 60, 40, 15, 0.35), band(-45, 30, 45, 15, 0.3)),
               refr=0.6, ior=2.0, refr_amb=-0.1, refr_k1=0.5, refr_k2=0.8,
               refr_bands=(band(135, 60, 45, 18, 0.4),), refr_blur=1.0, refr_k3=0.7,
               refr_dir=(0.8, -0.5, -0.3), lift=-0.07)


def gem_green():
    """Truncated octahedron turning about a 4-fold axis: octagon outline, diamond-shaped table."""
    s = 16.5
    pts = set()
    for perm in itertools.permutations((0, 1, 2)):
        for sa in (1, -1):
            for sb in (1, -1):
                v = [0, 0, 0]
                v[perm[1]] = sa * s
                v[perm[2]] = sb * 2 * s
                pts.add(tuple(v))
    planes = hull_planes(sorted(pts))
    squares = [(n, d) for n, d in planes if np.abs(n).max() > 0.99]
    # highlights stay mint (the rest pose's brightest pixels are about (165, 254, 165)), never white
    ramp = ramp8([(0, 152, 0), (1, 157, 1), (8, 173, 8), (35, 213, 35), (64, 254, 64), (108, 254, 108),
                  (165, 254, 165), (215, 254, 215)])
    bend = np.array([0.0 if np.abs(n).max() > 0.99 else 0.35 for n, _ in planes])
    return Gem("green", planes, ramp, bend=bend, axes=(33, 33, 33),
               fillet_planes=squares, fillet=13.0, fillet_pow=1.0,
               key=(-1, 1, 0.8), amb=0.12, kd1=0.60, q1=1.0, limb=0.6, kd2=0.6, q2=3.0,
               bands=(band(135, 60, 48, 30, 0.6, 0.8), band(-45, 60, 50, 15, 0.1)),
               refr=0.2, ior=1.6, refr_blur=1.2, lift=0.06, glow_k=0.15, glow_band=0.15)


def all_gems():
    return [gem_yellow(), gem_white(), gem_blue(), gem_red(), gem_purple(), gem_orange(), gem_green()]


# --- renderer -----------------------------------------------------------------------------------------

def box_blur(a, r):
    """Separable box blur (two passes ~ Gaussian) of a 2D array, radius r pixels."""
    if r < 1:
        return a
    r = int(round(r))
    k = 2 * r + 1
    for _ in range(2):
        for axis in (0, 1):
            p = np.pad(a, [(r, r) if i == axis else (0, 0) for i in range(2)], mode="edge")
            c = np.cumsum(p, axis=axis)
            c = np.concatenate([np.zeros_like(np.take(c, [0], axis=axis)), c], axis=axis)
            a = (np.take(c, range(k, c.shape[axis]), axis=axis)
                 - np.take(c, range(0, c.shape[axis] - k), axis=axis)) / k
    return a


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


def render(gem, spin, size, unit_px, origin, ss=SS, glow=None, pose=None):
    """Render `gem` turned `spin` degrees about the vertical axis into a (size x size) RGBA float image
    (straight alpha). unit_px: output pixels per model unit. origin: output pixel position (x right,
    y down, continuous) of the model's image-plane point (0, 0). glow: azimuth (degrees, 90 = from the
    top, counter-clockwise) of the extra hover light, or None. pose: a further rotation (view space)."""
    p = gem.p
    rot = orientation(gem, spin, pose)
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
    out = np.zeros((n_px, n_px, 4))
    if len(cols) == 0 or len(rows) == 0:
        return downsample(out, ss)
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

    rgb = np.zeros((len(sx), 3))
    h = np.nonzero(hit)[0]
    if len(h):
        dh = dirs[h]
        xh = eye + t_hit[h, None] * dh
        fh = face[h]
        xm = xh @ rot                       # back to model space
        nm = gem.shading_normals(xm, fh)      # model space
        n = nm @ rot.T
        v = -dh
        ndv = np.clip((n * v).sum(1), 0.0, 1.0)
        refl = 2 * ndv[:, None] * n - v
        # limb darkening from the facet normal, or (smooth_limb) from the proxy ellipsoid's normal
        limb_n = unit(xm / gem.axes ** 2) @ rot.T if p["smooth_limb"] else n
        limb = np.clip((limb_n * v).sum(1), 0.0, 1.0) ** p["limb"]
        key = unit(p["key"]) if p["key"] is not None else KEY
        if p["shade_fn"] is not None:
            inten = p["shade_fn"](gem, n, nm, xm)
        else:
            inten = (p["amb"] + p["kd1"] * np.clip(n @ key, 0, None) ** p["q1"] * limb
                     + p["kd2"] * np.clip(n @ FILL, 0, None) ** p["q2"] + bands_at(p["bands"], refl))
        if glow is not None:
            a = math.radians(glow)
            lg = unit((math.cos(a), math.sin(a), p["glow_z"]))
            inten += (p["glow_k"] * np.clip(n @ lg, 0, None) ** 3
                      + bands_at([band(glow, 45, 60, 22, p["glow_band"])], refl))
        if p["refr"] > 0:
            nf = nv[fh]
            eta = 1.0 / p["ior"]
            cosi = -(dh * nf).sum(1)
            k = 1 - eta * eta * (1 - cosi * cosi)
            din = unit(eta * dh + (eta * cosi - np.sqrt(np.clip(k, 0, None)))[:, None] * nf)
            den2 = din @ nv.T
            with np.errstate(divide="ignore", invalid="ignore"):
                t2 = (d - xh @ nv.T) / den2
            t2 = np.where(den2 > 1e-9, t2, np.inf)
            nb = nv[t2.argmin(1)]
            r2 = din - 2 * (din * nb).sum(1)[:, None] * nb
            inner = (p["refr_amb"] + p["refr_k1"] * np.clip(-(nb @ KEY), 0, None)
                     + p["refr_k2"] * np.clip(-(nb @ FILL), 0, None) + bands_at(p["refr_bands"], -r2))
            if p["refr_k3"]:
                inner = inner + p["refr_k3"] * (nb @ unit(p["refr_dir"]))
            if p["refr_blur"] > 0:
                img = np.zeros(len(sx))
                img[h] = inner
                wgt = np.zeros(len(sx))
                wgt[h] = 1
                r = p["refr_blur"] * unit_px * ss
                img = box_blur(img.reshape(gx.shape), r).ravel()
                wgt = box_blur(wgt.reshape(gx.shape), r).ravel()
                inner = img[h] / np.maximum(wgt[h], 1e-6)
            wr = p["refr"] * ndv ** p["refr_face"]
            inten = inten * (1 - wr) + inner * wr
        if gem.collect is not None:
            gem.collect.append(inten[ndv > 0.15])
        inten = np.clip(np.interp(inten, *gem.tone_curve()) + p["lift"], 0, 1)
        rgb[h] = ramp_colour(gem.ramp, inten)

    # outline: black, then a dark step into the body colour
    floor = np.array(gem.ramp[0][1]) / 255.0
    w0, w1 = p["outline"], p["step"]
    t_step = np.clip((dist - w0) / w1, 0, 1)[:, None]
    stepped = floor * p["step_dark"] * (1 - t_step) + rgb * t_step
    rgb = np.where((dist < w0)[:, None], 0.0, stepped)
    tile = np.zeros((len(rows), len(cols), 4))
    tile[..., :3] = rgb.reshape(len(rows), len(cols), 3)
    tile[..., 3] = inside.reshape(len(rows), len(cols))
    out[rows[0]:rows[-1] + 1, cols[0]:cols[-1] + 1] = tile
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


def render_cell(gem, spin=0.0, glow=None, scale=SCALE, ss=SS):
    unit_px = UNIT_PX * scale
    origin = gem_origin(gem, CELL * scale, unit_px, (ANCHOR[0] * scale, ANCHOR[1] * scale))
    img = render(gem, spin, CELL * scale, unit_px, origin, ss=ss, glow=glow)
    # the game samples x 2..50, y 0..48 of a cell: keep the gutter empty
    img[:, :2 * scale] = 0
    img[:, 51 * scale:] = 0
    img[49 * scale:] = 0
    return img


def make_sheet(gems=None, scale=SCALE, ss=SS, log=None):
    gems = gems or all_gems()
    c = CELL * scale
    sheet = np.zeros((2 * len(gems) * c, FRAMES * c, 4))
    for gi, gem in enumerate(gems):
        for f in range(FRAMES):
            sheet[2 * gi * c:(2 * gi + 1) * c, f * c:(f + 1) * c] = render_cell(gem, spin=f * SPIN_STEP,
                                                                                 scale=scale, ss=ss)
            sheet[(2 * gi + 1) * c:(2 * gi + 2) * c, f * c:(f + 1) * c] = render_cell(
                gem, glow=90.0 + f * GLOW_STEP, scale=scale, ss=ss)
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
# is read from the picture: places, sizes and colours below are hand-set constants (each gem's
# bounding box matches the old gem's outline, glows excluded).
TITLE_TARGET = "images/titlescreen6"
TITLE_RECT = (160, 146, 292, 120)        # x, y, width, height of the patch (title pixels)
# back to front: colour index, centre of the bounding box (title pixels), title pixels per model unit,
# spin, tilt (top toward the viewer) and roll (counter-clockwise), degrees
TITLE_GEMS = (
    (2, (211.0, 204.5), 0.93, 0.0, 0.0, 0.0),     # blue brilliant
    (6, (261.5, 219.0), 0.90, 0.0, 0.0, 0.0),     # green octagon
    (5, (357.5, 219.5), 1.06, 0.0, 0.0, 0.0),     # orange d20
    (3, (402.75, 199.5), 0.915, 0.0, 0.0, 0.0),   # red cushion
    (4, (310.0, 188.5), 0.97, 0.0, 0.0, 0.0),     # purple triangle
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


def render_title(scale=SCALE, ss=SS, log=None):
    """The title patch: an RGBA image of TITLE_RECT at `scale` (see TITLE_TARGET)."""
    gems = all_gems()
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
    for ci, (cx, cy), upx, spin, tilt, roll in TITLE_GEMS:
        gem = gems[ci]
        pose = rot_z(roll) @ rot_x(tilt)
        hull = project(gem, orientation(gem, spin, pose))
        bx, by = (hull.min(0) + hull.max(0)) / 2
        ox, oy = cx - bx * upx, cy + by * upx                  # title position of the model's origin
        dist = polygon_distance(np.stack([ox + hull[:, 0] * upx, oy - hull[:, 1] * upx], 1), X, Y)
        near = np.minimum(near, dist)
        out = np.clip(dist, 0, None)
        tint = ramp_colour(gem.ramp, np.array(0.75))
        rgb += ((kh * np.exp(-out / sh))[..., None] * haze + (kc * np.exp(-out / sc))[..., None] * tint
                + (kw * np.exp(-out / sw))[..., None])
        # the gem itself, rendered into a square tile around its origin
        size = int(math.ceil(np.abs(hull).max() * 2 * upx * scale)) + 8
        opx, opy = (ox - x0) * scale, (oy - y0) * scale
        tx, ty = int(math.floor(opx - size / 2)), int(math.floor(opy - size / 2))
        layers.append((render(gem, spin, size, upx * scale, (opx - tx, opy - ty), ss=ss, pose=pose), tx, ty))
        if log:
            log(f"  title {gem.name}")
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


def main():
    here = os.path.dirname(os.path.abspath(__file__))
    ap = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    ap.add_argument("--out", default=os.path.join(here, "..", "mods"))
    ap.add_argument("--no-title", action="store_true", help="leave the title screen's gems as they are")
    ap.add_argument("--preview", help="also write a preview PNG (the sheet on a dark background, @4x and 1x)")
    args = ap.parse_args()
    mod = os.path.join(os.path.abspath(args.out), "bejeweled2-gems")
    patch_rel = "patches/bej2-gems@4x.png"
    title_rel = "patches/bej2-title@4x.png"
    print("rendering")
    sheet = make_sheet(log=print)
    assert sheet.size == (FRAMES * CELL * SCALE, 14 * CELL * SCALE)
    path = os.path.join(mod, *patch_rel.split("/"))
    os.makedirs(os.path.dirname(path), exist_ok=True)
    sheet.save(path, optimize=True)
    patches = [{"target": "images/gemsheet6", "image": patch_rel, "x": 0, "y": 0, "mode": "replace"}]
    title_path = os.path.join(mod, *title_rel.split("/"))
    if args.no_title:
        if os.path.exists(title_path):
            os.remove(title_path)
    else:
        render_title(log=print).save(title_path, optimize=True)
        patches.append({"target": TITLE_TARGET, "image": title_rel, "x": TITLE_RECT[0], "y": TITLE_RECT[1],
                        "mode": "over"})
    with open(os.path.join(mod, "mod.json"), "w", encoding="utf-8") as f:
        json.dump({
            "id": "bejeweled2-gems",
            "name": "Bejeweled 2 Gems",
            "version": "1.0",
            "author": "byo-pop",
            "description": "The seven gems redrawn after Bejeweled 2 (new procedural art, close recreations): "
                           "lemon step cut, white football, azure brilliant, coral cushion, magenta triangle, "
                           "orange d20 and green truncated octahedron. HD (@4x); spin and hover animations"
                           + ("." if args.no_title else "; the same gems on the title screen."),
            "imagePatches": patches,
        }, f, indent=2)
        f.write("\n")
    print(f"wrote {mod}")
    if args.preview:
        one = box_down(sheet, SCALE)
        bg = Image.new("RGBA", (sheet.width, sheet.height + one.height * 2), (34, 30, 48, 255))
        bg.alpha_composite(sheet)
        bg.alpha_composite(one.resize((one.width * 2, one.height * 2), Image.NEAREST), (0, sheet.height))
        bg.convert("RGB").save(args.preview)
        print(f"preview {args.preview}")


if __name__ == "__main__":
    main()
