"""Deterministic PBR 3D asset generation for astro-voyager:
- Alcubierre Warp Bubble / Warp Drive visualization

Produces:
  res/models/warp_bubble.obj
  res/models/warp_bubble_albedo.bmp
  res/models/warp_bubble_normal.bmp
  res/models/warp_bubble_rough.bmp
  res/models/warp_bubble_metal.bmp
  res/models/warp_bubble_emissive.bmp
  docs/screens/warp_bubble_hero.png  (1280x720 Cycles)
"""
import os
import sys
import math
import struct
import shutil
import numpy as np
import bpy
import bmesh
from mathutils import Vector, Matrix

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
PROJECT_ROOT = os.path.dirname(SCRIPT_DIR)
REPO_ROOT = os.path.dirname(PROJECT_ROOT)
RES_MODELS = os.path.join(REPO_ROOT, "res", "models")
DOCS_SCREENS = os.path.join(REPO_ROOT, "docs", "screens")

os.makedirs(RES_MODELS, exist_ok=True)
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
    if rgb_array.dtype != np.uint8:
        u8 = (np.clip(rgb_array, 0.0, 1.0) * 255.0).astype(np.uint8)
    else:
        u8 = rgb_array
    with open(filepath, 'wb') as f:
        f.write(header)
        for y in range(H):
            row = u8[H - 1 - y]
            f.write(row.tobytes())
            if padding:
                f.write(b'\x00' * padding)

ATLAS = 1024

def make_textures():
    # Albedo: dark metallic hull with cyan energy accents
    alb = np.zeros((ATLAS, ATLAS, 3), dtype=np.uint8)
    alb[:, :] = (18, 22, 30)
    # Energy ring band (middle horizontal strip)
    mid = ATLAS // 2
    for i in range(ATLAS):
        d = abs(i - mid) / (ATLAS * 0.15)
        if d < 1.0:
            t = 1.0 - d
            alb[i, :] = (int(18 + 20*t), int(22 + 60*t), int(30 + 120*t))
    # Roughness: smooth metal
    rough = np.full((ATLAS, ATLAS, 3), 60, dtype=np.uint8)
    # Metalness: high
    metal = np.full((ATLAS, ATLAS, 3), 220, dtype=np.uint8)
    # Normal: flat
    normal = np.full((ATLAS, ATLAS, 3), (128, 128, 255), dtype=np.uint8)
    # Emissive: cyan ring glow
    emi = np.zeros((ATLAS, ATLAS, 3), dtype=np.uint8)
    for i in range(ATLAS):
        d = abs(i - mid) / (ATLAS * 0.08)
        if d < 1.0:
            t = (1.0 - d) ** 2
            emi[i, :] = (int(10*t), int(80*t), int(160*t))
    return alb, normal, rough, metal, emi

def build_mesh():
    bpy.ops.wm.read_factory_settings(use_empty=True)
    scene = bpy.context.scene

    # Central bubble sphere
    bpy.ops.mesh.primitive_uv_sphere_add(segments=32, ring_count=16, radius=0.6)
    bubble = bpy.context.active_object
    bubble.name = "Bubble"

    # Warp coil torus
    bpy.ops.mesh.primitive_torus_add(major_radius=1.4, minor_radius=0.18, major_segments=64, minor_segments=16)
    coil = bpy.context.active_object
    coil.name = "Coil"

    # Second coil (tilted)
    bpy.ops.mesh.primitive_torus_add(major_radius=1.4, minor_radius=0.12, major_segments=64, minor_segments=12)
    coil2 = bpy.context.active_object
    coil2.name = "Coil2"
    coil2.rotation_euler = (math.radians(60), 0, math.radians(30))

    # Join all
    bpy.ops.object.select_all(action='SELECT')
    bpy.context.view_layer.objects.active = bubble
    bpy.ops.object.join()
    obj = bpy.context.active_object
    obj.name = "WarpBubble"

    # Smooth shading
    for poly in obj.data.polygons:
        poly.use_smooth = True

    # UV unwrap
    bpy.ops.object.mode_set(mode='EDIT')
    bpy.ops.mesh.select_all(action='SELECT')
    bpy.ops.uv.smart_project(angle_limit=66.0, island_margin=0.02)
    bpy.ops.object.mode_set(mode='OBJECT')

    return obj

def export_obj(obj, path):
    bpy.ops.object.select_all(action='DESELECT')
    obj.select_set(True)
    bpy.context.view_layer.objects.active = obj
    bpy.ops.wm.obj_export(filepath=path, export_selected_objects=True,
                          export_uv=True, export_normals=True)

def main():
    obj = build_mesh()

    # Export OBJ
    out_obj = os.path.join(RES_MODELS, "warp_bubble.obj")
    export_obj(obj, out_obj)
    print(f"Exported {out_obj}")

    # Bake textures
    alb, normal, rough, metal, emi = make_textures()
    write_bmp24(os.path.join(RES_MODELS, "warp_bubble_albedo.bmp"), alb)
    write_bmp24(os.path.join(RES_MODELS, "warp_bubble_normal.bmp"), normal)
    write_bmp24(os.path.join(RES_MODELS, "warp_bubble_rough.bmp"), rough)
    write_bmp24(os.path.join(RES_MODELS, "warp_bubble_metal.bmp"), metal)
    write_bmp24(os.path.join(RES_MODELS, "warp_bubble_emissive.bmp"), emi)
    print("Textures written.")

    # Render showcase
    scene = bpy.context.scene
    scene.render.engine = "CYCLES"
    scene.cycles.device = "CPU"
    scene.render.resolution_x = 1280
    scene.render.resolution_y = 720
    scene.cycles.samples = 64
    scene.cycles.use_denoising = True

    world = bpy.data.worlds.new("WBWorld")
    scene.world = world
    world.use_nodes = True
    nt = world.node_tree
    bg = nt.nodes.new("ShaderNodeBackground")
    bg.inputs["Color"].default_value = (0.01, 0.015, 0.03, 1.0)
    bg.inputs["Strength"].default_value = 0.5
    out = nt.nodes.new("ShaderNodeOutputWorld")
    nt.links.new(bg.outputs["Background"], out.inputs["Surface"])

    # Camera
    cam_data = bpy.data.cameras.new("WBCam")
    cam_data.lens = 50.0
    cam = bpy.data.objects.new("WBCam", cam_data)
    cam.location = (4.0, -4.0, 2.5)
    cam.rotation_euler = (math.radians(60), 0, math.radians(45))
    scene.collection.objects.link(cam)
    scene.camera = cam

    # Lights
    key = bpy.data.lights.new("Key", "SUN")
    key.energy = 5.0
    key.color = (0.8, 0.9, 1.0)
    ko = bpy.data.objects.new("Key", key)
    ko.rotation_euler = (math.radians(-45), 0, math.radians(30))
    scene.collection.objects.link(ko)

    rim = bpy.data.lights.new("Rim", "SUN")
    rim.energy = 3.0
    rim.color = (0.2, 0.6, 1.0)
    ro = bpy.data.objects.new("Rim", rim)
    ro.rotation_euler = (math.radians(45), 0, math.radians(210))
    scene.collection.objects.link(ro)

    out_png = os.path.join(DOCS_SCREENS, "warp_bubble_hero.png")
    scene.render.filepath = out_png
    bpy.ops.render.render(write_still=True)
    print(f"Rendered {out_png}")

if __name__ == "__main__":
    main()
