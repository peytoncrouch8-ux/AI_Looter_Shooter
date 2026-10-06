"""The Chapel of Saint Ada on its knoll above Ransom's Rest (Docs/Areas/RansomsRest.md, zone "Chapel of Saint Ada",
Main 4 "Hallowed Ground"): a white clapboard frontier church, the kind a small Western town builds itself. Painted lap
siding (geometry, 18 cm to the weather) with corner boards, casings, a frieze under the eaves and a water table, the
paint grey and flaking at the foot and here and there a board sprung or bare, on a fieldstone footing under a steep
shake roof. The tower stands centred on the front gable: the oxide-red doors open at its foot under a little gabled
hood, a board-and-batten belfry with a louvred opening in each face round the bell, then a white shingled octagonal
spire with an iron cross, 17.9 m to the top of the cross, the tallest thing on Ransom's Rest. Tall pointed windows down
both sides, their glass mostly smashed since the night of the robbery, so the afternoon sun falls in across the pews.
A small apse at the back holds the plinth of the smashed Reliquary; a vestry leans on the right side by the sanctuary,
where Father Aldana keeps to his door, a lantern burning beside it.

Players walk in: the front doors stand open into the tower's vestibule, where the bell rope hangs, and an open pointed
doorway leads on into the nave: white boards on the walls, a plank floor, three pews either side of the aisle (one
knocked askew), a sanctuary two steps up with iron candle stands and a lectern, the apse with the Reliquary's plinth,
and the closed door to the vestry. Three oak trusses carry the roof.

Models (Art/README.md), built with looter_buildings:
- Chapel: the game's chapel, the clapboard church (option B, which the user picked on 2026-10-05). The pivot is on the
  ground on the nave's middle line, the tower toward the front (-Y). Nanite, its whole mesh the fallback. Sockets:
    SOCKET_Bell       the bell's swing axis in the belfry, 11.3 m up; the socket's front (-Y) runs along the axis
    SOCKET_Interact   the bell rope's woollen grip in the vestibule ("Ring the chapel bell")
    SOCKET_Reliquary  the top of the stone plinth in the apse: the smashed Reliquary's foot goes here, facing the nave
    SOCKET_Speaker    Father Aldana's vestry door on the right side, facing out (+X)
    SOCKET_Light      the lantern's glass beside the vestry door (an emissive pane, never a real light)
    SOCKET_Crepe      on the front door's head casing, facing out: where the graves kit's CrepeSwag hangs
    SOCKET_Perch_Hood     Hob's perch on the door hood's ridge
    SOCKET_Perch_Lantern  Hob's perch on the vestry lantern's bracket (Hob takes every socket whose name starts
                          with Perch; each socket needs its own name, or Unreal keeps only one)
  Collision: boxes for the footing and floors, the steps, the sanctuary, the apse floor and plinth, the walls (split
  round the doorways and the apse's arch; solid past the windows, whose sills are 1.35 m up), the door leaves, the
  pews, the tower above the vestibule (its louvred belfry out of anyone's reach), the cornice, the spire and the roofs,
  and the vestry as one closed block.
- ChapelBell: the bronze bell on its iron yoke. Its origin is the swing axis, which runs along its Y (front to back:
  the actor's forward, +X, in Unreal): code rolls it about that axis at SOCKET_Bell, so it swings left and right as
  seen from the front. No Nanite (it moves), LODs, no collision.
- Chapel_A: option A, not picked: the same chapel in timber frame, dark oak over lime plaster, with an open belfry and
  a shingled pyramid spire, 16 m to the cross. Built for the previews and the churchyard vignette only, never
  exported (when exporting, its code runs into a looter_chapel.NullModel, which keeps nothing).

    blender -b --factory-startup --python-expr "import sys; sys.path.insert(0, 'Tools/Blender')" \
        --python Art/Models/Buildings/Chapel.py -- --preview
renders Saved/ArtPreviews/Buildings/Chapel.png and Chapel_Back.png, in Saved/ArtPreviews/RansomsRest the interior
(Chapel_Interior.png), a close three-quarter view and the bell (Chapel/), and Chapel_A from the same cameras
(Buildings/Chapel_A.png, Chapel_A_Back.png, RansomsRest/Chapel/Chapel_A_Interior.png, Chapel_A_ThreeQuarter.png).
"""
import bmesh
import bpy
import looter_buildings as kit
from looter_buildings import Opening, Space, Tile, Trim, math
from mathutils import Matrix, Vector

import looter_chapel as lc
import looter_textures as lt

# Option A, the timber-frame chapel, comes first: the clapboard chapel after it shares its sizes, openings, roof lines,
# helpers and furnishings. The user picked the clapboard one, so A is built only for the previews and the churchyard
# vignette; otherwise its code runs into a NullModel, which keeps nothing (its own seed: leaving it out changes nothing).
BUILD_A = lt.want_preview() or __name__ == '__vignette__'
m = kit.Model('Chapel_A', seed=1888) if BUILD_A else lc.NullModel('Chapel_A', seed=1888)
rng = m.rng

# --- Sizes (metres; x across the nave, y from the tower at the front to the apse at the back, z up) ---
GRID = 0.8           # the timber grid: plaster is mapped in upright 0.8 m bands, every seam under a stud
BASE = 0.4           # top of the fieldstone footing
FLOOR = BASE + 0.03  # the plank floor
WALL = 4.4           # footing to the top of the wall plate
EAVE = BASE + WALL
PITCH = 50.0
THETA = math.radians(PITCH)
THICK = 0.16
X0, X1, Y0, Y1 = -3.2, 3.2, -4.0, 5.6        # the nave's outer wall faces
RISE = (X1 - X0) * 0.5 * math.tan(THETA)
TX = 1.6                                      # the tower's half width; it stands in front of the nave
TY0, TY1 = -7.2, Y0
TYC = (TY0 + TY1) * 0.5
SHAFT = 9.6          # the tower shaft's top: the belfry floor
BELFRY = 12.1        # the top of the belfry's plate, where the spire's eaves are
TIP = 15.1           # the spire's point (the cross stands 0.95 m above it)
AXIS_Z = 11.33       # the bell's swing axis
AX, AY = 1.2, 7.2    # the apse: x within +-AX, y from Y1 to AY
VX, VY = 5.6, 2.4    # the vestry: x from X1 to VX, y from VY to Y1
STEP = 0.18
SANCT = 2 * STEP     # the sanctuary, two steps up
SANCT_Y = 3.5        # its front edge (the step in front of it starts 0.3 m nearer)
POST, STUD = 0.22, 0.13
OUT, IN = (0.0, -1.0, 0.0), (0.0, 1.0, 0.0)

IRON = lt.material('MetalWorn', name='IronBlack', tint=0x2e2c2a)
m.slot('trim')
m.slot('planks')
m.slots['iron'] = (len(m.slots), IRON)
IRON_UV = Tile('MetalWorn')
PLANKS = Tile('WoodPlanks')
BEAM = lambda: Trim('C', lane='each')

# Where on the plaster strip (metres along it) each 0.8 m band starts at the foot of a wall: just below the stretch
# of big grey spalls and cracks (3.1-4.2 m along it), so the foot of every band is grey and cracked, a little higher
# or lower in each; the strip's clean run follows up the wall, its water stains under the eaves.
DAMAGE = (2.9, 3.15, 3.4)


class Plaster:
    """Lime plaster (strip G) in upright 0.8 m bands, as on the cottage, but each band slid along the strip by its own
    amount from DAMAGE: the plaster is grey and cracked at the foot of the walls and differs from bay to bay."""

    def apply(self, obj, rng):
        lt.trim_uv(obj, None, 'G', rotate=True, align='world', cut=True, stagger=False)
        mesh = obj.data
        uvs = mesh.uv_layers.active.data
        scale = lt.TRIM_DENSITY / lt.TRIM_SIZE
        starts = {}
        for poly in mesh.polygons:
            band = math.floor(poly.center.x / GRID + 1e-3)
            if band not in starts:
                starts[band] = rng.choice(DAMAGE) + rng.uniform(-0.08, 0.08)
            for index in poly.loop_indices:
                uvs[index].uv.x += starts[band] * scale


def hole(o):
    """A window's hole in its wall: a rectangle with a pointed head (opts head), or a 12-sided round window."""
    if o.opts.get('round'):
        r = o.w * 0.5
        return [(o.x + r + r * math.cos(2.0 * math.pi * k / 12), o.z + r + r * math.sin(2.0 * math.pi * k / 12))
                for k in range(12)]
    points = [(o.x, o.z), (o.x + o.w, o.z), (o.x + o.w, o.z + o.h)]
    if o.opts.get('head'):
        points.append((o.x + o.w * 0.5, o.z + o.h + o.opts['head']))
    points.append((o.x, o.z + o.h))
    return points


def top_of(o):
    return o.z + o.h + o.opts.get('head', 0.0)


def wall(p0, p1, items=(), height=WALL, top=None, bottom=None, z0=BASE, cuts=(0.3,), cuts_x=(), drop=None, uv=None):
    """A plaster wall from p0 to p1 (the outside on the right): its outline notched for doors standing on its foot
    (with a pointed head if they have one), holes for windows, the top level at height or along top (points from
    right to left), the foot level or along bottom (points from left to right). Sliced across at cuts (and upright at
    cuts_x), for the baked shading; drop(center, normal) -> True leaves out a face nobody sees (inside the tower's
    shaft, inside the closed vestry). uv maps it (the plaster by default). Returns (space, length)."""
    space = kit.wall_space(p0, p1, z0)
    length = (Vector(p1) - Vector(p0)).length
    hidden = []   # the outline's edges nobody sees: the foot on the footing, the top under a roof or a floor
    if bottom is not None:
        points = list(bottom)
        hidden += list(zip(points, points[1:]))
    else:
        points = [(0.0, 0.0)]
        for o in sorted((o for o in items if o.kind == 'door'), key=lambda o: o.x):
            hidden.append((points[-1], (o.x, 0.0)))
            points += [(o.x, 0.0), (o.x, o.h)]
            if o.opts.get('head'):
                points.append((o.x + o.w * 0.5, o.h + o.opts['head']))
            points += [(o.x + o.w, o.h), (o.x + o.w, 0.0)]
        hidden.append((points[-1], (length, 0.0)))
        points.append((length, 0.0))
    upper = list(top) if top is not None else [(length, height), (0.0, height)]
    hidden += list(zip(upper, upper[1:]))
    points += upper
    tb = kit._prism([points] + [hole(o) for o in items if o.kind != 'door'], THICK)
    z_lo, z_hi = min(z for x, z in points), max(z for x, z in points)
    kit._slice(tb, 2, sorted(z for z in set(cuts) if z_lo + 0.05 < z < z_hi - 0.05))
    kit._slice(tb, 0, [x for x in cuts_x if 0.05 < x < length - 0.05])

    def on_hidden_edge(c):
        for (ax, az), (bx, bz) in hidden:
            dx, dz = bx - ax, bz - az
            t = max(0.0, min(1.0, ((c.x - ax) * dx + (c.z - az) * dz) / max(dx * dx + dz * dz, 1e-12)))
            if math.hypot(c.x - (ax + dx * t), c.z - (az + dz * t)) < 0.004:
                return True
        return False
    tb.normal_update()
    # The ends sit inside the walls they meet, the foot on the footing, the top under a roof or a floor.
    gone = [f for f in tb.faces if abs(f.normal.y) < 0.2 and (
        (abs(f.normal.x) > 0.9 and (f.calc_center_median().x < 0.004 or f.calc_center_median().x > length - 0.004))
        or on_hidden_edge(f.calc_center_median()))]
    if drop is not None:
        gone += [f for f in tb.faces if f not in gone and drop(f.calc_center_median(), f.normal)]
    bmesh.ops.delete(tb, geom=gone, context='FACES_ONLY')
    m.emit(tb, uv or Plaster(), 'trim', None, space)
    return space, length


INSIDE_FACE = lambda c, n: n.y > 0.9                       # a wall's inner face (for walls nobody sees inside)
ABOVE_CEILING = lambda c, n: n.y > 0.9 and c.z > 3.25     # the tower's inner faces above the vestibule ceiling


def bar(space, p0, p1, size=0.045, uv='C'):
    """A square bar (a window's lattice), its ends left open where they meet the frame."""
    p0, p1 = Vector(p0), Vector(p1)
    length = (p1 - p0).length
    matrix = kit.toward(p0, p1, OUT) @ Matrix.Translation((length * 0.5, 0.0, 0.0))
    m.emit(kit._box((length, size, size), drop=('-x', '+x')), Trim(uv, width=size), 'trim', matrix, space)


def slab(points, thick):
    """A spire course: a flat piece (kit._prism) with only its face and its lower edge (the butt); its back, its
    ends under the hip boards and its top under the next course are left out."""
    tb = kit._prism([points], thick)
    tb.normal_update()
    bmesh.ops.delete(tb, geom=[f for f in tb.faces if not (f.normal.y < -0.99 or f.normal.z < -0.9)],
                     context='FACES_ONLY')
    return tb


def footing(x0, x1, y0, y1, height, out=0.08):
    """Fieldstone (strip D) round a footprint, standing out of the walls by out: kit.plinth's layout (the front and
    back run the whole width, the sides fit between), with the stone mapped so no band edge cuts it, and without the
    faces under the ground and the floor."""
    deep = 0.3
    specs = [((x0 - out, y0 - out), (x1 + out, y0 - out), 0.0), ((x1 + out, y0 - out), (x1 + out, y1 + out), deep),
             ((x1 + out, y1 + out), (x0 - out, y1 + out), 0.0), ((x0 - out, y1 + out), (x0 - out, y0 - out), deep)]
    for p0, p1, inset in specs:
        space = kit.wall_space(p0, p1, 0.0)
        length = (Vector(p1) - Vector(p0)).length - 2.0 * inset
        m.box((length, deep, height + 0.1), at=(inset + length * 0.5, deep * 0.5, height * 0.5 - 0.05),
              uv=Trim('D', world=True, v=0.3, u=rng.uniform(0.0, kit.U_REPEAT)), space=space,
              cuts=max(0, int(length / 1.8)), drop=('-z', '+y'))


def deck(slope, thick, rake_sides=(True, True), rake_uv='C'):
    """A roof's deck of planks (WoodPlanks: the ceiling inside, the soffit under the eaves), cut every 1.2 m along the
    eave so it sags with the covering, and the rake boards along its gable sides (as kit.roof_deck, which bands the
    trim sheet's boards across both faces; rake_uv maps them, oak by default)."""
    m.box((slope.width, slope.length, thick), at=(slope.width * 0.5, slope.length * 0.5, -thick * 0.5), uv=PLANKS,
          mat='planks', space=slope, cuts=max(1, int(slope.width / 1.2)), drop=('+y',))
    for side, x in enumerate((-0.025, slope.width + 0.025)):
        if rake_sides[side]:
            mid = (0.06 - thick - 0.05) * 0.5
            m.board((x, -0.02, mid), (x, slope.length - 0.02, mid), thick + 0.11, 0.05,
                    face=(-1.0, 0.0, 0.0) if side == 0 else (1.0, 0.0, 0.0), uv=rake_uv, space=slope)


def gable(x0, x1, y0, y1, eave_z, pitch, overhang=0.4, rake=0.3, thick=0.12, sag=0.0, ends=(True, True), frame=None,
          rake_uv='C'):
    """kit.gable's two slopes over walls from x0 to x1 and y0 to y1 (its ridge along X, frame to turn it), on a plank
    deck(). Returns the front and back Slopes."""
    theta = math.radians(pitch)
    run = (y1 - y0) * 0.5 + overhang
    length = run / math.cos(theta) + thick * math.tan(theta)
    width = x1 - x0 + 2.0 * rake
    n_f = Vector((0.0, -math.sin(theta), math.cos(theta)))
    front = kit.Slope(Vector((x0 - rake, y0 - overhang, eave_z - overhang * math.tan(theta))) + n_f * thick,
                      (1.0, 0.0, 0.0), (0.0, math.cos(theta), math.sin(theta)), width, length, sag, frame)
    n_b = Vector((0.0, math.sin(theta), math.cos(theta)))
    back = kit.Slope(Vector((x1 + rake, y1 + overhang, eave_z - overhang * math.tan(theta))) + n_b * thick,
                     (-1.0, 0.0, 0.0), (0.0, -math.cos(theta), math.sin(theta)), width, length, sag, frame)
    deck(front, thick, (ends[0], ends[1]), rake_uv)
    deck(back, thick, (ends[1], ends[0]), rake_uv)
    return front, back


def ridge(front, back, width=0.16, thick=0.05, lift=0.03):
    """An oak cap over the ridge, a board down each side, cut every 2.6 m or so to follow the sag."""
    for slope in (front, back):
        m.box((slope.width + 0.06, width, thick), matrix=kit.place((slope.width * 0.5, slope.length - width * 0.5 + 0.02,
              lift + thick * 0.5), (-2.0, 0.0, 0.0)), uv=Trim('C', lane=0), space=slope,
              cuts=max(1, int(slope.width / 2.6)), drop=('-z',))


def shakes(slope, piece=(2.6, 4.0), course=0.4, butt=0.035, start=-0.08, top=None, missing=0.02):
    """Wooden shakes (strip E) in 0.4 m courses: each course carries two of the texture's 20 cm rows, so a roof takes
    half the courses kit.shingles lays (one per row), and the butt of every course stands proud of the one below.
    Pieces run along the eave; a few sit crooked, a few have shakes missing."""
    top = slope.length + 0.02 if top is None else top
    y, k = start, 0
    while y < top - 0.06:
        h = min(course, top - y)
        x = -0.05 + rng.uniform(-0.4, 0.0)
        spans = []
        while x < slope.width + 0.05 - 1e-3:
            w = rng.uniform(*piece)
            if slope.width + 0.05 - (x + w) < 0.8:
                w = slope.width + 0.05 - x
            x_lo = max(x, -0.05)
            if k >= 2 and w > 1.2 and rng.random() < missing * 3.0:
                gap = rng.uniform(0.2, 0.45)
                at = rng.uniform(x_lo + 0.3, x + w - 0.3 - gap)
                spans += [(x_lo, at), (at + gap, x + w)]
            else:
                spans.append((x_lo, x + w))
            x += w
        for i, (a, b) in enumerate(spans):
            # Ends that butt against the next piece are hidden; the rakes' and a gap's show.
            drop = ['-z', '+y']
            if i > 0 and abs(spans[i - 1][1] - a) < 1e-6:
                drop.append('-x')
            if i < len(spans) - 1 and abs(spans[i + 1][0] - b) < 1e-6:
                drop.append('+x')
            tilt = -math.degrees(math.atan2(butt, h))
            matrix = kit.place(((a + b) * 0.5, y + h * 0.5 + rng.uniform(-0.012, 0.012), butt * 0.5 + 0.004 * (k % 2)),
                               (tilt, rng.uniform(-0.4, 0.4), 0.0))
            m.box((b - a, h, butt), matrix=matrix, uv=Trim('E', lane=(k % 2) * 2), space=slope, ao_floor=kit.ROOF_AO,
                  drop=tuple(drop))
        y += course - 0.03
        k += 1


def timber(space, p0, p1, width, depth=0.1, proud=0.05, face=OUT, drop=('+y',), uv=None):
    """An oak timber from p0 to p1 on a wall face (y = 0 outside, y = THICK inside), standing proud of it toward
    face, its back sunk into the plaster. drop leaves out faces nobody sees ('+y' its back, '-x'/'+x' its ends)."""
    p0, p1 = Vector(p0), Vector(p1)
    length = (p1 - p0).length
    if length < 0.03:
        return
    matrix = kit.toward(p0, p1, face) @ Matrix.Translation((length * 0.5, -(proud - depth * 0.5), 0.0))
    m.emit(kit._box((length, depth, width), drop=drop), uv or Trim('C', lane='each', width=max(width, depth)), 'trim',
           matrix, space)


def frame(space, length, items, top=WALL, roof=None, inside=False, rails=(1.2,), sill=True, braces=(), span=None,
          corners=(True, True), wrap=(False, False), post_top=None, post_bottom=None):
    """The timber frame over one face of a wall: a sill along the foot, a plate at top, rails (clipped to the gable
    above top), posts on the 0.8 m grid that stop at openings (heavy at the corners and every 2.4 m from the wall's
    middle, slim between), jambs where an opening's side is off the grid, braces in the bays listed in braces (bay
    index, rising toward 'left' or 'right', from z, to z) and, on a gable, posts on up to the roof line (roof(x) gives
    it). span limits it to part of the wall; corners says which ends are outside corners (a corner post there, set in
    by half a post or wrapped past the end). Proud of the plaster: posts 5 cm, rails and lintels a little more, braces
    a little less, so crossings never share a face."""
    face = IN if inside else OUT
    y = THICK if inside else 0.0
    dp = -0.01 if inside else 0.0
    lo, hi = span if span is not None else (0.0, length)
    n = int(round(length / GRID))

    def put(a, b, width, proud, drop=('+y', '-x', '+x'), depth=0.1):
        timber(space, (a[0], y, a[1]), (b[0], y, b[1]), width, depth, proud + dp, face, drop)

    def runs(z, pad, a, b):
        if roof is not None and top is not None and z > top:
            inner = [x for x in (a + (b - a) * i / 400.0 for i in range(401)) if roof(x) - 0.14 > z]
            if not inner:
                return []
            a, b = min(inner), max(inner)
        spans = [(a, b)]
        for o in items:
            if o.z - pad < z < top_of(o) + pad:
                spans = [(s, min(e, o.x - 0.01)) for s, e in spans] + [(max(s, o.x + o.w + 0.01), e) for s, e in spans]
        return [(s, e) for s, e in spans if e - s > 0.12]

    down, up = ('+z', '-z') if inside else ('-z', '+z')   # a horizontal timber's bottom and top in its own frame
    if sill:
        for s, e in runs(0.1, 0.0, lo + 0.04, hi - 0.04):
            put((s, 0.1), (e, 0.1), 0.2, 0.06, drop=('+y', down))
    for z in rails:
        for s, e in runs(z, 0.06, lo + 0.04, hi - 0.04):
            put((s, z), (e, z), 0.15, 0.055)
    if top is not None:
        for s, e in runs(top - 0.1, 0.0, lo + 0.04, hi - 0.04):
            put((s, top - 0.1), (e, top - 0.1), 0.2, 0.06, drop=('+y', up))
    posts = {}
    for k in range(n + 1):
        x = k * GRID
        if lo - 1e-6 <= x <= hi + 1e-6:
            posts[round(x, 4)] = POST if (k == 0 or k == n or (n - 2 * k) % 6 == 0) else STUD
    for o in items:
        for x in (o.x - STUD * 0.5, o.x + o.w + STUD * 0.5):
            if lo < x < hi and not any(abs(x - p) < STUD * 0.75 for p in posts):
                posts[round(x, 4)] = STUD
    for x, width in sorted(posts.items()):
        x0, x1 = x - width * 0.5, x + width * 0.5
        end = abs(x) < 1e-4 or abs(x - length) < 1e-4
        if end:
            if inside or not corners[0 if x < length * 0.5 else 1]:
                continue
            if x < length * 0.5:
                x0, x1 = (-0.065, POST) if wrap[0] else (0.0, POST)
            else:
                x0, x1 = (length - POST, length + 0.065) if wrap[1] else (length - POST, length)
        xc, w = (x0 + x1) * 0.5, x1 - x0
        base = post_bottom(xc) if post_bottom else (0.2 if sill else 0.0)
        if post_top:
            z_top = post_top(xc)
        elif top is not None:
            z_top = top - 0.2
        else:
            z_top = roof(xc) - 0.12
        z_runs = [(base, z_top)]
        for o in items:
            if o.x - w * 0.5 + 0.005 < xc < o.x + o.w + w * 0.5 - 0.005:
                g0 = o.z - (0.07 if o.z > 0.05 else 1.0)
                g1 = top_of(o) + 0.13 + o.opts.get('blind', 0.0)
                z_runs = [(a, min(b, g0)) for a, b in z_runs] + [(max(a, g1), b) for a, b in z_runs]
        for a, b in z_runs:
            if b - a > 0.08:
                put((xc, a), (xc, b), w, 0.05 + (0.012 if end else 0.0), depth=0.1 + (0.024 if end else 0.0))
        if roof is not None and top is not None and roof(xc) - top > 0.35:
            z_runs = [(top, roof(xc) - 0.1)]
            for o in items:
                if o.x - w * 0.5 < xc < o.x + o.w + w * 0.5:
                    z_runs = [(a, min(b, o.z - 0.07)) for a, b in z_runs] + [(max(a, top_of(o) + 0.13), b)
                                                                             for a, b in z_runs]
            for a, b in z_runs:
                if b - a > 0.08:
                    put((xc, a), (xc, b), w, 0.05)
    for bay, rising, z0, z1 in braces:
        a, b = bay * GRID + STUD * 0.5, (bay + 1) * GRID - STUD * 0.5
        if abs(bay * GRID) < 1e-4:
            a = POST
        if abs((bay + 1) * GRID - length) < 1e-4:
            b = length - POST
        if rising == 'left':
            put((b, z0), (a, z1), 0.14, 0.045)
        else:
            put((a, z0), (b, z1), 0.14, 0.045)


def raking_plate(space, x0, z0, x1, z1, inside=False):
    """A plate along a sloping wall top (a lean-to's walls)."""
    if inside:
        timber(space, (x0, THICK, z0 - 0.1), (x1, THICK, z1 - 0.1), 0.2, 0.1, 0.05, IN)
    else:
        timber(space, (x0, 0.0, z0 - 0.1), (x1, 0.0, z1 - 0.1), 0.2, 0.1, 0.06, OUT)


def head_timbers(space, o, inside=True, width=0.12, uv=None):
    """Timbers along a pointed head's two edges, just outside the hole, outside (and inside); uv maps them (oak by
    default)."""
    x0, x1, zs, za = o.x, o.x + o.w, o.z + o.h, top_of(o)
    xm = (x0 + x1) * 0.5
    faces = [(0.0, OUT, 0.058)] + ([(THICK, IN, 0.048)] if inside else [])
    for y, face, proud in faces:
        for a, b in (((x0, zs), (xm, za)), ((xm, za), (x1, zs))):
            d = Vector((b[0] - a[0], b[1] - a[1])).normalized()
            n = Vector((-d.y, d.x)) * (width * 0.5)
            ext = d * 0.07
            timber(space, (a[0] - ext.x + n.x, y, a[1] - ext.y + n.y), (b[0] + ext.x + n.x, y, b[1] + ext.y + n.y),
                   width, 0.1, proud, face, drop=('+y', '-x', '+x'), uv=uv)


def pane(space, points, y):
    """A pane of dark glass (strip H4) in the window's middle plane: a face each way, 2 mm apart, no edges."""
    n = len(points)
    if sum(points[i][0] * points[(i + 1) % n][1] - points[(i + 1) % n][0] * points[i][1] for i in range(n)) < 0.0:
        points = points[::-1]
    tb = kit._new_bmesh()
    front = [tb.verts.new((x, -0.001, z)) for x, z in points]
    back = [tb.verts.new((x, 0.001, z)) for x, z in points]
    tb.faces.new(front)
    tb.faces.new(back[::-1])
    m.emit(tb, Trim('H4', fit=True), 'trim', Matrix.Translation((0.0, y, 0.0)), space)


def lancet(space, o, glass=0.35, inside=True):
    """A tall pointed window between two studs: a sill, the pointed head's timbers, a lattice of oak bars and what is
    left of its glass (a few whole panes, a few shards in the corners: the rest lies smashed on the floor)."""
    x0, x1, z0, h, head = o.x, o.x + o.w, o.z, o.h, o.opts.get('head', 0.0)
    xm, zs = (x0 + x1) * 0.5, z0 + h
    m.box((o.w + 0.24, 0.13 + THICK * 0.5, 0.06), at=(xm, -0.065 + THICK * 0.25, z0 - 0.03),
          rot=(rng.uniform(-1.5, 1.5), 0.0, 0.0), uv='C', space=space)
    if head:
        head_timbers(space, o, inside)
    else:
        for y, face, proud in [(0.0, OUT, 0.058)] + ([(THICK, IN, 0.048)] if inside else []):
            timber(space, (x0 - 0.08, y, zs + 0.07), (x1 + 0.08, y, zs + 0.07), 0.14, 0.1, proud, face)
    yg = THICK * 0.5
    rows = o.opts.get('rows', 3)
    bar(space, (xm, yg, z0), (xm, yg, z0 + h + head - 0.03))
    for k in range(1, rows + (1 if head else 0)):
        bar(space, (x0 - 0.01, yg, z0 + h * k / rows), (x1 + 0.01, yg, z0 + h * k / rows))
    cells = []
    for c in range(2):
        for r in range(rows):
            a, b = x0 + c * o.w * 0.5, x0 + (c + 1) * o.w * 0.5
            cells.append([(a, z0 + h * r / rows), (b, z0 + h * r / rows), (b, z0 + h * (r + 1) / rows),
                          (a, z0 + h * (r + 1) / rows)])
    if head:
        cells += [[(x0, zs), (xm, zs), (xm, zs + head)], [(xm, zs), (x1, zs), (xm, zs + head)]]
    for cell in cells:
        roll = rng.random()
        if roll < glass:
            pane(space, cell, yg)
        elif roll < glass + 0.32:
            k = rng.randrange(len(cell))
            a, b, c = cell[k], cell[(k + 1) % len(cell)], cell[k - 1]
            t, s = rng.uniform(0.4, 0.85), rng.uniform(0.35, 0.8)
            pane(space, [a, (a[0] + (b[0] - a[0]) * t, a[1] + (b[1] - a[1]) * t),
                         (a[0] + (c[0] - a[0]) * s, a[1] + (c[1] - a[1]) * s)], yg)


def oculus(space, o, glass=0.4, inside=True):
    """A round window: an oak ring of eight short timbers and a cross of bars, part of its glass left."""
    r = o.w * 0.5
    cx, cz = o.x + r, o.z + r
    for y, face, proud in [(0.0, OUT, 0.058)] + ([(THICK, IN, 0.048)] if inside else []):
        for k in range(6):
            a0, a1 = 2.0 * math.pi * (k - 0.12) / 6, 2.0 * math.pi * (k + 1.12) / 6
            rr = r + 0.06
            timber(space, (cx + rr * math.cos(a0), y, cz + rr * math.sin(a0)),
                   (cx + rr * math.cos(a1), y, cz + rr * math.sin(a1)), 0.12, 0.1, proud, face)
    yg = THICK * 0.5
    bar(space, (cx, yg, cz - r), (cx, yg, cz + r))
    bar(space, (cx - r, yg, cz), (cx + r, yg, cz))
    for q in range(4):
        if rng.random() < glass:
            a0 = q * math.pi * 0.5
            pane(space, [(cx, cz)] + [(cx + r * math.cos(a0 + t * math.pi / 6.0), cz + r * math.sin(a0 + t * math.pi / 6.0))
                                      for t in range(4)], yg)


def plank_door(space, o, paint='A', hinge_left=True, lintel=True):
    """A closed plank door set a little into its wall (o in the wall's space): boards, battens and a Z brace on the
    outside, long iron strap hinges, a latch and a ring pull. The wall's own studs are its casing; lintel adds the oak
    lintel over it (the clapboard chapel's doors have painted casings instead)."""
    x0, w, h = o.x, o.w, o.h
    y = THICK * 0.3
    boards = max(3, int(round(w / 0.2)))
    bw = w / boards
    for k in range(boards):
        x = x0 + bw * (k + 0.5)
        m.board((x, y, 0.0), (x, y, h - rng.uniform(0.0, 0.03)), bw - 0.006, 0.05, uv=paint, space=space)
    for z in (0.3, h - 0.35):
        m.board((x0 + 0.06, y - 0.04, z), (x0 + w - 0.06, y - 0.04, z), 0.16, 0.035, uv=paint, space=space)
    low, high = (x0 + 0.1, x0 + w - 0.1) if hinge_left else (x0 + w - 0.1, x0 + 0.1)
    m.board((high, y - 0.04, 0.4), (low, y - 0.04, h - 0.45), 0.13, 0.03, uv=paint, space=space)
    hx = x0 if hinge_left else x0 + w
    for z in (0.3, h - 0.35):
        end = hx + w * 0.72 if hinge_left else hx - w * 0.72
        m.board((hx - (0.06 if hinge_left else -0.06), y - 0.065, z), (end, y - 0.065, z), 0.07, 0.014,
                uv=Trim('H3', fit=True), space=space)
    lx = x0 + w - 0.13 if hinge_left else x0 + 0.13
    m.box((0.05, 0.04, 0.16), at=(lx, y - 0.07, 1.0), uv=IRON_UV, mat='iron', space=space)
    m.box((0.12, 0.03, 0.03), at=(lx, y - 0.1, 1.05), uv=IRON_UV, mat='iron', space=space)
    if lintel:
        timber(space, (x0 - 0.12, 0.0, h + 0.07), (x0 + w + 0.12, 0.0, h + 0.07), 0.14, 0.1, 0.058, OUT)
    m.box((w + 0.04, THICK * 0.5 + 0.06, 0.05), at=(x0 + w * 0.5, THICK * 0.25 - 0.03, 0.02), uv='C', space=space)


def door_leaf(hinge, angle, width, height, mirror=False, paint='H2'):
    """One leaf of the front door, hinged at hinge (x, y, z) and swung angle degrees about it: boards in faded oxide
    red (strip H2), battens and a Z brace on its inner face, long iron strap hinges and a ring pull on its outer face.
    Built with the leaf along +x from its hinge, its outer face toward -y; mirror makes the other hand."""
    matrix = Matrix.Translation(hinge) @ Matrix.Rotation(math.radians(angle), 4, 'Z')
    if mirror:
        matrix = matrix @ Matrix.Diagonal((-1.0, 1.0, 1.0, 1.0))
    leaf = Space(matrix)
    boards = 3
    bw = width / boards
    for k in range(boards):
        x = bw * (k + 0.5)
        m.board((x, 0.025, 0.0), (x, 0.025, height - rng.uniform(0.0, 0.025)), bw - 0.008, 0.05,
                uv=Trim(paint, fit=True), space=leaf)   # a 24 cm board on the 20 cm strip
    for z in (0.34, height - 0.4):
        m.board((0.06, 0.068, z), (width - 0.05, 0.068, z), 0.16, 0.035, face=IN, uv=paint, space=leaf)
        m.board((-0.04, -0.008, z), (width * 0.74, -0.008, z), 0.075, 0.016, uv=Trim('H3', fit=True), space=leaf)
    m.board((width - 0.1, 0.07, 0.45), (0.1, 0.07, height - 0.5), 0.13, 0.03, face=IN, uv=paint, space=leaf)
    m.box((0.05, 0.03, 0.16), at=(width - 0.13, -0.018, 1.05), uv=IRON_UV, mat='iron', space=leaf)
    m.box((0.1, 0.02, 0.1), at=(width - 0.13, -0.04, 0.95), rot=(0.0, 45.0, 0.0), uv=IRON_UV, mat='iron', space=leaf)
    return matrix


def wall_hulls(space, length, items, height):
    """Collision for a wall: full-height boxes between its openings, boxes under and over each opening (one under
    and one over the whole row when its windows share their sill and head)."""
    if items and all(o.kind != 'door' for o in items) and len({(o.z, top_of(o)) for o in items}) == 1:
        z0, z1 = items[0].z, top_of(items[0])
        m.hull((length, THICK, z0), at=(length * 0.5, THICK * 0.5, z0 * 0.5), space=space)
        m.hull((length, THICK, height - z1), at=(length * 0.5, THICK * 0.5, (z1 + height) * 0.5), space=space)
        x = 0.0
        for o in sorted(items, key=lambda o: o.x) + [Opening('end', length, 0.0, 0.0, 0.0)]:
            if o.x - x > 0.05:
                m.hull((o.x - x, THICK, z1 - z0), at=((x + o.x) * 0.5, THICK * 0.5, (z0 + z1) * 0.5), space=space)
            x = o.x + o.w
        return
    x = 0.0
    for o in sorted(items, key=lambda o: o.x):
        if o.x - x > 0.05:
            m.hull((o.x - x, THICK, height), at=((x + o.x) * 0.5, THICK * 0.5, height * 0.5), space=space)
        if o.z > 0.05:
            m.hull((o.w, THICK, o.z), at=(o.x + o.w * 0.5, THICK * 0.5, o.z * 0.5), space=space)
        t = top_of(o)
        if height - t > 0.05:
            m.hull((o.w, THICK, height - t), at=(o.x + o.w * 0.5, THICK * 0.5, (t + height) * 0.5), space=space)
        x = o.x + o.w
    if length - x > 0.05:
        m.hull((length - x, THICK, height), at=((x + length) * 0.5, THICK * 0.5, height * 0.5), space=space)


def bay(k, z, h, head, **opts):
    """A window filling bay k of the grid (between two studs)."""
    return Opening('window', k * GRID + STUD * 0.5, z, GRID - STUD, h, head=head, **opts)


# --- The footing: fieldstone under the nave, the tower, the apse and the vestry. ---
footing(X0, X1, Y0, Y1, BASE)
footing(-TX, TX, TY0, TY1, BASE + 0.005)
footing(-AX, AX, Y1, AY, BASE + 0.007)
footing(X1, VX, VY, Y1, BASE + 0.006)
# Steps up to the tower door, and one in front of the vestry door.
STEPS = ((0.2, TY0 - 0.08 - 0.72, TY0 - 0.08 - 0.36, 2.5), (BASE - 0.005, TY0 - 0.08 - 0.36, TY0 - 0.08, 2.2))
for top, y0, y1, width in STEPS:
    m.box((width, y1 - y0, top + 0.1), at=(0.0, (y0 + y1) * 0.5, (top - 0.1) * 0.5), uv=Trim('D', world=True),
          cuts=1, drop=('-z',))
m.box((0.3, 1.5, 0.3), at=(VX + 0.08 + 0.15, 4.0, 0.05), uv=Trim('D', world=True), drop=('-z',))
m.section('footing')

# --- The nave: four walls, counterclockwise from above: front (the gable behind the tower), right, back, left. ---
roof_line = lambda x: WALL + (3.2 - abs(x - 3.2)) * math.tan(THETA)   # a gable wall's roof line (x along it)

INNER_DOOR = Opening('door', 3 * GRID + STUD * 0.5, 0.0, 2 * GRID - STUD, 2.5, head=0.5)
front_items = [INNER_DOOR]
space, length = wall((X0, Y0), (X1, Y0), front_items, top=[(6.4, WALL), (3.2, WALL + RISE), (0.0, WALL)],
                     cuts=(0.3, 3.2), cuts_x=(1.6, 4.8),
                     drop=lambda c, n: n.y < -0.9 and 1.6 < c.x < 4.8 and c.z > 3.25)   # inside the tower's shaft
front_space = space
# Outside, the timbers show only on the shoulders either side of the tower (and inside the vestibule, up to its
# ceiling); inside the nave, the whole gable.
frame(space, length, [], roof=roof_line, span=(0.0, 1.64), wrap=(True, False), corners=(True, False),
      braces=[(0, 'left', 1.25, WALL - 0.2), (1, 'right', 1.25, WALL - 0.2)])
frame(space, length, [], roof=roof_line, span=(4.76, 6.4), wrap=(False, True), corners=(False, True),
      braces=[(7, 'right', 1.25, WALL - 0.2), (6, 'left', 1.25, WALL - 0.2)])
frame(space, length, [INNER_DOOR], top=None, post_top=lambda x: 3.2, span=(1.62, 4.78), rails=(),
      corners=(False, False))
frame(space, length, front_items, roof=roof_line, inside=True, sill=False, rails=())
head_timbers(space, INNER_DOOR, width=0.14)
m.box((INNER_DOOR.w + 0.04, THICK + 0.06, 0.05), at=(INNER_DOOR.x + INNER_DOOR.w * 0.5, THICK * 0.5, 0.02), uv='C',
      space=space)

# The long walls: four windows down the left, three down the right and the door into the vestry by the sanctuary.
LEFT_BAYS, RIGHT_BAYS = (1, 4, 7, 10), (1, 4, 7)
WIN = dict(z=1.35, h=2.05, head=0.4, rows=2)
VESTRY_DOOR = Opening('window', 7.57, SANCT, 0.86, 2.0)   # a hole over the sanctuary floor (x along the right wall)
right_items = [bay(k, **WIN) for k in RIGHT_BAYS]
space, length = wall((X1, Y0), (X1, Y1), right_items + [VESTRY_DOOR])
right_space = space
frame(space, length, right_items, braces=[(0, 'left', 1.3, WALL - 0.2)],
      post_bottom=lambda x: 0.2 if x < 6.45 else 3.45)   # behind the vestry only the part over its roof shows
frame(space, length, right_items + [VESTRY_DOOR], inside=True, sill=False, rails=())
for o in right_items:
    lancet(space, o)
left_items = [bay(k, **WIN) for k in LEFT_BAYS]
space, length = wall((X0, Y1), (X0, Y0), left_items)
left_space = space
frame(space, length, left_items, braces=[(11, 'right', 1.3, WALL - 0.2)])
frame(space, length, left_items, inside=True, sill=False, rails=())
for o in left_items:
    lancet(space, o)

# The back gable: the apse's pointed arch over the sanctuary floor, a round window high up.
APSE_ARCH = Opening('window', 2 * GRID + STUD * 0.5, SANCT, 2 * GRID - STUD, 2.45, head=0.6)
ROSE = Opening('window', 3.2 - 0.48, 5.55, 0.96, 0.96, round=True)
back_items = [APSE_ARCH, ROSE]
space, length = wall((X1, Y1), (X0, Y1), back_items, top=[(6.4, WALL), (3.2, WALL + RISE), (0.0, WALL)])
back_space = space
frame(space, length, back_items, roof=roof_line, wrap=(True, True), rails=(1.2, WALL + 1.25),
      braces=[(0, 'left', 1.25, WALL - 0.2), (7, 'right', 1.25, WALL - 0.2)])
frame(space, length, back_items, roof=roof_line, inside=True, sill=False, rails=())
head_timbers(space, APSE_ARCH, width=0.14)
oculus(space, ROSE)
m.section('nave walls')

# --- The tower: a plastered shaft in three stages, the door at its foot. ---
DOOR = Opening('door', GRID + STUD * 0.5, 0.0, 2 * GRID - STUD, 2.4)
SHAFT_H = SHAFT - BASE
t_front = [DOOR, Opening('window', 1.6 - 0.31, 4.5, 0.62, 1.35, head=0.34, rows=2),
           Opening('window', 1.6 - 0.34, 6.95, 0.68, 0.68, round=True)]
t_side = [Opening('window', 1.6 - 0.31, 4.5, 0.62, 1.35, head=0.34, rows=2)]
STAGES = (3.3, 6.45)
TOWER_CUTS = (0.3, 3.2, SHAFT_H - 0.6)
space, length = wall((-TX, TY0), (TX, TY0), t_front, height=SHAFT_H, cuts=TOWER_CUTS, drop=ABOVE_CEILING)
tower_front = space
frame(space, length, t_front, top=SHAFT_H, rails=STAGES, wrap=(True, True),
      braces=[(0, 'left', 0.3, STAGES[0] - 0.1), (3, 'right', 0.3, STAGES[0] - 0.1),
              (0, 'right', STAGES[1] + 0.1, SHAFT_H - 0.2), (3, 'left', STAGES[1] + 0.1, SHAFT_H - 0.2)])
frame(space, length, [DOOR], top=3.32, inside=True, sill=False, rails=())
lancet(space, t_front[1], glass=1.0, inside=False)
oculus(space, t_front[2], glass=1.0, inside=False)
# The door: a lintel inside and out, iron pintles, and the leaves standing open into the vestibule.
dx0, dx1 = DOOR.x, DOOR.x + DOOR.w
for y, face, proud in ((0.0, OUT, 0.062), (THICK, IN, 0.05)):
    timber(space, (dx0 - 0.16, y, DOOR.h + 0.1), (dx1 + 0.16, y, DOOR.h + 0.1), 0.2, 0.12, proud, face)
m.box((DOOR.w + 0.06, THICK + 0.1, 0.05), at=((dx0 + dx1) * 0.5, THICK * 0.5 - 0.02, 0.025), uv='C', space=space)
LEAF_W, LEAF_H = DOOR.w * 0.5 - 0.01, DOOR.h - 0.04
LEAF_ANGLES = (101.0, -97.0)
leaf_l = door_leaf((-TX + dx0, TY0 + THICK * 0.6, FLOOR), LEAF_ANGLES[0], LEAF_W, LEAF_H)
leaf_r = door_leaf((-TX + dx1, TY0 + THICK * 0.6, FLOOR), LEAF_ANGLES[1], LEAF_W, LEAF_H, mirror=True)
# A little gabled hood over the door on two oak brackets.
mid = DOOR.x + DOOR.w * 0.5
for side in (-1.0, 1.0):
    x = mid + side * 0.92
    m.board((x, -0.02, 2.66), (x, -0.8, 2.66), 0.13, 0.1, face=(1.0, 0.0, 0.0), uv='C', space=space)
    m.board((x, -0.02, 2.06), (x, -0.5, 2.6), 0.11, 0.09, face=(1.0, 0.0, 0.0), uv='C', space=space)
hood_frame = space.matrix @ Matrix.Translation((mid, 0.0, 0.0)) @ Matrix.Rotation(math.radians(-90.0), 4, 'Z')
h_front, h_back = gable(0.0, 0.86, -0.98, 0.98, 2.8, 45.0, overhang=0.12, rake=0.12, thick=0.05,
                            frame=hood_frame, ends=(False, True))
shakes(h_front, piece=(3.0, 3.0))
shakes(h_back, piece=(3.0, 3.0))
ridge(h_front, h_back, width=0.1, thick=0.04)
m.socket('Crepe', (0.0, TY0 - 0.075, BASE + DOOR.h + 0.1))
m.socket('Perch_Hood', (0.0, TY0 - 0.9, BASE + 2.8 + 0.98 + 0.12))

space, length = wall((TX, TY0), (TX, TY1), t_side, height=SHAFT_H, cuts=TOWER_CUTS, drop=ABOVE_CEILING)
tower_right = space
frame(space, length, t_side, top=SHAFT_H, rails=STAGES,
      braces=[(0, 'left', 0.3, STAGES[0] - 0.1), (0, 'right', STAGES[1] + 0.1, SHAFT_H - 0.2),
              (3, 'left', STAGES[1] + 0.1, SHAFT_H - 0.2)])
frame(space, length, [], top=3.32, inside=True, sill=False, rails=())
lancet(space, t_side[0], glass=1.0, inside=False)
space, length = wall((-TX, TY1), (-TX, TY0), t_side, height=SHAFT_H, cuts=TOWER_CUTS, drop=ABOVE_CEILING)
tower_left = space
frame(space, length, t_side, top=SHAFT_H, rails=STAGES,
      braces=[(3, 'right', 0.3, STAGES[0] - 0.1), (3, 'left', STAGES[1] + 0.1, SHAFT_H - 0.2),
              (0, 'right', STAGES[1] + 0.1, SHAFT_H - 0.2)])
frame(space, length, [], top=3.32, inside=True, sill=False, rails=())
lancet(space, t_side[0], glass=1.0, inside=False)
# The back of the shaft, from the nave's roof up (x runs from the right corner to the left).
roof_at = lambda x: WALL + (TX - abs(TX - x)) * math.tan(THETA) + (3.2 - TX) * math.tan(THETA)
space, length = wall((TX, TY1), (-TX, TY1), [], height=SHAFT_H, cuts=(SHAFT_H - 0.6,),
                     bottom=[(0.0, roof_at(0.0)), (TX, roof_at(TX)), (2 * TX, roof_at(2 * TX))])
frame(space, length, [], top=SHAFT_H, rails=(8.65,), sill=False, post_bottom=lambda x: roof_at(x) + 0.05)
# Inside: the vestibule's plank ceiling on two joists.
m.box((3.0, TY1 - TY0, 0.12), at=(0.0, TYC + 0.1, BASE + 3.26), uv=PLANKS, mat='planks')
for y in (TY0 + 1.0, TY0 + 2.2):
    m.box((2.9, 0.14, 0.16), at=(0.0, y, BASE + 3.12), uv=BEAM())
m.section('tower shaft')

# --- The belfry: four oak posts on the shaft, boarded parapets, pointed braces under the plate, the bell frame. ---
m.box((2.96, 2.96, 0.1), at=(0.0, TYC, SHAFT - 0.05), uv=PLANKS, mat='planks')
for sx in (-1.0, 1.0):
    for sy in (-1.0, 1.0):
        x, y = sx * (TX - 0.12), TYC + sy * (TX - 0.12)
        m.board((x, y, SHAFT), (x, y, BELFRY - 0.2), 0.24, 0.24, uv=BEAM())
belfry_faces = [((-TX, TY0), (TX, TY0)), ((TX, TY0), (TX, TY1)), ((TX, TY1), (-TX, TY1)), ((-TX, TY1), (-TX, TY0))]
E = TX + 0.25          # the spire's eaves, out from the shaft
for i, (p0, p1) in enumerate(belfry_faces):
    sp = kit.wall_space(p0, p1, SHAFT)
    m.box((2 * TX - 0.48, 0.05, 0.72), at=(TX, 0.045, 0.36), uv=PLANKS, mat='planks', space=sp)
    m.box((2 * TX - 0.4, 0.16, 0.07), at=(TX, 0.07, 0.755), uv='C', space=sp)
    za, zt = AXIS_Z - SHAFT - 0.02, BELFRY - 0.2 - SHAFT - 0.03
    for a, b in (((0.2, za), (TX + 0.02, zt)), ((TX - 0.02, zt), (2 * TX - 0.2, za))):
        m.board((a[0], 0.12, a[1]), (b[0], 0.12, b[1]), 0.14, 0.12, uv='C', space=sp)
    inset = 0.0 if i % 2 == 0 else 0.24
    m.box((2 * TX - 2 * inset, 0.24, 0.2), at=(TX, 0.12, BELFRY - 0.1 - SHAFT), uv=BEAM(), space=sp)
    if i % 2 == 1:   # the side girts carry the bell beams
        m.box((2 * TX - 0.48, 0.18, 0.2), at=(TX, 0.12, 11.15 - SHAFT), uv=BEAM(), space=sp)
    soffit = 2 * E if i % 2 == 0 else 2 * TX
    m.box((soffit, E - TX, 0.04), at=(TX, -(E - TX) * 0.5, BELFRY - SHAFT - 0.02), uv='A', space=sp, drop=('+z',))
for s in (-1.0, 1.0):
    m.box((2 * TX - 0.08, 0.16, 0.2), at=(0.0, TYC + s * 0.62, 11.15), uv=BEAM())
    m.box((0.16, 0.14, 0.045), at=(0.0, TYC + s * 0.62, 11.25 + 0.0225), uv=IRON_UV, mat='iron')
m.socket('Bell', (0.0, TYC, AXIS_Z))
m.section('belfry')

# --- The spire: a pyramid of shakes in 0.4 m courses on a plank deck, oak boards on the hips, an iron cross. ---
H = TIP - BELFRY
L = math.hypot(E, H)
xl = lambda yy: E * yy / L - 0.03
xr = lambda yy: 2.0 * E - E * yy / L + 0.03
LAY = Matrix.Rotation(math.radians(-90.0), 4, 'X')    # a flat piece built in XZ, laid on a slope facing out
for k in range(4):
    frame_k = Matrix.Translation((0.0, TYC, BELFRY)) @ Matrix.Rotation(math.radians(90.0 * k), 4, 'Z')
    face = kit.Slope((-E, -E, 0.0), (1.0, 0.0, 0.0), (0.0, E / L, H / L), 2.0 * E, L, 0.0, frame=frame_k)
    m.emit(kit._prism([[(0.0, 0.0), (2.0 * E, 0.0), (E, L)]], 0.08), PLANKS, 'planks', LAY, face)
    course, step, butt = 0.4, 0.36, 0.035
    tilt = -math.degrees(math.atan2(butt, course))
    j, y0 = 0, -0.08
    while y0 < L - 0.22:
        y1 = min(y0 + course, L - 0.01)
        cy = (y0 + y1) * 0.5
        pts = [(xl(y0), y0 - cy), (xr(y0), y0 - cy), (xr(y1), y1 - cy), (xl(y1), y1 - cy)]
        matrix = (Matrix.Translation((0.0, cy, butt * 0.5 + 0.004 * (j % 2))) @ Matrix.Rotation(math.radians(tilt), 4, 'X')
                  @ Matrix.Translation((0.0, 0.0, butt * 0.5)) @ LAY)
        m.emit(slab(pts, butt), Trim('E', lane=(j % 2) * 2), 'trim', matrix, face, ao_floor=kit.ROOF_AO)
        y0 += step
        j += 1
    # The hip on this face's left: an oak board along it, lying across the courses.
    corner = frame_k @ Vector((-E, -E, 0.0))
    apex = frame_k @ Vector((0.0, 0.0, H))
    bisector = (frame_k.to_3x3() @ Vector((-H, -H, 2.0 * E))).normalized()
    m.board(corner + bisector * 0.07, apex + bisector * 0.03 - (apex - corner).normalized() * 0.05, 0.15, 0.05,
            face=tuple(bisector), uv='C')
m.cylinder((0.0, TYC, TIP - 0.25), (0.0, TYC, TIP + 0.32), 0.035, sides=6, uv=IRON_UV, mat='iron')
m.cylinder((0.0, TYC, TIP + 0.02), (0.0, TYC, TIP + 0.16), 0.075, sides=8, uv=IRON_UV, mat='iron')
m.box((0.05, 0.05, 0.66), at=(0.0, TYC, TIP + 0.62), uv=IRON_UV, mat='iron')
m.box((0.42, 0.05, 0.05), at=(0.0, TYC, TIP + 0.76), uv=IRON_UV, mat='iron')
m.section('spire')

# --- The nave's roof: shakes over a sagging deck, the ridge running front to back. ---
right_roof, left_roof = gable(Y0, Y1, -X1, -X0, EAVE, PITCH, overhang=0.4, rake=0.35, thick=0.12, sag=0.07,
                                  frame=Matrix.Rotation(math.radians(90.0), 4, 'Z'))
shakes(right_roof, piece=(3.4, 5.4))
shakes(left_roof, piece=(3.4, 5.4))
ridge(right_roof, left_roof)
m.section('nave roof')

# --- The apse: three short walls under a lean-to roof against the back gable. ---
APSE_WIN = bay(1, 1.55, 0.95, 0.28, rows=2)
apse_top = lambda y: 3.9 - (y - Y1) * 0.7 / (AY - Y1)   # the walls' top, from 3.9 at the nave to 3.2 at the back
space, length = wall((AX, Y1), (AX, AY), [], top=[(AY - Y1, apse_top(AY)), (0.0, apse_top(Y1))])
frame(space, length, [], top=None, roof=lambda x: apse_top(Y1 + x), corners=(False, True))
frame(space, length, [], top=None, roof=lambda x: apse_top(Y1 + x), inside=True, sill=False, rails=())
raking_plate(space, 0.0, apse_top(Y1), length, apse_top(AY))
space, length = wall((AX, AY), (-AX, AY), [APSE_WIN], height=3.2)
frame(space, length, [APSE_WIN], top=3.2, wrap=(True, True))
frame(space, length, [APSE_WIN], top=3.2, inside=True, sill=False, rails=())
lancet(space, APSE_WIN, glass=0.5)
space, length = wall((-AX, AY), (-AX, Y1), [], top=[(AY - Y1, apse_top(Y1)), (0.0, apse_top(AY))])
frame(space, length, [], top=None, roof=lambda x: apse_top(AY - x), corners=(True, False))
frame(space, length, [], top=None, roof=lambda x: apse_top(AY - x), inside=True, sill=False, rails=())
raking_plate(space, 0.0, apse_top(AY), length, apse_top(Y1))
a_theta = math.atan2(0.7, AY - Y1)
a_over, a_deck = 0.3, 0.08
a_origin = Vector((AX + 0.25, AY + a_over, BASE + 3.2 - a_over * math.tan(a_theta))) + \
    Vector((0.0, math.sin(a_theta), math.cos(a_theta))) * a_deck
apse_roof = kit.Slope(a_origin, (-1.0, 0.0, 0.0), (0.0, -math.cos(a_theta), math.sin(a_theta)), 2 * AX + 0.5,
                      (AY - Y1 + a_over) / math.cos(a_theta), sag=0.02)
deck(apse_roof, a_deck)
shakes(apse_roof, piece=(1.4, 2.4), top=apse_roof.length - 0.03)
m.box((apse_roof.width + 0.02, 0.18, 0.04), at=(apse_roof.width * 0.5, apse_roof.length - 0.08, 0.05),
      rot=(-6.0, 0.0, 0.0), uv='C', space=apse_roof, cuts=2)
m.section('apse')

# --- The vestry: a lean-to on the nave's right side by the sanctuary, its door facing out. ---
VDOOR = Opening('door', 1.17, 0.0, 0.86, 2.05)
VWIN = bay(1, 1.25, 0.85, 0.0, rows=2)
vestry_top = lambda x: 3.6 - (x - X1) / (VX - X1)        # the walls' top, from 3.6 at the nave to 2.6 outside
space, length = wall((X1, VY), (VX, VY), [], top=[(VX - X1, vestry_top(VX)), (0.0, vestry_top(X1))], drop=INSIDE_FACE)
frame(space, length, [], top=None, roof=lambda x: vestry_top(X1 + x), corners=(False, True), wrap=(False, True),
      braces=[(2, 'right', 1.25, 2.3)])
raking_plate(space, 0.0, vestry_top(X1), length, vestry_top(VX))
space, length = wall((VX, VY), (VX, Y1), [VDOOR], height=2.6, drop=INSIDE_FACE)
vestry_side = space
frame(space, length, [VDOOR], top=2.6, braces=[(0, 'left', 1.25, 2.4)])
plank_door(space, VDOOR, paint='A', hinge_left=False)
space, length = wall((VX, Y1), (X1, Y1), [VWIN], top=[(VX - X1, vestry_top(X1)), (0.0, vestry_top(VX))],
                     drop=INSIDE_FACE)
frame(space, length, [VWIN], top=None, roof=lambda x: vestry_top(VX - x), corners=(True, False), wrap=(True, False))
raking_plate(space, 0.0, vestry_top(VX), length, vestry_top(X1))
lancet(space, VWIN, glass=1.0, inside=False)
v_theta = math.atan2(1.0, VX - X1)
v_over, v_deck, v_rake = 0.35, 0.08, 0.3
v_origin = Vector((VX + v_over, VY - v_rake, BASE + 2.6 - v_over * math.tan(v_theta))) + \
    Vector((math.sin(v_theta), 0.0, math.cos(v_theta))) * v_deck
vestry_roof = kit.Slope(v_origin, (0.0, 1.0, 0.0), (-math.cos(v_theta), 0.0, math.sin(v_theta)), (Y1 - VY) + 2 * v_rake,
                        (VX - X1 + v_over) / math.cos(v_theta), sag=0.03)
deck(vestry_roof, v_deck)
shakes(vestry_roof, piece=(1.6, 2.6), top=vestry_roof.length - 0.03)
m.box((vestry_roof.width + 0.02, 0.18, 0.04), at=(vestry_roof.width * 0.5, vestry_roof.length - 0.08, 0.05),
      rot=(-6.0, 0.0, 0.0), uv='C', space=vestry_roof, cuts=2)
# The lantern by its door, on an iron bracket.
LANTERN = (VX + 0.34, 4.9, BASE + 1.98)
m.box((0.03, 0.12, 0.3), at=(VX + 0.015, LANTERN[1], LANTERN[2] + 0.32), uv=IRON_UV, mat='iron')
m.box((0.36, 0.03, 0.03), at=(VX + 0.18, LANTERN[1], LANTERN[2] + 0.43), uv=IRON_UV, mat='iron')
m.board((VX + 0.02, LANTERN[1], LANTERN[2] + 0.22), (VX + 0.26, LANTERN[1], LANTERN[2] + 0.42), 0.025, 0.025,
        face=(0.0, 1.0, 0.0), uv=IRON_UV, mat='iron')
lx, ly, lz = LANTERN
m.box((0.2, 0.2, 0.04), at=(lx, ly, lz + 0.15), uv=IRON_UV, mat='iron')
m.cylinder((lx, ly, lz + 0.17), (lx, ly, lz + 0.27), 0.11, 0.025, sides=6, uv=IRON_UV, mat='iron', caps=(False, False))
m.box((0.15, 0.15, 0.24), at=(lx, ly, lz), uv=Trim('H4', fit=True), mat='glow')
m.box((0.2, 0.2, 0.04), at=(lx, ly, lz - 0.14), uv=IRON_UV, mat='iron')
for dx in (-0.085, 0.085):
    for dy in (-0.085, 0.085):
        m.box((0.022, 0.022, 0.26), at=(lx + dx, ly + dy, lz), uv=IRON_UV, mat='iron', drop=('-z', '+z'))
m.box((0.02, 0.02, 0.12), at=(lx, ly, lz + 0.36), uv=IRON_UV, mat='iron', drop=('-z',))
m.socket('Light', LANTERN)
m.socket('Speaker', (VX + 0.09, VY + VDOOR.x + VDOOR.w * 0.5, BASE + 1.6), (0.0, 0.0, 90.0))
m.socket('Perch_Lantern', (VX + 0.3, LANTERN[1], LANTERN[2] + 0.46))
m.section('vestry')

# --- Inside: plank floors, the sanctuary, the apse's plinth, candle stands, a lectern, pews, trusses, the rope. ---
ALTAR_Y = 6.33
ALTAR_TOP = FLOOR + SANCT + 0.42
LX, LY, LZ = -2.15, 4.35, FLOOR + SANCT
PEW_ROWS = (-1.3, -0.05, 1.2)
PEW_LEN = 2.2
RX, RY = 0.95, TYC + 0.75


def pew(cx, cy, length, turn=0.0):
    """A plain pew facing the altar (+y): a seat, a leaning back board and shaped ends."""
    sp = Space(kit.place((cx, cy, FLOOR), (0.0, 0.0, turn)))
    m.box((length, 0.4, 0.045), at=(0.0, 0.02, 0.44), uv=PLANKS, mat='planks', space=sp)
    m.box((length, 0.03, 0.34), at=(0.0, -0.22, 0.74), rot=(9.0, 0.0, 0.0), uv=PLANKS, mat='planks', space=sp)
    end = [(-0.27, 0.0), (0.24, 0.0), (0.24, 0.5), (-0.33, 0.95)]
    for sx in (-1.0, 1.0):
        place = kit.place((sx * length * 0.5 + 0.025, 0.0, 0.0), (0.0, 0.0, 90.0))
        m.emit(kit._prism([end], 0.05), PLANKS, 'planks', place, sp)
    return sp


def furnish(right_roof, left_roof):
    """What both chapels have inside, in the model being built: the plank floors, the sanctuary two steps up, the
    apse's floor and the Reliquary's stone plinth, candle stands, a lectern, the pews, three trusses under the nave's
    roof slopes, the bell rope in the vestibule and the vestry's closed inner door."""
    m.box((6.2, Y1 - Y0 - 0.08, 0.13), at=(0.0, (Y0 + Y1) * 0.5, FLOOR - 0.065), uv=PLANKS, mat='planks')
    m.box((3.0, TY1 - TY0, 0.13), at=(0.0, TYC + 0.08, FLOOR - 0.065), uv=PLANKS, mat='planks')
    m.box((6.2, SANCT_Y - 3.2, STEP), at=(0.0, (SANCT_Y + 3.2) * 0.5, FLOOR + STEP * 0.5), uv=PLANKS, mat='planks')
    m.box((6.2, Y1 - 0.1 - SANCT_Y, SANCT), at=(0.0, (SANCT_Y + Y1 - 0.1) * 0.5, FLOOR + SANCT * 0.5), uv=PLANKS,
          mat='planks')
    m.box((2 * AX - 0.08, AY - Y1 + 0.06, SANCT + 0.1), at=(0.0, (Y1 - 0.1 + AY - 0.04) * 0.5,
          FLOOR + SANCT * 0.5 - 0.05), uv=PLANKS, mat='planks')
    m.box((APSE_ARCH.w + 0.04, THICK + 0.05, 0.03), at=(0.0, Y1 - THICK * 0.5, FLOOR + SANCT + 0.012), uv='C')
    m.box((1.36, 0.86, 0.33), at=(0.0, ALTAR_Y, FLOOR + SANCT + 0.165), uv=Trim('D', world=True, v=0.3), drop=('-z',))
    m.box((1.56, 1.02, 0.1), at=(0.0, ALTAR_Y, ALTAR_TOP - 0.05), uv=Trim('D', world=True, v=0.3), bevel=0.012)
    m.socket('Reliquary', (0.0, ALTAR_Y, ALTAR_TOP))
    for sx, burnt in ((-1.0, 1.0), (1.0, 0.55)):
        x, y, z0 = sx * 1.12, 5.1, FLOOR + SANCT
        m.cylinder((x, y, z0), (x, y, z0 + 0.05), 0.15, 0.1, sides=6, uv=IRON_UV, mat='iron', caps=(False, True))
        m.cylinder((x, y, z0 + 0.05), (x, y, z0 + 1.3), 0.024, sides=6, uv=IRON_UV, mat='iron', caps=(False, False))
        m.cylinder((x, y, z0 + 1.3), (x, y, z0 + 1.34), 0.11, sides=6, uv=IRON_UV, mat='iron')
        m.cylinder((x, y, z0 + 1.34), (x, y, z0 + 1.34 + 0.2 * burnt), 0.036, sides=6, uv=Trim('G'), caps=(False, True))
    m.box((0.38, 0.38, 0.06), at=(LX, LY, LZ + 0.03), uv=PLANKS, mat='planks', drop=('-z',))
    m.box((0.1, 0.1, 1.0), at=(LX, LY, LZ + 0.55), uv=PLANKS, mat='planks')
    m.box((0.52, 0.42, 0.035), at=(LX, LY, LZ + 1.1), rot=(-24.0, 0.0, 0.0), uv=PLANKS, mat='planks')
    m.box((0.52, 0.03, 0.05), at=(LX, LY - 0.2, LZ + 1.04), rot=(-24.0, 0.0, 0.0), uv=PLANKS, mat='planks')
    pews = []
    for k, py in enumerate(PEW_ROWS):
        for side in (-1.0, 1.0):
            cx = side * (0.65 + PEW_LEN * 0.5)
            if k == 1 and side < 0:   # knocked askew
                pews.append((cx + 0.12, py - 0.18, 9.0))
            else:
                pews.append((cx, py, rng.uniform(-0.8, 0.8)))
    for cx, cy, turn in pews:
        pew(cx, cy, PEW_LEN, turn)
    # Trusses: a tie beam, a king post and two principal rafters under the deck.
    for ty in (-1.6, 0.8, 3.2):
        m.box((6.3, 0.18, 0.22), at=(0.0, ty, EAVE - 0.13), uv=BEAM(), drop=('-x', '+x'))
        ridge_under = EAVE + RISE - 0.04
        m.board((0.0, ty, EAVE - 0.01), (0.0, ty, ridge_under + 0.01), 0.18, 0.16, face=(0.0, -1.0, 0.0), uv=BEAM())
        for slope, xs in ((right_roof, ty - (Y0 - 0.35)), (left_roof, (Y1 + 0.35) - ty)):
            y_lo, y_hi = 0.62 / math.cos(THETA), (3.6 - 0.09) / math.cos(THETA)
            m.box((0.16, y_hi - y_lo, 0.2), at=(xs, (y_lo + y_hi) * 0.5, -0.12 - 0.1), uv=BEAM(), space=slope,
                  drop=('+z',))
    # The bell rope: down through the vestibule ceiling, a red woollen grip, the tail tied off at a cleat.
    m.cylinder((RX, RY, BASE + 3.22), (RX, RY, FLOOR + 1.0), 0.017, sides=5, uv='C', caps=(False, True))
    m.cylinder((RX, RY, FLOOR + 1.2), (RX, RY, FLOOR + 1.62), 0.04, sides=6, uv='H2')
    m.cylinder((RX, RY, FLOOR + 1.0), (TX - THICK - 0.04, RY, FLOOR + 1.42), 0.017, sides=5, uv='C')
    m.box((0.05, 0.2, 0.04), at=(TX - THICK - 0.025, RY, FLOOR + 1.42), uv=IRON_UV, mat='iron')
    m.socket('Interact', (RX, RY, FLOOR + 1.41))
    # The vestry's inner door, closed, at the sanctuary's level (built facing the nave).
    inner = kit.wall_space((X1 - THICK, Y1), (X1 - THICK, Y0), BASE + SANCT)
    plank_door(inner, Opening('door', Y1 - (VESTRY_DOOR.x + VESTRY_DOOR.w + Y0), 0.0, VESTRY_DOOR.w, VESTRY_DOOR.h),
               paint='A', hinge_left=True)
    m.section('inside')


furnish(right_roof, left_roof)

# --- Collision ---
m.hull((X1 - X0 + 0.16, Y1 - Y0 + 0.16, FLOOR), at=(0.0, (Y0 + Y1) * 0.5, FLOOR * 0.5))
m.hull((2 * TX + 0.16, TY1 - TY0 + 0.08, FLOOR), at=(0.0, TYC - 0.04, FLOOR * 0.5))
for top, y0, y1, width in STEPS:
    m.hull((width, y1 - y0, top), at=(0.0, (y0 + y1) * 0.5, top * 0.5))
m.hull((0.3, 1.5, 0.2), at=(VX + 0.23, 4.0, 0.1))
m.hull((6.08, SANCT_Y - 3.2, STEP), at=(0.0, (SANCT_Y + 3.2) * 0.5, FLOOR + STEP * 0.5))
m.hull((6.08, Y1 - SANCT_Y, SANCT), at=(0.0, (SANCT_Y + Y1) * 0.5, FLOOR + SANCT * 0.5))
m.hull((2 * AX + 0.16, AY - Y1 + 0.08, FLOOR + SANCT), at=(0.0, (Y1 + AY) * 0.5 + 0.04, (FLOOR + SANCT) * 0.5))
m.hull((1.56, 1.02, 0.42), at=(0.0, ALTAR_Y, ALTAR_TOP - 0.21))
wall_hulls(front_space, 6.4, front_items, WALL)
wall_hulls(right_space, 9.6, [], WALL)   # solid: the sills are 1.35 m up, nobody passes a window
wall_hulls(left_space, 9.6, [], WALL)
wall_hulls(back_space, 6.4, [APSE_ARCH], WALL)
for y, sign in ((Y0, 1.0), (Y1, -1.0)):
    m.hull_points([(x, y + sign * t, z) for t in (0.0, THICK) for x, z in ((X0, EAVE), (X1, EAVE), (0.0, EAVE + RISE))])
wall_hulls(tower_front, 2 * TX, [DOOR], 3.32)
for sp in (tower_right, tower_left):
    m.hull((TY1 - TY0, THICK, 3.32), at=((TY1 - TY0) * 0.5, THICK * 0.5, 1.66), space=sp)
m.hull((2 * TX, TY1 - TY0, SHAFT - BASE - 3.2), at=(0.0, TYC, (BASE + 3.2 + SHAFT) * 0.5))
for leaf in (leaf_l, leaf_r):
    m.hull((LEAF_W, 0.06, LEAF_H), matrix=leaf @ Matrix.Translation((LEAF_W * 0.5, 0.03, LEAF_H * 0.5)))
for p0, p1 in belfry_faces:   # the parapets (the open belfry above them is out of anyone's reach)
    sp = kit.wall_space(p0, p1, SHAFT)
    m.hull((2 * TX - 0.48, 0.1, 0.79), at=(TX, 0.06, 0.395), space=sp)
m.hull((2 * TX, 2 * TX, 0.2), at=(0.0, TYC, BELFRY - 0.1))
m.hull_points([(sx * E, TYC + sy * E, BELFRY) for sx in (-1.0, 1.0) for sy in (-1.0, 1.0)] + [(0.0, TYC, TIP)])
for slope in (right_roof, left_roof):
    m.hull((slope.width, slope.length, 0.24), at=(slope.width * 0.5, slope.length * 0.5, -0.06), space=slope)
m.hull((apse_roof.width, apse_roof.length, 0.2), at=(apse_roof.width * 0.5, apse_roof.length * 0.5, -0.04),
       space=apse_roof)
for (p0, p1), height in ((((AX, Y1), (AX, AY)), 3.9), (((AX, AY), (-AX, AY)), 3.2), (((-AX, AY), (-AX, Y1)), 3.9)):
    sp = kit.wall_space(p0, p1, BASE)
    length = (Vector(p1) - Vector(p0)).length
    m.hull((length, THICK, height), at=(length * 0.5, THICK * 0.5, height * 0.5), space=sp)
m.hull_points([(x, y, z) for x in (X1, VX + 0.08) for y in (VY - 0.08, Y1 + 0.08) for z in (0.0, BASE + 2.7)] +
              [(X1, y, BASE + 3.75) for y in (VY - 0.08, Y1 + 0.08)])   # the closed vestry, to its roof
# The pews, a block either side of the aisle (the askew one included).
for side in (-1.0, 1.0):
    xs = [side * 0.55, side * (0.65 + PEW_LEN + 0.12)]
    m.hull((abs(xs[1] - xs[0]), PEW_ROWS[-1] - PEW_ROWS[0] + 0.85, 0.95),
           at=((xs[0] + xs[1]) * 0.5, (PEW_ROWS[0] + PEW_ROWS[-1]) * 0.5 - 0.1, FLOOR + 0.475))


# Vertex occlusion in two passes: the usual 1 m contact shading, and a 7 m pass without the ground plane that only the
# building's own walls and roof can darken, so the inside (lit by the sky through windows and doors) is dim while
# the outside keeps its light. Unreal applies it to the ambient light only, so the sun still falls bright inside.
_bake = lt.bake_vertex_ao


def _bake_two_pass(obj, samples=32, distance=1.0, ground=True, children=True, strength=1.0):
    _bake(obj, samples=samples, distance=distance, ground=ground, children=children, strength=strength)
    if obj.name not in ('Chapel', 'Chapel_A'):
        return
    near = lt._read_col(obj.data)[:, 3].copy()
    _bake(obj, samples=40, distance=7.0, ground=False, children=children)
    raw = lt._read_col(obj.data)
    raw[:, 3] = near * (0.3 + 0.7 * raw[:, 3])
    lt._write_col(obj.data, raw)


lt.bake_vertex_ao = _bake_two_pass
# The whole mesh (about 8.9k triangles) is the fallback Medium and Low draw: Unreal's own reduction would make tangents
# of its own on the simplified mesh.
chapel_a = m.finish(preview=False, fallback=100.0)   # None when it isn't built
lt.bake_vertex_ao = _bake

# --- The bell: a bronze bell (a lathe), its crown strapped to an iron yoke with gudgeons on the axis, a clapper. ---
class Lathe:
    """Cylindrical mapping round Z for a lathe (the bell) on a tileable set: U around it at radius, V up it, at the
    set's texel density, so the worn metal wraps without the seams a cube projection leaves on a round body. A flat
    cap (all its corners at one height) would get a single V and no UV area, so no tangents in Unreal: it is mapped
    flat from above instead."""

    def __init__(self, set_name, radius=0.3):
        self.set_name, self.radius = set_name, radius

    def apply(self, obj, rng):
        info = lt.SETS[self.set_name]
        scale = info['density'] / float(info['size'])
        mesh = obj.data
        uvs = mesh.uv_layers.active.data
        for poly in mesh.polygons:
            zs = [mesh.vertices[v].co.z for v in poly.vertices]
            if max(zs) - min(zs) < 1e-6:
                for index in poly.loop_indices:
                    co = mesh.vertices[mesh.loops[index].vertex_index].co
                    uvs[index].uv = (co.x * scale, co.y * scale)
                continue
            a_c = math.atan2(poly.center.y, poly.center.x)
            for index in poly.loop_indices:
                co = mesh.vertices[mesh.loops[index].vertex_index].co
                a = math.atan2(co.y, co.x) if co.x * co.x + co.y * co.y > 1e-10 else a_c
                a = a_c + (a - a_c + math.pi) % (2.0 * math.pi) - math.pi
                uvs[index].uv = (a * self.radius * scale, co.z * scale)


b = kit.Model('ChapelBell', seed=1889)
BRASS = lt.material('MetalWorn', name='BrassWorn', tint=0xc49c56)
b.slots['brass'] = (0, BRASS)
b.slots['iron'] = (1, IRON)
BELL = [(0.0, -0.1), (0.12, -0.1), (0.2, -0.12), (0.24, -0.17), (0.25, -0.24), (0.243, -0.27), (0.252, -0.4),
        (0.272, -0.55), (0.305, -0.67), (0.35, -0.755), (0.4, -0.82), (0.44, -0.865), (0.452, -0.895), (0.44, -0.915),
        (0.405, -0.91), (0.37, -0.86), (0.31, -0.77), (0.265, -0.66), (0.235, -0.52), (0.215, -0.36), (0.2, -0.2),
        (0.15, -0.15), (0.0, -0.15)]
SIDES = 16
tb = kit._new_bmesh()
rings = []
for r, z in BELL:
    if r < 1e-5:
        rings.append([tb.verts.new((0.0, 0.0, z))])
    else:
        rings.append([tb.verts.new((r * math.cos(2.0 * math.pi * k / SIDES), r * math.sin(2.0 * math.pi * k / SIDES), z))
                      for k in range(SIDES)])
for r0, r1 in zip(rings, rings[1:]):
    for k in range(SIDES):
        k1 = (k + 1) % SIDES
        if len(r0) == 1:
            tb.faces.new((r0[0], r1[k1], r1[k]))
        elif len(r1) == 1:
            tb.faces.new((r0[k], r0[k1], r1[0]))
        else:
            tb.faces.new((r0[k], r0[k1], r1[k1], r1[k]))
bmesh.ops.recalc_face_normals(tb, faces=tb.faces[:])
b.emit(tb, Lathe('MetalWorn'), 'brass', smooth=True)
b.box((0.1, 0.22, 0.05), at=(0.0, 0.0, -0.095), uv=Tile('MetalWorn'), mat='brass', bevel=0.01)
b.box((0.2, 1.08, 0.14), at=(0.0, 0.0, 0.0), uv=Tile('MetalWorn'), mat='iron', bevel=0.012)
for s in (-1.0, 1.0):
    b.cylinder((0.0, s * 0.54, 0.0), (0.0, s * 0.68, 0.0), 0.035, sides=8, uv=Tile('MetalWorn'), mat='iron',
               face=(0.0, 0.0, 1.0))
    b.box((0.24, 0.04, 0.2), at=(0.0, s * 0.09, -0.11), uv=Tile('MetalWorn'), mat='iron')
b.box((0.03, 0.03, 0.56), at=(0.0, 0.0, -0.42), uv=Tile('MetalWorn'), mat='iron')
b.cylinder((0.0, 0.0, -0.68), (0.0, 0.0, -0.8), 0.055, sides=8, uv=Tile('MetalWorn'), mat='iron')
bell = b.finish(ground=False, preview=False, Nanite=0, LODs='50,25', Collision='None')

# ======================================================================================================================
# The game's chapel (option B, picked by the user): the same chapel as a white clapboard frontier church. The same
# footprint, pivot, floors and furnishings, sockets, collision layout and material slots as option A above: painted lap
# siding (boards as geometry, 18 cm to the weather) with corner boards, casings, a frieze under the eaves and a water
# table at the foot, the paint grey and flaking low down and here and there a board sprung or bare; the same pointed
# windows, shake roofs and fieldstone footing. The tower stands centred on the front gable with the oxide-red doors
# open at its foot and rises to a louvred belfry round the bell, then a tall white octagonal spire with the iron cross,
# 17.9 m up. Inside, white boards on the walls instead of plaster and oak. Its apse arch is on the nave's axis.
# ======================================================================================================================
m = kit.Model('Chapel', seed=1890)
rng = m.rng
m.slot('trim')
m.slot('planks')
m.slots['iron'] = (len(m.slots), IRON)
WHITE = lc.Paint()
REVEAL, LAP_OUT = 0.18, 0.026   # the siding: its reveal, and how far a board's foot stands off the wall
CASE_W = 0.12                   # casings and corner boards
FRIEZE = 0.22                   # the frieze board under the long walls' eaves
WATER = 0.2                     # the water table at the foot of the walls; the siding starts on it
INNER = 0.33                    # the boards inside
B_TOP = 12.3                    # the top of the tower's walls, under the belfry's cornice
B_SPIRE = 12.45                 # the spire's foot, on the cornice
B_TIP = 17.0                    # its point; the cross stands 0.9 m over it
B_R = 1.2                       # the spire's octagon, centre to corner
B_WALL = B_TOP - BASE           # the tower walls' height
SHAFT_Z = SHAFT - BASE          # the belfry floor in the tower walls' frame


def coat(row, z):
    """The paint on a row of siding: grey and flaking near the foot, else clean (lap() picks out the odd bare or sprung
    board)."""
    if z < 0.62:
        return 'weathered'
    return 'clean'


def outline_of(length, items=(), height=WALL, top=None, bottom=None):
    """A wall's outline as wall() builds it: the foot notched for the doors, the top level or along top."""
    if bottom is not None:
        points = list(bottom)
    else:
        points = [(0.0, 0.0)]
        for o in sorted((o for o in items if o.kind == 'door'), key=lambda o: o.x):
            points += [(o.x, 0.0), (o.x, o.h)]
            if o.opts.get('head'):
                points.append((o.x + o.w * 0.5, o.h + o.opts['head']))
            points += [(o.x + o.w, o.h), (o.x + o.w, 0.0)]
        points.append((length, 0.0))
    return points + (list(top) if top is not None else [(length, height), (0.0, height)])


def trim(space, p0, p1, width, proud=0.045, face=OUT, depth=None):
    """A white trim board (casing, corner board, frieze) from p0 to p1 on a wall face, standing out `proud`."""
    timber(space, p0, p1, width, depth or proud, proud, face, drop=('+y', '-x', '+x'), uv=WHITE)


def side(p0, p1, items=(), height=WALL, top=None, bottom=None, z0=BASE, drop=None, cuts=(), cuts_x=(),
         corners=(True, True), blocks=(), lap_to=None, boards=None, board_blocks=(), frieze=False, water=True,
         casings=True, butt=WALL):
    """A wall of the clapboard chapel: wall()'s core in white (its faces mostly behind boards), the siding over its
    outside from the water table (or its foot) to lap_to (the top; under the frieze if frieze), clear of its openings'
    casings, its corner boards and blocks; corner boards at its ends (True: from the foot to the top there, a (z0, z1)
    range, or None at an inside corner); boards inside from boards[0] to boards[1] clear of board_blocks. Returns
    (space, length, outline)."""
    space, length = wall(p0, p1, items, height, top, bottom, z0, cuts=cuts, cuts_x=cuts_x, drop=drop, uv=WHITE)
    outline = outline_of(length, items, height, top, bottom)
    blk = [lc.opening_block(o, CASE_W - 0.02) for o in items if casings] + list(blocks)
    inner_x = [0.0, length]          # between the corner boards: where the water table and the frieze run
    for end, span in enumerate(corners):
        if not span:
            continue
        z_lo, z_hi = (0.0, height) if span is True else span
        if end == 0:
            blk.append(lc.box_block(-1.0, CASE_W - 0.02, z_lo - 0.01, z_hi + 0.01))
            trim(space, (CASE_W * 0.5, 0.0, z_lo), (CASE_W * 0.5, 0.0, z_hi), CASE_W)
            inner_x[0] = CASE_W if z_lo < 0.1 else 0.0
        else:
            blk.append(lc.box_block(length - CASE_W + 0.02, length + 1.0, z_lo - 0.01, z_hi + 0.01))
            trim(space, (length - CASE_W * 0.5 + 0.0225, 0.0, z_lo), (length - CASE_W * 0.5 + 0.0225, 0.0, z_hi),
                 CASE_W + 0.045)
            inner_x[1] = length - CASE_W if z_lo < 0.1 else length
    z_hi = lap_to if lap_to is not None else (height - FRIEZE if frieze else max(z for x, z in outline))
    z_lo = WATER if water else min(z for x, z in outline)
    lc.lap(m, space, outline, z_lo, z_hi, blk, REVEAL, LAP_OUT, coat=coat, features=0.02, butt=butt)
    if water:
        for a, b in lc.spans_at(outline, WATER * 0.5):
            a, b = max(a, inner_x[0]), min(b, inner_x[1])
            if b - a > 0.05:
                trim(space, (a, 0.0, WATER * 0.5), (b, 0.0, WATER * 0.5), WATER, 0.04)
    if frieze:
        trim(space, (CASE_W, 0.0, height - FRIEZE * 0.5), (length - CASE_W, 0.0, height - FRIEZE * 0.5), FRIEZE, 0.04)
    if boards:
        lc.lap(m, space, outline, boards[0], boards[1], list(board_blocks), INNER, 0.03, inner=THICK, butt=False)
    return space, length, outline


def b_bar(space, p0, p1, size=0.04):
    """A white glazing bar, its ends left open where they meet the frame."""
    p0, p1 = Vector(p0), Vector(p1)
    length = (p1 - p0).length
    matrix = kit.toward(p0, p1, OUT) @ Matrix.Translation((length * 0.5, 0.0, 0.0))
    m.emit(kit._box((length, size, size), drop=('-x', '+x')), WHITE, 'trim', matrix, space)


def casing(space, o, inside=True, outside=True, sill=True):
    """White casings round an opening, outside and inside: jambs, and over them two boards along a pointed head, a
    ring round a round window or one board across a square head; a sill under a window."""
    x0, x1, z0, h, head = o.x, o.x + o.w, o.z, o.h, o.opts.get('head', 0.0)
    xm, zs = (x0 + x1) * 0.5, z0 + h
    if sill and z0 > 0.05:
        m.box((o.w + 2 * CASE_W + 0.1, 0.12 + THICK * 0.5, 0.05), at=(xm, -0.06 + THICK * 0.25, z0 - 0.025),
              rot=(rng.uniform(-1.5, 1.5), 0.0, 0.0), uv=WHITE, space=space, drop=('+y',))
    sides = ([(0.0, OUT, 0.058)] if outside else []) + ([(THICK, IN, 0.048)] if inside else [])
    for y, face, proud in sides:
        if o.opts.get('round'):
            r = o.w * 0.5
            rr, cz = r + CASE_W * 0.5, z0 + r
            for k in range(8):
                a0, a1 = 2.0 * math.pi * (k - 0.1) / 8, 2.0 * math.pi * (k + 1.1) / 8
                trim(space, (xm + rr * math.cos(a0), y, cz + rr * math.sin(a0)),
                     (xm + rr * math.cos(a1), y, cz + rr * math.sin(a1)), CASE_W, proud, face, 0.1)
            continue
        for x in (x0 - CASE_W * 0.5, x1 + CASE_W * 0.5):
            trim(space, (x, y, z0 - (0.04 if face is IN and z0 > 0.05 else 0.0)), (x, y, zs + (0.05 if head else 0.0)),
                 CASE_W, proud, face, 0.1)
        if not head:
            trim(space, (x0 - CASE_W, y, zs + CASE_W * 0.5), (x1 + CASE_W, y, zs + CASE_W * 0.5), CASE_W, proud + 0.004,
                 face, 0.1)
    if head and outside:
        head_timbers(space, o, inside=inside, width=CASE_W, uv=WHITE)


def b_window(space, o, glass=0.35, inside=True):
    """A window of the clapboard chapel: its casings and sill, white glazing bars and what's left of its glass (a few
    whole panes, a few shards in the corners)."""
    casing(space, o, inside=inside)
    x0, x1, z0, h, head = o.x, o.x + o.w, o.z, o.h, o.opts.get('head', 0.0)
    xm, zs = (x0 + x1) * 0.5, z0 + h
    yg = THICK * 0.5
    if o.opts.get('round'):
        r = o.w * 0.5
        cz = z0 + r
        b_bar(space, (xm, yg, cz - r), (xm, yg, cz + r))
        b_bar(space, (xm - r, yg, cz), (xm + r, yg, cz))
        for q in range(4):
            if rng.random() < glass:
                a0 = q * math.pi * 0.5
                rim = [(xm + r * math.cos(a0 + t * math.pi / 6.0), cz + r * math.sin(a0 + t * math.pi / 6.0))
                       for t in range(4)]
                pane(space, [(xm, cz)] + rim, yg)
        return
    rows = o.opts.get('rows', 3)
    b_bar(space, (xm, yg, z0), (xm, yg, zs + head - 0.03))
    for k in range(1, rows + (1 if head else 0)):
        b_bar(space, (x0 - 0.01, yg, z0 + h * k / rows), (x1 + 0.01, yg, z0 + h * k / rows))
    cells = []
    for c in range(2):
        for r in range(rows):
            a, b = x0 + c * o.w * 0.5, x0 + (c + 1) * o.w * 0.5
            cells.append([(a, z0 + h * r / rows), (b, z0 + h * r / rows), (b, z0 + h * (r + 1) / rows),
                          (a, z0 + h * (r + 1) / rows)])
    if head:
        cells += [[(x0, zs), (xm, zs), (xm, zs + head)], [(xm, zs), (x1, zs), (xm, zs + head)]]
    for cell in cells:
        roll = rng.random()
        if roll < glass:
            pane(space, cell, yg)
        elif roll < glass + 0.32:
            k = rng.randrange(len(cell))
            a, b, c = cell[k], cell[(k + 1) % len(cell)], cell[k - 1]
            t, s = rng.uniform(0.4, 0.85), rng.uniform(0.35, 0.8)
            pane(space, [a, (a[0] + (b[0] - a[0]) * t, a[1] + (b[1] - a[1]) * t),
                         (a[0] + (c[0] - a[0]) * s, a[1] + (c[1] - a[1]) * s)], yg)


def louvres(space, o, step=0.19):
    """A belfry's louvred opening: its casing outside and louvre boards across it, sloping down and out to shed the
    rain, following the pointed head."""
    casing(space, o, inside=False, sill=False)
    x0, x1, z0, h, head = o.x, o.x + o.w, o.z, o.h, o.opts['head']
    xm, zs, za = (x0 + x1) * 0.5, z0 + h, z0 + h + head
    z = z0 + 0.1
    while z < za - 0.1:
        half = o.w * 0.5 if z <= zs else o.w * 0.5 * (za - z) / head
        if half > 0.1:
            m.box((2.0 * half + 0.03, 0.17, 0.016), at=(xm, THICK * 0.5, z), rot=(40.0, 0.0, 0.0), uv=WHITE,
                  space=space, drop=('-x', '+x', '+y'))
        z += step


# --- The footing, the steps. ---
footing(X0, X1, Y0, Y1, BASE)
footing(-TX, TX, TY0, TY1, BASE + 0.005)
footing(-AX, AX, Y1, AY, BASE + 0.007)
footing(X1, VX, VY, Y1, BASE + 0.006)
for top, y0, y1, width in STEPS:
    m.box((width, y1 - y0, top + 0.1), at=(0.0, (y0 + y1) * 0.5, (top - 0.1) * 0.5), uv=Trim('D', world=True),
          cuts=1, drop=('-z',))
m.box((0.3, 1.5, 0.3), at=(VX + 0.08 + 0.15, 4.0, 0.05), uv=Trim('D', world=True), drop=('-z',))
m.section('footing')

# --- The nave: front (the gable behind the tower), right, back, left. ---
IN_DOOR_BLOCK = lc.opening_block(INNER_DOOR, CASE_W - 0.02)
space, length, outline = side((X0, Y0), (X1, Y0), [INNER_DOOR], top=[(6.4, WALL), (3.2, WALL + RISE), (0.0, WALL)],
                              cuts=(3.2,), cuts_x=(1.6, 4.8),
                              drop=lambda c, n: n.y < -0.9 and 1.6 < c.x < 4.8 and c.z > 3.25,   # inside the shaft
                              corners=((0.0, WALL), (0.0, WALL)), blocks=[lc.box_block(1.58, 4.82)],
                              boards=(FLOOR - BASE, WALL + RISE), board_blocks=[IN_DOOR_BLOCK])
front_space_b = space
# The vestibule's side of it, inside the tower: boards up to its ceiling.
lc.lap(m, space, outline, FLOOR - BASE, 3.22, [lc.box_block(-1.0, 1.62), lc.box_block(4.78, 9.0), IN_DOOR_BLOCK],
       INNER, 0.03, butt=False)
casing(space, INNER_DOOR, sill=False)
m.box((INNER_DOOR.w + 0.04, THICK + 0.06, 0.05), at=(INNER_DOOR.x + INNER_DOOR.w * 0.5, THICK * 0.5, 0.02), uv='C',
      space=space)
WIN_BLOCKS = lambda items: [lc.opening_block(o, CASE_W - 0.02) for o in items]
right_items_b = [bay(k, **WIN) for k in RIGHT_BAYS]
space, length, outline = side((X1, Y0), (X1, Y1), right_items_b + [VESTRY_DOOR], frieze=True, casings=False,
                              blocks=WIN_BLOCKS(right_items_b) + [lc.box_block(6.38, 9.9, -1.0, 3.88)],
                              corners=(True, (3.75, WALL)), boards=(FLOOR - BASE, WALL),
                              board_blocks=WIN_BLOCKS(right_items_b + [VESTRY_DOOR]))
right_space_b = space
for o in right_items_b:
    b_window(space, o)
casing(space, VESTRY_DOOR, outside=False, sill=False)
left_items_b = [bay(k, **WIN) for k in LEFT_BAYS]
space, length, outline = side((X0, Y1), (X0, Y0), left_items_b, frieze=True, boards=(FLOOR - BASE, WALL),
                              board_blocks=WIN_BLOCKS(left_items_b))
left_space_b = space
for o in left_items_b:
    b_window(space, o)
# The back gable: the apse's pointed arch (on the nave's axis) over the sanctuary floor, a round window high up.
APSE_ARCH_B = Opening('window', 3.2 - 0.735, SANCT, 1.47, 2.45, head=0.6)
space, length, outline = side((X1, Y1), (X0, Y1), [APSE_ARCH_B, ROSE], casings=False,
                              top=[(6.4, WALL), (3.2, WALL + RISE), (0.0, WALL)], corners=((3.75, WALL), True),
                              blocks=[lc.box_block(1.98, 4.42, -1.0, 4.05), lc.opening_block(ROSE, CASE_W - 0.02)],
                              boards=(FLOOR - BASE, WALL + RISE), board_blocks=WIN_BLOCKS([APSE_ARCH_B, ROSE]))
back_space_b = space
casing(space, APSE_ARCH_B, sill=False)
b_window(space, ROSE, glass=0.4)
m.section('nave walls')

# --- The tower: its walls rise from the footing to the belfry's top, sided, a louvred opening in each belfry face. ---
LOUVRE = Opening('window', TX - 0.75, SHAFT_Z + 0.35, 1.5, 1.35, head=0.55)
T_WIN = Opening('window', 1.6 - 0.31, 4.5, 0.62, 1.35, head=0.34, rows=2)
T_ROUND = Opening('window', 1.6 - 0.34, 6.95, 0.68, 0.68, round=True)
SHAFT_INSIDE = lambda c, n: n.y > 0.9 and 3.28 < c.z < SHAFT_Z    # the shaft's inside, over the vestibule ceiling
TOP_LAP = B_WALL - 0.26                                           # under the belfry's frieze
ROOF_SIDE = WALL + (3.2 - TX) * math.tan(THETA) + 0.1            # where the nave's roof meets the tower's sides
space, length, outline = side((-TX, TY0), (TX, TY0), [DOOR, T_WIN, T_ROUND, LOUVRE], height=B_WALL,
                              cuts=(3.28, SHAFT_Z), drop=SHAFT_INSIDE, corners=((0.0, TOP_LAP), (0.0, TOP_LAP)),
                              lap_to=SHAFT_Z, butt=6.6, boards=(FLOOR - BASE, 3.22),
                              board_blocks=[lc.opening_block(DOOR, CASE_W - 0.02)])
tower_front_b = space
casing(space, DOOR, sill=False)
b_window(space, T_WIN, glass=1.0, inside=False)
b_window(space, T_ROUND, glass=1.0, inside=False)
louvres(space, LOUVRE)
dx0, dx1 = DOOR.x, DOOR.x + DOOR.w
m.box((DOOR.w + 0.06, THICK + 0.1, 0.05), at=((dx0 + dx1) * 0.5, THICK * 0.5 - 0.02, 0.025), uv='C', space=space)
leaf_l_b = door_leaf((-TX + dx0, TY0 + THICK * 0.6, FLOOR), LEAF_ANGLES[0], LEAF_W, LEAF_H)
leaf_r_b = door_leaf((-TX + dx1, TY0 + THICK * 0.6, FLOOR), LEAF_ANGLES[1], LEAF_W, LEAF_H, mirror=True)
# The gabled hood over the door on two white brackets.
mid = DOOR.x + DOOR.w * 0.5
for s in (-1.0, 1.0):
    x = mid + s * 0.92
    m.board((x, -0.02, 2.66), (x, -0.8, 2.66), 0.13, 0.1, face=(1.0, 0.0, 0.0), uv=WHITE, space=space)
    m.board((x, -0.02, 2.06), (x, -0.5, 2.6), 0.11, 0.09, face=(1.0, 0.0, 0.0), uv=WHITE, space=space)
hood_frame = space.matrix @ Matrix.Translation((mid, 0.0, 0.0)) @ Matrix.Rotation(math.radians(-90.0), 4, 'Z')
h_front, h_back = gable(0.0, 0.86, -0.98, 0.98, 2.8, 45.0, overhang=0.12, rake=0.12, thick=0.05,
                        frame=hood_frame, ends=(False, True), rake_uv=WHITE)
shakes(h_front, piece=(3.0, 3.0))
shakes(h_back, piece=(3.0, 3.0))
ridge(h_front, h_back, width=0.1, thick=0.04)
m.socket('Crepe', (0.0, TY0 - 0.075, BASE + DOOR.h + 0.1))
m.socket('Perch_Hood', (0.0, TY0 - 0.9, BASE + 2.8 + 0.98 + 0.12))
space, length, outline = side((TX, TY0), (TX, TY1), [T_WIN, LOUVRE], height=B_WALL, cuts=(3.28, SHAFT_Z),
                              drop=SHAFT_INSIDE, corners=((0.0, TOP_LAP), (ROOF_SIDE, TOP_LAP)), lap_to=SHAFT_Z,
                              boards=(FLOOR - BASE, 3.22), butt=6.6)
tower_right_b = space
b_window(space, T_WIN, glass=1.0, inside=False)
louvres(space, LOUVRE)
space, length, outline = side((-TX, TY1), (-TX, TY0), [T_WIN, LOUVRE], height=B_WALL, cuts=(3.28, SHAFT_Z),
                              drop=SHAFT_INSIDE, corners=((ROOF_SIDE, TOP_LAP), (0.0, TOP_LAP)), lap_to=SHAFT_Z,
                              boards=(FLOOR - BASE, 3.22), butt=6.6)
tower_left_b = space
b_window(space, T_WIN, glass=1.0, inside=False)
louvres(space, LOUVRE)
# The back, from the nave's roof up (x from the right corner to the left).
space, length, outline = side((TX, TY1), (-TX, TY1), [LOUVRE], height=B_WALL, cuts=(SHAFT_Z,),
                              drop=lambda c, n: n.y > 0.9 and c.z < SHAFT_Z,
                              bottom=[(0.0, roof_at(0.0)), (TX, roof_at(TX)), (2 * TX, roof_at(2 * TX))],
                              corners=((ROOF_SIDE, TOP_LAP), (ROOF_SIDE, TOP_LAP)), lap_to=SHAFT_Z, water=False,
                              butt=6.6)
louvres(space, LOUVRE)
# Round the tower: a belt where the belfry starts; above it board and batten either side of each louvred opening;
# the belfry's frieze and cornice.
for p0, p1 in belfry_faces:
    sp = kit.wall_space(p0, p1, BASE)
    trim(sp, (-0.03, 0.0, SHAFT_Z), (2 * TX + 0.03, 0.0, SHAFT_Z), 0.14, 0.07)
    for x in (0.33, 0.54, 2 * TX - 0.54, 2 * TX - 0.33):
        trim(sp, (x, 0.0, SHAFT_Z + 0.07), (x, 0.0, TOP_LAP), 0.05, 0.025)
    trim(sp, (0.0, 0.0, B_WALL - 0.13), (2 * TX, 0.0, B_WALL - 0.13), 0.26, 0.045)
    m.box((2 * TX + 0.5, 0.27, 0.12), at=(TX, -0.13, B_WALL + 0.06), uv=WHITE, space=sp, drop=('+y',))
m.box((2 * TX + 0.5, 2 * TX + 0.5, 0.03), at=(0.0, TYC, B_TOP + 0.135), uv=WHITE, drop=('-z',))
# Inside: the vestibule's ceiling on two joists, the belfry's floor and the bell's beams.
m.box((3.0, TY1 - TY0, 0.12), at=(0.0, TYC + 0.1, BASE + 3.26), uv=PLANKS, mat='planks')
for y in (TY0 + 1.0, TY0 + 2.2):
    m.box((2.9, 0.14, 0.16), at=(0.0, y, BASE + 3.12), uv=BEAM())
m.box((2.96, 2.96, 0.1), at=(0.0, TYC, SHAFT - 0.05), uv=PLANKS, mat='planks')
for s in (-1.0, 1.0):
    m.box((2 * TX - 0.08, 0.16, 0.2), at=(0.0, TYC + s * 0.62, 11.15), uv=BEAM())
    m.box((0.16, 0.14, 0.045), at=(0.0, TYC + s * 0.62, 11.25 + 0.0225), uv=IRON_UV, mat='iron')
m.socket('Bell', (0.0, TYC, AXIS_Z))
m.section('tower')

# --- The spire: eight faces of white shingles in 0.4 m courses on a deck, white boards on the hips, the cross. ---
H8 = B_TIP - B_SPIRE
APO = B_R * math.cos(math.pi / 8.0)    # the centre to the middle of a face's foot
HALF = B_R * math.sin(math.pi / 8.0)   # half a face's foot
L8 = math.hypot(H8, APO)
for k in range(8):
    frame_k = Matrix.Translation((0.0, TYC, B_SPIRE)) @ Matrix.Rotation(math.radians(45.0 * k), 4, 'Z')
    face = kit.Slope((-HALF, -APO, 0.0), (1.0, 0.0, 0.0), (0.0, APO / L8, H8 / L8), 2.0 * HALF, L8, 0.0, frame=frame_k)
    tb = kit._new_bmesh()
    tb.faces.new([tb.verts.new(c) for c in ((0.0, 0.0, -0.06), (2.0 * HALF, 0.0, -0.06), (HALF, L8, -0.06))])
    m.emit(tb, WHITE, 'trim', None, face)
    course, step, butt = 0.54, 0.5, 0.03
    tilt = -math.degrees(math.atan2(butt, course))
    j, y0 = 0, -0.06
    while y0 < L8 - 0.2:
        y1 = min(y0 + course, L8 - 0.01)
        cy = (y0 + y1) * 0.5
        xl, xr = (lambda yy: HALF * yy / L8 - 0.02), (lambda yy: 2.0 * HALF - HALF * yy / L8 + 0.02)
        pts = [(xl(y0), y0 - cy), (xr(y0), y0 - cy), (xr(y1), y1 - cy), (xl(y1), y1 - cy)]
        matrix = (Matrix.Translation((0.0, cy, butt * 0.5 + 0.004 * (j % 2))) @ Matrix.Rotation(math.radians(tilt), 4, 'X')
                  @ Matrix.Translation((0.0, 0.0, butt * 0.5)) @ LAY)
        m.emit(slab(pts, butt), WHITE, 'trim', matrix, face, ao_floor=kit.ROOF_AO)
        y0 += step
        j += 1
    # The hip on this face's left: a white board along it.
    corner = frame_k @ Vector((-HALF, -APO, 0.0))
    apex = frame_k @ Vector((0.0, 0.0, H8))
    radial = (frame_k.to_3x3() @ Vector((-HALF, -APO, 0.0))).normalized()
    bisector = Vector((radial.x * H8, radial.y * H8, B_R)).normalized()
    h0, h1 = corner + bisector * 0.05, apex + bisector * 0.02 - (apex - corner).normalized() * 0.05
    matrix = kit.toward(h0, h1, tuple(bisector)) @ Matrix.Translation(((h1 - h0).length * 0.5, 0.0, 0.0))
    m.emit(kit._box(((h1 - h0).length, 0.04, 0.1), drop=('+y', '-x', '+x')), WHITE, 'trim', matrix)
m.cylinder((0.0, TYC, B_TIP - 0.25), (0.0, TYC, B_TIP + 0.32), 0.035, sides=6, uv=IRON_UV, mat='iron')
m.cylinder((0.0, TYC, B_TIP + 0.02), (0.0, TYC, B_TIP + 0.16), 0.075, sides=8, uv=IRON_UV, mat='iron')
m.box((0.05, 0.05, 0.66), at=(0.0, TYC, B_TIP + 0.62), uv=IRON_UV, mat='iron')
m.box((0.42, 0.05, 0.05), at=(0.0, TYC, B_TIP + 0.76), uv=IRON_UV, mat='iron')
m.section('spire')

# --- The nave's roof: shakes, white rake boards. ---
right_roof_b, left_roof_b = gable(Y0, Y1, -X1, -X0, EAVE, PITCH, overhang=0.4, rake=0.35, thick=0.12, sag=0.07,
                                  frame=Matrix.Rotation(math.radians(90.0), 4, 'Z'), rake_uv=WHITE)
shakes(right_roof_b, piece=(3.4, 5.4))
shakes(left_roof_b, piece=(3.4, 5.4))
ridge(right_roof_b, left_roof_b)
m.section('nave roof')

# --- The apse: three short walls under a lean-to roof against the back gable. ---
APSE_IN = (FLOOR + SANCT - BASE, 3.9)
space, length, outline = side((AX, Y1), (AX, AY), [], top=[(AY - Y1, apse_top(AY)), (0.0, apse_top(Y1))],
                              corners=(None, (0.0, apse_top(AY))), boards=APSE_IN)
apse_r_b = space
space, length, outline = side((AX, AY), (-AX, AY), [APSE_WIN], height=3.2, corners=((0.0, 3.2), (0.0, 3.2)),
                              boards=(APSE_IN[0], 3.2), board_blocks=WIN_BLOCKS([APSE_WIN]))
b_window(space, APSE_WIN, glass=0.5)
space, length, outline = side((-AX, AY), (-AX, Y1), [], top=[(AY - Y1, apse_top(Y1)), (0.0, apse_top(AY))],
                              corners=((0.0, apse_top(AY)), None), boards=APSE_IN)
apse_roof_b = kit.Slope(a_origin, (-1.0, 0.0, 0.0), (0.0, -math.cos(a_theta), math.sin(a_theta)), 2 * AX + 0.5,
                        (AY - Y1 + a_over) / math.cos(a_theta), sag=0.02)
deck(apse_roof_b, a_deck, rake_uv=WHITE)
shakes(apse_roof_b, piece=(1.4, 2.4), top=apse_roof_b.length - 0.03)
m.box((apse_roof_b.width + 0.02, 0.18, 0.04), at=(apse_roof_b.width * 0.5, apse_roof_b.length - 0.08, 0.05),
      rot=(-6.0, 0.0, 0.0), uv='C', space=apse_roof_b, cuts=2)
m.section('apse')

# --- The vestry: a lean-to on the nave's right side by the sanctuary, its door facing out. ---
side((X1, VY), (VX, VY), [], top=[(VX - X1, vestry_top(VX)), (0.0, vestry_top(X1))], drop=INSIDE_FACE,
     corners=(None, (0.0, vestry_top(VX))))
space, length, outline = side((VX, VY), (VX, Y1), [VDOOR], height=2.6, drop=INSIDE_FACE,
                              corners=((0.0, 2.6), (0.0, 2.6)))
plank_door(space, VDOOR, paint='A', hinge_left=False, lintel=False)
casing(space, VDOOR, inside=False, sill=False)
space, length, outline = side((VX, Y1), (X1, Y1), [VWIN], top=[(VX - X1, vestry_top(X1)), (0.0, vestry_top(VX))],
                              drop=INSIDE_FACE, corners=((0.0, vestry_top(VX)), None))
b_window(space, VWIN, glass=1.0, inside=False)
vestry_roof_b = kit.Slope(v_origin, (0.0, 1.0, 0.0), (-math.cos(v_theta), 0.0, math.sin(v_theta)),
                          (Y1 - VY) + 2 * v_rake, (VX - X1 + v_over) / math.cos(v_theta), sag=0.03)
deck(vestry_roof_b, v_deck, rake_uv=WHITE)
shakes(vestry_roof_b, piece=(1.6, 2.6), top=vestry_roof_b.length - 0.03)
m.box((vestry_roof_b.width + 0.02, 0.18, 0.04), at=(vestry_roof_b.width * 0.5, vestry_roof_b.length - 0.08, 0.05),
      rot=(-6.0, 0.0, 0.0), uv='C', space=vestry_roof_b, cuts=2)
# The lantern by its door, on an iron bracket.
m.box((0.03, 0.12, 0.3), at=(VX + 0.015, LANTERN[1], LANTERN[2] + 0.32), uv=IRON_UV, mat='iron')
m.box((0.36, 0.03, 0.03), at=(VX + 0.18, LANTERN[1], LANTERN[2] + 0.43), uv=IRON_UV, mat='iron')
m.board((VX + 0.02, LANTERN[1], LANTERN[2] + 0.22), (VX + 0.26, LANTERN[1], LANTERN[2] + 0.42), 0.025, 0.025,
        face=(0.0, 1.0, 0.0), uv=IRON_UV, mat='iron')
m.box((0.2, 0.2, 0.04), at=(lx, ly, lz + 0.15), uv=IRON_UV, mat='iron')
m.cylinder((lx, ly, lz + 0.17), (lx, ly, lz + 0.27), 0.11, 0.025, sides=6, uv=IRON_UV, mat='iron', caps=(False, False))
m.box((0.15, 0.15, 0.24), at=(lx, ly, lz), uv=Trim('H4', fit=True), mat='glow')
m.box((0.2, 0.2, 0.04), at=(lx, ly, lz - 0.14), uv=IRON_UV, mat='iron')
for dx in (-0.085, 0.085):
    for dy in (-0.085, 0.085):
        m.box((0.022, 0.022, 0.26), at=(lx + dx, ly + dy, lz), uv=IRON_UV, mat='iron', drop=('-z', '+z'))
m.box((0.02, 0.02, 0.12), at=(lx, ly, lz + 0.36), uv=IRON_UV, mat='iron', drop=('-z',))
m.socket('Light', LANTERN)
m.socket('Speaker', (VX + 0.09, VY + VDOOR.x + VDOOR.w * 0.5, BASE + 1.6), (0.0, 0.0, 90.0))
m.socket('Perch_Lantern', (VX + 0.3, LANTERN[1], LANTERN[2] + 0.46))
m.section('vestry')

furnish(right_roof_b, left_roof_b)

# --- Collision: as the chapel's, but for the tower's top (a closed belfry, the cornice, the octagonal spire). ---
m.hull((X1 - X0 + 0.16, Y1 - Y0 + 0.16, FLOOR), at=(0.0, (Y0 + Y1) * 0.5, FLOOR * 0.5))
m.hull((2 * TX + 0.16, TY1 - TY0 + 0.08, FLOOR), at=(0.0, TYC - 0.04, FLOOR * 0.5))
for top, y0, y1, width in STEPS:
    m.hull((width, y1 - y0, top), at=(0.0, (y0 + y1) * 0.5, top * 0.5))
m.hull((0.3, 1.5, 0.2), at=(VX + 0.23, 4.0, 0.1))
m.hull((6.08, SANCT_Y - 3.2, STEP), at=(0.0, (SANCT_Y + 3.2) * 0.5, FLOOR + STEP * 0.5))
m.hull((6.08, Y1 - SANCT_Y, SANCT), at=(0.0, (SANCT_Y + Y1) * 0.5, FLOOR + SANCT * 0.5))
m.hull((2 * AX + 0.16, AY - Y1 + 0.08, FLOOR + SANCT), at=(0.0, (Y1 + AY) * 0.5 + 0.04, (FLOOR + SANCT) * 0.5))
m.hull((1.56, 1.02, 0.42), at=(0.0, ALTAR_Y, ALTAR_TOP - 0.21))
wall_hulls(front_space_b, 6.4, [INNER_DOOR], WALL)
wall_hulls(right_space_b, 9.6, [], WALL)   # solid: the sills are 1.35 m up, nobody passes a window
wall_hulls(left_space_b, 9.6, [], WALL)
wall_hulls(back_space_b, 6.4, [APSE_ARCH_B], WALL)
for y, sign in ((Y0, 1.0), (Y1, -1.0)):
    m.hull_points([(x, y + sign * t, z) for t in (0.0, THICK) for x, z in ((X0, EAVE), (X1, EAVE), (0.0, EAVE + RISE))])
wall_hulls(tower_front_b, 2 * TX, [DOOR], 3.32)
for sp in (tower_right_b, tower_left_b):
    m.hull((TY1 - TY0, THICK, 3.32), at=((TY1 - TY0) * 0.5, THICK * 0.5, 1.66), space=sp)
m.hull((2 * TX, TY1 - TY0, B_TOP - BASE - 3.2), at=(0.0, TYC, (BASE + 3.2 + B_TOP) * 0.5))   # the belfry included
for leaf in (leaf_l_b, leaf_r_b):
    m.hull((LEAF_W, 0.06, LEAF_H), matrix=leaf @ Matrix.Translation((LEAF_W * 0.5, 0.03, LEAF_H * 0.5)))
m.hull((2 * TX + 0.5, 2 * TX + 0.5, 0.15), at=(0.0, TYC, B_TOP + 0.075))
OCTAGON = [math.pi * (2 * k + 1) / 8.0 - math.pi * 0.5 for k in range(8)]   # the spire's corners round the tower axis
m.hull_points([(B_R * math.cos(a), TYC + B_R * math.sin(a), B_SPIRE) for a in OCTAGON] + [(0.0, TYC, B_TIP)])
for slope in (right_roof_b, left_roof_b):
    m.hull((slope.width, slope.length, 0.24), at=(slope.width * 0.5, slope.length * 0.5, -0.06), space=slope)
m.hull((apse_roof_b.width, apse_roof_b.length, 0.2), at=(apse_roof_b.width * 0.5, apse_roof_b.length * 0.5, -0.04),
       space=apse_roof_b)
for (p0, p1), height in ((((AX, Y1), (AX, AY)), 3.9), (((AX, AY), (-AX, AY)), 3.2), (((-AX, AY), (-AX, Y1)), 3.9)):
    sp = kit.wall_space(p0, p1, BASE)
    length = (Vector(p1) - Vector(p0)).length
    m.hull((length, THICK, height), at=(length * 0.5, THICK * 0.5, height * 0.5), space=sp)
m.hull_points([(x, y, z) for x in (X1, VX + 0.08) for y in (VY - 0.08, Y1 + 0.08) for z in (0.0, BASE + 2.7)] +
              [(X1, y, BASE + 3.75) for y in (VY - 0.08, Y1 + 0.08)])   # the closed vestry, to its roof
for s in (-1.0, 1.0):
    xs = [s * 0.55, s * (0.65 + PEW_LEN + 0.12)]
    m.hull((abs(xs[1] - xs[0]), PEW_ROWS[-1] - PEW_ROWS[0] + 0.85, 0.95),
           at=((xs[0] + xs[1]) * 0.5, (PEW_ROWS[0] + PEW_ROWS[-1]) * 0.5 - 0.1, FLOOR + 0.475))

lt.bake_vertex_ao = _bake_two_pass
chapel = m.finish(preview=False, fallback=100.0)
lt.bake_vertex_ao = _bake

if lt.want_preview():
    bell.location = (0.0, TYC, AXIS_Z)
    bpy.context.view_layer.update()
    # The game's chapel, then option A from the same cameras.
    for model, name, place in ((chapel, 'Chapel', 'RansomsRest'), (chapel_a, 'Chapel_A', 'RansomsRest/Chapel')):
        both = [model, bell]
        lt.preview(both, lt.preview_path('Buildings', name), view=(-1.0, -1.45, 0.36), fit=0.9)
        lt.preview(both, lt.preview_path('Buildings', f'{name}_Back'), view=(1.15, 1.35, 0.36), fit=0.9)
        # Inside, a low sun through the left windows. The preview darkens everything by the baked occlusion, Unreal
        # only the ambient light, so the sun is set stronger here to show its patches as bright as in the game.
        lc.render(both, lt.preview_path(place, f'{name}_Interior'), camera_at=(0.55, -3.35, FLOOR + 1.55),
                  look_at=(-0.15, 6.0, FLOOR + 1.45), lens=20.0, sun=(-0.86, -0.22, 0.46), sun_strength=7.0,
                  exposure=0.5, resolution=(1280, 800))
        lc.render(both, lt.preview_path('RansomsRest/Chapel', f'{name}_ThreeQuarter'), camera_at=(-8.5, -16.0, 1.75),
                  look_at=(0.2, -3.0, 6.0), lens=26.0, resolution=(1100, 1300))
    bell.location = (0.0, 0.0, 0.0)
    bpy.context.view_layer.update()
    lt.preview([bell], lt.preview_path('RansomsRest/Chapel', 'ChapelBell'), view=(-1.0, -1.6, 0.3), fit=0.9)
elif __name__ in ('__overview__', '__vignette__'):
    bell.location = (0.0, TYC, AXIS_Z)   # the overview shows it in the belfry
    bell['Mounted'] = 1
