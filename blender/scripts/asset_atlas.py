"""Shared flip-safe PBR atlas for Astro Voyager's own procedural assets.

One source of truth for the palette-slot atlas used by the fleet, Voyager,
own-craft and Aurora generators. Two invariants are easy to break by accident
and cost a re-render each, so they live here instead of being copied around:

  * The game's ``loadBmp`` in ``src/ObjModel.cpp`` and Blender's image import
    read the same BMP with opposite V. Every tile is therefore authored on a
    mirrored row axis (patterns become V-symmetric, and the derived normal's
    green channel flips with it), and each tile is written into both its row
    and its mirrored row, so a flipped read can never land on an empty tile.
  * Slot choice travels along U only, and tiles are square, so panel seams
    keep their aspect instead of stretching into stripes across a face.

Run from a generator:

    import asset_atlas
    MATERIAL = asset_atlas.build("dragon_PBR", PALETTE, MODELS, "dragon")
    ...
    asset_atlas.remap_uv(obj, slot)
"""
import math
import struct
from pathlib import Path

import bpy
import numpy as np

GRID = 4            # 4x4 tile grid inside a 1024 atlas -> 256 px per material
TILE = 256
ROLES_DEFAULT = ("panel", "rib", "quilt", "glass", "window",
                 "brushed", "recess", "lens")


def write_bmp(path, rgb):
    path = Path(path)
    height, width, _ = rgb.shape
    rows = np.clip(rgb * 255.0, 0, 255).astype(np.uint8)[:, :, ::-1]
    padding = (-width * 3) % 4
    payload = b"".join(row.tobytes() + b"\0" * padding for row in rows)
    header = struct.pack("<2sIHHI", b"BM", 54 + len(payload), 0, 0, 54)
    dib = struct.pack("<IiiHHIIiiII", 40, width, height, 1, 24, 0,
                      len(payload), 2835, 2835, 0, 0)
    path.write_bytes(header + dib + payload)


def _value_noise(size, cells, seed):
    """Smooth value noise in [0,1]: bilinear upsample of a coarse random grid."""
    rng = np.random.default_rng(seed)
    grid = rng.random((cells, cells)).astype(np.float32)
    axis = np.linspace(0.0, cells - 1.0, size)
    i0 = np.floor(axis).astype(np.int32)
    i1 = np.minimum(i0 + 1, cells - 1)
    f = (axis - i0).astype(np.float32)
    s = f * f * (3.0 - 2.0 * f)
    rows = grid[i0] * (1.0 - s)[:, None] + grid[i1] * s[:, None]
    return rows[:, i0] * (1.0 - s)[None, :] + rows[:, i1] * s[None, :]


def _fbm(size, seed, base=8, octaves=4):
    acc = np.zeros((size, size), dtype=np.float32)
    total = 0.0
    for o in range(octaves):
        weight = 1.0 / (1.0 + o)
        acc += _value_noise(size, base * (2 ** o), seed + o * 97) * weight
        total += weight
    return acc / total


def _normal_from_height(height, strength):
    gy, gx = np.gradient(height.astype(np.float32))
    n = np.stack([-gx * strength, -gy * strength, np.ones_like(gx)], axis=-1)
    n /= np.maximum(np.linalg.norm(n, axis=-1, keepdims=True), 1e-6)
    return (0.5 + 0.5 * n).astype(np.float32)


def tile(role, color, rough, metal, emission, seed, t=TILE):
    """One material tile: albedo, rough, metal, emissive and a height field.

    Height is turned into a normal map by the caller, so seams, puffs, louvers
    and window mullions catch the engine's key light instead of reading as flat
    coloured blocks.
    """
    yy, xx = np.mgrid[:t, :t].astype(np.float32)
    yy = np.minimum(yy, (t - 1) - yy)      # V-symmetric: see module docstring
    grime = _fbm(t, seed + 11, base=6)
    speckle = _fbm(t, seed + 41, base=48)
    alb = np.broadcast_to(np.asarray(color, np.float32), (t, t, 3)).copy()
    rgh = np.full((t, t), rough, np.float32)
    met = np.full((t, t), metal, np.float32)
    emi = np.broadcast_to(np.asarray(emission, np.float32), (t, t, 3)).copy()
    height = np.zeros((t, t), np.float32)

    if role == "panel":                   # riveted hull panels
        cell = 64.0
        seam = ((xx % cell < 2.0) | (yy % cell < 2.0)).astype(np.float32)
        bay = (((xx % cell) > 6) & ((xx % cell) < cell - 8) &
               ((yy % cell) > 6) & ((yy % cell) < cell - 8)).astype(np.float32)
        height += bay * 0.35 - seam * 0.5
        rivet = (((xx % cell) > 4) & ((xx % cell) < 8) &
                 ((yy % cell) > 4) & ((yy % cell) < 8))
        rivet |= (((xx % cell) > cell - 8) & ((xx % cell) < cell - 4) &
                  ((yy % cell) > cell - 8) & ((yy % cell) < cell - 4))
        height[rivet] += 0.25
        alb *= (0.90 + 0.13 * grime)[..., None]
        alb -= (0.16 * seam)[..., None]
        rgh += 0.12 * seam + 0.10 * grime
        met += 0.05 * seam
    elif role == "rib":                   # dark composite with ribbing
        rib = (np.sin(xx * math.tau / 22.0) * 0.5 + 0.5).astype(np.float32)
        height += rib * 0.4
        alb *= (0.82 + 0.30 * rib)[..., None]
        alb *= (0.90 + 0.20 * speckle)[..., None]
        rgh -= 0.08 * rib
    elif role == "quilt":                 # MLI foil: puffs, creases, tape
        cell = 32.0
        puff = (np.cos((xx % cell) / cell * math.tau) *
                np.cos((yy % cell) / cell * math.tau)).astype(np.float32)
        crease = ((xx % cell < 1.5) | (yy % cell < 1.5)).astype(np.float32)
        height += puff * 0.5 - crease * 0.6
        tape = (((xx > t * 0.10) & (xx < t * 0.35) &
                 (yy > t * 0.55) & (yy < t * 0.62)) |
                ((xx > t * 0.60) & (xx < t * 0.68) &
                 (yy > t * 0.05) & (yy < t * 0.40)))
        alb *= (0.86 + 0.26 * (puff * 0.5 + 0.5))[..., None]
        alb[tape] = (0.72, 0.68, 0.60)
        met[tape] = 0.55
        rgh = rgh + 0.14 * crease - 0.10 * (puff * 0.5 + 0.5)
        rgh[tape] = 0.35
    elif role == "glass":                 # optics / dark glass with coating rings
        ring = (np.sin(np.hypot(xx - t / 2, yy - t / 2) * math.tau / 40.0) * 0.5 + 0.5)
        alb *= (0.75 + 0.50 * ring)[..., None]
        rgh -= 0.18 * ring
        height += ring * 0.15
    elif role == "cells":                 # solar cells: bus bars + finger grid
        cw, ch = 42.0, 42.0
        gap = (xx % cw < 3.0) | (yy % ch < 3.0)
        bus = (xx % (cw * 2) < 4.0) | (yy % (ch * 2) < 4.0)
        finger = (yy % 6 < 1.0) & ~gap
        alb[gap] *= 0.45
        alb[bus] = (0.55, 0.57, 0.60)
        alb[finger] *= 1.18
        height[~gap] += 0.30
        height[gap] -= 0.25
        rgh[gap] = np.clip(rgh[gap] + 0.25, 0.0, 1.0)
        rgh[bus] = 0.30
        met[bus] = 0.70
    elif role == "window":                # lit windows: panes + mullions
        wx, wy = 48.0, 34.0
        mull = (xx % wx < 10.0) | (yy % wy < 9.0)
        win = ~mull
        alb[mull] = (0.03, 0.04, 0.06)
        alb[win] = np.minimum(np.asarray(color, np.float32) * 0.55, 1.0)
        emi[mull] = (0.0, 0.0, 0.0)
        emi[win] = np.asarray(emission, np.float32)
        rgh[mull] = 0.55
        rgh[win] = 0.08
        met[mull] = 0.35
        height[win] = 0.45
    elif role == "brushed":               # brushed bright metal
        brush = _fbm(t, seed + 7, base=160)
        alb *= (0.88 + 0.22 * brush)[..., None]
        rgh += 0.14 * (brush - 0.5)
        height += brush * 0.2
    elif role == "recess":                # dark cavities
        alb *= (0.80 + 0.40 * speckle)[..., None]
        rgh += 0.15 * grime
    elif role == "lens":                  # beacon / warning lens
        lens = (np.hypot(xx - t / 2, yy - t / 2) < t * 0.34).astype(np.float32)
        emi *= (0.35 + 0.65 * lens)[..., None]
        height += lens * 0.3
        rgh -= 0.20 * lens
    elif role == "polished":
        rgh[:] = rough
    elif role == "concrete":              # poured concrete with formwork joints
        cell = 128.0
        joint = ((xx % cell < 2.5) | (yy % cell < 2.5)).astype(np.float32)
        tie = (((xx % cell) > 20) & ((xx % cell) < 26) &
               ((yy % cell) > 20) & ((yy % cell) < 26))
        tie |= (((xx % cell) > cell - 26) & ((xx % cell) < cell - 20) &
                ((yy % cell) > cell - 26) & ((yy % cell) < cell - 20))
        height -= joint * 0.40
        height[tie] -= 0.30
        alb *= (0.90 + 0.18 * grime)[..., None]
        alb *= (0.95 + 0.10 * speckle)[..., None]
        alb -= (0.14 * joint)[..., None]
        alb[tie] *= 0.82
        rgh += 0.10 * grime + 0.08 * joint
    elif role == "masonry":               # brick / block courses, running bond
        bw, bh = 32.0, 16.0
        row = (yy // bh).astype(np.int32)
        shift = (row % 2) * (bw * 0.5)
        bx = (xx + shift) % bw
        joint = ((bx < 2.0) | ((yy % bh) < 2.0)).astype(np.float32)
        brick_id = row * 7 + ((xx + shift) // bw).astype(np.int32) * 13
        tone = ((brick_id * 37) % 11) / 11.0
        height -= joint * 0.55
        height += (1.0 - joint) * 0.12
        alb *= (0.86 + 0.22 * tone)[..., None]
        alb *= (0.92 + 0.16 * grime)[..., None]
        alb -= (0.22 * joint)[..., None]
        rgh += 0.10 * joint + 0.08 * grime
    elif role == "siding":                # horizontal clapboard boards
        bh = 26.0
        board = (yy % bh) / bh
        shadow = (board > 0.86).astype(np.float32)
        height += (1.0 - board) * 0.25 - shadow * 0.45
        alb *= (0.90 + 0.14 * (1.0 - board))[..., None]
        alb *= (0.93 + 0.14 * grime)[..., None]
        alb -= (0.18 * shadow)[..., None]
        rgh += 0.10 * shadow + 0.06 * grime
    elif role == "roof":                  # standing-seam metal roofing
        sw = 34.0
        seam = ((xx % sw) < 3.0).astype(np.float32)
        pan = (np.sin((xx % sw) / sw * math.tau) * 0.5 + 0.5).astype(np.float32)
        height += seam * 0.60 - pan * 0.08
        alb *= (0.88 + 0.16 * pan)[..., None]
        alb *= (0.92 + 0.16 * grime)[..., None]
        alb += (0.10 * seam)[..., None]
        rgh -= 0.12 * seam
        met += 0.10 * seam
    elif role == "asphalt":               # road / yard surface with lane paint
        agg = _fbm(t, seed + 23, base=96)
        crack = (_fbm(t, seed + 57, base=12) > 0.72).astype(np.float32)
        dash = (((yy > t * 0.47) & (yy < t * 0.53)) &
                ((xx % 64.0) < 34.0)).astype(np.float32)
        alb *= (0.80 + 0.40 * agg)[..., None]
        alb -= (0.25 * crack)[..., None]
        alb[dash > 0] = (0.78, 0.74, 0.55)
        height += agg * 0.18 - crack * 0.30
        rgh += 0.12 * agg + 0.10 * crack
        met *= 0.2
    else:
        raise ValueError(f"unknown atlas role {role!r}")

    return alb, np.clip(rgh, 0.02, 1.0), np.clip(met, 0.0, 1.0), emi, height


def build(material_name, palette, out_dir, prefix, size=1024,
          roles=ROLES_DEFAULT, overrides=None, emission_strength=3.0,
          seed_base=1000):
    """Write the five BMP maps for `palette` and return a linked Blender material."""
    out_dir = Path(out_dir)
    palette = list(palette)
    if overrides:
        for slot, val in overrides.items():
            palette[slot] = val
    if len(palette) != 8 or len(roles) != 8 or size < 1024 or size % 1024:
        raise ValueError("Atlas requires eight materials and a size divisible by 1024")
    tile_px = size // GRID
    maps = {k: np.zeros((size, size, 3), dtype=np.float32)
            for k in ("albedo", "rough", "metal", "normal", "emissive")}
    for slot, (color, rough, metal, emission) in enumerate(palette):
        role = roles[slot]
        alb, rgh, met, emi, height = tile(role, color, rough, metal, emission,
                                          seed=seed_base + slot * 137)
        rep = max(1, tile_px // TILE)
        if rep > 1:
            alb = np.repeat(np.repeat(alb, rep, 0), rep, 1)
            rgh = np.repeat(np.repeat(rgh, rep, 0), rep, 1)
            met = np.repeat(np.repeat(met, rep, 0), rep, 1)
            emi = np.repeat(np.repeat(emi, rep, 0), rep, 1)
            height = np.repeat(np.repeat(height, rep, 0), rep, 1)
        gy, gx = int(slot // GRID), int(slot % GRID)
        # Duplicate into the mirrored row so a flipped V read still finds the
        # same material instead of sampling empty atlas space.
        for row in {gy, GRID - 1 - gy}:
            region = (slice(row * tile_px, (row + 1) * tile_px),
                      slice(gx * tile_px, (gx + 1) * tile_px))
            maps["albedo"][region] = np.clip(alb, 0.0, 1.0)
            maps["rough"][region] = rgh[..., None]
            maps["metal"][region] = met[..., None]
            maps["emissive"][region] = np.clip(emi, 0.0, 1.0)
            maps["normal"][region] = _normal_from_height(height, 3.2)
    for key, pixels in maps.items():
        write_bmp(out_dir / f"{prefix}_{key}.bmp", pixels)

    material = bpy.data.materials.new(material_name)
    material.use_nodes = True
    tree = material.node_tree
    bsdf = tree.nodes.get("Principled BSDF")
    uv = tree.nodes.new("ShaderNodeTexCoord")
    mapping = tree.nodes.new("ShaderNodeMapping")
    mapping.inputs["Scale"].default_value = (1, -1, 1)
    mapping.inputs["Location"].default_value = (0, 1, 0)
    # Match the engine's top-down BMP upload, including asymmetric noise and normals.
    tree.links.new(uv.outputs["UV"], mapping.inputs["Vector"])
    for key, socket in (("albedo", "Base Color"), ("rough", "Roughness"),
                        ("metal", "Metallic"), ("emissive", "Emission Color"), ("normal", "Normal")):
        image = bpy.data.images.load(str(out_dir / f"{prefix}_{key}.bmp"), check_existing=False)
        if key in ("rough", "metal", "normal"):
            image.colorspace_settings.name = "Non-Color"
        node = tree.nodes.new("ShaderNodeTexImage")
        node.image = image
        tree.links.new(mapping.outputs["Vector"], node.inputs["Vector"])
        if key == "normal":
            normal = tree.nodes.new("ShaderNodeNormalMap")
            tree.links.new(node.outputs["Color"], normal.inputs["Color"])
            tree.links.new(normal.outputs["Normal"], bsdf.inputs[socket])
        else:
            tree.links.new(node.outputs["Color"], bsdf.inputs[socket])
    bsdf.inputs["Emission Strength"].default_value = emission_strength
    return material


def remap_uv(obj, slot):
    """Map each face onto its square atlas tile (call before other UV edits)."""
    uv = obj.data.uv_layers.active or obj.data.uv_layers.new(name="UVMap")
    gx, gy = slot % GRID, slot // GRID
    margin = 0.004
    span = 1.0 / GRID - 2.0 * margin
    for loop in uv.data:
        coord = loop.uv
        coord.x = gx / GRID + margin + coord.x * span
        coord.y = gy / GRID + margin + coord.y * span
