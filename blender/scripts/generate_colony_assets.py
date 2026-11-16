"""Procedural 3D planetary infrastructure assets for Astro Voyager:
- colony_dome: Geodesic biosphere habitat dome with pressurized airlocks and glowing window rings.
- colony_factory: Industrial ore refinery and additive fabrication plant with silos and gantry.
- colony_pad: Planetary spaceport landing pad with perimeter strobe towers and control nexus.

Flip-safe PBR pipeline matching src/ObjModel.cpp via asset_atlas.py.
Run: blender --background --python blender/scripts/generate_colony_assets.py
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

MATERIAL = None
PARTS = []


def make_atlas(prefix, overrides=None, size=1024):
    """Shared flip-safe atlas (asset_atlas enforces np.minimum(yy, (t - 1) - yy) and GRID - 1 - gy)."""
    return asset_atlas.build(f"{prefix}_PBR", PALETTE, MODELS, prefix, size=size,
                             overrides=overrides, seed_base=3000)


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
        bpy.ops.object.modifier_apply(modifier="Bevel")
    PARTS.append(obj)
    return obj


# ===========================================================================
# 1. Geodesic Biosphere Habitat Dome
# ===========================================================================
def build_colony_dome():
    # Concrete foundation collar
    bpy.ops.mesh.primitive_cylinder_add(radius=2.2, depth=0.25, vertices=32, location=(0, 0, 0.125))
    finish(bpy.context.object, 1)

    # Primary Geodesic dome
    bpy.ops.mesh.primitive_ico_sphere_add(subdivisions=3, radius=2.0, location=(0, 0, 0.2))
    dome = bpy.context.object
    # Slice off bottom hemisphere below z=0.2
    for v in dome.data.vertices:
        if v.co.z < 0.0:
            v.co.z = 0.0
    finish(dome, 0, smooth=True)

    # Glowing window rings around middle latitudes
    for r, z in ((1.75, 0.8), (1.35, 1.35), (0.85, 1.75)):
        bpy.ops.mesh.primitive_torus_add(major_radius=r, minor_radius=0.06,
                                         major_segments=32, minor_segments=8, location=(0, 0, z))
        finish(bpy.context.object, 4, smooth=True, bevel=0)

    # Apex observatory cupola & satellite relay mast
    bpy.ops.mesh.primitive_uv_sphere_add(radius=0.35, segments=16, ring_count=12, location=(0, 0, 2.15))
    finish(bpy.context.object, 3, smooth=True)

    bpy.ops.mesh.primitive_cylinder_add(radius=0.04, depth=0.8, vertices=8, location=(0, 0, 2.7))
    finish(bpy.context.object, 5)

    bpy.ops.mesh.primitive_cone_add(radius1=0.28, radius2=0.02, depth=0.15, vertices=16, location=(0, 0, 2.9))
    finish(bpy.context.object, 5)

    # 3 Radial pressurized airlock tunnels
    for angle_deg in (0, 120, 240):
        rad = math.radians(angle_deg)
        cx, cy = math.cos(rad) * 2.2, math.sin(rad) * 2.2
        bpy.ops.mesh.primitive_cube_add(size=1.0, location=(cx, cy, 0.4))
        tunnel = bpy.context.object
        tunnel.scale = (0.7, 0.5, 0.5)
        tunnel.rotation_euler = (0, 0, rad)
        bpy.ops.object.transform_apply(scale=True, rotation=True)
        finish(tunnel, 1)

        # Outer airlock hatch collar
        hx, hy = math.cos(rad) * 2.55, math.sin(rad) * 2.55
        bpy.ops.mesh.primitive_cylinder_add(radius=0.22, depth=0.08, vertices=16, location=(hx, hy, 0.4))
        hatch = bpy.context.object
        hatch.rotation_euler = (math.pi / 2, 0, rad + math.pi / 2)
        bpy.ops.object.transform_apply(rotation=True)
        finish(hatch, 7)


# ===========================================================================
# 2. Industrial Ore Refinery & Fabrication Plant
# ===========================================================================
def build_colony_factory():
    # Reinforced industrial slab
    bpy.ops.mesh.primitive_cube_add(size=1.0, location=(0, 0, 0.15))
    slab = bpy.context.object
    slab.scale = (3.4, 2.8, 0.3)
    bpy.ops.object.transform_apply(scale=True)
    finish(slab, 1)

    # Main heavy furnace hall
    bpy.ops.mesh.primitive_cube_add(size=1.0, location=(-0.5, 0.0, 0.9))
    hall = bpy.context.object
    hall.scale = (1.8, 2.2, 1.2)
    bpy.ops.object.transform_apply(scale=True)
    finish(hall, 0)

    # Smelting furnace silos (3 pressurized vertical cylinders)
    for i, sy in enumerate((-0.7, 0.0, 0.7)):
        bpy.ops.mesh.primitive_cylinder_add(radius=0.35, depth=1.8, vertices=24, location=(0.9, sy, 1.1))
        finish(bpy.context.object, 5, smooth=True)

        bpy.ops.mesh.primitive_uv_sphere_add(radius=0.35, segments=16, ring_count=8, location=(0.9, sy, 2.0))
        dome = bpy.context.object
        for v in dome.data.vertices:
            if v.co.z < 2.0:
                v.co.z = 2.0
        finish(dome, 2, smooth=True)

    # Twin high-temperature exhaust stacks
    for sy in (-0.5, 0.5):
        bpy.ops.mesh.primitive_cylinder_add(radius=0.14, depth=2.4, vertices=16, location=(-1.1, sy, 1.5))
        finish(bpy.context.object, 1)
        # Glowing exhaust nozzle cap
        bpy.ops.mesh.primitive_cylinder_add(radius=0.16, depth=0.15, vertices=16, location=(-1.1, sy, 2.7))
        finish(bpy.context.object, 7)

    # Gantry crane & conveyor spine
    bpy.ops.mesh.primitive_cube_add(size=1.0, location=(0.2, 0.0, 1.9))
    gantry = bpy.context.object
    gantry.scale = (2.2, 0.25, 0.25)
    bpy.ops.object.transform_apply(scale=True)
    finish(gantry, 5)

    # Liquid methane / mineral slurry sphere tank
    bpy.ops.mesh.primitive_uv_sphere_add(radius=0.55, segments=24, ring_count=16, location=(-0.8, -1.0, 0.8))
    finish(bpy.context.object, 2, smooth=True)


# ===========================================================================
# 3. Planetary Spaceport Landing Pad
# ===========================================================================
def build_colony_pad():
    # Octagonal primary landing tarmac
    bpy.ops.mesh.primitive_cylinder_add(radius=2.6, depth=0.25, vertices=8, location=(0, 0, 0.125))
    pad = bpy.context.object
    pad.rotation_euler = (0, 0, math.pi / 8)
    bpy.ops.object.transform_apply(rotation=True)
    finish(pad, 1)

    # Inner circular blast deflection ring
    bpy.ops.mesh.primitive_torus_add(major_radius=1.8, minor_radius=0.08,
                                     major_segments=32, minor_segments=8, location=(0, 0, 0.26))
    finish(bpy.context.object, 5, smooth=True, bevel=0)

    # Center target touchdown cross
    for rot in (0.0, math.pi / 2):
        bpy.ops.mesh.primitive_cube_add(size=1.0, location=(0, 0, 0.27))
        bar = bpy.context.object
        bar.scale = (1.4, 0.16, 0.02)
        bar.rotation_euler = (0, 0, rot)
        bpy.ops.object.transform_apply(scale=True, rotation=True)
        finish(bar, 7)

    # 8 Perimeter approach beacon pylons
    for i in range(8):
        angle = i * (math.tau / 8.0)
        px, py = math.cos(angle) * 2.45, math.sin(angle) * 2.45
        bpy.ops.mesh.primitive_cylinder_add(radius=0.08, depth=0.6, vertices=8, location=(px, py, 0.42))
        finish(bpy.context.object, 1)
        # Strobe head
        bpy.ops.mesh.primitive_uv_sphere_add(radius=0.09, segments=8, ring_count=6, location=(px, py, 0.74))
        finish(bpy.context.object, 7)

    # Flight Operations Control Tower
    bpy.ops.mesh.primitive_cube_add(size=1.0, location=(2.2, 1.8, 0.7))
    tower_base = bpy.context.object
    tower_base.scale = (0.7, 0.7, 1.1)
    bpy.ops.object.transform_apply(scale=True)
    finish(tower_base, 0)

    # Panoramic observation glass cab
    bpy.ops.mesh.primitive_cylinder_add(radius=0.45, depth=0.4, vertices=12, location=(2.2, 1.8, 1.45))
    finish(bpy.context.object, 4, smooth=True)

    # Fueling umbilical mast & cryogenic tanks
    bpy.ops.mesh.primitive_cylinder_add(radius=0.35, depth=0.9, vertices=16, location=(-2.1, 1.5, 0.6))
    finish(bpy.context.object, 2, smooth=True)

    bpy.ops.mesh.primitive_cylinder_add(radius=0.05, depth=1.6, vertices=8, location=(-1.9, 1.2, 0.9))
    boom = bpy.context.object
    boom.rotation_euler = (0, 0.5, 0.4)
    bpy.ops.object.transform_apply(rotation=True)
    finish(boom, 5)


# ===========================================================================
# Render Setup & Showcase Exporter
# ===========================================================================
def setup_showcase(scene, target, cam_dir):
    scene.render.engine = "CYCLES"
    scene.cycles.device = "CPU"
    scene.cycles.samples = 48
    scene.render.resolution_x = 1280
    scene.render.resolution_y = 720
    scene.render.resolution_percentage = 100
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
        obj.rotation_euler = (-obj.location).to_track_quat("-Z", "Y").to_euler()
        scene.collection.objects.link(obj)

    camera = bpy.data.cameras.new("ColonyCam")
    camera.type = "ORTHO"
    camera.ortho_scale = target * 1.8
    scene.camera = bpy.data.objects.new("ColonyCam", camera)
    scene.collection.objects.link(scene.camera)
    d = Vector(cam_dir).normalized()
    scene.camera.location = d * (target * 2.4)
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


ASSETS = [
    ("colony_dome", build_colony_dome, 3.0, (1.8, -2.4, 2.0)),
    ("colony_factory", build_colony_factory, 3.2, (2.2, -2.2, 1.9)),
    ("colony_pad", build_colony_pad, 3.2, (2.0, -2.4, 2.2)),
]


def main():
    MODELS.mkdir(parents=True, exist_ok=True)
    OUT.mkdir(parents=True, exist_ok=True)

    for name, builder, target, cam_dir in ASSETS:
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

        factor = target / max(joined.dimensions)
        joined.scale = (factor, factor, factor)
        bpy.ops.object.transform_apply(scale=True)
        triangles = len(joined.data.polygons)

        export_selection([joined], MODELS / f"{name}.obj")

        scene = bpy.context.scene
        setup_showcase(scene, target, cam_dir)
        scene.render.filepath = str(OUT / f"{name}_showcase.png")
        bpy.ops.render.render(write_still=True)
        print(f"DONE {name}: {triangles} triangles, exported to {MODELS / f'{name}.obj'}", flush=True)

    print("ALL COLONY ASSETS DONE", flush=True)


if __name__ == "__main__":
    main()
