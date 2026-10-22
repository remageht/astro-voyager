"""Deterministic PBR 3D asset generation and baking for astro-voyager:
- International Space Station (ISS) Model (Cycles Quality):
  * Integrated Truss Structure (ITS: S0, S1, P1, S3/S4, P3/P4, S5/S6, P5/P6)
  * Solar Alpha Rotary Joints (SARJ) with rotating collar detailing
  * 8 Photovoltaic Solar Array Wings (SAW) on articulated gimbals with high-detail cell textures
  * Active Thermal Control System (ATCS) accordion radiator arrays with corrugated micro-fins
  * Pressurized Module Cluster:
    - USOS: Destiny Lab, Unity (Node 1), Harmony (Node 2), Tranquility (Node 3), Columbus (ESA), Kibo (JAXA) + Exposed Facility porch, Quest Airlock with external gas tanks
    - ROS: Zvezda Service Module with stepped cylinders and solar wings, Zarya (FGB) with angled panels, Poisk/Rassvet/Nauka docking compartments
  * Earth-facing 7-window Cupola observation dome with warm interior cabin glow
  * International Docking Adapter (IDA-2 / PMA-2) with laser retroreflectors and docking target lights
  * Canadarm2 (SSRMS) mobile manipulator robotic arm and High-Gain Communications Dish
  * Navigation beacons (Port Red, Starboard Green, Zenith/Nadir White Strobes)

Produces:
  res/models/station.obj
  res/models/station_albedo.bmp, station_normal.bmp, station_rough.bmp, station_metal.bmp, station_emissive.bmp
  blender/out/final_showcase/station_showcase.png  (1280x720 Cycles)
  blender/out/final_showcase/station_close.png     (1280x720 Cycles)
  docs/screens/station_showcase.png
  docs/screens/station_close.png
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
PROJECT_ROOT = os.path.dirname(SCRIPT_DIR)  # blender directory
REPO_ROOT = os.path.dirname(PROJECT_ROOT)   # astro-voyager repository root
RES_MODELS = os.path.join(REPO_ROOT, "res", "models")
OUT_SHOWCASE = os.path.join(PROJECT_ROOT, "out", "final_showcase")
DOCS_SCREENS = os.path.join(REPO_ROOT, "docs", "screens")
BUILD_GL_MODELS = os.path.join(REPO_ROOT, "build-gl", "Release", "res", "models")
BUILD_MODELS = os.path.join(REPO_ROOT, "build", "Release", "res", "models")

os.makedirs(RES_MODELS, exist_ok=True)
os.makedirs(OUT_SHOWCASE, exist_ok=True)
os.makedirs(DOCS_SCREENS, exist_ok=True)
os.makedirs(BUILD_GL_MODELS, exist_ok=True)
os.makedirs(BUILD_MODELS, exist_ok=True)

# ---------------------------------------------------------------------------
# 24-bit uncompressed BMP writer matching src/ObjModel.cpp
# ---------------------------------------------------------------------------
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
    
    bgr = u8[:, :, [2, 1, 0]]
    if padding == 0:
        data = bgr.tobytes()
    else:
        pad = b'\x00' * padding
        rows = [bgr[y, :, :].tobytes() + pad for y in range(H)]
        data = b''.join(rows)
        
    with open(filepath, 'wb') as f:
        f.write(header)
        f.write(data)
    print(f"Saved BMP: {os.path.basename(filepath)} ({W}x{H}, {file_size / (1024 * 1024):.2f} MB)")


def save_blender_image_as_bmp(img, filepath):
    W, H = img.size
    pixels = np.array(img.pixels[:], dtype=np.float32).reshape(H, W, 4)
    rgb = pixels[:, :, :3]
    rgb_flipped = np.flipud(rgb)
    write_bmp24(filepath, rgb_flipped)


# ---------------------------------------------------------------------------
# Procedural PBR Material Shaders
# ---------------------------------------------------------------------------
def make_iss_material(name, mat_type):
    mat = bpy.data.materials.new(name)
    mat.use_nodes = True
    nodes = mat.node_tree.nodes
    links = mat.node_tree.links
    nodes.clear()
    
    out = nodes.new("ShaderNodeOutputMaterial")
    bsdf = nodes.new("ShaderNodeBsdfPrincipled")
    links.new(bsdf.outputs["BSDF"], out.inputs["Surface"])
    
    tex_coord = nodes.new("ShaderNodeTexCoord")
    
    # Store references for baking passes
    bake_color = None
    bake_rough = None
    bake_metal = None
    bake_emissive = None

    if mat_type == 'SOLAR_ARRAY':
        # Dark indigo photovoltaic cells with bright copper/gold busbars and silver grid
        brick = nodes.new("ShaderNodeTexBrick")
        brick.inputs["Scale"].default_value = 85.0
        brick.inputs["Mortar Size"].default_value = 0.04
        brick.inputs["Mortar Smooth"].default_value = 0.15
        brick.inputs["Color1"].default_value = (0.020, 0.045, 0.110, 1.0)  # Deep silicon blue
        brick.inputs["Color2"].default_value = (0.028, 0.055, 0.135, 1.0)  # Slightly varied cell
        brick.inputs["Mortar"].default_value = (0.850, 0.620, 0.200, 1.0)  # Gold/copper busbar
        links.new(tex_coord.outputs["UV"], brick.inputs["Vector"])
        
        # Color mix with central busbar runner
        wave = nodes.new("ShaderNodeTexWave")
        wave.wave_type = 'BANDS'
        wave.bands_direction = 'X'
        wave.inputs["Scale"].default_value = 18.0
        links.new(tex_coord.outputs["UV"], wave.inputs["Vector"])
        
        mix_col = nodes.new("ShaderNodeMixRGB")
        mix_col.blend_type = 'MIX'
        mix_col.inputs["Fac"].default_value = 0.18
        links.new(brick.outputs["Color"], mix_col.inputs["Color1"])
        links.new(wave.outputs["Color"], mix_col.inputs["Color2"])
        
        links.new(mix_col.outputs["Color"], bsdf.inputs["Base Color"])
        bake_color = mix_col.outputs["Color"]
        
        # Roughness: cells are glossy glass (0.12), busbars are satin (0.32)
        val_to_rgb_r = nodes.new("ShaderNodeValToRGB")
        val_to_rgb_r.color_ramp.elements[0].position = 0.0
        val_to_rgb_r.color_ramp.elements[0].color = (0.12, 0.12, 0.12, 1.0)
        val_to_rgb_r.color_ramp.elements[1].position = 1.0
        val_to_rgb_r.color_ramp.elements[1].color = (0.35, 0.35, 0.35, 1.0)
        links.new(brick.outputs["Fac"], val_to_rgb_r.inputs["Fac"])
        links.new(val_to_rgb_r.outputs["Color"], bsdf.inputs["Roughness"])
        bake_rough = val_to_rgb_r.outputs["Color"]
        
        # Metallic: busbars are metallic 0.92, cells are 0.35
        val_to_rgb_m = nodes.new("ShaderNodeValToRGB")
        val_to_rgb_m.color_ramp.elements[0].position = 0.0
        val_to_rgb_m.color_ramp.elements[0].color = (0.35, 0.35, 0.35, 1.0)
        val_to_rgb_m.color_ramp.elements[1].position = 1.0
        val_to_rgb_m.color_ramp.elements[1].color = (0.92, 0.92, 0.92, 1.0)
        links.new(brick.outputs["Fac"], val_to_rgb_m.inputs["Fac"])
        links.new(val_to_rgb_m.outputs["Color"], bsdf.inputs["Metallic"])
        bake_metal = val_to_rgb_m.outputs["Color"]
        
        # Bump: cell bevels
        bump = nodes.new("ShaderNodeBump")
        bump.inputs["Strength"].default_value = 0.65
        bump.inputs["Distance"].default_value = 0.02
        links.new(brick.outputs["Fac"], bump.inputs["Height"])
        links.new(bump.outputs["Normal"], bsdf.inputs["Normal"])

    elif mat_type == 'MODULE_HULL':
        # White/light-gray Beta cloth thermal insulation with anodized aluminum stringer belts
        voronoi = nodes.new("ShaderNodeTexVoronoi")
        voronoi.feature = 'DISTANCE_TO_EDGE'
        voronoi.inputs["Scale"].default_value = 35.0
        links.new(tex_coord.outputs["UV"], voronoi.inputs["Vector"])
        
        val_to_rgb_c = nodes.new("ShaderNodeValToRGB")
        val_to_rgb_c.color_ramp.elements[0].position = 0.02
        val_to_rgb_c.color_ramp.elements[0].color = (0.32, 0.35, 0.40, 1.0)  # Dark panel seam
        val_to_rgb_c.color_ramp.elements[1].position = 0.08
        val_to_rgb_c.color_ramp.elements[1].color = (0.88, 0.89, 0.91, 1.0)  # White/silver cloth
        links.new(voronoi.outputs["Distance"], val_to_rgb_c.inputs["Fac"])
        
        links.new(val_to_rgb_c.outputs["Color"], bsdf.inputs["Base Color"])
        bake_color = val_to_rgb_c.outputs["Color"]
        
        # Roughness: cloth is matte (0.60), seams are smooth (0.25)
        val_to_rgb_r = nodes.new("ShaderNodeValToRGB")
        val_to_rgb_r.color_ramp.elements[0].position = 0.03
        val_to_rgb_r.color_ramp.elements[0].color = (0.25, 0.25, 0.25, 1.0)
        val_to_rgb_r.color_ramp.elements[1].position = 0.10
        val_to_rgb_r.color_ramp.elements[1].color = (0.58, 0.58, 0.58, 1.0)
        links.new(voronoi.outputs["Distance"], val_to_rgb_r.inputs["Fac"])
        links.new(val_to_rgb_r.outputs["Color"], bsdf.inputs["Roughness"])
        bake_rough = val_to_rgb_r.outputs["Color"]
        
        # Metallic: 0.15
        rgb_m = nodes.new("ShaderNodeRGB")
        rgb_m.outputs["Color"].default_value = (0.15, 0.15, 0.15, 1.0)
        links.new(rgb_m.outputs["Color"], bsdf.inputs["Metallic"])
        bake_metal = rgb_m.outputs["Color"]
        
        bump = nodes.new("ShaderNodeBump")
        bump.inputs["Strength"].default_value = 0.45
        bump.inputs["Distance"].default_value = 0.015
        links.new(voronoi.outputs["Distance"], bump.inputs["Height"])
        links.new(bump.outputs["Normal"], bsdf.inputs["Normal"])

    elif mat_type == 'RADIATOR':
        # Accordion-folded corrugated thermal radiators with white high-emittance coating
        wave = nodes.new("ShaderNodeTexWave")
        wave.wave_type = 'BANDS'
        wave.bands_direction = 'X'
        wave.inputs["Scale"].default_value = 50.0
        links.new(tex_coord.outputs["UV"], wave.inputs["Vector"])
        
        val_to_rgb_c = nodes.new("ShaderNodeValToRGB")
        val_to_rgb_c.color_ramp.elements[0].position = 0.15
        val_to_rgb_c.color_ramp.elements[0].color = (0.75, 0.77, 0.80, 1.0)  # Shadow crevice
        val_to_rgb_c.color_ramp.elements[1].position = 0.85
        val_to_rgb_c.color_ramp.elements[1].color = (0.95, 0.96, 0.98, 1.0)  # White radiator fin
        links.new(wave.outputs["Color"], val_to_rgb_c.inputs["Fac"])
        
        links.new(val_to_rgb_c.outputs["Color"], bsdf.inputs["Base Color"])
        bake_color = val_to_rgb_c.outputs["Color"]
        
        rgb_r = nodes.new("ShaderNodeRGB")
        rgb_r.outputs["Color"].default_value = (0.45, 0.45, 0.45, 1.0)
        links.new(rgb_r.outputs["Color"], bsdf.inputs["Roughness"])
        bake_rough = rgb_r.outputs["Color"]
        
        rgb_m = nodes.new("ShaderNodeRGB")
        rgb_m.outputs["Color"].default_value = (0.10, 0.10, 0.10, 1.0)
        links.new(rgb_m.outputs["Color"], bsdf.inputs["Metallic"])
        bake_metal = rgb_m.outputs["Color"]
        
        bump = nodes.new("ShaderNodeBump")
        bump.inputs["Strength"].default_value = 0.85
        bump.inputs["Distance"].default_value = 0.03
        links.new(wave.outputs["Color"], bump.inputs["Height"])
        links.new(bump.outputs["Normal"], bsdf.inputs["Normal"])

    elif mat_type == 'GOLD_MLI':
        # Wrinkled Kapton / aluminized Mylar gold insulation
        noise = nodes.new("ShaderNodeTexNoise")
        noise.inputs["Scale"].default_value = 45.0
        noise.inputs["Detail"].default_value = 6.0
        noise.inputs["Roughness"].default_value = 0.7
        links.new(tex_coord.outputs["UV"], noise.inputs["Vector"])
        
        val_to_rgb_c = nodes.new("ShaderNodeValToRGB")
        val_to_rgb_c.color_ramp.elements[0].position = 0.3
        val_to_rgb_c.color_ramp.elements[0].color = (0.88, 0.62, 0.12, 1.0)  # Amber gold
        val_to_rgb_c.color_ramp.elements[1].position = 0.7
        val_to_rgb_c.color_ramp.elements[1].color = (0.98, 0.78, 0.22, 1.0)  # Bright gold foil
        links.new(noise.outputs["Fac"], val_to_rgb_c.inputs["Fac"])
        
        links.new(val_to_rgb_c.outputs["Color"], bsdf.inputs["Base Color"])
        bake_color = val_to_rgb_c.outputs["Color"]
        
        rgb_r = nodes.new("ShaderNodeRGB")
        rgb_r.outputs["Color"].default_value = (0.22, 0.22, 0.22, 1.0)
        links.new(rgb_r.outputs["Color"], bsdf.inputs["Roughness"])
        bake_rough = rgb_r.outputs["Color"]
        
        rgb_m = nodes.new("ShaderNodeRGB")
        rgb_m.outputs["Color"].default_value = (0.94, 0.94, 0.94, 1.0)
        links.new(rgb_m.outputs["Color"], bsdf.inputs["Metallic"])
        bake_metal = rgb_m.outputs["Color"]
        
        bump = nodes.new("ShaderNodeBump")
        bump.inputs["Strength"].default_value = 0.6
        bump.inputs["Distance"].default_value = 0.02
        links.new(noise.outputs["Fac"], bump.inputs["Height"])
        links.new(bump.outputs["Normal"], bsdf.inputs["Normal"])

    elif mat_type == 'TRUSS':
        # Dark anodized structural alloy for ITS truss framework
        rgb_c = nodes.new("ShaderNodeRGB")
        rgb_c.outputs["Color"].default_value = (0.18, 0.20, 0.24, 1.0)
        links.new(rgb_c.outputs["Color"], bsdf.inputs["Base Color"])
        bake_color = rgb_c.outputs["Color"]
        
        rgb_r = nodes.new("ShaderNodeRGB")
        rgb_r.outputs["Color"].default_value = (0.35, 0.35, 0.35, 1.0)
        links.new(rgb_r.outputs["Color"], bsdf.inputs["Roughness"])
        bake_rough = rgb_r.outputs["Color"]
        
        rgb_m = nodes.new("ShaderNodeRGB")
        rgb_m.outputs["Color"].default_value = (0.88, 0.88, 0.88, 1.0)
        links.new(rgb_m.outputs["Color"], bsdf.inputs["Metallic"])
        bake_metal = rgb_m.outputs["Color"]

    elif mat_type == 'DOCK':
        # Titanium docking collar and alignment target
        rgb_c = nodes.new("ShaderNodeRGB")
        rgb_c.outputs["Color"].default_value = (0.42, 0.45, 0.50, 1.0)
        links.new(rgb_c.outputs["Color"], bsdf.inputs["Base Color"])
        bake_color = rgb_c.outputs["Color"]
        
        rgb_r = nodes.new("ShaderNodeRGB")
        rgb_r.outputs["Color"].default_value = (0.30, 0.30, 0.30, 1.0)
        links.new(rgb_r.outputs["Color"], bsdf.inputs["Roughness"])
        bake_rough = rgb_r.outputs["Color"]
        
        rgb_m = nodes.new("ShaderNodeRGB")
        rgb_m.outputs["Color"].default_value = (0.80, 0.80, 0.80, 1.0)
        links.new(rgb_m.outputs["Color"], bsdf.inputs["Metallic"])
        bake_metal = rgb_m.outputs["Color"]

    elif mat_type == 'CUPOLA':
        # Observation dome: deep sapphire glass with warm amber interior cabin light
        rgb_c = nodes.new("ShaderNodeRGB")
        rgb_c.outputs["Color"].default_value = (0.04, 0.10, 0.22, 1.0)
        links.new(rgb_c.outputs["Color"], bsdf.inputs["Base Color"])
        bake_color = rgb_c.outputs["Color"]
        
        rgb_r = nodes.new("ShaderNodeRGB")
        rgb_r.outputs["Color"].default_value = (0.08, 0.08, 0.08, 1.0)
        links.new(rgb_r.outputs["Color"], bsdf.inputs["Roughness"])
        bake_rough = rgb_r.outputs["Color"]
        
        rgb_m = nodes.new("ShaderNodeRGB")
        rgb_m.outputs["Color"].default_value = (0.15, 0.15, 0.15, 1.0)
        links.new(rgb_m.outputs["Color"], bsdf.inputs["Metallic"])
        bake_metal = rgb_m.outputs["Color"]
        
        # Warm golden cabin glow
        bsdf.inputs["Emission Color"].default_value = (1.0, 0.82, 0.50, 1.0)
        bsdf.inputs["Emission Strength"].default_value = 4.0
        
        rgb_e = nodes.new("ShaderNodeRGB")
        rgb_e.outputs["Color"].default_value = (1.0, 0.82, 0.50, 1.0)
        bake_emissive = rgb_e.outputs["Color"]

    elif mat_type == 'NAV_RED':
        # Port nav beacon
        rgb_c = nodes.new("ShaderNodeRGB")
        rgb_c.outputs["Color"].default_value = (1.0, 0.05, 0.05, 1.0)
        links.new(rgb_c.outputs["Color"], bsdf.inputs["Base Color"])
        bake_color = rgb_c.outputs["Color"]
        bake_rough = rgb_c.outputs["Color"]
        bake_metal = rgb_c.outputs["Color"]
        
        bsdf.inputs["Emission Color"].default_value = (1.0, 0.05, 0.05, 1.0)
        bsdf.inputs["Emission Strength"].default_value = 8.0
        bake_emissive = rgb_c.outputs["Color"]

    elif mat_type == 'NAV_GREEN':
        # Starboard nav beacon
        rgb_c = nodes.new("ShaderNodeRGB")
        rgb_c.outputs["Color"].default_value = (0.05, 1.0, 0.15, 1.0)
        links.new(rgb_c.outputs["Color"], bsdf.inputs["Base Color"])
        bake_color = rgb_c.outputs["Color"]
        bake_rough = rgb_c.outputs["Color"]
        bake_metal = rgb_c.outputs["Color"]
        
        bsdf.inputs["Emission Color"].default_value = (0.05, 1.0, 0.15, 1.0)
        bsdf.inputs["Emission Strength"].default_value = 8.0
        bake_emissive = rgb_c.outputs["Color"]

    elif mat_type == 'STROBE_WHITE':
        # Docking / zenith strobe beacon
        rgb_c = nodes.new("ShaderNodeRGB")
        rgb_c.outputs["Color"].default_value = (1.0, 1.0, 1.0, 1.0)
        links.new(rgb_c.outputs["Color"], bsdf.inputs["Base Color"])
        bake_color = rgb_c.outputs["Color"]
        bake_rough = rgb_c.outputs["Color"]
        bake_metal = rgb_c.outputs["Color"]
        
        bsdf.inputs["Emission Color"].default_value = (1.0, 1.0, 1.0, 1.0)
        bsdf.inputs["Emission Strength"].default_value = 8.0
        bake_emissive = rgb_c.outputs["Color"]

    # Emission node used exclusively during bake passes
    bake_emit = nodes.new("ShaderNodeEmission")
    bake_emit.name = "BakeEmission"
    
    # Store references for fast switching
    mat["bake_color_node"] = bake_color.node.name if bake_color else None
    mat["bake_rough_node"] = bake_rough.node.name if bake_rough else None
    mat["bake_metal_node"] = bake_metal.node.name if bake_metal else None
    mat["bake_emissive_node"] = bake_emissive.node.name if bake_emissive else None
    
    return mat


def set_material_pass(mat, pass_type):
    nodes = mat.node_tree.nodes
    links = mat.node_tree.links
    out = nodes.get("Material Output")
    bsdf = nodes.get("Principled BSDF")
    emit = nodes.get("BakeEmission")
    
    if pass_type == 'PBR':
        links.new(bsdf.outputs["BSDF"], out.inputs["Surface"])
        return
        
    if pass_type == 'ALBEDO':
        node_name = mat.get("bake_color_node")
        if node_name and node_name in nodes:
            src = nodes[node_name].outputs.get("Color") or nodes[node_name].outputs[0]
            links.new(src, emit.inputs["Color"])
            emit.inputs["Strength"].default_value = 1.0
        else:
            emit.inputs["Color"].default_value = (0.5, 0.5, 0.5, 1.0)
        links.new(emit.outputs["Emission"], out.inputs["Surface"])
        
    elif pass_type == 'ROUGHNESS':
        node_name = mat.get("bake_rough_node")
        if node_name and node_name in nodes:
            src = nodes[node_name].outputs.get("Color") or nodes[node_name].outputs[0]
            links.new(src, emit.inputs["Color"])
            emit.inputs["Strength"].default_value = 1.0
        else:
            emit.inputs["Color"].default_value = (0.5, 0.5, 0.5, 1.0)
        links.new(emit.outputs["Emission"], out.inputs["Surface"])
        
    elif pass_type == 'METALLIC':
        node_name = mat.get("bake_metal_node")
        if node_name and node_name in nodes:
            src = nodes[node_name].outputs.get("Color") or nodes[node_name].outputs[0]
            links.new(src, emit.inputs["Color"])
            emit.inputs["Strength"].default_value = 1.0
        else:
            emit.inputs["Color"].default_value = (0.0, 0.0, 0.0, 1.0)
        links.new(emit.outputs["Emission"], out.inputs["Surface"])
        
    elif pass_type == 'EMISSIVE':
        node_name = mat.get("bake_emissive_node")
        if node_name and node_name in nodes:
            src = nodes[node_name].outputs.get("Color") or nodes[node_name].outputs[0]
            links.new(src, emit.inputs["Color"])
            emit.inputs["Strength"].default_value = 1.0
        else:
            emit.inputs["Color"].default_value = (0.0, 0.0, 0.0, 1.0)  # Pure black
        links.new(emit.outputs["Emission"], out.inputs["Surface"])


# ---------------------------------------------------------------------------
# Space Station / ISS Procedural Builder
# ---------------------------------------------------------------------------
def build_space_station():
    print("\n=======================================================")
    print("Building Authentic ISS (International Space Station)...")
    print("=======================================================")
    
    bpy.ops.object.select_all(action='SELECT')
    bpy.ops.object.delete(use_global=False)
    
    # 1. Instantiate Materials
    m_solar = make_iss_material("Mat_Solar", 'SOLAR_ARRAY')
    m_module = make_iss_material("Mat_Module", 'MODULE_HULL')
    m_radiator = make_iss_material("Mat_Radiator", 'RADIATOR')
    m_gold_mli = make_iss_material("Mat_GoldMLI", 'GOLD_MLI')
    m_truss = make_iss_material("Mat_Truss", 'TRUSS')
    m_dock = make_iss_material("Mat_Dock", 'DOCK')
    m_cupola = make_iss_material("Mat_Cupola", 'CUPOLA')
    m_nav_red = make_iss_material("Mat_NavRed", 'NAV_RED')
    m_nav_green = make_iss_material("Mat_NavGreen", 'NAV_GREEN')
    m_strobe = make_iss_material("Mat_Strobe", 'STROBE_WHITE')

    parts = []

    # Helper function to track active object
    def add_part(mat):
        obj = bpy.context.active_object
        obj.data.materials.append(mat)
        parts.append(obj)
        return obj

    # =======================================================================
    # A. INTEGRATED TRUSS STRUCTURE (ITS: along X axis)
    # =======================================================================
    # Main continuous truss beam spanning from -2.8 to +2.8
    bpy.ops.mesh.primitive_cube_add(size=1.0, location=(0.0, 0.0, 0.0))
    t_main = add_part(m_truss)
    t_main.name = "Truss_Main_Beam"
    t_main.scale = (5.6, 0.14, 0.14)

    # Truss upper and lower utility tray raceways
    bpy.ops.mesh.primitive_cube_add(size=1.0, location=(0.0, 0.10, 0.0))
    t_tray_top = add_part(m_dock)
    t_tray_top.name = "Truss_Tray_Zenith"
    t_tray_top.scale = (5.4, 0.04, 0.10)

    # Solar Alpha Rotary Joints (SARJ) - massive cylindrical bearings on Port (-1.5) and Starboard (+1.5)
    for sarj_x in [-1.5, 1.5]:
        bpy.ops.mesh.primitive_cylinder_add(vertices=24, radius=0.22, depth=0.28, location=(sarj_x, 0.0, 0.0), rotation=(0.0, math.pi/2, 0.0))
        sarj = add_part(m_dock)
        sarj.name = f"SARJ_Joint_{sarj_x}"

        # Outer ring gear collar
        bpy.ops.mesh.primitive_torus_add(major_radius=0.24, minor_radius=0.03, location=(sarj_x, 0.0, 0.0), rotation=(0.0, math.pi/2, 0.0))
        sarj_gear = add_part(m_gold_mli)
        sarj_gear.name = f"SARJ_Gear_{sarj_x}"

    # Structural truss bulkheads and segment hubs along the spine
    for hx in [-2.5, -2.0, -1.0, -0.5, 0.0, 0.5, 1.0, 2.0, 2.5]:
        bpy.ops.mesh.primitive_cube_add(size=1.0, location=(hx, 0.0, 0.0))
        hub = add_part(m_dock)
        hub.name = f"Truss_Hub_{hx}"
        hub.scale = (0.16, 0.18, 0.18)

    # S0 Truss zenith mounting struts down to Destiny lab (Y: 0.0 to -0.22, Z: 0.2 to 0.7)
    strut_coords = [(-0.16, -0.11, 0.25), (0.16, -0.11, 0.25), (-0.16, -0.11, 0.65), (0.16, -0.11, 0.65)]
    for sx, sy, sz in strut_coords:
        bpy.ops.mesh.primitive_cylinder_add(vertices=12, radius=0.025, depth=0.24, location=(sx, sy, sz))
        strut = add_part(m_truss)
        strut.name = f"Truss_S0_Strut_{sx}_{sz}"

    # =======================================================================
    # B. 8 PHOTOVOLTAIC SOLAR ARRAY WINGS (SAW)
    # =======================================================================
    # 4 gimbal stations: Port outboard P6 (-2.35), Port inboard P4 (-1.75),
    # Starboard inboard S4 (+1.75), Starboard outboard S6 (+2.35).
    # Each station has an upper (+Y) and lower (-Y) wing.
    saw_stations = [
        (-2.35,  0.80, 0.0, "P6_Zenith"),
        (-2.35, -0.80, 0.0, "P6_Nadir"),
        (-1.75,  0.80, 0.0, "P4_Zenith"),
        (-1.75, -0.80, 0.0, "P4_Nadir"),
        ( 1.75,  0.80, 0.0, "S4_Zenith"),
        ( 1.75, -0.80, 0.0, "S4_Nadir"),
        ( 2.35,  0.80, 0.0, "S6_Zenith"),
        ( 2.35, -0.80, 0.0, "S6_Nadir"),
    ]

    for wx, wy, wz, tag in saw_stations:
        # 1. Base canister (gold MLI cylinder at truss attachment)
        can_y = 0.12 if wy > 0 else -0.12
        bpy.ops.mesh.primitive_cylinder_add(vertices=16, radius=0.065, depth=0.22, location=(wx, can_y, wz))
        can = add_part(m_gold_mli)
        can.name = f"SAW_Canister_{tag}"

        # 2. Central folding deployer mast
        bpy.ops.mesh.primitive_cube_add(size=1.0, location=(wx, wy, wz))
        mast = add_part(m_truss)
        mast.name = f"SAW_Mast_{tag}"
        mast.scale = (0.045, 1.25, 0.035)

        # 3. Dual solar blanket panels on Port and Starboard sides of mast
        for side in [-1, 1]:
            blanket_x = wx + side * 0.14
            bpy.ops.mesh.primitive_cube_add(size=1.0, location=(blanket_x, wy, wz))
            blanket = add_part(m_solar)
            blanket.name = f"SAW_Blanket_{tag}_{side}"
            blanket.scale = (0.22, 1.20, 0.012)

    # =======================================================================
    # C. ACTIVE THERMAL CONTROL SYSTEM (ATCS) ACCORDION RADIATORS
    # =======================================================================
    # 3 major radiator wings extending backwards along -Z from P1 and S1 trusses
    radiator_configs = [
        (-0.85, 0.0, -0.75, "Port_P1"),
        ( 0.45, 0.0, -0.75, "Starboard_S1_A"),
        ( 0.95, 0.0, -0.75, "Starboard_S1_B"),
    ]

    for rx, ry, rz, tag in radiator_configs:
        # Accordion folded radiator array
        bpy.ops.mesh.primitive_cube_add(size=1.0, location=(rx, ry, rz))
        rad = add_part(m_radiator)
        rad.name = f"ATCS_Radiator_{tag}"
        rad.scale = (0.18, 0.018, 1.15)

        # Radiator rotary gimbal hub at truss
        bpy.ops.mesh.primitive_cylinder_add(vertices=16, radius=0.055, depth=0.18, location=(rx, ry, -0.12))
        rad_hub = add_part(m_dock)
        rad_hub.name = f"ATCS_Hub_{tag}"

    # P6 small Early Ammonia Servicing (EAS) radiator
    bpy.ops.mesh.primitive_cube_add(size=1.0, location=(-2.05, 0.0, -0.45))
    eas_rad = add_part(m_radiator)
    eas_rad.name = "ATCS_EAS_Radiator"
    eas_rad.scale = (0.12, 0.015, 0.65)

    # =======================================================================
    # D. PRESSURIZED MODULE CLUSTER (USOS & ROS along Z axis, Y = -0.25)
    # =======================================================================
    # Module centerline: Y = -0.24
    CY = -0.24

    # 1. Unity (Node 1) - Central connecting hub
    bpy.ops.mesh.primitive_cylinder_add(vertices=24, radius=0.22, depth=0.55, location=(0.0, CY, 0.0), rotation=(math.pi/2, 0.0, 0.0))
    unity = add_part(m_module)
    unity.name = "Module_Unity_Node1"

    # Unity radial berthing rings (CBM)
    for rot in [0.0, math.pi/2, math.pi, -math.pi/2]:
        bpy.ops.mesh.primitive_cylinder_add(vertices=16, radius=0.12, depth=0.08, location=(0.0 + 0.22 * math.cos(rot), CY + 0.22 * math.sin(rot), 0.0))
        cbm = add_part(m_dock)
        cbm.name = f"Unity_CBM_{rot}"

    # 2. Destiny US Laboratory (Forward from Unity, Z: 0.28 to 1.18)
    bpy.ops.mesh.primitive_cylinder_add(vertices=24, radius=0.23, depth=0.90, location=(0.0, CY, 0.72), rotation=(math.pi/2, 0.0, 0.0))
    destiny = add_part(m_module)
    destiny.name = "Module_Destiny_USLab"

    # Destiny forward and aft conical endcaps
    bpy.ops.mesh.primitive_cone_add(vertices=24, radius1=0.23, radius2=0.18, depth=0.10, location=(0.0, CY, 1.22), rotation=(-math.pi/2, 0.0, 0.0))
    destiny_cone_f = add_part(m_module)
    destiny_cone_f.name = "Destiny_Cone_Fwd"

    bpy.ops.mesh.primitive_cone_add(vertices=24, radius1=0.23, radius2=0.18, depth=0.10, location=(0.0, CY, 0.22), rotation=(math.pi/2, 0.0, 0.0))
    destiny_cone_a = add_part(m_module)
    destiny_cone_a.name = "Destiny_Cone_Aft"

    # Destiny nadir optical research window
    bpy.ops.mesh.primitive_cylinder_add(vertices=16, radius=0.065, depth=0.04, location=(0.0, CY - 0.23, 0.72))
    destiny_win = add_part(m_cupola)
    destiny_win.name = "Destiny_Nadir_Window"

    # 3. Harmony (Node 2) - Forward docking and lab hub (Z: 1.30 to 1.85)
    bpy.ops.mesh.primitive_cylinder_add(vertices=24, radius=0.22, depth=0.55, location=(0.0, CY, 1.55), rotation=(math.pi/2, 0.0, 0.0))
    harmony = add_part(m_module)
    harmony.name = "Module_Harmony_Node2"

    # 4. Columbus (ESA Laboratory) - Attached to Harmony Starboard (+X: 0.25 to 1.05)
    bpy.ops.mesh.primitive_cylinder_add(vertices=24, radius=0.21, depth=0.80, location=(0.65, CY, 1.55), rotation=(0.0, math.pi/2, 0.0))
    columbus = add_part(m_module)
    columbus.name = "Module_Columbus_ESA"

    # Columbus rounded end cap
    bpy.ops.mesh.primitive_uv_sphere_add(segments=16, ring_count=8, radius=0.21, location=(1.05, CY, 1.55))
    columbus_cap = add_part(m_module)
    columbus_cap.name = "Columbus_EndCap"
    columbus_cap.scale = (0.5, 1.0, 1.0)

    # 5. Kibo (JAXA JEM) - Attached to Harmony Port (-X: -0.25 to -1.35)
    bpy.ops.mesh.primitive_cylinder_add(vertices=24, radius=0.23, depth=1.10, location=(-0.80, CY, 1.55), rotation=(0.0, math.pi/2, 0.0))
    kibo = add_part(m_module)
    kibo.name = "Module_Kibo_JAXA"

    # Kibo Experiment Logistics Module (ELM-PS) cylinder on top (+Y)
    bpy.ops.mesh.primitive_cylinder_add(vertices=16, radius=0.18, depth=0.45, location=(-0.65, CY + 0.32, 1.55))
    kibo_elm = add_part(m_module)
    kibo_elm.name = "Kibo_ELM_PS"

    # Kibo Exposed Facility (EF - external "porch" experiment platform at -X tip)
    bpy.ops.mesh.primitive_cube_add(size=1.0, location=(-1.55, CY, 1.55))
    kibo_ef = add_part(m_truss)
    kibo_ef.name = "Kibo_Exposed_Facility"
    kibo_ef.scale = (0.42, 0.26, 0.36)

    # 6. Quest Joint Airlock - Branching off Starboard from Unity (Node 1)
    # Equipment lock (wider)
    bpy.ops.mesh.primitive_cylinder_add(vertices=16, radius=0.18, depth=0.35, location=(0.38, CY, 0.0), rotation=(0.0, math.pi/2, 0.0))
    quest_eq = add_part(m_module)
    quest_eq.name = "Airlock_Quest_Equipment"

    # Crew lock (narrower)
    bpy.ops.mesh.primitive_cylinder_add(vertices=16, radius=0.14, depth=0.32, location=(0.68, CY, 0.0), rotation=(0.0, math.pi/2, 0.0))
    quest_cr = add_part(m_module)
    quest_cr.name = "Airlock_Quest_Crew"

    # High-pressure gas tanks clustered on Quest airlock
    for tz, ty in [(-0.12, 0.12), (0.12, 0.12), (-0.12, -0.12), (0.12, -0.12)]:
        bpy.ops.mesh.primitive_uv_sphere_add(segments=12, ring_count=6, radius=0.055, location=(0.42, CY + ty, tz))
        tank = add_part(m_gold_mli)
        tank.name = f"Airlock_GasTank_{tz}_{ty}"

    # 7. Tranquility (Node 3) & Cupola - Attached to Port from Unity (Node 1)
    bpy.ops.mesh.primitive_cylinder_add(vertices=24, radius=0.22, depth=0.55, location=(-0.45, CY, 0.0), rotation=(0.0, math.pi/2, 0.0))
    tranq = add_part(m_module)
    tranq.name = "Module_Tranquility_Node3"

    # Cupola Observation Dome on Tranquility Nadir (-Y, pointing directly down at Earth!)
    bpy.ops.mesh.primitive_cylinder_add(vertices=16, radius=0.14, depth=0.10, location=(-0.45, CY - 0.26, 0.0))
    cupola_base = add_part(m_dock)
    cupola_base.name = "Cupola_Mount_Ring"

    # 7-Window Faceted Cupola Dome
    bpy.ops.mesh.primitive_cylinder_add(vertices=8, radius=0.13, depth=0.08, location=(-0.45, CY - 0.33, 0.0))
    cupola_dome = add_part(m_cupola)
    cupola_dome.name = "Cupola_Observation_Dome"

    # 8. International Docking Adapter (IDA-2 / PMA-2) at forward nose (Z: 1.85 to 2.25)
    bpy.ops.mesh.primitive_cone_add(vertices=24, radius1=0.21, radius2=0.16, depth=0.25, location=(0.0, CY, 1.95), rotation=(-math.pi/2, 0.0, 0.0))
    pma2 = add_part(m_dock)
    pma2.name = "PMA2_Adapter_Fwd"

    bpy.ops.mesh.primitive_cylinder_add(vertices=24, radius=0.16, depth=0.12, location=(0.0, CY, 2.12), rotation=(math.pi/2, 0.0, 0.0))
    ida2_ring = add_part(m_dock)
    ida2_ring.name = "IDA2_Docking_Collar"

    # Docking Petal Alignment Ring
    bpy.ops.mesh.primitive_torus_add(major_radius=0.165, minor_radius=0.022, location=(0.0, CY, 2.18), rotation=(0.0, 0.0, 0.0))
    ida_ring_petal = add_part(m_strobe)
    ida_ring_petal.name = "IDA2_Alignment_Ring"

    # Laser Retroreflector / Approach Cross
    bpy.ops.mesh.primitive_cube_add(size=1.0, location=(0.0, CY, 2.19))
    cross_h = add_part(m_strobe)
    cross_h.name = "Docking_Target_Cross_H"
    cross_h.scale = (0.16, 0.015, 0.015)

    bpy.ops.mesh.primitive_cube_add(size=1.0, location=(0.0, CY, 2.19))
    cross_v = add_part(m_strobe)
    cross_v.name = "Docking_Target_Cross_V"
    cross_v.scale = (0.015, 0.16, 0.015)

    # 9. Russian Orbital Segment (ROS - along negative Z axis)
    # Zarya (FGB - Functional Cargo Block, Z: -0.32 to -1.15)
    bpy.ops.mesh.primitive_cylinder_add(vertices=24, radius=0.21, depth=0.75, location=(0.0, CY, -0.72), rotation=(math.pi/2, 0.0, 0.0))
    zarya = add_part(m_gold_mli)
    zarya.name = "Module_Zarya_FGB"

    # Zarya side propellant fairings
    for zs in [-1, 1]:
        bpy.ops.mesh.primitive_cube_add(size=1.0, location=(zs * 0.22, CY, -0.72))
        z_fair = add_part(m_module)
        z_fair.name = f"Zarya_Fairing_{zs}"
        z_fair.scale = (0.06, 0.12, 0.65)

    # Zarya angled secondary solar wings
    for side in [-1, 1]:
        bpy.ops.mesh.primitive_cube_add(size=1.0, location=(side * 0.75, CY - 0.05, -0.72))
        z_wing = add_part(m_solar)
        z_wing.name = f"Zarya_Solar_Wing_{side}"
        z_wing.scale = (0.85, 0.012, 0.22)

    # Zvezda Service Module (Stepped Russian command hub, Z: -1.15 to -2.15)
    # Forward spherical docking transfer chamber
    bpy.ops.mesh.primitive_uv_sphere_add(segments=16, ring_count=10, radius=0.18, location=(0.0, CY, -1.22))
    zvezda_fwd = add_part(m_module)
    zvezda_fwd.name = "Zvezda_Spherical_Compartment"

    # Main cylindrical work compartment (stepped body)
    bpy.ops.mesh.primitive_cylinder_add(vertices=24, radius=0.23, depth=0.65, location=(0.0, CY, -1.65), rotation=(math.pi/2, 0.0, 0.0))
    zvezda_main = add_part(m_module)
    zvezda_main.name = "Zvezda_Main_Compartment"

    # Aft tapered engine section
    bpy.ops.mesh.primitive_cone_add(vertices=24, radius1=0.23, radius2=0.15, depth=0.25, location=(0.0, CY, -2.05), rotation=(math.pi/2, 0.0, 0.0))
    zvezda_aft_cone = add_part(m_module)
    zvezda_aft_cone.name = "Zvezda_Aft_Cone"

    # Aft docking collar (for Progress / Soyuz transports)
    bpy.ops.mesh.primitive_cylinder_add(vertices=24, radius=0.13, depth=0.12, location=(0.0, CY, -2.22), rotation=(math.pi/2, 0.0, 0.0))
    zvezda_dock = add_part(m_dock)
    zvezda_dock.name = "Zvezda_Aft_Docking_Port"

    # Zvezda primary steerable solar panels
    for side in [-1, 1]:
        bpy.ops.mesh.primitive_cube_add(size=1.0, location=(side * 1.05, CY, -1.65))
        zv_wing = add_part(m_solar)
        zv_wing.name = f"Zvezda_Solar_Wing_{side}"
        zv_wing.scale = (1.10, 0.012, 0.26)

    # Poisk / Rassvet docking modules branching nadir/zenith
    bpy.ops.mesh.primitive_cylinder_add(vertices=16, radius=0.14, depth=0.35, location=(0.0, CY + 0.28, -1.22))
    poisk = add_part(m_module)
    poisk.name = "Module_Poisk_Zenith"

    bpy.ops.mesh.primitive_cylinder_add(vertices=16, radius=0.14, depth=0.35, location=(0.0, CY - 0.28, -0.72))
    rassvet = add_part(m_module)
    rassvet.name = "Module_Rassvet_Nadir"

    # =======================================================================
    # E. CANADARM2 & COMMUNICATIONS DISH
    # =======================================================================
    # Canadarm2 boom joints mounted near Destiny / S0 truss
    arm_joints = [
        Vector((0.25, CY + 0.35, 0.45)),
        Vector((0.55, CY + 0.65, 0.70)),
        Vector((0.35, CY + 0.85, 1.15)),
    ]
    for aj in range(len(arm_joints) - 1):
        p1 = arm_joints[aj]
        p2 = arm_joints[aj + 1]
        mid = (p1 + p2) * 0.5
        dist = (p2 - p1).length
        bpy.ops.mesh.primitive_cylinder_add(vertices=12, radius=0.035, depth=dist, location=mid)
        arm_seg = add_part(m_module)
        arm_seg.name = f"Canadarm2_Boom_{aj}"

    # Space-to-Ground High Gain Antenna (SGANT)
    bpy.ops.mesh.primitive_cylinder_add(vertices=12, radius=0.025, depth=0.45, location=(0.65, 0.25, -0.15))
    dish_mast = add_part(m_truss)
    dish_mast.name = "SGANT_Antenna_Mast"

    bpy.ops.mesh.primitive_cone_add(vertices=24, radius1=0.28, radius2=0.02, depth=0.12, location=(0.65, 0.45, -0.15), rotation=(math.radians(35), math.radians(20), 0.0))
    dish = add_part(m_gold_mli)
    dish.name = "SGANT_Antenna_Dish"

    # =======================================================================
    # F. NAVIGATION & DOCKING APPROACH LIGHTS
    # =======================================================================
    # Port Red Navigation Beacon (Far Port Tip of P6 Truss: -2.82)
    bpy.ops.mesh.primitive_uv_sphere_add(segments=8, ring_count=4, radius=0.045, location=(-2.82, 0.0, 0.0))
    nav_r = add_part(m_nav_red)
    nav_r.name = "Nav_Light_Port_Red"

    # Starboard Green Navigation Beacon (Far Starboard Tip of S6 Truss: +2.82)
    bpy.ops.mesh.primitive_uv_sphere_add(segments=8, ring_count=4, radius=0.045, location=(2.82, 0.0, 0.0))
    nav_g = add_part(m_nav_green)
    nav_g.name = "Nav_Light_Starboard_Green"

    # Forward Docking Alignment Strobes around IDA-2 collar
    strobe_positions = [
        (-0.25, CY, 2.20),
        ( 0.25, CY, 2.20),
        ( 0.0, CY + 0.25, 2.20),
        ( 0.0, CY - 0.25, 2.20),
        ( 0.0, 0.16, 0.0),  # Truss zenith strobe
        ( 0.0, -0.16, 0.0), # Truss nadir strobe
    ]
    for s_idx, sp in enumerate(strobe_positions):
        bpy.ops.mesh.primitive_uv_sphere_add(segments=8, ring_count=4, radius=0.035, location=sp)
        strobe = add_part(m_strobe)
        strobe.name = f"Dock_Strobe_{s_idx}"

    # =======================================================================
    # G. UNIFIED MESH JOINING & UV UNWRAPPING
    # =======================================================================
    print(f"Joining {len(parts)} individual station components...")
    for obj in parts:
        if obj.name not in bpy.context.scene.collection.objects:
            bpy.context.scene.collection.objects.link(obj)
        obj.select_set(True)
    bpy.context.view_layer.objects.active = t_main
    bpy.ops.object.join()
    station = bpy.context.active_object
    station.name = "ISS_Space_Station"

    # Normalize dimensions: total wingspan across solar arrays (X axis) normalized to 1.0 unit
    bpy.ops.object.origin_set(type='ORIGIN_GEOMETRY', center='BOUNDS')
    bbox = [station.matrix_world @ Vector(corner) for corner in station.bound_box]
    min_x = min(v.x for v in bbox)
    max_x = max(v.x for v in bbox)
    cur_width = max_x - min_x
    target_width = 1.0  # Normalized to 1.0 so SceneManager scale directly governs world wingspan
    scale_factor = target_width / cur_width
    station.scale = (scale_factor, scale_factor, scale_factor)
    bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)

    # Re-center geometry to (0, 0, 0)
    bbox = [station.matrix_world @ Vector(corner) for corner in station.bound_box]
    center = Vector(((min(v.x for v in bbox) + max(v.x for v in bbox)) * 0.5,
                     (min(v.y for v in bbox) + max(v.y for v in bbox)) * 0.5,
                     (min(v.z for v in bbox) + max(v.z for v in bbox)) * 0.5))
    station.location = -center
    bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)

    # Generate non-overlapping UV atlas
    print("Generating non-overlapping UV atlas via Smart Project...")
    bpy.ops.object.mode_set(mode='EDIT')
    bpy.ops.mesh.select_all(action='SELECT')
    bpy.ops.uv.smart_project(angle_limit=1.15, island_margin=0.018)
    bpy.ops.object.mode_set(mode='OBJECT')

    poly_count = len(station.data.polygons)
    vert_count = len(station.data.vertices)
    print(f"ISS Station Mesh built: {vert_count} vertices, {poly_count} polygons.")
    return station


# ---------------------------------------------------------------------------
# PBR Texture Baking Suite
# ---------------------------------------------------------------------------
def bake_pbr_set(obj, base_path, tex_size=2048):
    print(f"\n--- Baking ISS PBR texture suite ({tex_size}x{tex_size}) ---")
    scene = bpy.context.scene
    scene.render.engine = 'CYCLES'
    scene.cycles.device = 'CPU'
    scene.cycles.samples = 1  # 1 sample suffices for emit/color passes
    
    bake_img = bpy.data.images.new(f"Bake_{obj.name}", tex_size, tex_size)
    
    # Configure image node target in all materials
    for slot in obj.material_slots:
        mat = slot.material
        if not mat or not mat.use_nodes:
            continue
        tex_node = mat.node_tree.nodes.get("BakeTarget")
        if not tex_node:
            tex_node = mat.node_tree.nodes.new("ShaderNodeTexImage")
            tex_node.name = "BakeTarget"
        tex_node.image = bake_img
        mat.node_tree.nodes.active = tex_node

    bpy.context.view_layer.objects.active = obj
    obj.select_set(True)

    # 1. Albedo Map
    print("Baking Albedo map...")
    for slot in obj.material_slots:
        set_material_pass(slot.material, 'ALBEDO')
    bpy.ops.object.bake(type='EMIT')
    save_blender_image_as_bmp(bake_img, f"{base_path}_albedo.bmp")

    # 2. Tangent-Space Normal Map
    print("Baking Normal map (Tangent space)...")
    for slot in obj.material_slots:
        set_material_pass(slot.material, 'PBR')
    scene.cycles.bake_type = 'NORMAL'
    scene.render.bake.normal_space = 'TANGENT'
    scene.render.bake.normal_r = 'POS_X'
    scene.render.bake.normal_g = 'POS_Y'
    scene.render.bake.normal_b = 'POS_Z'
    bpy.ops.object.bake(type='NORMAL')
    save_blender_image_as_bmp(bake_img, f"{base_path}_normal.bmp")

    # 3. Roughness Map
    print("Baking Roughness map...")
    for slot in obj.material_slots:
        set_material_pass(slot.material, 'ROUGHNESS')
    bpy.ops.object.bake(type='EMIT')
    save_blender_image_as_bmp(bake_img, f"{base_path}_rough.bmp")

    # 4. Metallic Map
    print("Baking Metallic map...")
    for slot in obj.material_slots:
        set_material_pass(slot.material, 'METALLIC')
    bpy.ops.object.bake(type='EMIT')
    save_blender_image_as_bmp(bake_img, f"{base_path}_metal.bmp")

    # 5. Emissive Map
    print("Baking Emissive map...")
    for slot in obj.material_slots:
        set_material_pass(slot.material, 'EMISSIVE')
    bpy.ops.object.bake(type='EMIT')
    save_blender_image_as_bmp(bake_img, f"{base_path}_emissive.bmp")

    # Restore materials to standard PBR
    for slot in obj.material_slots:
        set_material_pass(slot.material, 'PBR')
        
    bpy.data.images.remove(bake_img)
    print("All 5 PBR texture maps baked successfully!")


# ---------------------------------------------------------------------------
# BVH / Line-of-sight & Collision Validation for Close Camera
# ---------------------------------------------------------------------------
def find_valid_close_camera(station, target, desired_dist=0.50):
    """Ensure camera is not inside geometry and has clear line of sight to target."""
    depsgraph = bpy.context.evaluated_depsgraph_get()
    eval_station = station.evaluated_get(depsgraph)

    candidates = [
        Vector(( 0.28,  0.28,  0.32)),  # Elevated forward-right looking at SAW junction
        Vector(( 0.34,  0.22,  0.26)),
        Vector(( 0.20,  0.38,  0.24)),
        Vector(( 0.32,  0.25, -0.26)),
        Vector((-0.20,  0.36,  0.26)),
        Vector(( 0.16,  0.44,  0.16)),
        Vector((-0.26,  0.28, -0.28)),
    ]

    for offset in candidates:
        cam_pos = target + offset.normalized() * desired_dist

        # 1. Proximity check to mesh surface
        success, closest_pt, _, _ = eval_station.closest_point_on_mesh(cam_pos)
        dist_to_mesh = 999.0
        if success:
            dist_to_mesh = (cam_pos - closest_pt).length
            if dist_to_mesh < 0.045:
                continue  # Inside or too close to geometry

        # 2. Ray-cast check from camera to target
        ray_dir = (target - cam_pos).normalized()
        ray_dist = (target - cam_pos).length
        hit_success, hit_loc, _, _ = eval_station.ray_cast(cam_pos, ray_dir, distance=ray_dist + 0.1)

        if hit_success:
            dist_to_target = (hit_loc - target).length
            if dist_to_target < 0.07:
                print(f"BVH-ray hit verified: cam_pos={cam_pos}, dist_to_mesh={dist_to_mesh:.3f}, hit_err={dist_to_target:.3f}")
                return cam_pos
        else:
            print(f"BVH-ray unobstructed: cam_pos={cam_pos}, dist_to_mesh={dist_to_mesh:.3f}")
            return cam_pos

    # Safe fallback
    fallback_pos = target + Vector((0.24, 0.38, 0.26)).normalized() * desired_dist
    print(f"Using elevated fallback close camera: {fallback_pos}")
    return fallback_pos


# ---------------------------------------------------------------------------
# Sanity-check & Self-adjusting Render Loop
# ---------------------------------------------------------------------------
def render_with_sanity_check(scene, out_path, frame_tag, min_luma=18.0/255.0, max_dark_frac=0.35, max_iters=5):
    """Renders still and checks pixel luma and darkness, automatically tuning exposure/ambient up to max_iters."""
    world = scene.world
    bg_node = world.node_tree.nodes.get("Background") if (world and world.use_nodes) else None
    
    for it in range(1, max_iters + 1):
        scene.render.filepath = out_path
        bpy.ops.render.render(write_still=True)
        
        # Read pixels from saved image
        abs_p = os.path.abspath(out_path)
        img = bpy.data.images.load(abs_p)
        px = np.array(img.pixels[:], dtype=np.float32).reshape(-1, 4)
        rgb = px[:, :3]
        luma = 0.2126 * rgb[:, 0] + 0.7152 * rgb[:, 1] + 0.0722 * rgb[:, 2]
        mean_luma = float(np.mean(luma))
        dark_frac = float(np.mean(luma < (6.0 / 255.0)))
        bpy.data.images.remove(img)
        
        is_pass = (mean_luma >= min_luma) and (dark_frac <= max_dark_frac)
        status_str = "PASS" if is_pass else f"RETRY ({it}/{max_iters})"
        print(f"[{frame_tag} iter {it}] LUMA={mean_luma * 255.0:.2f}/255 (mean={mean_luma:.4f}, dark_frac={dark_frac * 100.0:.2f}%) {status_str}")
        
        if is_pass:
            break
            
        # Self-adjustment
        if mean_luma < min_luma:
            scene.view_settings.exposure += 0.35
            print(f"  Adjusting exposure -> {scene.view_settings.exposure:.2f}")
        if dark_frac > max_dark_frac and bg_node:
            cur_str = bg_node.inputs["Strength"].default_value
            bg_node.inputs["Strength"].default_value = cur_str + 0.30
            print(f"  Adjusting ambient background strength -> {bg_node.inputs['Strength'].default_value:.2f}")


# ---------------------------------------------------------------------------
# Cycles Showcase Renders
# ---------------------------------------------------------------------------
def render_station_showcases(station, out_hero_png, out_close_png):
    print(f"\n--- Setting up Cycles Studio Scene for Station Showcase Renders ---")
    scene = bpy.context.scene
    scene.render.engine = 'CYCLES'
    scene.cycles.device = 'CPU'
    scene.cycles.samples = 64
    scene.cycles.use_denoising = True
    scene.render.resolution_x = 1280
    scene.render.resolution_y = 720
    scene.render.image_settings.file_format = 'PNG'
    scene.render.image_settings.color_mode = 'RGBA'
    scene.view_settings.exposure = 0.25

    # Camera
    cam_data = bpy.data.cameras.new("StationCam")
    cam_data.lens = 48
    cam_obj = bpy.data.objects.new("StationCam", cam_data)
    scene.collection.objects.link(cam_obj)
    scene.camera = cam_obj

    # Lighting
    # 1. Direct Sun Key at ~35° from top-left (crisp vacuum sunlight)
    sun_data = bpy.data.lights.new("OrbitalSun", type='SUN')
    sun_data.energy = 9.0
    sun_data.color = (1.0, 0.98, 0.95)
    sun_obj = bpy.data.objects.new("OrbitalSun", sun_data)
    sun_obj.rotation_euler = (math.radians(35), math.radians(25), math.radians(-50))
    scene.collection.objects.link(sun_obj)

    # 2. Cool Azure Earth-Shine Albedo Rim/Bounce from Nadir
    earth_shine = bpy.data.lights.new("EarthAlbedo", type='SUN')
    earth_shine.energy = 3.5
    earth_shine.color = (0.22, 0.55, 0.95)
    earth_obj = bpy.data.objects.new("EarthAlbedo", earth_shine)
    earth_obj.rotation_euler = (math.radians(-45), math.radians(-25), math.radians(140))
    scene.collection.objects.link(earth_obj)

    # Deep space world background with subtle orbital airglow/scatter
    world = scene.world
    if not world:
        world = bpy.data.worlds.new("StationWorld")
        scene.world = world
    world.use_nodes = True
    bg = world.node_tree.nodes.get("Background")
    if bg:
        bg.inputs["Color"].default_value = (0.028, 0.038, 0.058, 1.0)
        bg.inputs["Strength"].default_value = 0.85

    # =======================================================================
    # Render 1: Hero Showcase - Full silhouette of ENTIRE station
    # (Distance >= 2.5 truss-span, elevated diagonal view)
    # =======================================================================
    print(f"\nRendering Hero View -> {out_hero_png} ...")
    cam_data.lens = 48
    cam_data.dof.use_dof = False

    target_hero = Vector((0.0, -0.04, 0.02))
    hero_dist = 2.75  # >= 2.5 truss-span (normalized wingspan is 1.0)
    cam_hero_pos = target_hero + Vector((-1.45, 1.30, 1.60)).normalized() * hero_dist
    cam_obj.location = cam_hero_pos
    cam_obj.rotation_euler = (target_hero - cam_hero_pos).to_track_quat('-Z', 'Y').to_euler()

    actual_hero_dist = (target_hero - cam_hero_pos).length
    print(f"Showcase Camera distance to target: {actual_hero_dist:.2f} truss-spans (requirement: >= 2.5)")

    render_with_sanity_check(scene, out_hero_png, "SHOWCASE", min_luma=18.0/255.0, max_dark_frac=0.35)
    print(f"Saved Showcase: {out_hero_png}")

    # =======================================================================
    # Render 2: Close-Up View - SAW junction / connection to truss
    # (Distance ~0.5 truss-span, DoF f/4, BVH line-of-sight validation)
    # =======================================================================
    print(f"\nRendering Close-Up View -> {out_close_png} ...")
    close_target = Vector((-0.29, 0.02, 0.01))  # Port SAW canister / truss junction
    close_dist = 0.50  # ~0.5 truss-span

    cam_close_pos = find_valid_close_camera(station, close_target, desired_dist=close_dist)
    cam_obj.location = cam_close_pos
    cam_obj.rotation_euler = (close_target - cam_close_pos).to_track_quat('-Z', 'Y').to_euler()

    # Depth of Field f/4 focused on the junction
    cam_data.lens = 52
    cam_data.dof.use_dof = True
    cam_data.dof.focus_distance = (close_target - cam_close_pos).length
    cam_data.dof.aperture_fstop = 4.0  # f/4!

    actual_close_dist = (close_target - cam_close_pos).length
    print(f"Close Camera distance to target: {actual_close_dist:.2f} truss-spans (requirement: ~0.5, DoF f/4)")

    render_with_sanity_check(scene, out_close_png, "CLOSE", min_luma=18.0/255.0, max_dark_frac=0.35)
    print(f"Saved Close-Up: {out_close_png}")


# ---------------------------------------------------------------------------
# Main Pipeline
# ---------------------------------------------------------------------------
def main():
    print("Starting astro-voyager Authentic ISS Asset Pipeline...")
    station = build_space_station()
    station_base = os.path.join(RES_MODELS, "station")

    # 1. Export OBJ (Wavefront, forward Y, up Z)
    station_obj_path = f"{station_base}.obj"
    print(f"Exporting OBJ -> {station_obj_path}")
    station.select_set(True)
    bpy.context.view_layer.objects.active = station
    bpy.ops.wm.obj_export(
        filepath=station_obj_path,
        export_selected_objects=True,
        export_uv=True,
        export_normals=True,
        export_materials=False,
        export_triangulated_mesh=True,
        forward_axis='Y',
        up_axis='Z'
    )
    # Also copy to build-gl and build
    shutil.copyfile(station_obj_path, os.path.join(BUILD_GL_MODELS, "station.obj"))
    shutil.copyfile(station_obj_path, os.path.join(BUILD_MODELS, "station.obj"))

    # 2. Bake PBR Texture Set
    bake_pbr_set(station, station_base, tex_size=2048)
    for ext in ["_albedo.bmp", "_normal.bmp", "_rough.bmp", "_metal.bmp", "_emissive.bmp"]:
        src_f = f"{station_base}{ext}"
        shutil.copyfile(src_f, os.path.join(BUILD_GL_MODELS, f"station{ext}"))
        shutil.copyfile(src_f, os.path.join(BUILD_MODELS, f"station{ext}"))
    print("Copied all textures to build-gl/Release/res/models/ and build/Release/res/models/")

    # 3. Cycles Showcase Renders
    hero_out = os.path.join(OUT_SHOWCASE, "station_showcase.png")
    close_out = os.path.join(OUT_SHOWCASE, "station_close.png")
    render_station_showcases(station, hero_out, close_out)

    # 4. Copy to docs/screens
    hero_doc = os.path.join(DOCS_SCREENS, "station_showcase.png")
    close_doc = os.path.join(DOCS_SCREENS, "station_close.png")
    shutil.copyfile(hero_out, hero_doc)
    shutil.copyfile(close_out, close_doc)
    print(f"Copied showcases to {hero_doc} and {close_doc}")

    print("\n=======================================================")
    print("OK / exported Space Station ISS Assets Successfully!")
    print("=======================================================")


if __name__ == '__main__':
    main()
