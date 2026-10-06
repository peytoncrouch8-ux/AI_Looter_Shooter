"""The false fronts of Main Street on Ransom's Rest (Docs/Areas/RansomsRest.md, Main Street): four frontier shops, each a
tall square facade with a lettered sign board hiding a gable roof behind it, built on the house trim sheet, and the
shutters that slam as Ellis walks by.

  FalseFront_Undertaker  "Bright & Daughter, Undertakers": the town's most cared-for facade, cream clapboards (the house
                         trim's cream strip G) and black trim (WoodBlack), gilt lettering (BrassWorn), an arched parapet
                         with urns. Tilly Bright talks through the left window, its lower sash pushed up
                         (SOCKET_Speaker); behind it a shallow room with black drapes, a dim lamp (LanternGlow,
                         SOCKET_Light), a coffin on display, and a card on a string: "Back after the funeral". Black crepe
                         on the door. A plain back door and a loading step face the undertaker's yard and the depot.
                         SOCKET_Smoke tops the chimney.
  FalseFront_Saloon      "The Gilt Spur": two storeys with a balcony over the boardwalk on four posts, a stepped parapet
                         with a spur on it. Boarded up: planks nailed across every door and window, and a board across the
                         doors reading CLOSED FOR MOURNING.
  FalseFront_Store       "Pruitt's General Store": the widest front, flat under a bracketed cornice with a low pediment,
                         two big display windows round glazed double doors, and a deep porch (a plank deck that chains
                         with the boardwalk, posts and a shake roof). Windows down both sides carry shutter sockets
                         (SOCKET_Shutter_L1..L4 / R1..R4). SOCKET_Smoke tops the stovepipe.
  FalseFront_Sheriff     the empty sheriff's office: a stone storey (a jail) with quoins, barred windows and an
                         iron-strapped door hung with black crepe, under a wooden false front lettered SHERIFF with a star.
                         SOCKET_Smoke tops the (cold) stovepipe.
  Shutter_Left/Right     a plank shutter (0.5 x 1.4 m) whose pivot is its hinge: the game hangs one on each
                         SOCKET_Shutter_L<n> / R<n>, where it hangs closed, and swings it open through 180 degrees (its
                         free edge out toward the street) until it lies flat against the wall. No collision.

Every building's floor and door thresholds are at the boardwalk's height (looter_town.DECK_TOP, 0.38 m), and its
pivot is the middle of its walls' footprint on the ground; the facade faces the front (-Y). A boardwalk (Boardwalk.py)
runs along the facade: the facade's front face is at y = -depth / 2 (FACADE_Y below), so the walk's pivot line goes
1.5 m in front of it. The store brings its own 3 m porch in place of the walk. Big roofs are shakes (the trim sheet's
tin reads as loot colors over a large area). Faces nobody sees are left out (walls' insides, the shakes' and
clapboards' hidden sides, the backs of trim), which keeps the sources at 4.7-9.3k triangles, and Nanite's fallback
keeps all of them (FALLBACK).

A scripted model (Art/README.md) built with looter_buildings and looter_town.
"""
import math

import looter_buildings as kit
import looter_textures as lt
import looter_town as town
from looter_buildings import Matrix, Opening, Trim, Vector

BASE = town.DECK_TOP        # floors and thresholds at the boardwalk's walking surface
THICK = 0.14                # walls
FT = 0.16                   # the false front's thickness
FACADE_Y = {}               # each building's facade front (y), for the street preview and the report


def siding(rng, rotate=False):
    """Weathered siding (strip A) at world scale, slid along the strip so walls don't match."""
    return Trim('A', world=True, rotate=rotate, u=rng.uniform(0.0, 6.4))


# Nanite's fallback (what Medium and Low draw) keeps every triangle: Unreal's own reduction of these meshes left
# vertices with zero tangents (the saloon at 61.8%), and the sources are lean enough to be drawn whole.
FALLBACK = 100.0


# --- The parts every false front shares ---

def false_front(m, x0, x1, y0, top, items, finish_siding=None, bottom=-BASE, covered=False):
    """The facade panel: its front face at y = y0 from x0 to x1, from the ground (bottom, in the facade's space whose
    z = 0 is the floor) up to the top profile (points (x, z) left to right in the facade's space), with holes for the
    openings (doors included: the panel runs on below them to the ground). covered: clapboards hide its face, so it
    isn't cut for the baked shading round the openings. Returns its space."""
    space = kit.wall_space((x0, y0), (x1, y0), BASE)
    width = x1 - x0
    outline = [(0.0, bottom), (width, bottom)] + list(reversed(top))
    holes = [kit.rect(o.x, max(o.z, 0.0), o.w, o.h) for o in items]
    m.panel(outline, holes, FT, finish_siding or siding(m.rng), space=space, around=() if covered else items,
            slices=0)
    return space


def shell(m, X0, X1, YF, Y1, wall_h, pitch, sides, back, rotate=False, water_table=True, corner='C', stone=False):
    """The walls behind a false front: the two side (eave) walls from the facade's back (YF) to Y1 and the back gable
    wall, on a fieldstone plinth, with their openings, corner boards and a water table along the bottom (stone: walls
    of fieldstone instead, without the boards). sides holds the right and left walls' openings with x measured from the
    front; back holds the back wall's (x from its left seen from behind). Returns the spaces by name; each space's
    items are its openings in its own coordinates."""
    rng = m.rng
    town.plinth(m, X0, X1, YF + 0.1, Y1, BASE, out=0.06)
    if stone:
        water_table, corner = False, None
    width = X1 - X0
    rise = width * 0.5 * math.tan(math.radians(pitch))
    length = Y1 - YF
    walls = {
        'right': ((X1, YF), (X1, Y1), None, sides.get('right', [])),
        'back': ((X1, Y1), (X0, Y1), (width * 0.5, wall_h + rise), back),
        'left': ((X0, Y1), (X0, YF), None, [Opening(o.kind, length - o.x - o.w, o.z, o.w, o.h, **o.opts)
                                            for o in sides.get('left', [])]),
    }
    spaces = {}
    for name, (p0, p1, apex, items) in walls.items():
        space = kit.wall_space(p0, p1, BASE)
        space.items = items
        spaces[name] = space
        span = (Vector(p1) - Vector(p0)).length
        uv = Trim('D', world=True, u=rng.uniform(0.0, 6.4)) if stone else siding(rng, rotate)
        town.panel(m, kit.wall_outline(span, wall_h, items, apex), kit.holes(items), THICK, uv, space=space,
                   around=items, slices=4.0, back=False)
        town.openings(m, space, items, THICK)
        top = wall_h if apex is None else wall_h - 0.02
        for x in ((0.07, span - 0.07) if apex is not None else (span - 0.07,) if name == 'right' else (0.07,)):
            if corner:
                town.trim_board(m, space, (x, 0.0, -0.05), (x, 0.0, top), 0.15, corner, thick=0.045)
        if water_table:
            spans = [(0.1, span - 0.1)]
            for o in items:
                if o.z <= 1e-4:
                    spans = town.subtract(spans, (o.x - 0.2, o.x + o.w + 0.2))
            for a, b in spans:
                town.trim_board(m, space, (a, 0.0, 0.1), (b, 0.0, 0.1), 0.2, 'C', thick=0.045)
        if apex is None and not stone:
            town.trim_board(m, space, (0.12, 0.0, wall_h - 0.14), (span - 0.12, 0.0, wall_h - 0.14), 0.24, 'A',
                            thick=0.035)
    return spaces


def hulls(m, XF0, XF1, Y0, X0, X1, Y1, eave, pitch, facade_h, back_over=0.3, overhang=0.28):
    """The usual collision: the facade, the walls and the roof."""
    m.hull((XF1 - XF0, FT + 0.04, facade_h), at=((XF0 + XF1) * 0.5, Y0 + FT * 0.5, facade_h * 0.5))
    m.hull((X1 - X0 + 0.1, Y1 - Y0 - FT, eave), at=((X0 + X1) * 0.5, (Y0 + FT + Y1) * 0.5, eave * 0.5))
    tan = math.tan(math.radians(pitch))
    ridge = eave + (X1 - X0) * 0.5 * tan + 0.25
    low = eave - overhang * tan + 0.2
    m.hull_points([(x, y, low) for x in (X0 - overhang, X1 + overhang) for y in (Y0 + FT, Y1 + back_over)] +
                  [((X0 + X1) * 0.5, y, ridge) for y in (Y0 + FT, Y1 + back_over)])


def roof_height(X0, X1, eave, pitch):
    """The roof's top surface over world x (for the props behind a facade)."""
    tan = math.tan(math.radians(pitch))
    half = (X1 - X0) * 0.5
    mid = (X0 + X1) * 0.5
    return lambda x: eave + max(0.0, half - abs(x - mid)) * tan + 0.16


def arch(x0, x1, z0, rise, segments=8):
    """Points of a segmental arch from (x0, z0) to (x1, z0), rising by rise in the middle (both ends included)."""
    chord = x1 - x0
    radius = (chord * chord * 0.25 + rise * rise) / (2.0 * rise)
    cx, cz = (x0 + x1) * 0.5, z0 + rise - radius
    a0 = math.asin(chord * 0.5 / radius)
    return [(cx + radius * math.sin(-a0 + 2.0 * a0 * k / segments), cz + radius * math.cos(-a0 + 2.0 * a0 * k / segments))
            for k in range(segments + 1)]


def urn(m, space, at, scale=1.0, look='woodblack'):
    """A turned urn finial standing on at."""
    s = scale
    town.lathe(m, [(0.0, 0.0), (0.13 * s, 0.0), (0.13 * s, 0.05 * s), (0.075 * s, 0.085 * s), (0.065 * s, 0.14 * s),
                   (0.13 * s, 0.21 * s), (0.155 * s, 0.29 * s), (0.125 * s, 0.37 * s), (0.065 * s, 0.41 * s),
                   (0.075 * s, 0.45 * s), (0.035 * s, 0.5 * s), (0.0, 0.53 * s)], at=at, sides=8, look=look, space=space)


# --- Bright & Daughter, Undertakers ---

def undertaker():
    m = town.Model('FalseFront_Undertaker', seed=301)
    rng = m.rng
    XF0, XF1, Y0, Y1 = -3.5, 3.5, -6.0, 6.0
    X0, X1 = -3.2, 3.2
    YF = Y0 + FT
    WALL, PITCH = 3.3, 38.0
    EAVE = BASE + WALL
    WF = XF1 - XF0
    FACADE_Y[m.name] = Y0
    # The parapet: flat shoulders and a segmental arch over the middle, all above the ridge (5.8 m over the floor).
    SHOULDER = 6.15
    top = [(0.0, SHOULDER), (1.75, SHOULDER)] + arch(1.75, 5.25, SHOULDER, 0.8)[1:-1] + [(5.25, SHOULDER),
                                                                                         (WF, SHOULDER)]
    TILLY = Opening('window', 0.75, 0.8, 1.6, 1.95, style='open', casing='woodblack', sash='woodblack', panes=(2, 2),
                    head='pediment', casing_w=0.14)
    RIGHT = Opening('window', 4.65, 0.8, 1.6, 1.95, style='glass', casing='woodblack', sash='woodblack', panes=(2, 2),
                    head='pediment', casing_w=0.14)
    DOOR = Opening('door', 2.95, 0.0, 1.1, 2.78, style='panel', finish='woodblack', casing='woodblack', knob='brass',
                   transom=0.4, casing_w=0.15, panels='world_G')
    UPPER = [Opening('window', 1.25, 4.95, 0.66, 0.92, style='blind', casing='woodblack', sash='woodblack', panes=(1, 2),
                     casing_w=0.11),
             Opening('window', 5.09, 4.95, 0.66, 0.92, style='blind', casing='woodblack', sash='woodblack', panes=(1, 2),
                     casing_w=0.11)]
    items = [TILLY, RIGHT, DOOR] + UPPER
    front = false_front(m, XF0, XF1, Y0, top, items, covered=True)
    m.section('facade panel')

    # Clapboards in cream over the face, stopping at the casings, the pilasters and the sign board.
    outline = [(0.0, -BASE), (WF, -BASE)] + list(reversed(top))
    keep = [(o.x - 0.16, o.x + o.w + 0.16, o.z - 0.1, o.z + o.h + (0.55 if o.opts.get('head') else 0.25))
            for o in items]
    keep += [(-0.1, 0.33, -1.0, 9.0), (WF - 0.33, WF + 0.1, -1.0, 9.0), (0.25, WF - 0.25, 3.12, 4.62)]
    lap = town.clapboards(m, front, outline, 0.3, 7.2, keep, look='band_G', exposure=0.2)
    # A black kick board along the foot, broken by the door.
    for a, b in ((0.0, DOOR.x - 0.15), (DOOR.x + DOOR.w + 0.15, WF)):
        town.board(m, (a, 0.0, 0.0), (b, 0.0, 0.0), 0.62, 0.05, look='woodblack', space=front, lift=0.025)
    # Pilasters at the corners, up to the sign band, and black corner boards on up to the shoulders.
    for x in (0.18, WF - 0.18):
        town.pilaster(m, front, x, 0.3, 3.1, width=0.3, look='woodblack', depth=0.07)
        town.board(m, (x, 0.0, 3.16), (x, 0.0, SHOULDER - 0.02), 0.3, 0.05, look='woodblack', space=front, lift=0.025)
    m.section('clapboards')

    town.openings(m, front, items, FT, out=lap)
    # Crepe: a bow and tails on the door, a swag over its head.
    town.crepe_bow(m, front, (DOOR.x + DOOR.w * 0.5, 0.0, 1.92), scale=1.4, tails=0.85, y=-0.02)
    town.crepe_swag(m, front, DOOR.x - 0.2, DOOR.x + DOOR.w + 0.2, DOOR.h + 0.42, sag=0.16, y=-0.12)
    m.section('openings')

    # The sign: a black field framed in black with a cream bead, gilt letters.
    sx0, sx1, sz0, sz1 = 0.3, WF - 0.3, 3.18, 4.52
    town.box(m, (sx1 - sx0, 0.05, sz1 - sz0), at=((sx0 + sx1) * 0.5, -lap - 0.025, (sz0 + sz1) * 0.5), look='woodblack',
             space=front, cuts=4)
    field_y = -lap - 0.05
    for z in (sz0 + 0.06, sz1 - 0.06):
        town.board(m, (sx0 - 0.06, field_y, z), (sx1 + 0.06, field_y, z), 0.14, 0.05, look='woodblack', space=front,
                   lift=0.025)
    for x in (sx0 + 0.02, sx1 - 0.02):
        town.board(m, (x, field_y, sz0 + 0.12), (x, field_y, sz1 - 0.12), 0.14, 0.05, look='woodblack', space=front,
                   lift=0.025)
    for z in (sz0 + 0.15, sz1 - 0.15):
        town.board(m, (sx0 + 0.12, field_y, z), (sx1 - 0.12, field_y, z), 0.025, 0.012, look='band_G', space=front,
                   lift=0.006)
    for x in (sx0 + 0.11, sx1 - 0.11):
        town.board(m, (x, field_y, sz0 + 0.14), (x, field_y, sz1 - 0.14), 0.025, 0.012, look='band_G', space=front,
                   lift=0.006)
    town.text(m, 'BRIGHT & DAUGHTER', (WF * 0.5, field_y, 4.1), height=0.36, width=5.6, space=front, look='brass',
              tol=0.028)
    town.text(m, 'UNDERTAKERS', (WF * 0.5, field_y, 3.56), height=0.25, space=front, look='brass', tol=0.028)
    # A ledge over the sign on small brackets.
    town.box(m, (WF - 0.2, 0.22, 0.1), at=(WF * 0.5, -lap - 0.11, 4.62), look='woodblack', space=front)
    for k in range(7):
        town.bracket(m, front, 0.45 + (WF - 0.9) * k / 6, 4.57, reach=0.17, drop_h=0.22, thick=0.06, look='woodblack',
                     out=lap)
    m.section('sign')

    # Over the ledge: an oval medallion with the firm's monogram between the two blind windows.
    mx, mz = WF * 0.5, 5.45
    ring = [(mx + 0.5 * math.cos(2.0 * math.pi * k / 16), mz + 0.36 * math.sin(2.0 * math.pi * k / 16))
            for k in range(16)]
    town.prism(m, ring, 0.04, look='woodblack', space=front, y=-lap - 0.04)
    inner = [(mx + 0.41 * math.cos(2.0 * math.pi * k / 16), mz + 0.28 * math.sin(2.0 * math.pi * k / 16))
             for k in range(16)]
    town.prism(m, inner, 0.01, look='band_G', space=front, y=-lap - 0.05)
    town.text(m, 'B&D', (mx, -lap - 0.05, mz), height=0.26, space=front, look='brass', tol=0.028)
    # The parapet's cap and cornice, urns on the shoulders and a ball over the arch.
    town.cap_profile(m, front, top, depth=0.24, height=0.14, look='woodblack', out=lap, back=FT + 0.06)
    town.cap_profile(m, front, [(x, z - 0.14) for x, z in top], depth=0.15, height=0.1, look='woodblack', out=lap,
                     back=0.0)
    for x in (0.18, WF - 0.18):
        urn(m, front, (x, -0.05, SHOULDER + 0.14), scale=1.1)
    apex = max(z for _, z in top)
    town.lathe(m, [(0.0, 0.0), (0.06, 0.0), (0.06, 0.04), (0.035, 0.07), (0.09, 0.14), (0.1, 0.2), (0.07, 0.27),
                   (0.0, 0.3)], at=(WF * 0.5, -0.05, apex + 0.14), sides=8, look='woodblack', space=front)
    m.section('parapet')

    # Behind Tilly's window: a shallow lit room. Pale walls (the black drapes and the coffin stand out on them), a plank
    # floor, the lamp.
    rx0, rx1, depth = 0.55, 2.55, 1.5
    ry0, ry1 = FT, FT + depth
    town.box(m, (rx1 - rx0, 0.04, 3.0), at=((rx0 + rx1) * 0.5, ry1 + 0.02, 1.5), look='world_G', space=front)
    for x in (rx0 - 0.02, rx1 + 0.02):
        town.box(m, (0.04, depth, 3.0), at=(x, (ry0 + ry1) * 0.5, 1.5), look='world_G', space=front)
    town.box(m, (rx1 - rx0, depth, 0.04), at=((rx0 + rx1) * 0.5, (ry0 + ry1) * 0.5, 3.0), look='woodblack', space=front)
    for k in range(int((rx1 - rx0) / 0.2)):
        x = rx0 + 0.1 + 0.2 * k
        town.board(m, (x, ry0, -0.02), (x, ry1, -0.02), 0.19, 0.04, look='A', face=(0.0, 0.0, 1.0), space=front)
    # Drapes gathered at each side of the window, and a valance across its head.
    for x_a, x_b in ((TILLY.x - 0.08, TILLY.x + 0.3), (TILLY.x + TILLY.w - 0.3, TILLY.x + TILLY.w + 0.08)):
        rows = []
        for j in range(5):
            z = TILLY.z - 0.02 + (TILLY.h + 0.05) * j / 4
            squeeze = 0.65 + 0.35 * (j / 4)   # gathered toward the bottom where they're tied back
            row = []
            for i in range(9):
                t = i / 8
                mid = (x_a + x_b) * 0.5
                x = mid + (x_a + (x_b - x_a) * t - mid) * squeeze
                row.append((x, FT + 0.12 + 0.035 * math.sin(t * math.pi * 4.0), z))
            rows.append(row)
        town.sheet(m, rows, look='crepe', space=front)
    town.crepe_swag(m, front, TILLY.x - 0.05, TILLY.x + TILLY.w + 0.05, TILLY.z + TILLY.h + 0.02, sag=0.1, width=0.2,
                    y=FT + 0.1)
    # The lamp on a small round table: a brass font, and a glowing globe.
    lx, ly = TILLY.x + TILLY.w * 0.36, FT + 0.55
    town.lathe(m, [(0.0, 0.0), (0.2, 0.0), (0.2, 0.03), (0.04, 0.05), (0.035, 0.6), (0.24, 0.62), (0.24, 0.66),
                   (0.0, 0.67)], at=(lx, ly, 0.2), sides=8, look='woodblack', space=front)
    town.lathe(m, [(0.0, 0.0), (0.075, 0.0), (0.075, 0.02), (0.03, 0.05), (0.025, 0.09), (0.085, 0.14), (0.08, 0.2),
                   (0.03, 0.23), (0.0, 0.24)], at=(lx, ly, 0.87), sides=8, look='brass', space=front)
    town.lathe(m, [(0.0, 0.0), (0.04, 0.0), (0.095, 0.06), (0.11, 0.13), (0.09, 0.21), (0.05, 0.25), (0.0, 0.26)],
               at=(lx, ly, 1.1), sides=8, look='glow', space=front)
    lamp = front.world((lx, ly, 1.23))
    m.socket('Light', lamp)
    # A coffin stood on its foot against the back wall, for show.
    cx = TILLY.x + TILLY.w * 0.66
    coffin = [(-0.19, 0.0), (0.19, 0.0), (0.29, 1.3), (0.25, 1.88), (-0.25, 1.88), (-0.29, 1.3)]
    town.prism(m, [(cx + x, z + 0.02) for x, z in coffin], 0.3, look='woodblack', space=front, y=ry1 - 0.32)
    town.prism(m, [(cx + x * 0.8, 0.12 + z * 0.9) for x, z in coffin], 0.02, look='woodblack', space=front, y=ry1 - 0.34)
    for z in (0.7, 1.4):
        town.box(m, (0.5 if z > 1.0 else 0.44, 0.03, 0.035), at=(cx, ry1 - 0.35, z), look='brass', space=front)
    # The card on a string from the raised sash: "Back after the funeral".
    card_y = FT * 0.55 - 0.1
    card_z = TILLY.z + TILLY.h * 0.5 - 0.27
    for dx in (-0.1, 0.1):
        town.box(m, (0.006, 0.006, 0.2), at=(lx + dx * 0.9 + 0.02, card_y + 0.004, card_z + 0.18), rot=(0.0, dx * 40.0, 0.0),
                 look='band_G', space=front)
    town.box(m, (0.31, 0.008, 0.19), at=(lx + 0.02, card_y, card_z), rot=(0.0, 1.5, 0.0), look='band_G', space=front)
    town.text(m, 'Back after', (lx + 0.02, card_y - 0.004, card_z + 0.042), width=0.25, height=0.065, space=front,
              look='woodblack', tol=0.05, rot=1.5)
    town.text(m, 'the funeral', (lx + 0.02, card_y - 0.004, card_z - 0.045), width=0.26, height=0.07, space=front,
              look='woodblack', tol=0.05, rot=1.5)
    m.socket('Speaker', front.world((TILLY.x + TILLY.w * 0.5, -0.12, TILLY.z + TILLY.h * 0.3)))
    m.section('window room')

    # The walls behind, the back door and its loading step, the roof and a fieldstone chimney.
    side_window = dict(style='glass', casing='A', panes=(2, 2))
    sides = {'right': [Opening('window', 3.4, 0.95, 0.9, 1.3, **side_window),
                       Opening('window', 8.6, 0.95, 0.9, 1.3, style='boarded')],
             'left': [Opening('window', 4.2, 0.95, 0.9, 1.3, **side_window)]}
    BACK_DOOR = Opening('door', 3.9, 0.0, 1.05, 2.15, hinge='right', casing='C')
    back = [BACK_DOOR, Opening('window', 1.3, 1.0, 0.8, 1.1, style='glass', casing='A', panes=(2, 2)),
            Opening('window', 2.78, 3.7, 0.84, 0.75, style='boarded')]
    spaces = shell(m, X0, X1, YF, Y1, WALL, PITCH, sides, back)
    m.section('walls')
    # The loading step: a plank platform at the floor on a sill, with one step down to the yard.
    bs = spaces['back']
    step_x0, step_x1 = BACK_DOOR.x - 0.35, BACK_DOOR.x + BACK_DOOR.w + 0.35
    for k in range(int(round((step_x1 - step_x0) / 0.2))):
        x = step_x0 + 0.1 + 0.2 * k
        town.board(m, (x, -1.05, 0.0 - 0.025 + rng.uniform(-0.004, 0.0)), (x, 0.02, -0.025), 0.19, 0.05, look='A',
                   face=(0.0, 0.0, 1.0), space=bs)
    for y in (-1.0, -0.1):
        town.box(m, (step_x1 - step_x0, 0.1, BASE - 0.05), at=((step_x0 + step_x1) * 0.5, y, -BASE * 0.5 - 0.05 + 0.02),
                 look='C', space=bs)
    for x in (step_x0 + 0.03, step_x1 - 0.03):
        town.board(m, (x, -1.06, -0.2), (x, 0.0, -0.2), 0.3, 0.05, look='A', face=(1.0 if x > 1.0 else -1.0, 0.0, 0.0),
                   space=bs)
    tread_z = -BASE * 0.5
    for k in range(2):
        town.board(m, (step_x0 + 0.15, -1.14 - 0.15 * k, tread_z - 0.025), (step_x1 - 0.15, -1.14 - 0.15 * k,
                   tread_z - 0.025), 0.15, 0.05, look='A', face=(0.0, 0.0, 1.0), space=bs)
    town.board(m, (step_x0 + 0.15, -1.36, tread_z * 0.5 - BASE * 0.25), (step_x1 - 0.15, -1.36, tread_z * 0.5 - BASE * 0.25),
               BASE * 0.5 - 0.02, 0.03, look='A', space=bs)
    m.section('loading step')
    CHIMNEY = (-1.7, 3.6)
    town.hidden_gable(m, X0, X1, Y0 + FT * 0.5, Y1 + 0.35, EAVE, PITCH, covering='shakes', overhang=0.3, sag=0.07,
                      chimney=(CHIMNEY[0], CHIMNEY[1], 0.42, 0.4))
    m.section('roof')
    roof_z = roof_height(X0, X1, EAVE, PITCH)
    chimney_top = EAVE + (X1 - X0) * 0.5 * math.tan(math.radians(PITCH)) + 0.7
    foot = roof_z(CHIMNEY[0]) - 0.7
    kit.stone_stack(m, (CHIMNEY[0], CHIMNEY[1], foot), (0.6, 0.7), chimney_top - foot, taper=0.03)
    m.box((0.76, 0.86, 0.1), at=(CHIMNEY[0], CHIMNEY[1], chimney_top + 0.05), uv=Trim('D', world=True), bevel=0.02)
    m.box((0.34, 0.42, 0.16), at=(CHIMNEY[0], CHIMNEY[1], chimney_top + 0.18), uv='D')
    m.socket('Smoke', (CHIMNEY[0], CHIMNEY[1], chimney_top + 0.3))
    town.facade_back(m, front, WF, top, lambda x: roof_z(XF0 + x) - BASE, FT)
    # A stone footing under the facade's returns, where the boardwalk doesn't reach.
    for x in (XF0 + 0.16, XF1 - 0.16):
        m.box((0.34, FT + 0.12, BASE + 0.05), at=(x, Y0 + FT * 0.5, BASE * 0.5 - 0.02), uv=Trim('D', world=True))
    m.section('chimney, facade back')

    hulls(m, XF0, XF1, Y0, X0, X1, Y1, EAVE, PITCH, BASE + SHOULDER + 0.14, back_over=0.35, overhang=0.3)
    m.hull_points([(XF0 + x, y, BASE + z + 0.14) for x, z in top if 1.7 < x < 5.3 for y in (Y0 - 0.1, Y0 + FT)] +
                  [(XF0 + x, y, BASE + SHOULDER) for x in (1.75, 5.25) for y in (Y0 - 0.1, Y0 + FT)])
    m.hull((step_x1 - step_x0, 1.1, BASE), at=(X1 - (step_x0 + step_x1) * 0.5, Y1 + 0.55, BASE * 0.5))
    m.hull((step_x1 - step_x0 - 0.3, 0.32, BASE * 0.5), at=(X1 - (step_x0 + step_x1) * 0.5, Y1 + 1.24, BASE * 0.25))
    m.hull((0.62, 0.72, chimney_top - foot + 0.3), at=(CHIMNEY[0], CHIMNEY[1], (foot + chimney_top) * 0.5))
    return m.finish(view=(-1.0, -1.7, 0.42), fit=0.78, fallback=FALLBACK)


# --- The Gilt Spur ---

def saloon():
    m = town.Model('FalseFront_Saloon', seed=302)
    rng = m.rng
    XF0, XF1, Y0, Y1 = -4.2, 4.2, -7.0, 7.0
    X0, X1 = -3.9, 3.9
    YF = Y0 + FT
    WALL, PITCH = 6.0, 18.0          # two storeys of 3 m
    EAVE = BASE + WALL
    WF = XF1 - XF0
    FACADE_Y[m.name] = Y0
    UP = 3.0                          # the second floor over the first
    top = [(0.0, 6.75), (1.4, 6.75), (1.4, 7.15), (2.7, 7.15), (2.7, 7.55), (5.7, 7.55), (5.7, 7.15), (7.0, 7.15),
           (7.0, 6.75), (WF, 6.75)]
    boarded = dict(style='boarded', casing='A', boards='planks', panes=(1, 1))
    LEFT = Opening('window', 0.75, 0.75, 1.9, 1.8, **boarded)
    RIGHT = Opening('window', 5.75, 0.75, 1.9, 1.8, **boarded)
    DOORS = Opening('door', 3.55, 0.0, 1.3, 2.62, style='double', finish='planks', casing='C', knob='H3', transom=0.4,
                    glass=True)
    UP_LEFT = Opening('window', 1.0, UP + 0.75, 1.2, 1.5, **boarded)
    UP_RIGHT = Opening('window', 6.2, UP + 0.75, 1.2, 1.5, **boarded)
    UP_DOOR = Opening('door', 3.7, UP, 1.0, 2.2)           # the balcony door's hole; the door is built a storey up
    items = [LEFT, RIGHT, DOORS, UP_LEFT, UP_RIGHT, UP_DOOR]
    front = false_front(m, XF0, XF1, Y0, top, items, covered=True)
    m.section('facade panel')
    town.openings(m, front, [LEFT, RIGHT, DOORS, UP_LEFT, UP_RIGHT], FT)
    upper = kit.wall_space((XF0, Y0), (XF1, Y0), BASE + UP)
    town.door(m, upper, Opening('door', UP_DOOR.x, 0.0, UP_DOOR.w, UP_DOOR.h, style='plank', finish='A', casing='C'),
              FT)
    # Everything boarded: the doors and the balcony door get their planks too, and the doors the mourning board.
    town.board_up(m, front, DOORS.x, 0.0, DOORS.w, DOORS.h - 0.1, look='planks', out=0.07, level=False)
    town.board_up(m, upper, UP_DOOR.x, 0.0, UP_DOOR.w, UP_DOOR.h, look='planks', out=0.06)
    sign_z = 1.42
    town.board(m, (DOORS.x - 0.45, -0.17, sign_z - 0.06), (DOORS.x + DOORS.w + 0.45, -0.17, sign_z + 0.06), 0.36, 0.04,
               look='planks', space=front)
    for end in (-1.0, 1.0):
        x = DOORS.x + DOORS.w * 0.5 + end * (DOORS.w * 0.5 + 0.33)
        town.box(m, (0.035, 0.014, 0.035), at=(x, -0.196, sign_z + end * 0.05), look='H3', space=front)
    town.text(m, 'CLOSED FOR MOURNING', (DOORS.x + DOORS.w * 0.5, -0.19, sign_z - 0.005), height=0.15, width=2.0,
              space=front, look='black', tol=0.03, rot=-math.degrees(math.atan2(0.12, DOORS.w + 0.9)))
    # Corner boards up the facade.
    for x in (0.1, WF - 0.1):
        town.trim_board(m, front, (x, 0.0, -BASE), (x, 0.0, 6.6), 0.2, 'C', thick=0.05)
    m.section('openings')

    # The sign: oxide-red boards in a weathered frame, faded cream letters; a spur over it on the parapet.
    sx0, sx1, sz0, sz1 = 0.55, WF - 0.55, 5.42, 6.42
    count = 5
    for k in range(count):
        z = sz0 + 0.06 + (sz1 - sz0 - 0.12) * (k + 0.5) / count
        town.board(m, (sx0, -0.02, z), (sx1, -0.02, z), (sz1 - sz0 - 0.12) / count - 0.006, 0.035, look='H2',
                   space=front, lift=0.0)
    for z in (sz0 + 0.03, sz1 - 0.03):
        town.board(m, (sx0 - 0.08, -0.05, z), (sx1 + 0.08, -0.05, z), 0.12, 0.05, look='C', space=front)
    for x in (sx0 - 0.02, sx1 + 0.02):
        town.board(m, (x, -0.05, sz0 + 0.09), (x, -0.05, sz1 - 0.09), 0.12, 0.05, look='C', space=front)
    town.text(m, 'THE GILT SPUR', (WF * 0.5, -0.05, (sz0 + sz1) * 0.5), height=0.48, width=6.4, space=front,
              look='cream', tol=0.028)
    # The spur's rowel: an eight-pointed star round a hub, and the heel band under it.
    cx, cz = WF * 0.5, 7.06
    star = []
    for k in range(16):
        r = 0.3 if k % 2 == 0 else 0.11
        a = math.pi * 0.5 + math.pi * k / 8
        star.append((cx + r * math.cos(a), cz + r * math.sin(a)))
    town.prism(m, star, 0.03, look='cream', space=front, y=-0.03)
    town.prism(m, [(cx + 0.07 * math.cos(2 * math.pi * k / 8), cz + 0.07 * math.sin(2 * math.pi * k / 8))
                   for k in range(8)], 0.02, look='H2', space=front, y=-0.04)
    band = []
    for k in range(9):
        a = math.radians(200.0 + 140.0 * k / 8)
        band.append((cx + 0.62 * math.cos(a), cz + 0.12 + 0.42 * math.sin(a)))
    for k in range(8, -1, -1):
        a = math.radians(200.0 + 140.0 * k / 8)
        band.append((cx + 0.52 * math.cos(a), cz + 0.12 + 0.33 * math.sin(a)))
    town.prism(m, band, 0.03, look='cream', space=front, y=-0.03)
    # A cornice over the sign on brackets, and caps along the stepped parapet.
    town.cornice(m, front, 0.0, WF, 6.72, look='C', project=0.28, height=0.26, brackets=1.2, frieze=0.0)
    town.cap_profile(m, front, top, depth=0.16, height=0.12, look='C', back=FT + 0.05)
    m.section('sign, parapet')

    # The balcony over the boardwalk: a deck at the second floor on a beam and four posts, a railing of square
    # balusters, a board ceiling under it.
    porch = kit.wall_space((XF0, Y0), (XF1, Y0), 0.0)
    deck_z = BASE + UP + 0.06
    edge = -town.WALK_DEPTH
    post_y = edge + 0.16
    posts = [0.2, 2.95, WF - 2.95, WF - 0.2]
    rail_top = deck_z + 1.02
    # The deck runs across the facade; town.deck works in the model's frame, so it's built in world coordinates.
    town.deck(m, XF0, XF1, Y0 + edge, Y0 + 0.02, top=deck_z, posts=(), sill=False, joists=0, back=False, plank_cuts=0)
    town.box(m, (WF, -edge, 0.03), at=(0.0, Y0 + edge * 0.5, deck_z - 0.27), look='world_A', cuts=6)
    town.board(m, (XF0, Y0 + post_y, deck_z - 0.2), (XF1, Y0 + post_y, deck_z - 0.2), 0.26, 0.2, look='C',
               space=None, face=(0.0, -1.0, 0.0))
    for x in (XF0 + 0.05, XF1 - 0.05):
        town.board(m, (x, Y0 + edge, deck_z - 0.17), (x, Y0, deck_z - 0.17), 0.22, 0.06, look='C',
                   face=(1.0 if x > 0 else -1.0, 0.0, 0.0))
    for x in posts:
        town.board(m, (x, post_y, -0.05), (x, post_y, rail_top + 0.12), 0.18, 0.18, look='C', space=porch,
                   face=(0.0, -1.0, 0.0), bevel=0.015)
        town.box(m, (0.26, 0.26, 0.06), at=(x, post_y, rail_top + 0.15), look='C', space=porch)
        town.box(m, (0.24, 0.24, 0.05), at=(x, post_y, BASE + 0.02), look='C', space=porch)
        for side in (-1.0, 1.0):
            if 0.3 < x + side * 0.5 < WF - 0.3:
                town.board(m, (x + side * 0.08, post_y, deck_z - 0.75), (x + side * 0.6, post_y, deck_z - 0.3), 0.11,
                           0.09, look='C', space=porch)
    # Railing: rails along the front and the two ends, balusters between.
    runs = [((0.2, post_y), (WF - 0.2, post_y)), ((0.1, post_y), (0.1, -0.05)), ((WF - 0.1, post_y), (WF - 0.1, -0.05))]
    for (ax, ay), (bx, by) in runs:
        for z, h in ((rail_top - 0.04, 0.09), (deck_z + 0.13, 0.07)):
            town.board(m, (ax, ay, z), (bx, by, z), h, 0.07, look='C', space=porch,
                       face=(0.0, -1.0, 0.0) if ay == by else (-1.0 if ax < 1.0 else 1.0, 0.0, 0.0))
        span = math.hypot(bx - ax, by - ay)
        count = int(span / 0.26)
        for k in range(1, count):
            t = k / count
            px, py = ax + (bx - ax) * t, ay + (by - ay) * t
            if any(abs(px - p) < 0.15 for p in posts) and ay == by:
                continue
            town.box(m, (0.05, 0.05, rail_top - deck_z - 0.22), at=(px, py, (rail_top + deck_z) * 0.5 + 0.03),
                     look='C', space=porch, drop=('-z', '+z'))
    m.section('balcony')

    # The walls behind: two storeys of weathered siding, windows on both floors, half of them boarded.
    glass = dict(style='glass', casing='A', panes=(2, 2))
    sides = {'right': [Opening('window', 2.5, 0.9, 0.9, 1.4, **glass),
                       Opening('window', 8.2, 0.9, 0.9, 1.4, style='boarded', boards='planks'),
                       Opening('window', 2.5, UP + 0.8, 0.9, 1.3, style='boarded', boards='planks'),
                       Opening('window', 8.2, UP + 0.8, 0.9, 1.3, **glass)],
             'left': [Opening('window', 3.0, 0.9, 0.9, 1.4, style='boarded', boards='planks'),
                      Opening('window', 6.5, UP + 0.8, 0.9, 1.3, **glass)]}
    back = [Opening('door', 4.9, 0.0, 1.0, 2.15, casing='C', hinge='left'),
            Opening('window', 1.6, 0.95, 0.9, 1.3, style='boarded', boards='planks'),
            Opening('window', 3.45, UP + 0.8, 0.9, 1.3, **glass)]
    spaces = shell(m, X0, X1, YF, Y1, WALL, PITCH, sides, back)
    town.board_up(m, spaces['back'], 4.9, 0.0, 1.0, 2.15, look='planks', out=0.07)
    # A belt course between the storeys on the side walls.
    for name in ('right', 'left'):
        span = Y1 - YF
        town.trim_board(m, spaces[name], (0.1, 0.0, UP - 0.12), (span - 0.1, 0.0, UP - 0.12), 0.18, 'C', thick=0.05)
    m.section('walls')
    town.hidden_gable(m, X0, X1, Y0 + FT * 0.5, Y1 + 0.3, EAVE, PITCH, covering='shakes', overhang=0.28, sag=0.06)
    roof_z = roof_height(X0, X1, EAVE, PITCH)
    town.facade_back(m, front, WF, top, lambda x: roof_z(XF0 + x) - BASE, FT)
    for x in (XF0 + 0.16, XF1 - 0.16):
        m.box((0.34, FT + 0.12, BASE + 0.05), at=(x, Y0 + FT * 0.5, BASE * 0.5 - 0.02), uv=Trim('D', world=True))
    m.section('roof')

    hulls(m, XF0, XF1, Y0, X0, X1, Y1, EAVE, PITCH, BASE + 6.87, back_over=0.3, overhang=0.28)
    m.hull((5.7 - 1.4, FT + 0.04, 0.4), at=(XF0 + 4.2, Y0 + FT * 0.5, BASE + 6.87 + 0.2))
    m.hull((3.0, FT + 0.04, 0.42), at=(XF0 + 4.2, Y0 + FT * 0.5, BASE + 7.27 + 0.21))
    m.hull((WF, -edge, rail_top + 0.1 - (deck_z - 0.3)), at=(0.0, Y0 + edge * 0.5,
                                                             (rail_top + 0.1 + deck_z - 0.3) * 0.5))
    for x in posts:
        m.hull((0.2, 0.2, deck_z - 0.3), at=(XF0 + x, Y0 + post_y, (deck_z - 0.3) * 0.5))
    return m.finish(view=(-1.0, -1.7, 0.42), fit=0.78, fallback=FALLBACK)


# --- Pruitt's General Store ---

def store(shutters):
    m = town.Model('FalseFront_Store', seed=303)
    rng = m.rng
    XF0, XF1, Y0, Y1 = -4.7, 4.7, -6.5, 6.5
    X0, X1 = -4.4, 4.4
    YF = Y0 + FT
    WALL, PITCH = 3.8, 24.0
    EAVE = BASE + WALL
    WF = XF1 - XF0
    FACADE_Y[m.name] = Y0
    TOP = 6.45
    # The widest front on the street: flat, under a heavy bracketed cornice, a low pediment in the middle.
    top = [(0.0, TOP), (3.5, TOP), (WF * 0.5, TOP + 0.5), (WF - 3.5, TOP), (WF, TOP)]
    display = dict(style='glass', casing='H1', sash='H1', panes=(3, 2), casing_w=0.14)
    LEFT = Opening('window', 0.62, 0.6, 2.85, 2.0, **display)
    RIGHT = Opening('window', WF - 0.62 - 2.85, 0.6, 2.85, 2.0, **display)
    DOORS = Opening('door', WF * 0.5 - 0.65, 0.0, 1.3, 2.85, style='double', finish='planks', casing='H1', knob='H3',
                    transom=0.4, glass=True)
    items = [LEFT, RIGHT, DOORS]
    front = false_front(m, XF0, XF1, Y0, top, items)
    m.section('facade panel')
    town.openings(m, front, items, FT)
    # Bulkheads under the display windows: two teal boards each, and pilasters at the corners.
    for o in (LEFT, RIGHT):
        for z in (0.16, 0.38):
            town.board(m, (o.x - 0.05, 0.0, z), (o.x + o.w + 0.05, 0.0, z), 0.2, 0.04, look='H1', space=front, lift=0.02)
    for x in (0.15, WF - 0.15):
        town.pilaster(m, front, x, 0.0, 3.25, width=0.28, look='C', depth=0.06)
    m.section('openings')

    # The sign: weathered boards in a teal frame, cream letters.
    sx0, sx1, sz0, sz1 = 0.55, WF - 0.55, 3.6, 5.24
    rows = 8
    for k in range(rows):
        z = sz0 + 0.02 + (sz1 - sz0 - 0.04) * (k + 0.5) / rows
        town.board(m, (sx0, 0.0, z), (sx1, 0.0, z), (sz1 - sz0 - 0.04) / rows - 0.005, 0.035, look='A', space=front,
                   lift=0.0175)
    field_y = -0.035
    for z in (sz0, sz1):
        town.board(m, (sx0 - 0.1, field_y, z), (sx1 + 0.1, field_y, z), 0.14, 0.05, look='H1', space=front, lift=0.025)
    for x in (sx0 - 0.03, sx1 + 0.03):
        town.board(m, (x, field_y, sz0 + 0.07), (x, field_y, sz1 - 0.07), 0.14, 0.05, look='H1', space=front,
                   lift=0.025)
    town.text(m, "PRUITT'S", (WF * 0.5, field_y, 4.74), height=0.62, width=6.5, space=front, look='cream', tol=0.028)
    town.text(m, 'GENERAL STORE', (WF * 0.5, field_y, 3.98), height=0.34, width=6.8, space=front, look='cream',
              tol=0.028)
    town.cornice(m, front, 0.0, WF, TOP, look='C', project=0.32, height=0.3, brackets=1.05, frieze=0.3)
    town.cap_profile(m, front, top[1:4], depth=0.2, height=0.12, look='C', back=FT + 0.05)
    m.section('sign, cornice')

    # The deep porch: a plank deck like the boardwalk's (it chains with it), four posts with knee braces, a beam and a
    # shake roof running back to the facade under the sign.
    deck_front = Y0 - town.WALK_DEPTH
    town.deck(m, XF0, XF1, deck_front, Y0, posts=[XF0 + 1.0 + 2.0 * k for k in range(5)], back=False, plank_cuts=0)
    porch = kit.wall_space((XF0, Y0), (XF1, Y0), 0.0)
    post_y = -town.WALK_DEPTH + 0.18
    beam_z = 3.08
    posts = [0.22, 3.25, WF - 3.25, WF - 0.22]
    for x in posts:
        town.board(m, (x, post_y, town.DECK_TOP), (x, post_y, beam_z), 0.16, 0.16, look='C', space=porch, bevel=0.015)
        town.box(m, (0.22, 0.22, 0.05), at=(x, post_y, town.DECK_TOP + 0.025), look='C', space=porch)
        for side in (-1.0, 1.0):
            if 0.3 < x + side * 0.5 < WF - 0.3:
                town.board(m, (x + side * 0.07, post_y, beam_z - 0.55), (x + side * 0.5, post_y, beam_z - 0.05), 0.1,
                           0.09, look='C', space=porch)
    town.board(m, (-0.1, post_y, beam_z + 0.11), (WF + 0.1, post_y, beam_z + 0.11), 0.22, 0.18, look='C', space=porch)
    wall_z = BASE + 3.42
    theta = math.atan2(wall_z - (beam_z + 0.22), -post_y)
    p_deck, p_over = 0.06, 0.34
    normal = Vector((0.0, -math.sin(theta), math.cos(theta)))
    origin = Vector((XF0 - 0.12, Y0 + post_y - p_over, beam_z + 0.22 - p_over * math.tan(theta))) + normal * p_deck
    p_len = (-post_y + p_over) / math.cos(theta)
    porch_roof = kit.Slope(origin, (1.0, 0.0, 0.0), (0.0, math.cos(theta), math.sin(theta)), WF + 0.24, p_len, sag=0.05)
    town.roof_deck(m, porch_roof, p_deck)
    town.shakes(m, porch_roof, piece=(4.5, 6.0), top=p_len - 0.03)
    town.board(m, (XF0 - 0.12, Y0 - 0.03, wall_z + 0.06), (XF1 + 0.12, Y0 - 0.03, wall_z + 0.06), 0.16, 0.05, look='C')
    fascia_z = beam_z + 0.22 - p_over * math.tan(theta) - 0.02
    town.board(m, (XF0 - 0.14, Y0 + post_y - p_over - 0.02, fascia_z), (XF1 + 0.14, Y0 + post_y - p_over - 0.02, fascia_z),
               0.2, 0.04, look='H1')
    m.section('porch')

    # The walls behind: windows down both sides with shutters (their sockets and pintles), a back door.
    side = dict(style='glass', casing='H1', sash='H1', panes=(2, 2))
    sides = {'right': [Opening('window', 2.6, 1.0, 0.74, 1.3, **side), Opening('window', 8.4, 1.0, 0.74, 1.3, **side)],
             'left': [Opening('window', 2.6, 1.0, 0.74, 1.3, **side), Opening('window', 8.4, 1.0, 0.74, 1.3, **side)]}
    BACK_DOOR = Opening('door', 5.6, 0.0, 1.05, 2.2, casing='C', hinge='left')
    back = [BACK_DOOR, Opening('window', 1.6, 1.0, 0.9, 1.2, style='boarded', casing='A')]
    spaces = shell(m, X0, X1, YF, Y1, WALL, PITCH, sides, back)
    number = 1
    for name in ('left', 'right'):
        for o in sorted(spaces[name].items, key=lambda o: o.x):
            shutter_sockets(m, spaces[name], o, number)
            number += 1
    bs = spaces['back']
    town.box(m, (BACK_DOOR.w + 0.5, 0.42, BASE * 0.5), at=(BACK_DOOR.x + BACK_DOOR.w * 0.5, -0.21, -BASE * 0.75),
             look='world_D', space=bs, bevel=0.02)
    m.section('walls')

    # The roof, and a stovepipe through it near the back with a rain cap.
    PIPE = (2.4, 3.9)
    town.hidden_gable(m, X0, X1, Y0 + FT * 0.5, Y1 + 0.3, EAVE, PITCH, overhang=0.28, sag=0.06,
                      chimney=(PIPE[0], PIPE[1], 0.24, 0.24))
    roof_z = roof_height(X0, X1, EAVE, PITCH)
    pipe_base = roof_z(PIPE[0]) - 0.35
    pipe_top = roof_z(PIPE[0]) + 1.25
    town.cylinder(m, (PIPE[0], PIPE[1], pipe_base), (PIPE[0], PIPE[1], pipe_top), 0.1, sides=10, look='iron',
                  caps=(False, True))
    town.cylinder(m, (PIPE[0], PIPE[1], roof_z(PIPE[0]) - 0.12), (PIPE[0], PIPE[1], roof_z(PIPE[0]) + 0.1), 0.22, 0.11,
                  sides=10, look='iron', caps=(False, False))
    town.cylinder(m, (PIPE[0], PIPE[1], pipe_top + 0.12), (PIPE[0], PIPE[1], pipe_top + 0.3), 0.26, 0.03, sides=10,
                  look='iron')
    for k in range(3):
        a = math.radians(90.0 + 120.0 * k)
        town.box(m, (0.02, 0.02, 0.16), at=(PIPE[0] + 0.09 * math.cos(a), PIPE[1] + 0.09 * math.sin(a), pipe_top + 0.06),
                 look='iron')
    m.socket('Smoke', (PIPE[0], PIPE[1], pipe_top + 0.08))
    town.facade_back(m, front, WF, top, lambda x: roof_z(XF0 + x) - BASE, FT)
    for x in (XF0 + 0.16, XF1 - 0.16):
        m.box((0.34, FT + 0.12, BASE + 0.05), at=(x, Y0 + FT * 0.5, BASE * 0.5 - 0.02), uv=Trim('D', world=True))
    m.section('roof')

    hulls(m, XF0, XF1, Y0, X0, X1, Y1, EAVE, PITCH, BASE + TOP, back_over=0.3, overhang=0.28)
    m.hull_points([(XF0 + x, y, BASE + z + 0.12) for x, z in top[1:4] for y in (Y0 - 0.1, Y0 + FT)] +
                  [(XF0 + x, y, BASE + TOP - 0.1) for x in (3.5, WF - 3.5) for y in (Y0 - 0.1, Y0 + FT)])
    m.hull((WF, town.WALK_DEPTH, town.DECK_TOP), at=(0.0, Y0 - town.WALK_DEPTH * 0.5, town.DECK_TOP * 0.5))
    for x in posts:
        m.hull((0.18, 0.18, beam_z - town.DECK_TOP), at=(XF0 + x, Y0 + post_y, (beam_z + town.DECK_TOP) * 0.5))
    m.hull((porch_roof.width, p_len, 0.2), at=(porch_roof.width * 0.5, p_len * 0.5, -0.02), space=porch_roof)
    obj = m.finish(preview=False, fallback=FALLBACK)
    if lt.want_preview():
        # Seen with its shutters hung: some swung open against the wall, some shut.
        copies = mount_shutters(obj, shutters)
        lt.preview([obj] + copies, lt.preview_path('Buildings', m.name), view=(-1.0, -1.7, 0.42), fit=0.78)
        for copy in copies:
            kit.bpy.data.objects.remove(copy)
    return obj


# --- The sheriff's office ---

def sheriff():
    m = town.Model('FalseFront_Sheriff', seed=304)
    rng = m.rng
    XF0, XF1, Y0, Y1 = -3.3, 3.3, -5.0, 5.0       # the wooden false front
    X0, X1 = -3.05, 3.05                          # the stone storey and walls
    STONE_T = 0.3
    WALL, PITCH = 3.3, 32.0
    EAVE = BASE + WALL
    WF, WS = XF1 - XF0, X1 - X0
    FACADE_Y[m.name] = Y0
    LOW = WALL                                    # the stone storey's top, over the floor
    top = [(0.0, 5.85), (2.2, 5.85), (2.2, 6.3), (WF - 2.2, 6.3), (WF - 2.2, 5.85), (WF, 5.85)]
    # The stone storey: a jail's walls, stone jambs and lintels round a heavy plank door and two barred windows.
    stone_front = kit.wall_space((X0, Y0), (X1, Y0), BASE)
    DOOR = Opening('door', WS * 0.5 - 0.5, 0.0, 1.0, 2.25, style='plank', finish='A', casing='D', hinge='right',
                   casing_w=0.18)
    barred = dict(style='barred', casing='D', panes=(1, 1), bars=6, casing_w=0.16)
    WIN_L = Opening('window', 0.75, 0.95, 0.9, 1.2, **barred)
    WIN_R = Opening('window', WS - 0.75 - 0.9, 0.95, 0.9, 1.2, **barred)
    items = [DOOR, WIN_L, WIN_R]
    outline = [(0.0, -BASE), (WS, -BASE), (WS, LOW), (0.0, LOW)]
    town.panel(m, outline, [kit.rect(o.x, max(o.z, 0.0), o.w, o.h) for o in items], STONE_T,
               Trim('D', world=True, u=rng.uniform(0.0, 6.4)), space=stone_front, around=items, slices=0, back=False)
    town.openings(m, stone_front, items, STONE_T)
    # Quoins: dressed blocks up both corners, long and short in turn, wrapping round onto the side walls.
    for x_edge, sign in ((0.0, 1.0), (WS, -1.0)):
        z, k = -BASE + 0.05, 0
        while z < LOW - 0.12:
            h = min(0.34 if k % 2 == 0 else 0.26, LOW - z)
            length = 0.5 if k % 2 == 0 else 0.32
            town.box(m, (length, 0.05, h - 0.025), at=(x_edge + sign * (length * 0.5 - 0.02), -0.025, z + h * 0.5),
                     look='D', space=stone_front)
            town.box(m, (0.05, 0.32 if k % 2 == 0 else 0.5, h - 0.025),
                     at=(x_edge - sign * 0.025, (0.32 if k % 2 == 0 else 0.5) * 0.5 - 0.04, z + h * 0.5), look='D',
                     space=stone_front)
            z += h
            k += 1
    # Black crepe on the door: a bow and tails, and a swag over the lintel.
    town.crepe_bow(m, stone_front, (DOOR.x + DOOR.w * 0.5, 0.0, 1.78), scale=1.1, tails=0.7, y=STONE_T * 0.3 - 0.07)
    town.crepe_swag(m, stone_front, DOOR.x - 0.3, DOOR.x + DOOR.w + 0.3, DOOR.h + 0.34, sag=0.15, y=-0.1)
    m.section('stone storey')

    # The wooden false front on the stone: siding, the sign, a star in the raised middle, a molded cornice.
    upper = kit.wall_space((XF0, Y0), (XF1, Y0), BASE + LOW)
    up_top = [(x, z - LOW) for x, z in top]
    town.panel(m, [(0.0, 0.0), (WF, 0.0)] + list(reversed(up_top)), (), FT, siding(rng), space=upper, slices=0)
    town.board(m, (-0.04, 0.0, 0.03), (WF + 0.04, 0.0, 0.03), 0.22, 0.09, look='C', space=upper, lift=0.045)
    for x in (0.1, WF - 0.1):
        town.trim_board(m, upper, (x, 0.0, 0.14), (x, 0.0, up_top[0][1] - 0.3), 0.2, 'C', thick=0.05)
    sx0, sx1, sz0, sz1 = 1.0, WF - 1.0, 0.42, 1.26
    for k in range(4):
        z = sz0 + 0.02 + (sz1 - sz0 - 0.04) * (k + 0.5) / 4
        town.board(m, (sx0, 0.0, z), (sx1, 0.0, z), (sz1 - sz0 - 0.04) / 4 - 0.005, 0.035, look='H1', space=upper,
                   lift=0.0175)
    field_y = -0.035
    for z in (sz0, sz1):
        town.board(m, (sx0 - 0.09, field_y, z), (sx1 + 0.09, field_y, z), 0.12, 0.05, look='C', space=upper, lift=0.025)
    for x in (sx0 - 0.03, sx1 + 0.03):
        town.board(m, (x, field_y, sz0 + 0.06), (x, field_y, sz1 - 0.06), 0.12, 0.05, look='C', space=upper, lift=0.025)
    town.text(m, 'SHERIFF', (WF * 0.5, field_y, (sz0 + sz1) * 0.5), height=0.46, width=3.9, space=upper, look='cream',
              tol=0.028)
    star = []
    for k in range(10):
        r = 0.21 if k % 2 == 0 else 0.085
        a = math.pi * 0.5 + math.pi * k / 5
        star.append((WF * 0.5 + r * math.cos(a), 2.62 + r * math.sin(a)))
    town.prism(m, star, 0.03, look='cream', space=upper, y=-0.03)
    town.cornice(m, upper, 0.0, WF, up_top[0][1], look='C', project=0.24, height=0.22, brackets=0, frieze=0.2)
    town.cap_profile(m, upper, up_top[1:5], depth=0.18, height=0.1, look='C', back=FT + 0.04)
    m.section('false front')

    # Stone walls behind, small barred cell windows, an iron-strapped back door; a shake roof and a stovepipe.
    cell = dict(style='barred', casing='D', panes=(1, 1), bars=4, casing_w=0.12)
    sides = {'right': [Opening('window', 2.3, 1.55, 0.7, 0.55, **cell), Opening('window', 6.4, 1.55, 0.7, 0.55, **cell)],
             'left': [Opening('window', 4.4, 1.55, 0.7, 0.55, **cell)]}
    back = [Opening('door', 3.6, 0.0, 1.0, 2.15, style='plank', finish='A', casing='D', hinge='left'),
            Opening('window', 1.0, 1.5, 0.7, 0.55, **cell)]
    shell(m, X0, X1, Y0 + STONE_T, Y1, WALL, PITCH, sides, back, stone=True)
    m.section('walls')
    PIPE = (-1.3, 2.9)
    town.hidden_gable(m, X0, X1, Y0 + FT * 0.5, Y1 + 0.3, EAVE, PITCH, overhang=0.3, sag=0.05, rake=0.3,
                      chimney=(PIPE[0], PIPE[1], 0.24, 0.24))
    roof_z = roof_height(X0, X1, EAVE, PITCH)
    pipe_top = roof_z(PIPE[0]) + 1.1
    town.cylinder(m, (PIPE[0], PIPE[1], roof_z(PIPE[0]) - 0.35), (PIPE[0], PIPE[1], pipe_top), 0.09, sides=10,
                  look='iron', caps=(False, True))
    town.cylinder(m, (PIPE[0], PIPE[1], pipe_top + 0.1), (PIPE[0], PIPE[1], pipe_top + 0.26), 0.22, 0.03, sides=10,
                  look='iron')
    for k in range(3):
        a = math.radians(90.0 + 120.0 * k)
        town.box(m, (0.02, 0.02, 0.14), at=(PIPE[0] + 0.08 * math.cos(a), PIPE[1] + 0.08 * math.sin(a), pipe_top + 0.05),
                 look='iron')
    m.socket('Smoke', (PIPE[0], PIPE[1], pipe_top + 0.07))
    town.facade_back(m, upper, WF, up_top, lambda x: roof_z(XF0 + x) - BASE - LOW, FT)
    m.section('roof')

    m.hull((WS + 0.1, Y1 - Y0, EAVE), at=(0.0, 0.0, EAVE * 0.5))
    m.hull((WF, FT + 0.06, 5.85 - LOW + 0.12), at=(0.0, Y0 + FT * 0.5, EAVE + (5.85 - LOW + 0.12) * 0.5))
    m.hull((WF - 4.4, FT + 0.04, 0.55), at=(0.0, Y0 + FT * 0.5, BASE + 5.85 + 0.27))
    tan = math.tan(math.radians(PITCH))
    m.hull_points([(x, y, EAVE - 0.3 * tan + 0.2) for x in (X0 - 0.3, X1 + 0.3) for y in (Y0 + FT, Y1 + 0.3)] +
                  [(0.0, y, EAVE + WS * 0.5 * tan + 0.25) for y in (Y0 + FT, Y1 + 0.3)])
    return m.finish(view=(-1.0, -1.7, 0.42), fit=0.78, fallback=FALLBACK)


def mount_shutters(building, shutters, open_every=2):
    """Copies of the shutters hung on a building's sockets for its preview: every other pair swung open flat against the
    wall. Remove them before an export (they'd be models of their own)."""
    copies = []
    kit.bpy.context.view_layer.update()
    sockets = sorted((c for c in building.children if c.name.startswith('SOCKET_Shutter_')), key=lambda c: c.name)
    for k, empty in enumerate(sockets):
        tag = empty.name[len('SOCKET_Shutter_')]
        number = int(''.join(ch for ch in empty.name.split('.')[0] if ch.isdigit()))
        copy = shutters[tag].copy()
        kit.bpy.context.scene.collection.objects.link(copy)
        swing = 0.0 if number % open_every else (-178.0 if tag == 'L' else 178.0)
        copy.matrix_world = empty.matrix_world @ Matrix.Rotation(math.radians(swing), 4, 'Z')
        copies.append(copy)
    return copies


# --- Shutters ---

SHUTTER_W, SHUTTER_H = 0.5, 1.4
# The pivot is the hinge: its pin stands 2 cm in front of the shutter's back face, so a shutter hung on a casing's face
# that swings open through 180 degrees lies flat against the wall instead of in it.
SHUTTER_BACK = 0.02


def shutter(name, side, seed, paint='H1'):
    """A plank shutter hung on its hinge at the origin, closed and facing -Y: three upright boards, two battens and a
    brace on the outside, strap hinges over the battens. side +1 reaches right of its hinge (it hangs at the left of a
    window), -1 left."""
    s = kit.Model(name, seed=seed)
    w, h = SHUTTER_W, SHUTTER_H
    y_board = SHUTTER_BACK - 0.0175
    boards = 3
    bw = (w - 0.01) / boards
    for k in range(boards):
        x = side * (0.01 + bw * (k + 0.5))
        s.board((x, y_board, 0.0), (x, y_board, h - s.rng.uniform(0.0, 0.02)), bw - 0.008, 0.035, uv=paint)
    for z in (0.22, h - 0.22):
        s.board((side * 0.04, y_board - 0.035, z), (side * (w - 0.03), y_board - 0.035, z), 0.12, 0.03, uv=paint)
    s.board((side * (w - 0.08), y_board - 0.035, 0.32), (side * 0.08, y_board - 0.035, h - 0.32), 0.1, 0.028, uv=paint)
    for z in (0.22, h - 0.22):
        s.board((side * -0.012, y_board - 0.055, z), (side * w * 0.68, y_board - 0.055, z), 0.06, 0.012,
                uv=Trim('H3', fit=True))
        s.cylinder((0.0, y_board - 0.05, z - 0.05), (0.0, y_board - 0.05, z + 0.05), 0.017, sides=6,
                   uv=Trim('H3', fit=True), face=(0.0, -1.0, 0.0))
    s.box((0.03, 0.02, 0.09), at=(side * (w - 0.06), y_board - 0.05, h * 0.5), uv=Trim('H3', fit=True))
    obj = s.finish(ground=False, preview=False, Nanite=0, LODs='50,25', Collision='None')
    return obj


def shutter_sockets(m, space, o, number, casing_w=0.13, casing_t=0.045):
    """SOCKET_Shutter_L<number> and _R<number> at the hinges of window o (closed shutters there), and the pintles they
    hang on."""
    for tag, x, side in (('L', o.x - casing_w, 1.0), ('R', o.x + o.w + casing_w, -1.0)):
        y = -casing_t - SHUTTER_BACK
        z = o.z - 0.05
        rotation = [math.degrees(a) for a in space.matrix.to_euler()]
        m.socket(f'Shutter_{tag}{number}', space.world((x, y, z)), rotation)
        for zz in (0.22, SHUTTER_H - 0.22):
            m.box((0.045, 0.05, 0.05), at=(x - side * 0.01, -casing_t - 0.025, z + zz - 0.07),
                  uv=Trim('H3', fit=True), space=space)


SHUTTERS = {'L': shutter('Shutter_Left', 1.0, 401), 'R': shutter('Shutter_Right', -1.0, 402)}
models = [undertaker(), saloon(), store(SHUTTERS), sheriff()]

if lt.want_preview():
    for obj in SHUTTERS.values():
        lt.preview([obj], lt.preview_path('RansomsRest/Town', obj.name), view=(-0.8, -1.6, 0.5), fit=0.9)
if __name__ == '__overview__':
    # Side by side for the overview, facades on one line.
    x = 0.0
    for obj in models:
        obj.location.x = x
        x += obj.dimensions.x + 3.0
    for k, obj in enumerate(SHUTTERS.values()):
        obj.location = (x + k * 0.8, -6.0, 0.0)
