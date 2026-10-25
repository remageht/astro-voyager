import numpy as np
from PIL import Image, ImageDraw

def run_comparison():
    # Camera and projection parameters matching both renders:
    rotY = 0.08
    c, s = np.cos(rotY), np.sin(rotY)
    R = np.array([[c, 0, s], [0, 1, 0], [-s, 0, c]])

    dock_local = np.array([0.0, -0.041885, 0.422339]) * 0.38
    dock_target = R @ dock_local

    loc_dir = np.array([0.50, 0.30, 0.82])
    loc_dir = loc_dir / np.linalg.norm(loc_dir)
    dir_world = R @ loc_dir

    cam_dist = 4.45
    cam_pos = dock_target + dir_world * cam_dist

    fwd = dock_target - cam_pos
    fwd = fwd / np.linalg.norm(fwd)
    up = np.array([0.0, 1.0, 0.0])
    right = np.cross(fwd, up)
    right = right / np.linalg.norm(right)
    cam_up = np.cross(right, fwd)

    aspect = 1280.0 / 720.0
    tan_h = np.tan(np.radians(7.5) / 2.0)

    verts = []
    faces = []
    with open('res/models/station.obj') as f:
        for line in f:
            if line.startswith('v '):
                parts = line.strip().split()
                verts.append([float(parts[1]), float(parts[2]), float(parts[3])])
            elif line.startswith('f '):
                parts = line.strip().split()
                idx = [int(p.split('/')[0]) - 1 for p in parts[1:4]]
                faces.append(idx)
    verts = np.array(verts)
    faces = np.array(faces)

    v_world = (verts * 0.38) @ R.T
    v_cam = (v_world - cam_pos) @ np.vstack([right, cam_up, -fwd]).T
    ndc_x = v_cam[:, 0] / (-v_cam[:, 2] * aspect * tan_h)
    ndc_y = v_cam[:, 1] / (-v_cam[:, 2] * tan_h)
    sx = (ndc_x * 0.5 + 0.5) * 1280.0
    sy = (1.0 - (ndc_y * 0.5 + 0.5)) * 720.0
    pts_2d = np.vstack([sx, sy]).T

    face_centers = verts[faces].mean(axis=1)
    is_wings = np.abs(face_centers[:, 0]) > 0.22
    is_modules = (~is_wings) & ((np.abs(face_centers[:, 2]) > 0.15) | (np.abs(face_centers[:, 1]) > 0.12))
    is_truss = (~is_wings) & (~is_modules)

    mask_wings = Image.new('L', (1280, 720), 0)
    mask_truss = Image.new('L', (1280, 720), 0)
    mask_modules = Image.new('L', (1280, 720), 0)

    draw_w = ImageDraw.Draw(mask_wings)
    draw_t = ImageDraw.Draw(mask_truss)
    draw_m = ImageDraw.Draw(mask_modules)

    for f_idx in np.where(is_wings)[0]:
        p = pts_2d[faces[f_idx]]
        draw_w.polygon([tuple(p[0]), tuple(p[1]), tuple(p[2])], fill=255)

    for f_idx in np.where(is_truss)[0]:
        p = pts_2d[faces[f_idx]]
        draw_t.polygon([tuple(p[0]), tuple(p[1]), tuple(p[2])], fill=255)

    for f_idx in np.where(is_modules)[0]:
        p = pts_2d[faces[f_idx]]
        draw_m.polygon([tuple(p[0]), tuple(p[1]), tuple(p[2])], fill=255)

    mw = np.array(mask_wings) > 0
    mt = np.array(mask_truss) > 0
    mm = np.array(mask_modules) > 0

    img_cy = np.array(Image.open('blender/out/cycles_station_check.png').convert('RGB'), dtype=np.float32)
    img_gl = np.array(Image.open('blender/out/ingame_station_check.png').convert('RGB'), dtype=np.float32)

    lum_cy = 0.2126 * img_cy[:,:,0] + 0.7152 * img_cy[:,:,1] + 0.0722 * img_cy[:,:,2]
    lum_gl = 0.2126 * img_gl[:,:,0] + 0.7152 * img_gl[:,:,1] + 0.0722 * img_gl[:,:,2]

    zones = [('Zone 1 (Wings)', mw), ('Zone 2 (Truss)', mt), ('Zone 3 (Modules)', mm)]

    print('\n' + '='*74)
    print('          SPACE STATION 3-ZONE PBR HISTOGRAM & PHOTOMETRIC AUDIT')
    print('='*74)
    all_pass = True
    print(f"Mask counts: mw={mw.sum()}, mt={mt.sum()}, mm={mm.sum()}")
    for name, m in zones:
        print(f"Raw {name}: cy_mean={lum_cy[m].mean():.1f}, cy_max={lum_cy[m].max():.1f}, gl_mean={lum_gl[m].mean():.1f}, gl_max={lum_gl[m].max():.1f}")
        fg_c = m & (lum_cy > 40.0)
        fg_g = m & (lum_gl > 40.0)

        rgb_c = img_cy[fg_c].mean(axis=0) if fg_c.sum() > 0 else np.array([0,0,0])
        rgb_g = img_gl[fg_g].mean(axis=0) if fg_g.sum() > 0 else np.array([0,0,0])

        l_c = lum_cy[fg_c].mean() if fg_c.sum() > 0 else 0.0
        l_g = lum_gl[fg_g].mean() if fg_g.sum() > 0 else 0.0
        std_g = lum_gl[fg_g].std() if fg_g.sum() > 0 else 0.0

        diff = abs(l_c - l_g) / max(l_c, l_g) * 100.0 if max(l_c, l_g) > 0 else 0.0
        status = "PASS" if diff <= 18.0 else ("WARN" if diff <= 25.0 else "FAIL")
        if status != "PASS":
            all_pass = False

        print(f'{name}:')
        print(f'  Cycles:  Mean Lum={l_c:.1f}, RGB=[{rgb_c[0]:.1f}, {rgb_c[1]:.1f}, {rgb_c[2]:.1f}] ({fg_c.sum()} px)')
        print(f'  In-Game: Mean Lum={l_g:.1f}, RGB=[{rgb_g[0]:.1f}, {rgb_g[1]:.1f}, {rgb_g[2]:.1f}] ({fg_g.sum()} px), std={std_g:.1f}')
        print(f'  Luma Diff: {diff:.1f}% (Requirement <= 18%: {status})')
        if name == 'Zone 1 (Wings)':
            wing_readability = "PASS" if std_g > 10.0 else "FAIL"
            print(f'  Wing Detail / Readability (std > 10.0): {std_g:.1f} ({wing_readability})')
        print('-'*74)

    print(f'Overall PBR Verification Result: {"ALL PASS (diff <= 18%)" if all_pass else "WARN / REVIEW"}')
    print('='*74 + '\n')
    return all_pass

if __name__ == '__main__':
    run_comparison()
