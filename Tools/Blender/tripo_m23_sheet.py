"""Pictures of Mains 2-3's Tripo models (Tools/Blender/looter_tripo_m23.py) in Saved/ArtPreviews/TripoM23/:
  --check Name[,Name]   each model alone, fitted as exported: three-quarter, side, top and front, its hulls in red wire
                        and its sockets as magenta balls (Check_<Name>_<view>.png)
  (no flag)             Sheet.png: the six beside the game's own props at one scale (a 1.8 m figure, the Tripo
                        loudspeaker post and sacks, the scripted flour sacks and silk cocoon), then each model in a tile.
    Tools\\artrun.ps1 -Script Tools\\Blender\\tripo_m23_sheet.py [-ScriptArgs '--check,SextonsFerry']
"""
import math
import os
import sys

import bpy
import numpy as np
from mathutils import Vector

import looter_textures as lt
import looter_tripo_m23 as m23
import looter_tripo_wardens as tw

REPO = lt.REPO
OUT = os.path.join(REPO, 'Saved', 'ArtPreviews', 'TripoM23')
os.makedirs(OUT, exist_ok=True)
args = [a for raw in (sys.argv[sys.argv.index('--') + 1:] if '--' in sys.argv else []) for a in raw.split(',') if a]


def scene_setup(res=(900, 900), ground=True):
    scene = bpy.context.scene
    engines = [e.identifier for e in bpy.types.RenderSettings.bl_rna.properties['engine'].enum_items]
    scene.render.engine = 'BLENDER_EEVEE_NEXT' if 'BLENDER_EEVEE_NEXT' in engines else 'BLENDER_EEVEE'
    scene.render.resolution_x, scene.render.resolution_y = res
    scene.view_settings.view_transform = 'AgX' if 'AgX' in [i.identifier for i in
                                                             scene.view_settings.bl_rna.properties['view_transform'].enum_items] else 'Filmic'
    world = bpy.data.worlds.get('W') or bpy.data.worlds.new('W')
    world.use_nodes = True
    world.node_tree.nodes['Background'].inputs[0].default_value = (0.55, 0.6, 0.68, 1)
    world.node_tree.nodes['Background'].inputs[1].default_value = 0.8
    scene.world = world
    if not bpy.data.objects.get('_Sun'):
        sun = bpy.data.objects.new('_Sun', bpy.data.lights.new('_Sun', 'SUN'))
        sun.data.energy = 3.2
        sun.data.color = (1.0, 0.95, 0.86)
        sun.rotation_euler = (math.radians(48), 0, math.radians(35))
        scene.collection.objects.link(sun)
    if ground and not bpy.data.objects.get('_Ground'):
        bpy.ops.mesh.primitive_plane_add(size=200)
        g = bpy.context.active_object
        g.name = '_Ground'
        mat = bpy.data.materials.new('_GroundMat')
        mat.use_nodes = True
        mat.node_tree.nodes['Principled BSDF'].inputs['Base Color'].default_value = (0.22, 0.2, 0.17, 1)
        mat.node_tree.nodes['Principled BSDF'].inputs['Roughness'].default_value = 1.0
        g.data.materials.append(mat)
    cam = bpy.data.objects.get('_Cam')
    if not cam:
        cam = bpy.data.objects.new('_Cam', bpy.data.cameras.new('_Cam'))
        scene.collection.objects.link(cam)
    scene.camera = cam
    return scene, cam


def bounds(objs):
    pts = [o.matrix_world @ Vector(c) for o in objs for c in o.bound_box]
    lo = Vector((min(p.x for p in pts), min(p.y for p in pts), min(p.z for p in pts)))
    hi = Vector((max(p.x for p in pts), max(p.y for p in pts), max(p.z for p in pts)))
    return lo, hi


def look(cam, target, direction, distance, ortho=None):
    cam.location = target + Vector(direction).normalized() * distance
    cam.rotation_euler = (target - cam.location).to_track_quat('-Z', 'Y').to_euler()
    if ortho:
        cam.data.type = 'ORTHO'
        cam.data.ortho_scale = ortho
    else:
        cam.data.type = 'PERSP'
        cam.data.lens = 50


def render(path):
    bpy.context.scene.render.filepath = path
    bpy.ops.render.render(write_still=True)
    print(f'LOOTER: wrote {path}', flush=True)


def mark_helpers(obj):
    red = bpy.data.materials.get('_Red') or bpy.data.materials.new('_Red')
    red.use_nodes = True
    red.node_tree.nodes['Principled BSDF'].inputs['Base Color'].default_value = (1, 0.05, 0.05, 1)
    red.node_tree.nodes['Principled BSDF'].inputs['Emission Color'].default_value = (1, 0.05, 0.05, 1)
    red.node_tree.nodes['Principled BSDF'].inputs['Emission Strength'].default_value = 2.0
    mag = bpy.data.materials.get('_Mag') or bpy.data.materials.new('_Mag')
    mag.use_nodes = True
    mag.node_tree.nodes['Principled BSDF'].inputs['Emission Color'].default_value = (1, 0, 1, 1)
    mag.node_tree.nodes['Principled BSDF'].inputs['Emission Strength'].default_value = 4.0
    for child in list(obj.children):
        if child.name.startswith('UCX_'):
            mod = child.modifiers.new('wire', 'WIREFRAME')
            mod.thickness = 0.015
            child.data.materials.clear()
            child.data.materials.append(red)
            child.display_type = 'TEXTURED'
        elif child.name.startswith('SOCKET_'):
            bpy.ops.mesh.primitive_uv_sphere_add(radius=0.05, location=child.matrix_world.translation)
            ball = bpy.context.active_object
            ball.name = '_Ball_' + child.name
            ball.data.materials.append(mag)


def check(names):
    for name in names:
        bpy.ops.wm.read_factory_settings(use_empty=True)
        obj, report = m23.build(name)
        bpy.context.view_layer.update()
        mark_helpers(obj)
        scene, cam = scene_setup()
        lo, hi = bounds([obj])
        bpy.data.objects['_Ground'].location.z = lo.z - 0.002
        c = (lo + hi) / 2
        size = max(hi - lo)
        for view, d, ortho in (('q', (-0.75, -1.0, 0.55), None), ('side', (-1, 0, 0.0001), size * 1.15),
                               ('front', (0, -1, 0.0001), size * 1.15), ('top', (0.0001, 0.0001, 1), size * 1.15),
                               ('back', (0.7, 0.9, 0.5), None)):
            look(cam, c, d, size * 2.0, ortho)
            render(os.path.join(OUT, f'Check_{name}_{view}.png'))


def sheet():
    """All six at one scale on a line, beside the game's props and a 1.8 m figure."""
    bpy.ops.wm.read_factory_settings(use_empty=True)
    built = []
    for name in m23.ORDER:
        if name in m23.BASE:
            obj, _ = m23.build(name)
            built.append(obj)
    refs = []
    for name in ('LoudspeakerPost', 'BarricadeSacks'):
        obj, _ = tw.build(name)
        refs.append(obj)
    for fbx in (os.path.join(REPO, 'Intermediate', 'ArtExport_Clutter', 'SM_FlourSacks.fbx'),
                os.path.join(REPO, 'Intermediate', 'ArtExport_RR_DenDressing', 'SM_Cocoon_Lying.fbx')):
        if os.path.exists(fbx):
            before = set(bpy.data.objects)
            bpy.ops.import_scene.fbx(filepath=fbx)
            new = [o for o in bpy.data.objects if o not in before and o.type == 'MESH' and not o.name.startswith('UCX')]
            for o in [o for o in bpy.data.objects if o not in before and o.name.startswith('UCX')]:
                bpy.data.objects.remove(o)
            for o in new:
                for slot in o.material_slots:
                    if slot.material:
                        set_name = slot.material.name.replace('MI_', '').split('.')[0]
                        if os.path.exists(lt.texture_path(set_name, 'BC')):
                            slot.material = lt.material(set_name)
            refs += new
    for o in bpy.data.objects:
        if o.name.startswith('UCX_') or o.name.startswith('SOCKET_'):
            o.hide_render = True
    # A 1.8 m figure.
    bpy.ops.mesh.primitive_cylinder_add(radius=0.22, depth=1.8, location=(0, 0, 0.9))
    man = bpy.context.active_object
    man.name = '_Figure'
    refs.insert(0, man)
    # Line them up along X, each model's footprint with a gap; the banner hung 2 m up.
    x = 0.0
    order = refs[:1] + built + refs[1:]
    for o in order:
        bpy.context.view_layer.update()
        lo, hi = bounds([o])
        o.location.x += x - lo.x
        o.location.y -= (lo.y + hi.y) / 2
        # Each stands on the ground (the ferry's pivot is its gunwale), the banner hung 1.5 m up.
        o.location.z += (1.5 if o.name == 'CompanyBanner' else 0.0) - lo.z
        x += (hi.x - lo.x) + 0.6
    scene, cam = scene_setup((2400, 900))
    bpy.context.view_layer.update()
    lo, hi = bounds(order)
    c = (lo + hi) / 2
    look(cam, c, (0.0001, -1.0, 0.18), 60.0, (hi.x - lo.x) * 1.04)
    render(os.path.join(OUT, 'Sheet_Line.png'))
    look(cam, c + Vector((0, 0, 0)), (-0.35, -1.0, 0.45), 60.0, (hi.x - lo.x) * 1.04)
    render(os.path.join(OUT, 'Sheet_Line34.png'))
    compose()


def compose():
    """Sheet.png: the line-up across the top, each model's three-quarter check below (three a row)."""
    tiles = [os.path.join(OUT, f'Check_{n}_q.png') for n in m23.ORDER]
    tiles = [t for t in tiles if os.path.exists(t)]
    width, cell = 2400, 800
    line = bpy.data.images.load(os.path.join(OUT, 'Sheet_Line.png'))
    lw, lh = line.size
    rows = (len(tiles) + 2) // 3
    height = lh + rows * cell
    canvas = np.ones((height, width, 4), np.float32) * 0.5
    canvas[..., 3] = 1.0

    def paste(img, x, y, w, h):
        img.scale(w, h)
        px = np.array(img.pixels[:], np.float32).reshape(h, w, 4)
        # Blender's rows run bottom up; the canvas is built bottom up too, so y counts from the canvas's bottom.
        canvas[y:y + h, x:x + w] = px

    paste(line, 0, height - lh, lw, lh)
    for k, t in enumerate(tiles):
        img = bpy.data.images.load(t)
        r, c = divmod(k, 3)
        paste(img, c * cell, height - lh - (r + 1) * cell, cell, cell)
    out = bpy.data.images.new('_Sheet', width, height)
    out.pixels = canvas.ravel()
    out.filepath_raw = os.path.join(OUT, 'Sheet.png')
    out.file_format = 'PNG'
    out.save()
    print(f'LOOTER: wrote {out.filepath_raw}', flush=True)


if args and args[0] == '--check':
    check(args[1:] if len(args) > 1 else m23.names())
else:
    sheet()
