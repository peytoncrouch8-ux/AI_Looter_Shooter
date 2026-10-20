"""Odessa's clothes below the head (Art/Models/Characters/Odessa.py calls these, in order): charcoal trousers tucked into
tall heeled riding boots, black gloves with short flared cuffs and the shirt's white cuffs above them, the gold brocade
waistcoat (a V closing under the cravat, four gold buttons, a watch chain to the pocket on her right), the shirt's front
and its white high collar, the black silk cravat (band, knot, the fall over the waistcoat, a gold stick pin), a slim
hip belt with the card case on her right hip; and bridge(), which spans a garment over the body's hollows. The tailcoat
is odessa_coat.py's.

Builders adapted from Ellis.py's (its trousers, stovepipe boots, gloves and gauntlets, waistcoat, belt): the lessons
there hold here (Docs/Pipeline.md, "Character models")."""
import math

import bpy  # first: bmesh and mathutils come with it
import bmesh
import numpy as np
from mathutils import Matrix, Vector

import looter_garments as gm
import odessa_kit as kit
from odessa_kit import FWD, log, slot

# --------------------------------------------------------------------------------------------- trousers and boots

BOOT_FRONT = 0.50       # the riding boots' top at the front (m: just under the knee) ...
BOOT_BACK = 0.455       # ... and at the back, cut lower behind the knee so it bends
BOOT_EASE = 0.009       # the shaft's room over the leg (fitted riding boots)
BOOT_TAPER = 0.06       # how much the shaft may narrow going down (m per m)
BOOT_WALL = 0.005       # the shaft's leather and lining round the well the trousers tuck into
BOOT_VOXEL = 0.0018     # the voxel size the shaft and the foot fuse at (m)
BOOT_TRIS = 6500        # each detailed boot's triangles
HEEL = 0.046            # the insole's height on the heel (a riding heel)
TROUSER_END = 0.39      # the trousers stop inside the boots


def boot_top(A):
    return (BOOT_FRONT + BOOT_BACK) * 0.5 + (BOOT_FRONT - BOOT_BACK) * 0.5 * np.cos(A)


def bridge(verts, faces, groups, k=0.85, slab=0.01, hang=None, front_y=None, smooth=6):
    """Cloth over the body's hollows: each point pushed out toward the convex hull of its group's slice (1 cm slabs,
    round the slice's own middle) by k of the way, so a garment spans the hollows the skin dips into (between the
    breasts, the navel, the small of the back, behind the knee) instead of following them; with hang (m in per m down),
    the front below a fullness falls in no faster than that (a waistcoat hangs from the bust, not under it). The
    push is smoothed over the mesh. groups: an integer per point (each its own hull; negative: left as it is)."""
    v = np.asarray(verts, dtype=np.float64)
    out = np.zeros(len(v))
    D = np.zeros((len(v), 2))
    for g in np.unique(groups):
        if g < 0:
            continue
        gi = np.nonzero(groups == g)[0]
        z = v[gi, 2]
        for z0 in np.arange(z.min(), z.max() + slab, slab):
            s = gi[np.abs(z - z0) <= slab * 0.6]
            if len(s) < 4:
                continue
            c = v[s, :2].mean(0)
            rel = v[s, :2] - c
            r = np.linalg.norm(rel, axis=1)
            d = rel / np.maximum(r, 1e-9)[:, None]
            try:
                hr = kit.hull_radii(kit.hull_2d(rel), d)
            except Exception:
                continue
            push = np.maximum(hr - r, 0.0) * k
            better = push > out[s]
            out[s[better]] = push[better]
            D[s[better]] = d[better]
    disp = np.c_[D * out[:, None], np.zeros(len(v))]
    if hang is not None and front_y is not None:
        # the front's fall below a fullness: on a grid of angle (round the slice middle) and height
        a = np.arctan2(v[:, 0], -(v[:, 1] - front_y))
        moved = v + disp
        ab = np.round(a / math.radians(4.0)).astype(int)
        zb = np.round(v[:, 2] / 0.01).astype(int)
        r_now = np.linalg.norm(moved[:, :2] - np.array([0.0, front_y]), axis=1)
        best = {}
        for i in range(len(v)):
            if groups[i] < 0:
                continue
            key = (ab[i], zb[i])
            best[key] = max(best.get(key, 0.0), r_now[i])
        need = np.zeros(len(v))
        for i in range(len(v)):
            if abs(a[i]) > math.radians(75) or groups[i] < 0:
                continue
            m = 0.0
            for dz in range(1, 13):
                rr = best.get((ab[i], zb[i] + dz))
                if rr is not None:
                    m = max(m, rr - hang * dz * 0.01)
            need[i] = max(0.0, m - r_now[i])
        dirs = np.c_[gm.unit(np.c_[moved[:, 0], moved[:, 1] - front_y]), np.zeros(len(v))]
        disp += dirs * need[:, None]
    disp = gm.laplacian(disp, faces, smooth)
    return v + disp


def build_trousers(fig):
    """Charcoal wool trousers from the waist into the boots, a little loose down the thigh, spanning the belly's and the
    seat's hollows (bridge), creased at the knee and bunched above the boot tops."""
    z = fig.wv[:, 2]
    keep = (z < fig.waist + 0.025) & (z > TROUSER_END) & (fig.w_arm < 0.2) & (fig.w_hand < 0.1)
    loose = gm.smoothstep(0.95, 0.60, z)
    inside = gm.smoothstep(BOOT_BACK + 0.0, BOOT_BACK - 0.03, z)
    off = (0.007 + 0.007 * loose) * (1.0 - inside) + 0.007 * inside
    piece, used = gm.shell(fig.wv, fig.faces, fig.fuv, fig.face_mask(keep), off, iters=14)
    split = fig.sk['thigh_l'][0][2] - 0.08
    pv = piece.verts
    groups = np.where(pv[:, 2] > split, 0, np.where(pv[:, 0] > 0, 1, 2))
    piece.verts = bridge(pv, piece.faces, groups, k=0.8)
    v = piece.verts
    nrm = gm.vertex_normals(v, piece.faces)
    for s_ in 'lr':
        knee = fig.sk[f'calf_{s_}'][0]
        side = (v[:, 0] * knee[0]) > 0
        dz = v[:, 2] - knee[2]
        behind = gm.smoothstep(knee[1] - 0.02, knee[1] + 0.05, v[:, 1])
        ring = np.sin(dz * 160.0 + v[:, 0] * 40.0) * np.exp(-(dz / 0.06) ** 2) * (0.35 + 0.65 * behind)
        tuck = np.sin(v[:, 2] * 210.0 + np.arctan2(v[:, 1] - knee[1], v[:, 0] - knee[0]) * 3.0) * \
            gm.smoothstep(BOOT_BACK + 0.10, BOOT_BACK + 0.0, v[:, 2]) * gm.smoothstep(BOOT_BACK - 0.03, BOOT_BACK, v[:, 2])
        v[side] += nrm[side] * (0.0035 * ring[side] + 0.003 * tuck[side])[:, None]
    piece.verts = v
    return gm.mesh('Trousers', piece, slot('Trousers'))


def foot_frame(fig, s):
    ankle, ball = fig.sk[f'foot_{s}'][0], fig.sk[f'ball_{s}'][0]
    f = gm.unit(np.array([ball[0] - ankle[0], ball[1] - ankle[1], 0.0]))
    return ankle, f, np.array([-f[1], f[0], 0.0])


def insole(fig, s_):
    """The insole's height along the foot (s_, m ahead of the ankle): HEEL on the heel, sloping down over the arch to
    1.3 cm under the ball and the toes, the toe tip sprung up a little."""
    sb = fig.ball_s
    return np.interp(s_, [-0.2, 0.18 * sb, 0.62 * sb, 1.05 * sb, 1.75 * sb], [HEEL, HEEL, 0.016, 0.013, 0.018])


def boot_shaft(fig, s):
    """A riding boot's shaft (Ellis.py's stovepipe, fitted closer): round the leg (its slice's convex hull, with room),
    narrowing at most BOOT_TAPER going down, its top higher at the front than behind the knee; a well inside."""
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
    zs = np.arange(BOOT_FRONT + 0.02, z_lo - 1e-6, -0.01)
    A = np.linspace(-math.pi, math.pi, 56, endpoint=False)
    D = np.stack(gm.radial(A, FWD), axis=1)
    hulls = [kit.hull_2d(slice_at(z)) for z in zs]
    cen = np.array([h.mean(axis=0) for h in hulls])
    for _ in range(6):
        cen[1:-1] = 0.25 * cen[:-2] + 0.5 * cen[1:-1] + 0.25 * cen[2:]
    fit = np.array([kit.hull_radii(h - c, D) for h, c in zip(hulls, cen)])
    rad = fit + BOOT_EASE
    rad = np.maximum(0.75 * rad + 0.25 * rad.mean(axis=1, keepdims=True), fit + 0.006)
    for i in range(1, len(zs)):
        rad[i] = np.maximum(rad[i], rad[i - 1] - BOOT_TAPER * (zs[i - 1] - zs[i]))
    up = zs[::-1]

    def ring(z, inset=0.0):
        zc = np.clip(z, up[0], up[-1])
        c = np.stack([np.interp(zc, up, cen[::-1, k]) for k in (0, 1)], axis=1)
        r = np.array([np.interp(zc[k], up, rad[::-1, k]) for k in range(len(A))]) - inset
        return np.c_[c + D * r[:, None], z]

    top = boot_top(A)
    bottom = insole(fig, ((ring(np.full(len(A), z_lo)) - ankle) @ f)) + 0.001
    floor = TROUSER_END - 0.03
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
    """The boot's foot on its last (Ellis.py's), slimmer: from the heel to a narrower toe 3 cm past the toes."""
    ankle, f, lat = foot_frame(fig, s)
    sx = 1.0 if s == 'l' else -1.0
    used = fig.b.used
    feet = fig.wv[used][(fig.w_foot[used] > 0.3) & (fig.wv[used][:, 0] * sx > 0)]
    sf, lf, zf = (feet - ankle) @ f, (feet - ankle) @ lat, feet[:, 2]
    s0, s1 = sf.min() - 0.004, sf.max() + 0.03
    u = np.linspace(0.0, 1.0, 46)
    S = s0 + (s1 - s0) * (0.6 * u + 0.4 * (0.5 - 0.5 * np.cos(math.pi * u)))
    lo, hi, top = np.zeros(len(S)), np.zeros(len(S)), np.zeros(len(S))
    for i, q in enumerate(S):
        near = np.abs(sf - q) < 0.01
        if near.any():
            lo[i], hi[i], top[i] = lf[near].min() - 0.008, lf[near].max() + 0.008, zf[near].max() + 0.009
        else:
            lo[i], hi[i], top[i] = lo[i - 1], hi[i - 1], top[i - 1]
    sb = sf[zf < 0.06].max()
    top = np.maximum(top, (insole(fig, S) + 0.040) * gm.smoothstep(sb - 0.10, sb - 0.06, S))
    s_ball = (fig.sk[f'ball_{s}'][0] - ankle) @ f
    for i in range(1, len(S)):
        if S[i] > s_ball:
            give = 0.24 * (S[i] - S[i - 1])                   # a slimmer toe than Ellis's
            lo[i], hi[i] = min(lo[i], lo[i - 1] + give), max(hi[i], hi[i - 1] - give)
    for _ in range(6):
        for a in (lo, hi, top):
            a[1:-1] = 0.25 * a[:-2] + 0.5 * a[1:-1] + 0.25 * a[2:]
    toe = np.clip((S - (s1 - 0.05)) / 0.05, 0.0, 1.0)
    heel = np.clip(((s0 + 0.02) - S) / 0.02, 0.0, 1.0)
    plan = np.clip(1.0 - toe ** 2.2, 0.0, 1.0) ** 0.45 * np.sqrt(np.clip(1.0 - heel ** 2, 0.0, 1.0))
    mid, half = (lo + hi) * 0.5, (hi - lo) * 0.5 * np.maximum(plan, 0.08)
    zb = insole(fig, S)
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


def build_boots(fig):
    """Tall black riding boots: the shaft and the foot fused into one leather surface (a voxel remesh) and the crease
    between them rounded, on a stacked sole with a welt and a tapered riding heel."""
    shafts = [gm.mesh(f'_Shaft_{s}', boot_shaft(fig, s), slot('Boots')) for s in 'lr']
    lasts = [gm.mesh(f'_Last_{s}', boot_last(fig, s), slot('Boots')) for s in 'lr']
    shaft_tree, last_tree = gm.bvh_objects(shafts), gm.bvh_objects(lasts)
    import looter_bake as bk
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
        if p[2] < 0.25:
            v = Vector(tuple(p))
            gap[i] = max(shaft_tree.find_nearest(v)[3], last_tree.find_nearest(v)[3])
    kit.fillet(ob, gm.smoothstep(0.022, 0.004, gap))
    m = ob.modifiers.new('reduce', 'DECIMATE')
    m.ratio = min(1.0, 2 * BOOT_TRIS / max(1, sum(len(p.vertices) - 2 for p in ob.data.polygons)))
    gm.apply_modifiers(ob, keep=())
    ob.data.shade_smooth()
    co = gm._object_coords(ob)
    soles = []
    for s in 'lr':
        sx = 1.0 if s == 'l' else -1.0
        ankle, f, lat = foot_frame(fig, s)
        side = co[co[:, 0] * sx > 0]
        foot_pts = side[side[:, 2] < insole(fig, (side - ankle) @ f) + 0.008]
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
        out += 0.0035
        ring = np.stack([mid[0] + out * np.cos(ang), mid[1] + out * np.sin(ang)], 1)
        s_ring = (np.c_[ring, np.zeros(len(ring))] - ankle) @ f
        top = insole(fig, s_ring) + 0.002
        heel = s_ring < 0.03
        arch = gm.smoothstep(0.025, 0.04, s_ring) * gm.smoothstep(0.62 * fig.ball_s + 0.02, 0.62 * fig.ball_s, s_ring)
        bottom = 0.011 * arch
        # the riding heel tapers: its bottom 6 mm in from its top
        inward = np.where(heel, 0.006, 0.0)
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
            verts.append(np.array([mid[0], mid[1], zc]))
            for i in range(n):
                a_, b_ = row * n + i, row * n + (i + 1) % n
                faces.append([a_, b_, cidx] if row == 0 else [b_, a_, cidx])
                uv += [(0.0, 0.0), (0.01, 0.0), (0.005, 0.01)]
        soles.append(gm.Piece(np.array(verts), faces, uv))
        log(f'boot {s}: toe tip at y {float(foot_pts[:, 1].min()):.3f}, heel at y {float(foot_pts[:, 1].max()):.3f}')
    sole_ob = gm.mesh('Soles', soles[0] + soles[1], slot('Soles'))
    return [ob, sole_ob]


# ------------------------------------------------------------------------------------- gloves, cuffs, the forearm

GLOVE_GIRTH = 1.12      # the gloves' girth over the bare hand's (Ellis's lesson: thin hands read clawed)
GLOVE_CUFF = 0.06       # the glove runs this far up the forearm inside its cuff (no wrist in the game mesh)
CUFF_TOP = 0.068        # the glove's flared cuff from the wrist up to here (m)
CUFF_FLARE = (0.010, 0.008)
CUFF_ROOM = 0.004       # the cuff's room over the glove
CUFF_LIP = 0.003        # its mouth's lip rolled out
SHIRT_CUFF = (0.055, 0.098)     # the shirt's white cuff: from, to (m up the forearm from the wrist), over the glove's cuff
SLEEVE_END = 0.083      # the coat's sleeve stops here, over the shirt's cuff: 2.8 cm of white shows
COAT_CUFF = (0.083, 0.135)      # the sleeve's darker turned-back cuff


def thicken_hand(fig, verts, used):
    W = fig.Wv[used]
    out = np.zeros_like(verts)
    for j, bone in enumerate(fig.names):
        w = W[:, j]
        if not (w > 1e-4).any():
            continue
        h, t = fig.sk[bone][0], fig.sk[bone][1]
        d = gm.unit(t - h)
        q = verts - h
        out += w[:, None] * (q - np.outer(q @ d, d))
    return verts + (GLOVE_GIRTH - 1.0) * out


def forearm_share(fig, s, P):
    e, w = fig.sk[f'lowerarm_{s}'][0], fig.sk[f'lowerarm_{s}'][1]
    length = np.linalg.norm(w - e)
    return ((P - e) @ ((w - e) / length)) / length


def forearm_frame(fig, s):
    e, w = fig.sk[f'lowerarm_{s}'][0], fig.sk[f'lowerarm_{s}'][1]
    axis = gm.unit(w - e)
    a = gm.unit(np.cross(-axis, [0.0, -1.0, 0.0]))
    b = np.cross(-axis, a)
    return e, w, axis, a, b


def ellipse_round(pts, a, b):
    """The smallest ellipse (axes along a and b) round points (relative to its centre): (ra, rb)."""
    x, y = np.abs(pts @ a), np.abs(pts @ b)
    best = None
    for ra in np.linspace(x.max() * 1.001, x.max() * 1.8, 40):
        rb = float((y / np.sqrt(np.maximum(1.0 - (x / ra) ** 2, 1e-6))).max())
        if best is None or ra * rb < best[0] * best[1]:
            best = (ra, rb)
    return best


def build_gloves(fig):
    """Black gloves: the hands' own shell and GLOVE_CUFF up the forearm, filled out (thicken_hand), and a short flared
    cuff over each wrist, its mouth fitted round the glove. Returns the parts and each side's cuff top (radii)."""
    keep = fig.w_hand > 0.45
    for s in 'lr':
        e, w = fig.sk[f'lowerarm_{s}'][0], fig.sk[f'lowerarm_{s}'][1]
        fore = fig.Wv[:, fig.col[f'lowerarm_{s}']] + fig.Wv[:, fig.col[f'hand_{s}']]
        t = forearm_share(fig, s, fig.wv)
        keep |= (fore > 0.5) & (t > 1.0 - GLOVE_CUFF / np.linalg.norm(w - e)) & (t < 1.2)
    piece, used = gm.shell(fig.wv, fig.faces, fig.fuv, fig.face_mask(keep), 0.0022, iters=1)
    piece.verts = thicken_hand(fig, piece.verts, used)
    glove = gm.mesh('Gloves', piece, slot('Gloves'))
    gv = piece.verts
    parts = [glove]
    tops = {}
    for s in 'lr':
        e, w, axis, a, b = forearm_frame(fig, s)
        steps = np.linspace(0, 1, 8)
        path = np.array([w + axis * (-0.004 - (CUFF_TOP - 0.004) * t) for t in steps])
        mine = gv[((gv - e) @ axis > 0.0) & (np.linalg.norm(np.cross(gv - e, axis), axis=1) < 0.08)
                  & ((gv[:, 0] > 0) == (s == 'l'))]
        ang = np.linspace(0, 2 * math.pi, 28, endpoint=False)
        axes = np.zeros((len(steps), 2))
        for i, p in enumerate(path):
            q = mine - p
            sl = q[np.abs(q @ axis) < 0.008]
            axes[i] = ellipse_round(sl, a, b) if len(sl) else (axes[i - 1] if i else (0.032, 0.026))
        axes = np.maximum(axes, axes[:1])
        fit = 1.0 / np.sqrt((np.cos(ang)[None] / axes[:, :1]) ** 2 + (np.sin(ang)[None] / axes[:, 1:]) ** 2)
        flare = np.outer(steps ** 1.4, CUFF_FLARE[0] * np.cos(ang) ** 2 + CUFF_FLARE[1] * np.sin(ang) ** 2)
        r = fit + CUFF_ROOM + flare
        r[0] += CUFF_LIP
        pc = gm.tube(path, np.ones((len(steps), 2)), n=len(ang), up=(0.0, -1.0, 0.0), shape=lambda i, _a: r[i])
        cuff = gm.mesh(f'Gauntlet_{s}', pc, slot('Gloves'))
        parts.append(cuff)
        tops[s] = (axes[-1] + CUFF_ROOM + np.array(CUFF_FLARE))
        log(f'glove cuff {s}: mouth {r[0].min() * 100:.1f}-{r[0].max() * 100:.1f} cm round the glove, top '
            f'{tops[s][0] * 100:.1f} x {tops[s][1] * 100:.1f} cm')
    return parts, tops


def build_shirt_cuffs(fig, tops):
    """The shirt's white linen cuffs, straight bands over the top of the glove's cuff (SHIRT_CUFF), a little room."""
    parts = []
    for s in 'lr':
        e, w, axis, a, b = forearm_frame(fig, s)
        ra, rb = tops[s] + 0.0035
        steps = np.linspace(0, 1, 6)
        path = np.array([w - axis * (SHIRT_CUFF[0] + (SHIRT_CUFF[1] - SHIRT_CUFF[0]) * t) for t in steps])
        ang = np.linspace(0, 2 * math.pi, 28, endpoint=False)
        r = 1.0 / np.sqrt((np.cos(ang) / ra) ** 2 + (np.sin(ang) / rb) ** 2)
        R = np.repeat(r[None], len(steps), 0)
        R[0] += 0.0015                                    # its edge turned out a little
        pc = gm.tube(path, np.ones((len(steps), 2)), n=len(ang), up=(0.0, -1.0, 0.0), shape=lambda i, _a: R[i])
        parts.append(gm.mesh(f'ShirtCuff_{s}', pc, slot('Shirt')))
        kit.THICK[f'ShirtCuff_{s}'] = 0.002
    return parts


# ------------------------------------------------------------------------------------- waistcoat, shirt, cravat

VEST_VEE = 0.036        # the waistcoat's V: half its width at the neck (m) ...
VEST_DEPTH = 0.125      # ... closing this far below it
BUTTON0 = 0.112         # its first button below the neck, and the step down
BUTTON_STEP = 0.037


def build_vest(fig):
    """The gold brocade waistcoat (the shirt's front in its V): the torso pushed out and smoothed as cloth, a V closing
    under the cravat, a clean hem level round the back with two points at the front, four gold buttons."""
    z = fig.wv[:, 2]
    nz = fig.neck[2]
    torso = (fig.w_arm < 0.25) & (fig.w_hand < 0.05) & (fig.w_head < 0.25)
    keep = torso & (z > fig.waist - 0.05) & (z < nz - 0.004)
    vest, _ = gm.shell(fig.wv, fig.faces, fig.fuv, fig.face_mask(keep), 0.0085, iters=14)
    # smoothed hard first (the bust's small bumps out of the cloth: the game mesh has no body under it), then over
    # the hollows
    vest.verts = gm.laplacian(vest.verts, vest.faces, 40)
    vest.verts = bridge(vest.verts, vest.faces, np.zeros(len(vest.verts), dtype=int), k=0.9, hang=0.9,
                        front_y=fig.centre[1])
    vv = vest.verts
    hem = gm.boundary(vest.faces, len(vv)) & (vv[:, 2] < fig.waist + 0.0)
    fr = vv[:, 1] < fig.centre[1] - 0.02
    vv[hem, 2] = fig.waist - 0.035 - 0.055 * fr[hem] * np.exp(-((np.abs(vv[hem, 0]) - 0.045) / 0.04) ** 2)
    vest.verts = gm.laplacian(vv, vest.faces, 4)
    ob = gm.mesh('Vest', vest, slot('Brocade'))
    cut_vee(ob, fig, VEST_VEE, VEST_DEPTH)
    kit.THICK['Vest'] = 0.002              # its hem and points show under the cutaway: an inside, not a hole
    tree = gm.bvh_objects([ob])
    buttons = []
    for k in range(4):
        zz = nz - BUTTON0 - k * BUTTON_STEP
        hit = tree.ray_cast(Vector((0.004, -0.5, zz)), Vector((0, 1, 0)), 1.0)
        if hit[0] is None:
            continue
        p, n = np.array(hit[0]), np.array(hit[1])
        b = kit.ellipsoid('VestButton', p + n * 0.0028, (0.0050, 0.0050, 0.0026), n, 12, 8, slot('Gold'))
        buttons.append(b)
    fig.vest_buttons = [np.array(b.data.vertices[0].co) for b in buttons]
    return ob, buttons


def cut_vee(ob, fig, vee, depth):
    """The waistcoat's V cut clean (planes, not the body's faces): two vertical planes from its point VEST_DEPTH under
    the neck up to vee either side at the neck, splitting the front's faces; the faces between them go."""
    nz = fig.neck[2]
    bm = bmesh.new()
    bm.from_mesh(ob.data)
    for side in (1.0, -1.0):
        fr = [f for f in bm.faces if f.calc_center_median().y < fig.centre[1] - 0.01 and
              f.calc_center_median().z > nz - depth - 0.03]
        if not fr:
            continue
        geom = list({v for f in fr for v in f.verts}) + list({e for f in fr for e in f.edges}) + fr
        d = Vector((side * vee, 0.0, depth)).normalized()
        n = d.cross(Vector((0.0, 1.0, 0.0)))
        bmesh.ops.bisect_plane(bm, geom=geom, plane_co=Vector((0.0, 0.0, nz - depth)), plane_no=n)
    inside = lambda c: (c.y < fig.centre[1] - 0.01 and c.z > nz - depth and
                        abs(c.x) < vee * (1.0 - (nz - c.z) / depth) - 1e-5)
    bmesh.ops.delete(bm, geom=[f for f in bm.faces if inside(f.calc_center_median())], context='FACES')
    bm.to_mesh(ob.data)
    bm.free()
    ob.data.update()


def build_shirt(fig):
    """The shirt's front where it shows, in the waistcoat's V either side of the cravat's fall, spanning the hollow
    between the breasts."""
    z = fig.wv[:, 2]
    torso = (fig.w_arm < 0.25) & (fig.w_hand < 0.05) & (fig.w_head < 0.25)
    front = fig.wv[:, 1] < fig.centre[1] - 0.02
    keep = torso & front & (np.abs(fig.wv[:, 0]) < 0.08) & (z > fig.neck[2] - VEST_DEPTH - 0.04) & (z < fig.neck[2] + 0.01)
    shirt, _ = gm.shell(fig.wv, fig.faces, fig.fuv, fig.face_mask(keep), 0.0040, iters=6)
    shirt.verts = gm.laplacian(shirt.verts, shirt.faces, 40)
    shirt.verts = bridge(shirt.verts, shirt.faces, np.zeros(len(shirt.verts), dtype=int), k=0.9)
    return gm.mesh('Shirt', shirt, slot('Shirt'))


def neck_tree(fig):
    """The neck's skin alone (not the jaw's underside, the chin, the skull): a BVH, world space."""
    if getattr(fig, '_neck_tree', None) is None:
        from mathutils.bvhtree import BVHTree
        neck = fig.Wv[:, fig.col['neck_01']] + fig.Wv[:, fig.col['neck_02']]
        keep = (neck > 0.25) & (fig.Wv[:, fig.col['head']] < 0.5)
        faces = [list(f) for f in fig.faces if keep[f].all()]
        fig._neck_tree = BVHTree.FromPolygons([tuple(p) for p in fig.wv], faces)
    return fig._neck_tree


def neck_ring(fig, z, n=72):
    """The neck's section at height z: its centre (on the neck's axis, neck_01 to the head's joint) and radius at n
    angles (0 the front, + her left), from the neck's skin (where it misses, from its neighbours)."""
    tree = neck_tree(fig)
    a0, b0 = fig.sk['neck_01'][0], fig.sk['head'][0]
    t = float(np.clip((z - a0[2]) / (b0[2] - a0[2]), 0.0, 1.0))
    c = a0 + (b0 - a0) * t
    ang = np.linspace(0, 2 * math.pi, n, endpoint=False)
    r = np.zeros(n)
    for i, a in enumerate(ang):
        d = Vector((math.sin(a), -math.cos(a), 0.0))
        hit = tree.ray_cast(Vector((c[0], c[1], z)) + d * 0.2, -d, 0.2)
        r[i] = 0.2 - hit[3] if hit[0] is not None else np.nan
    good = ~np.isnan(r)
    r = np.interp(ang, ang[good], r[good], period=2 * math.pi) if good.any() else np.full(n, 0.05)
    return c[:2], ang, r


COLLAR_OPEN = 9.0       # degrees either side of the front the shirt's collar leaves open (a finger's width)
COLLAR_ROOM = 0.006     # its room over the neck
COLLAR_H = (0.034, 0.066)   # its height at the throat and at the back (m over the neck's foot): the chin clears its front
                            # with the head bowed 25 degrees to a sight (at 4.4 cm it went 4.5 cm into it)


def build_collar(fig):
    """The shirt's starched white high collar round the neck, open a finger's width at the throat, lower there than
    behind (the chin clears it, the head bowed to a sight), its top edge rolled a little outward."""
    nz = fig.neck[2]
    th = np.linspace(math.radians(COLLAR_OPEN), math.radians(360.0 - COLLAR_OPEN), 84)
    rows = 8
    base = nz - 0.006
    tops = [base + COLLAR_H[0] + (COLLAR_H[1] - COLLAR_H[0]) * (0.5 - 0.5 * math.cos(a)) for a in th]
    # a starched band stands straight: its section the neck's a little up it (the neck's foot flares into the
    # shoulders, and a collar following it flared out like a dish), never inside the neck lower down
    _c, ang0, ref = neck_ring(fig, nz + 0.025)
    for _ in range(4):
        ref = 0.25 * np.roll(ref, 1) + 0.5 * ref + 0.25 * np.roll(ref, -1)
    rings = {}
    cv = []
    for j in range(rows):
        t = j / (rows - 1)
        for a, top in zip(th, tops):
            zz = base + (top - base) * t
            key = round(zz, 3)
            if key not in rings:
                rings[key] = neck_ring(fig, zz)
            c, ang, rr = rings[key]
            r = max(float(np.interp(a, ang0, ref, period=2 * math.pi)) + COLLAR_ROOM,
                    float(np.interp(a, ang, rr, period=2 * math.pi)) + 0.002) + 0.002 * t ** 2
            cv.append([c[0] + math.sin(a) * r, c[1] - math.cos(a) * r, zz])
    cv = np.array(cv).reshape(rows, -1, 3)
    # rows from the top down so the faces look out; smoothed round (the neck's own bumps out of a starched collar)
    for j in range(rows):
        for _ in range(4):
            cv[j, 1:-1] = 0.25 * cv[j, :-2] + 0.5 * cv[j, 1:-1] + 0.25 * cv[j, 2:]
    cv = cv[::-1]
    ob = gm.mesh('Collar', gm.grid(cv[..., 0], cv[..., 1], cv[..., 2]), slot('Shirt'))
    kit.faces_out(ob, fig.neck, (0.0, 0.0, 1.0))
    kit.THICK['Collar'] = 0.0025
    fig.collar_r = lambda a, z: _collar_radius(fig, cv[::-1], th, a, z)
    fig.collar_top = lambda a: float(np.interp(a, th, tops))
    fig.collar_base = base
    return ob


def _collar_radius(fig, wall, th, a, z):
    k = int(np.argmin(np.abs((th - a + math.pi) % (2 * math.pi) - math.pi)))
    zs = wall[:, k, 2]
    c = fig.neck[:2]
    rs = np.linalg.norm(wall[:, k, :2] - c, axis=1)
    return float(np.interp(z, zs, rs))


def build_cravat(fig, collar, under):
    """The black silk cravat: a band round the collar's foot, a knot at the throat, its puffed fall down over the
    waistcoat's V, a gold stick pin in the fall."""
    nz = fig.neck[2]
    ctree = gm.bvh_objects([collar])
    c = fig.neck[:2]
    th = np.linspace(0, 2 * math.pi, 72, endpoint=False)
    rows = []
    for zz in np.linspace(nz - 0.003, nz + 0.019, 4):
        rr = np.full(len(th), np.nan)
        for i, a in enumerate(th):
            d = Vector((math.sin(a), -math.cos(a), 0.0))
            # the collar's outermost within 4 mm of the row (its foot flares: between rows it stood out 5 mm)
            for dz in (-0.004, 0.0, 0.004):
                hit = ctree.ray_cast(Vector((c[0], c[1], zz + dz)) + d * 0.25, -d, 0.25)
                if hit[0] is not None and not 0.25 - hit[3] < rr[i]:
                    rr[i] = 0.25 - hit[3]
        good = ~np.isnan(rr)                          # (across the collar's open front: from either side)
        rr = np.interp(th, th[good], rr[good], period=2 * math.pi)
        # the chord across the opening lay inside the collar's two ends: they stood out through the band
        front = np.abs((th + math.pi) % (2 * math.pi) - math.pi)
        rr += 0.004 * gm.smoothstep(math.radians(COLLAR_OPEN + 10.0), math.radians(COLLAR_OPEN), front)
        rows.append([[c[0] + math.sin(a) * (r + 0.0035), c[1] - math.cos(a) * (r + 0.0035), zz] for a, r in zip(th, rr)])
    R = np.array(rows[::-1])
    for j in range(len(R)):
        for _ in range(3):
            R[j] = 0.25 * np.roll(R[j], 1, 0) + 0.5 * R[j] + 0.25 * np.roll(R[j], -1, 0)
    band = gm.mesh('CravatBand', gm.grid(R[..., 0], R[..., 1], R[..., 2], wrap=True), slot('Silk'))
    kit.faces_out(band, fig.neck, (0.0, 0.0, 1.0))
    kit.THICK['CravatBand'] = 0.0015
    front = R[:, 0].mean(0)
    knot_c = front + np.array([0.0, -0.010, -0.002])
    s = fig.b.scale
    knot = kit.ellipsoid('CravatKnot', knot_c, (0.013 * s, 0.0095 * s, 0.0075 * s), (0, -1, 0), 16, 10, slot('Silk'))
    # the fall: a strip over the shirt and the waistcoat's V from under the knot, widening, puffed in the middle
    tree = gm.bvh_objects(under + [band])
    rows = []
    for t in np.linspace(0, 1, 16):
        w = 0.009 + 0.016 * t ** 0.6
        zz = knot_c[2] - 0.006 - t * 0.095
        row = []
        for i in range(7):
            u = i / 6 - 0.5
            x = knot_c[0] + u * 2 * w
            hit = tree.ray_cast(Vector((x, -0.6, zz)), Vector((0, 1, 0)), 1.5)
            p = np.array(hit[0]) if hit[0] is not None else np.array([x, knot_c[1], zz])
            n = np.array(hit[1]) if hit[0] is not None else np.array([0.0, -1.0, 0.0])
            puff = (0.0025 + 0.0035 * t) * math.sin(math.pi * i / 6) + 0.0012 * math.sin(math.pi * i / 2) * t
            row.append(p + n * (0.0025 + puff))
        rows.append(row)
    R = np.array(rows)
    fall = gm.mesh('CravatFall', gm.grid(R[..., 0], R[..., 1], R[..., 2]), slot('Silk'))
    kit.THICK['CravatFall'] = 0.0015
    pin_at = R[6, 3]
    pin_n = gm.unit(np.cross(R[7, 3] - R[5, 3], R[6, 4] - R[6, 2]))
    if pin_n[1] > 0:
        pin_n = -pin_n
    pin = kit.ellipsoid('StickPin', pin_at + pin_n * 0.003, (0.0034, 0.0034, 0.0034), pin_n, 12, 8, slot('Gold'))
    return [band, knot, fall, pin]


def build_watch_chain(fig, vest):
    """A gold watch chain from the waistcoat's first button across to the pocket on her right, hanging in a shallow
    curve: oval links turned alternately flat and edge-up (the detail the bake reads), and a plain tube for the game
    mesh (the links on it baked)."""
    tree = gm.bvh_objects([vest])
    nz = fig.neck[2]
    a = np.array([0.004, nz - BUTTON0])
    b = np.array([-0.075, nz - BUTTON0 - 0.012])
    # Only hits on the waistcoat's front count: by the first button the two fronts part, and two rays slipped through
    # to the back panel 20 cm behind (2026-10-20): the game tube ran through the body there, and those faces baked
    # black (no link within the bake's reach). A missed sample takes its place between its neighbours on the front.
    hits = []
    for t in np.linspace(0, 1, 40):
        x, zz = a + (b - a) * t - np.array([0.0, 0.032 * math.sin(math.pi * t)])
        hit = tree.ray_cast(Vector((x, -0.6, zz)), Vector((0, 1, 0)), 1.5)
        front = hit[0] is not None and hit[1].y < -0.3
        hits.append((np.array(hit[0]), np.array(hit[1])) if front else None)
    good = [k for k, h in enumerate(hits) if h is not None]
    pts = []
    for k, h in enumerate(hits):
        if h is None:
            lo = max((g for g in good if g < k), default=None)
            hi = min((g for g in good if g > k), default=None)
            if lo is None or hi is None:
                continue                    # past the last hit on the front: the chain ends there
            w = (k - lo) / (hi - lo)
            h = (hits[lo][0] * (1.0 - w) + hits[hi][0] * w, gm.unit(hits[lo][1] * (1.0 - w) + hits[hi][1] * w))
        pts.append(h)
    bm = bmesh.new()
    path = []
    for k, (p, nrm) in enumerate(pts):
        tan = gm.unit(pts[min(k + 1, len(pts) - 1)][0] - pts[max(k - 1, 0)][0])
        side = nrm if k % 2 else np.cross(tan, nrm)
        side = gm.unit(side - tan * (side @ tan))
        m = Matrix((Vector(tan), Vector(side), Vector(np.cross(tan, side)))).transposed().to_4x4()
        m.translation = Vector(tuple(p + nrm * 0.0035))
        R, r = 0.0026, 0.0007
        vs = []
        for u in np.linspace(0, 2 * math.pi, 10, endpoint=False):
            for v in np.linspace(0, 2 * math.pi, 5, endpoint=False):
                q = Vector(((R + r * math.cos(v)) * math.cos(u) * 1.3, (R + r * math.cos(v)) * math.sin(u), r * math.sin(v)))
                vs.append(bm.verts.new(m @ q))
        for i in range(10):
            for j in range(5):
                i2, j2 = (i + 1) % 10, (j + 1) % 5
                bm.faces.new((vs[i * 5 + j], vs[i2 * 5 + j], vs[i2 * 5 + j2], vs[i * 5 + j2]))
        path.append(p + nrm * 0.0035)
    me = bpy.data.meshes.new('WatchChain')
    bm.to_mesh(me)
    bm.free()
    me.shade_smooth()
    links = bpy.data.objects.new('WatchChain', me)
    bpy.context.scene.collection.objects.link(links)
    links.data.materials.append(slot('Gold'))
    path = np.array(path)[::2]
    low = gm.mesh('_WatchChainLow', gm.tube(path, np.full(len(path), 0.0026), n=6), slot('Gold'))
    kit.CUSTOM_LOW['WatchChain'] = low
    fob = kit.ellipsoid('WatchFob', pts[-1][0] + pts[-1][1] * 0.004 + np.array([0.0, 0.0, -0.006]),
                        (0.006, 0.006, 0.0035), pts[-1][1], 12, 8, slot('Gold'))
    return [links, fob]


# ------------------------------------------------------------------------------------- the hip belt, the card case

def belt_height(fig, th):
    """The hip belt's middle round the hips (0 the front, + her left): slung a little lower at the front."""
    return fig.hip + 0.012 - 0.012 * np.maximum(0.0, np.cos(th)) ** 2


def build_belt(fig, under_obs):
    """A slim black belt slung at the hips over the trousers (under the tails at the back), a gold buckle in front; the
    oxblood card case on it at her right hip, a gold clasp."""
    tree = gm.bvh_objects(under_obs)
    n = 120
    th = np.linspace(-math.pi, math.pi, n, endpoint=False)
    dx, dy = gm.radial(th, FWD)
    path, nrm = [], []
    for i in range(n):
        z = float(belt_height(fig, th[i]))
        d = Vector((dx[i], dy[i], 0.0))
        o = Vector((fig.centre[0], fig.centre[1], z)) + d * 0.6
        hit = tree.ray_cast(o, -d, 1.0)
        p = np.array(hit[0]) if hit[0] is not None else np.array(o - d * 0.45)
        nn = np.array(hit[1]) if hit[0] is not None else np.array(d)
        nn[2] *= 0.5
        path.append(p + gm.unit(nn) * 0.005)
        nrm.append(gm.unit(nn))
    path, nrm = np.array(path), np.array(nrm)
    path = gm.laplacian(path, [[i, (i + 1) % n] for i in range(n)], 3)
    up = gm.unit(np.array([0.0, 0.0, 1.0]) - nrm * nrm[:, 2:3])
    rows = [path + up * 0.016, path + nrm * 0.0015, path - up * 0.016]
    P = np.array(rows)
    belt = gm.mesh('Belt', gm.grid(P[..., 0], P[..., 1], P[..., 2], wrap=True), slot('Belt'))
    kit.faces_out(belt, (fig.centre[0], fig.centre[1], 0.0), (0.0, 0.0, 1.0))
    kit.THICK['Belt'] = 0.004
    parts = [belt]
    i0 = int(np.argmin(np.abs(th)))
    f = path[i0] + nrm[i0] * 0.005
    side = gm.unit(np.cross(up[i0], nrm[i0]))
    frame = None
    for (a, b, w) in (((-1, -1), (1, -1), 0.005), ((1, -1), (1, 1), 0.005), ((1, 1), (-1, 1), 0.005),
                      ((-1, 1), (-1, -1), 0.005), ((0.0, -1), (0.0, 1), 0.003)):
        p0 = f + side * a[0] * 0.022 + up[i0] * a[1] * 0.019
        p1 = f + side * b[0] * 0.022 + up[i0] * b[1] * 0.019
        bar = gm.tube(np.array([p0, p1]), np.array([w * 0.5, w * 0.5]), n=8, up=nrm[i0], cap0=True, cap1=True)
        frame = bar if frame is None else frame + bar
    parts.append(gm.mesh('Buckle', frame, slot('Gold')))
    fig.belt = (th, path, nrm, up)
    parts += build_card_case(fig)
    return parts


CASE_AT = -1.35         # the card case round the hips: radians from the front, her right (at -0.95, over the hip
                        # crease, a crouch folded the trousers 5 cm over it; at -1.15 still 2.3 cm)
CASE_SIZE = (0.075, 0.100, 0.024)


def belt_frame(fig, angle):
    th, path, nrm, up = fig.belt
    i = int(np.argmin(np.abs((th - angle + math.pi) % (2 * math.pi) - math.pi)))
    tangent = gm.unit(path[(i + 1) % len(th)] - path[i - 1])
    return path[i], nrm[i], up[i], tangent


def rounded_box(name, centre, axes, half, material, n=3, p=4.0):
    bm = bmesh.new()
    bmesh.ops.create_cube(bm, size=2.0)
    bmesh.ops.subdivide_edges(bm, edges=bm.edges[:], cuts=n, use_grid_fill=True)
    for v in bm.verts:
        q = np.array(v.co)
        q = q / max(np.sum(np.abs(q) ** p) ** (1.0 / p), 1e-9)
        w = centre + axes[0] * q[0] * half[0] + axes[1] * q[1] * half[1] + axes[2] * q[2] * half[2]
        v.co = Vector(tuple(w))
    me = bpy.data.meshes.new(name)
    bm.to_mesh(me)
    bm.free()
    me.shade_smooth()
    ob = bpy.data.objects.new(name, me)
    bpy.context.scene.collection.objects.link(ob)
    ob.data.materials.append(material)
    return ob


def build_card_case(fig):
    """The card case on the belt at her right hip: an oxblood leather case hung by a loop over the belt, its lid's flap
    with a gold clasp."""
    p, n, up, t = belt_frame(fig, CASE_AT)
    across = gm.unit(np.cross(up, n))
    w, h, d = CASE_SIZE
    c = p - up * (0.016 + h * 0.5 - 0.006) + n * (d * 0.5 + 0.004)
    case = rounded_box('CardCase', c, (across, up, n), (w * 0.5, h * 0.5, d * 0.5), slot('Case'))
    loop = rounded_box('CardCaseLoop', p + n * 0.006 - up * 0.004, (across, up, n), (0.016, 0.026, 0.0035), slot('Case'))
    flap_c = c + up * (h * 0.5 - 0.02) + n * (d * 0.5 + 0.0015)
    flap = rounded_box('CardCaseFlap', flap_c, (across, up, n), (w * 0.5 - 0.002, 0.022, 0.002), slot('Case'), p=3.0)
    clasp = rounded_box('CardCaseClasp', flap_c - up * 0.016 + n * 0.003, (across, up, n), (0.009, 0.006, 0.0025),
                        slot('Gold'))
    fig.case = (c, n, up)
    return [case, loop, flap, clasp]


