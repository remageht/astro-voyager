"""Generate motion masks for the scenic main-menu backgrounds.

White = nebula/gas region allowed to animate; black = ship, planets, moons
and empty space that must stay pixel-identical. Pure stdlib, no deps.
Run from repo root: py tools/generate_menu_motion_masks.py
"""
import math
import struct
import zlib

W, H = 1792, 1024
OUT = "res/textures/main_menu_motion_{}.png"


def ellipse(x, y, cx, cy, rx, ry, rot=0.0):
    dx, dy = x - cx, y - cy
    if rot:
        c, s = math.cos(rot), math.sin(rot)
        dx, dy = c * dx + s * dy, -s * dx + c * dy
    return math.hypot(dx / rx, dy / ry)


def smooth(edge0, edge1, v):
    t = min(1.0, max(0.0, (v - edge0) / (edge1 - edge0)))
    return t * t * (3.0 - 2.0 * t)


def build(includes, excludes):
    rows = []
    for y in range(H):
        row = bytearray()
        for x in range(W):
            inc = 0.0
            for kind, p in includes:
                if kind == "ellipse":
                    cx, cy, rx, ry, rot, feather = p
                    d = ellipse(x, y, cx, cy, rx, ry, rot)
                    inc = max(inc, 1.0 - smooth(1.0 - feather / min(rx, ry), 1.0, d))
                elif kind == "annulus":
                    cx, cy, rxo, ryo, rxi, ryi, feather = p
                    do = ellipse(x, y, cx, cy, rxo, ryo)
                    di = ellipse(x, y, cx, cy, rxi, ryi)
                    outer = 1.0 - smooth(1.0 - feather / min(rxo, ryo), 1.0, do)
                    inner = smooth(1.0, 1.0 + feather / min(rxi, ryi), di)
                    inc = max(inc, outer * inner)
                elif kind == "rect":
                    x0, y0, x1, y1, feather = p
                    dx = max(x0 - x, x - x1, 0)
                    dy = max(y0 - y, y - y1, 0)
                    inc = max(inc, 1.0 - smooth(0.0, feather, math.hypot(dx, dy)))
            exc = 0.0
            for kind, p in excludes:
                if kind == "ellipse":
                    cx, cy, rx, ry, rot, feather = p
                    d = ellipse(x, y, cx, cy, rx, ry, rot)
                    exc = max(exc, 1.0 - smooth(1.0, 1.0 + feather / min(rx, ry), d))
                elif kind == "rect":
                    x0, y0, x1, y1, feather = p
                    dx = max(x0 - x, x - x1, 0)
                    dy = max(y0 - y, y - y1, 0)
                    exc = max(exc, 1.0 - smooth(0.0, feather, math.hypot(dx, dy)))
                elif kind == "below_line":
                    a, b, feather = p  # exclude y > a + b*x
                    exc = max(exc, smooth(a + b * x - feather, a + b * x, y))
            row.append(int(round(255.0 * min(1.0, max(0.0, inc - exc)))))
        rows.append(bytes(row))
    return rows


def write_png(path, rows):
    raw = b"".join(b"\x00" + r for r in rows)

    def chunk(tag, data):
        c = struct.pack(">I", len(data)) + tag + data
        return c + struct.pack(">I", zlib.crc32(tag + data) & 0xFFFFFFFF)

    png = b"\x89PNG\r\n\x1a\n"
    png += chunk(b"IHDR", struct.pack(">IIBBBBB", W, H, 8, 0, 0, 0, 0))
    png += chunk(b"IDAT", zlib.compress(raw, 9))
    png += chunk(b"IEND", b"")
    with open(path, "wb") as f:
        f.write(png)


FE = 60.0  # include feather, px
XE = 30.0  # exclude outward feather, px

MASKS = {
    "lagoon": (
        [("ellipse", (1130, 400, 560, 330, 0.0, FE)),
         ("ellipse", (1150, 760, 430, 220, 0.0, FE))],
        [("ellipse", (1140, 530, 480, 180, 0.0, XE)),   # planet + ring band
         ("rect", (1370, 480, 1620, 620, XE)),          # ring station
         ("ellipse", (575, 275, 45, 45, 0.0, XE))],     # crescent moon
    ),
    "crimson": (
        [("rect", (760, 0, 1792, 640, 80.0))],
        [("rect", (950, 430, 1440, 630, XE)),           # warp ship + binary star
         ("below_line", (973.0, -0.2286, 40.0))],       # planet limb
    ),
    "emerald": (
        [("annulus", (1230, 470, 600, 480, 300, 240, FE))],
        [("ellipse", (1230, 940, 640, 470, 0.0, XE)),   # planet + aurora limb
         ("rect", (1260, 470, 1792, 770, XE)),          # warp ship
         ("ellipse", (650, 625, 22, 22, 0.0, XE)),
         ("ellipse", (610, 590, 18, 18, 0.0, XE)),
         ("ellipse", (725, 500, 16, 16, 0.0, XE)),
         ("ellipse", (1155, 445, 16, 16, 0.0, XE)),
         ("ellipse", (1195, 517, 14, 14, 0.0, XE)),
         ("ellipse", (660, 808, 20, 20, 0.0, XE)),
         ("ellipse", (1020, 962, 22, 22, 0.0, XE)),
         ("ellipse", (1200, 1008, 20, 20, 0.0, XE)),
         ("ellipse", (1470, 998, 18, 18, 0.0, XE))],
    ),
    "violet": (
        [("ellipse", (1130, 360, 520, 300, math.radians(-20), FE))],
        [("rect", (1180, 410, 1792, 650, XE)),          # warp ship
         ("ellipse", (1520, 980, 560, 420, 0.0, XE))],  # icy moon
    ),
}

for name, (inc, exc) in MASKS.items():
    write_png(OUT.format(name), build(inc, exc))
    print("wrote", OUT.format(name))