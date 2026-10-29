"""Render a dramatic high-detail hero showcase of NASA Parker Solar Probe."""
import math
import os
import bpy
from mathutils import Vector

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
PROJECT_ROOT = os.path.dirname(SCRIPT_DIR)
REPO_ROOT = os.path.dirname(PROJECT_ROOT)
SRC_GLB = os.path.join(PROJECT_ROOT, "out", "nasa_src", "parker.glb")
STARMAP = os.path.join(REPO_ROOT, "res", "textures", "starmap_2020_4k_gal.exr")
OUT_PNG = os.path.join(REPO_ROOT, "docs", "screens", "parker_hero.png")

def main():
    bpy.ops.wm.read_factory_settings(use_empty=True)
    scene = bpy.context.scene

    # Import Parker GLB
    bpy.ops.import_scene.gltf(filepath=SRC_GLB)

    # Remove extra objects
    for name in ("Cube", "Camera", "Light", "_root"):
        o = scene.objects.get(name)
        if o:
            bpy.data.objects.remove(o, do_unlink=True)

    # Smooth shading
    for o in scene.objects:
        if o.type == "MESH":
            for poly in o.data.polygons:
                poly.use_smooth = True

    # PBR Material adjustments for realistic space metals
    for mat in bpy.data.materials:
        if not mat.node_tree:
            continue
        bsdf = mat.node_tree.nodes.get("Principled BSDF")
        if not bsdf:
            continue
        if "foil" in mat.name.lower():
            if "Roughness" in bsdf.inputs:
                bsdf.inputs["Roughness"].default_value = 0.22
            if "Metallic" in bsdf.inputs:
                bsdf.inputs["Metallic"].default_value = 0.92
        elif "black" in mat.name.lower():
            if "Roughness" in bsdf.inputs:
                bsdf.inputs["Roughness"].default_value = 0.70
            if "Metallic" in bsdf.inputs:
                bsdf.inputs["Metallic"].default_value = 0.10
        elif "metal" in mat.name.lower():
            if "Roughness" in bsdf.inputs:
                bsdf.inputs["Roughness"].default_value = 0.30
            if "Metallic" in bsdf.inputs:
                bsdf.inputs["Metallic"].default_value = 0.85

    # Render settings: Cycles CPU, Gaia Star Background
    scene.render.engine = "CYCLES"
    scene.cycles.device = "CPU"
    scene.render.resolution_x = 1280
    scene.render.resolution_y = 720
    scene.cycles.samples = 64
    scene.cycles.use_denoising = True
    scene.view_settings.exposure = 0.90

    world = bpy.data.worlds.new("ParkerWorld")
    scene.world = world
    world.use_nodes = True
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
        bg.inputs["Strength"].default_value = 0.40
        ambient = nt.nodes.new("ShaderNodeBackground")
        ambient.inputs["Color"].default_value = (0.025, 0.015, 0.010, 1.0) # subtle warm solar corona tint
        ambient.inputs["Strength"].default_value = 0.40
        add = nt.nodes.new("ShaderNodeAddShader")
        out = nt.nodes.get("World Output")
        nt.links.new(bg.outputs["Background"], add.inputs[0])
        nt.links.new(ambient.outputs["Background"], add.inputs[1])
        nt.links.new(add.outputs["Shader"], out.inputs["Surface"])

    # Hero camera: 3/4 perspective viewing heat shield, instruments, cooling radiators and solar array
    cam_data = bpy.data.cameras.new("HeroCam")
    cam_data.lens = 45.0
    cam = bpy.data.objects.new("HeroCam", cam_data)
    cam.location = (4.2, -4.8, 2.6)
    target = Vector((0.2, 0.0, 0.4))
    cam.rotation_euler = (target - cam.location).to_track_quat("-Z", "Y").to_euler()
    scene.collection.objects.link(cam)
    scene.camera = cam

    # Solar Key Light (intense sunlight illuminating the carbon-composite heat shield & bus edge)
    key_data = bpy.data.lights.new("SunKey", type="SUN")
    key_data.energy = 7.0
    key_data.color = (1.0, 0.96, 0.88)
    key = bpy.data.objects.new("SunKey", key_data)
    key.rotation_euler = Vector((-1.2, 0.9, -0.7)).normalized().to_track_quat("-Z", "Y").to_euler()
    scene.collection.objects.link(key)

    # Corona Backlight Rim
    rim_data = bpy.data.lights.new("Rim", type="SUN")
    rim_data.energy = 4.0
    rim_data.color = (1.0, 0.65, 0.30)
    rim = bpy.data.objects.new("Rim", rim_data)
    rim.rotation_euler = Vector((0.8, -1.1, 0.6)).normalized().to_track_quat("-Z", "Y").to_euler()
    scene.collection.objects.link(rim)

    # Soft Fill Light
    fill_data = bpy.data.lights.new("Fill", type="AREA")
    fill_data.energy = 850.0
    fill_data.size = 10.0
    fill_data.color = (0.78, 0.86, 1.0)
    fill = bpy.data.objects.new("Fill", fill_data)
    fill.location = (4.5, -5.5, 3.5)
    fill.rotation_euler = (target - fill.location).to_track_quat("-Z", "Y").to_euler()
    scene.collection.objects.link(fill)

    os.makedirs(os.path.dirname(OUT_PNG), exist_ok=True)
    scene.render.filepath = OUT_PNG
    print(f"Rendering Parker hero close-up to {OUT_PNG}...")
    bpy.ops.render.render(write_still=True)
    print("Done!")

if __name__ == "__main__":
    main()
