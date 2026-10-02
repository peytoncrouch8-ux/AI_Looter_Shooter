"""Shared parts for the Ransom's Rest station: Art/Models/Buildings/Depot.py (the depot, its platform kit, the water tower
and the coffin shed) and Art/Models/Props/Railway.py (the track kit, the buffer stop, the signal and the station board).
Everything else comes from looter_buildings (kit), so a script does:

    import looter_buildings as kit
    import looter_rail as rail

    m = rail.Model('Depot', seed=3)          # a looter_buildings Model that also knows the paints and the ballast
    m.board(p0, p1, 0.19, 0.05, uv=rail.Planks(), mat='planks')       # one plank of WoodPlanks
    rail.lettering(m, "RANSOM'S REST", 0.3, space=front, at=(x, -0.03, z))  # Rye lettering, a hair proud of a board
    rail.lantern(m, (x, y, z))               # an iron lantern with lit glass and SOCKET_Light

Material keys: looter_buildings' own ('trim' HouseTrim, 'planks' WoodPlanks, 'endgrain' WoodEndGrain, 'metal'
MetalRust, 'stone' StoneWall, 'glow' LanternGlow) and the shared names of the Ransom's Rest art brief ('cream'
PaintCream, 'oxide' PaintOxide, 'black' PaintBlack, 'ironblack' IronBlack, and for large painted wood 'woodteal',
'woodcream', 'woodblack', 'woodoxide'), plus 'ballast': Ballast, this family's track bed of cinders and pebbles (the
GroundDirt set, greyed).

Helpers: Planks (a UV spec: one plank of WoodPlanks, joint-free where the part is short enough), CleanPaint (lettering
on a chip-free patch of PaintWorn), lettering() (Rye, simplified until no letter folds), lantern(), fill() (n-gons cut
into triangles before Model.emit), socket_at() and preview_path().
"""
import math

import bmesh
import bpy
from mathutils import Matrix, Vector

import looter_buildings as kit
import looter_textures as lt
from looter_buildings import Trim

# Sign lettering: the Western display font Rye (SIL Open Font License, the licence is beside it). Georgia Bold stands in
# if it can't be loaded.
RYE = 'C:/Dev/AI_Looter_Shooter/Art/Fonts/Rye-Regular.ttf'
FALLBACK_FONT = 'C:/Windows/Fonts/georgiab.ttf'

# The brief's shared material names, made exactly as every other family makes them (same set, name and tint), and
# the ballast. Never give one of these names another tint.
PAINTS = {
    'cream': lambda: lt.material('PaintWorn', name='PaintCream', tint=0xe6dac2),
    'oxide': lambda: lt.material('PaintWorn', name='PaintOxide', tint=0x8c3e2c),
    'black': lambda: lt.material('PaintWorn', name='PaintBlack', tint=0x2a2622),
    'ironblack': lambda: lt.material('MetalWorn', name='IronBlack', tint=0x2e2c2a),
    # Large painted wood (PaintWorn's chips read as spots on big panels): paint over planks, the grain showing. The
    # brief's final tints (looter_train.WOOD_TINTS holds the same values).
    'woodteal': lambda: lt.material('WoodPlanks', name='WoodTeal', tint=0x7cbcc4),
    'woodcream': lambda: lt.material('WoodPlanks', name='WoodCream', tint=0xfffcf6),
    'woodblack': lambda: lt.material('WoodPlanks', name='WoodBlack', tint=0x48423c),
    'woodoxide': lambda: lt.material('WoodPlanks', name='WoodOxide', tint=0xbc7462),
    # The track's bed of cinders and pebbles: the dirt set, greyed.
    'ballast': lambda: lt.material('GroundDirt', name='Ballast', tint=0x9c968e),
}
PLANK_ROWS = 16         # WoodPlanks: 16 planks of 20 cm stacked along V, each running along U (3.2 m repeat)
IRON_SET = 'MetalWorn'  # IronBlack's texture set (map 'ironblack' parts with kit.Tile(IRON_SET))


class Model(kit.Model):
    """A looter_buildings Model whose parts may also use the paint and ballast materials (see the module's
    docstring)."""

    def slot(self, key):
        if key not in self.slots:
            material = PAINTS[key]() if key in PAINTS else kit._material(key)
            self.slots[key] = (len(self.slots), material)
        return self.slots[key][0]


# --- UV specs (looter_buildings parts are built along their own X, then placed) ---

class Planks:
    """A part on WoodPlanks: the grain (U) runs along the part's own X, at world scale, and every face stays inside
    one of the texture's 16 plank rows (row, or one picked at random), so a board shows one plank with its nails,
    never the gap between two. A part shorter than a plank's longest unbroken stretch is laid inside one, so it reads
    as one piece of timber (no butt joint across a tie or a post). A face wider than a plank is squeezed to fit."""

    def __init__(self, row=None, u=None):
        self.row, self.u = row, u

    def apply(self, obj, rng):
        info = lt.SETS['WoodPlanks']
        scale = info['density'] / float(info['size'])         # UV units per metre
        repeat = info['size'] / info['density']                # 3.2 m
        mesh = obj.data
        xs = [v.co.x for v in mesh.vertices]
        x_min, length = min(xs), max(xs) - min(xs)
        rows = [self.row] if self.row is not None else list(range(PLANK_ROWS))

        def clear(candidates):
            found = []
            for r in candidates:
                joints = sorted(lt.PLANK_JOINTS[r % PLANK_ROWS]) or [0.0]
                for a, b in zip(joints, joints[1:] + [joints[0] + repeat]):
                    if b - a >= length + 0.1:
                        found.append((r, a, b))
            return found
        stretches = clear(rows)
        if not stretches and self.row is not None:
            stretches = clear(range(PLANK_ROWS))   # an unbroken board matters more than the row asked for
        if self.u is not None:
            row, u0 = rows[0] if self.row is not None else rng.randrange(PLANK_ROWS), self.u
        elif stretches:
            row, a, b = rng.choice(stretches)
            u0 = a + 0.05 + rng.uniform(0.0, b - a - length - 0.1) - x_min
        else:
            row, u0 = rng.choice(rows), rng.uniform(0.0, repeat)
        inset = 2.0 / info['size']
        span = 1.0 / PLANK_ROWS - 2.0 * inset
        base = (row % PLANK_ROWS) / float(PLANK_ROWS) + inset
        mesh = obj.data
        uv = (mesh.uv_layers.active or mesh.uv_layers.new(name='UVMap')).data
        x = Vector((1.0, 0.0, 0.0))
        for p in mesh.polygons:
            n = p.normal
            if abs(n.dot(x)) > 0.7:
                # An end: its own plane, the longer side along U.
                ua = Vector((0.0, 1.0, 0.0)) if abs(n.y) < 0.7 else Vector((0.0, 0.0, 1.0))
                ua = (ua - n * n.dot(ua)).normalized()
            else:
                ua = (x - n * n.dot(x)).normalized()
            va = n.cross(ua)
            cos = [mesh.vertices[mesh.loops[i].vertex_index].co for i in p.loop_indices]
            ts = [c.dot(va) for c in cos]
            t0, width = min(ts), max(ts) - min(ts)
            squeeze = min(1.0, span / max(width * scale, 1e-6))
            for i, c in zip(p.loop_indices, cos):
                uv[i].uv = ((c.dot(ua) + u0) * scale, base + (c.dot(va) - t0) * scale * squeeze)


def fill(tb):
    """Cuts a part's n-gons into triangles with their normals right, before Model.emit: looter_buildings triangulates
    the whole model without updating normals first, which turns parts of a concave n-gon over (a rail's end, a
    letter)."""
    tb.normal_update()
    bmesh.ops.triangulate(tb, faces=[f for f in tb.faces if len(f.verts) > 4], quad_method='BEAUTY',
                          ngon_method='BEAUTY')
    return tb


# --- Lettering ---

def _font():
    try:
        return bpy.data.fonts.load(RYE, check_existing=True)
    except (RuntimeError, OSError):
        print(f'LOOTER: warning: could not load {RYE}; lettering falls back to {FALLBACK_FONT}', flush=True)
        return bpy.data.fonts.load(FALLBACK_FONT, check_existing=True)


def _text_bmesh(body, height, width=None, simplify=25.0, align='CENTER'):
    """Flat lettering in the XZ plane facing -Y: its cap height `height` (shrunk to fit `width` if given), centered on
    the origin (align 'LEFT' puts its left edge there). Rye's curves at the lowest resolution, and outline points on
    nearly straight runs (turning less than `simplify` degrees) dissolved: about 50 triangles a letter."""
    curve = bpy.data.curves.new('_text', 'FONT')
    curve.body, curve.font, curve.size = body, _font(), 1.0
    curve.resolution_u = 1
    curve.align_x, curve.align_y = 'CENTER', 'CENTER'
    src = bpy.data.objects.new('_text', curve)
    bpy.context.scene.collection.objects.link(src)
    bpy.context.view_layer.update()
    mesh = bpy.data.meshes.new_from_object(src.evaluated_get(bpy.context.evaluated_depsgraph_get()))
    bpy.data.objects.remove(src)
    bpy.data.curves.remove(curve)
    base = bmesh.new()
    base.from_mesh(mesh)
    bpy.data.meshes.remove(mesh)
    # Dissolving outline points can fold a letter over itself (a fold shows as a hole); a fold turns triangles over,
    # so try again gentler until none is.
    for angle in (simplify, simplify * 0.7, simplify * 0.45, simplify * 0.25, 0.0):
        tb = base.copy()
        if angle > 0.0:
            bmesh.ops.dissolve_limit(tb, angle_limit=math.radians(angle), use_dissolve_boundaries=False,
                                     verts=tb.verts[:], edges=tb.edges[:])
            # Cut the letters into triangles here, while their normals are right: looter_buildings triangulates a
            # model without updating its normals first, and a concave letter cut that way comes out turned over.
            tb.normal_update()
            bmesh.ops.triangulate(tb, faces=tb.faces[:], quad_method='BEAUTY', ngon_method='BEAUTY')
        tb.normal_update()
        if angle <= 0.0 or not any(f.normal.z < 0.0 for f in tb.faces):
            break
        tb.free()
    base.free()
    xs = [v.co.x for v in tb.verts]
    ys = [v.co.y for v in tb.verts]
    w, h = max(xs) - min(xs), max(ys) - min(ys)
    s = height / max(h, 1e-6)
    if width is not None and w * s > width:
        s = width / w
    cx = min(xs) if align == 'LEFT' else (min(xs) + max(xs)) * 0.5
    cy = (min(ys) + max(ys)) * 0.5
    # Into the XZ plane, the letters' faces toward -Y.
    tb.transform(Matrix.Rotation(math.radians(90.0), 4, 'X') @ Matrix.Scale(s, 4) @ Matrix.Translation((-cx, -cy, 0.0)))
    for face in tb.faces:
        face.normal_update()
        if face.normal.y > 0.0:
            face.normal_flip()
    if not tb.loops.layers.uv:
        tb.loops.layers.uv.new('UVMap')
    return tb, w * s


class CleanPaint:
    """A part (lettering) mapped onto one small chip-free patch of PaintWorn, so its paint reads clean and even: the
    texture's chips are as big as a letter and would break it (the board under the letters carries the wear)."""
    SPOT, SCALE = (0.1562, 0.5938), 0.01     # found by scanning T_PaintWorn_BC for its most even 64 px window

    def apply(self, obj, rng):
        mesh = obj.data
        uv = (mesh.uv_layers.active or mesh.uv_layers.new(name='UVMap')).data
        u0, v0 = self.SPOT
        for loop in mesh.loops:
            co = mesh.vertices[loop.vertex_index].co
            uv[loop.index].uv = (u0 + co.x * self.SCALE, v0 + co.z * self.SCALE)


def lettering(m, body, height, at=(0.0, 0.0, 0.0), space=None, rot=(0.0, 0.0, 0.0), width=None, mat='cream',
              simplify=25.0, align='CENTER'):
    """Painted lettering in Rye on a sign: flat letters facing -Y in `space` (a wall's space faces out), their middle
    at `at` (put it 3 mm in front of the board). Returns the lettering's width."""
    tb, w = _text_bmesh(body, height, width, simplify, align)
    m.emit(tb, CleanPaint(), mat, kit.place(at, rot), space)
    return w


# --- Iron and lamps on the trim sheet (no metal material needed) ---

IRON = Trim('H3', fit=True)


def lantern(m, at, size=0.2, space=None):
    """A square iron lantern with lit glass, the glass's middle at `at`: a hipped cap with a ring on top, corner bars,
    a base, all in the trim sheet's iron (H3), and the glass in LanternGlow. Adds SOCKET_Light at the glass."""
    x, y, z = at
    s = size
    m.box((s * 0.7, s * 0.7, s * 1.15), at=(x, y, z), uv=Trim('H4', fit=True), mat='glow', space=space)
    m.box((s * 1.1, s * 1.1, s * 0.2), at=(x, y, z + s * 0.67), uv=IRON, space=space, bevel=0.008)
    roof = kit._new_bmesh()
    corners = [roof.verts.new((sx * s * 0.55, sy * s * 0.55, 0.0)) for sx, sy in ((-1, -1), (1, -1), (1, 1), (-1, 1))]
    tip = roof.verts.new((0.0, 0.0, s * 0.42))
    for a, b in zip(corners, corners[1:] + corners[:1]):
        roof.faces.new((a, b, tip))
    m.emit(roof, IRON, 'trim', kit.place((x, y, z + s * 0.77)), space)
    m.box((s * 0.16, s * 0.05, s * 0.3), at=(x, y, z + s * 1.27), uv=IRON, space=space)
    for dx in (-1.0, 1.0):
        for dy in (-1.0, 1.0):
            m.box((s * 0.12, s * 0.12, s * 1.25), at=(x + dx * s * 0.38, y + dy * s * 0.38, z), uv=IRON, space=space)
    m.box((s * 1.0, s * 1.0, s * 0.18), at=(x, y, z - s * 0.66), uv=IRON, space=space, bevel=0.006)
    m.box((s * 0.4, s * 0.4, s * 0.16), at=(x, y, z - s * 0.82), uv=IRON, space=space)
    point = space.world(Vector(at)) if space is not None else Vector(at)
    m.socket('Light', tuple(point))


def socket_at(m, name, at, space=None, rotation=(0.0, 0.0, 0.0)):
    """A socket at a point given in a space (a wall's: its front, -Y, faces out of the wall). The socket turns with the
    space about Z only, so its top stays up."""
    point = space.world(Vector(at)) if space is not None else Vector(at)
    turn = 0.0
    if space is not None:
        front = space.matrix.to_3x3() @ Vector((0.0, -1.0, 0.0))
        turn = math.degrees(math.atan2(front.x, -front.y))
    m.socket(name, tuple(point), (rotation[0], rotation[1], rotation[2] + turn))


# --- Previews ---

def preview_path(family, name):
    """Saved/ArtPreviews/RansomsRest/<family>/<name>.png (props and kits; buildings use looter_buildings' own)."""
    return lt.preview_path('RansomsRest/' + family, name)
