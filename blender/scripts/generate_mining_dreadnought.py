"""Procedural 3D model generator for Heavy Mining Dreadnought / Colony Logistics Hauler.
Features:
- Armored command bridge & sensor array with panoramic visor
- Heavy industrial spinal chassis with reinforced bulkheads
- Dual frontal plasma mining laser arms with magnetic focal rings
- Rotary asteroid pulverizer and intake maw
- Modular cargo bay rack with 8 glowing hexagonal ore/gas canisters (He-3, Titan hydrocarbons, rare earths)
- Dorsal & ventral radiator heat dissipation fins
- Quad heavy fusion thrusters with glowing plasma exhaust cowlings
- Complete PBR texture set (albedo, normal, rough, metal, emissive) via asset_atlas.py
- High-res Cycles showcase render output

Run with:
blender --background --python blender/scripts/generate_mining_dreadnought.py
"""

import math
import struct
import sys
from pathlib import Path

import bpy
import numpy as np
from mathutils import Vector, Euler

# Import project asset_atlas
SCRIPTS_DIR = Path(__file__).resolve().parent
sys.path.insert(0, str(SCRIPTS_DIR))
import asset_atlas

ROOT = SCRIPTS_DIR.parents[1]
MODELS_DIR = ROOT / "res/models"
OUT_DIR = ROOT / "blender/out/final_showcase"
DOCS_DIR = ROOT / "docs/screens"

# Palette:
# 0: Dark gunmetal composite armor
# 1: Structural steel / dark ribs
# 2: Industrial hazard amber / gold for ore pods & warnings
# 3: Deep cyan / cryo-blue for Helium-3 pods
# 4: Bridge observation visor / sensor glass (cyan glow)
# 5: Radiator heat-sink copper/bronze
# 6: Thruster bells & dark machinery
# 7: Plasma laser emitter & reactor nozzle glow (blazing orange-amber emission)
PALETTE = [
    ((0.14, 0.16, 0.19), 0.42, 0.65, (0.0, 0.0, 0.0)),         # 0: panel
    ((0.08, 0.09, 0.11), 0.50, 0.85, (0.0, 0.0, 0.0)),         # 1: rib
    ((0.85, 0.55, 0.08), 0.32, 0.30, (0.15, 0.08, 0.01)),      # 2: quilt / hazard
    ((0.04, 0.40, 0.65), 0.28, 0.45, (0.02, 0.25, 0.45)),      # 3: cells / cryo
    ((0.00, 0.85, 0.95), 0.15, 0.10, (0.05, 0.90, 1.00)),      # 4: window / cyan
    ((0.42, 0.22, 0.12), 0.58, 0.90, (0.0, 0.0, 0.0)),         # 5: brushed / copper
    ((0.05, 0.06, 0.08), 0.25, 0.92, (0.0, 0.0, 0.0)),         # 6: recess / thruster
    ((1.00, 0.45, 0.05), 0.20, 0.10, (1.00, 0.50, 0.08)),      # 7: lens / plasma
]

PARTS = []
MATERIAL = None


def finish_mesh(obj, slot, smooth=False, bevel=0.006):
    """Assigns atlas material, remaps UVs to palette slot, applies bevel modifier."""
    obj.data.materials.append(MATERIAL)
    asset_atlas.remap_uv(obj, slot)
    for p in obj.data.polygons:
        p.use_smooth = smooth
    if bevel > 0.0:
        bpy.context.view_layer.objects.active = obj
        mod = obj.modifiers.new("Bevel", "BEVEL")
        mod.width = bevel
        mod.segments = 2
        mod.limit_method = "ANGLE"
        mod.angle_limit = math.radians(40)
        bpy.ops.object.modifier_apply(modifier=mod.name)
    PARTS.append(obj)
    return obj


def add_box(center, dims, slot=0, rot=(0, 0, 0), bevel=0.006):
    bpy.ops.mesh.primitive_cube_add(size=1.0, location=center)
    obj = bpy.context.object
    obj.scale = dims
    obj.rotation_euler = rot
    bpy.ops.object.transform_apply(scale=True, rotation=True)
    uv = obj.data.uv_layers.active
    for poly in obj.data.polygons:
        for corner, idx in enumerate(poly.loop_indices):
            uv.data[idx].uv = ((0, 0), (1, 0), (1, 1), (0, 1))[corner]
    return finish_mesh(obj, slot, bevel=bevel)


def add_cylinder(center, radius, length, axis="Y", sides=16, slot=0, rot=(0, 0, 0), bevel=0.004):
    bpy.ops.mesh.primitive_cylinder_add(
        vertices=sides, radius=radius, depth=length, location=center
    )
    obj = bpy.context.object
    if axis == "X":
        obj.rotation_euler = (0, math.pi / 2, 0)
    elif axis == "Y":
        obj.rotation_euler = (math.pi / 2, 0, 0)
    if any(rot):
        obj.rotation_euler.x += rot[0]
        obj.rotation_euler.y += rot[1]
        obj.rotation_euler.z += rot[2]
    bpy.ops.object.transform_apply(rotation=True)
    uv = obj.data.uv_layers.active
    for poly in obj.data.polygons:
        n = len(poly.loop_indices)
        for c, idx in enumerate(poly.loop_indices):
            u = c / float(n)
            v = 1.0 if c % 2 == 0 else 0.0
            uv.data[idx].uv = (u, v)
    return finish_mesh(obj, slot, smooth=True, bevel=bevel)


def add_cone(center, r1, r2, length, axis="Y", sides=16, slot=6):
    bpy.ops.mesh.primitive_cone_add(
        vertices=sides, radius1=r1, radius2=r2, depth=length, location=center
    )
    obj = bpy.context.object
    if axis == "Y":
        obj.rotation_euler = (math.pi / 2, 0, 0)
    elif axis == "X":
        obj.rotation_euler = (0, math.pi / 2, 0)
    bpy.ops.object.transform_apply(rotation=True)
    uv = obj.data.uv_layers.active
    for poly in obj.data.polygons:
        n = len(poly.loop_indices)
        for c, idx in enumerate(poly.loop_indices):
            uv.data[idx].uv = (c / float(n), 1.0 if c % 2 == 0 else 0.0)
    return finish_mesh(obj, slot, smooth=True, bevel=0.003)


def add_hex_pod(center, radius, length, slot=2):
    """Hexagonal container with end caps and clamp collars."""
    # Main hex body
    add_cylinder(center, radius, length, axis="Y", sides=6, slot=slot, bevel=0.004)
    # End caps
    cap_len = length * 0.12
    add_cylinder((center[0], center[1] + length * 0.5 + cap_len * 0.5, center[2]),
                 radius * 0.88, cap_len, axis="Y", sides=12, slot=1, bevel=0.002)
    add_cylinder((center[0], center[1] - length * 0.5 - cap_len * 0.5, center[2]),
                 radius * 0.88, cap_len, axis="Y", sides=12, slot=1, bevel=0.002)
    # Ring clamps
    for y_offset in (-length * 0.28, length * 0.28):
        add_cylinder((center[0], center[1] + y_offset, center[2]),
                     radius * 1.08, length * 0.08, axis="Y", sides=6, slot=0, bevel=0.002)


def build_mining_dreadnought():
    """Assembles all industrial components of the mining dreadnought."""
    # Coordinates convention *inside this builder*: +Y is Forward, +Z is Up,
    # +X is Starboard (Right). main() tips the joined hull onto the fleet
    # convention (-Z nose, +Y up, X spanwise) before export - see #195.
    
    # -------------------------------------------------------------
    # 1. CENTRAL SPINAL HULL & HEAVY BULKHEADS
    # -------------------------------------------------------------
    # Main spine chassis
    add_box((0, 0.0, 0.0), (1.1, 4.2, 0.75), slot=0, bevel=0.015)
    # Lower keel armor
    add_box((0, 0.1, -0.45), (0.85, 3.8, 0.35), slot=1, bevel=0.01)
    # Dorsal spine ridge
    add_box((0, -0.2, 0.45), (0.70, 3.6, 0.25), slot=1, bevel=0.008)

    # Lateral armor skirts / outriggers
    for side in (-1, 1):
        x = side * 0.75
        add_box((x, 0.0, -0.05), (0.35, 3.6, 0.55), slot=0, bevel=0.01)
        # Structural truss links between main hull and outriggers
        for y in (-1.2, -0.4, 0.4, 1.2):
            add_box((side * 0.6, y, 0.0), (0.25, 0.18, 0.35), slot=1, bevel=0.004)

    # -------------------------------------------------------------
    # 2. COMMAND BRIDGE TOWER & SENSOR ARRAY (Aft-Mid Dorsal)
    # -------------------------------------------------------------
    # Elevated bridge citadel
    add_box((0, -0.6, 0.78), (0.85, 1.1, 0.45), slot=0, bevel=0.012)
    # Upper bridge canopy
    add_box((0, -0.5, 1.05), (0.65, 0.85, 0.22), slot=1, bevel=0.008)
    # Panoramic armored visor (emissive cyan windows)
    add_box((0, -0.15, 1.06), (0.58, 0.12, 0.12), slot=4, bevel=0.002)
    # Side observation slits
    for side in (-1, 1):
        add_box((side * 0.32, -0.5, 1.06), (0.06, 0.6, 0.09), slot=4, bevel=0.002)

    # Communications and sensor mast
    add_cylinder((0, -1.0, 1.3), 0.05, 0.5, axis="Z", sides=8, slot=1)
    # High-gain directional radar dish
    add_cone((0, -1.0, 1.55), 0.28, 0.04, 0.15, axis="Z", sides=16, slot=5)
    add_cylinder((0, -1.0, 1.62), 0.02, 0.15, axis="Z", sides=6, slot=7)

    # -------------------------------------------------------------
    # 3. MODULAR CARGO BAY: 8 HEXAGONAL ORE PODS
    # -------------------------------------------------------------
    # 4 pairs of containers mounted laterally in cargo cradles
    cargo_configs = [
        (1.0, 2, "Hazard Ore"),      # Amber/Gold: Titanium & Platinum Ore
        (0.2, 3, "Helium-3"),        # Deep Cyan: Cryo Helium-3
        (-0.6, 2, "Silicate Regolith"),
        (-1.4, 3, "Hydrocarbons"),
    ]

    for y_pos, slot_id, label in cargo_configs:
        for side in (-1, 1):
            x_pos = side * 1.25
            z_pos = -0.05
            add_hex_pod((x_pos, y_pos, z_pos), radius=0.28, length=0.68, slot=slot_id)
            # Heavy magnetic cargo mount clamp attaching pod to outrigger
            add_box((side * 0.98, y_pos, z_pos), (0.22, 0.25, 0.22), slot=1, bevel=0.004)

    # -------------------------------------------------------------
    # 4. FRONT MINING RIG & DUAL PLASMA CUTTER ARMS (+Y forward)
    # -------------------------------------------------------------
    # Bow bulkhead transition
    add_box((0, 2.2, 0.0), (1.1, 0.45, 0.70), slot=0, bevel=0.01)
    
    # Central rotary rock crusher / pulverizer cone
    add_cone((0, 2.65, 0.0), 0.48, 0.22, 0.65, axis="Y", sides=12, slot=6)
    # Crusher teeth / intake vanes
    for i in range(6):
        ang = i * (math.pi / 3)
        cx = math.cos(ang) * 0.35
        cz = math.sin(ang) * 0.35
        add_box((cx, 2.65, cz), (0.06, 0.55, 0.06), slot=7, rot=(0, 0, ang), bevel=0.002)

    # Dual heavy laser boom arms
    for side in (-1, 1):
        x = side * 0.95
        # Heavy articulated shoulder mount
        add_box((x, 2.3, 0.1), (0.45, 0.5, 0.5), slot=0, bevel=0.01)
        # Forward extending boom arm
        add_box((x, 3.1, 0.1), (0.28, 1.2, 0.32), slot=1, bevel=0.008)
        # Plasma laser emitter housing
        add_cylinder((x, 3.8, 0.1), 0.22, 0.6, axis="Y", sides=16, slot=6, bevel=0.005)
        # Magnetic focusing rings (orange glow)
        add_cylinder((x, 3.7, 0.1), 0.27, 0.08, axis="Y", sides=16, slot=7, bevel=0.002)
        add_cylinder((x, 3.95, 0.1), 0.25, 0.08, axis="Y", sides=16, slot=7, bevel=0.002)
        # Laser emitter focal lens tip
        add_cone((x, 4.15, 0.1), 0.18, 0.06, 0.18, axis="Y", sides=16, slot=7)

    # -------------------------------------------------------------
    # 5. COOLING RADIATOR FINS (Dorsal & Keel Heat Sinks)
    # -------------------------------------------------------------
    # Dorsal radiator array (radiates excess mining laser & reactor heat)
    for i in range(5):
        y = 0.6 + i * 0.3
        add_box((0, y, 0.65), (0.04, 0.22, 0.45), slot=5, bevel=0.002)

    # Ventral radiator fins
    for i in range(4):
        y = -0.2 + i * 0.35
        add_box((0, y, -0.75), (0.04, 0.24, 0.35), slot=5, bevel=0.002)

    # -------------------------------------------------------------
    # 6. STERN PROPULSION & HEAVY FUSION THRUSTER CLUSTER (-Y aft)
    # -------------------------------------------------------------
    # Aft engineering engine block
    add_box((0, -2.2, 0.0), (1.3, 0.65, 0.85), slot=0, bevel=0.015)

    # Quad main thrusters (2x2 heavy rectangular-profile nozzle housings)
    thruster_offsets = [
        (-0.48, 0.26),  # Upper port
        (0.48, 0.26),   # Upper starboard
        (-0.48, -0.26), # Lower port
        (0.48, -0.26),  # Lower starboard
    ]

    for tx, tz in thruster_offsets:
        # Armored engine cowling
        add_cylinder((tx, -2.55, tz), 0.26, 0.55, axis="Y", sides=16, slot=1, bevel=0.005)
        # Expanding thruster bell
        add_cone((tx, -2.9, tz), 0.32, 0.22, 0.45, axis="Y", sides=24, slot=6)
        # Plasma nozzle interior emitter (blazing plasma thrust)
        add_cylinder((tx, -2.95, tz), 0.18, 0.12, axis="Y", sides=16, slot=7)

    # Secondary lateral maneuvering RCS blocks
    for side in (-1, 1):
        for y_rcs in (-1.8, 1.8):
            add_box((side * 1.05, y_rcs, 0.35), (0.16, 0.2, 0.16), slot=6, bevel=0.002)
            add_cone((side * 1.15, y_rcs, 0.35), 0.06, 0.02, 0.08, axis="X", sides=8, slot=7)


def setup_showcase_camera_and_lighting(scene, target_size=5.0):
    """Sets up dramatic cinematic lighting and camera for showcase render."""
    scene.render.engine = "CYCLES"
    scene.cycles.samples = 64
    scene.cycles.use_denoising = True
    scene.render.resolution_x = 1920
    scene.render.resolution_y = 1080
    scene.render.image_settings.file_format = "PNG"

    if not scene.world:
        scene.world = bpy.data.worlds.new("World")
    scene.world.use_nodes = True
    bg = scene.world.node_tree.nodes.get("Background")
    # Deep interstellar cosmic background
    bg.inputs["Color"].default_value = (0.006, 0.008, 0.014, 1.0)
    bg.inputs["Strength"].default_value = 0.4

    # 3-Point Cinematic Sci-Fi Lighting:
    # 1. Warm Solar Key Light (distant star illuminating top-starboard)
    key_light = bpy.data.lights.new("SunKey", "AREA")
    key_light.energy = 4200
    key_light.color = (1.0, 0.92, 0.80)
    key_light.size = 14
    key_obj = bpy.data.objects.new("SunKey", key_light)
    key_obj.location = (8.0, -8.0, 12.0)
    key_obj.rotation_euler = (-key_obj.location).to_track_quat("-Z", "Y").to_euler()
    scene.collection.objects.link(key_obj)

    # 2. Electric Cyan Rim Light (back-left edge defining silhouettes)
    rim_light = bpy.data.lights.new("CyanRim", "AREA")
    rim_light.energy = 5500
    rim_light.color = (0.0, 0.85, 1.0)
    rim_light.size = 10
    rim_obj = bpy.data.objects.new("CyanRim", rim_light)
    rim_obj.location = (-12.0, 10.0, 4.0)
    rim_obj.rotation_euler = (-rim_obj.location).to_track_quat("-Z", "Y").to_euler()
    scene.collection.objects.link(rim_obj)

    # 3. Soft Deep-Space Fill Light
    fill_light = bpy.data.lights.new("SoftFill", "AREA")
    fill_light.energy = 1800
    fill_light.color = (0.45, 0.60, 0.90)
    fill_light.size = 16
    fill_obj = bpy.data.objects.new("SoftFill", fill_light)
    fill_obj.location = (2.0, -12.0, -6.0)
    fill_obj.rotation_euler = (-fill_obj.location).to_track_quat("-Z", "Y").to_euler()
    scene.collection.objects.link(fill_obj)

    # Showcase Camera: 3/4 high-angle perspective showcasing the mining rig & bridge
    cam_data = bpy.data.cameras.new("DreadnoughtCamera")
    cam_data.lens = 50  # 50mm cinematic prime
    cam_data.clip_start = 0.1
    cam_data.clip_end = 100.0
    cam_obj = bpy.data.objects.new("DreadnoughtCamera", cam_data)
    scene.collection.objects.link(cam_obj)
    scene.camera = cam_obj

    # Position camera for majestic 3/4 hero view
    cam_obj.location = Vector((target_size * 1.3, target_size * 1.4, target_size * 0.9))
    look_target = Vector((0.0, 0.4, 0.0))
    direction = look_target - cam_obj.location
    cam_obj.rotation_euler = direction.to_track_quat("-Z", "Y").to_euler()


def main():
    MODELS_DIR.mkdir(parents=True, exist_ok=True)
    OUT_DIR.mkdir(parents=True, exist_ok=True)
    DOCS_DIR.mkdir(parents=True, exist_ok=True)

    bpy.ops.wm.read_factory_settings(use_empty=True)

    global MATERIAL, PARTS
    PARTS = []

    # 1. Build flip-safe PBR atlas for mining dreadnought
    prefix = "mining_dreadnought"
    print(f"Building PBR atlas for {prefix}...", flush=True)
    MATERIAL = asset_atlas.build(
        f"{prefix}_PBR", PALETTE, MODELS_DIR, prefix,
        size=1024,
        roles=("panel", "rib", "quilt", "cells", "window", "brushed", "recess", "lens"),
        seed_base=2048
    )

    # 2. Build 3D geometry
    print("Constructing dreadnought procedural geometry...", flush=True)
    build_mining_dreadnought()

    # 3. Join all components into unified game-ready mesh
    bpy.ops.object.select_all(action="DESELECT")
    for obj in PARTS:
        obj.select_set(True)
    bpy.context.view_layer.objects.active = PARTS[0]
    bpy.ops.object.join()
    joined = bpy.context.object
    joined.name = "MiningDreadnought"

    # Normalize to the fleet length. dart, shuttle, ring_ship and warp_ship are all
    # authored to ~2.3 in res/models and the renderer has no per-style scale, so a
    # hull registered as style 4 has to arrive at the same size or the chase camera
    # frames it wrong. (#195)
    bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)
    target_scale = 2.30
    max_dim = max(joined.dimensions)
    factor = target_scale / max_dim
    joined.scale = (factor, factor, factor)
    bpy.ops.object.transform_apply(scale=True)

    # Fleet axis convention (#195). The other hulls in res/models put their LENGTH
    # on the OBJ Z axis - dart, shuttle, ring_ship and warp_ship all measure ~2.3
    # on Z and ~0.5-0.9 on Y - which is what the ship renderer's shipHullModel and
    # SceneManager's ownUpToY both assume. This builder authored +Y forward / +Z up,
    # so the whole hull is tipped once here: Rx(-90) sends Blender +Y (length) to -Z
    # and Blender +Z (up) to +Y, giving generate_ships.py's "-Z is the nose, +Y up,
    # X spanwise". Do not "fix" this per part - the tip must stay a single step.
    joined.rotation_euler = (-math.pi / 2, 0.0, 0.0)
    bpy.ops.object.transform_apply(rotation=True)

    poly_count = len(joined.data.polygons)
    print(f"Dreadnought built successfully: {poly_count} polygons, size: {target_scale}", flush=True)

    # 4. Export OBJ + MTL to res/models/
    obj_path = MODELS_DIR / f"{prefix}.obj"
    print(f"Exporting OBJ to {obj_path}...", flush=True)
    bpy.ops.object.select_all(action="DESELECT")
    joined.select_set(True)
    bpy.context.view_layer.objects.active = joined
    bpy.ops.wm.obj_export(
        filepath=str(obj_path),
        export_selected_objects=True,
        export_uv=True,
        export_normals=True,
        export_materials=True,
        export_triangulated_mesh=True,
        forward_axis="Y",
        up_axis="Z",
        path_mode="RELATIVE",
    )

    # 5. Render high-res Cycles showcase. The camera above is authored for the
    # builder's own pose (+Y forward, +Z up = Blender's Z-up world), so undo the
    # #195 tip for the render pass only. The exported OBJ keeps the fleet frame;
    # this is purely so the hero shot is not a ship standing on its tail.
    joined.rotation_euler = (math.pi / 2, 0.0, 0.0)
    bpy.ops.object.transform_apply(rotation=True)

    scene = bpy.context.scene
    setup_showcase_camera_and_lighting(scene, target_size=target_scale)
    render_out = OUT_DIR / f"{prefix}_showcase.png"
    scene.render.filepath = str(render_out)
    print(f"Rendering showcase image to {render_out}...", flush=True)
    bpy.ops.render.render(write_still=True)

    # Copy to docs/screens for portfolio & documentation
    docs_copy = DOCS_DIR / f"{prefix}_showcase.png"
    try:
        import shutil
        shutil.copyfile(render_out, docs_copy)
        print(f"Copied showcase to {docs_copy}", flush=True)
    except Exception as e:
        print(f"Warning copying to docs: {e}", flush=True)

    print("ALL DONE: Mining Dreadnought successfully generated!", flush=True)


if __name__ == "__main__":
    main()
