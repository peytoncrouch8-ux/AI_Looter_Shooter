"""The station at Ransom's Rest (zone "Undertaker's Yard and the Depot", Docs/Areas/RansomsRest.md): the depot, its low
timber platform kit, the water tower in Stage Gap, and the undertaker's coffin shed. Scripted models (Art/README.md)
built with looter_buildings and looter_rail; every pivot is on the ground and every front (-Y) faces the track.

- Depot: a timber combination depot, 10 x 6 m: horizontal siding over a wainscot of upright boards, faded oxide-red
  trim, a steep shake roof breaking into a long, low shake canopy on knee brackets along the track side, the agent's
  bay with the ticket window, the waiting-room door, benches under the canopy, a lamp over the station board, a
  fieldstone chimney, and "RANSOM'S REST" in Rye on both gable ends and on a board hung from the canopy's fascia, facing
  the platform and the train. Its floor is the platform's height (0.40 m): stand
  it with its front wall on the platform's back edge, 1.65 + 4.0 m from the track's centreline (its pivot 8.65 m from
  the centreline), so the benches and the bay stand on the platform. Sockets: StationBoard (where Railway.py's board
  hangs, beside the door), Light (the lamp), Smoke (the chimney), Speaker (the ticket window), Crepe (over the door,
  for the graves kit's mourning crepe), Sit_1 and Sit_2 (the benches).
- DepotPlatform_4m, DepotPlatform_End: the platform kit. A 4 m piece runs from its pivot along +X to x = 4 m, where
  the next one starts; it is 4 m deep, its pivot in the middle of that depth, so its track-side edge is at y = -2 and
  the track's centreline at y = -3.65. Plank decking between edge timbers, a boarded face behind posts on both sides
  (either side can face the track). The End is a 2.4 m ramp down to the ground, put at either end of a run (turned
  180 degrees at the start). Walking surface 0.40 m, UCX boxes for walking.
- WaterTower, WaterTowerSpout: a stave tank with six iron hoops under a black-painted tin cone (IronBlack), on a
  braced timber trestle on
  fieldstone piers, 10.2 m to the finial, with a ladder (west side), a float gauge and the outlet pipe. The spout
  (IronBlack) is its own model, pivot on its hinge barrel, modeled level along its -Y; SOCKET_Spout on the tower is
  that hinge, its front toward the track. Pitch it about the hinge (the socket's left-right axis): about 50 degrees up
  to stow it, about 40 degrees down to fill a tank engine. Stand the tower with its centre 4.5 m from the track's
  centreline, past the platform's east ramp: the hinge is then 2.1 m from the centreline and the spout, lowered 40
  degrees, ends over it at 2.9 m.
- CoffinShed: the undertaker's plank shed (3.2 x 2.4 m, a shake roof sloping back) with coffin lids, a child's among
  them, leaning on its east wall, lumber on its west wall, and a lid on sawhorses before the door.
"""
import bmesh
from mathutils.geometry import tessellate_polygon

import looter_buildings as kit
from looter_buildings import Opening, Trim, math
import looter_rail as rail

Vector, Matrix = kit.Vector, kit.Matrix

# The rail contract (Art/Models/Props/Railway.py): the platform against the track.
PLATFORM_TOP = 0.40      # the walking surface above the track's ground
PLATFORM_EDGE = 1.65     # its track-side edge from the track's centreline
PLATFORM_DEPTH = 4.0     # from that edge back to the depot's front wall
PIECE = 4.0              # a platform piece's length
RAMP = 2.4               # the end ramp's run (9.5 degrees)


def upper_outline(length, z0, top, items, apex=None):
    """A wall's outline above its wainscot (from z0): notched for the doors and anything else standing on the floor,
    with a gable point apex."""
    points = [(0.0, z0)]
    for o in sorted((o for o in items if o.z <= 1e-4), key=lambda o: o.x):
        points += [(o.x, z0), (o.x, o.h), (o.x + o.w, o.h), (o.x + o.w, z0)]
    points += [(length, z0), (length, top)]
    if apex is not None:
        points.append(apex)
    points.append((0.0, top))
    return points


def facade(m, outline, holes=(), thick=0.12, uv=None, space=None, matrix=None, slices=2.5, around=(), mat='trim'):
    """kit's panel() with only the faces anyone sees: the front and the reveals of its holes. A closed building's
    inside, and a wall's edges under its roof and corner boards, are left out (they were about a third of a wall)."""
    uv = uv if uv is not None else Trim('A', world=True)
    tb = kit._new_bmesh()
    loops = [list(outline)] + [list(h) for h in holes]
    points = [p for loop in loops for p in loop]
    front = [tb.verts.new((x, 0.0, z)) for x, z in points]
    for a, b, c in tessellate_polygon([[Vector((x, z, 0.0)) for x, z in loop] for loop in loops]):
        try:
            face = tb.faces.new((front[a], front[b], front[c]))
        except ValueError:
            continue
        face.normal_update()
        if face.normal.y > 0.0:
            face.normal_flip()
    start = len(loops[0])
    for hole in loops[1:]:
        n = len(hole)
        center = Vector((sum(x for x, z in hole) / n, thick * 0.5, sum(z for x, z in hole) / n))
        back = [tb.verts.new((x, thick, z)) for x, z in hole]
        for i in range(n):
            j = (i + 1) % n
            face = tb.faces.new((front[start + i], front[start + j], back[j], back[i]))
            face.normal_update()
            if face.normal.dot(center - face.calc_center_median()) < 0.0:
                face.normal_flip()
        start += n
    triangles = [f for f in tb.faces if len(f.verts) == 3]
    inner = [e for e in {e for f in triangles for e in f.edges}
             if len(e.link_faces) == 2 and all(len(f.verts) == 3 for f in e.link_faces)]
    bmesh.ops.beautify_fill(tb, faces=triangles, edges=inner, method='ANGLE')
    bmesh.ops.join_triangles(tb, faces=[f for f in tb.faces if len(f.verts) == 3], angle_face_threshold=0.02,
                             angle_shape_threshold=math.pi)
    xs = [x for x, z in outline]
    zs = [z for x, z in outline]
    upright = isinstance(uv, Trim) and uv.rotate
    cut_x, cut_z = ([] if upright or not slices else kit.frange(min(xs) + slices, max(xs) - 0.1, slices)), set()
    for o in around:
        if o.z > 0.15:
            cut_z.add(round(o.z - 0.1, 2))
        if not upright:
            cut_x += [o.x - 0.2, o.x + o.w + 0.2]
    kit._slice(tb, 0, [x for x in cut_x if min(xs) + 0.05 < x < max(xs) - 0.05])
    kit._slice(tb, 2, [z for z in sorted(cut_z) if min(zs) + 0.05 < z < max(zs) - 0.05])
    m.emit(tb, uv, mat, matrix, space)


def shakes(m, slope, piece=(1.6, 3.0), start=-0.08, missing=0.02, skip=None):
    """kit.shingles with courses two of the texture's rows deep (0.4 m): half the pieces, each still standing proud of
    the course below; a few sit crooked, a few shakes are gone."""
    rng = m.rng
    course, butt = 0.4, 0.03
    top = slope.length + 0.02
    y, k = start, 0
    while y < top - 0.04:
        h = min(course, top - y)
        x = -0.05 + rng.uniform(-0.3, 0.0)
        while x < slope.width + 0.05 - 1e-3:
            w = rng.uniform(*piece)
            if slope.width + 0.05 - (x + w) < 0.6:
                w = slope.width + 0.05 - x
            x_lo = max(x, -0.05)
            spans = [(x_lo, x + w)]
            if k >= 1 and w > 0.9 and rng.random() < missing * 4.0:
                gap = rng.uniform(0.2, 0.4)
                at = rng.uniform(x_lo + 0.3, x + w - 0.3 - gap)
                spans = [(x_lo, at), (at + gap, x + w)]
            for a, b in spans:
                cx, cy = (a + b) * 0.5, y + h * 0.5
                if skip is not None and skip(cx, cy):
                    continue
                tilt = -math.degrees(math.atan2(butt, h))
                matrix = kit.place((cx, cy + rng.uniform(-0.012, 0.012), butt * 0.5 + 0.004 * (k % 2)),
                                   (tilt, rng.uniform(-0.4, 0.4), 0.0))
                m.box((b - a, h, butt), matrix=matrix, uv=Trim('E', lane=2 * (k % 2)), space=slope, ao_floor=kit.ROOF_AO,
                      drop=('-z', '+y'))
            x += w
        y += course - 0.02
        k += 1


def plinth3(m, x0, x1, y0, y1, height, out=0.06):
    """A fieldstone foundation on the gable ends and the back (the platform hides the front), standing out of the
    walls by out."""
    deep = 0.3
    for p0, p1 in (((x1 + out, y0), (x1 + out, y1 + out)), ((x1 + out, y1 + out), (x0 - out, y1 + out)),
                   ((x0 - out, y1 + out), (x0 - out, y0))):
        space = kit.wall_space(p0, p1, 0.0)
        length = (Vector(p1) - Vector(p0)).length
        m.box((length, deep, height + 0.1), at=(length * 0.5, deep * 0.5, height * 0.5 - 0.05),
              uv=Trim('D', world=True, u=m.rng.uniform(0.0, kit.U_REPEAT)), space=space,
              cuts=max(0, int(length / 2.5)), drop=('+y',))


def planks_at(offset_v):
    """WoodPlanks by world projection, the plank rows lined up with heights: a gap every 20 cm from offset_v."""
    class Spec:
        def apply(self, obj, rng):
            kit.lt.box_uv(obj, 'WoodPlanks', offset=(rng.uniform(0.0, 3.2), -offset_v))
    return Spec()


# --- The depot ---

def depot():
    m = rail.Model('Depot', seed=211)
    rng = m.rng
    X0, X1, Y0, Y1 = -5.0, 5.0, -3.0, 3.0      # outer faces of the walls
    BASE, WALL = PLATFORM_TOP, 3.5               # the floor (the platform's top) and the walls above it
    EAVE = BASE + WALL
    PITCH = 38.0
    THICK = 0.14
    L, D = X1 - X0, Y1 - Y0
    TAN = math.tan(math.radians(PITCH))
    RISE = D * 0.5 * TAN
    BELT = 0.98                                  # the belt rail between the wainscot and the siding

    plinth3(m, X0, X1, Y0, Y1, BASE, out=0.06)
    m.section('foundation')

    # --- Walls, counterclockwise from above: front (the track side), east gable, back, west gable. ---
    def glass(x, z=1.1, w=0.9, h=1.45, **opts):
        return Opening('window', x, z, w, h, style='glass', paint='H2', sash='H2', panes=(2, 2), **opts)
    W1, W2 = glass(0.6), glass(8.0)
    D1 = Opening('door', 2.2, 0.0, 1.0, 2.3, paint='H2', casing='H2', hinge='right')
    BAY = Opening('bay', 5.1, 0.0, 2.2, 3.1)     # the agent's bay stands in this notch
    BOARD_X = 4.15                               # the station board's middle, along the front wall
    front_items = [W1, D1, BAY, W2]
    east_items = [Opening('door', 2.4, 0.0, 1.2, 2.3, paint='H2', casing='H2', hinge='left')]
    back_items = [glass(1.3), Opening('door', 4.2, 0.0, 1.0, 2.25, paint='A', casing='H2', hinge='left'), glass(6.3),
                  Opening('window', 8.1, 1.1, 0.9, 1.45, style='boarded', paint='H2', sash='H2')]
    west_items = [glass(1.0), glass(4.1)]
    walls = [('front', (X0, Y0), (X1, Y0), False, front_items), ('east', (X1, Y0), (X1, Y1), True, east_items),
             ('back', (X1, Y1), (X0, Y1), False, back_items), ('west', (X0, Y1), (X0, Y0), True, west_items)]
    spaces = {}
    for name, p0, p1, gable_end, items in walls:
        space = kit.wall_space(p0, p1, BASE)
        spaces[name] = space
        length = (Vector(p1) - Vector(p0)).length
        apex = (length * 0.5, WALL + RISE) if gable_end else None
        # The siding starts behind the belt rail; the wainscot below stands in for the wall.
        facade(m, upper_outline(length, BELT - 0.05, WALL, items, apex), kit.holes(items), THICK,
               Trim('A', world=True, u=rng.uniform(0.0, 6.4)), space=space, around=items)
        kit.openings(m, space, items, THICK)
        # A wainscot of upright boards below the belt rail, stopping at the door casings (and at the bay).
        cuts = sorted(v for o in items if o.z <= 1e-4
                      for v in (o.x - (0.13 if o.kind == 'door' else 0.0), o.x + o.w + (0.13 if o.kind == 'door' else 0.0)))
        edges = [0.02] + cuts + [length - 0.02]
        for a, b in zip(edges[0::2], edges[1::2]):
            if b - a > 0.1:
                facade(m, kit.rect(a, 0.0, b - a, BELT - 0.04), (), 0.025, Trim('A', world=True, rotate=True),
                       space=space, matrix=Matrix.Translation((0.0, -0.025, 0.0)))
                kit.trim_board(m, space, (a, 0.0, BELT), (b, 0.0, BELT), 0.09, 'H2', thick=0.05, lift=0.0)
        # Corner boards (the front and back ones wrap over the gable walls' edges); a frieze under the eaves.
        top = WALL - 0.02
        wrap = 0.0 if gable_end else 0.05
        for x in (0.07 - wrap * 0.8, length - 0.07 + wrap * 0.8):
            kit.trim_board(m, space, (x, 0.0, -0.05), (x, 0.0, top), 0.14 + wrap, 'H2', thick=0.045)
        if not gable_end:
            kit.trim_board(m, space, (0.15, 0.0, WALL - 0.12), (length - 0.15, 0.0, WALL - 0.12), 0.2, 'H2',
                           thick=0.035)
    m.section('walls, windows, doors')

    # --- The agent's bay: three faces round the notch in the front wall, the ticket window in the middle one. ---
    A, B, C, Dp = (X0 + 5.1, Y0), (X0 + 5.6, Y0 - 0.7), (X0 + 6.8, Y0 - 0.7), (X0 + 7.3, Y0)
    BAY_H = 3.1
    bay_spaces = []
    for k, (p0, p1) in enumerate(((A, B), (B, C), (C, Dp))):
        space = kit.wall_space(p0, p1, BASE)
        bay_spaces.append(space)
        length = (Vector(p1) - Vector(p0)).length
        if k == 1:
            win = Opening('window', (length - 0.62) * 0.5, 1.05, 0.62, 0.85, style='glass', paint='H2', sash='H2',
                          panes=(2, 1), casing=0.09)
        else:
            win = Opening('window', (length - 0.44) * 0.5, 1.1, 0.44, 1.45, style='glass', paint='H2', sash='H2',
                          panes=(1, 2), casing=0.08)
        facade(m, kit.rect(0.0, BELT - 0.05, length, BAY_H - BELT + 0.05), kit.holes([win]), 0.1, Trim('A', world=True),
               space=space, around=[win], slices=0.0)
        kit.openings(m, space, [win], 0.1)
        facade(m, kit.rect(0.02, 0.0, length - 0.04, BELT - 0.04), (), 0.025, Trim('A', world=True, rotate=True),
               space=space, matrix=Matrix.Translation((0.0, -0.025, 0.0)))
        kit.trim_board(m, space, (0.0, 0.0, BELT), (length, 0.0, BELT), 0.09, 'H2', thick=0.05)
        kit.trim_board(m, space, (0.0, 0.0, BAY_H - 0.1), (length, 0.0, BAY_H - 0.1), 0.2, 'H2', thick=0.04)
        for x in (0.04, length - 0.04):
            kit.trim_board(m, space, (x, 0.0, -0.02), (x, 0.0, BAY_H - 0.2), 0.08, 'H2', thick=0.05)
    # Its flat top, under the canopy.
    cap = kit._new_bmesh()
    ring = [Vector((x, y, 0.0)) for x, y in ((A[0] - 0.06, A[1]), (B[0] - 0.04, B[1] - 0.06),
                                             (C[0] + 0.04, C[1] - 0.06), (Dp[0] + 0.06, Dp[1]))]
    lo = [cap.verts.new((p.x, p.y, BASE + BAY_H)) for p in ring]
    hi = [cap.verts.new((p.x, p.y, BASE + BAY_H + 0.1)) for p in ring]
    cap.faces.new(hi)
    cap.faces.new(list(reversed(lo)))
    for i in range(3):
        cap.faces.new((lo[i], lo[i + 1], hi[i + 1], hi[i]))
    m.emit(cap, Trim('C', world=True), 'trim')
    # The ticket window: a counter on brackets, an iron grille, TICKETS over it.
    tw = bay_spaces[1]
    tl = (Vector(C) - Vector(B)).length
    m.board((0.12, -0.13, 1.0), (tl - 0.12, -0.13, 1.0), 0.24, 0.045, face=(0.0, 0.0, 1.0), uv='C', space=tw)
    for x in (0.25, tl - 0.25):
        m.board((x, -0.03, 0.72), (x, -0.2, 0.97), 0.06, 0.05, face=(1.0, 0.0, 0.0), uv='C', space=tw)
    gx0 = (tl - 0.62) * 0.5
    for k in range(5):
        x = gx0 + 0.62 * (k + 0.5) / 5
        m.box((0.018, 0.018, 0.84), at=(x, 0.01, 1.47), uv=rail.IRON, space=tw)
    for z in (1.12, 1.82):
        m.box((0.62, 0.02, 0.025), at=(tl * 0.5, 0.0, z), uv=rail.IRON, space=tw)
    m.board((0.12, -0.03, 2.27), (tl - 0.12, -0.03, 2.27), 0.2, 0.035, uv=Trim('C', lane=2), space=tw)
    rail.lettering(m, 'TICKETS', 0.1, at=(tl * 0.5, -0.0505, 2.27), space=tw, width=0.82, simplify=40.0)
    rail.socket_at(m, 'Speaker', (tl * 0.5, -0.06, 1.47), space=tw)
    m.section('bay')

    # --- Roof: steep shakes; the front slope ends at the wall, where the canopy carries on at a low pitch. ---
    front_roof, back_roof = gable2(m, X0, X1, Y0, Y1, EAVE, PITCH, 0.0, 0.45, 0.45, deck=0.12, sag=0.06)
    CHIMNEY = (X0 + 4.2, 0.95)
    ch_x, ch_y = CHIMNEY

    def by_chimney(x, y):
        # The back slope runs from the east end (x = X1 + rake) westward, and up from its eave (Y1 + 0.45).
        wx = (X1 + 0.45) - x
        wy = Y1 + 0.45 - y * math.cos(math.radians(PITCH))
        return abs(wx - ch_x) < 0.48 and abs(wy - ch_y) < 0.5
    shakes(m, front_roof)
    shakes(m, back_roof, skip=by_chimney)
    kit.ridge_cap(m, front_roof, back_roof, uv='C', width=0.14, thick=0.05, lift=0.04)
    # Knee brackets under the rakes at the gable corners.
    for x, side in ((X0, -1.0), (X1, 1.0)):
        for y in (Y0 + 0.25, Y1 - 0.25):
            z_rake = EAVE + (D * 0.5 - abs(y)) * TAN
            m.board((x + side * 0.02, y, z_rake - 0.75), (x + side * 0.4, y, z_rake - 0.06), 0.1, 0.08, face=(0.0, 1.0, 0.0),
                    uv='C')
    m.section('main roof')

    # The chimney: a fieldstone stack through the back slope near the ridge, a cap stone, flashing.
    roof_z = EAVE + (Y1 - ch_y) * TAN
    top = EAVE + RISE + 0.75
    kit.stone_stack(m, (ch_x, ch_y, roof_z - 0.45), (0.62, 0.62), top - roof_z + 0.45, taper=0.03,
                    rot=rng.uniform(-2, 2))
    m.box((0.78, 0.78, 0.09), at=(ch_x, ch_y, top + 0.04), uv='D', bevel=0.02)
    m.box((0.36, 0.36, 0.14), at=(ch_x, ch_y, top + 0.15), uv='D')
    sx = (X1 + 0.45) - ch_x
    sy = (Y1 + 0.45 - ch_y) / math.cos(math.radians(PITCH))
    m.box((0.78, 0.78, 0.02), at=(sx, sy, 0.06), uv='F', space=back_roof)
    m.socket('Smoke', (ch_x, ch_y, top + 0.3))
    m.section('chimney')

    # --- The platform canopy: shakes like the main roof, at 9 degrees from under the eave out over the platform,
    # carried by outlookers on knee brackets between the openings, rafters between them, a fascia with the station's
    # name board on it for anyone arriving on the platform. ---
    C_DEPTH, C_PITCH = 3.2, 9.0
    C_X0, C_X1 = X0 - 0.4, X1 + 0.4
    tc = math.radians(C_PITCH)
    c_top = EAVE - 0.06
    c_edge = c_top - C_DEPTH * math.tan(tc)
    canopy = kit.Slope((C_X0, Y0 - C_DEPTH, c_edge), (1.0, 0.0, 0.0), (0.0, math.cos(tc), math.sin(tc)), C_X1 - C_X0,
                       C_DEPTH / math.cos(tc) + 0.02)
    roof_deck(m, canopy, 0.07)
    shakes(m, canopy, piece=(1.8, 3.2))
    fascia_y = Y0 - C_DEPTH - 0.025
    m.board((C_X0 - 0.03, fascia_y, c_edge - 0.1), (C_X1 + 0.03, fascia_y, c_edge - 0.1), 0.2, 0.05,
            uv=Trim('C', lane=0))
    m.board((C_X0 - 0.04, fascia_y - 0.03, c_edge - 0.215), (C_X1 + 0.04, fascia_y - 0.03, c_edge - 0.215), 0.06, 0.03,
            uv='H2')
    # RANSOM'S REST over the platform: two dark boards in a red frame hung on the fascia, iron straps over its top.
    sign_y, sign_z, sign_w = fascia_y - 0.065, c_edge - 0.25, 3.5     # its top just under the shakes' edge
    for k, z in enumerate((sign_z - 0.1, sign_z + 0.1)):
        m.board((-sign_w * 0.5, sign_y, z), (sign_w * 0.5, sign_y, z), 0.2, 0.035, uv=Trim('C', lane=k + 2))
    for z in (sign_z - 0.23, sign_z + 0.23):
        m.board((-sign_w * 0.5 - 0.06, sign_y - 0.01, z), (sign_w * 0.5 + 0.06, sign_y - 0.01, z), 0.06, 0.05, uv='H2')
    for x in (-sign_w * 0.5 - 0.03, sign_w * 0.5 + 0.03):
        m.board((x, sign_y - 0.01, sign_z - 0.2), (x, sign_y - 0.01, sign_z + 0.2), 0.06, 0.05, uv='H2')
    for x in (-sign_w * 0.35, sign_w * 0.35):
        m.box((0.06, 0.1, 0.16), at=(x, sign_y + 0.03, sign_z + 0.27), uv=rail.IRON)
    rail.lettering(m, "RANSOM'S REST", 0.25, at=(0.0, sign_y - 0.021, sign_z), width=sign_w - 0.3, simplify=30.0)

    def under(y_out, depth):
        """The height of a timber's top under the canopy deck, y_out out from the wall."""
        return c_top - y_out * math.tan(tc) - 0.07 - depth

    BRACKETS = [0.3, 1.85, 4.95, 7.65, 9.7]
    for lx in BRACKETS:
        x = X0 + lx
        m.board((x, Y0 - 0.02, under(0.0, 0.1)), (x, Y0 - C_DEPTH + 0.02, under(C_DEPTH, 0.1)), 0.2, 0.12,
                face=(1.0, 0.0, 0.0), uv=Trim('C', lane='each'))
        m.board((x, Y0 - 0.045, BASE + 2.05), (x, Y0 - 0.045, under(0.0, 0.2)), 0.16, 0.09, uv=Trim('C', lane='each'))
        m.board((x, Y0 - 0.1, BASE + 2.2), (x, Y0 - 1.95, under(1.95, 0.2) + 0.03), 0.15, 0.1, face=(1.0, 0.0, 0.0),
                uv='C')
        # A turned drop where the brace meets the outlooker.
        m.box((0.09, 0.09, 0.22), at=(x, Y0 - 1.95, under(1.95, 0.2) - 0.1), uv='C', bevel=0.015)
    for lx in kit.frange(-0.4 + 0.75, L + 0.4, 0.75):
        if all(abs(lx - b) > 0.35 for b in BRACKETS):
            x = X0 + lx
            m.board((x, Y0 - 0.02, under(0.0, 0.07)), (x, Y0 - C_DEPTH + 0.03, under(C_DEPTH, 0.07)), 0.14, 0.06,
                    face=(1.0, 0.0, 0.0), uv='C')
    # A ledger along the wall under the canopy.
    m.board((C_X0 + 0.4, Y0 - 0.04, under(0.0, 0.0) - 0.12), (C_X1 - 0.4, Y0 - 0.04, under(0.0, 0.0) - 0.12), 0.2,
            0.06, uv=Trim('C', lane=1))
    m.section('canopy')

    # --- Under the canopy: benches, the lamp over the station board. ---
    def bench(cx, length=1.7):
        seat = BASE + 0.45
        for k, (dy, w) in enumerate(((-0.43, 0.17), (-0.25, 0.17))):
            m.board((cx - length * 0.5 + rng.uniform(-0.02, 0.0), Y0 + dy, seat - 0.022),
                    (cx + length * 0.5 + rng.uniform(0.0, 0.02), Y0 + dy, seat - 0.022), w, 0.045, face=(0.0, 0.0, 1.0),
                    uv='A')
        m.board((cx - length * 0.5, Y0 - 0.115, BASE + 0.82), (cx + length * 0.5, Y0 - 0.115, BASE + 0.82), 0.17, 0.035,
                face=(0.0, -1.0, 0.1), uv='A')
        for x in (cx - length * 0.5 + 0.16, cx + length * 0.5 - 0.16):
            m.board((x, Y0 - 0.48, BASE), (x, Y0 - 0.48, seat - 0.05), 0.07, 0.06, face=(1.0, 0.0, 0.0), uv='C')
            m.board((x, Y0 - 0.09, BASE), (x, Y0 - 0.12, BASE + 0.93), 0.07, 0.06, face=(1.0, 0.0, 0.0), uv='C')
            m.board((x, Y0 - 0.53, seat - 0.075), (x, Y0 - 0.08, seat - 0.075), 0.07, 0.05, face=(1.0, 0.0, 0.0), uv='C')
            m.board((x, Y0 - 0.48, BASE + 0.12), (x, Y0 - 0.1, BASE + 0.12), 0.05, 0.04, face=(1.0, 0.0, 0.0), uv='C')
        m.board((cx - length * 0.5 + 0.16, Y0 - 0.3, BASE + 0.13), (cx + length * 0.5 - 0.16, Y0 - 0.3, BASE + 0.13),
                0.06, 0.04, uv='C')
        m.hull((length + 0.04, 0.52, 0.95), at=(cx, Y0 - 0.29, BASE + 0.475))
    bench(X0 + 1.05)
    bench(X0 + 8.45)
    m.socket('Sit_1', (X0 + 1.05, Y0 - 0.33, BASE + 0.45))
    m.socket('Sit_2', (X0 + 8.45, Y0 - 0.33, BASE + 0.45))

    bx = X0 + BOARD_X
    lamp = (bx, Y0 - 0.4, BASE + 2.62)       # under the canopy's ledger, clear of heads
    m.box((0.1, 0.03, 0.26), at=(bx, Y0 - 0.015, lamp[2] + 0.38), uv=rail.IRON)
    m.box((0.035, 0.44, 0.035), at=(bx, Y0 - 0.22, lamp[2] + 0.45), uv=rail.IRON)
    m.board((bx, Y0 - 0.02, lamp[2] + 0.27), (bx, Y0 - 0.3, lamp[2] + 0.44), 0.025, 0.025, face=(1.0, 0.0, 0.0),
            uv=rail.IRON)
    m.box((0.02, 0.02, 0.18), at=(bx, lamp[1], lamp[2] + 0.36), uv=rail.IRON)
    rail.lantern(m, lamp, size=0.2)
    m.socket('StationBoard', (bx, Y0, BASE + 1.6))
    m.socket('Crepe', (X0 + D1.x + D1.w * 0.5, Y0 - 0.06, BASE + D1.h + 0.22))
    m.section('benches, lamp')

    # --- Steps down from the back door and the baggage door. ---
    def steps(space, x0, w):
        for k, (top, out) in enumerate(((0.4, 0.15), (0.2, 0.45))):
            m.board((x0 - 0.1, -out, top - 0.025), (x0 + w + 0.1, -out, top - 0.025), 0.3, 0.05, face=(0.0, 0.0, 1.0),
                    uv='A', space=space)
            m.board((x0 - 0.08, -out - 0.13, top * 0.5 - 0.02), (x0 + w + 0.08, -out - 0.13, top * 0.5 - 0.02),
                    top - 0.04, 0.03, uv='A', space=space)
        for x in (x0 - 0.13, x0 + w + 0.13):
            m.board((x, -0.62, 0.02), (x, 0.0, 0.36), 0.18, 0.05, face=(1.0, 0.0, 0.0), uv='C', space=space)
        m.hull((w + 0.3, 0.3, 0.4), at=(x0 + w * 0.5, -0.15, 0.2), space=space)
        m.hull((w + 0.3, 0.3, 0.2), at=(x0 + w * 0.5, -0.45, 0.1), space=space)
    steps(kit.wall_space((X1, Y1), (X0, Y1), 0.0), 4.2, 1.0)
    steps(kit.wall_space((X1, Y0), (X1, Y1), 0.0), 2.4, 1.2)
    m.section('steps')

    # --- RANSOM'S REST on both gable ends: dark boards in a red frame, cream lettering. ---
    for name in ('west', 'east'):
        space = spaces[name]
        cx, cz, w, h = D * 0.5, WALL + 0.5, 3.3, 0.62
        for k in range(3):
            z = cz - h * 0.5 + 0.105 + k * 0.205
            m.board((cx - w * 0.5, -0.05, z), (cx + w * 0.5, -0.05, z), 0.2, 0.035, uv=Trim('C', lane=k + 1), space=space)
        for z in (cz - h * 0.5 - 0.03, cz + h * 0.5 + 0.03):
            m.board((cx - w * 0.5 - 0.06, -0.06, z), (cx + w * 0.5 + 0.06, -0.06, z), 0.07, 0.05, uv='H2', space=space)
        for x in (cx - w * 0.5 - 0.03, cx + w * 0.5 + 0.03):
            m.board((x, -0.06, cz - h * 0.5), (x, -0.06, cz + h * 0.5), 0.07, 0.05, uv='H2', space=space)
        rail.lettering(m, "RANSOM'S REST", 0.32, at=(cx, -0.0715, cz), space=space, width=w - 0.3, simplify=30.0)
    m.section('signs')

    # --- Collision: the body, the bay, the roof, the canopy and the chimney (benches and steps above). ---
    m.hull((L + 0.16, D + 0.16, EAVE), at=(0.0, 0.0, EAVE * 0.5))
    m.hull_points([(p[0], p[1], z) for p in (A, B, C, Dp) for z in (BASE, BASE + BAY_H + 0.1)])
    back_eave = EAVE - 0.45 * TAN
    m.hull_points([(x, Y0, EAVE) for x in (X0 - 0.45, X1 + 0.45)] + [(x, Y1 + 0.45, back_eave) for x in (X0 - 0.45, X1 + 0.45)]
                  + [(x, 0.0, EAVE + RISE + 0.22) for x in (X0 - 0.45, X1 + 0.45)])
    m.hull((canopy.width, canopy.length, 0.16), at=(canopy.width * 0.5, canopy.length * 0.5, -0.04), space=canopy)
    m.hull((0.7, 0.7, top - roof_z + 0.3), at=(ch_x, ch_y, (top + roof_z + 0.3) * 0.5))
    return m


def gable2(m, x0, x1, y0, y1, eave_z, pitch, over_front, over_back, rake, deck=0.12, sag=0.0):
    """kit.gable with its own overhang on each side (the depot's front slope stops at the wall, where its canopy
    starts). Returns the front and back Slopes."""
    theta = math.radians(pitch)
    half = (y1 - y0) * 0.5
    width = x1 - x0 + 2.0 * rake
    slopes = []
    for side, over in ((1.0, over_front), (-1.0, over_back)):
        length = (half + over) / math.cos(theta) + deck * math.tan(theta)
        normal = Vector((0.0, -side * math.sin(theta), math.cos(theta)))
        y_eave = y0 - over if side > 0 else y1 + over
        x_start = x0 - rake if side > 0 else x1 + rake
        eave = Vector((x_start, y_eave, eave_z - over * math.tan(theta))) + normal * deck
        slope = kit.Slope(eave, (side, 0.0, 0.0), (0.0, side * math.cos(theta), math.sin(theta)), width, length, sag)
        roof_deck(m, slope, deck)
        slopes.append(slope)
    return slopes


def roof_deck(m, slope, deck=0.12, every=2.8):
    """kit.roof_deck with a cut only every `every` metres (its top is under the covering, its underside shows only
    under the eaves), and the rake boards along its sides."""
    cuts = max(1, int(slope.width / every))
    m.box((slope.width, slope.length, deck), at=(slope.width * 0.5, slope.length * 0.5, -deck * 0.5),
          uv=Trim('A', world=True), space=slope, cuts=cuts)
    for side, x in enumerate((-0.025, slope.width + 0.025)):
        face = (-1.0, 0.0, 0.0) if side == 0 else (1.0, 0.0, 0.0)
        mid = (0.06 - deck - 0.05) * 0.5
        m.board((x, -0.02, mid), (x, slope.length - 0.02, mid), deck + 0.11, 0.05, face=face, uv='C', space=slope)


# --- The platform kit ---

EDGE_W = 0.16       # the edge timbers along both sides


def platform_side(m, x0, x1, y_edge, side, posts):
    """One long side of the platform: an edge timber flush with the deck, a boarded face set back under it and posts
    in front of the face (side -1: the track side at y_edge < 0)."""
    top = PLATFORM_TOP
    yc = y_edge - side * EDGE_W * 0.5
    m.board((x0, yc, top - 0.05), (x1, yc, top - 0.05), EDGE_W, 0.1, face=(0.0, 0.0, 1.0), uv=rail.Planks(), mat='planks')
    face_y = y_edge - side * 0.19
    m.box((x1 - x0, 0.04, 0.42), at=((x0 + x1) * 0.5, face_y, top - 0.1 - 0.21), uv=planks_at(top - 0.1),
          mat='planks', cuts=max(1, int((x1 - x0) / 1.0)) - 1, drop=('+z',))
    for x in posts:
        m.board((x, y_edge - side * 0.085, -0.15), (x, y_edge - side * 0.085, top - 0.1), 0.14, 0.14,
                face=(0.0, side, 0.0), uv=rail.Planks(), mat='planks')


def platform_4m():
    m = rail.Model('DepotPlatform_4m', seed=221)
    rng = m.rng
    half = PLATFORM_DEPTH * 0.5
    for k in range(int(PIECE / 0.2)):
        x = 0.1 + k * 0.2 + rng.uniform(-0.004, 0.004)
        z = PLATFORM_TOP - 0.025 + rng.uniform(-0.005, 0.0)
        y0 = -half + EDGE_W - rng.uniform(0.0, 0.01)
        y1 = half - EDGE_W + rng.uniform(0.0, 0.01)
        m.board((x, y0, z), (x + rng.uniform(-0.006, 0.006), y1, z), 0.186, 0.05, face=(0.0, 0.0, 1.0),
                uv=rail.Planks(row=rng.randrange(rail.PLANK_ROWS)), mat='planks', cuts=3)
    for side in (-1.0, 1.0):
        platform_side(m, 0.0, PIECE, side * half, side, (1.0, 3.0))
    m.hull((PIECE, PLATFORM_DEPTH, PLATFORM_TOP + 0.1), at=(PIECE * 0.5, 0.0, (PLATFORM_TOP - 0.1) * 0.5))
    return m


def platform_end():
    m = rail.Model('DepotPlatform_End', seed=231)
    rng = m.rng
    half = PLATFORM_DEPTH * 0.5
    a = math.atan2(PLATFORM_TOP, RAMP)
    normal = (math.sin(a), 0.0, math.cos(a))
    count = int(round(RAMP / math.cos(a) / 0.2))
    for k in range(count):
        t = (k + 0.5) / count
        x = RAMP * t
        z = PLATFORM_TOP * (1.0 - t) - 0.025 * math.cos(a)
        m.board((x, -half + EDGE_W - rng.uniform(0.0, 0.01), z), (x, half - EDGE_W + rng.uniform(0.0, 0.01), z),
                0.186, 0.05, face=normal, uv=rail.Planks(row=rng.randrange(rail.PLANK_ROWS)), mat='planks', cuts=3)
    for side in (-1.0, 1.0):
        y_edge = side * half
        yc = y_edge - side * EDGE_W * 0.5
        m.board((0.0, yc, PLATFORM_TOP - 0.05), (RAMP + 0.05, yc, -0.06), EDGE_W, 0.1, face=normal, uv=rail.Planks(),
                mat='planks')
        # The face under it, cut to the slope.
        face_y = y_edge - side * 0.19
        p0, p1 = ((0.0, face_y), (RAMP, face_y)) if side < 0 else ((RAMP, face_y), (0.0, face_y))
        space = kit.wall_space(p0, p1, 0.0)
        if side < 0:
            outline = [(0.0, -0.1), (RAMP, -0.1), (0.0, PLATFORM_TOP - 0.1)]
        else:
            outline = [(0.0, -0.1), (RAMP, -0.1), (RAMP, PLATFORM_TOP - 0.1)]
        m.panel(outline, (), 0.04, planks_at(PLATFORM_TOP - 0.1), mat='planks', space=space, slices=0.8)
        x = 0.55
        m.board((x, y_edge - side * 0.085, -0.15), (x, y_edge - side * 0.085, PLATFORM_TOP * (1.0 - x / RAMP) - 0.12),
                0.14, 0.14, face=(0.0, side, 0.0), uv=rail.Planks(), mat='planks')
    m.hull_points([(x, y, z) for y in (-half, half) for x, z in ((0.0, PLATFORM_TOP), (0.0, -0.1), (RAMP, 0.0),
                                                                    (RAMP, -0.1))])
    return m


# --- The water tower ---

TANK_R, TANK_Z0, TANK_H = 2.1, 5.25, 3.3        # the tank's radius, its bottom (on the joists) and its height
SPOUT_HINGE = (0.4, -2.38, 4.85)                 # under the tank's front edge, toward the track, between two joists
SEGMENTS = 24


def _emit_lp(m, obj, mat='trim', place=None):
    """Moves a looter_props part (already UV-mapped) into the model and deletes it."""
    import bpy
    tb = bmesh.new()
    tb.from_mesh(obj.data)
    mesh = obj.data
    bpy.data.objects.remove(obj)
    bpy.data.meshes.remove(mesh)
    m.emit(rail.fill(tb), None, mat, place)


def _cone_rows(m, r0, z0, r1, z1, rows, lap=0.08, lift=0.025):
    """A cone of black-painted tin (IronBlack) in rows of sheets from the eave up, each row's foot resting on the top
    of the row below; mapped at world scale round the cone."""
    per_m = 1.0 / 3.2                    # MetalWorn repeats every 3.2 m
    slant = math.hypot(r0 - r1, z1 - z0)
    step = (slant + lap * (rows - 1)) / rows
    for k in range(rows):
        d0 = k * (step - lap)
        d1 = min(d0 + step, slant)
        ra, za = r0 + (r1 - r0) * d0 / slant, z0 + (z1 - z0) * d0 / slant
        rb, zb = r0 + (r1 - r0) * d1 / slant, z0 + (z1 - z0) * d1 / slant
        out = lift * (rows - k)              # each row's foot rests on the top of the row below it
        tb = kit._new_bmesh()
        uv = tb.loops.layers.uv.active
        lo = [tb.verts.new(((ra + out) * math.cos(a), (ra + out) * math.sin(a), za))
              for a in (2.0 * math.pi * i / SEGMENTS for i in range(SEGMENTS))]
        hi = [tb.verts.new(((rb + out * 0.4) * math.cos(a), (rb + out * 0.4) * math.sin(a), zb))
              for a in (2.0 * math.pi * i / SEGMENTS for i in range(SEGMENTS))]
        u_shift = m.rng.uniform(0.0, 3.2)
        for i in range(SEGMENTS):
            j = (i + 1) % SEGMENTS
            face = tb.faces.new((lo[i], lo[j], hi[j], hi[i]))
            face.normal_update()
            if face.normal.z < 0.0:
                face.normal_flip()
            for loop in face.loops:
                # U by the row's foot radius at both edges, V the distance up the slope from the eave.
                a = 2.0 * math.pi * (i + (1 if loop.vert in (lo[j], hi[j]) else 0)) / SEGMENTS
                d = d0 if loop.vert in (lo[i], lo[j]) else d1
                loop[uv].uv = ((a * ra + u_shift) * per_m, d * per_m)
        m.emit(tb, None, 'ironblack', ao_floor=kit.ROOF_AO)


def water_tower():
    import looter_props as lp
    m = rail.Model('WaterTower', seed=241)
    rng = m.rng
    BASE_HALF, TOP_HALF, TRESTLE = 1.55, 1.3, 4.7
    corners = [(-1.0, -1.0), (1.0, -1.0), (1.0, 1.0), (-1.0, 1.0)]

    def leg(z):
        return BASE_HALF + (TOP_HALF - BASE_HALF) * (z - 0.45) / (TRESTLE - 0.45)

    # Fieldstone piers, battered timber legs, girts and cross braces on every face.
    for sx, sy in corners:
        kit.stone_stack(m, (sx * BASE_HALF, sy * BASE_HALF, -0.1), (0.62, 0.62), 0.55, taper=0.1,
                        rot=rng.uniform(-6.0, 6.0))
        m.board((sx * BASE_HALF, sy * BASE_HALF, 0.42), (sx * TOP_HALF, sy * TOP_HALF, TRESTLE), 0.28, 0.28,
                face=(0.0, sy, 0.0), uv=Trim('C', lane='each'), bevel=0.02)
    GIRT = 2.55
    for (ax, ay), (bx, by) in zip(corners, corners[1:] + corners[:1]):
        w = leg(GIRT)
        out = (ax + bx, ay + by, 0.0)
        m.board((ax * w, ay * w, GIRT), (bx * w, by * w, GIRT), 0.2, 0.12, face=out, uv=Trim('C', lane='each'),
                lift=0.14)
        for z0, z1 in ((0.75, GIRT - 0.1), (GIRT + 0.1, TRESTLE - 0.2)):
            w0, w1 = leg(z0), leg(z1)
            for p0, p1 in (((ax * w0, ay * w0, z0), (bx * w1, by * w1, z1)), ((bx * w0, by * w0, z0), (ax * w1, ay * w1, z1))):
                m.board(p0, p1, 0.14, 0.06, face=out, uv='C', lift=0.15 + (0.0 if p0[0] * ax > 0 else 0.06))
    # Cap beams across the leg tops and joists under the tank.
    for sy in (-1.0, 1.0):
        m.board((-TOP_HALF - 0.3, sy * TOP_HALF, TRESTLE + 0.15), (TOP_HALF + 0.3, sy * TOP_HALF, TRESTLE + 0.15), 0.3,
                0.3, face=(0.0, sy, 0.0), uv=Trim('C', lane='each'))
    for x in (-1.6, -0.8, 0.0, 0.8, 1.6):
        span = math.sqrt(max(TANK_R ** 2 - x * x, 0.0)) + 0.25
        m.board((x, -span, TRESTLE + 0.425), (x, span, TRESTLE + 0.425), 0.25, 0.2, face=(1.0, 0.0, 0.0),
                uv=Trim('C', lane='each'))
    m.section('trestle')

    # The tank: staves (each its own board), six iron hoops closer together low down where the water presses hardest,
    # a board bottom; lugs where each hoop's ends are drawn together.
    hoops = [0.16, 0.5, 0.9, 1.38, 1.95, 2.65]
    proud = 0.016
    rows = [(0.0, 0.05, 'bottom'), (TANK_R - 0.03, 0.05, 'bottom'), (TANK_R, 0.0, 'stave')]
    for h in hoops:
        rows += [(TANK_R, h - 0.03, 'stave'), (TANK_R + proud, h - 0.03, 'iron'), (TANK_R + proud, h + 0.03, 'iron'),
                 (TANK_R, h + 0.03, 'iron')]
    rows += [(TANK_R, TANK_H, 'stave'), (TANK_R - 0.06, TANK_H, 'stave'), (0.0, TANK_H - 0.02, 'bottom')]
    body, bands = lp.lathe([(r, TANK_Z0 + z) for r, z, _ in rows], segments=SEGMENTS)
    kinds = [rows[b + 1][2] for b in bands]
    lp.lathe_uv(body, 'Siding', [i for i, k in enumerate(kinds) if k == 'stave'], grain='up', seed=rng.randrange(999),
                per_column=True)
    lp.lathe_uv(body, 'Iron', [i for i, k in enumerate(kinds) if k == 'iron'], grain='around', seed=rng.randrange(999))
    kit.lt.trim_uv(body, [i for i, k in enumerate(kinds) if k == 'bottom'], 'Siding', align='world', cut=True)
    _emit_lp(m, body)
    for h in hoops:
        a = math.radians(-60.0)
        m.box((0.07, 0.06, 0.09), at=((TANK_R + 0.04) * math.cos(a), (TANK_R + 0.04) * math.sin(a), TANK_Z0 + h),
              rot=(0.0, 0.0, math.degrees(a) + 90.0), uv=rail.IRON)
    m.section('tank')

    # The roof: a cone of black-painted tin with a soffit under its overhang and a finial.
    eave_z, roof_r, apex = TANK_Z0 + TANK_H + 0.05, TANK_R + 0.28, TANK_Z0 + TANK_H + 1.2
    _cone_rows(m, roof_r, eave_z, 0.12, apex - 0.06, 4)
    soffit = kit._new_bmesh()
    outer = [soffit.verts.new((roof_r * math.cos(a), roof_r * math.sin(a), eave_z - 0.01))
             for a in (2.0 * math.pi * i / SEGMENTS for i in range(SEGMENTS))]
    inner = [soffit.verts.new(((TANK_R - 0.05) * math.cos(a), (TANK_R - 0.05) * math.sin(a), eave_z - 0.01))
             for a in (2.0 * math.pi * i / SEGMENTS for i in range(SEGMENTS))]
    for i in range(SEGMENTS):
        j = (i + 1) % SEGMENTS
        face = soffit.faces.new((outer[i], outer[j], inner[j], inner[i]))
        face.normal_update()
        if face.normal.z > 0.0:
            face.normal_flip()
    m.emit(soffit, Trim('A', world=True), 'trim')
    m.cylinder((0.0, 0.0, apex - 0.12), (0.0, 0.0, apex + 0.08), 0.16, 0.05, sides=8, uv=rail.IRON,
               cap_uv=rail.IRON)
    m.cylinder((0.0, 0.0, apex + 0.08), (0.0, 0.0, apex + 0.42), 0.035, sides=6, uv=rail.IRON, cap_uv=rail.IRON,
               caps=(False, True))
    m.box((0.12, 0.12, 0.12), at=(0.0, 0.0, apex + 0.3), rot=(45.0, 35.0, 0.0), uv=rail.IRON)
    m.section('roof')

    # A ladder up the west side (toward the platform) from the ground to under the eave, braced to the trestle.
    lx, ly = -(TANK_R + 0.17), 0.32
    for y in (ly - 0.22, ly + 0.22):
        m.board((lx, y, 0.0), (lx, y, eave_z - 0.12), 0.07, 0.05, face=(-1.0, 0.0, 0.0), uv=Trim('C', lane='each'))
    for z in kit.frange(0.3, eave_z - 0.2, 0.32):
        m.box((0.03, 0.44, 0.03), at=(lx, ly, z), uv=rail.IRON, drop=('-y', '+y'))
    for z, reach in ((GIRT, leg(GIRT)), (TRESTLE + 0.15, TOP_HALF + 0.3)):
        for y in (ly - 0.22, ly + 0.22):
            m.board((lx + 0.02, y, z), (-reach + 0.05, y, z), 0.05, 0.05, face=(0.0, -1.0, 0.0),
                    uv=rail.IRON)
    for z in (TANK_Z0 + 1.2, TANK_Z0 + 2.6):
        for y in (ly - 0.22, ly + 0.22):
            m.board((lx + 0.02, y, z), (-TANK_R + 0.02, y, z), 0.04, 0.04, face=(0.0, -1.0, 0.0),
                    uv=rail.IRON)
    m.section('ladder')

    # The float gauge on the front: a board on brackets with a red pointer, its cable over a pulley on the rim.
    gx = -0.55
    wall_y = -math.sqrt(TANK_R ** 2 - gx * gx)
    gy = wall_y - 0.15
    m.board((gx, gy, TANK_Z0 + 0.2), (gx, gy, TANK_Z0 + TANK_H - 0.1), 0.18, 0.04, uv=Trim('C', lane=1))
    for z in (TANK_Z0 + 0.4, TANK_Z0 + 1.7, TANK_Z0 + 3.0):
        m.box((0.04, 0.15, 0.04), at=(gx, wall_y - 0.075, z), uv=rail.IRON)
    pointer = kit._new_bmesh()
    tri = [pointer.verts.new(p) for p in ((-0.12, 0.0, 0.0), (0.06, 0.0, 0.07), (0.06, 0.0, -0.07))]
    pointer.faces.new(tri)
    m.emit(rail.fill(pointer), Trim('H2', fit=True), 'trim', kit.place((gx + 0.02, gy - 0.035, TANK_Z0 + 2.1)))
    m.box((0.1, 0.12, 0.1), at=(gx, wall_y - 0.09, TANK_Z0 + TANK_H + 0.02), uv=rail.IRON)
    m.box((0.012, 0.012, TANK_H - 1.3), at=(gx + 0.06, gy - 0.03, TANK_Z0 + 2.15 + (TANK_H - 1.3) * 0.5 - 0.15),
          uv=rail.IRON)
    m.section('gauge')

    # The outlet: a pipe down from the tank's bottom between two joists and out to the spout's hinge.
    hx, hy, hz = SPOUT_HINGE
    m.cylinder((hx, -1.75, TANK_Z0 + 0.05), (hx, -1.75, hz), 0.13, sides=8, uv=rail.IRON, cap_uv=rail.IRON,
               caps=(False, True))
    m.cylinder((hx, -1.75, hz), (hx, hy + 0.16, hz), 0.13, sides=8, uv=rail.IRON, cap_uv=rail.IRON,
               face=(0.0, 0.0, 1.0))
    m.box((0.5, 0.26, 0.34), at=(hx, hy + 0.12, hz), uv=rail.IRON, bevel=0.02)
    for x in (hx - 0.2, hx + 0.2):
        m.box((0.06, 0.22, 0.26), at=(x, hy + 0.02, hz), uv=rail.IRON)
    m.socket('Spout', SPOUT_HINGE)
    m.section('outlet')

    # Collision: the legs (on their piers), the tank and the roof.
    for sx, sy in corners:
        m.hull_points([(sx * leg(z) + dx, sy * leg(z) + dy, z) for z in (-0.1, TRESTLE) for dx in (-0.2, 0.2)
                       for dy in (-0.2, 0.2)])
    ring = [(math.cos(2.0 * math.pi * k / 12), math.sin(2.0 * math.pi * k / 12)) for k in range(12)]
    m.hull_points([(c * (TANK_R + 0.05), s * (TANK_R + 0.05), z) for c, s in ring
                   for z in (TRESTLE, TANK_Z0 + TANK_H)])
    m.hull_points([(c * roof_r, s * roof_r, eave_z) for c, s in ring] + [(0.0, 0.0, apex + 0.1)])
    return m


def water_tower_spout():
    """The spout: a hinge barrel across its pivot, the pipe out along -Y with an elbow and a canvas sock hanging from
    it, a counterweight arm behind the hinge. Modeled lowered and level; the game pitches it about X."""
    m = rail.Model('WaterTowerSpout', seed=251)
    metal = kit.Tile(rail.IRON_SET)
    m.cylinder((-0.22, 0.0, 0.0), (0.22, 0.0, 0.0), 0.15, sides=10, uv=metal, mat='ironblack', face=(0.0, 0.0, 1.0))
    m.cylinder((0.0, -0.1, 0.0), (0.0, -2.75, 0.0), 0.12, 0.115, sides=10, uv=metal, mat='ironblack', face=(0.0, 0.0, 1.0),
               caps=(False, False))
    for y in (-0.9, -1.85):
        m.cylinder((0.0, y + 0.04, 0.0), (0.0, y - 0.04, 0.0), 0.14, sides=10, uv=metal, mat='ironblack',
                   face=(0.0, 0.0, 1.0))
    m.cylinder((0.0, -2.75, 0.0), (0.0, -3.05, -0.2), 0.115, 0.11, sides=10, uv=metal, mat='ironblack',
               face=(0.0, 0.0, 1.0), caps=(False, False))
    m.cylinder((0.0, -3.05, -0.2), (0.0, -3.12, -0.62), 0.11, 0.08, sides=10, uv=metal, mat='ironblack',
               face=(0.0, 1.0, 0.0), caps=(False, True))
    m.box((0.06, 0.75, 0.06), at=(0.0, 0.42, 0.04), uv=metal, mat='ironblack')
    m.box((0.28, 0.22, 0.28), at=(0.0, 0.78, 0.0), uv=metal, mat='ironblack', bevel=0.02)
    # A pull ring under the elbow for the fireman's rope.
    m.box((0.03, 0.03, 0.14), at=(0.0, -2.9, -0.2), uv=metal, mat='ironblack')
    return m


# --- The coffin shed ---

def coffin_outline(length, foot, shoulder, head):
    """A coffin lid's outline (x across, z along it from the foot), widest at the shoulders."""
    s = length * 0.72
    return [(-foot * 0.5, 0.0), (foot * 0.5, 0.0), (shoulder * 0.5, s), (head * 0.5, length), (-head * 0.5, length),
            (-shoulder * 0.5, s)]


def coffin_shed():
    m = rail.Model('CoffinShed', seed=261)
    rng = m.rng
    X0, X1, Y0, Y1 = -1.6, 1.6, -1.2, 1.2
    BASE = 0.18
    FRONT_H, BACK_H = 2.45, 2.05          # wall heights over the floor: the tin sheds water to the back
    THICK = 0.06
    W, D = X1 - X0, Y1 - Y0

    def slope_z(y):
        return FRONT_H + (y - Y0) / D * (BACK_H - FRONT_H)

    # Flat stones under the corners and the middle of the sides, a plank floor on them.
    for x in (X0 + 0.2, 0.0, X1 - 0.2):
        for y in (Y0 + 0.2, Y1 - 0.2):
            kit.stone_stack(m, (x, y, -0.04), (0.36, 0.36), BASE - 0.02, rot=rng.uniform(-10.0, 10.0))
    m.box((W + 0.04, D + 0.04, 0.05), at=(0.0, 0.0, BASE - 0.025), uv=Trim('A', world=True))
    # Walls of upright boards; the door and a small window in front.
    DOOR = Opening('door', 0.45, 0.0, 1.0, 2.12, paint='A', casing='C', hinge='left')
    WIN = Opening('window', 2.15, 1.15, 0.6, 0.6, style='glass', paint='C', sash='C', panes=(2, 2), casing=0.1)
    walls = [((X0, Y0), (X1, Y0), [DOOR, WIN]), ((X1, Y0), (X1, Y1), []), ((X1, Y1), (X0, Y1), []),
             ((X0, Y1), (X0, Y0), [])]
    for p0, p1, items in walls:
        space = kit.wall_space(p0, p1, BASE)
        length = (Vector(p1) - Vector(p0)).length
        z_start, z_end = slope_z(p0[1]), slope_z(p1[1])
        outline = kit.wall_outline(length, z_end, items)
        outline[-1] = (0.0, z_start)
        facade(m, outline, kit.holes(items), THICK, Trim('A', world=True, rotate=True), space=space, around=items,
               slices=0.0)
        kit.openings(m, space, items, THICK)
        for x, zt in ((0.05, z_start), (length - 0.05, z_end)):
            kit.trim_board(m, space, (x, 0.0, -0.08), (x, 0.0, zt - 0.03), 0.1, 'C', thick=0.035)
        kit.trim_board(m, space, (0.1, 0.0, 0.06), (length - 0.1, 0.0, 0.06), 0.12, 'C', thick=0.03)
    m.section('walls')

    # The roof: shakes on a plank deck, sloping to the back, like the depot's.
    theta = math.atan2(FRONT_H - BACK_H, D)
    over, deck = 0.25, 0.05
    normal = Vector((0.0, math.sin(theta), math.cos(theta)))
    origin = Vector((X1 + 0.2, Y1 + over, BASE + BACK_H - over * math.tan(theta))) + normal * deck
    roof = kit.Slope(origin, (-1.0, 0.0, 0.0), (0.0, -math.cos(theta), math.sin(theta)), W + 0.4,
                     (D + 2.0 * over) / math.cos(theta), sag=0.03)
    roof_deck(m, roof, deck, every=1.6)
    shakes(m, roof, piece=(1.2, 2.2))
    m.section('roof')

    # Coffin lids leaning on the east wall, the small one in front; lumber leaning on the west wall.
    lids = [(-0.62, 0.42, 1.95, 0.32, 0.62, 0.44, 4.0), (0.02, 0.5, 1.95, 0.32, 0.62, 0.44, -3.0),
            (0.64, 0.58, 1.9, 0.3, 0.6, 0.42, 2.0), (0.12, 0.7, 1.15, 0.22, 0.4, 0.3, -6.0)]
    for y_c, out, length, foot, shoulder, head, roll in lids:
        top_out = 0.06 + (out - 0.06) * 0.08
        up = Vector((-(out - top_out), 0.0, length)).normalized()
        across = Vector((0.0, 1.0, 0.0))
        frame = kit.basis((X1 + out, y_c, 0.0), across, up.cross(across), up) @ \
            Matrix.Rotation(math.radians(roll), 4, 'Y')
        lid = kit._prism([coffin_outline(length, foot, shoulder, head)], 0.035)
        m.emit(lid, Trim('A', world=True, rotate=True), 'trim', frame)
    for k, (y_c, lean) in enumerate(((-0.55, 0.32), (-0.3, 0.38), (0.4, 0.3))):
        p0 = (X0 - lean, y_c, 0.02)
        p1 = (X0 - 0.05, y_c + rng.uniform(-0.05, 0.05), 2.2 + rng.uniform(-0.1, 0.1))
        m.board(p0, p1, 0.19, 0.03, face=(-1.0, 0.0, 0.2), uv='A')
    m.section('lids and lumber')

    # Two sawhorses before the shed with a lid lying across them.
    for x in (0.55, 1.75):
        y_c = Y0 - 1.25
        m.board((x, y_c - 0.38, 0.72), (x, y_c + 0.38, 0.72), 0.1, 0.08, face=(0.0, 0.0, 1.0), uv='C')
        for dy in (-0.3, 0.3):
            for dx in (-1.0, 1.0):
                m.board((x + dx * 0.04, y_c + dy, 0.68), (x + dx * 0.24, y_c + dy * 1.1, 0.0), 0.06, 0.05,
                        face=(dx, 0.0, 0.0), uv='C')
    # The lid lies face up, its foot toward -X: its length (the prism's Z) along +X, its front (-Y) up.
    lying = kit._prism([coffin_outline(1.95, 0.32, 0.62, 0.44)], 0.035)
    m.emit(lying, Trim('A', world=True, rotate=True), 'trim',
           Matrix.Translation((0.17, Y0 - 1.25, 0.795)) @ Matrix.Rotation(math.radians(-90.0), 4, 'Z') @
           Matrix.Rotation(math.radians(-90.0), 4, 'X'))
    m.section('sawhorses')

    # Collision: the shed, its roof, the leaning lids, the sawhorses with their load.
    m.hull((W + 0.08, D + 0.08, BASE + BACK_H), at=(0.0, 0.0, (BASE + BACK_H) * 0.5))
    m.hull((roof.width, roof.length, 0.12), at=(roof.width * 0.5, roof.length * 0.5, -0.02), space=roof)
    m.hull_points([(X1, y, z) for y in (-1.0, 1.0) for z in (0.0, 1.9)] + [(X1 + 0.75, y, 0.0) for y in (-1.0, 1.0)])
    m.hull((1.95, 0.85, 0.85), at=(1.15, Y0 - 1.25, 0.425))
    return m


# --- Build ---

depot_model = depot()
# About 10.1k triangles: the fallback keeps 78% (about 7.9k), flattening the thinnest boards and shake butts first.
depot_obj = depot_model.finish(preview=False, fallback=78.0)
piece_obj = platform_4m().finish(preview=False, fallback=100.0, view=(-1.0, -1.4, 0.7))
end_obj = platform_end().finish(preview=False, fallback=100.0)
tower_obj = water_tower().finish(preview=False, fallback=100.0)
spout_obj = water_tower_spout().finish(ground=False, preview=False, Nanite=0, LODs='50,25', Collision='None')
shed_obj = coffin_shed().finish(preview=False, fallback=100.0)

if kit.lt.want_preview() or __name__ in ('__overview__', '__station__'):
    # The spout hangs on the tower's socket, raised the way it rests (models export from their own origins).
    spout_obj.location = SPOUT_HINGE
    spout_obj.rotation_euler = (math.radians(-50.0), 0.0, 0.0)
    if __name__ == '__overview__':
        spout_obj['Mounted'] = 1
if kit.lt.want_preview():
    kit.lt.preview([tower_obj, spout_obj], kit.lt.preview_path('Buildings', 'WaterTower'), view=(-1.0, -1.5, 0.3),
                   fit=0.95)
    spout_obj.location, spout_obj.rotation_euler = (0.0, 0.0, 0.0), (0.0, 0.0, 0.0)
    kit.lt.preview([spout_obj], rail.preview_path('Depot', 'WaterTowerSpout'), view=(-1.0, -0.8, 0.5), fit=0.9)
    kit.lt.preview([shed_obj], kit.lt.preview_path('Buildings', 'CoffinShed'), view=(1.0, -1.3, 0.5), fit=0.85)

if kit.lt.want_preview():
    import bpy
    # The depot on its platform: pieces run past both ends, the ramps beyond.
    shown = [depot_obj]
    extra = []
    track_y = -3.0 - PLATFORM_DEPTH - PLATFORM_EDGE
    mid_y = -3.0 - PLATFORM_DEPTH * 0.5
    for k in range(4):
        copy = piece_obj.copy()
        copy.name = f'Preview_Platform_{k}'
        copy.location = (-8.0 + k * PIECE, mid_y, 0.0)
        bpy.context.scene.collection.objects.link(copy)
        extra.append(copy)
    for x, turn in ((-8.0, math.pi), (8.0, 0.0)):
        copy = end_obj.copy()
        copy.name = f'Preview_End_{x}'
        copy.location = (x, mid_y, 0.0)
        copy.rotation_euler = (0.0, 0.0, turn)
        bpy.context.scene.collection.objects.link(copy)
        extra.append(copy)
    kit.lt.preview(shown + extra, kit.lt.preview_path('Buildings', 'Depot'), view=(-1.0, -1.25, 0.45), fit=0.72)
    kit.lt.preview(shown + extra, rail.preview_path('Depot', 'Depot_Front'), view=(0.25, -1.0, 0.18), fit=0.62)
    kit.lt.preview(shown + extra, rail.preview_path('Depot', 'Depot_Back'), view=(1.0, 1.3, 0.5), fit=0.8)
    kit.lt.preview([piece_obj], kit.lt.preview_path('Buildings', 'DepotPlatform_4m'), view=(-1.0, -1.4, 0.8), fit=0.9)
    kit.lt.preview([end_obj], kit.lt.preview_path('Buildings', 'DepotPlatform_End'), view=(-1.0, -1.4, 0.8), fit=0.9)
    for o in extra:
        bpy.data.objects.remove(o)
