"""Tripo upload views of the heroes' melee tools (Art/Concepts/MeleeTools/README.md): the game's own models, built by
Art/Models/Weapons/Melee/MeleeTools.py unchanged (its materials, textures and baked occlusion), stood upright (the tool's
long axis vertical) and rendered from the front, back, left and right in orthographic Cycles, 1024 x 1024, the same
scale in all four views of a tool.

    Tools\\artrun.ps1 -Script Tools\\Blender\\melee_views.py [-ScriptArgs GraveSpade,RazorFan]
      --samples N (96)  --size N (1024)  --fill F (0.85: the tool's height in the frame)  --out <dir>  --glow F (0.25: the game colour, not blown to white)

Writes Art/Concepts/MeleeTools/<Tool>/upload/{front,back,left,right}.png (transparent) and upload_white/ (on #ffffff),
<Tool> = GraveSpade, RazorFan, FerryGaff, PistonFist, RockPick (the model's name without "Melee_").
Tool space (MeleeTools.py): u along the tool, v the strike side, w across. Standing: u up; the front's camera side is
the tool's +v (blade face, piston's top) or +w (the fan's card faces, the gaff's and pick's profile with the hook or
pick curling to the image's right). Left shows the tool's own left side (as the hero views: the tool faces the camera
in the front view), right its right.
"""
import importlib.util
import math
import os
import sys

import bpy
import numpy as np
from mathutils import Matrix, Vector

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
MINE = sys.argv[sys.argv.index('--') + 1:] if '--' in sys.argv else []


def opt(name, default):
    return type(default)(MINE[MINE.index(name) + 1]) if name in MINE else default


SAMPLES = opt('--samples', 96)
SIZE = opt('--size', 1024)
FILL = opt('--fill', 0.85)
GLOW = opt('--glow', 0.25)
OUT_ROOT = opt('--out', os.path.join(REPO, 'Art', 'Concepts', 'MeleeTools'))
ONLY = [a for a in ' '.join(a for a in MINE if not a.startswith('--') and not a[0].isdigit() and not a[0] == '/'
                            and ':' not in a).replace(',', ' ').split() if a]

# the model name -> (folder, the front's camera side in tool space: 'v' or 'w')
TOOLS = {'Melee_GraveSpade': ('GraveSpade', 'v'), 'Melee_RazorFan': ('RazorFan', 'w'),
         'Melee_FerryGaff': ('FerryGaff', 'w'), 'Melee_PistonFist': ('PistonFist', 'v'),
         'Melee_RockPick': ('RockPick', 'w')}
# Standing: tool u (Blender -Y) -> +Z up, v (Blender +Z) -> -Y, w (Blender +X) -> -X
STAND = Matrix(((-1.0, 0.0, 0.0, 0.0), (0.0, 0.0, -1.0, 0.0), (0.0, -1.0, 0.0, 0.0), (0.0, 0.0, 0.0, 1.0)))
SIDE = {'v': Vector((0.0, -1.0, 0.0)), 'w': Vector((-1.0, 0.0, 0.0))}


def log(m):
    print(f'MELEE_VIEWS {m}', flush=True)


def build():
    path = os.path.join(REPO, 'Art', 'Models', 'Weapons', 'Melee', 'MeleeTools.py')
    sys.argv = [sys.argv[0], '--']          # no --preview: the script just builds
    spec = importlib.util.spec_from_file_location('melee_tools_model', path)
    mod = importlib.util.module_from_spec(spec)
    sys.modules['melee_tools_model'] = mod
    spec.loader.exec_module(mod)
    return mod.MODELS


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
        sc.view_settings.view_transform = 'Standard'
        sc.view_settings.look = 'None'
    except Exception:
        pass
    for ob in [o for o in sc.objects if o.type in ('LIGHT', 'CAMERA')]:
        bpy.data.objects.remove(ob)
    world = bpy.data.worlds.get('MeleeViews') or bpy.data.worlds.new('MeleeViews')
    sc.world = world
    world.use_nodes = True
    bg = world.node_tree.nodes['Background']
    bg.inputs['Color'].default_value = (0.7, 0.71, 0.73, 1.0)     # soft even ambient from all round (metal reflects it)
    bg.inputs['Strength'].default_value = 0.9


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


def tune_glow():
    """Emissive surfaces: the game colour, bright but not blown to white (the preview strengths are 4-5)."""
    for mat in bpy.data.materials:
        if 'Glow' in mat.keys() and mat.use_nodes:
            for n in mat.node_tree.nodes:
                if n.type == 'BSDF_PRINCIPLED':
                    n.inputs['Emission Strength'].default_value = float(mat['Glow']) * GLOW


def render_tool(model, cam, lights):
    folder, side = TOOLS[model.name]
    out = os.path.join(OUT_ROOT, folder)
    for d in ('upload', 'upload_white'):
        os.makedirs(os.path.join(out, d), exist_ok=True)
    for ob in bpy.context.scene.objects:
        if ob.type == 'MESH':
            ob.hide_render = ob is not model
    model.matrix_world = STAND
    bpy.context.view_layer.update()
    lo, hi = bounds([model])
    c = Vector(((lo + hi) / 2).tolist())
    ext = hi - lo
    log(f'{folder}: {np.round(ext * 100, 1)} cm (x, y, z)')
    cam.data.ortho_scale = max(ext[2] / FILL, ext[0] / 0.95, ext[1] / 0.95)
    front = SIDE[side]
    up = Vector((0.0, 0.0, 1.0))
    left = up.cross(front)                       # the tool's own left (it faces the camera in the front view)
    key, fill, rim = lights
    D = 12.0
    for name, back in (('front', front), ('back', -front), ('left', left), ('right', -left)):
        right = (-back).cross(up)                # the image's right
        cam.location = c + back * D
        look(cam, c)
        key.location = c + back * 1.6 - right * 1.2 + up * 1.1
        fill.location = c + back * 1.7 + right * 1.5 + up * 0.2
        rim.location = c - back * 1.5 + right * 0.6 + up * 1.3
        for lt in lights:
            look(lt, c)
        path = os.path.join(out, 'upload', f'{name}.png')
        bpy.context.scene.render.filepath = path
        bpy.ops.render.render(write_still=True)
        white_copy(path, os.path.join(out, 'upload_white', f'{name}.png'))
        log(f'{folder}: wrote {name}')


def main():
    models = build()
    setup_scene()
    tune_glow()
    cd = bpy.data.cameras.new('ViewCam')
    cd.type = 'ORTHO'
    cd.clip_start, cd.clip_end = 0.1, 50.0
    cam = bpy.data.objects.new('ViewCam', cd)
    bpy.context.scene.collection.objects.link(cam)
    bpy.context.scene.camera = cam
    lights = (light('Key', 40.0, 1.5), light('Fill', 18.0, 2.0), light('Rim', 22.0, 1.2))
    for model in models:
        if ONLY and TOOLS[model.name][0] not in ONLY:
            continue
        render_tool(model, cam, lights)
    log('done')


main()
