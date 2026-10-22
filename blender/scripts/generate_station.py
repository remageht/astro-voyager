"""Deterministic PBR 3D asset generation and baking for astro-voyager:
- ISS / Orbital Space Station Model:
  * Central Integrated Truss Structure (ITS)
  * 8 Massive Photovoltaic Solar Array Wings (SAW) on articulated gimbals
  * Pressurized Module Cluster (Zarya, Zvezda, Destiny, Harmony, Columbus, Kibo, Unity)
  * Earth-facing Cupola 7-window observation dome
  * Accordion-folded Thermal Control Radiator Panels (TCS)
  * Docking collars (PMA / IDA) with approach alignment guides
  * Canadarm2 robotic arm and High-Gain Communications Dish
  * Navigation beacons and docking target lights

Produces:
  res/models/station.obj
  res/models/station_albedo.bmp, station_normal.bmp, station_rough.bmp, station_metal.bmp, station_emissive.bmp
  blender/out/final_showcase/station_showcase.png  (1280x720 Cycles)
  blender/out/final_showcase/station_close.png     (1280x720 Cycles)
  docs/screens/station_showcase.png
  docs/screens/station_close.png
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
BUILD_GL_MODELS = os.path.join(REPO_ROOT, "build-gl", "Release", "res", "models")

os.makedirs(RES_MODELS, exist_ok=True)
os.makedirs(OUT_SHOWCASE, exist_ok=True)
os.makedirs(DOCS_SCREENS, exist_ok=True)
os.makedirs(BUILD_GL_MODELS, exist_ok=True)

# ---------------------------------------------------------------------------
# 24-bit uncompressed BMP writer matching src/ObjModel.cpp
# ---------------------------------------------------------------------------
def write_bmp24(filepath, rgb_array):
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
        voronoi.inputs["Scale"].default_value = 32.0
        bump = nodes.new("ShaderNodeBump")
        bump.inputs["Strength"].default_value = 0.45
        bump.inputs["Distance"].default_value = 0.015
        links.new(tex_coord.outputs["UV"], voronoi.inputs["Vector"])
        links.new(voronoi.outputs["Distance"], bump.inputs["Height"])
        links.new(bump.outputs["Normal"], bsdf.inputs["Normal"])
        
    elif bump_type == 'solar_grid':
        tex_coord = nodes.new("ShaderNodeTexCoord")
        brick = nodes.new("ShaderNodeTexBrick")
        brick.inputs["Scale"].default_value = 65.0
        brick.inputs["Mortar Size"].default_value = 0.03
        brick.inputs["Mortar Smooth"].default_value = 0.1
        bump = nodes.new("ShaderNodeBump")
        bump.inputs["Strength"].default_value = 0.8
        bump.inputs["Distance"].default_value = 0.02
        links.new(tex_coord.outputs["UV"], brick.inputs["Vector"])
        links.new(brick.outputs["Color"], bump.inputs["Height"])
        links.new(bump.outputs["Normal"], bsdf.inputs["Normal"])

    elif bump_type == 'radiator_fins':
        tex_coord = nodes.new("ShaderNodeTexCoord")
        wave = nodes.new("ShaderNodeTexWave")
        wave.wave_type = 'BANDS'
        wave.bands_direction = 'X'
        wave.inputs["Scale"].default_value = 40.0
        bump = nodes.new("ShaderNodeBump")
        bump.inputs["Strength"].default_value = 0.7
        bump.inputs["Distance"].default_value = 0.025
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


def bake_pbr_set(obj, base_path, tex_size=2048):
    print(f"\n--- Baking Station PBR texture set ({tex_size}x{tex_size}) ---")
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

    for slot in obj.material_slots:
        set_material_bake_pass(slot.material, 'PBR')
        
    bpy.data.images.remove(bake_img)
    print("Baking complete!")


# ---------------------------------------------------------------------------
# Space Station / ISS Procedural Builder
# ---------------------------------------------------------------------------
def build_space_station():
    print("\n=======================================================")
    print("Building ISS / Orbital Space Station Model...")
    print("=======================================================")
    
    bpy.ops.object.select_all(action='SELECT')
    bpy.ops.object.delete(use_global=False)
    
    # Materials
    m_truss = make_pbr_material("Mat_Truss", base_color=(0.18, 0.20, 0.24), metallic=0.88, roughness=0.35)
    m_module = make_pbr_material("Mat_Module", base_color=(0.88, 0.88, 0.85), metallic=0.15, roughness=0.45, bump_type='panels')
    m_gold_mli = make_pbr_material("Mat_GoldMLI", base_color=(0.95, 0.74, 0.18), metallic=0.92, roughness=0.22)
    m_solar = make_pbr_material("Mat_Solar", base_color=(0.035, 0.075, 0.18), metallic=0.45, roughness=0.18, bump_type='solar_grid')
    m_radiator = make_pbr_material("Mat_Radiator", base_color=(0.94, 0.95, 0.96), metallic=0.10, roughness=0.50, bump_type='radiator_fins')
    m_dock = make_pbr_material("Mat_Dock", base_color=(0.42, 0.45, 0.50), metallic=0.78, roughness=0.32)
    m_cupola = make_pbr_material("Mat_Cupola", base_color=(0.05, 0.20, 0.40), metallic=0.10, roughness=0.08, emission=(0.25, 0.70, 1.0), emission_strength=2.2)
    m_nav_green = make_pbr_material("Mat_NavGreen", base_color=(0.1, 1.0, 0.3), metallic=0.0, roughness=0.2, emission=(0.1, 1.0, 0.3), emission_strength=5.0)
    m_nav_red = make_pbr_material("Mat_NavRed", base_color=(1.0, 0.1, 0.1), metallic=0.0, roughness=0.2, emission=(1.0, 0.1, 0.1), emission_strength=5.0)
    m_dock_strobe = make_pbr_material("Mat_Strobe", base_color=(1.0, 0.95, 0.8), metallic=0.0, roughness=0.2, emission=(1.0, 0.95, 0.8), emission_strength=6.0)

    parts = []

    # 1. Main Integrated Truss Structure (ITS) along X axis
    # Main truss beam spanning from -2.8 to +2.8
    bpy.ops.mesh.primitive_cube_add(size=1.0, location=(0.0, 0.0, 0.0))
    truss_main = bpy.context.active_object
    truss_main.name = "Truss_Main"
    truss_main.scale = (5.6, 0.16, 0.16)
    truss_main.data.materials.append(m_truss)
    parts.append(truss_main)

    # Cross-beam collars and joint hubs along the truss
    for x_pos in [-2.4, -1.6, -0.8, 0.0, 0.8, 1.6, 2.4]:
        bpy.ops.mesh.primitive_cube_add(size=1.0, location=(x_pos, 0.0, 0.0))
        hub = bpy.context.active_object
        hub.name = f"Truss_Hub_{x_pos}"
        hub.scale = (0.22, 0.22, 0.22)
        hub.data.materials.append(m_dock)
        parts.append(hub)

    # 2. Main Solar Array Wings (8 massive panels: 4 Port, 4 Starboard)
    # Each wing pair has an upper and lower wing extending along Y/Z
    solar_positions = [
        # Port (Left, negative X): S4 / S6 segments
        (-2.4,  0.75, 0.0),
        (-2.4, -0.75, 0.0),
        (-1.8,  0.75, 0.0),
        (-1.8, -0.75, 0.0),
        # Starboard (Right, positive X): P4 / P6 segments
        ( 1.8,  0.75, 0.0),
        ( 1.8, -0.75, 0.0),
        ( 2.4,  0.75, 0.0),
        ( 2.4, -0.75, 0.0),
    ]

    for i, (sx, sy, sz) in enumerate(solar_positions):
        # Solar Panel Wing
        bpy.ops.mesh.primitive_cube_add(size=1.0, location=(sx, sy, sz))
        wing = bpy.context.active_object
        wing.name = f"Solar_Array_{i}"
        wing.scale = (0.28, 1.15, 0.018)
        wing.data.materials.append(m_solar)
        parts.append(wing)

        # Central mast and gold MLI canister
        canister_y = 0.12 if sy > 0 else -0.12
        bpy.ops.mesh.primitive_cylinder_add(vertices=16, radius=0.045, depth=0.25, location=(sx, canister_y, sz))
        can = bpy.context.active_object
        can.name = f"Solar_Canister_{i}"
        can.data.materials.append(m_gold_mli)
        parts.append(can)

    # 3. Thermal Control Radiator Panels (TCS) extending backward along -Z
    radiator_x = [-0.9, -0.4, 0.4, 0.9]
    for i, rx in enumerate(radiator_x):
        bpy.ops.mesh.primitive_cube_add(size=1.0, location=(rx, 0.0, -0.65))
        rad = bpy.context.active_object
        rad.name = f"Radiator_{i}"
        rad.scale = (0.16, 0.02, 0.95)
        rad.data.materials.append(m_radiator)
        parts.append(rad)

    # 4. Pressurized Habitation & Lab Modules (Central Longitudinal Spine along Z)
    # Module 1: Unity / Node 1 (central hub)
    bpy.ops.mesh.primitive_cylinder_add(vertices=24, radius=0.24, depth=0.60, location=(0.0, -0.25, 0.0), rotation=(math.pi/2, 0.0, 0.0))
    unity = bpy.context.active_object
    unity.name = "Module_Unity"
    unity.data.materials.append(m_module)
    parts.append(unity)

    # Module 2: Destiny US Laboratory (forward, positive Z)
    bpy.ops.mesh.primitive_cylinder_add(vertices=24, radius=0.25, depth=0.90, location=(0.0, -0.25, 0.72), rotation=(math.pi/2, 0.0, 0.0))
    destiny = bpy.context.active_object
    destiny.name = "Module_Destiny"
    destiny.data.materials.append(m_module)
    parts.append(destiny)

    # Module 3: Harmony / Node 2 (forward docking hub)
    bpy.ops.mesh.primitive_cylinder_add(vertices=24, radius=0.24, depth=0.55, location=(0.0, -0.25, 1.40), rotation=(math.pi/2, 0.0, 0.0))
    harmony = bpy.context.active_object
    harmony.name = "Module_Harmony"
    harmony.data.materials.append(m_module)
    parts.append(harmony)

    # International Docking Adapter (IDA / PMA-2) at forward nose
    bpy.ops.mesh.primitive_cone_add(vertices=24, radius1=0.22, radius2=0.16, depth=0.30, location=(0.0, -0.25, 1.80), rotation=(-math.pi/2, 0.0, 0.0))
    ida_fwd = bpy.context.active_object
    ida_fwd.name = "Docking_IDA_Fwd"
    ida_fwd.data.materials.append(m_dock)
    parts.append(ida_fwd)

    # Docking Petal Ring / Alignment Guide
    bpy.ops.mesh.primitive_torus_add(major_radius=0.18, minor_radius=0.025, location=(0.0, -0.25, 1.95), rotation=(0.0, 0.0, 0.0))
    dock_ring = bpy.context.active_object
    dock_ring.name = "Docking_Ring_Fwd"
    dock_ring.data.materials.append(m_dock_strobe)
    parts.append(dock_ring)

    # Lateral Modules: Columbus (ESA Lab) to starboard (+X) and Kibo (JAXA Lab) to port (-X)
    bpy.ops.mesh.primitive_cylinder_add(vertices=24, radius=0.22, depth=0.75, location=(0.58, -0.25, 1.40), rotation=(0.0, math.pi/2, 0.0))
    columbus = bpy.context.active_object
    columbus.name = "Module_Columbus"
    columbus.data.materials.append(m_module)
    parts.append(columbus)

    bpy.ops.mesh.primitive_cylinder_add(vertices=24, radius=0.24, depth=0.95, location=(-0.68, -0.25, 1.40), rotation=(0.0, math.pi/2, 0.0))
    kibo = bpy.context.active_object
    kibo.name = "Module_Kibo"
    kibo.data.materials.append(m_module)
    parts.append(kibo)

    # Kibo Exposed Facility (porch)
    bpy.ops.mesh.primitive_cube_add(size=1.0, location=(-1.35, -0.25, 1.40))
    kibo_ef = bpy.context.active_object
    kibo_ef.name = "Kibo_ExposedFacility"
    kibo_ef.scale = (0.35, 0.35, 0.35)
    kibo_ef.data.materials.append(m_truss)
    parts.append(kibo_ef)

    # Earth-facing Cupola 7-window observation dome (nadir, -Y direction)
    bpy.ops.mesh.primitive_cylinder_add(vertices=16, radius=0.14, depth=0.12, location=(0.0, -0.52, 0.0), rotation=(0.0, 0.0, 0.0))
    cupola_base = bpy.context.active_object
    cupola_base.name = "Cupola_Base"
    cupola_base.data.materials.append(m_dock)
    parts.append(cupola_base)

    bpy.ops.mesh.primitive_uv_sphere_add(segments=16, ring_count=8, radius=0.12, location=(0.0, -0.56, 0.0))
    cupola_dome = bpy.context.active_object
    cupola_dome.name = "Cupola_Window"
    cupola_dome.scale = (1.0, 0.5, 1.0)
    cupola_dome.data.materials.append(m_cupola)
    parts.append(cupola_dome)

    # Russian Orbital Segment (Aft, negative Z): Zarya (FGB) and Zvezda Service Module
    bpy.ops.mesh.primitive_cylinder_add(vertices=24, radius=0.22, depth=0.85, location=(0.0, -0.25, -0.65), rotation=(math.pi/2, 0.0, 0.0))
    zarya = bpy.context.active_object
    zarya.name = "Module_Zarya"
    zarya.data.materials.append(m_gold_mli)
    parts.append(zarya)

    bpy.ops.mesh.primitive_cylinder_add(vertices=24, radius=0.24, depth=0.95, location=(0.0, -0.25, -1.45), rotation=(math.pi/2, 0.0, 0.0))
    zvezda = bpy.context.active_object
    zvezda.name = "Module_Zvezda"
    zvezda.data.materials.append(m_module)
    parts.append(zvezda)

    # Russian Segment Solar Wings (Zarya / Zvezda secondary wings)
    for side in [-1, 1]:
        bpy.ops.mesh.primitive_cube_add(size=1.0, location=(side * 0.75, -0.25, -0.65))
        rus_wing = bpy.context.active_object
        rus_wing.name = f"Russian_Solar_Wing_{side}"
        rus_wing.scale = (0.85, 0.015, 0.22)
        rus_wing.data.materials.append(m_solar)
        parts.append(rus_wing)

    # Aft Docking Collar (Zvezda Aft Port)
    bpy.ops.mesh.primitive_cylinder_add(vertices=24, radius=0.14, depth=0.18, location=(0.0, -0.25, -1.98), rotation=(math.pi/2, 0.0, 0.0))
    aft_dock = bpy.context.active_object
    aft_dock.name = "Docking_Aft"
    aft_dock.data.materials.append(m_dock)
    parts.append(aft_dock)

    # 5. Canadarm2 Robotic Manipulator Arm
    arm_pts = [
        Vector((0.25, 0.15, 0.3)),
        Vector((0.45, 0.45, 0.5)),
        Vector((0.25, 0.65, 0.9)),
    ]
    for j in range(len(arm_pts) - 1):
        p1 = arm_pts[j]
        p2 = arm_pts[j+1]
        mid = (p1 + p2) * 0.5
        diff = p2 - p1
        dist = diff.length
        bpy.ops.mesh.primitive_cylinder_add(vertices=12, radius=0.035, depth=dist, location=mid)
        arm_seg = bpy.context.active_object
        arm_seg.name = f"Canadarm_Segment_{j}"
        arm_seg.data.materials.append(m_module)
        parts.append(arm_seg)

    # 6. High-Gain Communications Dish Antenna
    bpy.ops.mesh.primitive_cylinder_add(vertices=12, radius=0.025, depth=0.45, location=(0.7, 0.35, -0.2))
    ant_mast = bpy.context.active_object
    ant_mast.name = "Antenna_Mast"
    ant_mast.data.materials.append(m_truss)
    parts.append(ant_mast)

    bpy.ops.mesh.primitive_cone_add(vertices=24, radius1=0.28, radius2=0.02, depth=0.12, location=(0.7, 0.55, -0.2), rotation=(math.radians(35), math.radians(20), 0.0))
    ant_dish = bpy.context.active_object
    ant_dish.name = "Antenna_Dish"
    ant_dish.data.materials.append(m_gold_mli)
    parts.append(ant_dish)

    # 7. Navigation & Docking Target Lights
    # Port Red Nav Light
    bpy.ops.mesh.primitive_uv_sphere_add(segments=8, ring_count=4, radius=0.045, location=(-2.85, 0.0, 0.0))
    nav_r = bpy.context.active_object
    nav_r.name = "Nav_Light_Port_Red"
    nav_r.data.materials.append(m_nav_red)
    parts.append(nav_r)

    # Starboard Green Nav Light
    bpy.ops.mesh.primitive_uv_sphere_add(segments=8, ring_count=4, radius=0.045, location=(2.85, 0.0, 0.0))
    nav_g = bpy.context.active_object
    nav_g.name = "Nav_Light_Starboard_Green"
    nav_g.data.materials.append(m_nav_green)
    parts.append(nav_g)

    # Docking Approach Target Lights
    dock_lights_pos = [
        (-0.35, -0.25, 1.85),
        ( 0.35, -0.25, 1.85),
        ( 0.0,   0.05, 1.85),
        ( 0.0,  -0.55, 1.85),
    ]
    for k, lp in enumerate(dock_lights_pos):
        bpy.ops.mesh.primitive_uv_sphere_add(segments=8, ring_count=4, radius=0.035, location=lp)
        dl = bpy.context.active_object
        dl.name = f"Dock_Strobe_{k}"
        dl.data.materials.append(m_dock_strobe)
        parts.append(dl)

    # Join All Parts
    for obj in parts:
        if obj.name not in bpy.context.scene.collection.objects:
            bpy.context.scene.collection.objects.link(obj)
        obj.select_set(True)
    bpy.context.view_layer.objects.active = truss_main
    bpy.ops.object.join()
    station = bpy.context.active_object
    station.name = "ISS_Space_Station"

    # Normalize to exact scale (~5.5 length across solar truss, centered at (0,0,0))
    bpy.ops.object.origin_set(type='ORIGIN_GEOMETRY', center='BOUNDS')
    bbox = [station.matrix_world @ Vector(corner) for corner in station.bound_box]
    min_x = min(v.x for v in bbox)
    max_x = max(v.x for v in bbox)
    min_y = min(v.y for v in bbox)
    max_y = max(v.y for v in bbox)
    min_z = min(v.z for v in bbox)
    max_z = max(v.z for v in bbox)
    cur_w = max_x - min_x
    scale_fac = 5.4 / cur_w
    station.scale = (scale_fac, scale_fac, scale_fac)
    bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)

    # Re-center
    bbox = [station.matrix_world @ Vector(corner) for corner in station.bound_box]
    center = Vector(((min(v.x for v in bbox) + max(v.x for v in bbox)) * 0.5,
                     (min(v.y for v in bbox) + max(v.y for v in bbox)) * 0.5,
                     (min(v.z for v in bbox) + max(v.z for v in bbox)) * 0.5))
    station.location = -center
    bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)

    # UV Smart Project with clean seams and margins
    bpy.ops.object.mode_set(mode='EDIT')
    bpy.ops.mesh.select_all(action='SELECT')
    bpy.ops.uv.smart_project(angle_limit=1.15, island_margin=0.015)
    bpy.ops.object.mode_set(mode='OBJECT')

    print(f"Station created successfully: {len(station.data.vertices)} vertices, {len(station.data.polygons)} polygons.")
    return station


# ---------------------------------------------------------------------------
# Showcase Renders in Cycles
# ---------------------------------------------------------------------------
def render_station_showcases(station, out_hero_png, out_close_png):
    print(f"\n--- Setting up Cycles Studio Scene for Station Showcase Renders ---")
    scene = bpy.context.scene
    scene.render.engine = 'CYCLES'
    scene.cycles.device = 'CPU'
    scene.cycles.samples = 64
    scene.cycles.use_denoising = True
    scene.render.resolution_x = 1280
    scene.render.resolution_y = 720
    scene.render.image_settings.file_format = 'PNG'
    scene.render.image_settings.color_mode = 'RGBA'

    # Camera
    cam_data = bpy.data.cameras.new("StationCam")
    cam_data.lens = 42
    cam_obj = bpy.data.objects.new("StationCam", cam_data)
    scene.collection.objects.link(cam_obj)
    scene.camera = cam_obj

    # Lighting
    # 1. Brilliant High-Intensity Orbital Sun Key
    sun_data = bpy.data.lights.new("OrbitalSun", type='SUN')
    sun_data.energy = 5.5
    sun_data.color = (1.0, 0.98, 0.94)
    sun_obj = bpy.data.objects.new("OrbitalSun", sun_data)
    sun_obj.rotation_euler = (math.radians(50), math.radians(35), math.radians(-45))
    scene.collection.objects.link(sun_obj)

    # 2. Cool Blue Earth-Shine Albedo Rim Light
    earth_shine = bpy.data.lights.new("EarthAlbedo", type='SUN')
    earth_shine.energy = 2.4
    earth_shine.color = (0.28, 0.55, 0.95)
    earth_obj = bpy.data.objects.new("EarthAlbedo", earth_shine)
    earth_obj.rotation_euler = (math.radians(-40), math.radians(-30), math.radians(135))
    scene.collection.objects.link(earth_obj)

    # Deep space world background
    world = scene.world
    if not world:
        world = bpy.data.worlds.new("StationWorld")
        scene.world = world
    world.use_nodes = True
    bg = world.node_tree.nodes.get("Background")
    if bg:
        bg.inputs["Color"].default_value = (0.008, 0.012, 0.022, 1.0)
        bg.inputs["Strength"].default_value = 0.40

    # === Render 1: Hero Perspective of the full station ===
    print(f"Rendering Hero View -> {out_hero_png} ...")
    cam_pos = Vector((-3.8, 2.4, 4.2))
    target = Vector((0.0, -0.15, 0.2))
    direction = target - cam_pos
    cam_obj.location = cam_pos
    cam_obj.rotation_euler = direction.to_track_quat('-Z', 'Y').to_euler()

    scene.render.filepath = out_hero_png
    bpy.ops.render.render(write_still=True)
    print(f"Saved: {out_hero_png}")

    # === Render 2: Close-up of the Habitation Modules, Cupola and Docking Hub ===
    print(f"Rendering Close-Up View -> {out_close_png} ...")
    cam_close_pos = Vector((1.4, -0.9, 2.5))
    close_target = Vector((0.0, -0.35, 1.0))
    cam_obj.location = cam_close_pos
    cam_obj.rotation_euler = (close_target - cam_close_pos).to_track_quat('-Z', 'Y').to_euler()

    scene.render.filepath = out_close_png
    bpy.ops.render.render(write_still=True)
    print(f"Saved: {out_close_png}")


# ---------------------------------------------------------------------------
# Main Pipeline
# ---------------------------------------------------------------------------
def main():
    print("Starting astro-voyager ISS / Space Station Asset Generator...")
    station = build_space_station()
    station_base = os.path.join(RES_MODELS, "station")

    # 1. Export OBJ (Wavefront, forward Y, up Z)
    station_obj_path = f"{station_base}.obj"
    print(f"Exporting OBJ -> {station_obj_path}")
    station.select_set(True)
    bpy.context.view_layer.objects.active = station
    bpy.ops.wm.obj_export(
        filepath=station_obj_path,
        export_selected_objects=True,
        export_uv=True,
        export_normals=True,
        export_materials=False,
        export_triangulated_mesh=True,
        forward_axis='Y',
        up_axis='Z'
    )
    # Also copy to build-gl
    shutil.copyfile(station_obj_path, os.path.join(BUILD_GL_MODELS, "station.obj"))

    # 2. Bake PBR Texture Set
    bake_pbr_set(station, station_base, tex_size=2048)
    for ext in ["_albedo.bmp", "_normal.bmp", "_rough.bmp", "_metal.bmp", "_emissive.bmp"]:
        src_f = f"{station_base}{ext}"
        dst_f = os.path.join(BUILD_GL_MODELS, f"station{ext}")
        shutil.copyfile(src_f, dst_f)
    print("Copied all textures to build-gl/Release/res/models/")

    # 3. Cycles Showcase Renders
    hero_out = os.path.join(OUT_SHOWCASE, "station_showcase.png")
    close_out = os.path.join(OUT_SHOWCASE, "station_close.png")
    render_station_showcases(station, hero_out, close_out)

    # 4. Copy to docs/screens
    hero_doc = os.path.join(DOCS_SCREENS, "station_showcase.png")
    close_doc = os.path.join(DOCS_SCREENS, "station_close.png")
    shutil.copyfile(hero_out, hero_doc)
    shutil.copyfile(close_out, close_doc)
    print(f"Copied showcases to {hero_doc} and {close_doc}")

    print("\n=======================================================")
    print("Space Station Generation & Rendering Complete!")
    print("=======================================================")


if __name__ == '__main__':
    main()
