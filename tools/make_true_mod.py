#!/usr/bin/env python3
"""Generate the "TRUE" joke mod: every gem becomes a square meme picture, crudely tinted to the gem's colour,
that spins and wobbles by
crudely skewing the image, the "EXCELLENT!" popup for big moves says "TRUE..." instead, and "NO MOVES!"
becomes "NO TRUE...".

    python tools/make_true_mod.py [--image <picture>] [--game <Bejeweled Deluxe data folder>] [--out mods]

--image: any picture (jpg/png/gif). It is center-cropped to a square. Without --image the script uses
         mods/true-gem/source/true.* if present, otherwise a plain "TRUE..." stand-in card.
--game:  folder with the game's images/ and data/ (default: the repo's staged assets/, else the Steam
         install). Needed for the font extension below.

The gem art is an @2x image patch over all of images/gemsheet6: 7 gems, 2 rows each (row 2g = spin
animation, row 3 = glint animation, 15 frames of 52x52 each at 1x).

Banner texts (GO!, LEVEL n, EXCELLENT!, ...) are pre-rendered with the legacy font overfont2, which has
no period. The mod therefore ships an extended copy of that font: the strip gains a "." glyph made from
the dot of the font's own "!" and data/overfont2.txt gains the matching entry. Those two files are
derived from the user's game files and are generated locally (git-ignored).
"""
import argparse
import glob
import json
import math
import os

from PIL import Image, ImageChops, ImageDraw, ImageFont, ImageOps

CELL = 52
SCALE = 2
FRAMES = 15
GEMS = 7
# Gem order in gemsheet6 and the "bad recolour" for each: the picture is multiplied by a loud flat colour,
# so the white card takes the gem's colour and the black "True..." stays black. White stays as it is.
GEM_TINTS = [
    (255, 236, 24),   # 0 yellow
    None,             # 1 white
    (36, 150, 255),   # 2 blue
    (244, 34, 44),    # 3 red
    (204, 44, 226),   # 4 purple
    (255, 138, 20),   # 5 orange
    (64, 222, 44),    # 6 green
]
SIDE = 42 * SCALE    # the picture's size inside the 104 px cell (gems are ~44-46 px at 1x)


def stand_in_card(size):
    """Placeholder until a real picture is supplied: a grey card that says TRUE..."""
    img = Image.new("RGB", (size, size), (40, 40, 40))
    d = ImageDraw.Draw(img)
    for y in range(size):
        g = int(30 + 90 * y / size)
        d.line([(0, y), (size, y)], fill=(g, g, g))
    font = None
    for name in ("arialbd.ttf", "Arial Bold.ttf", "DejaVuSans-Bold.ttf"):
        try:
            font = ImageFont.truetype(name, size // 4)
            break
        except OSError:
            continue
    font = font or ImageFont.load_default()
    text = "TRUE..."
    box = d.textbbox((0, 0), text, font=font)
    x = (size - (box[2] - box[0])) / 2
    y = size * 0.72 - (box[3] - box[1]) / 2
    d.text((x + 3, y + 3), text, font=font, fill=(0, 0, 0))
    d.text((x, y), text, font=font, fill=(255, 255, 255))
    return img


def load_picture(path):
    if path is None:
        return stand_in_card(SIDE * 4)
    img = Image.open(path)
    img = ImageOps.exif_transpose(img).convert("RGB")
    return ImageOps.fit(img, (SIDE * 4, SIDE * 4), Image.LANCZOS, centering=(0.5, 0.4))


def skewed(picture, scale_x, shear, flash=0.0):
    """Draws the square picture into one @2x cell, squashed horizontally and sheared. On purpose this
    looks cheap: a flat card with a thick black border and no shading."""
    cell = CELL * SCALE
    card = picture.resize((SIDE, SIDE), Image.LANCZOS)
    if flash > 0:
        card = Image.blend(card, Image.new("RGB", card.size, (255, 255, 255)), flash)
    card = ImageOps.expand(card, border=3, fill=(0, 0, 0)).convert("RGBA")
    w, h = card.size
    sx = max(0.06, abs(scale_x))
    if scale_x < 0:
        card = ImageOps.mirror(card)
    out_w = int(math.ceil(w * sx + abs(shear) * h)) + 2
    # inverse affine map: output (x, y) -> input ((x - shear*(y - h/2) - offset) / sx, y)
    offset = (out_w - w * sx) / 2
    warped = card.transform((out_w, h), Image.AFFINE,
                            (1 / sx, -shear / sx, (-offset + shear * h / 2) / sx, 0, 1, 0),
                            resample=Image.BILINEAR, fillcolor=(0, 0, 0, 0))
    frame = Image.new("RGBA", (cell, cell), (0, 0, 0, 0))
    if warped.width > cell:
        warped = warped.resize((cell, h), Image.BILINEAR)
    frame.alpha_composite(warped, ((cell - warped.width) // 2, (cell - h) // 2))
    return frame


def find_game_dir(explicit):
    here = os.path.dirname(os.path.abspath(__file__))
    for d in ([explicit] if explicit else [os.path.join(here, "..", "assets"),
                                           r"A:\SteamLibrary\steamapps\common\Bejeweled Deluxe"]):
        if d and os.path.isfile(os.path.join(d, "data", "overfont2.txt")):
            return d
    return None


def find_case(folder, name):
    for n in os.listdir(folder):
        if n.lower() == name.lower():
            return os.path.join(folder, n)
    return None


def add_period_to_overfont(game_dir, mod_dir):
    """Writes images/overfont2.png (RGBA) and data/overfont2.txt with an extra "." glyph appended."""
    desc = open(os.path.join(game_dir, "data", "overfont2.txt")).read()
    tokens = desc.split()
    entries = [(tokens[i], int(tokens[i + 1])) for i in range(2, len(tokens) - 1, 2)]
    if any(ch == "." for ch, _ in entries):
        return  # the font already has a period
    images = os.path.join(game_dir, "images")
    colour = Image.open(find_case(images, "overfont2.png")).convert("RGB")
    mask = Image.open(find_case(images, "_overfont2.png")).convert("RGB").getchannel("B")  # SAF uses the low byte
    strip = colour.copy()
    strip.putalpha(mask)
    w, h = strip.size

    x = 0
    for ch, cw in entries:
        if ch == "!":
            break
        x += cw
    bang_w = dict(entries)["!"]
    bang = strip.crop((x, 0, x + bang_w, h))
    # The glyphs are white with a dark outline, and the outline joins the stroke and the dot in the
    # alpha channel, so find the dot by brightness: it is the lowest run of rows with bright pixels.
    # Cut in the middle of the dark gap above it so the dot keeps its own outline.
    px = bang.load()
    bright = [any(px[i, y][3] > 128 and sum(px[i, y][:3]) > 600 for i in range(bang_w)) for y in range(h)]
    dot_bottom = max(y for y in range(h) if bright[y])
    dot_top = dot_bottom
    while dot_top > 0 and bright[dot_top - 1]:
        dot_top -= 1
    stroke_bottom = dot_top - 1
    while stroke_bottom > 0 and not bright[stroke_bottom]:
        stroke_bottom -= 1
    cut = (stroke_bottom + dot_top + 1) // 2
    period = Image.new("RGBA", (bang_w, h), (0, 0, 0, 0))
    period.paste(bang.crop((0, cut, bang_w, h)), (0, cut))
    top, bottom = cut, dot_bottom

    out = Image.new("RGBA", (w + bang_w, h), (0, 0, 0, 0))
    out.paste(strip, (0, 0))
    out.paste(period, (w, 0))
    os.makedirs(os.path.join(mod_dir, "images"), exist_ok=True)
    os.makedirs(os.path.join(mod_dir, "data"), exist_ok=True)
    out.save(os.path.join(mod_dir, "images", "overfont2.png"))
    with open(os.path.join(mod_dir, "data", "overfont2.txt"), "w") as f:
        f.write(desc.rstrip() + f" . {bang_w}\n")
    print(f"overfont2: added '.' ({bang_w} px wide, dot rows {top}-{bottom}) -> strip {w + bang_w}x{h}")


def tinted(picture, tint):
    if tint is None:
        return picture
    return ImageChops.multiply(picture, Image.new("RGB", picture.size, tint))


def make_patch(picture, path):
    cell = CELL * SCALE
    sheet = Image.new("RGBA", (FRAMES * cell, 2 * GEMS * cell), (0, 0, 0, 0))
    for gem in range(GEMS):
        card = tinted(picture, GEM_TINTS[gem])
        top = 2 * gem * cell
        for f in range(FRAMES):
            t = f / FRAMES
            # spin: a "rotation" faked by squashing the card and leaning it. The card narrows to a sliver
            # and widens again always showing its front, so the text is never mirrored, and the lean
            # changes sides at the edge-on frames; the last frame leads smoothly back into the first.
            sheet.alpha_composite(skewed(card, abs(math.cos(math.pi * t)), 0.35 * math.sin(2 * math.pi * t)),
                                  (f * cell, top))
            # glint: the card wobbles back and forth with a cheap white flash in the middle
            flash = max(0.0, 1.0 - abs(f - 7) / 2.5) * 0.55
            sheet.alpha_composite(skewed(card, 1.0, 0.22 * math.sin(2 * math.pi * t), flash), (f * cell, top + cell))
    os.makedirs(os.path.dirname(path), exist_ok=True)
    sheet.save(path, optimize=True)
    return sheet


# The logo's "JEWE" (letters plus their outline and shadow) in the title screen and in the side panel of
# the game screen, at 1x: (target image, x, y, width, height), measured on the original art.
LOGO_BOXES = [
    ("images/titlescreen6", "logo-title", 152, 28, 268, 128),
    ("images/alphaback1", "logo-panel", 40, 4, 91, 44),
]
LOGO_SCALE = 4  # sharp with the 4x Textures mod too


def logo_font(size):
    for name in ("impact.ttf", "Impact.ttf", "arialbd.ttf", "DejaVuSans-Bold.ttf"):
        try:
            return ImageFont.truetype(name, size)
        except OSError:
            continue
    return ImageFont.load_default()


def make_logo_patches(mod):
    """BEJEWELED -> BE[TRUE]LED: a black box over JEWE with TRUE in white. Drawn from scratch, so the patches
    contain nothing from the game."""
    entries = []
    for target, name, x, y, w, h in LOGO_BOXES:
        img = Image.new("RGBA", (w * LOGO_SCALE, h * LOGO_SCALE), (0, 0, 0, 255))
        d = ImageDraw.Draw(img)
        size = int(img.height * 0.9)
        while True:
            font = logo_font(size)
            box = d.textbbox((0, 0), "TRUE", font=font)
            if (box[2] - box[0] <= img.width * 0.88 and box[3] - box[1] <= img.height * 0.72) or size < 8:
                break
            size -= 2
        tx = (img.width - (box[2] - box[0])) / 2 - box[0]
        ty = (img.height - (box[3] - box[1])) / 2 - box[1]
        d.text((tx, ty), "TRUE", font=font, fill=(255, 255, 255, 255))
        rel = f"patches/{name}@{LOGO_SCALE}x.png"
        img.save(os.path.join(mod, *rel.split("/")))
        entries.append({"target": target, "image": rel, "x": x, "y": y, "mode": "replace"})
    return entries


def main():
    here = os.path.dirname(os.path.abspath(__file__))
    ap = argparse.ArgumentParser()
    ap.add_argument("--image")
    ap.add_argument("--game")
    ap.add_argument("--out", default=os.path.join(here, "..", "mods"))
    ap.add_argument("--preview")
    args = ap.parse_args()
    mod = os.path.join(os.path.abspath(args.out), "true-gem")

    image = args.image
    if image is None:
        found = sorted(glob.glob(os.path.join(mod, "source", "true.*")))
        image = found[0] if found else None
    picture = load_picture(image)
    print(f"picture: {image or 'stand-in TRUE... card (pass --image or put source/true.jpg in the mod)'}")

    patch_rel = "patches/true-gems@2x.png"
    old = os.path.join(mod, "patches", "white-true@2x.png")  # earlier versions patched the white gem only
    if os.path.exists(old):
        os.remove(old)
    sheet = make_patch(picture, os.path.join(mod, *patch_rel.split("/")))
    logo_patches = make_logo_patches(mod)

    game = find_game_dir(args.game)
    if game:
        add_period_to_overfont(game, mod)
    else:
        print("warning: game files not found (pass --game); the banner font has no '.', so it will read TRUE")
    with open(os.path.join(mod, "mod.json"), "w", encoding="utf-8") as f:
        json.dump({
            "id": "true-gem",
            "name": "TRUE",
            "version": "1.0",
            "author": "wasm-bejeweled",
            "description": "Every gem is a badly animated, badly recoloured 'True...' picture, the logo says "
                           "BE TRUE LED, big moves say TRUE... instead of EXCELLENT!, and running out of moves "
                           "says NO TRUE...",
            "imagePatches": [
                {"target": "images/gemsheet6", "image": patch_rel, "x": 0, "y": 0, "mode": "replace"}
            ] + logo_patches,
            "strings": {"EXCELLENT": "TRUE...", "NO_MOVES": "NO TRUE..."},
        }, f, indent=2)
        f.write("\n")
    print(f"wrote {mod}")
    if args.preview:
        bg = Image.new("RGBA", sheet.size, (34, 30, 48, 255))
        bg.alpha_composite(sheet)
        bg.convert("RGB").save(args.preview)


if __name__ == "__main__":
    main()
