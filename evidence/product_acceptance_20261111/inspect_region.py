"""Crop a screen region and report non-background pixels (read-only image check)."""
import sys
from collections import Counter

from PIL import Image


def main(path, x0, y0, x1, y1, out):
    im = Image.open(path).convert("RGB")
    print("size=", im.size)
    crop = im.crop((int(x0), int(y0), int(x1), int(y1)))
    crop.save(out)
    cnt = Counter(crop.getdata())
    print("top colors:", cnt.most_common(6))
    bg = cnt.most_common(1)[0][0]

    def dist(p):
        return sum(abs(a - b) for a, b in zip(p, bg))

    non_bg = sum(1 for p in crop.getdata() if dist(p) > 30)
    print("non_bg_pixels=", non_bg, "of", crop.size[0] * crop.size[1])


if __name__ == "__main__":
    main(*sys.argv[1:])
