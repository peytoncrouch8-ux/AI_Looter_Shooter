"""The Venom Spitter from Tripo: its texture set (Art/Textures/TripoSpitter, MI_SpitterTripoBody on the World master) and
its meshes as Blender objects. Not a model (Tools\\models.ps1 skips parts_*.py); Spitter.py runs it.

One 2048 atlas (REGIONS). What Tripo painted (the carapace and face, the leg, palp and fang) is baked from its 4K map
onto our parts' templates (Cycles, selected to active; the body from Tripo's shell without its legs, so no leg paint
lands on the carapace); parts_spitter_paint.py then gives it our look and paints what Tripo only painted on the sac
(veins, collar) and what is new (the venom core, the eyes, the waist).
A bake is cached: it is redone only when the .glb or these scripts are newer than the set's maps.
"""
import math
import os

import bmesh
import bpy
import numpy as np

import looter_rebake as rb
import looter_textures as lt
import parts_spitter as ps

SET = 'TripoSpitter'
MATERIAL = 'SpitterTripoBody'
SIZE = 2048
# Atlas regions (u0, u1, v0, v1), UV space (v up).
REGIONS = {
    'body': (0.0, 0.75, 0.5, 1.0), 'femur': (0.0, 0.5, 0.25, 0.5), 'tibia': (0.0, 0.75, 0.0, 0.25),
    'knob': (0.5, 0.625, 0.25, 0.5), 'palp': (0.625, 1.0, 0.25, 0.375), 'fang': (0.625, 1.0, 0.375, 0.5),
    'core': (0.75, 1.0, 0.5, 2.0 / 3.0), 'gel': (0.75, 1.0, 2.0 / 3.0, 5.0 / 6.0), 'eye': (0.75, 0.875, 0.0, 0.125),
    'vein': (0.875, 1.0, 0.0, 0.125), 'collar': (0.75, 1.0, 0.125, 0.25),
}
BAKED = ('body', 'femur', 'tibia', 'knob', 'palp', 'fang')


# --- Blender objects from parts ---

def to_object(part, which, material):
    """part as a Blender object (meters, Blender axes): which 'V' (final, creature cm) or 'T' (template, Tripo units),
    only the vertices its faces use, its UVs in its atlas region."""
    pts = part.V if which == 'V' else part.T
    used = sorted({i for f in part.faces for i in f})
    remap = {old: new for new, old in enumerate(used)}
    if which == 'V':
        co = [(p[1] * 0.01, -p[0] * 0.01, p[2] * 0.01) for p in (pts[i] for i in used)]   # creature cm -> Blender m
    else:
        co = [tuple(pts[i]) for i in used]
    me = bpy.data.meshes.new(part.name)
    me.from_pydata(co, [], [[remap[i] for i in f] for f in part.faces])
    u0, u1, v0, v1 = REGIONS[part.region]
    layer = me.uv_layers.new(name='UVMap')
    flat = [(u0 + u * (u1 - u0), v0 + v * (v1 - v0)) for uvs in part.uv for u, v in uvs]
    layer.data.foreach_set('uv', [c for uv in flat for c in uv])
    me.materials.append(material)
    for p in me.polygons:
        p.use_smooth = True
    me.validate()
    obj = bpy.data.objects.new(part.name if which == 'V' else '_T' + part.name, me)
    bpy.context.scene.collection.objects.link(obj)
    return obj


def join(objects, name):
    bpy.ops.object.select_all(action='DESELECT')
    for o in objects:
        o.select_set(True)
    bpy.context.view_layer.objects.active = objects[0]
    bpy.ops.object.join()
    obj = bpy.context.view_layer.objects.active
    obj.name = obj.data.name = name
    return obj


# --- The bake ---

def _source(obj, image, keep_faces=None, name='_TripoBake'):
    """A copy of Tripo's shell emitting its colour map (only keep_faces when given)."""
    src = obj.copy()
    src.data = obj.data.copy()
    src.name = src.data.name = name
    bpy.context.scene.collection.objects.link(src)
    if keep_faces is not None:
        bm = bmesh.new()
        bm.from_mesh(src.data)
        bm.faces.ensure_lookup_table()
        bmesh.ops.delete(bm, geom=[f for f in bm.faces if not keep_faces[f.index]], context='FACES')
        bm.to_mesh(src.data)
        bm.free()
    mat = bpy.data.materials.new(name + 'Mat')
    mat.use_nodes = True
    nodes, links = mat.node_tree.nodes, mat.node_tree.links
    nodes.clear()
    out = nodes.new('ShaderNodeOutputMaterial')
    em = nodes.new('ShaderNodeEmission')
    tex = nodes.new('ShaderNodeTexImage')
    tex.image = image
    links.new(tex.outputs['Color'], em.inputs['Color'])
    links.new(em.outputs['Emission'], out.inputs['Surface'])
    src.data.materials.clear()
    src.data.materials.append(mat)
    return src


def _bake_into(target, source, img, cage, reach):
    sc = bpy.context.scene
    sc.render.engine = 'CYCLES'
    sc.cycles.device = 'CPU'
    sc.cycles.samples = 4
    mat = bpy.data.materials.new('_bake_target')
    mat.use_nodes = True
    node = mat.node_tree.nodes.new('ShaderNodeTexImage')
    node.image = img
    mat.node_tree.nodes.active = node
    target.data.materials.clear()
    target.data.materials.append(mat)
    bpy.ops.object.select_all(action='DESELECT')
    source.select_set(True)
    target.select_set(True)
    bpy.context.view_layer.objects.active = target
    bpy.ops.object.bake(type='EMIT', use_selected_to_active=True, cage_extrusion=cage, max_ray_distance=reach,
                        margin=16, margin_type='EXTEND', use_clear=False, target='IMAGE_TEXTURES')


def bake(m, templates):
    """Tripo's colour on the templates' atlas texels: (linear RGB (S, S, 3) rows top first, covered mask). templates:
    {region: [template objects]}; the body's from Tripo without legs, palps, fangs and sac."""
    obj, keep = m['obj'], m['keep']
    keep_faces = np.array([all(keep[v] for v in p.vertices) for p in obj.data.polygons])
    img = rb._image('_SpitterBake', SIZE)
    hit = rb._image('_SpitterHit', SIZE)
    rb._fill(img, (0.0, 0.0, 0.0, 0.0))
    rb._fill(hit, (0.0, 0.0, 0.0, 0.0))
    white = bpy.data.images.new('_white', 4, 4)
    white.pixels.foreach_set([1.0] * 64)
    for group, faces in (('body', keep_faces), ('limbs', None)):
        objs = [o for region, os_ in templates.items() for o in os_ if (region == 'body') == (group == 'body')]
        if not objs:
            continue
        target = join(objs, f'_BakeTarget_{group}')
        for image, out in ((m['maps']['BC'], img), (white, hit)):
            src = _source(obj, image, faces)
            _bake_into(target, src, out, cage=0.6, reach=1.4)
            bpy.data.objects.remove(src)
        bpy.data.objects.remove(target)
    colour = rb._read(img)[..., :3]
    covered = rb._read(hit)[..., 0] > 0.5
    lt._log(f'Spitter Tripo: baked {covered.mean() * 100:.1f}% of the atlas from Tripo\'s map')
    return colour, covered


# --- Painting ---

def box(region, size=SIZE):
    """A region's pixel box: (row0, row1, col0, col1), rows top first."""
    u0, u1, v0, v1 = REGIONS[region]
    return int(round((1.0 - v1) * size)), int(round((1.0 - v0) * size)), int(round(u0 * size)), int(round(u1 * size))


def lin(hex_value):
    return np.array(lt.hex_color(hex_value)[:3], np.float32)


def grads(region):
    """A region's (U, V) in 0..1 per pixel (U along, V round/up), rows top first."""
    r0, r1, c0, c1 = box(region)
    u = (np.arange(c1 - c0) + 0.5) / (c1 - c0)
    v = 1.0 - (np.arange(r1 - r0) + 0.5) / (r1 - r0)
    return np.meshgrid(u, v)


def texture(m, pieces, sources):
    """The set's maps (baked, then painted in our look by parts_spitter_paint.py, unless cached) and its material.
    pieces: {bone: [Part]}."""
    import parts_spitter_paint as paint
    if rb.cached(SET, sources):
        lt._log(f'{SET}: maps up to date, bake skipped')
        return lt.material(SET, name=MATERIAL, master='World')
    scratch = bpy.data.materials.new('_SpitterScratch')
    templates = {}
    for plist in pieces.values():
        for p in plist:
            if p.region in BAKED and p.T and p.T[0] is not None:
                templates.setdefault(p.region, []).append(to_object(p, 'T', scratch))
    colour, covered = bake(m, templates)
    bc, orm = paint.style(rb._dilate(colour, covered, steps=16), covered)
    paint.paint_built(bc, orm)
    normal = rb.normals({'BC': bc}, relief=0.8)
    return rb.write({'BC': bc, 'ORM': orm}, normal, SET, MATERIAL)
