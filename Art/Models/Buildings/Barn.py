"""The barn at the farmstead: a big plank barn with a gambrel roof of rusty corrugated tin, great double doors in the
front gable framed and cross-braced in faded oxide red, a hayloft door under a hoist beam and small windows down the
sides.

A scripted model (Art/README.md) built with looter_buildings: the pivot is the middle of the footprint at ground level
and the big doors face the front (-Y).

The file also makes the barn Ransom's Rest uses (Docs/Areas/RansomsRest.md: Ransom Farm and the Whitlock barn):

  Barn          the tutorial island's barn. It's in the game there, so its build is exactly what it was: barn() without
                ransom makes the same calls in the same order.
  Barn_Ransom   the same barn (footprint, pivot, walls, doors, hoist beam, collision) under a roof of weathered shakes
                (trim strip E) instead of the tin, whose blue-grey sheets and bright orange rust bands read as loot
                colours over a roof this big. The shakes follow the gambrel's two pitches, a wooden cap covers the ridge
                and a board the knuckle where the steep slope meets the shallow one. The tin's rusty hoist pulley and
                hook stay on the beam, so it keeps the MetalRust slot.

Everything before the roof draws from the barn's random stream in the same order, so the two barns' walls and doors
come out the same.
"""
import looter_buildings as kit
from looter_buildings import Opening, Trim, math

X0, X1, Y0, Y1 = -4.0, 4.0, -6.0, 6.0   # outer faces of the walls: the gables face front and back
BASE = 0.3
WALL = 3.8
EAVE = BASE + WALL
THICK = 0.16
LOWER, UPPER, KNUCKLE = 62.0, 24.0, 1.1
k_rise = KNUCKLE * math.tan(math.radians(LOWER))
r_rise = ((X1 - X0) * 0.5 - KNUCKLE) * math.tan(math.radians(UPPER))
SIDING = Trim('A', world=True, rotate=True)
OVERHANG, RAKE = 0.35, 0.4
# Barn_Ransom's shakes: the courses of the cottage's roof (a texture row each), in runs of 2.2 to 3.8 m between breaks
# (the texture draws each shake), so the big roof costs about what the tin did.
SHAKE_PIECE = (2.2, 3.8)


def leaf(m, space, x0, z0, w, h, y=0.05, braced=True, hinge_left=True):
    """A door leaf of vertical planks framed and cross-braced in oxide-red boards, on long strap hinges."""
    m.box((w, 0.05, h), at=(x0 + w * 0.5, y, z0 + h * 0.5), uv=Trim('A', world=True, rotate=True), space=space)
    f = 0.16
    for z in (z0 + f * 0.5, z0 + h - f * 0.5):
        m.board((x0, y - 0.04, z), (x0 + w, y - 0.04, z), f, 0.035, uv='H2', space=space)
    for x in (x0 + f * 0.5, x0 + w - f * 0.5):
        m.board((x, y - 0.04, z0 + f), (x, y - 0.04, z0 + h - f), f, 0.035, uv='H2', space=space)
    if braced:
        for a, b in (((x0 + f, z0 + f), (x0 + w - f, z0 + h - f)), ((x0 + f, z0 + h - f), (x0 + w - f, z0 + f))):
            m.board((a[0], y - 0.04, a[1]), (b[0], y - 0.04, b[1]), 0.13, 0.03, uv='H2', space=space)
    hinge_x = x0 if hinge_left else x0 + w
    reach = w * 0.45 * (1.0 if hinge_left else -1.0)
    for z in (z0 + 0.35, z0 + h - 0.35):
        m.board((hinge_x - 0.06 * (1.0 if hinge_left else -1.0), y - 0.07, z), (hinge_x + reach, y - 0.07, z), 0.08,
                0.015, uv=Trim('H3', fit=True), space=space)


def barn(name='Barn', ransom=False):
    """The barn. ransom roofs it in shakes (Barn_Ransom); without it, the tutorial island's, exactly as it was."""
    m = kit.Model(name, seed=45)
    rng = m.rng

    kit.plinth(m, X0, X1, Y0, Y1, BASE, out=0.05)
    m.section('foundation')

    # --- Walls, counterclockwise from above: front gable, right side, back gable, left side. ---
    BIG = Opening('door', 2.4, 0.0, 3.2, 3.4)
    LOFT = Opening('window', 3.35, 4.25, 1.3, 1.3)
    BACK_DOOR = Opening('door', 3.4, 0.0, 1.2, 2.3)
    VENT = Opening('window', 3.6, 4.6, 0.8, 0.8)
    side_windows = [Opening('window', x, 1.9, 0.8, 0.8, style='glass', paint='H2', panes=(2, 2)) for x in (1.6, 5.6, 9.6)]
    side_windows[1].opts.update(style='boarded')
    gable_top = [(X1 - X0 - KNUCKLE, WALL + k_rise), ((X1 - X0) * 0.5, WALL + k_rise + r_rise), (KNUCKLE, WALL + k_rise)]
    walls = [
        ((X0, Y0), (X1, Y0), True, [BIG, LOFT]),
        ((X1, Y0), (X1, Y1), False, side_windows),
        ((X1, Y1), (X0, Y1), True, [BACK_DOOR, VENT]),
        ((X0, Y1), (X0, Y0), False, [Opening(o.kind, o.x + 0.8, o.z, o.w, o.h, **o.opts) for o in side_windows]),
    ]
    spaces = []
    for p0, p1, gable_end, items in walls:
        space = kit.wall_space(p0, p1, BASE)
        spaces.append(space)
        length = (kit.Vector(p1) - kit.Vector(p0)).length
        m.panel(kit.wall_outline(length, WALL, items, gable_top if gable_end else None), kit.holes(items), THICK,
                Trim('A', world=True, rotate=True, u=rng.uniform(0.0, 6.4)), space=space, around=items)
        windows = [o for o in items if o.kind == 'window' and o.opts]
        kit.openings(m, space, windows, THICK)
        # Corner boards, a rot board along the bottom and, on the long sides, a girt line of nail heads (a board).
        for x in (0.08, length - 0.08):
            kit.trim_board(m, space, (x, 0.0, -0.05), (x, 0.0, WALL - 0.02), 0.16, 'C', thick=0.045)
        spans = [(0.16, length - 0.16)]
        for o in items:
            if o.z <= 1e-4:
                spans = [(a, min(b, o.x - 0.15)) for a, b in spans] + [(max(a, o.x + o.w + 0.15), b) for a, b in spans]
        for a, b in spans:
            if b - a > 0.2:
                kit.trim_board(m, space, (a, 0.0, 0.12), (b, 0.0, 0.12), 0.22, 'C', thick=0.04)
        if not gable_end:
            kit.trim_board(m, space, (0.16, 0.0, WALL - 0.15), (length - 0.16, 0.0, WALL - 0.15), 0.24, 'A',
                           thick=0.035)
    m.section('walls')

    # --- The front: the great doors, the hayloft door and the hoist beam; the back door. ---
    front, back = spaces[0], spaces[2]
    half = BIG.w * 0.5
    leaf(m, front, BIG.x, 0.0, half - 0.01, BIG.h, hinge_left=True)
    leaf(m, front, BIG.x + half + 0.01, 0.0, half - 0.01, BIG.h, hinge_left=False)
    # A heavy header over the doors and posts at their sides.
    m.board((BIG.x - 0.25, -0.06, BIG.h + 0.12), (BIG.x + BIG.w + 0.25, -0.06, BIG.h + 0.12), 0.24, 0.12,
            uv=Trim('C', lane='each'), space=front, bevel=0.012)
    for x in (BIG.x - 0.1, BIG.x + BIG.w + 0.1):
        m.board((x, -0.05, -0.02), (x, -0.05, BIG.h), 0.2, 0.1, uv=Trim('C', lane='each'), space=front)
    leaf(m, front, LOFT.x, LOFT.z, LOFT.w, LOFT.h, hinge_left=True)
    for x in (LOFT.x - 0.08, LOFT.x + LOFT.w + 0.08):
        kit.trim_board(m, front, (x, 0.0, LOFT.z - 0.1), (x, 0.0, LOFT.z + LOFT.h + 0.14), 0.16, 'H2', thick=0.04)
    kit.trim_board(m, front, (LOFT.x - 0.2, 0.0, LOFT.z + LOFT.h + 0.1),
                   (LOFT.x + LOFT.w + 0.2, 0.0, LOFT.z + LOFT.h + 0.1), 0.16, 'H2', thick=0.045)
    m.box((LOFT.w + 0.4, 0.14, 0.06), at=(LOFT.x + LOFT.w * 0.5, -0.07, LOFT.z - 0.05), uv='C', space=front)
    apex_z = WALL + k_rise + r_rise
    beam_z = apex_z - 0.5
    mid = (X1 - X0) * 0.5
    m.board((mid, 0.5, beam_z), (mid, -1.35, beam_z), 0.22, 0.2, face=(0.0, 0.0, 1.0), uv=Trim('C', lane='each'),
            space=front, bevel=0.012)
    m.board((mid, -0.05, beam_z - 0.7), (mid, -0.6, beam_z - 0.1), 0.12, 0.1, face=(1.0, 0.0, 0.0), uv='C',
            space=front)
    m.cylinder((mid - 0.05, -1.15, beam_z - 0.24), (mid + 0.05, -1.15, beam_z - 0.24), 0.11, sides=10,
               uv=kit.Tile('MetalRust'), mat='metal', space=front, face=(0.0, 0.0, 1.0))
    m.box((0.14, 0.03, 0.18), at=(mid, -1.15, beam_z - 0.16), uv=kit.Tile('MetalRust'), mat='metal', space=front)
    m.cylinder((mid, -1.26, beam_z - 0.26), (mid, -1.26, beam_z - 1.9), 0.018, sides=5, uv='C', space=front,
               caps=(False, True))
    m.box((0.05, 0.05, 0.18), at=(mid, -1.26, beam_z - 1.98), uv=kit.Tile('MetalRust'), mat='metal', space=front)
    leaf(m, back, BACK_DOOR.x, 0.0, BACK_DOOR.w, BACK_DOOR.h, braced=True, hinge_left=False)
    for x in (BACK_DOOR.x - 0.08, BACK_DOOR.x + BACK_DOOR.w + 0.08):
        kit.trim_board(m, back, (x, 0.0, -0.02), (x, 0.0, BACK_DOOR.h + 0.14), 0.16, 'C', thick=0.045)
    kit.trim_board(m, back, (BACK_DOOR.x - 0.2, 0.0, BACK_DOOR.h + 0.1),
                   (BACK_DOOR.x + BACK_DOOR.w + 0.2, 0.0, BACK_DOOR.h + 0.1), 0.18, 'C', thick=0.05)
    # The loft vent: louvres of weathered boards.
    m.box((VENT.w, 0.02, VENT.h), at=(VENT.x + VENT.w * 0.5, 0.1, VENT.z + VENT.h * 0.5), uv=Trim('H4', fit=True),
          space=back)
    for k in range(4):
        z = VENT.z + 0.12 + k * 0.19
        m.board((VENT.x, -0.02, z), (VENT.x + VENT.w, -0.02, z), 0.15, 0.025, uv='A', space=back, lift=0.0)
    for x in (VENT.x - 0.07, VENT.x + VENT.w + 0.07):
        kit.trim_board(m, back, (x, 0.0, VENT.z - 0.1), (x, 0.0, VENT.z + VENT.h + 0.1), 0.14, 'C', thick=0.04)
    m.section('doors')

    # --- Roof: a gambrel, the ridge running front to back: tin, or (Barn_Ransom) shakes. ---
    slopes, knuckle_z, ridge_z = kit.gambrel(m, Y0, Y1, -X1, -X0, EAVE, LOWER, UPPER, KNUCKLE, overhang=OVERHANG,
                                             rake=RAKE, deck=0.1, sag=0.12,
                                             frame=kit.Matrix.Rotation(math.radians(90.0), 4, 'Z'))
    if ransom:
        shake_roof(m, slopes)
    else:
        for slope in slopes:
            kit.tin(m, slope)
        kit.ridge_cap(m, slopes[1], slopes[3])
        # Flashing over each knuckle, where the steep slope meets the shallow one.
        for low in (slopes[0], slopes[2]):
            m.box((low.width + 0.06, 0.3, 0.02), at=(low.width * 0.5, low.length - 0.02, 0.06), rot=(-18.0, 0.0, 0.0),
                  uv='F', space=low, cuts=8, ao_floor=kit.ROOF_AO)
    m.section('roof')

    # --- Collision: the barn's body and its roof (a gambrel is convex). ---
    m.hull((X1 - X0 + 0.12, Y1 - Y0 + 0.12, EAVE), at=(0.0, 0.0, EAVE * 0.5))
    eave_top = EAVE - OVERHANG * math.tan(math.radians(LOWER)) + 0.2
    profile = [(X0 - OVERHANG, eave_top), (X0 + KNUCKLE, knuckle_z + 0.25), (0.0, ridge_z + 0.3),
               (X1 - KNUCKLE, knuckle_z + 0.25), (X1 + OVERHANG, eave_top)]
    m.hull_points([(x, y, z) for x, z in profile for y in (Y0 - RAKE, Y1 + RAKE)])

    return m.finish()


def shakes(m, slope, course=0.2, butt=0.03, piece=SHAKE_PIECE, start=-0.08, missing=0.02):
    """kit.shingles for a big roof: the same courses of shakes on strip E's rows, each run's butt standing proud of
    the course below, a few runs crooked and a few shakes gone; but a run's ends are left open where it meets the next
    run of its course (the step between them is a hair, and the deck under them shows through any gap), which halves
    the roof's triangles. The runs at the rakes keep their outer ends."""
    rng = m.rng
    top = slope.length + 0.02
    exposure = course - 0.02
    y = start
    k = 0
    while y < top - 0.04:
        h = min(course, top - y)
        x = -0.05 + rng.uniform(-0.3, 0.0)
        while x < slope.width + 0.05 - 1e-3:
            w = rng.uniform(*piece)
            if slope.width + 0.05 - (x + w) < 0.6:
                w = slope.width + 0.05 - x
            x_lo = max(x, -0.05)
            spans = [(x_lo, x + w, False, False)]   # from, to, and whether each end is at a gap
            if k >= 2 and w > 0.9 and rng.random() < missing * 3.0:
                gap = rng.uniform(0.2, 0.45)
                at = rng.uniform(x_lo + 0.3, x + w - 0.3 - gap)
                spans = [(x_lo, at, False, True), (at + gap, x + w, True, False)]
            for a, b, gap_a, gap_b in spans:
                cx, cy = (a + b) * 0.5, y + h * 0.5
                tilt = -math.degrees(math.atan2(butt, h))
                jitter = rng.uniform(-0.012, 0.012)
                matrix = kit.place((cx, cy + jitter, butt * 0.5 + 0.004 * (k % 2)),
                                   (tilt, rng.uniform(-0.4, 0.4), 0.0))
                drop = ['-z', '+y']
                if a > -0.04 and not gap_a:
                    drop.append('-x')
                if b < slope.width + 0.04 and not gap_b:
                    drop.append('+x')
                m.box((b - a, h, butt), matrix=matrix, uv=Trim('E', lane=k % 4), space=slope, ao_floor=kit.ROOF_AO,
                      drop=tuple(drop))
            x += w
        y += exposure
        k += 1


def shake_roof(m, slopes):
    """Barn_Ransom's covering: shakes on all four slopes (the courses run on from the steep slope over the knuckle, as
    a roofer laps them), a wooden ridge cap, and a cap board along each knuckle over the courses' joint."""
    for slope in slopes:
        shakes(m, slope)
    kit.ridge_cap(m, slopes[1], slopes[3], uv='C', width=0.2, thick=0.05)
    for low in (slopes[0], slopes[2]):
        m.box((low.width + 0.06, 0.24, 0.04), at=(low.width * 0.5, low.length - 0.04, 0.075), rot=(-18.0, 0.0, 0.0),
              uv=Trim('C', lane=0), space=low, cuts=8, ao_floor=kit.ROOF_AO)


# --- The models ---

barn()
if __name__ != '__overview__':            # the buildings' overview shows the tutorial island's barn only
    barn('Barn_Ransom', ransom=True)
