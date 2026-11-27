"""Repair 3D OBJ mesh artifacts (degenerate faces, inverted/bent normals, zero-UV slivers)
in Blender 5.2+ while preserving existing UV layouts so baked PBR BMP maps stay aligned.

Run:
  & "C:\\Program Files\\Blender Foundation\\Blender 5.2\\blender.exe" -b -P tools/repair_obj_artifacts.py
"""

import glob
import math
import os
import shutil
import bpy
import bmesh

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
MODELS_DIR = os.path.join(ROOT, "res", "models")


def clean_zero_uv_slivers(bm, uv_layer):
    """Give any remaining zero-area UV triangles a tiny valid epsilon offset so dFdx/dFdy never collapses."""
    fixed = 0
    for f in bm.faces:
        if len(f.loops) < 3:
            continue
        u0, v0 = f.loops[0][uv_layer].uv
        u1, v1 = f.loops[1][uv_layer].uv
        u2, v2 = f.loops[2][uv_layer].uv
        area2 = abs((u1 - u0) * (v2 - v0) - (u2 - u0) * (v1 - v0))
        if area2 < 1e-10:
            # Tiny non-degenerate triangle around (u0, v0) clamped safely inside [0, 1]
            eps = 2e-4
            cu = min(max(u0, 0.0), 1.0)
            cv = min(max(v0, 0.0), 1.0)
            du = -eps if cu > 0.5 else eps
            dv = -eps if cv > 0.5 else eps
            f.loops[0][uv_layer].uv = (cu, cv)
            f.loops[1][uv_layer].uv = (cu + du, cv)
            f.loops[2][uv_layer].uv = (cu, cv + dv)
            fixed += 1
    return fixed


def repair_model(filename, smooth_angle_deg=38.0, merge_doubles=False, recalc_normals=True, fix_uv_slivers=False, split_nonmanifold=False):
    filepath = os.path.join(MODELS_DIR, filename)
    if not os.path.exists(filepath):
        print(f"[SKIP] Missing {filename}")
        return

    bpy.ops.wm.read_factory_settings(use_empty=True)
    bpy.ops.wm.obj_import(filepath=filepath, forward_axis='Y', up_axis='Z')
    objs = [o for o in bpy.context.scene.objects if o.type == 'MESH']
    if not objs:
        print(f"[SKIP] No meshes in {filename}")
        return

    bpy.ops.object.select_all(action='DESELECT')
    for o in objs:
        o.select_set(True)
    bpy.context.view_layer.objects.active = objs[0]
    if len(objs) > 1:
        bpy.ops.object.join()
    obj = bpy.context.view_layer.objects.active
    obj.name = os.path.splitext(filename)[0]

    bpy.ops.object.select_all(action='DESELECT')
    obj.select_set(True)
    bpy.context.view_layer.objects.active = obj
    if obj.data.has_custom_normals:
        bpy.ops.object.mode_set(mode='EDIT')
        bpy.ops.mesh.customdata_custom_splitnormals_clear()
        bpy.ops.object.mode_set(mode='OBJECT')

    bpy.ops.object.mode_set(mode='EDIT')
    bm = bmesh.from_edit_mesh(obj.data)
    if merge_doubles:
        bmesh.ops.remove_doubles(bm, verts=bm.verts, dist=1e-5)
    bmesh.ops.dissolve_degenerate(bm, dist=1e-5)
    non_tris = [f for f in bm.faces if len(f.verts) > 3]
    if non_tris:
        bmesh.ops.triangulate(bm, faces=non_tris, quad_method='BEAUTY', ngon_method='BEAUTY')
    tiny = [f for f in bm.faces if f.calc_area() < 1e-8]
    if tiny:
        bmesh.ops.delete(bm, geom=tiny, context='FACES')

    if split_nonmanifold:
        ang_rad = math.radians(smooth_angle_deg if smooth_angle_deg is not None else 38.0)
        bad_edges = [
            e for e in bm.edges
            if (not e.is_manifold) or (len(e.link_faces) == 2 and e.calc_face_angle(0.0) > ang_rad)
        ]
        if bad_edges:
            bmesh.ops.split_edges(bm, edges=bad_edges)

    uv_layer = bm.loops.layers.uv.active
    uv_fixed = 0
    if fix_uv_slivers and uv_layer is not None:
        uv_fixed = clean_zero_uv_slivers(bm, uv_layer)

    bmesh.update_edit_mesh(obj.data)
    if recalc_normals:
        bpy.ops.mesh.select_all(action='SELECT')
        bpy.ops.mesh.normals_make_consistent(inside=False)
    bpy.ops.object.mode_set(mode='OBJECT')

    # Apply crisp split shading by angle so sharp creases never bend smooth normals inward
    bpy.ops.object.select_all(action='DESELECT')
    obj.select_set(True)
    bpy.context.view_layer.objects.active = obj
    if smooth_angle_deg is not None:
        bpy.ops.object.shade_smooth_by_angle(angle=math.radians(smooth_angle_deg), keep_sharp_edges=True)

    bpy.ops.wm.obj_export(
        filepath=filepath,
        export_selected_objects=True,
        export_normals=True,
        export_uv=True,
        export_materials=False,
        export_triangulated_mesh=True,
        forward_axis='Y',
        up_axis='Z',
    )
    print(f"[REPAIRED] {filename}: verts={len(obj.data.vertices)} polys={len(obj.data.polygons)} tiny_removed={len(tiny)} uv_fixed={uv_fixed}")


def main():
    # 1. Warp Ship hero asset: split non-manifold & sharp creases so vertex normals never flip across hard seams
    repair_model("warp_ship.obj", smooth_angle_deg=35.0, merge_doubles=False, recalc_normals=True, fix_uv_slivers=True, split_nonmanifold=True)
    shutil.copyfile(os.path.join(MODELS_DIR, "warp_ship.obj"),
                    os.path.join(MODELS_DIR, "warp_ship_separated.obj"))
    print("[SYNC] Copied repaired warp_ship.obj -> warp_ship_separated.obj")

    # 2. Fleet/colony/probe/station assets with degenerate faces, flipped normals, or zero-UV slivers
    # (Clean models station, telescope, shuttle, warp_bubble, warp_drive_vis are left untouched.)
    for name, recalc, fix_uv, split_nm in (
        ("warp_core.obj", True, True, True),
        ("voyager.obj", False, True, False),
        ("station_megastructure.obj", False, True, False),
        ("bridge_interior.obj", False, True, False),
        ("ark_ship.obj", False, True, False),
        ("rover.obj", True, True, False),
        ("probe.obj", True, True, False),
        ("mining_dreadnought.obj", False, True, False),
        ("colony.obj", True, True, False),
        ("lander.obj", True, True, False),
        ("mir.obj", False, True, False),
        ("dragon.obj", False, True, False),
        ("sail.obj", True, True, False),
        ("dart.obj", True, True, False),
        ("station_aurora.obj", False, True, False),
        ("station_aurora_ring.obj", False, True, False),
        ("soyuz.obj", False, True, False),
        ("jumpgate.obj", False, True, False),
    ):
        repair_model(name, smooth_angle_deg=38.0, merge_doubles=False, recalc_normals=recalc, fix_uv_slivers=fix_uv, split_nonmanifold=split_nm)

    # 3. Planetary colony infrastructure buildings (eliminate collapsed bevel/zero-area triangles)
    for cp in sorted(glob.glob(os.path.join(MODELS_DIR, "colony_*.obj"))):
        cname = os.path.basename(cp)
        recalc_col = cname in ("colony_pad.obj", "colony_school.obj")
        repair_model(cname, smooth_angle_deg=38.0, merge_doubles=False, recalc_normals=recalc_col, fix_uv_slivers=True)


if __name__ == "__main__":
    main()
