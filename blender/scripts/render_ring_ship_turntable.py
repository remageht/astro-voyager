"""Render eight roll angles of the hand-tuned Ring Ship OBJ.

The Ring Ship mesh is git-ignored and its baked PBR atlas belongs to the
current UV layout, so this tool reuses `render_ring_ship_showcase.py` instead
of regenerating the asset.  Each frame rolls the ship around its long axis in
45-degree steps; the static two-pylon ring mounts and the running lights then
read from every side.

Usage (headless):
  blender -b -P blender/scripts/render_ring_ship_turntable.py
  blender -b -P blender/scripts/render_ring_ship_turntable.py -- --quick

Frames:
  blender/out/turntable_frames/ring_ship_00.png ... ring_ship_07.png
  (quick: blender/out/turntable_preview/)

Compose the committed sheet:
  powershell -ExecutionPolicy Bypass -File tools/make_contact_sheet.ps1 `
      -InputDir blender/out/turntable_frames `
      -Out docs/screens/ring_ship_turntable.png `
      -Cols 4 -CellWidth 640 -CellHeight 360
"""
import math
import os
import sys

import bpy

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, SCRIPT_DIR)
import render_ring_ship_showcase as showcase  # noqa: E402

FRAMES = 8


def argv_flag(name):
    return name in sys.argv


def load_ship():
    if not os.path.isfile(showcase.OBJ_PATH):
        raise FileNotFoundError(showcase.OBJ_PATH)
    bpy.ops.wm.obj_import(filepath=showcase.OBJ_PATH,
                          forward_axis="Y", up_axis="Z")
    ship = next(o for o in bpy.context.selected_objects if o.type == "MESH")
    ship.name = "RingShipTurntable"

    # The game atlas is stored top-down; Blender loads images bottom-up.
    uv_layer = ship.data.uv_layers.active
    if uv_layer is not None:
        for loop_uv in uv_layer.data:
            loop_uv.uv.y = 1.0 - loop_uv.uv.y

    ship.data.materials.clear()
    ship.data.materials.append(showcase.build_baked_material())
    for poly in ship.data.polygons:
        poly.material_index = 0
        poly.use_smooth = True
    return ship


def main():
    quick = argv_flag("--quick")
    out_dir = os.path.join(
        showcase.PROJECT_ROOT, "out",
        "turntable_preview" if quick else "turntable_frames")
    os.makedirs(out_dir, exist_ok=True)

    bpy.ops.wm.read_factory_settings(use_empty=True)
    scene = bpy.context.scene
    ship = load_ship()

    showcase.setup_render(scene, quick)
    scene.render.resolution_x = 320 if quick else 640
    scene.render.resolution_y = 180 if quick else 360
    # The contact-sheet frames are smaller than the hero shot; keep the bright
    # ring bands from clipping while the hull stays readable.
    scene.view_settings.exposure = 0.25
    showcase.add_camera_and_lights(scene)

    base = (math.radians(-72.0), math.radians(18.0), math.radians(35.0))
    for i in range(FRAMES):
        # Local Z is the ship's long axis; Euler Z is applied last, so this is
        # a clean roll of the whole hull + ring mounts in front of the camera.
        ship.rotation_euler = (base[0], base[1], base[2] + math.radians(45.0 * i))
        scene.render.filepath = os.path.join(out_dir, f"ring_ship_{i:02d}.png")
        print(f"[{i + 1}/{FRAMES}] {scene.render.filepath}")
        bpy.ops.render.render(write_still=True)

    print("Ring Ship turntable frames complete.")


if __name__ == "__main__":
    main()
