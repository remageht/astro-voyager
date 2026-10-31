"""Blender 5.2 Cycles renderer for Tidal Disruption Event AT2019qiz showcase.

Generates:
  docs/screens/at2019qiz.png
  docs/screens/tde_at2019qiz_hero.png
"""
import os
import sys
import math
import shutil
import numpy as np
import bpy
import bmesh
from mathutils import Vector, Matrix, Euler

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
PROJECT_ROOT = os.path.dirname(SCRIPT_DIR)
REPO_ROOT = os.path.dirname(PROJECT_ROOT)
DOCS_SCREENS = os.path.join(REPO_ROOT, "docs", "screens")
os.makedirs(DOCS_SCREENS, exist_ok=True)

def reset_scene():
    bpy.ops.wm.read_factory_settings(use_empty=True)

def create_emission_mat(name, color, strength):
    mat = bpy.data.materials.new(name=name)
    mat.use_nodes = True
    nodes = mat.node_tree.nodes
    links = mat.node_tree.links
    nodes.clear()

    node_out = nodes.new(type='ShaderNodeOutputMaterial')
    node_emi = nodes.new(type='ShaderNodeEmission')
    node_emi.inputs['Color'].default_value = color
    node_emi.inputs['Strength'].default_value = strength
    links.new(node_emi.outputs['Emission'], node_out.inputs['Surface'])
    return mat

def create_black_hole_mat():
    mat = bpy.data.materials.new(name="BlackHoleHorizon")
    mat.use_nodes = True
    nodes = mat.node_tree.nodes
    links = mat.node_tree.links
    nodes.clear()

    node_out = nodes.new(type='ShaderNodeOutputMaterial')
    node_bsdf = nodes.new(type='ShaderNodeBsdfPrincipled')
    node_bsdf.inputs['Base Color'].default_value = (0.001, 0.001, 0.002, 1.0)
    node_bsdf.inputs['Roughness'].default_value = 1.0
    links.new(node_bsdf.outputs['BSDF'], node_out.inputs['Surface'])
    return mat

def build_scene():
    reset_scene()
    scene = bpy.context.scene
    scene.render.engine = 'CYCLES'
    scene.cycles.device = 'CPU'
    scene.cycles.samples = 64
    scene.cycles.preview_samples = 16
    scene.view_settings.view_transform = 'Filmic'
    scene.view_settings.look = 'Medium High Contrast'

    # World background: Deep space pitch black
    world = bpy.data.worlds.new("TDEWorld")
    world.use_nodes = True
    wnodes = world.node_tree.nodes
    wlinks = world.node_tree.links
    wnodes.clear()
    node_wout = wnodes.new(type='ShaderNodeOutputWorld')
    node_bg = wnodes.new(type='ShaderNodeBackground')
    node_bg.inputs['Color'].default_value = (0.0005, 0.0008, 0.0018, 1.0)
    node_bg.inputs['Strength'].default_value = 1.0
    wlinks.new(node_bg.outputs['Background'], node_wout.inputs['Surface'])
    scene.world = world

    # 1. Central Supermassive Black Hole Event Horizon (R = 1.35)
    bpy.ops.mesh.primitive_uv_sphere_add(radius=1.35, segments=64, ring_count=32, location=(0, 0, 0))
    bh_obj = bpy.context.active_object
    bh_obj.name = "SMBH_Horizon"
    bpy.ops.object.shade_smooth()
    bh_obj.data.materials.append(create_black_hole_mat())

    # 2. Photon Sphere Ring (R = 1.95, thin intense torus)
    bpy.ops.mesh.primitive_torus_add(
        major_radius=1.95,
        minor_radius=0.038,
        major_segments=96,
        minor_segments=24,
        location=(0, 0, 0),
        rotation=(math.radians(32), math.radians(18), 0)
    )
    photon_ring = bpy.context.active_object
    photon_ring.name = "PhotonRing"
    bpy.ops.object.shade_smooth()
    photon_ring.data.materials.append(create_emission_mat("PhotonMat", (0.85, 1.15, 1.7, 1.0), 8.0))

    # 3. Accretion Disk (Inclined circularized disk R=2.8, thickness=0.25)
    bpy.ops.mesh.primitive_torus_add(
        major_radius=2.8,
        minor_radius=0.26,
        major_segments=96,
        minor_segments=32,
        location=(0, 0, 0),
        rotation=(math.radians(35), math.radians(20), 0)
    )
    disk_obj = bpy.context.active_object
    disk_obj.name = "AccretionDisk"
    bpy.ops.object.shade_smooth()
    disk_obj.data.materials.append(create_emission_mat("DiskMat", (0.55, 0.85, 1.45, 1.0), 5.5))

    # 4. Spaghettified Stellar Stream (Relativistic spiral ribbon)
    curve_data = bpy.data.curves.new('SpaghettiCurve', type='CURVE')
    curve_data.dimensions = '3D'
    curve_data.bevel_depth = 0.085
    curve_data.bevel_resolution = 8

    polyline = curve_data.splines.new('BEZIER')
    n_points = 28
    polyline.bezier_points.add(n_points - 1)

    # Parametric spiral wrapping horizon from r=1.7 to r=7.2
    for i in range(n_points):
        t = i / float(n_points - 1)
        r = 1.70 + 5.50 * (t ** 1.3)
        angle = t * math.pi * 2.6 - 0.45
        x = r * math.cos(angle)
        y = r * math.sin(angle) * 0.75
        z = (1.0 - t) * 0.55 * math.sin(angle * 2.0)
        pt = polyline.bezier_points[i]
        pt.co = Vector((x, y, z))
        pt.handle_left_type = 'AUTO'
        pt.handle_right_type = 'AUTO'

    curve_obj = bpy.data.objects.new('SpaghettiStream', curve_data)
    bpy.context.collection.objects.link(curve_obj)
    curve_obj.data.materials.append(create_emission_mat("StreamMat", (0.75, 1.15, 1.65, 1.0), 7.0))

    # Receding arm trailing tail (golden flame)
    curve_data2 = bpy.data.curves.new('TrailingCurve', type='CURVE')
    curve_data2.dimensions = '3D'
    curve_data2.bevel_depth = 0.11
    curve_data2.bevel_resolution = 8
    polyline2 = curve_data2.splines.new('BEZIER')
    polyline2.bezier_points.add(14)
    for i in range(15):
        t = i / 14.0
        r = 2.4 + 5.2 * t
        angle = -0.5 - t * math.pi * 0.95
        x = r * math.cos(angle)
        y = r * math.sin(angle) * 0.70
        z = -t * 0.4
        pt = polyline2.bezier_points[i]
        pt.co = Vector((x, y, z))
        pt.handle_left_type = 'AUTO'
        pt.handle_right_type = 'AUTO'

    curve_obj2 = bpy.data.objects.new('TrailingTail', curve_data2)
    bpy.context.collection.objects.link(curve_obj2)
    curve_obj2.data.materials.append(create_emission_mat("TailMat", (1.45, 0.85, 0.35, 1.0), 4.5))

    # 5. Stream-Stream Self-Intersection Shock Knot
    shock_pos = Vector((-1.85, 0.95, 0.18))
    bpy.ops.mesh.primitive_uv_sphere_add(radius=0.28, segments=32, ring_count=16, location=shock_pos)
    shock_obj = bpy.context.active_object
    shock_obj.name = "ShockKnot"
    bpy.ops.object.shade_smooth()
    shock_obj.data.materials.append(create_emission_mat("ShockMat", (1.1, 1.35, 1.95, 1.0), 15.0))

    # 6. Secondary Bow Shock Knots
    for i, angle_offset in enumerate([-0.3, 0.35, 0.75]):
        kpos = shock_pos + Vector((math.cos(angle_offset)*0.45, math.sin(angle_offset)*0.45, 0.05))
        bpy.ops.mesh.primitive_uv_sphere_add(radius=0.12, segments=16, ring_count=8, location=kpos)
        kobj = bpy.context.active_object
        kobj.name = f"ShockRipple_{i}"
        bpy.ops.object.shade_smooth()
        kobj.data.materials.append(create_emission_mat(f"KnotMat_{i}", (0.7, 0.95, 1.5, 1.0), 8.0))

    # 7. Background Starfield of Host Galaxy (2MASX J04463790-1013349)
    np.random.seed(2019)
    for i in range(160):
        pos = np.random.uniform(-25.0, 25.0, 3)
        if np.linalg.norm(pos) < 9.0:
            pos *= 2.5
        size = np.random.uniform(0.04, 0.12)
        bpy.ops.mesh.primitive_uv_sphere_add(radius=size, location=tuple(pos))
        star = bpy.context.active_object
        col_t = np.random.uniform(0.0, 1.0)
        c = (0.7 + 0.3 * col_t, 0.85 + 0.15 * col_t, 1.0, 1.0)
        star.data.materials.append(create_emission_mat(f"Star_{i}", c, np.random.uniform(4.0, 10.0)))

    # 8. Camera setup
    cam_data = bpy.data.cameras.new("MainCamera")
    cam_data.lens = 52.0
    cam_obj = bpy.data.objects.new("MainCamera", cam_data)
    bpy.context.collection.objects.link(cam_obj)
    cam_obj.location = (0.0, -12.5, 4.8)
    cam_obj.rotation_euler = (math.radians(72), 0, 0)
    scene.camera = cam_obj

    # 9. Rim Light
    light_data = bpy.data.lights.new(name="RimLight", type='SUN')
    light_data.energy = 2.0
    light_data.color = (0.6, 0.85, 1.1)
    light_obj = bpy.data.objects.new("RimLight", light_data)
    bpy.context.collection.objects.link(light_obj)
    light_obj.rotation_euler = (math.radians(55.0), math.radians(20.0), math.radians(-35.0))

    return scene

def render_showcases():
    scene = build_scene()

    # 1. Hero Showcase (1920x1080)
    hero_path = os.path.join(DOCS_SCREENS, "tde_at2019qiz_hero.png")
    scene.render.resolution_x = 1920
    scene.render.resolution_y = 1080
    scene.render.filepath = hero_path
    print(f"[Blender] Rendering hero showcase: {hero_path}")
    bpy.ops.render.render(write_still=True)

    # 2. UI Preview Card (800x450)
    thumb_path = os.path.join(DOCS_SCREENS, "at2019qiz.png")
    scene.render.resolution_x = 800
    scene.render.resolution_y = 450
    scene.render.filepath = thumb_path
    print(f"[Blender] Rendering thumbnail showcase: {thumb_path}")
    bpy.ops.render.render(write_still=True)

    print("[Blender] Showcases rendered successfully!")

if __name__ == '__main__':
    render_showcases()
