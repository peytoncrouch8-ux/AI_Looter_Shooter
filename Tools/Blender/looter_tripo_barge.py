"""The dredge barge's rework (Docs/Work/2026-11-04-Art/CONTRACT.md, round 2: "the Tripo hull REWORKED"): Tripo's barge
(Art/Tripo/DredgeBarge) reshaped by script so the game's walkable kit fits it. looter_tripo_wardens.build runs rework() on
the fitted mesh (its SPECS['DredgeBarge'] 'rework'), before the maps, sockets and LODs, so every export reproduces it.

Frame: the barge's (BargeRules.h), cm, pivot bottom-centre at z 0, bow +X, the berth side -Y, Z up; B() turns a point into
the model's Blender metres (front -Y: Unreal +X is Blender -Y, Unreal +Y is Blender -X).

What Tripo made (BARGE_MEASURE.md, and this lane's sections of the fitted mesh): ONE closed skin (one island, 75,775
triangles, 37 open edges), nothing modelled inside. Its open deck is terraces at 2250-2371 between a midship block
(x -575..1990, |y| ~800, roof ~3150 with a railed walk to |y| 1000), a cabin on it (x -250..1825, to 3790) with two white
tanks (x 100..1000, y +-800, to 4849: "the 46 m tower"), an aft engine casing (x -3250..-650, to 3020), two stacks (axes
(-1107, 21) and (-2373, 19), r ~200 from 3300 up, rings r 240 at 4500-4600, tops 4808), a scoop at the bow (x 2300..3750,
walls |y| 400-650 to ~3000). The hull is round: half-beam 1050-1100 at z 2000-2200 amidships, 950 at x 2000, 700 at
x 3000, 880 at x -3300; never the bounds' 1237.5 (only thin plates reach it).

The rework, in order:
1. The tower (the cabin over x TOWER_SRC from TOWER_FOOT up, with the tanks) is copied out, squeezed along x to fit
   between the stacks and set on the house roof over x TOWER_DST (its foot on the roof; its cut ends capped).
2. Cut away above CUT between the house's aft wall and the scoop (x HOUSE_AFT..DECK_FORE): the midship block, cabin and
   tanks, the casing, the foredeck terraces (2371, 2300), the side rails and the scoop's part aft of DECK_FORE. Above the
   roof the stacks, their pipes and stays stay (but not where the tower stands); below ROOF inside the house nothing stays
   (the kit's hall fills it: its .Hull test traces z 2350 and 3100), so the stacks start at the roof.
3. Fill: one flat deck at DECK over x DECK_AFT..DECK_FORE to the bulwark; the scoop's cut face at DECK_FORE capped.
4. The house: x HOUSE_AFT..HOUSE_FORE, full beam BEAM, roof at ROOF, walls WALL_T thick, hollow (no floor at deck level:
   the kit floors it), a door DOOR_W x DOOR_H at y 0 in its fore face with its threshold at DECK.
5. The bulwark: RAIL_H high, RAIL_T thick, outer face at BEAM, x DECK_AFT..DECK_FORE, a GAP_W gap over a SILL_H sill at
   GAP_X on -Y; short end walls across the bow beside the scoop (else the deck's front edge is open).
6. The guard: Tripo's hull is narrower than BEAM everywhere at the deck, so the deck and the house overhang it; a belt
   (fascia at BEAM from GUARD_LO, a chamfer, a flat underside running GUARD_INTO into the hull skin) closes the overhang
   all along x HOUSE_AFT..DECK_FORE, like a tug's deck guard.
7. Collision (UCX_ boxes): deck slab, bulwarks (split round the gap and at RAIL_SPLIT_X), the hull body below the deck,
   the house walls round the door, its roof, the stacks and tower from the roof up, the scoop, the stern; none inside the
   house. Sockets Door and Gangway (Tripo's HoseDrum, Stack_1, Stack_2 are placed by the build).
8. UV0: Tripo's atlas has no tiling area, so new faces take plain patches of it (looter_tripo_rework.Paint): tiny squares
   inside the UV charts of a matching Tripo surface (PAINTS: the old foredeck for the deck, the midship block's walls for
   the house, the casing's roof for the roof, the hull's sheer band for the bulwark and guard) where colour and normal map
   are flattest and the colour is that surface's usual one; each ~3 m tile of a new surface (laid like plating) gets one
   at random (SEED), so the paint is Tripo's and plates differ a little in tone. The detail layer (M_World's, object
   space) draws the plates at close range. The maps and write_maps are unchanged.
9. Clean-up (looter_tripo_rework.tidy): nearly level faces are not cut level (LEVEL_FACE), cut Tripo polygons are
   triangulated the best way round and hairline slivers dropped: the export's tangent check (Tools/Blender/tangentcheck.py)
   finds nothing Unreal would warn about.
Round 2 (the coordinator, after round 1's previews): the scoop moved forward and the house dressed.
10. The scoop: Tripo's bucket lay on the foredeck over x ~2280..3380 (its X-frame arms and the bow's rail ring in front,
   to x 3683), so the flat deck out to DECK_FORE cut most of it away. Everything of it above SCOOP_FOOT inside
   x > SCOOP_X0, |y| < SCOOP_HALF is moved SCOOP_SHIFT forward (its aft end onto the deck's fore edge; it overhangs the
   bow and the bounds grow forward) on a new seat (a plate from the bow's end walls to past its front, following its
   plan, top at SCOOP_FOOT, underside SCOOP_SEAT); its cut sides are capped.
11. The house dressed (looter_tripo_barge_house): ribs, portholes, a catwalk ring with Tripo's walk railing re-used, the
   door's frame, lintel plate and steps, vents on the roof; all outside the walls.
"""
import math

import bmesh
from mathutils import Vector

import looter_model as lm
import looter_tripo_barge_house as house
import looter_tripo_rework as rw
from looter_tripo_rework import (B, Build, Paint, U, above, bisect, box_of, cap, outline, over, reaching, spanning,
                                 steep, strip, ue_centre)

# --- The contract's numbers (cm) ---
DECK = 2250.0                         # BargeRules DeckTop
CUT = 2220.0                          # Tripo's skin goes above this from HOUSE_AFT to DECK_FORE (30 under the deck)
DECK_AFT, DECK_FORE = -400.0, 3350.0  # the open deck; the scoop beyond DECK_FORE stays as Tripo made it
BEAM = 1237.5                         # the bulwark's and the house's outer faces
RAIL_H, RAIL_T = 110.0, 15.0          # BargeRules RailHeight; the bulwark's thickness
GAP_X, GAP_W, SILL_H = 2000.0, 300.0, 2.0   # BerthX: the gangway's gap on -Y (BerthSide), over a sill
RAIL_SPLIT_X = 3200.0                 # the bulwark's collision splits here too
HOUSE_AFT, HOUSE_FORE = -3300.0, -400.0
ROOF, WALL_T, ROOF_T = 3300.0, 20.0, 20.0
BAND_LO, BAND_HI = 50.0, 70.0         # the house's outer walls: a dark foot band and a band under the roof's edge
COAMING_H, COAMING_T = 30.0, 20.0     # the roof's rim
DOOR_W, DOOR_H = 240.0, 300.0         # at y 0 in the fore face, threshold at DECK (GetHatchSpot)
# --- Measured on Tripo's mesh, and this lane's choices ---
STACKS = ((-1107.0, 21.0), (-2373.0, 19.0))
STACK_KEEP_R = 330.0                  # the stacks' rings reach r 240; pipes and stays are kept above the roof anyway
TOWER_SRC = (90.0, 1010.0)            # the cabin slice under the tanks (tanks x 100..1000)
TOWER_FOOT = 3300.0                   # cut at the midship block's railed walk: the slice's foot lands on ROOF
TOWER_DST = (-2140.0, -1340.0)        # between the stacks (shafts r ~200: free x -2173..-1307); the contract's
                                      # -2000..-1200 would put the cabin into stack 1
TOWER_CABIN_TOP = 3800.0              # the cabin's roof under the tanks (for its collision)
GUARD_LO, GUARD_CHAMFER, GUARD_INTO = 2000.0, 40.0, 100.0
SCOOP_X0, SCOOP_FOOT, SCOOP_HALF = 2250.0, 2310.0, 700.0   # the bucket, its arms and rail: above the bow's deck (2300)
SCOOP_SHIFT = DECK_FORE - SCOOP_X0    # forward, so its aft end meets the deck's fore edge
SCOOP_SEAT, SEAT_MARGIN, SEAT_MIN = 2200.0, 30.0, 200.0   # its seat's underside; how far the seat runs past it
STATION = 100.0                       # x step of the guard's underside and the house's ledge along the hull
HULL_BOXES = (                        # the hull's collision below the deck (x0, x1, z0, z1): each as wide as the skin's
    ('hull top', -2800.0, 1200.0, 1700.0, CUT),          # narrowest section over its x range, HULL_BOX_IN inside it
    ('hull top fore', 1200.0, 2400.0, 1800.0, CUT),
    ('hull top bow', 2400.0, DECK_FORE, 1950.0, CUT),
    ('hull top stern', -3400.0, -2800.0, 1700.0, CUT),
    ('hull middle', -2500.0, 1500.0, 1300.0, 1700.0),
    ('hull belly', -2000.0, 1000.0, 1100.0, 1300.0))
HULL_BOX_IN = 25.0
SEED = 1104
PAINTS = {                            # Tripo faces a paint is taken from: normal test, x, z, |y| ranges (cm)
    'deck': dict(up=0.9, x=(1300.0, 3300.0), z=(2240.0, 2400.0), y=(0.0, 1000.0)),
    'wall': dict(side=0.2, x=(-500.0, 1950.0), z=(2350.0, 3050.0), y=(700.0, 1100.0)),
    'roof': dict(up=0.9, x=(-3200.0, -800.0), z=(2750.0, 3100.0), y=(0.0, 1000.0)),
    'hull': dict(side=0.35, x=(-2500.0, 1500.0), z=(1850.0, 2200.0), y=(900.0, 1300.0)),
    'glass': dict(side=0.3, x=(-250.0, 1825.0), z=(3300.0, 3750.0), y=(700.0, 1000.0), dark=0.12),  # cabin windows
}

UCX = []                              # (name, centre, size) in cm, for the log


def log(message):
    print(f'LOOTER: DredgeBarge rework: {message}', flush=True)


# --- The rework ---

def rework(obj, report, maps):
    """Reshapes the fitted barge obj in place (see the module's docstring); adds its UCX_ boxes and its Door and Gangway
    sockets; notes what it did in report."""
    UCX.clear()
    bm = bmesh.new()
    bm.from_mesh(obj.data)
    uv = bm.loops.layers.uv.active
    tag = bm.faces.layers.int.new('rework_part')     # 0 Tripo's, 1 the moved tower, 2 new
    paint = Paint(bm, uv, maps, PAINTS, SEED)
    build = Build(bm, uv, paint, tag)
    stations = [HOUSE_AFT + STATION * k for k in range(int(round((DECK_FORE - HOUSE_AFT) / STATION)) + 1)]
    guard = {s: [y - GUARD_INTO for y in outline(bm, GUARD_LO, s, stations, STATION)] for s in (-1, 1)}
    hull = _hull_widths(bm)
    ledge = {s: [y - GUARD_INTO for y in outline(bm, CUT, s, stations, STATION)] for s in (-1, 1)}
    log('hull half-beam at z %.0f (x, port, stbd; every 5 m): %s' % (GUARD_LO, [
        (round(stations[k]), round(guard[-1][k] + GUARD_INTO), round(guard[1][k] + GUARD_INTO))
        for k in range(0, len(stations), 5)]))

    railed = house.rail_copies(bm, tag, 4)          # (Tripo's walk railing, before the clear takes it)
    moved = _tower(bm, tag)
    scooped = _scoop(bm, tag)
    deleted = _clear(bm, tag)
    cap(bm, build, tag, 0, 0, DECK_FORE, CUT, (-1.0, 0.0, 0.0), 'hull')
    cut_edge = _scoop_edges(bm, tag)
    cap(bm, build, tag, 1, 0, TOWER_DST[0], ROOF, (-1.0, 0.0, 0.0), 'wall')
    cap(bm, build, tag, 1, 0, TOWER_DST[1], ROOF, (1.0, 0.0, 0.0), 'wall')
    cap(bm, build, tag, 3, 0, SCOOP_X0 + SCOOP_SHIFT, SCOOP_FOOT, (-1.0, 0.0, 0.0), 'hull')
    for s in (-1, 1):
        cap(bm, build, tag, 3, 1, s * SCOOP_HALF, SCOOP_FOOT, (0.0, float(s), 0.0), 'hull')
    _deck(build)
    _house(build, ledge, stations)
    _guard(build, guard, stations, cut_edge)
    seat = _seat(build, bm, tag)
    dressing = house.dress(build)
    split = rw.tidy(bm, lambda f: f[tag] != 2)
    measured = {'scoop': box_of(bm, tag, 3), 'seat': seat,
                'stern': box_of(bm, tag, 0, lambda p: p[0] < HOUSE_AFT - 1.0 and p[2] > CUT - 1.0),
                'cradle': box_of(bm, tag, 0, lambda p: p[2] < 700.0),
                'cabin': box_of(bm, tag, 1, lambda p: 3420.0 < p[2] < 3700.0),
                'tanks': box_of(bm, tag, 1, lambda p: p[2] > TOWER_CABIN_TOP + 50.0),
                'tower': box_of(bm, tag, 1),
                'stack tops': [max((U(v.co)[2] for v in bm.verts
                                    if math.hypot(U(v.co)[0] - sx, U(v.co)[1] - sy) < 260.0), default=4800.0)
                               for sx, sy in STACKS]}
    bm.faces.layers.int.remove(tag)
    bm.normal_update()
    bm.to_mesh(obj.data)
    bm.free()
    obj.data.update()
    _collision(obj, measured, hull, dressing)
    for name, p, yaw in (('Door', (HOUSE_FORE, 0.0, DECK), 0.0), ('Gangway', (GAP_X, -BEAM, DECK), -90.0)):
        # An Unreal yaw turns the other way about Blender's Z; an empty's -Y is the socket's front.
        lm.socket(obj, name, tuple(B(p)), (0.0, 0.0, -yaw))
        report.setdefault('sockets', {})[name] = p
    obj['Fallback'] = 100.0
    report['rework'] = {'polygons triangulated, slivers dropped': split, 'tower faces moved': moved,
                        'faces cut away': deleted, 'new faces': build.faces,
                        'scoop faces moved': scooped, 'railing faces laid': railed, 'measured': measured,
                        'ucx': len(UCX)}
    for name, centre, size in UCX:
        log(f'UCX {name}: centre {centre} size {size}')


def _tower(bm, tag):
    """Copies the cabin slice over TOWER_SRC from TOWER_FOOT up (the tanks on it) onto the roof over TOWER_DST."""
    x0, x1 = TOWER_SRC
    # The foot where the slice is, then its ends above the foot.
    bisect(bm, spanning(bm, tag, 0, 2, TOWER_FOOT, lambda ps: over(0, x0, x1)(ps) and steep(ps)), 2, TOWER_FOOT)
    for x in (x0, x1):
        bisect(bm, spanning(bm, tag, 0, 0, x, above(TOWER_FOOT)), 0, x)
    faces = [f for f in bm.faces if f[tag] == 0 and x0 < ue_centre(f)[0] < x1 and ue_centre(f)[2] > TOWER_FOOT]
    edges = {e for f in faces for e in f.edges}
    verts = {v for f in faces for v in f.verts}
    made = bmesh.ops.duplicate(bm, geom=faces + list(edges) + list(verts))
    squeeze = (TOWER_DST[1] - TOWER_DST[0]) / (x1 - x0)
    for g in made['geom']:
        if isinstance(g, bmesh.types.BMFace):
            g[tag] = 1
        elif isinstance(g, bmesh.types.BMVert):
            x = -g.co.y * 100.0
            g.co.y = -(TOWER_DST[0] + (x - x0) * squeeze) / 100.0
            g.co.z += (ROOF - TOWER_FOOT) / 100.0
    log(f'tower: {len(faces)} faces from x {x0:.0f}..{x1:.0f} over z {TOWER_FOOT:.0f} to x {TOWER_DST[0]:.0f}..'
        f'{TOWER_DST[1]:.0f} (x {squeeze:.3f}), foot on the roof at {ROOF:.0f}')
    return len(faces)


def _in_stack(c):
    return any(math.hypot(c[0] - sx, c[1] - sy) < STACK_KEEP_R for sx, sy in STACKS)


def _clear(bm, tag):
    """Cuts Tripo's skin on the planes that bound the cleared volume, then deletes what lies in it."""
    pad = 30.0
    # The planes where faces reach above CUT, then CUT between them (never across the stern's or the scoop's own
    # surfaces, some nearly level with it); the roof in the house, then its planes above the roof.
    for x in (HOUSE_AFT, DECK_FORE):
        bisect(bm, spanning(bm, tag, 0, 0, x, reaching(CUT)), 0, x)
    inside, house = over(0, HOUSE_AFT + 0.5, DECK_FORE - 0.5), over(0, HOUSE_AFT, HOUSE_FORE)
    bisect(bm, spanning(bm, tag, 0, 2, CUT, lambda ps: inside(ps) and steep(ps)), 2, CUT)
    bisect(bm, spanning(bm, tag, 0, 2, ROOF, lambda ps: house(ps) and above(CUT)(ps) and steep(ps)), 2, ROOF)
    for x in (HOUSE_FORE, TOWER_DST[0] - pad, TOWER_DST[1] + pad):
        # (Not through the stacks: their rings reach into the tower's margin and stay whole.)
        bisect(bm, [f for f in spanning(bm, tag, 0, 0, x, above(ROOF)) if not _in_stack(ue_centre(f))], 0, x)

    def doomed(f):
        x, y, z = ue_centre(f)
        if not (HOUSE_AFT < x < DECK_FORE and z > CUT):
            return False
        if z < ROOF or x > HOUSE_FORE:
            return True          # inside the house, or above the open deck
        # Above the roof: the stacks, their pipes and stays stay, but not where the tower stands.
        return TOWER_DST[0] - pad < x < TOWER_DST[1] + pad and not _in_stack((x, y))
    faces = [f for f in bm.faces if f[tag] == 0 and doomed(f)]
    bmesh.ops.delete(bm, geom=faces, context='FACES')
    # Above the roof only what hangs together with a stack stays (its pipes and stays); rail ends and stair rails of
    # the casing and the midship block, cut free at the roof, go.
    roofed = {f for f in bm.faces if f[tag] == 0 and HOUSE_AFT < ue_centre(f)[0] < HOUSE_FORE and ue_centre(f)[2] > ROOF}
    todo = [f for f in roofed if _in_stack(ue_centre(f)) and
            min(math.hypot(ue_centre(f)[0] - sx, ue_centre(f)[1] - sy) for sx, sy in STACKS) < 260.0]
    kept = set(todo)
    while todo:
        f = todo.pop()
        for e in f.edges:
            for g in e.link_faces:
                if g in roofed and g not in kept:
                    kept.add(g)
                    todo.append(g)
    strays = list(roofed - kept)
    bmesh.ops.delete(bm, geom=strays, context='FACES')
    log(f'cut away {len(faces)} faces above z {CUT:.0f} over x {HOUSE_AFT:.0f}..{DECK_FORE:.0f}, and {len(strays)} '
        f'loose above the roof')
    return len(faces) + len(strays)


def _scoop(bm, tag):
    """Moves the bucket with its arms and the bow's rail ring (Tripo's, above SCOOP_FOOT in x > SCOOP_X0, |y| <
    SCOOP_HALF) SCOOP_SHIFT forward, tagged 3; its cut sides are capped later."""
    x0, foot, half = SCOOP_X0, SCOOP_FOOT, SCOOP_HALF
    near = lambda ps: max(p[0] for p in ps) > x0 and min(abs(p[1]) for p in ps) < half
    bisect(bm, spanning(bm, tag, 0, 2, foot, lambda ps: near(ps) and steep(ps)), 2, foot)
    bisect(bm, spanning(bm, tag, 0, 0, x0, lambda ps: above(foot)(ps) and min(abs(p[1]) for p in ps) < half), 0, x0)
    for s in (-1, 1):
        bisect(bm, spanning(bm, tag, 0, 1, s * half, lambda ps: above(foot)(ps) and max(p[0] for p in ps) > x0), 1,
               s * half)
    faces = [f for f in bm.faces if f[tag] == 0 and ue_centre(f)[0] > x0 and abs(ue_centre(f)[1]) < half
             and ue_centre(f)[2] > foot]
    geom = faces + list({e for f in faces for e in f.edges}) + list({v for f in faces for v in f.verts})
    for g in bmesh.ops.duplicate(bm, geom=geom)['geom']:
        if isinstance(g, bmesh.types.BMFace):
            g[tag] = 3
        elif isinstance(g, bmesh.types.BMVert):
            g.co.y -= SCOOP_SHIFT / 100.0
    bmesh.ops.delete(bm, geom=faces, context='FACES')
    log(f'scoop: {len(faces)} faces (bucket, arms, the bow rail) above z {foot:.0f} moved {SCOOP_SHIFT:.0f} forward')
    return len(faces)


def _seat(build, bm, tag):
    """The moved scoop's seat: a plate from the bow's end walls to past its front, as wide as it is at each station
    (SCOOP_HALF at most), top at SCOOP_FOOT. Returns its box (x0, y0, z0, x1, y1, z1)."""
    pts = [U(v.co) for f in bm.faces if f[tag] == 3 for v in f.verts]
    x0, front = DECK_FORE - RAIL_T, max(p[0] for p in pts) + SEAT_MARGIN
    n = max(1, int(math.ceil((front - x0) / STATION)))
    xs = [x0 + (front - x0) * k / n for k in range(n + 1)]
    half = [SCOOP_HALF] + [min(SCOOP_HALF, max([abs(p[1]) for p in pts if abs(p[0] - x) < STATION], default=0.0)
                               + SEAT_MARGIN) for x in xs[1:]]
    half = [max(h, SEAT_MIN) for h in half]
    zt, zb = SCOOP_FOOT, SCOOP_SEAT
    for k in range(n):
        xa, xb, ya, yb = xs[k], xs[k + 1], half[k], half[k + 1]
        build.face([(xa, -ya, zt), (xb, -yb, zt), (xb, yb, zt), (xa, ya, zt)], (0, 0, 1), 'deck')
        build.face([(xa, -ya, zb), (xb, -yb, zb), (xb, yb, zb), (xa, ya, zb)], (0, 0, -1), 'hull')
        for s in (-1, 1):
            build.face([(xa, s * ya, zb), (xb, s * yb, zb), (xb, s * yb, zt), (xa, s * ya, zt)], (0, s, 0), 'hull')
    for x, h, out in ((xs[0], half[0], -1), (xs[-1], half[-1], 1)):
        build.face([(x, -h, zb), (x, h, zb), (x, h, zt), (x, -h, zt)], (out, 0, 0), 'hull')
    log(f'scoop seat: x {x0:.0f}..{front:.0f}, |y| <= {max(half):.0f} (front {half[-1]:.0f}), z {zb:.0f}..{zt:.0f}')
    return [x0, -SCOOP_HALF, zb, front, SCOOP_HALF, zt]


def _scoop_edges(bm, tag):
    """The bow's half-width (cm, per side) at the deck's fore edge at CUT (where its cap meets the guard's end)."""
    cut_edge = {}
    pts = [U(v.co) for v in bm.verts if abs(U(v.co)[0] - DECK_FORE) < rw.EPS * 100.0
           and any(f[tag] == 0 for f in v.link_faces)]
    for s in (-1, 1):
        low = [abs(p[1]) for p in pts if p[1] * s > 0 and abs(p[2] - CUT) < 1.0]
        cut_edge[s] = round(max(low), 1) if low else SCOOP_HALF
    log(f'bow at x {DECK_FORE:.0f}: half-width at z {CUT:.0f} {cut_edge}')
    return cut_edge


def _deck(build):
    inner = BEAM - RAIL_T
    top = DECK + RAIL_H
    g0, g1 = GAP_X - GAP_W / 2.0, GAP_X + GAP_W / 2.0
    build.rect((DECK_AFT, -inner, DECK), (1, 0, 0), (0, 1, 0), DECK_FORE - DECK_AFT, 2 * inner, (0, 0, 1), 'deck')
    # The gangway's sill in the gap.
    build.box_x(g0, g1, -BEAM, -inner, DECK, DECK + SILL_H, 'deck', '+z +y')
    # The bulwark: inner face and top in runs (open at the gap on -Y), the gap's jambs.
    for s, runs in ((1, [(DECK_AFT, DECK_FORE)]), (-1, [(DECK_AFT, g0), (g1, DECK_FORE)])):
        for a, b in runs:
            build.rect((a, s * inner, DECK), (1, 0, 0), (0, 0, 1), b - a, RAIL_H, (0, -s, 0), 'hull')
            build.rect((a, min(s * inner, s * BEAM), top), (1, 0, 0), (0, 1, 0), b - a, RAIL_T, (0, 0, 1), 'hull')
    build.box_x(g0 - RAIL_T, g0, -BEAM, -inner, DECK + SILL_H, top, 'hull', '+x')
    build.box_x(g1, g1 + RAIL_T, -BEAM, -inner, DECK + SILL_H, top, 'hull', '-x')
    # Across the bow beside the scoop's seat: short end walls, so the deck's front edge is closed.
    for s in (-1, 1):
        y0, y1 = sorted((s * SCOOP_HALF, s * inner))
        build.box_x(DECK_FORE - RAIL_T, DECK_FORE, y0, y1, DECK, top, 'hull', '-x +z ' + ('+y' if s < 0 else '-y'))
        y0, y1 = sorted((s * SCOOP_HALF, s * BEAM))
        build.box_x(DECK_FORE - RAIL_T, DECK_FORE, y0, y1, DECK, top, 'hull', '+x')


def _house(build, ledge, stations):
    i_y, i_x0, i_x1, ceil = BEAM - WALL_T, HOUSE_AFT + WALL_T, HOUSE_FORE - WALL_T, ROOF - ROOF_T
    dh, head = DOOR_W / 2.0, DECK + DOOR_H
    # Fore face (outer at HOUSE_FORE from the deck, inner WALL_T aft of it from CUT) round the door.
    _wall(build, (HOUSE_FORE, -BEAM), (0, 1, 0), BEAM - dh, DECK, ROOF, (1, 0, 0))
    _wall(build, (HOUSE_FORE, dh), (0, 1, 0), BEAM - dh, DECK, ROOF, (1, 0, 0))
    _wall(build, (HOUSE_FORE, -dh), (0, 1, 0), 2 * dh, head, ROOF, (1, 0, 0))
    build.rect((i_x1, -i_y, CUT), (0, 1, 0), (0, 0, 1), i_y - dh, ceil - CUT, (-1, 0, 0), 'wall')
    build.rect((i_x1, dh, CUT), (0, 1, 0), (0, 0, 1), i_y - dh, ceil - CUT, (-1, 0, 0), 'wall')
    build.rect((i_x1, -dh, head), (0, 1, 0), (0, 0, 1), 2 * dh, ceil - head, (-1, 0, 0), 'wall')
    build.rect((i_x1, -dh, CUT), (0, 1, 0), (0, 0, 1), 2 * dh, DECK - CUT, (-1, 0, 0), 'wall')
    # The door's jambs (facing into the opening), the lintel's underside, the threshold at the deck.
    for y, out in ((-dh, 1), (dh, -1)):
        build.face([(i_x1, y, DECK), (HOUSE_FORE, y, DECK), (HOUSE_FORE, y, head), (i_x1, y, head)], (0, out, 0), 'wall')
    build.face([(i_x1, -dh, head), (HOUSE_FORE, -dh, head), (HOUSE_FORE, dh, head), (i_x1, dh, head)], (0, 0, -1), 'wall')
    build.face([(i_x1, -dh, DECK), (HOUSE_FORE, -dh, DECK), (HOUSE_FORE, dh, DECK), (i_x1, dh, DECK)], (0, 0, 1), 'deck')
    # Inside: the side and aft walls' inner faces, the ceiling; a ledge at CUT from the hull's cut edge to the walls.
    for s in (-1, 1):
        build.rect((i_x0, s * i_y, CUT), (1, 0, 0), (0, 0, 1), i_x1 - i_x0, ceil - CUT, (0, -s, 0), 'wall')
    build.rect((i_x0, -i_y, CUT), (0, 1, 0), (0, 0, 1), 2 * i_y, ceil - CUT, (1, 0, 0), 'wall')
    build.rect((i_x0, -i_y, ceil), (1, 0, 0), (0, 1, 0), i_x1 - i_x0, 2 * i_y, (0, 0, -1), 'roof')
    build.rect((HOUSE_AFT, -BEAM, ROOF), (1, 0, 0), (0, 1, 0), HOUSE_FORE - HOUSE_AFT, 2 * BEAM, (0, 0, 1), 'roof')
    strip(build, stations, ledge, CUT, i_y, (0, 0, 1), 'deck', STATION, (i_x0, i_x1))
    # The aft face above the deck (below it, the guard's ends and the stern's hull).
    _wall(build, (HOUSE_AFT, -BEAM), (0, 1, 0), 2 * BEAM, DECK, ROOF, (-1, 0, 0))
    # The roof's rim: outer faces up from the walls', its top, its inner faces down to the roof.
    top, t = ROOF + COAMING_H, COAMING_T
    for s in (-1, 1):
        build.rect((HOUSE_AFT, s * BEAM, ROOF), (1, 0, 0), (0, 0, 1), HOUSE_FORE - HOUSE_AFT, COAMING_H, (0, s, 0), 'roof')
        build.rect((HOUSE_AFT + t, s * (BEAM - t), ROOF), (1, 0, 0), (0, 0, 1), HOUSE_FORE - HOUSE_AFT - 2 * t, COAMING_H,
                   (0, -s, 0), 'roof')
        build.rect((HOUSE_AFT, min(s * BEAM, s * (BEAM - t)), top), (1, 0, 0), (0, 1, 0), HOUSE_FORE - HOUSE_AFT, t,
                   (0, 0, 1), 'roof')
    for x, s in ((HOUSE_AFT, -1), (HOUSE_FORE, 1)):
        build.rect((x, -BEAM, ROOF), (0, 1, 0), (0, 0, 1), 2 * BEAM, COAMING_H, (s, 0, 0), 'roof')
        build.rect((x - s * t, -(BEAM - t), ROOF), (0, 1, 0), (0, 0, 1), 2 * (BEAM - t), COAMING_H, (-s, 0, 0), 'roof')
        build.rect((min(x, x - s * t), -(BEAM - t), top), (1, 0, 0), (0, 1, 0), t, 2 * (BEAM - t), (0, 0, 1), 'roof')


def _wall(build, origin, eu, lu, z0, z1, out):
    """A house wall's outer face from z0 to z1 along eu from origin (x, y), in bands: the dark foot, the paint, the
    band under the roof's edge."""
    cuts = [z for z in (DECK + BAND_LO, ROOF - BAND_HI) if z0 < z < z1]
    zs = [z0] + cuts + [z1]
    for a, b in zip(zs, zs[1:]):
        key = 'hull' if b <= DECK + BAND_LO else ('roof' if a >= ROOF - BAND_HI else 'wall')
        build.rect((origin[0], origin[1], a), eu, (0, 0, 1), lu, b - a, out, key)


def _guard(build, guard, stations, cut_edge):
    """The belt round the deck and the house where they overhang Tripo's hull: the outer faces at BEAM (the fascia from
    the chamfer up, and above it the bulwark's or the house's), the chamfer, the underside into the hull, both ends."""
    g0, g1 = GAP_X - GAP_W / 2.0, GAP_X + GAP_W / 2.0
    lo, top = GUARD_LO + GUARD_CHAMFER, DECK + RAIL_H
    for s in (-1, 1):
        spans = [(HOUSE_AFT, HOUSE_FORE, ROOF, 'wall')]
        spans += [(HOUSE_FORE, DECK_FORE, top, 'hull')] if s > 0 else \
            [(HOUSE_FORE, g0, top, 'hull'), (g0, g1, DECK + SILL_H, 'hull'), (g1, DECK_FORE, top, 'hull')]
        for a, b, z1, key in spans:
            build.rect((a, s * BEAM, lo), (1, 0, 0), (0, 0, 1), b - a, DECK - lo, (0, s, 0), 'hull')
            if key == 'wall':
                _wall(build, (a, s * BEAM), (1, 0, 0), b - a, DECK, z1, (0, s, 0))
            else:
                build.rect((a, s * BEAM, DECK), (1, 0, 0), (0, 0, 1), b - a, z1 - DECK, (0, s, 0), key)
        # The chamfer, inward and down from the fascia's foot.
        c = Vector((0.0, -s, -1.0)).normalized()
        build.rect((HOUSE_AFT, s * BEAM, lo), (1, 0, 0), c, DECK_FORE - HOUSE_AFT, GUARD_CHAMFER * math.sqrt(2.0),
                   (0, s, -1), 'hull')
        # Both ends: the guard's section (from inside the hull to the chamfer, up to the deck aft, to CUT at the bow).
        for x, out, k, z_top in ((HOUSE_AFT, -1, 0, DECK), (DECK_FORE, 1, len(stations) - 1, CUT)):
            y_in = guard[s][k]
            for pts in ([(x, s * y_in, GUARD_LO), (x, s * (BEAM - GUARD_CHAMFER), GUARD_LO),
                         (x, s * (BEAM - GUARD_CHAMFER), z_top), (x, s * y_in, z_top)],
                        [(x, s * (BEAM - GUARD_CHAMFER), GUARD_LO), (x, s * BEAM, lo), (x, s * BEAM, z_top),
                         (x, s * (BEAM - GUARD_CHAMFER), z_top)]):
                build.face(pts, (out, 0, 0), 'hull')
        # At the bow, from CUT up to the deck, outside the scoop's cap.
        build.face([(DECK_FORE, s * cut_edge[s], CUT), (DECK_FORE, s * BEAM, CUT), (DECK_FORE, s * BEAM, DECK),
                    (DECK_FORE, s * cut_edge[s], DECK)], (1, 0, 0), 'hull')
    # The aft face between the guard's ends, under the house (mostly inside the stern's hull).
    y_in = min(guard[-1][0], guard[1][0])
    build.rect((HOUSE_AFT, -y_in, GUARD_LO), (0, 1, 0), (0, 0, 1), 2 * y_in, DECK - GUARD_LO, (-1, 0, 0), 'hull')
    # The underside.
    strip(build, stations, guard, GUARD_LO, BEAM - GUARD_CHAMFER, (0, 0, -1), 'hull', STATION)


def ucx(obj, *box):
    rw.ucx(obj, UCX, *box)


def _hull_widths(bm):
    """Each HULL_BOXES box's half-width: the hull skin's narrowest section (both sides, both of its heights) over its x."""
    xs = [-3700.0 + STATION * k for k in range(int(7400.0 / STATION) + 1)]
    lines = {(z, s): outline(bm, z, s, xs, STATION) for _, _, _, z0, z1 in HULL_BOXES for z in (z0, z1) for s in (-1, 1)}
    widths = {}
    for name, x0, x1, z0, z1 in HULL_BOXES:
        ks = [k for k, x in enumerate(xs) if x0 <= x <= x1]
        widths[name] = max(min(lines[(z, s)][k] for z in (z0, z1) for s in (-1, 1) for k in ks) - HULL_BOX_IN, 50.0)
    return widths


def _collision(obj, m, hull, dressing):
    inner, top = BEAM - RAIL_T, DECK + RAIL_H
    g0, g1 = GAP_X - GAP_W / 2.0, GAP_X + GAP_W / 2.0
    ucx(obj, 'deck slab', DECK_AFT, DECK_FORE, -BEAM, BEAM, GUARD_LO, DECK)
    for s, runs in ((1, [(DECK_AFT, RAIL_SPLIT_X), (RAIL_SPLIT_X, DECK_FORE)]),
                    (-1, [(DECK_AFT, g0), (g1, RAIL_SPLIT_X), (RAIL_SPLIT_X, DECK_FORE)])):
        side = '+Y' if s > 0 else '-Y'
        for a, b in runs:
            ucx(obj, f'bulwark {side}', a, b, s * inner, s * BEAM, DECK, top)
        ucx(obj, f'bow end wall {side}', DECK_FORE - RAIL_T, DECK_FORE, s * SCOOP_HALF, s * inner, DECK, top)
    # Tripo's hull below the deck, inside its skin.
    for name, x0, x1, z0, z1 in HULL_BOXES:
        ucx(obj, name, x0, x1, -hull[name], hull[name], z0, z1)
    if m['cradle']:
        c = m['cradle']
        ucx(obj, 'cradle and drum', c[0], c[3], c[1], c[4], c[2], 950.0)
    # The house: below the deck its belt outside the hull; above, walls round the door and the roof; nothing inside.
    for s in (-1, 1):
        ucx(obj, 'house guard', HOUSE_AFT, HOUSE_FORE, s * (BEAM - 350.0), s * BEAM, GUARD_LO, CUT)
        ucx(obj, 'house side wall', HOUSE_AFT, HOUSE_FORE, s * (BEAM - WALL_T), s * BEAM, CUT, ROOF)
        ucx(obj, 'house fore wall', HOUSE_FORE - WALL_T, HOUSE_FORE, s * DOOR_W / 2.0, s * (BEAM - WALL_T), DECK, ROOF)
    ucx(obj, 'house lintel', HOUSE_FORE - WALL_T, HOUSE_FORE, -DOOR_W / 2.0, DOOR_W / 2.0, DECK + DOOR_H, ROOF)
    ucx(obj, 'house aft wall', HOUSE_AFT, HOUSE_AFT + WALL_T, -(BEAM - WALL_T), BEAM - WALL_T, CUT, ROOF)
    ucx(obj, 'house roof', HOUSE_AFT, HOUSE_FORE, -BEAM, BEAM, ROOF - ROOF_T, ROOF)
    # From the roof up: the stacks (shaft, then the rings and cap), the tower (cabin, tanks).
    for (sx, sy), z_top in zip(STACKS, m['stack tops']):
        ucx(obj, 'stack shaft', sx - 205.0, sx + 205.0, sy - 205.0, sy + 205.0, ROOF, 4450.0)
        ucx(obj, 'stack head', sx - 245.0, sx + 245.0, sy - 245.0, sy + 245.0, 4450.0, z_top)
    if m['cabin']:
        c = m['cabin']
        ucx(obj, 'tower cabin', TOWER_DST[0], TOWER_DST[1], c[1], c[4], ROOF, TOWER_CABIN_TOP)
    if m['tanks']:
        c = m['tanks']
        ucx(obj, 'tower tanks', c[0], c[3], c[1], c[4], TOWER_CABIN_TOP, c[5])
    for name in ('scoop', 'seat', 'stern'):
        c = m[name]
        if c:
            ucx(obj, {'seat': 'scoop seat'}.get(name, name), c[0], c[3], c[1], c[4], c[2], c[5])
    # The house's dressing: the catwalk ring and its railing, the door's jambs and steps (walkable).
    for box in dressing:
        ucx(obj, *box)


def preview(obj):
    """Saved/ArtPreviews/DredgeBargeRework/: three-quarter views from the bow and the stern, the top, the deck at eye
    height looking aft at the house and its door, the berth side at the gap, the bow's end at the scoop."""
    import os

    import bpy

    import looter_textures as lt
    import looter_train as tk
    folder = os.path.join(lt.PREVIEW_DIR, 'DredgeBargeRework')
    bpy.context.view_layer.update()      # the object's bounds (the views fit them) after the rework
    tk.render([obj], 'bow', view=(0.9, -1.3, 0.42), lens=40.0, fit=0.75, folder=folder)
    tk.render([obj], 'stern', view=(-0.9, 1.35, 0.38), lens=40.0, fit=0.75, folder=folder)
    tk.closeup([obj], 'top', B((0.0, 0.0, 2500.0)), (0.0, 0.08, 1.0), 110.0, lens=35.0, resolution=(900, 1600),
               folder=folder)
    eye = B((1500.0, 0.0, DECK + 160.0))
    tk.closeup([obj], 'deck', eye + Vector((0.0, 10.0, -0.6)), (0.0, -1.0, 0.06), 10.0, lens=20.0, folder=folder)
    # From the Lamplighter's deck at the berth, 11.6 m out, at the gangway's gap.
    tk.closeup([obj], 'berth', B((GAP_X, -BEAM, DECK + 50.0)), (1.0, 0.0, -0.034), 11.6, lens=26.0, folder=folder)
    tk.closeup([obj], 'bow_end', B((DECK_FORE, 0.0, DECK + 300.0)), (0.35, 0.9, 0.35), 14.0, lens=30.0, folder=folder)
    # From the deck at (600, 0) at eye height, looking at the door.
    tk.closeup([obj], 'door', B((HOUSE_FORE, 0.0, DECK + 150.0)), (0.0, -1.0, 0.01), 10.0, lens=28.0, folder=folder)
    tk.render([obj], 'bow_far', view=(0.3, -1.4, 0.3), lens=40.0, fit=0.8, folder=folder)
