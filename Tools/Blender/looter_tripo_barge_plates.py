"""Plates pressed from Tripo's own surfaces for the side walks' passages (looter_tripo_barge_passage): a stretch of a
walk's wall or floor copied aside (Plating: moved STASH_DZ under the barge before the passages' cuts take its place),
pieces cut from it one after another along the stretch, each pressed flat to RELIEF of its depth and laid as a
rectangle anywhere (plate()), turned to face its way as one piece (stairs.rect's rule). The bulwark's band and capping
are the stairs' Stock (looter_tripo_barge_stairs), its bars the stairs' bar(). Every face keeps Tripo's paint (its UVs
copied); the copies aside are dropped when the passages are done (drop())."""
import bmesh
import numpy as np
from mathutils import Vector

from looter_tripo_rework import D, copy, patch, ue_centre

STASH_DZ = -5000.0                     # cm: the copies aside lie this far under the barge while the cuts run
RELIEF = 0.25                          # of a piece's source depth kept (rivets and plate edges, shallow: as the stairs')
DEPTH_IN, DEPTH_OUT = 25.0, 8.0        # cm into and out of a wall's face (its plane, leaning or plumb) its plating is
                                       # copied from (what stands out more is a shard or a fitting, not plating)
PRESS_OUT = 4.0                        # cm: a stock face standing out of the face is pressed as if this far out at most
STOCK_BASE = 25.0                      # cm over its floor a wall's plating is read from (its foot flares out into the
                                       # skirt: read from the floor, every plate's foot came out empty, the seam open)
FILL_CLEAR = 20.0                      # cm: a stock stretch keeps this far off the rework's fills (caps: no plating)
STRETCH_MIN = 60.0                     # cm: the shortest stretch kept when the fills part one
FLOOR_DEPTH = (6.0, 2.0)               # cm under and over a floor's top its plating is copied from
MOVE_ON = (0.61, 17.0)                 # the stretch read on by this share of each piece's length plus this (cm)
PIECE_MIN = 12.0                       # cm: the least source a piece is cut from either way (slivers had no tangents)


class Plating:
    """A plating stock: stretches of one surface, either a wall (kind 'wall': its face |y| = line(x, z) on side s,
    from its floor's top up) or a floor (kind 'floor': its top z = top(x) between |y| y0..y1). stretches: [(x0, x1)];
    for a wall, line(x, z) its face's |y| (z over its floor: it may lean) and top(x) its floor's z; height: how much
    of the wall is plating (cm)."""

    def __init__(self, bm, tag, part, kind, side, stretches, line=None, top=None, height=None, y_span=None, skip=None):
        self.kind, self.side, self.line, self.top, self.height = kind, side, line, top, height
        self.y_span = y_span
        self.stretches = [(a, b) for a, b in stretches if b - a >= PIECE_MIN]
        z0 = STOCK_BASE if kind == 'wall' else 0.0
        if not self.stretches:
            raise ValueError(f'a passage plating stock ({kind}, side {side}) has no stretch')
        picked = []
        for f in bm.faces:
            x, y, z = ue_centre(f)
            xs = [-v.co.y * 100.0 for v in f.verts]       # a face reaching into a stretch counts (by its middle,
            zs = [v.co.z * 100.0 for v in f.verts]        # big faces at a stretch's ends were holes in its pieces)
            if y * side <= 0.0 or f[tag] == skip or not any(max(xs) > a - 5.0 and min(xs) < b + 5.0
                                                            for a, b in self.stretches):
                continue                                  # skip: the rework's fills (caps painted in a smear)
            if kind == 'wall':
                d = line(x, z - top(x)) - abs(y)
                if -DEPTH_OUT < d < DEPTH_IN and max(zs) > top(x) + z0 - 2.0 and min(zs) < top(x) + z0 + height + 10.0:
                    picked.append(f)
            elif y_span[0] - 5.0 < abs(y) < y_span[1] + 5.0 and top(x) - FLOOR_DEPTH[0] < z < top(x) + FLOOR_DEPTH[1]:
                picked.append(f)
        self.stretches = self._clear_of(bm, tag, skip, line, top, height, y_span)
        self.faces = copy(bm, picked, tag, part, move=lambda p: (p[0], p[1], p[2] + STASH_DZ))
        self.k, self.at = 0, 0.0

    def take(self, length):
        """(stretch x0, x where the next piece of length cm starts): read on along the stretches in turn."""
        for _ in range(2 * len(self.stretches) + 2):
            a, b = self.stretches[self.k]
            if length <= b - a:
                if self.at + length > b - a:
                    self.at = (self.at * MOVE_ON[0]) % max(b - a - length, 1.0)
                x = a + self.at
                self.at += length * MOVE_ON[0] + MOVE_ON[1]
                if self.at > b - a:
                    self.k, self.at = (self.k + 1) % len(self.stretches), 0.0
                return x
            self.k, self.at = (self.k + 1) % len(self.stretches), 0.0
        return None

    def _clear_of(self, bm, tag, skip, line, top, height, y_span):
        """The stretches parted round the fills in the stock's window (a corridor's cap is no plating: its paint is a
        smear; left out, a hole in every piece cut there). The stretches as they were if that leaves too little."""
        if skip is None:
            return self.stretches
        xs = []
        for f in bm.faces:
            if f[tag] != skip:
                continue
            x, y, z = ue_centre(f)
            if y * self.side <= 0.0 or not any(a - FILL_CLEAR < x < b + FILL_CLEAR for a, b in self.stretches):
                continue
            if self.kind == 'wall':
                inside = (-DEPTH_OUT < line(x, z - top(x)) - abs(y) < DEPTH_IN
                          and top(x) < z < top(x) + STOCK_BASE + height + 10.0)
            else:
                inside = y_span[0] < abs(y) < y_span[1] and abs(z - top(x)) < FLOOR_DEPTH[0]
            if inside:
                xs.append(x)
        out = []
        for a, b in self.stretches:
            cuts = sorted(x for x in xs if a - FILL_CLEAR < x < b + FILL_CLEAR)
            start = a
            for x in cuts:
                if x - FILL_CLEAR - start >= STRETCH_MIN:
                    out.append((start, x - FILL_CLEAR))
                start = max(start, x + FILL_CLEAR)
            if b - start >= STRETCH_MIN:
                out.append((start, b))
        return out if sum(b - a for a, b in out) >= 2.0 * STRETCH_MIN else self.stretches

    def longest(self):
        return max(b - a for a, b in self.stretches)


def plate(bm, tag, part, stock, o, a, b, n):
    """A rectangle o + s a + t b (s, t in 0..1; cm) facing n, pressed from the stock: a along the stock's x, b up its
    wall or across its floor (outboard). Longer than its stretches, a piece is several. Returns its faces."""
    o, a, b, n = (np.array(v, dtype=float) for v in (o, a, b, n))
    n = n / np.linalg.norm(n)
    la, lb = float(np.linalg.norm(a)), float(np.linalg.norm(b))
    most = stock.longest()
    parts = max(1, int(np.ceil(la / most)))
    faces = []
    for k in range(parts):
        o_k, a_k = o + a * (k / parts), a / parts
        faces += _piece(bm, tag, part, stock, o_k, a_k, b, n, max(la / parts, PIECE_MIN), max(lb, PIECE_MIN))
    return faces


def _piece(bm, tag, part, stock, o, a, b, n, la, lb):
    sx = stock.take(la)
    if sx is None:
        return []
    if stock.kind == 'wall':
        lb = min(lb, stock.height)
        ys = [stock.line(sx + la * u, h) for u in (0.0, 0.5, 1.0) for h in (STOCK_BASE, STOCK_BASE + lb)]
        top = stock.top(sx + la / 2.0)
        lo_y, hi_y = min(ys) - DEPTH_IN, max(ys) + DEPTH_OUT
        z0 = top + STOCK_BASE                            # read from over the stock's own foot
        box = (sx, sx + la, lo_y, hi_y, z0 + STASH_DZ, z0 + lb + STASH_DZ)
        st = lambda p: ((p[0] - sx) / la, (p[2] - STASH_DZ - z0) / lb,
                        max(stock.line(p[0], p[2] - STASH_DZ - top) - abs(p[1]), -PRESS_OUT))
    else:
        lb = min(lb, stock.y_span[1] - stock.y_span[0])
        top = stock.top(sx + la / 2.0)
        lo_y = stock.y_span[0]
        box = (sx, sx + la, lo_y, lo_y + lb, top - FLOOR_DEPTH[0] + STASH_DZ, top + FLOOR_DEPTH[1] + STASH_DZ)
        st = lambda p: ((p[0] - sx) / la, (abs(p[1]) - lo_y) / lb, max(top - (p[2] - STASH_DZ), 0.0))   # never over
    if stock.side < 0:                                   # the stock's own side (its y negative)
        box = (box[0], box[1], -box[3], -box[2], box[4], box[5])

    def move(p):
        s, t, d = st(p)
        return tuple(o + a * s + b * t - n * (d * RELIEF))
    new = patch(bm, stock.faces, box, tag, part, move)
    nb = D(tuple(n))
    for f in new:
        f.normal_update()
        f.smooth = False
    if sum((f.normal * f.calc_area() for f in new), Vector()).dot(nb) < 0.0:
        bmesh.ops.reverse_faces(bm, faces=new)
        for f in new:
            f.normal_update()
    face_out(new, nb)
    return new


def face_out(faces, nb, least=-0.1):
    """Every face of a pressed plate turned to face its plate's way nb (Blender frame): a stock face folded back by the
    press (a cap's or a rivet's back) is culled in the game, a hole you see the ground through."""
    for f in faces:
        if f.is_valid and f.normal.dot(nb) < least:
            f.normal_flip()
            f.normal_update()


def drop(bm, stocks):
    """The copies aside deleted (the passages are done)."""
    gone = [f for s in stocks for f in s.faces if f.is_valid]
    bmesh.ops.delete(bm, geom=gone, context='FACES')
    return len(gone)

