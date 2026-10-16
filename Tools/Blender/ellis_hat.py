"""Ellis's hat seated on his head, and the head under it (Art/Models/Characters/Ellis.py's build_hat, hat_bone and
make_lows call these).

The user, 2026-10-15: "can see through the head/hat, hat doesn't seat on his head properly". What was wrong (round 6):
- The game skin was a crystal: make_lows spared everything within 3 cm of the eyes (1298 of the head's 4268 vertices)
  from one Decimate at 0.30, so the whole budget went there and the skull collapsed into a dozen huge triangles. Under
  the hat there was no back or top of the head: from behind and below you looked through the hat's opening at the sky.
  decimate_skin() now reduces the head by regions, each its own share.
- The crown had no inside: one-sided, Unreal culls it seen from below. Ellis.py's THICK gives it a 4 mm felt wall
  inward (its inner shell faces the head), and a leather sweatband (HatBandInner) lines the crown's foot.
- The portrait's hat was sized round a level slice of the head 5 cm over the eyes plus 1.4 cm, then tipped 9 degrees
  forward about the band's middle: its band stood 2-3.6 cm off the skull all round and its back 4.4 cm over the
  occipital bump. fit() seats it: the band's foot on a plane from BROW_UP over the brows at the front to the occipital
  bump at the back, the crown's foot fitted to the head's section in that plane plus the pressed hair's room (ROOM_*),
  the felt and the sweatband; the brim keeps most of its outline (BRIM_KEEP), the crown its creases.
The look stays the round-4 option C hat (Ellis.py's build_hat: the deeper crease, the harder brim curl, the brim's reach
0.88 of the portrait's).
"""
import math

import bpy  # first: as a Python module (the cloud box's bpy) bmesh and mathutils come with it
import bmesh
import numpy as np
from mathutils.bvhtree import BVHTree
from mathutils import Vector

import looter_garments as gm

BROW = 0.018            # the brows over the eyes' centres (m; MakeHuman's brow ridge, measured on Ellis's head)
BROW_UP = 0.020         # the sweatband's foot over the brows at the front (a cattleman's fit: two fingers)
BACK_UP = 0.012         # ... and over the occipital bump, the fullest point of the back of the head (at the bump
                        # itself the hat sat level and lost its rake; 1.2 cm up it tips 4 degrees forward)
ROOM_FRONT = 0.002      # the sweatband off the skin on the bare forehead
ROOM_HAIR = 0.007       # ... and where the hair runs under it (pressed hair: the sides and back, from the temples)
FELT = 0.004            # the crown's felt (Ellis.py THICK: the inner shell 4 mm in)
SWEAT = 0.0035          # the sweatband's face inside the felt's: clear of the reduced crown (Decimate leaves its foot
                        # about 22 sides, 28 degrees at most: chords 2.7 mm in)
SWEAT_H = 0.035         # the sweatband's height up the crown's inside
BRIM_KEEP = 0.7         # how much of the crown's narrowing the brim's edge does not follow (its outline stays the
                        # user's; the brim reaches further from the slimmer crown)
CROWN_AIR = 0.008       # the least room between the skull and the crown's inside (the crease comes down over the top)
# decimate_skin's shares by region: the lids whole (at 0.75 their slits tore at the corners), the skull under the
# crown (MakeHuman's scalp is coarse already: 276 triangles), the face and neck under the bandana and collar (3016),
# and what shows (3352: forehead, temples, ears, the back of the head). 8480 -> ~2860 (the crystal was 2608)
SKIN_LOW = {'eyes': 1.0, 'skull': 0.25, 'hidden': 0.10, 'seen': 0.20}
EYE_KEEP = 0.017        # around each eye the lids keep their detail (the glowing slits under the brim)


def head_tree(fig):
    """The detailed head (the skin's faces) as a BVH in world space."""
    if getattr(fig, '_head_tree', None) is None:
        keep = fig.w_head > 0.45
        faces = [list(f) for f in fig.faces if keep[f].all()]
        fig._head_tree = BVHTree.FromPolygons([tuple(p) for p in fig.wv], faces)
    return fig._head_tree


def _hit(tree, origin, direction, reach=0.3):
    loc = tree.ray_cast(Vector(tuple(origin)), Vector(tuple(gm.unit(direction))), reach)[0]
    return None if loc is None else np.array(loc)


def seat(fig):
    """Where the hat sits (cached on fig): the band plane's centre O and axes (x his left, f forward, u up the crown),
    the forehead and occipital points F and B it passes through, and the head's section in that plane: ring_ang (from
    the front, + toward his left) and ring_r (m from O to the skin)."""
    if getattr(fig, 'hat_seat', None):
        return fig.hat_seat
    tree = head_tree(fig)
    hy = fig.sk['head'][0][1]
    inside = lambda z: np.array([0.0, hy, z])
    brow_z = fig.eye_z + BROW
    F = _hit(tree, inside(brow_z + BROW_UP), (0.0, -1.0, 0.0))
    # the occipital bump: the back of the head's furthest point, in the middle
    c = inside(fig.eye_z + 0.03)
    best = None
    for deg in np.arange(140.0, 221.0, 2.0):
        a = math.radians(deg)
        p = _hit(tree, c, (0.0, -math.cos(a), math.sin(a)))
        if p is not None and (best is None or p[1] > best[1]):
            best = p
    B = best + np.array([0.0, 0.0, BACK_UP])
    x = np.array([1.0, 0.0, 0.0])
    f = gm.unit(F - B)
    u = np.cross(f, x)
    O = (F + B) * 0.5
    ang = np.linspace(-math.pi, math.pi, 192, endpoint=False)
    r = []
    for a in ang:
        p = _hit(tree, O, f * math.cos(a) + x * math.sin(a))
        r.append(np.linalg.norm(p - O) if p is not None else np.nan)
    r = np.array(r)
    good = ~np.isnan(r)
    r = np.interp(ang, ang[good], r[good], period=2 * math.pi)
    # a band is stiff: smoothed round (a few degrees), it bridges the little hollows
    k = np.array([1, 2, 3, 2, 1], dtype=np.float64)
    r = np.convolve(np.concatenate([r[-2:], r, r[:2]]), k / k.sum(), mode='valid')
    fig.hat_seat = dict(O=O, x=x, f=f, u=u, F=F, B=B, ring_ang=ang, ring_r=r)
    return fig.hat_seat


def room(ang):
    """The sweatband's room off the skin at angle ang (radians from the front): the bare forehead, then the hair."""
    return ROOM_FRONT + (ROOM_HAIR - ROOM_FRONT) * gm.smoothstep(math.radians(35), math.radians(60), np.abs(ang))


def ring_at(s, ang):
    """The head's section radius (m) at angles ang."""
    return np.interp(ang, s['ring_ang'], s['ring_r'], period=2 * math.pi)


def band_ring(fig, n=96, lift=0.0):
    """The sweatband's foot, round the head (n points, world space, the front first, going round by his left): where
    the hair comes out from under the hat (Ellis round 6's contract between HAT and HAIR). lift raises it up the crown
    (m)."""
    s = seat(fig)
    ang = np.linspace(0.0, 2 * math.pi, n, endpoint=False)
    ang = np.where(ang > math.pi, ang - 2 * math.pi, ang)
    r = ring_at(s, ang) + room(ang)
    return s['O'] + (np.outer(np.cos(ang), s['f']) + np.outer(np.sin(ang), s['x'])) * r[:, None] + s['u'] * lift


def _frame_of(ring):
    """The portrait hat's own frame from its crown's foot (a level ellipse of 96, the front first): centre, x, f, u."""
    O = ring.mean(0)
    _, _, vt = np.linalg.svd(ring - O)
    u = vt[2] if vt[2][2] > 0 else -vt[2]
    f = ring[0] - O
    f = gm.unit(f - u * (f @ u))
    x = np.cross(u, f)
    return O, x, f, u


def _local(P, frame):
    O, x, f, u = frame
    Q = P - O
    return np.stack([Q @ x, Q @ f, Q @ u], 1)


def _world(L, frame):
    O, x, f, u = frame
    return O + np.outer(L[:, 0], x) + np.outer(L[:, 1], f) + np.outer(L[:, 2], u)


def _faces_out(ob, axis_point, axis):
    """Turns a crown's faces to face away from its axis if most face in (Ellis.py's THICK grows its felt inward)."""
    me = ob.data
    score = 0.0
    for p in me.polygons:
        c = np.array(p.center) - axis_point
        c = c - axis * (c @ axis)
        score += float(np.array(p.normal) @ c) * p.area
    if score < 0.0:
        bm = bmesh.new()
        bm.from_mesh(me)
        bmesh.ops.reverse_faces(bm, faces=bm.faces[:])
        bm.to_mesh(me)
        bm.free()
        me.update()


def fit(fig, parts, band_material=None):
    """Seats the portrait's hat (crown, brim, band: EllisBlender.build_hat moved into the figure's space, Ellis.py's
    option C touches done) on the head: the crown's foot fitted, column by column, to the head's section in the band
    plane plus the room, the felt and the sweatband; the brim's spokes follow at their root and BRIM_KEEP of the way
    less at the edge; the crown raised if its crease would come down within CROWN_AIR of the skull; all of it turned
    and moved onto seat(). Adds the sweatband (HatBandInner) and returns the parts."""
    s = seat(fig)
    crown = next(o for o in parts if o.name.startswith('HatCrown'))
    brim = next(o for o in parts if o.name.startswith('HatBrim'))
    band = next(o for o in parts if o.name.startswith('HatBand'))
    cc = gm._object_coords(crown)
    src = _frame_of(cc[:96])
    dst = (s['O'], s['x'], s['f'], s['u'])
    base = _local(cc[:96], src)
    psi = np.arctan2(base[:, 0], base[:, 1])                     # each column's angle round the foot
    want = ring_at(s, psi) + room(psi) + SWEAT + FELT            # the crown's outside at its foot
    k = want / np.linalg.norm(base[:, :2], axis=1)               # each column's scale
    # the crown: every row of a column scaled alike (the creases and pinches keep their shape); the tip on the axis
    L = _local(cc, src)
    col = np.arange(len(L)) % 96
    top = len(L) - 1 if len(L) % 96 == 1 else None
    kk = k[col]
    if top is not None:
        kk[top] = k.mean()
    L[:, :2] *= kk[:, None]
    # room over the skull: the crown's inside (FELT in from its outside) at least CROWN_AIR off it, raising the crown's
    # upper rows (by height) if the crease came down too far
    tree = head_tree(fig)
    raise_by = 0.0
    for _ in range(12):
        Lr = L.copy()
        Lr[:, 2] *= 1.0 + raise_by
        P = _world(Lr, dst)
        worst = 1.0
        for p in P[Lr[:, 2] > 0.05]:                             # over the top (the walls stand clear of the skull)
            loc, nrm, _i, d = tree.find_nearest(Vector(tuple(p)), 0.1)
            if loc is not None:
                worst = min(worst, (Vector(tuple(p)) - loc).dot(nrm) - FELT)
        if worst >= CROWN_AIR:
            break
        raise_by += 0.02
    L[:, 2] *= 1.0 + raise_by
    crown.data.vertices.foreach_set('co', _world(L, dst).astype(np.float32).ravel())
    crown.data.update()
    # the band (round the crown's foot, a little outside it): its columns as the crown's
    bb = gm._object_coords(band)
    Lb = _local(bb, src)
    Lb[:, :2] *= k[np.arange(len(Lb)) % 96][:, None]
    band.data.vertices.foreach_set('co', _world(Lb, dst).astype(np.float32).ravel())
    band.data.update()
    # the brim: spokes of 96; the root ring as the crown's foot, the edge in by (1 - BRIM_KEEP) of that
    br = gm._object_coords(brim)
    Lr = _local(br, src).reshape(-1, 96, 3)
    rad = np.linalg.norm(Lr[..., :2], axis=2)
    r0, r1 = rad[0], rad[-1]
    shrink = r0 * (1.0 - k)
    r1n = r1 - (1.0 - BRIM_KEEP) * shrink
    t = (rad - r0) / np.maximum(r1 - r0, 1e-6)
    new = r0 * k + t * (r1n - r0 * k)
    Lr[..., :2] *= (new / np.maximum(rad, 1e-9))[..., None]
    brim.data.vertices.foreach_set('co', _world(Lr.reshape(-1, 3), dst).astype(np.float32).ravel())
    brim.data.update()
    _faces_out(crown, s['O'], s['u'])
    # the sweatband: a leather strip on the crown's inside, from its foot SWEAT_H up, facing the head
    # every other column of the crown at the foot and SWEAT_H up, pulled in by the felt (the crown's wall stands near
    # upright there)
    Lc = L[:len(L) - (top is not None)].reshape(-1, 96, 3)
    rows = []
    for h in (0.0, SWEAT_H):
        pts = []
        for j in range(0, 96, 2):
            c = Lc[:5, j]                                        # the lowest rows rise in step (the crease is higher)
            p = np.array([np.interp(h, c[:, 2], c[:, 0]), np.interp(h, c[:, 2], c[:, 1]), h])
            rad = np.linalg.norm(p[:2])
            p[:2] *= (rad - FELT - SWEAT) / rad
            pts.append(p)
        rows.append(_world(np.array(pts), dst))
    R = np.array(rows[::-1])                                     # rows top down (gm.grid's way)
    piece = gm.grid(R[..., 0], R[..., 1], R[..., 2], wrap=True)
    inner = gm.mesh('HatBandInner', piece, band_material)
    # facing the head (in toward the crown's axis)
    _faces_out(inner, s['O'], s['u'])
    bm = bmesh.new()
    bm.from_mesh(inner.data)
    bmesh.ops.reverse_faces(bm, faces=bm.faces[:])
    bm.to_mesh(inner.data)
    bm.free()
    inner.data.update()
    fig.hat_fit = dict(s, raise_by=raise_by, k=k, air=worst)
    return parts + [inner]


PIVOT = (0.04, -0.03)   # the hat bone's head from the back of the sweatband's foot: forward, up (m)


def pivot(fig, parts):
    """The hat bone (head, tail; world): the game tips the hat brim-up about its head while aiming (FBodyHatTip, 18
    degrees). Inside the back of the skull, PIVOT from the back of the sweatband's foot, so the band slides round the
    head as it tips: tipped 20 degrees its foot lifts at most 1 cm off the forehead and 0.6 off the sides and back, the
    brim reaches no hair. (About the back of the band itself, round 5's pivot, the front lifted 3 cm and the temples
    2.5; 6 cm further in, the brim's back pressed 1.6 cm into the hair; the brim's right front rises 8.2 cm, 9.7 there.)"""
    s = seat(fig)
    back = s['O'] - s['f'] * (ring_at(s, np.array([math.pi]))[0] + room(np.array([math.pi]))[0])
    head = back + s['f'] * PIVOT[0] + s['u'] * PIVOT[1]
    return head, head + s['f'] * 0.2


def decimate_skin(fig, low):
    """The game's head (low: a copy of the detailed skin) reduced region by region, each to its SKIN_LOW share: the
    eyes' lids kept, the skull under the crown, the face and neck hidden under the bandana and collar, and what shows
    (the forehead under the band, temples, ears, the back of the head over the collar). One Decimate per region with
    the rest spared (a vertex group, inverted): from one ratio over all of it, sparing the eyes spent the budget there
    and the skull fell to a dozen triangles."""
    s = seat(fig)

    def regions(co):
        d = np.minimum(np.linalg.norm(co - fig.eyes[0], axis=1), np.linalg.norm(co - fig.eyes[1], axis=1))
        eyes = d < EYE_KEEP
        skull = ((co - s['O']) @ s['u'] > 0.012) & ~eyes
        hidden = (co[:, 2] < fig.eye_z - 0.035) & ~eyes & ~skull
        return {'eyes': eyes, 'skull': skull, 'hidden': hidden, 'seen': ~(eyes | skull | hidden)}

    before = sum(len(p.vertices) - 2 for p in low.data.polygons)
    passes = []
    for name in ('skull', 'hidden', 'seen', 'eyes'):
        share = SKIN_LOW[name]
        if share >= 1.0:
            continue
        co = gm._object_coords(low)
        reg = regions(co)[name]
        tris = sum(len(p.vertices) - 2 for p in low.data.polygons)
        mine = sum(len(p.vertices) - 2 for p in low.data.polygons if reg[list(p.vertices)].all())
        if not mine:
            continue
        m = low.modifiers.new('decimate', 'DECIMATE')
        m.ratio = (tris - mine * (1.0 - share)) / tris
        m.use_symmetry = True
        m.symmetry_axis = 'X'
        g = low.vertex_groups.new(name='keep')
        g.add([int(i) for i in np.nonzero(~reg)[0]], 1.0, 'REPLACE')
        m.vertex_group = 'keep'
        m.invert_vertex_group = True
        m.vertex_group_factor = 1.0
        gm.apply_modifiers(low, keep=())
        low.vertex_groups.remove(low.vertex_groups['keep'])
        passes.append((name, mine, tris, sum(len(p.vertices) - 2 for p in low.data.polygons)))
    after = sum(len(p.vertices) - 2 for p in low.data.polygons)
    fig.skin_passes = passes
    return before, after
