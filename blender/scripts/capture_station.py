"""
Blender script to render cycles_station_check.png with camera framing
matching in-game:
- Target: docking port IDA-2 collar point Vector((0.0, -0.041885, 0.422339))
- Camera direction: normalize(Vector((0.50, 0.30, 0.82)))
- Distance: 4.45 / 0.38 = 11.7105 units (or scaled by 0.38: distance 4.45)
- FOV: 7.5 degrees (vertical)
- Cycles engine: 64 samples
- Output: blender/out/cycles_station_check.png
"""
import bpy
import math
import os
from mathutils import Vector, Matrix

def setup_cycles_check():
    # Clear all objects in the scene
    bpy.ops.wm.read_factory_settings(use_empty=True)
    scene = bpy.context.scene

    # Import station OBJ
    obj_path = os.path.abspath("res/models/station.obj")
    print(f"Importing OBJ: {obj_path}")
    bpy.ops.wm.obj_import(filepath=obj_path)

    # Find imported station object
    station = None
    for obj in bpy.context.selected_objects:
        if obj.type == 'MESH':
            station = obj
            break
    if not station:
        raise RuntimeError("No mesh object imported from station.obj")

    # Scale to in-game size (0.38) and apply in-game rotation around Y (0.08 rad at t=1.0)
    station.scale = (0.38, 0.38, 0.38)
    station.rotation_euler = (0.0, 0.08, 0.0)
    bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)

    # Setup PBR material with baked textures
    mat = bpy.data.materials.new(name="Station_PBR")
    mat.use_nodes = True
    nodes = mat.node_tree.nodes
    links = mat.node_tree.links
    nodes.clear()

    output_node = nodes.new(type='ShaderNodeOutputMaterial')
    bsdf = nodes.new(type='ShaderNodeBsdfPrincipled')
    links.new(bsdf.outputs['BSDF'], output_node.inputs['Surface'])

    # Helper to load texture
    def load_tex(path, is_color=True):
        img = bpy.data.images.load(os.path.abspath(path))
        if not is_color:
            img.colorspace_settings.name = 'Non-Color'
        tex_node = nodes.new(type='ShaderNodeTexImage')
        tex_node.image = img
        return tex_node

    albedo_node = load_tex("res/models/station_albedo.bmp", is_color=True)
    links.new(albedo_node.outputs['Color'], bsdf.inputs['Base Color'])

    rough_node = load_tex("res/models/station_rough.bmp", is_color=False)
    links.new(rough_node.outputs['Color'], bsdf.inputs['Roughness'])

    metal_node = load_tex("res/models/station_metal.bmp", is_color=False)
    links.new(metal_node.outputs['Color'], bsdf.inputs['Metallic'])

    normal_img = load_tex("res/models/station_normal.bmp", is_color=False)
    normal_map = nodes.new(type='ShaderNodeNormalMap')
    links.new(normal_img.outputs['Color'], normal_map.inputs['Color'])
    links.new(normal_map.outputs['Normal'], bsdf.inputs['Normal'])

    emissive_node = load_tex("res/models/station_emissive.bmp", is_color=True)
    if 'Emission Color' in bsdf.inputs:
        links.new(emissive_node.outputs['Color'], bsdf.inputs['Emission Color'])
    elif 'Emission' in bsdf.inputs:
        links.new(emissive_node.outputs['Color'], bsdf.inputs['Emission'])

    station.data.materials.clear()
    station.data.materials.append(mat)

    # Render settings
    scene.render.engine = 'CYCLES'
    scene.cycles.device = 'CPU'
    scene.cycles.samples = 64
    scene.render.resolution_x = 1280
    scene.render.resolution_y = 720
    scene.render.image_settings.file_format = 'PNG'
    scene.render.image_settings.color_mode = 'RGB'
    scene.view_settings.view_transform = 'Standard'

    # World background (deep space matching skybox)
    world = bpy.data.worlds.new("SpaceWorld")
    scene.world = world
    world.use_nodes = True
    bg = world.node_tree.nodes.get("Background")
    if bg:
        # Match in-game clear color [38, 38, 40] in 0-255 range -> ~0.15 linear
        bg.inputs["Color"].default_value = (0.015, 0.015, 0.018, 1.0)
        bg.inputs["Strength"].default_value = 1.0

    # Lighting: Sun key light from (0.55, 0.45, 0.70)
    sun_data = bpy.data.lights.new("Sun", type='SUN')
    sun_data.energy = 4.2
    sun_data.color = (1.0, 0.95, 0.88)
    sun_obj = bpy.data.objects.new("Sun", sun_data)
    key_dir = Vector((0.55, 0.45, 0.70)).normalized()
    sun_obj.rotation_euler = (-key_dir).to_track_quat('-Z', 'Y').to_euler()
    scene.collection.objects.link(sun_obj)

    # Fill light from below (Earth-shine bounce)
    earth_data = bpy.data.lights.new("EarthBounce", type='SUN')
    earth_data.energy = 1.2
    earth_data.color = (0.22, 0.55, 0.95)
    earth_obj = bpy.data.objects.new("EarthBounce", earth_data)
    earth_dir = Vector((-0.2, -0.9, 0.3)).normalized()
    earth_obj.rotation_euler = (-earth_dir).to_track_quat('-Z', 'Y').to_euler()
    scene.collection.objects.link(earth_obj)

    # Camera setup:
    rotY = 0.08
    dock_z = 0.422339 * 0.38
    dock_y = -0.041885 * 0.38
    dock_target = Vector((dock_z * math.sin(rotY), dock_y, dock_z * math.cos(rotY)))

    loc_dir = Vector((0.50, 0.30, 0.82)).normalized()
    dir_world = Vector((
        loc_dir.x * math.cos(rotY) + loc_dir.z * math.sin(rotY),
        loc_dir.y,
        -loc_dir.x * math.sin(rotY) + loc_dir.z * math.cos(rotY)
    )).normalized()

    cam_dist = 4.45
    cam_pos = dock_target + dir_world * cam_dist

    cam_data = bpy.data.cameras.new("CheckCam")
    cam_data.sensor_fit = 'VERTICAL'
    cam_data.lens_unit = 'FOV'
    cam_data.angle = math.radians(7.5)

    fwd = (dock_target - cam_pos).normalized()
    up = Vector((0.0, 1.0, 0.0))
    right = fwd.cross(up).normalized()
    cam_up = right.cross(fwd).normalized()
    R = Matrix([right, cam_up, -fwd]).transposed()
    cam_obj = bpy.data.objects.new("CheckCam", cam_data)
    cam_obj.matrix_world = Matrix.Translation(cam_pos) @ R.to_4x4()
    scene.collection.objects.link(cam_obj)
    scene.camera = cam_obj

    # Render
    out_path = os.path.abspath("blender/out/cycles_station_check.png")
    scene.render.filepath = out_path
    print(f"Rendering Cycles check to: {out_path}")
    bpy.ops.render.render(write_still=True)
    print("Cycles render complete!")

if __name__ == '__main__':
    setup_cycles_check()
