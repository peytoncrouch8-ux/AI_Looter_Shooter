"""Ransom's Rest's own frontier homes (Docs/Areas/RansomsRest.md): ten dwellings, each built once, so a player walking the
valley never meets the same house twice (the user's call, 2026-10-13; Skyreach keeps the older shared models). Half
the town fled when its saint went dark, so half of them are lived in (lamplit windows behind linen curtains, smoke
from the chimney, crepe on a door or two) and half stand empty (boarded windows, shutters hanging, a porch gone slack,
a hole in the roof); all of them are closed, solid and walkable round.

  RR_Home_A  the saltbox (lived in): two storeys at the front, one at the back under a long sweep of shakes; cream
             clapboards with oxide-red trim, a big fieldstone chimney through the ridge, a pedimented hood over a
             panelled door hung with crepe, stone steps, a bench, a rain barrel and the winter's wood out back.
  RR_Home_B  the dogtrot (lived in): two pens of hewn logs with white chinking under one long roof, an open breezeway
             between them (walk straight through), a porch the whole length, a fieldstone chimney up each end.
  RR_Home_C  the shotgun house (empty): one room wide and three deep, its gable to the street; tarred-black board-and-
             batten (WoodBlack) on stone piers behind a skirting with boards gone, every window boarded or shuttered, the
             teal door nailed shut, a porch whose corner post lies on its deck so its board roof hangs slack, and a hole
             in the shakes over the back room, the dark attic under it.
  RR_Home_D  the gallery house (lived in): two full storeys in oxide-red clapboards with cream trim, a double gallery
             across the front (a porch below, a railed balcony above, a door onto it), a chimney at each gable end.
  RR_Home_E  the dugout (lived in): a homesteader's house dug under a mound of sod (dry grass, Hay, on a convex mound
             a player can walk up), its front of upright split logs between two dry-stone wing walls, a stovepipe
             through the grass, an old wagon sheet of canvas on poles before the door, the winter's wood stacked by it.
  RR_Home_F  the stone house (empty): fieldstone walls with quoins and timber lintels, a steep sagging roof fallen in
             at one end, a chimney stack rising out of its gable, shutters closed, one hanging, a woodshed collapsing
             against its side.
  RR_Home_G  the claim cabin (empty): a small grey board-and-batten cabin, its gable to the front, roofed in battened
             boards running down the slope with a few gone, a lean-to of rough boards on its side with the stovepipe, a
             hayloft hatch under a hoist pole, on stone piers.
  RR_Home_H  the porch house (lived in): a square house of faded teal clapboards (WoodTeal) under a pyramid of shakes,
             its chimney out of the peak, a porch on turned posts wrapping the front and one side under a hipped roof,
             cream trim and shutters.
  RR_Home_I  the L house (empty): a storey and a half of weathered grey clapboards with a gabled wing to the street; in
             the crook a porch that has lost its corner post and sags, the roof broken in over the front room.
  RR_Home_J  the shack (empty): one room of rough planks nailed level under a shed roof of planks lapped across it, an
             awning of boards over the door on two poles (one knocked askew), the stovepipe out through the wall with an
             elbow, a chopping block and a rain barrel.

And the Rest's own yard pieces, so none of Skyreach's stand here either:

  RR_Well         a sweep well: a crib of hewn timbers over the shaft, a forked post carrying the long sweep pole on an
                  iron pin, a stone lashed to its short end, the bucket on a pole from its long end; a plank trough.
                  Pivot: the middle of the crib on the ground.
  RR_Outhouse     a privy of grey board-and-batten under a little gable of shakes, a star cut through its door (dark
                  behind), a vent slot in the gable. Pivot: its door on the ground, like the homes.
  RR_Windmill     a windpump on a tower of hewn timbers with board bracing, a ladder to a railed platform, the head on a
                  turntable with a tail boom and a board vane in faded oxide, the pump rod down to an iron pump, and a
                  stave tank on a timber stand beside it, fed by a pipe. SOCKET_Fan is the hub. Pivot: the middle of
                  the tower's foot on the ground.
  RR_WindmillFan  its wheel of wooden sails, built round its hub facing -Y (SM_WindmillFan's frame), turned by the game
                  about its front axis: no Nanite (LODs), no collision.

Tin is used nowhere: the trim sheet's corrugated strip reads as loot colours (blue sheets, orange rust), and MetalRust's
grey-green and orange too, so stovepipes and hoops are black iron (IronBlack).

Every home is a scripted model (Art/README.md) built with Tools/Blender/looter_homes.py (on looter_buildings and
looter_town): the house trim sheet and the shared tileables and tints (no new texture sets or tints), the lived-in
houses' lamplit panes on WindowGlow (Farmhouse.py's material: the level's AHouseLights raises its Glow at dusk and lights
a lamp at SOCKET_Light, a metre behind the front windows; the empty houses have neither), SOCKET_Window_<n> on each
lamplit pane of a lived-in home (the pane's outer face, its middle, front out through the glass: UWindowLifeSubsystem
shows somebody at home there, a shadow crossing, a curtain twitching; numbered in build order), SOCKET_Smoke on every
chimney or stovepipe (SOCKET_Smoke_2 on a second chimney), convex UCX_ hulls, baked AO, and Nanite's fallback keeping
every triangle (the buildings' rule).

The pivot is the front door at ground level: the middle of the doorway on the front wall's outer face, the ground there
z = 0 (foundations and piers reach 0.35 m below it). The front faces -Y (+X in Unreal). Each home's report line
(HOMES: ...) gives its bounds and its walls' extent from the pivot, so the level can lay it on a lot.

  --preview   renders each home in the area's golden late-afternoon light into Saved/ArtPreviews/RansomsRest/Homes/,
              the kit in a row, and the look sheet Saved/ArtPreviews/RansomsRest/LookSheets/35_Homes.jpg
  --only A,C  builds just those homes (while working on them; no look sheet); --no-ao skips the AO bake; --stats prints
              where the triangles go
"""
import math

import bmesh

import looter_buildings as kit
import looter_homes as homes
import looter_textures as lt
import looter_town as town
from looter_buildings import Matrix, Opening, Space, Tile, Trim, Vector, place

ARGS = homes.args()
ONLY = []
if '--only' in ARGS:
    for arg in ARGS[ARGS.index('--only') + 1:]:      # artrun.ps1 splits its -ScriptArgs at commas: A,C comes as A C
        if arg.startswith('--'):
            break
        ONLY += [k.strip().upper() for k in arg.split(',') if k.strip()]
LAP = 0.026          # how far clapboards stand out of the wall's face (casings sit on them)


# --- Shared steps ---

def wall_spaces(x0, x1, y0, y1, base):
    """The four walls of a rectangle, counterclockwise from above (front, right, back, left): (space, length)."""
    walls = [((x0, y0), (x1, y0)), ((x1, y0), (x1, y1)), ((x1, y1), (x0, y1)), ((x0, y1), (x0, y0))]
    return [(kit.wall_space(p0, p1, base), (Vector(p1) - Vector(p0)).length) for p0, p1 in walls]


def polygon_spaces(points, base):
    """Walls round a polygon given counterclockwise from above: (space, length) for each edge."""
    out = []
    for p0, p1 in zip(points, points[1:] + points[:1]):
        out.append((kit.wall_space(p0, p1, base), (Vector(p1) - Vector(p0)).length))
    return out


def rect(length, height):
    return [(0.0, 0.0), (length, 0.0), (length, height), (0.0, height)]


def gable_outline(length, height, rise, apex=None):
    """A gable wall's outline: eaves at height, the apex rise over them at apex along it (the middle by default)."""
    return [(0.0, 0.0), (length, 0.0), (length, height), (length * 0.5 if apex is None else apex, height + rise),
            (0.0, height)]


def notch(outline, items):
    """An outline (starting (0, 0), (length, 0)) notched for the doors standing on its foot, in order along it."""
    doors = sorted((o for o in items if o.kind == 'door' and o.z <= 1e-4), key=lambda o: o.x)
    points = [outline[0]]
    for o in doors:
        points += [(o.x, 0.0), (o.x, o.h), (o.x + o.w, o.h), (o.x + o.w, 0.0)]
    return points + list(outline[1:])


def clad(m, space, length, outline, items, thick=0.16, cladding='laps', look='band_G', trim='C', wrap=False,
         frieze=False, table=True, corner_w=0.15, corner=True):
    """One wall of a closed house: its shell (the outline, notched for doors and holed for windows), its cladding
    ('laps': clapboards in look; 'batten': upright boards in look with a batten on every joint), its openings, corner
    boards (wrap on the walls whose boards cover the next wall's edge), a water table along its foot and, on an eave
    wall, a frieze board under the eaves."""
    rng = m.rng
    top0, top1 = outline[-1][1], outline[2][1]
    edges = [(-0.1, corner_w + 0.01, -1.0, 30.0), (length - corner_w - 0.01, length + 0.1, -1.0, 30.0)] if corner else []
    if cladding == 'laps':
        uv, mat = homes.lap_backing(look, rng)
        homes.shell(m, notch(outline, items), kit.holes(items), thick, uv, space, slices=0, mat=mat)
        homes.laps(m, space, outline, 0.2 if table else 0.0, max(z for x, z in outline), homes.keep_out(items) + edges,
                   look=look, out=LAP)
        out = LAP
    else:
        if look in town.WOODS:
            uv, mat = homes.Boards(rotate=True), look
        else:
            uv, mat = Trim(look, world=True, rotate=True, u=rng.uniform(0.0, kit.U_REPEAT)), 'trim'
        homes.shell(m, notch(outline, items), kit.holes(items), thick, uv, space, around=items, mat=mat)
        homes.battens(m, space, outline, 0.0, length, homes.keep_out(items) + edges, look=look, z0=0.2 if table else 0.0)
        out = 0.0
    homes.openings(m, space, items, thick, out=out)
    if corner:
        homes.corners(m, space, length, top0 - 0.02, top1 - 0.02, look=trim, out=out, width=corner_w, wrap=wrap)
    if table:
        homes.band(m, space, homes.door_spans(items, length), 0.1, 0.2, look=trim, out=out)
    if frieze:
        homes.band(m, space, [(0.12, length - 0.12)], min(top0, top1) - 0.14, 0.24, look=trim, out=out)


def skip_round(slope, boxes):
    """A shakes skip: the parts of a slope over any of the world boxes (cx, cy, half x, half y): chimneys through it."""
    def skip(x, y):
        w = slope.world((x, y, 0.0))
        return any(abs(w.x - cx) < hx and abs(w.y - cy) < hy for cx, cy, hx, hy in boxes)
    return skip


def roof_z(eave_z, wall_y, y, pitch):
    """The deck top's height over a point y on a slope rising from a wall at wall_y (either side)."""
    return eave_z + abs(y - wall_y) * math.tan(math.radians(pitch))


# --- RR_Home_A: the saltbox ---

def home_a():
    m = homes.Home('RR_Home_A', seed=501, lived=True, window_sockets=True)
    X0, X1, Y0, Y1 = -3.6, 3.6, -3.2, 3.2
    BASE = 0.5                       # the floor, on the fieldstone foundation
    FRONT_H, BACK_H = 4.9, 2.9       # floor to wall plate: two storeys at the front, one at the back
    PITCH = 42.0
    tan = math.tan(math.radians(PITCH))
    run_f = ((Y1 - Y0) - (FRONT_H - BACK_H) / tan) * 0.5
    run_b = (Y1 - Y0) - run_f
    RIDGE_Y = Y0 + run_f
    RIDGE_Z = BASE + FRONT_H + run_f * tan
    homes.plinth(m, X0, X1, Y0, Y1, BASE)
    m.section('foundation')

    lit = dict(paint='H2', sash='H2', lit=True)
    dark = dict(paint='H2', sash='H2')
    W, H, Z = 0.82, 1.35, 0.85
    DOOR = Opening('door', 3.1, 0.0, 1.0, 2.2, style='panel', finish='woodoxide', casing='H2', knob='brass',
                   transom=0.32, crepe=True, casing_w=0.14)
    up = 3.1
    walls = [
        [Opening('window', 0.75, Z, W, H, **lit), DOOR, Opening('window', 5.63, Z, W, H, **lit),
         Opening('window', 0.75, up, W, 1.2, **lit), Opening('window', 3.19, up, W, 1.2, **dark),
         Opening('window', 5.63, up, W, 1.2, **dark)],
        [Opening('window', 1.15, Z, W, H, **lit), Opening('window', 4.45, Z, W, H, **dark),
         Opening('window', 1.05, 3.15, 0.7, 1.05, **dark)],
        [Opening('door', 1.25, 0.0, 0.95, 2.05, style='plank', finish='woodoxide', casing='H2', hinge='right'),
         Opening('window', 4.7, 0.9, W, 1.2, **dark)],
        [Opening('window', 6.4 - 1.15 - W, Z, W, H, **dark), Opening('window', 1.4, Z, 0.7, 1.1, **dark),
         Opening('window', 6.4 - 1.05 - 0.7, 3.15, 0.7, 1.05, **dark)],
    ]
    ridge_rel = FRONT_H + run_f * tan
    outlines = [
        rect(X1 - X0, FRONT_H),
        [(0.0, 0.0), (Y1 - Y0, 0.0), (Y1 - Y0, BACK_H), (run_f, ridge_rel), (0.0, FRONT_H)],
        rect(X1 - X0, BACK_H),
        [(0.0, 0.0), (Y1 - Y0, 0.0), (Y1 - Y0, FRONT_H), (Y1 - Y0 - run_f, ridge_rel), (0.0, BACK_H)],
    ]
    for k, ((space, length), items, outline) in enumerate(zip(wall_spaces(X0, X1, Y0, Y1, BASE), walls, outlines)):
        clad(m, space, length, outline, items, look='band_G', trim='woodoxide', wrap=k in (0, 2), frieze=k in (0, 2))
    m.section('walls, windows, doors')

    # The door's hood: a little pediment on two scroll brackets.
    front = kit.wall_space((X0, Y0), (X1, Y0), BASE)
    mid = DOOR.x + DOOR.w * 0.5
    for side in (-1.0, 1.0):
        town.bracket(m, front, mid + side * 0.62, 2.72, reach=0.5, drop_h=0.5, thick=0.08, look='woodoxide', out=LAP)
    hood_frame = front.matrix @ Matrix.Translation((mid, 0.0, 0.0)) @ Matrix.Rotation(math.radians(-90.0), 4, 'Z')
    h_front, h_back = kit.gable(m, 0.0, 0.62, -0.72, 0.72, 2.78, 32.0, overhang=0.08, rake=0.1, deck=0.05,
                                frame=hood_frame, ends=(False, True))
    town.shakes(m, h_front, piece=(0.7, 1.0))
    town.shakes(m, h_back, piece=(0.7, 1.0))
    town.ridge_cap(m, h_front, h_back, uv='C', width=0.1, thick=0.04)
    homes.stone_steps(m, 0.0, 1.5, Y0 - 0.06, BASE, 2)
    m.section('door hood, steps')

    # The roof: a short front slope and the long sweep down the back, shakes, a ridge cap and the chimney.
    CH = (0.0, RIDGE_Y + 0.55)
    front_roof = homes.plane(m, X0, X1, Y0, BASE + FRONT_H, PITCH, run_f, 1.0, overhang=0.35, rake=(0.35, 0.35),
                             deck=0.1, sag=0.04)
    back_roof = homes.plane(m, X0, X1, Y1, BASE + BACK_H, PITCH, run_b, -1.0, overhang=0.35, rake=(0.35, 0.35),
                            deck=0.1, sag=0.1)
    for slope in (front_roof, back_roof):
        town.shakes(m, slope, piece=(2.2, 3.8), skip=skip_round(slope, [(CH[0], CH[1], 0.5, 0.55)]))
    town.ridge_cap(m, front_roof, back_roof, uv='C', width=0.16, thick=0.05)
    m.section('roof')
    homes.stack(m, CH[0], CH[1], roof_z(BASE + BACK_H, Y1, CH[1], PITCH) - 0.6, RIDGE_Z + 0.95, size=(0.86, 0.92))
    m.section('chimney')

    # Dressing: a rain barrel under the back eave, firewood stacked along the back wall, a bench by the door.
    homes.barrel(m, (X1 - 0.45, Y1 + 0.5, 0.0))
    homes.woodpile(m, Y1 + 0.06, X0 + 0.6, X0 + 2.4, rows=4, depth=0.42, outward=1.0, axis='y')
    homes.bench(m, X0 + 1.55, Y0 - 0.06, length=1.2, facing=-1.0)
    m.section('dressing')

    # Collision: the body to the back eave, the tall front and the roof over it, the hood.
    m.hull((X1 - X0 + 0.12, Y1 - Y0 + 0.12, BASE + BACK_H), at=(0.0, 0.0, (BASE + BACK_H) * 0.5))
    zf = BASE + FRONT_H - 0.35 * tan + 0.22
    zb = BASE + BACK_H - 0.35 * tan + 0.22
    pts = []
    for x in (X0 - 0.35, X1 + 0.35):
        pts += [(x, Y0 - 0.06, BASE + BACK_H - 0.1), (x, Y0 - 0.35, zf), (x, RIDGE_Y, RIDGE_Z + 0.25),
                (x, Y1 + 0.35, zb), (x, Y1 + 0.06, BASE + BACK_H - 0.1)]
    m.hull_points(pts)
    m.hull((1.5, 0.75, 0.45), at=(0.0, Y0 - 0.4, 2.6 + BASE + 0.2))
    m.socket('Light', (-1.5, Y0 + 1.0, BASE + 1.5))
    return m.finish(pivot=(0.0, Y0), footprint=(X0, X1, Y0, Y1))


# --- RR_Home_B: the dogtrot ---

def hewn_wall(m, space, length, top, items, parity, course=0.3, log_h=0.25, depth=0.22, ext=0.13):
    """A wall of hewn logs: square-hewn timbers (the trim sheet's hewn faces) laid in courses with cream chinking
    showing between them (the plaster strip, set back), running past the corners (half a course up on every other wall,
    as notched corners stack), cut round the openings with squared jambs."""
    rng = m.rng
    chink = Space(space.matrix @ Matrix.Translation((0.0, 0.05, 0.0)))
    homes.shell(m, notch(rect(length, top), items), kit.holes(items), 0.12, Trim('G', world=True, u=rng.uniform(4.2, 5.2)),
                chink, slices=0)
    z = course * 0.5 if parity else 0.0
    while z + log_h <= top + 0.02:
        pieces = [(-ext + rng.uniform(-0.02, 0.02), length + ext + rng.uniform(-0.02, 0.02))]
        for o in items:
            if z < o.z + o.h + 0.02 and z + log_h > o.z - 0.02:
                pieces = town.subtract(pieces, (o.x - 0.06, o.x + o.w + 0.06))
        for a, b in pieces:
            h = log_h + rng.uniform(-0.02, 0.01)
            m.box((b - a, depth, h), at=((a + b) * 0.5, depth * 0.5 - 0.01 + rng.uniform(-0.008, 0.008), z + log_h * 0.5),
                  rot=(0.0, rng.uniform(-0.3, 0.3), 0.0), uv=Trim('C', width=h), space=space, drop=('+y',),
                  cuts=int((b - a) / 2.4))
        z += course
    for o in items:
        for x in (o.x - 0.06, o.x + o.w + 0.06):
            m.board((x, 0.11, o.z - (0.03 if o.z > 0 else 0.0)), (x, 0.11, o.z + o.h + 0.1), 0.12, 0.24,
                    uv=Trim('C', lane='each'), space=space)
        m.board((o.x - 0.14, 0.1, o.z + o.h + 0.07), (o.x + o.w + 0.14, 0.1, o.z + o.h + 0.07), 0.14, 0.24,
                uv=Trim('C', lane='each'), space=space)


def home_b():
    m = homes.Home('RR_Home_B', seed=502, lived=True, window_sockets=True)
    PEN, TROT, D = 4.4, 2.6, 5.0
    LX0, LX1 = -TROT * 0.5 - PEN, -TROT * 0.5
    RX0, RX1 = TROT * 0.5, TROT * 0.5 + PEN
    Y0, Y1 = -D * 0.5, D * 0.5
    BASE, WALL, PITCH = 0.45, 2.7, 34.0
    EAVE = BASE + WALL
    tan = math.tan(math.radians(PITCH))
    RIDGE_Z = EAVE + D * 0.5 * tan
    rng = m.rng
    for x0, x1 in ((LX0, LX1), (RX0, RX1)):
        homes.plinth(m, x0, x1, Y0, Y1, BASE, out=0.04)
    m.section('foundations')

    win = dict(paint='C', sash='woodcream', style='open', shutter='A', casing_w=0.1)
    pens = {
        'left': (LX0, LX1, [
            [Opening('window', 1.6, 0.9, 0.85, 1.15, lit=True, **win)],
            [Opening('door', 1.9, 0.0, 0.95, 2.05, style='plank', finish='A', casing='C', hinge='left')],
            [Opening('window', 1.8, 0.95, 0.8, 1.05, paint='C', sash='woodcream', casing_w=0.1)],
            [Opening('window', 2.9, 1.25, 0.6, 0.75, paint='C', sash='woodcream', casing_w=0.1, panes=(2, 2))],
        ]),
        'right': (RX0, RX1, [
            [Opening('window', 2.0, 0.9, 0.85, 1.15, lit=True, **win)],
            [Opening('window', 1.4, 0.95, 0.8, 1.05, paint='C', sash='woodcream', casing_w=0.1)],
            [Opening('window', 1.9, 0.95, 0.8, 1.05, paint='C', sash='woodcream', casing_w=0.1)],
            [Opening('door', 2.1, 0.0, 0.95, 2.05, style='plank', finish='A', casing='C', hinge='right')],
        ]),
    }
    for name, (x0, x1, walls) in pens.items():
        for k, ((space, length), items) in enumerate(zip(wall_spaces(x0, x1, Y0, Y1, BASE), walls)):
            hewn_wall(m, space, length, WALL, items, parity=k % 2)
            homes.openings(m, space, items, 0.22, out=0.0)
            if k % 2:
                # Board gables over the side walls, up under the roof.
                rise = D * 0.5 * tan
                vent = Opening('window', length * 0.5 - 0.25, WALL + 0.5, 0.5, 0.42)
                homes.shell(m, [(0.0, WALL), (length, WALL), (length * 0.5, WALL + rise)], kit.holes([vent]), 0.06,
                            Trim('A', world=True, rotate=True, u=rng.uniform(0.0, 6.4)),
                            Space(space.matrix @ Matrix.Translation((0.0, 0.08, 0.0))), around=[vent])
                m.box((0.5, 0.02, 0.42), at=(vent.x + 0.25, 0.12, vent.z + 0.21), uv=Trim('H4', fit=True), space=space)
                for j in range(3):
                    town.board(m, (vent.x - 0.02, 0.06, vent.z + 0.08 + j * 0.13),
                               (vent.x + 0.52, 0.06, vent.z + 0.1 + j * 0.13),
                               0.11, 0.025, look='A', space=space, drop=('+y',))
    m.section('pens')

    # The breezeway's floor and the porch the whole length, on the pens' floor; steps up in front of the breezeway.
    PY = Y0 - 2.1
    town.deck(m, LX1, RX0, Y0, Y1, top=BASE, posts=(LX1 + 0.3, RX0 - 0.3), front=False, back=True, sill=False)
    posts = [LX0 + 0.12, LX0 + 2.25, LX1 - 0.2, RX0 + 0.2, RX1 - 2.25, RX1 - 0.12]
    town.deck(m, LX0 - 0.1, RX1 + 0.1, PY, Y0, top=BASE, posts=posts, sill=True, ends=(LX0 - 0.1, RX1 + 0.1))
    homes.steps(m, 0.0, 1.6, PY, BASE, 1)
    homes.steps(m, 0.0, 1.6, Y1, BASE, 1, toward=1.0)       # out the back of the breezeway
    BEAM = EAVE - 0.55
    for x in posts:
        homes.post(m, x, PY + 0.12, BASE, BEAM - 0.08, size=0.16)
    m.board((LX0 - 0.25, PY + 0.12, BEAM), (RX1 + 0.25, PY + 0.12, BEAM), 0.18, 0.16, uv=Trim('C', lane='each'))
    for x in posts:
        for side in (-1.0, 1.0):
            if (x == posts[0] and side < 0) or (x == posts[-1] and side > 0):
                continue
            m.board((x + side * 0.07, PY + 0.12, BEAM - 0.55), (x + side * 0.5, PY + 0.12, BEAM - 0.08), 0.1, 0.09, uv='C')
    # Plates over the breezeway, front and back, carrying the roof between the pens.
    for y in (Y0 + 0.11, Y1 - 0.11):
        m.board((LX1 - 0.2, y, EAVE - 0.13), (RX0 + 0.2, y, EAVE - 0.13), 0.26, 0.22, uv=Trim('C', lane='each'))
    m.section('breezeway, porch')

    # The roof: one long gable over both pens and the breezeway, the porch's shed roof under its front eave.
    front_roof = homes.plane(m, LX0, RX1, Y0, EAVE, PITCH, D * 0.5, 1.0, overhang=0.12, rake=(0.45, 0.45), deck=0.1,
                             sag=0.08)
    back_roof = homes.plane(m, LX0, RX1, Y1, EAVE, PITCH, D * 0.5, -1.0, overhang=0.45, rake=(0.45, 0.45), deck=0.1,
                            sag=0.1)
    CL, CR = (LX0 - 0.45, 0.55), (RX1 + 0.45, -0.45)
    for slope in (front_roof, back_roof):
        town.shakes(m, slope, piece=(2.4, 4.0), skip=skip_round(slope, [(CL[0], CL[1], 0.48, 0.5),
                                                                         (CR[0], CR[1], 0.48, 0.5)]))
    town.ridge_cap(m, front_roof, back_roof, uv='C', width=0.16, thick=0.05)
    porch_roof = homes.shed_roof(m, LX0 - 0.1, RX1 + 0.1, PY + 0.12, BEAM + 0.1, Y0 + 0.1, EAVE - 0.12, overhang=0.32,
                                 rake=(0.3, 0.3), deck=0.07, sag=0.06)
    town.shakes(m, porch_roof, piece=(2.4, 4.0), top=porch_roof.length - 0.02)
    m.section('roofs')
    homes.outside_chimney(m, LX0 - 0.02, CL[1], -1.0, RIDGE_Z + 0.9, BASE, width=1.35, deep=0.95)
    homes.outside_chimney(m, RX1 + 0.02, CR[1], 1.0, RIDGE_Z + 0.75, BASE, width=1.25, deep=0.9, socket='Smoke_2')
    m.section('chimneys')

    # Living: a plank bench along the breezeway's wall, a washtub on its floor, a rain barrel out back.
    town.board(m, (LX1 + 0.05, -0.8, BASE + 0.44), (LX1 + 0.05, 0.6, BASE + 0.44), 0.3, 0.05, look='A',
               face=(0.0, 0.0, 1.0))
    for y in (-0.7, 0.5):
        m.box((0.05, 0.05, 0.44), at=(LX1 + 0.12, y, BASE + 0.22), uv='C')
    homes.lathe(m, [(0.0, 0.0), (0.2, 0.0), (0.27, 0.22), (0.25, 0.235), (0.0, 0.235)], (RX0 - 0.35, 1.4, BASE), 10,
                Tile('MetalWorn'), 'iron')
    homes.barrel(m, (LX0 + 0.55, Y1 + 0.55, 0.0))
    m.hull((0.4, 1.5, 0.48), at=(LX1 + 0.2, -0.1, BASE + 0.24))
    m.section('dressing')

    # Collision: the pens, the breezeway's and porch's floors (walkable), the posts, the roofs.
    for x0, x1 in ((LX0, LX1), (RX0, RX1)):
        m.hull((x1 - x0 + 0.26, D + 0.04, EAVE), at=((x0 + x1) * 0.5, 0.0, EAVE * 0.5))
    m.hull((RX0 - LX1, D, BASE), at=(0.0, 0.0, BASE * 0.5))
    m.hull((RX1 - LX0 + 0.2, Y0 - PY, BASE), at=(0.0, (Y0 + PY) * 0.5, BASE * 0.5))
    for x in posts:
        m.hull((0.18, 0.18, BEAM - BASE), at=(x, PY + 0.12, (BASE + BEAM) * 0.5))
    homes.gable_hull(m, LX0, RX1, Y0, Y1, EAVE, EAVE, 0.0, RIDGE_Z, overhang=0.3, rake=0.45, pitch_front=PITCH)
    homes.slope_hull(m, porch_roof)
    m.socket('Light', (LX0 + 2.0, Y0 + 1.0, BASE + 1.5))
    return m.finish(pivot=(0.0, Y0), footprint=(LX0, RX1, Y0, Y1))


# --- RR_Home_C: the shotgun house ---

def home_c():
    m = homes.Home('RR_Home_C', seed=503)
    X0, X1, Y0, Y1 = -1.95, 1.95, -4.8, 4.8
    BASE, WALL, PITCH = 0.6, 3.0, 42.0
    EAVE = BASE + WALL
    tan = math.tan(math.radians(PITCH))
    RISE = (X1 - X0) * 0.5 * tan
    rng = m.rng
    piers = [(x, y) for x in (X0 + 0.15, X1 - 0.15) for y in kit.frange(Y0 + 0.15, Y1, 2.35)] + [(0.0, Y0 + 0.15),
                                                                                               (0.0, Y1 - 0.15)]
    homes.piers(m, piers, BASE - 0.1, size=0.34)
    # Skirting: upright boards between the piers, a few gone; the floor's underside behind them.
    homes.quad(m, [(X0, Y0, BASE - 0.12), (X1, Y0, BASE - 0.12), (X1, Y1, BASE - 0.12), (X0, Y1, BASE - 0.12)],
               Trim('A', world=True), facing=(0.0, 0.0, -1.0))
    for (space, length) in wall_spaces(X0, X1, Y0, Y1, 0.0):
        x = 0.1
        while x < length - 0.1:
            if rng.random() > 0.12:
                top = BASE - 0.08 + rng.uniform(-0.02, 0.0)
                town.board(m, (x, 0.0, -0.05), (x, 0.0, top), 0.18, 0.025, look='A', space=space, lift=0.013,
                           drop=('+y',))
            x += 0.2
    m.section('piers, skirting')

    shut = dict(paint='woodcream', sash='woodcream', shutter='A')
    DOOR = Opening('door', 2.25, 0.0, 0.92, 2.25, style='panel', finish='woodteal', casing='woodcream', hinge='right',
                   transom=0.3, boarded=True)
    walls = [
        ([DOOR, Opening('window', 0.5, 0.95, 0.85, 1.45, style='boarded', **shut)], True),
        ([Opening('window', 1.4, 0.95, 0.8, 1.35, style='shut', **shut),
          Opening('window', 4.7, 0.95, 0.8, 1.35, style='hanging', **shut),
          Opening('window', 7.6, 0.95, 0.8, 1.35, style='boarded', **shut)], False),
        ([Opening('door', 1.2, 0.0, 0.85, 2.1, style='plank', finish='woodteal', casing='woodcream', boarded=True),
          Opening('window', 2.5, 1.1, 0.7, 1.1, style='shut', **shut)], True),
        ([Opening('window', 1.2, 0.95, 0.8, 1.35, style='boarded', **shut),
          Opening('window', 4.1, 0.95, 0.8, 1.35, style='shut', **shut),
          Opening('window', 7.4, 0.95, 0.8, 1.35, style='hanging', **shut)], False),
    ]
    for k, ((space, length), (items, gable)) in enumerate(zip(wall_spaces(X0, X1, Y0, Y1, BASE), walls)):
        outline = gable_outline(length, WALL, RISE) if gable else rect(length, WALL)
        clad(m, space, length, outline, items, cladding='batten', look='woodblack', trim='woodcream', wrap=gable,
             frieze=not gable, table=False, corner_w=0.13)
        if gable:
            # A louvred vent high in the gable.
            vz = WALL + RISE * 0.45
            homes.shell(m, rect(0.7, 0.5), (), 0.04, Trim('H4', fit=True), Space(space.matrix @ Matrix.Translation(
                (length * 0.5 - 0.35, 0.1, vz))), slices=0)
            for j in range(4):
                z = vz + 0.07 + j * 0.12
                town.board(m, (length * 0.5 - 0.38, -0.01, z), (length * 0.5 + 0.38, -0.01, z), 0.1, 0.02,
                           look='woodcream', space=space, lift=0.02, drop=('+y',))
            town.trim_board(m, space, (length * 0.5 - 0.42, 0.0, vz - 0.04), (length * 0.5 + 0.42, 0.0, vz - 0.04), 0.08,
                            'woodcream', thick=0.04)
    m.section('walls')

    # The porch: a deck on piers across the front; its left post is gone (it lies on the deck), so the beam and the
    # tin roof hang slack at that corner.
    PY = Y0 - 1.8
    DECK = BASE - 0.06
    town.deck(m, X0 - 0.05, X1 + 0.05, PY, Y0, top=DECK, posts=(X0 + 0.15, X1 - 0.15), sill=True,
              ends=(X0 - 0.05, X1 + 0.05))
    homes.steps(m, 0.8, 1.2, PY, DECK, 2)
    POST_Y = PY + 0.12
    BEAM = BASE + 2.45
    SLACK = 0.42
    homes.post(m, X1 - 0.12, POST_Y, DECK, BEAM - 0.08, size=0.13)
    m.board((X1 + 0.15, POST_Y, BEAM), (X0 - 0.15, POST_Y, BEAM - SLACK), 0.16, 0.12, uv=Trim('C', lane='each'))
    # The fallen post across the deck, and the stub where it stood.
    m.board((X0 + 0.3, PY + 0.5, DECK + 0.07), (X0 + 1.9, PY + 1.25, DECK + 0.07), 0.13, 0.13, face=(0.0, 0.0, 1.0),
            uv=Trim('C', lane='each'))
    m.box((0.19, 0.19, 0.05), at=(X0 + 0.12, POST_Y, DECK + 0.025), uv='C')
    porch_roof = homes.shed_roof(m, X0 - 0.1, X1 + 0.1, POST_Y, BEAM + 0.09, Y0 + 0.05, BASE + 2.95, overhang=0.3,
                                 rake=(0.1, 0.1), deck=0.06, sag=0.03, cls=homes.Droop, corner=SLACK, side='left')
    homes.board_roof(m, porch_roof, every=0.25, missing=(1, 6), top=porch_roof.length - 0.03)
    m.section('porch')

    # The roof: shakes front to back, broken in over the back room (on the left, the street side as it's usually
    # seen), the attic dark under it; a leaning stovepipe over the front room.
    frame = Matrix.Rotation(math.radians(90.0), 4, 'Z')
    HOLE = (1.5, 3.1, 0.95, 2.1)            # in the left slope's frame: its x runs from the back
    right = homes.plane(m, Y0, Y1, -X1, EAVE, PITCH, (X1 - X0) * 0.5, 1.0, overhang=0.35, rake=(0.35, 0.35), deck=0.1,
                        sag=0.12, frame=frame)
    left = homes.plane(m, Y0, Y1, -X0, EAVE, PITCH, (X1 - X0) * 0.5, -1.0, overhang=0.35, rake=(0.35, 0.35), deck=0.1,
                       sag=0.12, frame=frame, hole=HOLE)
    PIPE = (-0.8, -1.4)
    town.shakes(m, right, piece=(2.2, 3.6), missing=0.05)
    pipe_skip = skip_round(left, [(PIPE[0], PIPE[1], 0.16, 0.16)])
    hole_skip = homes.ragged(HOLE, 31)
    town.shakes(m, left, piece=(2.2, 3.6), skip=lambda x, y: hole_skip(x, y) or pipe_skip(x, y), missing=0.05)
    town.ridge_cap(m, right, left, uv='C', width=0.16, thick=0.05)
    rz = EAVE + RISE
    homes.attic(m, X0 + 0.1, X1 - 0.1, Y0 + 0.1, Y1 - 0.1, EAVE - 0.02,
                gables=[[(X0 + 0.1, y, EAVE), (X1 - 0.1, y, EAVE), (0.0, y, rz - 0.15)] for y in (Y0 + 0.1, Y1 - 0.1)])
    homes.stovepipe(m, PIPE[0], PIPE[1], roof_z(EAVE, X0, PIPE[0], PITCH) - 0.3, rz + 0.75, lean=(-0.12, 0.05))
    m.section('roof')
    # A shutter fallen from the right side, lying in the grass.
    fallen = Space(Matrix.Translation((X1 + 0.75, -0.6, 0.02)) @ Matrix.Rotation(math.radians(-90.0), 4, 'X')
                   @ Matrix.Rotation(math.radians(24.0), 4, 'Y'))
    homes.shutter_leaf(m, fallen, 0.0, 0.0, 0.42, 1.36, 0.0, 'A')
    m.section('dressing')

    m.hull((X1 - X0 + 0.04, Y1 - Y0 + 0.04, EAVE), at=(0.0, 0.0, EAVE * 0.5))
    m.hull_points([(x, y, EAVE - 0.35 * tan + 0.22) for x in (X0 - 0.35, X1 + 0.35) for y in (Y0 - 0.35, Y1 + 0.35)] +
                  [(0.0, y, rz + 0.25) for y in (Y0 - 0.35, Y1 + 0.35)])
    m.hull((X1 - X0 + 0.1, Y0 - PY, DECK), at=(0.0, (Y0 + PY) * 0.5, DECK * 0.5))
    m.hull((0.15, 0.15, BEAM - DECK), at=(X1 - 0.12, POST_Y, (BEAM + DECK) * 0.5))
    homes.slope_hull(m, porch_roof)
    return m.finish(pivot=(X0 + DOOR.x + DOOR.w * 0.5, Y0), footprint=(X0, X1, Y0, Y1))


# --- RR_Home_D: the gallery house ---

def home_d():
    m = homes.Home('RR_Home_D', seed=504, lived=True, window_sockets=True)
    X0, X1, Y0, Y1 = -4.0, 4.0, -2.7, 2.7
    BASE, STOREY, PITCH = 0.55, 2.8, 30.0
    WALL = STOREY * 2.0
    EAVE = BASE + WALL
    tan = math.tan(math.radians(PITCH))
    RISE = (Y1 - Y0) * 0.5 * tan
    RIDGE_Z = EAVE + RISE
    homes.plinth(m, X0, X1, Y0, Y1, BASE)
    m.section('foundation')

    lit = dict(paint='band_G', sash='band_G', lit=True, panes=(2, 2))
    dark = dict(paint='band_G', sash='band_G', panes=(2, 2))
    W, H = 0.8, 1.5
    U = STOREY + 0.75
    DOOR = Opening('door', 3.5, 0.0, 1.0, 2.35, style='panel', finish='woodblack', casing='band_G', knob='brass',
                   transom=0.35, glass=True)
    UPDOOR = Opening('door', 3.55, STOREY + 0.02, 0.9, 2.15, style='panel', finish='woodblack', casing='band_G',
                     glass=True)
    walls = [
        [Opening('window', 0.55, 0.85, W, H, **lit), Opening('window', 1.85, 0.85, W, H, **lit), DOOR,
         Opening('window', 5.35, 0.85, W, H, **dark), Opening('window', 6.65, 0.85, W, H, **dark),
         Opening('window', 0.55, U, W, 1.35, **lit), Opening('window', 1.85, U, W, 1.35, **dark), UPDOOR,
         Opening('window', 5.35, U, W, 1.35, **dark), Opening('window', 6.65, U, W, 1.35, **lit)],
        [Opening('window', 2.3, 0.85, W, H, **dark), Opening('window', 2.3, U, W, 1.35, **dark)],
        [Opening('door', 5.2, 0.0, 0.95, 2.15, style='plank', finish='woodblack', casing='band_G', hinge='right'),
         Opening('window', 1.6, 0.85, W, H, **dark), Opening('window', 4.0, U, W, 1.35, **dark)],
        [Opening('window', 2.3, 0.85, W, H, **lit), Opening('window', 2.3, U, W, 1.35, **dark)],
    ]
    for k, ((space, length), items) in enumerate(zip(wall_spaces(X0, X1, Y0, Y1, BASE), walls)):
        gable = k % 2 == 1
        outline = gable_outline(length, WALL, RISE) if gable else rect(length, WALL)
        upper = [o for o in items if o.z > STOREY - 0.1 and o.kind == 'door']
        clad(m, space, length, outline, [o for o in items if o not in upper], look='H2', trim='band_G', wrap=not gable,
             frieze=not gable)
        for o in upper:      # the door onto the balcony: its sill is the balcony's floor
            local = Opening('door', o.x, 0.0, o.w, o.h, **o.opts)
            up_space = Space(space.matrix @ Matrix.Translation((0.0, 0.0, o.z)))
            town.door(m, up_space, local, 0.16, out=LAP)
        # A belt course between the storeys.
        homes.band(m, space, [(0.14, length - 0.14)], STOREY + 0.02, 0.16, look='band_G', out=LAP)
    m.section('walls')

    # The double gallery: a porch below, a balcony above on the same posts, a shed roof over both.
    GY = Y0 - 2.0
    town.deck(m, X0 - 0.1, X1 + 0.1, GY, Y0, top=BASE, posts=(X0 + 0.1, -1.3, 1.3, X1 - 0.1), sill=True,
              ends=(X0 - 0.1, X1 + 0.1), plank_cuts=0)
    homes.steps(m, 0.0, 1.8, GY, BASE, 2)
    UP = BASE + STOREY
    # The balcony's floor: its joists hide behind the porch's ceiling, so it has none.
    town.deck(m, X0 - 0.1, X1 + 0.1, GY, Y0, top=UP, front=True, back=False, sill=False, ends=(X0 - 0.1, X1 + 0.1),
              joists=0, plank_cuts=0)
    ceiling = UP - 0.22                  # the porch's ceiling: the balcony floor's underside
    homes.quad(m, [(X0 - 0.1, GY, ceiling), (X1 + 0.1, GY, ceiling), (X1 + 0.1, Y0, ceiling), (X0 - 0.1, Y0, ceiling)],
               Trim('A', world=True), facing=(0.0, 0.0, -1.0))
    TOP = EAVE - 0.4
    posts = [X0 + 0.05, -1.3, 1.3, X1 - 0.05]
    PYP = GY + 0.12
    for x in posts:
        homes.post(m, x, PYP, BASE, UP - 0.2, size=0.15, look='woodcream', turned=True)
        homes.post(m, x, PYP, UP, TOP - 0.1, size=0.13, look='woodcream', turned=True)
    m.board((X0 - 0.2, PYP, UP - 0.12), (X1 + 0.2, PYP, UP - 0.12), 0.2, 0.1, uv=Trim('C', lane='each'))
    m.board((X0 - 0.2, PYP, TOP), (X1 + 0.2, PYP, TOP), 0.18, 0.12, uv=Trim('C', lane='each'))
    for x0, x1 in ((X0 + 0.15, -1.38), (-1.22, -0.52), (0.52, 1.22), (1.38, X1 - 0.15)):
        homes.railing(m, (x0, PYP, UP), (x1, PYP, UP), height=0.92, every=0.19, look='band_G', picket='band_G')
    for x in (X0 - 0.02, X1 + 0.02):
        homes.railing(m, (x, PYP + 0.1, UP), (x, Y0 - 0.1, UP), height=0.92, every=0.19, look='band_G', picket='band_G')
    gallery = homes.shed_roof(m, X0 - 0.1, X1 + 0.1, PYP, TOP + 0.09, Y0 + 0.05, EAVE - 0.15, overhang=0.3,
                              rake=(0.15, 0.15), deck=0.07, sag=0.04)
    town.shakes(m, gallery, piece=(2.4, 4.0), top=gallery.length - 0.02)
    m.section('gallery')

    # The roof and the chimneys at both gable ends.
    front_roof = homes.plane(m, X0, X1, Y0, EAVE, PITCH, (Y1 - Y0) * 0.5, 1.0, overhang=0.4, rake=(0.35, 0.35),
                             deck=0.1, sag=0.05)
    back_roof = homes.plane(m, X0, X1, Y1, EAVE, PITCH, (Y1 - Y0) * 0.5, -1.0, overhang=0.4, rake=(0.35, 0.35),
                            deck=0.1, sag=0.05)
    CL, CR = (X0 + 0.55, 0.3), (X1 - 0.55, 0.3)
    for slope in (front_roof, back_roof):
        town.shakes(m, slope, piece=(2.4, 4.0), skip=skip_round(slope, [(CL[0], CL[1], 0.42, 0.48),
                                                                         (CR[0], CR[1], 0.42, 0.48)]))
    town.ridge_cap(m, front_roof, back_roof, uv='C', width=0.16, thick=0.05)
    for (cx, cy), socket in ((CL, 'Smoke'), (CR, 'Smoke_2')):
        homes.stack(m, cx, cy, roof_z(EAVE, Y1, cy, PITCH) - 0.6, RIDGE_Z + 0.8, size=(0.66, 0.78), socket=socket)
    m.section('roof')

    m.hull((X1 - X0 + 0.1, Y1 - Y0 + 0.1, EAVE), at=(0.0, 0.0, EAVE * 0.5))
    homes.gable_hull(m, X0, X1, Y0, Y1, EAVE, EAVE, 0.0, RIDGE_Z, overhang=0.4, rake=0.35, pitch_front=PITCH)
    m.hull((X1 - X0 + 0.2, Y0 - GY, BASE), at=(0.0, (Y0 + GY) * 0.5, BASE * 0.5))
    m.hull((X1 - X0 + 0.2, Y0 - GY, 0.26), at=(0.0, (Y0 + GY) * 0.5, UP - 0.13))
    m.hull((X1 - X0 + 0.2, 0.08, 0.95), at=(0.0, PYP, UP + 0.47))
    for x in posts:
        m.hull((0.16, 0.16, TOP - BASE), at=(x, PYP, (BASE + TOP) * 0.5))
    homes.slope_hull(m, gallery)
    m.socket('Light', (-2.2, Y0 + 1.0, BASE + 1.5))
    return m.finish(pivot=(0.0, Y0), footprint=(X0, X1, Y0, Y1))


# --- RR_Home_E: the dugout ---

def mound_z(x, y, top=2.42, xo=5.6, yo=8.2, k=0.8):
    """The sod mound's height: a plateau over the room falling a little to the back, its sides and back sloping away
    (a min of planes, so the mound is convex and one hull fits it)."""
    return min(top - 0.04 * y, k * (xo - abs(x)), k * (yo - y))


def home_e():
    m = homes.Home('RR_Home_E', seed=505, lived=True, window_sockets=True)
    rng = m.rng
    XF = 2.4                         # the front wall's half width
    FRONT = 2.18                     # its height, under the sod's lip
    # The mound: a grid over the plan, the front row overhanging the walls as a lip of sod, the outer ring sunk below
    # the ground; dry grass (Hay) over it, bare earth round its foot.
    xs = [-6.05, -5.4, -4.7, -4.0, -3.3, -2.6, -1.9, -1.0, 0.0, 1.0, 1.9, 2.6, 3.3, 4.0, 4.7, 5.4, 6.05]
    ys = [-0.22, 0.0, 0.7, 1.6, 2.6, 3.6, 4.6, 5.3, 6.0, 6.8, 7.6, 8.2, 8.62]

    def z_at(x, y):
        if y < 0.0:
            return mound_z(x, 0.0) - 0.16 - rng.uniform(0.0, 0.05)
        return max(mound_z(x, y), -0.3)
    grid = [[None] * len(xs) for _ in ys]
    for j, y in enumerate(ys):
        for i, x in enumerate(xs):
            z = z_at(x, y)
            if 0.0 < y < 8.0 and 0.0 < z < 2.3:
                z += rng.uniform(-0.05, 0.05)
            elif z >= 2.3 and y > 0.1:
                z += rng.uniform(-0.03, 0.04)
            grid[j][i] = Vector((x + (rng.uniform(-0.08, 0.08) if 0 < i < len(xs) - 1 else 0.0),
                                 y + (rng.uniform(-0.08, 0.08) if 0 < j < len(ys) - 1 else 0.0), z))
    for key, pick in (('hay', lambda zc: zc > 0.32), ('dirt', lambda zc: zc <= 0.32)):
        tb = kit._new_bmesh()
        verts = [[tb.verts.new(p) for p in row] for row in grid]
        for j in range(len(ys) - 1):
            for i in range(len(xs) - 1):
                quad = (verts[j][i], verts[j][i + 1], verts[j + 1][i + 1], verts[j + 1][i])
                zc = sum(v.co.z for v in quad) / 4.0
                if pick(zc):
                    face = tb.faces.new(quad)
        tb.normal_update()
        for f in tb.faces:
            if f.normal.z < 0.0:
                f.normal_flip()
        loose = [v for v in tb.verts if not v.link_faces]
        bmesh.ops.delete(tb, geom=loose, context='VERTS')
        m.emit(tb, homes.Tiled('Hay', 900) if key == 'hay' else Tile('GroundDirt'), key, smooth=True)
    m.section('mound')

    # Dry-stone wing walls holding the mound back either side of the front, their tops under the sod's lip.
    for sign in (-1.0, 1.0):
        a, b = XF, 5.5                       # out to where the mound's foot meets the ground
        steps = [XF + (b - a) * t / 5.0 for t in range(6)]
        top = [(s - a, max(mound_z(s, 0.0) - 0.06, -0.15)) for s in steps]
        outline = [(0.0, -0.3), (b - a, -0.3)] + list(reversed(top))
        if sign > 0:
            space = kit.wall_space((a, 0.0), (b, 0.0), 0.0)
            pts = outline
        else:
            space = kit.wall_space((-b, 0.0), (-a, 0.0), 0.0)
            pts = [(0.0, -0.3), (b - a, -0.3)] + [((b - a) - (s - a), z) for s, z in top]
        homes.shell(m, pts, (), 0.4, Trim('D', world=True, u=rng.uniform(0.0, 6.4)), space, slices=1.0)
    m.section('wing walls')

    # The front: upright split logs between corner posts, a plank door and a little window, a log lintel over them,
    # the purlin under the sod's lip and the rafters' ends through the front.
    DOOR = Opening('door', -0.45, 0.0, 0.9, 1.95, style='plank', finish='A', casing='C', hinge='left')
    WIN = Opening('window', 0.95, 0.85, 0.62, 0.62, paint='C', sash='A', lit=True, panes=(2, 2), single=True,
                  casing_w=0.09)
    front = kit.wall_space((-XF, 0.0), (XF, 0.0), 0.0)
    homes.shell(m, notch(rect(2.0 * XF, FRONT), [Opening('door', XF + DOOR.x, 0.0, DOOR.w, DOOR.h)]),
                [kit.rect(XF + WIN.x, WIN.z, WIN.w, WIN.h)], 0.2, Trim('C', world=True, rotate=True), front, slices=0)
    x = -XF + 0.13
    while x < XF - 0.05:
        cuts = [(DOOR.x - 0.04, DOOR.x + DOOR.w + 0.04, 0.0, DOOR.h + 0.06), (WIN.x - 0.1, WIN.x + WIN.w + 0.1, WIN.z - 0.08,
                                                                               WIN.z + WIN.h + 0.12)]
        runs = [(0.0, FRONT - rng.uniform(0.0, 0.05))]
        for c0, c1, z0, z1 in cuts:
            if c0 - 0.1 < x < c1 + 0.1:
                runs = town.subtract(runs, (z0, z1))
        for lo, hi in runs:
            r = rng.uniform(0.1, 0.125)
            kit.log(m, (x, -0.02, lo), (x, -0.02, hi), r, (0.0, -1.0, 0.0), sides=6,
                    caps=(False, lo > 0.1), v=rng.choice((0.0, 0.4)))
        x += 0.235
    opening_space = kit.wall_space((-XF, 0.0), (XF, 0.0), 0.0)
    homes.door(m, opening_space, Opening('door', XF + DOOR.x, 0.0, DOOR.w, DOOR.h, **DOOR.opts), 0.24, out=0.1)
    homes.window(m, opening_space, Opening('window', XF + WIN.x, WIN.z, WIN.w, WIN.h, **WIN.opts), 0.24, out=0.1)
    for x in (-XF - 0.05, XF + 0.05):
        kit.log(m, (x, -0.05, -0.2), (x, -0.05, FRONT + 0.1), 0.15, (0.0, -1.0, 0.0), sides=8)
    kit.log(m, (-XF - 0.35, -0.12, FRONT + 0.1), (XF + 0.35, -0.12, FRONT + 0.1), 0.15, (0.0, -1.0, 0.0), sides=8)
    for x in kit.frange(-XF + 0.2, XF, 0.55):
        kit.log(m, (x + rng.uniform(-0.04, 0.04), 0.05, FRONT + 0.31), (x, -0.32 - rng.uniform(0.0, 0.08), FRONT + 0.29),
                0.075, (0.0, 0.0, 1.0), sides=6, caps=(False, True))
    m.section('front')

    # The arbor before the door: four poles, a beam across the front and a ridge pole back to the purlin, and an old
    # wagon sheet of canvas thrown over them, sagging between the poles.
    AY = -2.3
    poles = [(-1.6, AY), (1.6, AY), (-1.6, -0.3), (1.6, -0.3)]
    for x, y in poles:
        kit.log(m, (x, y, -0.2), (x, y, 2.0 if y < -1 else 2.2), 0.07, (0.0, -1.0, 0.0), sides=6)
    for y, z in ((AY, 2.06), (-0.3, 2.26)):
        kit.log(m, (-1.85, y, z), (1.85, y, z), 0.07, (0.0, -1.0, 0.0), sides=6)
    for x in (-1.6, 0.0, 1.6):
        kit.log(m, (x, AY - 0.15, 2.12), (x, 0.05, 2.34), 0.05, (0.0, 0.0, 1.0), sides=5)
    # The sheet lies on the three poles (x -1.6, 0, 1.6), sagging between them, its hem hanging over the front beam
    # and down past the ends.
    rows = []
    for j in range(7):
        v = j / 6.0
        y = AY - 0.4 + (0.25 - (AY - 0.4)) * v
        row = []
        for i in range(13):
            u = i / 12.0
            x = -2.0 + 4.0 * u
            pole = 2.2 + (y - (AY - 0.15)) / (0.2 - AY) * 0.22 + 0.06       # the poles' tops under it
            gap = min(abs(x - p) for p in (-1.6, 0.0, 1.6))
            z = pole - 0.07 * math.sin(math.pi * min(gap / 0.8, 1.0)) * math.sin(math.pi * min(max(v, 0.05), 0.95))
            if abs(x) > 1.6:                                               # past the end poles it falls away
                z -= (abs(x) - 1.6) * 0.55
            if j == 0:                                                     # the hem, over the front beam
                z -= 0.22 + rng.uniform(0.0, 0.04)
            row.append((x, y + (rng.uniform(-0.015, 0.015) if j == 0 else 0.0), z))
        rows.append(row)
    homes.sheet(m, rows, Tile('Polymer'), mat='canvas', facing=(0.0, 0.0, 1.0))
    # Its underside, a hair below (one-sided faces back to back), seen from the door.
    homes.sheet(m, [[(x, y, z - 0.008) for x, y, z in row] for row in rows], Tile('Polymer'), mat='canvas',
                facing=(0.0, 0.0, -1.0))
    m.section('arbor')

    # A stovepipe up through the sod; a bench and a water barrel by the door, a washtub hung on the post.
    homes.stovepipe(m, 1.3, 2.4, mound_z(1.3, 2.4) - 0.3, mound_z(1.3, 2.4) + 0.9, radius=0.09)
    homes.bench(m, -1.55, -0.02, length=1.1, facing=-1.0)
    homes.barrel(m, (1.95, -0.45, 0.0), height=0.85, radius=0.28)
    homes.lathe(m, [(0.0, 0.0), (0.22, 0.0), (0.3, 0.2), (0.28, 0.21), (0.0, 0.21)], (-1.6, -0.12, 1.15), 10,
                Tile('MetalWorn'), 'iron')
    # The winter's wood stacked against the left wing wall, a chopping block before it.
    homes.woodpile(m, -0.02, -4.7, -2.9, rows=4, depth=0.45, outward=-1.0, axis='y')
    m.cylinder((-3.2, -1.3, -0.05), (-3.2, -1.3, 0.48), 0.23, sides=9, uv=Trim('B', v=0.0), face=(0.0, -1.0, 0.0),
               caps=(False, True), cap_uv=kit.EndGrain(), cap_mat='endgrain')
    m.hull((0.46, 0.46, 0.5), at=(-3.2, -1.3, 0.25))
    m.section('dressing')

    pts = []
    for y in (0.0, 1.0, 4.6, 6.0, 8.62):
        for x in (-6.05, -2.55, 0.0, 2.55, 6.05):
            pts.append((x, y, max(mound_z(x, y), -0.3) + 0.04))
    m.hull_points(pts)
    for x, y in poles:
        m.hull((0.14, 0.14, 2.1), at=(x, y, 1.05))
    m.hull((3.9, 2.6, 0.2), at=(0.0, (AY - 0.25) * 0.5, 2.3))
    m.socket('Light', (0.4, 1.0, 1.5))
    return m.finish(pivot=(0.0, 0.0), footprint=(-XF, XF, 0.0, 5.0))


# --- RR_Home_F: the stone house ---

def stone_walls(m, x0, x1, y0, y1, base, wall, rise, walls, thick=0.5):
    """Fieldstone walls: the stone panels (gable ends up to the apex), timber lintels and stone sills at the openings,
    and dressed quoins up the corners."""
    rng = m.rng
    for k, ((space, length), items) in enumerate(zip(wall_spaces(x0, x1, y0, y1, base), walls)):
        gable = k % 2 == 1
        outline = gable_outline(length, wall, rise) if gable else rect(length, wall)
        # From below the ground up: the stone runs on under the doors' thresholds, so they are holes, not notches.
        outline = [(0.0, -base - homes.BELOW), (length, -base - homes.BELOW)] + outline[2:]
        homes.shell(m, outline, [kit.rect(o.x, max(o.z, 0.0), o.w, o.h) for o in items], thick,
                    Trim('D', world=True, u=rng.uniform(0.0, 6.4)), space, around=items, slices=1.1)
        for o in items:
            m.board((o.x - 0.22, -0.02, o.z + o.h + 0.11), (o.x + o.w + 0.22, -0.02, o.z + o.h + 0.11), 0.22, 0.3,
                    uv=Trim('C', lane='each'), space=space, lift=-0.1)
            if o.kind == 'window':
                m.box((o.w + 0.16, 0.16, 0.08), at=(o.x + o.w * 0.5, -0.05, o.z - 0.04), uv='D', space=space)
        homes.openings(m, space, [o for o in items], thick, out=0.0)
        for x_edge, sign in ((0.0, 1.0), (length, -1.0)):
            z, j = -0.15, 0
            while z < wall - 0.15:
                h = min(0.34 if j % 2 == 0 else 0.26, wall - z)
                ln = 0.5 if j % 2 == 0 else 0.32
                town.box(m, (ln, 0.05, h - 0.025), at=(x_edge + sign * (ln * 0.5 - 0.02), -0.025, z + h * 0.5), look='D',
                         space=space, drop=('+y',))
                z += h
                j += 1


def home_f():
    m = homes.Home('RR_Home_F', seed=506)
    X0, X1, Y0, Y1 = -3.4, 3.4, -2.7, 2.7
    BASE, WALL, PITCH = 0.3, 3.1, 46.0
    EAVE = BASE + WALL
    tan = math.tan(math.radians(PITCH))
    RISE = (Y1 - Y0) * 0.5 * tan
    RIDGE_Z = EAVE + RISE
    rng = m.rng
    shut = dict(paint='C', sash='A', shutter='A', casing_w=0.08)
    DOOR = Opening('door', 2.9, 0.0, 1.0, 2.1, style='plank', finish='A', casing='C', hinge='left', boarded=True,
                   casing_w=0.1)
    walls = [
        [Opening('window', 0.9, 0.9, 0.8, 1.15, style='shut', **shut), DOOR,
         Opening('window', 5.1, 0.9, 0.8, 1.15, style='hanging', **shut)],
        [Opening('window', 2.3, 0.95, 0.75, 1.1, style='boarded', **shut),
         Opening('window', 2.35, WALL + 0.4, 0.65, 0.85, style='shut', **shut)],
        [Opening('window', 1.5, 0.95, 0.75, 1.1, style='shut', **shut),
         Opening('door', 4.2, 0.0, 0.9, 2.0, style='plank', finish='A', casing='C', boarded=True, casing_w=0.1)],
        [Opening('window', 1.1, 1.3, 0.55, 0.7, style='boarded', **shut)],
    ]
    stone_walls(m, X0, X1, Y0, Y1, BASE, WALL, RISE, walls)
    homes.stone_steps(m, 0.0, 1.4, Y0 - 0.02, BASE, 1, run=0.4)
    m.section('stone walls')

    # The roof: steep shakes sagging along the ridge; the front slope fallen in toward the right end.
    HOLE = (4.45, 6.1, 0.9, 2.55)
    front_roof = homes.plane(m, X0, X1, Y0, EAVE, PITCH, (Y1 - Y0) * 0.5, 1.0, overhang=0.32, rake=(0.25, 0.25),
                             deck=0.1, sag=0.2, hole=HOLE)
    back_roof = homes.plane(m, X0, X1, Y1, EAVE, PITCH, (Y1 - Y0) * 0.5, -1.0, overhang=0.32, rake=(0.25, 0.25),
                            deck=0.1, sag=0.2)
    town.shakes(m, front_roof, piece=(1.6, 3.0), skip=homes.ragged(HOLE, 61), missing=0.06)
    town.shakes(m, back_roof, piece=(1.6, 3.0), missing=0.06)
    town.ridge_cap(m, front_roof, back_roof, uv='C', width=0.16, thick=0.05)
    homes.attic(m, X0 + 0.5, X1 - 0.5, Y0 + 0.5, Y1 - 0.5, EAVE - 0.02,
                gables=[[(x, Y0 + 0.5, EAVE), (x, Y1 - 0.5, EAVE), (x, 0.0, RIDGE_Z - 0.4)] for x in (X0 + 0.5, X1 - 0.5)])
    m.section('roof')
    # The chimney rises out of the left gable's apex.
    kit.stone_stack(m, (X0 + 0.32, 0.0, RIDGE_Z - 0.9), (0.72, 0.9), 1.85, taper=0.02)
    homes.flue_cap(m, X0 + 0.32, 0.0, RIDGE_Z + 0.95, (0.72, 0.9))
    m.section('chimney')

    # The woodshed against the right gable: posts and a board roof, its outer post leaning and the roof dropping with
    # it; a few logs still stacked under it.
    SX0, SX1, SY0, SY1 = X1, X1 + 2.0, -1.9, 1.9
    HIGH, LOW = BASE + 2.25, 1.9
    for y in (SY0 + 0.1, SY1 - 0.1):
        m.board((SX1 - 0.12, y, -0.1), (SX1 - 0.12 + (0.22 if y > 0 else 0.0), y, LOW - (0.35 if y > 0 else 0.0)), 0.12,
                0.12, uv=Trim('C', lane='each'))
    m.board((SX1 - 0.12, SY0 - 0.1, LOW + 0.06), (SX1 + 0.1, SY1 + 0.1, LOW - 0.34), 0.14, 0.1,
            uv=Trim('C', lane='each'))
    m.board((X1 + 0.06, SY0 - 0.15, HIGH - 0.08), (X1 + 0.06, SY1 + 0.15, HIGH - 0.08), 0.14, 0.12,
            uv=Trim('C', lane='each'))
    # Its roof falls away from the wall toward +X: a slope whose eave runs along +Y from the front (so x along the eave
    # by up the slope points out of the roof), its back corner over the leaning post dropping with it.
    shed = homes.Droop(Vector((SX1 + 0.18, SY0 - 0.2, LOW - 0.02)), (0.0, 1.0, 0.0),
                       (-(SX1 + 0.18 - X1), 0.0, HIGH - LOW + 0.02), SY1 - SY0 + 0.4,
                       math.hypot(SX1 + 0.18 - X1, HIGH - LOW + 0.02), 0.05, corner=0.32, side='right')
    town.roof_deck(m, shed, 0.04, rake_boards=False)
    homes.board_roof(m, shed, every=0.25, look='A', missing=(3, 9, 10))
    homes.woodpile(m, X1 + 0.04, SY0 + 0.4, SY0 + 2.3, rows=3, depth=0.5, outward=1.0, axis='x')
    m.section('woodshed')

    m.hull((X1 - X0 + 0.1, Y1 - Y0 + 0.1, EAVE), at=(0.0, 0.0, EAVE * 0.5))
    homes.gable_hull(m, X0, X1, Y0, Y1, EAVE, EAVE, 0.0, RIDGE_Z, overhang=0.32, rake=0.25, pitch_front=PITCH)
    m.hull((0.8, 1.0, 1.9), at=(X0 + 0.32, 0.0, RIDGE_Z - 0.0))
    for y in (SY0 + 0.1, SY1 - 0.1):
        m.hull((0.15, 0.15, LOW), at=(SX1 - 0.12, y, LOW * 0.5))
    homes.slope_hull(m, shed, thick=0.15)
    return m.finish(pivot=(0.0, Y0), footprint=(X0, X1, Y0, Y1))


# --- RR_Home_G: the claim cabin ---

def home_g():
    m = homes.Home('RR_Home_G', seed=507)
    X0, X1, Y0, Y1 = -2.5, 2.5, -2.2, 2.2
    BASE, WALL, PITCH = 0.38, 2.45, 44.0
    EAVE = BASE + WALL
    tan = math.tan(math.radians(PITCH))
    RISE = (X1 - X0) * 0.5 * tan
    rng = m.rng
    homes.piers(m, [(x, y) for x in (X0 + 0.12, 0.0, X1 - 0.12) for y in (Y0 + 0.12, Y1 - 0.12)], BASE - 0.1, size=0.32)
    homes.quad(m, [(X0, Y0, BASE - 0.12), (X1, Y0, BASE - 0.12), (X1, Y1, BASE - 0.12), (X0, Y1, BASE - 0.12)],
               Trim('A', world=True), facing=(0.0, 0.0, -1.0))
    for (space, length) in wall_spaces(X0, X1, Y0, Y1, 0.0):
        m.board((0.0, 0.08, BASE - 0.2), (length, 0.08, BASE - 0.2), 0.18, 0.16, uv=Trim('C', lane='each'), space=space)
    m.section('piers, sills')

    win = dict(paint='C', sash='A', casing_w=0.1)
    DOOR = Opening('door', 2.05, 0.0, 0.9, 2.05, style='plank', finish='A', casing='C', hinge='left', boarded=True)
    HATCH = Opening('door', 2.05, 0.0, 0.9, 0.95, style='plank', finish='A', casing='C', hinge='right')
    walls = [
        ([DOOR, Opening('window', 3.55, 0.95, 0.75, 1.0, style='boarded', **win)], True),
        ([], False),
        ([Opening('window', 1.9, 1.0, 0.75, 1.0, style='shut', shutter='A', **win)], True),
        ([Opening('window', 1.6, 0.95, 0.75, 1.0, style='hanging', shutter='A', **win)], False),
    ]
    for k, ((space, length), (items, gable)) in enumerate(zip(wall_spaces(X0, X1, Y0, Y1, BASE), walls)):
        outline = gable_outline(length, WALL, RISE) if gable else rect(length, WALL)
        clad(m, space, length, outline, items, cladding='batten', look='A', trim='C', wrap=gable, frieze=False,
             table=False, corner_w=0.12)
        if k == 0:
            # The hayloft hatch high in the front gable, closed, with a hoist pole over it.
            loft = Space(space.matrix @ Matrix.Translation((0.0, 0.0, WALL + 0.25)))
            homes.shell(m, rect(HATCH.w, HATCH.h), (), 0.04, Trim('H4', fit=True), Space(loft.matrix @ Matrix.Translation(
                (HATCH.x, 0.1, 0.0))), slices=0)
            town.door(m, loft, HATCH, 0.12)
            m.board((length * 0.5, 0.5, WALL + RISE - 0.35), (length * 0.5, -0.75, WALL + RISE - 0.35), 0.14, 0.12,
                    face=(0.0, 0.0, 1.0), uv=Trim('C', lane='each'), space=space)
    homes.stone_steps(m, 0.0, 1.1, Y0 - 0.02, BASE, 1, run=0.38)
    m.section('walls')

    # The roof: boards down the slope, battened, a few gone and a patch of tin over one bad spot.
    right = homes.plane(m, Y0, Y1, -X1, EAVE, PITCH, (X1 - X0) * 0.5, 1.0, overhang=0.32, rake=(0.3, 0.3), deck=0.08,
                        sag=0.08, frame=Matrix.Rotation(math.radians(90.0), 4, 'Z'))
    left = homes.plane(m, Y0, Y1, -X0, EAVE, PITCH, (X1 - X0) * 0.5, -1.0, overhang=0.32, rake=(0.3, 0.3), deck=0.08,
                       sag=0.08, frame=Matrix.Rotation(math.radians(90.0), 4, 'Z'))
    homes.board_roof(m, right, every=0.34, missing=(4, 9))
    homes.board_roof(m, left, every=0.34, missing=(6,))
    town.ridge_cap(m, right, left, uv='C', width=0.18, thick=0.05)
    m.section('roof')

    # The lean-to of rough boards on the right side, its own small door, a board roof, the stovepipe through it.
    LX0, LX1, LY0, LY1 = X1, X1 + 2.3, -1.5, Y1
    LHIGH, LLOW = BASE + 2.2, BASE + 1.65
    lean = [((LX0, LY0), (LX1, LY0)), ((LX1, LY0), (LX1, LY1)), ((LX1, LY1), (LX0, LY1))]
    for k, (p0, p1) in enumerate(lean):
        # Its floor is the ground: the walls stand on a sill from just under it.
        space = kit.wall_space(p0, p1, 0.0)
        length = (Vector(p1) - Vector(p0)).length
        if k == 1:
            outline = [(0.0, -0.15), (length, -0.15), (length, LLOW), (0.0, LLOW)]
        elif k == 0:
            outline = [(0.0, -0.15), (length, -0.15), (length, LLOW), (0.0, LHIGH)]
        else:
            outline = [(0.0, -0.15), (length, -0.15), (length, LHIGH), (0.0, LLOW)]
        items = [Opening('door', 0.7, 0.0, 0.8, 1.7, style='plank', finish='A', casing='C')] if k == 0 else []
        homes.shell(m, outline, [kit.rect(o.x, 0.0, o.w, o.h) for o in items], 0.06,
                    Trim('A', world=True, rotate=True, u=rng.uniform(0.0, 6.4)), space, around=items)
        homes.openings(m, space, items, 0.06)
        m.board((0.0, 0.05, 0.05), (length, 0.05, 0.05), 0.2, 0.1, uv=Trim('C', lane='each'), space=space)
    # Its roof falls away from the cabin toward +X: x along the eave from the front, so its z is up.
    rise = LHIGH - LLOW
    shed = kit.Slope(Vector((LX1 + 0.25, LY0 - 0.15, LLOW - 0.25 * rise / (LX1 - LX0))), (0.0, 1.0, 0.0),
                     (-(LX1 - LX0), 0.0, rise), LY1 - LY0 + 0.3, math.hypot(LX1 + 0.25 - LX0, rise * 1.1), 0.04)
    town.roof_deck(m, shed, 0.05, rake_boards=False)
    homes.board_roof(m, shed, every=0.27, missing=(2,))
    homes.stovepipe(m, LX0 + 0.8, 1.2, LHIGH - 0.4, EAVE + RISE * 0.6, radius=0.08)
    m.section('lean-to')
    homes.barrel(m, (X0 - 0.42, Y0 + 0.6, 0.0), height=0.85, radius=0.28)
    m.section('dressing')

    m.hull((X1 - X0 + 0.04, Y1 - Y0 + 0.04, EAVE), at=(0.0, 0.0, EAVE * 0.5))
    m.hull_points([(x, y, EAVE - 0.32 * tan + 0.22) for x in (X0 - 0.32, X1 + 0.32) for y in (Y0 - 0.3, Y1 + 0.3)] +
                  [(0.0, y, EAVE + RISE + 0.25) for y in (Y0 - 0.3, Y1 + 0.3)])
    m.hull((LX1 - LX0, LY1 - LY0, LLOW), at=((LX0 + LX1) * 0.5, (LY0 + LY1) * 0.5, LLOW * 0.5))
    homes.slope_hull(m, shed, thick=0.15)
    return m.finish(pivot=(X0 + DOOR.x + DOOR.w * 0.5, Y0), footprint=(X0, X1, Y0, Y1))


# --- RR_Home_H: the porch house ---

def home_h():
    m = homes.Home('RR_Home_H', seed=508, lived=True, window_sockets=True)
    X0, X1, Y0, Y1 = -3.4, 3.4, -3.4, 3.4
    BASE, WALL, PITCH = 0.5, 3.05, 33.0
    EAVE = BASE + WALL
    theta = math.radians(PITCH)
    tan = math.tan(theta)
    rng = m.rng
    homes.plinth(m, X0, X1, Y0, Y1, BASE)
    m.section('foundation')

    # Faded teal clapboards (WoodTeal), cream trim and shutters on the porch's two sides.
    lit = dict(paint='band_G', sash='band_G', lit=True, style='open', shutter='band_G', panes=(2, 2))
    dark = dict(paint='band_G', sash='band_G', style='open', shutter='band_G', panes=(2, 2))
    plain = dict(paint='band_G', sash='band_G', panes=(2, 2))
    W, H = 0.85, 1.45
    DOOR = Opening('door', 2.9, 0.0, 1.0, 2.3, style='panel', finish='woodblack', casing='band_G', knob='brass',
                   transom=0.32, glass=True)
    walls = [
        [Opening('window', 0.85, 0.85, W, H, **lit), DOOR, Opening('window', 5.1, 0.85, W, H, **lit)],
        [Opening('window', 1.0, 0.85, W, H, **lit),
         Opening('door', 3.0, 0.0, 0.95, 2.2, style='panel', finish='woodblack', casing='band_G', knob='brass'),
         Opening('window', 4.95, 0.85, W, H, **plain)],
        [Opening('window', 1.6, 0.85, W, H, **plain), Opening('window', 4.4, 0.85, W, H, **plain)],
        [Opening('window', 1.6, 0.85, W, H, **plain), Opening('window', 4.4, 0.85, W, H, lit=True, **plain)],
    ]
    for k, ((space, length), items) in enumerate(zip(wall_spaces(X0, X1, Y0, Y1, BASE), walls)):
        clad(m, space, length, rect(length, WALL), items, look='woodteal', trim='band_G', wrap=k in (0, 2),
             frieze=True)
    m.section('walls')

    # The pyramid: four hip planes meeting at the peak, shakes with cap boards up the hips, a finial, the chimney out of
    # the peak.
    OV = 0.45
    half = (X1 - X0) * 0.5
    run = half + OV
    length = run / math.cos(theta) + 0.1 * tan
    peak = EAVE + half * tan
    slopes = []
    corners = [(X0, Y0), (X1, Y0), (X1, Y1), (X0, Y1)]
    for k in range(4):
        (ax, ay), (bx, by) = corners[k], corners[(k + 1) % 4]
        along = Vector((bx - ax, by - ay, 0.0)).normalized()
        out = Vector((along.y, -along.x, 0.0))
        up = (-out * math.cos(theta) + Vector((0.0, 0.0, math.sin(theta))))
        normal = out * math.sin(theta) + Vector((0.0, 0.0, math.cos(theta)))
        origin = Vector((ax, ay, EAVE)) - along * OV + out * OV - Vector((0.0, 0.0, OV * tan)) + normal * 0.1
        slope = kit.Slope(origin, along, up, (X1 - X0) + 2.0 * OV, length, 0.0)
        bounds = homes.hip_bounds(slope, theta)
        homes.hip_deck(m, slope, bounds, 0.1, length)
        homes.hip_shakes(m, slope, bounds, length - 0.05, piece=(2.4, 4.0))
        slopes.append((slope, normal))
    for k in range(4):
        slope, n0 = slopes[k]
        _, n1 = slopes[(k + 1) % 4]
        p0 = slope.world((slope.width, 0.0, 0.0))
        p1 = Vector((0.0, 0.0, peak + 0.1))
        homes.cap_line(m, p0, p1, (n0 + n1).normalized(), width=0.22, thick=0.05, look='C', lift=0.05)
    homes.stack(m, 0.0, 0.0, peak - 0.9, peak + 0.85, size=(0.7, 0.7), taper=0.02)
    m.section('roof')

    # The wraparound porch: across the front and down the right side, turned posts, a railing, steps to the front door,
    # a hipped roof turning the corner.
    PD = 2.0
    PY, PX = Y0 - PD, X1 + PD
    DECK = BASE
    town.deck(m, X0 - 0.1, PX, PY, Y0, top=DECK, posts=(X0, -1.0, 1.4, PX - 0.12), sill=True, ends=(X0 - 0.1,),
              plank_cuts=0)
    # The side deck: planks across it, from the wall out, on three joists, with rim boards.
    for y in kit.frange(Y0 + 0.1, Y1 + 0.1, 0.2):
        town.board(m, (X1 + 0.02, y, DECK - 0.025), (PX + 0.02, y + m.rng.uniform(-0.01, 0.01), DECK - 0.025), 0.188, 0.05,
                   look='planks', face=(0.0, 0.0, 1.0), drop=('+y', '-x'))
    for y in (Y0 + 0.4, 0.0, Y1 - 0.2):
        m.box((PX - X1, 0.08, 0.16), at=((X1 + PX) * 0.5, y, DECK - 0.13), uv='C', drop=('-z',))
    town.board(m, (PX, Y0, DECK - 0.15), (PX, Y1 + 0.1, DECK - 0.15), 0.2, 0.045, look='A', face=(1.0, 0.0, 0.0))
    town.board(m, (X1, Y1 + 0.1, DECK - 0.15), (PX, Y1 + 0.1, DECK - 0.15), 0.2, 0.045, look='A', face=(0.0, 1.0, 0.0))
    homes.steps(m, 0.0, 1.7, PY, DECK, 2)
    TOP = EAVE - 0.5
    PPY, PPX = PY + 0.12, PX - 0.12
    posts = [(X0 + 0.05, PPY), (-1.05, PPY), (1.05, PPY), (PPX, PPY), (PPX, -1.15), (PPX, 1.15), (PPX, Y1 - 0.05)]
    for x, y in posts:
        homes.post(m, x, y, DECK, TOP - 0.1, size=0.15, look='woodcream', turned=True)
    m.board((X0 - 0.15, PPY, TOP), (PPX + 0.1, PPY, TOP), 0.18, 0.12, uv=Trim('C', lane='each'))
    m.board((PPX, PPY - 0.1, TOP), (PPX, Y1 + 0.15, TOP), 0.18, 0.12, uv=Trim('C', lane='each'))
    rail = dict(height=0.86, every=0.18, look='woodcream', picket='woodcream')
    homes.railing(m, (X0 + 0.12, PPY, DECK), (-1.12, PPY, DECK), **rail)
    homes.railing(m, (1.12, PPY, DECK), (PPX - 0.08, PPY, DECK), **rail)
    for a, b in ((PPY + 0.08, -1.22), (-1.08, 1.08), (1.22, Y1 - 0.12)):
        homes.railing(m, (PPX, a, DECK), (PPX, b, DECK), **rail)
    # The porch roof: two shed planes meeting at a hip over the corner.
    p_theta = math.atan2(EAVE - 0.12 - (TOP + 0.09), PD - 0.12)
    p_len = (PD - 0.12 + 0.3) / math.cos(p_theta)
    pc, ps = math.cos(p_theta), math.sin(p_theta)
    front_origin = Vector((X0 - 0.25, PPY - 0.3, TOP + 0.09 - 0.3 * math.tan(p_theta)))
    front_slope = kit.Slope(front_origin, (1.0, 0.0, 0.0), (0.0, pc, ps), (PPX + 0.3) - (X0 - 0.25), p_len, 0.03)
    # The side plane's eave runs from the front corner back (+Y), so x along it by up the slope points out of the roof;
    # its hip is at its front end.
    side_origin = Vector((PPX + 0.3, PPY - 0.3, TOP + 0.09 - 0.3 * math.tan(p_theta)))
    side_slope = kit.Slope(side_origin, (0.0, 1.0, 0.0), (-pc, 0.0, ps), (Y1 + 0.25) - (PPY - 0.3), p_len, 0.03)
    for slope, ends in ((front_slope, (False, True)), (side_slope, (True, False))):
        bounds = homes.hip_bounds(slope, p_theta, ends=ends)
        homes.hip_deck(m, slope, bounds, 0.06, p_len)
        homes.hip_shakes(m, slope, bounds, p_len - 0.03, piece=(2.0, 3.4))
    n0 = Vector((0.0, -ps, pc))
    n1 = Vector((ps, 0.0, pc))
    homes.cap_line(m, front_slope.world((front_slope.width, 0.0, 0.0)), Vector((X1, Y0, EAVE - 0.12)),
                   (n0 + n1).normalized(), width=0.2, thick=0.04, look='C', lift=0.05)
    m.section('porch')
    homes.barrel(m, (X0 - 0.45, Y1 - 0.5, 0.0))
    m.section('dressing')

    m.hull((X1 - X0 + 0.1, Y1 - Y0 + 0.1, EAVE), at=(0.0, 0.0, EAVE * 0.5))
    m.hull_points([(x, y, EAVE - OV * tan + 0.22) for x in (X0 - OV, X1 + OV) for y in (Y0 - OV, Y1 + OV)] +
                  [(0.0, 0.0, peak + 0.25)])
    m.hull((PX - X0 + 0.1, Y0 - PY, DECK), at=((PX + X0) * 0.5, (Y0 + PY) * 0.5, DECK * 0.5))
    m.hull((PX - X1, Y1 - Y0 + 0.1, DECK), at=((PX + X1) * 0.5, (Y0 + Y1) * 0.5, DECK * 0.5))
    for x, y in posts:
        m.hull((0.17, 0.17, TOP - DECK), at=(x, y, (TOP + DECK) * 0.5))
    homes.slope_hull(m, front_slope, thick=0.18)
    homes.slope_hull(m, side_slope, thick=0.18)
    m.socket('Light', (-1.9, Y0 + 1.0, BASE + 1.5))
    return m.finish(pivot=(0.0, Y0), footprint=(X0, X1, Y0, Y1))


# --- RR_Home_I: the L house ---

def home_i():
    m = homes.Home('RR_Home_I', seed=509)
    X0, X1, Y0, Y1 = -4.0, 4.0, -2.4, 2.4          # the main block
    WX0, WX1, WY0 = 0.6, 3.6, -5.4                 # the wing, out to the front
    BASE, WALL = 0.5, 3.4
    EAVE = BASE + WALL
    PM, PW = 40.0, 45.0
    tm, tw = math.tan(math.radians(PM)), math.tan(math.radians(PW))
    RIDGE_M = EAVE + (Y1 - Y0) * 0.5 * tm
    RIDGE_W = EAVE + (WX1 - WX0) * 0.5 * tw
    rng = m.rng
    plan = [(X0, Y0), (WX0, Y0), (WX0, WY0), (WX1, WY0), (WX1, Y0), (X1, Y0), (X1, Y1), (X0, Y1)]
    homes.plinth(m, X0, X1, Y0, Y1, BASE)
    homes.plinth(m, WX0, WX1, WY0, Y0 - 0.3, BASE)
    m.section('foundations')

    win = dict(paint='woodcream', sash='woodcream', shutter='A', casing_w=0.1)
    DOOR = Opening('door', 1.9, 0.0, 1.0, 2.2, style='panel', finish='planks', casing='woodcream', hinge='left',
                   transom=0.3, boarded=True)
    rise_w = (WX1 - WX0) * 0.5 * tw
    rise_m = (Y1 - Y0) * 0.5 * tm
    walls = [
        (rect(4.6, WALL), [Opening('window', 0.5, 0.9, 0.85, 1.4, style='boarded', **win), DOOR], True),
        (rect(3.0, WALL), [Opening('window', 1.1, 0.9, 0.85, 1.4, style='shut', **win)], False),
        (gable_outline(3.0, WALL, rise_w), [Opening('window', 0.4, 0.9, 0.8, 1.4, style='boarded', **win),
                                            Opening('window', 1.8, 0.9, 0.8, 1.4, style='hanging', **win),
                                            Opening('window', 1.15, WALL + 0.25, 0.7, 0.8, style='boarded', **win)], True),
        (rect(3.0, WALL), [Opening('window', 1.1, 0.9, 0.8, 1.4, style='shut', **win)], False),
        (rect(0.4, WALL), [], True),
        (gable_outline(4.8, WALL, rise_m), [Opening('window', 1.9, 0.9, 0.85, 1.4, style='boarded', **win),
                                            Opening('window', 2.0, WALL + 0.3, 0.75, 0.85, style='shut', **win)], False),
        (rect(8.0, WALL), [Opening('window', 1.3, 0.9, 0.85, 1.4, style='shut', **win),
                           Opening('door', 4.3, 0.0, 0.95, 2.1, style='plank', finish='A', casing='woodcream',
                                   boarded=True),
                           Opening('window', 6.2, 0.9, 0.85, 1.4, style='boarded', **win)], True),
        (gable_outline(4.8, WALL, rise_m), [Opening('window', 2.0, 0.9, 0.85, 1.4, style='hanging', **win),
                                            Opening('window', 2.05, WALL + 0.3, 0.75, 0.85, style='boarded', **win)],
         False),
    ]
    for (space, length), (outline, items, wrap) in zip(polygon_spaces(plan, BASE), walls):
        clad(m, space, length, outline, items, look='A', trim='woodcream', wrap=wrap, frieze=len(outline) == 4,
             corner_w=min(0.15, length * 0.5))
    m.section('walls')

    # The main roof and the wing's: the wing's ridge runs back until it dies into the main roof's front slope. The main
    # front slope has broken in over the front room; the attic shows dark under it.
    HOLE = (1.2, 2.9, 0.75, 2.15)
    m_front = homes.plane(m, X0, X1, Y0, EAVE, PM, (Y1 - Y0) * 0.5, 1.0, overhang=0.35, rake=(0.3, 0.3), deck=0.1,
                          sag=0.1, hole=HOLE)
    m_back = homes.plane(m, X0, X1, Y1, EAVE, PM, (Y1 - Y0) * 0.5, -1.0, overhang=0.35, rake=(0.3, 0.3), deck=0.1,
                         sag=0.1)
    CH = (-2.6, 0.45)
    town.shakes(m, m_front, piece=(2.0, 3.4), skip=homes.ragged(HOLE, 91), missing=0.05)
    town.shakes(m, m_back, piece=(2.0, 3.4), skip=skip_round(m_back, [(CH[0], CH[1], 0.4, 0.45)]), missing=0.05)
    town.ridge_cap(m, m_front, m_back, uv='C', width=0.16, thick=0.05)
    meet = Y0 + (RIDGE_W - EAVE) / tm + 0.25          # where the wing's ridge meets the main front slope
    frame = Matrix.Rotation(math.radians(90.0), 4, 'Z')
    w_right = homes.plane(m, WY0, meet, -WX1, EAVE, PW, (WX1 - WX0) * 0.5, 1.0, overhang=0.32, rake=(0.3, 0.0),
                          deck=0.1, sag=0.04, frame=frame, rake_boards=(True, False))
    w_left = homes.plane(m, WY0, meet, -WX0, EAVE, PW, (WX1 - WX0) * 0.5, -1.0, overhang=0.32, rake=(0.3, 0.0),
                         deck=0.1, sag=0.04, frame=frame, rake_boards=(True, False))
    town.shakes(m, w_right, piece=(2.0, 3.4), missing=0.03)
    town.shakes(m, w_left, piece=(2.0, 3.4), missing=0.03)
    town.ridge_cap(m, w_right, w_left, uv='C', width=0.16, thick=0.05)
    homes.attic(m, X0 + 0.1, X1 - 0.1, Y0 + 0.1, Y1 - 0.1, EAVE - 0.02,
                gables=[[(x, Y0 + 0.1, EAVE), (x, Y1 - 0.1, EAVE), (x, 0.0, RIDGE_M - 0.15)] for x in (X0 + 0.1, X1 - 0.1)])
    homes.stack(m, CH[0], CH[1], roof_z(EAVE, Y1, CH[1], PM) - 0.6, RIDGE_M + 0.8, size=(0.6, 0.7))
    m.section('roofs')

    # The porch in the crook, sagging at its lost corner post (lying in the grass before it).
    PY = Y0 - 1.9
    DECK = BASE - 0.05
    town.deck(m, X0 - 0.05, WX0, PY, Y0, top=DECK, posts=(X0 + 0.1, -1.7, WX0 - 0.1), sill=True, ends=(X0 - 0.05,))
    homes.steps(m, X0 + DOOR.x + DOOR.w * 0.5, 1.3, PY, DECK, 2)
    TOP = BASE + 2.75
    SLACK = 0.5
    PPY = PY + 0.12
    homes.post(m, -1.55, PPY, DECK, TOP - 0.08 - SLACK * 0.35, size=0.14)
    homes.post(m, WX0 - 0.12, PPY, DECK, TOP - 0.08, size=0.14)
    m.board((WX0 + 0.05, PPY, TOP), (X0 - 0.2, PPY, TOP - SLACK), 0.17, 0.12, uv=Trim('C', lane='each'))
    m.board((X0 - 0.4, PY - 0.6, 0.07), (X0 + 1.3, PY - 1.6, 0.09), 0.14, 0.14, face=(0.0, 0.0, 1.0),
            uv=Trim('C', lane='each'))
    porch = homes.shed_roof(m, X0 - 0.05, WX0 - 0.02, PPY, TOP + 0.09, Y0 + 0.05, EAVE - 0.25, overhang=0.28,
                            rake=(0.2, 0.0), deck=0.06, sag=0.05, cls=homes.Droop, corner=SLACK, side='left')
    town.shakes(m, porch, piece=(1.6, 2.8), top=porch.length - 0.02, missing=0.08)
    m.section('porch')

    m.hull((X1 - X0 + 0.1, Y1 - Y0 + 0.1, EAVE), at=(0.0, 0.0, EAVE * 0.5))
    m.hull((WX1 - WX0 + 0.1, Y0 - WY0 + 0.05, EAVE), at=((WX0 + WX1) * 0.5, (Y0 + WY0) * 0.5, EAVE * 0.5))
    homes.gable_hull(m, X0, X1, Y0, Y1, EAVE, EAVE, 0.0, RIDGE_M, overhang=0.35, rake=0.3, pitch_front=PM)
    wz = EAVE - 0.32 * tw + 0.22
    m.hull_points([(x, y, wz) for x in (WX0 - 0.32, WX1 + 0.32) for y in (WY0 - 0.3, Y0)] +
                  [((WX0 + WX1) * 0.5, y, RIDGE_W + 0.25) for y in (WY0 - 0.3, Y0)])
    m.hull((WX0 - X0 + 0.05, Y0 - PY, DECK), at=((X0 + WX0) * 0.5, (Y0 + PY) * 0.5, DECK * 0.5))
    for x in (-1.55, WX0 - 0.12):
        m.hull((0.15, 0.15, TOP - DECK - 0.2), at=(x, PPY, (TOP + DECK - 0.2) * 0.5))
    homes.slope_hull(m, porch, thick=0.18)
    return m.finish(pivot=(X0 + DOOR.x + DOOR.w * 0.5, Y0), footprint=(X0, X1, WY0, Y1))


# --- RR_Home_J: the tin shack ---

def home_j():
    m = homes.Home('RR_Home_J', seed=510)
    X0, X1, Y0, Y1 = -2.1, 2.1, -1.8, 1.8
    BASE = 0.28
    FRONT_H, BACK_H = 2.85, 2.1
    rng = m.rng
    # Sill logs on flat stones, the floor's underside.
    for x in (X0 + 0.15, X1 - 0.15):
        for y in (Y0 + 0.2, Y1 - 0.2):
            kit.stone_stack(m, (x, y, -0.2), (0.34, 0.34), 0.3, taper=0.1, rot=rng.uniform(-8.0, 8.0))
    for y in (Y0 + 0.08, Y1 - 0.08):
        kit.log(m, (X0 - 0.12, y, BASE - 0.12), (X1 + 0.12, y, BASE - 0.12), 0.12, (0.0, -1.0, 0.0), sides=8)
    m.section('sills')

    DOOR = Opening('door', 1.65, 0.0, 0.9, 2.0, style='plank', finish='A', casing='C', hinge='right', boarded=True)
    win = dict(paint='C', sash='A', casing_w=0.09)
    walls = [
        ([DOOR, Opening('window', 2.95, 1.0, 0.7, 0.75, style='boarded', **win)],
         rect(X1 - X0, FRONT_H)),
        ([Opening('window', 1.3, 1.05, 0.7, 0.75, style='shut', shutter='A', **win)],
         [(0.0, 0.0), (Y1 - Y0, 0.0), (Y1 - Y0, BACK_H), (0.0, FRONT_H)]),
        ([], rect(X1 - X0, BACK_H)),
        ([], [(0.0, 0.0), (Y1 - Y0, 0.0), (Y1 - Y0, FRONT_H), (0.0, BACK_H)]),
    ]
    for k, ((space, length), (items, outline)) in enumerate(zip(wall_spaces(X0, X1, Y0, Y1, BASE), walls)):
        homes.shell(m, notch(outline, items), kit.holes(items), 0.1, Trim('A', world=True, u=rng.uniform(0.0, 6.4)),
                    space, slices=0)
        top = max(z for x, z in outline)
        # Rough planks nailed level, a little apart, one or two gone.
        z = 0.0
        row = 0
        while z < top - 0.05:
            h = 0.22
            spans = town.intersect(town.spans_at(outline, z + 0.01), town.spans_at(outline, z + h - 0.01))
            for x0, x1, k0, k1 in homes.keep_out(items, casing=0.09):
                if k0 < z + h and k1 > z:
                    spans = town.subtract(spans, (x0, x1))
            for a, b in spans:
                if rng.random() < 0.04:
                    continue
                town.board(m, (a - 0.02, 0.0, z + h * 0.5), (b + 0.02, 0.0, z + h * 0.5 + rng.uniform(-0.01, 0.01)),
                           h - 0.025, 0.022, look='planks', space=space, lift=0.011, drop=('+y',))
            z += h
            row += 1
        homes.openings(m, space, items, 0.1, out=0.022)
        for x in (0.06, length - 0.06):
            h0 = outline[-1][1] if x < 1.0 else outline[2][1]
            m.board((x, -0.04, -0.05), (x, -0.04, h0 - 0.02), 0.12, 0.1, uv=Trim('C', lane='each'), space=space)
    m.section('walls')

    # The shed roof: rough planks lapped across it on a deck, a fascia along its high edge.
    roof = homes.shed_roof(m, X0, X1, Y1, BASE + BACK_H, Y0, BASE + FRONT_H, overhang=0.35, rake=(0.3, 0.3), deck=0.05,
                           sag=0.06)
    homes.lapped_roof(m, roof, top=roof.length + 0.02)
    m.board((X0 - 0.35, Y0 - 0.06, BASE + FRONT_H + 0.02), (X1 + 0.35, Y0 - 0.06, BASE + FRONT_H + 0.02), 0.18, 0.04,
            uv='C')
    m.section('roof')

    # An awning of boards over the door on two poles, the left one knocked askew.
    AW = (-1.0, 1.0)
    AY = Y0 - 1.25
    AZ, WZ = BASE + 1.95, BASE + 2.35
    for x, lean in ((AW[0] + 0.08, 0.16), (AW[1] - 0.08, 0.0)):
        kit.log(m, (x, AY + 0.1, -0.15), (x + lean, AY + 0.1 + lean * 0.4, AZ - 0.05 - lean * 0.6), 0.055,
                (0.0, -1.0, 0.0), sides=6)
    kit.log(m, (AW[0] - 0.12, AY + 0.1, AZ - 0.1), (AW[1] + 0.12, AY + 0.1, AZ), 0.06, (0.0, -1.0, 0.0), sides=6)
    awning = homes.shed_roof(m, AW[0] - 0.05, AW[1] + 0.05, AY + 0.1, AZ + 0.06, Y0 - 0.02, WZ, overhang=0.12,
                             rake=(0.08, 0.08), deck=0.03, sag=0.03, cls=homes.Droop, corner=0.12, side='left')
    homes.board_roof(m, awning, every=0.22, missing=(3,), battens=False)
    m.section('awning')

    # The stovepipe out through the left wall, an elbow, and up past the eave.
    PZ = BASE + 1.85
    tile = Tile('MetalWorn')
    m.cylinder((X0 + 0.05, 0.3, PZ), (X0 - 0.42, 0.3, PZ), 0.08, sides=8, uv=tile, mat='iron', caps=(False, False))
    m.box((0.2, 0.2, 0.2), at=(X0 - 0.48, 0.3, PZ), uv=tile, mat='iron', bevel=0.03)
    homes.stovepipe(m, X0 - 0.48, 0.3, PZ + 0.08, BASE + FRONT_H + 0.6, radius=0.08)
    m.board((X0 - 0.02, 0.3, PZ + 0.7), (X0 - 0.48, 0.3, PZ + 0.7), 0.03, 0.01, face=(0.0, -1.0, 0.0), uv=tile,
            mat='iron')
    m.section('stovepipe')

    # The yard: a chopping block with split wood about it, a rain barrel at the back corner.
    cx, cy = X1 + 0.85, Y0 - 0.5
    m.cylinder((cx, cy, -0.05), (cx, cy, 0.5), 0.24, sides=9, uv=Trim('B', v=0.0), face=(0.0, -1.0, 0.0),
               caps=(False, True), cap_uv=kit.EndGrain(), cap_mat='endgrain')
    for k in range(5):
        a = rng.uniform(0.0, 6.28)
        r = rng.uniform(0.35, 0.8)
        px, py = cx + r * math.cos(a), cy + r * math.sin(a)
        b = rng.uniform(0.0, 3.14)
        m.cylinder((px, py, 0.05), (px + 0.4 * math.cos(b), py + 0.4 * math.sin(b), 0.06), 0.06, sides=3,
                   uv=Trim('C'), face=(0.0, 0.0, 1.0), phase=rng.uniform(0.0, 360.0), cap_uv=kit.EndGrain(),
                   cap_mat='endgrain')
    homes.barrel(m, (X1 + 0.38, Y1 - 0.2, 0.0), height=0.85, radius=0.28)
    m.hull((0.5, 0.5, 0.55), at=(cx, cy, 0.27))
    m.section('yard')

    m.hull((X1 - X0 + 0.1, Y1 - Y0 + 0.1, BASE + BACK_H), at=(0.0, 0.0, (BASE + BACK_H) * 0.5))
    m.hull_points([(x, y, z) for x in (X0 - 0.3, X1 + 0.3) for y, z in ((Y0 - 0.06, BASE + BACK_H),
                                                                        (Y0 - 0.35, BASE + FRONT_H + 0.12),
                                                                        (Y1 + 0.35, BASE + BACK_H - 0.1))])
    for x in (AW[0] + 0.12, AW[1] - 0.08):
        m.hull((0.13, 0.13, AZ), at=(x, AY + 0.1, AZ * 0.5))
    homes.slope_hull(m, awning, thick=0.12)
    return m.finish(pivot=(X0 + DOOR.x + DOOR.w * 0.5, Y0), footprint=(X0, X1, Y0, Y1))


# --- RR_Well: the sweep well ---

def rope(m, p0, p1, r=0.014):
    """A rope from p0 to p1: a lane of the weathered siding, which reads as old hemp this thin (Well.py's)."""
    m.cylinder(p0, p1, r, sides=5, uv=Trim('A', lane=3), caps=(False, False))


def bucket(m, at, look_iron='iron'):
    """A stave bucket with two iron hoops and a bail (Well.py's)."""
    bx, by, bz = at
    tile = Tile('MetalWorn')
    m.cylinder((bx, by, bz - 0.13), (bx, by, bz + 0.13), 0.13, 0.155, sides=10, uv=Trim('A', lane=1), caps=(True, False))
    for z in (bz - 0.07, bz + 0.08):
        m.cylinder((bx, by, z - 0.02), (bx, by, z + 0.02), 0.144 + (z - bz) * 0.1, sides=10, uv=tile, mat=look_iron,
                   caps=(False, False))
    m.board((bx - 0.16, by, bz + 0.12), (bx, by, bz + 0.27), 0.02, 0.015, face=(0.0, -1.0, 0.0), uv=tile, mat=look_iron)
    m.board((bx, by, bz + 0.27), (bx + 0.16, by, bz + 0.12), 0.02, 0.015, face=(0.0, -1.0, 0.0), uv=tile, mat=look_iron)


def rr_well():
    """A sweep well: a crib of hewn timbers over the shaft, a forked post beside it carrying a long sweep pole on an
    iron pin, a stone lashed to its short end as the counterweight, and from the long end a pole down to the bucket over
    the water; a plank trough beside it. Its pivot is the middle of the crib at ground level."""
    m = homes.Home('RR_Well', seed=521)
    rng = m.rng
    H = 0.62                                      # the crib's half width (outer faces)
    for k in range(4):
        z = -0.3 + k * 0.27
        # Each course goes round all four sides: the logs along X, and those along Y half a log higher, notched over
        # them at the corners, where their ends run past.
        for side in (-1.0, 1.0):
            m.box((2.0 * H + 0.2, 0.18, 0.24), at=(0.0, side * (H - 0.09), z + 0.12),
                  rot=(0.0, 0.0, rng.uniform(-1.0, 1.0)), uv=Trim('C', width=0.24), cuts=0)
            m.box((0.18, 2.0 * H + 0.2, 0.24), at=(side * (H - 0.09), 0.0, z + 0.12 + 0.135),
                  rot=(0.0, 0.0, rng.uniform(-1.0, 1.0)), uv=Trim('C', width=0.24), cuts=0)
    TOP = -0.3 + 3 * 0.27 + 0.135 + 0.24
    m.box((2.0 * H - 0.36, 2.0 * H - 0.36, 0.02), at=(0.0, 0.0, 0.22), uv=Trim('H4', fit=True), drop=('-z',))
    for k, y in enumerate((-0.1, 0.12)):        # half the shaft boarded over
        m.board((-H - 0.05, y + 0.25, TOP + 0.02), (H + 0.05, y + 0.25 + rng.uniform(-0.03, 0.03), TOP + 0.02), 0.21,
                0.04, face=(0.0, 0.0, 1.0), uv='A')
    for k in range(5):                            # flat stones round its foot
        a = 2.0 * math.pi * (k + rng.uniform(0.0, 0.5)) / 5.0
        r = H + rng.uniform(0.2, 0.45)
        m.box((rng.uniform(0.3, 0.45), rng.uniform(0.22, 0.32), 0.07), at=(r * math.cos(a), r * math.sin(a), 0.01),
              rot=(rng.uniform(-3, 3), rng.uniform(-3, 3), math.degrees(a)), uv='D', bevel=0.02)
    m.section('crib')

    PX = -2.9
    PIN = Vector((PX, 0.0, 2.88))
    kit.log(m, (PX, 0.0, -0.3), (PX, 0.0, 2.68), 0.13, (0.0, -1.0, 0.0), sides=8, r1=0.115)
    for side in (-1.0, 1.0):
        kit.log(m, (PX, side * 0.02, 2.5), (PX, side * 0.17, 3.05), 0.06, (0.0, -1.0, 0.0), sides=6)
    m.cylinder((PX, -0.22, PIN.z), (PX, 0.22, PIN.z), 0.022, sides=6, uv=Tile('MetalWorn'), mat='iron')
    a = math.radians(33.0)
    along = Vector((math.cos(a), 0.0, math.sin(a)))
    long_end, short_end = PIN + along * 3.75, PIN - along * 1.85
    kit.log(m, short_end, long_end, 0.085, (0.0, -1.0, 0.0), sides=8, r1=0.05)
    rope(m, short_end + Vector((0.02, 0.0, -0.06)), short_end + Vector((0.05, 0.0, -0.36)), 0.016)
    kit.stone_stack(m, (short_end.x + 0.05, 0.0, short_end.z - 0.72), (0.46, 0.4), 0.36, taper=0.15, bevel=0.05)
    hang = Vector((long_end.x, 0.0, 1.78))
    kit.log(m, long_end + Vector((0.0, 0.0, -0.03)), hang, 0.03, (0.0, -1.0, 0.0), sides=5)
    bucket(m, (hang.x, 0.0, hang.z - 0.3))
    m.section('sweep')

    # The trough: a plank box on two blocks, its water dark under the rim.
    TY, TL, TW, TH = -1.25, 1.9, 0.5, 0.48
    for x in (-TL * 0.32, TL * 0.32):
        m.box((0.16, TW + 0.1, 0.12), at=(x - 0.6, TY, 0.06), uv='C')
    for side in (-1.0, 1.0):
        m.board((-0.6 - TL * 0.5, TY + side * TW * 0.5, 0.12 + TH * 0.5), (-0.6 + TL * 0.5, TY + side * TW * 0.5,
                0.12 + TH * 0.5), TH, 0.05, face=(0.0, side, 0.0), uv='A')
        m.board((-0.6 + side * (TL * 0.5 - 0.03), TY - TW * 0.5, 0.12 + TH * 0.5), (-0.6 + side * (TL * 0.5 - 0.03),
                TY + TW * 0.5, 0.12 + TH * 0.5), TH, 0.05, face=(side, 0.0, 0.0), uv='A')
    m.box((TL - 0.08, TW - 0.06, 0.02), at=(-0.6, TY, 0.12 + TH - 0.1), uv=Trim('H4', fit=True), drop=('-z',))
    m.section('trough')

    m.hull((2.0 * H + 0.2, 2.0 * H + 0.2, TOP + 0.06), at=(0.0, 0.0, TOP * 0.5 + 0.03))
    m.hull((0.3, 0.42, 3.1), at=(PX, 0.0, 1.55))
    m.hull_points([short_end + Vector((0.0, s * 0.09, t * 0.09)) for s in (-1, 1) for t in (-1, 1)] +
                  [long_end + Vector((0.0, s * 0.06, t * 0.06)) for s in (-1, 1) for t in (-1, 1)])
    m.hull((0.5, 0.44, 0.4), at=(short_end.x + 0.05, 0.0, short_end.z - 0.54))
    m.hull((TL, TW + 0.1, TH + 0.12), at=(-0.6, TY, (TH + 0.12) * 0.5))
    return m.finish(pivot=(0.0, 0.0), footprint=(-H, H, -H, H))


# --- RR_Outhouse: the privy ---

def star(cx, cz, r0, r1, points=5, turn=90.0):
    """A star's outline (x, z), counterclockwise."""
    pts = []
    for k in range(points * 2):
        a = math.radians(turn) + math.pi * k / points
        r = r0 if k % 2 == 0 else r1
        pts.append((cx + r * math.cos(a), cz + r * math.sin(a)))
    return pts


def rr_outhouse():
    """A privy under a little gable of shakes, grey board-and-batten on two flat stones, a star cut through its door
    (Skyreach's has the crescent and a tin roof), a vent slot in the gable. Its pivot is the door at ground level."""
    m = homes.Home('RR_Outhouse', seed=522)
    rng = m.rng
    X0, X1, Y0, Y1 = -0.72, 0.72, -0.68, 0.68
    BASE, WALL, PITCH = 0.16, 2.05, 48.0
    EAVE = BASE + WALL
    tan = math.tan(math.radians(PITCH))
    RISE = (X1 - X0) * 0.5 * tan
    for y in (Y0 + 0.18, Y1 - 0.18):
        kit.stone_stack(m, (0.0, y, -0.15), (X1 - X0 + 0.18, 0.4), BASE + 0.13, rot=rng.uniform(-3.0, 3.0), bevel=0.02)
    DOOR = Opening('door', 0.27, 0.0, 0.9, 1.9)
    for k, ((space, length), items) in enumerate(zip(wall_spaces(X0, X1, Y0, Y1, BASE),
                                                     [[DOOR], [], [], []])):
        gable = k % 2 == 0
        outline = gable_outline(length, WALL, RISE) if gable else rect(length, WALL)
        homes.shell(m, notch(outline, items), (), 0.05, Trim('A', world=True, rotate=True, u=rng.uniform(0.0, 6.4)),
                    space, around=items)
        homes.battens(m, space, outline, 0.0, length, homes.keep_out(items) + [(-0.1, 0.11, -1, 9), (length - 0.11,
                                                                                                    length + 0.1, -1, 9)],
                      look='A', every=0.36)
        homes.corners(m, space, length, outline[-1][1] - 0.02, outline[2][1] - 0.02, look='C', width=0.1, wrap=gable)
    front = kit.wall_space((X0, Y0), (X1, Y0), BASE)
    # The door: upright planks with a star cut through, a Z brace, strap hinges, a turn-button; dark behind the star.
    leaf = kit.rect(DOOR.x + 0.01, 0.0, DOOR.w - 0.02, DOOR.h)
    m.panel(leaf, [star(DOOR.x + DOOR.w * 0.5, 1.55, 0.12, 0.05)], 0.04, Trim('A', world=True, rotate=True),
            space=front, matrix=Matrix.Translation((0.0, 0.015, 0.0)))
    homes.quad(m, [(DOOR.x, 0.4, 1.3), (DOOR.x + DOOR.w, 0.4, 1.3), (DOOR.x + DOOR.w, 0.4, 1.85), (DOOR.x, 0.4, 1.85)],
               Trim('A', world=True, rotate=True), space=front, facing=(0.0, -1.0, 0.0))
    for z in (0.3, 1.2):
        m.board((DOOR.x + 0.05, -0.015, z), (DOOR.x + DOOR.w - 0.05, -0.015, z), 0.12, 0.03, uv='A', space=front)
    m.board((DOOR.x + DOOR.w - 0.1, -0.015, 0.36), (DOOR.x + 0.1, -0.015, 1.14), 0.1, 0.025, uv='A', space=front)
    for z in (0.3, 1.2):
        m.board((DOOR.x - 0.04, -0.04, z), (DOOR.x + 0.34, -0.04, z), 0.06, 0.012, uv=Trim('H3', fit=True), space=front)
    m.box((0.04, 0.04, 0.12), at=(DOOR.x + DOOR.w - 0.03, -0.035, 1.0), rot=(0.0, 25.0, 0.0), uv='C', space=front)
    for x in (DOOR.x - 0.05, DOOR.x + DOOR.w + 0.05):
        kit.trim_board(m, front, (x, 0.0, -0.02), (x, 0.0, DOOR.h + 0.08), 0.1, 'C', thick=0.035)
    kit.trim_board(m, front, (DOOR.x - 0.1, 0.0, DOOR.h + 0.06), (DOOR.x + DOOR.w + 0.1, 0.0, DOOR.h + 0.06), 0.1, 'C',
                   thick=0.035)
    # A vent slot high in the front gable, under a little drip board.
    vz = WALL + RISE * 0.3
    m.box((0.42, 0.02, 0.1), at=((X1 - X0) * 0.5, 0.03, vz), uv=Trim('H4', fit=True), space=front)
    m.box((0.5, 0.06, 0.03), at=((X1 - X0) * 0.5, -0.03, vz + 0.08), uv='C', space=front)
    m.box((1.0, 0.34, 0.05), at=(0.0, Y0 - 0.25, -0.01), rot=(0.0, 0.0, 3.0), uv='D', bevel=0.02)
    m.section('walls, door')
    frame = Matrix.Rotation(math.radians(90.0), 4, 'Z')
    right = homes.plane(m, Y0, Y1, -X1, EAVE, PITCH, (X1 - X0) * 0.5, 1.0, overhang=0.16, rake=(0.22, 0.18), deck=0.05,
                        sag=0.03, frame=frame)
    left = homes.plane(m, Y0, Y1, -X0, EAVE, PITCH, (X1 - X0) * 0.5, -1.0, overhang=0.16, rake=(0.22, 0.18), deck=0.05,
                       sag=0.03, frame=frame)
    town.shakes(m, right, piece=(0.8, 1.3), missing=0.04)
    town.shakes(m, left, piece=(0.8, 1.3), missing=0.04)
    town.ridge_cap(m, right, left, uv='C', width=0.12, thick=0.04)
    m.section('roof')
    m.hull((X1 - X0 + 0.06, Y1 - Y0 + 0.06, EAVE), at=(0.0, 0.0, EAVE * 0.5))
    m.hull_points([(x, y, EAVE - 0.16 * tan + 0.15) for x in (X0 - 0.16, X1 + 0.16) for y in (Y0 - 0.22, Y1 + 0.18)] +
                  [(0.0, y, EAVE + RISE + 0.2) for y in (Y0 - 0.22, Y1 + 0.18)])
    return m.finish(pivot=(X0 + DOOR.x + DOOR.w * 0.5, Y0), footprint=(X0, X1, Y0, Y1))


# --- RR_Windmill and RR_WindmillFan: the timber windpump ---

WM_BASE, WM_TOP, WM_HIGH = 1.35, 0.42, 7.6        # the tower's half width at the foot and the top, its height
WM_HUB = (0.0, -0.98, 8.38)


def wm_half(z):
    return WM_BASE + (WM_TOP - WM_BASE) * z / WM_HIGH


def rr_windmill():
    """A windpump on a tower of timbers (Skyreach's is steel lattice): four hewn legs leaning in, girts and X braces of
    boards, a ladder up the front to a plank platform, the head on a turntable with a long tail boom and a board vane
    in faded oxide, the pump rod down the middle to an iron pump on a block, and beside it a stave tank on a timber
    stand, fed by a pipe. SOCKET_Fan is the hub (RR_WindmillFan hangs there and turns about its front axis, as
    SM_WindmillFan does). Its pivot is the middle of the tower's foot at ground level."""
    m = homes.Home('RR_Windmill', seed=523)
    rng = m.rng
    corners_ = [(-1.0, -1.0), (1.0, -1.0), (1.0, 1.0), (-1.0, 1.0)]
    for sx, sy in corners_:
        kit.stone_stack(m, (sx * WM_BASE, sy * WM_BASE, -0.25), (0.5, 0.5), 0.45, taper=0.12,
                        rot=rng.uniform(-6.0, 6.0))
        m.board((sx * WM_BASE, sy * WM_BASE, 0.15), (sx * WM_TOP, sy * WM_TOP, WM_HIGH + 0.05), 0.2, 0.2,
                face=(sx, sy, 0.0), uv=Trim('C', lane='each'))
    levels = [0.4, 2.15, 3.75, 5.15, 6.4, 7.45]
    for z in levels:
        w = wm_half(z) + 0.1
        for (ax, ay), (bx, by) in zip(corners_, corners_[1:] + corners_[:1]):
            face = Vector(((ax + bx) * 0.5, (ay + by) * 0.5, 0.0)).normalized()
            m.board((ax * w, ay * w, z), (bx * w, by * w, z), 0.16, 0.06, face=face, uv=Trim('C'), cuts=0)
    for z0, z1 in zip(levels, levels[1:]):
        w0, w1 = wm_half(z0) + 0.13, wm_half(z1) + 0.13
        for (ax, ay), (bx, by) in zip(corners_, corners_[1:] + corners_[:1]):
            face = Vector(((ax + bx) * 0.5, (ay + by) * 0.5, 0.0)).normalized()
            m.board((ax * w0, ay * w0, z0 + 0.08), (bx * w1, by * w1, z1 - 0.08), 0.11, 0.035, face=face, uv='A',
                    cuts=0)
            m.board((bx * w0, by * w0, z0 + 0.08), (ax * w1, ay * w1, z1 - 0.08), 0.11, 0.035, face=face, uv='A',
                    cuts=0)
    m.section('tower')
    # The ladder up the front face, and the platform round the top with a rail.
    for x_frac in (-0.28, 0.28):
        m.board((x_frac * wm_half(0.0), -wm_half(0.0) - 0.16, 0.0), (x_frac * wm_half(7.4), -wm_half(7.4) - 0.16, 7.4),
                0.08, 0.05, face=(0.0, -1.0, 0.0), uv='C')
    for k in range(21):
        z = 0.35 + k * 0.34
        w = wm_half(z)
        m.board((-0.28 * w, -w - 0.17, z), (0.28 * w, -w - 0.17, z), 0.05, 0.04, face=(0.0, 0.0, 1.0), uv='C', cuts=0)
    PZ = WM_HIGH - 0.05
    for y in kit.frange(-0.95, 1.0, 0.2):
        if abs(y) < 0.3:
            for x0, x1 in ((-0.95, -0.3), (0.3, 0.95)):
                m.board((x0, y, PZ), (x1, y, PZ), 0.19, 0.045, face=(0.0, 0.0, 1.0), uv='A', cuts=0)
        else:
            m.board((-0.95, y, PZ), (0.95, y, PZ), 0.19, 0.045, face=(0.0, 0.0, 1.0), uv='A', cuts=0)
    for (ax, ay), (bx, by) in ((((-1.0, 1.0)), (1.0, 1.0)), ((1.0, -1.0), (1.0, 1.0)), ((-1.0, -1.0), (-1.0, 1.0))):
        m.board((ax * 0.95, ay * 0.95, PZ + 0.95), (bx * 0.95, by * 0.95, PZ + 0.95), 0.07, 0.07, face=(0.0, 0.0, 1.0),
                uv='C', cuts=0)
        for px, py in ((ax, ay), (bx, by)):
            m.board((px * 0.95, py * 0.95, PZ), (px * 0.95, py * 0.95, PZ + 0.95), 0.07, 0.07, uv='C', cuts=0)
    m.section('ladder, platform')
    # The head: a turntable, the gearbox, the shaft out to the hub, the tail boom and its vane.
    iron = Tile('MetalWorn')
    m.cylinder((0.0, 0.0, WM_HIGH), (0.0, 0.0, WM_HIGH + 0.14), 0.36, sides=10, uv=Trim('C'))
    m.box((0.36, 1.0, 0.42), at=(0.0, -0.15, WM_HUB[2] - 0.1), uv=iron, mat='iron', bevel=0.03)
    m.cylinder((0.0, -0.6, WM_HUB[2]), (0.0, WM_HUB[1] + 0.07, WM_HUB[2]), 0.065, sides=8, uv=iron, mat='iron')
    m.board((0.0, 0.25, WM_HIGH + 0.55), (0.0, 3.1, WM_HIGH + 0.85), 0.14, 0.1, face=(1.0, 0.0, 0.0),
            uv=Trim('C', lane='each'))
    m.board((0.0, 0.3, WM_HIGH + 0.25), (0.0, 1.9, WM_HIGH + 0.75), 0.08, 0.06, face=(1.0, 0.0, 0.0), uv='C')
    vane = kit.Space(kit.basis((0.0, 3.0, WM_HIGH + 0.85), (0.0, 1.0, 0.0), (-1.0, 0.0, 0.0), (0.0, 0.0, 1.0)))
    for k, (z0, z1) in enumerate(((-0.55, -0.33), (-0.33, -0.11), (-0.11, 0.11), (0.11, 0.33), (0.33, 0.55))):
        length = 1.7 - abs((z0 + z1) * 0.5) * 0.9
        m.board((0.0, 0.0, (z0 + z1) * 0.5), (length, 0.0, (z0 + z1) * 0.5), 0.195, 0.025, uv='H2', space=vane)
    for x in (0.05, 0.9):
        m.board((x, -0.03, -0.55), (x, -0.03, 0.55), 0.07, 0.035, uv='C', space=vane)
    m.socket('Fan', WM_HUB)
    m.section('head')
    # The pump: the rod down the middle to an iron pitcher pump on a block; the stave tank and its pipe.
    m.cylinder((0.0, 0.0, 0.9), (0.0, 0.0, WM_HIGH + 0.02), 0.022, sides=5, uv=iron, mat='iron', caps=(False, False))
    m.box((0.5, 0.5, 0.42), at=(0.0, 0.0, 0.21), uv=Trim('C', width=0.42))
    homes.lathe(m, [(0.0, 0.42), (0.09, 0.42), (0.09, 0.9), (0.06, 0.95), (0.0, 0.95)], (0.0, 0.0, 0.0), 8, iron,
                'iron')
    m.cylinder((0.08, 0.0, 0.8), (0.42, 0.0, 0.74), 0.04, sides=6, uv=iron, mat='iron')
    TX, TY, TR, TS, TT = 2.75, 0.6, 1.0, 0.55, 1.3      # the tank: middle, radius, its stand's height, its own height
    for sx in (-1.0, 1.0):
        for sy in (-1.0, 1.0):
            m.board((TX + sx * 0.7, TY + sy * 0.7, -0.1), (TX + sx * 0.7, TY + sy * 0.7, TS - 0.06), 0.16, 0.16,
                    uv=Trim('C', lane='each'))
    for y in kit.frange(TY - 1.0, TY + 1.05, 0.2):
        m.board((TX - 1.05, y, TS - 0.025), (TX + 1.05, y, TS - 0.025), 0.19, 0.05, face=(0.0, 0.0, 1.0), uv='A',
                cuts=0)
    homes.lathe(m, [(0.0, 0.0), (TR, 0.0), (TR * 1.02, TT * 0.5), (TR, TT), (TR - 0.05, TT), (TR - 0.05, TT - 0.18)],
                (TX, TY, TS), 16, homes.Boards(rotate=True), 'planks')
    # The water inside, a little under the rim: a disc turned to face up.
    m.emit(kit._disc(TR - 0.05, 16, 0.0), Trim('H4', fit=True), 'trim',
           place((TX, TY, TS + TT - 0.18), (-90.0, 0.0, 0.0)))
    for hz in (0.2, 0.65, 1.1):
        r = TR * (1.0 + 0.02 * math.sin(math.pi * hz / TT)) + 0.012
        m.cylinder((TX, TY, TS + hz - 0.025), (TX, TY, TS + hz + 0.025), r, sides=16, uv=iron, mat='iron',
                   caps=(False, False), face=(0.0, 0.0, 1.0))
    m.cylinder((0.42, 0.0, 0.74), (TX - TR * 0.92, 0.0, TS + TT - 0.25), 0.04, sides=6, uv=iron, mat='iron')
    m.section('pump, tank')
    m.hull_points([(sx * wm_half(z) * 1.08, sy * wm_half(z) * 1.08, z) for sx, sy in corners_ for z in (0.0, PZ)])
    m.hull((2.0, 2.0, 0.12), at=(0.0, 0.0, PZ))
    m.hull((0.5, 1.2, 0.9), at=(0.0, -0.1, WM_HIGH + 0.4))
    m.hull_points([(TX + 1.06 * math.cos(2.0 * math.pi * k / 12), TY + 1.06 * math.sin(2.0 * math.pi * k / 12), z)
                   for k in range(12) for z in (0.0, TS + TT)])
    return m.finish(pivot=(0.0, 0.0), footprint=(-WM_BASE, WM_BASE, -WM_BASE, WM_BASE))


def rr_windmill_fan():
    """The windpump's wheel: wooden sails, eighteen pitched slats on two board rings and six arms round an iron hub,
    built round its hub facing -Y (SM_WindmillFan's frame). It turns, so no Nanite, LODs and no collision."""
    f = homes.Home('RR_WindmillFan', seed=524)
    R_IN, R_OUT, SLATS = 0.55, 1.7, 18
    for k in range(SLATS):
        a = 2.0 * math.pi * k / SLATS
        blade = Matrix.Rotation(-a, 4, 'Y') @ Matrix.Rotation(math.radians(28.0), 4, 'X')
        widths = (0.14, 0.27)

        def taper(co, widths=widths):
            # Built at the rim's width and narrowed toward the hub (the mapping is laid on before the shape).
            t = co.x / (R_OUT - R_IN) + 0.5
            k = (widths[0] + (widths[1] - widths[0]) * t) / widths[1]
            return Vector((co.x + (R_OUT + R_IN) * 0.5, co.y, co.z * k))
        f.emit(kit._box((R_OUT - R_IN, 0.02, widths[1])), Trim('A', width=widths[1]), 'trim', blade, shape=taper)
    for radius, size in ((R_IN + 0.04, 0.06), (R_OUT - 0.06, 0.07), (1.12, 0.05)):
        for k in range(SLATS):
            a0, a1 = 2.0 * math.pi * k / SLATS, 2.0 * math.pi * (k + 1) / SLATS
            p0 = (radius * math.cos(a0), -0.04, radius * math.sin(a0))
            p1 = (radius * math.cos(a1), -0.04, radius * math.sin(a1))
            length = (Vector(p1) - Vector(p0)).length
            f.emit(kit._box((length + 0.012, size * 0.6, size)), Trim('C', width=size), 'trim',
                   kit.toward(p0, p1, (0.0, -1.0, 0.0)) @ Matrix.Translation((length * 0.5, 0.0, 0.0)))
    for k in range(6):
        a = 2.0 * math.pi * (k + 0.5) / 6
        p1 = (R_OUT * math.cos(a), -0.07, R_OUT * math.sin(a))
        length = Vector(p1).length
        f.emit(kit._box((length, 0.05, 0.08)), Trim('C', width=0.08), 'trim',
               kit.toward((0.0, -0.07, 0.0), p1, (0.0, -1.0, 0.0)) @ Matrix.Translation((length * 0.5, 0.0, 0.0)))
    iron = Tile('MetalWorn')
    f.cylinder((0.0, 0.08, 0.0), (0.0, -0.18, 0.0), 0.14, sides=10, uv=iron, mat='iron', face=(0.0, 0.0, 1.0))
    f.cylinder((0.0, -0.18, 0.0), (0.0, -0.3, 0.0), 0.1, 0.03, sides=10, uv=iron, mat='iron', face=(0.0, 0.0, 1.0),
               caps=(False, False))
    return f.finish(ground=False, Nanite=0, LODs='40,12', Collision='None')


BUILDERS = [('A', home_a), ('B', home_b), ('C', home_c), ('D', home_d), ('E', home_e), ('F', home_f), ('G', home_g),
            ('H', home_h), ('I', home_i), ('J', home_j), ('WELL', rr_well), ('OUTHOUSE', rr_outhouse),
            ('WINDMILL', rr_windmill), ('FAN', rr_windmill_fan)]
CAPTIONS = {
    'RR_Home_A': 'The saltbox (lived in): two storeys in front,\na long sweep of shakes down the back.',
    'RR_Home_B': 'The dogtrot (lived in): two hewn-log pens and\nan open breezeway under one long roof.',
    'RR_Home_C': 'The shotgun house (empty): tarred boards, nailed\nshut, its porch slack, its roof broken.',
    'RR_Home_D': 'The gallery house (lived in): two storeys of\noxide red, a porch below, a balcony above.',
    'RR_Home_E': "The dugout (lived in): a homesteader's room\nunder a mound of sod, canvas over the door.",
    'RR_Home_F': 'The stone house (empty): fieldstone and quoins,\nthe roof fallen in, the woodshed going.',
    'RR_Home_G': 'The claim cabin (empty): grey board-and-batten,\na board roof, a lean-to with the stove.',
    'RR_Home_H': 'The porch house (lived in): faded teal under a\npyramid of shakes, a porch round two sides.',
    'RR_Home_I': 'The L house (empty): a gabled wing to the street,\nits porch sagging, the roof broken in.',
    'RR_Home_J': 'The shack (empty): rough planks under a lapped\nplank roof, the stovepipe out the wall.',
    'RR_Well': "The sweep well: a hewn crib, a forked post, the\nsweep with its stone, the bucket; a trough.",
    'RR_Outhouse': "The privy: board-and-batten under a gable of\nshakes, a star cut in the door.",
    'RR_Windmill': "The windpump: a timber tower, wooden sails\n(RR_WindmillFan), a stave tank on a stand.",
}
# The renders' views where the default (front-left three-quarter, sun from the front-right) hides what makes the house:
# the saltbox's long back from its side, the dogtrot's breezeway from lower down, the stone house's woodshed side.
SHOTS = {
    'RR_Home_A': dict(view=(-1.0, -0.52, 0.25), sun=(-50.0, 18.0)),
    'RR_Home_B': dict(view=(-0.45, -1.0, 0.17), height_bias=0.38),
    'RR_Home_E': dict(view=(-0.55, -1.0, 0.32)),
    'RR_Home_F': dict(view=(0.7, -1.0, 0.34), sun=(-38.0, 16.0), figure='left'),
    'RR_Home_J': dict(view=(-0.7, -1.0, 0.5)),
    'RR_Well': dict(margin=1.1),
    'RR_Windmill': dict(margin=1.08),
}
ROWS = [['RR_Home_D', 'RR_Home_B', 'RR_Home_A', 'RR_Home_H', 'RR_Home_I'],
        ['RR_Home_C', 'RR_Home_E', 'RR_Home_F', 'RR_Home_G', 'RR_Home_J'],
        ['RR_Windmill', 'RR_Well', 'RR_Outhouse']]

models = [build() for key, build in BUILDERS if not ONLY or key in ONLY]

if lt.want_preview():
    by_name = {o.name: o for o in models}
    if 'RR_Windmill' in by_name and 'RR_WindmillFan' in by_name:
        homes.mount(by_name['RR_Windmill'], by_name['RR_WindmillFan'], 'Fan')     # on its socket, as the game hangs it
    if 'RR_WindmillFan' in by_name:
        by_name['RR_WindmillFan'].hide_render = True
    homes.previews([o for o in models if o.name != 'RR_WindmillFan'], CAPTIONS, sheet=not ONLY, shots_by_name=SHOTS,
                   rows=ROWS)
