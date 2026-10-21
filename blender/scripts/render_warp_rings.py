import os
import bpy
from mathutils import Vector

REPO = r"c:\Users\Эмир\Documents\Default Project\astro-voyager"
OBJ_PATH = os.path.join(REPO, "res", "models", "warp_ship.obj")
ALBEDO = os.path.join(REPO, "res", "models", "warp_ship_albedo.bmp")
EMISSIVE = os.path.join(REPO, "res", "models", "warp_ship_emissive.bmp")
NORMAL = os.path.join(REPO, "res", "models", "warp_ship_normal.bmp")
ROUGH = os.path.join(REPO, "res", "models", "warp_ship_rough.bmp")
METAL = os.path.join(REPO, "res", "models", "warp_ship_metal.bmp")

OUT_DIR = os.path.join(REPO, "blender", "out", "final_showcase")
DOCS_DIR = os.path.join(REPO, "docs", "screens")
os.makedirs(OUT_DIR, exist_ok=True)
os.makedirs(DOCS_DIR, exist_ok=True)

def main():
    bpy.ops.wm.read_factory_settings(use_empty=True)
    
    # Import separated warp_ship.obj
    print("Importing:", OBJ_PATH)
    bpy.ops.wm.obj_import(filepath=OBJ_PATH)
    
    meshes = [o for o in bpy.data.objects if o.type == "MESH"]
    if not meshes:
        print("ERROR: No mesh found!")
        return
    
    ship = meshes[0]
    
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
    
    # Texture nodes helper
    def add_tex(path, is_color, loc):
        if not os.path.exists(path):
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
        bsdf.inputs["Emission Strength"].default_value = 3.5
        
    t_nrm = add_tex(NORMAL, False, (-600, -750))
    if t_nrm:
        nrm_node = nodes.new("ShaderNodeNormalMap")
        nrm_node.location = (-300, -750)
        links.new(t_nrm.outputs["Color"], nrm_node.inputs["Color"])
        links.new(nrm_node.outputs["Normal"], bsdf.inputs["Normal"])
        
    ship.data.materials.clear()
    ship.data.materials.append(mat)
    
    # Lighting & World
    world = bpy.data.worlds.new("SpaceWorld")
    world.use_nodes = True
    bg = world.node_tree.nodes.get("Background")
    if bg:
        bg.inputs["Color"].default_value = (0.005, 0.008, 0.015, 1.0)
        bg.inputs["Strength"].default_value = 0.5
    bpy.context.scene.world = world
    
    # Sun light
    sun_data = bpy.data.lights.new("KeySun", type="SUN")
    sun_data.energy = 3.5
    sun_data.color = (0.95, 0.98, 1.0)
    sun_obj = bpy.data.objects.new("KeySun", sun_data)
    sun_obj.rotation_euler = (0.7, 0.4, 0.9)
    bpy.context.collection.objects.link(sun_obj)
    
    # Rim light
    rim_data = bpy.data.lights.new("RimSun", type="SUN")
    rim_data.energy = 2.0
    rim_data.color = (0.2, 0.6, 1.0)
    rim_obj = bpy.data.objects.new("RimSun", rim_data)
    rim_obj.rotation_euler = (-1.2, -0.6, -2.1)
    bpy.context.collection.objects.link(rim_obj)
    
    # Render settings
    scene = bpy.context.scene
    scene.render.engine = "CYCLES"
    scene.cycles.device = "CPU"
    scene.cycles.samples = 64
    scene.render.resolution_x = 1280
    scene.render.resolution_y = 720
    scene.render.film_transparent = False
    
    cam_data = bpy.data.cameras.new("Cam")
    cam_data.lens = 45
    cam_obj = bpy.data.objects.new("Cam", cam_data)
    bpy.context.collection.objects.link(cam_obj)
    scene.camera = cam_obj
    
    # View 1: 3/4 perspective angle showcasing 3 separated rings
    cam_obj.location = Vector((1.4, -1.8, 0.9))
    direction = Vector((0.0, 0.0, 0.0)) - cam_obj.location
    rot_quat = direction.to_track_quat("-Z", "Y")
    cam_obj.rotation_euler = rot_quat.to_euler()
    
    out_separated = os.path.join(OUT_DIR, "warp_rings_separated.png")
    scene.render.filepath = out_separated
    bpy.ops.render.render(write_still=True)
    print("Rendered:", out_separated)
    
    import shutil
    shutil.copyfile(out_separated, os.path.join(DOCS_DIR, "warp_rings_separated.png"))
    
    # View 2: Top-down view showing concentric rings with vacuum gap
    cam_obj.location = Vector((0.0, 0.0, 2.5))
    cam_obj.rotation_euler = (0.0, 0.0, 1.5708)
    out_top = os.path.join(OUT_DIR, "warp_rings_top.png")
    scene.render.filepath = out_top
    bpy.ops.render.render(write_still=True)
    print("Rendered:", out_top)
    shutil.copyfile(out_top, os.path.join(DOCS_DIR, "warp_rings_top.png"))

if __name__ == "__main__":
    main()
