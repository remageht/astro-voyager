import os, sys, math, struct, bpy, bmesh, numpy as np
from mathutils import Vector, Matrix

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
PROJECT_ROOT = os.path.dirname(SCRIPT_DIR)
REPO_ROOT = os.path.dirname(PROJECT_ROOT)
OUT_MODELS = os.path.join(REPO_ROOT, "res", "models")
DOCS_SCREENS = os.path.join(REPO_ROOT, "docs", "screens")
PLASMA_SRC = os.path.join(PROJECT_ROOT, "out", "warp_tunnel_src", "extracted_Image_0.png")

os.makedirs(OUT_MODELS, exist_ok=True)
os.makedirs(DOCS_SCREENS, exist_ok=True)

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
    u8 = (np.clip(rgb_array, 0.0, 1.0) * 255.0).astype(np.uint8) if rgb_array.dtype != np.uint8 else rgb_array
    with open(filepath, 'wb') as f:
        f.write(header)
        for y in range(H):
            row = u8[H - 1 - y]
            f.write(row.tobytes())
            if padding:
                f.write(b'\x00' * padding)
    print("Wrote BMP: " + filepath)

ATLAS = 1024

def generate_textures():
    print("=== Generating Authentic PBR Textures ===")
    plasma_raw = None
    if os.path.exists(PLASMA_SRC):
        bimg = bpy.data.images.load(PLASMA_SRC)
        w, h = bimg.size
        px = np.array(bimg.pixels[:], dtype=np.float32).reshape((h, w, 4))
        plasma_raw = (px[:, :, 0] * 0.299 + px[:, :, 1] * 0.587 + px[:, :, 2] * 0.114)
        bpy.data.images.remove(bimg)
        print("Loaded extracted plasma mist from:", PLASMA_SRC)
    else:
        print("Warning: Plasma image not found, using procedural fallback")
        plasma_raw = np.zeros((256, 256), dtype=np.float32)

    # Upscale / tile plasma to 1024x1024
    tiled_plasma = np.tile(plasma_raw, (4, 4))

    # --- 1. Warp Tunnel Tube Textures ---
    alb_tube = np.zeros((ATLAS, ATLAS, 3), dtype=np.uint8)
    emi_tube = np.zeros((ATLAS, ATLAS, 3), dtype=np.uint8)
    nrm_tube = np.full((ATLAS, ATLAS, 3), (128, 128, 255), dtype=np.uint8)
    rgh_tube = np.full((ATLAS, ATLAS, 3), 14, dtype=np.uint8)
    met_tube = np.full((ATLAS, ATLAS, 3), 225, dtype=np.uint8)

    for y in range(ATLAS):
        v = y / ATLAS
        ring_phase = (v * 8.0) % 1.0
        is_ring_joint = (ring_phase < 0.04) or (ring_phase > 0.96)
        is_glow_ring = (ring_phase > 0.46) and (ring_phase < 0.54)
        
        for x in range(ATLAS):
            u = x / ATLAS
            u_phase = (u * 16.0) % 1.0
            is_panel_seam = (u_phase < 0.03) or (u_phase > 0.97)

            base_val = 6
            if is_panel_seam or is_ring_joint:
                alb_tube[y, x] = (2, 3, 5)
                rgh_tube[y, x] = 40
            else:
                panel_id = int(u * 16.0) + int(v * 8.0) * 16
                tint = (math.sin(panel_id * 12.9898) * 0.5 + 0.5) * 4.0
                val = int(base_val + tint)
                alb_tube[y, x] = (val, int(val * 1.3), int(val * 2.2))

            if is_panel_seam:
                nx = int(128 + (0.5 - u_phase) * 60)
                nrm_tube[y, x, 0] = np.clip(nx, 0, 255)
            if is_ring_joint:
                ny = int(128 + (0.5 - ring_phase) * 60)
                nrm_tube[y, x, 1] = np.clip(ny, 0, 255)

            if is_glow_ring:
                ring_glow = 1.0 - abs(ring_phase - 0.50) / 0.04
                emi_tube[y, x] = (int(25 * ring_glow), int(120 * ring_glow), int(255 * ring_glow))
            elif is_panel_seam and (int(v * 32) % 4 == 0):
                emi_tube[y, x] = (15, 80, 210)

    write_bmp24(os.path.join(OUT_MODELS, "warp_tunnel_tube_albedo.bmp"), alb_tube)
    write_bmp24(os.path.join(OUT_MODELS, "warp_tunnel_tube_normal.bmp"), nrm_tube)
    write_bmp24(os.path.join(OUT_MODELS, "warp_tunnel_tube_rough.bmp"), rgh_tube)
    write_bmp24(os.path.join(OUT_MODELS, "warp_tunnel_tube_metal.bmp"), met_tube)
    write_bmp24(os.path.join(OUT_MODELS, "warp_tunnel_tube_emissive.bmp"), emi_tube)

    # --- 2. Warp Tunnel Scrap / Structural Scaffolding Textures ---
    alb_scrap = np.full((ATLAS, ATLAS, 3), (200, 205, 215), dtype=np.uint8)
    emi_scrap = np.zeros((ATLAS, ATLAS, 3), dtype=np.uint8)
    nrm_scrap = np.full((ATLAS, ATLAS, 3), (128, 128, 255), dtype=np.uint8)
    rgh_scrap = np.full((ATLAS, ATLAS, 3), 48, dtype=np.uint8)
    met_scrap = np.full((ATLAS, ATLAS, 3), 245, dtype=np.uint8)

    neon_b = 255
    neon_g = int(255 * (0.216 / 1.0))
    neon_r = int(255 * (0.145 / 1.0))

    for y in range(ATLAS):
        v = y / ATLAS
        ring_stripe = (v * 16.0) % 1.0
        is_neon_rib = (ring_stripe > 0.40) and (ring_stripe < 0.60)
        for x in range(ATLAS):
            u = x / ATLAS
            long_stripe = (u * 24.0) % 1.0
            is_neon_rail = (long_stripe > 0.42) and (long_stripe < 0.58)

            if is_neon_rib or is_neon_rail:
                glow = 1.0
                emi_scrap[y, x] = (int(neon_r * glow), int(neon_g * glow), int(neon_b * glow))
                alb_scrap[y, x] = (230, 240, 255)
            elif (ring_stripe < 0.08) or (long_stripe < 0.08):
                alb_scrap[y, x] = (150, 155, 165)
                rgh_scrap[y, x] = 80

    write_bmp24(os.path.join(OUT_MODELS, "warp_tunnel_scrap_albedo.bmp"), alb_scrap)
    write_bmp24(os.path.join(OUT_MODELS, "warp_tunnel_scrap_normal.bmp"), nrm_scrap)
    write_bmp24(os.path.join(OUT_MODELS, "warp_tunnel_scrap_rough.bmp"), rgh_scrap)
    write_bmp24(os.path.join(OUT_MODELS, "warp_tunnel_scrap_metal.bmp"), met_scrap)
    write_bmp24(os.path.join(OUT_MODELS, "warp_tunnel_scrap_emissive.bmp"), emi_scrap)

    # --- 3. Warp Tunnel Cloud / Plasma Vortex Textures ---
    alb_cloud = np.zeros((ATLAS, ATLAS, 3), dtype=np.uint8)
    emi_cloud = np.zeros((ATLAS, ATLAS, 3), dtype=np.uint8)
    nrm_cloud = np.full((ATLAS, ATLAS, 3), (128, 128, 255), dtype=np.uint8)
    rgh_cloud = np.full((ATLAS, ATLAS, 3), 10, dtype=np.uint8)
    met_cloud = np.full((ATLAS, ATLAS, 3), 10, dtype=np.uint8)

    cr = 0.559; cg = 0.512; cb = 1.0
    for y in range(ATLAS):
        for x in range(ATLAS):
            val = float(tiled_plasma[y, x])
            val_hdr = math.pow(val, 1.2) * 1.5
            val_hdr = min(1.0, val_hdr)

            r = int(cr * val_hdr * 255)
            g = int(cg * val_hdr * 255)
            b = int(cb * val_hdr * 255)
            alb_cloud[y, x] = (r, g, b)
            emi_cloud[y, x] = (int(r * 0.95), int(g * 0.95), b)

    write_bmp24(os.path.join(OUT_MODELS, "warp_tunnel_cloud_albedo.bmp"), alb_cloud)
    write_bmp24(os.path.join(OUT_MODELS, "warp_tunnel_cloud_normal.bmp"), nrm_cloud)
    write_bmp24(os.path.join(OUT_MODELS, "warp_tunnel_cloud_rough.bmp"), rgh_cloud)
    write_bmp24(os.path.join(OUT_MODELS, "warp_tunnel_cloud_metal.bmp"), met_cloud)
    write_bmp24(os.path.join(OUT_MODELS, "warp_tunnel_cloud_emissive.bmp"), emi_cloud)

    # 4. Backward compatible combined textures
    write_bmp24(os.path.join(OUT_MODELS, "warp_tunnel_albedo.bmp"), alb_tube)
    write_bmp24(os.path.join(OUT_MODELS, "warp_tunnel_normal.bmp"), nrm_tube)
    write_bmp24(os.path.join(OUT_MODELS, "warp_tunnel_rough.bmp"), rgh_tube)
    write_bmp24(os.path.join(OUT_MODELS, "warp_tunnel_metal.bmp"), met_tube)
    write_bmp24(os.path.join(OUT_MODELS, "warp_tunnel_emissive.bmp"), emi_tube)

    print("All PBR textures generated successfully.")

def create_cylinder_mesh(name, radius, length, radial_segs=64, height_segs=16, flip_normals=True):
    mesh = bpy.data.meshes.new(name)
    bm = bmesh.new()
    uv_layer = bm.loops.layers.uv.new("UVMap")

    verts = []
    for h_idx in range(height_segs + 1):
        v_coord = h_idx / height_segs
        y_pos = -length * 0.5 + v_coord * length
        ring_verts = []
        for r_idx in range(radial_segs):
            theta = (r_idx / radial_segs) * 2.0 * math.pi
            x_pos = radius * math.cos(theta)
            z_pos = radius * math.sin(theta)
            v = bm.verts.new((x_pos, y_pos, z_pos))
            ring_verts.append(v)
        verts.append(ring_verts)

    bm.verts.ensure_lookup_table()

    for h_idx in range(height_segs):
        v0 = h_idx / height_segs
        v1 = (h_idx + 1) / height_segs
        for r_idx in range(radial_segs):
            r_next = (r_idx + 1) % radial_segs
            u0 = r_idx / radial_segs
            u1 = (r_idx + 1) / radial_segs

            v_bl = verts[h_idx][r_idx]
            v_br = verts[h_idx][r_next]
            v_tr = verts[h_idx + 1][r_next]
            v_tl = verts[h_idx + 1][r_idx]

            if flip_normals:
                face = bm.faces.new((v_bl, v_tl, v_tr, v_br))
                face.smooth = True
                for loop in face.loops:
                    if loop.vert == v_bl: loop[uv_layer].uv = (u0, v0)
                    elif loop.vert == v_tl: loop[uv_layer].uv = (u0, v1)
                    elif loop.vert == v_tr: loop[uv_layer].uv = (u1, v1)
                    elif loop.vert == v_br: loop[uv_layer].uv = (u1, v0)
            else:
                face = bm.faces.new((v_bl, v_br, v_tr, v_tl))
                face.smooth = True
                for loop in face.loops:
                    if loop.vert == v_bl: loop[uv_layer].uv = (u0, v0)
                    elif loop.vert == v_br: loop[uv_layer].uv = (u1, v0)
                    elif loop.vert == v_tr: loop[uv_layer].uv = (u1, v1)
                    elif loop.vert == v_tl: loop[uv_layer].uv = (u0, v1)

    bmesh.ops.recalc_face_normals(bm, faces=bm.faces)
    bm.to_mesh(mesh)
    bm.free()
    obj = bpy.data.objects.new(name, mesh)
    bpy.context.collection.objects.link(obj)
    return obj

def create_scrap_mesh(name, radius, length, num_ribs=8, num_rails=12):
    mesh = bpy.data.meshes.new(name)
    bm = bmesh.new()
    uv_layer = bm.loops.layers.uv.new("UVMap")

    # Longitudinal rails
    for rail_idx in range(num_rails):
        theta = (rail_idx / num_rails) * 2.0 * math.pi
        w_theta = 0.02
        th0 = theta - w_theta
        th1 = theta + w_theta
        r_inner = radius * 0.97

        p0 = (r_inner * math.cos(th0), -length*0.5, r_inner * math.sin(th0))
        p1 = (r_inner * math.cos(th1), -length*0.5, r_inner * math.sin(th1))
        p2 = (r_inner * math.cos(th1),  length*0.5, r_inner * math.sin(th1))
        p3 = (r_inner * math.cos(th0),  length*0.5, r_inner * math.sin(th0))

        v0 = bm.verts.new(p0)
        v1 = bm.verts.new(p1)
        v2 = bm.verts.new(p2)
        v3 = bm.verts.new(p3)

        f = bm.faces.new((v0, v3, v2, v1))
        f.smooth = True
        u0 = rail_idx / num_rails
        u1 = u0 + 0.05
        for loop in f.loops:
            if loop.vert == v0: loop[uv_layer].uv = (u0, 0.0)
            elif loop.vert == v3: loop[uv_layer].uv = (u0, 1.0)
            elif loop.vert == v2: loop[uv_layer].uv = (u1, 1.0)
            elif loop.vert == v1: loop[uv_layer].uv = (u1, 0.0)

    # Circumferential ribs
    for rib_idx in range(num_ribs):
        y_center = -length*0.5 + (rib_idx + 0.5) * (length / num_ribs)
        dy = length / (num_ribs * 6.0)
        y0 = y_center - dy
        y1 = y_center + dy
        r_inner = radius * 0.95
        segs = 32
        rib_verts_0 = []
        rib_verts_1 = []
        for s in range(segs):
            theta = (s / segs) * 2.0 * math.pi
            rib_verts_0.append(bm.verts.new((r_inner * math.cos(theta), y0, r_inner * math.sin(theta))))
            rib_verts_1.append(bm.verts.new((r_inner * math.cos(theta), y1, r_inner * math.sin(theta))))

        for s in range(segs):
            s_next = (s + 1) % segs
            v0 = rib_verts_0[s]
            v1 = rib_verts_0[s_next]
            v2 = rib_verts_1[s_next]
            v3 = rib_verts_1[s]
            f = bm.faces.new((v0, v3, v2, v1))
            f.smooth = True
            u0 = s / segs
            u1 = (s + 1) / segs
            for loop in f.loops:
                if loop.vert == v0: loop[uv_layer].uv = (u0, 0.5)
                elif loop.vert == v3: loop[uv_layer].uv = (u0, 0.6)
                elif loop.vert == v2: loop[uv_layer].uv = (u1, 0.6)
                elif loop.vert == v1: loop[uv_layer].uv = (u1, 0.5)

    bmesh.ops.recalc_face_normals(bm, faces=bm.faces)
    bm.to_mesh(mesh)
    bm.free()
    obj = bpy.data.objects.new(name, mesh)
    bpy.context.collection.objects.link(obj)
    return obj

def export_obj_layer(obj, filepath):
    bpy.ops.object.select_all(action='DESELECT')
    obj.select_set(True)
    bpy.context.view_layer.objects.active = obj
    bpy.ops.wm.obj_export(filepath=filepath, export_selected_objects=True, export_uv=True, export_normals=True)
    print("Exported OBJ:", filepath)

def build_models():
    print("=== Building High-Fidelity 3D Meshes ===")
    bpy.ops.wm.read_factory_settings(use_empty=True)

    tube = create_cylinder_mesh("WarpTunnelTube", radius=1.0, length=1.0, radial_segs=64, height_segs=16, flip_normals=True)
    scrap = create_scrap_mesh("WarpTunnelScrap", radius=0.96, length=1.0, num_ribs=6, num_rails=12)
    cloud = create_cylinder_mesh("WarpTunnelCloud", radius=0.92, length=1.0, radial_segs=48, height_segs=12, flip_normals=True)

    export_obj_layer(tube, os.path.join(OUT_MODELS, "warp_tunnel_tube.obj"))
    export_obj_layer(scrap, os.path.join(OUT_MODELS, "warp_tunnel_scrap.obj"))
    export_obj_layer(cloud, os.path.join(OUT_MODELS, "warp_tunnel_cloud.obj"))

    # Also combined mesh for orbital jump gate
    bpy.ops.object.select_all(action='SELECT')
    bpy.ops.object.duplicate()
    dups = [o for o in bpy.context.selected_objects if o.type == 'MESH']
    bpy.context.view_layer.objects.active = dups[0]
    bpy.ops.object.join()
    comb = bpy.context.view_layer.objects.active
    comb.name = "WarpTunnelCombined"
    export_obj_layer(comb, os.path.join(OUT_MODELS, "warp_tunnel.obj"))

    print("All OBJ models built successfully.")

def render_preview():
    print("=== Rendering Cycles Preview ===")
    scene = bpy.context.scene
    scene.render.engine = "CYCLES"
    scene.cycles.device = "CPU"
    scene.render.resolution_x = 1280
    scene.render.resolution_y = 720
    scene.cycles.samples = 64
    scene.cycles.use_denoising = True

    cam_data = bpy.data.cameras.new("TunnelCam")
    cam_data.lens = 28.0
    cam = bpy.data.objects.new("TunnelCam", cam_data)
    cam.location = (0.0, -15.0, 0.0)
    cam.rotation_euler = (math.radians(90), 0, 0)
    scene.collection.objects.link(cam)
    scene.camera = cam

    mat_tube = bpy.data.materials.new("MatTube")
    mat_tube.use_nodes = True
    bsdf_t = mat_tube.node_tree.nodes.get("Principled BSDF")
    if bsdf_t:
        bsdf_t.inputs["Base Color"].default_value = (0.02, 0.04, 0.08, 1.0)
        bsdf_t.inputs["Metallic"].default_value = 0.90
        bsdf_t.inputs["Roughness"].default_value = 0.05
        bsdf_t.inputs["Emission Color"].default_value = (0.1, 0.5, 1.0, 1.0)
        bsdf_t.inputs["Emission Strength"].default_value = 1.5

    mat_scrap = bpy.data.materials.new("MatScrap")
    mat_scrap.use_nodes = True
    bsdf_s = mat_scrap.node_tree.nodes.get("Principled BSDF")
    if bsdf_s:
        bsdf_s.inputs["Base Color"].default_value = (0.8, 0.85, 0.9, 1.0)
        bsdf_s.inputs["Metallic"].default_value = 0.95
        bsdf_s.inputs["Roughness"].default_value = 0.20
        bsdf_s.inputs["Emission Color"].default_value = (0.145, 0.216, 1.0, 1.0)
        bsdf_s.inputs["Emission Strength"].default_value = 4.0

    mat_cloud = bpy.data.materials.new("MatCloud")
    mat_cloud.use_nodes = True
    bsdf_c = mat_cloud.node_tree.nodes.get("Principled BSDF")
    if bsdf_c:
        bsdf_c.inputs["Base Color"].default_value = (0.559, 0.512, 1.0, 1.0)
        bsdf_c.inputs["Emission Color"].default_value = (0.559, 0.512, 1.0, 1.0)
        bsdf_c.inputs["Emission Strength"].default_value = 6.0
        bsdf_c.inputs["Alpha"].default_value = 0.45

    tube_obj = bpy.data.objects.get("WarpTunnelTube")
    scrap_obj = bpy.data.objects.get("WarpTunnelScrap")
    cloud_obj = bpy.data.objects.get("WarpTunnelCloud")
    if tube_obj: tube_obj.data.materials.append(mat_tube)
    if scrap_obj: scrap_obj.data.materials.append(mat_scrap)
    if cloud_obj: cloud_obj.data.materials.append(mat_cloud)

    for o in [tube_obj, scrap_obj, cloud_obj]:
        if o:
            o.scale = (8.5, 40.0, 8.5)

    out_hero = os.path.join(DOCS_SCREENS, "warp_tunnel_hero.png")
    scene.render.filepath = out_hero
    bpy.ops.render.render(write_still=True)
    print("Rendered hero to:", out_hero)

if __name__ == "__main__":
    generate_textures()
    build_models()
    render_preview()
