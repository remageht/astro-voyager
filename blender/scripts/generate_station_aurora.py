import argparse
import json
import math
import struct
import sys
from pathlib import Path

import bmesh
import bpy
import numpy as np
from mathutils import Vector


ROOT = Path(__file__).resolve().parents[2]
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
GROUPS = {}
MATERIAL = None


def write_bmp(path, rgb):
    height, width, _ = rgb.shape
    rows = np.clip(rgb * 255.0, 0, 255).astype(np.uint8)[:, :, ::-1]
    padding = (-width * 3) % 4
    payload = b"".join(row.tobytes() + b"\0" * padding for row in rows)
    header = struct.pack("<2sIHHI", b"BM", 54 + len(payload), 0, 0, 54)
    dib = struct.pack("<IiiHHIIiiII", 40, width, height, 1, 24, 0,
                      len(payload), 2835, 2835, 0, 0)
    path.write_bytes(header + dib + payload)


def make_atlas(out, size=1024):
    maps = {key: np.zeros((size, size, 3), dtype=np.float32)
            for key in ("albedo", "rough", "metal", "normal", "emissive")}
    tile = size // 8
    yy, xx = np.mgrid[:size, :tile]
    yy = np.minimum(yy, size - 1 - yy)
    for slot, (color, rough, metal, emission) in enumerate(PALETTE):
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
            variation = 0.96 + 0.08 * ((xx // 16 + yy // 64) % 2)
            cells *= variation[:, :, None]
            maps["albedo"][region] = cells
        elif slot == 0:
            seams = (xx % 64 < 1) | (yy % 128 < 2)
            panels = np.broadcast_to(np.array(color), (size, tile, 3)).copy()
            panels[seams] *= 0.62
            maps["albedo"][region] = panels
    for key, pixels in maps.items():
        # Vertical symmetry keeps Blender and the game's top-down BMP loader consistent.
        write_bmp(out / f"station_aurora_{key}.bmp", pixels)
    material = bpy.data.materials.new("Aurora_PBR")
    material.use_nodes = True
    tree = material.node_tree
    bsdf = tree.nodes.get("Principled BSDF")
    for key, socket in (("albedo", "Base Color"), ("rough", "Roughness"),
                        ("metal", "Metallic"), ("emissive", "Emission Color")):
        image = bpy.data.images.load(str(out / f"station_aurora_{key}.bmp"))
        if key in ("rough", "metal"):
            image.colorspace_settings.name = "Non-Color"
        node = tree.nodes.new("ShaderNodeTexImage")
        node.image = image
        tree.links.new(node.outputs["Color"], bsdf.inputs[socket])
    bsdf.inputs["Emission Strength"].default_value = 3.0
    return material


def finish(obj, name, slot, group, smooth=False):
    obj.name = name
    obj.data.materials.append(MATERIAL)
    uv = obj.data.uv_layers.active or obj.data.uv_layers.new(name="UVMap")
    for polygon in obj.data.polygons:
        polygon.use_smooth = smooth and len(polygon.vertices) == 4
        for index in polygon.loop_indices:
            coord = uv.data[index].uv
            coord.x = slot * 0.125 + 0.006 + coord.x * 0.113
            coord.y = 0.012 + coord.y * 0.976
    GROUPS.setdefault(group, []).append(obj)
    return obj


def box(name, center, dimensions, slot=0, group="core", rotation=0.0):
    bpy.ops.mesh.primitive_cube_add(size=1.0, location=center)
    obj = bpy.context.object
    obj.scale = dimensions
    obj.rotation_euler.z = rotation
    uv = obj.data.uv_layers.active
    for polygon in obj.data.polygons:
        for corner, index in enumerate(polygon.loop_indices):
            uv.data[index].uv = ((0, 0), (1, 0), (1, 1), (0, 1))[corner]
    return finish(obj, name, slot, group)


def cylinder(name, start, end, radius, slot=0, group="core", vertices=32):
    a, b = Vector(start), Vector(end)
    bpy.ops.mesh.primitive_cylinder_add(vertices=vertices, radius=radius,
                                       depth=(b - a).length, location=(a + b) / 2)
    obj = bpy.context.object
    obj.rotation_euler = (b - a).to_track_quat("Z", "Y").to_euler()
    return finish(obj, name, slot, group, smooth=True)


def torus(name, radius, tube, z, slot=1, group="core", center=(0, 0)):
    bpy.ops.mesh.primitive_torus_add(major_segments=64, minor_segments=8,
                                    major_radius=radius, minor_radius=tube,
                                    location=(center[0], center[1], z))
    obj = finish(bpy.context.object, name, slot, group)
    for polygon in obj.data.polygons:
        polygon.use_smooth = True
    return obj


def ring_segment(name, angle, span, radius, section, slot, z=0.0):
    steps = max(2, math.ceil(span / (math.pi / 48)))
    vertices, faces = [], []
    count = len(section)
    for i in range(steps + 1):
        theta = angle - span / 2 + span * i / steps
        for radial, height in section:
            vertices.append(((radius + radial) * math.cos(theta),
                             (radius + radial) * math.sin(theta), z + height))
    for i in range(steps):
        for j in range(count):
            k = (j + 1) % count
            faces.append((i * count + j, i * count + k,
                          (i + 1) * count + k, (i + 1) * count + j))
    faces.extend((tuple(reversed(range(count))),
                  tuple(steps * count + j for j in range(count))))
    mesh = bpy.data.meshes.new(name)
    mesh.from_pydata(vertices, [], faces)
    bm = bmesh.new()
    bm.from_mesh(mesh)
    bmesh.ops.recalc_face_normals(bm, faces=list(bm.faces))
    bm.to_mesh(mesh)
    bm.free()
    uv = mesh.uv_layers.new(name="UVMap")
    for polygon in mesh.polygons:
        for corner, index in enumerate(polygon.loop_indices):
            uv.data[index].uv = ((0, 0), (1, 0), (1, 1), (0, 1))[corner % 4]
    obj = bpy.data.objects.new(name, mesh)
    bpy.context.collection.objects.link(obj)
    return finish(obj, name, slot, "ring")


def radial(angle, distance, z=0.0):
    return (math.cos(angle) * distance, math.sin(angle) * distance, z)


def build_station():
    cross_section = [(-0.52, -0.35), (-0.64, -0.20), (-0.64, 0.20),
                     (-0.52, 0.35), (0.52, 0.35), (0.64, 0.20),
                     (0.64, -0.20), (0.52, -0.35)]
    window_section = [(-0.018, -0.035), (-0.018, 0.035),
                      (0.018, 0.035), (0.018, -0.035)]
    for i in range(24):
        angle = i * math.tau / 24
        ring_segment(f"Habitat_{i:02d}", angle, math.radians(14.5), 4.8,
                     cross_section, 0)
        ring_segment(f"Bulkhead_{i:02d}", angle + math.radians(7.5),
                     math.radians(0.5), 4.8, cross_section, 1)
        for offset in (-4.6, 0, 4.6):
            ring_segment(f"Window_{i:02d}_{offset}", angle + math.radians(offset),
                         math.radians(3.3), 5.445, window_section, 4)
        for z in (-0.38, 0.38):
            ring_segment(f"ThermalStrip_{i:02d}_{z}", angle,
                         math.radians(12), 4.85, window_section, 1, z=z)
        if i % 3 == 0:
            box(f"HabitatEquipment_{i}", radial(angle, 4.8, 0.46),
                (0.40, 0.56, 0.20), 2, "ring", angle)
    for i in range(4):
        angle = i * math.tau / 4
        cylinder(f"TransferTunnel_{i}", radial(angle, 0.72),
                 radial(angle, 4.25), 0.23, group="ring")
        for step in range(1, 6):
            distance = 0.8 + step * 0.54
            cylinder(f"TunnelCollar_{i}_{step}", radial(angle, distance - 0.03),
                     radial(angle, distance + 0.03), 0.27, 1, "ring")
        for side in (-1, 1):
            a = Vector(radial(angle, 1.0, -0.25))
            b = Vector(radial(angle, 4.2, -0.25))
            lateral = Vector((-math.sin(angle), math.cos(angle), 0)) * side * 0.28
            cylinder(f"TunnelRail_{i}_{side}", a + lateral, b + lateral,
                     0.045, 2, "ring", vertices=12)
            cylinder(f"TunnelLight_{i}_{side}", radial(angle, 1.15, 0.225),
                     radial(angle, 4.0, 0.225), 0.017, 4, "ring", vertices=8)
    cylinder("CentralHub", (0, 0, -2.15), (0, 0, 2.15), 0.80)
    for z in (-1.95, -1.25, -0.5, 0.5, 1.25, 1.95):
        torus(f"HubStructuralBand_{z}", 0.81, 0.065, z)
    for z in (-1.48, 1.48):
        torus(f"HubNavigationBand_{z}", 0.82, 0.023, z, 4)
    for i in range(8):
        angle = i * math.tau / 8
        box(f"HubArmor_{i}", radial(angle, 0.81),
            (0.06, 0.32, 1.85), 1, rotation=angle)
    cylinder("DockNeck", (0, 0, 2.15), (0, 0, 2.70), 0.59, 1)
    torus("DockOuterCollar", 0.68, 0.12, 2.62, 0)
    torus("DockGuidanceLight", 0.60, 0.025, 2.72, 4)
    cylinder("DockRecess", (0, 0, 2.29), (0, 0, 2.31), 0.59, 6)
    for i in range(8):
        angle = i * math.tau / 8
        box(f"DockLatch_{i}", radial(angle, 0.69, 2.77),
            (0.16, 0.08, 0.20), 5, rotation=angle)
    cylinder("RearServiceBus", (0, 0, -2.15), (0, 0, -2.75), 0.62, 1)
    torus("RearServiceCollar", 0.62, 0.075, -2.7, 2)
    for i in range(4):
        angle = (i + 0.5) * math.tau / 4
        cylinder(f"LabConnector_{i}", radial(angle, 0.7, -0.85),
                 radial(angle, 1.45, -0.85), 0.20)
        center = radial(angle, 1.8)
        cylinder(f"Laboratory_{i}", (center[0], center[1], -1.65),
                 (center[0], center[1], 0.6), 0.38)
        for z in (-1.45, -0.95, -0.3, 0.4):
            torus(f"LaboratoryBand_{i}_{z}", 0.39, 0.035, z, 1,
                  center=(center[0], center[1]))
        torus(f"LaboratoryWindow_{i}", 0.39, 0.022, 0.22, 4,
              center=(center[0], center[1]))
        box(f"LabRadiator_{i}", radial(angle, 2.22, -0.6),
            (0.045, 0.40, 1.20), 1, rotation=angle)
    for side in (-1, 1):
        cylinder(f"SolarBoom_{side}", (side * 0.6, 0, -1.85),
                 (side * 8.1, 0, -1.85), 0.095, 1, "panels", vertices=16)
        for z in (-1.65, -2.05):
            cylinder(f"BoomRail_{side}_{z}", (side * 1.0, 0, z),
                     (side * 8.1, 0, z), 0.035, 2, "panels", vertices=8)
        for n in range(12):
            x0 = side * (0.9 + n * 0.6)
            x1 = side * (1.5 + n * 0.6)
            cylinder(f"BoomBrace_{side}_{n}", (x0, 0, -1.65),
                     (x1, 0, -2.05), 0.025, 0, "panels", vertices=8)
        for wing in (-1, 1):
            x, y, z = side * 8.1, wing * 2.15, -1.85
            box(f"SolarBlanket_{side}_{wing}", (x, y, z),
                (2.45, 3.90, 0.055), 3, "panels")
            for edge in (-1, 1):
                box(f"SolarFrameX_{side}_{wing}_{edge}", (x + edge * 1.25, y, z),
                    (0.075, 4.05, 0.11), 2, "panels")
                box(f"SolarFrameY_{side}_{wing}_{edge}", (x, y + edge * 1.98, z),
                    (2.55, 0.075, 0.11), 2, "panels")
            for divider in (-0.98, 0, 0.98):
                box(f"SolarDivider_{side}_{wing}_{divider}", (x, y + divider, z + 0.035),
                    (2.45, 0.025, 0.025), 1, "panels")
            cylinder(f"PanelHinge_{side}_{wing}", (x, wing * 0.1, z),
                     (x, wing * 0.32, z), 0.17, 0, "panels")
        cylinder(f"BoomBeacon_{side}", (side * 9.4, 0, -1.9),
                 (side * 9.4, 0, -1.8), 0.07, 7 if side < 0 else 4,
                 "panels", vertices=12)
    cylinder("AntennaMast", (-1.28, 1.28, 0.6), (-1.28, 1.28, 1.75),
             0.045, 1, vertices=12)
    dish_vertices = [(0.0, 0.0, 0.0)]
    dish_faces = []
    for i in range(32):
        angle = i * math.tau / 32
        dish_vertices.append((0.52 * math.cos(angle), 0.52 * math.sin(angle), 0.15))
        dish_faces.append((0, i + 1, (i + 1) % 32 + 1))
    mesh = bpy.data.meshes.new("AntennaDish")
    mesh.from_pydata(dish_vertices, [], dish_faces)
    mesh.uv_layers.new(name="UVMap")
    dish = bpy.data.objects.new("AntennaDish", mesh)
    dish.location = (-1.28, 1.28, 1.75)
    bpy.context.collection.objects.link(dish)
    finish(dish, "AntennaDish", 0, "core")
    torus("DishRim", 0.52, 0.022, 1.90, 1, center=(-1.28, 1.28))
    cylinder("AntennaFeed", (-1.28, 1.28, 1.75), (-1.28, 1.28, 2.03),
             0.025, 2, vertices=12)
    for text, location, size in (("AURORA", (-0.90, -4.9, 0.39), 0.35),
                                  ("A-01", (-0.45, 4.62, 0.39), 0.30)):
        bpy.ops.object.text_add(location=location)
        obj = bpy.context.object
        obj.data.body = text
        obj.data.size = size
        obj.data.extrude = 0.006
        bpy.ops.object.convert(target="MESH")
        finish(bpy.context.object, f"Marking_{text}", 1, "ring")


def prepare_meshes():
    for objects in GROUPS.values():
        for obj in objects:
            bpy.ops.object.select_all(action="DESELECT")
            obj.select_set(True)
            bpy.context.view_layer.objects.active = obj
            bpy.ops.object.transform_apply(location=False, rotation=True, scale=True)
            modifier = obj.modifiers.new("Triangulated", "TRIANGULATE")
            bpy.ops.object.modifier_apply(modifier=modifier.name)
    bpy.ops.object.select_all(action="DESELECT")
    meshes = [obj for objects in GROUPS.values() for obj in objects]
    for obj in meshes:
        obj.select_set(True)
    bpy.context.view_layer.objects.active = meshes[0]
    return meshes


def validate_geometry(meshes):
    triangles = 0
    for obj in meshes:
        mesh = obj.data
        assert mesh.vertices and mesh.polygons, obj.name
        assert all(math.isfinite(value) for vertex in mesh.vertices for value in vertex.co)
        assert all(len(face.vertices) == 3 and face.area > 1e-10 for face in mesh.polygons), obj.name
        assert all(math.isfinite(value) and 0 <= value <= 1
                   for corner in mesh.uv_layers.active.data for value in corner.uv), obj.name
        triangles += len(mesh.polygons)
    return triangles


def export_models(out, meshes):
    for suffix, objects in (("", meshes), *[(f"_{name}", objects)
                                            for name, objects in GROUPS.items()]):
        bpy.ops.object.select_all(action="DESELECT")
        for obj in objects:
            obj.select_set(True)
        bpy.context.view_layer.objects.active = objects[0]
        bpy.ops.wm.obj_export(filepath=str(out / f"station_aurora{suffix}.obj"),
                              export_selected_objects=True, export_uv=True,
                              export_normals=True, export_materials=True,
                              export_triangulated_mesh=True, forward_axis="Y", up_axis="Z",
                              path_mode="RELATIVE")
    bpy.ops.object.select_all(action="DESELECT")
    for obj in meshes:
        obj.select_set(True)
    bpy.ops.export_scene.gltf(filepath=str(out / "station_aurora.glb"),
                              export_format="GLB", use_selection=True,
                              export_yup=True, export_texcoords=True, export_normals=True)


def set_camera(scene, position, target=(0, 0, -0.3)):
    scene.camera.location = position
    scene.camera.rotation_euler = (Vector(target) - scene.camera.location).to_track_quat("-Z", "Y").to_euler()


def setup_showcase(scene, samples, resolution):
    scene.render.engine = "CYCLES"
    scene.cycles.device = "CPU"
    scene.cycles.samples = samples
    scene.cycles.use_denoising = True
    scene.render.resolution_x, scene.render.resolution_y = resolution
    scene.render.resolution_percentage = 100
    scene.render.image_settings.file_format = "PNG"
    scene.world = bpy.data.worlds.new("DeepSpace")
    scene.world.use_nodes = True
    background = scene.world.node_tree.nodes.get("Background")
    background.inputs["Color"].default_value = (0.009, 0.015, 0.027, 1)
    background.inputs["Strength"].default_value = 0.5
    for name, position, energy, color, size in (
        ("SunKey", (4, -10, 14), 2800, (1.0, 0.88, 0.72), 9),
        ("BlueRim", (-8, 5, 5), 3500, (0.28, 0.62, 1.0), 8),
        ("SoftFill", (1, -5, 5), 1200, (0.70, 0.85, 1.0), 12),
    ):
        data = bpy.data.lights.new(name, "AREA")
        data.energy, data.color, data.size = energy, color, size
        obj = bpy.data.objects.new(name, data)
        obj.location = position
        obj.rotation_euler = (-obj.location).to_track_quat("-Z", "Y").to_euler()
        scene.collection.objects.link(obj)
    camera = bpy.data.cameras.new("ShowcaseCamera")
    camera.type = "ORTHO"
    camera.ortho_scale = 22.2
    scene.camera = bpy.data.objects.new("ShowcaseCamera", camera)
    scene.collection.objects.link(scene.camera)
    set_camera(scene, (11, -17, 18))
    for screen in bpy.data.screens:
        for area in screen.areas:
            if area.type == "VIEW_3D":
                area.spaces.active.region_3d.view_perspective = "CAMERA"
                area.spaces.active.shading.type = "MATERIAL"


def validate_obj_roundtrip(out, expected_triangles):
    bpy.ops.object.select_all(action="DESELECT")
    bpy.ops.wm.obj_import(filepath=str(out / "station_aurora.obj"),
                          forward_axis="Y", up_axis="Z")
    imported = list(bpy.context.selected_objects)
    actual = sum(len(obj.data.polygons) for obj in imported if obj.type == "MESH")
    assert actual == expected_triangles, (actual, expected_triangles)
    for obj in imported:
        bpy.data.objects.remove(obj, do_unlink=True)
    print(f"PASS: OBJ round-trip, {actual} triangles", flush=True)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--out", type=Path, default=ROOT / "blender/out/station_aurora")
    parser.add_argument("--quick", action="store_true")
    args = parser.parse_args(sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else [])
    out = args.out.resolve()
    out.mkdir(parents=True, exist_ok=True)
    bpy.ops.wm.read_factory_settings(use_empty=True)
    global MATERIAL
    GROUPS.clear()
    MATERIAL = make_atlas(out)
    build_station()
    meshes = prepare_meshes()
    triangles = validate_geometry(meshes)
    print(f"PASS: geometry, UVs and normals prepared; {len(meshes)} parts, {triangles} triangles", flush=True)
    export_models(out, meshes)
    validate_obj_roundtrip(out, triangles)
    scene = bpy.context.scene
    setup_showcase(scene, 12 if args.quick else 48,
                   (960, 640) if args.quick else (1600, 1066))
    bpy.ops.object.select_all(action="DESELECT")
    bpy.context.view_layer.objects.active = None
    bpy.ops.file.pack_all()
    bpy.ops.wm.save_as_mainfile(filepath=str(out / "station_aurora.blend"))
    scene.render.filepath = str(out / "station_aurora_showcase.png")
    bpy.ops.render.render(write_still=True)
    set_camera(scene, (0, 0, 24), (0, 0, 0))
    scene.camera.data.ortho_scale = 21.0
    scene.render.filepath = str(out / "station_aurora_front.png")
    bpy.ops.render.render(write_still=True)
    metadata = {
        "name": "Aurora", "parts": len(meshes), "triangles": triangles,
        "groups": {name: len(objects) for name, objects in GROUPS.items()},
        "obj_axes": {"forward": "Y", "up": "Z"},
        "texture_size": 1024, "obj_roundtrip": "passed",
        "integrated_into_game": False,
    }
    (out / "station_aurora_manifest.json").write_text(json.dumps(metadata, indent=2), encoding="utf-8")
    print(f"DONE: {out}", flush=True)


if __name__ == "__main__":
    main()
