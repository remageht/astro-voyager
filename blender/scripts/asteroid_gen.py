"""Deterministic procedural asteroid generator & PBR regolith pipeline (Blender headless).

Usage:
  "C:\\Program Files\\Blender Foundation\\Blender 5.2\\blender.exe" -b -P blender/scripts/asteroid_gen.py

Pipeline features:
1. Procedural Regolith PBR Pipeline:
   - Authentic carbonaceous chondrite / silicate rock shader.
   - Multi-scale micro-craters (bowls with raised impact rims).
   - Organic thermal & tectonic fractures / cleavage fissures.
   - Regolith fine dust / fines accumulation in depressions.
   - Glassy impact melt droplets & chondrules (glossy micro-glints).
2. PBR Texture Baking (2048x2048 24-bit BMP compatible with src/ObjModel.cpp):
   - res/models/asteroid_albedo.bmp (and blender/out/)
   - res/models/asteroid_normal.bmp (tangent space)
   - res/models/asteroid_rough.bmp
   - res/models/asteroid_metal.bmp
3. Optimized Meshes with UV Unwrapping:
   - res/models/asteroid_1.obj, asteroid_2.obj, asteroid_3.obj
   - blender/out/asteroid_1.obj, asteroid_2.obj, asteroid_3.obj (+ glb, mtl)
4. Asteroid Belt Showcase Scene (Cycles):
   - Hero foreground asteroid, secondary companions, and a dense orbital belt swarm.
   - Grazing illumination from distant star casting dramatic space shadows.
   - Deep space starfield & galactic nebula ambient.
   - blender/out/final_showcase/asteroids_showcase.png -> copied to docs/screens/asteroids_showcase.png.
   - preview_variants.png, preview_close.png, asteroid_styles_compare.png, asteroid_lowpoly_variants.png.
"""

import math
import os
import random
import shutil
import struct
import time

import bpy
from mathutils import Vector, noise
import numpy as np

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
BLENDER_DIR = os.path.dirname(SCRIPT_DIR)
REPO_ROOT = os.path.dirname(BLENDER_DIR)
OUT_DIR = os.path.join(BLENDER_DIR, "out")
FINAL_SHOWCASE_DIR = os.path.join(OUT_DIR, "final_showcase")
RES_MODELS_DIR = os.path.join(REPO_ROOT, "res", "models")
DOCS_SCREENS_DIR = os.path.join(REPO_ROOT, "docs", "screens")

VARIANTS = [
    (1, (1.00, 0.90, 0.82)),
    (2, (0.92, 1.00, 0.86)),
    (3, (0.96, 0.84, 1.00)),
]
LOWPOLY_VARIANTS = [
    (11, (1.02, 0.90, 0.92)),
    (12, (0.90, 1.00, 0.88)),
    (13, (0.95, 0.88, 1.02)),
]

SUBDIV = 5
LOWPOLY_SUBDIV = 2
FREQ = 1.7
AMP = 0.42
CRUNCH_FREQ = 3.6
CRUNCH_AMP = 0.14
CRATER_COUNT = 7
SPACING = 3.4
TEX_RES = 2048


def write_bmp24(filepath, rgb_array):
    """Save rgb_array as 24-bit uncompressed BMP (BI_RGB).
    100% compatible with C++ loader in src/ObjModel.cpp.
    rgb_array shape: (H, W, 3). Note: bottom row first in file.
    If array is top-row-first (standard numpy/PIL), pass np.flipud(rgb_array).
    """
    H, W, _ = rgb_array.shape
    row_bytes = W * 3
    padding = (4 - (row_bytes % 4)) % 4
    stride = row_bytes + padding
    image_size = stride * H
    file_size = 54 + image_size

    header = bytearray(54)
    header[0:2] = b'BM'
    struct.pack_into('<I', header, 2, file_size)
    struct.pack_into('<I', header, 10, 54)
    struct.pack_into('<I', header, 14, 40)
    struct.pack_into('<i', header, 18, W)
    struct.pack_into('<i', header, 22, H)
    struct.pack_into('<H', header, 26, 1)
    struct.pack_into('<H', header, 28, 24)
    struct.pack_into('<I', header, 30, 0)
    struct.pack_into('<I', header, 34, image_size)

    if rgb_array.dtype != np.uint8:
        u8 = (np.clip(rgb_array, 0.0, 1.0) * 255.0).astype(np.uint8)
    else:
        u8 = rgb_array
    bgr = u8[:, :, [2, 1, 0]]

    if padding == 0:
        data = bgr.tobytes()
    else:
        pad = b'\x00' * padding
        rows = [bgr[y, :, :].tobytes() + pad for y in range(H)]
        data = b''.join(rows)

    os.makedirs(os.path.dirname(os.path.abspath(filepath)), exist_ok=True)
    with open(filepath, 'wb') as f:
        f.write(header)
        f.write(data)
    print(f"Saved BMP: {filepath} ({W}x{H}, {file_size / 1024 / 1024:.2f} MB)")


def sample_noise(p):
    try:
        return noise.turbulence(p, 5, False)
    except Exception:
        total = 0.0
        amp = 0.5
        freq = 1.0
        for _ in range(5):
            total += amp * (noise.noise(p * freq) + 1.0) * 0.5
            amp *= 0.5
            freq *= 2.0
        return total


def fbm_normalized(mesh, seed, freq, salt):
    off = Vector((seed * 13.7 + salt, seed * 7.3 - salt, seed * 3.1 + salt))
    vals = [sample_noise(v.co * freq + off) for v in mesh.vertices]
    lo, hi = min(vals), max(vals)
    span = max(hi - lo, 1e-6)
    return [(x - lo) / span for x in vals]


def crater_list(seed):
    rnd = random.Random(seed * 101 + 7)
    craters = []
    for _ in range(CRATER_COUNT):
        d = Vector((rnd.uniform(-1, 1), rnd.uniform(-1, 1), rnd.uniform(-1, 1)))
        if d.length < 1e-3:
            d = Vector((0.0, 0.0, 1.0))
        d.normalize()
        craters.append((d, rnd.uniform(0.20, 0.50), rnd.uniform(0.05, 0.11)))
    return craters


def create_procedural_regolith_material():
    """Builds the comprehensive procedural regolith shader network."""
    mat = bpy.data.materials.new("ProceduralRegolithGenerator")
    mat.use_nodes = True
    nodes = mat.node_tree.nodes
    links = mat.node_tree.links
    nodes.clear()

    output = nodes.new("ShaderNodeOutputMaterial")
    output.location = (1800, 0)
    bsdf = nodes.new("ShaderNodeBsdfPrincipled")
    bsdf.location = (1500, 0)
    links.new(bsdf.outputs["BSDF"], output.inputs["Surface"])

    emit = nodes.new("ShaderNodeEmission")
    emit.location = (1500, 300)
    emit.inputs["Strength"].default_value = 1.0

    coord = nodes.new("ShaderNodeTexCoord")
    coord.location = (-1800, 0)

    # 3D domain distortion for organic cracks and crater warping
    dist_noise = nodes.new("ShaderNodeTexNoise")
    dist_noise.location = (-1600, 200)
    dist_noise.inputs["Scale"].default_value = 3.8
    dist_noise.inputs["Detail"].default_value = 3.0
    links.new(coord.outputs["Object"], dist_noise.inputs["Vector"])

    dist_scale = nodes.new("ShaderNodeVectorMath")
    dist_scale.location = (-1450, 200)
    dist_scale.operation = "SCALE"
    dist_scale.inputs["Scale"].default_value = 0.14
    links.new(dist_noise.outputs["Color"], dist_scale.inputs["Vector"])

    dist_add = nodes.new("ShaderNodeVectorMath")
    dist_add.location = (-1300, 200)
    dist_add.operation = "ADD"
    links.new(coord.outputs["Object"], dist_add.inputs[0])
    links.new(dist_scale.outputs["Vector"], dist_add.inputs[1])

    warped_vec = dist_add.outputs["Vector"]
    obj_vec = coord.outputs["Object"]

    # 1. Macro tonal / rock variation
    macro_noise = nodes.new("ShaderNodeTexNoise")
    macro_noise.location = (-1100, 700)
    macro_noise.inputs["Scale"].default_value = 2.6
    macro_noise.inputs["Detail"].default_value = 4.0
    macro_noise.inputs["Roughness"].default_value = 0.55
    links.new(obj_vec, macro_noise.inputs["Vector"])

    # 2. Medium craters (impact structures with rims)
    craters_med = nodes.new("ShaderNodeTexVoronoi")
    craters_med.location = (-1100, 400)
    craters_med.feature = "F1"
    craters_med.inputs["Scale"].default_value = 11.0
    links.new(warped_vec, craters_med.inputs["Vector"])

    ramp_craters_med = nodes.new("ShaderNodeValToRGB")
    ramp_craters_med.location = (-800, 400)
    ramp_craters_med.color_ramp.elements[0].position = 0.0
    ramp_craters_med.color_ramp.elements[0].color = (0.04, 0.04, 0.04, 1.0)
    el_bowl = ramp_craters_med.color_ramp.elements.new(0.38)
    el_bowl.color = (0.38, 0.38, 0.38, 1.0)
    el_rim = ramp_craters_med.color_ramp.elements.new(0.50)
    el_rim.color = (0.95, 0.95, 0.95, 1.0)
    ramp_craters_med.color_ramp.elements[-1].position = 0.65
    ramp_craters_med.color_ramp.elements[-1].color = (0.50, 0.50, 0.50, 1.0)
    links.new(craters_med.outputs["Distance"], ramp_craters_med.inputs["Fac"])

    # 3. Micro craters (dense bombardment pockmarks)
    craters_micro = nodes.new("ShaderNodeTexVoronoi")
    craters_micro.location = (-1100, 100)
    craters_micro.feature = "F1"
    craters_micro.inputs["Scale"].default_value = 32.0
    links.new(obj_vec, craters_micro.inputs["Vector"])

    ramp_craters_micro = nodes.new("ShaderNodeValToRGB")
    ramp_craters_micro.location = (-800, 100)
    ramp_craters_micro.color_ramp.elements[0].position = 0.0
    ramp_craters_micro.color_ramp.elements[0].color = (0.12, 0.12, 0.12, 1.0)
    el_mrim = ramp_craters_micro.color_ramp.elements.new(0.42)
    el_mrim.color = (0.88, 0.88, 0.88, 1.0)
    ramp_craters_micro.color_ramp.elements[-1].position = 0.60
    ramp_craters_micro.color_ramp.elements[-1].color = (0.50, 0.50, 0.50, 1.0)
    links.new(craters_micro.outputs["Distance"], ramp_craters_micro.inputs["Fac"])

    # 4. Fractures / cracks (layered thermal and impact fissures)
    cracks = nodes.new("ShaderNodeTexVoronoi")
    cracks.location = (-1100, -200)
    cracks.feature = "DISTANCE_TO_EDGE"
    cracks.inputs["Scale"].default_value = 16.0
    links.new(warped_vec, cracks.inputs["Vector"])

    ramp_cracks = nodes.new("ShaderNodeValToRGB")
    ramp_cracks.location = (-800, -200)
    ramp_cracks.color_ramp.elements[0].position = 0.0
    ramp_cracks.color_ramp.elements[0].color = (0.02, 0.02, 0.02, 1.0)
    el_slip = ramp_cracks.color_ramp.elements.new(0.06)
    el_slip.color = (0.75, 0.75, 0.75, 1.0)
    ramp_cracks.color_ramp.elements[-1].position = 0.22
    ramp_cracks.color_ramp.elements[-1].color = (0.50, 0.50, 0.50, 1.0)
    links.new(cracks.outputs["Distance"], ramp_cracks.inputs["Fac"])

    # 5. Fine regolith grit / dust powder
    grit = nodes.new("ShaderNodeTexNoise")
    grit.location = (-1100, -500)
    grit.inputs["Scale"].default_value = 85.0
    grit.inputs["Detail"].default_value = 8.0
    grit.inputs["Roughness"].default_value = 0.72
    links.new(obj_vec, grit.inputs["Vector"])

    # 6. Impact Melts & Chondrules (tiny glassy spherical beads)
    melts = nodes.new("ShaderNodeTexVoronoi")
    melts.location = (-1100, -800)
    melts.feature = "F1"
    melts.inputs["Scale"].default_value = 52.0
    links.new(obj_vec, melts.inputs["Vector"])

    melt_mask = nodes.new("ShaderNodeMath")
    melt_mask.location = (-800, -800)
    melt_mask.operation = "LESS_THAN"
    links.new(melts.outputs["Distance"], melt_mask.inputs[0])
    melt_mask.inputs[1].default_value = 0.095

    # --- Combine Height ---
    math_h1 = nodes.new("ShaderNodeMath")
    math_h1.location = (-500, 400)
    math_h1.operation = "MULTIPLY_ADD"
    links.new(ramp_craters_med.outputs["Color"], math_h1.inputs[0])
    math_h1.inputs[1].default_value = 0.38
    math_h1.inputs[2].default_value = -0.19

    math_h2 = nodes.new("ShaderNodeMath")
    math_h2.location = (-500, 100)
    math_h2.operation = "MULTIPLY_ADD"
    links.new(ramp_craters_micro.outputs["Color"], math_h2.inputs[0])
    math_h2.inputs[1].default_value = 0.22
    math_h2.inputs[2].default_value = -0.11

    math_h3 = nodes.new("ShaderNodeMath")
    math_h3.location = (-500, -200)
    math_h3.operation = "MULTIPLY_ADD"
    links.new(ramp_cracks.outputs["Color"], math_h3.inputs[0])
    math_h3.inputs[1].default_value = 0.32
    math_h3.inputs[2].default_value = -0.16

    math_h4 = nodes.new("ShaderNodeMath")
    math_h4.location = (-500, -500)
    math_h4.operation = "MULTIPLY_ADD"
    links.new(grit.outputs["Fac"], math_h4.inputs[0])
    math_h4.inputs[1].default_value = 0.16
    math_h4.inputs[2].default_value = -0.08

    sum1 = nodes.new("ShaderNodeMath")
    sum1.location = (-300, 300)
    sum1.operation = "ADD"
    links.new(math_h1.outputs["Value"], sum1.inputs[0])
    links.new(math_h2.outputs["Value"], sum1.inputs[1])

    sum2 = nodes.new("ShaderNodeMath")
    sum2.location = (-300, -100)
    sum2.operation = "ADD"
    links.new(math_h3.outputs["Value"], sum2.inputs[0])
    links.new(math_h4.outputs["Value"], sum2.inputs[1])

    sum_all = nodes.new("ShaderNodeMath")
    sum_all.location = (-100, 100)
    sum_all.operation = "ADD"
    links.new(sum1.outputs["Value"], sum_all.inputs[0])
    links.new(sum2.outputs["Value"], sum_all.inputs[1])

    total_height = nodes.new("ShaderNodeMath")
    total_height.location = (100, 0)
    total_height.operation = "ADD"
    links.new(sum_all.outputs["Value"], total_height.inputs[0])
    links.new(melt_mask.outputs["Value"], total_height.inputs[1])

    bump = nodes.new("ShaderNodeBump")
    bump.location = (400, -200)
    bump.inputs["Strength"].default_value = 1.0
    bump.inputs["Distance"].default_value = 0.08
    links.new(total_height.outputs["Value"], bump.inputs["Height"])
    links.new(bump.outputs["Normal"], bsdf.inputs["Normal"])

    # --- Combine Albedo ---
    ramp_macro_col = nodes.new("ShaderNodeValToRGB")
    ramp_macro_col.location = (-800, 700)
    ramp_macro_col.color_ramp.elements[0].position = 0.2
    ramp_macro_col.color_ramp.elements[0].color = (0.11, 0.105, 0.10, 1.0)
    ramp_macro_col.color_ramp.elements[1].position = 0.8
    ramp_macro_col.color_ramp.elements[1].color = (0.22, 0.19, 0.15, 1.0)
    links.new(macro_noise.outputs["Fac"], ramp_macro_col.inputs["Fac"])

    mix_rims = nodes.new("ShaderNodeMix")
    mix_rims.location = (100, 600)
    mix_rims.data_type = "RGBA"
    links.new(ramp_macro_col.outputs["Color"], mix_rims.inputs["A"])
    mix_rims.inputs["B"].default_value = (0.34, 0.32, 0.30, 1.0)

    rim_factor = nodes.new("ShaderNodeMath")
    rim_factor.location = (-100, 700)
    rim_factor.operation = "SMOOTH_MAX"
    rim_factor.inputs[1].default_value = 0.0
    links.new(math_h1.outputs["Value"], rim_factor.inputs[0])
    links.new(rim_factor.outputs["Value"], mix_rims.inputs["Factor"])

    mix_cracks = nodes.new("ShaderNodeMix")
    mix_cracks.location = (400, 500)
    mix_cracks.data_type = "RGBA"
    links.new(mix_rims.outputs["Result"], mix_cracks.inputs["A"])
    mix_cracks.inputs["B"].default_value = (0.045, 0.04, 0.038, 1.0)
    crack_depth = nodes.new("ShaderNodeMath")
    crack_depth.location = (200, 450)
    crack_depth.operation = "LESS_THAN"
    links.new(ramp_cracks.outputs["Color"], crack_depth.inputs[0])
    crack_depth.inputs[1].default_value = 0.15
    links.new(crack_depth.outputs["Value"], mix_cracks.inputs["Factor"])

    mix_albedo = nodes.new("ShaderNodeMix")
    mix_albedo.location = (700, 400)
    mix_albedo.data_type = "RGBA"
    links.new(mix_cracks.outputs["Result"], mix_albedo.inputs["A"])
    mix_albedo.inputs["B"].default_value = (0.032, 0.032, 0.032, 1.0)
    links.new(melt_mask.outputs["Value"], mix_albedo.inputs["Factor"])
    links.new(mix_albedo.outputs["Result"], bsdf.inputs["Base Color"])

    # --- Combine Roughness ---
    ramp_rough = nodes.new("ShaderNodeValToRGB")
    ramp_rough.location = (400, 150)
    ramp_rough.color_ramp.elements[0].position = 0.2
    ramp_rough.color_ramp.elements[0].color = (0.72, 0.72, 0.72, 1.0)
    ramp_rough.color_ramp.elements[1].position = 0.8
    ramp_rough.color_ramp.elements[1].color = (0.97, 0.97, 0.97, 1.0)
    links.new(macro_noise.outputs["Fac"], ramp_rough.inputs["Fac"])

    mix_rough = nodes.new("ShaderNodeMix")
    mix_rough.location = (700, 150)
    mix_rough.data_type = "FLOAT"
    links.new(ramp_rough.outputs["Color"], mix_rough.inputs[2])
    mix_rough.inputs[3].default_value = 0.28
    links.new(melt_mask.outputs["Value"], mix_rough.inputs[0])
    links.new(mix_rough.outputs[0], bsdf.inputs["Roughness"])

    # Metallic: tiny 0.02 base with 0.15 on chondrule melts
    mix_metal = nodes.new("ShaderNodeMix")
    mix_metal.location = (700, -50)
    mix_metal.data_type = "FLOAT"
    mix_metal.inputs[2].default_value = 0.02
    mix_metal.inputs[3].default_value = 0.18
    links.new(melt_mask.outputs["Value"], mix_metal.inputs[0])
    links.new(mix_metal.outputs[0], bsdf.inputs["Metallic"])

    return mat, emit, mix_albedo, mix_rough, mix_metal, bump


def bake_pbr_textures(res=TEX_RES):
    """Bakes the procedural regolith shader into 24-bit BMP maps."""
    print(f"=== Baking Asteroid PBR Textures ({res}x{res}) ===")
    t_start = time.time()

    bpy.ops.wm.read_factory_settings(use_empty=True)
    bpy.ops.mesh.primitive_ico_sphere_add(subdivisions=5, radius=1.0)
    sphere = bpy.context.active_object
    bpy.ops.object.mode_set(mode='EDIT')
    bpy.ops.uv.sphere_project(clip_to_bounds=True)
    bpy.ops.object.mode_set(mode='OBJECT')

    mat, emit, albedo_node, rough_node, metal_node, bump_node = create_procedural_regolith_material()
    sphere.data.materials.append(mat)
    nodes = mat.node_tree.nodes
    links = mat.node_tree.links
    output = nodes.get("Material Output")

    scene = bpy.context.scene
    scene.render.engine = 'CYCLES'
    scene.cycles.device = 'CPU'
    scene.cycles.samples = 1
    scene.render.bake.use_selected_to_active = False

    bake_img = bpy.data.images.new("BakeTarget", res, res)
    tex_node = nodes.new("ShaderNodeTexImage")
    tex_node.image = bake_img
    nodes.active = tex_node

    # 1. Albedo (Emit bake for pure unlit color)
    print("  Baking Albedo map...")
    t0 = time.time()
    links.new(albedo_node.outputs["Result"], emit.inputs["Color"])
    links.new(emit.outputs["Emission"], output.inputs["Surface"])
    scene.cycles.bake_type = 'EMIT'
    bpy.ops.object.bake(type='EMIT')
    px_albedo = np.array(bake_img.pixels[:], dtype=np.float32).reshape((res, res, 4))[:, :, :3]
    albedo_data = np.flipud(px_albedo)
    print(f"  Albedo done in {time.time() - t0:.2f} s")

    # 2. Roughness (Emit bake)
    print("  Baking Roughness map...")
    t0 = time.time()
    links.new(rough_node.outputs[0], emit.inputs["Color"])
    links.new(emit.outputs["Emission"], output.inputs["Surface"])
    scene.cycles.bake_type = 'EMIT'
    bpy.ops.object.bake(type='EMIT')
    px_rough = np.array(bake_img.pixels[:], dtype=np.float32).reshape((res, res, 4))[:, :, :3]
    rough_data = np.flipud(px_rough)
    print(f"  Roughness done in {time.time() - t0:.2f} s")

    # 3. Metallic (Emit bake)
    print("  Baking Metallic map...")
    t0 = time.time()
    links.new(metal_node.outputs[0], emit.inputs["Color"])
    links.new(emit.outputs["Emission"], output.inputs["Surface"])
    scene.cycles.bake_type = 'EMIT'
    bpy.ops.object.bake(type='EMIT')
    px_metal = np.array(bake_img.pixels[:], dtype=np.float32).reshape((res, res, 4))[:, :, :3]
    metal_data = np.flipud(px_metal)
    print(f"  Metallic done in {time.time() - t0:.2f} s")

    # 4. Tangent Normal map
    print("  Baking Tangent Normal map...")
    t0 = time.time()
    bsdf = nodes.get("Principled BSDF")
    links.new(bsdf.outputs["BSDF"], output.inputs["Surface"])
    scene.cycles.bake_type = 'NORMAL'
    scene.render.bake.normal_space = 'TANGENT'
    bpy.ops.object.bake(type='NORMAL')
    px_normal = np.array(bake_img.pixels[:], dtype=np.float32).reshape((res, res, 4))[:, :, :3]
    normal_data = np.flipud(px_normal)
    print(f"  Normal done in {time.time() - t0:.2f} s")

    # Save to res/models/
    write_bmp24(os.path.join(RES_MODELS_DIR, "asteroid_albedo.bmp"), albedo_data)
    write_bmp24(os.path.join(RES_MODELS_DIR, "asteroid_normal.bmp"), normal_data)
    write_bmp24(os.path.join(RES_MODELS_DIR, "asteroid_rough.bmp"), rough_data)
    write_bmp24(os.path.join(RES_MODELS_DIR, "asteroid_metal.bmp"), metal_data)

    # Save identical copies to blender/out/
    write_bmp24(os.path.join(OUT_DIR, "asteroid_albedo.bmp"), albedo_data)
    write_bmp24(os.path.join(OUT_DIR, "asteroid_normal.bmp"), normal_data)
    write_bmp24(os.path.join(OUT_DIR, "asteroid_rough.bmp"), rough_data)
    write_bmp24(os.path.join(OUT_DIR, "asteroid_metal.bmp"), metal_data)

    print(f"=== All Asteroid PBR Textures Baked & Saved in {time.time() - t_start:.2f} s ===\n")


def create_asteroid_pbr_material():
    """Creates a Principled BSDF material referencing the baked PBR BMP maps."""
    mat = bpy.data.materials.new("AsteroidPBR")
    mat.use_nodes = True
    nodes = mat.node_tree.nodes
    links = mat.node_tree.links
    nodes.clear()

    output = nodes.new("ShaderNodeOutputMaterial")
    output.location = (800, 0)
    bsdf = nodes.new("ShaderNodeBsdfPrincipled")
    bsdf.location = (500, 0)
    links.new(bsdf.outputs["BSDF"], output.inputs["Surface"])

    # Albedo texture
    albedo_path = os.path.join(RES_MODELS_DIR, "asteroid_albedo.bmp")
    img_albedo = bpy.data.images.load(albedo_path, check_existing=True)
    tex_albedo = nodes.new("ShaderNodeTexImage")
    tex_albedo.location = (100, 300)
    tex_albedo.image = img_albedo
    links.new(tex_albedo.outputs["Color"], bsdf.inputs["Base Color"])

    # Roughness texture
    rough_path = os.path.join(RES_MODELS_DIR, "asteroid_rough.bmp")
    img_rough = bpy.data.images.load(rough_path, check_existing=True)
    img_rough.colorspace_settings.name = 'Non-Color'
    tex_rough = nodes.new("ShaderNodeTexImage")
    tex_rough.location = (100, 50)
    tex_rough.image = img_rough
    links.new(tex_rough.outputs["Color"], bsdf.inputs["Roughness"])

    # Metallic texture
    metal_path = os.path.join(RES_MODELS_DIR, "asteroid_metal.bmp")
    if os.path.exists(metal_path):
        img_metal = bpy.data.images.load(metal_path, check_existing=True)
        img_metal.colorspace_settings.name = 'Non-Color'
        tex_metal = nodes.new("ShaderNodeTexImage")
        tex_metal.location = (100, -200)
        tex_metal.image = img_metal
        links.new(tex_metal.outputs["Color"], bsdf.inputs["Metallic"])

    # Normal map texture
    normal_path = os.path.join(RES_MODELS_DIR, "asteroid_normal.bmp")
    img_normal = bpy.data.images.load(normal_path, check_existing=True)
    img_normal.colorspace_settings.name = 'Non-Color'
    tex_normal = nodes.new("ShaderNodeTexImage")
    tex_normal.location = (0, -450)
    tex_normal.image = img_normal

    norm_map_node = nodes.new("ShaderNodeNormalMap")
    norm_map_node.location = (260, -450)
    norm_map_node.space = 'TANGENT'
    norm_map_node.inputs["Strength"].default_value = 1.0
    links.new(tex_normal.outputs["Color"], norm_map_node.inputs["Color"])
    links.new(norm_map_node.outputs["Normal"], bsdf.inputs["Normal"])

    return mat


def make_asteroid(seed, axes, pbr_mat=None):
    """Generates an asteroid mesh with UV unwrapping and PBR material."""
    bpy.ops.mesh.primitive_ico_sphere_add(subdivisions=SUBDIV, radius=1.0)
    obj = bpy.context.active_object
    obj.name = "Asteroid_%d" % seed
    mesh = obj.data
    mesh.name = "AsteroidMesh_%d" % seed

    # Spherical UV unwrapping
    bpy.ops.object.mode_set(mode='EDIT')
    bpy.ops.uv.sphere_project(clip_to_bounds=True)
    bpy.ops.object.mode_set(mode='OBJECT')

    t = fbm_normalized(mesh, seed, FREQ, 0.0)
    c = fbm_normalized(mesh, seed, CRUNCH_FREQ, 5.0)
    craters = crater_list(seed)
    for i, v in enumerate(mesh.vertices):
        n = v.co.normalized()
        r = 1.0 + AMP * (t[i] - 0.5) + CRUNCH_AMP * (c[i] - 0.5)
        for cd, cr, depth in craters:
            ang = math.acos(max(-1.0, min(1.0, n.dot(cd))))
            if ang < cr:
                u = ang / cr
                bowl = 0.5 + 0.5 * math.cos(math.pi * u)
                rim = math.exp(-((u - 0.82) / 0.18) ** 2) * 0.06
                r -= depth * bowl
                r += depth * rim
        v.co = Vector((n.x * r * axes[0], n.y * r * axes[1], n.z * r * axes[2]))

    for poly in mesh.polygons:
        poly.use_smooth = True
    mesh.update()

    if pbr_mat is not None:
        obj.data.materials.append(pbr_mat)
    return obj


def make_lowpoly_asteroid(seed, axes):
    bpy.ops.mesh.primitive_ico_sphere_add(subdivisions=LOWPOLY_SUBDIV, radius=1.0)
    obj = bpy.context.active_object
    obj.name = "AsteroidLowPoly_%d" % seed
    mesh = obj.data
    mesh.name = "AsteroidLowPolyMesh_%d" % seed

    rnd = random.Random(seed * 313 + 11)
    t = fbm_normalized(mesh, seed, 1.05, 2.0)
    c = fbm_normalized(mesh, seed, 2.3, 9.0)
    craters = []
    for _ in range(4):
        d = Vector((rnd.uniform(-1, 1), rnd.uniform(-1, 1), rnd.uniform(-1, 1)))
        if d.length < 1e-3:
            d = Vector((0.0, 0.0, 1.0))
        d.normalize()
        craters.append((d, rnd.uniform(0.25, 0.50), rnd.uniform(0.10, 0.16)))
    for i, v in enumerate(mesh.vertices):
        n = v.co.normalized()
        r = 1.0 + 0.30 * (t[i] - 0.5) + 0.10 * (c[i] - 0.5) + rnd.uniform(-0.055, 0.055)
        for cd, cr, depth in craters:
            ang = math.acos(max(-1.0, min(1.0, n.dot(cd))))
            if ang < cr:
                u = ang / cr
                bowl = 0.5 + 0.5 * math.cos(math.pi * u)
                rim = math.exp(-((u - 0.82) / 0.18) ** 2) * 0.05
                r -= depth * bowl
                r += depth * rim
        v.co = Vector((n.x * r * axes[0], n.y * r * axes[1], n.z * r * axes[2]))
    for poly in mesh.polygons:
        poly.use_smooth = False
    mesh.update()

    mat = bpy.data.materials.new("AsteroidLowPoly_%d" % seed)
    mat.use_nodes = True
    bsdf = mat.node_tree.nodes.get("Principled BSDF")
    if bsdf is not None:
        bsdf.inputs["Base Color"].default_value = (0.30, 0.285, 0.27, 1.0)
        bsdf.inputs["Roughness"].default_value = 0.82
    obj.data.materials.append(mat)
    return obj


def add_label(body, loc, cam_loc, size=0.42):
    bpy.ops.object.text_add(location=loc)
    tobj = bpy.context.active_object
    tobj.name = "Label_" + body.replace(" ", "_").replace("-", "").replace("(", "").replace(")", "")
    tobj.data.body = body
    tobj.data.align_x = "CENTER"
    tobj.data.align_y = "TOP"
    tobj.data.size = size
    direction = Vector(cam_loc) - Vector(loc)
    tobj.rotation_euler = direction.to_track_quat("Z", "Y").to_euler()
    mat = bpy.data.materials.new(tobj.name + "_Mat")
    mat.use_nodes = True
    bsdf = mat.node_tree.nodes.get("Principled BSDF")
    if bsdf is not None:
        bsdf.inputs["Base Color"].default_value = (0.85, 0.90, 1.0, 1.0)
        bsdf.inputs["Emission Color"].default_value = (0.85, 0.90, 1.0, 1.0)
        bsdf.inputs["Emission Strength"].default_value = 1.2
    tobj.data.materials.append(mat)
    return tobj


def setup_space_world(scene):
    """Sets up deep cosmic void with starfield and distant galactic nebula."""
    world = bpy.data.worlds.new("SpaceWorld")
    scene.world = world
    world.use_nodes = True
    nodes = world.node_tree.nodes
    links = world.node_tree.links
    nodes.clear()

    output = nodes.new("ShaderNodeOutputWorld")
    output.location = (700, 0)
    bg = nodes.new("ShaderNodeBackground")
    bg.location = (400, 0)
    bg.inputs[0].default_value = (0.003, 0.004, 0.006, 1.0)
    bg.inputs[1].default_value = 1.0

    coord = nodes.new("ShaderNodeTexCoord")
    coord.location = (-600, 0)

    # Pin-point distant stars
    star_vor = nodes.new("ShaderNodeTexVoronoi")
    star_vor.location = (-300, 200)
    star_vor.feature = "F1"
    star_vor.inputs["Scale"].default_value = 180.0
    links.new(coord.outputs["Generated"], star_vor.inputs["Vector"])

    star_bright = nodes.new("ShaderNodeMath")
    star_bright.location = (-100, 200)
    star_bright.operation = "LESS_THAN"
    star_bright.inputs[1].default_value = 0.015
    links.new(star_vor.outputs["Distance"], star_bright.inputs[0])

    star_emit = nodes.new("ShaderNodeEmission")
    star_emit.location = (120, 200)
    star_emit.inputs["Color"].default_value = (0.95, 0.98, 1.0, 1.0)
    star_emit.inputs["Strength"].default_value = 2.5
    links.new(star_bright.outputs["Value"], star_emit.inputs["Strength"])

    # Faint galactic dust band
    neb_noise = nodes.new("ShaderNodeTexNoise")
    neb_noise.location = (-300, -200)
    neb_noise.inputs["Scale"].default_value = 1.6
    neb_noise.inputs["Detail"].default_value = 4.0
    links.new(coord.outputs["Generated"], neb_noise.inputs["Vector"])

    neb_ramp = nodes.new("ShaderNodeValToRGB")
    neb_ramp.location = (-100, -200)
    neb_ramp.color_ramp.elements[0].position = 0.45
    neb_ramp.color_ramp.elements[0].color = (0.0, 0.0, 0.0, 1.0)
    neb_ramp.color_ramp.elements[1].position = 0.8
    neb_ramp.color_ramp.elements[1].color = (0.006, 0.009, 0.018, 1.0)
    links.new(neb_noise.outputs["Fac"], neb_ramp.inputs["Fac"])

    add_world = nodes.new("ShaderNodeAddShader")
    add_world.location = (550, 100)
    links.new(bg.outputs["Background"], add_world.inputs[0])
    links.new(star_emit.outputs["Emission"], add_world.inputs[1])
    links.new(add_world.outputs["Shader"], output.inputs["Surface"])


def setup_scene():
    scene = bpy.context.scene
    scene.render.engine = "CYCLES"
    scene.cycles.device = "CPU"
    scene.cycles.samples = 64
    scene.render.resolution_x = 720
    scene.render.resolution_y = 480
    scene.render.image_settings.file_format = "PNG"

    setup_space_world(scene)

    bpy.ops.object.light_add(type="SUN", location=(8.0, -10.0, 6.0))
    sun = bpy.context.active_object
    sun.name = "DistantStarLight"
    sun.data.energy = 6.0
    sun.data.color = (1.0, 0.97, 0.93)
    sun.rotation_euler = (0.65, 0.20, 0.85)

    # Soft cool fill / rim from deep space
    bpy.ops.object.light_add(type="SUN", location=(-8.0, 8.0, -4.0))
    fill = bpy.context.active_object
    fill.name = "GalacticFillLight"
    fill.data.energy = 0.25
    fill.data.color = (0.65, 0.78, 1.0)
    fill.rotation_euler = (-0.75, -0.25, -2.2)

    bpy.ops.object.camera_add(location=(0.0, 0.0, 0.0))
    cam = bpy.context.active_object
    cam.data.lens = 50
    scene.camera = cam
    return cam


def aim(cam, loc, target):
    cam.location = Vector(loc)
    direction = Vector(target) - cam.location
    cam.rotation_euler = direction.to_track_quat("-Z", "Y").to_euler()


def render_to(path, samples=64, res_x=None, res_y=None):
    os.makedirs(os.path.dirname(os.path.abspath(path)), exist_ok=True)
    scene = bpy.context.scene
    scene.cycles.samples = samples
    old_x, old_y = scene.render.resolution_x, scene.render.resolution_y
    if res_x is not None:
        scene.render.resolution_x = res_x
    if res_y is not None:
        scene.render.resolution_y = res_y

    scene.render.filepath = path
    bpy.ops.render.render(write_still=True)
    print("RENDERED:", path)

    if res_x is not None or res_y is not None:
        scene.render.resolution_x = old_x
        scene.render.resolution_y = old_y


def export_variant(obj, base):
    """Exports OBJ and GLB to both blender/out/ and res/models/."""
    for o in bpy.context.scene.objects:
        o.select_set(False)
    obj.select_set(True)
    bpy.context.view_layer.objects.active = obj

    # 1. blender/out/
    obj_path_out = os.path.join(OUT_DIR, base + ".obj")
    try:
        bpy.ops.wm.obj_export(
            filepath=obj_path_out,
            export_selected_objects=True,
            export_uv=True,
            export_normals=True,
            export_materials=True,
            export_triangulated_mesh=True,
            apply_transform=True,
        )
    except Exception as e:
        print("wm.obj_export fallback:", e)
        bpy.ops.wm.obj_export(filepath=obj_path_out, export_selected_objects=True)
    print("EXPORTED:", obj_path_out)

    glb_path_out = os.path.join(OUT_DIR, base + ".glb")
    try:
        bpy.ops.export_scene.gltf(
            filepath=glb_path_out, export_format="GLB", use_selection=True
        )
        print("EXPORTED:", glb_path_out)
    except Exception as exc:
        print("GLB export failed:", exc)

    # 2. res/models/ (for primary asteroids 1..3)
    if not base.startswith("asteroid_lowpoly"):
        res_obj = os.path.join(RES_MODELS_DIR, base + ".obj")
        shutil.copyfile(obj_path_out, res_obj)
        print("COPIED TO RES:", res_obj)


def build_asteroid_belt_showcase(objs, pbr_mat, cam):
    """Sets up a stunning, cinematic asteroid belt scene lit by a distant star."""
    print("=== Setting up Asteroid Belt Showcase Scene ===")
    rnd = random.Random(42)

    # Hero Asteroid in the foreground
    hero = objs[0]
    hero.location = (0.45, -0.85, -0.15)
    hero.rotation_euler = (0.35, 0.65, 0.20)
    hero.scale = (1.25, 1.25, 1.25)
    hero.hide_render = False

    # Secondary Asteroid in midground left
    second = objs[1]
    second.location = (-2.8, 3.2, 0.6)
    second.rotation_euler = (-0.45, 0.85, 1.1)
    second.scale = (0.95, 0.95, 0.95)
    second.hide_render = False

    # Tertiary Asteroid in mid-upper background
    third = objs[2]
    third.location = (3.6, 6.5, 1.8)
    third.rotation_euler = (1.2, -0.3, 0.7)
    third.scale = (1.1, 1.1, 1.1)
    third.hide_render = False

    # Swarm of orbital belt asteroid fragments stretching into deep space
    belt_objs = []
    base_meshes = [o.data for o in objs]
    num_belt = 64
    for i in range(num_belt):
        mesh = rnd.choice(base_meshes)
        ast = bpy.data.objects.new("Belt_%d" % i, mesh)
        bpy.context.collection.objects.link(ast)

        # Distribute along a curved orbital stream receding into depth
        t = i / float(num_belt)
        dist = 4.0 + t * 45.0
        angle = -0.4 + t * 1.6 + rnd.uniform(-0.15, 0.15)
        x = math.sin(angle) * dist + rnd.uniform(-1.5, 1.5)
        y = math.cos(angle) * dist + rnd.uniform(-2.0, 2.0)
        z = -1.2 + t * 3.5 + rnd.uniform(-1.2, 1.2)
        ast.location = (x, y, z)

        sz = rnd.uniform(0.08, 0.42) * (1.0 + t * 0.5)
        ast.scale = (sz * rnd.uniform(0.85, 1.15),
                     sz * rnd.uniform(0.85, 1.15),
                     sz * rnd.uniform(0.85, 1.15))
        ast.rotation_euler = (rnd.uniform(0, 6.28), rnd.uniform(0, 6.28), rnd.uniform(0, 6.28))
        if not ast.data.materials:
            ast.data.materials.append(pbr_mat)
        belt_objs.append(ast)

    # Distant Star Flare / Sun Representation in background
    bpy.ops.mesh.primitive_uv_sphere_add(segments=24, ring_count=16, radius=1.2, location=(18.0, 52.0, 16.0))
    star_obj = bpy.context.active_object
    star_obj.name = "DistantStarDisk"
    star_mat = bpy.data.materials.new("DistantStarDiskMat")
    star_mat.use_nodes = True
    star_bsdf = star_mat.node_tree.nodes.get("Principled BSDF")
    if star_bsdf:
        star_bsdf.inputs["Emission Color"].default_value = (1.0, 0.98, 0.92, 1.0)
        star_bsdf.inputs["Emission Strength"].default_value = 18.0
    star_obj.data.materials.append(star_mat)

    # Camera placement for the showcase
    cam.data.lens = 65
    cam_pos = (-0.85, -5.2, 1.1)
    cam_target = (0.25, 0.0, 0.1)
    aim(cam, cam_pos, cam_target)

    # Lighting tuning for high-contrast space lighting
    sun = bpy.data.objects.get("DistantStarLight")
    if sun:
        sun.data.energy = 8.0
        sun.data.color = (1.0, 0.98, 0.92)
        # Direct light glancing from the upper right
        dir_to_star = Vector((18.0, 52.0, 16.0)) - Vector((0, 0, 0))
        sun.rotation_euler = (-dir_to_star).to_track_quat("-Z", "Y").to_euler()

    # Render showcase
    showcase_path = os.path.join(FINAL_SHOWCASE_DIR, "asteroids_showcase.png")
    render_to(showcase_path, samples=128, res_x=1920, res_y=1080)

    # Copy to docs/screens/
    docs_showcase = os.path.join(DOCS_SCREENS_DIR, "asteroids_showcase.png")
    os.makedirs(DOCS_SCREENS_DIR, exist_ok=True)
    shutil.copyfile(showcase_path, docs_showcase)
    print("COPIED SHOWCASE TO DOCS:", docs_showcase)

    # Clean up belt objects to restore default layout for previews & blend file
    for bo in belt_objs:
        bpy.data.objects.remove(bo, do_unlink=True)
    bpy.data.objects.remove(star_obj, do_unlink=True)


def main():
    os.makedirs(OUT_DIR, exist_ok=True)
    os.makedirs(FINAL_SHOWCASE_DIR, exist_ok=True)
    os.makedirs(RES_MODELS_DIR, exist_ok=True)
    os.makedirs(DOCS_SCREENS_DIR, exist_ok=True)

    print("BLENDER:", bpy.app.version_string)

    # Step 1: Bake PBR texture set (Albedo, Normal, Roughness, Metallic)
    bake_pbr_textures(res=TEX_RES)

    # Step 2: Build game-ready scene with PBR textured asteroids
    bpy.ops.wm.read_factory_settings(use_empty=True)
    cam = setup_scene()
    pbr_mat = create_asteroid_pbr_material()

    objs = []
    for i, (seed, axes) in enumerate(VARIANTS):
        obj = make_asteroid(seed, axes, pbr_mat=pbr_mat)
        obj.location = (i * SPACING, 0.0, 0.0)
        objs.append(obj)

    low_objs = []
    for i, (seed, axes) in enumerate(LOWPOLY_VARIANTS):
        obj = make_lowpoly_asteroid(seed, axes)
        obj.location = (i * SPACING, 0.0, -SPACING)
        obj.hide_render = True
        low_objs.append(obj)

    # Export variants to blender/out/ and res/models/
    for (seed, _axes), obj in zip(VARIANTS, objs):
        export_variant(obj, "asteroid_%d" % seed)
    for (seed, _axes), obj in zip(LOWPOLY_VARIANTS, low_objs):
        export_variant(obj, "asteroid_lowpoly_%d" % seed)

    # Step 3: Render Asteroid Belt Final Showcase
    build_asteroid_belt_showcase(objs, pbr_mat, cam)

    # Step 4: Render QA Stills (preview_variants, preview_close, styles_compare, lowpoly_variants)
    print("=== Rendering QA Stills ===")
    for obj in objs:
        obj.scale = (1.0, 1.0, 1.0)
        obj.rotation_euler = (0.0, 0.0, 0.0)
        obj.hide_render = False
    for i, obj in enumerate(objs):
        obj.location = (i * SPACING, 0.0, 0.0)

    sun = bpy.data.objects.get("DistantStarLight")
    if sun:
        sun.rotation_euler = (0.7, 0.15, 0.9)
        sun.data.energy = 4.5

    mid = (len(VARIANTS) - 1) * SPACING * 0.5
    cam.data.lens = 35
    aim(cam, (mid + 2.2, -8.6, 3.3), (mid, 0.0, 0.0))
    render_to(os.path.join(OUT_DIR, "preview_variants.png"), samples=64, res_x=720, res_y=480)

    cam.data.lens = 50
    aim(cam, (1.75, -2.15, 0.95), (0.0, 0.0, 0.0))
    render_to(os.path.join(OUT_DIR, "preview_close.png"), samples=64, res_x=720, res_y=480)

    for obj in objs[1:]:
        obj.hide_render = True
    for obj in low_objs[1:]:
        obj.hide_render = True
    objs[0].location = (-1.75, 0.0, 0.0)
    low_objs[0].location = (1.75, 0.0, 0.0)
    low_objs[0].hide_render = False
    compare_cam = (0.0, -7.6, 2.9)
    cam.data.lens = 40
    aim(cam, compare_cam, (0.0, 0.0, -0.25))
    add_label("PBR RUBBLE-PILE\n(TEXTURED)", (-1.75, -0.45, -1.35), compare_cam, size=0.22)
    add_label("LOW-POLY\nFLAT-SHADED", (1.75, -0.45, -1.35), compare_cam, size=0.22)
    render_to(os.path.join(OUT_DIR, "asteroid_styles_compare.png"), samples=64, res_x=720, res_y=480)

    for obj in list(bpy.context.scene.objects):
        if obj.name.startswith("Label_"):
            bpy.data.objects.remove(obj, do_unlink=True)
    for obj in objs:
        obj.hide_render = True
    for i, obj in enumerate(low_objs):
        obj.location = (i * SPACING, 0.0, 0.0)
        obj.hide_render = False
    cam.data.lens = 35
    aim(cam, (mid + 2.2, -8.6, 3.3), (mid, 0.0, 0.0))
    render_to(os.path.join(OUT_DIR, "asteroid_lowpoly_variants.png"), samples=64, res_x=720, res_y=480)

    for i, obj in enumerate(objs):
        obj.location = (i * SPACING, 0.0, 0.0)
        obj.hide_render = False
    for i, obj in enumerate(low_objs):
        obj.location = (i * SPACING, 0.0, SPACING)
        obj.hide_render = False

    blend_path = os.path.join(OUT_DIR, "asteroids.blend")
    bpy.ops.wm.save_as_mainfile(filepath=blend_path)
    print("SAVED BLEND:", blend_path)
    print("=== PIPELINE COMPLETE ===")


if __name__ == "__main__":
    main()
