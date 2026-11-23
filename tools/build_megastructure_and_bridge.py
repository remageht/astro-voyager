r"""
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
import numpy as np
import bpy
import bmesh
from mathutils import Vector, Matrix

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
MODELS_DIR = os.path.join(ROOT, "res", "models")
SCREENS_DIR = os.path.join(ROOT, "docs", "screens")
os.makedirs(MODELS_DIR, exist_ok=True)
os.makedirs(SCREENS_DIR, exist_ok=True)

GEN_SCRIPTS = os.path.join(os.path.dirname(ROOT), "astro-voyager-generators", "scripts")
if os.path.isdir(GEN_SCRIPTS):
    sys.path.insert(0, GEN_SCRIPTS)

try:
    import scifi_detailing_toolkit as sdt
    HAS_SDT = True
except ImportError:
    HAS_SDT = False

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
def generate_megastructure_textures(size=2048):
    print(f"[TEXTURES] Generating Megastructure PBR textures ({size}x{size}) with NumPy...")
    half = size // 2

    albedo = np.full((size, size, 3), 195, dtype=np.uint8)
    normal = np.full((size, size, 3), [128, 128, 255], dtype=np.uint8)
    rough = np.full((size, size, 3), 110, dtype=np.uint8)
    metal = np.full((size, size, 3), 200, dtype=np.uint8)
    emissive = np.zeros((size, size, 3), dtype=np.uint8)

    # Global subtle panel seams (every 64 px)
    grid_mask_x = (np.arange(size) % 64) < 2
    grid_mask_y = (np.arange(size) % 64) < 2
    grid_2d = grid_mask_x[None, :] | grid_mask_y[:, None]
    albedo[grid_2d] = (albedo[grid_2d] * 0.72).astype(np.uint8)
    normal[grid_2d, 0] = 118
    normal[grid_2d, 1] = 118
    rough[grid_2d] = 160

    # -------------------------------------------------------------------------
    # Q1 (top-left: x < half, y >= half): Core Hub, Spire & Antennas
    # -------------------------------------------------------------------------
    q1_x = slice(0, half)
    q1_y = slice(half, size)
    albedo[q1_y, q1_x] = [170, 178, 188]
    metal[q1_y, q1_x] = 220
    rough[q1_y, q1_x] = 95

    # Vertical cooling vanes on reactor housing
    vane_mask = (np.arange(half) % 32) < 6
    albedo[half:size, 0:half][:, vane_mask] = [45, 52, 62]
    metal[half:size, 0:half][:, vane_mask] = 240
    rough[half:size, 0:half][:, vane_mask] = 80

    # Cyan reactor plasma containment band
    rx1, rx2 = int(half * 0.35), int(half * 0.65)
    ry1, ry2 = half + int(half * 0.20), half + int(half * 0.32)
    albedo[ry1:ry2, rx1:rx2] = [180, 240, 255]
    emissive[ry1:ry2, rx1:rx2] = [0, 220, 255]
    rough[ry1:ry2, rx1:rx2] = 20
    metal[ry1:ry2, rx1:rx2] = 30

    # -------------------------------------------------------------------------
    # Q2 (top-right: x >= half, y >= half): Habitation Rings
    # Tubular unwrap: u_norm in [0, 1] along circumference, v_norm in [0, 1] along tube
    # -------------------------------------------------------------------------
    # Base hull: Clean light titanium aerospace alloy
    albedo[half:size, half:size] = [218, 224, 232]
    metal[half:size, half:size] = 205
    rough[half:size, half:size] = 100

    # Repeating modular bays along circumference (32 bays)
    bay_x = (np.arange(half) % (half // 32)) < 3
    albedo[half:size, half:size][:, bay_x] = [160, 170, 182]
    normal[half:size, half:size][:, bay_x, 0] = 110
    rough[half:size, half:size][:, bay_x] = 150

    # 1. Outer Whipple Shield Armor Belt at outer equator (v_norm = 0.00..0.08 and 0.92..1.00)
    outer_1 = half
    outer_2 = half + int(half * 0.08)
    outer_3 = half + int(half * 0.92)
    outer_4 = size
    albedo[outer_1:outer_2, half:size] = [118, 128, 140]
    metal[outer_1:outer_2, half:size] = 230
    rough[outer_1:outer_2, half:size] = 85
    albedo[outer_3:outer_4, half:size] = [118, 128, 140]
    metal[outer_3:outer_4, half:size] = 230
    rough[outer_3:outer_4, half:size] = 85

    # Strobe beacons along outer Whipple belt
    strobe_x = (np.arange(half) % (half // 16)) == 0
    albedo[outer_1:outer_2, half:size][:, strobe_x] = [255, 255, 255]
    emissive[outer_1:outer_2, half:size][:, strobe_x] = [255, 255, 255]
    albedo[outer_3:outer_4, half:size][:, strobe_x] = [255, 255, 255]
    emissive[outer_3:outer_4, half:size][:, strobe_x] = [255, 255, 255]

    # 2. Dorsal Observation Promenade (Windows at v_norm = 0.20..0.30, around phi = pi/2)
    w_top_1 = half + int(half * 0.20)
    w_top_2 = half + int(half * 0.30)
    win_cell_dorsal = (np.arange(half) % 16) >= 4
    for wy in range(w_top_1, w_top_2):
        # Warm golden cabin glow
        albedo[wy, half:size][win_cell_dorsal] = [255, 238, 180]
        emissive[wy, half:size][win_cell_dorsal] = [255, 225, 140]
        rough[wy, half:size][win_cell_dorsal] = 20
        metal[wy, half:size][win_cell_dorsal] = 20

    # 3. Inner Service & Docking Collar (at v_norm = 0.46..0.54, facing core hub)
    inner_1 = half + int(half * 0.46)
    inner_2 = half + int(half * 0.54)
    albedo[inner_1:inner_2, half:size] = [105, 112, 122]
    metal[inner_1:inner_2, half:size] = 220
    rough[inner_1:inner_2, half:size] = 110

    # 4. Ventral Observation & Science Labs Promenade (Windows at v_norm = 0.70..0.80, around phi = 3pi/2)
    w_bot_1 = half + int(half * 0.70)
    w_bot_2 = half + int(half * 0.80)
    win_cell_ventral = (np.arange(half) % 16) >= 4
    for wy in range(w_bot_1, w_bot_2):
        # Cool cyan research laboratory glow
        albedo[wy, half:size][win_cell_ventral] = [190, 240, 255]
        emissive[wy, half:size][win_cell_ventral] = [100, 220, 255]
        rough[wy, half:size][win_cell_ventral] = 20
        metal[wy, half:size][win_cell_ventral] = 20

    # -------------------------------------------------------------------------
    # Q3 (bottom-left: x < half, y < half): Trusses, Spokes & APAS Docking Ports
    # -------------------------------------------------------------------------
    albedo[0:half, 0:half] = [82, 88, 98]
    metal[0:half, 0:half] = 190
    rough[0:half, 0:half] = 130

    # Diagonal hazard chevron strips for docking slipway guides
    diag_grid = ((np.arange(half)[:, None] + np.arange(half)[None, :]) // 16) % 2
    haz_mask = (diag_grid == 0)
    haz_y1, haz_y2 = int(half * 0.15), int(half * 0.35)
    haz_x1, haz_x2 = int(half * 0.15), int(half * 0.35)
    albedo[haz_y1:haz_y2, haz_x1:haz_x2][haz_mask[haz_y1:haz_y2, haz_x1:haz_x2]] = [240, 195, 25]
    albedo[haz_y1:haz_y2, haz_x1:haz_x2][~haz_mask[haz_y1:haz_y2, haz_x1:haz_x2]] = [28, 30, 36]
    rough[haz_y1:haz_y2, haz_x1:haz_x2] = 150
    metal[haz_y1:haz_y2, haz_x1:haz_x2] = 70

    # -------------------------------------------------------------------------
    # Q4 (bottom-right: x >= half, y < half): Solar Arrays & Corrugated Radiators
    # -------------------------------------------------------------------------
    # Upper half of Q4: Solar Photovoltaics (navy silicon)
    sol_y1, sol_y2 = half // 2, half
    sol_x1, sol_x2 = half, size
    albedo[sol_y1:sol_y2, sol_x1:sol_x2] = [18, 35, 80]
    metal[sol_y1:sol_y2, sol_x1:sol_x2] = 175
    rough[sol_y1:sol_y2, sol_x1:sol_x2] = 40
    # Silver busbars
    bus_x = (np.arange(half) % 32) < 2
    bus_y = (np.arange(half // 2) % 16) < 2
    bus_2d = bus_x[None, :] | bus_y[:, None]
    albedo[sol_y1:sol_y2, sol_x1:sol_x2][bus_2d] = [210, 218, 230]
    metal[sol_y1:sol_y2, sol_x1:sol_x2][bus_2d] = 230
    rough[sol_y1:sol_y2, sol_x1:sol_x2][bus_2d] = 80

    # Lower half of Q4: Corrugated Thermal Heat Radiators (reflective silver/white)
    rad_y1, rad_y2 = 0, half // 2
    albedo[rad_y1:rad_y2, sol_x1:sol_x2] = [238, 242, 248]
    metal[rad_y1:rad_y2, sol_x1:sol_x2] = 240
    rough[rad_y1:rad_y2, sol_x1:sol_x2] = 55
    # Corrugated vertical normal grooves
    vane_phase = np.sin(np.arange(half) * (2.0 * np.pi / 16.0))
    vane_nx = np.clip(128 + vane_phase * 65, 0, 255).astype(np.uint8)
    normal[rad_y1:rad_y2, sol_x1:sol_x2, 0] = vane_nx[None, :]

    # Save all 5 BMPs
    prefix = os.path.join(MODELS_DIR, "station_megastructure")
    write_bmp24(f"{prefix}_albedo.bmp", size, size, albedo.tobytes())
    write_bmp24(f"{prefix}_normal.bmp", size, size, normal.tobytes())
    write_bmp24(f"{prefix}_rough.bmp", size, size, rough.tobytes())
    write_bmp24(f"{prefix}_metal.bmp", size, size, metal.tobytes())
    write_bmp24(f"{prefix}_emissive.bmp", size, size, emissive.tobytes())


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
def add_torus_tubular_uv(bm, major_r, minor_r, major_segs, minor_segs,
                         u_range=(0.5, 1.0), v_range=(0.5, 1.0), z_offset=0.0):
    """Creates a high-poly torus in bmesh with continuous, non-overlapping tubular UV mapping."""
    uv_layer = bm.loops.layers.uv.verify()
    u_min, u_max = u_range
    v_min, v_max = v_range

    verts = []
    for i in range(major_segs):
        theta = 2.0 * math.pi * i / major_segs
        cos_t = math.cos(theta)
        sin_t = math.sin(theta)
        ring = []
        for j in range(minor_segs):
            phi = 2.0 * math.pi * j / minor_segs
            cos_p = math.cos(phi)
            sin_p = math.sin(phi)
            x = (major_r + minor_r * cos_p) * cos_t
            y = (major_r + minor_r * cos_p) * sin_t
            z = minor_r * sin_p + z_offset
            v = bm.verts.new((x, y, z))
            v.normal = Vector((cos_p * cos_t, cos_p * sin_t, sin_p)).normalized()
            ring.append(v)
        verts.append(ring)

    bm.verts.ensure_lookup_table()

    for i in range(major_segs):
        i_next = (i + 1) % major_segs
        u0 = u_min + (u_max - u_min) * (i / major_segs)
        u1 = u_min + (u_max - u_min) * ((i + 1) / major_segs)
        for j in range(minor_segs):
            j_next = (j + 1) % minor_segs
            v0 = v_min + (v_max - v_min) * (j / minor_segs)
            v1 = v_min + (v_max - v_min) * ((j + 1) / minor_segs)

            v_a = verts[i][j]
            v_b = verts[i_next][j]
            v_c = verts[i_next][j_next]
            v_d = verts[i][j_next]

            try:
                face = bm.faces.new((v_a, v_b, v_c, v_d))
                face.smooth = True
                for loop in face.loops:
                    if loop.vert == v_a:
                        loop[uv_layer].uv = (u0, v0)
                    elif loop.vert == v_b:
                        loop[uv_layer].uv = (u1, v0)
                    elif loop.vert == v_c:
                        loop[uv_layer].uv = (u1, v1)
                    elif loop.vert == v_d:
                        loop[uv_layer].uv = (u0, v1)
            except ValueError:
                pass


def build_megastructure_mesh():
    print("[MESH] Building Orbital Megastructure 3D model with AAA procedural detailing...")
    bpy.ops.wm.read_factory_settings(use_empty=True)
    bm = bmesh.new()

    # 1. Primary Habitation Torus Ring (radius 12.0, tube radius 1.15, 128x32 segments)
    print("  Adding Primary Habitation Torus (128x32)...")
    add_torus_tubular_uv(bm, major_r=12.0, minor_r=1.15, major_segs=128, minor_segs=32,
                         u_range=(0.5, 1.0), v_range=(0.5, 1.0), z_offset=0.0)

    # 2. Secondary Counter-Rotating Habitation Ring (radius 7.2, tube radius 0.75, 96x24 segments)
    print("  Adding Secondary Habitation Torus (96x24)...")
    add_torus_tubular_uv(bm, major_r=7.2, minor_r=0.75, major_segs=96, minor_segs=24,
                         u_range=(0.5, 1.0), v_range=(0.5, 1.0), z_offset=2.4)

    mesh = bpy.data.meshes.new("StationMegastructure_Mesh")
    bm.to_mesh(mesh)
    bm.free()

    mega = bpy.data.objects.new("StationMegastructure", mesh)
    bpy.context.collection.objects.link(mega)
    bpy.context.view_layer.objects.active = mega

    parts = [mega]

    # 3. Central Core Hub & Spire (into Q1)
    bpy.ops.mesh.primitive_cylinder_add(vertices=48, radius=2.6, depth=11.0, location=(0, 0, 0.5))
    parts.append(bpy.context.active_object)

    # Upper Command Cupola Ring
    bpy.ops.mesh.primitive_torus_add(major_radius=2.9, minor_radius=0.35, major_segments=48, minor_segments=16,
                                     location=(0, 0, 4.2))
    parts.append(bpy.context.active_object)

    # Upper Communication Spire & Needle
    bpy.ops.mesh.primitive_cone_add(vertices=32, radius1=1.0, radius2=0.06, depth=9.0, location=(0, 0, 9.5))
    parts.append(bpy.context.active_object)

    # 4 Parabolic communications dishes on articulated booms
    for a in range(4):
        ang = a * (math.pi / 2.0)
        px = math.cos(ang) * 2.2
        py = math.sin(ang) * 2.2
        bpy.ops.mesh.primitive_cylinder_add(vertices=12, radius=0.12, depth=1.6,
                                            location=(px * 0.7, py * 0.7, 7.8),
                                            rotation=(math.pi/4, 0, ang))
        parts.append(bpy.context.active_object)
        bpy.ops.mesh.primitive_cylinder_add(vertices=24, radius=0.85, depth=0.22,
                                            location=(px, py, 8.4),
                                            rotation=(math.pi/3, 0, ang))
        parts.append(bpy.context.active_object)

    # 4. Six Major Structural Spokes connecting Core to Outer Ring
    for i in range(6):
        ang = i * (math.pi / 3.0)
        sx = math.cos(ang) * 6.0
        sy = math.sin(ang) * 6.0
        bpy.ops.mesh.primitive_cylinder_add(vertices=24, radius=0.42, depth=11.4,
                                            location=(sx, sy, 0.0),
                                            rotation=(math.pi/2, 0, ang + math.pi/2))
        parts.append(bpy.context.active_object)

        hx = math.cos(ang) * 2.9
        hy = math.sin(ang) * 2.9
        bpy.ops.mesh.primitive_cylinder_add(vertices=24, radius=0.62, depth=1.2,
                                            location=(hx, hy, 0.0),
                                            rotation=(math.pi/2, 0, ang + math.pi/2))
        parts.append(bpy.context.active_object)

        rx = math.cos(ang) * 11.2
        ry = math.sin(ang) * 11.2
        bpy.ops.mesh.primitive_cylinder_add(vertices=24, radius=0.58, depth=1.1,
                                            location=(rx, ry, 0.0),
                                            rotation=(math.pi/2, 0, ang + math.pi/2))
        parts.append(bpy.context.active_object)

        px = math.cos(ang) * 4.6
        py = math.sin(ang) * 4.6
        bpy.ops.mesh.primitive_cube_add(size=1.0, location=(px, py, 0.0))
        pod = bpy.context.active_object
        pod.scale = (1.6, 0.85, 0.85)
        pod.rotation_euler = (0, 0, ang)
        bpy.ops.object.transform_apply(scale=True, rotation=True)
        parts.append(pod)

    # 4 Counter-rotating ring support struts
    for i in range(4):
        ang = i * (math.pi / 2.0) + (math.pi / 4.0)
        sx = math.cos(ang) * 4.4
        sy = math.sin(ang) * 4.4
        bpy.ops.mesh.primitive_cylinder_add(vertices=16, radius=0.28, depth=5.6,
                                            location=(sx, sy, 1.2),
                                            rotation=(math.pi/3.5, 0, ang + math.pi/2))
        parts.append(bpy.context.active_object)

    # 5. Lower Industrial Docking Quadrant
    bpy.ops.mesh.primitive_cylinder_add(vertices=48, radius=3.4, depth=3.2, location=(0, 0, -6.8))
    parts.append(bpy.context.active_object)

    # 4 Heavy capital ship docking slipways / cradles
    for d in range(4):
        dang = d * (math.pi / 2.0)
        dx = math.cos(dang) * 5.6
        dy = math.sin(dang) * 5.6
        bpy.ops.mesh.primitive_cube_add(size=1.0, location=(dx, dy, -6.8))
        cradle = bpy.context.active_object
        cradle.scale = (3.2, 1.1, 1.8)
        cradle.rotation_euler = (0, 0, dang)
        bpy.ops.object.transform_apply(scale=True, rotation=True)
        parts.append(cradle)

        if HAS_SDT:
            apas = sdt.create_docking_port_apas(name=f"APAS_{d}", radius=0.55)
            apas.location = (math.cos(dang) * 7.2, math.sin(dang) * 7.2, -6.8)
            apas.rotation_euler = (0, math.pi/2, dang)
            parts.append(apas)

    # 6. Thermal Radiator Arrays (6 large panels along lower spine)
    for f in range(6):
        fang = f * (math.pi / 3.0)
        fx = math.cos(fang) * 4.8
        fy = math.sin(fang) * 4.8
        if HAS_SDT:
            rad = sdt.create_radiator_panel(name=f"Radiator_{f}", length=4.5, width=1.6, vanes=18)
            rad.location = (fx, fy, -10.5)
            rad.rotation_euler = (0, 0, fang)
            parts.append(rad)
        else:
            bpy.ops.mesh.primitive_cube_add(size=1.0, location=(fx, fy, -10.5))
            fin = bpy.context.active_object
            fin.scale = (0.08, 1.8, 4.5)
            fin.rotation_euler = (0, 0, fang)
            bpy.ops.object.transform_apply(scale=True, rotation=True)
            parts.append(fin)

    # 7. Articulated Solar Arrays (4 large wings extending radially)
    for s_idx in range(4):
        sang = s_idx * (math.pi / 2.0) + (math.pi / 4.0)
        sx = math.cos(sang) * 3.2
        sy = math.sin(sang) * 3.2
        if HAS_SDT:
            solar = sdt.create_solar_array(name=f"SolarWing_{s_idx}", length=7.5, width=2.2, segments=6)
            solar.location = (sx, sy, -2.5)
            solar.rotation_euler = (0, 0, sang)
            parts.append(solar)

    # Join all sub-components into a unified megastructure object
    bpy.ops.object.select_all(action='DESELECT')
    for p in parts:
        p.select_set(True)
    bpy.context.view_layer.objects.active = mega
    bpy.ops.object.join()

    # Smart unwrap non-torus faces into Q1/Q3/Q4 without disturbing the torus UVs
    bpy.ops.object.mode_set(mode='EDIT')
    bm_join = bmesh.from_edit_mesh(mega.data)
    uv_layer = bm_join.loops.layers.uv.verify()

    unwrapped_faces = []
    for face in bm_join.faces:
        u_vals = [loop[uv_layer].uv.x for loop in face.loops]
        if all(u >= 0.49 for u in u_vals):
            continue
        unwrapped_faces.append(face)

    bpy.ops.mesh.select_all(action='DESELECT')
    for f in unwrapped_faces:
        f.select = True
    bmesh.update_edit_mesh(mega.data)

    bpy.ops.uv.smart_project(angle_limit=66.0, island_margin=0.015)
    bm_join = bmesh.from_edit_mesh(mega.data)
    uv_layer = bm_join.loops.layers.uv.verify()
    for face in bm_join.faces:
        if face.select:
            for loop in face.loops:
                uv = loop[uv_layer].uv
                loop[uv_layer].uv = (uv.x * 0.48, uv.y * 0.98)

    bmesh.update_edit_mesh(mega.data)
    bpy.ops.mesh.normals_make_consistent(inside=False)
    bpy.ops.object.mode_set(mode='OBJECT')

    for poly in mega.data.polygons:
        poly.use_smooth = True

    bev = mega.modifiers.new(name="Bevel", type='BEVEL')
    bev.width = 0.008
    bev.segments = 2
    bev.limit_method = 'ANGLE'
    bev.angle_limit = math.radians(35.0)
    bev.harden_normals = True

    wn = mega.modifiers.new(name="WeightedNormal", type='WEIGHTED_NORMAL')
    wn.keep_sharp = True
    wn.weight = 80

    bpy.ops.object.select_all(action='DESELECT')
    mega.select_set(True)
    bpy.context.view_layer.objects.active = mega
    bpy.ops.object.origin_set(type='ORIGIN_GEOMETRY', center='BOUNDS')
    megastructure = mega
    megastructure.location = (0, 0, 0)

    out_obj = os.path.join(MODELS_DIR, "station_megastructure.obj")
    bpy.ops.wm.obj_export(filepath=out_obj, export_selected_objects=True,
                          export_normals=True, export_uv=True, export_materials=False,
                          export_triangulated_mesh=False, forward_axis='Y', up_axis='Z')
    print(f"[EXPORT] Megastructure saved: {out_obj} ({len(mega.data.polygons)} polygons)")
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
    generate_megastructure_textures(2048)
    generate_bridge_interior_textures(1024)
    render_showcases()
    print("=== ASSET GENERATION COMPLETED SUCCESSFULLY ===")

if __name__ == "__main__":
    main()
