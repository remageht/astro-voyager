"""Blender 5.2 Cycles renderer for Kilonova GW170817 / AT 2017gfo showcase.

Generates:
  docs/screens/gw170817.png
  docs/screens/kilonova_gw170817_hero.png
"""
import os
import sys
import math
import shutil
import numpy as np
import bpy
import bmesh
from mathutils import Vector, Matrix

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

def create_translucent_mat(name, color, alpha, roughness=0.1):
    mat = bpy.data.materials.new(name=name)
    mat.use_nodes = True
    nodes = mat.node_tree.nodes
    links = mat.node_tree.links
    nodes.clear()

    node_out = nodes.new(type='ShaderNodeOutputMaterial')
    node_bsdf = nodes.new(type='ShaderNodeBsdfPrincipled')
    node_bsdf.inputs['Base Color'].default_value = color
    node_bsdf.inputs['Emission Color'].default_value = color
    node_bsdf.inputs['Emission Strength'].default_value = 2.0
    node_bsdf.inputs['Alpha'].default_value = alpha
    node_bsdf.inputs['Roughness'].default_value = roughness
    links.new(node_bsdf.outputs['BSDF'], node_out.inputs['Surface'])
    return mat

def build_spacetime_gw_grid():
    # Spacetime gravitational wave ripples grid
    bpy.ops.mesh.primitive_grid_add(x_subdivisions=120, y_subdivisions=120, size=24.0)
    grid = bpy.context.active_object
    grid.name = "SpacetimeGWGrid"
    grid.location = (0, 0, -0.6)

    # Deform vertices by gravitational quadrupole waves: h ~ cos(k*r - w*t) * cos(2*theta) / sqrt(r)
    me = grid.data
    bm = bmesh.new()
    bm.from_mesh(me)

    for v in bm.verts:
        x, y = v.co.x, v.co.y
        r = math.hypot(x, y)
        if r > 0.05:
            theta = math.atan2(y, x)
            # Quadrupole wave envelope
            quad = math.cos(2.0 * theta)
            wave = math.sin(r * 2.2) * quad / math.sqrt(r + 0.4)
            # Concentric merger ripple
            v.co.z = wave * 0.45 - 0.8 / (r + 0.6)

    bm.to_mesh(me)
    bm.free()

    # Wireframe grid material
    mat = bpy.data.materials.new(name="SpacetimeGridMat")
    mat.use_nodes = True
    nodes = mat.node_tree.nodes
    links = mat.node_tree.links
    nodes.clear()
    out = nodes.new(type='ShaderNodeOutputMaterial')
    wire = nodes.new(type='ShaderNodeWireframe')
    wire.inputs['Size'].default_value = 0.02
    emi = nodes.new(type='ShaderNodeEmission')
    emi.inputs['Color'].default_value = (0.2, 0.65, 1.0, 1.0)
    emi.inputs['Strength'].default_value = 3.0
    trans = nodes.new(type='ShaderNodeBsdfTransparent')
    mix = nodes.new(type='ShaderNodeMixShader')
    links.new(wire.outputs['Fac'], mix.inputs['Fac'])
    links.new(trans.outputs['BSDF'], mix.inputs[1])
    links.new(emi.outputs['Emission'], mix.inputs[2])
    links.new(mix.outputs['Shader'], out.inputs['Surface'])
    grid.data.materials.append(mat)
    return grid

def build_kilonova_scene():
    reset_scene()

    # 1. Spacetime metric ripples
    build_spacetime_gw_grid()

    # 2. Concentric GW wave packets (luminous rings)
    for i in range(1, 6):
        r_ring = i * 1.8
        bpy.ops.mesh.primitive_torus_add(major_radius=r_ring, minor_radius=0.035 + i * 0.012, major_segments=96, minor_segments=16)
        ring = bpy.context.active_object
        ring.name = f"GWRing_{i}"
        ring.location = (0, 0, -0.6 + math.sin(i * 1.2) * 0.15)
        # Quadrupole brightness tint
        ring_mat = create_emission_mat(f"GWRingMat_{i}", (0.25, 0.70 + 0.05 * i, 1.0, 1.0), 4.5 / math.sqrt(i))
        ring.data.materials.append(ring_mat)

    # 3. Binary Neutron Star / Merger Remnant
    # NS1 (1.48 Msun)
    bpy.ops.mesh.primitive_uv_sphere_add(segments=48, ring_count=24, radius=0.32)
    ns1 = bpy.context.active_object
    ns1.name = "NeutronStar1"
    ns1.location = (-0.65, 0.15, -0.3)
    ns1_mat = create_emission_mat("NS1Mat", (0.85, 0.95, 1.4, 1.0), 18.0)
    ns1.data.materials.append(ns1_mat)

    # NS2 (1.26 Msun)
    bpy.ops.mesh.primitive_uv_sphere_add(segments=48, ring_count=24, radius=0.28)
    ns2 = bpy.context.active_object
    ns2.name = "NeutronStar2"
    ns2.location = (0.75, -0.18, -0.35)
    ns2_mat = create_emission_mat("NS2Mat", (0.70, 0.88, 1.35, 1.0), 15.0)
    ns2.data.materials.append(ns2_mat)

    # Central hypermassive merger flash core
    bpy.ops.mesh.primitive_uv_sphere_add(segments=48, ring_count=24, radius=0.55)
    core = bpy.context.active_object
    core.name = "MergerFlashCore"
    core.location = (0, 0, -0.3)
    core_mat = create_emission_mat("MergerFlashMat", (1.0, 0.98, 0.95, 1.0), 30.0)
    core.data.materials.append(core_mat)

    # 4. Relativistic Bipolar Jets (Short GRB 170817A)
    # North Jet (approaching)
    bpy.ops.mesh.primitive_cone_add(vertices=48, radius1=0.85, radius2=0.04, depth=5.5)
    jet_n = bpy.context.active_object
    jet_n.name = "RelativisticJetNorth"
    jet_n.location = (0.35, 0.25, 2.4)
    jet_n.rotation_euler = (math.radians(-15), math.radians(8), 0)
    jet_mat = create_emission_mat("JetMatNorth", (0.50, 0.85, 1.5, 1.0), 9.0)
    jet_n.data.materials.append(jet_mat)

    # South Jet (receding)
    bpy.ops.mesh.primitive_cone_add(vertices=48, radius1=0.75, radius2=0.03, depth=4.8)
    jet_s = bpy.context.active_object
    jet_s.name = "RelativisticJetSouth"
    jet_s.location = (-0.35, -0.25, -3.2)
    jet_s.rotation_euler = (math.radians(165), math.radians(-8), 0)
    jet_s_mat = create_emission_mat("JetMatSouth", (0.75, 0.35, 0.95, 1.0), 4.5)
    jet_s.data.materials.append(jet_s_mat)

    # 5. Expanding r-Process Nucleosynthesis Fireball (AT 2017gfo)
    # Equatorial Red Lanthanide Torus (Au, Pt, U synthesis)
    bpy.ops.mesh.primitive_torus_add(major_radius=2.2, minor_radius=0.95, major_segments=64, minor_segments=24)
    red_cloud = bpy.context.active_object
    red_cloud.name = "RedLanthanideCloud"
    red_cloud.location = (0, 0, -0.4)
    red_mat = create_translucent_mat("RedLanthanideMat", (1.35, 0.55, 0.15, 0.85), 0.75, 0.3)
    red_cloud.data.materials.append(red_mat)

    # Outer Crimson Shock Shell
    bpy.ops.mesh.primitive_torus_add(major_radius=3.4, minor_radius=0.75, major_segments=64, minor_segments=20)
    crimson_cloud = bpy.context.active_object
    crimson_cloud.name = "CrimsonShell"
    crimson_cloud.location = (0, 0, -0.45)
    crimson_mat = create_translucent_mat("CrimsonShellMat", (1.10, 0.20, 0.08, 0.70), 0.55, 0.4)
    crimson_cloud.data.materials.append(crimson_mat)

    # Polar Blue Photosphere Wind (fast polar ejecta)
    bpy.ops.mesh.primitive_uv_sphere_add(segments=48, ring_count=24, radius=1.6)
    blue_wind = bpy.context.active_object
    blue_wind.name = "BluePolarWind"
    blue_wind.scale = (1.1, 1.1, 1.8)
    blue_wind.location = (0, 0, -0.1)
    blue_mat = create_translucent_mat("BlueWindMat", (0.55, 0.80, 1.4, 0.65), 0.45, 0.2)
    blue_wind.data.materials.append(blue_mat)

    # 6. Lighting & Environment
    scene = bpy.context.scene
    scene.render.engine = 'CYCLES'
    scene.cycles.device = 'CPU'
    scene.render.resolution_x = 1280
    scene.render.resolution_y = 720
    scene.cycles.samples = 64
    scene.cycles.use_denoising = True

    # Dark space background with soft stars
    world = bpy.data.worlds.new("CosmicWorld")
    scene.world = world
    world.use_nodes = True
    nt = world.node_tree
    nt.nodes.clear()
    bg = nt.nodes.new("ShaderNodeBackground")
    bg.inputs["Color"].default_value = (0.002, 0.003, 0.007, 1.0)
    bg.inputs["Strength"].default_value = 0.5
    out = nt.nodes.new("ShaderNodeOutputWorld")
    nt.links.new(bg.outputs["Background"], out.inputs["Surface"])

    # Distant NGC 4993 Host Galaxy Billboard in background
    bpy.ops.mesh.primitive_plane_add(size=14.0)
    host = bpy.context.active_object
    host.name = "NGC4993Host"
    host.location = (-12.0, 18.0, 5.0)
    host.rotation_euler = (math.radians(35), math.radians(-25), math.radians(45))
    host_mat = create_translucent_mat("HostGalaxyMat", (0.95, 0.82, 0.65, 0.45), 0.35, 0.8)
    host.data.materials.append(host_mat)

    # 7. Camera (dramatic 3/4 perspective overlooking the entire merger & jets)
    cam_data = bpy.data.cameras.new("KilonovaCam")
    cam_data.lens = 38.0
    cam = bpy.data.objects.new("KilonovaCam", cam_data)
    cam.location = (10.5, -13.0, 5.5)
    cam.rotation_euler = (math.radians(72), math.radians(0), math.radians(38))
    scene.collection.objects.link(cam)
    scene.camera = cam

    # Key / Rim / Fill lighting
    key = bpy.data.lights.new("KeyLight", "AREA")
    key.energy = 550.0
    key.size = 6.0
    key.color = (1.1, 0.92, 0.70)
    ko = bpy.data.objects.new("KeyLight", key)
    ko.location = (6.0, -7.0, 7.0)
    scene.collection.objects.link(ko)

    rim = bpy.data.lights.new("RimLight", "AREA")
    rim.energy = 380.0
    rim.size = 5.0
    rim.color = (0.35, 0.80, 1.3)
    ro = bpy.data.objects.new("RimLight", rim)
    ro.location = (-7.0, 6.0, 4.0)
    scene.collection.objects.link(ro)

    # Render target 1: docs/screens/gw170817.png
    out1 = os.path.join(DOCS_SCREENS, "gw170817.png")
    scene.render.filepath = out1
    print(f"Rendering {out1}...")
    bpy.ops.render.render(write_still=True)
    print(f"Rendered: {out1}")

    # Render target 2: docs/screens/kilonova_gw170817_hero.png
    out2 = os.path.join(DOCS_SCREENS, "kilonova_gw170817_hero.png")
    shutil.copyfile(out1, out2)
    print(f"Copied to hero: {out2}")

if __name__ == "__main__":
    build_kilonova_scene()
