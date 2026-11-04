"""The dredge barge's fore jumbo (looter_tripo_barge.rework): the 16 m of hull fore of the house built from three
different slabs of Tripo's own foredeck side (B, C, D: 70, 60 and 50 cm from the plates between Tripo's knee and the
band's bracket at the cut), copied unmirrored and laid in RUN, a sequence searched so no copy sits next to its twin and
each slab's recurrences vary (110-310 cm). Tripo put one knee on each side of its foredeck (x ~2035..2070, it stays
where it is) and a bracket in the band at the cut: no copy carries either, as two would stand closer than Tripo's own.
(Copies turned port for starboard were tried: the two sides' paint differs too much, every turned copy read as a tile.)

Each copy is warped to the hull's section at the cut (y scaled per height by the fitted half-beam's ratio: the hull
narrows over the slabs), then morphed at its ends onto the cut's section itself (each end corner moved to the nearest
point of that section, the move fading out MORPH_FADE into the copy: the sheer, the bilge and the strake differ a few
cm from plane to plane, 6-16 cm at 90 % for planes 50-110 cm aft of the cut), so every joint is the same section; the
joints are then zipped: corners snapped, every corner of either side set into the other's edges, the coincident
corners merged (watertight). Mirrored copies made butterfly seams of the paint at every joint and a sawtooth of the
bulwark's stiffeners; these make plain plate seams.
"""
import bmesh
import numpy as np

import looter_tripo_rework as rw
from looter_tripo_rework import B, U, bisect, copy, spanning, ue_centre

CUT = 2190.0                           # Tripo's x: the bow parts here (its section is the one every copy is warped to)
SLABS = {'B': (2090.0, 2160.0),        # 70 cm
         'C': (2075.0, 2135.0),        # 60 cm
         'D': (2110.0, 2160.0)}        # 50 cm: all from the plates between Tripo's knee (x ~2035..2070) and the
                                       # band's bracket at the cut (~2165..2190), which every copy otherwise carried
RUN = 'D B C B D C B C D B D C D B C D C B C B D C D B D C D'.split()   # 16.0 m: 8 B, 9 C, 10 D, searched for
                                       # varied recurrences, no twin neighbours, no joint within 30 cm of the berth
                                       # gap's ends (final x 3000, 3300)
FIT_X, FIT_STEP = (2005.0, 2190.0), 10.0          # the stations the hull's taper is fitted over
PROFILE_Z = (900.0, 2476.0, 25.0)      # the heights the half-beams are taken at
FIT_REJECT, FIT_DROPS = 4.0, 6         # cm: a station this far off the taper's line (a knee) is dropped, up to 6
RATIO = (0.85, 1.2)                    # the warp's limits
ON_PLANE, SNAP, ON_EDGE, MERGE = 0.5, 20.0, 1.5, 0.3   # cm: a joint's edges; snap reach; on an edge; merged
CHORD, HOLE_SIDES = 0.12, 12           # a corner counts as on an edge within this share of its length; slits filled
MORPH_MAX, MORPH_K = 80.0, 4           # cm: the end morph's reach; end corners blended per corner
MORPH_FADE = 12.0                      # cm: the end morph fades out this far into a copy (across the whole copy it
                                       # tilted each plate: a light patch at every joint)


def log(message):
    print(f'LOOTER: DredgeBarge rework: {message}', flush=True)


def build(bm, tag, nearest, dx, part, fill, tripo, still, strut_root):
    """Cuts at CUT and the slabs' ends, moves the bow dx forward, fills the gap with RUN's copies and zips the joints.
    Returns the joints' x (final) and the zip's tally."""
    movable = lambda f: f[tag] not in still
    for x in sorted({CUT} | {e for ab in SLABS.values() for e in ab}):
        bisect(bm, [f for f in spanning(bm, tag, None, 0, x) if movable(f)], 0, x)
    taper = Taper(bm, tag, still)
    ref = _section(bm, tag, still, CUT, strut_root)
    src = {k: [f for f in bm.faces if movable(f) and f[tag] == tripo and a < ue_centre(f)[0] < b]
           for k, (a, b) in SLABS.items()}
    fore = rw.separate(bm, [f for f in bm.faces if movable(f) and ue_centre(f)[0] > CUT])
    for v in {v for f in fore for v in f.verts}:
        v.co.y -= dx / 100.0
    pos, joints = CUT, [CUT]
    for item in RUN:
        key, turned = item[0], item.endswith('~')
        a, b = SLABS[key]

        def move(p, a=a, pos=pos, turned=turned):
            x, y, z = p
            s = 1 if y >= 0.0 else -1
            y = y * taper.ratio(x, z, s, -s if turned else s)
            return (x - a + pos, -y if turned else y, z)
        new = copy(bm, src[key], tag, part, move, flip=turned)
        new += rw.close_slot(bm, new, strut_root, tag, fill, nearest, (pos, pos + b - a))
        _morph(new, pos, pos + b - a, ref)
        pos += b - a
        joints.append(pos)
    if abs(pos - (CUT + dx)) > 0.5:
        raise ValueError(f'the run is {pos - CUT:.1f} cm long, the jumbo {dx:.1f}')
    rw.close_slot(bm, [f for f in fore if f.is_valid and ue_centre(f)[0] < CUT + dx + 400.0], strut_root, tag, fill,
                  nearest, (CUT + dx,))
    tally = zip_joints(bm, tag, joints, still, fill, nearest)
    log(f'jumbo at x {CUT:.0f}: {len(RUN)} copies of slabs ' + ', '.join(
        f'{k} {a:.0f}..{b:.0f}' for k, (a, b) in SLABS.items()) + f' (unmirrored, {sum(i.endswith("~") for i in RUN)} '
        f'turned, ends morphed onto the section at the cut), {dx:.0f} cm; {len(joints)} joints zipped: '
        f'{tally["snapped"]} corners snapped, {tally["split"]} set '
        f'into edges, {tally["merged"]} merged, {tally["slits"]} slits filled, {tally["open"]} edges left open')
    return joints, tally


def _section(bm, tag, still, x, slot_z):
    """The section at x (cut there already) as segments (y, z): starts and directions; the keel's slot (the strut's root
    stands in it) closed by a line at slot_z, as the copies' keel plates close theirs."""
    on = lambda v: abs(U(v.co)[0] - x) < 0.05
    edges = [e for e in bm.edges if on(e.verts[0]) and on(e.verts[1])
             and any(f[tag] not in still for f in e.link_faces)]
    a = np.array([_yz(e.verts[0]) for e in edges])
    b = np.array([_yz(e.verts[1]) for e in edges])
    ends = np.concatenate([a, b])
    ys = ends[np.abs(ends[:, 1] - slot_z) < 1.0, 0]
    if len(ys):
        a = np.vstack([a, [(ys.min(), slot_z)]])
        b = np.vstack([b, [(ys.max(), slot_z)]])
    return a, b - a


def _on(p, seg):
    """The nearest point to p on the segments seg: (distance, point)."""
    a, d = seg
    t = np.clip(((p - a) * d).sum(1) / np.maximum((d * d).sum(1), 1e-9), 0.0, 1.0)
    q = a + d * t[:, None]
    dist = np.sqrt(((q - p) ** 2).sum(1))
    i = int(np.argmin(dist))
    return dist[i], q[i]


def _morph(faces, x0, x1, ref):
    """Moves a copy's two end sections onto ref, the move fading out over MORPH_FADE (smoothstep)."""
    verts = {v for f in faces if f.is_valid for v in f.verts}
    fade = min(MORPH_FADE, (x1 - x0) / 2.0)   # near the joint only: a seam like a plate's, the plates between untilted
    ends = []
    for xe in (x0, x1):
        pts = np.array([_yz(v) for v in verts if abs(U(v.co)[0] - xe) < 0.05]).reshape(-1, 2)
        moves = []
        for p in pts:
            d, q = _on(p, ref)
            moves.append(q - p if d < MORPH_MAX else (0.0, 0.0))
        ends.append((xe, pts, np.array(moves).reshape(-1, 2)))
    for v in verts:
        x, y, z = U(v.co)
        p, move = np.array((y, z)), np.zeros(2)
        for xe, pts, moves in ends:
            w = 1.0 - min(abs(x - xe) / fade, 1.0)
            if w <= 0.0 or not len(pts):
                continue
            w = w * w * (3.0 - 2.0 * w)       # flat at both ends: the copies meet without a kink 
            dist = np.sqrt(((pts - p) ** 2).sum(1))
            near = np.argsort(dist)[:MORPH_K]
            k = 1.0 / (dist[near] + 1e-3)
            move += w * (moves[near] * k[:, None]).sum(0) / k.sum()
        v.co = B((x, y + move[0], z + move[1]))
    return len(verts)


class Taper:
    """The hull's half-beam per side as a straight line along x at each height, fitted over FIT_X (knees dropped)."""

    def __init__(self, bm, tag, still):
        faces = [f for f in bm.faces if f[tag] not in still and FIT_X[0] - 20.0 < ue_centre(f)[0] < FIT_X[1] + 20.0]
        arrays = rw.edge_arrays(bm, faces)
        xs = np.arange(FIT_X[0] + 1.0, FIT_X[1], FIT_STEP)        # off the cut planes
        self.zs = np.arange(*PROFILE_Z)
        self.coef = {}
        for s in (-1, 1):
            h = np.array([rw.profile(arrays, x, self.zs, s) for x in xs])
            coef = np.zeros((len(self.zs), 2))
            for j in range(len(self.zs)):
                keep = h[:, j] > 50.0
                c = np.array([0.0, h[:, j].max()])
                for _ in range(FIT_DROPS + 1):
                    if keep.sum() < 3:
                        break
                    c = np.polyfit(xs[keep], h[keep, j], 1)
                    res = np.where(keep, np.abs(np.polyval(c, xs) - h[:, j]), 0.0)
                    if res.max() < FIT_REJECT:
                        break
                    keep[int(np.argmax(res))] = False
                coef[j] = c
            self.coef[s] = coef

    def beam(self, s, x, z):
        j = min(max((z - self.zs[0]) / (self.zs[1] - self.zs[0]), 0.0), len(self.zs) - 1.0)
        j0 = int(j)
        j1, f = min(j0 + 1, len(self.zs) - 1), j - j0
        c = self.coef[s]
        return np.polyval(c[j0], x) * (1.0 - f) + np.polyval(c[j1], x) * f

    def ratio(self, x, z, s_from, s_to):
        """The y scale taking a point at x on side s_from to the cut's section on side s_to."""
        here, there = self.beam(s_from, x, z), self.beam(s_to, CUT, z)
        return min(max(there / here, RATIO[0]), RATIO[1]) if here > 50.0 else 1.0


def zip_joints(bm, tag, joints, still, fill, nearest):
    """Zips every joint plane: the open edges there of the faces aft of it with those fore of it."""
    found = {}
    for e in bm.edges:
        if not e.is_boundary or any(f[tag] in still for f in e.link_faces):
            continue
        x0, x1 = U(e.verts[0].co)[0], U(e.verts[1].co)[0]
        j = next((j for j in joints if abs(x0 - j) < ON_PLANE and abs(x1 - j) < ON_PLANE), None)
        if j is not None:
            found.setdefault(j, []).append(e)
    tally = dict(snapped=0, split=0, merged=0, open=0, slits=0)
    left = []
    for j in joints:
        out = _zip(bm, j, found.get(j, []))
        left += out.pop('left')
        for k, n in out.items():
            tally[k] += n
    # What the zip left open at a joint: thin slits between the two sides, filled in the paint beside them.
    made = bmesh.ops.holes_fill(bm, edges=[e for e in left if e.is_valid and e.is_boundary], sides=HOLE_SIDES)['faces']
    for f in made:
        f[tag] = fill
        f.normal_update()
        nearest.paint(f)
    tally['slits'] = len(made)
    tally['open'] = sum(1 for e in left if e.is_valid and e.is_boundary)
    return tally


def _yz(v):
    p = U(v.co)
    return np.array((p[1], p[2]))


def _nearest(p, edges):
    """The nearest point to p (y, z) on edges: (distance, edge index, parameter from verts[0], point)."""
    a = np.array([_yz(e.verts[0]) for e in edges])
    d = np.array([_yz(e.verts[1]) for e in edges]) - a
    ll = np.maximum((d * d).sum(1), 1e-9)
    t = np.clip(((p - a) * d).sum(1) / ll, 0.0, 1.0)
    q = a + d * t[:, None]
    dist = np.sqrt(((q - p) ** 2).sum(1))
    i = int(np.argmin(dist))
    return dist[i], i, t[i], q[i]


def _zip(bm, x, edges):
    aft = [e for e in edges if ue_centre(e.link_faces[0])[0] < x]
    fwd = [e for e in edges if ue_centre(e.link_faces[0])[0] >= x]
    out = dict(snapped=0, split=0, merged=0, left=[])
    if not aft or not fwd:
        out['left'] = list(edges)
        return out
    put = lambda v, y, z: setattr(v, 'co', B((x, y, z)))
    for v in {v for e in aft for v in e.verts}:
        put(v, *_yz(v))
    for v in {v for e in fwd for v in e.verts}:
        d, _, _, q = _nearest(_yz(v), aft)
        if d < SNAP:
            put(v, *q)
            out['snapped'] += 1
        else:
            put(v, *_yz(v))
    made = []
    # Each side's corners set into the other side's edges where they fall inside one (no T-junctions left).
    for verts, other in (({v for e in aft for v in e.verts}, fwd), ({v for e in fwd for v in e.verts}, aft)):
        for v in verts:
            p = _yz(v)
            d, i, t, q = _nearest(p, other)
            e = other[i]
            ends = min(np.linalg.norm(p - _yz(e.verts[0])), np.linalg.norm(p - _yz(e.verts[1])))
            if d < max(ON_EDGE, CHORD * e.calc_length() * 100.0) and ends > MERGE:
                ne, nv = bmesh.utils.edge_split(e, e.verts[0], float(t))
                put(nv, *p)
                other.append(ne)
                made.append(nv)
                out['split'] += 1
    verts = list({v for e in aft + fwd if e.is_valid for v in e.verts} | set(made))
    n = len(bm.verts)
    bmesh.ops.remove_doubles(bm, verts=[v for v in verts if v.is_valid], dist=MERGE / 100.0)
    out['merged'] = n - len(bm.verts)
    on = lambda v: abs(U(v.co)[0] - x) < ON_PLANE
    out['left'] = list({e for v in verts if v.is_valid for e in v.link_edges
                        if e.is_boundary and on(e.verts[0]) and on(e.verts[1])})
    return out
