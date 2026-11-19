"""Deterministic PBR 3D asset generation and baking for astro-voyager:
- Style 1: "Ring Ship" (Alcubierre Tri-Ring Warp Cruiser)

Produces:
  res/models/ring_ship.obj
  res/models/ring_ship_albedo.bmp
  res/models/ring_ship_normal.bmp
  res/models/ring_ship_rough.bmp
  res/models/ring_ship_metal.bmp
  res/models/ring_ship_emissive.bmp
  blender/out/final_showcase/ring_ship_showcase.png  (1280x720 Cycles)
  docs/screens/ring_ship_showcase.png
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
        voronoi.inputs["Scale"].default_value = 28.0
        
        bump = nodes.new("ShaderNodeBump")
        bump.inputs["Strength"].default_value = 0.55
        bump.inputs["Distance"].default_value = 0.015
        
        links.new(tex_coord.outputs["UV"], voronoi.inputs["Vector"])
        links.new(voronoi.outputs["Distance"], bump.inputs["Height"])
        links.new(bump.outputs["Normal"], bsdf.inputs["Normal"])
        
    elif bump_type == 'coils':
        tex_coord = nodes.new("ShaderNodeTexCoord")
        wave = nodes.new("ShaderNodeTexWave")
        wave.wave_type = 'BANDS'
        wave.bands_direction = 'X'
        wave.inputs["Scale"].default_value = 40.0
        
        bump = nodes.new("ShaderNodeBump")
        bump.inputs["Strength"].default_value = 0.70
        bump.inputs["Distance"].default_value = 0.02
        
        links.new(tex_coord.outputs["UV"], wave.inputs["Vector"])
        links.new(wave.outputs["Color"], bump.inputs["Height"])
        links.new(bump.outputs["Normal"], bsdf.inputs["Normal"])

    elif bump_type == 'nozzle':
        tex_coord = nodes.new("ShaderNodeTexCoord")
        wave = nodes.new("ShaderNodeTexWave")
        wave.wave_type = 'BANDS'
        wave.bands_direction = 'Y'
        wave.inputs["Scale"].default_value = 35.0
        
        bump = nodes.new("ShaderNodeBump")
        bump.inputs["Strength"].default_value = 0.6
        bump.inputs["Distance"].default_value = 0.015
        
        links.new(tex_coord.outputs["UV"], wave.inputs["Vector"])
        links.new(wave.outputs["Color"], bump.inputs["Height"])
        links.new(bump.outputs["Normal"], bsdf.inputs["Normal"])

    return mat


def set_material_bake_pass(mat, pass_type):
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
    print(f"\n--- Baking PBR texture set ({tex_size}x{tex_size}) for {obj.name} ---")
    scene = bpy.context.scene
    scene.render.engine = 'CYCLES'
    scene.cycles.device = 'CPU'
    scene.cycles.samples = 4
    
    bake_img = bpy.data.images.new(f"Bake_{obj.name}", tex_size, tex_size)
    
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
# Geometry Builder: Ring Ship (Style 1)
# ---------------------------------------------------------------------------
# ---------------------------------------------------------------------------
# Detail pass primitives (same contract as generate_ships.py after #189).
# Frame of this hull: -Z is the nose, +Z aft, +Y up, X spanwise. Cylinder, cone
# and torus primitives are born on their local Z, so rot=(0,0,0) already aims
# them fore-aft; (half,0,0) tips them DOWN, (-half,0,0) UP, (0,+-half,0) out
# the sides.
#
# Ring Ship has one constraint the other hulls do not: shaders/ship.vert spins
# only vertices with length(xy) >= 0.372, and only inside three z-slabs of the
# NORMALIZED mesh ([-0.36,-0.10], [0.14,0.31], [0.57,0.76]) - the three rings.
# Anything outboard of r=0.366 therefore has to stay within +-0.09 of a ring
# centre (-0.44, 0.0, +0.44 pre-normalize) or the ring tears; everything else
# must stay inboard of 0.366 so the static pylons keep their grip.
# ---------------------------------------------------------------------------
def r_box(name, loc, dims, rot=(0.0, 0.0, 0.0), mat=None, parts=None):
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


def r_cyl(name, loc, r, depth, rot=(0.0, 0.0, 0.0), segs=24, mat=None, parts=None):
    bpy.ops.mesh.primitive_cylinder_add(vertices=segs, radius=r, depth=depth,
                                        location=loc, rotation=rot)
    o = bpy.context.active_object
    o.name = name
    if mat:
        o.data.materials.append(mat)
    if parts is not None:
        parts.append(o)
    return o


def r_cone(name, loc, r1, r2, depth, rot=(0.0, 0.0, 0.0), segs=24, mat=None, parts=None):
    bpy.ops.mesh.primitive_cone_add(vertices=segs, radius1=r1, radius2=r2,
                                    depth=depth, location=loc, rotation=rot)
    o = bpy.context.active_object
    o.name = name
    if mat:
        o.data.materials.append(mat)
    if parts is not None:
        parts.append(o)
    return o


def r_torus(name, loc, major, minor, rot=(0.0, 0.0, 0.0), segs=48, sides=12,
            mat=None, parts=None):
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


def ring_ship_details(parts, M):
    """Reference pass for the Alcubierre tri-ring cruiser: hull banding, sensor
    and deflector hardware, cupola framing, conduit clamps, radiator field,
    per-ring coil and braking hardware and a full stern cluster. Everything is
    real geometry, so the silhouette survives without textures."""
    hull, accent, coil = M["hull"], M["accent"], M["coil"]
    emitter, cockpit, engine, plasma = M["emitter"], M["cockpit"], M["engine"], M["plasma"]
    half = math.pi / 2
    ring_z = (-0.44, 0.0, 0.44)

    # 1. Fuselage banding and keel stringers (inboard, static)
    for z, r in ((-1.05, 0.036), (-0.80, 0.070), (-0.55, 0.094), (-0.25, 0.104),
                 (0.00, 0.107), (0.25, 0.104), (0.45, 0.097), (0.60, 0.090),
                 (0.72, 0.078)):
        r_torus(f"RingArt_band_{z:+.2f}", (0.0, 0.0, z), r + 0.004, 0.006,
                segs=32, sides=6, mat=accent, parts=parts)
    for side in (-1.0, 1.0):
        r_box(f"RingArt_keel_{side:+.0f}", (side * 0.086, 0.0, 0.02),
              (0.006, 0.012, 1.10), mat=accent, parts=parts)

    # 2. Nose: pitot boom, sensor louvres, forward mast
    # The pitot stays inside the original nose tip (z=-1.42) and the stern plug
    # inside the old aux-plasma extent (z=+0.849): normalization divides by the
    # bounding-box length, so detail that sticks out at either end slides all
    # three rings out of the spin slabs in shaders/ship.vert. Checked below.
    r_cyl("RingArt_pitot", (0.0, 0.0, -1.30), 0.006, 0.24, segs=10, mat=accent, parts=parts)
    r_cone("RingArt_pitotTip", (0.0, 0.0, -1.40), 0.006, 0.001, 0.04,
           rot=(math.pi, 0.0, 0.0), segs=10, mat=hull, parts=parts)
    for k, z in enumerate((-1.16, -1.02, -0.90)):
        r_box(f"RingArt_sensorVane_{k}", (0.0, 0.0, z), (0.052 - k * 0.006, 0.004, 0.030),
              mat=accent, parts=parts)
    for side in (-1.0, 1.0):
        r_cyl(f"RingArt_noseAnt_{side:+.0f}", (side * 0.030, -0.030, -0.98), 0.004, 0.16,
              rot=(0.0, side * 0.30, 0.0), segs=8, mat=accent, parts=parts)

    # 3. Forward deflector collar: lip, vanes and focusing ring
    r_torus("RingArt_deflLip", (0.0, 0.0, -0.855), 0.086, 0.010, segs=28, sides=8,
            mat=coil, parts=parts)
    for k in range(10):
        a = k * math.tau / 10
        r_box(f"RingArt_deflVane_{k}", (0.082 * math.cos(a), 0.082 * math.sin(a), -0.80),
              (0.020, 0.006, 0.050), rot=(0.0, 0.0, a), mat=accent, parts=parts)
    r_torus("RingArt_deflField", (0.0, 0.0, -0.88), 0.062, 0.005, segs=24, sides=6,
            mat=emitter, parts=parts)

    # 4. Cockpit cupola: frame ribs, sill and sunshade
    for k, z in enumerate((-0.50, -0.42, -0.34, -0.28)):
        r_box(f"RingArt_cupolaRib_{k}", (0.0, 0.086, z), (0.058 - k * 0.004, 0.040, 0.006),
              mat=accent, parts=parts)
    r_box("RingArt_cupolaSill", (0.0, 0.062, -0.545), (0.070, 0.020, 0.010),
          mat=hull, parts=parts)
    r_box("RingArt_cupolaVisor", (0.0, 0.090, -0.575), (0.062, 0.030, 0.008),
          rot=(0.42, 0.0, 0.0), mat=accent, parts=parts)

    # 5. Spine conduit clamps and ventral radiator field (inboard, static)
    for side_y in (1.0, -1.0):
        tag = "D" if side_y > 0 else "V"
        for k, z in enumerate((-0.24, -0.08, 0.08, 0.24)):
            r_box(f"RingArt_conduitClamp_{tag}_{k}", (0.0, side_y * 0.112, z),
                  (0.034, 0.008, 0.014), mat=accent, parts=parts)
    for side in (-1.0, 1.0):
        for k in range(5):
            z = -0.10 + k * 0.11
            r_box(f"RingArt_radiator_{side:+.0f}_{k}", (side * 0.150, -0.020, z),
                  (0.130, 0.004, 0.070), rot=(0.0, side * 0.18, 0.0), mat=accent,
                  parts=parts)
            r_box(f"RingArt_radFin_{side:+.0f}_{k}", (side * 0.150, -0.020, z),
                  (0.130, 0.014, 0.006), rot=(0.0, side * 0.18, 0.0), mat=hull,
                  parts=parts)

    # 6. Per-ring hardware. Coil collars, braking calipers and node caps all sit
    # within +-0.09 of the ring centre so the shader spin band keeps them rigid.
    for r_idx, rz in enumerate(ring_z):
        tag = f"R{r_idx + 1}"
        r_torus(f"RingArt_coilCollarA_{tag}", (0.0, 0.0, rz - 0.030), 0.42, 0.014,
                segs=56, sides=8, mat=coil, parts=parts)
        r_torus(f"RingArt_coilCollarB_{tag}", (0.0, 0.0, rz + 0.030), 0.42, 0.014,
                segs=56, sides=8, mat=coil, parts=parts)
        for k in range(16):
            a = k * math.tau / 16
            r_box(f"RingArt_coilWinding_{tag}_{k}",
                  (0.42 * math.cos(a), 0.42 * math.sin(a), rz),
                  (0.014, 0.014, 0.062), rot=(0.0, 0.0, a), mat=accent, parts=parts)
        # Braking calipers grip the ring from inside at r=0.358 (< 0.366 static)
        for k in range(8):
            a = k * math.tau / 8 + math.tau / 16
            r_box(f"RingArt_brake_{tag}_{k}",
                  (0.358 * math.cos(a), 0.358 * math.sin(a), rz),
                  (0.026, 0.026, 0.020), rot=(0.0, 0.0, a), mat=hull, parts=parts)
        # Node caps turn each bare cylinder into a tuned emitter head
        for k in range(8):
            a = k * math.pi / 4.0
            nx, ny = 0.42 * math.cos(a), 0.42 * math.sin(a)
            r_cyl(f"RingArt_nodeCap_{tag}_{k}", (nx, ny, rz + 0.038), 0.020, 0.012,
                  segs=14, mat=emitter, parts=parts)
            r_cyl(f"RingArt_nodeBase_{tag}_{k}", (nx, ny, rz - 0.038), 0.022, 0.010,
                  segs=14, mat=accent, parts=parts)

    # 7. Aft ring service bridge and strobes (inside slab 3, so it spins with R3)
    r_torus("RingArt_aftServiceRing", (0.0, 0.0, 0.44), 0.376, 0.006,
            segs=48, sides=6, mat=accent, parts=parts)
    for side in (-1.0, 1.0):
        r_box(f"RingArt_strobe_{side:+.0f}", (side * 0.435, 0.0, 0.485),
              (0.014, 0.020, 0.010), mat=plasma, parts=parts)

    # 8. Stern cluster: gimbal, feed lines, CTV rings, tail cone and RCS
    r_torus("RingArt_mainGimbal", (0.0, 0.0, 0.74), 0.070, 0.008, segs=24, sides=6,
            mat=accent, parts=parts)
    for k in range(3):
        a = k * math.tau / 3
        r_cyl(f"RingArt_feedLine_{k}", (0.052 * math.cos(a), 0.052 * math.sin(a), 0.70),
              0.010, 0.16, segs=10, mat=accent, parts=parts)
    for k in range(6):
        a = k * math.tau / 6
        cx, cy = 0.088 * math.cos(a), 0.088 * math.sin(a)
        r_torus(f"RingArt_auxCTV_{k}", (cx, cy, 0.845), 0.034, 0.004, segs=16, sides=6,
                mat=accent, parts=parts)
    r_cone("RingArt_tailPlug", (0.0, 0.0, 0.80), 0.020, 0.040, 0.10, segs=18,
           mat=engine, parts=parts)
    for side in (-1.0, 1.0):
        r_cyl(f"RingArt_omsPod_{side:+.0f}", (side * 0.115, 0.0, 0.66), 0.028, 0.14,
              segs=16, mat=hull, parts=parts)
        r_cone(f"RingArt_omsBell_{side:+.0f}", (side * 0.115, 0.0, 0.76), 0.014, 0.024,
               0.06, segs=14, mat=engine, parts=parts)
    for z in (0.30, 0.52):
        for side in (-1.0, 1.0):
            r_cone(f"RingArt_rcs_{z:+.2f}_{side:+.0f}", (side * 0.098, 0.060, z),
                   0.008, 0.016, 0.030, rot=(-half, 0.0, 0.0), segs=10, mat=engine,
                   parts=parts)
    r_cyl("RingArt_aftMast", (0.0, 0.118, 0.58), 0.005, 0.14, rot=(-half, 0.0, 0.0),
          segs=8, mat=accent, parts=parts)

    # 9. Formation lights along the spine and ventral strip
    for z in (-0.30, 0.02, 0.34):
        r_box(f"RingArt_formationD_{z:+.2f}", (0.0, 0.122, z), (0.050, 0.004, 0.010),
              mat=plasma, parts=parts)


def build_ring_ship():
    print("\n================ Building Ring Ship (Style 1) ================")
    bpy.ops.wm.read_factory_settings(use_empty=True)
    parts = []

    # Materials
    m_hull = make_pbr_material("RingHull", (0.84, 0.88, 0.94), 0.88, 0.30, bump_type='panels')
    m_accent = make_pbr_material("RingAccent", (0.13, 0.14, 0.17), 0.70, 0.40, bump_type='panels') # Dark armor pylons
    m_coil = make_pbr_material("RingCoil", (0.76, 0.82, 0.90), 0.96, 0.16, bump_type='coils')       # Superconducting coil
    m_emitter = make_pbr_material("RingEmitter", (0.05, 0.85, 1.0), 0.0, 0.1, emission=(0.02, 0.90, 1.0), emission_strength=4.5) # Quantum plasma
    m_cockpit = make_pbr_material("RingCockpit", (0.03, 0.06, 0.10), 0.10, 0.05, emission=(0.10, 0.75, 1.0), emission_strength=2.4)
    m_engine = make_pbr_material("RingEngine", (0.15, 0.16, 0.19), 0.95, 0.26, bump_type='nozzle')
    m_plasma = make_pbr_material("RingPlasma", (1.0, 0.85, 0.6), 0.0, 0.5, emission=(1.0, 0.45, 0.12), emission_strength=5.0)
    m_nav_red = make_pbr_material("RingNavRed", (0.85, 0.05, 0.05), 0.0, 0.1, emission=(1.0, 0.08, 0.08), emission_strength=3.5)
    m_nav_green = make_pbr_material("RingNavGreen", (0.05, 0.85, 0.15), 0.0, 0.1, emission=(0.08, 1.0, 0.18), emission_strength=3.5)

    # 1. Main Needle Fuselage
    bm = bmesh.new()
    sections = [
        # (z, r_x, r_y) - streamlined needle fuselage
        (-1.42, 0.000, 0.000), # Nose needle tip
        (-1.25, 0.016, 0.016),
        (-1.05, 0.034, 0.034),
        (-0.80, 0.068, 0.068), # Sensor forward bulge
        (-0.55, 0.092, 0.092),
        (-0.25, 0.102, 0.098),
        ( 0.00, 0.105, 0.100), # Center waist
        ( 0.25, 0.102, 0.098),
        ( 0.45, 0.095, 0.092),
        ( 0.60, 0.088, 0.086), # Stern cowl transition
        ( 0.72, 0.076, 0.076),
    ]
    
    prev_ring = None
    N = 16
    for idx, (z, rx, ry) in enumerate(sections):
        if idx == 0:
            v_apex = bm.verts.new(Vector((0.0, 0.0, z)))
            prev_ring = [v_apex] * N
            continue
        ring = []
        for i in range(N):
            theta = 2.0 * math.pi * i / N
            x = rx * math.cos(theta)
            y = ry * math.sin(theta)
            ring.append(bm.verts.new(Vector((x, y, z))))
        if idx == 1:
            for i in range(N):
                bm.faces.new([v_apex, ring[i], ring[(i+1)%N]])
        else:
            for i in range(N):
                bm.faces.new([prev_ring[i], ring[i], ring[(i+1)%N], prev_ring[(i+1)%N]])
        prev_ring = ring
    # Cap the back
    bm.faces.new(list(reversed(prev_ring)))

    me_fuse = bpy.data.meshes.new('Ring_Fuselage')
    bm.to_mesh(me_fuse)
    bm.free()
    obj_fuse = bpy.data.objects.new('Ring_Fuselage', me_fuse)
    obj_fuse.data.materials.append(m_hull)
    parts.append(obj_fuse)

    # 2. Cockpit Observation Cupola
    bpy.ops.mesh.primitive_uv_sphere_add(segments=24, ring_count=12, radius=1.0, location=(0.0, 0.086, -0.42))
    cockpit = bpy.context.active_object
    cockpit.name = "Ring_Cockpit"
    cockpit.scale = (0.052, 0.034, 0.18)
    cockpit.data.materials.append(m_cockpit)
    parts.append(cockpit)

    # 3. Dorsal & Ventral Power Spine / Conduit Channels
    for side_y in (1.0, -1.0):
        bpy.ops.mesh.primitive_cube_add(size=1.0, location=(0.0, side_y * 0.104, 0.0))
        spine = bpy.context.active_object
        spine.name = f"Ring_Spine_{'D' if side_y > 0 else 'V'}"
        spine.scale = (0.026, 0.016, 0.58)
        spine.data.materials.append(m_accent)
        parts.append(spine)

        # Luminous plasma conduit line inside spine
        bpy.ops.mesh.primitive_cube_add(size=1.0, location=(0.0, side_y * 0.114, 0.0))
        conduit = bpy.context.active_object
        conduit.name = f"Ring_Conduit_{'D' if side_y > 0 else 'V'}"
        conduit.scale = (0.012, 0.006, 0.52)
        conduit.data.materials.append(m_emitter)
        parts.append(conduit)

    # 4. Forward Deflector Ring Collar
    bpy.ops.mesh.primitive_cylinder_add(vertices=24, radius=0.082, depth=0.04, location=(0.0, 0.0, -0.82))
    deflector = bpy.context.active_object
    deflector.name = "Ring_Deflector"
    deflector.data.materials.append(m_accent)
    parts.append(deflector)

    # 5. Three Alcubierre Resonance Warp Rings (-0.44, 0.0, +0.44)
    # v4.1: каждое кольцо крепится к корпусу ДВУМЯ статическими пилонами,
    # но кольца не связаны друг с другом. Ориентация чередуется (1/3 — по X,
    # 2 — по Y), чтобы пилоны не образовывали продольные «рельсы»/клетку.
    ring_positions = [-0.44, 0.0, 0.44]
    for r_idx, rz in enumerate(ring_positions):
        r_tag = f"R{r_idx+1}"
        
        # 5a. Outer Structural Torus
        bpy.ops.mesh.primitive_torus_add(
            major_radius=0.42,
            minor_radius=0.026,
            major_segments=48,
            minor_segments=16,
            location=(0.0, 0.0, rz)
        )
        torus_out = bpy.context.active_object
        torus_out.name = f"Ring_Outer_{r_tag}"
        torus_out.data.materials.append(m_coil)
        parts.append(torus_out)

        # 5b. Inner Luminous Quantum Plasma Band
        bpy.ops.mesh.primitive_torus_add(
            major_radius=0.39,
            minor_radius=0.012,
            major_segments=36,
            minor_segments=12,
            location=(0.0, 0.0, rz)
        )
        torus_emit = bpy.context.active_object
        torus_emit.name = f"Ring_Emitter_{r_tag}"
        torus_emit.data.materials.append(m_emitter)
        parts.append(torus_emit)

        # 5c. Harmonic Field Nodes (8 per ring)
        for node_i in range(8):
            ang = node_i * math.pi / 4.0
            nx = 0.42 * math.cos(ang)
            ny = 0.42 * math.sin(ang)
            bpy.ops.mesh.primitive_cylinder_add(vertices=12, radius=0.016, depth=0.055, location=(nx, ny, rz))
            node_obj = bpy.context.active_object
            node_obj.name = f"Ring_Node_{r_tag}_{node_i}"
            node_obj.data.materials.append(m_accent)
            parts.append(node_obj)

        # 5d. Hull mounts (static). Outer end at r=0.370 in pre-normalize
        # space (< shader spin threshold 0.372 after ~0.987 normalize), so the
        # strut never enters the spinning torus band. Inner end sits inside
        # the fuselage. Two per ring, 180° apart, alternating axis per ring.
        mount_axis = (0.0, 1.0) if r_idx == 1 else (1.0, 0.0)
        for s in (-1.0, 1.0):
            mx = s * 0.2325 * mount_axis[0]
            my = s * 0.2325 * mount_axis[1]
            bpy.ops.mesh.primitive_cube_add(size=1.0, location=(mx, my, rz))
            mount = bpy.context.active_object
            mount.name = f"Ring_Mount_{r_tag}_{'P' if s < 0 else 'S'}"
            mount.scale = (0.275 if mount_axis[0] else 0.016,
                           0.275 if mount_axis[1] else 0.016,
                           0.016)
            mount.data.materials.append(m_accent)
            parts.append(mount)

    # 6. Navigation Lights on Aft Ring (Z = 0.44)
    # Port (Left, -X = Red)
    bpy.ops.mesh.primitive_uv_sphere_add(segments=12, ring_count=8, radius=0.022, location=(-0.435, 0.0, 0.44))
    nav_l = bpy.context.active_object
    nav_l.name = "Ring_Nav_Port"
    nav_l.data.materials.append(m_nav_red)
    parts.append(nav_l)

    # Starboard (Right, +X = Green)
    bpy.ops.mesh.primitive_uv_sphere_add(segments=12, ring_count=8, radius=0.022, location=(0.435, 0.0, 0.44))
    nav_r = bpy.context.active_object
    nav_r.name = "Ring_Nav_Starboard"
    nav_r.data.materials.append(m_nav_green)
    parts.append(nav_r)

    # 7. Stern Sublight Propulsion Cluster
    # 7a. Central Main Engine Nozzle
    # Identity rotation: the fuselage length is the Z axis (nose at -Z), so a
    # bell born on local Z already points aft. The old rotation=(pi/2,0,0) aimed
    # it down instead - same defect class as #189, caught here by #199.
    bpy.ops.mesh.primitive_cone_add(vertices=24, radius1=0.065, radius2=0.095, depth=0.18, location=(0.0, 0.0, 0.82))
    main_noz = bpy.context.active_object
    main_noz.name = "Ring_MainNozzle"
    main_noz.data.materials.append(m_engine)
    parts.append(main_noz)

    # Main Engine Plasma Disc
    bpy.ops.mesh.primitive_cylinder_add(vertices=24, radius=0.058, depth=0.02, location=(0.0, 0.0, 0.88))
    main_pla = bpy.context.active_object
    main_pla.name = "Ring_MainPlasma"
    main_pla.data.materials.append(m_plasma)
    parts.append(main_pla)

    # 7b. Cluster of 6 Attitude Vectoring Thrusters (radius 0.088)
    for k in range(6):
        a = k * (2.0 * math.pi / 6.0)
        cx = 0.088 * math.cos(a)
        cy = 0.088 * math.sin(a)
        
        bpy.ops.mesh.primitive_cone_add(vertices=16, radius1=0.022, radius2=0.034, depth=0.12, location=(cx, cy, 0.78))
        aux_noz = bpy.context.active_object
        aux_noz.name = f"Ring_AuxNozzle_{k}"
        aux_noz.data.materials.append(m_engine)
        parts.append(aux_noz)
        
        bpy.ops.mesh.primitive_uv_sphere_add(segments=12, ring_count=6, radius=0.019, location=(cx, cy, 0.83))
        aux_pla = bpy.context.active_object
        aux_pla.name = f"Ring_AuxPlasma_{k}"
        aux_pla.scale = (1.0, 1.0, 0.2)
        aux_pla.data.materials.append(m_plasma)
        parts.append(aux_pla)

    # 8. Reference-driven detail pass (#197)
    ring_ship_details(parts, {"hull": m_hull, "accent": m_accent, "coil": m_coil,
                              "emitter": m_emitter, "cockpit": m_cockpit,
                              "engine": m_engine, "plasma": m_plasma})

    # -----------------------------------------------------------------------
    # Join All Parts into Unified Hero Mesh
    # -----------------------------------------------------------------------
    for obj in parts:
        if obj.name not in bpy.context.scene.collection.objects:
            bpy.context.scene.collection.objects.link(obj)
        obj.select_set(True)
    bpy.context.view_layer.objects.active = obj_fuse
    bpy.ops.object.join()
    ship = bpy.context.active_object
    ship.name = "RingShip"

    # Normalize to exact ~2.32 length matching canonical astro-voyager scale
    bpy.ops.object.origin_set(type='ORIGIN_GEOMETRY', center='BOUNDS')
    bbox = [ship.matrix_world @ Vector(corner) for corner in ship.bound_box]
    min_z = min(v.z for v in bbox)
    max_z = max(v.z for v in bbox)
    cur_len = max_z - min_z
    scale_fac = 2.32 / cur_len
    ship.scale = (scale_fac, scale_fac, scale_fac)
    bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)

    # Re-center (Middle ring at Z=0.0)
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

    # Spin-band guard. shaders/ship.vert rotates only vertices with
    # length(xy) >= 0.372 and only inside three z-slabs of THIS normalized mesh.
    # Normalization divides by the bounding-box length, so any detail that
    # extends the nose or stern slides the rings out of their slabs and the
    # drive silently stops spinning - or tears, at the smoothstep edges. Fail
    # the build here rather than shipping a ring ship that only looks right
    # in a still frame.
    spin_slabs = ((-0.36, -0.10), (0.14, 0.31), (0.57, 0.76))
    for rz, slab in zip(ring_positions, spin_slabs):
        z_norm = rz * scale_fac - center.z
        if not (slab[0] <= z_norm <= slab[1]):
            raise RuntimeError(
                f"ring at pre-normalize z={rz:+.2f} lands at z={z_norm:+.3f}, "
                f"outside shader spin slab {slab}. Detail geometry has moved the "
                f"hull bounding box; pull it back inside nose/stern limits.")
    print("Ring spin slabs: all three rings inside their shader bands.", flush=True)

    # Smooth shading and crisp edge split
    for poly in ship.data.polygons:
        poly.use_smooth = True
    split_mod = ship.modifiers.new(name="EdgeSplit", type='EDGE_SPLIT')
    split_mod.split_angle = math.radians(38)
    bpy.context.view_layer.objects.active = ship
    bpy.ops.object.modifier_apply(modifier="EdgeSplit")

    # Smart UV projection for baking
    bpy.ops.object.mode_set(mode='EDIT')
    bpy.ops.mesh.select_all(action='SELECT')
    bpy.ops.uv.smart_project(angle_limit=1.15, island_margin=0.015)
    bpy.ops.object.mode_set(mode='OBJECT')

    print(f"Ring Ship created successfully: {len(ship.data.vertices)} vertices, {len(ship.data.polygons)} polygons.")
    return ship


# ---------------------------------------------------------------------------
# Showcase Renderer (Cycles)
# ---------------------------------------------------------------------------
def render_showcase(ship_obj, out_png):
    print(f"\n--- Rendering Showcase Image -> {out_png} ---")
    scene = bpy.context.scene
    scene.render.engine = 'CYCLES'
    scene.cycles.device = 'CPU'
    scene.cycles.samples = 32
    scene.render.resolution_x = 1280
    scene.render.resolution_y = 720
    scene.render.film_transparent = False

    # Remove existing cameras/lights
    for o in list(scene.objects):
        if o.type in ('CAMERA', 'LIGHT'):
            bpy.data.objects.remove(o, do_unlink=True)

    # Hero 3/4 perspective camera framed for 16:9 diagonal silhouette
    cam_data = bpy.data.cameras.new("ShowcaseCam")
    cam_data.lens = 42
    cam_obj = bpy.data.objects.new("ShowcaseCam", cam_data)
    cam_obj.location = (2.2, -2.5, 1.4)
    target = Vector((0.0, 0.0, 0.0))
    direction = target - cam_obj.location
    rot_quat = direction.to_track_quat('-Z', 'Y')
    cam_obj.rotation_euler = rot_quat.to_euler()
    scene.collection.objects.link(cam_obj)
    scene.camera = cam_obj

    # Key Sun Light (illuminates nose, cockpit, and rings from front-left)
    sun_data = bpy.data.lights.new("KeySun", type='SUN')
    sun_data.energy = 5.5
    sun_data.color = (1.0, 0.98, 0.92)
    sun_obj = bpy.data.objects.new("KeySun", sun_data)
    sun_dir = Vector((-0.9, 1.1, -0.8)).normalized()
    sun_obj.rotation_euler = sun_dir.to_track_quat('-Z', 'Y').to_euler()
    scene.collection.objects.link(sun_obj)

    # Cool Rim Light (from rear-right)
    rim_data = bpy.data.lights.new("RimFill", type='SUN')
    rim_data.energy = 3.2
    rim_data.color = (0.35, 0.70, 1.0)
    rim_obj = bpy.data.objects.new("RimFill", rim_data)
    rim_dir = Vector((1.0, -0.8, 1.0)).normalized()
    rim_obj.rotation_euler = rim_dir.to_track_quat('-Z', 'Y').to_euler()
    scene.collection.objects.link(rim_obj)

    # Deep Space Background
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
    ship_obj.rotation_euler = (math.radians(-72), math.radians(18), math.radians(35))

    scene.render.filepath = out_png
    bpy.ops.render.render(write_still=True)
    ship_obj.rotation_euler = (0.0, 0.0, 0.0)
    print(f"Showcase render saved to: {out_png}")


# ---------------------------------------------------------------------------
# Main Pipeline
# ---------------------------------------------------------------------------
def main():
    print("Starting astro-voyager Ring Ship Generator & Baker...")
    
    ship_obj = build_ring_ship()
    ring_base = os.path.join(RES_MODELS, "ring_ship")
    
    # Export OBJ
    ring_obj_path = f"{ring_base}.obj"
    print(f"Exporting OBJ -> {ring_obj_path}")
    ship_obj.select_set(True)
    bpy.context.view_layer.objects.active = ship_obj
    bpy.ops.wm.obj_export(
        filepath=ring_obj_path,
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
            bake_pbr_set(ship_obj, ring_base, tex_size=2048)
        
        # Showcase render
        showcase_out = os.path.join(OUT_SHOWCASE, "ring_ship_showcase.png")
        render_showcase(ship_obj, showcase_out)
        doc_dest = os.path.join(DOCS_SCREENS, "ring_ship_showcase.png")
        shutil.copyfile(showcase_out, doc_dest)
        print(f"Copied showcase to {doc_dest}")

    print("\n=======================================================")
    print("Ring Ship asset generation completed successfully!")
    print("=======================================================")


if __name__ == '__main__':
    main()
