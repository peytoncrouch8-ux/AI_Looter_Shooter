"""Yard and backlot dressing for Ransom's Rest (Docs/Areas/RansomsRest.md): the pieces the level asked for to dress Main
Street's backlots, the undertaker's yard and the Sink's broken rim fence. A scripted model file (Art/README.md) in the
frontier kit's look (FalseFronts.py, Ruins.py, Fences.py, Graves.py): the house trim sheet and the brief's shared
material names, put together with looter_buildings parts (boards, beams, roofs) and looter_props parts (sweeps, lathes,
stones) in one looter_ruins Model each. Every pivot is on the ground at the middle of the footprint and the front faces
-Y, unless a model says otherwise.

  Woodshed      the saloon's and the store's backlots: an open-fronted woodshed (2.8 x 1.5 m between its posts, 2.25 m
                high at the front, 1.85 m at the back) on six hewn posts set on flat fieldstones, a shake roof falling to
                the back, board-and-batten back, side boards with air gaps, split firewood stacked inside (full on the
                left, burnt down on the right) and a chopping block with an axe in it in front of its right post. The
                pivot is the middle of the posts' footprint; the block stands 1.2 m in front of it.
  LeanTo        a lean-to (3.0 x 1.9 m) on two posts against a wall: its back edge (the ledger board) meets the wall at
                y = +0.95 m (the pivot's +Y), so set it with the wall's face 0.95 m behind the pivot, turned to face away
                from the building. It stands 2.86 m high at the wall (the flashing board's top) and 2.34 m at its front
                beam (2.18 m clear under it), so it fits under every false front's back wall (eaves at 3.68 m on the
                undertaker's and the sheriff's, 4.18 m on the store, 6.38 m on the saloon), beside their back doors
                (2.58 m to the head). A board-and-batten roof (the trim's tin reads blue over this much roof), two
                barrels (a broom handle in the open one), two crates, a shovel and a pick against the wall and a coil
                of rope on a post.
  LumberStack   the undertaker's yard: sawn boards (3.6 m) stacked in ten stickered layers on three bearers, coffin-length
                planks (2 m) laid across the top and three more leaning against its front (3.8 x 1.6 m, 1.05 m high;
                the leaning planks reach 0.6 m in front of it).
  Dray          the undertaker's dray: a sound, four-wheeled flat dray in Bright & Daughter's black and brass (the hearse
                car's WoodBlack, IronBlack and BrassWorn), its bed (1.3 x 2.9 m, the floor 0.95 m up) worn bare where
                the coffins slide, BRIGHT & DAUGHTER in brass along both side boards, a driver's bench with two brass
                carriage lamps, low wheels on leaf springs, a brake, and its shafts down on the ground in front. 1.77 x
                5.43 m with the shafts; the pivot is the middle of that footprint, so the bed's middle is 1.22 m behind
                it (+Y) and the shaft tips 2.72 m in front. SOCKET_Light_1/_2 at the lamps' glass (unlit by day: the
                trim's dark glass), SOCKET_Sit on the bench, facing forward.
  FallenPine    a pine snapped off at its stump and fallen: 7 m of trunk lying on the ground with broken branch stubs
                and three heavier snapped limbs, its butt splintered beside the stump's splintered top (the trunk lies
                along +X, the stump at its -X end; 7.9 x 1.5 m; the trunk 0.56 m high at its propped butt, under
                0.45 m, a step, from 0.7 m along, 0.15 m at its top). Down for years, its bark has weathered grey: the
                dead tree's BarkOak (DeadTree.py; the living pines' BarkPine reads freshly cut orange), sloughed off in
                patches along its sun-baked top; bare and broken wood is the trim's debarked log strip. Hulls round the stump and the trunk's two halves (step over its top end).
  SinkWarning   the Sink's broken rim fence: a hand-painted warning on two whitewashed planks nailed to a leaning post:
                DANGER in oxide red beside a crude black skull and crossbones, GROUND GIVES WAY and KEEP OUT under it; the
                lower plank's end is split off. The pivot is the post's foot; SOCKET_Interact in front of the board (a
                read prompt).

Budgets (Art/README.md, the brief): the small props have no Nanite and LODs of 50% and 25%; the dray and the woodshed
pass 2k triangles and keep Nanite with a fallback of every triangle. Every model has baked vertex occlusion; the sheds,
the stack, the dray's body, the pine and the sign's post have UCX_ hulls, the thin parts (shafts, tools, stubs) none.

    artrun.ps1 -Script Art\\Models\\Props\\Backlots.py -Preview [-ScriptArgs --only=Dray,Woodshed]

The previews (--preview) render each piece in the level's golden late afternoon (the sun 15 degrees up; each shot turns
the piece so the sun comes from the camera's front left, as a placement in the level can) beside a 1.8 m figure, a close
three-quarter view of the dray and the woodshed, the family overview and two vignettes: the undertaker's yard (the dray,
the lumber stack and Graves.py's coffins from Intermediate/ArtExport_RR_Chapel) and a backlot (the woodshed and the
lean-to against a false front's back wall), into Saved/ArtPreviews/RansomsRest/Backlots/.
"""
import math
import os
import random

import bmesh
import bpy
from mathutils import Matrix, Vector, noise

import looter_buildings as kit
from looter_buildings import Opening, Tile, Trim
import looter_model as lm
import looter_props as lp
import looter_ruins as lr
import looter_textures as lt
import looter_town as town

# The brief's shared material names (MI_<name> in Unreal): one name, one look, in every model that uses it.
TRIM = lr.material('trim')
PLANKS = lr.material('planks')
END_GRAIN = lr.material('endgrain')
IRON = lr.material('ironblack')
BRASS = lr.material('brass')
WOOD_BLACK = lt.material('WoodPlanks', name='WoodBlack', tint=0x48423c)       # large black-painted wood (the hearse)
PAINT_BLACK = lt.material('PaintWorn', name='PaintBlack', tint=0x2a2622)      # small black paint: lettering, the skull
PAINT_OXIDE = lt.material('PaintWorn', name='PaintOxide', tint=0x8c3e2c)      # oxide-red paint: DANGER
BARK_OAK = lt.material('BarkOak')            # weathered bark: firewood, the fallen pine (Trees.py, DeadTree.py)

SMALL = dict(Nanite=0, LODs='50,25')
PREVIEW_DIR = os.path.join(lt.REPO, 'Saved', 'ArtPreviews', 'RansomsRest', 'Backlots')
TRIM_SCALE = lt.TRIM_DENSITY / lt.TRIM_SIZE                     # UV units per metre on the trim sheet
# Bare, broken and split wood (splinters, split firewood, the pine where its bark is gone) goes on the trim's strip B,
# two debarked logs 40 cm across with the grain along U (the siding's nail rows would read as boards nailed on).
LOGS_V = lt.TRIM_STRIPS['B'][0]
INSET = 1.0 / lt.TRIM_SIZE


def log_v(lane, t):
    """V on strip B, t metres across log lane (0 or 1), kept clear of the chinking at its edges."""
    return LOGS_V + INSET + (lane * 0.4 + 0.04 + min(max(t, 0.0), 0.32)) * TRIM_SCALE


def tile_scale(set_name):
    info = lt.SETS[set_name]
    return info['density'] / float(info['size'])


class Model(lr.Model):
    """looter_ruins' Model (kit parts and looter_props parts merged into one mesh, the brief's materials by key) that
    also takes the town's finish keys ('woodblack', 'oxide', 'black', 'iron', ...) and any material by its name. Keys
    that name one material share one slot."""

    def slot(self, key):
        if key not in self.slots:
            if key in lr._MAKERS:
                mat = lr.material(key)
            elif key in town.MATERIALS:
                mat = town.material(key)
            else:
                mat = bpy.data.materials[key]
            self.slots[key] = (self._index_of(mat), mat)
        return self.slots[key][0]


# --- Building blocks: faces straight into a model's mesh ---

def put_faces(m, cos, faces, smooth=False):
    """Adds faces to the model's mesh: cos are corner positions, faces (indices, uvs, material key). Returns the new
    vertices and faces."""
    verts = [m.bm.verts.new(Vector(c)) for c in cos]
    made = []
    for idx, uvs, key in faces:
        try:
            face = m.bm.faces.new([verts[i] for i in idx])
        except ValueError:
            continue
        face.material_index = m.slot(key)
        face.smooth = smooth
        face[m.floor] = 0.0
        for loop, uv in zip(face.loops, uvs):
            loop[m.uv].uv = uv
        made.append(face)
    return verts, made


def perpendicular(v):
    other = Vector((0.0, 0.0, 1.0)) if abs(v.z) < 0.9 else Vector((1.0, 0.0, 0.0))
    return v.cross(other).normalized()


def bark_tube(m, points, radii, sides, key, seam=(0.0, 0.0, -1.0), rough=0.0, seed=0, flare=None, set_name='BarkOak',
              faces_out=None):
    """A round, tapering part along points (a trunk, a stump, a branch stub, a round of firewood) with bark at world
    scale: U runs round it with its one seam on the side toward seam (a log's underside), V along it. A radius of 0
    closes the end to a point. rough makes the bark lumpy (metres); flare(ring, angle) widens a ring (root buttresses).
    Returns the rings (lists of vertices; a point is a list of one) and each ring's (center, tangent, side, other);
    faces_out (a list) gets each side face with its ring and column, (face, i, k)."""
    rnd = random.Random(seed)
    scale = tile_scale(set_name)
    pts = [Vector(p) for p in points]
    n = len(pts)
    tangents = [(pts[min(i + 1, n - 1)] - pts[max(i - 1, 0)]).normalized() for i in range(n)]
    s = Vector(seam)
    side = (s - tangents[0] * s.dot(tangents[0]))
    side = side.normalized() if side.length > 1e-6 else perpendicular(tangents[0])
    u0, v0 = rnd.uniform(0.0, 3.2), rnd.uniform(0.0, 3.2)
    offset = Vector((rnd.uniform(0, 50), rnd.uniform(0, 50), rnd.uniform(0, 50)))
    rings, frames, along = [], [], [0.0]
    for i in range(1, n):
        along.append(along[-1] + (pts[i] - pts[i - 1]).length)
    for i, (p, t) in enumerate(zip(pts, tangents)):
        if i:
            # Parallel transport: the frame turns with the line, so the tube never twists.
            side = tangents[i - 1].rotation_difference(t) @ side
            side = (side - t * side.dot(t)).normalized()
        other = t.cross(side)
        frames.append((p, t, side, other))
        if radii[i] < 1e-5:
            rings.append([m.bm.verts.new(p)])
            continue
        ring = []
        for k in range(sides):
            a = 2.0 * math.pi * k / sides
            r = radii[i] * (flare(i, a) if flare else 1.0)
            q = p + (side * math.cos(a) + other * math.sin(a)) * r
            if rough:
                q += (q - p).normalized() * rough * noise.noise(q * 5.0 + offset)
            ring.append(m.bm.verts.new(q))
        rings.append(ring)
    slot = m.slot(key)

    def uv(i, k):
        # U from the side opposite the seam (no shear along the top of a tapering log), V along.
        a = 2.0 * math.pi * k / sides
        return ((a - math.pi) * radii_mean(i) * scale + u0, along[i] * scale + v0)

    def radii_mean(i):
        return max(radii[i], radii[max(i - 1, 0)] * 0.5, 0.01)

    for i in range(n - 1):
        lo, hi = rings[i], rings[i + 1]
        for k in range(sides):
            k1 = k + 1
            if len(hi) == 1:
                corners, uvs = (lo[k], lo[k1 % sides], hi[0]), (uv(i, k), uv(i, k1), uv(i + 1, k + 0.5))
            elif len(lo) == 1:
                corners, uvs = (lo[0], hi[k1 % sides], hi[k]), (uv(i, k + 0.5), uv(i + 1, k1), uv(i + 1, k))
            else:
                corners = (lo[k], lo[k1 % sides], hi[k1 % sides], hi[k])
                uvs = (uv(i, k), uv(i, k1), uv(i + 1, k1), uv(i + 1, k))
            try:
                face = m.bm.faces.new(corners)
            except ValueError:
                continue
            face.material_index = slot
            face.smooth = True
            face[m.floor] = 0.0
            for loop, c in zip(face.loops, uvs):
                loop[m.uv].uv = c
            if faces_out is not None:
                faces_out.append((face, i, k))
    return rings, frames


def end_cap(m, ring, center, direction, radius, seed=0, key='endgrain', turn=None):
    """A sawn end on WoodEndGrain: one face over the ring, its rings centered on center (the pith)."""
    d = Vector(direction).normalized()
    ua = perpendicular(d)
    va = d.cross(ua)
    a = random.Random(seed).uniform(0.0, 2.0 * math.pi) if turn is None else turn
    ua, va = ua * math.cos(a) + va * math.sin(a), -ua * math.sin(a) + va * math.cos(a)
    order = list(ring)
    normal = (order[1].co - order[0].co).cross(order[2].co - order[1].co)
    if normal.dot(d) < 0.0:
        order.reverse()
    face = m.bm.faces.new(order)
    face.material_index = m.slot(key)
    face.smooth = False
    face[m.floor] = 0.0
    c = Vector(center)
    for loop in face.loops:
        q = loop.vert.co - c
        loop[m.uv].uv = (0.52 + q.dot(ua) / radius * 0.45, 0.53 + q.dot(va) / radius * 0.45)
    return face


def grain_uv(face, layer, axis, center, width, rnd):
    """Maps one face onto the trim's debarked log wood, the grain along axis (broken wood, splinters)."""
    n = face.normal
    va = n.cross(axis)
    if va.length < 0.2:
        va = perpendicular(axis)
    va.normalize()
    u0 = rnd.uniform(0.0, 6.4)
    lane = rnd.randrange(2)
    ts = [(loop.vert.co - center).dot(va) for loop in face.loops]
    t_min = min(ts)
    squeeze = min(1.0, 0.32 / max(max(ts) - t_min, 1e-4))
    for loop, t in zip(face.loops, ts):
        u = ((loop.vert.co - center).dot(axis) + u0) * TRIM_SCALE
        loop[layer].uv = (u, log_v(lane, (t - t_min) * squeeze))


def break_cap(m, ring, center, direction, radius, seed, spikes=(), reach=1.6, key='trim'):
    """A snapped end: a ring of splinters of uneven length round a low, torn middle, on the trim's weathered grain.
    spikes [(angle, length), ...] stand long splinters up where the wood tore last (angles as the ring's, from its
    first vertex)."""
    rnd = random.Random(seed)
    c, d = Vector(center), Vector(direction).normalized()
    sides = len(ring)
    tips = []
    for k, vert in enumerate(ring):
        out = (vert.co - c) * rnd.uniform(0.5, 0.82)
        length = radius * rnd.uniform(0.15, 1.0) ** 2 * reach
        a = 2.0 * math.pi * k / sides
        for angle, spike in spikes:
            gap = abs((a - angle + math.pi) % (2.0 * math.pi) - math.pi)
            if gap < 2.0 * math.pi / sides * 0.75:
                length = max(length, spike * (1.0 - gap / (2.0 * math.pi / sides)) + radius * 0.2)
                out = (vert.co - c) * 0.88
        tips.append(m.bm.verts.new(c + out + d * length))
    middle = m.bm.verts.new(c + d * radius * rnd.uniform(0.15, 0.35))
    slot = m.slot(key)
    faces = []
    for k in range(sides):
        k1 = (k + 1) % sides
        for corners in ((ring[k], ring[k1], tips[k1], tips[k]), (tips[k], tips[k1], middle)):
            center_f = sum((v.co for v in corners), Vector()) / len(corners)
            normal = (corners[1].co - corners[0].co).cross(corners[2].co - corners[1].co)
            if normal.dot(center_f - c) < 0.0:
                corners = tuple(reversed(corners))
            try:
                face = m.bm.faces.new(corners)
            except ValueError:
                continue
            face.material_index = slot
            face.smooth = False
            face[m.floor] = 0.0
            faces.append(face)
    m.bm.normal_update()
    for face in faces:
        grain_uv(face, m.uv, d, c, radius * 2.0, rnd)
    return tips


def firewood(m, front, length, radius, kind, turn, rnd, back=False, axis='y'):
    """A stick of split firewood: its sawn front end at front (the log's pith there) facing -Y (or -X with axis='x'),
    length long. kind: 'half', 'quarter', 'third' or 'round' (unsplit). Sawn ends on WoodEndGrain (the rings round the
    pith), bark on BarkOak, split faces on the trim's weathered grain. back closes its far end (a loose stick)."""
    r = radius
    if kind == 'half':
        outline = [(r, 0.0), (r * 0.5, r * 0.87), (-r * 0.5, r * 0.87), (-r, 0.0)]
    elif kind == 'quarter':
        outline = [(0.0, 0.0), (r, 0.0), (r * 0.71, r * 0.71), (0.0, r)]
    elif kind == 'third':
        outline = [(0.0, 0.0), (r, 0.0), (r * 0.5, r * 0.87), (-r * 0.5, r * 0.87)]
    else:
        outline = [(r * math.cos(a), r * math.sin(a)) for a in (2.0 * math.pi * k / 6 for k in range(6))]
    c, s = math.cos(turn), math.sin(turn)
    pts = [(u * c - w * s, u * s + w * c) for u, w in outline]
    f = Vector(front)

    def at(u, w, depth):
        if axis == 'y':
            return f + Vector((u, depth, w))
        return f + Vector((depth, -u, w))
    n = len(pts)
    cos = [at(u, w, 0.0) for u, w in pts] + [at(u, w, length) for u, w in pts]
    bark_scale, phase = tile_scale('BarkOak'), rnd.uniform(0.0, 3.0)
    grain_u = rnd.uniform(0.0, 6.4)
    spin = rnd.uniform(0.0, 2.0 * math.pi)
    faces = []
    # The sawn end, the log's rings round its pith (the outline's origin).
    cap_uv = [(0.52 + (u * math.cos(spin) - w * math.sin(spin)) / r * 0.45,
               0.53 + (u * math.sin(spin) + w * math.cos(spin)) / r * 0.45) for u, w in pts]
    faces.append((list(range(n)), cap_uv, 'endgrain'))
    if back:
        faces.append((list(range(2 * n - 1, n - 1, -1)), list(reversed(cap_uv)), 'endgrain'))
    lane = rnd.randrange(2)
    across = rnd.uniform(0.0, 0.08)
    for k in range(n):
        k1 = (k + 1) % n
        (ua, wa), (ub, wb) = pts[k], pts[k1]
        mid = math.hypot((ua + ub) * 0.5, (wa + wb) * 0.5)
        bark = kind == 'round' or mid > 0.6 * r
        idx = [k, k + n, k1 + n, k1]
        if bark:
            a0 = math.atan2(wa, ua)
            a1 = a0 + (math.atan2(wb, ub) - a0 + math.pi) % (2.0 * math.pi) - math.pi
            us = (a0 * r * bark_scale + phase, a1 * r * bark_scale + phase)
            uvs = [(us[0], 0.0), (us[0], length * bark_scale), (us[1], length * bark_scale), (us[1], 0.0)]
            faces.append((idx, uvs, 'BarkOak'))
        else:
            span = math.hypot(ub - ua, wb - wa)
            v_a, v_b = log_v(lane, across), log_v(lane, across + span)
            uvs = [(grain_u * TRIM_SCALE, v_a), ((grain_u + length) * TRIM_SCALE, v_a),
                   ((grain_u + length) * TRIM_SCALE, v_b), (grain_u * TRIM_SCALE, v_b)]
            faces.append((idx, uvs, 'trim'))
    # A front cap faces -Y when its outline runs counterclockwise seen from the front; the sides follow it.
    return put_faces(m, cos, faces)


def outline_box(pts):
    xs = [p[0] for p in pts]
    zs = [p[1] for p in pts]
    return min(xs), max(xs), min(zs), max(zs)


def stick_outline(kind, radius, turn):
    """The cross-section firewood() makes, for packing a stack."""
    r = radius
    if kind == 'half':
        outline = [(r, 0.0), (r * 0.5, r * 0.87), (-r * 0.5, r * 0.87), (-r, 0.0)]
    elif kind == 'quarter':
        outline = [(0.0, 0.0), (r, 0.0), (r * 0.71, r * 0.71), (0.0, r)]
    elif kind == 'third':
        outline = [(0.0, 0.0), (r, 0.0), (r * 0.5, r * 0.87), (-r * 0.5, r * 0.87)]
    else:
        outline = [(r * math.cos(a), r * math.sin(a)) for a in (2.0 * math.pi * k / 6 for k in range(6))]
    c, s = math.cos(turn), math.sin(turn)
    return [(u * c - w * s, u * s + w * c) for u, w in outline]


def stone_pad(m, at, size, seed):
    """A flat fieldstone a post stands on (keeps its foot out of the wet)."""
    part = lr.stone_block(size, (at[0], at[1], size[2] * 0.3 - 0.03), (0.0, 0.0, random.Random(seed).uniform(0, 90)),
                          seed=seed, rough=0.012, bevel=0.0, key='trim', strip='Stone')
    m.add(part)


def shift(m, offset):
    """Moves everything built so far (mesh, hulls, sockets) by offset."""
    d = Vector(offset)
    for v in m.bm.verts:
        v.co += d
    m.hulls = [[Vector(p) + d for p in hull] for hull in m.hulls]
    m.sockets = [(name, tuple(Vector(at) + d), rot) for name, at, rot in m.sockets]


def tilt_all(m, matrix, first_vert=0):
    m.bm.verts.ensure_lookup_table()
    for v in m.bm.verts[first_vert:]:
        v.co = matrix @ v.co


def footprint_middle(m):
    xs = [v.co.x for v in m.bm.verts]
    ys = [v.co.y for v in m.bm.verts]
    return Vector(((min(xs) + max(xs)) * 0.5, (min(ys) + max(ys)) * 0.5, 0.0))


# --- Barrels, crates, tools (the lean-to's clutter, all on the trim sheet) ---

def barrel(m, at, seed, height=0.9, lid=True, yaw=0.0):
    """A wooden barrel: weathered staves (each its own board), three iron hoops, a board head (or open, dark inside)."""
    r_end, r_mid, proud = 0.25, 0.3, 0.008

    def radius(z):
        return r_end + (r_mid - r_end) * math.sin(math.pi * min(max(z / height, 0.0), 1.0)) ** 0.8
    rows = [(0.0, 0.0, None), (radius(0.0), 0.0, 'end')]
    for z0, z1 in ((0.07, 0.12), (height * 0.5 - 0.03, height * 0.5 + 0.03), (height - 0.12, height - 0.07)):
        rows += [(radius(z0), z0, 'stave'), (radius(z0) + proud, z0, 'iron'), (radius(z1) + proud, z1, 'iron'),
                 (radius(z1), z1, 'iron')]
    top = height - (0.03 if lid else 0.09)
    rows += [(radius(height), height, 'stave'), (radius(height) - 0.025, height, 'stave'),
             (radius(height) - 0.025, top, 'stave' if not lid else 'stave'), (0.0, top, 'head')]
    body, bands = lp.lathe([(r, z) for r, z, _ in rows], segments=12)
    kinds = [rows[b + 1][2] for b in bands]
    for v in body.data.vertices:
        if math.hypot(v.co.x, v.co.y) > 0.1:
            k = round(math.atan2(v.co.y, v.co.x) / (2.0 * math.pi / 12))
            wobble = 1.0 + 0.012 * noise.noise(Vector((k * 1.7, seed, 0.3)))
            v.co.x *= wobble
            v.co.y *= wobble
    staves = [i for i, k in enumerate(kinds) if k in ('stave', 'end')]
    hoops = [i for i, k in enumerate(kinds) if k == 'iron']
    head = [i for i, k in enumerate(kinds) if k == 'head']
    lt.assign(body, TRIM)
    lp.lathe_uv(body, 'Siding', staves, grain='up', seed=seed, per_column=True)
    lp.lathe_uv(body, 'Iron', hoops, grain='around', seed=seed + 1)
    lt.trim_uv(body, head, 'Siding' if lid else 'Beams', align='world', v_offset=0.3 if lid else 0.6)
    lp.place(body, (at[0], at[1], at[2] if len(at) > 2 else 0.0), (0.0, 0.0, yaw))
    m.add(body)


def crate(m, at, size, seed, yaw=0.0, z=0.0):
    """A plank crate: its faces the trim's siding at world scale (boards and nail rows), corner battens, top battens."""
    rnd = random.Random(seed)
    sx, sy, sz = size
    frame = kit.place((at[0], at[1], z), (0.0, 0.0, yaw))
    parts = []
    body = lp.block((sx - 0.02, sy - 0.02, sz - 0.01), (0.0, 0.0, (sz - 0.01) * 0.5))
    lt.assign(body, TRIM)
    lt.trim_uv(body, None, 'Siding', align='world', cut=True, u_offset=rnd.uniform(0.0, 6.4),
               v_offset=rnd.uniform(0.0, 0.2))
    parts.append(body)
    for x in (-1.0, 1.0):
        for y in (-1.0, 1.0):
            post = lp.block((0.05, 0.05, sz), (x * (sx * 0.5 - 0.02), y * (sy * 0.5 - 0.02), sz * 0.5))
            parts.append(lp.grain(post, 'Beams', axis=(0.0, 0.0, 1.0), seed=rnd.randint(0, 999)))
    for y in (-1.0, 1.0):
        bat = lp.block((sx, 0.03, 0.07), (0.0, y * (sy * 0.5 + 0.005), sz - 0.045))
        parts.append(lp.grain(bat, 'Beams', axis=(1.0, 0.0, 0.0), seed=rnd.randint(0, 999)))
    for p in parts:
        p.data.transform(frame)
        m.add(p)


def shovel(m, foot, top, seed):
    """A long-handled shovel stood on its blade, its handle against the wall."""
    foot, top = Vector(foot), Vector(top)
    d = (top - foot).normalized()
    handle = lp.sweep([foot + d * 0.28, top], lp.ngon(0.018, 5, seed=seed))
    lp.grain(handle, 'Beams', axis=d, seed=seed)
    m.add(handle)
    blade = lp.block((0.24, 0.012, 0.3), (0.0, 0.0, 0.0))
    for v in blade.data.vertices:
        v.co.y += 0.02 * (1.0 - (v.co.x / 0.12) ** 2)          # dished
        if v.co.z < 0.0:
            v.co.x *= 0.85 + 0.15 * (v.co.z + 0.15) / 0.15        # a rounded point
    lp.trim(blade, 'Iron', seed=seed)
    frame = kit.toward(foot, top, face=(0.0, -1.0, 0.0))
    blade.data.transform(frame @ Matrix.Translation((0.15, 0.0, 0.0)) @ Matrix.Rotation(math.radians(90.0), 4, 'Y'))
    m.add(blade)


def pick(m, foot, top, seed):
    """A pick, its head on the ground, the handle against the wall."""
    foot, top = Vector(foot), Vector(top)
    d = (top - foot).normalized()
    handle = lp.sweep([foot + d * 0.05, top], lp.ngon(0.02, 5, seed=seed), scales=[1.15, 0.9])
    lp.grain(handle, 'Beams', axis=d, seed=seed + 1)
    m.add(handle)
    side = d.cross(Vector((0.0, 1.0, 0.0))).normalized()
    head = lp.sweep([foot + side * 0.33 + d * 0.05, foot + side * 0.12 + d * 0.11, foot + d * 0.12,
                     foot - side * 0.12 + d * 0.11, foot - side * 0.33 + d * 0.04],
                    [(-0.016, -0.022), (0.016, -0.022), (0.016, 0.022), (-0.016, 0.022)],
                    up=(0.0, 1.0, 0.0), scales=[0.35, 0.9, 1.1, 0.9, 0.4])
    lp.trim(head, 'Iron', seed=seed + 2)
    m.add(head)


def rope_coil(m, hang, seed, radius=0.17):
    """A coil of rope hung on a peg: two loops hanging from the peg, sagging under their weight."""
    parts = []
    for k in range(2):
        pts = []
        for j in range(13):
            a = 2.0 * math.pi * j / 12
            pts.append((hang[0] + 0.012 * k, hang[1] - 0.03 - 0.025 * k,
                        hang[2] - radius - radius * math.cos(a) * (1.0 + 0.12 * k) + 0.02 * k))
            x, y, z = pts[-1]
            pts[-1] = (x + radius * 0.85 * math.sin(a), y, z)
        parts.append(lr.tube(pts, 0.014, sides=5, lane=1 + k, seed=seed + k, caps=(False, False)))
    peg = lp.block((0.03, 0.12, 0.03), (hang[0], hang[1] - 0.04, hang[2] + 0.01))
    parts.append(lp.grain(peg, 'Beams', axis=(0.0, 1.0, 0.0), seed=seed))
    for p in parts:
        m.add(p, smooth=True)


# --- The woodshed ---

def woodshed(name, seed):
    m = Model(name, seed=seed)
    rng = m.rng
    XP, YF, YB = 1.3, -0.7, 0.7          # posts: x = -XP, 0, XP; front and back rows
    ZF, ZB = 2.25, 1.85                  # the plates' tops, front and back
    POST = 0.12
    # Posts on flat stones, plates along the front and back, girts down the sides, knee braces at the front.
    for k, x in enumerate((-XP, 0.0, XP)):
        for y, top in ((YF, ZF - 0.15), (YB, ZB - 0.15)):
            stone_pad(m, (x, y), (0.3, 0.28, 0.12), seed + 10 + k + int(y * 10))
            m.board((x, y, 0.05), (x, y, top), POST, POST, face=(0.0, -1.0, 0.0), uv=Trim('C', lane='each'))
    for y, z in ((YF, ZF), (YB, ZB)):
        m.board((-XP - 0.16, y, z - 0.075), (XP + 0.16, y, z - 0.075), 0.15, 0.13, face=(0.0, -1.0, 0.0),
                uv=Trim('C', lane='each'), cuts=2)
    for x in (-XP, XP):
        m.board((x, YB - 0.06, ZB - 0.2), (x, YF + 0.06, ZF - 0.2), 0.1, 0.08, face=(1.0 if x > 0 else -1.0, 0.0, 0.0),
                uv=Trim('C', lane='each'))
    for x, sign in ((-XP, 1.0), (0.0, 1.0), (0.0, -1.0), (XP, -1.0)):
        m.board((x, YF, ZF - 0.62), (x + sign * 0.42, YF, ZF - 0.16), 0.1, 0.07, face=(0.0, -1.0, 0.0), uv='C')
    m.section('frame')

    # The roof: one slope falling to the back, on rafters, shakes on a board deck, a fascia along the front edge.
    theta = math.atan2(ZF - ZB, YB - YF)
    over, rake, deck = 0.32, 0.22, 0.06
    normal = Vector((0.0, math.sin(theta), math.cos(theta)))
    origin = Vector((XP + POST * 0.5 + rake, YB + over, ZB - over * math.tan(theta))) + normal * deck
    width = 2.0 * (XP + POST * 0.5 + rake)
    length = (YB - YF + 2.0 * over) / math.cos(theta)
    slope = kit.Slope(origin, (-1.0, 0.0, 0.0), (0.0, -math.cos(theta), math.sin(theta)), width, length, sag=0.03)
    kit.roof_deck(m, slope, deck)
    kit.shingles(m, slope)
    for x in kit.frange(0.25, width - 0.3, (width - 0.5) / 4.0) + [width - 0.25]:
        m.box((0.06, length - 0.06, 0.11), at=(x, length * 0.5, -deck - 0.055), uv=Trim('C', lane='each'), space=slope)
    m.board((-0.02, length + 0.012, -0.02), (width + 0.02, length + 0.012, -0.02), deck + 0.12, 0.035,
            face=(0.0, 1.0, 0.0), uv='C', space=slope)
    m.board((-0.02, length - 0.06, 0.06), (width + 0.02, length - 0.06, 0.06), 0.16, 0.025, face=(0.0, 0.0, 1.0),
            uv='C', space=slope)
    m.section('roof')

    # The back: boards and battens. The sides: boards with air gaps between them, up to the back plate.
    yb = YB + POST * 0.5
    back = kit.wall_space((XP + 0.08, yb), (-XP - 0.08, yb), 0.06)
    span = 2.0 * XP + 0.16
    m.panel(kit.wall_outline(span, ZB - 0.2), (), 0.025, Trim('A', world=True, rotate=True), space=back)
    for x in kit.frange(0.1, span - 0.05, 0.4) + [span - 0.1]:
        m.board((x, -0.015, 0.0), (x, -0.015, ZB - 0.22), 0.06, 0.02, face=(0.0, -1.0, 0.0), uv='A', space=back)
    for sign in (-1.0, 1.0):
        x = sign * (XP + POST * 0.5 + 0.013)
        for z in kit.frange(0.19, ZB - 0.25, 0.22):
            m.board((x, YF - 0.07, z), (x, YB + 0.07, z), 0.18, 0.025, face=(sign, 0.0, 0.0), uv='A')
    m.section('walls')

    # Firewood: two bearer poles, then courses of split sticks with their sawn ends out: full to 1.55 m on the left,
    # burnt down on the right. Behind the front row the stack is a block whose top shows bark.
    for y in (-0.48, 0.3):
        rings, _ = bark_tube(m, [(-XP + 0.08, y, 0.05), (XP - 0.08, y, 0.05)], [0.05, 0.05], 6, 'BarkOak',
                             seed=seed + int(y * 100), set_name='BarkOak')
    front, depth = -0.63, 0.42
    x0, x1, split_x = -XP + POST * 0.5 + 0.02, XP - POST * 0.5 - 0.02, 0.3
    kinds = ['half', 'quarter', 'third', 'half', 'quarter', 'round']
    tops = {}
    z = 0.1
    for course in range(8):
        x = x0
        height = 0.0
        limit = x1 if course < 3 else x1 - 0.45 if course == 3 else split_x + (0.25 if course == 4 else 0.0)
        while x < limit - 0.08:
            kind = rng.choice(kinds)
            radius = rng.uniform(0.085, 0.115) if kind != 'round' else rng.uniform(0.05, 0.065)
            turn = rng.uniform(-math.pi, math.pi)
            pts = stick_outline(kind, radius, turn)
            lo_u, hi_u, lo_w, hi_w = outline_box(pts)
            w = hi_u - lo_u
            if x + w * 0.92 > limit:
                break
            cx = x - lo_u
            cz = z - lo_w - rng.uniform(0.0, 0.02)
            firewood(m, (cx, front + rng.uniform(-0.03, 0.02), cz), depth, radius, kind, turn, rng)
            x += w * 0.9
            height = max(height, (hi_w - lo_w) * 0.88)
        tops[course] = (z + height, x)
        z += height
    full = tops[7][0]
    low = tops[2][0]
    for (a, b, top) in ((x0, split_x, full - 0.06), (split_x, x1, low - 0.05)):
        tb = kit._box((b - a, YB - 0.06 - (front + depth), top - 0.08), drop=('-z', '-y'))
        m.emit(tb, Tile('BarkOak'), 'BarkOak', kit.place(((a + b) * 0.5, (front + depth + YB - 0.06) * 0.5,
                                                            0.08 + (top - 0.08) * 0.5)))
    m.section('firewood')

    # The chopping block in front of the right post: a round of oak, an axe stuck in it, split sticks round it.
    bx, by = 0.95, -1.2
    rings, frames = bark_tube(m, [(bx, by, -0.04), (bx, by, 0.2), (bx, by, 0.44)], [0.25, 0.245, 0.24], 9, 'BarkOak',
                              seam=(0.0, 1.0, 0.0), rough=0.01, seed=seed + 5, set_name='BarkOak')
    end_cap(m, rings[-1], (bx, by, 0.44), (0.0, 0.0, 1.0), 0.24, seed=seed + 6)
    axe_at = Vector((bx - 0.05, by - 0.02, 0.44))
    handle_dir = Vector((-0.55, -0.35, 0.75)).normalized()
    m.board(axe_at + handle_dir * 0.05, axe_at + handle_dir * 0.78, 0.045, 0.03, face=(0.0, -1.0, 0.0), uv='C')
    head = kit.toward(axe_at - handle_dir * 0.02, axe_at + handle_dir * 0.1, face=(0.0, -1.0, 0.0))
    m.box((0.16, 0.035, 0.11), matrix=head @ kit.place((0.05, 0.0, 0.0)), uv=Trim('H3', fit=True))
    for k in range(3):
        a = rng.uniform(-2.4, -0.6)
        kind = rng.choice(('half', 'quarter', 'third'))
        stick_r = rng.uniform(0.08, 0.1)
        f = Vector((bx + math.cos(a) * 0.5, by + math.sin(a) * 0.45, 0.0))
        firewood(m, f + Vector((0.0, 0.0, 0.01)), 0.4, stick_r, kind, rng.uniform(-0.3, 0.3) + math.pi * (k % 2),
                 rng, back=True, axis='x' if k % 2 else 'y')
    m.section('block')
    m.hull((2.0 * XP + 0.2, YB - YF + 0.2, ZB), at=(0.0, 0.0, ZB * 0.5))
    m.hull_points([(x, y, z) for x in (-XP - 0.1, XP + 0.1) for y, z in ((YF - 0.1, ZB), (YF - 0.1, ZF + 0.12),
                                                                          (YB + 0.1, ZB), (YB + 0.1, ZB + 0.12))])
    m.hull((0.5, 0.5, 0.44), at=(bx, by, 0.22))
    m.socket('Interact', (bx - 0.2, by - 0.35, 0.6), (0.0, 0.0, 0.0))
    return m.finish(fallback=100.0, ao_distance=0.7, preview=False)


# --- The lean-to ---

def lean_to(name, seed):
    m = Model(name, seed=seed)
    rng = m.rng
    WALL_Y = 0.95                        # the wall's face (the ledger's back)
    YP, XP = -0.72, 1.35                 # front posts
    POST_TOP, BEAM = 2.18, 0.16
    LEDGER_TOP = 2.72                    # the rafters' tops at the wall; the tin over them tops out at 2.8 m
    for x in (-XP, XP):
        stone_pad(m, (x, YP), (0.28, 0.28, 0.11), seed + int(x * 10))
        m.board((x, YP, 0.05), (x, YP, POST_TOP), 0.12, 0.12, face=(0.0, -1.0, 0.0), uv=Trim('C', lane='each'))
        sign = 1.0 if x < 0 else -1.0
        m.board((x, YP, POST_TOP - 0.5), (x + sign * 0.42, YP, POST_TOP - 0.05), 0.09, 0.07, face=(0.0, -1.0, 0.0),
                uv='C')
    m.board((-XP - 0.2, YP, POST_TOP + BEAM * 0.5), (XP + 0.2, YP, POST_TOP + BEAM * 0.5), BEAM, 0.12,
            face=(0.0, -1.0, 0.0), uv=Trim('C', lane='each'), cuts=2)
    m.board((-XP - 0.25, WALL_Y - 0.025, LEDGER_TOP - 0.11), (XP + 0.25, WALL_Y - 0.025, LEDGER_TOP - 0.11), 0.22,
            0.05, face=(0.0, -1.0, 0.0), uv=Trim('C', lane='each'), cuts=2)
    # Rafters notched over the ledger and the beam, the roof on them: boards and corrugated tin.
    theta = math.atan2(LEDGER_TOP - (POST_TOP + BEAM), WALL_Y - 0.05 - YP)
    over = 0.3
    up = Vector((0.0, -math.sin(theta), math.cos(theta)))          # out of the roof (it falls to the front)
    r_top = Vector((0.0, WALL_Y - 0.05, LEDGER_TOP))
    r_dir = Vector((0.0, -math.cos(theta), -math.sin(theta)))
    r_len = (WALL_Y - 0.05 - YP + over) / math.cos(theta)
    for x in (-XP - 0.12, -0.68, 0.0, 0.68, XP + 0.12):
        a = r_top + Vector((x, 0.0, 0.0)) - up * 0.06
        m.board(a, a + r_dir * r_len, 0.12, 0.06, face=(1.0, 0.0, 0.0), uv=Trim('C', lane='each'))
    deck = 0.03
    eave = r_top + r_dir * r_len
    width = 2.0 * (XP + 0.3)
    origin = Vector((-width * 0.5, eave.y, eave.z)) + up * deck
    slope = kit.Slope(origin, (1.0, 0.0, 0.0), (0.0, math.cos(theta), math.sin(theta)), width, r_len, sag=0.02)
    # Board and batten (a low shed roof; the trim's tin reads blue over this much roof): boards running down the
    # slope, battens over every other joint, rake boards at the ends.
    m.box((width, r_len, deck), at=(width * 0.5, r_len * 0.5, -deck * 0.5), uv=Trim('A', world=True, rotate=True, u=0.0),
          space=slope, cuts=2)
    for x in kit.frange(0.4, width - 0.1, 0.4):
        m.box((0.07, r_len + 0.01, 0.022), at=(x + rng.uniform(-0.01, 0.01), r_len * 0.5, 0.011),
              uv=Trim('C', lane=rng.randrange(4)), space=slope, drop=('-z',))
    for x, face in ((-0.025, (-1.0, 0.0, 0.0)), (width + 0.025, (1.0, 0.0, 0.0))):
        m.board((x, -0.02, -0.01), (x, r_len, -0.01), 0.1, 0.05, face=face, uv='C', space=slope)
    m.board((-0.02, -0.025, -0.015), (width + 0.02, -0.025, -0.015), 0.09, 0.035, face=(0.0, -1.0, 0.0), uv='C',
            space=slope)
    # A flashing board along the wall over the roof's top edge.
    m.board((-width * 0.5, WALL_Y - 0.06, LEDGER_TOP + 0.07), (width * 0.5, WALL_Y - 0.06, LEDGER_TOP + 0.07), 0.14,
            0.025, face=(0.0, -1.0, 0.0), uv='C')
    m.section('frame and roof')

    # Under it, against the wall: two barrels (one lidded, one open with a broom in it), two crates stacked, a shovel
    # and a pick leaning on the wall, a coil of rope on the left post.
    barrel(m, (-0.95, 0.55), seed + 20, yaw=rng.uniform(0, 360))
    barrel(m, (-0.33, 0.62), seed + 21, height=0.82, lid=False, yaw=rng.uniform(0, 360))
    broom_foot = Vector((-0.33, 0.62, 0.2))
    broom_top = Vector((-0.26, 0.75, 1.55))
    stick = lp.sweep([broom_foot, broom_top], lp.ngon(0.016, 5, seed=seed))
    lp.grain(stick, 'Beams', axis=broom_top - broom_foot, seed=seed + 3)
    m.add(stick)
    crate(m, (0.85, 0.6), (0.72, 0.6, 0.55), seed + 30, yaw=rng.uniform(-4, 4))
    crate(m, (0.8, 0.62), (0.58, 0.5, 0.42), seed + 31, yaw=rng.uniform(8, 14), z=0.55)
    shovel(m, (0.25, 0.75, 0.0), (0.3, WALL_Y - 0.03, 1.32), seed + 40)
    pick(m, (0.42, 0.62, 0.0), (0.45, WALL_Y - 0.03, 0.86), seed + 41)
    rope_coil(m, (-XP, YP - 0.065, 1.62), seed + 50)
    m.section('clutter')
    for x in (-XP, XP):
        m.hull((0.14, 0.14, POST_TOP), at=(x, YP, POST_TOP * 0.5))
    m.hull((2.3, 0.75, 1.0), at=(-0.05, WALL_Y - 0.4, 0.5))
    roof = []
    for x in (-width * 0.5, width * 0.5):
        for y, z in ((eave.y, eave.z), (WALL_Y, LEDGER_TOP)):
            roof += [(x, y, z - 0.08), (x, y, z + 0.08)]
    m.hull_points(roof)
    return m.finish(fallback=None, ao_distance=0.6, preview=False, **SMALL)


# --- The lumber stack ---

def lumber_stack(name, seed):
    m = Model(name, seed=seed)
    rng = m.rng
    bearers = (-1.55, 0.0, 1.55)
    for x in bearers:
        m.board((x, -0.75, 0.08), (x, 0.75, 0.08), 0.16, 0.16, face=(0.0, 0.0, 1.0), uv=Trim('C', lane='each'))
    z = 0.16
    T = 0.05
    for layer in range(10):
        count = 6 if layer < 9 else 4
        widths = [rng.uniform(0.21, 0.26) for _ in range(count)]
        gap = (1.45 - sum(widths)) / max(count - 1, 1)
        y = -0.725
        for k, w in enumerate(widths):
            length = rng.uniform(3.45, 3.7)
            x = rng.uniform(-0.07, 0.07)
            yc = y + w * 0.5 + rng.uniform(-0.01, 0.01)
            m.board((x - length * 0.5, yc, z + T * 0.5), (x + length * 0.5, yc, z + T * 0.5), w, T,
                    face=(0.0, 0.0, 1.0), uv=lr.PlankRow(), mat='planks')
            y += w + gap
        z += T
        if layer < 9:
            for x in bearers:
                m.board((x + rng.uniform(-0.04, 0.04), -0.74, z + 0.0125), (x + rng.uniform(-0.04, 0.04), 0.74,
                        z + 0.0125), 0.035, 0.025, face=(0.0, 0.0, 1.0), uv='C')
            z += 0.025
    top = z
    # Coffin boards (2 m) laid across the top on two stickers, a little fanned.
    for x in (0.55, 1.45):
        m.board((x, -0.6, top + 0.0125), (x, 0.6, top + 0.0125), 0.035, 0.025, face=(0.0, 0.0, 1.0), uv='C')
    cz = top + 0.025
    for k in range(5):
        x = 0.62 + k * 0.165 + rng.uniform(-0.01, 0.01)
        yaw = rng.uniform(-3.0, 3.0)
        d = Vector((math.sin(math.radians(yaw)), math.cos(math.radians(yaw)), 0.0))
        c = Vector((x + 0.33 * 0.0, rng.uniform(-0.04, 0.04), cz + 0.0175 + (0.035 if k % 2 else 0.0)))
        m.board(c - d * 1.0, c + d * 1.0, 0.3, 0.035, face=(0.0, 0.0, 1.0), uv=lr.PlankRow(), mat='planks')
    # Three more leaning on the front, resting on the stack's top edge.
    edge = Vector((0.0, -0.72, top + 0.01))
    for k, (x, out) in enumerate(((-1.25, 0.58), (-0.92, 0.5), (-0.6, 0.62))):
        foot = Vector((x + rng.uniform(-0.03, 0.03), edge.y - out, 0.02))
        rest = Vector((x + rng.uniform(-0.08, 0.08), edge.y + 0.02, edge.z + 0.02))
        d = (rest - foot).normalized()
        m.board(foot, foot + d * 2.0, 0.3 - 0.02 * k, 0.035, face=(0.0, -1.0, 0.25), uv=lr.PlankRow(), mat='planks')
    m.section('lumber')
    m.hull((3.85, 1.55, top + 0.12), at=(0.0, 0.0, (top + 0.12) * 0.5))
    m.hull_points([(x, y, z) for x in (-1.45, -0.42) for y, z in ((-0.72, top + 0.4), (-0.72, 0.0), (-1.38, 0.0))])
    return m.finish(fallback=None, ao_distance=0.5, preview=False, **SMALL)


# --- The undertaker's dray ---

def wheel(radius, width, spokes, seed, hub=0.085, felloe=0.06, tyre=0.012, segments=14, outer=1.0):
    """A sound spoked wheel in the XY plane round the origin, its axle along Z (outer: the side that faces out, +1 or
    -1 along Z): a black felloe and spokes (WoodBlack, grain round the felloe and along each spoke), an iron tyre
    (IronBlack), a hub with iron bands and a brass cap. Returns an lp part."""
    rnd = random.Random(seed)
    inner, rt, w2 = radius - felloe, radius - tyre, width * 0.5
    ring, bands = lp.lathe([(inner, -w2), (rt, -w2), (radius, -w2), (radius, w2), (rt, w2), (inner, w2)],
                           segments=segments, closed=True)
    lt.assign(ring, WOOD_BLACK)
    tyre_faces = [i for i, b in enumerate(bands) if b in (1, 2, 3)]
    lt.assign(ring, IRON, tyre_faces)
    mesh = ring.data
    layer = mesh.uv_layers.active.data
    planks = tile_scale('WoodPlanks')
    metal = tile_scale('MetalWorn')
    lane = rnd.randrange(16)
    for p in mesh.polygons:
        iron = p.index in tyre_faces
        flat = abs(p.normal.z) > 0.9
        cos = [mesh.vertices[mesh.loops[li].vertex_index].co for li in p.loop_indices]
        a_c = math.atan2(p.center.y, p.center.x)
        across = [math.hypot(c.x, c.y) if flat else c.z for c in cos]
        lo = min(across)
        for li, c, t in zip(p.loop_indices, cos, across):
            a = a_c + (math.atan2(c.y, c.x) - a_c + math.pi) % (2.0 * math.pi) - math.pi
            if iron:
                layer[li].uv = (a * radius * metal, t * metal)
            else:
                layer[li].uv = (a * (inner + felloe * 0.5) * planks, (lane * 0.2 + 0.03 + (t - lo)) * planks)
    parts = [ring]
    start = rnd.uniform(0.0, math.pi)
    for k in range(spokes):
        a = start + 2.0 * math.pi * k / spokes
        spoke = lp.sweep([(hub * 0.8, 0.0, 0.0), (inner + 0.012, 0.0, 0.0)],
                         [(-0.015, -0.021), (0.015, -0.021), (0.012, 0.021), (-0.012, 0.021)], scales=[1.0, 0.82])
        bm = bmesh.new()
        bm.from_mesh(spoke.data)
        bm.normal_update()
        bmesh.ops.delete(bm, geom=[f for f in bm.faces if abs(f.normal.x) > 0.9], context='FACES_ONLY')
        bm.to_mesh(spoke.data)
        bm.free()
        lt.assign(spoke, WOOD_BLACK)
        lr.PlankRow().apply(spoke, rnd)
        lp.place(spoke, (0.0, 0.0, 0.0), (0.0, 0.0, math.degrees(a)))
        parts.append(spoke)
    hw = width * 1.15
    profile = [(0.0, -hw), (hub * 0.62, -hw), (hub, -hw * 0.62), (hub, hw * 0.62), (hub * 0.62, hw), (0.0, hw)]
    if outer < 0:
        profile = [(r, -z) for r, z in reversed(profile)]
    hub_part, hb = lp.lathe(profile, segments=8)
    lt.assign(hub_part, WOOD_BLACK)
    iron_bands = [i for i, b in enumerate(hb) if b in (1, 3)]
    cap = [i for i, b in enumerate(hb) if b == 4]
    lt.assign(hub_part, IRON, iron_bands)
    lt.assign(hub_part, BRASS, cap)
    lp.lathe_uv(hub_part, 'WoodPlanks', [i for i, b in enumerate(hb) if b in (0, 2)], grain='up', seed=seed)
    lp.lathe_uv(hub_part, 'MetalWorn', iron_bands + cap, grain='around', seed=seed + 1)
    lr.split_poles(hub_part, 'around')
    parts.append(hub_part)
    return lp.join('_wheel', parts)


def leaf_spring(m, center_y, x, axle_top, bed_z, seed, half=0.42):
    """A semi-elliptic leaf spring along Y: its middle clamped on the axle by two U-bolts, its ends shackled under the
    bed, three leaves, the shortest underneath."""
    low, high = axle_top + 0.012, bed_z - 0.045            # the main leaf's middle and ends
    rise = high - low
    for k, (length, drop) in enumerate(((half, 0.0), (half * 0.78, 0.0125), (half * 0.55, 0.025))):
        pts = []
        for j in range(7):
            t = -1.0 + 2.0 * j / 6
            pts.append((x, center_y + t * length, low - drop + rise * t * t * (length / half) ** 2))
        leaf = lp.sweep(pts, [(-0.03, -0.006), (0.03, -0.006), (0.03, 0.006), (-0.03, 0.006)])
        lt.assign(leaf, IRON)
        lt.box_uv(leaf, 'MetalWorn', seed=seed + k)
        m.add(leaf)
    for dy in (-0.06, 0.06):
        town.box(m, (0.08, 0.025, 0.09), at=(x, center_y + dy, axle_top + 0.01), look='iron')
    for dy in (-half, half):
        town.box(m, (0.075, 0.04, 0.07), at=(x, center_y + dy, bed_z - 0.035), look='iron')


def carriage_lamp(m, at, side, socket_name):
    """A brass carriage lamp on a bracket off the dashboard: a square body with glass on three sides (the trim's dark
    window glass), a pitched cap and a chimney. SOCKET_<socket_name> at its glass."""
    x, y, z = at
    town.box(m, (0.14, 0.14, 0.03), at=(x, y, z - 0.105), look='brass')
    town.box(m, (0.12, 0.12, 0.18), at=(x, y, z), look='H4')
    for dx in (-0.06, 0.06):
        for dy in (-0.06, 0.06):
            town.box(m, (0.02, 0.02, 0.2), at=(x + dx, y + dy, z), look='brass')
    town.box(m, (0.145, 0.145, 0.025), at=(x, y, z + 0.1), look='brass')
    town.lathe(m, [(0.1, 0.0), (0.0, 0.07)], at=(x, y, z + 0.112), sides=4, look='brass', phase=math.pi * 0.25)
    town.cylinder(m, (x, y, z + 0.15), (x, y, z + 0.22), 0.022, sides=6, look='brass')
    town.box(m, (0.03, 0.12, 0.03), at=(x - side * 0.07, y, z - 0.06), look='iron')
    m.socket(socket_name, (x, y, z))


def dray(name, seed):
    m = Model(name, seed=seed)
    rng = m.rng
    BED_Z, HW, HL = 0.95, 0.65, 1.45     # the floor's top, half width, half length (the bed's middle at the origin)
    FRONT, REAR = -1.0, 0.95             # axles
    RF, RR = 0.4, 0.47                   # wheel radii: low, so the bed (and its lettering) clears them
    WX = 0.8                             # wheel planes
    SILL = 0.83                          # the sills' bottoms (on the springs)
    SEAT_Y, LOAD_Y = -0.76, -0.5         # the bench's middle; the load bed (and the side boards) from here back
    # The floor: seven boards worn bare on top where the coffins slide (the trim's weathered siding).
    for k in range(7):
        x = -HW + 0.093 + k * 0.186
        m.board((x, -HL, BED_Z - 0.02), (x, HL, BED_Z - 0.02), 0.178, 0.04, face=(0.0, 0.0, 1.0), uv='A')
    # The frame and sides in black: sills on the springs, raves along the edges, low side boards lettered in brass,
    # a rail across the back, iron stake irons at the side boards' ends.
    for s in (-1.0, 1.0):
        town.board(m, (s * 0.45, -HL + 0.05, SILL + 0.04), (s * 0.45, HL - 0.05, SILL + 0.04), 0.08, 0.1,
                   look='woodblack', face=(s, 0.0, 0.0))
        town.board(m, (s * (HW + 0.025), -HL, BED_Z - 0.065), (s * (HW + 0.025), HL, BED_Z - 0.065), 0.13, 0.05,
                   look='woodblack', face=(s, 0.0, 0.0))
        town.board(m, (s * (HW + 0.02), LOAD_Y, BED_Z + 0.11), (s * (HW + 0.02), HL, BED_Z + 0.11), 0.22, 0.04,
                   look='woodblack', face=(s, 0.0, 0.0))
        for y in (LOAD_Y + 0.06, HL - 0.06):
            town.box(m, (0.012, 0.05, 0.36), at=(s * (HW + 0.047), y, BED_Z + 0.06), look='iron')
        for y in (-1.1, 0.3, 1.0):
            town.box(m, (0.012, 0.05, 0.12), at=(s * (HW + 0.054), y, BED_Z - 0.065), look='iron')
    for y, sign in ((-HL, -1.0), (HL, 1.0)):
        town.board(m, (-HW - 0.045, y + sign * 0.025, BED_Z - 0.065), (HW + 0.045, y + sign * 0.025, BED_Z - 0.065),
                   0.13, 0.05, look='woodblack', face=(0.0, sign, 0.0))
    town.board(m, (-HW, HL + 0.02, BED_Z + 0.11), (HW, HL + 0.02, BED_Z + 0.11), 0.22, 0.04, look='woodblack',
               face=(0.0, 1.0, 0.0))
    # BRIGHT & DAUGHTER along both side boards, reading front to back on the right and back to front on the left.
    right = kit.wall_space((HW + 0.04, LOAD_Y), (HW + 0.04, HL))
    left = kit.wall_space((-HW - 0.04, HL), (-HW - 0.04, LOAD_Y))
    for space in (right, left):
        town.text(m, 'BRIGHT & DAUGHTER', ((HL - LOAD_Y) * 0.5, 0.0, BED_Z + 0.112), height=0.095, width=1.62,
                  space=space, look='brass', tol=0.026)
    m.section('bed')

    # The driver's bench at the front: a box seat with a backrest, brass hand rails, a dashboard leaning forward over
    # the footboard with a brass rail and the lamps.
    town.box(m, (1.0, 0.42, 0.38), at=(0.0, SEAT_Y, BED_Z + 0.19), look='woodblack')
    town.box(m, (1.06, 0.48, 0.045), at=(0.0, SEAT_Y, BED_Z + 0.4), look='woodblack')
    town.board(m, (-0.5, SEAT_Y + 0.25, BED_Z + 0.66), (0.5, SEAT_Y + 0.25, BED_Z + 0.66), 0.24, 0.04,
               look='woodblack', face=(0.0, -0.97, 0.24))
    for x in (-0.42, 0.42):
        town.box(m, (0.04, 0.03, 0.34), at=(x, SEAT_Y + 0.24, BED_Z + 0.52), rot=(-14.0, 0.0, 0.0), look='iron')
    for s in (-1.0, 1.0):
        rail = [(s * 0.53, SEAT_Y - 0.2, BED_Z + 0.42), (s * 0.55, SEAT_Y - 0.15, BED_Z + 0.58),
                (s * 0.55, SEAT_Y + 0.1, BED_Z + 0.62), (s * 0.53, SEAT_Y + 0.22, BED_Z + 0.68)]
        m.add(lr.tube(rail, 0.013, sides=5, key='brass', set_name='MetalWorn', seed=seed + int(s)))
    lean = Vector((0.0, -0.34, 0.94))
    dash_foot = Vector((0.0, -HL - 0.02, BED_Z))
    town.board(m, dash_foot + lean * 0.22 - Vector((0.56, 0.0, 0.0)), dash_foot + lean * 0.22 + Vector((0.56, 0.0, 0.0)),
               0.44, 0.035, look='woodblack', face=(0.0, -0.94, -0.34))
    dash_top = dash_foot + lean * 0.45
    town.box(m, (1.18, 0.03, 0.03), at=tuple(dash_top - Vector((0.0, 0.03, 0.0))), rot=(20.0, 0.0, 0.0), look='brass')
    for i, s in enumerate((-1.0, 1.0)):
        carriage_lamp(m, (s * 0.66, dash_top.y + 0.02, dash_top.z - 0.02), s, f'Light_{i + 1}')
    m.socket('Sit', (0.0, SEAT_Y, BED_Z + 0.42))
    m.section('bench')

    # Running gear: the axles, spring blocks, leaf springs under the sills, the reach between the axles.
    for y, r in ((FRONT, RF), (REAR, RR)):
        town.box(m, (2.0 * WX - 0.02, 0.1, 0.1), at=(0.0, y, r), look='woodblack')
        axle_top = r + 0.05
        if y == FRONT:
            for s in (-1.0, 1.0):
                town.box(m, (0.14, 0.2, 0.12), at=(s * 0.45, y, axle_top + 0.06), look='woodblack')
            axle_top += 0.12
        for s in (-1.0, 1.0):
            leaf_spring(m, y, s * 0.45, axle_top, SILL, seed + int(y * 10) + int(s * 3))
    town.board(m, (0.0, FRONT, 0.5), (0.0, REAR, 0.6), 0.09, 0.08, look='woodblack', face=(0.0, 0.0, 1.0))
    # The brake: a lever by the bench, a rod back to a beam with blocks against the rear tyres.
    lever_foot = Vector((HW + 0.085, SEAT_Y + 0.18, BED_Z - 0.1))
    lever_top = Vector((HW + 0.1, SEAT_Y - 0.06, BED_Z + 0.68))
    m.add(lr.tube([lever_foot, lever_top], 0.016, sides=5, key='ironblack', set_name='MetalWorn', seed=seed + 4))
    town.lathe(m, [(0.0, 0.0), (0.03, 0.02), (0.03, 0.07), (0.0, 0.09)], at=tuple(lever_top), sides=6, look='brass')
    brake_y = REAR - RR - 0.04
    town.box(m, (2.0 * WX + 0.1, 0.07, 0.07), at=(0.0, brake_y, 0.52), look='woodblack')
    for s in (-1.0, 1.0):
        town.box(m, (0.09, 0.07, 0.22), at=(s * WX, brake_y + 0.035, 0.5), rot=(-20.0, 0.0, 0.0), look='woodblack')
    m.add(lr.tube([lever_foot, (HW + 0.08, brake_y, 0.56)], 0.01, sides=4, key='ironblack', set_name='MetalWorn',
                  seed=seed + 5))
    m.section('gear')

    # The wheels: small ones in front (to turn under the bed), big ones behind.
    for y, r in ((FRONT, RF), (REAR, RR)):
        for s in (-1.0, 1.0):
            w = wheel(r, 0.075, 12, seed + 60 + int(y * 10) + int(s * 2), segments=16 if r < 0.45 else 18)
            lp.place(w, (0.0, 0.0, 0.0), (0.0, 0.0, rng.uniform(0.0, 30.0)))       # turned on its axle
            lp.place(w, (s * WX, y, r), (0.0, 90.0 * s, 0.0))                       # the hub cap faces out
            m.add(w)
    m.section('wheels')

    # The shafts, down on the ground in front: two black poles from the front axle with iron tips, a bar across them
    # and the singletree.
    tips = []
    for s in (-1.0, 1.0):
        hinge = Vector((s * 0.42, FRONT - 0.06, RF + 0.02))
        bend = Vector((s * 0.39, FRONT - 1.2, 0.27))
        tip = Vector((s * 0.33, FRONT - 2.92, 0.035))
        town.board(m, hinge, bend + (bend - hinge).normalized() * 0.04, 0.075, 0.06, look='woodblack',
                   face=(s, 0.0, 0.0))
        town.board(m, bend, tip, 0.068, 0.055, look='woodblack', face=(s, 0.0, 0.0))
        d = (tip - bend).normalized()
        town.box(m, (0.075, 0.065, 0.13), matrix=kit.toward(tip - d * 0.12, tip, face=(s, 0.0, 0.0))
                 @ Matrix.Translation((0.06, 0.0, 0.0)) @ Matrix.Rotation(math.radians(90.0), 4, 'Z'), look='iron')
        hook = bend + d * 0.6 + Vector((0.0, 0.0, 0.04))
        town.box(m, (0.03, 0.06, 0.07), at=tuple(hook), look='iron')
        tips.append(tip)
    town.board(m, (-0.42, FRONT - 0.55, RF - 0.02), (0.42, FRONT - 0.55, RF - 0.02), 0.07, 0.06, look='woodblack',
               face=(0.0, -1.0, 0.0))
    town.board(m, (-0.36, FRONT - 0.66, RF + 0.04), (0.36, FRONT - 0.66, RF + 0.04), 0.06, 0.05, look='woodblack',
               face=(0.0, -1.0, 0.0))
    for x in (-0.36, 0.36):
        town.box(m, (0.03, 0.05, 0.05), at=(x, FRONT - 0.68, RF + 0.04), look='iron')
    m.section('shafts')

    m.hull_points([(x, y, z) for x in (-WX - 0.06, WX + 0.06) for y in (-HL - 0.15, HL + 0.06)
                   for z in (0.0, BED_Z + 0.22)])
    m.hull_points([(x, y, z) for x in (-0.55, 0.55) for y in (SEAT_Y - 0.24, SEAT_Y + 0.3)
                   for z in (BED_Z + 0.2, BED_Z + 0.8)])
    # The pivot: the middle of the whole footprint (the shafts' tips to the back rail).
    mid = footprint_middle(m)
    shift(m, -mid)
    obj = m.finish(fallback=100.0, ao_distance=0.6, preview=False)
    # Road dust and mud on the lower wheels and gear.
    lr.darken(obj, lambda p, n: max(0.0, 1.0 - p.z / 0.32) if abs(p.x) > 0.6 or p.z < 0.12 else 0.0, strength=0.35)
    return obj


# --- The fallen pine ---

def fallen_pine(name, seed):
    m = Model(name, seed=seed)
    rng = m.rng
    # The stump: a flared foot with five root buttresses running into the ground, snapped off 0.5 m up, the wood that
    # tore last standing up in long splinters on the side the tree fell away from.
    sx, sy = -3.35, 0.32
    s_pts = [(sx, sy, -0.18), (sx, sy, -0.02), (sx, sy, 0.1), (sx, sy, 0.26), (sx, sy, 0.44)]
    phase = rng.uniform(0.0, 2.0 * math.pi)

    def roots(i, a):
        near = (1.0, 0.85, 0.45, 0.12, 0.0)[i]
        lobe = max(0.0, math.cos(5 * a + phase)) ** 3
        return 1.0 + 0.7 * near * (0.3 + 0.7 * lobe)
    s_rings, s_frames = bark_tube(m, s_pts, [0.27, 0.27, 0.265, 0.26, 0.255], 12, 'BarkOak', seam=(0.0, 1.0, 0.0),
                                  rough=0.012, seed=seed + 1, flare=roots)
    # The tree fell toward +X: the splinters stand on the stump's far side from it.
    _, t, side, other = s_frames[-1]
    away = Vector((-1.0, 0.0, 0.0))
    spike_a = math.atan2(away.dot(other), away.dot(side))
    break_cap(m, s_rings[-1], s_pts[-1], (0.0, 0.0, 1.0), 0.255, seed + 2,
              spikes=[(spike_a, 0.42), (spike_a + 0.6, 0.26)], reach=1.2)
    m.section('stump')

    # The trunk: 7 m lying along +X from its splintered butt beside the stump to its snapped top, tapering 0.23 to
    # 0.075 m, a little bowed, its butt propped up on the broken wood and the trunk settled into the ground beyond.
    LENGTH = 7.0
    butt = Vector((sx + 0.42, sy - 0.22, 0.0))
    top = Vector((sx + 0.42 + LENGTH * 0.995, sy - 0.62, 0.0))
    count = 22
    pts, radii = [], []
    for i in range(count + 1):
        t = i / count
        r = 0.23 + (0.075 - 0.23) * t ** 0.85
        wander = noise.noise(Vector((t * 3.0, 0.5, seed * 0.11)))
        p = butt.lerp(top, t) + Vector((0.0, 0.18 * math.sin(math.pi * t) + 0.1 * wander, 0.0))
        lift = 0.1 * max(0.0, 1.0 - t / 0.22) ** 2
        sink = 0.035 * math.sin(math.pi * min(t / 0.85, 1.0))
        p.z = r + lift - sink
        pts.append(p)
        radii.append(r)
    sides = 10
    trunk_faces = []
    t_rings, t_frames = bark_tube(m, pts, radii, sides, 'BarkOak', seam=(0.0, 0.0, -1.0), rough=0.008, seed=seed + 3,
                                  faces_out=trunk_faces)
    # Dead and down for years: along its sun-baked top the bark has sloughed off in long patches, showing the grey
    # wood under it (the trim's debarked logs, one log's width across the top).
    along = [0.0]
    for a, b in zip(pts, pts[1:]):
        along.append(along[-1] + (b - a).length)
    where = {}
    for i, ring in enumerate(t_rings):
        for k, vert in enumerate(ring):
            where[vert] = (i, k)
    trim_slot = m.slot('trim')
    grain_u = rng.uniform(0.0, 6.4)
    for face, i, k in trunk_faces:
        a_c = 2.0 * math.pi * (k + 0.5) / sides
        t = (i + 0.5) / count
        patch = noise.noise(Vector((t * 11.0, a_c * 1.4, seed * 0.37))) - 0.9 * max(0.0, abs(a_c - math.pi) - 1.0)
        if not 0.08 < t < 0.95 or patch < 0.12:
            continue
        face.material_index = trim_slot
        face.smooth = True
        for loop in face.loops:
            vi, vk = where[loop.vert]
            if vk == 0 and k == sides - 1:          # the wrap face's far side
                vk = sides
            # Across: the top half of the trunk (the patches' widest reach) spread over one log of the strip.
            turn = 2.0 * math.pi * vk / sides - math.pi
            loop[m.uv].uv = ((along[vi] + grain_u) * TRIM_SCALE, log_v(0, 0.16 + turn / (0.5 * math.pi) * 0.155))
    direction = (pts[1] - pts[0]).normalized()
    break_cap(m, t_rings[0], pts[0], -direction, radii[0], seed + 4, spikes=[(rng.uniform(0.0, 6.28), 0.34)],
              reach=1.5)
    end = (pts[-1] - pts[-2]).normalized()
    break_cap(m, t_rings[-1], pts[-1], end, radii[-1], seed + 5, reach=1.8)
    m.section('trunk')

    # Branch stubs in whorls up the trunk: short dead ones low down, longer broken ones toward the top; those that
    # would point into the ground snapped off when it fell.
    k = 0
    for w in range(9):
        t = 0.22 + w * 0.085 + rng.uniform(-0.015, 0.015)
        i = min(int(t * count), count - 1)
        f = t * count - i
        center = pts[i].lerp(pts[i + 1], f)
        r = radii[i] + (radii[i + 1] - radii[i]) * f
        _, tangent, side, other = t_frames[i]
        turn = rng.uniform(0.0, 2.0 * math.pi)
        for j in range(rng.choice((3, 4, 4))):
            a = turn + 2.0 * math.pi * j / 4 + rng.uniform(-0.3, 0.3)
            radial = side * math.cos(a) + other * math.sin(a)
            d = (radial + tangent * 0.55).normalized()
            if d.z < -0.3:
                continue
            length = rng.uniform(0.14, 0.24) + 0.32 * t * rng.uniform(0.4, 1.0)
            base_r = rng.uniform(0.028, 0.04) * (1.2 - 0.4 * t)
            start = center + radial * r * 0.5
            tip = start + d * (r * 0.5 + length)
            if tip.z < 0.03:
                tip.z = 0.03
            mid_p = start.lerp(tip, 0.55)
            bark_tube(m, [start, mid_p, tip], [base_r, base_r * 0.75, 0.0], 5, 'BarkOak', seam=(0.0, 0.0, -1.0),
                      seed=seed + 20 + k)
            k += 1
    # Three heavier limbs, snapped off short when the tree came down, their ends splintered.
    for j, (t, a, length) in enumerate(((0.52, 2.2, 0.75), (0.66, 3.9, 0.95), (0.8, 2.9, 0.6))):
        i = int(t * count)
        _, tangent, side, other = t_frames[i]
        radial = side * math.cos(a) + other * math.sin(a)
        d = (radial + tangent * 0.45).normalized()
        r = radii[i] * 0.42
        start = pts[i] + radial * radii[i] * 0.4
        end = start + d * (radii[i] * 0.6 + length)
        l_rings, _ = bark_tube(m, [start, start.lerp(end, 0.5), end], [r, r * 0.88, r * 0.78], 6, 'BarkOak',
                               seam=(0.0, 0.0, -1.0), rough=0.004, seed=seed + 60 + j)
        break_cap(m, l_rings[-1], end, d, r * 0.78, seed + 70 + j, reach=2.2)
    m.section('stubs')
    # Collision: the stump, and the trunk in two hulls (step over its top end, round its butt).
    m.hull_points([v.co.copy() for ring in s_rings[1:] for v in ring])
    half = count // 2
    for lo, hi in ((0, half + 1), (half, count + 1)):
        m.hull_points([v.co.copy() for ring in t_rings[lo:hi] for v in ring])
    mid = footprint_middle(m)
    shift(m, -mid)
    return m.finish(fallback=None, ao_distance=0.7, preview=False, **SMALL)


# --- The Sink's warning sign ---

def strokes(m, pieces, y, key='black'):
    """Flat painted shapes a hair proud of a board face at y (facing -Y): each piece is a convex outline [(x, z), ...]
    counterclockwise seen from the front, mapped at world scale."""
    scale = tile_scale('PaintWorn')
    for outline in pieces:
        cos = [(x, y, z) for x, z in outline]
        uvs = [(x * scale + 0.37, z * scale + 0.61) for x, z in outline]
        put_faces(m, cos, [(list(range(len(outline))), uvs, key)])


def bar(p, q, width):
    """A painted stroke from p to q (x, z) as a quad, counterclockwise seen from the front."""
    p, q = Vector(p), Vector(q)
    d = (q - p).normalized()
    n = Vector((-d.y, d.x)) * (width * 0.5)
    return [tuple(p - n), tuple(q - n), tuple(q + n), tuple(p + n)]


def band(points, width):
    """A painted line along points (x, z) as one strip of quads with mitred joints, counterclockwise seen from the
    front: an outline drawn in one stroke."""
    pts = [Vector(p) for p in points]
    left = []
    for i, p in enumerate(pts):
        d0 = (p - pts[i - 1]).normalized() if i else (pts[1] - p).normalized()
        d1 = (pts[i + 1] - p).normalized() if i < len(pts) - 1 else d0
        d = (d0 + d1).normalized()
        n = Vector((-d.y, d.x))
        scale = 1.0 / max(n.dot(Vector((-d0.y, d0.x))), 0.5)
        left.append(n * (width * 0.5 * scale))
    quads = []
    for i in range(len(pts) - 1):
        a, b = pts[i], pts[i + 1]
        quads.append([tuple(a - left[i]), tuple(b - left[i + 1]), tuple(b + left[i + 1]), tuple(a + left[i])])
    return quads


def skull(m, cx, cz, size, y, rnd):
    """A crude painted skull and crossbones, as a hand with a brush and a hurry would."""
    s = size
    pieces = []
    # The crossbones behind it: two strokes with knobbed ends.
    for a, b in (((-0.5, -0.62), (0.5, 0.0)), ((-0.5, 0.0), (0.5, -0.62))):
        p = (cx + a[0] * s, cz + a[1] * s)
        q = (cx + b[0] * s, cz + b[1] * s)
        pieces.append(bar(p, q, 0.075 * s))
        for e in (p, q):
            for o in (-1.0, 1.0):
                pieces.append([(e[0] - 0.05 * s, e[1] + o * 0.035 * s - 0.03 * s), (e[0] + 0.05 * s, e[1] + o * 0.035 * s
                               - 0.03 * s), (e[0] + 0.05 * s, e[1] + o * 0.035 * s + 0.03 * s),
                               (e[0] - 0.05 * s, e[1] + o * 0.035 * s + 0.03 * s)])
    # The cranium and jaw in one stroke: round the top, down the cheeks, along the jaw line.
    outline = [(cx - 0.2 * s, cz - 0.32 * s)]
    for k in range(10):
        a = math.radians(215.0 - 250.0 * k / 9)
        outline.append((cx + 0.36 * s * math.cos(a), cz + 0.05 * s + 0.39 * s * math.sin(a)))
    outline += [(cx + 0.2 * s, cz - 0.32 * s), (cx - 0.21 * s, cz - 0.33 * s)]
    pieces += band(outline, 0.072 * s)
    # Eyes, the nose, teeth.
    for ex in (-0.15, 0.15):
        e = (cx + ex * s, cz + 0.02 * s)
        pieces.append([(e[0], e[1] - 0.1 * s), (e[0] + 0.09 * s, e[1]), (e[0], e[1] + 0.08 * s), (e[0] - 0.09 * s, e[1])])
    pieces.append([(cx - 0.05 * s, cz - 0.08 * s), (cx + 0.05 * s, cz - 0.08 * s), (cx, cz - 0.17 * s)][::-1])
    for tx in (-0.11, 0.0, 0.11):
        pieces.append(bar((cx + tx * s, cz - 0.24 * s), (cx + tx * s + rnd.uniform(-0.01, 0.01) * s, cz - 0.36 * s),
                          0.035 * s))
    strokes(m, pieces, y)


def sink_warning(name, seed):
    m = Model(name, seed=seed)
    rng = m.rng
    # The post: hewn, its top cut on a slant to shed rain, 1.72 m out of the ground.
    first = len(m.bm.verts)
    m.board((0.0, 0.0, -0.32), (0.0, 0.0, 1.72), 0.11, 0.11, face=(0.0, -1.0, 0.0), uv=Trim('C', lane='each'))
    m.bm.verts.ensure_lookup_table()
    for v in m.bm.verts[first:]:
        if v.co.z > 1.7:
            v.co.z -= 0.05 * (0.5 - v.co.y / 0.11)
    # Two whitewashed planks nailed across its front, the lower one's right end broken off.
    face_y = -0.055 - 0.034
    upper = [(-0.58, 1.29), (0.58, 1.29), (0.58, 1.56), (-0.58, 1.555)]
    # The lower plank split off along the grain at its right end, raggedly, the top edge left longest.
    lower = [(-0.575, 1.005), (0.405, 1.005), (0.43, 1.028), (0.41, 1.062), (0.468, 1.101), (0.398, 1.128),
             (0.44, 1.171), (0.487, 1.198), (0.432, 1.243), (0.452, 1.275), (-0.575, 1.275)]
    for outline in (upper, lower):
        town.prism(m, outline, 0.034, look='band_G', y=face_y)
    # Nailed to the post along each plank's top and bottom edge, clear of the lettering.
    for z in (1.31, 1.535, 1.025, 1.255):
        x = rng.uniform(-0.025, 0.025)
        m.add(lp.nail((x, face_y, z), (0.0, -1.0, 0.0), size=0.017))
    paint_y = face_y - 0.0015
    town.text(m, 'DANGER', (0.12, paint_y + 0.0015, 1.43), height=0.15, width=0.78, look='oxide', rot=-1.5, tol=0.02)
    town.text(m, 'GROUND GIVES WAY', (-0.11, paint_y + 0.0015, 1.195), height=0.058, width=0.84, look='black',
              rot=0.8, tol=0.026)
    town.text(m, 'KEEP OUT', (-0.17, paint_y + 0.0015, 1.072), height=0.055, look='oxide', rot=-1.0, tol=0.026)
    skull(m, -0.42, 1.44, 0.2, paint_y, rng)
    m.section('sign')
    # It leans back and a little to one side, as a post driven into loose ground does.
    lean = kit.place((0.0, 0.0, 0.0), (-3.5, 2.5, 4.0))
    tilt_all(m, lean)
    m.hull_points([lean @ Vector((x, y, z)) for x in (-0.065, 0.065) for y in (-0.065, 0.065) for z in (0.0, 1.72)])
    m.hull_points([lean @ Vector((x, y, z)) for x in (-0.59, 0.59) for y in (face_y - 0.01, face_y + 0.035)
                   for z in (1.0, 1.565)])
    m.socket('Interact', tuple(lean @ Vector((0.0, face_y - 0.05, 1.28))))
    return m.finish(fallback=None, ao_distance=0.4, preview=False, **SMALL)


# --- Previews: the level's golden late afternoon (Farmhouse.py's and TownGate.py's look sheet) ---

GOLDEN_SKY = [(0.0, 0x6b6050), (0.47, 0x8e8270), (0.5, 0xe9d2a6), (0.53, 0xd9d6c4), (0.62, 0xa9bfd2), (1.0, 0x5d84b6)]
GOLDEN_GLOWS = [(0xffe0a8, 24.0, 0.9), (0xfff2d0, 600.0, 6.0)]
SUN_UP = 15.0


def sky(stops, glows):
    """A painted sky: colours by the view's height, glows round the sun (its direction set per shot)."""
    world = bpy.data.worlds.get('_Golden') or bpy.data.worlds.new('_Golden')
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
    dot.name = 'SunDot'
    links.new(coords.outputs['Generated'], dot.inputs[0])
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
    """The renders' set: Eevee with screen-space ray tracing and AgX, a dirt yard, hulls hidden, the golden sun."""
    scene = bpy.context.scene
    scene.render.engine = 'BLENDER_EEVEE_NEXT'
    ee = scene.eevee
    ee.taa_render_samples = 64
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
    mesh = bpy.data.meshes.new('_Ground')
    mesh.from_pydata([(-150.0, -150.0, 0.0), (150.0, -150.0, 0.0), (150.0, 150.0, 0.0), (-150.0, 150.0, 0.0)], [],
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
    scene.world = sky(GOLDEN_SKY, GOLDEN_GLOWS)
    return scene


def aim_sun(scene, eye, target, left=50.0):
    """Points the sun (15 degrees up) from the camera's front left: as if the piece were turned that way to the
    level's sun. Returns the direction toward it."""
    view = Vector(eye) - Vector(target)
    view.z = 0.0
    view.normalize()
    a = math.radians(-left)
    h = Vector((view.x * math.cos(a) - view.y * math.sin(a), view.x * math.sin(a) + view.y * math.cos(a), 0.0))
    toward = (h * math.cos(math.radians(SUN_UP)) + Vector((0.0, 0.0, math.sin(math.radians(SUN_UP))))).normalized()
    sun = bpy.data.objects['_Sun']
    sun.rotation_euler = (-toward).to_track_quat('-Z', 'Y').to_euler()
    scene.world.node_tree.nodes['SunDot'].inputs[1].default_value = toward
    return toward


def shoot(scene, path, eye, target, lens=40.0, size=(1400, 1000), left=50.0):
    aim_sun(scene, eye, target, left)
    cam = bpy.data.objects.get('_Camera') or bpy.data.objects.new('_Camera', bpy.data.cameras.new('_Camera'))
    if cam.name not in scene.collection.objects:
        scene.collection.objects.link(cam)
    cam.location = Vector(eye)
    cam.rotation_euler = (Vector(target) - Vector(eye)).to_track_quat('-Z', 'Y').to_euler()
    cam.data.lens, cam.data.clip_start, cam.data.clip_end = lens, 0.02, 2000.0
    scene.camera = cam
    scene.render.resolution_x, scene.render.resolution_y = size
    scene.render.resolution_percentage = 100
    scene.render.filepath = path
    os.makedirs(os.path.dirname(path), exist_ok=True)
    bpy.ops.render.render(write_still=True)
    lt._log(f'backlots: rendered {path}')
    return path


def figure():
    """A 1.8 m figure for scale: a plain clay mannequin standing at its origin, facing -Y (TownGate.py's)."""
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


def copy_of(obj, at=(0.0, 0.0, 0.0), turn=0.0, name='_Copy'):
    """Another object showing obj's mesh (for staging), removed by clear_copies()."""
    other = bpy.data.objects.new(name, obj.data)
    bpy.context.scene.collection.objects.link(other)
    other.location = at
    other.rotation_euler = (0.0, 0.0, math.radians(turn))
    other['_staged'] = True
    return other


def clear_copies():
    for o in [o for o in bpy.context.scene.objects if o.get('_staged')]:
        bpy.data.objects.remove(o)


def show_only(objects):
    keep = set(objects)
    for o in bpy.context.scene.objects:
        if o.type == 'MESH' and not o.name.startswith(('_Ground',)):
            o.hide_render = o not in keep


def bounds(objects):
    pts = [o.matrix_world @ Vector(c) for o in objects for c in o.bound_box]
    lo = Vector((min(p.x for p in pts), min(p.y for p in pts), min(p.z for p in pts)))
    hi = Vector((max(p.x for p in pts), max(p.y for p in pts), max(p.z for p in pts)))
    return lo, hi


def framed(objects, view, fit=1.0, lens=40.0, size=(1400, 1000), lift=0.0):
    """An eye and target that frame objects from direction view."""
    lo, hi = bounds(objects)
    center = (lo + hi) * 0.5 + Vector((0.0, 0.0, lift))
    radius = (hi - lo).length * 0.5
    half_fov = math.atan(36.0 / (2.0 * lens))
    half_fov = math.atan(math.tan(half_fov) * min(size) / max(size))
    distance = radius * fit / math.sin(half_fov)
    return center + Vector(view).normalized() * distance, center


def import_fbx(path):
    """A model from an export folder, ready to stage: its hulls dropped, its materials the session's own (by name), its
    occlusion in the 'Col' attribute the materials read. Returns its top object, or None."""
    if not os.path.exists(path):
        return None
    before = set(bpy.data.objects)
    bpy.ops.import_scene.fbx(filepath=path)
    new = [o for o in bpy.data.objects if o not in before]
    roots = []
    for o in new:
        if o.name.startswith('UCX_') or o.type != 'MESH':
            if o.type != 'MESH' or o.name.startswith('UCX_'):
                bpy.data.objects.remove(o)
            continue
        roots.append(o)
    for o in roots:
        o.parent = None
        mesh = o.data
        if mesh.color_attributes and 'Col' not in mesh.color_attributes:
            mesh.color_attributes[0].name = 'Col'
        for i, mat in enumerate(mesh.materials):
            base = mat.name.split('.')[0]
            if base in bpy.data.materials and bpy.data.materials[base] is not mat:
                mesh.materials[i] = bpy.data.materials[base]
            elif base == 'HouseTrim':
                mesh.materials[i] = TRIM
        # The import's own turn and scale go into the mesh, so copies sharing it stand the right way up.
        bpy.context.view_layer.update()
        basis = o.matrix_world.copy()
        basis.translation = (0.0, 0.0, 0.0)
        mesh.transform(basis)
        o.matrix_world = Matrix.Identity(4)
        if max(o.dimensions) > 20.0:
            mesh.transform(Matrix.Scale(0.01, 4))
        mesh.update()
    bpy.context.view_layer.update()
    return roots[0] if roots else None


def back_wall():
    """A stand-in for a false front's back wall (FalseFronts.py's shell): weathered siding on a fieldstone plinth, a
    corner board, a water table, a plank back door and a boarded window, under the gable."""
    m = Model('_BackWall', seed=7)
    span, wall_h, pitch = 8.8, 3.8, 24.0
    space = kit.wall_space((-span * 0.5, 0.0), (span * 0.5, 0.0), 0.38)       # facing -Y, the yard
    door = Opening('door', 5.6, 0.0, 1.05, 2.2, casing='C', hinge='left')
    rise = span * 0.5 * math.tan(math.radians(pitch))
    m.panel(kit.wall_outline(span, wall_h, [door], (span * 0.5, wall_h + rise)), (), 0.14, Trim('A', world=True),
            space=space, around=[door])
    kit.openings(m, space, [door], 0.14)
    for x in (0.07, span - 0.07):
        kit.trim_board(m, space, (x, 0.0, -0.05), (x, 0.0, wall_h), 0.15, 'C', thick=0.045)
    kit.trim_board(m, space, (0.12, 0.0, 0.1), (5.4, 0.0, 0.1), 0.2, 'C', thick=0.045)
    kit.trim_board(m, space, (6.85, 0.0, 0.1), (span - 0.12, 0.0, 0.1), 0.2, 'C', thick=0.045)
    kit.plinth(m, -span * 0.5, span * 0.5, 0.0, 6.0, 0.38, out=0.06)
    town.box(m, (1.55, 0.42, 0.19), at=(-span * 0.5 + 5.6 + 0.525, -0.27, 0.1), look='world_D', bevel=0.02)
    return m.finish(ao=False, preview=False)


def previews(models):
    by = {o.name: o for o in models}
    scene = stage()
    person = figure()
    out = lambda name: os.path.join(PREVIEW_DIR, name + '.png')
    # Each piece on its own, the figure beside it.
    singles = {
        'Woodshed': ((-0.85, -1.5, 0.62), (2.05, -1.6), 0.95),
        'LeanTo': ((-0.75, -1.5, 0.5), (1.95, -1.25), 0.95),
        'LumberStack': ((-0.7, -1.5, 0.62), (2.5, -1.2), 0.95),
        'Dray': ((-1.25, -1.0, 0.55), (1.4, -0.9), 0.9),
        'FallenPine': ((-0.55, -1.5, 0.55), (0.6, -1.4), 0.85),
        'SinkWarning': ((-0.5, -1.6, 0.35), (1.0, -0.4), 1.0),
    }
    for name, (view, at, fit) in singles.items():
        if name not in by:
            continue
        obj = by[name]
        person.location = (at[0], at[1], 0.0)
        show_only([obj, person])
        eye, target = framed([obj, person], view, fit)
        shoot(scene, out(name), eye, target)
    # Close three-quarter views of the two hero pieces.
    for name, view, fit, left in (('Dray', (-1.0, -1.4, 0.45), 0.62, 55.0), ('Woodshed', (-0.55, -1.6, 0.28), 0.62, 40.0)):
        if name in by:
            show_only([by[name]])
            eye, target = framed([by[name]], view, fit)
            shoot(scene, out(name + '_close'), eye, target, lens=50.0, left=left)
    if 'Dray' in by:
        show_only([by['Dray']])
        eye, target = framed([by['Dray']], (0.95, 1.2, 0.5), 0.7)
        shoot(scene, out('Dray_back'), eye, target, lens=45.0, left=40.0)
    if 'FallenPine' in by:
        # The snapped end: the stump and the trunk's butt beside it, the bare grey wood along its top.
        show_only([by['FallenPine']])
        lo, hi = bounds([by['FallenPine']])
        target = Vector((lo.x + 1.6, 0.0, 0.3))
        shoot(scene, out('FallenPine_close'), target + Vector((-1.5, -2.6, 1.25)), target, lens=40.0, left=60.0)
    if 'SinkWarning' in by:
        show_only([by['SinkWarning']])
        eye, target = framed([by['SinkWarning']], (-0.15, -1.6, 0.05), 0.75, lift=0.35)
        shoot(scene, out('SinkWarning_close'), eye, target, lens=55.0, left=35.0)

    # The family side by side, in two rows: the yard pieces behind, the obstacles in front.
    staged = []
    y = 0.0
    for row in (('Woodshed', 'LeanTo', 'Dray'), ('LumberStack', 'FallenPine', 'SinkWarning')):
        x = 0.0
        placed = []
        for name in (n for n in row if n in by):
            c = copy_of(by[name], turn=-90.0 if name == 'Dray' else 0.0)
            bpy.context.view_layer.update()
            lo, hi = bounds([c])
            c.location.x += x - lo.x
            c.location.y = y - (lo.y + hi.y) * 0.5
            x += hi.x - lo.x + 1.5
            placed.append(c)
        for c in placed:
            c.location.x -= (x - 1.5) * 0.5
        staged += placed
        y -= 4.2
    person.location = (0.6, -2.3, 0.0)
    bpy.context.view_layer.update()
    show_only(staged + [person])
    eye, target = framed(staged + [person], (-0.12, -1.6, 0.62), 0.58, lens=35.0, size=(1800, 1000))
    shoot(scene, out('Backlots_overview'), eye, target, lens=35.0, size=(1800, 1000), left=45.0)
    try:
        import shutil
        shutil.copyfile(out('Backlots_overview'), os.path.join(os.path.dirname(PREVIEW_DIR), 'Backlots_overview.png'))
    except OSError:
        pass
    clear_copies()

    # The undertaker's yard: the dray drawn up by the lumber stack, coffins from the graves kit stacked beside it.
    if 'Dray' in by and 'LumberStack' in by:
        chapel = os.path.join(lt.REPO, 'Intermediate', 'ArtExport_RR_Chapel')
        coffin = import_fbx(os.path.join(chapel, 'SM_Coffin_Closed.fbx'))
        broken = import_fbx(os.path.join(chapel, 'SM_Coffin_Broken.fbx'))
        dray_copy = copy_of(by['Dray'], (0.0, 0.0, 0.0), 12.0)
        staged = [dray_copy, copy_of(by['LumberStack'], (3.6, 2.4, 0.0), -8.0)]
        if coffin is not None:
            for k, (at, turn, z) in enumerate((((1.6, -1.1), 4.0, 0.0), ((1.62, -1.12), 7.0, 0.435),
                                               ((2.6, -1.25), -3.0, 0.0))):
                staged.append(copy_of(coffin, (at[0], at[1], z), turn + 90.0))
            # One on the dray's load bed (its middle 0.98 m in from the back rail), ready to go.
            bpy.context.view_layer.update()
            back = max(Vector(c).y for c in by['Dray'].bound_box)
            on_bed = dray_copy.matrix_world @ Vector((0.0, back - 0.98, 0.95))
            staged.append(copy_of(coffin, tuple(on_bed), 12.0 + 90.0))
            coffin.hide_render = True
        if broken is not None:
            broken.hide_render = True
        person.location = (2.2, -2.4, 0.0)
        person.rotation_euler = (0.0, 0.0, math.radians(-30.0))
        bpy.context.view_layer.update()
        show_only(staged + [person])
        eye, target = framed(staged + [person], (-0.6, -1.5, 0.48), 0.72, lens=35.0, size=(1600, 1000))
        shoot(scene, out('Vignette_UndertakersYard'), eye, target, lens=35.0, size=(1600, 1000), left=55.0)
        clear_copies()
        person.rotation_euler = (0.0, 0.0, 0.0)

    # A backlot: the lean-to against a false front's back wall beside its back door, the woodshed out in the yard.
    if 'Woodshed' in by and 'LeanTo' in by:
        wall = back_wall()
        wall_y = 0.0
        staged = [wall, copy_of(by['LeanTo'], (-1.7, wall_y - 0.95, 0.0), 0.0),
                  copy_of(by['Woodshed'], (-4.6, -4.4, 0.0), 24.0)]
        person.location = (0.4, -2.3, 0.0)
        bpy.context.view_layer.update()
        show_only(staged + [person])
        eye, target = framed(staged + [person], (-0.3, -1.6, 0.42), 0.62, lens=35.0, size=(1600, 1000))
        shoot(scene, out('Vignette_Backlot'), eye, target, lens=35.0, size=(1600, 1000), left=50.0)
        clear_copies()
        bpy.data.objects.remove(wall)


def report(models):
    for obj in models:
        tris = lp.tri_count(obj)
        hulls = len([c for c in obj.children if c.name.startswith('UCX_')])
        sockets = [c.name[7:].split('.')[0] for c in obj.children if c.name.startswith('SOCKET_')]
        dims = obj.dimensions
        lods = obj.get('LODs')
        lod_text = (f"LODs {lods} ~ {', '.join(str(int(tris * float(p) / 100.0)) for p in lods.split(','))}"
                    if lods else f"Nanite, fallback {obj.get('Fallback', 100)}%")
        print(f'BACKLOTS: {obj.name}: {tris} triangles ({lod_text}), {dims.x:.2f} x {dims.y:.2f} x {dims.z:.2f} m, '
              f'{hulls} hulls, sockets {", ".join(sockets) or "-"}, materials '
              f'{", ".join(m.name for m in obj.data.materials)}', flush=True)


BUILDERS = [
    ('Woodshed', woodshed, (401,)),
    ('LeanTo', lean_to, (411,)),
    ('LumberStack', lumber_stack, (421,)),
    ('Dray', dray, (431,)),
    ('FallenPine', fallen_pine, (441,)),
    ('SinkWarning', sink_warning, (451,)),
]

if __name__ == '__main__':
    ONLY = next((a.split('=', 1)[1].split(',') for a in kit._args() if a.startswith('--only=')), None)
    models = [build(name, *args) for name, build, args in BUILDERS if ONLY is None or name in ONLY]
    report(models)
    if lt.want_preview():
        previews(models)
