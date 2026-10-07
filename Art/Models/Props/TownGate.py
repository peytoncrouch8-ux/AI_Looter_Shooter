"""The town gate of Ransom's Rest (Docs/Areas/RansomsRest.md: Main Street's west end, where the farm road comes in
through the orchard; Main 3 "Cold Welcome", the Unpaid's fight at the gate; Hob perches on its north post): two posts
11.6 m apart centre to centre (layout.json's obstacles townGateNorth and townGateSouth, 4 m tall, about 1.2 m square in
plan) and the town's name across them, RANSOM'S REST in Rye, REST EASY on the back for those leaving.

  TownGate         the game's gate: the timber gate, option B of three, which the user picked on 2026-10-07. Squared
                   hewn posts on fieldstone piers, strapped down, a tie beam let into them under a narrow shake roof
                   (the notice board's), knee braces, and a long oxide-red board in a hewn frame hung under the beam's
                   front on iron straps, lettered in cream between two stars, shot through in the gang's raid (each
                   shot out through the back). A lantern on an iron arm on the south post, kept lit for those not yet
                   home (SOCKET_Light); the wanted poster's spot on that post's face (SOCKET_Decal); on the north post
                   at eye level a little board, POPULATION 212, the number struck out and 96 painted in beside it: half
                   the Rest left when the saint went dark, and the raid took more. Hob perches on the north post's cap
                   (SOCKET_Perch). 4.36 m tall, 12.7 m wide over its piers. Nanite, its whole mesh the fallback.

The options not picked are still built for the previews, to compare, but never exported (each has its own seed, so
leaving them out changes nothing else):
  TownGate_A       the log gate: peeled log posts standing in notched log cribs filled and heaped with fieldstone, a
                   sagging log crossbeam let into the posts and bolted through, eye bolts under it for the sign. On the
                   south post a plank for notices and a horseshoe hanging ends-down from the one nail it has left: the
                   luck ran out. 4.45 m tall, 13.0 m wide.
  TownGate_A_Sign  its name board, hung on two chains from SOCKET_Sign: three whitewashed planks lettered in black, a
                   pinstripe round them, bullet holes, and a plank hung under it: POP. 212, struck out, 96. Its pivot is
                   the hanging point, midway between the chains' eyes; it would swing about its X axis (Unreal's Y).
  TownGate_C       TownGate in mourning (the same seed draws the same gate): wreaths of dark evergreen with crepe bows
                   in place of the stars, black crepe swagged along both eaves from rosettes, ribbon tails by the
                   posts, a crepe band and bow round each post, a ribbon tied on the lantern's arm.

Axes and placing: the pivot is on the ground midway between the posts. The posts stand on the model's X axis, the north
post at x = -5.8 and the south post at x = +5.8; the gate's outward side, toward the farm road, is the model's front
(-Y, Unreal +X). With Unreal's X north and Y east (the level's), yaw -90 turns the front to face west, which puts the
north post north. SOCKET_Perch, where Hob's feet grip, faces out of town turned 50 degrees toward the south post: down
the farm road from the north post (its last stretch in layout.json lies 51 degrees south of straight out). It sits a
little behind the middle of the post's top, so his drooping tail hangs past the top's back edge.

Collision is on the posts only (UCX boxes round each post and its base), so a fight flows under and through the gate.
The gates are large props of 2-5k triangles: Nanite, with the whole mesh as the fallback (what Medium draws). A's sign
is a small prop: Nanite 0, LODs 50% and 25%.

A scripted model (Art/README.md) built with looter_buildings and looter_town.

    artrun.ps1 -Script Art\\Models\\Props\\TownGate.py -Preview
renders the look sheet into Saved/ArtPreviews/RansomsRest/TownGate/ (golden late afternoon, a 1.8 m figure for scale,
Hob on his perch; the options not picked too, and the three side by side) and the overview,
Saved/ArtPreviews/RansomsRest/TownGate_overview.png.
"""
import math
import os
import runpy
import sys

import bmesh
import bpy
from mathutils import Matrix, Vector

import looter_buildings as kit
import looter_textures as lt
import looter_town as town
from looter_buildings import Space, Trim, place

POST_X = 5.8                   # the posts' centres: 11.6 m apart
NORTH_X, SOUTH_X = -POST_X, POST_X
# Hob looks down the farm road the player comes by: out of town (-Y), turned toward the south post. From the north
# post, the road's last stretch in layout.json (towards (-1300, -2600) cm) lies 51 degrees south of straight out.
ROAD_YAW = 50.0
SMALL = dict(Nanite=0, LODs='50,25')
BACK = Space(Matrix.Rotation(math.pi, 4, 'Z'))     # a board's back face: lettering there reads from behind


# --- Mapping ---

class LogBand:
    """A log's round side on one of strip B's two logs, its chinking left out: the grain (U) along the log at world
    scale, V up across the log's band and back down (mirrored, so there is no seam), folds times round the log, so a
    thick log's bark-free wood isn't stretched round it."""

    def __init__(self, around, lane=None, band=(0.03, 0.35), folds=1):
        self.around, self.lane, self.band, self.folds = around, lane, band, folds

    def apply(self, obj, rng):
        lane = rng.choice((0.0, 0.4)) if self.lane is None else self.lane
        u0 = rng.uniform(0.0, kit.U_REPEAT)
        base = lt.TRIM_STRIPS['B'][0]
        scale = lt.TRIM_DENSITY / lt.TRIM_SIZE
        lo, hi = lane + self.band[0], lane + self.band[1]
        mesh = obj.data
        uv = mesh.uv_layers.active.data
        for p in mesh.polygons:
            for li in p.loop_indices:
                co = mesh.vertices[mesh.loops[li].vertex_index].co
                t = min(max(co.z / self.around, 0.0), 1.0) * self.folds
                f = 1.0 - abs(2.0 * (t - math.floor(t + 1e-9) if t < self.folds - 1e-9 else 1.0) - 1.0)
                uv[li].uv = ((u0 + co.x) * scale, base + (lo + (hi - lo) * f) * scale)


class Lane:
    """A squared timber on one beam face of strip C: each side's width fitted across one 20 cm lane, so the strip's
    dark edges fall on the arrises (and the chamfers), the grain along the part (its X) at world scale."""

    def __init__(self, strip='C'):
        self.strip = strip

    def apply(self, obj, rng):
        mesh = obj.data
        width = kit.LANES[self.strip]
        count = int(round(kit.strip_height(self.strip) / width))
        base = lt.TRIM_STRIPS[self.strip][0]
        scale = lt.TRIM_DENSITY / lt.TRIM_SIZE
        inset = 1.5 / lt.TRIM_DENSITY
        u0 = rng.uniform(0.0, kit.U_REPEAT)
        cos = [v.co.copy() for v in mesh.vertices]
        uv = mesh.uv_layers.active.data
        lanes = {}
        X = Vector((1.0, 0.0, 0.0))
        for p in mesh.polygons:
            n = p.normal
            if abs(n.x) > 0.7:                       # an end: flat in its own plane
                across, along = Vector((0.0, 0.0, 1.0)), Vector((0.0, 1.0, 0.0))
            else:
                across = n.cross(X).normalized()
                along = X
            key = tuple(round(c, 1) for c in n)
            if key not in lanes:
                lanes[key] = rng.randrange(count)
            proj = [c.dot(across) for c in cos]
            lo, hi = min(proj), max(proj)
            for li in p.loop_indices:
                co = cos[mesh.loops[li].vertex_index]
                t = (co.dot(across) - lo) / max(hi - lo, 1e-6)
                v = lanes[key] * width + inset + t * (width - 2.0 * inset)
                uv[li].uv = ((u0 + co.dot(along)) * scale, base + v * scale)


class Spot:
    """A small part on one spot of the trim sheet (v metres up the strip, u along it), mapped flat in its own XZ plane
    at world scale (a bullet hole's dark wood, the bare wood round it), or with fit, scaled so its height spans fit
    metres from v up (a beam's sawn end on one beam face)."""

    def __init__(self, strip, v, u, fit=None):
        self.strip, self.v, self.u, self.fit = strip, v, u, fit

    def apply(self, obj, rng):
        mesh = obj.data
        base = lt.TRIM_STRIPS[self.strip][0]
        scale = lt.TRIM_DENSITY / lt.TRIM_SIZE
        zs = [v.co.z for v in mesh.vertices]
        k, z0 = (self.fit / max(max(zs) - min(zs), 1e-6), min(zs)) if self.fit else (1.0, 0.0)
        uv = mesh.uv_layers.active.data
        for p in mesh.polygons:
            for li in p.loop_indices:
                co = mesh.vertices[mesh.loops[li].vertex_index].co
                uv[li].uv = ((self.u + co.x * k) * scale, base + (self.v + (co.z - z0) * k) * scale)


class Swatch:
    """A part on a FoliagePalette swatch (as Boardwalk.py's memorial maps its wreath): U from its first vertex (the
    root) to the vertex farthest from it (the tip), V across the swatch."""

    def __init__(self, name):
        self.name = name

    def apply(self, obj, rng):
        mesh = obj.data
        rows = len(lt.PALETTE)
        index = lt.PALETTE.index(self.name)
        v_lo, v_hi = (index + 0.15) / rows, (index + 0.85) / rows
        cos = [v.co.copy() for v in mesh.vertices]
        root = cos[0]
        tip = max(range(len(cos)), key=lambda i: (cos[i] - root).length)
        direction = (cos[tip] - root).normalized()
        side = direction.cross(Vector((0.0, 0.0, 1.0)) if abs(direction.z) < 0.9 else Vector((1.0, 0.0, 0.0)))
        side.normalize()
        along = [(c - root).dot(direction) for c in cos]
        across = [(c - root).dot(side) for c in cos]
        lo, hi = min(along), max(along)
        a_lo, a_hi = min(across), max(across)
        uv = mesh.uv_layers.active.data
        for p in mesh.polygons:
            for li in p.loop_indices:
                i = mesh.loops[li].vertex_index
                u = (along[i] - lo) / max(hi - lo, 1e-6)
                w = (across[i] - a_lo) / max(a_hi - a_lo, 1e-6)
                uv[li].uv = (min(max(u, 0.01), 0.99), v_lo + w * (v_hi - v_lo))


class RingSwatch:
    """A wreath's core ring (sections of corners, made section by section) on the dark root end of a FoliagePalette
    swatch: U by section, V by corner, so every face keeps UV area (mapped along the ring's length, as Swatch would,
    some of its faces get none, and no tangents in Unreal; Graves.py's wreath works round it the same way)."""

    def __init__(self, name, corners=4):
        self.name, self.corners = name, corners

    def apply(self, obj, rng):
        mesh = obj.data
        rows = len(lt.PALETTE)
        index = lt.PALETTE.index(self.name)
        v_lo, v_hi = (index + 0.15) / rows, (index + 0.85) / rows
        uv = mesh.uv_layers.active.data
        for p in mesh.polygons:
            for li in p.loop_indices:
                section, corner = divmod(mesh.loops[li].vertex_index, self.corners)
                uv[li].uv = (0.08 + 0.16 * (section % 2), v_lo + (v_hi - v_lo) * (0.15 + 0.7 * corner / (self.corners - 1)))


# --- Parts ---

def sheet_xs(xs, around, rows):
    """A flat grid facing -Y with columns at the x values in xs and rows up to around (a log's side, unrolled)."""
    tb = kit._new_bmesh()
    grid = [[tb.verts.new((x, 0.0, around * j / rows)) for j in range(rows + 1)] for x in xs]
    for i in range(len(xs) - 1):
        for j in range(rows):
            tb.faces.new((grid[i][j], grid[i + 1][j], grid[i + 1][j + 1], grid[i][j + 1]))
    return tb


def log(m, p0, p1, r0, r1=None, sides=10, cuts=0, rings=(), caps=(True, True), face=(0.0, -1.0, 0.0), sag=0.0,
        chamfer=(0.0, 0.0), wobble=0.0, lane=None, space=None):
    """A peeled log from p0 to p1 (its axis), its radius r0 tapering to r1, on strip B (LogBand), its sawn ends on
    WoodEndGrain. cuts adds evenly spaced rings along it and rings adds rings at these distances from p0 (for its sag
    and its baked shading); sag drops its middle; chamfer rounds off each end's rim by that much; wobble makes the
    inner rings a little uneven (a fraction of the radius)."""
    rng = m.rng
    r1 = r0 if r1 is None else r1
    p0, p1 = Vector(p0), Vector(p1)
    length = (p1 - p0).length
    xs = [0.0, length] + [length * k / (cuts + 1) for k in range(1, cuts + 1)] + [d for d in rings if 0.02 < d < length - 0.02]
    if chamfer[0] > 0.0:
        xs.append(chamfer[0])
    if chamfer[1] > 0.0:
        xs.append(length - chamfer[1])
    xs = sorted(set(round(x, 5) for x in xs))
    wob = [1.0 if i in (0, len(xs) - 1) else 1.0 + rng.uniform(-wobble, wobble) for i in range(len(xs))]
    around = 1.0
    start = math.radians(-90.0)

    def radius(i):
        r = (r0 + (r1 - r0) * xs[i] / max(length, 1e-6)) * wob[i]
        if i == 0:
            r -= chamfer[0]
        if i == len(xs) - 1:
            r -= chamfer[1]
        return r

    def roll(co):
        i = min(range(len(xs)), key=lambda k: abs(xs[k] - co.x))
        r = radius(i)
        a = start + 2.0 * math.pi * co.z / around
        drop = sag * math.sin(math.pi * co.x / max(length, 1e-6))
        return Vector((co.x, -r * math.cos(a), r * math.sin(a) - drop))

    frame = kit.toward(p0, p1, face)
    # One fold of the band covers about 0.64 m of the log's girth (0.32 m of wood up and back down).
    folds = max(1, int(round(math.pi * (r0 + r1) / 0.64)))
    m.emit(sheet_xs(xs, around, sides), LogBand(around, lane, folds=folds), 'trim', frame, space, shape=roll,
           smooth=True)
    for end, i, turn in ((0, 0, -90.0), (1, len(xs) - 1, 90.0)):
        if caps[end]:
            disc_phase = start if end == 0 else math.pi - start
            cap = kit._disc(radius(i), sides, disc_phase)
            matrix = frame @ Matrix.Translation((xs[i], 0.0, 0.0)) @ place(rot=(0.0, 0.0, turn))
            m.emit(cap, kit.EndGrain(), 'endgrain', matrix, space)


def timber(m, p0, p1, w, d, face=(0.0, -1.0, 0.0), bevel=0.018, cuts=None, drop=(), space=None):
    """A squared hewn timber from p0 to p1 (its middle line): w across, d deep toward face, chamfered, on strip C."""
    length = (Vector(p1) - Vector(p0)).length
    cuts = int(length / 1.2) if cuts is None else cuts
    matrix = kit.toward(p0, p1, face) @ Matrix.Translation((length * 0.5, 0.0, 0.0))
    m.emit(kit._box((length, d, w), bevel, cuts, drop), Lane(), 'trim', matrix, space)


IRON = Trim('H3', fit=True)


class DullIron(Trim):
    """Iron on the trim sheet's strap strip pinned to its dullest stretch (3.58-3.69 m along it, the least rust)
    instead of a random one: the straps at the posts' feet, where a random stretch put one on a bright orange rust
    patch. It still draws the random number a random stretch would, so every part after it comes out as before."""

    def __init__(self):
        super().__init__('H3', fit=True, u=3.635)

    def apply(self, obj, rng):
        rng.uniform(0.0, kit.U_REPEAT)
        super().apply(obj, rng)


def iron(m, size, at, rot=(0.0, 0.0, 0.0), space=None, drop=(), bevel=0.0, uv=IRON):
    m.emit(kit._box(size, bevel, 0, drop), uv, 'trim', place(at, rot), space)


def wire_loop(m, center, u, v, a, b, wire, seg=6, sides=3, space=None):
    """A loop of round iron wire: an ellipse of half-axes a (along u) and b (along v) round center."""
    tb = kit._new_bmesh()
    c, u, v = Vector(center), Vector(u).normalized(), Vector(v).normalized()
    n = u.cross(v).normalized()
    rings = []
    for i in range(seg):
        t = 2.0 * math.pi * i / seg
        p = c + u * (a * math.cos(t)) + v * (b * math.sin(t))
        tangent = (-u * (a * math.sin(t)) + v * (b * math.cos(t))).normalized()
        out = tangent.cross(n).normalized()
        rings.append([tb.verts.new(p + out * (wire * math.cos(2.0 * math.pi * k / sides)) +
                                   n * (wire * math.sin(2.0 * math.pi * k / sides))) for k in range(sides)])
    for i in range(seg):
        r0, r1 = rings[i], rings[(i + 1) % seg]
        for k in range(sides):
            tb.faces.new((r0[k], r1[k], r1[(k + 1) % sides], r0[(k + 1) % sides]))
    bmesh.ops.recalc_face_normals(tb, faces=tb.faces[:])
    m.emit(tb, IRON, 'trim', None, space, smooth=True)


def chain(m, top, bottom, links, wire=0.0085, space=None):
    """A short chain from top to bottom: links of round wire, each turned a quarter from the last."""
    top, bottom = Vector(top), Vector(bottom)
    d = bottom - top
    pitch = d.length / links
    d.normalize()
    side = Vector((1.0, 0.0, 0.0)) if abs(d.x) < 0.9 else Vector((0.0, 1.0, 0.0))
    s1 = d.cross(side).normalized()
    s2 = d.cross(s1).normalized()
    for k in range(links):
        a = pitch * 0.5 + wire * 1.4
        wire_loop(m, top + d * (pitch * (k + 0.5)), d, s1 if k % 2 == 0 else s2, a, a * 0.52, wire, space=space)


def flat(points):
    """A flat polygon in the XZ plane facing -Y (points (x, z), any winding)."""
    tb = kit._new_bmesh()
    face = tb.faces.new([tb.verts.new((x, 0.0, z)) for x, z in points])
    face.normal_update()
    if face.normal.y > 0.0:
        face.normal_flip()
    return tb


def bullet_hole(m, x, z, y, rng, back=False, space=None):
    """A bullet hole in a board's face at (x, y, z) facing -Y (back=True: the face at y facing +Y, where the shot came
    out, torn wider): a dark hole in a splintered ring of bare wood where the paint flew off."""
    r = rng.uniform(0.0085, 0.011) * (1.25 if back else 1.0)
    n = 7
    angles = [2.0 * math.pi * k / n + rng.uniform(-0.25, 0.25) for k in range(n)]
    inner = [(r * math.cos(a), r * math.sin(a)) for a in angles]
    reach = (2.0, 3.2) if back else (1.7, 2.8)
    outer = [(r * rng.uniform(*reach) * math.cos(a), r * rng.uniform(*reach) * math.sin(a)) for a in angles]
    turn = Matrix.Rotation(math.pi, 4, 'Z') if back else Matrix.Identity(4)
    lift = 0.0026
    for points, spec, proud in ((outer, Spot('A', 0.1, 1.3 + x), lift - 0.0006), (inner, Spot('C', 0.195, 2.0 + x), lift)):
        matrix = Matrix.Translation((x, y + (proud if back else -proud), z)) @ turn
        m.emit(flat(points), spec, 'trim', matrix, space)


def crepe_swag(m, x0, x1, z_top, y, sag, rng, cols=12, rows=3, layers=2, space=None):
    """A swag of black crepe pinned along a line at y (its front toward -Y) from x0 to x1, z_top(x) its pinned top
    edge: gathered tight at the ends, sagging by sag in the middle and bellying out, folds along the drape. Two layers
    back to back read from both sides; one does where something always hides its back."""
    tb = kit._new_bmesh()
    width = x1 - x0
    phase = rng.uniform(0.0, 1.0)
    for layer, dy in ((0, 0.0), (1, 0.007))[:layers]:
        grid = []
        for i in range(cols + 1):
            u = i / cols
            belly = math.sin(math.pi * u)
            x = x0 + width * u
            top = z_top(x) - 0.03 * belly
            bottom = z_top(x) - 0.06 - sag * belly
            column = []
            for j in range(rows + 1):
                t = j / rows
                fold = 0.014 * math.sin(2.0 * math.pi * (t * 2.0 + phase + u * 0.6)) * belly
                yy = y - 0.01 - 0.05 * belly * math.sin(math.pi * (0.25 + 0.75 * t)) - fold + dy
                column.append(tb.verts.new((x, yy, top + (bottom - top) * t)))
            grid.append(column)
        for i in range(cols):
            for j in range(rows):
                quad = (grid[i][j], grid[i][j + 1], grid[i + 1][j + 1], grid[i + 1][j])
                tb.faces.new(quad if layer == 0 else quad[::-1])
    tb.normal_update()
    if list(tb.faces)[0].normal.y > 0.0:
        bmesh.ops.reverse_faces(tb, faces=tb.faces[:])
    spec, key = town.finish('crepe')
    m.emit(tb, spec, key, None, space, smooth=True)


def rosette(m, at, r=0.075, space=None):
    """A rosette of gathered crepe and its button, on a face at at (its front toward -Y)."""
    town.lathe(m, [(0.0, 0.028), (0.034, 0.022), (r, 0.003), (0.0, -0.006)], at=at, sides=7, look='crepe', space=space,
               axis=Matrix.Rotation(math.radians(90.0), 4, 'X'), phase=math.pi / 7)
    town.box(m, (0.04, 0.02, 0.04), at=(at[0], at[1] - 0.033, at[2]), rot=(0.0, 45.0, 0.0), look='crepe', space=space,
             drop=('+y',))


def ribbon(m, top, length, lean, rng, width=0.052, space=None):
    """A ribbon tail of crepe hanging from top, a little askew."""
    x, y, z = top
    town.box(m, (width, 0.004, length), at=(x + math.sin(math.radians(lean)) * length * 0.5, y, z - length * 0.5),
             rot=(rng.uniform(-4.0, 4.0), lean, rng.uniform(-6.0, 6.0)), look='crepe', space=space)


def wreath(m, cx, cz, face, R=0.2, sprigs=32):
    """A mourning wreath of dark evergreen hung from a nail on a face at y = face (its front toward -Y): sprigs round
    a core ring on the foliage swatches (as the town memorial's), a crepe bow with tails at its foot."""
    rng = m.rng
    iron(m, (0.018, 0.03, 0.018), (cx, face - 0.012, cz + R + 0.035))
    tb = kit._new_bmesh()
    rings = []
    for i in range(10):
        a = 2.0 * math.pi * i / 10
        radial = Vector((math.cos(a), 0.0, math.sin(a)))
        c = Vector((cx, face - 0.042, cz)) + radial * R
        rings.append([tb.verts.new(c + radial * (0.04 * math.cos(b)) + Vector((0.0, -0.035 * math.sin(b), 0.0)))
                      for b in (math.pi * (0.25 + 0.5 * j) for j in range(4))])
    for i in range(10):
        r0, r1 = rings[i], rings[(i + 1) % 10]
        for j in range(4):
            tb.faces.new((r0[j], r1[j], r1[(j + 1) % 4], r0[(j + 1) % 4]))
    bmesh.ops.recalc_face_normals(tb, faces=tb.faces[:])
    m.emit(tb, RingSwatch('GrassDeep'), 'foliage', None, smooth=True)
    for k in range(sprigs):
        a = 2.0 * math.pi * (k + rng.uniform(-0.3, 0.3)) / sprigs
        radial = Vector((math.cos(a), 0.0, math.sin(a)))
        tangent = Vector((-math.sin(a), 0.0, math.cos(a)))
        ring_r = R + (0.034 if k % 2 else -0.034) + rng.uniform(-0.01, 0.01)
        base = Vector((cx, face - 0.075 - rng.uniform(0.0, 0.025), cz)) + radial * ring_r
        tip = base + tangent * rng.uniform(0.09, 0.12) + radial * rng.uniform(-0.02, 0.03) + Vector((0.0, -0.015, 0.0))
        mid = base.lerp(tip, 0.45) + Vector((0.0, -0.012, 0.0))
        side = radial * rng.uniform(0.026, 0.034)
        sb = kit._new_bmesh()
        sb.faces.new([sb.verts.new(p) for p in (base, mid - side, tip, mid + side)])
        m.emit(sb, Swatch(rng.choice(('Fern', 'GrassDeep', 'CloverDark', 'Moss'))), 'foliage', None)
    town.crepe_bow(m, None, (cx, 0.0, cz - R - 0.03), scale=0.85, tails=0.34, y=face - 0.09)


# --- Option A: the log gate ---

A_TOP = 4.45                   # the posts' sawn tops
A_POST_R = (0.228, 0.192)      # the posts' radius at the foot and the top
A_LEAN = {NORTH_X: (0.035, -0.02), SOUTH_X: (-0.02, 0.025)}   # how far each top stands off its foot (x, y)
A_BEAM_Z = 4.05                # the crossbeam's axis at the posts
A_BEAM_HALF = 6.5              # its ends
A_BEAM_R = (0.205, 0.172)      # its butt (north) and top (south)
A_BEAM_SAG = 0.045
A_HANG_Z = 3.76                # the eye bolts' rings: the sign's pivot
A_CHAIN_X = 2.55               # the chains
CRIB = 1.2                     # the stone-filled cribs round the posts' feet: square
CRIB_R = 0.1                   # their logs
CRIB_PITCH = 0.155             # from course to course (X and Y in turn)
CRIB_COURSES = 7


def beam_sag(x):
    return A_BEAM_SAG * math.sin(math.pi * (x + A_BEAM_HALF) / (2.0 * A_BEAM_HALF))


def beam_radius(x):
    return A_BEAM_R[0] + (A_BEAM_R[1] - A_BEAM_R[0]) * (x + A_BEAM_HALF) / (2.0 * A_BEAM_HALF)


def post_axis(cx, z):
    """The point on a leaning post's axis at height z."""
    dx, dy = A_LEAN[cx]
    t = (z + 0.25) / (A_TOP + 0.25)
    return Vector((cx + dx * t, dy * t, z))


def post_radius(z):
    return A_POST_R[0] + (A_POST_R[1] - A_POST_R[0]) * (z + 0.25) / (A_TOP + 0.25)


def crib(m, cx):
    """A crib of notched logs round a post's foot, filled with fieldstone and heaped with it on top."""
    rng = m.rng
    half = CRIB * 0.5 - CRIB_R
    for k in range(CRIB_COURSES):
        z = CRIB_R - 0.025 + k * CRIB_PITCH
        for side in (-1.0, 1.0):
            r = CRIB_R * rng.uniform(0.93, 1.06)
            over0, over1 = rng.uniform(0.12, 0.2), rng.uniform(0.12, 0.2)
            if k % 2 == 0:                       # along X: the crib's front and back
                y = side * half + rng.uniform(-0.01, 0.01)
                p0, p1, face = (cx - half - over0, y, z), (cx + half + over1, y, z), (0.0, side, 0.0)
            else:                                # along Y: its two sides
                x = cx + side * half + rng.uniform(-0.01, 0.01)
                p0, p1, face = (x, -half - over0, z), (x, half + over1, z), (side, 0.0, 0.0)
            log(m, p0, p1, r, r * rng.uniform(0.9, 1.0), sides=8, face=face)
    top = CRIB_R - 0.025 + (CRIB_COURSES - 1) * CRIB_PITCH + 0.02
    # The fill: fieldstone seen between the logs, a heap of it round the post and loose stones on that.
    fill = CRIB * 0.5 - 0.1
    town.box(m, (fill * 2.0, fill * 2.0, top + 0.02), at=(cx, 0.0, (top + 0.02) * 0.5 - 0.02), look='world_D',
             drop=('-z',))
    tb = kit._new_bmesh()
    ring = [tb.verts.new((cx + sx * fill, sy * fill, top)) for sx, sy in ((-1, -1), (1, -1), (1, 1), (-1, 1))]
    peak = [tb.verts.new((cx + sx * 0.28 + rng.uniform(-0.03, 0.03), sy * 0.28 + rng.uniform(-0.03, 0.03),
                          top + rng.uniform(0.17, 0.24))) for sx, sy in ((-1, -1), (1, -1), (1, 1), (-1, 1))]
    for i in range(4):
        tb.faces.new((ring[i], ring[(i + 1) % 4], peak[(i + 1) % 4], peak[i]))
    tb.faces.new(peak)
    bmesh.ops.recalc_face_normals(tb, faces=tb.faces[:])
    tb.normal_update()
    for f in tb.faces:
        if f.normal.z < 0.0:
            f.normal_flip()
    m.emit(tb, Trim('D', world=True), 'trim')
    for k in range(6):
        a = 2.0 * math.pi * k / 6 + rng.uniform(-0.3, 0.3)
        d = rng.uniform(0.3, 0.42)
        size = (rng.uniform(0.19, 0.29), rng.uniform(0.15, 0.23), rng.uniform(0.12, 0.18))
        town.box(m, size, at=(cx + d * math.cos(a), d * math.sin(a), top + 0.07 + size[2] * 0.2),
                 rot=(rng.uniform(-14, 14), rng.uniform(-14, 14), math.degrees(a) + rng.uniform(-25, 25)),
                 look='world_D', drop=('-z',))
    return top


def horseshoe(m, at, turn):
    """A horseshoe hanging from one nail on a post's front (at: the nail), its ends down: it slipped round on the one
    nail it has left, and the luck ran out of it."""
    x, y, z = at
    r, w, t = 0.062, 0.024, 0.011
    tb = kit._new_bmesh()
    seg = 9
    span = math.radians(250.0)
    rings = []
    for i in range(seg + 1):
        a = -span * 0.5 + span * i / seg
        radial = Vector((math.sin(a), 0.0, math.cos(a)))
        c = radial * r
        rings.append([tb.verts.new(c + radial * (dr * w * 0.5) + Vector((0.0, dy * t * 0.5, 0.0)))
                      for dr, dy in ((-1, -1), (1, -1), (1, 1), (-1, 1))])
    for i in range(seg):
        for k in range(4):
            tb.faces.new((rings[i][k], rings[i + 1][k], rings[i + 1][(k + 1) % 4], rings[i][(k + 1) % 4]))
    tb.faces.new(rings[0][::-1])
    tb.faces.new(rings[-1])
    bmesh.ops.recalc_face_normals(tb, faces=tb.faces[:])
    # Built toe up, ends down; it hangs from the nail through its toe, swung a little round it (turn, degrees).
    a = math.radians(turn)
    matrix = Matrix.Translation((x, y - t * 0.5, z)) @ Matrix.Rotation(-a, 4, 'Y') @ Matrix.Translation((0.0, 0.0, -r))
    m.emit(tb, IRON, 'trim', matrix)
    iron(m, (0.022, 0.03, 0.022), (x, y - 0.012, z - 0.012))


def notice_plank(m, cx, z0, z1):
    """Two boards nailed up a log post's front for notices, flat for a poster. Returns the socket point on its face."""
    rng = m.rng
    zc = (z0 + z1) * 0.5
    r = post_radius(zc)
    c = post_axis(cx, zc)
    y = c.y - r - 0.012
    for dx in (-0.095, 0.095):
        town.board(m, (c.x + dx, y, z0 + rng.uniform(-0.02, 0.0)), (c.x + dx, y, z1 + rng.uniform(0.0, 0.025)), 0.185,
                   0.03, look='A')
        for zz in (z0 + 0.06, z1 - 0.06):
            iron(m, (0.022, 0.012, 0.022), (c.x + dx + rng.uniform(-0.02, 0.02), y - 0.019, zz))
    return Vector((c.x, y - 0.015 - 0.001, zc))


def perch(m, top, back):
    """SOCKET_Perch on a post's flat top: facing down the farm road, set back from the middle of the top so Hob's tail
    (which droops a centimetre or two below his feet 15-25 cm behind them) hangs past the top's back edge."""
    a = math.radians(ROAD_YAW)
    forward = Vector((math.sin(a), -math.cos(a), 0.0))
    m.socket('Perch', top - forward * back, (0.0, 0.0, ROAD_YAW))


def gate_a(name, seed):
    """Option A, the log gate: peeled log posts in stone-filled cribs, a log crossbeam let into them, the sign hung from
    it (TownGate_A_Sign on SOCKET_Sign)."""
    m = town.Model(name, seed=seed)
    for cx in (NORTH_X, SOUTH_X):
        top_crib = crib(m, cx)
        log(m, (cx, 0.0, -0.25), post_axis(cx, A_TOP), A_POST_R[0], A_POST_R[1], sides=12, cuts=3,
            rings=(top_crib + 0.5, A_BEAM_Z + 0.25 - 0.32, A_BEAM_Z + 0.25 + 0.3), caps=(False, True), wobble=0.02,
            chamfer=(0.0, 0.02))
    m.section('posts and cribs')
    # The crossbeam: one long log let into both posts, sagging a little, its ends past them.
    log(m, (-A_BEAM_HALF, 0.0, A_BEAM_Z), (A_BEAM_HALF, 0.0, A_BEAM_Z), A_BEAM_R[0], A_BEAM_R[1], sides=12, cuts=11,
        sag=A_BEAM_SAG, chamfer=(0.012, 0.012), lane=0.4)
    # Through-bolts at the crossings: a square washer and nut on both faces.
    for cx in (NORTH_X, SOUTH_X):
        c = post_axis(cx, A_BEAM_Z)
        for side in (-1.0, 1.0):
            y = side * (beam_radius(cx) + 0.004)
            iron(m, (0.13, 0.016, 0.13), (c.x, y, A_BEAM_Z - beam_sag(cx)), rot=(0.0, 45.0 * (side > 0), 0.0))
            iron(m, (0.06, 0.04, 0.06), (c.x, y + side * 0.02, A_BEAM_Z - beam_sag(cx)))
    # The eye bolts the sign hangs from: through the beam, a washer and nut on top, an eye under it.
    for x in (-A_CHAIN_X, A_CHAIN_X):
        z_axis = A_BEAM_Z - beam_sag(x)
        r = beam_radius(x)
        iron(m, (0.1, 0.1, 0.014), (x, 0.0, z_axis + r - 0.002))
        iron(m, (0.05, 0.05, 0.035), (x, 0.0, z_axis + r + 0.02))
        iron(m, (0.022, 0.022, z_axis - r - A_HANG_Z), (x, 0.0, (z_axis - r + A_HANG_Z) * 0.5 + 0.02))
        wire_loop(m, (x, 0.0, A_HANG_Z), (0.0, 0.0, 1.0), (0.0, 1.0, 0.0), 0.036, 0.036, 0.0105, seg=8, sides=4)
    m.section('beam and iron')
    # The south post: a plank for notices (the wanted poster's spot) and the horseshoe over it.
    decal = notice_plank(m, SOUTH_X, 1.32, 1.86)
    shoe = post_axis(SOUTH_X, 2.2)
    horseshoe(m, (shoe.x - 0.03, shoe.y - post_radius(2.2) - 0.004, 2.2), 14.0)
    m.section('plank and horseshoe')
    m.socket('Sign', (0.0, 0.0, A_HANG_Z))
    m.socket('Decal', decal)
    perch(m, post_axis(NORTH_X, A_TOP), 0.05)
    for cx in (NORTH_X, SOUTH_X):
        m.hull((CRIB + 0.06, CRIB + 0.06, 1.2), at=(cx, 0.0, 0.55))
        c = post_axis(cx, (A_TOP + 1.0) * 0.5)
        m.hull((0.46, 0.46, A_TOP - 1.0), at=(c.x, c.y, (A_TOP + 1.0) * 0.5))
    return m


# --- Option A's hanging sign ---

SIGN_W = 5.6                   # the board's width: the chains hang 0.25 m in from its ends
SIGN_PLANK = 0.31              # three planks
SIGN_T = 0.05
SIGN_TOP = -0.3                # the board's top edge under the eyes (the pivot, z = 0)


def sign_a(name, seed):
    """TownGate_A_Sign: the name board on its two chains, its pivot the hanging point (the eyes, z = 0)."""
    m = town.Model(name, seed=seed)
    rng = m.rng
    top = SIGN_TOP
    bottom = top - 3 * SIGN_PLANK
    for k in range(3):
        zc = top - SIGN_PLANK * (k + 0.5)
        x0, x1 = -SIGN_W * 0.5 + rng.uniform(-0.015, 0.01), SIGN_W * 0.5 + rng.uniform(-0.01, 0.015)
        town.board(m, (x0, 0.0, zc + rng.uniform(-0.003, 0.003)), (x1, 0.0, zc), SIGN_PLANK - 0.008, SIGN_T,
                   look='band_G', cuts=3)
    # Battens on the back under the hooks, the hooks' straps over the top and their eyes, and the chains.
    for x in (-A_CHAIN_X, A_CHAIN_X):
        town.board(m, (x, SIGN_T * 0.5 + 0.017, top - 0.004), (x, SIGN_T * 0.5 + 0.017, bottom + 0.03), 0.11, 0.034,
                   look='A', face=(0.0, 1.0, 0.0), drop=('+y',))
        for side in (-1.0, 1.0):
            iron(m, (0.05, 0.009, 0.15), (x, side * (SIGN_T * 0.5 + 0.0045 + (0.034 if side > 0 else 0.0)), top - 0.07))
        iron(m, (0.05, SIGN_T + 0.052, 0.009), (x, 0.017, top + 0.0045))
        wire_loop(m, (x, 0.0, top + 0.04), (0.0, 0.0, 1.0), (0.0, 1.0, 0.0), 0.03, 0.03, 0.009, seg=8, sides=4)
        chain(m, (x, 0.0, -0.03), (x, 0.0, top + 0.072), 3)
    m.section('board, hooks, chains')
    # RANSOM'S REST in black Rye on the front, REST EASY on the back for those leaving, a pinstripe round the front.
    mid = (top + bottom) * 0.5
    town.text(m, "RANSOM'S REST", (0.0, -SIGN_T * 0.5, mid + 0.005), height=0.46, look='black', tol=0.026)
    town.text(m, 'REST EASY', (0.0, -SIGN_T * 0.5, mid + 0.005), height=0.38, look='black', tol=0.026, space=BACK)
    inset = 0.075
    y = -SIGN_T * 0.5 - 0.0012
    for z in (top - inset, bottom + inset):
        town.box(m, (SIGN_W - 2.0 * inset + 0.018, 0.002, 0.018), at=(0.0, y, z), look='black')
    for x in (-SIGN_W * 0.5 + inset, SIGN_W * 0.5 - inset):
        town.box(m, (0.018, 0.002, top - bottom - 2.0 * inset - 0.018), at=(x, y, mid), look='black')
    m.section('lettering')
    # The gang's shots on their way out of town a week ago: a ragged grouping by the right end and a few strays,
    # each out through the back.
    holes = [(1.86, mid + 0.19), (2.02, mid + 0.06), (1.95, mid - 0.15), (2.17, mid + 0.24), (-0.66, mid - 0.27),
             (0.38, mid + 0.3), (-2.12, mid + 0.02)]
    for x, z in holes:
        x += rng.uniform(-0.03, 0.03)
        z += rng.uniform(-0.03, 0.03)
        bullet_hole(m, x, z, -SIGN_T * 0.5, rng)
        bullet_hole(m, x + rng.uniform(-0.01, 0.01), z + rng.uniform(-0.01, 0.01), SIGN_T * 0.5, rng, back=True)
    m.section('bullet holes')
    population(m, bottom)
    m.section('population plank')
    return m.finish(ground=False, preview=False, Collision='None', **SMALL)


def population(m, bottom):
    """A plank hung under the board on two S-hooks: POP. 212, the number struck out and 96 painted in beside it in another
    hand. Half the Rest left when the saint went dark, and the gang's raid took more."""
    rng = m.rng
    w, h = 1.32, 0.25
    zc = bottom - 0.062 - h * 0.5
    town.board(m, (-w * 0.5, 0.0, zc), (w * 0.5, 0.0, zc - 0.012), h, 0.038, look='band_G', cuts=0)
    for x in (-0.46, 0.46):
        wire_loop(m, (x, 0.0, bottom - 0.034), (0.0, 0.0, 1.0), (1.0, 0.0, 0.0), 0.036, 0.016, 0.006, seg=6, sides=3)
    tilt = -math.degrees(math.atan2(0.012, w))
    town.text(m, 'POP.', (-0.37, -0.019, zc - 0.002), height=0.135, look='black', tol=0.03, rot=tilt)
    town.text(m, '212', (0.02, -0.019, zc - 0.006), height=0.135, look='black', tol=0.03, rot=tilt)
    town.box(m, (0.36, 0.002, 0.02), at=(0.02, -0.0215, zc - 0.004), rot=(0.0, -7.0, 0.0), look='black')
    town.text(m, '96', (0.42, -0.019, zc + 0.016), height=0.12, look='black', tol=0.04, rot=6.0)
    for x in (-0.6, 0.6):
        iron(m, (0.02, 0.01, 0.02), (x, -0.024, zc + rng.uniform(-0.05, 0.05)))


# --- Options B and C: the timber gate, and the same gate in mourning ---

B_TOP = 4.3                    # the posts' tops (their caps over them)
B_POST = 0.32
B_PIER = 0.5                   # the fieldstone piers' tops
B_BEAM_Z = 3.7                 # the tie beam's middle
B_BEAM = (0.3, 0.26)           # tall, deep
B_BEAM_HALF = 6.15
B_BOARD_HALF = 4.25
B_BOARD_TOP = 3.53
B_BOARD_Y = -0.0675            # the board's middle plane: its frame's face flush with the beam's front


def gate_b(name, seed, mourning=False):
    """The timber gate (TownGate, option B): squared hewn posts on fieldstone piers, a tie beam under a narrow shake
    roof, knee braces, and the long oxide-red board hung under the beam. mourning makes option C: the same gate (the
    same seed draws the same gate) with wreaths in place of the board's stars, crepe swagged along both eaves, crepe
    bands and bows on the posts and a ribbon on the lantern's arm."""
    m = town.Model(name, seed=seed)
    rng = m.rng
    for cx in (NORTH_X, SOUTH_X):
        kit.stone_stack(m, (cx, 0.0, -0.15), (1.1, 1.1), B_PIER + 0.15, taper=0.12, bevel=0.03)
        timber(m, (cx, 0.0, B_PIER - 0.03), (cx, 0.0, B_TOP), B_POST, B_POST, bevel=0.02)
        m.emit(kit._box((0.42, 0.42, 0.06), 0.012, 0, ()), Lane(), 'trim', place((cx, 0.0, B_TOP + 0.03)))
        # Iron straps holding the post's foot to the pier, front and back.
        for side in (-1.0, 1.0):
            y = side * (B_POST * 0.5 + 0.006)
            iron(m, (0.075, 0.012, 0.5), (cx, y, B_PIER + 0.18), uv=DullIron())
            for z in (B_PIER + 0.08, B_PIER + 0.33):
                iron(m, (0.04, 0.03, 0.04), (cx, y + side * 0.01, z))
    m.section('piers and posts')
    # The tie beam let into the posts, its sawn ends past them, bolted through.
    timber(m, (-B_BEAM_HALF, 0.0, B_BEAM_Z), (B_BEAM_HALF, 0.0, B_BEAM_Z), B_BEAM[0], B_BEAM[1], drop=('-x', '+x'),
           cuts=8)
    for cx in (NORTH_X, SOUTH_X):
        for side in (-1.0, 1.0):
            y = side * (B_BEAM[1] * 0.5 + 0.006)
            iron(m, (0.12, 0.012, 0.12), (cx, y, B_BEAM_Z), rot=(0.0, 45.0, 0.0))
            iron(m, (0.055, 0.035, 0.055), (cx, y + side * 0.018, B_BEAM_Z))
    # Knee braces from the posts up to the beam, outside the board's ends.
    for sx in (-1.0, 1.0):
        p0 = Vector((sx * (POST_X - B_POST * 0.5 + 0.06), 0.0, 2.6))
        p1 = Vector((sx * 4.63, 0.0, B_BEAM_Z - B_BEAM[0] * 0.5 + 0.06))
        timber(m, p0, p1, 0.15, 0.15, bevel=0.012, cuts=0)
        for p in (p0.lerp(p1, 0.12), p0.lerp(p1, 0.88)):
            for side in (-1.0, 1.0):
                iron(m, (0.045, 0.03, 0.045), (p.x, side * 0.085, p.z))
    m.section('beam and braces')
    # A narrow shake roof along the beam, the posts standing up through it.
    eave = B_BEAM_Z + B_BEAM[0] * 0.5
    front, back = town.gable(m, -B_BEAM_HALF + 0.08, B_BEAM_HALF - 0.08, -B_BEAM[1] * 0.5, B_BEAM[1] * 0.5, eave,
                             38.0, overhang=0.2, rake=0.05, deck=0.03)
    for slope in (front, back):
        town.shakes(m, slope, piece=(2.2, 3.6))
    town.ridge_cap(m, front, back, uv='C', width=0.1, thick=0.035)
    rise = B_BEAM[1] * 0.5 * math.tan(math.radians(38.0))
    for sx in (-1.0, 1.0):
        x = sx * (B_BEAM_HALF - 0.09)
        p0, p1 = ((x, B_BEAM[1] * 0.5), (x, -B_BEAM[1] * 0.5)) if sx < 0 else ((x, -B_BEAM[1] * 0.5), (x, B_BEAM[1] * 0.5))
        town.prism(m, [(0.0, 0.0), (B_BEAM[1], 0.0), (B_BEAM[1] * 0.5, rise)], 0.02, look='C',
                   space=kit.wall_space(p0, p1, eave))
    m.section('roof')
    # The board: five oxide-red boards in a frame of hewn trim, hung under the beam's front on iron straps.
    yb = B_BOARD_Y
    face = yb - 0.0175
    for k in range(5):
        zc = B_BOARD_TOP - 0.2 * (k + 0.5)
        town.board(m, (-B_BOARD_HALF - 0.07, yb, zc), (B_BOARD_HALF + 0.07, yb, zc + rng.uniform(-0.003, 0.003)), 0.194,
                   0.035, look='H2')
    bottom = B_BOARD_TOP - 1.0
    for y, f in ((face - 0.0225, (0.0, -1.0, 0.0)), (yb + 0.0175 + 0.0175, (0.0, 1.0, 0.0))):
        thick = 0.045 if f[1] < 0 else 0.035
        for z in (B_BOARD_TOP - 0.03, bottom + 0.0):
            town.board(m, (-B_BOARD_HALF - 0.15, y, z), (B_BOARD_HALF + 0.15, y, z), 0.1, thick, look='C', face=f)
        for x in (-B_BOARD_HALF - 0.05, B_BOARD_HALF + 0.05):
            town.board(m, (x, y, bottom + 0.05), (x, y, B_BOARD_TOP - 0.08), 0.1, thick, look='C', face=f)
    for x in (-3.4, -1.6, 1.6, 3.4):
        iron(m, (0.065, 0.011, 0.36), (x, -B_BEAM[1] * 0.5 - 0.0055, 3.64))
        for z in (B_BEAM_Z + 0.06, B_BOARD_TOP - 0.03):
            iron(m, (0.035, 0.022, 0.035), (x, -B_BEAM[1] * 0.5 - 0.016, z))
    mid = B_BOARD_TOP - 0.5
    town.text(m, "RANSOM'S REST", (0.0, face, mid + 0.01), height=0.54, look='cream', tol=0.026)
    town.text(m, 'REST EASY', (0.0, -(yb + 0.0175), mid + 0.01), height=0.44, look='cream', tol=0.026, space=BACK)
    m.section('board and lettering')
    holes = [(-2.35, mid + 0.18), (-2.2, mid - 0.06), (-2.48, mid + 0.02), (0.9, mid + 0.31), (2.75, mid - 0.28),
             (3.1, mid + 0.25), (-0.4, mid - 0.33)]
    for x, z in holes:
        x += rng.uniform(-0.03, 0.03)
        z += rng.uniform(-0.03, 0.03)
        bullet_hole(m, x, z, face, rng)
        bullet_hole(m, x + rng.uniform(-0.01, 0.01), z + rng.uniform(-0.01, 0.01), yb + 0.0175, rng, back=True)
    m.section('bullet holes')
    # The lantern on its iron arm on the south post, kept lit for those not yet home.
    arm_z = 2.62
    y0 = -B_POST * 0.5
    iron(m, (0.09, 0.012, 0.22), (SOUTH_X, y0 - 0.006, arm_z - 0.08))
    iron(m, (0.03, 0.5, 0.03), (SOUTH_X, y0 - 0.25, arm_z))
    stay = Vector((SOUTH_X, y0, arm_z - 0.3)), Vector((SOUTH_X, y0 - 0.36, arm_z))
    length = (stay[1] - stay[0]).length
    m.emit(kit._box((length, 0.022, 0.022), 0.0, 0, ()), IRON, 'trim', kit.toward(*stay, (1.0, 0.0, 0.0)) @
           Matrix.Translation((length * 0.5, 0.0, 0.0)))
    hook_y = y0 - 0.46
    wire_loop(m, (SOUTH_X, hook_y, arm_z - 0.05), (0.0, 0.0, 1.0), (0.0, 1.0, 0.0), 0.035, 0.035, 0.008, seg=8, sides=4)
    lantern(m, (SOUTH_X, hook_y, arm_z - 0.37))
    m.section('lantern')
    # The north post: the town's population on a little board at eye level.
    population_board(m, NORTH_X, y0, 1.74)
    m.section('population board')
    # What B and C don't share comes last, so the same seed draws the same gate for both. The beam's sawn ends fill
    # the chamfered box's open ends: end grain on B; on C, which needs its slots for the crepe and the wreaths, a patch
    # of the beam's own strip.
    hd, hw = (B_BEAM[1] - 0.036) * 0.5, (B_BEAM[0] - 0.036) * 0.5
    for sx in (-1.0, 1.0):
        spec, key = (kit.EndGrain(), 'endgrain') if not mourning else (Spot('C', 0.205, 1.0 + sx, fit=0.19), 'trim')
        m.emit(flat([(-hd, -hw), (hd, -hw), (hd, hw), (-hd, hw)]), spec, key,
               Matrix.Translation((sx * B_BEAM_HALF, 0.0, B_BEAM_Z)) @ Matrix.Rotation(sx * math.pi * 0.5, 4, 'Z'))
    if mourning:
        mourning_b(m, mid, face, arm_z, y0)
        m.section('mourning')
    else:
        for sx in (-1.0, 1.0):
            star = []
            for k in range(10):
                r = 0.17 if k % 2 == 0 else 0.068
                a = math.pi * 0.5 + math.pi * k / 5
                star.append((sx * 3.62 + r * math.cos(a), mid + 0.01 + r * math.sin(a)))
            town.prism(m, star, 0.004, look='cream', y=face - 0.004)
    m.socket('Decal', (SOUTH_X, y0 - 0.001, 1.55))
    perch(m, Vector((NORTH_X, 0.0, B_TOP + 0.06)), 0.14)
    for cx in (NORTH_X, SOUTH_X):
        m.hull((1.12, 1.12, B_PIER + 0.15), at=(cx, 0.0, (B_PIER - 0.15) * 0.5))
        m.hull((B_POST + 0.02, B_POST + 0.02, B_TOP + 0.06 - B_PIER), at=(cx, 0.0, (B_TOP + 0.06 + B_PIER) * 0.5))
    return m


def lantern(m, at):
    """A small iron lantern with lit glass, its middle at at, hanging by its bail. Adds SOCKET_Light at the glass."""
    x, y, z = at
    m.box((0.2, 0.2, 0.035), at=(x, y, z + 0.145), uv=IRON, bevel=0.008)
    m.cylinder((x, y, z + 0.162), (x, y, z + 0.25), 0.11, 0.028, sides=8, uv=IRON, caps=(False, True))
    m.box((0.15, 0.15, 0.24), at=(x, y, z), uv=Trim('H4', fit=True), mat='glow')
    m.box((0.2, 0.2, 0.035), at=(x, y, z - 0.135), uv=IRON, bevel=0.008)
    for dx in (-0.085, 0.085):
        for dy in (-0.085, 0.085):
            m.box((0.022, 0.022, 0.26), at=(x + dx, y + dy, z), uv=IRON)
    wire_loop(m, (x, y, z + 0.29), (0.0, 0.0, 1.0), (1.0, 0.0, 0.0), 0.045, 0.03, 0.007, seg=6, sides=3)
    m.socket('Light', (x, y, z))


def population_board(m, cx, y_face, zc):
    """A little board nailed to a post's front, in the gate board's paint: POPULATION 212, the number struck out and 96
    painted in beside it in another hand. Half the Rest left when the saint went dark, and the gang's raid took more."""
    rng = m.rng
    w, t = 0.64, 0.03
    y = y_face - t * 0.5
    for k, dz in enumerate((0.086, -0.086)):
        town.board(m, (cx - w * 0.5 + rng.uniform(-0.01, 0.01), y, zc + dz), (cx + w * 0.5 + rng.uniform(-0.01, 0.01), y,
                   zc + dz + rng.uniform(-0.004, 0.004)), 0.168, t, look='H2', cuts=0)
        for sx in (-1.0, 1.0):
            iron(m, (0.02, 0.012, 0.02), (cx + sx * (w * 0.5 - 0.05), y - t * 0.5 - 0.004, zc + dz))
    face = y - t * 0.5
    town.text(m, 'POPULATION', (cx, face, zc + 0.088), height=0.056, look='cream', tol=0.035)
    town.text(m, '212', (cx - 0.12, face, zc - 0.078), height=0.112, look='cream', tol=0.03)
    town.box(m, (0.29, 0.002, 0.018), at=(cx - 0.12, face - 0.0025, zc - 0.076), rot=(0.0, -8.0, 0.0), look='cream')
    town.text(m, '96', (cx + 0.165, face, zc - 0.07), height=0.1, look='cream', tol=0.04, rot=5.0)


def mourning_b(m, mid, face, arm_z, y0):
    """Option C's mourning: wreaths in place of the board's stars, black crepe swagged along the beam's front over the
    board, rosettes at the pins and ribbon tails by the posts, a crepe band and bow round each post, a ribbon tied on
    the lantern's arm."""
    rng = m.rng
    for sx in (-1.0, 1.0):
        wreath(m, sx * 3.62, mid + 0.02, face, R=0.235, sprigs=36)
    # The bunting hangs from both eaves' edges, out in the light: pinned under the shakes' butts along the roof deck's
    # edge (0.35 m out from the beam's middle), clear of the lettering. From either side the board and the beam hide
    # the other side's swags, so each is one layer.
    y = -0.352
    z = 3.69
    pins = [-5.48, -2.74, 0.0, 2.74, 5.48]
    for space, cols in ((None, 10), (BACK, 8)):
        for x0, x1 in zip(pins, pins[1:]):
            crepe_swag(m, x0 + 0.05, x1 - 0.05, lambda x: z, y, 0.24, rng, cols=cols, layers=1, space=space)
        for x in pins:
            rosette(m, (x, y - 0.018, z - 0.02), space=space)
    for sx, x in ((-1.0, pins[0]), (1.0, pins[-1])):
        for k, (dx, length) in enumerate(((-0.025, 0.62), (0.03, 0.74))):
            ribbon(m, (x + dx, y - 0.03, z - 0.05), length, sx * rng.uniform(2.0, 6.0) * (1 if k else -1), rng)
    for cx in (NORTH_X, SOUTH_X):
        bz = 3.12
        town.box(m, (B_POST + 0.022, B_POST + 0.022, 0.16), at=(cx, 0.0, bz), look='crepe', drop=('-z', '+z'))
        town.crepe_bow(m, None, (cx, 0.0, bz), scale=1.0, tails=0.4, y=-(B_POST * 0.5 + 0.011) - 0.004)
    # A ribbon tied round the lantern's arm, its two ends hanging.
    ty = y0 - 0.2
    town.box(m, (0.046, 0.06, 0.046), at=(SOUTH_X, ty, arm_z), look='crepe')
    for k, lean in enumerate((-7.0, 9.0)):
        ribbon(m, (SOUTH_X + (k - 0.5) * 0.03, ty - 0.032, arm_z - 0.02), 0.26 + 0.05 * k, lean, rng, width=0.04)


# --- The models ---

def finish(m, mourning=False):
    """The export settings: a gate is a large prop of 3-5k triangles, so Nanite with its whole mesh as the fallback
    (what Medium draws); one under 2k triangles would be Nanite 0 with LODs."""
    small = m.triangles() <= 2000
    obj = m.finish(preview=False, **SMALL) if small else m.finish(preview=False, fallback=100.0)
    if mourning:
        # The wreaths are on the foliage master: no wind for them (R = 0) and a variation per sprig (G); the baked
        # occlusion in A stays.
        lt.set_foliage_colors(obj, wind=0.0, variation='island', seed=7)
    return obj


# The user picked option B on 2026-10-07: it is the game's gate, TownGate. A, its sign and C are still built for the
# previews, to compare, but never exported; each has its own seed, so leaving them out changes nothing else.
PREVIEW = lt.want_preview()
gate_A = finish(gate_a('TownGate_A', 1101)) if PREVIEW else None
sign_A = sign_a('TownGate_A_Sign', 1102) if PREVIEW else None
gate = finish(gate_b('TownGate', 1201))
gate_C = finish(gate_b('TownGate_C', 1201, mourning=True), mourning=True) if PREVIEW else None
MODELS = [obj for obj in (gate_A, sign_A, gate, gate_C) if obj is not None]
GATES = [obj for obj in (gate_A, gate, gate_C) if obj is not None]
for obj in MODELS:
    tris = sum(len(p.vertices) - 2 for p in obj.data.polygons)
    print(f'TOWNGATE: {obj.name}: {tris} triangles, {len([c for c in obj.children if c.name.startswith("UCX_")])} hulls, '
          f'sockets {", ".join(c.name for c in obj.children if c.name.startswith("SOCKET_"))}, '
          f'materials {", ".join(mt.name for mt in obj.data.materials)}, props '
          f'{ {k: obj[k] for k in obj.keys() if k in ("Nanite", "LODs", "Fallback", "Collision")} }', flush=True)


# --- The look sheet (--preview) ---

PREVIEW_DIR = os.path.join(lt.PREVIEW_DIR, 'RansomsRest', 'TownGate')
GATE_YAW = -90.0               # the level turns the gate so its front (Unreal +X) faces west


def bearing(azimuth, elevation, yaw=GATE_YAW):
    """Toward a sun at a compass bearing and elevation, in the gate's own frame as the level turns it (Farmhouse.py's
    look sheet: Unreal's +X is north and +Y east, and the model's front, Blender's -Y, is its +X)."""
    a, e, t = math.radians(azimuth), math.radians(elevation), math.radians(yaw)
    north, east = math.cos(a) * math.cos(e), math.sin(a) * math.cos(e)
    x, y = north * math.cos(t) + east * math.sin(t), -north * math.sin(t) + east * math.cos(t)
    return (-y, -x, math.sin(e))


# The level's light, as the farmhouse's look sheet paints it: the golden late afternoon's sun at 247.5 degrees
# (west-south-west), 15 degrees up, in front of the gate and a little to its south.
GOLDEN_SUN = bearing(247.5, 15.0)
GOLDEN_SKY = [(0.0, 0x6b6050), (0.47, 0x8e8270), (0.5, 0xe9d2a6), (0.53, 0xd9d6c4), (0.62, 0xa9bfd2), (1.0, 0x5d84b6)]
GOLDEN_GLOWS = [(0xffe0a8, 24.0, 0.9), (0xfff2d0, 600.0, 6.0)]


def sky(name, stops, sun, glows):
    """A painted sky (Farmhouse.py's): colours by the view's height, glows round the sun."""
    world = bpy.data.worlds.get(name) or bpy.data.worlds.new(name)
    world.use_nodes = True
    nodes, links = world.node_tree.nodes, world.node_tree.links
    nodes.clear()
    out = nodes.new('ShaderNodeOutputWorld')
    background = nodes.new('ShaderNodeBackground')
    coords = nodes.new('ShaderNodeTexCoord')
    split = nodes.new('ShaderNodeSeparateXYZ')
    links.new(coords.outputs['Generated'], split.inputs['Vector'])
    remap = nodes.new('ShaderNodeMapRange')
    remap.inputs['From Min'].default_value = -1.0
    links.new(split.outputs['Z'], remap.inputs['Value'])
    ramp = nodes.new('ShaderNodeValToRGB')
    links.new(remap.outputs['Result'], ramp.inputs['Fac'])
    elements = ramp.color_ramp.elements
    elements[0].position, elements[0].color = stops[0][0], lt.hex_color(stops[0][1])
    elements[1].position, elements[1].color = stops[-1][0], lt.hex_color(stops[-1][1])
    for position, color in stops[1:-1]:
        elements.new(position).color = lt.hex_color(color)
    color = ramp.outputs['Color']
    dot = nodes.new('ShaderNodeVectorMath')
    dot.operation = 'DOT_PRODUCT'
    links.new(coords.outputs['Generated'], dot.inputs[0])
    dot.inputs[1].default_value = Vector(sun).normalized()
    clamp = nodes.new('ShaderNodeMath')
    clamp.operation = 'MAXIMUM'
    links.new(dot.outputs['Value'], clamp.inputs[0])
    for glow, power, strength in glows:
        lift = nodes.new('ShaderNodeMath')
        lift.operation = 'POWER'
        links.new(clamp.outputs['Value'], lift.inputs[0])
        lift.inputs[1].default_value = power
        scale = nodes.new('ShaderNodeMath')
        scale.operation = 'MULTIPLY'
        links.new(lift.outputs['Value'], scale.inputs[0])
        scale.inputs[1].default_value = strength
        add = nodes.new('ShaderNodeMix')
        add.data_type = 'RGBA'
        add.blend_type = 'ADD'
        links.new(scale.outputs['Value'], add.inputs['Factor'])
        links.new(color, add.inputs['A'])
        add.inputs['B'].default_value = lt.hex_color(glow)
        color = add.outputs['Result']
    links.new(color, background.inputs['Color'])
    links.new(background.outputs['Background'], out.inputs['Surface'])
    return world


def stage():
    """The renders' set (Farmhouse.py's): Eevee with ray-traced screen-space light and AgX, the street's dirt, the hulls
    hidden, glowing glass glowing, the golden sun and sky."""
    scene = bpy.context.scene
    scene.render.engine = 'BLENDER_EEVEE_NEXT'
    ee = scene.eevee
    ee.taa_render_samples = 80
    ee.use_shadows = True
    if hasattr(ee, 'use_raytracing'):
        ee.use_raytracing = True
        ee.ray_tracing_method = 'SCREEN'
    if hasattr(ee, 'use_fast_gi'):
        ee.use_fast_gi = True
    scene.render.film_transparent = False
    scene.render.image_settings.file_format = 'PNG'
    scene.render.image_settings.color_mode = 'RGB'
    scene.view_settings.view_transform = 'AgX'
    scene.view_settings.look = 'AgX - Medium High Contrast'
    for o in scene.objects:
        if o.name.startswith('UCX_'):
            o.hide_render = True
    for mat in bpy.data.materials:
        if mat.get('Glow') and mat.use_nodes:
            bsdf = next(n for n in mat.node_tree.nodes if n.type == 'BSDF_PRINCIPLED')
            bsdf.inputs['Emission Color'].default_value = bsdf.inputs['Base Color'].default_value
            bsdf.inputs['Emission Strength'].default_value = float(mat['Glow'])
    mesh = bpy.data.meshes.new('_Ground')
    mesh.from_pydata([(-120.0, -120.0, 0.0), (120.0, -120.0, 0.0), (120.0, 120.0, 0.0), (-120.0, 120.0, 0.0)], [],
                     [(0, 1, 2, 3)])
    mesh.uv_layers.new(name='UVMap')
    ground = bpy.data.objects.new('_Ground', mesh)
    scene.collection.objects.link(ground)
    lt.assign(ground, lt.material('GroundDirt'))
    lt.box_uv(ground, 'GroundDirt')
    col = mesh.color_attributes.new('Col', 'BYTE_COLOR', 'CORNER')
    col.data.foreach_set('color', [1.0] * (4 * len(mesh.loops)))
    sun = bpy.data.objects.new('_Sun', bpy.data.lights.new('_Sun', 'SUN'))
    scene.collection.objects.link(sun)
    sun.data.energy, sun.data.color, sun.data.angle = 4.4, (1.0, 0.8, 0.56), math.radians(2.0)
    sun.rotation_euler = (-Vector(GOLDEN_SUN).normalized()).to_track_quat('-Z', 'Y').to_euler()
    scene.world = sky('_Golden', GOLDEN_SKY, GOLDEN_SUN, GOLDEN_GLOWS)
    return scene


def shoot(scene, path, eye, target, lens, size, exposure=0.0):
    cam = bpy.data.objects.get('_Camera') or bpy.data.objects.new('_Camera', bpy.data.cameras.new('_Camera'))
    if cam.name not in scene.collection.objects:
        scene.collection.objects.link(cam)
    cam.location = Vector(eye)
    cam.rotation_euler = (Vector(target) - Vector(eye)).to_track_quat('-Z', 'Y').to_euler()
    cam.data.lens, cam.data.clip_start, cam.data.clip_end = lens, 0.02, 2000.0
    scene.camera = cam
    scene.render.resolution_x, scene.render.resolution_y = size
    scene.render.resolution_percentage = 100
    scene.view_settings.exposure = exposure
    scene.render.filepath = path
    bpy.ops.render.render(write_still=True)
    lt._log(f'towngate: rendered {path}')
    return path


def stand_in():
    """A 1.8 m figure for scale: a plain clay mannequin standing at the origin, facing -Y."""
    bm = bmesh.new()

    def limb(p0, p1, r0, r1, segments=10):
        p0, p1 = Vector(p0), Vector(p1)
        d = p1 - p0
        rot = Vector((0.0, 0.0, 1.0)).rotation_difference(d).to_matrix().to_4x4()
        bmesh.ops.create_cone(bm, cap_ends=True, segments=segments, radius1=r0, radius2=r1, depth=d.length,
                              matrix=Matrix.Translation((p0 + p1) * 0.5) @ rot)

    def blob(c, r, scale=(1.0, 1.0, 1.0)):
        bmesh.ops.create_uvsphere(bm, u_segments=12, v_segments=8, radius=r,
                                  matrix=Matrix.Translation(c) @ Matrix.Diagonal((*scale, 1.0)))
    for sx in (-1.0, 1.0):
        blob((sx * 0.1, -0.05, 0.045), 0.06, (0.95, 2.1, 0.75))
        limb((sx * 0.1, 0.0, 0.06), (sx * 0.1, 0.0, 0.5), 0.05, 0.062)
        limb((sx * 0.1, 0.0, 0.5), (sx * 0.1, 0.0, 0.93), 0.064, 0.088)
        blob((sx * 0.2, 0.0, 1.42), 0.068)
        limb((sx * 0.21, 0.0, 1.42), (sx * 0.24, 0.012, 1.12), 0.05, 0.044)
        limb((sx * 0.24, 0.012, 1.12), (sx * 0.255, -0.02, 0.86), 0.042, 0.034)
        blob((sx * 0.258, -0.022, 0.81), 0.045, (0.8, 0.9, 1.2))
    blob((0.0, 0.0, 0.96), 0.17, (1.12, 0.72, 0.62))
    limb((0.0, 0.0, 0.96), (0.0, 0.0, 1.45), 0.15, 0.19, segments=12)
    limb((0.0, 0.0, 1.46), (0.0, 0.0, 1.58), 0.055, 0.05)
    blob((0.0, -0.01, 1.68), 0.11, (0.9, 1.0, 1.09))
    bmesh.ops.scale(bm, vec=(1.0, 0.68, 1.0), verts=[v for v in bm.verts if 0.94 < v.co.z < 1.47 and abs(v.co.x) < 0.2])
    mesh = bpy.data.meshes.new('_Human')
    bm.to_mesh(mesh)
    bm.free()
    for poly in mesh.polygons:
        poly.use_smooth = True
    obj = bpy.data.objects.new('_Human', mesh)
    bpy.context.scene.collection.objects.link(obj)
    mat = bpy.data.materials.get('_Clay') or bpy.data.materials.new('_Clay')
    mat.use_nodes = True
    bsdf = next(n for n in mat.node_tree.nodes if n.type == 'BSDF_PRINCIPLED')
    bsdf.inputs['Base Color'].default_value = lt.hex_color(0x8a847a)
    bsdf.inputs['Roughness'].default_value = 0.85
    mesh.materials.append(mat)
    return obj


def crow():
    """Hob's game model (Art/Models/Creatures/Hob.py), built here without its own previews: the rig, its pose()."""
    saved = sys.argv[:]
    sys.argv = [sys.argv[0]]
    try:
        hob = runpy.run_path(os.path.join(lt.REPO, 'Art', 'Models', 'Creatures', 'Hob.py'), run_name='hob_on_gate')
    finally:
        sys.argv = saved
    hob['game_shading']()
    return hob


def socket_of(obj, name):
    bpy.context.view_layer.update()
    return next(c for c in obj.children if c.name.split('.')[0] == 'SOCKET_' + name)


def mount(model, socket, turn=0.0):
    """A copy of a model hung on a socket (turn swings it about the socket's X, degrees)."""
    copy = model.copy()
    bpy.context.scene.collection.objects.link(copy)
    copy.matrix_world = socket.matrix_world @ Matrix.Rotation(math.radians(turn), 4, 'X')
    return copy


def combine(paths, labels, out, gap=12):
    """The renders in a row, labelled in Rye (looter_posters' labels)."""
    import numpy as np
    import looter_posters as posters
    panels = []
    for path in paths:
        img = bpy.data.images.load(path, check_existing=False)
        img.colorspace_settings.name = 'Non-Color'
        w, h = img.size
        px = np.empty(w * h * 4, np.float32)
        img.pixels.foreach_get(px)
        bpy.data.images.remove(img)
        panels.append(px.reshape(h, w, 4)[::-1, :, :3].copy())
    sep = np.full((panels[0].shape[0], gap, 3), 0.08, np.float32)
    row = np.concatenate([part for panel in panels for part in (panel, sep)][:-1], axis=1)
    x = 0
    for panel, label in zip(panels, labels):
        posters.sheet_label(row, label, x + 34, 78, 46, color=(0.95, 0.65, 0.29))
        x += panel.shape[1] + gap
    lt.write_png(out, lt.to8(np.clip(row, 0.0, 1.0)))
    lt._log(f'towngate: {out}')


def previews():
    """The look sheet in Saved/ArtPreviews/RansomsRest/TownGate/, in the level's golden late afternoon:
      TownGate                the game's gate (option B) from the front three-quarter by the north post, a 1.8 m figure
                              beside it, Hob on his perch looking down the farm road
      TownGate_Lettering      its board from the street, a standing player's eye
      TownGate_Back           from the town side, down Main Street
      TownGate_Perch          Hob on the north post's cap, close
      TownGate_A, TownGate_C  and their _Lettering and _Back: the options not picked, the same views, for reference
      TownGate_options        the three side by side, as the user picked from them
    and the overview, Saved/ArtPreviews/RansomsRest/TownGate_overview.png: the three gates and A's sign."""
    os.makedirs(PREVIEW_DIR, exist_ok=True)
    hob = crow()
    rig = hob['RIG']
    scene = stage()
    human = stand_in()
    sign = mount(sign_A, socket_of(gate_A, 'Sign'))
    sign_A.hide_render = True
    views = []

    def show(shown):
        for obj in GATES:
            obj.hide_render = obj is not shown
        sign.hide_render = shown is not gate_A
        rig.matrix_world = socket_of(shown, 'Perch').matrix_world
        hob['pose'](dict(head=(-35.0, 6.0, -8.0)))
    options = ((gate_A, 'TownGate_A'), (gate, 'TownGate'), (gate_C, 'TownGate_C'))
    for g, name in options:
        show(g)
        human.location, human.rotation_euler = (-3.7, -1.6, 0.0), (0.0, 0.0, math.radians(-30.0))
        path = os.path.join(PREVIEW_DIR, f'{name}.png')
        shoot(scene, path, (-10.6, -12.4, 2.25), (-0.6, 0.0, 2.5), 38.0, (1400, 1050), exposure=-0.15)
        views.append(path)
        human.location = (40.0, 0.0, 0.0)
        if g is gate_A:
            eye, target, lens = (0.8, -7.4, 1.7), (0.0, 0.0, A_HANG_Z + SIGN_TOP - 1.5 * SIGN_PLANK - 0.12), 38.0
        else:
            eye, target, lens = (1.0, -9.6, 1.7), (0.0, 0.0, B_BOARD_TOP - 0.5), 32.0
        shoot(scene, os.path.join(PREVIEW_DIR, f'{name}_Lettering.png'), eye, target, lens, (1600, 900), exposure=-0.15)
        shoot(scene, os.path.join(PREVIEW_DIR, f'{name}_Back.png'), (8.5, 11.5, 2.0), (0.4, 0.0, 2.6), 36.0, (1400, 1050),
              exposure=0.35)
    combine(views, ['A  LOG GATE', 'B  TIMBER GATE  (PICKED)', 'C  IN MOURNING'],
            os.path.join(PREVIEW_DIR, 'TownGate_options.png'))
    # Hob on the north post's cap, close, from the front three-quarter (a long lens from a ladder's height).
    show(gate)
    hob['pose'](dict(head=(-20.0, 4.0, -6.0)))
    top = socket_of(gate, 'Perch').matrix_world.translation
    path = os.path.join(PREVIEW_DIR, '_perch.png')
    shoot(scene, path, (top.x - 1.0, top.y - 2.6, top.z + 0.25), (top.x, top.y, top.z - 0.02), 85.0, (1000, 900),
          exposure=-0.1)
    combine([path], ['HOB ON SOCKET_PERCH'], os.path.join(PREVIEW_DIR, 'TownGate_Perch.png'))
    os.remove(path)
    # The renders of option B under its old name, now TownGate's.
    for stale in ('TownGate_B.png', 'TownGate_B_Lettering.png', 'TownGate_B_Back.png'):
        if os.path.exists(os.path.join(PREVIEW_DIR, stale)):
            os.remove(os.path.join(PREVIEW_DIR, stale))
    # The three side by side in the neutral preview light: the gates in a row, A's sign standing on its own in front.
    rig.location = (0.0, 0.0, -50.0)
    human.hide_render = True
    copies = []
    for k, g in enumerate(GATES):
        copy = g.copy()
        scene.collection.objects.link(copy)
        copy.location = ((k - 1) * 15.5, 0.0, 0.0)
        copy.hide_render = False
        copies.append(copy)
    sign_copy = sign_A.copy()
    scene.collection.objects.link(sign_copy)
    sign_copy.location = (-15.5, -7.0, -min(Vector(c).z for c in sign_A.bound_box))
    sign_copy.hide_render = False
    hung = sign.copy()
    scene.collection.objects.link(hung)
    hung.matrix_world = Matrix.Translation((-15.5, 0.0, 0.0)) @ sign.matrix_world
    hung.hide_render = False
    bpy.context.view_layer.update()
    shown = copies + [sign_copy, hung]
    lt.preview(shown, lt.preview_path('RansomsRest', 'TownGate_overview'), view=(-0.25, -1.6, 0.42), fit=0.5,
               resolution=(1900, 820), ground_at='origin')
    for obj in shown:
        bpy.data.objects.remove(obj)


if lt.want_preview():
    previews()
