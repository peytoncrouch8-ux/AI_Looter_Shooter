"""Reference views of the Stone Sentinel for remaking it in Tripo (2026-10-29): Art/Models/Creatures/StoneSentinel.py's
statue built as it is, then posed NEUTRAL for rigging instead of its dormant clasp: upright, weight even, feet apart, arms
hanging about 20 degrees out from its sides with the elbows a little bent (clear of the robe), hands apart, the head
facing forward and only slightly bowed; no plinth. Four orthographic views (front, back, its left side, its right side),
2048 px square, the figure about 85% of the height, flat even light (a white sky and one very soft sun: no hard shadows; Cycles, so the hood's hollow and the folds read), on a
transparent background (RGBA: a backdrop would be modelled as a piece), and a contact sheet of the four (views.png, also
transparent).

    powershell -NoProfile -File Tools\\artrun.ps1 -Script Tools\\Blender\\sentinel_views.py
        (Art/Concepts/Creatures/StoneSentinel/upload/front.png, back.png, left.png, right.png, views.png)
"""
import math
import os

import bpy
import numpy as np
from mathutils import Matrix, Vector

import looter_textures as lt

MODEL = os.path.join(lt.REPO, 'Art', 'Models', 'Creatures', 'StoneSentinel.py')
OUT = os.path.join(lt.REPO, 'Art', 'Concepts', 'Creatures', 'StoneSentinel', 'upload')
SIZE = 2048
FILL = 0.85


def log(message):
    print(f'SENTINEL VIEWS: {message}', flush=True)


# The statue, as the model script builds it (its previews off: no --preview here).
with open(MODEL, encoding='utf-8') as source:
    code = source.read()
S = {'__name__': 'sentinel_for_views', '__file__': MODEL}
exec(compile(code, MODEL, 'exec'), S)
RIG, BODY, PLINTH = S['RIG'], S['BODY'], S['PLINTH']


def blender_dir(x, y, z):
    """A creature-space direction (x forward, y left, z up) in Blender's axes (front -Y)."""
    return Vector((y, -x, z)).normalized()


def aim(name, direction):
    """Turns a bone (as posed now, its parents' turns included) so it points along direction (Blender axes), about its
    head."""
    bpy.context.view_layer.update()
    pb = RIG.pose.bones[name]
    head, tail = pb.head.copy(), pb.tail.copy()
    turn = (tail - head).normalized().rotation_difference(direction).to_matrix().to_4x4()
    pb.matrix = Matrix.Translation(head) @ turn @ Matrix.Translation(-head) @ pb.matrix
    bpy.context.view_layer.update()


def turn(name, axis, degrees):
    """Turns a bone about an axis (Blender axes) through its head."""
    bpy.context.view_layer.update()
    pb = RIG.pose.bones[name]
    head = pb.head.copy()
    rot = Matrix.Rotation(math.radians(degrees), 4, axis)
    pb.matrix = Matrix.Translation(head) @ rot @ Matrix.Translation(-head) @ pb.matrix
    bpy.context.view_layer.update()


def neutral_pose():
    bpy.context.view_layer.objects.active = RIG
    bpy.ops.object.mode_set(mode='POSE')
    out20 = math.radians(20.0)
    for s, sfx in ((1.0, 'l'), (-1.0, 'r')):
        # The upper arm hangs 20 degrees out from the side; the forearm a little further out and bent 12 degrees forward
        # at the elbow; the hand along it.
        aim(f'upperarm_{sfx}', blender_dir(0.0, s * math.sin(out20), -math.cos(out20)))
        fore = blender_dir(math.sin(math.radians(12.0)), s * math.sin(math.radians(24.0)), -1.0)
        aim(f'lowerarm_{sfx}', fore)
        aim(f'hand_{sfx}', fore)
        # Feet set apart under the robe, about shoulder width.
        aim(f'thigh_{sfx}', blender_dir(0.0, s * 0.16, -1.0))
        aim(f'calf_{sfx}', blender_dir(0.04, s * 0.06, -1.0))
        aim(f'foot_{sfx}', blender_dir(1.0, 0.0, -0.25))
    # The head lifted to face forward, still a little bowed (28 degrees at rest, 8 here): about its left-right axis.
    turn('head', 'X', -20.0)
    bpy.ops.object.mode_set(mode='OBJECT')
    bpy.context.view_layer.update()


def render(path, eye, height):
    scene = bpy.context.scene
    for o in scene.objects:
        o.hide_render = o is not BODY
    cam = bpy.data.objects.new('_ViewCam', bpy.data.cameras.new('_ViewCam'))
    cam.data.type = 'ORTHO'
    cam.data.ortho_scale = height / FILL
    target = Vector((0.0, 0.0, height * 0.5))
    cam.location = target + Vector(eye) * 10.0
    cam.rotation_euler = (target - cam.location).to_track_quat('-Z', 'Y').to_euler()
    cam.data.clip_start, cam.data.clip_end = 0.1, 100.0
    scene.collection.objects.link(cam)
    world = bpy.data.worlds.new('_ViewWorld')
    world.use_nodes = True
    bg = world.node_tree.nodes['Background']
    bg.inputs['Color'].default_value = (1.0, 1.0, 1.0, 1.0)
    bg.inputs['Strength'].default_value = 0.85
    scene.world = world
    # One very soft key from above and in front of the camera (a 30-degree sun: no hard edge to its shadows), so the
    # hood's hollow, the folds and the sleeves' mouths read as depth, not as paint.
    sun = bpy.data.objects.new('_ViewSun', bpy.data.lights.new('_ViewSun', 'SUN'))
    sun.data.energy = 1.6
    sun.data.angle = math.radians(30.0)
    toward = (Vector(eye) + Vector((0.0, 0.0, 0.9))).normalized()
    sun.rotation_euler = (-toward).to_track_quat('-Z', 'Y').to_euler()
    scene.collection.objects.link(sun)
    scene.camera = cam
    scene.render.engine = 'CYCLES'
    scene.cycles.device = 'CPU'
    scene.cycles.samples = 96
    scene.cycles.use_denoising = True
    scene.render.resolution_x = scene.render.resolution_y = SIZE
    scene.render.resolution_percentage = 100
    scene.render.film_transparent = True
    scene.render.image_settings.file_format = 'PNG'
    scene.render.image_settings.color_mode = 'RGBA'
    scene.view_settings.view_transform = 'AgX'
    scene.view_settings.look = 'AgX - Base Contrast'
    scene.render.filepath = path
    bpy.ops.render.render(write_still=True)
    bpy.data.objects.remove(cam)
    bpy.data.objects.remove(sun)
    bpy.data.worlds.remove(world)
    log(path)
    return path


def read_rgba(path):
    image = bpy.data.images.load(path, check_existing=False)
    image.colorspace_settings.name = 'Non-Color'
    w, h = image.size
    px = np.empty(w * h * 4, np.float32)
    image.pixels.foreach_get(px)
    bpy.data.images.remove(image)
    return px.reshape(h, w, 4)[::-1]


def sheet(paths, out, scale=4):
    """The four side by side at a quarter size, transparent between them."""
    tiles = [read_rgba(p)[::scale, ::scale] for p in paths]
    h, w = tiles[0].shape[:2]
    gap = 16
    canvas = np.zeros((h, len(tiles) * w + (len(tiles) - 1) * gap, 4), np.float32)
    for i, t in enumerate(tiles):
        canvas[:, i * (w + gap):i * (w + gap) + w] = t
    lt.write_png(out, lt.to8(np.clip(canvas, 0.0, 1.0)))
    log(out)


def main():
    os.makedirs(OUT, exist_ok=True)
    S['preview_moss']()
    # The fold moss left out here: its patches follow the mesh's faces and read as square tiles a remake would copy (the
    # moss settling on the upward faces stays).
    slot = BODY.data.materials.find(S['MOSS'].name)
    if slot >= 0:
        BODY.data.materials[slot] = S['STONE']
    # The baked occlusion left out (it was baked in the dormant pose: the clasped hands' shadow stays on the chest once they
    # hang at the sides); Cycles' own light finds the hollows of the posed statue instead.
    for mat in (S['STONE'], S['MOSS']):
        nodes, links = mat.node_tree.nodes, mat.node_tree.links
        vc = next(n for n in nodes if n.type == 'VERTEX_COLOR')
        targets = [link.to_socket for link in vc.outputs['Alpha'].links]
        power = nodes.new('ShaderNodeMath')
        power.operation = 'POWER'
        power.inputs[1].default_value = 0.0
        links.new(vc.outputs['Alpha'], power.inputs[0])
        for socket in targets:
            links.new(power.outputs['Value'], socket)
    neutral_pose()
    lo, hi = lt._bounds([BODY])
    height = hi.z
    log(f'posed: height {hi.z:.2f} m, x {lo.x:.2f}..{hi.x:.2f}, y {lo.y:.2f}..{hi.y:.2f}, bottom {lo.z:.3f}')
    # Its front faces -Y; its left side is +X.
    views = [('front', (0.0, -1.0, 0.0)), ('back', (0.0, 1.0, 0.0)), ('left', (1.0, 0.0, 0.0)), ('right', (-1.0, 0.0, 0.0))]
    paths = [render(os.path.join(OUT, f'{name}.png'), eye, height) for name, eye in views]
    sheet(paths, os.path.join(OUT, 'views.png'))


main()
