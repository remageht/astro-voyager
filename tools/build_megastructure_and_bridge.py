"""
build_megastructure_and_bridge.py
==================================
Procedural generation of:
1. res/models/station_megastructure.obj + 2K PBR BMPs (albedo, normal, rough, metal, emissive)
2. res/models/bridge_interior.obj + 2K PBR BMPs (albedo, normal, rough, metal, emissive)
3. Cycles showcase renders in docs/screens/

Run via Blender:
& "C:\Program Files\Blender Foundation\Blender 5.2\blender.exe" -b -P tools/build_megastructure_and_bridge.py
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
        src_row = (height - 1 - y) * width * 3
        dst_row = y * stride
        for x in range(width):
            src_idx = src_row + x * 3
            dst_idx = dst_row + x * 3
            # rgb_buffer is [R, G, B]
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
# 1. GENERATE PROCEDURAL PBR TEXTURES FOR MEGASTRUCTURE
# =========================================================================
def generate_megastructure_textures(size=1024):
    print(f"[TEXTURES] Generating Megastructure PBR textures ({size}x{size})...")
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

            # Default baseline
            r, g, b = 180, 190, 205      # Titanium alloy
            nx, ny, nz = 128, 128, 255   # Flat normal
            ro = 110                     # Semi-matte
            me = 210                     # High metallic
            er, eg, eb = 0, 0, 0         # No emission

            # Grid paneling effect across all quadrants
            grid_u = (u * 32.0) % 1.0
            grid_v = (v * 32.0) % 1.0
            is_panel_seam = (grid_u < 0.05 or grid_v < 0.05)
            if is_panel_seam:
                r, g, b = int(r * 0.5), int(g * 0.5), int(b * 0.5)
                nx = 110 if grid_u < 0.05 else 128
                ny = 110 if grid_v < 0.05 else 128
                ro = 180

            # Quadrant layout:
            # Q1 (u < 0.5, v >= 0.5): Spire & Core Hub
            if u < 0.5 and v >= 0.5:
                # Vertical cooling vanes and dark carbon composite band
                if ((u * 64.0) % 1.0) < 0.2:
                    r, g, b = 50, 58, 68
                    me = 240
                    ro = 90
                # Cyan status beacons and core plasma glow
                if 0.20 < u < 0.30 and 0.70 < v < 0.80:
                    er, eg, eb = 0, 220, 255 # Bright cyan
                    r, g, b = 200, 245, 255

            # Q2 (u >= 0.5, v >= 0.5): Habitation Rings
            elif u >= 0.5 and v >= 0.5:
                # Bands of lit observation windows (warm incandescent and lab cyan)
                win_strip = int(v * 128) % 4
                win_col = int(u * 128) % 3
                if win_strip == 1 and win_col != 0:
                    # Alternating warm golden and soft cyan windows
                    if (int(u * 64) % 2) == 0:
                        er, eg, eb = 255, 225, 150 # Warm golden
                        r, g, b = 255, 240, 180
                    else:
                        er, eg, eb = 60, 210, 255  # Cyan lab
                        r, g, b = 160, 230, 255
                    ro = 30
                    me = 40
                elif win_strip == 3:
                    # Hydroponic green bay glow
                    er, eg, eb = 40, 220, 90
                    r, g, b = 100, 255, 140
                    ro = 40
                    me = 30

            # Q3 (u < 0.5, v < 0.5): Trusses & Spokes
            elif u < 0.5 and v < 0.5:
                # High-contrast structural graphite and hazard stripes
                r, g, b = 80, 90, 105
                me = 220
                ro = 130
                # Yellow-black hazard warning strips
                diag = int((u + v) * 64) % 4
                if 0.15 < u < 0.35 and 0.15 < v < 0.35:
                    if diag < 2:
                        r, g, b = 230, 180, 20
                    else:
                        r, g, b = 25, 25, 30
                    ro = 160
                    me = 80

            # Q4 (u >= 0.5, v < 0.5): Solar Arrays & Docking Hangars
            else:
                # Deep blue silicon photovoltaics
                solar_grid_x = int(u * 64) % 2
                solar_grid_y = int(v * 64) % 2
                r, g, b = 18, 38, 85
                me = 190
                ro = 45
                if solar_grid_x == 0 or solar_grid_y == 0:
                    r, g, b = 180, 180, 200 # Silver busbar line
                    ro = 90
                # Docking approach neon guidance beacons
                if 0.70 < u < 0.80 and 0.15 < v < 0.25:
                    er, eg, eb = 255, 60, 40 # Red warning beacons
                    r, g, b = 255, 120, 100

            # Fill buffers
            albedo[idx] = max(0, min(255, r))
            albedo[idx + 1] = max(0, min(255, g))
            albedo[idx + 2] = max(0, min(255, b))

            normal[idx] = max(0, min(255, nx))
            normal[idx + 1] = max(0, min(255, ny))
            normal[idx + 2] = max(0, min(255, nz))

            rough[idx] = ro; rough[idx + 1] = ro; rough[idx + 2] = ro
            metal[idx] = me; metal[idx + 1] = me; metal[idx + 2] = me

            emissive[idx] = er
            emissive[idx + 1] = eg
            emissive[idx + 2] = eb

    prefix = os.path.join(MODELS_DIR, "station_megastructure")
    write_bmp24(f"{prefix}_albedo.bmp", size, size, albedo)
    write_bmp24(f"{prefix}_normal.bmp", size, size, normal)
    write_bmp24(f"{prefix}_rough.bmp", size, size, rough)
    write_bmp24(f"{prefix}_metal.bmp", size, size, metal)
    write_bmp24(f"{prefix}_emissive.bmp", size, size, emissive)


# =========================================================================
# 2. GENERATE PROCEDURAL PBR TEXTURES FOR BRIDGE INTERIOR
# =========================================================================
def generate_bridge_interior_textures(size=1024):
    print(f"[TEXTURES] Generating Bridge Interior PBR textures ({size}x{size})...")
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

            r, g, b = 38, 44, 52          # Dark cockpit composite
            nx, ny, nz = 128, 128, 255   # Flat normal
            ro = 160                     # Matte ergonomic plastic/composite
            me = 60                      # Low metal (tactile panels)
            er, eg, eb = 0, 0, 0         # No emission

            # Q1 (u < 0.5, v >= 0.5): Floor & Arch Framing
            if u < 0.5 and v >= 0.5:
                # Diamond treadplate pattern for floor
                tread = (int(u * 128) + int(v * 128)) % 2
                if tread == 0:
                    r, g, b = 25, 29, 36
                    ro = 190
                    me = 180
                else:
                    r, g, b = 48, 54, 64
                    ro = 120
                    me = 210
                # Recessed LED light guide strips (soft cyan)
                if abs(u - 0.25) < 0.015:
                    er, eg, eb = 0, 180, 255
                    r, g, b = 180, 235, 255
                    ro = 20

            # Q2 (u >= 0.5, v >= 0.5): Consoles & Holographic MFD Displays
            elif u >= 0.5 and v >= 0.5:
                # Glass screen border
                r, g, b = 12, 16, 22
                ro = 25
                me = 100
                # Glowing instrument graphics (radar, pitch ladder, telemetry)
                screen_u = (u - 0.5) / 0.5
                screen_v = (v - 0.5) / 0.5
                # Screen bezel frame
                if 0.10 < screen_u < 0.90 and 0.10 < screen_v < 0.90:
                    # Screen interior
                    r, g, b = 8, 22, 34
                    # Holographic HUD lines (cyan and emerald vectors)
                    is_crosshair = (abs(screen_u - 0.5) < 0.008 or abs(screen_v - 0.5) < 0.008)
                    dist_center = math.hypot(screen_u - 0.5, screen_v - 0.5)
                    is_circle = abs(dist_center - 0.3) < 0.012
                    is_waveform = abs(math.sin(screen_u * 30.0) * 0.15 + 0.3 - screen_v) < 0.012

                    if is_crosshair or is_circle:
                        er, eg, eb = 0, 235, 255 # Bright cyan
                        r, g, b = 100, 245, 255
                    elif is_waveform:
                        er, eg, eb = 16, 230, 140 # Emerald telemetry
                        r, g, b = 120, 255, 180
                    elif screen_v < 0.25 and (int(screen_u * 40) % 2 == 0):
                        er, eg, eb = 245, 160, 20 # Amber warning LEDs
                        r, g, b = 255, 200, 80

            # Q3 (u < 0.5, v < 0.5): Crew Seats & Ergonomic Materials
            elif u < 0.5 and v < 0.5:
                # Perforated pilot leather / breathable mesh
                perf = (int(u * 128) % 2) & (int(v * 128) % 2)
                r, g, b = (28, 32, 40) if perf else (42, 48, 58)
                ro = 190
                me = 10
                # Orange accent stitching
                if abs(u - 0.25) < 0.008:
                    r, g, b = 230, 110, 25
                    ro = 140

            # Q4 (u >= 0.5, v < 0.5): Holo-Table & Bulkhead Blast Door
            else:
                # Heavy reinforced steel door & hazard chevrons
                r, g, b = 70, 78, 88
                me = 210
                ro = 110
                # Tactical Holo-Table emitter ring (pulsing cyan energy)
                hu = (u - 0.5) / 0.5
                hv = v / 0.5
                h_dist = math.hypot(hu - 0.5, hv - 0.5)
                if abs(h_dist - 0.35) < 0.035:
                    er, eg, eb = 0, 210, 255
                    r, g, b = 150, 235, 255
                    ro = 15
                    me = 140
                elif h_dist < 0.12:
                    er, eg, eb = 100, 250, 255
                    r, g, b = 200, 255, 255

            albedo[idx] = max(0, min(255, r))
            albedo[idx + 1] = max(0, min(255, g))
            albedo[idx + 2] = max(0, min(255, b))

            normal[idx] = max(0, min(255, nx))
            normal[idx + 1] = max(0, min(255, ny))
            normal[idx + 2] = max(0, min(255, nz))

            rough[idx] = ro; rough[idx + 1] = ro; rough[idx + 2] = ro
            metal[idx] = me; metal[idx + 1] = me; metal[idx + 2] = me

            emissive[idx] = er
            emissive[idx + 1] = eg
            emissive[idx + 2] = eb

    prefix = os.path.join(MODELS_DIR, "bridge_interior")
    write_bmp24(f"{prefix}_albedo.bmp", size, size, albedo)
    write_bmp24(f"{prefix}_normal.bmp", size, size, normal)
    write_bmp24(f"{prefix}_rough.bmp", size, size, rough)
    write_bmp24(f"{prefix}_metal.bmp", size, size, metal)
    write_bmp24(f"{prefix}_emissive.bmp", size, size, emissive)


# =========================================================================
# 3. BUILD 3D MESH: ORBITAL MEGASTRUCTURE
# =========================================================================
def build_megastructure_mesh():
    print("[MESH] Building Orbital Megastructure 3D model...")
    bpy.ops.wm.read_factory_settings(use_empty=True)
    parts = []

    # 1. Central Spire & Core Hub
    # Spire needle (top)
    bpy.ops.mesh.primitive_cone_add(vertices=24, radius1=0.8, radius2=0.08, depth=8.0, location=(0, 0, 6.0))
    spire_top = bpy.context.active_object
    spire_top.name = "SpireNeedle"
    parts.append(spire_top)

    # Core main cylinder
    bpy.ops.mesh.primitive_cylinder_add(vertices=32, radius=2.4, depth=10.0, location=(0, 0, 0))
    core_cyl = bpy.context.active_object
    core_cyl.name = "CoreHub"
    parts.append(core_cyl)

    # Communication dish arrays on spire
    for a in range(4):
        angle = a * (math.pi / 2.0)
        px = math.cos(angle) * 1.8
        py = math.sin(angle) * 1.8
        bpy.ops.mesh.primitive_cylinder_add(vertices=16, radius=0.6, depth=0.15, location=(px, py, 7.5),
                                            rotation=(math.pi/3, 0, angle))
        dish = bpy.context.active_object
        parts.append(dish)

    # 2. Primary Habitation Torus Ring (Stanford Torus scale)
    # Outer ring radius = 12.0, tube radius = 0.95
    bpy.ops.mesh.primitive_torus_add(major_radius=12.0, minor_radius=0.95,
                                     major_segments=64, minor_segments=20,
                                     location=(0, 0, 0))
    outer_ring = bpy.context.active_object
    outer_ring.name = "OuterHabitationRing"
    parts.append(outer_ring)

    # Secondary counter-rotating Habitation Ring
    bpy.ops.mesh.primitive_torus_add(major_radius=7.0, minor_radius=0.65,
                                     major_segments=48, minor_segments=16,
                                     location=(0, 0, 1.8))
    inner_ring = bpy.context.active_object
    inner_ring.name = "InnerHabitationRing"
    parts.append(inner_ring)

    # 3. Six Major Spoke Struts connecting Core to Outer Ring
    for i in range(6):
        ang = i * (math.pi / 3.0)
        sx = math.cos(ang) * 6.0
        sy = math.sin(ang) * 6.0
        # Cylinder strut
        bpy.ops.mesh.primitive_cylinder_add(vertices=12, radius=0.35, depth=11.6,
                                            location=(sx, sy, 0),
                                            rotation=(math.pi/2, 0, ang + math.pi/2))
        strut = bpy.context.active_object
        parts.append(strut)

        # High-speed transit pod along spoke
        pod_x = math.cos(ang) * 4.2
        pod_y = math.sin(ang) * 4.2
        bpy.ops.mesh.primitive_cube_add(size=0.8, location=(pod_x, pod_y, 0))
        pod = bpy.context.active_object
        pod.scale = (1.2, 0.7, 0.7)
        bpy.ops.object.transform_apply(scale=True)
        parts.append(pod)

    # 4. Industrial Lower Docking Quadrant
    bpy.ops.mesh.primitive_cylinder_add(vertices=32, radius=3.2, depth=2.5, location=(0, 0, -6.0))
    dock_hub = bpy.context.active_object
    parts.append(dock_hub)

    # Four massive capital ship docking cradles / slipways
    for d in range(4):
        dang = d * (math.pi / 2.0)
        dx = math.cos(dang) * 5.2
        dy = math.sin(dang) * 5.2
        bpy.ops.mesh.primitive_cube_add(size=1.0, location=(dx, dy, -6.0))
        cradle = bpy.context.active_object
        cradle.scale = (2.8, 0.8, 1.6)
        cradle.rotation_euler = (0, 0, dang)
        bpy.ops.object.transform_apply(scale=True, rotation=True)
        parts.append(cradle)

    # 5. Massive Radiator Fins and Solar Arrays
    for f in range(3):
        fang = f * (2.0 * math.pi / 3.0) + (math.pi / 6.0)
        fx = math.cos(fang) * 4.5
        fy = math.sin(fang) * 4.5
        bpy.ops.mesh.primitive_cube_add(size=1.0, location=(fx, fy, -9.5))
        fin = bpy.context.active_object
        fin.scale = (0.08, 2.2, 4.5)
        fin.rotation_euler = (0, 0, fang)
        bpy.ops.object.transform_apply(scale=True, rotation=True)
        parts.append(fin)

    # Join all objects into a single mesh
    bpy.ops.object.select_all(action='DESELECT')
    for p in parts:
        p.select_set(True)
    bpy.context.view_layer.objects.active = parts[0]
    bpy.ops.object.join()
    megastructure = bpy.context.active_object
    megastructure.name = "StationMegastructure"

    # Smart UV Project to unwrap cleanly
    bpy.ops.object.mode_set(mode='EDIT')
    bpy.ops.mesh.select_all(action='SELECT')
    bpy.ops.uv.smart_project(angle_limit=66.0, island_margin=0.02)
    bpy.ops.mesh.normals_make_consistent(inside=False)
    bpy.ops.object.mode_set(mode='OBJECT')

    # Recenter and normalize scale to match astro-voyager space scale (~20.0 diameter)
    bpy.ops.object.origin_set(type='ORIGIN_GEOMETRY', center='BOUNDS')
    megastructure.location = (0, 0, 0)
    
    # Export OBJ
    out_obj = os.path.join(MODELS_DIR, "station_megastructure.obj")
    bpy.ops.wm.obj_export(filepath=out_obj, export_selected_objects=True)
    print(f"[EXPORT] Megastructure saved: {out_obj}")
    return megastructure


# =========================================================================
# 4. BUILD 3D MESH: SHIP BRIDGE INTERIOR
# =========================================================================
def build_bridge_interior_mesh():
    print("[MESH] Building Ship Bridge Interior 3D model...")
    bpy.ops.wm.read_factory_settings(use_empty=True)
    parts = []

    # 1. Deck Floor & Elevated Platform
    bpy.ops.mesh.primitive_cube_add(size=1.0, location=(0, 0, -0.1))
    floor = bpy.context.active_object
    floor.scale = (6.0, 7.5, 0.2)
    bpy.ops.object.transform_apply(scale=True)
    parts.append(floor)

    # Raised command dais (where captain's chair sits)
    bpy.ops.mesh.primitive_cylinder_add(vertices=24, radius=1.6, depth=0.15, location=(0, 0.8, 0.05))
    dais = bpy.context.active_object
    parts.append(dais)

    # 2. Forward Panoramic Canopy Arches (Reinforced aerospace glass canopy frame)
    for side in (-1, 1):
        # Angled front mullions
        bpy.ops.mesh.primitive_cube_add(size=1.0, location=(side * 2.8, -1.8, 1.2))
        mullion = bpy.context.active_object
        mullion.scale = (0.25, 0.35, 2.5)
        mullion.rotation_euler = (math.radians(-25), side * math.radians(15), 0)
        bpy.ops.object.transform_apply(scale=True, rotation=True)
        parts.append(mullion)

    # Top canopy brow arch
    bpy.ops.mesh.primitive_cube_add(size=1.0, location=(0, -2.5, 2.3))
    brow = bpy.context.active_object
    brow.scale = (5.6, 0.4, 0.3)
    bpy.ops.object.transform_apply(scale=True)
    parts.append(brow)

    # Ceiling center structural spine with cable trays
    bpy.ops.mesh.primitive_cube_add(size=1.0, location=(0, 0.5, 2.5))
    spine = bpy.context.active_object
    spine.scale = (1.2, 5.0, 0.3)
    bpy.ops.object.transform_apply(scale=True)
    parts.append(spine)

    # 3. Captain's Command Chair
    # Base pedestal
    bpy.ops.mesh.primitive_cylinder_add(vertices=16, radius=0.45, depth=0.45, location=(0, 0.8, 0.35))
    parts.append(bpy.context.active_object)
    # Seat cushion
    bpy.ops.mesh.primitive_cube_add(size=1.0, location=(0, 0.8, 0.65))
    seat = bpy.context.active_object
    seat.scale = (0.85, 0.8, 0.18)
    bpy.ops.object.transform_apply(scale=True)
    parts.append(seat)
    # Backrest (angled back slightly)
    bpy.ops.mesh.primitive_cube_add(size=1.0, location=(0, 1.15, 1.25))
    back = bpy.context.active_object
    back.scale = (0.75, 0.16, 1.0)
    back.rotation_euler = (math.radians(10), 0, 0)
    bpy.ops.object.transform_apply(scale=True, rotation=True)
    parts.append(back)
    # Armrests with integrated flight controls
    for s in (-1, 1):
        bpy.ops.mesh.primitive_cube_add(size=1.0, location=(s * 0.5, 0.8, 0.85))
        arm = bpy.context.active_object
        arm.scale = (0.16, 0.65, 0.12)
        bpy.ops.object.transform_apply(scale=True)
        parts.append(arm)
        # Joystick knob
        bpy.ops.mesh.primitive_cylinder_add(vertices=12, radius=0.05, depth=0.14, location=(s * 0.5, 0.55, 0.95))
        parts.append(bpy.context.active_object)

    # 4. Pilot & Navigator Stations (Left and Right front consoles)
    for s in (-1, 1):
        pos_x = s * 1.7
        pos_y = -1.6
        # Curved console desk
        bpy.ops.mesh.primitive_cube_add(size=1.0, location=(pos_x, pos_y, 0.7))
        desk = bpy.context.active_object
        desk.scale = (1.5, 0.8, 0.7)
        desk.rotation_euler = (0, 0, -s * math.radians(20))
        bpy.ops.object.transform_apply(scale=True, rotation=True)
        parts.append(desk)

        # Angled MFD screens
        bpy.ops.mesh.primitive_cube_add(size=1.0, location=(pos_x, pos_y - 0.25, 1.15))
        screen = bpy.context.active_object
        screen.scale = (1.2, 0.08, 0.55)
        screen.rotation_euler = (math.radians(-30), 0, -s * math.radians(20))
        bpy.ops.object.transform_apply(scale=True, rotation=True)
        parts.append(screen)

        # Operator chairs
        bpy.ops.mesh.primitive_cylinder_add(vertices=14, radius=0.35, depth=0.35, location=(pos_x, pos_y + 0.9, 0.3))
        parts.append(bpy.context.active_object)
        bpy.ops.mesh.primitive_cube_add(size=1.0, location=(pos_x, pos_y + 0.9, 0.55))
        c_seat = bpy.context.active_object
        c_seat.scale = (0.65, 0.65, 0.14)
        bpy.ops.object.transform_apply(scale=True)
        parts.append(c_seat)
        bpy.ops.mesh.primitive_cube_add(size=1.0, location=(pos_x, pos_y + 1.2, 0.95))
        c_back = bpy.context.active_object
        c_back.scale = (0.6, 0.12, 0.7)
        bpy.ops.object.transform_apply(scale=True)
        parts.append(c_back)

    # 5. Central Tactical Holographic Table (Holo-Table)
    bpy.ops.mesh.primitive_cylinder_add(vertices=24, radius=0.9, depth=0.75, location=(0, -0.6, 0.45))
    holo_base = bpy.context.active_object
    parts.append(holo_base)
    # Beveled holographic projection emitter ring
    bpy.ops.mesh.primitive_torus_add(major_radius=0.75, minor_radius=0.08, major_segments=32, minor_segments=12,
                                     location=(0, -0.6, 0.85))
    parts.append(bpy.context.active_object)

    # 6. Aft Bulkhead Wall & Airlock Blast Door
    bpy.ops.mesh.primitive_cube_add(size=1.0, location=(0, 3.4, 1.4))
    bulkhead = bpy.context.active_object
    bulkhead.scale = (5.8, 0.35, 2.6)
    bpy.ops.object.transform_apply(scale=True)
    parts.append(bulkhead)

    # Airlock door frame & panels
    bpy.ops.mesh.primitive_cube_add(size=1.0, location=(0, 3.3, 1.1))
    airlock = bpy.context.active_object
    airlock.scale = (1.6, 0.15, 2.0)
    bpy.ops.object.transform_apply(scale=True)
    parts.append(airlock)

    # Join all bridge components
    bpy.ops.object.select_all(action='DESELECT')
    for p in parts:
        p.select_set(True)
    bpy.context.view_layer.objects.active = parts[0]
    bpy.ops.object.join()
    bridge = bpy.context.active_object
    bridge.name = "BridgeInterior"

    # Smart UV Project
    bpy.ops.object.mode_set(mode='EDIT')
    bpy.ops.mesh.select_all(action='SELECT')
    bpy.ops.uv.smart_project(angle_limit=66.0, island_margin=0.02)
    bpy.ops.mesh.normals_make_consistent(inside=False)
    bpy.ops.object.mode_set(mode='OBJECT')

    # Center origin
    bpy.ops.object.origin_set(type='ORIGIN_GEOMETRY', center='BOUNDS')
    bridge.location = (0, 0, 0)

    # Export OBJ
    out_obj = os.path.join(MODELS_DIR, "bridge_interior.obj")
    bpy.ops.wm.obj_export(filepath=out_obj, export_selected_objects=True)
    print(f"[EXPORT] Bridge Interior saved: {out_obj}")
    return bridge


# =========================================================================
# 5. CYCLES SHOWCASE RENDERS
# =========================================================================
def setup_pbr_material(obj, name, prefix):
    mat = bpy.data.materials.new(name=name)
    mat.use_nodes = True
    nodes = mat.node_tree.nodes
    links = mat.node_tree.links
    bsdf = nodes.get("Principled BSDF")

    def add_tex(filename, is_data=False):
        tex_node = nodes.new("ShaderNodeTexImage")
        if os.path.exists(filename):
            tex_node.image = bpy.data.images.load(filename)
            if is_data:
                tex_node.image.colorspace_settings.name = "Non-Color"
        return tex_node

    tex_albedo = add_tex(f"{prefix}_albedo.bmp")
    links.new(tex_albedo.outputs["Color"], bsdf.inputs["Base Color"])

    tex_rough = add_tex(f"{prefix}_rough.bmp", is_data=True)
    links.new(tex_rough.outputs["Color"], bsdf.inputs["Roughness"])

    tex_metal = add_tex(f"{prefix}_metal.bmp", is_data=True)
    links.new(tex_metal.outputs["Color"], bsdf.inputs["Metallic"])

    tex_emis = add_tex(f"{prefix}_emissive.bmp")
    links.new(tex_emis.outputs["Color"], bsdf.inputs["Emission Color"])
    bsdf.inputs["Emission Strength"].default_value = 3.5

    tex_norm = add_tex(f"{prefix}_normal.bmp", is_data=True)
    norm_map = nodes.new("ShaderNodeNormalMap")
    norm_map.inputs["Strength"].default_value = 1.2
    links.new(tex_norm.outputs["Color"], norm_map.inputs["Color"])
    links.new(norm_map.outputs["Normal"], bsdf.inputs["Normal"])

    obj.data.materials.clear()
    obj.data.materials.append(mat)


def render_showcases():
    print("[RENDER] Rendering Cycles showcase frames with PBR materials...")

    # 1. Megastructure Showcase
    mega_obj = build_megastructure_mesh()
    setup_pbr_material(mega_obj, "MegastructurePBR", os.path.join(MODELS_DIR, "station_megastructure"))

    scene = bpy.context.scene
    scene.render.engine = "CYCLES"
    scene.cycles.device = "CPU"
    scene.cycles.samples = 24
    scene.cycles.use_denoising = True
    scene.render.resolution_x = 1280
    scene.render.resolution_y = 720
    scene.render.image_settings.file_format = "PNG"

    # World lighting
    scene.world = bpy.data.worlds.new("CosmicVoid")
    bg = scene.world.node_tree.nodes.get("Background")
    if bg:
        bg.inputs["Color"].default_value = (0.005, 0.008, 0.015, 1)
        bg.inputs["Strength"].default_value = 0.55

    # 3-point sci-fi studio lighting
    lights = [
        ("Key", (18, -22, 14), 10500, (0.95, 0.98, 1.0), 10),
        ("CyanRim", (-20, 18, 10), 8500, (0.1, 0.75, 1.0), 12),
        ("WarmBounce", (0, -12, -14), 4500, (1.0, 0.55, 0.15), 15),
    ]
    for name, pos, energy, color, sz in lights:
        data = bpy.data.lights.new(name, "AREA")
        data.energy = energy
        data.color = color
        data.size = sz
        obj = bpy.data.objects.new(name, data)
        obj.location = pos
        obj.rotation_euler = (-Vector(pos)).to_track_quat("-Z", "Y").to_euler()
        scene.collection.objects.link(obj)

    cam_data = bpy.data.cameras.new("MegaCam")
    cam = bpy.data.objects.new("MegaCam", cam_data)
    cam.location = (20.0, -24.0, 14.0)
    cam.rotation_euler = (-Vector(cam.location)).to_track_quat("-Z", "Y").to_euler()
    scene.collection.objects.link(cam)
    scene.camera = cam

    out_mega = os.path.join(SCREENS_DIR, "megastructure_showcase.png")
    scene.render.filepath = out_mega
    bpy.ops.render.render(write_still=True)
    print(f"[RENDER] Megastructure showcase written: {out_mega}")

    # 2. Bridge Interior Showcase
    bridge_obj = build_bridge_interior_mesh()
    setup_pbr_material(bridge_obj, "BridgePBR", os.path.join(MODELS_DIR, "bridge_interior"))

    scene = bpy.context.scene
    scene.render.engine = "CYCLES"
    scene.cycles.samples = 24
    scene.cycles.use_denoising = True
    scene.render.resolution_x = 1280
    scene.render.resolution_y = 720
    scene.render.image_settings.file_format = "PNG"

    scene.world = bpy.data.worlds.new("BridgeAmbience")
    bg = scene.world.node_tree.nodes.get("Background")
    if bg:
        bg.inputs["Color"].default_value = (0.01, 0.015, 0.022, 1)

    int_lights = [
        ("RoofMain", (0, 0, 1.0), 450, (0.75, 0.88, 1.0)),
        ("HoloGlow", (0, -1.1, 0.2), 350, (0.0, 0.95, 1.0)),
        ("FrontWarm", (0, -2.2, 0.0), 220, (1.0, 0.75, 0.4)),
    ]
    for name, pos, energy, color in int_lights:
        data = bpy.data.lights.new(name, "POINT")
        data.energy = energy
        data.color = color
        obj = bpy.data.objects.new(name, data)
        obj.location = pos
        scene.collection.objects.link(obj)

    bridge_cam_data = bpy.data.cameras.new("BridgeCam")
    bridge_cam_data.lens = 22.0 # Wide angle sci-fi FOV
    bridge_cam = bpy.data.objects.new("BridgeCam", bridge_cam_data)
    # Viewing from behind-right showing captain chair, holo-table, consoles, and front canopy
    bridge_cam.location = (2.2, 3.2, 1.6)
    target = Vector((-0.3, -1.2, -0.3))
    dir_vec = target - Vector(bridge_cam.location)
    bridge_cam.rotation_euler = dir_vec.to_track_quat("-Z", "Y").to_euler()
    scene.collection.objects.link(bridge_cam)
    scene.camera = bridge_cam

    out_bridge = os.path.join(SCREENS_DIR, "bridge_interior_showcase.png")
    scene.render.filepath = out_bridge
    bpy.ops.render.render(write_still=True)
    print(f"[RENDER] Bridge Interior showcase written: {out_bridge}")


def main():
    print("=== ASTRO-VOYAGER: BUILD MEGASTRUCTURE & BRIDGE ASSETS ===")
    generate_megastructure_textures(1024)
    generate_bridge_interior_textures(1024)
    render_showcases()
    print("=== ASSET GENERATION COMPLETED SUCCESSFULLY ===")

if __name__ == "__main__":
    main()
