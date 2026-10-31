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
    # Normalize diameter to ~4.0 units (radius 2.0)
    scale = 4.0 / size
    for o in meshes:
        o.data.transform(Matrix.Translation(-center))
        for v in o.data.vertices:
            v.co *= scale
        for poly in o.data.polygons:
            poly.use_smooth = True
        if not o.data.uv_layers:
            bpy.context.view_layer.objects.active = o
            bpy.ops.object.mode_set(mode='EDIT')
            bpy.ops.mesh.select_all(action='SELECT')
            bpy.ops.uv.smart_project(angle_limit=66.0, island_margin=0.02)
            bpy.ops.object.mode_set(mode='OBJECT')
    bpy.context.view_layer.update()

    # Identify distinct layers:
    tube_obj = None
    scrap_obj = None
    cloud_obj = None
    for o in meshes:
        if "Tunnel_0" in o.name:
            tube_obj = o
        elif "Tunnel.001" in o.name:
            scrap_obj = o
        elif "Torus.002" in o.name:
            cloud_obj = o

    layers_to_export = [
        (tube_obj, "warp_tunnel_tube.obj", "WarpTunnelTube"),
        (scrap_obj, "warp_tunnel_scrap.obj", "WarpTunnelScrap"),
        (cloud_obj, "warp_tunnel_cloud.obj", "WarpTunnelCloud")
    ]

    for obj, filename, final_name in layers_to_export:
        if not obj:
            continue
        obj.name = final_name
        out_path = os.path.join(OUT_MODELS, filename)
        bpy.ops.object.select_all(action='DESELECT')
        obj.select_set(True)
        bpy.context.view_layer.objects.active = obj
        bpy.ops.wm.obj_export(filepath=out_path, export_selected_objects=True, export_uv=True, export_normals=True)
        print("Exported layer " + out_path)

    # Also export combined mesh for backward-compatibility with dock-warp-tunnel
    bpy.ops.object.select_all(action='DESELECT')
    for o in meshes:
        o.select_set(True)
    bpy.context.view_layer.objects.active = meshes[0]
    bpy.ops.object.duplicate()
    dup_meshes = [o for o in bpy.context.selected_objects if o.type == "MESH"]
    bpy.context.view_layer.objects.active = dup_meshes[0]
    bpy.ops.object.join()
    comb = bpy.context.view_layer.objects.active
    comb.name = "WarpTunnelCombined"
    out_comb = os.path.join(OUT_MODELS, "warp_tunnel.obj")
    bpy.ops.object.select_all(action='DESELECT')
    comb.select_set(True)
    bpy.context.view_layer.objects.active = comb
    bpy.ops.wm.obj_export(filepath=out_comb, export_selected_objects=True, export_uv=True, export_normals=True)
    print("Exported combined " + out_comb)

    # Create textures (procedural, based on material colors)
    # 1. Base / Combined textures
    alb = np.zeros((ATLAS, ATLAS, 3), dtype=np.uint8)
    for y in range(ATLAS):
        for x in range(ATLAS):
            t = (x + y) / (2*ATLAS)
            alb[y, x] = (int(10 + 20*t), int(18 + 45*t), int(35 + 85*t))
    rough = np.full((ATLAS, ATLAS, 3), 45, dtype=np.uint8)
    metal = np.full((ATLAS, ATLAS, 3), 50, dtype=np.uint8)
    normal = np.full((ATLAS, ATLAS, 3), (128, 128, 255), dtype=np.uint8)
    emi = np.zeros((ATLAS, ATLAS, 3), dtype=np.uint8)
    for y in range(ATLAS):
        d = abs(y - ATLAS//2) / (ATLAS * 0.3)
        if d < 1.0:
            t = (1.0 - d) ** 2
            emi[y, :] = (int(10*t), int(80*t), int(180*t))

    write_bmp24(os.path.join(OUT_MODELS, "warp_tunnel_albedo.bmp"), alb)
    write_bmp24(os.path.join(OUT_MODELS, "warp_tunnel_normal.bmp"), normal)
    write_bmp24(os.path.join(OUT_MODELS, "warp_tunnel_rough.bmp"), rough)
    write_bmp24(os.path.join(OUT_MODELS, "warp_tunnel_metal.bmp"), metal)
    write_bmp24(os.path.join(OUT_MODELS, "warp_tunnel_emissive.bmp"), emi)

    # 2. Tube textures (high-tech titanium hull with luminescent energy rings)
    alb_tube = np.zeros((ATLAS, ATLAS, 3), dtype=np.uint8)
    emi_tube = np.zeros((ATLAS, ATLAS, 3), dtype=np.uint8)
    for y in range(ATLAS):
        rib = 0.5 + 0.5 * math.cos(y * 0.05)
        for x in range(ATLAS):
            flt = 0.5 + 0.5 * math.sin(x * 0.08)
            val = int(12 + 15 * rib + 10 * flt)
            alb_tube[y, x] = (val, int(val * 1.25), int(val * 1.8))
            if abs(math.sin(y * 0.03)) > 0.88:
                emi_tube[y, x] = (15, 110, 240)
    write_bmp24(os.path.join(OUT_MODELS, "warp_tunnel_tube_albedo.bmp"), alb_tube)
    write_bmp24(os.path.join(OUT_MODELS, "warp_tunnel_tube_normal.bmp"), normal)
    write_bmp24(os.path.join(OUT_MODELS, "warp_tunnel_tube_rough.bmp"), np.full((ATLAS, ATLAS, 3), 35, dtype=np.uint8))
    write_bmp24(os.path.join(OUT_MODELS, "warp_tunnel_tube_metal.bmp"), np.full((ATLAS, ATLAS, 3), 80, dtype=np.uint8))
    write_bmp24(os.path.join(OUT_MODELS, "warp_tunnel_tube_emissive.bmp"), emi_tube)

    # 3. Scrap textures (structural debris and cage ribs)
    alb_scrap = np.full((ATLAS, ATLAS, 3), 65, dtype=np.uint8)
    emi_scrap = np.zeros((ATLAS, ATLAS, 3), dtype=np.uint8)
    for y in range(ATLAS):
        for x in range(ATLAS):
            if (x % 64 < 8) and (y % 64 < 8):
                emi_scrap[y, x] = (255, 140, 20)  # Amber warning markers
    write_bmp24(os.path.join(OUT_MODELS, "warp_tunnel_scrap_albedo.bmp"), alb_scrap)
    write_bmp24(os.path.join(OUT_MODELS, "warp_tunnel_scrap_normal.bmp"), normal)
    write_bmp24(os.path.join(OUT_MODELS, "warp_tunnel_scrap_rough.bmp"), np.full((ATLAS, ATLAS, 3), 60, dtype=np.uint8))
    write_bmp24(os.path.join(OUT_MODELS, "warp_tunnel_scrap_metal.bmp"), np.full((ATLAS, ATLAS, 3), 90, dtype=np.uint8))
    write_bmp24(os.path.join(OUT_MODELS, "warp_tunnel_scrap_emissive.bmp"), emi_scrap)

    # 4. Cloud textures (swirling plasma torus)
    alb_cloud = np.zeros((ATLAS, ATLAS, 3), dtype=np.uint8)
    emi_cloud = np.zeros((ATLAS, ATLAS, 3), dtype=np.uint8)
    for y in range(ATLAS):
        cy = abs(y - ATLAS // 2) / (ATLAS * 0.45)
        fade = max(0.0, 1.0 - cy) ** 1.5
        for x in range(ATLAS):
            swirl = 0.5 + 0.5 * math.sin(x * 0.04 + y * 0.02)
            glow = fade * swirl
            alb_cloud[y, x] = (int(20 * glow), int(70 * glow), int(160 * glow))
            emi_cloud[y, x] = (int(30 * glow), int(120 * glow), int(255 * glow))
    write_bmp24(os.path.join(OUT_MODELS, "warp_tunnel_cloud_albedo.bmp"), alb_cloud)
    write_bmp24(os.path.join(OUT_MODELS, "warp_tunnel_cloud_normal.bmp"), normal)
    write_bmp24(os.path.join(OUT_MODELS, "warp_tunnel_cloud_rough.bmp"), np.full((ATLAS, ATLAS, 3), 10, dtype=np.uint8))
    write_bmp24(os.path.join(OUT_MODELS, "warp_tunnel_cloud_metal.bmp"), np.full((ATLAS, ATLAS, 3), 10, dtype=np.uint8))
    write_bmp24(os.path.join(OUT_MODELS, "warp_tunnel_cloud_emissive.bmp"), emi_cloud)

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
