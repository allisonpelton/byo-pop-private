#!/usr/bin/env python3
"""Generate Big Money's example mods. All of their art and text is original: drawn here from shapes and
written here, with nothing taken from the game's files, so the output is committed with the source.

    python games/bigmoney/tools/make_example_mods.py [--out <mods folder, default games/bigmoney/mods>]

Creates, in games/bigmoney/mods/:
  fake-money/   "Fake Money! Deluxe": crypto coins, a "Fake money!" logo and crypto rank titles.
      images/horzcoins@2x.png   the coin flip sheet: 16 frames (columns) x 5 colours (rows) of 29x29 cells,
                                at 2x. Frame f shows the coin turned by f * 180/16 degrees about its
                                vertical axis; the game plays them when coins are minted.
      patches/coinfaces@2x.png  column 0 of images/coins (the resting coin of each colour), pasted over the
                                game's sheet by an "imagePatches" entry. The sheet's other 19 columns are
                                the coloured blobs of the vanish animation and stay as they are.
      images/bigmoneylogo@2x.png, images/floatlogo@2x.png
                                the title screen logo (472x239) and the small logo in the top left corner
                                of the game screens (156x76)
      properties/fakemoney.xml  the 27 rank titles of the stats panel (gRankNames, same score thresholds)
                                and the texts that name the game

The coin rows keep the game's colours (red, blue, yellow, green, purple), so groups read the same; each
gets a symbol in the spirit of a cryptocurrency mark, drawn from simple shapes: a tilted B with bars (red
would be wrong for it, so it is the yellow coin), a stacked diamond (blue), a D with a bar (red), a T with
a ring (green) and three slanted bars (purple). The coin is shaded from a height map (raised rim, slight
dome, the symbol engraved), lit from the top left like the game's coins.

The logo uses a block font defined below on a grid of cells, in the style of the game's logo: bright green
letters with a black outline and drop, in a yellow-green glow.

Needs Pillow and numpy.
"""
import argparse
import json
import math
import os

import numpy as np
from PIL import Image, ImageDraw, ImageFilter

SCALE = 2           # the mod's images are @2x
SS = 4              # supersampling for the drawing
CELL = 29           # coin cell size in the game's sheets (1x)
COIN_D = 28.4       # coin diameter (1x); the game's coins fill their cells with a soft edge
EDGE_T = 5.0        # coin thickness (1x) seen edge-on
FLIP_FRAMES = 16    # images/horzcoins: 464 / 29

# Per coin row: the game's colour (dark, mid, light), measured from the colours the rows show in play, and
# the symbol
COINS = [
    ("red", (168, 3, 3), (230, 23, 23), (250, 140, 140), "doge"),
    ("blue", (6, 89, 168), (24, 122, 231), (150, 236, 250), "diamond"),
    ("yellow", (167, 153, 8), (234, 213, 23), (252, 250, 170), "bitcoin"),
    ("green", (6, 168, 6), (21, 232, 21), (170, 250, 170), "tether"),
    ("purple", (156, 7, 156), (232, 21, 232), (250, 175, 250), "bars"),
]

# The 27 ranks (score thresholds 0, 1000, ... 500000 as in the game). The stats panel's font has letters,
# digits, space and ! ? . , + only, and about 20 characters fit.
RANK_TITLES = [
    "Nocoiner", "Faucet Dripper", "Airdrop Hunter", "Gas Fee Victim", "Paper Hands", "Bag Holder",
    "Dip Buyer", "Memecoin Minnow", "Fomo Trader", "Rug Pull Survivor", "Moon Boy", "Degen Ape", "Shiller",
    "Hodler", "Yield Farmer", "Node Runner", "Miner", "Validator", "Diamond Hands", "Influencer",
    "Crypto Bro", "Exchange Founder", "Whale", "Mega Whale", "Lambo Owner", "Island Owner",
    "Probably Satoshi",
]

# Texts that name the game (properties/default.xml ids); ^RRGGBB^ sets the colour, as in the game's texts
STRINGS = {
    "BIG_MONEY_COMBO": "Fake Money Combo!",
    "HOW_TO_PLAY_BIG_MONEY_PUZZLE": "^FFFF51^How to play Fake Money: Puzzle",
    "X_HOW_TO_PLAY_BIG_MONEY_ACTION": "^FFFF51^How to play Fake Money: Action",
    "X_HOW_TO_PLAY_BIG_MONEY_STRATEGY": "^FFFF51^How to play Fake Money: Strategy",
}


# ---------------------------------------------------------------------------------------------------------
# Coins

def symbol_mask(kind, size):
    """The symbol as an anti-aliased mask (float 0..1), size x size, drawn in a [-1, 1] box."""
    big = size * 4
    im = Image.new("L", (big, big), 0)
    d = ImageDraw.Draw(im)

    def P(x, y):
        return ((x + 1) * big / 2, (y + 1) * big / 2)

    def rect(x0, y0, x1, y1, fill=255):
        d.rectangle([P(x0, y0), P(x1, y1)], fill=fill)

    def bowl(x0, y0, x1, y1, stroke):
        # a "D" bowl: a straight part from the stem, then the right half of an ellipse, counter cut out
        rx = min((y1 - y0) / 2, (x1 - x0) * 0.6)
        rect(x0, y0, x1 - rx, y1)
        d.pieslice([P(x1 - 2 * rx, y0), P(x1, y1)], -90, 90, fill=255)
        rect(x0, y0 + stroke, x1 - rx, y1 - stroke, 0)
        d.pieslice([P(x1 - 2 * rx + stroke, y0 + stroke), P(x1 - stroke, y1 - stroke)], -90, 90, fill=0)

    if kind == "bitcoin":
        # B with two bars above and below, tilted like a well-known coin's mark
        s = 0.17
        rect(-0.42, -0.62, -0.42 + s, 0.62)
        bowl(-0.42, -0.62, 0.30, 0.02, s)
        bowl(-0.42, -0.02, 0.40, 0.62, s)
        rect(-0.42, -0.62, -0.42 + s, 0.62)
        for x in (-0.24, 0.02):
            rect(x, -0.86, x + 0.11, -0.55)
            rect(x, 0.55, x + 0.11, 0.86)
        im = im.rotate(-14, resample=Image.BICUBIC)
    elif kind == "doge":
        # D with a bar across its stem
        s = 0.18
        bowl(-0.32, -0.66, 0.52, 0.66, s)
        rect(-0.32, -0.66, -0.32 + s, 0.66)
        rect(-0.58, -0.08, 0.02, 0.08)
    elif kind == "diamond":
        # a tall diamond over a chevron, split by a crease
        d.polygon([P(0, -0.92), P(0.52, 0.08), P(0, 0.36), P(-0.52, 0.08)], fill=255)
        d.polygon([P(-0.52, 0.22), P(0, 0.5), P(0.52, 0.22), P(0, 0.92)], fill=255)
        d.line([P(0, -0.92), P(0, 0.36)], fill=0, width=max(1, big // 40))
    elif kind == "tether":
        # T with a flat ring around the stem
        rect(-0.6, -0.66, 0.6, -0.42)
        rect(-0.13, -0.66, 0.13, 0.74)
        ring = Image.new("L", (big, big), 0)
        rd = ImageDraw.Draw(ring)
        rd.ellipse([P(-0.7, -0.24), P(0.7, 0.14)], outline=255, width=max(1, int(big * 0.055)))
        im = Image.fromarray(np.maximum(np.asarray(im), np.asarray(ring)))
    elif kind == "bars":
        # three slanted bars, the middle one slanted the other way
        k = 0.2
        for y, flip in ((-0.56, False), (-0.11, True), (0.34, False)):
            h = 0.24
            if not flip:
                d.polygon([P(-0.5 + k, y), P(0.66, y), P(0.5 - k + 0.0, y + h), P(-0.66, y + h)], fill=255)
            else:
                d.polygon([P(-0.66, y), P(0.5 - k, y), P(0.66, y + h), P(-0.5 + k, y + h)], fill=255)
    else:
        raise ValueError(kind)
    return np.asarray(im.resize((size, size), Image.LANCZOS)).astype(np.float64) / 255.0


def blur(a, radius):
    im = Image.fromarray(np.clip(a * 255.0, 0, 255).astype(np.uint8))
    return np.asarray(im.filter(ImageFilter.GaussianBlur(radius))).astype(np.float64) / 255.0


def shade(dark, mid, light, s):
    """Colour for shading s (0..1): dark -> mid -> light."""
    dark, mid, light = (np.array(c, dtype=np.float64) for c in (dark, mid, light))
    s = np.clip(s, 0.0, 1.0)[..., None]
    lo = dark + (mid - dark) * np.clip(s / 0.55, 0, 1)
    hi = mid + (light - mid) * np.clip((s - 0.55) / 0.45, 0, 1)
    return np.where(s < 0.55, lo, hi)


def coin_face(coin, size):
    """The coin seen face-on: RGBA float array size x size (supersampled; alpha 0..1)."""
    _, dark, mid, light, kind = coin
    c = (size - 1) / 2.0
    radius = size / 2.0
    y, x = np.mgrid[0:size, 0:size].astype(np.float64)
    u, v = (x - c) / radius, (y - c) / radius
    r = np.sqrt(u * u + v * v)

    # Height: slight dome, raised rim, engraved symbol
    h = 0.12 * (1.0 - np.clip(r, 0, 1) ** 2)
    rim = (r > 0.78) & (r < 1.0)
    h = h + np.where(rim, 0.2 * np.sin(np.pi * (r - 0.78) / 0.22), 0.0)
    sym_size = int(size * 0.62)
    sym = np.zeros((size, size))
    o = (size - sym_size) // 2
    sym[o:o + sym_size, o:o + sym_size] = symbol_mask(kind, sym_size)
    h = h - 0.07 * blur(sym, size / 120.0)

    # Light from the top left
    gy, gx = np.gradient(h * size * 0.5)
    n = np.dstack([-gx, -gy, np.ones_like(h)])
    n /= np.linalg.norm(n, axis=2, keepdims=True)
    L = np.array([-0.5, -0.62, 0.6])
    L /= np.linalg.norm(L)
    diffuse = np.clip((n * L).sum(axis=2), 0, 1)
    H = L + np.array([0, 0, 1.0])
    H /= np.linalg.norm(H)
    spec = np.clip((n * H).sum(axis=2), 0, 1) ** 40

    s = 0.12 + 0.95 * diffuse - 0.12 * sym
    rgb = shade(dark, mid, light, s)
    rgb = rgb + spec[..., None] * 140.0
    rgb = rgb * np.where(r > 0.955, 0.62, 1.0)[..., None]  # a thin dark edge

    alpha = np.clip((1.0 - r) * radius + 0.5, 0, 1)
    return np.dstack([np.clip(rgb, 0, 255), alpha])


def coin_flip_frame(coin, face, angle, size, thickness):
    """The coin turned by angle about its vertical axis (edge band on the side turning towards us)."""
    _, dark, mid, light, _ = coin
    cs, sn = math.cos(angle), math.sin(angle)
    w = max(1, int(round(size * abs(cs))))
    e = int(round(thickness * sn))
    out = np.zeros((size, size, 4))

    face_im = Image.fromarray(np.clip(face[..., :3], 0, 255).astype(np.uint8))
    face_a = Image.fromarray(np.clip(face[..., 3] * 255, 0, 255).astype(np.uint8))
    fw = np.asarray(face_im.resize((w, size), Image.LANCZOS)).astype(np.float64)
    fa = np.asarray(face_a.resize((w, size), Image.LANCZOS)).astype(np.float64) / 255.0
    fw = fw * (0.5 + 0.5 * abs(cs))  # the face darkens as it turns away from the light

    total = w + e
    x0 = (size - total) // 2
    edge_on_right = cs >= 0
    face_x = x0 if edge_on_right else x0 + e

    # Edge band: the face's silhouette swept across the thickness, with reeding lines
    if e > 0:
        band_a = np.zeros((size, size))
        for t in range(e + 1):
            xs = (x0 + t) if edge_on_right else (x0 + e - t)
            lo, hi = max(0, xs), min(size, xs + w)
            if hi > lo:
                band_a[:, lo:hi] = np.maximum(band_a[:, lo:hi], fa[:, lo - xs:hi - xs])
        y = np.arange(size, dtype=np.float64)[:, None]
        reed = 0.5 + 0.5 * np.cos(y * 2 * np.pi / (size / 14.0))
        vert = 1.0 - np.abs(y - size / 2) / (size / 2)
        s = 0.18 + 0.32 * vert + 0.1 * reed
        band_rgb = shade(dark, mid, light, np.broadcast_to(s, (size, size)))
        out[..., :3] = band_rgb
        out[..., 3] = band_a

    # Face on top
    lo, hi = max(0, face_x), min(size, face_x + w)
    fa_c = fa[:, lo - face_x:hi - face_x]
    out[:, lo:hi, :3] = fw[:, lo - face_x:hi - face_x] * fa_c[..., None] + out[:, lo:hi, :3] * (1 - fa_c[..., None])
    out[:, lo:hi, 3] = np.maximum(out[:, lo:hi, 3], fa_c)
    return out


def to_image(a):
    rgb = np.clip(a[..., :3], 0, 255).astype(np.uint8)
    alpha = np.clip(a[..., 3] * 255, 0, 255).astype(np.uint8)
    return Image.fromarray(np.dstack([rgb, alpha]), "RGBA")


def make_coins(out_dir):
    cell = CELL * SCALE
    hi = cell * SS
    d_hi = int(round(COIN_D * SCALE * SS))
    pad = (hi - d_hi) // 2
    faces = Image.new("RGBA", (cell, cell * len(COINS)), (0, 0, 0, 0))
    flips = Image.new("RGBA", (cell * FLIP_FRAMES, cell * len(COINS)), (0, 0, 0, 0))
    for row, coin in enumerate(COINS):
        face = coin_face(coin, d_hi)
        canvas = np.zeros((hi, hi, 4))
        canvas[pad:pad + d_hi, pad:pad + d_hi] = face
        # Downsample premultiplied, so transparent pixels add no dark fringe
        faces.paste(downsample(canvas, cell), (0, row * cell))
        for f in range(FLIP_FRAMES):
            frame = coin_flip_frame(coin, face, f * math.pi / FLIP_FRAMES, d_hi, EDGE_T * SCALE * SS)
            canvas = np.zeros((hi, hi, 4))
            canvas[pad:pad + d_hi, pad:pad + d_hi] = frame
            flips.paste(downsample(canvas, cell), (f * cell, row * cell))
    os.makedirs(os.path.join(out_dir, "patches"), exist_ok=True)
    os.makedirs(os.path.join(out_dir, "images"), exist_ok=True)
    faces.save(os.path.join(out_dir, "patches", "coinfaces@2x.png"), optimize=True)
    flips.save(os.path.join(out_dir, "images", "horzcoins@2x.png"), optimize=True)


def downsample(a, size):
    """RGBA float (alpha 0..1) -> RGBA image of size x size, averaging premultiplied colour."""
    pm = np.dstack([a[..., :3] * a[..., 3:4], a[..., 3:4] * 255.0])
    im = Image.fromarray(np.clip(pm, 0, 255).astype(np.uint8), "RGBA")
    small = np.asarray(im.resize((size, size), Image.BOX)).astype(np.float64)
    alpha = small[..., 3:4]
    rgb = np.where(alpha > 0, small[..., :3] * 255.0 / np.maximum(alpha, 1), 0)
    return Image.fromarray(np.dstack([np.clip(rgb, 0, 255), alpha]).astype(np.uint8), "RGBA")


# ---------------------------------------------------------------------------------------------------------
# Logo

# Block font: '#' = filled cell. Capitals are 7 cells high; lower case letters 5 (x-height) with
# ascenders 2 above and descenders 2 below. Each glyph: (rows, top row from the cap line).
GLYPHS = {
    "F": ([
        "#####",
        "#####",
        "##...",
        "####.",
        "####.",
        "##...",
        "##...",
    ], 0),
    "a": ([
        "####.",
        "...##",
        "#####",
        "##.##",
        "#####",
    ], 2),
    "k": ([
        "##...",
        "##...",
        "##.##",
        "####.",
        "###..",
        "####.",
        "##.##",
    ], 0),
    "e": ([
        "#####",
        "##.##",
        "#####",
        "##...",
        "#####",
    ], 2),
    "m": ([
        "########",
        "##.##.##",
        "##.##.##",
        "##.##.##",
        "##.##.##",
    ], 2),
    "o": ([
        "#####",
        "##.##",
        "##.##",
        "##.##",
        "#####",
    ], 2),
    "n": ([
        "#####",
        "##.##",
        "##.##",
        "##.##",
        "##.##",
    ], 2),
    "y": ([
        "##.##",
        "##.##",
        "##.##",
        "#####",
        "...##",
        "...##",
        "####.",
    ], 2),
    "!": ([
        "##",
        "##",
        "##",
        "..",
        "##",
    ], 2),
}


def word_mask(word, unit):
    """Mask (float 0..1) of a word in the block font, unit px per cell, one cell between letters."""
    widths = [len(GLYPHS[ch][0][0]) for ch in word]
    w_cells = sum(widths) + len(word) - 1
    h_cells = 9
    im = Image.new("L", (int(w_cells * unit), int(h_cells * unit)), 0)
    d = ImageDraw.Draw(im)
    x = 0
    for ch, gw in zip(word, widths):
        rows, top = GLYPHS[ch]
        for j, row in enumerate(rows):
            for i, c in enumerate(row):
                if c == "#":
                    x0, y0 = (x + i) * unit, (top + j) * unit
                    d.rectangle([x0, y0, x0 + unit - 1, y0 + unit - 1], fill=255)
        x += gw + 1
    # Rounded corners: blur and threshold (convex corners round, concave ones fill a little)
    im = im.filter(ImageFilter.GaussianBlur(unit * 0.28)).point(lambda p: 255 if p >= 128 else 0)
    return np.asarray(im).astype(np.float64) / 255.0


# Logo layout in the title screen logo's frame (472x239): "Fake" in 15 px cells at the top left, ending
# where the game's top word ends so the "Deluxe" badge (drawn over the logo's top right) stays clear of it,
# and "money!" in 11.5 px cells under it
LOGO_W, LOGO_H = 472, 239
LOGO_TOP = (15.0, 52, 12)        # cell size, x, y of the cap line
LOGO_BOTTOM = (11.5, 36, 102)


def make_logo_art(k, glow=True):
    """The "Fake money!" logo at k px per logo pixel, an RGBA float array (LOGO_W*k x LOGO_H*k): in a
    yellow-green glow (the title screen's logo) or with a soft shadow (the small logo)."""
    u1, u2 = LOGO_TOP[0] * k, LOGO_BOTTOM[0] * k
    top = word_mask("Fake", u1)
    bottom = word_mask("money!", u2)
    w, h = int(LOGO_W * k), int(LOGO_H * k)
    mask = np.zeros((h, w))

    def put(m, x, y):
        hh, ww = min(m.shape[0], h - y), min(m.shape[1], w - x)
        mask[y:y + hh, x:x + ww] = np.maximum(mask[y:y + hh, x:x + ww], m[:hh, :ww])

    put(top, int(LOGO_TOP[1] * k), int(LOGO_TOP[2] * k))
    put(bottom, int(LOGO_BOTTOM[1] * k), int(LOGO_BOTTOM[2] * k))

    def grow(m, r):
        if r <= 0:
            return m
        im = Image.fromarray((m * 255).astype(np.uint8)).filter(ImageFilter.MaxFilter(2 * int(r) + 1))
        return np.asarray(im).astype(np.float64) / 255.0

    outline = grow(mask, u1 * 0.18)
    drop = np.zeros_like(outline)
    steps = int(u1 * 0.4)
    for t in range(steps + 1):  # the letters' black side, down and to the right
        drop[t:, t:] = np.maximum(drop[t:, t:], outline[:h - t, :w - t])
    rgb = np.zeros((h, w, 3))
    if glow:
        rgb[:] = np.array([214, 236, 10], dtype=np.float64)
        a = np.clip(blur(grow(drop, u1 * 0.25), u1 * 0.45) * 1.6, 0, 1) * 0.9
    else:
        shadow = np.zeros_like(drop)
        o = int(u1 * 0.5)
        shadow[o:, o:] = drop[:h - o, :w - o]
        a = blur(shadow, u1 * 0.3) * 0.55
    # black outline and drop
    a = np.maximum(a, drop)
    rgb = rgb * (1 - drop[..., None])
    # green letters: lighter at the top of each line, a bevel from the blurred mask
    yy = np.arange(h, dtype=np.float64)[:, None]
    top_y, bottom_y = LOGO_TOP[2] * k, LOGO_BOTTOM[2] * k + 2 * u2
    grad = np.where(yy < bottom_y - u2, (yy - top_y) / (7 * u1), (yy - bottom_y) / (5 * u2))
    grad = np.clip(grad, 0, 1)
    green_top = np.array([120, 255, 70], dtype=np.float64)
    green_bottom = np.array([20, 200, 10], dtype=np.float64)
    letter = green_top + (green_bottom - green_top) * grad[..., None]
    soft = blur(mask, u1 * 0.12)
    gy, gx = np.gradient(soft)
    bevel = np.clip(-(gx + gy) * u1 * 0.9, -0.5, 0.5)
    letter = letter * (1 + bevel[..., None])
    rgb = rgb * (1 - mask[..., None]) + letter * mask[..., None]
    a = np.maximum(a, mask)
    return np.dstack([np.clip(rgb, 0, 255), a])


def fit_logo(art, width, height):
    """art scaled to fit width x height (keeping its aspect), centred; RGBA image."""
    ah, aw = art.shape[:2]
    k = min(width / aw, height / ah)
    nw, nh = max(1, int(aw * k)), max(1, int(ah * k))
    canvas = np.zeros((height * SS, width * SS, 4))
    big = np.asarray(Image.fromarray(np.clip(art[..., :3], 0, 255).astype(np.uint8)).resize((nw * SS, nh * SS), Image.LANCZOS)).astype(np.float64)
    big_a = np.asarray(Image.fromarray(np.clip(art[..., 3] * 255, 0, 255).astype(np.uint8)).resize((nw * SS, nh * SS), Image.LANCZOS)).astype(np.float64) / 255.0
    x0, y0 = (width - nw) * SS // 2, (height - nh) * SS // 2
    canvas[y0:y0 + nh * SS, x0:x0 + nw * SS, :3] = big
    canvas[y0:y0 + nh * SS, x0:x0 + nw * SS, 3] = big_a
    pm = np.dstack([canvas[..., :3] * canvas[..., 3:4], canvas[..., 3:4] * 255.0])
    im = Image.fromarray(np.clip(pm, 0, 255).astype(np.uint8), "RGBA").resize((width, height), Image.BOX)
    small = np.asarray(im).astype(np.float64)
    alpha = small[..., 3:4]
    rgb = np.where(alpha > 0, small[..., :3] * 255.0 / np.maximum(alpha, 1), 0)
    return Image.fromarray(np.dstack([np.clip(rgb, 0, 255), alpha]).astype(np.uint8), "RGBA")


def make_logos(out_dir):
    fit_logo(make_logo_art(SCALE * 2), 472 * SCALE, 239 * SCALE).save(
        os.path.join(out_dir, "images", "bigmoneylogo@2x.png"), optimize=True)
    fit_logo(make_logo_art(SCALE * 2, glow=False), 156 * SCALE, 76 * SCALE).save(
        os.path.join(out_dir, "images", "floatlogo@2x.png"), optimize=True)


# ---------------------------------------------------------------------------------------------------------
# Texts and manifest

def xml_escape(s):
    return s.replace("&", "&amp;").replace("<", "&lt;").replace(">", "&gt;")


def make_texts(out_dir):
    assert len(RANK_TITLES) == 27
    os.makedirs(os.path.join(out_dir, "properties"), exist_ok=True)
    lines = ['<?xml version="1.0"?>', '<Properties version="1.0" language="en" country="US">', "",
             "<!-- Fake Money! Deluxe: rank titles of the stats panel, lowest first (27, as in the game) -->",
             '<StringArray id="gRankNames">']
    lines += ["  <String>%s</String>" % xml_escape(t) for t in RANK_TITLES]
    lines += ["</StringArray>", "", "<!-- Texts that name the game -->"]
    lines += ['<String id="%s">%s</String>' % (k, xml_escape(v)) for k, v in STRINGS.items()]
    lines += ["", "</Properties>", ""]
    with open(os.path.join(out_dir, "properties", "fakemoney.xml"), "w", encoding="utf-8", newline="\n") as f:
        f.write("\n".join(lines))


def make_fake_money(mods_dir):
    out_dir = os.path.join(mods_dir, "fake-money")
    os.makedirs(os.path.join(out_dir, "images"), exist_ok=True)
    make_coins(out_dir)
    make_logos(out_dir)
    make_texts(out_dir)
    manifest = {
        "id": "fake-money",
        "name": "Fake Money! Deluxe",
        "version": "1.0",
        "author": "byo-pop examples",
        "description": "Crypto coins, a \"Fake money!\" logo and crypto rank titles. Original art, drawn by "
                       "games/bigmoney/tools/make_example_mods.py.",
        "imagePatches": [
            {"target": "images/coins", "image": "patches/coinfaces@2x.png", "x": 0, "y": 0}
        ],
        "strings": ["properties/fakemoney.xml"],
    }
    with open(os.path.join(out_dir, "mod.json"), "w", encoding="utf-8", newline="\n") as f:
        f.write(json.dumps(manifest, indent=2) + "\n")
    print("wrote", out_dir)


def main():
    here = os.path.dirname(os.path.abspath(__file__))
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("--out", default=os.path.join(here, "..", "mods"), help="mods folder")
    args = ap.parse_args()
    make_fake_money(os.path.abspath(args.out))


if __name__ == "__main__":
    main()
