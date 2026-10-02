"""Helpers for the town on Ransom's Rest: the false-front buildings of Main Street (Art/Models/Buildings/FalseFronts.py)
and the boardwalk kit (Art/Models/Props/Boardwalk.py). It sits on looter_buildings (kit) and adds what a frontier
street needs beside the house trim sheet:

  - the shared paint, brass, iron and crepe materials (Model, finish names such as 'cream', 'black', 'brass', 'crepe');
  - boards on WoodPlanks, each mapped across exactly one of the texture's planks (PlankRow);
  - Rye lettering as low-poly painted geometry a hair proud of its board (text);
  - false-front outlines, clapboards, pilasters, bracketed cornices, sign boards, windows and doors in any finish,
    boarding-up, iron bars, mourning crepe;
  - the plank deck that the boardwalk pieces and the store's porch share (deck), so they read as one walk.

A finish names a look: a strip of the house trim sheet ('A' siding, 'C' beams, 'D' stone, 'E' shakes, 'G' the cream
plaster strip, 'H1' teal, 'H2' oxide red, 'H3' iron strap, 'H4' glass; 'band_G' maps a part from a random height in
the strip), 'planks' or a painted wood ('woodblack', 'woodteal', ...: boards up to 26 cm wide lie across one plank,
wider parts take the planks at world scale), or a small-part finish in FINISHES ('cream', 'black' paint for lettering
and trim, 'brass', 'iron', 'crepe'). Parts are placed in a kit Space (a wall: x along it, z up, the outside toward
-y). Everything random comes from the model's seeded rng, so a script gives the same mesh on every run.

    import looter_town as town
    m = town.Model('FalseFront_Store', seed=5)
    front = kit.wall_space((-4.5, -6.5), (4.5, -6.5), 0.38)
    town.text(m, "PRUITT'S", (4.5, -0.06, 4.6), height=0.55, space=front, look='cream')
"""
import math
import os

import bmesh
import bpy
from mathutils import Matrix, Vector
from mathutils.geometry import tessellate_polygon

import looter_buildings as kit
import looter_textures as lt
from looter_buildings import Opening, Space, Tile, Trim, place

# Sign lettering: the Western display face Rye (SIL Open Font License, licence beside it), Georgia Bold if it fails.
FONT = os.path.join(lt.REPO, 'Art', 'Fonts', 'Rye-Regular.ttf').replace('\\', '/')
FALLBACK_FONT = 'C:/Windows/Fonts/georgiab.ttf'

DECK_TOP = 0.38          # the boardwalk's walking surface: under the character's step height
PLANK = 0.2              # one plank of WoodPlanks (16 across its 3.2 m), and the deck's plank pitch
WALK_DEPTH = 3.0         # the boardwalk from a building's front to the street
GLASS = ('+y', '-x', '+x', '-z', '+z')   # a pane shows only its face: its edges and back are in the frame

# --- Materials ---

# The shared material names of the Ransom's Rest brief: one name, one look, in every model that uses it. The Paint*
# names (PaintWorn) are for small parts only: on large panels their chips read as spots. Large painted wood takes the
# Wood* names (WoodPlanks tinted; a tint can only darken the mid-brown set, so WoodCream is bleached bare wood). True
# cream on a large face comes from the house trim's cream strip G (finish 'band_G').
MATERIALS = {
    'trim': lambda: lt.material('HouseTrim'),
    'planks': lambda: lt.material('WoodPlanks'),
    'metal': lambda: lt.material('MetalRust'),
    'stone': lambda: lt.material('StoneWall'),
    'endgrain': lambda: lt.material('WoodEndGrain'),
    'glow': lambda: kit._material('glow'),
    'cream': lambda: lt.material('PaintWorn', name='PaintCream', tint=0xe6dac2),
    'black': lambda: lt.material('PaintWorn', name='PaintBlack', tint=0x2a2622),
    'teal': lambda: lt.material('PaintWorn', name='PaintTeal', tint=0x41706a),
    'oxide': lambda: lt.material('PaintWorn', name='PaintOxide', tint=0x8c3e2c),
    'woodteal': lambda: lt.material('WoodPlanks', name='WoodTeal', tint=0x7cbcc4),
    'woodcream': lambda: lt.material('WoodPlanks', name='WoodCream', tint=0xfffcf6),
    'woodblack': lambda: lt.material('WoodPlanks', name='WoodBlack', tint=0x48423c),
    'woodoxide': lambda: lt.material('WoodPlanks', name='WoodOxide', tint=0xbc7462),
    'crepe': lambda: lt.material('Polymer', name='MourningCrepe', tint=0x161518),
    'brass': lambda: lt.material('MetalWorn', name='BrassWorn', tint=0xc49c56),
    'iron': lambda: lt.material('MetalWorn', name='IronBlack', tint=0x2e2c2a),
    'hay': lambda: lt.material('Hay'),
    'foliage': lambda: lt.material('FoliagePalette'),
}
# Finishes on a tileable set (world-scale box mapping): the finish name is also the material key.
FINISHES = {'cream': 'PaintWorn', 'black': 'PaintWorn', 'teal': 'PaintWorn', 'oxide': 'PaintWorn',
            'woodteal': 'WoodPlanks', 'woodcream': 'WoodPlanks', 'woodblack': 'WoodPlanks', 'woodoxide': 'WoodPlanks',
            'planks': 'WoodPlanks', 'brass': 'MetalWorn', 'iron': 'MetalWorn', 'crepe': 'Polymer', 'hay': 'Hay',
            'metal': 'MetalRust', 'stone': 'StoneWall'}
WOODS = ('woodteal', 'woodcream', 'woodblack', 'woodoxide', 'planks')


def material(key):
    return MATERIALS[key]()


class Model(kit.Model):
    """A kit Model that also knows the town's materials (MATERIALS keys)."""

    def slot(self, key):
        if key not in self.slots:
            self.slots[key] = (len(self.slots), material(key))
        return self.slots[key][0]


class PlankRow:
    """A board on WoodPlanks, mapped in the part's own frame as kit boards are built (length along X, the broad side
    facing -Y, its width along Z): the grain runs along it and its width spans exactly one of the texture's 16 planks,
    so the painted gaps fall on the board's edges. row picks the plank (random by default)."""

    def __init__(self, row=None):
        self.row = row

    def apply(self, obj, rng):
        mesh = obj.data
        info = lt.SETS['WoodPlanks']
        scale = info['density'] / float(info['size'])        # UV units per meter
        rows = int(round(info['size'] / (info['density'] * PLANK)))
        row = rng.randrange(rows) if self.row is None else self.row
        u0 = rng.uniform(0.0, 1.0)
        v0, v1 = row / rows, (row + 1) / rows
        inset = 0.5 / info['size']
        cos = [v.co for v in mesh.vertices]
        y_lo, y_hi = min(c.y for c in cos), max(c.y for c in cos)
        z_lo, z_hi = min(c.z for c in cos), max(c.z for c in cos)
        uv = mesh.uv_layers.active.data
        for p in mesh.polygons:
            n = p.normal
            for li in p.loop_indices:
                co = mesh.vertices[mesh.loops[li].vertex_index].co
                tz = (co.z - z_lo) / max(z_hi - z_lo, 1e-6)
                ty = (co.y - y_lo) / max(y_hi - y_lo, 1e-6)
                if abs(n.y) >= max(abs(n.x), abs(n.z)):           # the broad faces
                    u, t = u0 + co.x * scale, tz
                elif abs(n.z) >= abs(n.x):                         # the long edges
                    u, t = u0 + co.x * scale, 0.42 + 0.16 * ty
                else:                                              # the ends
                    u, t = u0 + co.y * scale, tz
                uv[li].uv = (u, v0 + inset + (v1 - v0 - 2.0 * inset) * t)


class Band(Trim):
    """A part on a trim strip without lanes (G, the cream plaster strip), mapped face-aligned from a random height in
    the strip, so stacked boards don't repeat."""

    def apply(self, obj, rng):
        room = kit.strip_height(self.strip) - (self.width or 0.25) - 0.01
        self.v = rng.uniform(0.0, max(room, 0.0))
        Trim.apply(self, obj, rng)


def finish(name, width=None, board=False):
    """(uv spec, material key) for a finish name (see the module's docstring): a trim strip, 'band_<strip>' (a strip
    mapped from a random height), 'planks' or a Wood* name ('board_<name>' maps a board across one plank; board=True
    does so for boards up to 26 cm wide), or a tileable finish in FINISHES."""
    if name.startswith('board_'):
        return PlankRow(), name[6:]
    if board and name in WOODS and (width is None or width <= 0.26):
        return PlankRow(), name
    if name.startswith('band_'):
        return Band(name[5:], width=width), 'trim'
    if name.startswith('world_'):
        return Trim(name[6:], world=True), 'trim'
    if name == 'glow':
        return Trim('H4', fit=True), 'glow'
    if name in FINISHES:
        return Tile(FINISHES[name]), name
    if name in ('H3', 'H4'):
        return Trim(name, fit=True), 'trim'
    return kit._spec(name, width=width), 'trim'


# --- Parts in a finish ---

def box(m, size, at=(0.0, 0.0, 0.0), rot=(0.0, 0.0, 0.0), look='C', space=None, matrix=None, drop=(), bevel=0.0,
        cuts=0, ao_floor=0.0):
    """A box of size (x, y, z) in a finish (see kit Model.box)."""
    spec, key = finish(look, width=max(size[1], size[2]))
    m.emit(kit._box(size, bevel, cuts, drop), spec, key, matrix if matrix is not None else place(at, rot), space,
           ao_floor=ao_floor)


def board(m, p0, p1, width, thick, look='A', face=(0.0, -1.0, 0.0), space=None, lift=0.0, drop=(), cuts=None,
          bevel=0.0):
    """A board from p0 to p1 (its middle line), width across, thick deep, its broad side toward face (see kit Model.board);
    drop leaves out sides nobody sees ('+y' is the back of the broad side)."""
    length = (Vector(p1) - Vector(p0)).length
    if cuts is None:
        cuts = int(length / 2.5)
    matrix = kit.toward(p0, p1, face) @ Matrix.Translation((length * 0.5, -lift, 0.0))
    spec, key = finish(look, width=max(width, thick), board=True)
    m.emit(kit._box((length, thick, width), bevel, cuts, drop), spec, key, matrix, space)


def _prism(loops, depth):
    """kit._prism, made repeatable: a flat piece from loops of (x, z) points (the outline, then any holes), its front
    at y = 0 facing -Y, depth thick toward +Y. The kit's version hands beautify_fill the fill's inner edges from a set of
    mesh edges, which iterates in memory-address order (different on every run), and the fill comes out different for
    a different order, so the same source exported a different mesh each time. Here they go in mesh order."""
    tb = kit._new_bmesh()
    points = [p for loop in loops for p in loop]
    triangles = tessellate_polygon([[Vector((x, z, 0.0)) for x, z in loop] for loop in loops])
    front = [tb.verts.new((x, 0.0, z)) for x, z in points]
    back = [tb.verts.new((x, depth, z)) for x, z in points]
    for a, b, c in triangles:
        for face in ((front[a], front[b], front[c]), (back[c], back[b], back[a])):
            try:
                tb.faces.new(face)
            except ValueError:
                pass
    start = 0
    for loop in loops:
        n = len(loop)
        for i in range(n):
            j = (i + 1) % n
            tb.faces.new((front[start + i], front[start + j], back[start + j], back[start + i]))
        start += n
    bmesh.ops.recalc_face_normals(tb, faces=tb.faces[:])
    # As in the kit: corners in a line can come out of the fill as flat triangles, so the edges between the fill's
    # triangles are turned until every triangle has some shape.
    triangles = [f for f in tb.faces if len(f.verts) == 3]
    inner, seen = [], set()
    for f in triangles:
        for e in f.edges:
            if e not in seen and len(e.link_faces) == 2 and all(len(g.verts) == 3 for g in e.link_faces):
                seen.add(e)
                inner.append(e)
    bmesh.ops.beautify_fill(tb, faces=triangles, edges=inner, method='ANGLE')
    bmesh.ops.join_triangles(tb, faces=[f for f in tb.faces if len(f.verts) == 3], angle_face_threshold=0.02,
                             angle_shape_threshold=math.pi)
    return tb


def panel(m, outline, holes=(), thick=0.14, uv=None, mat='trim', space=None, slices=1.2, around=(), back=True):
    """A wall panel as kit Model.panel makes it (outline (x, z) counterclockwise seen from the front, holes inside it,
    the front facing -Y, thick toward +Y), but back=False leaves out its inside face: walls nobody sees from within
    (closed doors, dark windows) don't pay for it. The reveals round its openings stay."""
    uv = uv if uv is not None else Trim('A', world=True)
    tb = _prism([list(outline)] + [list(h) for h in holes], thick)
    if not back:
        tb.normal_update()
        bmesh.ops.delete(tb, geom=[f for f in tb.faces if f.normal.y > 0.99], context='FACES_ONLY')
    xs = [x for x, z in outline]
    zs = [z for x, z in outline]
    upright = isinstance(uv, Trim) and uv.rotate
    cut_x, cut_z = [], set()
    if slices and not upright:
        cut_x += kit.frange(min(xs) + slices, max(xs) - 0.1, slices)
    for o in around:
        if o.z > 0.15:
            cut_z.add(round(o.z - 0.1, 2))
        if not upright:
            cut_x += [o.x - 0.2, o.x + o.w + 0.2]
    kit._slice(tb, 0, [x for x in cut_x if min(xs) + 0.05 < x < max(xs) - 0.05])
    kit._slice(tb, 2, [z for z in sorted(cut_z) if min(zs) + 0.05 < z < max(zs) - 0.05])
    m.emit(tb, uv, mat, None, space)


def trim_board(m, space, p0, p1, width, look, thick=0.04, out=0.0):
    """A casing or trim board on a wall face (the face at y = -out)."""
    board(m, p0, p1, width, thick, look=look, space=space, lift=out + thick * 0.5, drop=('+y',))


def prism(m, outline, depth, look='C', space=None, matrix=None, y=0.0):
    """A flat piece: outline (x, z) in a wall's plane, its front at y facing -Y, depth thick toward +Y."""
    zs = [z for _, z in outline]
    spec, key = finish(look, width=max(max(zs) - min(zs), depth))
    m.emit(_prism([list(outline)], depth), spec, key,
           (matrix if matrix is not None else Matrix.Identity(4)) @ Matrix.Translation((0.0, y, 0.0)), space)


def cylinder(m, p0, p1, r0, r1=None, sides=8, look='C', space=None, caps=(True, True), face=(0.0, -1.0, 0.0)):
    spec, key = finish(look)
    m.cylinder(p0, p1, r0, r1, sides=sides, uv=spec, mat=key, space=space, caps=caps, face=face)


def lathe(m, profile, at=(0.0, 0.0, 0.0), sides=8, look='brass', space=None, phase=0.0, axis=None):
    """A turned part around a vertical axis through at: profile [(radius, z), ...] from bottom to top (a radius of 0
    closes an end). axis (a matrix) turns it (a part lying down)."""
    tb = kit._new_bmesh()
    rings = []
    for r, z in profile:
        if r <= 1e-6:
            rings.append([tb.verts.new((0.0, 0.0, z))])
        else:
            rings.append([tb.verts.new((r * math.cos(phase + 2.0 * math.pi * k / sides),
                                        r * math.sin(phase + 2.0 * math.pi * k / sides), z)) for k in range(sides)])
    for r0, r1 in zip(rings, rings[1:]):
        for k in range(sides):
            k1 = (k + 1) % sides
            if len(r0) == 1 and len(r1) == 1:
                continue
            try:
                if len(r0) == 1:
                    tb.faces.new((r0[0], r1[k1], r1[k]))
                elif len(r1) == 1:
                    tb.faces.new((r0[k], r0[k1], r1[0]))
                else:
                    tb.faces.new((r0[k], r0[k1], r1[k1], r1[k]))
            except ValueError:
                pass
    bmesh.ops.recalc_face_normals(tb, faces=tb.faces[:])
    spec, key = finish(look)
    matrix = Matrix.Translation(Vector(at)) @ (axis if axis is not None else Matrix.Identity(4))
    m.emit(tb, spec, key, matrix, space, smooth=True)


def sheet(m, points, look='crepe', space=None, smooth=True, flip=False):
    """A one-sided surface through a grid of points (rows of (x, y, z)), facing -Y as given (flip turns it round):
    drapes, ribbons, cloth."""
    tb = kit._new_bmesh()
    grid = [[tb.verts.new(p) for p in row] for row in points]
    for j in range(len(grid) - 1):
        for i in range(len(grid[j]) - 1):
            quad = (grid[j][i], grid[j][i + 1], grid[j + 1][i + 1], grid[j + 1][i])
            try:
                face = tb.faces.new(quad)
            except ValueError:
                continue
    tb.normal_update()
    want = Vector((0.0, 1.0 if flip else -1.0, 0.0))
    total = sum((f.normal * f.calc_area() for f in tb.faces), Vector())
    if total.dot(want) < 0.0:
        for f in tb.faces:
            f.normal_flip()
    spec, key = finish(look)
    m.emit(tb, spec, key, None, space, smooth=smooth)


# --- Lettering ---

_FONT_DATA = []
_LOOPS = {}


def _font():
    if not _FONT_DATA:
        try:
            _FONT_DATA.append(bpy.data.fonts.load(FONT, check_existing=True))
        except Exception as error:  # the bundled face is missing: say so and letter in Georgia Bold
            print(f'LOOTER: warning: Rye failed to load ({error}); lettering in {FALLBACK_FONT}', flush=True)
            _FONT_DATA.append(bpy.data.fonts.load(FALLBACK_FONT, check_existing=True))
    return _FONT_DATA[0]


def _glyph_loops(body, steps=3):
    """The outlines of a line of text at size 1 (an em), as closed loops of 2D points: each Bezier segment of the
    glyphs sampled in steps (straight ones in one)."""
    if (body, steps) in _LOOPS:
        return _LOOPS[(body, steps)]
    curve = bpy.data.curves.new('_text', 'FONT')
    curve.body = body
    curve.font = _font()
    curve.size = 1.0
    src = bpy.data.objects.new('_text', curve)
    bpy.context.scene.collection.objects.link(src)
    bpy.context.view_layer.update()
    deps = bpy.context.evaluated_depsgraph_get()
    evaluated = src.evaluated_get(deps)
    as_curve = evaluated.to_curve(deps)
    loops = []
    for spline in as_curve.splines:
        points = spline.bezier_points
        n = len(points)
        loop = []
        for i in range(n):
            a, b = points[i].co, points[i].handle_right
            c, d = points[(i + 1) % n].handle_left, points[(i + 1) % n].co
            straight = (b - a).length < 1e-6 and (c - d).length < 1e-6
            count = 1 if straight else steps
            for s in range(count):
                t = s / count
                u = 1.0 - t
                q = a * (u * u * u) + b * (3.0 * u * u * t) + c * (3.0 * u * t * t) + d * (t * t * t)
                loop.append(Vector((q.x, q.y)))
        clean = []
        for p in loop:
            if not clean or (p - clean[-1]).length > 1e-5:
                clean.append(p)
        if len(clean) > 2 and (clean[0] - clean[-1]).length < 1e-5:
            clean.pop()
        if len(clean) >= 3:
            loops.append(clean)
    evaluated.to_curve_clear()
    bpy.data.objects.remove(src)
    bpy.data.curves.remove(curve)
    _LOOPS[(body, steps)] = loops
    return loops


def _rdp(points, tol):
    if len(points) < 3:
        return points
    a, b = points[0], points[-1]
    ab = b - a
    length = ab.length
    best, index = -1.0, 0
    for i in range(1, len(points) - 1):
        p = points[i]
        d = (p - a).length if length < 1e-9 else abs(ab.x * (p.y - a.y) - ab.y * (p.x - a.x)) / length
        if d > best:
            best, index = d, i
    if best > tol:
        return _rdp(points[:index + 1], tol)[:-1] + _rdp(points[index:], tol)
    return [a, b]


def _simplify(loop, tol):
    """A closed loop with the points that stray less than tol from a straight run dropped (Ramer-Douglas-Peucker)."""
    if len(loop) < 5 or tol <= 0.0:
        return loop
    far = max(range(len(loop)), key=lambda i: (loop[i] - loop[0]).length)
    return _rdp(loop[:far + 1], tol)[:-1] + _rdp(loop[far:] + [loop[0]], tol)[:-1]


def _area(loop):
    return 0.5 * abs(sum(loop[i].x * loop[(i + 1) % len(loop)].y - loop[(i + 1) % len(loop)].x * loop[i].y
                         for i in range(len(loop))))


def _fill(loops):
    """Fills the loops as Blender fills a 2D curve (holes where loops nest), returning (points, triangles)."""
    curve = bpy.data.curves.new('_fill', 'CURVE')
    curve.dimensions = '2D'
    curve.fill_mode = 'FRONT'
    for loop in loops:
        spline = curve.splines.new('POLY')
        spline.points.add(len(loop) - 1)
        for point, p in zip(spline.points, loop):
            point.co = (p.x, p.y, 0.0, 1.0)
        spline.use_cyclic_u = True
    obj = bpy.data.objects.new('_fill', curve)
    bpy.context.scene.collection.objects.link(obj)
    bpy.context.view_layer.update()
    mesh = bpy.data.meshes.new_from_object(obj.evaluated_get(bpy.context.evaluated_depsgraph_get()))
    bpy.data.objects.remove(obj)
    bpy.data.curves.remove(curve)
    points = [(v.co.x, v.co.y) for v in mesh.vertices]
    faces = [tuple(p.vertices) for p in mesh.polygons]
    bpy.data.meshes.remove(mesh)
    return points, faces


def text_extent(body):
    """The ink box of a line of text at size 1: (x0, y0, x1, y1)."""
    points = [p for loop in _glyph_loops(body) for p in loop]
    return (min(p.x for p in points), min(p.y for p in points), max(p.x for p in points), max(p.y for p in points))


def text(m, body, at, height=None, width=None, space=None, look='cream', tol=0.022, lift=0.0015, rot=0.0,
         size=None, align='center'):
    """Painted lettering in Rye: one line, its ink centered on at (x, y, z in space; align 'left' puts its left end
    there instead), standing on the plane y = at[1] - lift facing -Y. It's as big as height (the ink's height) or width
    allows, or size (meters per em). tol (in ems) is how far the outline may stray from the font's curves: the glyphs
    stay recognisably Rye at 0.02-0.03 and cost a third of their full detail. rot tilts it in its plane (degrees).
    Returns the ink's (width, height)."""
    x0, y0, x1, y1 = text_extent(body)
    ink_w, ink_h = x1 - x0, y1 - y0
    if size is None:
        sizes = [s for s in ((height / ink_h) if height else None, (width / ink_w) if width else None) if s]
        size = min(sizes) if sizes else 1.0
    loops = [_simplify(loop, tol) for loop in _glyph_loops(body)]
    loops = [loop for loop in loops if len(loop) >= 3 and _area(loop) > (tol * 0.6) ** 2]
    points, faces = _fill(loops)
    cx = x0 if align == 'left' else (x0 + x1) * 0.5
    cy = (y0 + y1) * 0.5
    tb = kit._new_bmesh()
    verts = [tb.verts.new(((x - cx) * size, 0.0, (y - cy) * size)) for x, y in points]
    for face in faces:
        try:
            f = tb.faces.new([verts[i] for i in face])
        except ValueError:
            continue
        f.normal_update()
        if f.normal.y > 0.0:
            f.normal_flip()
    spec, key = finish(look)
    m.emit(tb, spec, key, place((at[0], at[1] - lift, at[2]), (0.0, rot, 0.0)), space)
    return ink_w * size, ink_h * size


# --- Outlines and spans ---

def facade_outline(width, top, openings=()):
    """A false front's outline in its wall space: the bottom edge from x = 0 to width (notched for the doors standing on
    it), up the right side, along the top profile (points (x, z) from left to right, from x = 0 to width) and down the
    left side: counterclockwise seen from the front."""
    points = [(0.0, 0.0)]
    for o in sorted((o for o in openings if o.z <= 1e-4), key=lambda o: o.x):
        points += [(o.x, 0.0), (o.x, o.h), (o.x + o.w, o.h), (o.x + o.w, 0.0)]
    points.append((width, 0.0))
    points += list(reversed(top))
    if points[-1] == (0.0, 0.0):
        points.pop()
    return points


def spans_at(outline, z):
    """The x intervals inside a polygon outline [(x, z), ...] at height z."""
    xs = []
    n = len(outline)
    for i in range(n):
        (xa, za), (xb, zb) = outline[i], outline[(i + 1) % n]
        if (za <= z < zb) or (zb <= z < za):
            xs.append(xa + (z - za) * (xb - xa) / (zb - za))
    xs.sort()
    return [(xs[i], xs[i + 1]) for i in range(0, len(xs) - 1, 2)]


def subtract(spans, cut, keep=0.03):
    """spans [(a, b), ...] without the interval cut (pieces shorter than keep go)."""
    c0, c1 = cut
    out = []
    for a, b in spans:
        if c1 <= a or c0 >= b:
            out.append((a, b))
            continue
        if c0 > a:
            out.append((a, c0))
        if c1 < b:
            out.append((c1, b))
    return [(a, b) for a, b in out if b - a > keep]


def intersect(spans_a, spans_b):
    out = []
    for a0, a1 in spans_a:
        for b0, b1 in spans_b:
            lo, hi = max(a0, b0), min(a1, b1)
            if hi - lo > 1e-4:
                out.append((lo, hi))
    return out


# --- Walls ---

def clapboards(m, space, outline, z0, z1, keep_out=(), look='cream', exposure=0.2, out=0.026, thick=0.014):
    """Lapped clapboards over a wall face (its face at y = 0 in space), rows of exposure from z0 to z1 inside the
    outline, each board tilted so its lower edge stands out by out; they stop at the boxes in keep_out ((x0, x1, z0,
    z1): casings, a sign board). Returns how far they stand out (casings over them sit further out)."""
    tilt = -math.degrees(math.atan2(out - thick, exposure))
    z = z0
    while z < z1 - 0.03:
        h = min(exposure, z1 - z)
        spans = intersect(spans_at(outline, z + 0.01), spans_at(outline, z + h - 0.01))
        for x0, x1, k0, k1 in keep_out:
            if k0 < z + h - 0.01 and k1 > z + 0.01:
                spans = subtract(spans, (x0, x1))
        for a, b in spans:
            box(m, (b - a, thick, h + 0.014), at=((a + b) * 0.5, -(out + thick) * 0.5 + 0.003, z + h * 0.5),
                rot=(tilt, 0.0, 0.0), look=look, space=space, drop=('+y', '+z', '-x', '+x'))
        z += exposure
    return out


def pilaster(m, space, x, z0, z1, width=0.26, look='C', out=0.0, depth=0.06, cap=True, base=True, cap_look=None):
    """A flat pilaster on a wall face: a board standing out depth, a plinth block at its foot and a capital at its top."""
    board(m, (x, 0.0, z0), (x, 0.0, z1), width, depth, look=look, space=space, lift=out + depth * 0.5)
    cap_look = cap_look or look
    if base:
        box(m, (width + 0.06, depth + 0.035, 0.3), at=(x, -out - (depth + 0.035) * 0.5, z0 + 0.15), look=cap_look,
            space=space)
    if cap:
        box(m, (width + 0.08, depth + 0.04, 0.12), at=(x, -out - (depth + 0.04) * 0.5, z1 - 0.06), look=cap_look,
            space=space)
        box(m, (width + 0.14, depth + 0.07, 0.06), at=(x, -out - (depth + 0.07) * 0.5, z1 + 0.03), look=cap_look,
            space=space)


def bracket(m, space, x, z, reach=0.26, drop_h=0.42, thick=0.08, look='C', out=0.0):
    """A scroll bracket under a cornice: its top at z against the underside, reaching out from the wall face."""
    r, s = reach, drop_h / 0.42
    profile = [(0.0, 0.0), (r, 0.0), (r, -0.06 * s), (0.8 * r, -0.085 * s), (0.62 * r, -0.13 * s), (0.52 * r, -0.2 * s),
               (0.4 * r, -0.28 * s), (0.24 * r, -0.34 * s), (0.16 * r, -0.4 * s), (0.0, -drop_h)]
    spec, key = finish(look, width=max(reach, drop_h))
    # Built in the XZ plane (x = how far out) and turned so it stands out toward -Y, thick along X.
    matrix = place((x - thick * 0.5, -out, z)) @ Matrix.Rotation(math.radians(-90.0), 4, 'Z')
    m.emit(_prism([profile], thick), spec, key, matrix, space)


def cornice(m, space, x0, x1, z, look='C', crown_look=None, project=0.3, height=0.34, brackets=0.75, out=0.0,
            frieze=0.34, returns=True):
    """A bracketed cornice along a wall top from x0 to x1, the top of its crown at z: a frieze board, a bed molding,
    the projecting crown with its cap, and scroll brackets under the crown every `brackets` meters (none: 0)."""
    crown_look = crown_look or look
    length = x1 - x0
    mid = (x0 + x1) * 0.5
    # Frieze: a broad board under the crown.
    if frieze:
        board(m, (x0 + 0.02, 0.0, z - height - frieze * 0.5), (x1 - 0.02, 0.0, z - height - frieze * 0.5), frieze, 0.035,
              look=look, space=space, lift=out + 0.0175)
    # Bed molding, the crown (a deep box) and a thin cap on it.
    box(m, (length + 0.06, 0.1, 0.08), at=(mid, -out - 0.05, z - height + 0.04), look=look, space=space)
    box(m, (length + project * 0.9, project, height - 0.1), at=(mid, -out - project * 0.5, z - (height - 0.1) * 0.5 - 0.05),
        look=crown_look, space=space, cuts=max(0, int(length / 1.5)))
    box(m, (length + project * 1.1, project + 0.05, 0.05), at=(mid, -out - (project + 0.05) * 0.5, z - 0.025),
        look=crown_look, space=space, cuts=max(0, int(length / 1.5)))
    if brackets:
        count = max(2, int(round(length / brackets)) + 1)
        for k in range(count):
            x = x0 + 0.12 + (length - 0.24) * k / (count - 1)
            bracket(m, space, x, z - height, reach=project * 0.85, drop_h=max(0.26, min(0.42, frieze + 0.1)), look=look,
                    out=out)


def cap_profile(m, space, top, depth=0.22, height=0.12, look='C', out=0.0, back=0.14):
    """Cap boards along a false front's top profile (points (x, z) from left to right), each segment a board lying on
    the edge and standing proud of the face and the back."""
    for (xa, za), (xb, zb) in zip(top, top[1:]):
        if abs(xa - xb) < 1e-4 and abs(za - zb) < 1e-4:
            continue
        if abs(xa - xb) < 1e-4:   # a riser of a step: a board up the edge
            box(m, (0.1, depth + back, abs(zb - za) + height), at=(xa, (back - depth) * 0.5 - out,
                (za + zb) * 0.5 + height * 0.5), look=look, space=space)
            continue
        length = math.hypot(xb - xa, zb - za)
        angle = math.degrees(math.atan2(zb - za, xb - xa))
        mid = ((xa + xb) * 0.5, (za + zb) * 0.5)
        box(m, (length + 0.1, depth + back, height), at=(mid[0], (back - depth) * 0.5 - out, mid[1] + height * 0.5),
            rot=(0.0, -angle, 0.0), look=look, space=space)


# --- Windows and doors (in a wall's space: x along the wall, z up, the outer face at y = 0, outside toward -y) ---

def window(m, space, o, depth, out=0.0):
    """A window in opening o. opts: style 'glass', 'open' (a double-hung window with its lower sash pushed up: the
    lower half is open), 'boarded', 'barred' or 'blind' (no reveal: a false front's fake window); casing and sash (finish
    names), casing_w, panes (columns, rows), head 'cap' or 'pediment', boards (the boarding's finish), bars (count).
    out: where the wall's face is (the face of clapboards, which casings sit on)."""
    rng = m.rng
    x0, z0, w, h = o.x, o.z, o.w, o.h
    style = o.opts.get('style', 'glass')
    casing = o.opts.get('casing', 'A')
    sash = o.opts.get('sash', casing)
    cw = o.opts.get('casing_w', 0.13)
    ct = o.opts.get('casing_t', 0.045)
    for x in (x0 - cw * 0.5, x0 + w + cw * 0.5):
        trim_board(m, space, (x, 0.0, z0 - 0.02), (x, 0.0, z0 + h + cw), cw, casing, thick=ct, out=out)
    head = o.opts.get('head', 'cap')
    head_z = z0 + h + cw * 0.6
    trim_board(m, space, (x0 - cw - 0.02, 0.0, head_z), (x0 + w + cw + 0.02, 0.0, head_z), cw * 1.25, casing,
               thick=ct + 0.01, out=out)
    if head == 'pediment':
        # A small pediment over the head: a triangle of the casing's finish with a cap along its slopes.
        span = w + 2.0 * cw + 0.12
        rise = span * 0.22
        base_z = z0 + h + cw * 1.25
        prism(m, [(x0 - cw - 0.06, base_z), (x0 + w + cw + 0.06, base_z), (x0 + w * 0.5, base_z + rise)], 0.04,
              look=casing, space=space, y=-out - ct - 0.05)
        slope = math.degrees(math.atan2(rise, span * 0.5))
        for sign in (-1.0, 1.0):
            cx = x0 + w * 0.5 + sign * span * 0.25
            box(m, (span * 0.5 / math.cos(math.radians(slope)) + 0.08, 0.1, 0.06),
                at=(cx, -out - ct - 0.06, base_z + rise * 0.5 + 0.04), rot=(0.0, sign * slope, 0.0), look=casing,
                space=space)
        box(m, (span + 0.06, 0.1, 0.06), at=(x0 + w * 0.5, -out - ct - 0.06, base_z + 0.02), look=casing, space=space)
    else:
        box(m, (w + 2.0 * cw + 0.12, 0.09 + ct, 0.045), at=(x0 + w * 0.5, -out - (0.09 + ct) * 0.5, head_z + cw * 0.65 + 0.02),
            look=casing, space=space)
    sill_depth = 0.1 + out + depth * 0.5
    box(m, (w + 2.0 * cw + 0.1, sill_depth, 0.06), at=(x0 + w * 0.5, -0.1 - out + sill_depth * 0.5, z0 - 0.03),
        rot=(rng.uniform(-1.2, 1.2), 0.0, 0.0), look=casing, space=space)
    glass_y = depth * 0.55 if style != 'blind' else 0.02
    cols, rows = o.opts.get('panes', (2, 2))
    if style == 'open':
        # The upper sash stays; the lower one is pushed up in front of it, so the window's lower half is open.
        half = h * 0.5
        box(m, (w, 0.02, half), at=(x0 + w * 0.5, glass_y, z0 + half * 1.5), look='H4', space=space, drop=GLASS)
        box(m, (w, 0.02, half), at=(x0 + w * 0.5, glass_y - 0.05, z0 + half * 1.5 - 0.02), look='H4', space=space,
            drop=GLASS)
        frame = 0.05
        for zc, yc in ((z0 + half, glass_y - 0.05), (z0 + h - frame * 0.5 - 0.02, glass_y - 0.05),
                       (z0 + half + frame * 0.5, glass_y)):
            board(m, (x0, yc, zc), (x0 + w, yc, zc), frame + 0.02, 0.045, look=sash, space=space, drop=('+y',))
        for i in range(1, cols):
            x = x0 + w * i / cols
            board(m, (x, glass_y - 0.08, z0 + half), (x, glass_y - 0.08, z0 + h), 0.04, 0.035, look=sash, space=space,
                  drop=('+y',))
        for x in (x0 + 0.025, x0 + w - 0.025):
            board(m, (x, glass_y - 0.05, z0 + half), (x, glass_y - 0.05, z0 + h), 0.05, 0.045, look=sash, space=space,
                  drop=('+y',))
    else:
        box(m, (w, 0.02, h), at=(x0 + w * 0.5, glass_y, z0 + h * 0.5), look='H4', space=space, drop=GLASS)
        for i in range(1, cols):
            x = x0 + w * i / cols
            board(m, (x, glass_y - 0.03, z0), (x, glass_y - 0.03, z0 + h), 0.045, 0.04, look=sash, space=space,
                  drop=('+y',))
        for j in range(1, rows):
            z = z0 + h * j / rows
            board(m, (x0, glass_y - 0.035, z), (x0 + w, glass_y - 0.035, z), 0.045, 0.04, look=sash, space=space,
                  drop=('+y',))
    if style == 'boarded':
        board_up(m, space, x0, z0, w, h, o.opts.get('boards', 'A'), out=out + ct)
    elif style == 'barred':
        bars(m, space, x0, z0, w, h, o.opts.get('bars', 7), y=-0.02)


def board_up(m, space, x0, z0, w, h, look='A', out=0.045, level=True, extra=0.16):
    """Planks nailed across an opening: an X, and a level plank across, each a little askew, with nail heads."""
    rng = m.rng
    cx, cz = x0 + w * 0.5, z0 + h * 0.5
    reach = math.hypot(w + extra, h + extra) * 0.5
    angle = math.atan2(h, w)
    for k, sign in enumerate((1.0, -1.0)):
        a = sign * angle + math.radians(rng.uniform(-5.0, 5.0))
        d = Vector((math.cos(a), 0.0, math.sin(a))) * reach
        board(m, (cx - d.x, 0.0, cz - d.z), (cx + d.x, 0.0, cz + d.z), rng.uniform(0.17, 0.21), 0.035, look=look,
              space=space, lift=out + 0.02 + k * 0.036, drop=('+y',))
        p = Vector((cx, 0.0, cz)) + d * (0.85 if k else -0.85)
        box(m, (0.032, 0.012, 0.032), at=(p.x, -out - 0.06 - k * 0.036, p.z), look='H3', space=space, drop=('+y',))
    if level:
        a = math.radians(rng.uniform(-4.0, 4.0))
        half = (w + extra) * 0.5
        zc = cz + rng.uniform(-0.2, 0.15) * h
        board(m, (cx - half, 0.0, zc - half * math.sin(a)), (cx + half, 0.0, zc + half * math.sin(a)),
              rng.uniform(0.18, 0.22), 0.035, look=look, space=space, lift=out + 0.1, drop=('+y',))
        for end in (-1.0, 1.0):
            box(m, (0.032, 0.012, 0.032), at=(cx + end * (half - 0.08), -out - 0.13,
                zc + end * (half - 0.08) * math.sin(a)), look='H3', space=space, drop=('+y',))


def bars(m, space, x0, z0, w, h, count=7, y=-0.02, look='iron'):
    """Iron bars across a window: round uprights between two flat bands, set into the casing."""
    for k in range(count):
        x = x0 + w * (k + 0.5) / count
        cylinder(m, (x, y, z0 - 0.03), (x, y, z0 + h + 0.03), 0.017, sides=6, look=look, space=space,
                 caps=(False, False), face=(0.0, 0.0, 1.0))
    for z in (z0 + 0.12, z0 + h - 0.12):
        box(m, (w + 0.06, 0.02, 0.05), at=(x0 + w * 0.5, y - 0.02, z), look=look, space=space)


def door(m, space, o, depth, out=0.0):
    """A door in opening o, a little into the wall. opts: style 'plank' (boards, battens, a brace, strap hinges),
    'panel' (a framed door of four raised panels, a glazed upper pair with glass=True) or 'double' (two glazed
    leaves); finish (the leaf), casing, hinge ('left' or 'right'), transom (the height of a light over the door, 0 for
    none; it's inside the opening's height), knob ('brass' or 'H3'), casing_w."""
    rng = m.rng
    x0, w, h = o.x, o.w, o.h
    style = o.opts.get('style', 'plank')
    look = o.opts.get('finish', 'A')
    casing = o.opts.get('casing', 'A')
    transom = o.opts.get('transom', 0.0)
    knob = o.opts.get('knob', 'H3')
    hinge_left = o.opts.get('hinge', 'left') == 'left'
    cw = o.opts.get('casing_w', 0.14)
    leaf_h = h - transom - (0.06 if transom else 0.0)
    y = depth * 0.3
    if transom:
        box(m, (w, 0.02, transom), at=(x0 + w * 0.5, depth * 0.5, leaf_h + 0.06 + transom * 0.5), look='H4', space=space,
            drop=GLASS)
        board(m, (x0, depth * 0.45 - 0.03, leaf_h + 0.03), (x0 + w, depth * 0.45 - 0.03, leaf_h + 0.03), 0.08, 0.06,
              look=casing, space=space)
        mid = x0 + w * 0.5
        board(m, (mid, depth * 0.5 - 0.03, leaf_h + 0.06), (mid, depth * 0.5 - 0.03, h), 0.04, 0.035, look=casing,
              space=space)
    leaves = [(x0, w, hinge_left)] if style != 'double' else [(x0, w * 0.5, True), (x0 + w * 0.5, w * 0.5, False)]
    for lx, lw, left in leaves:
        if style == 'plank':
            boards = max(3, int(round(lw / 0.2)))
            bw = lw / boards
            for k in range(boards):
                x = lx + bw * (k + 0.5)
                board(m, (x, y, 0.0), (x, y, leaf_h - rng.uniform(0.0, 0.025)), bw - 0.006, 0.05, look=look,
                      space=space, drop=('+y',))
            for z in (0.3, leaf_h - 0.35):
                board(m, (lx + 0.06, y - 0.04, z), (lx + lw - 0.06, y - 0.04, z), 0.16, 0.035, look=look, space=space,
                      drop=('+y',))
            low, high = (lx + 0.1, lx + lw - 0.1) if left else (lx + lw - 0.1, lx + 0.1)
            board(m, (high, y - 0.04, 0.4), (low, y - 0.04, leaf_h - 0.45), 0.13, 0.03, look=look, space=space,
                  drop=('+y',))
            hinge_x = lx if left else lx + lw
            reach = lw * 0.72
            for z in (0.3, leaf_h - 0.35):
                end = hinge_x + reach if left else hinge_x - reach
                board(m, (hinge_x - (0.09 if left else -0.09), y - 0.065, z), (end, y - 0.065, z), 0.07, 0.014,
                      look='H3', space=space, drop=('+y',))
                box(m, (0.05, 0.05, 0.11), at=(hinge_x - (0.1 if left else -0.1), -out - 0.03, z), look='H3',
                    space=space)
        else:
            # A framed leaf: stiles and rails round raised panels (or glass in the upper pair).
            box(m, (lw - 0.01, 0.045, leaf_h), at=(lx + lw * 0.5, y + 0.01, leaf_h * 0.5), look=look, space=space,
                drop=('+y',))
            stile = 0.12 if lw > 0.7 else 0.09
            for x in (lx + stile * 0.5, lx + lw - stile * 0.5):
                board(m, (x, y - 0.025, 0.0), (x, y - 0.025, leaf_h), stile, 0.03, look=look, space=space,
                      drop=('+y',))
            rails = (0.12, leaf_h * 0.42, leaf_h - 0.08)
            for z in rails:
                board(m, (lx + stile, y - 0.025, z), (lx + lw - stile, y - 0.025, z), 0.14 if z < 1.0 else 0.1, 0.03,
                      look=look, space=space, drop=('+y',))
            glass = o.opts.get('glass', style == 'double')
            panels = o.opts.get('panels', look)
            pane_w = (lw - 2.0 * stile - 0.06) * (0.5 if lw > 0.7 else 1.0)
            columns = 2 if lw > 0.7 else 1
            for c in range(columns):
                px = lx + stile + 0.03 + pane_w * (c + 0.5) + (0.06 * c if columns == 2 else 0.0)
                for z0, z1, glazed in ((0.22, leaf_h * 0.42 - 0.08, False), (leaf_h * 0.42 + 0.08, leaf_h - 0.15, glass)):
                    if glazed:
                        box(m, (pane_w, 0.02, z1 - z0), at=(px, y - 0.005, (z0 + z1) * 0.5), look='H4', space=space,
                            drop=GLASS)
                    else:
                        box(m, (pane_w - 0.04, 0.03, z1 - z0 - 0.04), at=(px, y - 0.03, (z0 + z1) * 0.5), look=panels,
                            space=space, bevel=0.012, drop=('+y',))
        # The knob or latch, on the side away from the hinge.
        kx = lx + lw - 0.09 if left else lx + 0.09
        if knob == 'brass':
            lathe(m, [(0.0, -0.06), (0.032, -0.055), (0.036, -0.03), (0.016, -0.015), (0.012, 0.0)], sides=8,
                  look='brass', space=space, at=(kx, y - 0.04, 1.0), axis=Matrix.Rotation(math.radians(-90.0), 4, 'X'))
            box(m, (0.06, 0.012, 0.2), at=(kx, y - 0.03, 1.02), look='brass', space=space)
        else:
            box(m, (0.05, 0.04, 0.16), at=(kx, y - 0.07, 1.0), look='H3', space=space)
            box(m, (0.12, 0.03, 0.03), at=(kx, y - 0.1, 1.05), look='H3', space=space)
    for x in (x0 - cw * 0.5, x0 + w + cw * 0.5):
        trim_board(m, space, (x, 0.0, -0.02), (x, 0.0, h + cw), cw, casing, thick=0.05, out=out)
    trim_board(m, space, (x0 - cw - 0.03, 0.0, h + cw * 0.55), (x0 + w + cw + 0.03, 0.0, h + cw * 0.55), cw * 1.2,
               casing, thick=0.06, out=out)
    box(m, (w + 0.08, depth * 0.5 + 0.1 + out, 0.05), at=(x0 + w * 0.5, depth * 0.25 - 0.05 - out * 0.5, -0.005),
        look='C', space=space)


def openings(m, space, items, depth, out=0.0):
    for o in items:
        if o.kind == 'window':
            window(m, space, o, depth, out)
        elif o.kind == 'door':
            door(m, space, o, depth, out)


# --- Mourning crepe ---

def crepe_bow(m, space, at, scale=1.0, tails=0.7, y=0.0):
    """A bow of black crepe with two tails hanging from it, on a surface facing -Y: its knot at at."""
    x, _, z = at
    s = scale
    for side in (-1.0, 1.0):
        # Each loop: a band between an outer and an inner ellipse, the inner one standing forward so it puffs out.
        cx = x + side * 0.1 * s
        outer, inner = [], []
        for k in range(9):
            a = side * 2.0 * math.pi * k / 8
            ox, oz = 0.1 * s * math.cos(a), 0.052 * s * math.sin(a)
            outer.append((cx + ox, y - 0.012 * s, z + oz + 0.01 * s))
            inner.append((cx + ox * 0.4, y - 0.05 * s, z + oz * 0.4 + 0.01 * s))
        sheet(m, [outer, inner], look='crepe', space=space)
    box(m, (0.06 * s, 0.05 * s, 0.07 * s), at=(x, y - 0.035 * s, z), look='crepe', space=space, bevel=0.01 * s)
    for side in (-1.0, 1.0):
        rows = []
        for j in range(6):
            t = j / 5.0
            zt = z - 0.03 * s - tails * t
            xt = x + side * (0.02 + 0.09 * t) * s + 0.015 * math.sin(t * 7.0 + side) * s
            ripple = 0.012 * math.sin(t * 9.0 + side * 1.3) * s
            rows.append([(xt - 0.035 * s, y - 0.02 * s + ripple, zt), (xt + 0.035 * s, y - 0.02 * s - ripple, zt)])
        sheet(m, [[r[0] for r in rows], [r[1] for r in rows]], look='crepe', space=space)


def crepe_swag(m, space, x0, x1, z, sag=0.14, width=0.16, y=-0.03, folds=10):
    """A swag of crepe hung across between two nails, sagging in the middle, its folds rippling."""
    top, bottom = [], []
    for k in range(folds + 1):
        t = k / folds
        x = x0 + (x1 - x0) * t
        dz = -sag * math.sin(math.pi * t)
        ripple = 0.012 * math.sin(t * math.pi * folds)
        top.append((x, y + ripple, z + dz * 0.6))
        bottom.append((x, y - 0.03 + ripple, z + dz - width))
    sheet(m, [bottom, top], look='crepe', space=space)


# --- The plank deck (boardwalk, porch) ---

def deck(m, x0, x1, y0, y1, top=DECK_TOP, posts=(), front=True, back=True, sill=True, ends=(), overhang=0.02,
         joists=3, plank_cuts=1.4):
    """A plank deck from x0 to x1, its planks running across it from y0 (the front, toward -Y) to y1, their top at top:
    planks every PLANK (the first centered at x0 + PLANK / 2, so decks chain), joists along X under them, rim boards
    along the front (front=True) and back, posts under the joists at the x positions in posts, a ground sill under the
    front posts, and end boards across the open ends at the x values in ends. plank_cuts: a plank is cut every so many
    meters for its baked shading (0: never)."""
    rng = m.rng
    count = int(round((x1 - x0) / PLANK))
    for k in range(count):
        x = x0 + PLANK * (k + 0.5)
        dz = rng.uniform(-0.004, 0.003)
        yaw = rng.uniform(-0.25, 0.25)
        y_front = y0 - overhang + rng.uniform(-0.01, 0.01)
        p0, p1 = Vector((x, y_front, top - 0.025 + dz)), Vector((x, y1, top - 0.025 + dz))
        length = (p1 - p0).length
        matrix = (Matrix.Translation(Vector((x, (y_front + y1) * 0.5, 0.0))) @ Matrix.Rotation(math.radians(yaw), 4, 'Z')
                  @ Matrix.Translation(Vector((-x, -(y_front + y1) * 0.5, 0.0))) @ kit.toward(p0, p1, (0.0, 0.0, 1.0))
                  @ Matrix.Translation((length * 0.5, 0.0, 0.0)))
        cuts = int(length / plank_cuts) if plank_cuts else 0
        m.emit(kit._box((length, 0.05, PLANK - 0.012 - rng.uniform(0.0, 0.006)), 0.0, cuts, ('+y',)),
               PlankRow(), 'planks', matrix)
    width = y1 - y0
    for j in range(joists):
        y = y0 + 0.12 + (width - 0.24) * j / max(joists - 1, 1)
        box(m, (x1 - x0, 0.08, 0.16), at=((x0 + x1) * 0.5, y, top - 0.05 - 0.08), look='C', drop=('-z',))
    rim_h = 0.2
    for side, on in ((y0, front), (y1, back)):
        if on:
            face = (0.0, -1.0, 0.0) if side == y0 else (0.0, 1.0, 0.0)
            board(m, (x0, side, top - 0.05 - rim_h * 0.5), (x1, side, top - 0.05 - rim_h * 0.5), rim_h, 0.045,
                  look='A', face=face, lift=-0.0225 if side == y0 else -0.0225, drop=('+y',))
    for x in posts:
        for y in (y0 + 0.1, y1 - 0.1):
            box(m, (0.14, 0.14, top - 0.21 + 0.12), at=(x + rng.uniform(-0.03, 0.03), y, (top - 0.21 - 0.12) * 0.5),
                rot=(0.0, 0.0, rng.uniform(-6.0, 6.0)), look='C', drop=('-z',))
    if sill:
        box(m, (x1 - x0, 0.16, 0.14), at=((x0 + x1) * 0.5, y0 + 0.1, -0.05), look='C', drop=('-z',))
    for x in ends:
        face = (-1.0, 0.0, 0.0) if abs(x - x0) < 1e-6 else (1.0, 0.0, 0.0)
        board(m, (x, y0, top - 0.05 - rim_h * 0.5), (x, y1, top - 0.05 - rim_h * 0.5), rim_h, 0.045, look='A',
              face=face, lift=-0.0225, drop=('+y',))


# --- Roofs and foundations: the kit's, without the faces nobody sees ---
# Each draws the same random values in the same order as the kit function it copies, so a building keeps its look.

def plinth(m, x0, x1, y0, y1, height, out=0.08):
    """kit.plinth with its inside and bottom faces left out and a vertex row every 2.4 m instead of every 1.1 m."""
    rng = m.rng
    deep = 0.3
    specs = [((x0 - out, y0 - out), (x1 + out, y0 - out), 0.0), ((x1 + out, y0 - out), (x1 + out, y1 + out), deep),
             ((x1 + out, y1 + out), (x0 - out, y1 + out), 0.0), ((x0 - out, y1 + out), (x0 - out, y0 - out), deep)]
    for p0, p1, inset in specs:
        space = kit.wall_space(p0, p1, 0.0)
        length = (Vector(p1) - Vector(p0)).length - 2.0 * inset
        m.box((length, deep, height + 0.1), at=(inset + length * 0.5, deep * 0.5, height * 0.5 - 0.05),
              uv=Trim('D', world=True, u=rng.uniform(0.0, kit.U_REPEAT)), space=space,
              cuts=max(0, int(length / 2.4)), drop=('+y', '-z'))


def roof_deck(m, slope, deck=0.12, uv='A', rake_boards=True, rake_sides=(True, True)):
    """kit.roof_deck, cut every 3 m along the eave (enough to follow the sag) instead of every 1.2 m."""
    m.box((slope.width, slope.length, deck), at=(slope.width * 0.5, slope.length * 0.5, -deck * 0.5),
          uv=Trim(uv, world=True), space=slope, cuts=max(1, int(slope.width / 3.0)))
    if rake_boards:
        for side, x in enumerate((-0.025, slope.width + 0.025)):
            if rake_sides[side]:
                face = (-1.0, 0.0, 0.0) if side == 0 else (1.0, 0.0, 0.0)
                mid = (0.06 - deck - 0.05) * 0.5
                m.board((x, -0.02, mid), (x, slope.length - 0.02, mid), deck + 0.11, 0.05, face=face, uv='C',
                        space=slope)


def gable(m, x0, x1, y0, y1, eave_z, pitch, overhang=0.4, rake=0.3, deck=0.12, sag=0.0, deck_uv='A',
          rake_boards=True, ends=(True, True), frame=None):
    """kit.gable on this module's roof_deck."""
    theta = math.radians(pitch)
    half = (y1 - y0) * 0.5
    run = half + overhang
    length = run / math.cos(theta) + deck * math.tan(theta)
    width = x1 - x0 + 2.0 * rake
    normal_f = Vector((0.0, -math.sin(theta), math.cos(theta)))
    eave_f = Vector((x0 - rake, y0 - overhang, eave_z - overhang * math.tan(theta))) + normal_f * deck
    front = kit.Slope(eave_f, (1.0, 0.0, 0.0), (0.0, math.cos(theta), math.sin(theta)), width, length, sag, frame)
    normal_b = Vector((0.0, math.sin(theta), math.cos(theta)))
    eave_b = Vector((x1 + rake, y1 + overhang, eave_z - overhang * math.tan(theta))) + normal_b * deck
    back = kit.Slope(eave_b, (-1.0, 0.0, 0.0), (0.0, -math.cos(theta), math.sin(theta)), width, length, sag, frame)
    roof_deck(m, front, deck, deck_uv, rake_boards, (ends[0], ends[1]))
    roof_deck(m, back, deck, deck_uv, rake_boards, (ends[1], ends[0]))
    return front, back


def ridge_cap(m, front, back, uv='F', width=0.22, thick=0.025, lift=0.03):
    """kit.ridge_cap without its underside (it lies on the covering), cut every 3 m instead of every metre."""
    for slope in (front, back):
        m.box((slope.width + 0.06, width, thick),
              matrix=place((slope.width * 0.5, slope.length - width * 0.5 + 0.02, lift + thick * 0.5), (-2.0, 0.0, 0.0)),
              uv=Trim(uv, lane=0) if uv in kit.LANES else uv, space=slope, cuts=max(1, int(slope.width / 3.0)),
              drop=('-z',))


def _outside(a, b, y, skip, step=0.05):
    """The parts of [a, b] (along a course at y) that skip leaves alone."""
    parts, start = [], None
    n = max(1, int(math.ceil((b - a) / step)))
    for i in range(n + 1):
        x = a + (b - a) * i / n
        inside = skip(x, y)
        if not inside and start is None:
            start = x
        elif inside and start is not None:
            parts.append((start, x - (b - a) / n * 0.5))
            start = None
    if start is not None:
        parts.append((start, b))
    return [(p, q) for p, q in parts if q - p > 0.05]


def shakes(m, slope, course=0.2, butt=0.03, piece=(1.4, 2.6), start=-0.08, top=None, missing=0.02, skip=None):
    """kit.shingles, the same courses and the same random choices, made cheaper and tidier: a piece's two end faces
    (hidden against its neighbours, and at the slope's sides under the rake boards) are left out, except beside a
    missing shake, and a piece crossing the skip area (a chimney) is cut round it instead of left out whole, which
    with long pieces left bare deck metres long beside a stack."""
    rng = m.rng
    top = slope.length + 0.02 if top is None else top
    exposure = course - 0.02
    y = start
    k = 0
    while y < top - 0.04:
        h = min(course, top - y)
        x = -0.05 + rng.uniform(-0.3, 0.0)
        while x < slope.width + 0.05 - 1e-3:
            w = rng.uniform(*piece)
            if slope.width + 0.05 - (x + w) < 0.6:
                w = slope.width + 0.05 - x
            x_lo = max(x, -0.05)
            spans = [(x_lo, x + w, False, False)]           # (from, to, keep its low end, keep its high end)
            if k >= 2 and w > 0.9 and rng.random() < missing * 3.0:
                gap = rng.uniform(0.2, 0.45)
                at = rng.uniform(x_lo + 0.3, x + w - 0.3 - gap)
                spans = [(x_lo, at, False, True), (at + gap, x + w, True, False)]
            cy = y + h * 0.5
            tilt = -math.degrees(math.atan2(butt, h))
            for a, b, keep_lo, keep_hi in spans:
                cx = (a + b) * 0.5
                if skip is not None and skip(cx, cy):
                    # The kit left this piece out and drew nothing for it: what's left beside the stack goes in plain.
                    jitter, roll, u = 0.0, 0.0, (cx * 0.37) % kit.U_REPEAT
                else:
                    jitter = rng.uniform(-0.012, 0.012)
                    roll = rng.uniform(-0.4, 0.4)
                    u = rng.uniform(0.0, kit.U_REPEAT)           # what the kit's Trim drew for the piece's U
                parts = _outside(a, b, cy, skip) if skip is not None else [(a, b)]
                for pa, pb in parts:
                    drop = ['-z', '+y']
                    if not (keep_lo or pa > a + 1e-6):
                        drop.append('-x')
                    if not (keep_hi or pb < b - 1e-6):
                        drop.append('+x')
                    matrix = place(((pa + pb) * 0.5, cy + jitter, butt * 0.5 + 0.004 * (k % 2)), (tilt, roll, 0.0))
                    m.box((pb - pa, h, butt), matrix=matrix, uv=Trim('E', lane=k % 4, u=u + (pa + pb - a - b) * 0.5),
                          space=slope, ao_floor=kit.ROOF_AO, drop=tuple(drop))
            x += w
        y += exposure
        k += 1


# --- The hidden roof ---

def hidden_gable(m, x0, x1, y_front, y_back, eave_z, pitch, covering='shakes', overhang=0.28, rake=0.3, sag=0.06,
                 deck_t=0.1, chimney=None, piece=(4.5, 6.0)):
    """A false front's gable roof: its ridge runs from the back of the facade (y_front) to past the back wall (y_back,
    the rake included), over walls from x0 to x1 resting on them at eave_z. No rake board in front (the facade hides
    it). covering: 'shakes' (big roofs: the tin strip's blue-grey and rust bands read as loot colors over a large area)
    or 'tin'. chimney (x, y, half x, half y) leaves the covering off round a stack. Returns the right and left Slopes."""
    frame = Matrix.Rotation(math.radians(90.0), 4, 'Z')
    right, left = gable(m, y_front + rake, y_back - rake, -x1, -x0, eave_z, pitch, overhang=overhang, rake=rake,
                        deck=deck_t, sag=sag, ends=(False, True), frame=frame)

    def gap(slope):
        if chimney is None:
            return None

        def skip(x, y):
            world = slope.world((x, y, 0.0))
            return abs(world.x - chimney[0]) < chimney[2] and abs(world.y - chimney[1]) < chimney[3]
        return skip
    for slope in (right, left):
        if covering == 'tin':
            kit.tin(m, slope, skip=gap(slope))
        else:
            # Long pieces: each is a run of the strip's painted shakes, so fewer, longer boxes cost less and look alike.
            shakes(m, slope, piece=piece, skip=gap(slope))
    ridge_cap(m, right, left, uv='F' if covering == 'tin' else 'C', width=0.22 if covering == 'tin' else 0.16,
              thick=0.025 if covering == 'tin' else 0.05)
    return right, left


def facade_back(m, space, width, top, roof_z, thick, look='C', studs=None):
    """The back of a false front above its roof: studs up to the top and two raking braces down onto the roof, as real
    false fronts are propped. top is the profile ((x, z) left to right); roof_z(x) the roof's height under x (in the
    facade's space); thick the facade's thickness (its back face at y = thick); studs the x positions (default: every
    1.2 m)."""
    studs = studs or [x for x in kit.frange(0.6, width - 0.3, 1.2)]

    def top_at(x):
        for (xa, za), (xb, zb) in zip(top, top[1:]):
            if xa - 1e-6 <= x <= xb + 1e-6 and xb > xa:
                return za + (zb - za) * (x - xa) / (xb - xa)
        return max(z for _, z in top)
    for x in studs:
        z0, z1 = roof_z(x) - 0.15, top_at(x) - 0.08
        if z1 - z0 > 0.3:
            board(m, (x, thick, z0), (x, thick, z1), 0.12, 0.08, look=look, face=(0.0, 1.0, 0.0), space=space,
                  lift=0.04)
    for x in (width * 0.3, width * 0.7):
        z_top = top_at(x) - 0.5
        z_roof = roof_z(x)
        if z_top - z_roof > 0.4:
            run = (z_top - z_roof) * 1.3
            board(m, (x, thick + 0.08, z_top), (x, thick + 0.08 + run, z_roof + 0.05), 0.1, 0.07, look=look,
                  face=(1.0, 0.0, 0.0), space=space)
