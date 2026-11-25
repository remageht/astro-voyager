"""
build_interstellar_ark.py
=========================
Procedural modeling and PBR asset baking for the Interstellar Ark Starship:
- res/models/ark_ship.obj + res/models/ark_ship.mtl
- 24-bit uncompressed BMP textures (1024x1024):
  - ark_ship_albedo.bmp
  - ark_ship_normal.bmp
  - ark_ship_rough.bmp
  - ark_ship_metal.bmp
  - ark_ship_emissive.bmp
- Cycles showcase render: docs/screens/ark_ship_showcase.png

Run via Blender 5.2:
& "C:\\Program Files\\Blender Foundation\\Blender 5.2\\blender.exe" -b -P tools/build_interstellar_ark.py
"""

import os
import sys
import math
import struct
import bpy
from mathutils import Vector, Matrix

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
MODELS_DIR = os.path.join(ROOT, "res", "models")
SCREENS_DIR = os.path.join(ROOT, "docs", "screens")
os.makedirs(MODELS_DIR, exist_ok=True)
os.makedirs(SCREENS_DIR, exist_ok=True)

# =========================================================================
# BMP 24-bit Writer Helper
# =========================================================================
def write_bmp24(filepath, width, height, rgb_buffer):
    """Writes a 24-bit uncompressed Windows BMP file."""
    row_bytes = width * 3
    padding = (4 - (row_bytes % 4)) % 4
    stride = row_bytes + padding
    image_size = stride * height
    file_size = 54 + image_size

    header = bytearray(54)
    # File Header
    header[0:2] = b'BM'
    struct.pack_into('<I', header, 2, file_size)
    struct.pack_into('<I', header, 6, 0)
    struct.pack_into('<I', header, 10, 54) # offset
    # Info Header
    struct.pack_into('<I', header, 14, 40)
    struct.pack_into('<i', header, 18, width)
    struct.pack_into('<i', header, 22, height)
    struct.pack_into('<H', header, 26, 1)  # planes
    struct.pack_into('<H', header, 28, 24) # bpp
    struct.pack_into('<I', header, 30, 0)  # compression BI_RGB
    struct.pack_into('<I', header, 34, image_size)
    struct.pack_into('<i', header, 38, 2835) # 72 DPI
    struct.pack_into('<i', header, 42, 2835)
    struct.pack_into('<I', header, 46, 0)
    struct.pack_into('<I', header, 50, 0)

    # Pixel data (bottom-up, BGR order)
    data = bytearray(image_size)
    for y in range(height):
        src_row = y * width * 3
        dst_row = y * stride
        for x in range(width):
            src_idx = src_row + x * 3
            dst_idx = dst_row + x * 3
            r = rgb_buffer[src_idx]
            g = rgb_buffer[src_idx + 1]
            b = rgb_buffer[src_idx + 2]
            data[dst_idx] = b
            data[dst_idx + 1] = g
            data[dst_idx + 2] = r

    with open(filepath, 'wb') as f:
        f.write(header)
        f.write(data)
    print(f"[BMP] Saved: {filepath} ({width}x{height}, {file_size/1024:.1f} KB)")


# =========================================================================
# 1. GENERATE PROCEDURAL PBR TEXTURES FOR INTERSTELLAR ARK
# =========================================================================
def generate_ark_textures(size=1024):
    print(f"[TEXTURES] Generating Ark Ship PBR textures ({size}x{size})...")
    albedo = bytearray(size * size * 3)
    normal = bytearray(size * size * 3)
    rough = bytearray(size * size * 3)
    metal = bytearray(size * size * 3)
    emissive = bytearray(size * size * 3)

    for y in range(size):
        v = y / size
        for x in range(size):
            u = x / size
            idx = (y * size + x) * 3

            # Baseline composite dark alloy hull
            r, g, b = 42, 48, 58         # Deep slate titanium
            nx, ny, nz = 128, 128, 255   # Flat normal
            ro = 130                     # Semi-matte composite
            me = 190                     # High metallic
            er, eg, eb = 0, 0, 0         # No emission by default

            # --- UV Layout Regions ---
            # u in [0.0..0.4]: Primary Centrifuge Ring & Spokes
            # u in [0.4..0.7]: Spine Hull & Reactor
            # u in [0.7..1.0]: Nose Bridge, Dish & Radiators

            if u < 0.4:
                # Habitat Centrifuge Ring
                ring_u = u / 0.4
                panel_x = (ring_u * 32.0) % 1.0
                panel_y = (v * 16.0) % 1.0

                # Panel grooves
                if panel_x < 0.05 or panel_y < 0.06:
                    r, g, b = 28, 32, 40
                    nx = 110 if panel_x < 0.025 else 146
                    ro = 180
                    me = 120
                else:
                    # Windows & observation galleries
                    if 0.35 < panel_y < 0.65 and 0.15 < panel_x < 0.85:
                        win_grid = (panel_x * 8.0) % 1.0
                        if win_grid > 0.25:
                            # Luminous cyan starlight windows
                            r, g, b = 180, 245, 255
                            er, eg, eb = 0, 220, 255
                            ro = 30
                            me = 40
                    elif 0.82 < panel_y < 0.95:
                        # Warning amber accent band along perimeter rim
                        r, g, b = 220, 130, 25
                        ro = 110
                        me = 160
            elif u < 0.7:
                # Central Spine & Reactor
                spine_u = (u - 0.4) / 0.3
                seg_x = (spine_u * 12.0) % 1.0
                seg_y = (v * 24.0) % 1.0

                if seg_x < 0.06 or seg_y < 0.05:
                    r, g, b = 22, 26, 34
                    ro = 200
                    me = 90
                else:
                    # Interleaved thermal radiator tiles & piping
                    if seg_y > 0.6:
                        r, g, b = 60, 68, 80
                        me = 230
                        ro = 90
                    # Cryo conduit / plasma status indicator
                    if 0.25 < seg_x < 0.32:
                        r, g, b = 20, 200, 240
                        er, eg, eb = 0, 180, 230
            else:
                # Command Bridge, Parabolic Dish & Sensors
                bow_u = (u - 0.7) / 0.3
                if bow_u < 0.5:
                    # High-gain dish mesh surface (gold-beryllium / titanium lattice)
                    grid_d = (math.sin(bow_u * 40.0) * math.cos(v * 40.0))
                    if grid_d > 0.3:
                        r, g, b = 175, 150, 70 # Gold reflective mesh
                        me = 240
                        ro = 60
                    else:
                        r, g, b = 80, 85, 95
                        me = 210
                        ro = 100
                else:
                    # Bridge canopy & avionics
                    if 0.3 < v < 0.7:
                        r, g, b = 160, 230, 255
                        er, eg, eb = 0, 200, 240 # Glowing forward cockpit glass
                        ro = 25
                        me = 20

            albedo[idx] = r
            albedo[idx + 1] = g
            albedo[idx + 2] = b

            normal[idx] = nx
            normal[idx + 1] = ny
            normal[idx + 2] = nz

            rough[idx] = ro
            rough[idx + 1] = ro
            rough[idx + 2] = ro

            metal[idx] = me
            metal[idx + 1] = me
            metal[idx + 2] = me

            emissive[idx] = er
            emissive[idx + 1] = eg
            emissive[idx + 2] = eb

    # Write all 5 textures
    write_bmp24(os.path.join(MODELS_DIR, "ark_ship_albedo.bmp"), size, size, albedo)
    write_bmp24(os.path.join(MODELS_DIR, "ark_ship_normal.bmp"), size, size, normal)
    write_bmp24(os.path.join(MODELS_DIR, "ark_ship_rough.bmp"), size, size, rough)
    write_bmp24(os.path.join(MODELS_DIR, "ark_ship_metal.bmp"), size, size, metal)
    write_bmp24(os.path.join(MODELS_DIR, "ark_ship_emissive.bmp"), size, size, emissive)


# =========================================================================
# 2. BUILD INTERSTELLAR ARK STARSHIP 3D MESH
# =========================================================================
def build_ark_mesh():
    print("[MESH] Building Interstellar Ark Starship 3D geometry...")
    bpy.ops.wm.read_factory_settings(use_empty=True)
    scene = bpy.context.scene

    mesh_objs = []

    def create_cylinder(name, loc, r1, r2, depth, vertices=32, rot=(0,0,0)):
        bpy.ops.mesh.primitive_cylinder_add(
            vertices=vertices,
            radius=r1,
            depth=depth,
            location=loc,
            rotation=rot
        )
        obj = bpy.context.active_object
        obj.name = name
        mesh_objs.append(obj)
        return obj

    def create_cone(name, loc, r1, r2, depth, vertices=32, rot=(0,0,0)):
        bpy.ops.mesh.primitive_cone_add(
            vertices=vertices,
            radius1=r1,
            radius2=r2,
            depth=depth,
            location=loc,
            rotation=rot
        )
        obj = bpy.context.active_object
        obj.name = name
        mesh_objs.append(obj)
        return obj

    def create_torus(name, loc, r_major, r_minor, seg_major=64, seg_minor=16, rot=(0,0,0)):
        bpy.ops.mesh.primitive_torus_add(
            major_segments=seg_major,
            minor_segments=seg_minor,
            major_radius=r_major,
            minor_radius=r_minor,
            location=loc,
            rotation=rot
        )
        obj = bpy.context.active_object
        obj.name = name
        mesh_objs.append(obj)
        return obj

    def create_cube(name, loc, scale, rot=(0,0,0)):
        bpy.ops.mesh.primitive_cube_add(
            size=1.0,
            location=loc,
            rotation=rot
        )
        obj = bpy.context.active_object
        obj.scale = scale
        bpy.ops.object.transform_apply(scale=True)
        obj.name = name
        mesh_objs.append(obj)
        return obj

    # --- AXIAL SPINE HULL (Flight axis Z) ---
    # Central corridor tube
    create_cylinder("Spine_Main", (0, 0, 0.0), 0.38, 0.38, 4.8, vertices=48)
    # Forward reinforced sleeve
    create_cylinder("Spine_FwdSleeve", (0, 0, 1.4), 0.44, 0.44, 1.2, vertices=32)
    # Midship collar
    create_cylinder("Spine_MidCollar", (0, 0, 0.3), 0.48, 0.48, 0.5, vertices=48)
    # Aft engineering sleeve
    create_cylinder("Spine_AftSleeve", (0, 0, -1.3), 0.42, 0.42, 1.4, vertices=32)

    # --- NOSE COMMAND BRIDGE & HIGH-GAIN DISH ---
    # Bridge module cone
    create_cone("Bridge_Cone", (0, 0, 2.55), 0.44, 0.22, 1.1, vertices=32)
    # Forward nose cap
    create_cone("Nose_Cap", (0, 0, 3.25), 0.22, 0.06, 0.5, vertices=24)
    # Forward observation cupola
    create_cylinder("Bridge_Cupola", (0, 0.18, 2.8), 0.14, 0.10, 0.18, vertices=16, rot=(math.radians(35), 0, 0))

    # Sensor needle probes
    for angle_deg in [0, 90, 180, 270]:
        rad = math.radians(angle_deg)
        px = math.cos(rad) * 0.14
        py = math.sin(rad) * 0.14
        create_cylinder(f"Sensor_Probe_{angle_deg}", (px, py, 3.65), 0.014, 0.003, 0.6, vertices=8)
    # Central probe
    create_cylinder("Central_Probe", (0, 0, 3.85), 0.022, 0.005, 0.9, vertices=12)

    # High-gain parabolic communications dish antenna (prominent on dorsal-starboard nose)
    # Dish base gimbal
    create_cylinder("Dish_Gimbal", (0.15, 0.38, 2.35), 0.10, 0.10, 0.22, vertices=16, rot=(math.radians(20), math.radians(-15), 0))
    # Dish reflector parabolic cone (facing up-forward)
    dish_rot = (math.radians(-55), math.radians(-25), 0)
    create_cone("Dish_Reflector", (0.28, 0.62, 2.58), 0.68, 0.05, 0.22, vertices=48, rot=dish_rot)
    # Feed horn subreflector tripod
    create_cylinder("Dish_FeedHorn", (0.36, 0.78, 2.74), 0.045, 0.025, 0.25, vertices=12, rot=dish_rot)

    # Nose ribbed collars
    for z_rib in [1.9, 2.1, 2.3]:
        create_torus(f"Nose_Rib_{z_rib}", (0, 0, z_rib), 0.46, 0.035, seg_major=36, seg_minor=10)

    # --- PRIMARY HABITAT CENTRIFUGE (Z = 0.35) ---
    cent_z = 0.35
    r_hab = 1.75
    # Main outer torus ring
    create_torus("Hab_MainTorus", (0, 0, cent_z), r_hab, 0.18, seg_major=72, seg_minor=20)
    # Inner rim brace ring
    create_torus("Hab_InnerBrace", (0, 0, cent_z), r_hab - 0.30, 0.045, seg_major=48, seg_minor=12)

    # 8 Radial Elevator Spokes / Transit Conduits
    for i in range(8):
        theta = i * (2.0 * math.pi / 8.0)
        sx = math.cos(theta) * (r_hab * 0.5)
        sy = math.sin(theta) * (r_hab * 0.5)
        # Rotated cylinder along spoke vector
        rot_z = theta + math.pi / 2.0
        create_cylinder(f"Hab_Spoke_{i}", (sx, sy, cent_z), 0.075, 0.075, r_hab - 0.36, vertices=16, rot=(math.radians(90), 0, rot_z))
        # Spoke hub fairing wedge
        fx = math.cos(theta) * 0.48
        fy = math.sin(theta) * 0.48
        create_cube(f"Spoke_Fairing_{i}", (fx, fy, cent_z), (0.14, 0.14, 0.28), rot=(0, 0, theta))

    # Radiator cooling fins / telemetry spikes along outer rim (36 fins around perimeter)
    for k in range(36):
        phi = k * (2.0 * math.pi / 36.0)
        rx = math.cos(phi) * (r_hab + 0.22)
        ry = math.sin(phi) * (r_hab + 0.22)
        # Thin blade fin extending outward
        create_cube(f"Rim_Fin_{k}", (rx, ry, cent_z), (0.025, 0.08, 0.26), rot=(0, 0, phi))

    # Observation blister pods (8 panoramic pods facing forward)
    for p in range(8):
        psi = (p + 0.5) * (2.0 * math.pi / 8.0)
        bx = math.cos(psi) * r_hab
        by = math.sin(psi) * r_hab
        create_cylinder(f"Obs_Blister_{p}", (bx, by, cent_z + 0.16), 0.11, 0.08, 0.12, vertices=16)

    # --- SECONDARY TECHNICAL CENTRIFUGE (Z = -1.25) ---
    sec_z = -1.25
    r_sec = 1.15
    create_torus("Sec_Torus", (0, 0, sec_z), r_sec, 0.10, seg_major=48, seg_minor=14)
    for j in range(4):
        omega = j * (math.pi / 2.0)
        hx = math.cos(omega) * (r_sec * 0.5)
        hy = math.sin(omega) * (r_sec * 0.5)
        create_cylinder(f"Sec_Spoke_{j}", (hx, hy, sec_z), 0.05, 0.05, r_sec - 0.24, vertices=12, rot=(math.radians(90), 0, omega + math.pi/2.0))

    # --- FUEL PODS & RADIATORS (Midship Z = -0.55) ---
    for q in range(4):
        beta = q * (math.pi / 2.0) + math.pi / 4.0
        tx = math.cos(beta) * 0.62
        ty = math.sin(beta) * 0.62
        create_cylinder(f"Fuel_Tank_{q}", (tx, ty, -0.52), 0.18, 0.18, 1.4, vertices=24)
        create_cone(f"Tank_Cap_Fwd_{q}", (tx, ty, 0.22), 0.18, 0.05, 0.16, vertices=16)
        create_cone(f"Tank_Cap_Aft_{q}", (tx, ty, -1.26), 0.18, 0.05, 0.16, vertices=16, rot=(math.radians(180), 0, 0))

    # Radiator wing panels extending on +/- X
    create_cube("Radiator_Wing_Right", (1.05, 0, -0.52), (0.85, 0.025, 1.25))
    create_cube("Radiator_Wing_Left", (-1.05, 0, -0.52), (0.85, 0.025, 1.25))

    # --- PROPULSION & AFT THRUSTERS (Z = -2.3 to -2.9) ---
    # Engine thrust structure bulkhead
    create_cone("Thrust_Bulkhead", (0, 0, -2.25), 0.44, 0.58, 0.7, vertices=32, rot=(math.radians(180), 0, 0))
    # 4 Fusion propulsion bell nozzles
    nozzle_offsets = [(0.26, 0.26), (-0.26, 0.26), (0.26, -0.26), (-0.26, -0.26)]
    for n_idx, (nx, ny) in enumerate(nozzle_offsets):
        create_cone(f"Engine_Bell_{n_idx}", (nx, ny, -2.75), 0.20, 0.09, 0.52, vertices=24, rot=(math.radians(180), 0, 0))
        # Inner throat glow plug
        create_cylinder(f"Engine_Throat_{n_idx}", (nx, ny, -2.60), 0.08, 0.08, 0.14, vertices=16)
    # Central ion vector gimbal
    create_cone("Engine_CenterIon", (0, 0, -2.82), 0.16, 0.07, 0.58, vertices=24, rot=(math.radians(180), 0, 0))

    # --- JOIN INTO SINGLE OPTIMIZED OBJECT ---
    bpy.ops.object.select_all(action='DESELECT')
    for obj in mesh_objs:
        obj.select_set(True)
    bpy.context.view_layer.objects.active = mesh_objs[0]
    bpy.ops.object.join()
    ark_starship = bpy.context.active_object
    ark_starship.name = "ark_ship"

    # Smooth shading
    bpy.ops.object.shade_smooth()

    # Generate UVs (Smart UV Project)
    bpy.ops.object.mode_set(mode='EDIT')
    bpy.ops.mesh.select_all(action='SELECT')
    bpy.ops.uv.smart_project(angle_limit=66.0, island_margin=0.015)
    bpy.ops.object.mode_set(mode='OBJECT')

    poly_count = len(ark_starship.data.polygons)
    print(f"[MESH] Interstellar Ark created with {poly_count} polygons.")

    # Create Material linking PBR textures
    mat = bpy.data.materials.new(name="ArkShipMaterial")
    mat.use_nodes = True
    nodes = mat.node_tree.nodes
    links = mat.node_tree.links
    nodes.clear()

    # Output node
    node_out = nodes.new(type='ShaderNodeOutputMaterial')
    node_out.location = (400, 0)
    # Principled BSDF
    node_bsdf = nodes.new(type='ShaderNodeBsdfPrincipled')
    node_bsdf.location = (0, 0)
    links.new(node_bsdf.outputs['BSDF'], node_out.inputs['Surface'])

    def add_image_texture(filepath, loc, is_color=True):
        if os.path.exists(filepath):
            node_tex = nodes.new(type='ShaderNodeTexImage')
            node_tex.location = loc
            img = bpy.data.images.load(filepath)
            node_tex.image = img
            if not is_color:
                node_tex.image.colorspace_settings.name = 'Non-Color'
            return node_tex
        return None

    # Load textures
    albedo_tex = add_image_texture(os.path.join(MODELS_DIR, "ark_ship_albedo.bmp"), (-400, 300), True)
    if albedo_tex:
        links.new(albedo_tex.outputs['Color'], node_bsdf.inputs['Base Color'])

    rough_tex = add_image_texture(os.path.join(MODELS_DIR, "ark_ship_rough.bmp"), (-400, 50), False)
    if rough_tex:
        links.new(rough_tex.outputs['Color'], node_bsdf.inputs['Roughness'])

    metal_tex = add_image_texture(os.path.join(MODELS_DIR, "ark_ship_metal.bmp"), (-400, -180), False)
    if metal_tex:
        links.new(metal_tex.outputs['Color'], node_bsdf.inputs['Metallic'])

    emissive_tex = add_image_texture(os.path.join(MODELS_DIR, "ark_ship_emissive.bmp"), (-400, -400), True)
    if emissive_tex:
        links.new(emissive_tex.outputs['Color'], node_bsdf.inputs['Emission Color'])
        node_bsdf.inputs['Emission Strength'].default_value = 2.8

    normal_tex = add_image_texture(os.path.join(MODELS_DIR, "ark_ship_normal.bmp"), (-600, -600), False)
    if normal_tex:
        node_norm_map = nodes.new(type='ShaderNodeNormalMap')
        node_norm_map.location = (-200, -600)
        links.new(normal_tex.outputs['Color'], node_norm_map.inputs['Color'])
        links.new(node_norm_map.outputs['Normal'], node_bsdf.inputs['Normal'])

    ark_starship.data.materials.append(mat)

    # Export Wavefront OBJ
    out_obj = os.path.join(MODELS_DIR, "ark_ship.obj")
    bpy.ops.wm.obj_export(
        filepath=out_obj,
        export_selected_objects=True,
        apply_modifiers=True,
        export_uv=True,
        export_normals=True,
        export_materials=True,
        forward_axis='Z',
        up_axis='Y'
    )
    print(f"[OBJ] Exported: {out_obj} ({os.path.getsize(out_obj)/1024:.1f} KB)")

    # Also duplicate/alias to voyager.obj for full in-engine upgrade option
    out_voyager = os.path.join(MODELS_DIR, "voyager.obj")
    try:
        import shutil
        shutil.copyfile(out_obj, out_voyager)
        print(f"[OBJ] Synchronized upgrade to: {out_voyager}")
    except Exception as e:
        print(f"[WARN] Could not copy to voyager.obj: {e}")

    # =========================================================================
    # 3. RENDER CYCLES SHOWCASE IMAGE
    # =========================================================================
    print("[RENDER] Rendering Cycles showcase frame...")
    scene = bpy.context.scene
    scene.render.engine = 'CYCLES'
    scene.cycles.device = 'CPU'
    scene.cycles.samples = 64
    scene.render.resolution_x = 1280
    scene.render.resolution_y = 720
    scene.render.film_transparent = False

    # Deep Space World Background
    world = bpy.data.worlds.new("DeepSpaceWorld")
    world.use_nodes = True
    wnodes = world.node_tree.nodes
    wbg = wnodes.get("Background")
    if wbg:
        wbg.inputs['Color'].default_value = (0.008, 0.012, 0.022, 1.0) # Deep stellar void
        wbg.inputs['Strength'].default_value = 0.5
    scene.world = world

    # Background Planet (Azure atmosphere crescent)
    bpy.ops.mesh.primitive_uv_sphere_add(segments=48, ring_count=24, radius=14.0, location=(-12.0, 22.0, -9.0))
    planet = bpy.context.active_object
    planet.name = "Background_Planet"
    p_mat = bpy.data.materials.new(name="PlanetMat")
    p_mat.use_nodes = True
    p_nodes = p_mat.node_tree.nodes
    p_bsdf = p_nodes.get("Principled BSDF")
    if p_bsdf:
        p_bsdf.inputs['Base Color'].default_value = (0.08, 0.35, 0.85, 1.0)
        p_bsdf.inputs['Roughness'].default_value = 0.85
    planet.data.materials.append(p_mat)

    # Key Stellar Light (Bright distant star)
    bpy.ops.object.light_add(type='SUN', location=(15.0, -12.0, 10.0))
    sun = bpy.context.active_object
    sun.data.energy = 6.2
    sun.data.color = (1.0, 0.98, 0.94)

    # Rim light from planet limb
    bpy.ops.object.light_add(type='POINT', location=(-8.0, 10.0, -4.0))
    rim = bpy.context.active_object
    rim.data.energy = 450.0
    rim.data.color = (0.15, 0.65, 1.0)

    # Camera framing: diagonal 3/4 perspective matching user reference
    bpy.ops.object.camera_add(location=(7.6, -6.6, 2.7))
    cam = bpy.context.active_object
    scene.camera = cam

    # Look target at habitat ring center
    target = Vector((0.1, 0.1, 0.2))
    direction = target - cam.location
    rot_quat = direction.to_track_quat('-Z', 'Y')
    euler = rot_quat.to_euler()
    # Tilt slightly around viewing axis for dynamic space slant
    euler.rotate_axis('Z', math.radians(24))
    cam.rotation_euler = euler
    cam.data.lens = 32.0 # Cinematic lens framing full vessel

    out_screen = os.path.join(SCREENS_DIR, "ark_ship_showcase.png")
    scene.render.filepath = out_screen
    bpy.ops.render.render(write_still=True)
    print(f"[RENDER] Saved showcase: {out_screen}")


if __name__ == "__main__":
    generate_ark_textures(size=1024)
    build_ark_mesh()
    print("[ALL DONE] Interstellar Ark generation pipeline completed successfully!")
