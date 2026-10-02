"""Helpers for the scripted buildings (Art/Models/Buildings/<Name>.py; Art/README.md, Docs/TutorialIsland.md).

A building is one mesh put together from parts: boards, beams, logs, wall panels with openings, roof sheets and
shingles, stone. Each part is built in its own frame (its length along local X, its face toward local -Y, or lying
flat facing +Z), mapped there with looter_textures (lt.trim_uv onto the house trim sheet, lt.box_uv for tileables,
lt.cap_uv for sawn ends), so wood grain runs along every board however it's placed, and then put in place. A Space
places a group of parts: a wall (x along it, z up, the outside toward -y) or a roof slope (x along the eave, y up the
slope, z out of the roof, which sags in the middle). finish() makes the model object, its UCX_ hulls and SOCKET_
empties, bakes ambient occlusion into the vertex color alpha and renders the preview when the script runs with
--preview (--no-ao skips the bake, --stats prints where the triangles go). models.ps1 puts Tools/Blender on sys.path,
so a building script does:

    import looter_buildings as kit
    from looter_buildings import Opening, Trim

    m = kit.Model('Shed', seed=3)
    space = kit.wall_space((-2.0, -1.5), (2.0, -1.5))               # the front wall, facing -Y
    door = Opening('door', 1.5, 0.0, 1.0, 2.0)
    m.panel(kit.wall_outline(4.0, 2.4, [door]), kit.holes([door]), 0.12, Trim('A', world=True, rotate=True),
            space=space, around=[door])
    kit.openings(m, space, [door], 0.12)
    front, back = kit.gable(m, -2.0, 2.0, -1.5, 1.5, 2.4, 35.0, sag=0.05)
    kit.tin(m, front)
    kit.tin(m, back)
    m.hull((4.0, 3.0, 2.4), at=(0.0, 0.0, 1.2))
    m.finish()

Distances are meters; a model's pivot is the middle of its footprint at ground level and its front faces -Y.
Everything random comes from the model's seeded random.Random, so a building comes out the same on every run.
"""
import math
import os
import random
import sys

import bmesh
import bpy
from mathutils import Euler, Matrix, Vector
from mathutils.geometry import tessellate_polygon

import looter_model as lm
import looter_textures as lt

# Where the building scripts live (overview() runs them all).
SOURCE_DIR = os.path.join(os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))), 'Art', 'Models',
                          'Buildings')
UP = Vector((0.0, 0.0, 1.0))
U_REPEAT = lt.TRIM_SIZE / lt.TRIM_DENSITY  # the trim sheet repeats along U every 6.4 m
# Strips made of parallel pieces: the width of one piece (a board, log, beam face or shingle row), in meters.
LANES = {'A': 0.2, 'B': 0.4, 'C': 0.2, 'E': 0.2}
# Material keys: most of a building is the trim sheet; the rest are tileable sets or the lantern glow.
MATERIALS = {'trim': 'HouseTrim', 'metal': 'MetalRust', 'planks': 'WoodPlanks', 'stone': 'StoneWall',
             'endgrain': 'WoodEndGrain'}


def _args():
    """The script's own arguments (after '--'): --preview renders it, --no-ao skips the ambient occlusion bake."""
    return sys.argv[sys.argv.index('--') + 1:] if '--' in sys.argv else []


def strip_height(strip):
    """The world height one strip of the trim sheet covers (0.8 m, or 0.2 m for the H sub-strips)."""
    v0, v1 = lt.TRIM_STRIPS[lt.TRIM_ALIASES.get(strip, strip)]
    return (v1 - v0) * lt.TRIM_SIZE / lt.TRIM_DENSITY


def _material(key):
    if key == 'glow':
        # The same material as Art/Models/Props/LanternPost.py's lantern, so they share one instance in Unreal.
        return lm.material('LanternGlow', 0xffc56e, Glow=5.0, Variation=0.04)
    return lt.material(MATERIALS[key])


# --- Frames ---

def basis(origin, x, y, z):
    """The matrix whose columns are the axes x, y, z and the origin."""
    return Matrix(((x[0], y[0], z[0], origin[0]), (x[1], y[1], z[1], origin[1]), (x[2], y[2], z[2], origin[2]),
                   (0.0, 0.0, 0.0, 1.0)))


def place(at=(0.0, 0.0, 0.0), rot=(0.0, 0.0, 0.0)):
    """A placement: a position and an XYZ rotation in degrees."""
    return Matrix.LocRotScale(Vector(at), Euler([math.radians(a) for a in rot]), None)


def toward(p0, p1, face=(0.0, -1.0, 0.0)):
    """A frame at p0 whose X runs to p1 and whose -Y faces `face` (as nearly as it can): a part built along X with its
    front toward -Y lies from p0 to p1, front toward face."""
    p0, p1 = Vector(p0), Vector(p1)
    x = (p1 - p0).normalized()
    f = Vector(face)
    f = f - x * f.dot(x)
    if f.length < 1e-6:
        f = UP - x * UP.dot(x) if abs(x.z) < 0.9 else Vector((0.0, -1.0, 0.0))
    y = -f.normalized()
    return basis(p0, x, y, x.cross(y))


class Space:
    """Coordinates that a group of parts is placed in: matrix takes them into the model, after bend (a function of a
    point in these coordinates, giving an offset: a roof's sag)."""

    def __init__(self, matrix=None, bend=None):
        self.matrix = matrix if matrix is not None else Matrix.Identity(4)
        self.bend = bend

    def world(self, co):
        co = Vector(co)
        if self.bend is not None:
            co = co + self.bend(co)
        return self.matrix @ co


WORLD = Space()


def wall_space(p0, p1, z0=0.0):
    """A wall from p0 to p1 (x, y on the ground; the outside is on the right when walking from p0 to p1, so walls
    listed counterclockwise from above face out): x along it, z up from z0, the outer face at y = 0 and the inside
    toward +y."""
    x = Vector((p1[0] - p0[0], p1[1] - p0[1], 0.0)).normalized()
    return Space(basis((p0[0], p0[1], z0), x, UP.cross(x), UP))


class Slope(Space):
    """One plane of a roof: x along the eave (0 to width), y up the slope (0 to length), z out of the roof, with z = 0
    the top of the deck where the covering lies. The roof sags: the middle of the ridge drops sag meters (straight
    down, so two slopes still meet at the ridge), the eave about a third of that, the gable ends not at all."""

    def __init__(self, origin, x_dir, up_dir, width, length, sag=0.0, frame=None):
        x = Vector(x_dir).normalized()
        y = Vector(up_dir).normalized()
        matrix = basis(origin, x, y, x.cross(y))
        super().__init__(frame @ matrix if frame is not None else matrix, self._sag)
        self.width, self.length, self.sag = width, length, sag
        self.down = self.matrix.to_3x3().inverted() @ Vector((0.0, 0.0, -1.0))

    def drop(self, x, y):
        across = math.sin(math.pi * min(max(x / self.width, 0.0), 1.0))
        return self.sag * across * (0.3 + 0.7 * min(max(y / self.length, 0.0), 1.0))

    def _sag(self, co):
        return self.down * self.drop(co.x, co.y)


# --- UV mapping specs (applied to a part in its own frame) ---

class Trim:
    """A part on the house trim sheet (lt.trim_uv): strip is 'A'...'G', 'H1'...'H4' or an alias ('Siding', 'Iron'...).
    world=True keeps world scale over big faces (walls, roofs, decks) and cuts them into bands; otherwise each face
    starts at the strip's bottom plus v meters, on one lane (a board, log, beam face or shingle row: None picks one at
    random, 'each' gives each side of a beam its own). rotate turns the grain a quarter (vertical boards on a wall).
    fit squeezes a tall face into the strip (glass). u slides it along U (random by default)."""

    def __init__(self, strip, lane=None, world=False, rotate=False, v=0.0, u=None, fit=False, width=None):
        self.strip = lt.TRIM_ALIASES.get(strip, strip)
        self.lane, self.world, self.rotate, self.v, self.u, self.fit, self.width = lane, world, rotate, v, u, fit, width

    def apply(self, obj, rng):
        u = self.u if self.u is not None else rng.uniform(0.0, U_REPEAT)
        if self.world:
            lt.trim_uv(obj, None, self.strip, u_offset=u, rotate=self.rotate, v_offset=self.v, align='world', cut=True)
            return
        height = strip_height(self.strip)
        squeeze = None
        if self.fit:
            # Faces are fitted to the strip by squeezing the part across its grain (Y and Z) while it's mapped.
            spans = [max(v.co[i] for v in obj.data.vertices) - min(v.co[i] for v in obj.data.vertices) for i in (1, 2)]
            squeeze = Matrix.Diagonal((1.0, min(1.0, height * 0.999 / max(spans[0], 1e-6)),
                                       min(1.0, height * 0.999 / max(spans[1], 1e-6)), 1.0))
            obj.data.transform(squeeze)
        lane_width = LANES.get(self.strip)
        if lane_width and self.lane == 'each':
            groups = {}
            for polygon in obj.data.polygons:
                key = tuple(round(c, 1) for c in polygon.normal)
                groups.setdefault(key, []).append(polygon.index)
            count = int(round(height / lane_width))
            if self.width is not None:
                count = max(1, min(count, int((height - self.width) / lane_width + 1e-6) + 1))
            lanes = list(range(count))
            rng.shuffle(lanes)
            for index, faces in enumerate(groups.values()):
                lt.trim_uv(obj, faces, self.strip, u_offset=u, rotate=self.rotate,
                           v_offset=self.v + lanes[index % len(lanes)] * lane_width)
        else:
            v = self.v
            if lane_width:
                count = int(round(height / lane_width))
                if self.width is not None:  # a wide piece starts low enough to fit
                    count = max(1, min(count, int((height - self.width) / lane_width + 1e-6) + 1))
                lane = self.lane if self.lane is not None else rng.randrange(count)
                v += lane * lane_width
            lt.trim_uv(obj, None, self.strip, u_offset=u, rotate=self.rotate, v_offset=v)
        if squeeze is not None:
            obj.data.transform(squeeze.inverted())


class Tile:
    """A part on a tileable set: world-scale cube projection (lt.box_uv), the grain along each face's longer side."""

    def __init__(self, set_name, along='long'):
        self.set_name, self.along = set_name, along

    def apply(self, obj, rng):
        lt.box_uv(obj, self.set_name, along=self.along, seed=rng.randrange(1 << 30))


class EndGrain:
    """Saw-cut ends (logs, posts) on WoodEndGrain (lt.cap_uv): rings in the middle, bark at the rim, each end turned."""

    def apply(self, obj, rng):
        lt.cap_uv(obj, None, seed=rng.randrange(1 << 30))


def _spec(uv, width=None, lane=None):
    if isinstance(uv, str):
        return Trim(uv, lane=lane, width=width)
    if isinstance(uv, Trim) and uv.width is None and width is not None:
        uv.width = width  # the part's widest side: lanes it can't fit are skipped
    return uv


# --- Geometry (local frames) ---

def _new_bmesh():
    tb = bmesh.new()
    tb.loops.layers.uv.new('UVMap')
    return tb


def _slice(tb, axis, positions):
    normal = Vector((1.0, 0.0, 0.0)) if axis == 0 else Vector((0.0, 1.0, 0.0)) if axis == 1 else UP
    for p in positions:
        bmesh.ops.bisect_plane(tb, geom=tb.verts[:] + tb.edges[:] + tb.faces[:], dist=1e-5, plane_co=normal * p,
                               plane_no=normal)


SIDES = {'-x': (-1, 0, 0), '+x': (1, 0, 0), '-y': (0, -1, 0), '+y': (0, 1, 0), '-z': (0, 0, -1), '+z': (0, 0, 1)}


def _box(size, bevel=0.0, cuts=0, drop=()):
    """A box centered on the origin; cuts slices it across X (so a long part can bend); drop names sides nobody sees
    ('-z' under a roof sheet, '+y' under the next row) to leave out."""
    tb = _new_bmesh()
    bmesh.ops.create_cube(tb, size=1.0, matrix=Matrix.Diagonal((size[0], size[1], size[2], 1.0)))
    if bevel > 0.0:
        bmesh.ops.bevel(tb, geom=tb.edges[:], offset=bevel, segments=1, affect='EDGES', clamp_overlap=True)
    if drop:
        tb.normal_update()
        gone = [f for f in tb.faces if any(f.normal.dot(Vector(SIDES[d])) > 0.99 for d in drop)]
        bmesh.ops.delete(tb, geom=gone, context='FACES_ONLY')
    if cuts:
        _slice(tb, 0, [-size[0] * 0.5 + size[0] * i / (cuts + 1) for i in range(1, cuts + 1)])
    return tb


def _prism(loops, depth):
    """A flat piece: loops of (x, z) points (the outline counterclockwise seen from the front, then any holes); the
    front at y = 0 faces -Y and the piece is depth thick toward +Y."""
    tb = _new_bmesh()
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
    # Corners in a line (two windows' tops level across a wall) can come out of the fill as flat triangles, which have
    # no tangent frame: edges between the fill's triangles are turned until every triangle has some shape.
    triangles = [f for f in tb.faces if len(f.verts) == 3]
    inner = [e for e in {e for f in triangles for e in f.edges}
             if len(e.link_faces) == 2 and all(len(f.verts) == 3 for f in e.link_faces)]
    bmesh.ops.beautify_fill(tb, faces=triangles, edges=inner, method='ANGLE')
    bmesh.ops.join_triangles(tb, faces=[f for f in tb.faces if len(f.verts) == 3], angle_face_threshold=0.02,
                             angle_shape_threshold=math.pi)
    return tb


def _sheet(width, height, cols=1, rows=1):
    """A flat grid from (0, 0) to (width, height) in the XZ plane, facing -Y (the unrolled side of a cylinder)."""
    tb = _new_bmesh()
    grid = [[tb.verts.new((width * i / cols, 0.0, height * j / rows)) for j in range(rows + 1)] for i in range(cols + 1)]
    for i in range(cols):
        for j in range(rows):
            tb.faces.new((grid[i][j], grid[i + 1][j], grid[i + 1][j + 1], grid[i][j + 1]))
    return tb


def _disc(radius, sides, phase):
    """A flat n-gon in the XZ plane facing -Y, its corners at angles phase + k * 360 / sides from +X toward +Z."""
    tb = _new_bmesh()
    verts = [tb.verts.new((radius * math.cos(phase + 2.0 * math.pi * k / sides), 0.0,
                           radius * math.sin(phase + 2.0 * math.pi * k / sides))) for k in range(sides)]
    face = tb.faces.new(verts)
    face.normal_update()
    if face.normal.y > 0.0:
        face.normal_flip()
    return tb


def frange(start, stop, step):
    """start, start + step, ... while short of stop (either direction)."""
    values = []
    x = start
    while (x < stop - 1e-9) if step > 0 else (x > stop + 1e-9):
        values.append(x)
        x += step
    return values


def rect(x, z, w, h):
    return [(x, z), (x + w, z), (x + w, z + h), (x, z + h)]


class Opening:
    """A window or door in a wall: its lower left corner (x, z) in the wall's space, width and height; opts go to the
    assembly (window() or door())."""

    def __init__(self, kind, x, z, w, h, **opts):
        self.kind, self.x, self.z, self.w, self.h, self.opts = kind, x, z, w, h, opts


def wall_outline(length, height, openings=(), apex=None):
    """A wall's outline: a rectangle, notched for the doors standing on its bottom, with a gable point apex (x, z)
    or a list of them from right to left (a gambrel's gable)."""
    points = [(0.0, 0.0)]
    for o in sorted((o for o in openings if o.z <= 1e-4), key=lambda o: o.x):
        points += [(o.x, 0.0), (o.x, o.h), (o.x + o.w, o.h), (o.x + o.w, 0.0)]
    points += [(length, 0.0), (length, height)]
    if apex is not None:
        points += list(apex) if isinstance(apex[0], (tuple, list)) else [apex]
    points.append((0.0, height))
    return points


# --- The model ---

class Model:
    """One building mesh being put together (see the module's docstring). seed makes its weathering repeatable."""

    def __init__(self, name, seed=1):
        self.name = name
        self.rng = random.Random(seed)
        self.bm = bmesh.new()
        self.uv = self.bm.loops.layers.uv.new('UVMap')
        self.floor = self.bm.faces.layers.float.new('_aofloor')
        self.slots = {}
        self.hulls = []
        self.sockets = []
        self._mesh = bpy.data.meshes.new('_part')
        self._obj = bpy.data.objects.new('_part', self._mesh)

    def triangles(self):
        return sum(len(f.verts) - 2 for f in self.bm.faces)

    def section(self, label):
        """With --stats, prints the triangles added since the last section (to see where a budget goes)."""
        if '--stats' in _args():
            total = self.triangles()
            print(f'BUILDING: {self.name} {label}: {total - getattr(self, "_counted", 0)}', flush=True)
            self._counted = total

    def slot(self, key):
        if key not in self.slots:
            self.slots[key] = (len(self.slots), _material(key))
        return self.slots[key][0]

    def emit(self, tb, uv=None, mat='trim', place=None, space=None, shape=None, smooth=False, ao_floor=0.0):
        """Adds a part built in tb, in its own frame: maps it there (uv: a Trim, a Tile or None), reshapes it with shape
        (a function of a point in that frame), then place puts it in space and space in the model. ao_floor is the
        least ambient occlusion its faces keep after the bake (see finish())."""
        tb.normal_update()
        tb.to_mesh(self._mesh)
        tb.free()
        if uv is not None:
            uv.apply(self._obj, self.rng)
        src = bmesh.new()
        src.from_mesh(self._mesh)
        src_uv = src.loops.layers.uv.active
        place = place if place is not None else Matrix.Identity(4)
        space = space if space is not None else WORLD
        slot = self.slot(mat)
        verts = {}
        for v in src.verts:
            co = shape(v.co.copy()) if shape is not None else v.co.copy()
            verts[v] = self.bm.verts.new(space.world(place @ co))
        flip = (space.matrix.to_3x3() @ place.to_3x3()).determinant() < 0.0
        for face in src.faces:
            loops = list(face.loops)
            if flip:
                loops.reverse()
            try:
                new = self.bm.faces.new([verts[loop.vert] for loop in loops])
            except ValueError:
                continue
            new.material_index = slot
            new.smooth = smooth
            new[self.floor] = ao_floor
            uvs = {verts[loop.vert]: loop[src_uv].uv.copy() for loop in loops} if src_uv is not None else {}
            for loop in new.loops:
                loop[self.uv].uv = uvs.get(loop.vert, (0.0, 0.0))
        src.free()

    # Parts.

    def box(self, size, at=(0.0, 0.0, 0.0), rot=(0.0, 0.0, 0.0), uv='C', mat='trim', space=None, bevel=0.0, cuts=0,
            matrix=None, ao_floor=0.0, drop=()):
        """A box of size (x, y, z) at a position and rotation (degrees), or placed by matrix; its grain runs along X.
        drop leaves out sides nobody sees (see _box)."""
        self.emit(_box(size, bevel, cuts, drop), _spec(uv, width=max(size[1], size[2])), mat,
                  matrix if matrix is not None else place(at, rot), space, ao_floor=ao_floor)

    def board(self, p0, p1, width, thick, face=(0.0, -1.0, 0.0), uv='A', mat='trim', space=None, lane=None, lift=0.0,
              bevel=0.0, cuts=None):
        """A board or beam along the line p0 to p1 (its middle), width across and thick deep, its broad side toward
        face; lift moves it that way. A long one is cut every 2.5 m or so (cuts), for its baked shading."""
        length = (Vector(p1) - Vector(p0)).length
        if cuts is None:
            cuts = int(length / 2.5)
        matrix = toward(p0, p1, face) @ Matrix.Translation((length * 0.5, -lift, 0.0))
        self.emit(_box((length, thick, width), bevel, cuts), _spec(uv, width=max(width, thick), lane=lane), mat, matrix,
                  space)

    def cylinder(self, p0, p1, r0, r1=None, sides=8, uv='C', mat='trim', space=None, caps=(True, True),
                 face=(0.0, -1.0, 0.0), phase=-90.0, cap_uv=None, cap_mat=None):
        """A round part from p0 to p1 (posts, logs, pipes; a cone with r1). Its side is mapped unrolled: around it
        runs one strip's height (for a Trim; a log's front half is then one of strip B's logs, from its bottom at
        phase -90 degrees up to its top), or the circumference at world scale (for a Tile). Angles start from face."""
        r1 = r0 if r1 is None else r1
        length = (Vector(p1) - Vector(p0)).length
        spec = _spec(uv)
        around = strip_height(spec.strip) if isinstance(spec, Trim) else math.pi * (r0 + r1)
        if isinstance(spec, Trim):
            spec = Trim(spec.strip, world=True, rotate=False, v=spec.v, u=spec.u)
        start = math.radians(phase)

        def roll(co):
            r = r0 + (r1 - r0) * co.x / max(length, 1e-6)
            a = start + 2.0 * math.pi * co.z / around
            return Vector((co.x, -r * math.cos(a), r * math.sin(a)))

        frame = toward(p0, p1, face)
        self.emit(_sheet(length, around, 1, sides), spec, mat, frame, space, shape=roll, smooth=True)
        cap_spec = _spec(cap_uv) if cap_uv is not None else (
            Trim(spec.strip, lane=0) if isinstance(spec, Trim) else Tile(spec.set_name))
        for end, radius, turn in ((0, r0, -90.0), (1, r1, 90.0)):
            if caps[end] and radius > 1e-4:
                # The disc's corners meet the side's: angle a sits at (r cos a, r sin a) on the disc, turned to the end.
                disc_phase = start if end == 0 else math.pi - start
                cap = _disc(radius, sides, disc_phase)
                matrix = frame @ Matrix.Translation((length * end, 0.0, 0.0)) @ place(rot=(0.0, 0.0, turn))
                self.emit(cap, cap_spec, cap_mat or mat, matrix, space)

    def panel(self, outline, holes=(), thick=0.12, uv=None, mat='trim', space=None, matrix=None, slices=1.2,
              around=()):
        """A flat piece (a wall): outline (x, z) counterclockwise seen from the front, holes inside it; the front faces
        -Y in space and it's thick toward +Y. The baked shading lives on vertices, so unless its grain is turned upright
        (whose bands already cut it) it is sliced every slices meters along X, and it is sliced just outside the
        openings listed in around, so the dark corners of a window stay under its casing."""
        uv = uv if uv is not None else Trim('A', world=True)
        tb = _prism([list(outline)] + [list(h) for h in holes], thick)
        xs = [x for x, z in outline]
        zs = [z for x, z in outline]
        upright = isinstance(uv, Trim) and uv.rotate
        cut_x, cut_z = [], set()
        if slices and not upright:
            cut_x += frange(min(xs) + slices, max(xs) - 0.1, slices)
        for o in around:
            if o.z > 0.15:  # a cut under the sill (windows at one height share it)
                cut_z.add(round(o.z - 0.1, 2))
            if not upright:
                cut_x += [o.x - 0.2, o.x + o.w + 0.2]
        _slice(tb, 0, [x for x in cut_x if min(xs) + 0.05 < x < max(xs) - 0.05])
        _slice(tb, 2, [z for z in sorted(cut_z) if min(zs) + 0.05 < z < max(zs) - 0.05])
        self.emit(tb, uv, mat, matrix, space)

    # Collision and sockets.

    def hull(self, size, at=(0.0, 0.0, 0.0), rot=(0.0, 0.0, 0.0), space=None, matrix=None):
        """A box collision hull (convex), placed like a box part."""
        matrix = matrix if matrix is not None else place(at, rot)
        space = space if space is not None else WORLD
        corners = [Vector((sx * size[0], sy * size[1], sz * size[2])) * 0.5
                   for sx in (-1, 1) for sy in (-1, 1) for sz in (-1, 1)]
        self.hulls.append([space.world(matrix @ c) for c in corners])

    def hull_points(self, points, space=None):
        """A convex collision hull around these points."""
        space = space if space is not None else WORLD
        self.hulls.append([space.world(p) for p in points])

    def socket(self, name, location, rotation=(0.0, 0.0, 0.0)):
        self.sockets.append((name, tuple(location), tuple(rotation)))

    # The end.

    def finish(self, ao=True, ground=True, preview=True, view=(-1.0, -1.6, 0.55), fit=0.8, fallback=100.0, **props):
        """Makes the model object (with its hulls and sockets), bakes ambient occlusion into the vertex color alpha and
        renders the preview when asked for (--preview). fallback is the share of triangles Nanite's fallback keeps (what
        Medium and Low draw): all of them by default, since the budgets count them; props become the model's custom
        properties (Nanite=0, LODs='40,12', Collision='None'...). Returns the object."""
        bpy.data.objects.remove(self._obj)
        bpy.data.meshes.remove(self._mesh)
        mesh = bpy.data.meshes.new(self.name)
        _triangulate(self.bm)
        self.bm.normal_update()
        self.bm.to_mesh(mesh)
        self.bm.free()
        for key, (index, material) in sorted(self.slots.items(), key=lambda item: item[1][0]):
            mesh.materials.append(material)
        obj = bpy.data.objects.new(self.name, mesh)
        bpy.context.scene.collection.objects.link(obj)
        if fallback is not None and props.get('Nanite', 1):
            obj['Fallback'] = float(fallback)
        for key, value in props.items():
            obj[key] = value
        triangles = sum(len(p.vertices) - 2 for p in mesh.polygons)
        print(f"BUILDING: {self.name}: {triangles} triangles, {len(self.hulls)} hulls, "
              f"materials {', '.join(m.name for m in mesh.materials)}", flush=True)
        if '--stats' in _args():
            degenerate_report(obj)
        if ao and '--no-ao' not in _args():
            lt.bake_vertex_ao(obj, ground=ground)
            _raise_ao(mesh)
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
            lt.preview([obj], lt.preview_path('Buildings', self.name), view=view, fit=fit)
        return obj


def _triangulate(bm):
    """Cuts the model into triangles here, the same way on every export. Band cuts leave faces with extra corners on
    straight sides (where a neighbor was cut), and an importer cutting those itself can fan zero-area triangles across
    them, which have no tangent frame (Unreal's 'degenerate tangent bases'); Blender's beauty fill doesn't. Slivers a
    cut left a hair from a corner go too: sub-millimeter edges collapse; a triangle with its corners in a line has its
    long side turned toward its neighbor (both then lie in the neighbor's plane) where the two share their UVs; and
    any flat triangle still left is dropped (it covers nothing)."""
    bmesh.ops.triangulate(bm, faces=bm.faces[:], quad_method='BEAUTY', ngon_method='BEAUTY')
    uv = bm.loops.layers.uv.active

    def flat(face):
        return face.is_valid and face.calc_area() < 1e-6  # under a square millimeter

    def same_uvs(edge):
        a, b = edge.link_faces
        if a.material_index != b.material_index:
            return False
        corners = [{loop.vert: loop[uv].uv.copy() for loop in f.loops} for f in (a, b)]
        return all((corners[0][v] - corners[1][v]).length < 1e-6 for v in edge.verts)

    for _ in range(4):
        slivers = [f for f in bm.faces if flat(f)]
        if not slivers:
            return
        # Lists in mesh order, never sets of mesh elements: a set of them iterates in memory-address order, which
        # changes from run to run, and these operations give a different mesh for a different order, so the same
        # source exported a different mesh each time (lettered signs changed by hundreds of KB).
        short = []
        for f in slivers:
            short.extend(e for e in f.edges if e.calc_length() < 1e-3 and e not in short)
        if short:
            bmesh.ops.dissolve_degenerate(bm, dist=1e-3, edges=short)
            bmesh.ops.triangulate(bm, faces=[f for f in bm.faces if len(f.verts) > 3], quad_method='BEAUTY',
                                  ngon_method='BEAUTY')
        turn = []
        for face in (f for f in bm.faces if flat(f)):
            longest = max(face.edges, key=lambda e: e.calc_length())
            if len(longest.link_faces) == 2 and not any(e in turn for e in face.edges) and same_uvs(longest):
                turn.append(longest)
        if not turn:
            break
        bmesh.ops.rotate_edges(bm, edges=turn, use_ccw=False)
    bmesh.ops.delete(bm, geom=[f for f in bm.faces if flat(f)], context='FACES_ONLY')


def degenerate_report(obj, limit=8):
    """Prints the triangles Unreal can't build a tangent frame for (its 'degenerate tangent bases' warning): those whose
    UVs have next to no area, or which have next to no area themselves. Returns how many there are."""
    bm = bmesh.new()
    bm.from_mesh(obj.data)
    bmesh.ops.triangulate(bm, faces=bm.faces[:])
    uv = bm.loops.layers.uv.active
    bad = []
    for face in bm.faces:
        a, b, c = (loop[uv].uv for loop in face.loops)
        uv_area = abs((b - a).cross(c - a)) * 0.5
        area = face.calc_area()
        # A texel is 1/2048 of the sheet: under a hundredth of a texel squared, the UVs have no direction left.
        if area < 1e-8 or uv_area < (0.01 / 2048.0) ** 2:
            name = obj.data.materials[face.material_index].name if obj.data.materials else '?'
            bad.append((name, tuple(round(v, 2) for v in face.calc_center_median()), area, uv_area))
    bm.free()
    print(f'BUILDING: {obj.name}: {len(bad)} triangles with no UV area or no area', flush=True)
    for name, center, area, uv_area in bad[:limit]:
        print(f'BUILDING:   {name} at {center}: area {area:.2e} m2, UV area {uv_area:.2e}', flush=True)
    return len(bad)


def _raise_ao(mesh):
    """Lifts the baked occlusion of faces with an ao_floor (their '_aofloor' attribute) to at least that."""
    import numpy as np
    count = len(mesh.polygons)
    floors = np.zeros(count, dtype=np.float32)
    mesh.attributes['_aofloor'].data.foreach_get('value', floors)
    if not floors.any():
        return
    totals = np.zeros(count, dtype=np.int64)
    mesh.polygons.foreach_get('loop_total', totals)
    col = mesh.color_attributes['Col']
    raw = np.empty(len(mesh.loops) * 4, dtype=np.float32)
    col.data.foreach_get('color_srgb', raw)
    raw = raw.reshape(-1, 4)
    raw[:, 3] = np.maximum(raw[:, 3], np.repeat(floors, totals))
    col.data.foreach_set('color_srgb', raw.ravel())
    mesh.update()


# --- Assemblies (in a wall's space: x along the wall, z up, the outer face at y = 0, outside toward -y) ---

def trim_board(m, space, p0, p1, width, paint, thick=0.035, lift=0.0, lane=None):
    """A casing or trim board on the wall face: painted (H1 teal, H2 oxide red) or weathered wood (A)."""
    m.board(p0, p1, width, thick, uv=paint, space=space, lift=lift + thick * 0.5, lane=lane)


def window(m, space, o, depth):
    """A window in opening o: casing, sill, sash and dark glass; opts: style ('glass', 'boarded', 'shutters'), paint
    (the casing: 'A' weathered, 'H1' teal, 'H2' oxide red), sash paint, panes (columns, rows)."""
    rng = m.rng
    x0, z0, w, h = o.x, o.z, o.w, o.h
    style = o.opts.get('style', 'glass')
    paint = o.opts.get('paint', 'A')
    sash_paint = o.opts.get('sash', paint)
    cw = o.opts.get('casing', 0.12)
    # Casing: sides, a head with a drip cap, and a thick sill that sticks out (frame=False: the wall's own timbers
    # frame it, only the sill is added).
    if o.opts.get('frame', True):
        for x in (x0 - cw * 0.5, x0 + w + cw * 0.5):
            trim_board(m, space, (x, 0.0, z0 - 0.02), (x, 0.0, z0 + h + cw), cw, paint)
        trim_board(m, space, (x0 - cw - 0.02, 0.0, z0 + h + cw * 0.6), (x0 + w + cw + 0.02, 0.0, z0 + h + cw * 0.6),
                   cw * 1.2, paint)
        m.box((w + 2.0 * cw + 0.12, 0.09, 0.04), at=(x0 + w * 0.5, -0.045, z0 + h + cw * 1.2 + 0.02), uv='C',
              space=space)
    m.box((w + 2.0 * cw + 0.1, 0.1 + depth * 0.5, 0.06), at=(x0 + w * 0.5, -0.05 + depth * 0.25, z0 - 0.03),
          rot=(rng.uniform(-1.5, 1.5), 0.0, 0.0), uv='C', space=space)
    # Glass half way into the wall, a sash round it and muntins across.
    # Dark glass half way into the wall (its reveal frames it) and muntins across.
    glass_y = depth * 0.55
    m.box((w, 0.02, h), at=(x0 + w * 0.5, glass_y, z0 + h * 0.5), uv=Trim('H4', fit=True), space=space)
    cols, rows = o.opts.get('panes', (2, 2))
    for i in range(1, cols):
        x = x0 + w * i / cols
        m.board((x, glass_y - 0.03, z0), (x, glass_y - 0.03, z0 + h), 0.045, 0.04, uv=sash_paint, space=space)
    for j in range(1, rows):
        z = z0 + h * j / rows
        m.board((x0, glass_y - 0.035, z), (x0 + w, glass_y - 0.035, z), 0.045, 0.04, uv=sash_paint, space=space)
    if style == 'boarded':
        # Planks nailed across: an X and one level board, each a little askew.
        cx, cz = x0 + w * 0.5, z0 + h * 0.5
        reach = math.hypot(w + cw, h + cw) * 0.5 + 0.02
        angle = math.atan2(h, w)
        for k, sign in enumerate((1.0, -1.0)):
            a = sign * angle + math.radians(rng.uniform(-6.0, 6.0))
            d = Vector((math.cos(a), 0.0, math.sin(a))) * reach
            m.board((cx - d.x, 0.0, cz - d.z), (cx + d.x, 0.0, cz + d.z), 0.19, 0.035, space=space,
                    lift=0.055 + k * 0.035)
        a = math.radians(rng.uniform(-5.0, 5.0))
        half = (w + cw * 1.6) * 0.5
        zc = cz + rng.uniform(-0.25, 0.1) * h
        m.board((cx - half, 0.0, zc - half * math.sin(a)), (cx + half, 0.0, zc + half * math.sin(a)), 0.19, 0.035,
                space=space, lift=0.13)
    elif style == 'shutters':
        # Plank shutters folded back against the wall; one hangs askew from a lost hinge.
        crooked = rng.random() < 0.6
        for side, sign in ((0, -1.0), (1, 1.0)):
            sw = w * 0.5 + 0.02
            edge = x0 - cw if side == 0 else x0 + w + cw
            center = edge + sign * sw * 0.5
            tilt = rng.uniform(4.0, 9.0) * (1 if rng.random() < 0.5 else -1) if crooked and side == 1 else 0.0
            frame = place((center, -0.05, z0 + h * 0.5), (0.0, tilt, 0.0))
            shutter = Space(space.matrix @ frame)
            boards = 3
            for k in range(boards):
                x = -sw * 0.5 + sw * (k + 0.5) / boards
                m.board((x, 0.0, -h * 0.5 - 0.03), (x, 0.0, h * 0.5 + 0.03), sw / boards - 0.008, 0.035,
                        uv=paint if paint != 'A' else 'H1', space=shutter)
            m.board((-sw * 0.45, 0.0, -h * 0.28), (sw * 0.45, 0.0, h * 0.28), 0.09, 0.03,
                    uv=paint if paint != 'A' else 'H1', space=shutter, lift=0.03)


def door(m, space, o, depth):
    """A plank door in opening o, a little into the wall: boards, battens and a diagonal brace, long iron strap hinges,
    a latch, a casing and a threshold. opts: paint for the boards ('A' weathered, 'H1' teal, 'H2' oxide red), casing
    paint, hinge side ('left' or 'right')."""
    rng = m.rng
    x0, w, h = o.x, o.w, o.h
    paint = o.opts.get('paint', 'A')
    casing = o.opts.get('casing', 'A')
    hinge_left = o.opts.get('hinge', 'left') == 'left'
    y = depth * 0.3
    boards = max(3, int(round(w / 0.2)))
    bw = w / boards
    for k in range(boards):
        x = x0 + bw * (k + 0.5)
        m.board((x, y, 0.0), (x, y, h - rng.uniform(0.0, 0.03)), bw - 0.006, 0.05, uv=paint, space=space)
    # Battens and a brace on the outside face.
    for z in (0.3, h - 0.35):
        m.board((x0 + 0.06, y - 0.04, z), (x0 + w - 0.06, y - 0.04, z), 0.16, 0.035, uv=paint, space=space)
    low, high = (x0 + 0.1, x0 + w - 0.1) if hinge_left else (x0 + w - 0.1, x0 + 0.1)
    m.board((high, y - 0.04, 0.4), (low, y - 0.04, h - 0.45), 0.13, 0.03, uv=paint, space=space)
    # Iron strap hinges over the battens, longer than they need to be, with their pintles on the casing.
    hinge_x = x0 if hinge_left else x0 + w
    reach = w * 0.72
    for z in (0.3, h - 0.35):
        end = hinge_x + reach if hinge_left else hinge_x - reach
        m.board((hinge_x - (0.09 if hinge_left else -0.09), y - 0.065, z), (end, y - 0.065, z), 0.07, 0.014,
                uv=Trim('H3', fit=True), space=space)
        m.box((0.05, 0.05, 0.11), at=(hinge_x - (0.1 if hinge_left else -0.1), -0.03, z), uv=Trim('H3', fit=True),
              space=space)
    # A latch and a ring pull.
    latch_x = x0 + w - 0.14 if hinge_left else x0 + 0.14
    m.box((0.05, 0.04, 0.16), at=(latch_x, y - 0.07, 1.0), uv=Trim('H3', fit=True), space=space)
    m.box((0.12, 0.03, 0.03), at=(latch_x, y - 0.1, 1.05), uv=Trim('H3', fit=True), space=space)
    # Casing and threshold.
    cw = 0.13
    for x in (x0 - cw * 0.5, x0 + w + cw * 0.5):
        trim_board(m, space, (x, 0.0, -0.02), (x, 0.0, h + cw), cw, casing)
    trim_board(m, space, (x0 - cw - 0.03, 0.0, h + cw * 0.55), (x0 + w + cw + 0.03, 0.0, h + cw * 0.55), cw * 1.15,
               casing)
    m.box((w + 0.06, depth * 0.5 + 0.08, 0.05), at=(x0 + w * 0.5, depth * 0.25 - 0.04, -0.005), uv='C', space=space)


def openings(m, space, items, depth):
    """Builds the assemblies of a wall's openings (after its panel)."""
    for o in items:
        if o.kind == 'window':
            window(m, space, o, depth)
        elif o.kind == 'door':
            door(m, space, o, depth)


def holes(items):
    """The holes a wall panel needs for its windows (doors notch the outline instead)."""
    return [rect(o.x, o.z, o.w, o.h) for o in items if o.z > 1e-4]


# --- Roofs ---

def gable(m, x0, x1, y0, y1, eave_z, pitch, overhang=0.4, rake=0.3, deck=0.12, sag=0.0, deck_uv='A',
          rake_boards=True, ends=(True, True), frame=None):
    """A gable roof over walls from x0 to x1 and y0 (front) to y1 (back), its ridge along X: the deck of both
    slopes and the rake boards (at the x0 and x1 ends, as ends says). The deck rests on the walls' outer top edge at
    eave_z. frame (a matrix) places the whole roof, for one whose ridge runs another way (a dormer). Returns the front
    and back Slopes (z = 0 on them is the top of the deck); cover them with tin() or shingles()."""
    theta = math.radians(pitch)
    half = (y1 - y0) * 0.5
    run = half + overhang
    length = run / math.cos(theta) + deck * math.tan(theta)
    width = x1 - x0 + 2.0 * rake
    normal_f = Vector((0.0, -math.sin(theta), math.cos(theta)))
    eave_f = Vector((x0 - rake, y0 - overhang, eave_z - overhang * math.tan(theta))) + normal_f * deck
    front = Slope(eave_f, (1.0, 0.0, 0.0), (0.0, math.cos(theta), math.sin(theta)), width, length, sag, frame)
    normal_b = Vector((0.0, math.sin(theta), math.cos(theta)))
    eave_b = Vector((x1 + rake, y1 + overhang, eave_z - overhang * math.tan(theta))) + normal_b * deck
    back = Slope(eave_b, (-1.0, 0.0, 0.0), (0.0, -math.cos(theta), math.sin(theta)), width, length, sag, frame)
    roof_deck(m, front, deck, deck_uv, rake_boards, (ends[0], ends[1]))
    roof_deck(m, back, deck, deck_uv, rake_boards, (ends[1], ends[0]))
    return front, back


def gambrel(m, x0, x1, y0, y1, eave_z, lower=60.0, upper=25.0, knuckle=1.2, overhang=0.4, rake=0.3, deck=0.12,
            sag=0.0, frame=None):
    """A gambrel (barn) roof over walls from x0 to x1 and y0 to y1, its ridge along X: on each side a steep lower slope
    (lower degrees, rising over knuckle meters of run) and a shallow upper one (upper degrees) to the ridge. Returns
    (front lower, front upper, back lower, back upper) Slopes and the knuckle and ridge heights."""
    tl, tu = math.radians(lower), math.radians(upper)
    half = (y1 - y0) * 0.5
    knuckle_z = eave_z + knuckle * math.tan(tl)
    ridge_z = knuckle_z + (half - knuckle) * math.tan(tu)
    width = x1 - x0 + 2.0 * rake
    slopes = []
    for side in (1.0, -1.0):  # front (from y0), then back (from y1)
        wall_y = y0 if side > 0 else y1
        x_dir = (side, 0.0, 0.0)
        x_start = x0 - rake if side > 0 else x1 + rake
        n_l = Vector((0.0, -side * math.sin(tl), math.cos(tl)))
        n_u = Vector((0.0, -side * math.sin(tu), math.cos(tu)))
        # The lower slope: from the eave's overhang up to just past the knuckle.
        origin = Vector((x_start, wall_y - side * overhang, eave_z - overhang * math.tan(tl))) + n_l * deck
        length = (overhang + knuckle) / math.cos(tl) + deck * 0.5
        low = Slope(origin, x_dir, (0.0, side * math.cos(tl), math.sin(tl)), width, length, sag * 0.5, frame)
        # The upper slope: from the knuckle to the ridge.
        origin = Vector((x_start, wall_y + side * knuckle, knuckle_z)) + n_u * deck
        length = (half - knuckle) / math.cos(tu) + deck * math.tan(tu)
        high = Slope(origin, x_dir, (0.0, side * math.cos(tu), math.sin(tu)), width, length, sag, frame)
        for slope in (low, high):
            roof_deck(m, slope, deck)
        slopes += [low, high]
    return slopes, knuckle_z, ridge_z


def roof_deck(m, slope, deck=0.12, uv='A', rake_boards=True, rake_sides=(True, True)):
    """The boards under a slope's covering (seen from below the eaves and where the covering's gone), and the rake
    boards along its sides."""
    cuts = max(1, int(slope.width / 1.2))
    m.box((slope.width, slope.length, deck), at=(slope.width * 0.5, slope.length * 0.5, -deck * 0.5),
          uv=Trim(uv, world=True), space=slope, cuts=cuts)
    if rake_boards:
        for side, x in enumerate((-0.025, slope.width + 0.025)):
            if rake_sides[side]:
                face = (-1.0, 0.0, 0.0) if side == 0 else (1.0, 0.0, 0.0)
                # From a little under the deck to just over the covering's edge.
                mid = (0.06 - deck - 0.05) * 0.5
                m.board((x, -0.02, mid), (x, slope.length - 0.02, mid), deck + 0.11, 0.05, face=face, uv='C',
                        space=slope)


# A roof's covering is open to the sky; only the next row's lap covers its upper edge, which would otherwise bake a dark
# band down every row (vertex AO has nothing but the corners to shade with).
ROOF_AO = 0.78
TIN_SHEET = 0.8  # the width of one corrugated sheet in strip F: its seams fall every 0.8 m along U


def tin(m, slope, row=0.8, lap=0.1, sheets=(1, 2, 2, 2, 3), thick=0.02, start=-0.08, top=None, missing=0.03,
        loose=0.08, replaced=0.12, skip=None):
    """Corrugated tin in rows of sheets from the eave up, each row lapping over the one below. A piece is one to three
    of the texture's 0.8 m sheets (sheets picks the count), lined up with its seams; the pattern runs on down the whole
    slope, so rust streaks carry from row to row, except on pieces that were replaced (another stretch of the strip).
    Now and then a piece is gone or lifting. skip(x, y) -> True leaves a spot bare (under a dormer)."""
    rng = m.rng
    top = slope.length + 0.02 if top is None else top
    u_base = rng.randrange(8) * TIN_SHEET
    # Equal rows no longer than a sheet, from the eave to the top.
    span = top - start
    rows = max(1, math.ceil((span - lap) / (row - lap) - 1e-6))
    h = (span + (rows - 1) * lap) / rows
    for r in range(rows):
        y = start + r * (h - lap)
        x = -0.04
        used = 0  # texture sheets laid so far in this row: keeps the pieces on the strip's seams
        last = False
        while not last:
            count = rng.choice(sheets)
            w = count * TIN_SHEET
            if slope.width + 0.04 - (x + w) < TIN_SHEET * 0.6:
                w = slope.width + 0.04 - x
                last = True
            cx, cy = x + w * 0.5, y + h * 0.5
            if (skip is None or not skip(cx, cy)) and (r == 0 or count > 1 or last or rng.random() > missing):
                lift = thick * 0.5 + (0.004 if used % 2 else 0.0)
                tilt = -math.degrees(math.atan2(thick * 1.2, h))
                roll = rng.uniform(-0.4, 0.4)
                if rng.random() < loose:
                    roll += rng.choice((-1.0, 1.0)) * rng.uniform(1.2, 2.5)
                    lift += 0.015
                u_left = u_base + used * TIN_SHEET
                if rng.random() < replaced:
                    u_left += rng.randrange(1, 8) * TIN_SHEET
                matrix = place((cx, cy + rng.uniform(-0.015, 0.015), lift), (tilt, roll, rng.uniform(-0.3, 0.3)))
                # trim_uv measures U from the box's middle.
                m.box((w, h, thick), matrix=matrix, uv=Trim('F', u=u_left + 0.5 * w), space=slope, ao_floor=ROOF_AO,
                      drop=('-z', '+y') if r < rows - 1 else ('-z',))
            used += count
            x += w - 0.02


def shingles(m, slope, course=0.2, butt=0.03, piece=(1.4, 2.6), start=-0.08, top=None, missing=0.02, skip=None):
    """Wooden shakes in courses from the eave up: each course a row of pieces whose butt edge stands proud of the
    course below, following the texture's rows (strip E); a few pieces sit crooked, a few are gone."""
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
            spans = [(x_lo, x + w)]
            if k >= 2 and w > 0.9 and rng.random() < missing * 3.0:
                # A few shakes gone: a short gap in the piece shows the deck.
                gap = rng.uniform(0.2, 0.45)
                at = rng.uniform(x_lo + 0.3, x + w - 0.3 - gap)
                spans = [(x_lo, at), (at + gap, x + w)]
            for a, b in spans:
                cx, cy = (a + b) * 0.5, y + h * 0.5
                if skip is not None and skip(cx, cy):
                    continue
                tilt = -math.degrees(math.atan2(butt, h))
                jitter = rng.uniform(-0.012, 0.012)
                matrix = place((cx, cy + jitter, butt * 0.5 + 0.004 * (k % 2)), (tilt, rng.uniform(-0.4, 0.4), 0.0))
                m.box((b - a, h, butt), matrix=matrix, uv=Trim('E', lane=k % 4), space=slope, ao_floor=ROOF_AO,
                      drop=('-z', '+y'))
            x += w
        y += exposure
        k += 1


def log(m, p0, p1, r=0.2, out=(0.0, -1.0, 0.0), r1=None, sides=10, caps=(True, True), space=None, v=None):
    """A round log from p0 to p1 (its axis) on strip B: its outer half (toward out) is one of the strip's two logs from
    bottom to top, so the chinking painted between them falls where logs meet. r1 tapers it. Its sawn ends show end
    grain (WoodEndGrain), except on thin poles."""
    p0, p1 = Vector(p0), Vector(p1)
    x = p1 - p0
    if x.cross(-Vector(out)).z < 0.0:  # keep the frame's Z up, so the log's bottom maps to the strip's bottom
        p0, p1 = p1, p0
        caps = (caps[1], caps[0])
        r, r1 = (r if r1 is None else r1), r
    v = m.rng.choice((0.0, 0.4)) if v is None else v
    thick = max(r, r1 or r) >= 0.08
    m.cylinder(p0, p1, r, r1, sides=sides, uv=Trim('B', v=v), face=out, phase=-90.0, caps=caps, space=space,
               cap_uv=EndGrain() if thick else Trim('C', width=2.0 * max(r, r1 or r)),
               cap_mat='endgrain' if thick else None)


def ridge_cap(m, front, back, uv='F', width=0.22, thick=0.025, lift=0.03):
    """A cap over the ridge where two slopes meet: a strip down each side, bending with the sag."""
    for slope in (front, back):
        cuts = max(1, int(slope.width / 1.0))
        m.box((slope.width + 0.06, width, thick),
              matrix=place((slope.width * 0.5, slope.length - width * 0.5 + 0.02, lift + thick * 0.5), (-2.0, 0.0, 0.0)),
              uv=Trim(uv, lane=0) if uv in LANES else uv, space=slope, cuts=cuts)


# --- Stone, chimneys, lanterns ---

def plinth(m, x0, x1, y0, y1, height, out=0.08, bevel=0.0):
    """A fieldstone foundation (strip D) around the footprint, standing out of the walls by out."""
    rng = m.rng
    deep = 0.3
    # The front and back run the whole width; the sides fit between them (end faces touching a neighbor's side
    # would bake black and darken the whole face).
    specs = [((x0 - out, y0 - out), (x1 + out, y0 - out), 0.0), ((x1 + out, y0 - out), (x1 + out, y1 + out), deep),
             ((x1 + out, y1 + out), (x0 - out, y1 + out), 0.0), ((x0 - out, y1 + out), (x0 - out, y0 - out), deep)]
    for p0, p1, inset in specs:
        space = wall_space(p0, p1, 0.0)
        length = (Vector(p1) - Vector(p0)).length - 2.0 * inset
        # Vertices every metre or so along it: the baked ambient occlusion lives on vertices.
        m.box((length, deep, height + 0.1), at=(inset + length * 0.5, deep * 0.5, height * 0.5 - 0.05),
              uv=Trim('D', world=True, u=rng.uniform(0.0, U_REPEAT)), space=space, bevel=bevel,
              cuts=max(0, int(length / 1.1)))


def stone_stack(m, center, size, height, taper=0.0, bevel=0.03, rot=0.0, space=None, uv='D'):
    """A block of fieldstone (a chimney stack, a pier) standing on center, size (x, y) at its foot, narrowing by the
    fraction taper toward its top, turned rot degrees about Z."""
    x, y, z = center

    def narrow(co):
        k = 1.0 - taper * (co.z / height + 0.5)
        return Vector((co.x * k, co.y * k, co.z))
    m.emit(_box((size[0], size[1], height), bevel), Trim(uv, world=True), 'trim',
           place((x, y, z + height * 0.5), (0.0, 0.0, rot)), space, shape=narrow if taper else None)


def lantern(m, at, hang=0.0, mat_frame='metal'):
    """A small iron lantern with lit glass, its middle at at; hang > 0 adds a hook and a chain of that length above it.
    Adds SOCKET_Light at the glass."""
    x, y, z = at
    frame = Tile('MetalRust')
    m.box((0.22, 0.22, 0.045), at=(x, y, z + 0.155), uv=frame, mat=mat_frame, bevel=0.01)
    m.cylinder((x, y, z + 0.175), (x, y, z + 0.26), 0.12, 0.03, sides=8, uv=frame, mat=mat_frame, caps=(True, False))
    m.box((0.16, 0.16, 0.24), at=(x, y, z), uv=Trim('H4', fit=True), mat='glow')  # untextured, but needs real UVs
    m.box((0.22, 0.22, 0.04), at=(x, y, z - 0.14), uv=frame, mat=mat_frame, bevel=0.01)
    for dx in (-0.09, 0.09):
        for dy in (-0.09, 0.09):
            m.box((0.025, 0.025, 0.27), at=(x + dx, y + dy, z), uv=frame, mat=mat_frame)
    m.box((0.02, 0.1, 0.02), at=(x, y, z + 0.3), uv=frame, mat=mat_frame)
    if hang > 0.0:
        m.box((0.02, 0.02, hang), at=(x, y, z + 0.3 + hang * 0.5), uv=frame, mat=mat_frame)
    m.socket('Light', (x, y, z))


# --- Previews of the whole set ---

# The overview's two rows, back then front: the big buildings behind the small ones.
BUILDINGS = [['LookoutTower', 'Barn', 'Farmhouse', 'Windmill'],
             ['LogCabin', 'Cottage', 'Well', 'Outhouse', 'GunRack', 'Bridge']]


def overview(rows=None, gap=3.0, out=None):
    """Builds every building in one scene, in rows side by side along X, and renders them together for scale:
    Saved/ArtPreviews/Buildings/overview.png:

        blender -b --factory-startup --python-expr "import sys; sys.path.append('Tools/Blender');
            import looter_buildings; looter_buildings.overview()"
    """
    import runpy
    rows = rows or BUILDINGS
    bpy.ops.wm.read_factory_settings(use_empty=True)
    shown = []
    y = 0.0
    for row in rows:
        x = 0.0
        placed_row = []
        for name in row:
            before = set(bpy.context.scene.objects)
            runpy.run_path(os.path.join(SOURCE_DIR, name + '.py'), run_name='__overview__')
            roots = [o for o in bpy.context.scene.objects if o not in before and o.parent is None and o.type == 'MESH'
                     and not o.name.startswith(('_', 'UCX_'))]
            placed = [o for o in roots if not o.get('Mounted')]
            corners = [o.matrix_world @ Vector(c) for o in placed for c in o.bound_box]
            lo, hi = min(c.x for c in corners), max(c.x for c in corners)
            for o in roots:
                o.location.x += x - lo
                o.location.y += y
            x += hi - lo + gap
            placed_row += roots
        # Center the row, then step the next one forward.
        for o in placed_row:
            o.location.x -= (x - gap) * 0.5
        ys = [(o.matrix_world @ Vector(c)).y for o in placed_row for c in o.bound_box]
        depth = max(ys) - min(ys)
        y -= depth * 0.5 + 13.0
        shown += placed_row
    bpy.context.view_layer.update()
    return lt.preview(shown, out or lt.preview_path('Buildings', 'overview'), view=(-0.15, -1.6, 0.85), fit=0.66,
                      lens=35.0)
