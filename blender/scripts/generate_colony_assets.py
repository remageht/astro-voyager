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
  colony_townhall   - civic hall: colonnade, pediment, clock tower, plaza
  colony_parliament - domed assembly: drum, wings, atrium, flag masts
  colony_police     - precinct: garage bays, sally port, radio mast, helipad
  colony_warehouse  - depot: long-span sheds, dock bays, container yard, rail
  colony_military   - base: perimeter towers, shelters, radar, vehicle park
  colony_houses     - private sector: gabled plots, PV, service drops, DHW
  colony_apartments - storey blocks with balconies, courtyard, DHW substation
  colony_pipeline   - gas/water trunk: loops, valve stations, pig traps, skid
  colony_powerline  - lattice pylons, sag spans, angle guys, end substation
  colony_solar      - tracker rows, inverter skids, combiners, met mast
  colony_diesel     - genset containers, day tanks, bund, bulk tank, switchgear
  colony_inverter   - BESS containers, PCS cabinets, fire main, control room
  colony_wind       - three HAWTs, pads, collector substation, met mast

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


def make_atlas(prefix, size=1024, roles=None, overrides=None):
    return asset_atlas.build(f"{prefix}_PBR", [CONCRETE] + list(PALETTE[1:]),
                             MODELS, prefix, size=size, seed_base=3000,
                             roles=roles or asset_atlas.ROLES_DEFAULT,
                             overrides=overrides)


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
    supports = max(1, int(supports))
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
                 0.055 + lvl * 0.012, 5, supports=max(1, int(length / 1.1)))
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
# Civic / housing / utility helpers
# ===========================================================================
def rot2(x, y, a):
    c, s = math.cos(a), math.sin(a)
    return (x * c - y * s, x * s + y * c)


def solid(verts, faces, center=(0, 0, 0), rot=(0, 0, 0), slot=0, bevel=0.004,
          smooth=False, name="Solid"):
    """from_pydata body with recalculated normals (same dance as lathe())."""
    mesh = bpy.data.meshes.new(name)
    mesh.from_pydata([Vector(v) for v in verts], [], faces)
    mesh.update()
    obj = bpy.data.objects.new(name, mesh)
    bpy.context.collection.objects.link(obj)
    obj.location = center
    obj.rotation_euler = rot
    bpy.ops.object.select_all(action="DESELECT")
    obj.select_set(True)
    bpy.context.view_layer.objects.active = obj
    bpy.ops.object.transform_apply(location=True, rotation=True)
    bpy.ops.object.mode_set(mode="EDIT")
    bpy.ops.mesh.select_all(action="SELECT")
    bpy.ops.mesh.normals_make_consistent(inside=False)
    bpy.ops.object.mode_set(mode="OBJECT")
    return finish(obj, slot, smooth=smooth, bevel=bevel)


def prism(center, w, d, h, slot=0, rot_z=0.0, ridge="y"):
    """Triangular prism: gable roofs, pediments, sawtooth sheds."""
    hw, hd = w / 2, d / 2
    if ridge == "y":
        verts = [(-hw, -hd, 0), (hw, -hd, 0), (hw, hd, 0), (-hw, hd, 0),
                 (0, -hd, h), (0, hd, h)]
        faces = [(3, 2, 1, 0), (0, 4, 5, 3), (1, 2, 5, 4), (0, 1, 4), (2, 3, 5)]
    else:
        verts = [(-hw, -hd, 0), (hw, -hd, 0), (hw, hd, 0), (-hw, hd, 0),
                 (-hw, 0, h), (hw, 0, h)]
        faces = [(3, 2, 1, 0), (0, 1, 5, 4), (2, 3, 4, 5), (3, 0, 4), (1, 2, 5)]
    return solid(verts, faces, center, (0, 0, rot_z), slot, name="Prism")


def blade(root, length, chord_root, chord_tip, thick, rot_x=0.0, slot=0, twist=0.4):
    """Tapered, twisted turbine blade. Radial axis is +Y rotated about X."""
    verts = []
    for (u, chord, ang) in ((0.0, chord_root, 0.0), (length, chord_tip, twist)):
        c, s = math.cos(ang), math.sin(ang)
        for (v, w) in ((-chord / 2, -thick / 2), (chord / 2, -thick / 2),
                       (chord / 2, thick / 2), (-chord / 2, thick / 2)):
            verts.append((v * c - w * s, u, v * s + w * c))
    faces = [(3, 2, 1, 0), (4, 5, 6, 7), (0, 1, 5, 4),
             (1, 2, 6, 5), (2, 3, 7, 6), (3, 0, 4, 7)]
    return solid(verts, faces, root, (rot_x, 0, 0), slot, bevel=0.002,
                 smooth=True, name="Blade")


def cable(p0, p1, sag=0.10, slot=1, segs=8, radius=0.010):
    """Parabolic sag span: conductors, pipe bridges, guy wires."""
    a, b = Vector(p0), Vector(p1)
    pts = []
    for i in range(segs + 1):
        t = i / segs
        p = a.lerp(b, t)
        p.z -= sag * 4.0 * t * (1.0 - t)
        pts.append(p)
    for i in range(segs):
        cyl(pts[i], pts[i + 1], radius, slot, verts=6)


def lattice_pylon(base, height, base_w, top_w, slot=5, arms=2):
    """HV lattice tower: tapered legs, rings, X bracing, arms, insulators."""
    bx, by, bz = base
    corners = ((-1, -1), (1, -1), (1, 1), (-1, 1))
    for sx, sy in corners:
        cyl((bx + sx * base_w / 2, by + sy * base_w / 2, bz),
            (bx + sx * top_w / 2, by + sy * top_w / 2, bz + height),
            0.020, slot, verts=6)
    panels = 6
    for k in range(panels + 1):
        t = k / panels
        w = base_w + (top_w - base_w) * t
        z = bz + height * t
        for i in range(4):
            sx, sy = corners[i]
            tx, ty = corners[(i + 1) % 4]
            cyl((bx + sx * w / 2, by + sy * w / 2, z),
                (bx + tx * w / 2, by + ty * w / 2, z), 0.012, slot, verts=5)
        if k < panels:
            t2 = (k + 1) / panels
            w2 = base_w + (top_w - base_w) * t2
            z2 = bz + height * t2
            for i in range(4):
                sx, sy = corners[i]
                tx, ty = corners[(i + 2) % 4]
                cyl((bx + sx * w / 2, by + sy * w / 2, z),
                    (bx + tx * w2 / 2, by + ty * w2 / 2, z2), 0.008, slot, verts=5)
    for i in range(arms):
        z = bz + height * (0.60 + 0.17 * i)
        aw = base_w * (1.7 - 0.3 * i)
        cyl((bx - aw / 2, by, z), (bx + aw / 2, by, z), 0.024, slot, verts=8)
        cyl((bx - aw / 2, by, z), (bx, by, z + 0.10), 0.012, slot, verts=5)
        cyl((bx + aw / 2, by, z), (bx, by, z + 0.10), 0.012, slot, verts=5)
        for sx in (-1, 1):
            x = bx + sx * aw / 2
            for d in range(4):
                cyl((x, by, z - 0.03 - d * 0.032), (x, by, z - 0.055 - d * 0.032),
                    0.032, 3, verts=10)
    cyl((bx, by, bz + height), (bx, by, bz + height + 0.12), 0.010, slot, verts=6)


def vehicle(center, rot_z=0.0, kind="car"):
    cx, cy, cz = center
    if kind == "truck":
        L, W, H = 0.66, 0.24, 0.20
    elif kind == "apc":
        L, W, H = 0.60, 0.26, 0.18
    else:
        L, W, H = 0.44, 0.20, 0.12
    box((cx, cy, cz + H * 0.5 + 0.05), (L, W, H), 1, rot_z)
    if kind == "truck":
        box((cx - L * 0.18, cy, cz + H + 0.10), (L * 0.55, W * 0.96, 0.16), 0, rot_z)
        box((cx + L * 0.30, cy, cz + H * 0.5 + 0.10), (L * 0.26, W * 0.9, 0.14), 3, rot_z)
    elif kind == "apc":
        box((cx, cy, cz + H + 0.06), (L * 0.5, W * 0.7, 0.08), 1, rot_z)
        cyl((cx, cy, cz + H + 0.10), (cx + L * 0.42, cy, cz + H + 0.12), 0.018, 5, verts=8)
    else:
        box((cx, cy, cz + H + 0.08), (L * 0.5, W * 0.9, 0.09), 3, rot_z)
        box((cx, cy, cz + H + 0.14), (0.10, W * 0.8, 0.025), 7, rot_z, bevel=0)
    wheels = (-L * 0.32, L * 0.32) if kind == "car" else (-L * 0.34, -L * 0.05, L * 0.30)
    for lx in wheels:
        for ly in (-W * 0.5, W * 0.5):
            ax, ay = rot2(lx, ly - 0.025, rot_z)
            bx, by = rot2(lx, ly + 0.025, rot_z)
            cyl((cx + ax, cy + ay, cz + 0.05), (cx + bx, cy + by, cz + 0.05),
                0.052, 6, verts=12)


def window_grid(center, w, h, cols, rows, slot=4, face="y", depth=0.02):
    cx, cy, cz = center
    cols = max(1, cols)
    rows = max(1, rows)
    for i in range(cols):
        for j in range(rows):
            u = (i + 0.5) / cols - 0.5
            v = (j + 0.5) / rows - 0.5
            if face == "y":
                box((cx + u * w, cy, cz + v * h),
                    (w / cols * 0.60, depth, h / rows * 0.52), slot, bevel=0)
            else:
                box((cx, cy + u * w, cz + v * h),
                    (depth, w / cols * 0.60, h / rows * 0.52), slot, bevel=0)


def balcony_grid(center, w, h, cols, rows, slot=5, face="y"):
    cx, cy, cz = center
    for i in range(cols):
        for j in range(rows):
            u = (i + 0.5) / cols - 0.5
            v = (j + 0.5) / rows - 0.5
            bw = w / cols * 0.72
            if face == "y":
                box((cx + u * w, cy, cz + v * h), (bw, 0.07, 0.015), slot, bevel=0)
                box((cx + u * w, cy - 0.035, cz + v * h + 0.045),
                    (bw, 0.012, 0.075), 3, bevel=0)
            else:
                box((cx, cy + u * w, cz + v * h), (0.07, bw, 0.015), slot, bevel=0)
                box((cx - 0.035, cy + u * w, cz + v * h + 0.045),
                    (0.012, bw, 0.075), 3, bevel=0)


def storey_block(center, floors, w, d, floor_h=0.26, slot=0, balconies=True):
    """Apartment block: floor bands, glazed facades, balconies, roof plant."""
    cx, cy, cz = center
    H = floors * floor_h
    box((cx, cy, cz + H / 2), (w, d, H), slot)
    for f in range(1, floors):
        box((cx, cy, cz + f * floor_h), (w + 0.015, d + 0.015, 0.018), 5, bevel=0)
    cols = max(2, int(w / 0.20))
    side = max(2, int(d / 0.22))
    window_grid((cx, cy - d / 2 - 0.008, cz + H / 2), w * 0.92, H * 0.84, cols, floors, 4, "y")
    window_grid((cx, cy + d / 2 + 0.008, cz + H / 2), w * 0.92, H * 0.84, cols, floors, 4, "y")
    window_grid((cx - w / 2 - 0.008, cy, cz + H / 2), d * 0.88, H * 0.84, side, floors, 4, "x")
    window_grid((cx + w / 2 + 0.008, cy, cz + H / 2), d * 0.88, H * 0.84, side, floors, 4, "x")
    if balconies:
        balcony_grid((cx, cy - d / 2 - 0.045, cz + H / 2), w * 0.9, H * 0.8,
                     max(1, cols // 2), floors - 1, 5, "y")
    box((cx, cy, cz + H + 0.025), (w + 0.03, d + 0.03, 0.05), 2)
    box((cx + w * 0.22, cy, cz + H + 0.13), (w * 0.20, d * 0.30, 0.16), 1)
    cyl((cx - w * 0.26, cy - d * 0.22, cz + H + 0.05),
        (cx - w * 0.26, cy - d * 0.22, cz + H + 0.24), 0.065, 5, verts=14)
    fan_unit((cx - w * 0.05, cy + d * 0.26, cz + H + 0.05), 0.055)
    box((cx, cy - d / 2 - 0.05, cz + 0.17), (0.30, 0.10, 0.025), 5)
    box((cx, cy - d / 2 - 0.006, cz + 0.10), (0.16, 0.02, 0.20), 3, bevel=0)
    for i in range(3):
        box((cx - 0.10 + i * 0.10, cy - d / 2 - 0.05, cz + 0.09), (0.012, 0.012, 0.18), 5, bevel=0)


def house(center, w=0.46, d=0.40, floors=1, roof_h=0.17, rot_z=0.0,
          wall=1, roof=2, porch=True, pv=False):
    """Detached house: siding walls, gable roof, chimney, porch, optional PV."""
    cx, cy, cz = center
    H = floors * 0.23
    box((cx, cy, cz + H / 2), (w, d, H), wall, rot_z)
    prism((cx, cy, cz + H), w * 1.10, d * 1.10, roof_h, roof, rot_z, ridge="y")
    dx, dy = rot2(w * 0.26, d * 0.18, rot_z)
    box((cx + dx, cy + dy, cz + H + roof_h * 0.55), (0.055, 0.055, roof_h * 1.1), 0, rot_z)
    window_grid((cx, cy - d / 2 - 0.006, cz + H * 0.55), w * 0.8, H * 0.5,
                2 if floors == 1 else 3, floors, 4, "y")
    window_grid((cx, cy + d / 2 + 0.006, cz + H * 0.55), w * 0.8, H * 0.5, 2, floors, 4, "y")
    fx, fy = rot2(0, -d / 2 - 0.006, rot_z)
    box((cx + fx, cy + fy, cz + 0.09), (0.09, 0.02, 0.18), 3, rot_z, bevel=0)
    if porch:
        px, py = rot2(0, -d / 2 - 0.09, rot_z)
        box((cx + px, cy + py, cz + 0.015), (w * 0.5, 0.16, 0.03), 0, rot_z)
        box((cx + px, cy + py, cz + 0.235), (w * 0.52, 0.18, 0.02), roof, rot_z)
        for sx in (-w * 0.22, w * 0.22):
            qx, qy = rot2(sx, -d / 2 - 0.15, rot_z)
            cyl((cx + qx, cy + qy, cz + 0.03), (cx + qx, cy + qy, cz + 0.23), 0.014, 5, verts=8)
    if pv:
        for i in range(2):
            sx = -w * 0.22 + i * w * 0.44
            qx, qy = rot2(sx, d * 0.16, rot_z)
            panel = box((cx + qx, cy + qy, cz + H + roof_h * 0.42),
                        (w * 0.34, d * 0.30, 0.012), 3, rot_z, bevel=0)
            panel.rotation_euler.x = -0.42
            bpy.ops.object.transform_apply(rotation=True)
    gx, gy = rot2(w * 0.62, 0, rot_z)
    box((cx + gx, cy + gy, cz + 0.07), (0.24, d * 0.8, 0.14), wall, rot_z)
    prism((cx + gx, cy + gy, cz + 0.14), 0.26, d * 0.84, 0.07, roof, rot_z, ridge="y")


def road_strip(p0, p1, width=0.30, z=0.012, slot=6):
    a, b = Vector(p0), Vector(p1)
    mid = (a + b) / 2
    ang = math.atan2(b.y - a.y, b.x - a.x)
    box((mid.x, mid.y, z), ((b - a).length, width, 0.024), slot, ang, bevel=0)


def lamp_post(center, height=0.46, slot=5):
    cx, cy, cz = center
    cyl((cx, cy, cz), (cx, cy, cz + height), 0.016, slot, verts=8)
    cyl((cx, cy, cz + height), (cx + 0.09, cy, cz + height), 0.012, slot, verts=6)
    box((cx + 0.11, cy, cz + height - 0.012), (0.07, 0.035, 0.018), 7, bevel=0)


def bollard(center, slot=5):
    cx, cy, cz = center
    cyl((cx, cy, cz), (cx, cy, cz + 0.11), 0.026, slot, verts=10)
    sphere((cx, cy, cz + 0.12), 0.026, 7, segs=8, rings=6)


def bench(center, rot_z=0.0):
    cx, cy, cz = center
    box((cx, cy, cz + 0.05), (0.20, 0.06, 0.015), 2, rot_z)
    box((cx, cy + 0.03, cz + 0.09), (0.20, 0.015, 0.07), 2, rot_z)
    for sx in (-0.08, 0.08):
        dx, dy = rot2(sx, 0, rot_z)
        box((cx + dx, cy + dy, cz + 0.025), (0.02, 0.05, 0.05), 1, rot_z)


def planter(center, rot_z=0.0):
    cx, cy, cz = center
    box((cx, cy, cz + 0.04), (0.22, 0.12, 0.08), 0, rot_z)
    box((cx, cy, cz + 0.085), (0.19, 0.09, 0.02), 6, rot_z, bevel=0)
    for i in range(3):
        dx, dy = rot2(-0.06 + i * 0.06, 0, rot_z)
        sphere((cx + dx, cy + dy, cz + 0.12), 0.045, 3, segs=10, rings=8, zscale=0.8)


def helipad(center, r=0.46):
    cx, cy, cz = center
    cyl((cx, cy, cz), (cx, cy, cz + 0.03), r, 6, verts=32)
    torus((cx, cy, cz + 0.035), r * 0.94, 0.018, 5, major_seg=32, minor_seg=6)
    box((cx - r * 0.22, cy, cz + 0.045), (0.05, r * 0.6, 0.012), 0, bevel=0)
    box((cx + r * 0.22, cy, cz + 0.045), (0.05, r * 0.6, 0.012), 0, bevel=0)
    box((cx, cy, cz + 0.045), (r * 0.44, 0.05, 0.012), 0, bevel=0)
    for i in range(8):
        a = i * math.tau / 8
        sphere((cx + r * 0.86 * math.cos(a), cy + r * 0.86 * math.sin(a), cz + 0.05),
               0.022, 7, segs=8, rings=6)


def radar_dish(center, r=0.26, tilt=0.55):
    cx, cy, cz = center
    profile = [(0.0, 0.0)]
    for i in range(1, 9):
        rr = r * i / 8
        profile.append((rr, 0.30 * (rr / r) ** 2 * r))
    dish = lathe(profile, (cx, cy, cz + 0.30), 5, segs=28)
    dish.rotation_euler.x = tilt
    bpy.ops.object.transform_apply(rotation=True)
    cyl((cx, cy, cz), (cx, cy, cz + 0.30), 0.035, 1, verts=12)
    cyl((cx, cy, cz + 0.30), (cx, cy + 0.16, cz + 0.44), 0.014, 5, verts=8)
    sphere((cx, cy + 0.17, cz + 0.45), 0.028, 7, segs=10, rings=8)
    box((cx, cy, cz + 0.06), (0.16, 0.16, 0.12), 0)


def watchtower(center, h=0.62):
    cx, cy, cz = center
    for sx, sy in ((-1, -1), (1, -1), (1, 1), (-1, 1)):
        cyl((cx + sx * 0.09, cy + sy * 0.09, cz), (cx + sx * 0.07, cy + sy * 0.07, cz + h),
            0.018, 5, verts=8)
    for k in range(3):
        z = cz + h * (k + 1) / 4
        box((cx, cy, z), (0.20, 0.20, 0.014), 5, bevel=0)
    box((cx, cy, cz + h + 0.09), (0.26, 0.26, 0.18), 0)
    window_grid((cx, cy - 0.135, cz + h + 0.10), 0.22, 0.10, 2, 1, 4, "y")
    window_grid((cx, cy + 0.135, cz + h + 0.10), 0.22, 0.10, 2, 1, 4, "y")
    box((cx, cy, cz + h + 0.20), (0.30, 0.30, 0.03), 2)
    railing((cx - 0.15, cy - 0.15, cz + h + 0.21), (cx + 0.15, cy - 0.15, cz + h + 0.21), 5, 4, 0.07)
    floodlight((cx + 0.10, cy - 0.10, cz + h + 0.22), 0.16)
    for i in range(6):
        box((cx - 0.11, cy + 0.10, cz + 0.08 + i * 0.09), (0.05, 0.012, 0.012), 5, bevel=0)


def barrier_gate(center, rot_z=0.0, arm=0.5):
    cx, cy, cz = center
    box((cx, cy, cz + 0.09), (0.10, 0.10, 0.18), 1, rot_z)
    dx, dy = rot2(arm / 2, 0, rot_z)
    box((cx + dx, cy + dy, cz + 0.16), (arm, 0.035, 0.035), 0, rot_z)
    for i in range(4):
        sx = arm * (0.15 + i * 0.22)
        ex, ey = rot2(sx, 0, rot_z)
        box((cx + ex, cy + ey, cz + 0.16), (arm * 0.10, 0.04, 0.04), 7, rot_z, bevel=0)


def solar_row(origin, length, panels=6, tilt=0.45, slot=3):
    """Single-axis tracker row: piers, torque tube, tilted PV modules."""
    ox, oy, oz = origin
    n = max(2, panels)
    for i in range(n + 1):
        x = ox + length * i / n
        cyl((x, oy, oz), (x, oy, oz + 0.16), 0.022, 5, verts=8)
        box((x, oy, oz + 0.02), (0.10, 0.10, 0.04), 0)
    cyl((ox, oy, oz + 0.17), (ox + length, oy, oz + 0.17), 0.026, 5, verts=10)
    pw = length / n * 0.94
    for i in range(n):
        x = ox + length * (i + 0.5) / n
        panel = box((x, oy, oz + 0.22), (pw, 0.30, 0.014), slot, bevel=0)
        panel.rotation_euler.x = tilt
        bpy.ops.object.transform_apply(rotation=True)
        box((x, oy, oz + 0.19), (pw * 0.9, 0.02, 0.02), 1, bevel=0)
    box((ox + length * 0.5, oy + 0.22, oz + 0.07), (0.14, 0.10, 0.14), 1)
    louvered_block((ox + length * 0.5, oy + 0.22, oz + 0.16), (0.12, 0.09, 0.05), 5, 4, "y")


def wind_turbine(base, height=1.5, rotor=0.52, yaw=0.0):
    """Three-blade HAWT: tapered tower, nacelle, hub, twisted blades, pad."""
    bx, by, bz = base
    cyl((bx, by, bz), (bx, by, bz + 0.05), 0.16, 0, verts=24)
    cone((bx, by, bz + height / 2), 0.075, 0.038, height, 0, verts=24)
    for i in range(3):
        z = bz + height * (0.25 + i * 0.25)
        torus((bx, by, z), 0.075 - 0.012 * i, 0.006, 5, major_seg=20, minor_seg=5)
    nx = bx + math.cos(yaw) * 0.10
    ny = by + math.sin(yaw) * 0.10
    box((nx, ny, bz + height + 0.03), (0.22, 0.09, 0.09), 0, yaw)
    box((nx - math.cos(yaw) * 0.10, ny - math.sin(yaw) * 0.10, bz + height + 0.05),
        (0.06, 0.07, 0.05), 1, yaw)
    hx = bx + math.cos(yaw) * 0.20
    hy = by + math.sin(yaw) * 0.20
    cone((hx, hy, bz + height + 0.03), 0.055, 0.02, 0.09, 5, verts=16)
    for k in range(3):
        ang = yaw + math.pi / 2 + k * math.tau / 3
        root = (hx + math.cos(ang) * 0.05, hy + math.sin(ang) * 0.05, bz + height + 0.03)
        blade(root, rotor, 0.075, 0.022, 0.016, rot_x=ang - math.pi / 2, slot=0, twist=0.5)
    cyl((bx, by, bz + 0.05), (bx, by, bz + height * 0.4), 0.012, 5, verts=6)
    box((bx + 0.14, by, bz + 0.05), (0.10, 0.08, 0.10), 1)


def tank_horizontal(center, r, length, slot=2, rot_z=0.0):
    cx, cy, cz = center
    ax, ay = rot2(-length / 2, 0, rot_z)
    bx, by = rot2(length / 2, 0, rot_z)
    cyl((cx + ax, cy + ay, cz + r), (cx + bx, cy + by, cz + r), r, slot, verts=20)
    sphere((cx + ax, cy + ay, cz + r), r, slot, segs=14, rings=10)
    sphere((cx + bx, cy + by, cz + r), r, slot, segs=14, rings=10)
    for sx in (-length * 0.3, length * 0.3):
        dx, dy = rot2(sx, 0, rot_z)
        box((cx + dx, cy + dy, cz + r * 0.4), (0.06, r * 1.6, r * 0.8), 1, rot_z)


# ===========================================================================
# Reference-driven detail helpers (детали по натурным референсам)
# ===========================================================================
def hesco_row(p0, p1, tiers=2, slot=0):
    """Concertainer / HESCO bastion: stacked mesh boxes with soil crests."""
    a, b = Vector(p0), Vector(p1)
    length = (b - a).length
    n = max(1, int(length / 0.22))
    for i in range(n):
        t = (i + 0.5) / n
        p = a.lerp(b, t)
        ang = math.atan2(b.y - a.y, b.x - a.x)
        for k in range(tiers):
            z = 0.11 + k * 0.20
            box((p.x, p.y, z), (0.20, 0.16, 0.19), slot, ang)
            for m in range(3):                              # mesh corrugation
                off = -0.07 + m * 0.07
                dx, dy = rot2(0, off, ang)
                box((p.x + dx, p.y + dy, z), (0.012, 0.17, 0.19), 5, ang, bevel=0)
        top = 0.11 + tiers * 0.20
        box((p.x, p.y, top + 0.02), (0.21, 0.17, 0.04), 6, ang, bevel=0)


def dormer(center, w=0.12, h=0.09, rot_z=0.0, roof_slot=2):
    """Roof dormer: cheeks, gable cap, glazed front."""
    cx, cy, cz = center
    box((cx, cy, cz + h * 0.5), (w, w * 0.9, h), 1, rot_z)
    prism((cx, cy, cz + h), w * 1.15, w * 0.95, h * 0.5, roof_slot, rot_z, ridge="x")
    dx, dy = rot2(0, -w * 0.48, rot_z)
    box((cx + dx, cy + dy, cz + h * 0.55), (w * 0.62, 0.02, h * 0.62), 4, rot_z, bevel=0)


def modillion_cornice(center, w, d, count=14, slot=0):
    """Repeated brackets under the cornice edge."""
    cx, cy, cz = center
    for i in range(count):
        u = (i + 0.5) / count - 0.5
        for face in (-1, 1):
            box((cx + u * w, cy + face * (d / 2 - 0.02), cz), (0.028, 0.05, 0.05), slot, bevel=0)
        if abs(u) < 0.48:
            box((cx + u * w, cy - (d / 2 - 0.02), cz), (0.028, 0.05, 0.05), slot, bevel=0)


def balustrade(p0, p1, height=0.14, posts=14, slot=0):
    a, b = Vector(p0), Vector(p1)
    ang = math.atan2(b.y - a.y, b.x - a.x)
    length = (b - a).length
    box((a.x, a.y, 0), (length, 0.05, 0.03), slot, ang, bevel=0)
    box(((a.x + b.x) / 2, (a.y + b.y) / 2, height), (length, 0.05, 0.03), slot, ang, bevel=0)
    for i in range(posts):
        t = (i + 0.5) / posts
        p = a.lerp(b, t)
        box((p.x, p.y, height * 0.5), (0.022, 0.022, height), slot, ang, bevel=0.002)


def window_hood(center, w, h=0.05, face="y", slot=0):
    cx, cy, cz = center
    if face == "y":
        box((cx, cy, cz + h), (w * 1.18, 0.035, 0.022), slot, bevel=0)
        for sx in (-w * 0.55, w * 0.55):
            box((cx + sx, cy, cz + h * 0.4), (0.022, 0.04, h * 0.9), slot, bevel=0)
    else:
        box((cx, cy, cz + h), (0.035, w * 1.18, 0.022), slot, bevel=0)


def arched_opening(center, w, h, face="y", slot=3):
    """Arcuated window/door head with keystones."""
    cx, cy, cz = center
    if face == "y":
        box((cx, cy, cz - h * 0.35), (w * 0.86, 0.04, h * 0.7), slot, bevel=0)
        torus((cx, cy, cz), w * 0.43, 0.035, 0, major_seg=18, minor_seg=6)
        for i in range(5):
            a = math.pi * (0.15 + 0.7 * i / 4)
            box((cx + w * 0.43 * math.cos(a), cy, cz + w * 0.43 * math.sin(a)),
                (0.03, 0.05, 0.05), 0, bevel=0)
    else:
        box((cx, cy, cz - h * 0.35), (0.04, w * 0.86, h * 0.7), slot, bevel=0)
        torus((cx, cy, cz), w * 0.43, 0.035, 0, major_seg=18, minor_seg=6)


def dock_seal(center, rot_z=0.0):
    """Foam dock seal/shelter frame plus bumper and wheel chocks."""
    cx, cy, cz = center
    for sx in (-0.24, 0.24):
        dx, dy = rot2(sx, 0, rot_z)
        box((cx + dx, cy + dy, cz + 0.46), (0.07, 0.09, 0.62), 2, rot_z)
    dx, dy = rot2(0, 0, rot_z)
    box((cx + dx, cy + dy, cz + 0.80), (0.55, 0.09, 0.07), 2, rot_z)
    box((cx, cy - 0.06, cz + 0.16), (0.62, 0.05, 0.10), 6, rot_z, bevel=0)  # bumper
    for sx in (-0.14, 0.14):
        dx, dy = rot2(sx, -0.10, rot_z)
        box((cx + dx, cy + dy, cz + 0.05), (0.06, 0.10, 0.06), 7, rot_z, bevel=0)


def camera_mast(center, height=0.7, heads=2):
    cx, cy, cz = center
    cyl((cx, cy, cz), (cx, cy, cz + height), 0.022, 5, verts=10)
    for i in range(heads):
        a = i * math.tau / max(1, heads)
        dx, dy = rot2(0.16, 0, a)
        cyl((cx, cy, cz + height), (cx + dx, cy + dy, cz + height - 0.02), 0.012, 5, verts=6)
        box((cx + dx * 1.1, cy + dy * 1.1, cz + height - 0.05), (0.06, 0.045, 0.04), 1, a)
        cyl((cx + dx * 1.1, cy + dy * 1.1, cz + height - 0.075),
            (cx + dx * 1.2, cy + dy * 1.2, cz + height - 0.075), 0.02, 3, verts=10)


def helicopter(center, rot_z=0.0):
    """Parked utility helicopter for the pad: fuselage, tail, skids, rotor."""
    cx, cy, cz = center
    box((cx, cy, cz + 0.10), (0.30, 0.16, 0.15), 1, rot_z)
    sphere((cx - 0.16, cy, cz + 0.11), 0.085, 3, segs=12, rings=8)
    dx, dy = rot2(0.24, 0, rot_z)
    box((cx + dx, cy + dy, cz + 0.14), (0.28, 0.05, 0.05), 1, rot_z)
    dx, dy = rot2(0.36, 0, rot_z)
    box((cx + dx, cy + dy, cz + 0.20), (0.03, 0.16, 0.11), 1, rot_z)
    cyl((cx + dx, cy + dy, cz + 0.18), (cx + dx + 0.10, cy + dy, cz + 0.20), 0.008, 5, verts=6)
    cyl((cx, cy, cz + 0.20), (cx, cy, cz + 0.24), 0.022, 5, verts=10)
    for k in range(4):
        a = rot_z + k * math.tau / 4
        ex, ey = rot2(0.30 * math.cos(a), 0.30 * math.sin(a), 0.0)
        box((cx + ex, cy + ey, cz + 0.25), (0.30, 0.03, 0.008), 0, a, bevel=0)
    for sy in (-0.10, 0.10):
        sx, sy2 = rot2(0, sy, rot_z)
        box((cx + sx, cy + sy2, cz + 0.02), (0.34, 0.02, 0.02), 5, rot_z, bevel=0)
        for ux in (-0.12, 0.12):
            vx, vy = rot2(ux, sy, rot_z)
            cyl((cx + vx, cy + vy, cz + 0.03), (cx + vx, cy + vy, cz + 0.10), 0.012, 5, verts=6)


def insulator_string(center, drop=0.14, discs=4, slot=3):
    cx, cy, cz = center
    for i in range(discs):
        cyl((cx, cy, cz - i * drop / discs), (cx, cy, cz - (i + 0.8) * drop / discs),
            0.030, slot, verts=12)
    cyl((cx, cy, cz - drop), (cx, cy, cz - drop - 0.02), 0.012, 5, verts=8)


def slewing_drive(center, rot_z=0.0):
    cx, cy, cz = center
    box((cx, cy, cz + 0.05), (0.14, 0.12, 0.10), 1, rot_z)
    cyl((cx, cy, cz + 0.10), (cx, cy, cz + 0.18), 0.045, 5, verts=14)
    torus((cx, cy, cz + 0.16), 0.055, 0.012, 5, major_seg=16, minor_seg=6)
    box((cx + 0.09, cy, cz + 0.22), (0.05, 0.06, 0.10), 0, rot_z)   # controller


def blade_root_bolt_circle(center, r=0.05, n=10, rot_x=0.0):
    """Ring of root bolts on a blade flange whose axis runs along +X."""
    cx, cy, cz = center
    for i in range(n):
        a = rot_x + i * math.tau / n
        oy, oz = r * math.cos(a), r * math.sin(a)
        cyl((cx, cy + oy, cz + oz), (cx + 0.02, cy + oy, cz + oz), 0.007, 5, verts=6)


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


# ===========================================================================
# 8. Town hall (ратуша): colonnade, pediment, clock tower, civic plaza
# ===========================================================================
def build_colony_townhall():
    box((0, 0, 0.06), (4.4, 3.6, 0.12), 6)                 # plaza
    road_strip((-2.2, -1.5, 0.13), (2.2, -1.5, 0.13), 0.34, 0.13)
    box((0, 0.45, 0.20), (2.6, 1.7, 0.16), 0)              # stylobate
    for i in range(3):                                      # entrance steps
        box((0, -0.42 - i * 0.10, 0.16 - i * 0.04), (1.5 + i * 0.12, 0.10, 0.05), 0)
    box((0, 0.55, 0.86), (2.3, 1.4, 1.16), 1)              # main masonry block
    for z in (0.42, 1.30):                                  # string courses
        box((0, 0.55, z), (2.36, 1.46, 0.05), 0)
    box((0, 0.55, 1.48), (2.42, 1.52, 0.10), 0)            # cornice
    for sx in (-1, 1):                                      # quoins
        for i in range(6):
            box((sx * 1.14, 0.55 - 0.62 + i * 0.25, 0.40 + i * 0.02),
                (0.10, 0.14, 0.16), 0, bevel=0.003)
    window_grid((0, -0.16, 0.92), 2.0, 0.72, 5, 2, 4, "y")
    window_grid((0, 1.26, 0.92), 2.0, 0.72, 5, 2, 4, "y")
    window_grid((-1.16, 0.55, 0.92), 1.2, 0.72, 3, 2, 4, "x")
    window_grid((1.16, 0.55, 0.92), 1.2, 0.72, 3, 2, 4, "x")
    prism((0, 0.55, 1.53), 2.34, 1.44, 0.22, 2, ridge="x")  # roof
    box((0, 0.55, 1.78), (0.5, 0.4, 0.16), 3)              # roof lantern
    for i in range(6):                                      # portico columns
        x = -0.85 + i * 0.34
        cyl((x, -0.30, 0.28), (x, -0.30, 1.20), 0.055, 0, verts=16)
        box((x, -0.30, 1.24), (0.14, 0.14, 0.06), 0)
        box((x, -0.30, 0.30), (0.15, 0.15, 0.06), 0)
    box((0, -0.30, 1.32), (2.1, 0.30, 0.10), 0)            # entablature
    prism((0, -0.30, 1.42), 2.14, 0.32, 0.30, 0, ridge="x")  # pediment
    sphere((0, -0.30, 1.62), 0.06, 7, segs=10, rings=8)
    box((0, -0.14, 0.62), (0.44, 0.05, 0.62), 3)           # doors
    box((0, -0.17, 1.02), (0.6, 0.04, 0.06), 7, bevel=0)   # signage light
    railing((-0.95, -0.52, 0.24), (-0.55, -0.52, 0.24), 5, 4, 0.10)
    railing((0.55, -0.52, 0.24), (0.95, -0.52, 0.24), 5, 4, 0.10)

    # Clock tower with belfry, dial, pyramidal cap and flag
    box((-1.55, 0.75, 1.05), (0.52, 0.52, 1.9), 1)
    for z in (0.6, 1.5, 1.95):
        box((-1.55, 0.75, z), (0.58, 0.58, 0.05), 0)
    louvered_block((-1.55, 0.75, 1.72), (0.44, 0.44, 0.30), 1, 5, "y")
    torus((-1.55, 0.48, 1.28), 0.16, 0.022, 5, major_seg=24, minor_seg=6)
    cyl((-1.55, 0.47, 1.28), (-1.55, 0.45, 1.28), 0.14, 0, verts=20)
    box((-1.55, 0.44, 1.34), (0.015, 0.02, 0.11), 1, bevel=0)
    box((-1.50, 0.44, 1.28), (0.09, 0.02, 0.015), 1, bevel=0)
    cone((-1.55, 0.75, 2.16), 0.40, 0.02, 0.34, 2, verts=4)
    cyl((-1.55, 0.75, 2.33), (-1.55, 0.75, 2.72), 0.014, 5, verts=8)
    box((-1.44, 0.75, 2.62), (0.22, 0.012, 0.13), 7, bevel=0)
    sphere((-1.55, 0.75, 2.75), 0.035, 7, segs=8, rings=6)

    for x in (-1.9, 1.9):                                   # plaza furniture
        lamp_post((x, -1.15, 0.12), 0.52)
        bench((x * 0.6, -1.05, 0.12), 0.0)
        planter((x * 0.35, -1.25, 0.12))
    for i in range(7):
        bollard((-1.5 + i * 0.5, -1.72, 0.12))
    for sx in (-1, 1):
        cyl((sx * 1.35, -1.0, 0.12), (sx * 1.35, -1.0, 1.0), 0.016, 5, verts=8)
        box((sx * 1.35 + 0.09, -1.0, 0.90), (0.18, 0.012, 0.11), 4, bevel=0)
    floodlight((-2.0, 1.5, 0.12), 0.8)
    floodlight((2.0, 1.5, 0.12), 0.8)
    container((1.85, 1.35, 0.16), 0.3, 1)

    # Detail pass (reference: Georgian-revival city halls - rusticated base,
    # modillion cornice, projecting porch, window balconies, dormer roof).
    for i in range(7):                                      # rusticated ground courses
        box((0, 0.55, 0.30 + i * 0.055), (2.34 - i * 0.01, 1.44, 0.022), 0, bevel=0)
    modillion_cornice((0, 0.55, 1.42), 2.3, 1.44, 16)       # brackets under cornice
    for i in range(4):                                      # grand central staircase
        box((0, -0.62 - i * 0.13, 0.20 - i * 0.045), (1.9 + i * 0.2, 0.13, 0.05), 0)
    balustrade((-0.95, -0.66, 0.20), (-0.95, -1.02, 0.20), 0.13, 6)
    balustrade((0.95, -0.66, 0.20), (0.95, -1.02, 0.20), 0.13, 6)
    for i in range(5):                                      # window hoods + balconettes
        x = -0.8 + i * 0.4
        window_hood((x, -0.16, 1.12), 0.30, 0.05, "y")
        box((x, -0.20, 1.06), (0.34, 0.05, 0.02), 5, bevel=0)
        railing((x - 0.15, -0.24, 1.06), (x + 0.15, -0.24, 1.06), 5, 4, 0.07)
    arched_opening((-0.85, -0.20, 0.62), 0.34, 0.5, "y")    # rusticated arched bay
    arched_opening((0.85, -0.20, 0.62), 0.34, 0.5, "y")
    for i in range(3):                                      # roof dormers
        dormer((-0.6 + i * 0.6, -0.10, 1.62), 0.14, 0.10)
    box((0, 0.55, 1.60), (2.44, 1.5, 0.05), 0, bevel=0)     # mansard band
    balustrade((-1.15, -0.16, 1.52), (1.15, -0.16, 1.52), 0.11, 18)  # roof balustrade
    cyl((-1.55, 0.42, 1.28), (-1.55, 0.42, 1.30), 0.15, 0, verts=20)  # dial on second face
    torus((-1.55, 0.42, 1.28), 0.16, 0.022, 5, major_seg=24, minor_seg=6)
    for sx in (-1, 1):                                      # porch lamp brackets
        dx, dy = rot2(sx * 0.34, -0.34, 0.0)
        cyl((dx, 0.55 + dy, 0.98), (dx, 0.55 + dy - 0.06, 0.90), 0.010, 5, verts=6)
        sphere((dx, 0.55 + dy - 0.07, 0.88), 0.035, 7, segs=8, rings=6)


# ===========================================================================
# 9. Parliament (госдума): dome on a drum, colonnaded wings, glass atrium
# ===========================================================================
def build_colony_parliament():
    box((0, 0, 0.06), (4.8, 3.8, 0.12), 6)
    road_strip((-2.4, -1.6, 0.13), (2.4, -1.6, 0.13), 0.36, 0.13)
    box((0, 0.35, 0.22), (3.6, 2.0, 0.20), 0)
    for i in range(4):
        box((0, -0.72 - i * 0.09, 0.18 - i * 0.035), (2.0 + i * 0.14, 0.09, 0.045), 0)

    for sx in (-1, 1):                                      # flanking wings
        box((sx * 1.30, 0.45, 0.80), (1.10, 1.5, 1.0), 1)
        box((sx * 1.30, 0.45, 1.34), (1.18, 1.58, 0.09), 0)
        prism((sx * 1.30, 0.45, 1.43), 1.14, 1.54, 0.16, 2, ridge="y")
        window_grid((sx * 1.30, -0.31, 0.82), 0.95, 0.66, 3, 2, 4, "y")
        window_grid((sx * 1.30, 1.21, 0.82), 0.95, 0.66, 3, 2, 4, "y")
        for i in range(4):                                  # wing colonnade
            x = sx * 1.30 - 0.42 + i * 0.28
            cyl((x, -0.42, 0.32), (x, -0.42, 1.10), 0.042, 0, verts=14)
            box((x, -0.42, 1.14), (0.11, 0.11, 0.05), 0)
        box((sx * 1.30, -0.42, 1.20), (1.16, 0.22, 0.07), 0)

    box((0, 0.45, 0.95), (1.30, 1.5, 1.30), 1)              # central mass
    window_grid((0, -0.31, 0.95), 1.1, 0.9, 3, 2, 4, "y")
    for i in range(8):                                      # grand portico
        x = -0.63 + i * 0.18
        cyl((x, -0.55, 0.32), (x, -0.55, 1.42), 0.05, 0, verts=16)
        box((x, -0.55, 1.46), (0.13, 0.13, 0.06), 0)
    box((0, -0.55, 1.54), (1.6, 0.30, 0.10), 0)
    prism((0, -0.55, 1.64), 1.64, 0.32, 0.34, 0, ridge="x")
    box((0, -0.28, 0.72), (0.5, 0.05, 0.72), 3)

    cyl((0, 0.45, 1.60), (0, 0.45, 1.95), 0.62, 0, verts=32)  # drum
    for i in range(12):
        a = i * math.tau / 12
        cyl((0.60 * math.cos(a), 0.45 + 0.60 * math.sin(a), 1.62),
            (0.60 * math.cos(a), 0.45 + 0.60 * math.sin(a), 1.94), 0.035, 0, verts=10)
    torus((0, 0.45, 1.96), 0.66, 0.035, 5, major_seg=36, minor_seg=8)
    hemisphere((0, 0.45, 1.96), 0.62, 2, segs=32, rings=16, zscale=0.86)
    for i in range(8):                                      # dome ribs
        a = i * math.tau / 8
        cyl((0.60 * math.cos(a), 0.45 + 0.60 * math.sin(a), 1.98),
            (0.10 * math.cos(a), 0.45 + 0.10 * math.sin(a), 2.46), 0.022, 5, verts=8)
    cyl((0, 0.45, 2.44), (0, 0.45, 2.62), 0.10, 0, verts=16)
    cone((0, 0.45, 2.70), 0.13, 0.02, 0.16, 2, verts=16)
    cyl((0, 0.45, 2.78), (0, 0.45, 2.98), 0.014, 5, verts=8)
    sphere((0, 0.45, 3.02), 0.05, 7, segs=10, rings=8)

    box((0, 1.35, 0.62), (1.0, 0.5, 0.9), 3)                # glass atrium link
    for i in range(5):
        box((-0.45 + i * 0.22, 1.35, 0.62), (0.02, 0.52, 0.92), 5, bevel=0)
    box((0, 1.35, 1.10), (1.06, 0.56, 0.06), 2)

    for i, x in enumerate((-0.9, 0.0, 0.9)):                # flag masts
        cyl((x, -1.25, 0.12), (x, -1.25, 1.15), 0.016, 5, verts=8)
        box((x + 0.10, -1.25, 1.02), (0.20, 0.012, 0.12), (4, 7, 4)[i], bevel=0)
    for i in range(9):
        bollard((-1.8 + i * 0.45, -1.78, 0.12))
    for x in (-2.1, 2.1):
        lamp_post((x, -1.2, 0.12), 0.55)
        bench((x * 0.72, -1.15, 0.12))
        planter((x * 0.45, -1.35, 0.12))
    barrier_gate((-2.0, -1.6, 0.12), 0.0, 0.45)
    barrier_gate((2.0, -1.6, 0.12), math.pi, 0.45)
    floodlight((-2.2, 1.6, 0.12), 0.85)
    floodlight((2.2, 1.6, 0.12), 0.85)
    switchyard((1.95, 1.45, 0.12), rows=1, bays=2, span=0.3)

    # Detail pass (reference: Westminster-type assembly halls - corner towers,
    # traceried windows, roof balustrade, terrace steps, central statue).
    for sx, sy in ((-1.85, -0.30), (1.85, -0.30), (-1.85, 1.20), (1.85, 1.20)):
        box((sx, sy, 0.85), (0.34, 0.34, 1.30), 1)
        box((sx, sy, 1.55), (0.40, 0.40, 0.10), 0)
        cone((sx, sy, 1.74), 0.24, 0.02, 0.30, 2, verts=4)
        cyl((sx, sy, 1.92), (sx, sy, 2.06), 0.014, 5, verts=6)
        sphere((sx, sy, 2.10), 0.035, 7, segs=8, rings=6)
        window_grid((sx, sy - 0.18, 0.95), 0.24, 0.7, 1, 2, 4, "y")
    for sx in (-1, 1):                                      # portico arches
        for i in range(3):
            arched_opening((-0.42 + i * 0.42, -0.58, 1.02), 0.34, 0.6, "y")
    for i in range(7):                                      # tracery mullions, wings
        for sx in (-1, 1):
            box((sx * 1.30 - 0.45 + (i % 4) * 0.30, -0.44, 0.62 + (i // 4) * 0.42),
                (0.018, 0.02, 0.62), 5, bevel=0)
    balustrade((-1.72, -0.30, 1.50), (1.72, -0.30, 1.50), 0.12, 26)
    for i in range(6):                                      # terrace steps
        box((0, -0.95 - i * 0.11, 0.20 - i * 0.033), (2.6 + i * 0.18, 0.11, 0.045), 0)
    box((0, -1.42, 0.22), (0.44, 0.44, 0.24), 0)            # statue plinth
    cyl((0, -1.42, 0.42), (0, -1.42, 0.72), 0.05, 1, verts=12)
    sphere((0, -1.42, 0.78), 0.06, 1, segs=12, rings=8)
    box((0, -1.42, 0.90), (0.02, 0.16, 0.20), 5, bevel=0)
    for sx in (-1, 1):                                      # flanking lamp standards
        for i in range(3):
            lamp_post((sx * (0.9 + i * 0.45), -1.62, 0.12), 0.52)
    for sx in (-1, 1):                                      # security gate piers
        box((sx * 2.35, -1.72, 0.30), (0.16, 0.16, 0.42), 0)
        box((sx * 2.35, -1.72, 0.55), (0.20, 0.20, 0.08), 2)


# ===========================================================================
# 10. Police precinct: garage bays, sally port, radio mast, patrol yard
# ===========================================================================
def build_colony_police():
    box((0, 0, 0.06), (4.4, 3.6, 0.12), 6)
    road_strip((-2.2, -1.45, 0.13), (2.2, -1.45, 0.13), 0.36, 0.13)
    box((-0.55, 0.55, 0.72), (2.0, 1.5, 1.2), 1)            # precinct block
    box((-0.55, 0.55, 1.36), (2.08, 1.58, 0.09), 0)
    box((-0.55, 0.55, 1.44), (1.9, 1.4, 0.10), 2)
    for f in range(2):
        window_grid((-0.55, -0.21, 0.45 + f * 0.55), 1.8, 0.34, 6, 1, 4, "y")
        window_grid((-0.55, 1.31, 0.45 + f * 0.55), 1.8, 0.34, 6, 1, 4, "y")
    window_grid((-1.56, 0.55, 0.72), 1.3, 0.9, 4, 2, 4, "x")
    box((-0.55, -0.22, 0.30), (0.5, 0.05, 0.5), 3)          # entrance
    box((-0.55, -0.25, 0.62), (0.7, 0.04, 0.07), 4, bevel=0)  # sign band
    box((-0.55, -0.25, 0.70), (0.7, 0.04, 0.03), 7, bevel=0)
    cyl((-1.45, -0.35, 0.12), (-1.45, -0.35, 1.05), 0.016, 5, verts=8)
    box((-1.36, -0.35, 0.95), (0.18, 0.012, 0.11), 4, bevel=0)

    box((1.35, 0.55, 0.52), (1.5, 1.5, 0.8), 0)             # garage bay
    box((1.35, 0.55, 0.96), (1.58, 1.58, 0.08), 2)
    for i in range(3):                                      # roll-up doors
        x = 0.85 + i * 0.5
        box((x, -0.21, 0.42), (0.42, 0.04, 0.62), 1)
        for k in range(7):
            box((x, -0.235, 0.16 + k * 0.085), (0.42, 0.012, 0.02), 5, bevel=0)
        box((x, -0.24, 0.78), (0.44, 0.03, 0.04), 7, bevel=0)
    box((1.35, -0.55, 0.14), (1.6, 0.6, 0.03), 6, bevel=0)  # apron
    for i in range(4):
        box((0.75 + i * 0.4, -0.55, 0.16), (0.03, 0.5, 0.012), 0, bevel=0)

    fence_loop((1.35, -1.15, 0.12), 1.7, 0.9, 0.30, posts=12)  # sally port
    vehicle((0.95, -1.15, 0.12), 0.2, "car")
    vehicle((1.55, -1.25, 0.12), -0.1, "car")
    vehicle((1.95, -1.05, 0.12), 0.4, "truck")
    barrier_gate((0.55, -1.15, 0.12), math.pi / 2, 0.4)

    cyl((-1.85, 1.35, 0.12), (-1.85, 1.35, 1.9), 0.035, 5, verts=10)  # radio mast
    for i in range(4):
        z = 0.5 + i * 0.4
        cyl((-1.85, 1.35, z), (-1.85 + 0.18, 1.35, z + 0.1), 0.010, 5, verts=6)
        cyl((-1.85, 1.35, z), (-1.85 - 0.18, 1.35, z + 0.1), 0.010, 5, verts=6)
    radar_dish((-1.85, 1.35, 1.55), 0.18, 0.5)
    for i in range(3):
        cyl((-1.85 + 0.10 * (i - 1), 1.35, 1.9), (-1.85 + 0.10 * (i - 1), 1.35, 2.15),
            0.010, 5, verts=6)
    sphere((-1.85, 1.35, 2.18), 0.035, 7, segs=8, rings=6)
    for i in range(3):
        guy_a = i * math.tau / 3
        cable((-1.85, 1.35, 1.8), (-1.85 + 0.5 * math.cos(guy_a), 1.35 + 0.5 * math.sin(guy_a), 0.12),
              0.02, 5, segs=4, radius=0.006)

    helipad((1.55, 1.45, 0.12), 0.42)
    for i in range(6):
        bollard((-1.6 + i * 0.5, -1.72, 0.12))
    lamp_post((-2.0, -1.0, 0.12), 0.5)
    lamp_post((2.05, -0.9, 0.12), 0.5)
    floodlight((-2.05, 1.6, 0.12), 0.8)
    floodlight((2.1, 0.0, 0.12), 0.8)
    container((-2.0, 0.2, 0.16), 0.2, 1)
    planter((-0.2, -1.0, 0.12))
    bench((0.2, -1.05, 0.12))

    # Detail pass (reference: modern precinct design - interlocking sally port
    # inside the fenced envelope, prisoner transfer canopy, wash bay, blast
    # planters and bollards, separated visitor/staff parking, ID totem).
    for sx in (-0.55, 0.55):                                # interlocking gate leaves
        box((1.35 + sx, -1.60, 0.24), (0.06, 0.06, 0.36), 5)
        for i in range(4):
            box((1.35 + sx, -1.60, 0.12 + i * 0.09), (0.05, 0.05, 0.02), 1, bevel=0)
    box((1.35, -1.60, 0.44), (1.3, 0.05, 0.05), 5, bevel=0)
    box((1.35, -0.30, 0.90), (0.9, 0.7, 0.05), 2)           # prisoner transfer canopy
    for sx in (-0.4, 0.4):
        box((1.35 + sx, -0.30, 0.45), (0.05, 0.05, 0.9), 5, bevel=0)
    box((2.05, -0.85, 0.13), (0.5, 0.42, 0.03), 6, bevel=0)  # vehicle wash bay
    for i in range(3):
        cyl((1.9 + i * 0.15, -0.85, 0.16), (1.9 + i * 0.15, -0.85, 0.44), 0.012, 5, verts=8)
        cyl((1.9 + i * 0.15, -0.85, 0.44), (1.9 + i * 0.15, -0.72, 0.44), 0.010, 5, verts=6)
    box((0.05, -1.30, 0.55), (0.34, 0.10, 0.9), 1)          # ID signage totem
    box((0.05, -1.36, 0.72), (0.28, 0.02, 0.42), 4, bevel=0)
    box((0.05, -1.36, 0.30), (0.28, 0.02, 0.10), 7, bevel=0)
    for i in range(5):                                       # raised blast planters
        box((-1.9 + i * 0.42, -1.62, 0.20), (0.34, 0.30, 0.22), 0)
        box((-1.9 + i * 0.42, -1.62, 0.33), (0.30, 0.26, 0.05), 6, bevel=0)
        for k in range(2):
            sphere((-1.98 + i * 0.42 + k * 0.16, -1.62, 0.40), 0.07, 3, segs=10, rings=8)
    for i in range(9):                                       # decorative bollard line
        bollard((-1.7 + i * 0.42, -1.95, 0.12))
    box((2.05, 0.30, 0.14), (0.6, 0.4, 0.06), 0)            # fuel island
    for sx in (-0.22, 0.22):
        cyl((2.05 + sx, 0.30, 0.17), (2.05 + sx, 0.30, 0.42), 0.03, 1, verts=10)
    box((2.05, 0.30, 0.52), (0.66, 0.5, 0.04), 2)
    camera_mast((-1.35, -1.72, 0.12), 0.72, 2)
    camera_mast((2.25, -1.72, 0.12), 0.66, 1)
    box((-0.55, 1.32, 1.44), (0.7, 0.5, 0.10), 0)           # roof equipment cage
    for i in range(4):
        box((-0.85 + i * 0.2, 1.32, 1.52), (0.02, 0.5, 0.10), 5, bevel=0)
    fan_unit((-0.55, 1.32, 1.50), 0.07)
    helicopter((1.55, 1.45, 0.15), 0.4)


# ===========================================================================
# 11. Warehouse depot: long-span sheds, dock bays, container yard, rail spur
# ===========================================================================
def build_colony_warehouse():
    box((0, 0, 0.06), (4.8, 3.8, 0.12), 6)
    road_strip((-2.4, -1.55, 0.13), (2.4, -1.55, 0.13), 0.40, 0.13)

    for i, sy in enumerate((0.85, -0.35)):                  # two long-span sheds
        box((-0.7, sy, 0.62), (2.6, 1.0, 1.0), 0)
        prism((-0.7, sy, 1.12), 2.66, 1.06, 0.26, 2, ridge="x")
        for k in range(6):                                  # ridge vents
            box((-1.8 + k * 0.44, sy, 1.36), (0.22, 0.16, 0.10), 5)
            fan_unit((-1.8 + k * 0.44, sy, 1.42), 0.05)
        louvered_block((-0.7, sy - 0.51, 0.72), (2.4, 0.05, 0.5), 1, 12, "y")
        for k in range(9):                                  # wall ribs
            box((-1.9 + k * 0.30, sy + 0.51, 0.62), (0.03, 0.03, 1.0), 5, bevel=0)
        box((-0.7, sy, 0.16), (2.7, 1.1, 0.10), 0)

    box((-0.7, -1.05, 0.30), (2.6, 0.4, 0.36), 0)           # dock platform
    for i in range(4):                                      # dock bays
        x = -1.75 + i * 0.7
        box((x, -0.86, 0.52), (0.5, 0.05, 0.44), 1)
        for k in range(5):
            box((x, -0.885, 0.34 + k * 0.09), (0.5, 0.012, 0.02), 5, bevel=0)
        box((x, -0.92, 0.30), (0.52, 0.06, 0.06), 6)        # leveler
        box((x - 0.26, -0.90, 0.36), (0.05, 0.06, 0.14), 7, bevel=0)
        box((x + 0.26, -0.90, 0.36), (0.05, 0.06, 0.14), 7, bevel=0)
        box((x, -0.86, 0.80), (0.56, 0.10, 0.05), 2)        # canopy
    vehicle((-1.75, -1.30, 0.12), math.pi / 2, "truck")
    vehicle((-0.35, -1.32, 0.12), math.pi / 2 + 0.1, "truck")

    box((1.65, 0.9, 0.55), (1.0, 1.2, 0.86), 1)             # office annex
    box((1.65, 0.9, 1.02), (1.06, 1.26, 0.07), 0)
    window_grid((1.65, 0.29, 0.62), 0.85, 0.5, 3, 2, 4, "y")
    window_grid((2.16, 0.9, 0.62), 1.0, 0.5, 3, 2, 4, "x")
    box((1.65, 0.28, 0.30), (0.24, 0.04, 0.4), 3)
    stairs((1.65, 0.16, 0.12), 0.18, 0.2, steps=4)

    for i in range(3):                                      # container yard
        container((1.35 + i * 0.34, -0.55, 0.16), 0.0, (1, 2, 0)[i])
        container((1.35 + i * 0.34, -0.90, 0.16), 0.1, (2, 0, 1)[i])
    container((1.69, -0.55, 0.42), 0.0, 5)
    pallet_stack((2.15, -0.2, 0.12), 4)
    pallet_stack((2.15, -0.6, 0.12), 3)
    gantry_crane((1.7, -0.75, 0.12), 1.1, 0.9)

    for sx in (-0.16, 0.16):                                # rail spur
        box((sx, 1.62, 0.14), (4.4, 0.035, 0.05), 5, bevel=0)
    for i in range(22):
        box((-2.1 + i * 0.2, 1.62, 0.115), (0.06, 0.44, 0.03), 1, bevel=0)
    box((-1.2, 1.62, 0.30), (0.9, 0.34, 0.30), 2)           # rail wagon
    for i in range(4):
        cyl((-1.55 + i * 0.24, 1.48, 0.18), (-1.55 + i * 0.24, 1.76, 0.18), 0.06, 6, verts=12)

    fence_loop((0, 0.1, 0.12), 4.6, 3.4, 0.32, posts=18)
    for x in (-2.1, 0.0, 2.1):
        floodlight((x, -1.75, 0.12), 0.85)
    lamp_post((-2.2, 0.6, 0.12), 0.5)
    barrier_gate((-2.2, -1.55, 0.12), 0.0, 0.4)
    transformer((2.2, 1.5, 0.12), 0.7)

    # Detail pass (reference: logistics-park dock design - seals and shelters
    # around the trailer, bumpers, wheel chocks, staging lanes, apron joints,
    # downpipes and scuppers, mezzanine stair, sprinkler riser, light masts).
    for i in range(4):
        dock_seal((-1.75 + i * 0.7, -0.84, 0.30))
        for k in range(3):                                   # apron joint lines
            box((-1.9 + i * 0.7 + k * 0.2, -1.20, 0.145), (0.012, 0.5, 0.012), 0, bevel=0)
    for i in range(9):                                       # staging lane markings
        box((-2.0 + i * 0.45, -1.72, 0.145), (0.02, 0.34, 0.012), 0, bevel=0)
    for i in range(10):                                      # apron control joints
        box((-2.2 + i * 0.48, -1.02, 0.145), (0.014, 0.02, 0.012), 0, bevel=0)
    for sx in (-1.95, 0.55):                                 # downpipes + scuppers
        for sy in (0.85, -0.35):
            cyl((sx, sy + 0.52, 0.30), (sx, sy + 0.52, 1.18), 0.018, 5, verts=10)
            box((sx, sy + 0.46, 0.26), (0.05, 0.05, 0.06), 5, bevel=0)
        for k in range(3):
            box((sx + 0.02, sy + 0.30 + k * 0.28, 1.24), (0.06, 0.05, 0.04), 6, bevel=0)
    box((1.15, 0.30, 0.55), (0.5, 0.34, 0.04), 5)            # mezzanine stair flight
    for i in range(8):
        box((0.95 + i * 0.06, 0.30, 0.32 + i * 0.035), (0.05, 0.32, 0.012), 0, bevel=0)
    railing((1.45, 0.14, 0.58), (1.45, 0.46, 0.58), 5, 4, 0.10)
    cyl((2.15, 0.30, 0.12), (2.15, 0.30, 1.20), 0.026, 2, verts=12)  # sprinkler riser
    pipe_run((2.15, 0.30, 1.10), (1.2, 0.30, 1.10), 0.022, 5, supports=2)
    for x in (-1.9, 0.2, 2.1):                               # 4-head yard light masts
        cyl((x, -1.95, 0.12), (x, -1.95, 1.15), 0.026, 5, verts=10)
        for k in range(4):
            a = k * math.tau / 4
            box((x + 0.13 * math.cos(a), -1.95 + 0.13 * math.sin(a), 1.18),
                (0.12, 0.08, 0.04), 7, a, bevel=0)
    for i in range(8):                                       # dock-face bollards
        bollard((-1.95 + i * 0.55, -0.66, 0.12))


# ===========================================================================
# 12. Military base: perimeter, towers, shelters, radar, vehicle park, helipad
# ===========================================================================
def build_colony_military():
    box((0, 0, 0.06), (4.8, 3.8, 0.12), 6)
    road_strip((-2.4, -1.5, 0.13), (2.4, -1.5, 0.13), 0.40, 0.13)
    road_strip((0.9, -1.5, 0.13), (0.9, 1.6, 0.13), 0.34, 0.13)
    fence_loop((0, 0.1, 0.12), 4.6, 3.5, 0.34, posts=20)
    for sx, sy in ((-2.2, -1.6), (2.2, 1.7)):
        watchtower((sx, sy, 0.12), 0.66)
    box((-0.9, -1.55, 0.34), (0.7, 0.4, 0.44), 0)           # gatehouse
    box((-0.9, -1.55, 0.60), (0.78, 0.48, 0.07), 2)
    window_grid((-0.9, -1.76, 0.38), 0.5, 0.2, 2, 1, 4, "y")
    barrier_gate((-0.45, -1.5, 0.12), 0.0, 0.55)
    barrier_gate((-1.35, -1.5, 0.12), math.pi, 0.55)
    for i in range(5):
        bollard((-1.6 + i * 0.35, -1.28, 0.12))

    for i, sy in enumerate((0.95, 0.15)):                   # barracks
        box((-1.35, sy, 0.42), (1.5, 0.6, 0.6), 1)
        prism((-1.35, sy, 0.72), 1.56, 0.66, 0.18, 2, ridge="x")
        window_grid((-1.35, sy - 0.31, 0.44), 1.3, 0.3, 5, 1, 4, "y")
        box((-1.35, sy - 0.32, 0.24), (0.16, 0.03, 0.3), 3)
        for k in range(3):
            cyl((-1.9 + k * 0.55, sy + 0.32, 0.72), (-1.9 + k * 0.55, sy + 0.32, 0.86),
                0.03, 5, verts=10)

    for i, sx in enumerate((-0.15, 0.75)):                  # hardened shelters
        box((sx, 1.15, 0.16), (0.8, 0.7, 0.10), 0)
        arch = hemisphere((sx, 1.15, 0.20), 0.40, 0, segs=24, rings=12, zscale=0.62)
        arch.scale.y = 0.85
        bpy.ops.object.transform_apply(scale=True)
        box((sx, 0.80, 0.30), (0.62, 0.06, 0.36), 1)        # blast door
        for k in range(4):
            box((sx, 0.77, 0.16 + k * 0.09), (0.62, 0.02, 0.02), 5, bevel=0)
        box((sx, 1.52, 0.22), (0.7, 0.10, 0.14), 6)         # berm
    box((1.75, 1.15, 0.26), (0.7, 0.7, 0.30), 0)            # ammo bunker
    box((1.75, 1.15, 0.44), (0.78, 0.78, 0.08), 6)
    box((1.75, 0.79, 0.26), (0.3, 0.05, 0.3), 1)
    cyl((1.75, 1.15, 0.48), (1.75, 1.15, 0.66), 0.03, 5, verts=10)

    radar_dish((-1.9, 1.55, 0.12), 0.30, 0.5)               # radar + comms
    cyl((-1.9, 1.55, 0.12), (-1.9, 1.55, 0.42), 0.05, 1, verts=12)
    for i in range(3):
        x = -1.55 + i * 0.22
        cyl((x, 1.75, 0.12), (x, 1.75, 0.95 - i * 0.12), 0.014, 5, verts=8)
        for k in range(3):
            cyl((x, 1.75, 0.5 + k * 0.14), (x + 0.10, 1.75, 0.56 + k * 0.14), 0.007, 5, verts=5)
    sphere((-1.55, 1.75, 0.98), 0.03, 7, segs=8, rings=6)

    vehicle((0.35, -0.55, 0.12), 0.1, "apc")                # vehicle park
    vehicle((0.95, -0.60, 0.12), -0.15, "apc")
    vehicle((1.55, -0.55, 0.12), 0.2, "truck")
    vehicle((0.35, -1.05, 0.12), 0.0, "truck")
    for i in range(4):
        box((0.15 + i * 0.5, -0.30, 0.14), (0.03, 0.5, 0.012), 0, bevel=0)

    helipad((1.75, -0.95, 0.12), 0.44)
    tank_horizontal((-1.55, -0.75, 0.12), 0.14, 0.6, 2, 0.2)  # fuel bladders
    tank_horizontal((-1.55, -1.10, 0.12), 0.14, 0.6, 2, -0.1)
    box((-1.55, -0.92, 0.14), (1.0, 0.85, 0.06), 0)
    pipe_run((-1.25, -0.92, 0.22), (-0.6, -0.92, 0.22), 0.025, 5, supports=3)
    for x in (-2.1, 0.0, 2.1):
        floodlight((x, -1.75, 0.12), 0.9)
    floodlight((-2.2, 0.6, 0.12), 0.8)
    container((2.15, 0.35, 0.16), 0.3, 1)
    pallet_stack((2.15, -0.1, 0.12), 3)
    switchyard((2.15, 1.6, 0.12), rows=1, bays=2, span=0.28)

    # Detail pass (reference: HESCO bastion force-protection layout - stacked
    # concertainer blast walls, T-hangar wing walls, dragon teeth, berm,
    # checkpoint with blocker and mirror, EWP, guied comms mast, bowser).
    hesco_row((-2.25, -1.62, 0.12), (2.25, -1.62, 0.12), 2)
    hesco_row((-0.55, -1.62, 0.12), (-0.55, -0.95, 0.12), 2)
    hesco_row((-1.75, 1.62, 0.12), (1.75, 1.62, 0.12), 3)
    for sx in (-0.15, 0.75):                                 # T-hangar wing walls
        for s in (-1, 1):
            box((sx + 0.30, 1.15 + s * 0.42, 0.24), (0.5, 0.08, 0.36), 0)
            hesco_row((sx - 0.05, 1.15 + s * 0.46, 0.12), (sx + 0.55, 1.15 + s * 0.46, 0.12), 2)
    for i in range(7):                                       # dragon teeth
        box((-2.1 + i * 0.28, 1.85, 0.20), (0.16, 0.16, 0.28), 0, 0.78)
    box((1.75, 1.15, 0.50), (0.86, 0.86, 0.10), 6)          # bunker berm crest
    box((-0.9, -1.85, 0.30), (0.34, 0.24, 0.40), 0)         # checkpoint booth
    box((-0.9, -1.85, 0.54), (0.40, 0.30, 0.06), 2)
    box((-1.12, -1.85, 0.40), (0.04, 0.22, 0.22), 4, bevel=0)
    cyl((-0.62, -1.85, 0.62), (-0.62, -1.85, 0.92), 0.012, 5, verts=8)   # mirror arm
    box((-0.62, -1.78, 0.88), (0.10, 0.02, 0.14), 5, bevel=0)
    barrier_gate((-0.55, -1.62, 0.12), math.pi / 2, 0.5)
    for i in range(3):                                       # EWP sandbag rows
        box((-1.95 + i * 0.12, -0.30, 0.16 + (i % 2) * 0.06), (0.10, 0.08, 0.12), 6)
    cyl((2.35, 0.35, 0.12), (2.35, 0.35, 0.62), 0.016, 5, verts=8)  # EWP mast
    box((2.35, 0.35, 0.66), (0.20, 0.14, 0.03), 5, bevel=0)
    box((2.35, 0.35, 0.72), (0.14, 0.10, 0.03), 5, bevel=0)
    for i in range(3):                                       # guyed comms mast
        a = i * math.tau / 3 + 0.3
        cable((-1.55, 1.75, 0.85), (-1.55 + 0.34 * math.cos(a), 1.75 + 0.34 * math.sin(a), 0.12),
              0.02, 5, segs=4, radius=0.006)
    tank_horizontal((2.30, -0.55, 0.14), 0.13, 0.5, 2, math.pi / 2)      # fuel bowser
    pipe_run((2.30, -0.55, 0.22), (2.05, -0.55, 0.22), 0.02, 5, supports=2)
    helicopter((1.75, -0.95, 0.15), 0.2)
    for sx in (-2.25, 2.25):                                 # range poles
        cyl((sx, 1.0, 0.12), (sx, 1.0, 0.55), 0.012, 7, verts=8)


# ===========================================================================
# 13. Private housing sector (частный сектор): plots, gables, PV, service drops
# ===========================================================================
def build_colony_houses():
    box((0, 0, 0.05), (4.6, 3.8, 0.10), 6)
    road_strip((-2.3, -0.55, 0.11), (2.3, -0.55, 0.11), 0.42, 0.11)
    road_strip((0.35, -0.55, 0.112), (0.35, 1.85, 0.112), 0.30, 0.112)
    for sx in (-1, 1):                                      # sidewalks
        box((0, -0.55 + sx * 0.28, 0.115), (4.6, 0.14, 0.03), 0, bevel=0)

    plots = [(-1.75, 0.55, 0.50, 0.44, 1, 0.18, 0.0, True),
             (-0.85, 0.60, 0.44, 0.40, 2, 0.16, 0.12, False),
             (0.95, 0.60, 0.52, 0.46, 1, 0.20, -0.1, True),
             (1.85, 0.55, 0.42, 0.38, 1, 0.15, 0.2, False),
             (-1.70, -1.35, 0.48, 0.42, 1, 0.17, 0.05, False),
             (-0.75, -1.40, 0.54, 0.46, 2, 0.19, -0.08, True),
             (1.05, -1.35, 0.46, 0.40, 1, 0.16, 0.15, False),
             (1.90, -1.40, 0.44, 0.40, 1, 0.18, -0.2, True)]
    for (x, y, w, d, fl, rh, rz, pv) in plots:
        house((x, y, 0.10), w, d, fl, rh, rz, wall=1, roof=2, porch=True, pv=pv)
        fence_loop((x, y, 0.10), w + 0.34, d + 0.34, 0.14, posts=8)
        road_strip((x, y - d / 2 - 0.20, 0.105), (x, -0.55 + (0.30 if y > 0 else -0.30), 0.105),
                   0.16, 0.105)
        planter((x + w * 0.4, y + d * 0.4, 0.10))

    for i, x in enumerate((-1.9, -0.6, 0.9, 2.0)):          # street lights + poles
        lamp_post((x, -0.28, 0.10), 0.44)
    for i, x in enumerate((-1.6, 0.2, 1.7)):                # service drops
        cyl((x, 1.75, 0.10), (x, 1.75, 0.72), 0.022, 5, verts=8)
        cyl((x - 0.10, 1.75, 0.66), (x + 0.10, 1.75, 0.66), 0.014, 5, verts=6)
        for k in range(3):
            cable((x - 0.08 + k * 0.08, 1.75, 0.64),
                  (x - 0.08 + k * 0.08, 0.95, 0.42), 0.05, 1, segs=5, radius=0.006)
    cable((-1.6, 1.75, 0.70), (0.2, 1.75, 0.70), 0.06, 1, segs=8)
    cable((0.2, 1.75, 0.70), (1.7, 1.75, 0.70), 0.06, 1, segs=8)

    cyl((-2.05, 1.55, 0.10), (-2.05, 1.55, 0.52), 0.14, 5, verts=18)  # water tank
    cone((-2.05, 1.55, 0.58), 0.15, 0.03, 0.10, 2, verts=18)
    for i in range(4):
        a = i * math.tau / 4
        cyl((-2.05 + 0.12 * math.cos(a), 1.55 + 0.12 * math.sin(a), 0.10),
            (-2.05 + 0.12 * math.cos(a), 1.55 + 0.12 * math.sin(a), 0.30), 0.016, 5, verts=6)
    box((-2.05, 1.20, 0.20), (0.24, 0.18, 0.20), 1)         # pump house
    louvered_block((-2.05, 1.20, 0.32), (0.20, 0.15, 0.06), 5, 4, "y")
    pipe_run((-2.05, 1.32, 0.16), (-1.2, 1.32, 0.16), 0.022, 5, supports=3)

    box((2.05, 1.55, 0.22), (0.34, 0.24, 0.24), 1)          # backup genset
    louvered_block((2.05, 1.55, 0.36), (0.30, 0.20, 0.08), 5, 5, "y")
    cyl((2.16, 1.55, 0.40), (2.16, 1.55, 0.62), 0.022, 6, verts=10)
    fan_unit((1.94, 1.55, 0.40), 0.05)
    container((2.05, 1.20, 0.14), 0.0, 2)

    box((-0.2, 1.45, 0.12), (0.5, 0.4, 0.04), 0)            # playground
    for sx in (-0.18, 0.18):
        cyl((-0.2 + sx, 1.32, 0.14), (-0.2 + sx, 1.32, 0.44), 0.016, 5, verts=8)
    cyl((-0.38, 1.32, 0.44), (-0.02, 1.32, 0.44), 0.014, 5, verts=8)
    box((-0.2, 1.55, 0.26), (0.16, 0.30, 0.02), 2, 0.0)
    box((-0.2, 1.42, 0.34), (0.16, 0.04, 0.16), 7, bevel=0)
    for i in range(3):
        bench((-1.2 + i * 1.1, -0.95, 0.10), 0.0)
    fence_loop((0, 0.1, 0.10), 4.5, 3.7, 0.16, posts=16)

    # Detail pass (reference: low-rise residential plots - bay windows, dormers,
    # hip/gable mix, garden walls, driveways with cars, meters, satellite dishes).
    for (x, y, w, d, fl, rh, rz, pv) in plots:
        bx, by = rot2(-w * 0.34, -d * 0.55, rz)             # bay window
        box((x + bx, y + by, 0.10 + 0.11), (w * 0.30, 0.10, 0.22), 1, rz)
        box((x + bx, y + by - 0.05, 0.10 + 0.11), (w * 0.24, 0.02, 0.16), 4, rz, bevel=0)
        dormer((x + w * 0.10, y - d * 0.10, 0.10 + 2 * 0.23 + rh * 0.35), 0.11, 0.08, rz)
        mx, my = rot2(w * 0.52, -d * 0.52, rz)              # meter box + lamp
        box((x + mx, y + my, 0.20), (0.07, 0.04, 0.12), 0, rz)
        box((x + mx, y + my - 0.03, 0.24), (0.05, 0.02, 0.05), 4, rz, bevel=0)
        cyl((x - w * 0.40, y + d * 0.30, 0.10), (x - w * 0.40, y + d * 0.30, 0.16),
            0.008, 5, verts=6)                              # TV/satellite mast
        dish = sphere((x - w * 0.40, y + d * 0.30, 0.19), 0.05, 3, segs=10, rings=6, zscale=0.5)
        dish.rotation_euler.x = 0.7
        bpy.ops.object.transform_apply(rotation=True)
        box((x - w * 0.30, y - d * 0.72, 0.115), (0.16, 0.06, 0.02), 6, rz, bevel=0)
        vehicle((x + w * 0.30, y - d * 0.75, 0.10), rz, "car")
    for i in range(4):                                       # hedge rows along street
        box((-1.6 + i * 1.05, -0.86, 0.20), (0.8, 0.08, 0.16), 3, bevel=0)
    for x in (-1.2, 0.6):                                    # clothesline posts
        cyl((x, 1.55, 0.10), (x, 1.55, 0.34), 0.012, 5, verts=6)
        cable((x, 1.55, 0.33), (x + 0.5, 1.55, 0.33), 0.02, 1, segs=4, radius=0.004)
    for i in range(3):                                       # rain barrels at plots
        cyl((-1.1 + i * 1.2, 1.62, 0.10), (-1.1 + i * 1.2, 1.62, 0.26), 0.07, 2, verts=14)


# ===========================================================================
# 14. Apartment blocks (квартирные дома): storeys, balconies, courtyard, DHW
# ===========================================================================
def build_colony_apartments():
    box((0, 0, 0.05), (4.8, 3.9, 0.10), 6)
    road_strip((-2.4, -1.62, 0.11), (2.4, -1.62, 0.11), 0.40, 0.11)
    storey_block((-1.45, 0.75, 0.10), 12, 0.86, 0.62, 0.24, 0, True)
    storey_block((-0.30, 0.95, 0.10), 8, 0.92, 0.58, 0.24, 0, True)
    storey_block((1.05, 0.80, 0.10), 5, 1.00, 0.60, 0.24, 0, True)
    storey_block((1.95, -0.35, 0.10), 9, 0.70, 0.70, 0.24, 0, True)

    box((-0.55, -0.55, 0.115), (2.4, 1.0, 0.03), 0, bevel=0)  # courtyard
    road_strip((-1.7, -0.55, 0.13), (0.6, -0.55, 0.13), 0.18, 0.13)
    for i in range(4):
        bench((-1.4 + i * 0.55, -0.95, 0.12), 0.0)
        planter((-1.4 + i * 0.55, -0.20, 0.12))
    box((-0.55, -0.55, 0.14), (0.44, 0.34, 0.03), 6, bevel=0)  # playground
    for sx in (-0.16, 0.16):
        cyl((-0.55 + sx, -0.66, 0.16), (-0.55 + sx, -0.66, 0.46), 0.016, 5, verts=8)
    cyl((-0.71, -0.66, 0.46), (-0.39, -0.66, 0.46), 0.014, 5, verts=8)
    box((-0.55, -0.44, 0.28), (0.14, 0.26, 0.02), 2)
    for i in range(3):
        sphere((-0.75 + i * 0.2, -0.40, 0.20), 0.05, 3, segs=10, rings=8)

    box((1.35, -1.15, 0.115), (1.5, 0.6, 0.03), 6, bevel=0)   # parking
    for i in range(5):
        box((0.75 + i * 0.3, -1.15, 0.13), (0.025, 0.5, 0.012), 0, bevel=0)
    vehicle((0.90, -1.15, 0.12), math.pi / 2, "car")
    vehicle((1.50, -1.10, 0.12), math.pi / 2 + 0.1, "car")
    vehicle((2.00, -1.18, 0.12), math.pi / 2 - 0.1, "truck")

    box((-2.05, -0.85, 0.26), (0.44, 0.40, 0.32), 1)         # DHW substation
    louvered_block((-2.05, -0.85, 0.44), (0.38, 0.34, 0.10), 5, 6, "y")
    box((-2.05, -0.85, 0.52), (0.48, 0.44, 0.06), 2)
    for i in range(2):
        cyl((-2.05, -0.62 + i * 0.10, 0.20), (-2.05, -0.62 + i * 0.10, 0.62), 0.035, 5, verts=12)
    pipe_run((-1.85, -0.85, 0.18), (-0.9, -0.85, 0.18), 0.030, 2, supports=4)
    pipe_run((-0.9, -0.85, 0.18), (-0.9, 0.45, 0.18), 0.030, 2, supports=4)
    pipe_run((0.2, -0.85, 0.18), (1.6, -0.85, 0.18), 0.026, 5, supports=4)
    for i in range(3):
        torus((-1.5 + i * 0.6, -0.85, 0.24), 0.05, 0.012, 5, major_seg=14, minor_seg=5)

    cyl((2.25, 1.45, 0.10), (2.25, 1.45, 0.80), 0.024, 5, verts=8)  # service pole
    cyl((2.13, 1.45, 0.72), (2.37, 1.45, 0.72), 0.014, 5, verts=6)
    transformer((2.25, 1.20, 0.10), 0.55)
    cable((2.25, 1.45, 0.76), (1.05, 1.10, 0.50), 0.08, 1, segs=6)
    for x in (-2.2, -0.9, 0.5, 1.9):
        lamp_post((x, -1.35, 0.10), 0.48)
    box((-2.2, 0.15, 0.22), (0.30, 0.24, 0.24), 1)          # waste enclosure
    box((-2.2, 0.15, 0.36), (0.34, 0.28, 0.04), 2)
    for i in range(3):
        container((-2.28 + i * 0.09, 0.15, 0.14), 0.0, (1, 2, 5)[i])
    fence_loop((0, 0.05, 0.10), 4.7, 3.8, 0.18, posts=18)

    # Detail pass (reference: panel housing estates - expressed stair cores,
    # wrap corner glazing, balcony screens, taller ground storey with shopfront
    # glazing, roof plant rooms, refuse enclosure, deck-access gallery).
    blocks = ((-1.45, 0.75, 12, 0.86, 0.62), (-0.30, 0.95, 8, 0.92, 0.58),
              (1.05, 0.80, 5, 1.00, 0.60), (1.95, -0.35, 9, 0.70, 0.70))
    for (bx, by, floors, w, d) in blocks:
        H = floors * 0.24
        box((bx + w * 0.34, by, 0.10 + H / 2), (0.16, d + 0.06, H), 1)   # stair core
        window_grid((bx + w * 0.42, by, 0.10 + H / 2), d * 0.8, H * 0.8, 1, floors, 4, "x")
        box((bx + w * 0.34, by, 0.10 + H + 0.06), (0.20, d + 0.10, 0.10), 0)
        for face in (-1, 1):                                 # wrap corner glazing
            window_grid((bx - w / 2 - 0.01 * face, by + face * d * 0.42, 0.10 + H / 2),
                        0.24, H * 0.8, 1, floors, 4, "x")
        box((bx, by - d / 2 - 0.02, 0.10 + 0.16), (w * 0.96, 0.02, 0.30), 4, bevel=0)
        box((bx, by - d / 2 - 0.06, 0.10 + 0.34), (w * 0.98, 0.10, 0.03), 2)  # shop canopy
        box((bx - w * 0.30, by - d / 2 - 0.07, 0.10 + 0.40), (w * 0.30, 0.02, 0.10), 7, bevel=0)
        box((bx + w * 0.20, by, 0.10 + H + 0.10), (w * 0.34, d * 0.42, 0.14), 1)  # plant room
        for k in range(2):
            cyl((bx + w * 0.10 + k * 0.12, by + d * 0.20, 0.10 + H + 0.24),
                (bx + w * 0.10 + k * 0.12, by + d * 0.20, 0.10 + H + 0.34), 0.03, 5, verts=10)
    for i in range(2):                                       # balcony glazing screens
        bx, by, floors, w, d = blocks[i]
        H = floors * 0.24
        for j in range(3):
            box((bx - w * 0.3 + j * w * 0.3, by - d / 2 - 0.09, 0.10 + H * 0.5),
                (w * 0.22, 0.012, H * 0.55), 4, bevel=0)
    box((-2.15, -1.35, 0.24), (0.5, 0.36, 0.34), 1)          # refuse enclosure
    box((-2.15, -1.35, 0.44), (0.56, 0.42, 0.06), 2)
    for i in range(3):
        box((-2.32 + i * 0.17, -1.35, 0.18), (0.13, 0.24, 0.22), 6)
    bx, by, floors, w, d = blocks[2]                         # deck-access gallery
    box((bx - w / 2 - 0.10, by, 0.10 + 0.26), (0.14, d * 0.9, 0.04), 5)
    for i in range(4):
        box((bx - w / 2 - 0.10, by - d * 0.4 + i * d * 0.26, 0.10 + 0.14), (0.12, 0.03, 0.24), 1)
    for (bx, by, floors, w, d) in blocks:                    # estate colour bands
        H = floors * 0.24
        box((bx, by - d / 2 - 0.004, 0.10 + H * 0.97), (w * 0.9, 0.01, 0.06), 7, bevel=0)
    for i in range(2):                                       # swing frame in playground
        box((-0.72 + i * 0.34, -0.52, 0.24), (0.02, 0.02, 0.28), 5, bevel=0)
    cable((-0.72, -0.52, 0.38), (-0.38, -0.52, 0.38), 0.01, 5, segs=3, radius=0.005)
    for sx in (-0.62, -0.48):
        cyl((sx, -0.52, 0.38), (sx, -0.52, 0.30), 0.005, 1, verts=6)
        box((sx, -0.52, 0.29), (0.06, 0.04, 0.012), 2, bevel=0)
    barrier_gate((-2.3, -1.62, 0.10), 0.0, 0.4)


# ===========================================================================
# 15. Pipeline trunk (газ/вода): supports, loops, valve stations, pig traps
# ===========================================================================
def build_colony_pipeline():
    box((0, 0, 0.05), (4.8, 3.4, 0.10), 6)
    road_strip((-2.4, -1.25, 0.11), (2.4, -1.25, 0.11), 0.34, 0.11)
    pipe_run((-2.3, 0.55, 0.30), (2.3, 0.55, 0.30), 0.075, 5, supports=12)
    pipe_run((-2.3, 0.85, 0.24), (2.3, 0.85, 0.24), 0.050, 2, supports=12)
    for i in range(3):                                      # expansion loops
        x = -1.5 + i * 1.5
        torus((x, 0.55, 0.42), 0.16, 0.075, 5, major_seg=20, minor_seg=10)
        box((x, 0.55, 0.16), (0.10, 0.10, 0.12), 0)
    for i in range(2):                                      # valve stations
        x = -0.75 + i * 1.6
        box((x, 0.55, 0.42), (0.26, 0.26, 0.16), 1)
        cyl((x, 0.55, 0.50), (x, 0.55, 0.66), 0.030, 5, verts=12)
        torus((x, 0.55, 0.68), 0.10, 0.016, 7, major_seg=18, minor_seg=6)
        for k in range(4):
            a = k * math.tau / 4
            cyl((x, 0.55, 0.68), (x + 0.10 * math.cos(a), 0.55 + 0.10 * math.sin(a), 0.68),
                0.008, 5, verts=5)
        box((x + 0.22, 0.55, 0.30), (0.14, 0.14, 0.36), 0)
        louvered_block((x + 0.22, 0.55, 0.52), (0.12, 0.12, 0.08), 5, 4, "y")

    box((-1.85, -0.45, 0.34), (0.80, 0.60, 0.48), 1)        # pump / compressor house
    louvered_block((-1.85, -0.45, 0.62), (0.72, 0.52, 0.16), 5, 8, "y")
    box((-1.85, -0.45, 0.74), (0.86, 0.66, 0.07), 2)
    for i in range(2):
        fan_unit((-2.05 + i * 0.4, -0.45, 0.78), 0.09)
    cyl((-1.45, -0.45, 0.30), (-1.45, 0.55, 0.30), 0.060, 5, verts=16)
    box((-1.85, -0.78, 0.24), (0.30, 0.16, 0.28), 0)        # motor + coupling guard
    cyl((-1.85, -0.62, 0.24), (-1.85, -0.45, 0.24), 0.07, 1, verts=14)
    pipe_run((-1.45, -0.45, 0.55), (-1.45, 0.55, 0.55), 0.022, 2, supports=3)

    for i, x in enumerate((0.55, 1.35)):                    # pig launcher / receiver
        cyl((x, -0.55, 0.26), (x + 0.55, -0.55, 0.26), 0.11, 5, verts=18)
        cone((x + 0.62, -0.55, 0.26), 0.11, 0.07, 0.14, 1, verts=18)
        torus((x + 0.55, -0.55, 0.26), 0.12, 0.018, 7, major_seg=18, minor_seg=6)
        box((x + 0.2, -0.55, 0.12), (0.10, 0.24, 0.12), 0)
        cyl((x + 0.62, -0.55, 0.36), (x + 0.62, -0.55, 0.52), 0.016, 5, verts=8)
        cyl((x + 0.62, -0.55, 0.52), (x + 0.40, -0.55, 0.52), 0.014, 5, verts=8)
        pipe_run((x, -0.55, 0.26), (x, 0.55, 0.30), 0.045, 5, supports=3)

    box((2.0, -0.45, 0.24), (0.5, 0.4, 0.28), 0)            # metering skid
    for i in range(3):
        cyl((1.85 + i * 0.15, -0.45, 0.38), (1.85 + i * 0.15, -0.45, 0.52), 0.035, 5, verts=12)
        box((1.85 + i * 0.15, -0.45, 0.56), (0.08, 0.08, 0.06), 3)
    pipe_rack((1.75, -0.20, 0.10), 0.6, levels=2)
    box((2.0, -0.90, 0.30), (0.34, 0.24, 0.40), 1)          # control cabinet
    louvered_block((2.0, -0.90, 0.54), (0.30, 0.20, 0.10), 5, 5, "y")
    box((2.0, -1.03, 0.34), (0.16, 0.02, 0.12), 4, bevel=0)
    for i in range(6):                                      # CP test posts
        x = -2.0 + i * 0.8
        box((x, 1.15, 0.20), (0.06, 0.06, 0.20), 0)
        box((x, 1.15, 0.32), (0.08, 0.08, 0.04), 7, bevel=0)
    for i in range(5):                                      # marker posts
        x = -1.8 + i * 0.9
        cyl((x, 0.20, 0.10), (x, 0.20, 0.34), 0.018, 7, verts=8)
    fence_loop((0, 0.1, 0.10), 4.7, 3.3, 0.26, posts=16)
    floodlight((-2.2, 1.3, 0.10), 0.8)
    floodlight((2.2, 1.3, 0.10), 0.8)
    lamp_post((0.0, -1.05, 0.10), 0.46)

    # Detail pass (reference: block valve station specs - mainline tee and
    # bypass, ESD valves, insulating joints with surge diverters, thrust blocks,
    # reducer vents, scraper trap, CP rectifier with anode bed, marker plates).
    for i, x in enumerate((-0.75, 0.85)):                   # block valve assemblies
        box((x, 0.55, 0.30), (0.34, 0.34, 0.30), 0)         # mainline tee pit cover
        for s in (-1, 1):
            pipe_run((x, 0.55, 0.30), (x, 0.55 + s * 0.42, 0.30), 0.05, 5, supports=2)
            cyl((x, 0.55 + s * 0.42, 0.30), (x, 0.55 + s * 0.42, 0.52), 0.045, 1, verts=14)
            torus((x, 0.55 + s * 0.42, 0.56), 0.07, 0.012, 7, major_seg=16, minor_seg=5)
            box((x + 0.10, 0.55 + s * 0.42, 0.62), (0.10, 0.05, 0.06), 5, bevel=0)  # actuator
        pipe_run((x - 0.30, 0.13, 0.24), (x + 0.30, 0.13, 0.24), 0.03, 2, supports=2)  # bypass
        cyl((x, 0.13, 0.24), (x, 0.13, 0.40), 0.028, 1, verts=12)
    for x in (-1.15, 1.25):                                 # insulating joints + arrestors
        torus((x, 0.55, 0.30), 0.082, 0.020, 2, major_seg=18, minor_seg=6)
        box((x, 0.72, 0.34), (0.12, 0.08, 0.14), 1)
        for k in range(2):
            cyl((x - 0.03 + k * 0.06, 0.78, 0.41), (x - 0.03 + k * 0.06, 0.78, 0.52),
                0.008, 5, verts=6)
            box((x - 0.03 + k * 0.06, 0.78, 0.54), (0.03, 0.03, 0.04), 7, bevel=0)
    for i in range(4):                                       # thrust/anchor blocks
        box((-1.9 + i * 1.25, 0.55, 0.14), (0.24, 0.24, 0.16), 0)
    for i, x in enumerate((0.55, 1.35)):                     # pig trap vents + reducers
        cyl((x + 0.70, -0.55, 0.26), (x + 0.70, -0.55, 0.62), 0.02, 5, verts=10)
        cyl((x + 0.70, -0.55, 0.62), (x + 0.86, -0.55, 0.70), 0.016, 5, verts=8)
        cone((x + 0.88, -0.55, 0.71), 0.03, 0.005, 0.05, 7, verts=10)
        cyl((x + 0.30, -0.55, 0.37), (x + 0.30, -0.55, 0.26), 0.05, 1, verts=14)
        box((x + 0.42, -0.30, 0.20), (0.16, 0.12, 0.16), 0)  # scrubber knock-out drum
    box((2.25, 0.55, 0.24), (0.22, 0.16, 0.28), 1)           # CP rectifier cabinet
    box((2.25, 0.55, 0.40), (0.26, 0.20, 0.05), 2)
    window_grid((2.25, 0.46, 0.30), 0.12, 0.10, 1, 1, 4, "y")
    for i in range(4):                                       # deep anode bed
        cyl((2.25 - 0.1 + i * 0.07, 0.85, 0.10), (2.25 - 0.1 + i * 0.07, 0.85, 0.02),
            0.014, 5, verts=8)
        box((2.25 - 0.1 + i * 0.07, 0.85, 0.12), (0.05, 0.05, 0.04), 0, bevel=0)
    for i in range(5):                                       # marker plates on posts
        x = -2.0 + i * 1.0
        cyl((x, 0.20, 0.34), (x, 0.20, 0.40), 0.02, 7, verts=8)
        box((x, 0.20, 0.44), (0.16, 0.02, 0.10), 0, bevel=0)
        box((x, 0.19, 0.44), (0.12, 0.01, 0.06), 4, bevel=0)
    box((-2.25, -0.10, 0.30), (0.18, 0.22, 0.34), 1)         # crossing warning sign
    box((-2.25, -0.22, 0.42), (0.20, 0.02, 0.20), 7, bevel=0)


# ===========================================================================
# 16. Transmission line (ЛЭП): lattice pylons, sag spans, end substation
# ===========================================================================
def build_colony_powerline():
    box((0, 0, 0.05), (4.8, 3.4, 0.10), 6)
    road_strip((-2.4, -1.30, 0.11), (2.4, -1.30, 0.11), 0.30, 0.11)
    xs = (-1.85, -0.25, 1.35)
    for i, x in enumerate(xs):
        lattice_pylon((x, 0.45, 0.10), 1.55 - i * 0.05, 0.42, 0.16, 5, arms=2)
        box((x, 0.45, 0.13), (0.56, 0.56, 0.06), 0)
    for i in range(len(xs) - 1):                            # conductors, 2 circuits
        a, b = xs[i], xs[i + 1]
        ha, hb = 1.55 - i * 0.05, 1.55 - (i + 1) * 0.05
        for arm in range(2):
            za = 0.10 + ha * (0.60 + 0.17 * arm) - 0.16
            zb = 0.10 + hb * (0.60 + 0.17 * arm) - 0.16
            aw = 0.42 * (1.7 - 0.3 * arm) / 2
            bw = 0.42 * (1.7 - 0.3 * arm) / 2
            for sx in (-1, 1):
                cable((a + sx * aw, 0.45, za), (b + sx * bw, 0.45, zb),
                      0.14, 1, segs=10, radius=0.011)
        cable((a, 0.45, 0.10 + ha + 0.10), (b, 0.45, 0.10 + hb + 0.10),
              0.10, 5, segs=8, radius=0.007)
    for i in range(3):                                      # angle tower guys
        a = math.tau * i / 3 + 0.4
        cable((-0.25, 0.45, 1.45), (-0.25 + 0.6 * math.cos(a), 0.45 + 0.6 * math.sin(a), 0.10),
              0.02, 5, segs=4, radius=0.007)
        box((-0.25 + 0.6 * math.cos(a), 0.45 + 0.6 * math.sin(a), 0.13), (0.10, 0.10, 0.06), 0)

    box((2.05, 0.45, 0.16), (1.0, 1.2, 0.12), 0)            # end substation pad
    transformer((1.85, 0.75, 0.22), 0.85)
    transformer((2.25, 0.75, 0.22), 0.85)
    switchyard((2.05, 0.05, 0.22), rows=2, bays=3, span=0.30)
    box((2.05, 1.05, 0.40), (0.6, 0.34, 0.36), 1)           # control house
    louvered_block((2.05, 1.05, 0.60), (0.52, 0.28, 0.10), 5, 6, "y")
    box((2.05, 1.05, 0.68), (0.66, 0.40, 0.06), 2)
    window_grid((2.05, 0.87, 0.44), 0.4, 0.16, 2, 1, 4, "y")
    for i in range(3):
        cyl((1.75 + i * 0.3, 0.45, 0.22), (1.75 + i * 0.3, 0.45, 0.72), 0.018, 5, verts=8)
        cable((1.75 + i * 0.3, 0.45, 0.70), (1.35 + 0.42 * 0.55, 0.45, 1.20), 0.06, 1, segs=5)
    box((-2.2, -0.6, 0.24), (0.3, 0.2, 0.28), 1)            # line cabinet
    box((-2.2, -0.71, 0.30), (0.14, 0.02, 0.10), 7, bevel=0)
    for i in range(4):
        x = -1.8 + i * 1.1
        box((x, -0.85, 0.22), (0.16, 0.02, 0.20), 0)
        box((x, -0.86, 0.30), (0.12, 0.012, 0.08), 7, bevel=0)
    fence_loop((0, 0.1, 0.10), 4.7, 3.3, 0.28, posts=16)
    floodlight((-2.2, 1.35, 0.10), 0.85)
    floodlight((2.3, -0.9, 0.10), 0.85)
    lamp_post((-0.6, -1.1, 0.10), 0.46)

    # Detail pass (reference: tower field guides - conductors hung below the
    # suspension insulator strings, twin bundles, shield wire on the peak with
    # rods, stub footings, ladder, bird spikes, strain hardware, marker spheres).
    for i, x in enumerate(xs):                              # re-hang below strings
        h = 1.55 - i * 0.05
        for arm in range(2):
            z = 0.10 + h * (0.60 + 0.17 * arm)
            aw = 0.42 * (1.7 - 0.3 * arm) / 2
            for sx in (-1, 1):
                insulator_string((x + sx * aw, 0.45, z), 0.16, 5)
                insulator_string((x + sx * aw * 0.55, 0.45, z), 0.16, 5)
        insulator_string((x, 0.45, 0.10 + h), 0.12, 4)      # peak earth-wire hook
    for i in range(len(xs) - 1):                            # twin-bundle conductors
        a, b = xs[i], xs[i + 1]
        ha, hb = 1.55 - i * 0.05, 1.55 - (i + 1) * 0.05
        for arm in range(2):
            za = 0.10 + ha * (0.60 + 0.17 * arm) - 0.20
            zb = 0.10 + hb * (0.60 + 0.17 * arm) - 0.20
            aw = 0.42 * (1.7 - 0.3 * arm)
            for sx in (-1, 1):
                for off in (-0.035, 0.035):
                    cable((a + sx * aw / 2 + off, 0.45, za),
                          (b + sx * aw / 2 + off, 0.45, zb), 0.16, 1, segs=10, radius=0.009)
        cable((a, 0.45, 0.10 + ha - 0.12), (b, 0.45, 0.10 + hb - 0.12),
              0.12, 5, segs=8, radius=0.006)                # shield wire span
        sphere(((a + b) / 2, 0.45, 0.10 + (ha + hb) / 2 - 0.30), 0.05, 3, segs=10, rings=8)
    for i, x in enumerate(xs):                              # footings, stubs, ladder
        h = 1.55 - i * 0.05
        for sx, sy in ((-1, -1), (1, -1), (1, 1), (-1, 1)):
            box((x + sx * 0.26, 0.45 + sy * 0.26, 0.12), (0.16, 0.16, 0.06), 0)
            cyl((x + sx * 0.24, 0.45 + sy * 0.24, 0.15),
                (x + sx * 0.20, 0.45 + sy * 0.20, 0.34), 0.022, 5, verts=8)
        for k in range(9):
            box((x + 0.09, 0.45 - 0.09, 0.30 + k * 0.12), (0.05, 0.14, 0.012), 5, bevel=0)
        for arm in range(2):                                # bird spikes on arms
            z = 0.10 + h * (0.60 + 0.17 * arm) + 0.02
            for j in range(6):
                box((x - 0.30 + j * 0.12, 0.45, z), (0.012, 0.012, 0.05), 5, bevel=0)
    box((-2.35, 0.45, 0.30), (0.16, 0.16, 0.36), 1)         # strain/angle dead-end
    for i in range(3):
        cyl((-2.35, 0.30 + i * 0.15, 0.48), (-2.20, 0.30 + i * 0.15, 0.48), 0.012, 1, verts=10)
    box((0.0, -0.95, 0.13), (4.2, 0.5, 0.03), 6, bevel=0)   # cable trench cover
    for i in range(12):
        box((-1.95 + i * 0.36, -0.95, 0.15), (0.02, 0.52, 0.012), 0, bevel=0)
    box((2.05, -0.55, 0.42), (0.9, 0.16, 0.06), 5)          # substation A-frame
    for sx in (-0.4, 0.4):
        cyl((2.05 + sx, -0.55, 0.12), (2.05 + sx * 0.6, -0.55, 0.45), 0.022, 5, verts=8)
    for i in range(3):
        insulator_string((1.85 + i * 0.2, -0.55, 0.42), 0.12, 4)


# ===========================================================================
# 17. Solar farm (СЭС): tracker rows, inverter skids, met mast, trenches
# ===========================================================================
def build_colony_solar():
    box((0, 0, 0.05), (4.8, 3.6, 0.10), 6)
    road_strip((-2.4, -1.45, 0.11), (2.4, -1.45, 0.11), 0.32, 0.11)
    for r in range(4):
        y = -0.85 + r * 0.62
        solar_row((-2.0, y, 0.10), 3.4, panels=7, tilt=0.42 + r * 0.03, slot=3)
        box((-0.3, y + 0.30, 0.115), (3.6, 0.06, 0.03), 0, bevel=0)  # cable trench
    for i in range(2):                                      # inverter skids
        x = -1.35 + i * 1.5
        box((x, 1.45, 0.28), (0.70, 0.44, 0.36), 1)
        louvered_block((x, 1.45, 0.50), (0.62, 0.38, 0.14), 5, 8, "y")
        box((x, 1.45, 0.60), (0.76, 0.50, 0.06), 2)
        fan_unit((x - 0.2, 1.45, 0.63), 0.07)
        fan_unit((x + 0.2, 1.45, 0.63), 0.07)
        transformer((x + 0.55, 1.45, 0.10), 0.6)
        box((x, 1.20, 0.20), (0.20, 0.12, 0.20), 0)
        box((x, 1.13, 0.24), (0.12, 0.02, 0.10), 4, bevel=0)
    for i in range(6):                                      # combiner boxes
        x = -1.9 + i * 0.72
        box((x, 1.05, 0.20), (0.12, 0.10, 0.20), 1)
        box((x, 0.99, 0.24), (0.07, 0.02, 0.07), 7, bevel=0)
    cyl((2.15, 1.35, 0.10), (2.15, 1.35, 1.25), 0.026, 5, verts=10)  # met mast
    for i in range(3):
        z = 0.45 + i * 0.3
        cyl((2.15, 1.35, z), (2.15 + 0.14, 1.35, z), 0.010, 5, verts=6)
    cyl((2.29, 1.35, 1.20), (2.29, 1.35, 1.30), 0.008, 5, verts=6)
    for i in range(3):
        a = i * math.tau / 3
        cyl((2.29, 1.35, 1.30), (2.29 + 0.06 * math.cos(a), 1.35 + 0.06 * math.sin(a), 1.32),
            0.006, 5, verts=5)
        sphere((2.29 + 0.07 * math.cos(a), 1.35 + 0.07 * math.sin(a), 1.32), 0.018, 5, segs=8, rings=6)
    box((2.15, 1.05, 0.22), (0.18, 0.14, 0.24), 1)
    for i in range(3):
        a = i * math.tau / 3 + 0.5
        cable((2.15, 1.35, 1.15), (2.15 + 0.4 * math.cos(a), 1.35 + 0.4 * math.sin(a), 0.10),
              0.02, 5, segs=4, radius=0.006)
    box((-2.15, 1.35, 0.30), (0.44, 0.34, 0.40), 1)         # O&M cabin
    louvered_block((-2.15, 1.35, 0.54), (0.38, 0.28, 0.10), 5, 5, "y")
    box((-2.15, 1.35, 0.62), (0.50, 0.40, 0.06), 2)
    window_grid((-2.15, 1.17, 0.36), 0.26, 0.14, 2, 1, 4, "y")
    fence_loop((0, 0.05, 0.10), 4.7, 3.5, 0.26, posts=18)
    barrier_gate((-2.3, -1.45, 0.10), 0.0, 0.4)

    # Detail pass (reference: utility-scale tracker arrays - N-S torque tubes
    # with row pitch for backtracking, slewing drive + controller per row,
    # 2-up portrait module tables, mid/end clamps, grounding, washing water).
    for r in range(4):
        y = -0.85 + r * 0.62
        slewing_drive((-0.2, y, 0.10))
        box((-2.0, y, 0.115), (0.10, 0.06, 0.03), 5, bevel=0)   # row controller post
        box((-2.0, y, 0.20), (0.12, 0.08, 0.10), 1)
        for i in range(8):                                       # pile rows
            x = -2.0 + i * 0.44
            box((x, y - 0.14, 0.12), (0.06, 0.05, 0.05), 0, bevel=0)
            box((x, y + 0.14, 0.12), (0.06, 0.05, 0.05), 0, bevel=0)
        for i in range(7):                                       # module tables
            x = -1.88 + i * 0.48
            for s in (-0.07, 0.07):
                box((x, y + s, 0.235), (0.44, 0.13, 0.010), 3, bevel=0)
            for s in (-0.14, 0.0, 0.14):                         # mid/end clamps
                box((x - 0.22, y + s, 0.25), (0.02, 0.02, 0.02), 5, bevel=0)
                box((x + 0.22, y + s, 0.25), (0.02, 0.02, 0.02), 5, bevel=0)
            cyl((x, y - 0.155, 0.245), (x, y + 0.155, 0.245), 0.006, 5, verts=6)
    box((-2.15, -1.20, 0.22), (0.26, 0.22, 0.28), 2)             # washing water tank
    cyl((-2.15, -1.20, 0.50), (-2.15, -1.20, 0.58), 0.03, 5, verts=10)
    for i in range(3):                                           # MV cable markers
        box((-1.2 + i * 1.1, 1.15, 0.15), (0.10, 0.02, 0.10), 7, bevel=0)
    for x in (-1.35, 0.15):                                      # skid earth electrodes
        for k in range(3):
            cyl((x - 0.2 + k * 0.2, 1.62, 0.10), (x - 0.2 + k * 0.2, 1.62, 0.22),
                0.008, 5, verts=6)
    floodlight((-2.2, -1.0, 0.10), 0.8)
    floodlight((2.2, -1.0, 0.10), 0.8)


# ===========================================================================
# 18. Diesel genset yard (ДГУ): containers, day tanks, bund, exhaust, switchgear
# ===========================================================================
def build_colony_diesel():
    box((0, 0, 0.05), (4.4, 3.4, 0.10), 6)
    road_strip((-2.2, -1.35, 0.11), (2.2, -1.35, 0.11), 0.34, 0.11)
    for i in range(4):                                      # containerized gensets
        x = -1.55 + i * 0.85
        box((x, 0.55, 0.32), (0.72, 0.50, 0.44), 1)
        louvered_block((x, 0.29, 0.34), (0.66, 0.05, 0.30), 5, 10, "y")
        louvered_block((x, 0.81, 0.34), (0.66, 0.05, 0.30), 5, 10, "y")
        box((x, 0.55, 0.58), (0.78, 0.56, 0.07), 2)
        fan_unit((x - 0.22, 0.55, 0.62), 0.09)
        fan_unit((x + 0.22, 0.55, 0.62), 0.09)
        cyl((x + 0.30, 0.70, 0.60), (x + 0.30, 0.70, 0.95), 0.035, 6, verts=12)
        cyl((x + 0.30, 0.70, 0.95), (x + 0.30, 0.70, 1.05), 0.055, 5, verts=12)
        box((x - 0.30, 0.30, 0.24), (0.14, 0.04, 0.16), 3, bevel=0)
        box((x, 0.55, 0.12), (0.80, 0.58, 0.06), 0)
        for k in range(3):
            box((x - 0.2 + k * 0.2, 0.28, 0.16), (0.05, 0.03, 0.05), 7, bevel=0)
    box((0, -0.35, 0.14), (3.6, 0.7, 0.08), 0)              # bund
    for i in range(2):
        tank_horizontal((-1.0 + i * 1.6, -0.35, 0.18), 0.16, 0.8, 2, 0.0)
        cyl((-1.0 + i * 1.6, -0.35, 0.40), (-1.0 + i * 1.6, -0.35, 0.50), 0.03, 5, verts=10)
    pipe_run((-1.6, -0.10, 0.24), (1.6, -0.10, 0.24), 0.026, 5, supports=6)
    for i in range(4):
        pipe_run((-1.55 + i * 0.85, -0.10, 0.24), (-1.55 + i * 0.85, 0.30, 0.24), 0.020, 2, supports=2)
    cyl((1.85, -0.35, 0.18), (1.85, -0.35, 0.85), 0.20, 2, verts=22)  # bulk tank
    cone((1.85, -0.35, 0.92), 0.21, 0.04, 0.14, 1, verts=22)
    for i in range(3):
        torus((1.85, -0.35, 0.35 + i * 0.2), 0.205, 0.012, 5, major_seg=22, minor_seg=5)
    cyl((1.85, -0.14, 0.20), (1.85, 0.10, 0.20), 0.03, 5, verts=10)
    box((1.85, 0.20, 0.24), (0.20, 0.16, 0.20), 1)
    box((-1.95, -0.85, 0.30), (0.5, 0.34, 0.40), 1)         # switchgear / control
    louvered_block((-1.95, -0.85, 0.54), (0.44, 0.28, 0.10), 5, 6, "y")
    box((-1.95, -0.85, 0.62), (0.56, 0.40, 0.06), 2)
    window_grid((-1.95, -1.03, 0.36), 0.3, 0.14, 2, 1, 4, "y")
    switchyard((-0.6, -0.95, 0.10), rows=1, bays=3, span=0.30)
    pipe_rack((0.6, -0.95, 0.10), 1.0, levels=2)
    box((1.5, -0.95, 0.24), (0.24, 0.18, 0.28), 7)          # fire cabinet
    box((1.5, -1.05, 0.30), (0.14, 0.02, 0.10), 0, bevel=0)
    fence_loop((0, 0.05, 0.10), 4.3, 3.3, 0.28, posts=16)
    floodlight((-2.0, 1.3, 0.10), 0.85)
    floodlight((2.0, 1.3, 0.10), 0.85)
    lamp_post((0.0, -1.15, 0.10), 0.46)
    barrier_gate((-2.1, -1.35, 0.10), 0.0, 0.4)

    # Detail pass (reference: packaged genset yards - rain-capped stacks with
    # silencers, day-tank level gauges and bund drains, battery/charger room,
    # local control panels, lifting lugs, spill kits, grating walkway, tray).
    for i in range(4):
        x = -1.55 + i * 0.85
        cyl((x + 0.30, 0.70, 1.05), (x + 0.30, 0.70, 1.12), 0.055, 5, verts=12)
        cone((x + 0.30, 0.70, 1.16), 0.075, 0.02, 0.05, 5, verts=12)   # rain cap
        box((x + 0.30, 0.86, 0.70), (0.16, 0.14, 0.22), 1)             # silencer
        pipe_run((x + 0.30, 0.78, 0.72), (x + 0.30, 0.86, 0.72), 0.02, 5, supports=1)
        for k in range(2):                                             # lifting lugs
            box((x - 0.28 + k * 0.56, 0.55, 0.62), (0.03, 0.03, 0.05), 5, bevel=0)
        box((x - 0.34, 0.30, 0.30), (0.05, 0.06, 0.12), 0)             # local panel
        box((x - 0.36, 0.30, 0.34), (0.02, 0.04, 0.06), 4, bevel=0)
        cyl((x - 0.20, 0.36, 0.62), (x - 0.20, 0.36, 0.70), 0.02, 6, verts=10)
    box((0.0, 0.20, 0.13), (3.4, 0.16, 0.03), 5, bevel=0)              # walkway grating
    for i in range(9):
        box((-1.6 + i * 0.4, 0.20, 0.15), (0.02, 0.16, 0.012), 0, bevel=0)
    box((-1.95, -0.30, 0.30), (0.30, 0.24, 0.40), 1)                   # battery / charger
    box((-1.95, -0.30, 0.54), (0.34, 0.28, 0.06), 2)
    for i in range(3):
        box((-2.06 + i * 0.11, -0.18, 0.24), (0.08, 0.04, 0.24), 0, bevel=0)
    box((1.85, -0.85, 0.20), (0.18, 0.14, 0.24), 0)                    # spill kit
    box((1.85, -0.93, 0.26), (0.14, 0.02, 0.10), 7, bevel=0)
    for i in range(2):                                                 # extinguishers
        cyl((1.30 + i * 0.14, -1.10, 0.14), (1.30 + i * 0.14, -1.10, 0.34), 0.035, 7, verts=12)
    pipe_rack((-1.4, -0.60, 0.10), 2.4, levels=1)                      # cable tray to switchgear
    cyl((1.95, 0.60, 0.12), (1.95, 0.60, 1.30), 0.02, 6, verts=10)     # exhaust monitor
    box((1.95, 0.60, 1.32), (0.10, 0.06, 0.06), 1, bevel=0)


# ===========================================================================
# 19. Inverter / battery station (инверторная, BESS): PCS, HVAC, fire main
# ===========================================================================
def build_colony_inverter():
    box((0, 0, 0.05), (4.4, 3.4, 0.10), 6)
    road_strip((-2.2, -1.35, 0.11), (2.2, -1.35, 0.11), 0.32, 0.11)
    for i in range(4):                                      # battery containers
        x = -1.5 + i * 0.82
        box((x, 0.65, 0.32), (0.70, 0.46, 0.44), 1)
        box((x, 0.65, 0.58), (0.76, 0.52, 0.07), 2)
        for k in range(6):
            box((x - 0.28 + k * 0.11, 0.41, 0.34), (0.05, 0.03, 0.30), 5, bevel=0)
        box((x + 0.24, 0.90, 0.40), (0.20, 0.10, 0.18), 0)  # HVAC
        fan_unit((x + 0.24, 0.90, 0.52), 0.06)
        box((x - 0.24, 0.90, 0.36), (0.16, 0.08, 0.14), 3)
        box((x, 0.40, 0.20), (0.18, 0.03, 0.12), 4, bevel=0)
        box((x, 0.65, 0.12), (0.78, 0.54, 0.06), 0)
    for i in range(2):                                      # PCS inverter cabinets
        x = -1.1 + i * 1.5
        box((x, -0.25, 0.36), (0.60, 0.40, 0.52), 0)
        louvered_block((x, -0.46, 0.38), (0.54, 0.05, 0.34), 5, 9, "y")
        louvered_block((x, -0.04, 0.38), (0.54, 0.05, 0.34), 5, 9, "y")
        box((x, -0.25, 0.66), (0.66, 0.46, 0.07), 2)
        for k in range(3):
            fan_unit((x - 0.18 + k * 0.18, -0.25, 0.70), 0.06)
        box((x, -0.47, 0.24), (0.22, 0.03, 0.14), 3, bevel=0)
        transformer((x + 0.5, -0.25, 0.10), 0.65)
    pipe_run((-1.9, 0.20, 0.62), (1.9, 0.20, 0.62), 0.022, 7, supports=8)  # fire main
    for i in range(5):
        x = -1.6 + i * 0.8
        cyl((x, 0.20, 0.62), (x, 0.45, 0.62), 0.014, 5, verts=8)
        cone((x, 0.47, 0.62), 0.03, 0.01, 0.05, 5, verts=10)
    cyl((-2.0, 0.20, 0.10), (-2.0, 0.20, 0.70), 0.05, 7, verts=14)
    switchyard((0.0, -0.95, 0.10), rows=1, bays=4, span=0.32)
    pipe_rack((-1.6, -0.75, 0.10), 1.2, levels=2)
    box((1.85, -0.85, 0.32), (0.44, 0.34, 0.44), 1)         # control room
    louvered_block((1.85, -0.85, 0.58), (0.38, 0.28, 0.10), 5, 5, "y")
    box((1.85, -0.85, 0.66), (0.50, 0.40, 0.06), 2)
    window_grid((1.85, -1.03, 0.38), 0.28, 0.16, 2, 1, 4, "y")
    cyl((1.85, -0.60, 0.66), (1.85, -0.60, 0.95), 0.012, 5, verts=8)
    sphere((1.85, -0.60, 0.98), 0.03, 7, segs=8, rings=6)
    for i in range(4):
        box((-1.5 + i * 0.82, 0.10, 0.115), (0.6, 0.06, 0.03), 0, bevel=0)
    fence_loop((0, 0.05, 0.10), 4.3, 3.3, 0.28, posts=16)
    floodlight((-2.0, 1.3, 0.10), 0.85)
    floodlight((2.0, 1.3, 0.10), 0.85)
    lamp_post((0.4, -1.15, 0.10), 0.46)

    # Detail pass (reference: BESS risk-layout guidance - fire separation
    # bunds between units, DC switch cabinets, relief vent panels, deluge
    # valve skid with water tank and pump, CCTV, turnstile gate, signage).
    for i in range(3):
        box((-1.09 + i * 0.82, 0.65, 0.14), (0.10, 0.52, 0.16), 0)     # separation bund
    for i in range(4):
        x = -1.5 + i * 0.82
        box((x - 0.30, 0.65, 0.30), (0.10, 0.40, 0.36), 1)             # DC switch cabinet
        box((x - 0.30, 0.44, 0.50), (0.08, 0.02, 0.10), 7, bevel=0)
        for k in range(2):
            box((x + 0.10 + k * 0.14, 0.90, 0.58), (0.10, 0.02, 0.12), 3, bevel=0)  # vents
        box((x, 0.65, 0.60), (0.22, 0.16, 0.03), 5, bevel=0)           # roof vent bank
    box((1.35, 0.20, 0.24), (0.30, 0.24, 0.30), 1)                     # deluge valve skid
    for i in range(3):
        cyl((1.25 + i * 0.10, 0.05, 0.30), (1.25 + i * 0.10, 0.05, 0.44), 0.02, 5, verts=10)
    cyl((1.95, 0.55, 0.10), (1.95, 0.55, 0.62), 0.16, 5, verts=18)     # fire water tank
    cone((1.95, 0.55, 0.68), 0.17, 0.03, 0.10, 1, verts=18)
    box((1.65, 0.55, 0.22), (0.22, 0.18, 0.24), 1)                     # fire pump house
    pipe_run((1.80, 0.55, 0.62), (1.35, 0.20, 0.62), 0.02, 5, supports=3)
    camera_mast((-2.05, -1.10, 0.10), 0.75, 2)
    camera_mast((2.05, 1.30, 0.10), 0.70, 2)
    box((-0.05, -1.62, 0.30), (0.5, 0.10, 0.42), 0)                    # turnstile gate
    for i in range(2):
        cyl((-0.25 + i * 0.4, -1.62, 0.12), (-0.25 + i * 0.4, -1.62, 0.52), 0.02, 5, verts=8)
    box((-0.05, -1.78, 0.55), (0.34, 0.02, 0.22), 4, bevel=0)          # signage panel
    for i in range(5):                                                 # trench covers
        box((-1.6 + i * 0.7, 0.20, 0.115), (0.5, 0.10, 0.02), 0, bevel=0)
    for i in range(4):                                                 # earth electrodes
        cyl((-1.8 + i * 1.1, 1.30, 0.10), (-1.8 + i * 1.1, 1.30, 0.24), 0.008, 5, verts=6)


# ===========================================================================
# 20. Wind farm (ветряки): HAWTs, pads, collector substation, met mast
# ===========================================================================
def build_colony_wind():
    box((0, 0, 0.05), (4.8, 3.6, 0.10), 6)
    road_strip((-2.4, -1.40, 0.11), (2.4, -1.40, 0.11), 0.32, 0.11)
    road_strip((-1.55, -1.40, 0.11), (-1.55, 0.55, 0.11), 0.24, 0.11)
    road_strip((0.35, -1.40, 0.11), (0.35, 0.85, 0.11), 0.24, 0.11)
    road_strip((1.85, -1.40, 0.11), (1.85, 0.35, 0.11), 0.24, 0.11)
    wind_turbine((-1.55, 0.75, 0.10), 1.75, 0.60, yaw=0.35)
    wind_turbine((0.35, 1.05, 0.10), 1.55, 0.52, yaw=-0.25)
    wind_turbine((1.85, 0.55, 0.10), 1.35, 0.46, yaw=0.85)
    for (x, y) in ((-1.55, 0.75), (0.35, 1.05), (1.85, 0.55)):
        box((x + 0.24, y, 0.16), (0.16, 0.14, 0.12), 1)     # pad-mounted switch
        box((x + 0.24, y - 0.08, 0.20), (0.08, 0.02, 0.06), 7, bevel=0)
        box((x, y, 0.115), (0.5, 0.06, 0.03), 0, bevel=0)   # cable trench
    box((0.0, -0.75, 0.16), (1.3, 0.7, 0.12), 0)            # collector substation
    transformer((-0.35, -0.75, 0.22), 0.8)
    transformer((0.35, -0.75, 0.22), 0.8)
    switchyard((0.0, -0.45, 0.22), rows=1, bays=3, span=0.30)
    box((0.0, -1.05, 0.34), (0.5, 0.3, 0.36), 1)
    louvered_block((0.0, -1.05, 0.56), (0.44, 0.24, 0.10), 5, 6, "y")
    box((0.0, -1.05, 0.64), (0.56, 0.36, 0.06), 2)
    window_grid((0.0, -1.21, 0.40), 0.3, 0.14, 2, 1, 4, "y")
    cyl((-2.15, 1.35, 0.10), (-2.15, 1.35, 1.45), 0.024, 5, verts=10)  # met mast
    for i in range(4):
        z = 0.4 + i * 0.3
        cyl((-2.15, 1.35, z), (-2.15 + 0.13, 1.35, z), 0.009, 5, verts=6)
        box((-2.02, 1.35, z + 0.02), (0.05, 0.04, 0.03), 3, bevel=0)
    for i in range(3):
        a = i * math.tau / 3
        cable((-2.15, 1.35, 1.35), (-2.15 + 0.45 * math.cos(a), 1.35 + 0.45 * math.sin(a), 0.10),
              0.02, 5, segs=4, radius=0.006)
    fence_loop((0, 0.05, 0.10), 4.7, 3.5, 0.24, posts=16)
    barrier_gate((-2.3, -1.40, 0.10), 0.0, 0.4)

    # Detail pass (reference: turbine component guides - tower door, internal
    # ladder and platform, yaw ring, nacelle anemometer/vane and aviation
    # light, hub pitch rings with root bolt circles, grout ring, laydown pad).
    for (x, y, hgt, rot) in ((-1.55, 0.75, 1.75, 0.35), (0.35, 1.05, 1.55, -0.25),
                             (1.85, 0.55, 1.35, 0.85)):
        box((x - 0.075, y - 0.02, 0.10 + 0.22), (0.02, 0.09, 0.22), 1, rot)   # tower door
        for k in range(9):
            box((x - 0.06, y, 0.30 + k * 0.14), (0.02, 0.05, 0.012), 5, rot, bevel=0)
        torus((x, y, 0.10 + hgt * 0.96), 0.055, 0.008, 5, major_seg=18, minor_seg=5)  # yaw ring
        cyl((x, y, 0.10 + hgt + 0.10), (x, y, 0.10 + hgt + 0.14), 0.03, 5, verts=12)
        box((x + 0.02, y - 0.05, 0.10 + hgt + 0.16), (0.05, 0.04, 0.03), 1, rot)  # anemometer
        box((x + 0.02, y + 0.05, 0.10 + hgt + 0.16), (0.05, 0.04, 0.03), 1, rot)  # wind vane
        sphere((x, y, 0.10 + hgt + 0.20), 0.022, 7, segs=8, rings=6)              # aviation light
        torus((x, y, 0.10 + 0.06), 0.085, 0.012, 0, major_seg=20, minor_seg=6)    # grout ring
        box((x + 0.10, y + 0.10, 0.14), (0.08, 0.06, 0.08), 1, rot)               # cable entry
        for k in range(4):                                                        # platform rails
            a2 = rot + k * math.tau / 4
            box((x + 0.09 * math.cos(a2), y + 0.09 * math.sin(a2), 0.10 + hgt * 0.5),
                (0.012, 0.012, 0.10), 5, bevel=0)
    for (x, y, hgt, rot) in ((-1.55, 0.75, 1.75, 0.35), (0.35, 1.05, 1.55, -0.25),
                             (1.85, 0.55, 1.35, 0.85)):
        hx = x + math.cos(rot) * 0.20
        hy = y + math.sin(rot) * 0.20
        for k in range(3):                                     # blade root bolt circles
            blade_root_bolt_circle((hx, hy, 0.10 + hgt + 0.03), 0.052, 10, k * math.tau / 3)
    box((-0.6, 1.55, 0.115), (0.9, 0.7, 0.03), 0)              # laydown / crane pad
    for i in range(4):
        box((-0.9 + i * 0.22, 1.55, 0.13), (0.02, 0.66, 0.012), 0, bevel=0)
    container((-0.6, 1.30, 0.14), 0.0, 2)
    for i in range(3):                                         # transformer pads
        box((-1.55 + i * 1.7, -0.20, 0.14), (0.24, 0.20, 0.12), 0)
        box((-1.55 + i * 1.7, -0.20, 0.26), (0.16, 0.14, 0.12), 1)
    box((0.0, -1.20, 0.16), (0.34, 0.24, 0.16), 1)             # spare parts container
    box((0.0, -1.20, 0.28), (0.38, 0.28, 0.05), 2)
    for i in range(2):                                         # road culverts
        torus((-1.2 + i * 1.55, -1.55, 0.13), 0.07, 0.014, 5, major_seg=14, minor_seg=5)
    floodlight((-2.2, -1.0, 0.10), 0.8)
    floodlight((2.2, -1.0, 0.10), 0.8)


ASSETS = [
    ("colony_dome", build_colony_dome, 3.0, (1.8, -2.4, 1.4)),
    ("colony_factory", build_colony_factory, 3.4, (2.0, -2.3, 1.5)),
    ("colony_pad", build_colony_pad, 3.2, (2.0, -2.4, 1.6)),
    ("colony_nuclear", build_colony_nuclear, 4.2, (2.1, -2.3, 1.5)),
    ("colony_hydro", build_colony_hydro, 4.6, (2.2, -2.2, 1.4)),
    ("colony_thermal", build_colony_thermal, 4.6, (2.1, -2.4, 1.5)),
    ("colony_datacenter", build_colony_datacenter, 4.2, (2.0, -2.3, 1.5)),
    ("colony_townhall", build_colony_townhall, 3.6, (2.0, -2.4, 1.5)),
    ("colony_parliament", build_colony_parliament, 4.0, (2.0, -2.4, 1.5)),
    ("colony_police", build_colony_police, 3.6, (2.0, -2.4, 1.5)),
    ("colony_warehouse", build_colony_warehouse, 4.2, (2.1, -2.3, 1.5)),
    ("colony_military", build_colony_military, 4.2, (2.1, -2.3, 1.5)),
    ("colony_houses", build_colony_houses, 3.8, (2.0, -2.4, 1.5)),
    ("colony_apartments", build_colony_apartments, 4.2, (2.0, -2.4, 1.5)),
    ("colony_pipeline", build_colony_pipeline, 4.4, (2.1, -2.3, 1.4)),
    ("colony_powerline", build_colony_powerline, 4.4, (2.1, -2.3, 1.4)),
    ("colony_solar", build_colony_solar, 4.4, (2.1, -2.3, 1.4)),
    ("colony_diesel", build_colony_diesel, 4.0, (2.1, -2.3, 1.5)),
    ("colony_inverter", build_colony_inverter, 4.0, (2.1, -2.3, 1.5)),
    ("colony_wind", build_colony_wind, 4.4, (2.1, -2.3, 1.4)),
]

# Slot semantics stay the same across assets (0 structure, 1 walls, 2 roof,
# 3 glazing, 4 lit windows, 5 alloy, 6 ground, 7 beacon); only the tile roles
# and a couple of colours change so civic masonry reads as brick, not hull.
BRICK = ((0.55, 0.32, 0.25), 0.74, 0.04, (0.0, 0.0, 0.0))
ROOF_METAL = ((0.30, 0.33, 0.38), 0.42, 0.62, (0.0, 0.0, 0.0))
CIVIC_ROLES = ("concrete", "masonry", "roof", "glass", "window",
               "brushed", "asphalt", "lens")
ATLAS_CONFIG = {
    "colony_townhall": {"roles": CIVIC_ROLES, "overrides": {1: BRICK, 2: ROOF_METAL}},
    "colony_parliament": {"roles": CIVIC_ROLES, "overrides": {1: BRICK, 2: ROOF_METAL}},
    "colony_police": {"roles": CIVIC_ROLES,
                      "overrides": {1: ((0.42, 0.44, 0.48), 0.66, 0.10, (0.0, 0.0, 0.0)),
                                    2: ROOF_METAL}},
    "colony_warehouse": {"roles": CIVIC_ROLES,
                         "overrides": {1: ((0.50, 0.52, 0.55), 0.55, 0.35, (0.0, 0.0, 0.0)),
                                       2: ROOF_METAL}},
    "colony_military": {"roles": CIVIC_ROLES,
                        "overrides": {1: ((0.36, 0.38, 0.32), 0.72, 0.08, (0.0, 0.0, 0.0)),
                                      2: ((0.28, 0.30, 0.26), 0.60, 0.30, (0.0, 0.0, 0.0))}},
    "colony_houses": {"roles": ("concrete", "siding", "roof", "glass", "window",
                                "brushed", "asphalt", "lens"),
                      "overrides": {1: ((0.76, 0.72, 0.64), 0.70, 0.05, (0.0, 0.0, 0.0)),
                                    2: ((0.34, 0.26, 0.24), 0.55, 0.35, (0.0, 0.0, 0.0))}},
    "colony_apartments": {"roles": ("concrete", "panel", "roof", "glass", "window",
                                    "brushed", "asphalt", "lens"),
                          "overrides": {1: ((0.62, 0.62, 0.60), 0.66, 0.10, (0.0, 0.0, 0.0))}},
    "colony_pipeline": {"roles": ("concrete", "rib", "quilt", "glass", "window",
                                  "brushed", "asphalt", "lens")},
    "colony_powerline": {"roles": ("concrete", "rib", "quilt", "glass", "window",
                                   "brushed", "asphalt", "lens")},
    "colony_solar": {"roles": ("concrete", "rib", "quilt", "cells", "window",
                               "brushed", "asphalt", "lens")},
    "colony_diesel": {"roles": ("concrete", "rib", "quilt", "glass", "window",
                                "brushed", "asphalt", "lens")},
    "colony_inverter": {"roles": ("concrete", "rib", "quilt", "glass", "window",
                                  "brushed", "asphalt", "lens")},
    "colony_wind": {"roles": ("concrete", "panel", "quilt", "glass", "window",
                              "brushed", "asphalt", "lens")},
}


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
        MATERIAL = make_atlas(name, **ATLAS_CONFIG.get(name, {}))
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
