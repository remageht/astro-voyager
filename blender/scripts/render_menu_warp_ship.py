import os
import math
import bpy
from mathutils import Vector, Euler

REPO = r"c:\Users\Эмир\Documents\Default Project\astro-voyager"
OBJ_PATH = os.path.join(REPO, "res", "models", "warp_ship.obj")
ALBEDO = os.path.join(REPO, "res", "models", "warp_ship_albedo.bmp")
EMISSIVE = os.path.join(REPO, "res", "models", "warp_ship_emissive.bmp")
NORMAL = os.path.join(REPO, "res", "models", "warp_ship_normal.bmp")
ROUGH = os.path.join(REPO, "res", "models", "warp_ship_rough.bmp")
METAL = os.path.join(REPO, "res", "models", "warp_ship_metal.bmp")

OUT_PNG = os.path.join(REPO, "blender", "out", "warp_ship_menu_render.png")
os.makedirs(os.path.dirname(OUT_PNG), exist_ok=True)

def main():
    bpy.ops.wm.read_factory_settings(use_empty=True)
    
    # Import warp_ship.obj
    print("Importing OBJ:", OBJ_PATH)
    bpy.ops.wm.obj_import(filepath=OBJ_PATH)
    
    meshes = [o for o in bpy.data.objects if o.type == "MESH"]
    if not meshes:
        print("ERROR: No mesh found!")
        return
    
    ship = meshes[0]
    ship.name = "HeroWarpCruiser"
    
    # Create PBR material
    mat = bpy.data.materials.new("WarpShip_PBR")
    mat.use_nodes = True
    nodes = mat.node_tree.nodes
    links = mat.node_tree.links
    nodes.clear()
    
    out_node = nodes.new("ShaderNodeOutputMaterial")
    out_node.location = (600, 0)
    
    bsdf = nodes.new("ShaderNodeBsdfPrincipled")
    bsdf.location = (200, 0)
    links.new(bsdf.outputs["BSDF"], out_node.inputs["Surface"])
    
    def add_tex(path, is_color, loc):
        if not os.path.exists(path):
            print("Missing texture:", path)
            return None
        tex = nodes.new("ShaderNodeTexImage")
        tex.image = bpy.data.images.load(path)
        if not is_color:
            tex.image.colorspace_settings.name = "Non-Color"
        tex.location = loc
        return tex
    
    t_alb = add_tex(ALBEDO, True, (-300, 300))
    if t_alb:
        links.new(t_alb.outputs["Color"], bsdf.inputs["Base Color"])
        
    t_met = add_tex(METAL, False, (-300, 0))
    if t_met:
        links.new(t_met.outputs["Color"], bsdf.inputs["Metallic"])
        
    t_rou = add_tex(ROUGH, False, (-300, -250))
    if t_rou:
        links.new(t_rou.outputs["Color"], bsdf.inputs["Roughness"])
        
    t_emi = add_tex(EMISSIVE, True, (-300, -500))
    if t_emi:
        links.new(t_emi.outputs["Color"], bsdf.inputs["Emission Color"])
        bsdf.inputs["Emission Strength"].default_value = 5.5
        
    t_nrm = add_tex(NORMAL, False, (-600, -750))
    if t_nrm:
        nrm_node = nodes.new("ShaderNodeNormalMap")
        nrm_node.location = (-300, -750)
        links.new(t_nrm.outputs["Color"], nrm_node.inputs["Color"])
        links.new(nrm_node.outputs["Normal"], bsdf.inputs["Normal"])
        
    ship.data.materials.clear()
    ship.data.materials.append(mat)
    
    # Position ship: nose is at -Z. Rotate to point forward-left (-X, -Z)
    ship.location = (0.0, 0.0, 0.0)
    # Pitch up slightly (~10 deg), yaw left (~-16 deg), bank (~40 deg)
    ship.rotation_euler = Euler((math.radians(10.0), math.radians(-16.0), math.radians(40.0)), "XYZ")
    
    # Lighting:
    # 1. Key light from the Black Hole accretion disk (front-left, warm golden-orange)
    bh_light_data = bpy.data.lights.new("AccretionKeyLight", type="SUN")
    bh_light_data.energy = 5.0
    bh_light_data.color = (1.0, 0.72, 0.35)
    bh_light = bpy.data.objects.new("AccretionKeyLight", bh_light_data)
    bh_light.rotation_euler = Euler((math.radians(45.0), math.radians(-30.0), math.radians(70.0)), "XYZ")
    bpy.context.collection.objects.link(bh_light)
    
    # 2. Fill rim light from deep space & warp glow (cool cyan-blue)
    rim_light_data = bpy.data.lights.new("WarpRimLight", type="SUN")
    rim_light_data.energy = 2.5
    rim_light_data.color = (0.2, 0.65, 1.0)
    rim_light = bpy.data.objects.new("WarpRimLight", rim_light_data)
    rim_light.rotation_euler = Euler((math.radians(-50.0), math.radians(45.0), math.radians(-120.0)), "XYZ")
    bpy.context.collection.objects.link(rim_light)
    
    # 3. Ambient bounce
    world = bpy.data.worlds.new("SpaceWorld")
    world.use_nodes = True
    bg = world.node_tree.nodes.get("Background")
    if bg:
        bg.inputs["Color"].default_value = (0.01, 0.015, 0.03, 1.0)
        bg.inputs["Strength"].default_value = 0.4
    bpy.context.scene.world = world

    # Render settings
    scene = bpy.context.scene
    scene.render.engine = "CYCLES"
    scene.cycles.device = "CPU"
    scene.cycles.samples = 64
    scene.render.resolution_x = 1376
    scene.render.resolution_y = 768
    scene.render.film_transparent = True
    scene.render.image_settings.file_format = "PNG"
    scene.render.image_settings.color_mode = "RGBA"
    
    # Camera setup: framing the ship nicely in the frame
    cam_data = bpy.data.cameras.new("HeroCam")
    cam_data.lens = 55.0
    cam_obj = bpy.data.objects.new("HeroCam", cam_data)
    bpy.context.collection.objects.link(cam_obj)
    scene.camera = cam_obj
    
    # View looking at the ship from 3/4 perspective
    cam_obj.location = Vector((1.8, -2.4, 1.1))
    direction = Vector((0.0, 0.0, 0.0)) - cam_obj.location
    rot_quat = direction.to_track_quat("-Z", "Y")
    cam_obj.rotation_euler = rot_quat.to_euler()
    
    scene.render.filepath = OUT_PNG
    print(f"Rendering transparent hero ship to {OUT_PNG}...")
    bpy.ops.render.render(write_still=True)
    print("Render complete!")

if __name__ == "__main__":
    main()
