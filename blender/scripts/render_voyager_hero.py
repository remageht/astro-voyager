"""Render a cinematic high-detail showcase of the NASA Voyager Spacecraft."""
import math
import os
import bpy
from mathutils import Vector

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
PROJECT_ROOT = os.path.dirname(SCRIPT_DIR)
REPO_ROOT = os.path.dirname(PROJECT_ROOT)
SRC_GLB = os.path.join(PROJECT_ROOT, "out", "nasa_src", "voyager_b.glb")
STARMAP = os.path.join(REPO_ROOT, "res", "textures", "starmap_2020_4k_gal.exr")
OUT_PNG = os.path.join(REPO_ROOT, "docs", "screens", "voyager_hero.png")

def main():
    bpy.ops.wm.read_factory_settings(use_empty=True)
    scene = bpy.context.scene

    # Import Voyager GLB
    bpy.ops.import_scene.gltf(filepath=SRC_GLB)

    # Remove unwanted helper objects and dummy cube
    for name in ("Cube", "Camera", "Light", "_root"):
        o = scene.objects.get(name)
        if o:
            bpy.data.objects.remove(o, do_unlink=True)

    # Ensure smooth shading on all parts
    for o in scene.objects:
        if o.type == "MESH":
            for poly in o.data.polygons:
                poly.use_smooth = True

    # Enhance Principled BSDF on materials for realism
    for mat in bpy.data.materials:
        if not mat.node_tree:
            continue
        bsdf = mat.node_tree.nodes.get("Principled BSDF")
        if bsdf:
            # Subtle metallic and crisp roughness for NASA thermal coatings
            if mat.name == "tex_01":
                if "Roughness" in bsdf.inputs:
                    bsdf.inputs["Roughness"].default_value = 0.35
                if "Metallic" in bsdf.inputs:
                    bsdf.inputs["Metallic"].default_value = 0.65
            elif "AO" in mat.name:
                if "Roughness" in bsdf.inputs:
                    bsdf.inputs["Roughness"].default_value = 0.45
                if "Metallic" in bsdf.inputs:
                    bsdf.inputs["Metallic"].default_value = 0.25

    # Render settings: Cycles CPU, Gaia Star Background
    scene.render.engine = "CYCLES"
    scene.cycles.device = "CPU"
    scene.render.resolution_x = 1280
    scene.render.resolution_y = 720
    scene.cycles.samples = 64
    scene.cycles.use_denoising = True
    scene.view_settings.exposure = 0.85

    world = bpy.data.worlds.new("VoyagerWorld")
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
        bg.inputs["Strength"].default_value = 0.45
        ambient = nt.nodes.new("ShaderNodeBackground")
        ambient.inputs["Color"].default_value = (0.015, 0.022, 0.045, 1.0)
        ambient.inputs["Strength"].default_value = 0.35
        add = nt.nodes.new("ShaderNodeAddShader")
        out = nt.nodes.get("World Output")
        nt.links.new(bg.outputs["Background"], add.inputs[0])
        nt.links.new(ambient.outputs["Background"], add.inputs[1])
        nt.links.new(add.outputs["Shader"], out.inputs["Surface"])

    # Hero camera framing: 3/4 perspective focusing on the main bus, dish and instruments
    cam_data = bpy.data.cameras.new("HeroCam")
    cam_data.lens = 48.0
    cam = bpy.data.objects.new("HeroCam", cam_data)
    cam.location = (8.5, -9.5, 4.5)
    target = Vector((0.0, 0.5, 0.8))
    cam.rotation_euler = (target - cam.location).to_track_quat("-Z", "Y").to_euler()
    scene.collection.objects.link(cam)
    scene.camera = cam

    # Directional Key Sunlight catching the dish & Golden Record
    key_data = bpy.data.lights.new("SunKey", type="SUN")
    key_data.energy = 5.0
    key_data.color = (1.0, 0.98, 0.92)
    key = bpy.data.objects.new("SunKey", key_data)
    key.rotation_euler = Vector((-1.0, 1.1, -0.6)).normalized().to_track_quat("-Z", "Y").to_euler()
    scene.collection.objects.link(key)

    # Blue Rim Light
    rim_data = bpy.data.lights.new("Rim", type="SUN")
    rim_data.energy = 3.2
    rim_data.color = (0.35, 0.70, 1.0)
    rim = bpy.data.objects.new("Rim", rim_data)
    rim.rotation_euler = Vector((1.0, -1.0, 0.8)).normalized().to_track_quat("-Z", "Y").to_euler()
    scene.collection.objects.link(rim)

    # Soft Fill Light
    fill_data = bpy.data.lights.new("Fill", type="AREA")
    fill_data.energy = 950.0
    fill_data.size = 12.0
    fill_data.color = (0.75, 0.85, 1.0)
    fill = bpy.data.objects.new("Fill", fill_data)
    fill.location = (9.0, -11.0, 6.0)
    fill.rotation_euler = (target - fill.location).to_track_quat("-Z", "Y").to_euler()
    scene.collection.objects.link(fill)

    os.makedirs(os.path.dirname(OUT_PNG), exist_ok=True)
    scene.render.filepath = OUT_PNG
    print(f"Rendering Voyager hero close-up to {OUT_PNG}...")
    bpy.ops.render.render(write_still=True)
    print("Done!")

if __name__ == "__main__":
    main()
