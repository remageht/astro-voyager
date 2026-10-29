"""Render the current `res/models/ring_ship.obj` showcase (Cycles CPU).

The Ring Ship OBJ is a hand-tuned, git-ignored asset: its baked PBR atlas
matches the UV layout that was produced by `generate_ring_ship.py` on
2026-10-26. Re-running the generator would re-project UVs and invalidate the
existing BMP set, so this script deliberately renders the *existing* mesh and
atlas instead of regenerating them.

Usage (headless):
  blender -b -P blender/scripts/render_ring_ship_showcase.py
  blender -b -P blender/scripts/render_ring_ship_showcase.py -- --quick
  blender -b -P blender/scripts/render_ring_ship_showcase.py -- --out path.png

Options after `--`: --quick (640x360 preview), --out PATH, --flat (debug
material without the atlas), --no-flip-v (keep raw OBJ UVs; the baked BMP
atlas is top-down, so V is flipped by default).

Writes:
  blender/out/final_showcase/ring_ship_showcase.png
  docs/screens/ring_ship_showcase.png
"""
import math
import os
import shutil
import sys

import bpy
from mathutils import Vector

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
PROJECT_ROOT = os.path.dirname(SCRIPT_DIR)  # blender/
REPO_ROOT = os.path.dirname(PROJECT_ROOT)   # astro-voyager/
RES_MODELS = os.path.join(REPO_ROOT, "res", "models")
OUT_SHOWCASE = os.path.join(PROJECT_ROOT, "out", "final_showcase")
DOCS_SCREENS = os.path.join(REPO_ROOT, "docs", "screens")

OBJ_PATH = os.path.join(RES_MODELS, "ring_ship.obj")


def argv_flag(name):
    return name in sys.argv


def argv_value(name, default):
    if name in sys.argv:
        i = sys.argv.index(name)
        if i + 1 < len(sys.argv):
            return sys.argv[i + 1]
    return default


def load_texture(name, non_color=False):
    path = os.path.join(RES_MODELS, name)
    if not os.path.isfile(path):
        raise FileNotFoundError(path)
    img = bpy.data.images.load(path, check_existing=True)
    if non_color:
        img.colorspace_settings.name = "Non-Color"
    return img


def build_flat_material():
    """Debug material: no atlas, just a neutral PBR surface."""
    mat = bpy.data.materials.new("RingShipFlat")
    mat.use_nodes = True
    bsdf = mat.node_tree.nodes.get("Principled BSDF")
    bsdf.inputs["Base Color"].default_value = (0.72, 0.78, 0.88, 1.0)
    bsdf.inputs["Metallic"].default_value = 0.55
    bsdf.inputs["Roughness"].default_value = 0.32
    return mat


def build_baked_material():
    """Principled BSDF wired to the five baked BMP maps of the ship atlas."""
    mat = bpy.data.materials.new("RingShipBaked")
    mat.use_nodes = True
    nt = mat.node_tree
    bsdf = nt.nodes.get("Principled BSDF")

    albedo = nt.nodes.new("ShaderNodeTexImage")
    albedo.name = "Albedo"
    albedo.image = load_texture("ring_ship_albedo.bmp")
    nt.links.new(albedo.outputs["Color"], bsdf.inputs["Base Color"])

    rough = nt.nodes.new("ShaderNodeTexImage")
    rough.name = "Roughness"
    rough.image = load_texture("ring_ship_rough.bmp", non_color=True)
    nt.links.new(rough.outputs["Color"], bsdf.inputs["Roughness"])

    metal = nt.nodes.new("ShaderNodeTexImage")
    metal.name = "Metallic"
    metal.image = load_texture("ring_ship_metal.bmp", non_color=True)
    nt.links.new(metal.outputs["Color"], bsdf.inputs["Metallic"])

    normal_tex = nt.nodes.new("ShaderNodeTexImage")
    normal_tex.name = "Normal"
    normal_tex.image = load_texture("ring_ship_normal.bmp", non_color=True)
    normal_map = nt.nodes.new("ShaderNodeNormalMap")
    nt.links.new(normal_tex.outputs["Color"], normal_map.inputs["Color"])
    nt.links.new(normal_map.outputs["Normal"], bsdf.inputs["Normal"])

    emissive = nt.nodes.new("ShaderNodeTexImage")
    emissive.name = "Emissive"
    emissive.image = load_texture("ring_ship_emissive.bmp")
    if "Emission Color" in bsdf.inputs:
        nt.links.new(emissive.outputs["Color"], bsdf.inputs["Emission Color"])
    else:  # Blender 3.x name
        nt.links.new(emissive.outputs["Color"], bsdf.inputs["Emission"])
    if "Emission Strength" in bsdf.inputs:
        # The generator bakes a clamped 0..1 emissive atlas but shades the
        # emitter at strength 4.5; mirror that here or the plasma band goes
        # dull grey in the showcase.
        bsdf.inputs["Emission Strength"].default_value = 7.0
    return mat


def setup_render(scene, quick):
    scene.render.engine = "CYCLES"
    scene.cycles.device = "CPU"
    scene.render.resolution_x = 640 if quick else 1280
    scene.render.resolution_y = 360 if quick else 720
    scene.render.resolution_percentage = 100
    scene.render.film_transparent = False
    scene.cycles.samples = 12 if quick else 64
    scene.cycles.use_denoising = True
    scene.view_settings.exposure = 0.6

    # Deep space background, same values as generate_ring_ship.py.
    world = scene.world
    if world is None:
        world = bpy.data.worlds.new("SpaceWorld")
        scene.world = world
    world.use_nodes = True
    bg = world.node_tree.nodes.get("Background")
    if bg:
        bg.inputs["Color"].default_value = (0.010, 0.012, 0.022, 1.0)
        bg.inputs["Strength"].default_value = 0.55


def add_camera_and_lights(scene):
    cam_data = bpy.data.cameras.new("ShowcaseCam")
    cam_data.lens = 42
    cam = bpy.data.objects.new("ShowcaseCam", cam_data)
    cam.location = (2.2, -2.5, 1.4)
    direction = Vector((0.0, 0.0, 0.0)) - cam.location
    cam.rotation_euler = direction.to_track_quat("-Z", "Y").to_euler()
    scene.collection.objects.link(cam)
    scene.camera = cam

    key_data = bpy.data.lights.new("KeySun", type="SUN")
    key_data.energy = 7.5
    key_data.color = (1.0, 0.98, 0.92)
    key = bpy.data.objects.new("KeySun", key_data)
    key.rotation_euler = Vector((-0.9, 1.1, -0.8)).normalized().to_track_quat(
        "-Z", "Y").to_euler()
    scene.collection.objects.link(key)

    rim_data = bpy.data.lights.new("RimFill", type="SUN")
    rim_data.energy = 4.5
    rim_data.color = (0.35, 0.70, 1.0)
    rim = bpy.data.objects.new("RimFill", rim_data)
    rim.rotation_euler = Vector((1.0, -0.8, 1.0)).normalized().to_track_quat(
        "-Z", "Y").to_euler()
    scene.collection.objects.link(rim)

    # Soft camera-side fill so the baked metal atlas does not read as black
    # patches against the deep-space background.
    fill_data = bpy.data.lights.new("SoftFill", type="AREA")
    fill_data.energy = 1200.0
    fill_data.shape = "DISK"
    fill_data.size = 7.0
    fill_data.color = (0.78, 0.86, 1.0)
    fill = bpy.data.objects.new("SoftFill", fill_data)
    fill.location = (3.2, -4.0, 2.6)
    fill.rotation_euler = (Vector((0.0, 0.0, 0.0)) - fill.location).to_track_quat(
        "-Z", "Y").to_euler()
    scene.collection.objects.link(fill)


def main():
    if not os.path.isfile(OBJ_PATH):
        raise FileNotFoundError(OBJ_PATH)

    bpy.ops.wm.read_factory_settings(use_empty=True)
    scene = bpy.context.scene
    quick = argv_flag("--quick")

    # Round-trip the same axis convention the generator used for export.
    bpy.ops.wm.obj_import(filepath=OBJ_PATH, forward_axis="Y", up_axis="Z")
    ship = next(o for o in bpy.context.selected_objects if o.type == "MESH")
    ship.name = "RingShip"

    # The game's BMP atlas is stored top-down while Blender's image origin is
    # bottom-left; flip V so the baked islands land where the OBJ UVs expect.
    if not argv_flag("--no-flip-v"):
        uv_layer = ship.data.uv_layers.active
        if uv_layer is not None:
            for loop_uv in uv_layer.data:
                loop_uv.uv.y = 1.0 - loop_uv.uv.y

    # The OBJ has no material slots: one baked atlas covers every face.
    ship.data.materials.clear()
    ship.data.materials.append(
        build_flat_material() if argv_flag("--flat") else build_baked_material())
    for poly in ship.data.polygons:
        poly.material_index = 0
        poly.use_smooth = True

    # Same cinematic 3/4 tilt as the procedural generator showcase.
    ship.rotation_euler = (math.radians(-72), math.radians(18), math.radians(35))
    bpy.context.view_layer.update()

    setup_render(scene, quick)
    add_camera_and_lights(scene)

    out_png = argv_value(
        "--out", os.path.join(OUT_SHOWCASE, "ring_ship_showcase.png"))
    os.makedirs(os.path.dirname(out_png), exist_ok=True)
    scene.render.filepath = out_png
    print(f"Rendering {OBJ_PATH} -> {out_png} "
          f"({'quick' if quick else 'final'})")
    bpy.ops.render.render(write_still=True)

    if not quick:
        doc_dest = os.path.join(DOCS_SCREENS, "ring_ship_showcase.png")
        shutil.copyfile(out_png, doc_dest)
        print(f"Copied showcase to {doc_dest}")
    print("Ring Ship showcase render complete.")


if __name__ == "__main__":
    main()
