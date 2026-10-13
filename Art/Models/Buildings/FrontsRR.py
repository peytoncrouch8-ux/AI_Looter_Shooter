"""Three more false fronts for Main Street on Ransom's Rest (Docs/Areas/RansomsRest_Expansion.md, section 9), in the
style of FalseFronts.py's four (a tall square facade with a lettered sign hiding a gable roof of shakes, on the house
trim sheet, Rye lettering), each its own shape and paint so the street doesn't repeat:

  RR_Front_Assay   the Providence Mining Co.'s assay office, shut since the fall: a pedimented "temple front" in cream
                   trim on weathered siding (the company trying to look respectable), a black sign board lettered in
                   cream, PROVIDENCE MINING CO. / ASSAY OFFICE, the company's crossed pick and hammer in the pediment,
                   black iron shutters closed and barred over both windows, a heavy strapped door chained and padlocked
                   with the company's notice nailed on it; small barred windows down the sides.
  RR_Front_Hotel   the Larkspur House, two storeys, boarded: bleached pale boards (WoodCream) trimmed in black, a porch
                   across the front (a deck that chains with the boardwalk, four posts and a shed roof of shakes), a
                   black-lettered sign band LARKSPUR HOUSE under a bracketed cornice and HOTEL in the arched panel over
                   it; every window and both doors planked over.
  RR_Front_Barber  the barber's, shut: a small front of faded teal boards (WoodTeal) with cream trim, its top stepped up
                   at the corners, a cream sign board lettered BARBER in oxide red over SHAVE . BATH . TEETH in black, a
                   little shake hood on scroll brackets over the half-glazed door and the shop window, linen blinds drawn
                   down both, a CLOSED card in the door, and a barber's pole on a bracket by the door.

Like FalseFronts.py's, a building's floor and door thresholds are at the boardwalk's height (looter_town.DECK_TOP,
0.38 m) and the facade faces the front (-Y). The pivot follows the homes kit (looter_homes, Homestead.py): the middle of
the front door's opening on the facade's outer face, at the ground (z = 0); each FRONTS: report line gives the walls'
extent from it, so the level can lay the facade on Main Street's line. The boardwalk (Boardwalk.py) runs in front as for
the others (its pivot line 1.5 m out from the facade); the hotel brings its own 3 m porch in its place, as Pruitt's
store does. Big roofs are shakes; the faces nobody sees are left out; Nanite's fallback keeps every triangle.

A scripted model (Art/README.md) built with looter_rr_town (on looter_homes, looter_town and looter_buildings).
  --preview   renders each front in the area's late-afternoon light into Saved/ArtPreviews/RansomsRest/Town/ and writes
              FrontsRR_report.json for the look sheet (Tools/Blender/town_sheet.py)
  --stats     prints where the triangles go; --no-ao skips the AO bake
"""
import math

import looter_buildings as kit
import looter_homes as homes
import looter_rr_town as rr
import looter_textures as lt
import looter_town as town
from looter_buildings import Matrix, Opening, Space, Trim, Vector

BASE = town.DECK_TOP
FT = rr.FT


def report(m, obj, door, walls):
    x0, x1, y0, y1 = walls
    print(f'FRONTS: {obj.name}: door at x {door:.2f} on the facade; walls x {x0 - door:.2f}..{x1 - door:.2f}, '
          f'y {0.0:.2f}..{y1 - y0:.2f} from the pivot; floor {BASE:.2f}', flush=True)


# --- The Providence Mining Co.'s assay office ---

def pick_and_hammer(m, space, cx, cz, y, s=1.0, look='cream'):
    """The company's mark: a miner's pick and a hammer crossed, flat pieces on a face (facing -y) round (cx, cz)."""
    for sign, head in ((1.0, 'pick'), (-1.0, 'hammer')):
        frame = Matrix.Translation((cx, 0.0, cz)) @ Matrix.Rotation(math.radians(sign * 38.0), 4, 'Y')
        handle = [(-0.025 * s, -0.42 * s), (0.025 * s, -0.42 * s), (0.02 * s, 0.32 * s), (-0.02 * s, 0.32 * s)]
        town.prism(m, handle, 0.02, look=look, space=space, matrix=frame, y=y)
        if head == 'pick':
            pts = [(-0.3 * s, 0.33 * s), (-0.12 * s, 0.37 * s), (0.0, 0.4 * s), (0.12 * s, 0.37 * s), (0.3 * s, 0.33 * s),
                   (0.12 * s, 0.31 * s), (0.0, 0.3 * s), (-0.12 * s, 0.31 * s)]
        else:
            pts = [(-0.14 * s, 0.28 * s), (0.14 * s, 0.28 * s), (0.14 * s, 0.4 * s), (-0.14 * s, 0.4 * s)]
        town.prism(m, pts, 0.025, look=look, space=space, matrix=frame, y=y - 0.005)


def iron_shutters(m, space, o, out=0.045):
    """Black iron shutters closed over window o: two plate leaves with riveted bands, a flat bar across them on staples
    (a company's strongroom window)."""
    rng = m.rng
    half = o.w * 0.5 + 0.06
    y = -out - 0.02
    for k, x in enumerate((o.x - 0.06 + half * 0.5, o.x + o.w + 0.06 - half * 0.5)):
        town.box(m, (half - 0.012, 0.02, o.h + 0.1), at=(x, y, o.z + o.h * 0.5), look='iron', space=space, drop=('+y',))
        for z in (o.z + 0.22, o.z + o.h - 0.22):
            town.box(m, (half - 0.06, 0.012, 0.07), at=(x, y - 0.016, z), look='iron', space=space, drop=('+y',))
    town.box(m, (o.w + 0.36, 0.03, 0.07), at=(o.x + o.w * 0.5, y - 0.04, o.z + o.h * 0.52),
             rot=(0.0, rng.uniform(-1.0, 1.0), 0.0), look='iron', space=space)
    for x in (o.x - 0.14, o.x + o.w + 0.14):
        town.box(m, (0.04, 0.05, 0.12), at=(x, y - 0.02, o.z + o.h * 0.52), look='iron', space=space)


def assay():
    m = rr.Building('RR_Front_Assay', seed=611)
    rng = m.rng
    XF0, XF1, Y0, Y1 = -3.4, 3.4, -4.5, 4.5
    X0, X1 = -3.15, 3.15
    YF = Y0 + FT
    WALL, PITCH = 3.3, 30.0
    EAVE = BASE + WALL
    WF = XF1 - XF0
    CORNICE, PEAK = 4.85, 5.95                    # the pediment's base and its peak over the floor
    top = [(0.0, CORNICE), (WF * 0.5, PEAK), (WF, CORNICE)]
    DOOR = Opening('door', WF * 0.5 - 0.6, 0.0, 1.2, 2.45, style='plank', finish='A', casing='band_G', hinge='left',
                   casing_w=0.16)
    shut = dict(style='glass', casing='band_G', sash='C', panes=(2, 2), casing_w=0.14)
    LEFT = Opening('window', 0.62, 0.85, 1.0, 1.6, **shut)
    RIGHT = Opening('window', WF - 0.62 - 1.0, 0.85, 1.0, 1.6, **shut)
    items = [LEFT, DOOR, RIGHT]
    front = rr.false_front(m, XF0, XF1, Y0, top, items, rr.siding(rng), BASE)
    town.openings(m, front, items, FT)
    for o in (LEFT, RIGHT):
        iron_shutters(m, front, o)
    m.section('facade, openings')

    # Shut since the fall: the door chained through its strap and the jamb, a padlock, the company's notice nailed on.
    cy = FT * 0.3 - 0.08
    for k in range(7):
        z = 1.02 + 0.07 * k * (1.0 if k < 4 else -1.0) + (0.42 if k >= 4 else 0.0)
        x = DOOR.x + DOOR.w - 0.18 + 0.06 * k
        town.box(m, (0.06, 0.02, 0.035), at=(x, cy, z), rot=(0.0, 35.0 + 50.0 * (k % 2), 0.0), look='iron',
                 space=front)
    town.box(m, (0.08, 0.035, 0.1), at=(DOOR.x + DOOR.w - 0.02, cy - 0.02, 0.86), look='brass', space=front)
    paper = (DOOR.x + 0.22, cy - 0.005, 1.38)
    town.box(m, (0.34, 0.006, 0.44), at=paper, rot=(0.0, rng.uniform(-3.0, 3.0), 0.0), look='band_G', space=front)
    for dx, dz in ((-0.15, 0.2), (0.15, 0.2)):
        town.box(m, (0.02, 0.01, 0.02), at=(paper[0] + dx, cy - 0.01, paper[2] + dz), look='iron', space=front)
    m.section('chain, notice')

    # The temple front in cream: pilasters at the corners and either side of the door, an entablature (a frieze and a
    # cornice) across under the pediment, the pediment's raking cornices and the mark in its tympanum.
    # The corner pilasters carry the entablature; the door's stop under the sign board, which spans between them.
    for x in (0.18, WF - 0.18):
        town.pilaster(m, front, x, 0.0, CORNICE - 0.62, width=0.26, look='band_G', depth=0.06)
    for x in (DOOR.x - 0.42, DOOR.x + DOOR.w + 0.42):
        town.pilaster(m, front, x, 0.0, 2.8, width=0.2, look='band_G', depth=0.06)
    town.cornice(m, front, 0.0, WF, CORNICE, look='band_G', project=0.24, height=0.24, brackets=0, frieze=0.34)
    slope = math.degrees(math.atan2(PEAK - CORNICE, WF * 0.5))
    run = math.hypot(WF * 0.5, PEAK - CORNICE)
    for sign in (-1.0, 1.0):
        cx = WF * 0.5 + sign * WF * 0.25
        town.box(m, (run + 0.2, 0.26, 0.16), at=(cx, -0.13, (CORNICE + PEAK) * 0.5 + 0.08),
                 rot=(0.0, sign * slope, 0.0), look='band_G', space=front)
    pick_and_hammer(m, front, WF * 0.5, CORNICE + 0.42, -0.02, s=0.82)
    m.section('pilasters, cornice, pediment')

    # The sign: black boards framed in cream, cream letters.
    sx0, sx1, sz0, sz1 = 0.42, WF - 0.42, 2.95, 4.08
    for k in range(5):
        z = sz0 + 0.02 + (sz1 - sz0 - 0.04) * (k + 0.5) / 5
        town.board(m, (sx0, 0.0, z), (sx1, 0.0, z), (sz1 - sz0 - 0.04) / 5 - 0.004, 0.035, look='woodblack', space=front,
                   lift=0.0175)
    field_y = -0.035
    for z in (sz0, sz1):
        town.board(m, (sx0 - 0.08, field_y, z), (sx1 + 0.08, field_y, z), 0.1, 0.045, look='band_G', space=front,
                   lift=0.0225)
    for x in (sx0 - 0.03, sx1 + 0.03):
        town.board(m, (x, field_y, sz0 + 0.05), (x, field_y, sz1 - 0.05), 0.1, 0.045, look='band_G', space=front,
                   lift=0.0225)
    town.text(m, 'PROVIDENCE MINING CO.', (WF * 0.5, field_y, 3.74), height=0.3, width=5.55, space=front, look='cream',
              tol=0.032)
    town.text(m, 'ASSAY OFFICE', (WF * 0.5, field_y, 3.24), height=0.24, space=front, look='cream', tol=0.032)
    m.section('sign')

    # The walls behind: weathered siding, small barred windows (an assayer's strongroom), a strapped back door; shakes and a
    # stovepipe.
    barred = dict(style='barred', casing='C', panes=(1, 1), bars=4, casing_w=0.12)
    sides = {'right': [Opening('window', 2.4, 1.3, 0.7, 0.8, **barred), Opening('window', 6.0, 1.3, 0.7, 0.8, **barred)],
             'left': [Opening('window', 3.6, 1.3, 0.7, 0.8, **barred)]}
    back = [Opening('door', 3.7, 0.0, 1.05, 2.15, style='plank', finish='A', casing='C', hinge='right'),
            Opening('window', 1.3, 1.3, 0.7, 0.8, **barred)]
    rr.shell(m, X0, X1, YF, Y1, WALL, PITCH, sides, back, BASE)
    m.section('walls')
    PIPE = (1.6, 2.6)
    town.hidden_gable(m, X0, X1, Y0 + FT * 0.5, Y1 + 0.3, EAVE, PITCH, covering='shakes', overhang=0.28, sag=0.05,
                      chimney=(PIPE[0], PIPE[1], 0.24, 0.24))
    roof_z = rr.roof_height(X0, X1, EAVE, PITCH)
    rr.chimney_pipe(m, PIPE[0], PIPE[1], roof_z(PIPE[0]) - 0.35, roof_z(PIPE[0]) + 1.0)
    town.facade_back(m, front, WF, top, lambda x: roof_z(XF0 + x) - BASE, FT)
    for x in (XF0 + 0.16, XF1 - 0.16):
        m.box((0.34, FT + 0.12, BASE + 0.05), at=(x, Y0 + FT * 0.5, BASE * 0.5 - 0.02), uv=Trim('D', world=True))
    m.section('roof')

    m.socket('Decal', front.world((DOOR.x - 1.1, -0.07, 1.6)))
    m.socket('Decal_2', front.world((DOOR.x + DOOR.w + 1.1, -0.07, 1.6)))
    rr.front_hulls(m, XF0, XF1, Y0, X0, X1, Y1, EAVE, PITCH, BASE + CORNICE + 0.1)
    m.hull_points([(XF0 + x, y, BASE + z + 0.15) for x, z in top for y in (Y0 - 0.26, Y0 + FT)])
    door = XF0 + DOOR.x + DOOR.w * 0.5
    m.rooms.append('closed: no rooms inside (shut since the fall)')
    m.captions = ['The Providence Mining Co.\'s assay office, shut since', 'the fall: iron shutters, a chained door.']
    obj = m.finish(pivot=(door, Y0), footprint=(XF0, XF1, Y0, Y1))
    report(m, obj, door, (XF0, XF1, Y0, Y1))
    return m, obj


# --- The Larkspur House ---

def hotel():
    m = rr.Building('RR_Front_Hotel', seed=612)
    rng = m.rng
    XF0, XF1, Y0, Y1 = -5.0, 5.0, -6.0, 6.0
    X0, X1 = -4.75, 4.75
    YF = Y0 + FT
    UP = 3.25
    WALL, PITCH = 2.0 * UP, 18.0
    EAVE = BASE + WALL
    WF = XF1 - XF0
    TOP = 7.75
    AX0, AX1 = 3.3, WF - 3.3
    top = [(0.0, TOP), (AX0, TOP)] + rr.arch(AX0, AX1, TOP, 0.7)[1:-1] + [(AX1, TOP), (WF, TOP)]
    boarded = dict(style='boarded', casing='woodblack', sash='woodblack', boards='planks', panes=(2, 2), casing_w=0.13)
    DOOR = Opening('door', WF * 0.5 - 0.75, 0.0, 1.5, 2.75, style='double', finish='woodblack', casing='woodblack',
                   knob='brass', transom=0.42, glass=True, casing_w=0.15)
    lobby = dict(boarded, panes=(3, 2))
    ground = [Opening('window', x, 0.55, 2.3, 2.0, **lobby) for x in (0.95, WF - 0.95 - 2.3)]
    upper = [Opening('window', x, UP + 0.85, 0.85, 1.45, **boarded) for x in (0.85, 3.0, WF - 3.0 - 0.85, WF - 0.85 - 0.85)]
    items = ground + [DOOR] + upper
    front = rr.false_front(m, XF0, XF1, Y0, top, items, homes.Boards(rotate=False), BASE, mat='woodcream', covered=True)
    town.openings(m, front, items, FT)
    town.board_up(m, front, DOOR.x, 0.05, DOOR.w, DOOR.h - 0.1, look='planks', out=0.07, level=True)
    for x in (0.12, WF - 0.12):
        town.pilaster(m, front, x, 0.0, TOP - 0.6, width=0.24, look='woodblack', depth=0.06, cap=False)
    town.board(m, (0.0, 0.0, UP - 0.12), (WF, 0.0, UP - 0.12), 0.22, 0.05, look='woodblack', space=front, lift=0.025)
    m.section('facade, openings')

    # The sign band: LARKSPUR HOUSE painted black on cream boards in a black frame; a bracketed cornice over it; HOTEL in the
    # arched panel.
    sx0, sx1, sz0, sz1 = 0.45, WF - 0.45, 5.72, 6.72
    for k in range(5):
        z = sz0 + 0.02 + (sz1 - sz0 - 0.04) * (k + 0.5) / 5
        town.board(m, (sx0, 0.0, z), (sx1, 0.0, z), (sz1 - sz0 - 0.04) / 5 - 0.004, 0.03, look='band_G', space=front,
                   lift=0.015)
    for z in (sz0, sz1):
        town.board(m, (sx0 - 0.08, -0.03, z), (sx1 + 0.08, -0.03, z), 0.1, 0.04, look='woodblack', space=front, lift=0.02)
    for x in (sx0 - 0.03, sx1 + 0.03):
        town.board(m, (x, -0.03, sz0 + 0.05), (x, -0.03, sz1 - 0.05), 0.1, 0.04, look='woodblack', space=front, lift=0.02)
    town.text(m, 'LARKSPUR HOUSE', (WF * 0.5, -0.03, (sz0 + sz1) * 0.5), height=0.56, width=8.4, space=front,
              look='black', tol=0.034)
    town.cornice(m, front, 0.0, WF, TOP, look='woodblack', project=0.3, height=0.28, brackets=1.6, frieze=0.28)
    town.cap_profile(m, front, top[1:-1], depth=0.18, height=0.1, look='woodblack', back=FT + 0.05)
    town.text(m, 'HOTEL', (WF * 0.5, -0.02, TOP + 0.31), height=0.34, space=front, look='black', tol=0.034)
    m.section('sign, cornice')

    # The porch: a plank deck that chains with the boardwalk, four posts with knee braces, a shed roof of shakes back to
    # the facade under the upper windows.
    deck_front = Y0 - town.WALK_DEPTH
    town.deck(m, XF0, XF1, deck_front, Y0, posts=[XF0 + 1.0 + 2.0 * k for k in range(5)], back=False, plank_cuts=0)
    porch = kit.wall_space((XF0, Y0), (XF1, Y0), 0.0)
    post_y = -town.WALK_DEPTH + 0.18
    beam_z = 3.05
    posts = [0.22, 3.4, WF - 3.4, WF - 0.22]
    for x in posts:
        town.board(m, (x, post_y, town.DECK_TOP), (x, post_y, beam_z), 0.16, 0.16, look='woodblack', space=porch,
                   bevel=0.015)
        town.box(m, (0.22, 0.22, 0.05), at=(x, post_y, town.DECK_TOP + 0.025), look='woodblack', space=porch)
        for side in (-1.0, 1.0):
            if 0.3 < x + side * 0.5 < WF - 0.3:
                town.board(m, (x + side * 0.07, post_y, beam_z - 0.55), (x + side * 0.5, post_y, beam_z - 0.05), 0.1,
                           0.09, look='woodblack', space=porch)
    town.board(m, (-0.1, post_y, beam_z + 0.11), (WF + 0.1, post_y, beam_z + 0.11), 0.22, 0.18, look='woodblack',
               space=porch)
    wall_z = BASE + UP + 0.1
    theta = math.atan2(wall_z - (beam_z + 0.22), -post_y)
    p_deck, p_over = 0.06, 0.34
    normal = Vector((0.0, -math.sin(theta), math.cos(theta)))
    origin = Vector((XF0 - 0.12, Y0 + post_y - p_over, beam_z + 0.22 - p_over * math.tan(theta))) + normal * p_deck
    p_len = (-post_y + p_over) / math.cos(theta)
    porch_roof = kit.Slope(origin, (1.0, 0.0, 0.0), (0.0, math.cos(theta), math.sin(theta)), WF + 0.24, p_len, sag=0.05)
    town.roof_deck(m, porch_roof, p_deck)
    town.shakes(m, porch_roof, piece=(4.5, 6.0), top=p_len - 0.03)
    fascia_z = beam_z + 0.22 - p_over * math.tan(theta) - 0.02
    town.board(m, (XF0 - 0.14, Y0 + post_y - p_over - 0.02, fascia_z), (XF1 + 0.14, Y0 + post_y - p_over - 0.02, fascia_z),
               0.2, 0.04, look='woodblack')
    m.section('porch')

    # The walls behind: two storeys of weathered siding, windows on both floors, most boarded; a back door boarded.
    glass = dict(style='glass', casing='C', panes=(2, 2))
    shut = dict(style='boarded', casing='C', boards='planks', panes=(2, 2))
    sides = {'right': [Opening('window', 3.4, 0.9, 0.9, 1.4, **shut), Opening('window', 7.6, UP + 0.8, 0.85, 1.3, **glass)],
             'left': [Opening('window', 5.0, 0.9, 0.9, 1.4, **shut), Opening('window', 2.6, UP + 0.8, 0.85, 1.3, **glass)]}
    back = [Opening('door', 6.2, 0.0, 1.0, 2.15, casing='C', hinge='left'),
            Opening('window', 2.6, UP + 0.8, 0.9, 1.3, **shut)]
    spaces = rr.shell(m, X0, X1, YF, Y1, WALL, PITCH, sides, back, BASE)
    town.board_up(m, spaces['back'], 6.2, 0.0, 1.0, 2.15, look='planks', out=0.07)
    for name in ('right', 'left'):
        town.trim_board(m, spaces[name], (0.1, 0.0, UP - 0.12), (Y1 - YF - 0.1, 0.0, UP - 0.12), 0.18, 'C', thick=0.05)
    m.section('walls')
    CH = (-2.3, 2.6)
    town.hidden_gable(m, X0, X1, Y0 + FT * 0.5, Y1 + 0.35, EAVE, PITCH, covering='shakes', overhang=0.28, sag=0.06,
                      chimney=(CH[0], CH[1], 0.38, 0.42))
    roof_z = rr.roof_height(X0, X1, EAVE, PITCH)
    chimney_top = roof_z(CH[0]) + 1.15
    foot = roof_z(CH[0]) - 0.6
    kit.stone_stack(m, (CH[0], CH[1], foot), (0.62, 0.72), chimney_top - foot, taper=0.03)
    homes.flue_cap(m, CH[0], CH[1], chimney_top, (0.62, 0.72))
    town.facade_back(m, front, WF, top, lambda x: roof_z(XF0 + x) - BASE, FT)
    m.section('roof')

    m.socket('Decal', front.world((3.74, -0.07, 1.45)))
    m.socket('Decal_2', front.world((WF - 3.74, -0.07, 1.45)))
    rr.front_hulls(m, XF0, XF1, Y0, X0, X1, Y1, EAVE, PITCH, BASE + TOP)
    m.hull((AX1 - AX0, FT + 0.04, 0.75), at=(XF0 + (AX0 + AX1) * 0.5, Y0 + FT * 0.5, BASE + TOP + 0.37))
    m.hull((WF, town.WALK_DEPTH, town.DECK_TOP), at=(0.0, Y0 - town.WALK_DEPTH * 0.5, town.DECK_TOP * 0.5))
    for x in posts:
        m.hull((0.18, 0.18, beam_z - town.DECK_TOP), at=(XF0 + x, Y0 + post_y, (beam_z + town.DECK_TOP) * 0.5))
    m.hull((porch_roof.width, p_len, 0.2), at=(porch_roof.width * 0.5, p_len * 0.5, -0.02), space=porch_roof)
    m.hull((0.66, 0.76, chimney_top - foot + 0.3), at=(CH[0], CH[1], (foot + chimney_top) * 0.5))
    door = XF0 + DOOR.x + DOOR.w * 0.5
    m.rooms.append('closed: no rooms inside (boarded); its porch deck is walkable at the boardwalk\'s height')
    m.captions = ['The Larkspur House: two storeys boarded up,', 'a porch that carries on the boardwalk.']
    obj = m.finish(pivot=(door, Y0), footprint=(XF0, XF1, Y0, Y1))
    report(m, obj, door, (XF0, XF1, Y0, Y1))
    return m, obj


# --- The barber's ---

def barber_pole(m, at, height=0.95, r=0.065, turns=1.6, stripes=3):
    """A barber's pole standing at at (its foot): a cream pole with oxide stripes wound round it, brass caps."""
    x, y, z = at
    town.cylinder(m, (x, y, z), (x, y, z + height), r, sides=10, look='cream', caps=(False, False))
    steps = 14
    for s in range(stripes):
        rows = []
        for i in range(steps + 1):
            t = i / steps
            zz = z + 0.03 + (height - 0.06) * t
            a0 = 2.0 * math.pi * (turns * t + s / stripes)
            rows.append([((r + 0.003) * math.cos(a0 + da) + x, (r + 0.003) * math.sin(a0 + da) + y, zz + dz)
                         for da, dz in ((0.0, 0.0), (0.55, 0.06))])
        tb = kit._new_bmesh()
        grid = [[tb.verts.new(p) for p in row] for row in rows]
        for i in range(steps):
            f = tb.faces.new((grid[i][0], grid[i + 1][0], grid[i + 1][1], grid[i][1]))
            f.normal_update()
            c = f.calc_center_median()
            if f.normal.dot(Vector((c.x - x, c.y - y, 0.0))) < 0.0:
                f.normal_flip()
        spec, key = town.finish('oxide')
        m.emit(tb, spec, key, smooth=True)
    for zz, h in ((z - 0.07, 0.07), (z + height, 0.08)):
        town.lathe(m, [(0.0, 0.0), (r + 0.02, 0.0), (r + 0.02, h * 0.5), (r * 0.6, h), (0.0, h + 0.02)], at=(x, y, zz),
                   sides=10, look='brass')
    town.lathe(m, [(0.0, 0.0), (0.04, 0.01), (0.05, 0.05), (0.03, 0.09), (0.0, 0.1)], at=(x, y, z + height + 0.08),
               sides=8, look='brass')


def barber():
    m = rr.Building('RR_Front_Barber', seed=613)
    rng = m.rng
    XF0, XF1, Y0, Y1 = -2.85, 2.85, -4.0, 4.0
    X0, X1 = -2.6, 2.6
    YF = Y0 + FT
    WALL, PITCH = 3.1, 32.0
    EAVE = BASE + WALL
    WF = XF1 - XF0
    TOP, EAR = 4.72, 5.08
    top = [(0.0, EAR), (0.48, EAR), (0.48, TOP), (WF - 0.48, TOP), (WF - 0.48, EAR), (WF, EAR)]
    DOOR = Opening('door', 0.45, 0.0, 1.1, 2.5, style='panel', finish='woodteal', casing='band_G', knob='brass',
                   glass=True, transom=0.34, casing_w=0.13)
    WIN = Opening('window', 2.2, 0.7, 2.1, 1.75, style='glass', casing='band_G', sash='band_G', panes=(3, 2),
                  casing_w=0.13)
    items = [DOOR, WIN]
    front = rr.false_front(m, XF0, XF1, Y0, top, items, homes.Boards(rotate=False), BASE, mat='woodteal')
    town.openings(m, front, items, FT)
    for x in (0.1, WF - 0.1):
        town.trim_board(m, front, (x, 0.0, -BASE), (x, 0.0, EAR - 0.12), 0.2, 'band_G', thick=0.05)
    town.board(m, (0.0, 0.0, 0.08), (WF, 0.0, 0.08), 0.2, 0.05, look='band_G', space=front, lift=0.025)
    m.section('facade, openings')

    # Shut: linen blinds drawn right down behind the window's and the door's glass (6 mm in front of it, FalseFronts'
    # door_blinds), a CLOSED card hung in the door.
    gy = FT * 0.55 - 0.012
    rr.face_part(m, [front.world((WIN.x + 0.03, gy, WIN.z + 0.06)), front.world((WIN.x + WIN.w - 0.03, gy, WIN.z + 0.06)),
                     front.world((WIN.x + WIN.w - 0.03, gy, WIN.z + WIN.h - 0.03)),
                     front.world((WIN.x + 0.03, gy, WIN.z + WIN.h - 0.03))], homes.Cloth(4.4), facing=(0.0, -1.0, 0.0))
    town.box(m, (WIN.w - 0.04, 0.012, 0.03), at=(WIN.x + WIN.w * 0.5, gy - 0.008, WIN.z + 0.08), look='C', space=front,
             drop=('+y',))
    leaf_h = DOOR.h - 0.34 - 0.06
    dy = FT * 0.3 - 0.022
    z0, z1 = leaf_h * 0.42 + 0.08, leaf_h - 0.15
    pw = DOOR.w - 2.0 * 0.12 - 0.06
    rr.face_part(m, [front.world((DOOR.x + 0.15, dy, z0)), front.world((DOOR.x + 0.15 + pw, dy, z0)),
                     front.world((DOOR.x + 0.15 + pw, dy, z1)), front.world((DOOR.x + 0.15, dy, z1))],
                 homes.Cloth(5.5), facing=(0.0, -1.0, 0.0))
    card_z = (z0 + z1) * 0.5 - 0.05
    cx = DOOR.x + DOOR.w * 0.5
    for sx in (-0.07, 0.07):
        town.box(m, (0.004, 0.004, 0.14), at=(cx + sx * 0.8, dy - 0.006, card_z + 0.14), rot=(0.0, sx * 220.0, 0.0),
                 look='C', space=front)
    town.box(m, (0.26, 0.006, 0.12), at=(cx, dy - 0.008, card_z), rot=(0.0, -2.0, 0.0), look='band_G', space=front)
    town.text(m, 'CLOSED', (cx, dy - 0.012, card_z), width=0.21, height=0.06, space=front, look='black', tol=0.05,
              rot=-2.0)
    m.section('blinds, card')

    # The hood over the door and window: a shed roof of shakes on two scroll brackets.
    hood_z = 2.98
    for x in (0.25, WF - 0.25):
        town.bracket(m, front, x, hood_z, reach=0.62, drop_h=0.55, thick=0.08, look='band_G')
    hood = kit.wall_space((XF0, Y0), (XF1, Y0), BASE)
    theta = math.radians(22.0)
    reach = 0.85
    h_origin = Vector((XF0 + 0.05, Y0 - reach, BASE + hood_z + 0.04 - 0.12)) + Vector((0.0, -math.sin(theta),
                                                                                      math.cos(theta))) * 0.04
    hood_slope = kit.Slope(h_origin, (1.0, 0.0, 0.0), (0.0, math.cos(theta), math.sin(theta)), WF - 0.1,
                           reach / math.cos(theta) + 0.02, sag=0.02)
    town.roof_deck(m, hood_slope, 0.04)
    town.shakes(m, hood_slope, piece=(2.5, 3.2))
    m.section('hood')

    # The sign: a cream board in a teal frame: BARBER in oxide red, SHAVE . BATH . TEETH in black between diamonds.
    sx0, sx1, sz0, sz1 = 0.6, WF - 0.6, 3.22, 4.38
    for k in range(5):
        z = sz0 + 0.02 + (sz1 - sz0 - 0.04) * (k + 0.5) / 5
        town.board(m, (sx0, 0.0, z), (sx1, 0.0, z), (sz1 - sz0 - 0.04) / 5 - 0.004, 0.03, look='band_G', space=front,
                   lift=0.015)
    for z in (sz0, sz1):
        town.board(m, (sx0 - 0.07, -0.03, z), (sx1 + 0.07, -0.03, z), 0.09, 0.04, look='woodteal', space=front,
                   lift=0.02)
    for x in (sx0 - 0.025, sx1 + 0.025):
        town.board(m, (x, -0.03, sz0 + 0.045), (x, -0.03, sz1 - 0.045), 0.09, 0.04, look='woodteal', space=front,
                   lift=0.02)
    town.text(m, 'BARBER', (WF * 0.5, -0.03, 4.0), height=0.44, width=3.6, space=front, look='oxide', tol=0.03)
    words = ('SHAVE', 'BATH', 'TEETH')
    word_h, gap = 0.17, 0.17
    widths = []
    for word in words:
        e = town.text_extent(word)
        widths.append((e[2] - e[0]) / (e[3] - e[1]) * word_h)
    x = WF * 0.5 - (sum(widths) + gap * (len(words) - 1)) * 0.5
    for k, word in enumerate(words):
        w, _ = town.text(m, word, (x, -0.03, 3.48), height=word_h, space=front, look='black', tol=0.04, align='left')
        x += w + gap
        if k < 2:
            town.prism(m, [(x - 0.085, 3.43), (x - 0.045, 3.48), (x - 0.085, 3.53), (x - 0.125, 3.48)], 0.01, look='oxide',
                       space=front, y=-0.031)
    town.cornice(m, front, 0.48, WF - 0.48, TOP, look='band_G', project=0.2, height=0.2, brackets=0, frieze=0.0)
    town.cap_profile(m, front, top, depth=0.14, height=0.08, look='band_G', back=FT + 0.04)
    m.section('sign, cap')

    # The barber's pole on an iron bracket left of the door.
    pole = front.world((0.2, -0.32, 1.25))
    town.board(m, front.world((0.2, -0.02, 1.15)), (pole.x, pole.y, BASE + 1.15), 0.04, 0.03, look='iron',
               face=(1.0, 0.0, 0.0))
    town.board(m, front.world((0.2, -0.02, 2.3)), (pole.x, pole.y, BASE + 2.3), 0.04, 0.03, look='iron',
               face=(1.0, 0.0, 0.0))
    barber_pole(m, (pole.x, pole.y, BASE + 1.25))
    m.section('pole')

    sides = {'right': [Opening('window', 3.4, 1.0, 0.85, 1.25, style='glass', casing='C', panes=(2, 2))],
             'left': [Opening('window', 2.8, 1.0, 0.85, 1.25, style='boarded', casing='C')]}
    back = [Opening('door', 2.8, 0.0, 1.0, 2.15, style='plank', finish='A', casing='C', hinge='left'),
            Opening('window', 1.0, 1.05, 0.7, 0.9, style='glass', casing='C', panes=(2, 2))]
    rr.shell(m, X0, X1, YF, Y1, WALL, PITCH, sides, back, BASE)
    m.section('walls')
    PIPE = (-1.4, 2.2)
    town.hidden_gable(m, X0, X1, Y0 + FT * 0.5, Y1 + 0.3, EAVE, PITCH, covering='shakes', overhang=0.28, sag=0.05,
                      chimney=(PIPE[0], PIPE[1], 0.24, 0.24))
    roof_z = rr.roof_height(X0, X1, EAVE, PITCH)
    rr.chimney_pipe(m, PIPE[0], PIPE[1], roof_z(PIPE[0]) - 0.35, roof_z(PIPE[0]) + 0.95)
    town.facade_back(m, front, WF, top, lambda x: roof_z(XF0 + x) - BASE, FT)
    m.section('roof')

    m.socket('Decal', front.world((WF - 0.32, -0.07, 1.25)), (0.0, 0.0, 0.0))
    rr.front_hulls(m, XF0, XF1, Y0, X0, X1, Y1, EAVE, PITCH, BASE + TOP)
    for x in (0.0, WF - 0.48):
        m.hull((0.48, FT + 0.04, EAR - TOP), at=(XF0 + x + 0.24, Y0 + FT * 0.5, BASE + (TOP + EAR) * 0.5))
    m.hull((hood_slope.width, hood_slope.length, 0.14), at=(hood_slope.width * 0.5, hood_slope.length * 0.5, -0.02),
           space=hood_slope)
    door = XF0 + DOOR.x + DOOR.w * 0.5
    m.rooms.append('closed: no rooms inside (shut, its blinds drawn)')
    m.captions = ["The barber's (SHAVE . BATH . TEETH), shut:", 'blinds drawn, a CLOSED card, the pole by the door.']
    obj = m.finish(pivot=(door, Y0), footprint=(XF0, XF1, Y0, Y1))
    report(m, obj, door, (XF0, XF1, Y0, Y1))
    return m, obj


BUILT = [assay(), hotel(), barber()]
models = [obj for _, obj in BUILT]

if lt.want_preview():
    shots = {}
    views = {'RR_Front_Assay': dict(view=(-0.55, -1.0, 0.3)), 'RR_Front_Hotel': dict(view=(-0.6, -1.0, 0.32)),
             'RR_Front_Barber': dict(view=(0.6, -1.0, 0.28), sun=(-38.0, 16.0), figure='left')}
    for m, obj in BUILT:
        opts = views.get(obj.name, {})
        shots[obj.name] = [rr.exterior(obj, obj.name, **opts)]
    rr.write_report('FrontsRR', [m for m, _ in BUILT], shots)
