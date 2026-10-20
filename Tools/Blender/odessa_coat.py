"""Odessa's wine tailcoat (Art/Models/Characters/Odessa.py calls these): fitted to the waist, open in front with darker
silk lapels and a rolled collar, cut away at the waist in a curve back to long tails split up the back to the waist
seam (two gold buttons at its top), fitted sleeves with darker turned-back cuffs over the shirt's white cuffs.

Its body and sleeves are one shell of the body, 'Coat' (torso and arms pushed out over the waistcoat and smoothed as
cloth, the bust's and the back's hollows spanned), cut clean by planes (bmesh bisect) at the neckline, the waist, the
front opening and the cuffs; the set-in sleeves' seams drawn round the armholes ('Seams'). (A coat draped round the
torso by columns, with sleeves of their own, could not make a shoulder: its top either squared off over the arms or
left the sleeves' heads open, 2026-10-19.) The tails ('CoatTails') hang below from under the coat's waist seam, draped
by columns round the hips (wrap_radii), plumb off the seat, the cutaway's curve their front edge."""
import math

import bpy  # first: bmesh and mathutils come with it
import bmesh
import numpy as np
from mathutils import Vector

import looter_garments as gm
import odessa_clothes as cl
import odessa_kit as kit
from odessa_kit import FWD, log, slot

COAT_OFF = 0.0185       # the coat's body off the skin (m): over the waistcoat (8.5 mm) with 1 cm of room
TAIL_EASE = 0.010       # the tails' room over the hips and the seat
TAIL_HEM = 0.50         # the tails' hem (m): to the back of the knee
TAIL_FLARE = 0.02       # how much the tails stand off further at the hem (m)
CUT = 0.13              # the cutaway: the front edge curves from the waist back to the tails over this height (m)
TAIL_TOP_EDGE = 1.95    # the tails' outer edge where the cutaway reaches them (radians from the front: past the side)
TAIL_HEM_EDGE = 2.36    # ... and at their hem (they narrow as they fall)
SPLIT = 0.008           # the tails part this much at the hem (m between them), closed at the waist seam
TUCK = 0.012            # the tails' top tucked this far up under the coat's waist seam (m)
LAPEL_END = 0.16        # the lapels end this far over the waist (m); the fronts below them hang straight
ARMHOLE = 0.012         # the armholes: planes square to each upper arm this far in from the shoulder joint (m)
ARM_REACH = 0.12        # a sleeve's faces lie within this of the arm's bones (m)


def front_gap(fig, z):
    """Half the coat's opening across (m) at height z, above the waist: the V under the rolled collar widening down the
    lapels' fold, then the fronts hanging nearly straight to the waist."""
    nz = fig.neck[2]
    lap = fig.waist + LAPEL_END
    return float(np.interp(z, [fig.waist, lap, nz - 0.06, nz + 0.02], [0.080, 0.085, 0.056, 0.042]))


def front_angle(fig, z, r_front):
    """The tails' front edge at height z: radians round from the front. Down from the waist the cutaway curves back past
    the hips to the tails' edge, which narrows to the hem."""
    w = fig.waist
    a_w = math.asin(min(0.98, front_gap(fig, w) / max(r_front, 0.05)))
    if z >= w:
        return a_w
    if z >= w - CUT:
        t = float(gm.smoothstep(0.0, 1.0, (w - z) / CUT))
        return a_w + (TAIL_TOP_EDGE - a_w) * t ** 0.8
    t = (w - CUT - z) / max(w - CUT - TAIL_HEM, 1e-6)
    return TAIL_TOP_EDGE + (TAIL_HEM_EDGE - TAIL_TOP_EDGE) * t


def armhole(fig, s):
    j, el = fig.sk[f'upperarm_{s}'][0], fig.sk[f'upperarm_{s}'][1]
    d = gm.unit(el - j)
    return j - d * ARMHOLE, d


def _bisect(bm, faces, co, no):
    if not faces:
        return
    geom = list({v for f in faces for v in f.verts}) + list({e for f in faces for e in f.edges}) + list(faces)
    bmesh.ops.bisect_plane(bm, geom=geom, plane_co=Vector(tuple(co)), plane_no=Vector(tuple(no)))


# ------------------------------------------------------------------------------------------------- body and sleeves

def build_coat(fig, vest, body):
    """The coat's body and sleeves, one shell (see the module's docstring)."""
    wv = fig.wv
    nz = fig.neck[2]
    arm = np.clip((fig.w_arm + fig.w_hand - 0.15) / 0.4, 0.0, 1.0)
    region = (fig.w_head < 0.25) & ((wv[:, 2] > fig.waist - 0.05) | (fig.w_arm + fig.w_hand > 0.3))
    for s, sx in (('l', 1.0), ('r', -1.0)):
        e, w, axis, a, b = cl.forearm_frame(fig, s)
        region &= ~((wv[:, 0] * sx > 0) & (((wv - w) @ axis) > -cl.SLEEVE_END + 0.03))
    along = np.zeros(len(wv))
    for s, sx in (('l', 1.0), ('r', -1.0)):
        a, w = fig.sk[f'upperarm_{s}'][0], fig.sk[f'lowerarm_{s}'][1]
        t = np.clip(((wv - a) @ gm.unit(w - a)) / np.linalg.norm(w - a), 0, 1)
        along = np.where(wv[:, 0] * sx > 0, t, along)
    sleeve_off = 0.012 + 0.003 * gm.smoothstep(0.0, 0.25, along) + 0.008 * along
    off = COAT_OFF * (1.0 - arm) + sleeve_off * arm
    piece, used = gm.shell(wv, fig.faces, fig.fuv, fig.face_mask(region), off, iters=18)
    a_p = arm[used]
    v = piece.verts
    # the body smoothed hard as cloth (the arms less: Taubin keeps their volume), then over its hollows
    v = gm.laplacian(v, piece.faces, 40, weight=1.0 - 0.7 * a_p)
    # the bust's points shrunk away (plain smoothing there; the waistcoat's own shape, cleared over below, holds it out):
    # Taubin's keeps them, and the coat read as moulded on her
    bust = (v[:, 1] < fig.centre[1]) & (v[:, 2] > fig.waist + 0.04) & (v[:, 2] < fig.shoulder_z - 0.04) & (a_p < 0.3)
    v = gm.laplacian(v, piece.faces, 25, lam=0.5, mu=0.0, weight=bust * 1.0)
    groups = np.where(a_p < 0.5, 0, -1)
    v = cl.bridge(v, piece.faces, groups, k=0.9, hang=0.9, front_y=fig.centre[1])
    # sleeve folds: rings bunched above the cuff, crescents in the crook of the elbow
    nrm = gm.vertex_normals(v, piece.faces)
    for s, sx in (('l', 1.0), ('r', -1.0)):
        el, w = fig.sk[f'lowerarm_{s}'][0], fig.sk[f'lowerarm_{s}'][1]
        side = (v[:, 0] * sx > 0) & (a_p > 0.5)
        d = gm.unit(w - el)
        t = (v - el) @ d
        ring = np.cos(t / 0.05 * 2 * math.pi + 0.8 * np.sin(np.arctan2(v[:, 2] - el[2], v[:, 1] - el[1]) * 2)) * 0.5 + 0.5
        bunch = gm.smoothstep(0.08, 0.16, t) * 0.005 * ring ** 2
        crook = np.exp(-(t / 0.06) ** 2) * 0.005 * np.sin(t * 90)
        v[side] += nrm[side] * (bunch[side] + crook[side])[:, None]
    # each sleeve's end gathered round the shirt's cuff (its radius + 4.5 mm)
    for s, sx in (('l', 1.0), ('r', -1.0)):
        e, w, axis, a, b = cl.forearm_frame(fig, s)
        side = v[:, 0] * sx > 0
        up = -((v - w) @ axis)
        k = gm.smoothstep(cl.SLEEVE_END + 0.07, cl.SLEEVE_END + 0.005, up) * side * (a_p > 0.5)
        q = v - w
        along_ax = np.outer(q @ axis, axis)
        perp = q - along_ax
        ra, rb = fig.cuff_tops[s] + 0.0035 + 0.0045
        ang = np.arctan2(perp @ b, perp @ a)
        want = 1.0 / np.sqrt((np.cos(ang) / ra) ** 2 + (np.sin(ang) / rb) ** 2)
        have = np.linalg.norm(perp, axis=1)
        new = have + (want - have) * k
        v = np.where(side[:, None], w + along_ax + perp * (new / np.maximum(have, 1e-9))[:, None], v)
    # over the waistcoat (the body at least 6 mm out of it), the sleeves off the arms
    vt = gm.bvh_objects([vest])
    body_t = gm.bvh_objects([body])
    tor = a_p < 0.5
    v[tor] = gm.clear(v[tor], vt, 0.006, 0.03)
    v[~tor] = gm.clear(v[~tor], body_t, 0.010, 0.1)
    piece.verts = v
    ob = gm.mesh('Coat', piece, slot('Coat'))
    # MakeHuman's faces are 2-3 cm across the torso: once subdivided the cloth is smooth and the cuts follow their
    # planes closely
    m = ob.modifiers.new('fine', 'SUBSURF')
    m.levels = 1
    m.uv_smooth = 'PRESERVE_BOUNDARIES'
    gm.apply_modifiers(ob, keep=())
    coat = cut_coat(fig, ob)
    kit.set_seams(coat, seam_distance(fig, coat))
    kit.THICK['Coat'] = 0.003
    kit.EDGE_KEEP['Coat'] = 0.010
    log(f'coat: body and sleeves {len(coat.data.polygons)} faces; open {front_gap(fig, fig.waist + 0.1) * 200:.0f} cm '
        f'across over the waist')
    return coat


def cut_coat(fig, ob):
    """The coat's shell cut clean by planes: the neckline, the waist (the body; not the arms, which hang past it), the
    front opening (along front_gap, two planes a side), the sleeves' ends; then parted at the armholes."""
    nz, wz = fig.neck[2], fig.waist
    c_y = fig.centre[1]
    holes = {sx: armhole(fig, s) for s, sx in (('l', 1.0), ('r', -1.0))}
    ends = {}
    for s, sx in (('l', 1.0), ('r', -1.0)):
        e, w, axis, a, b = cl.forearm_frame(fig, s)
        ends[sx] = (w - axis * cl.SLEEVE_END, -axis)

    lines = {}
    for s, sx in (('l', 1.0), ('r', -1.0)):
        lines[sx] = (fig.sk[f'upperarm_{s}'][0], fig.sk[f'lowerarm_{s}'][0], fig.sk[f'lowerarm_{s}'][1])

    def near_arm(c, reach=ARM_REACH):
        sx = 1.0 if c[0] > 0 else -1.0
        j, el, w = lines[sx]
        best = 1.0
        for a, b in ((j, el), (el, w + (w - el) * 0.3)):
            t = float(np.clip(((c - a) @ (b - a)) / ((b - a) @ (b - a)), 0.0, 1.0))
            best = min(best, float(np.linalg.norm(c - (a + (b - a) * t))))
        return best < reach

    def on_arm(c):
        # past the armhole's plane and near the arm (the plane runs on through the torso's side far below)
        c = np.asarray(c)
        sx = 1.0 if c[0] > 0 else -1.0
        p, d = holes[sx]
        return (c - p) @ d > 0.0 and near_arm(c)

    bm = bmesh.new()
    bm.from_mesh(ob.data)
    cen = lambda f: np.array(f.calc_center_median())
    # the neckline and the waist (the body's faces near them)
    _bisect(bm, [f for f in bm.faces if cen(f)[2] > nz - 0.07 and abs(cen(f)[0]) < 0.16 and not on_arm(cen(f))],
            (0.0, 0.0, nz - 0.004), (0.0, 0.0, 1.0))
    _bisect(bm, [f for f in bm.faces if abs(cen(f)[2] - wz) < 0.06 and not on_arm(cen(f))], (0.0, 0.0, wz), (0.0, 0.0, 1.0))
    # the front opening: the fold line from the neck to the lapels' end and from there to the waist, each a plane
    lap = wz + LAPEL_END
    knots = [(z, front_gap(fig, z)) for z in (nz + 0.02, nz - 0.06, lap, wz - 0.02)]     # (front_gap's own knots)
    for side in (1.0, -1.0):
        for (z0, g0), (z1, g1) in zip(knots[:-1], knots[1:]):
            d = Vector((side * (g1 - g0), 0.0, z1 - z0)).normalized()
            n = d.cross(Vector((0.0, 1.0, 0.0)))
            mine = [f for f in bm.faces if cen(f)[1] < c_y and z1 - 0.012 < cen(f)[2] < z0 + 0.012 and
                    cen(f)[0] * side > 0 and not on_arm(cen(f))]
            _bisect(bm, mine, (side * g0, 0.0, z0), n)
    # the sleeves' ends
    for sx in (1.0, -1.0):
        q, n = ends[sx]
        _bisect(bm, [f for f in bm.faces if cen(f)[0] * sx > 0 and near_arm(cen(f), ARM_REACH + 0.03)], q, n)

    def gone(c):
        if on_arm(c):
            sx = 1.0 if c[0] > 0 else -1.0
            q, n = ends[sx]
            return (c - q) @ n < 0.0
        if c[2] > nz - 0.004 or c[2] < wz:
            return True
        return c[1] < c_y and abs(c[0]) < front_gap(fig, c[2]) - 1e-5
    bmesh.ops.delete(bm, geom=[f for f in bm.faces if gone(cen(f))], context='FACES')
    bmesh.ops.delete(bm, geom=[v for v in bm.verts if not v.link_faces], context='VERTS')
    bm.to_mesh(ob.data)
    bm.free()
    ob.data.update()
    return ob


def arm_distance(fig, P, s):
    """Each point's distance (m) from side s's arm bones (shoulder to elbow, elbow on past the wrist)."""
    j, el, w = fig.sk[f'upperarm_{s}'][0], fig.sk[f'lowerarm_{s}'][0], fig.sk[f'lowerarm_{s}'][1]
    best = np.full(len(P), 1.0)
    for a, b in ((j, el), (el, w + (w - el) * 0.3)):
        t = np.clip(((P - a) @ (b - a)) / ((b - a) @ (b - a)), 0.0, 1.0)
        best = np.minimum(best, np.linalg.norm(P - (a + np.outer(t, b - a)), axis=1))
    return best


def seam_distance(fig, ob):
    """Metres from each vertex to the coat's sewn seams: down each side, up the middle of the back, round each armhole
    (where its plane meets the cloth near the arm: the set-in sleeve's seam)."""
    co = gm._object_coords(ob)
    ang = kit.round_angle(co, fig.centre)
    rad = np.linalg.norm(co[:, :2] - fig.centre, axis=1)
    wrap = lambda a_: np.abs((a_ + math.pi) % (2 * math.pi) - math.pi)
    back = wrap(ang - math.pi) * rad
    sides = np.minimum(wrap(ang - 1.75), wrap(ang + 1.75)) * rad
    out = np.where(co[:, 2] > fig.waist - 0.03, np.minimum(back, sides), 1.0)
    for s, sx in (('l', 1.0), ('r', -1.0)):
        p, d = armhole(fig, s)
        mine = (co[:, 0] * sx > 0) & (arm_distance(fig, co, s) < 0.10)
        out = np.where(mine, np.minimum(out, np.abs((co - p) @ d)), out)
    return out


# ------------------------------------------------------------------------------------------------------- the tails

def wrap_radii(pts, centre, fine, zs, hb=math.radians(6.0), zb=0.012):
    """R (rows, angles): the outermost of pts round centre in each column (fine, radians from the front, + her left)
    within hb, within zb of each height; columns that saw nothing filled from their neighbours."""
    f, l = gm.frame2d(FWD)
    rel = pts[:, :2] - centre
    ang = np.arctan2(rel @ l, rel @ f)
    rad = np.linalg.norm(rel, axis=1)
    R = np.zeros((len(zs), len(fine)))
    for j, z in enumerate(zs):
        sel = np.abs(pts[:, 2] - z) < zb
        if not sel.any():
            continue
        a, r = ang[sel], rad[sel]
        d = np.abs((a[None, :] - fine[:, None] + math.pi) % (2 * math.pi) - math.pi) < hb
        R[j] = np.where(d, r[None, :], 0.0).max(1)
    for j in range(len(zs)):
        good = R[j] > 0
        if good.any() and not good.all():
            R[j] = np.interp(fine, fine[good], R[j, good], period=2 * math.pi)
    return R


def waist_ring(fig, coat, fine):
    """The coat's body at its waist seam: its radius round fig.centre at each angle (fine), across the front opening
    from either side."""
    co = gm._object_coords(coat)
    edge = gm.boundary([list(p.vertices) for p in coat.data.polygons], len(co))
    body = (arm_distance(fig, co, 'l') > ARM_REACH) & (arm_distance(fig, co, 'r') > ARM_REACH)
    low = co[edge & body & (np.abs(co[:, 2] - fig.waist) < 0.004)]     # (not the sleeves' ends, hanging past it)
    ang = kit.round_angle(low, fig.centre)
    rad = np.linalg.norm(low[:, :2] - fig.centre, axis=1)
    order = np.argsort(ang)
    return np.interp(fine, ang[order], rad[order], period=2 * math.pi)


def build_tails(fig, under_pts, coat):
    """The tails and the cutaway, one cloth: columns from her left front edge round the back to her right one, from
    TUCK up under the coat's waist seam down to the hem; at the seam the coat's own waist (2 mm inside it), below it
    hanging plumb off the seat over the belt and the thighs, flaring a little to the hem; the front edge the cutaway's
    curve back to the tails' edge (front_angle); split up the back to the waist seam."""
    rng = np.random.default_rng(29)
    top = fig.waist + TUCK
    n_z, n_t = 52, 112
    fine = np.linspace(-math.pi, math.pi, 720, endpoint=False)
    zs = np.linspace(top, TAIL_HEM, n_z)
    ring = waist_ring(fig, coat, fine) - 0.002
    R = wrap_radii(under_pts, fig.centre, fine, zs) + TAIL_EASE
    for _ in range(16):
        R = 0.25 * np.roll(R, 1, 1) + 0.5 * R + 0.25 * np.roll(R, -1, 1)
    w_row = int(np.argmin(np.abs(zs - fig.waist)))
    R[:w_row + 1] = ring[None, :]
    R = np.maximum.accumulate(R, axis=0)                  # hanging: never in under what's above it
    for _ in range(3):
        R[1:-1] = 0.25 * R[:-2] + 0.5 * R[1:-1] + 0.25 * R[2:]
    v = np.clip((fig.waist - zs) / (fig.waist - TAIL_HEM), 0.0, 1.0)[:, None]
    R = R + TAIL_FLARE * v ** 1.5
    half = n_t // 2
    ph = rng.random(8) * 6.28
    rows = []
    for j, z in enumerate(zs):
        r_front = float(np.interp(0.0, fine, R[j], period=2 * math.pi))
        a = front_angle(fig, z, r_front)
        g = SPLIT * 0.5 * float(np.clip((fig.waist - 0.02 - z) / (fig.waist - 0.02 - TAIL_HEM), 0.0, 1.0)) ** 1.3
        r_back = float(np.interp(math.pi, fine, R[j], period=2 * math.pi))
        back = math.pi - g / max(r_back, 0.05)
        th = np.concatenate([np.linspace(a, back, half), np.linspace(-back, -a, n_t - half)])
        r = np.interp(th, fine, R[j], period=2 * math.pi)
        vv = max((fig.waist - z) / (fig.waist - TAIL_HEM), 0.0)
        fold = 0.006 * vv ** 1.2 * (0.55 * np.sin(th * 9.0 + ph[0] + 0.7 * np.sin(vv * 2.0 + ph[1]))
                                    + 0.45 * np.sin(th * 15.3 + ph[2] + vv * 1.5))
        rows.append((th, r + fold, z))
    TH = np.array([r[0] for r in rows])
    RR = np.array([r[1] for r in rows])
    ZZ = np.array([[r[2]] * n_t for r in rows])
    hem_t = np.abs(TH[-1])
    ZZ[-1] += -0.012 * np.sin(np.clip((hem_t - TAIL_HEM_EDGE) / (math.pi - TAIL_HEM_EDGE), 0, 1) * math.pi)
    corner = np.clip(1.0 - (np.abs(TH) - TAIL_HEM_EDGE) / 0.12, 0.0, 1.0) * gm.smoothstep(TAIL_HEM + 0.05, TAIL_HEM, ZZ)
    ZZ += corner * 0.035 * gm.smoothstep(TAIL_HEM + 0.05, TAIL_HEM, ZZ)
    dx, dy = gm.radial(TH, FWD)
    X = fig.centre[0] + RR * dx
    Y = fig.centre[1] + RR * dy
    ob = gm.mesh('CoatTails', gm.grid(X, Y, ZZ), slot('Coat'))
    # weld the back's two columns above the split's top; below it the strip between them goes
    bm = bmesh.new()
    bm.from_mesh(ob.data)
    bm.verts.ensure_lookup_table()
    split_top = fig.waist - 0.02
    pairs = [(bm.verts[j * n_t + half - 1], bm.verts[j * n_t + half]) for j in range(n_z) if zs[j] >= split_top]
    strip = []
    for f in bm.faces:
        idx = [vv_.index for vv_ in f.verts]
        if {i % n_t for i in idx} == {half - 1, half} and min(zs[i // n_t] for i in idx) < split_top - 1e-6:
            strip.append(f)
    bmesh.ops.delete(bm, geom=strip, context='FACES')
    for a_, b_ in pairs:
        mid = (a_.co + b_.co) * 0.5
        a_.co = b_.co = mid
    bmesh.ops.remove_doubles(bm, verts=[vv_ for p in pairs for vv_ in p], dist=1e-5)
    bm.to_mesh(ob.data)
    bm.free()
    kit.THICK['CoatTails'] = 0.004
    kit.EDGE_KEEP['CoatTails'] = 0.012
    co = gm._object_coords(ob)
    ang = kit.round_angle(co, fig.centre)
    rad = np.linalg.norm(co[:, :2] - fig.centre, axis=1)
    wrap = lambda a_: np.abs((a_ + math.pi) % (2 * math.pi) - math.pi)
    back = np.where(co[:, 2] > split_top, wrap(ang - math.pi) * rad, 1.0)
    kit.set_seams(ob, np.minimum(back, np.abs(co[:, 2] - fig.waist)))
    log(f'tails: {len(ob.data.polygons)} faces, the cutaway from the waist to {math.degrees(TAIL_TOP_EDGE):.0f} degrees, '
        f'{math.degrees(TAIL_HEM_EDGE):.0f} at the hem, {TAIL_HEM:.2f} m')
    return ob


def build_coat_buttons(fig, tails):
    """Two gold buttons at the top of the tails' split, just under the waist seam either side of it."""
    tree = gm.bvh_objects([tails])
    out = []
    for side in (1.0, -1.0):
        a = side * (math.pi - 0.20)
        try:
            p = kit.on_surface(tree, fig.centre, a, fig.waist - 0.012)
        except ValueError:
            continue
        n = gm.unit(np.array([p[0] - fig.centre[0], p[1] - fig.centre[1], 0.0]))
        out.append(kit.ellipsoid('CoatButton', p + n * 0.006, (0.0068, 0.0068, 0.0032), n, 12, 8, slot('Gold')))
    return out


# --------------------------------------------------------------------------------------- lapels, collar, cuffs

def build_lapels(fig, coat):
    """The darker silk lapels folded back over the coat's fronts from under the rolled collar to LAPEL_END over the
    waist, widest across the chest, notched below the collar: strips laid on the coat from the front (rays along +y at
    the fold line and out from it), the fold's edge on the coat's own."""
    tree = gm.bvh_objects([coat])
    nz = fig.neck[2]
    lo, hi = fig.waist + LAPEL_END, nz - 0.022
    built = []
    for side in (1, -1):
        rows = []
        for z in np.linspace(lo, hi, 16):
            g = front_gap(fig, z)
            t = (z - lo) / (hi - lo)
            w = 0.010 + 0.040 * float(gm.smoothstep(0.0, 0.55, t)) * (1.0 - 0.45 * float(gm.smoothstep(0.7, 1.0, t)))
            w -= 0.020 * math.exp(-((t - 0.80) / 0.045) ** 2)          # the notch
            row = []
            for k, (u, lift) in enumerate(((0.0006, 0.0042), (0.5 * w, 0.006), (w, 0.0040))):
                hit = (None,)
                for extra in (0.0, 0.001, 0.002, 0.004, 0.007):
                    x = side * (g + u + extra)
                    hit = tree.ray_cast(Vector((x, -0.6, z)), Vector((0.0, 1.0, 0.0)), 1.0)
                    # (the front panel only: by the neck a ray past its edge went on to the back's, and the lapel's
                    # top reached round behind the neck)
                    if hit[0] is not None and hit[0].y < fig.centre[1] - 0.02:
                        break
                    hit = (None,)
                if hit[0] is None and rows:
                    p = rows[-1][k]                         # the row below's point, carried up
                    row.append(np.array([p[0], p[1], z]))
                    continue
                p = np.array(hit[0]) if hit[0] is not None else np.array([side * (g + u), fig.centre[1] - 0.12, z])
                n = np.array(hit[1]) if hit[0] is not None else np.array([0.0, -1.0, 0.0])
                if n[1] > 0:
                    n = -n
                row.append(p + n * lift)
            rows.append(row)
        R = np.array(rows).transpose(1, 0, 2)
        if side < 0:
            R = R[::-1]
        ob = gm.mesh(f'Lapel_{"l" if side > 0 else "r"}', gm.grid(R[..., 0], R[..., 1], R[..., 2]), slot('CoatTrim'))
        faces_toward(ob, (0.0, -1.0, 0.0))
        kit.THICK[ob.name] = 0.003
        kit.EDGE_KEEP['Lapel'] = 0.008
        built.append(ob)
    return built


def faces_toward(ob, d):
    """Turns a sheet's faces round if most of them face away from direction d."""
    me = ob.data
    score = sum(float(np.array(p.normal) @ np.asarray(d)) * p.area for p in me.polygons)
    if score < 0.0:
        bm = bmesh.new()
        bm.from_mesh(me)
        bmesh.ops.reverse_faces(bm, faces=bm.faces[:])
        bm.to_mesh(me)
        bm.free()
        me.update()


def build_coat_collar(fig, coat):
    """The coat's rolled collar standing behind the neck outside the shirt's, from the lapels' tops round the back,
    rising to the back and rolling outward at its top; darker silk. Its foot sits 4 mm outside the coat's neckline edge,
    from 3 mm under it (a foot running 2 cm straight down inside the coat left the coat's cut edge standing out: shards
    round the back; one wrapped round the coat's outermost 6 mm down spread over the shoulders like a cape's)."""
    nz = fig.neck[2]
    open_a = math.asin(min(0.95, front_gap(fig, nz - 0.03) / 0.085)) + 0.05
    th = np.linspace(open_a, 2 * math.pi - open_a, 90)
    rows = 8
    nc = fig.neck[:2]
    z_top = nz - 0.004                          # cut_coat's neckline
    z_lo = z_top - 0.003
    # the neckline edge's radius round the neck at each angle
    co = gm._object_coords(coat)
    edge = gm.boundary([list(p.vertices) for p in coat.data.polygons], len(co))
    rim = co[edge & (np.abs(co[:, 2] - z_top) < 0.003) & (np.linalg.norm(co[:, :2] - nc, axis=1) < 0.16)]
    ang = np.arctan2(rim[:, 0] - nc[0], -(rim[:, 1] - nc[1])) % (2 * math.pi)
    order = np.argsort(ang)
    R = np.interp(th, ang[order], np.linalg.norm(rim[:, :2] - nc, axis=1)[order], period=2 * math.pi)
    for _ in range(3):
        R[1:-1] = 0.25 * R[:-2] + 0.5 * R[1:-1] + 0.25 * R[2:]
    cv = []
    for j in range(rows):
        t = j / (rows - 1)
        for i, a in enumerate(th):
            back = 0.5 - 0.5 * math.cos(a)
            hgt = 0.007 + 0.040 * back      # (its top where it was: the neck's base in front, 4 cm up behind)
            z = z_lo + hgt * t
            r_in = fig.collar_r(a, min(z, nz + 0.03)) + 0.009
            r = r_in + 0.004 + 0.018 * t ** 2.2 * (0.6 + 0.4 * back)
            # 4 mm out past the coat's edge up to it, then easing back to the stand's own line over 1.6 cm
            rc = float(R[i]) + 0.004
            s = float(np.clip(1.0 - (z - z_top) / 0.016, 0.0, 1.0))
            r = max(r, r + (rc - r) * s * s * (3.0 - 2.0 * s))
            cv.append([nc[0] + math.sin(a) * r, nc[1] - math.cos(a) * r, z])
    cv = np.array(cv).reshape(rows, -1, 3)[::-1]
    ob = gm.mesh('CoatCollar', gm.grid(cv[..., 0], cv[..., 1], cv[..., 2]), slot('CoatTrim'))
    kit.faces_out(ob, fig.neck, (0.0, 0.0, 1.0))
    kit.THICK['CoatCollar'] = 0.004
    kit.EDGE_KEEP['CoatCollar'] = 0.006
    return ob


def build_coat_cuffs(fig, sleeves):
    """The sleeves' darker turned-back cuffs: bands round each sleeve's end (COAT_CUFF), 2.5 mm proud of it, folded
    over its edge."""
    st = gm.bvh_objects([sleeves])
    parts = []
    for s in 'lr':
        e, w, axis, a, b = cl.forearm_frame(fig, s)
        rows = []
        for u in np.linspace(cl.COAT_CUFF[0] + 0.004, cl.COAT_CUFF[1], 5):
            ring = []
            centre = w - axis * u
            for k in range(28):
                ang = 2 * math.pi * k / 28
                d = a * math.cos(ang) + b * math.sin(ang)
                hit = st.ray_cast(Vector(tuple(centre + d * 0.15)), Vector(tuple(-d)), 0.2)
                p = np.array(hit[0]) if hit[0] is not None else centre + d * 0.045
                ring.append(p + d * 0.0025)
            rows.append(ring + ring[:1])
        rows.insert(0, [p + axis * 0.007 for p in rows[0]])
        R = np.array(rows)
        ob = gm.mesh(f'CoatCuff_{s}', gm.grid(R[..., 0], R[..., 1], R[..., 2]), slot('CoatTrim'))
        kit.faces_out(ob, w, axis)                          # (its rows run up the arm: built facing in)
        kit.THICK[ob.name] = 0.0025
        parts.append(ob)
    return parts


def edge_angles(co, edges, centre, z, side):
    """The angles (radians from the front, on side +1 left / -1 right) where a garment's edges cross height z: its
    crossings' nearest to the front and nearest to the back."""
    a, b = co[edges[:, 0]], co[edges[:, 1]]
    da, db = a[:, 2] - z, b[:, 2] - z
    cut = (da * db <= 0.0) & (da != db)
    p = a[cut] + (b[cut] - a[cut]) * (da[cut] / (da[cut] - db[cut]))[:, None]
    ang = kit.round_angle(p, centre) * side
    ang = ang[ang > 0]
    return float(ang.min()), float(ang.max())
