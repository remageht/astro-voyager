"""Deterministic PBR 3D asset generation and baking for astro-voyager:
- Style 0: "Dart" (sleek high-speed interceptor/scout)
- Style 2: "Shuttle" (heavy tactical shuttle/explorer)

Produces:
  res/models/dart.obj
  res/models/dart_albedo.bmp, dart_normal.bmp, dart_rough.bmp, dart_metal.bmp, dart_emissive.bmp
  res/models/shuttle.obj
  res/models/shuttle_albedo.bmp, shuttle_normal.bmp, shuttle_rough.bmp, shuttle_metal.bmp, shuttle_emissive.bmp
  blender/out/final_showcase/dart_showcase.png  (1280x720 Cycles)
  blender/out/final_showcase/shuttle_showcase.png  (1280x720 Cycles)
  docs/screens/dart_showcase.png
  docs/screens/shuttle_showcase.png
"""
import os
import sys
import math
import struct
import shutil
import numpy as np
import bpy
import bmesh
from mathutils import Vector, Matrix

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
PROJECT_ROOT = os.path.dirname(SCRIPT_DIR)  # blender directory
REPO_ROOT = os.path.dirname(PROJECT_ROOT)   # astro-voyager repository root
RES_MODELS = os.path.join(REPO_ROOT, "res", "models")
OUT_SHOWCASE = os.path.join(PROJECT_ROOT, "out", "final_showcase")
DOCS_SCREENS = os.path.join(REPO_ROOT, "docs", "screens")

os.makedirs(RES_MODELS, exist_ok=True)
os.makedirs(OUT_SHOWCASE, exist_ok=True)
os.makedirs(DOCS_SCREENS, exist_ok=True)

# ---------------------------------------------------------------------------
# 24-bit uncompressed BMP writer matching src/ObjModel.cpp
# ---------------------------------------------------------------------------
def write_bmp24(filepath, rgb_array):
    """Writes 24-bit uncompressed BMP matching C++ loadBmp in src/ObjModel.cpp.
    rgb_array shape: (H, W, 3).
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
        
    with open(filepath, 'wb') as f:
        f.write(header)
        f.write(data)
    print(f"Saved BMP: {os.path.basename(filepath)} ({W}x{H}, {file_size / (1024 * 1024):.2f} MB)")


def save_blender_image_as_bmp(img, filepath):
    W, H = img.size
    pixels = np.array(img.pixels[:], dtype=np.float32).reshape(H, W, 4)
    rgb = pixels[:, :, :3]
    # Blender row 0 is bottom (v=0). C++ loadBmp reads row 0 as v=1.
    # Flip upside down to align UVs correctly.
    rgb_flipped = np.flipud(rgb)
    write_bmp24(filepath, rgb_flipped)


# ---------------------------------------------------------------------------
# Material Utilities
# ---------------------------------------------------------------------------
def make_pbr_material(name, base_color, metallic, roughness, emission=(0,0,0), emission_strength=0.0, bump_type=None):
    mat = bpy.data.materials.new(name)
    mat.use_nodes = True
    nodes = mat.node_tree.nodes
    links = mat.node_tree.links
    nodes.clear()
    
    out = nodes.new("ShaderNodeOutputMaterial")
    bsdf = nodes.new("ShaderNodeBsdfPrincipled")
    links.new(bsdf.outputs["BSDF"], out.inputs["Surface"])
    
    bsdf.inputs["Base Color"].default_value = (base_color[0], base_color[1], base_color[2], 1.0)
    bsdf.inputs["Metallic"].default_value = metallic
    bsdf.inputs["Roughness"].default_value = roughness
    bsdf.inputs["Emission Color"].default_value = (emission[0], emission[1], emission[2], 1.0)
    bsdf.inputs["Emission Strength"].default_value = emission_strength
    
    if bump_type == 'panels':
        tex_coord = nodes.new("ShaderNodeTexCoord")
        voronoi = nodes.new("ShaderNodeTexVoronoi")
        voronoi.feature = 'DISTANCE_TO_EDGE'
        voronoi.inputs["Scale"].default_value = 24.0
        
        bump = nodes.new("ShaderNodeBump")
        bump.inputs["Strength"].default_value = 0.65
        bump.inputs["Distance"].default_value = 0.02
        
        links.new(tex_coord.outputs["UV"], voronoi.inputs["Vector"])
        links.new(voronoi.outputs["Distance"], bump.inputs["Height"])
        links.new(bump.outputs["Normal"], bsdf.inputs["Normal"])
        
    elif bump_type == 'tiles':
        tex_coord = nodes.new("ShaderNodeTexCoord")
        brick = nodes.new("ShaderNodeTexBrick")
        brick.inputs["Scale"].default_value = 45.0
        brick.inputs["Mortar Size"].default_value = 0.025
        brick.inputs["Mortar Smooth"].default_value = 0.1
        
        bump = nodes.new("ShaderNodeBump")
        bump.inputs["Strength"].default_value = 0.8
        bump.inputs["Distance"].default_value = 0.03
        
        links.new(tex_coord.outputs["UV"], brick.inputs["Vector"])
        links.new(brick.outputs["Color"], bump.inputs["Height"])
        links.new(bump.outputs["Normal"], bsdf.inputs["Normal"])

    elif bump_type == 'nozzle':
        tex_coord = nodes.new("ShaderNodeTexCoord")
        wave = nodes.new("ShaderNodeTexWave")
        wave.wave_type = 'BANDS'
        wave.bands_direction = 'Y'
        wave.inputs["Scale"].default_value = 35.0
        
        bump = nodes.new("ShaderNodeBump")
        bump.inputs["Strength"].default_value = 0.5
        bump.inputs["Distance"].default_value = 0.015
        
        links.new(tex_coord.outputs["UV"], wave.inputs["Vector"])
        links.new(wave.outputs["Color"], bump.inputs["Height"])
        links.new(bump.outputs["Normal"], bsdf.inputs["Normal"])

    return mat


def set_material_bake_pass(mat, pass_type):
    """Configures material for baking a specific pass via an Emission node."""
    nodes = mat.node_tree.nodes
    links = mat.node_tree.links
    out = nodes.get("Material Output")
    bsdf = nodes.get("Principled BSDF")
    
    emit = nodes.get("BakeEmission")
    if not emit:
        emit = nodes.new("ShaderNodeEmission")
        emit.name = "BakeEmission"
        
    if pass_type == 'PBR':
        links.new(bsdf.outputs["BSDF"], out.inputs["Surface"])
    elif pass_type == 'ALBEDO':
        emit.inputs["Color"].default_value = bsdf.inputs["Base Color"].default_value
        emit.inputs["Strength"].default_value = 1.0
        links.new(emit.outputs["Emission"], out.inputs["Surface"])
    elif pass_type == 'METALLIC':
        m = bsdf.inputs["Metallic"].default_value
        emit.inputs["Color"].default_value = (m, m, m, 1.0)
        emit.inputs["Strength"].default_value = 1.0
        links.new(emit.outputs["Emission"], out.inputs["Surface"])
    elif pass_type == 'ROUGHNESS':
        r = bsdf.inputs["Roughness"].default_value
        emit.inputs["Color"].default_value = (r, r, r, 1.0)
        emit.inputs["Strength"].default_value = 1.0
        links.new(emit.outputs["Emission"], out.inputs["Surface"])
    elif pass_type == 'EMISSIVE':
        ec = bsdf.inputs["Emission Color"].default_value
        es = bsdf.inputs["Emission Strength"].default_value
        emit.inputs["Color"].default_value = (ec[0] * es, ec[1] * es, ec[2] * es, 1.0)
        emit.inputs["Strength"].default_value = 1.0
        links.new(emit.outputs["Emission"], out.inputs["Surface"])


# ---------------------------------------------------------------------------
# Baking Suite
# ---------------------------------------------------------------------------
def bake_pbr_set(obj, base_path, tex_size=2048):
    """Bakes Albedo, Normal, Roughness, Metallic, Emissive to 24-bit BMPs."""
    print(f"\n--- Baking PBR texture set ({tex_size}x{tex_size}) for {obj.name} ---")
    scene = bpy.context.scene
    scene.render.engine = 'CYCLES'
    scene.cycles.device = 'CPU'
    scene.cycles.samples = 4
    
    # Create target bake image
    bake_img = bpy.data.images.new(f"Bake_{obj.name}", tex_size, tex_size)
    
    # Attach bake image to all material slots
    for slot in obj.material_slots:
        mat = slot.material
        if not mat or not mat.use_nodes:
            continue
        tex_node = mat.node_tree.nodes.get("BakeTarget")
        if not tex_node:
            tex_node = mat.node_tree.nodes.new("ShaderNodeTexImage")
            tex_node.name = "BakeTarget"
        tex_node.image = bake_img
        mat.node_tree.nodes.active = tex_node

    bpy.context.view_layer.objects.active = obj
    obj.select_set(True)

    # 1. Albedo
    print("Baking Albedo map...")
    for slot in obj.material_slots:
        set_material_bake_pass(slot.material, 'ALBEDO')
    bpy.ops.object.bake(type='EMIT')
    save_blender_image_as_bmp(bake_img, f"{base_path}_albedo.bmp")

    # 2. Normal (Tangent space)
    print("Baking Normal map...")
    for slot in obj.material_slots:
        set_material_bake_pass(slot.material, 'PBR')
    scene.cycles.bake_type = 'NORMAL'
    scene.render.bake.normal_space = 'TANGENT'
    scene.render.bake.normal_r = 'POS_X'
    scene.render.bake.normal_g = 'POS_Y'
    scene.render.bake.normal_b = 'POS_Z'
    bpy.ops.object.bake(type='NORMAL')
    save_blender_image_as_bmp(bake_img, f"{base_path}_normal.bmp")

    # 3. Roughness
    print("Baking Roughness map...")
    for slot in obj.material_slots:
        set_material_bake_pass(slot.material, 'ROUGHNESS')
    bpy.ops.object.bake(type='EMIT')
    save_blender_image_as_bmp(bake_img, f"{base_path}_rough.bmp")

    # 4. Metallic
    print("Baking Metallic map...")
    for slot in obj.material_slots:
        set_material_bake_pass(slot.material, 'METALLIC')
    bpy.ops.object.bake(type='EMIT')
    save_blender_image_as_bmp(bake_img, f"{base_path}_metal.bmp")

    # 5. Emissive
    print("Baking Emissive map...")
    for slot in obj.material_slots:
        set_material_bake_pass(slot.material, 'EMISSIVE')
    bpy.ops.object.bake(type='EMIT')
    save_blender_image_as_bmp(bake_img, f"{base_path}_emissive.bmp")

    # Restore PBR mode
    for slot in obj.material_slots:
        set_material_bake_pass(slot.material, 'PBR')
        
    bpy.data.images.remove(bake_img)
    print("Baking complete!")


# ---------------------------------------------------------------------------
# Detail pass helpers: small primitives used to build real surface structure
# (slats, seals, tiles, nozzles, gear, RCS) instead of relying on texture only.
# Frame convention of both hulls: -Z is the nose, +Z aft, +Y up, X spanwise.
# ---------------------------------------------------------------------------
def p_box(name, loc, dims, rot=(0.0, 0.0, 0.0), mat=None, parts=None):
    bpy.ops.mesh.primitive_cube_add(size=1.0, location=loc)
    o = bpy.context.active_object
    o.name = name
    o.scale = dims
    if any(rot):
        o.rotation_euler = rot
    bpy.ops.object.transform_apply(scale=True, rotation=True)
    if mat:
        o.data.materials.append(mat)
    if parts is not None:
        parts.append(o)
    return o


def p_cyl(name, loc, r, depth, rot=(0.0, 0.0, 0.0), segs=24, mat=None, parts=None):
    bpy.ops.mesh.primitive_cylinder_add(vertices=segs, radius=r, depth=depth,
                                        location=loc, rotation=rot)
    o = bpy.context.active_object
    o.name = name
    if mat:
        o.data.materials.append(mat)
    if parts is not None:
        parts.append(o)
    return o


def p_cone(name, loc, r1, r2, depth, rot=(0.0, 0.0, 0.0), segs=24, mat=None, parts=None):
    bpy.ops.mesh.primitive_cone_add(vertices=segs, radius1=r1, radius2=r2,
                                    depth=depth, location=loc, rotation=rot)
    o = bpy.context.active_object
    o.name = name
    if mat:
        o.data.materials.append(mat)
    if parts is not None:
        parts.append(o)
    return o


def p_torus(name, loc, major, minor, rot=(0.0, 0.0, 0.0), segs=24, sides=8, mat=None, parts=None):
    bpy.ops.mesh.primitive_torus_add(major_radius=major, minor_radius=minor,
                                     major_segments=segs, minor_segments=sides,
                                     location=loc, rotation=rot)
    o = bpy.context.active_object
    o.name = name
    if mat:
        o.data.materials.append(mat)
    if parts is not None:
        parts.append(o)
    return o


def p_sphere(name, loc, r, scale=(1.0, 1.0, 1.0), mat=None, parts=None, segs=20, rings=12):
    bpy.ops.mesh.primitive_uv_sphere_add(segments=segs, ring_count=rings, radius=r,
                                         location=loc)
    o = bpy.context.active_object
    o.name = name
    o.scale = scale
    bpy.ops.object.transform_apply(scale=True)
    if mat:
        o.data.materials.append(mat)
    if parts is not None:
        parts.append(o)
    return o


def p_nozzle(name, loc, r_throat, r_exit, depth, mat, plasma=None, parts=None, segs=28):
    """Bell nozzle with throat ring and an optional glow disc inside.
    Identity rotation keeps the bell on the hull's fore-aft axis."""
    p_cone(name, loc, r_throat, r_exit, depth, segs=segs, mat=mat, parts=parts)
    p_torus(name + "_lip", (loc[0], loc[1], loc[2] + depth / 2), r_exit, r_exit * 0.10,
            segs=segs, sides=6, mat=mat, parts=parts)
    if plasma is not None:
        p_cyl(name + "_glow", (loc[0], loc[1], loc[2] + depth * 0.30),
              r_throat * 0.92, depth * 0.35, segs=segs, mat=plasma, parts=parts)


def p_rcs_quad(name, loc, mat, parts, r=0.014, d=0.030):
    """Four attitude-control bells in a diamond on the hull skin."""
    half = math.pi / 2
    for k, rot in enumerate(((-half, 0.0, 0.0), (half, 0.0, 0.0),
                             (0.0, half, 0.0), (0.0, -half, 0.0))):
        off = ((0.0, 0.028, 0.0), (0.0, -0.028, 0.0), (0.028, 0.0, 0.0),
               (-0.028, 0.0, 0.0))[k]
        p_cone(f"{name}_{k}", (loc[0] + off[0], loc[1] + off[1], loc[2] + off[2]),
               r * 0.55, r, d, rot=rot, segs=12, mat=mat, parts=parts)


def p_missile(name, loc, length, r, mat_body, mat_dark, parts):
    p_cyl(name, loc, r, length, segs=16, mat=mat_body, parts=parts)
    p_cone(name + "_nose", (loc[0], loc[1], loc[2] - length / 2 - r * 1.6),
           r, 0.001, r * 3.2, rot=(math.pi, 0.0, 0.0), segs=16, mat=mat_dark, parts=parts)
    for k in range(4):
        a = k * math.tau / 4
        p_box(f"{name}_fin_{k}", (loc[0] + r * 1.4 * math.cos(a),
                                  loc[1] + r * 1.4 * math.sin(a),
                                  loc[2] + length * 0.36),
              (0.006, 0.030, 0.045), rot=(0.0, 0.0, a), mat=mat_dark, parts=parts)


# ---------------------------------------------------------------------------
# Geometry Builder: Dart (Style 0)
# ---------------------------------------------------------------------------
# Cylinder/cone/torus primitives are born on their local Z, so in the frame
# above rot=(0,0,0) already aims them fore-aft; (half,0,0) tips them DOWN,
# (-half,0,0) UP and (0,+-half,0) out the sides. Getting this wrong is #189.
def dart_details(parts, M):
    """Reference pass for a sleek interceptor: control surfaces, intake and
    nozzle hardware, TPS tiles, gear doors, RCS, hardpoints and canopy framing.
    Everything here is geometry, so the silhouette survives without textures."""
    hull, accent, graphite = M["hull"], M["accent"], M["graphite"]
    cockpit, engine, plasma = M["cockpit"], M["engine"], M["plasma"]
    half = math.pi / 2

    # 1. Fuselage reinforcement bands and keel line
    for z, xw, yt, yb in ((-0.80, 0.115, 0.045, -0.035), (-0.55, 0.115, 0.080, -0.055),
                          (-0.25, 0.145, 0.110, -0.070), (0.10, 0.140, 0.105, -0.065),
                          (0.45, 0.130, 0.095, -0.055), (0.68, 0.105, 0.080, -0.045)):
        p_box(f"DartArt_band_{z:+.2f}", (0.0, (yt + yb) * 0.5, z),
              (2 * xw + 0.010, yt - yb + 0.010, 0.010), mat=graphite, parts=parts)
    p_box("DartArt_keel", (0.0, -0.075, 0.05), (0.012, 0.014, 1.05), mat=accent, parts=parts)

    # 2. Belly thermal tiles (real relief, not a texture trick)
    for iz, z in enumerate((-0.62, -0.44, -0.26, -0.08, 0.10, 0.28, 0.46)):
        for ix in range(-2, 3):
            p_box(f"DartArt_tile_{iz}_{ix}", (ix * 0.042, -0.078 + abs(ix) * 0.006, z),
                  (0.034, 0.008, 0.052), mat=graphite, parts=parts)

    # 3. Ventral strakes and intake splitters
    for side in (-1.0, 1.0):
        tag = "L" if side < 0 else "R"
        p_box(f"DartArt_strake_{tag}", (side * 0.085, -0.078, 0.12),
              (0.014, 0.030, 0.72), rot=(0.0, 0.0, side * 0.04), mat=accent, parts=parts)
        p_box(f"DartArt_splitter_{tag}", (side * 0.105, -0.058, -0.34),
              (0.050, 0.012, 0.30), rot=(0.06 * side, side * 0.16, 0.0),
              mat=hull, parts=parts)
        p_box(f"DartArt_divider_{tag}", (side * 0.105, -0.052, -0.30),
              (0.008, 0.026, 0.26), rot=(0.0, side * 0.16, 0.0), mat=graphite, parts=parts)
        p_torus(f"DartArt_lip_{tag}", (side * 0.105, -0.052, -0.44), 0.032, 0.007,
                rot=(0.0, 0.0, side * 0.16), segs=20, sides=6, mat=engine, parts=parts)

    # 4. Twin engine cowlings with fan faces and burner petals
    for side in (-1.0, 1.0):
        tag = "L" if side < 0 else "R"
        cx = side * 0.155
        p_cyl(f"DartArt_cowl_{tag}", (cx, 0.012, 0.72), 0.072, 0.34,
              segs=28, mat=hull, parts=parts)
        p_torus(f"DartArt_cowlLip_{tag}", (cx, 0.012, 0.555), 0.072, 0.009,
                segs=28, sides=6, mat=graphite, parts=parts)
        p_cyl(f"DartArt_fan_{tag}", (cx, 0.012, 0.575), 0.062, 0.012,
              segs=24, mat=engine, parts=parts)
        for k in range(10):
            a = k * math.tau / 10
            p_box(f"DartArt_blade_{tag}_{k}",
                  (cx + 0.032 * math.cos(a), 0.012 + 0.032 * math.sin(a), 0.578),
                  (0.050, 0.008, 0.004), rot=(0.0, 0.0, a), mat=graphite, parts=parts)
        p_cone(f"DartArt_plug_{tag}", (cx, 0.012, 0.66), 0.014, 0.030, 0.10,
               segs=18, mat=engine, parts=parts)
        for k in range(8):
            a = k * math.tau / 8
            p_box(f"DartArt_petal_{tag}_{k}",
                  (cx + 0.052 * math.cos(a), 0.012 + 0.052 * math.sin(a), 0.865),
                  (0.020, 0.012, 0.052), rot=(0.22, 0.0, a), mat=graphite, parts=parts)
        p_nozzle(f"DartArt_bell_{tag}", (cx, 0.012, 0.90), 0.046, 0.066, 0.14,
                 engine, plasma=plasma, parts=parts)
        p_torus(f"DartArt_ctv_{tag}", (cx, 0.012, 0.955), 0.066, 0.006,
                segs=24, sides=6, mat=accent, parts=parts)

    # 5. Wing slats, flaps, fences, tip rails and hardpoints
    for side in (-1.0, 1.0):
        tag = "L" if side < 0 else "R"
        le_ry = -side * 0.75
        p_box(f"DartArt_slat_{tag}", (side * 0.46, 0.004, 0.02),
              (0.62, 0.014, 0.052), rot=(0.0, le_ry, 0.0), mat=accent, parts=parts)
        for k, (fx, fz) in enumerate(((0.30, 0.44), (0.60, 0.40))):
            p_box(f"DartArt_flap_{tag}_{k}", (side * fx, 0.0, fz),
                  (0.26, 0.012, 0.042), rot=(0.0, side * 0.10, 0.0), mat=hull, parts=parts)
            p_box(f"DartArt_hinge_{tag}_{k}", (side * fx, 0.008, fz - 0.022),
                  (0.26, 0.005, 0.006), mat=graphite, parts=parts)
        for k, fx in enumerate((0.30, 0.52)):
            p_box(f"DartArt_fence_{tag}_{k}", (side * fx, 0.020, -0.02 + fx * 0.42),
                  (0.008, 0.030, 0.13), rot=(0.0, side * 0.75, 0.0), mat=graphite, parts=parts)
        p_box(f"DartArt_rail_{tag}", (side * 0.795, 0.02, 0.20),
              (0.018, 0.014, 0.16), mat=graphite, parts=parts)
        p_missile(f"DartArt_store_{tag}", (side * 0.42, -0.030, 0.10), 0.30, 0.022,
                  hull, accent, parts)
        p_box(f"DartArt_pylon_{tag}", (side * 0.42, -0.018, 0.10),
              (0.012, 0.026, 0.09), mat=graphite, parts=parts)
        p_missile(f"DartArt_store_in_{tag}", (side * 0.60, -0.026, 0.22), 0.24, 0.019,
                  hull, accent, parts)
        p_box(f"DartArt_pylon_in_{tag}", (side * 0.60, -0.016, 0.22),
              (0.011, 0.022, 0.08), mat=graphite, parts=parts)

    # 6. Canopy framing, HUD and seat
    for k, z in enumerate((-0.56, -0.44, -0.32, -0.22)):
        p_box(f"DartArt_canopyRib_{k}", (0.0, 0.098, z), (0.105 - k * 0.006, 0.052, 0.010),
              rot=(0.10, 0.0, 0.0), mat=graphite, parts=parts)
    p_box("DartArt_hud", (0.0, 0.086, -0.60), (0.052, 0.004, 0.030),
          rot=(-0.35, 0.0, 0.0), mat=cockpit, parts=parts)
    p_box("DartArt_seat", (0.0, 0.062, -0.40), (0.044, 0.030, 0.060), mat=graphite, parts=parts)
    p_box("DartArt_seatBack", (0.0, 0.070, -0.33), (0.042, 0.016, 0.062),
          rot=(0.18, 0.0, 0.0), mat=graphite, parts=parts)
    p_torus("DartArt_hoop", (0.0, 0.086, -0.30), 0.030, 0.006,
            segs=18, sides=6, mat=accent, parts=parts)

    # 7. Spine avionics, antennas and pitot boom
    for k, z in enumerate((-0.05, 0.16, 0.38)):
        p_box(f"DartArt_blister_{k}", (0.0, 0.126, z), (0.030, 0.020, 0.10),
              mat=graphite, parts=parts)
    p_box("DartArt_bladeAnt", (0.0, 0.150, 0.30), (0.005, 0.048, 0.010),
          rot=(0.0, 0.25, 0.0), mat=accent, parts=parts)
    for side in (-1.0, 1.0):
        p_box(f"DartArt_rwr_{side:+.0f}", (side * 0.055, 0.118, 0.60),
              (0.022, 0.014, 0.030), rot=(0.0, 0.0, side * 0.30), mat=graphite, parts=parts)
    p_cyl("DartArt_pitot", (0.0, -0.020, -1.18), 0.007, 0.26,
          segs=12, mat=graphite, parts=parts)
    p_cone("DartArt_pitotTip", (0.0, -0.020, -1.32), 0.007, 0.001, 0.03,
           rot=(math.pi, 0.0, 0.0), segs=12, mat=accent, parts=parts)
    p_cyl("DartArt_tailMast", (0.0, 0.135, 0.70), 0.006, 0.12, rot=(half, 0.0, 0.0),
          segs=10, mat=graphite, parts=parts)

    # 8. RCS clusters: nose, tail and wingtips
    p_rcs_quad("DartArt_rcs_nose", (0.0, 0.030, -0.95), engine, parts, r=0.011, d=0.024)
    p_rcs_quad("DartArt_rcs_tail", (0.0, 0.030, 0.80), engine, parts, r=0.012, d=0.026)
    for side in (-1.0, 1.0):
        p_rcs_quad(f"DartArt_rcs_tip_{side:+.0f}", (side * 0.775, 0.026, 0.30),
                   engine, parts, r=0.009, d=0.020)

    # 9. Landing gear bays with doors and hinges
    p_box("DartArt_gearDoorN", (0.0, -0.082, -0.62), (0.048, 0.008, 0.16),
          mat=graphite, parts=parts)
    for k in range(3):
        p_box(f"DartArt_gearDoorNHinge_{k}", (0.0, -0.086, -0.69 + k * 0.07),
              (0.050, 0.004, 0.006), mat=accent, parts=parts)
    for side in (-1.0, 1.0):
        p_box(f"DartArt_gearDoorM_{side:+.0f}", (side * 0.20, -0.070, 0.16),
              (0.056, 0.008, 0.20), mat=graphite, parts=parts)
        p_cyl(f"DartArt_gearBay_{side:+.0f}", (side * 0.20, -0.062, 0.16), 0.026, 0.16,
              segs=16, mat=engine, parts=parts)

    # 10. Formation lights, strobes and fin detailing
    for side in (-1.0, 1.0):
        p_box(f"DartArt_formation_{side:+.0f}", (side * 0.40, 0.016, 0.06),
              (0.30, 0.004, 0.012), rot=(0.0, side * 0.75, 0.0), mat=plasma, parts=parts)
        p_cyl(f"DartArt_strobe_{side:+.0f}", (side * 0.795, 0.030, 0.28), 0.020, 0.014,
              rot=(0.0, half, 0.0), segs=14, mat=graphite, parts=parts)
    p_box("DartArt_finLE", (0.0, 0.075, 0.44), (0.030, 0.012, 0.42),
          rot=(0.42, 0.0, 0.0), mat=accent, parts=parts)
    for k, z in enumerate((0.30, 0.48, 0.66)):
        p_box(f"DartArt_rudderHinge_{k}", (0.0, 0.20 + k * 0.05, z),
              (0.026, 0.004, 0.006), rot=(0.42, 0.0, 0.0), mat=graphite, parts=parts)
    p_cyl("DartArt_finAnt", (0.0, 0.365, 0.74), 0.005, 0.09, rot=(0.35, 0.0, 0.0),
          segs=8, mat=graphite, parts=parts)

    # 11. Tail heat ring between the bells
    p_torus("DartArt_tailRing", (0.0, 0.012, 0.80), 0.150, 0.012,
            segs=32, sides=6, mat=graphite, parts=parts)
    p_cone("DartArt_tailCone", (0.0, 0.012, 0.86), 0.030, 0.004, 0.12,
           segs=18, mat=engine, parts=parts)


def build_dart():
    print("\n================ Building Dart (Style 0) ================")
    bpy.ops.wm.read_factory_settings(use_empty=True)
    parts = []

    # Materials
    m_hull = make_pbr_material("DartHull", (0.74, 0.76, 0.80), 0.85, 0.35, bump_type='panels')
    m_accent = make_pbr_material("DartAccent", (0.88, 0.35, 0.04), 0.60, 0.40, bump_type='panels') # Orange racing trim
    m_graphite = make_pbr_material("DartGraphite", (0.12, 0.13, 0.16), 0.70, 0.45, bump_type='panels')
    m_cockpit = make_pbr_material("DartCockpit", (0.04, 0.08, 0.12), 0.10, 0.06, emission=(0.10, 0.75, 1.0), emission_strength=2.2)
    m_engine = make_pbr_material("DartEngine", (0.14, 0.15, 0.18), 0.95, 0.28, bump_type='nozzle')
    m_plasma = make_pbr_material("DartPlasma", (1.0, 0.85, 0.6), 0.0, 0.5, emission=(1.0, 0.42, 0.05), emission_strength=4.5)
    m_nav_red = make_pbr_material("DartNavRed", (0.8, 0.05, 0.05), 0.0, 0.1, emission=(1.0, 0.05, 0.05), emission_strength=3.0)
    m_nav_green = make_pbr_material("DartNavGreen", (0.05, 0.8, 0.15), 0.0, 0.1, emission=(0.05, 1.0, 0.15), emission_strength=3.0)

    # 1. Fuselage
    bm = bmesh.new()
    sections = [
        # (z, y_top, x_sh, y_sh, x_ch, y_ch, x_bsh, y_bsh, y_bot)
        (-1.15, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0),
        (-1.02, 0.012, 0.009, 0.009, 0.012, 0.0, 0.009, -0.009, -0.012),
        (-0.80, 0.045, 0.040, 0.025, 0.065, 0.0, 0.040, -0.025, -0.035),
        (-0.55, 0.080, 0.075, 0.045, 0.115, 0.0, 0.075, -0.040, -0.055),
        (-0.25, 0.110, 0.095, 0.060, 0.145, 0.0, 0.090, -0.050, -0.070),
        ( 0.10, 0.105, 0.095, 0.055, 0.140, 0.0, 0.085, -0.045, -0.065),
        ( 0.45, 0.095, 0.085, 0.050, 0.130, 0.0, 0.075, -0.040, -0.055),
        ( 0.70, 0.080, 0.070, 0.040, 0.105, 0.0, 0.060, -0.035, -0.045),
        ( 0.78, 0.070, 0.060, 0.035, 0.090, 0.0, 0.050, -0.030, -0.040),
    ]
    prev_verts = None
    for idx, s in enumerate(sections):
        z, yt, xsh, ysh, xch, ych, xbsh, ybsh, yb = s
        if idx == 0:
            v_apex = bm.verts.new(Vector((0.0, 0.0, z)))
            prev_verts = [v_apex] * 8
            continue
        ring = [
            bm.verts.new(Vector((  0.0,   yt, z))),
            bm.verts.new(Vector((  xsh,  ysh, z))),
            bm.verts.new(Vector((  xch,  ych, z))),
            bm.verts.new(Vector(( xbsh, ybsh, z))),
            bm.verts.new(Vector((  0.0,   yb, z))),
            bm.verts.new(Vector((-xbsh, ybsh, z))),
            bm.verts.new(Vector(( -xch,  ych, z))),
            bm.verts.new(Vector(( -xsh,  ysh, z))),
        ]
        if idx == 1:
            for i in range(8):
                bm.faces.new([v_apex, ring[i], ring[(i+1)%8]])
        else:
            for i in range(8):
                bm.faces.new([prev_verts[i], ring[i], ring[(i+1)%8], prev_verts[(i+1)%8]])
        prev_verts = ring
    bm.faces.new(list(reversed(prev_verts)))
    
    me_hull = bpy.data.meshes.new('Dart_Fuselage')
    bm.to_mesh(me_hull)
    bm.free()
    obj_hull = bpy.data.objects.new('Dart_Fuselage', me_hull)
    obj_hull.data.materials.append(m_hull)
    parts.append(obj_hull)

    # 2. Cockpit Canopy
    bpy.ops.mesh.primitive_uv_sphere_add(segments=24, ring_count=12, radius=1.0, location=(0.0, 0.085, -0.42))
    canopy = bpy.context.active_object
    canopy.name = "Dart_Cockpit"
    canopy.scale = (0.075, 0.045, 0.20)
    canopy.data.materials.append(m_cockpit)
    parts.append(canopy)

    # 3. Dorsal Spine
    bpy.ops.mesh.primitive_cube_add(size=1.0, location=(0.0, 0.108, 0.18))
    spine = bpy.context.active_object
    spine.name = "Dart_Spine"
    spine.scale = (0.035, 0.024, 0.45)
    spine.data.materials.append(m_graphite)
    parts.append(spine)

    # 4. Vertical Stabilizer Fin
    bm_fin = bmesh.new()
    fv = [
        bm_fin.verts.new(Vector(( 0.012, 0.08,  0.15))),
        bm_fin.verts.new(Vector((-0.012, 0.08,  0.15))),
        bm_fin.verts.new(Vector((-0.004, 0.36,  0.64))),
        bm_fin.verts.new(Vector(( 0.004, 0.36,  0.64))),
        bm_fin.verts.new(Vector(( 0.005, 0.35,  0.72))),
        bm_fin.verts.new(Vector((-0.005, 0.35,  0.72))),
        bm_fin.verts.new(Vector((-0.012, 0.07,  0.76))),
        bm_fin.verts.new(Vector(( 0.012, 0.07,  0.76))),
    ]
    bm_fin.faces.new([fv[0], fv[3], fv[2], fv[1]]) # front
    bm_fin.faces.new([fv[3], fv[4], fv[5], fv[2]]) # top
    bm_fin.faces.new([fv[4], fv[7], fv[6], fv[5]]) # rear
    bm_fin.faces.new([fv[0], fv[1], fv[6], fv[7]]) # bottom
    bm_fin.faces.new([fv[0], fv[7], fv[4], fv[3]]) # right
    bm_fin.faces.new([fv[1], fv[2], fv[5], fv[6]]) # left
    me_fin = bpy.data.meshes.new('Dart_Fin')
    bm_fin.to_mesh(me_fin)
    bm_fin.free()
    obj_fin = bpy.data.objects.new('Dart_Fin', me_fin)
    obj_fin.data.materials.append(m_accent)
    parts.append(obj_fin)

    # 5. Ventral Air Intake
    bpy.ops.mesh.primitive_cube_add(size=1.0, location=(0.0, -0.08, -0.25))
    intake = bpy.context.active_object
    intake.name = "Dart_Intake"
    intake.scale = (0.075, 0.038, 0.24)
    intake.data.materials.append(m_graphite)
    parts.append(intake)

    # 6. Swept Delta Wings (Left and Right)
    for side in (-1.0, 1.0):
        tag = "L" if side < 0 else "R"
        bm_w = bmesh.new()
        wv = [
            bm_w.verts.new(Vector((side * 0.14,  0.008, -0.28))),
            bm_w.verts.new(Vector((side * 0.14, -0.008, -0.28))),
            bm_w.verts.new(Vector((side * 0.78,  0.004,  0.32))),
            bm_w.verts.new(Vector((side * 0.78, -0.004,  0.32))),
            bm_w.verts.new(Vector((side * 0.74,  0.004,  0.38))),
            bm_w.verts.new(Vector((side * 0.74, -0.004,  0.38))),
            bm_w.verts.new(Vector((side * 0.12,  0.008,  0.48))),
            bm_w.verts.new(Vector((side * 0.12, -0.008,  0.48))),
        ]
        # Create 6 faces
        if side > 0:
            bm_w.faces.new([wv[0], wv[2], wv[3], wv[1]])
            bm_w.faces.new([wv[2], wv[4], wv[5], wv[3]])
            bm_w.faces.new([wv[4], wv[6], wv[7], wv[5]])
            bm_w.faces.new([wv[6], wv[0], wv[1], wv[7]])
            bm_w.faces.new([wv[0], wv[6], wv[4], wv[2]]) # top
            bm_w.faces.new([wv[1], wv[3], wv[5], wv[7]]) # bottom
        else:
            bm_w.faces.new([wv[0], wv[1], wv[3], wv[2]])
            bm_w.faces.new([wv[2], wv[3], wv[5], wv[4]])
            bm_w.faces.new([wv[4], wv[5], wv[7], wv[6]])
            bm_w.faces.new([wv[6], wv[7], wv[1], wv[0]])
            bm_w.faces.new([wv[0], wv[2], wv[4], wv[6]]) # top
            bm_w.faces.new([wv[1], wv[7], wv[5], wv[3]]) # bottom
            
        me_w = bpy.data.meshes.new(f'Dart_Wing_{tag}')
        bm_w.to_mesh(me_w)
        bm_w.free()
        obj_w = bpy.data.objects.new(f'Dart_Wing_{tag}', me_w)
        obj_w.data.materials.append(m_hull)
        parts.append(obj_w)

        # Wingtip Fin
        bpy.ops.mesh.primitive_cube_add(size=1.0, location=(side * 0.78, 0.015, 0.34))
        tip = bpy.context.active_object
        tip.name = f"Dart_Wingtip_{tag}"
        tip.scale = (0.015, 0.065, 0.10)
        tip.data.materials.append(m_accent)
        parts.append(tip)

        # Nav Light
        nav_mat = m_nav_red if side < 0 else m_nav_green
        bpy.ops.mesh.primitive_uv_sphere_add(segments=12, ring_count=8, radius=0.016, location=(side * 0.795, 0.02, 0.28))
        nav = bpy.context.active_object
        nav.name = f"Dart_Nav_{tag}"
        nav.data.materials.append(nav_mat)
        parts.append(nav)

        # Canted Ventral Stabilizer
        bpy.ops.mesh.primitive_cube_add(size=1.0, location=(side * 0.18, -0.06, 0.52), rotation=(0.0, 0.0, -side * 0.35))
        subfin = bpy.context.active_object
        subfin.name = f"Dart_Subfin_{tag}"
        subfin.scale = (0.014, 0.065, 0.16)
        subfin.data.materials.append(m_graphite)
        parts.append(subfin)

    # 7. Main Engine Nozzle
    bpy.ops.mesh.primitive_cone_add(vertices=24, radius1=0.09, radius2=0.118, depth=0.36, location=(0.0, 0.0, 0.94))
    nozzle = bpy.context.active_object
    nozzle.name = "Dart_MainNozzle"
    nozzle.data.materials.append(m_engine)
    parts.append(nozzle)

    # Main Engine Plasma Throat
    bpy.ops.mesh.primitive_uv_sphere_add(segments=16, ring_count=8, radius=0.075, location=(0.0, 0.0, 0.92))
    plasma = bpy.context.active_object
    plasma.name = "Dart_Plasma"
    plasma.scale = (1.0, 1.0, 0.25)
    plasma.data.materials.append(m_plasma)
    parts.append(plasma)

    # Twin Secondary Thrusters
    for side in (-1.0, 1.0):
        tag = "L" if side < 0 else "R"
        bpy.ops.mesh.primitive_cone_add(vertices=16, radius1=0.035, radius2=0.046, depth=0.20, location=(side * 0.12, 0.015, 0.88))
        aux_noz = bpy.context.active_object
        aux_noz.name = f"Dart_AuxNozzle_{tag}"
        aux_noz.data.materials.append(m_engine)
        parts.append(aux_noz)
        
        bpy.ops.mesh.primitive_uv_sphere_add(segments=12, ring_count=6, radius=0.030, location=(side * 0.12, 0.015, 0.87))
        aux_pla = bpy.context.active_object
        aux_pla.name = f"Dart_AuxPlasma_{tag}"
        aux_pla.scale = (1.0, 1.0, 0.2)
        aux_pla.data.materials.append(m_plasma)
        parts.append(aux_pla)

    # 8. Reference-driven detail pass
    dart_details(parts, {"hull": m_hull, "accent": m_accent, "graphite": m_graphite,
                         "cockpit": m_cockpit, "engine": m_engine, "plasma": m_plasma})

    # Join All Parts
    for obj in parts:
        if obj.name not in bpy.context.scene.collection.objects:
            bpy.context.scene.collection.objects.link(obj)
        obj.select_set(True)
    bpy.context.view_layer.objects.active = obj_hull
    bpy.ops.object.join()
    ship = bpy.context.active_object
    ship.name = "Dart"

    # Normalize to exact ~2.3 length, center at (0,0,0)
    bpy.ops.object.origin_set(type='ORIGIN_GEOMETRY', center='BOUNDS')
    bbox = [ship.matrix_world @ Vector(corner) for corner in ship.bound_box]
    min_z = min(v.z for v in bbox)
    max_z = max(v.z for v in bbox)
    cur_len = max_z - min_z
    scale_fac = 2.30 / cur_len
    ship.scale = (scale_fac, scale_fac, scale_fac)
    bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)

    # Re-center
    bbox = [ship.matrix_world @ Vector(corner) for corner in ship.bound_box]
    min_x = min(v.x for v in bbox)
    max_x = max(v.x for v in bbox)
    min_y = min(v.y for v in bbox)
    max_y = max(v.y for v in bbox)
    min_z = min(v.z for v in bbox)
    max_z = max(v.z for v in bbox)
    center = Vector(((min_x + max_x) * 0.5, (min_y + max_y) * 0.5, (min_z + max_z) * 0.5))
    ship.location = -center
    bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)

    # Smooth shading and edge split
    for poly in ship.data.polygons:
        poly.use_smooth = True
    split_mod = ship.modifiers.new(name="EdgeSplit", type='EDGE_SPLIT')
    split_mod.split_angle = math.radians(35)
    bpy.context.view_layer.objects.active = ship
    bpy.ops.object.modifier_apply(modifier="EdgeSplit")

    # UV Smart Project
    bpy.ops.object.mode_set(mode='EDIT')
    bpy.ops.mesh.select_all(action='SELECT')
    bpy.ops.uv.smart_project(angle_limit=1.15, island_margin=0.015)
    bpy.ops.object.mode_set(mode='OBJECT')

    print(f"Dart created successfully: {len(ship.data.vertices)} vertices, {len(ship.data.polygons)} polygons.")
    return ship


# ---------------------------------------------------------------------------
# Geometry Builder: Shuttle (Style 2)
# ---------------------------------------------------------------------------
def shuttle_details(parts, M):
    """Reference pass for a heavy tactical shuttle: real TPS tile field, bridge
    framing, cargo bay with rails/arm/payload, deployed radiators, full engine
    cluster with feed plumbing, elevons, gear and RCS."""
    hull, accent, tiles = M["hull"], M["accent"], M["tiles"]
    cargo, cockpit, engine, plasma = M["cargo"], M["cockpit"], M["engine"], M["plasma"]
    half = math.pi / 2

    # 1. Thermal protection tile field on the belly and nose underside
    for iz, z in enumerate((-1.00, -0.80, -0.60, -0.40, -0.20, 0.00, 0.20, 0.40, 0.60)):
        for ix in range(-6, 7):
            x = ix * 0.108
            if abs(x) > 0.36 - abs(z) * 0.10:
                continue
            p_box(f"ShutArt_tile_{iz}_{ix}", (x, -0.122, z), (0.094, 0.012, 0.094),
                  mat=tiles, parts=parts)
    for k in range(3):
        p_box(f"ShutArt_tileSeam_{k}", (0.0, -0.128, -0.60 + k * 0.60),
              (0.66, 0.006, 0.012), mat=hull, parts=parts)

    # 2. Nose: sensor radome, pitot boom, formation lights, cargo seam
    p_sphere("ShutArt_radome", (0.0, -0.030, -1.16), 0.055, scale=(1.0, 1.0, 0.7),
             mat=tiles, parts=parts)
    p_cyl("ShutArt_pitot", (0.0, -0.030, -1.34), 0.008, 0.30,
          segs=12, mat=engine, parts=parts)
    for side in (-1.0, 1.0):
        p_box(f"ShutArt_noseLight_{side:+.0f}", (side * 0.14, 0.020, -1.02),
              (0.030, 0.010, 0.010), rot=(0.0, 0.0, side * 0.4), mat=plasma, parts=parts)
        p_box(f"ShutArt_formation_{side:+.0f}", (side * 0.20, 0.108, -0.10),
              (0.28, 0.006, 0.012), rot=(0.0, side * 0.10, 0.0), mat=accent, parts=parts)

    # 3. Bridge framing: mullions, sun visor, side window frames, lamps
    for k, z in enumerate((-0.92, -0.82, -0.72, -0.62)):
        p_box(f"ShutArt_mullion_{k}", (0.0, 0.112, z), (0.30 - k * 0.02, 0.012, 0.012),
              rot=(-0.30, 0.0, 0.0), mat=hull, parts=parts)
    p_box("ShutArt_visor", (0.0, 0.128, -0.98), (0.40, 0.050, 0.014),
          rot=(-0.42, 0.0, 0.0), mat=accent, parts=parts)
    for side in (-1.0, 1.0):
        for k in range(3):
            p_box(f"ShutArt_sideFrame_{side:+.0f}_{k}", (side * 0.30, 0.085, -0.86 + k * 0.10),
                  (0.008, 0.030, 0.070), rot=(0.0, side * 0.22, 0.0), mat=hull, parts=parts)
        p_cyl(f"ShutArt_lamp_{side:+.0f}", (side * 0.20, 0.060, -1.10), 0.022, 0.014,
              segs=14, mat=plasma, parts=parts)

    # 4. Cargo bay: rails, crates, robotic arm, latches, docking bolts, payload
    for side in (-1.0, 1.0):
        p_box(f"ShutArt_bayRail_{side:+.0f}", (side * 0.30, 0.150, 0.18),
              (0.020, 0.016, 1.06), mat=engine, parts=parts)
    for k, (cz, cl) in enumerate(((0.02, 0.30), (0.34, 0.24))):
        p_box(f"ShutArt_crate_{k}", (0.0, 0.168, cz), (0.42, 0.052, cl),
              mat=cargo, parts=parts)
        for j in range(3):
            p_box(f"ShutArt_crateRib_{k}_{j}", (0.0, 0.196, cz - cl * 0.35 + j * cl * 0.35),
                  (0.43, 0.008, 0.014), mat=accent, parts=parts)
    arm_base = (0.26, 0.160, 0.56)
    p_cyl("ShutArt_armBase", arm_base, 0.030, 0.050, rot=(-half, 0.0, 0.0),
          mat=engine, parts=parts)
    p_torus("ShutArt_armJoint1", (arm_base[0], 0.185, arm_base[2]), 0.026, 0.010,
            rot=(0.0, half, 0.0), segs=18, sides=6, mat=accent, parts=parts)
    p_box("ShutArt_armUpper", (0.30, 0.230, 0.44), (0.022, 0.110, 0.022),
          rot=(0.0, 0.0, 0.35), mat=hull, parts=parts)
    p_box("ShutArt_armFore", (0.40, 0.290, 0.28), (0.018, 0.090, 0.018),
          rot=(0.55, 0.0, 0.20), mat=hull, parts=parts)
    p_box("ShutArt_armTool", (0.455, 0.315, 0.155), (0.014, 0.050, 0.014),
          rot=(0.9, 0.0, 0.2), mat=engine, parts=parts)
    for k in range(12):
        a = k * math.tau / 12
        p_cyl(f"ShutArt_bolt_{k}", (0.095 * math.cos(a), 0.172, 0.52 + 0.095 * math.sin(a)),
              0.008, 0.018, rot=(half, 0.0, 0.0), segs=8, mat=engine, parts=parts)
    for k in range(4):
        p_box(f"ShutArt_latch_{k}", (-0.16 + (k % 2) * 0.32, 0.158, 0.02 + (k // 2) * 0.32),
              (0.030, 0.010, 0.014), mat=accent, parts=parts)
    p_box("ShutArt_payloadBus", (0.0, 0.205, 0.18), (0.14, 0.110, 0.14), mat=cargo, parts=parts)
    for side in (-1.0, 1.0):
        p_box(f"ShutArt_payloadPanel_{side:+.0f}", (side * 0.28, 0.205, 0.18),
              (0.34, 0.006, 0.13), rot=(0.0, side * 0.12, 0.0), mat=cockpit, parts=parts)
        for j in range(4):
            p_box(f"ShutArt_panelRib_{side:+.0f}_{j}", (side * (0.14 + j * 0.09), 0.212, 0.18),
                  (0.006, 0.006, 0.13), mat=hull, parts=parts)

    # 5. Deployed radiator panels with fin arrays
    for side in (-1.0, 1.0):
        p_box(f"ShutArt_radStrut_{side:+.0f}", (side * 0.50, 0.105, 0.42),
              (0.020, 0.140, 0.020), rot=(0.0, side * 0.5, 0.0), mat=engine, parts=parts)
        p_box(f"ShutArt_radPanel_{side:+.0f}", (side * 0.66, 0.175, 0.42),
              (0.30, 0.010, 0.44), rot=(0.0, side * 0.25, 0.0), mat=cargo, parts=parts)
        for j in range(9):
            p_box(f"ShutArt_radFin_{side:+.0f}_{j}", (side * 0.66, 0.186, 0.22 + j * 0.05),
                  (0.30, 0.026, 0.008), rot=(0.0, side * 0.25, 0.0), mat=hull, parts=parts)

    # 6. Engine cluster: three main bells, gimbal rings, feed plumbing, OMS
    for k, (ex, ey) in enumerate(((-0.16, -0.03), (0.16, -0.03), (0.0, -0.07))):
        p_nozzle(f"ShutArt_mainBell_{k}", (ex, ey, 0.90), 0.055, 0.095, 0.26,
                 engine, plasma=plasma, parts=parts, segs=28)
        p_torus(f"ShutArt_gimbal_{k}", (ex, ey, 0.78), 0.062, 0.010,
                segs=24, sides=6, mat=accent, parts=parts)
        p_cyl(f"ShutArt_feed_{k}", (ex, ey - 0.055, 0.66), 0.016, 0.30,
              segs=12, mat=cargo, parts=parts)
    for side in (-1.0, 1.0):
        p_box(f"ShutArt_omsPod_{side:+.0f}", (side * 0.24, -0.020, 0.72),
              (0.140, 0.070, 0.20), mat=hull, parts=parts)
        for k in (-1, 1):
            p_nozzle(f"ShutArt_omsBell_{side:+.0f}_{k}", (side * 0.24 + k * 0.04, -0.020, 0.86),
                     0.022, 0.036, 0.10, engine, plasma=plasma, parts=parts, segs=18)
        p_rcs_quad(f"ShutArt_omsRcs_{side:+.0f}", (side * 0.30, -0.020, 0.62),
                   engine, parts, r=0.012, d=0.026)

    # 7. Wing surfaces: slats, elevons, keel strakes, body flap, tip verticals
    for side in (-1.0, 1.0):
        p_box(f"ShutArt_slat_{side:+.0f}", (side * 0.52, 0.010, -0.10),
              (0.62, 0.016, 0.070), rot=(0.0, side * 0.30, 0.0), mat=tiles, parts=parts)
        for k, (fx, fz) in enumerate(((0.40, 0.44), (0.66, 0.50))):
            p_box(f"ShutArt_elevon_{side:+.0f}_{k}", (side * fx, 0.004, fz),
                  (0.24, 0.014, 0.050), rot=(0.0, side * 0.08, 0.0), mat=hull, parts=parts)
            p_box(f"ShutArt_elevonHinge_{side:+.0f}_{k}", (side * fx, 0.014, fz - 0.026),
                  (0.24, 0.005, 0.007), mat=engine, parts=parts)
        p_box(f"ShutArt_keelStrake_{side:+.0f}", (side * 0.22, -0.100, -0.30),
              (0.020, 0.026, 0.50), rot=(0.0, side * 0.25, 0.0), mat=tiles, parts=parts)
        p_box(f"ShutArt_tipVert_{side:+.0f}", (side * 0.72, 0.045, 0.50),
              (0.014, 0.050, 0.14), rot=(0.35, side * 0.20, 0.0), mat=accent, parts=parts)
        p_sphere(f"ShutArt_tipNav_{side:+.0f}", (side * 0.735, 0.070, 0.56), 0.014,
                 mat=M["nav_red"] if side < 0 else M["nav_green"], parts=parts,
                 segs=12, rings=8)
    p_box("ShutArt_bodyFlap", (0.0, 0.030, 0.86), (0.44, 0.014, 0.20),
          rot=(0.0, 0.22, 0.0), mat=hull, parts=parts)

    # 8. Fuselage bands, cabin windows, handrails, dish, camera pods
    for z, xw in ((-0.55, 0.38), (-0.15, 0.44), (0.25, 0.44), (0.55, 0.40)):
        p_box(f"ShutArt_band_{z:+.2f}", (0.0, 0.012, z), (2 * xw + 0.012, 0.246, 0.012),
              mat=cargo, parts=parts)
    for k in range(6):
        p_box(f"ShutArt_cabinWin_{k}", (-0.30, 0.055, -0.40 + k * 0.13),
              (0.010, 0.030, 0.060), rot=(0.0, 0.28, 0.0), mat=cockpit, parts=parts)
        p_box(f"ShutArt_cabinWinR_{k}", (0.30, 0.055, -0.40 + k * 0.13),
              (0.010, 0.030, 0.060), rot=(0.0, -0.28, 0.0), mat=cockpit, parts=parts)
    for side in (-1.0, 1.0):
        p_cyl(f"ShutArt_handrail_{side:+.0f}", (side * 0.34, 0.148, 0.18), 0.006, 0.70,
              segs=10, mat=engine, parts=parts)
    p_cyl("ShutArt_dishMast", (0.20, 0.150, -0.20), 0.008, 0.10, rot=(-half, 0.0, 0.0),
          mat=engine, parts=parts)
    p_cone("ShutArt_dish", (0.20, 0.150, -0.14), 0.004, 0.070, 0.045,
           rot=(-0.5, 0.0, 0.0), segs=20, mat=cargo, parts=parts)
    for k in range(2):
        p_box(f"ShutArt_camPod_{k}", (-0.26 + k * 0.52, 0.120, -0.68),
              (0.040, 0.030, 0.040), mat=engine, parts=parts)

    # 9. Landing gear: nose strut with wheel, two mains with fairings
    p_box("ShutArt_noseStrut", (0.0, -0.150, -0.86), (0.018, 0.018, 0.16),
          mat=engine, parts=parts)
    p_cyl("ShutArt_noseWheel", (0.0, -0.245, -0.86), 0.045, 0.030,
          rot=(0.0, half, 0.0), segs=18, mat=tiles, parts=parts)
    for side in (-1.0, 1.0):
        p_box(f"ShutArt_mainStrut_{side:+.0f}", (side * 0.26, -0.150, 0.06),
              (0.020, 0.020, 0.18), rot=(0.0, side * 0.18, 0.0), mat=engine, parts=parts)
        p_cyl(f"ShutArt_mainWheel_{side:+.0f}", (side * 0.28, -0.250, 0.02), 0.055, 0.036,
              rot=(0.0, half, 0.0), segs=20, mat=tiles, parts=parts)
        p_box(f"ShutArt_mainFairing_{side:+.0f}", (side * 0.27, -0.190, 0.04),
              (0.060, 0.050, 0.100), mat=hull, parts=parts)
        p_box(f"ShutArt_gearDoor_{side:+.0f}", (side * 0.26, -0.128, 0.14),
              (0.070, 0.008, 0.18), rot=(0.0, side * 0.10, 0.0), mat=cargo, parts=parts)

    # 10. Nose and tail RCS diamonds
    p_rcs_quad("ShutArt_rcs_nose", (0.0, 0.060, -1.06), engine, parts, r=0.013, d=0.028)
    for side in (-1.0, 1.0):
        p_rcs_quad(f"ShutArt_rcs_tail_{side:+.0f}", (side * 0.40, 0.020, 0.74),
                   engine, parts, r=0.011, d=0.024)


def build_shuttle():
    print("\n================ Building Shuttle (Style 2) ================")
    bpy.ops.wm.read_factory_settings(use_empty=True)
    parts = []

    # Materials
    m_hull = make_pbr_material("ShuttleHull", (0.84, 0.85, 0.88), 0.20, 0.42, bump_type='panels')
    m_accent = make_pbr_material("ShuttleAccent", (0.08, 0.32, 0.68), 0.30, 0.38, bump_type='panels') # NASA cobalt blue
    m_tiles = make_pbr_material("ShuttleTiles", (0.08, 0.08, 0.09), 0.05, 0.78, bump_type='tiles') # Black ceramic tiles
    m_cargo = make_pbr_material("ShuttleCargo", (0.75, 0.76, 0.78), 0.60, 0.35, bump_type='panels')
    m_cockpit = make_pbr_material("ShuttleCockpit", (0.04, 0.06, 0.10), 0.10, 0.06, emission=(0.12, 0.70, 0.95), emission_strength=2.2)
    m_engine = make_pbr_material("ShuttleEngine", (0.14, 0.15, 0.18), 0.95, 0.28, bump_type='nozzle')
    m_plasma = make_pbr_material("ShuttlePlasma", (1.0, 0.85, 0.6), 0.0, 0.5, emission=(1.0, 0.40, 0.05), emission_strength=4.5)
    m_nav_red = make_pbr_material("ShuttleNavRed", (0.8, 0.05, 0.05), 0.0, 0.1, emission=(1.0, 0.05, 0.05), emission_strength=3.0)
    m_nav_green = make_pbr_material("ShuttleNavGreen", (0.05, 0.8, 0.15), 0.0, 0.1, emission=(0.05, 1.0, 0.15), emission_strength=3.0)

    # 1. Main Lifting-Body Fuselage
    bm = bmesh.new()
    sections = [
        # (z, y_top, x_sh, y_sh, x_ch, y_ch, x_bsh, y_bsh, y_bot)
        (-1.15, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0),
        (-1.05, 0.055, 0.08, 0.045, 0.12, 0.01, 0.10, -0.040, -0.055),
        (-0.85, 0.110, 0.19, 0.080, 0.26, 0.01, 0.22, -0.065, -0.090),
        (-0.55, 0.135, 0.28, 0.095, 0.38, 0.01, 0.32, -0.075, -0.105),
        (-0.15, 0.140, 0.32, 0.100, 0.44, 0.01, 0.38, -0.080, -0.110),
        ( 0.25, 0.135, 0.32, 0.100, 0.44, 0.01, 0.38, -0.080, -0.110),
        ( 0.55, 0.125, 0.29, 0.095, 0.40, 0.01, 0.34, -0.075, -0.105),
        ( 0.75, 0.115, 0.25, 0.085, 0.34, 0.01, 0.28, -0.070, -0.095),
    ]
    prev_verts = None
    for idx, s in enumerate(sections):
        z, yt, xsh, ysh, xch, ych, xbsh, ybsh, yb = s
        if idx == 0:
            v_apex = bm.verts.new(Vector((0.0, 0.0, z)))
            prev_verts = [v_apex] * 8
            continue
        ring = [
            bm.verts.new(Vector((  0.0,   yt, z))),
            bm.verts.new(Vector((  xsh,  ysh, z))),
            bm.verts.new(Vector((  xch,  ych, z))),
            bm.verts.new(Vector(( xbsh, ybsh, z))),
            bm.verts.new(Vector((  0.0,   yb, z))),
            bm.verts.new(Vector((-xbsh, ybsh, z))),
            bm.verts.new(Vector(( -xch,  ych, z))),
            bm.verts.new(Vector(( -xsh,  ysh, z))),
        ]
        if idx == 1:
            for i in range(8):
                bm.faces.new([v_apex, ring[i], ring[(i+1)%8]])
        else:
            for i in range(8):
                bm.faces.new([prev_verts[i], ring[i], ring[(i+1)%8], prev_verts[(i+1)%8]])
        prev_verts = ring
    bm.faces.new(list(reversed(prev_verts)))

    me_hull = bpy.data.meshes.new('Shuttle_Fuselage')
    bm.to_mesh(me_hull)
    bm.free()
    obj_hull = bpy.data.objects.new('Shuttle_Fuselage', me_hull)
    obj_hull.data.materials.append(m_hull)
    parts.append(obj_hull)

    # 2. Thermal Protection Tiles (Belly Plate)
    bpy.ops.mesh.primitive_cube_add(size=1.0, location=(0.0, -0.095, -0.10))
    belly = bpy.context.active_object
    belly.name = "Shuttle_BellyTiles"
    belly.scale = (0.76, 0.025, 1.55)
    belly.data.materials.append(m_tiles)
    parts.append(belly)

    # 3. Panoramic Bridge Windows
    bpy.ops.mesh.primitive_uv_sphere_add(segments=24, ring_count=12, radius=1.0, location=(0.0, 0.095, -0.68))
    bridge = bpy.context.active_object
    bridge.name = "Shuttle_Bridge"
    bridge.scale = (0.16, 0.075, 0.16)
    bridge.data.materials.append(m_cockpit)
    parts.append(bridge)

    # Side Cabin Windows
    for side in (-1.0, 1.0):
        tag = "L" if side < 0 else "R"
        bpy.ops.mesh.primitive_cube_add(size=1.0, location=(side * 0.28, 0.08, -0.45), rotation=(0.0, 0.0, side * 0.35))
        win = bpy.context.active_object
        win.name = f"Shuttle_SideWindow_{tag}"
        win.scale = (0.015, 0.035, 0.15)
        win.data.materials.append(m_cockpit)
        parts.append(win)

    # 4. Dorsal Cargo Bay Doors
    bpy.ops.mesh.primitive_cube_add(size=1.0, location=(0.0, 0.142, 0.18))
    cargo = bpy.context.active_object
    cargo.name = "Shuttle_CargoDoors"
    cargo.scale = (0.34, 0.015, 0.58)
    cargo.data.materials.append(m_cargo)
    parts.append(cargo)

    # Airlock Docking Ring
    bpy.ops.mesh.primitive_cylinder_add(vertices=24, radius=0.085, depth=0.035, location=(0.0, 0.155, 0.52), rotation=(math.pi/2, 0.0, 0.0))
    airlock = bpy.context.active_object
    airlock.name = "Shuttle_Airlock"
    airlock.data.materials.append(m_engine)
    parts.append(airlock)

    # 5. Twin Outboard Engine Nacelles & Pylons
    for side in (-1.0, 1.0):
        tag = "L" if side < 0 else "R"
        # Pylon
        bpy.ops.mesh.primitive_cube_add(size=1.0, location=(side * 0.45, 0.005, 0.20), rotation=(0.0, 0.0, side * 0.08))
        pylon = bpy.context.active_object
        pylon.name = f"Shuttle_Pylon_{tag}"
        pylon.scale = (0.22, 0.045, 0.55)
        pylon.data.materials.append(m_hull)
        parts.append(pylon)

        # Nacelle Body
        bpy.ops.mesh.primitive_cylinder_add(vertices=24, radius=0.115, depth=1.10, location=(side * 0.58, 0.015, 0.25))
        nac = bpy.context.active_object
        nac.name = f"Shuttle_Nacelle_{tag}"
        nac.data.materials.append(m_accent)
        parts.append(nac)

        # Nacelle Ram-Air Intake Cowl (Front)
        bpy.ops.mesh.primitive_cone_add(vertices=24, radius1=0.12, radius2=0.095, depth=0.18, location=(side * 0.58, 0.015, -0.36))
        cowl = bpy.context.active_object
        cowl.name = f"Shuttle_IntakeCowl_{tag}"
        cowl.data.materials.append(m_engine)
        parts.append(cowl)

        # Nacelle Intake Interior
        bpy.ops.mesh.primitive_uv_sphere_add(segments=12, ring_count=8, radius=0.085, location=(side * 0.58, 0.015, -0.34))
        int_rec = bpy.context.active_object
        int_rec.name = f"Shuttle_IntakeGrille_{tag}"
        int_rec.scale = (1.0, 1.0, 0.2)
        int_rec.data.materials.append(m_tiles)
        parts.append(int_rec)

        # Heavy Main Rocket Nozzle (Rear)
        bpy.ops.mesh.primitive_cone_add(vertices=24, radius1=0.085, radius2=0.125, depth=0.32, location=(side * 0.58, 0.015, 0.95))
        noz = bpy.context.active_object
        noz.name = f"Shuttle_Nozzle_{tag}"
        noz.data.materials.append(m_engine)
        parts.append(noz)

        # Exhaust Plasma Throat
        bpy.ops.mesh.primitive_uv_sphere_add(segments=16, ring_count=8, radius=0.078, location=(side * 0.58, 0.015, 0.94))
        pla = bpy.context.active_object
        pla.name = f"Shuttle_Plasma_{tag}"
        pla.scale = (1.0, 1.0, 0.25)
        pla.data.materials.append(m_plasma)
        parts.append(pla)

        # Nav Light on Outer Nacelle Flank
        nav_mat = m_nav_red if side < 0 else m_nav_green
        bpy.ops.mesh.primitive_uv_sphere_add(segments=12, ring_count=8, radius=0.020, location=(side * 0.70, 0.015, -0.05))
        nav = bpy.context.active_object
        nav.name = f"Shuttle_Nav_{tag}"
        nav.data.materials.append(nav_mat)
        parts.append(nav)

    # 6. Center Transom Engine & OMS Pods
    bpy.ops.mesh.primitive_cone_add(vertices=24, radius1=0.065, radius2=0.095, depth=0.22, location=(0.0, -0.015, 0.85))
    center_noz = bpy.context.active_object
    center_noz.name = "Shuttle_CenterNozzle"
    center_noz.data.materials.append(m_engine)
    parts.append(center_noz)

    bpy.ops.mesh.primitive_uv_sphere_add(segments=12, ring_count=6, radius=0.060, location=(0.0, -0.015, 0.84))
    center_pla = bpy.context.active_object
    center_pla.name = "Shuttle_CenterPlasma"
    center_pla.scale = (1.0, 1.0, 0.2)
    center_pla.data.materials.append(m_plasma)
    parts.append(center_pla)

    # 7. Vertical Stabilizer Fin
    bm_fin = bmesh.new()
    fv = [
        bm_fin.verts.new(Vector(( 0.018, 0.12,  0.28))),
        bm_fin.verts.new(Vector((-0.018, 0.12,  0.28))),
        bm_fin.verts.new(Vector((-0.008, 0.38,  0.65))),
        bm_fin.verts.new(Vector(( 0.008, 0.38,  0.65))),
        bm_fin.verts.new(Vector(( 0.009, 0.37,  0.74))),
        bm_fin.verts.new(Vector((-0.009, 0.37,  0.74))),
        bm_fin.verts.new(Vector((-0.018, 0.11,  0.77))),
        bm_fin.verts.new(Vector(( 0.018, 0.11,  0.77))),
    ]
    bm_fin.faces.new([fv[0], fv[3], fv[2], fv[1]])
    bm_fin.faces.new([fv[3], fv[4], fv[5], fv[2]])
    bm_fin.faces.new([fv[4], fv[7], fv[6], fv[5]])
    bm_fin.faces.new([fv[0], fv[1], fv[6], fv[7]])
    bm_fin.faces.new([fv[0], fv[7], fv[4], fv[3]])
    bm_fin.faces.new([fv[1], fv[2], fv[5], fv[6]])
    me_fin = bpy.data.meshes.new('Shuttle_Fin')
    bm_fin.to_mesh(me_fin)
    bm_fin.free()
    obj_fin = bpy.data.objects.new('Shuttle_Fin', me_fin)
    obj_fin.data.materials.append(m_accent)
    parts.append(obj_fin)

    # 8. Reference-driven detail pass
    shuttle_details(parts, {"hull": m_hull, "accent": m_accent, "tiles": m_tiles,
                            "cargo": m_cargo, "cockpit": m_cockpit, "engine": m_engine,
                            "plasma": m_plasma, "nav_red": m_nav_red,
                            "nav_green": m_nav_green})

    # Join All Parts
    for obj in parts:
        if obj.name not in bpy.context.scene.collection.objects:
            bpy.context.scene.collection.objects.link(obj)
        obj.select_set(True)
    bpy.context.view_layer.objects.active = obj_hull
    bpy.ops.object.join()
    ship = bpy.context.active_object
    ship.name = "Shuttle"

    # Normalize to exact ~2.3 length, center at (0,0,0)
    bpy.ops.object.origin_set(type='ORIGIN_GEOMETRY', center='BOUNDS')
    bbox = [ship.matrix_world @ Vector(corner) for corner in ship.bound_box]
    min_z = min(v.z for v in bbox)
    max_z = max(v.z for v in bbox)
    cur_len = max_z - min_z
    scale_fac = 2.30 / cur_len
    ship.scale = (scale_fac, scale_fac, scale_fac)
    bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)

    # Re-center
    bbox = [ship.matrix_world @ Vector(corner) for corner in ship.bound_box]
    min_x = min(v.x for v in bbox)
    max_x = max(v.x for v in bbox)
    min_y = min(v.y for v in bbox)
    max_y = max(v.y for v in bbox)
    min_z = min(v.z for v in bbox)
    max_z = max(v.z for v in bbox)
    center = Vector(((min_x + max_x) * 0.5, (min_y + max_y) * 0.5, (min_z + max_z) * 0.5))
    ship.location = -center
    bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)

    # Smooth shading and edge split
    for poly in ship.data.polygons:
        poly.use_smooth = True
    split_mod = ship.modifiers.new(name="EdgeSplit", type='EDGE_SPLIT')
    split_mod.split_angle = math.radians(35)
    bpy.context.view_layer.objects.active = ship
    bpy.ops.object.modifier_apply(modifier="EdgeSplit")

    # UV Smart Project
    bpy.ops.object.mode_set(mode='EDIT')
    bpy.ops.mesh.select_all(action='SELECT')
    bpy.ops.uv.smart_project(angle_limit=1.15, island_margin=0.015)
    bpy.ops.object.mode_set(mode='OBJECT')

    print(f"Shuttle created successfully: {len(ship.data.vertices)} vertices, {len(ship.data.polygons)} polygons.")
    return ship


# ---------------------------------------------------------------------------
# Showcase Render in Cycles
# ---------------------------------------------------------------------------
def render_showcase(obj, out_png):
    print(f"\n--- Rendering Showcase Image for {obj.name} -> {out_png} ---")
    scene = bpy.context.scene
    scene.render.engine = 'CYCLES'
    scene.cycles.device = 'CPU'
    scene.cycles.samples = 48
    scene.cycles.use_denoising = True
    scene.render.resolution_x = 1280
    scene.render.resolution_y = 720
    scene.render.image_settings.file_format = 'PNG'
    scene.render.image_settings.color_mode = 'RGBA'

    # Camera
    cam_data = bpy.data.cameras.new("ShowcaseCam")
    cam_data.lens = 42
    cam_obj = bpy.data.objects.new("ShowcaseCam", cam_data)
    scene.collection.objects.link(cam_obj)
    scene.camera = cam_obj
    
    cam_pos = Vector((2.2, -2.5, 1.4))
    target = Vector((0.0, 0.0, 0.0))
    direction = target - cam_pos
    rot_quat = direction.to_track_quat('-Z', 'Y')
    cam_obj.location = cam_pos
    cam_obj.rotation_euler = rot_quat.to_euler()

    # Lighting
    # 1. Warm Sun Key Light
    sun_data = bpy.data.lights.new("SunKey", type='SUN')
    sun_data.energy = 5.2
    sun_data.color = (1.0, 0.98, 0.92)
    sun_obj = bpy.data.objects.new("SunKey", sun_data)
    sun_dir = Vector((-0.9, 1.1, -0.8)).normalized()
    sun_obj.rotation_euler = sun_dir.to_track_quat('-Z', 'Y').to_euler()
    scene.collection.objects.link(sun_obj)

    # 2. Cool Blue Rim Light
    rim_data = bpy.data.lights.new("RimFill", type='SUN')
    rim_data.energy = 3.2
    rim_data.color = (0.40, 0.68, 1.0)
    rim_obj = bpy.data.objects.new("RimFill", rim_data)
    rim_dir = Vector((1.0, -0.8, 1.0)).normalized()
    rim_obj.rotation_euler = rim_dir.to_track_quat('-Z', 'Y').to_euler()
    scene.collection.objects.link(rim_obj)

    # Deep space world background
    world = scene.world
    if not world:
        world = bpy.data.worlds.new("SpaceWorld")
        scene.world = world
    world.use_nodes = True
    bg = world.node_tree.nodes.get("Background")
    if bg:
        bg.inputs["Color"].default_value = (0.010, 0.012, 0.022, 1.0)
        bg.inputs["Strength"].default_value = 0.40

    # Dynamic 3/4 cinematic flight tilt for 16:9 widescreen showcase
    if "Shuttle" in obj.name:
        obj.rotation_euler = (math.radians(65), math.radians(-18), math.radians(-35))
    else:
        obj.rotation_euler = (math.radians(-70), math.radians(18), math.radians(35))

    scene.render.filepath = out_png
    bpy.ops.render.render(write_still=True)
    obj.rotation_euler = (0.0, 0.0, 0.0)
    print(f"Render saved to: {out_png}")


# ---------------------------------------------------------------------------
# Main Pipeline
# ---------------------------------------------------------------------------
def main():
    print("Starting astro-voyager Ship Asset Generator & Baker...")
    
    # === 1. DART (Style 0) ===
    dart_obj = build_dart()
    dart_base = os.path.join(RES_MODELS, "dart")
    
    # Export OBJ
    dart_obj_path = f"{dart_base}.obj"
    print(f"Exporting OBJ -> {dart_obj_path}")
    dart_obj.select_set(True)
    bpy.context.view_layer.objects.active = dart_obj
    bpy.ops.wm.obj_export(
        filepath=dart_obj_path,
        export_selected_objects=True,
        export_uv=True,
        export_normals=True,
        export_materials=False,
        export_triangulated_mesh=True,
        forward_axis='Y',
        up_axis='Z'
    )
    
    if "--obj-only" not in sys.argv:
        if "--render-only" not in sys.argv:
            # Bake textures
            bake_pbr_set(dart_obj, dart_base, tex_size=2048)
        
        # Showcase render
        dart_showcase_out = os.path.join(OUT_SHOWCASE, "dart_showcase.png")
        render_showcase(dart_obj, dart_showcase_out)
        dart_doc = os.path.join(DOCS_SCREENS, "dart_showcase.png")
        shutil.copyfile(dart_showcase_out, dart_doc)
        print(f"Copied showcase to {dart_doc}")

    # === 2. SHUTTLE (Style 2) ===
    shuttle_obj = build_shuttle()
    shuttle_base = os.path.join(RES_MODELS, "shuttle")
    
    # Export OBJ
    shuttle_obj_path = f"{shuttle_base}.obj"
    print(f"Exporting OBJ -> {shuttle_obj_path}")
    shuttle_obj.select_set(True)
    bpy.context.view_layer.objects.active = shuttle_obj
    bpy.ops.wm.obj_export(
        filepath=shuttle_obj_path,
        export_selected_objects=True,
        export_uv=True,
        export_normals=True,
        export_materials=False,
        export_triangulated_mesh=True,
        forward_axis='Y',
        up_axis='Z'
    )
    
    if "--obj-only" not in sys.argv:
        if "--render-only" not in sys.argv:
            # Bake textures
            bake_pbr_set(shuttle_obj, shuttle_base, tex_size=2048)
        
        # Showcase render
        shuttle_showcase_out = os.path.join(OUT_SHOWCASE, "shuttle_showcase.png")
        render_showcase(shuttle_obj, shuttle_showcase_out)
        shuttle_doc = os.path.join(DOCS_SCREENS, "shuttle_showcase.png")
        shutil.copyfile(shuttle_showcase_out, shuttle_doc)
        print(f"Copied showcase to {shuttle_doc}")

    print("\n=======================================================")
    print("All tasks completed successfully!")
    print("=======================================================")


if __name__ == '__main__':
    main()
