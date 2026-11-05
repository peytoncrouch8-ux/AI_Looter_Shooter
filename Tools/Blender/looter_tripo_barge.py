"""The dredge barge reworked from Tripo's own pieces (Docs/Work/2026-11-04-Art/CONTRACT.md, "Round 3"): no new structure;
Tripo's faces are moved, squeezed and copied (their UVs with them) so the walkable kit fits the ship as the player knows
it from the spyglass: the low boiler casing with its two stacks aft, the tall house with its portholes and the white
tanks midship, the raked scoop at the bow. looter_tripo_wardens.build runs rework() on the fitted mesh (the 'rework' of
SPECS['DredgeBarge']) before the maps, sockets and LODs, and slots() after the maps (the material slots). Its floors are
measured by looter_tripo_barge_floors, its collision is looter_tripo_barge_collision, its previews and the measured
layout (Docs/Work/2026-11-04-Art/BARGE_LAYOUT.md) are looter_tripo_barge_check.

Frame: the barge's, cm, pivot bottom-centre at z 0, bow +X, the berth side -Y, Z up (B() into Blender metres).
What Tripo made (measured on the fitted mesh): one closed skin, 75,775 triangles. The hull is fullest amidships
(half-beam 1050-1120 at z 1700-2300 over x -2200..800) and narrows forward (930 at x 1800, 810 at 2300). Floors: the
stern platform, the cross walk between the casing and the house (x -670..-534) and the side walks all ~2255; the walks
are trenches 16-160 cm wide between the casing's or the house's skirt and the bulwark's inner face (|y| 875-960), the
casing's at its dome end and aft end roofed by the bulwark's lip; the foredeck 2254 inside a bulwark rim (inner face |y|
~723, top 2343) over x 1990..2300; the scoop's floor 2371 between its walls over x 2300..3250 (Tripo's 1.2 m step: the
forecastle). Under the hull a cradle with the hose drum (x -1790..-10, z 0..950) and a front strut (x 1850..2450, z
714..1062, its root in the keel's underside).

The rework, in order (every number a constant below):
1. Squeeze in y about the centreline: the casing by SQUEEZE_CASING, the house by SQUEEZE (blended over the cross walk),
   from just under the walk floors up. Over a walk's floor everything inboard of the bulwark's inner face squeezes whole
   (walls, skirts, what stands out of them); the floor stretches from the walls' moved feet to the bulwark, whose inner
   face stays; what stands over the bulwark's top (the casing's waist ledges) squeezes too. The stacks keep their round
   (their axes move), the two tanks scale whole by SQUEEZE.
0. Walks cleared (looter_tripo_barge_clear, before the squeeze): what Tripo's pieces carry across the side walks at a
   pawn's height (the casing's capping lips and band straps, the house's horn pipes) cut where it crosses, capped.
   After the squeeze its corridors take what is left where a walk measures under WALK_RUNS' widths or WALK_HEAD of
   headroom (both walks of the casing and the house, end to end: the deck fight's pushes come up all four).
2. Jumbo, fore (looter_tripo_barge_jumbo): everything fore of FORE_CUT moves FORE_DX forward; the gap is filled with
   FORE_COPIES copies of FORE_SLAB (the foredeck's plain plates between Tripo's knee and the band's bracket), every
   other one mirrored and each an extrusion of the cut's section, so every joint meets the same end of the slab and
   welds exactly (no crease, no zigzag of the strakes' edges).
   Their paint is then read off long stretches of Tripo's own side and foredeck (looter_tripo_barge_skin: no stretch
   of the side comes back within 7.1 m). The front strut stays whole where it hangs (cut off at STRUT_ROOT, or each
   copy carries a stub of its root); a keel plate under each copy closes the root's slot. (Round 4's unmirrored run of
   three zipped slabs is in git at 0c0750bf: its morphed joints creased the band under the strake, its paint came back
   every 0.5-0.8 m.)
3. Jumbo, midship: the same with MID_SLAB (the cross walk from the casing's fore face to under the house's overhang)
   MID_COPIES times (their own paint), opening the cross walk to 3.8 m for the hatch. The cradle and drum stay whole.
4. The berth: a GAP_W gap in the -Y bulwark rim of the well deck at GAP_X, cut at the sill's height first, its cut ends
   and sill filled in the paint next to them.
5. The forecastle's stairs (looter_tripo_barge_stairs): two flights (STAIR_Y) from the well deck up to the scoop's
   floor: capping treads and nosings, riveted band kick plates, stringers and handrails, the step's face clad in band.
6. The hatch: a HATCH hole in the cross walk over the hall's rim, its coaming COAMING_H high on three sides (the +Y
   edge open: the kit's stair goes down there) from the well deck's bulwark band laid flat, topped with the rim's top.
7. Material slots (slots()): Hull (below HULL_TOP), Deck (walkable surfaces), Super (house, casing, stacks, tanks,
   scoop, rails and fittings), Walls (the house's and casing's walls: exactly the faces looter_tripo_barge_paint.walls
   picks, the REBAKE lane's rule, taken out of Super for their own texture set): TripoDredgeBarge_Hull, _Deck, _Super,
   _Walls (Unreal: MI_<name>).
8. Collision and sockets (looter_tripo_barge_collision; sockets here): HoseDrum (Tripo's), Stack_1 and Stack_2 (their
   axes moved by the squeeze), Gangway (on the gap's sill, facing out), Hatch (the hole's centre).
"""
import math

import bmesh
import numpy as np

import looter_model as lm
import looter_textures as lt
import looter_tripo_barge_clear as clear
import looter_tripo_barge_skin as skin
import looter_tripo_rework as rw
from looter_tripo_barge_floors import DECK_UP, RIM_CLEAR, RIM_OUT, WALK_FLOOR, rim_at, rims, trench_at, trenches
from looter_tripo_rework import B, U, bisect, patch, spanning, steep, turn, ue_centre

# --- Tripo's barge as fitted (cm) ---
WELL = 2254.0                          # the foredeck's level: the well deck
CROSS = 2250.0                         # the cross walk between the casing and the house
FORECASTLE = 2371.0                    # the scoop's floor (terrace B): Tripo's 1.2 m step
STACKS = ((-1107.0, 21.0), (-2373.0, 19.0))
STACK_R, STACK_FOOT = 260.0, 2900.0
TANKS = (90.0, 1010.0, 760.0, 3830.0, 3900.0)    # x0, x1, |y| within, blend from z .. to z: the two white tanks
CRADLE = (-1850.0, 50.0, 950.0)        # x0, x1, below z: the cradle with the hose drum (stays whole)
STRUT = (1800.0, 2500.0, 300.0, 1000.0)          # x0, x1, |y| within, below z: the front strut (stays whole)
STRUT_ROOT = 1062.0                    # aft of the fore cut the root reaches the keel's underside (1054-1080): cut
                                       # there, or every copy carries a stub of it (a row of teeth under the keel)
# --- 1. The squeeze ---
SQUEEZE = 0.85                         # the house (the asked 0.85-0.9): its walks open from Tripo's 16-160 cm to >= 150
SQUEEZE_CASING = 0.82                  # the casing: 0.85 left 126 cm by its waist ledges' brackets (x -2700..-2200)
SQ_SPLIT = (-660.0, -620.0)            # x: the casing's factor blends into the house's over the cross walk
SQ_X, SQ_X_BLEND = (-3360.0, 2005.0), 40.0       # behind the casing's aft pillar (x -3354..-3300: across the blend
                                                 # its sides sheared into streaks) .. the house's fore face
SQ_Z = (2235.0, 2245.0)                # from just under the walk floors up: the skirts at the walls' feet squeeze too
BULWARK_KEEP = 10.0                    # the floor's last 10 cm by the bulwark stays: its inner face does not move
WALK_FALLBACK = (880.0, 900.0)         # a station with no floor found: |y| squeezed up to .. kept from
FLOOR_BAND, WALL_FADE = 6.0, 5.0       # cm: over the floor's top the walls' rule takes over; its fade at the bulwark
SQ_FULL_Z = (2450.0, 2700.0)           # above the rails everything squeezes (the walk canopies overhang the walks)
WALK_HEAD = 220.0                      # cm: the side walks' headroom end to end (the deck fight's pushes come up all four)
WALK_RUNS = {'casing': (SQ_X[0] + 20.0, -670.0, 120.0),      # Tripo's x from, to (the casing's aft pillar .. its fore
             'house': (-350.0, 1985.0, 150.0)}               # face; the house's aft face .. fore face), the clear width

# --- 2, 3. The jumbo ---
FORE_CUT, FORE_DX, FORE_COPIES = 2163.0, 1600.0, 18    # the bow parts at Tripo's x 2163 (just aft of the band's
                                       # bracket, ~2165..2190, which stays on the bow) and moves 16 m (the most asked)
FORE_SLAB = (FORE_CUT - FORE_DX / FORE_COPIES, FORE_CUT)   # 88.9 cm fore of Tripo's knee (x ~2035..2070)
MID_SLAB, MID_COPIES = (-664.0, -604.0), 4       # 60 cm x 4 = 2.4 m: the cross walk from the casing to under the house's
                                                 # overhang (x -600), one rail post in it (a 12 cm slab smeared its paint)
MID_DX = (MID_SLAB[1] - MID_SLAB[0]) * MID_COPIES
KEEL_LINE = (1100.0, 70.0)             # z under, |y| within: the jumbo's keel line, painted as one strip
MID_POST = (2355.0, 850.0)             # z, |y|: the slab's rail post (x -644..-620, to 2445) left out of its copies
# --- 4. The berth gap (final coordinates) ---
GAP_X, GAP_W, GAP_SILL = 3150.0, 300.0, 2262.0
# --- 5. The forecastle's stairs ---
STAIR_Y = ((150.0, 450.0), (-450.0, -150.0))
STAIR_STEPS, TREAD = 6, 30.0
# --- 6. The hatch and the hall under it (final coordinates) ---
HATCH = (-615.0, -435.0, -225.0, 225.0)          # x0, x1, y0, y1 in the cross walk
COAMING_H, COAMING_T = 60.0, 15.0
HALL = dict(x=-1185.0, y=0.0, r=750.0, floor=1300.0, ceiling=2220.0)   # its fore rim under the hatch's fore edge
# --- 7. Material slots ---
SLOTS = ('Hull', 'Deck', 'Super', 'Walls')   # the rework marks the first three (barge_slot); slots() takes Walls out of Super
HULL_TOP = 2170.0                      # below the rubbing strake
DECK_Z = (2230.0, 2390.0)
UV_THIN = 0.05                         # a stairs face with under this share of its plates' UV per area is repainted
# Parts (the face tag)
TRIPO, FORE, MID, STAIRS, COAMING, FILL, KEEP, STRUT_PART = 0, 1, 2, 3, 4, 5, 6, 7

UCX = []


def log(message):
    print(f'LOOTER: DredgeBarge rework: {message}', flush=True)


def final_x(x):
    """Where a point of Tripo's fitted barge at x ends up after both jumbo cuts."""
    return x + (MID_DX if x > MID_SLAB[1] else 0.0) + (FORE_DX if x > FORE_CUT else 0.0)


def smooth(a, b, t):
    return min(max((t - a) / (b - a), 0.0), 1.0)


def rework(obj, report, maps):
    import looter_tripo_barge_collision as collision   # here, not at the top: they import this module's constants
    import looter_tripo_barge_jumbo as jumbo
    import looter_tripo_barge_stairs as flights
    UCX.clear()
    bm = bmesh.new()
    bm.from_mesh(obj.data)
    uv = bm.loops.layers.uv.active
    tag = bm.faces.layers.int.new('rework_part')
    nearest = rw.Nearest(bm, uv)
    source = skin.Source(bm, uv)                       # Tripo's surface as it came: the jumbo's paint is read off it
    _keep(bm, tag)
    cut, caps = clear.walks(bm, tag, nearest, FILL)
    log(f'walks cleared: {cut} faces crossing them cut, {caps} cap faces')
    _squeeze(bm)
    found, gone, capped = clear.corridors(bm, tag, nearest, FILL, WALK_RUNS, WALK_HEAD)
    log(f'walks opened after the squeeze: {found} corridors, {gone} faces cut, {capped} cap faces')
    fore = jumbo.insert(bm, tag, FORE_SLAB, FORE_COPIES, FORE, nearest, still=(KEEP, STRUT_PART), extrude=True)
    skin.paint(bm, tag, FORE, source, FORE_CUT, FORE_DX / FORE_COPIES, fore)
    mid = jumbo.insert(bm, tag, MID_SLAB, MID_COPIES, MID, nearest, still=(KEEP,), trim=MID_POST)
    gap = _gap(bm, tag, nearest)
    stairs = flights.build(bm, tag)
    hatch = _hatch(bm, tag)
    posts, post_faces = clear.posts(bm, tag, nearest, FILL)
    log(f'forecastle: {posts} stub posts ({post_faces} faces) taken off its floor')
    split = rw.tidy(bm, lambda f: f[tag] != STAIRS)
    # Every fill painted again on its final shape (the cuts after it moved its neighbours), the jumbo's keel line as
    # one strip, any face left without UV area; the stairs' plates triangulated (a corner on a straight edge made the
    # exporter cut a zero-area triangle there).
    keel = [f for f in bm.faces if FORE_CUT - 5.0 < ue_centre(f)[0] < final_x(FORE_CUT) + FORE_DX + 5.0
            and any(U(v.co)[2] < KEEL_LINE[0] and abs(U(v.co)[1]) < KEEL_LINE[1] for v in f.verts)]
    bmesh.ops.triangulate(bm, faces=[f for f in bm.faces if f[tag] == STAIRS and len(f.verts) > 3], quad_method='BEAUTY',
                          ngon_method='BEAUTY')
    rw.repaint(bm, tag, FILL, nearest, keel)            # after: a triangle split off a plate can have no UV area
    rw.tidy(bm, lambda f: False)
    for f in bm.faces:                                  # and what the last tidy split
        if rw.uv_flat(uv, f):
            nearest.paint(f)
    # The stairs' plates pressed from the stock: a sliver whose paint is a line (far less UV for its size than its
    # plate's) has no tangent in Unreal: its own small patch.
    density = lambda f: _uv_area(uv, f) / max(f.calc_area(), 1e-12)
    plates = [f for f in bm.faces if f[tag] == STAIRS]
    if plates:
        typical = float(np.median([density(f) for f in plates]))
        for f in plates:
            if density(f) < typical * UV_THIN:
                nearest.paint(f)
    _slot_layer(bm, tag)
    pieces = collision.pieces(bm, tag)
    bm.faces.layers.int.remove(tag)
    bm.normal_update()
    bm.to_mesh(obj.data)
    bm.free()
    obj.data.update()
    collision.build(obj, pieces)
    _route_open()
    sq = lambda y: y * SQUEEZE_CASING                  # both stacks stand on the casing
    sockets = {'Gangway': ((GAP_X, -gap['outer'], GAP_SILL), -90.0),     # on the sill's outboard edge, facing out
               'Hatch': ((sum(HATCH[:2]) / 2.0, 0.0, CROSS), 0.0)}
    for name, (p, yaw) in sockets.items():
        lm.socket(obj, name, tuple(B(p)), (0.0, 0.0, -yaw))
        report.setdefault('sockets', {})[name] = p
    report['socket_at'] = {f'Stack_{k + 1}': (sx, sq(sy)) for k, (sx, sy) in enumerate(STACKS)}
    report['rework'] = {'fore copies': fore, 'mid copies': mid, 'gap': gap, 'stairs faces': stairs,
                        'coaming faces': hatch, 'triangulated, slivers': split, 'ucx': len(UCX)}
    for name, centre, size, _ in UCX:
        log(f'UCX {name}: centre {centre} size {size}')


def _route_open():
    """The stern push's way up both side walks (looter_tripo_barge_route, on the UCX as Unreal's import takes them):
    shut, the build fails here, whoever builds it (two builds of the same code differ a little, so a check of one
    build does not vouch for another)."""
    import bpy
    import looter_tripo_barge_check as check
    import looter_tripo_barge_route as stern
    bpy.context.view_layer.update()
    check._HULLS.clear()
    shut = [(side, got) for side in (-1, 1) if not (got := stern.route(check._hulls(), side))[0]]
    check._HULLS.clear()
    if shut:
        raise ValueError('the stern route is shut: ' + '; '.join(f'{"+Y" if s > 0 else "-Y"} past x {g[1]:.0f}: {g[2]}'
                                                                  for s, g in shut))
    log('the stern route is open up both walks (as Looter.World.Barge.Fight walks it)')


# --- Pieces that stay whole ---

def _keep(bm, tag):
    """The cradle with the drum and the front strut: cut off the hull at their tops, parted, tagged KEEP (no move)."""
    x0, x1, z = CRADLE
    sx0, sx1, sy, sz = STRUT
    near = lambda ps: min(abs(p[1]) for p in ps) < sy
    for top, region in ((z, over_x(x0, x1)), (sz, lambda ps: over_x(sx0, sx1)(ps) and near(ps)),
                        (STRUT_ROOT, lambda ps: over_x(sx0, FORE_CUT)(ps) and near(ps))):
        bisect(bm, spanning(bm, tag, TRIPO, 2, top, lambda ps, region=region: region(ps) and steep(ps)), 2, top)
    keep = []
    for f in bm.faces:
        x, y, zc = ue_centre(f)
        if x0 < x < x1 and zc < z:
            f[tag] = KEEP
            keep.append(f)
        elif sx0 < x < sx1 and abs(y) < sy and (zc < sz or (x < FORE_CUT and zc < STRUT_ROOT)):
            f[tag] = STRUT_PART
            keep.append(f)
    keep = rw.separate(bm, keep)
    log(f'kept whole: {len(keep)} faces (the cradle and drum, the front strut)')


def over_x(a, b):
    return lambda ps: max(p[0] for p in ps) > a and min(p[0] for p in ps) < b


# --- 1. The squeeze ---

def _squeeze(bm):
    """The house and the casing narrowed in y (see the docstring); the stacks' axes follow, the tanks scale whole."""
    x0, x1 = SQ_X
    tx0, tx1, ty, tz0, tz1 = TANKS
    tc = ((tx0 + tx1) / 2.0, 0.0, tz0)
    trench = trenches(bm)
    rim = rims(bm, trench)
    moved = 0
    for v in bm.verts:
        x, y, z = U(v.co)
        if z < SQ_Z[0] or x < x0 - SQ_X_BLEND or x > x1 + SQ_X_BLEND:
            continue
        wx = min(smooth(x0 - SQ_X_BLEND, x0, x), 1.0 - smooth(x1, x1 + SQ_X_BLEND, x))
        f = squeeze_at(x)
        stack = next(((sx, sy) for sx, sy in STACKS if math.hypot(x - sx, y - sy) < STACK_R and z > STACK_FOOT), None)
        if stack:
            p = (x, y + (f - 1.0) * stack[1] * wx, z)
        elif tx0 < x < tx1 and abs(y) < ty and z > tz0:
            # The tanks: whole (round), blending from the squeeze below them to the scale over their feet.
            k = smooth(tz0, tz1, z)
            whole = tuple(c + SQUEEZE * (q - c) for q, c in zip((x, y, z), tc))
            p = (x + (whole[0] - x) * k, y * SQUEEZE + (whole[1] - y * SQUEEZE) * k, z + (whole[2] - z) * k)
        else:
            # Over the walk's floor all inboard of the bulwark squeezes (walls, skirts, what stands out of them: one
            # factor, or their parts tear into spikes); the floor itself stretches from the walls' feet to the bulwark.
            t = trench_at(trench, x, y)
            lo, hi, zf = t if t else (*WALK_FALLBACK, WALK_FLOOR[1])
            keep = max(hi - BULWARK_KEEP, lo + 3.0)
            floor_w = 1.0 - smooth(lo + 2.0, keep, abs(y))
            wall_w = 1.0 - smooth(keep - WALL_FADE, keep, abs(y))
            up = smooth(zf + 1.0, zf + FLOOR_BAND, z)
            low = smooth(SQ_Z[0], SQ_Z[1], z) * (floor_w + (wall_w - floor_w) * up)
            # Ledges and brackets over the bulwark's top (the casing's waist ledges reach its inner face): whole.
            top = rim_at(rim, x, y)
            over = smooth(top + RIM_CLEAR, top + 2.0 * RIM_CLEAR, z) if top is not None and abs(y) < hi + RIM_OUT else 0.0
            w = wx * max(low, over, smooth(SQ_FULL_Z[0], SQ_FULL_Z[1], z))
            p = (x, y * (1.0 - w * (1.0 - f)), z)
        v.co = B(p)
        moved += 1
    log(f'squeeze: {moved} vertices in y, the casing to {SQUEEZE_CASING}, the house to {SQUEEZE}; stacks round, tanks '
        f'whole')


def squeeze_at(x):
    """The squeeze's factor at Tripo's x: the casing's aft of the cross walk, the house's fore of it."""
    return SQUEEZE_CASING + (SQUEEZE - SQUEEZE_CASING) * smooth(SQ_SPLIT[0], SQ_SPLIT[1], x)


# --- 4. The berth gap ---

def _gap(bm, tag, nearest):
    """Cuts the -Y bulwark rim of the well deck open over GAP_W at GAP_X (final x); fills the cut's ends and sill."""
    x0, x1 = GAP_X - GAP_W / 2.0, GAP_X + GAP_W / 2.0
    side = lambda ps: max(p[1] for p in ps) < -500.0
    # The sill's height first, then the gap's ends above it (so the cut's chains end on the sill).
    bisect(bm, spanning(bm, tag, None, 2, GAP_SILL, lambda ps: side(ps) and over_x(x0 - 30.0, x1 + 30.0)(ps) and steep(ps)),
           2, GAP_SILL)
    for x in (x0, x1):
        bisect(bm, spanning(bm, tag, None, 0, x, lambda ps: side(ps) and above(GAP_SILL)(ps)), 0, x)
    doomed = [f for f in bm.faces if x0 < ue_centre(f)[0] < x1 and ue_centre(f)[1] < -500.0
              and ue_centre(f)[2] > GAP_SILL]
    parts = {f[tag] for f in doomed}
    bmesh.ops.delete(bm, geom=doomed, context='FACES')
    filled = 0
    for x, out in ((x0, 1.0), (x1, -1.0)):            # each end one cap, whatever pieces meet there, one paint
        end = rw.cap(bm, tag, None, 0, x, GAP_SILL, (out, 0.0, 0.0), nearest, FILL, region=lambda q: q[1] < -500.0)
        rw.paint_like(nearest.uv, end) or rw.paint_group(nearest, end)
        filled += len(end)
    sill = rw.cap(bm, tag, None, 2, GAP_SILL, None, (0.0, 0.0, 1.0), nearest, FILL,
                  region=lambda q: q[1] < -500.0 and x0 - 1.0 < q[0] < x1 + 1.0)
    rw.paint_like(nearest.uv, sill) or rw.paint_group(nearest, sill)
    filled += len(sill)
    outer = max((-U(v.co)[1] for f in sill for v in f.verts), default=800.0)    # the sill's outboard edge
    log(f'berth gap: {len(doomed)} faces cut at x {x0:.0f}..{x1:.0f} on -Y, {filled} filled; the sill\'s outboard '
        f'edge at |y| {outer:.0f}')
    return {'x': GAP_X, 'w': GAP_W, 'sill': GAP_SILL, 'outer': round(outer, 1)}


def above(z):
    return lambda ps: min(p[2] for p in ps) > z - 1.0


# --- 5. The forecastle's stairs (looter_tripo_barge_stairs) ---

def terrace_aft(bm):
    """The forecastle's aft edge (final x): the aftmost up-facing face at its level near the centreline."""
    xs = [ue_centre(f)[0] for f in bm.faces if abs(ue_centre(f)[2] - FORECASTLE) < 8.0 and abs(ue_centre(f)[1]) < 500.0
          and f.normal.z > 0.9 and ue_centre(f)[0] > final_x(2200.0)]
    return min(xs) if xs else final_x(2300.0)


# --- 6. The hatch ---

def _hatch(bm, tag):
    """The hole in the cross walk over the hall's rim and its coaming: walls of the well deck's bulwark band (inner
    face, rivets) COAMING_H high, COAMING_T thick, topped with the rim's top."""
    x0, x1, y0, y1 = HATCH
    deck = lambda ps: max(abs(p[2] - CROSS) for p in ps) < 12.0
    hole = (x0, x1, y0, y1, CROSS - 12.0, CROSS + 12.0)
    for axis, lo, hi in ((0, x0, x1), (1, y0, y1)):
        for value in (lo, hi):
            bisect(bm, spanning(bm, tag, None, axis, value, lambda ps: deck(ps) and over_x(x0 - 50, x1 + 50)(ps)),
                   axis, value)
    doomed = [f for f in bm.faces if rw.in_box(f, hole) and all(
        hole[2 * a] - 0.5 <= ue_centre(f)[a] <= hole[2 * a + 1] + 0.5 for a in range(3))]
    bmesh.ops.delete(bm, geom=doomed, context='FACES')
    # The source: the +Y bulwark rim of the well deck (a stretch clear of the gap and the stairs).
    sx = final_x(FORE_CUT) + 200.0
    band = [f for f in bm.faces if sx - 50.0 < ue_centre(f)[0] < sx + 600.0 and ue_centre(f)[1] > 650.0]
    inner = min((U(v.co)[1] for f in band if abs(ue_centre(f)[2] - 2300.0) < 30.0 and abs(f.normal.x) > 0.8
                 for v in f.verts), default=723.0)
    z0 = WELL + 8.0
    made = 0
    t = COAMING_T
    # (length, the hole's edge centre (x, y), outward): the +Y edge stays open, the kit's stair goes down there.
    walls = [(x1 - x0 + 2 * t, ((x0 + x1) / 2.0, y0), (0.0, -1.0)),
             (y1 - y0 + t, (x1, (y0 + y1 - t) / 2.0), (1.0, 0.0)), (y1 - y0 + t, (x0, (y0 + y1 - t) / 2.0), (-1.0, 0.0))]
    for length, (ex, ey), o in walls:
        # The source band faces -y (the +Y bulwark's inner face); turned by ang it faces into the hole (-o).
        ang = math.degrees(math.atan2(-o[0], o[1]))
        box = (sx, sx + length, inner - 5.0, inner + 40.0, z0, z0 + COAMING_H)
        src_c = (sx + length / 2.0, inner, z0)
        # Flattened onto the face's plane: the band's rounded top would cross the wall's two sides into an X.
        for a, (dx, dy) in ((ang, (ex, ey)), (ang + 180.0, (ex + o[0] * t, ey + o[1] * t))):
            mv = turn(a, src_c, (dx, dy, CROSS))
            made += len(patch(bm, band, box, tag, COAMING, lambda p, mv=mv: mv((p[0], inner, p[2]))))
        # The top: the rim's top strip, t wide, over the wall.
        top = (sx, sx + length, inner + 5.0, inner + 5.0 + t, 2325.0, 2360.0)
        tsrc = (sx + length / 2.0, inner + 5.0 + t / 2.0, 2343.0)
        made += len(patch(bm, band, top, tag, COAMING,
                          turn(ang, tsrc, (ex + o[0] * t / 2.0, ey + o[1] * t / 2.0, CROSS + COAMING_H + 1.0))))
    log(f'hatch: {len(doomed)} deck faces cut out at x {x0:.0f}..{x1:.0f}, y {y0:.0f}..{y1:.0f}; coaming {made} faces '
        f'(bulwark band at |y| {inner:.0f})')
    return made


# --- 7. Material slots ---

def _uv_area(uv, f):
    us = [l[uv].uv for l in f.loops]
    return abs(sum((us[k] - us[0]).cross(us[k + 1] - us[0]) for k in range(1, len(us) - 1))) / 2.0


def _slot_layer(bm, tag):
    layer = bm.faces.layers.int.get('barge_slot') or bm.faces.layers.int.new('barge_slot')
    counts = [0] * len(SLOTS)
    for f in bm.faces:
        x, y, z = ue_centre(f)
        if f.normal.z > DECK_UP and DECK_Z[0] < z < DECK_Z[1]:
            k = 1
        elif z < HULL_TOP and f[tag] != KEEP:
            k = 0
        else:
            k = 2
        f[layer] = k
        counts[k] += 1
    log('slots: ' + ', '.join(f'{n} {c}' for n, c in zip(SLOTS, counts) if c))


def slots(obj, set_name, master):
    """After the build's maps: the faces' slots (barge_slot, with the walls looter_tripo_barge_paint.walls picks taken
    out of Super) as four materials."""
    import looter_tripo_barge_paint as barge_paint     # here: it imports this module (its wall rule: one source of truth)
    mesh = obj.data
    attr = mesh.attributes.get('barge_slot')
    if attr is None:
        return
    values = [d.value for d in attr.data]
    wall = barge_paint.walls(obj)
    if wall is not None:
        values = [SLOTS.index('Walls') if w else v for v, w in zip(values, wall)]
        log(f'slots: {int(sum(wall))} faces of Super are Walls')
    for k, name in enumerate(SLOTS):
        mat = lt.material(set_name, name=f'{set_name}_{name}', master=master)   # Unreal: slot X, MI_X
        lt.assign(obj, mat, [i for i, v in enumerate(values) if v == k])
    mesh.attributes.remove(attr)
    # Drop the unused first slot (the whole-mesh material the build gave every face before). materials.pop() moves the
    # faces' indices down itself (round 3 moved them again by hand: every face one slot off; check: _slots()).
    used = {p.material_index for p in mesh.polygons}
    for i in reversed(range(len(mesh.materials))):
        if i not in used:
            mesh.materials.pop(index=i)
