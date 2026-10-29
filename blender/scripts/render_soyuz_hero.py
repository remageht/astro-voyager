"""Render a cinematic showcase of Soviet/Russian Soyuz-MS spacecraft (Cycles)."""
import math
import os
import bpy
from mathutils import Matrix, Vector

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
PROJECT_ROOT = os.path.dirname(SCRIPT_DIR)
REPO_ROOT = os.path.dirname(PROJECT_ROOT)
SRC_GLTF = os.path.join(PROJECT_ROOT, "out", "mir_src", "scene.gltf")
STARMAP = os.path.join(REPO_ROOT, "res", "textures", "starmap_2020_4k_gal.exr")
OUT_PNG = os.path.join(REPO_ROOT, "docs", "screens", "soyuz_hero.png")

def main():
    bpy.ops.wm.read_factory_settings(use_empty=True)
    scene = bpy.context.scene

    # Import Mir & Soyuz glTF
    bpy.ops.import_scene.gltf(filepath=SRC_GLTF)

    # Keep only Soyuz parts, delete Mir, Progress and empty helpers
    soyuz_mesh_names = {"Soyuz.001_Soyuz_color_0", "Soyuz.001_Soyuz_black_Isolation_0"}
    for o in list(scene.objects):
        if o.type == "MESH" and o.name not in soyuz_mesh_names:
            bpy.data.objects.remove(o, do_unlink=True)
        elif o.type == "EMPTY" and o.name not in ("Soyuz.001", "RootNode"):
            bpy.data.objects.remove(o, do_unlink=True)

    soyuz_meshes = [o for o in scene.objects if o.name in soyuz_mesh_names]

    # Smooth shading & PBR materials setup
    for o in soyuz_meshes:
        for poly in o.data.polygons:
            poly.use_smooth = True

    # Render settings: Cycles CPU, Gaia Star Background
    scene.render.engine = "CYCLES"
    scene.cycles.device = "CPU"
    scene.render.resolution_x = 1280
    scene.render.resolution_y = 720
    scene.cycles.samples = 64
    scene.cycles.use_denoising = True
    scene.view_settings.exposure = 0.85

    world = bpy.data.worlds.new("SoyuzWorld")
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

    # Calculate bounding box & center
    mn = Vector((1e9, 1e9, 1e9))
    mx = Vector((-1e9, -1e9, -1e9))
    for o in soyuz_meshes:
        for c in o.bound_box:
            w = o.matrix_world @ Vector(c)
            for i in range(3):
                mn[i] = min(mn[i], w[i])
                mx[i] = max(mx[i], w[i])
    center = (mn + mx) * 0.5
    size = max(mx[i] - mn[i] for i in range(3))

    # Center all meshes
    for o in soyuz_meshes:
        o.location -= center

    # Hero camera: close-up 3/4 orbital angle displaying descent module, orbital module, and solar wings
    cam_data = bpy.data.cameras.new("HeroCam")
    cam_data.lens = 45.0
    cam = bpy.data.objects.new("HeroCam", cam_data)
    cam.location = (size * 0.72, -size * 1.05, size * 0.45)
    target = Vector((0.0, 0.0, 0.05))
    cam.rotation_euler = (target - cam.location).to_track_quat("-Z", "Y").to_euler()
    scene.collection.objects.link(cam)
    scene.camera = cam

    # Directional Key Sunlight catching thermal blankets and green orbital module
    key_data = bpy.data.lights.new("SunKey", type="SUN")
    key_data.energy = 6.0
    key_data.color = (1.0, 0.98, 0.92)
    key = bpy.data.objects.new("SunKey", key_data)
    key.rotation_euler = Vector((-1.1, 1.2, -0.6)).normalized().to_track_quat("-Z", "Y").to_euler()
    scene.collection.objects.link(key)

    # Blue Earth-reflected Rim Light highlighting solar panels
    rim_data = bpy.data.lights.new("Rim", type="SUN")
    rim_data.energy = 3.8
    rim_data.color = (0.35, 0.70, 1.0)
    rim = bpy.data.objects.new("Rim", rim_data)
    rim.rotation_euler = Vector((1.0, -1.0, 0.8)).normalized().to_track_quat("-Z", "Y").to_euler()
    scene.collection.objects.link(rim)

    # Soft Fill Light
    fill_data = bpy.data.lights.new("Fill", type="AREA")
    fill_data.energy = 800.0
    fill_data.size = size * 2.5
    fill_data.color = (0.75, 0.85, 1.0)
    fill = bpy.data.objects.new("Fill", fill_data)
    fill.location = (size * 1.5, -size * 1.8, size * 1.2)
    fill.rotation_euler = (target - fill.location).to_track_quat("-Z", "Y").to_euler()
    scene.collection.objects.link(fill)

    os.makedirs(os.path.dirname(OUT_PNG), exist_ok=True)
    scene.render.filepath = OUT_PNG
    print(f"Rendering Soyuz-MS showcase to {OUT_PNG}...")
    bpy.ops.render.render(write_still=True)
    print("Done!")

if __name__ == "__main__":
    main()
