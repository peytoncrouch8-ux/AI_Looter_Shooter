"""The side walks made ships' passages (looter_tripo_barge.rework, after both jumbos: final x; round 6, the user's main
complaint: jagged black and brown shards at a pawn's eye along all four walks). Each walk (the casing's and the
house's, both sides) is measured station by station: its wall's face (WALL_REF: a low share of the rays that meet a
face square to y, so pipe roots standing out and notches cut in read as neither) and its bulwark's inner face
(BULWARK_REF), each smoothed (LINE_MEDIAN) and straightened (Douglas-Peucker to LINE_TOL) into straight runs that meet
where the house steps. Between them, from the floor up to AIR_H, everything Tripo's squeeze and the corridors left
(the skirt at the wall's foot, the horn pipes' roots and ends, the corridors' caps) is cut away: the wall's foot DEPTH
deep behind its new plates up to LINE_H, the bulwark's inner face DEPTH_B up to under its capping, what stands over
those (pipe roots, the corridors' leftovers) flush with the wall (WALL_TOL) and RAIL_KEEP inboard of the bulwark (its
capping, posts and rails stay); each visible cut capped flat in its plane (looter_tripo_barge_cut). Then the passage
is built of Tripo's own plates (looter_tripo_barge_plates; their paint comes with them): the wall carried plumb down
to the floor in plates of its own plating (clean stretches of the same walk's wall foot, read on piece by piece), a
rubbing strake (STRAKE) and a head flashing (FLASH) over the joint at LINE_H of the bulwark's band and capping, the
bulwark's inner face in plates of its riveted band up to its capping with a strip of capping over the joint, the
passage's ends closed, and the floor carried to both plates where the cut took a skirt's foot. The wall's plates stand
DEPTH (more than the REBAKE lane's THIN) clear of anything behind them, so looter_tripo_barge_paint.walls takes them
for the Walls set and its paint."""
import json
import os

import numpy as np
from mathutils.bvhtree import BVHTree

import looter_tripo_barge as barge
import looter_tripo_barge_floors as fl
import looter_tripo_barge_plates as plates
import looter_tripo_barge_stairs as stairs
from looter_tripo_rework import B, D, U

# --- Where (final x) ---
CASING_FROM = -3050.0                  # the casing's walk is a passage fore of its round aft end (a deck aft of it)
WIDE_MAX = 400.0                       # cm: a station wider than this between its wall and bulwark is deck, not passage
                                       # (the house's aft end: 370)
NARROW_MIN = 60.0                      # cm: and narrower than this a misread
GAP_FILL = 16                          # stations with no line read (a pipe's root over them, a door), between ones with:
                                       # their lines carried across
EDGE_MEDIAN = 3                        # stations each side a walk's floor edges are the median over (as the check)
# --- The lines ---
FAR = 6.0                              # m: a ray across a walk
FACE_Y = 0.8                           # a ray's hit counts when its face is this square to y and faces the walk
WALL_REF = tuple(range(20, 221, 20))   # cm over the floor: the wall's face read here (the plates' height and over)
WALL_PCT = 30.0                        # its percentile: under it pipe roots and skirts standing out, over it notches
WALL_FOUND = 5                         # rays of WALL_REF that must meet the wall square
BULWARK_REF = (10.0, 20.0, 30.0, 40.0, 50.0)
BULWARK_PCT = 70.0                     # the bulwark's inner face: the outer of its hits (shards stand inboard of it)
TOP_REF = tuple(range(20, 121, 5))     # cm: where the bulwark's inner face may end under its capping
TOP_NEAR, TOP_FACE = 6.0, 0.7          # a hit within this of the face's line, this square: still the inner face
TOP_PCT, TOP_POST = 75.0, 100.0        # a walk's bulwark top: this percentile of its stations' (up under the capping; one
                                       # height end to end: per segment its capping strip stepped), a post's over this
LINE_MEDIAN = 8                        # stations each side of the lines' running median (a corridor's cap plane over
                                       # 7 stations read as the casing's wall; a median keeps the house's steps)
LINE_TOL = 3.0                         # cm: the straightened lines keep within this of the smoothed ones
TOP_STEP = 2.0                         # cm: a floor stepping more than this between stations starts a segment
SEG_MAX = 400.0                        # cm: the longest segment (its floor and bulwark top read as one)
STOCK_FIT, STOCK_MIN = 2.5, 100.0      # a wall stretch this even (its rays within this of one smooth face) and on its line,
                                       # this long: stock
STOCK_FOUND = 9                        # rays of WALL_REF a stock station's wall must answer
FLOOR_IN = 30.0                        # cm: the floor's stock keeps this far off both plates (they slant)


# --- The seam check (on the finished mesh) ---
SEAM_HEIGHTS = (5.0, 30.0, 60.0, 95.0)  # cm over the floor: rays from the walk's middle at the wall (the user's)
SEAM_BULWARK = (5.0, 30.0)            # and at the bulwark's plates (under its capping)
SEAM_DOWN = (2.0, 6.0, 12.0)          # cm out from each plate: rays down at the seam from SEAM_FROM over the floor
SEAM_FROM, SEAM_FLOOR = 40.0, 15.0    # a down ray must stop over the walk's floor - SEAM_FLOOR (the floor dips and
                                      # rises some 8 cm across a walk: past that it went through)
SEAM_STEP, SEAM_END = 12.5, 8.0       # cm between the rays along x; kept off the passage's ends
SEAM_NEAR = 12.0                      # cm: a ray at a plate that gets past its line by more went through it
SEAM_FACING = 0.2                     # the walk's floor met at its middle faces up this much
SEAM_BACK = -0.35                     # a face met facing the ray less than this is seen from behind: culled in the
                                      # game, a hole the ground shows through (edge-on faces of a bar pass)
PASSAGES = []                         # the walks as the last build cut them (lines, floor level), for check_seams


def log(message):
    print(f'LOOTER: DredgeBarge passages: {message}', flush=True)


def check_seams(obj):
    """Rays from every passage's middle at its wall's plates (SEAM_HEIGHTS), at its bulwark's (SEAM_BULWARK) and down at
    both seams (SEAM_DOWN), every SEAM_STEP along x: none may get through (meet nothing, or meet a face past the
    plate's line by SEAM_NEAR or under the floor by SEAM_FLOOR) or meet a face from behind (SEAM_BACK). Returns (rays
    cast, the misses as (walk, x, what, where met))."""
    import bmesh
    bm = bmesh.new()
    bm.from_mesh(obj.data)
    bm.normal_update()
    tree = BVHTree.FromBMesh(bm)
    rays, misses = 0, []

    def cast(p, d):
        hit = tree.ray_cast(B(p), D(d), FAR)
        if hit[0] is None:
            return None
        facing = -hit[1].dot(D(d).normalized())
        return U(hit[0]), facing
    for w in PASSAGES:
        s, where = w['s'], side_name(w['s'], w['name'])
        for x in np.arange(w['xs'][0] + SEAM_END, w['xs'][-1] - SEAM_END, SEAM_STEP):
            x = float(x) + 0.7                 # off the plates' joints (at a station's x a ray can slip a seam)
            yw, yb = w['yw'](x), w['yb'](x)
            ym = (yw + yb) / 2.0
            got = cast((x, s * ym, w['H'] + 60.0), (0.0, 0.0, -1.0))
            rays += 1
            if got is None or got[1] < SEAM_FACING or got[0][2] < w['H'] - SEAM_FLOOR:
                misses.append((where, round(x), 'the floor at the middle', got and round(got[0][2] - w['H'])))
                continue
            floor = got[0][2]
            for h, line, dy, heights in ((yw, 'wall', -1.0, SEAM_HEIGHTS), (yb, 'bulwark', 1.0, SEAM_BULWARK)):
                for z in heights:
                    got = cast((x, s * ym, floor + z), (0.0, s * dy, 0.0))
                    rays += 1
                    past = got is not None and (abs(got[0][1]) - h) * dy > SEAM_NEAR
                    if got is None or past or got[1] < SEAM_BACK:
                        misses.append((where, round(x), f'{line} at {z:.0f} cm',
                                       None if got is None else (round(abs(got[0][1]) - h), round(got[1], 2))))
                for out in SEAM_DOWN:
                    y = h - dy * out                   # out from the plate into the walk
                    got = cast((x, s * y, floor + SEAM_FROM), (0.0, 0.0, -1.0))
                    rays += 1
                    if got is None or got[0][2] < floor - SEAM_FLOOR or got[1] < SEAM_BACK:
                        misses.append((where, round(x), f'{line} seam {out:.0f} cm out',
                                       None if got is None else (round(got[0][2] - floor), round(got[1], 2))))
    bm.free()
    return rays, misses


def side_name(s, name):
    return f'{"+Y" if s > 0 else "-Y"} {name}'


def runs():
    """{name: (x from, x to)} final x: the casing's and the house's walks (looter_tripo_barge.WALK_RUNS)."""
    c, h = barge.WALK_RUNS['casing'], barge.WALK_RUNS['house']
    return {'casing': (max(c[0], CASING_FROM), barge.final_x(c[1])), 'house': (barge.final_x(h[0]), barge.final_x(h[1]))}


def build(bm, tag, nearest, fill):
    """All four passages; returns {side walk: (x from, x to, segments)} and logs what was cut and made."""
    import looter_tripo_barge_cut as cut     # here: it imports this module's constants
    bm.normal_update()
    table = fl.trenches(bm)
    tree = BVHTree.FromBMesh(bm)
    bm.faces.ensure_lookup_table()
    capped = lambda i: bm.faces[i][tag] == fill       # a corridor's or a walk's cap: no plating stock (its paint is a smear)
    walks = []
    PASSAGES.clear()
    for name, (x0, x1) in runs().items():
        for s in (1, -1):
            got = _passage(table, tree, name, x0, x1, s, capped)
            if got:
                walks.append(got)
                PASSAGES.append(got)
    band = stairs.Stock(bm)
    stocks = []
    for w in walks:
        w['wall_stock'] = _wall_stock(bm, tag, w, walks, fill)
        # The floor's stock: the walk's own floor between its plates, where its wall is clean (its lines read there:
        # read at the passage's start, the casing's narrower aft end, half its pieces were empty).
        stretches = w['clean'] or [(w['xs'][0], w['xs'][-1])]
        xm = sum(stretches[0]) / 2.0
        span = (w['yw'](xm) + FLOOR_IN, w['yb'](xm) - FLOOR_IN)
        w['floor_stock'] = plates.Plating(bm, tag, cut.PASSAGE, 'floor', w['s'], stretches, top=w['top'], y_span=span,
                                          skip=fill)
        stocks += [w['wall_stock'], w['floor_stock']]
    out = {}
    for w in walks:
        gone, caps = cut.carve(bm, tag, nearest, fill, w)
        made = cut.plates_for(bm, tag, w, band) + cut.seals(bm, tag, nearest, w)
        out[side_name(w['s'], w['name'])] = (round(w['xs'][0]), round(w['xs'][-1]), len(w['segs']))
        log(f'{side_name(w["s"], w["name"])}: x {w["xs"][0]:.0f}..{w["xs"][-1]:.0f}, {len(w["segs"])} segments; wall '
            f'|y| {min(w["yw_s"]):.0f}..{max(w["yw_s"]):.0f}, bulwark {min(w["yb_s"]):.0f}..{max(w["yb_s"]):.0f}; {gone} '
            f'faces cut, {caps} cap faces, {made} plate faces; wall stock '
            + ', '.join(f'{a:.0f}..{b:.0f}' for a, b in w['wall_stock'].stretches))
    strips = cut.floors(bm, tag, walks)
    dropped = plates.drop(bm, stocks)
    before, after = cut.flatten(bm, tag)
    if os.environ.get('LOOTER_PASSAGE_DUMP'):
        _holes(bm, tag, walks, cut.PASSAGE)
    log(f'floor strips: {strips} faces; {dropped} stock faces dropped; plate faces merged from {before} to {after}')
    return out


def _holes(bm, tag, walks, part):
    """A probe's: rays from each walk's middle at its wall's plates (every 10 cm, 10..190 cm up): what they meet that
    is not a plate, or a plate behind the line (a hole in the plates), logged."""
    bm.normal_update()
    tree = BVHTree.FromBMesh(bm)
    bm.faces.ensure_lookup_table()
    for w in walks:
        s, bad = w['s'], []
        for x in np.arange(w['xs'][0] + 5.0, w['xs'][-1] - 5.0, 10.0):
            yw, yb = w['yw'](x), w['yb'](x)
            for z in range(10, 191, 20):
                h = _hit(tree, (x, s * (yw + yb) / 2.0, w['H'] + z), (0.0, -s, 0.0))
                if h is None or bm.faces[h[2]][tag] != part or h[0] < yw - 8.0:
                    bad.append((round(float(x)), z, None if h is None else round(h[0]),
                                None if h is None else bm.faces[h[2]][tag]))
        log(f'{side_name(s, w["name"])}: holes probe: {len(bad)} rays of the wall plates miss: {bad[:12]}')


# --- Measuring ---

def _hit(tree, p, d):
    """(|y| of the hit, its face's normal y in the barge's frame, its face's index) of a ray from p (cm) along d, or
    None."""
    hit = tree.ray_cast(B(p), D(d), FAR)
    if hit[0] is None:
        return None
    return abs(U(hit[0])[1]), -hit[1].x, hit[2]


def _passage(table, tree, name, x0, x1, s, capped):
    """One side walk's passage: its stations read, its run, its lines, its segments (a dict), or None."""
    step = fl.WALK_STEP
    width = barge.WALK_RUNS[name][2]
    keys = sorted(k for (sd, k) in table if sd == s and x0 < k * step < x1)
    rows = {}
    for i, k in enumerate(keys):
        near = [table[(s, j)] for j in keys[max(i - EDGE_MEDIAN, 0):i + EDGE_MEDIAN + 1]]
        lo, hi = float(np.median([t[0] for t in near])), float(np.median([t[1] for t in near]))
        top = table[(s, k)][2]
        x = k * step
        ym = fl.walk_middle(lo, hi, width)
        wall, caps = [], 0
        for z in WALL_REF:
            h = _hit(tree, (x, s * ym, top + z), (0.0, -s, 0.0))
            if h and h[1] * s > FACE_Y:
                wall.append((z, h[0]))
                caps += capped(h[2])
        bul = []
        for z in BULWARK_REF:
            h = _hit(tree, (x, s * ym, top + z), (0.0, s, 0.0))
            if h and h[1] * s < -FACE_Y:
                bul.append(h[0])
        yw = float(np.percentile([y for _, y in wall], WALL_PCT)) if len(wall) >= WALL_FOUND else None
        yb = float(np.percentile(bul, BULWARK_PCT)) if bul else None
        bt = None
        if yb is not None:
            for z in TOP_REF:
                h = _hit(tree, (x, s * ym, top + z), (0.0, s, 0.0))
                if h and h[1] * s < -TOP_FACE and abs(h[0] - yb) < TOP_NEAR:
                    bt = float(z)
        ok = yw is not None and yb is not None and bt is not None and NARROW_MIN < yb - yw < WIDE_MAX
        rows[k] = dict(x=x, top=top, yw=yw, yb=yb, bt=bt, ok=ok, wall=wall, bul=bul, ym=ym, caps=caps)
    dump = os.environ.get('LOOTER_PASSAGE_DUMP')
    if dump:                                # the stations as read (a probe's): LOOTER_PASSAGE_DUMP=<folder>
        with open(os.path.join(dump, f'passage_{name}_{s:+d}.json'), 'w') as fh:
            json.dump(rows, fh)
    # The longest run of good stations, gaps of up to GAP_FILL filled across.
    best, cur, gap = [], [], 0
    for k in range(min(keys, default=0), max(keys, default=-1) + 1):
        if k in rows and rows[k]['ok']:
            cur, gap = cur + [k], 0
        elif cur and gap < GAP_FILL:
            gap += 1
            cur.append(k)
        else:
            best, cur, gap = max(best, cur, key=len), [], 0
    best = max(best, cur, key=len)
    while best and not (best[-1] in rows and rows[best[-1]]['ok']):
        best.pop()
    if len(best) < 2 * LINE_MEDIAN + 1:
        log(f'{side_name(s, name)}: no passage read')
        return None
    xs = np.array([k * step for k in best], float)
    good = [k for k in best if k in rows and rows[k]['ok']]
    gx = np.array([rows[k]['x'] for k in good])
    fill_in = lambda key: np.interp(xs, gx, np.array([rows[k][key] for k in good]))
    top = fill_in('top')
    yw_s, yb_s = _smooth(fill_in('yw')), _smooth(fill_in('yb'))
    bw, bb = _dp(xs, yw_s, LINE_TOL), _dp(xs, yb_s, LINE_TOL)
    yw = lambda x, bx=xs[bw], by=yw_s[bw]: float(np.interp(x, bx, by))
    yb = lambda x, bx=xs[bb], by=yb_s[bb]: float(np.interp(x, bx, by))
    tops = [rows[k]['bt'] for k in good if rows[k]['bt'] <= TOP_POST]
    bt = float(np.percentile(tops, TOP_PCT)) - 1.0
    # Segments: where either line bends, where the floor steps, no longer than SEG_MAX.
    cuts = sorted(set(bw) | set(bb) | {i for i in range(1, len(xs)) if abs(top[i] - top[i - 1]) > TOP_STEP})
    segs = []
    for i, j in zip(cuts, cuts[1:]):
        n = max(1, int(np.ceil((xs[j] - xs[i]) / SEG_MAX)))
        edges = np.linspace(xs[i], xs[j], n + 1)
        for a, b in zip(edges, edges[1:]):
            inside = (xs >= a - 1.0) & (xs <= b + 1.0)
            segs.append(dict(xa=float(a), xb=float(b), T=float(top[inside].max()), Tmin=float(top[inside].min()),
                             bt=bt))
    # The wall's clean stretches (stock): every ray square and within STOCK_TOL of the line, STOCK_MIN long.
    clean, run, fits = [], [], []
    for k in best:
        r = rows.get(k)
        even = bool(r and r['ok'] and len(r['wall']) >= STOCK_FOUND and not r['caps'] and abs(r['yw'] - yw(r['x'])) < LINE_TOL)
        if even:                            # one smooth face, plumb or curving in (the casing: 18 cm over 2 m)
            z, y = np.array(r['wall']).T
            fit = np.polyfit(z, y, 2)
            even = float(np.abs(np.polyval(fit, z) - y).max()) < STOCK_FIT
            if even:
                fits.append((r['x'], fit[2], fit[1], fit[0]))
        if even:
            run.append(r['x'])
            continue
        if run and run[-1] - run[0] >= STOCK_MIN:
            clean.append((run[0], run[-1]))
        run = []
    if run and run[-1] - run[0] >= STOCK_MIN:
        clean.append((run[0], run[-1]))
    top_at = lambda x, xs=xs, t=top: float(np.interp(x, xs, t))
    fx, fa, fb, fc = (np.array(c) for c in zip(*fits)) if fits else (xs, yw_s, np.zeros(len(xs)), np.zeros(len(xs)))
    face = lambda x, z, fx=fx, fa=fa, fb=fb, fc=fc: float(np.interp(x, fx, fa) + np.interp(x, fx, fb) * z
                                                         + np.interp(x, fx, fc) * z * z)
    log(f'{side_name(s, name)}: {len(good)} of {len(rows)} stations read, the passage x {xs[0]:.0f}..{xs[-1]:.0f}; '
        f'wall bends at x ' + ', '.join(f'{xs[i]:.0f} ({yw_s[i]:.0f})' for i in bw) + '; bulwark bends at x '
        + ', '.join(f'{xs[i]:.0f} ({yb_s[i]:.0f})' for i in bb) + f'; clean wall {clean}')
    return dict(name=name, s=s, xs=xs, yw=yw, yb=yb, yw_s=yw_s, yb_s=yb_s, top=top_at, segs=segs, clean=clean,
                face=face, H=float(np.median(top)))     # H: the walk's floor level, the plates' heights over it


def _smooth(v):
    return np.array([np.median(v[max(i - LINE_MEDIAN, 0):i + LINE_MEDIAN + 1]) for i in range(len(v))])


def _dp(xs, ys, tol):
    """Douglas-Peucker: the indices of the points a polyline through them keeps within tol of all."""
    keep, todo = {0, len(xs) - 1}, [(0, len(xs) - 1)]
    while todo:
        i, j = todo.pop()
        if j <= i + 1:
            continue
        t = (xs[i + 1:j] - xs[i]) / (xs[j] - xs[i])
        dev = np.abs(ys[i + 1:j] - (ys[i] + t * (ys[j] - ys[i])))
        k = int(np.argmax(dev))
        if dev[k] > tol:
            keep.add(i + 1 + k)
            todo += [(i, i + 1 + k), (i + 1 + k, j)]
    return sorted(keep)


def _wall_stock(bm, tag, w, walks, fill):
    """The walk's wall plating: its own clean stretches, or (none long enough) the same walk's other side's."""
    import looter_tripo_barge_cut as cut
    src = w if w['clean'] else next((o for o in walks if o['name'] == w['name'] and o['clean']), None)
    if src is None:
        src = max(walks, key=lambda o: sum(b - a for a, b in o['clean']))
    if src is not w:
        log(f'{side_name(w["s"], w["name"])}: no clean wall of its own: plating from {side_name(src["s"], src["name"])}')
    return plates.Plating(bm, tag, cut.PASSAGE, 'wall', src['s'], src['clean'], line=src['face'], top=src['top'],
                          height=cut.LINE_H, skip=fill)
