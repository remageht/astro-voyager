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
# Geometry Builder: Dart (Style 0)
# ---------------------------------------------------------------------------
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
    bpy.ops.mesh.primitive_cone_add(vertices=24, radius1=0.09, radius2=0.118, depth=0.36, location=(0.0, 0.0, 0.94), rotation=(math.pi/2, 0.0, 0.0))
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
        bpy.ops.mesh.primitive_cone_add(vertices=16, radius1=0.035, radius2=0.046, depth=0.20, location=(side * 0.12, 0.015, 0.88), rotation=(math.pi/2, 0.0, 0.0))
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
    bpy.ops.mesh.primitive_cylinder_add(vertices=24, radius=0.085, depth=0.035, location=(0.0, 0.155, 0.52))
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
        bpy.ops.mesh.primitive_cylinder_add(vertices=24, radius=0.115, depth=1.10, location=(side * 0.58, 0.015, 0.25), rotation=(math.pi/2, 0.0, 0.0))
        nac = bpy.context.active_object
        nac.name = f"Shuttle_Nacelle_{tag}"
        nac.data.materials.append(m_accent)
        parts.append(nac)

        # Nacelle Ram-Air Intake Cowl (Front)
        bpy.ops.mesh.primitive_cone_add(vertices=24, radius1=0.12, radius2=0.095, depth=0.18, location=(side * 0.58, 0.015, -0.36), rotation=(-math.pi/2, 0.0, 0.0))
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
        bpy.ops.mesh.primitive_cone_add(vertices=24, radius1=0.085, radius2=0.125, depth=0.32, location=(side * 0.58, 0.015, 0.95), rotation=(math.pi/2, 0.0, 0.0))
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
    bpy.ops.mesh.primitive_cone_add(vertices=24, radius1=0.065, radius2=0.095, depth=0.22, location=(0.0, -0.015, 0.85), rotation=(math.pi/2, 0.0, 0.0))
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
