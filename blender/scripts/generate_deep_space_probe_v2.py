"""Generate an ultra-detailed Deep Space Science & Relay Probe (Probe v2).

Creates high-fidelity geometric details:
  - 12-sided faceted avionics bus with thermal MLI foil & radiator louvers
  - Cassegrain high-gain parabolic antenna with tripod subreflector & waveguide
  - Dual articulating solar wings with gold busbars, frame trusses & hinges
  - 4x RCS quad-thruster blocks with titanium rocket nozzles
  - Ion propulsion drive with glowing blue xenon grid emitter
  - Star trackers with optical coated lenses
  - Rear magnetometer boom with fluxgate sensor package
  - Whip antennas & wingtip LED navigation strobes
Renders a cinematic 1280x720 Cycles hero showcase to docs/screens/probe_showcase_v2.png.
"""
import math
import os
import bpy
from mathutils import Matrix, Vector

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
PROJECT_ROOT = os.path.dirname(SCRIPT_DIR)
REPO_ROOT = os.path.dirname(PROJECT_ROOT)
STARMAP = os.path.join(REPO_ROOT, "res", "textures", "starmap_2020_4k_gal.exr")
OUT_PNG = os.path.join(REPO_ROOT, "docs", "screens", "probe_showcase_v2.png")
OUT_OBJ = os.path.join(REPO_ROOT, "res", "models", "probe_v2.obj")

def clean_scene():
    bpy.ops.wm.read_factory_settings(use_empty=True)

def create_pbr_material(name, color, metallic, roughness, bump_strength=0.0,
                        emission_color=None, emission_strength=0.0):
    mat = bpy.data.materials.new(name)
    mat.use_nodes = True
    nt = mat.node_tree
    nt.nodes.clear()

    out = nt.nodes.new("ShaderNodeOutputMaterial")
    out.location = (400, 0)
    bsdf = nt.nodes.new("ShaderNodeBsdfPrincipled")
    bsdf.location = (0, 0)
    nt.links.new(bsdf.outputs["BSDF"], out.inputs["Surface"])

    bsdf.inputs["Base Color"].default_value = (color[0], color[1], color[2], 1.0)
    bsdf.inputs["Metallic"].default_value = metallic
    bsdf.inputs["Roughness"].default_value = roughness

    if bump_strength > 0:
        # Procedural micro-wrinkle / texture bump
        noise = nt.nodes.new("ShaderNodeTexNoise")
        noise.location = (-400, -200)
        noise.inputs["Scale"].default_value = 45.0
        noise.inputs["Detail"].default_value = 12.0
        noise.inputs["Roughness"].default_value = 0.65

        bump = nt.nodes.new("ShaderNodeBump")
        bump.location = (-180, -200)
        bump.inputs["Strength"].default_value = bump_strength
        bump.inputs["Distance"].default_value = 0.05
        nt.links.new(noise.outputs["Fac"], bump.inputs["Height"])
        nt.links.new(bump.outputs["Normal"], bsdf.inputs["Normal"])

    if emission_color and emission_strength > 0:
        if "Emission Color" in bsdf.inputs:
            bsdf.inputs["Emission Color"].default_value = (
                emission_color[0], emission_color[1], emission_color[2], 1.0
            )
            bsdf.inputs["Emission Strength"].default_value = emission_strength
        else:
            bsdf.inputs["Emission"].default_value = (
                emission_color[0], emission_color[1], emission_color[2], 1.0
            )

    return mat

def build_probe_v2():
    # Materials
    m_gold_foil = create_pbr_material(
        "GoldMLI", (0.95, 0.72, 0.18), metallic=0.92, roughness=0.25, bump_strength=0.35
    )
    m_silver_radiator = create_pbr_material(
        "SilverRadiator", (0.85, 0.88, 0.92), metallic=0.95, roughness=0.18, bump_strength=0.10
    )
    m_carbon_dish = create_pbr_material(
        "CarbonDish", (0.12, 0.13, 0.15), metallic=0.15, roughness=0.35, bump_strength=0.20
    )
    m_dish_face = create_pbr_material(
        "DishFace", (0.92, 0.94, 0.96), metallic=0.20, roughness=0.28, bump_strength=0.08
    )
    m_titanium = create_pbr_material(
        "Titanium", (0.55, 0.58, 0.62), metallic=0.85, roughness=0.30
    )
    m_solar_cell = create_pbr_material(
        "SolarCell", (0.04, 0.12, 0.38), metallic=0.45, roughness=0.15, bump_strength=0.15
    )
    m_solar_gold = create_pbr_material(
        "SolarGoldBusbar", (0.92, 0.78, 0.22), metallic=0.95, roughness=0.20
    )
    m_dark_composite = create_pbr_material(
        "DarkComposite", (0.06, 0.07, 0.08), metallic=0.10, roughness=0.45
    )
    m_ion_glow = create_pbr_material(
        "IonGlow", (0.15, 0.65, 1.0), metallic=0.0, roughness=0.2,
        emission_color=(0.20, 0.75, 1.0), emission_strength=8.0
    )
    m_cyan_strobe = create_pbr_material(
        "CyanStrobe", (0.1, 0.9, 0.8), metallic=0.0, roughness=0.1,
        emission_color=(0.2, 1.0, 0.85), emission_strength=10.0
    )
    m_amber_beacon = create_pbr_material(
        "AmberBeacon", (1.0, 0.6, 0.1), metallic=0.0, roughness=0.1,
        emission_color=(1.0, 0.65, 0.15), emission_strength=8.0
    )
    m_optics = create_pbr_material(
        "CoatedOptics", (0.02, 0.05, 0.15), metallic=0.1, roughness=0.05
    )

    objects = []

    # 1. Main Avionics Bus: 12-sided prism with beveled profile
    bpy.ops.mesh.primitive_cylinder_add(
        vertices=12, radius=0.45, depth=0.85, location=(0, 0, 0)
    )
    bus = bpy.context.active_object
    bus.name = "AvionicsBus"
    bus.data.materials.append(m_gold_foil)
    objects.append(bus)

    # 2. Thermal Radiator Panels on 4 facets
    for angle_deg in (0, 90, 180, 270):
        rad = math.radians(angle_deg)
        x = math.cos(rad) * 0.455
        y = math.sin(rad) * 0.455
        bpy.ops.mesh.primitive_cube_add(size=1.0, location=(x, y, 0))
        rad_panel = bpy.context.active_object
        rad_panel.scale = (0.02, 0.22, 0.36)
        rad_panel.rotation_euler = (0, 0, rad)
        rad_panel.data.materials.append(m_silver_radiator)
        objects.append(rad_panel)

        # Louver ribs on radiator
        for louver_z in (-0.25, -0.15, -0.05, 0.05, 0.15, 0.25):
            bpy.ops.mesh.primitive_cube_add(
                size=1.0, location=(x * 1.02, y * 1.02, louver_z)
            )
            louver = bpy.context.active_object
            louver.scale = (0.015, 0.20, 0.015)
            louver.rotation_euler = (0, 0, rad)
            louver.data.materials.append(m_titanium)
            objects.append(louver)

    # 3. Structural Rings (Fore and Aft octagonal clamping collars)
    for z in (-0.43, 0.43):
        bpy.ops.mesh.primitive_cylinder_add(
            vertices=24, radius=0.47, depth=0.06, location=(0, 0, z)
        )
        collar = bpy.context.active_object
        collar.data.materials.append(m_titanium)
        objects.append(collar)

    # 4. High-Gain Parabolic Antenna Dish (Forward facing: +Z)
    dish_z = 0.58
    bpy.ops.mesh.primitive_cone_add(
        vertices=32, radius1=0.72, radius2=0.08, depth=0.24, location=(0, 0, dish_z + 0.12)
    )
    dish = bpy.context.active_object
    dish.rotation_euler = (math.pi, 0, 0)
    dish.data.materials.append(m_dish_face)
    objects.append(dish)

    # Dish rear carbon-composite backing ribs
    for a in range(8):
        ang = a * (math.pi / 4.0)
        bpy.ops.mesh.primitive_cylinder_add(
            vertices=8, radius=0.015, depth=0.70, location=(0, 0, dish_z + 0.08)
        )
        rib = bpy.context.active_object
        rib.scale = (1, 1, 1)
        rib.rotation_euler = (math.sin(ang)*0.3, math.cos(ang)*0.3, ang)
        rib.data.materials.append(m_carbon_dish)
        objects.append(rib)

    # Tripod Sub-Reflector Feed
    feed_z = dish_z + 0.42
    bpy.ops.mesh.primitive_cone_add(
        vertices=16, radius1=0.10, radius2=0.02, depth=0.10, location=(0, 0, feed_z)
    )
    feed_horn = bpy.context.active_object
    feed_horn.data.materials.append(m_gold_foil)
    objects.append(feed_horn)

    for i in range(3):
        leg_ang = i * (math.pi * 2.0 / 3.0)
        leg_x = math.cos(leg_ang) * 0.35
        leg_y = math.sin(leg_ang) * 0.35
        # Strut from dish rim to feed
        bpy.ops.mesh.primitive_cylinder_add(
            vertices=8, radius=0.012, depth=0.55,
            location=(leg_x * 0.5, leg_y * 0.5, dish_z + 0.22)
        )
        strut = bpy.context.active_object
        dx = -leg_x
        dy = -leg_y
        dz = 0.35
        strut.rotation_euler = Vector((dx, dy, dz)).normalized().to_track_quat("-Z", "Y").to_euler()
        strut.data.materials.append(m_titanium)
        objects.append(strut)

    # 5. Ion Propulsion Drive (Stern: -Z)
    bpy.ops.mesh.primitive_cone_add(
        vertices=24, radius1=0.26, radius2=0.14, depth=0.22, location=(0, 0, -0.54)
    )
    thruster_bell = bpy.context.active_object
    thruster_bell.data.materials.append(m_titanium)
    objects.append(thruster_bell)

    # Glowing Xenon Ion Grid inside nozzle
    bpy.ops.mesh.primitive_cylinder_add(
        vertices=16, radius=0.16, depth=0.03, location=(0, 0, -0.62)
    )
    ion_grid = bpy.context.active_object
    ion_grid.data.materials.append(m_ion_glow)
    objects.append(ion_grid)

    # 6. RCS Quad-Thruster Blocks (4 corners at Z = 0)
    for corner_ang in (45, 135, 225, 315):
        c_rad = math.radians(corner_ang)
        cx = math.cos(c_rad) * 0.48
        cy = math.sin(c_rad) * 0.48
        bpy.ops.mesh.primitive_cube_add(size=1.0, location=(cx, cy, 0.15))
        rcs_body = bpy.context.active_object
        rcs_body.scale = (0.08, 0.08, 0.08)
        rcs_body.data.materials.append(m_titanium)
        objects.append(rcs_body)

        # 4 micro nozzles per block
        for nd in ((1, 0, 0), (-1, 0, 0), (0, 0, 1), (0, 0, -1)):
            nx = cx + nd[0] * 0.05
            ny = cy + nd[1] * 0.05
            nz = 0.15 + nd[2] * 0.05
            bpy.ops.mesh.primitive_cone_add(
                vertices=8, radius1=0.025, radius2=0.008, depth=0.04, location=(nx, ny, nz)
            )
            micro_nozzle = bpy.context.active_object
            micro_nozzle.rotation_euler = Vector(nd).normalized().to_track_quat("-Z", "Y").to_euler()
            micro_nozzle.data.materials.append(m_titanium)
            objects.append(micro_nozzle)

    # 7. Dual Articulating Solar Array Wings (+X and -X)
    for side in (-1, 1):
        # Yoke / Boom mount
        bpy.ops.mesh.primitive_cylinder_add(
            vertices=12, radius=0.035, depth=0.25, location=(side * 0.58, 0, 0)
        )
        yoke = bpy.context.active_object
        yoke.rotation_euler = (0, math.pi / 2.0, 0)
        yoke.data.materials.append(m_titanium)
        objects.append(yoke)

        # Main panel frame
        panel_x = side * 1.55
        bpy.ops.mesh.primitive_cube_add(size=1.0, location=(panel_x, 0, 0))
        frame = bpy.context.active_object
        frame.scale = (1.70, 0.72, 0.03)
        frame.data.materials.append(m_dark_composite)
        objects.append(frame)

        # Solar cell grid inserts (2 rows of 4 cells)
        for rx in range(4):
            cx = side * (0.85 + rx * 0.45)
            for rz in (-0.18, 0.18):
                bpy.ops.mesh.primitive_cube_add(
                    size=1.0, location=(cx, 0.018, rz)
                )
                cell = bpy.context.active_object
                cell.scale = (0.38, 0.008, 0.32)
                cell.data.materials.append(m_solar_cell)
                objects.append(cell)

                # Gold interconnect busbar lines
                bpy.ops.mesh.primitive_cube_add(
                    size=1.0, location=(cx, 0.022, rz)
                )
                bar = bpy.context.active_object
                bar.scale = (0.36, 0.005, 0.02)
                bar.data.materials.append(m_solar_gold)
                objects.append(bar)

        # Wingtip navigation strobe light
        bpy.ops.mesh.primitive_uv_sphere_add(
            segments=12, ring_count=8, radius=0.04,
            location=(side * 2.44, 0, 0.36)
        )
        tip_strobe = bpy.context.active_object
        tip_strobe.data.materials.append(m_cyan_strobe if side > 0 else m_amber_beacon)
        objects.append(tip_strobe)

    # 8. Star Trackers (Optical Baffles on bus side)
    for st_ang in (30, -30):
        s_rad = math.radians(st_ang)
        sx = math.cos(s_rad) * 0.46
        sy = math.sin(s_rad) * 0.46
        bpy.ops.mesh.primitive_cylinder_add(
            vertices=16, radius=0.06, depth=0.18, location=(sx, sy, -0.15)
        )
        st_tube = bpy.context.active_object
        st_tube.rotation_euler = (0.4, 0, s_rad)
        st_tube.data.materials.append(m_dark_composite)
        objects.append(st_tube)

        # Lens element
        bpy.ops.mesh.primitive_cylinder_add(
            vertices=16, radius=0.05, depth=0.02, location=(sx * 1.08, sy * 1.08, -0.10)
        )
        lens = bpy.context.active_object
        lens.rotation_euler = (0.4, 0, s_rad)
        lens.data.materials.append(m_optics)
        objects.append(lens)

    # 9. Magnetometer Boom extending backwards (-Z and angle)
    boom_pts = [
        Vector((0, 0, -0.45)),
        Vector((0.15, -0.25, -1.15)),
        Vector((0.25, -0.45, -1.85))
    ]
    for seg in range(len(boom_pts) - 1):
        p1 = boom_pts[seg]
        p2 = boom_pts[seg + 1]
        mid = (p1 + p2) * 0.5
        dir_vec = p2 - p1
        bpy.ops.mesh.primitive_cylinder_add(
            vertices=8, radius=0.025, depth=dir_vec.length, location=mid
        )
        boom_seg = bpy.context.active_object
        boom_seg.rotation_euler = dir_vec.to_track_quat("-Z", "Y").to_euler()
        boom_seg.data.materials.append(m_carbon_dish)
        objects.append(boom_seg)

    # Magnetometer Sensor Canister at tip
    bpy.ops.mesh.primitive_cylinder_add(
        vertices=16, radius=0.08, depth=0.15, location=boom_pts[-1]
    )
    mag_sensor = bpy.context.active_object
    mag_sensor.data.materials.append(m_gold_foil)
    objects.append(mag_sensor)

    # 10. Whip Antennas (2x long slender dipole elements)
    for w_ang in (110, 250):
        w_rad = math.radians(w_ang)
        wx = math.cos(w_rad) * 0.44
        wy = math.sin(w_rad) * 0.44
        bpy.ops.mesh.primitive_cylinder_add(
            vertices=6, radius=0.006, depth=1.40, location=(wx * 1.6, wy * 1.6, 0.40)
        )
        whip = bpy.context.active_object
        whip.rotation_euler = (0.45, 0.35, w_rad)
        whip.data.materials.append(m_titanium)
        objects.append(whip)

    # Join or parent all objects
    for o in objects:
        for poly in o.data.polygons:
            poly.use_smooth = True

    return objects

def setup_render_and_lighting(scene):
    scene.render.engine = "CYCLES"
    scene.cycles.device = "CPU"
    scene.render.resolution_x = 1280
    scene.render.resolution_y = 720
    scene.cycles.samples = 64
    scene.cycles.use_denoising = True
    scene.view_settings.exposure = 0.85

    world = bpy.data.worlds.new("ProbeV2World")
    scene.world = world
    world.use_nodes = True
    nt = world.node_tree
    bg = nt.nodes.get("Background")
    if bg is None:
        bg = nt.nodes.new("ShaderNodeBackground")
        out = nt.nodes.get("World Output")
        nt.links.new(bg.outputs["Background"], out.inputs["Surface"])

    if os.path.isfile(STARMAP):
        env = nt.nodes.new("ShaderNodeTexEnvironment")
        env.image = bpy.data.images.load(STARMAP, check_existing=True)
        nt.links.new(env.outputs["Color"], bg.inputs["Color"])
        bg.inputs["Strength"].default_value = 0.42
        ambient = nt.nodes.new("ShaderNodeBackground")
        ambient.inputs["Color"].default_value = (0.015, 0.022, 0.040, 1.0)
        ambient.inputs["Strength"].default_value = 0.32
        add = nt.nodes.new("ShaderNodeAddShader")
        out = nt.nodes.get("World Output")
        nt.links.new(bg.outputs["Background"], add.inputs[0])
        nt.links.new(ambient.outputs["Background"], add.inputs[1])
        nt.links.new(add.outputs["Shader"], out.inputs["Surface"])

    # Hero camera: dramatic 3/4 perspective catching dish, gold MLI body, solar cells, RCS and ion thruster
    cam_data = bpy.data.cameras.new("HeroCam")
    cam_data.lens = 45.0
    cam = bpy.data.objects.new("HeroCam", cam_data)
    cam.location = (2.8, -3.2, 1.8)
    target = Vector((0.0, 0.0, 0.1))
    cam.rotation_euler = (target - cam.location).to_track_quat("-Z", "Y").to_euler()
    scene.collection.objects.link(cam)
    scene.camera = cam

    # Directional Key Sunlight (intense space sun hitting dish face and gold bus)
    key_data = bpy.data.lights.new("SunKey", type="SUN")
    key_data.energy = 6.5
    key_data.color = (1.0, 0.98, 0.92)
    key = bpy.data.objects.new("SunKey", key_data)
    key.rotation_euler = Vector((-1.0, 1.2, -0.6)).normalized().to_track_quat("-Z", "Y").to_euler()
    scene.collection.objects.link(key)

    # Blue Rim Light
    rim_data = bpy.data.lights.new("Rim", type="SUN")
    rim_data.energy = 4.0
    rim_data.color = (0.35, 0.70, 1.0)
    rim = bpy.data.objects.new("Rim", rim_data)
    rim.rotation_euler = Vector((1.0, -1.0, 0.8)).normalized().to_track_quat("-Z", "Y").to_euler()
    scene.collection.objects.link(rim)

    # Soft Fill Light
    fill_data = bpy.data.lights.new("Fill", type="AREA")
    fill_data.energy = 1100.0
    fill_data.size = 8.0
    fill_data.color = (0.75, 0.85, 1.0)
    fill = bpy.data.objects.new("Fill", fill_data)
    fill.location = (3.2, -3.8, 2.2)
    fill.rotation_euler = (target - fill.location).to_track_quat("-Z", "Y").to_euler()
    scene.collection.objects.link(fill)

def main():
    print("Building Deep Space Probe v2 model...")
    clean_scene()
    build_probe_v2()

    # Tilt the whole spacecraft slightly to showcase all angles
    for o in bpy.data.objects:
        if o.type == "MESH":
            o.rotation_euler.rotate(Matrix.Rotation(math.radians(-25), 3, "X").to_euler())
            o.rotation_euler.rotate(Matrix.Rotation(math.radians(35), 3, "Z").to_euler())

    scene = bpy.context.scene
    setup_render_and_lighting(scene)

    os.makedirs(os.path.dirname(OUT_PNG), exist_ok=True)
    scene.render.filepath = OUT_PNG
    print(f"Rendering Probe v2 showcase to {OUT_PNG}...")
    bpy.ops.render.render(write_still=True)
    print("Showcase rendered successfully!")

if __name__ == "__main__":
    main()
