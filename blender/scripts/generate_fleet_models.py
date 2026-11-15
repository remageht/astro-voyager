"""Four high-detail own procedural fleet assets for Astro Voyager.

  rover   - Mars rover: rocker-bogie suspension, 6 grouser wheels, RTG,
            remote-sensing mast with camera head, folded robotic arm + turret,
            high/low-gain antennas, deck greebles.
  colony  - O'Neill ring colony: central docking spine, spinning torus habitat
            with lit window bands, six truss spokes, solar mirror wings.
  lander  - crewed lunar lander: gold descent octagon + descent engine bell,
            ascent crew module, four splayed legs with footpads, tanks, RCS,
            airlock, ladder, antennas.
  sail    - solar sail probe: billowed reflective square sail with quadrant
            seams, four diagonal truss booms, central micro-bus, tip vanes.

Shared PBR-atlas + OBJ pipeline (albedo/normal/rough/metal/emissive BMP),
normalized to game sizes, exported to res/models/<name>.obj, and a
1600x1066 @64-sample Cycles hero per asset. No external assets.
Run: blender --background --python blender/scripts/generate_fleet_models.py
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
OUT = ROOT / "blender/out/fleet"
PALETTE = [
    ((0.82, 0.84, 0.87), 0.34, 0.45, (0.0, 0.0, 0.0)),   # 0 white hull
    ((0.06, 0.07, 0.09), 0.34, 0.80, (0.0, 0.0, 0.0)),   # 1 dark titanium
    ((0.80, 0.58, 0.16), 0.28, 0.78, (0.0, 0.0, 0.0)),   # 2 gold MLI
    ((0.03, 0.06, 0.16), 0.20, 0.45, (0.0, 0.0, 0.0)),   # 3 optics dark blue
    ((0.10, 0.80, 0.85), 0.20, 0.10, (0.10, 0.90, 0.95)),# 4 cyan windows
    ((0.60, 0.62, 0.66), 0.30, 0.85, (0.0, 0.0, 0.0)),   # 5 bright metal
    ((0.02, 0.03, 0.05), 0.40, 0.30, (0.0, 0.0, 0.0)),   # 6 dark recess
    ((0.95, 0.55, 0.12), 0.25, 0.10, (1.0, 0.6, 0.15)),  # 7 amber beacon
]
MATERIAL = None
PARTS = []


def make_atlas(prefix, overrides=None, size=1024):
    """Shared flip-safe atlas (asset_atlas enforces np.minimum(yy, (t - 1) - yy) and GRID - 1 - gy)."""
    return asset_atlas.build(f"{prefix}_PBR", PALETTE, MODELS, prefix, size=size,
                             overrides=overrides, seed_base=1000)



def finish(obj, slot, smooth=False, bevel=0.006):
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
        mod.angle_limit = math.radians(40)
        bpy.ops.object.modifier_apply(modifier=mod.name)
    PARTS.append(obj)
    return obj


def box(center, dims, slot=0, rotation=0.0, bevel=0.006):
    bpy.ops.mesh.primitive_cube_add(size=1.0, location=center)
    obj = bpy.context.object
    obj.scale = dims
    obj.rotation_euler.z = rotation
    uv = obj.data.uv_layers.active
    for polygon in obj.data.polygons:
        for corner, index in enumerate(polygon.loop_indices):
            uv.data[index].uv = ((0, 0), (1, 0), (1, 1), (0, 1))[corner]
    return finish(obj, slot, bevel=bevel)


def cylinder(start, end, radius, slot=0, vertices=48):
    a, b = Vector(start), Vector(end)
    bpy.ops.mesh.primitive_cylinder_add(vertices=vertices, radius=radius,
                                        depth=(b - a).length, location=(a + b) / 2)
    obj = bpy.context.object
    obj.rotation_euler = (b - a).to_track_quat("Z", "Y").to_euler()
    return finish(obj, slot, smooth=True)


def cone(center, r1, r2, depth, slot=0, vertices=64):
    bpy.ops.mesh.primitive_cone_add(vertices=vertices, radius1=r1, radius2=r2,
                                    depth=depth, location=center)
    return finish(bpy.context.object, slot, smooth=True)


def sphere(center, radius, slot=0, zscale=1.0):
    bpy.ops.mesh.primitive_uv_sphere_add(segments=48, ring_count=24,
                                         radius=radius, location=center)
    obj = bpy.context.object
    obj.scale.z = zscale
    return finish(obj, slot, smooth=True)


def torus(center, radius, tube, slot=1, major=96, minor=16):
    bpy.ops.mesh.primitive_torus_add(major_segments=major, minor_segments=minor,
                                     major_radius=radius, minor_radius=tube,
                                     location=center)
    obj = bpy.context.object
    for polygon in obj.data.polygons:
        polygon.use_smooth = True
    return finish(obj, slot, bevel=0)


def prism(center, radius, depth, sides, slot=1, bevel=0.006):
    bpy.ops.mesh.primitive_cylinder_add(vertices=sides, radius=radius, depth=depth,
                                        location=center)
    return finish(bpy.context.object, slot, bevel=bevel)


def wheel(center, radius, width, slot=1):
    cx, cy, cz = center
    cylinder((cx, cy - width / 2, cz), (cx, cy + width / 2, cz), radius, slot, vertices=32)
    cylinder((cx, cy - width / 2 - 0.01, cz), (cx, cy + width / 2 + 0.01, cz),
             radius * 0.45, 5, vertices=24)
    for g in range(20):  # grousers / tread
        a = g * math.tau / 20
        gx = cx + radius * math.cos(a)
        gz = cz + radius * math.sin(a)
        b = box((gx, cy, gz), (0.03, width, 0.02), 5, 0, bevel=0)
        b.rotation_euler.y = -a
    for s in range(6):  # hub spokes
        a = s * math.tau / 6
        cylinder((cx, cy, cz),
                 (cx + radius * 0.42 * math.cos(a), cy, cz + radius * 0.42 * math.sin(a)),
                 0.012, 5, vertices=8)


# ---------------------------------------------------------------- rover
def build_rover():
    # Warm electronics box: bevelled chassis with recessed instrument bays,
    # side louvers and corner brackets instead of one plain gold brick.
    box((0, 0, 0.35), (0.95, 0.7, 0.34), 2)
    for sx in (-1, 1):
        for sz in (0.22, 0.48):
            box((sx * 0.48, 0, sz), (0.02, 0.5, 0.16), 6, 0, bevel=0)   # bays
        for l in range(7):                                               # louvers
            box((sx * 0.478, -0.28 + l * 0.093, 0.35), (0.012, 0.07, 0.2),
                5, 0, bevel=0)
    for cx in (-0.44, 0.44):                                             # brackets
        for cy in (-0.32, 0.32):
            for cz in (0.2, 0.5):
                box((cx, cy, cz), (0.07, 0.07, 0.05), 5, 0, bevel=0.004)
    box((0, 0, 0.53), (0.9, 0.66, 0.04), 0)                              # deck plate
    for d in range(9):                                                   # deck rail
        box((-0.4 + d * 0.1, -0.33, 0.565), (0.02, 0.02, 0.05), 5, 0, bevel=0)
    for gx, gy, gd, gs in ((-0.3, 0.15, 0.16, 1), (0.0, 0.15, 0.2, 1),
                           (0.3, 0.15, 0.16, 1), (0.22, -0.2, 0.14, 6)):
        box((gx, gy, 0.57), (gd, gd * 1.2, 0.06), gs)                   # deck boxes
    for t in range(5):                                                   # sample tubes
        cylinder((0.05 + t * 0.08, -0.05, 0.55), (0.05 + t * 0.08, -0.05, 0.63),
                 0.022, 5, vertices=16)
    for c in range(3):                                                   # conduit runs
        cylinder((-0.45, -0.2 + c * 0.2, 0.55), (0.42, -0.24 + c * 0.2, 0.55),
                 0.012, 1, vertices=8)
    box((0, 0.36, 0.3), (0.7, 0.02, 0.22), 5)                            # radiator

    # Rocker-bogie suspension: front bogie, rear rocker, differential bar.
    for side in (-1, 1):
        y = side * 0.5
        front_hub = Vector((0.55, y, 0.16))
        mid_hub = Vector((-0.05, y, 0.16))
        rear_hub = Vector((-0.55, y, 0.16))
        pivot = Vector((0.0, y, 0.34))
        rocker_top = Vector((0.3, y, 0.42))
        bogie_pivot = Vector((-0.3, y, 0.4))
        cylinder(pivot, rocker_top, 0.035, 5, vertices=12)               # rocker arm
        cylinder(rocker_top, front_hub, 0.03, 5, vertices=12)
        cylinder(rocker_top, mid_hub, 0.03, 5, vertices=12)
        cylinder(bogie_pivot, pivot, 0.032, 5, vertices=12)              # bogie beam
        cylinder(bogie_pivot, rear_hub, 0.028, 5, vertices=12)
        cylinder(mid_hub, rear_hub, 0.028, 5, vertices=12)
        for hub in (front_hub, mid_hub, rear_hub):                       # wheel forks
            cylinder(hub - Vector((0, 0.09, 0)), hub + Vector((0, 0.09, 0)),
                     0.05, 5, vertices=16)
            for s in range(4):
                a = s * math.tau / 4
                cylinder(hub + Vector((0.05 * math.cos(a), 0, 0.05 * math.sin(a))),
                         hub - Vector((0.05 * math.cos(a), 0, 0.05 * math.sin(a))),
                         0.012, 5, vertices=8)
        for hub in (pivot, bogie_pivot):
            cylinder(hub - Vector((0, 0.06, 0)), hub + Vector((0, 0.06, 0)),
                     0.055, 1, vertices=20)                               # pivot caps
    cylinder((0, -0.5, 0.42), (0, 0.5, 0.42), 0.03, 5, vertices=12)      # diff bar
    for wx, wy in ((0.55, 0.58), (-0.05, 0.58), (-0.55, 0.58),
                   (0.55, -0.58), (-0.05, -0.58), (-0.55, -0.58)):
        wheel((wx, wy, 0.16), 0.16, 0.14, 1)

    # RTG at the rear on a shadow-shielded boom.
    rtg_dir = Vector((-1, 0, 0.35)).normalized()
    base = Vector((-0.5, 0, 0.45))
    cylinder(base, base + rtg_dir * 0.3, 0.03, 1, vertices=12)
    c = base + rtg_dir * 0.5
    cylinder(c - rtg_dir * 0.16, c + rtg_dir * 0.16, 0.11, 1, vertices=24)
    for f in range(12):
        a = f * math.tau / 12
        perp = rtg_dir.cross(Vector((0, 1, 0))).normalized()
        perp2 = rtg_dir.cross(perp).normalized()
        off = (perp * math.cos(a) + perp2 * math.sin(a)) * 0.14
        fin = box(tuple(c + off), (0.04, 0.008, 0.28), 5, 0, bevel=0)
        fin.rotation_euler = rtg_dir.to_track_quat("Z", "Y").to_euler()
    box(tuple(c + rtg_dir * 0.19), (0.02, 0.26, 0.26), 6, 0, bevel=0)     # shadow shield

    # Remote-sensing mast: two joints, dual-head camera, colour calibration target.
    cylinder((0.3, -0.2, 0.55), (0.3, -0.2, 0.85), 0.035, 5, vertices=12)
    cylinder((0.3, -0.2, 0.85), (0.36, -0.2, 1.05), 0.03, 5, vertices=12)
    cylinder((0.3, -0.2, 0.85), (0.3, -0.2, 0.87), 0.055, 1, vertices=20)
    box((0.36, -0.2, 1.12), (0.24, 0.14, 0.13), 0)
    for ex in (-0.06, 0.06):
        cylinder((0.36 + ex, -0.29, 1.12), (0.36 + ex, -0.34, 1.12), 0.032, 3, vertices=20)
        cylinder((0.36 + ex, -0.34, 1.12), (0.36 + ex, -0.355, 1.12), 0.022, 6, vertices=16)
    box((0.36, -0.2, 1.22), (0.16, 0.1, 0.03), 5)                         # top solar panel
    target = Vector((0.12, -0.34, 0.62))
    cylinder(target, target + Vector((0, -0.03, 0)), 0.07, 5, vertices=24)
    for s in range(6):                                                    # calibration wedges
        a = s * math.tau / 6
        wedge = target + Vector((0.045 * math.cos(a), -0.035, 0.045 * math.sin(a)))
        box(tuple(wedge), (0.03, 0.012, 0.03), (0, 2, 3, 4, 5, 7)[s], a, bevel=0)

    # Folded robotic arm with turret, drill and scoop.
    shoulder = Vector((0.45, 0.2, 0.4))
    elbow = Vector((0.75, 0.2, 0.2))
    wrist = Vector((0.7, 0.2, -0.05))
    cylinder(shoulder, elbow, 0.035, 5, vertices=12)
    cylinder(elbow, wrist, 0.03, 5, vertices=12)
    for p in (shoulder, elbow):
        cylinder(p - Vector((0, 0.06, 0)), p + Vector((0, 0.06, 0)), 0.05, 1, vertices=18)
    prism(tuple(wrist), 0.09, 0.1, 8, 1)
    cylinder(wrist, wrist + Vector((0, 0, -0.1)), 0.02, 5, vertices=12)
    cylinder(wrist + Vector((0.06, 0, -0.06)), wrist + Vector((0.13, 0, -0.1)),
             0.015, 5, vertices=10)
    box(tuple(wrist + Vector((0.15, 0, -0.11))), (0.06, 0.08, 0.015), 5, 0, bevel=0)

    # Antennas: flat high-gain plate, low-gain dish, UHF whip, beacon.
    prism((-0.2, -0.2, 0.62), 0.16, 0.03, 6, 0)
    cylinder((-0.2, -0.2, 0.56), (-0.2, -0.2, 0.62), 0.02, 5, vertices=10)
    cone((-0.05, -0.36, 0.6), 0.11, 0.02, 0.07, 0, vertices=24)
    cylinder((-0.35, 0.2, 0.55), (-0.35, 0.2, 0.85), 0.01, 5, vertices=8)
    sphere((-0.35, 0.2, 0.86), 0.02, 4)
    box((0.44, 0.3, 0.56), (0.03, 0.03, 0.02), 7, 0, bevel=0)


# ---------------------------------------------------------------- colony
def build_colony():
    ring_r, tube = 1.6, 0.26
    # central docking spine with collar rings and counterweight bell
    cylinder((0, 0, -1.1), (0, 0, 1.1), 0.22, 0, vertices=48)
    for z in (-1.0, -0.5, 0.0, 0.5, 1.0):
        torus((0, 0, z), 0.24, 0.035, 1, major=48, minor=10)
    prism((0, 0, 1.2), 0.16, 0.16, 12, 5)            # fore docking port
    prism((0, 0, -1.2), 0.16, 0.16, 12, 5)           # aft port
    cylinder((0, 0, 1.28), (0, 0, 1.42), 0.09, 1, vertices=20)
    for s in range(8):                               # hub window ring
        a = s * math.tau / 8
        box((0.2 * math.cos(a), 0.2 * math.sin(a), 0.95), (0.07, 0.07, 0.12), 4, a, bevel=0)
    # habitat torus ring
    torus((0, 0, 0), ring_r, tube, 0, major=192, minor=32)
    # lit window bands riding on the outer surface of the ring, so they still
    # read at the ~0.24-unit in-game size
    for off in (-0.13, 0.13):
        torus((0, 0, off), ring_r + 0.19, 0.07, 4, major=192, minor=12)
    # ring frame ribs and service lines
    for i in range(64):
        a = i * math.tau / 64
        box(((ring_r + tube * 0.62) * math.cos(a), (ring_r + tube * 0.62) * math.sin(a), 0),
            (0.05, 0.06, 0.62), 1, a, bevel=0)
    for i in range(32):
        a = i * math.tau / 32 + math.tau / 64
        cylinder((ring_r * math.cos(a), ring_r * math.sin(a), -0.3),
                 (ring_r * math.cos(a), ring_r * math.sin(a), 0.3), 0.018, 5, vertices=8)
    # four docking hubs with approach lights on the rim
    for i in range(4):
        a = i * math.tau / 4
        hub = Vector(((ring_r + tube) * math.cos(a), (ring_r + tube) * math.sin(a), 0))
        prism(tuple(hub), 0.14, 0.16, 8, 5)
        cylinder(tuple(hub), tuple(hub + Vector((0.12 * math.cos(a), 0.12 * math.sin(a), 0))),
                 0.06, 1, vertices=16)
        box(tuple(hub + Vector((0, 0, 0.2))), (0.05, 0.05, 0.04), 7, a, bevel=0)
    # six truss spokes with cross bracing and outboard flanges
    for i in range(6):
        a = i * math.tau / 6
        inner = Vector((0.24 * math.cos(a), 0.24 * math.sin(a), 0))
        outer = Vector(((ring_r - tube * 0.4) * math.cos(a),
                        (ring_r - tube * 0.4) * math.sin(a), 0))
        cylinder(inner, outer, 0.05, 5, vertices=12)
        for dz in (-0.09, 0.09):
            up = Vector((0, 0, dz))
            cylinder(inner + up, outer + up, 0.022, 5, vertices=8)
        for t in (0.25, 0.5, 0.75):
            p = inner.lerp(outer, t)
            cylinder(tuple(p + Vector((0, 0, -0.09))), tuple(p + Vector((0, 0, 0.09))),
                     0.016, 5, vertices=8)
            torus(tuple(p), 0.07, 0.012, 1, major=10, minor=6)
        torus(tuple(outer), 0.1, 0.02, 1, major=16, minor=8)
    # solar mirror wings off the spine, framed and segmented
    for side in (-1, 1):
        cylinder((0, 0, side * 0.7), (0, side * 1.0, side * 0.7), 0.035, 1, vertices=12)
        box((0, side * 1.5, side * 0.7), (0.92, 0.72, 0.02), 3)
        for e in range(2):
            box((0, side * (1.5 - 0.36 + e * 0.72), side * 0.7), (0.94, 0.03, 0.05), 5, 0, bevel=0)
            box((-0.46 + e * 0.92, side * 1.5, side * 0.7), (0.03, 0.74, 0.05), 5, 0, bevel=0)
        for d in range(12):
            box((-0.44 + d * 0.08, side * 1.5, side * 0.7), (0.006, 0.7, 0.028), 1, 0, bevel=0)
        for t in (-0.3, 0.3):
            cylinder((0, side * 1.0, side * 0.7), (t, side * 1.5, side * 0.7),
                     0.012, 5, vertices=8)
    # radiator fins off the ring
    for i in range(6):
        a = i * math.tau / 6 + math.tau / 12
        box(((ring_r + 0.1) * math.cos(a), (ring_r + 0.1) * math.sin(a), -0.35),
            (0.3, 0.02, 0.16), 5, a, bevel=0)
    cylinder((0.3, 0.3, 1.42), (0.55, 0.55, 1.75), 0.012, 5, vertices=8)
    cone((0.58, 0.58, 1.78), 0.13, 0.02, 0.07, 0, vertices=24)
    sphere((0, 0, 1.32), 0.05, 7)


# ---------------------------------------------------------------- lander
def build_lander():
    # descent stage octagon (gold foil)
    prism((0, 0, 0.0), 0.7, 0.5, 8, 2)
    torus((0, 0, 0.25), 0.72, 0.03, 1, major=8, minor=8)
    torus((0, 0, -0.25), 0.72, 0.03, 1, major=8, minor=8)
    # descent engine bell
    cone((0, 0, -0.5), 0.12, 0.34, 0.4, 1)
    cylinder((0, 0, -0.28), (0, 0, -0.34), 0.1, 5, vertices=24)
    # propellant tanks around descent stage
    for i in range(4):
        a = i * math.tau / 4 + math.pi / 4
        sphere((0.55 * math.cos(a), 0.55 * math.sin(a), 0.0), 0.2, 5)
    # ascent crew module
    cylinder((0, 0, 0.25), (0, 0, 0.75), 0.42, 0, vertices=32)
    cone((0, 0, 0.9), 0.42, 0.2, 0.3, 0)
    prism((0, 0, 1.1), 0.14, 0.12, 12, 5)            # top docking port
    # windows + airlock
    for a in (0.5, -0.5):
        box((0.42 * math.cos(a), 0.42 * math.sin(a), 0.6), (0.02, 0.12, 0.1), 3, a, bevel=0)
    box((0.42, 0, 0.45), (0.03, 0.22, 0.28), 1)      # airlock hatch
    # four splayed landing legs + footpads
    for i in range(4):
        a = i * math.tau / 4 + math.pi / 4
        hip = Vector((0.6 * math.cos(a), 0.6 * math.sin(a), 0.1))
        foot = Vector((1.15 * math.cos(a), 1.15 * math.sin(a), -0.75))
        cylinder(hip, foot, 0.04, 5, vertices=12)
        cylinder(Vector((0.5 * math.cos(a), 0.5 * math.sin(a), -0.2)), foot, 0.025, 5, vertices=10)
        prism(tuple(foot), 0.16, 0.05, 16, 5)
    # ladder + RCS quads + antennas
    for r in range(5):
        box((0.72, 0, 0.1 - r * 0.13), (0.02, 0.12, 0.01), 5, 0, bevel=0)
    for i in range(4):
        a = i * math.tau / 4
        cx, cy = 0.44 * math.cos(a), 0.44 * math.sin(a)
        box((cx, cy, 0.7), (0.06, 0.06, 0.06), 1, a)
        cone((cx * 1.1, cy * 1.1, 0.7), 0.02, 0.008, 0.03, 5, vertices=10)
    cylinder((0.2, 0.2, 1.0), (0.2, 0.2, 1.3), 0.01, 5, vertices=8)
    sphere((0.2, 0.2, 1.32), 0.025, 4)
    prism((-0.25, -0.25, 0.85), 0.14, 0.03, 6, 0)    # HGA
    # crew-module window band, equipment locker and handrails
    torus((0, 0, 0.66), 0.43, 0.05, 4, major=48, minor=10)
    box((0.0, -0.46, 0.42), (0.24, 0.06, 0.22), 1, 0, bevel=0)
    for r in range(3):
        cylinder((0.45, -0.2 + r * 0.2, 0.3), (0.45, -0.2 + r * 0.2, 0.75),
                 0.012, 5, vertices=8)
    for i in range(4):                                # footpad cleats
        a = i * math.tau / 4 + math.pi / 4
        for d in (-0.12, 0.12):
            box((1.15 * math.cos(a) + d * math.sin(a),
                 1.15 * math.sin(a) - d * math.cos(a), -0.79),
                (0.05, 0.05, 0.06), 5, a, bevel=0)
    cone((-0.4, 0.4, 0.62), 0.12, 0.02, 0.06, 0, vertices=20)   # comm dish
    cylinder((-0.4, 0.4, 0.5), (-0.4, 0.4, 0.6), 0.015, 5, vertices=8)


# ---------------------------------------------------------------- sail
def sail_membrane(half, slot, bulge=0.12, n=40):
    verts, faces = [], []
    for i in range(n + 1):
        u = i / n * 2 - 1
        for j in range(n + 1):
            v = j / n * 2 - 1
            z = bulge * (1 - u * u) * (1 - v * v)
            verts.append((u * half, v * half, z))
    for i in range(n):
        for j in range(n):
            a = i * (n + 1) + j
            faces.append((a, a + 1, a + n + 2, a + n + 1))
    mesh = bpy.data.meshes.new("Sail")
    mesh.from_pydata(verts, [], faces)
    mesh.update()
    obj = bpy.data.objects.new("Sail", mesh)
    bpy.context.collection.objects.link(obj)
    bpy.context.view_layer.objects.active = obj
    solid = obj.modifiers.new("Solid", "SOLIDIFY")
    solid.thickness = 0.006
    bpy.ops.object.modifier_apply(modifier=solid.name)
    return finish(obj, slot, smooth=True, bevel=0)


def build_sail():
    half = 1.7
    sail_membrane(half, 5)                           # reflective silver sail
    # quadrant seams
    box((0, 0, 0.13), (2 * half, 0.02, 0.02), 1, 0, bevel=0)
    box((0, 0, 0.13), (0.02, 2 * half, 0.02), 1, 0, bevel=0)
    # four diagonal truss booms to corners
    for sx in (-1, 1):
        for sy in (-1, 1):
            corner = Vector((sx * half, sy * half, 0.0))
            cylinder((0, 0, -0.15), corner, 0.02, 1, vertices=10)
            for t in (0.3, 0.6, 0.9):
                p = Vector((0, 0, -0.15)).lerp(corner, t)
                torus(tuple(p), 0.03, 0.006, 1, major=6, minor=6)
            box(tuple(corner), (0.12, 0.12, 0.04), 3, 0, bevel=0)  # tip control vane
    # central micro-bus
    box((0, 0, -0.28), (0.3, 0.3, 0.3), 2)
    box((0, 0, -0.45), (0.32, 0.32, 0.04), 1)
    prism((0, 0, -0.12), 0.1, 0.06, 6, 0)            # small HGA
    cylinder((0.12, 0.12, -0.4), (0.12, 0.12, -0.55), 0.01, 5, vertices=8)
    sphere((0.12, 0.12, -0.56), 0.02, 4)
    for i in range(4):
        a = i * math.tau / 4 + math.pi / 4
        cone((0.16 * math.cos(a), 0.16 * math.sin(a), -0.45), 0.02, 0.008, 0.03, 5, vertices=10)


FLEET = (
    ("rover", build_rover, 2.6, (0.9, -1.1, 0.6),
     {0: ((0.86, 0.86, 0.88), 0.35, 0.4, (0, 0, 0))}),
    ("colony", build_colony, 4.0, (0.7, -1.0, 0.55), None),
    ("lander", build_lander, 3.0, (0.9, -1.1, 0.5), None),
    ("sail", build_sail, 3.6, (0.5, -1.0, 0.9),
     {5: ((0.90, 0.90, 0.94), 0.10, 0.95, (0, 0, 0))}),
)


def setup_showcase(scene, target, cam_dir):
    scene.render.engine = "CYCLES"
    scene.cycles.device = "CPU"
    scene.cycles.samples = 64
    scene.cycles.use_denoising = True
    scene.render.resolution_x, scene.render.resolution_y = 1600, 1066
    scene.render.resolution_percentage = 100
    scene.render.image_settings.file_format = "PNG"
    scene.world = bpy.data.worlds.new("DeepSpace")
    scene.world.use_nodes = True
    bg = scene.world.node_tree.nodes.get("Background")
    bg.inputs["Color"].default_value = (0.008, 0.012, 0.022, 1)
    bg.inputs["Strength"].default_value = 0.5
    for name, pos, energy, color, size in (
        ("SunKey", (5, -9, 12), 3000, (1.0, 0.90, 0.74), 9),
        ("BlueRim", (-8, 6, 4), 3200, (0.30, 0.62, 1.0), 8),
        ("SoftFill", (2, -6, 4), 1100, (0.72, 0.85, 1.0), 12),
    ):
        data = bpy.data.lights.new(name, "AREA")
        data.energy, data.color, data.size = energy, color, size
        obj = bpy.data.objects.new(name, data)
        obj.location = pos
        obj.rotation_euler = (-obj.location).to_track_quat("-Z", "Y").to_euler()
        scene.collection.objects.link(obj)
    camera = bpy.data.cameras.new("ShowcaseCamera")
    camera.type = "ORTHO"
    camera.ortho_scale = target * 1.7
    scene.camera = bpy.data.objects.new("ShowcaseCamera", camera)
    scene.collection.objects.link(scene.camera)
    d = Vector(cam_dir).normalized()
    scene.camera.location = d * (target * 2.2)
    scene.camera.rotation_euler = (-d).to_track_quat("-Z", "Y").to_euler()


def export_selection(objs, path):
    bpy.ops.object.select_all(action="DESELECT")
    for o in objs:
        o.select_set(True)
    bpy.context.view_layer.objects.active = objs[0]
    bpy.ops.wm.obj_export(filepath=str(path), export_selected_objects=True,
                          export_uv=True, export_normals=True,
                          export_materials=True, export_triangulated_mesh=True,
                          forward_axis="Y", up_axis="Z", path_mode="RELATIVE")


def main():
    MODELS.mkdir(parents=True, exist_ok=True)
    OUT.mkdir(parents=True, exist_ok=True)
    for name, builder, target, cam_dir, overrides in FLEET:
        bpy.ops.wm.read_factory_settings(use_empty=True)
        global MATERIAL, PARTS
        PARTS = []
        MATERIAL = make_atlas(name, overrides=overrides)
        builder()
        bpy.ops.object.select_all(action="DESELECT")
        for obj in PARTS:
            obj.select_set(True)
        bpy.context.view_layer.objects.active = PARTS[0]
        bpy.ops.object.join()
        joined = bpy.context.object
        bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)
        factor = target / max(joined.dimensions)
        joined.scale = (factor, factor, factor)
        bpy.ops.object.transform_apply(scale=True)
        triangles = len(joined.data.polygons)
        export_selection([joined], MODELS / f"{name}.obj")
        scene = bpy.context.scene
        setup_showcase(scene, target, cam_dir)
        scene.render.filepath = str(OUT / f"{name}_showcase.png")
        bpy.ops.render.render(write_still=True)
        print(f"DONE {name}: {triangles} triangles, size {target}", flush=True)
    print("ALL DONE", flush=True)


if __name__ == "__main__":
    main()