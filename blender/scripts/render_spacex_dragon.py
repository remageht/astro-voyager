"""Render a cinematic showcase of SpaceX Crew Dragon (Cycles)."""
import math
import os
import bpy
from mathutils import Matrix, Vector

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
PROJECT_ROOT = os.path.dirname(SCRIPT_DIR)
REPO_ROOT = os.path.dirname(PROJECT_ROOT)
SRC_GLTF = os.path.join(PROJECT_ROOT, "out", "spacex_src", "dragon", "scene.gltf")
STARMAP = os.path.join(REPO_ROOT, "res", "textures", "starmap_2020_4k_gal.exr")
OUT_PNG = os.path.join(REPO_ROOT, "docs", "screens", "spacex_dragon_hero.png")

# Hero framing: full capsule + trunk, three-quarter orbital attitude.
CAMERA_LENS = 45.0
CAMERA_DIR = Vector((0.52, -0.72, 0.34)).normalized()
FRAME_MARGIN = 1.0


def flatten_meshes(scene):
    """Bake parent/world transforms into mesh data so bounds are world-space."""
    meshes = [o for o in scene.objects if o.type == "MESH"]
    for o in meshes:
        world = o.matrix_world.copy()
        o.parent = None
        o.data.transform(world)
        o.matrix_world = Matrix.Identity(4)
    bpy.context.view_layer.update()
    return meshes


def bounds(meshes):
    mn = Vector((1e18, 1e18, 1e18))
    mx = Vector((-1e18, -1e18, -1e18))
    for o in meshes:
        for corner in o.bound_box:
            w = o.matrix_world @ Vector(corner)
            for i in range(3):
                mn[i] = min(mn[i], w[i])
                mx[i] = max(mx[i], w[i])
    return mn, mx


def main():
    bpy.ops.wm.read_factory_settings(use_empty=True)
    scene = bpy.context.scene

    # Import Crew Dragon
    bpy.ops.import_scene.gltf(filepath=SRC_GLTF)

    # Remove the ground plane (Mesh_2) and default cube
    for name in ("Cube", "Mesh_2"):
        o = scene.objects.get(name)
        if o:
            bpy.data.objects.remove(o, do_unlink=True)

    # Smooth shading & PBR materials setup
    for o in scene.objects:
        if o.type == "MESH":
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

    world = bpy.data.worlds.new("DragonWorld")
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

    # Bake world transforms, then center the whole spacecraft on the origin.
    meshes = flatten_meshes(scene)
    mn, mx = bounds(meshes)
    center = (mn + mx) * 0.5
    for o in meshes:
        o.data.transform(Matrix.Translation(-center))

    # Rotate into orbital flight attitude: a side-on three-quarter view so the
    # nose, heat shield and full trunk read as one silhouette.
    rot = (Matrix.Rotation(math.radians(-65), 4, "X") @
           Matrix.Rotation(math.radians(215), 4, "Z"))
    for o in meshes:
        o.data.transform(rot)
    bpy.context.view_layer.update()

    # Fit the camera around the rotated bounding sphere. The source glTF is
    # not normalized, so deriving the distance from the actual bounds keeps
    # the full capsule and trunk in frame at any asset scale.
    mn, mx = bounds(meshes)
    extent = mx - mn
    radius = 0.5 * math.sqrt(extent.x ** 2 + extent.y ** 2 + extent.z ** 2)
    sensor_h = 36.0 * scene.render.resolution_y / scene.render.resolution_x
    half_v = math.atan((sensor_h * 0.5) / CAMERA_LENS)
    distance = radius / math.sin(half_v) * FRAME_MARGIN

    cam_data = bpy.data.cameras.new("HeroCam")
    cam_data.lens = CAMERA_LENS
    cam_data.clip_start = max(0.01, distance * 0.001)
    cam_data.clip_end = distance * 10.0
    cam = bpy.data.objects.new("HeroCam", cam_data)
    cam.location = CAMERA_DIR * distance
    target = Vector((0.0, 0.0, 0.0))
    cam.rotation_euler = (target - cam.location).to_track_quat("-Z", "Y").to_euler()
    scene.collection.objects.link(cam)
    scene.camera = cam

    # Directional Key Sunlight catching capsule curves and normal map panel lines
    key_data = bpy.data.lights.new("SunKey", type="SUN")
    key_data.energy = 6.0
    key_data.color = (1.0, 0.98, 0.92)
    key = bpy.data.objects.new("SunKey", key_data)
    key.rotation_euler = Vector((-1.0, 1.2, -0.7)).normalized().to_track_quat("-Z", "Y").to_euler()
    scene.collection.objects.link(key)

    # Blue Earth-reflected Rim Light highlighting silhouette
    rim_data = bpy.data.lights.new("Rim", type="SUN")
    rim_data.energy = 4.0
    rim_data.color = (0.35, 0.70, 1.0)
    rim = bpy.data.objects.new("Rim", rim_data)
    rim.rotation_euler = Vector((1.0, -1.0, 0.8)).normalized().to_track_quat("-Z", "Y").to_euler()
    scene.collection.objects.link(rim)

    # Soft Fill Light, scaled with the model so it stays a fill at any size
    fill_data = bpy.data.lights.new("Fill", type="AREA")
    fill_data.energy = 1100.0
    fill_data.size = radius * 7.0
    fill_data.color = (0.75, 0.85, 1.0)
    fill = bpy.data.objects.new("Fill", fill_data)
    fill.location = CAMERA_DIR * distance * 0.9 + Vector((0.4, -0.2, 0.5)) * radius
    fill.rotation_euler = (target - fill.location).to_track_quat("-Z", "Y").to_euler()
    scene.collection.objects.link(fill)

    print(f"Dragon bounds: {tuple(round(v, 3) for v in extent)}, "
          f"radius={radius:.3f}, camera distance={distance:.3f}")

    os.makedirs(os.path.dirname(OUT_PNG), exist_ok=True)
    scene.render.filepath = OUT_PNG
    print(f"Rendering SpaceX Crew Dragon showcase to {OUT_PNG}...")
    bpy.ops.render.render(write_still=True)
    print("Done!")

if __name__ == "__main__":
    main()
