"""Render NASA Science spacecraft showcases from the official 3D Resources GLBs.

The GLB sources are local, git-ignored assets downloaded by
`tools/fetch_nasa_assets.ps1` into `blender/out/nasa_src/`.  This script
imports them, normalises scale/orientation, lights them with the same
cinematic rig as the other astro-voyager showcases, and uses the NASA/Goddard
SVS "Deep Star Maps 2020" all-sky map as the background when it is available.

Usage (headless):
  powershell -ExecutionPolicy Bypass -File tools/fetch_nasa_assets.ps1
  blender -b -P blender/scripts/render_nasa_showcase.py -- --model voyager
  blender -b -P blender/scripts/render_nasa_showcase.py -- --model hubble --quick
  blender -b -P blender/scripts/render_nasa_showcase.py -- --all
  blender -b -P blender/scripts/render_nasa_showcase.py -- --model parker --out custom.png

Presets: voyager, hubble, parker.
Final renders go to blender/out/final_showcase/<name>_showcase.png and are
copied to docs/screens/<name>_showcase.png.  --quick writes 640x360 previews
to blender/out/nasa_preview/ and never touches docs/screens.

Credits:
  Voyager Probe (B), Hubble Space Telescope (A), Parker Solar Probe:
    NASA 3D Resources (public domain, NASA media usage guidelines).
  Deep Star Maps 2020:
    NASA/Goddard Space Flight Center Scientific Visualization Studio,
    Gaia DR2: ESA/Gaia/DPAC.
"""
import math
import os
import shutil
import sys

import bpy
from mathutils import Matrix, Vector

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
PROJECT_ROOT = os.path.dirname(SCRIPT_DIR)  # blender/
REPO_ROOT = os.path.dirname(PROJECT_ROOT)   # astro-voyager/
SRC_DIR = os.path.join(PROJECT_ROOT, "out", "nasa_src")
OUT_FINAL = os.path.join(PROJECT_ROOT, "out", "final_showcase")
OUT_QUICK = os.path.join(PROJECT_ROOT, "out", "nasa_preview")
DOCS_SCREENS = os.path.join(REPO_ROOT, "docs", "screens")
STARMAP = os.path.join(REPO_ROOT, "res", "textures", "starmap_2020_4k_gal.exr")

# Per-model hero framing.  Azimuth/elevation are in degrees, distance is in
# the normalised 4-unit model space, and the target offset shifts the camera
# pivot toward the interesting part of a long boom/dish.
PRESETS = {
    "voyager": {
        "glb": "voyager_b.glb",
        "out": "voyager_showcase.png",
        "az": 35.0,
        "el": 15.0,
        "dist": 7.3,
        "target": (0.0, 1.4, 0.0),
        "rot": (0.0, 0.0, 0.0),
    },
    "hubble": {
        "glb": "hubble_a.glb",
        "out": "hubble_showcase.png",
        "az": 35.0,
        "el": 18.0,
        "dist": 16.0,
        "target": (0.0, 0.0, 0.0),
        "rot": (90.0, 0.0, 0.0),
    },
    "parker": {
        "glb": "parker.glb",
        "out": "parker_showcase.png",
        "az": 35.0,
        "el": 18.0,
        "dist": 9.5,
        "target": (0.0, 0.0, 0.0),
        "rot": (0.0, 0.0, 0.0),
    },
}


def argv():
    return sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []


def arg_value(flag, default=None):
    a = argv()
    if flag in a:
        i = a.index(flag)
        if i + 1 < len(a):
            return a[i + 1]
    return default


def has_flag(flag):
    return flag in argv()


def join_meshes():
    meshes = [o for o in bpy.context.scene.objects if o.type == "MESH"]
    if not meshes:
        raise RuntimeError("No meshes found in the imported GLB")
    bpy.ops.object.select_all(action="DESELECT")
    for o in meshes:
        o.select_set(True)
    bpy.context.view_layer.objects.active = meshes[0]
    bpy.ops.object.join()
    obj = bpy.context.view_layer.objects.active
    bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)
    return obj


def normalize_model(obj, rotation_deg):
    """Center the mesh, scale its largest dimension to 4 units, then rotate."""
    mn = Vector((1e9, 1e9, 1e9))
    mx = Vector((-1e9, -1e9, -1e9))
    for corner in obj.bound_box:
        for i in range(3):
            mn[i] = min(mn[i], corner[i])
            mx[i] = max(mx[i], corner[i])
    center = (mn + mx) * 0.5
    obj.data.transform(Matrix.Translation(-center))

    size = max(mx[i] - mn[i] for i in range(3))
    if size <= 0.0:
        raise RuntimeError("Imported mesh has zero size")
    obj.data.transform(Matrix.Scale(4.0 / size, 4))

    rx, ry, rz = (math.radians(v) for v in rotation_deg)
    if rx or ry or rz:
        rot = (Matrix.Rotation(rz, 4, "Z") @
               Matrix.Rotation(ry, 4, "Y") @
               Matrix.Rotation(rx, 4, "X"))
        obj.data.transform(rot)

        # Rotation can move the visual center; recenter once more.
        mn = Vector((1e9, 1e9, 1e9))
        mx = Vector((-1e9, -1e9, -1e9))
        for v in obj.data.vertices:
            for i in range(3):
                mn[i] = min(mn[i], v.co[i])
                mx[i] = max(mx[i], v.co[i])
        center = (mn + mx) * 0.5
        obj.data.transform(Matrix.Translation(-center))

    obj.data.update()


def setup_world(scene, use_starmap):
    world = bpy.data.worlds.new("NasaSpace")
    scene.world = world
    world.use_nodes = True
    nt = world.node_tree
    bg = nt.nodes.get("Background")
    if bg is None:
        bg = nt.nodes.new("ShaderNodeBackground")
        out = nt.nodes.get("World Output")
        nt.links.new(bg.outputs["Background"], out.inputs["Surface"])

    if use_starmap and os.path.isfile(STARMAP):
        env = nt.nodes.new("ShaderNodeTexEnvironment")
        env.image = bpy.data.images.load(STARMAP, check_existing=True)
        nt.links.new(env.outputs["Color"], bg.inputs["Color"])
        bg.inputs["Strength"].default_value = 0.38

        # A faint blue ambient keeps the unlit side of the spacecraft readable
        # and stops the all-sky map from looking muddy behind the model.
        ambient = nt.nodes.new("ShaderNodeBackground")
        ambient.inputs["Color"].default_value = (0.012, 0.018, 0.035, 1.0)
        ambient.inputs["Strength"].default_value = 0.30
        add = nt.nodes.new("ShaderNodeAddShader")
        out = nt.nodes.get("World Output")
        nt.links.new(bg.outputs["Background"], add.inputs[0])
        nt.links.new(ambient.outputs["Background"], add.inputs[1])
        nt.links.new(add.outputs["Shader"], out.inputs["Surface"])
    else:
        bg.inputs["Color"].default_value = (0.008, 0.010, 0.018, 1.0)
        bg.inputs["Strength"].default_value = 0.55


def add_sun(scene, name, energy, color, direction):
    data = bpy.data.lights.new(name, "SUN")
    data.energy = energy
    data.color = color
    obj = bpy.data.objects.new(name, data)
    obj.rotation_euler = Vector(direction).normalized().to_track_quat(
        "-Z", "Y").to_euler()
    scene.collection.objects.link(obj)


def add_area(scene, name, energy, size, color, location):
    data = bpy.data.lights.new(name, "AREA")
    data.energy = energy
    data.shape = "DISK"
    data.size = size
    data.color = color
    obj = bpy.data.objects.new(name, data)
    obj.location = location
    obj.rotation_euler = (Vector((0.0, 0.0, 0.0)) - obj.location).to_track_quat(
        "-Z", "Y").to_euler()
    scene.collection.objects.link(obj)


def setup_lights(scene):
    add_sun(scene, "KeySun", 3.2, (1.0, 0.97, 0.90), (-0.9, 1.1, -0.8))
    add_sun(scene, "RimFill", 2.6, (0.40, 0.70, 1.0), (1.0, -0.6, 0.8))
    add_area(scene, "SoftFill", 900.0, 10.0, (0.75, 0.84, 1.0), (4.0, -5.0, 3.0))
    add_area(scene, "FrontFill", 300.0, 7.0, (0.90, 0.95, 1.0), (5.0, -6.0, 2.0))


def setup_camera(scene, preset):
    target = Vector(preset["target"])
    az = math.radians(preset["az"])
    el = math.radians(preset["el"])
    dist = preset["dist"]
    offset = Vector((
        dist * math.cos(az) * math.cos(el),
        dist * math.sin(az) * math.cos(el),
        dist * math.sin(el),
    ))

    data = bpy.data.cameras.new("ShowcaseCam")
    data.lens = 50.0
    cam = bpy.data.objects.new("ShowcaseCam", data)
    cam.location = target + offset
    cam.rotation_euler = (target - cam.location).to_track_quat(
        "-Z", "Y").to_euler()
    scene.collection.objects.link(cam)
    scene.camera = cam


def setup_render(scene, quick):
    scene.render.engine = "CYCLES"
    scene.cycles.device = "CPU"
    scene.cycles.samples = 16 if quick else 64
    scene.cycles.use_denoising = True
    scene.render.resolution_x = 640 if quick else 1280
    scene.render.resolution_y = 360 if quick else 720
    scene.render.resolution_percentage = 100
    scene.render.film_transparent = False
    scene.render.image_settings.file_format = "PNG"
    scene.render.image_settings.color_mode = "RGB"
    scene.view_settings.exposure = 0.35


def render_preset(name, quick=False, out_override=None, use_starmap=True):
    preset = PRESETS[name]
    glb_path = os.path.join(SRC_DIR, preset["glb"])
    if not os.path.isfile(glb_path):
        raise FileNotFoundError(
            f"{glb_path}\nRun tools/fetch_nasa_assets.ps1 first.")

    bpy.ops.wm.read_factory_settings(use_empty=True)
    scene = bpy.context.scene
    bpy.ops.import_scene.gltf(filepath=glb_path)
    obj = join_meshes()
    obj.name = f"NASA_{name}"
    normalize_model(obj, preset["rot"])

    setup_world(scene, use_starmap)
    setup_lights(scene)
    setup_camera(scene, preset)
    setup_render(scene, quick)

    if out_override:
        out_png = os.path.abspath(out_override)
    elif quick:
        out_png = os.path.join(OUT_QUICK, f"{name}_preview.png")
    else:
        out_png = os.path.join(OUT_FINAL, preset["out"])

    os.makedirs(os.path.dirname(out_png), exist_ok=True)
    scene.render.filepath = out_png
    print(f"[{name}] {glb_path} -> {out_png} "
          f"({'quick' if quick else 'final'})")
    bpy.ops.render.render(write_still=True)

    if not quick and not out_override:
        doc_dest = os.path.join(DOCS_SCREENS, preset["out"])
        shutil.copyfile(out_png, doc_dest)
        print(f"[{name}] copied to {doc_dest}")


def main():
    quick = has_flag("--quick")
    use_starmap = not has_flag("--no-starmap")
    out_override = arg_value("--out")

    if has_flag("--all"):
        names = list(PRESETS.keys())
    else:
        name = arg_value("--model", "voyager")
        if name not in PRESETS:
            raise SystemExit(
                f"Unknown model '{name}'. Choose: {', '.join(PRESETS)}")
        names = [name]

    if out_override and len(names) > 1:
        raise SystemExit("--out requires a single --model, not --all")

    for name in names:
        render_preset(name, quick=quick, out_override=out_override,
                      use_starmap=use_starmap)
    print("NASA showcase render complete.")


if __name__ == "__main__":
    main()
