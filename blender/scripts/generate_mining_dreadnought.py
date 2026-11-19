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


def add_dreadnought_details():
    """Reference-driven relief pass for the Heavy Mining Dreadnought.

    Everything below is authored in the builder frame (+Y forward, +Z up,
    +X starboard) and is real geometry, not texture tricks: reinforcement
    banding, weld seams, keel and dorsal stringers and outrigger trusses on the
    spinal chassis; a bridge visor with mullions, sun shade, handrails, masts,
    sensor horns and dish feed; crusher teeth, hinge knuckles, hydraulic rams,
    ring vanes, emitter lenses and boom cable runs on the mining rig; pod
    straps, manifold piping and hazard-striped end frames on the cargo rack;
    fin arrays and edge rails on the radiator panels; and bell throats, gimbal
    rings, feed mains, RCS clusters, a service crane and berthing hardware in
    the engine room.

    Nothing floats: each piece is either buried in a hull surface or welded to
    hardware that is. Rule of thumb used throughout - protrude by at least
    0.02 and embed by at least 0.01, so no face lands coplanar with an existing
    one (that is what z-fights in the atlas bake).
    """
    # Radial placement helpers. The primitives in this file only aim on the
    # X/Y/Z axes, so anything that has to point off-axis is built from an
    # axis="X" cylinder plus a single Euler component:
    #   rot=(0, -a, 0) -> aims along the outward radial (cos a, 0, sin a) of
    #                     the +Y spine (used by the rig, pods, engines)
    def radial_spine(center, radius, length, azim, sides=10, slot=1, bevel=0.0):
        add_cylinder(center, radius, length, axis="X", rot=(0.0, -azim, 0.0),
                     sides=sides, slot=slot, bevel=bevel)

    def spoke(center, dims, azim, slot=1, bevel=0.0):
        """Box whose dims read (radial, fore-aft, tangential) at azimuth azim."""
        add_box(center, dims, slot=slot, rot=(0.0, -azim, 0.0), bevel=bevel)

    def nozzle_aft(center, r=0.024, flare=0.055, length=0.11, slot=7):
        """Nozzle bell opening aft (-Y): narrow at +Y, flared at -Y."""
        add_cone(center, r, flare, length, axis="Y", sides=10, slot=slot)

    def nozzle_side(center, outboard, r=0.022, flare=0.050, length=0.10, slot=7):
        """Nozzle bell opening outboard along +-X."""
        add_cone(center,
                 r if outboard > 0 else flare,
                 flare if outboard > 0 else r,
                 length, axis="X", sides=10, slot=slot)

    # ------------------------------------------------------------------
    # A. SPINAL CHASSIS: banding, weld seams, stringers, trusses
    # ------------------------------------------------------------------
    # Transverse reinforcement bands around the spine (X +-0.55, Z +-0.375)
    for y_band in (-1.95, -1.55, 1.30, 1.70, 2.00):
        add_box((0.0, y_band, 0.0), (1.17, 0.11, 0.83), slot=1, bevel=0.005)
    # Matching band plates struck across the lateral skirts (X +-0.925 face)
    for y_band in (-1.55, 1.30, 1.70):
        for side in (-1, 1):
            add_box((side * 0.94, y_band, -0.05), (0.05, 0.13, 0.62), slot=6, bevel=0.0)

    # Longitudinal weld seams along the spine sides and the dorsal ridge. The
    # skirts occupy X +-0.575..0.925 / Z -0.325..0.225, so the seams have to
    # ride above and below that band or they vanish into the gap.
    for side in (-1, 1):
        for z_seam in (0.31, -0.34):
            add_box((side * 0.56, -0.1, z_seam), (0.04, 4.05, 0.05), slot=6, bevel=0.0)
        add_box((side * 0.36, -0.2, 0.59), (0.05, 3.40, 0.05), slot=6, bevel=0.0)

    # Keel stringers plus cross ties (keel armour bottom is Z = -0.625)
    for side in (-1, 1):
        add_box((side * 0.30, 0.15, -0.645), (0.10, 3.55, 0.08), slot=1, bevel=0.004)
    for y_tie in (-1.5, -0.9, -0.3, 0.3, 0.9, 1.5):
        add_box((0.0, y_tie, -0.645), (0.62, 0.10, 0.08), slot=6, bevel=0.0)

    # Dorsal ridge stringers and transverse ribs
    for side in (-1, 1):
        add_box((side * 0.24, -0.2, 0.585), (0.07, 3.45, 0.06), slot=1, bevel=0.003)
    for y_rib in (-1.85, -1.45, 0.15, 0.55, 1.45):
        add_box((0.0, y_rib, 0.585), (0.66, 0.09, 0.05), slot=6, bevel=0.0)

    # Skirt-to-keel structural struts and outboard armour patches
    for side in (-1, 1):
        for y_strut in (-1.5, -0.5, 0.5, 1.5):
            add_box((side * 0.55, y_strut, -0.50), (0.30, 0.13, 0.24), slot=1, bevel=0.003)
        for y_patch in (-1.15, -0.35, 0.50, 0.95):
            add_box((side * 0.935, y_patch, -0.05), (0.05, 0.52, 0.40), slot=0, bevel=0.0)
        # access ladder rungs on the skirt outer face
        for k in range(6):
            add_box((side * 0.94, -1.70 + k * 0.16, -0.20), (0.06, 0.08, 0.025),
                    slot=1, bevel=0.0)

    # ------------------------------------------------------------------
    # B. BRIDGE: visor, mullions, sun shade, rails, masts, sensors
    # ------------------------------------------------------------------
    # Armored visor band carried forward of the canopy face (canopy: Y -0.925
    # .. -0.075, X +-0.325, Z 0.94 .. 1.16) - the original window box sits
    # inside the canopy, so this is what actually reads as glass.
    add_box((0.0, -0.055, 1.055), (0.60, 0.08, 0.15), slot=4, bevel=0.002)
    for x_mull in (-0.24, -0.12, 0.0, 0.12, 0.24):
        add_box((x_mull, -0.04, 1.055), (0.028, 0.05, 0.17), slot=1, bevel=0.0)
    # Sun shade: horizontal lip off the canopy nose plus a drooped tip plate
    add_box((0.0, 0.0, 1.155), (0.68, 0.20, 0.05), slot=1, bevel=0.003)
    add_box((0.0, 0.115, 1.13), (0.68, 0.07, 0.035), slot=2,
            rot=(-0.55, 0.0, 0.0), bevel=0.0)
    # Mullion ribs standing proud of the side observation slits (X +-0.35)
    for side in (-1, 1):
        for y_slit in (-0.72, -0.5, -0.28):
            add_box((side * 0.355, y_slit, 1.06), (0.04, 0.06, 0.13), slot=1, bevel=0.0)

    # Walkway handrail on the exposed citadel roof (deck Z = 1.005)
    for side in (-1, 1):
        add_box((side * 0.42, -0.62, 1.115), (0.022, 1.02, 0.022), slot=2, bevel=0.0)
        for y_post in (-1.10, -0.86, -0.62, -0.38, -0.14):
            add_box((side * 0.42, y_post, 1.055), (0.02, 0.02, 0.13), slot=1, bevel=0.0)
    add_box((0.0, -1.12, 1.115), (0.86, 0.022, 0.022), slot=2, bevel=0.0)
    for x_post in (-0.30, 0.0, 0.30):
        add_box((x_post, -1.12, 1.055), (0.02, 0.02, 0.13), slot=1, bevel=0.0)

    # Bridge wing platforms with edge kerbs, gussets and wingtip lights
    for side in (-1, 1):
        add_box((side * 0.51, -0.60, 0.93), (0.20, 0.46, 0.045), slot=0, bevel=0.003)
        add_box((side * 0.60, -0.60, 0.965), (0.02, 0.44, 0.05), slot=2, bevel=0.0)
        add_box((side * 0.55, -0.42, 0.88), (0.10, 0.05, 0.10), slot=1,
                rot=(0.0, 0.0, side * 0.6), bevel=0.0)
        add_box((side * 0.58, -0.60, 0.975), (0.05, 0.10, 0.06), slot=4, bevel=0.0)

    # Whip antennas and an aft blade mast off the canopy roof
    for side in (-1, 1):
        add_cylinder((side * 0.27, -0.86, 1.42), 0.013, 0.55, axis="Z", sides=8, slot=1)
        add_cone((side * 0.27, -0.86, 1.715), 0.013, 0.001, 0.06, axis="Z", sides=8, slot=6)
    add_cylinder((0.0, -0.90, 1.32), 0.022, 0.36, axis="Z", sides=8, slot=1)
    add_box((0.0, -0.90, 1.54), (0.012, 0.16, 0.12), slot=1, rot=(0.0, 0.25, 0.0), bevel=0.0)
    add_box((0.0, -0.90, 1.42), (0.14, 0.03, 0.03), slot=6, bevel=0.0)

    # Sensor horn cluster and lens banks on the citadel nose and flanks
    for side in (-1, 1):
        add_cone((side * 0.33, 0.0, 0.78), 0.10, 0.035, 0.16, axis="Y", sides=12, slot=6)
        for y_lens, slot_lens in ((-0.30, 4), (-0.62, 7), (-0.94, 4)):
            add_cylinder((side * 0.435, y_lens, 0.80), 0.05, 0.04, axis="X",
                         sides=12, slot=slot_lens, bevel=0.0)

    # Comm mast foot: the stock mast starts 0.045 above the citadel deck, so
    # pedestal it down before rebuilding the dish on top of it.
    add_box((0.0, -1.0, 1.02), (0.20, 0.22, 0.12), slot=1, bevel=0.003)
    # Radar dish: mouth rim, feed rod, three stanchions, spreader and horn
    add_cylinder((0.0, -1.0, 1.50), 0.305, 0.04, axis="Z", sides=20, slot=5, bevel=0.002)
    add_cylinder((0.0, -1.0, 1.62), 0.030, 0.34, axis="Z", sides=10, slot=6)
    for k in range(3):
        ang = k * math.tau / 3.0 + 0.4
        add_cylinder((0.22 * math.cos(ang), -1.0 + 0.22 * math.sin(ang), 1.64),
                     0.012, 0.30, axis="Z", sides=6, slot=1)
    add_cylinder((0.0, -1.0, 1.775), 0.23, 0.03, axis="Z", sides=18, slot=1, bevel=0.0)
    add_cone((0.0, -1.0, 1.83), 0.07, 0.02, 0.10, axis="Z", sides=12, slot=6)

    # ------------------------------------------------------------------
    # C. CARGO RACK: straps, bolts, manifold piping, hazard end frames
    # ------------------------------------------------------------------
    pod_ys = (1.0, 0.2, -0.6, -1.4)
    for y_pod in pod_ys:
        for side in (-1, 1):
            # strap bands over the hex shells (radius 0.28, Z centre -0.05)
            for y_off in (-0.20, 0.20):
                add_cylinder((side * 1.25, y_pod + y_off, -0.05), 0.31, 0.055,
                             axis="Y", sides=12, slot=1, bevel=0.0)
                # strap bolt blocks top and bottom
                for z_bolt in (0.24, -0.34):
                    add_box((side * 1.25, y_pod + y_off, z_bolt), (0.12, 0.08, 0.05),
                            slot=2, bevel=0.0)

    # Fore-aft manifold headers on the skirt outer lower edge, with a drop
    # leg into every canister and two crossovers under the keel.
    for side in (-1, 1):
        add_cylinder((side * 0.95, -0.2, -0.26), 0.038, 3.20, axis="Y",
                     sides=12, slot=5, bevel=0.002)
        for y_pod in pod_ys:
            add_cylinder((side * 1.12, y_pod, -0.26), 0.028, 0.30, axis="X",
                         sides=10, slot=5, bevel=0.0)
            add_box((side * 0.99, y_pod, -0.26), (0.06, 0.07, 0.09), slot=1, bevel=0.0)
    # Crossover lines slung under the keel with a riser to each header
    for y_xover in (0.6, -1.0):
        add_cylinder((0.0, y_xover, -0.645), 0.034, 1.95, axis="X", sides=12,
                     slot=5, bevel=0.0)
        for side in (-1, 1):
            add_cylinder((side * 0.95, y_xover, -0.45), 0.030, 0.42, axis="Z",
                         sides=10, slot=5, bevel=0.0)

    # Hazard-striped end frames: outer posts bite into the canister ends,
    # inner posts land on the skirts, rails close the hoop.
    for y_frame in (1.46, -1.82):
        for side in (-1, 1):
            add_box((side * 1.40, y_frame, -0.05), (0.10, 0.12, 0.80), slot=2, bevel=0.004)
            add_box((side * 0.90, y_frame, 0.15), (0.10, 0.12, 0.30), slot=2, bevel=0.0)
            add_box((side * 0.90, y_frame, -0.30), (0.10, 0.12, 0.26), slot=2, bevel=0.0)
        add_box((0.0, y_frame, 0.30), (2.90, 0.11, 0.07), slot=2, bevel=0.003)
        add_box((0.0, y_frame, -0.42), (2.90, 0.11, 0.07), slot=2, bevel=0.003)
        # corner strobes on both ends of the frame
        for side in (-1, 1):
            add_box((side * 1.40, y_frame, 0.345), (0.08, 0.09, 0.06), slot=7, bevel=0.0)
    # Rack handrails tying the two end frames together
    for side in (-1, 1):
        add_box((side * 1.40, -0.18, 0.335), (0.06, 3.30, 0.05), slot=1, bevel=0.0)
        add_box((side * 1.40, -0.18, -0.455), (0.06, 3.30, 0.05), slot=1, bevel=0.0)

    # ------------------------------------------------------------------
    # D. MINING RIG: teeth, hinge knuckles, rams, ring vanes, cables
    # ------------------------------------------------------------------
    # Pulveriser drum collar at the maw root (cone runs Y 2.325 .. 2.975)
    add_cylinder((0.0, 2.36, 0.0), 0.50, 0.14, axis="Y", sides=20, slot=1, bevel=0.003)
    # Tooth ring on the forward rim
    for k in range(12):
        ang = k * math.tau / 12.0
        spoke((0.46 * math.cos(ang), 2.90, 0.46 * math.sin(ang)),
              (0.14, 0.17, 0.11), ang, slot=6, bevel=0.002)
    # Breaker spikes in three rows down the tapered flank
    for y_row, r_row in ((2.50, 0.30), (2.65, 0.35), (2.80, 0.41)):
        for k in range(8):
            ang = k * math.tau / 8.0 + (0.2 if y_row == 2.65 else 0.0)
            radial_spine(((r_row + 0.02) * math.cos(ang), y_row, (r_row + 0.02) * math.sin(ang)),
                         0.030, 0.20, ang, sides=8, slot=7)

    for side in (-1, 1):
        x = side * 0.95
        # Boom shoulder and emitter hinge knuckles
        add_cylinder((x, 2.55, 0.1), 0.10, 0.52, axis="X", sides=16, slot=1, bevel=0.003)
        add_cylinder((x, 3.62, 0.1), 0.075, 0.46, axis="X", sides=14, slot=1, bevel=0.002)
        # Hydraulic ram: shoulder top to boom flank, barrel + rod + clevises
        ram_tilt = -0.38
        add_box((x, 2.12, 0.36), (0.14, 0.12, 0.10), slot=1, bevel=0.0)
        add_cylinder((x, 2.31, 0.29), 0.058, 0.42, axis="Y", sides=12, slot=6,
                     rot=(ram_tilt, 0.0, 0.0), bevel=0.002)
        add_cylinder((x, 2.58, 0.18), 0.027, 0.36, axis="Y", sides=10, slot=5,
                     rot=(ram_tilt, 0.0, 0.0), bevel=0.0)
        add_box((x, 2.72, 0.12), (0.12, 0.10, 0.09), slot=1, bevel=0.0)
        # Rigid underside brace from the shoulder into the boom belly, with a
        # jack pad block that the boom can grip on to.
        add_cylinder((x, 2.75, -0.075), 0.045, 0.80, axis="Y", sides=10, slot=6,
                     rot=(0.062, 0.0, 0.0), bevel=0.002)
        add_box((x, 3.02, -0.10), (0.12, 0.10, 0.12), slot=1, bevel=0.0)

        # Focusing-ring collars, cooling vanes and emitter hardware
        for y_ring, r_ring in ((3.70, 0.27), (3.95, 0.25)):
            add_cylinder((x, y_ring + 0.05, 0.1), r_ring + 0.035, 0.05, axis="Y",
                         sides=18, slot=1, bevel=0.002)
            for k in range(4):
                ang = k * math.tau / 4.0 + math.pi / 4.0
                spoke((x + (r_ring + 0.05) * math.cos(ang), y_ring,
                       0.1 + (r_ring + 0.05) * math.sin(ang)),
                      (0.09, 0.09, 0.05), ang, slot=7, bevel=0.0)
        # Emitter nose collar, lens and four-prong guard cage
        add_cylinder((x, 4.20, 0.1), 0.205, 0.05, axis="Y", sides=16, slot=1, bevel=0.002)
        add_cylinder((x, 4.245, 0.1), 0.10, 0.06, axis="Y", sides=14, slot=7, bevel=0.0)
        for k in range(4):
            ang = k * math.tau / 4.0 + math.pi / 4.0
            spoke((x + 0.175 * math.cos(ang), 4.30, 0.1 + 0.175 * math.sin(ang)),
                  (0.05, 0.24, 0.05), ang, slot=6, bevel=0.0)

        # Cable runs and conduit clamps along both boom faces
        for x_off, z_off in ((0.10, 0.18), (-0.16, 0.0)):
            add_cylinder((x + side * x_off, 3.05, 0.1 + z_off), 0.030, 1.15, axis="Y",
                         sides=10, slot=6, bevel=0.0)
            for y_clamp in (2.62, 2.95, 3.28, 3.61):
                add_box((x + side * x_off, y_clamp, 0.1 + z_off), (0.07, 0.05, 0.07),
                        slot=1, bevel=0.0)
        # Junction box on the shoulder top, bracketed to the boom cable tray
        add_box((x, 2.44, 0.39), (0.16, 0.14, 0.12), slot=6, bevel=0.002)
        add_box((x + side * 0.055, 2.50, 0.31), (0.21, 0.14, 0.10), slot=6, bevel=0.0)

    # Prospector probe mast on the maw face - the pulveriser drum swallows
    # anything axially short, so the probe has to clear Y = 2.975.
    add_box((0.0, 2.98, 0.0), (0.22, 0.10, 0.16), slot=1, bevel=0.002)
    add_cylinder((0.0, 3.04, 0.0), 0.060, 0.05, axis="Y", sides=12, slot=4, bevel=0.0)
    add_cylinder((0.0, 3.06, 0.0), 0.055, 0.30, axis="Y", sides=12, slot=1, bevel=0.002)
    add_cone((0.0, 3.28, 0.0), 0.008, 0.055, 0.16, axis="Y", sides=12, slot=6)

    # Bow RCS blocks on the bulkhead corners
    for side in (-1, 1):
        for z_rcs in (0.28, -0.28):
            add_box((side * 0.50, 2.44, z_rcs), (0.14, 0.14, 0.14), slot=6, bevel=0.002)
            for y_noz in (2.40, 2.50):
                nozzle_aft((side * 0.50, y_noz, z_rcs))

    # ------------------------------------------------------------------
    # E. RADIATORS: fin arrays, edge rails, tip weights
    # ------------------------------------------------------------------
    # Dorsal panels: (0, 0.6 + i*0.3, 0.65), 0.04 x 0.22 x 0.45
    for i in range(5):
        y_fin = 0.6 + i * 0.3
        for side in (-1, 1):
            add_box((side * 0.024, y_fin, 0.655), (0.025, 0.24, 0.47), slot=1, bevel=0.0)
        for z_slat in (0.50, 0.66, 0.82):
            add_box((0.0, y_fin, z_slat), (0.15, 0.05, 0.028), slot=5, bevel=0.0)
        add_box((0.0, y_fin, 0.885), (0.06, 0.24, 0.04), slot=1, bevel=0.0)

    # Ventral panels: (0, -0.2 + i*0.35, -0.75), 0.04 x 0.24 x 0.35
    for i in range(4):
        y_fin = -0.2 + i * 0.35
        for side in (-1, 1):
            add_box((side * 0.024, y_fin, -0.745), (0.025, 0.26, 0.37), slot=1, bevel=0.0)
        for z_slat in (-0.86, -0.74, -0.62):
            add_box((0.0, y_fin, z_slat), (0.14, 0.05, 0.026), slot=5, bevel=0.0)
        add_box((0.0, y_fin, -0.93), (0.07, 0.26, 0.04), slot=1, bevel=0.0)

    # ------------------------------------------------------------------
    # F. ENGINE ROOM: throats, gimbals, feeds, RCS, berthing, crane
    # ------------------------------------------------------------------
    thruster_offsets = ((-0.48, 0.26), (0.48, 0.26), (-0.48, -0.26), (0.48, -0.26))
    for tx, tz in thruster_offsets:
        # Expanding throat bolted onto the existing bell rim (r 0.22 at Y -3.125)
        add_cone((tx, -3.22, tz), 0.22, 0.345, 0.30, axis="Y", sides=24, slot=6)
        add_cylinder((tx, -3.34, tz), 0.355, 0.045, axis="Y", sides=20, slot=5, bevel=0.002)
        # Gimbal ring plus lugs between cowl and block
        add_cylinder((tx, -2.74, tz), 0.29, 0.07, axis="Y", sides=18, slot=1, bevel=0.002)
        for k in range(4):
            ang = k * math.tau / 4.0 + math.pi / 4.0
            spoke((tx + 0.30 * math.cos(ang), -2.74, tz + 0.30 * math.sin(ang)),
                  (0.10, 0.12, 0.09), ang, slot=6, bevel=0.0)
        # Outboard feed line running aft to the cowl
        add_cylinder((tx + (0.22 if tx > 0 else -0.22), -2.45, tz), 0.035, 0.55,
                    axis="Y", sides=10, slot=5, bevel=0.0)
        # Auger/pump casing on top of each cowl
        add_box((tx, -2.42, tz + (0.24 if tz > 0 else -0.24)), (0.20, 0.30, 0.10),
                slot=0, bevel=0.002)

    # Transverse propellant mains across the aft bell cluster
    for tz in (0.26, -0.26):
        add_cylinder((0.0, -2.56, tz), 0.042, 1.50, axis="X", sides=12, slot=5, bevel=0.002)

    # Berthing port on the engine block aft face (Y = -2.525)
    add_cylinder((0.0, -2.57, 0.0), 0.19, 0.16, axis="Y", sides=20, slot=1, bevel=0.003)
    add_cylinder((0.0, -2.58, 0.0), 0.12, 0.20, axis="Y", sides=16, slot=6, bevel=0.0)
    for k in range(4):
        ang = k * math.tau / 4.0 + math.pi / 4.0
        spoke((0.21 * math.cos(ang), -2.60, 0.21 * math.sin(ang)),
              (0.08, 0.12, 0.07), ang, slot=2, bevel=0.0)
        add_box((0.20 * math.cos(k * math.tau / 4.0), -2.645,
                 0.20 * math.sin(k * math.tau / 4.0)), (0.05, 0.05, 0.05), slot=4, bevel=0.0)

    # Aft RCS clusters on the skirt tails plus midship lateral blocks
    for side in (-1, 1):
        for y_rcs, z_rcs in ((-1.86, 0.10), (-1.05, -0.20)):
            add_box((side * 0.85, y_rcs, z_rcs), (0.18, 0.18, 0.15), slot=6, bevel=0.002)
            nozzle_aft((side * 0.85, y_rcs - 0.10, z_rcs))
            nozzle_side((side * 0.96, y_rcs, z_rcs), side)
        add_box((side * 0.94, 0.35, 0.05), (0.12, 0.20, 0.18), slot=6, bevel=0.002)
        nozzle_side((side * 0.99, 0.35, 0.05), side)
        # The stock lateral RCS blocks (X +-1.05, Z 0.35) hang in mid-air off
        # the skirt corner - give each one a mounting pylon back to the skirt.
        for y_pylon in (-1.8, 1.8):
            add_box((side * 0.97, y_pylon, 0.24), (0.12, 0.22, 0.22), slot=1, bevel=0.003)

    # Service crane on the aft deck for ore-hauler berthing duty
    add_cylinder((0.30, -2.10, 0.59), 0.075, 0.36, axis="Z", sides=12, slot=1, bevel=0.002)
    add_cylinder((0.30, -2.10, 0.44), 0.11, 0.06, axis="Z", sides=14, slot=6, bevel=0.0)
    add_box((0.30, -2.36, 0.79), (0.08, 0.62, 0.06), slot=2, bevel=0.002)
    add_box((0.30, -2.64, 0.73), (0.07, 0.07, 0.12), slot=1, bevel=0.0)
    add_cylinder((0.30, -2.64, 0.66), 0.030, 0.10, axis="Z", sides=8, slot=6, bevel=0.0)

    # Aft engineering grilles and access panels on the block flanks
    for side in (-1, 1):
        for k in range(4):
            add_box((side * 0.665, -2.45 + k * 0.16, 0.30), (0.03, 0.10, 0.20),
                    slot=6, bevel=0.0)
        add_box((side * 0.665, -2.10, -0.20), (0.03, 0.55, 0.30), slot=0, bevel=0.0)

    # ------------------------------------------------------------------
    # G. NAV LIGHTS AND MARKERS
    # ------------------------------------------------------------------
    add_box((0.0, -1.0, 1.80), (0.06, 0.06, 0.05), slot=7, bevel=0.0)
    for side in (-1, 1):
        add_box((side * 0.50, 2.40, 0.33), (0.08, 0.08, 0.06), slot=4, bevel=0.0)
        add_box((side * 0.94, -1.86, 0.20), (0.05, 0.08, 0.06), slot=7, bevel=0.0)


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

    # -------------------------------------------------------------
    # 7. REFERENCE-DRIVEN RELIEF PASS (banding, rig hardware, RCS, piping)
    # -------------------------------------------------------------
    add_dreadnought_details()


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
