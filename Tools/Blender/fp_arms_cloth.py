"""The heroes' first-person sleeves and cuffs (Tools/Blender/fp_arms_heroes.py), lofted the way FPArms.py lofts Ellis's
duster sleeve: rings along a path from the sleeve's end on the forearm, round the elbow, 25 cm up the upper arm (where
the code floats the shoulder, the cut end stays under the frame), each ring an ellipse with the cloth's folds (FPArms'
sleeve_fold: rings bunched above the cuff, spirals down the forearm, crescents in the crook of the elbow).

Over the sleeve's end a turned-back cuff (the sleeve's cloth folded back up the forearm: a rounded fold at the hand end,
its top edge lying on the sleeve), fitted round the sleeve turn by turn with a little room, with thickness (inside
faces run back up under it, so its mouth never shows through). Crane's are torn (the top edge and the fold notched);
Pike's carry a buttoned tab. Odessa's white shirt cuff is a plain band over her glove's cuff (ShirtCuff).

Turns (theta) are the hand frame's: 0 over the back of the hand, +pi/2 the thumb's side, pi the palm's (the underarm
seam). Lengths along the sleeve (v) in cm from its end.
"""
import math

import bpy
import numpy as np

from fp_arms_base import A, C, ellipse_r, log, smoothstep, unit, v3
from fp_arms_hands import mesh_object

sleeve_fold = A['sleeve_fold']
PLAIN_STEP = 1.55                  # rings on the forearm between the cuff's folds and the elbow's (cm apart)
ELBOW_STEP = (5.5, 0.72)           # the elbow's folds: how far either side of it, rings how far apart (cm)


class SleevePath:
    """The sleeve's centre line (Blender, m) from its end on the forearm (x_start cm up the hand frame's x, a little
    under the bones' line) round the elbow to the cut on the upper arm, by arc length (cm), with a parallel-transported
    frame whose (zz, yy) start as the hand frame's back and thumb directions (FPArms.py's SleevePath from another
    start)."""

    def __init__(self, rest, x_start, z_off=-0.25, cut=0.25):
        R = rest.R
        start = rest.to_world(v3(x_start, 0.0, z_off) * C)
        E, S = rest.elbow, rest.shoulder
        up_dir = unit(S - E)
        p1 = E + unit(start - E) * 0.065
        p2 = E + up_dir * 0.06
        end = E + up_dir * cut
        n0 = max(12, int(np.linalg.norm(p1 - start) / 0.004))
        pts = [start + (p1 - start) * t for t in np.linspace(0.0, 1.0, n0)]
        for t in np.linspace(0.0, 1.0, 30)[1:]:
            pts.append((1 - t) ** 2 * p1 + 2 * (1 - t) * t * E + t * t * p2)
        pts += [p2 + (end - p2) * t for t in np.linspace(0.0, 1.0, 30)[1:]]
        P = np.array(pts)
        seg = np.linalg.norm(np.diff(P, axis=0), axis=1)
        self.s = np.concatenate([[0.0], np.cumsum(seg)]) / C
        self.P = P
        T = unit(np.gradient(P, axis=0))
        zz, yy = [R[:, 2]], [R[:, 1]]
        for i in range(1, len(P)):
            z = unit(zz[-1] - T[i] * (T[i] @ zz[-1]))
            zz.append(z)
            yy.append(unit(np.cross(T[i], z)))
        self.T, self.zz, self.yy = T, np.array(zz), np.array(yy)
        self.elbow_v = float(self.s[int(np.argmin(np.linalg.norm(P - E, axis=1)))])
        self.length = float(self.s[-1])
        inner = unit(unit(rest.wrist - E) + unit(S - E))
        k = int(np.argmin(np.abs(self.s - self.elbow_v)))
        self.inner_theta = math.atan2(inner @ self.yy[k], inner @ self.zz[k])
        self.x_start = x_start

    def at(self, v):
        v = np.atleast_1d(np.asarray(v, dtype=np.float64))
        i = np.clip(np.searchsorted(self.s, v) - 1, 0, len(self.s) - 2)
        t = ((v - self.s[i]) / np.maximum(self.s[i + 1] - self.s[i], 1e-9))[:, None]
        lerp = lambda Arr: Arr[i] * (1 - t) + Arr[i + 1] * t
        return lerp(self.P), unit(lerp(self.zz)), unit(lerp(self.yy)), unit(lerp(self.T))

    def points(self, theta, v, ry, rz):
        """Points (Blender, m) at turns theta (array) on the ring at v with half sizes ry, rz (cm, arrays or numbers)."""
        P, zz, yy, _ = self.at(v)
        P, zz, yy = P[0], zz[0], yy[0]
        ry = np.broadcast_to(ry, theta.shape)
        rz = np.broadcast_to(rz, theta.shape)
        return P[None, :] + (np.outer(ry * np.sin(theta), yy) + np.outer(rz * np.cos(theta), zz)) * C


class SleeveSpec:
    """A sleeve: x_start (its end, hand frame cm), end (ry, rz) its gathered end's half sizes (under the cuff), knots
    [(v, ry, rz)] its free half sizes (v may be 'elbow+3' style offsets: callables of the path), folds (a share of
    FPArms' fold depth), rim (where the folds bunch: the cuff's top, cm from the end)."""

    def __init__(self, x_start, end, knots, folds=1.0, rim=6.0, segs=18):
        self.x_start, self.end, self.knots, self.folds, self.rim, self.segs = x_start, end, knots, folds, rim, segs


def sleeve_radii(spec, path, v):
    knots = [(0.0, spec.end[0], spec.end[1])] + [(k(path) if callable(k) else k, ry, rz) for k, ry, rz in spec.knots]
    knots.sort(key=lambda t: t[0])
    vs = [k[0] for k in knots] + [path.length + 1.0]
    rys = [k[1] for k in knots] + [knots[-1][1]]
    rzs = [k[2] for k in knots] + [knots[-1][2]]
    return np.interp(v, vs, rys), np.interp(v, vs, rzs)


def sleeve_ring(spec, path, theta, v):
    """The sleeve's half sizes at turns theta and v with its folds (none under the cuff), and the fold height."""
    ry, rz = sleeve_radii(spec, path, v)
    out = smoothstep(spec.rim - 0.6, spec.rim + 1.0, v)
    fold = sleeve_fold(theta, v, spec.rim, path.elbow_v, path.inner_theta) * spec.folds * out
    return ry + fold, rz + fold, fold


def sleeve_rows(spec, path):
    """The rings' arc lengths: close where the cloth folds; far apart on the upper arm (it changes slowly and stays under
    the frame), which pays for the rings of FPArms.py's longer forearm (FOREARM_STRETCH) at the same triangle count."""
    vs = [0.0]
    while vs[-1] < path.length:
        v = vs[-1]
        step = PLAIN_STEP
        if v < spec.rim - 0.5:
            step = 1.2
        elif v < spec.rim + 7.0:
            step = 0.62
        elif abs(v - path.elbow_v) < ELBOW_STEP[0]:
            step = ELBOW_STEP[1]
        elif v > path.elbow_v + ELBOW_STEP[0]:
            step = A['UPPER_STEP']
        vs.append(min(v + step, path.length))
        if path.length - vs[-1] < 0.5:
            vs[-1] = path.length
    return np.array(vs)


def seams_attribute(obj, red, green):
    """The 'Seams' colour the looks' construction() reads (looter_looks): R near an open edge, G near a sewn seam (1 at
    it, 0 from 10 cm away), per vertex."""
    me = obj.data
    attr = me.color_attributes.new('Seams', 'FLOAT_COLOR', 'POINT')
    col = np.zeros((len(me.vertices), 4), dtype=np.float32)
    col[:, 0], col[:, 1], col[:, 3] = red, green, 1.0
    attr.data.foreach_set('color', col.ravel())


def build_sleeve(rest, spec, name='Sleeve_r'):
    """The sleeve (Blender space, its rest pose) with its UVs (unrolled from the underarm seam), its 'Seams' (the
    underarm seam and the back seam), the cut end's lining, and per vertex (theta, v, fold)."""
    path = SleevePath(rest, spec.x_start)
    vs = sleeve_rows(spec, path)
    segs = spec.segs
    theta_s = np.linspace(0.0, 2.0 * math.pi, segs + 1)       # from the underarm seam (the palm side)
    theta = theta_s - math.pi
    verts, uvs, data = [], [], []
    for v in vs:
        r_y, r_z, fold = sleeve_ring(spec, path, theta, v)
        verts.append(path.points(theta, v, r_y, r_z))
        ry, rz = sleeve_radii(spec, path, v)
        circ = math.pi * (3 * (ry + rz) - math.sqrt((3 * ry + rz) * (ry + 3 * rz)))
        uvs.append(np.stack([(theta_s / (2 * math.pi) - 0.5) * circ, np.full_like(theta_s, v)], axis=1))
        data.append(np.stack([theta, np.full_like(theta, v), fold], axis=1))
    nr = len(vs)
    faces = [[i * (segs + 1) + j, i * (segs + 1) + j + 1, (i + 1) * (segs + 1) + j + 1, (i + 1) * (segs + 1) + j]
             for i in range(nr - 1) for j in range(segs)]
    V, UV, D = np.vstack(verts), np.vstack(uvs) * C, np.vstack(data)
    obj = mesh_object(name, V, faces, UV, axis=path.P, outer=np.ones(len(V), bool))
    r_mean = 0.5 * (sleeve_radii(spec, path, vs)[0].mean() + sleeve_radii(spec, path, vs)[1].mean())
    th, vv = D[:, 0], D[:, 1]
    d_under = np.abs(np.angle(np.exp(1j * (th - math.pi)))) * r_mean * C
    d_back = np.abs(np.angle(np.exp(1j * (th - 0.35)))) * r_mean * C
    green = np.clip(1.0 - np.minimum(d_under, d_back) / 0.1, 0.0, 1.0) * (vv > spec.rim - 1.0)
    seams_attribute(obj, np.zeros(len(V)), green)
    # the cut end's lining: the last ring, a ring stepped in and a fan (an object of its own: its own UV island)
    P, zz, yy, T = (a[0] for a in path.at(path.length))
    ry, rz = sleeve_radii(spec, path, path.length)
    inner = P[None, :] - T[None, :] * 0.006 + (np.outer(0.82 * ry * np.sin(theta), yy) +
                                                np.outer(0.82 * rz * np.cos(theta), zz)) * C
    LV = np.vstack([V[(nr - 1) * (segs + 1):], inner, (P - T * 0.012)[None, :]])
    LUV = np.vstack([np.stack([4.0 * np.sin(theta), 4.0 * np.cos(theta)], axis=1),
                     np.stack([3.0 * np.sin(theta), 3.0 * np.cos(theta)], axis=1), np.array([[0.0, 0.0]])]) * C
    lf = []
    for j in range(segs):
        lf.append([j, j + 1, segs + 1 + j + 1, segs + 1 + j])
        lf.append([segs + 1 + j, segs + 1 + j + 1, 2 * (segs + 1)])
    lining = mesh_object(name.replace('Sleeve', 'SleeveLining'), LV, lf, LUV)
    lining_flip(lining, P, T)
    log(f'{name}: {len(obj.data.polygons)} faces, {path.length:.1f} cm (elbow at {path.elbow_v:.1f}, cuff top '
        f'{spec.rim:.1f})')
    return obj, lining, dict(theta=th, v=vv, fold=D[:, 2], path=path)


def lining_flip(obj, centre, T):
    """The cut's lining faces out of the sleeve's end (along T)."""
    me = obj.data
    nrm = np.array([p.normal[:] for p in me.polygons])
    if float((nrm @ T).mean()) < 0.0:
        me.flip_normals()


def sleeve_weights(v, path):
    up = smoothstep(path.elbow_v - 3.5, path.elbow_v + 3.5, v)
    up = np.where(v < 0, 1.0, up)
    return {'upperarm': up, 'lowerarm': 1.0 - up}


class CuffSpec:
    """A turned-back cuff over the sleeve's end: length (cm up the arm from the sleeve's end), below (how far its fold
    reaches past the sleeve's end toward the hand), room over the sleeve, flare (extra at the fold), t (its cloth's
    thickness), ragged (notches torn in its edges, cm), tab (a buttoned tab: (theta, v0, v1, width) or None)."""

    def __init__(self, length, below=0.4, room=0.35, flare=0.3, t=0.3, ragged=0.0, tab=None, segs=28, seed=3):
        self.length, self.below, self.room, self.flare, self.t = length, below, room, flare, t
        self.ragged, self.tab, self.segs, self.seed = ragged, tab, segs, seed


def build_cuff(sleeve_spec, path, cs, name='Cuff_r'):
    """The turned-back cuff (Blender space): fitted round the sleeve under it turn by turn (the most it stands out
    over the cuff's length) with room; rows from inside under its top edge, down its inside, round the fold, up its
    outside and over its top edge onto the sleeve. Returns the object, per vertex (theta, v, kind: 0 outside, 1
    inside) and its outer half sizes per turn."""
    segs = cs.segs
    theta = np.linspace(-math.pi, math.pi, segs + 1)
    # the sleeve's reach under the cuff, turn by turn
    reach_y = np.zeros(segs + 1)
    reach_z = np.zeros(segs + 1)
    for v in np.linspace(0.0, cs.length + 0.6, 14):
        ry, rz, _ = sleeve_ring(sleeve_spec, path, theta, v)
        reach_y, reach_z = np.maximum(reach_y, ry), np.maximum(reach_z, rz)
    rng = np.random.default_rng(cs.seed)
    # torn edges: a few notches of different depths round each edge
    def notches(count):
        out = np.zeros(segs + 1)
        if cs.ragged <= 0:
            return out
        for _ in range(count):
            c, w, dpt = rng.uniform(-math.pi, math.pi), rng.uniform(0.25, 0.6), rng.uniform(0.4, 1.0) * cs.ragged
            d = np.angle(np.exp(1j * (theta - c)))
            out = np.maximum(out, dpt * np.clip(1.0 - np.abs(d) / w, 0.0, 1.0) ** 0.7)
        out += cs.ragged * 0.18 * np.abs(np.sin(theta * 7.0 + 1.3) * np.sin(theta * 3.0 + 0.4))
        out[-1] = out[0]
        return out
    top_cut = notches(7)
    fold_cut = notches(6) * 0.8              # the fold (the edge the eye sees) torn too, a little less
    L, B, t = cs.length, cs.below, cs.t
    v_top = L - top_cut
    v_fold = -B + fold_cut

    def ring(v, out_k, inset):
        """Half sizes along the cuff: out_k 0 at the top edge, 1 at the fold (flared there)."""
        ry = reach_y + cs.room + cs.flare * out_k - inset
        rz = reach_z + cs.room + cs.flare * out_k - inset
        P = np.array([path.points(np.array([th]), vv, np.array([a]), np.array([b]))[0]
                      for th, vv, a, b in zip(theta, np.broadcast_to(v, theta.shape), ry, rz)])
        return P

    rows = []           # (points, theta, v, kind)
    span = lambda f: v_fold + (v_top - v_fold) * f
    # inside: from under the top edge down to the fold
    for f in (0.7, 0.45, 0.2, 0.06):
        vv = span(f)
        rows.append((ring(vv, 1.0 - f, t), vv, 1))
    # the fold, rounded: from the inside round its lowest point to the outside
    for a in (0.25, 0.5, 0.75):
        ang = a * math.pi
        vv = v_fold + 0.05 * (1.0 - math.sin(ang)) - 0.12 * math.sin(ang)
        rows.append((ring(vv, 1.0, t * 0.5 * (1.0 + math.cos(ang))), vv, 0))
    # outside up to the top edge
    for f in (0.04, 0.15, 0.3, 0.48, 0.66, 0.83, 0.95, 1.0):
        vv = span(f)
        rows.append((ring(vv, 1.0 - f, 0.0), vv, 0))
    # over the top edge onto the sleeve
    rows.append((ring(v_top + 0.06, 0.0, t * 0.45), v_top + 0.06, 0))
    rows.append((ring(v_top - 0.05, 0.0, t * 0.95), v_top, 1))
    verts = [r[0] for r in rows]
    nr = len(rows)
    faces = [[i * (segs + 1) + j, i * (segs + 1) + j + 1, (i + 1) * (segs + 1) + j + 1, (i + 1) * (segs + 1) + j]
             for i in range(nr - 1) for j in range(segs)]
    # UVs: unrolled round the arm (u from the underarm), down the profile (v by arc length)
    r_mean = float(np.mean(0.5 * (reach_y + reach_z))) + cs.room
    uvs, arc, prev = [], 0.0, None
    for P, vv, kind in rows:
        if prev is not None:
            arc += float(np.mean(np.linalg.norm(P - prev, axis=1))) / C
        prev = P
        uvs.append(np.stack([(theta + math.pi) * r_mean, np.full_like(theta, arc)], axis=1))
    V, UV = np.vstack(verts), np.vstack(uvs) * C
    th = np.tile(theta, nr)
    vv = np.concatenate([np.broadcast_to(r[1], theta.shape) for r in rows])
    kind = np.concatenate([np.full(segs + 1, r[2]) for r in rows])
    obj = mesh_object(name, V, faces, UV, axis=path.P, outer=kind == 0)
    # sewing: the top edge is hemmed (an open edge's topstitching; a torn one has none), the fold is a fold
    edge_d = np.abs(vv - np.tile(v_top, nr)) * C
    red = np.clip(1.0 - edge_d / 0.1, 0.0, 1.0) * (kind == 0) * (cs.ragged <= 0)
    seams_attribute(obj, red, np.zeros(len(V)))
    log(f'{name}: {len(obj.data.polygons)} faces, {L:.1f} cm, {cs.room:.2f} cm over the sleeve'
        + (f', torn up to {top_cut.max():.1f} cm' if cs.ragged else ''))
    return obj, dict(theta=th, v=vv, kind=kind, reach=(reach_y + cs.room, reach_z + cs.room), theta_rows=theta)


def build_tab(path, cuff_data, cs, name='CuffTab_r'):
    """A buttoned tab on the cuff's outside (Pike's): a short flap lying 2 mm proud of the cuff, its corners clipped;
    and its button's place (point, normal)."""
    th0, v0, v1, width = cs.tab
    reach_y, reach_z = cuff_data['reach']
    theta = cuff_data['theta_rows']
    ry0 = float(np.interp(th0, theta, reach_y)) + 0.22
    rz0 = float(np.interp(th0, theta, reach_z)) + 0.22
    r_mean = 0.5 * (ry0 + rz0)
    half = width * 0.5 / r_mean
    us = np.linspace(-1.0, 1.0, 5)
    vs = np.linspace(v0, v1, 6)
    verts, uvs = [], []
    for k, vv in enumerate(vs):
        clip = 0.25 if k in (0, len(vs) - 1) else 0.0             # clipped corners at its ends
        for u in us:
            th = th0 + u * half * (1.0 - clip * abs(u))
            lift = 0.12 * (1.0 - u * u)                            # a little rounded across
            p = path.points(np.array([th]), vv, np.array([ry0 + lift]), np.array([rz0 + lift]))[0]
            verts.append(p)
            uvs.append((u * width * 0.5, vv))
    # its underside (a thin plate: the edges show their thickness)
    n_top = len(verts)
    for k, vv in enumerate(vs):
        clip = 0.25 if k in (0, len(vs) - 1) else 0.0
        for u in us:
            th = th0 + u * half * (1.0 - clip * abs(u))
            p = path.points(np.array([th]), vv, np.array([ry0 - 0.15]), np.array([rz0 - 0.15]))[0]
            verts.append(p)
            uvs.append((u * width * 0.5 + width + 0.4, vv))
    nu = len(us)
    faces = []
    for k in range(len(vs) - 1):
        for j in range(nu - 1):
            a = k * nu + j
            faces.append([a, a + 1, a + nu + 1, a + nu])
            faces.append([n_top + a, n_top + a + nu, n_top + a + nu + 1, n_top + a + 1])
    for k in range(len(vs) - 1):                                  # the long edges
        for j in (0, nu - 1):
            a, b = k * nu + j, (k + 1) * nu + j
            faces.append([a, b, n_top + b, n_top + a] if j == 0 else [b, a, n_top + a, n_top + b])
    for j in range(nu - 1):                                       # the ends
        a, b = j, j + 1
        faces.append([b, a, n_top + a, n_top + b])
        a, b = (len(vs) - 1) * nu + j, (len(vs) - 1) * nu + j + 1
        faces.append([a, b, n_top + b, n_top + a])
    V, UV = np.array(verts), np.array(uvs) * C
    obj = mesh_object(name, V, faces, UV, axis=path.P, outer=np.arange(len(V)) < n_top)
    vb = v0 + (v1 - v0) * 0.28
    P = path.points(np.array([th0]), vb, np.array([ry0 + 0.12]), np.array([rz0 + 0.12]))[0]
    Pc, zz, yy, _ = (a[0] for a in path.at(vb))
    nrm = unit(zz * math.cos(th0) + yy * math.sin(th0))
    return obj, (P, nrm)


def build_button(at, normal, radius=0.48, name='CuffButton_r', segs=16):
    """A domed brass button (Pike's cuff tab): a rim and a low dome, four holes' dimples left to the texture."""
    t1 = unit(np.cross(normal, v3(0.0, 0.0, 1.0)) if abs(normal[2]) < 0.9 else np.cross(normal, v3(1.0, 0, 0)))
    t2 = np.cross(normal, t1)
    psi = np.linspace(0.0, 2 * math.pi, segs, endpoint=False)
    profile = [(1.0, -0.05), (1.0, 0.08), (0.88, 0.17), (0.6, 0.22), (0.0, 0.24)]
    verts, uvs = [], []
    for rs, z in profile:
        r = radius * rs
        ring = at[None, :] + (np.outer(r * np.cos(psi), t1) + np.outer(r * np.sin(psi), t2) + normal[None, :] * z) * C
        verts.append(ring if rs > 0 else ring[:1])
        uvs.append(np.stack([np.cos(psi) * r, np.sin(psi) * r], axis=1) if rs > 0 else np.array([[0.0, 0.0]]))
    faces = []
    for i in range(3):
        for j in range(segs):
            a, b = i * segs + j, i * segs + (j + 1) % segs
            faces.append([a, b, b + segs, a + segs])
    tip = 4 * segs
    for j in range(segs):
        faces.append([3 * segs + j, 3 * segs + (j + 1) % segs, tip])
    # its underside, so the rim's edge has a back
    base = len(np.vstack(verts))
    V = np.vstack(verts + [at[None, :] - normal[None, :] * 0.0006])
    UV = np.vstack(uvs + [np.array([[0.0, 0.0]])]) * C
    for j in range(segs):
        faces.append([(j + 1) % segs, j, base])
    return mesh_object(name, V, faces, UV, closed=True)


class ShirtCuff:
    """A shirt's plain band cuff round the forearm (hand frame cm): from x0 (its edge, toward the hand) up to x1, half
    sizes (ry, rz) about a centre zc under the bones' line, its edge rolled out by lip, t its cloth's thickness."""

    def __init__(self, x0, x1, ry, rz, zc=-0.15, lip=0.12, t=0.18, segs=28):
        self.x0, self.x1, self.ry, self.rz, self.zc, self.lip, self.t, self.segs = x0, x1, ry, rz, zc, lip, t, segs

    def mesh(self, name='ShirtCuff_r'):
        segs = self.segs
        theta = np.linspace(-math.pi, math.pi, segs + 1)
        rows = [(self.x0 - 2.6, self.t, 1), (self.x0 - 0.4, self.t, 1), (self.x0 + 0.02, self.t * 0.55, 0),
                (self.x0, 0.0, 0)]
        span = self.x1 - self.x0
        for f in (0.08, 0.3, 0.55, 0.8, 1.0):
            rows.append((self.x0 + span * f, 0.0, 0))
        verts, uvs, kinds = [], [], []
        arc, prev = 0.0, None
        for x, inset, kind in rows:
            lip = self.lip * np.exp(-((x - self.x0) / 0.5) ** 2) if kind == 0 else 0.0
            ry, rz = self.ry + lip - inset, self.rz + lip - inset
            p = np.stack([np.full_like(theta, x), ry * np.sin(theta), self.zc + rz * np.cos(theta)], axis=1)
            if prev is not None:
                arc += float(np.mean(np.linalg.norm(p - prev, axis=1)))
            prev = p
            verts.append(p)
            uvs.append(np.stack([theta * 0.5 * (self.ry + self.rz), np.full_like(theta, arc)], axis=1))
            kinds.append(np.full(segs + 1, kind))
        nr = len(rows)
        faces = [[i * (segs + 1) + j + 1, i * (segs + 1) + j, (i + 1) * (segs + 1) + j, (i + 1) * (segs + 1) + j + 1]
                 for i in range(nr - 1) for j in range(segs)]
        V, UV = np.vstack(verts) * C, np.vstack(uvs) * C
        kind = np.concatenate(kinds)
        axis = np.stack([np.linspace(self.x1 - 3.0, self.x0 + 3.0, 20), np.zeros(20), np.full(20, self.zc)], 1) * C
        obj = mesh_object(name, V, faces, UV, axis=axis, outer=kind == 0)
        x = V[:, 0] / C
        seams_attribute(obj, np.clip(1.0 - np.abs(x - self.x0) * C / 0.1, 0, 1) * (kind == 0), np.zeros(len(V)))
        return obj, dict(co=V, kind=kind)
