"""The Meadow Wolf's round parts from the Tripo model (parts_spider_build.py runs it): the cephalothorax (the body's and
the head's bones), the abdomen, the eyes. Each shell is a latitude-longitude grid of rays from its middle onto Tripo's
surface (its own faces only), so the shape is Tripo's and the topology an even quad grid (poles as fans, as
lc.ellipsoid's), with lc.ellipsoid's UVs on Spider.py's atlas regions: U round (0.5 the back), V along (0 the rear
pole), the painter's own axes.
- cephalothorax(): the holes where the legs, chelicerae and palps were and the waist are bridged by a smooth membrane
  (fill_grid); the grid is cut at the ring nearest X_HEAD into the body's part (region carapace) and the head's (region
  head, V from HEAD_V0): the seam ring is in both, and seam_normals() shades across it once the parts are joined.
- abdomen(): its front, where it grows out of the carapace, closes on the egg fitted to it (shell rays that find
  nothing there take the egg's radius, then blend in); two spinnerets at its rear as the scripted spider's.
- eyes(): glossy black balls (Spider.py's solid eye patch, which the tier look lights) where Tripo painted its eyes: a
  fine grid of rays over the face, the black (or wine-dark, never brown) hits gathered into blobs, made symmetric, and
  a wolf spider's eight kept (the big forward pair, the row of four under it, the pair on top)."""
import math

import numpy as np
from mathutils import Vector

import looter_creatures as lc
import parts_spider_shape as ss

X_HEAD = 18.0           # cm: the head's part of the cephalothorax starts at the ring nearest this
HEAD_V0 = 0.45          # the head's region starts at this V (its painter darkens the face past 0.6)
CARAPACE_GRID = (26, 40)
ABDOMEN_GRID = (28, 44)
EYE_GRID = (90, 140)    # the face's fine grid (rings over the head's part, segments round)
EYE_DARK = 0.16         # Tripo's eyes: every channel under this (sRGB)
EYE_LEAST = 0.9         # cm: smaller dark specks are paint, not eyes
EYE_PAIR = 4.0          # cm: a blob and the other side's mirrored this near are one pair of eyes
EYE_SINK = 0.5          # an eye ball's middle sits this share of its radius under Tripo's painted eye
SPINNERETS = True


def log(message):
    ss.log(message)


def _smooth_grid(r, passes=1, keep=None):
    for _ in range(passes):
        up = np.vstack([r[:1], r[:-1]])
        down = np.vstack([r[1:], r[-1:]])
        avg = (up + down + np.roll(r, 1, 1) + np.roll(r, -1, 1) + 4.0 * r) / 8.0
        r = avg if keep is None else np.where(keep, r, avg)
    return r


def _grid_points(centre, dirs, r):
    return centre + dirs * r[..., None]


def emit(m, centre, dirs, r, poles, fwd, rows, regions):
    """Faces of the grid between ring rows rows[0]..rows[1] (0 the rear pole, rings the front pole) into lc.Mesh m, with
    their own vertices (a part cut out of a shell). regions(i) -> (region, V) for ring i."""
    rings = r.shape[0] + 1
    segs = r.shape[1]
    bm = m.bm
    lo, hi = rows
    P = _grid_points(centre, dirs, r)
    ring = {i: [bm.verts.new(Vector(P[i - 1, j])) for j in range(segs)]
            for i in range(max(lo, 1), min(hi, rings - 1) + 1)}
    rear = bm.verts.new(Vector(centre - fwd * poles[0])) if lo == 0 else None
    front = bm.verts.new(Vector(centre + fwd * poles[1])) if hi == rings else None
    for j in range(segs):
        j1 = (j + 1) % segs
        ua, ub = j / segs, (j + 1) / segs
        if rear is not None:
            reg, v1 = regions(1)
            m.face((rear, ring[1][j1], ring[1][j]),
                   [m.at(reg, (ua + ub) / 2, 0.0), m.at(reg, ub, v1), m.at(reg, ua, v1)])
        for i in range(max(lo, 1), min(hi, rings - 1)):
            reg, va = regions(i)
            vb = regions(i + 1, reg)[1]
            m.face((ring[i][j], ring[i][j1], ring[i + 1][j1], ring[i + 1][j]),
                   [m.at(reg, ua, va), m.at(reg, ub, va), m.at(reg, ub, vb), m.at(reg, ua, vb)])
        if front is not None:
            reg, vl = regions(rings - 1)
            m.face((ring[rings - 1][j], ring[rings - 1][j1], front),
                   [m.at(reg, ua, vl), m.at(reg, ub, vl), m.at(reg, (ua + ub) / 2, 1.0)])


def cephalothorax(m_body, m_head, src, V, labels):
    """The carapace and head (V: the stretched cephalothorax's space). Returns what the eyes and the abdomen need."""
    mask = labels == ss.CARAPACE
    P = V[mask]
    lo, hi = P.min(0), P.max(0)
    centre = np.array([(lo[0] + hi[0]) * 0.5, 0.0, (lo[2] + hi[2]) * 0.5])
    radii = (hi - lo) * 0.5
    rings, segs = CARAPACE_GRID
    r, _, dirs, poles, frame = ss.sample_shell(V, src.F, mask, centre, (1, 0, 0), (0, 0, 1), radii, rings, segs)
    log(f'cephalothorax: centre {np.round(centre, 1).tolist()}, radii {np.round(radii, 1).tolist()}, '
        f'{np.isnan(r).mean() * 100:.0f}% of its rays bridged')
    r = ss.fill_grid(r)
    r = _smooth_grid(r, 1)
    poles = [p if np.isfinite(p) else float(np.mean(r[0 if k == 0 else -1])) for k, p in enumerate(poles)]
    # The ring nearest X_HEAD cuts the head's part off.
    xs = (centre + dirs * r[..., None])[..., 0].mean(1)
    ks = int(np.argmin(np.abs(xs - X_HEAD))) + 1

    def body_regions(i, reg=None):
        return 'carapace', i / rings

    def head_regions(i, reg=None):
        return 'head', HEAD_V0 + (1.0 - HEAD_V0) * (i - ks) / (rings - ks)
    emit(m_body, centre, dirs, r, poles, frame[0], (0, ks), body_regions)
    emit(m_head, centre, dirs, r, poles, frame[0], (ks, rings), head_regions)
    shell = _grid_points(centre, dirs, r)
    log(f'cephalothorax: x {shell[..., 0].min():.0f}..{centre[0] + poles[1]:.0f} cm, the head from x {xs[ks - 1]:.0f}')
    return dict(centre=centre, radii=radii, frame=frame, fwd=frame[0], dirs=dirs, r=r, ks=ks, rings=rings, src=src, V=V,
                mask=mask, tree=ss.surface(V, src.F, mask))


def abdomen(m, src, V, labels, fit, space):
    """The abdomen (V: uniform space), moved back by what the cephalothorax's stretch added behind it."""
    c, A = fit
    mask = labels == ss.ABDOMEN
    front = V[mask][:, 0].max()
    shift = np.array([space.stretch(np.array([front, 0.0, 0.0]))[0] - front, 0.0, 0.0])
    w, vecs = np.linalg.eigh(A)
    axis = vecs[:, np.argmin(w)]                       # the egg's long axis (its smallest eigenvalue)
    fwd = axis * (1.0 if axis[0] > 0 else -1.0)
    radii = 1.0 / np.sqrt(w[np.argsort(w)])
    rings, segs = ABDOMEN_GRID
    Vs = V + shift
    centre = c + shift
    r, _, dirs, poles, frame = ss.sample_shell(Vs, src.F, mask, centre, fwd, (0, 0, 1), tuple(radii), rings, segs)
    known = ~np.isnan(r)
    egg = 1.0 / np.sqrt(np.einsum('ijk,kl,ijl->ij', dirs, A, dirs)) * 0.97
    r = np.where(known, r, egg)
    r = _smooth_grid(r, 30, keep=known)
    r = _smooth_grid(r, 1)
    egg_poles = [1.0 / math.sqrt(fwd @ A @ fwd) * 0.97] * 2
    poles = [p if np.isfinite(p) else e for p, e in zip(poles, egg_poles)]
    emit(m, centre, dirs, r, poles, frame[0], (0, rings), lambda i, reg=None: ('abdomen', i / rings))
    log(f'abdomen: moved {shift[0]:.1f} cm back, centre {np.round(centre, 1).tolist()}, axis '
        f'{np.round(fwd, 3).tolist()}, {(~known).mean() * 100:.0f}% of its rays on the egg (its front), rear at x '
        f'{centre[0] - poles[0] * fwd[0]:.0f}')
    if SPINNERETS:
        rear = centre - fwd * poles[0]
        for side in (1.0, -1.0):
            root = rear + fwd * 4 + np.array([0, side * 3, -2])
            end = rear - fwd * 5 + np.array([0, side * 4, -5])
            lc.tube(m, [Vector(root), Vector(end)], [3.0, 1.2], 'misc', sides=6, patch=True)
    return dict(centre=centre, dirs=dirs, r=r, fwd=fwd, shift=shift)


def eyes(m, face):
    """Glossy black balls where Tripo painted the eyes (a fine grid of rays over the head's part; see the module)."""
    src, V, centre, radii = face['src'], face['V'], face['centre'], face['radii']
    tree = face['tree']
    fwd, left, up = face['frame']
    rings, segs = EYE_GRID
    theta0 = math.pi * (face['ks'] - 1) / face['rings']
    dark = np.zeros((rings, segs), bool)
    rgb = np.zeros((rings, segs, 3))
    pts = np.zeros((rings, segs, 3))
    nrm = np.zeros((rings, segs, 3))
    for i in range(rings):
        theta = theta0 + (math.pi - theta0) * (i + 0.5) / rings
        for j in range(segs):
            phi = -math.pi + 2.0 * math.pi * j / segs
            a, rr = -math.cos(theta), math.sin(theta)
            d = fwd * (a * radii[0]) + left * (rr * math.sin(phi) * radii[1]) + up * (rr * math.cos(phi) * radii[2])
            d /= np.linalg.norm(d)
            hit = tree.ray_cast(Vector(centre), Vector(d), 1000.0)
            if hit[0] is None:
                continue
            pts[i, j] = np.array(hit[0])
            nrm[i, j] = np.array(hit[1])
            rgb[i, j] = _colour(src, V, face, hit)
            # Tripo's eyes are black or a dark wine red, never brown: blue at least green (its dark paint is brown).
            dark[i, j] = rgb[i, j].max() < EYE_DARK and rgb[i, j][2] >= rgb[i, j][1]
    # Tripo's eyes carry painted highlights that break them up: closed (grown a cell, shrunk back) before the blobs.
    dark = _shrink(_grow(dark)) & (np.linalg.norm(pts, axis=-1) > 0)
    # Blobs: 4-connected dark cells (wrapping round).
    seen = np.zeros_like(dark)
    blobs = []
    for i in range(rings):
        for j in range(segs):
            if not dark[i, j] or seen[i, j]:
                continue
            stack, cells = [(i, j)], []
            seen[i, j] = True
            while stack:
                a, b = stack.pop()
                cells.append((a, b))
                for da, db in ((1, 0), (-1, 0), (0, 1), (0, -1)):
                    p, q = a + da, (b + db) % segs
                    if 0 <= p < rings and dark[p, q] and not seen[p, q]:
                        seen[p, q] = True
                        stack.append((p, q))
            if len(cells) >= 4:
                Q = np.array([pts[a, b] for a, b in cells])
                N = np.array([nrm[a, b] for a, b in cells]).mean(0)
                mid = Q.mean(0)
                radius = float(np.sqrt(((Q - mid) ** 2).sum(1).max())) * 1.05
                blobs.append((mid, N / np.linalg.norm(N), radius, len(cells)))
    blobs = _wolf_eyes(_mirrored([b for b in blobs if b[2] >= EYE_LEAST]))
    for mid, n, radius, count in blobs:
        # The big forward pair are Tripo's wide ovals (flatter along their look, lower than wide); the rest round.
        big = radius > 3.0
        size = (radius * 0.75, radius, radius * 0.72) if big else (radius, radius, radius)
        ball = mid - n * size[0] * EYE_SINK
        lc.ellipsoid(m, Vector(ball), size, 'eye', fwd=Vector(n), segs=16 if big else 8, rings=10 if big else 6,
                     patch=True)
    log(f'eyes: {len(blobs)} ({", ".join(f"r {b[2]:.1f} at {np.round(b[0], 0).tolist()}" for b in blobs)})')
    return blobs


def _colour(src, V, face, hit):
    idx = face.setdefault('_idx', np.nonzero(face['mask'][src.F].all(1))[0])
    tri = int(idx[hit[2]])
    rgb = ss.colour_at(src, tri, np.array(hit[0]), V)
    return rgb


def _grow(mask):
    out = mask.copy()
    out[1:] |= mask[:-1]
    out[:-1] |= mask[1:]
    return out | np.roll(mask, 1, 1) | np.roll(mask, -1, 1)


def _shrink(mask):
    out = mask.copy()
    out[1:] &= mask[:-1]
    out[:-1] &= mask[1:]
    return out & np.roll(mask, 1, 1) & np.roll(mask, -1, 1)


def _mirrored(blobs):
    """The eyes made symmetric: a blob and the other side's mirrored one within EYE_PAIR become a pair (their places
    averaged, the bigger radius: Tripo's paint loses part of an eye to a highlight); a blob on the middle line stays on
    it; a blob alone gets its mirror."""
    flip = np.array([1.0, -1.0, 1.0])
    left = [b for b in blobs if b[0][1] > 1.5]
    right = [b for b in blobs if b[0][1] < -1.5]
    middle = [b for b in blobs if abs(b[0][1]) <= 1.5]
    out, used = [], set()
    for b in sorted(left, key=lambda b: -b[2]):
        near = [(np.linalg.norm(c[0] * flip - b[0]), k) for k, c in enumerate(right) if k not in used]
        near = [x for x in near if x[0] < max(EYE_PAIR, b[2])]
        if near:
            _, k = min(near)
            used.add(k)
            c = right[k]
            mid = (b[0] + c[0] * flip) * 0.5
            n = b[1] + c[1] * flip
            radius = max(b[2], c[2])
        else:
            mid, n, radius = b[0], b[1], b[2]
        n = n / np.linalg.norm(n)
        out += [(mid, n, radius, b[3]), (mid * flip, n * flip, radius, b[3])]
    for k, c in enumerate(right):
        if k not in used:
            out += [(c[0], c[1], c[2], c[3]), (c[0] * flip, c[1] * flip, c[2], c[3])]
    for b in middle:
        mid, n = b[0] * np.array([1.0, 0.0, 1.0]), b[1] * np.array([1.0, 0.0, 1.0])
        out.append((mid, n / np.linalg.norm(n), b[2], b[3]))
    return out


def _wolf_eyes(blobs):
    """A wolf spider's eight from the blobs: the big forward pair (the biggest on the brow), the row of four small ones
    under it, the pair on top behind it. Anything else dark on the face is Tripo's paint."""
    big = sorted([b for b in blobs if b[2] > 3.0 and abs(b[0][1]) < 12.0], key=lambda b: -b[2])[:2]
    if len(big) < 2:
        log('eyes: no big forward pair found; none placed')
        return []
    bx, bz = np.mean([b[0][0] for b in big]), np.mean([b[0][2] for b in big])
    row = [b for b in blobs
           if b[2] <= 2.2 and abs(b[0][1]) < 12.0 and bz - 9.0 < b[0][2] < bz - 2.0 and b[0][0] > bx - 3.0]
    top = [b for b in blobs if b[2] <= 2.5 and b[0][2] > bz - 3.0 and b[0][0] < bx - 5.0 and abs(b[0][1]) < 14.0]
    return big + row + top
