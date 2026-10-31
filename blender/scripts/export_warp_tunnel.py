import os, sys, math, struct, bpy, bmesh, numpy as np
from mathutils import Vector, Matrix

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
PROJECT_ROOT = os.path.dirname(SCRIPT_DIR)
REPO_ROOT = os.path.dirname(PROJECT_ROOT)
SRC_GLTF = os.path.join(PROJECT_ROOT, "out", "warp_tunnel_src", "scene.glb")
OUT_MODELS = os.path.join(REPO_ROOT, "res", "models")
OUT_SHOWCASE = os.path.join(PROJECT_ROOT, "out", "final_showcase")
DOCS_SCREENS = os.path.join(REPO_ROOT, "docs", "screens")

os.makedirs(OUT_MODELS, exist_ok=True)
os.makedirs(OUT_SHOWCASE, exist_ok=True)
os.makedirs(DOCS_SCREENS, exist_ok=True)

def write_bmp24(filepath, rgb_array):
    H, W, _ = rgb_array.shape
    row_bytes = W * 3
    padding = (4 - (row_bytes % 4)) % 4
    stride = row_bytes + padding
    image_size = stride * H
    file_size = 54 + image_size
    header = bytearray(54)
    header[0:2] = b'BM'
    struct.pack_into('<I', header, 2, file_size)
    struct.pack_into('<I', header, 10, 54)
    struct.pack_into('<I', header, 14, 40)
    struct.pack_into('<i', header, 18, W)
    struct.pack_into('<i', header, 22, H)
    struct.pack_into('<H', header, 26, 1)
    struct.pack_into('<H', header, 28, 24)
    struct.pack_into('<I', header, 30, 0)
    struct.pack_into('<I', header, 34, image_size)
    u8 = (np.clip(rgb_array, 0.0, 1.0) * 255.0).astype(np.uint8) if rgb_array.dtype != np.uint8 else rgb_array
    with open(filepath, 'wb') as f:
        f.write(header)
        for y in range(H):
            row = u8[H - 1 - y]
            f.write(row.tobytes())
            if padding:
                f.write(b'\x00' * padding)

ATLAS = 1024

def main():
    bpy.ops.wm.read_factory_settings(use_empty=True)
    scene = bpy.context.scene
    bpy.ops.import_scene.gltf(filepath=SRC_GLTF)

    # Bake world transforms
    meshes = [o for o in scene.objects if o.type == "MESH"]
    for o in meshes:
        world = o.matrix_world.copy()
        o.parent = None
        o.data.transform(world)
        o.matrix_world = Matrix.Identity(4)
    bpy.context.view_layer.update()

    # Normalize: center and scale to ~2.0 units
    mn = Vector((1e9,1e9,1e9)); mx = Vector((-1e9,-1e9,-1e9))
    for o in meshes:
        for c in o.bound_box:
            w = o.matrix_world @ Vector(c)
            for i in range(3):
                mn[i]=min(mn[i],w[i]); mx[i]=max(mx[i],w[i])
    center = (mn + mx) * 0.5
    size = max(mx[i]-mn[i] for i in range(3))
    scale = 2.0 / size
    for o in meshes:
        o.data.transform(Matrix.Translation(-center))
        for v in o.data.vertices:
            v.co *= scale
    bpy.context.view_layer.update()

    # Smooth shading
    for o in meshes:
        for poly in o.data.polygons:
            poly.use_smooth = True

    # UV unwrap if missing
    for o in meshes:
        if not o.data.uv_layers:
            bpy.context.view_layer.objects.active = o
            bpy.ops.object.mode_set(mode='EDIT')
            bpy.ops.mesh.select_all(action='SELECT')
            bpy.ops.uv.smart_project(angle_limit=66.0, island_margin=0.02)
            bpy.ops.object.mode_set(mode='OBJECT')

    # Join all meshes into one
    bpy.ops.object.select_all(action='DESELECT')
    for o in meshes:
        o.select_set(True)
    bpy.context.view_layer.objects.active = meshes[0]
    bpy.ops.object.join()
    obj = bpy.context.view_layer.objects.active
    obj.name = "WarpTunnel"

    # Export OBJ
    out_obj = os.path.join(OUT_MODELS, "warp_tunnel.obj")
    bpy.ops.object.select_all(action='DESELECT')
    obj.select_set(True)
    bpy.context.view_layer.objects.active = obj
    bpy.ops.wm.obj_export(filepath=out_obj, export_selected_objects=True, export_uv=True, export_normals=True)
    print("Exported " + out_obj)

    # Create textures (procedural, based on material colors)
    # Albedo: dark tunnel with blue/cyan glow
    alb = np.zeros((ATLAS, ATLAS, 3), dtype=np.uint8)
    for y in range(ATLAS):
        for x in range(ATLAS):
            t = (x + y) / (2*ATLAS)
            alb[y, x] = (int(10 + 20*t), int(15 + 40*t), int(30 + 80*t))
    # Roughness
    rough = np.full((ATLAS, ATLAS, 3), 40, dtype=np.uint8)
    # Metalness: low (organic tunnel)
    metal = np.full((ATLAS, ATLAS, 3), 30, dtype=np.uint8)
    # Normal: flat
    normal = np.full((ATLAS, ATLAS, 3), (128, 128, 255), dtype=np.uint8)
    # Emissive: cyan glow along the tunnel
    emi = np.zeros((ATLAS, ATLAS, 3), dtype=np.uint8)
    for y in range(ATLAS):
        d = abs(y - ATLAS//2) / (ATLAS * 0.3)
        if d < 1.0:
            t = (1.0 - d) ** 2
            emi[y, :] = (int(5*t), int(60*t), int(120*t))

    write_bmp24(os.path.join(OUT_MODELS, "warp_tunnel_albedo.bmp"), alb)
    write_bmp24(os.path.join(OUT_MODELS, "warp_tunnel_normal.bmp"), normal)
    write_bmp24(os.path.join(OUT_MODELS, "warp_tunnel_rough.bmp"), rough)
    write_bmp24(os.path.join(OUT_MODELS, "warp_tunnel_metal.bmp"), metal)
    write_bmp24(os.path.join(OUT_MODELS, "warp_tunnel_emissive.bmp"), emi)
    print("Textures written.")

    # Render showcase
    scene = bpy.context.scene
    scene.render.engine = "CYCLES"
    scene.cycles.device = "CPU"
    scene.render.resolution_x = 1280
    scene.render.resolution_y = 720
    scene.cycles.samples = 64
    scene.cycles.use_denoising = True

    world = bpy.data.worlds.new("WTWorld")
    scene.world = world
    world.use_nodes = True
    nt = world.node_tree
    bg = nt.nodes.new("ShaderNodeBackground")
    bg.inputs["Color"].default_value = (0.005, 0.01, 0.02, 1.0)
    bg.inputs["Strength"].default_value = 0.3
    out = nt.nodes.new("ShaderNodeOutputWorld")
    nt.links.new(bg.outputs["Background"], out.inputs["Surface"])

    cam_data = bpy.data.cameras.new("WTCam")
    cam_data.lens = 35.0
    cam = bpy.data.objects.new("WTCam", cam_data)
    cam.location = (0.0, -3.0, 1.0)
    cam.rotation_euler = (math.radians(75), 0, 0)
    scene.collection.objects.link(cam)
    scene.camera = cam

    key = bpy.data.lights.new("Key", "SUN")
    key.energy = 3.0
    key.color = (0.6, 0.8, 1.0)
    ko = bpy.data.objects.new("Key", key)
    ko.rotation_euler = (math.radians(-30), 0, math.radians(20))
    scene.collection.objects.link(ko)

    rim = bpy.data.lights.new("Rim", "SUN")
    rim.energy = 4.0
    rim.color = (0.1, 0.5, 1.0)
    ro = bpy.data.objects.new("Rim", rim)
    ro.rotation_euler = (math.radians(30), 0, math.radians(200))
    scene.collection.objects.link(ro)

    out_png = os.path.join(DOCS_SCREENS, "warp_tunnel_hero.png")
    scene.render.filepath = out_png
    bpy.ops.render.render(write_still=True)
    print("Rendered " + out_png)

if __name__ == "__main__":
    main()
