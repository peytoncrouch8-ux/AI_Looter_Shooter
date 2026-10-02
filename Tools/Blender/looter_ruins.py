"""Helpers for the Ransom's Rest ruins family: Art/Models/Props/Ruins.py (ruins, broken walls, burnt wagons, cairns),
Art/Models/Buildings/Lookout.py (the lookout, its cliff stairs, the mooring post) and Art/Models/Props/BurialDeck.py
(the burial boards deck, the biers, the keeper's lanterns). See Art/README.md for the rules every model follows.

A model is a looter_buildings Model (kit.Model: boards, boxes, walls with openings, roofs, hulls, sockets, baked AO)
with three additions:
  - Model.slot() knows the shared material names of the Ransom's Rest brief by key ('charcoal', 'granite',
    'ironblack', 'brass', 'crepe', ... besides the kit's 'trim', 'planks', 'stone', 'metal', 'endgrain', 'glow'),
    and keys that resolve to one material share one slot;
  - Model.add(obj) merges a part built with looter_props (lp.block, lp.sweep, lp.lathe...) and mapped with its tools
    (lp.grain, lp.lathe_uv, lt.box_uv...), keeping its UVs and materials;
  - Model.finish() takes the AO distance and where the preview goes (props preview to
    Saved/ArtPreviews/RansomsRest/Ruins/).

UV specs for kit parts: PlankRow puts a board on one plank of WoodPlanks (grain along it, clear of the texture's
butt joints), Grain runs a tileable set's V along a part (charred timber on the bark texture). Builders for stones,
ropes with twisted strands and frayed ends, spoked wheels and iron lanterns live here too, and fences_wall() reads
Fences.py's own dry-stone wall code, so the broken, fallen and corner wall pieces match SM_StoneWall exactly.

Everything random comes from seeded random.Random, so every run gives the same meshes.
"""
import ast
import math
import os
import random
import types

import bmesh
import bpy
from mathutils import Euler, Matrix, Vector, noise

import looter_buildings as kit
import looter_model as lm
import looter_props as lp
import looter_textures as lt

REPO = lt.REPO
PREVIEW_DIR = os.path.join(REPO, 'Saved', 'ArtPreviews', 'RansomsRest', 'Ruins')
X_AXIS = Vector((1.0, 0.0, 0.0))
UP = Vector((0.0, 0.0, 1.0))

# --- Materials: the brief's shared names (Unreal material instances, MI_<name>) ---

_MAKERS = {
    'trim': lambda: lt.material('HouseTrim'),
    'planks': lambda: lt.material('WoodPlanks'),
    'endgrain': lambda: lt.material('WoodEndGrain'),
    'metal': lambda: lt.material('MetalRust'),
    'stone': lambda: lt.material('StoneWall'),
    'charcoal': lambda: lt.material('BarkOak', name='Charcoal', tint=0x4a4440),
    'granite': lambda: lt.material('RockGranite', MossAmount=0.6),
    'cliff': lambda: lt.material('RockCliff', MossAmount=0.6),
    'ironblack': lambda: lt.material('MetalWorn', name='IronBlack', tint=0x2e2c2a),
    'brass': lambda: lt.material('MetalWorn', name='BrassWorn', tint=0xc49c56),
    'crepe': lambda: lt.material('Polymer', name='MourningCrepe', tint=0x161518),
    'cream': lambda: lt.material('PaintWorn', name='PaintCream', tint=0xe6dac2),
    'glow': lambda: kit._material('glow'),
}
_NAMES = {'trim': 'HouseTrim', 'planks': 'WoodPlanks', 'endgrain': 'WoodEndGrain', 'metal': 'MetalRust',
          'stone': 'StoneWall', 'charcoal': 'Charcoal', 'granite': 'RockGranite', 'cliff': 'RockCliff',
          'ironblack': 'IronBlack', 'brass': 'BrassWorn', 'crepe': 'MourningCrepe', 'cream': 'PaintCream',
          'glow': 'LanternGlow'}


def material(key):
    """The shared material for a key (made on first use in this Blender session)."""
    name = _NAMES[key]
    mat = bpy.data.materials.get(name)
    if mat is None or (key != 'glow' and mat.get('Master') is None):
        mat = _MAKERS[key]()
    return mat


def tile_scale(set_name):
    """UV units per metre of a tileable set at its texel density."""
    info = lt.SETS[set_name]
    return info['density'] / float(info['size'])


# --- The model ---

class Model(kit.Model):
    """kit.Model with the brief's materials by key, lp parts merged with add(), and finish() options (see the
    module's docstring)."""

    def slot(self, key):
        if key not in self.slots:
            mat = material(key) if key in _MAKERS else bpy.data.materials[key]
            self.slots[key] = (self._index_of(mat), mat)
        return self.slots[key][0]

    def _index_of(self, mat):
        for index, existing in self.slots.values():
            if existing.name == mat.name:
                return index
        return len({index for index, _ in self.slots.values()})

    def add(self, obj, smooth=None, ao_floor=0.0):
        """Merges an lp-built part (an object whose mesh is already in model space, mapped and with its materials)
        into the model, then deletes it. smooth overrides its faces' shading."""
        mesh = obj.data
        src = bmesh.new()
        src.from_mesh(mesh)
        src.transform(obj.matrix_world)
        uv_src = src.loops.layers.uv.active
        slots = []
        for mat in mesh.materials:
            key = next((k for k, n in _NAMES.items() if n == mat.name), mat.name)
            slots.append(self.slot(key))
        verts = {v: self.bm.verts.new(v.co) for v in src.verts}
        for face in src.faces:
            try:
                new = self.bm.faces.new([verts[loop.vert] for loop in face.loops])
            except ValueError:
                continue
            new.material_index = slots[face.material_index] if slots else self.slot('trim')
            new.smooth = face.smooth if smooth is None else smooth
            new[self.floor] = ao_floor
            for loop_new, loop_src in zip(new.loops, face.loops):
                loop_new[self.uv].uv = loop_src[uv_src].uv if uv_src is not None else (0.0, 0.0)
        src.free()
        bpy.data.objects.remove(obj)
        if mesh.users == 0:
            bpy.data.meshes.remove(mesh)

    def finish(self, ao=True, ground=True, preview=True, view=(-1.0, -1.6, 0.55), fit=0.8, fallback=100.0,
               ao_distance=1.0, out=None, lens=40.0, **props):
        """Makes the model object with its hulls and sockets, bakes its AO (rays reach ao_distance metres) and renders
        its preview with --preview (to out, else Saved/ArtPreviews/Buildings/<name>.png). Returns the object."""
        bpy.data.objects.remove(self._obj)
        bpy.data.meshes.remove(self._mesh)
        mesh = bpy.data.meshes.new(self.name)
        kit._triangulate(self.bm)
        self.bm.normal_update()
        self.bm.to_mesh(mesh)
        self.bm.free()
        by_index = {index: mat for index, mat in self.slots.values()}
        for index in sorted(by_index):
            mesh.materials.append(by_index[index])
        obj = bpy.data.objects.new(self.name, mesh)
        bpy.context.scene.collection.objects.link(obj)
        if fallback is not None and props.get('Nanite', 1):
            obj['Fallback'] = float(fallback)
        for key, value in props.items():
            obj[key] = value
        triangles = sum(len(p.vertices) - 2 for p in mesh.polygons)
        print(f"RUINS: {self.name}: {triangles} triangles, {len(self.hulls)} hulls, {len(self.sockets)} sockets, "
              f"materials {', '.join(m.name for m in mesh.materials)}", flush=True)
        if '--stats' in kit._args():
            kit.degenerate_report(obj)
        if ao and '--no-ao' not in kit._args():
            lt.bake_vertex_ao(obj, ground=ground, distance=ao_distance)
            kit._raise_ao(mesh)
        mesh.attributes.remove(mesh.attributes['_aofloor'])
        for points in self.hulls:
            hb = bmesh.new()
            verts = [hb.verts.new(p) for p in points]
            result = bmesh.ops.convex_hull(hb, input=verts)
            inside = [v for v in result['geom_interior'] + result['geom_unused'] if isinstance(v, bmesh.types.BMVert)]
            bmesh.ops.delete(hb, geom=list(set(inside)), context='VERTS')
            hull_mesh = bpy.data.meshes.new('UCX_' + self.name)
            hb.to_mesh(hull_mesh)
            hb.free()
            hull = bpy.data.objects.new('UCX_' + self.name, hull_mesh)
            bpy.context.scene.collection.objects.link(hull)
            hull.parent = obj
            hull.display_type = 'WIRE'
        for name, location, rotation in self.sockets:
            lm.socket(obj, name, location, rotation)
        if preview and lt.want_preview():
            lt.preview([obj], out or lt.preview_path('Buildings', self.name), view=view, fit=fit, lens=lens)
        return obj


def darken(obj, weight, strength=0.6):
    """Lowers the baked occlusion (vertex color alpha) where weight(world position, normal) > 0, by strength x weight:
    stains and soot. M_World darkens the base color with some of it (DiffuseAO) and the ambient light with all of it."""
    import numpy as np
    mesh = obj.data
    col = mesh.color_attributes.get('Col')
    if col is None:          # not baked (--no-ao)
        return
    raw = np.empty(4 * len(mesh.loops), dtype=np.float32)
    col.data.foreach_get('color', raw)
    raw = raw.reshape(-1, 4)
    matrix = obj.matrix_world
    for poly in mesh.polygons:
        n = (matrix.to_3x3() @ poly.normal).normalized()
        for li in poly.loop_indices:
            p = matrix @ mesh.vertices[mesh.loops[li].vertex_index].co
            w = weight(p, n)
            if w > 0.0:
                raw[li, 3] *= 1.0 - strength * min(w, 1.0)
    col.data.foreach_set('color', raw.ravel())
    mesh.update()


def soot_strip(obj, strip, strength=0.5, where=None):
    """Lowers the baked occlusion on the faces mapped onto one strip of the house trim sheet (where(position) narrows
    it): soot on fire-blackened roof tin, grime on iron."""
    import numpy as np
    mesh = obj.data
    col = mesh.color_attributes.get('Col')
    if col is None:
        return
    slot = mesh.materials.find('HouseTrim')
    v0, v1 = lt.TRIM_STRIPS[lt.TRIM_ALIASES.get(strip, strip)]
    uv = mesh.uv_layers.active.data
    raw = np.empty(4 * len(mesh.loops), dtype=np.float32)
    col.data.foreach_get('color', raw)
    raw = raw.reshape(-1, 4)
    for poly in mesh.polygons:
        if poly.material_index != slot:
            continue
        vs = [uv[li].uv.y for li in poly.loop_indices]
        if not all(v0 - 1e-4 <= v <= v1 + 1e-4 for v in vs):
            continue
        if where is not None and not where(obj.matrix_world @ poly.center):
            continue
        for li in poly.loop_indices:
            raw[li, 3] *= 1.0 - strength
    col.data.foreach_set('color', raw.ravel())
    mesh.update()


# --- UV specs for kit parts (applied in the part's own frame: length along X) ---

class PlankRow:
    """A board on one plank of a planks set (WoodPlanks: 16 planks of 20 cm along U): the grain runs along the board
    (its X), its broad faces show that one plank, and it sits on a stretch clear of the texture's butt joints when one
    is long enough (a long board shows a joint, like a real one)."""

    def __init__(self, row=None, u=None, set_name='WoodPlanks'):
        self.row, self.u, self.set_name = row, u, set_name

    def apply(self, obj, rng):
        info = lt.SETS[self.set_name]
        scale = tile_scale(self.set_name)
        repeat = info['size'] / float(info['density'])
        lane = 0.2
        rows = int(round(repeat / lane))
        row = self.row if self.row is not None else rng.randrange(rows)
        mesh = obj.data
        xs = [v.co.x for v in mesh.vertices]
        x0, length = min(xs), max(xs) - min(xs)
        u0 = self.u
        if u0 is None:
            joints = sorted(lt.PLANK_JOINTS[row]) if self.set_name == 'WoodPlanks' else []
            if joints:
                stretches = [(a, b if b > a else b + repeat) for a, b in zip(joints, joints[1:] + joints[:1])]
                fitting = [s for s in stretches if s[1] - s[0] >= length + 0.12]
                if fitting:
                    a, b = rng.choice(fitting)
                    u0 = a + 0.06 + rng.uniform(0.0, b - a - length - 0.12)
                else:
                    a, b = max(stretches, key=lambda s: s[1] - s[0])
                    u0 = (a + b) * 0.5 - length * 0.5
            else:
                u0 = rng.uniform(0.0, repeat)
        uv = (mesh.uv_layers.active or mesh.uv_layers.new(name='UVMap')).data
        for p in mesh.polygons:
            n = p.normal
            cos = [mesh.vertices[mesh.loops[li].vertex_index].co for li in p.loop_indices]
            end = abs(n.x) > 0.7
            a_axis = Vector((0.0, 1.0, 0.0)) if end else n.cross(X_AXIS).normalized()
            ts = [c.dot(a_axis) for c in cos]
            t_min, width = min(ts), max(max(ts) - min(ts), 1e-6)
            span = min(width, lane - 0.02)
            v0 = row * lane + 0.01 + (lane - 0.02 - span) * 0.5
            for li, c, t in zip(p.loop_indices, cos, ts):
                s = (c.z if end else c.x - x0) + u0
                uv[li].uv = (s * scale, (v0 + (t - t_min) / width * span) * scale)


class Grain:
    """A tileable set with its V along the part (X): bark furrows, here the alligatored char of burnt timber."""

    def __init__(self, set_name='BarkOak'):
        self.set_name = set_name

    def apply(self, obj, rng):
        scale = tile_scale(self.set_name)
        info = lt.SETS[self.set_name]
        repeat = info['size'] / float(info['density'])
        off = (rng.uniform(0.0, repeat), rng.uniform(0.0, repeat))
        mesh = obj.data
        uv = (mesh.uv_layers.active or mesh.uv_layers.new(name='UVMap')).data
        for p in mesh.polygons:
            n = p.normal
            end = abs(n.x) > 0.7
            a_axis = Vector((0.0, 1.0, 0.0)) if end else n.cross(X_AXIS).normalized()
            for li in p.loop_indices:
                c = mesh.vertices[mesh.loops[li].vertex_index].co
                u, v = (c.dot(a_axis), c.z) if end else (c.dot(a_axis), c.x)
                uv[li].uv = ((u + off[0]) * scale, (v + off[1]) * scale)


# --- Shapes (lp parts: objects in model space, mapped, ready for Model.add) ---

def stone_block(size, center, rotation=(0.0, 0.0, 0.0), seed=0, rough=0.02, bevel=0.03, key='trim', strip='Stone',
                set_name=None):
    """A rough block of stone (fallen masonry, rubble, a hearth slab): a chamfered box pushed about by noise, on the
    trim's fieldstone (strip) or a tileable set (set_name)."""
    part = lp.block(size, (0.0, 0.0, 0.0), bevel=min(bevel, min(size) * 0.3))
    lp.rough(part, rough, 5.0, seed)
    lp.place(part, center, rotation)
    if set_name:
        lt.assign(part, material(key))
        lt.box_uv(part, set_name, seed=seed)
    else:
        lp.trim(part, strip, seed=seed, material=material(key))
    return part


def round_stone(size, center, rotation=(0.0, 0.0, 0.0), seed=0, tris=72, key='granite', set_name='RockGranite',
                flat=0.3, squash=1.0):
    """A weathered rounded stone (a cairn stone, a field stone): a lumpy rounded box, flat underneath (flat: how much
    of its height is cut off at the bottom), reduced to about tris triangles."""
    rnd = random.Random(seed)
    bm = lp.new_bmesh()
    bmesh.ops.create_icosphere(bm, subdivisions=2, radius=1.0)
    offset = Vector((rnd.uniform(0, 50), rnd.uniform(0, 50), rnd.uniform(0, 50)))
    sx, sy, sz = size
    for v in bm.verts:
        d = v.co.normalized()
        r = (abs(d.x) ** 2.6 + abs(d.y) ** 2.6 + abs(d.z) ** 2.6) ** (-1.0 / 2.6)
        r *= 1.0 + 0.1 * noise.noise(d * 1.4 + offset) + 0.03 * noise.noise(d * 4.0 + offset)
        z = d.z * r * sz * 0.5
        floor = -sz * 0.5 * (1.0 - flat)
        v.co = Vector((d.x * r * sx * 0.5, d.y * r * sy * 0.5, max(z, floor) * squash))
    part = lp.mesh_object(bm)
    if tris and len(part.data.polygons) > tris:
        mod = part.modifiers.new('Reduce', 'DECIMATE')
        mod.ratio = tris / float(len(part.data.polygons))
        mod.use_collapse_triangulate = True
        with bpy.context.temp_override(object=part, active_object=part, selected_objects=[part],
                                       selected_editable_objects=[part]):
            bpy.ops.object.modifier_apply(modifier=mod.name)
    # The pivot of the stone: the middle of its bottom.
    lowest = min(v.co.z for v in part.data.vertices)
    for v in part.data.vertices:
        v.co.z -= lowest
    lp.place(part, center, rotation)
    lt.assign(part, material(key))
    lt.box_uv(part, set_name, seed=seed)
    return part


def tube(points, radius, sides=6, strip='A', lane=3, key='trim', radii=None, caps=(True, True), set_name=None,
         wobble=0.0, seed=0):
    """A rope (or a wire, a hoop, a bail) along points: a round tube whose grain runs along it. On a trim strip, around
    the tube is one lane of it (a board of A or C) or the whole strip (the H strips); on a tileable set (set_name),
    world scale. radii (one per point) tapers it; wobble (metres) makes a rope's strands lumpy."""
    pts = [Vector(p) for p in points]
    rnd = random.Random(seed)
    bm = lp.new_bmesh()
    uv_layer = bm.loops.layers.uv.active
    lengths = [0.0]
    for i in range(1, len(pts)):
        lengths.append(lengths[-1] + (pts[i] - pts[i - 1]).length)
    tangents = [(pts[min(i + 1, len(pts) - 1)] - pts[max(i - 1, 0)]).normalized() for i in range(len(pts))]
    # Frames carried along the path (parallel transport), so the tube never twists where it turns.
    side = tangents[0].cross(UP) if abs(tangents[0].z) < 0.95 else tangents[0].cross(X_AXIS)
    side.normalize()
    rings = []
    for i, p in enumerate(pts):
        t = tangents[i]
        if i:
            side = tangents[i - 1].rotation_difference(t) @ side
            side = (side - t * side.dot(t)).normalized()
        up = side.cross(t).normalized()
        r = radii[i] if radii else radius
        ring = []
        for k in range(sides):
            a = 2.0 * math.pi * k / sides
            rr = r * (1.0 + (rnd.uniform(-wobble, wobble) / max(r, 1e-4) if wobble else 0.0))
            ring.append(bm.verts.new(p + (side * math.cos(a) + up * math.sin(a)) * rr))
        rings.append(ring)
    if set_name:
        scale = tile_scale(set_name)
        around = 2.0 * math.pi * radius

        def uv_of(i, k):
            return lengths[i] * scale, k / sides * around * scale
    else:
        key_strip = lt.TRIM_ALIASES.get(strip, strip)
        v_lo, v_hi = lt.TRIM_STRIPS[key_strip]
        scale = lt.TRIM_DENSITY / lt.TRIM_SIZE
        lane_m = kit.LANES.get(key_strip)
        if lane_m:
            v0, span = v_lo + lane * lane_m * scale, lane_m * scale
        else:
            v0, span = v_lo, v_hi - v_lo
        inset = 1.5 / lt.TRIM_SIZE
        u0 = rnd.uniform(0.0, 6.4)

        def uv_of(i, k):
            return (lengths[i] + u0) * scale, v0 + inset + (span - 2.0 * inset) * k / sides
    for i in range(len(rings) - 1):
        for k in range(sides):
            k1 = k + 1
            face = bm.faces.new((rings[i][k], rings[i][k1 % sides], rings[i + 1][k1 % sides], rings[i + 1][k]))
            for loop, (ii, kk) in zip(face.loops, ((i, k), (i, k1), (i + 1, k1), (i + 1, k))):
                loop[uv_layer].uv = uv_of(ii, kk)
    for end, ring in ((0, rings[0]), (1, rings[-1])):
        if caps[end]:
            face = bm.faces.new(ring if end else list(reversed(ring)))
            u, v = uv_of(0 if end == 0 else len(rings) - 1, 0)
            for loop, k in zip(face.loops, range(sides)):
                loop[uv_layer].uv = (u + 0.003 * math.cos(2.0 * math.pi * k / sides),
                                     v + 0.003 * (1.0 + math.sin(2.0 * math.pi * k / sides)))
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces)
    part = lp.mesh_object(bm)
    lt.assign(part, material(key))
    return part


def frayed(end, direction, radius, seed, count=5, length=0.09, key='trim'):
    """A rope's frayed end: thin strands splaying and curling out of it."""
    rnd = random.Random(seed)
    d = Vector(direction).normalized()
    side = d.cross(UP) if abs(d.z) < 0.95 else d.cross(X_AXIS)
    side.normalize()
    up = side.cross(d)
    parts = []
    for k in range(count):
        a = 2.0 * math.pi * (k + rnd.uniform(-0.3, 0.3)) / count
        out = (side * math.cos(a) + up * math.sin(a))
        start = Vector(end) + out * radius * 0.5 - d * 0.01
        spread = rnd.uniform(0.25, 0.7)
        bend = rnd.uniform(-0.4, 0.4)
        lng = length * rnd.uniform(0.6, 1.25)
        pts = [start + (d * (lng * t) + out * (lng * spread * t * t) + up * (bend * lng * t * t))
               for t in (0.0, 0.5, 1.0)]
        parts.append(tube(pts, radius * 0.32, sides=3, lane=rnd.randrange(4), radii=[radius * 0.34, radius * 0.26,
                                                                                    radius * 0.1], key=key))
    return parts


def lathe(profile, segments, key, strip=None, set_name=None, grain='around', seed=0, center=(0.0, 0.0, 0.0),
          rotation=(0.0, 0.0, 0.0), closed=False):
    """A turned part (lp.lathe around Z) mapped on a trim strip or a tileable set, then placed."""
    part, bands = lp.lathe(profile, segments=segments, closed=closed)
    lt.assign(part, material(key))
    lp.lathe_uv(part, set_name or strip, grain=grain, seed=seed)
    lp.place(part, center, rotation)
    return part, bands


def wheel(radius, width, spokes, seed, hub=0.1, felloe=0.075, tyre=0.014, wood='charcoal', burnt=0.0, gap=None):
    """A spoked wagon wheel in the XY plane around the origin, its axle along Z (turn it with lp.place): an iron tyre
    (the trim's iron strap) on a felloe, spokes and a hub with iron bands. wood is 'charcoal' (burnt) or 'trim'.
    burnt (0..1) burns spokes away; gap (an angle in degrees) burns a stretch of the felloe and tyre away there."""
    rnd = random.Random(seed)
    inner = radius - felloe
    parts = []
    span = (0.0, 360.0) if gap is None else (gap + 28.0, gap + 360.0 - 28.0)
    segments = 14

    def ring(r0, r1, z0, z1, key, set_name, hidden):
        """A ring (or an arc of one) as a closed profile turned about Z, without the face hidden against its partner
        (hidden: 1 the outside, under the tyre; 3 the inside, on the felloe)."""
        a0, a1 = math.radians(span[0]), math.radians(span[1])
        steps = segments if gap is None else int(segments * (a1 - a0) / (2.0 * math.pi)) + 1
        bm = lp.new_bmesh()
        uv_layer = bm.loops.layers.uv.active
        profile = [(r0, z0), (r1, z0), (r1, z1), (r0, z1)]
        rows = []
        for s in range(steps + 1):
            a = a0 + (a1 - a0) * s / steps
            rows.append([bm.verts.new((math.cos(a) * r, math.sin(a) * r, z)) for r, z in profile])
        scale = tile_scale(set_name)
        for s in range(steps):
            for j in range(4):
                if j == hidden:
                    continue
                j1 = (j + 1) % 4
                face = bm.faces.new((rows[s][j], rows[s][j1], rows[s + 1][j1], rows[s + 1][j]))
                for loop, (ss, jj) in zip(face.loops, ((s, j), (s, j1), (s + 1, j1), (s + 1, j))):
                    a = a0 + (a1 - a0) * ss / steps
                    r, z = profile[jj]
                    # Around the wheel along V (the bark's furrows), across it along U.
                    across = r if j in (0, 2) else z
                    loop[uv_layer].uv = (across * scale * 1.0, a * radius * scale)
        if gap is not None:
            for end in (0, steps):
                face = bm.faces.new(rows[end] if end else list(reversed(rows[end])))
                for loop, (r, z) in zip(face.loops, profile if end else list(reversed(profile))):
                    loop[uv_layer].uv = (r * scale, z * scale)
        bmesh.ops.recalc_face_normals(bm, faces=bm.faces)
        part = lp.mesh_object(bm)
        lt.assign(part, material(key))
        return part

    if wood == 'charcoal':
        parts.append(ring(inner, radius - tyre, -width * 0.5, width * 0.5, 'charcoal', 'BarkOak', 1))
    else:
        felloe_part = ring(inner, radius - tyre, -width * 0.5, width * 0.5, 'trim', 'BarkOak', 1)
        lp.lathe_uv(felloe_part, 'Beams', grain='around', seed=seed)
        parts.append(felloe_part)
    tyre_part = ring(radius - tyre, radius, -width * 0.56, width * 0.56, 'trim', 'MetalRust', 3)
    lp.lathe_uv(tyre_part, 'Iron', grain='around', seed=seed + 7)
    parts.append(tyre_part)
    # The hub: a turned block with an iron band at each end.
    burnt_wood = wood == 'charcoal'
    hub_part, _ = lathe([(0.0, -width * 1.1), (hub * 0.7, -width * 1.1), (hub, -width * 0.45), (hub, width * 0.45),
                         (hub * 0.7, width * 1.1), (0.0, width * 1.1)], 8, wood,
                        set_name='BarkOak' if burnt_wood else None, strip=None if burnt_wood else 'Beams', grain='up',
                        seed=seed)
    parts.append(hub_part)
    for z in (-width * 0.75, width * 0.75):
        band, _ = lathe([(hub * 0.86, z - 0.018), (hub * 0.93, z - 0.018), (hub * 0.93, z + 0.018),
                         (hub * 0.86, z + 0.018)], 8, 'trim', strip='Iron', closed=True, seed=seed + 1)
        parts.append(band)
    start = rnd.uniform(0.0, math.pi)
    for k in range(spokes):
        a = start + 2.0 * math.pi * k / spokes
        deg = math.degrees(a) % 360.0
        if gap is not None and min(abs(deg - gap % 360.0), 360.0 - abs(deg - gap % 360.0)) < 30.0:
            continue
        reach = 1.0
        if burnt > 0.0 and rnd.random() < burnt:
            if rnd.random() < 0.45:
                continue           # burnt away
            reach = rnd.uniform(0.35, 0.75)
        d = Vector((math.cos(a), math.sin(a), 0.0))
        p0, p1 = d * (hub * 0.85), d * (hub * 0.85 + (inner + 0.02 - hub * 0.85) * reach)
        # Spokes are deeper along the axle than across the wheel, thinning toward the felloe.
        spoke = lp.sweep([p0, p1], [(-0.017, -0.024), (0.017, -0.024), (0.014, 0.024), (-0.014, 0.024)],
                         scales=[1.0, 0.8 if reach < 1.0 else 0.85])
        if wood == 'charcoal':
            lp.grain(spoke, 'BarkOak', axis=d, seed=rnd.randint(0, 999), material=material('charcoal'))
        else:
            lp.grain(spoke, 'Beams', axis=d, seed=rnd.randint(0, 999))
        parts.append(spoke)
    return lp.join('_wheel', parts)


def iron_lantern(at, seed, key_iron='ironblack', size=1.0, hood=True):
    """A hanging iron lantern, its middle at at: a square frame with four lit panes (LanternGlow, mapped on the trim's
    window glass so a swap to MI_HouseTrim shows it dark), a peaked hood with a vent and a ring on top. Returns the
    parts and the ring's top (where it hangs from) and the glass middle (for SOCKET_Light)."""
    rnd = random.Random(seed)
    x, y, z = at
    s = size
    parts = []
    w, h = 0.2 * s, 0.26 * s
    glass = lp.block((w, w, h), (x, y, z))
    lt.assign(glass, material('glow'))
    lt.trim_uv(glass, None, 'Glass', fit=True, u_offset=rnd.uniform(0.0, 6.4))
    parts.append(glass)
    iron = material(key_iron)
    on_trim = key_iron == 'trim'          # the trim's iron strap (no extra material slot) or black-painted iron
    set_name = None if on_trim else 'MetalWorn'
    strip = 'Iron' if on_trim else None

    def iron_box(size3, center, rot=(0.0, 0.0, 0.0)):
        part = lp.block(size3, center, rot)
        lt.assign(part, iron)
        if on_trim:
            lt.trim_uv(part, None, 'Iron', fit=True, u_offset=rnd.uniform(0.0, 6.4))
        else:
            lt.box_uv(part, set_name, seed=rnd.randint(0, 999))
        return part
    frame = 0.022 * s
    for dx in (-1.0, 1.0):
        for dy in (-1.0, 1.0):
            parts.append(iron_box((frame, frame, h + 0.05 * s), (x + dx * w * 0.5, y + dy * w * 0.5, z)))
    for zz in (z - h * 0.5 - 0.015 * s, z + h * 0.5 + 0.012 * s):
        parts.append(iron_box((w + 0.05 * s, w + 0.05 * s, 0.03 * s), (x, y, zz)))
    # Guard bars across each pane.
    for k in range(4):
        a = k * math.pi * 0.5
        d = Vector((math.cos(a), math.sin(a), 0.0))
        c = Vector((x, y, z)) + d * (w * 0.5 + 0.008 * s)
        parts.append(iron_box((0.012 * s if abs(d.x) < 0.5 else 0.01 * s, 0.012 * s if abs(d.y) < 0.5 else 0.01 * s,
                               h), (c.x, c.y, c.z)))
    low = z - h * 0.5
    base, _ = lathe([(0.0, low - 0.09 * s), (0.06 * s, low - 0.09 * s), (0.1 * s, low - 0.035 * s),
                     (0.0, low - 0.035 * s)], 4, key_iron, set_name=set_name, strip=strip, seed=seed,
                    rotation=(0.0, 0.0, 45.0))
    lp.place(base, (x, y, 0.0))
    parts.append(base)
    top = z + h * 0.5 + 0.03 * s
    if hood:
        cap, _ = lathe([(0.0, top + 0.13 * s), (0.17 * s, top), (0.17 * s, top - 0.012 * s), (0.0, top - 0.012 * s)], 4,
                       key_iron, set_name=set_name, strip=strip, seed=seed + 1, rotation=(0.0, 0.0, 45.0))
        lp.place(cap, (x, y, 0.0))
        parts.append(cap)
        vent, _ = lathe([(0.0, top + 0.2 * s), (0.04 * s, top + 0.2 * s), (0.045 * s, top + 0.1 * s),
                         (0.0, top + 0.1 * s)], 6, key_iron, set_name=set_name, strip=strip, seed=seed + 2)
        lp.place(vent, (x, y, 0.0))
        parts.append(vent)
        top += 0.2 * s
    ring_r = 0.035 * s
    ring_pts = [(x + ring_r * math.cos(a), y, top + ring_r + ring_r * math.sin(a))
                for a in (2.0 * math.pi * k / 8 for k in range(9))]
    parts.append(tube(ring_pts, 0.007 * s, sides=4, key=key_iron, set_name=set_name, strip=strip or 'A',
                      caps=(False, False)))
    return parts, Vector((x, y, top + 2.0 * ring_r)), Vector((x, y, z))


# --- Stairs, railings and their collision (kit parts) ---

def flight(m, foot, head, across, width, risers, seed=0, stringer=(0.07, 0.28), tread_uv=None, tread_mat='planks',
           nosing=0.03):
    """A timber stair flight from foot (the middle of its first riser, on the lower floor) up to head (the middle of
    the upper floor's edge it arrives at): risers - 1 treads on two stringers. across is the horizontal direction
    across it (its width). Returns the rise and going of a step."""
    rnd = random.Random(seed)
    foot, head = Vector(foot), Vector(head)
    across = Vector(across).normalized()
    d = Vector((head.x - foot.x, head.y - foot.y, 0.0))
    run = d.length
    d.normalize()
    rise = (head.z - foot.z) / risers
    going = run / (risers - 1)
    for k in range(1, risers):
        c = foot + d * ((k - 1) * going + going * 0.5 - nosing * 0.5) + Vector((0.0, 0.0, k * rise - 0.025))
        jitter = across * rnd.uniform(-0.01, 0.01)
        m.board(c - across * width * 0.5 + jitter, c + across * width * 0.5 + jitter, going + nosing, 0.05,
                face=(0.0, 0.0, 1.0), uv=tread_uv or PlankRow(), mat=tread_mat)
    # The stringers follow the nosing line, their top edge just under the treads.
    st_w, st_d = stringer
    for side in (-1.0, 1.0):
        off = across * side * (width * 0.5 + st_w * 0.5)
        drop = Vector((0.0, 0.0, -st_d * 0.5 + 0.02))
        p0 = foot + off + drop + Vector((0.0, 0.0, rise * 0.5)) - d * going * 0.3
        p1 = head + off + drop + d * 0.02
        m.board(p0, p1, st_d, st_w, face=(across * side).to_tuple(), uv=kit.Trim('C', lane='each'))
    return rise, going


def flight_hull(m, foot, head, across, width, thick=0.3):
    """A walkable ramp hull over a flight: its top runs along the treads' nosings."""
    foot, head = Vector(foot), Vector(head)
    across = Vector(across).normalized() * (width * 0.5)
    d = Vector((head.x - foot.x, head.y - foot.y, 0.0)).normalized()
    low = foot - d * 0.12
    down = Vector((0.0, 0.0, -thick))
    m.hull_points([low + across, low - across, head + across, head - across,
                   low + across + down, low - across + down, head + across + down, head - across + down])


def hull_along(m, p0, p1, height, thick=0.1, below=0.0):
    """A box hull standing on the line p0-p1 (level or sloped), height tall and thick through: a railing, a wall."""
    p0, p1 = Vector(p0), Vector(p1)
    d = Vector((p1.x - p0.x, p1.y - p0.y, 0.0))
    side = d.cross(UP).normalized() * (thick * 0.5) if d.length > 1e-6 else Vector((thick * 0.5, 0.0, 0.0))
    up, down = Vector((0.0, 0.0, height)), Vector((0.0, 0.0, -below))
    m.hull_points([p + s + h for p in (p0, p1) for s in (side, -side) for h in (up, down)])


def railing(m, p0, p1, height=1.0, face=None, post_every=1.4, posts=True, ends=(True, True), mid=0.5, cross=False,
            post=0.11, rail=(0.12, 0.06), seed=0, post_below=0.28, cuts=None):
    """A timber railing along the line p0-p1 (its walking level: a deck's edge, a stair's nosing line): square posts,
    a top rail and a mid rail (mid: its height as a share of height, None for none), and with cross a saltire brace
    in each bay. face: the side the rails are fixed to (outward). cuts: how many times the rails are cut along their
    length for the baked shading (None: the kit's default, every 2.5 m). Returns the post positions."""
    p0, p1 = Vector(p0), Vector(p1)
    flat = Vector((p1.x - p0.x, p1.y - p0.y, 0.0))
    length = flat.length
    if face is None:
        face = flat.cross(UP).normalized()
    face = Vector(face).normalized()
    count = max(1, int(math.ceil(length / post_every)))
    spots = [p0.lerp(p1, k / count) for k in range(count + 1)]
    if not ends[0]:
        spots = spots[1:]
    if not ends[1]:
        spots = spots[:-1]
    if posts:
        for p in spots:
            m.board(p - Vector((0.0, 0.0, post_below)), p + Vector((0.0, 0.0, height + 0.05)), post, post,
                    face=face.to_tuple(), uv=kit.Trim('C', lane='each'), bevel=0.01)
    up = Vector((0.0, 0.0, 1.0))
    shift = face * (post * 0.5 + rail[1] * 0.5)
    m.board(p0 + up * (height - rail[0] * 0.5) + shift, p1 + up * (height - rail[0] * 0.5) + shift, rail[0], rail[1],
            face=face.to_tuple(), uv=kit.Trim('C', lane='each'), cuts=cuts)
    if mid is not None:
        m.board(p0 + up * height * mid + shift, p1 + up * height * mid + shift, rail[0] * 0.85, rail[1] * 0.75,
                face=face.to_tuple(), uv='A', cuts=cuts)
    if cross:
        for a, b in zip(spots, spots[1:]):
            lo, hi = up * 0.08, up * (height * mid - 0.06) if mid else up * (height - 0.16)
            for p, q in ((a + lo, b + hi), (a + hi, b + lo)):
                m.board(p + shift * 0.6, q + shift * 0.6, 0.08, 0.04, face=face.to_tuple(), uv='C')
    return spots


# --- The dry-stone wall (Fences.py's own code) ---

def fences_wall():
    """Fences.py's dry-stone wall: WALL_LENGTH, WALL_DENSITY, BODY_TOP, wall_profile(), stones(), wall_body() and
    coping(), read from its source (only those, so its own models aren't built). Pieces made with them chain with
    SM_StoneWall and SM_StoneWallEnd without a seam, and follow it if it ever changes."""
    path = os.path.join(REPO, 'Art', 'Models', 'Props', 'Fences.py')
    with open(path, encoding='utf-8') as file:
        tree = ast.parse(file.read(), path)
    functions = {'wall_profile', 'stones', 'wall_body', 'coping'}
    names = {'WALL_LENGTH', 'WALL_DENSITY', 'BODY_TOP'}
    body = []
    for node in tree.body:
        if isinstance(node, (ast.Import, ast.ImportFrom)):
            body.append(node)
        elif isinstance(node, ast.FunctionDef) and node.name in functions:
            body.append(node)
        elif isinstance(node, ast.Assign) and any(isinstance(t, ast.Name) and t.id in names for t in node.targets):
            body.append(node)
    namespace = {'__name__': 'fences_wall', '__file__': path}
    exec(compile(ast.Module(body=body, type_ignores=[]), path, 'exec'), namespace)
    missing = (functions | names) - set(namespace)
    if missing:
        raise RuntimeError(f"Fences.py no longer defines {', '.join(sorted(missing))}")
    return types.SimpleNamespace(**{key: namespace[key] for key in functions | names})


# --- Previews ---

def preview_path(name):
    return os.path.join(PREVIEW_DIR, name + '.png')


def preview(objects, name, **options):
    """Renders objects to Saved/ArtPreviews/RansomsRest/Ruins/<name>.png (only with --preview)."""
    if lt.want_preview():
        return lt.preview(objects, preview_path(name), **options)
    return None
