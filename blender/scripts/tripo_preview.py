"""Tripo FBX reference preview: import the owner's ring-ship example and
render deterministic orbit stills for evaluation (not for shipping).

Usage:
  & "C:\\Program Files\\Blender Foundation\\Blender 5.2\\blender.exe" -b -P "blender\\scripts\\tripo_preview.py"

Artifacts: blender/out/tripo_ref/tripo_hero.png, tripo_rear.png, tripo_side.png,
tripo_top.png + printed stats (tris, dimensions, materials, textures).
"""

import math
import os

import bpy
from mathutils import Vector

FBX_PATH = (
    r"C:\Users\Эмир\Downloads\Telegram Desktop"
    r"\ImageToStl_com_tripo_pbr_model_9b7e5624_8e9b_455d_8e12_1cd9ee957e79.fbx"
)
HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
OUT_DIR = os.path.join(REPO, "blender", "out", "tripo_ref")

VIEWS = [
    # name, azimuth(deg, 0 = +X), elevation(deg), lens
    ("tripo_hero", 35.0, 18.0, 50.0),
    ("tripo_rear", 210.0, 12.0, 50.0),
    ("tripo_side", 90.0, 4.0, 55.0),
    ("tripo_top", 35.0, 65.0, 50.0),
]


def main():
    bpy.ops.wm.read_factory_settings(use_empty=True)

    try:
        bpy.ops.preferences.addon_enable(module="io_scene_fbx")
    except Exception:
        pass  # built-in in 4.x/5.x

    print("importing:", FBX_PATH)
    bpy.ops.import_scene.fbx(filepath=FBX_PATH)

    meshes = [o for o in bpy.data.objects if o.type == "MESH"]
    if not meshes:
        print("ERROR: no mesh objects imported")
        return 1

    # --- stats + bounding box (world space) ---
    tris = 0
    for o in meshes:
        me = o.data
        me.calc_loop_triangles()
        tris += len(me.loop_triangles)
    mn = Vector((1e18, 1e18, 1e18))
    mx = Vector((-1e18, -1e18, -1e18))
    for o in meshes:
        for c in o.bound_box:
            w = o.matrix_world @ Vector(c)
            mn = Vector((min(mn[i], w[i]) for i in range(3)))
            mx = Vector((max(mx[i], w[i]) for i in range(3)))
    size = mx - mn
    center = (mn + mx) * 0.5
    radius = max(size) * 0.5
    print("meshes:", len(meshes), "tris:", tris)
    print("dims:", tuple(round(v, 3) for v in size))
    mats = {m.name for o in meshes for m in o.data.materials if m}
    print("materials:", sorted(mats))
    imgs = [i.name for i in bpy.data.images if i.name != "Render Result"]
    print("textures:", len(imgs), imgs[:8])

    # --- world + lights ---
    world = bpy.data.worlds.new("W")
    bpy.context.scene.world = world
    world.use_nodes = True
    bg = world.node_tree.nodes["Background"]
    bg.inputs[0].default_value = (0.035, 0.045, 0.06, 1.0)
    bg.inputs[1].default_value = 1.0

    sun = bpy.data.objects.new("Sun", bpy.data.lights.new("Sun", "SUN"))
    bpy.context.collection.objects.link(sun)
    sun.data.energy = 3.0
    sun.rotation_euler = (math.radians(55.0), 0.0, math.radians(35.0))

    key = bpy.data.objects.new("Key", bpy.data.lights.new("Key", "AREA"))
    bpy.context.collection.objects.link(key)
    key.data.energy = 4000.0 * max(radius, 1.0) ** 2
    key.data.size = max(radius * 2.0, 1.0)
    key.location = center + Vector((radius * 1.8, -radius * 1.6, radius * 1.4))
    key.rotation_euler = (math.radians(55.0), 0.0, math.radians(40.0))

    # --- camera ---
    cam_data = bpy.data.cameras.new("Cam")
    cam = bpy.data.objects.new("Cam", cam_data)
    bpy.context.collection.objects.link(cam)
    bpy.context.scene.camera = cam

    scene = bpy.context.scene
    scene.render.engine = "CYCLES"
    scene.cycles.device = "CPU"
    scene.cycles.samples = 32
    scene.cycles.use_denoising = True
    scene.render.resolution_x = 960
    scene.render.resolution_y = 540
    scene.render.film_transparent = False

    os.makedirs(OUT_DIR, exist_ok=True)
    for name, az, el, lens in VIEWS:
        cam_data.lens = lens
        dist = radius * 3.1
        a = math.radians(az)
        e = math.radians(el)
        pos = center + Vector(
            (dist * math.cos(e) * math.cos(a),
             dist * math.cos(e) * math.sin(a),
             dist * math.sin(e))
        )
        cam.location = pos
        look = center - pos
        cam.rotation_euler = look.to_track_quat("-Z", "Y").to_euler()
        path = os.path.join(OUT_DIR, name + ".png")
        scene.render.filepath = path
        bpy.ops.render.render(write_still=True)
        print("rendered:", path)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
