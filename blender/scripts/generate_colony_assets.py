"""Procedural planetary infrastructure assets for Astro Voyager colonies.

Assets (own procedural geometry, no third-party models):
  colony_dome       - geodesic biosphere habitat with airlock tunnels
  colony_factory    - ore refinery & additive-fabrication plant
  colony_pad        - spaceport tarmac with strobe pylons and control tower
  colony_nuclear    - fission station: containment, crane bay, turbine hall,
                      hyperboloid cooling towers, cask yard, switchyard
  colony_hydro      - dam with crest gallery, spillway/flip buckets, intake
                      towers, penstocks, powerhouse, transformer yard, tailrace
  colony_thermal    - condensing station: screen-wall boiler house, banded
                      stacks, ACC A-frames, fuel yard, precipitator, switchyard
  colony_datacenter - data campus: windowless halls with louvers, chiller
                      plant, dry coolers, substation, genset row, fibre mast

Proportions were checked against public plant photography and layout material
(hyperboloid cooling towers, dam crest/spillway, boiler-house/stack/ACC layout,
generator yard + substation + chiller plant of a data campus); every mesh is
rebuilt from primitives here. PBR atlases come from the shared flip-safe
asset_atlas module.

Run: blender --background --python blender/scripts/generate_colony_assets.py
Subset: add "-- --only nuclear hydro datacenter".
"""
import math
import sys
from pathlib import Path

import bpy
from mathutils import Vector

sys.path.insert(0, str(Path(__file__).resolve().parent))
import asset_atlas

ROOT = Path(__file__).resolve().parents[2]
MODELS = ROOT / "res/models"
OUT = ROOT / "blender/out/colony"

PALETTE = [
    ((0.85, 0.86, 0.88), 0.32, 0.45, (0.0, 0.0, 0.0)),    # 0 white composite
    ((0.08, 0.09, 0.12), 0.35, 0.82, (0.0, 0.0, 0.0)),    # 1 dark titanium
    ((0.82, 0.60, 0.18), 0.28, 0.75, (0.0, 0.0, 0.0)),    # 2 gold MLI / tanks
    ((0.04, 0.07, 0.18), 0.20, 0.50, (0.0, 0.0, 0.0)),    # 3 dark blue solar/optics
    ((0.15, 0.85, 0.95), 0.15, 0.10, (0.15, 0.90, 0.98)), # 4 cyan windows
    ((0.65, 0.67, 0.70), 0.28, 0.88, (0.0, 0.0, 0.0)),    # 5 bright alloy
    ((0.02, 0.03, 0.04), 0.45, 0.30, (0.0, 0.0, 0.0)),    # 6 dark recess / exhaust
    ((0.98, 0.60, 0.12), 0.20, 0.10, (1.0, 0.65, 0.15)),  # 7 amber beacon
]
# Slot 0 doubles as structural concrete: matte, low metalness, warm grey.
CONCRETE = ((0.58, 0.56, 0.52), 0.62, 0.15, (0.0, 0.0, 0.0))

MATERIAL = None
PARTS = []


def make_atlas(prefix, size=1024):
    return asset_atlas.build(f"{prefix}_PBR", [CONCRETE] + list(PALETTE[1:]),
                             MODELS, prefix, size=size, seed_base=3000)


def finish(obj, slot, smooth=False, bevel=0.005):
    obj.data.materials.append(MATERIAL)
    asset_atlas.remap_uv(obj, slot)
    for polygon in obj.data.polygons:
        polygon.use_smooth = smooth
    if bevel > 0:
        bpy.context.view_layer.objects.active = obj
        mod = obj.modifiers.new("Bevel", "BEVEL")
        mod.width = bevel
        mod.segments = 2
        mod.limit_method = "ANGLE"
        mod.angle_limit = math.radians(42)
        bpy.ops.object.modifier_apply(modifier=mod.name)
    PARTS.append(obj)
    return obj


def box(center, dims, slot=0, rot_z=0.0, bevel=0.005):
    bpy.ops.mesh.primitive_cube_add(size=1.0, location=center)
    obj = bpy.context.object
    obj.scale = dims
    obj.rotation_euler.z = rot_z
    bpy.ops.object.transform_apply(scale=True, rotation=True)
    return finish(obj, slot, bevel=bevel)


def cyl(p0, p1, radius, slot=0, verts=20, smooth=True):
    a, b = Vector(p0), Vector(p1)
    bpy.ops.mesh.primitive_cylinder_add(vertices=verts, radius=radius,
                                        depth=(b - a).length, location=(a + b) / 2)
    obj = bpy.context.object
    obj.rotation_euler = (b - a).to_track_quat("Z", "Y").to_euler()
    return finish(obj, slot, smooth=smooth)


def cone(center, r1, r2, depth, slot=0, verts=28, smooth=True):
    bpy.ops.mesh.primitive_cone_add(vertices=verts, radius1=r1, radius2=r2,
                                    depth=depth, location=center)
    return finish(bpy.context.object, slot, smooth=smooth)


def sphere(center, radius, slot=0, segs=20, rings=12, zscale=1.0):
    bpy.ops.mesh.primitive_uv_sphere_add(segments=segs, ring_count=rings,
                                         radius=radius, location=center)
    obj = bpy.context.object
    obj.scale.z = zscale
    bpy.ops.object.transform_apply(scale=True)
    return finish(obj, slot, smooth=True)


def hemisphere(center, radius, slot=0, segs=28, rings=12, zscale=1.0):
    """Upper half of a sphere built directly (Blender 5.2 has no polygon.remove)."""
    verts, faces = [], []
    for i in range(rings):
        phi = (i / rings) * (math.pi / 2)
        z, r = math.sin(phi), math.cos(phi)
        for s in range(segs):
            a = s * math.tau / segs
            verts.append((r * math.cos(a), r * math.sin(a), z))
    for i in range(rings - 1):
        for s in range(segs):
            n = (s + 1) % segs
            a0 = i * segs + s
            faces.append((a0, i * segs + n, (i + 1) * segs + n, (i + 1) * segs + s))
    apex = len(verts)
    verts.append((0.0, 0.0, 1.0))
    last = (rings - 1) * segs
    for s in range(segs):
        faces.append((last + s, last + (s + 1) % segs, apex))
    base = len(verts)
    verts.append((0.0, 0.0, 0.0))
    for s in range(segs):
        faces.append((s, base, (s + 1) % segs))
    mesh = bpy.data.meshes.new("Hemisphere")
    mesh.from_pydata(verts, [], faces)
    mesh.update()
    obj = bpy.data.objects.new("Hemisphere", mesh)
    bpy.context.collection.objects.link(obj)
    obj.scale = (radius, radius, radius * zscale)
    obj.location = center
    bpy.ops.object.select_all(action="DESELECT")
    obj.select_set(True)
    bpy.context.view_layer.objects.active = obj
    bpy.ops.object.transform_apply(scale=True, location=True)
    bpy.ops.object.mode_set(mode="EDIT")
    bpy.ops.mesh.select_all(action="SELECT")
    bpy.ops.mesh.normals_make_consistent(inside=False)
    bpy.ops.object.mode_set(mode="OBJECT")
    for poly in obj.data.polygons:
        poly.use_smooth = True
    return finish(obj, slot, bevel=0)


def torus(center, major, minor, slot=0, major_seg=32, minor_seg=10):
    bpy.ops.mesh.primitive_torus_add(major_radius=major, minor_radius=minor,
                                     major_segments=major_seg,
                                     minor_segments=minor_seg, location=center)
    obj = bpy.context.object
    for poly in obj.data.polygons:
        poly.use_smooth = True
    return finish(obj, slot, bevel=0)


def lathe(profile, center, slot=0, segs=40):
    """Revolve an (radius, z) profile about Z through `center`."""
    verts, faces = [], []
    rings = len(profile)
    for s in range(segs):
        a = s * math.tau / segs
        for (r, z) in profile:
            verts.append((r * math.cos(a), r * math.sin(a), z))
    for s in range(segs):
        nxt = (s + 1) % segs
        for i in range(rings - 1):
            a0 = s * rings + i
            b0 = nxt * rings + i
            faces.append((a0, b0, b0 + 1, a0 + 1))
    faces.append(tuple(range(rings - 1, -1, -1)))
    top = segs * rings
    faces.append(tuple(range(top - rings, top)))
    mesh = bpy.data.meshes.new("Lathe")
    mesh.from_pydata(verts, [], faces)
    mesh.update()
    obj = bpy.data.objects.new("Lathe", mesh)
    bpy.context.collection.objects.link(obj)
    obj.location = center
    bpy.ops.object.select_all(action="DESELECT")
    obj.select_set(True)
    bpy.context.view_layer.objects.active = obj
    bpy.ops.object.transform_apply(location=True)
    bpy.ops.object.mode_set(mode="EDIT")
    bpy.ops.mesh.select_all(action="SELECT")
    bpy.ops.mesh.normals_make_consistent(inside=False)
    bpy.ops.object.mode_set(mode="OBJECT")
    for poly in obj.data.polygons:
        poly.use_smooth = True
    return finish(obj, slot, bevel=0)


def cooling_tower(base, height, throat_r, base_r, segs=32):
    """Natural-draught hyperboloid shell: widest at the floor, throat at 0.72h,
    flared rim, plus the basin ring and inlet framing."""
    z_t = 0.72 * height
    ratio = max((base_r / throat_r) ** 2 - 1.0, 1e-3)
    b = z_t / math.sqrt(ratio)
    steps = 16
    profile = []
    for i in range(steps + 1):
        z = height * i / steps
        r = throat_r * math.sqrt(1.0 + ((z - z_t) / b) ** 2)
        if i >= steps - 2:                      # flared top lip
            r *= 1.0 + 0.10 * (i - (steps - 3)) / 2.0
        profile.append((r, z))
    obj = lathe(profile, base, slot=0, segs=segs)
    r0 = profile[0][0]
    torus((base[0], base[1], base[2] + 0.03), r0 * 1.03, r0 * 0.075, 1,
          major_seg=segs, minor_seg=8)
    for i in range(0, segs, 4):                 # vertical concrete ribs
        a = i * math.tau / segs
        for t in (0.30, 0.55, 0.80):
            z = t * height
            k = min(int(t * steps), steps)
            r = profile[k][0]
            cyl((base[0] + r * math.cos(a), base[1] + r * math.sin(a), z - 0.14),
                (base[0] + r * math.cos(a), base[1] + r * math.sin(a), z + 0.14),
                r0 * 0.016, 1, verts=6)
    return obj


def stack(base, height, radius, bands=4):
    """Banded exhaust stack: taper, rim, ladder rungs and an aviation beacon."""
    cyl(base, (base[0], base[1], base[2] + height), radius, 0, verts=20)
    cone((base[0], base[1], base[2] + height + 0.02), radius * 1.16,
         radius * 0.94, 0.07, 1, verts=20)
    for b in range(1, bands + 1):
        z = base[2] + height * b / (bands + 1)
        torus((base[0], base[1], z), radius * 1.04, radius * 0.11, 1,
              major_seg=18, minor_seg=6)
    steps = max(4, int(height / 0.24))
    for s in range(steps):
        z = base[2] + 0.05 + s * (height - 0.12) / steps
        cyl((base[0] + radius * 1.08, base[1], z),
            (base[0] + radius * 1.08, base[1], z + 0.07), radius * 0.06, 5, verts=6)
    sphere((base[0], base[1], base[2] + height + 0.10), radius * 0.17, 7,
           segs=8, rings=6)


def railing(p0, p1, slot=5, posts=6, height=0.12):
    a, b = Vector(p0), Vector(p1)
    cyl(a, b, height * 0.06, slot, verts=6)
    mid = (a + b) * 0.5
    cyl((mid.x, mid.y, mid.z - height * 0.5), (mid.x, mid.y, mid.z - height * 0.06),
        height * 0.05, slot, verts=6)
    for i in range(posts + 1):
        p = a.lerp(b, i / posts)
        cyl((p.x, p.y, p.z - height), p, height * 0.06, slot, verts=6)


def pipe_run(p0, p1, radius, slot=5, supports=4):
    a, b = Vector(p0), Vector(p1)
    cyl(p0, p1, radius, slot, verts=12)
    for i in range(supports + 1):
        p = a.lerp(b, i / supports)
        box((p.x, p.y, p.z - radius * 2.4), (radius * 0.9, radius * 3.2, radius * 3.6),
            1, bevel=0)


def pipe_rack(origin, length, levels=3):
    ox, oy, oz = origin
    for i in range(int(length / 1.1) + 1):
        x = ox + i * 1.1
        for s in (-0.28, 0.28):
            cyl((x, oy + s, oz), (x, oy + s, oz + 1.05 + levels * 0.22), 0.045, 1, verts=8)
        box((x, oy, oz + 1.05 + levels * 0.22), (0.10, 0.74, 0.05), 5, bevel=0)
    for lvl in range(levels):
        z = oz + 1.15 + lvl * 0.22
        pipe_run((ox, oy - 0.18 + lvl * 0.06, z),
                 (ox + length, oy - 0.18 + lvl * 0.06, z),
                 0.055 + lvl * 0.012, 5, supports=int(length / 1.1))
    box((ox + length, oy, oz + 0.55), (0.12, 0.8, 1.1), 1, bevel=0)


def louvered_block(center, dims, slot=0, louvers=8, face="y", bands=1):
    box(center, dims, slot)
    cx, cy, cz = center
    sx, sy, sz = dims
    for i in range(louvers):
        t = (i + 0.5) / louvers
        if face == "y":
            box((cx - sx / 2 + t * sx, cy + sy / 2 + 0.008, cz),
                (sx / louvers * 0.72, 0.016, sz * bands), 1, bevel=0)
        else:
            box((cx + sx / 2 + 0.008, cy - sy / 2 + t * sy, cz),
                (0.016, sy / louvers * 0.72, sz * bands), 1, bevel=0)


def transformer(center, scale=1.0):
    cx, cy, cz = center
    box((cx, cy, cz + 0.22 * scale), (0.5 * scale, 0.36 * scale, 0.44 * scale), 5)
    box((cx, cy, cz + 0.02 * scale), (0.58 * scale, 0.44 * scale, 0.04 * scale), 1, bevel=0)
    for s in (-1, 1):
        cyl((cx + s * 0.14 * scale, cy, cz + 0.44 * scale),
            (cx + s * 0.14 * scale, cy, cz + 0.70 * scale), 0.055 * scale, 1, verts=10)
        for k in range(3):
            torus((cx + s * 0.14 * scale, cy, cz + 0.50 * scale + k * 0.07 * scale),
                  0.06 * scale, 0.012 * scale, 5, major_seg=10, minor_seg=6)
        cyl((cx + s * 0.14 * scale, cy, cz + 0.76 * scale),
            (cx + s * 0.14 * scale, cy, cz + 0.98 * scale), 0.02 * scale, 5, verts=8)


def switchyard(center, rows=2, bays=4, span=0.62):
    cx, cy, cz = center
    for r in range(rows):
        y = cy + r * span
        for b in range(bays):
            x = cx + b * span
            for s in (-0.4, 0.4):
                cyl((x + s * span, y, cz), (x + s * span, y, cz + 0.52), 0.03, 1, verts=8)
            box((x, y, cz + 0.55), (span * 0.9, 0.05, 0.04), 5, bevel=0)
            for i in range(5):
                cyl((x - span * 0.3 + i * span * 0.15, y, cz + 0.52),
                    (x - span * 0.3 + i * span * 0.15, y, cz + 0.61), 0.018, 2, verts=8)
            transformer((x, y + span * 0.34, cz), scale=0.55)
    box((cx + span * (bays - 1) / 2, cy + span * rows, cz + 0.02),
        (span * bays * 1.05, 0.02, 0.04), 1, bevel=0)


def fence_loop(center, w, d, height=0.34, posts=10):
    cx, cy, cz = center
    corners = ((cx - w / 2, cy - d / 2), (cx + w / 2, cy - d / 2),
               (cx + w / 2, cy + d / 2), (cx - w / 2, cy + d / 2))
    for i in range(4):
        a, b = corners[i], corners[(i + 1) % 4]
        railing((a[0], a[1], cz + height), (b[0], b[1], cz + height), 5, posts, height)
        cyl((a[0], a[1], cz), (a[0], a[1], cz + height * 1.18), height * 0.10, 1, verts=8)


def floodlight(center, height=0.9):
    cx, cy, cz = center
    cyl((cx, cy, cz), (cx, cy, cz + height), 0.022, 5, verts=8)
    box((cx, cy, cz + height + 0.03), (0.16, 0.07, 0.05), 1)
    box((cx, cy, cz + height + 0.005), (0.13, 0.05, 0.015), 4, bevel=0)


def container(center, rot_z=0.0, slot=0):
    box(center, (0.30, 0.14, 0.13), slot, rot_z)
    cx, cy, cz = center
    for i in range(5):
        box((cx - 0.12 + i * 0.06, cy, cz), (0.006, 0.145, 0.115), 1, rot_z, bevel=0)


def pallet_stack(center, n=3):
    cx, cy, cz = center
    for i in range(n):
        box((cx, cy, cz + 0.03 + i * 0.06), (0.20, 0.20, 0.05), 2)


def stairs(center, rise, run, steps=8, slot=5):
    cx, cy, cz = center
    for i in range(steps):
        box((cx, cy, cz + rise * (i + 0.5) / steps),
            (0.10, run / steps * 0.95, rise / steps * 0.9), slot, bevel=0)


def gantry_crane(center, span, height):
    cx, cy, cz = center
    for s in (-1, 1):
        box((cx, cy + s * span / 2, cz + height / 2), (0.09, 0.09, height), 1)
        for e in (-1, 1):
            box((cx + e * span / 2, cy + s * span / 2, cz + 0.04), (0.3, 0.3, 0.08), 1)
    box((cx, cy, cz + height), (span * 1.08, 0.16, 0.10), 5)
    box((cx, cy, cz + height - 0.12), (span * 1.08, 0.05, 0.03), 1, bevel=0)
    tx = cx + span * 0.18
    box((tx, cy, cz + height - 0.16), (0.22, 0.26, 0.10), 2)
    cyl((tx, cy, cz + height - 0.21), (tx, cy, cz + height - 0.42), 0.012, 5, verts=8)
    box((tx, cy, cz + height - 0.48), (0.14, 0.14, 0.06), 5)


def conveyor(p0, p1, width=0.16, supports=5):
    a, b = Vector(p0), Vector(p1)
    mid = (a + b) / 2
    box((mid.x, mid.y, mid.z), ((b - a).length, width, width * 0.45), 5,
        rot_z=math.atan2((b - a).y, (b - a).x))
    for i in range(supports + 1):
        p = a.lerp(b, i / supports)
        cyl((p.x, p.y, 0.0), (p.x, p.y, p.z), width * 0.42, 1, verts=8)
        box((p.x, p.y, p.z + width * 0.55), (width * 1.3, width * 1.3, width * 0.18),
            1, bevel=0)


def fan_unit(center, radius, slot=5):
    cx, cy, cz = center
    cyl((cx, cy, cz), (cx, cy, cz + radius * 0.7), radius, 1, verts=16)
    torus((cx, cy, cz + radius * 0.75), radius, radius * 0.13, slot,
          major_seg=16, minor_seg=6)
    for f in range(4):
        a = f * math.tau / 4
        box((cx + radius * 0.55 * math.cos(a), cy + radius * 0.55 * math.sin(a),
             cz + radius * 0.78), (radius * 0.62, radius * 0.12, 0.012), 1, a, bevel=0)


# ===========================================================================
# 1. Habitat dome
# ===========================================================================
def build_colony_dome():
    box((0, 0, 0.12), (4.4, 4.4, 0.25), 1)
    hemisphere((0, 0, 0.25), 2.0, 0, segs=32, rings=16, zscale=0.98)
    for r, z in ((1.78, 0.85), (1.42, 1.42), (0.90, 1.86)):
        torus((0, 0, z), r, 0.055, 4, major_seg=40, minor_seg=8)
    for i in range(12):                       # meridian ribs
        a = i * math.tau / 12
        cyl((0, 0, 0.3), (2.0 * math.cos(a), 2.0 * math.sin(a), 1.9), 0.035, 5, verts=8)
    sphere((0, 0, 2.16), 0.34, 3, segs=16, rings=12)
    cyl((0, 0, 2.45), (0, 0, 3.05), 0.035, 5, verts=8)
    cone((0, 0, 3.12), 0.26, 0.02, 0.14, 5, verts=16)
    sphere((0, 0, 3.22), 0.05, 7, segs=8, rings=6)
    for angle_deg in (0, 120, 240):
        rad = math.radians(angle_deg)
        cx, cy = math.cos(rad) * 2.2, math.sin(rad) * 2.2
        box((cx, cy, 0.42), (0.7, 0.5, 0.5), 1, rad)
        hx, hy = math.cos(rad) * 2.58, math.sin(rad) * 2.58
        cyl((hx, hy, 0.2), (hx, hy, 0.62), 0.22, 7, verts=16)
        cyl((hx, hy, 0.62), (hx, hy, 0.70), 0.24, 5, verts=16)
        railing((cx - 0.3, cy - 0.3, 0.68), (cx + 0.3, cy - 0.3, 0.68), 5, 4, 0.12)
    for i in range(6):
        a = i * math.tau / 6 + 0.5
        floodlight((2.9 * math.cos(a), 2.9 * math.sin(a), 0.0), 0.75)
    container((3.3, -1.2, 0.09), 0.4, 2)
    container((3.3, -1.6, 0.09), 0.4, 0)
    pallet_stack((2.6, 2.4, 0.0))


# ===========================================================================
# 2. Refinery & fabrication plant (detail pass)
# ===========================================================================
def build_colony_factory():
    box((0, 0, 0.15), (4.6, 3.6, 0.3), 1)
    for i in range(9):                        # kerb / cable trench covers
        box((-2.1 + i * 0.52, 1.72, 0.34), (0.05, 0.06, 0.12), 5, bevel=0)

    # Process hall: stepped roof, monitor vents, louvered walls, personnel door
    louvered_block((-0.7, 0.1, 1.05), (2.0, 2.4, 1.5), 0, 10, "y")
    box((-0.7, 0.1, 1.86), (2.06, 2.46, 0.16), 0)
    box((-0.7, 0.1, 2.06), (1.5, 1.9, 0.30), 1)
    for i in range(4):
        box((-1.3 + i * 0.42, 0.1, 2.24), (0.24, 1.7, 0.10), 5, bevel=0)
    box((0.28, -1.12, 0.62), (0.5, 0.06, 1.2), 3)          # door
    for i in range(7):
        box((-1.66, -1.1 + i * 0.36, 1.05), (0.05, 0.22, 1.4), 1, bevel=0)

    # Fabrication bay with a glazed strip window and roof extractors
    box((1.55, -0.6, 0.85), (1.5, 2.0, 1.7), 0)
    box((1.55, -1.62, 1.32), (1.52, 0.05, 0.30), 4, bevel=0)
    for i in range(4):
        box((0.9 + i * 0.42, -0.6, 1.74), (0.34, 2.04, 0.06), 1, bevel=0)
    for i in range(3):
        fan_unit((0.9 + i * 0.5, -0.6, 1.92), 0.14)

    # Silo cluster: ribbed shells, cone roofs, walkway, ladder, vent pipes
    for i, sy in enumerate((-1.25, -0.45, 0.35, 1.15)):
        cx = 1.9
        cyl((cx, sy, 0.3), (cx, sy, 1.9), 0.34, 5, verts=24)
        cone((cx, sy, 2.06), 0.36, 0.05, 0.30, 1, verts=24)
        sphere((cx, sy, 2.34), 0.09, 7, segs=8, rings=6)
        for b in range(3):
            torus((cx, sy, 0.6 + b * 0.5), 0.35, 0.02, 1, major_seg=20, minor_seg=6)
        for s in range(9):
            cyl((cx + 0.36, sy, 0.35 + s * 0.18), (cx + 0.36, sy, 0.45 + s * 0.18),
                0.012, 5, verts=6)
        if i < 3:
            box((cx, (sy + (-1.25 + (i + 1) * 0.8)) / 2, 1.86), (0.24, 0.8, 0.05), 5, bevel=0)
            railing((cx - 0.12, sy, 1.91), (cx - 0.12, -1.25 + (i + 1) * 0.8, 1.91), 5, 4, 0.12)
    cyl((1.9, -1.25, 1.95), (1.9, 1.15, 1.95), 0.05, 5, verts=10)

    # Dust collector with hopper and venturi stack
    box((-2.0, 1.0, 0.9), (0.7, 0.7, 1.2), 1)
    cone((-2.0, 1.0, 0.2), 0.34, 0.06, 0.34, 1, verts=12)
    for i in range(5):
        cyl((-2.0 - 0.35 + i * 0.17, 1.0, 1.52), (-2.0 - 0.35 + i * 0.17, 1.0, 1.80),
            0.055, 5, verts=10)
    cyl((-2.0, 1.0, 1.86), (-1.2, 0.6, 1.6), 0.07, 5, verts=10)

    # Stacks with bands and warning lights
    stack((-1.05, -1.35, 0.3), 2.9, 0.13, bands=5)
    stack((-0.55, -1.35, 0.3), 2.5, 0.11, bands=4)

    gantry_crane((0.4, 1.35, 0.3), 2.4, 1.7)

    # Pipe rack and process vessels
    pipe_rack((-2.2, -1.75, 0.3), 4.4, levels=3)
    for i, sx in enumerate((-1.5, -0.6, 0.3)):
        cyl((sx, -1.75, 1.62), (sx, -1.75, 2.15), 0.05, 5, verts=10)
        cyl((sx, -1.75, 2.15), (sx, -1.2, 2.15), 0.05, 5, verts=10)
    cyl((2.15, -1.4, 0.3), (2.15, -1.4, 1.35), 0.42, 2, verts=20)
    sphere((2.15, -1.4, 1.35), 0.42, 2, segs=20, rings=10, zscale=0.55)
    cyl((2.15, -0.7, 0.55), (2.15, -0.7, 1.15), 0.30, 2, verts=16)
    sphere((2.15, -0.7, 1.15), 0.30, 2, segs=16, rings=8, zscale=0.55)
    pipe_run((2.15, -1.4, 1.9), (1.0, -1.4, 1.9), 0.05, 5, supports=2)

    # Conveyor, transfer tower and ore stockpile
    conveyor((-2.2, 1.75, 1.15), (0.9, 1.75, 1.15), 0.17, supports=4)
    box((0.9, 1.75, 0.9), (0.34, 0.34, 1.8), 1)
    sphere((1.7, 1.75, 0.45), 0.55, 2, segs=20, rings=8, zscale=0.42)

    # Yard electrics and control room
    transformer((-0.15, 1.62, 0.3), scale=0.9)
    transformer((-0.95, 1.62, 0.3), scale=0.9)
    box((2.05, 1.55, 0.55), (0.9, 0.7, 1.1), 0)
    box((2.05, 1.2, 0.86), (0.84, 0.04, 0.26), 4, bevel=0)
    cyl((2.05, 1.55, 1.15), (2.05, 1.55, 1.75), 0.02, 5, verts=6)
    box((2.05, 1.55, 1.80), (0.22, 0.22, 0.05), 1, bevel=0)
    for i in range(3):                        # switchgear cubicles
        box((1.4 + i * 0.3, -1.62, 0.45), (0.22, 0.16, 0.6), 1)
        box((1.4 + i * 0.3, -1.62, 0.78), (0.16, 0.10, 0.06), 5, bevel=0)

    container((-2.3, -1.0, 0.1), 0.0, 2)
    container((-2.3, -1.35, 0.1), 0.0, 0)
    for i in range(4):                        # drum store
        cyl((-1.9 + i * 0.22, -1.95, 0.16), (-1.9 + i * 0.22, -1.95, 0.42), 0.10, 1, verts=12)
    pallet_stack((-0.4, -1.9, 0.3))
    floodlight((2.3, -1.9, 0.3), 0.9)
    floodlight((-2.3, 1.9, 0.3), 0.9)
    fence_loop((0.0, 0.0, 0.3), 4.4, 3.4, 0.34, 12)
    sphere((0.0, 0.0, 2.35), 0.06, 7, segs=8, rings=6)


# ===========================================================================
# 3. Spaceport pad
# ===========================================================================
def build_colony_pad():
    cyl((0, 0, 0.125), (0, 0, 0.125), 2.6, 1, verts=8)
    torus((0, 0, 0.27), 1.85, 0.075, 5, major_seg=40, minor_seg=8)
    for i in range(4):                        # touchdown cross
        a = i * math.pi / 2
        box((0.7 * math.cos(a), 0.7 * math.sin(a), 0.28),
            (1.3, 0.16, 0.02), 7, a, bevel=0)
    for i in range(8):                        # approach beacon pylons
        a = i * math.tau / 8
        px, py = 2.3 * math.cos(a), 2.3 * math.sin(a)
        cyl((px, py, 0.25), (px, py, 1.0), 0.045, 1, verts=8)
        sphere((px, py, 1.06), 0.075, 7, segs=10, rings=6)
        box((px, py, 0.62), (0.10, 0.10, 0.05), 5, a, bevel=0)
    cyl((0, 0, 0.25), (0, 0, 1.5), 0.05, 5, verts=10)         # wind mast
    box((0.0, 0.0, 1.62), (0.5, 0.5, 0.28), 1)
    box((0.0, 0.0, 1.80), (0.42, 0.42, 0.06), 4, bevel=0)
    sphere((0, 0, 1.95), 0.06, 7, segs=8, rings=6)
    cyl((-1.9, -1.9, 0.25), (-1.9, -1.9, 1.15), 0.42, 2, verts=20)   # LOX tank
    sphere((-1.9, -1.9, 1.15), 0.42, 2, segs=20, rings=10, zscale=0.5)
    pipe_run((-1.9, -1.9, 0.6), (-0.6, -1.6, 0.6), 0.05, 5, supports=2)
    box((2.1, 1.6, 0.5), (0.9, 0.8, 1.0), 0)                 # control tower
    box((2.1, 1.2, 0.78), (0.8, 0.04, 0.3), 4, bevel=0)
    cyl((2.1, 1.6, 1.0), (2.1, 1.6, 1.9), 0.02, 5, verts=6)
    cone((2.1, 1.6, 1.98), 0.20, 0.02, 0.12, 5, verts=16)
    for i in range(5):                        # container store
        box((-0.4 + i * 0.35, 2.35, 0.28), (0.28, 0.14, 0.36), 1)
    for i in range(12):                       # blast deflectors
        a = i * math.tau / 12
        box((2.45 * math.cos(a), 2.45 * math.sin(a), 0.28), (0.12, 0.12, 0.06), 5, a, bevel=0)
    floodlight((2.6, -2.2, 0.25), 1.0)
    floodlight((-2.6, 2.2, 0.25), 1.0)
    fence_loop((0, 0, 0.25), 5.2, 5.2, 0.3, 14)


# ===========================================================================
# 4. Fission station
# ===========================================================================
def build_colony_nuclear():
    box((0, 0, 0.12), (5.6, 4.2, 0.24), 1)

    # Reactor containment: cylindrical shell + hemispherical cap + penetrations
    cx, cy = -1.35, 0.35
    cyl((cx, cy, 0.24), (cx, cy, 1.55), 1.05, 0, verts=32)
    hemisphere((cx, cy, 1.55), 1.05, 0, segs=32, rings=14, zscale=0.86)
    for b in (0.55, 1.05, 1.5):
        torus((cx, cy, b), 1.07, 0.035, 1, major_seg=32, minor_seg=8)
    box((cx, cy - 1.02, 0.72), (0.7, 0.14, 1.0), 1)              # personnel airlock
    cyl((cx, cy - 1.12, 0.72), (cx, cy - 1.26, 0.72), 0.30, 5, verts=16)
    for i in range(6):                                           # steam line penetrations
        a = i * math.tau / 6
        cyl((cx + 0.9 * math.cos(a), cy + 0.9 * math.sin(a), 1.2),
            (cx + 1.35 * math.cos(a), cy + 1.35 * math.sin(a), 1.2), 0.09, 5, verts=12)
    cyl((cx, cy, 2.5), (cx, cy, 3.0), 0.06, 5, verts=8)          # ventilation mast
    sphere((cx, cy, 3.05), 0.07, 7, segs=8, rings=6)

    # Reactor crane bay
    box((cx + 1.35, cy, 0.95), (1.5, 1.9, 1.5), 0)
    box((cx + 1.35, cy, 1.78), (1.56, 1.96, 0.16), 1)
    for i in range(5):
        box((cx + 0.72 + i * 0.32, cy - 0.97, 1.05), (0.06, 0.04, 1.0), 3, bevel=0)

    # Turbine hall with roof monitors and condenser pipes
    tx = 0.95
    louvered_block((tx, -0.75, 0.95), (3.1, 1.5, 1.3), 0, 12, "y")
    box((tx, -0.75, 1.66), (3.16, 1.56, 0.14), 1)
    for i in range(4):
        box((tx - 1.2 + i * 0.8, -0.75, 1.82), (0.5, 1.2, 0.18), 5, bevel=0)
        box((tx - 1.2 + i * 0.8, -0.75, 1.95), (0.42, 1.0, 0.08), 1, bevel=0)
    box((tx + 1.55, -0.75, 0.6), (0.06, 1.1, 1.2), 3)            # personnel door
    for i in range(3):
        cyl((tx - 1.0 + i * 1.0, -1.55, 0.35), (tx - 1.0 + i * 1.0, -1.55, 1.35),
            0.11, 5, verts=12)

    # Two hyperboloid cooling towers, basin and pump house
    cooling_tower((1.0, 1.65, 0.24), 1.95, 0.55, 0.85, segs=32)
    cooling_tower((2.75, 1.65, 0.24), 1.65, 0.47, 0.72, segs=28)
    box((1.85, 1.65, 0.26), (3.0, 1.3, 0.06), 3)                 # basin wall
    box((0.15, 1.75, 0.55), (0.7, 0.6, 0.7), 1)                  # pump house
    pipe_run((0.5, 1.75, 0.6), (1.0, 1.65, 0.5), 0.07, 5, supports=2)

    # Spent-fuel cask yard
    box((-1.6, -1.55, 0.26), (2.0, 1.1, 0.06), 1)
    for r in range(2):
        for c in range(5):
            x, y = -2.4 + c * 0.42, -1.75 + r * 0.42
            cyl((x, y, 0.29), (x, y, 0.55), 0.14, 5, verts=14)
            for b in range(3):
                torus((x, y, 0.34 + b * 0.08), 0.145, 0.012, 1, major_seg=14, minor_seg=6)
            box((x, y, 0.58), (0.16, 0.16, 0.04), 7, bevel=0)

    # Rad-waste drums, auxiliary stack, service building
    for i in range(4):
        cyl((-2.5 + i * 0.24, 0.9, 0.28), (-2.5 + i * 0.24, 0.9, 0.56), 0.11, 2, verts=12)
    stack((-2.55, -0.45, 0.28), 1.6, 0.09, bands=3)
    box((-2.45, 1.75, 0.5), (1.0, 0.7, 1.0), 0)
    box((-2.45, 1.4, 0.7), (0.9, 0.04, 0.28), 4, bevel=0)

    switchyard((0.4, -2.05, 0.26), rows=2, bays=4, span=0.6)
    fence_loop((0, 0, 0.24), 5.4, 4.0, 0.32, 14)
    for i in range(4):
        sx = -2.6 if i % 2 == 0 else 2.6
        sy = -1.9 if i < 2 else 1.9
        floodlight((sx, sy, 0.24), 0.95)


# ===========================================================================
# 5. Hydro station
# ===========================================================================
def build_colony_hydro():
    # Curved gravity dam: battered downstream face, crest at +Z
    segs, arc, r = 20, math.radians(120), 2.35
    for i in range(segs):
        a0 = -arc / 2 + arc * i / segs
        a1 = -arc / 2 + arc * (i + 1) / segs
        p0 = (r * math.cos(a0), r * math.sin(a0))
        p1 = (r * math.cos(a1), r * math.sin(a1))
        q0 = ((r - 0.6) * math.cos(a0), (r - 0.6) * math.sin(a0))
        q1 = ((r - 0.6) * math.cos(a1), (r - 0.6) * math.sin(a1))
        verts = [(p0[0], p0[1], 0.0), (p1[0], p1[1], 0.0), (p1[0], p1[1], 2.0),
                 (p0[0], p0[1], 2.0), (q0[0], q0[1], 0.3), (q1[0], q1[1], 0.3),
                 (q1[0], q1[1], 1.9), (q0[0], q0[1], 1.9)]
        faces = [(0, 1, 2, 3), (4, 7, 6, 5), (0, 4, 5, 1), (3, 2, 6, 7),
                 (0, 3, 7, 4), (1, 5, 6, 2)]
        mesh = bpy.data.meshes.new("DamSeg")
        mesh.from_pydata(verts, [], faces)
        mesh.update()
        obj = bpy.data.objects.new("DamSeg", mesh)
        bpy.context.collection.objects.link(obj)
        finish(obj, 0, smooth=False, bevel=0)

    # Crest gallery, walkways and gate hoists
    box((0, r, 2.1), (2.2, 0.36, 0.24), 1)
    railing((-1.05, r + 0.22, 2.24), (1.05, r + 0.22, 2.24), 5, 10, 0.14)
    railing((-1.05, r - 0.22, 2.24), (1.05, r - 0.22, 2.24), 5, 10, 0.14)
    for i in range(3):
        x = -0.8 + i * 0.8
        box((x, r, 2.5), (0.36, 0.36, 0.55), 5)
        cyl((x, r, 2.8), (x, r, 3.05), 0.05, 5, verts=8)

    # Spillway bays with ogee piers, chute steps and flip buckets
    for i in range(3):
        x = -0.55 + i * 0.55
        box((x, r - 0.8, 1.15), (0.44, 1.4, 0.10), 1, bevel=0)
        for s in range(5):
            box((x, r - 1.3 + s * 0.24, 0.92 - s * 0.17), (0.46, 0.06, 0.10), 5, bevel=0)
        cone((x, r - 1.6, 0.28), 0.20, 0.06, 0.22, 5, verts=12)
        box((x, r - 0.45, 1.65), (0.12, 0.12, 1.1), 0)

    # Intake towers with trash racks and penstocks
    for i in range(3):
        x = -0.9 + i * 0.9
        box((x, r + 0.95, 1.3), (0.5, 0.5, 2.2), 0)
        box((x, r + 0.68, 1.05), (0.44, 0.04, 0.5), 4, bevel=0)
        for j in range(6):
            box((x - 0.2 + j * 0.08, r + 1.21, 0.75), (0.02, 0.02, 0.9), 1, bevel=0)
        cyl((x, r + 0.95, 0.2), (x, r - 0.55, 0.5), 0.16, 5, verts=14)

    # Powerhouse: generator bay, roof crane, tailrace and draft tubes
    box((0, r - 2.6, 0.78), (2.7, 1.5, 1.55), 0)
    box((0, r - 2.6, 1.6), (2.76, 1.56, 0.14), 1)
    for i in range(4):
        box((-0.9 + i * 0.6, r - 2.6, 1.76), (0.4, 1.2, 0.2), 5, bevel=0)
        cyl((-0.9 + i * 0.6, r - 2.6, 1.94), (-0.9 + i * 0.6, r - 2.6, 2.1),
            0.12, 2, verts=12)
    for i in range(4):
        cyl((-1.05 + i * 0.7, r - 3.4, 0.35), (-1.05 + i * 0.7, r - 3.65, 0.35),
            0.22, 1, verts=14)
    box((0, r - 3.8, 0.2), (2.7, 0.5, 0.4), 3)                   # stilling basin
    for i in range(9):
        box((-1.2 + i * 0.3, r - 2.6, 0.06), (0.05, 1.4, 0.12), 1, bevel=0)

    # Transformer yard and outdoor switchyard on the abutment
    switchyard((1.95, r - 1.7, 0.24), rows=2, bays=3, span=0.55)
    transformer((-1.9, r - 1.7, 0.24), scale=1.0)
    transformer((-1.9, r - 2.4, 0.24), scale=1.0)
    for i in range(3):
        cyl((-2.1 + i * 0.2, r - 3.0, 0.24), (-2.1 + i * 0.2, r - 3.0, 0.9),
            0.03, 5, verts=8)

    # Reservoir: water surface must meet the upstream face and be contained by
    # abutment walls, otherwise it reads as a floating card in space.
    water_z = 1.72
    box((0, r + 0.95, water_z), (4.4, 2.6, 0.05), 3)
    for s in (-1, 1):                    # abutment walls closing the reservoir
        x = s * 2.2
        box((x, r + 1.6, 1.05), (0.5, 3.4, 2.1), 0)
        box((x * 0.72, r + 3.0, 1.05), (3.6, 0.45, 2.1), 0)
    for i in range(9):                   # shoreline rocks along the banks
        a = -arc / 2 + arc * i / 8
        box((1.15 * r * math.cos(a), 1.15 * r * math.sin(a) + 0.35, 0.25),
            (0.5, 0.5, 0.5), 0, a)
    # Tailrace channel carrying the discharged water away from the powerhouse
    box((0, r - 4.15, 0.14), (2.7, 1.0, 0.06), 3)
    for s in (-1, 1):
        box((s * 1.4, r - 4.15, 0.3), (0.12, 1.1, 0.4), 1)
    for i in range(7):
        box((-2.4 + i * 0.8, r + 0.15, 2.04), (0.5, 0.12, 0.05), 5, bevel=0)
    box((-2.7, r - 2.7, 0.6), (0.9, 0.8, 1.2), 0)
    box((-2.7, r - 3.12, 0.9), (0.8, 0.04, 0.3), 4, bevel=0)
    cyl((-2.7, r - 2.7, 1.2), (-2.7, r - 2.7, 2.0), 0.02, 5, verts=6)
    fence_loop((0, r - 2.3, 0.24), 5.6, 2.6, 0.3, 12)
    floodlight((2.5, r - 3.5, 0.24), 0.9)
    floodlight((-2.5, r - 3.5, 0.24), 0.9)
    sphere((0, r - 2.6, 2.25), 0.07, 7, segs=8, rings=6)


# ===========================================================================
# 6. Condensing station
# ===========================================================================
def build_colony_thermal():
    box((0, 0, 0.12), (6.0, 4.0, 0.24), 1)

    # Boiler house: screen-wall bays, access platforms, stair towers, deaerators
    for b in range(4):
        x = -1.95 + b * 1.3
        box((x, 0.55, 1.35), (1.1, 1.5, 2.4), 1)
        for i in range(5):
            box((x - 0.45 + i * 0.22, -0.22, 1.35), (0.06, 0.05, 2.0), 5, bevel=0)
        box((x, 0.55, 2.62), (1.16, 1.56, 0.16), 0)
        for f in (0.7, 1.5, 2.2):
            box((x, -0.35, f), (1.05, 0.16, 0.05), 5, bevel=0)
            railing((x - 0.5, -0.42, f + 0.14), (x + 0.5, -0.42, f + 0.14), 5, 5, 0.14)
        stairs((x + 0.62, 0.1, 0.3), 2.4, 0.9, steps=10)
    for i in range(2):
        cyl((-1.3 + i * 2.6, 0.55, 2.9), (-1.3 + i * 2.6, 0.55, 3.5), 0.28, 2, verts=18)
        sphere((-1.3 + i * 2.6, 0.55, 3.5), 0.28, 2, segs=18, rings=8, zscale=0.5)

    stack((-2.6, 1.35, 0.24), 3.6, 0.17, bands=6)
    stack((-2.6, 0.35, 0.24), 3.2, 0.15, bands=5)

    # Turbine hall with roof monitors, crane bay and extraction pipes
    box((1.35, -0.9, 1.0), (3.0, 1.7, 1.7), 0)
    box((1.35, -0.9, 1.9), (3.06, 1.76, 0.14), 1)
    for i in range(4):
        box((0.35 + i * 0.68, -0.9, 2.05), (0.46, 1.4, 0.22), 5, bevel=0)
    for i in range(4):
        cyl((0.3 + i * 0.7, -1.78, 0.3), (0.3 + i * 0.7, -1.78, 1.1), 0.13, 5, verts=14)
        cyl((0.3 + i * 0.7, -1.78, 1.1), (0.3 + i * 0.7, -1.78, 1.35), 0.17, 1, verts=14)
    box((2.9, -0.9, 0.62), (0.06, 1.2, 1.2), 3)

    # Air-cooled condenser A-frames with fan rows and exhaust ducts
    for i in range(3):
        x = -0.4 + i * 1.25
        for s in (-1, 1):
            box((x, -2.35 + s * 0.3, 0.75), (1.05, 0.10, 1.0), 1, bevel=0)
            box((x, -2.35 + s * 0.3, 1.34), (1.05, 0.9, 0.06), 5, bevel=0)
        for j in range(2):
            fan_unit((x - 0.28 + j * 0.56, -2.35, 1.42), 0.24)
        box((x, -1.6, 0.5), (0.9, 0.6, 0.5), 1)

    # Fuel yard: stockpiles, stacker/reclaimer, conveyor gantry
    sphere((-2.3, -1.9, 0.5), 0.75, 2, segs=20, rings=8, zscale=0.36)
    sphere((-1.1, -2.1, 0.45), 0.6, 2, segs=18, rings=8, zscale=0.34)
    box((-2.3, -1.0, 0.9), (0.16, 0.16, 1.5), 5)
    box((-2.3, -1.0, 1.72), (1.6, 0.2, 0.12), 5)
    cyl((-1.5, -1.0, 1.72), (-1.5, -1.9, 1.05), 0.05, 5, verts=8)
    conveyor((-2.9, -0.35, 1.3), (0.9, -0.35, 1.3), 0.18, supports=5)

    # Electrostatic precipitator boxes and ductwork
    for i in range(3):
        box((-2.55 + i * 0.62, 1.85, 0.75), (0.56, 0.7, 1.2), 1)
        box((-2.55 + i * 0.62, 1.85, 1.4), (0.6, 0.74, 0.08), 5, bevel=0)
        cyl((-2.55 + i * 0.62, 1.85, 1.5), (-2.55 + i * 0.62, 1.85, 1.75), 0.07, 1, verts=10)
    cyl((-1.6, 1.2, 1.5), (-1.6, 1.85, 1.5), 0.22, 5, verts=14)

    # Circulating water basin, pump house and penstocks
    box((2.6, 1.5, 0.2), (1.6, 1.4, 0.06), 3)
    box((2.6, 2.25, 0.45), (0.7, 0.5, 0.9), 1)
    for i in range(3):
        cyl((2.3 + i * 0.3, 1.95, 0.5), (2.3 + i * 0.3, 1.5, 0.5), 0.07, 5, verts=10)

    switchyard((0.2, 2.35, 0.24), rows=2, bays=5, span=0.58)
    for i in range(3):
        transformer((1.0 + i * 0.85, 1.55, 0.24), scale=0.85)

    box((2.7, -1.9, 0.6), (1.0, 0.8, 1.2), 0)
    box((2.7, -2.32, 0.85), (0.9, 0.04, 0.3), 4, bevel=0)
    container((2.1, -2.4, 0.12), 0.0, 2)
    container((2.1, -2.75, 0.12), 0.0, 0)
    fence_loop((0, 0, 0.24), 5.8, 3.8, 0.32, 16)
    for i in range(4):
        sx = -2.8 if i % 2 == 0 else 2.8
        sy = -1.8 if i < 2 else 1.8
        floodlight((sx, sy, 0.24), 0.95)


# ===========================================================================
# 7. Data campus
# ===========================================================================
def build_colony_datacenter():
    box((0, 0, 0.1), (5.2, 3.8, 0.2), 1)

    # Data halls: windowless shells, louver banks, roof monitors, docks
    for hy in (-0.85, 0.85):
        louvered_block((-0.75, hy, 0.95), (3.0, 1.4, 1.5), 0, 14, "y")
        box((-0.75, hy, 1.78), (3.06, 1.46, 0.18), 1)
        for j in range(5):
            box((-2.0 + j * 0.62, hy, 1.95), (0.42, 1.1, 0.20), 5, bevel=0)
            for k in range(3):
                cyl((-2.14 + j * 0.62 + k * 0.14, hy, 2.10),
                    (-2.14 + j * 0.62 + k * 0.14, hy, 2.24), 0.045, 1, verts=8)
        for j in range(6):                      # wall-mounted AHU units
            box((0.72, hy - 0.62 + j * 0.24, 0.95), (0.05, 0.16, 1.2), 1, bevel=0)
        box((-2.3, hy, 0.5), (0.06, 0.9, 1.0), 3)   # dock door
        for j in range(3):
            box((-1.7 + j * 0.4, hy - 0.85, 0.32), (0.3, 0.14, 0.34), 5, bevel=0)
        railing((-2.35, hy - 0.6, 1.05), (-2.35, hy + 0.6, 1.05), 5, 5, 0.14)

    # Chiller plant: fan arrays on plinths with pipe headers
    for r in range(2):
        for c in range(4):
            x, y = 1.55 + c * 0.42, -1.35 + r * 0.5
            box((x, y, 0.3), (0.36, 0.36, 0.4), 1)
            fan_unit((x, y, 0.52), 0.17)
    pipe_run((1.5, -1.6, 0.9), (3.0, -1.6, 0.9), 0.06, 5, supports=3)
    pipe_run((1.5, -1.75, 0.9), (3.0, -1.75, 0.9), 0.06, 5, supports=3)

    # Dry coolers on legs (liquid-cooled loop) with CDU skids
    for i in range(3):
        x = -2.1 + i * 0.75
        box((x, -1.55, 0.75), (0.62, 0.34, 0.42), 5)
        for s in (-1, 1):
            cyl((x + s * 0.24, -1.55, 0.2), (x + s * 0.24, -1.55, 0.55), 0.03, 1, verts=8)
        for f in range(4):
            cyl((x - 0.21 + f * 0.14, -1.55, 0.98), (x - 0.21 + f * 0.14, -1.55, 1.06),
                0.055, 1, verts=10)
        box((x, -1.2, 0.32), (0.3, 0.3, 0.44), 1)

    # Substation island
    switchyard((0.4, 1.85, 0.2), rows=1, bays=4, span=0.6)
    for i in range(2):
        transformer((2.0 + i * 0.7, 1.85, 0.2), scale=0.8)
    fence_loop((1.4, 1.85, 0.2), 3.4, 1.1, 0.34, 10)

    # Diesel genset row with exhaust stacks and day tanks
    for i in range(4):
        x = -2.2 + i * 0.55
        box((x, 1.75, 0.42), (0.44, 0.34, 0.64), 5)
        box((x, 1.75, 0.78), (0.36, 0.28, 0.08), 1, bevel=0)
        cyl((x - 0.16, 1.75, 0.86), (x - 0.16, 1.75, 1.12), 0.045, 1, verts=10)
        box((x + 0.14, 1.75, 0.3), (0.14, 0.24, 0.2), 2)

    # Operations building with glazed facade, PV roof and link antenna
    box((2.35, -0.35, 0.75), (1.2, 1.5, 1.5), 0)
    box((1.72, -0.35, 1.0), (0.05, 1.3, 0.9), 4, bevel=0)
    for i in range(4):
        box((2.35, -0.98 + i * 0.42, 1.56), (1.1, 0.3, 0.05), 1, bevel=0)
    box((2.35, -0.35, 1.66), (1.24, 1.54, 0.06), 3)
    for i in range(6):
        box((1.95 + (i % 3) * 0.4, -0.75 + (i // 3) * 0.7, 1.74), (0.32, 0.55, 0.03), 3, bevel=0)
    cyl((2.35, -0.35, 1.72), (2.35, -0.35, 2.5), 0.02, 5, verts=6)
    cone((2.35, -0.35, 2.56), 0.16, 0.02, 0.1, 5, verts=14)

    # Fibre entrance mast with guy lines and cable trays
    cyl((-0.2, 2.45, 0.2), (-0.2, 2.45, 2.6), 0.05, 5, verts=10)
    for i in range(3):
        a = i * math.tau / 3
        cyl((-0.2 + 0.9 * math.cos(a), 2.45 + 0.9 * math.sin(a), 0.2),
            (-0.2 + 0.18 * math.cos(a), 2.45 + 0.18 * math.sin(a), 2.3), 0.012, 5, verts=6)
    for i in range(4):
        box((-0.2, 2.45, 0.7 + i * 0.5), (0.30, 0.06, 0.06), 1, bevel=0)
    sphere((-0.2, 2.45, 2.68), 0.07, 7, segs=8, rings=6)

    container((-2.45, -0.6, 0.1), math.pi / 2, 2)
    container((-2.45, -1.0, 0.1), math.pi / 2, 0)
    pallet_stack((-1.4, -2.15, 0.2))
    for i in range(4):
        floodlight((-2.3 + i * 1.5, -2.15, 0.2), 0.85)
    fence_loop((0, 0, 0.2), 5.0, 3.6, 0.3, 14)


ASSETS = [
    ("colony_dome", build_colony_dome, 3.0, (1.8, -2.4, 1.4)),
    ("colony_factory", build_colony_factory, 3.4, (2.0, -2.3, 1.5)),
    ("colony_pad", build_colony_pad, 3.2, (2.0, -2.4, 1.6)),
    ("colony_nuclear", build_colony_nuclear, 4.2, (2.1, -2.3, 1.5)),
    ("colony_hydro", build_colony_hydro, 4.6, (2.2, -2.2, 1.4)),
    ("colony_thermal", build_colony_thermal, 4.6, (2.1, -2.4, 1.5)),
    ("colony_datacenter", build_colony_datacenter, 4.2, (2.0, -2.3, 1.5)),
]


def setup_showcase(scene, center, diag, cam_dir):
    scene.render.engine = "CYCLES"
    scene.cycles.device = "CPU"
    scene.cycles.samples = 48
    scene.cycles.use_denoising = True
    scene.render.resolution_x = 1280
    scene.render.resolution_y = 720
    scene.render.image_settings.file_format = "PNG"

    scene.world = bpy.data.worlds.new("PlanetarySky")
    scene.world.use_nodes = True
    bg = scene.world.node_tree.nodes.get("Background")
    bg.inputs["Color"].default_value = (0.012, 0.016, 0.028, 1)
    bg.inputs["Strength"].default_value = 0.6

    for name, pos, energy, color, size in (
        ("SunKey", (6, -8, 10), 3200, (1.0, 0.92, 0.78), 8),
        ("BlueRim", (-7, 6, 5), 2800, (0.35, 0.65, 1.0), 8),
        ("SoftFill", (1, -5, 4), 1000, (0.75, 0.85, 1.0), 10),
    ):
        data = bpy.data.lights.new(name, "AREA")
        data.energy, data.color, data.size = energy, color, size
        obj = bpy.data.objects.new(name, data)
        obj.location = pos
        obj.rotation_euler = (-Vector(pos)).to_track_quat("-Z", "Y").to_euler()
        scene.collection.objects.link(obj)

    # Frame the whole bounding volume: the earlier fixed origin target clipped
    # stacks and cooling towers out of the top of the shot.
    camera = bpy.data.cameras.new("ColonyCam")
    camera.type = "ORTHO"
    camera.ortho_scale = diag * 1.15
    cam = bpy.data.objects.new("ColonyCam", camera)
    scene.collection.objects.link(cam)
    d = Vector(cam_dir).normalized()
    cam.location = Vector(center) + d * (diag * 3.0)
    cam.rotation_euler = (-d).to_track_quat("-Z", "Y").to_euler()
    scene.camera = cam


def export_selection(objs, path):
    bpy.ops.object.select_all(action="DESELECT")
    for o in objs:
        o.select_set(True)
    bpy.context.view_layer.objects.active = objs[0]
    bpy.ops.wm.obj_export(filepath=str(path), export_selected_objects=True,
                          export_uv=True, export_normals=True,
                          export_materials=True, export_triangulated_mesh=True,
                          forward_axis="Y", up_axis="Z", path_mode="RELATIVE")


def bounds(obj):
    lo = Vector((1e9, 1e9, 1e9))
    hi = Vector((-1e9, -1e9, -1e9))
    for corner in obj.bound_box:
        w = Vector(corner)
        for i in range(3):
            lo[i] = min(lo[i], w[i])
            hi[i] = max(hi[i], w[i])
    return lo, hi


def main():
    argv = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
    only = set(argv[argv.index("--only") + 1:]) if "--only" in argv else set()
    MODELS.mkdir(parents=True, exist_ok=True)
    OUT.mkdir(parents=True, exist_ok=True)

    for name, builder, target, cam_dir in ASSETS:
        if only and not any(o in name for o in only):
            continue
        bpy.ops.wm.read_factory_settings(use_empty=True)
        global MATERIAL, PARTS
        PARTS = []
        MATERIAL = make_atlas(name)
        builder()

        bpy.ops.object.select_all(action="DESELECT")
        for obj in PARTS:
            obj.select_set(True)
        bpy.context.view_layer.objects.active = PARTS[0]
        bpy.ops.object.join()
        joined = bpy.context.object
        bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)

        lo, hi = bounds(joined)
        center = (lo + hi) / 2
        diag = (hi - lo).length
        factor = target / max(hi - lo)
        joined.scale = (factor, factor, factor)
        bpy.ops.object.transform_apply(scale=True)
        triangles = len(joined.data.polygons)
        export_selection([joined], MODELS / f"{name}.obj")

        scene = bpy.context.scene
        setup_showcase(scene, center * factor, diag * factor, cam_dir)
        scene.render.filepath = str(OUT / f"{name}_showcase.png")
        bpy.ops.render.render(write_still=True)
        print(f"DONE {name}: {triangles} triangles, size {target}", flush=True)

    print("ALL COLONY ASSETS DONE", flush=True)


if __name__ == "__main__":
    main()
