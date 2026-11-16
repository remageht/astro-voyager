"""High-detail Voyager-class interstellar probe (own procedural asset).

Signature elements of the real Voyager spacecraft, rebuilt from primitives:
  - 3.7 m parabolic High-Gain Antenna (solid paraboloid shell) with concentric
    panel rings, radial seams, tripod subreflector feed and low-gain mast
  - decagonal electronics bus wrapped in gold MLI with thermal louvers
  - the Golden Record (grooved gold disc + etched cover) on the bus flank
  - RTG boom carrying three finned MHW radioisotope generators
  - science scan platform: wide/narrow-angle cameras + spectrometer barrels
  - long magnetometer truss boom with two fluxgate sensor canisters
  - V-shaped planetary-radio-astronomy dipole whips
  - hydrazine thruster quads, star tracker, sun sensor, cosmic-ray detector

Shares the proven PBR-atlas + OBJ pipeline (albedo/normal/rough/metal/emissive
BMP), normalizes to a 3.0-unit game asset, exports res/models/voyager.obj and
renders a 1600x1066 @64-sample hero plus a Golden Record close-up. No external
assets. Run: blender --background --python blender/scripts/generate_voyager_probe.py
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
OUT = ROOT / "blender/out/voyager"
# slot: (albedo, roughness, metallic, emission)
PALETTE = [
    ((0.90, 0.91, 0.93), 0.30, 0.35, (0.0, 0.0, 0.0)),   # 0 white dish / hull
    ((0.07, 0.08, 0.10), 0.34, 0.82, (0.0, 0.0, 0.0)),   # 1 dark titanium
    ((0.86, 0.63, 0.19), 0.26, 0.80, (0.0, 0.0, 0.0)),   # 2 gold MLI / record
    ((0.03, 0.06, 0.16), 0.20, 0.45, (0.0, 0.0, 0.0)),   # 3 optics / dark blue
    ((0.10, 0.85, 0.80), 0.20, 0.10, (0.10, 0.95, 0.85)),# 4 cyan marker
    ((0.62, 0.64, 0.68), 0.30, 0.85, (0.0, 0.0, 0.0)),   # 5 bright metal / fins
    ((0.02, 0.03, 0.05), 0.40, 0.30, (0.0, 0.0, 0.0)),   # 6 dark recess
    ((0.95, 0.55, 0.12), 0.25, 0.10, (1.0, 0.6, 0.15)),  # 7 amber beacon
]
MATERIAL = None
PARTS = []


def make_atlas(prefix, size=1024):
    """Shared flip-safe atlas (asset_atlas enforces np.minimum(yy, (t - 1) - yy) and GRID - 1 - gy)."""
    return asset_atlas.build(f"{prefix}_PBR", PALETTE, MODELS, prefix, size=size,
                             seed_base=2000)



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


def paraboloid(center, radius, depth, slot=0, rings=28, seg=128, thickness=0.03):
    cx, cy, cz = center
    verts = [(cx, cy, cz)]
    for ri in range(1, rings + 1):
        r = radius * ri / rings
        z = cz + depth * (r / radius) ** 2
        for si in range(seg):
            a = si * math.tau / seg
            verts.append((cx + r * math.cos(a), cy + r * math.sin(a), z))
    faces = []
    for si in range(seg):
        faces.append((0, 1 + si, 1 + (si + 1) % seg))
    for ri in range(rings - 1):
        b0 = 1 + ri * seg
        b1 = 1 + (ri + 1) * seg
        for si in range(seg):
            s2 = (si + 1) % seg
            faces.append((b0 + si, b1 + si, b1 + s2, b0 + s2))
    mesh = bpy.data.meshes.new("HGA")
    mesh.from_pydata(verts, [], faces)
    mesh.update()
    obj = bpy.data.objects.new("HGA", mesh)
    bpy.context.collection.objects.link(obj)
    bpy.context.view_layer.objects.active = obj
    solid = obj.modifiers.new("Solid", "SOLIDIFY")
    solid.thickness = thickness
    solid.offset = -1.0
    bpy.ops.object.modifier_apply(modifier=solid.name)
    return finish(obj, slot, smooth=True, bevel=0)


def radial_seams(center, radius, count, length, slot=1, z=0.0):
    cx, cy, cz = center
    for i in range(count):
        a = i * math.tau / count
        box((cx + radius * math.cos(a), cy + radius * math.sin(a), cz + z),
            (0.01, 0.018, length), slot, a, bevel=0)


def build_voyager():
    # ---- High-Gain Antenna (forward +Z) ----
    dish_z = 0.62
    paraboloid((0, 0, dish_z), 1.40, 0.34, slot=0)
    # concentric panel rings on the reflective face
    for rr in (0.42, 0.74, 1.06, 1.36):
        z = dish_z + 0.34 * (rr / 1.40) ** 2 + 0.005
        torus((0, 0, z), rr, 0.008, 1, major=128, minor=8)
    radial_seams((0, 0, dish_z), 0.9, 24, 0.02, 1, z=0.18)
    # rim structural ring
    torus((0, 0, dish_z + 0.34), 1.40, 0.02, 5, major=128, minor=12)
    # central hub + feed waveguide
    cylinder((0, 0, dish_z - 0.05), (0, 0, dish_z + 0.08), 0.10, 1)
    # tripod subreflector
    feed_z = dish_z + 0.92
    for i in range(3):
        a = i * math.tau / 3 + math.pi / 6
        rim = Vector((1.15 * math.cos(a), 1.15 * math.sin(a), dish_z + 0.22))
        cylinder(rim, (0, 0, feed_z), 0.014, 5, vertices=16)
    cone((0, 0, feed_z), 0.11, 0.04, 0.12, 2)          # subreflector
    cylinder((0, 0, feed_z + 0.06), (0, 0, feed_z + 0.20), 0.02, 1, vertices=16)
    sphere((0, 0, feed_z + 0.22), 0.03, 4)             # low-gain tip
    # dish rear radial backing ribs
    for i in range(12):
        a = i * math.tau / 12
        cylinder((0.12 * math.cos(a), 0.12 * math.sin(a), dish_z - 0.04),
                 (1.30 * math.cos(a), 1.30 * math.sin(a), dish_z + 0.28),
                 0.012, 1, vertices=8)

    # ---- Decagonal electronics bus ----
    bus_z = 0.18
    prism((0, 0, bus_z), 0.50, 0.46, 10, slot=2)
    torus((0, 0, bus_z + 0.24), 0.51, 0.02, 1, major=10, minor=8)
    torus((0, 0, bus_z - 0.24), 0.51, 0.02, 1, major=10, minor=8)
    # thermal louvers on three facets
    for k in range(3):
        a = k * math.tau / 3 + 0.3
        for lz in range(-3, 4):
            box((0.505 * math.cos(a), 0.505 * math.sin(a), bus_z + lz * 0.05),
                (0.02, 0.26, 0.012), 5, a, bevel=0)
    # avionics boxes greeble
    for k in range(5):
        a = k * math.tau / 5 + 0.6
        box((0.5 * math.cos(a), 0.5 * math.sin(a), bus_z - 0.05),
            (0.10, 0.14, 0.16), 1, a)

    # ---- Golden Record on the bus flank (+Y face) ----
    gr_y = 0.53
    gr = prism((0.0, gr_y, bus_z + 0.02), 0.26, 0.02, 48, slot=2, bevel=0.002)
    gr.rotation_euler.x = math.pi / 2
    for rr in (0.10, 0.16, 0.22):
        t = torus((0.0, gr_y + 0.012, bus_z + 0.02), rr, 0.004, 2, major=64, minor=6)
        t.rotation_euler.x = math.pi / 2
    hub = prism((0.0, gr_y + 0.014, bus_z + 0.02), 0.04, 0.024, 24, slot=1, bevel=0)
    hub.rotation_euler.x = math.pi / 2
    # etched cover diagram ticks
    for i in range(12):
        a = i * math.tau / 12
        box((0.19 * math.cos(a), gr_y + 0.016, bus_z + 0.02 + 0.19 * math.sin(a)),
            (0.006, 0.004, 0.03), 1, 0, bevel=0)

    # ---- RTG boom (-X) with three finned generators ----
    rtg_dir = Vector((-1, -0.15, -0.35)).normalized()
    base = Vector((0, 0, bus_z - 0.1))
    for s in range(4):
        p1 = base + rtg_dir * (0.5 + s * 0.28)
        p2 = base + rtg_dir * (0.5 + (s + 1) * 0.28)
        cylinder(p1, p2, 0.03, 1, vertices=12)
    for u in range(3):
        c = base + rtg_dir * (1.75 + u * 0.42)
        axis = rtg_dir
        rtg = cylinder(c - axis * 0.18, c + axis * 0.18, 0.14, 1, vertices=32)
        # radial cooling fins
        for f in range(16):
            a = f * math.tau / 16
            perp = axis.cross(Vector((0, 0, 1))).normalized()
            perp2 = axis.cross(perp).normalized()
            off = (perp * math.cos(a) + perp2 * math.sin(a)) * 0.17
            fin = box(tuple(c + off), (0.05, 0.008, 0.34), 5, 0, bevel=0)
            fin.rotation_euler = axis.to_track_quat("Z", "Y").to_euler()
        sphere(tuple(c + axis * 0.2), 0.05, 7)  # warm end cap glow

    # ---- Science scan platform (+X) with cameras & spectrometers ----
    sci_base = Vector((0.5, 0.0, bus_z - 0.05))
    sci_dir = Vector((1, 0.1, -0.1)).normalized()
    cylinder(sci_base, sci_base + sci_dir * 0.7, 0.035, 1, vertices=12)
    plat = sci_base + sci_dir * 0.8
    box(tuple(plat), (0.28, 0.34, 0.22), 1)
    # narrow + wide angle cameras
    for off, r, ln in (((0, 0.12, 0.16), 0.06, 0.34), ((0, -0.12, 0.12), 0.08, 0.24)):
        cp = plat + Vector(off)
        cylinder(cp, cp + Vector((0.0, 0.0, ln)), r, 1, vertices=32)
        cylinder(cp + Vector((0, 0, ln)), cp + Vector((0, 0, ln + 0.02)), r * 0.9, 3, vertices=32)
    # IRIS + UVS spectrometer barrels
    for ox in (-0.14, 0.14):
        cp = plat + Vector((ox, 0.0, 0.05))
        cylinder(cp, cp + Vector((0.22, 0, 0.0)), 0.045, 6, vertices=24)
    sphere(tuple(plat + Vector((0.0, 0.18, -0.12))), 0.04, 4)

    # ---- Magnetometer truss boom (aft -Z) ----
    mag_start = Vector((0.2, 0.2, bus_z - 0.24))
    mag_dir = Vector((0.25, 0.25, -1)).normalized()
    seg_len = 0.34
    for s in range(8):
        p1 = mag_start + mag_dir * (s * seg_len)
        p2 = mag_start + mag_dir * ((s + 1) * seg_len)
        cylinder(p1, p2, 0.012, 5, vertices=8)
        # truss cross-bracing ring
        torus(tuple(p2), 0.03, 0.006, 1, major=8, minor=6)
    for tip in (5, 8):
        c = mag_start + mag_dir * (tip * seg_len)
        cylinder(c - mag_dir * 0.06, c + mag_dir * 0.06, 0.05, 2, vertices=16)

    # ---- Planetary Radio Astronomy V dipoles (±Y) ----
    for side in (-1, 1):
        d = Vector((0.15 * side, side, 0.1)).normalized()
        p0 = Vector((0.0, 0.3 * side, bus_z))
        cylinder(p0, p0 + d * 1.9, 0.01, 5, vertices=8)
        sphere(tuple(p0 + d * 1.9), 0.025, 4 if side > 0 else 7)

    # ---- Hydrazine thruster quads ----
    for k in range(4):
        a = k * math.tau / 4 + math.pi / 4
        cx, cy = 0.5 * math.cos(a), 0.5 * math.sin(a)
        box((cx, cy, bus_z - 0.2), (0.07, 0.07, 0.07), 1, a)
        for nd in ((1, 0, 0), (0, 1, 0), (0, 0, -1)):
            base = Vector((cx, cy, bus_z - 0.2))
            dirv = Vector(nd).normalized()
            cone(tuple(base + dirv * 0.06), 0.022, 0.008, 0.04, 5, vertices=12)

    # ---- Star tracker + sun sensor + cosmic-ray detector ----
    cylinder((0.35, -0.35, bus_z + 0.1), (0.5, -0.5, bus_z + 0.22), 0.05, 6, vertices=24)
    cylinder((0.5, -0.5, bus_z + 0.22), (0.53, -0.53, bus_z + 0.25), 0.045, 3, vertices=24)
    box((-0.3, 0.3, bus_z + 0.24), (0.12, 0.12, 0.06), 1)
    box((-0.3, 0.3, bus_z + 0.28), (0.08, 0.08, 0.02), 4, bevel=0)


def setup_showcase(scene, target, cam_dir, ortho=None):
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
    camera.ortho_scale = ortho if ortho else target * 1.7
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
    bpy.ops.wm.read_factory_settings(use_empty=True)
    global MATERIAL, PARTS
    PARTS = []
    MATERIAL = make_atlas("voyager")
    build_voyager()
    bpy.ops.object.select_all(action="DESELECT")
    for obj in PARTS:
        obj.select_set(True)
    bpy.context.view_layer.objects.active = PARTS[0]
    bpy.ops.object.join()
    joined = bpy.context.object
    bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)
    target = 3.0
    factor = target / max(joined.dimensions)
    joined.scale = (factor, factor, factor)
    bpy.ops.object.transform_apply(scale=True)
    triangles = len(joined.data.polygons)
    export_selection([joined], MODELS / "voyager.obj")
    scene = bpy.context.scene
    setup_showcase(scene, target, (0.8, -1.1, 0.7))
    scene.render.filepath = str(OUT / "voyager_showcase.png")
    bpy.ops.render.render(write_still=True)
    setup_showcase(scene, target, (0.2, 1.2, 0.35), ortho=target * 0.9)
    scene.render.filepath = str(OUT / "voyager_record.png")
    bpy.ops.render.render(write_still=True)
    print(f"DONE voyager: {triangles} triangles, size {target}", flush=True)


if __name__ == "__main__":
    main()