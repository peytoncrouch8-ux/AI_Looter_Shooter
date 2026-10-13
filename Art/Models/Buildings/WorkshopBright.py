"""Bright & Daughter's coffin workshop over its cellar, behind the undertaker's front on Ransom's Rest
(Docs/Areas/RansomsRest_Expansion.md, sections 4 and 9: Knocking Below), and the cellar's doors.

  RR_Workshop_Bright  Thaddeus Bright's workshop, walked into from the undertaker's yard through its double doors
                      (standing open): weathered board and batten, black trim, a shake roof open inside to its boards and
                      tie beams. Inside, 9.7 x 6.7 m: two coffin benches (a coffin half made on one), lumber racks, his
                      tool wall, a box stove, a finished coffin with the brass handles Tilly put on it, the measuring hook
                      (his tape and the cellar key hang on it), the orders slate (four names chalked, a fifth struck
                      through in another hand: T. BRIGHT. PINE. NO BRASS.), a lamp; in the back corner a boarded stair
                      enclosure with the cellar door, a little bench beside it with his box on it.
                      Down a stone stair that turns back on itself (19 risers), the cellar: 12 x 8 m inside, six bays of
                      groin vaults 2.6 m high on two stone piers and the walls (one bay is the stairwell), coffin racks,
                      four coffins on trestles (lids shoved aside: they got out); through a door in its back wall the cold
                      room, 4 x 4 m under a barrel vault, Thaddeus's bench and an empty pair of trestles.
  RR_CellarDoor       a heavy plank door with iron straps (1.08 x 2.08 m), hung by the game on SOCKET_CellarDoor (the stair
                      enclosure's door, locked till the key's found) and SOCKET_ColdRoomDoor: its pivot is its hinge at the
                      floor, the leaf reaches -X from it, its front faces -Y; it hangs shut on the socket and swings open
                      toward the socket's front (+90 degrees about Z in Blender). Collision: one box. No Nanite.

The pivot follows the homes kit (looter_homes): the middle of the yard doors' opening on the front wall's outer face, at
the ground (z = 0); the workshop's floor is 0.30 over it, the cellar's floor 3.30 under it (the vaults' tops 0.40 under
the ground, the cold room's 0.60). The cellar reaches past the workshop's walls (under the yard in front of the doors,
under the ground at the west end and behind): the WORKSHOP: lines give every extent from the pivot, and the rectangle
where the terrain must open over the stairwell (or sink below -3.6 m).

Sockets: WorkshopDoor (outside the doors at the foot of the step), MeasuringHook, OrdersSlate, CellarDoor and ColdRoomDoor
(RR_CellarDoor's hinges), ThaddeusBox (his box on the bench by the cellar door), CellarFloor (the cellar's floor at the
stair's foot), Coffin_1..4 (the coffins on trestles), ThaddeusBench (where he stands at his bench, facing it), the lamps
Light_Workshop, Light_CellarStair, Light_Cellar_1, Light_Cellar_2, Light_ColdRoom, and Smoke.

A scripted model (Art/README.md) built with looter_rr_town (on looter_homes, looter_town, looter_buildings).
  --preview   renders it in the area's late-afternoon light, the workshop roof off, the cellar from above with everything
              over it cut away, views inside, into Saved/ArtPreviews/RansomsRest/Town/, and writes WorkshopBright_report.json
  --stats     prints where the triangles go; --no-ao skips the AO bake
"""
import math

import looter_buildings as kit
import looter_homes as homes
import looter_rr_town as rr
import looter_textures as lt
import looter_town as town
from looter_buildings import Matrix, Opening, Space, Tile, Trim, Vector

BASE = 0.30                                  # the workshop's floor over the yard at its doors
X0, X1, Y0, Y1 = -5.0, 5.0, -3.5, 3.5        # the workshop's walls' outer faces
T = 0.15
XI0, XI1, YI0, YI1 = X0 + T, X1 - T, Y0 + T, Y1 - T
WALL_H = 3.0
EAVE = BASE + WALL_H
PITCH = 35.0
DOOR = (-3.8, -2.2)                          # the yard doors (x)
# The cellar: 12 x 8 m inside, three bays along x by two along y, 4 m each; its floor DEPTH under the ground.
DEPTH = 3.3
CF = -DEPTH                                  # the cellar's floor
ZS, ZC = CF + 1.5, CF + 2.6                  # the vaults' springing and crown
CX = (-7.15, -3.15, 0.85, 4.85)              # bay lines along x (the walls' inner faces and the piers' middles)
CY = (-4.65, -0.65, 3.35)                    # along y
WALL_C = 0.4                                 # the cellar's stone walls
HOLE = (2.6, XI1, -0.65, YI1)                # the stairwell's hole in the workshop floor
ENC = 0.12                                   # the enclosure's board walls
EH = 2.4                                     # their height
CELLAR_DOOR = (3.7, 4.8)                     # the enclosure's door (x), in its front wall
COLD = (-7.15, -3.15, 3.75, 7.75)            # the cold room inside
COLD_DOOR = (-5.7, -4.6)
COLD_CROWN = CF + 2.4
R_RISE, R_RUN = (BASE - CF) / 19, 0.26
F1 = (3.81, XI1)                             # flight 1 (the workshop down to the half landing) across x
F2 = (2.6, 3.69)                             # flight 2 (the landing down to the cellar)
SPINE = (3.69, 3.81)
LAND_Y = CY[1] + 9 * R_RUN                   # the landing's front edge (1.69)
LAND_Z = BASE - 10 * R_RISE                  # the landing's height
FOOT2 = LAND_Y - 8 * R_RUN                   # flight 2's foot (-0.39)
PIVOT = ((DOOR[0] + DOOR[1]) * 0.5, Y0)
STONE = Tile('StoneWall', along=None)


def arch_z(t, half, rise, zs):
    """A segmental arch's height t from its middle (half its span, its rise over zs)."""
    radius = (half * half + rise * rise) / (2.0 * rise)
    return zs + rise - radius + math.sqrt(max(radius * radius - min(t * t, half * half), 0.0))


def arch_len(t, half, rise):
    """The length along that arch from its middle to t (signed)."""
    radius = (half * half + rise * rise) / (2.0 * rise)
    return radius * math.asin(max(-1.0, min(1.0, t / radius)))


def vault_z(t, half=2.0):
    return arch_z(t, half, ZC - ZS, ZS)


def nose1(y):
    """Flight 1's nose line over y (it climbs toward -y from the landing to the workshop floor)."""
    return LAND_Z + (LAND_Y - y) / R_RUN * R_RISE + R_RISE


# --- The cellar's vaults and walls ---

def groin_bay(m, x0, x1, y0, y1, n=8):
    """A groin vault over a bay: two barrels crossing, the higher of the two overhead, cut into the four sectors between
    its diagonals; each sector a strip of one barrel, its stones mapped unrolled round it."""
    hx, hy = (x1 - x0) * 0.5, (y1 - y0) * 0.5
    xc, yc = (x0 + x1) * 0.5, (y0 + y1) * 0.5
    rise = ZC - ZS
    for side in ('y0', 'y1', 'x0', 'x1'):
        tb = kit._new_bmesh()
        verts = {}

        def vert(p):
            key = (round(p[0], 4), round(p[1], 4), round(p[2], 4))
            if key not in verts:
                verts[key] = tb.verts.new(p)
            return verts[key]
        rows = []
        for i in range(n + 1):
            if side in ('y0', 'y1'):
                x = x0 + (x1 - x0) * i / n
                reach = hy * (1.0 - abs(x - xc) / hx)
                z = arch_z(x - xc, hx, rise, ZS)
                ya = y0 if side == 'y0' else y1
                yb = ya + (reach if side == 'y0' else -reach)
                rows.append([vert((x, ya + (yb - ya) * k / 2.0, z)) for k in range(3)])
            else:
                y = y0 + (y1 - y0) * i / n
                reach = hx * (1.0 - abs(y - yc) / hy)
                z = arch_z(y - yc, hy, rise, ZS)
                xa = x0 if side == 'x0' else x1
                xb = xa + (reach if side == 'x0' else -reach)
                rows.append([vert((xa + (xb - xa) * k / 2.0, y, z)) for k in range(3)])
        for i in range(n):
            for k in range(2):
                quad = list(dict.fromkeys([rows[i][k], rows[i + 1][k], rows[i + 1][k + 1], rows[i][k + 1]]))
                if len(quad) < 3:
                    continue
                try:
                    f = tb.faces.new(quad)
                except ValueError:
                    continue
                f.normal_update()
                if f.normal.z > 0.0:
                    f.normal_flip()
        if side in ('y0', 'y1'):
            fn = (lambda co, xc=xc, hx=hx, rise=rise: (co.y, arch_len(co.x - xc, hx, rise)))
        else:
            fn = (lambda co, yc=yc, hy=hy, rise=rise: (co.x, arch_len(co.y - yc, hy, rise)))
        m.emit(tb, rr.Unrolled('StoneWall', fn), 'stone', smooth=True)


def rib(m, p0, p1, half, width=0.3, depth=0.09, n=10, ring=False):
    """A stone rib under the vaults along a bay line from p0 to p1 (plan points; its arch over that span): its underside
    depth below the vaults and its two sides (ring: the ring of an arch into the stairwell, whose side shows over it)."""
    p0, p1 = Vector((p0[0], p0[1], 0.0)), Vector((p1[0], p1[1], 0.0))
    along = (p1 - p0).normalized()
    across = Vector((-along.y, along.x, 0.0)) * (width * 0.5)
    span = (p1 - p0).length
    mid = p0 + along * span * 0.5
    tb = kit._new_bmesh()
    rise = ZC - ZS
    pts = [(p0 + along * (span * i / n), arch_z(span * i / n - span * 0.5, half, rise, ZS)) for i in range(n + 1)]
    lo = [[tb.verts.new(c + across * s + Vector((0.0, 0.0, z - depth))) for s in (-1.0, 1.0)] for c, z in pts]
    hi = [[tb.verts.new(c + across * s + Vector((0.0, 0.0, z + (0.45 if ring else 0.0)))) for s in (-1.0, 1.0)]
          for c, z in pts]
    for i in range(n):
        for quad in ((lo[i][0], lo[i + 1][0], lo[i + 1][1], lo[i][1]), (lo[i][0], hi[i][0], hi[i + 1][0], lo[i + 1][0]),
                     (lo[i][1], lo[i + 1][1], hi[i + 1][1], hi[i][1])):
            tb.faces.new(quad)
    for f in tb.faces:
        f.normal_update()
        c = f.calc_center_median()
        out = Vector((c.x - mid.x, c.y - mid.y, 0.0))
        lateral = out - along * out.dot(along)
        if abs(f.normal.z) > 0.7:
            if f.normal.z > 0.0:
                f.normal_flip()
        elif f.normal.dot(lateral) < 0.0:
            f.normal_flip()
    m.emit(tb, STONE, 'stone')


def lunette_wall(m, along_axis, fixed, spans, facing, holes=()):
    """A cellar wall's inside face (one-sided, toward facing) along x (at y = fixed) or along y (at x = fixed): its
    bays' spans [(a, b, kind)] in order, kind 'arch' (a lunette up to the vault's arch over it) or a height (a flat top),
    door holes [(a, b, top)] notched into its foot."""
    pts = [(spans[0][0], CF)]
    for a, b, kind in spans:
        if kind == 'arch':
            pts += [(a + (b - a) * i / 8, vault_z(a + (b - a) * i / 8 - (a + b) * 0.5, (b - a) * 0.5)) for i in range(9)]
        else:
            pts += [(a, kind), (b, kind)]
    pts.append((spans[-1][1], CF))
    for a, b, top in sorted(holes, key=lambda h: -h[0]):
        pts += [(b, CF), (b, top), (a, top), (a, CF)]
    clean = []
    for p in pts:
        if not clean or abs(p[0] - clean[-1][0]) > 1e-4 or abs(p[1] - clean[-1][1]) > 1e-4:
            clean.append(p)
    if abs(clean[0][0] - clean[-1][0]) < 1e-4 and abs(clean[0][1] - clean[-1][1]) < 1e-4:
        clean.pop()
    world = [(s, fixed, z) for s, z in clean] if along_axis == 'x' else [(fixed, s, z) for s, z in clean]
    rr.polygon(m, world, STONE, 'stone', facing=facing)


def pier(m, x, y, size=0.6):
    """A square stone pier up to the springing, a plain impost block on it."""
    kit.stone_stack(m, (x, y, CF), (size, size), ZS - CF - 0.12, taper=0.0, bevel=0.0, uv='D')
    m.box((size + 0.1, size + 0.1, 0.12), at=(x, y, ZS - 0.06), uv=Trim('D', world=True))
    m.hull((size, size, ZS - CF), at=(x, y, (ZS + CF) * 0.5))


def rack(m, frame, length=2.3, levels=(0.06, 0.72), coffins=2, coffin_len=1.95):
    """A coffin rack (its middle at frame's origin on the floor, along x): four posts, two shelves of rails, coffins."""
    for x in (-length * 0.5 + 0.05, length * 0.5 - 0.05):
        for y in (-0.33, 0.33):
            town.box(m, (0.08, 0.08, 1.38), at=(x, y, 0.69), look='planks', space=frame, drop=('-z',))
    for z in levels:
        for y in (-0.3, 0.3):
            town.board(m, (-length * 0.5, y, z + 0.05), (length * 0.5, y, z + 0.05), 0.1, 0.06, look='C', space=frame,
                       face=(0.0, -1.0, 0.0))
    small = coffin_len < 1.5
    for k in range(coffins):
        start = -coffin_len * 0.5
        coffin_frame = Space(frame.matrix @ Matrix.Translation((start, 0.0, levels[k] + 0.1)))
        rr.coffin(m, coffin_frame, length=coffin_len, foot=0.24 if small else 0.34, shoulder=0.4 if small else 0.6,
                  head=0.3 if small else 0.44, depth=0.28 if small else 0.36, lid='on',
                  look='woodcream' if k else 'planks')
    m.hull((length, 0.74, 1.4), at=(0.0, 0.0, 0.7), space=frame)


# --- The workshop ---

def workshop():
    m = rr.Building('RR_Workshop_Bright', seed=811)
    rng = m.rng
    tan = math.tan(math.radians(PITCH))
    rise = (Y1 - Y0) * 0.5 * tan

    # Outer walls: weathered board and batten on a fieldstone plinth, black trim; windows with their lower sashes up.
    dw = dict(casing_w=0.11, panes=(2, 2))
    F_DOOR = Opening('door', DOOR[0] - X0, 0.0, DOOR[1] - DOOR[0], 2.3)
    W_F1 = Opening('window', 4.6, 1.0, 0.9, 1.3, **dw)           # x -0.4 .. 0.5
    W_F2 = Opening('window', 6.7, 1.0, 0.9, 1.3, **dw)           # x 1.7 .. 2.6
    W_B1 = Opening('window', X1 + 0.3, 1.0, 0.9, 1.3, **dw)      # along the back wall from its east end: x -1.2 .. -0.3
    sign_x = F_DOOR.x + F_DOOR.w * 0.5
    walls = [((X0, Y0), (X1, Y0), [F_DOOR, W_F1, W_F2], False, [(sign_x - 1.06, sign_x + 1.06, 2.5, 3.0)]),
             ((X1, Y0), (X1, Y1), [], True, []), ((X1, Y1), (X0, Y1), [W_B1], False, []),
             ((X0, Y1), (X0, Y0), [], True, [])]
    spaces = []
    for p0, p1, items, gable, extra in walls:
        space = kit.wall_space(p0, p1, BASE)
        length = (Vector(p1) - Vector(p0)).length
        outline = [(0.0, 0.0), (length, 0.0), (length, WALL_H)]
        if gable:
            outline.append((length * 0.5, WALL_H + rise - 0.02))
        outline.append((0.0, WALL_H))
        notched = [outline[0]]
        for o in sorted((o for o in items if o.kind == 'door'), key=lambda o: o.x):
            notched += [(o.x, 0.0), (o.x, o.h), (o.x + o.w, o.h), (o.x + o.w, 0.0)]
        notched += outline[1:]
        homes.shell(m, notched, kit.holes(items), T, Trim('A', world=True, rotate=True, u=rng.uniform(0.0, 6.4)), space,
                    around=items)
        edges = [(-0.1, 0.16, -1.0, 30.0), (length - 0.16, length + 0.1, -1.0, 30.0)]
        homes.battens(m, space, outline, 0.0, length, homes.keep_out(items) + edges + extra, look='A', every=0.7,
                      z0=0.21)
        homes.corners(m, space, length, WALL_H - 0.02, WALL_H - 0.02, look='woodblack', width=0.14, wrap=not gable)
        homes.band(m, space, homes.door_spans(items, length), 0.1, 0.2, look='woodblack')
        spaces.append(space)
    homes.plinth(m, X0, X1, Y0, Y1, BASE)
    front, back = spaces[0], spaces[2]
    for space, o in ((front, W_F1), (front, W_F2), (back, W_B1)):
        rr.room_window(m, space, o, T, paint='woodblack', sash='woodblack', open_lower=True)
    for p0, p1, o in (((X0, Y0), (X1, Y0), W_F1), ((X0, Y0), (X1, Y0), W_F2), ((X1, Y1), (X0, Y1), W_B1)):
        rr.window_inside(m, p0, p1, BASE, T, o, look='C')
    m.section('outer walls, windows')

    # The yard doors: a black double plank door, both leaves standing open inward; a name board over it; a plank step
    # down to the yard.
    rr.casing(m, front, F_DOOR, 'woodblack', cw=0.14, sill=False)
    rr.window_inside(m, (X0, Y0), (X1, Y0), BASE, T, F_DOOR, look='C', stool=False)
    rr.reveal_floor(m, front, F_DOOR, T)
    half = (DOOR[1] - DOOR[0]) * 0.5
    rr.plank_leaf(m, rr.hinged((DOOR[0] + 0.03, YI0 + 0.06, BASE), 172.0), half - 0.01, 2.27, look='woodblack',
                  battens_on=1.0)
    rr.plank_leaf(m, rr.hinged((DOOR[1] - 0.03, YI0 + 0.06, BASE), -172.0), half - 0.01, 2.27, look='woodblack',
                  battens_on=1.0, hinge_left=False)
    for z in (2.665, 2.805):
        town.board(m, (sign_x - 1.0, -0.035, z), (sign_x + 1.0, -0.035, z), 0.13, 0.03, look='woodblack', space=front)
    town.text(m, 'BRIGHT & DAUGHTER', (sign_x, -0.052, 2.735), height=0.13, width=1.8, space=front, look='cream',
              tol=0.04)
    homes.steps(m, PIVOT[0], F_DOOR.w + 0.5, Y0, BASE, 1, toward=-1.0, run=0.32)
    m.socket('WorkshopDoor', (PIVOT[0], Y0 - 0.75, 0.0), (0.0, 0.0, 0.0))
    m.section('yard doors')

    # The roof: shakes, open inside to its boards; three tie beams across and a ridge board; the stove's pipe.
    front_roof, back_roof = town.gable(m, X0, X1, Y0, Y1, EAVE, PITCH, overhang=0.35, rake=0.3, deck=0.08, sag=0.05)
    PIPE = (0.55, 2.2)
    for slope in (front_roof, back_roof):
        town.shakes(m, slope, piece=(3.4, 5.0), skip=lambda x, y, s=slope: (
            abs(s.world((x, y, 0.0)).x - PIPE[0]) < 0.2 and abs(s.world((x, y, 0.0)).y - PIPE[1]) < 0.2))
    town.ridge_cap(m, front_roof, back_roof, uv='C', width=0.16, thick=0.05)
    BEAMS = (-3.3, -1.3, 1.5)
    for x in BEAMS:
        town.board(m, (x, YI0, EAVE - 0.12), (x, YI1, EAVE - 0.12), 0.2, 0.14, look='C', face=(0.0, 0.0, -1.0))
    town.board(m, (XI0, 0.0, EAVE + 3.5 * tan - 0.22), (XI1, 0.0, EAVE + 3.5 * tan - 0.22), 0.2, 0.08, look='C',
               face=(0.0, -1.0, 0.0))
    roof_at = EAVE + (Y1 - PIPE[1]) * tan
    rr.chimney_pipe(m, PIPE[0], PIPE[1], BASE + 1.6, roof_at + 0.9)
    m.section('roof')

    # The room's walls inside: rough boards; the gables' insides over the plate; the floor of planks round the hole.
    boards = Trim('A', world=True, rotate=True)
    rr.wall_cells(m, (XI1, YI0), (XI0, YI0), BASE, 0.0, WALL_H, [
        (XI1 - DOOR[1], XI1 - DOOR[0], 0.0, 2.3), (XI1 - 2.6, XI1 - 1.7, 1.0, 2.3), (XI1 - 0.5, XI1 + 0.4, 1.0, 2.3)],
        boards)
    rr.wall_cells(m, (XI0, YI0), (XI0, YI1), BASE, 0.0, WALL_H, [], boards)
    rr.wall_cells(m, (XI0, YI1), (XI1, YI1), BASE, 0.0, WALL_H, [(-1.2 - XI0, -0.3 - XI0, 1.0, 2.3)], boards)
    rr.wall_cells(m, (XI1, YI1), (XI1, YI0), BASE, 0.0, WALL_H, [], boards)
    for x, face in ((XI0, (1.0, 0.0, 0.0)), (XI1, (-1.0, 0.0, 0.0))):
        rr.polygon(m, [(x, YI0, EAVE), (x, YI1, EAVE), (x, 0.0, EAVE + (Y1 - Y0) * 0.5 * tan - 0.1)], boards, facing=face)
    rr.flat_cells(m, XI0, XI1, YI0, YI1, BASE, holes=[HOLE])
    m.section('room')

    # The stair enclosure: board walls round the hole (both faces), the cellar door in its front wall (its leaf is
    # RR_CellarDoor, hung on SOCKET_CellarDoor); a little bench beside it with his box on it.
    ex0, ey0 = HOLE[0] - ENC, HOLE[2] - ENC
    planks = homes.Boards(rotate=True)
    rr.wall_cells(m, (ex0, YI1), (ex0, ey0), BASE, 0.0, EH, [], planks, 'planks')
    rr.wall_cells(m, (HOLE[0], HOLE[2]), (HOLE[0], YI1), BASE, -0.25, EH, [], planks, 'planks')
    rr.wall_cells(m, (ex0, ey0), (XI1, ey0), BASE, 0.0, EH, [(CELLAR_DOOR[0] - ex0, CELLAR_DOOR[1] - ex0, 0.0, 2.1)],
                  planks, 'planks')
    rr.wall_cells(m, (XI1, HOLE[2]), (HOLE[0], HOLE[2]), BASE, -0.25, EH,
                  [(XI1 - CELLAR_DOOR[1], XI1 - CELLAR_DOOR[0], -0.3, 2.1)], planks, 'planks')
    town.box(m, (ENC, YI1 - ey0, 0.03), at=(ex0 + ENC * 0.5, (ey0 + YI1) * 0.5, BASE + EH + 0.015), look='C')
    town.box(m, (XI1 - HOLE[0], ENC, 0.03), at=((HOLE[0] + XI1) * 0.5, ey0 + ENC * 0.5, BASE + EH + 0.015), look='C')
    cdoor = Opening('door', CELLAR_DOOR[0] - ex0, 0.0, CELLAR_DOOR[1] - CELLAR_DOOR[0], 2.1)
    rr.doorway_trim(m, (ex0, ey0), (XI1, ey0), BASE, ENC, cdoor, 'woodblack', threshold=False)
    rr.lining(m, (CELLAR_DOOR[0], ey0), (CELLAR_DOOR[1], ey0), (0.0, 1.0), ENC, BASE, BASE + 2.1, look='C')
    m.socket('CellarDoor', (CELLAR_DOOR[1] - 0.01, ey0 + ENC * 0.5, BASE), (0.0, 0.0, 0.0))
    town.box(m, (0.9, 0.36, 0.05), at=(3.1, ey0 - 0.24, BASE + 0.45), look='A')
    for x in (2.75, 3.45):
        town.box(m, (0.06, 0.3, 0.42), at=(x, ey0 - 0.24, BASE + 0.21), look='C', drop=('-z',))
    town.box(m, (0.46, 0.28, 0.2), at=(3.12, ey0 - 0.25, BASE + 0.575), rot=(0.0, 0.0, 4.0), look='woodblack')
    town.box(m, (0.06, 0.02, 0.04), at=(3.12, ey0 - 0.395, BASE + 0.6), look='brass')
    m.socket('ThaddeusBox', (3.12, ey0 - 0.25, BASE + 0.675), rr.rot_to((0.0, -1.0)))
    m.hull((0.95, 0.4, 0.68), at=(3.1, ey0 - 0.24, BASE + 0.34))
    m.section('stair enclosure')

    # --- The workshop's things ---
    # Two coffin benches: a coffin half made on one (sides and floor, no lid), boards, a plane and a saw on the other.
    for bx, by in ((-1.55, 0.95), (-1.55, -1.15)):
        town.box(m, (2.5, 0.85, 0.06), at=(bx, by, BASE + 0.78), look='planks')
        for sx in (-1.0, 1.0):
            town.box(m, (0.08, 0.75, 0.75), at=(bx + sx * 1.05, by, BASE + 0.375), look='C', drop=('-z',))
        town.box(m, (2.1, 0.06, 0.08), at=(bx, by, BASE + 0.2), look='C')
        m.hull((2.5, 0.85, 0.81), at=(bx, by, BASE + 0.405))
    rr.coffin(m, Space(Matrix.Translation((-2.55, 0.95, BASE + 0.81))), lid='none', look='woodcream')
    for k in range(4):
        town.board(m, (-2.6, -1.3 + 0.105 * k, BASE + 0.835 + 0.025 * k), (-0.5, -1.3 + 0.105 * k,
                   BASE + 0.835 + 0.025 * k), 0.2, 0.025, look='planks', face=(0.0, 0.0, 1.0))
    town.box(m, (0.26, 0.07, 0.08), at=(-0.75, -0.85, BASE + 0.85), rot=(0.0, 0.0, 20.0), look='C')
    town.prism(m, [(-0.6, 0.0), (0.0, 0.0), (0.0, 0.14), (-0.55, 0.05)], 0.003, look='iron',
               matrix=Matrix.Translation((-0.8, -1.5, BASE + 0.811)) @ Matrix.Rotation(math.radians(-90.0), 4, 'X'))
    # A finished coffin, its brass handles on, on trestles under the front windows.
    for x in (0.3, 1.9):
        rr.trestle(m, Space(Matrix.Translation((x, -2.5, BASE))))
    rr.coffin(m, Space(Matrix.Translation((0.12, -2.5, BASE + 0.62))), lid='on', look='woodcream', brass=True,
              lid_look='woodcream')
    m.hull((2.1, 0.72, 1.06), at=(1.1, -2.5, BASE + 0.53))
    # The lumber racks along the west wall: iron brackets, pine boards stacked on them.
    for z in (0.55, 1.15, 1.75):
        for y in (-2.6, -0.8, 1.0, 2.7):
            town.board(m, (XI0 + 0.005, y, BASE + z - 0.06), (XI0 + 0.5, y, BASE + z - 0.06), 0.05, 0.03, look='iron',
                       face=(0.0, -1.0, 0.0))
        for k in range(3 if z < 1.5 else 2):
            town.board(m, (XI0 + 0.1 + 0.13 * k, -3.0, BASE + z - 0.025 + 0.002 * k), (XI0 + 0.1 + 0.13 * k, 3.1,
                       BASE + z - 0.025), 0.12, 0.03, look='planks', face=(0.0, 0.0, 1.0), cuts=2)
    m.hull((0.5, 6.2, 1.8), at=(XI0 + 0.25, 0.05, BASE + 0.9))
    # His tool wall on the back wall: a board with saws, squares, a mallet, chisels and a brace hung on it.
    tw = kit.wall_space((-4.6, YI1), (-1.5, YI1), BASE)
    town.box(m, (3.1, 0.02, 1.0), at=(1.55, -0.01, 1.55), look='planks', space=tw, drop=('+y',))
    for x, kind in ((0.35, 'saw'), (0.85, 'saw'), (1.3, 'square'), (1.7, 'mallet'), (2.05, 'chisels'), (2.5, 'brace'),
                    (2.85, 'square')):
        if kind == 'saw':
            town.prism(m, [(x - 0.12, 1.15), (x + 0.12, 1.15), (x + 0.08, 1.9), (x - 0.04, 1.9)], 0.004, look='iron',
                       space=tw, y=-0.03)
            town.box(m, (0.1, 0.03, 0.14), at=(x + 0.02, -0.035, 1.92), look='C', space=tw)
        elif kind == 'square':
            town.prism(m, [(x - 0.02, 1.2), (x + 0.02, 1.2), (x + 0.02, 1.76), (x + 0.32, 1.76), (x + 0.32, 1.8),
                           (x - 0.02, 1.8)], 0.006, look='C', space=tw, y=-0.03)
        elif kind == 'mallet':
            town.box(m, (0.04, 0.04, 0.32), at=(x, -0.04, 1.5), look='C', space=tw)
            town.box(m, (0.14, 0.09, 0.1), at=(x, -0.05, 1.7), look='C', space=tw)
        elif kind == 'chisels':
            for c in range(4):
                town.box(m, (0.025, 0.02, 0.24), at=(x + 0.06 * c, -0.035, 1.45 + 0.02 * (c % 2)), look='C', space=tw)
            town.box(m, (0.24, 0.04, 0.03), at=(x + 0.09, -0.035, 1.32), look='A', space=tw)
        else:
            town.box(m, (0.02, 0.02, 0.4), at=(x, -0.04, 1.6), look='iron', space=tw)
            town.box(m, (0.22, 0.02, 0.02), at=(x + 0.1, -0.04, 1.6), look='iron', space=tw)
    # The box stove by the back wall, its pipe up through the roof.
    sx0, sy0 = PIPE
    town.box(m, (0.62, 0.42, 0.42), at=(sx0, sy0, BASE + 0.48), look='iron')
    for dx in (-0.26, 0.26):
        for dy in (-0.16, 0.16):
            town.box(m, (0.04, 0.04, 0.27), at=(sx0 + dx, sy0 + dy, BASE + 0.135), look='iron', drop=('-z',))
    town.box(m, (0.2, 0.03, 0.18), at=(sx0, sy0 - 0.225, BASE + 0.5), look='iron')
    town.cylinder(m, (sx0, sy0, BASE + 0.69), (sx0, sy0, BASE + 1.6), 0.07, sides=8, look='iron', caps=(False, False))
    m.hull((0.66, 0.46, 0.72), at=(sx0, sy0, BASE + 0.36))
    # By the cellar door, on the east wall: the measuring hook (his tape coiled on it, the cellar key), the orders slate.
    rw = kit.wall_space((XI1, ey0), (XI1, YI0), BASE)          # the east wall's inside in the room: s = ey0 - y
    hook_s, hook_z = 0.5, 1.62
    town.box(m, (0.08, 0.03, 0.12), at=(hook_s, -0.015, hook_z), look='C', space=rw)
    town.board(m, (hook_s, -0.04, hook_z + 0.02), (hook_s, -0.1, hook_z - 0.02), 0.012, 0.012, look='brass', space=rw,
               face=(1.0, 0.0, 0.0))
    town.cylinder(m, (hook_s, -0.075, hook_z - 0.16), (hook_s, -0.115, hook_z - 0.16), 0.075, sides=10, look='brass',
                  space=rw)
    town.board(m, (hook_s + 0.02, -0.07, hook_z - 0.2), (hook_s + 0.05, -0.07, hook_z - 0.55), 0.016, 0.003,
               look='band_G', space=rw)
    town.box(m, (0.014, 0.006, 0.11), at=(hook_s - 0.03, -0.11, hook_z - 0.08), rot=(0.0, 12.0, 0.0), look='iron',
             space=rw)
    town.lathe(m, [(0.0, 0.0), (0.022, 0.0), (0.022, 0.006), (0.0, 0.006)], at=(hook_s - 0.04, -0.11, hook_z),
               sides=6, look='iron', space=rw, axis=Matrix.Rotation(math.radians(90.0), 4, 'X'))
    m.socket('MeasuringHook', rw.world((hook_s, -0.08, hook_z - 0.05)), rr.rot_to((-1.0, 0.0)))
    s0, s1, z0, z1 = 1.0, 2.0, 1.15, 1.85
    town.box(m, (s1 - s0, 0.02, z1 - z0), at=((s0 + s1) * 0.5, -0.01, (z0 + z1) * 0.5), look='black', space=rw,
             drop=('+y',))
    for z in (z0 - 0.02, z1 + 0.02):
        town.board(m, (s0 - 0.04, -0.02, z), (s1 + 0.04, -0.02, z), 0.04, 0.03, look='C', space=rw)
    for s in (s0 - 0.02, s1 + 0.02):
        town.board(m, (s, -0.02, z0), (s, -0.02, z1), 0.04, 0.03, look='C', space=rw)
    lines = ['ORDERS', 'DOLAN  PINE  6FT', 'AVERY  PINE  5FT 5', 'PIKE  PINE  5FT 10', 'KESSLER  OAK  5FT 2']
    rr.chalk(m, rw, lines, (s0 + 0.06, -0.0215, z1 - 0.07), height=0.045, tol=0.06, gap=1.75)
    last = z1 - 0.07 - 5 * 0.045 * 1.75
    widths = rr.chalk(m, rw, ['T. BRIGHT.  PINE.  NO BRASS.'], (s0 + 0.06, -0.0215, last), height=0.042, tol=0.06,
                      rot_jitter=4.0)
    rr.strike(m, rw, s0 + 0.04, s0 + 0.08 + widths[0], last, -0.022)
    m.socket('OrdersSlate', rw.world(((s0 + s1) * 0.5, -0.05, (z0 + z1) * 0.5)), rr.rot_to((-1.0, 0.0)))
    rr.lamp_hanging(m, (BEAMS[1], -0.1, BASE + 2.3), EAVE - 0.22, socket='Light_Workshop')
    m.section('workshop things')

    # --- The stair down: flight 1 from the cellar door to the half landing, flight 2 back down to the cellar's floor
    # beside it; a stone spine wall between them (flight 1's railing), a stone parapet along flight 2 and the landing. ---
    rr.flight(m, ((F1[0] + F1[1]) * 0.5, LAND_Y), (0.0, -1.0), F1[1] - F1[0], 10, R_RISE, R_RUN, LAND_Z, look='A',
              top_tread=False, stringers=(False, False))
    rr.flight(m, ((F2[0] + F2[1]) * 0.5, FOOT2), (0.0, 1.0), F2[1] - F2[0], 9, R_RISE, R_RUN, CF, look='A',
              top_tread=False, stringers=(False, False))
    rr.flat_cells(m, F2[0], XI1, LAND_Y, YI1, LAND_Z, every=0)
    spine_top = [(HOLE[2] + 0.05, nose1(HOLE[2] + 0.05) + 0.9), (LAND_Y, nose1(LAND_Y) + 0.9)]
    west = [(HOLE[2] + 0.05, CF), (FOOT2, CF), (LAND_Y, LAND_Z - 0.04)] + list(reversed(spine_top))
    rr.polygon(m, [(SPINE[0], y, z) for y, z in west], STONE, 'stone', facing=(-1.0, 0.0, 0.0))
    east = [(HOLE[2] + 0.05, nose1(HOLE[2] + 0.05) - 0.04), (LAND_Y, nose1(LAND_Y) - 0.04)] + list(reversed(spine_top))
    rr.polygon(m, [(SPINE[1], y, z) for y, z in east], STONE, 'stone', facing=(1.0, 0.0, 0.0))
    (ya, za), (yb, zb) = spine_top
    town.board(m, ((SPINE[0] + SPINE[1]) * 0.5, ya, za + 0.03), ((SPINE[0] + SPINE[1]) * 0.5, yb, zb + 0.03), 0.18,
               0.06, look='D', face=(0.0, 0.0, 1.0))
    par = F2[0]
    ptop = [(FOOT2 - 0.3, CF + 0.9), (LAND_Y, LAND_Z + 0.9), (YI1, LAND_Z + 0.9)]
    outside = [(FOOT2 - 0.3, CF), (YI1, CF)] + list(reversed(ptop))
    rr.polygon(m, [(par - ENC, y, z) for y, z in outside], STONE, 'stone', facing=(-1.0, 0.0, 0.0))
    inside = [(FOOT2 - 0.3, CF), (FOOT2, CF), (LAND_Y, LAND_Z - 0.04), (YI1, LAND_Z - 0.04)] + list(reversed(ptop))
    rr.polygon(m, [(par, y, z) for y, z in inside], STONE, 'stone', facing=(1.0, 0.0, 0.0))
    for (ya, za), (yb, zb) in zip(ptop, ptop[1:]):
        town.board(m, (par - ENC * 0.5, ya, za + 0.03), (par - ENC * 0.5, yb, zb + 0.03), 0.2, 0.06, look='D',
                   face=(0.0, 0.0, 1.0))
    m.section('stair')

    # --- The cellar: groin vaults over five bays (the sixth is the stairwell), ribs between them, two piers, the walls'
    # lunettes; the stairwell's sides over its arches up to the workshop floor, its ceiling the floor's joists. ---
    for (x0, x1), (y0, y1) in (((CX[0], CX[1]), (CY[0], CY[1])), ((CX[1], CX[2]), (CY[0], CY[1])),
                               ((CX[2], CX[3]), (CY[0], CY[1])), ((CX[0], CX[1]), (CY[1], CY[2])),
                               ((CX[1], CX[2]), (CY[1], CY[2]))):
        groin_bay(m, x0, x1, y0, y1)
    for p0, p1, ring in (((CX[1], CY[0]), (CX[1], CY[1]), False), ((CX[1], CY[1]), (CX[1], CY[2]), False),
                         ((CX[2], CY[0]), (CX[2], CY[1]), False), ((CX[0], CY[1]), (CX[1], CY[1]), False),
                         ((CX[1], CY[1]), (CX[2], CY[1]), False), ((CX[2], CY[1]), (CX[2], CY[2]), True),
                         ((CX[2], CY[1]), (CX[3], CY[1]), True)):
        rib(m, p0, p1, 2.0, ring=ring)
    for x, y in ((CX[1], CY[1]), (CX[2], CY[1])):
        pier(m, x, y)
    rr.flat_cells(m, CX[0], CX[3], CY[0], CY[2], CF, uv=Tile('GroundDirt', along=None), mat='dirt', every=2.0)
    lunette_wall(m, 'y', CX[0], [(CY[0], CY[1], 'arch'), (CY[1], CY[2], 'arch')], (1.0, 0.0, 0.0))
    lunette_wall(m, 'x', CY[0], [(CX[0], CX[1], 'arch'), (CX[1], CX[2], 'arch'), (CX[2], CX[3], 'arch')],
                 (0.0, 1.0, 0.0))
    lunette_wall(m, 'y', CX[3], [(CY[0], CY[1], 'arch'), (CY[1], CY[2], BASE)], (-1.0, 0.0, 0.0))
    lunette_wall(m, 'x', CY[2], [(CX[0], CX[1], 'arch'), (CX[1], CX[2], 'arch'), (CX[2], CX[3], BASE)],
                 (0.0, -1.0, 0.0), holes=[(COLD_DOOR[0], COLD_DOOR[1], CF + 2.1)])
    ceil = BASE - 0.25
    for along, fixed, a, b, face in (('y', CX[2], CY[1], CY[2], (1.0, 0.0, 0.0)),
                                     ('x', CY[1], CX[2], CX[3], (0.0, 1.0, 0.0))):
        curve = [(a + (b - a) * i / 8, vault_z(a + (b - a) * i / 8 - (a + b) * 0.5) + 0.45) for i in range(9)]
        pts = curve + [(b, ceil), (a, ceil)]
        world = [(fixed, s, z) for s, z in pts] if along == 'y' else [(s, fixed, z) for s, z in pts]
        rr.polygon(m, world, STONE, 'stone', facing=face)
    rr.flat_cells(m, CX[2], HOLE[0] - ENC, CY[1], CY[2], ceil + 0.2, up=False, every=0)
    for x in kit.frange(CX[2] + 0.3, HOLE[0] - 0.2, 0.6):
        town.board(m, (x, CY[1], ceil + 0.1), (x, CY[2], ceil + 0.1), 0.2, 0.08, look='C', face=(1.0, 0.0, 0.0))
    m.section('cellar')

    # The coffin racks along the west wall in both bays and in the stairwell's nook (a child's coffins), four coffins on
    # trestles in the bays, their lids shoved aside or off.
    turn = Matrix.Rotation(math.radians(90.0), 4, 'Z')
    rack(m, Space(Matrix.Translation((CX[0] + 0.42, -2.65, CF)) @ turn))
    rack(m, Space(Matrix.Translation((CX[0] + 0.42, 1.35, CF)) @ turn), coffins=1)
    rack(m, Space(Matrix.Translation((1.66, CY[2] - 0.42, CF))), length=1.5, coffin_len=1.2)
    lids = ('ajar', 'off', 'ajar', 'on')
    spots = ((-5.15, -2.55), (-1.15, -2.7), (-1.15, 1.45), (2.85, -3.15))
    for k, ((x, y), lid) in enumerate(zip(spots, lids)):
        frame = Space(Matrix.Translation((x, y, CF)))
        for dx in (-0.62, 0.62):
            rr.trestle(m, Space(frame.matrix @ Matrix.Translation((dx, 0.0, 0.0))))
        rr.coffin(m, Space(frame.matrix @ Matrix.Translation((-0.98, 0.0, 0.62))), lid=lid, look='planks',
                  lid_look='planks')
        if lid == 'off':
            lean = Space(frame.matrix @ Matrix.Translation((-1.25, 0.45, 0.0)) @
                         Matrix.Rotation(math.radians(-75.0), 4, 'Y'))
            rr.coffin(m, lean, length=1.97, depth=0.04, lid='none', look='planks')
        m.hull((2.0, 0.66, 1.0), at=(0.0, 0.0, 0.5), space=frame)
        m.socket(f'Coffin_{k + 1}', frame.world((0.0, 0.0, 1.0)), (0.0, 0.0, 0.0))
    rr.lamp_bracket(m, (CX[1] + 0.3, CY[1]), (1.0, 0.0), ZS - 0.3, socket='Light_Cellar_1')
    rr.lamp_bracket(m, (CX[2], CY[1] - 0.3), (0.0, -1.0), ZS - 0.3, socket='Light_Cellar_2')
    rr.lamp_bracket(m, (XI1, 2.6), (-1.0, 0.0), LAND_Z + 2.0, socket='Light_CellarStair')
    m.socket('CellarFloor', (3.15, -1.2, CF), rr.rot_to((0.0, -1.0)))
    m.section('cellar things')

    # --- The cold room: through a doorway in the back wall (its door is RR_CellarDoor on SOCKET_ColdRoomDoor), a barrel
    # vault, Thaddeus's bench and a pair of trestles with nothing on them. ---
    cx0, cx1, cy0, cy1 = COLD
    rr.flat_cells(m, cx0, cx1, cy0, cy1, CF, uv=Tile('GroundDirt', along=None), mat='dirt', every=2.0)
    chalf = (cx1 - cx0) * 0.5
    ccen = (cx0 + cx1) * 0.5
    crise = COLD_CROWN - ZS
    tb = kit._new_bmesh()
    rows = []
    for i in range(9):
        x = cx0 + (cx1 - cx0) * i / 8
        z = arch_z(x - ccen, chalf, crise, ZS)
        rows.append([tb.verts.new((x, y, z)) for y in (cy0, (cy0 + cy1) * 0.5, cy1)])
    for i in range(8):
        for k in range(2):
            f = tb.faces.new((rows[i][k], rows[i + 1][k], rows[i + 1][k + 1], rows[i][k + 1]))
            f.normal_update()
            if f.normal.z > 0.0:
                f.normal_flip()
    m.emit(tb, rr.Unrolled('StoneWall', lambda co: (co.y, arch_len(co.x - ccen, chalf, crise))), 'stone', smooth=True)
    for x, face in ((cx0, (1.0, 0.0, 0.0)), (cx1, (-1.0, 0.0, 0.0))):
        rr.polygon(m, [(x, cy0, CF), (x, cy1, CF), (x, cy1, ZS), (x, cy0, ZS)], STONE, 'stone', facing=face)
    end = [(cx0 + (cx1 - cx0) * i / 8, arch_z(cx0 + (cx1 - cx0) * i / 8 - ccen, chalf, crise, ZS)) for i in range(9)]
    for y, face, holes in ((cy0, (0.0, 1.0, 0.0), [(COLD_DOOR[0], COLD_DOOR[1], CF + 2.1)]), (cy1, (0.0, -1.0, 0.0), [])):
        pts = [(cx0, CF)] + end + [(cx1, CF)]
        for a, b, top in holes:
            pts += [(b, CF), (b, top), (a, top), (a, CF)]
        rr.polygon(m, [(x, y, z) for x, z in pts], STONE, 'stone', facing=face)
    rr.lining(m, (COLD_DOOR[0], CY[2]), (COLD_DOOR[1], CY[2]), (0.0, 1.0), cy0 - CY[2], CF, CF + 2.1, look='C')
    m.socket('ColdRoomDoor', (COLD_DOOR[1] - 0.01, (CY[2] + cy0) * 0.5, CF), (0.0, 0.0, 0.0))
    bx, by = ccen, cy1 - 0.42
    town.box(m, (2.2, 0.7, 0.08), at=(bx, by, CF + 0.86), look='A')
    for sx in (-1.0, 1.0):
        for sy in (-1.0, 1.0):
            town.box(m, (0.09, 0.09, 0.82), at=(bx + sx * 1.0, by + sy * 0.28, CF + 0.41), look='planks', drop=('-z',))
    town.box(m, (2.0, 0.6, 0.03), at=(bx, by, CF + 0.2), look='planks')
    town.prism(m, [(0.0, 0.0), (0.04, 0.0), (0.04, 0.5), (0.3, 0.5), (0.3, 0.54), (0.0, 0.54)], 0.006, look='C',
               matrix=Matrix.Translation((bx - 0.7, by - 0.1, CF + 0.903)) @ Matrix.Rotation(math.radians(-90.0), 4, 'X'))
    town.box(m, (0.6, 0.03, 0.006), at=(bx + 0.25, by - 0.18, CF + 0.903), rot=(0.0, 0.0, -9.0), look='band_G')
    rr.lamp_table(m, (bx + 0.85, by + 0.15, CF + 0.9), socket='Light_ColdRoom')
    m.hull((2.2, 0.7, 0.9), at=(bx, by, CF + 0.45))
    for dx in (-0.6, 0.6):
        rr.trestle(m, Space(Matrix.Translation((bx + dx, by - 1.55, CF))))
    m.hull((1.6, 0.7, 0.62), at=(bx, by - 1.55, CF + 0.31))
    m.socket('ThaddeusBench', (bx, by - 0.85, CF), rr.rot_to((0.0, 1.0)))
    m.section('cold room')

    # --- Collision: the workshop (walls round the yard doors, the floor round the hole, the enclosure round its door, the
    # roof), the stair, the cellar and the cold room. ---
    fx0, fx1 = DOOR
    for a, b in ((X0, fx0), (fx1, X1)):
        m.hull((b - a, T, EAVE), at=((a + b) * 0.5, Y0 + T * 0.5, EAVE * 0.5))
    m.hull((fx1 - fx0, T, EAVE - BASE - 2.3), at=((fx0 + fx1) * 0.5, Y0 + T * 0.5, (EAVE + BASE + 2.3) * 0.5))
    m.hull((X1 - X0, T, EAVE), at=(0.0, Y1 - T * 0.5, EAVE * 0.5))
    for x in (X0 + T * 0.5, X1 - T * 0.5):
        m.hull((T, Y1 - Y0, EAVE), at=(x, 0.0, EAVE * 0.5))
    m.hull((HOLE[0] - X0, Y1 - Y0, 0.3), at=((X0 + HOLE[0]) * 0.5, 0.0, BASE - 0.15))
    m.hull((X1 - HOLE[0], HOLE[2] - Y0, 0.3), at=((HOLE[0] + X1) * 0.5, (Y0 + HOLE[2]) * 0.5, BASE - 0.15))
    m.hull((ENC, YI1 - ey0, EH), at=(ex0 + ENC * 0.5, (ey0 + YI1) * 0.5, BASE + EH * 0.5))
    m.hull((CELLAR_DOOR[0] - ex0, ENC, EH), at=((ex0 + CELLAR_DOOR[0]) * 0.5, ey0 + ENC * 0.5, BASE + EH * 0.5))
    m.hull((CELLAR_DOOR[1] - CELLAR_DOOR[0], ENC, EH - 2.1), at=((CELLAR_DOOR[0] + CELLAR_DOOR[1]) * 0.5,
                                                                ey0 + ENC * 0.5, BASE + (EH + 2.1) * 0.5))
    m.hull_points([(x, y, z) for x in (X0 - 0.35, X1 + 0.35) for y, z in (
        (Y0 - 0.35, EAVE - 0.35 * tan + 0.2), (Y1 + 0.35, EAVE - 0.35 * tan + 0.2), (0.0, EAVE + 3.5 * tan + 0.25))])
    m.hull((XI1 - F2[0], YI1 - LAND_Y, LAND_Z - CF), at=((F2[0] + XI1) * 0.5, (LAND_Y + YI1) * 0.5, (LAND_Z + CF) * 0.5))
    m.hull((F1[1] - F1[0], LAND_Y - HOLE[2], LAND_Z - CF), at=((F1[0] + F1[1]) * 0.5, (HOLE[2] + LAND_Y) * 0.5,
                                                               (LAND_Z + CF) * 0.5))
    m.hull_points([(x, y, z) for x in SPINE for y, z in ((HOLE[2] + 0.05, CF), (LAND_Y, CF)) + tuple(spine_top)])
    m.hull_points([(x, y, z) for x in (par - ENC, par) for y, z in ((FOOT2 - 0.3, CF), (YI1, CF)) + tuple(ptop)])
    m.hull((CX[3] - CX[0] + 0.8, CY[2] - CY[0] + 0.8, 0.3), at=((CX[0] + CX[3]) * 0.5, (CY[0] + CY[2]) * 0.5, CF - 0.15))
    for x in (CX[0] - WALL_C * 0.5, CX[3] + WALL_C * 0.5):
        m.hull((WALL_C, CY[2] - CY[0] + 2.0 * WALL_C, BASE - CF), at=(x, (CY[0] + CY[2]) * 0.5, (BASE + CF) * 0.5))
    m.hull((CX[3] - CX[0], WALL_C, ZC - CF + 0.3), at=((CX[0] + CX[3]) * 0.5, CY[0] - WALL_C * 0.5,
                                                       (ZC + CF + 0.3) * 0.5))
    for a, b in ((CX[0], COLD_DOOR[0]), (COLD_DOOR[1], CX[3])):
        m.hull((b - a, WALL_C, BASE - CF), at=((a + b) * 0.5, CY[2] + WALL_C * 0.5, (BASE + CF) * 0.5))
    m.hull((COLD_DOOR[1] - COLD_DOOR[0], WALL_C, ZC + 0.3 - (CF + 2.1)),
           at=((COLD_DOOR[0] + COLD_DOOR[1]) * 0.5, CY[2] + WALL_C * 0.5, (CF + 2.1 + ZC + 0.3) * 0.5))
    for (x0, x1), (y0, y1) in (((CX[0], CX[2]), (CY[0], CY[2])), ((CX[2], CX[3]), (CY[0], CY[1]))):
        m.hull((x1 - x0, y1 - y0, 0.35), at=((x0 + x1) * 0.5, (y0 + y1) * 0.5, ZC + 0.12))
    m.hull((cx1 - cx0 + 0.8, cy1 - cy0 + 0.4, 0.3), at=(ccen, (cy0 + cy1) * 0.5 + 0.2, CF - 0.15))
    for x in (cx0 - WALL_C * 0.5, cx1 + WALL_C * 0.5):
        m.hull((WALL_C, cy1 - cy0 + WALL_C, COLD_CROWN - CF + 0.3), at=(x, (cy0 + cy1) * 0.5 + 0.2,
                                                                         (COLD_CROWN + CF + 0.3) * 0.5))
    m.hull((cx1 - cx0, WALL_C, COLD_CROWN - CF + 0.3), at=(ccen, cy1 + WALL_C * 0.5, (COLD_CROWN + CF + 0.3) * 0.5))
    m.hull((cx1 - cx0, cy1 - cy0, 0.3), at=(ccen, (cy0 + cy1) * 0.5, COLD_CROWN + 0.1))

    px, py = PIVOT
    m.rooms += [
        f'workshop {XI1 - XI0:.2f} x {YI1 - YI0:.2f} m inside, floor +{BASE:.2f}, open to its roof (tie beams at '
        f'+{EAVE - 0.22:.2f}); yard doors {DOOR[1] - DOOR[0]:.2f} x 2.30 m, the cellar door 1.10 x 2.10 m',
        f'stair: 19 risers of {R_RISE * 100:.1f} cm (treads {R_RUN * 100:.0f} cm); flight 1 {F1[1] - F1[0]:.2f} m wide '
        f'down to a half landing at {LAND_Z:+.2f}, flight 2 {F2[1] - F2[0]:.2f} m wide back down to the floor',
        f'cellar {CX[3] - CX[0]:.2f} x {CY[2] - CY[0]:.2f} m inside, floor {CF:+.2f} ({DEPTH:.2f} m under the ground at '
        f'the pivot), groin vaults springing at {ZS:+.2f}, crowns {ZC:+.2f} (2.60 m clear), their tops {ZC + 0.3:+.2f}; '
        f'two piers 0.60 m',
        f'cold room {COLD[1] - COLD[0]:.2f} x {COLD[3] - COLD[2]:.2f} m, floor {CF:+.2f}, barrel vault crown '
        f'{COLD_CROWN:+.2f} (2.40 m clear); its door 1.10 x 2.10 m',
        f'underground from the pivot: cellar x {CX[0] - WALL_C - px:.2f}..{CX[3] + WALL_C - px:.2f}, '
        f'y {CY[0] - WALL_C - py:.2f}..{CY[2] + WALL_C - py:.2f}; cold room x {COLD[0] - WALL_C - px:.2f}..'
        f'{COLD[1] + WALL_C - px:.2f}, y {COLD[2] - WALL_C - py:.2f}..{COLD[3] + WALL_C - py:.2f}; z {CF - 0.3:+.2f} to '
        f'{ZC + 0.3:+.2f}',
        f'terrain hole over the stairwell (or the terrain under {CF - 0.3:+.2f} there): x {HOLE[0] - ENC - px:.2f}..'
        f'{X1 - px:.2f}, y {HOLE[2] - ENC - py:.2f}..{Y1 - py:.2f} (its edges hide under the enclosure and the walls)',
    ]
    for line in m.rooms:
        print(f'WORKSHOP: {line}', flush=True)
    m.captions = ["Bright & Daughter's coffin workshop over its cellar:", 'the stair down, groin vaults, the cold room.']
    return m, m.finish(pivot=PIVOT, footprint=(X0, X1, Y0, Y1))


# --- The cellar door ---

def cellar_door():
    """RR_CellarDoor: a heavy plank leaf, its hinge at the origin on the floor, the leaf along -X, front toward -Y."""
    m = rr.Building('RR_CellarDoor', seed=821)
    w, h = 1.08, 2.08
    rr.plank_leaf(m, Space(Matrix.Identity(4)), w, h, look='C', battens_on=1.0, hinge_left=False, straps=False,
                  thick=0.05)
    for z in (0.32, h - 0.38):
        town.board(m, (0.04, -0.03, z), (-w * 0.82, -0.03, z), 0.07, 0.014, look='iron', drop=('+y',))
        town.board(m, (0.04, 0.072, z), (-w * 0.82, 0.072, z), 0.07, 0.014, look='iron', face=(0.0, 1.0, 0.0),
                   drop=('+y',))
    town.box(m, (0.1, 0.012, 0.16), at=(-w + 0.11, -0.031, 1.0), look='iron', drop=('+y',))
    town.lathe(m, [(0.05, 0.0), (0.06, 0.008), (0.05, 0.016), (0.04, 0.008)], at=(-w + 0.11, -0.05, 0.93), sides=8,
               look='iron', axis=Matrix.Rotation(math.radians(90.0), 4, 'X'))
    m.hull((w, 0.1, h), at=(-w * 0.5, 0.02, h * 0.5))
    m.captions = ['The cellar door (the stair and the cold room),', 'hung by the game on its hinge sockets.']
    return m, m.finish(pivot=(0.0, 0.0), ground=False, Nanite=0, LODs='50,25')


BUILT = [workshop(), cellar_door()]
models = [obj for _, obj in BUILT]


def at(x, y, z):
    """A point as built (the workshop's footprint middle) where the exported model has it."""
    return (x - PIVOT[0], y - PIVOT[1], z)


if lt.want_preview():
    (wm, shop), (dm, door) = BUILT
    shots = {shop.name: [], door.name: []}
    shots[shop.name].append(rr.exterior(shop, 'RR_Workshop_Bright', view=(-0.6, -1.0, 0.32)))
    shots[shop.name].append(rr.plan_view(shop, 'RR_Workshop_Plan', z_cut=BASE + 2.45, direction=(-0.3, -0.6, 1.0),
                                         lamp_energy=40.0, keep=lambda c: (XI0 - 0.01 <= c.x + PIVOT[0] <= XI1 + 0.01 and
                                                                           YI0 - 0.01 <= c.y + PIVOT[1] <= YI1 + 0.01
                                                                           and c.z > 0.0)))
    shots[shop.name].append(rr.plan_view(shop, 'RR_Workshop_Cellar', z_cut=CF + 2.75, direction=(-0.3, -0.62, 1.0),
                                         lamp_energy=80.0, ground=False))
    shots[shop.name].append(rr.inside(shop, 'RR_Workshop_Inside', at(-3.0, -3.0, BASE + 1.45), at(2.6, 1.0, BASE + 1.2),
                                      lens=20.0))
    shots[shop.name].append(rr.inside(shop, 'RR_Workshop_Stair', at(4.3, -1.6, BASE + 1.45), at(3.6, 2.5, LAND_Z + 0.6),
                                      lens=20.0, ground=False))
    shots[shop.name].append(rr.inside(shop, 'RR_Workshop_CellarView', at(3.1, -1.4, CF + 1.45), at(-5.5, -0.5, CF + 1.0),
                                      lens=20.0, lamp_energy=90.0))
    shots[shop.name].append(rr.inside(shop, 'RR_Workshop_ColdRoom', at(-5.15, 3.0, CF + 1.45), at(-5.15, 7.7, CF + 0.9),
                                      lens=22.0, lamp_energy=90.0))
    shots[door.name].append(rr.exterior(door, 'RR_CellarDoor', view=(-0.5, -1.0, 0.2), figure='right', margin=1.3))
    rr.write_report('WorkshopBright', [wm, dm], shots)
