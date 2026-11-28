"""Render five per-background menu exit departure sequences with Blender 5.2.

Frame 1 matches the active main-menu backdrop at full resolution (scaled via
CompositorNodeScale in CROP mode matching shaders/menu.frag). A smooth 8-frame
dissolve reveals an isotropically inpainted clean plate plus the game's PBR
3D ship/station with animated engine exhaust plumes and cinematic departure
lighting, saving 120 frames (1280x720 @ 30 fps) + synthesized stereo audio.

Run from the repository root:
  "C:/Program Files/Blender Foundation/Blender 5.2/blender.exe" -b -P tools/render_menu_exit_animations.py -- --test
  "C:/Program Files/Blender Foundation/Blender 5.2/blender.exe" -b -P tools/render_menu_exit_animations.py -- --export-game
"""

import argparse
import math
import shutil
import subprocess
import sys
import wave
from pathlib import Path

import bpy
import numpy as np
from mathutils import Euler, Quaternion, Vector

ROOT = Path(__file__).resolve().parents[1]
MODELS = ROOT / "res" / "models"
OUT = ROOT / "blender" / "out" / "menu_exit_animations"
GAME_VIDEO = ROOT / "res" / "video"

FPS = 30
DURATION_S = 4.0
FRAMES = int(round(FPS * DURATION_S))

# Each variant describes:
# - background: menu art path
# - capsules: normalized top-down rotated capsules (x0, y0, x1, y1, radius) to erase ONLY the 2D ship/station
# - model / length / emission_mult: real game OBJ+PBR asset
# - start / rotation / path / bank / camera_delta: departure choreography matching art framing
# - key / key_energy / fill / fill_energy / accent: filmic AgX lighting matching scene atmosphere
# - has_plume: attach procedural glowing thruster plume mesh at aft +Z
VARIANTS = {
    "blackhole": {
        "label": "Black Hole",
        "background": "res/textures/main_menu_bg.jpg",
        "capsules": (
            (0.585, 0.720, 0.948, 0.625, 0.064),  # fuselage + dorsal fin + yellow nose tip
            (0.752, 0.678, 0.775, 0.672, 0.136),  # forward warp ring (covers top arch & bottom cyan rim)
            (0.834, 0.654, 0.858, 0.648, 0.124),  # mid warp ring
            (0.896, 0.636, 0.920, 0.630, 0.114),  # aft warp ring
        ),
        "model": "warp_ship",
        "length": 6.4,
        "emission_mult": 1.10,
        "start": (4.2, -1.55, -8.8),
        "rotation": (10.0, 118.0, 14.0),
        "path": ((4.2, -1.55, -8.8), (5.0, -0.35, -11.2), (7.6, 2.2, -20.5), (10.8, 4.6, -37.0)),
        "bank": (-9.0, 10.0, 18.0),
        "cam_delta": (-0.35, 0.14, -0.60),
        "key": (1.0, 0.64, 0.34),
        "key_energy": 1.9,
        "key_rot": (20.0, 42.0, -18.0),
        "fill": (0.30, 0.62, 1.0),
        "fill_energy": 0.45,
        "accent": (0.28, 0.82, 1.0),
        "has_plume": True,
    },
    "lagoon": {
        "label": "Celestial Lagoon",
        "background": "res/textures/main_menu_bg_lagoon.png",
        "capsules": (
            (0.775, 0.562, 0.892, 0.528, 0.056),  # tilted orbital ring station
        ),
        "model": "station_megastructure",
        "length": 3.5,
        "emission_mult": 1.05,
        "start": (4.30, -0.40, -9.0),
        "rotation": (76.0, -10.0, 22.0),
        "path": ((4.30, -0.40, -9.0), (3.55, 0.25, -11.4), (1.35, 1.55, -19.0), (-1.8, 2.5, -35.0)),
        "bank": (10.0, -14.0, 22.0),
        "cam_delta": (0.42, 0.18, -0.85),
        "key": (0.58, 0.82, 1.0),
        "key_energy": 1.45,
        "key_rot": (18.0, -48.0, 16.0),
        "fill": (0.18, 0.42, 0.82),
        "fill_energy": 0.35,
        "accent": (0.22, 0.80, 1.0),
        "has_plume": False,
    },
    "crimson": {
        "label": "Crimson Horizon",
        "background": "res/textures/main_menu_bg_crimson.png",
        "capsules": (
            (0.545, 0.528, 0.790, 0.538, 0.046),  # fuselage
            (0.608, 0.528, 0.626, 0.530, 0.092),  # ring 1
            (0.648, 0.532, 0.666, 0.534, 0.095),  # ring 2
            (0.686, 0.538, 0.706, 0.540, 0.098),  # ring 3
        ),
        "model": "warp_ship",
        "length": 6.0,
        "emission_mult": 1.05,
        "start": (2.0, -0.25, -8.8),
        "rotation": (8.0, 122.0, 6.0),
        "path": ((2.0, -0.25, -8.8), (2.7, -0.65, -10.6), (4.6, -2.1, -17.5), (7.6, -4.4, -34.0)),
        "bank": (10.0, 10.0, -14.0),
        "cam_delta": (-0.30, -0.10, -0.55),
        "key": (1.0, 0.44, 0.22),
        "key_energy": 1.75,
        "key_rot": (28.0, 18.0, -12.0),
        "fill": (0.34, 0.52, 0.95),
        "fill_energy": 0.38,
        "accent": (1.0, 0.42, 0.16),
        "has_plume": True,
    },
    "emerald": {
        "label": "Emerald Sanctuary",
        "background": "res/textures/main_menu_bg_emerald.png",
        "capsules": (
            (0.715, 0.552, 0.990, 0.650, 0.058),  # fuselage + aft tail
            (0.785, 0.575, 0.815, 0.588, 0.125),  # ring 1
            (0.835, 0.595, 0.868, 0.608, 0.132),  # ring 2
            (0.888, 0.616, 0.925, 0.628, 0.138),  # ring 3
        ),
        "model": "warp_ship",
        "length": 6.2,
        "emission_mult": 1.05,
        "start": (3.4, -0.95, -8.6),
        "rotation": (14.0, 130.0, -10.0),
        "path": ((3.4, -0.95, -8.6), (4.1, 0.05, -10.4), (6.0, 2.5, -16.5), (9.2, 5.2, -31.0)),
        "bank": (-11.0, 11.0, 15.0),
        "cam_delta": (-0.32, 0.16, -0.50),
        "key": (0.28, 0.98, 0.68),
        "key_energy": 1.65,
        "key_rot": (-18.0, -34.0, 22.0),
        "fill": (0.18, 0.65, 1.0),
        "fill_energy": 0.42,
        "accent": (0.16, 1.0, 0.62),
        "has_plume": True,
    },
    "violet": {
        "label": "Violet Frontier",
        "background": "res/textures/main_menu_bg_violet.png",
        "capsules": (
            (0.668, 0.542, 0.995, 0.525, 0.058),  # fuselage + aft engine deck + right plume
            (0.742, 0.530, 0.774, 0.528, 0.102),  # ring 1 (full top+bottom rim covered)
            (0.822, 0.520, 0.862, 0.518, 0.106),  # ring 2 + dorsal spar
        ),
        "model": "warp_ship",
        "length": 6.1,
        "emission_mult": 0.92,
        "start": (3.25, -0.38, -8.7),
        "rotation": (7.0, 116.0, 14.0),
        "path": ((3.25, -0.38, -8.7), (2.1, 0.65, -10.4), (-0.8, 1.85, -15.5), (-5.2, 3.9, -31.0)),
        "bank": (9.0, -13.0, -16.0),
        "cam_delta": (0.36, 0.14, -0.60),
        "key": (0.72, 0.52, 1.0),
        "key_energy": 1.65,
        "key_rot": (16.0, 32.0, -18.0),
        "fill": (0.28, 0.52, 0.98),
        "fill_energy": 0.40,
        "accent": (0.52, 0.68, 1.0),
        "has_plume": True,
    },
}


def find_ffmpeg():
    exe = shutil.which("ffmpeg")
    if exe:
        return exe
    for candidate in (
        Path("C:/dev/ffmpeg.exe"),
        Path.home() / "AppData/Local/Microsoft/WinGet/Links/ffmpeg.exe",
    ):
        if candidate.is_file():
            return str(candidate)
    for candidate in (Path.home() / "AppData/Local/Microsoft/WinGet/Packages").glob("**/ffmpeg.exe"):
        if candidate.is_file():
            return str(candidate)
    return None


def try_set(obj, prop, value):
    try:
        setattr(obj, prop, value)
        return True
    except (TypeError, AttributeError):
        return False


def set_enum(obj, prop, candidates):
    for value in candidates:
        try:
            setattr(obj, prop, value)
            return True
        except TypeError:
            pass
    return False


def smooth01(value):
    value = min(1.0, max(0.0, value))
    return value * value * (3.0 - 2.0 * value)


def bezier(points, u):
    p0, p1, p2, p3 = (Vector(p) for p in points)
    v = 1.0 - u
    return (v ** 3) * p0 + 3.0 * (v ** 2) * u * p1 + 3.0 * v * (u ** 2) * p2 + (u ** 3) * p3


def load_image_top_down(path):
    image = bpy.data.images.load(str(path), check_existing=True)
    width, height = image.size
    pixels = np.array(image.pixels[:], dtype=np.float32).reshape(height, width, 4)
    return np.flipud(pixels), width, height


def save_image_top_down(path, pixels):
    height, width = pixels.shape[:2]
    image = bpy.data.images.new(path.stem, width=width, height=height, alpha=True)
    image.pixels.foreach_set(np.flipud(pixels).astype(np.float32).reshape(-1))
    image.filepath_raw = str(path)
    image.file_format = "PNG"
    image.save()
    bpy.data.images.remove(image)


def build_capsule_mask(height, width, capsules, feather_px=14.0):
    """Build tight smooth mask covering ONLY specified 2D capsules."""
    yy, xx = np.mgrid[0:height, 0:width].astype(np.float32)
    min_dist = np.full((height, width), 1e9, dtype=np.float32)
    for x0, y0, x1, y1, r_norm in capsules:
        ax = x0 * width
        ay = y0 * height
        bx = x1 * width
        by = y1 * height
        vx = bx - ax
        vy = by - ay
        denom = vx * vx + vy * vy + 1e-6
        t = np.clip(((xx - ax) * vx + (yy - ay) * vy) / denom, 0.0, 1.0)
        px = ax + t * vx
        py = ay + t * vy
        dist = np.sqrt((xx - px) ** 2 + (yy - py) ** 2) - r_norm * min(width, height)
        min_dist = np.minimum(min_dist, dist)

    hard = min_dist <= 0.0
    alpha = 1.0 - np.clip((min_dist + 0.5 * feather_px) / max(1.5 * feather_px, 1.0), 0.0, 1.0)
    alpha = alpha * alpha * (3.0 - 2.0 * alpha)
    return hard, alpha


def box_downsample_2x(img, mask):
    """Downsample RGB + known mask by 2x, averaging only known pixels."""
    h, w = mask.shape
    h2 = h - (h % 2)
    w2 = w - (w % 2)
    img_c = img[:h2, :w2]
    known = (~mask[:h2, :w2]).astype(np.float32)

    k2 = (
        known[0::2, 0::2]
        + known[0::2, 1::2]
        + known[1::2, 0::2]
        + known[1::2, 1::2]
    )
    weighted = (
        img_c[0::2, 0::2] * known[0::2, 0::2, None]
        + img_c[0::2, 1::2] * known[0::2, 1::2, None]
        + img_c[1::2, 0::2] * known[1::2, 0::2, None]
        + img_c[1::2, 1::2] * known[1::2, 1::2, None]
    )
    sub_mask = k2 < 0.5
    sub_img = np.where(k2[..., None] > 0.5, weighted / np.maximum(k2[..., None], 1.0), 0.0)
    return sub_img, sub_mask


def bilinear_upsample_to(small, target_h, target_w):
    """Vectorized bilinear upsampling in pure NumPy."""
    sh, sw = small.shape[:2]
    yy = np.linspace(0.0, sh - 1.0, target_h, dtype=np.float32)
    xx = np.linspace(0.0, sw - 1.0, target_w, dtype=np.float32)
    y0 = np.floor(yy).astype(np.int32)
    x0 = np.floor(xx).astype(np.int32)
    y1 = np.minimum(y0 + 1, sh - 1)
    x1 = np.minimum(x0 + 1, sw - 1)
    wy = (yy - y0)[:, None, None]
    wx = (xx - x0)[None, :, None]
    return (
        small[y0[:, None], x0[None, :]] * (1.0 - wy) * (1.0 - wx)
        + small[y0[:, None], x1[None, :]] * (1.0 - wy) * wx
        + small[y1[:, None], x0[None, :]] * wy * (1.0 - wx)
        + small[y1[:, None], x1[None, :]] * wy * wx
    )


def isotropic_inpaint(rgb, hole_mask, seed=42):
    """Multi-scale isotropic harmonic diffusion inpainting (zero directional stripes)."""
    pyramid = [(rgb.astype(np.float32).copy(), hole_mask.copy())]
    cur_img, cur_mask = pyramid[0]
    for _ in range(7):
        if not np.any(cur_mask) or min(cur_mask.shape) <= 16:
            break
        cur_img, cur_mask = box_downsample_2x(cur_img, cur_mask)
        pyramid.append((cur_img, cur_mask))

    # Solve coarsest level completely
    top_img, top_mask = pyramid[-1]
    known_vals = top_img[~top_mask]
    mean_col = known_vals.mean(axis=0) if len(known_vals) > 0 else np.array([0.02, 0.03, 0.05], dtype=np.float32)
    coarse = top_img.copy()
    coarse[top_mask] = mean_col

    def diffuse(buf, orig, msk, iters):
        h, w = msk.shape
        work = buf.copy()
        p = np.empty((h + 2, w + 2, 3), dtype=np.float32)
        m = msk[..., None]
        for _ in range(iters):
            p[1:-1, 1:-1] = work
            p[0, 1:-1] = work[0]
            p[-1, 1:-1] = work[-1]
            p[1:-1, 0] = work[:, 0]
            p[1:-1, -1] = work[:, -1]
            p[0, 0] = work[0, 0]
            p[0, -1] = work[0, -1]
            p[-1, 0] = work[-1, 0]
            p[-1, -1] = work[-1, -1]
            # 8-neighbor isotropic stencil (cardinals weight 0.15, diagonals 0.10 -> rotationally smooth)
            smooth = (
                0.15 * (p[:-2, 1:-1] + p[2:, 1:-1] + p[1:-1, :-2] + p[1:-1, 2:])
                + 0.10 * (p[:-2, :-2] + p[:-2, 2:] + p[2:, :-2] + p[2:, 2:])
            )
            work = np.where(m, smooth, orig)
        return work

    coarse = diffuse(coarse, top_img, top_mask, 90)
    up = coarse
    for lvl in range(len(pyramid) - 2, -1, -1):
        lvl_img, lvl_mask = pyramid[lvl]
        up = bilinear_upsample_to(up, lvl_mask.shape[0], lvl_mask.shape[1])
        merged = np.where(lvl_mask[..., None], up, lvl_img)
        iters = 32 if lvl > 0 else 28
        up = diffuse(merged, lvl_img, lvl_mask, iters)

    # Subtle ambient grain & faint pinpoint stars matching surrounding space texture
    rng = np.random.default_rng(seed)
    grain = rng.normal(0.0, 0.0045, size=up.shape).astype(np.float32)
    star_roll = rng.random(size=hole_mask.shape)
    dark_mask = (up.mean(axis=2) < 0.08) & hole_mask
    stars = np.where((star_roll > 0.9988) & dark_mask, rng.uniform(0.18, 0.45, size=hole_mask.shape), 0.0).astype(np.float32)
    out = np.clip(up + grain * hole_mask[..., None] + stars[..., None], 0.0, 1.0)
    return np.where(hole_mask[..., None], out, rgb)


def make_clean_plate(background_path, capsules, output_path, seed):
    pixels, width, height = load_image_top_down(background_path)
    hard_mask, alpha = build_capsule_mask(height, width, capsules, feather_px=16.0)
    filled = isotropic_inpaint(pixels[..., :3], hard_mask, seed=seed)
    clean = pixels.copy()
    clean[..., :3] = pixels[..., :3] * (1.0 - alpha[..., None]) + filled * alpha[..., None]
    output_path.parent.mkdir(parents=True, exist_ok=True)
    save_image_top_down(output_path, clean)
    return output_path


def import_obj_merged(filepath, name):
    bpy.ops.wm.obj_import(filepath=str(filepath), forward_axis="Y", up_axis="Z")
    meshes = [obj for obj in bpy.context.selected_objects if obj.type == "MESH"]
    if not meshes:
        return None
    if len(meshes) > 1:
        bpy.ops.object.select_all(action="DESELECT")
        for mesh in meshes:
            mesh.select_set(True)
        bpy.context.view_layer.objects.active = meshes[0]
        bpy.ops.object.join()
        obj = bpy.context.view_layer.objects.active
    else:
        obj = meshes[0]
    obj.name = name
    return obj


def build_pbr_material(name, prefix, emission_mult=1.1):
    material = bpy.data.materials.new(name)
    tree = material.node_tree
    bsdf = next(node for node in tree.nodes if node.type == "BSDF_PRINCIPLED")
    uv = tree.nodes.new("ShaderNodeTexCoord")

    for suffix, color_space, socket in (
        ("albedo", "sRGB", "Base Color"),
        ("rough", "Non-Color", "Roughness"),
        ("metal", "Non-Color", "Metallic"),
        ("emissive", "sRGB", "Emission Color"),
    ):
        path = MODELS / f"{prefix}_{suffix}.bmp"
        if not path.is_file():
            continue
        image = bpy.data.images.load(str(path), check_existing=True)
        if max(image.size) > 4096:
            try:
                ratio = 2048.0 / max(image.size)
                image.scale(int(image.size[0] * ratio), int(image.size[1] * ratio))
            except Exception:
                pass
        image.colorspace_settings.name = color_space
        texture = tree.nodes.new("ShaderNodeTexImage")
        texture.image = image
        tree.links.new(uv.outputs["UV"], texture.inputs["Vector"])
        tree.links.new(texture.outputs["Color"], bsdf.inputs[socket])
        if suffix == "emissive":
            bsdf.inputs["Emission Strength"].default_value = emission_mult

    normal_path = MODELS / f"{prefix}_normal.bmp"
    if normal_path.is_file():
        image = bpy.data.images.load(str(normal_path), check_existing=True)
        if max(image.size) > 4096:
            try:
                ratio = 2048.0 / max(image.size)
                image.scale(int(image.size[0] * ratio), int(image.size[1] * ratio))
            except Exception:
                pass
        image.colorspace_settings.name = "Non-Color"
        texture = tree.nodes.new("ShaderNodeTexImage")
        texture.image = image
        normal = tree.nodes.new("ShaderNodeNormalMap")
        normal.inputs["Strength"].default_value = 0.85
        tree.links.new(uv.outputs["UV"], texture.inputs["Vector"])
        tree.links.new(texture.outputs["Color"], normal.inputs["Color"])
        tree.links.new(normal.outputs["Normal"], bsdf.inputs["Normal"])
    return material


def build_plume_material(name, color_rgb):
    """Volumetric-look Fresnel + axial-fade glowing ion/warp exhaust cone material."""
    material = bpy.data.materials.new(name)
    try_set(material, "surface_render_method", "BLENDED")
    try_set(material, "use_backface_culling", False)
    tree = material.node_tree
    tree.nodes.clear()
    out = tree.nodes.new("ShaderNodeOutputMaterial")
    mix_sh = tree.nodes.new("ShaderNodeMixShader")
    transp = tree.nodes.new("ShaderNodeBsdfTransparent")
    emit = tree.nodes.new("ShaderNodeEmission")
    emit.inputs["Color"].default_value = (color_rgb[0], color_rgb[1], color_rgb[2], 1.0)
    emit.inputs["Strength"].default_value = 3.8

    tex_coord = tree.nodes.new("ShaderNodeTexCoord")
    sep = tree.nodes.new("ShaderNodeSeparateXYZ")
    # Z=0 is at the nozzle mouth (full), Z=1 is tail tip (0.0)
    ramp_len = tree.nodes.new("ShaderNodeValToRGB")
    ramp_len.color_ramp.interpolation = "EASE"
    ramp_len.color_ramp.elements[0].position = 0.0
    ramp_len.color_ramp.elements[0].color = (1.0, 1.0, 1.0, 1.0)
    ramp_len.color_ramp.elements[1].position = 0.95
    ramp_len.color_ramp.elements[1].color = (0.0, 0.0, 0.0, 1.0)

    lw = tree.nodes.new("ShaderNodeLayerWeight")
    lw.inputs["Blend"].default_value = 0.48
    # Higher power gives a soft feathered plasma core with zero hard cone silhouette
    pow_node = tree.nodes.new("ShaderNodeMath")
    pow_node.operation = "POWER"
    pow_node.inputs[1].default_value = 4.2

    mult = tree.nodes.new("ShaderNodeMath")
    mult.operation = "MULTIPLY"

    scale_alpha = tree.nodes.new("ShaderNodeMath")
    scale_alpha.operation = "MULTIPLY"
    scale_alpha.inputs[1].default_value = 0.45

    tree.links.new(tex_coord.outputs["Generated"], sep.inputs["Vector"])
    tree.links.new(sep.outputs["Z"], ramp_len.inputs["Fac"])
    tree.links.new(lw.outputs["Facing"], pow_node.inputs[0])
    tree.links.new(ramp_len.outputs["Color"], mult.inputs[0])
    tree.links.new(pow_node.outputs["Value"], mult.inputs[1])
    tree.links.new(mult.outputs["Value"], scale_alpha.inputs[0])
    tree.links.new(scale_alpha.outputs["Value"], mix_sh.inputs["Fac"])
    tree.links.new(transp.outputs["BSDF"], mix_sh.inputs[1])
    tree.links.new(emit.outputs["Emission"], mix_sh.inputs[2])
    tree.links.new(mix_sh.outputs["Shader"], out.inputs["Surface"])
    return material


def make_image_plane(name, path, z, cam_z=4.0, lens=34.0, sensor_width=36.0):
    """Size backdrop plane to match camera cover-projection of image aspect ratio."""
    image = bpy.data.images.load(str(path), check_existing=True)
    img_w, img_h = max(image.size[0], 1), max(image.size[1], 1)
    img_aspect = img_w / img_h
    dist = cam_z - z
    frustum_w = dist * (sensor_width / lens)
    frustum_h = frustum_w * (9.0 / 16.0)
    scale_x = frustum_w
    scale_y = max(frustum_h, frustum_w / img_aspect)

    bpy.ops.mesh.primitive_plane_add(size=1.0)
    plane = bpy.context.object
    plane.name = name
    plane.location = (0.0, 0.0, z)
    plane.scale = (scale_x, scale_y, 1.0)

    material = bpy.data.materials.new(name + "Mat")
    tree = material.node_tree
    output = next(node for node in tree.nodes if node.type == "OUTPUT_MATERIAL")
    emission = tree.nodes.new("ShaderNodeEmission")
    texture = tree.nodes.new("ShaderNodeTexImage")
    texture.image = image
    texture.extension = "EXTEND"
    tree.links.new(texture.outputs["Color"], emission.inputs["Color"])
    tree.links.new(emission.outputs["Emission"], output.inputs["Surface"])
    plane.data.materials.append(material)
    return plane


def setup_camera(scene, variant):
    data = bpy.data.cameras.new("ExitCamera")
    data.type = "PERSP"
    data.lens = 34.0
    data.sensor_width = 36.0
    data.clip_start = 0.1
    data.clip_end = 500.0
    camera = bpy.data.objects.new("ExitCamera", data)
    scene.collection.objects.link(camera)
    scene.camera = camera
    dx, dy, dz = variant["cam_delta"]
    for frame in range(1, FRAMES + 1):
        u = (frame - 1) / max(1, FRAMES - 1)
        # Hold steady during initial 8-frame dissolve so frame 1..8 aligns cleanly
        eased = smooth01(max(0.0, (u - 0.06) / 0.94))
        camera.location = (dx * eased, dy * eased, 4.0 + dz * eased)
        camera.rotation_euler = (0.0, 0.0, 0.0)
        camera.keyframe_insert("location", frame=frame)
        camera.keyframe_insert("rotation_euler", frame=frame)


def setup_lighting(scene, variant):
    for name, energy, color, rotation in (
        ("ThemeKey", variant["key_energy"], variant["key"], variant["key_rot"]),
        ("ThemeFill", variant["fill_energy"], variant["fill"], (-32.0, -45.0, 14.0)),
    ):
        data = bpy.data.lights.new(name, "SUN")
        data.energy = energy
        data.color = color
        light = bpy.data.objects.new(name, data)
        light.rotation_euler = tuple(math.radians(value) for value in rotation)
        scene.collection.objects.link(light)


def setup_ship(scene, variant):
    model_stem = variant["model"]
    mat_prefix = variant.get("material_prefix", model_stem)
    ship = import_obj_merged(MODELS / f"{model_stem}.obj", "DepartingHero")
    if ship is None:
        raise RuntimeError(f"Could not import {model_stem}.obj")
    for polygon in ship.data.polygons:
        polygon.use_smooth = True
    ship.data.materials.clear()
    ship.data.materials.append(
        build_pbr_material("DepartingHeroPBR", mat_prefix, emission_mult=variant["emission_mult"])
    )
    bpy.context.view_layer.update()
    local_dims = Vector(ship.dimensions)
    max_dimension = max(local_dims.x, local_dims.y, local_dims.z, 1e-3)
    scale = variant["length"] / max_dimension
    ship.scale = (scale, scale, scale)
    ship.location = variant["start"]
    base = Euler(tuple(math.radians(value) for value in variant["rotation"]), "XYZ").to_quaternion()
    bank_pitch, bank_yaw, bank_roll = variant["bank"]

    engine_data = bpy.data.lights.new("DepartureEngineGlow", "POINT")
    engine_data.color = variant["accent"]
    engine_data.energy = 18.0
    engine = bpy.data.objects.new("DepartureEngineGlow", engine_data)
    scene.collection.objects.link(engine)
    engine.parent = ship
    engine.location = (0.0, 0.0, local_dims.z * 0.55)

    plume = None
    if variant.get("has_plume", False):
        # Size plume in ship-local space so world-space length/radius stay independent of OBJ scale
        nozzle_r = max(local_dims.x, local_dims.y) * 0.105
        plume_len = local_dims.z * 0.45
        bpy.ops.mesh.primitive_cone_add(
            vertices=32, radius1=nozzle_r, radius2=nozzle_r * 0.08, depth=plume_len, location=(0.0, 0.0, 0.0)
        )
        plume = bpy.context.object
        plume.name = "DeparturePlume"
        for p in plume.data.polygons:
            p.use_smooth = True
        for v in plume.data.vertices:
            v.co.z += plume_len * 0.5
        plume.data.materials.append(build_plume_material("DeparturePlumeMat", variant["accent"]))
        plume.parent = ship
        plume.location = (0.0, 0.0, local_dims.z * 0.44)

    previous = None
    for frame in range(1, FRAMES + 1):
        u = (frame - 1) / max(1, FRAMES - 1)
        eased = smooth01(u)
        # Slightly cubic-accelerated departure near end
        flight_u = 0.72 * eased + 0.28 * (u ** 2.4)
        ship.location = tuple(bezier(variant["path"], flight_u))
        pitch = math.radians(bank_pitch) * math.sin(u * math.pi)
        yaw = math.radians(bank_yaw) * u
        roll = math.radians(bank_roll) * smooth01(u)
        rotation = (
            base
            @ Quaternion((1.0, 0.0, 0.0), pitch)
            @ Quaternion((0.0, 1.0, 0.0), yaw)
            @ Quaternion((0.0, 0.0, 1.0), roll)
        )
        if previous is not None:
            rotation.make_compatible(previous)
        previous = rotation
        ship.rotation_euler = tuple(rotation.to_euler("XYZ"))
        ship.keyframe_insert("location", frame=frame)
        ship.keyframe_insert("rotation_euler", frame=frame)
        engine_data.energy = 24.0 + 180.0 * (u ** 1.8)
        engine_data.keyframe_insert("energy", frame=frame)
        if plume is not None:
            thrust = 0.28 + 1.55 * smooth01(min(1.0, u / 0.75)) + 0.85 * (u ** 3.0)
            plume.scale = (1.0, 1.0, thrust)
            plume.keyframe_insert("scale", frame=frame)
    return ship


def setup_compositor(scene, original_path):
    try:
        scene.use_nodes = True
    except Exception:
        pass
    try:
        group = scene.compositing_node_group
        if group is None:
            group = bpy.data.node_groups.new("ExitComp", "CompositorNodeTree")
            scene.compositing_node_group = group
        if not any(socket.in_out == "OUTPUT" for socket in group.interface.items_tree):
            group.interface.new_socket(name="Image", in_out="OUTPUT", socket_type="NodeSocketColor")
        group.nodes.clear()
        layers = group.nodes.new("CompositorNodeRLayers")
        original = group.nodes.new("CompositorNodeImage")
        original.image = bpy.data.images.load(str(original_path), check_existing=True)

        # Match menu shader aspect cover-crop at ANY render resolution (640x360 test & 1280x720 export)
        scale_node = group.nodes.new("CompositorNodeScale")
        scale_node.inputs["Type"].default_value = "Render Size"
        scale_node.inputs["Frame Type"].default_value = "Crop"

        mix = group.nodes.new("CompositorNodeAlphaOver")
        mix.inputs["Factor"].default_value = 1.0
        mix.inputs["Factor"].keyframe_insert("default_value", frame=1)
        mix.inputs["Factor"].default_value = 0.0
        mix.inputs["Factor"].keyframe_insert("default_value", frame=9)

        glare = group.nodes.new("CompositorNodeGlare")
        output = group.nodes.new("NodeGroupOutput")

        group.links.new(original.outputs["Image"], scale_node.inputs["Image"])
        group.links.new(scale_node.outputs["Image"], mix.inputs["Foreground"])
        group.links.new(layers.outputs["Image"], mix.inputs["Background"])
        group.links.new(mix.outputs["Image"], glare.inputs["Image"])
        group.links.new(glare.outputs["Image"], output.inputs["Image"])

        set_enum(glare.inputs["Type"], "default_value", ("Fog Glow", "Bloom"))
        for name, value in (("Threshold", 0.92), ("Size", 7), ("Strength", 0.22), ("Mix", -0.2)):
            socket = glare.inputs.get(name)
            if socket is not None:
                socket.default_value = value
        return mix
    except Exception as exc:
        scene.compositing_node_group = None
        print(f"WARN compositor: {exc}", flush=True)
        return None


def synthesize_audio(path, variant_key, variant):
    rate = 48000
    count = int(round(DURATION_S * rate))
    t = np.arange(count, dtype=np.float64) / rate
    u = t / DURATION_S
    rng = np.random.default_rng(sum(ord(ch) for ch in variant["label"]))
    tau = 2.0 * math.pi

    engine_frequency = 44.0 + 42.0 * (u ** 1.3)
    engine_phase = tau * np.cumsum(engine_frequency) / rate
    engine = 0.20 * (np.sin(engine_phase) + 0.35 * np.sin(engine_phase * 1.51))
    noise = rng.standard_normal(count)
    kernel = np.hanning(96)
    kernel /= kernel.sum()
    air = np.convolve(noise, kernel, mode="same")
    whoosh = air * (np.sin(math.pi * u) ** 1.7) * 0.42

    base_frequency = {
        "blackhole": 220.0,
        "lagoon": 329.63,
        "crimson": 196.0,
        "emerald": 293.66,
        "violet": 261.63,
    }[variant_key]
    chime_env = np.where(t > 2.15, np.exp(-(t - 2.15) * 2.3), 0.0)
    chime = (
        np.sin(tau * base_frequency * t)
        + 0.55 * np.sin(tau * base_frequency * 1.5 * t)
        + 0.30 * np.sin(tau * base_frequency * 2.0 * t)
    ) * chime_env * 0.11

    left = engine + whoosh * (0.75 + 0.20 * np.sin(t * 2.0)) + chime
    right = engine + whoosh * (0.75 - 0.20 * np.sin(t * 2.0)) + np.roll(chime, 420)
    envelope = np.clip(t / 0.14, 0.0, 1.0) * np.clip((DURATION_S - t) / 0.48, 0.0, 1.0) ** 1.5
    stereo = np.stack([left, right], axis=-1) * envelope[:, None]
    peak = np.max(np.abs(stereo)) + 1e-12
    stereo *= min(1.0, 0.88 / peak)
    pcm = (np.clip(stereo, -1.0, 1.0) * 32767.0).astype("<i2")
    path.parent.mkdir(parents=True, exist_ok=True)
    with wave.open(str(path), "wb") as wav:
        wav.setnchannels(2)
        wav.setsampwidth(2)
        wav.setframerate(rate)
        wav.writeframes(pcm.tobytes())


def encode_and_export(ffmpeg, variant_dir, game_dir, export_game):
    wav_path = variant_dir / "audio.wav"
    mp4_path = variant_dir / "departure_30fps.mp4"
    frames_dir = variant_dir / "frames"
    if ffmpeg is None:
        print("WARN ffmpeg not found; frames remain unencoded", flush=True)
        return
    subprocess.run([
        ffmpeg, "-y", "-hide_banner", "-loglevel", "error",
        "-framerate", str(FPS), "-i", str(frames_dir / "frame_%04d.png"),
        "-i", str(wav_path),
        "-vf", "vignette=angle=PI/3.5,fade=t=out:st=3.52:d=0.48,format=yuv420p",
        "-c:v", "libx264", "-pix_fmt", "yuv420p", "-crf", "17", "-preset", "medium",
        "-c:a", "aac", "-b:a", "192k", "-shortest", str(mp4_path),
    ], check=True)
    if not export_game:
        return
    game_dir.mkdir(parents=True, exist_ok=True)
    for old in game_dir.glob("frame_*.jpg"):
        old.unlink()
    subprocess.run([
        ffmpeg, "-y", "-hide_banner", "-loglevel", "error",
        "-i", str(mp4_path), "-vf", f"fps={FPS},scale=1280:720:flags=lanczos",
        "-q:v", "4", str(game_dir / "frame_%04d.jpg"),
    ], check=True)
    shutil.copy2(wav_path, game_dir / "audio.wav")
    count = len(list(game_dir.glob("frame_*.jpg")))
    print(f"EXPORTED {game_dir.name}: {count} frames + audio.wav", flush=True)


def render_variant(name, variant, test=False, export_game=False, encode_only=False):
    variant_dir = OUT / name
    frames_dir = variant_dir / "frames"
    frames_dir.mkdir(parents=True, exist_ok=True)
    background = ROOT / variant["background"]
    clean_path = variant_dir / "clean_plate.png"
    seed = sum(ord(ch) for ch in name) * 131 + 17
    make_clean_plate(background, variant["capsules"], clean_path, seed=seed)
    synthesize_audio(variant_dir / "audio.wav", name, variant)

    if encode_only:
        encode_and_export(find_ffmpeg(), variant_dir, GAME_VIDEO / f"exit_{name}", export_game)
        return

    bpy.ops.wm.read_factory_settings(use_empty=True)
    scene = bpy.context.scene
    set_enum(scene.render, "engine", ("BLENDER_EEVEE_NEXT", "BLENDER_EEVEE", "CYCLES"))
    scene.render.resolution_percentage = 100
    scene.render.fps = FPS
    try_set(scene.render, "use_motion_blur", True)
    try_set(scene.render, "motion_blur_shutter", 0.45)
    try_set(scene.eevee, "taa_render_samples", 12 if test else 20)
    # Filmic / AgX prevents highlight clipping on sunlit white metallic structures
    set_enum(scene.view_settings, "view_transform", ("AgX", "Filmic", "Standard"))
    scene.view_settings.exposure = -0.35

    world = bpy.data.worlds.new("ExitWorld")
    world.use_nodes = True
    world.node_tree.nodes["Background"].inputs["Strength"].default_value = 0.05
    scene.world = world

    setup_camera(scene, variant)
    make_image_plane("CleanBackdrop", clean_path, -30.0)
    setup_lighting(scene, variant)
    setup_ship(scene, variant)
    compositor_mix = setup_compositor(scene, background)

    if test:
        scene.render.resolution_x = 640
        scene.render.resolution_y = 360
        for index, seconds in enumerate((0.0, 0.8, 1.6, 2.4, 3.2), start=1):
            frame = max(1, min(FRAMES, int(round(seconds * FPS)) + 1))
            scene.frame_set(frame)
            if compositor_mix is not None:
                print(f"COMPOSITOR factor frame={frame}: {compositor_mix.inputs['Factor'].default_value}", flush=True)
            path = variant_dir / f"test_{index:02d}_f{frame:04d}.png"
            scene.render.filepath = str(path)
            bpy.ops.render.render(write_still=True)
            print(f"TEST STILL: {path}", flush=True)
        return

    scene.render.resolution_x = 1280
    scene.render.resolution_y = 720
    scene.frame_start = 1
    scene.frame_end = FRAMES
    scene.render.filepath = str(frames_dir / "frame_")
    bpy.ops.render.render(animation=True)
    encode_and_export(find_ffmpeg(), variant_dir, GAME_VIDEO / f"exit_{name}", export_game)


def main():
    argv = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
    parser = argparse.ArgumentParser()
    parser.add_argument("--test", action="store_true")
    parser.add_argument("--encode-only", action="store_true")
    parser.add_argument("--export-game", action="store_true")
    parser.add_argument("--variants", default=",".join(VARIANTS))
    args = parser.parse_args(argv)
    for name in (part.strip() for part in args.variants.split(",") if part.strip()):
        if name not in VARIANTS:
            raise SystemExit(f"Unknown variant: {name}")
        render_variant(name, VARIANTS[name], test=args.test, export_game=args.export_game, encode_only=args.encode_only)


if __name__ == "__main__":
    main()
