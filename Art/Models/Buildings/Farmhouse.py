"""The farmhouse at the farmstead (spawn): a one-and-a-half-storey plank house on a fieldstone foundation, weathered
vertical siding, a rusty corrugated tin roof with a dormer and a stovepipe, and a covered porch across the front with
a lantern by the door. The reference is the shack in Docs/Art/StyleTarget_Outpost.png.

A scripted model (Art/README.md) built with looter_buildings: the pivot is the middle of the footprint at ground level
and the porch faces the front (-Y). SOCKET_Smoke tops the stovepipe, SOCKET_Light is the lantern.

The file also makes Ransom Farm's farmhouse on Ransom's Rest (Docs/Areas/RansomsRest.md: Grandma Delia's door, Main 1,
6 and 7) and the two props that go with it:

  Farmhouse          the tutorial island's derelict house (boarded window, a shutter hanging askew, a closed plank
                     door). It's in the game there, so its build is exactly what it was: farmhouse() without lived_in
                     makes the same calls in the same order.
  Farmhouse_Ransom   the same house lived in, with the same footprint, pivot, walls, roof, porch, collision and
                     SOCKET_Smoke, so it stands where the old one stood. Every window glazed (the boarded ones too)
                     with linen curtains half drawn behind the sash bars, the two front ones lamplit (WindowGlow); the
                     shutters hung straight and latched open; the plank door open inward against the hall wall; black
                     crepe swagged over the door with ribbon tails; a shallow lamplit hall behind the screen door
                     (pale walls over a board wainscot, a board floor, a runner to a lamp on a side table at its end,
                     on the door's axis, and on its left wall Delia's black shawl on a peg and the hall mirror covered
                     in crepe); and on the porch a rocking chair, a broom and a pail, and a stool beside the door for
                     the plate. Sockets:
                       SOCKET_ScreenDoor  the screen door's hinge line at its foot: SM_ScreenDoor hangs there closed
                       SOCKET_Speaker     Delia's speaker point: the middle of the screen door's face, 1.5 m over the
                                          floor, facing out
                       SOCKET_Handoff     where a held thing comes out through the screen door opened a crack
                                          (Heirloom, Main 7): 1 m over the floor, just outside the door on its latch
                                          side, facing out
                       SOCKET_Plate       the stool's seat: SM_PorchPlate sits there
                       SOCKET_Light       the hall lamp's globe; SOCKET_Light_Porch the lantern by the door (the old
                                          house's SOCKET_Light)
                       SOCKET_Smoke       the stovepipe's top, as on the old house
  ScreenDoor         a light timber screen door in the house's faded teal (trim strip H1): stiles and rails, a push
                     rail, a kick panel, a brace across the lower screen, a door spring, a pull and two hinges; the
                     screens are ScreenMesh on the Glass master. Its pivot is its hinge line at its foot and its front
                     faces -Y: hung on SOCKET_ScreenDoor it sits closed, and a turn about its Z swings it open (out
                     toward the porch: +25 degrees of yaw in Unreal, -25 about Blender's Z). No collision, no Nanite.
  PorchPlate         the plate Delia sets out: a checked cloth over a plate of food (the cloth takes the plate's shape;
                     the plate never shows), a fork beside it and a tin cup. Its pivot is the middle of its foot; it
                     sits on SOCKET_Plate. No collision, no Nanite.

Parts the lived-in house leaves out (the boards over its windows, the shutters as they hung, the closed door, the loose
board) are still built, into a sink that draws from the house's random stream, so everything the two houses share
comes out the same; its own parts draw from a stream of their own (Variant). --preview renders the old house's preview
and the lived-in house's look sheet (Saved/ArtPreviews/RansomsRest/Farmhouse/, previews()).
"""
import math
import os
import random
from contextlib import contextmanager

import bmesh

import looter_buildings as kit
import looter_model as lm
import looter_textures as lt
from looter_buildings import Matrix, Opening, Space, Tile, Trim, Vector, place

X0, X1, Y0, Y1 = -4.2, 4.2, -3.1, 3.1   # outer faces of the walls
BASE = 0.6          # top of the foundation: the floor
WALL = 3.5          # siding height up to the eave (a storey and a half)
EAVE = BASE + WALL
PITCH = 40.0
THICK = 0.14
W, D = X1 - X0, Y1 - Y0
RISE = (D * 0.5) * kit.math.tan(kit.math.radians(PITCH))


def farmhouse(name='Farmhouse', lived_in=False):
    """The house. lived_in makes Delia's (Farmhouse_Ransom); without it, the tutorial island's, exactly as it was."""
    m = (LivedIn if lived_in else kit.Model)(name, seed=7)
    rng = m.rng
    v = Variant(m) if lived_in else None
    siding = Trim('A', world=True, rotate=True)  # vertical boards

    kit.plinth(m, X0, X1, Y0, Y1, BASE)

    m.section('foundation')

    # --- Walls, counterclockwise from above: front, right, back, left. ---
    door = Opening('door', W * 0.5 - 0.5, 0.0, 1.0, 2.15, hinge='left')
    walls = [
        ((X0, Y0), (X1, Y0), None, [
            Opening('window', 1.15, 0.85, 0.9, 1.3, style='boarded'),
            door,
            Opening('window', 6.35, 0.85, 0.9, 1.3, style='shutters', paint='H1', sash='H1')]),
        ((X1, Y0), (X1, Y1), (D * 0.5, WALL + RISE), [
            Opening('window', 2.65, 0.85, 0.9, 1.3, style='glass', paint='H1', panes=(2, 3)),
            Opening('window', 2.72, 3.75, 0.76, 0.9, style='boarded')]),
        ((X1, Y1), (X0, Y1), None, [
            Opening('window', 1.6, 0.9, 0.8, 1.2, style='shutters', paint='H2', sash='A'),
            Opening('window', 6.0, 0.9, 0.8, 1.2, style='glass', paint='A', panes=(2, 2))]),
        ((X0, Y1), (X0, Y0), (D * 0.5, WALL + RISE), [
            Opening('window', 1.8, 0.85, 0.9, 1.3, style='shutters', paint='H1', sash='H1'),
            Opening('window', 2.72, 3.75, 0.76, 0.9, style='glass', paint='A', panes=(1, 2))]),
    ]
    for index, (p0, p1, apex, items) in enumerate(walls):
        space = kit.wall_space(p0, p1, BASE)
        length = (kit.Vector(p1) - kit.Vector(p0)).length
        m.panel(kit.wall_outline(length, WALL, items, apex), kit.holes(items), THICK,
                Trim('A', world=True, rotate=True, u=rng.uniform(0.0, 6.4)), space=space, around=items)
        if v is None:
            kit.openings(m, space, items, THICK)
        else:
            v.openings(space, items, THICK, front=index == 0)
        gable_end = apex is not None
        # Corner boards (the front and back ones wrap over the side walls' edges), the water table along the foundation
        # (broken by the door) and, on the eave walls, a frieze under the soffit.
        top = WALL if not gable_end else WALL - 0.02
        for x, w in ((0.07 - (0.0 if gable_end else 0.04), 0.14 + (0.0 if gable_end else 0.08) * 0.5),
                     (length - 0.07 + (0.0 if gable_end else 0.04), 0.14 + (0.0 if gable_end else 0.08) * 0.5)):
            kit.trim_board(m, space, (x, 0.0, -0.05), (x, 0.0, top), w, 'C', thick=0.04, lane=rng.randrange(4))
        doors = [o for o in items if o.kind == 'door']
        spans = [(0.1, length - 0.1)]
        for o in doors:
            spans = [(a, o.x - 0.2) for a, b in spans] + [(o.x + o.w + 0.2, b) for a, b in spans]
        for a, b in spans:
            kit.trim_board(m, space, (a, 0.0, 0.1), (b, 0.0, 0.1), 0.2, 'C', thick=0.045, lift=0.01)
        if not gable_end:
            kit.trim_board(m, space, (0.12, 0.0, WALL - 0.14), (length - 0.12, 0.0, WALL - 0.14), 0.24, 'A',
                           thick=0.035)
        else:
            # A few horizontal boards nailed over rot low on the gable walls.
            x = rng.uniform(0.6, length - 1.6)
            for k in range(2):
                kit.trim_board(m, space, (x + rng.uniform(-0.05, 0.05), 0.0, 0.32 + k * 0.2),
                               (x + 0.9 + rng.uniform(-0.1, 0.1), 0.0, 0.34 + k * 0.2), 0.19, 'A', thick=0.03,
                               lift=0.035)

    # A patch over the front siding beside the door, and a loose board. Delia's house has neither: her stool stands
    # where the patch was and her broom where the board leaned.
    front = kit.wall_space((X0, Y0), (X1, Y0), BASE)
    tidy = m if v is None else v.sink
    for k in range(3):
        kit.trim_board(tidy, front, (5.0, 0.0, 0.35 + k * 0.2),
                       (5.85 + rng.uniform(-0.08, 0.08), 0.0, 0.35 + k * 0.2 + 0.02), 0.19, 'A', thick=0.03, lift=0.035)
    tidy.board((2.95, -0.05, 0.2), (3.05, -0.09, 1.35), 0.19, 0.03, uv='A', space=front)

    m.section('walls, windows, doors')

    # --- Main roof: tin over a sagging deck, with a dormer in front and the stovepipe. ---
    front_roof, back_roof = kit.gable(m, X0, X1, Y0, Y1, EAVE, PITCH, overhang=0.45, rake=0.35, deck=0.12, sag=0.09)
    cos_p = kit.math.cos(kit.math.radians(PITCH))
    DORMER_Y, DORMER_HALF = -2.75, 0.9

    def under_dormer(x, y):
        # Slope coordinates to the house's: the front slope starts 0.45 m out from the wall and 0.35 m past the corner.
        wx, wy = x + X0 - 0.35, Y0 - 0.45 + y * cos_p
        return abs(wx) < DORMER_HALF - 0.5 and DORMER_Y + 0.6 < wy < -0.75
    kit.tin(m, front_roof, skip=under_dormer)
    kit.tin(m, back_roof)
    kit.ridge_cap(m, front_roof, back_roof)

    m.section('main roof')

    # The dormer: a small gable facing front, its ridge running back into the main roof.
    tan_p = kit.math.tan(kit.math.radians(PITCH))
    D_EAVE = 5.5
    D_BASE = 4.25
    dormer_frame = kit.Matrix.Translation((0.0, DORMER_Y, 0.0)) @ kit.Matrix.Rotation(kit.math.radians(90.0), 4, 'Z')
    d_front, d_back = kit.gable(m, 0.0, 2.3, -DORMER_HALF, DORMER_HALF, D_EAVE, PITCH, overhang=0.12, rake=0.18,
                                deck=0.08, sag=0.0, ends=(True, False), frame=dormer_frame)
    kit.tin(m, d_front, sheets=(1, 2))
    kit.tin(m, d_back, sheets=(1, 2))
    kit.ridge_cap(m, d_front, d_back)
    d_rise = DORMER_HALF * tan_p
    d_window = Opening('window', DORMER_HALF - 0.36, 0.52, 0.72, 0.62, style='glass', paint='H1', sash='H1',
                       panes=(2, 2))
    d_space = kit.wall_space((-DORMER_HALF, DORMER_Y), (DORMER_HALF, DORMER_Y), D_BASE)
    m.panel(kit.wall_outline(2 * DORMER_HALF, D_EAVE - D_BASE, [], (DORMER_HALF, D_EAVE - D_BASE + d_rise)),
            kit.holes([d_window]), 0.1, siding, space=d_space, around=[d_window])
    kit.openings(m, d_space, [d_window], 0.1)
    if v is not None:
        with v.own():
            curtains(m, d_space, d_window, 0.1)
    for x in (0.06, 2 * DORMER_HALF - 0.06):
        kit.trim_board(m, d_space, (x, 0.0, 0.2), (x, 0.0, D_EAVE - D_BASE - 0.02), 0.12, 'C', thick=0.035)
    # Cheek walls from the dormer's face back until they vanish into the roof.
    cheek = 1.55
    for p0, p1 in (((DORMER_HALF, DORMER_Y), (DORMER_HALF, DORMER_Y + cheek)),
                   ((-DORMER_HALF, DORMER_Y + cheek), (-DORMER_HALF, DORMER_Y))):
        space = kit.wall_space(p0, p1, D_BASE)
        m.panel(kit.rect(0.0, 0.0, cheek, D_EAVE - D_BASE), (), 0.1, Trim('A', world=True, rotate=True), space=space)

    m.section('dormer')

    # The stovepipe, through the front slope, with a rain cap; smoke comes out under the cap.
    PIPE = (2.75, -1.25)
    roof_z = EAVE + (PIPE[1] - Y0) * tan_p
    pipe_top = 7.55
    m.cylinder((PIPE[0], PIPE[1], roof_z - 0.2), (PIPE[0], PIPE[1], pipe_top), 0.1, sides=10, uv=kit.Tile('MetalRust'),
               mat='metal', caps=(False, True))
    m.cylinder((PIPE[0], PIPE[1], pipe_top - 0.35), (PIPE[0], PIPE[1], pipe_top - 0.28), 0.125, sides=10,
               uv=kit.Tile('MetalRust'), mat='metal')
    m.cylinder((PIPE[0], PIPE[1], pipe_top + 0.12), (PIPE[0], PIPE[1], pipe_top + 0.3), 0.26, 0.03, sides=10,
               uv=kit.Tile('MetalRust'), mat='metal')
    for k in range(3):
        a = kit.math.radians(90.0 + 120.0 * k)
        x, y = PIPE[0] + 0.09 * kit.math.cos(a), PIPE[1] + 0.09 * kit.math.sin(a)
        m.box((0.02, 0.02, 0.16), at=(x, y, pipe_top + 0.06), uv=kit.Tile('MetalRust'), mat='metal')
    # A flashing plate where the pipe goes through the tin.
    pipe_slope_x = PIPE[0] - (X0 - 0.35)
    pipe_slope_y = (PIPE[1] - (Y0 - 0.45)) / cos_p
    m.box((0.55, 0.6, 0.02), at=(pipe_slope_x, pipe_slope_y, 0.06), rot=(0.0, 0.0, 3.0), uv='F', space=front_roof)
    m.socket('Smoke', (PIPE[0], PIPE[1], pipe_top + 0.08))

    m.section('stovepipe')

    # --- Porch: plank floor on stone piers, posts, a beam with knee braces and a low tin roof. ---
    PX0, PX1, PY = -3.6, 3.6, Y0 - 2.2
    for y in kit.frange(Y0 - 0.1, PY, -0.2):
        m.board((PX0 - rng.uniform(0.0, 0.04), y, BASE - 0.025), (PX1 + rng.uniform(0.0, 0.04), y, BASE - 0.025), 0.19,
                0.05, face=(0.0, 0.0, 1.0), uv='A')
    m.board((PX0, PY + 0.05, BASE - 0.17), (PX1, PY + 0.05, BASE - 0.17), 0.24, 0.07, uv='C')
    for x in (PX0 - 0.03, PX1 + 0.03):
        m.board((x, PY, BASE - 0.17), (x, Y0, BASE - 0.17), 0.24, 0.07, face=(1.0 if x > 0 else -1.0, 0.0, 0.0), uv='C')
    POSTS = [-3.45, -1.15, 1.15, 3.45]
    PY_POST = PY + 0.17
    for x in POSTS:
        kit.stone_stack(m, (x, PY_POST, 0.0), (0.4, 0.4), BASE - 0.2, taper=0.1, rot=rng.uniform(-8.0, 8.0), bevel=0.0)
        m.board((x, PY_POST, BASE), (x, PY_POST, 3.0), 0.16, 0.16, uv=Trim('C', lane='each'), bevel=0.015)
        m.box((0.22, 0.22, 0.05), at=(x, PY_POST, BASE + 0.025), uv='C')
    BEAM_Z = 3.1
    m.board((PX0 - 0.15, PY_POST, BEAM_Z), (PX1 + 0.15, PY_POST, BEAM_Z), 0.2, 0.17, uv=Trim('C', lane='each'),
            bevel=0.015)
    for x in POSTS:
        for side in (-1.0, 1.0):
            if (x < 0 and side < 0 and x == POSTS[0]) or (x > 0 and side > 0 and x == POSTS[-1]):
                continue
            m.board((x + side * 0.06, PY_POST, 2.5), (x + side * 0.55, PY_POST, 3.0), 0.1, 0.09, uv='C')
    # Steps up to the door.
    for k, (top, y) in enumerate(((0.4, PY - 0.15), (0.2, PY - 0.45))):
        m.board((-0.8, y, top - 0.025), (0.8, y, top - 0.025), 0.3, 0.05, face=(0.0, 0.0, 1.0), uv='A')
        m.board((-0.78, y - 0.13, top * 0.5 - 0.02), (0.78, y - 0.13, top * 0.5 - 0.02), top - 0.05, 0.03, uv='A')
    for x in (-0.82, 0.82):
        m.board((x, PY - 0.62, 0.02), (x, PY + 0.02, BASE - 0.05), 0.18, 0.05, face=(1.0 if x > 0 else -1.0, 0.0, 0.0),
                uv='C')

    m.section('porch')

    # The porch roof: a shed slope from the wall down onto the beam and past it.
    P_THETA = kit.math.atan2(3.55 - (BEAM_Z + 0.1), Y0 - PY_POST)
    p_deck = 0.08
    p_normal = kit.Vector((0.0, -kit.math.sin(P_THETA), kit.math.cos(P_THETA)))
    p_over = 0.35
    p_origin = (kit.Vector((PX0 - 0.25, PY_POST - p_over, BEAM_Z + 0.1 - p_over * kit.math.tan(P_THETA)))
                + p_normal * p_deck)
    p_len = (Y0 - (PY_POST - p_over)) / kit.math.cos(P_THETA)
    porch_roof = kit.Slope(p_origin, (1.0, 0.0, 0.0), (0.0, kit.math.cos(P_THETA), kit.math.sin(P_THETA)),
                           PX1 - PX0 + 0.5, p_len, sag=0.06)
    kit.roof_deck(m, porch_roof, p_deck)
    kit.tin(m, porch_roof, top=p_len - 0.03)
    m.box((porch_roof.width + 0.02, 0.2, 0.05), at=(porch_roof.width * 0.5, p_len - 0.1, 0.05), rot=(-6.0, 0.0, 0.0),
          uv='F', space=porch_roof, cuts=6)

    m.section('porch roof')

    # A lantern on an iron bracket beside the door.
    LANTERN = (door.x + door.w + 0.45 + X0, Y0 - 0.34, BASE + 2.02)
    m.box((0.12, 0.03, 0.3), at=(LANTERN[0], Y0 - 0.015, LANTERN[2] + 0.32), uv=kit.Tile('MetalRust'), mat='metal')
    m.box((0.03, 0.36, 0.03), at=(LANTERN[0], Y0 - 0.18, LANTERN[2] + 0.43), uv=kit.Tile('MetalRust'), mat='metal')
    m.board((LANTERN[0], Y0 - 0.02, LANTERN[2] + 0.22), (LANTERN[0], Y0 - 0.26, LANTERN[2] + 0.42), 0.025, 0.025,
            face=(1.0, 0.0, 0.0), uv=kit.Tile('MetalRust'), mat='metal')
    kit.lantern(m, LANTERN, hang=0.0)
    if v is not None:
        # The hall lamp takes SOCKET_Light (the speaker's light, behind the screen); the lantern's is renamed.
        m.sockets[-1] = ('Light_Porch',) + m.sockets[-1][1:]
    m.box((0.02, 0.02, 0.1), at=(LANTERN[0], LANTERN[1], LANTERN[2] + 0.37), uv=kit.Tile('MetalRust'), mat='metal')

    m.section('lantern')

    if v is not None:
        v.dress(front)

    # --- Collision: the house body, the roof, the dormer, the porch, its steps, posts and roof. ---
    m.hull((W + 0.16, D + 0.16, EAVE), at=(0.0, 0.0, EAVE * 0.5))
    eave_top = EAVE - 0.45 * tan_p + 0.2
    ridge_top = EAVE + RISE + 0.22
    m.hull_points([(x, y, eave_top) for x in (X0 - 0.35, X1 + 0.35) for y in (Y0 - 0.45, Y1 + 0.45)] +
                  [(x, 0.0, ridge_top) for x in (X0 - 0.35, X1 + 0.35)])
    m.hull_points([(x, y, z) for x in (-DORMER_HALF - 0.2, DORMER_HALF + 0.2) for y in (DORMER_Y - 0.15, DORMER_Y + 1.6)
                   for z in (D_BASE, D_EAVE)] + [(0.0, y, D_EAVE + d_rise + 0.15) for y in (DORMER_Y - 0.15, -0.3)])
    m.hull((PX1 - PX0 + 0.08, Y0 - PY + 0.05, BASE), at=(0.0, (Y0 + PY) * 0.5, BASE * 0.5))
    m.hull((1.64, 0.3, 0.4), at=(0.0, PY - 0.15, 0.2))
    m.hull((1.64, 0.3, 0.2), at=(0.0, PY - 0.45, 0.1))
    for x in POSTS:
        m.hull((0.18, 0.18, 3.0 - BASE), at=(x, PY_POST, (BASE + 3.0) * 0.5))
    m.hull((porch_roof.width, p_len, 0.2), at=(porch_roof.width * 0.5, p_len * 0.5, -0.02), space=porch_roof)

    if v is None:
        return m.finish()
    obj = m.finish(preview=False)
    v.close()
    return obj


# --- The lived-in house (Farmhouse_Ransom) ---
# Its front wall's space (front in farmhouse(): s along the wall from X0, t in from its outer face, u up from the floor)
# holds the door, the hall and the crepe.

DOOR_X, DOOR_W, DOOR_H = W * 0.5 - 0.5, 1.0, 2.15   # the front door's opening, as farmhouse() makes it
# The hall behind the door: its left wall in the door jamb's plane (the open door lies against it), its right wall
# 0.65 m past the other jamb, its end wall 1.9 m in; built only as far as it shows through the door.
HALL_S0, HALL_S1 = DOOR_X - 0.01, DOOR_X + DOOR_W + 0.65
HALL_T0, HALL_T1 = THICK - 0.01, 1.9
HALL_TOP = 2.65
# The screen shows lamplit plaster whichever way it's seen: the end wall from the step (from the left wall to about
# 0.4 m right of the door's middle), the right wall from the path on the left, the left wall from the porch's right
# side. The lamp on its side table stands against the end wall on the door's axis, behind Delia's voice; the dark
# things (her shawl and the covered mirror) hang on the left wall, which the step sees edge on.
LAMP = (DOOR_X + DOOR_W * 0.5 + 0.02, -0.17)   # the side table: along the end wall, out from it (the globe on the axis)
# Where the hall's walls sit along the plaster strip (m), so the stretches that show through the door are clean ones,
# without the strip's chips (at 2.28-2.47, 3.13-3.57, 3.65-4.12 and 5.22-5.38 m) or its pale water stains (0 to 0.75
# and 5.4 to 6.4 m), which the lamp shows through the screen as smudges. The left wall's middle: its chipped part is
# behind the open door. The right and end walls are longer than the strip's clean metre (4.13 to 5.2 m), so each is two
# planes whose plaster folds over at a seam (PLASTER_FOLD: the right wall's 0.99 m in, the end wall's in line with the
# door's right jamb, just past what the step sees): both halves run from the fold, at this U, back toward 4.2 m.
PLASTER_U = {'left': 4.1, 'right': 5.12, 'end': 5.17}
PLASTER_FOLD = {'right': 0.99, 'end': DOOR_X + DOOR_W}
# The hall's board wainscot (strip C's hewn faces, upright) under a cap rail, this high: a darker band low on the walls,
# kept low so the lamplit plaster fills most of the screen.
WAINSCOT = 0.70
# Delia's hall mirror and her shawl, on the left wall past the open door (its free edge 1.07 m in): the mirror's frame
# from, to (m in from the front wall's outer face) and from, to over the floor, covered in crepe as a house of mourning
# covers its mirrors, where the porch's right side sees it through the door; and the shawl's peg, nearer the corner.
MIRROR = (1.12, 1.44, 1.16, 1.86)
SHAWL_T = 1.68
# The screen door in the frame: its front face flush with the casings', clear of the jambs and the threshold.
SCREEN_GAP, SCREEN_T, SCREEN_FOOT = 0.012, 0.035, 0.025
SCREEN_W = DOOR_W - 2.0 * SCREEN_GAP
SCREEN_H = DOOR_H - SCREEN_FOOT - 0.025              # its head swings under the crepe's lowest fold
SCREEN_HINGES = (0.27, SCREEN_H - 0.3)               # the hinges' middles over its foot
SCREEN_RAILS = ((0.0, 0.15), (0.40, 0.48), (0.88, 1.0))   # bottom, kick and push rails (z over its foot)
SCREEN_PIVOT = (X0 + DOOR_X + SCREEN_GAP, Y0 - SCREEN_T + 0.002, BASE + SCREEN_FOOT)
# The porch dressing (in the house's frame): the stool beside the door under the lantern, the broom and the pail on the
# door's other side, the rocking chair in front of the left window turned toward the steps.
STOOL, STOOL_TOP = (X0 + DOOR_X + DOOR_W + 0.48, Y0 - 0.25, BASE), 0.48
BROOM = (X0 + DOOR_X - 0.45, Y0 - 0.33, BASE)
PAIL = (X0 + DOOR_X - 0.82, Y0 - 0.3, BASE)
CHAIR, CHAIR_YAW = (-2.5, -4.1, BASE), 20.0
# Clean stretches of the trim sheet's strips along U (m), measured from T_HouseTrim_BC: plaster (G) without chips, for
# the curtains and the cloth's cream checks; solid oxide-red paint (H2), for its red checks.
CREAM_U = (4.2, 4.3, 4.4, 4.5, 4.6, 5.45, 5.55, 5.65, 5.75, 5.85)
RED_U = (0.55, 0.63, 0.71, 2.33, 2.41, 2.49, 2.57, 2.65, 5.38, 5.45)

# The front windows' panes (WindowGlow): the room behind the glass. By day it's about as dark as the other windows'
# glass (the trim sheet's H4, #2F393F) but warm, with a faint lamp glow (Glow is the emissive multiplier of the colour,
# as on LanternGlow), so the panes read as glass between the curtains; for dusk the game raises Glow and they read as
# lit windows. A slot of its own, so it changes without the lanterns.
WINDOW_COLOR, WINDOW_GLOW_DAY, WINDOW_GLOW_DUSK = 0x2e2219, 1.0, 14.0
# The insect screen's look on the Glass master (M_Glass: unlit, translucent; Fresnel exponent 3): a dark grey-green
# mesh that shows half of what's behind it face on and closes up toward a glance, darkening there, never brightening.
SCREEN_TINT, SCREEN_OPACITY, SCREEN_EDGE_OPACITY, SCREEN_RIM = 0x2e3530, 0.5, 0.95, 0.6

LIVED_IN = {
    'crepe': lambda: lt.material('Polymer', name='MourningCrepe', tint=0x161518),
    'windowglow': lambda: lm.material('WindowGlow', WINDOW_COLOR, Glow=WINDOW_GLOW_DAY, Variation=0.04),
}


class LivedIn(kit.Model):
    """A kit Model that also knows the lived-in house's materials (LIVED_IN)."""

    def slot(self, key):
        if key not in self.slots:
            make = LIVED_IN.get(key)
            self.slots[key] = (len(self.slots), make() if make else kit._material(key))
        return self.slots[key][0]


class Variant:
    """What turns the derelict house into Delia's. A part the lived-in house leaves out is built into a sink that shares
    the house's random stream, so it draws what it drew before and everything the two houses share comes out the same;
    the lived-in house's own parts draw from a stream of their own (inside own()), and the hall from another, so a
    change to the hall leaves the porch's things as they were."""

    def __init__(self, m, seed=71):
        self.m = m
        self.rng = random.Random(seed)
        self.hall_rng = random.Random(seed + 1)
        self.sink = kit.Model('_Sink', seed=0)
        self.sink.rng = m.rng

    @contextmanager
    def own(self, rng=None):
        main = self.m.rng
        self.m.rng = rng or self.rng
        try:
            yield self.m.rng
        finally:
            self.m.rng = main

    def openings(self, space, items, depth, front):
        """A wall's openings: every window glazed with curtains (on the front, lamplit), shutters hung straight, the
        door open."""
        m = self.m
        for o in items:
            if o.kind == 'door':
                kit.openings(self.sink, space, [o], depth)
                with self.own():
                    open_door(m, space, o, depth)
                continue
            style = o.opts.get('style', 'glass')
            if style == 'glass':
                kit.openings(m, space, [o], depth)
            else:
                kit.openings(self.sink, space, [o], depth)
                o = o if style == 'shutters' else glazed(o, front)
                with self.own():
                    window(m, space, o, depth, lit=front)
            with self.own():
                curtains(m, space, o, depth)

    def dress(self, front):
        """The hall, the crepe, the porch's things and the sockets the story uses."""
        m = self.m
        with self.own(self.hall_rng):
            light = hall(m, front)
        with self.own():
            crepe(m, front)
            rocking_chair(m, CHAIR, CHAIR_YAW)
            stool(m, STOOL)
            broom(m, BROOM)
            pail(m, PAIL)
        m.section('hall, crepe, porch things')
        m.socket('Light', light)
        m.socket('ScreenDoor', SCREEN_PIVOT)
        m.socket('Speaker', (X0 + DOOR_X + DOOR_W * 0.5, SCREEN_PIVOT[1], BASE + 1.5))
        m.socket('Handoff', (X0 + DOOR_X + DOOR_W - 0.08, Y0 - 0.22, BASE + 1.0))
        m.socket('Plate', (STOOL[0], STOOL[1], STOOL[2] + STOOL_TOP))

    def close(self):
        """Frees the sink (what it holds is never used)."""
        self.sink.bm.free()
        kit.bpy.data.objects.remove(self.sink._obj)
        kit.bpy.data.meshes.remove(self.sink._mesh)


# --- Parts (built in a kit Space; a wall's: x along it, z up, the outside toward -y) ---

def glazed(o, front):
    """A boarded window glazed: on the front like the shuttered window on the door's other side (teal casing, sash and
    shutters), elsewhere like the far gable's window (a weathered casing, two panes)."""
    if front:
        return Opening('window', o.x, o.z, o.w, o.h, style='shutters', paint='H1', sash='H1', panes=(2, 2))
    return Opening('window', o.x, o.z, o.w, o.h, style='glass', paint='A', panes=(1, 2))


def window(m, space, o, depth, lit=False):
    """kit.window for the lived-in house: the same casing, sill, glass and sash bars, the glass lamplit (WindowGlow)
    when lit, and shutters hung straight, each held open against the wall by an iron catch."""
    rng = m.rng
    x0, z0, w, h = o.x, o.z, o.w, o.h
    paint = o.opts.get('paint', 'A')
    sash_paint = o.opts.get('sash', paint)
    cw = o.opts.get('casing', 0.12)
    for x in (x0 - cw * 0.5, x0 + w + cw * 0.5):
        kit.trim_board(m, space, (x, 0.0, z0 - 0.02), (x, 0.0, z0 + h + cw), cw, paint)
    kit.trim_board(m, space, (x0 - cw - 0.02, 0.0, z0 + h + cw * 0.6), (x0 + w + cw + 0.02, 0.0, z0 + h + cw * 0.6),
                   cw * 1.2, paint)
    m.box((w + 2.0 * cw + 0.12, 0.09, 0.04), at=(x0 + w * 0.5, -0.045, z0 + h + cw * 1.2 + 0.02), uv='C', space=space)
    m.box((w + 2.0 * cw + 0.1, 0.1 + depth * 0.5, 0.06), at=(x0 + w * 0.5, -0.05 + depth * 0.25, z0 - 0.03),
          rot=(rng.uniform(-1.5, 1.5), 0.0, 0.0), uv='C', space=space)
    glass_y = depth * 0.55
    m.box((w, 0.02, h), at=(x0 + w * 0.5, glass_y, z0 + h * 0.5), uv=Trim('H4', fit=True),
          mat='windowglow' if lit else 'trim', space=space)
    cols, rows = o.opts.get('panes', (2, 2))
    for i in range(1, cols):
        x = x0 + w * i / cols
        m.board((x, glass_y - 0.03, z0), (x, glass_y - 0.03, z0 + h), 0.045, 0.04, uv=sash_paint, space=space)
    for j in range(1, rows):
        z = z0 + h * j / rows
        m.board((x0, glass_y - 0.035, z), (x0 + w, glass_y - 0.035, z), 0.045, 0.04, uv=sash_paint, space=space)
    if o.opts.get('style') != 'shutters':
        return
    shutter_paint = paint if paint != 'A' else 'H1'
    sw = w * 0.5 + 0.02
    for sign in (-1.0, 1.0):
        edge = x0 - cw if sign < 0 else x0 + w + cw
        shutter = Space(space.matrix @ place((edge + sign * sw * 0.5, -0.05, z0 + h * 0.5)))
        for k in range(3):
            x = -sw * 0.5 + sw * (k + 0.5) / 3
            m.board((x, 0.0, -h * 0.5 - 0.03), (x, 0.0, h * 0.5 + 0.03), sw / 3 - 0.008, 0.035, uv=shutter_paint,
                    space=shutter)
        m.board((-sw * 0.45, 0.0, -h * 0.28), (sw * 0.45, 0.0, h * 0.28), 0.09, 0.03, uv=shutter_paint, space=shutter,
                lift=0.03)
        # The catch: an iron dog pinned to the wall past the shutter's outer edge, turned in over its face.
        out = edge + sign * sw
        m.board((out + sign * 0.04, -0.006, z0 + 0.17), (out - sign * 0.025, -0.076, z0 + 0.16), 0.022, 0.012,
                uv=Trim('H3', fit=True), space=space)


class Cloth:
    """A curtain on the trim sheet's cream plaster strip G, which reads as old linen: U along the wall from a clean
    stretch of the strip (CREAM_U), V up the curtain fitted into the strip (its folds hide the stretch)."""

    def __init__(self, u0):
        self.u0 = u0

    def apply(self, obj, rng):
        mesh = obj.data
        v0, v1 = lt.TRIM_STRIPS['G']
        inset = 2.0 / lt.TRIM_SIZE
        scale = lt.TRIM_DENSITY / lt.TRIM_SIZE
        x_lo = min(v.co.x for v in mesh.vertices)
        z_lo = min(v.co.z for v in mesh.vertices)
        z_span = max(max(v.co.z for v in mesh.vertices) - z_lo, 1e-6)
        uv = mesh.uv_layers.active.data
        for loop in mesh.loops:
            co = mesh.vertices[loop.vertex_index].co
            uv[loop.index].uv = ((self.u0 + co.x - x_lo) * scale,
                                 v0 + inset + (co.z - z_lo) / z_span * (v1 - v0 - 2.0 * inset))


def sheet(m, rows, uv, mat='trim', space=None, facing=(0.0, -1.0, 0.0), smooth=True):
    """A one-sided cloth through a grid of points (rows of (x, y, z) in space), turned to face facing."""
    tb = kit._new_bmesh()
    grid = [[tb.verts.new(p) for p in row] for row in rows]
    for j in range(len(grid) - 1):
        for i in range(len(grid[j]) - 1):
            try:
                tb.faces.new((grid[j][i], grid[j][i + 1], grid[j + 1][i + 1], grid[j + 1][i]))
            except ValueError:
                pass
    tb.normal_update()
    if sum((f.normal * f.calc_area() for f in tb.faces), Vector()).dot(Vector(facing)) < 0.0:
        bmesh.ops.reverse_faces(tb, faces=tb.faces[:])
    m.emit(tb, uv, mat, None, space, smooth=smooth)


def curtains(m, space, o, depth):
    """Two linen curtains behind the sash bars, gathered at the head, tied back a little under half way and falling to
    the sill, drawn aside to leave the middle of the window clear. They hang a centimetre in front of the glass."""
    rng = m.rng
    x0, z0, w, h = o.x, o.z, o.w, o.h
    glass = depth * 0.55 - 0.01
    for side in (-1.0, 1.0):
        edge = x0 - 0.012 if side < 0 else x0 + w + 0.012    # tucked behind the reveal
        cover = w * rng.uniform(0.3, 0.36) + 0.012
        rows = []
        for z, span, fold in ((z0 + h - 0.004, cover, 0.016), (z0 + h * 0.44, cover * 0.42, 0.007),
                              (z0 + 0.012, cover * 0.68, 0.012)):
            rows.append([(edge - side * span * i / 4.0, glass - 0.009 - (fold if i % 2 else 0.0), z) for i in range(5)])
        sheet(m, rows, Cloth(rng.choice(CREAM_U)), space=space)


def plane(m, space, corner, x_dir, y_dir, size_x, size_y, uv, mat='trim', cuts=0, mirror=False):
    """A one-sided rectangle from corner, size_x along x_dir and size_y along y_dir (unit vectors in space), seen from
    the side x_dir x y_dir points to: the inside faces of the hall. Mapped as the part's own wall (x along, y up, from
    its middle); mirror runs its texture the other way along x (the part is built mirrored and the kit turns its faces
    back, so it faces the same way)."""
    x, y = Vector(x_dir), Vector(y_dir)
    frame = kit.basis(Vector(corner) + x * size_x * 0.5 + y * size_y * 0.5, -x if mirror else x, y.cross(x), y)
    m.emit(kit._box((size_x, 0.004, size_y), 0.0, cuts, ('+y', '-x', '+x', '-z', '+z')), uv, mat, frame, space)


def open_door(m, space, o, depth, angle=90.0):
    """kit.door's plank door turned on its hinge side through angle degrees, open inward against the hall's left wall,
    with its casing and threshold where they were; its pintles go (the screen door hangs in the frame now, on two hinges
    whose leaves are on the casing here)."""
    rng = m.rng
    x0, w, h = o.x, o.w, o.h
    paint = o.opts.get('paint', 'A')
    casing = o.opts.get('casing', 'A')
    y = depth * 0.3
    hinge = Vector((x0, y + 0.025, 0.0))              # the boards' inner face on the hinge side
    leaf = Space(space.matrix @ Matrix.Translation(hinge) @ Matrix.Rotation(math.radians(angle), 4, 'Z')
                 @ Matrix.Translation(-hinge))
    boards = max(3, int(round(w / 0.2)))
    bw = w / boards
    for k in range(boards):
        x = x0 + bw * (k + 0.5)
        m.board((x, y, 0.0), (x, y, h - rng.uniform(0.0, 0.03)), bw - 0.006, 0.05, uv=paint, space=leaf)
    for z in (0.3, h - 0.35):
        m.board((x0 + 0.06, y - 0.04, z), (x0 + w - 0.06, y - 0.04, z), 0.16, 0.035, uv=paint, space=leaf)
    m.board((x0 + w - 0.1, y - 0.04, 0.4), (x0 + 0.1, y - 0.04, h - 0.45), 0.13, 0.03, uv=paint, space=leaf)
    for z in (0.3, h - 0.35):
        m.board((x0 + 0.02, y - 0.065, z), (x0 + w * 0.72, y - 0.065, z), 0.07, 0.014, uv=Trim('H3', fit=True),
                space=leaf)
    m.box((0.05, 0.04, 0.16), at=(x0 + w - 0.14, y - 0.07, 1.0), uv=Trim('H3', fit=True), space=leaf)
    m.box((0.12, 0.03, 0.03), at=(x0 + w - 0.14, y - 0.1, 1.05), uv=Trim('H3', fit=True), space=leaf)
    cw = 0.13
    for x in (x0 - cw * 0.5, x0 + w + cw * 0.5):
        kit.trim_board(m, space, (x, 0.0, -0.02), (x, 0.0, h + cw), cw, casing)
    kit.trim_board(m, space, (x0 - cw - 0.03, 0.0, h + cw * 0.55), (x0 + w + cw + 0.03, 0.0, h + cw * 0.55), cw * 1.15,
                   casing)
    m.box((w + 0.06, depth * 0.5 + 0.08, 0.05), at=(x0 + w * 0.5, depth * 0.25 - 0.04, -0.005), uv='C', space=space)
    for z in SCREEN_HINGES:
        m.box((0.065, 0.006, 0.085), at=(x0 - 0.035, -0.038, SCREEN_FOOT + z), uv=Trim('H3', fit=True), space=space)


def lathe(m, profile, at=(0.0, 0.0, 0.0), sides=8, uv=None, mat='trim', space=None, axis=None, smooth=True):
    """A turned part around a vertical axis through at: profile [(radius, z), ...] in order (a radius of 0 closes an
    end). axis (a matrix) turns it. Corners that should stay sharp are separate calls."""
    tb = kit._new_bmesh()
    rings = []
    for r, z in profile:
        if r <= 1e-6:
            rings.append([tb.verts.new((0.0, 0.0, z))])
        else:
            rings.append([tb.verts.new((r * math.cos(2.0 * math.pi * k / sides),
                                        r * math.sin(2.0 * math.pi * k / sides), z)) for k in range(sides)])
    for r0, r1 in zip(rings, rings[1:]):
        for k in range(sides):
            k1 = (k + 1) % sides
            if len(r0) == 1 and len(r1) == 1:
                continue
            if len(r0) == 1:
                face = (r0[0], r1[k1], r1[k])
            elif len(r1) == 1:
                face = (r0[k], r0[k1], r1[0])
            else:
                face = (r0[k], r0[k1], r1[k1], r1[k])
            try:
                tb.faces.new(face)
            except ValueError:
                pass
    bmesh.ops.recalc_face_normals(tb, faces=tb.faces[:])
    matrix = Matrix.Translation(Vector(at)) @ (axis if axis is not None else Matrix.Identity(4))
    m.emit(tb, uv, mat, matrix, space, smooth=smooth)


def tube(m, points, radius, sides=4, uv=None, mat='trim', space=None):
    """A thin round rod along a polyline (a pail's bail, a cup's handle): one open cylinder per stretch."""
    for p0, p1 in zip(points, points[1:]):
        m.cylinder(p0, p1, radius, sides=sides, uv=uv, mat=mat, space=space, caps=(False, False))


def hall(m, front):
    """The shallow hall behind the screen door, built only as far as it shows through the door: plaster walls (the
    strip G, as in Tilly's window room) over a board wainscot, a board floor and ceiling, a runner down the middle to
    a lamp on a side table against the end wall on the door's axis (LanternGlow), and on the left wall past the open
    door Delia's black shawl on a peg and the hall mirror covered in crepe. The planes are sliced for their baked
    shading. Returns the lamp globe's middle (SOCKET_Light)."""
    s0, s1, t0, t1, top = HALL_S0, HALL_S1, HALL_T0, HALL_T1, HALL_TOP
    boards = Trim('A', world=True)
    up, along, back, left, right = (0.0, 0.0, 1.0), (0.0, 1.0, 0.0), (0.0, -1.0, 0.0), (1.0, 0.0, 0.0), (-1.0, 0.0, 0.0)
    plane(m, front, (s0, t0, -0.01), along, up, t1 - t0 + 0.01, top + 0.02, Trim('G', world=True, u=PLASTER_U['left']),
          cuts=2)
    # The right and end walls in two, folded at PLASTER_FOLD: the first part's plaster runs up to the fold, the second
    # part's (mirrored) back down from it.
    fold, u = PLASTER_FOLD['right'], PLASTER_U['right']
    for start, length, mirror in ((t1 + 0.01, t1 + 0.01 - fold, False), (fold, fold - t0, True)):
        plane(m, front, (s1, start, -0.01), back, up, length, top + 0.02, Trim('G', world=True, u=u - length * 0.5),
              cuts=1, mirror=mirror)
    fold, u = PLASTER_FOLD['end'], PLASTER_U['end']
    for start, length, mirror in ((s0 - 0.01, fold - s0 + 0.01, False), (fold, s1 + 0.01 - fold, True)):
        plane(m, front, (start, t1, -0.01), left, up, length, top + 0.02, Trim('G', world=True, u=u - length * 0.5),
              cuts=1, mirror=mirror)
    # The floor and the ceiling.
    plane(m, front, (s1 + 0.01, t0 - 0.08, 0.002), along, right, t1 - t0 + 0.09, s1 - s0 + 0.02, boards, cuts=2)
    plane(m, front, (s0 - 0.01, t0, top), along, left, t1 - t0 + 0.01, s1 - s0 + 0.02, boards, cuts=2)
    wall = Space(front.matrix @ Matrix.Translation((0.0, t1, 0.0)))     # the end wall's face, as a wall facing out
    # The runner: faded oxide red (H2) with cream and teal bands across it near its ends (the cream from the plaster
    # strip's middle, clear of its edge).
    m.box((1.5, 0.52, 0.008), at=(DOOR_X + DOOR_W * 0.5, 1.08, 0.006), rot=(0.0, 0.0, 92.0),
          uv=Trim('H2', fit=True, u=2.55), space=front, drop=('-z',))
    for k, t in enumerate((0.45, 0.55, 1.61, 1.71)):
        x = DOOR_X + DOOR_W * 0.5 + (t - 1.08) * math.cos(math.radians(92.0))
        m.box((0.524, 0.05 if k in (0, 3) else 0.025, 0.009), at=(x, t, 0.0065), rot=(0.0, 0.0, 2.0),
              uv=Trim('G', fit=True, v=0.3) if k in (0, 3) else Trim('H1', fit=True), space=front, drop=('-z',))
    # The side table and the lamp on it: a dark iron font (H3) and a glowing chimney.
    lx, ly = LAMP
    table_top = 0.76
    m.box((0.34, 0.3, 0.03), at=(lx, ly, table_top - 0.015), uv='C', space=wall)
    for dx in (-0.14, 0.14):
        for dy in (-0.115, 0.115):
            m.board((lx + dx, ly + dy, 0.0), (lx + dx, ly + dy, table_top - 0.03), 0.035, 0.035, uv='C', space=wall)
    lathe(m, [(0.0, 0.0), (0.075, 0.0), (0.075, 0.02), (0.03, 0.05), (0.08, 0.12), (0.035, 0.17), (0.0, 0.17)],
          at=(lx - 0.02, ly + 0.02, table_top), uv=Trim('H3', fit=True), space=wall)
    globe = (lx - 0.02, ly + 0.02, table_top + 0.17)
    lathe(m, [(0.0, 0.0), (0.04, 0.0), (0.09, 0.07), (0.085, 0.15), (0.04, 0.22), (0.035, 0.3), (0.0, 0.3)],
          at=globe, uv=Trim('H4', fit=True), mat='glow', space=wall)
    # Delia's shawl on the middle of three pegs on a rail past the open door (SHAWL_T): black wool hung by its middle,
    # falling in folds that deepen toward its ragged hem.
    side = Space(front.matrix @ kit.basis((s0, 0.0, 0.0), (0.0, 1.0, 0.0), (-1.0, 0.0, 0.0), (0.0, 0.0, 1.0)))
    kit.trim_board(m, side, (SHAWL_T - 0.16, 0.0, 1.72), (SHAWL_T + 0.16, 0.0, 1.72), 0.085, 'C', thick=0.025)
    for t in (SHAWL_T - 0.11, SHAWL_T, SHAWL_T + 0.11):
        m.board((t, -0.02, 1.71), (t, -0.1, 1.735), 0.024, 0.024, uv='C', space=side)
    rows = []
    for z, half, out in ((1.755, 0.03, 0.1), (1.66, 0.11, 0.078), (1.45, 0.16, 0.052), (1.2, 0.17, 0.042),
                         (0.98, 0.155, 0.036)):
        depth = 0.024 * min(1.0, (1.755 - z) / 0.25)
        rows.append([(SHAWL_T - half + 2.0 * half * i / 6.0, -(out + (depth if i % 2 else 0.0)),
                      z + (0.03 * math.sin(i * 1.9 + 0.4) if z < 1.0 else 0.0)) for i in range(7)])
    sheet(m, rows, Tile('Polymer'), mat='crepe', space=side)
    # The wainscot: upright boards 1.2 cm proud of the plaster (on the left wall behind the open door too, which stands
    # off the wall by that much), a cap rail along their top, mitred into the corners.
    upright = Trim('C', world=True, rotate=True)
    plane(m, front, (s0 + 0.012, t0, -0.01), along, up, t1 - t0 + 0.01, WAINSCOT + 0.01, upright, cuts=2)
    plane(m, front, (s1 - 0.012, t1 + 0.01, -0.01), back, up, t1 - t0 + 0.01, WAINSCOT + 0.01, upright, cuts=2)
    plane(m, front, (s0 + 0.012, t1 - 0.012, -0.01), left, up, s1 - s0 - 0.024, WAINSCOT + 0.01, upright, cuts=1)
    inset = 0.014 + 0.011
    for p0, p1, face in (((s0 + inset, t0, WAINSCOT), (s0 + inset, t1 - 0.014, WAINSCOT), left),
                         ((s1 - inset, t1 - 0.014, WAINSCOT), (s1 - inset, t0, WAINSCOT), right),
                         ((s0 + 0.036, t1 - inset, WAINSCOT), (s1 - 0.036, t1 - inset, WAINSCOT), back)):
        length = (Vector(p1) - Vector(p0)).length
        m.box((length, 0.022, 0.045), matrix=kit.toward(p0, p1, face) @ Matrix.Translation((length * 0.5, 0.0, 0.0)),
              uv='C', space=front, drop=('+y', '-x', '+x', '-z'))
    covered_mirror(m, front)
    return wall.world((globe[0], globe[1], globe[2] + 0.11))


def covered_mirror(m, front):
    """The hall mirror on the left wall (MIRROR) under black crepe: the cloth laid over the frame's top falls in folds
    that deepen toward its hem, wrapping the frame's sides, and the frame's bottom rail shows under it."""
    a, b, z0, z1 = MIRROR
    width = b - a
    # The left wall's face: x from the frame's front edge toward the back, z up from the floor, the hall toward -y.
    wall = Space(front.matrix @ kit.basis((HALL_S0 + 0.002, a, 0.0), (0.0, 1.0, 0.0), (-1.0, 0.0, 0.0),
                                          (0.0, 0.0, 1.0)))
    m.box((width, 0.03, 0.05), at=(width * 0.5, -0.015, z0 + 0.025), uv='C', space=wall, drop=('+y', '+z'))
    rows = []
    for k, (z, out, fold, spread) in enumerate(((z1 + 0.012, 0.004, 0.0, 0.0), (z1 + 0.006, 0.036, 0.0, 0.006),
                                                (z1 - 0.22, 0.04, 0.006, 0.014), (z0 + 0.3, 0.042, 0.012, 0.02),
                                                (z0 + 0.035, 0.044, 0.018, 0.026))):
        row = []
        for i in range(7):
            edge = i in (0, 6) and k > 0
            sag = 0.012 * math.sin(math.pi * i / 6.0) if k == 4 else 0.0
            row.append((-spread + (width + 2.0 * spread) * i / 6.0,
                        -0.012 if edge else -(out + (fold if i % 2 else 0.0)), z - sag))
        rows.append(row)
    sheet(m, rows, Tile('Polymer'), mat='crepe', space=wall)


def crepe(m, front):
    """Black crepe over the door: a swag across the head casing between two rosettes, bellying out in folds, and two
    ribbon tails hanging from each rosette down the side casings. The swag's lowest fold clears the screen door's head
    as it swings."""
    rng = m.rng
    a, b = DOOR_X - 0.16, DOOR_X + DOOR_W + 0.16
    top = DOOR_H + 0.13 * 0.55 + 0.13 * 1.15 * 0.5 + 0.004     # the head casing's top edge
    rows = []
    for j in range(4):
        row = []
        for i in range(11):
            u, v = i / 10.0, j / 3.0
            belly = math.sin(math.pi * u)
            z0, z1 = top - 0.012 - 0.025 * belly, top - 0.055 - 0.085 * belly
            fold = 0.008 * math.sin(2.0 * math.pi * (2.5 * v + 0.15)) * belly
            row.append((a + (b - a) * u, -0.045 - 0.04 * belly * math.sin(math.pi * (0.2 + 0.8 * v)) - fold,
                        z0 + (z1 - z0) * v))
        rows.append(row)
    sheet(m, rows, Tile('Polymer'), mat='crepe', space=front)
    rosette = [(0.0, 0.016), (0.03, 0.013), (0.056, 0.004), (0.062, -0.004), (0.0, -0.004)]
    for x in (a, b):
        lathe(m, rosette, at=(x, -0.05, top - 0.035), uv=Tile('Polymer'), mat='crepe', space=front,
              axis=Matrix.Rotation(math.radians(90.0), 4, 'X'))
        for k, dx in enumerate((-0.022, 0.026)):
            end = top - 0.47 - 0.07 * k - rng.uniform(0.0, 0.03)
            rows = []
            for j in range(4):
                v = j / 3.0
                z = top - 0.06 + (end - (top - 0.06)) * v
                xc = x + dx * (1.0 + 0.6 * v) + 0.006 * math.sin(v * 5.0 + k)
                y = -0.043 - 0.004 * math.sin(v * 4.0 + k)
                rows.append([(xc - 0.027, y, z), (xc + 0.027, y - 0.002, z)])
            sheet(m, [[r[0] for r in rows], [r[1] for r in rows]], Tile('Polymer'), mat='crepe', space=front)


def rocking_chair(m, at, yaw):
    """A ladder-back rocking chair in the posts' weathered beam wood (C) with a plank seat (A): rockers, legs, back
    posts leaning back, four slats, arms. Its seat's middle stands over at, facing -Y turned yaw degrees."""
    s = Space(Matrix.Translation(Vector(at)) @ Matrix.Rotation(math.radians(yaw), 4, 'Z'))

    def rock(y):
        return y * y / 2.6               # the rockers' curve (1.3 m radius)
    for x in (-0.235, 0.235):
        ys = [-0.47 + 0.9 * k / 4.0 for k in range(5)]
        outline = [(y, rock(y)) for y in ys] + [(y, rock(y) + 0.055) for y in reversed(ys)]
        # Drawn in the chair's YZ plane: the prism's X turned onto the chair's Y, its thickness across.
        m.emit(kit._prism([outline], 0.032), Trim('C', width=0.15), 'trim', place((x + 0.016, 0.0, 0.0),
                                                                                   (0.0, 0.0, 90.0)), s)
    for x in (-0.215, 0.215):
        m.board((x, -0.17, rock(-0.17) + 0.03), (x, -0.17, 0.64), 0.04, 0.04, uv='C', space=s)
        m.board((x, 0.19, rock(0.19) + 0.03), (x, 0.28, 1.07), 0.045, 0.045, uv='C', space=s)
        arm_x = x + (0.02 if x > 0 else -0.02)
        m.board((arm_x, 0.25, 0.655), (arm_x, -0.28, 0.655), 0.06, 0.025, face=(0.0, 0.0, 1.0), uv='C', space=s)
        m.board((x, -0.17, 0.19), (x, 0.2, 0.19), 0.03, 0.03, uv='C', space=s)
    m.board((-0.215, -0.17, 0.21), (0.215, -0.17, 0.21), 0.03, 0.03, uv='C', space=s)
    m.box((0.5, 0.44, 0.035), at=(0.0, 0.0, 0.43), rot=(-3.0, 0.0, 0.0), uv='A', space=s)
    for k, z in enumerate((0.62, 0.75, 0.88, 1.0)):
        y = 0.19 + (z - 0.04) * 0.09 / 1.03
        m.board((-0.215, y, z), (0.215, y, z), 0.065 if k < 3 else 0.085, 0.02, uv='C', space=s)
    m.hull((0.6, 0.96, 1.08), at=(0.0, -0.01, 0.54), space=s)


def stool(m, at):
    """A plank stool against the wall: a thick seat on two slab legs with a notched foot, and a stretcher."""
    s = Space(Matrix.Translation(Vector(at)))
    top = STOOL_TOP
    m.box((0.46, 0.32, 0.045), at=(0.0, 0.0, top - 0.0225), uv='C', space=s)
    leg = [(-0.135, 0.0), (-0.06, 0.0), (0.0, 0.075), (0.06, 0.0), (0.135, 0.0), (0.115, top - 0.045),
           (-0.115, top - 0.045)]
    for x in (-0.16, 0.16):
        m.emit(kit._prism([leg], 0.038), Trim('C', width=top), 'trim', place((x + 0.019, 0.0, 0.0), (0.0, 0.0, 90.0)),
               s)
    m.box((0.3, 0.025, 0.07), at=(0.0, 0.0, top - 0.17), uv='C', space=s)
    m.hull((0.46, 0.32, top), at=(0.0, 0.0, top * 0.5), space=s)


def broom(m, foot):
    """A corn broom leaning on the wall: a straw head (the plaster strip's pale cream) stitched with two red bands (H2)
    on a long handle. foot is the middle of its bristles' edge on the floor; it leans back until it touches the wall."""
    length = 1.42
    lean = math.degrees(math.asin(min(0.4, (Y0 - 0.012 - foot[1]) / length)))
    s = Space(Matrix.Translation(Vector(foot)) @ Matrix.Rotation(math.radians(14.0), 4, 'Z')
              @ Matrix.Rotation(math.radians(-lean), 4, 'X'))
    head = [(-0.15, 0.0), (0.15, 0.0), (0.14, 0.1), (0.07, 0.3), (-0.07, 0.3), (-0.14, 0.1)]
    m.emit(kit._prism([head], 0.05), Trim('G'), 'trim', place((0.0, -0.025, 0.0)), s)
    for z in (0.205, 0.255):
        half = 0.14 - (z - 0.1) / 0.2 * 0.07
        m.box((half * 2.0 + 0.006, 0.054, 0.018), at=(0.0, 0.0, z), uv=Trim('H2', fit=True), space=s)
    m.cylinder((0.0, 0.0, 0.27), (0.0, 0.0, length), 0.0135, sides=6, uv='C', space=s)


def pail(m, at):
    """A tin pail in MetalRust's pale enamel and rust, its wire bail standing up, leaning back a little."""
    s = Space(Matrix.Translation(Vector(at)))
    tin = Tile('MetalRust')
    lathe(m, [(0.118, 0.0), (0.146, 0.27)], sides=10, uv=tin, mat='metal', space=s)
    lathe(m, [(0.146, 0.27), (0.153, 0.277), (0.14, 0.281)], sides=10, uv=tin, mat='metal', space=s)
    lathe(m, [(0.14, 0.281), (0.116, 0.035), (0.0, 0.035)], sides=10, uv=tin, mat='metal', space=s)
    for x in (-0.149, 0.149):
        m.box((0.012, 0.03, 0.04), at=(x, 0.0, 0.25), uv=tin, mat='metal', space=s)
    turn = Matrix.Rotation(math.radians(-22.0), 4, 'X')
    bail = [turn @ Vector((0.152 * math.cos(math.radians(a)), 0.0, 0.25 + 0.16 * math.sin(math.radians(a))))
            for a in (180.0, 135.0, 90.0, 45.0, 0.0)]
    tube(m, bail, 0.0045, uv=tin, mat='metal', space=s)


# --- SM_ScreenDoor ---

def screen_material():
    """ScreenMesh: insect screen on the Glass master (M_Glass: unlit and translucent), a dark grey-green mesh half
    see-through face on (Opacity) and nearly solid at a glance (EdgeOpacity), darker there (RimBrightness under 1), so
    the lamp and the hall show through it and it never goes milky; no texture. Its nodes only stand in for M_Glass in
    the previews: the same sums (Fresnel (1 - N.V)^3; emissive Tint x lerp(1, RimBrightness, Fresnel); opacity
    lerp(Opacity, EdgeOpacity, Fresnel)), and like a translucent surface in the game it casts no shadow."""
    bpy = kit.bpy
    tint, opacity, edge, rim = SCREEN_TINT, SCREEN_OPACITY, SCREEN_EDGE_OPACITY, SCREEN_RIM
    mat = bpy.data.materials.get('ScreenMesh') or bpy.data.materials.new('ScreenMesh')
    mat.use_nodes = True
    nodes, links = mat.node_tree.nodes, mat.node_tree.links
    nodes.clear()
    out = nodes.new('ShaderNodeOutputMaterial')
    geometry = nodes.new('ShaderNodeNewGeometry')
    dot = nodes.new('ShaderNodeVectorMath')
    dot.operation = 'DOT_PRODUCT'
    links.new(geometry.outputs['Normal'], dot.inputs[0])
    links.new(geometry.outputs['Incoming'], dot.inputs[1])
    facing = nodes.new('ShaderNodeMath')
    facing.operation = 'ABSOLUTE'
    links.new(dot.outputs['Value'], facing.inputs[0])
    grazing = nodes.new('ShaderNodeMath')
    grazing.operation = 'SUBTRACT'
    grazing.inputs[0].default_value = 1.0
    links.new(facing.outputs['Value'], grazing.inputs[1])
    fresnel = nodes.new('ShaderNodeMath')                 # M_Glass's Fresnel: (1 - N.V)^3
    fresnel.operation = 'POWER'
    links.new(grazing.outputs['Value'], fresnel.inputs[0])
    fresnel.inputs[1].default_value = 3.0
    alpha = nodes.new('ShaderNodeMapRange')
    links.new(fresnel.outputs['Value'], alpha.inputs['Value'])
    alpha.inputs['To Min'].default_value, alpha.inputs['To Max'].default_value = opacity, edge
    bright = nodes.new('ShaderNodeMapRange')
    links.new(fresnel.outputs['Value'], bright.inputs['Value'])
    bright.inputs['To Min'].default_value, bright.inputs['To Max'].default_value = 1.0, rim
    glow = nodes.new('ShaderNodeEmission')
    glow.inputs['Color'].default_value = lm.hex_color(tint)
    links.new(bright.outputs['Result'], glow.inputs['Strength'])
    clear = nodes.new('ShaderNodeBsdfTransparent')
    mix = nodes.new('ShaderNodeMixShader')
    links.new(alpha.outputs['Result'], mix.inputs['Fac'])
    links.new(clear.outputs['BSDF'], mix.inputs[1])
    links.new(glow.outputs['Emission'], mix.inputs[2])
    # Shadow rays see straight through it.
    path = nodes.new('ShaderNodeLightPath')
    unshadowed = nodes.new('ShaderNodeMixShader')
    links.new(path.outputs['Is Shadow Ray'], unshadowed.inputs['Fac'])
    links.new(mix.outputs['Shader'], unshadowed.inputs[1])
    links.new(clear.outputs['BSDF'], unshadowed.inputs[2])
    links.new(unshadowed.outputs['Shader'], out.inputs['Surface'])
    mat.diffuse_color = lm.hex_color(tint)[:3] + [opacity]
    if hasattr(mat, 'surface_render_method'):
        mat.surface_render_method = 'BLENDED'
    if hasattr(mat, 'use_transparent_shadow'):
        mat.use_transparent_shadow = True
    mat.use_backface_culling = True
    for key in [k for k in mat.keys() if not k.startswith('_')]:
        del mat[key]
    mat['Master'] = 'Glass'
    mat['Tint'] = '#{:06X}'.format(tint)
    mat['Opacity'] = opacity
    mat['EdgeOpacity'] = edge
    mat['RimBrightness'] = rim
    return mat


class ScreenModel(kit.Model):
    """A kit Model with the screen's material ('screen')."""

    def slot(self, key):
        if key not in self.slots:
            self.slots[key] = (len(self.slots), screen_material() if key == 'screen' else kit._material(key))
        return self.slots[key][0]


class Flat:
    """UVs straight from a part's X and Z in meters: something to give the untextured screen real tangents."""

    def apply(self, obj, rng):
        mesh = obj.data
        uv = mesh.uv_layers.active.data
        for loop in mesh.loops:
            co = mesh.vertices[loop.vertex_index].co
            uv[loop.index].uv = (co.x, co.z)


def screen_door():
    """SM_ScreenDoor (see the module's docstring): built around its pivot, the hinge line at its foot on its front face;
    it reaches along +X and its front faces -Y."""
    s = ScreenModel('ScreenDoor', seed=73)
    w, h, t = SCREEN_W, SCREEN_H, SCREEN_T
    y = t * 0.5
    stile = 0.09
    iron = Trim('H3', fit=True)
    for x in (stile * 0.5, w - stile * 0.5):
        s.board((x, y, 0.0), (x, y, h), stile, t, uv='H1')
    for z0, z1 in list(SCREEN_RAILS) + [(h - 0.09, h)]:
        z = (z0 + z1) * 0.5
        s.board((stile - 0.005, y, z), (w - stile + 0.005, y, z), z1 - z0, t - 0.004, uv='H1')
    # The kick panel: four upright boards between the bottom and kick rails (each within the paint strip's width).
    k0, k1 = SCREEN_RAILS[0][1], SCREEN_RAILS[1][0]
    bw = (w - 2.0 * stile) / 4.0
    for k in range(4):
        x = stile + bw * (k + 0.5)
        s.board((x, y, k0 - 0.01), (x, y, k1 + 0.01), bw - 0.006, 0.016, uv='H1')
    # The screens, run into the frame's grooves; a brace across the lower one from the hinge side's foot up to the
    # latch side, where the door would sag.
    for z0, z1 in ((SCREEN_RAILS[1][1], SCREEN_RAILS[2][0]), (SCREEN_RAILS[2][1], h - 0.09)):
        tb = kit._new_bmesh()
        x0, x1 = stile - 0.012, w - stile + 0.012
        corners = [tb.verts.new((x, y + 0.002, z))
                   for x, z in ((x0, z0 - 0.012), (x1, z0 - 0.012), (x1, z1 + 0.012), (x0, z1 + 0.012))]
        face = tb.faces.new(corners)
        face.normal_update()
        if face.normal.y > 0.0:
            face.normal_flip()
        s.emit(tb, Flat(), 'screen')
    lz0, lz1 = SCREEN_RAILS[1][1], SCREEN_RAILS[2][0]
    s.board((stile - 0.01, y, lz0 + 0.03), (w - stile + 0.01, y, lz1 - 0.03), 0.055, 0.022, uv='H1')
    # Two hinges: a knuckle on the hinge line and a leaf on the stile (their other leaves are on the house's casing).
    for z in SCREEN_HINGES:
        s.cylinder((0.0, 0.0, z - 0.045), (0.0, 0.0, z + 0.045), 0.011, sides=6, uv=iron)
        s.box((0.055, 0.005, 0.085), at=(0.03, -0.0025, z), uv=iron)
    # The pull on the latch stile, at the push rail.
    pull_z = sum(SCREEN_RAILS[2]) * 0.5
    for dz in (-0.07, 0.07):
        s.box((0.016, 0.03, 0.016), at=(w - 0.045, -0.015, pull_z + dz), uv=iron)
    s.box((0.018, 0.016, 0.17), at=(w - 0.045, -0.034, pull_z), uv=iron)
    # The door spring on the inside, from the hinge stile down to the push rail, with its hooks.
    p0, p1 = (0.05, t + 0.012, 1.17), (0.4, t + 0.012, SCREEN_RAILS[2][1] - 0.015)
    s.cylinder(p0, p1, 0.009, sides=6, uv=iron)
    for p in (p0, p1):
        s.box((0.02, 0.02, 0.012), at=(p[0], t + 0.006, p[2]), uv=iron)
    return s.finish(preview=False, Nanite=0, LODs='50,25', Collision='None')


# --- SM_PorchPlate ---

class Checks:
    """A cloth laid out flat on a grid of square cells from (x0, y0): each cell its own patch of the trim sheet at world
    scale, cream plaster (G) and faded oxide-red paint (H2) in turn, a gingham check."""

    def __init__(self, x0, y0, cell):
        self.x0, self.y0, self.cell = x0, y0, cell

    def apply(self, obj, rng):
        mesh = obj.data
        uv = mesh.uv_layers.active.data
        scale = lt.TRIM_DENSITY / lt.TRIM_SIZE
        for p in mesh.polygons:
            i = int(math.floor((p.center.x - self.x0) / self.cell))
            j = int(math.floor((p.center.y - self.y0) / self.cell))
            red = (i + j) % 2 == 1
            v_lo = lt.TRIM_STRIPS['H2' if red else 'G'][0] + (0.07 if red else 0.3) * scale
            spots = RED_U if red else CREAM_U
            u0 = spots[(i * 7 + j * 3) % len(spots)]
            for li in p.loop_indices:
                co = mesh.vertices[mesh.loops[li].vertex_index].co
                uv[li].uv = ((u0 + co.x - (self.x0 + i * self.cell)) * scale,
                             v_lo + (co.y - (self.y0 + j * self.cell)) * scale)


def porch_plate():
    """SM_PorchPlate (see the module's docstring): built around its pivot, the middle of its foot."""
    p = kit.Model('PorchPlate', seed=79)
    rng = p.rng
    tin = Tile('MetalRust')
    plate = (-0.02, 0.0)
    # The cloth over the plate: a check of six by six cells laid flat, then draped over a mound of food and the plate's
    # rim (12 cm out) down onto the seat all round. The plate itself is never seen, so it isn't built: under a drape
    # this coarse, its rim came through the cloth between the grid's points.
    n, half = 6, 0.15
    cell = 2.0 * half / n
    tb = kit._new_bmesh()
    grid = [[tb.verts.new((-half + cell * i, -half + cell * j, 0.0)) for i in range(n + 1)] for j in range(n + 1)]
    for j in range(n):
        for i in range(n):
            tb.faces.new((grid[j][i], grid[j][i + 1], grid[j + 1][i + 1], grid[j + 1][i]))
    ripple = [rng.uniform(0.0, 6.28) for _ in range(3)]

    def drape(co):
        r = math.hypot(co.x, co.y)
        if r < 0.06:
            z = 0.06 - 4.5 * r * r
        elif r < 0.122:
            z = 0.0438 + (r - 0.06) / 0.062 * (0.027 - 0.0438)
        else:
            z = max(0.004, 0.027 - (r - 0.122) * 0.95)
        z += 0.003 * math.sin(co.x * 41.0 + ripple[0]) * math.sin(co.y * 37.0 + ripple[1]) * min(1.0, r / 0.08)
        return Vector((co.x, co.y, z))
    p.emit(tb, Checks(-half, -half, cell), 'trim', place((plate[0], plate[1], 0.0), (0.0, 0.0, 12.0)), smooth=True,
           shape=drape)
    # A three-tined fork lying beside it (iron, H3) along its left edge, and a tin cup to the right of it, both on the
    # seat clear of the cloth (whose edges ripple on the seat).
    fork = [(-0.009, 0.0), (0.009, 0.0), (0.008, 0.1), (0.013, 0.125), (0.013, 0.185), (0.0085, 0.185), (0.0075, 0.138),
            (0.0025, 0.138), (0.002, 0.185), (-0.002, 0.185), (-0.0025, 0.138), (-0.0075, 0.138), (-0.0085, 0.185),
            (-0.013, 0.185), (-0.013, 0.125), (-0.008, 0.1)]
    p.emit(kit._prism([fork], 0.004), Trim('H3', fit=True), 'trim',
           Matrix.Translation((-0.173, -0.135, 0.0045)) @ Matrix.Rotation(math.radians(12.0), 4, 'Z')
           @ Matrix.Rotation(math.radians(-90.0), 4, 'X'))
    cup = (0.162, 0.07)
    lathe(p, [(0.036, 0.0), (0.041, 0.085)], at=(cup[0], cup[1], 0.0), uv=tin, mat='metal')
    lathe(p, [(0.041, 0.085), (0.0445, 0.0885), (0.038, 0.089)], at=(cup[0], cup[1], 0.0), uv=tin, mat='metal')
    lathe(p, [(0.038, 0.089), (0.035, 0.012), (0.0, 0.012)], at=(cup[0], cup[1], 0.0), uv=tin, mat='metal')
    handle = [Vector((cup[0] + x, cup[1], z)) for x, z in ((0.04, 0.07), (0.063, 0.067), (0.067, 0.045), (0.06, 0.023),
                                                            (0.039, 0.02))]
    tube(p, handle, 0.0035, uv=tin, mat='metal')
    return p.finish(preview=False, Nanite=0, LODs='50,25', Collision='None')


# --- The look sheet (--preview) ---

PREVIEW_DIR = os.path.join(lt.PREVIEW_DIR, 'RansomsRest', 'Farmhouse')
HOUSE_YAW = 90.0     # Ransom's Rest's layout turns the house so its porch faces east


def bearing(azimuth, elevation, yaw=HOUSE_YAW):
    """Toward a sun at a compass bearing (degrees from north toward east) and elevation, in the house's own frame as
    the level turns it (yaw in Unreal's degrees; Unreal's +X is north and +Y east, and the house's front, Blender's -Y,
    is its +X)."""
    a, e, t = math.radians(azimuth), math.radians(elevation), math.radians(yaw)
    north, east = math.cos(a) * math.cos(e), math.sin(a) * math.cos(e)
    x, y = north * math.cos(t) + east * math.sin(t), -north * math.sin(t) + east * math.cos(t)
    return (-y, -x, math.sin(e))


# The level's light (Art/Levels/RansomsRest/layout.json, "environment"): the golden late afternoon's sun at 247.5
# degrees, west-south-west, 15 degrees up, which the posters' renders paint (looter_posters: their sky, the sun's
# strength and warmth). It stands behind the house and a little to its left, so the porch is in the shade all
# afternoon and the low sun rakes the south gable.
GOLDEN_SUN = bearing(247.5, 15.0)
GOLDEN_SKY = [(0.0, 0x6b6050), (0.47, 0x8e8270), (0.5, 0xe9d2a6), (0.53, 0xd9d6c4), (0.62, 0xa9bfd2), (1.0, 0x5d84b6)]
GOLDEN_GLOWS = [(0xffe0a8, 24.0, 0.9), (0xfff2d0, 600.0, 6.0)]
# The game's Dusk state (layout.json's "Dusk": the sun at 252 degrees, 4 up), as Sexton's renders paint it
# (Art/Models/Characters/MisterSexton.py): glowing amber toward the sun, the haze cooling to violet away from it. The
# sun is behind the house, and its lamps and windows carry the front.
DUSK_SUN = bearing(252.0, 4.0)
DUSK_SKY = [(0.0, 0x1a1418), (0.47, 0x2e2630), (0.5, 0x7c7290), (0.512, 0x6e6478), (0.56, 0x54484e), (0.7, 0x3e3438),
            (1.0, 0x2a2428)]
DUSK_GLOWS = [(0xff9a48, 6.0, 0.5), (0xffc27a, 60.0, 1.3), (0xfff2dc, 1400.0, 40.0)]
LAMPLIGHT = (1.0, 0.72, 0.45)
# The point lights at SOCKET_Light (the hall lamp) and SOCKET_Light_Porch (the lantern), in Blender's watts: by day the
# hall lamp only warms the hall behind the screen; at dusk both are up and carry the front, with the windows' Glow (the
# hall lamp twice as bright, so the right wall, 1.15 m from it, fills the screen seen from the path with its glow).
LAMPS_DAY, LAMPS_DUSK = (12.0, 8.0), (24.0, 12.0)
# The renders' exposure in the golden light: the porch faces away from the low sun and stands in the house's shade, as
# the game's auto exposure sees it there.
SHADE_EXPOSURE = 0.5


def sky(name, stops, sun, glows):
    """A painted sky (the posters' and Sexton's): colours by the view's height, glows round the sun."""
    bpy = kit.bpy
    world = bpy.data.worlds.get(name) or bpy.data.worlds.new(name)
    world.use_nodes = True
    nodes, links = world.node_tree.nodes, world.node_tree.links
    nodes.clear()
    out = nodes.new('ShaderNodeOutputWorld')
    background = nodes.new('ShaderNodeBackground')
    coords = nodes.new('ShaderNodeTexCoord')
    split = nodes.new('ShaderNodeSeparateXYZ')
    links.new(coords.outputs['Generated'], split.inputs['Vector'])
    remap = nodes.new('ShaderNodeMapRange')
    remap.inputs['From Min'].default_value = -1.0
    links.new(split.outputs['Z'], remap.inputs['Value'])
    ramp = nodes.new('ShaderNodeValToRGB')
    links.new(remap.outputs['Result'], ramp.inputs['Fac'])
    elements = ramp.color_ramp.elements
    elements[0].position, elements[0].color = stops[0][0], lt.hex_color(stops[0][1])
    elements[1].position, elements[1].color = stops[-1][0], lt.hex_color(stops[-1][1])
    for position, color in stops[1:-1]:
        elements.new(position).color = lt.hex_color(color)
    color = ramp.outputs['Color']
    dot = nodes.new('ShaderNodeVectorMath')
    dot.operation = 'DOT_PRODUCT'
    links.new(coords.outputs['Generated'], dot.inputs[0])
    dot.inputs[1].default_value = Vector(sun).normalized()
    clamp = nodes.new('ShaderNodeMath')
    clamp.operation = 'MAXIMUM'
    links.new(dot.outputs['Value'], clamp.inputs[0])
    for glow, power, strength in glows:
        lift = nodes.new('ShaderNodeMath')
        lift.operation = 'POWER'
        links.new(clamp.outputs['Value'], lift.inputs[0])
        lift.inputs[1].default_value = power
        scale = nodes.new('ShaderNodeMath')
        scale.operation = 'MULTIPLY'
        links.new(lift.outputs['Value'], scale.inputs[0])
        scale.inputs[1].default_value = strength
        add = nodes.new('ShaderNodeMix')
        add.data_type = 'RGBA'
        add.blend_type = 'ADD'
        links.new(scale.outputs['Value'], add.inputs['Factor'])
        links.new(color, add.inputs['A'])
        add.inputs['B'].default_value = lt.hex_color(glow)
        color = add.outputs['Result']
    links.new(color, background.inputs['Color'])
    links.new(background.outputs['Background'], out.inputs['Surface'])
    return world


def stage():
    """The renders' set: Eevee as the posters render (ray-traced screen-space light, AgX), the street's dirt underfoot,
    the hulls hidden, glowing glass glowing (a material's Glow, the game's emissive strength, as Eevee's emission)."""
    bpy = kit.bpy
    scene = bpy.context.scene
    scene.render.engine = 'BLENDER_EEVEE_NEXT'
    ee = scene.eevee
    ee.taa_render_samples = 96
    ee.use_shadows = True
    if hasattr(ee, 'use_raytracing'):
        ee.use_raytracing = True
        ee.ray_tracing_method = 'SCREEN'
    if hasattr(ee, 'use_fast_gi'):
        ee.use_fast_gi = True
    scene.render.film_transparent = False
    scene.render.image_settings.file_format = 'PNG'
    scene.render.image_settings.color_mode = 'RGB'
    scene.view_settings.view_transform = 'AgX'
    scene.view_settings.look = 'AgX - Medium High Contrast'
    for o in scene.objects:
        if o.name.startswith('UCX_'):
            o.hide_render = True
    for mat in bpy.data.materials:
        if mat.get('Glow') and mat.use_nodes:
            bsdf = next(n for n in mat.node_tree.nodes if n.type == 'BSDF_PRINCIPLED')
            bsdf.inputs['Emission Color'].default_value = bsdf.inputs['Base Color'].default_value
            bsdf.inputs['Emission Strength'].default_value = float(mat['Glow'])
    mesh = bpy.data.meshes.new('_Ground')
    mesh.from_pydata([(-120.0, -120.0, 0.0), (120.0, -120.0, 0.0), (120.0, 120.0, 0.0), (-120.0, 120.0, 0.0)], [],
                     [(0, 1, 2, 3)])
    mesh.uv_layers.new(name='UVMap')
    ground = bpy.data.objects.new('_Ground', mesh)
    scene.collection.objects.link(ground)
    lt.assign(ground, lt.material('GroundDirt'))
    lt.box_uv(ground, 'GroundDirt')
    col = mesh.color_attributes.new('Col', 'BYTE_COLOR', 'CORNER')
    col.data.foreach_set('color', [1.0] * (4 * len(mesh.loops)))
    sun = bpy.data.objects.new('_Sun', bpy.data.lights.new('_Sun', 'SUN'))
    scene.collection.objects.link(sun)
    return scene, sun


def light(scene, sun, toward, strength, color, world):
    """The sun (toward it, its strength and colour) and the sky."""
    sun.data.energy, sun.data.color, sun.data.angle = strength, color, math.radians(2.0)
    sun.rotation_euler = (-Vector(toward).normalized()).to_track_quat('-Z', 'Y').to_euler()
    scene.world = world


def mount(model, socket, turn=0.0):
    """A copy of a prop hung on a socket (turn swings it about its pivot, degrees about Z)."""
    copy = model.copy()
    kit.bpy.context.scene.collection.objects.link(copy)
    copy.matrix_world = socket.matrix_world @ Matrix.Rotation(math.radians(turn), 4, 'Z')
    return copy


def lamp(scene, at, power):
    """The level's light at a SOCKET_Light: a shadowless point light, lamplight-warm, reaching 4 m."""
    data = kit.bpy.data.lights.new('_Lamp', 'POINT')
    data.energy, data.color, data.shadow_soft_size, data.use_shadow = power, LAMPLIGHT, 0.03, False
    if hasattr(data, 'use_custom_distance'):
        data.use_custom_distance, data.cutoff_distance = True, 4.0
    obj = kit.bpy.data.objects.new('_Lamp', data)
    obj.location = at
    scene.collection.objects.link(obj)
    return obj


def shoot(scene, path, eye, target, lens, size, exposure=0.0):
    """Renders from eye toward target."""
    bpy = kit.bpy
    cam = bpy.data.objects.get('_Camera') or bpy.data.objects.new('_Camera', bpy.data.cameras.new('_Camera'))
    if cam.name not in scene.collection.objects:
        scene.collection.objects.link(cam)
    cam.location = Vector(eye)
    cam.rotation_euler = (Vector(target) - Vector(eye)).to_track_quat('-Z', 'Y').to_euler()
    cam.data.lens, cam.data.clip_start, cam.data.clip_end = lens, 0.02, 2000.0
    scene.camera = cam
    scene.render.resolution_x, scene.render.resolution_y = size
    scene.render.resolution_percentage = 100
    scene.view_settings.exposure = exposure
    scene.render.filepath = path
    bpy.ops.render.render(write_still=True)
    lt._log(f'farmhouse: rendered {path}')
    return path


def side_by_side(paths, labels, out):
    """The renders in a row, labelled in Rye (looter_posters' labels); the single renders are removed."""
    import numpy as np
    import looter_posters as posters
    bpy = kit.bpy
    panels = []
    for path in paths:
        img = bpy.data.images.load(path, check_existing=False)
        img.colorspace_settings.name = 'Non-Color'
        w, h = img.size
        px = np.empty(w * h * 4, np.float32)
        img.pixels.foreach_get(px)
        bpy.data.images.remove(img)
        panels.append(px.reshape(h, w, 4)[::-1, :, :3].copy())
    gap = np.full((panels[0].shape[0], 12, 3), 0.08, np.float32)
    row = np.concatenate([part for panel in panels for part in (panel, gap)][:-1], axis=1)
    x = 0
    for panel, label in zip(panels, labels):
        posters.sheet_label(row, label, x + 34, 78, 46, color=(0.95, 0.65, 0.29))
        x += panel.shape[1] + 12
    lt.write_png(out, lt.to8(np.clip(row, 0.0, 1.0)))
    for path in paths:
        os.remove(path)
    lt._log(f'farmhouse: {out}')


def previews(house, ransom, door, dish):
    """The lived-in house's look sheet in Saved/ArtPreviews/RansomsRest/Farmhouse/, in the level's light (its golden
    late afternoon, the low sun behind the house, as the posters paint it; the game's Dusk state for the dusk shot):
      Farmhouse_BeforeAfter  the derelict house and Delia's side by side, front three-quarter from 12 m
      Farmhouse_Door         the talking door from a standing player's eye (1.7 m), 3 m from the screen
      Farmhouse_Dusk         Delia's house at dusk: the lamps up and the front windows' Glow raised (WINDOW_GLOW_DUSK)
      Farmhouse_Handoff      the screen door swung 25 degrees open, as for Heirloom
      Farmhouse_Props        the screen door and the plate on their own
    The props hang on the house's sockets, and both lamps have the point light the level puts at a SOCKET_Light."""
    bpy = kit.bpy
    os.makedirs(PREVIEW_DIR, exist_ok=True)
    scene, sun = stage()
    bpy.context.view_layer.update()
    sockets = {c.name.split('.')[0][len('SOCKET_'):]: c for c in ransom.children if c.name.startswith('SOCKET_')}
    screen = mount(door, sockets['ScreenDoor'])
    plate = mount(dish, sockets['Plate'])
    lamps = [lamp(scene, sockets[name].matrix_world.translation, power)
             for name, power in zip(('Light', 'Light_Porch'), LAMPS_DAY)]
    delia = [ransom, screen, plate] + lamps
    door.hide_render = dish.hide_render = True
    golden = sky('_Golden', GOLDEN_SKY, GOLDEN_SUN, GOLDEN_GLOWS)
    light(scene, sun, GOLDEN_SUN, 4.4, (1.0, 0.8, 0.56), golden)
    panes = bpy.data.materials['WindowGlow']
    panes_glow = next(n for n in panes.node_tree.nodes if n.type == 'BSDF_PRINCIPLED').inputs['Emission Strength']

    def lit(dusk):
        # The windows' Glow and the lamps as the game sets them by day or at dusk.
        panes_glow.default_value = WINDOW_GLOW_DUSK if dusk else WINDOW_GLOW_DAY
        for obj, power in zip(lamps, LAMPS_DUSK if dusk else LAMPS_DAY):
            obj.data.energy = power

    def show(objects, on):
        for o in objects:
            o.hide_render = not on
    three_quarter = ((-7.3, -10.5, 2.5), (0.3, -1.3, 2.75), 30.0)
    before, after = (os.path.join(PREVIEW_DIR, f'_{k}.png') for k in ('before', 'after'))
    show(delia, False)
    shoot(scene, before, *three_quarter, (1100, 1000), exposure=SHADE_EXPOSURE)
    show(delia, True)
    house.hide_render = True
    shoot(scene, after, *three_quarter, (1100, 1000), exposure=SHADE_EXPOSURE)
    side_by_side([before, after], ['BEFORE', 'AFTER'], os.path.join(PREVIEW_DIR, 'Farmhouse_BeforeAfter.png'))
    shoot(scene, os.path.join(PREVIEW_DIR, 'Farmhouse_Door.png'), (0.35, Y0 - 3.0, 1.7), (0.05, Y0, 1.72), 26.0,
          (1600, 1000), exposure=SHADE_EXPOSURE)
    screen.matrix_world = sockets['ScreenDoor'].matrix_world @ Matrix.Rotation(math.radians(-25.0), 4, 'Z')
    shoot(scene, os.path.join(PREVIEW_DIR, 'Farmhouse_Handoff.png'), (1.55, -5.05, 2.25), (0.05, Y0 - 0.15, 1.55),
          28.0, (1600, 1000), exposure=SHADE_EXPOSURE)
    screen.matrix_world = sockets['ScreenDoor'].matrix_world
    light(scene, sun, DUSK_SUN, 2.6, (1.0, 0.6, 0.34), sky('_Dusk', DUSK_SKY, DUSK_SUN, DUSK_GLOWS))
    lit(dusk=True)
    shoot(scene, os.path.join(PREVIEW_DIR, 'Farmhouse_Dusk.png'), (-7.6, -11.2, 2.3), (0.3, -1.3, 2.5), 28.0,
          (1920, 1080), exposure=0.2)
    lit(dusk=False)
    # The props on their own, away from the house, in the golden light.
    show(delia, False)
    door.hide_render = dish.hide_render = False
    door.location, dish.location = (40.0, 0.0, 0.0), (44.0, 0.0, 0.0)
    light(scene, sun, GOLDEN_SUN, 4.4, (1.0, 0.8, 0.56), golden)
    shots = [os.path.join(PREVIEW_DIR, f'_{k}.png') for k in ('screen', 'plate')]
    shoot(scene, shots[0], (41.9, -3.0, 1.55), (40.45, 0.0, 1.05), 35.0, (900, 1100), exposure=SHADE_EXPOSURE)
    shoot(scene, shots[1], (44.36, -0.5, 0.4), (43.99, 0.0, 0.035), 50.0, (900, 1100), exposure=SHADE_EXPOSURE)
    side_by_side(shots, ['SCREEN DOOR', 'PORCH PLATE'], os.path.join(PREVIEW_DIR, 'Farmhouse_Props.png'))


# --- The models ---

house = farmhouse()
if __name__ != '__overview__':            # the buildings' overview shows the tutorial island's house only
    ransom = farmhouse('Farmhouse_Ransom', lived_in=True)
    door = screen_door()
    dish = porch_plate()
    if lt.want_preview():
        previews(house, ransom, door, dish)
