"""Blender 5.2 Cycles renderer for M16 Pillars of Creation showcase.

Generates:
  docs/screens/m16.png
  docs/screens/pillars_m16_hero.png
"""
import os
import sys
import math
import shutil
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

def create_dust_material(name, base_col, rim_col):
    mat = bpy.data.materials.new(name=name)
    mat.use_nodes = True
    nodes = mat.node_tree.nodes
    links = mat.node_tree.links
    nodes.clear()

    node_out = nodes.new(type='ShaderNodeOutputMaterial')
    node_mix = nodes.new(type='ShaderNodeMixShader')
    
    # Base dark molecular dust with warm subsurface
    node_bsdf = nodes.new(type='ShaderNodeBsdfPrincipled')
    node_bsdf.inputs['Base Color'].default_value = base_col
    node_bsdf.inputs['Roughness'].default_value = 0.90
    node_bsdf.inputs['Subsurface Weight'].default_value = 0.35
    node_bsdf.inputs['Subsurface Radius'].default_value = (1.0, 0.4, 0.15)

    # Glowing ionization rim
    node_fresnel = nodes.new(type='ShaderNodeFresnel')
    node_fresnel.inputs['IOR'].default_value = 1.45

    node_emi = nodes.new(type='ShaderNodeEmission')
    node_emi.inputs['Color'].default_value = rim_col
    node_emi.inputs['Strength'].default_value = 4.2

    links.new(node_fresnel.outputs['Fac'], node_mix.inputs['Fac'])
    links.new(node_bsdf.outputs['BSDF'], node_mix.inputs[1])
    links.new(node_emi.outputs['Emission'], node_mix.inputs[2])
    links.new(node_mix.outputs['Shader'], node_out.inputs['Surface'])
    return mat

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

def create_nebula_bg_mat():
    mat = bpy.data.materials.new(name="EagleNebulaBG")
    mat.use_nodes = True
    nodes = mat.node_tree.nodes
    links = mat.node_tree.links
    nodes.clear()

    node_out = nodes.new(type='ShaderNodeOutputMaterial')
    node_coord = nodes.new(type='ShaderNodeTexCoord')
    node_noise = nodes.new(type='ShaderNodeTexNoise')
    node_noise.inputs['Scale'].default_value = 1.8
    node_noise.inputs['Detail'].default_value = 6.0
    node_noise.inputs['Roughness'].default_value = 0.65

    node_ramp = nodes.new(type='ShaderNodeValToRGB')
    node_ramp.color_ramp.elements[0].position = 0.25
    node_ramp.color_ramp.elements[0].color = (0.015, 0.008, 0.025, 1.0)
    node_ramp.color_ramp.elements[1].position = 0.75
    node_ramp.color_ramp.elements[1].color = (0.28, 0.12, 0.22, 1.0)

    # Add teal high-ionization accent
    node_ramp.color_ramp.elements.new(0.92)
    node_ramp.color_ramp.elements[2].color = (0.12, 0.38, 0.45, 1.0)

    node_emi = nodes.new(type='ShaderNodeEmission')
    node_emi.inputs['Strength'].default_value = 1.8

    links.new(node_coord.outputs['Generated'], node_noise.inputs['Vector'])
    links.new(node_noise.outputs['Fac'], node_ramp.inputs['Fac'])
    links.new(node_ramp.outputs['Color'], node_emi.inputs['Color'])
    links.new(node_emi.outputs['Emission'], node_out.inputs['Surface'])
    return mat

def create_pillar_mesh(name, base_pos, height, radius_base, segments=32, rings=60):
    bm = bmesh.new()
    prev_verts = []
    
    for r in range(rings):
        t = r / float(rings - 1)
        z = t * height
        # Natural tapering to zero at the top
        profile = (1.0 - t * 0.85) * math.cos(t * 1.57079)
        curvature_x = math.sin(t * 2.8) * 0.6 * t
        curvature_y = math.cos(t * 2.0) * 0.25 * t

        curr_verts = []
        for s in range(segments):
            angle = s * 2.0 * math.pi / segments
            # Bumpy organic ridges
            ridge = 1.0 + 0.18 * math.sin(angle * 3.0 + t * 6.0) + 0.10 * math.cos(angle * 6.0)
            rad = radius_base * profile * ridge
            x = math.cos(angle) * rad + curvature_x + base_pos[0]
            y = math.sin(angle) * rad + curvature_y + base_pos[1]
            curr_verts.append(bm.verts.new((x, y, z + base_pos[2])))

        if prev_verts:
            for s in range(segments):
                s_next = (s + 1) % segments
                bm.faces.new((prev_verts[s], prev_verts[s_next], curr_verts[s_next], curr_verts[s]))
        prev_verts = curr_verts

    # Dome cap at the tip
    tip_z = base_pos[2] + height
    tip_vert = bm.verts.new((base_pos[0] + curvature_x, base_pos[1] + curvature_y, tip_z))
    for s in range(segments):
        s_next = (s + 1) % segments
        bm.faces.new((prev_verts[s], prev_verts[s_next], tip_vert))

    mesh = bpy.data.meshes.new(name)
    bm.to_mesh(mesh)
    bm.free()
    obj = bpy.data.objects.new(name, mesh)
    bpy.context.scene.collection.objects.link(obj)
    return obj

def build_scene():
    reset_scene()
    scene = bpy.context.scene
    scene.render.engine = 'CYCLES'
    scene.cycles.device = 'CPU'
    scene.cycles.samples = 64
    scene.cycles.preview_samples = 16
    scene.view_settings.view_transform = 'Filmic'
    scene.view_settings.look = 'High Contrast'
    scene.render.resolution_x = 1280
    scene.render.resolution_y = 720

    # World background
    world = bpy.data.worlds.new("SpaceWorld")
    world.use_nodes = True
    bg_node = world.node_tree.nodes['Background']
    bg_node.inputs['Color'].default_value = (0.005, 0.007, 0.015, 1.0)
    bg_node.inputs['Strength'].default_value = 1.0
    scene.world = world

    # Background nebula billboard
    bpy.ops.mesh.primitive_plane_add(size=35.0, location=(0.0, 8.0, 0.0), rotation=(math.radians(90.0), 0.0, 0.0))
    bg_plane = bpy.context.active_object
    bg_plane.name = "Nebula_Background"
    bg_plane.data.materials.append(create_nebula_bg_mat())

    # Pillar materials
    mat_dust_left = create_dust_material("PillarLeft", (0.045, 0.028, 0.018, 1.0), (0.22, 0.82, 0.92, 1.0))
    mat_dust_mid = create_dust_material("PillarMid", (0.040, 0.024, 0.015, 1.0), (0.95, 0.52, 0.20, 1.0))
    mat_dust_right = create_dust_material("PillarRight", (0.042, 0.026, 0.018, 1.0), (0.88, 0.38, 0.22, 1.0))

    # Pillar I (Left giant column, ~4 light years tall)
    p1 = create_pillar_mesh("Pillar_I", (-2.6, 0.0, -4.5), 8.2, 1.45)
    p1.data.materials.append(mat_dust_left)

    # Pillar II (Center spire)
    p2 = create_pillar_mesh("Pillar_II", (0.0, -0.3, -4.5), 6.5, 1.15)
    p2.data.materials.append(mat_dust_mid)

    # Pillar III (Right column)
    p3 = create_pillar_mesh("Pillar_III", (2.5, 0.3, -4.5), 5.0, 0.95)
    p3.data.materials.append(mat_dust_right)

    # Embedded protostars & EGG nodules at pillar tips
    egg_mat = create_emission_mat("ProtostarYSO", (1.0, 0.78, 0.40, 1.0), 45.0)
    egg_coords = [
        (-2.2, 0.2, 3.8),   # Tip of Pillar I
        (-2.6, 0.1, 3.4),   # Left EGG nodule
        (0.3, -0.2, 2.1),   # Tip of Pillar II
        (2.6, 0.4, 0.6),    # Tip of Pillar III
        (-1.8, -0.1, 1.5),  # Mid YSO
        (0.2, 0.0, -0.5)    # Embedded infant star
    ]
    for i, loc in enumerate(egg_coords):
        bpy.ops.mesh.primitive_uv_sphere_add(radius=0.10, location=loc)
        egg_obj = bpy.context.active_object
        egg_obj.name = f"Protostar_{i+1}"
        egg_obj.data.materials.append(egg_mat)

    # Hot massive O/B ionizing stars of cluster NGC 6611 (top-right)
    star_mat = create_emission_mat("ClusterStarBlue", (0.80, 0.92, 1.0, 1.0), 65.0)
    cluster_stars = [
        (4.2, 1.5, 4.5, 0.35),
        (5.5, 2.0, 5.2, 0.28),
        (3.4, 1.0, 5.8, 0.22),
        (6.0, 0.8, 3.8, 0.30),
        (-3.8, 2.5, 5.5, 0.18),
        (1.2, 2.2, 6.0, 0.20)
    ]
    for i, (sx, sy, sz, srad) in enumerate(cluster_stars):
        bpy.ops.mesh.primitive_uv_sphere_add(radius=srad, location=(sx, sy, sz))
        s_obj = bpy.context.active_object
        s_obj.name = f"ClusterStar_{i+1}"
        s_obj.data.materials.append(star_mat)

    # Direct UV Sun Light from NGC 6611 cluster (top right angled toward pillars)
    sun_data = bpy.data.lights.new(name="NGC6611_ClusterLight", type='SUN')
    sun_data.energy = 4.5
    sun_data.color = (0.78, 0.90, 1.0)
    sun_obj = bpy.data.objects.new(name="NGC6611_ClusterLight", object_data=sun_data)
    scene.collection.objects.link(sun_obj)
    sun_obj.rotation_euler = Euler((math.radians(52.0), math.radians(-38.0), math.radians(25.0)), 'XYZ')

    # Warm rim light from ionization front
    rim_data = bpy.data.lights.new(name="Ionization_RimLight", type='POINT')
    rim_data.energy = 140.0
    rim_data.color = (0.95, 0.55, 0.25)
    rim_obj = bpy.data.objects.new(name="Ionization_RimLight", object_data=rim_data)
    rim_obj.location = (1.5, -4.5, 1.5)
    scene.collection.objects.link(rim_obj)

    # Camera positioned to frame all three magnificent pillars
    cam_data = bpy.data.cameras.new(name="HeroCamera")
    cam_data.lens = 42
    cam_obj = bpy.data.objects.new(name="HeroCamera", object_data=cam_data)
    cam_obj.location = (0.1, -12.5, 0.2)
    cam_obj.rotation_euler = Euler((math.radians(88.5), 0.0, math.radians(0.8)), 'XYZ')
    scene.collection.objects.link(cam_obj)
    scene.camera = cam_obj

def render_showcase():
    build_scene()
    hero_path = os.path.join(DOCS_SCREENS, "pillars_m16_hero.png")
    m16_path = os.path.join(DOCS_SCREENS, "m16.png")

    bpy.context.scene.render.filepath = hero_path
    print(f"Rendering hero shot to {hero_path}...")
    bpy.ops.render.render(write_still=True)

    if os.path.exists(hero_path):
        shutil.copy2(hero_path, m16_path)
        print(f"Copied to {m16_path}")

        brain_dir = os.path.expanduser(r"~/.gemini/antigravity/brain/61717e81-a8ae-49ec-a1e1-565f37c911aa")
        if os.path.exists(brain_dir):
            brain_hero = os.path.join(brain_dir, "pillars_m16_hero.png")
            shutil.copy2(hero_path, brain_hero)
            print(f"Copied to brain artifact: {brain_hero}")

if __name__ == "__main__":
    render_showcase()
