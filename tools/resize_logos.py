#!/usr/bin/env python3
"""Resize official NRL badges into Pebble-sized PNGs."""
import os
import subprocess
import tempfile
import urllib.request

from PIL import Image, ImageDraw, ImageOps

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
RAW = os.path.join(ROOT, "resources", "images", "_raw")
OUT = os.path.join(ROOT, "resources", "images")

NRL_GREEN = "#00cf5d"

# short code used in watch payloads -> source filename
TEAMS = [
    ("BRO", "broncos"),
    ("BUL", "bulldogs"),
    ("COW", "cowboys"),
    ("DOL", "dolphins"),
    ("DRA", "dragons"),
    ("EEL", "eels"),
    ("KNI", "knights"),
    ("PAN", "panthers"),
    ("SOU", "rabbitohs"),
    ("CAN", "raiders"),
    ("SYD", "roosters"),
    ("MAN", "sea-eagles"),
    ("CRO", "sharks"),
    ("MEL", "storm"),
    ("GLD", "titans"),
    ("WAR", "warriors"),
    ("WST", "wests-tigers"),
    ("NSW", "blues"),
    ("QLD", "maroons"),
]


def fit_square(im, size):
    im = im.convert("RGBA")
    bbox = im.getbbox()
    if bbox:
        im = im.crop(bbox)
    canvas = Image.new("RGBA", (size, size), (0, 0, 0, 0))
    im.thumbnail((size, size), Image.Resampling.LANCZOS)
    x = (size - im.width) // 2
    y = (size - im.height) // 2
    canvas.paste(im, (x, y), im)
    return canvas


def opaque_mean_luma(im):
    rgba = im.convert("RGBA")
    pixels = rgba.getdata()
    total = 0.0
    count = 0
    for r, g, b, a in pixels:
        if a > 40:
            total += 0.299 * r + 0.587 * g + 0.114 * b
            count += 1
    return (total / count) if count else 0.0


def to_white(bw):
    """1-bit white marks on black, for highlighted / black B&W rows."""
    return ImageOps.invert(bw.convert("L")).convert("1")


def to_bw(im):
    """High-contrast 1-bit icon on white.

    Dark/colour artwork becomes black. Light/white artwork (for the black
    header) also becomes black so B&W Pebble can invert it when highlighted.
    """
    rgba = im.convert("RGBA")
    light_mark = opaque_mean_luma(rgba) > 160
    out = Image.new("1", rgba.size, 1)
    src = rgba.load()
    dst = out.load()
    w, h = rgba.size
    for y in range(h):
        for x in range(w):
            r, g, b, a = src[x, y]
            if a <= 40:
                continue
            lum = 0.299 * r + 0.587 * g + 0.114 * b
            if light_mark:
                if lum > 80:
                    dst[x, y] = 0
            elif lum < 200:
                dst[x, y] = 0
    return out


def is_green(r, g, b, a):
    return a > 40 and g > 80 and g >= r + 30 and g >= b + 30


def is_dark(r, g, b, a):
    return a > 40 and r < 40 and g < 40 and b < 40


def recolor_nrl(im, letters="white"):
    """Menu icon only: solid black fill, white or cut-out letters/chevrons."""
    im = im.convert("RGBA")
    pixels = im.load()
    w, h = im.size
    seen = [[False] * w for _ in range(h)]
    q = []
    for x in range(w):
        q.append((x, 0))
        q.append((x, h - 1))
    for y in range(h):
        q.append((0, y))
        q.append((w - 1, y))
    while q:
        x, y = q.pop()
        if x < 0 or y < 0 or x >= w or y >= h or seen[y][x]:
            continue
        seen[y][x] = True
        r, g, b, a = pixels[x, y]
        if a < 20 or is_dark(r, g, b, a):
            pixels[x, y] = (0, 0, 0, 0)
            q.append((x + 1, y))
            q.append((x - 1, y))
            q.append((x, y + 1))
            q.append((x, y - 1))

    for y in range(h):
        for x in range(w):
            r, g, b, a = pixels[x, y]
            if a < 20:
                continue
            if is_green(r, g, b, a):
                pixels[x, y] = (0, 0, 0, 255)
            elif r < 50 and g < 50 and b < 50:
                if letters == "cutout":
                    pixels[x, y] = (0, 0, 0, 0)
                else:
                    pixels[x, y] = (255, 255, 255, 255)
    return im


def punch_near(im, pred):
    """Set matching pixels (and flood-filled edge matches) to transparent."""
    im = im.convert("RGBA")
    pixels = im.load()
    w, h = im.size
    for y in range(h):
        for x in range(w):
            r, g, b, a = pixels[x, y]
            if a > 0 and pred(r, g, b, a):
                pixels[x, y] = (r, g, b, 0)
    return im


def invert_opaque(im):
    im = im.convert("RGBA")
    pixels = im.load()
    w, h = im.size
    for y in range(h):
        for x in range(w):
            r, g, b, a = pixels[x, y]
            if a > 0:
                pixels[x, y] = (255 - r, 255 - g, 255 - b, a)
    return im


def lighten_dark_marks(im):
    """Turn near-black marks white; leave chromatic colours (blue/maroon)."""
    im = im.convert("RGBA")
    pixels = im.load()
    w, h = im.size
    for y in range(h):
        for x in range(w):
            r, g, b, a = pixels[x, y]
            if a < 20:
                continue
            mx = max(r, g, b)
            mn = min(r, g, b)
            if mx < 50 or (mx < 90 and (mx - mn) < 18):
                pixels[x, y] = (255, 255, 255, a)
    return im


def prepare_nrlw(im):
    """Wikipedia NRLW: punch white/light ground, invert dark mark to white."""
    im = im.convert("RGBA")
    im = punch_near(im, lambda r, g, b, a: r > 230 and g > 230 and b > 230)
    return invert_opaque(im)


def prepare_origin_w(im):
    """Wikia white Origin W: punch black ground, keep white/colour artwork."""
    im = im.convert("RGBA")
    return punch_near(im, lambda r, g, b, a: r < 28 and g < 28 and b < 28)


def prepare_origin_men(im):
    """Origin men shield is blue/maroon; black wordmark must read on black UI."""
    im = im.convert("RGBA")
    if opaque_mean_luma(im) < 80:
        return invert_opaque(im)
    return lighten_dark_marks(im)


def prepare_nsw(im):
    """2021 NSW Blues corporate mark: drop white/black ground, keep sky blue."""
    im = im.convert("RGBA")
    im = punch_near(im, lambda r, g, b, a: r > 240 and g > 240 and b > 240)
    return punch_near(im, lambda r, g, b, a: r < 18 and g < 18 and b < 18)


def make_trophy(size=48):
    im = Image.new("RGBA", (size, size), (0, 0, 0, 0))
    d = ImageDraw.Draw(im)
    gold = (255, 204, 64, 255)
    d.ellipse((12, 4, 36, 26), fill=gold)
    d.rectangle((16, 16, 32, 30), fill=gold)
    d.arc((4, 8, 20, 28), start=90, end=270, fill=gold, width=3)
    d.arc((28, 8, 44, 28), start=270, end=90, fill=gold, width=3)
    d.polygon([(21, 30), (27, 30), (25, 38), (23, 38)], fill=gold)
    d.rectangle((18, 38, 30, 41), fill=gold)
    d.polygon([(12, 46), (36, 46), (32, 41), (16, 41)], fill=gold)
    return im


def fetch_url(url, dest):
    req = urllib.request.Request(url, headers={
        "User-Agent": "Mozilla/5.0 (compatible; NRLFan/1.0)",
        "Accept": "*/*",
    })
    with urllib.request.urlopen(req, timeout=30) as resp:
        data = resp.read()
    os.makedirs(os.path.dirname(dest), exist_ok=True)
    with open(dest, "wb") as f:
        f.write(data)
    print("fetched", url, "->", dest, len(data))
    return dest


def save_png(im, path):
    os.makedirs(os.path.dirname(path), exist_ok=True)
    im.save(path, "PNG")
    print(path, im.size, im.mode)


def ensure_svg_fill(svg_path, fill):
    with open(svg_path, encoding="utf-8") as f:
        text = f.read()
    if 'fill="' in text:
        return svg_path
    text = text.replace("<path ", '<path fill="%s" ' % fill, 1)
    dest = svg_path + ".filled.svg"
    with open(dest, "w", encoding="utf-8") as f:
        f.write(text)
    return dest


def rasterize_svg_resvg(svg_path, png_path, size=256):
    js = os.path.join(ROOT, "tools", "rasterize_svg.js")
    if not os.path.exists(js):
        return False
    env = os.environ.copy()
    extra = os.pathsep.join([
        os.path.join(tempfile.gettempdir(), "nrl-resvg", "node_modules"),
        os.path.join(ROOT, "tools", ".resvg", "node_modules"),
        env.get("NODE_PATH", ""),
    ])
    env["NODE_PATH"] = extra
    try:
        subprocess.check_call(["node", js, svg_path, png_path, str(size)], env=env)
        return os.path.exists(png_path)
    except (OSError, subprocess.CalledProcessError):
        return False


def rasterize_svg(svg_path, png_path, size=256):
    os.makedirs(os.path.dirname(png_path), exist_ok=True)
    size_s = str(size)
    commands = [
        ["rsvg-convert", "-w", size_s, "-h", size_s, "-o", png_path, svg_path],
        ["magick", "-background", "none", "-density", "300", svg_path,
         "-resize", "%dx%d" % (size, size), png_path],
        ["convert", "-background", "none", "-density", "300", svg_path,
         "-resize", "%dx%d" % (size, size), png_path],
    ]
    for cmd in commands:
        try:
            subprocess.check_call(cmd, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
            if os.path.exists(png_path):
                return True
        except (OSError, subprocess.CalledProcessError):
            continue
    try:
        import cairosvg
        cairosvg.svg2png(url=svg_path, write_to=png_path, output_width=size, output_height=size)
        if os.path.exists(png_path):
            return True
    except Exception:
        pass
    return rasterize_svg_resvg(svg_path, png_path, size)


def load_badge(name, fill=None):
    png = os.path.join(RAW, name + ".png")
    svg = os.path.join(RAW, name + ".svg")
    if os.path.exists(png):
        return Image.open(png)
    if os.path.exists(svg):
        src = ensure_svg_fill(svg, fill) if fill else svg
        tmp = os.path.join(tempfile.gettempdir(), "nrl-badge-%s.png" % name)
        if rasterize_svg(src, tmp):
            return Image.open(tmp)
        raise RuntimeError("Could not rasterize %s" % svg)
    raise FileNotFoundError("No badge source for " + name)


def save_store_icons():
    """Shrink the original nrl.svg to app-store sizes. No crop or recolor."""
    svg = os.path.join(RAW, "nrl.svg")
    tmp = os.path.join(tempfile.gettempdir(), "nrl-store-src.png")
    if not os.path.exists(svg):
        raise FileNotFoundError(svg)
    if not rasterize_svg(svg, tmp, size=1024):
        raise RuntimeError("Could not rasterize %s" % svg)
    src = Image.open(tmp).convert("RGBA")
    os.makedirs(os.path.join(ROOT, "store"), exist_ok=True)
    for size, name in ((80, "icon-80.png"), (144, "icon-144.png")):
        out = src.resize((size, size), Image.Resampling.LANCZOS)
        save_png(out, os.path.join(ROOT, "store", name))


def save_comp_logo(im, basename):
    color = fit_square(im, 48)
    bw = to_bw(color)
    save_png(color, os.path.join(OUT, basename + ".png"))
    save_png(color, os.path.join(OUT, basename + "~color.png"))
    save_png(bw, os.path.join(OUT, basename + "~bw.png"))
    save_png(to_white(bw), os.path.join(OUT, basename + "_white.png"))
    return color


def main():
    os.makedirs(OUT, exist_ok=True)

    nrl = load_badge("nrl")
    save_comp_logo(nrl, "logo_nrl")
    save_png(fit_square(recolor_nrl(nrl.copy(), "cutout"), 25), os.path.join(OUT, "menu_icon.png"))
    save_store_icons()

    nrlw_src = os.path.join(RAW, "nrlw_wiki.png")
    if os.path.exists(nrlw_src):
        save_comp_logo(prepare_nrlw(Image.open(nrlw_src)), "logo_nrlw")
    else:
        print("skip logo_nrlw: missing %s (will not fall back to nrlw.svg)" % nrlw_src)

    ow_src = os.path.join(RAW, "origin_w.png")
    if os.path.exists(ow_src):
        save_comp_logo(prepare_origin_w(Image.open(ow_src)), "logo_origin_w")
    else:
        print("skip logo_origin_w: missing %s" % ow_src)

    om_svg = os.path.join(RAW, "origin_men.svg")
    if os.path.exists(om_svg):
        tmp = os.path.join(tempfile.gettempdir(), "nrl-origin-men.png")
        if not rasterize_svg(om_svg, tmp, size=512):
            raise RuntimeError("Could not rasterize %s" % om_svg)
        save_comp_logo(prepare_origin_men(Image.open(tmp)), "logo_origin")
    else:
        print("skip logo_origin: missing %s" % om_svg)

    nsw_svg = os.path.join(RAW, "nsw_2021.svg")
    nsw_url = "https://static.wikia.nocookie.net/logopedia/images/5/50/NSWBlues_2021-corporate.svg"
    if not os.path.exists(nsw_svg):
        try:
            fetch_url(nsw_url, nsw_svg)
        except Exception as err:
            print("NSW fetch failed", err)
    if os.path.exists(nsw_svg):
        tmp = os.path.join(tempfile.gettempdir(), "nrl-nsw-2021.png")
        if rasterize_svg(nsw_svg, tmp, size=512):
            save_comp_logo(prepare_nsw(Image.open(tmp)), "logo_nsw")
        else:
            print("skip logo_nsw: rasterize failed")

    save_comp_logo(make_trophy(48), "logo_trophy")

    for code, name in TEAMS:
        if code == "NSW" and os.path.exists(nsw_svg):
            continue
        src = os.path.join(RAW, name + ".png")
        if not os.path.exists(src):
            continue
        im = Image.open(src)
        color = fit_square(im, 48)
        bw = to_bw(color)
        save_png(color, os.path.join(OUT, "logo_{}~color.png".format(code.lower())))
        save_png(bw, os.path.join(OUT, "logo_{}~bw.png".format(code.lower())))
        save_png(to_white(bw), os.path.join(OUT, "logo_{}_white.png".format(code.lower())))


if __name__ == "__main__":
    main()
