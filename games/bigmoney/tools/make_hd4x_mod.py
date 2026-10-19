#!/usr/bin/env python3
"""Generate Big Money's "4x Textures" mod: every image of the game at 4x resolution, upscaled with Real-ESRGAN
from the user's own game files. The output is derived from the game's art, so it is generated locally and
git-ignored (games/bigmoney/mods/.gitignore); never commit or share it.

    python games/bigmoney/tools/make_hd4x_mod.py [--game <Big Money Deluxe folder>]
                                                 [--upscaler <realesrgan-ncnn-vulkan.exe>]
                                                 [--out <mods folder, default games/bigmoney/mods>]
                                                 [--keep-temp]

Adapted from games/bejeweled/tools/make_hd4x_mod.py (same method; Big Money's sprite sheet grids). It runs
the upscaler on the GPU for a minute or two; copy the result to build/native/mods-bigmoney (or import the
folder in the web build's Mods dialog).

Real-ESRGAN ncnn-vulkan (BSD-3-Clause): https://github.com/xinntao/Real-ESRGAN/releases (v0.2.5.0,
realesrgan-ncnn-vulkan-20220424-windows.zip). It needs a Vulkan GPU.

For every image stem in images/ the mod gets:
  images/<stem>@4x.png      RGBA: the colour upscaled 4x with its alpha (the 1x mask baked in). This is the
                            GPU texture (see platform/ImageLoader.h, HD variants).
  images/<stem>.<ext>, images/_<stem>.<ext>
                            the original 1x files, copied. Code that reads pixels on the CPU (the bright
                            coins, the badge, the level-up squash; see games/bigmoney/src/HDPixels.h) then
                            sees exactly the original images.
Stems that are only a mask (_<stem> with no colour image, here _clouds) get _<stem>@4x.png (grey) and the
1x mask.

How the upscale avoids the usual artifacts:
  * Transparent pixels hold junk colours. They are filled from the nearest opaque pixels first, so edges
    get no halos.
  * Sprite sheets are upscaled cell by cell (GRIDS, the cells the game draws) and font strips glyph run by
    glyph run (split at fully transparent columns), so neighbouring frames and glyphs do not bleed into
    each other.
  * Models: realesrgan-x4plus everywhere, except realesrgan-x4plus-anime for the fonts (crisper outlines).
"""
import argparse
import json
import os
import shutil
import subprocess
import sys
import tempfile

import numpy as np
from PIL import Image

SCALE = 4
PAD = 8
EXTS = [".tga", ".jpg", ".png", ".gif", ".jpeg", ".bmp"]  # the loader's probe order (kImageExtensions)
DEFAULT_UPSCALER = r"E:\retools\realesrgan-ncnn-vulkan-20220424\bin\realesrgan-ncnn-vulkan.exe"
MODEL = "realesrgan-x4plus"
FONT_MODEL = "realesrgan-x4plus-anime"
# Sheets cut into cells: (cell width, cell height), from the rectangles the game draws (games/bigmoney/src)
GRIDS = {
    "coins": (29, 29), "horzcoins": (29, 29),         # coins and their flip (Board::DrawPiece)
    "coin": (40, 40), "dollar": (33, 33), "sparkle": (20, 20), "explosion": (45, 91), "bag": (33, 36),
    "bill": (120, 50), "mouths": (184, 136), "rankstrip": (100, 100), "countdown": (160, 160),
    "instructoarrows": (50, 50), "instructotext": (200, 50), "checkboxes": (22, 23),
    "buttonstates": (108, 29), "gamedifftype": (100, 15), "gametypestext": (154, 248),
    "startarrows": (30, 30), "helpcircles": (104, 330),
}
MOD_ID = "hd4x-textures"


def find_game_dir(explicit):
    here = os.path.dirname(os.path.abspath(__file__))
    for d in ([explicit] if explicit else [os.path.join(here, "..", "..", "..", "assets", "bigmoney"),
                                           r"A:\SteamLibrary\steamapps\common\Big Money Deluxe"]):
        if d and os.path.isdir(os.path.join(d, "images")) and os.path.isdir(os.path.join(d, "data")):
            return os.path.abspath(d)
    return None


def find_variant(names, stem):
    """First file named stem + ext in the loader's probe order (names: lower-case name -> real name)."""
    for e in EXTS:
        n = names.get((stem + e).lower())
        if n:
            return n
    return None


def image_stems(images_dir):
    names = {n.lower(): n for n in os.listdir(images_dir)}
    stems = {}
    for n in os.listdir(images_dir):
        base, ext = os.path.splitext(n)
        if ext.lower() not in EXTS or "@" in base:
            continue
        stem = base[1:] if base.startswith("_") else base[:-1] if base.endswith("_") else base
        stems[stem.lower()] = stem
    out = []
    for stem in sorted(stems.values(), key=str.lower):
        colour = find_variant(names, stem)
        mask = find_variant(names, "_" + stem) or find_variant(names, stem + "_")
        out.append((stem, colour, mask))
    return out


def font_stems(data_dir):
    """Image stems used as font strips: legacy descriptors are named after their image, newer ones name it
    in LayerSetImage."""
    fonts = set()
    for n in os.listdir(data_dir):
        if not n.lower().endswith(".txt"):
            continue
        text = open(os.path.join(data_dir, n), encoding="latin-1").read()
        if "LayerSetImage" in text:
            for line in text.splitlines():
                parts = line.split("'")
                if line.split() and line.split()[0] == "LayerSetImage" and len(parts) >= 3:
                    fonts.add(os.path.basename(parts[1]).lower())
        elif text.split() and text.split()[0].lstrip("-").isdigit():
            fonts.add(os.path.splitext(n)[0].lower())
    return fonts


def load_1x(images_dir, colour, mask):
    """The image as the loader builds it at 1x: RGBA, the mask's blue channel as alpha."""
    img = None
    if colour:
        img = Image.open(os.path.join(images_dir, colour)).convert("RGBA")
    if mask:
        m = Image.open(os.path.join(images_dir, mask)).convert("RGB").getchannel("B")
        if img is None:
            img = Image.new("RGBA", m.size, (255, 255, 255, 0))
        if img.size == m.size:
            img.putalpha(m)
    return np.array(img)


def fill_transparent(rgba):
    """RGB of transparent pixels from the nearest opaque ones (iterative dilation), so the upscaler sees
    the edge colours instead of junk."""
    rgb = rgba[..., :3].astype(np.float32)
    known = rgba[..., 3] >= 16
    if known.all() or not known.any():
        return rgba[..., :3].copy()
    rgb[~known] = 0
    for _ in range(64):
        if known.all():
            break
        acc = np.zeros_like(rgb)
        cnt = np.zeros(known.shape, np.float32)
        k = known.astype(np.float32)
        for dy, dx in ((-1, 0), (1, 0), (0, -1), (0, 1), (-1, -1), (-1, 1), (1, -1), (1, 1)):
            acc += np.roll(np.roll(rgb * k[..., None], dy, 0), dx, 1)
            cnt += np.roll(np.roll(k, dy, 0), dx, 1)
        grow = (~known) & (cnt > 0)
        rgb[grow] = acc[grow] / cnt[grow][:, None]
        known = known | grow
    rgb[~known] = rgb[known].mean(axis=0) if known.any() else 0
    return np.clip(rgb + 0.5, 0, 255).astype(np.uint8)


class Piece:
    """A rectangle of one image, upscaled on its own (padded with its edge pixels) and pasted back in
    place."""

    def __init__(self, rect, pad_mode, model):
        self.rect, self.pad_mode, self.model = rect, pad_mode, model


def pieces_for(stem, rgba, fonts):
    h, w = rgba.shape[:2]
    key = stem.lower()
    model = FONT_MODEL if key in fonts else MODEL
    opaque = bool((rgba[..., 3] == 255).all())
    if key in GRIDS:
        cw, ch = GRIDS[key]
        return [Piece((x, y, min(x + cw, w), min(y + ch, h)), "edge", model)
                for y in range(0, h, ch) for x in range(0, w, cw)]
    if key in fonts and not opaque:
        # Runs of columns with ink, split at fully transparent columns; each run takes the transparent
        # columns around it up to the middle of the gap
        ink = (rgba[..., 3] > 0).any(axis=0)
        runs, x = [], 0
        while x < w:
            if ink[x]:
                s = x
                while x < w and ink[x]:
                    x += 1
                runs.append((s, x))
            else:
                x += 1
        out = []
        for i, (s, e) in enumerate(runs):
            left = 0 if i == 0 else (runs[i - 1][1] + s) // 2
            right = w if i == len(runs) - 1 else (e + runs[i + 1][0]) // 2
            out.append(Piece((left, 0, right, h), "edge", model))
        return out or [Piece((0, 0, w, h), "edge", model)]
    return [Piece((0, 0, w, h), "edge", model)]


def padded(arr, mode):
    pads = ((PAD, PAD), (PAD, PAD)) + (((0, 0),) if arr.ndim == 3 else ())
    return np.pad(arr, pads, mode=mode)


def run_upscaler(exe, model, in_dir, out_dir):
    os.makedirs(out_dir, exist_ok=True)
    r = subprocess.run([exe, "-i", in_dir, "-o", out_dir, "-n", model, "-s", str(SCALE), "-f", "png"],
                       capture_output=True, text=True)
    if r.returncode != 0:
        sys.exit(f"upscaler failed ({model}): {r.stderr[-2000:]}")


def main():
    here = os.path.dirname(os.path.abspath(__file__))
    ap = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    ap.add_argument("--game")
    ap.add_argument("--upscaler", default=DEFAULT_UPSCALER)
    ap.add_argument("--out", default=os.path.join(here, "..", "mods"))
    ap.add_argument("--keep-temp", action="store_true")
    args = ap.parse_args()

    game = find_game_dir(args.game)
    if not game:
        sys.exit("game files not found (pass --game <Big Money Deluxe folder>)")
    if not os.path.isfile(args.upscaler):
        sys.exit(f"upscaler not found: {args.upscaler}")
    images_dir = os.path.join(game, "images")
    fonts = font_stems(os.path.join(game, "data"))
    mod = os.path.join(os.path.abspath(args.out), MOD_ID)
    out_images = os.path.join(mod, "images")
    if os.path.isdir(out_images):
        shutil.rmtree(out_images)
    os.makedirs(out_images)

    work = tempfile.mkdtemp(prefix="hd4x-")
    try:
        # 1. Cut every image into pieces and write the padded inputs, one folder per model
        jobs = []  # (stem, colour, mask, rgba, pieces)
        n = 0
        for stem, colour, mask in image_stems(images_dir):
            rgba = load_1x(images_dir, colour, mask)
            pieces = pieces_for(stem, rgba, fonts)
            for p in pieces:
                x0, y0, x1, y1 = p.rect
                sub = rgba[y0:y1, x0:x1]
                p.id = n
                n += 1
                p.opaque = bool((sub[..., 3] == 255).all())
                p.empty = not (sub[..., 3] > 0).any()
                if p.empty:
                    continue
                d = os.path.join(work, "in-" + p.model)
                os.makedirs(d, exist_ok=True)
                Image.fromarray(padded(fill_transparent(sub), p.pad_mode)).save(os.path.join(d, f"{p.id:05d}c.png"))
                if not p.opaque:
                    a = padded(sub[..., 3], p.pad_mode)
                    d = os.path.join(work, "in-" + MODEL)
                    os.makedirs(d, exist_ok=True)
                    Image.fromarray(np.dstack([a, a, a])).save(os.path.join(d, f"{p.id:05d}a.png"))
            jobs.append((stem, colour, mask, rgba, pieces))
        print(f"{len(jobs)} images, {n} pieces")

        # 2. Upscale each folder in one run
        for model in sorted({MODEL, FONT_MODEL}):
            d = os.path.join(work, "in-" + model)
            if os.path.isdir(d):
                print(f"upscaling {len(os.listdir(d))} inputs with {model}...")
                run_upscaler(args.upscaler, model, d, os.path.join(work, "out-" + model))

        # 3. Reassemble, write the 4x image and copy the 1x originals
        total = 0
        for stem, colour, mask, rgba, pieces in jobs:
            h, w = rgba.shape[:2]
            big = np.zeros((h * SCALE, w * SCALE, 4), np.uint8)
            for p in pieces:
                if p.empty:
                    continue
                c = np.array(Image.open(os.path.join(work, "out-" + p.model, f"{p.id:05d}c.png")).convert("RGB"))
                if p.opaque:
                    a = np.full(c.shape[:2], 255, np.uint8)
                else:
                    a = np.array(Image.open(os.path.join(work, "out-" + MODEL, f"{p.id:05d}a.png")).convert("L"))
                x0, y0, x1, y1 = p.rect
                b = PAD * SCALE
                tw, th = (x1 - x0) * SCALE, (y1 - y0) * SCALE
                big[y0 * SCALE:y1 * SCALE, x0 * SCALE:x1 * SCALE, :3] = c[b:b + th, b:b + tw]
                big[y0 * SCALE:y1 * SCALE, x0 * SCALE:x1 * SCALE, 3] = a[b:b + th, b:b + tw]
            if colour:
                Image.fromarray(big, "RGBA").save(os.path.join(out_images, stem + "@4x.png"), optimize=True)
                shutil.copy2(os.path.join(images_dir, colour), os.path.join(out_images, colour))
            else:
                Image.fromarray(big[..., 3], "L").save(os.path.join(out_images, "_" + stem + "@4x.png"), optimize=True)
            if mask:
                shutil.copy2(os.path.join(images_dir, mask), os.path.join(out_images, mask))
            total += 1
    finally:
        if args.keep_temp:
            print(f"temp files kept in {work}")
        else:
            shutil.rmtree(work, ignore_errors=True)

    with open(os.path.join(mod, "mod.json"), "w", encoding="utf-8") as f:
        json.dump({
            "id": MOD_ID,
            "name": "4x Textures",
            "version": "1.0",
            "author": "byo-pop",
            "description": "Every image at 4x resolution, upscaled with Real-ESRGAN from your own game files. "
                           "Looks best in a large window or fullscreen.",
        }, f, indent=2)
        f.write("\n")
    size = sum(os.path.getsize(os.path.join(out_images, n)) for n in os.listdir(out_images))
    print(f"wrote {mod}: {total} images, {size / 1e6:.1f} MB")


if __name__ == "__main__":
    main()
