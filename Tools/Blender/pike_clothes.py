"""Pike's clothes (Art/Models/Characters/Pike.py calls these with M, the model's module: its slot(), log() and sizes).
From the boots up: the breeches, the puttees wound over them, ankle boots laced over hooks, the waistcoat and the shirt
in its V, the field coat (one shell of the torso and arms, open at the front: Ellis.py's duster was draped and hidden
its shoulders under a cape, which Pike hasn't), its skirt below the belt (draped), cuffs, rolled collar, lapels, patch
pockets, and the neckerchief. Units: metres, Z up, the front -Y; the figure's left +X.
"""
import math

import bpy
import bmesh
import numpy as np
from mathutils import Matrix, Vector
from mathutils.bvhtree import BVHTree

import looter_garments as gm
import looter_model as lm

# the parts whose looks sew hems and seams from the 'Seams' attribute (looter_looks.construction)
SEAMED = ('Coat', 'Skirt', 'Collar', 'Lapel', 'Pocket', 'Flap', 'Cuff', 'Kerchief', 'Waistcoat', 'Belt', 'Strap',
          'Bandolier', 'RuckLid', 'Rucksack', 'Trousers', 'HatBrim', 'Pouch', 'PouchFlap')


# ------------------------------------------------------------------------------------------------------------ helpers

def set_seams(ob, seam_dist=None):
    """The 'Seams' colour attribute the looks sew from: R how close each vertex is to an open edge, G to a sewn seam
    (seam_dist, metres per vertex), both 1 there and 0 from 10 cm (Ellis.py's)."""
    from mathutils import kdtree
    me = ob.data
    co = gm._object_coords(ob)
    count = {}
    for p in me.polygons:
        vs = list(p.vertices)
        for k in range(len(vs)):
            key = (min(vs[k], vs[(k + 1) % len(vs)]), max(vs[k], vs[(k + 1) % len(vs)]))
            count[key] = count.get(key, 0) + 1
    d_edge = np.full(len(co), 1.0)
    pts = []
    for (a, b), c in count.items():
        if c == 1:
            for t in np.linspace(0.0, 1.0, 6):
                pts.append(co[a] * (1 - t) + co[b] * t)
    if pts:
        tree = kdtree.KDTree(len(pts))
        for i, q in enumerate(pts):
            tree.insert(q, i)
        tree.balance()
        d_edge = np.array([tree.find(q)[2] for q in co])
    if seam_dist is None and 'SeamDist' in me.attributes:
        seam_dist = np.empty(len(co), dtype=np.float32)
        me.attributes['SeamDist'].data.foreach_get('value', seam_dist)
    col = np.ones((len(co), 4), dtype=np.float32)
    col[:, 0] = 1.0 - np.clip(d_edge / 0.1, 0.0, 1.0)
    col[:, 1] = 0.0 if seam_dist is None else 1.0 - np.clip(np.asarray(seam_dist) / 0.1, 0.0, 1.0)
    col[:, 2] = 0.0
    attr = me.color_attributes.get('Seams') or me.color_attributes.new('Seams', 'FLOAT_COLOR', 'POINT')
    attr.data.foreach_set('color', col.ravel())


def keep_seam_dist(ob, dist):
    ob.data.attributes.new('SeamDist', 'FLOAT', 'POINT').data.foreach_set('value', np.asarray(dist, np.float32))


def round_angle(P, centre, fwd=(0.0, -1.0)):
    """Angles of points round a vertical axis at centre (radians: 0 the front, + toward the figure's left)."""
    f, l = gm.frame2d(np.asarray(fwd))
    rel = np.atleast_2d(P)[:, :2] - np.asarray(centre)[:2]
    return np.arctan2(rel @ l, rel @ f)


def outer_r(tree, centre, a, z, reach=0.6):
    """How far out from a vertical axis at centre a surface's outermost point lies along angle a at height z (a ray
    from outside, inward), or None."""
    dx, dy = gm.radial(np.float64(a), np.array([0.0, -1.0]))
    o = Vector((centre[0] + dx * reach, centre[1] + dy * reach, z))
    hit = tree.ray_cast(o, Vector((-dx, -dy, 0.0)), reach)
    return None if hit[0] is None else reach - hit[3]


def face_out(ob, centre, z_band=None):
    """Turns a sheet's faces to face away from a vertical axis at centre if most face in."""
    me = ob.data
    score = 0.0
    for p in me.polygons:
        c = np.array(p.center)
        if z_band is not None and not (z_band[0] <= c[2] <= z_band[1]):
            continue
        r = np.array([c[0] - centre[0], c[1] - centre[1], 0.0])
        score += float(np.array(p.normal) @ r) * p.area
    if score < 0.0:
        bm = bmesh.new()
        bm.from_mesh(me)
        bmesh.ops.reverse_faces(bm, faces=bm.faces[:])
        bm.to_mesh(me)
        bm.free()
        me.update()


def place_button(name, at, normal, mat, radius=0.0058, flat=0.55):
    bm = bmesh.new()
    lm.uv_sphere(bm, 10, 6, radius)      # fixed face order: the bake must fit the next run
    me = bpy.data.meshes.new(name)
    bm.to_mesh(me)
    bm.free()
    me.shade_smooth()
    ob = bpy.data.objects.new(name, me)
    bpy.context.scene.collection.objects.link(ob)
    ob.matrix_world = Matrix.Translation(Vector(tuple(at))) @ Vector(tuple(normal)).to_track_quat('Z', 'Y').to_matrix().to_4x4() \
        @ Matrix.Diagonal((1.0, 1.0, flat, 1.0))
    ob.data.transform(ob.matrix_world)
    ob.matrix_world = Matrix.Identity(4)
    ob.data.materials.append(mat)
    return ob


def project_grid(tree, rows, lift, name, mat, direction=(0.0, 1.0, 0.0)):
    """A grid of (x, z, extra lift) rows projected onto a surface along direction (from the front by default) and lifted
    off it along its normal; arc-length UVs (gm.grid's). A ray that misses takes the surface's nearest point to where
    its neighbour landed (a miss once threw a pocket's corner 60 cm out in front of him)."""
    d = Vector(direction)
    P = []
    last_y = None
    for row in rows:
        line = []
        for x, z, extra in row:
            o = Vector((x, -d.y * 0.6, z)) if abs(d.y) > 0.5 else Vector((-d.x * 0.6, 0.0, z))
            hit = tree.ray_cast(o, d, 1.5)
            if hit[0] is None:
                guess = Vector((x, last_y if last_y is not None else 0.0, z))
                loc, nrm, _i, _d = tree.find_nearest(guess, 0.3)
                if loc is None:
                    line.append(np.array([x, last_y or 0.0, z]))
                    continue
                hit = (loc, nrm)
            else:
                last_y = hit[0].y
            line.append(np.array(hit[0] + hit[1] * (lift + extra)))
        P.append(line)
    P = np.array(P)
    return gm.mesh(name, gm.grid(P[..., 0], P[..., 1], P[..., 2]), mat)


def hull_2d(p):
    """The convex hull of 2D points, counterclockwise (Andrew's monotone chain)."""
    p = p[np.lexsort((p[:, 1], p[:, 0]))]
    cross = lambda o, a, b: (a[0] - o[0]) * (b[1] - o[1]) - (a[1] - o[1]) * (b[0] - o[0])
    lower, upper = [], []
    for q in p:
        while len(lower) >= 2 and cross(lower[-2], lower[-1], q) <= 0:
            lower.pop()
        lower.append(q)
    for q in p[::-1]:
        while len(upper) >= 2 and cross(upper[-2], upper[-1], q) <= 0:
            upper.pop()
        upper.append(q)
    return np.array(lower[:-1] + upper[:-1])


def hull_radii(hull, dirs):
    """How far out along each unit direction (k, 2) from the origin a convex polygon round it reaches."""
    e = np.roll(hull, -1, axis=0) - hull
    den = dirs[:, 0, None] * e[None, :, 1] - dirs[:, 1, None] * e[None, :, 0]
    with np.errstate(divide='ignore', invalid='ignore'):
        t = (hull[None, :, 0] * e[None, :, 1] - hull[None, :, 1] * e[None, :, 0]) / den
        u = (hull[None, :, 0] * dirs[:, 1, None] - hull[None, :, 1] * dirs[:, 0, None]) / den
    ok = (np.abs(den) > 1e-12) & (t > 0.0) & (u >= -1e-9) & (u <= 1.0 + 1e-9)
    return np.where(ok, t, 0.0).max(axis=1)


def leg_slicer(fig, s):
    """slice_at(z): the leg's section on side s at height z (2D points), cut through its faces."""
    sx = 1.0 if s == 'l' else -1.0
    ed = {(min(p[k], p[k - 1]), max(p[k], p[k - 1])) for p in fig.faces for k in range(len(p))}
    ed = np.array(sorted(ed))
    a, b = fig.wv[ed[:, 0]], fig.wv[ed[:, 1]]
    keep = (a[:, 0] * sx > 0.0) & (b[:, 0] * sx > 0.0) & (a[:, 2] < 0.75) & (b[:, 2] < 0.75)
    a, b = a[keep], b[keep]

    def slice_at(z):
        da, db = a[:, 2] - z, b[:, 2] - z
        cut = (da * db <= 0.0) & (da != db)
        return (a[cut] + (b[cut] - a[cut]) * (da[cut] / (da[cut] - db[cut]))[:, None])[:, :2]
    return slice_at


def neck_column(fig, room=0.010):
    """Per MakeHuman vertex: the neck itself (and the head), which the coat, the waistcoat and the shirt leave open:
    within the neck's radius (+ room) of neck_01's axis from 3 cm under its joint up. (By the head's weights alone the
    coat lost the upper chest beside the throat: neck_01's weight reaches down over the collarbones.)"""
    cache = fig.__dict__.setdefault('_neck_col', {})
    if room in cache:
        return cache[room]
    n = fig.neck
    r = np.linalg.norm(fig.wv[:, :2] - n[:2], axis=1)
    band = (np.abs(fig.wv[:, 2] - (n[2] + 0.03)) < 0.008) & (fig.w_head > 0.5)
    r_neck = float(np.percentile(r[band], 70)) if band.any() else 0.065
    cache[room] = ((r < r_neck + room) & (fig.wv[:, 2] > n[2] - 0.03)) | (fig.Wv[:, fig.col['head']] > 0.3)
    fig.r_neck = r_neck
    return cache[room]


# ---------------------------------------------------------------------------------------------- breeches, puttees

def build_trousers(M, fig):
    """Olive wool breeches: the hips and legs pushed out and smoothed, from the waist down into the puttees, full over
    the thigh and bloused over the puttees' top under the knee."""
    z = fig.wv[:, 2]
    PT = M.PUTTEE_TOP
    keep = (z < M.BELT + 0.03) & (z > PT - 0.06) & (fig.w_arm < 0.2) & (fig.w_hand < 0.1)
    loose = gm.smoothstep(0.95, 0.70, z) * gm.smoothstep(PT + 0.02, PT + 0.16, z)
    blouse = gm.smoothstep(PT - 0.005, PT + 0.03, z) * gm.smoothstep(PT + 0.13, PT + 0.05, z)
    inside = gm.smoothstep(PT + 0.0, PT - 0.025, z)                 # tucked in: tighter than the puttee over it
    off = (0.009 + 0.012 * loose + 0.012 * blouse) * (1.0 - inside) + 0.008 * inside
    # breeches: the outer thigh flared above the knee
    for s_, sx in (('l', 1.0), ('r', -1.0)):
        th_ = fig.sk[f'thigh_{s_}']
        mid = (th_[0] + th_[1]) * 0.5
        outer = gm.smoothstep(0.0, 0.05, (fig.wv[:, 0] - mid[0]) * sx) * (fig.wv[:, 0] * sx > 0)
        off = off + 0.020 * outer * np.exp(-((z - (PT + 0.20)) / 0.10) ** 2)
    piece, used = gm.shell(fig.wv, fig.faces, fig.fuv, fig.face_mask(keep), off, iters=14)
    v = piece.verts
    nrm = gm.vertex_normals(v, piece.faces)
    for s_ in 'lr':
        knee = fig.sk[f'calf_{s_}'][0]
        side = (v[:, 0] * knee[0]) > 0
        dz = v[:, 2] - knee[2]
        behind = gm.smoothstep(knee[1] - 0.02, knee[1] + 0.05, v[:, 1])
        ring = np.sin(dz * 150.0 + v[:, 0] * 40.0) * np.exp(-(dz / 0.06) ** 2) * (0.35 + 0.65 * behind)
        bag = np.sin(v[:, 2] * 190.0 + np.arctan2(v[:, 1] - knee[1], v[:, 0] - knee[0]) * 3.0)
        bl =gm.smoothstep(PT, PT + 0.03, v[:, 2]) * gm.smoothstep(PT + 0.12, PT + 0.05, v[:, 2])
        v[side] += nrm[side] * (0.004 * ring[side] + 0.004 * (bag * bl)[side])[:, None]
    piece.verts = v
    return gm.mesh('Trousers', piece, M.slot('Trousers'))


PUTTEE_ROOM = 0.013      # the puttee's wool over the leg (m): the breeches tucked under it at 8 mm
PUTTEE_PITCH = 0.056     # how far down each turn winds (m)
PUTTEE_STEP = 0.0028     # each turn's edge standing proud of the one under it (m)


def leg_tube(fig, s, z_top, z_bot, room, dz=0.004, n=56, smooth=6):
    """A tube round the leg on side s from z_top down to z_bot: per slice the leg's convex hull plus room, a little
    rounder than the leg, its centre line and radii smoothed (a wound puttee, not a sock). Returns (rows, cols, 3)
    points from the top down, and the angles."""
    slice_at = leg_slicer(fig, s)
    zs = np.arange(z_top, z_bot - 1e-6, -dz)
    A = np.linspace(-math.pi, math.pi, n, endpoint=False)
    D = np.stack(gm.radial(A, np.array([0.0, -1.0])), axis=1)
    hulls = [hull_2d(slice_at(z)) for z in zs]
    cen = np.array([h.mean(axis=0) for h in hulls])
    for _ in range(smooth * 2):
        cen[1:-1] = 0.25 * cen[:-2] + 0.5 * cen[1:-1] + 0.25 * cen[2:]
    fit = np.array([hull_radii(h - c, D) for h, c in zip(hulls, cen)])
    rad = np.maximum(0.8 * fit + 0.2 * fit.mean(axis=1, keepdims=True), fit - 0.002) + room
    for _ in range(smooth):
        rad[1:-1] = 0.25 * rad[:-2] + 0.5 * rad[1:-1] + 0.25 * rad[2:]
        rad = 0.25 * np.roll(rad, 1, 1) + 0.5 * rad + 0.25 * np.roll(rad, -1, 1)
    rad = np.maximum(rad, fit + room * 0.8)
    P = np.zeros((len(zs), n, 3))
    P[..., :2] = cen[:, None, :] + D[None] * rad[..., None]
    P[..., 2] = zs[:, None]
    return P, A, cen, rad


def build_puttees(M, fig):
    """Olive-tan wool puttees wound up the calf from inside the boot to just under the knee: each turn spirals and
    its lower edge stands proud of the turn under it; a tape wound round the top twice."""
    out = []
    for s in 'lr':
        sx = 1.0 if s == 'l' else -1.0
        P, A, cen, rad = leg_tube(fig, s, M.PUTTEE_TOP, M.BOOT_TOP - 0.03, PUTTEE_ROOM)
        z = P[..., 2]
        a = np.broadcast_to(A[None, :], z.shape)
        # winding up from the ankle, outward round the leg: each turn's lower edge lies over the turn below
        u = np.mod(z / PUTTEE_PITCH - sx * a / (2 * math.pi) + 0.06 * np.sin(a * 2.0 + z * 40.0), 1.0)
        prof = np.where(u < 0.88, 1.0 - 0.85 * (u / 0.88), 0.15 + 0.85 * gm.smoothstep(0.88, 1.0, u))
        top = M.PUTTEE_TOP
        tape = gm.smoothstep(top - 0.032, top - 0.026, z) * gm.smoothstep(top - 0.002, top - 0.008, z)
        tape_step = 0.0015 * (np.mod((top - z) / 0.012, 1.0) < 0.7)
        d = PUTTEE_STEP * prof * (1.0 - tape) + tape * (0.003 + tape_step)
        # the top turned over a little; the bottom tucked into the boot
        d += 0.002 * gm.smoothstep(top - 0.012, top, z)
        dirs = P[..., :2] - cen[:, None, :]
        dirs /= np.maximum(np.linalg.norm(dirs, axis=-1, keepdims=True), 1e-9)
        P[..., :2] += dirs * d[..., None]
        piece = gm.grid(P[..., 0], P[..., 1], P[..., 2], wrap=True)
        out.append(gm.mesh(f'Puttee_{s}', piece, M.slot('Puttees')))
    M.log(f'puttees: {P.shape[0]} rows of {P.shape[1]}, {M.PUTTEE_TOP:.3f} to {M.BOOT_TOP - 0.03:.3f} m')
    return out


# ------------------------------------------------------------------------------------------------------------ boots

BOOT_EASE = 0.026        # the shaft's room over the leg (m): the puttee (1.6 cm and its turns) inside its 5 mm wall
BOOT_TAPER = 0.15
BOOT_WALL = 0.005
BOOT_VOXEL = 0.0018
BOOT_TRIS = 6000


def insole(s):
    """The insole's height along the foot (s, m ahead of the ankle): 3.4 cm on the heel, 1.4 under the ball."""
    return np.interp(s, [-0.2, 0.03, 0.10, 0.17, 0.25], [0.034, 0.034, 0.014, 0.014, 0.019])


def foot_frame(fig, s):
    ankle, ball = fig.sk[f'foot_{s}'][0], fig.sk[f'ball_{s}'][0]
    f = gm.unit(np.array([ball[0] - ankle[0], ball[1] - ankle[1], 0.0]))
    return ankle, f, np.array([-f[1], f[0], 0.0])


def boot_shaft(M, fig, s):
    """An ankle boot's shaft: round the leg's slices (convex hull, with room for the puttee) from its top down to the
    insole, its top nearly level (a lace boot's, a little lower at the tongue), a well inside it the puttee tucks into
    (Ellis.py's stovepipe made short)."""
    ankle, f, lat = foot_frame(fig, s)
    slice_at = leg_slicer(fig, s)
    z_lo = ankle[2] + 0.03
    zs = np.arange(M.BOOT_TOP + 0.02, z_lo - 1e-6, -0.006)
    A = np.linspace(-math.pi, math.pi, 56, endpoint=False)
    D = np.stack(gm.radial(A, np.array([0.0, -1.0])), axis=1)
    hulls = [hull_2d(slice_at(z)) for z in zs]
    cen = np.array([h.mean(axis=0) for h in hulls])
    for _ in range(6):
        cen[1:-1] = 0.25 * cen[:-2] + 0.5 * cen[1:-1] + 0.25 * cen[2:]
    fit = np.array([hull_radii(h - c, D) for h, c in zip(hulls, cen)])
    rad = fit + BOOT_EASE
    rad = np.maximum(0.75 * rad + 0.25 * rad.mean(axis=1, keepdims=True), fit + 0.02)
    for i in range(1, len(zs)):
        rad[i] = np.maximum(rad[i], rad[i - 1] - BOOT_TAPER * (zs[i - 1] - zs[i]))
    up = zs[::-1]

    def ring(z, inset=0.0):
        zc = np.clip(z, up[0], up[-1])
        c = np.stack([np.interp(zc, up, cen[::-1, k]) for k in (0, 1)], axis=1)
        r = np.array([np.interp(zc[k], up, rad[::-1, k]) for k in range(len(A))]) - inset
        return np.c_[c + D * r[:, None], z]

    top = M.BOOT_TOP - 0.008 * np.cos(A) ** 2 * (np.cos(A) > 0)
    bottom = insole(((ring(np.full(len(A), z_lo)) - ankle) @ f)) + 0.001
    floor = M.BOOT_TOP - 0.06
    rows = [ring(floor + (top - floor) * t, BOOT_WALL) for t in np.linspace(0.0, 1.0, 8)]
    rows.append(ring(top + 0.0025, BOOT_WALL * 0.5))
    rows += [ring(top + (bottom - top) * t) for t in np.linspace(0.0, 1.0, 22)]
    R = np.array(rows)
    piece = gm.grid(R[..., 0], R[..., 1], R[..., 2], wrap=True)
    verts, faces, n = list(piece.verts), piece.faces, len(A)
    for row, up_ in ((0, True), (len(R) - 1, False)):
        c = len(verts)
        verts.append(R[row].mean(axis=0))
        for k in range(n):
            a_, b_ = row * n + k, row * n + (k + 1) % n
            faces.append([c, a_, b_] if up_ else [c, b_, a_])
    return gm.Piece(np.array(verts), faces)


def boot_last(fig, s):
    """A boot's foot on its last (Ellis.py's): rounded sections from the heel to a round toe box 2.5 cm past the toes."""
    ankle, f, lat = foot_frame(fig, s)
    sx = 1.0 if s == 'l' else -1.0
    used = fig.b.used
    feet = fig.wv[used][(fig.w_foot[used] > 0.3) & (fig.wv[used][:, 0] * sx > 0)]
    sf, lf, zf = (feet - ankle) @ f, (feet - ankle) @ lat, feet[:, 2]
    s0, s1 = sf.min() - 0.006, sf.max() + 0.025
    u = np.linspace(0.0, 1.0, 46)
    S = s0 + (s1 - s0) * (0.6 * u + 0.4 * (0.5 - 0.5 * np.cos(math.pi * u)))
    lo, hi, top = np.zeros(len(S)), np.zeros(len(S)), np.zeros(len(S))
    for i, q in enumerate(S):
        near = np.abs(sf - q) < 0.01
        if near.any():
            lo[i], hi[i], top[i] = lf[near].min() - 0.010, lf[near].max() + 0.010, zf[near].max() + 0.012
        else:
            lo[i], hi[i], top[i] = lo[i - 1], hi[i - 1], top[i - 1]
    sb = sf[zf < 0.05].max()
    top = np.maximum(top, (insole(S) + 0.048) * gm.smoothstep(sb - 0.10, sb - 0.06, S))
    s_ball = (fig.sk[f'ball_{s}'][0] - ankle) @ f
    for i in range(1, len(S)):
        if S[i] > s_ball:
            give = 0.15 * (S[i] - S[i - 1])
            lo[i], hi[i] = min(lo[i], lo[i - 1] + give), max(hi[i], hi[i - 1] - give)
    for _ in range(6):
        for a in (lo, hi, top):
            a[1:-1] = 0.25 * a[:-2] + 0.5 * a[1:-1] + 0.25 * a[2:]
    toe = np.clip((S - (s1 - 0.045)) / 0.045, 0.0, 1.0)
    heel = np.clip(((s0 + 0.02) - S) / 0.02, 0.0, 1.0)
    plan = np.clip(1.0 - toe ** 2.5, 0.0, 1.0) ** 0.4 * np.sqrt(np.clip(1.0 - heel ** 2, 0.0, 1.0))
    mid, half = (lo + hi) * 0.5, (hi - lo) * 0.5 * np.maximum(plan, 0.08)
    zb = insole(S)
    zt = zb + (top - zb) * np.maximum(np.sqrt(np.clip(1.0 - toe ** 2, 0.0, 1.0)), 0.08)
    phi = np.linspace(0.0, 2 * math.pi, 32, endpoint=False)
    cu, su = np.cos(phi), np.sin(phi)
    lat_u = np.sign(cu) * np.abs(cu) ** (2.0 / 3.2)
    up_u = np.where(su >= 0.0, np.abs(su) ** (2.0 / 3.0), -np.abs(su) ** (2.0 / 9.0))
    R = []
    for i in range(len(S)):
        zc, hz = (zb[i] + zt[i]) * 0.5, (zt[i] - zb[i]) * 0.5
        xy = ankle[None, :2] + f[None, :2] * S[i] + lat[None, :2] * (mid[i] + half[i] * lat_u)[:, None]
        R.append(np.c_[xy, zc + hz * up_u])
    R = np.array(R)
    piece = gm.grid(R[..., 0], R[..., 1], R[..., 2], wrap=True)
    verts, faces, n = list(piece.verts), piece.faces, len(phi)
    for row, push in ((0, -0.004), (len(S) - 1, 0.0)):
        c = len(verts)
        verts.append(R[row].mean(axis=0) + f * push)
        for k in range(n):
            a_, b_ = row * n + k, row * n + (k + 1) % n
            faces.append([c, a_, b_] if row == 0 else [c, b_, a_])
    return gm.Piece(np.array(verts), faces)


def fillet(ob, near, iters=60, step=0.5):
    """Rounds the crease where fused solids meet (Ellis.py's)."""
    me = ob.data
    co = gm._object_coords(ob)
    ed = np.empty(len(me.edges) * 2, dtype=np.int64)
    me.edges.foreach_get('vertices', ed)
    a, b = ed[0::2], ed[1::2]
    cnt = np.bincount(a, minlength=len(co)) + np.bincount(b, minlength=len(co))
    k = (step * near)[:, None]
    for _ in range(iters):
        acc = np.stack([np.bincount(a, co[b, j], len(co)) + np.bincount(b, co[a, j], len(co)) for j in range(3)], 1)
        co += k * (acc / np.maximum(cnt, 1)[:, None] - co)
    me.vertices.foreach_set('co', co.astype(np.float32).ravel())
    me.update()


def build_boots(M, fig, puttees):
    """Brown leather ankle boots: the short shaft and the foot on its last fused into one surface, the crease rounded,
    on a stacked sole with a welt and a low heel; laced up the front over iron hooks."""
    import looter_bake as bk
    shafts = [gm.mesh(f'_Shaft_{s}', boot_shaft(M, fig, s), M.slot('Boots')) for s in 'lr']
    lasts = [gm.mesh(f'_Last_{s}', boot_last(fig, s), M.slot('Boots')) for s in 'lr']
    shaft_tree, last_tree = gm.bvh_objects(shafts), gm.bvh_objects(lasts)
    bk.select(shafts + lasts, shafts[0])
    bpy.ops.object.join()
    ob = bpy.context.view_layer.objects.active
    ob.name = ob.data.name = 'Boots'
    m = ob.modifiers.new('fuse', 'REMESH')
    m.mode, m.voxel_size, m.adaptivity = 'VOXEL', BOOT_VOXEL, 0.0
    gm.apply_modifiers(ob, keep=())
    co = gm._object_coords(ob)
    gap = np.ones(len(co))
    for i, p in enumerate(co):
        if p[2] < 0.16:
            v = Vector(tuple(p))
            gap[i] = max(shaft_tree.find_nearest(v)[3], last_tree.find_nearest(v)[3])
    fillet(ob, gm.smoothstep(0.022, 0.004, gap))
    m = ob.modifiers.new('reduce', 'DECIMATE')
    m.ratio = min(1.0, 2 * BOOT_TRIS / max(1, sum(len(p.vertices) - 2 for p in ob.data.polygons)))
    gm.apply_modifiers(ob, keep=())
    ob.data.shade_smooth()
    # the stacked soles: under each boot's footprint, out 4 mm into a welt
    co = gm._object_coords(ob)
    soles = []
    for s in 'lr':
        sx = 1.0 if s == 'l' else -1.0
        ankle, f, lat = foot_frame(fig, s)
        side = co[co[:, 0] * sx > 0]
        foot_pts = side[side[:, 2] < insole((side - ankle) @ f) + 0.008]
        mid = foot_pts[:, :2].mean(axis=0)
        ang = np.linspace(-math.pi, math.pi, 72, endpoint=False)
        rel = foot_pts[:, :2] - mid
        a = np.arctan2(rel[:, 1], rel[:, 0])
        r = np.linalg.norm(rel, axis=1)
        out = np.zeros(len(ang))
        for i, t in enumerate(ang):
            dd = np.abs((a - t + math.pi) % (2 * math.pi) - math.pi)
            near = dd < math.radians(9)
            out[i] = r[near].max() if near.any() else 0.0
        for _ in range(3):
            out = 0.25 * np.roll(out, 1) + 0.5 * out + 0.25 * np.roll(out, -1)
        out += 0.004
        ring = np.stack([mid[0] + out * np.cos(ang), mid[1] + out * np.sin(ang)], 1)
        s_ring = (np.c_[ring, np.zeros(len(ring))] - ankle) @ f
        top = insole(s_ring) + 0.002
        heel = s_ring < 0.035
        arch = gm.smoothstep(0.03, 0.045, s_ring) * gm.smoothstep(0.11, 0.085, s_ring)
        bottom = 0.011 * arch
        inward = np.where(heel, 0.003, 0.0)
        rel_r = ring - mid
        ring_b = ring - gm.unit(np.c_[rel_r, np.zeros(len(rel_r))])[:, :2] * inward[:, None]
        X = np.stack([ring[:, 0], ring[:, 0], ring_b[:, 0]])
        Y = np.stack([ring[:, 1], ring[:, 1], ring_b[:, 1]])
        Z = np.stack([top, top - 0.006, bottom])
        sole = gm.grid(X, Y, Z, wrap=True)
        verts = list(sole.verts)
        faces, uv = sole.faces, sole.uv
        n = len(ang)
        for row, zc in ((0, top.mean()), (2, 0.0)):
            cidx = len(verts)
            verts.append(np.array([mid[0], mid[1], zc if row == 0 else 0.0]))
            for i in range(n):
                a_, b_ = row * n + i, row * n + (i + 1) % n
                faces.append([a_, b_, cidx] if row == 0 else [b_, a_, cidx])
                uv += [(0.0, 0.0), (0.01, 0.0), (0.005, 0.01)]
        soles.append(gm.Piece(np.array(verts), faces, uv))
        M.log(f'boot {s}: toe tip at y {float(foot_pts[:, 1].min()):.3f}, heel at y {float(foot_pts[:, 1].max()):.3f}')
    sole_ob = gm.mesh('Soles', soles[0] + soles[1], M.slot('Soles'))
    laces, eyelets = boot_laces(M, fig, ob)
    return [ob, sole_ob, laces, eyelets]


def boot_laces(M, fig, boot):
    """The lacing up each boot's front: iron hooks either side of the slit from the instep to the top, a leather lace
    crossing between them and tied off at the top."""
    tree = gm.bvh_objects([boot])
    hooks, lace = None, None
    for s in 'lr':
        ankle, f, lat = foot_frame(fig, s)
        pairs = []
        for k, z in enumerate(np.linspace(0.072, M.BOOT_TOP - 0.012, 6)):
            row = []
            for side in (-1.0, 1.0):
                # a ray from the front onto the instep and the shaft, beside the slit
                o = ankle + lat * (side * 0.0115) + np.array([0.0, 0.0, z - ankle[2]]) + f * 0.25
                hit = tree.ray_cast(Vector(tuple(o)), Vector(tuple(-f)), 0.4)
                if hit[0] is None:
                    row = None
                    break
                row.append((np.array(hit[0]), np.array(hit[1])))
            if row:
                pairs.append(row)
        for i, row in enumerate(pairs):
            for p, n in row:
                h = gm.tube(np.array([p - n * 0.001, p + n * 0.0035]), np.array([0.0028, 0.0022]), n=8, cap1=True)
                hooks = h if hooks is None else hooks + h
        for i in range(len(pairs) - 1):
            for a, b in ((0, 1), (1, 0)):
                p0, n0 = pairs[i][a]
                p1, n1 = pairs[i + 1][b]
                mid = (p0 + p1) * 0.5 + gm.unit(n0 + n1) * 0.004
                path = np.array([p0 + n0 * 0.003, mid, p1 + n1 * 0.003])
                t = gm.tube(path, np.array([0.0016] * 3), n=6)
                lace = t if lace is None else lace + t
        # tied off at the top: a small knot and two short ends
        if pairs:
            (pa, na), (pb, nb) = pairs[-1]
            k = (pa + pb) * 0.5 + gm.unit(na + nb) * 0.006
            for side in (-1.0, 1.0):
                end = np.array([k, k + lat * side * 0.012 - f * 0.004 + np.array([0, 0, -0.012]),
                                k + lat * side * 0.016 - f * 0.008 + np.array([0, 0, -0.03])])
                t = gm.tube(end, np.array([0.0018, 0.0016, 0.0014]), n=6, cap1=True)
                lace = lace + t
            ring = np.array([k + (lat * math.cos(a) * 0.005 + gm.unit(na + nb) * math.sin(a) * 0.003) for a in
                             np.linspace(0, 2 * math.pi, 9)])
            lace = lace + gm.tube(ring, np.array([0.002] * 9), n=6)
    return gm.mesh('Laces', lace, M.slot('Lace')), gm.mesh('Eyelets', hooks, M.slot('Iron'))


# ------------------------------------------------------------------------------------------------ waistcoat, shirt

def build_waistcoat(M, fig):
    """A brown tweed waistcoat (and the linen shirt in its V): the torso pushed out a little and smoothed, cut to a V at
    the throat, its hem level round the back and pointed at the front below the belt, brass buttons."""
    z = fig.wv[:, 2]
    x = fig.wv[:, 0]
    neck = fig.neck[2]
    front = fig.wv[:, 1] < fig.centre[1] - 0.02
    torso = (fig.w_arm < 0.25) & (fig.w_hand < 0.05) & ~neck_column(fig, 0.004)
    # its V no lower than the neckerchief's point (lower, the shirt in it showed ragged under the kerchief)
    hw = 0.050 * gm.smoothstep(neck - 0.13, neck - 0.02, z)
    vee = front & (np.abs(x) < hw)
    keep = torso & (z > M.BELT - 0.09) & (z < neck - 0.004) & ~vee
    vest, _ = gm.shell(fig.wv, fig.faces, fig.fuv, fig.face_mask(keep), 0.0075, iters=14)
    vv = vest.verts
    hem = gm.boundary(vest.faces, len(vv)) & (vv[:, 2] < M.BELT - 0.02)
    fr = vv[:, 1] < fig.centre[1] - 0.02
    vv[hem, 2] = M.BELT - 0.05 - 0.045 * fr[hem] * np.exp(-((np.abs(vv[hem, 0]) - 0.045) / 0.045) ** 2)
    vest.verts = gm.laplacian(vv, vest.faces, 2, weight=(vv[:, 2] < M.BELT + 0.02) * 0.5)
    ob = gm.mesh('Waistcoat', vest, M.slot('Waistcoat'))
    shirt_keep = torso & front & (np.abs(x) < 0.11) & (z > neck - 0.30) & (z < neck + 0.012)
    shirt, _ = gm.shell(fig.wv, fig.faces, fig.fuv, fig.face_mask(shirt_keep), 0.003, iters=3)
    sh = gm.mesh('Shirt', shirt, M.slot('Shirt'))
    tree = gm.bvh_objects([ob])
    buttons = []
    for k in range(6):
        zz = neck - 0.215 - k * 0.042
        hit = tree.ray_cast(Vector((0.010, -0.5, zz)), Vector((0, 1, 0)), 1.0)
        if hit[0] is None:
            continue
        buttons.append(place_button('VestButton', np.array(hit[0] + hit[1] * 0.003), np.array(hit[1]), M.slot('Brass'),
                                    radius=0.0052))
    return ob, sh, buttons


# ----------------------------------------------------------------------------------------------------------- the coat

SLEEVE_END = 0.012       # the sleeve stops this far up the forearm from the wrist (m); the cuff over its end
COAT_EASE = (0.017, 0.007)   # off the body (m): from the chest down, and more toward the belt (his belly)


def opening(M, fig, z):
    """Half the coat's opening across the front (m) at height z: narrow at the throat, wide over the chest where the
    lapels turn back, drawn in at the belt, parting again below it."""
    sz, neck, b = fig.shoulder_z, fig.neck[2], M.BELT
    return np.interp(z, [M.HEM - 0.05, b - 0.06, b, b + 0.12, sz - 0.17, sz - 0.05, neck + 0.02],
                     [0.115, 0.058, 0.046, 0.066, 0.086, 0.080, 0.062])


def subdivide(piece, attrs=()):
    """A piece split once more: every face into quads at its edges' middles and its centre (no smoothing), the UVs and
    any per-vertex attributes (arrays) carried along. Returns (piece, attrs)."""
    V = np.asarray(piece.verts)
    out_v = [p for p in V]
    out_a = [list(np.asarray(a)) for a in attrs]
    edge = {}

    def mid(a, b):
        key = (min(a, b), max(a, b))
        if key not in edge:
            edge[key] = len(out_v)
            out_v.append((V[a] + V[b]) * 0.5)
            for arr, src in zip(out_a, attrs):
                arr.append((src[a] + src[b]) * 0.5)
        return edge[key]
    faces, uv, k = [], [], 0
    for f in piece.faces:
        n = len(f)
        uvs = [np.asarray(q) for q in piece.uv[k:k + n]]
        k += n
        c = len(out_v)
        out_v.append(V[f].mean(0))
        for arr, src in zip(out_a, attrs):
            arr.append(np.asarray(src)[f].mean(0))
        cu = sum(uvs) / n
        mids = [mid(f[i], f[(i + 1) % n]) for i in range(n)]
        mu = [(uvs[i] + uvs[(i + 1) % n]) * 0.5 for i in range(n)]
        for i in range(n):
            faces.append([f[i], mids[i], c, mids[i - 1]])
            uv += [tuple(uvs[i]), tuple(mu[i]), tuple(cu), tuple(mu[i - 1])]
    return gm.Piece(np.array(out_v), faces, uv), [np.array(a) for a in out_a]


def drop_faces(piece, attrs, gone):
    """The piece without the faces where gone (bool per face), its unused vertices dropped."""
    keep = [i for i, g in enumerate(gone) if not g]
    k, starts = 0, []
    for f in piece.faces:
        starts.append(k)
        k += len(f)
    used = np.unique(np.concatenate([piece.faces[i] for i in keep]))
    remap = -np.ones(len(piece.verts), dtype=np.int64)
    remap[used] = np.arange(len(used))
    faces = [[int(remap[v]) for v in piece.faces[i]] for i in keep]
    uv = [piece.uv[starts[i] + j] for i in keep for j in range(len(piece.faces[i]))]
    return gm.Piece(piece.verts[used], faces, uv), [np.asarray(a)[used] for a in attrs]


def build_coat(M, fig, under):
    """The field coat's body and sleeves in one: the torso and arms from the belt (under it) to the neck pushed out
    and smoothed until the anatomy is gone (MakeHuman's UVs come with it), split twice finer (MakeHuman's 3 cm faces
    cut a ragged opening), open down the front (opening), the opening's edge drawn clean; the sleeves looser toward the
    cuff, folded at the elbow and bunched above the cuff, ending SLEEVE_END up from the wrist."""
    z = fig.wv[:, 2]
    neck = fig.neck[2]
    keep = (z > M.BELT - 0.02) & (z < neck + 0.020) & ~neck_column(fig) & (fig.w_hand < 0.1)
    keep &= ~(fig.w_leg > 0.5)
    ends = {}
    for s, sx in (('l', 1.0), ('r', -1.0)):
        e, w = fig.sk[f'lowerarm_{s}'][0], fig.sk[f'lowerarm_{s}'][1]
        L = np.linalg.norm(w - e)
        t_end = 1.0 - SLEEVE_END / L
        ends[s] = t_end
        t = fig.forearm_share(s, fig.wv)
        keep &= ~((fig.wv[:, 0] * sx > 0.05) & (t > t_end + 0.06))
    # the offset: the torso's ease, looser toward the belt; the arms' looser toward the cuff, gathered at it
    along = np.zeros(len(fig.wv))
    for s, sx in (('l', 1.0), ('r', -1.0)):
        a, w = fig.sk[f'upperarm_{s}'][0], fig.sk[f'lowerarm_{s}'][1]
        t = np.clip(((fig.wv - a) @ gm.unit(w - a)) / np.linalg.norm(w - a), 0, 1)
        along = np.where(fig.wv[:, 0] * sx > 0, t, along)
    torso_off = COAT_EASE[0] + COAT_EASE[1] * gm.smoothstep(fig.shoulder_z - 0.1, M.BELT + 0.05, z)
    arm_off = 0.019 + 0.012 * along
    for s, sx in (('l', 1.0), ('r', -1.0)):
        e, w = fig.sk[f'lowerarm_{s}'][0], fig.sk[f'lowerarm_{s}'][1]
        up = -((fig.wv - w) @ gm.unit(w - e))
        gather = gm.smoothstep(SLEEVE_END, SLEEVE_END + 0.07, up)
        arm_off = np.where(fig.wv[:, 0] * sx > 0, 0.011 + (arm_off - 0.011) * gather, arm_off)
    k = np.clip(fig.w_arm, 0.0, 1.0)
    off = torso_off * (1.0 - k) + arm_off * k
    piece, used = gm.shell(fig.wv, fig.faces, fig.fuv, fig.face_mask(keep), off, iters=16)
    attrs = [fig.w_arm[used]]
    for _ in range(2):
        piece, attrs = subdivide(piece, attrs)
    w_arm = attrs[0]
    v = piece.verts
    # the opening: the faces whose middle lies in it go, then its edge drawn onto the line
    cen = np.array([v[f].mean(0) for f in piece.faces])
    fa = np.array([w_arm[f].mean() for f in piece.faces])
    gone = (cen[:, 1] < fig.centre[1] - 0.01) & (np.abs(cen[:, 0]) < opening(M, fig, cen[:, 2])) & (fa < 0.3)
    # the sleeves' ends: the faces past each one go
    for s, sx in (('l', 1.0), ('r', -1.0)):
        t = fig.forearm_share(s, cen)
        gone |= (cen[:, 0] * sx > 0.05) & (t > ends[s]) & (fa > 0.3)
    piece, (w_arm,) = drop_faces(piece, [w_arm], gone)
    v = piece.verts
    nb = gm.boundary(piece.faces, len(v))
    g = opening(M, fig, v[:, 2])
    edge = nb & (v[:, 1] < fig.centre[1] - 0.005) & (v[:, 2] > M.BELT - 0.01) & (v[:, 2] < neck - 0.004) & \
        (np.abs(v[:, 0]) < g + 0.012) & (w_arm < 0.3)
    v[edge, 0] = np.sign(v[edge, 0]) * g[edge]
    for s, sx in (('l', 1.0), ('r', -1.0)):
        e, w = fig.sk[f'lowerarm_{s}'][0], fig.sk[f'lowerarm_{s}'][1]
        ax = gm.unit(w - e)
        L = np.linalg.norm(w - e)
        t = ((v - e) @ ax) / L
        cuff = nb & (v[:, 0] * sx > 0.15) & (t > ends[s] - 0.06) & (w_arm > 0.3)
        v[cuff] -= np.outer((t[cuff] - ends[s]) * L, ax)
    v = gm.laplacian(v, piece.faces, 4, pinned=nb)
    nrm = gm.vertex_normals(v, piece.faces)
    # folds: rings bunched above the cuffs and crescents in the crook of the elbows; the coat gathered above the belt
    for s, sx in (('l', 1.0), ('r', -1.0)):
        el, w = fig.sk[f'lowerarm_{s}'][0], fig.sk[f'lowerarm_{s}'][1]
        side = (v[:, 0] * sx > 0.12) & (w_arm > 0.4)
        d = gm.unit(w - el)
        t = (v - el) @ d
        ring = np.cos(t / 0.05 * 2 * math.pi + 0.8 * np.sin(np.arctan2(v[:, 2] - el[2], v[:, 1] - el[1]) * 2)) * 0.5 + 0.5
        bunch = gm.smoothstep(0.10, 0.20, t) * 0.008 * ring ** 2
        crook = np.exp(-(t / 0.06) ** 2) * 0.006 * np.sin(t * 90)
        v[side] += nrm[side] * (bunch[side] + crook[side])[:, None]
    torso = w_arm < 0.3
    ang = round_angle(v, fig.centre)
    gather = gm.smoothstep(M.BELT + 0.10, M.BELT + 0.02, v[:, 2]) * gm.smoothstep(M.BELT - 0.03, M.BELT + 0.01, v[:, 2])
    pleat = 0.0035 * gather * (0.6 * np.sin(ang * 14.0 + 0.7) + 0.4 * np.sin(ang * 23.0 + 2.1))
    drag = 0.0025 * np.sin((v[:, 2] - 0.9 * np.abs(v[:, 0])) * 70.0) * gm.smoothstep(0.09, 0.15, np.abs(v[:, 0])) * \
        gm.smoothstep(fig.shoulder_z - 0.30, fig.shoulder_z - 0.18, v[:, 2]) * gm.smoothstep(fig.shoulder_z - 0.02,
                                                                                               fig.shoulder_z - 0.12, v[:, 2])
    v[torso] += nrm[torso] * (pleat + drag)[torso][:, None]
    piece.verts = v
    piece.verts = gm.clear(piece.verts, gm.bvh_objects(under), 0.006, 0.05)
    ob = gm.mesh('Coat', piece, M.slot('Coat'))
    ob.data.attributes.new('ArmShare', 'FLOAT', 'POINT').data.foreach_set('value', w_arm.astype(np.float32))
    # the sewn seams: down the sides, round the armholes, down the back
    co = gm._object_coords(ob)
    a = round_angle(co, fig.centre)
    rad = np.linalg.norm(co[:, :2] - fig.centre, axis=1)
    wrap = lambda x: np.abs((x + math.pi) % (2 * math.pi) - math.pi)
    sides = np.minimum(wrap(a - M.SIDE_SEAM), wrap(a + M.SIDE_SEAM)) * rad
    back = wrap(a - math.pi) * rad
    armhole = np.abs(w_arm - 0.5) * 0.25
    keep_seam_dist(ob, np.where(w_arm > 0.15, armhole, np.minimum(sides, back)))
    M.log(f'coat: {len(co)} points; sleeves end {SLEEVE_END * 100:.1f} cm up the forearm')
    return ob


def skirt_gap(M, z):
    """Half the skirt's opening across the front at height z (m): buttoned over (negative: the left front laps over
    the right) from the belt down, parting only near the hem."""
    return np.interp(z, [M.HEM - 0.01, M.HEM + 0.10, M.BELT - 0.02, M.BELT + 0.02], [0.028, -0.004, -0.020, -0.020])


def build_skirt(M, fig, under_obs, over=()):
    """The coat's skirt from under the belt to mid-thigh: hung round the hips (each column outside the widest point at
    or above it from the belt down: the belt cinches the coat, the skirt flares from there), parting at the front,
    a little fuller toward the hem, in broad uneven folds; the hem uneven."""
    parts_extra = []
    pts = np.concatenate([gm.object_points(o) for o in under_obs])
    pts = pts[(pts[:, 2] < M.BELT + 0.05) & (pts[:, 2] > M.HEM - 0.05)]
    pts = pts[np.abs(pts[:, 0]) < 0.32]
    top, hem = M.BELT + 0.012, M.HEM
    n_z, n_t = 30, 112
    fine = np.linspace(-math.pi, math.pi, 720, endpoint=False)
    rng = np.random.default_rng(31)
    flare = lambda th, v: 0.017 + 0.006 * gm.smoothstep(0.0, 0.3, v) + 0.034 * gm.smoothstep(0.15, 1.0, v) ** 1.3 * \
        (0.5 + 0.5 * np.abs(np.sin(th)))
    X, Y, Z, R = gm.drape(pts, fig.centre, M.FWD, fine, lambda th: top, lambda th: hem, n_z, 0.0,
                          hb=math.radians(7.0), zb=0.01, smooth=(10, 3), wrap=True, flare=flare)
    zs = Z[:, 0]
    ph = rng.random(8) * 6.28
    rows = []
    for j, z in enumerate(zs):
        r_front = np.interp(0.0, fine, R[j], period=2 * math.pi)
        g = float(skirt_gap(M, z))
        a = math.asin(float(np.clip(g / max(r_front, 0.05), -0.5, 0.98)))
        th = np.linspace(a, 2 * math.pi - a, n_t)
        r = np.interp(th, fine, R[j], period=2 * math.pi)
        if a < 0.0:
            # buttoned over: the left front laps over the right, standing off it
            r = r + 0.0045 * gm.smoothstep(-a + 0.06, -a - 0.01, th) - 0.002 * gm.smoothstep(2 * math.pi + a - 0.06,
                                                                                               2 * math.pi + a + 0.01, th)
        v = (top - z) / (top - hem)
        fold = 0.011 * v ** 1.2 * (0.45 * np.sin(th * 5.0 + ph[0] + 0.7 * np.sin(v * 2.0 + ph[1]))
                                   + 0.35 * np.sin(th * 8.3 + ph[2] + 0.9 * np.sin(v * 2.6 + ph[3]))
                                   + 0.20 * np.sin(th * 13.7 + ph[4] + v * 1.5))
        rows.append((th, r + fold, z))
    TH = np.array([q[0] for q in rows])
    RR = np.array([q[1] for q in rows])
    ZZ = np.array([[q[2]] * n_t for q in rows])
    ZZ[-1] += 0.008 * np.sin(TH[-1] * 7 + 1.0) - 0.010 * np.cos(TH[-1]) ** 2 * (np.cos(TH[-1]) < 0)
    dx, dy = gm.radial(TH, M.FWD)
    X = fig.centre[0] + RR * dx
    Y = fig.centre[1] + RR * dy
    P = np.stack([X, Y, ZZ], -1).reshape(-1, 3)
    # outside the coat's foot under the belt and clear of the breeches and the waistcoat's points
    P = gm.clear(P, gm.bvh_objects([o for o in under_obs if not o.name.startswith('_')] + list(over)), 0.004, 0.05)
    P = P.reshape(X.shape + (3,))
    ob = gm.mesh('Skirt', gm.grid(P[..., 0], P[..., 1], P[..., 2]), M.slot('Coat'))
    face_out(ob, fig.centre)
    co = gm._object_coords(ob)
    a = round_angle(co, fig.centre)
    rad = np.linalg.norm(co[:, :2] - fig.centre, axis=1)
    wrap = lambda x: np.abs((x + math.pi) % (2 * math.pi) - math.pi)
    keep_seam_dist(ob, np.minimum(np.minimum(wrap(a - M.SIDE_SEAM), wrap(a + M.SIDE_SEAM)), wrap(a - math.pi)) * rad)
    # two brass buttons down the lapped front
    tree = gm.bvh_objects([ob])
    for k, z in enumerate((M.BELT - 0.065, M.BELT - 0.145)):
        g = float(skirt_gap(M, z))
        hit = tree.ray_cast(Vector((-g * 0.2 + 0.016, -0.6, z)), Vector((0, 1, 0)), 1.5)
        if hit[0] is not None:
            parts_extra.append(place_button(f'CoatButton_skirt{k}', np.array(hit[0] + hit[1] * 0.003), np.array(hit[1]),
                                            M.slot('Brass'), 0.0058))
    M.log(f'skirt: {top:.3f} to {hem:.3f} m, {n_z} rows of {n_t}')
    return ob


def sleeve_end(fig, coat, s):
    """The sleeve's end on side s: its centre on the forearm's axis, the axis, the frame across it and the end ring's
    half-widths (across, front to back)."""
    e, w = fig.sk[f'lowerarm_{s}'][0], fig.sk[f'lowerarm_{s}'][1]
    ax = gm.unit(w - e)
    L = np.linalg.norm(w - e)
    t_end = 1.0 - SLEEVE_END / L
    c = e + ax * (t_end * L)
    a = gm.unit(np.cross(-ax, [0.0, -1.0, 0.0]))
    b = np.cross(-ax, a)
    co = gm._object_coords(coat)
    sx = 1.0 if s == 'l' else -1.0
    near = co[(co[:, 0] * sx > 0.15) & (np.abs((co - c) @ ax) < 0.03) & (np.linalg.norm(np.cross(co - c, ax), axis=1) < 0.09)]
    q = near - c
    return c, ax, a, b, np.abs(q @ a).max(), np.abs(q @ b).max()


def build_cuffs(M, fig, coat):
    """Turned-back cuffs over each sleeve's end: a band 4 cm deep standing a few mm off the sleeve, its lip rolled,
    a tab with a brass button on the outside."""
    parts = []
    for s in 'lr':
        c, ax, a, b, ra, rb = sleeve_end(fig, coat, s)
        steps = np.linspace(0.0, 1.0, 6)
        path = np.array([c + ax * (0.004 - 0.042 * t) for t in steps])
        rr = np.array([[ra + 0.0045 + 0.0015 * (t < 0.1), rb + 0.0045 + 0.0015 * (t < 0.1)] for t in steps])
        piece = gm.tube(path, rr, n=26, up=(0.0, -1.0, 0.0))
        cuff = gm.mesh(f'Cuff_{s}', piece, M.slot('Coat'))
        parts.append(cuff)
        # the tab and its button, on the outside of the cuff (away from the body), a little to the back
        sx = 1.0 if s == 'l' else -1.0
        out = gm.unit(np.array([sx, 0.25, 0.1]))
        out = gm.unit(out - ax * (out @ ax))
        tree = gm.bvh_objects([cuff])
        mid = c - ax * 0.02
        hit = tree.ray_cast(Vector(tuple(mid + out * 0.15)), Vector(tuple(-out)), 0.3)
        at = np.array(hit[0]) if hit[0] is not None else mid + out * (ra + 0.006)
        side = gm.unit(np.cross(out, ax))
        # rows from the elbow's side toward the hand: the grid's faces then face out (the other way round they faced the
        # cuff, baked its shadow, near black, and Unreal culled them from outside)
        tab = np.array([[at - ax * 0.012 + side * w_ + out * 0.003 for w_ in (-0.009, 0.0, 0.009)],
                        [at + ax * 0.018 + side * w_ + out * 0.0025 for w_ in (-0.009, 0.0, 0.009)]])
        parts.append(gm.mesh(f'CuffTab_{s}', gm.grid(tab[..., 0], tab[..., 1], tab[..., 2]), M.slot('Coat')))
        parts.append(place_button(f'CoatButton_cuff{s}', at - ax * 0.002 + out * 0.006, out, M.slot('Brass'), 0.0048))
    return parts


# --------------------------------------------------------------------------------------------- collar and lapels

def neckline(fig, coat):
    """The coat's neckline as (angles, radii, heights) round neck_01's axis, sorted by angle, smoothed: in each 5-degree
    wedge the coat's boundary point nearest the neck (its inner edge round the neck; the opening's edges below it left
    out)."""
    co = gm._object_coords(coat)
    nb = gm.boundary([list(p.vertices) for p in coat.data.polygons], len(co))
    top = co[nb & (co[:, 2] > fig.shoulder_z + 0.02)]
    a = round_angle(top, fig.neck)
    r = np.linalg.norm(top[:, :2] - fig.neck[:2], axis=1)
    bins = np.floor((a + math.pi) / (2 * math.pi) * 72).astype(int) % 72
    pts = []
    for b_ in range(72):
        sel = (bins == b_) & (r < 0.16) & (top[:, 2] > fig.neck[2] - 0.05)
        if sel.any():
            k = np.nonzero(sel)[0][np.argmin(r[sel])]
            pts.append(top[k])
    pts = np.array(pts)
    a = round_angle(pts, fig.neck)
    o = np.argsort(a)
    r = np.linalg.norm(pts[o, :2] - fig.neck[:2], axis=1)
    return a[o], r, pts[o, 2]


COLLAR_FROM = 34.0       # degrees from the front where the collar starts on each side (it meets the lapels' tops)


def build_collar(M, fig, coat):
    """A low collar round the back of the neck: a stand from the coat's neckline up 1.4-2.8 cm (highest at the back),
    leaning out a little, its top edge turned over; its ends by the throat where the lapels start."""
    tree = gm.bvh_objects([coat])
    na, nr, nz = neckline(fig, coat)
    neck = fig.neck
    # from where the neckline starts beside the opening, round the back
    start = max(COLLAR_FROM, math.degrees(np.abs(na).min()) + 2.0)
    th = np.radians(np.linspace(start, 360.0 - start, 72))
    th = np.where(th > math.pi, th - 2 * math.pi, th)
    # its foot on the coat's neckline (its inner edge round the neck, per angle, smoothed round)
    order = np.argsort(np.where(na < 0, na + 2 * math.pi, na))
    na_c = np.where(na < 0, na + 2 * math.pi, na)[order]
    on = lambda a, v: float(np.interp(a if a >= 0 else a + 2 * math.pi, na_c, v[order]))
    rs = np.array([on(a, nr) for a in th])
    zs = np.array([on(a, nz) for a in th])
    for k in range(4):
        rs[1:-1] = 0.25 * rs[:-2] + 0.5 * rs[1:-1] + 0.25 * rs[2:]
        zs[1:-1] = 0.25 * zs[:-2] + 0.5 * zs[1:-1] + 0.25 * zs[2:]
    M.log(f'collar: its foot {rs.min() * 100:.1f}..{rs.max() * 100:.1f} cm from the neck, {zs.min():.3f}..{zs.max():.3f} m; '
          f'at the back {rs[len(rs) // 2] * 100:.1f} cm, {zs[len(zs) // 2]:.3f} m')
    cols = []
    for a, r0, z0 in zip(th, rs, zs):
        back = 0.5 - 0.5 * math.cos(a)
        end = float(gm.smoothstep(math.radians(start + 16), math.radians(start), min(abs(a), 2 * math.pi - abs(a))))
        hs = 0.020 + 0.008 * back - 0.006 * end

        # a stand leaning out a little, its top edge turned over 1.2 cm (with a fall down over the shoulders it read as
        # a rolled ring round his neck, a neck pillow)
        prof = [(r0 - 0.0015, z0 - 0.006), (r0 + 0.001, z0 + 0.40 * hs), (r0 + 0.004, z0 + 0.82 * hs),
                (r0 + 0.008, z0 + hs), (r0 + 0.0125, z0 + hs - 0.004), (r0 + 0.0145, z0 + hs - 0.012)]
        d = np.array([math.sin(a), -math.cos(a)])
        cols.append([(neck[0] + d[0] * r, neck[1] + d[1] * r, z) for r, z in prof])
    P = np.array(cols).transpose(1, 0, 2)[::-1]           # rows from the fall's edge in to the neckline
    ob = gm.mesh('Collar', gm.grid(P[..., 0], P[..., 1], P[..., 2]), M.slot('Coat'))
    # the turned edge (the first rows' faces) faces out: it's what shows; the stand under it then faces the neck
    me = ob.data
    nfall = 2 * (P.shape[1] - 1)
    score = 0.0
    for p in list(me.polygons)[:nfall]:
        c = np.array(p.center)
        score += float(np.array(p.normal) @ np.array([c[0] - neck[0], c[1] - neck[1], 0.0])) * p.area
    if score < 0.0:
        bm = bmesh.new()
        bm.from_mesh(me)
        bmesh.ops.reverse_faces(bm, faces=bm.faces[:])
        bm.to_mesh(me)
        bm.free()
        me.update()
    return ob


LAPEL_DROP = 0.21        # the lapels run this far down from their top (m)
LAPEL_TOP = 0.045        # their top (the notch where the collar meets them) this far under neck_01's joint (m)


def build_lapels(M, fig, coat):
    """Lapels turned back either side of the opening from the collar's ends down to their points, lying on the coat
    (their fold rolled a little off it), notched where they meet the collar."""
    tree = gm.bvh_objects([coat])
    neck = fig.neck[2]
    parts = []
    for side in (-1.0, 1.0):
        rows = []
        for j in range(14):
            t = j / 13
            z = neck - LAPEL_TOP - t * LAPEL_DROP
            g = float(opening(M, fig, z))
            w = 0.054 * (1.0 - t) ** 0.85 * (0.72 if t < 0.09 else 1.0) + 0.004
            rows.append([(side * (g + 0.003 + w * u), z + 0.010 * u * (1 - t) ** 2, 0.0035 * (1.0 - u) ** 2) for u in
                         np.linspace(0.0, 1.0, 6)])
        ob = project_grid(tree, rows, 0.003, f'Lapel_{"l" if side > 0 else "r"}', M.slot('Coat'))
        face_out(ob, fig.centre)
        parts.append(ob)
    return parts


# ----------------------------------------------------------------------------------------------------- the pockets

def build_pockets(M, fig, coat, skirt):
    """Patch pockets with buttoned flaps: two on the chest, two big ones on the skirt; a pencil in the left chest
    pocket's slot."""
    sz = fig.shoulder_z
    parts = []
    specs = [('chest', coat, 0.100, 0.172, sz - 0.225, sz - 0.108),
             ('hip', skirt, 0.112, 0.214, M.HEM + 0.05, M.BELT - 0.115)]
    for kind, host, x0, x1, z0, z1 in specs:
        tree = gm.bvh_objects([host])
        for side in (-1.0, 1.0):
            s = 'l' if side > 0 else 'r'
            rows = []
            for j in range(8):
                v = j / 7
                z = z1 - (z1 - z0) * v
                row = []
                for i in range(7):
                    u = i / 6
                    # the bottom corners rounded
                    corner = max(0.0, (v - 0.82) / 0.18) * max(0.0, abs(u - 0.5) * 2 - 0.7) / 0.3
                    x = side * (x0 + (x1 - x0) * (u - 0.5 * corner * 0.0))
                    row.append((x, z + 0.012 * corner ** 2, 0.0))
                rows.append(row)
            pocket = project_grid(tree, rows, 0.003, f'Pocket_{kind}{s}', M.slot('Coat'))
            face_out(pocket, fig.centre)
            parts.append(pocket)
            ptree = gm.bvh_objects([host, pocket])
            frows = []
            fz0, fz1 = z1 - 0.042, z1 + 0.010
            for j in range(5):
                v = j / 4
                row = []
                for i in range(7):
                    u = i / 6
                    point = 0.010 * (1.0 - abs(u - 0.5) * 2) * v      # the flap's edge pointed at its middle
                    row.append((side * (x0 - 0.004 + (x1 - x0 + 0.008) * u), fz1 - (fz1 - fz0) * v - point,
                                0.0025 * v))
                frows.append(row)
            flap = project_grid(ptree, frows, 0.0035, f'Flap_{kind}{s}', M.slot('Coat'))
            face_out(flap, fig.centre)
            parts.append(flap)
            bx = side * (x0 + x1) * 0.5
            hit = gm.bvh_objects([flap]).ray_cast(Vector((bx, -0.6, fz0 + 0.004)), Vector((0, 1, 0)), 1.5)
            if hit[0] is not None:
                parts.append(place_button(f'CoatButton_{kind}{s}', np.array(hit[0] + hit[1] * 0.003), np.array(hit[1]),
                                          M.slot('Brass'), 0.0055))
            if kind == 'chest' and side > 0:
                # the pencil, in a slot sewn at the pocket's inner side, its point up past the flap
                px = x0 + 0.010
                h0 = tree.ray_cast(Vector((px, -0.6, z1 - 0.03)), Vector((0, 1, 0)), 1.5)
                h1 = tree.ray_cast(Vector((px - 0.002, -0.6, z1 + 0.038)), Vector((0, 1, 0)), 1.5)
                if h0[0] is not None and h1[0] is not None:
                    a = np.array(h0[0] + h0[1] * 0.009)
                    b = np.array(h1[0] + h1[1] * 0.009)
                    d = gm.unit(b - a)
                    pen = gm.tube(np.array([a, b - d * 0.012, b - d * 0.011, b + d * 0.004]),
                                  np.array([0.0038, 0.0038, 0.0036, 0.0006]), n=6, cap0=True, cap1=True)
                    parts.append(gm.mesh('Pencil', pen, M.slot('Wood')))
    return parts


# ------------------------------------------------------------------------------------------------- the neckerchief

KERCHIEF_FRONT = math.radians(50.0)   # within this of the front the neckerchief lies over the clothes, beyond it on the neck


def build_kerchief(M, fig, under_obs):
    """The teal neckerchief round the throat over the collar, higher at the back, its point down the chest; a knot in
    front with two short tails (the portrait's build_kerchief, in the figure's space)."""
    neck = fig.neck
    centre = neck[:2] + np.array([0.0, -0.012])
    # in front over everything near the throat (the collar's ends, the lapels, the shirt); round the sides and the back
    # on the neck itself, inside the collar (draped over the collar it stood round his neck like a ring): under_obs[0]
    # is the skin
    pts = []
    for k, o in enumerate(under_obs):
        q = gm.object_points(o)
        q = q[np.linalg.norm(q[:, :2] - neck[:2], axis=1) < 0.12]
        if k > 0:
            q = q[np.abs(round_angle(q, centre)) < KERCHIEF_FRONT]
        pts.append(q)
    pts = np.concatenate(pts)
    s = fig.b.scale
    cz = fig.chin_z
    th = np.linspace(-math.pi, math.pi, 120, endpoint=False)
    top = lambda t: cz - 0.030 + 0.010 * (0.5 - 0.5 * math.cos(t))
    bot = lambda t: top(t) - (0.040 + 0.068 * max(math.cos(t), 0.0) ** 3) * s
    X, Y, Z, R = gm.drape(pts, centre, M.FWD, th, top, bot, 22, 0.008, hb=math.radians(5), zb=0.01, smooth=(3, 2),
                          wrap=True)
    v = np.linspace(0, 1, Z.shape[0])[:, None]
    rng = np.random.default_rng(13)
    ph = rng.random(4) * 6.28
    sag = 0.0045 * np.sin(v * 15 + th[None] * 2 + ph[0]) + 0.003 * np.sin(th[None] * 9 + v * 3 + ph[1])
    front = np.clip(1.0 - np.abs(th[None, :]) / 1.2, 0, 1)
    sag += 0.003 * np.sin(v * 22 + th[None] * 4 + ph[2]) * front * v
    dx, dy = gm.radial(th[None, :], M.FWD)
    ob = gm.mesh('Kerchief', gm.grid(X + sag * dx, Y + sag * dy, Z, wrap=True), M.slot('Kerchief'))
    face_out(ob, centre)
    i = int(np.argmin(np.abs(th)))
    k = np.array([X[3, i], Y[3, i] - 0.010, Z[3, i] - 0.004])
    knot = ellipsoid('KerchiefKnot', k, (0.019, 0.011, 0.014), (0.0, -1.0, 0.0), M.slot('Kerchief'))
    tails = []
    for sx in (-1.0, 1.0):
        line = np.array([k + np.array([sx * (0.006 + 0.020 * t), -0.008 - 0.006 * t, -0.006 - 0.040 * t])
                         for t in np.linspace(0, 1, 7)])
        wv = (0.026 - 0.006 * np.linspace(0, 1, 7))[:, None]
        side = np.array([1.0, 0.0, 0.45 * sx])
        side /= np.linalg.norm(side)
        P = np.stack([line - side * wv * 0.5, line + side * wv * 0.5], 1)
        tails.append(gm.mesh(f'KerchiefTail_{"l" if sx > 0 else "r"}', gm.grid(P[..., 0], P[..., 1], P[..., 2]),
                             M.slot('Kerchief')))
    M.log(f'neckerchief: from {Z.max():.3f} down to its point at {Z.min():.3f} m')
    return [ob, knot] + tails


def ellipsoid(name, centre, radii, z_axis, mat, segments=14, rings=9):
    bm = bmesh.new()
    lm.uv_sphere(bm, segments, rings, 1.0)
    me = bpy.data.meshes.new(name)
    bm.to_mesh(me)
    bm.free()
    me.shade_smooth()
    ob = bpy.data.objects.new(name, me)
    bpy.context.scene.collection.objects.link(ob)
    rot = Vector(tuple(z_axis)).to_track_quat('Z', 'Y').to_matrix().to_4x4()
    m = Matrix.Translation(Vector(tuple(centre))) @ rot @ Matrix.Diagonal((*radii, 1.0))
    ob.data.transform(m)
    ob.data.materials.append(mat)
    return ob
