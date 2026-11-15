"""Compares pictures of the same name in two game folders (the Collection check, README.md "The PopCap Game Collection version"):
size, mode, the number of pixels that differ (as RGBA) and the largest difference of a channel, and a side-by-side picture of each
in the output folder.

    python cmpimages.py <folder a> <folder b> <output folder> <name>...      names relative to the folders, e.g. images/titlescreen.jpg
"""
import os
import sys

from PIL import Image, ImageChops


def main():
    folder_a, folder_b, out = sys.argv[1:4]
    os.makedirs(out, exist_ok=True)
    for name in sys.argv[4:]:
        a = Image.open(os.path.join(folder_a, name))
        b = Image.open(os.path.join(folder_b, name))
        print(name, 'a', a.size, a.mode, '| b', b.size, b.mode)
        if a.size != b.size:
            continue
        ra, rb = a.convert('RGBA'), b.convert('RGBA')
        diff = ImageChops.difference(ra, rb)
        pixels = sum(1 for p in diff.get_flattened_data() if p != (0, 0, 0, 0))
        largest = max(max(p) for p in diff.get_flattened_data())
        print('   pixels that differ:', pixels, 'of', ra.size[0] * ra.size[1], ', largest difference of a channel', largest)
        side = Image.new('RGBA', (a.size[0] * 2 + 10, a.size[1]), (255, 0, 255, 255))
        side.paste(ra, (0, 0))
        side.paste(rb, (a.size[0] + 10, 0))
        side.convert('RGB').save(os.path.join(out, 'cmp_' + os.path.splitext(os.path.basename(name))[0] + '.png'))


main()
