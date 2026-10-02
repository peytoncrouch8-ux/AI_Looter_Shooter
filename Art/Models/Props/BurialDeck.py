"""The burial boards on Gravewind Point (Docs/Areas/RansomsRest.md: Gravewind Point, Main 6, the boss): the deck the
Ransoms laid their dead on, facing the sunset, and the Keeper's Lantern. A scripted model file (Art/README.md) built
with looter_buildings through Tools/Blender/looter_ruins.py.

  BurialDeck         the boss arena at (-150, 12): a flat timber deck 18 x 25 m (X across, Y along), its walking surface
                     0.4 m over the rock. Its front (-Y: face it west) is built 3 m out over the canyon: the joists run
                     on past the rim (the last bearer stands on the rim at y = -9.3; the lip of the cliff should run at
                     y = -9.5) and seven raking struts carry a header under their ends from iron shoes on the cliff face
                     3.5 m below the deck (at y = -9.6: the face must reach them). The front edge is OPEN, no railing:
                     the fight uses it. Low railings close the two sides; the back (+Y, the Keeper's Gate road) has a
                     railing with a 6 m opening and two steps up. The keeper's bench stands by the entrance.
                     Sockets lay out the scene: SOCKET_Bier_1..8 (two rows of four facing the sunset) and
                     SOCKET_LanternPost_1..3 (the two front corners, and the keeper's post by the entrance).
                     Nanite on, its whole mesh the fallback (decks keep a full fallback); the walking surface's
                     collision is full UCX boxes.
  Bier               a burial board on posts, 2.2 x 0.8 m, its top at 0.9 m (waist height; cover in the fight), its foot
                     (-Y) toward the sunset, carrying poles running out past both ends (3 m overall), lashing rings
                     and loose lashings. SOCKET_Sit on the foot of the board, facing the sunset: Abel sits there after
                     the fight. Place 8 at the deck's Bier sockets.
  KeeperLanternPost  a 3.2 m post on a cross base with a braced arm; an iron lantern hangs from it, lit (LanternGlow,
                     SOCKET_Light), and black ribbons tied under the arm stream with the Gravewind. SOCKET_Interact at
                     the lantern (relighting it); SOCKET_Hang, the iron hook below the arm where a keeper hangs his own
                     lantern (the Keeper's Lantern's SOCKET_Grip goes there).
  KeepersLantern     Abel's lantern, the player's held prop: a brass fount and dome, a bulbous glass globe in a black
                     iron guard, a bail with a turned wooden grip. SOCKET_Grip at the grip (the hand), SOCKET_Light at
                     the flame. Its globe is LanternGlow (lit); its UVs sit on the trim's window glass, so setting that
                     slot to MI_HouseTrim shows it dark (Main 5 and 6, before Abel lights it).

The lantern glass on the post and in the held lantern is the shared LanternGlow (an emissive surface, never a real
light): the level adds a shadowless point light where SOCKET_Light is, at most three in view.
"""
import math
import random

from mathutils import Matrix, Vector, noise

import looter_buildings as kit
from looter_buildings import Trim
import looter_props as lp
import looter_ruins as lr
import looter_textures as lt

ONLY = next((a.split('=', 1)[1].split(',') for a in kit._args() if a.startswith('--only=')), None)
DECK = 0.4                                     # the deck's walking surface above the rock
HX, Y0, Y1 = 9.0, -12.5, 12.5                  # the deck: x -9..9, y -12.5 (the open front, over the drop)..12.5
RIM = -9.5                                     # the cliff's lip under the deck
BIERS = [(x, y) for y in (-4.8, 1.6) for x in (-6.3, -2.1, 2.1, 6.3)]
POSTS = [(-7.6, -8.6), (7.6, -8.6), (-4.6, 9.4)]


# --- The deck ---

def burial_deck(name='BurialDeck', seed=701):
    m = lr.Model(name, seed=seed)
    rng = m.rng
    PLANK, JOIST, BEARER = 0.05, 0.18, 0.17
    beam = Trim('C', lane='each')
    iron = Trim('H3', fit=True)
    jz = DECK - PLANK - JOIST * 0.5
    bz = DECK - PLANK - JOIST - BEARER * 0.5

    # Bearers across the point on stone pads; the last one stands on the rim.
    bearers = [RIM + 0.3 + 2.45 * k for k in range(10)]
    for y in bearers:
        m.board((-HX + 0.1, y, bz), (HX - 0.1, y, bz), BEARER, 0.2, uv=beam)
        for x in (-7.8, 0.0, 7.8):
            m.add(lr.stone_block((0.42, 0.42, 0.18), (x + rng.uniform(-0.1, 0.1), y, -0.04),
                                 (0.0, 0.0, rng.uniform(0, 90)), seed=rng.randrange(1 << 20), rough=0.01, bevel=0.0,
                                 key='trim', strip='Stone'))
    # Joists along the deck, running out over the drop at the front; the rim boards round the edge.
    joists = [-HX + 0.08 + (2 * HX - 0.16) * k / 20 for k in range(21)]
    for x in joists:
        m.board((x, Y0 + 0.06, jz), (x, Y1 - 0.06, jz), JOIST, 0.12, face=(1.0, 0.0, 0.0), uv=beam, cuts=2)
    for y, face in ((Y0 + 0.04, (0.0, -1.0, 0.0)), (Y1 - 0.04, (0.0, 1.0, 0.0))):
        m.board((-HX, y, jz), (HX, y, jz), JOIST + 0.03, 0.07, face=face, uv=beam, cuts=3)
    for x, face in ((-HX + 0.03, (-1.0, 0.0, 0.0)), (HX - 0.03, (1.0, 0.0, 0.0))):
        m.board((x, Y0, jz), (x, Y1, jz), JOIST + 0.03, 0.06, face=face, uv=beam, cuts=4)
    m.section('frame')

    # Over the drop: a header under the joists' ends, seven raking struts down to iron shoes on the cliff face, a
    # waler across them and saltire braces between, so the cantilever reads from the canyon and from the boards.
    hz = jz - JOIST * 0.5 - 0.11
    m.board((-HX + 0.2, Y0 + 0.35, hz), (HX - 0.2, Y0 + 0.35, hz), 0.22, 0.2, uv=beam, cuts=6)
    FOOT_Y, FOOT_Z = RIM - 0.1, DECK - 3.5
    struts = [-8.4 + 2.8 * k for k in range(7)]
    for x in struts:
        top, foot = Vector((x, Y0 + 0.45, hz - 0.08)), Vector((x, FOOT_Y, FOOT_Z))
        m.board(foot, top, 0.22, 0.2, face=(1.0, 0.0, 0.0), uv=beam, cuts=2)
        m.box((0.32, 0.06, 0.34), at=(x, FOOT_Y + 0.05, FOOT_Z - 0.02), uv=iron)
        m.box((0.34, 0.3, 0.05), at=(x, FOOT_Y - 0.12, FOOT_Z - 0.17), uv=iron)
    mid = 0.5
    wy, wz = FOOT_Y + (Y0 + 0.45 - FOOT_Y) * mid, FOOT_Z + (hz - 0.08 - FOOT_Z) * mid
    m.board((struts[0] - 0.2, wy - 0.13, wz), (struts[-1] + 0.2, wy - 0.13, wz), 0.18, 0.1, uv=beam, cuts=6)
    for a, b in zip(struts[::2], struts[1::2]):
        p0, p1 = Vector((a, FOOT_Y - 0.05, FOOT_Z + 0.4)), Vector((b, wy + 0.05, wz))
        m.board(p0 + Vector((0.0, 0.12, 0.0)), p1 + Vector((0.0, 0.12, 0.0)), 0.12, 0.08,
                face=(0.0, -0.8, 0.6), uv='C')
    m.section('cantilever')

    # Boards across the deck in two staggered lengths a row (the texture draws the shorter joints), broken over a joist;
    # those over the drop keep their undersides (seen from the canyon), the rest leave them out. Open sky over all of
    # them bakes even, so they need no extra vertices along them.
    y = Y0 + 0.1
    row = 0
    while y < Y1 - 0.05:
        split = joists[7 + (row * 7) % 7]
        pieces = [(-HX - rng.uniform(0.0, 0.03), split), (split, HX + rng.uniform(0.0, 0.03))]
        over_drop = y < RIM + 0.1
        for xa, xb in pieces:
            length = xb - xa - 0.006
            tb = kit._box((length, PLANK, 0.19), 0.0, 0, drop=() if over_drop else ('+y',))
            lift = rng.uniform(-0.004, 0.006)
            matrix = kit.place(((xa + xb) * 0.5, y + rng.uniform(-0.006, 0.006), DECK - PLANK * 0.5 + lift),
                               (-90.0 + rng.uniform(-0.4, 0.4), rng.uniform(-0.25, 0.25), rng.uniform(-0.1, 0.1)))
            m.emit(tb, lr.PlankRow(rng.randrange(16)), 'planks', matrix)
        y += 0.2
        row += 1
    # The walking surface: full boxes, front to back.
    for y0, y1 in ((Y0, -4.0), (-4.0, 4.5), (4.5, Y1)):
        m.hull((2 * HX, y1 - y0, 0.4), at=(0.0, (y0 + y1) * 0.5, DECK - 0.2))
    m.section('boards')

    # Low railings: the sides whole, the back open in the middle for the steps; the front open (it's part of the fight).
    RAIL = 0.95
    lr.railing(m, (-HX + 0.06, Y0 + 0.08, DECK), (-HX + 0.06, Y1 - 0.08, DECK), RAIL, face=(-1.0, 0.0, 0.0),
               post_every=2.5, seed=seed, cuts=2)
    lr.railing(m, (HX - 0.06, Y0 + 0.08, DECK), (HX - 0.06, Y1 - 0.08, DECK), RAIL, face=(1.0, 0.0, 0.0),
               post_every=2.5, seed=seed + 1, cuts=2)
    GATE = 3.0
    for xa, xb in ((-HX + 0.06, -GATE), (GATE, HX - 0.06)):
        lr.railing(m, (xa, Y1 - 0.08, DECK), (xb, Y1 - 0.08, DECK), RAIL, face=(0.0, 1.0, 0.0), post_every=2.0,
                   ends=(False, False), seed=seed + 2, cuts=1)
    for p0, p1 in (((-HX + 0.06, Y0), (-HX + 0.06, Y1)), ((HX - 0.06, Y0), (HX - 0.06, Y1)),
                   ((-HX, Y1 - 0.08), (-GATE, Y1 - 0.08)), ((GATE, Y1 - 0.08), (HX, Y1 - 0.08))):
        lr.hull_along(m, (p0[0], p0[1], DECK), (p1[0], p1[1], DECK), RAIL + 0.05, thick=0.14)
    # Two thick gateposts at the opening, taller than the rail, with iron caps.
    for x in (-GATE, GATE):
        m.board((x, Y1 - 0.1, -0.2), (x, Y1 - 0.1, DECK + 1.45), 0.2, 0.2, uv=beam, bevel=0.015)
        m.box((0.26, 0.26, 0.05), at=(x, Y1 - 0.1, DECK + 1.47), uv=iron)
        m.hull((0.22, 0.22, 1.45), at=(x, Y1 - 0.1, DECK + 0.72))
    m.section('railings')

    # Two steps up from the road at the opening.
    for k, (z, depth) in enumerate(((DECK * 2.0 / 3.0, 0.34), (DECK / 3.0, 0.34))):
        yy = Y1 + 0.17 + depth * k
        m.board((-GATE + 0.1, yy, z - 0.03), (GATE - 0.1, yy, z - 0.03), depth, 0.06, face=(0.0, 0.0, 1.0),
                uv=lr.PlankRow(rng.randrange(16)), mat='planks')
        m.board((-GATE + 0.2, yy, (z - 0.06) * 0.5), (GATE - 0.2, yy, (z - 0.06) * 0.5), z - 0.06, 0.1,
                face=(0.0, 1.0, 0.0), uv=beam)
    m.hull_points([(x, y, z) for x in (-GATE + 0.1, GATE - 0.1)
                   for y, z in ((Y1 - 0.1, DECK), (Y1 - 0.1, DECK - 0.3), (Y1 + 0.9, 0.0), (Y1 + 0.9, -0.1))])
    # The keeper's bench by the entrance, where the keeper sat the night with his dead.
    b0, b1, by = -8.6, -6.2, Y1 - 0.75
    for yy in (by - 0.09, by + 0.09):
        m.board((b0, yy, DECK + 0.45), (b1, yy, DECK + 0.45), 0.17, 0.05, face=(0.0, 0.0, 1.0), uv='A')
    for x in (b0 + 0.2, b1 - 0.2):
        m.board((x, by, DECK), (x, by, DECK + 0.42), 0.34, 0.06, face=(1.0, 0.0, 0.0), uv='C')
    m.hull((b1 - b0, 0.4, 0.48), at=((b0 + b1) * 0.5, by, DECK + 0.24))
    m.section('steps and bench')

    # The scene's layout.
    for k, (x, y) in enumerate(BIERS):
        m.socket(f'Bier_{k + 1}', (x, y, DECK), (0.0, 0.0, (-2.5, 1.5, -1.0, 2.0, 1.0, -2.0, 2.5, -1.5)[k]))
    for k, (x, y) in enumerate(POSTS):
        m.socket(f'LanternPost_{k + 1}', (x, y, DECK), (0.0, 0.0, 0.0))
    return m.finish(view=(-1.3, -1.0, 0.75), fit=0.85, fallback=100.0, ao_distance=1.0,
                    out=lr.preview_path(name))


# --- The bier ---

def bier(name='Bier', seed=711):
    """A burial board on posts, its foot (-Y) toward the sunset."""
    m = lr.Model(name, seed=seed)
    rng = m.rng
    L, WD, TOP = 2.2, 0.8, 0.9
    T = 0.06
    beam = Trim('C', lane='each')
    # Four posts splayed a little, two cross rails under the board, a stretcher low along each side.
    px, py = WD * 0.5 - 0.1, L * 0.5 - 0.22
    for sx in (-1, 1):
        for sy in (-1, 1):
            m.board((sx * (px + 0.06), sy * (py + 0.04), -0.04), (sx * px, sy * py, TOP - T - 0.05), 0.11, 0.11,
                    uv=beam, bevel=0.01)
    for sy in (-1, 1):
        m.board((-px, sy * py, TOP - T - 0.06), (px, sy * py, TOP - T - 0.06), 0.12, 0.1,
                uv=beam)
    for sx in (-1, 1):
        m.board((sx * (px + 0.045), -py - 0.1, 0.24), (sx * (px + 0.045), py + 0.1, 0.24), 0.1, 0.06,
                face=(sx, 0.0, 0.0), uv='C')
    # The board: four planks on two battens, worn smooth; a raised head block at the head (+Y).
    for k in range(4):
        x = -WD * 0.5 + 0.1 + 0.2 * k
        m.board((x, -L * 0.5 - rng.uniform(0.0, 0.02), TOP - T * 0.5), (x, L * 0.5 + rng.uniform(0.0, 0.02),
                TOP - T * 0.5), 0.195, T, face=(0.0, 0.0, 1.0), uv=Trim('A', lane=rng.randrange(4)), cuts=2)
    for sy in (-0.6, 0.6):
        m.board((-WD * 0.5 + 0.06, sy, TOP - T - 0.025), (WD * 0.5 - 0.06, sy, TOP - T - 0.025), 0.08, 0.05,
                face=(0.0, 0.0, 1.0), uv='C')
    m.box((WD - 0.16, 0.22, 0.08), at=(0.0, L * 0.5 - 0.16, TOP + 0.04), uv=Trim('C', lane=1), bevel=0.015)
    # The carrying poles along both sides, running out past the ends into worn grips: the board is carried to the
    # point with its dead and set down on its posts, and the poles are what make it read as a bier.
    for sx in (-1, 1):
        x = sx * (px + 0.075)
        pts = [(x, -L * 0.5 - 0.42, TOP - T - 0.06), (x, -L * 0.5 - 0.1, TOP - T - 0.04),
               (x, L * 0.5 + 0.1, TOP - T - 0.04), (x, L * 0.5 + 0.42, TOP - T - 0.06)]
        m.add(lr.tube(pts, 0.04, sides=6, strip='C', lane=1 + (sx > 0), radii=[0.03, 0.04, 0.04, 0.03]),
              smooth=True)
    # Iron rings at the corners for the lashings, a frayed lashing still knotted to one.
    iron = Trim('H3', fit=True)
    for sx in (-1, 1):
        for sy in (-1, 1):
            x, y = sx * (WD * 0.5 + 0.005), sy * (L * 0.5 - 0.3)
            m.box((0.02, 0.07, 0.05), at=(x, y, TOP - 0.03), uv=iron)
            ring = [(x + sx * 0.012, y + 0.045 * math.cos(a), TOP - 0.075 + 0.045 * math.sin(a))
                    for a in [2.0 * math.pi * k / 10 for k in range(11)]]
            m.add(lr.tube(ring, 0.007, sides=4, strip='Iron', caps=(False, False)), smooth=True)
    # The lashings that held the dead against the wind, cast loose across the board and hanging down its sides.
    for k, (y, skew) in enumerate(((-0.45, 0.12), (0.35, -0.1))):
        pts = [(-WD * 0.5 - 0.03, y - skew - 0.02, TOP - 0.35), (-WD * 0.5 - 0.035, y - skew, TOP - 0.05),
               (-WD * 0.5 + 0.02, y - skew * 0.8, TOP + 0.012), (0.0, y, TOP + 0.016),
               (WD * 0.5 - 0.02, y + skew * 0.8, TOP + 0.012), (WD * 0.5 + 0.035, y + skew, TOP - 0.05),
               (WD * 0.5 + 0.04, y + skew + 0.03, TOP - 0.26 - 0.08 * k)]
        m.add(lr.tube(pts, 0.014, sides=5, lane=1 + k, seed=seed + 1 + k, wobble=0.0015), smooth=True)
        for part in lr.frayed(pts[-1], Vector(pts[-1]) - Vector(pts[-2]), 0.014, seed + 3 + k, length=0.07):
            m.add(part, smooth=True)
    m.hull((WD + 0.04, L + 0.04, TOP), at=(0.0, 0.0, TOP * 0.5))
    m.hull((2 * px + 0.23, L + 0.84, 0.1), at=(0.0, 0.0, TOP - T - 0.05))          # the carrying poles
    # Abel's seat: on the foot of the board, facing the sunset.
    m.socket('Sit', (0.0, -L * 0.5 + 0.2, TOP), (0.0, 0.0, 0.0))
    return m.finish(view=(-1.0, -1.4, 0.7), fit=1.0, fallback=None, ao_distance=0.5, out=lr.preview_path(name),
                    Nanite=0, LODs='50,25')


# --- The keeper's lantern post ---

def ribbon(m, top, length, width, seed, lean=Vector((0.0, -1.0, 0.0))):
    """A black mourning ribbon hanging from top and streaming with the wind toward lean: two thin layers back to back
    (so it shows from both sides), mapped along its length."""
    rnd = random.Random(seed)
    bm = lp.new_bmesh()
    uv_layer = bm.loops.layers.uv.active
    steps = 6
    rows = []
    d = Vector(lean).normalized()
    side = d.cross(Vector((0.0, 0.0, 1.0))).normalized()
    for i in range(steps + 1):
        t = i / steps
        flutter = 0.06 * math.sin(t * 7.0 + seed) * t
        c = Vector(top) + d * (length * 0.55 * t ** 1.3) + Vector((0.0, 0.0, -length * 0.8 * t)) + side * flutter
        twist = 0.6 * t + rnd.uniform(-0.1, 0.1)
        w = side * math.cos(twist) * width * 0.5 + Vector((0.0, 0.0, 1.0)) * math.sin(twist) * width * 0.25
        rows.append((c - w, c + w))
    scale = lr.tile_scale('Polymer')
    for layer, offset in ((0, 0.0), (1, 0.003)):
        verts = [(bm.verts.new(a + d * offset), bm.verts.new(b + d * offset)) for a, b in rows]
        for i in range(steps):
            quad = (verts[i][0], verts[i][1], verts[i + 1][1], verts[i + 1][0])
            face = bm.faces.new(quad if layer else tuple(reversed(quad)))
            for loop in face.loops:
                k = next(j for j, pair in enumerate(verts) if loop.vert in pair)
                u = (0.0 if loop.vert is verts[k][0] else width) * scale
                loop[uv_layer].uv = (u, k / steps * length * scale)
    part = lp.mesh_object(bm)
    lt.assign(part, lr.material('crepe'))
    m.add(part, smooth=True)


def keeper_lantern_post(name='KeeperLanternPost', seed=721):
    m = lr.Model(name, seed=seed)
    rng = m.rng
    TOP = 3.2
    beam = Trim('C', lane='each')
    iron = Trim('H3', fit=True)
    # The post on a cross of sills, braced, bolted down to the boards.
    m.board((0.0, 0.0, 0.0), (0.0, 0.0, TOP), 0.18, 0.18, uv=beam, bevel=0.018, cuts=3)
    for a in (0.0, 90.0):
        d = Vector((math.cos(math.radians(a)), math.sin(math.radians(a)), 0.0))
        m.board(-d * 0.55 + Vector((0.0, 0.0, 0.06)), d * 0.55 + Vector((0.0, 0.0, 0.06)), 0.12, 0.14,
                face=(-d.y, d.x, 0.0), uv=beam)
        for s in (-1.0, 1.0):
            m.board(d * s * 0.46 + Vector((0.0, 0.0, 0.1)), d * s * 0.08 + Vector((0.0, 0.0, 0.62)), 0.08, 0.07,
                    face=(-d.y, d.x, 0.0), uv='C')
            m.box((0.07, 0.07, 0.025), at=tuple(d * s * 0.42 + Vector((0.0, 0.0, 0.135))), uv=iron)
    # The arm toward the front (-Y) with its brace, an iron strap over the joint, a hook at its end.
    ARM = 0.95
    m.board((0.0, 0.08, TOP - 0.12), (0.0, -ARM, TOP - 0.12), 0.13, 0.1, face=(1.0, 0.0, 0.0), uv=beam)
    m.board((0.0, -0.06, TOP - 0.75), (0.0, -ARM * 0.55, TOP - 0.17), 0.09, 0.08, face=(1.0, 0.0, 0.0), uv='C')
    m.box((0.2, 0.22, 0.03), at=(0.0, -0.02, TOP - 0.045), uv=iron)
    m.box((0.21, 0.03, 0.2), at=(0.0, -0.1, TOP - 0.13), uv=iron)
    hook = Vector((0.0, -ARM + 0.08, TOP - 0.19))
    m.add(lr.tube([hook + Vector((0.0, 0.0, 0.06)), hook, hook + Vector((0.0, -0.035, -0.03)),
                   hook + Vector((0.0, -0.035, -0.07))], 0.01, sides=4, strip='Iron'), smooth=True)
    # The lantern on a short chain of links.
    links_top = hook.z - 0.06
    for k in range(3):
        z = links_top - 0.045 * k
        ring = [(hook.x + (0.012 * math.cos(a) if k % 2 else 0.0), hook.y - 0.035 + (0.0 if k % 2 else
                 0.012 * math.cos(a)), z + 0.022 * math.sin(a)) for a in [2.0 * math.pi * j / 6 for j in range(7)]]
        m.add(lr.tube(ring, 0.005, sides=3, strip='Iron', caps=(False, False)))
    parts, ring_top, glass = lr.iron_lantern((hook.x, hook.y - 0.035, links_top - 0.5), seed + 3, key_iron='trim',
                                             size=1.15)
    shift = Vector((0.0, 0.0, links_top - 0.12 - ring_top.z))
    for part in parts:
        lp.place(part, tuple(shift))
        m.add(part)
    glass = glass + shift
    m.socket('Light', tuple(glass))
    m.socket('Interact', tuple(glass))
    # The keeper's own hook, low on the post's front face, where he hangs his lantern while he sits up.
    hang = Vector((0.0, -0.09, 1.78))
    m.box((0.05, 0.02, 0.12), at=(0.0, -0.095, hang.z + 0.05), uv=iron)
    m.add(lr.tube([(0.0, -0.1, hang.z + 0.06), (0.0, -0.17, hang.z + 0.05), (0.0, -0.19, hang.z - 0.0),
                   (0.0, -0.165, hang.z - 0.035)], 0.009, sides=4, strip='Iron'), smooth=True)
    m.socket('Hang', (0.0, -0.18, hang.z), (0.0, 0.0, 0.0))
    # Black ribbons tied under the arm, streaming west with the Gravewind.
    m.box((0.2, 0.2, 0.05), at=(0.0, 0.0, TOP - 0.36), uv=Trim('C', lane=2))
    for k, (dx, ln) in enumerate(((-0.06, 0.9), (0.05, 0.7), (0.0, 1.05))):
        ribbon(m, (dx, -0.11, TOP - 0.36), ln, 0.075, seed + 10 + k, lean=Vector((dx * 2.0, -1.0, 0.0)))
    m.hull((0.2, 0.2, TOP), at=(0.0, 0.0, TOP * 0.5))
    m.hull((1.2, 1.2, 0.14), at=(0.0, 0.0, 0.07))
    return m.finish(view=(-1.2, -1.2, 0.45), fit=1.0, fallback=None, ao_distance=0.5, out=lr.preview_path(name),
                    Nanite=0, LODs='50,25')


# --- The Keeper's Lantern ---

def keepers_lantern(name='KeepersLantern', seed=731):
    """The held lantern: built to be seen at arm's length in first person."""
    m = lr.Model(name, seed=seed)
    SEG = 20
    # The brass fount: a flat drum with a rolled foot and a filler cap; the burner collar on top.
    fount, _ = lr.lathe([(0.0, 0.0), (0.072, 0.0), (0.082, 0.006), (0.086, 0.018), (0.086, 0.034), (0.08, 0.044),
                         (0.07, 0.05), (0.062, 0.052), (0.062, 0.06), (0.0, 0.06)], SEG, 'brass', set_name='MetalWorn',
                        seed=seed)
    m.add(fount, smooth=True)
    cap, _ = lr.lathe([(0.0, 0.05), (0.012, 0.05), (0.012, 0.066), (0.0, 0.07)], 8, 'brass', set_name='MetalWorn',
                      seed=seed + 1, center=(0.05, 0.0, 0.0))
    m.add(cap, smooth=True)
    # The globe: bulbous glass between the collar and the dome.
    globe, _ = lr.lathe([(0.0, 0.058), (0.056, 0.058), (0.07, 0.08), (0.078, 0.11), (0.08, 0.135), (0.076, 0.165),
                         (0.064, 0.192), (0.05, 0.21), (0.0, 0.21)], SEG, 'glow', strip='Glass', seed=seed + 2)
    m.add(globe, smooth=True)
    m.socket('Light', (0.0, 0.0, 0.122))
    # The guard: black iron wires bowed round the globe from the collar to the dome, and a ring round its middle.
    for k in range(4):
        a = math.pi * 0.25 + math.pi * 0.5 * k
        d = Vector((math.cos(a), math.sin(a), 0.0))
        pts = [d * r + Vector((0.0, 0.0, z)) for r, z in ((0.066, 0.056), (0.084, 0.08), (0.094, 0.11), (0.096, 0.135),
                                                         (0.092, 0.165), (0.078, 0.192), (0.064, 0.214))]
        m.add(lr.tube(pts, 0.0045, sides=5, key='ironblack', set_name='MetalWorn', seed=seed + 3 + k), smooth=True)
    guard = [(0.097 * math.cos(a), 0.097 * math.sin(a), 0.135)
             for a in [2.0 * math.pi * j / SEG for j in range(SEG + 1)]]
    m.add(lr.tube(guard, 0.005, sides=5, key='ironblack', set_name='MetalWorn', caps=(False, False)), smooth=True)
    for z, r in ((0.057, 0.07), (0.213, 0.066)):
        band = [(r * math.cos(a), r * math.sin(a), z) for a in [2.0 * math.pi * j / SEG for j in range(SEG + 1)]]
        m.add(lr.tube(band, 0.006, sides=5, key='ironblack', set_name='MetalWorn', caps=(False, False)), smooth=True)
    # The dome: a brass collar, a vented hood and the chimney cap, with a little sun-ring finial (the keepers' mark).
    dome, _ = lr.lathe([(0.0, 0.21), (0.068, 0.21), (0.074, 0.218), (0.072, 0.232), (0.062, 0.25), (0.045, 0.27),
                        (0.03, 0.282), (0.028, 0.3), (0.042, 0.304), (0.044, 0.312), (0.03, 0.322), (0.0, 0.326)], SEG,
                       'brass', set_name='MetalWorn', seed=seed + 8)
    m.add(dome, smooth=True)
    for k in range(8):           # vent slots round the hood, dark
        a = 2.0 * math.pi * k / 8
        c = Vector((math.cos(a) * 0.058, math.sin(a) * 0.058, 0.252))
        m.box((0.012, 0.022, 0.012), matrix=Matrix.LocRotScale(c, Matrix.Rotation(a, 3, 'Z').to_quaternion(), None)
              @ Matrix.Rotation(math.radians(-38.0), 4, 'Y'), uv=Trim('H4', fit=True))
    sun = [(0.018 * math.cos(a), 0.0, 0.345 + 0.018 * math.sin(a)) for a in [2.0 * math.pi * j / 12 for j in range(13)]]
    m.add(lr.tube(sun, 0.0035, sides=4, key='brass', set_name='MetalWorn', caps=(False, False)), smooth=True)
    m.cylinder((0.0, 0.0, 0.322), (0.0, 0.0, 0.33), 0.006, sides=6, uv=kit.Tile('MetalWorn'), mat='brass')
    # The bail: ears on the collar, a black iron wire arching over the dome, a turned wooden grip at its top.
    for s in (-1.0, 1.0):
        m.box((0.01, 0.018, 0.03), at=(s * 0.074, 0.0, 0.222), uv=kit.Tile('MetalWorn'), mat='brass')
    bail = []
    for k in range(17):
        t = math.pi * k / 16
        bail.append((0.079 * math.cos(t), 0.0, 0.222 + 0.205 * math.sin(t) ** 0.85))
    m.add(lr.tube(bail, 0.0045, sides=6, key='ironblack', set_name='MetalWorn', seed=seed + 9), smooth=True)
    grip, _ = lr.lathe([(0.0, -0.05), (0.011, -0.05), (0.015, -0.035), (0.016, 0.0), (0.015, 0.035), (0.011, 0.05),
                        (0.0, 0.05)], 10, 'trim', strip='Beams', seed=seed + 10)
    lp.place(grip, (0.0, 0.0, 0.0), (0.0, 90.0, 0.0))
    lp.place(grip, (0.0, 0.0, 0.427))
    m.add(grip, smooth=True)
    m.socket('Grip', (0.0, 0.0, 0.427), (0.0, 0.0, 0.0))
    m.hull_points([(0.088 * math.cos(a), 0.088 * math.sin(a), z) for a in [2.0 * math.pi * j / 8 for j in range(8)]
                   for z in (0.0, 0.32)])
    return m.finish(view=(-1.0, -1.5, 0.45), fit=1.15, fallback=None, ao_distance=0.12, out=lr.preview_path(name),
                    Nanite=0, LODs='50,25')


BUILDERS = [('BurialDeck', burial_deck), ('Bier', bier), ('KeeperLanternPost', keeper_lantern_post),
            ('KeepersLantern', keepers_lantern)]
models = [build() for name, build in BUILDERS if ONLY is None or name in ONLY]
