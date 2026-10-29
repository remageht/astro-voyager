"""Export SpaceX Crew Dragon into game-ready OBJ and 2048x2048 5-channel PBR BMP atlas.

Layout:
  res/models/dragon.obj
  res/models/dragon_albedo.bmp
  res/models/dragon_normal.bmp
  res/models/dragon_rough.bmp
  res/models/dragon_metal.bmp
  res/models/dragon_emissive.bmp
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
SRC_GLTF = os.path.join(PROJECT_ROOT, "out", "spacex_src", "dragon", "scene.gltf")
TEX_DIR = os.path.join(PROJECT_ROOT, "out", "spacex_src", "dragon", "textures")
OUT_MODELS = os.path.join(REPO_ROOT, "res", "models")

ATLAS_SIZE = 2048

def write_bmp_24(filepath, rgb_array):
    """Write (H, W, 3) uint8 RGB array as a 24-bit uncompressed BMP.
    Stored top-down so ObjModel.cpp's loadBmp (which inverts row order for bottom-up)
    places row 0 at v=0.0 in OpenGL texture space.
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

    # Flip vertically so loadBmp inverted row maps correctly to OpenGL texture space
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
    # Nearest / bilinear sampling via numpy
    y_idx = (np.linspace(0, h - 1, target_h)).astype(np.int32)
    x_idx = (np.linspace(0, w - 1, target_w)).astype(np.int32)
    return arr[np.ix_(y_idx, x_idx)]

def main():
    bpy.ops.wm.read_factory_settings(use_empty=True)
    bpy.ops.import_scene.gltf(filepath=SRC_GLTF)

    # 1. Remove helper objects
    for name in ("Cube", "Mesh_2"):
        o = bpy.data.objects.get(name)
        if o:
            bpy.data.objects.remove(o, do_unlink=True)

    # 2. Quadrant Layout for Atlas:
    # Top Half (Y: 1024..2048, X: 0..2048): Body (Mesh_0, Mesh_4, Mesh_5, Mesh_6)
    #   u' = u, v' = 0.5 + v * 0.5
    # Bottom Left (Y: 0..1024, X: 0..1024): Trunk (Mesh_8)
    #   u' = u * 0.5, v' = v * 0.5
    # Bottom Right (Y: 0..1024, X: 1024..2048): Heatshield (Mesh_7, Mesh_1, Mesh_3)
    #   u' = 0.5 + u * 0.5, v' = v * 0.5

    for o in list(bpy.data.objects):
        if o.type != "MESH":
            continue
        mesh = o.data
        if not mesh.uv_layers:
            mesh.uv_layers.new(name="UVMap")
        uv_layer = mesh.uv_layers.active.data

        if o.name in ("Mesh_0", "Mesh_4", "Mesh_5", "Mesh_6"):
            # Body quadrant: scale V by 0.5 and shift by +0.5
            for loop in uv_layer:
                loop.uv.y = 0.5 + loop.uv.y * 0.5
        elif o.name == "Mesh_8":
            # Trunk: scale U and V by 0.5
            for loop in uv_layer:
                loop.uv.x = loop.uv.x * 0.5
                loop.uv.y = loop.uv.y * 0.5
        else: # Mesh_7, Mesh_1, Mesh_3 (Heatshield & metal fittings)
            # Heatshield: scale U and V by 0.5, shift U by +0.5
            for loop in uv_layer:
                loop.uv.x = 0.5 + loop.uv.x * 0.5
                loop.uv.y = loop.uv.y * 0.5

    # 3. Join all Dragon parts into single mesh
    meshes = [o for o in bpy.data.objects if o.type == "MESH"]
    bpy.ops.object.select_all(action="DESELECT")
    for m in meshes:
        m.select_set(True)
    bpy.context.view_layer.objects.active = meshes[0]
    bpy.ops.object.join()
    dragon = bpy.context.view_layer.objects.active
    dragon.name = "Dragon"

    # 4. Center and scale to game dimensions (length ~2.5 units, nose towards -Z)
    mn = Vector((1e9, 1e9, 1e9))
    mx = Vector((-1e9, -1e9, -1e9))
    for c in dragon.bound_box:
        w = dragon.matrix_world @ Vector(c)
        for i in range(3):
            mn[i] = min(mn[i], w[i])
            mx[i] = max(mx[i], w[i])
    center = (mn + mx) * 0.5
    size = max(mx[i] - mn[i] for i in range(3))

    # Center origin
    dragon.location -= center
    bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)

    # Scale to ~2.6 units length
    scale_factor = 2.6 / size
    dragon.scale = (scale_factor, scale_factor, scale_factor)
    bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)

    # Ensure all polygons are smooth
    for poly in dragon.data.polygons:
        poly.use_smooth = True

    # 5. Export OBJ
    out_obj = os.path.join(OUT_MODELS, "dragon.obj")
    os.makedirs(OUT_MODELS, exist_ok=True)
    bpy.ops.wm.obj_export(filepath=out_obj, export_materials=False,
                          forward_axis="NEGATIVE_Z", up_axis="Y")
    print(f"Exported OBJ: {out_obj}")

    # 6. Compose 2048x2048 PBR Textures
    # Load source textures
    body_alb = resize_image(load_image_rgb("body_baseColor.png"), 2048, 1024)
    body_nrm = resize_image(load_image_rgb("body_normal.png"), 2048, 1024)
    body_orm = resize_image(load_image_rgb("body_metallicRoughness.png"), 2048, 1024)

    trunk_alb = resize_image(load_image_rgb("trunk_baseColor.png"), 1024, 1024)
    trunk_nrm = resize_image(load_image_rgb("trunk_normal.png"), 1024, 1024)
    trunk_orm = resize_image(load_image_rgb("trunk_metallicRoughness.png"), 1024, 1024)

    hs_alb = resize_image(load_image_rgb("heatshield_baseColor.png"), 1024, 1024)
    hs_nrm = resize_image(load_image_rgb("heatshield_normal.png"), 1024, 1024)
    hs_orm = resize_image(load_image_rgb("heatshield_metallicRoughness.png"), 1024, 1024)

    # Initialize atlas buffers
    atlas_alb = np.full((ATLAS_SIZE, ATLAS_SIZE, 3), 220, dtype=np.uint8) # clean white
    atlas_nrm = np.zeros((ATLAS_SIZE, ATLAS_SIZE, 3), dtype=np.uint8)
    atlas_nrm[:, :] = [128, 128, 255] # flat normal
    atlas_rou = np.full((ATLAS_SIZE, ATLAS_SIZE, 3), 100, dtype=np.uint8)
    atlas_met = np.zeros((ATLAS_SIZE, ATLAS_SIZE, 3), dtype=np.uint8)
    atlas_emi = np.zeros((ATLAS_SIZE, ATLAS_SIZE, 3), dtype=np.uint8)

    # Paste Top Half: Body (Y: 1024..2048)
    atlas_alb[1024:2048, 0:2048] = body_alb
    atlas_nrm[1024:2048, 0:2048] = body_nrm
    # glTF metallicRoughness: G = Roughness, B = Metallic
    atlas_rou[1024:2048, 0:2048] = np.repeat(body_orm[:, :, 1:2], 3, axis=2)
    atlas_met[1024:2048, 0:2048] = np.repeat(body_orm[:, :, 2:3], 3, axis=2)

    # Paste Bottom Left: Trunk (Y: 0..1024, X: 0..1024)
    atlas_alb[0:1024, 0:1024] = trunk_alb
    atlas_nrm[0:1024, 0:1024] = trunk_nrm
    atlas_rou[0:1024, 0:1024] = np.repeat(trunk_orm[:, :, 1:2], 3, axis=2)
    atlas_met[0:1024, 0:1024] = np.repeat(trunk_orm[:, :, 2:3], 3, axis=2)

    # Paste Bottom Right: Heatshield (Y: 0..1024, X: 1024..2048)
    atlas_alb[0:1024, 1024:2048] = hs_alb
    atlas_nrm[0:1024, 1024:2048] = hs_nrm
    atlas_rou[0:1024, 1024:2048] = np.repeat(hs_orm[:, :, 1:2], 3, axis=2)
    atlas_met[0:1024, 1024:2048] = np.repeat(hs_orm[:, :, 2:3], 3, axis=2)

    # Subtle emissive status indicator on trunk solar panel edge
    atlas_emi[100:140, 20:60] = [30, 220, 180]

    # 7. Write BMP maps
    write_bmp_24(os.path.join(OUT_MODELS, "dragon_albedo.bmp"), atlas_alb)
    write_bmp_24(os.path.join(OUT_MODELS, "dragon_normal.bmp"), atlas_nrm)
    write_bmp_24(os.path.join(OUT_MODELS, "dragon_rough.bmp"), atlas_rou)
    write_bmp_24(os.path.join(OUT_MODELS, "dragon_metal.bmp"), atlas_met)
    write_bmp_24(os.path.join(OUT_MODELS, "dragon_emissive.bmp"), atlas_emi)

    print("SpaceX Crew Dragon asset export complete!")

if __name__ == "__main__":
    main()
