"""High-detail own procedural replacements for Crew Dragon, Soyuz and Webb.
High-segment primitives + bevel hard-surface edges + greebles (panel bands,
radiator fins, thruster nozzles, window frames, solar cell dividers, mirror
spacers, sunshield booms). Normalizes to in-game sizes (2.6 / 2.8 / 2.2),
exports OBJ + PBR BMP into res/models/ under the names the game loads, and
renders a 1600x1066 @64-sample showcase per craft. No external assets.
Run: blender --background --python blender/scripts/generate_own_craft.py
"""
import math
import struct
from pathlib import Path

import bpy
import numpy as np
from mathutils import Vector

ROOT = Path(__file__).resolve().parents[2]
MODELS = ROOT / "res/models"
OUT = ROOT / "blender/out/own_craft"
PALETTE = [
    ((0.76, 0.80, 0.85), 0.38, 0.45, (0.0, 0.0, 0.0)),
    ((0.055, 0.075, 0.105), 0.30, 0.80, (0.0, 0.0, 0.0)),
    ((0.64, 0.38, 0.09), 0.28, 0.75, (0.0, 0.0, 0.0)),
    ((0.025, 0.065, 0.19), 0.24, 0.40, (0.0, 0.0, 0.0)),
    ((0.03, 0.55, 0.72), 0.22, 0.15, (0.03, 0.65, 0.90)),
    ((0.95, 0.24, 0.055), 0.36, 0.30, (0.0, 0.0, 0.0)),
    ((0.018, 0.045, 0.08), 0.12, 0.30, (0.0, 0.0, 0.0)),
    ((0.65, 0.035, 0.018), 0.28, 0.10, (1.0, 0.055, 0.01)),
]
MATERIAL = None
PARTS = []
MIRROR_PARTS = []
TAG = ""


def write_bmp(path, rgb):
    height, width, _ = rgb.shape
    rows = np.clip(rgb * 255.0, 0, 255).astype(np.uint8)[:, :, ::-1]
    padding = (-width * 3) % 4
    payload = b"".join(row.tobytes() + b"\0" * padding for row in rows)
    header = struct.pack("<2sIHHI", b"BM", 54 + len(payload), 0, 0, 54)
    dib = struct.pack("<IiiHHIIiiII", 40, width, height, 1, 24, 0,
                      len(payload), 2835, 2835, 0, 0)
    path.write_bytes(header + dib + payload)


def make_atlas(prefix, size=1024, overrides=None):
    palette = list(PALETTE)
    if overrides:
        for slot, val in overrides.items():
            palette[slot] = val
    maps = {k: np.zeros((size, size, 3), dtype=np.float32)
            for k in ("albedo", "rough", "metal", "normal", "emissive")}
    tile = size // 8
    yy, xx = np.mgrid[:size, :tile]
    yy = np.minimum(yy, size - 1 - yy)
    for slot, (color, rough, metal, emission) in enumerate(palette):
        x0 = slot * tile
        region = (slice(None), slice(x0, x0 + tile))
        for key, value in (("albedo", color), ("rough", rough),
                           ("metal", metal), ("emissive", emission)):
            maps[key][region] = value
        maps["normal"][region] = (0.5, 0.5, 1.0)
        if slot == 3:
            grid = (xx % 16 < 1) | (yy % 64 < 2)
            cells = np.broadcast_to(np.array(color), (size, tile, 3)).copy()
            cells[grid] = (0.26, 0.36, 0.47)
            maps["albedo"][region] = cells
        elif slot == 0:
            seams = (xx % 64 < 1) | (yy % 128 < 2)
            panels = np.broadcast_to(np.array(color), (size, tile, 3)).copy()
            panels[seams] *= 0.62
            maps["albedo"][region] = panels
    for key, pixels in maps.items():
        write_bmp(MODELS / f"{prefix}_{key}.bmp", pixels)
    material = bpy.data.materials.new(f"{prefix}_PBR")
    material.use_nodes = True
    tree = material.node_tree
    bsdf = tree.nodes.get("Principled BSDF")
    for key, socket in (("albedo", "Base Color"), ("rough", "Roughness"),
                        ("metal", "Metallic"), ("emissive", "Emission Color")):
        image = bpy.data.images.load(str(MODELS / f"{prefix}_{key}.bmp"))
        if key in ("rough", "metal"):
            image.colorspace_settings.name = "Non-Color"
        node = tree.nodes.new("ShaderNodeTexImage")
        node.image = image
        tree.links.new(node.outputs["Color"], bsdf.inputs[socket])
    bsdf.inputs["Emission Strength"].default_value = 3.0
    return material


def finish(obj, slot, smooth=False, bevel=0.008):
    obj.data.materials.append(MATERIAL)
    uv = obj.data.uv_layers.active or obj.data.uv_layers.new(name="UVMap")
    for polygon in obj.data.polygons:
        polygon.use_smooth = smooth
        for index in polygon.loop_indices:
            coord = uv.data[index].uv
            coord.x = slot * 0.125 + 0.006 + coord.x * 0.113
            coord.y = 0.012 + coord.y * 0.976
    if bevel > 0:
        bpy.context.view_layer.objects.active = obj
        mod = obj.modifiers.new("Bevel", "BEVEL")
        mod.width = bevel
        mod.segments = 2
        mod.limit_method = "ANGLE"
        mod.angle_limit = math.radians(40)
        bpy.ops.object.modifier_apply(modifier=mod.name)
    PARTS.append(obj)
    if TAG == "mirror":
        MIRROR_PARTS.append(obj)
    return obj


def box(center, dims, slot=0, rotation=0.0, bevel=0.008):
    bpy.ops.mesh.primitive_cube_add(size=1.0, location=center)
    obj = bpy.context.object
    obj.scale = dims
    obj.rotation_euler.z = rotation
    uv = obj.data.uv_layers.active
    for polygon in obj.data.polygons:
        for corner, index in enumerate(polygon.loop_indices):
            uv.data[index].uv = ((0, 0), (1, 0), (1, 1), (0, 1))[corner]
    return finish(obj, slot, bevel=bevel)


def cylinder(start, end, radius, slot=0, vertices=64):
    a, b = Vector(start), Vector(end)
    bpy.ops.mesh.primitive_cylinder_add(vertices=vertices, radius=radius,
                                        depth=(b - a).length, location=(a + b) / 2)
    obj = bpy.context.object
    obj.rotation_euler = (b - a).to_track_quat("Z", "Y").to_euler()
    return finish(obj, slot, smooth=True)


def cone(center, r1, r2, depth, slot=0, vertices=96):
    bpy.ops.mesh.primitive_cone_add(vertices=vertices, radius1=r1, radius2=r2,
                                    depth=depth, location=center)
    return finish(bpy.context.object, slot, smooth=True)


def sphere(center, radius, slot=0, zscale=1.0):
    bpy.ops.mesh.primitive_uv_sphere_add(segments=64, ring_count=32,
                                         radius=radius, location=center)
    obj = bpy.context.object
    obj.scale.z = zscale
    return finish(obj, slot, smooth=True)


def torus(center, radius, tube, slot=1):
    bpy.ops.mesh.primitive_torus_add(major_segments=128, minor_segments=24,
                                     major_radius=radius, minor_radius=tube,
                                     location=center)
    obj = bpy.context.object
    for polygon in obj.data.polygons:
        polygon.use_smooth = True
    return finish(obj, slot, bevel=0)


def hex(center, radius, depth, slot=2):
    bpy.ops.mesh.primitive_cylinder_add(vertices=6, radius=radius, depth=depth,
                                        location=center)
    return finish(bpy.context.object, slot, bevel=0.004)


def radial_seams(center_z, radius, count, length, slot=1):
    for i in range(count):
        a = i * math.tau / count
        box((radius * math.cos(a), radius * math.sin(a), center_z),
            (0.012, 0.02, length), slot, a, bevel=0)


def build_dragon():
    cone((0, 0, -1.25), 0.62, 0.55, 0.12, 6)
    radial_seams(-1.25, 0.60, 16, 0.10)
    cone((0, 0, -0.75), 0.62, 0.30, 0.85, 0)
    for z, r in ((-0.95, 0.55), (-0.70, 0.46), (-0.45, 0.37)):
        torus((0, 0, z), r, 0.012, 1)
    sphere((0, 0, -0.30), 0.30, 0, zscale=0.5)
    torus((0, 0, -0.16), 0.16, 0.03, 1)
    cylinder((0, 0, -0.16), (0, 0, -0.10), 0.14, 6)
    bpy.ops.mesh.primitive_cylinder_add(vertices=64, radius=0.17, depth=0.03,
                                        location=(0.24, 0, -0.02))
    hatch = bpy.context.object
    hatch.rotation_euler.y = math.radians(70)
    finish(hatch, 0, smooth=True)
    torus((0, 0, -0.12), 0.22, 0.05, 1)
    for i in range(8):
        a = i * math.tau / 8
        box((0.24 * math.cos(a), 0.24 * math.sin(a), -0.10),
            (0.05, 0.03, 0.06), 5, a, bevel=0)
    for i in range(4):
        a = i * math.tau / 4
        box((0.42 * math.cos(a), 0.42 * math.sin(a), -0.55),
            (0.10, 0.16, 0.30), 1, a)
        cone((0.50 * math.cos(a), 0.50 * math.sin(a), -0.72), 0.05, 0.07, 0.08, 6)
        box((0.50 * math.cos(a + 0.4), 0.50 * math.sin(a + 0.4), -0.60),
            (0.09, 0.13, 0.13), 1, a)
        box((0.52 * math.cos(a + 0.4), 0.52 * math.sin(a + 0.4), -0.60),
            (0.05, 0.09, 0.09), 4, a, bevel=0)
        for q in range(4):
            qa = a + 0.9 + q * 0.12
            cone((0.58 * math.cos(qa), 0.58 * math.sin(qa), -0.40),
                 0.02, 0.03, 0.04, 6, vertices=32)
    cylinder((0, 0, 0.0), (0, 0, 1.10), 0.60, 0)
    for i in range(8):
        a = i * math.tau / 8
        box((0.60 * math.cos(a), 0.60 * math.sin(a), 0.55),
            (0.02, 0.04, 1.00), 1, a, bevel=0)
    for i in range(4):
        a = i * math.tau / 4 + 0.4
        box((0.60 * math.cos(a), 0.60 * math.sin(a), 0.55),
            (0.03, 0.30, 0.90), 1, a)
        for f in range(12):
            z = 0.15 + f * 0.07
            box((0.63 * math.cos(a), 0.63 * math.sin(a), z),
                (0.02, 0.28, 0.012), 0, a, bevel=0)
    box((0.61, 0, 0.55), (0.02, 0.50, 0.90), 3)
    box((0.62, 0, 0.55), (0.01, 0.52, 0.92), 2, bevel=0)
    for d in range(10):
        box((0.625, -0.22 + d * 0.05, 0.55), (0.005, 0.008, 0.88), 1, bevel=0)
    cylinder((0, 0, 1.10), (0, 0, 1.20), 0.55, 6)
    torus((0, 0, 1.10), 0.56, 0.02, 1)


def build_soyuz():
    sphere((0, 0, 1.00), 0.42, 0)
    torus((0, 0, 1.00), 0.42, 0.012, 1)
    torus((0, 0, 1.15), 0.36, 0.010, 1)
    box((0, 0.40, 1.00), (0.16, 0.03, 0.16), 1)
    for side in (-1, 1):
        cone((side * 0.30, 0.30, 1.20), 0.10, 0.03, 0.06, 0)
        cylinder((side * 0.30, 0.30, 1.20), (side * 0.30, 0.30, 1.30), 0.015, 1, 32)
    cylinder((0, 0, 1.40), (0, 0, 1.57), 0.10, 1)
    cone((0, 0, 1.60), 0.10, 0.03, 0.08, 1)
    torus((0, 0, 1.42), 0.11, 0.02, 1)
    cone((0, 0, 0.35), 0.45, 0.30, 0.75, 0)
    cone((0, 0, -0.05), 0.45, 0.45, 0.08, 6)
    radial_seams(-0.05, 0.44, 16, 0.06)
    box((0.30, 0.30, 0.55), (0.06, 0.06, 0.10), 1)
    cylinder((0, 0, -0.10), (0, 0, -1.00), 0.42, 0)
    cone((0, 0, -1.15), 0.42, 0.30, 0.30, 1)
    for side in (-1, 1):
        sphere((side * 0.30, 0, -0.55), 0.16, 1)
    for side in (-1, 1):
        box((side * 0.95, 0, -0.55), (1.30, 0.50, 0.03), 3)
        box((side * 0.95, 0, -0.55), (1.34, 0.06, 0.05), 2)
        for d in range(20):
            x = side * (0.35 + d * 0.062)
            box((x, 0, -0.55), (0.008, 0.48, 0.035), 1, bevel=0)
        cylinder((side * 0.42, 0, -0.55), (side * 0.60, 0, -0.55), 0.05, 1)
    for i in range(4):
        a = i * math.tau / 4 + 0.4
        cone((0.40 * math.cos(a), 0.40 * math.sin(a), -1.28), 0.03, 0.05, 0.06, 6, 32)
    cylinder((0, 0.42, -0.40), (0, 0.75, -0.40), 0.03, 1, 32)
    cone((0, 0.80, -0.40), 0.08, 0.02, 0.05, 0)
    box((0, -0.44, -0.55), (0.30, 0.02, 0.60), 1)
    for f in range(10):
        box((0, -0.46, -0.80 + f * 0.055), (0.28, 0.012, 0.012), 0, bevel=0)


def membrane(z, sx, sy, slot, curve):
    nx, ny = 28, 14
    verts, faces = [], []
    for i in range(nx + 1):
        u = i / nx * 2 - 1
        for j in range(ny + 1):
            v = j / ny * 2 - 1
            taper = 1.0 - 0.35 * abs(u)
            verts.append((u * sx / 2, v * sy / 2 * taper,
                          z + curve * (0.3 * u * u - 0.2 * v * v)))
    for i in range(nx):
        for j in range(ny):
            a = i * (ny + 1) + j
            faces.append((a, a + 1, a + ny + 2, a + ny + 1))
    mesh = bpy.data.meshes.new("Membrane")
    mesh.from_pydata(verts, [], faces)
    mesh.uv_layers.new(name="UVMap")
    obj = bpy.data.objects.new("Membrane", mesh)
    bpy.context.collection.objects.link(obj)
    for polygon in mesh.polygons:
        polygon.use_smooth = True
    return finish(obj, slot, bevel=0)


def build_webb():
    global TAG
    TAG = "mirror"
    for ring, count, dist in ((0, 6, 0.30), (1, 12, 0.60)):
        for i in range(count):
            a = i * math.tau / count + (0.0 if ring == 0 else math.tau / 24)
            x, y = dist * math.cos(a), dist * math.sin(a)
            hex((x, y, 0.49), 0.165, 0.02, 1)
            hex((x, y, 0.51), 0.155, 0.03, 2)
    box((0, 0, 0.45), (1.30, 1.10, 0.04), 1)
    box((0, 0, 0.55), (0.16, 0.16, 0.12), 6)
    for i in range(3):
        a = i * math.tau / 3 + math.tau / 12
        cylinder((0.62 * math.cos(a), 0.62 * math.sin(a), 0.50),
                 (0, 0, 1.15), 0.022, 1, 32)
    hex((0, 0, 1.15), 0.09, 0.04, 2)
    cylinder((0, 0, 1.15), (0, 0, 1.22), 0.03, 1, 32)
    TAG = "body"
    for layer in range(5):
        s = 1.0 - layer * 0.05
        z = -0.05 - layer * 0.07
        membrane(z, 2.2 * s, 1.1 * s, 0 if layer % 2 == 0 else 5,
                 0.10 + 0.02 * layer)
        for ex in (-1, 1):
            cylinder((ex * 1.1 * s, -0.45 * s, z), (ex * 1.1 * s, 0.45 * s, z),
                     0.015, 1, 32)
        for ey in (-1, 1):
            cylinder((-1.1 * s, ey * 0.45 * s, z), (1.1 * s, ey * 0.45 * s, z),
                     0.012, 1, 32)
    box((0, 0, -0.55), (0.50, 0.50, 0.30), 5)
    for i in range(4):
        box((0, 0, -0.42 - i * 0.08), (0.52, 0.52, 0.012), 0, bevel=0)
    box((-0.70, 0, -0.55), (0.70, 0.35, 0.02), 3)
    for d in range(14):
        box((-1.00 + d * 0.045, 0, -0.55), (0.006, 0.33, 0.025), 1, bevel=0)
    cylinder((0.25, 0.25, -0.60), (0.45, 0.45, -0.80), 0.02, 1, 32)
    cone((0.45, 0.45, -0.85), 0.16, 0.04, 0.10, 0)
    cylinder((0.45, 0.45, -0.85), (0.45, 0.45, -0.95), 0.015, 1, 32)
    for side in (-1, 1):
        box((side * 0.20, 0.20, -0.55), (0.08, 0.08, 0.08), 0)
        cone((side * 0.20, -0.20, -0.72), 0.02, 0.04, 0.05, 6, 32)


CRAFT = (("dragon", build_dragon, 2.6, None),
         ("soyuz", build_soyuz, 2.8,
          {0: ((0.58, 0.62, 0.56), 0.42, 0.35, (0.0, 0.0, 0.0))}),
         ("telescope", build_webb, 2.2,
          {0: ((0.85, 0.85, 0.88), 0.30, 0.60, (0.0, 0.0, 0.0)),
           5: ((0.90, 0.72, 0.76), 0.35, 0.40, (0.0, 0.0, 0.0))}))


def setup_showcase(scene, target):
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
    bg.inputs["Color"].default_value = (0.009, 0.015, 0.027, 1)
    bg.inputs["Strength"].default_value = 0.5
    for name, pos, energy, color, size in (
        ("SunKey", (4, -10, 14), 2800, (1.0, 0.88, 0.72), 9),
        ("BlueRim", (-8, 5, 5), 3500, (0.28, 0.62, 1.0), 8),
        ("SoftFill", (1, -5, 5), 1200, (0.70, 0.85, 1.0), 12),
    ):
        data = bpy.data.lights.new(name, "AREA")
        data.energy, data.color, data.size = energy, color, size
        obj = bpy.data.objects.new(name, data)
        obj.location = pos
        obj.rotation_euler = (-obj.location).to_track_quat("-Z", "Y").to_euler()
        scene.collection.objects.link(obj)
    camera = bpy.data.cameras.new("ShowcaseCamera")
    camera.type = "ORTHO"
    camera.ortho_scale = target * 1.6
    scene.camera = bpy.data.objects.new("ShowcaseCamera", camera)
    scene.collection.objects.link(scene.camera)
    scene.camera.location = (target * 0.7, -target * 1.1, target * 1.1)
    scene.camera.rotation_euler = (Vector((0, 0, 0)) - scene.camera.location).to_track_quat("-Z", "Y").to_euler()


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
    for name, builder, target, overrides in CRAFT:
        bpy.ops.wm.read_factory_settings(use_empty=True)
        global MATERIAL, PARTS, MIRROR_PARTS, TAG
        PARTS = []
        MIRROR_PARTS = []
        TAG = ""
        MATERIAL = make_atlas(name, overrides=overrides)
        builder()
        if name == "telescope":
            bpy.context.view_layer.update()
            lo = Vector((1e9, 1e9, 1e9))
            hi = Vector((-1e9, -1e9, -1e9))
            for obj in PARTS:
                for c in obj.bound_box:
                    w = obj.matrix_world @ Vector(c)
                    lo = Vector((min(lo[i], w[i]) for i in range(3)))
                    hi = Vector((max(hi[i], w[i]) for i in range(3)))
            factor = target / max(hi - lo)
            for obj in PARTS:
                obj.location = obj.location * factor
                obj.scale = obj.scale * factor
            bpy.context.view_layer.update()
            body = [o for o in PARTS if o not in MIRROR_PARTS]
            export_selection(MIRROR_PARTS, MODELS / "telescope_mirror.obj")
            export_selection(body, MODELS / "telescope_body.obj")
            export_selection(PARTS, MODELS / f"{name}.obj")
            triangles = sum(len(o.data.polygons) for o in PARTS)
        else:
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
        setup_showcase(scene, target)
        scene.render.filepath = str(OUT / f"{name}_showcase.png")
        bpy.ops.render.render(write_still=True)
        print(f"DONE {name}: {triangles} triangles, size {target}", flush=True)
    print("ALL DONE", flush=True)


if __name__ == "__main__":
    main()