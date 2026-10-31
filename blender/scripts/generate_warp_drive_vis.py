import os, math, struct, bpy, bmesh, numpy as np
from mathutils import Vector, Matrix

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
PROJECT_ROOT = os.path.dirname(SCRIPT_DIR)
REPO_ROOT = os.path.dirname(PROJECT_ROOT)
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

    # --- Spacetime grid (deformed plane) ---
    # Create a grid mesh and displace it to form a warp bubble
    bpy.ops.mesh.primitive_grid_add(x_subdivisions=40, y_subdivisions=40, size=4.0)
    grid = bpy.context.active_object
    grid.name = "SpacetimeGrid"
    bm = bmesh.new()
    bm.from_mesh(grid.data)
    for v in bm.verts:
        x, y = v.co.x, v.co.y
        r = math.sqrt(x*x + y*y)
        # Alcubierre bubble: flat inside, distorted at the wall
        if r < 0.3:
            z = 0.0
        elif r < 1.2:
            # Wall: smooth bump
            t = (r - 0.3) / 0.9
            z = 0.6 * math.sin(t * math.pi) * (1 - t)
        else:
            z = 0.0
        v.co.z = z
    bm.to_mesh(grid.data)
    bm.free()

    # --- Central ship sphere ---
    bpy.ops.mesh.primitive_uv_sphere_add(segments=16, ring_count=8, radius=0.15)
    ship = bpy.context.active_object
    ship.name = "Ship"
    ship.location = (0, 0, 0.05)

    # --- Warp field rings (3 concentric tori) ---
    for i, (maj, minr) in enumerate([(0.9, 0.03), (1.1, 0.025), (1.3, 0.02)]):
        bpy.ops.mesh.primitive_torus_add(major_radius=maj, minor_radius=minr, major_segments=64, minor_segments=8)
        ring = bpy.context.active_object
        ring.name = "FieldRing" + str(i)
        ring.rotation_euler = (math.radians(i * 30), math.radians(i * 15), 0)

    # Join all
    bpy.ops.object.select_all(action='SELECT')
    bpy.context.view_layer.objects.active = grid
    bpy.ops.object.join()
    obj = bpy.context.active_object
    obj.name = "WarpDriveVis"

    # Smooth shading
    for poly in obj.data.polygons:
        poly.use_smooth = True

    # UV unwrap
    bpy.ops.object.mode_set(mode='EDIT')
    bpy.ops.mesh.select_all(action='SELECT')
    bpy.ops.uv.smart_project(angle_limit=66.0, island_margin=0.02)
    bpy.ops.object.mode_set(mode='OBJECT')

    # Export OBJ
    out_obj = os.path.join(OUT_MODELS, "warp_drive_vis.obj")
    bpy.ops.object.select_all(action='DESELECT')
    obj.select_set(True)
    bpy.context.view_layer.objects.active = obj
    bpy.ops.wm.obj_export(filepath=out_obj, export_selected_objects=True, export_uv=True, export_normals=True)
    print("Exported " + out_obj)

    # Textures
    # Albedo: dark space with blue grid
    alb = np.zeros((ATLAS, ATLAS, 3), dtype=np.uint8)
    alb[:, :] = (8, 12, 20)
    # Roughness
    rough = np.full((ATLAS, ATLAS, 3), 30, dtype=np.uint8)
    # Metalness
    metal = np.full((ATLAS, ATLAS, 3), 10, dtype=np.uint8)
    # Normal: flat
    normal = np.full((ATLAS, ATLAS, 3), (128, 128, 255), dtype=np.uint8)
    # Emissive: cyan/blue grid lines
    emi = np.zeros((ATLAS, ATLAS, 3), dtype=np.uint8)
    for i in range(0, ATLAS, 32):
        emi[i, :] = (10, 40, 80)
        emi[:, i] = (10, 40, 80)

    write_bmp24(os.path.join(OUT_MODELS, "warp_drive_vis_albedo.bmp"), alb)
    write_bmp24(os.path.join(OUT_MODELS, "warp_drive_vis_normal.bmp"), normal)
    write_bmp24(os.path.join(OUT_MODELS, "warp_drive_vis_rough.bmp"), rough)
    write_bmp24(os.path.join(OUT_MODELS, "warp_drive_vis_metal.bmp"), metal)
    write_bmp24(os.path.join(OUT_MODELS, "warp_drive_vis_emissive.bmp"), emi)
    print("Textures written.")

    # Render showcase
    scene = bpy.context.scene
    scene.render.engine = "CYCLES"
    scene.cycles.device = "CPU"
    scene.render.resolution_x = 1280
    scene.render.resolution_y = 720
    scene.cycles.samples = 64
    scene.cycles.use_denoising = True

    world = bpy.data.worlds.new("WDVWorld")
    scene.world = world
    world.use_nodes = True
    nt = world.node_tree
    bg = nt.nodes.new("ShaderNodeBackground")
    bg.inputs["Color"].default_value = (0.005, 0.01, 0.02, 1.0)
    bg.inputs["Strength"].default_value = 0.3
    out = nt.nodes.new("ShaderNodeOutputWorld")
    nt.links.new(bg.outputs["Background"], out.inputs["Surface"])

    cam_data = bpy.data.cameras.new("WDVCam")
    cam_data.lens = 50.0
    cam = bpy.data.objects.new("WDVCam", cam_data)
    cam.location = (3.0, -3.0, 2.0)
    cam.rotation_euler = (math.radians(55), 0, math.radians(45))
    scene.collection.objects.link(cam)
    scene.camera = cam

    key = bpy.data.lights.new("Key", "SUN")
    key.energy = 4.0
    key.color = (0.7, 0.85, 1.0)
    ko = bpy.data.objects.new("Key", key)
    ko.rotation_euler = (math.radians(-40), 0, math.radians(30))
    scene.collection.objects.link(ko)

    rim = bpy.data.lights.new("Rim", "SUN")
    rim.energy = 5.0
    rim.color = (0.1, 0.4, 1.0)
    ro = bpy.data.objects.new("Rim", rim)
    ro.rotation_euler = (math.radians(40), 0, math.radians(210))
    scene.collection.objects.link(ro)

    out_png = os.path.join(DOCS_SCREENS, "warp_drive_vis_hero.png")
    scene.render.filepath = out_png
    bpy.ops.render.render(write_still=True)
    print("Rendered " + out_png)

if __name__ == "__main__":
    main()
