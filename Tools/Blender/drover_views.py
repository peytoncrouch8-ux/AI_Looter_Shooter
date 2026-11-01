"""Tripo upload views of the Drover's part sets (lane TRIPO-PISTOLS, 2026-11-01): the game's own scripted parts, built by
Art/Models/Weapons/Drover.py unchanged in shape (its materials, textures and baked occlusion), each set's parts wearing its
maker's colours (Docs/Work/2026-10-22-Next/MAKERS.md, as the AR's and Ranchhand's Tripo sets: 1 Saint Maud, 2 Roundhouse,
3 Windfall, 4 Meridian), rendered from the front, back, left and right in orthographic Cycles, 1024 x 1024, one scale
across a part's four views, the part filling ~84% (the AR's concept views' rules).

    Tools\\artrun.ps1 -Script Tools\\Blender\\drover_views.py [-ScriptArgs Body_Ironclad,Grip_Coffin]
      --samples N (96)  --size N (1024)  --fill F (0.84)  --glow F (0.25)  --out <dir>  --plain (no dressing)

Writes Art/Concepts/Weapons/Drover/<Slot>_<Key>/upload/{front,back,left,right}.png (transparent) and upload_white/ (on
#ffffff). Views (as the AR's): front = from the muzzle end looking back, back = from the grip's end, left = the gun's left
side (muzzle to the image's left), right = its right side (muzzle to the right).
The cylinder's views show the drum alone (its rounds, rims and ratchet): the ejector rod, its sleeve and the crane are
the scripted pieces in the game (Art/Models/Weapons/Tripo/Drover/parts_set<N>.py keeps them), so Tripo makes only the part
that turns. Each set's maker detail (rivets, bands, tape, pinstripes, checkering) is added for the views only
(Tools/Blender/drover_dressing.py), as the AR's concept views were drawn richer than its scripted parts.
"""
import importlib.util
import math
import os
import sys

import bpy
import bmesh
import numpy as np
from mathutils import Vector

import drover_dressing

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
MINE = sys.argv[sys.argv.index('--') + 1:] if '--' in sys.argv else []


def opt(name, default):
    return type(default)(MINE[MINE.index(name) + 1]) if name in MINE else default


SAMPLES = opt('--samples', 96)
SIZE = opt('--size', 1024)
FILL = opt('--fill', 0.84)
GLOW = opt('--glow', 0.25)
OUT_ROOT = opt('--out', os.path.join(REPO, 'Art', 'Concepts', 'Weapons', 'Drover'))
PLAIN = '--plain' in MINE   # the scripted part as it is, without the maker's dressing (drover_dressing.py)
ONLY = [a for a in ' '.join(a for a in MINE if not a.startswith('--') and not a.replace('.', '').isdigit()
                            and ':' not in a and '\\' not in a and '/' not in a).replace(',', ' ').split() if a]

# The four sets (the user's AR rule: each set its own designs, never another recoloured: here each set is other keys,
# each its own scripted shape). Keys are Drover.parts.csv's; none new, none renamed.
SETS = {
    1: ('SaintMaud', ['Body_Blued', 'Barrel_Service', 'Cylinder_Fluted6', 'Sight_Notch', 'Grip_Plowhandle']),
    2: ('Roundhouse', ['Body_Ironclad', 'Barrel_Lugged', 'Cylinder_Bigbore5', 'Sight_Glass', 'Grip_Coffin']),
    3: ('Windfall', ['Body_Scrapper', 'Barrel_Comped', 'Cylinder_Plain6', 'Sight_Peep', 'Grip_Wrapped']),
    4: ('Meridian', ['Body_Ranger', 'Barrel_Vented', 'Cylinder_Moon6', 'Sight_Pip', 'Grip_Target']),
}
# Each maker's colours over the scripted materials: material name -> (texture set, tint). The AR's and Ranchhand's Tripo
# sets: 1 ivory enamel, cast iron, gilt brass; 2 oxblood, black iron, brass; 3 sea teal over rust, galvanized tin, copper;
# 4 satin white, graphite, machined aluminium, fine brass. Others (glows, glass, heritage) keep theirs.
LOOKS = {
    'SaintMaud': {'GunPolymerSand': ('Polymer', 0xe8e0cc), 'GunSteelBlued': ('MetalWorn', 0x3a3633),
                  'GunBlackMetal': ('MetalWorn', 0x34302c), 'GunSteelBare': ('MetalWorn', 0x77726c),
                  'GunBrass': ('MetalWorn', 0xe8c06a), 'GunPolymerBlack': ('Polymer', 0x3a3633),
                  'GunPolymerGrey': ('Polymer', 0x5a5450)},
    'Roundhouse': {'GunPolymerSand': ('Polymer', 0x8e3a2a), 'GunSteelBlued': ('MetalWorn', 0x2c2d30),
                   'GunBlackMetal': ('MetalWorn', 0x2a2b2e), 'GunSteelBare': ('MetalWorn', 0x5a5c60),
                   'GunBrass': ('MetalWorn', 0xd9a441), 'GunPolymerBlack': ('Polymer', 0x2c2d30),
                   'GunPolymerGrey': ('Polymer', 0xefe3c4)},
    'Windfall': {'GunPolymerSand': ('Polymer', 0x4f8f8a), 'GunSteelBlued': ('MetalRust', 0x9a7a66),
                 'GunBlackMetal': ('MetalWorn', 0x3a3836), 'GunSteelBare': ('MetalWorn', 0x8f9396),
                 'GunBrass': ('MetalWorn', 0xc9794a), 'GunPolymerBlack': ('Polymer', 0x2e2d2b),
                 'GunPolymerGrey': ('Polymer', 0x7d9fb5), 'GunRubber': ('Polymer', 0x262626),
                 'GunTape': ('Polymer', 0x2a2a2a)},
    'Meridian': {'GunPolymerSand': ('Polymer', 0xeeece6), 'GunSteelBlued': ('MetalWorn', 0x3a3d42),
                 'GunBlackMetal': ('MetalWorn', 0x2f3134), 'GunSteelBare': ('MetalWorn', 0xb8bcc0),
                 'GunBrass': ('MetalWorn', 0xc99a4a), 'GunPolymerBlack': ('Polymer', 0x3a3d42),
                 'GunPolymerGrey': ('Polymer', 0xb8bcc0)},
}


def log(m):
    print(f'DROVER_VIEWS {m}', flush=True)


def build():
    path = os.path.join(REPO, 'Art', 'Models', 'Weapons', 'Drover.py')
    sys.argv = [sys.argv[0], '--']          # no --preview: the script just builds
    os.environ['LOOTER_DROVER_TRIPO'] = '0'   # the scripted parts are the views' source
    spec = importlib.util.spec_from_file_location('drover_model', path)
    mod = importlib.util.module_from_spec(spec)
    sys.modules['drover_model'] = mod
    spec.loader.exec_module(mod)
    return mod


def maker_look(obj, maker):
    """Swaps obj's materials for the maker's colours (copies, so the other parts keep theirs)."""
    import looter_textures as lt
    for slot in obj.material_slots:
        mat = slot.material
        if mat is None or mat.name not in LOOKS[maker]:
            continue
        set_name, tint = LOOKS[maker][mat.name]
        name = f'{mat.name}_{maker}'
        slot.material = bpy.data.materials.get(name) or lt.material(set_name, name=name, master='Gun', tint=tint)


def drum_only(obj, mod):
    """Drops the cylinder's ejector rod, sleeve and crane (outside the drum: past its front face, or off its axis)."""
    # By piece: the drum, bands, moon clip and ratchet sit on the axis, the rounds and rims round it inside the drum; the
    # rod, its head and sleeve and the crane's arm lie past the front face, the crane's hinge tube 2.8 cm off the axis.
    me = obj.data
    cm = 0.01
    length = mod.CYL_U1 - mod.CYL_U0
    bm = bmesh.new()
    bm.from_mesh(me)
    seen, doomed = set(), []
    for face in bm.faces:
        if face.index in seen:
            continue
        stack, piece = [face], []
        seen.add(face.index)
        while stack:
            f = stack.pop()
            piece.append(f)
            for e in f.edges:
                for g in e.link_faces:
                    if g.index not in seen:
                        seen.add(g.index)
                        stack.append(g)
        pts = [v.co for f in piece for v in f.verts]
        c = sum(pts, Vector()) / len(pts)
        if -c.y / cm > length + 0.05 or math.hypot(c.x, c.z) / cm > 2.0:
            doomed += piece
    bmesh.ops.delete(bm, geom=list(set(doomed)), context='FACES')
    bm.to_mesh(me)
    bm.free()
    log(f'{obj.name}: {len(set(doomed))} faces of rod and crane left out of the views')


def bounds(obs):
    pts = []
    for ob in obs:
        co = np.empty(len(ob.data.vertices) * 3)
        ob.data.vertices.foreach_get('co', co)
        co = co.reshape(-1, 3)
        M = np.array(ob.matrix_world)
        pts.append(co @ M[:3, :3].T + M[:3, 3])
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
    world = bpy.data.worlds.get('DroverViews') or bpy.data.worlds.new('DroverViews')
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
    """Emissive surfaces: the game colour, bright but not blown to white (the preview strengths are 4-6)."""
    for mat in bpy.data.materials:
        if 'Glow' in mat.keys() and mat.use_nodes:
            for n in mat.node_tree.nodes:
                if n.type == 'BSDF_PRINCIPLED':
                    n.inputs['Emission Strength'].default_value = float(mat['Glow']) * GLOW


def render_part(key, obj, cam, lights):
    out = os.path.join(OUT_ROOT, key)
    for d in ('upload', 'upload_white'):
        os.makedirs(os.path.join(out, d), exist_ok=True)
    for ob in bpy.context.scene.objects:
        if ob.type == 'MESH':
            ob.hide_render = ob is not obj
    obj.location = (0.0, 0.0, 0.0)
    obj.rotation_euler = (0.0, 0.0, 0.0)
    bpy.context.view_layer.update()
    lo, hi = bounds([obj])
    c = Vector(((lo + hi) / 2).tolist())
    ext = hi - lo
    log(f'{key}: {np.round(ext * 100, 2)} cm (x across, y along, z up)')
    cam.data.ortho_scale = float(max(ext)) / FILL
    up = Vector((0.0, 0.0, 1.0))
    key_l, fill, rim = lights
    D = 3.0
    # Gun space in Blender: muzzle -Y, the gun's left +X, up +Z.
    for name, back in (('front', Vector((0.0, -1.0, 0.0))), ('back', Vector((0.0, 1.0, 0.0))),
                       ('left', Vector((1.0, 0.0, 0.0))), ('right', Vector((-1.0, 0.0, 0.0)))):
        right = (-back).cross(up)                # the image's right
        cam.location = c + back * D
        look(cam, c)
        key_l.location = c + back * 0.8 - right * 0.6 + up * 0.55
        fill.location = c + back * 0.85 + right * 0.75 + up * 0.1
        rim.location = c - back * 0.75 + right * 0.3 + up * 0.65
        for lt_ in lights:
            look(lt_, c)
        path = os.path.join(out, 'upload', f'{name}.png')
        bpy.context.scene.render.filepath = path
        bpy.ops.render.render(write_still=True)
        white_copy(path, os.path.join(out, 'upload_white', f'{name}.png'))
    log(f'{key}: wrote four views')


def main():
    mod = build()
    setup_scene()
    tune_glow()
    cd = bpy.data.cameras.new('ViewCam')
    cd.type = 'ORTHO'
    cd.clip_start, cd.clip_end = 0.01, 10.0
    cam = bpy.data.objects.new('ViewCam', cd)
    bpy.context.scene.collection.objects.link(cam)
    bpy.context.scene.camera = cam
    lights = (light('Key', 10.0, 0.5), light('Fill', 4.5, 0.7), light('Rim', 5.5, 0.4))
    for number, (maker, keys) in SETS.items():
        for key in keys:
            if ONLY and key not in ONLY:
                continue
            obj = mod.built[key]
            obj.location = (0.0, 0.0, 0.0)   # the part's own space (the script laid it out in rows): dressing joins into it
            obj.rotation_euler = (0.0, 0.0, 0.0)
            bpy.context.view_layer.update()
            maker_look(obj, maker)
            if key.startswith('Cylinder_'):
                drum_only(obj, mod)
            if not PLAIN:
                added = drover_dressing.dress(key, obj, mod.origins[key], mod, maker, LOOKS)
                log(f'{key}: {added} dressing pieces ({maker})')
            render_part(key, obj, cam, lights)
    log('done')


main()
