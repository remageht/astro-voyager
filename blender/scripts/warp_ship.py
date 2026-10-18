"""Deterministic warp-ship model + warp-flight animation (Blender headless).

Usage:
  "C:\\Program Files\\Blender Foundation\\Blender 5.2\\blender.exe" -b -P blender/scripts/warp_ship.py

Outputs into blender/out/:
  warp_ship.obj / .mtl / .glb   (joined hero model)
  warp_ship.blend
  warp_hero.png, warp_profile.png  (3/4 + portside design stills)
  warp_f012/f050/f100.png       (animation QA stills)
  warp_flight.gif               (60 frames @ 320x180, 4.8 s warp jump)

Design cues (classic warp ships): streamlined hull + twin nacelles on
pylons, glowing bussard collectors (orange), blue warp-field grilles,
deflector dish, bright engine. Movement: idle -> charge (glow ramps) ->
warp (stars stretch into long parallel streaks, camera widens FOV).
Deterministic: seeded RNG for the star field, fixed keyframes.
"""
import math
import os
import random
import sys

import bpy
from mathutils import Vector

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(SCRIPT_DIR)
OUT_DIR = os.path.join(ROOT, "out")

FPS = 24
FRAME_END = 120
STREAK_COUNT = 160
SEED = 11
ANIM = "--anim" in sys.argv
CHASE_POS = (-9.5, 2.2, 1.0)
CHASE_TARGET = (-0.6, 0.0, 0.25)


def new_material(name, base, metallic, roughness, emission=None, strength=0.0,
                 spec=None):
    mat = bpy.data.materials.new(name)
    mat.use_nodes = True
    bsdf = mat.node_tree.nodes.get("Principled BSDF")
    if bsdf is not None:
        bsdf.inputs["Base Color"].default_value = (base[0], base[1], base[2], 1.0)
        bsdf.inputs["Metallic"].default_value = metallic
        bsdf.inputs["Roughness"].default_value = roughness
        if spec is not None and "Specular IOR Level" in bsdf.inputs:
            bsdf.inputs["Specular IOR Level"].default_value = spec
        if emission is not None:
            bsdf.inputs["Emission Color"].default_value = (
                emission[0], emission[1], emission[2], 1.0
            )
            bsdf.inputs["Emission Strength"].default_value = strength
    return mat


def make_materials():
    return {
        "hull": new_material("WarpHull", (0.72, 0.74, 0.80), 0.75, 0.32),
        "accent": new_material("WarpAccent", (0.09, 0.10, 0.12), 0.60, 0.45),
        "bussard": new_material("WarpBussard", (0.05, 0.02, 0.01), 0.0, 0.55,
                                (1.0, 0.28, 0.10), 1.2, spec=0.05),
        "grille": new_material("WarpGrille", (0.02, 0.05, 0.10), 0.0, 0.5,
                               (0.25, 0.55, 1.0), 1.8, spec=0.15),
        "engine": new_material("WarpEngine", (0.05, 0.06, 0.09), 0.0, 0.5,
                               (0.75, 0.85, 1.0), 2.2, spec=0.1),
        "streak": new_material("WarpStreak", (0.0, 0.0, 0.0), 0.0, 0.5,
                               (0.78, 0.86, 1.0), 4.5),
    }


def add_material(obj, mat):
    obj.data.materials.append(mat)


def build_ship(mats):
    parts = []

    bpy.ops.mesh.primitive_uv_sphere_add(segments=48, ring_count=24, radius=1.0)
    hull = bpy.context.active_object
    hull.name = "Hull"
    hull.scale = (2.7, 0.55, 0.44)
    add_material(hull, mats["hull"])
    parts.append(hull)

    bpy.ops.mesh.primitive_uv_sphere_add(segments=24, ring_count=12, radius=0.30,
                                         location=(0.7, 0.0, 0.48))
    bridge = bpy.context.active_object
    bridge.name = "Bridge"
    bridge.scale = (1.3, 0.75, 0.55)
    add_material(bridge, mats["hull"])
    parts.append(bridge)

    bpy.ops.mesh.primitive_cone_add(vertices=32, radius1=0.45, radius2=0.13,
                                    depth=0.75,
                                    location=(2.40, 0.0, 0.0),
                                    rotation=(0.0, math.pi / 2.0, 0.0))
    nose = bpy.context.active_object
    nose.name = "Nose"
    add_material(nose, mats["hull"])
    parts.append(nose)

    bpy.ops.mesh.primitive_cylinder_add(vertices=32, radius=0.455, depth=0.07,
                                        location=(2.03, 0.0, 0.0),
                                        rotation=(0.0, math.pi / 2.0, 0.0))
    nose_ring = bpy.context.active_object
    nose_ring.name = "NoseCollar"
    add_material(nose_ring, mats["accent"])
    parts.append(nose_ring)

    bpy.ops.mesh.primitive_cylinder_add(vertices=24, radius=0.16, depth=0.06,
                                        location=(2.30, 0.0, -0.26),
                                        rotation=(0.0, math.pi / 2.0, 0.0))
    deflector = bpy.context.active_object
    deflector.name = "Deflector"
    add_material(deflector, mats["grille"])
    parts.append(deflector)

    for side in (-1.0, 1.0):
        tag = "L" if side < 0.0 else "R"

        bpy.ops.mesh.primitive_cylinder_add(vertices=24, radius=0.28, depth=3.4,
                                            location=(-0.6, side * 1.35, 0.55),
                                            rotation=(0.0, math.pi / 2.0, 0.0))
        nac = bpy.context.active_object
        nac.name = "Nacelle_" + tag
        add_material(nac, mats["hull"])
        parts.append(nac)

        bpy.ops.mesh.primitive_uv_sphere_add(segments=24, ring_count=12,
                                             radius=0.31,
                                             location=(1.26, side * 1.35, 0.55))
        bussard = bpy.context.active_object
        bussard.name = "Bussard_" + tag
        bussard.scale = (0.62, 1.0, 1.0)
        add_material(bussard, mats["bussard"])
        parts.append(bussard)

        bpy.ops.mesh.primitive_cylinder_add(vertices=24, radius=0.295, depth=0.05,
                                            location=(1.07, side * 1.35, 0.55),
                                            rotation=(0.0, math.pi / 2.0, 0.0))
        collar = bpy.context.active_object
        collar.name = "BussardCollar_" + tag
        add_material(collar, mats["accent"])
        parts.append(collar)

        bpy.ops.mesh.primitive_uv_sphere_add(segments=16, ring_count=8,
                                             radius=0.24,
                                             location=(-2.36, side * 1.35, 0.55))
        tip = bpy.context.active_object
        tip.name = "NacelleTip_" + tag
        tip.scale = (0.6, 1.0, 1.0)
        add_material(tip, mats["engine"])
        parts.append(tip)

        bpy.ops.mesh.primitive_cube_add(size=1.0,
                                        location=(-0.6, side * 1.62, 0.55))
        grille = bpy.context.active_object
        grille.name = "Grille_" + tag
        grille.scale = (1.5, 0.045, 0.12)
        add_material(grille, mats["grille"])
        parts.append(grille)

        bpy.ops.mesh.primitive_cube_add(size=1.0,
                                        location=(-0.45, side * 0.70, 0.40),
                                        rotation=(side * 0.25, 0.0, 0.0))
        pylon = bpy.context.active_object
        pylon.name = "Pylon_" + tag
        pylon.scale = (1.55, 1.10, 0.075)
        add_material(pylon, mats["hull"])
        parts.append(pylon)

        bpy.ops.mesh.primitive_cube_add(size=1.0,
                                        location=(-1.75, side * 0.62, 0.34),
                                        rotation=(side * 0.42, 0.0, 0.0))
        brace = bpy.context.active_object
        brace.name = "PylonBrace_" + tag
        brace.scale = (0.28, 1.05, 0.06)
        add_material(brace, mats["accent"])
        parts.append(brace)

    bpy.ops.mesh.primitive_cylinder_add(vertices=24, radius=0.26, depth=0.10,
                                        location=(-2.78, 0.0, 0.0),
                                        rotation=(0.0, math.pi / 2.0, 0.0))
    engine = bpy.context.active_object
    engine.name = "Engine"
    add_material(engine, mats["engine"])
    parts.append(engine)

    for obj in parts:
        obj.select_set(True)
    bpy.context.view_layer.objects.active = hull
    bpy.ops.object.join()
    ship = bpy.context.active_object
    ship.name = "WarpShip"
    bpy.ops.object.origin_set(type="ORIGIN_GEOMETRY")
    for obj in bpy.context.scene.objects:
        obj.select_set(False)
    return ship


def build_streaks(mats):
    coll = bpy.data.collections.new("Streaks")
    bpy.context.scene.collection.children.link(coll)
    rnd = random.Random(SEED)
    streaks = []
    for _ in range(STREAK_COUNT):
        length = rnd.uniform(1.5, 3.5)
        x0 = rnd.uniform(-60.0, 60.0)
        r = 3.0 + 25.0 * math.sqrt(rnd.random())
        ang = rnd.uniform(0.0, 2.0 * math.pi)
        bpy.ops.mesh.primitive_cube_add(size=1.0)
        obj = bpy.context.active_object
        obj.name = "Streak"
        obj.scale = (length, 0.025, 0.025)
        obj.location = (x0, r * math.cos(ang), r * math.sin(ang))
        add_material(obj, mats["streak"])
        streaks.append((obj, length))
        for c in obj.users_collection:
            c.objects.unlink(obj)
        coll.objects.link(obj)
    return coll, streaks


def aim(obj, loc, target):
    obj.location = Vector(loc)
    direction = Vector(target) - obj.location
    obj.rotation_euler = direction.to_track_quat("-Z", "Y").to_euler()


def emission_socket(mat):
    return mat.node_tree.nodes["Principled BSDF"].inputs["Emission Strength"]


def key_emission(mat, frames):
    socket = emission_socket(mat)
    for frame, value in frames:
        socket.default_value = value
        socket.keyframe_insert("default_value", frame=frame)


def animate(ship, streaks, mats, cam):
    scene = bpy.context.scene
    scene.frame_start = 1
    scene.frame_end = FRAME_END
    scene.render.fps = FPS

    for frame, z in ((1, 0.0), (30, 0.06), (60, -0.04), (90, 0.03),
                     (FRAME_END, 0.0)):
        ship.location = (0.0, 0.0, z)
        ship.keyframe_insert("location", frame=frame)

    for obj, length in streaks:
        x0 = obj.location.x
        for frame, dx in ((1, 0.0), (48, -8.0), (FRAME_END, -200.0)):
            obj.location.x = x0 + dx
            obj.keyframe_insert("location", index=0, frame=frame)
        for frame, k in ((1, 1.0), (48, 1.0), (FRAME_END, 14.0)):
            obj.scale.x = length * k
            obj.keyframe_insert("scale", index=0, frame=frame)

    key_emission(mats["engine"], ((1, 1.4), (40, 2.2), (52, 8.0),
                                  (FRAME_END, 5.0)))
    key_emission(mats["streak"], ((1, 4.5), (44, 4.5), (52, 8.0),
                                  (FRAME_END, 5.5)))

    for frame, lens in ((1, 45.0), (40, 43.0), (FRAME_END, 28.0)):
        cam.data.lens = lens
        cam.data.keyframe_insert("lens", frame=frame)


def setup_scene():
    scene = bpy.context.scene
    scene.render.engine = "CYCLES"
    scene.cycles.device = "CPU"
    scene.cycles.samples = 24
    scene.cycles.use_denoising = True
    scene.render.image_settings.file_format = "PNG"

    world = bpy.data.worlds.new("World")
    scene.world = world
    world.use_nodes = True
    bg = world.node_tree.nodes.get("Background")
    if bg is not None:
        bg.inputs[0].default_value = (0.045, 0.055, 0.08, 1.0)
        bg.inputs[1].default_value = 1.0

    bpy.ops.object.light_add(type="SUN", location=(6.0, -4.0, 7.0))
    key = bpy.context.active_object
    key.data.energy = 3.0
    key.rotation_euler = (0.62, 0.20, 0.85)

    bpy.ops.object.light_add(type="SUN", location=(-5.0, 3.0, -4.0))
    fill = bpy.context.active_object
    fill.data.energy = 0.55
    fill.rotation_euler = (-0.9, -0.3, -2.2)

    bpy.ops.object.camera_add(location=(0.0, 0.0, 0.0))
    cam = bpy.context.active_object
    cam.data.lens = 50
    scene.camera = cam
    return cam


def render_still(path, frame, res, samples):
    scene = bpy.context.scene
    scene.render.resolution_x, scene.render.resolution_y = res
    scene.cycles.samples = samples
    scene.frame_set(frame)
    scene.render.filepath = path
    bpy.ops.render.render(write_still=True)
    print("RENDERED:", path)


def export_ship(ship):
    for obj in bpy.context.scene.objects:
        obj.select_set(False)
    ship.select_set(True)
    bpy.context.view_layer.objects.active = ship

    obj_path = os.path.join(OUT_DIR, "warp_ship.obj")
    try:
        bpy.ops.wm.obj_export(filepath=obj_path, export_selected_objects=True)
    except TypeError:
        bpy.ops.wm.obj_export(filepath=obj_path)
    print("EXPORTED:", obj_path)

    glb_path = os.path.join(OUT_DIR, "warp_ship.glb")
    try:
        bpy.ops.export_scene.gltf(filepath=glb_path, export_format="GLB",
                                  use_selection=True)
        print("EXPORTED:", glb_path)
    except Exception as exc:
        print("GLB export failed:", exc)


def assemble_gif(frame_paths, out_path, step=2, delay_cs=8):
    import numpy as np

    pal = np.zeros((256, 3), dtype=np.uint8)
    for r in range(6):
        for g in range(6):
            for b in range(6):
                pal[r * 36 + g * 6 + b] = (r * 51, g * 51, b * 51)

    frames = []
    for path in frame_paths:
        img = bpy.data.images.load(path)
        img.colorspace_settings.name = "Non-Color"
        w, h = img.size
        buf = np.empty(w * h * 4, dtype=np.float32)
        img.pixels.foreach_get(buf)
        bpy.data.images.remove(img)
        rgb = buf.reshape(h, w, 4)[::-1, :, :3]
        if step > 1 and h % step == 0 and w % step == 0:
            rgb = rgb.reshape(h // step, step, w // step, step, 3).mean(axis=(1, 3))
        else:
            rgb = rgb[::step, ::step]
        rgb8 = (np.clip(rgb, 0.0, 1.0) * 255.0 + 0.5).astype(np.uint16)
        idx = ((rgb8[:, :, 0] * 5 + 127) // 255) * 36 \
            + ((rgb8[:, :, 1] * 5 + 127) // 255) * 6 \
            + ((rgb8[:, :, 2] * 5 + 127) // 255)
        idx[rgb8.max(axis=2) < 30] = 0
        frames.append(idx.astype(np.uint8))

    height, width = frames[0].shape

    def lzw(indices):
        clear, end = 256, 257
        out = bytearray()
        state = [0, 0]

        def emit(code, size):
            state[0] |= code << state[1]
            state[1] += size
            while state[1] >= 8:
                out.append(state[0] & 0xFF)
                state[0] >>= 8
                state[1] -= 8

        table = {bytes([i]): i for i in range(256)}
        next_code, code_size = 258, 9
        emit(clear, code_size)
        prev = b""
        for pin in indices.tobytes():
            cur = prev + bytes([pin])
            if cur in table:
                prev = cur
                continue
            emit(table[prev], code_size)
            if next_code < 4096:
                table[cur] = next_code
                next_code += 1
                if next_code > (1 << code_size) and code_size < 12:
                    code_size += 1
            else:
                emit(clear, code_size)
                table = {bytes([i]): i for i in range(256)}
                next_code, code_size = 258, 9
            prev = bytes([pin])
        if prev:
            emit(table[prev], code_size)
        emit(end, code_size)
        if state[1]:
            out.append(state[0] & 0xFF)
        return bytes(out)

    def lzw_decode(data, expected_len):
        clear, end = 256, 257
        table = [bytes([i]) for i in range(256)] + [b"", b""]
        code_size = 9
        bitpos = 0

        def read_code():
            nonlocal bitpos
            code = 0
            for i in range(code_size):
                code |= ((data[(bitpos + i) >> 3] >> ((bitpos + i) & 7)) & 1) << i
            bitpos += code_size
            return code

        out = bytearray()
        prev = None
        while True:
            code = read_code()
            if code == clear:
                table = [bytes([i]) for i in range(256)] + [b"", b""]
                code_size = 9
                prev = None
                continue
            if code == end:
                break
            if code < len(table):
                entry = table[code]
            elif code == len(table) and prev is not None:
                entry = prev + prev[:1]
            else:
                raise ValueError("gif self-check: bad code")
            out += entry
            if prev is not None and len(table) < 4096:
                table.append(prev + entry[:1])
                if len(table) == (1 << code_size) and code_size < 12:
                    code_size += 1
            prev = entry
            if len(out) > expected_len:
                raise ValueError("gif self-check: overrun")
        return bytes(out)

    check = lzw_decode(lzw(frames[0]), frames[0].size)
    if check != frames[0].tobytes():
        raise RuntimeError("gif self-check failed")

    def sub_blocks(data):
        out = bytearray()
        for i in range(0, len(data), 255):
            chunk = data[i:i + 255]
            out.append(len(chunk))
            out += chunk
        out.append(0)
        return bytes(out)

    out = bytearray()
    out += b"GIF89a"
    out += width.to_bytes(2, "little") + height.to_bytes(2, "little")
    out += bytes([0xF7, 0, 0])
    out += pal.tobytes()
    out += b"\x21\xFF\x0BNETSCAPE2.0\x03\x01\x00\x00\x00"
    for fidx in frames:
        out += b"\x21\xF9\x04\x04" + delay_cs.to_bytes(2, "little") + b"\x00\x00"
        out += b"\x2C" + (0).to_bytes(2, "little") * 2
        out += width.to_bytes(2, "little") + height.to_bytes(2, "little")
        out += bytes([0x00, 0x08])
        out += sub_blocks(lzw(fidx))
    out += b"\x3B"
    with open(out_path, "wb") as fh:
        fh.write(out)
    print("GIF:", out_path, os.path.getsize(out_path), "bytes")


def main():
    os.makedirs(OUT_DIR, exist_ok=True)
    print("BLENDER:", bpy.app.version_string)
    bpy.ops.wm.read_factory_settings(use_empty=True)

    mats = make_materials()
    cam = setup_scene()
    ship = build_ship(mats)
    streaks_coll, streaks = build_streaks(mats)
    animate(ship, streaks, mats, cam)

    export_ship(ship)
    blend_path = os.path.join(OUT_DIR, "warp_ship.blend")
    bpy.ops.wm.save_as_mainfile(filepath=blend_path)
    print("SAVED:", blend_path)

    aim(cam, CHASE_POS, CHASE_TARGET)
    render_still(os.path.join(OUT_DIR, "warp_f012.png"), 12, (720, 480), 64)
    render_still(os.path.join(OUT_DIR, "warp_f050.png"), 50, (720, 480), 64)
    render_still(os.path.join(OUT_DIR, "warp_f100.png"), 100, (720, 480), 64)

    streaks_coll.hide_render = True
    aim(cam, (7.6, -5.2, 2.8), (-0.2, 0.0, 0.15))
    render_still(os.path.join(OUT_DIR, "warp_hero.png"), 1, (720, 480), 64)
    aim(cam, (0.15, -9.0, 1.05), (0.0, 0.0, 0.30))
    render_still(os.path.join(OUT_DIR, "warp_profile.png"), 1, (720, 480), 64)
    streaks_coll.hide_render = False

    if ANIM:
        scene = bpy.context.scene
        frames_dir = os.path.join(OUT_DIR, "warp_frames")
        os.makedirs(frames_dir, exist_ok=True)
        scene.render.resolution_x, scene.render.resolution_y = (640, 360)
        scene.cycles.samples = 24
        scene.render.image_settings.file_format = "PNG"
        scene.render.filepath = os.path.join(frames_dir, "w")
        aim(cam, CHASE_POS, CHASE_TARGET)
        bpy.ops.render.render(animation=True)
        paths = [os.path.join(frames_dir, "w%04d.png" % f)
                 for f in range(1, FRAME_END + 1)]
        assemble_gif(paths[::2], os.path.join(OUT_DIR, "warp_flight.gif"))
    print("DONE")


if __name__ == "__main__":
    main()
