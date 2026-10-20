"""Hollis Crane's clothes for his game model (Art/Models/Characters/Crane.py), adapted from Ellis.py's builders: dark
wool trousers tucked into tall worn boots, the old linen shirt torn over the ember (and the ember glowing in the tear),
a plain belt, the long slate coat (draped, its skirt settled by Blender's cloth, its hem torn into pointed tatters), its
sleeves with turned cuffs over the bare wrists, the ragged shoulder cape draped over the arms as the game holds them,
and its low collar. Measures from the figure (crane_common.Figure)."""
import math
import time

import bpy
import bmesh
import numpy as np
from mathutils import Matrix, Vector

import crane_common as cc
import looter_garments as gm
import looter_human as hu
import looter_bake as bk
from crane_common import BOOT_TOP, FWD, HEM, OPT, SIM, log, slot


# ------------------------------------------------------------------------------------------------------ trousers

def build_trousers(fig):
    """Dark wool trousers: the hips and legs pushed out and smoothed, from the waist to inside the boots, bloused a
    little over the boot tops, creased behind the knees and over the boots."""
    z = fig.wv[:, 2]
    keep = (z < fig.waist + 0.02) & (z > BOOT_TOP - 0.07) & (fig.w_arm < 0.2) & (fig.w_hand < 0.1)
    loose = gm.smoothstep(fig.hip_z - 0.06, fig.hip_z - 0.48, z)
    blouse = gm.smoothstep(BOOT_TOP + 0.01, BOOT_TOP + 0.05, z) * gm.smoothstep(BOOT_TOP + 0.16, BOOT_TOP + 0.06, z)
    inside = gm.smoothstep(BOOT_TOP + 0.01, BOOT_TOP - 0.02, z)
    off = (0.009 + 0.010 * loose + 0.007 * blouse) * (1.0 - inside) + 0.010 * inside
    piece, used = gm.shell(fig.wv, fig.faces, fig.fuv, fig.face_mask(keep), off, iters=14)
    v = piece.verts
    nrm = gm.vertex_normals(v, piece.faces)
    for s_ in 'lr':
        knee = fig.sk[f'calf_{s_}'][0]
        side = (v[:, 0] * knee[0]) > 0
        dz = v[:, 2] - knee[2]
        behind = gm.smoothstep(knee[1] - 0.02, knee[1] + 0.05, v[:, 1])
        ring = np.sin(dz * 160.0 + v[:, 0] * 40.0) * np.exp(-(dz / 0.06) ** 2) * (0.35 + 0.65 * behind)
        tuck = np.sin(v[:, 2] * 210.0 + np.arctan2(v[:, 1] - knee[1], v[:, 0] - knee[0]) * 3.0) * \
            gm.smoothstep(BOOT_TOP + 0.13, BOOT_TOP + 0.03, v[:, 2]) * gm.smoothstep(BOOT_TOP - 0.01, BOOT_TOP + 0.03, v[:, 2])
        v[side] += nrm[side] * (0.004 * ring[side] + 0.004 * tuck[side])[:, None]
    piece.verts = v
    return gm.mesh('Trousers', piece, slot('Trousers'))


# ------------------------------------------------------------------------------------------------------ boots

def insole(s):
    """The boot's insole along the foot (s, m ahead of the ankle): on the heel 4 cm up, sloping down over the arch to
    1.8 cm under the ball and the toes, the toe tip sprung up (Ellis.py's, on his 3 cm heels)."""
    return np.interp(s, [-0.2, 0.03, 0.11, 0.19, 0.28], [0.040, 0.040, 0.018, 0.018, 0.024])


BOOT_EASE = 0.013
BOOT_TAPER = 0.08
BOOT_WALL = 0.005
BOOT_VOXEL = 0.0019
BOOT_TRIS = 7000


def foot_frame(fig, s):
    ankle, ball = fig.sk[f'foot_{s}'][0], fig.sk[f'ball_{s}'][0]
    f = gm.unit(np.array([ball[0] - ankle[0], ball[1] - ankle[1], 0.0]))
    return ankle, f, np.array([-f[1], f[0], 0.0])


def boot_shaft(fig, s):
    """A tall boot's shaft (Ellis.py's stovepipe): one closed thick-walled tube round the leg's slices with room,
    narrowing at most BOOT_TAPER going down, its top dipping 3 cm front and back, a well inside the trousers tuck into."""
    sx = 1.0 if s == 'l' else -1.0
    ankle, f, lat = foot_frame(fig, s)
    ed = {(min(p[k], p[k - 1]), max(p[k], p[k - 1])) for p in fig.faces for k in range(len(p))}
    ed = np.array(sorted(ed))
    a, b = fig.wv[ed[:, 0]], fig.wv[ed[:, 1]]
    keep = (a[:, 0] * sx > 0.0) & (b[:, 0] * sx > 0.0)
    a, b = a[keep], b[keep]

    def slice_at(z):
        da, db = a[:, 2] - z, b[:, 2] - z
        cut = (da * db <= 0.0) & (da != db)
        return (a[cut] + (b[cut] - a[cut]) * (da[cut] / (da[cut] - db[cut]))[:, None])[:, :2]

    z_lo = ankle[2] + 0.03
    zs = np.arange(BOOT_TOP + 0.02, z_lo - 1e-6, -0.01)
    A = np.linspace(-math.pi, math.pi, 56, endpoint=False)
    D = np.stack(gm.radial(A, FWD), axis=1)
    hulls = [cc.hull_2d(slice_at(z)) for z in zs]
    cen = np.array([h.mean(axis=0) for h in hulls])
    for _ in range(6):
        cen[1:-1] = 0.25 * cen[:-2] + 0.5 * cen[1:-1] + 0.25 * cen[2:]
    fit = np.array([cc.hull_radii(h - c, D) for h, c in zip(hulls, cen)])
    rad = fit + BOOT_EASE
    rad = np.maximum(0.75 * rad + 0.25 * rad.mean(axis=1, keepdims=True), fit + 0.008)
    for i in range(1, len(zs)):
        rad[i] = np.maximum(rad[i], rad[i - 1] - BOOT_TAPER * (zs[i - 1] - zs[i]))
    up = zs[::-1]

    def ring(z, inset=0.0):
        zc = np.clip(z, up[0], up[-1])
        c = np.stack([np.interp(zc, up, cen[::-1, k]) for k in (0, 1)], axis=1)
        r = np.array([np.interp(zc[k], up, rad[::-1, k]) for k in range(len(A))]) - inset
        return np.c_[c + D * r[:, None], z]

    top = BOOT_TOP - 0.03 * np.cos(A) ** 2
    bottom = insole(((ring(np.full(len(A), z_lo)) - ankle) @ f)) + 0.001
    floor = BOOT_TOP - 0.15
    rows = [ring(floor + (top - floor) * t, BOOT_WALL) for t in np.linspace(0.0, 1.0, 14)]
    rows.append(ring(top + 0.0025, BOOT_WALL * 0.5))
    rows += [ring(top + (bottom - top) * t) for t in np.linspace(0.0, 1.0, 46)]
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
    """A boot's foot on its last (Ellis.py's): rounded-rectangle sections from the heel to a round toe box 2.5 cm past
    the toes, the toe a little square, sprung up."""
    ankle, f, lat = foot_frame(fig, s)
    sx = 1.0 if s == 'l' else -1.0
    used = fig.b.used
    feet = fig.wv[used][(fig.w_foot[used] > 0.3) & (fig.wv[used][:, 0] * sx > 0)]
    sf, lf, zf = (feet - ankle) @ f, (feet - ankle) @ lat, feet[:, 2]
    s0, s1 = sf.min() - 0.004, sf.max() + 0.025
    u = np.linspace(0.0, 1.0, 46)
    S = s0 + (s1 - s0) * (0.6 * u + 0.4 * (0.5 - 0.5 * np.cos(math.pi * u)))
    lo, hi, top = np.zeros(len(S)), np.zeros(len(S)), np.zeros(len(S))
    for i, q in enumerate(S):
        near = np.abs(sf - q) < 0.01
        if near.any():
            lo[i], hi[i], top[i] = lf[near].min() - 0.009, lf[near].max() + 0.009, zf[near].max() + 0.010
        else:
            lo[i], hi[i], top[i] = lo[i - 1], hi[i - 1], top[i - 1]
    sb = sf[zf < 0.06].max()
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


def boot_look(fig):
    """The boots' look measures (looter_looks.leather's boot=(toe_y, welt_z)): the toe cap stitched 5.5 cm behind the
    boot's tip, the welt 7 mm over the insole under the ball."""
    feet = fig.wv[fig.b.used][fig.w_foot[fig.b.used] > 0.3]
    tip = float(feet[:, 1].min()) - 0.025
    cc.BOOT_LOOK = (tip + 0.055, float(insole(0.15)) + 0.0075)
    return cc.BOOT_LOOK


def build_boots(fig):
    """Tall worn boots: a straight shaft and the foot on its last fused into one leather surface (a voxel remesh), the
    crease between them rounded, on a stacked sole with a welt and a low heel (Ellis.py's build_boots)."""
    boot_look(fig)
    shafts = [gm.mesh(f'_Shaft_{s}', boot_shaft(fig, s), slot('Boots')) for s in 'lr']
    lasts = [gm.mesh(f'_Last_{s}', boot_last(fig, s), slot('Boots')) for s in 'lr']
    shaft_tree, last_tree = gm.bvh_objects(shafts), gm.bvh_objects(lasts)
    bk.select(shafts + lasts, shafts[0])
    bpy.ops.object.join()
    ob = bpy.context.view_layer.objects.active
    ob.name = ob.data.name = 'Boots'
    m = ob.modifiers.new('fuse', 'REMESH')
    m.mode, m.voxel_size, m.adaptivity = 'VOXEL', BOOT_VOXEL, 0.0
    gm.apply_modifiers(ob, keep=())
    co = gm._object_coords(ob)
    gap = np.zeros(len(co))
    for i, p in enumerate(co):
        if p[2] < 0.27:
            v = Vector(tuple(p))
            gap[i] = max(shaft_tree.find_nearest(v)[3], last_tree.find_nearest(v)[3])
        else:
            gap[i] = 1.0
    cc.fillet(ob, gm.smoothstep(0.022, 0.004, gap))
    m = ob.modifiers.new('reduce', 'DECIMATE')
    m.ratio = min(1.0, 2 * BOOT_TRIS / max(1, cc.tris_of(ob)))
    gm.apply_modifiers(ob, keep=())
    ob.data.shade_smooth()
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
        arch = gm.smoothstep(0.03, 0.045, s_ring) * gm.smoothstep(0.12, 0.09, s_ring)
        bottom = 0.012 * arch
        inward = np.where(heel, 0.004, 0.0)
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
        log(f'boot {s}: toe tip at y {float(foot_pts[:, 1].min()):.3f}, heel at y {float(foot_pts[:, 1].max()):.3f}')
    sole_ob = gm.mesh('Soles', soles[0] + soles[1], slot('Soles'))
    return [ob, sole_ob]


# ------------------------------------------------------------------------------------------- the shirt and the ember

def opening(fig, z):
    """Half the gap between the coat's fronts (m, across) at height z: a V from under the collar (the shirt and the
    ember's tear in it show), open wide below the waist so his legs stride free."""
    top = fig.neck[2]
    v = 0.045 + (top - z) * 0.16
    waist, low = fig.waist + 0.02, fig.waist - 0.20
    hips = 0.18
    return np.where(z > waist, v, np.where(z > low, hips + (z - low) / (waist - low) * (0.045 + (top - waist) * 0.16 - hips),
                                           hips))


EMBER_AT = (0.033, 0.145)     # the ember over his heart: m to his left of the neck's middle, m below the neck's base
TEAR = 0.029                  # the tear's size (the portrait's 0.026 at his height)


def ember_point(fig, body_tree):
    hit = body_tree.ray_cast(Vector((fig.neck[0] + EMBER_AT[0], -0.6, fig.neck[2] - EMBER_AT[1])), Vector((0, 1, 0)), 1.5)
    return hit[0], hit[1]


def build_shirt(fig, body_ob):
    """The old linen shirt: the torso pushed out and smoothed from under the trousers' waist to the neck, torn open over
    the ember (the portrait's tear(): a ragged hole, taller than wide), a collarless band round the neck open at the
    throat; and the ember behind the tear, a glowing patch lying on the chest (its Glow material in the game)."""
    z = fig.wv[:, 2]
    torso = (fig.w_arm < 0.25) & (fig.w_hand < 0.05) & (fig.w_head < 0.25)
    keep = torso & (z > fig.waist - 0.07) & (z < fig.neck[2] + 0.004)
    piece, _ = gm.shell(fig.wv, fig.faces, fig.fuv, fig.face_mask(keep), 0.0065, iters=10)
    ob = gm.mesh('Shirt', piece, slot('Shirt'))
    tree = gm.bvh_objects([body_ob])
    P, N = ember_point(fig, tree)
    rng = np.random.default_rng(41)
    CP_tear(ob, P, TEAR, rng)
    # the ember: a patch over the chest behind the tear, 2.5 mm off the skin (the shirt stands 6.5 mm off)
    s = TEAR * 1.6
    rows = []
    for j in range(9):
        row = []
        for i in range(9):
            x, zz = P.x + (i / 8 - 0.5) * 2 * s, P.z + (j / 8 - 0.5) * 3.2 * s
            hit = tree.ray_cast(Vector((x, -0.6, zz)), Vector((0, 1, 0)), 1.5)
            q = (hit[0] + hit[1] * 0.0025) if hit[0] is not None else Vector((x, P.y - 0.0025, zz))
            row.append(tuple(q))
        rows.append(row)
    R = np.array(rows)[::-1]
    ember = gm.mesh('Ember', gm.grid(R[..., 0], R[..., 1], R[..., 2]), slot('Ember'))
    fig.ember = np.array(P)
    band = shirt_band(fig, body_ob)
    log(f'shirt: torn over the ember at ({P.x * 100:.1f}, {P.y * 100:.1f}, {P.z * 100:.1f}) cm')
    return ob, band, ember


def CP_tear(ob, centre, size, rng):
    """The portrait's tear() (a ragged hole, taller than wide, round the centre), cut clean: the cloth
    there cut finer, the faces inside the outline gone, then the hole's edge laid onto the outline (deleted faces
    alone left a staircase of the shell's quads) with frayed threads jagging it, curled out a little."""
    bm = bmesh.new()
    bm.from_mesh(ob.data)
    for reach in (size * 2.6, size * 1.9):
        bmesh.ops.subdivide_edges(bm, edges=[e for e in bm.edges if (e.verts[0].co - centre).length < reach],
                                  cuts=1, use_grid_fill=True)
    ph = rng.random(4) * 6.283

    def rr(th):
        # (the portrait's five lobes read as a star cut in the shirt: a ragged slit, its edge in fewer, softer lobes)
        return size * (1 + 0.16 * math.sin(3 * th + ph[0]) + 0.09 * math.sin(7 * th + ph[1]) + 0.05 * math.sin(13 * th + ph[2]))

    def scaled(d):
        return math.hypot(d.x / 0.8, d.z / 1.3)
    dead = []
    for f in bm.faces:
        d = f.calc_center_median() - centre
        if d.y < 0.03 and scaled(d) < rr(math.atan2(d.z, d.x)):
            dead.append(f)
    bmesh.ops.delete(bm, geom=dead, context='FACES')
    bmesh.ops.delete(bm, geom=[v for v in bm.verts if not v.link_faces], context='VERTS')
    bm.normal_update()
    moved = 0
    for v in bm.verts:
        d = v.co - centre
        if d.length > size * 2.6 or d.y > 0.03 or not any(e.is_boundary for e in v.link_edges):
            continue
        th = math.atan2(d.z, d.x)
        want = rr(th) * (1.0 + 0.05 * math.sin(23 * th + ph[3]) + 0.03 * math.sin(41 * th))
        k = want / max(scaled(d), 1e-6)
        v.co.x = centre.x + d.x * k
        v.co.z = centre.z + d.z * k
        v.co += v.normal * 0.0015
        moved += 1
    bm.to_mesh(ob.data)
    bm.free()
    ob.data.update()
    log(f'shirt: the tear cut, {moved} edge points laid on its outline')


BAND_OPEN = 24.0         # degrees either side of the throat the shirt's band leaves open
BAND_H = (0.030, 0.034)  # its height at the throat and at the back (m)


def shirt_band(fig, body_ob):
    """The shirt's collarless band round the base of the neck, 6 mm off the skin, open at the throat (no stud), its
    top edge over the neck's skin and its foot over the shirt's neckline."""
    tree = gm.bvh_objects([body_ob])
    neck = fig.neck
    th = np.radians(np.linspace(BAND_OPEN, 360.0 - BAND_OPEN, 64))
    rows = []
    for t in np.linspace(1.0, 0.0, 5):                    # rows top down, faces out
        row = []
        for a in th:
            back = 0.5 - 0.5 * math.cos(a)
            h = BAND_H[0] + (BAND_H[1] - BAND_H[0]) * back
            z = neck[2] - 0.012 + h * t
            d = Vector((math.sin(a), -math.cos(a), 0.0))
            o = Vector((neck[0], neck[1], z))
            hit = tree.ray_cast(o + d * 0.3, -d, 0.3)
            r = (0.3 - hit[3]) if hit[0] is not None else 0.06
            r += 0.006 + 0.002 * (1.0 - t)
            row.append((neck[0] + d.x * r, neck[1] + d.y * r, z))
        rows.append(row)
    R = np.array(rows)
    return gm.mesh('ShirtBand', gm.grid(R[..., 0], R[..., 1], R[..., 2]), slot('Shirt'))


# ------------------------------------------------------------------------------------------------------ the belt

def build_belt(fig, under_obs):
    """A plain leather belt round the waist over the trousers, an iron frame buckle in front, an iron ring at the left
    hip the toll chain hangs from."""
    tree = gm.bvh_objects(under_obs)
    n = 112
    th = np.linspace(-math.pi, math.pi, n, endpoint=False)
    dx, dy = gm.radial(th, FWD)
    z0 = fig.waist - 0.012
    path, nrm = [], []
    for i in range(n):
        d = Vector((dx[i], dy[i], 0.0))
        o = Vector((fig.centre[0], fig.centre[1], z0)) + d * 0.6
        hit = tree.ray_cast(o, -d, 1.0)
        p = np.array(hit[0]) if hit[0] is not None else np.array(o - d * 0.45)
        nn = np.array(hit[1]) if hit[0] is not None else np.array(d)
        nn[2] *= 0.5
        path.append(p + gm.unit(nn) * 0.005)
        nrm.append(gm.unit(nn))
    path, nrm = np.array(path), np.array(nrm)
    path = gm.laplacian(path, [[i, (i + 1) % n] for i in range(n)], 3)
    up = gm.unit(np.array([0.0, 0.0, 1.0]) - nrm * nrm[:, 2:3])
    P = np.array([path + up * 0.019, path + nrm * 0.002, path - up * 0.019])
    belt = gm.mesh('Belt', gm.grid(P[..., 0], P[..., 1], P[..., 2], wrap=True), slot('Belt'))
    i0 = int(np.argmin(np.abs(th)))
    f = path[i0] + nrm[i0] * 0.005
    side = gm.unit(np.cross(up[i0], nrm[i0]))
    frame = None
    for (a, b, w) in (((-1, -1), (1, -1), 0.006), ((1, -1), (1, 1), 0.006), ((1, 1), (-1, 1), 0.006),
                      ((-1, 1), (-1, -1), 0.006), ((0.1, -1), (0.1, 1), 0.004)):
        p0 = f + side * a[0] * 0.026 + up[i0] * a[1] * 0.024
        p1 = f + side * b[0] * 0.026 + up[i0] * b[1] * 0.024
        bar = gm.tube(np.array([p0, p1]), np.array([w * 0.5, w * 0.5]), n=8, up=nrm[i0])
        frame = bar if frame is None else frame + bar
    buckle = gm.mesh('Buckle', frame, slot('Iron'))
    fig.belt = (th, path, nrm, up)
    return [belt, buckle]


def belt_frame(fig, angle):
    """Point, outward normal, up and the direction round the belt at angle (0 the front, + his left)."""
    th, path, nrm, up = fig.belt
    i = int(np.argmin(np.abs((th - angle + math.pi) % (2 * math.pi) - math.pi)))
    tangent = gm.unit(path[(i + 1) % len(th)] - path[i - 1])
    return path[i], nrm[i], up[i], tangent


# ------------------------------------------------------------------------------------------------------ the coat

SIDE_SEAM = 1.83
VENT_TOP = 0.88
COAT_EASE = (0.016, 0.011)
FRONT_PULL = 0.016       # how much closer the coat's front edges lie over the shirt from the waist up (m)
N_COLS, N_ROWS = 108, 50


def tatters(th):
    """How far each column of the coat's hem is torn up (m, + shorter) at angle th: pointed strips 8-15 cm wide, the
    notches between them 4-9 cm deep, each point a little longer or shorter; a notch at the back vent (both its edges
    end alike)."""
    rng = np.random.default_rng(17)
    edges = [-math.pi]
    while edges[-1] < math.pi - 0.2:
        edges.append(edges[-1] + rng.uniform(0.28, 0.50))
    edges[-1] = math.pi
    edges = np.array(edges)
    depth = rng.uniform(0.04, 0.09, len(edges) - 1)
    point = rng.uniform(-0.01, 0.02, len(edges) - 1)
    t = np.atleast_1d(th)
    t = (t + math.pi) % (2 * math.pi) - math.pi
    k = np.clip(np.searchsorted(edges, t) - 1, 0, len(edges) - 2)
    s = (t - edges[k]) / (edges[k + 1] - edges[k])
    return point[k] + depth[k] * np.abs(2.0 * s - 1.0) ** 1.4


def hem_at(th):
    """The coat's hem height round it before its tatters are torn (tear_hem): the tatters' points' length, a little
    uneven, lower at the back. (Built with its tatters, the hem's zigzag of short and long columns crumpled the whole
    back into fine pleats in the simulation; plain, it settles into a coat's broad folds.)"""
    return HEM - 0.07 + 0.010 * np.sin(th * 5 + 1.0) - 0.012 * np.cos(th) ** 2 * (np.cos(th) < 0)


TEAR_BAND = 0.16         # the tatters are torn into the settled coat's last 16 cm of each column


def tear_hem(coat):
    """Tears the settled coat's hem into its tatters: each column (the 'Col' attribute, its rows 'Row') shortened by its
    'Tear' (tatters at its angle) within its last TEAR_BAND of length, its points slid up along the settled cloth, so the
    folds the simulation gave it stay."""
    me = coat.data
    n = len(me.vertices)
    col, row, tear = (np.zeros(n, dtype=np.int32), np.zeros(n, dtype=np.int32), np.zeros(n, dtype=np.float32))
    me.attributes['Col'].data.foreach_get('value', col)
    me.attributes['Row'].data.foreach_get('value', row)
    me.attributes['Tear'].data.foreach_get('value', tear)
    co = gm._object_coords(coat)
    out = co.copy()
    for c in np.unique(col):
        ids = np.nonzero(col == c)[0]
        ids = ids[np.argsort(row[ids])][::-1]                   # from the hem up
        pts = co[ids]
        arc = np.concatenate([[0.0], np.cumsum(np.linalg.norm(np.diff(pts, axis=0), axis=1))])
        d = float(np.clip(tear[ids[0]], 0.0, TEAR_BAND * 0.8))
        band = arc < TEAR_BAND
        want = d + arc[band] * (TEAR_BAND - d) / TEAR_BAND     # where along the column (from its old end) each goes
        for k, a in zip(np.nonzero(band)[0], want):
            out[ids[k]] = np.array([np.interp(a, arc, pts[:, j]) for j in range(3)])
    me.vertices.foreach_set('co', out.astype(np.float32).ravel())
    me.update()
    log(f'coat: the hem torn up {tear.min() * 100:.0f}-{tear.max() * 100:.0f} cm into its tatters')


def build_coat(fig, under_pts, clear_obs=()):
    """The long slate coat: draped round the torso from under the collar to the hem, open in a V over the shirt and
    wide below the waist, a vent up the back, flared and fluted toward the hem (Ellis.py's duster); the hem's tatters are
    torn into it after it settles (tear_hem)."""
    rng = np.random.default_rng(21)
    top = fig.neck[2] - 0.022           # under the cape's top edge and the collar (at -1.2 cm it rose through them)
    low = HEM - 0.09
    fine = np.linspace(-math.pi, math.pi, 720, endpoint=False)
    hip = fig.hip_z - 0.04
    flare_amt = 0.12 if SIM else 0.08
    ease = lambda v: COAT_EASE[0] * gm.smoothstep(0.02, 0.18, v) + COAT_EASE[1] * gm.smoothstep(0.30, 0.50, v)
    flare = lambda th, v: ease(v) + flare_amt * gm.smoothstep(0.42, 1.0, v) ** 1.3 * (0.35 + 0.65 * np.abs(np.sin(th)))
    nz_fine = 90
    X, Y, Z, R = gm.drape(under_pts, fig.centre, FWD, fine, lambda th: top, lambda th: low, nz_fine, 0.016,
                          hb=math.radians(7.0), zb=0.01, smooth=(10, 3), wrap=True, flare=flare)
    zs_fine = Z[:, 0]
    ph = rng.random(8) * 6.28
    v_rows = np.linspace(0.0, 1.0, N_ROWS)
    TH, RR, ZZ = [], [], []
    half = N_COLS // 2
    for j, v in enumerate(v_rows):
        z_mid = top + (HEM - top) * v
        r_front = np.interp(0.0, fine, R[int(np.argmin(np.abs(zs_fine - z_mid)))], period=2 * math.pi)
        a_l = math.asin(min(0.98, float(opening(fig, z_mid)) / max(r_front, 0.05)))
        a_r = math.asin(min(0.98, float(opening(fig, z_mid)) / max(r_front, 0.05)))
        th = np.concatenate([np.linspace(a_l, math.pi, half), np.linspace(-math.pi, -a_r, N_COLS - half)])
        z = top + (hem_at(th) - top) * v
        cols = np.round((th + math.pi) / (2 * math.pi) * 720).astype(int) % 720
        r = np.array([np.interp(z[i], zs_fine[::-1], R[::-1, cols[i]]) for i in range(len(th))])
        # the fronts lie closer over the shirt from the waist up (standing off it, the coat's dark inside showed
        # round the V)
        e = np.minimum(np.abs(th - a_l), np.abs(th + a_r))
        r -= FRONT_PULL * gm.smoothstep(0.35, 0.0, e) * gm.smoothstep(fig.waist - 0.05, fig.waist + 0.10, z)
        below = gm.smoothstep(hip + 0.05, HEM, z)
        fold = (0.010 if SIM else 0.026) * below ** 1.3 * (0.45 * np.sin(th * 4.0 + ph[0] + 0.7 * np.sin(v * 2.0 + ph[1]))
                                                          + 0.35 * np.sin(th * 7.3 + ph[2] + 0.9 * np.sin(v * 2.6 + ph[3]))
                                                          + 0.20 * np.sin(th * 12.7 + ph[4] + v * 1.5))
        fold += 0.005 * gm.smoothstep(fig.waist + 0.15, fig.waist, z) * gm.smoothstep(hip - 0.15, hip, z) * np.sin(th * 9.1 + ph[5])
        TH.append(th)
        RR.append(r + fold)
        ZZ.append(z)
    TH, RR, ZZ = np.array(TH), np.array(RR), np.array(ZZ)
    dx, dy = gm.radial(TH, FWD)
    X = fig.centre[0] + RR * dx
    Y = fig.centre[1] + RR * dy
    if clear_obs:
        P = gm.clear(np.stack([X, Y, ZZ], -1).reshape(-1, 3), gm.bvh_objects(clear_obs), 0.008, 0.05).reshape(X.shape + (3,))
        X, Y, ZZ = P[..., 0], P[..., 1], P[..., 2]
    piece = gm.grid(X, Y, ZZ)
    ob = gm.mesh('Coat', piece, slot('Coat'))
    # each point's column and row, and its column's tear (tear_hem, after the simulation)
    jj, ii = np.divmod(np.arange(N_ROWS * N_COLS), N_COLS)
    tear = (tatters(TH[-1]) + 0.012)[ii]
    for key, kind, vals in (('Col', 'INT', ii), ('Row', 'INT', jj), ('Tear', 'FLOAT', tear)):
        ob.data.attributes.new(key, kind, 'POINT').data.foreach_set('value', vals.astype(np.int32 if kind == 'INT' else np.float32))
    bm = bmesh.new()
    bm.from_mesh(ob.data)
    bm.verts.ensure_lookup_table()
    zs = ZZ[:, half]
    pairs = [(bm.verts[j * N_COLS + half - 1], bm.verts[j * N_COLS + half]) for j in range(N_ROWS) if zs[j] >= VENT_TOP]
    vent = [(bm.verts[j * N_COLS + half - 1], bm.verts[j * N_COLS + half]) for j in range(N_ROWS) if zs[j] < VENT_TOP]
    for a, b in pairs:
        a.co = b.co = (a.co + b.co) * 0.5
    bmesh.ops.remove_doubles(bm, verts=[v for p in pairs for v in p], dist=1e-5)
    bm.verts.index_update()
    ob['vent'] = [v.index for p in vent for v in p]
    bm.to_mesh(ob.data)
    bm.free()
    co = gm._object_coords(ob)
    rel = co[:, :2] - fig.centre
    f, l = gm.frame2d(FWD)
    ang = np.arctan2(rel @ l, rel @ f)
    rad = np.linalg.norm(rel, axis=1)
    wrap = lambda a: np.abs((a + math.pi) % (2 * math.pi) - math.pi)
    back = np.where(co[:, 2] > VENT_TOP - 0.01, wrap(ang - math.pi) * rad, 1.0)
    sides = np.minimum(wrap(ang - SIDE_SEAM), wrap(ang + SIDE_SEAM)) * rad
    cc.set_seams(ob, np.minimum(back, sides))
    return ob


LEG_FILL_DROP = 0.03


def leg_fill(fig, obs, z0, z1, gap=0.004, coat=None):
    """A collider only the simulation sees filling between the legs (Ellis.py's): the coat's back hangs straight over
    the gap, dropping near plumb from the seat; with coat, from the coat's own back line at the top of the fill, 1.2 cm
    in (his stooped shoulders hang the coat's back well behind his seat: from the seat it swung in and pleated)."""
    pts = np.concatenate([gm.object_points(o) for o in obs])
    c = fig.centre
    A = np.linspace(-math.pi, math.pi, 64, endpoint=False)
    D = np.stack(gm.radial(A, FWD), axis=1)
    back = gm.smoothstep(-0.2, -0.6, np.cos(A))
    zs, radii = [], []
    for z in np.arange(z1, z0 - 1e-6, -0.03):
        sl = pts[np.abs(pts[:, 2] - z) < 0.02][:, :2]
        if len(sl) >= 3:
            zs.append(z)
            radii.append(cc.hull_radii(cc.hull_2d(sl) - c, D) + gap)
    rings = []
    run = radii[0]
    if coat is not None:
        cp = gm._object_coords(coat)
        sl = cp[np.abs(cp[:, 2] - zs[0]) < 0.03][:, :2] - c
        ang = np.arctan2(sl @ gm.frame2d(FWD)[1], sl @ gm.frame2d(FWD)[0])
        rad = np.linalg.norm(sl, axis=1)
        for i, a in enumerate(A):
            near = np.abs((ang - a + math.pi) % (2 * math.pi) - math.pi) < math.radians(8)
            if near.any():
                run[i] = max(run[i], float(np.median(rad[near])) - 0.012)
    for k, (z, r) in enumerate(zip(zs, radii)):
        if k:
            run = np.maximum(r, run - LEG_FILL_DROP * (zs[k - 1] - z))
        r = r + back * (run - r)
        rings.append(np.c_[c + D * r[:, None], np.full(len(D), z)])
    R = np.array(rings)
    return gm.mesh('_LegFill', gm.grid(R[..., 0], R[..., 1], R[..., 2], wrap=True))


def welded(ob, pairs, name):
    """A copy of mesh ob with each vertex pair (keep, drop) welded into one, and for each of ob's vertices its vertex
    in the copy."""
    co = gm._object_coords(ob)
    to = np.arange(len(co))
    to[pairs[:, 1]] = pairs[:, 0]
    keep = np.ones(len(co), dtype=bool)
    keep[pairs[:, 1]] = False
    at = (np.cumsum(keep) - 1)[to]
    faces = []
    for p in ob.data.polygons:
        f = [int(at[v]) for v in p.vertices]
        f = [v for k, v in enumerate(f) if v != f[k - 1]]
        if len(set(f)) >= 3:
            faces.append(f)
    me = bpy.data.meshes.new(name)
    me.from_pydata(co[keep].tolist(), [], faces)
    copy = bpy.data.objects.new(name, me)
    bpy.context.scene.collection.objects.link(copy)
    return copy, at


def lap_vent(fig, coat, vent):
    """The settled vent's edges lapped as a built coat's: the left flap 4 mm out and across the right's."""
    co = gm._object_coords(coat)
    k = gm.smoothstep(VENT_TOP, VENT_TOP - 0.08, co[vent[:, 0], 2])[:, None]
    for col, out, across in ((0, 0.004, 0.025), (1, 0.0, -0.02)):
        p = co[vent[:, col]]
        rel = p[:, :2] - fig.centre
        r = np.linalg.norm(rel, axis=1)
        ang = cc.round_angle(p, fig.centre)
        ex, ey = gm.radial(ang, FWD)
        tx, ty = gm.radial(ang + math.pi / 2, FWD)
        p[:, 0] += (ex * out + tx * across * r) * k[:, 0]
        p[:, 1] += (ey * out + ty * across * r) * k[:, 0]
        co[vent[:, col]] = p
    coat.data.vertices.foreach_set('co', co.astype(np.float32).ravel())
    coat.data.update()


SEAT_SMOOTH = 24         # passes of smoothing over the settled coat's back under the seat


def seat_smooth(fig, cloth):
    """The settled coat's back from the seat to the knees smoothed (Taubin's, its volume kept: SEAT_SMOOTH passes), so
    its fine upright pleats go and its broad folds stay (the cloth gathered into them from the stooped back's width;
    bending 28 instead of 12 in the simulation left them as they were)."""
    P = gm._object_coords(cloth)
    rel = P[:, :2] - fig.centre
    f, l = gm.frame2d(FWD)
    cos_a = (rel @ f) / np.maximum(np.linalg.norm(rel, axis=1), 1e-6)
    w = gm.smoothstep(-0.25, -0.75, cos_a) * gm.smoothstep(fig.hip_z + 0.06, fig.hip_z - 0.04, P[:, 2]) \
        * gm.smoothstep(HEM + 0.05, fig.hip_z - 0.55, P[:, 2])
    P = gm.laplacian(P, [list(p.vertices) for p in cloth.data.polygons], SEAT_SMOOTH, weight=w)
    cloth.data.vertices.foreach_set('co', P.astype(np.float32).ravel())
    cloth.data.update()


def settle(fig, coat, colliders):
    """The coat's skirt falls into its folds (Blender's cloth over the body, trousers, shirt, belt and boots, held above
    the hips; kept in Crane.drape.json). Bending 12: a heavy coat's broad folds (Ellis.py's). Its back under the seat
    smoothed after (seat_smooth)."""
    vent = np.array(coat['vent'], dtype=np.int64).reshape(-1, 2)
    cloth, at = welded(coat, vent, '_CoatCloth')
    zd = gm._object_coords(cloth)[:, 2]
    t0 = time.time()
    ran = gm.simulate([cloth], [gm.smoothstep(fig.hip_z - 0.10, fig.hip_z + 0.06, zd)], colliders, cc.DRAPE_FILE,
                      frames=int(cc.arg_value('--frames', 90)), stiff=(40.0, 40.0, 15.0, 12.0), mass=0.3,
                      self_collision=True,
                      note='The settled coat of Hollis Crane (Art/Models/Characters/Crane.py writes it: the cloth '
                           'simulation\'s result for the digest of its inputs).')
    seat_smooth(fig, cloth)
    coat.data.vertices.foreach_set('co', gm._object_coords(cloth)[at].astype(np.float32).ravel())
    coat.data.update()
    bpy.data.objects.remove(cloth)
    lap_vent(fig, coat, vent)
    tear_hem(coat)
    log(f'cloth: {"simulated" if ran else "kept from Crane.drape.json"} ({time.time() - t0:.0f} s)')


# ------------------------------------------------------------------------------------------------------ the sleeves

SLEEVE_END = 0.012       # the sleeve stops this far up the forearm from the wrist joint, under its cuff
SLEEVE_TUCK = 0.014      # how close the sleeve's end lies on the forearm, gathered under the cuff (m)
CUFF_LEN = 0.065         # the turned cuff's length up the forearm
CUFF_ROOM = 0.004        # its room over the sleeve under it


def build_sleeves(fig, under):
    """The coat's sleeves: the arms pushed out loose (looser toward the cuff) and smoothed, folded at the elbow and
    bunched above the cuff, gathered in under the cuff to the wrist."""
    keep = (fig.w_arm > 0.45) & (fig.w_hand < 0.25)
    for s, sx in (('l', 1.0), ('r', -1.0)):
        e, w = fig.sk[f'lowerarm_{s}'][0], fig.sk[f'lowerarm_{s}'][1]
        beyond = ((fig.wv - w) @ gm.unit(w - e)) > -SLEEVE_END
        keep &= ~((fig.wv[:, 0] * sx > 0) & beyond)
    along = np.zeros(len(fig.wv))
    for s, sx in (('l', 1.0), ('r', -1.0)):
        a, w = fig.sk[f'upperarm_{s}'][0], fig.sk[f'lowerarm_{s}'][1]
        t = np.clip(((fig.wv - a) @ gm.unit(w - a)) / np.linalg.norm(w - a), 0, 1)
        along = np.where(fig.wv[:, 0] * sx > 0, t, along)
    off = 0.018 + 0.014 * along
    for s, sx in (('l', 1.0), ('r', -1.0)):
        e, w = fig.sk[f'lowerarm_{s}'][0], fig.sk[f'lowerarm_{s}'][1]
        up = -((fig.wv - w) @ gm.unit(w - e))
        gather = gm.smoothstep(SLEEVE_END + CUFF_LEN * 0.6, SLEEVE_END + CUFF_LEN + 0.05, up)
        off = np.where(fig.wv[:, 0] * sx > 0, SLEEVE_TUCK + (off - SLEEVE_TUCK) * gather, off)
    piece, used = gm.shell(fig.wv, fig.faces, fig.fuv, fig.face_mask(keep), off, iters=18)
    v = piece.verts
    nrm = gm.vertex_normals(v, piece.faces)
    for s, sx in (('l', 1.0), ('r', -1.0)):
        el, w = fig.sk[f'lowerarm_{s}'][0], fig.sk[f'lowerarm_{s}'][1]
        side = v[:, 0] * sx > 0
        d = gm.unit(w - el)
        t = (v - el) @ d
        ring = np.cos(t / 0.055 * 2 * math.pi + 0.8 * np.sin(np.arctan2(v[:, 2] - el[2], v[:, 1] - el[1]) * 2)) * 0.5 + 0.5
        bunch = gm.smoothstep(0.08, 0.16, t) * gm.smoothstep(np.linalg.norm(w - el) - CUFF_LEN + 0.01,
                                                           np.linalg.norm(w - el) - CUFF_LEN - 0.03, t) * 0.009 * ring ** 2
        crook = np.exp(-(t / 0.06) ** 2) * 0.007 * np.sin(t * 90)
        v[side] += nrm[side] * (bunch[side] + crook[side])[:, None]
    piece.verts = gm.clear(v, gm.bvh_objects(under), 0.012, 0.1)
    return gm.mesh('Sleeves', piece, slot('Coat'))


def build_cuffs(fig, sleeves):
    """The turned-back cuffs: a short tube round each sleeve's end from the wrist CUFF_LEN up the forearm, fitted round
    the sleeve angle by angle (the smallest ellipse round the forearm's axis holding its slice) with CUFF_ROOM, its
    mouth's lip rolled out 3 mm, a little flared."""
    sv = gm.object_points(sleeves)
    out = []
    for s in 'lr':
        e, w = fig.sk[f'lowerarm_{s}'][0], fig.sk[f'lowerarm_{s}'][1]
        axis = gm.unit(w - e)
        steps = np.linspace(0, 1, 7)
        path = np.array([w + axis * (0.004 - (CUFF_LEN + 0.004) * t) for t in steps])
        up_t = -axis
        a = gm.unit(np.cross(up_t, [0.0, -1.0, 0.0]))
        b = np.cross(up_t, a)
        mine = sv[((sv - e) @ axis > 0.0) & (np.linalg.norm(np.cross(sv - e, axis), axis=1) < 0.09) & ((sv[:, 0] > 0) == (s == 'l'))]
        ang = np.linspace(0, 2 * math.pi, 24, endpoint=False)
        axes = np.zeros((len(steps), 2))
        for i, p in enumerate(path):
            q = mine - p
            sl = q[np.abs(q @ axis) < 0.01]
            if not len(sl):
                axes[i] = axes[i - 1] if i else (0.035, 0.03)
                continue
            x, y = np.abs(sl @ a), np.abs(sl @ b)
            best = None
            for ra in np.linspace(x.max() * 1.001, x.max() * 1.8, 40):
                rb = float((y / np.sqrt(np.maximum(1.0 - (x / ra) ** 2, 1e-6))).max())
                if best is None or ra * rb < best[0] * best[1]:
                    best = (ra, rb)
            axes[i] = best
        axes = np.maximum(axes, axes.max(axis=0) * 0.9)
        fit = 1.0 / np.sqrt((np.cos(ang)[None] / axes[:, :1]) ** 2 + (np.sin(ang)[None] / axes[:, 1:]) ** 2)
        r = fit + CUFF_ROOM + 0.004 * (1.0 - steps)[:, None]
        r[0] += 0.003
        pc = gm.tube(path, np.ones((len(steps), 2)), n=len(ang), up=(0.0, -1.0, 0.0), shape=lambda i, _a: r[i])
        out.append(gm.mesh(f'Cuff_{s}', pc, slot('Coat')))
    return out


# ------------------------------------------------------------------------------------------------------ the cape

IDLE_ARM = 10.0
CAPE_EASE = 0.010
CAPE_SKIN = ('clavicle_l', 'clavicle_r', 'upperarm_l', 'upperarm_r')
CAPE_OPEN = 48.0         # degrees either side of the front the cape leaves open (the portrait's 52; Ellis's 26): the
                         # star, the ember's tear and the chain across the chest show
CAPE_DROP = 0.15         # the cape's hem below the shoulder line, before its tatters


def arm_turns(fig):
    out = {}
    for s, sx in (('l', 1.0), ('r', -1.0)):
        h, t = fig.sk[f'upperarm_{s}'][0], fig.sk[f'upperarm_{s}'][1]
        a = math.radians(IDLE_ARM)
        to = Vector((sx * math.sin(a), -0.04, -math.cos(a))).normalized()
        out[sx] = (h, np.array(Vector(tuple(gm.unit(t - h))).rotation_difference(to).to_matrix()))
    return out


def arm_share(fig, W, sx):
    s = 'l' if sx > 0 else 'r'
    cols = [j for j, n in enumerate(fig.names) if n.endswith('_' + s) and
            n.split('_')[0] in ('upperarm', 'lowerarm', 'hand', 'thumb', 'index', 'middle', 'ring', 'pinky')]
    return W[:, cols].sum(1)


def arms_down(fig, P, W, back=False):
    out = P.copy()
    for sx, (h, R) in arm_turns(fig).items():
        w = arm_share(fig, W, sx)[:, None]
        turned = (P - h) @ (R if back else R.T) + h
        out += w * (turned - P)
    return out


def idle_skin_weights(fig, posed, points):
    from mathutils.bvhtree import BVHTree
    used = fig.b.used
    remap = -np.ones(len(fig.wv), dtype=np.int64)
    remap[used] = np.arange(len(used))
    tris = [(remap[f[0]], remap[f[k]], remap[f[k + 1]]) for f in fig.faces for k in range(1, len(f) - 1)]
    tris = [tuple(int(i) for i in t) for t in tris if min(t) >= 0]
    tree = BVHTree.FromPolygons([tuple(map(float, p)) for p in posed], tris, all_triangles=True)
    Wu = fig.Wv[used]
    out = np.zeros((len(points), Wu.shape[1]))
    for i, p in enumerate(points):
        loc, _n, fi, _d = tree.find_nearest(Vector(tuple(p)))
        if loc is None:
            continue
        t = tris[fi]
        bc = hu.barycentric(np.array(loc), *(posed[j] for j in t))
        out[i] = sum(bc[k] * Wu[t[k]] for k in range(3))
    return out


def cape_tatters(th):
    """The cape's hem torn into pointed strips (the portrait's "ragged"): strips 10-20 degrees wide round the neck,
    the notches between them 2-6 cm deep, each point a little longer or shorter."""
    rng = np.random.default_rng(5)
    edges = [math.radians(CAPE_OPEN)]
    end = math.radians(360.0 - CAPE_OPEN)
    while edges[-1] < end - 0.15:
        edges.append(edges[-1] + rng.uniform(0.17, 0.34))
    edges[-1] = end
    edges = np.array(edges)
    depth = rng.uniform(0.02, 0.06, len(edges) - 1)
    point = rng.uniform(-0.01, 0.015, len(edges) - 1)
    t = np.atleast_1d(np.asarray(th, dtype=np.float64)) % (2 * math.pi)
    k = np.clip(np.searchsorted(edges, t) - 1, 0, len(edges) - 2)
    s = np.clip((t - edges[k]) / (edges[k + 1] - edges[k]), 0.0, 1.0)
    return point[k] + depth[k] * np.abs(2.0 * s - 1.0) ** 1.3


def build_cape(fig, body, coat, sleeves, extra_pts=None):
    """The ragged shoulder cape (Ellis.py's build_cape: draped over the arms let down to the game's idle, its top
    carried up into the bind pose by its share of the upper arm), open in front (CAPE_OPEN), its hem torn into pointed
    strips (cape_tatters) with a few rips: holes cut through it low down."""
    neck = fig.neck
    sz = fig.shoulder_z
    used = fig.b.used
    posed_body = arms_down(fig, fig.wv[used], fig.Wv[used])
    sl = gm.object_points(sleeves)
    posed_sleeves = arms_down(fig, sl, fig.b.weights_at(sl)[1])
    parts = [posed_body[posed_body[:, 2] > sz - 0.30], gm.object_points(coat), posed_sleeves]
    if extra_pts is not None and len(extra_pts):
        parts.append(extra_pts)
    under_pts = np.concatenate(parts)
    th = np.linspace(math.radians(CAPE_OPEN), math.radians(360.0 - CAPE_OPEN), 150)
    top = lambda t: neck[2] - 0.006
    point = lambda t: 0.05 * max(0.0, -math.cos(t)) ** 3
    bot = lambda t: sz - CAPE_DROP - 0.035 * abs(math.sin(t)) - point(t) + float(cape_tatters(t)[0])
    X, Y, Z, R = gm.drape(under_pts, neck[:2], FWD, th, top, bot, 26, 0.026, hb=math.radians(7), zb=0.012, smooth=(4, 3))
    top_r = R[0].max()
    raw = gm.drape(under_pts, neck[:2], FWD, th, top, bot, 26, 0.008, hb=math.radians(7), zb=0.012, smooth=(0, 0))[3]
    grow = np.maximum(raw - R, 0.0)
    if grow.any():
        f, l = gm.frame2d(FWD)
        R = R + grow
        X = neck[0] + R * (np.cos(th)[None, :] * f[0] + np.sin(th)[None, :] * l[0])
        Y = neck[1] + R * (np.cos(th)[None, :] * f[1] + np.sin(th)[None, :] * l[1])
    v = np.linspace(0, 1, Z.shape[0])[:, None]
    rng = np.random.default_rng(5)
    ph = rng.random(6) * 6.28
    T = th[None, :]
    corner = gm.smoothstep(0.0, 0.6, np.minimum(T - th[0], th[-1] - T))
    shoulders = 0.018 * gm.smoothstep(0.15, 0.55, v) * np.sin(T) ** 2
    flute = shoulders + CAPE_EASE * gm.smoothstep(0.0, 0.4, v) + (0.022 + 0.014 * corner) * v ** 1.4 \
        + v ** 1.1 * (0.012 * np.sin(T * 9.0 + ph[0] + 0.8 * np.sin(v * 3 + ph[1]))
                      + 0.008 * np.sin(T * 13.3 + ph[2] + 0.6 * np.sin(v * 2 + ph[3]))
                      + 0.004 * np.sin(T * 21.7 + ph[4] + v * 4)) \
        + 0.004 * v * np.sin(T * 31.0 + ph[5] + v * 6)
    dx, dy = gm.radial(th[None, :], FWD)
    piece = gm.grid(X + flute * dx, Y + flute * dy, Z)
    P = piece.verts
    W = idle_skin_weights(fig, posed_body, P)
    hem_z = np.repeat(Z[-1][None, :], Z.shape[0], 0).ravel()
    chain = gm.smoothstep(sz, (sz + hem_z) * 0.5, P[:, 2])
    keep = {}
    for bone in CAPE_SKIN:
        keep[bone] = W[:, fig.col[bone]]
    rest = 1.0 - sum(keep.values())
    keep['spine_05'] = keep.get('spine_05', 0.0) + rest
    for sx, (h, Rm) in arm_turns(fig).items():
        s = 'l' if sx > 0 else 'r'
        sel = (P[:, 0] * sx) >= 0.0
        for k in np.nonzero(sel)[0]:
            w = (1.0 - chain[k]) * keep[f'upperarm_{s}'][k]
            if w <= 1e-4:
                continue
            A = np.eye(3) + w * (Rm - np.eye(3))
            P[k] = np.linalg.solve(A, P[k] - w * (h - Rm @ h))
    piece.verts = P
    ob = gm.mesh('Cape', piece, slot('Coat'))
    for bone, w in keep.items():
        ob.data.attributes.new('CapeW_' + bone, 'FLOAT', 'POINT').data.foreach_set('value', np.asarray(w, np.float32))
    ob.data.attributes.new('CapeChain', 'FLOAT', 'POINT').data.foreach_set('value', chain.astype(np.float32))
    # (round 1 tore holes through it low down by deleting faces: square holes, like pixels; its torn hem is enough)
    arm = max(float(keep['upperarm_l'].max()), float(keep['upperarm_r'].max()))
    log(f'cape: draped over the arms let down {IDLE_ARM:.0f} degrees; its top rides the upper arms up to {arm:.2f}; '
        f'{grow.max() * 100:.1f} cm out at most over what the smoothing let in')
    return ob, top_r


def cape_weights(fig, ob):
    n = len(ob.data.vertices)
    parts = {}
    for bone in CAPE_SKIN + ('spine_05',):
        a = np.zeros(n, dtype=np.float32)
        if 'CapeW_' + bone in ob.data.attributes:
            ob.data.attributes['CapeW_' + bone].data.foreach_get('value', a)
        parts[bone] = np.clip(a, 0.0, 1.0)
    return hu.blend(fig.names, n, parts)


def idle_sleeves(fig, sleeves):
    sl = gm.object_points(sleeves)
    return arms_down(fig, sl, fig.b.weights_at(sl)[1])


def cape_idle(fig, ob, co, back=False):
    n = len(co)
    chain = np.zeros(n, dtype=np.float32)
    if 'CapeChain' in ob.data.attributes:
        ob.data.attributes['CapeChain'].data.foreach_get('value', chain)
    out = co.copy()
    for sx, (h, Rm) in arm_turns(fig).items():
        a = np.zeros(n, dtype=np.float32)
        key = 'CapeW_upperarm_' + ('l' if sx > 0 else 'r')
        if key in ob.data.attributes:
            ob.data.attributes[key].data.foreach_get('value', a)
        w = (1.0 - np.clip(chain, 0.0, 1.0)) * np.clip(a, 0.0, 1.0)
        for k in np.nonzero(w > 1e-4)[0]:
            A = np.eye(3) + w[k] * (Rm - np.eye(3))
            t = w[k] * (h - Rm @ h)
            out[k] = np.linalg.solve(A, co[k] - t) if back else A @ co[k] + t
    return out


def clear_cape(fig, capes, under, sleeves, gap=0.003):
    """Pushes the reduced cape back outside the reduced coat (and what lies on it) and the reduced sleeves, all as the
    game's idle has them (Ellis.py's). (This also brings the cape's top behind the neck down onto the coat's top edge,
    under the collar's foot. Left high there, the cape took the collar's foot out through it, so instead Crane.py's
    tuck_coat_top draws the coat's top in under the cape.)"""
    from mathutils.bvhtree import BVHTree
    trees = [gm.bvh_objects(under)] if under else []
    for s in sleeves:
        P = idle_sleeves(fig, s)
        trees.append(BVHTree.FromPolygons([tuple(map(float, p)) for p in P], [tuple(p.vertices) for p in s.data.polygons]))
    for ob in capes:
        co = cape_idle(fig, ob, gm._object_coords(ob))
        for tree in trees:
            co = gm.clear(co, tree, gap, 0.03)
        co = cape_idle(fig, ob, co, back=True)
        ob.data.vertices.foreach_set('co', co.astype(np.float32).ravel())
        ob.data.update()


# ------------------------------------------------------------------------------------------------------ the collar

COLLAR_OPEN = 30.0       # degrees either side of the front the collar leaves open
COLLAR_GAP = 0.010       # its room over the shirt's band and the neck
COLLAR_OVER = 0.009      # its wall this far outside the cape's top edge (at the idle): the cape is sewn on under it (its
                         # foot inside the cape's top edge, the long hair slid down between them and under the cape)


def build_collar(fig, inside, cape):
    """The coat's low collar, a band standing up round the neck over the cape's top edge (the portrait's "short
    funnel": 3 cm at the throat, 4 at the back), rolled out a little at its top edge, worn soft; COLLAR_GAP off the
    shirt's band and the neck; its wall COLLAR_OVER outside the cape's top edge at the idle."""
    from mathutils.bvhtree import BVHTree
    neck = fig.neck
    th = np.linspace(math.radians(COLLAR_OPEN), math.radians(360.0 - COLLAR_OPEN), 72)
    pts, faces, off = [], [], 0
    for o in inside:
        p = gm.object_points(o)
        pts.append(p)
        faces += [[i + off for i in poly.vertices] for poly in o.data.polygons]
        off += len(p)
    tree = BVHTree.FromPolygons([tuple(map(float, q)) for q in np.concatenate(pts)], faces)

    def env(a, z):
        best = 0.0
        for da in np.radians((-4.0, 0.0, 4.0)):
            d = Vector((math.sin(a + da), -math.cos(a + da), 0.0))
            for dz in (-0.006, 0.0, 0.006):
                hit = tree.ray_cast(Vector((neck[0], neck[1], z + dz)) + d * 0.3, -d, 0.3)
                if hit[0] is not None:
                    best = max(best, 0.3 - hit[3])
        return best
    topc = cape_idle(fig, cape, gm._object_coords(cape))[:150]
    ra = np.arctan2(topc[:, 0] - neck[0], -(topc[:, 1] - neck[1])) % (2 * math.pi)
    order = np.argsort(ra)
    rr = np.linalg.norm(topc[:, :2] - neck[:2], axis=1)[order]
    for _ in range(5):
        rr = np.maximum(rr, np.maximum(np.roll(rr, 1), np.roll(rr, -1)))
    for _ in range(6):
        rr = 0.25 * np.roll(rr, 1) + 0.5 * rr + 0.25 * np.roll(rr, -1)
    ring = (ra[order], rr)
    rows = 6
    cv = []
    for j in range(rows):
        t = j / (rows - 1)
        for a in th:
            back = 0.5 - 0.5 * math.cos(a)
            end = float(gm.smoothstep(math.radians(COLLAR_OPEN + 25), math.radians(COLLAR_OPEN), min(a, 2 * math.pi - a)))
            hgt = 0.030 + 0.012 * back + 0.002 * math.sin(a * 5.3 + 1.1) + 0.001 * math.sin(a * 13.7) - 0.004 * end
            z = neck[2] - 0.022 + hgt * t
            base = max(env(a, z) + COLLAR_GAP, float(np.interp(a, ring[0], ring[1], period=2 * math.pi)) + COLLAR_OVER)
            r = base + 0.006 * t ** 2 + 0.0012 * math.sin(a * 7 + t * 2) - 0.002 * math.exp(-((t - 0.25) / 0.1) ** 2)
            d = math.cos(a) * np.array([0.0, -1.0]) + math.sin(a) * np.array([1.0, 0.0])
            cv.append([neck[0] + d[0] * r, neck[1] + d[1] * r, z])
    cv = np.array(cv).reshape(rows, -1, 3)[::-1]
    ob = gm.mesh('Collar', gm.grid(cv[..., 0], cv[..., 1], cv[..., 2]), slot('Coat'))
    return ob
