"""The Gravemother's headstone (Saved/Gravemother/CONTRACT.md, BARROW-ART; Art/Models/Creatures/GravemotherStone.py
builds the models from this, Gravemother.py's preview borrows it): a toppled granite headstone bound to her abdomen by
roots, armour that breaks in three states.

The slab: an arched headstone, 80 x 116 x 17 cm at the spider's 1x (1.44 x 2.09 m on her at 1.8x), its carved face
toward her head (stone coordinates: looter_barrow's module docstring). It is a flat profile extruded, so it breaks in 2-D:
five cracks (A the long diagonal, B, C and D off it, E a small corner piece) cut the profile into pieces, and each piece is
extruded on its own. Its edges are rounded by the normal map, not the mesh.

  state 0  intact: one solid slab
  state 1  cracked (at 66% of its health): every piece in place, a hair apart (the cracks are open a few mm, their faces
           fresh granite), the cracked texture
  state 2  shattered (at 33%): the pieces shifted and tipped along the cracks, the corner piece gone, a front root
           snapped
  shards   the five pieces of state 2 (without the corner piece), each its own mesh at the stone's pivot: spawned at the
           stone's transform they are state 2, and fall from there
  chips    small flakes for hits (world size, pivot at their middle)

Each state carries the roots that bind it (to where they enter her hide: looter_barrow.ROOTS) and the grave earth packed
round its foot, which fills the gap between the flat slab and her round back.

The texture set GravemotherStone (2048 px, about 8.8 px per cm at 1x) is painted here: the carved face (a worn border,
a cross, illegible lettering, rain streaks, lichen, soil at the foot), the rough back, the dressed edges, each intact and
cracked (the cracks along the pieces' own break lines, chipped lips, bullet chips). Fracture faces are the granite set,
fresh (looter_barrow.stone_break).
"""
import math
import random

import bmesh
import numpy as np
from mathutils import Matrix, Vector, noise

import looter_barrow as lb
import looter_textures as lt

SET = 'GravemotherStone'
TEX = 2048
W, H, T = lb.STONE_W, lb.STONE_H, lb.STONE_T
ARCH = H - W / 2.0
FACE_PX = 704                  # a face region's width (px): 80 cm
COL_PX = 160                   # an edge column's width (px): the 17 cm thickness
COL_CM = H                     # an edge column's length (cm): 1024 px
PX_CM = 1024.0 / H             # px per cm (8.83)
FACE_U = FACE_PX / TEX
# Atlas regions (u0, u1, v0, v1), Blender UVs (v up from the bottom of the texture).
REGIONS = {
    'front0': (0.0, FACE_U, 0.5, 1.0), 'front1': (FACE_U, 2 * FACE_U, 0.5, 1.0), 'edge0': (2 * FACE_U, 1.0, 0.5, 1.0),
    'back0': (0.0, FACE_U, 0.0, 0.5), 'back1': (FACE_U, 2 * FACE_U, 0.0, 0.5), 'edge1': (2 * FACE_U, 1.0, 0.0, 0.5),
}
# Material slots of a stone model.
M_STONE, M_BREAK, M_ROOTS, M_MUD = 0, 1, 2, 3
UV_BREAK = lb.SCALE * 0.01 * 320.0 / 1024.0      # the granite set's repeat, per stone cm (she is 1.8x)
UV_MUD = lb.SCALE * 0.01 * 0.5                    # the earth at its foot (GravemotherEarth): 2 m a repeat
UV_ROOT = lb.ROOT_UV_PER_CM                       # GravemotherRoots: 0.6 m a repeat

# --- The cracks (stone cm, the face plane: x across, z up). Junctions are exact points of the crack they start on;
# ends on the rim are pulled onto it. ---

CRACKS = [
    dict(name='A', pts=[(-36.0, 93.0), (-24.0, 84.0), (-15.0, 71.0), (2.0, 62.0), (10.0, 47.0), (26.0, 37.0), (40.5, 27.0)]),
    dict(name='B', pts=[(2.0, 62.0), (8.0, 75.0), (14.0, 92.0), (22.5, 109.0)]),
    dict(name='C', pts=[(-15.0, 71.0), (-26.0, 59.0), (-33.0, 50.0), (-40.5, 42.0)]),
    dict(name='D', pts=[(10.0, 47.0), (5.0, 31.0), (-2.0, 17.0), (-8.0, -0.5)]),
    dict(name='E', pts=[(-12.0, 114.4), (-16.0, 108.5), (-23.0, 104.0), (-33.5, 98.4)]),
]
# Hairlines in the cracked texture only (not cut).
HAIRLINES = [[(-24.0, 84.0), (-30.0, 76.0), (-34.0, 73.0)], [(26.0, 37.0), (30.0, 24.0), (29.0, 12.0)],
             [(14.0, 92.0), (22.0, 88.0), (29.0, 86.0)], [(5.0, 31.0), (13.0, 26.0), (18.0, 20.0)],
             [(-33.0, 50.0), (-30.0, 38.0), (-31.0, 27.0)]]
# How the pieces move in state 2 (each found by a point inside it): offset (cm), a turn (degrees) about an axis through
# its middle. The corner piece (CORNER inside it, cut off by E) has fallen away by then.
SHIFT = {
    'upper_left': ((-22.0, 94.0), (-1.1, 1.4, -0.9), (0.0, 1.0, 0.25), -3.5),
    'upper_right': ((22.0, 80.0), (1.3, 2.2, -1.2), (0.2, 1.0, -0.3), 4.5),
    'left': ((-35.0, 60.0), (-1.6, 0.6, -0.4), (0.0, 1.0, 0.0), 2.5),
    'lower_left': ((-20.0, 30.0), (-0.4, -0.3, 0.0), (0.0, 1.0, 0.0), -0.8),
    'lower_right': ((25.0, 15.0), (0.8, 0.9, 0.1), (0.0, 1.0, 0.0), -1.5),
}
CORNER = (-22.0, 107.0)
SHARD_ORDER = ('upper_left', 'upper_right', 'left', 'lower_left', 'lower_right')


def jag(pts, seed, amp=1.5, step=2.6):
    """A crack's jagged line through its control points (kept exact: the junctions); the wander tapers to nothing at
    each control point."""
    out = [Vector((pts[0][0], pts[0][1]))]
    for i in range(len(pts) - 1):
        a, b = Vector(pts[i]), Vector(pts[i + 1])
        d = b - a
        n = max(int(d.length / step), 1)
        side = Vector((-d.y, d.x)).normalized()
        for k in range(1, n):
            t = k / n
            o = amp * (noise.noise(Vector((seed * 3.7 + i, t * d.length * 0.21, 0.4)))
                       + 0.45 * noise.noise(Vector((seed * 1.3, t * d.length * 0.9, i + 2.5))))
            out.append(a + d * t + side * o * math.sin(math.pi * t) ** 0.35)
        out.append(b)
    return [(p.x, p.y) for p in out]


# --- The outline, with what its edges need for their texture strip ---

_RIM = []


def rim_outline():
    """The worn outline (looter_barrow.stone_outline) from the right foot corner, counter-clockwise (up the right side,
    over the top, down the left, along the foot back), as dicts: co (x, z), kind ('rim', 'corner' or 'foot') and s, cm
    along the rim from the right corner (the corners carry theirs: 0 and the rim's length). A point is added wherever the
    rim passes from one texture column to the next."""
    if _RIM:
        return [dict(p) for p in _RIM]
    raw = lb.stone_outline()
    n = len(raw)
    start = max((i for i in range(n) if raw[i][1] <= 0.01), key=lambda i: raw[i][0])
    order = raw[start:] + raw[:start]
    out = [dict(co=order[0], kind='corner', s=0.0)]
    s, i = 0.0, 1
    while True:
        p = order[i]
        prev = out[-1]['co']
        seg = math.dist(prev, p)
        while math.floor((s + seg - 1e-6) / COL_CM) > math.floor(s / COL_CM):
            cut = (math.floor(s / COL_CM) + 1) * COL_CM
            t = (cut - s) / seg
            co = (prev[0] + (p[0] - prev[0]) * t, prev[1] + (p[1] - prev[1]) * t)
            out.append(dict(co=co, kind='rim', s=cut))
            seg -= cut - s
            s, prev = cut, co
        s += seg
        i += 1
        if p[1] <= 0.01:
            out.append(dict(co=p, kind='corner', s=s))
            break
        out.append(dict(co=p, kind='rim', s=s))
    for p in order[i:]:
        out.append(dict(co=p, kind='foot', s=0.0))
    _RIM.extend(out)
    return [dict(p) for p in out]


def rim_total():
    return max(p['s'] for p in rim_outline())


# --- Cutting the profile into pieces ---

KINDS = ('crack', 'rim', 'foot', 'corner')


def cut_pieces():
    """The profile cut along the cracks: a list of pieces, each its polygon counter-clockwise in (x, z), as dicts co,
    kind ('crack' for the points on a crack) and s."""
    bm = bmesh.new()
    k_layer = bm.verts.layers.int.new('kind')
    s_layer = bm.verts.layers.float.new('s')
    verts = []
    for p in rim_outline():
        v = bm.verts.new((p['co'][0], p['co'][1], 0.0))
        v[k_layer] = KINDS.index(p['kind'])
        v[s_layer] = p['s']
        verts.append(v)
    bm.faces.new(verts)

    def vert_at(co, tol=1e-3):
        return next((v for v in bm.verts if (v.co.xy - Vector(co)).length < tol), None)

    def on_outline(co):
        """The boundary point nearest co, made a vertex (its edge split there)."""
        best = None
        for e in bm.edges:
            a, b = e.verts
            if not e.is_boundary or a[k_layer] == 0 or b[k_layer] == 0:
                continue
            ab = b.co.xy - a.co.xy
            t = max(0.0, min(1.0, (Vector(co) - a.co.xy).dot(ab) / max(ab.length_squared, 1e-9)))
            d = (a.co.xy + ab * t - Vector(co)).length
            if best is None or d < best[0]:
                best = (d, e, t)
        _, e, t = best
        a, b = e.verts
        if t < 0.03:
            return a
        if t > 0.97:
            return b
        ka, kb = KINDS[a[k_layer]], KINDS[b[k_layer]]
        foot = ka in ('foot', 'corner') and kb in ('foot', 'corner')
        s = 0.0 if foot else a[s_layer] + (b[s_layer] - a[s_layer]) * t
        _, v = bmesh.utils.edge_split(e, a, t)
        v[k_layer] = KINDS.index('foot' if foot else 'rim')
        v[s_layer] = s
        return v

    for i, crack in enumerate(CRACKS):
        line = jag(crack['pts'], seed=i + 1)
        start = vert_at(line[0]) or on_outline(line[0])
        end = vert_at(line[-1]) or on_outline(line[-1])
        face = next(f for f in start.link_faces if f in end.link_faces)
        before = set(bm.verts)
        bmesh.utils.face_split(face, start, end, coords=[Vector((x, z, 0.0)) for x, z in line[1:-1]])
        # The split's new points take their layers from the face's: they're the crack's.
        for v in set(bm.verts) - before:
            v[k_layer] = 0
            v[s_layer] = 0.0
    pieces = []
    for f in bm.faces:
        poly = [dict(co=(v.co.x, v.co.y), kind=KINDS[v[k_layer]], s=v[s_layer]) for v in f.verts]
        area = 0.5 * sum(a['co'][0] * b['co'][1] - b['co'][0] * a['co'][1] for a, b in zip(poly, poly[1:] + poly[:1]))
        if area < 0:
            poly.reverse()
        pieces.append(poly)
    bm.free()
    return pieces


def inside(poly, pt):
    x, z = pt
    hit = False
    for a, b in zip(poly, poly[1:] + poly[:1]):
        (x0, z0), (x1, z1) = a['co'], b['co']
        if (z0 > z) != (z1 > z) and x < x0 + (z - z0) * (x1 - x0) / (z1 - z0):
            hit = not hit
    return hit


def shrink(poly, gap):
    """A piece pulled in by gap (cm) along its cracks, so neighbours stand a crack apart: crack points move inward
    (left of the counter-clockwise edges), rim points next to a crack slide away from it along the rim. Each point
    keeps where it was (orig): its texture stays put."""
    n = len(poly)
    out = []
    for i, p in enumerate(poly):
        a, b = poly[i - 1], poly[(i + 1) % n]
        co = Vector(p['co'])
        if p['kind'] == 'crack':
            e1, e2 = co - Vector(a['co']), Vector(b['co']) - co
            inward = Vector((-e1.y, e1.x)).normalized() + Vector((-e2.y, e2.x)).normalized()
            if inward.length > 1e-6:
                co = co + inward.normalized() * gap
        else:
            for q, r in ((a, b), (b, a)):
                if q['kind'] == 'crack' and r['kind'] != 'crack':
                    d = Vector(r['co']) - co
                    if d.length > 1e-6:
                        co = co + d.normalized() * min(gap * 1.4, d.length * 0.4)
        out.append(dict(p, co=(co.x, co.y), orig=p.get('orig', p['co'])))
    return out


# --- Building a piece (or the whole slab) in 3-D ---

def face_uv(region, x, z, mirror=False):
    u0, u1, v0, v1 = REGIONS[region]
    fx = (W / 2.0 - x) / W if mirror else (x + W / 2.0) / W
    return (u0 + fx * (u1 - u0), v0 + z / H * (v1 - v0))


def edge_uv(region, col, along_cm, y):
    """A point of the edge strip: column col (0-2 the rim, 3 the foot), along_cm from the column's start, y across."""
    u0, u1, v0, v1 = REGIONS[region]
    cu0 = u0 + col * COL_PX / TEX
    return (cu0 + (y + T / 2.0) / T * COL_PX / TEX, v0 + max(0.0, min(1.0, along_cm / COL_CM)) * (v1 - v0))


def extrude(bm, poly, variant, move=None):
    """A piece's slab into bm: its front (+y) and back faces, its rim and foot sides (the stone's texture), its crack
    sides (fresh granite). move: a 4x4 placing it (state 2). Returns its vertices."""
    uv = bm.loops.layers.uv.active
    front, back = [], []
    origs = [p.get('orig', p['co']) for p in poly]
    for p, (ox, oz) in zip(poly, origs):
        x, z = p['co']
        dx = dz = 0.0
        if p['kind'] == 'crack':
            # The break isn't square through the slab: its back line wanders a little off the front's (the same for
            # both pieces of a crack: the noise reads where the point was).
            dx = 0.6 * noise.noise(Vector((ox * 0.21, oz * 0.21, 7.7)))
            dz = 0.6 * noise.noise(Vector((ox * 0.21, oz * 0.21, 3.1)))
        f = Vector((x, T / 2.0, z))
        b = Vector((x + dx, -T / 2.0, z + dz))
        if move is not None:
            f, b = move @ f, move @ b
        front.append(bm.verts.new(f))
        back.append(bm.verts.new(b))
    faces = []
    # The polygon is counter-clockwise in (x, z): as a face that's a -y normal, the back's; the front is reversed.
    fr = bm.faces.new(list(reversed(front)))
    for loop, o in zip(fr.loops, list(reversed(origs))):
        loop[uv].uv = face_uv('front' + variant, *o)
    bk = bm.faces.new(back)
    for loop, o in zip(bk.loops, origs):
        loop[uv].uv = face_uv('back' + variant, *o, mirror=True)
    for face in (fr, bk):
        face.material_index = M_STONE
        faces.append(face)
    n = len(poly)
    total = rim_total()
    for i in range(n):
        j = (i + 1) % n
        a, b = poly[i], poly[j]
        side = bm.faces.new((front[i], front[j], back[j], back[i]))       # faces out of a counter-clockwise outline
        faces.append(side)
        if 'crack' in (a['kind'], b['kind']):
            side.material_index = M_BREAK
            continue
        side.material_index = M_STONE
        ev = 'edge' + variant
        if a['kind'] in ('foot', 'corner') and b['kind'] in ('foot', 'corner'):
            sa, sb = origs[i][0] + W / 2.0, origs[j][0] + W / 2.0
            col, base = 3, 0.0
        else:
            sa, sb = a['s'], b['s']
            if a['kind'] == 'corner':
                sa = 0.0 if origs[i][0] > 0 else total
            if b['kind'] == 'corner':
                sb = 0.0 if origs[j][0] > 0 else total
            col = min(int(min(sa, sb) // COL_CM), 2)
            base = col * COL_CM
        coords = (edge_uv(ev, col, sa - base, T / 2.0), edge_uv(ev, col, sb - base, T / 2.0),
                  edge_uv(ev, col, sb - base, -T / 2.0), edge_uv(ev, col, sa - base, -T / 2.0))
        for loop, c in zip(side.loops, coords):
            loop[uv].uv = c
    return front + back


def piece_name(poly):
    if inside(poly, CORNER):
        return 'corner'
    for key, (pt, *_rest) in SHIFT.items():
        if inside(poly, pt):
            return key
    return None


def piece_move(key, poly):
    """State 2's move of a piece: about its middle, turned and shifted (stone cm)."""
    _, offset, axis, deg = SHIFT[key]
    xs = [p['co'][0] for p in poly]
    zs = [p['co'][1] for p in poly]
    c = Vector((sum(xs) / len(xs), 0.0, sum(zs) / len(zs)))
    return (Matrix.Translation(c + Vector(offset)) @ Matrix.Rotation(math.radians(deg), 4, Vector(axis).normalized())
            @ Matrix.Translation(-c))


def slab(bm, state, only=None):
    """The slab in a state (0, 1 or 2) into bm. only: just that piece of state 2 (a shard). Returns
    {piece name: its vertices}."""
    if state == 0:
        return {'slab': extrude(bm, rim_outline(), '0')}
    out = {}
    for poly in cut_pieces():
        key = piece_name(poly)
        if key is None:
            raise RuntimeError('a piece of the headstone matches no SHIFT point: check CRACKS against SHIFT')
        if state == 2 and key == 'corner':
            continue
        if only is not None and key != only:
            continue
        gap = 0.3 if state == 1 else 0.6
        move = piece_move(key, poly) if state == 2 else None
        out[key] = extrude(bm, shrink(poly, gap), '1', move)
    return out


# --- Roots and the earth at the foot (routed in creature cm, built in the stone's frame) ---

def root_paths(S, snapped=()):
    """Each root's path in stone cm and its radii: over the stone, then down onto her back to where it enters her
    hide. A snapped root (state 2) comes as two pieces with a gap, their ends splintered."""
    F = lb.stone_frame(S)
    Finv = F.inverted()
    ab = lb.Abdomen(S)
    outline = lb.stone_outline()
    paths = []
    for root in lb.ROOTS:
        r0, r1 = root['r0'], root['r1']
        n = len(root['path'])
        pts, radii = [], []
        for k, (x, side, z) in enumerate(root['path']):
            r = r0 + (r1 - r0) * 0.4 * k / max(n - 1, 1)
            if side:
                pts.append(Vector((x, side * (T / 2.0 + r * 0.7), z)))
            else:
                ox, oz = min(outline, key=lambda p: (p[0] - x) ** 2 + (p[1] - z) ** 2)
                c = Vector((0.0, ARCH)) if oz > ARCH else Vector((0.0, oz))
                out = (Vector((ox, oz)) - c).normalized()
                pts.append(Vector((ox + out.x * r * 0.7, 0.0, oz + out.y * r * 0.7)))
            radii.append(r)
        # Down onto her back: from its last point on the stone over her hide, then diving into her along the line its
        # stub keeps (looter_barrow.root_dive).
        last = F @ pts[-1]
        anchor, dive = lb.root_dive(S, root)
        # From the stone it curves down to its dive and goes into her straight along it (the stub's line): a quadratic
        # curve to the dive's top, its middle point on the dive line, never above where it left the stone (no hooks).
        top = anchor + dive * lb.ROOT_DIVE_CM
        k = max(2.0, min(5.0, (last.z - top.z) / max(dive.z, 0.1)))
        mid = top + dive * k
        steps = 6
        for j in range(1, steps + 1):
            t = j / steps
            r = r0 + (r1 - r0) * (0.4 + 0.5 * t)
            p = last * (1.0 - t) ** 2 + mid * (2.0 * (1.0 - t) * t) + top * t * t
            q, nq = ab.point(*ab.coords(p), 1.0)
            if (p - q).dot(nq) < r * 0.9:
                p = q + nq * r * 0.9
            pts.append(Finv @ p)
            radii.append(r)
        pts.append(Finv @ (anchor - dive * 2.5))
        radii.append(r1)
        # Its far end tapers to a fine tip gripping the stone (a cut end there read as a post).
        radii[0] = r0 * 0.25
        pts, radii = smooth_path(pts, radii, 2, 3.2)
        if root['name'] in snapped:
            cut = int(len(pts) * 0.45)
            paths.append((root['name'] + 'Top', pts[:cut], radii[:cut], (False, True)))
            paths.append((root['name'] + 'Low', pts[cut + 2:], radii[cut + 2:], (True, False)))
        else:
            paths.append((root['name'], pts, radii, ()))
    return paths


def smooth_path(pts, radii, rounds, spacing):
    """Chaikin's corner cutting (its ends kept), then points every spacing cm; the radii follow."""
    pts = [Vector(p) for p in pts]
    for _ in range(rounds):
        np_, nr = [pts[0]], [radii[0]]
        for (a, ra), (b, rb) in zip(zip(pts, radii), zip(pts[1:], radii[1:])):
            np_ += [a.lerp(b, 0.25), a.lerp(b, 0.75)]
            nr += [ra * 0.75 + rb * 0.25, ra * 0.25 + rb * 0.75]
        np_.append(pts[-1])
        nr.append(radii[-1])
        pts, radii = np_, nr
    dist = [0.0]
    for a, b in zip(pts, pts[1:]):
        dist.append(dist[-1] + (b - a).length)
    count = max(int(dist[-1] / spacing), 2)
    out_p, out_r, j = [], [], 0
    for k in range(count + 1):
        d = dist[-1] * k / count
        while j < len(dist) - 2 and dist[j + 1] < d:
            j += 1
        t = (d - dist[j]) / max(dist[j + 1] - dist[j], 1e-9)
        out_p.append(pts[j].lerp(pts[j + 1], t))
        out_r.append(radii[j] + (radii[j + 1] - radii[j]) * t)
    return out_p, out_r


def roots_into(bm, S, snapped=(), seed=5):
    """The roots, a few rootlets off each, splinters at a snapped root's ends (stone cm)."""
    rng = random.Random(seed)
    for i, (name, pts, radii, broken) in enumerate(root_paths(S, snapped)):
        lb.tube(bm, pts, radii, mat=M_ROOTS, sides=7, uv_per=UV_ROOT, wobble=0.16, seed=i)
        for end, at in ((0, 0), (1, -1)):
            if broken and broken[end]:
                tip, prev = pts[at], pts[at + 1 if at == 0 else at - 1]
                d = (tip - prev).normalized()
                for k in range(4):
                    jitter = Vector((rng.uniform(-1, 1), rng.uniform(-1, 1), rng.uniform(-1, 1))) * 0.45
                    lb.tube(bm, [tip - d * 0.6, tip + (d + jitter).normalized() * rng.uniform(1.4, 3.2)],
                            [radii[at] * 0.42, 0.12], mat=M_ROOTS, sides=4, uv_per=UV_ROOT)
        # Rootlets: short thin feelers off it, gripping the stone or her hide.
        for k in range(3):
            j = rng.randint(2, len(pts) - 3)
            t = (pts[j + 1] - pts[j - 1]).normalized()
            side = t.cross(Vector((rng.uniform(-1, 1), rng.uniform(-1, 1), rng.uniform(-1, 1)))).normalized()
            mid = pts[j] + side * radii[j] * 1.3 + t * rng.uniform(1.0, 3.0)
            tip = mid + side * rng.uniform(1.5, 3.5) + t * rng.uniform(1.0, 4.0)
            lb.tube(bm, [pts[j], mid, tip], [radii[j] * 0.34, radii[j] * 0.2, 0.1], mat=M_ROOTS, sides=5,
                    uv_per=UV_ROOT)


def ab_inside(ab, p):
    """< 1 inside the abdomen's egg (Spider.py's deform undone), > 1 outside."""
    rel = Vector(p) - ab.center
    rx, ry, rz = ab.radii
    a, Y, Z = rel.dot(ab.fwd) / rx, rel.dot(ab.left) / ry, rel.dot(ab.up) / rz
    front = 1.0 - 0.12 * max(a, 0.0)
    s = Y / front
    u = Z / (front * (0.8 if Z < 0.0 else 1.0))
    return a * a + s * s + u * u


def foot_loop(offset=2.6):
    """Points round the slab's foot (stone x, y), a little out from it: along the front, round the right end, back
    along the back, round the left end."""
    ty = T / 2.0 + offset
    pts = [(-W / 2.0 + W * k / 20, ty) for k in range(21)]
    pts += [(W / 2.0 + math.sin(math.pi * k / 6) * offset, math.cos(math.pi * k / 6) * ty) for k in range(1, 6)]
    pts += [(W / 2.0 - W * k / 20, -ty) for k in range(21)]
    pts += [(-W / 2.0 - math.sin(math.pi * k / 6) * offset, -math.cos(math.pi * k / 6) * ty) for k in range(1, 6)]
    return pts


def mud_collar(bm, S, seed=11):
    """Grave earth packed round the stone's foot, filling the gap between the flat slab and her round back: a lumpy ring
    hugging the slab where it meets her hide (deeper where the slab stands off her back), clods on it, smears up the
    slab."""
    F = lb.stone_frame(S)
    Finv = F.inverted()
    ab = lb.Abdomen(S)
    rng = random.Random(seed)
    ring, radii, squash = [], [], []
    for x, y in foot_loop():
        # Where the slab's foot leaves her hide (z up the stone), or, where it stands off her back, the gap under it.
        z = -20.0
        while z < 30.0 and ab_inside(ab, F @ Vector((x, y, z))) < 1.0:
            z += 0.5
        z = max(z, 0.0)
        p = F @ Vector((x, y, z))
        gap = 0.0
        while gap < 30.0 and ab_inside(ab, p - Vector((0.0, 0.0, gap))) > 1.0:
            gap += 0.5
        r = 3.0 + 0.8 * noise.noise(Vector((x * 0.13, y * 0.4, 0.5))) + gap * 0.5
        ring.append(Finv @ (p - Vector((0.0, 0.0, gap * 0.5 - 0.8))))
        radii.append(r)
        squash.append((1.0 + gap / max(r, 1.0) * 0.4, 1.0))
    ring.append(ring[0])
    radii.append(radii[0])
    squash.append(squash[0])
    lb.tube(bm, ring, radii, mat=M_MUD, sides=10, uv_per=UV_MUD, wobble=0.3, seed=seed, caps=(False, False),
            squash=lambda k: squash[k])
    for k in range(16):
        i = rng.randrange(len(ring) - 1)
        c = ring[i] + Vector((rng.uniform(-2, 2), rng.uniform(-2, 2), radii[i] * rng.uniform(0.3, 0.9)))
        r = rng.uniform(2.2, 4.5)
        lb.blob(bm, c, (r * 1.3, r, r * 0.75), seed=seed + k, lump=0.35, subdiv=2, mat=M_MUD)
    for k in range(7):
        x = rng.uniform(-W / 2.0 + 6.0, W / 2.0 - 6.0)
        side = rng.choice((-1, 1))
        c = Vector((x, side * (T / 2.0 + 0.3), rng.uniform(5.0, 12.0)))
        lb.blob(bm, c, (rng.uniform(4, 8), 0.9, rng.uniform(3, 6)), seed=seed + 40 + k, lump=0.4, subdiv=2, mat=M_MUD)


def finish_uvs(bm):
    """Tiling cube UVs on the fracture faces (granite) and the earth (its set); every face of more than four sides cut
    into triangles (Unreal's importer fans an n-gon, and a fan across a piece's notch would cross its edge)."""
    # Points a piece's shrink slid onto their neighbours leave faces without area: gone first.
    bmesh.ops.dissolve_degenerate(bm, dist=1e-3, edges=bm.edges[:])
    bm.normal_update()
    lb.box_uv_faces(bm, [f for f in bm.faces if f.material_index == M_BREAK], UV_BREAK)
    lb.box_uv_faces(bm, [f for f in bm.faces if f.material_index == M_MUD], UV_MUD)
    bmesh.ops.triangulate(bm, faces=[f for f in bm.faces if len(f.verts) > 4], quad_method='BEAUTY',
                          ngon_method='EAR_CLIP')


def hull_points(margin=0.6):
    """The slab's simple collision: the unworn profile a little outside the worn one, both faces (stone cm)."""
    pts = []
    for k in range(9):
        z = ARCH * k / 8.0
        for x in (-W / 2.0 - margin, W / 2.0 + margin):
            pts.append((x, z))
    for k in range(1, 16):
        a = math.pi * k / 16.0
        r = W / 2.0 + margin
        pts.append((math.cos(a) * r, ARCH + math.sin(a) * r))
    return [Vector((x, y, z)) for x, z in pts for y in (-T / 2.0 - margin - 1.5, T / 2.0 + margin + 1.5)]


# --- The texture: GravemotherStone ---

def _seg_dist(px, py, a, b):
    ax, ay = a
    bx, by = b
    vx, vy = bx - ax, by - ay
    t = np.clip(((px - ax) * vx + (py - ay) * vy) / max(vx * vx + vy * vy, 1e-9), 0.0, 1.0)
    return np.hypot(px - ax - t * vx, py - ay - t * vy)


def _poly_dist(px, py, pts):
    d = np.full(px.shape, 1e9, np.float32)
    for a, b in zip(pts, pts[1:]):
        d = np.minimum(d, _seg_dist(px, py, a, b))
    return d


def _letters(px, py, rng):
    """Worn lettering: rows of letter-like strokes (no real words), each groove's distance (cm)."""
    d = np.full(px.shape, 1e9, np.float32)
    rows = [(80.0, 42.0, 4.4), (71.0, 52.0, 4.0), (62.0, 30.0, 4.0), (49.5, 46.0, 3.2), (42.0, 34.0, 3.2)]
    for z0, width, h in rows:
        x = -width / 2.0
        while x < width / 2.0 - 2.0:
            w = h * rng.uniform(0.55, 0.85)
            shape = rng.randrange(7)
            strokes = {
                0: [((0, 0), (0, 1)), ((0, 1), (1, 1)), ((0, 0.5), (0.8, 0.5))],
                1: [((0, 0), (0.5, 1)), ((0.5, 1), (1, 0)), ((0.25, 0.45), (0.75, 0.45))],
                2: [((0, 0), (0, 1)), ((1, 0), (1, 1)), ((0, 0.5), (1, 0.5))],
                3: [((0.5, 0), (0.5, 1)), ((0, 1), (1, 1))],
                4: [((0, 0), (0, 1)), ((0, 0), (1, 0))],
                5: [((0, 1), (1, 1)), ((1, 1), (0, 0)), ((0, 0), (1, 0))],
                6: [((0, 0), (0, 1)), ((0, 1), (1, 0.5)), ((1, 0.5), (0, 0.5))],
            }[shape]
            for (ax, ay), (bx, by) in strokes:
                if rng.random() < 0.18:                    # worn away
                    continue
                d = np.minimum(d, _seg_dist(px, py, (x + ax * w, z0 + ay * h), (x + bx * w, z0 + by * h)))
            x += w + rng.uniform(0.9, 1.6) + (rng.uniform(1.5, 3.0) if rng.random() < 0.15 else 0.0)
    return d


def _face_xz(region):
    """The stone (x, z) of every texel of a face region (rows from the top of the texture)."""
    u0, u1, v0, v1 = REGIONS[region]
    cols = int(round((u1 - u0) * TEX))
    rows = int(round((v1 - v0) * TEX))
    x = (np.arange(cols, dtype=np.float32) + 0.5) / cols * W - W / 2.0
    z = H - (np.arange(rows, dtype=np.float32) + 0.5) / rows * H
    return np.meshgrid(x, z)


def _granite(shape, seed, finish):
    """Granite's speckle and its weathering (sRGB colour, height in m). finish: 'polished' (the carved face),
    'dressed' (the chiselled edges: fine, low relief), 'pitched' (the rough back)."""
    g = lt.worley(shape, (int(shape[1] / 3.2), int(shape[0] / 3.2)), seed, jitter=1.0)
    kind = lt.per_cell(g['id'], g['count'], seed + 1)
    base = lt.mix(lt.rgb(0x84817b), lt.rgb(0x8d887f), lt.smooth(-1.0, 1.2, lt.noise(shape, seed + 2, 60.0, octaves=2)))
    color = lt.mix(base, lt.rgb(0x47443f), (kind < 0.12).astype(np.float32) * 0.5)
    color = lt.mix(color, lt.rgb(0xc8c4bb), (kind > 0.9).astype(np.float32) * 0.35)
    color = lt.mix(color, lt.rgb(0xa39488), ((kind > 0.74) & (kind <= 0.9)).astype(np.float32) * 0.25)
    fine = lt.noise(shape, seed + 3, 0.7)
    color = color * (1.0 + 0.05 * fine)[..., None]
    knobs = lt.noise(shape, seed + 4, 14.0 if finish == "pitched" else 30.0, octaves=3)
    relief = {"polished": 0.0006, "dressed": 0.0009, "pitched": 0.003}[finish]
    height = relief * knobs + 0.00015 * fine + 0.0001 * (kind > 0.88)
    if finish == "pitched":
        height += 0.0015 * lt.noise(shape, seed + 5, 6.0, octaves=2)
    return color, height, fine


def _lichen(shape, seed, amount):
    x, y = lt.coords(shape)
    wx, wy = x + 10.0 * lt.noise(shape, seed, 9.0), y + 10.0 * lt.noise(shape, seed + 1, 9.0)
    ros = lt.worley(shape, (14, 20), seed + 2, x=wx, y=wy)
    size = lt.per_cell(ros['id'], ros['count'], seed + 3, 7.0, 30.0)
    alive = lt.per_cell(ros['id'], ros['count'], seed + 4) < amount
    return lt.smooth(1.0, 0.75, ros['f1'] / size) * alive * lt.smooth(-0.6, 0.6, lt.noise(shape, seed + 5, 2.2))


def _paint_face(region, front, cracked):
    xs, zs = _face_xz(region)
    shape = xs.shape
    if not front:
        xs = -xs                                                  # the back's texture is mirrored (see face_uv)
    outline = [tuple(p) for p in lb.stone_outline()]
    inside_d = _poly_dist(xs, zs, outline + outline[:1])          # cm to the worn edge
    seed = 500 + (0 if front else 50) + (7 if cracked else 0)
    color, height, fine = _granite(shape, 500 + (0 if front else 50), "polished" if front else "pitched")
    rng = random.Random(77)
    groove = np.zeros(shape, np.float32)
    if front:
        # The carving: a border groove inset from the edge, a cross in the arch, worn lettering, a rule under it.
        inset = 6.5
        bd = np.abs(inside_d - inset)
        bd = np.where(zs > 20.0, bd, 1e9)
        groove = np.maximum(groove, lt.smooth(0.75, 0.25, bd))
        cross = np.minimum(_seg_dist(xs, zs, (0.0, 89.0), (0.0, 105.0)), _seg_dist(xs, zs, (-5.5, 99.5), (5.5, 99.5)))
        groove = np.maximum(groove, lt.smooth(1.0, 0.45, cross))
        letters = _letters(xs, zs, rng)
        groove = np.maximum(groove, lt.smooth(0.55, 0.2, letters) * lt.smooth(-0.8, 0.2, lt.noise(shape, 601, 18.0)))
        rule = _seg_dist(xs, zs, (-14.0, 55.5), (14.0, 55.5))
        groove = np.maximum(groove, lt.smooth(0.5, 0.2, rule) * 0.85)
        groove *= (0.75 + 0.25 * lt.smooth(-1.0, 1.0, lt.noise(shape, 602, 6.0)))
        height -= 0.0018 * groove
        color = lt.mix(color, lt.rgb(0x5b5852), groove * 0.55)
        # Dirt and moss settled in the grooves' bottoms.
        color = lt.mix(color, lt.rgb(0x5f6046), groove * lt.smooth(0.2, 1.0, lt.noise(shape, 603, 8.0)) * 0.3)
    # Rain streaks running down from the top, the face darker toward its foot where soil splashed it.
    streak = lt.smooth(-0.2, 1.3, lt.noise(shape, seed + 10, 4.0, 90.0, octaves=2)) * lt.smooth(20.0, 95.0, zs)
    color = lt.mix(color, lt.rgb(0x625f59), streak * 0.25)
    soil = lt.smooth(26.0, 2.0, zs + 6.0 * lt.noise(shape, seed + 11, 20.0))
    color = lt.mix(color, lt.rgb(0x6d5b47), soil * 0.7)
    color = lt.mix(color, lt.rgb(0x8d7a60), soil * lt.specks(shape, seed + 12, 0.2, 2.0) * 0.6)
    height += 0.0008 * soil * lt.noise(shape, seed + 13, 3.0)
    # Lichen: grey-green rosettes, a few orange specks; more on the back.
    lichen = _lichen(shape, seed + 20, 0.22 if front else 0.5) * lt.smooth(15.0, 40.0, zs)
    color = lt.mix(color, lt.mix(lt.rgb(0x868871), lt.rgb(0x989680), lt.smooth(-1, 1, lt.noise(shape, seed + 21, 3.0))),
                   lichen * 0.42)
    color = lt.mix(color, lt.rgb(0xb98a4a), lt.specks(shape, seed + 22, 0.006, 1.6) * 0.75 * lt.smooth(20.0, 50.0, zs))
    height += 0.0005 * lichen
    # The worn edge rounded off: the face falls away over its last 1.5 cm.
    round_off = lt.smooth(1.6, 0.0, inside_d)
    height -= 0.004 * round_off ** 2
    color = lt.mix(color, lt.rgb(0xa7a298), round_off * 0.25)
    if cracked:
        color, height = _cracks_on(color, height, xs, zs, shape, seed + 30, chips=16 if front else 8)
    rough = np.full(shape, 0.66 if front else 0.86, np.float32) + 0.05 * fine + 0.2 * groove + 0.12 * lichen + 0.15 * soil
    return color, height, rough


def _cracks_on(color, height, xs, zs, shape, seed, chips):
    """The pieces' break lines as open cracks: a dark core, chipped paler lips, hairlines; and bullet chips."""
    d = np.full(shape, 1e9, np.float32)
    for i, crack in enumerate(CRACKS):
        d = np.minimum(d, _poly_dist(xs, zs, jag(crack['pts'], seed=i + 1)))
    hair = np.full(shape, 1e9, np.float32)
    for i, line in enumerate(HAIRLINES):
        hair = np.minimum(hair, _poly_dist(xs, zs, jag(line, seed=20 + i, amp=0.8, step=1.6)))
    wobble = 0.25 * lt.noise(shape, seed, 6.0)
    core = lt.smooth(0.7, 0.25, d + wobble * 0.5)
    lip = lt.smooth(2.4, 0.6, d + wobble * 2.0) * (1.0 - core) * lt.smooth(-0.8, 0.4, lt.noise(shape, seed + 1, 5.0))
    hairline = lt.smooth(0.32, 0.08, hair + wobble * 0.3)
    color = lt.mix(color, lt.rgb(0xb7b1a6), lip * 0.6)
    color = lt.mix(color, lt.rgb(0x3a3631), hairline * 0.8)
    color = lt.mix(color, lt.rgb(0x1e1b18), core)
    height -= 0.003 * core + 0.0006 * hairline + 0.0008 * lip
    # Bullet chips: pale gouges with a dark heart.
    rng = np.random.default_rng(seed + 2)
    ragged = 0.25 * lt.noise(shape, seed + 3, 3.0)
    for _ in range(chips):
        cx, cz = rng.uniform(-34, 34), rng.uniform(8, 105)
        r = rng.uniform(0.8, 2.2)
        e = np.hypot((xs - cx) / (r * rng.uniform(1.0, 1.8)), (zs - cz) / r) + ragged
        gouge = lt.smooth(1.0, 0.7, e)
        color = lt.mix(color, lt.rgb(0xb5b0a6), gouge * 0.75)
        color = lt.mix(color, lt.rgb(0x4e4a44), lt.smooth(0.45, 0.15, e))
        height -= 0.0012 * gouge
    return color, height


def _paint_edge(region, cracked):
    """The dressed rim (chisel marks across it, worn round at both faces) in its four columns: three for the rim (cm
    along it from the right foot corner), one for the foot."""
    u0, u1, v0, v1 = REGIONS[region]
    rows = int(round((v1 - v0) * TEX))
    cols = int(round((u1 - u0) * TEX))
    shape = (rows, cols)
    color, height, fine = _granite(shape, 560 + (9 if cracked else 0), "dressed")
    across = (np.arange(cols, dtype=np.float32) % COL_PX + 0.5) / COL_PX            # 0 back .. 1 front
    across = np.broadcast_to(across[None, :], shape)
    chisel = np.sin(np.arange(rows, dtype=np.float32)[:, None] * 2.0 * math.pi / 9.0 + 3.0 * lt.noise(shape, 571, 6.0)) \
        * lt.smooth(-0.6, 0.6, lt.noise(shape, 572, 20.0))
    height += 0.0003 * chisel
    edge = np.minimum(across, 1.0 - across) * T                                      # cm to a face
    height -= 0.004 * lt.smooth(1.6, 0.0, edge) ** 2
    color = lt.mix(color, lt.rgb(0xa7a298), lt.smooth(1.6, 0.0, edge) * 0.25)
    col = np.arange(cols)[None, :] // COL_PX
    along = (rows - 1 - np.arange(rows, dtype=np.float32)[:, None] + 0.5) / rows * COL_CM
    s = np.broadcast_to(along + col * COL_CM, shape)
    # Height up the stone (for the soil line), from s.
    z = np.zeros(shape, np.float32)
    total = rim_total()
    side = H - W / 2.0
    z = np.where(s < side, s, np.where(s > total - side, total - s, H))
    z = np.where(col == 3, 0.0, z)
    soil = lt.smooth(24.0, 2.0, z + 5.0 * lt.noise(shape, 573, 20.0))
    color = lt.mix(color, lt.rgb(0x6d5b47), soil * 0.75)
    if cracked:
        # Where a crack meets the rim it runs across it.
        rim = rim_outline()
        for i, crack in enumerate(CRACKS):
            for end in (crack['pts'][0], crack['pts'][-1]):
                if end[1] <= 0.5:
                    continue
                p = min(rim, key=lambda q: (q['co'][0] - end[0]) ** 2 + (q['co'][1] - end[1]) ** 2)
                if p['kind'] == 'foot':
                    continue
                c = int(p['s'] // COL_CM)
                row = rows - 1 - (p['s'] - c * COL_CM) / COL_CM * rows
                y = np.arange(rows, dtype=np.float32)[:, None] + 3.0 * lt.noise(shape, 580 + i, 4.0)
                line = lt.smooth(5.0, 1.5, np.abs(y - row)) * (col == min(c, 2))
                color = lt.mix(color, lt.rgb(0x1e1b18), line)
                height -= 0.003 * line
    rough = 0.84 + 0.05 * fine
    return color, height, rough


def paint_stone():
    """Writes T_GravemotherStone_BC, _N and _ORM (Art/Textures/GravemotherStone)."""
    color = np.zeros((TEX, TEX, 3), np.float32)
    color[:] = lt.rgb(0x8e8b86)
    height = np.zeros((TEX, TEX), np.float32)
    rough = np.full((TEX, TEX), 0.8, np.float32)
    jobs = [('front0', lambda: _paint_face('front0', True, False)), ('front1', lambda: _paint_face('front1', True, True)),
            ('back0', lambda: _paint_face('back0', False, False)), ('back1', lambda: _paint_face('back1', False, True)),
            ('edge0', lambda: _paint_edge('edge0', False)), ('edge1', lambda: _paint_edge('edge1', True))]
    for region, job in jobs:
        u0, u1, v0, v1 = REGIONS[region]
        r0, r1 = int(round((1.0 - v1) * TEX)), int(round((1.0 - v0) * TEX))
        c0, c1 = int(round(u0 * TEX)), int(round(u1 * TEX))
        c, h, r = job()
        color[r0:r1, c0:c1], height[r0:r1, c0:c1], rough[r0:r1, c0:c1] = c, h, r
    px_m = 0.01 / PX_CM
    ao = lt.occlusion(height, px_m, (2, 5, 12), 1.0)
    color = color * (0.72 + 0.28 * ao)[..., None]
    lt.save_set(SET, color, height, px_m, np.clip(rough, 0.0, 1.0), ao=ao, normal_strength=1.0)
