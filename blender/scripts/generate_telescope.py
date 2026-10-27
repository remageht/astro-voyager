"""Deterministic PBR 3D asset generation and baking for astro-voyager:
- Deep Space Science Observatory / Telescope (JWST-inspired)

Produces:
  res/models/telescope.obj
  res/models/telescope_albedo.bmp
  res/models/telescope_normal.bmp
  res/models/telescope_rough.bmp
  res/models/telescope_metal.bmp
  res/models/telescope_emissive.bmp
  blender/out/final_showcase/telescope_showcase.png  (1280x720 Cycles)
  docs/screens/telescope_showcase.png
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
    
    if bump_type == 'shield':
        # Foil wrinkle bump
        tex_coord = nodes.new("ShaderNodeTexCoord")
        noise = nodes.new("ShaderNodeTexNoise")
        noise.inputs["Scale"].default_value = 18.0
        noise.inputs["Detail"].default_value = 4.0
        noise.inputs["Roughness"].default_value = 0.65
        
        bump = nodes.new("ShaderNodeBump")
        bump.inputs["Strength"].default_value = 0.45
        bump.inputs["Distance"].default_value = 0.02
        
        links.new(tex_coord.outputs["UV"], noise.inputs["Vector"])
        links.new(noise.outputs["Fac"], bump.inputs["Height"])
        links.new(bump.outputs["Normal"], bsdf.inputs["Normal"])
        
    elif bump_type == 'solar':
        # Grid lines
        tex_coord = nodes.new("ShaderNodeTexCoord")
        brick = nodes.new("ShaderNodeTexBrick")
        brick.inputs["Scale"].default_value = 32.0
        brick.inputs["Mortar Size"].default_value = 0.03
        
        bump = nodes.new("ShaderNodeBump")
        bump.inputs["Strength"].default_value = 0.50
        bump.inputs["Distance"].default_value = 0.015
        
        links.new(tex_coord.outputs["UV"], brick.inputs["Vector"])
        links.new(brick.outputs["Color"], bump.inputs["Height"])
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
# Geometry Builder: Deep Space Telescope (JWST)
# ---------------------------------------------------------------------------
def build_telescope():
    print("\n================ Building Deep Space Telescope (JWST) ================")
    bpy.ops.wm.read_factory_settings(use_empty=True)
    parts = []

    # Materials
    m_gold = make_pbr_material("JWST_GoldMirror", (1.00, 0.82, 0.35), 0.98, 0.08)
    m_graphite = make_pbr_material("JWST_GraphiteTruss", (0.12, 0.13, 0.15), 0.70, 0.42)
    m_shield = make_pbr_material("JWST_Sunshield", (0.90, 0.88, 0.85), 0.90, 0.28, bump_type='shield')
    m_bus = make_pbr_material("JWST_SpacecraftBus", (0.80, 0.82, 0.85), 0.85, 0.32)
    m_solar = make_pbr_material("JWST_SolarPanel", (0.05, 0.08, 0.18), 0.40, 0.18, bump_type='solar')
    m_antenna = make_pbr_material("JWST_Antenna", (0.88, 0.90, 0.92), 0.82, 0.30)
    m_sensor_led = make_pbr_material("JWST_SensorLED", (0.1, 0.8, 1.0), 0.0, 0.1, emission=(0.1, 0.8, 1.0), emission_strength=4.0)
    m_comm_glow = make_pbr_material("JWST_CommGlow", (1.0, 0.5, 0.1), 0.0, 0.1, emission=(1.0, 0.5, 0.1), emission_strength=3.0)

    # 1. 18-Segment Gold Primary Mirror Array
    # Hexagon radius R = 0.135, center-to-center distance D = sqrt(3)*R = 0.2338
    hex_radius = 0.135
    hex_thickness = 0.022
    D = math.sqrt(3.0) * hex_radius
    
    # Grid coordinates for the 18 segments in 2 concentric rings
    # Ring 1 (6 hexes):
    hex_centers = []
    for i in range(6):
        a = i * math.pi / 3.0 + math.pi / 6.0
        hex_centers.append((D * math.cos(a), D * math.sin(a)))
    # Ring 2 (12 hexes):
    # 6 at vertices (radius 2*D*cos(30) = 2*D):
    for i in range(6):
        a = i * math.pi / 3.0 + math.pi / 6.0
        hex_centers.append((2.0 * D * math.cos(a), 2.0 * D * math.sin(a)))
    # 6 at edge midpoints:
    for i in range(6):
        a = i * math.pi / 3.0
        hex_centers.append((math.sqrt(3.0) * D * math.cos(a), math.sqrt(3.0) * D * math.sin(a)))

    for h_idx, (hx, hy) in enumerate(hex_centers):
        # Create 6-sided cylinder (hexagon)
        bpy.ops.mesh.primitive_cylinder_add(
            vertices=6,
            radius=hex_radius * 0.98, # Tiny gap between segments
            depth=hex_thickness,
            location=(hx, hy, 0.18),
            rotation=(0.0, 0.0, math.pi / 6.0)
        )
        hex_obj = bpy.context.active_object
        hex_obj.name = f"JWST_Hex_{h_idx+1}"
        hex_obj.data.materials.append(m_gold)
        parts.append(hex_obj)

    # 2. Central Instrument Backplane & Cassegrain Baffle
    bpy.ops.mesh.primitive_cylinder_add(
        vertices=24,
        radius=0.12,
        depth=0.16,
        location=(0.0, 0.0, 0.22)
    )
    baffle = bpy.context.active_object
    baffle.name = "JWST_CenterBaffle"
    baffle.data.materials.append(m_graphite)
    parts.append(baffle)

    # Backplane Structural Frame (holding the mirror segments)
    # Backplane Structural Frame (matching 18-hex perimeter)
    bpy.ops.mesh.primitive_cylinder_add(
        vertices=12,
        radius=0.54,
        depth=0.035,
        location=(0.0, 0.0, 0.14)
    )
    backplane = bpy.context.active_object
    backplane.name = "JWST_Backplane"
    backplane.data.materials.append(m_graphite)
    parts.append(backplane)

    # 3. Secondary Mirror Tripod Mast & Convex Secondary Mirror
    # Mast apex at Z = 0.88
    apex_z = 0.88
    strut_angles = [math.pi / 2.0, 7.0 * math.pi / 6.0, 11.0 * math.pi / 6.0]
    for s_i, ang in enumerate(strut_angles):
        base_x = 0.50 * math.cos(ang)
        base_y = 0.50 * math.sin(ang)
        base_z = 0.18
        
        dx = 0.0 - base_x
        dy = 0.0 - base_y
        dz = apex_z - base_z
        length = math.sqrt(dx*dx + dy*dy + dz*dz)
        mid = Vector(((base_x + 0.0)*0.5, (base_y + 0.0)*0.5, (base_z + apex_z)*0.5))
        
        bpy.ops.mesh.primitive_cylinder_add(
            vertices=12,
            radius=0.016,
            depth=length,
            location=mid
        )
        strut = bpy.context.active_object
        strut.name = f"JWST_Strut_{s_i+1}"
        direction = Vector((dx, dy, dz)).normalized()
        rot_quat = Vector((0, 0, 1)).rotation_difference(direction)
        strut.rotation_euler = rot_quat.to_euler()
        strut.data.materials.append(m_graphite)
        parts.append(strut)

    # Convex Secondary Mirror at apex
    bpy.ops.mesh.primitive_cylinder_add(
        vertices=16,
        radius=0.10,
        depth=0.025,
        location=(0.0, 0.0, apex_z - 0.02)
    )
    sec_mirror = bpy.context.active_object
    sec_mirror.name = "JWST_SecondaryMirror"
    sec_mirror.data.materials.append(m_gold)
    parts.append(sec_mirror)

    # 4. Multi-Layer Kite Sunshield (5 distinct Kapton membrane layers)
    # Diamond/kite shaped layers spreading out wide beneath backplane
    shield_layers = 5
    for l_idx in range(shield_layers):
        lz = 0.04 - l_idx * 0.038
        scale_k = 1.0 - l_idx * 0.030
        
        bm_s = bmesh.new()
        # Diamond vertices: Nose (+Y), Tail (-Y), Port (-X), Starboard (+X)
        y_nose = 1.70 * scale_k
        y_tail = -1.55 * scale_k
        x_span = 1.25 * scale_k
        
        # 3D thin foil sheet with top and bottom vertices
        v0 = bm_s.verts.new(Vector((  0.0,    y_nose, lz + 0.003)))
        v1 = bm_s.verts.new(Vector(( x_span,   0.05,  lz + 0.003)))
        v2 = bm_s.verts.new(Vector((  0.0,    y_tail, lz + 0.003)))
        v3 = bm_s.verts.new(Vector((-x_span,   0.05,  lz + 0.003)))

        v4 = bm_s.verts.new(Vector((  0.0,    y_nose, lz - 0.003)))
        v5 = bm_s.verts.new(Vector(( x_span,   0.05,  lz - 0.003)))
        v6 = bm_s.verts.new(Vector((  0.0,    y_tail, lz - 0.003)))
        v7 = bm_s.verts.new(Vector((-x_span,   0.05,  lz - 0.003)))

        bm_s.faces.new([v0, v1, v2, v3]) # Top face
        bm_s.faces.new([v7, v6, v5, v4]) # Bottom face
        bm_s.faces.new([v0, v4, v5, v1]) # Side rim
        bm_s.faces.new([v1, v5, v6, v2])
        bm_s.faces.new([v2, v6, v7, v3])
        bm_s.faces.new([v3, v7, v4, v0])
        
        me_s = bpy.data.meshes.new(f'JWST_Sunshield_L{l_idx+1}')
        bm_s.to_mesh(me_s)
        bm_s.free()
        obj_s = bpy.data.objects.new(f'JWST_Sunshield_L{l_idx+1}', me_s)
        obj_s.data.materials.append(m_shield)
        parts.append(obj_s)

    # Sunshield Mid-Boom Arms (+X and -X spreader trusses)
    for side_x in (1.0, -1.0):
        bpy.ops.mesh.primitive_cylinder_add(
            vertices=12,
            radius=0.020,
            depth=2.50,
            location=(0.0, 0.05, -0.04),
            rotation=(0.0, math.pi / 2.0, 0.0)
        )
        boom = bpy.context.active_object
        boom.name = f"JWST_Boom_{'R' if side_x > 0 else 'L'}"
        boom.data.materials.append(m_graphite)
        parts.append(boom)

    # 5. Spacecraft Bus & Service Module (Below Sunshield on Hot Side)
    # Main Avionics Cube
    bpy.ops.mesh.primitive_cube_add(
        size=1.0,
        location=(0.0, -0.15, -0.26)
    )
    bus = bpy.context.active_object
    bus.name = "JWST_BusModule"
    bus.scale = (0.44, 0.44, 0.22)
    bus.data.materials.append(m_bus)
    parts.append(bus)

    # Sensor status LEDs on bus
    bpy.ops.mesh.primitive_uv_sphere_add(
        segments=12,
        ring_count=8,
        radius=0.022,
        location=(0.20, -0.15, -0.22)
    )
    sled = bpy.context.active_object
    sled.name = "JWST_SensorLED"
    sled.data.materials.append(m_sensor_led)
    parts.append(sled)

    # 6. Solar Array Panel (Mounted angled at the aft end of the bus)
    bpy.ops.mesh.primitive_cube_add(
        size=1.0,
        location=(0.0, -0.75, -0.36),
        rotation=(math.radians(22), 0.0, 0.0)
    )
    solar = bpy.context.active_object
    solar.name = "JWST_SolarPanel"
    solar.scale = (0.55, 0.40, 0.015)
    solar.data.materials.append(m_solar)
    parts.append(solar)

    # 7. High-Gain Communication Antenna Dish
    bpy.ops.mesh.primitive_cylinder_add(
        vertices=24,
        radius=0.18,
        depth=0.035,
        location=(0.28, -0.32, -0.42),
        rotation=(math.radians(-35), math.radians(20), 0.0)
    )
    hga = bpy.context.active_object
    hga.name = "JWST_AntennaDish"
    hga.data.materials.append(m_antenna)
    parts.append(hga)

    # Antenna Feed Horn / Comm Glow
    bpy.ops.mesh.primitive_uv_sphere_add(
        segments=12,
        ring_count=8,
        radius=0.024,
        location=(0.32, -0.36, -0.48)
    )
    feed = bpy.context.active_object
    feed.name = "JWST_AntennaFeed"
    feed.data.materials.append(m_comm_glow)
    parts.append(feed)

    # -----------------------------------------------------------------------
    # Join All Parts into Unified Hero Mesh
    # -----------------------------------------------------------------------
    for obj in parts:
        if obj.name not in bpy.context.scene.collection.objects:
            bpy.context.scene.collection.objects.link(obj)
        obj.select_set(True)
    bpy.context.view_layer.objects.active = backplane
    bpy.ops.object.join()
    telescope = bpy.context.active_object
    telescope.name = "Telescope"

    # Scale to ~2.2 units length for standard in-game landmark scale
    bpy.ops.object.origin_set(type='ORIGIN_GEOMETRY', center='BOUNDS')
    bbox = [telescope.matrix_world @ Vector(corner) for corner in telescope.bound_box]
    min_y = min(v.y for v in bbox)
    max_y = max(v.y for v in bbox)
    cur_len = max_y - min_y
    scale_fac = 2.20 / cur_len
    telescope.scale = (scale_fac, scale_fac, scale_fac)
    bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)

    # Re-center
    bbox = [telescope.matrix_world @ Vector(corner) for corner in telescope.bound_box]
    min_x = min(v.x for v in bbox)
    max_x = max(v.x for v in bbox)
    min_y = min(v.y for v in bbox)
    max_y = max(v.y for v in bbox)
    min_z = min(v.z for v in bbox)
    max_z = max(v.z for v in bbox)
    center = Vector(((min_x + max_x) * 0.5, (min_y + max_y) * 0.5, (min_z + max_z) * 0.5))
    telescope.location = -center
    bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)

    # Smooth shading and crisp edge split
    for poly in telescope.data.polygons:
        poly.use_smooth = True
    split_mod = telescope.modifiers.new(name="EdgeSplit", type='EDGE_SPLIT')
    split_mod.split_angle = math.radians(35)
    bpy.context.view_layer.objects.active = telescope
    bpy.ops.object.modifier_apply(modifier="EdgeSplit")

    # Smart UV projection for baking
    bpy.ops.object.mode_set(mode='EDIT')
    bpy.ops.mesh.select_all(action='SELECT')
    bpy.ops.uv.smart_project(angle_limit=1.15, island_margin=0.015)
    bpy.ops.object.mode_set(mode='OBJECT')

    print(f"Telescope created successfully: {len(telescope.data.vertices)} vertices, {len(telescope.data.polygons)} polygons.")
    return telescope


# ---------------------------------------------------------------------------
# Showcase Renderer (Cycles)
# ---------------------------------------------------------------------------
def render_showcase(tele_obj, out_png):
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

    # Hero 3/4 perspective camera angled towards the gold mirror. The earlier
    # 42mm shot from (2.8,-3.0,2.2) left the 18-segment mirror too small next
    # to the sunshield; a higher 55mm three-quarter view keeps both readable.
    cam_data = bpy.data.cameras.new("ShowcaseCam")
    cam_data.lens = 55
    cam_obj = bpy.data.objects.new("ShowcaseCam", cam_data)
    cam_obj.location = (2.4, -2.4, 3.4)
    target = Vector((0.0, 0.0, 0.12))
    direction = target - cam_obj.location
    rot_quat = direction.to_track_quat('-Z', 'Y')
    cam_obj.rotation_euler = rot_quat.to_euler()
    scene.collection.objects.link(cam_obj)
    scene.camera = cam_obj

    # Key Golden Sunlight (shining onto gold mirror face)
    sun_data = bpy.data.lights.new("KeySun", type='SUN')
    sun_data.energy = 6.0
    sun_data.color = (1.0, 0.98, 0.92)
    sun_obj = bpy.data.objects.new("KeySun", sun_data)
    sun_dir = Vector((-0.6, 0.8, -1.0)).normalized()
    sun_obj.rotation_euler = sun_dir.to_track_quat('-Z', 'Y').to_euler()
    scene.collection.objects.link(sun_obj)

    # Cool Blue Cosmic Rim Light
    rim_data = bpy.data.lights.new("RimFill", type='SUN')
    rim_data.energy = 3.0
    rim_data.color = (0.35, 0.65, 1.0)
    rim_obj = bpy.data.objects.new("RimFill", rim_data)
    rim_dir = Vector((1.0, -0.6, 0.5)).normalized()
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

    scene.render.filepath = out_png
    bpy.ops.render.render(write_still=True)
    print(f"Showcase render saved to: {out_png}")


# ---------------------------------------------------------------------------
# Main Pipeline
# ---------------------------------------------------------------------------
def main():
    print("Starting astro-voyager Deep Space Telescope Generator & Baker...")
    
    tele_obj = build_telescope()
    tele_base = os.path.join(RES_MODELS, "telescope")
    
    # Export OBJ
    tele_obj_path = f"{tele_base}.obj"
    print(f"Exporting OBJ -> {tele_obj_path}")
    tele_obj.select_set(True)
    bpy.context.view_layer.objects.active = tele_obj
    bpy.ops.wm.obj_export(
        filepath=tele_obj_path,
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
            bake_pbr_set(tele_obj, tele_base, tex_size=2048)
        
        # Showcase render
        showcase_out = os.path.join(OUT_SHOWCASE, "telescope_showcase.png")
        render_showcase(tele_obj, showcase_out)
        doc_dest = os.path.join(DOCS_SCREENS, "telescope_showcase.png")
        shutil.copyfile(showcase_out, doc_dest)
        print(f"Copied showcase to {doc_dest}")

    print("\n=======================================================")
    print("Deep Space Telescope asset generation completed successfully!")
    print("=======================================================")


if __name__ == '__main__':
    main()
