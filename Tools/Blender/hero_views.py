"""Tripo upload views of a hero (Art/Concepts/Heroes/README.md): the game's own model, built by its script
(Art/Models/Characters/<Hero>.py) with the baked textures it has in the game, posed in a neutral A-pose and rendered
from the front, back, left and right in orthographic Cycles, 1024 x 1024, the same scale in all four.

The hero script runs unchanged: its main() is called as for `--preview`, and its preview render function (`preview`,
Gauge's `render_set`) is swapped for this one, which gets the rigged figure in its viewer materials (the baked maps),
saves it to Saved/HeroViews/<Hero>.blend (later runs pose and render from there, no build), poses it and renders.

    Tools\\artrun.ps1 -Script Tools\\Blender\\hero_views.py -ScriptArgs Ellis          build (6 threads), then render
    blender -b Saved\\HeroViews\\Ellis.blend --python Tools\\Blender\\hero_views.py -- Ellis --render   render only
      --samples N (64)  --size N (1024)  --fill F (0.85: the figure's height in the frame)  --out <dir>

Writes Art/Concepts/Heroes/<Hero>/upload/{front,back,left,right}.png (transparent) and upload_white/ (on #ffffff).
Views: front faces the camera; left shows the hero's own left side (they face the image's left); right their right.
Build at 6 threads (artrun's default): the baked maps fit only the mesh decimated at that count.
"""
import importlib.util
import math
import os
import sys

import bpy
import numpy as np
from mathutils import Matrix, Quaternion, Vector

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
MINE = sys.argv[sys.argv.index('--') + 1:] if '--' in sys.argv else []
HERO = next((a for a in MINE if not a.startswith('--')), 'Ellis')


def opt(name, default):
    return type(default)(MINE[MINE.index(name) + 1]) if name in MINE else default


SAMPLES = opt('--samples', 64)
SIZE = opt('--size', 1024)
FILL = opt('--fill', 0.85)
OUT = opt('--out', os.path.join(REPO, 'Art', 'Concepts', 'Heroes', HERO))
BLEND = os.path.join(REPO, 'Saved', 'HeroViews', f'{HERO}.blend')

# The A-pose: the upper arms this far below horizontal, out to the sides; the elbows bent this little (forward); the
# hands straight on the forearms, palms down; legs at rest (straight, the feet a little apart in every build)
ARM_DOWN = 42.0
ELBOW = 6.0
FINGER_EASE = 0.6        # the share of each finger joint's built curl taken out
# Parts held in the hands (none in today's game models: their guns are the game's); hidden by name prefix
HELD = {}
# each view's camera side of the figure: front, back, the hero's left (+X: built facing -Y), the hero's right
VIEWS = (('front', Vector((0.0, -1.0, 0.0))), ('back', Vector((0.0, 1.0, 0.0))),
         ('left', Vector((1.0, 0.0, 0.0))), ('right', Vector((-1.0, 0.0, 0.0))))


def log(m):
    print(f'HERO_VIEWS [{HERO}] {m}', flush=True)


class ViewsDone(Exception):
    pass


# --------------------------------------------------------------------------------------------------------------- pose

def _apply(pb, m):
    pb.matrix = m @ pb.matrix
    bpy.context.view_layer.update()


def _about(pb, q):
    h = pb.head.copy()
    _apply(pb, Matrix.Translation(h) @ q.to_matrix().to_4x4() @ Matrix.Translation(-h))


def _point(pb, d):
    _about(pb, (pb.tail - pb.head).normalized().rotation_difference(d.normalized()))


def _palm_normal(arm, s):
    """The palm's outward normal: across the knuckles (index to pinky metacarpal) crossed with the hand's length, on
    the side the thumb's tip is (the palm side)."""
    P = arm.pose.bones
    hand = P[f'hand_{s}']
    d = (hand.tail - hand.head).normalized()
    a = (P[f'pinky_metacarpal_{s}'].head - P[f'index_metacarpal_{s}'].head).normalized()
    n = d.cross(a).normalized()
    thumb = P[f'thumb_03_{s}'].tail - hand.head
    return n if thumb.dot(n) > 0 else -n


def a_pose(arm):
    for pb in arm.pose.bones:
        pb.rotation_mode = 'QUATERNION'
        pb.matrix_basis = Matrix.Identity(4)
    bpy.context.view_layer.update()
    P = arm.pose.bones
    for s, x in (('l', 1.0), ('r', -1.0)):
        up, lo, hand = P[f'upperarm_{s}'], P[f'lowerarm_{s}'], P[f'hand_{s}']
        rest = (up.tail - up.head).normalized()
        log(f'upperarm_{s} at rest {math.degrees(math.asin(-rest.z)):.0f} degrees below horizontal')
        a = math.radians(ARM_DOWN)
        d_up = Vector((x * math.cos(a), -0.04, -math.sin(a))).normalized()
        _point(up, d_up)
        # the elbow a little bent: the forearm turned toward the front about the arm's horizontal normal
        bend = Quaternion(d_up.cross(Vector((0.0, -1.0, 0.0))).normalized(), math.radians(-ELBOW) * x)
        d_lo = (bend @ d_up)
        if d_lo.y > d_up.y:           # whichever way round, the hand goes forward, not back
            d_lo = Quaternion(bend.axis, -bend.angle) @ d_up
        _point(lo, d_lo)
        _point(hand, d_lo)
        # the fingers relaxed open: each joint's built curl (a loose fist in the gloves) eased most of the way straight
        for f in ('index', 'middle', 'ring', 'pinky'):
            parent = P.get(f'{f}_metacarpal_{s}')
            for i in (1, 2, 3):
                pb = P.get(f'{f}_0{i}_{s}')
                if pb is None or parent is None:
                    break
                q = (pb.tail - pb.head).normalized().rotation_difference((parent.tail - parent.head).normalized())
                _about(pb, Quaternion().slerp(q, FINGER_EASE))
                parent = pb
        # palms down: the whole arm rolled about the shoulder-to-wrist line until the palm faces the ground
        axis = (hand.head - up.head).normalized()
        want = Vector((0.0, 0.0, -1.0))
        want = (want - axis * want.dot(axis)).normalized()
        have = _palm_normal(arm, s)
        have = (have - axis * have.dot(axis)).normalized()
        ang = math.atan2(axis.dot(have.cross(want)), have.dot(want))
        h = up.head.copy()
        _apply(up, Matrix.Translation(h) @ Matrix.Rotation(ang, 4, axis) @ Matrix.Translation(-h))
        log(f'arm {s}: rolled {math.degrees(ang):.0f} degrees; palm normal {tuple(round(v, 2) for v in _palm_normal(arm, s))}')


# ------------------------------------------------------------------------------------------------------------- render

def visible_meshes():
    return [o for o in bpy.context.scene.objects if o.type == 'MESH' and not o.hide_render]


def bounds(obs):
    dg = bpy.context.evaluated_depsgraph_get()
    pts = []
    for ob in obs:
        ev = ob.evaluated_get(dg)
        me = ev.to_mesh()
        co = np.empty(len(me.vertices) * 3)
        me.vertices.foreach_get('co', co)
        co = co.reshape(-1, 3)
        M = np.array(ob.matrix_world)
        pts.append(co @ M[:3, :3].T + M[:3, 3])
        ev.to_mesh_clear()
    p = np.concatenate(pts)
    return p.min(0), p.max(0)


def look(ob, target):
    ob.rotation_euler = (target - ob.location).to_track_quat('-Z', 'Y').to_euler()


def setup_scene():
    sc = bpy.context.scene
    sc.render.engine = 'CYCLES'
    sc.cycles.device = 'CPU'
    sc.cycles.samples = SAMPLES
    sc.cycles.use_adaptive_sampling = True
    sc.cycles.use_denoising = True
    try:
        sc.cycles.denoiser = 'OPENIMAGEDENOISE'
    except Exception:
        pass
    sc.cycles.transparent_max_bounces = 48
    sc.cycles.max_bounces = 8
    sc.render.resolution_x = sc.render.resolution_y = SIZE
    sc.render.resolution_percentage = 100
    sc.render.film_transparent = True
    sc.render.image_settings.file_format = 'PNG'
    sc.render.image_settings.color_mode = 'RGBA'
    sc.render.image_settings.color_depth = '8'
    try:
        sc.view_settings.view_transform = 'AgX'
        sc.view_settings.look = 'None'
    except Exception:
        pass
    for ob in [o for o in sc.objects if o.type in ('LIGHT', 'CAMERA')]:
        bpy.data.objects.remove(ob)
    world = bpy.data.worlds.get('HeroViews') or bpy.data.worlds.new('HeroViews')
    sc.world = world
    world.use_nodes = True
    bg = world.node_tree.nodes['Background']
    bg.inputs['Color'].default_value = (0.55, 0.56, 0.58, 1.0)    # soft even ambient from all round
    bg.inputs['Strength'].default_value = 0.5


def light(name, energy, size):
    ld = bpy.data.lights.new(name, 'AREA')
    ld.energy = energy
    ld.shape = 'DISK'
    ld.size = size
    ob = bpy.data.objects.new(name, ld)
    bpy.context.scene.collection.objects.link(ob)
    return ob


def white_copy(src, dst):
    """The transparent render over pure white (straight alpha, as a viewer composites it)."""
    img = bpy.data.images.load(src, check_existing=False)
    w, h = img.size
    px = np.empty(w * h * 4, dtype=np.float32)
    img.pixels.foreach_get(px)
    px = px.reshape(-1, 4)
    a = px[:, 3:4]
    px[:, :3] = px[:, :3] * a + (1.0 - a)
    px[:, 3] = 1.0
    out = bpy.data.images.new('white', w, h, alpha=True)
    out.pixels.foreach_set(px.ravel())
    out.filepath_raw = dst
    out.file_format = 'PNG'
    out.save()
    bpy.data.images.remove(img)
    bpy.data.images.remove(out)


def render_views(arm):
    for ob in bpy.context.scene.objects:
        if ob.type == 'MESH' and (ob.name.startswith(('UCX_', '_')) or ob.name.startswith(HELD.get(HERO, ()) or ('\0',))):
            ob.hide_render = True
    a_pose(arm)
    setup_scene()
    obs = visible_meshes()
    lo, hi = bounds(obs)
    c = Vector(((lo + hi) / 2).tolist())
    ext = hi - lo
    log(f'figure {np.round(ext, 3)} m (x, y, z); {len(obs)} meshes: {", ".join(o.name for o in obs)}')
    scale = max(ext[2] / FILL, ext[0] / 0.95, ext[1] / 0.95)
    cd = bpy.data.cameras.new('ViewCam')
    cd.type = 'ORTHO'
    cd.ortho_scale = scale
    cd.clip_start, cd.clip_end = 0.1, 50.0
    cam = bpy.data.objects.new('ViewCam', cd)
    bpy.context.scene.collection.objects.link(cam)
    bpy.context.scene.camera = cam
    # the studio rides with the camera: every view lit the same (key front-left above, fill right, rim behind above)
    key, fill, rim = light('Key', 150.0, 3.0), light('Fill', 60.0, 4.0), light('Rim', 75.0, 2.5)
    D = 12.0
    for d in ('upload', 'upload_white'):
        os.makedirs(os.path.join(OUT, d), exist_ok=True)
    for name, back in VIEWS:                               # back: from the figure toward the camera
        right = (-back).cross(Vector((0.0, 0.0, 1.0)))    # the image's right
        up = Vector((0.0, 0.0, 1.0))
        cam.location = c + back * D
        look(cam, c)
        key.location = c + back * 3.2 - right * 2.4 + up * 2.2
        fill.location = c + back * 3.4 + right * 3.0 + up * 0.4
        rim.location = c - back * 3.0 + right * 1.2 + up * 2.6
        for lt in (key, fill, rim):
            look(lt, c + up * 0.15)
        path = os.path.join(OUT, 'upload', f'{name}.png')
        bpy.context.scene.render.filepath = path
        bpy.ops.render.render(write_still=True)
        white_copy(path, os.path.join(OUT, 'upload_white', f'{name}.png'))
        log(f'wrote {name}')


# -------------------------------------------------------------------------------------------------------------- build

def save_blend():
    os.makedirs(os.path.dirname(BLEND), exist_ok=True)
    for img in bpy.data.images:
        if img.packed_file is None and img.source in ('FILE', 'GENERATED'):
            try:
                img.pack()
            except Exception as e:
                log(f'could not pack {img.name}: {e}')
    bpy.ops.wm.save_as_mainfile(filepath=BLEND, check_existing=False, compress=True, copy=True)
    log(f'saved {BLEND}')


def armature():
    arms = [o for o in bpy.context.scene.objects if o.type == 'ARMATURE']
    assert arms, 'no armature in the scene'
    return arms[0]


def instead_of_preview(*args, **kwargs):
    save_blend()
    render_views(armature())
    raise ViewsDone()


def build():
    path = os.path.join(REPO, 'Art', 'Models', 'Characters', f'{HERO}.py')
    # the hero script reads its flags from sys.argv when imported: as a --preview run of the full shot
    sys.argv = [sys.argv[0], '--', '--preview', '--shots', 'full']
    name = f'hero_{HERO}'                       # not "Crane": Art/Portraits has a Crane.py on the path too
    spec = importlib.util.spec_from_file_location(name, path)
    mod = importlib.util.module_from_spec(spec)
    sys.modules[name] = mod
    spec.loader.exec_module(mod)
    hook = 'render_set' if hasattr(mod, 'render_set') else 'preview'
    setattr(mod, hook, instead_of_preview)
    try:
        mod.main()
    except ViewsDone:
        log('done')
        return
    raise RuntimeError(f'{HERO}.py finished without reaching its preview: nothing rendered')


if __name__ == '__main__':
    if '--render' in MINE:
        render_views(armature())
        log('done')
    else:
        build()
