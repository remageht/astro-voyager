"""Blender 5.2 Cycles renderer for Supernova SN 1987A showcase.

Generates:
  docs/screens/sn1987a.png
  docs/screens/supernova_sn1987a_hero.png
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

def create_translucent_mat(name, color, alpha, roughness=0.1, emi_strength=2.0):
    mat = bpy.data.materials.new(name=name)
    mat.use_nodes = True
    nodes = mat.node_tree.nodes
    links = mat.node_tree.links
    nodes.clear()

    node_out = nodes.new(type='ShaderNodeOutputMaterial')
    node_bsdf = nodes.new(type='ShaderNodeBsdfPrincipled')
    node_bsdf.inputs['Base Color'].default_value = color
    node_bsdf.inputs['Emission Color'].default_value = color
    node_bsdf.inputs['Emission Strength'].default_value = emi_strength
    node_bsdf.inputs['Alpha'].default_value = alpha
    node_bsdf.inputs['Roughness'].default_value = roughness
    links.new(node_bsdf.outputs['BSDF'], node_out.inputs['Surface'])
    return mat

def build_equatorial_ring():
    # Central equatorial ring (R=2.8, minor radius=0.08)
    bpy.ops.mesh.primitive_torus_add(
        major_radius=2.8,
        minor_radius=0.09,
        major_segments=96,
        minor_segments=24
    )
    ring = bpy.context.active_object
    ring.name = "EquatorialRing"
    mat = create_emission_mat("ER_BaseMat", (0.95, 0.35, 0.18, 1.0), 3.5)
    ring.data.materials.append(mat)
    return ring

def build_string_of_pearls(parent):
    # 32 hot spots / pearls along the equatorial ring
    pearl_mat_hot = create_emission_mat("PearlHotMat", (1.4, 1.25, 1.0, 1.0), 12.0)
    pearl_mat_warm = create_emission_mat("PearlWarmMat", (1.2, 0.85, 0.5, 1.0), 8.5)

    num_pearls = 32
    r = 2.8
    for i in range(num_pearls):
        theta = (i / num_pearls) * 2.0 * math.pi
        # Realistic positional jitter
        theta += (np.sin(i * 3.7) * 0.05)
        pr = r + np.cos(i * 5.1) * 0.06
        px = pr * math.cos(theta)
        py = pr * math.sin(theta)
        pz = np.sin(i * 7.3) * 0.04

        size = 0.11 + np.sin(i * 2.3) * 0.03
        bpy.ops.mesh.primitive_ico_sphere_add(subdivisions=2, radius=size, location=(px, py, pz))
        pearl = bpy.context.active_object
        pearl.name = f"Pearl_{i:02d}"
        pearl.parent = parent

        mat = pearl_mat_hot if (i % 3 != 0) else pearl_mat_warm
        pearl.data.materials.append(mat)

def build_outer_rings():
    # Outer upper ring
    bpy.ops.mesh.primitive_torus_add(
        major_radius=4.2,
        minor_radius=0.05,
        major_segments=96,
        minor_segments=16,
        location=(0.3, 0.0, 2.3)
    )
    upper = bpy.context.active_object
    upper.name = "OuterRing_Upper"
    mat_upper = create_emission_mat("UpperRingMat", (0.45, 0.70, 1.15, 1.0), 2.2)
    upper.data.materials.append(mat_upper)

    # Outer lower ring
    bpy.ops.mesh.primitive_torus_add(
        major_radius=4.2,
        minor_radius=0.05,
        major_segments=96,
        minor_segments=16,
        location=(-0.3, 0.0, -2.3)
    )
    lower = bpy.context.active_object
    lower.name = "OuterRing_Lower"
    mat_lower = create_emission_mat("LowerRingMat", (0.65, 0.50, 1.05, 1.0), 2.0)
    lower.data.materials.append(mat_lower)

def build_keyhole_debris():
    # Inner radioactive debris ejecta (JWST keyhole shape)
    bpy.ops.mesh.primitive_ico_sphere_add(subdivisions=3, radius=1.05, location=(0.08, 0.05, 0.0))
    debris = bpy.context.active_object
    debris.name = "KeyholeDebris"
    debris.scale = (1.2, 0.75, 0.5)

    # Displace vertices to form keyhole / peanut crescent
    me = debris.data
    bm = bmesh.new()
    bm.from_mesh(me)
    for v in bm.verts:
        d = math.hypot(v.co.x, v.co.y)
        # Notch cutout
        notch = math.exp(-((v.co.x - 0.3)**2 + (v.co.y - 0.2)**2) / 0.4)
        v.co *= (1.0 - 0.45 * notch)
        # Turbulence
        v.co.z *= (1.0 + 0.2 * math.sin(v.co.x * 5.0))
    bm.to_mesh(me)
    bm.free()

    mat = create_translucent_mat("DebrisMat", (1.1, 0.65, 0.35, 1.0), 0.75, roughness=0.3, emi_strength=4.5)
    debris.data.materials.append(mat)

    # Central compact remnant (neutron star / PWN)
    bpy.ops.mesh.primitive_ico_sphere_add(subdivisions=2, radius=0.12, location=(0, 0, 0))
    core = bpy.context.active_object
    core.name = "CompactRemnantCore"
    core_mat = create_emission_mat("CoreMat", (0.8, 1.2, 2.0, 1.0), 20.0)
    core.data.materials.append(core_mat)

def build_shockwave():
    # Expanding blast wave sphere
    bpy.ops.mesh.primitive_ico_sphere_add(subdivisions=4, radius=3.2, location=(0, 0, 0))
    shock = bpy.context.active_object
    shock.name = "BlastShockWave"
    shock.scale = (1.0, 1.0, 0.85)
    mat = create_translucent_mat("ShockMat", (0.25, 0.55, 0.95, 1.0), 0.12, roughness=0.1, emi_strength=1.8)
    shock.data.materials.append(mat)

def build_companion_stars():
    # Companion stars 2 and 3
    bpy.ops.mesh.primitive_ico_sphere_add(subdivisions=2, radius=0.15, location=(3.8, 2.5, 0.2))
    s2 = bpy.context.active_object
    s2.name = "Star2"
    s2.data.materials.append(create_emission_mat("Star2Mat", (0.9, 1.0, 1.2, 1.0), 15.0))

    bpy.ops.mesh.primitive_ico_sphere_add(subdivisions=2, radius=0.13, location=(-3.5, -2.8, -0.1))
    s3 = bpy.context.active_object
    s3.name = "Star3"
    s3.data.materials.append(create_emission_mat("Star3Mat", (1.1, 1.0, 0.8, 1.0), 12.0))

def build_background_starfield():
    np.random.seed(1987)
    num_stars = 350
    coords = []
    for _ in range(num_stars):
        dist = np.random.uniform(22.0, 38.0)
        u = np.random.uniform(-1.0, 1.0)
        phi = np.random.uniform(0.0, 2.0 * math.pi)
        r_xy = math.sqrt(max(0.0, 1.0 - u * u))
        coords.append((dist * r_xy * math.cos(phi), dist * r_xy * math.sin(phi), dist * u))

    mesh = bpy.data.meshes.new("LMC_Stars")
    obj = bpy.data.objects.new("LMC_Starfield", mesh)
    bpy.context.collection.objects.link(obj)

    bm = bmesh.new()
    for pt in coords:
        bm.verts.new(pt)
    bm.to_mesh(mesh)
    bm.free()

    mat = create_emission_mat("StarfieldMat", (1.0, 0.95, 0.9, 1.0), 5.0)
    obj.data.materials.append(mat)

def setup_camera_and_lights():
    # Camera matching HST/JWST inclination ~43 deg to ring plane
    cam_data = bpy.data.cameras.new("ShowcaseCam")
    cam_data.lens = 65.0
    cam_obj = bpy.data.objects.new("ShowcaseCam", cam_data)
    bpy.context.collection.objects.link(cam_obj)

    # Position camera looking down at ~43 deg
    cam_obj.location = (0.0, -11.5, 10.8)
    cam_obj.rotation_euler = Euler((math.radians(47.0), 0.0, 0.0), 'XYZ')
    bpy.context.scene.camera = cam_obj

    # Key light: warm peach and cool cyan rim lights
    light_data = bpy.data.lights.new(name="RimLight", type='SUN')
    light_data.energy = 2.5
    light_data.color = (0.6, 0.8, 1.0)
    light_obj = bpy.data.objects.new("RimLight", light_data)
    bpy.context.collection.objects.link(light_obj)
    light_obj.rotation_euler = Euler((math.radians(60.0), math.radians(25.0), math.radians(-40.0)), 'XYZ')

def setup_render_engine(output_path):
    scene = bpy.context.scene
    scene.render.engine = 'CYCLES'
    scene.render.resolution_x = 1280
    scene.render.resolution_y = 720
    scene.render.resolution_percentage = 100
    scene.render.filepath = output_path
    scene.render.image_settings.file_format = 'PNG'
    scene.render.image_settings.color_mode = 'RGBA'

    # World background dark space
    world = bpy.data.worlds.new("SpaceWorld")
    world.use_nodes = True
    bg = world.node_tree.nodes.get("Background")
    if bg:
        bg.inputs["Color"].default_value = (0.002, 0.003, 0.007, 1.0)
        bg.inputs["Strength"].default_value = 1.0
    scene.world = world

    # Cycles quality settings
    cscene = scene.cycles
    cscene.samples = 32
    cscene.preview_samples = 16
    cscene.use_denoising = True

    prefs = bpy.context.preferences.addons['cycles'].preferences
    prefs.refresh_devices()
    for dev in prefs.devices:
        dev.use = True
    cscene.device = 'GPU' if any(d.type in ('CUDA', 'OPTIX', 'METAL', 'HIP', 'ONEAPI') for d in prefs.devices) else 'CPU'

def main():
    print("Building Supernova SN 1987A showcase scene...")
    reset_scene()

    er = build_equatorial_ring()
    build_string_of_pearls(er)
    build_outer_rings()
    build_keyhole_debris()
    build_shockwave()
    build_companion_stars()
    build_background_starfield()

    setup_camera_and_lights()

    hero_png = os.path.join(DOCS_SCREENS, "supernova_sn1987a_hero.png")
    setup_render_engine(hero_png)

    print(f"Rendering SN 1987A hero showcase to {hero_png}...")
    bpy.ops.render.render(write_still=True)

    sn_png = os.path.join(DOCS_SCREENS, "sn1987a.png")
    shutil.copyfile(hero_png, sn_png)
    print(f"Copied showcase to {sn_png}")
    print("SN 1987A showcase rendering completed successfully!")

if __name__ == "__main__":
    main()
