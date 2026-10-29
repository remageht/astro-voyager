"""Export Soviet/Russian Soyuz-MS spacecraft into game-ready OBJ and PBR BMP atlas.

Layout:
  res/models/soyuz.obj
  res/models/soyuz_albedo.bmp
  res/models/soyuz_normal.bmp
  res/models/soyuz_rough.bmp
  res/models/soyuz_metal.bmp
  res/models/soyuz_emissive.bmp
"""
import math
import os
import struct
import bpy
import numpy as np
from mathutils import Matrix, Vector

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
PROJECT_ROOT = os.path.dirname(SCRIPT_DIR)
REPO_ROOT = os.path.dirname(PROJECT_ROOT)
SRC_GLTF = os.path.join(PROJECT_ROOT, "out", "mir_src", "scene.gltf")
TEX_DIR = os.path.join(PROJECT_ROOT, "out", "mir_src", "textures")
OUT_MODELS = os.path.join(REPO_ROOT, "res", "models")

ATLAS_SIZE = 1024

def write_bmp_24(filepath, rgb_array):
    """Write (H, W, 3) uint8 RGB array as a 24-bit uncompressed BMP.
    Stored top-down so ObjModel.cpp's loadBmp places row 0 at v=0.0.
    """
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

    if rgb_array.dtype != np.uint8:
        u8 = (np.clip(rgb_array, 0.0, 1.0) * 255.0).astype(np.uint8)
    else:
        u8 = rgb_array

    u8_flipped = np.flipud(u8)
    bgr = u8_flipped[:, :, [2, 1, 0]]
    if padding == 0:
        data = bgr.tobytes()
    else:
        pad = b'\x00' * padding
        rows = [bgr[y, :, :].tobytes() + pad for y in range(H)]
        data = b''.join(rows)

    with open(filepath, 'wb') as f:
        f.write(header)
        f.write(data)
    print(f"Wrote BMP: {filepath} ({W}x{H}, {file_size} bytes)")

def load_image_rgb(name):
    p = os.path.join(TEX_DIR, name)
    if not os.path.exists(p):
        print(f"Warning: texture not found: {p}")
        return None
    img = bpy.data.images.load(p, check_existing=True)
    w, h = img.size[0], img.size[1]
    px = np.array(img.pixels[:], dtype=np.float32).reshape((h, w, 4))
    rgb = (px[:, :, :3] * 255.0).clip(0, 255).astype(np.uint8)
    return rgb

def resize_image(arr, target_w, target_h):
    if arr is None:
        return np.zeros((target_h, target_w, 3), dtype=np.uint8)
    h, w, c = arr.shape
    if w == target_w and h == target_h:
        return arr
    y_idx = (np.linspace(0, h - 1, target_h)).astype(np.int32)
    x_idx = (np.linspace(0, w - 1, target_w)).astype(np.int32)
    return arr[np.ix_(y_idx, x_idx)]

def main():
    bpy.ops.wm.read_factory_settings(use_empty=True)
    bpy.ops.import_scene.gltf(filepath=SRC_GLTF)

    # 1. Keep only Soyuz meshes
    soyuz_mesh_names = {"Soyuz.001_Soyuz_color_0", "Soyuz.001_Soyuz_black_Isolation_0"}
    for o in list(bpy.context.scene.objects):
        if o.name not in soyuz_mesh_names:
            bpy.data.objects.remove(o, do_unlink=True)

    # 2. Split UV Atlas:
    # Left Half (X: 0..512, Y: 0..1024): Soyuz_color (Mesh 2)
    #   u' = u * 0.5, v' = v
    # Right Half (X: 512..1024, Y: 0..1024): Soyuz_black_Isolation (Mesh 3)
    #   u' = 0.5 + u * 0.5, v' = v
    for o in list(bpy.context.scene.objects):
        if o.type != "MESH":
            continue
        mesh = o.data
        if not mesh.uv_layers:
            mesh.uv_layers.new(name="UVMap")
        uv_layer = mesh.uv_layers.active.data

        if "color" in o.name:
            for loop in uv_layer:
                loop.uv.x = loop.uv.x * 0.5
        else: # black isolation
            for loop in uv_layer:
                loop.uv.x = 0.5 + loop.uv.x * 0.5

    # 3. Join Soyuz meshes
    meshes = [o for o in bpy.context.scene.objects if o.type == "MESH"]
    bpy.ops.object.select_all(action="DESELECT")
    for m in meshes:
        m.select_set(True)
    bpy.context.view_layer.objects.active = meshes[0]
    bpy.ops.object.join()
    soyuz = bpy.context.view_layer.objects.active
    soyuz.name = "Soyuz"

    # 4. Center and scale (length ~2.8 units, docking probe towards -Z)
    mn = Vector((1e9, 1e9, 1e9))
    mx = Vector((-1e9, -1e9, -1e9))
    for c in soyuz.bound_box:
        w = soyuz.matrix_world @ Vector(c)
        for i in range(3):
            mn[i] = min(mn[i], w[i])
            mx[i] = max(mx[i], w[i])
    center = (mn + mx) * 0.5
    size = max(mx[i] - mn[i] for i in range(3))

    soyuz.location -= center
    bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)

    scale_factor = 2.8 / size
    soyuz.scale = (scale_factor, scale_factor, scale_factor)
    bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)

    for poly in soyuz.data.polygons:
        poly.use_smooth = True

    # 5. Export OBJ
    out_obj = os.path.join(OUT_MODELS, "soyuz.obj")
    os.makedirs(OUT_MODELS, exist_ok=True)
    bpy.ops.wm.obj_export(filepath=out_obj, export_materials=False,
                          forward_axis="NEGATIVE_Z", up_axis="Y")
    print(f"Exported OBJ: {out_obj}")

    # 6. Compose 1024x1024 PBR Texture Atlas
    color_alb = resize_image(load_image_rgb("Progress_color_baseColor.png"), 512, 1024)
    color_nrm = resize_image(load_image_rgb("Progress_color_normal.png"), 512, 1024)
    color_orm = resize_image(load_image_rgb("Soyuz_color_metallicRoughness.png"), 512, 1024)

    black_alb = np.full((1024, 512, 3), 18, dtype=np.uint8) # Dark black thermal blanket
    black_nrm = resize_image(load_image_rgb("Progress_black_Isolation_normal.png"), 512, 1024)
    black_orm = resize_image(load_image_rgb("Soyuz_black_Isolation_metallicRoughness.png"), 512, 1024)

    atlas_alb = np.full((ATLAS_SIZE, ATLAS_SIZE, 3), 200, dtype=np.uint8)
    atlas_nrm = np.zeros((ATLAS_SIZE, ATLAS_SIZE, 3), dtype=np.uint8)
    atlas_nrm[:, :] = [128, 128, 255]
    atlas_rou = np.full((ATLAS_SIZE, ATLAS_SIZE, 3), 120, dtype=np.uint8)
    atlas_met = np.zeros((ATLAS_SIZE, ATLAS_SIZE, 3), dtype=np.uint8)
    atlas_emi = np.zeros((ATLAS_SIZE, ATLAS_SIZE, 3), dtype=np.uint8)

    # Paste Left Half (X: 0..512): Soyuz_color
    atlas_alb[:, 0:512] = color_alb
    atlas_nrm[:, 0:512] = color_nrm
    atlas_rou[:, 0:512] = np.repeat(color_orm[:, :, 1:2], 3, axis=2)
    atlas_met[:, 0:512] = np.repeat(color_orm[:, :, 2:3], 3, axis=2)

    # Paste Right Half (X: 512..1024): Soyuz_black_Isolation
    atlas_alb[:, 512:1024] = black_alb
    atlas_nrm[:, 512:1024] = black_nrm
    atlas_rou[:, 512:1024] = np.repeat(black_orm[:, :, 1:2], 3, axis=2)
    atlas_met[:, 512:1024] = np.repeat(black_orm[:, :, 2:3], 3, axis=2)

    # Green orientation periscope beacon emissive highlight
    atlas_emi[20:40, 20:40] = [20, 240, 90]

    # Write BMPs
    write_bmp_24(os.path.join(OUT_MODELS, "soyuz_albedo.bmp"), atlas_alb)
    write_bmp_24(os.path.join(OUT_MODELS, "soyuz_normal.bmp"), atlas_nrm)
    write_bmp_24(os.path.join(OUT_MODELS, "soyuz_rough.bmp"), atlas_rou)
    write_bmp_24(os.path.join(OUT_MODELS, "soyuz_metal.bmp"), atlas_met)
    write_bmp_24(os.path.join(OUT_MODELS, "soyuz_emissive.bmp"), atlas_emi)

    print("Soyuz-MS asset export complete!")

if __name__ == "__main__":
    main()
