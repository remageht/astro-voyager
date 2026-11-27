"""Deterministic Interstellar Ark mesh, PBR bake and Cycles showcase.

Run with Blender 5.2:
  blender -b -P tools/build_interstellar_ark.py
"""

import math
import os
import struct
import sys

import bmesh
import bpy
import numpy as np
from mathutils import Matrix, Vector

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
MODELS_DIR = os.path.join(ROOT, "res", "models")
SCREENS_DIR = os.path.join(ROOT, "docs", "screens")
OUT_DIR = os.path.join(ROOT, "blender", "out", "final_showcase")
TOOLKIT_DIR = os.environ.get(
    "ASTRO_GENERATORS_DIR",
    os.path.join(os.path.dirname(ROOT), "astro-voyager-generators", "scripts"),
)
for directory in (MODELS_DIR, SCREENS_DIR, OUT_DIR):
    os.makedirs(directory, exist_ok=True)
if not os.path.isfile(os.path.join(TOOLKIT_DIR, "scifi_detailing_toolkit.py")):
    raise RuntimeError(f"Sci-fi detailing toolkit not found: {TOOLKIT_DIR}")
sys.path.append(TOOLKIT_DIR)
import scifi_detailing_toolkit as sdt


def active_object():
    return bpy.context.view_layer.objects.active


def write_bmp24(filepath, width, height, rgb_buffer):
    row_bytes = width * 3
    padding = (4 - row_bytes % 4) % 4
    stride = row_bytes + padding
    header = bytearray(54)
    header[0:2] = b"BM"
    struct.pack_into("<I", header, 2, 54 + stride * height)
    struct.pack_into("<I", header, 10, 54)
    struct.pack_into("<I", header, 14, 40)
    struct.pack_into("<i", header, 18, width)
    struct.pack_into("<i", header, 22, height)
    struct.pack_into("<H", header, 26, 1)
    struct.pack_into("<H", header, 28, 24)
    struct.pack_into("<I", header, 34, stride * height)
    struct.pack_into("<i", header, 38, 2835)
    struct.pack_into("<i", header, 42, 2835)
    with open(filepath, "wb") as handle:
        handle.write(header)
        for y in range(height):
            src_row = y * width * 3
            row = rgb_buffer[src_row:src_row + row_bytes]
            bgr = np.frombuffer(row, dtype=np.uint8).reshape(width, 3)[:, ::-1]
            handle.write(bgr.tobytes())
            if padding:
                handle.write(b"\0" * padding)
    print(f"[BMP] {os.path.basename(filepath)} {width}x{height}")


def save_bake_image(image, filepath):
    width, height = image.size
    pixels = np.asarray(image.pixels[:], dtype=np.float32).reshape(height, width, 4)
    rgb = np.flipud(pixels[:, :, :3])
    rgb = (np.clip(rgb, 0.0, 1.0) * 255.0 + 0.5).astype(np.uint8)
    write_bmp24(filepath, width, height, rgb.tobytes())


def material(name, color, metallic, roughness, emission=(0.0, 0.0, 0.0), strength=0.0):
    mat = bpy.data.materials.new(name)
    mat.use_nodes = True
    nodes = mat.node_tree.nodes
    links = mat.node_tree.links
    nodes.clear()
    output = nodes.new("ShaderNodeOutputMaterial")
    bsdf = nodes.new("ShaderNodeBsdfPrincipled")
    links.new(bsdf.outputs["BSDF"], output.inputs["Surface"])
    bsdf.inputs["Base Color"].default_value = (*color, 1.0)
    bsdf.inputs["Metallic"].default_value = metallic
    bsdf.inputs["Roughness"].default_value = roughness
    bsdf.inputs["Emission Color"].default_value = (*emission, 1.0)
    bsdf.inputs["Emission Strength"].default_value = strength

    texcoord = nodes.new("ShaderNodeTexCoord")
    noise = nodes.new("ShaderNodeTexNoise")
    noise.inputs["Scale"].default_value = 34.0
    noise.inputs["Detail"].default_value = 2.0
    noise.inputs["Roughness"].default_value = 0.62
    links.new(texcoord.outputs["UV"], noise.inputs["Vector"])
    ramp = nodes.new("ShaderNodeValToRGB")
    base = max(color)
    ramp.color_ramp.elements[0].position = 0.18
    ramp.color_ramp.elements[0].color = (*(c * 0.72 for c in color), 1.0)
    ramp.color_ramp.elements[1].position = 0.82
    ramp.color_ramp.elements[1].color = (*(min(1.0, c * 1.08 + base * 0.015) for c in color), 1.0)
    links.new(noise.outputs["Fac"], ramp.inputs["Fac"])
    links.new(ramp.outputs["Color"], bsdf.inputs["Base Color"])
    bump = nodes.new("ShaderNodeBump")
    bump.inputs["Strength"].default_value = 0.16
    bump.inputs["Distance"].default_value = 0.008
    links.new(noise.outputs["Fac"], bump.inputs["Height"])
    links.new(bump.outputs["Normal"], bsdf.inputs["Normal"])
    return mat


def finish_object(obj, mats, parts, bevel=0.0):
    if mats:
        for mat in mats:
            obj.data.materials.append(mat)
    if bevel > 0.0:
        modifier = obj.modifiers.new("MicroBevel", "BEVEL")
        modifier.width = bevel
        modifier.segments = 2
        modifier.limit_method = "ANGLE"
        modifier.angle_limit = math.radians(32.0)
        modifier.harden_normals = True
        bpy.context.view_layer.objects.active = obj
        bpy.ops.object.modifier_apply(modifier=modifier.name)
    parts.append(obj)
    return obj


def p_box(name, loc, dims, rot=(0.0, 0.0, 0.0), mats=None, parts=None, bevel=0.006):
    bpy.ops.mesh.primitive_cube_add(size=1.0, location=loc, rotation=rot)
    obj = active_object()
    obj.name = name
    obj.scale = dims
    bpy.ops.object.transform_apply(scale=True)
    return finish_object(obj, mats, parts, bevel)


def p_cyl(name, loc, radius, depth, rot=(0.0, 0.0, 0.0), segs=32,
          mats=None, parts=None, bevel=0.0):
    bpy.ops.mesh.primitive_cylinder_add(vertices=segs, radius=radius, depth=depth,
                                        location=loc, rotation=rot)
    obj = active_object()
    obj.name = name
    return finish_object(obj, mats, parts, bevel)


def p_cone(name, loc, r1, r2, depth, rot=(0.0, 0.0, 0.0), segs=32,
           mats=None, parts=None, bevel=0.0):
    bpy.ops.mesh.primitive_cone_add(vertices=segs, radius1=r1, radius2=r2,
                                    depth=depth, location=loc, rotation=rot)
    obj = active_object()
    obj.name = name
    return finish_object(obj, mats, parts, bevel)


def p_torus(name, loc, major, minor, rot=(0.0, 0.0, 0.0), segs=64, sides=12,
            mats=None, parts=None):
    bpy.ops.mesh.primitive_torus_add(major_segments=segs, minor_segments=sides,
                                     major_radius=major, minor_radius=minor,
                                     location=loc, rotation=rot)
    obj = active_object()
    obj.name = name
    return finish_object(obj, mats, parts)


def p_annular_cylinder(name, loc, outer_radius, inner_radius, depth, segs,
                       mats, parts):
    bm = bmesh.new()
    rings = []
    for z in (-depth * 0.5, depth * 0.5):
        outer = []
        inner = []
        for j in range(segs):
            angle = j * math.tau / segs
            outer.append(bm.verts.new((math.cos(angle) * outer_radius,
                                       math.sin(angle) * outer_radius, z)))
            inner.append(bm.verts.new((math.cos(angle) * inner_radius,
                                       math.sin(angle) * inner_radius, z)))
        rings.append((outer, inner))
    for side in range(2):
        outer, inner = rings[side]
        for j in range(segs):
            k = (j + 1) % segs
            bm.faces.new((outer[j], outer[k], inner[k], inner[j]))
    for j in range(segs):
        k = (j + 1) % segs
        bm.faces.new((rings[0][0][j], rings[1][0][j],
                      rings[1][0][k], rings[0][0][k]))
        bm.faces.new((rings[0][1][k], rings[1][1][k],
                      rings[1][1][j], rings[0][1][j]))
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces)
    mesh = bpy.data.meshes.new(f"{name}_Mesh")
    bm.to_mesh(mesh)
    bm.free()
    obj = bpy.data.objects.new(name, mesh)
    obj.location = loc
    bpy.context.collection.objects.link(obj)
    return finish_object(obj, mats, parts)


def p_beam(name, start, end, radius, mats, parts, segs=10):
    start = Vector(start)
    end = Vector(end)
    direction = end - start
    return p_cyl(name, (start + end) * 0.5, radius, direction.length,
                 rot=direction.to_track_quat("Z", "Y").to_euler(), segs=segs,
                 mats=mats, parts=parts)


def p_bell_nozzle(name, loc, throat, exit_radius, depth, mats, parts, segs=40):
    bm = bmesh.new()
    steps = 14
    outer = []
    inner = []
    for i in range(steps + 1):
        t = i / steps
        z = -depth * 0.5 + depth * t
        radius = throat + (exit_radius - throat) * t ** 1.55
        wall = 0.008 + 0.012 * t
        flute = 0.0025 * math.sin(math.pi * t) ** 0.7 * math.cos(t * math.pi * 8.0)
        ring_o = []
        ring_i = []
        for j in range(segs):
            angle = j * math.tau / segs
            ring_o.append(bm.verts.new((math.cos(angle) * (radius + flute),
                                        math.sin(angle) * (radius + flute), z)))
            ring_i.append(bm.verts.new((math.cos(angle) * (radius - wall),
                                        math.sin(angle) * (radius - wall), z)))
        outer.append(ring_o)
        inner.append(ring_i)
    for i in range(steps):
        for j in range(segs):
            k = (j + 1) % segs
            face = bm.faces.new((outer[i][j], outer[i + 1][j],
                                 outer[i + 1][k], outer[i][k]))
            face.material_index = 0
            face = bm.faces.new((inner[i][k], inner[i + 1][k],
                                 inner[i + 1][j], inner[i][j]))
            face.material_index = 1
    for j in range(segs):
        k = (j + 1) % segs
        face = bm.faces.new((outer[-1][j], outer[-1][k],
                             inner[-1][k], inner[-1][j]))
        face.material_index = 2
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces)
    mesh = bpy.data.meshes.new(f"{name}_Mesh")
    bm.to_mesh(mesh)
    bm.free()
    obj = bpy.data.objects.new(name, mesh)
    obj.location = loc
    bpy.context.collection.objects.link(obj)
    obj.data.materials.append(mats[0])
    obj.data.materials.append(mats[1])
    obj.data.materials.append(mats[2])
    parts.append(obj)

    p_cyl(f"{name}_Throat", (loc[0], loc[1], loc[2] + depth * 0.37),
          throat * 0.94, depth * 0.18, segs=segs // 2, mats=[mats[1]], parts=parts)
    p_cyl(f"{name}_Plasma", (loc[0], loc[1], loc[2] - depth * 0.18),
          throat * 0.95, 0.018, segs=segs // 2, mats=[mats[3]], parts=parts)
    p_torus(f"{name}_Collar", (loc[0], loc[1], loc[2] + depth * 0.34),
            throat * 1.28, throat * 0.13, segs=segs // 2, sides=8,
            mats=[mats[0]], parts=parts)


def p_dish(name, loc, radius, depth, rotation, mats, parts):
    bm = bmesh.new()
    rings = 14
    segs = 64
    front = []
    back = []
    for i in range(rings + 1):
        t = i / rings
        r = radius * t if i else 0.0001
        z = depth * t * t
        front.append([bm.verts.new((math.cos(j * math.tau / segs) * r,
                                    math.sin(j * math.tau / segs) * r, z))
                      for j in range(segs)])
        back.append([bm.verts.new((math.cos(j * math.tau / segs) * r,
                                   math.sin(j * math.tau / segs) * r,
                                   z - 0.018 * (1.0 - 0.55 * t)))
                     for j in range(segs)])
    for i in range(rings):
        for j in range(segs):
            k = (j + 1) % segs
            bm.faces.new((front[i][j], front[i + 1][j],
                          front[i + 1][k], front[i][k]))
            bm.faces.new((back[i][k], back[i + 1][k],
                          back[i + 1][j], back[i][j]))
    for j in range(segs):
        k = (j + 1) % segs
        bm.faces.new((front[-1][j], front[-1][k], back[-1][k], back[-1][j]))
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces)
    mesh = bpy.data.meshes.new(f"{name}_Mesh")
    bm.to_mesh(mesh)
    bm.free()
    obj = bpy.data.objects.new(name, mesh)
    obj.location = loc
    obj.rotation_euler = rotation
    bpy.context.collection.objects.link(obj)
    finish_object(obj, [mats[0]], parts)

    transform = Matrix.Translation(Vector(loc)) @ rotation.to_matrix().to_4x4()
    for j in range(12):
        angle = j * math.tau / 12
        a = transform @ Vector((0.0, 0.0, -0.05))
        b = transform @ Vector((math.cos(angle) * radius * 0.96,
                                 math.sin(angle) * radius * 0.96,
                                 depth * 0.92 - 0.03))
        p_beam(f"{name}_Rib_{j}", a, b, 0.009, [mats[1]], parts, segs=8)
    feed = transform @ Vector((0.0, 0.0, depth * 1.42))
    p_cyl(f"{name}_Feed", feed, 0.045, 0.22, rot=rotation,
          segs=16, mats=[mats[1]], parts=parts)
    for j in range(3):
        angle = j * math.tau / 3
        a = transform @ Vector((math.cos(angle) * radius * 0.78,
                                 math.sin(angle) * radius * 0.78, depth * 0.61))
        p_beam(f"{name}_FeedStrut_{j}", a, feed, 0.008, [mats[1]], parts, segs=8)


def add_toolkit_part(obj, mats, parts, name):
    obj.name = name
    if mats:
        obj.data.materials.clear()
        for mat in mats:
            obj.data.materials.append(mat)
    for modifier in list(obj.modifiers):
        bpy.context.view_layer.objects.active = obj
        bpy.ops.object.modifier_apply(modifier=modifier.name)
    parts.append(obj)


def set_bake_pass(mat, mode, bake_image):
    nodes = mat.node_tree.nodes
    links = mat.node_tree.links
    output = next(node for node in nodes if node.type == "OUTPUT_MATERIAL")
    bsdf = next(node for node in nodes if node.type == "BSDF_PRINCIPLED")
    emit = next((node for node in nodes if node.type == "EMISSION"), None)
    if emit is None:
        emit = nodes.new("ShaderNodeEmission")
    target = next((node for node in nodes if node.type == "TEX_IMAGE"), None)
    if target is None:
        target = nodes.new("ShaderNodeTexImage")
    target.image = bake_image
    nodes.active = target
    if mode == "PBR":
        links.new(bsdf.outputs["BSDF"], output.inputs["Surface"])
        return
    for link in list(links):
        if link.to_node == emit and link.to_socket == emit.inputs["Color"]:
            links.remove(link)
    emit.inputs["Strength"].default_value = 1.0
    if mode == "ALBEDO":
        source = next((node for node in nodes if node.type == "VALTORGB"), None)
        emit.inputs["Color"].default_value = bsdf.inputs["Base Color"].default_value
        if source is not None:
            links.new(source.outputs["Color"], emit.inputs["Color"])
    elif mode == "ROUGHNESS":
        value = bsdf.inputs["Roughness"].default_value
        emit.inputs["Color"].default_value = (value, value, value, 1.0)
    elif mode == "METALLIC":
        value = bsdf.inputs["Metallic"].default_value
        emit.inputs["Color"].default_value = (value, value, value, 1.0)
    elif mode == "EMISSIVE":
        color = bsdf.inputs["Emission Color"].default_value
        strength = bsdf.inputs["Emission Strength"].default_value
        emit.inputs["Color"].default_value = (
            *(1.0 - math.exp(-color[i] * strength) for i in range(3)), 1.0)
    links.new(emit.outputs["Emission"], output.inputs["Surface"])


def bake_pbr(obj, base_path, size=2048):
    scene = bpy.context.scene
    scene.render.engine = "CYCLES"
    scene.cycles.device = "CPU"
    scene.cycles.samples = 4
    scene.render.bake.margin = 12
    image = bpy.data.images.new("ArkBakeTarget", size, size, alpha=True)
    bpy.context.view_layer.objects.active = obj
    obj.select_set(True)
    materials = [mat for mat in obj.data.materials if mat and mat.use_nodes]
    passes = (("ALBEDO", "EMIT", "_albedo.bmp"),
              ("NORMAL", "NORMAL", "_normal.bmp"),
              ("ROUGHNESS", "EMIT", "_rough.bmp"),
              ("METALLIC", "EMIT", "_metal.bmp"),
              ("EMISSIVE", "EMIT", "_emissive.bmp"))
    for mode, bake_type, suffix in passes:
        print(f"[BAKE] {mode}")
        image.colorspace_settings.name = "sRGB" if mode in ("ALBEDO", "EMISSIVE") else "Non-Color"
        for mat in materials:
            set_bake_pass(mat, mode, image)
        if bake_type == "NORMAL":
            scene.cycles.bake_type = "NORMAL"
            scene.render.bake.normal_space = "TANGENT"
            scene.render.bake.normal_r = "POS_X"
            scene.render.bake.normal_g = "POS_Y"
            scene.render.bake.normal_b = "POS_Z"
            bpy.ops.object.bake(type="NORMAL")
        else:
            scene.cycles.bake_type = "EMIT"
            bpy.ops.object.bake(type="EMIT")
        save_bake_image(image, base_path + suffix)
    for mat in materials:
        set_bake_pass(mat, "PBR", image)
    bpy.data.images.remove(image)


def build_ark():
    bpy.ops.wm.read_factory_settings(use_empty=True)
    parts = []
    hull = material("Ark_Hull", (0.50, 0.54, 0.57), 0.55, 0.48)
    pale = material("Ark_Ceramic", (0.70, 0.72, 0.70), 0.15, 0.62)
    dark = material("Ark_Structure", (0.045, 0.058, 0.070), 0.76, 0.42)
    gold = material("Ark_MLI", (0.58, 0.34, 0.075), 0.86, 0.30)
    copper = material("Ark_Copper", (0.46, 0.19, 0.07), 0.90, 0.27)
    glass = material("Ark_Glass", (0.035, 0.18, 0.26), 0.18, 0.20,
                     (0.01, 0.22, 0.34), 0.65)
    cyan = material("Ark_Cyan", (0.03, 0.26, 0.34), 0.35, 0.30,
                    (0.01, 0.42, 0.68), 1.8)
    amber = material("Ark_Amber", (0.58, 0.22, 0.025), 0.30, 0.36,
                     (0.80, 0.12, 0.01), 1.5)
    radiator = material("Ark_Radiator", (0.72, 0.76, 0.79), 0.42, 0.46)
    nozzle = material("Ark_Nozzle", (0.16, 0.18, 0.20), 0.94, 0.24)
    inner = material("Ark_NozzleInner", (0.035, 0.040, 0.046), 0.72, 0.48)
    lip = material("Ark_NozzleLip", (0.38, 0.40, 0.42), 0.96, 0.18)
    plasma = material("Ark_Plasma", (0.08, 0.50, 0.92), 0.10, 0.20,
                      (0.06, 0.55, 1.0), 7.0)

    p_cyl("Spine_PressureHull", (0, 0, 0.15), 0.40, 4.90,
          segs=48, mats=[hull], parts=parts, bevel=0.018)
    p_cyl("Spine_DarkCore", (0, 0, -0.15), 0.43, 3.65,
          segs=48, mats=[dark], parts=parts, bevel=0.014)
    for i, z in enumerate((-2.15, -1.72, -0.92, -0.18, 0.62, 1.36, 1.92)):
        p_cyl(f"Hull_Band_{i}", (0, 0, z), 0.46 if i < 5 else 0.43,
              0.12, segs=48, mats=[dark], parts=parts, bevel=0.008)
        p_torus(f"Hull_BandLip_{i}", (0, 0, z + 0.055), 0.45, 0.018,
                segs=48, sides=8, mats=[lip], parts=parts)

    p_cone("Bridge_ArmoredCone", (0, 0, 2.55), 0.48, 0.36, 1.02,
           segs=48, mats=[pale], parts=parts, bevel=0.012)
    p_cone("Bridge_DarkFairing", (0, 0, 2.22), 0.52, 0.48, 0.40,
           segs=48, mats=[dark], parts=parts, bevel=0.012)
    p_cone("Nose_Cap", (0, 0, 3.16), 0.36, 0.29, 0.34,
           segs=40, mats=[hull], parts=parts, bevel=0.008)
    for i in range(12):
        angle = i * math.tau / 12
        radius = 0.405
        p_box(f"Bridge_Window_{i}",
              (math.cos(angle) * radius, math.sin(angle) * radius, 2.69),
              (0.09, 0.022, 0.20), rot=(0, 0, angle),
              mats=[glass], parts=parts, bevel=0.008)
    for z in (1.93, 2.12, 2.31):
        p_torus(f"Bridge_Collar_{z}", (0, 0, z), 0.49, 0.026,
                segs=48, sides=10, mats=[lip], parts=parts)
    p_cyl("Sensor_Package", (0, 0, 3.43), 0.11, 0.20,
          segs=24, mats=[dark], parts=parts, bevel=0.01)
    p_cyl("Sensor_Aperture", (0, 0, 3.54), 0.065, 0.025,
          segs=24, mats=[glass], parts=parts)
    for i in range(4):
        angle = i * math.tau / 4 + math.pi / 4
        p_box(f"Nose_Sensor_{i}",
              (math.cos(angle) * 0.34, math.sin(angle) * 0.34, 3.13),
              (0.07, 0.045, 0.13), rot=(0, 0, angle),
              mats=[dark], parts=parts, bevel=0.006)
        p_cyl(f"Nose_Lens_{i}",
              (math.cos(angle) * 0.365, math.sin(angle) * 0.365, 3.16),
              0.022, 0.030, rot=(math.pi / 2, 0, angle),
              segs=12, mats=[glass], parts=parts)

    dish_rotation = Vector((0.25, -0.60, 0.20)).to_track_quat("Z", "Y").to_euler()
    p_cyl("Dish_Gimbal", (0.22, 0.38, 2.15), 0.12, 0.30,
          segs=24, mats=[dark], parts=parts, bevel=0.01)
    p_dish("Dish_HighGain", (0.22, 0.42, 2.34), 0.48, 0.14,
           dish_rotation, [gold, lip], parts)

    drum_z = 0.55
    drum_radius = 2.35
    p_annular_cylinder("Habitat_PressureDrum", (0, 0, drum_z),
                       drum_radius, 0.52, 2.00, 128, [hull], parts)
    for side in (-1, 1):
        p_torus(f"Habitat_EndRing_{side}", (0, 0, drum_z + side * 1.00),
                drum_radius - 0.02, 0.070, segs=128, sides=14,
                mats=[dark], parts=parts)
        p_torus(f"Habitat_HubRing_{side}", (0, 0, drum_z + side * 1.00),
                0.55, 0.055, segs=48, sides=10,
                mats=[lip], parts=parts)
    for z in (drum_z - 0.74, drum_z - 0.37, drum_z,
              drum_z + 0.37, drum_z + 0.74):
        p_torus(f"Habitat_CircumferentialBand_{z}", (0, 0, z),
                drum_radius + 0.008, 0.020, segs=128, sides=8,
                mats=[dark], parts=parts)
    for i in range(48):
        angle = i * math.tau / 48
        p_box(f"Habitat_ArmorPanel_{i}",
              (math.cos(angle) * (drum_radius + 0.014),
               math.sin(angle) * (drum_radius + 0.014), drum_z),
              (0.034, 0.292, 1.82), rot=(0, 0, angle),
              mats=[hull if i % 4 else pale], parts=parts, bevel=0.004)
        if i % 2 == 0:
            p_box(f"Habitat_Window_{i}",
                  (math.cos(angle) * (drum_radius + 0.034),
                   math.sin(angle) * (drum_radius + 0.034), drum_z + 0.46),
                  (0.020, 0.17, 0.060), rot=(0, 0, angle),
                  mats=[glass], parts=parts, bevel=0.003)
        if i % 4 == 0:
            p_box(f"Habitat_WindowLower_{i}",
                  (math.cos(angle) * (drum_radius + 0.034),
                   math.sin(angle) * (drum_radius + 0.034), drum_z - 0.46),
                  (0.020, 0.17, 0.060), rot=(0, 0, angle),
                  mats=[glass], parts=parts, bevel=0.003)
        if i % 6 == 0:
            p_box(f"Habitat_ServiceHatch_{i}",
                  (math.cos(angle) * (drum_radius + 0.038),
                   math.sin(angle) * (drum_radius + 0.038), drum_z),
                  (0.024, 0.15, 0.22), rot=(0, 0, angle),
                  mats=[dark], parts=parts, bevel=0.004)
    for i in range(12):
        angle = i * math.tau / 12
        p_box(f"Habitat_LongitudinalRail_{i}",
              (math.cos(angle) * (drum_radius + 0.044),
               math.sin(angle) * (drum_radius + 0.044), drum_z),
              (0.052, 0.052, 1.94), rot=(0, 0, angle),
              mats=[lip], parts=parts, bevel=0.005)
    for i in range(8):
        angle = (i + 0.5) * math.tau / 8
        p_cyl(f"Observation_Pod_{i}",
              (math.cos(angle) * 1.92, math.sin(angle) * 1.92, drum_z + 1.06),
              0.13, 0.14, segs=24, mats=[pale], parts=parts, bevel=0.008)
        p_cyl(f"Observation_Glass_{i}",
              (math.cos(angle) * 1.92, math.sin(angle) * 1.92, drum_z + 1.14),
              0.088, 0.018, segs=24, mats=[glass], parts=parts)
    for i in range(4):
        angle = i * math.tau / 4 + math.pi / 4
        x = math.cos(angle) * 1.18
        y = math.sin(angle) * 1.18
        p_cyl(f"Habitat_Dock_{i}", (x, y, drum_z + 1.04),
              0.15, 0.14, segs=24, mats=[dark], parts=parts, bevel=0.008)
        p_torus(f"Habitat_DockRing_{i}", (x, y, drum_z + 1.12),
                0.15, 0.020, segs=24, sides=8, mats=[lip], parts=parts)
        p_cyl(f"Habitat_DockHatch_{i}", (x, y, drum_z + 1.125),
              0.105, 0.020, segs=24, mats=[hull], parts=parts)

    tech_z = -1.30
    tech_radius = 0.92
    p_annular_cylinder("TechCentrifuge_Drum", (0, 0, tech_z),
                       tech_radius, 0.43, 0.58, 64, [dark], parts)
    for side in (-1, 1):
        p_torus(f"TechCentrifuge_EndRing_{side}", (0, 0, tech_z + side * 0.29),
                tech_radius, 0.038, segs=64, sides=10,
                mats=[lip], parts=parts)
    for i in range(24):
        angle = i * math.tau / 24
        p_box(f"TechCentrifuge_Module_{i}",
              (math.cos(angle) * (tech_radius + 0.016),
               math.sin(angle) * (tech_radius + 0.016), tech_z),
              (0.024, 0.19, 0.42), rot=(0, 0, angle),
              mats=[hull if i % 3 else dark], parts=parts, bevel=0.004)
    for i in range(4):
        angle = i * math.tau / 4
        p_box(f"TechCentrifuge_Equipment_{i}",
              (math.cos(angle) * 0.98, math.sin(angle) * 0.98, tech_z),
              (0.18, 0.14, 0.30), rot=(0, 0, angle),
              mats=[gold if i == 0 else dark], parts=parts, bevel=0.006)

    for i in range(12):
        angle = i * math.tau / 12
        radius = drum_radius + 0.08
        center = Vector((math.cos(angle) * radius, math.sin(angle) * radius, drum_z))
        p_cyl(f"CryoPod_{i}", center, 0.14, 1.25,
              segs=24, mats=[pale], parts=parts, bevel=0.012)
        for end in (-0.63, 0.63):
            p_cone(f"CryoPod_Cap_{i}_{end}", center + Vector((0, 0, end)),
                   0.14, 0.055, 0.18, segs=24,
                   mats=[hull], parts=parts,
                   rot=(math.pi if end < 0 else 0, 0, 0), bevel=0.004)
        p_torus(f"CryoPod_StrapA_{i}", center + Vector((0, 0, -0.38)),
                0.144, 0.014, segs=24, sides=8, mats=[dark], parts=parts)
        p_torus(f"CryoPod_StrapB_{i}", center + Vector((0, 0, 0.38)),
                0.144, 0.014, segs=24, sides=8, mats=[dark], parts=parts)
        p_box(f"CryoPod_Status_{i}", center + Vector((0, 0, 0.04)),
              (0.050, 0.028, 0.62), rot=(0, 0, angle),
              mats=[cyan], parts=parts, bevel=0.003)

    for side in (-1, 1):
        panel = sdt.create_radiator_panel(f"Radiator_{side}", length=2.30,
                                          width=1.05, vanes=28)
        panel.rotation_euler = (0, math.pi / 2, 0)
        panel.location = (side * 3.05, 0, -0.75)
        add_toolkit_part(panel, [radiator, dark], parts, f"RadiatorWing_{side}")
        p_box(f"Radiator_Backbone_{side}", (side * 3.02, 0, -0.75),
              (0.08, 1.14, 2.44), mats=[dark], parts=parts, bevel=0.008)
        p_cyl(f"Radiator_Manifold_{side}", (side * 1.72, 0, -0.75),
              0.060, 2.34, rot=(0, math.pi / 2, 0),
              segs=16, mats=[copper], parts=parts, bevel=0.006)
        for y in (-0.42, 0.42):
            for z in (-1.55, 0.05):
                p_beam(f"Radiator_Support_{side}_{y}_{z}",
                       (side * 0.46, y, z), (side * 2.82, y, z),
                       0.032, [dark], parts, segs=10)

    for i in range(4):
        angle = math.pi / 4 + i * math.pi / 2
        loc = (math.cos(angle) * 0.72, math.sin(angle) * 0.72, -1.98)
        p_cyl(f"FuelTank_{i}", loc, 0.17, 1.05,
              segs=28, mats=[gold], parts=parts, bevel=0.018)
        for z in (-2.49, -1.47):
            p_torus(f"FuelTank_Band_{i}_{z}", (loc[0], loc[1], z),
                    0.174, 0.018, segs=28, sides=8, mats=[dark], parts=parts)

    p_cyl("Engine_ThrustFrame", (0, 0, -2.70), 0.62, 0.34,
          segs=48, mats=[dark], parts=parts, bevel=0.018)
    p_torus("Engine_OuterBrace", (0, 0, -2.82), 0.56, 0.055,
            segs=64, sides=12, mats=[lip], parts=parts)
    engine_offsets = ((0.29, 0.29), (-0.29, 0.29),
                      (0.29, -0.29), (-0.29, -0.29))
    for i, (x, y) in enumerate(engine_offsets):
        p_bell_nozzle(f"FusionEngine_{i}", (x, y, -3.12), 0.070, 0.235, 0.72,
                      [nozzle, inner, lip, plasma], parts, segs=44)
        p_cyl(f"FusionEngine_ActuatorA_{i}", (x + 0.15, y, -2.80),
              0.025, 0.34, segs=12, mats=[copper], parts=parts, bevel=0.004)
        p_cyl(f"FusionEngine_ActuatorB_{i}", (x, y + 0.15, -2.80),
              0.025, 0.34, segs=12, mats=[copper], parts=parts, bevel=0.004)
    p_bell_nozzle("IonVectorEngine", (0, 0, -3.23), 0.10, 0.20, 0.82,
                  [nozzle, inner, lip, plasma], parts, segs=48)
    for i in range(6):
        angle = i * math.tau / 6
        loc = (math.cos(angle) * 0.49, math.sin(angle) * 0.49, -2.98)
        p_bell_nozzle(f"Vernier_{i}", loc, 0.025, 0.070, 0.30,
                      [nozzle, inner, lip, cyan], parts, segs=24)

    for i in range(4):
        angle = i * math.tau / 4 + math.pi / 4
        rcs = sdt.create_rcs_quad(f"RCS_{i}", scale=0.17)
        rcs.location = (math.cos(angle) * 0.49, math.sin(angle) * 0.49, 1.50)
        rcs.rotation_euler.z = angle
        add_toolkit_part(rcs, [dark, pale, lip], parts, f"RCS_Cluster_{i}")

    dock = sdt.create_docking_port_apas("Docking_APAS", radius=0.25)
    for modifier in list(dock.modifiers):
        if modifier.type == "BEVEL":
            dock.modifiers.remove(modifier)
    dock.location = (0, 0, -2.30)
    dock.rotation_euler.x = math.pi
    add_toolkit_part(dock, [dark, lip, glass], parts, "Aft_DockingPort")

    for i in range(18):
        angle = i * math.tau / 18
        z = 1.66 + (i % 3) * 0.18 if i < 9 else -2.02 + ((i - 9) % 3) * 0.18
        p_box(f"Hull_Greeble_{i}",
              (math.cos(angle) * 0.445, math.sin(angle) * 0.445, z),
              (0.08, 0.045, 0.16), rot=(0, 0, angle),
              mats=[dark if i % 2 else hull], parts=parts, bevel=0.004)
    for i in range(10):
        angle = i * math.tau / 10
        p_cyl(f"Service_Conduit_{i}",
              (math.cos(angle) * 0.47, math.sin(angle) * 0.47, 1.92),
              0.016, 1.30, segs=8,
              mats=[copper if i % 2 else lip], parts=parts)
    for i in range(8):
        angle = i * math.tau / 8
        p_box(f"Navigation_Light_{i}",
              (math.cos(angle) * 0.49, math.sin(angle) * 0.49, 1.82),
              (0.035, 0.035, 0.055), mats=[cyan if i % 3 else amber],
              parts=parts, bevel=0.006)

    bpy.ops.object.select_all(action="DESELECT")
    for obj in parts:
        obj.select_set(True)
    bpy.context.view_layer.objects.active = parts[0]
    bpy.ops.object.join()
    ark = active_object()
    ark.name = "ark_ship"
    bpy.ops.object.mode_set(mode="EDIT")
    bpy.ops.mesh.select_all(action="SELECT")
    bpy.ops.uv.smart_project(angle_limit=58.0, island_margin=0.006)
    bpy.ops.object.mode_set(mode="OBJECT")
    bpy.ops.object.shade_smooth()
    triangulate = ark.modifiers.new("GameTriangulation", "TRIANGULATE")
    triangulate.quad_method = "BEAUTY"
    triangulate.ngon_method = "BEAUTY"
    bpy.context.view_layer.objects.active = ark
    bpy.ops.object.modifier_apply(modifier=triangulate.name)
    bpy.ops.object.mode_set(mode="EDIT")
    bm_ark = bmesh.from_edit_mesh(ark.data)
    bmesh.ops.dissolve_degenerate(bm_ark, dist=1e-5)
    tiny_ark = [f for f in bm_ark.faces if f.calc_area() < 1e-8]
    if tiny_ark:
        bmesh.ops.delete(bm_ark, geom=tiny_ark, context="FACES")
    bmesh.update_edit_mesh(ark.data)
    bpy.ops.mesh.select_all(action="SELECT")
    bpy.ops.mesh.normals_make_consistent(inside=False)
    bpy.ops.object.mode_set(mode="OBJECT")
    polygons = len(ark.data.polygons)
    vertices = len(ark.data.vertices)
    print(f"[MESH] {vertices} vertices, {polygons} faces")

    base_path = os.path.join(MODELS_DIR, "ark_ship")
    bake_pbr(ark, base_path, size=2048)
    out_obj = base_path + ".obj"
    bpy.ops.wm.obj_export(filepath=out_obj, export_selected_objects=True,
                          apply_modifiers=True, export_uv=True,
                          export_normals=True, export_materials=True,
                          forward_axis="Z", up_axis="Y")
    print(f"[OBJ] {out_obj}")

    bpy.ops.wm.save_as_mainfile(filepath=os.path.join(OUT_DIR, "ark_ship.blend"))
    render_showcase(ark)
    render_angles(ark)


def render_showcase(ark):
    scene = bpy.context.scene
    scene.render.engine = "CYCLES"
    scene.cycles.device = "CPU"
    scene.cycles.samples = 96
    scene.cycles.use_denoising = True
    scene.render.resolution_x = 1600
    scene.render.resolution_y = 1000
    scene.render.resolution_percentage = 100
    scene.render.image_settings.file_format = "PNG"
    scene.render.film_transparent = False
    world = bpy.data.worlds.new("Ark_DeepSpace")
    world.use_nodes = True
    background = next(node for node in world.node_tree.nodes if node.type == "BACKGROUND")
    background.inputs["Color"].default_value = (0.002, 0.004, 0.010, 1.0)
    background.inputs["Strength"].default_value = 0.32
    scene.world = world

    bpy.ops.mesh.primitive_uv_sphere_add(segments=64, ring_count=32, radius=12.0,
                                        location=(-10.0, 15.0, -8.0))
    planet = active_object()
    planet.name = "Showcase_Planet"
    bpy.ops.object.shade_smooth()
    mat = bpy.data.materials.new("Showcase_PlanetMat")
    mat.use_nodes = True
    nodes = mat.node_tree.nodes
    links = mat.node_tree.links
    nodes.clear()
    output = nodes.new("ShaderNodeOutputMaterial")
    bsdf = nodes.new("ShaderNodeBsdfPrincipled")
    links.new(bsdf.outputs["BSDF"], output.inputs["Surface"])
    bsdf.inputs["Base Color"].default_value = (0.035, 0.16, 0.34, 1.0)
    bsdf.inputs["Roughness"].default_value = 0.72
    bsdf.inputs["Metallic"].default_value = 0.05
    planet.data.materials.append(mat)

    bpy.ops.object.light_add(type="SUN", location=(6, -8, 10))
    sun = active_object()
    sun.data.energy = 4.5
    sun.data.color = (1.0, 0.91, 0.78)
    sun.rotation_euler = (math.radians(28), math.radians(-24), math.radians(18))
    bpy.ops.object.light_add(type="AREA", location=(-5, -2, 3))
    key = active_object()
    key.data.energy = 900
    key.data.shape = "DISK"
    key.data.size = 5
    key.data.color = (0.50, 0.72, 1.0)
    key.rotation_euler = (Vector((0, 0, 0)) - key.location).to_track_quat("-Z", "Y").to_euler()
    bpy.ops.object.light_add(type="AREA", location=(2, 5, -4))
    rim = active_object()
    rim.data.energy = 1100
    rim.data.shape = "DISK"
    rim.data.size = 4
    rim.data.color = (0.18, 0.48, 1.0)
    rim.rotation_euler = (Vector((0, 0, 0)) - rim.location).to_track_quat("-Z", "Y").to_euler()

    bpy.ops.object.camera_add(location=(10.0, -11.0, 5.5))
    camera = active_object()
    scene.camera = camera
    camera.rotation_euler = (Vector((0, 0, 0.15)) - camera.location).to_track_quat("-Z", "Y").to_euler()
    camera.rotation_euler.rotate_axis("Z", math.radians(-18))
    camera.data.lens = 50
    scene.render.filepath = os.path.join(SCREENS_DIR, "ark_ship_showcase.png")
    bpy.ops.render.render(write_still=True)


def render_angles(ark):
    scene = bpy.context.scene
    scene.render.resolution_x = 1600
    scene.render.resolution_y = 1000
    scene.cycles.samples = 48
    camera = scene.camera
    views = (("three_quarter", (10.0, -11.0, 5.5), (0, 0, 0.15), 50),
             ("port", (-10.5, -0.4, 2.4), (0, 0, 0.1), 52),
             ("aft", (7.0, -8.0, -7.0), (0, 0, -0.1), 52),
             ("engines", (4.2, -5.8, -3.6), (0, 0, -2.65), 58))
    for name, position, target, lens in views:
        camera.location = position
        camera.rotation_euler = (Vector(target) - camera.location).to_track_quat("-Z", "Y").to_euler()
        camera.rotation_euler.rotate_axis("Z", math.radians(20 if name == "engines" else -12))
        camera.data.lens = lens
        scene.render.filepath = os.path.join(OUT_DIR, f"ark_ship_{name}.png")
        bpy.ops.render.render(write_still=True)


if __name__ == "__main__":
    build_ark()
    print("[DONE] Interstellar Ark remaster complete")
