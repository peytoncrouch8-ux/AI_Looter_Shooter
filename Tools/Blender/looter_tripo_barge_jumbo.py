"""The dredge barge's two jumbos (looter_tripo_barge.rework steps 2 and 3): the hull cut at a slab's fore end, the part
fore of it moved on, the gap filled with copies of the slab, every other one mirrored, so each joint meets the same end
of the slab and welds exactly. The fore one (extrude) is an extrusion of the section at the cut: a temporary copy of its
slab is put onto that section, split along x at the section's corners (no face chords across one: Tripo's slab sags and
twists a few cm along its length, and mirrored that read as a zigzag of the strakes' edges and of the bands' shading at
every copy) and split along its length to REFINE_EDGE (looter_tripo_barge_skin reads each corner's paint off Tripo's
side: across a 1 m face the paint between the corners went straight and drew the triangles); the copies are made of it,
and it goes. The midship one keeps its slab's faces, each copy a prism of its fore end's section.
"""
import bmesh
import numpy as np

import looter_tripo_barge_clear as clear
import looter_tripo_barge_skin as skin
import looter_tripo_rework as rw
from looter_tripo_barge import FILL, KEEL_LINE, STRUT, STRUT_PART, STRUT_ROOT, TRIPO, log
from looter_tripo_rework import B, D, U, bisect, copy, spanning, ue_centre, ue_normal

EXTRUDE_REACH = 25.0                   # cm: a slab corner this near the cut's section is put on it
ALIGN_REACH, ALIGN_ANGLE = 30.0, 2.0   # cm, degrees: the slab split along x at a corner of the section that turns this
                                       # much, its faces this near it
REFINE_EDGE, REFINE_ROUNDS = 30.0, 3   # cm: the painted faces split till no edge runs longer along x; at most this often
ON_END = 0.05                          # cm: a corner this near an end plane lies on it (its edges are the joint's)
SLOT_EDGE = 1.0                        # cm: a corner this near the strut's slot plane (STRUT_ROOT, within its |y|) stays
                                       # (rw.close_slot closes the slot along it)
TEMP = 8                               # the temporary extruded slab's part tag


def insert(bm, tag, slab, copies, part, nearest, still, trim=None, extrude=False):
    """Cuts at slab[1], moves everything fore of it (but the parts still) copies * the slab's length forward and fills
    the gap with alternately mirrored copies of the slab (Tripo's faces with their UVs; extrude: of its extrusion).
    Where the front strut was cut off the keel, the copies' and the moved bow's slots are closed. Returns how far it
    moved."""
    a, b = slab
    length = b - a
    dx = length * copies
    movable = lambda f: f[tag] not in still
    for x in (a, b):
        bisect(bm, [f for f in spanning(bm, tag, None, 0, x) if movable(f)], 0, x)
    fore = [f for f in bm.faces if movable(f) and ue_centre(f)[0] > b]
    fore = rw.separate(bm, fore)
    for v in {v for f in fore for v in f.verts}:
        v.co.y -= dx / 100.0
    src = [f for f in bm.faces if movable(f) and f[tag] == TRIPO and a < ue_centre(f)[0] < b]
    warp = _prism(src, a, b)
    if extrude:
        src = _extruded(bm, tag, src, warp, a, b)
        warp = lambda p: p
    # Every other copy mirrored, starting with the first and ending unmirrored (an even count), so each joint meets the
    # same end of the slab: the slab's fore end at the aft part, between copies 2k and 2k+1 and at the bow, its aft end
    # between the others. The joints are exact and welded.
    joints = [b + k * length for k in range(copies + 1)]
    for k in range(copies):
        mirror = k % 2 == 0

        def move(p, k=k, mirror=mirror):
            x, y, z = warp(p)
            if mirror:
                x = a + b - x
            return (x + (k + 1) * length, y, z)
        new = copy(bm, src, tag, part, move, flip=mirror)
        if trim:
            new = clear.trim(bm, tag, nearest, FILL, new, trim)
        if STRUT_PART in still:
            rw.close_slot(bm, new, STRUT_ROOT, tag, FILL, nearest, (b + k * length, b + (k + 1) * length))
    if extrude:
        bmesh.ops.delete(bm, geom=[f for f in src if f.is_valid], context='FACES')
    if STRUT_PART in still:
        rw.close_slot(bm, [f for f in fore if f.is_valid and ue_centre(f)[0] < b + dx + 400.0], STRUT[3], tag, FILL,
                      nearest, (b + dx,))
    welded = _weld(bm, tag, joints, still)
    log(f'jumbo at x {b:.0f}: {len(src)} faces of x {a:.0f}..{b:.0f} copied {copies} times (every other mirrored'
        f'{", an extrusion of the section there" if extrude else ""}), {dx:.0f} cm, {welded} vertices welded at the '
        f'joints')
    return dx


def _weld(bm, tag, joints, still):
    """Merges the coincident corners at the copies' joints (one surface, smooth across)."""
    verts = [v for v in bm.verts if any(abs(U(v.co)[0] - x) < rw.EPS * 100.0 for x in joints)
             and any(f[tag] not in still for f in v.link_faces)]
    n = len(bm.verts)
    bmesh.ops.remove_doubles(bm, verts=verts, dist=0.002)
    return n - len(bm.verts)


def _prism(faces, a, b):
    """A move making the slab's section that of its fore end b at every x (y scaled by the half-beam's ratio)."""
    arrays = rw.edge_arrays(None, faces)
    zs = np.arange(900.0, 2476.0, 25.0)
    xs = np.linspace(a + 1.0, b - 1.0, 9)          # just inside: edges ending on a cut plane are not crossings
    hb = {s: np.array([rw.profile(arrays, x, zs, s) for x in xs]) for s in (-1, 1)}

    def warp(p):
        x, y, z = p
        if abs(y) < 1.0:
            return p
        s = 1 if y > 0 else -1
        i = min(max((x - a) / (b - a) * (len(xs) - 1), 0.0), len(xs) - 1.0)
        j = min(max((z - zs[0]) / (zs[1] - zs[0]), 0.0), len(zs) - 1.0)
        i0, j0 = int(i), int(j)
        i1, j1 = min(i0 + 1, len(xs) - 1), min(j0 + 1, len(zs) - 1)
        fi, fj = i - i0, j - j0
        h = hb[s]
        here = (h[i0, j0] * (1 - fi) + h[i1, j0] * fi) * (1 - fj) + (h[i0, j1] * (1 - fi) + h[i1, j1] * fi) * fj
        there = h[-1, j0] * (1 - fj) + h[-1, j1] * fj
        r = there / here if here > 50.0 else 1.0
        return (x, y * min(max(r, 0.85), 1.2), z)
    return warp


def _section(src, b):
    """The slab's section at its fore end b: its edges as (y, z) segments (starts, directions), and its corners that
    turn more than ALIGN_ANGLE, each with the section's tangent there."""
    on = lambda v: abs(U(v.co)[0] - b) < ON_END
    edges = list({e for f in src for e in f.edges if on(e.verts[0]) and on(e.verts[1])})
    yz = lambda v: (round(U(v.co)[1], 2), round(U(v.co)[2], 2))
    p0 = np.array([yz(e.verts[0]) for e in edges]).reshape(-1, 2)
    d = np.array([yz(e.verts[1]) for e in edges]).reshape(-1, 2) - p0
    nbrs = {}
    for e in edges:
        u, w = yz(e.verts[0]), yz(e.verts[1])
        nbrs.setdefault(u, []).append(w)
        nbrs.setdefault(w, []).append(u)
    corners = []
    for v, ns in nbrs.items():
        if _on_slot(v[0], v[1]) or len(ns) != 2:
            continue
        d0, d1 = np.subtract(v, ns[0]), np.subtract(ns[1], v)
        n0, n1 = np.linalg.norm(d0), np.linalg.norm(d1)
        if n0 < 1e-6 or n1 < 1e-6:
            continue
        d0, d1 = d0 / n0, d1 / n1
        if np.degrees(np.arccos(np.clip(d0 @ d1, -1.0, 1.0))) > ALIGN_ANGLE:
            t = d0 + d1
            corners.append((np.array(v), t / max(np.linalg.norm(t), 1e-9)))
    return p0, d, corners


def _on_slot(y, z):
    return abs(z - STRUT_ROOT) < SLOT_EDGE and abs(y) < STRUT[2]


def _extruded(bm, tag, src, warp, a, b):
    """The slab as an extrusion of its section at b (a temporary copy, tagged TEMP): every corner warped and put onto
    the section, split along x at its corners and to REFINE_EDGE along x where the skin paints it. Returns its faces."""
    p0, d, corners = _section(src, b)
    ll = np.maximum((d * d).sum(1), 1e-9)

    def snap(y, z):
        q = np.array((y, z))
        t = np.clip(((q - p0) * d).sum(1) / ll, 0.0, 1.0)
        near = p0 + d * t[:, None]
        dist = np.sqrt(((near - q) ** 2).sum(1))
        i = int(np.argmin(dist))
        # Under the keel's line every corner goes onto it: what hung there (the strut root's stubs) was a row of
        # teeth under the copies.
        far = dist[i] < EXTRUDE_REACH or z < KEEL_LINE[0]
        return (float(near[i][0]), float(near[i][1])) if far else (y, z)

    def move(p):
        x, y, z = warp(p)
        return (x, y, z) if _on_slot(y, z) or not len(p0) else (x, *snap(y, z))
    temp = copy(bm, src, tag, TEMP, move)
    for v, t in corners:
        near = [f for f in temp if f.is_valid and any(abs(U(q.co)[1] - v[0]) < ALIGN_REACH and
                                                      abs(U(q.co)[2] - v[1]) < ALIGN_REACH for q in f.verts)]
        if not near:
            continue
        geom = near + list({e for f in near for e in f.edges}) + list({q for f in near for q in f.verts})
        made = bmesh.ops.bisect_plane(bm, geom=geom, dist=rw.BISECT_DIST, plane_co=B((0.0, v[0], v[1])),
                                      plane_no=D((0.0, t[0], t[1])))
        temp = list({f for f in temp if f.is_valid} | {g for g in made['geom'] if isinstance(g, bmesh.types.BMFace)})
    for q in {q for f in temp for q in f.verts}:
        x, y, z = U(q.co)
        if not _on_slot(y, z):
            q.co = B((x, *snap(y, z)))
    end = lambda q: abs(U(q.co)[0] - a) < ON_END or abs(U(q.co)[0] - b) < ON_END
    for _ in range(REFINE_ROUNDS):
        for f in temp:
            f.normal_update()
        painted = [f for f in temp if f.is_valid and skin.family(ue_centre(f), ue_normal(f),
                                                                 1 if ue_centre(f)[1] >= 0.0 else -1)]
        long = list({e for f in painted for e in f.edges
                     if abs(U(e.verts[0].co)[0] - U(e.verts[1].co)[0]) > REFINE_EDGE
                     and not (end(e.verts[0]) and end(e.verts[1]))})
        if not long:
            break
        bmesh.ops.subdivide_edges(bm, edges=long, cuts=1, use_grid_fill=True)
        temp = [f for f in bm.faces if f[tag] == TEMP]
    bmesh.ops.triangulate(bm, faces=[f for f in temp if len(f.verts) > 4], quad_method='BEAUTY', ngon_method='BEAUTY')
    temp = [f for f in bm.faces if f[tag] == TEMP]
    for f in temp:
        f.normal_update()
    log(f'jumbo slab extruded: {len(src)} faces onto the section at x {b:.0f} ({len(corners)} corners), {len(temp)} '
        f'faces split to {REFINE_EDGE:.0f} cm along x')
    return temp
