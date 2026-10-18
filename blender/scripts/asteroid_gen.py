"""Deterministic procedural asteroid generator (Blender headless).

Usage:
  "C:\\Program Files\\Blender Foundation\\Blender 5.2\\blender.exe" -b -P blender/scripts/asteroid_gen.py

Outputs into blender/out/:
  asteroid_<seed>.obj / .mtl / .glb          (rubble-pile style, seeds 1..3)
  asteroid_lowpoly_<seed>.obj / .mtl / .glb  (faceted style, seeds 11..13)
  asteroids.blend
  preview_variants.png, preview_close.png
  asteroid_styles_compare.png    (rubble vs low-poly, labeled)
  asteroid_lowpoly_variants.png  (low-poly seeds 11..13)

Design (rubble-pile): unit icosphere -> normalized fBm (mathutils.noise
turbulence) radial displacement -> deterministic crater dents (seeded RNG) ->
per-axis squash. Design (low-poly): coarse icosphere subdiv 1, low-freq fBm
plus per-vertex jitter, flat shading (no smooth) -> faceted shard.
Same seed => same mesh, run to run (noise is position-hashed, RNG is seeded).
"""
import math
import os
import random

import bpy
from mathutils import Vector, noise

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(SCRIPT_DIR)
OUT_DIR = os.path.join(ROOT, "out")

VARIANTS = [
    (1, (1.00, 0.90, 0.82)),
    (2, (0.92, 1.00, 0.86)),
    (3, (0.96, 0.84, 1.00)),
]
LOWPOLY_VARIANTS = [
    (11, (1.02, 0.90, 0.92)),
    (12, (0.90, 1.00, 0.88)),
    (13, (0.95, 0.88, 1.02)),
]
LOWPOLY_SUBDIV = 2
SUBDIV = 5
FREQ = 1.7
AMP = 0.42
CRUNCH_FREQ = 3.6
CRUNCH_AMP = 0.14
CRATER_COUNT = 7
SPACING = 3.4


def sample_noise(p):
    try:
        return noise.turbulence(p, 5, False)
    except Exception:
        total = 0.0
        amp = 0.5
        freq = 1.0
        for _ in range(5):
            total += amp * (noise.noise(p * freq) + 1.0) * 0.5
            amp *= 0.5
            freq *= 2.0
        return total


def fbm_normalized(mesh, seed, freq, salt):
    off = Vector((seed * 13.7 + salt, seed * 7.3 - salt, seed * 3.1 + salt))
    vals = [sample_noise(v.co * freq + off) for v in mesh.vertices]
    lo, hi = min(vals), max(vals)
    span = max(hi - lo, 1e-6)
    return [(x - lo) / span for x in vals]


def crater_list(seed):
    rnd = random.Random(seed * 101 + 7)
    craters = []
    for _ in range(CRATER_COUNT):
        d = Vector((rnd.uniform(-1, 1), rnd.uniform(-1, 1), rnd.uniform(-1, 1)))
        if d.length < 1e-3:
            d = Vector((0.0, 0.0, 1.0))
        d.normalize()
        craters.append((d, rnd.uniform(0.20, 0.50), rnd.uniform(0.05, 0.11)))
    return craters


def make_asteroid(seed, axes):
    bpy.ops.mesh.primitive_ico_sphere_add(subdivisions=SUBDIV, radius=1.0)
    obj = bpy.context.active_object
    obj.name = "Asteroid_%d" % seed
    mesh = obj.data
    mesh.name = "AsteroidMesh_%d" % seed

    t = fbm_normalized(mesh, seed, FREQ, 0.0)
    c = fbm_normalized(mesh, seed, CRUNCH_FREQ, 5.0)
    craters = crater_list(seed)
    for i, v in enumerate(mesh.vertices):
        n = v.co.normalized()
        r = 1.0 + AMP * (t[i] - 0.5) + CRUNCH_AMP * (c[i] - 0.5)
        for cd, cr, depth in craters:
            ang = math.acos(max(-1.0, min(1.0, n.dot(cd))))
            if ang < cr:
                u = ang / cr
                bowl = 0.5 + 0.5 * math.cos(math.pi * u)
                rim = math.exp(-((u - 0.82) / 0.18) ** 2) * 0.06
                r -= depth * bowl
                r += depth * rim
        v.co = Vector((n.x * r * axes[0], n.y * r * axes[1], n.z * r * axes[2]))
    for poly in mesh.polygons:
        poly.use_smooth = True
    mesh.update()

    mat = bpy.data.materials.new("AsteroidRock_%d" % seed)
    mat.use_nodes = True
    bsdf = mat.node_tree.nodes.get("Principled BSDF")
    if bsdf is not None:
        bsdf.inputs["Base Color"].default_value = (0.24, 0.215, 0.19, 1.0)
        bsdf.inputs["Roughness"].default_value = 0.95
    obj.data.materials.append(mat)
    return obj


def make_lowpoly_asteroid(seed, axes):
    bpy.ops.mesh.primitive_ico_sphere_add(subdivisions=LOWPOLY_SUBDIV, radius=1.0)
    obj = bpy.context.active_object
    obj.name = "AsteroidLowPoly_%d" % seed
    mesh = obj.data
    mesh.name = "AsteroidLowPolyMesh_%d" % seed

    rnd = random.Random(seed * 313 + 11)
    t = fbm_normalized(mesh, seed, 1.05, 2.0)
    c = fbm_normalized(mesh, seed, 2.3, 9.0)
    craters = []
    for _ in range(4):
        d = Vector((rnd.uniform(-1, 1), rnd.uniform(-1, 1), rnd.uniform(-1, 1)))
        if d.length < 1e-3:
            d = Vector((0.0, 0.0, 1.0))
        d.normalize()
        craters.append((d, rnd.uniform(0.25, 0.50), rnd.uniform(0.10, 0.16)))
    for i, v in enumerate(mesh.vertices):
        n = v.co.normalized()
        r = 1.0 + 0.30 * (t[i] - 0.5) + 0.10 * (c[i] - 0.5) \
            + rnd.uniform(-0.055, 0.055)
        for cd, cr, depth in craters:
            ang = math.acos(max(-1.0, min(1.0, n.dot(cd))))
            if ang < cr:
                u = ang / cr
                bowl = 0.5 + 0.5 * math.cos(math.pi * u)
                rim = math.exp(-((u - 0.82) / 0.18) ** 2) * 0.05
                r -= depth * bowl
                r += depth * rim
        v.co = Vector((n.x * r * axes[0], n.y * r * axes[1], n.z * r * axes[2]))
    for poly in mesh.polygons:
        poly.use_smooth = False
    mesh.update()

    mat = bpy.data.materials.new("AsteroidLowPoly_%d" % seed)
    mat.use_nodes = True
    bsdf = mat.node_tree.nodes.get("Principled BSDF")
    if bsdf is not None:
        bsdf.inputs["Base Color"].default_value = (0.30, 0.285, 0.27, 1.0)
        bsdf.inputs["Roughness"].default_value = 0.82
    obj.data.materials.append(mat)
    return obj


def add_label(body, loc, cam_loc, size=0.42):
    bpy.ops.object.text_add(location=loc)
    tobj = bpy.context.active_object
    tobj.name = "Label_" + body.replace(" ", "_").replace("-", "").replace("(", "").replace(")", "")
    tobj.data.body = body
    tobj.data.align_x = "CENTER"
    tobj.data.align_y = "TOP"
    tobj.data.size = size
    direction = Vector(cam_loc) - Vector(loc)
    tobj.rotation_euler = direction.to_track_quat("Z", "Y").to_euler()
    mat = bpy.data.materials.new(tobj.name + "_Mat")
    mat.use_nodes = True
    bsdf = mat.node_tree.nodes.get("Principled BSDF")
    if bsdf is not None:
        bsdf.inputs["Base Color"].default_value = (0.85, 0.90, 1.0, 1.0)
        bsdf.inputs["Emission Color"].default_value = (0.85, 0.90, 1.0, 1.0)
        bsdf.inputs["Emission Strength"].default_value = 1.2
    tobj.data.materials.append(mat)
    return tobj


def setup_scene():
    scene = bpy.context.scene
    scene.render.engine = "CYCLES"
    scene.cycles.device = "CPU"
    scene.cycles.samples = 64
    scene.render.resolution_x = 720
    scene.render.resolution_y = 480
    scene.render.image_settings.file_format = "PNG"

    world = bpy.data.worlds.new("World")
    scene.world = world
    world.use_nodes = True
    bg = world.node_tree.nodes.get("Background")
    if bg is not None:
        bg.inputs[0].default_value = (0.035, 0.04, 0.05, 1.0)
        bg.inputs[1].default_value = 1.0

    bpy.ops.object.light_add(type="SUN", location=(4.0, -3.0, 5.0))
    sun = bpy.context.active_object
    sun.data.energy = 4.0
    sun.rotation_euler = (0.7, 0.15, 0.9)

    bpy.ops.object.camera_add(location=(0.0, 0.0, 0.0))
    cam = bpy.context.active_object
    cam.data.lens = 50
    scene.camera = cam
    return cam


def aim(cam, loc, target):
    cam.location = Vector(loc)
    direction = Vector(target) - cam.location
    cam.rotation_euler = direction.to_track_quat("-Z", "Y").to_euler()


def render_to(path):
    bpy.context.scene.render.filepath = path
    bpy.ops.render.render(write_still=True)
    print("RENDERED:", path)


def export_variant(obj, base):
    for o in bpy.context.scene.objects:
        o.select_set(False)
    obj.select_set(True)
    bpy.context.view_layer.objects.active = obj

    obj_path = os.path.join(OUT_DIR, base + ".obj")
    try:
        bpy.ops.wm.obj_export(filepath=obj_path, export_selected_objects=True)
    except TypeError:
        bpy.ops.wm.obj_export(filepath=obj_path)
    print("EXPORTED:", obj_path)

    glb_path = os.path.join(OUT_DIR, base + ".glb")
    try:
        bpy.ops.export_scene.gltf(
            filepath=glb_path, export_format="GLB", use_selection=True
        )
        print("EXPORTED:", glb_path)
    except Exception as exc:
        print("GLB export failed:", exc)


def main():
    os.makedirs(OUT_DIR, exist_ok=True)
    print("BLENDER:", bpy.app.version_string)
    bpy.ops.wm.read_factory_settings(use_empty=True)
    cam = setup_scene()

    objs = []
    for i, (seed, axes) in enumerate(VARIANTS):
        obj = make_asteroid(seed, axes)
        obj.location = (i * SPACING, 0.0, 0.0)
        objs.append(obj)

    low_objs = []
    for i, (seed, axes) in enumerate(LOWPOLY_VARIANTS):
        obj = make_lowpoly_asteroid(seed, axes)
        obj.location = (i * SPACING, 0.0, -SPACING)
        obj.hide_render = True
        low_objs.append(obj)

    for (seed, _axes), obj in zip(VARIANTS, objs):
        export_variant(obj, "asteroid_%d" % seed)
    for (seed, _axes), obj in zip(LOWPOLY_VARIANTS, low_objs):
        export_variant(obj, "asteroid_lowpoly_%d" % seed)

    blend_path = os.path.join(OUT_DIR, "asteroids.blend")

    mid = (len(VARIANTS) - 1) * SPACING * 0.5
    cam.data.lens = 35
    aim(cam, (mid + 2.2, -8.6, 3.3), (mid, 0.0, 0.0))
    render_to(os.path.join(OUT_DIR, "preview_variants.png"))

    cam.data.lens = 50
    aim(cam, (1.75, -2.15, 0.95), (0.0, 0.0, 0.0))
    render_to(os.path.join(OUT_DIR, "preview_close.png"))

    for obj in objs[1:]:
        obj.hide_render = True
    for obj in low_objs[1:]:
        obj.hide_render = True
    objs[0].location = (-1.75, 0.0, 0.0)
    low_objs[0].location = (1.75, 0.0, 0.0)
    low_objs[0].hide_render = False
    compare_cam = (0.0, -7.6, 2.9)
    cam.data.lens = 40
    aim(cam, compare_cam, (0.0, 0.0, -0.25))
    add_label("RUBBLE-PILE\n(CURRENT)", (-1.75, -0.45, -1.35), compare_cam,
              size=0.22)
    add_label("LOW-POLY\nFLAT-SHADED", (1.75, -0.45, -1.35), compare_cam,
              size=0.22)
    render_to(os.path.join(OUT_DIR, "asteroid_styles_compare.png"))

    for obj in list(bpy.context.scene.objects):
        if obj.name.startswith("Label_"):
            bpy.data.objects.remove(obj, do_unlink=True)
    for obj in objs:
        obj.hide_render = True
    for i, obj in enumerate(low_objs):
        obj.location = (i * SPACING, 0.0, 0.0)
        obj.hide_render = False
    cam.data.lens = 35
    aim(cam, (mid + 2.2, -8.6, 3.3), (mid, 0.0, 0.0))
    render_to(os.path.join(OUT_DIR, "asteroid_lowpoly_variants.png"))

    for i, obj in enumerate(objs):
        obj.location = (i * SPACING, 0.0, 0.0)
        obj.hide_render = False
    for i, obj in enumerate(low_objs):
        obj.location = (i * SPACING, 0.0, SPACING)
        obj.hide_render = False
    bpy.ops.wm.save_as_mainfile(filepath=blend_path)
    print("SAVED:", blend_path)

    print("DONE")


main()
