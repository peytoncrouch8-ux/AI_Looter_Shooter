"""Preview renders of an area (Eevee) into a folder (Saved/ArtPreviews/Terrain/<Area> by default): the layout's views
(layout.json preview.views) and plan.png (top down, annotated with the zones, preview.labels and the computed points).
Clay blocks stand in for the buildings, for scale. Everything added is removed afterwards.

A view is an eye and a target, each [X, Y, height above the ground] in cm (an eye outside the map square takes its
height as absolute), and a lens in mm.
"""
import math
import os

import bpy
import numpy as np
from mathutils import Vector

import area_computed

STAND_INS = {  # kind: length along the actor's forward, width, height (m)
    'Farmhouse': (9.0, 11.0, 6.5), 'Barn': (12.0, 9.0, 8.0), 'Well': (1.8, 1.8, 1.4), 'LogCabin': (6.5, 8.0, 5.0),
    'Cottage': (6.5, 8.0, 5.5), 'Outhouse': (1.3, 1.3, 2.4), 'GunRack': (0.5, 2.0, 1.6), 'Windmill': (2.5, 2.5, 11.0),
    'LookoutTower': (4.0, 4.0, 10.0), 'TargetDummy': (0.5, 0.6, 1.8),
}


def _to_b(x, y, z):
    return Vector((-y, -x, z))


def _material(name, color, emission=0.0):
    mat = bpy.data.materials.get(name) or bpy.data.materials.new(name)
    mat.use_nodes = True
    bsdf = next(n for n in mat.node_tree.nodes if n.type == 'BSDF_PRINCIPLED')
    bsdf.inputs['Base Color'].default_value = color
    bsdf.inputs['Roughness'].default_value = 0.8
    if emission:
        bsdf.inputs['Emission Color'].default_value = color
        bsdf.inputs['Emission Strength'].default_value = emission
    return mat


def _world(scene):
    world = bpy.data.worlds.new('_PreviewSky')
    world.use_nodes = True
    nodes, links = world.node_tree.nodes, world.node_tree.links
    nodes.clear()
    out = nodes.new('ShaderNodeOutputWorld')
    bg = nodes.new('ShaderNodeBackground')
    coords = nodes.new('ShaderNodeTexCoord')
    split = nodes.new('ShaderNodeSeparateXYZ')
    ramp = nodes.new('ShaderNodeValToRGB')
    norm = nodes.new('ShaderNodeVectorMath')
    norm.operation = 'NORMALIZE'
    links.new(coords.outputs['Generated'], norm.inputs[0])
    links.new(norm.outputs['Vector'], split.inputs['Vector'])
    links.new(split.outputs['Z'], ramp.inputs['Fac'])
    ramp.color_ramp.interpolation = 'EASE'
    e = ramp.color_ramp.elements
    e[0].position, e[0].color = 0.0, (0.26, 0.29, 0.33, 1.0)
    e[1].position, e[1].color = 0.62, (0.16, 0.30, 0.62, 1.0)
    mid = e.new(0.03)
    mid.color = (0.62, 0.66, 0.66, 1.0)
    links.new(ramp.outputs['Color'], bg.inputs['Color'])
    bg.inputs['Strength'].default_value = 1.0
    links.new(bg.outputs['Background'], out.inputs['Surface'])
    world.mist_settings.start = 110.0
    world.mist_settings.depth = 700.0
    world.mist_settings.falloff = 'QUADRATIC'
    scene.world = world
    return world


def _haze(scene, on):
    """Aerial haze from the mist pass, mixed toward a pale sky color in the compositor."""
    scene.view_layers[0].use_pass_mist = on
    scene.use_nodes = on
    if not on:
        return
    tree = scene.node_tree
    tree.nodes.clear()
    layers = tree.nodes.new('CompositorNodeRLayers')
    mix = tree.nodes.new('CompositorNodeMixRGB')
    mix.inputs[2].default_value = (0.66, 0.72, 0.78, 1.0)
    scale = tree.nodes.new('CompositorNodeMath')
    scale.operation = 'MULTIPLY'
    scale.inputs[1].default_value = 0.4
    tree.links.new(layers.outputs['Mist'], scale.inputs[0])
    tree.links.new(scale.outputs[0], mix.inputs[0])
    tree.links.new(layers.outputs['Image'], mix.inputs[1])
    comp = tree.nodes.new('CompositorNodeComposite')
    tree.links.new(mix.outputs[0], comp.inputs['Image'])


def _sun(forward, added, strength=4.2, side=0.85, ahead=0.3, elevation=36.0):
    """A warm sun from the upper left of a view looking along forward (Blender axes)."""
    flat = Vector((forward.x, forward.y, 0.0))
    flat = flat.normalized() if flat.length > 1e-6 else Vector((0.0, 1.0, 0.0))
    left = Vector((0.0, 0.0, 1.0)).cross(flat)
    toward = (flat * ahead + left * side).normalized()
    up = math.radians(elevation)
    toward = (toward * math.cos(up) + Vector((0.0, 0.0, math.sin(up)))).normalized()
    sun = bpy.data.objects.new('_Sun', bpy.data.lights.new('_Sun', 'SUN'))
    sun.data.energy = strength
    sun.data.color = (1.0, 0.9, 0.76)
    sun.data.angle = math.radians(1.5)
    sun.rotation_euler = (-toward).to_track_quat('-Z', 'Y').to_euler()
    bpy.context.scene.collection.objects.link(sun)
    added.append(sun)
    return sun


def _stand_ins(area, added):
    clay = _material('_Clay', (0.55, 0.52, 0.47, 1.0))
    for p in area.layout['placements']:
        if p['kind'] not in STAND_INS:
            continue
        length, width, height = STAND_INS[p['kind']]
        x, y = np.asarray(p['location']) / 100.0
        z = float(area.height(x, y))
        mesh = bpy.data.meshes.new('_Stand')
        hx, hy = width * 0.5, length * 0.5
        mesh.from_pydata([(-hx, -hy, 0), (hx, -hy, 0), (hx, hy, 0), (-hx, hy, 0),
                          (-hx, -hy, height), (hx, -hy, height), (hx, hy, height), (-hx, hy, height)], [],
                         [(0, 3, 2, 1), (4, 5, 6, 7), (0, 1, 5, 4), (1, 2, 6, 5), (2, 3, 7, 6), (3, 0, 4, 7)])
        mesh.materials.append(clay)
        obj = bpy.data.objects.new('_Stand_' + p['id'], mesh)
        obj.location = _to_b(x, y, z - 0.3)
        obj.rotation_euler = (0.0, 0.0, -math.radians(p.get('yaw', 0.0)))
        bpy.context.scene.collection.objects.link(obj)
        added.append(obj)


def _camera(eye, target, lens, added, ortho=None):
    cam = bpy.data.objects.new('_Camera', bpy.data.cameras.new('_Camera'))
    cam.data.lens = lens
    cam.data.clip_start = 0.2
    cam.data.clip_end = 2000.0
    cam.location = eye
    if ortho is not None:
        cam.data.type = 'ORTHO'
        cam.data.ortho_scale = ortho
        cam.rotation_euler = (0.0, 0.0, math.pi)  # looking down, north (-Y) up, east (-X) right
    else:
        cam.rotation_euler = (target - eye).to_track_quat('-Z', 'Y').to_euler()
    bpy.context.scene.collection.objects.link(cam)
    added.append(cam)
    bpy.context.scene.camera = cam
    return cam


def _render(path, size, samples=24):
    scene = bpy.context.scene
    scene.render.engine = 'BLENDER_EEVEE_NEXT'
    scene.eevee.taa_render_samples = samples
    scene.render.resolution_x, scene.render.resolution_y = size
    scene.render.resolution_percentage = 100
    scene.render.image_settings.file_format = 'PNG'
    scene.render.image_settings.color_mode = 'RGB'
    scene.view_settings.view_transform = 'AgX'
    scene.view_settings.look = 'AgX - Medium High Contrast'
    scene.render.filepath = path
    bpy.ops.render.render(write_still=True)


def _cleanup(added):
    for obj in added:
        data = obj.data
        bpy.data.objects.remove(obj)
        if data is None or data.users:
            continue
        for store in (bpy.data.meshes, bpy.data.lights, bpy.data.cameras, bpy.data.curves):
            if data.name in store and store[data.name] == data:
                store.remove(data)
                break


def render_views(area, out_dir, log=print, only=None):
    scene = bpy.context.scene
    world = _world(scene)
    _haze(scene, True)
    for name, view in area.layout.get('preview', {}).get('views', {}).items():
        if only and name not in only:
            continue
        (ex, ey, eh), (tx, ty, th) = (np.asarray(view[k], dtype=np.float64) / 100.0 for k in ('eye', 'target'))
        added = []
        _stand_ins(area, added)
        inside = abs(ex) < area.half and abs(ey) < area.half
        eye = _to_b(ex, ey, float(area.height(ex, ey)) + eh if inside else eh)
        target = _to_b(tx, ty, float(area.height(tx, ty)) + th)
        _camera(eye, target, view['lens'], added)
        _sun((target - eye).normalized(), added)
        path = os.path.join(out_dir, name + '.png')
        _render(path, (1600, 900))
        _cleanup(added)
        log(f'terrain: preview {path}')
    _haze(scene, False)
    bpy.data.worlds.remove(world)


def _text(body, x, y, size, color, added, z=40.0):
    curve = bpy.data.curves.new('_Label', 'FONT')
    curve.body = body
    curve.size = size
    curve.align_x = 'CENTER'
    curve.align_y = 'CENTER'
    obj = bpy.data.objects.new('_Label', curve)
    obj.visible_shadow = False  # annotations float above the terrain: no shadows on it
    obj.location = _to_b(x, y, z)
    obj.rotation_euler = (0.0, 0.0, math.pi)
    obj.data.materials.append(_material('_Label' + str(color), color, emission=3.0))
    bpy.context.scene.collection.objects.link(obj)
    added.append(obj)
    return obj


def _line(points, width, color, added, z=30.0, closed=False):
    curve = bpy.data.curves.new('_Line', 'CURVE')
    curve.dimensions = '3D'
    curve.bevel_depth = width
    spline = curve.splines.new('POLY')
    spline.points.add(len(points) - 1)
    for p, (x, y) in zip(spline.points, points):
        v = _to_b(x, y, z)
        p.co = (v.x, v.y, v.z, 1.0)
    spline.use_cyclic_u = closed
    obj = bpy.data.objects.new('_Line', curve)
    obj.visible_shadow = False
    obj.data.materials.append(_material('_Line' + str(color), color, emission=2.0))
    bpy.context.scene.collection.objects.link(obj)
    added.append(obj)


def render_plan(area, out_dir, log=print):
    """Top down over the map square: the area in its macro colors, zones outlined, labels, and the computed points
    (cliff dressing, hanging rim cliffs, the bridge, the waterfall)."""
    scene = bpy.context.scene
    world = _world(scene)
    added = []
    _stand_ins(area, added)
    _camera(Vector((0.0, 0.0, 300.0)), None, 50.0, added, ortho=2.0 * area.half + 1.2)
    _sun(Vector((0.0, -1.0, -0.3)), added, strength=3.6, side=0.8, ahead=-0.6, elevation=40.0)
    white, dark = (1.0, 1.0, 1.0, 1.0), (0.02, 0.02, 0.03, 1.0)
    zone_color = (1.0, 0.45, 0.9, 1.0)
    for zone in area.layout['zones']:
        pts = np.asarray(zone['polygon']) / 100.0
        _line([tuple(p) for p in pts], 0.12, zone_color, added, closed=True)
    for text, (x, y) in area.layout.get('preview', {}).get('labels', {}).items():
        x, y = x / 100.0, y / 100.0
        _text(text, x - 0.5, y + 0.5, 3.4, dark, added, z=39.0)
        _text(text, x, y, 3.4, white, added)
    data = area_computed.compute(area)
    orange, cyan, red = (1.0, 0.5, 0.1, 1.0), (0.2, 0.9, 1.0, 1.0), (1.0, 0.1, 0.1, 1.0)
    for points in data['cliffs'].values():
        for point in points:
            x, y = point['location'][0] / 100.0, point['location'][1] / 100.0
            a = math.radians(point['yaw'])
            # Hanging cliffs (the rim, with its drop below) in cyan, standing ones in orange.
            hanging = 'drop' in point
            length = 2.5 if hanging else 3.0
            _line([(x, y), (x + length * math.cos(a), y + length * math.sin(a))], 0.25 if hanging else 0.35,
                  cyan if hanging else orange, added)
    b = data['bridge']
    if b:
        bx, by = b['location'][0] / 100.0, b['location'][1] / 100.0
        a = math.radians(b['yaw'])
        half = b['span'] / 200.0
        _line([(bx - half * math.cos(a), by - half * math.sin(a)), (bx + half * math.cos(a), by + half * math.sin(a))],
              0.9, red, added)
        _text('Bridge', bx + 4.0, by + 8.0, 2.6, white, added)
    w = data['waterfall']
    if w:
        _text('Waterfall', w['location'][0] / 100.0 - 4.0, w['location'][1] / 100.0 - 6.0, 2.6, white, added)
    path = os.path.join(out_dir, 'plan.png')
    _render(path, (2048, 2048), samples=16)
    _cleanup(added)
    bpy.data.worlds.remove(world)
    log(f'terrain: preview {path}')


def render_all(area, out_dir, log=print):
    os.makedirs(out_dir, exist_ok=True)
    render_plan(area, out_dir, log)
    render_views(area, out_dir, log)
