"""Render a quick preview of warp_drive.glb."""
import os
import bpy
from mathutils import Vector

SRC = r"C:\Users\Эмир\Documents\kimi\tasks\2026-10-18\21-19-50-3ecbc4f6\warp_drive.glb"
OUT = r"c:\Users\Эмир\Documents\Default Project\astro-voyager\docs\screens\warp_drive_preview.png"

def main():
    bpy.ops.wm.read_factory_settings(use_empty=True)
    bpy.ops.import_scene.gltf(filepath=SRC)

    scene = bpy.context.scene
    scene.render.engine = "CYCLES"
    scene.cycles.device = "CPU"
    scene.render.resolution_x = 1280
    scene.render.resolution_y = 720
    scene.cycles.samples = 32
    scene.cycles.use_denoising = True

    # Camera
    cam_data = bpy.data.cameras.new("Cam")
    cam_data.lens = 45.0
    cam = bpy.data.objects.new("Cam", cam_data)
    cam.location = (4.0, -5.0, 3.0)
    cam.rotation_euler = (Vector((0, 0, 0)) - cam.location).to_track_quat("-Z", "Y").to_euler()
    scene.collection.objects.link(cam)
    scene.camera = cam

    # Sun light
    sun_data = bpy.data.lights.new("Sun", type="SUN")
    sun_data.energy = 5.0
    sun = bpy.data.objects.new("Sun", sun_data)
    sun.rotation_euler = Vector((-1.0, 1.0, -0.8)).normalized().to_track_quat("-Z", "Y").to_euler()
    scene.collection.objects.link(sun)

    # Rim light
    rim_data = bpy.data.lights.new("Rim", type="SUN")
    rim_data.energy = 3.0
    rim_data.color = (0.3, 0.7, 1.0)
    rim = bpy.data.objects.new("Rim", rim_data)
    rim.rotation_euler = Vector((1.0, -1.0, 0.8)).normalized().to_track_quat("-Z", "Y").to_euler()
    scene.collection.objects.link(rim)

    scene.render.filepath = OUT
    print(f"Rendering to {OUT}...")
    bpy.ops.render.render(write_still=True)
    print("Done!")

if __name__ == "__main__":
    main()
