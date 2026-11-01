"""Pictures of Mains 4-7's Tripo models (Tools/Blender/looter_tripo_m47.py) in Saved/ArtPreviews/TripoM47/:
  --check Name[,Name]   each model alone, fitted as exported: three-quarter, side, top, front and back, its hulls in red
                        wire and its sockets as magenta balls (Check_<Name>_<view>.png)
  (no flag)             Sheet.png: the two beside the game's own models at one scale (a 1.8 m figure, the scripted
                        smashed Reliquary it replaces, the chapel bell, Abel's lantern, the stand-in rock SM_Rock_B at the
                        coal's size), a close row of the coal beside the stand-in rock, then each model's checks.
    Tools\\artrun.ps1 -Script Tools\\Blender\\tripo_m47_sheet.py [-ScriptArgs '--check,SoulCoal']
"""
import math
import os
import sys

import bpy
import numpy as np
from mathutils import Vector

import looter_textures as lt
import looter_tripo_m47 as m47

REPO = lt.REPO
OUT = os.path.join(REPO, 'Saved', 'ArtPreviews', 'TripoM47')
os.makedirs(OUT, exist_ok=True)
args = [a for raw in (sys.argv[sys.argv.index('--') + 1:] if '--' in sys.argv else []) for a in raw.split(',') if a]
VIEWS = ('q', 'front', 'side', 'top', 'back')


def scene_setup(res=(900, 900), ground=True, dusk=False):
    scene = bpy.context.scene
    engines = [e.identifier for e in bpy.types.RenderSettings.bl_rna.properties['engine'].enum_items]
    scene.render.engine = 'BLENDER_EEVEE_NEXT' if 'BLENDER_EEVEE_NEXT' in engines else 'BLENDER_EEVEE'
    scene.render.resolution_x, scene.render.resolution_y = res
    scene.view_settings.view_transform = 'AgX'
    world = bpy.data.worlds.get('W') or bpy.data.worlds.new('W')
    world.use_nodes = True
    world.node_tree.nodes['Background'].inputs[0].default_value = (0.55, 0.6, 0.68, 1)
    world.node_tree.nodes['Background'].inputs[1].default_value = 0.25 if dusk else 0.8
    scene.world = world
    sun = bpy.data.objects.get('_Sun')
    if not sun:
        sun = bpy.data.objects.new('_Sun', bpy.data.lights.new('_Sun', 'SUN'))
        sun.data.color = (1.0, 0.95, 0.86)
        sun.rotation_euler = (math.radians(48), 0, math.radians(35))
        scene.collection.objects.link(sun)
    sun.data.energy = 0.6 if dusk else 3.2
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


def glow_preview():
    """The glow slots (their Glow property) shown emissive in the renders, as the game draws them."""
    for mat in bpy.data.materials:
        glow = mat.get('Glow')
        if glow and mat.use_nodes:
            bsdf = next((n for n in mat.node_tree.nodes if n.type == 'BSDF_PRINCIPLED'), None)
            if bsdf:
                bsdf.inputs['Emission Color'].default_value = bsdf.inputs['Base Color'].default_value
                bsdf.inputs['Emission Strength'].default_value = float(glow) * 0.6


def mark_helpers(obj, ball=0.03):
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
            mod.thickness = 0.004
            child.data.materials.clear()
            child.data.materials.append(red)
        elif child.name.startswith('SOCKET_'):
            bpy.ops.mesh.primitive_uv_sphere_add(radius=ball, location=child.matrix_world.translation)
            b = bpy.context.active_object
            b.name = '_Ball_' + child.name
            b.data.materials.append(mag)


def check(names):
    for name in names:
        bpy.ops.wm.read_factory_settings(use_empty=True)
        obj, report = m47.build(name)
        bpy.context.view_layer.update()
        glow_preview()
        lo, hi = bounds([obj])
        size = max(hi - lo)
        mark_helpers(obj, ball=0.02 * size / 0.3 if size < 0.5 else 0.03)
        scene, cam = scene_setup()
        bpy.data.objects['_Ground'].location.z = lo.z - 0.002
        c = (lo + hi) / 2
        for view, d, ortho in (('q', (-0.75, -1.0, 0.65), None), ('side', (-1, 0, 0.0001), size * 1.25),
                               ('front', (0, -1, 0.0001), size * 1.25), ('top', (0.0001, 0.0001, 1), size * 1.25),
                               ('back', (0.7, 0.9, 0.5), None)):
            look(cam, c, d, size * 2.2, ortho)
            render(os.path.join(OUT, f'Check_{name}_{view}.png'))


def import_fbx(path):
    """A game model's export (its UCX hulls dropped), its texture-set materials rebuilt so it renders as in the game."""
    if not os.path.exists(path):
        print(f'LOOTER: no {path}', flush=True)
        return []
    before = set(bpy.data.objects)
    bpy.ops.import_scene.fbx(filepath=path)
    new = [o for o in bpy.data.objects if o not in before]
    for o in [o for o in new if o.name.startswith('UCX') or o.type != 'MESH']:
        bpy.data.objects.remove(o)
    new = [o for o in bpy.data.objects if o not in before]
    for o in new:
        for slot in o.material_slots:
            if slot.material:
                set_name = slot.material.name.replace('MI_', '').split('.')[0]
                if os.path.exists(lt.texture_path(set_name, 'BC')):
                    slot.material = lt.material(set_name)
    return new


def lineup(order, gap, path, res, direction=(-0.35, -1.0, 0.4), floor=None):
    x = 0.0
    for o in order:
        bpy.context.view_layer.update()
        lo, hi = bounds([o])
        o.location.x += x - lo.x
        o.location.y -= (lo.y + hi.y) / 2
        o.location.z -= lo.z
        x += (hi.x - lo.x) + gap
    scene, cam = scene_setup(res)
    bpy.context.view_layer.update()
    lo, hi = bounds(order)
    c = (lo + hi) / 2
    look(cam, c, direction, 60.0, max(hi.x - lo.x, (hi.z - lo.z) * res[0] / res[1]) * 1.08)
    render(path)


def sheet():
    """The line-up at one scale, the coal close beside its stand-in, then the checks (compose)."""
    bpy.ops.wm.read_factory_settings(use_empty=True)
    reliquary, _ = m47.build('ReliquaryWhole')
    coal, _ = m47.build('SoulCoal')
    glow_preview()
    smashed = import_fbx(os.path.join(REPO, 'Intermediate', 'ArtExport_RR_Reliquary', 'SM_Reliquary_Smashed.fbx'))
    bell = import_fbx(os.path.join(REPO, 'Intermediate', 'ArtExport_RR_Chapel', 'SM_ChapelBell.fbx'))
    lantern = import_fbx(os.path.join(REPO, 'Intermediate', 'ArtExport_RR_Abel', 'SM_AbelLantern.fbx'))
    rock = import_fbx(os.path.join(REPO, 'Intermediate', 'ArtExport_RocksProps', 'SM_Rock_B.fbx'))
    for o in bpy.data.objects:
        if o.name.startswith('UCX_') or o.name.startswith('SOCKET_'):
            o.hide_render = True
    # The stand-in rock at the coal's size (the code fits either by its bounds).
    for r in rock:
        bpy.context.view_layer.update()
        lo, hi = bounds([r])
        r.scale *= 0.30 / max(hi.x - lo.x, hi.y - lo.y)
    bpy.ops.mesh.primitive_cylinder_add(radius=0.22, depth=1.8, location=(0, 0, 0.9))
    man = bpy.context.active_object
    man.name = '_Figure'
    order = [man, reliquary] + smashed[:1] + bell[:1] + lantern[:1] + [coal] + rock[:1]
    lineup(order, 0.45, os.path.join(OUT, 'Sheet_Line.png'), (2400, 900))
    # Close: the coal and the stand-in rock, by day and at dusk (the veins).
    for o in list(bpy.data.objects):
        if o not in (coal, rock[0] if rock else None, bpy.data.objects.get('_Cam'), bpy.data.objects.get('_Sun'),
                     bpy.data.objects.get('_Ground')) and o.parent is None:
            o.hide_render = True
    for o in [coal] + rock[:1]:
        o.location = (0, 0, 0)
    lineup([coal] + rock[:1], 0.12, os.path.join(OUT, 'Sheet_Coal.png'), (1200, 600), (-0.5, -1.0, 0.8))
    scene_setup((1200, 600), dusk=True)
    render(os.path.join(OUT, 'Sheet_CoalDusk.png'))
    compose()


def compose():
    """Sheet.png: the line-up across the top; the coal by day and dusk; each model's three-quarter, front and top."""
    width = 2400
    rows = [[os.path.join(OUT, 'Sheet_Line.png')],
            [os.path.join(OUT, 'Sheet_Coal.png'), os.path.join(OUT, 'Sheet_CoalDusk.png')]]
    for name in m47.ORDER:
        rows.append([os.path.join(OUT, f'Check_{name}_{v}.png') for v in ('q', 'front', 'top')])
    loaded = []
    for row in rows:
        files = [f for f in row if os.path.exists(f)]
        if not files:
            continue
        cell = width // len(files)
        imgs = []
        for f in files:
            img = bpy.data.images.load(f)
            w, h = img.size
            ch = int(round(h * cell / w))
            img.scale(cell, ch)
            imgs.append((np.array(img.pixels[:], np.float32).reshape(ch, cell, 4), cell, ch))
        loaded.append(imgs)
    height = sum(max(i[2] for i in row) for row in loaded)
    canvas = np.ones((height, width, 4), np.float32) * 0.5
    canvas[..., 3] = 1.0
    top = height
    for row in loaded:
        rh = max(i[2] for i in row)
        x = 0
        for px, cw, ch in row:
            canvas[top - ch:top, x:x + cw] = px
            x += cw
        top -= rh
    out = bpy.data.images.new('_Sheet', width, height)
    out.pixels = canvas.ravel()
    out.filepath_raw = os.path.join(OUT, 'Sheet.png')
    out.file_format = 'PNG'
    out.save()
    print(f'LOOTER: wrote {out.filepath_raw}', flush=True)


if args and args[0] == '--check':
    check(args[1:] if len(args) > 1 else m47.names())
else:
    sheet()
