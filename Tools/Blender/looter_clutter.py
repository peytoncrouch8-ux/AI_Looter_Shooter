"""Helpers for the frontier clutter (Art/Models/Props/FrontierClutter.py, look sheet 42 via Tools/Blender/clutter_sheet.py):
the small props that dress Ransom's Rest's porches, yards and Main Street. A model is a looter_ruins Model (kit parts:
boxes, boards, cylinders, panels; lp parts merged with add(); hulls, sockets) with what small, mostly round, mostly
one-material props need on top:

  - materials by key: 'trim' (HouseTrim, the house trim sheet: nearly everything), 'endgrain' (WoodEndGrain), 'glow'
    (LanternGlow), 'tin' (MailboxTin), 'brass' (BrassWorn), 'dirt' (GroundDirt): the shared names, made exactly as their
    first families make them, so each is one material instance in Unreal;
  - the trim sheet used as a palette: weathered wood (strips A, B, C), cream (G, its clean stretches), faded teal and
    oxide paint (H1, H2), iron (H3), dark glass or water (H4); strip_uv() puts a point given in metres on a strip;
  - parts written straight into the model's mesh with their UVs: tube() (rods, rails, bent bars, ropes: any profile
    swept along a line, its grain along it at world scale), turned() (lathe work: staves, hoops, cans, bottles, pump
    bodies; grain round it, up it or stave by stave), cloth() (a thin two-sided sheet through a grid: shawls, rags,
    leaves, pennants), lettering() (Rye lettering, looter_town's, laid through any mapping onto a board or a sack);
  - Model.done(): the sliver-free triangulation (lr.triangulate_clean), smoothing by angle, baked occlusion, Nanite off
    and LODs for the bigger ones, hulls, sockets, and a report line (REPORT, for the previews and the look sheet);
  - the previews' set: Ransom's Rest's golden late afternoon (looter_homes' stage and sun), a framing that hugs small
    things, buildings brought in from earlier exports (import_fbx) and props dropped onto them (drop()).

Every random choice comes from a seeded random.Random, mesh elements are only ever walked in index order, so every run
gives the same meshes (Art/README.md: export twice and compare).
"""
import json
import math
import os
import random

import bmesh
import bpy
from mathutils import Matrix, Vector
from mathutils.bvhtree import BVHTree

import looter_buildings as kit
import looter_homes as homes
import looter_model as lm
import looter_props as lp
import looter_ruins as lr
import looter_textures as lt
import looter_town as town

ARGS = kit._args()
UP = Vector((0.0, 0.0, 1.0))
X_AXIS = Vector((1.0, 0.0, 0.0))
TRIM_SCALE = lt.TRIM_DENSITY / lt.TRIM_SIZE          # UV units per metre on the trim sheet
INSET = 1.5 / lt.TRIM_SIZE                           # a texel and a half clear of a strip's edge
PREVIEW_DIR = os.path.join(lt.PREVIEW_DIR, 'RansomsRest', 'Clutter')
REPORT_PATH = os.path.join(PREVIEW_DIR, 'FrontierClutter.report.json')
SHEET_PATH = os.path.join(homes.SHEET_DIR, '42_FrontierClutter.jpg')
REPORT = {}

# --- Materials ---

MAKERS = {
    'trim': lambda: lt.material('HouseTrim'),
    'endgrain': lambda: lt.material('WoodEndGrain'),
    'glow': lambda: kit._material('glow'),
    'tin': lambda: lt.material('MetalWorn', name='MailboxTin', tint=0xc9c6bd),
    'brass': lambda: lt.material('MetalWorn', name='BrassWorn', tint=0xc49c56),
    'dirt': lambda: lt.material('GroundDirt'),
    'hay': lambda: lt.material('Hay'),
}
NAMES = {'trim': 'HouseTrim', 'endgrain': 'WoodEndGrain', 'glow': 'LanternGlow', 'tin': 'MailboxTin',
         'brass': 'BrassWorn', 'dirt': 'GroundDirt', 'hay': 'Hay'}
KEY_OF = {name: key for key, name in NAMES.items()}
KEY_OF.update({'ironblack': 'ironblack'})
SET_OF = {'tin': 'MetalWorn', 'brass': 'MetalWorn', 'dirt': 'GroundDirt', 'endgrain': 'WoodEndGrain', 'hay': 'Hay'}
# The trim's iron strip (H3) carries rust blotches sized for a building's straps; on small iron (hoops, a pump, a
# horseshoe) done() packs its U this many times tighter and folds it into the strip's cleanest stretch.
IRON_U = 1.6
IRON_CLEAN = (3.6, 0.5)  # (start, length) in metres along U: the iron strip's stretch with the least rust (1%, measured)
IRON_DIM = 0.85        # and its baked occlusion is lowered by this factor (M_World darkens the colour with it)


def material(key):
    """The shared material for a key (made on first use in this Blender session)."""
    name = NAMES[key]
    mat = bpy.data.materials.get(name)
    if mat is None or (key != 'glow' and mat.get('Master') is None):
        mat = MAKERS[key]()
    return mat


# --- The trim sheet as a palette ---

STRIPS = {'Siding': 'A', 'Logs': 'B', 'Beams': 'C', 'Stone': 'D', 'Shingles': 'E', 'Tin': 'F', 'Plaster': 'G',
          'TrimTeal': 'H1', 'TrimRed': 'H2', 'Iron': 'H3', 'Glass': 'H4'}
ALIAS = {v: k for k, v in STRIPS.items()}
# Clean stretches of the cream strip G along U (metres), clear of its stains and cracks (T_HouseTrim_BC, read off the
# texture, 2026-10-14).
CREAM = [(4.12, 4.88), (5.42, 6.35)]


def strip_of(strip):
    return lt.TRIM_ALIASES.get(strip, strip)


def strip_height(strip):
    v0, v1 = lt.TRIM_STRIPS[strip_of(strip)]
    return (v1 - v0) / TRIM_SCALE


def lane_width(strip):
    return kit.LANES.get(strip_of(strip)) or strip_height(strip)


def lanes(strip):
    return max(1, int(round(strip_height(strip) / lane_width(strip))))


def strip_uv(strip, u_m, v_m):
    """The UV of a point u_m metres along a trim strip and v_m metres up it from its bottom (kept a texel and a half
    inside it)."""
    v0, v1 = lt.TRIM_STRIPS[strip_of(strip)]
    v = v0 + v_m * TRIM_SCALE
    return (u_m * TRIM_SCALE, min(max(v, v0 + INSET), v1 - INSET))


def pick_u(strip, length, rng, lane=None):
    """A lane and a start along U (metres) where a part `length` long lies on one board, clear of the strip's end
    joints (looter_props.JOINT_FREE); for strips without a table, any lane and start."""
    name = ALIAS.get(strip_of(strip))
    table = lp.JOINT_FREE.get(name) if name else None
    if not table:
        return (rng.randrange(lanes(strip)) if lane is None else lane), rng.uniform(0.0, 6.4)
    if strip_of(strip) == 'G':
        table = [(0, a, b) for a, b in CREAM]
    regions = [r for r in table if lane is None or r[0] == lane or lanes(strip) == 1] or table
    fitting = [r for r in regions if r[2] - r[1] >= length + 0.1] or sorted(regions, key=lambda r: r[1] - r[2])[:1]
    board, start, end = rng.choice(fitting)
    u = start + 0.05 + rng.uniform(0.0, max(0.0, end - start - length - 0.1))
    return (board if lanes(strip) > 1 else 0), u


# --- The model ---

class Model(lr.Model):
    """lr.Model with the clutter's materials by key (or by their names), raw faces with UVs (put), a move of everything
    built so far (shift), and done(): a small prop finished and reported."""

    def __init__(self, name, seed=1):
        super().__init__(name, seed)
        self.notes = {}

    def slot(self, key):
        key = KEY_OF.get(key, key)
        if key not in self.slots:
            mat = material(key) if key in MAKERS else (lr.material(key) if key in lr._MAKERS else
                                                        bpy.data.materials[key])
            self.slots[key] = (self._index_of(mat), mat)
        return self.slots[key][0]

    def put(self, cos, faces, key='trim', smooth=True, ao_floor=0.0):
        """Adds faces: cos are corner positions, faces (indices, uvs) each. Returns the new vertices."""
        verts = [self.bm.verts.new(Vector(c)) for c in cos]
        slot = self.slot(key)
        for idx, uvs in faces:
            try:
                face = self.bm.faces.new([verts[i] for i in idx])
            except ValueError:
                continue
            face.material_index = slot
            face.smooth = smooth
            face[self.floor] = ao_floor
            for loop, uv in zip(face.loops, uvs):
                loop[self.uv].uv = uv
        return verts

    def shift(self, offset):
        """Moves everything built so far, hulls and sockets too."""
        d = Vector(offset)
        for v in self.bm.verts:
            v.co += d
        self.hulls = [[Vector(p) + d for p in hull] for hull in self.hulls]
        self.sockets = [(n, tuple(Vector(l) + d), r) for n, l, r in self.sockets]

    def bounds(self):
        cos = [v.co for v in self.bm.verts]
        return (Vector((min(c.x for c in cos), min(c.y for c in cos), min(c.z for c in cos))),
                Vector((max(c.x for c in cos), max(c.y for c in cos), max(c.z for c in cos))))

    def center_footprint(self):
        """Moves the model so the middle of its footprint is the origin (the ground stays at z = 0). Returns the move."""
        lo, hi = self.bounds()
        d = Vector((-(lo.x + hi.x) * 0.5, -(lo.y + hi.y) * 0.5, 0.0))
        self.shift(d)
        return d

    def iron_speckle(self, factor=IRON_U):
        """Small iron on the trim's iron strip: its U, packed `factor` times tighter, is folded back and forth into
        IRON_CLEAN (mirrored, each face on one fold so nothing smears), so hoops and brackets show the strip's dark
        iron and only a little of its rust."""
        if 'trim' not in self.slots:
            return
        slot = self.slots['trim'][0]
        v0, v1 = lt.TRIM_STRIPS['H3']
        start, span = IRON_CLEAN
        for face in self.bm.faces:
            if face.material_index != slot:
                continue
            if not all(v0 - 1e-5 <= loop[self.uv].uv.y <= v1 + 1e-5 for loop in face.loops):
                continue
            metres = [loop[self.uv].uv.x / TRIM_SCALE * factor for loop in face.loops]
            fold = math.floor(min(metres) / span)
            for loop, m in zip(face.loops, metres):
                t = m / span - fold
                t = 1.0 - t if fold % 2 else t
                loop[self.uv].uv = ((start + t * span) * TRIM_SCALE, loop[self.uv].uv.y)

    def done(self, pivot, caption='', ao=0.4, ground=True, smooth=62.0, lods=None, nanite=False, fallback=100.0,
             collision=None, min_height=0.0012, crumb=0.006, darken=None):
        """Finishes the model: one mesh with its slots, smooth shading with hard edges past `smooth` degrees, occlusion
        baked within `ao` metres (the ground under the pivot counts with ground), Nanite off with LODs at 50% and 25%
        once it passes 600 triangles (lods: '50,25', None for the default, '' for none), its hulls and sockets. darken
        {material key: factor} lowers the baked occlusion on that material's faces (tilled soil darker than the street:
        M_World darkens with it)."""
        self.iron_speckle()
        bpy.data.objects.remove(self._obj)
        bpy.data.meshes.remove(self._mesh)
        mesh = bpy.data.meshes.new(self.name)
        lr.triangulate_clean(self.bm, min_height=min_height, crumb=crumb)
        self.bm.to_mesh(mesh)
        self.bm.free()
        by_index = {index: mat for index, mat in self.slots.values()}
        for index in sorted(by_index):
            mesh.materials.append(by_index[index])
        obj = bpy.data.objects.new(self.name, mesh)
        bpy.context.scene.collection.objects.link(obj)
        triangles = sum(len(p.vertices) - 2 for p in mesh.polygons)
        if nanite:
            obj['Fallback'] = float(fallback)
        else:
            obj['Nanite'] = 0
            lod = lods if lods is not None else ('50,25' if triangles >= 600 else '')
            if lod:
                obj['LODs'] = lod
        if collision:
            obj['Collision'] = collision
        lm.smooth(obj, smooth)
        if '--no-ao' not in ARGS:
            lt.bake_vertex_ao(obj, ground=ground, distance=ao)
            kit._raise_ao(mesh)
        # The trim's iron strip reads too orange on small iron: its faces are dimmed (IRON_DIM), and any material in
        # darken by its factor.
        darken = dict(darken or {})
        darken.setdefault('H3', IRON_DIM)
        raw = lt._read_col(mesh)
        uv_layer = mesh.uv_layers.active.data
        iron_v0, iron_v1 = lt.TRIM_STRIPS['H3']
        trim_index = mesh.materials.find('HouseTrim')
        for key, factor in darken.items():
            if key == 'H3':
                faces = [p for p in mesh.polygons if p.material_index == trim_index and
                         all(iron_v0 - 1e-5 <= uv_layer[li].uv.y <= iron_v1 + 1e-5 for li in p.loop_indices)]
            else:
                index = mesh.materials.find(NAMES.get(key, key))
                faces = [p for p in mesh.polygons if index >= 0 and p.material_index == index]
            for poly in faces:
                for li in poly.loop_indices:
                    raw[li, 3] *= factor
        lt._write_col(mesh, raw)
        if '_aofloor' in mesh.attributes:
            mesh.attributes.remove(mesh.attributes['_aofloor'])
        for points in self.hulls:
            hb = bmesh.new()
            verts = [hb.verts.new(p) for p in points]
            result = bmesh.ops.convex_hull(hb, input=verts)
            inside = [v for v in result['geom_interior'] + result['geom_unused'] if isinstance(v, bmesh.types.BMVert)]
            bmesh.ops.delete(hb, geom=list(dict.fromkeys(inside)), context='VERTS')
            hull_mesh = bpy.data.meshes.new('UCX_' + self.name)
            hb.to_mesh(hull_mesh)
            hb.free()
            hull = bpy.data.objects.new('UCX_' + self.name, hull_mesh)
            bpy.context.scene.collection.objects.link(hull)
            hull.parent = obj
            hull.display_type = 'WIRE'
        for name, location, rotation in self.sockets:
            lm.socket(obj, name, location, rotation)
        report(obj, pivot, caption)
        return obj


# --- Reporting ---

def socket_list(obj):
    out = []
    for child in sorted(obj.children, key=lambda o: o.name):
        if child.name.startswith('SOCKET_'):
            out.append((child.name[7:].split('.')[0], tuple(round(c, 3) for c in child.location),
                        tuple(round(math.degrees(a), 1) for a in child.rotation_euler)))
    return out


def report(obj, pivot, caption=''):
    mesh = obj.data
    triangles = sum(len(p.vertices) - 2 for p in mesh.polygons)
    co = [v.co for v in mesh.vertices]
    lo = Vector((min(c.x for c in co), min(c.y for c in co), min(c.z for c in co)))
    hi = Vector((max(c.x for c in co), max(c.y for c in co), max(c.z for c in co)))
    size = hi - lo
    sockets = socket_list(obj)
    hulls = len([c for c in obj.children if c.name.startswith('UCX_')])
    REPORT[obj.name] = dict(triangles=triangles, materials=[m.name for m in mesh.materials],
                            size_cm=[round(s * 100.0, 1) for s in size],
                            bounds_m=[[round(v, 3) for v in lo], [round(v, 3) for v in hi]],
                            sockets=[dict(name=n, location_m=l, rotation_deg=r) for n, l, r in sockets], hulls=hulls,
                            pivot=pivot, caption=caption, nanite=int(obj.get('Nanite', 1)), lods=obj.get('LODs', ''),
                            collision=obj.get('Collision', 'hulls' if hulls else 'mesh'))
    lt._log(f"CLUTTER {obj.name}: {triangles} triangles, {len(mesh.materials)} slot(s) "
            f"({', '.join(m.name for m in mesh.materials)}), {size.x * 100:.0f} x {size.y * 100:.0f} x "
            f"{size.z * 100:.0f} cm, {hulls} hull(s), LODs {obj.get('LODs', '-')}, sockets "
            + ('; '.join(f'{n} {l}' for n, l, r in sockets) or '-'))


def write_report():
    os.makedirs(PREVIEW_DIR, exist_ok=True)
    with open(REPORT_PATH, 'w', encoding='utf-8') as f:
        json.dump(REPORT, f, indent=1)
    lt._log(f'CLUTTER report {REPORT_PATH}')


# --- Frames along a line ---

def frames(points, up=None):
    """(point, tangent, side, normal) along a polyline, the frame carried along it without twisting. up (optional)
    fixes the first frame: side = tangent x up."""
    pts = [Vector(p) for p in points]
    n = len(pts)
    tangents = [(pts[min(i + 1, n - 1)] - pts[max(i - 1, 0)]).normalized() for i in range(n)]
    t0 = tangents[0]
    if up is not None and t0.cross(Vector(up)).length > 1e-6:
        side = t0.cross(Vector(up))
    else:
        side = t0.cross(UP) if abs(t0.z) < 0.95 else t0.cross(X_AXIS)
    side.normalize()
    out = []
    for i, t in enumerate(tangents):
        if i:
            side = tangents[i - 1].rotation_difference(t) @ side
            side = (side - t * side.dot(t)).normalized()
        out.append((pts[i], t, side, side.cross(t).normalized()))
    return out


def _orient(cos, idx, outward):
    """idx turned so the face's normal points along outward."""
    a, b, c = (Vector(cos[i]) for i in idx[:3])
    n = Vector()
    for k in range(len(idx)):
        p, q = Vector(cos[idx[k]]), Vector(cos[idx[(k + 1) % len(idx)]])
        n += Vector(((p.y - q.y) * (p.z + q.z), (p.z - q.z) * (p.x + q.x), (p.x - q.x) * (p.y + q.y)))
    return list(idx) if n.dot(outward) >= 0.0 else list(reversed(idx))


def ngon(radius, sides, start=0.0, squash=1.0):
    return [(math.cos(start + 2.0 * math.pi * k / sides) * radius,
             math.sin(start + 2.0 * math.pi * k / sides) * radius * squash) for k in range(sides)]


def rect(w, h):
    return [(-w * 0.5, -h * 0.5), (w * 0.5, -h * 0.5), (w * 0.5, h * 0.5), (-w * 0.5, h * 0.5)]


def tube(m, points, profile, strip='A', up=None, scales=None, caps=(True, True), lane=None, u0=None, key='trim',
         set_name=None, smooth=True, wobble=0.0, ao_floor=0.0, profiles=None, shifts=None):
    """A part swept along points (a rod, a rail, a bent bar, a rope, a stalk): its cross-section is profile [(across,
    up), ...] (counterclockwise), scaled by scales (one per point), or profiles (one per point, each as many corners). On a
    trim strip its grain runs along it at world scale and its girth lies across one lane (a board of A or C; the whole
    strip for the H strips and G), squeezed into it if it's wider; on a tileable set (key 'tin', 'brass', 'dirt'), world
    scale both ways."""
    rng = m.rng
    fr = frames(points, up)
    profile = profiles[0] if profiles else profile
    count = len(profile)
    perim = [0.0]
    for k in range(count):
        a, b = Vector(profile[k]), Vector(profile[(k + 1) % count])
        perim.append(perim[-1] + (b - a).length)
    total = perim[-1]
    along = [0.0]
    for i in range(1, len(fr)):
        along.append(along[-1] + (fr[i][0] - fr[i - 1][0]).length)
    set_name = set_name or SET_OF.get(key)
    if set_name:
        scale = lt.SETS[set_name]['density'] / float(lt.SETS[set_name]['size'])
        off = (rng.uniform(0.0, 3.0), rng.uniform(0.0, 3.0))

        def uv(i, k):
            return ((along[i] + off[0]) * scale, (perim[k] + off[1]) * scale)
    else:
        width = lane_width(strip)
        lane_i, u_start = pick_u(strip, along[-1], rng, lane)
        if u0 is not None:
            u_start = u0
        squeeze = min(1.0, (width - 0.02) / max(total, 1e-6))
        base = lane_i * width + (width - total * squeeze) * 0.5

        def uv(i, k):
            return strip_uv(strip, u_start + along[i], base + perim[k] * squeeze)
    cos = []
    rings = []
    for i, (p, t, side, nrm) in enumerate(fr):
        s = scales[i] if scales else 1.0
        ring = []
        for k, (a, b) in enumerate(profiles[i] if profiles else profile):
            q = p + side * (a * s) + nrm * (b * s) + (t * shifts[i][k] if shifts else Vector())
            if wobble:
                q += (side * rng.uniform(-wobble, wobble) + nrm * rng.uniform(-wobble, wobble))
            ring.append(len(cos))
            cos.append(q)
        rings.append(ring)
    faces = []
    for i in range(len(rings) - 1):
        mid = (fr[i][0] + fr[i + 1][0]) * 0.5
        for k in range(count):
            k1 = (k + 1) % count
            idx = [rings[i][k], rings[i][k1], rings[i + 1][k1], rings[i + 1][k]]
            uvs = {rings[i][k]: uv(i, k), rings[i][k1]: uv(i, k + 1), rings[i + 1][k1]: uv(i + 1, k + 1),
                   rings[i + 1][k]: uv(i + 1, k)}
            center = sum((cos[j] for j in idx), Vector()) * 0.25
            idx = _orient(cos, idx, center - mid)
            faces.append((idx, [uvs[j] for j in idx]))
    for end, i in ((0, 0), (1, len(rings) - 1)):
        if caps[end]:
            t = fr[i][1] * (1.0 if end else -1.0)
            idx = _orient(cos, rings[i], t)
            c = uv(i, 0)
            r = 0.002 if not set_name else 0.0
            faces.append((idx, [(c[0] + r * math.cos(2.0 * math.pi * k / count),
                                 c[1] + r * math.sin(2.0 * math.pi * k / count)) for k in range(len(idx))]))
    m.put(cos, faces, key=key, smooth=smooth, ao_floor=ao_floor)
    return [fr[i][0] for i in range(len(fr))]


def rod(m, points, radius, strip='A', sides=6, radii=None, caps=(True, True), lane=None, key='trim', u0=None,
        wobble=0.0):
    """A round rod along points (radii: one per point tapers it)."""
    scales = [r / radius for r in radii] if radii else None
    return tube(m, points, ngon(radius, sides, start=math.pi / sides), strip=strip, scales=scales, caps=caps,
                lane=lane, key=key, u0=u0, wobble=wobble)


def arc(center, radius, a0, a1, steps, axis='y', squash=1.0):
    """Points on a circular arc around center: in the XZ plane (axis 'y'), YZ ('x') or XY ('z'); angles in degrees."""
    out = []
    for k in range(steps + 1):
        a = math.radians(a0 + (a1 - a0) * k / steps)
        c, s = math.cos(a) * radius, math.sin(a) * radius * squash
        if axis == 'y':
            out.append(Vector(center) + Vector((c, 0.0, s)))
        elif axis == 'x':
            out.append(Vector(center) + Vector((0.0, c, s)))
        else:
            out.append(Vector(center) + Vector((c, s, 0.0)))
    return out


def spline(points, per=4, closed=False):
    """A Catmull-Rom curve through points, per steps a span."""
    pts = [Vector(p) for p in points]
    n = len(pts)
    out = []
    for i in range(n if closed else n - 1):
        p0 = pts[(i - 1) % n] if closed else pts[max(i - 1, 0)]
        p1, p2 = pts[i % n], pts[(i + 1) % n]
        p3 = pts[(i + 2) % n] if closed else pts[min(i + 2, n - 1)]
        for k in range(per):
            t = k / per
            out.append(0.5 * (2.0 * p1 + (p2 - p0) * t + (2.0 * p0 - 5.0 * p1 + 4.0 * p2 - p3) * t * t
                              + (3.0 * p1 - p0 - 3.0 * p2 + p3) * t * t * t))
    out.append(out[0].copy() if closed else pts[-1])
    return out


# --- Lathe work ---

def turned(m, profile, sides, at=(0.0, 0.0, 0.0), rot=None, strip='A', key='trim', grain='around', lane=None,
           u0=None, v0=None, phase=0.0, smooth=True, ao_floor=0.0, set_name=None, density=None):
    """A part turned about its own Z: profile [(radius, z), ...] (a radius of 0 closes an end in a point) traced with the
    solid on its left (up the outside, down the inside: the faces then face out), sides around, then turned by rot (a
    3x3 matrix or XYZ degrees) and moved to at. Mapping:
      grain 'around'  U round it at world scale, V along the profile (hoops, tyres, cans, the pump);
      grain 'up'      U along the profile, V round it within one lane (a turned post), squeezed if it's thicker;
      grain 'fit'     like 'up', the girth always spread over the lane (a bottle on the glass strip);
      grain 'staves'  like 'up', but each column of faces is its own board (lane and stretch): coopered staves.
    On a tileable set (key 'tin', 'brass', 'dirt') both run at world scale (density: texels per metre)."""
    rng = m.rng
    prof = [(float(r), float(z)) for r, z in profile]
    s = [0.0]
    for i in range(1, len(prof)):
        s.append(s[-1] + math.hypot(prof[i][0] - prof[i - 1][0], prof[i][1] - prof[i - 1][1]))
    if rot is None:
        matrix = Matrix.Identity(3)
    elif isinstance(rot, Matrix):
        matrix = rot.to_3x3()
    else:
        matrix = kit.place((0.0, 0.0, 0.0), rot).to_3x3()
    origin = Vector(at)
    set_name = set_name or SET_OF.get(key)
    tile = None
    if set_name:
        tile = (density or lt.SETS[set_name]['density']) / float(lt.SETS[set_name]['size'])
        off = (rng.uniform(0.0, 3.0), rng.uniform(0.0, 3.0))
    cos = []
    rings = []
    for r, z in prof:
        if r <= 1e-6:
            rings.append([len(cos)])
            cos.append(origin + matrix @ Vector((0.0, 0.0, z)))
            continue
        ring = []
        for k in range(sides):
            a = phase + 2.0 * math.pi * k / sides
            ring.append(len(cos))
            cos.append(origin + matrix @ Vector((math.cos(a) * r, math.sin(a) * r, z)))
        rings.append(ring)
    girth = 2.0 * math.pi * max(r for r, _ in prof)
    columns = {}
    if not tile:
        width = lane_width(strip)
        lane_i, u_start = pick_u(strip, s[-1] if grain != 'around' else girth, rng, lane)
        u_start = u_start if u0 is None else u0
        v_squeeze = min(1.0, (width - 0.03) / max(s[-1], 1e-6))

    r_max = max(r for r, _ in prof)

    def uv(i, k, column):
        r = prof[i][0]
        a = 2.0 * math.pi * k / sides
        # Round the part U runs at its widest girth on every ring, so a shoulder or a neck doesn't shear the texture.
        if tile:
            if grain == 'around':
                return ((a * r_max + off[0]) * tile, (s[i] + off[1]) * tile)
            return ((s[i] + off[0]) * tile, (a * max(r, 0.004) + off[1]) * tile)
        if grain == 'around':
            if v0 is not None:
                base = v0
            elif lanes(strip) > 1:
                base = lane_i * width + 0.015
            else:
                base = (width - s[-1] * v_squeeze) * 0.5       # the middle of a single-lane strip (iron, paint)
            return strip_uv(strip, u_start + a * r_max, base + s[i] * v_squeeze)
        if grain == 'fit':
            return strip_uv(strip, u_start + s[i], lane_i * width + 0.01 + (k / sides) * (width - 0.02))
        if grain == 'staves':
            if column not in columns:
                columns[column] = pick_u(strip, s[-1], rng)
            lane_c, u_c = columns[column]
            stave = girth / sides
            local = (k - column) * stave
            vv = lane_c * width + max(0.0, (width - stave) * 0.5) + min(local, width - 0.01)
            return strip_uv(strip, u_c + s[i], vv)
        squeeze = min(1.0, (width - 0.02) / girth)
        return strip_uv(strip, u_start + s[i], lane_i * width + (width - girth * squeeze) * 0.5 + (k / sides) * girth * squeeze)

    axis = matrix @ UP
    faces = []
    for i in range(len(rings) - 1):
        r0, r1 = rings[i], rings[i + 1]
        dr, dz = prof[i + 1][0] - prof[i][0], prof[i + 1][1] - prof[i][1]
        for k in range(sides):
            k1 = k + 1
            if len(r0) == 1 and len(r1) == 1:
                continue
            if len(r0) == 1:
                idx = [r0[0], r1[k], r1[k1 % sides]]
                uvs = [uv(i, k + 0.5, k), uv(i + 1, k, k), uv(i + 1, k1, k)]
            elif len(r1) == 1:
                idx = [r0[k], r0[k1 % sides], r1[0]]
                uvs = [uv(i, k, k), uv(i, k1, k), uv(i + 1, k + 0.5, k)]
            else:
                idx = [r0[k], r0[k1 % sides], r1[k1 % sides], r1[k]]
                uvs = [uv(i, k, k), uv(i, k1, k), uv(i + 1, k1, k), uv(i + 1, k, k)]
            # The profile's outward normal (dz, -dr) in the (radius, z) plane, at the face's middle angle.
            a = phase + 2.0 * math.pi * (k + 0.5) / sides
            radial = matrix @ Vector((math.cos(a), math.sin(a), 0.0))
            out = radial * dz - axis * dr
            mapping = dict(zip(idx, uvs))
            idx = _orient(cos, idx, out)
            faces.append((idx, [mapping[j] for j in idx]))
    m.put(cos, faces, key=key, smooth=smooth, ao_floor=ao_floor)


def hoop(m, at, radius, z0, z1, sides, proud=0.005, rot=None, strip='H3', key='trim'):
    """An iron hoop round a vessel: its lip, face and lip (no inside face), radius at its middle."""
    turned(m, [(radius - 0.004, z0), (radius + proud, z0 + 0.003), (radius + proud, z1 - 0.003), (radius - 0.004, z1)],
           sides, at=at, rot=rot, strip=strip, key=key, grain='around')


def vessel(m, at, r_bottom, r_top, height, staves, wall=0.018, floor=0.035, belly=0.0, hoops=(), water=None,
           strip='A', rot=None, bottom=False):
    """A coopered vessel open at the top (a bucket, a washtub): staves (each its own board), the rim, the inside down to
    its floor, iron hoops at the heights in hoops, and water (its height) on the dark glass strip."""
    def r_at(z):
        t = z / height
        return r_bottom + (r_top - r_bottom) * t + belly * math.sin(math.pi * t)
    zs = [0.0, height * 0.25, height * 0.5, height * 0.75, height]
    outer = [(r_at(z), z) for z in zs]
    inner = [(r_at(z) - wall, z) for z in reversed(zs) if z >= floor] + [(r_at(floor) - wall, floor), (0.0, floor)]
    profile = ([(0.0, 0.0)] if bottom else []) + outer + inner
    turned(m, profile, staves, at=at, rot=rot, strip=strip, grain='staves', phase=math.pi / staves)
    for z in hoops:
        hoop(m, at, r_at(z), z - 0.016, z + 0.016, staves, rot=rot)
    if water is not None:
        axis = (kit.place((0.0, 0.0, 0.0), rot).to_3x3() if rot is not None else Matrix.Identity(3)) @ UP
        disc(m, Vector(at) + axis * water, r_at(water) - wall + 0.002, staves, strip='H4', normal=axis)
    return r_at


def disc(m, center, radius, sides, strip='H4', normal=(0.0, 0.0, 1.0), key='trim', u0=None):
    """A flat round face (water in a vessel, a lid), mapped flat onto a strip, squeezed into it."""
    rng = m.rng
    n = Vector(normal).normalized()
    a_axis = n.cross(Vector((0.0, 1.0, 0.0))) if abs(n.y) < 0.9 else n.cross(X_AXIS)
    a_axis.normalize()
    b_axis = n.cross(a_axis)
    c = Vector(center)
    h = strip_height(strip)
    u_start = rng.uniform(0.0, 6.4) if u0 is None else u0
    squeeze = min(1.0, (h * 0.5 - 0.01) / max(radius, 1e-6))
    cos = [c]
    uvs = [strip_uv(strip, u_start, h * 0.5)]
    for k in range(sides):
        a = 2.0 * math.pi * (k + 0.5) / sides
        x, y = math.cos(a) * radius, math.sin(a) * radius
        cos.append(c + a_axis * x + b_axis * y)
        uvs.append(strip_uv(strip, u_start + x, h * 0.5 + y * squeeze))
    faces = []
    for k in range(sides):
        idx = _orient(cos, [0, 1 + k, 1 + (k + 1) % sides], n)
        faces.append((idx, [uvs[j] for j in idx]))
    m.put(cos, faces, key=key)


# --- Cloth and thin sheets ---

def cloth(m, rows, thick, strip='G', uv_of=None, key='trim', edges=False, facing=None, u0=None, v0=0.1, smooth=True):
    """A thin two-sided sheet through a grid of points (rows of the same length): each point moved half of thick to
    either side along the sheet's normal there, the two faces mapped alike at world scale on a strip (uv_of(j, i) gives
    (u, v) in metres; by default its distance along the rows and down the columns). facing: a direction the front
    should face (else as the grid winds). edges closes the rim (folded wool, a sack's seam)."""
    rng = m.rng
    R, C = len(rows), len(rows[0])
    P = [[Vector(p) for p in row] for row in rows]
    N = []
    for j in range(R):
        line = []
        for i in range(C):
            du = P[j][min(i + 1, C - 1)] - P[j][max(i - 1, 0)]
            dv = P[min(j + 1, R - 1)][i] - P[max(j - 1, 0)][i]
            n = du.cross(dv)
            line.append(n.normalized() if n.length > 1e-12 else Vector((0.0, -1.0, 0.0)))
        N.append(line)
    if facing is not None:
        total = sum((n for line in N for n in line), Vector())
        if total.dot(Vector(facing)) < 0.0:
            N = [[-n for n in line] for line in N]
    if uv_of is None:
        u_start = (pick_u(strip, 1.0, rng)[1] if u0 is None else u0)
        dist_u = [[0.0] * C for _ in range(R)]
        dist_v = [[0.0] * C for _ in range(R)]
        for j in range(R):
            for i in range(1, C):
                dist_u[j][i] = dist_u[j][i - 1] + (P[j][i] - P[j][i - 1]).length
        for i in range(C):
            for j in range(1, R):
                dist_v[j][i] = dist_v[j - 1][i] + (P[j][i] - P[j - 1][i]).length

        def uv_of(j, i):
            return u_start + dist_u[j][i], v0 + dist_v[j][i]
    cos = []
    top, bot = [], []
    for j in range(R):
        top.append([])
        bot.append([])
        for i in range(C):
            top[j].append(len(cos))
            cos.append(P[j][i] + N[j][i] * thick * 0.5)
            bot[j].append(len(cos))
            cos.append(P[j][i] - N[j][i] * thick * 0.5)
    faces = []

    def quad(a, b, c, d, out, uvs):
        idx = [a, b, c, d]
        mapping = dict(zip(idx, uvs))
        idx = _orient(cos, idx, out)
        faces.append((idx, [mapping[x] for x in idx]))
    for j in range(R - 1):
        for i in range(C - 1):
            n = N[j][i] + N[j + 1][i + 1]
            uvs = [strip_uv(strip, *uv_of(jj, ii)) for jj, ii in ((j, i), (j, i + 1), (j + 1, i + 1), (j + 1, i))]
            quad(top[j][i], top[j][i + 1], top[j + 1][i + 1], top[j + 1][i], n, uvs)
            quad(bot[j][i], bot[j][i + 1], bot[j + 1][i + 1], bot[j + 1][i], -n, uvs)
    if edges:
        rim = ([(0, i) for i in range(C)] + [(j, C - 1) for j in range(1, R)] +
               [(R - 1, i) for i in range(C - 2, -1, -1)] + [(j, 0) for j in range(R - 2, 0, -1)])
        center = sum((P[j][i] for j, i in rim), Vector()) / len(rim)
        for k in range(len(rim)):
            (j0, i0), (j1, i1) = rim[k], rim[(k + 1) % len(rim)]
            out = (P[j0][i0] + P[j1][i1]) * 0.5 - center
            uvs = [strip_uv(strip, *uv_of(j0, i0)), strip_uv(strip, *uv_of(j1, i1)),
                   strip_uv(strip, uv_of(j1, i1)[0], uv_of(j1, i1)[1] + thick),
                   strip_uv(strip, uv_of(j0, i0)[0], uv_of(j0, i0)[1] + thick)]
            quad(top[j0][i0], top[j1][i1], bot[j1][i1], bot[j0][i0], out, uvs)
    m.put(cos, faces, key=key, smooth=smooth)


def strap(m, centers, widths, normals, thick, strip='G', key='trim', u0=None, lane_v=0.3, across=None):
    """A thin strap (a leaf blade, a ribbon) along centers: widths across it (one per point), its broad side toward
    normals (a list: one per point; else one for all), or its width along across (one direction for all)."""
    fr = frames(centers)
    rows = []
    for i, (p, t, side, n) in enumerate(fr):
        if across is not None:
            a = Vector(across)
            a = a - t * a.dot(t)
        else:
            nn = Vector(normals[i]) if isinstance(normals, list) else Vector(normals)
            a = t.cross(nn)
        across_i = a.normalized() if a.length > 1e-9 else side
        w = widths[i] * 0.5
        rows.append([p - across_i * w, p + across_i * w])
    # The strap's length runs along the strip (U, which repeats), its width across it, centred on lane_v.
    along = [0.0]
    for i in range(1, len(fr)):
        along.append(along[-1] + (fr[i][0] - fr[i - 1][0]).length)
    u_start = pick_u(strip, along[-1], m.rng)[1] if u0 is None else u0
    cloth(m, rows, thick, strip=strip, key=key,
          uv_of=lambda j, i: (u_start + along[j], lane_v + (i - 0.5) * widths[j]))


# --- Lettering (looter_town's Rye) through any mapping ---

def glyph_mesh(body, tol=0.025):
    """A line of text as 2D triangles at 1 em: points, faces and the ink box."""
    loops = [town._simplify(loop, tol) for loop in town._glyph_loops(body)]
    loops = [loop for loop in loops if len(loop) >= 3 and town._area(loop) > (tol * 0.6) ** 2]
    points, faces = town._fill(loops)
    return points, faces, town.text_extent(body)


def lettering(m, body, height, mapper, strip='G', width=None, tol=0.025, u0=None, v0=0.3, key='trim',
              align='center'):
    """Painted lettering laid through mapper(x, y) -> (point, normal): x along the line and y up it, in metres from
    the ink's middle (align 'left': from its left end). Faces turn to mapper's normal; their UVs lie on a strip (paint
    of that strip's colour). Returns the ink's (width, height)."""
    points, faces, (x0, y0, x1, y1) = glyph_mesh(body, tol)
    ink_w, ink_h = x1 - x0, y1 - y0
    size = height / ink_h
    if width:
        size = min(size, width / ink_w)
    cx = x0 if align == 'left' else (x0 + x1) * 0.5
    cy = (y0 + y1) * 0.5
    u_start = (pick_u(strip, ink_w * size, m.rng)[1] if u0 is None else u0)
    cos, normals, uvs = [], [], []
    for x, y in points:
        lx, ly = (x - cx) * size, (y - cy) * size
        p, n = mapper(lx, ly)
        cos.append(p)
        normals.append(Vector(n))
        uvs.append(strip_uv(strip, u_start + lx, v0 + ly + ink_h * size * 0.5))
    out = []
    for face in faces:
        n = sum((normals[i] for i in face), Vector())
        idx = _orient(cos, list(face), n)
        out.append((idx, [uvs[i] for i in idx]))
    m.put(cos, out, key=key, smooth=False)
    return ink_w * size, ink_h * size


def plane_mapper(origin, right, up, lift=0.0015):
    """A mapper onto a flat face: origin the text's middle, right and up its directions."""
    o, r, u = Vector(origin), Vector(right).normalized(), Vector(up).normalized()
    n = r.cross(u).normalized()

    def mapper(x, y):
        return o + r * x + u * y + n * lift, n
    return mapper


# --- Boards with a shape ---

def plank(m, p0, p1, width, thick, face=(0.0, -1.0, 0.0), uv='A', sag=0.0, cuts=4, twist=0.0, bevel=0.0):
    """A board as kit Model.board makes it, sagging by sag in the middle (toward the back of its broad side, -face) and
    twisted by twist (degrees, end to end)."""
    length = (Vector(p1) - Vector(p0)).length
    matrix = kit.toward(p0, p1, face) @ Matrix.Translation((length * 0.5, 0.0, 0.0))

    def shape(co):
        t = co.x / max(length * 0.5, 1e-6)
        co = Vector((co.x, co.y + sag * (1.0 - t * t), co.z))
        if twist:
            a = math.radians(twist) * t * 0.5
            co = Vector((co.x, co.y * math.cos(a) - co.z * math.sin(a), co.y * math.sin(a) + co.z * math.cos(a)))
        return co
    m.emit(kit._box((length, thick, width), bevel, cuts), kit._spec(uv, width=max(width, thick)), 'trim', matrix, None,
           shape=shape)


def prism_part(m, outline, depth, matrix, uv='A', key='trim'):
    """A flat piece (outline (x, z) counterclockwise from the front, then holes as further loops), its front facing -Y,
    depth thick toward +Y, placed by matrix."""
    loops = outline if isinstance(outline[0][0], (list, tuple)) else [outline]
    m.emit(kit._prism([list(l) for l in loops], depth), kit._spec(uv) if isinstance(uv, str) else uv, key, matrix, None)


# --- Previews: Ransom's Rest's golden late afternoon ---

SUN_OBJ = []


def stage(samples=48):
    scene, sun = homes.stage(samples=samples)
    SUN_OBJ.append(sun)
    return scene


def frame(objs, direction, size, lens, margin=1.1, bias=0.45):
    """An eye and a target framing objs from direction (toward the camera); small things fill the frame too."""
    pts = homes.points_of(objs)
    lo = Vector((min(p.x for p in pts), min(p.y for p in pts), min(p.z for p in pts)))
    hi = Vector((max(p.x for p in pts), max(p.y for p in pts), max(p.z for p in pts)))
    target = Vector(((lo.x + hi.x) * 0.5, (lo.y + hi.y) * 0.5, lo.z + (hi.z - lo.z) * bias))
    d = Vector(direction).normalized()
    tan_x = 18.0 / lens
    tan_y = tan_x * size[1] / size[0]
    forward = -d
    right = forward.cross(UP).normalized()
    up = right.cross(forward).normalized()
    dist = 0.05
    for p in pts:
        rel = p - target
        x, y, z = rel.dot(right), rel.dot(up), rel.dot(forward)
        dist = max(dist, abs(x) * margin / tan_x - z, abs(y) * margin / tan_y - z)
    return target + d * dist, target


def show(keep):
    keep = set(keep)
    for o in bpy.context.scene.objects:
        if o.type in ('MESH', 'CURVE', 'FONT'):
            visible = o.name == '_Ground' or o in keep or (o.parent in keep and not o.name.startswith('UCX_'))
            o.hide_render = not visible
        if o.name.startswith('UCX_'):
            o.hide_render = True


def shoot(name, objs, extra=(), view=(-0.6, -1.0, 0.5), sun=(38.0, 22.0), lens=50.0, size=(1500, 1000),
          margin=1.12, bias=0.42, exposure=-0.1, eye=None, target=None):
    scene = bpy.context.scene
    show(list(objs) + list(extra))
    homes.light(scene, SUN_OBJ[0], homes.golden_sun(*sun))
    if eye is None:
        eye, target = frame(list(objs), view, size, lens, margin, bias)
    return homes.shoot(scene, os.path.join(PREVIEW_DIR, name + '.png'), eye, target, lens, size, exposure)


def copy_of(obj, at=(0.0, 0.0, 0.0), turn=0.0, tilt=(0.0, 0.0)):
    """Another object showing obj's mesh for staging (removed by clear_copies), its sockets' empties left behind."""
    other = bpy.data.objects.new('_Copy_' + obj.name, obj.data)
    bpy.context.scene.collection.objects.link(other)
    other.location = at
    other.rotation_euler = (math.radians(tilt[0]), math.radians(tilt[1]), math.radians(turn))
    other['_staged'] = True
    return other


def clear_copies():
    for o in [o for o in bpy.context.scene.objects if o.get('_staged')]:
        bpy.data.objects.remove(o)


def import_fbx(path):
    """A model from an export folder for staging: its hulls dropped, its materials the session's own by name (their
    textures), its occlusion in 'Col'. Returns it (named '_Set_<name>'), or None if the file isn't there."""
    if not os.path.exists(path):
        lt._log(f'CLUTTER: no {path}; the vignette goes without it')
        return None
    before = set(bpy.data.objects)
    bpy.ops.import_scene.fbx(filepath=path)
    new = [o for o in bpy.data.objects if o not in before]
    roots = []
    for o in new:
        if o.type != 'MESH' or o.name.startswith(('UCX_', 'SOCKET_')):
            bpy.data.objects.remove(o)
            continue
        roots.append(o)
    known = {'HouseTrim': lambda: lt.material('HouseTrim')}
    for o in roots:
        o.parent = None
        mesh = o.data
        if mesh.color_attributes and 'Col' not in mesh.color_attributes:
            mesh.color_attributes[0].name = 'Col'
        for i, mat in enumerate(mesh.materials):
            if mat is None:
                continue
            base = mat.name.split('.')[0]
            if base.startswith('MI_'):
                base = base[3:]
            if base in bpy.data.materials and bpy.data.materials[base] is not mat and bpy.data.materials[base].get('Master'):
                mesh.materials[i] = bpy.data.materials[base]
            elif base in known:
                mesh.materials[i] = known[base]()
            elif base in town.MATERIALS or base.lower() in town.MATERIALS:
                pass
        bpy.context.view_layer.update()
        basis = o.matrix_world.copy()
        basis.translation = (0.0, 0.0, 0.0)
        mesh.transform(basis)
        o.matrix_world = Matrix.Identity(4)
        if max(o.dimensions) > 60.0:
            mesh.transform(Matrix.Scale(0.01, 4))
        mesh.update()
        o.name = '_Set_' + o.name
    bpy.context.view_layer.update()
    return roots[0] if roots else None


def town_materials():
    """Makes every shared material an imported building may name, so import_fbx can put the textured ones on it."""
    for key in town.MATERIALS:
        try:
            town.material(key)
        except Exception:
            pass
    homes.EXTRA['windowglow']()
    homes.EXTRA['canvas']()
    for key in MAKERS:
        material(key)


class Surface:
    """Ray casts against staged objects (their meshes in the world), to set props down on a deck or against a wall."""

    def __init__(self, objs):
        verts, polys = [], []
        for o in objs:
            if o is None:
                continue
            base = len(verts)
            verts.extend(o.matrix_world @ v.co for v in o.data.vertices)
            polys.extend([base + i for i in p.vertices] for p in o.data.polygons)
        self.tree = BVHTree.FromPolygons(verts, polys, epsilon=0.0) if polys else None

    def down(self, x, y, start=8.0, default=0.0):
        """The height of the first surface under (x, y) from start metres up."""
        if self.tree is None:
            return default
        hit, _, _, _ = self.tree.ray_cast(Vector((x, y, start)), Vector((0.0, 0.0, -1.0)), start + 5.0)
        return hit.z if hit is not None else default

    def toward(self, origin, direction, reach=10.0):
        if self.tree is None:
            return None
        hit, normal, _, _ = self.tree.ray_cast(Vector(origin), Vector(direction).normalized(), reach)
        return (hit, normal) if hit is not None else None
