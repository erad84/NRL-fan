#!/usr/bin/env python3
"""Build 1-bit white-on-black logos from existing ~bw variants."""
import glob
import os
import shutil

from PIL import Image, ImageOps

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
OUT = os.path.join(ROOT, "resources", "images")


def main():
    origin_w = os.path.join(OUT, "logo_origin_w.png")
    origin_w_color = os.path.join(OUT, "logo_origin_w~color.png")
    if os.path.exists(origin_w_color):
        shutil.copyfile(origin_w_color, origin_w)
        print("restored", origin_w)

    n = 0
    for src in sorted(glob.glob(os.path.join(OUT, "*~bw.png"))):
        dest = os.path.join(OUT, os.path.basename(src).replace("~bw.png", "_white.png"))
        bw = Image.open(src)
        white = ImageOps.invert(bw.convert("L")).convert("1")
        white.save(dest, "PNG")
        print(dest, white.size)
        n += 1

    for stale in glob.glob(os.path.join(OUT, "*_w.png")):
        if os.path.basename(stale) == "logo_origin_w.png":
            continue
        os.remove(stale)
        print("removed", stale)
    print("wrote", n)


if __name__ == "__main__":
    main()
