"""Convert the real equirectangular Earth map (blender/ref/earth_map.jpg) into
the 24-bit BMP the game's BMP-only loader reads (res/models/earth_map.bmp).
Blender pixel rows are bottom-up, which matches standard BMP storage order,
so no vertical flip is applied. Run headless:
blender --background --python tools/earth_map_to_bmp.py
"""
import struct
from pathlib import Path

import bpy
import numpy as np

ROOT = Path(__file__).resolve().parents[1]
src = ROOT / "blender/ref/earth_map.jpg"
dst = ROOT / "res/models/earth_map.bmp"

img = bpy.data.images.load(str(src))
img.colorspace_settings.name = "Non-Color"
w, h = img.size
px = np.array(img.pixels[:], dtype=np.float32).reshape(h, w, 4)
bgr = np.clip(px[:, :, :3] * 255.0, 0, 255).astype(np.uint8)[:, :, ::-1]
padding = (-w * 3) % 4
payload = b"".join(row.tobytes() + b"\0" * padding for row in bgr)
header = struct.pack("<2sIHHI", b"BM", 54 + len(payload), 0, 0, 54)
dib = struct.pack("<IiiHHIIiiII", 40, w, h, 1, 24, 0, len(payload), 2835, 2835, 0, 0)
dst.write_bytes(header + dib + payload)
print(f"wrote {dst} {w}x{h}", flush=True)