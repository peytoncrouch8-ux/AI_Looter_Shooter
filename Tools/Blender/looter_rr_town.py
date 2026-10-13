"""Helpers for Ransom's Rest's new town buildings (Docs/Areas/RansomsRest_Expansion.md, section 9): the three Main Street
fronts (Art/Models/Buildings/FrontsRR.py), Amos's hay barn (BarnWhitlock.py), the depot and its coal stage (DepotRR.py)
and Bright & Daughter's coffin workshop over its cellar (WorkshopBright.py). It sits on looter_homes (Home: the town's
finishes, WindowGlow, the pivot moved to the front door at finish()), looter_town and looter_buildings, and adds:

  - Building, a Home that also keeps its sockets' places and its rooms for the report and the look sheet;
  - the false front's parts (FalseFronts.py's own, which builds its four fronts when imported, so copied here);
  - rooms you walk into: one-sided inner faces of walls round their openings, floors and ceilings with holes, casings on
    both faces of a doorway, door leaves standing open, windows seen from both sides (broken panes, a lower sash pushed
    up), straight flights of stairs with their ramp hulls, railings and balusters;
  - furniture: benches, tables, chairs, a pot-bellied stove, lamps (LanternGlow) with their sockets, coffins and
    trestles, shelves, chalk lettering;
  - previews: the golden afternoon of looter_homes, roof-off views (a copy of the model without what's above a height),
    views inside at eye height with the lamps lit, and a report per script that town_sheet.py lays out as the look sheet.

Everything random comes from the model's seeded random.Random, and nothing iterates a set of mesh elements, so a building
comes out the same on every run (Docs/Pipeline.md: export twice and compare).
"""
import json
import math
import os

import bmesh
import bpy
from mathutils import Matrix, Vector
from mathutils.geometry import tessellate_polygon

import looter_buildings as kit
import looter_homes as homes
import looter_textures as lt
import looter_town as town
from looter_buildings import Opening, Space, Tile, Trim, place

PREVIEW_DIR = os.path.join(lt.PREVIEW_DIR, 'RansomsRest', 'Town')
SHEET_PATH = os.path.join(lt.PREVIEW_DIR, 'RansomsRest', 'LookSheets', '36_Town.jpg')
# Nanite's fallback (what Medium and Low draw) keeps every triangle: the buildings' rule (FalseFronts.py, Homestead.py).
FALLBACK = 100.0
EYE = 1.42                       # the player's eyes over the floor (0.85 of the mannequin), for the views inside
UP = Vector((0.0, 0.0, 1.0))


def args():
    return homes.args()


# --- The model ---

class Building(homes.Home):
    """A Home (looter_homes) that keeps what the report needs: its sockets' places (after the pivot moved) and its rooms
    (lines of text). finish(pivot) moves the model so pivot (x, y on the ground) is its origin, as the homes do."""

    def __init__(self, name, seed, lived=False):
        super().__init__(name, seed, lived)
        self.rooms = []
        self.captions = []

    def finish(self, pivot=(0.0, 0.0), footprint=None, **options):
        options.setdefault('fallback', FALLBACK)
        obj = super().finish(pivot=pivot, footprint=footprint, **options)
        self.report['socket_at'] = [(name, [round(c, 3) for c in at], [round(r, 1) for r in rotation])
                                    for name, at, rotation in self.sockets]
        self.report['rooms'] = list(self.rooms)
        self.report['pivot_from_middle'] = [round(-pivot[0], 3), round(-pivot[1], 3)]
        for name, at, rotation in self.sockets:
            print(f'SOCKET: {self.name} {name} at ({at[0]:.2f}, {at[1]:.2f}, {at[2]:.2f}) rot {tuple(rotation)}',
                  flush=True)
        return obj


def rot_to(direction):
    """A socket's rotation (degrees about Z) so its front (the empty's -Y) points along direction (x, y)."""
    return (0.0, 0.0, math.degrees(math.atan2(direction[0], -direction[1])))


# --- Faces ---

def face_part(m, points, uv, mat='trim', facing=None, space=None, smooth=False):
    """A one-sided flat polygon through points (x, y, z), turned to face facing if given."""
    tb = kit._new_bmesh()
    verts = [tb.verts.new(p) for p in points]
    f = tb.faces.new(verts)
    f.normal_update()
    if facing is not None and f.normal.dot(Vector(facing)) < 0.0:
        f.normal_flip()
    m.emit(tb, uv, mat, None, space, smooth=smooth)


def wall_face(m, a, b, z0, outline, holes=(), uv=None, mat='trim', cuts=1.2):
    """The inside face of a wall: one-sided, on the line a -> b (plan points; walking from a to b the room is on the
    right), outline [(s, z)...] counterclockwise as seen from the room (s along a -> b from 0, z up from z0), holes
    [(s0, s1, z0, z1)...] for the openings. Cut every `cuts` metres along it and under the openings for the AO."""
    space = kit.wall_space(a, b, z0)
    loops = [list(outline)] + [kit.rect(s0, zz0, s1 - s0, zz1 - zz0) for s0, s1, zz0, zz1 in holes]
    tb = kit._prism(loops, 0.01)
    tb.normal_update()
    bmesh.ops.delete(tb, geom=[f for f in tb.faces if f.normal.y > -0.99], context='FACES')
    xs = [p[0] for p in outline]
    zs = [p[1] for p in outline]
    upright = isinstance(uv, Trim) and uv.rotate
    cut_x = [] if upright or not cuts else kit.frange(min(xs) + cuts, max(xs) - 0.1, cuts)
    cut_z = []
    for s0, s1, zz0, zz1 in holes:
        if not upright:
            cut_x += [s0 - 0.2, s1 + 0.2]
        if zz0 > 0.15 and round(zz0 - 0.1, 2) not in cut_z:
            cut_z.append(round(zz0 - 0.1, 2))
    kit._slice(tb, 0, sorted(x for x in cut_x if min(xs) + 0.05 < x < max(xs) - 0.05))
    kit._slice(tb, 2, sorted(z for z in cut_z if min(zs) + 0.05 < z < max(zs) - 0.05))
    m.emit(tb, uv if uv is not None else plaster(), mat, None, space)
    return space


def rect_outline(length, height):
    return [(0.0, 0.0), (length, 0.0), (length, height), (0.0, height)]


def _grid(m, cuts_u, cuts_v, keep, point, uv, mat, space=None, facing=None):
    """A grid of quads (cells between the cut values along u and v; keep(cu, cv) says which cells are there), its corners
    at point(u, v), turned to face facing (a direction in space). Neighbouring cells share their corners, so the AO is
    smooth across them and an opening's corner is a vertex."""
    tb = kit._new_bmesh()
    verts = {}

    def vert(i, j):
        if (i, j) not in verts:
            verts[(i, j)] = tb.verts.new(point(cuts_u[i], cuts_v[j]))
        return verts[(i, j)]
    for i in range(len(cuts_u) - 1):
        for j in range(len(cuts_v) - 1):
            cu, cv = (cuts_u[i] + cuts_u[i + 1]) * 0.5, (cuts_v[j] + cuts_v[j + 1]) * 0.5
            if not keep(cu, cv):
                continue
            f = tb.faces.new((vert(i, j), vert(i + 1, j), vert(i + 1, j + 1), vert(i, j + 1)))
            f.normal_update()
            if facing is not None and f.normal.dot(Vector(facing)) < 0.0:
                f.normal_flip()
    if not tb.faces:
        tb.free()
        return
    m.emit(tb, uv, mat, None, space)


def _cuts(lo, hi, extra, every):
    values = [lo, hi] + [v for v in extra if lo + 1e-3 < v < hi - 1e-3]
    if every:
        values += [v for v in kit.frange(lo + every, hi - 0.15, every)]
    values = sorted(values)
    out = []
    for v in values:
        if not out or v - out[-1] > 0.02:
            out.append(v)
    if hi - out[-1] > 1e-6:
        out[-1] = hi
    return out


def wall_cells(m, a, b, z_floor, z0, z1, openings=(), uv=None, mat='trim', every=1.4, s_range=None):
    """The inside face of a wall as a grid of cells: on the line a -> b (plan; the room on the right walking a -> b), from
    z0 to z1 over the floor at z_floor, leaving out the openings [(s0, s1, oz0, oz1), ...] (s along a -> b, heights over
    the floor). Doors that reach the floor, windows inside it, anything crossing its edges all cut cleanly."""
    space = kit.wall_space(a, b, z_floor)
    length = (Vector(b) - Vector(a)).length
    s0, s1 = s_range if s_range else (0.0, length)
    us = _cuts(s0, s1, [v for o in openings for v in o[:2]], every)
    vs = _cuts(z0, z1, [v for o in openings for v in o[2:]], 0.0)

    def keep(cu, cv):
        return not any(o[0] < cu < o[1] and o[2] < cv < o[3] for o in openings)
    _grid(m, us, vs, keep, lambda u, v: (u, 0.0, v), uv if uv is not None else plaster(), mat, space, (0.0, -1.0, 0.0))
    return space


def flat_cells(m, x0, x1, y0, y1, z, holes=(), uv=None, mat='planks', up=True, every=1.6):
    """A floor (up) or a ceiling (facing down) from x0..x1 by y0..y1 at height z, as a grid of cells, leaving out the
    holes [(hx0, hx1, hy0, hy1), ...]."""
    xs = _cuts(x0, x1, [v for h in holes for v in h[:2]], every)
    ys = _cuts(y0, y1, [v for h in holes for v in h[2:]], every)

    def keep(cx, cy):
        return not any(h[0] < cx < h[1] and h[2] < cy < h[3] for h in holes)
    _grid(m, xs, ys, keep, lambda u, v: (u, v, z), uv if uv is not None else Tile('WoodPlanks', along=None), mat, None,
          (0.0, 0.0, 1.0 if up else -1.0))


def lining(m, p0, p1, depth_dir, depth, z0, z1, look='C', head=True, thick=0.02):
    """The boards lining a doorway's or a window's reveal through a wall: jambs at p0 and p1 (plan points on one face
    of the wall), depth along depth_dir (into the wall), from z0 to z1, and a head board."""
    p0, p1 = Vector((p0[0], p0[1], 0.0)), Vector((p1[0], p1[1], 0.0))
    d = Vector((depth_dir[0], depth_dir[1], 0.0)).normalized()
    along = (p1 - p0).normalized()
    for p, inward in ((p0, along), (p1, -along)):
        c = p + d * depth * 0.5 + inward * thick * 0.5
        town.board(m, (c.x, c.y, z0), (c.x, c.y, z1), depth, thick, look=look, face=tuple(inward), drop=('+y',))
    if head:
        c = (p0 + p1) * 0.5 + d * depth * 0.5
        town.board(m, (p0.x + d.x * depth * 0.5, p0.y + d.y * depth * 0.5, z1 - thick * 0.5),
                   (p1.x + d.x * depth * 0.5, p1.y + d.y * depth * 0.5, z1 - thick * 0.5), depth, thick, look=look,
                   face=(0.0, 0.0, -1.0), drop=('+y',))


def panel_leaf(m, frame, w, h, look='woodoxide', glass=True, hinge_left=True, thick=0.045):
    """A framed door leaf in frame (a Space: the hinge edge on its z axis, the leaf along +x (hinge_left) or -x, its
    faces toward -y and +y), both faces built: a slab, stiles and rails standing proud on both faces, a lower raised
    panel each side and, with glass, a pane in the upper half."""
    s = 1.0 if hinge_left else -1.0
    cx = s * w * 0.5
    stile, rail = 0.11, 0.13
    split = h * 0.45
    if glass:
        # The slab below the glass and round it, the pane (both faces) in the opening it leaves.
        town.box(m, (w, thick, split + 0.06), at=(cx, 0.0, (split + 0.06) * 0.5), look=look, space=frame)
        for x in (s * stile * 0.5, s * (w - stile * 0.5)):
            town.box(m, (stile, thick, h - split - 0.06), at=(x, 0.0, split + 0.06 + (h - split - 0.06) * 0.5), look=look,
                     space=frame, drop=('-z',))
        town.box(m, (w - 2.0 * stile, thick, rail), at=(cx, 0.0, h - rail * 0.5), look=look, space=frame,
                 drop=('-x', '+x'))
        m.box((w - 2.0 * stile, 0.008, h - split - 0.06 - rail), at=(cx, 0.0, split + 0.06 + (h - split - 0.06 - rail) * 0.5),
              uv=Trim('H4', fit=True), space=frame, drop=('-x', '+x', '-z', '+z'))
    else:
        town.box(m, (w, thick, h), at=(cx, 0.0, h * 0.5), look=look, space=frame)
    for side in (-1.0, 1.0):
        y = side * (thick * 0.5 + 0.008)
        back = '+y' if side < 0 else '-y'
        for x in (s * stile * 0.5, s * (w - stile * 0.5)):
            town.box(m, (stile, 0.016, h), at=(x, y, h * 0.5), look=look, space=frame, drop=(back, '-z'))
        for z in (rail * 0.5 + 0.06, split, h - rail * 0.5):
            town.box(m, (w - 2.0 * stile, 0.016, rail), at=(cx, y, z), look=look, space=frame, drop=(back, '-x', '+x'))
        town.box(m, (w - 2.0 * stile - 0.1, 0.012, split - rail - 0.2), at=(cx, y, (split + rail + 0.06) * 0.5),
                 look=look, space=frame, drop=(back,))
    knob_x = s * (w - 0.08)
    for side in (-1.0, 1.0):
        town.box(m, (0.05, 0.03, 0.05), at=(knob_x, side * (thick * 0.5 + 0.03), 1.0), look='brass', space=frame,
                 drop=('+y' if side < 0 else '-y',))


def hinged(origin, yaw):
    """A door leaf's frame: its hinge at origin (x, y, z), turned yaw degrees about z."""
    return Space(Matrix.Translation(Vector(origin)) @ Matrix.Rotation(math.radians(yaw), 4, 'Z'))


def flat(m, outline, z, uv, mat='trim', up=True, holes=(), every=1.6):
    """A horizontal one-sided face at height z (a floor facing up, a ceiling facing down): outline and holes are loops of
    (x, y) points. Sliced every `every` metres both ways, so the AO has vertices to live on."""
    tb = kit._new_bmesh()
    loops = [list(outline)] + [list(h) for h in holes]
    points = [p for loop in loops for p in loop]
    verts = [tb.verts.new((x, y, z)) for x, y in points]
    for a, b, c in tessellate_polygon([[Vector((x, y, 0.0)) for x, y in loop] for loop in loops]):
        try:
            f = tb.faces.new((verts[a], verts[b], verts[c]))
        except ValueError:
            continue
        f.normal_update()
        if (f.normal.z > 0.0) != up:
            f.normal_flip()
    tris = [f for f in tb.faces if len(f.verts) == 3]
    inner = [e for e in dict.fromkeys(e for f in tris for e in f.edges)
             if len(e.link_faces) == 2 and all(len(f.verts) == 3 for f in e.link_faces)]
    bmesh.ops.beautify_fill(tb, faces=tris, edges=inner, method='ANGLE')
    bmesh.ops.join_triangles(tb, faces=[f for f in tb.faces if len(f.verts) == 3], angle_face_threshold=0.02,
                             angle_shape_threshold=math.pi)
    xs = [p[0] for p in outline]
    ys = [p[1] for p in outline]
    if every:
        kit._slice(tb, 0, [x for x in kit.frange(min(xs) + every, max(xs) - 0.1, every)])
        kit._slice(tb, 1, [y for y in kit.frange(min(ys) + every, max(ys) - 0.1, every)])
    m.emit(tb, uv, mat)


def box_loop(x0, x1, y0, y1):
    """A rectangle's corners, counterclockwise from above."""
    return [(x0, y0), (x1, y0), (x1, y1), (x0, y1)]


def plaster(z0=0.0):
    """The trim sheet's cream plaster at world scale, its grain upright (FalseFronts' office). Turned upright, the strip
    runs up the wall, so a wall from z0 (over the space's floor) up 3.4 m is given the strip's cleanest 3.4 m (from 5.0 m
    on along it, round to 1.98 m): the stretch with the fewest chips."""
    return Trim('G', world=True, rotate=True, u=5.0 - z0)


class Unrolled:
    """A tileable set mapped by a function of each vertex in the part's frame: (u, v) in metres, scaled to the set's
    texel density (a vault's stone courses following its curve)."""

    def __init__(self, set_name, fn):
        self.set_name, self.fn = set_name, fn

    def apply(self, obj, rng):
        info = lt.SETS[self.set_name]
        scale = info['density'] / float(info['size'])
        du, dv = rng.uniform(0.0, 3.2), rng.uniform(0.0, 3.2)
        mesh = obj.data
        uv = mesh.uv_layers.active.data
        for loop in mesh.loops:
            u, v = self.fn(mesh.vertices[loop.vertex_index].co)
            uv[loop.index].uv = ((u + du) * scale, (v + dv) * scale)


# --- The false front's parts (FalseFronts.py's, the same calls) ---

FT = 0.16                       # the false front's thickness
WALL_T = 0.14


def siding(rng, rotate=False):
    return Trim('A', world=True, rotate=rotate, u=rng.uniform(0.0, 6.4))


def false_front(m, x0, x1, y0, top, items, uv, base, mat='trim', bottom=None, covered=False):
    """FalseFronts.false_front with a material: the facade panel from x0 to x1 at y0 (its front face), from the ground
    (bottom in the facade's space, whose z = 0 is the floor) up to the top profile, holes for the openings."""
    space = kit.wall_space((x0, y0), (x1, y0), base)
    width = x1 - x0
    bottom = -base if bottom is None else bottom
    outline = [(0.0, bottom), (width, bottom)] + list(reversed(top))
    holes = [kit.rect(o.x, max(o.z, 0.0), o.w, o.h) for o in items]
    m.panel(outline, holes, FT, uv, mat=mat, space=space, around=() if covered else items, slices=0)
    return space


def shell(m, X0, X1, YF, Y1, wall_h, pitch, sides, back, base, rotate=False, corner='C', trim_look='A'):
    """FalseFronts.shell: the side and back walls behind a false front (weathered siding) on a fieldstone plinth, their
    openings, corner boards, a water table and a frieze. Returns the walls' spaces by name."""
    rng = m.rng
    town.plinth(m, X0, X1, YF + 0.1, Y1, base, out=0.06)
    width = X1 - X0
    rise = width * 0.5 * math.tan(math.radians(pitch))
    length = Y1 - YF
    walls = {
        'right': ((X1, YF), (X1, Y1), None, sides.get('right', [])),
        'back': ((X1, Y1), (X0, Y1), (width * 0.5, wall_h + rise), back),
        'left': ((X0, Y1), (X0, YF), None, [Opening(o.kind, length - o.x - o.w, o.z, o.w, o.h, **o.opts)
                                            for o in sides.get('left', [])]),
    }
    spaces = {}
    for name, (p0, p1, apex, items) in walls.items():
        space = kit.wall_space(p0, p1, base)
        space.items = items
        spaces[name] = space
        span = (Vector(p1) - Vector(p0)).length
        town.panel(m, kit.wall_outline(span, wall_h, items, apex), kit.holes(items), WALL_T, siding(rng, rotate),
                   space=space, around=items, slices=4.0, back=False)
        town.openings(m, space, items, WALL_T)
        top = wall_h if apex is None else wall_h - 0.02
        for x in ((0.07, span - 0.07) if apex is not None else (span - 0.07,) if name == 'right' else (0.07,)):
            town.trim_board(m, space, (x, 0.0, -0.05), (x, 0.0, top), 0.15, corner, thick=0.045)
        spans = [(0.1, span - 0.1)]
        for o in items:
            if o.z <= 1e-4:
                spans = town.subtract(spans, (o.x - 0.2, o.x + o.w + 0.2))
        for a, b in spans:
            town.trim_board(m, space, (a, 0.0, 0.1), (b, 0.0, 0.1), 0.2, corner, thick=0.045)
        if apex is None:
            town.trim_board(m, space, (0.12, 0.0, wall_h - 0.14), (span - 0.12, 0.0, wall_h - 0.14), 0.24, trim_look,
                            thick=0.035)
    return spaces


def front_hulls(m, XF0, XF1, Y0, X0, X1, Y1, eave, pitch, facade_h, back_over=0.3, overhang=0.28):
    """FalseFronts.hulls: the facade, the walls and the roof."""
    m.hull((XF1 - XF0, FT + 0.04, facade_h), at=((XF0 + XF1) * 0.5, Y0 + FT * 0.5, facade_h * 0.5))
    m.hull((X1 - X0 + 0.1, Y1 - Y0 - FT, eave), at=((X0 + X1) * 0.5, (Y0 + FT + Y1) * 0.5, eave * 0.5))
    tan = math.tan(math.radians(pitch))
    ridge = eave + (X1 - X0) * 0.5 * tan + 0.25
    low = eave - overhang * tan + 0.2
    m.hull_points([(x, y, low) for x in (X0 - overhang, X1 + overhang) for y in (Y0 + FT, Y1 + back_over)] +
                  [((X0 + X1) * 0.5, y, ridge) for y in (Y0 + FT, Y1 + back_over)])


def roof_height(X0, X1, eave, pitch):
    """The roof's top over world x."""
    tan = math.tan(math.radians(pitch))
    half = (X1 - X0) * 0.5
    mid = (X0 + X1) * 0.5
    return lambda x: eave + max(0.0, half - abs(x - mid)) * tan + 0.16


def arch(x0, x1, z0, rise, segments=8):
    """Points of a segmental arch from (x0, z0) to (x1, z0) rising by rise in the middle (ends included)."""
    chord = x1 - x0
    radius = (chord * chord * 0.25 + rise * rise) / (2.0 * rise)
    cx, cz = (x0 + x1) * 0.5, z0 + rise - radius
    a0 = math.asin(min(1.0, chord * 0.5 / radius))
    return [(cx + radius * math.sin(-a0 + 2.0 * a0 * k / segments), cz + radius * math.cos(-a0 + 2.0 * a0 * k / segments))
            for k in range(segments + 1)]


def arch_z(x, x0, x1, z0, rise):
    """The height of that arch over x (z0 outside it)."""
    chord = x1 - x0
    if x <= x0 or x >= x1:
        return z0
    radius = (chord * chord * 0.25 + rise * rise) / (2.0 * rise)
    cx, cz = (x0 + x1) * 0.5, z0 + rise - radius
    return cz + math.sqrt(max(radius * radius - (x - cx) ** 2, 0.0))


def chimney_pipe(m, x, y, z_from, top, radius=0.1, socket='Smoke'):
    """A black iron stovepipe with a rain cap on three legs (FalseFronts' stores'), its smoke socket under the cap."""
    town.cylinder(m, (x, y, z_from), (x, y, top), radius, sides=10, look='iron', caps=(False, True))
    town.cylinder(m, (x, y, z_from + 0.25), (x, y, z_from + 0.47), radius * 2.2, radius * 1.1, sides=10, look='iron',
                  caps=(False, False))
    town.cylinder(m, (x, y, top + 0.12), (x, y, top + 0.3), radius * 2.6, 0.03, sides=10, look='iron')
    for k in range(3):
        a = math.radians(90.0 + 120.0 * k)
        town.box(m, (0.02, 0.02, 0.16), at=(x + radius * 0.9 * math.cos(a), y + radius * 0.9 * math.sin(a), top + 0.06),
                 look='iron')
    if socket:
        m.socket(socket, (x, y, top + 0.08))


# --- Openings in walls you see from both sides ---

def casing(m, space, o, look, out=0.0, cw=0.12, thick=0.035, sill=True, head_cap=True):
    """Casings round opening o on a wall face (the face at y = -out in space, the outside toward -y): two jambs and a
    head, and for a window a stool under it. Used on a room's side of an outer wall and on both faces of a partition."""
    top = o.z + o.h
    for x in (o.x - cw * 0.5, o.x + o.w + cw * 0.5):
        town.trim_board(m, space, (x, 0.0, o.z - (0.0 if o.z > 0.01 else 0.02)), (x, 0.0, top + cw), cw, look,
                        thick=thick, out=out)
    town.trim_board(m, space, (o.x - cw - 0.02, 0.0, top + cw * 0.6), (o.x + o.w + cw + 0.02, 0.0, top + cw * 0.6),
                    cw * 1.2, look, thick=thick + 0.008, out=out)
    if head_cap:
        town.box(m, (o.w + 2.0 * cw + 0.1, 0.05 + thick, 0.03), at=(o.x + o.w * 0.5, -out - (0.05 + thick) * 0.5,
                                                                    top + cw * 1.2 + 0.015), look=look, space=space)
    if sill and o.z > 0.01:
        town.box(m, (o.w + 2.0 * cw + 0.06, 0.09, 0.035), at=(o.x + o.w * 0.5, -out - 0.045, o.z - 0.0175), look=look,
                 space=space, drop=('+y',))


def reveal_floor(m, space, o, depth, look='C'):
    """A threshold across a doorway's full depth (y 0 to depth in space)."""
    town.box(m, (o.w + 0.04, depth + 0.02, 0.03), at=(o.x + o.w * 0.5, depth * 0.5, 0.015), look=look, space=space,
             drop=('-z',))


def plank_leaf(m, frame, w, h, look='A', straps=True, battens_on=-1.0, thick=0.045, brace=True, hinge_left=True):
    """A plank door leaf in frame (a Space): its hinge edge on the frame's z axis, the leaf along +x (hinge_left) or -x
    from it, its outside face toward -y; both faces built (it's seen open from inside and out). Battens and a brace on
    the battens_on side (-1: the -y face), iron straps over the battens."""
    rng = m.rng
    sign = 1.0 if hinge_left else -1.0
    boards = max(3, int(round(w / 0.19)))
    bw = w / boards
    for k in range(boards):
        x = sign * bw * (k + 0.5)
        town.board(m, (x, 0.0, 0.0), (x, 0.0, h - rng.uniform(0.0, 0.02)), bw - 0.006, thick, look=look, space=frame)
    by = battens_on * (thick * 0.5 + 0.0175)
    for z in (0.3, h - 0.35):
        town.board(m, (sign * 0.06, by, z), (sign * (w - 0.06), by, z), 0.15, 0.035, look=look, space=frame,
                   drop=('+y',) if battens_on < 0 else ('-y',))
    if brace:
        town.board(m, (sign * (w - 0.1), by, 0.42), (sign * 0.1, by, h - 0.45), 0.12, 0.03, look=look, space=frame,
                   drop=('+y',) if battens_on < 0 else ('-y',))
    if straps:
        sy = battens_on * (thick * 0.5 + 0.04)
        for z in (0.3, h - 0.35):
            town.board(m, (sign * -0.04, sy, z), (sign * w * 0.7, sy, z), 0.06, 0.012, look='H3', space=frame,
                       drop=('+y',) if battens_on < 0 else ('-y',))


def doorway_trim(m, a, b, z0, thick, o, look, both=True, threshold=True):
    """Casings on both faces of a doorway o in a wall from a to b (its outer or first face's line; the wall thick toward
    the left of a -> b), and a threshold board."""
    front = kit.wall_space(a, b, z0)
    casing(m, front, o, look, cw=0.11)
    if both:
        length = (Vector(b) - Vector(a)).length
        along = (Vector(b) - Vector(a)).normalized()
        inward = Vector((-along.y, along.x))
        a2 = Vector(a) + inward * thick
        b2 = Vector(b) + inward * thick
        back = kit.wall_space(tuple(b2), tuple(a2), z0)
        mirrored = Opening(o.kind, length - o.x - o.w, o.z, o.w, o.h, **o.opts)
        casing(m, back, mirrored, look, cw=0.11)
    if threshold and o.z < 0.01:
        reveal_floor(m, front, o, thick)
    return front


def room_window(m, space, o, depth, paint='H2', sash=None, broken=(), open_lower=False, shards=True, lit=False):
    """A window seen from both sides (a room's): the outer casing, a sill, and the sash at the middle of the wall: its
    stiles, rails, meeting rail and muntins built whole (the town's windows leave out their backs), the panes as thin
    glass both faces of which show. broken lists panes gone (by number, row by row from the bottom left), shards leaves a
    jagged piece in some of them; open_lower pushes the lower sash up behind the upper (the lower half open)."""
    rng = m.rng
    sash = sash or paint
    x0, z0, w, h = o.x, o.z, o.w, o.h
    cw = o.opts.get('casing_w', 0.11)
    for x in (x0 - cw * 0.5, x0 + w + cw * 0.5):
        town.trim_board(m, space, (x, 0.0, z0 - 0.02), (x, 0.0, z0 + h + cw), cw, paint, thick=0.04)
    head = z0 + h + cw * 0.65
    town.trim_board(m, space, (x0 - cw - 0.02, 0.0, head), (x0 + w + cw + 0.02, 0.0, head), cw * 1.3, paint, thick=0.048)
    town.box(m, (w + 2.0 * cw + 0.1, 0.16, 0.06), at=(x0 + w * 0.5, -0.03, z0 - 0.03),
             rot=(rng.uniform(-1.5, 1.5), 0.0, 0.0), look='C', space=space, drop=('+y',))
    gy = depth * 0.5
    cols, rows = o.opts.get('panes', (2, 2))
    half_rows = max(1, rows // 2)
    lower_y = gy + 0.045 if open_lower else gy

    def bar(p0, p1, wdt, y, drop=()):
        town.board(m, (p0[0], y, p0[1]), (p1[0], y, p1[1]), wdt, 0.035, look=sash, space=space, drop=drop)
    stile = 0.045
    # The upper sash, then the lower one (raised behind the upper when open_lower).
    for k, (zs0, zs1, y) in enumerate(((z0 + h * 0.5, z0 + h, gy),
                                       ((z0 + h * 0.5 - 0.02, z0 + h - 0.02, lower_y) if open_lower else
                                        (z0, z0 + h * 0.5, gy)))):
        for x in (x0 + stile * 0.5, x0 + w - stile * 0.5):
            bar((x, zs0), (x, zs1), stile, y, drop=('-x', '+x'))
        bar((x0 + stile, zs0 + 0.03), (x0 + w - stile, zs0 + 0.03), 0.06 if k else 0.05, y, drop=('-x', '+x'))
        bar((x0 + stile, zs1 - 0.025), (x0 + w - stile, zs1 - 0.025), 0.05, y, drop=('-x', '+x', '+z'))
        for i in range(1, cols):
            x = x0 + w * i / cols
            bar((x, zs0 + 0.05), (x, zs1 - 0.05), 0.028, y, drop=('-z', '+z'))
        for j in range(1, half_rows):
            z = zs0 + (zs1 - zs0) * j / half_rows
            bar((x0 + stile, z), (x0 + w - stile, z), 0.028, y, drop=('-x', '+x'))
        # The panes: thin glass, both faces.
        pw = (w - 2.0 * stile) / cols
        ph = (zs1 - zs0 - 0.08) / half_rows
        for j in range(half_rows):
            for i in range(cols):
                number = (j + (half_rows if k == 0 else 0)) * cols + i
                cx = x0 + stile + pw * (i + 0.5)
                cz = zs0 + 0.055 + ph * (j + 0.5)
                if number in broken:
                    if shards and rng.random() < 0.55:
                        # A jagged piece left in the bottom corner of the frame: a sheet of glass, both faces.
                        side = rng.choice((-1.0, 1.0))
                        tri = [(cx + side * pw * 0.45, cz - ph * 0.45), (cx - side * pw * rng.uniform(-0.1, 0.35),
                                                                          cz - ph * 0.45),
                               (cx + side * pw * 0.45, cz + ph * rng.uniform(-0.05, 0.3))]
                        for face in (-1.0, 1.0):
                            face_part(m, [(px, y + face * 0.002, pz) for px, pz in tri], Trim('H4', fit=True),
                                      facing=(0.0, face, 0.0), space=space)
                    continue
                m.box((pw, 0.006, ph), at=(cx, y, cz), uv=Trim('H4', fit=True), mat='windowglow' if lit else 'trim',
                      space=space, drop=('-x', '+x', '-z', '+z'))


def window_inside(m, a, b, z0, thick, o, look='C', stool=True):
    """The room's side of a window o in the outer wall a -> b (outside on the right): its casing and stool on the inner
    face."""
    length = (Vector(b) - Vector(a)).length
    along = (Vector(b) - Vector(a)).normalized()
    inward = Vector((-along.y, along.x))
    a2, b2 = Vector(a) + inward * thick, Vector(b) + inward * thick
    back = kit.wall_space(tuple(b2), tuple(a2), z0)
    mirrored = Opening(o.kind, length - o.x - o.w, o.z, o.w, o.h, **o.opts)
    casing(m, back, mirrored, look, cw=0.1, sill=stool)
    return back


def inner_holes(items, length, thick):
    """A wall's openings as holes in its inner face (wall_face): along the inner face from the wall's far end, inset by
    the thickness at both corners."""
    return [(length - thick - (o.x + o.w), length - thick - o.x, max(o.z, 0.0), o.z + o.h) for o in items]


# --- Stairs ---

def flight(m, start, direction, width, count, rise, run, z0, look='A', top_tread=True, risers=True,
           stringers=(True, True), hull=True, solid=True, nose=0.025):
    """A straight flight up from a floor at z0: its first riser's foot at start (the middle of its width), climbing along
    direction (a plan unit vector) count risers of rise, each with its tread (run deep) on top; without top_tread the
    last riser steps onto a landing. Stringers on the sides asked for (left, right as you climb). The hull is a ramp
    through the treads' noses (solid: down to the floor, else a slab under it). Returns the plan point and height where
    the climb ends (the last nose)."""
    d = Vector((direction[0], direction[1], 0.0)).normalized()
    side = Vector((d.y, -d.x, 0.0))          # to the right as you climb
    o = Vector((start[0], start[1], z0))
    treads = count if top_tread else count - 1
    for k in range(1, count + 1):
        z = z0 + k * rise
        a = o + d * ((k - 1) * run)
        if risers:
            p = a + Vector((0.0, 0.0, -rise * 0.5))
            town.board(m, p - side * (width * 0.5 - 0.01), p + side * (width * 0.5 - 0.01), rise, 0.025, look=look,
                       face=(-d.x, -d.y, 0.0), lift=-0.0125, drop=('+y',))
        if k <= treads:
            c = a + d * (run * 0.5 - nose * 0.5) + Vector((0.0, 0.0, -0.022))
            town.board(m, c - side * width * 0.5, c + side * width * 0.5, run + nose, 0.044, look=look,
                       face=(0.0, 0.0, 1.0), drop=('+y',))
    end = o + d * (treads * run)
    top = z0 + count * rise
    for k, on in enumerate(stringers):
        if not on:
            continue
        s = -1.0 if k == 0 else 1.0
        p0 = o + side * s * (width * 0.5 + 0.025) + Vector((0.0, 0.0, 0.1))
        p1 = end + side * s * (width * 0.5 + 0.025) + Vector((0.0, 0.0, top - z0 + 0.1))
        town.board(m, p0 - d * 0.05, p1, 0.28, 0.05, look='C', face=tuple(side * s), drop=())
    if hull:
        slope = rise / run
        n0 = o - d * run
        n1 = o + d * ((count - 1) * run)
        pts = []
        for s in (-0.5, 0.5):
            w = side * width * s
            pts += [n0 + w, n1 + w + Vector((0.0, 0.0, top - z0))]
            if solid:
                pts += [n1 + w]
            else:
                pts += [n0 + w + Vector((0.0, 0.0, -0.05)), n1 + w + Vector((0.0, 0.0, top - z0 - 0.3))]
        m.hull_points([tuple(p) for p in pts])
    return end, top


def handrail(m, p0, p1, height=0.9, posts=True, every=0.18, look='C', baluster_look='C', newels=True):
    """A railing following the line p0 -> p1 (3D points on the stair's or floor's edge): a handrail height over it,
    square balusters every `every`, newel posts at its ends."""
    p0, p1 = Vector(p0), Vector(p1)
    lift = Vector((0.0, 0.0, height))
    town.board(m, p0 + lift, p1 + lift, 0.07, 0.06, look=look, face=(0.0, 0.0, 1.0))
    length = (p1 - p0).length
    count = max(1, int(length / every))
    # Balusters and newels as boards (mapped along their length: a box this tall would squeeze the trim strip).
    for k in range(1, count):
        q = p0 + (p1 - p0) * (k / count)
        town.board(m, (q.x, q.y, q.z), (q.x, q.y, q.z + height - 0.04), 0.035, 0.035, look=baluster_look,
                   drop=('-x', '+x'))
    if newels:
        for q in (p0, p1):
            town.board(m, (q.x, q.y, q.z), (q.x, q.y, q.z + height + 0.12), 0.11, 0.11, look=look, drop=('-x',))
            town.box(m, (0.15, 0.15, 0.05), at=(q.x, q.y, q.z + height + 0.145), look=look)


# --- Furniture and fittings ---

def lamp_hanging(m, at, ceiling, socket=None, glow=True):
    """A railroad lamp hung from a ceiling at height ceiling over at (x, y, the flame's height): a rod down from a ceiling
    plate, a smoke bell, the brass font and its lit chimney (LanternGlow). The socket sits at the flame."""
    x, y, z = at
    town.box(m, (0.14, 0.14, 0.02), at=(x, y, ceiling - 0.01), look='iron', drop=('+z',))
    town.box(m, (0.018, 0.018, ceiling - z - 0.32), at=(x, y, (ceiling + z + 0.32) * 0.5), look='iron', drop=('-z', '+z'))
    town.lathe(m, [(0.0, 0.0), (0.2, 0.0), (0.05, 0.08), (0.0, 0.09)], at=(x, y, z + 0.25), sides=8, look='iron')
    town.lathe(m, [(0.0, 0.0), (0.08, 0.0), (0.09, 0.06), (0.03, 0.12), (0.0, 0.125)], at=(x, y, z - 0.17), sides=6,
               look='brass')
    town.lathe(m, [(0.03, 0.0), (0.065, 0.08), (0.035, 0.2), (0.0, 0.2)], at=(x, y, z - 0.06), sides=6,
               look='glow' if glow else 'H4')
    if socket:
        m.socket(socket, (x, y, z))


def lamp_bracket(m, wall_point, outward, z, socket=None, glow=True):
    """A lamp on an iron wall bracket: wall_point (x, y) on the wall's face, outward the plan direction out of it."""
    d = Vector((outward[0], outward[1], 0.0)).normalized()
    w = Vector((wall_point[0], wall_point[1], 0.0))
    lamp = w + d * 0.26
    town.box(m, (0.08, 0.08, 0.2), at=(w.x + d.x * 0.01, w.y + d.y * 0.01, z + 0.25), look='iron')
    town.board(m, (w.x, w.y, z + 0.32), (lamp.x, lamp.y, z + 0.32), 0.025, 0.025, look='iron', face=(0.0, 0.0, 1.0))
    town.lathe(m, [(0.0, 0.0), (0.075, 0.0), (0.085, 0.05), (0.055, 0.1), (0.0, 0.11)], at=(lamp.x, lamp.y, z - 0.17),
               sides=8, look='brass')
    town.lathe(m, [(0.025, 0.0), (0.06, 0.05), (0.065, 0.12), (0.035, 0.2), (0.0, 0.2)], at=(lamp.x, lamp.y, z - 0.07),
               sides=6, look='glow' if glow else 'H4')
    town.box(m, (0.012, 0.012, 0.2), at=(lamp.x, lamp.y, z + 0.22), look='iron', drop=('-z', '+z'))
    if socket:
        m.socket(socket, (lamp.x, lamp.y, z))


def lamp_table(m, at, socket=None, glow=True):
    """A small oil lamp standing on a table at at (x, y, the table's top)."""
    x, y, z = at
    town.lathe(m, [(0.0, 0.0), (0.075, 0.0), (0.08, 0.02), (0.03, 0.06), (0.06, 0.12), (0.045, 0.17), (0.0, 0.18)],
               at=(x, y, z), sides=8, look='brass')
    town.lathe(m, [(0.025, 0.0), (0.05, 0.05), (0.055, 0.1), (0.03, 0.18), (0.0, 0.18)], at=(x, y, z + 0.18), sides=6,
               look='glow' if glow else 'H4')
    if socket:
        m.socket(socket, (x, y, z + 0.26))


def bench(m, x0, x1, y_back, z, facing, back=True, legs=2, hull=True, space=None):
    """A slatted station bench along X from x0 to x1 (in space), its back toward y_back and its seat facing `facing` (+1
    toward +y, -1 toward -y), standing on the floor at z, with a back rest of two boards when back."""
    rng = m.rng
    f = facing
    seat = z + 0.45
    for k, dy in enumerate((0.12, 0.28, 0.44)):
        y = y_back + f * dy
        town.board(m, (x0 + rng.uniform(0.0, 0.02), y, seat - 0.022), (x1 - rng.uniform(0.0, 0.02), y, seat - 0.022),
                   0.14, 0.04, look='A', face=(0.0, 0.0, 1.0), space=space)
    if back:
        for zz in (z + 0.72, z + 0.9):
            town.board(m, (x0, y_back + f * 0.06, zz), (x1, y_back + f * 0.06, zz), 0.14, 0.035, look='A',
                       face=(0.0, f, 0.0), space=space)
    for k in range(legs):
        x = x0 + 0.12 + (x1 - x0 - 0.24) * k / max(legs - 1, 1)
        town.board(m, (x, y_back + f * 0.48, z), (x, y_back + f * 0.48, seat - 0.045), 0.06, 0.06, look='C',
                   face=(1.0, 0.0, 0.0), drop=('-z',), space=space)
        town.board(m, (x, y_back + f * 0.05, z), (x, y_back + f * 0.05, (z + 0.98) if back else seat - 0.045), 0.06,
                   0.06, look='C', face=(1.0, 0.0, 0.0), drop=('-z',), space=space)
        town.board(m, (x, y_back + f * 0.02, seat - 0.07), (x, y_back + f * 0.52, seat - 0.07), 0.06, 0.05, look='C',
                   face=(1.0, 0.0, 0.0), space=space)
    if hull:
        m.hull((x1 - x0, 0.52, (0.98 if back else 0.45)), at=((x0 + x1) * 0.5, y_back + f * 0.27,
                                                                z + (0.98 if back else 0.45) * 0.5), space=space)


def polygon(m, points, uv, mat='trim', facing=None, space=None):
    """A one-sided flat polygon (any shape, concave too) through points (x, y, z), filled by tessellation, turned to face
    facing."""
    pts = [Vector(p) for p in points]
    # tessellate_polygon fills in the xy plane: project onto the plane the polygon faces most (Newell's normal).
    n = Vector((0.0, 0.0, 0.0))
    for i, p in enumerate(pts):
        q = pts[(i + 1) % len(pts)]
        n += Vector(((p.y - q.y) * (p.z + q.z), (p.z - q.z) * (p.x + q.x), (p.x - q.x) * (p.y + q.y)))
    axis = max(range(3), key=lambda i: abs(n[i]))
    keep = [i for i in range(3) if i != axis]
    flat2d = [Vector((p[keep[0]], p[keep[1]], 0.0)) for p in pts]
    tb = kit._new_bmesh()
    verts = [tb.verts.new(p) for p in pts]
    for a, b, c in tessellate_polygon([flat2d]):
        try:
            f = tb.faces.new((verts[a], verts[b], verts[c]))
        except ValueError:
            continue
        f.normal_update()
        if facing is not None and f.normal.dot(Vector(facing)) < 0.0:
            f.normal_flip()
    bmesh.ops.join_triangles(tb, faces=[f for f in tb.faces if len(f.verts) == 3], angle_face_threshold=0.02,
                             angle_shape_threshold=math.pi)
    m.emit(tb, uv, mat, None, space)


def stove(m, at, pipe_to, socket=None, ring=0.29):
    """A pot-bellied stove standing on at (x, y, floor): its legs, the cast belly (black iron), a door, a top, its pipe
    up to pipe_to (z, a ceiling) with a collar there."""
    x, y, z = at
    for k in range(4):
        a = math.radians(45.0 + 90.0 * k)
        town.box(m, (0.05, 0.05, 0.18), at=(x + 0.2 * math.cos(a), y + 0.2 * math.sin(a), z + 0.09),
                 rot=(0.0, 0.0, math.degrees(a)), look='iron', drop=('-z',))
    profile = [(0.0, 0.16), (0.25, 0.16), (ring, 0.42), (ring * 1.02, 0.64), (0.22, 0.88), (0.25, 0.94), (0.12, 1.04),
               (0.0, 1.05)]
    town.lathe(m, profile, at=(x, y, z), sides=10, look='iron')
    town.box(m, (0.22, 0.04, 0.18), at=(x, y - ring - 0.005, z + 0.55), look='iron')
    town.box(m, (0.03, 0.03, 0.03), at=(x + 0.08, y - ring - 0.035, z + 0.55), look='brass')
    town.cylinder(m, (x, y, z + 1.04), (x, y, pipe_to), 0.075, sides=8, look='iron', caps=(False, False),
                  face=(0.0, 0.0, 1.0))
    town.cylinder(m, (x, y, pipe_to - 0.06), (x, y, pipe_to), 0.13, sides=8, look='iron', caps=(True, False),
                  face=(0.0, 0.0, 1.0))
    m.hull((0.64, 0.64, 1.06), at=(x, y, z + 0.53))
    if socket:
        m.socket(socket, (x, y, z + 0.55))


def table(m, x, y, z, w, d, h=0.76, look='C', top_look='A', yaw=0.0, hull=True):
    frame = Space(Matrix.Translation((x, y, z)) @ Matrix.Rotation(math.radians(yaw), 4, 'Z'))
    town.box(m, (w, d, 0.04), at=(0.0, 0.0, h - 0.02), look=top_look, space=frame)
    for sx in (-1.0, 1.0):
        for sy in (-1.0, 1.0):
            town.box(m, (0.05, 0.05, h - 0.04), at=(sx * (w * 0.5 - 0.06), sy * (d * 0.5 - 0.06), (h - 0.04) * 0.5),
                     look=look, space=frame, drop=('-z', '+z'))
    for sy in (-1.0, 1.0):
        town.box(m, (w - 0.14, 0.025, 0.08), at=(0.0, sy * (d * 0.5 - 0.06), h - 0.08), look=look, space=frame,
                 drop=('-x', '+x'))
    if hull:
        m.hull((w, d, h), at=(0.0, 0.0, h * 0.5), space=frame)


def chair(m, x, y, z, yaw=0.0, fallen=False):
    """A plain kitchen chair at (x, y) on the floor z, its seat facing -y turned by yaw (a fallen one lies on its back)."""
    base = Matrix.Translation((x, y, z)) @ Matrix.Rotation(math.radians(yaw), 4, 'Z')
    if fallen:
        base = base @ Matrix.Translation((0.0, 0.2, 0.21)) @ Matrix.Rotation(math.radians(-90.0), 4, 'X')
    frame = Space(base)
    town.box(m, (0.42, 0.42, 0.035), at=(0.0, 0.0, 0.45), look='C', space=frame)
    town.box(m, (0.4, 0.03, 0.3), at=(0.0, 0.195, 0.75), look='C', space=frame)
    for sx in (-1.0, 1.0):
        for sy in (-1.0, 1.0):
            hgt = 0.92 if sy > 0 else 0.44
            town.board(m, (sx * 0.18, sy * 0.18, 0.0), (sx * 0.18, sy * 0.18, hgt), 0.035, 0.035, look='C', space=frame,
                       drop=('-x',))


def coffin_points(length, foot, shoulder, head):
    """A coffin's outline in plan (x along it from the foot at 0, y across), widest at the shoulders."""
    s = length * 0.72
    return [(0.0, -foot * 0.5), (s, -shoulder * 0.5), (length, -head * 0.5), (length, head * 0.5), (s, shoulder * 0.5),
            (0.0, foot * 0.5)]


def coffin(m, frame, length=1.95, foot=0.34, shoulder=0.6, head=0.44, depth=0.36, lid='on', look='woodcream',
           lid_look=None, brass=False, wall=0.03):
    """A six-sided pine coffin in frame (a Space: its foot at the origin, lying along +x, its bottom at z = 0): open (lid
    'off', the inside showing), 'on', 'ajar' (slid off at the head, cocked) or 'none'. Brass handles if brass."""
    rng = m.rng
    outer = coffin_points(length, foot, shoulder, head)
    inner = coffin_points(length - 2.0 * wall, foot - 2.0 * wall, shoulder - 2.0 * wall, head - 2.0 * wall)
    inner = [(x + wall, y) for x, y in inner]
    tb = kit._new_bmesh()
    ob = [tb.verts.new((x, y, 0.0)) for x, y in outer]
    ot = [tb.verts.new((x, y, depth)) for x, y in outer]
    n = len(outer)
    for i in range(n):
        j = (i + 1) % n
        tb.faces.new((ob[i], ob[j], ot[j], ot[i]))
    # The outline runs counterclockwise from above, so these windings face out, up and in (no recalculation: an open
    # shell's normals could come out all reversed).
    if lid in ('off', 'ajar', 'none'):
        it = [tb.verts.new((x, y, depth)) for x, y in inner]
        ib = [tb.verts.new((x, y, 0.04)) for x, y in inner]
        for i in range(n):
            j = (i + 1) % n
            tb.faces.new((ot[i], ot[j], it[j], it[i]))
            tb.faces.new((it[i], it[j], ib[j], ib[i]))
        tb.faces.new(ib)
    spec, key = town.finish(look, width=depth)
    if isinstance(spec, town.PlankRow):
        spec = homes.Boards(rotate=True)
    m.emit(tb, spec, key, None, frame)
    if lid in ('on', 'ajar'):
        lid_pts = coffin_points(length + 0.04, foot + 0.04, shoulder + 0.04, head + 0.04)
        if lid == 'ajar':
            lid_frame = Space(frame.matrix @ Matrix.Translation((0.35, 0.12, depth + 0.02)) @
                              Matrix.Rotation(math.radians(rng.uniform(9.0, 16.0)), 4, 'Z') @
                              Matrix.Rotation(math.radians(-3.0), 4, 'X'))
        else:
            lid_frame = Space(frame.matrix @ Matrix.Translation((-0.02, 0.0, depth)))
        lb = kit._new_bmesh()
        lo = [lb.verts.new((x, y, 0.0)) for x, y in lid_pts]
        hi = [lb.verts.new((x, y, 0.035)) for x, y in lid_pts]
        lb.faces.new(hi)
        lb.faces.new(list(reversed(lo)))
        for i in range(n):
            j = (i + 1) % n
            lb.faces.new((lo[i], lo[j], hi[j], hi[i]))
        bmesh.ops.recalc_face_normals(lb, faces=lb.faces[:])
        m.emit(lb, homes.Boards(rotate=True), town.finish(lid_look or look)[1], None, lid_frame)
    if brass:
        for xx in (length * 0.3, length * 0.62):
            for s in (-1.0, 1.0):
                half = shoulder * 0.5 - (shoulder - foot) * 0.5 * abs(length * 0.72 - xx) / (length * 0.72)
                town.box(m, (0.16, 0.025, 0.035), at=(xx, s * (half + 0.03), depth * 0.55), look='brass', space=frame)


def trestle(m, frame, width=0.66, height=0.62, look='C'):
    """A sawhorse trestle in frame (a Space): its top beam along y across width at height, two pairs of splayed legs."""
    town.board(m, (0.0, -width * 0.5, height - 0.045), (0.0, width * 0.5, height - 0.045), 0.09, 0.09, look=look,
               face=(0.0, 0.0, 1.0), space=frame)
    for sy in (-1.0, 1.0):
        for sx in (-1.0, 1.0):
            town.board(m, (sx * 0.035, sy * (width * 0.5 - 0.06), height - 0.06), (sx * 0.2, sy * (width * 0.5 - 0.02), 0.0),
                       0.06, 0.045, look=look, face=(sx, 0.0, 0.0), space=frame)
    town.board(m, (0.0, -width * 0.5 + 0.08, height * 0.35), (0.0, width * 0.5 - 0.08, height * 0.35), 0.06, 0.03,
               look=look, space=frame, face=(1.0, 0.0, 0.0))


def shelves(m, frame, length, depth, height, levels, look='C', board_look='A', back=False):
    """Open shelving in frame (its foot's middle at the origin, along x, its back toward +y): uprights and boards."""
    for x in kit.frange(-length * 0.5 + 0.04, length * 0.5 + 0.01, max(0.9, length / max(1, round(length / 1.2)))):
        for y in (-depth * 0.5 + 0.03, depth * 0.5 - 0.03):
            town.box(m, (0.05, 0.05, height), at=(x, y, height * 0.5), look=look, space=frame, drop=('-z',))
    for z in levels:
        town.board(m, (-length * 0.5, 0.0, z - 0.015), (length * 0.5, 0.0, z - 0.015), depth, 0.03, look=board_look,
                   face=(0.0, 0.0, 1.0), space=frame)
    if back:
        town.box(m, (length, 0.02, height), at=(0.0, depth * 0.5, height * 0.5), look=board_look, space=frame,
                 drop=('+y',))


def chalk(m, space, lines, at, height=0.05, look='cream', tol=0.06, gap=1.6, rot_jitter=1.5, align='left'):
    """Lines of chalk lettering on a board (the plane y = at[1] in space, facing -y): each line's ink height `height`,
    its top line first, a little crooked; returns the lines' widths."""
    rng = m.rng
    x, y, z = at
    widths = []
    for k, line in enumerate(lines):
        if not line:
            continue
        w, _ = town.text(m, line, (x, y, z - k * height * gap), height=height, space=space, look=look, tol=tol,
                         lift=0.0015, rot=rng.uniform(-rot_jitter, rot_jitter), align=align)
        widths.append(w)
    return widths


def strike(m, space, x0, x1, z, y, look='cream', width=0.012):
    """A chalk line struck through a word."""
    town.box(m, (x1 - x0, 0.003, width), at=((x0 + x1) * 0.5, y - 0.002, z), rot=(0.0, m.rng.uniform(-3.0, 3.0), 0.0),
             look=look, space=space, drop=('+y',))


def long_gun(m, frame, kind='rifle', look_wood='C'):
    """A long gun stood on its butt in frame (its butt at the origin, its barrel up +z, its side toward -y): a stock, a
    receiver and a barrel (and a second barrel for a shotgun); simple, seen through a hatch."""
    town.prism(m, [(-0.05, 0.0), (0.06, 0.0), (0.04, 0.32), (0.02, 0.42), (-0.015, 0.42), (-0.03, 0.3)], 0.04,
               look=look_wood, space=frame, y=-0.02)
    town.box(m, (0.05, 0.04, 0.2), at=(0.005, 0.0, 0.52), look='iron', space=frame)
    length = 0.62 if kind == 'rifle' else 0.5
    town.box(m, (0.035, 0.035, 0.34), at=(0.0, 0.0, 0.6 + 0.17), look=look_wood, space=frame)
    for dx in ((0.0,) if kind == 'rifle' else (-0.011, 0.011)):
        town.box(m, (0.018, 0.018, length), at=(dx, 0.0, 0.62 + length * 0.5), look='iron', space=frame,
                 drop=('-z',))


def tag(m, at, space=None, look='band_G', yaw=0.0):
    """A paper tag (a little cream card hung at at, a little askew)."""
    x, y, z = at
    town.box(m, (0.06, 0.004, 0.09), at=(x, y, z - 0.045), rot=(0.0, m.rng.uniform(-12.0, 12.0), yaw), look=look,
             space=space, drop=('-z', '+z'))


def trunk(m, x, y, z, w, d, h, yaw=0.0, look='woodoxide', hull=True):
    """A steamer trunk: a box of painted boards with a domed-ish lid, iron corners and straps, brass lock."""
    frame = Space(Matrix.Translation((x, y, z)) @ Matrix.Rotation(math.radians(yaw), 4, 'Z'))
    town.box(m, (w, d, h * 0.8), at=(0.0, 0.0, h * 0.4), look=look, space=frame, drop=('-z',))
    town.box(m, (w + 0.01, d + 0.01, h * 0.2), at=(0.0, 0.0, h * 0.9), look=look, space=frame)
    for sx in (-1.0, 1.0):
        town.box(m, (0.05, d + 0.02, h * 0.98), at=(sx * (w * 0.5 - 0.12), 0.0, h * 0.49), look='C', space=frame,
                 drop=('-z',))
    town.box(m, (0.06, 0.02, 0.07), at=(0.0, -d * 0.5 - 0.01, h * 0.78), look='brass', space=frame)
    if hull:
        m.hull((w, d, h), at=(0.0, 0.0, h * 0.5), space=frame)


def crate(m, x, y, z, w, d, h, yaw=0.0, hull=True):
    frame = Space(Matrix.Translation((x, y, z)) @ Matrix.Rotation(math.radians(yaw), 4, 'Z'))
    town.box(m, (w, d, h), at=(0.0, 0.0, h * 0.5), look='planks', space=frame, drop=('-z',))
    for sx in (-1.0, 1.0):
        town.box(m, (0.06, d + 0.02, h + 0.01), at=(sx * (w * 0.5 - 0.03), 0.0, h * 0.5), look='C', space=frame,
                 drop=('-z',))
    if hull:
        m.hull((w, d, h), at=(0.0, 0.0, h * 0.5), space=frame)


def hay_mound(m, cx, cy, z, rx, ry, top, seed_turn=0.0, rings=4, sides=12):
    """A heap of loose hay: a lumpy dome (Hay, world scale) over an ellipse rx by ry at z, top metres high."""
    rng = m.rng
    tb = kit._new_bmesh()
    rows = []
    for j in range(rings + 1):
        t = j / rings
        r = math.cos(t * math.pi * 0.5)
        h = top * math.sin(t * math.pi * 0.5) ** 0.8
        if j == rings:
            rows.append([tb.verts.new((cx, cy, z + top))])
            continue
        row = []
        for i in range(sides):
            a = seed_turn + 2.0 * math.pi * i / sides
            k = 1.0 + rng.uniform(-0.08, 0.08) * (1.0 if j else 0.5)
            row.append(tb.verts.new((cx + rx * r * k * math.cos(a), cy + ry * r * k * math.sin(a),
                                     z + h + (rng.uniform(-0.04, 0.06) if j else -0.03))))
        rows.append(row)
    for j in range(rings):
        a, b = rows[j], rows[j + 1]
        for i in range(sides):
            i1 = (i + 1) % sides
            if len(b) == 1:
                tb.faces.new((a[i], a[i1], b[0]))
            else:
                tb.faces.new((a[i], a[i1], b[i1], b[i]))
    bmesh.ops.recalc_face_normals(tb, faces=tb.faces[:])
    m.emit(tb, Tile('Hay'), 'hay', smooth=True)


# --- Previews ---

def ensure_dir(path):
    os.makedirs(os.path.dirname(path), exist_ok=True)
    return path


def cutaway(obj, z_cut=None, keep=None, name=None):
    """A copy of obj for the views of its rooms: without the faces above z_cut (all their corners), or only the faces
    keep(center) says to keep. Never exported (its name starts with '_')."""
    copy = obj.copy()
    copy.data = obj.data.copy()
    copy.name = name or ('_Cut_' + obj.name)
    bpy.context.scene.collection.objects.link(copy)
    bm = bmesh.new()
    bm.from_mesh(copy.data)
    if z_cut is not None:
        # Sliced through at the cut, everything over it gone: a clean section, walls and all.
        bmesh.ops.bisect_plane(bm, geom=bm.verts[:] + bm.edges[:] + bm.faces[:], dist=1e-5,
                               plane_co=(0.0, 0.0, z_cut), plane_no=(0.0, 0.0, 1.0), clear_outer=True)
    gone = [f for f in bm.faces if keep is not None and not keep(f.calc_center_median())]
    bmesh.ops.delete(bm, geom=gone, context='FACES')
    bm.to_mesh(copy.data)
    bm.free()
    for child in list(copy.children):
        child.parent = None
    return copy


def lamp_lights(obj, prefix='SOCKET_Light', energy=60.0, color=(1.0, 0.72, 0.42), radius=0.05, skip=()):
    """Point lights at the model's lamp sockets, for the views inside (the game lights them with its own lamps)."""
    lights = []
    bpy.context.view_layer.update()
    for child in obj.children:
        if not child.name.startswith(prefix) or any(s in child.name for s in skip):
            continue
        data = bpy.data.lights.new('_Lamp_' + child.name, 'POINT')
        data.energy, data.color, data.shadow_soft_size = energy, color, radius
        light = bpy.data.objects.new('_Lamp_' + child.name, data)
        bpy.context.scene.collection.objects.link(light)
        light.location = child.matrix_world.translation
        lights.append(light)
    return lights


def remove(objs):
    for o in objs:
        data = o.data
        bpy.data.objects.remove(o)
        if isinstance(data, bpy.types.Mesh):
            bpy.data.meshes.remove(data)
        elif isinstance(data, bpy.types.Light):
            bpy.data.lights.remove(data)


_STAGE = {}


def stage():
    """looter_homes' set (Eevee, the street's dirt, the golden sun), made once per run, and a 1.8 m figure."""
    if not _STAGE:
        scene, sun = homes.stage()
        human = homes.figure()
        _STAGE.update(scene=scene, sun=sun, human=human)
    return _STAGE['scene'], _STAGE['sun'], _STAGE['human']


def exterior(obj, out_name, view=homes.VIEW, sun=homes.SUN, figure='right', lens=35.0, margin=1.04, exposure=-0.15,
             height_bias=0.42, extra=(), size=homes.SHOT):
    """The model in Ransom's Rest's golden late afternoon from a three-quarter view, with the figure beside it."""
    scene, sun_obj, human = stage()
    homes.light(scene, sun_obj, homes.golden_sun(*sun))
    shown = [obj] + list(extra)
    for o in scene.objects:
        if o.type in ('MESH', 'CURVE') and not o.name.startswith(('_Ground', '_Human')):
            o.hide_render = o not in shown
    lo, hi = homes._bounds(obj)
    if figure == 'right':
        human.location = (hi.x + 0.8, lo.y + 0.9, 0.0)
    elif figure == 'left':
        human.location = (lo.x - 0.8, lo.y + 0.9, 0.0)
    else:
        human.location = figure
    human.hide_render = False
    eye, target = homes.frame_shot(shown + [human], view, size, lens, margin=margin, height_bias=height_bias)
    return homes.shoot(scene, os.path.join(PREVIEW_DIR, out_name + '.png'), eye, target, lens, size, exposure)


def inside(obj, out_name, eye, target, lens=24.0, z_cut=None, keep=None, lamps=True, lamp_energy=60.0, sun=homes.SUN,
           exposure=0.45, size=homes.SHOT, human_at=None, sun_strength=None, extra=(), ground=True):
    """A view of the rooms: from eye toward target, of a copy of the model cut away above z_cut (or keeping only what
    keep(center) keeps), with point lights at its lamp sockets (the game's lamps)."""
    scene, sun_obj, human = stage()
    homes.light(scene, sun_obj, homes.golden_sun(*sun))
    show_ground(ground)
    if sun_strength is not None:
        sun_obj.data.energy = sun_strength
    cut = cutaway(obj, z_cut=z_cut, keep=keep) if (z_cut is not None or keep is not None) else None
    shown = [cut or obj] + list(extra)
    for o in scene.objects:
        if o.type in ('MESH', 'CURVE') and not o.name.startswith(('_Ground',)):
            o.hide_render = o not in shown and not (human_at is not None and o is human)
    if human_at is not None:
        human.location = human_at
        human.hide_render = False
    else:
        human.hide_render = True
    lights = lamp_lights(obj, energy=lamp_energy) if lamps else []
    try:
        return homes.shoot(scene, os.path.join(PREVIEW_DIR, out_name + '.png'), eye, target, lens, size, exposure)
    finally:
        remove(lights + ([cut] if cut else []))
        sun_obj.data.energy = homes.SUN_STRENGTH
        show_ground(True)


def show_ground(on):
    ground = bpy.data.objects.get('_Ground')
    if ground is not None:
        ground.hide_render = not on


def cull_backfaces(on):
    """Backface culling on every material (the renders only): rooms built of one-sided faces then read from above or
    outside as a dollhouse, their walls and vaults seen only from inside."""
    for mat in bpy.data.materials:
        if not mat.name.startswith(('_Sheet', '_Preview', '_Clay')):
            mat.use_backface_culling = on


def plan_view(obj, out_name, z_cut, direction=(-0.35, -0.55, 1.0), lens=35.0, margin=1.05, lamps=True,
              lamp_energy=60.0, exposure=0.0, keep=None, size=homes.SHOT, sun=(30.0, 62.0), ground=True):
    """A roof-off view: the model cut away above z_cut, seen from above at a slant, framed on what's left (ground=False
    leaves the street's dirt out, for what's under it)."""
    scene, sun_obj, human = stage()
    homes.light(scene, sun_obj, homes.golden_sun(*sun))
    show_ground(ground)
    cull_backfaces(True)
    cut = cutaway(obj, z_cut=z_cut, keep=keep)
    for o in scene.objects:
        if o.type in ('MESH', 'CURVE') and not o.name.startswith('_Ground'):
            o.hide_render = o is not cut
    human.hide_render = True
    lights = lamp_lights(obj, energy=lamp_energy) if lamps else []
    bpy.context.view_layer.update()
    try:
        eye, target = homes.frame_shot([cut], direction, size, lens, margin=margin, height_bias=0.3)
        return homes.shoot(scene, os.path.join(PREVIEW_DIR, out_name + '.png'), eye, target, lens, size, exposure)
    finally:
        remove(lights + [cut])
        show_ground(True)
        cull_backfaces(False)


def write_report(script, buildings, shots):
    """The report town_sheet.py reads: each building's numbers, sockets and rooms, and the renders made of it."""
    data = {}
    for b in buildings:
        info = dict(b.report)
        info['captions'] = b.captions
        info['shots'] = shots.get(b.name, [])
        data[b.name] = info
    path = ensure_dir(os.path.join(PREVIEW_DIR, f'{script}_report.json'))
    with open(path, 'w', encoding='utf-8') as file:
        json.dump(data, file, indent=1, default=lambda o: list(o) if isinstance(o, (Vector, tuple)) else str(o))
    lt._log(f'town: report {path}')
    return path
