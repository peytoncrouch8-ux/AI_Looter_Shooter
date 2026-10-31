"""The Meadow Wolf's limbs rebuilt from the Tripo model (parts_spider_build.py runs it): every leg, palp and chelicera
as clean rings of quads along its own centre line, its radius at each ring read off Tripo's surface by rays, so the
shape (each segment's swell, the narrow joints, the claws' taper) is Tripo's and the topology is ours: even rings with
edge loops at every joint, UVs along and round the limb onto Spider.py's atlas (U along, V round, 0.5 on top: the
painter's own axes), so the painted hide's rings and joints fall on Tripo's joints.

- frames(), sample(): a limb's stations (its centre line, cm) with their frames (top toward the bend's pole, in the
  limb's plane) and radii (SIDES rays a station; a missed ray filled round its ring, a missed ring from its neighbour).
- joints(): the narrow points along a leg (the radius's dips): the coxa's, the knee, the patella's, the metatarsus's.
- carry(): a bone's stations from the model's pose into the rig's: its own joint frame (head, along to its far joint,
  top toward the pole) onto the rig bone's, stretched along by a piecewise rule (the coxa and patella little, the long
  segments the rest) and scaled across.
- tube(): the rings as quads into an lc.Mesh, its ends rounded off, closed to a hub or drawn to a point (a claw)."""
import math

import numpy as np
from mathutils import Vector

SIDES = 12
DOME = 0.9             # a rounded end reaches this share of its ring's radius past it


def frames(P, normal, pole):
    """Each station's (tangent, top, side): top square to the tangent in the plane whose normal is normal, toward pole;
    side = tangent x top (as Spider.py's segment() turns its rings)."""
    T = np.gradient(P, axis=0)
    T /= np.maximum(np.linalg.norm(T, axis=1, keepdims=True), 1e-9)
    n = np.asarray(normal, float) / np.linalg.norm(normal)
    flip = -1.0 if np.cross(n, (P[-1] - P[0]) / np.linalg.norm(P[-1] - P[0])) @ np.asarray(pole, float) < 0 else 1.0
    top = np.cross(n, T) * flip
    top /= np.maximum(np.linalg.norm(top, axis=1, keepdims=True), 1e-9)
    side = np.cross(T, top)
    return T, top, side


def angles(sides=SIDES):
    """The ring's angles from its top (Spider.py's: -pi, the underside, first; V = j / sides, 0.5 on top)."""
    return np.array([-math.pi + 2.0 * math.pi * j / sides for j in range(sides)])


def sample(tree, P, T, top, side, sides=SIDES, reach=60.0):
    """Radii (stations x sides) by rays from each station, a missed ray filled from its ring's hits (round it), a ring
    under a third hit copied from its neighbour."""
    a = angles(sides)
    R = np.full((len(P), sides), np.nan)
    for i in range(len(P)):
        for j in range(sides):
            d = top[i] * math.cos(a[j]) + side[i] * math.sin(a[j])
            hit = tree.ray_cast(Vector(P[i]), Vector(d), reach)
            if hit[0] is not None:
                R[i, j] = hit[3]
    good = np.isfinite(R).sum(1) >= sides / 3.0
    for i in range(len(P)):
        if good[i] and np.isnan(R[i]).any():
            ok = np.isfinite(R[i])
            idx = np.arange(sides)
            R[i] = np.interp(idx, np.concatenate([idx[ok] - sides, idx[ok], idx[ok] + sides]),
                             np.concatenate([R[i, ok]] * 3))
    known = np.nonzero(good)[0]
    if not len(known):
        raise ValueError('limb: no ring hit its surface')
    for i in range(len(P)):
        if not good[i]:
            R[i] = R[known[np.argmin(np.abs(known - i))]]
    return R


def smooth_radii(R, along=1, around=1):
    R = R.copy()
    for _ in range(along):
        Q = R.copy()
        Q[1:-1] = (R[:-2] + 2.0 * R[1:-1] + R[2:]) / 4.0
        R = Q
    for _ in range(around):
        R = (np.roll(R, 1, 1) + 2.0 * R + np.roll(R, -1, 1)) / 4.0
    return R


def arc(P):
    return np.concatenate([[0.0], np.cumsum(np.linalg.norm(np.diff(P, axis=0), axis=1))])


def _dips(m, s, lo, hi, window=6.0):
    """The narrow points of m (local minima) between arc lengths lo and hi: [(index, prominence)], the prominence how
    far m rises within window cm on its lower side."""
    out = []
    for i in range(1, len(m) - 1):
        if lo <= s[i] <= hi and m[i] <= m[i - 1] and m[i] < m[i + 1]:
            left = m[(s >= s[i] - window) & (s < s[i])].max()
            right = m[(s > s[i]) & (s <= s[i] + window)].max()
            out.append((i, float(min(left, right) - m[i])))
    return out


def joints(R, s, start):
    """A leg's joints from its mean radius m along it (arc lengths s; Tripo's hip ring at start): the coxa's narrow
    point (the deepest before the femur's swell), the knee (the first clear narrow point after the swell), the
    patella's end (the first after that), the metatarsus's start (the clearest narrow point further on, else where the
    radius falls hardest), the tarsus's (most of the way on: Tripo draws it as a taper). Returns arc lengths."""
    m = R.mean(1)
    length = s[-1]
    leg = length - start
    sel = np.nonzero((s >= start + leg * 0.08) & (s <= start + leg * 0.42))[0]
    swell = int(sel[np.argmax(m[sel])])
    found = _dips(m, s, start + 4.0, s[swell] - 3.0)
    coxa = s[max(found, key=lambda d: d[1])[0]] if found else start + 0.25 * (s[swell] - start)
    found = [i for i, p in _dips(m, s, s[swell] + 3.0, s[swell] + leg * 0.35) if p >= 0.15]
    if found:
        knee = s[found[0]]
    else:
        sel = np.nonzero((s >= s[swell] + 3.0) & (s <= s[swell] + leg * 0.35))[0]
        knee = s[sel[np.argmin(m[sel])]]
    found = [i for i, p in _dips(m, s, knee + leg * 0.06, knee + leg * 0.35) if p >= 0.12]
    patella = s[found[0]] if found else knee + leg * 0.18
    found = [(i, p) for i, p in _dips(m, s, patella + leg * 0.1, length - leg * 0.1) if p >= 0.15]
    if found:
        meta = s[max(found, key=lambda d: d[1])[0]]
    else:
        sel = np.nonzero((s >= patella + leg * 0.1) & (s <= length - leg * 0.1))[0]
        meta = s[sel[np.argmin(np.gradient(m)[sel])]]
    return dict(coxa=coxa, knee=knee, patella=patella, meta=meta, tarsus=meta + 0.72 * (length - meta), tip=length)


def piecewise(knots):
    """g(a): linear through knots [(a, g)], carried on past both ends at the end segments' slopes."""
    knots = sorted(knots)
    xs = np.array([k[0] for k in knots])
    ys = np.array([k[1] for k in knots])

    def g(a):
        a = np.asarray(a, float)
        out = np.interp(a, xs, ys)
        lo = (ys[1] - ys[0]) / (xs[1] - xs[0])
        hi = (ys[-1] - ys[-2]) / (xs[-1] - xs[-2])
        out = np.where(a < xs[0], ys[0] + (a - xs[0]) * lo, out)
        return np.where(a > xs[-1], ys[-1] + (a - xs[-1]) * hi, out)
    return g


def carry(P, source, target, g, across):
    """Stations P from the source bone frame into the target's: along through g, across scaled (across: the off-line
    scale of the centre line, the bends of a leg between its joints)."""
    h, x, y, z = source
    H, X, Y, Z = target
    d = P - h
    a, b, c = d @ y, d @ x, d @ z
    return H + np.outer(g(a), Y) + (np.outer(b, X) + np.outer(c, Z)) * across


def tube(m, C, top, side, R, U, region, start='hub', end='round', tip=None, region_at=None):
    """Rings at stations C (radii R, stations x sides; frames top and side) as quads into the lc.Mesh m: U along (each
    station's place in region, 0..1; or a function u(k, region)), V round. start/end: 'hub' (closed to a low point just
    inside the ring), 'round' (a dome: about half a sphere, a joint's end) or 'tip' (drawn to the point tip: a claw).
    region_at(k): the region of the quads after station k (a UV seam where it changes), else region."""
    sides = R.shape[1]
    a = angles(sides)
    T = np.gradient(C, axis=0)
    T /= np.maximum(np.linalg.norm(T, axis=1, keepdims=True), 1e-9)
    rows = [(C[i], R[i], top[i], side[i]) for i in range(len(C))]
    bm = m.bm

    def ring(centre, radii, tp, sd):
        return [bm.verts.new(Vector(centre + (tp * math.cos(a[j]) + sd * math.sin(a[j])) * radii[j]))
                for j in range(sides)]

    verts = [ring(*row) for row in rows]
    reg = (lambda k: region) if region_at is None else region_at
    u_at = U if callable(U) else (lambda k, r: U[k])
    for k in range(len(rows) - 1):
        rk = reg(k)
        ua, ub = u_at(k, rk), u_at(k + 1, rk)
        for j in range(sides):
            j1 = (j + 1) % sides
            va, vb = j / sides, (j + 1) / sides
            m.face((verts[k][j], verts[k + 1][j], verts[k + 1][j1], verts[k][j1]),
                   [m.at(rk, ua, va), m.at(rk, ub, va), m.at(rk, ub, vb), m.at(rk, ua, vb)])

    def close(k, kind, first):
        rk = reg(0 if first else len(rows) - 2)
        u = u_at(k, rk)
        ring_v = verts[k]
        sign = -1.0 if first else 1.0
        r = float(np.mean(R[k]))
        if kind == 'round':
            # A dome: two rings on a quarter circle (at 35 and 65 degrees), then its point; a joint's end, smooth.
            depth = r * DOME
            for angle in (35.0, 65.0):
                c, s_ = math.cos(math.radians(angle)), math.sin(math.radians(angle))
                shoulder = ring(C[k] + T[k] * sign * depth * s_, R[k] * c, top[k], side[k])
                for j in range(sides):
                    j1 = (j + 1) % sides
                    va, vb = j / sides, (j + 1) / sides
                    quad = (shoulder[j], ring_v[j], ring_v[j1], shoulder[j1]) if first else \
                        (ring_v[j], shoulder[j], shoulder[j1], ring_v[j1])
                    m.face(quad, [m.at(rk, u, va), m.at(rk, u, va), m.at(rk, u, vb), m.at(rk, u, vb)])
                ring_v = shoulder
            hub = C[k] + T[k] * sign * depth
        elif kind == 'tip':
            hub = np.asarray(tip, float)
        else:
            hub = C[k] + T[k] * sign * r * 0.5
        h = bm.verts.new(Vector(hub))
        for j in range(sides):
            j1 = (j + 1) % sides
            f = (h, ring_v[j1], ring_v[j]) if first else (h, ring_v[j], ring_v[j1])
            m.face(f, [m.at(rk, u, 0.5), m.at(rk, u, (j + 1) / sides), m.at(rk, u, j / sides)])

    close(0, start, True)
    close(len(rows) - 1, end, False)
