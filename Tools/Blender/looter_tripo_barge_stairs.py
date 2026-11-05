"""The forecastle's stairs (looter_tripo_barge.rework, step 5): two flights from the well deck up Tripo's 1.2 m step to
the scoop's floor, every face one of Tripo's own with its paint: treads of the bulwark's capping laid flat, each front
edge a capping nosing standing proud of a kick plate of the bulwark's riveted band, side stringers of the band along
the slope over stepped side plates, a handrail on posts along both open sides; the step's own face clad in kick plates
of the band where the flights leave it bare. Every piece is cut from the well deck's +Y bulwark (its outer face's
riveted band, its capping's top; STOCK, past the jumbo's start; the inner face is near black), each from a stretch
further along (no two pieces the same rivets), its relief pressed to RELIEF of its depth (flat, the band's rounded top
crossed a bar's sides into an X). The stairs' collision is the ramps' (looter_tripo_barge_collision), as before.
"""
import bmesh
import numpy as np
from mathutils import Vector

from looter_tripo_barge import (FORE_CUT, FORECASTLE, STAIR_STEPS, STAIR_Y, STAIRS, TREAD, WELL, final_x, log,
                                terrace_aft)
from looter_tripo_rework import D, U, patch, ue_centre, ue_normal

NOSING = (5.0, 2.0, 4.0)               # cm: each tread's front edge: tall, proud of its riser, deep
RISER_BACK = 0.5                       # cm: the kick plates stand this far aft of the treads' front (the top one off
                                       # the step's face: one plane, they flickered)
STRINGER = (10.0, 32.0, 4.0)           # cm: over the nosings' line, under it, thick
RAIL = (92.0, 6.0, 5.0, 3)             # cm: the handrail over the nosings' line, its bar's side, its posts' side; posts
KICK = (1.0, 2.0, 2)                   # cm: the step face's kick plates stand off it, reach into it; rows
STOCK = (200.0, 800.0)                 # cm fore of the jumbo's start: the +Y bulwark the pieces are cut from
BAND_Z = (WELL + 8.0, 2330.0)          # its outer face's riveted band (under the capping, over the strake)
CAP_Z, CAP_UP = (2320.0, 2370.0), 0.85 # its capping's top
FACING = 0.8                           # a face of the band faces outboard this square; of the step's face, aft
CAP_SHARE = (15.0, 85.0)               # percentiles: the capping's top across, its dark rounded edges left out
STEP_REACH = (40.0, 900.0)             # cm: the step's face is looked for this far from the forecastle's aft edge, |y|
RELIEF = 0.25                          # of a piece's source depth kept (its rivets and plate edges, shallow)
WELD = 0.001                           # m: a bar's faces' corners merged
MOVE_ON = (0.61, 17.0)                 # the stock read on by this share of each piece's length plus this (cm)
SOURCE_MIN = 12.0                      # cm: the least piece of the stock a rectangle is cut from, either way


class Stock:
    """The well deck's +Y bulwark as plates: its outer face's band (along x by z) and its capping's top (along x by
    y), read from a place that moves on with every piece."""

    def __init__(self, bm):
        self.x0, self.x1 = final_x(FORE_CUT) + STOCK[0], final_x(FORE_CUT) + STOCK[1]
        self.faces = [f for f in bm.faces if self.x0 - 50.0 < ue_centre(f)[0] < self.x1 + 50.0
                      and ue_centre(f)[1] > 500.0]
        band = [U(v.co)[1] for f in self.faces if ue_normal(f)[1] > FACING
                and BAND_Z[0] < ue_centre(f)[2] < BAND_Z[1] for v in f.verts]
        cap = [U(v.co) for f in self.faces if f.normal.z > CAP_UP and CAP_Z[0] < ue_centre(f)[2] < CAP_Z[1]
               for v in f.verts]
        if not band or not cap:
            raise ValueError('the stairs\' stock: no band or capping on the +Y bulwark')
        self.outer = float(np.percentile(band, 95))
        ys = [p[1] for p in cap]
        self.cap = (float(np.percentile(ys, CAP_SHARE[0])), float(np.percentile(ys, CAP_SHARE[1])),
                    float(np.median([p[2] for p in cap])))
        self.at = 0.0
        log(f'stairs\' stock: x {self.x0:.0f}..{self.x1:.0f}, the band\'s face at y {self.outer:.0f}, the capping '
            f'y {self.cap[0]:.0f}..{self.cap[1]:.0f} at z {self.cap[2]:.0f}')

    def take(self, length):
        """Where along the stock (x) the next piece, length cm long, is cut."""
        span = self.x1 - self.x0
        if length > span:
            raise ValueError(f'a stair piece {length:.0f} cm long: the stock is {span:.0f}')
        if self.at + length > span:
            self.at = (self.at * MOVE_ON[0]) % max(span - length, 1.0)
        x = self.x0 + self.at
        self.at += length * MOVE_ON[0] + MOVE_ON[1]
        return x


def _v(p):
    return np.array(p, dtype=float)


def rect(bm, tag, stock, kind, o, a, b, n):
    """A rectangle o + s a + t b (s, t in 0..1; cm) facing n, cut from the stock: kind 'band' (a along its x, b up) or
    'cap' (a along its x, b across). Longer than the stock is high or wide, it stretches. Returns its faces."""
    o, a, b, n = _v(o), _v(a), _v(b), _v(n) / np.linalg.norm(n)
    # The source is never thinner than SOURCE_MIN (a bar's 3 cm side cut from 3 cm of the stock was all slivers: its
    # corners had no tangents in Unreal); a narrow face takes a wider piece, pressed.
    la, lb = max(float(np.linalg.norm(a)), SOURCE_MIN), max(float(np.linalg.norm(b)), SOURCE_MIN)
    sx = stock.take(la)
    if kind == 'band':
        lb = min(lb, BAND_Z[1] - BAND_Z[0])
        box = (sx, sx + la, stock.outer - 40.0, stock.outer + 5.0, BAND_Z[0], BAND_Z[0] + lb)
        st = lambda p: ((p[0] - sx) / la, (p[2] - BAND_Z[0]) / lb, stock.outer - p[1])   # depth: into the bulwark
    else:
        lb = min(lb, stock.cap[1] - stock.cap[0])
        box = (sx, sx + la, stock.cap[0], stock.cap[0] + lb, CAP_Z[0], CAP_Z[1])
        st = lambda p: ((p[0] - sx) / la, (p[1] - stock.cap[0]) / lb, stock.cap[2] - p[2])  # depth: under its top

    def move(p):
        s, t, d = st(p)
        return tuple(o + a * s + b * t - n * (d * RELIEF))
    new = patch(bm, stock.faces, box, tag, STAIRS, move)
    # Turned to face n as one piece (face by face, a pressed rivet's sides went either way: their corners met with
    # opposite tangents, Unreal's zero tangents).
    nb = D(tuple(n))
    for f in new:
        f.normal_update()
        f.smooth = False
    if sum((f.normal * f.calc_area() for f in new), Vector()).dot(nb) < 0.0:
        bmesh.ops.reverse_faces(bm, faces=new)
        for f in new:
            f.normal_update()
    return new


def bar(bm, tag, stock, o, a, b, c, kinds=('band', 'cap', 'cap')):
    """A closed bar: corner o, edges a (its length), b, c (cm); kinds: its two a-b sides, its two a-c sides, its ends.
    Returns its faces."""
    o, a, b, c = _v(o), _v(a), _v(b), _v(c)
    u = lambda w: w / np.linalg.norm(w)
    faces = []
    for base, e1, e2, out, kind in ((o, a, b, -u(c), kinds[0]), (o + c, a, b, u(c), kinds[0]),
                                    (o, a, c, -u(b), kinds[1]), (o + b, a, c, u(b), kinds[1]),
                                    (o, b, c, -u(a), kinds[2]), (o + a, b, c, u(a), kinds[2])):
        faces += rect(bm, tag, stock, kind, base, e1, e2, out)
    bmesh.ops.remove_doubles(bm, verts=list({v for f in faces if f.is_valid for v in f.verts}), dist=WELD)
    return [f for f in faces if f.is_valid]


def build(bm, tag):
    """Both flights and the step's face; returns the number of faces made."""
    aft = terrace_aft(bm)
    rise = (FORECASTLE - WELL) / STAIR_STEPS
    stock = Stock(bm)
    line = lambda x: FORECASTLE - (aft - x) * rise / TREAD          # the nosings' line
    tall, proud, deep = NOSING
    over, under, thick = STRINGER
    height, side, post, posts = RAIL
    made = []
    for y0, y1 in STAIR_Y:
        w = y1 - y0
        for k in range(1, STAIR_STEPS + 1):
            xk, zk = aft - TREAD * (STAIR_STEPS - k), WELL + rise * k
            made += rect(bm, tag, stock, 'band', (xk - RISER_BACK, y0, zk - rise), (0.0, w, 0.0), (0.0, 0.0, rise),
                         (-1.0, 0.0, 0.0))
            made += bar(bm, tag, stock, (xk - proud, y0, zk - tall), (0.0, w, 0.0), (0.0, 0.0, tall + 0.3),
                        (deep, 0.0, 0.0), kinds=('cap', 'cap', 'cap'))
            if k == STAIR_STEPS:
                break
            made += rect(bm, tag, stock, 'cap', (xk, y0, zk), (0.0, w, 0.0), (TREAD, 0.0, 0.0), (0.0, 0.0, 1.0))
            # The side plates under the stringers (their stepped tops hidden behind them).
            h = zk - under + 6.0 - WELL
            for y, out in ((y0, -1.0), (y1, 1.0)):
                if h > 1.0:
                    made += rect(bm, tag, stock, 'band', (xk, y, WELL), (TREAD, 0.0, 0.0), (0.0, 0.0, h), (0.0, out, 0.0))
        # Stringers, posts and a handrail on both sides, outside the treads.
        foot = aft - (FORECASTLE - WELL + over) * TREAD / rise          # where the stringer's top edge meets the deck
        r0, r1 = aft - TREAD * (STAIR_STEPS - 0.5), aft
        for y, out in ((y0, -1.0), (y1, 1.0)):
            ya = y if out > 0 else y - thick
            made += bar(bm, tag, stock, (foot, ya, line(foot) - under), (aft - foot, 0.0, line(aft) - line(foot)),
                        (0.0, 0.0, over + under), (0.0, thick, 0.0))
            yc = y + out * thick / 2.0
            made += bar(bm, tag, stock, (r0, yc - side / 2.0, line(r0) + height - side),
                        (r1 - r0, 0.0, line(r1) - line(r0)), (0.0, 0.0, side), (0.0, side, 0.0))
            for x in np.linspace(r0 + post, r1 - post, posts):
                z0 = line(x) + over
                made += bar(bm, tag, stock, (x - post / 2.0, yc - post / 2.0, z0),
                            (0.0, 0.0, line(x) + height - side - z0), (post, 0.0, 0.0), (0.0, post, 0.0))
    made += _step_face(bm, tag, stock, aft)
    log(f'stairs: two flights of {STAIR_STEPS} steps ({rise:.1f} cm) up to the forecastle\'s aft face x {aft:.0f}: '
        f'capping treads and nosings, band kick plates, stringers and handrails; {len(made)} faces of the +Y bulwark')
    return len(made)


def _step_face(bm, tag, stock, aft):
    """The step's face clad where the flights leave it bare: band kick plates KICK[0] off it, in KICK[2] rows."""
    ys = [abs(U(v.co)[1]) for f in bm.faces if abs(ue_centre(f)[0] - aft) < STEP_REACH[0]
          and WELL < ue_centre(f)[2] < FORECASTLE and ue_normal(f)[0] < -FACING and abs(ue_centre(f)[1]) < STEP_REACH[1]
          for v in f.verts]
    half = max(ys) if ys else 600.0
    thick = STRINGER[2]
    inner = min(abs(y) for pair in STAIR_Y for y in pair) - thick
    outer = max(abs(y) for pair in STAIR_Y for y in pair) + thick
    off, into, rows = KICK
    h = (FORECASTLE - WELL) / rows
    made = []
    for ya, yb in ((-inner, inner), (outer, half), (-half, -outer)):
        if yb - ya < 5.0:
            continue
        for r in range(rows):
            made += bar(bm, tag, stock, (aft - off, ya, WELL + h * r), (0.0, yb - ya, 0.0), (0.0, 0.0, h - 0.2),
                        (off + into, 0.0, 0.0))
    log(f'stairs: the step\'s face clad |y| {inner:.0f} and {outer:.0f}..{half:.0f}, {rows} rows')
    return made
