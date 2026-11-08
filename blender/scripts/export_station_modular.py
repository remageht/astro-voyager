"""Export modular station meshes from the ISS generator.

Builds the same station as generate_station.py (single monolithic mesh
ISS_Space_Station normalized to wingspan 1.0), then:
  - separates material slots into solar panels (Mat_Solar) vs core,
  - creates an artificial-gravity centrifuge ring in *normalized* coords.

Exports:
  res/models/station.obj        monolith = core + panels + ring (backward compat)
  res/models/station_core.obj   station without solar blankets
  res/models/station_ring.obj   centrifuge torus + spokes + counterweights
  res/models/station_panels.obj solar blankets (Mat_Solar polys only)

All share the SAME 4-zone UV atlas (PBR bake done once by generate_station.py).
"""
import os, sys, math, shutil, bpy
from mathutils import Vector

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
REPO_ROOT = os.path.dirname(os.path.dirname(SCRIPT_DIR))
RES_MODELS = os.path.join(REPO_ROOT, "res", "models")
OUT_SHOWCASE = os.path.join(REPO_ROOT, "blender", "out", "final_showcase")
BUILD_GL_MODELS = os.path.join(REPO_ROOT, "build-gl", "Release", "res", "models")
BUILD_MODELS = os.path.join(REPO_ROOT, "build", "Release", "res", "models")

os.makedirs(RES_MODELS, exist_ok=True)
os.makedirs(OUT_SHOWCASE, exist_ok=True)

sys.path.insert(0, SCRIPT_DIR)
from generate_station import build_space_station

def export_objs(objs, path):
    bpy.ops.object.select_all(action='DESELECT')
    for o in objs:
        o.select_set(True)
    if not bpy.context.selected_objects:
        print("  WARNING: no meshes for " + path + ", skipping")
        return False
    bpy.context.view_layer.objects.active = bpy.context.selected_objects[0]
    bpy.ops.wm.obj_export(
        filepath=path, export_selected_objects=True, export_uv=True,
        export_normals=True, export_materials=False,
        export_triangulated_mesh=True, forward_axis='Y', up_axis='Z')
    print("  Exported " + path + " (" + str(len(bpy.context.selected_objects)) + " objects)")
    return True

def build_ring():
    # Normalized space: station wingspan == 1.0, modules ~0.15 radius.
    bpy.ops.mesh.primitive_torus_add(
        major_radius=0.32, minor_radius=0.055,
        major_segments=48, minor_segments=12, location=(0.0, 0.0, 0.0))
    ring = bpy.context.active_object
    ring.name = "CentrifugeRing"
    for i in range(4):
        ang = i * math.pi / 2
        bpy.ops.mesh.primitive_cylinder_add(
            vertices=10, radius=0.012, depth=0.32,
            location=(math.cos(ang)*0.16, 0.0, math.sin(ang)*0.16),
            rotation=(math.radians(90), 0, ang))
        s = bpy.context.active_object
        s.name = "RingSpoke_" + str(i)
    for i in range(4):
        ang = i * math.pi / 2 + math.pi / 4
        bpy.ops.mesh.primitive_cube_add(
            size=1.0, location=(math.cos(ang)*0.35, 0.0, math.sin(ang)*0.35))
        w = bpy.context.active_object
        w.name = "RingCounterweight_" + str(i)
        w.scale = (0.05, 0.04, 0.04)
    bpy.ops.object.select_all(action='DESELECT')
    for o in bpy.context.scene.objects:
        if o.name.startswith("CentrifugeRing") or o.name.startswith("RingSpoke") or o.name.startswith("RingCounterweight"):
            o.select_set(True)
    bpy.context.view_layer.objects.active = bpy.context.selected_objects[0]
    bpy.ops.object.join()
    ring_obj = bpy.context.active_object
    ring_obj.name = "StationRing"
    # UVs into USOS quadrant (top-left 0..0.5 x 0.5..1.0) to share the atlas
    bpy.ops.object.mode_set(mode='EDIT')
    bpy.ops.mesh.select_all(action='SELECT')
    bpy.ops.uv.smart_project(angle_limit=1.15, island_margin=0.012)
    bpy.ops.object.mode_set(mode='OBJECT')
    uv = ring_obj.data.uv_layers.active
    xs = [l.uv.x for l in uv.data]
    ys = [l.uv.y for l in uv.data]
    mnx, mxx = min(xs), max(xs)
    mny, mxy = min(ys), max(ys)
    for l in uv.data:
        l.uv.x = 0.008 + ((l.uv.x - mnx) / max(mxx - mnx, 1e-6)) * 0.484
        l.uv.y = 0.508 + ((l.uv.y - mny) / max(mxy - mny, 1e-6)) * 0.484
    return ring_obj

def split_station_by_material(station):
    # Find material index for Mat_Solar
    solar_idx = -1
    for i, m in enumerate(station.data.materials):
        if m and m.name == "Mat_Solar":
            solar_idx = i
            break
    print("Solar material index: " + str(solar_idx) + " of " + str(len(station.data.materials)))
    bpy.ops.object.select_all(action='DESELECT')
    station.select_set(True)
    bpy.context.view_layer.objects.active = station
    bpy.ops.object.mode_set(mode='EDIT')
    bpy.ops.mesh.separate(type='MATERIAL')
    bpy.ops.object.mode_set(mode='OBJECT')
    panels = []
    core = []
    for o in bpy.context.scene.objects:
        if o.type != 'MESH':
            continue
        if o is station:
            continue
        if o.name.startswith("ISS_Space_Station"):
            arrays = [m.name if m else "" for m in o.data.materials]
            if solar_idx >= 0 and len(arrays) == 1 and "Mat_Solar" in arrays[0]:
                panels.append(o)
            else:
                core.append(o)
    if len(station.data.polygons) > 0:
        mats = [m.name if m else "" for m in station.data.materials]
        if solar_idx >= 0 and len(mats) == 1 and "Mat_Solar" in mats[0]:
            panels.append(station)
        else:
            core.append(station)
    else:
        bpy.data.objects.remove(station, do_unlink=True)
    print("Panels objects: " + str(len(panels)) + ", Core objects: " + str(len(core)))
    # Join core into one
    if len(core) > 1:
        bpy.ops.object.select_all(action='DESELECT')
        for o in core:
            o.select_set(True)
        bpy.context.view_layer.objects.active = core[0]
        bpy.ops.object.join()
        core = [bpy.context.active_object]
        core[0].name = "StationCore"
    if len(panels) > 1:
        bpy.ops.object.select_all(action='DESELECT')
        for o in panels:
            o.select_set(True)
        bpy.context.view_layer.objects.active = panels[0]
        bpy.ops.object.join()
        panels = [bpy.context.active_object]
        panels[0].name = "StationPanels"
    return core, panels

def main():
    print("=== Modular station export ===")
    station = build_space_station()
    ring_obj = build_ring()
    core, panels = split_station_by_material(station)
    print("Core polys: " + str(sum(len(o.data.polygons) for o in core)))
    print("Panels polys: " + str(sum(len(o.data.polygons) for o in panels)))
    print("Ring polys: " + str(len(ring_obj.data.polygons)))
    all_meshes = [o for o in bpy.context.scene.objects if o.type == 'MESH']
    export_objs(all_meshes, os.path.join(RES_MODELS, "station.obj"))
    export_objs(core, os.path.join(RES_MODELS, "station_core.obj"))
    export_objs([ring_obj], os.path.join(RES_MODELS, "station_ring.obj"))
    export_objs(panels, os.path.join(RES_MODELS, "station_panels.obj"))
    for n in ["station.obj", "station_core.obj", "station_ring.obj", "station_panels.obj"]:
        s = os.path.join(RES_MODELS, n)
        for d in [BUILD_GL_MODELS, BUILD_MODELS]:
            if os.path.isdir(d) and os.path.isfile(s):
                shutil.copyfile(s, os.path.join(d, n))
    print("=== Done ===")

if __name__ == "__main__":
    main()
