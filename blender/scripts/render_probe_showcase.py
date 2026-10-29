"""Render the Deep Space Science & Relay Probe showcase (Cycles CPU).

Uses res/models/probe.obj and its 5-channel baked BMP atlas:
  probe_albedo.bmp, probe_rough.bmp, probe_metal.bmp,
  probe_normal.bmp, probe_emissive.bmp
With Gaia DR2 Deep Star Maps 2020 background (starmap_2020_4k_gal.exr).

Usage (headless):
  blender -b -P blender/scripts/render_probe_showcase.py
  blender -b -P blender/scripts/render_probe_showcase.py -- --quick
  blender -b -P blender/scripts/render_probe_showcase.py -- --out path.png

Writes:
  blender/out/final_showcase/probe_showcase.png
  docs/screens/probe_showcase.png
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
STARMAP = os.path.join(REPO_ROOT, "res", "textures", "starmap_2020_4k_gal.exr")

OBJ_PATH = os.path.join(RES_MODELS, "probe.obj")


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


def build_probe_material():
    """Principled BSDF wired to the 5 baked BMP maps of the probe atlas."""
    mat = bpy.data.materials.new("ProbeBaked")
    nt = mat.node_tree
    bsdf = nt.nodes.get("Principled BSDF")

    albedo = nt.nodes.new("ShaderNodeTexImage")
    albedo.name = "Albedo"
    albedo.image = load_texture("probe_albedo.bmp")
    nt.links.new(albedo.outputs["Color"], bsdf.inputs["Base Color"])

    rough = nt.nodes.new("ShaderNodeTexImage")
    rough.name = "Roughness"
    rough.image = load_texture("probe_rough.bmp", non_color=True)
    nt.links.new(rough.outputs["Color"], bsdf.inputs["Roughness"])

    metal = nt.nodes.new("ShaderNodeTexImage")
    metal.name = "Metallic"
    metal.image = load_texture("probe_metal.bmp", non_color=True)
    nt.links.new(metal.outputs["Color"], bsdf.inputs["Metallic"])

    normal_tex = nt.nodes.new("ShaderNodeTexImage")
    normal_tex.name = "Normal"
    normal_tex.image = load_texture("probe_normal.bmp", non_color=True)
    normal_map = nt.nodes.new("ShaderNodeNormalMap")
    nt.links.new(normal_tex.outputs["Color"], normal_map.inputs["Color"])
    nt.links.new(normal_map.outputs["Normal"], bsdf.inputs["Normal"])

    emissive = nt.nodes.new("ShaderNodeTexImage")
    emissive.name = "Emissive"
    emissive.image = load_texture("probe_emissive.bmp")
    if "Emission Color" in bsdf.inputs:
        nt.links.new(emissive.outputs["Color"], bsdf.inputs["Emission Color"])
    else:
        nt.links.new(emissive.outputs["Color"], bsdf.inputs["Emission"])
    if "Emission Strength" in bsdf.inputs:
        bsdf.inputs["Emission Strength"].default_value = 8.0
    return mat


def setup_render(scene, quick):
    scene.render.engine = "CYCLES"
    scene.cycles.device = "CPU"
    scene.render.resolution_x = 640 if quick else 1280
    scene.render.resolution_y = 360 if quick else 720
    scene.render.resolution_percentage = 100
    scene.render.film_transparent = False
    scene.cycles.samples = 16 if quick else 64
    scene.cycles.use_denoising = True
    scene.view_settings.exposure = 0.85

    world = bpy.data.worlds.new("ProbeWorld")
    scene.world = world
    nt = world.node_tree
    bg = nt.nodes.get("Background")
    if bg is None:
        bg = nt.nodes.new("ShaderNodeBackground")
        out = nt.nodes.get("World Output")
        nt.links.new(bg.outputs["Background"], out.inputs["Surface"])

    if os.path.isfile(STARMAP):
        env = nt.nodes.new("ShaderNodeTexEnvironment")
        env.image = bpy.data.images.load(STARMAP, check_existing=True)
        nt.links.new(env.outputs["Color"], bg.inputs["Color"])
        bg.inputs["Strength"].default_value = 0.45

        ambient = nt.nodes.new("ShaderNodeBackground")
        ambient.inputs["Color"].default_value = (0.015, 0.022, 0.040, 1.0)
        ambient.inputs["Strength"].default_value = 0.35
        add = nt.nodes.new("ShaderNodeAddShader")
        out = nt.nodes.get("World Output")
        nt.links.new(bg.outputs["Background"], add.inputs[0])
        nt.links.new(ambient.outputs["Background"], add.inputs[1])
        nt.links.new(add.outputs["Shader"], out.inputs["Surface"])
    else:
        bg.inputs["Color"].default_value = (0.008, 0.010, 0.020, 1.0)
        bg.inputs["Strength"].default_value = 0.5


def add_camera_and_lights(scene):
    cam_data = bpy.data.cameras.new("ShowcaseCam")
    cam_data.lens = 45
    cam = bpy.data.objects.new("ShowcaseCam", cam_data)
    cam.location = (2.2, -2.4, 1.35)
    direction = Vector((0.0, 0.0, 0.05)) - cam.location
    cam.rotation_euler = direction.to_track_quat("-Z", "Y").to_euler()
    scene.collection.objects.link(cam)
    scene.camera = cam

    # Main solar key light catching dish & solar panel face
    key_data = bpy.data.lights.new("KeySun", type="SUN")
    key_data.energy = 8.5
    key_data.color = (1.0, 0.98, 0.92)
    key = bpy.data.objects.new("KeySun", key_data)
    key.rotation_euler = Vector((-1.1, 0.8, -0.9)).normalized().to_track_quat(
        "-Z", "Y").to_euler()
    scene.collection.objects.link(key)

    # Blue rim light highlighting foil bus & silhouette
    rim_data = bpy.data.lights.new("RimFill", type="SUN")
    rim_data.energy = 4.5
    rim_data.color = (0.35, 0.70, 1.0)
    rim = bpy.data.objects.new("RimFill", rim_data)
    rim.rotation_euler = Vector((1.0, -1.2, 0.6)).normalized().to_track_quat(
        "-Z", "Y").to_euler()
    scene.collection.objects.link(rim)

    # Soft camera fill so dark metallic silicon facets are visible
    fill_data = bpy.data.lights.new("SoftFill", type="AREA")
    fill_data.energy = 1100.0
    fill_data.shape = "DISK"
    fill_data.size = 6.0
    fill_data.color = (0.75, 0.85, 1.0)
    fill = bpy.data.objects.new("SoftFill", fill_data)
    fill.location = (2.2, -2.6, 1.6)
    fill.rotation_euler = (Vector((0.0, 0.0, 0.05)) - fill.location).to_track_quat(
        "-Z", "Y").to_euler()
    scene.collection.objects.link(fill)


def main():
    if not os.path.isfile(OBJ_PATH):
        raise FileNotFoundError(OBJ_PATH)

    bpy.ops.wm.read_factory_settings(use_empty=True)
    scene = bpy.context.scene
    quick = argv_flag("--quick")

    bpy.ops.wm.obj_import(filepath=OBJ_PATH, forward_axis="Y", up_axis="Z")
    probe = next(o for o in bpy.context.selected_objects if o.type == "MESH")
    probe.name = "DeepSpaceProbe"

    if not argv_flag("--no-flip-v"):
        uv_layer = probe.data.uv_layers.active
        if uv_layer is not None:
            for loop_uv in uv_layer.data:
                loop_uv.uv.y = 1.0 - loop_uv.uv.y

    probe.data.materials.clear()
    probe.data.materials.append(build_probe_material())
    for poly in probe.data.polygons:
        poly.material_index = 0
        poly.use_smooth = True

    # Presentation angle showcasing gold foil, dish, solar arrays front (+Y) and sensor boom
    probe.rotation_euler = (math.radians(35), math.radians(-25), math.radians(155))
    bpy.context.view_layer.update()

    setup_render(scene, quick)
    add_camera_and_lights(scene)

    out_png = argv_value(
        "--out", os.path.join(OUT_SHOWCASE, "probe_showcase.png"))
    os.makedirs(os.path.dirname(out_png), exist_ok=True)
    scene.render.filepath = out_png
    print(f"Rendering {OBJ_PATH} -> {out_png} "
          f"({'quick' if quick else 'final'})")
    bpy.ops.render.render(write_still=True)

    if not quick:
        doc_dest = os.path.join(DOCS_SCREENS, "probe_showcase.png")
        shutil.copyfile(out_png, doc_dest)
        print(f"Copied showcase to {doc_dest}")
    print("Probe showcase render complete.")


if __name__ == "__main__":
    main()
