#!/usr/bin/env python3
"""Makes the site's bitmap icons from web/brand/favicon.svg (docs/WEB_PLAYER.md, "The site's logo and icons"):
web/brand/favicon.ico (16, 32 and 48 px, for browsers without SVG icons) and web/brand/apple-touch-icon.png
(180 px, opaque: iOS rounds the corners itself). Run it after changing favicon.svg; the results are kept in git.

    python tools/brand_icons.py [--edge <msedge.exe>]

Draws with headless Microsoft Edge, so the bitmaps match what a browser draws from the SVG, and Pillow."""
import argparse, os, subprocess, sys, tempfile, time
from PIL import Image

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
BRAND = os.path.join(ROOT, "web", "brand")
EDGE = r"C:\Program Files (x86)\Microsoft\Edge\Application\msedge.exe"
ICO_SIZES = (16, 32, 48)
TOUCH = 180

def render(edge, svg, work):
    """A page with the icon at each size (transparent around it) and the touch icon on the tile's own gradient,
    so its rounded corners disappear. Returns the screenshot."""
    url = "file:///" + svg.replace("\\", "/")
    x, cells = 0, []
    for n in ICO_SIZES:
        cells.append(f'<img src="{url}" style="position:absolute;left:{x}px;top:0;width:{n}px;height:{n}px">')
        x += n + 8
    cells.append(f'<div style="position:absolute;left:0;top:64px;width:{TOUCH}px;height:{TOUCH}px;'
                 f'background:linear-gradient(#4AAEF0,#1A6CCB)"><img src="{url}" width="{TOUCH}" height="{TOUCH}"></div>')
    page = os.path.join(work, "icons.html")
    with open(page, "w") as f:
        f.write('<!doctype html><html><body style="margin:0;background:transparent">' + "".join(cells) + "</body></html>")
    shot = os.path.join(work, "icons.png")
    subprocess.run([edge, "--headless=new", "--disable-gpu", "--hide-scrollbars", "--no-first-run",
                    "--user-data-dir=" + os.path.join(work, "profile"), "--force-device-scale-factor=1",
                    "--default-background-color=00000000", "--virtual-time-budget=2000",
                    "--screenshot=" + shot, "--window-size=400,260", "file:///" + page.replace("\\", "/")],
                   stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, timeout=120)
    for _ in range(40):  # Edge can hand the work to a process that writes the file a moment later
        if os.path.exists(shot) and os.path.getsize(shot) > 0:
            time.sleep(0.3)
            return Image.open(shot).convert("RGBA")
        time.sleep(0.25)
    sys.exit("brand_icons: Edge wrote no screenshot")

def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--edge", default=EDGE, help="msedge.exe")
    args = ap.parse_args()
    svg = os.path.join(BRAND, "favicon.svg")
    with tempfile.TemporaryDirectory() as work:
        shot = render(args.edge, svg, work)
        icons, x = [], 0
        for n in ICO_SIZES:
            icons.append(shot.crop((x, 0, x + n, n)))
            x += n + 8
        touch = shot.crop((0, 64, TOUCH, 64 + TOUCH)).convert("RGB")
    if icons[0].getpixel((0, 0))[3] > 64 or icons[0].getpixel((8, 8))[3] != 255:  # (0, 0): the rounded corner
        sys.exit("brand_icons: the 16 px icon has the wrong transparency (Edge drew no transparent background?)")
    ico = os.path.join(BRAND, "favicon.ico")
    icons[-1].save(ico, format="ICO", sizes=[(n, n) for n in ICO_SIZES], append_images=icons[:-1])
    touch.save(os.path.join(BRAND, "apple-touch-icon.png"), optimize=True)
    with Image.open(ico) as check:
        sizes = sorted(check.info.get("sizes", []))
    print(f"favicon.ico: {os.path.getsize(ico)} bytes, sizes {sizes}; apple-touch-icon.png: "
          f"{os.path.getsize(os.path.join(BRAND, 'apple-touch-icon.png'))} bytes")

if __name__ == "__main__":
    main()
