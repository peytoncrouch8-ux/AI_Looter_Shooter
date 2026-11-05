"""The side walks' passages cut and built (looter_tripo_barge_passage measures them; see its docstring for the
whole): carve() cuts one walk's region away (its wall's slot DEPTH deep behind the wall's plates up to LINE_H, the
bulwark's slot DEPTH_B up to its capping, the walk's air from Z0 to AIR_H between the wall's face, WALL_TOL off it over
LINE_H, and the bulwark's face, RAIL_KEEP in from it over its capping) on vertical planes along its straight runs and
level planes over each segment's floor, and caps every cut still in sight flat in its plane (in the paint beside it);
plates_for() lays the wall's plates, the rubbing strake, the head flashing, the bulwark's plates, its capping strip
and the passage's end plates; floors() carries the floor to both plates where the cut took a skirt's foot. Frame: the
barge's, cm, final x."""
import bmesh
import numpy as np

import looter_tripo_barge_clear as clear
import looter_tripo_barge_floors as fl
import looter_tripo_barge_plates as plates
import looter_tripo_barge_stairs as stairs
import looter_tripo_rework as rw
from looter_tripo_barge import PASSAGE        # the face tag of every plate
from looter_tripo_rework import B, D, U, ue_centre

# --- The cut (cm over the segment's floor; |y| off the lines) ---
LINE_H = 200.0                         # the wall's plates' height (over the shards, under the portholes)
DEPTH = 45.0                           # the wall's foot cut away behind its plates (> barge_paint.THIN 35 and the
                                       # plates' pressed relief: they are walls)
DEPTH_B = 6.0                          # the bulwark's inner face cut away behind its plates
FLOOR_CUT = 4.0                        # the slots behind the plates go this far under the floor (the plates hide it)
Z0 = 0.5                               # the walk's air starts this far over its floor
AIR_H = LINE_H                         # and is cleared up to the plates' top only: over it the corridors cleared the
                                       # walks to 222 and their caps close the shells there (cut away at 240, the
                                       # casing's ledges stood open over the walk, the floor under them read as inside
                                       # and the capsule hit the bulwark; cut at 215, their caps took the headroom to 219)
WALL_TOL = {'house': 40.0, 'casing': 25.0}  # over LINE_H what stands this far out of the wall's face is cut, flat: the
                                       # pipe roots' and the corridors' leftovers; the wall's own relief and the casing's
                                       # round aft end, bulging over the plates, stay uncut (a tighter cut left the wall
                                       # over the flashing full of capped notches)
RAIL_KEEP = 30.0                       # over its capping the bulwark keeps this much inboard (capping, posts, rails)
FLOOR_UP = 1.0                         # cm: a floor face (up, at most this over its floor) is never cut
CAP_ABOVE = 18.0                       # cm over the bulwark's plates' top: its capping's top; what stands on it that an
                                       # earlier cut left (a strap's or a horn pipe's stub, a corridor's fin: a piece
                                       # with a fill face) goes, cut there; posts and rails stay
RAIL_OUT = 150.0                       # cm outboard of the bulwark's face such stubs are looked for (a piece reaching on
                                       # past it, or along to what is not cut, stays)
SPAN = 0.05                            # cm: a face reaching this far past a plane both ways is cut on it
ON = rw.EPS * 100.0                    # cm: a cut's edge this near its plane lies on it
# --- The plates ---
SINK = 4.0                             # cm: the plates reach under the floor
STRAKE = (95.0, 12.0, 5.0)             # the rubbing strake: its middle over the floor, tall, proud of the plates
FLASH = (6.0, 11.0, 2.0)               # the head flashing over LINE_H: tall, proud (over what the cut left there), into
                                       # the plates; past it, to WALL_TOL, the cut is capped
BAR_OVER = 2.0                         # cm: the strake and the flashing run on past each bend (no gap at its outer corner)
CAP_STRIP = (2.0, DEPTH_B + 6.0, 0.3)  # the bulwark's capping strip: in from its plates, out over the slot, over the top
STRIP_OVER = 3.0                       # cm: a floor strip runs this far under the plates and over the floor beside it
STRIP_UP = -0.3                        # cm: under the floor's top (no fighting where it overlaps; laid over it, the
                                       # strips lifted the walk's floor and the casing walks' headroom read 219)
PLATE_FLAT = 3.0                       # degrees: a plate's faces this near one plane (and one paint) are one face
                                       # (Tripo's plating comes in rivet-dense triangles; its relief is pressed flat)


def _v(p):
    return np.array(p, dtype=float)


def _line(g, L, s, c):
    """The vertical plane |y| = L(x) + c over segment g on side s: (a point, its unit normal toward +y)."""
    xa, xb = g['xa'], g['xb']
    m = (L(xb) - L(xa)) / (xb - xa)
    n = _v((-s * m, 1.0, 0.0))
    return _v((xa, s * (L(xa) + c), 0.0)), n / np.linalg.norm(n)


def _dist(f, co, n):
    return [float(np.dot(_v(U(v.co)) - co, n)) for v in f.verts]


def _spans(f, co, n):
    d = _dist(f, co, n)
    return min(d) < -SPAN and max(d) > SPAN


def _bisect(bm, faces, co, n):
    faces = list(faces)
    edges = {e for f in faces for e in f.edges}
    verts = {v for f in faces for v in f.verts}
    made = bmesh.ops.bisect_plane(bm, geom=faces + list(edges) + list(verts), dist=rw.BISECT_DIST, plane_co=B(tuple(co)),
                                  plane_no=D(tuple(n)).normalized())
    return [g for g in made['geom'] if isinstance(g, bmesh.types.BMFace)]


def _zr(f, g):
    zs = [U(v.co)[2] - g['T'] for v in f.verts]
    return min(zs), max(zs)


def _yr(f, s):
    ys = [s * U(v.co)[1] for v in f.verts]
    return min(ys), max(ys)


def _planes(g, w, s):
    """Segment g's cut planes: (point, normal, which faces: test(face)) for the vertical planes along the lines (each
    over the heights it bounds) and the level ones (each over the |y| it bounds)."""
    yw, yb, T, bt, tol = w['yw'], w['yb'], g['T'], g['bt'], WALL_TOL[w['name']]
    fc = T - g['Tmin'] + FLOOR_CUT
    up = w['H'] - T                    # the heights over the walk's own floor level, the cut's floor over the segment's
    line, top, air = LINE_H + up, bt + up, AIR_H + up
    out = []
    for L, c, z0, z1 in ((yw, -DEPTH, -fc, line), (yw, 0.0, -fc, Z0 + 1.0), (yw, tol, line, air),
                         (yb, DEPTH_B, -fc, top), (yb, 0.0, -fc, Z0 + 1.0), (yb, -RAIL_KEEP, top, air)):
        if z1 <= z0 + 0.5:
            continue                    # nothing over the plates is cut (AIR_H = LINE_H)
        co, n = _line(g, L, s, c)
        out.append((co, n, lambda f, z0=z0, z1=z1: _zr(f, g)[1] > z0 - 1.0 and _zr(f, g)[0] < z1 + 1.0))
    for z, lo, hi in ((-fc, lambda x: yw(x) - DEPTH, lambda x: yb(x) + DEPTH_B), (Z0, yw, yb),
                      (line, lambda x: yw(x) - DEPTH, lambda x: yw(x) + tol),
                      (top, lambda x: yb(x) - RAIL_KEEP, lambda x: yb(x) + DEPTH_B),
                      (air, lambda x: yw(x) + tol, lambda x: yb(x) - RAIL_KEEP)):
        def test(f, lo=lo, hi=hi):
            x = ue_centre(f)[0]
            a, b = _yr(f, s)
            return b > lo(x) - 1.0 and a < hi(x) + 1.0
        out.append((_v((0.0, 0.0, T + z)), _v((0.0, 0.0, 1.0)), test))
    return out


def _inside(f, g, w, s):
    """Whether face f (its middle) lies in the region segment g cuts away."""
    x, y, z = ue_centre(f)
    if not g['xa'] <= x <= g['xb'] or y * s <= 0.0:
        return False
    ay, zr = abs(y), z - g['T']
    yw, yb, bt = w['yw'](x), w['yb'](x), g['bt']
    fc = g['T'] - g['Tmin'] + FLOOR_CUT
    up = w['H'] - g['T']
    line, bt, air = LINE_H + up, bt + up, AIR_H + up
    if yw - DEPTH < ay < yw and -fc < zr < line:
        return True
    if yb < ay < yb + DEPTH_B and -fc < zr < bt:
        return True
    if not Z0 < zr < air:
        return False
    if f.normal.z > fl.DECK_UP and zr < FLOOR_UP:
        return False                      # a floor's face
    lo = yw if zr < line else yw + WALL_TOL[w['name']]
    hi = yb if zr < bt else yb - RAIL_KEEP
    return lo < ay < hi


def carve(bm, tag, nearest, fill, w):
    """One walk's region cut away (see the docstring); returns (faces cut away, cap faces)."""
    s, segs, yw, yb = w['s'], w['segs'], w['yw'], w['yb']
    xa0, xb0 = segs[0]['xa'], segs[-1]['xb']
    ylo = min(yw(x) for x in w['xs']) - DEPTH - 2.0
    yhi = max(yb(x) for x in w['xs']) + DEPTH_B + 2.0
    zlo = min(g['Tmin'] for g in segs) - FLOOR_CUT - 2.0
    zhi = w['H'] + AIR_H + 2.0

    def near(f):
        ps = [U(v.co) for v in f.verts]
        return (max(p[0] for p in ps) > xa0 - 1.0 and min(p[0] for p in ps) < xb0 + 1.0
                and max(s * p[1] for p in ps) > ylo and min(s * p[1] for p in ps) < yhi
                and max(p[2] for p in ps) > zlo and min(p[2] for p in ps) < zhi)
    faces = [f for f in bm.faces if near(f)]
    for x in sorted({g['xa'] for g in segs} | {g['xb'] for g in segs}):
        cut = rw.spanning(bm, tag, None, 0, x, faces=faces)
        if cut:
            gone = set(cut)
            faces = list(dict.fromkeys([f for f in faces if f not in gone and f.is_valid] + rw.bisect(bm, cut, 0, x)))
    doomed = []
    for g in segs:
        mine = [f for f in faces if f.is_valid and g['xa'] - SPAN <= ue_centre(f)[0] <= g['xb'] + SPAN]
        for co, n, test in _planes(g, w, s):
            cut = [f for f in mine if f.is_valid and _spans(f, co, n) and test(f)]
            if cut:
                gone = set(cut)
                mine = list(dict.fromkeys([f for f in mine if f not in gone and f.is_valid] + _bisect(bm, cut, co, n)))
        doomed += [f for f in mine if f.is_valid and _inside(f, g, w, s)]
        faces = [f for f in faces if f.is_valid]
    doomed = list(dict.fromkeys(doomed))
    rim = {e for f in doomed for e in f.edges}
    bmesh.ops.delete(bm, geom=doomed, context='FACES')
    rim = [e for e in rim if e.is_valid and e.is_boundary]
    caps = _caps(bm, tag, nearest, fill, w, rim)
    gone, stub_caps = _stubs(bm, tag, nearest, fill, w)
    return len(doomed) + gone, caps + stub_caps


def _caps(bm, tag, nearest, fill, w, rim):
    """The cuts still in sight closed flat in their planes: the wall's face over LINE_H, the bulwark's keep over its
    capping, the air's top, the capping's foot, the passage's two ends."""
    s, segs = w['s'], w['segs']
    yw, yb = w['yw'], w['yb']
    made = 0
    for g in segs:
        T, up = g['T'], w['H'] - g['T']
        line, bt, air = LINE_H + up, g['bt'] + up, AIR_H + up
        inx = lambda e, g=g: all(g['xa'] - ON <= U(v.co)[0] <= g['xb'] + ON for v in e.verts)
        zin = lambda e, a, b, T=T: all(a - ON <= U(v.co)[2] - T <= b + ON for v in e.verts)
        yin = lambda e, lo, hi: all(lo(U(v.co)[0]) - ON <= s * U(v.co)[1] <= hi(U(v.co)[0]) + ON for v in e.verts)
        on = lambda e, co, n: all(abs(d) < ON for d in [float(np.dot(_v(U(v.co)) - co, n)) for v in e.verts])
        jobs = []
        tol = WALL_TOL[w['name']]
        co, n = _line(g, yw, s, tol)
        jobs.append(([e for e in rim if e.is_valid and inx(e) and on(e, co, n) and zin(e, line, air)], s * n))
        co, n = _line(g, yb, s, -RAIL_KEEP)
        jobs.append(([e for e in rim if e.is_valid and inx(e) and on(e, co, n) and zin(e, bt, air)], -s * n))
        for z, lo, hi in ((air, lambda x: yw(x) + tol, lambda x: yb(x) - RAIL_KEEP),
                          (line, lambda x: yw(x) + FLASH[1], lambda x: yw(x) + tol),
                          (bt, lambda x: yb(x) - RAIL_KEEP, lambda x: yb(x) + DEPTH_B)):
            zc = _v((0.0, 0.0, T + z))
            jobs.append(([e for e in rim if e.is_valid and inx(e) and on(e, zc, _v((0, 0, 1))) and yin(e, lo, hi)],
                         _v((0.0, 0.0, -1.0))))
        for edges, facing in jobs:
            if edges:
                made += clear._close(bm, tag, nearest, fill, edges, tuple(facing))
    for x, out in ((segs[0]['xa'], 1.0), (segs[-1]['xb'], -1.0)):
        edges = [e for e in rim if e.is_valid and all(abs(U(v.co)[0] - x) < ON for v in e.verts)]
        if edges:
            made += clear._close(bm, tag, nearest, fill, edges, (out, 0.0, 0.0))
    return made


def _stubs(bm, tag, nearest, fill, w):
    """What an earlier cut left standing on the bulwark's capping (see CAP_ABOVE) taken off at the capping's top, the
    cut closed facing up; returns (faces gone, cap faces)."""
    s, segs, yb = w['s'], w['segs'], w['yb']
    xa0, xb0 = segs[0]['xa'], segs[-1]['xb']
    z0, z1 = w['H'] + segs[0]['bt'] + CAP_ABOVE, w['H'] + AIR_H + 60.0

    def band(ps):
        x = sum(p[0] for p in ps) / len(ps)
        ys = [s * p[1] for p in ps]
        return (max(p[0] for p in ps) > xa0 and min(p[0] for p in ps) < xb0 and max(ys) > yb(x) - RAIL_KEEP - 5.0
                and min(ys) < yb(x) + RAIL_OUT and max(p[2] for p in ps) > z0 and min(p[2] for p in ps) < z1)
    faces = [f for f in bm.faces if band([U(v.co) for v in f.verts])]
    plane, up = _v((0.0, 0.0, z0)), _v((0.0, 0.0, 1.0))
    cut = [f for f in faces if _spans(f, plane, up)]
    if cut:
        gone = set(cut)
        faces = list(dict.fromkeys([f for f in faces if f not in gone and f.is_valid] + _bisect(bm, cut, plane, up)))
    inside = lambda f: (xa0 < ue_centre(f)[0] < xb0 and ue_centre(f)[2] > z0 and band([U(v.co) for v in f.verts])
                        and all(xa0 <= U(v.co)[0] <= xb0 and U(v.co)[2] < z1 for v in f.verts))
    zone = {f for f in faces if f.is_valid and inside(f)}
    on_cut = lambda e: all(abs(U(v.co)[2] - z0) < ON for v in e.verts)
    doomed, seen = [], set()
    for f in zone:
        if f in seen:
            continue
        piece, todo, whole = [], [f], True
        seen.add(f)
        while todo:
            g = todo.pop()
            piece.append(g)
            for e in g.edges:
                for h in e.link_faces:
                    if h in zone:
                        if h not in seen:
                            seen.add(h)
                            todo.append(h)
                    elif not on_cut(e):
                        whole = False        # it reaches on out of the zone: not a stub
        if whole and any(g[tag] == fill for g in piece):
            doomed += piece
    rim = {e for f in doomed for e in f.edges}
    bmesh.ops.delete(bm, geom=doomed, context='FACES')
    edges = [e for e in rim if e.is_valid and e.is_boundary and on_cut(e)]
    made = clear._close(bm, tag, nearest, fill, edges, (0.0, 0.0, 1.0)) if edges else 0
    return len(doomed), made


def plates_for(bm, tag, w, band):
    """The walk's plates (see the docstring); returns the faces made."""
    s, segs, yw, yb = w['s'], w['segs'], w['yw'], w['yb']
    stock = w['wall_stock']
    tall, proud = STRAKE[1], STRAKE[2]
    f_tall, f_out, f_in = FLASH
    c_in, c_out, c_up = CAP_STRIP
    made = []
    for g in segs:
        xa, xb, H, Tmin, bt = g['xa'], g['xb'], w['H'], g['Tmin'], g['bt']
        wa, wb, ba, bb = yw(xa), yw(xb), yb(xa), yb(xb)
        a_w = _v((xb - xa, s * (wb - wa), 0.0))
        a_b = _v((xb - xa, s * (bb - ba), 0.0))
        out_w = _v((-(wb - wa) / (xb - xa), s, 0.0))
        out_w /= np.linalg.norm(out_w)
        in_b = _v(((bb - ba) / (xb - xa), -s, 0.0))
        in_b /= np.linalg.norm(in_b)
        foot = Tmin - SINK
        o_w = _v((xa, s * wa, foot))
        made += plates.plate(bm, tag, PASSAGE, stock, o_w, a_w, (0.0, 0.0, H + LINE_H - foot), out_w)
        u = a_w / np.linalg.norm(a_w)
        e0, e1 = (0.0 if g is segs[0] else BAR_OVER), (0.0 if g is segs[-1] else BAR_OVER)
        o_bar, a_bar = o_w - u * e0, a_w + u * (e0 + e1)
        o = o_bar - out_w + _v((0.0, 0.0, H + STRAKE[0] - tall / 2.0 - foot))
        made += stairs.bar(bm, tag, band, o, a_bar, (0.0, 0.0, tall), out_w * (proud + 1.0))
        o = o_bar - out_w * f_in + _v((0.0, 0.0, H + LINE_H - f_tall / 2.0 - foot))
        made += stairs.bar(bm, tag, band, o, a_bar, (0.0, 0.0, f_tall), out_w * (f_in + f_out))
        o_b = _v((xa, s * ba, foot))
        made += stairs.rect(bm, tag, band, 'band', o_b, a_b, (0.0, 0.0, H + bt - foot), in_b)
        o = o_b + in_b * c_in + _v((0.0, 0.0, H + bt + c_up - foot))
        made += stairs.rect(bm, tag, band, 'cap', o, a_b, -in_b * (c_in + c_out), (0.0, 0.0, 1.0))
    # The ends: the slots behind the plates closed.
    for g, x, out in ((segs[0], segs[0]['xa'], -1.0), (segs[-1], segs[-1]['xb'], 1.0)):
        foot = g['Tmin'] - SINK
        n = (out, 0.0, 0.0)
        made += plates.plate(bm, tag, PASSAGE, stock, (x, s * (yw(x) - DEPTH), foot), (0.0, s * DEPTH, 0.0),
                             (0.0, 0.0, w['H'] + LINE_H - foot), n)
        made += stairs.rect(bm, tag, band, 'band', (x, s * yb(x), foot), (0.0, s * DEPTH_B, 0.0),
                            (0.0, 0.0, w['H'] + g['bt'] - foot), n)
    for f in made:
        if f.is_valid:
            f[tag] = PASSAGE
    return len(made)


def floors(bm, tag, walks):
    """Where the cut took a skirt's foot the floor carried on to the plates (strips of the walk's own floor at each
    station); returns the faces made."""
    bm.normal_update()
    table = fl.trenches(bm)
    step = fl.WALK_STEP
    made = []
    for w in walks:
        s = w['s']
        for x in w['xs']:
            t = table.get((s, int(round(x / step))))
            if t is None:
                continue
            lo, hi, _ = t
            top = min(t[2], w['top'](x))      # the floor as read before the cut (after it, a stub's foot read 1-3 cm over
            yw, yb = w['yw'](x), w['yb'](x)    # it: strips laid there took the casing walks' headroom to 219)
            for y0, y1 in ((yw - STRIP_OVER, lo + STRIP_OVER), (hi - STRIP_OVER, yb + STRIP_OVER)):
                if y1 - y0 > 3.0 * STRIP_OVER:
                    made += plates.plate(bm, tag, PASSAGE, w['floor_stock'], (x - step / 2.0, s * y0, top + STRIP_UP),
                                         (step, 0.0, 0.0), (0.0, s * (y1 - y0), 0.0), (0.0, 0.0, 1.0))
    for f in made:
        if f.is_valid:
            f[tag] = PASSAGE
    return len(made)


def flatten(bm, tag):
    """Every plate's faces that lie in one plane within PLATE_FLAT and one stretch of paint merged (the rework
    triangulates them again). Returns (faces before, after)."""
    faces = [f for f in bm.faces if f[tag] == PASSAGE]
    before = len(faces)
    bmesh.ops.dissolve_limit(bm, angle_limit=np.radians(PLATE_FLAT), use_dissolve_boundaries=False,
                             verts=list({v for f in faces for v in f.verts}), edges=list({e for f in faces for e in f.edges}),
                             delimit={'UV'})
    return before, sum(1 for f in bm.faces if f[tag] == PASSAGE)
