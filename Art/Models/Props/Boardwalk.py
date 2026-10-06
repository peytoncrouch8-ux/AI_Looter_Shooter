"""The boardwalk kit of Main Street on Ransom's Rest (Docs/Areas/RansomsRest.md, Main Street) and its street props: a
raised plank walk along the false fronts (Art/Models/Buildings/FalseFronts.py), awnings over it, hitch rails at its
edge, the Rim Rangers' notice board and the town memorial.

  Boardwalk_4m, Boardwalk_2m  a plank walk 3 m deep, its top at 0.38 m (under the character's step height): planks
                              across it every 0.2 m on joists, rim boards front and back, posts on a ground sill.
  Boardwalk_Steps             the end of a run: an end board and one step of 0.19 m down to the ground, along +X.
  Boardwalk_Corner            a 3 x 3 m square with mitred planks where a run turns: open on its -X side (the run
                              comes in) and its +Y side (it turns away from the street), rims on +X and -Y.
  Awning_4m, Awning_4m_Boards one 4 m bay of a shed roof over the walk, of shakes or of battened boards: a post at its
                              start (x = 0), a beam to the next bay's post, rafters from a ledger on the facade.
                              Awning_Post ends a run. (No tin: the trim sheet's tin reads as loot colors in a run.)
  HitchRail                   a peeled pole on two hewn posts with two iron rings, for the street edge.
  NoticeBoard                 the Rim Rangers' board: two posts, a little shake roof, RIM RANGERS on the header, and a
                              flat face for the posters and Ruth Calder's note (decals). SOCKET_Interact in front,
                              SOCKET_Decal at the middle of the face.
  TownMemorial                two hats on a post, a black felt and a straw, and a mourning wreath, on a little cairn
                              with larkspur laid at its foot.

Chaining: the walk, steps and awning pieces have their pivot on the ground at their start (x = 0) on the walk's middle
line (y = 0): the walk runs along +X from y = -1.5 (the street side, the front) to y = +1.5, where the building fronts
stand. Pieces join every 2 or 4 m with no seam (planks every 0.2 m from x = 0.1). Boardwalk_Steps goes at a run's end
(turned 180 degrees at its start). Boardwalk_Corner's pivot is its middle: put it 1.5 m past a run's end; the run turning
away from the street starts on its +Y edge, turned 90 degrees (turn the corner -90 degrees for a run's other end).
Awning bays chain every 4 m on the same line as the walk (their ledger sits on the facade at y = +1.5, 3.3 m up);
finish a run with Awning_Post at the last bay's end.

Every piece is a small prop: Nanite off, LODs 50% and 25%. A scripted model (Art/README.md) built with looter_buildings
and looter_town.
"""
import math

import bmesh

import looter_buildings as kit
import looter_textures as lt
import looter_town as town
from looter_buildings import Matrix, Trim, Vector

TOP = town.DECK_TOP
HALF = town.WALK_DEPTH * 0.5          # the walk spans y = -HALF (street) to +HALF (the building fronts)
PROPS = dict(Nanite=0, LODs='50,25')
PREVIEW = 'RansomsRest/Town'


def done(m, **props):
    obj = m.finish(preview=False, **dict(PROPS, **props))
    return obj


# --- The walk ---

def boardwalk(name, length, seed):
    m = town.Model(name, seed=seed)
    posts = [x for x in (1.0, 3.0) if x < length]
    town.deck(m, 0.0, length, -HALF, HALF, posts=posts, plank_cuts=0)
    m.hull((length, town.WALK_DEPTH + 0.04, TOP + 0.1), at=(length * 0.5, 0.0, (TOP - 0.1) * 0.5))
    return done(m)


def steps(name, seed):
    m = town.Model(name, seed=seed)
    tread = TOP * 0.5                      # one step of 0.19 m between the walk and the ground
    # The end board over the walk's open end (its joists and plank ends), then the tread, its riser and blocks under.
    town.board(m, (0.0, -HALF, TOP - 0.15), (0.0, HALF, TOP - 0.15), 0.2, 0.045, look='A', face=(1.0, 0.0, 0.0),
               lift=-0.0225)
    for k, x in enumerate((0.095, 0.27)):
        town.board(m, (x, -HALF - 0.01, tread - 0.025 + m.rng.uniform(-0.003, 0.0)), (x, HALF + 0.01, tread - 0.025),
                   0.168, 0.05, look='planks', face=(0.0, 0.0, 1.0), drop=('+y',))
    town.board(m, (0.33, -HALF, (tread - 0.05) * 0.5 - 0.02), (0.33, HALF, (tread - 0.05) * 0.5 - 0.02), tread - 0.01,
               0.035, look='A', face=(1.0, 0.0, 0.0), lift=0.0, drop=('+y',))
    for y in (-HALF + 0.12, 0.0, HALF - 0.12):
        town.box(m, (0.3, 0.07, tread - 0.05), at=(0.16, y, (tread - 0.05) * 0.5 - 0.01), look='C', drop=('-z',))
    m.hull((0.36, town.WALK_DEPTH + 0.02, tread + 0.05), at=(0.18, 0.0, (tread - 0.05) * 0.5))
    return done(m)


def corner(name, seed):
    """A 3 x 3 m square of walk, its planks mitred along the diagonal from its inner corner (-X, +Y) to its outer one."""
    m = town.Model(name, seed=seed)
    rng = m.rng
    w = town.PLANK - 0.012
    z = TOP - 0.025
    for k in range(int(round(town.WALK_DEPTH / town.PLANK))):
        c = -HALF + town.PLANK * (k + 0.5)
        # Planks running along Y beside the -X edge (as the incoming run's do), cut where they meet the diagonal...
        y_end = -c
        if y_end + HALF > 0.04:
            p0, p1 = Vector((c, -HALF - 0.02, z)), Vector((c, y_end, z))
            length = (p1 - p0).length
            matrix = kit.toward(p0, p1, (0.0, 0.0, 1.0)) @ Matrix.Translation((length * 0.5, 0.0, 0.0))
            m.emit(kit._box((length, 0.05, w), 0.0, 0, ('+y',)), town.PlankRow(), 'planks', matrix,
                   shape=lambda co: Vector((co.x + (co.z if co.x > 0.0 else 0.0), co.y, co.z)))
        # ... and planks running along X beside the +Y edge (across the run that turns away), cut the same way.
        x_start = -c
        if HALF - x_start > 0.04:
            p0, p1 = Vector((x_start, c, z)), Vector((HALF + 0.02, c, z))
            length = (p1 - p0).length
            matrix = kit.toward(p0, p1, (0.0, 0.0, 1.0)) @ Matrix.Translation((length * 0.5, 0.0, 0.0))
            m.emit(kit._box((length, 0.05, w), 0.0, 0, ('+y',)), town.PlankRow(), 'planks', matrix,
                   shape=lambda co: Vector((co.x - (co.z if co.x < 0.0 else 0.0), co.y, co.z)))
    # Under it: rims on the two outer edges, a joist along each and one under the mitre, posts and a ground sill.
    rim = 0.2
    town.board(m, (-HALF, -HALF, TOP - 0.05 - rim * 0.5), (HALF, -HALF, TOP - 0.05 - rim * 0.5), rim, 0.045, look='A',
               face=(0.0, -1.0, 0.0), lift=-0.0225, drop=('+y',))
    town.board(m, (HALF, -HALF, TOP - 0.05 - rim * 0.5), (HALF, HALF, TOP - 0.05 - rim * 0.5), rim, 0.045, look='A',
               face=(1.0, 0.0, 0.0), lift=-0.0225, drop=('+y',))
    town.box(m, (town.WALK_DEPTH, 0.08, 0.16), at=(0.0, -HALF + 0.12, TOP - 0.13), look='C', drop=('-z',))
    town.box(m, (0.08, town.WALK_DEPTH, 0.16), at=(HALF - 0.12, 0.0, TOP - 0.13), look='C', drop=('-z',))
    diagonal = town.WALK_DEPTH * math.sqrt(2.0)
    town.box(m, (diagonal - 0.2, 0.08, 0.16), at=(0.0, 0.0, TOP - 0.13), rot=(0.0, 0.0, -45.0), look='C', drop=('-z',))
    for x, y in ((HALF - 0.1, -HALF + 0.1), (-0.5, -HALF + 0.1), (HALF - 0.1, 0.5), (0.0, 0.0)):
        town.box(m, (0.14, 0.14, TOP - 0.21 + 0.12), at=(x, y, (TOP - 0.21 - 0.12) * 0.5),
                 rot=(0.0, 0.0, rng.uniform(-6.0, 6.0)), look='C', drop=('-z',))
    town.box(m, (town.WALK_DEPTH, 0.16, 0.14), at=(0.0, -HALF + 0.1, -0.05), look='C', drop=('-z',))
    town.box(m, (0.16, town.WALK_DEPTH - 0.16, 0.14), at=(HALF - 0.1, 0.08, -0.05), look='C', drop=('-z',))
    m.hull((town.WALK_DEPTH + 0.04, town.WALK_DEPTH + 0.04, TOP + 0.1), at=(0.0, 0.0, (TOP - 0.1) * 0.5))
    return done(m)


# --- Awnings ---

POST_Y = -HALF - 0.12                 # the posts stand just off the walk's street edge, down to the ground
BEAM_TOP = 2.92
LEDGER_TOP = 3.42                     # where the roof meets the facade (y = +HALF)
FRONT_Y = -HALF - 0.47                # the roof's front edge


def awning_post(m, x):
    town.board(m, (x, POST_Y, -0.05), (x, POST_Y, BEAM_TOP - 0.2), 0.15, 0.15, look='C', bevel=0.012)
    town.box(m, (0.2, 0.2, 0.05), at=(x, POST_Y, BEAM_TOP - 0.225), look='C')
    m.hull((0.17, 0.17, BEAM_TOP - 0.15), at=(x, POST_Y, (BEAM_TOP - 0.25) * 0.5))


def awning(name, seed, covering='shakes'):
    m = town.Model(name, seed=seed)
    bay = 4.0
    awning_post(m, 0.0)
    town.board(m, (0.0, POST_Y, BEAM_TOP - 0.1), (bay, POST_Y, BEAM_TOP - 0.1), 0.2, 0.15, look='C')
    town.board(m, (0.0, HALF - 0.025, LEDGER_TOP - 0.1), (bay, HALF - 0.025, LEDGER_TOP - 0.1), 0.2, 0.05, look='C',
               face=(0.0, -1.0, 0.0))
    theta = math.atan2(LEDGER_TOP - BEAM_TOP, (HALF - 0.05) - POST_Y)

    def rafter_top(y):
        return BEAM_TOP + (y - POST_Y) * math.tan(theta)
    for x in (0.5, 1.5, 2.5, 3.5):
        p0 = (x, HALF - 0.05, rafter_top(HALF - 0.05) - 0.07)
        p1 = (x, FRONT_Y + 0.03, rafter_top(FRONT_Y + 0.03) - 0.07)
        town.board(m, p0, p1, 0.14, 0.06, look='C', face=(1.0, 0.0, 0.0))
    # The roof: a deck on the rafters, covered from the front edge up to the facade, a fascia along the front. The
    # covering stops exactly at the bay's ends, so bays chain without overlapping.
    deck = 0.04
    normal = Vector((0.0, -math.sin(theta), math.cos(theta)))
    lap = 0.05 if covering == 'shakes' else 0.0           # how far the kit's shakes run past a slope's sides
    origin = Vector((lap, FRONT_Y, rafter_top(FRONT_Y))) + normal * deck
    length = (HALF - FRONT_Y) / math.cos(theta)
    slope = kit.Slope(origin, (1.0, 0.0, 0.0), (0.0, math.cos(theta), math.sin(theta)), bay - 2.0 * lap, length,
                      sag=0.03)
    m.box((bay, length, deck), at=(bay * 0.5 - lap, length * 0.5, -deck * 0.5), uv=Trim('A', world=True), space=slope,
          cuts=3)
    if covering == 'shakes':
        kit.shingles(m, slope, piece=(1.3, 2.4), top=length - 0.02)
    else:
        # Boards down the slope, a batten over each joint (the bay's first batten covers the joint with the bay before).
        count = int(round(bay / town.PLANK))
        for k in range(count):
            x = town.PLANK * (k + 0.5)
            town.board(m, (x, -0.01, 0.0125), (x, length, 0.0125), town.PLANK - 0.006, 0.025, look='A',
                       face=(0.0, 0.0, 1.0), space=slope, drop=('+y',))
            batten = town.PLANK * k
            town.board(m, (batten, -0.01, 0.035), (batten, length, 0.035), 0.055, 0.02, look='A', face=(0.0, 0.0, 1.0),
                       space=slope, drop=('+y',))
    town.board(m, (0.0, HALF - 0.03, LEDGER_TOP + 0.06), (bay, HALF - 0.03, LEDGER_TOP + 0.06), 0.14, 0.04, look='C',
               face=(0.0, -1.0, 0.0))
    fascia_z = rafter_top(FRONT_Y) - 0.06
    town.board(m, (0.0, FRONT_Y - 0.02, fascia_z), (bay, FRONT_Y - 0.02, fascia_z), 0.18, 0.04, look='A')
    return done(m)


def awning_end(name, seed):
    m = town.Model(name, seed=seed)
    awning_post(m, 0.0)
    return done(m)


# --- Hitch rail ---

def hitch_rail(name, seed):
    m = town.Model(name, seed=seed)
    rng = m.rng
    span = 2.6
    top = 1.02
    for x in (-span * 0.5, span * 0.5):
        lean = rng.uniform(-1.5, 1.5)
        town.board(m, (x, 0.0, -0.05), (x + math.sin(math.radians(lean)) * top, 0.0, top), 0.15, 0.15, look='C',
                   bevel=0.012)
        town.box(m, (0.17, 0.17, 0.04), at=(x, 0.0, top + 0.01), look='C', rot=(0.0, 0.0, rng.uniform(-4.0, 4.0)))
    # The pole: one peeled log on strip B, sagging a hair, its ends past the posts.
    kit.log(m, (-span * 0.5 - 0.22, 0.0, top + 0.09), (span * 0.5 + 0.22, 0.0, top + 0.085), r=0.07, sides=8)
    for x in (-span * 0.5, span * 0.5):
        town.box(m, (0.05, 0.17, 0.035), at=(x, 0.0, top + 0.1), look='H3', drop=('-z',))
    # Two iron rings hanging from staples, for the reins.
    for x in (-0.55, 0.62):
        segments = 8
        for k in range(segments):
            a0, a1 = 2.0 * math.pi * k / segments, 2.0 * math.pi * (k + 1) / segments
            r = 0.06
            p0 = (x + r * math.sin(a0), -0.075, top + 0.0 - r + r * math.cos(a0))
            p1 = (x + r * math.sin(a1), -0.075, top + 0.0 - r + r * math.cos(a1))
            town.board(m, p0, p1, 0.014, 0.014, look='H3', face=(0.0, -1.0, 0.0))
        town.box(m, (0.03, 0.03, 0.06), at=(x, -0.06, top + 0.03), look='H3')
    m.hull((span + 0.5, 0.2, top + 0.2), at=(0.0, 0.0, (top + 0.2) * 0.5 - 0.05))
    obj = done(m)
    return obj


# --- The Rim Rangers' notice board ---

def notice_board(name, seed):
    m = town.Model(name, seed=seed)
    rng = m.rng
    half = 0.85
    post_top = 2.45
    for x in (-half, half):
        town.board(m, (x, 0.0, -0.05), (x, 0.0, post_top), 0.14, 0.14, look='C', bevel=0.012)
    # The face: one flat panel of upright boards (the posters and the note are decals on it), in a frame.
    fw = 2.0 * half - 0.16
    fz0, fz1 = 0.88, 1.84
    m.box((fw, 0.04, fz1 - fz0), at=(0.0, -0.06, (fz0 + fz1) * 0.5), uv=Trim('A', world=True, rotate=True))
    for z in (fz0 - 0.04, fz1 + 0.04):
        town.board(m, (-half + 0.02, -0.06, z), (half - 0.02, -0.06, z), 0.1, 0.07, look='C')
    for x in (-half + 0.1, half - 0.1):
        town.board(m, (x, -0.06, fz0), (x, -0.06, fz1), 0.07, 0.07, look='C')
    town.board(m, (-half - 0.05, -0.07, fz0 - 0.1), (half + 0.05, -0.07, fz0 - 0.1), 0.09, 0.12, look='C',
               face=(0.0, 0.0, 1.0))
    # The header: an oxide-red board with the Rangers' name.
    hz = fz1 + 0.2
    town.board(m, (-half + 0.02, -0.07, hz), (half - 0.02, -0.07, hz), 0.2, 0.035, look='H2')
    town.text(m, 'RIM RANGERS', (0.0, -0.088, hz), height=0.12, width=1.4, look='cream', tol=0.03)
    # A little gable roof of shakes on a crossbeam.
    town.board(m, (-half - 0.12, 0.0, post_top - 0.06), (half + 0.12, 0.0, post_top - 0.06), 0.12, 0.12, look='C')
    front, back = kit.gable(m, -half - 0.1, half + 0.1, -0.16, 0.16, post_top, 38.0, overhang=0.22, rake=0.1, deck=0.035)
    for slope in (front, back):
        kit.shingles(m, slope, piece=(0.7, 1.2))
    kit.ridge_cap(m, front, back, uv='C', width=0.1, thick=0.035)
    m.socket('Interact', (0.0, -0.55, (fz0 + fz1) * 0.5))
    m.socket('Decal', (0.0, -0.081, (fz0 + fz1) * 0.5))
    m.hull((2.0 * half + 0.2, 0.3, post_top + 0.35), at=(0.0, -0.02, (post_top + 0.35) * 0.5 - 0.05))
    return done(m)


# --- The town memorial ---

class Swatch:
    """A part on a FoliagePalette swatch, as lt.swatch_uv maps one (U from its root at 0 to its tip at 1, V across the
    swatch), but repeatable: its root is its first vertex and its tip the vertex farthest from it, in mesh order
    (swatch_uv picks its direction from a set of vertices, which iterates in a different order on every run)."""

    def __init__(self, name):
        self.name = name

    def apply(self, obj, rng):
        mesh = obj.data
        rows = len(lt.PALETTE)
        index = lt.PALETTE.index(self.name)
        v_lo, v_hi = (index + 0.15) / rows, (index + 0.85) / rows
        cos = [v.co.copy() for v in mesh.vertices]
        root = cos[0]
        tip = max(range(len(cos)), key=lambda i: (cos[i] - root).length)
        direction = (cos[tip] - root).normalized()
        side = direction.cross(Vector((0.0, 0.0, 1.0)) if abs(direction.z) < 0.9 else Vector((1.0, 0.0, 0.0)))
        side.normalize()
        along = [(c - root).dot(direction) for c in cos]
        across = [(c - root).dot(side) for c in cos]
        lo, hi = min(along), max(along)
        a_lo, a_hi = min(across), max(across)
        uv = mesh.uv_layers.active.data
        for p in mesh.polygons:
            for li in p.loop_indices:
                i = mesh.loops[li].vertex_index
                u = (along[i] - lo) / max(hi - lo, 1e-6)
                w = (across[i] - a_lo) / max(a_hi - a_lo, 1e-6)
                uv[li].uv = (min(max(u, 0.01), 0.99), v_lo + w * (v_hi - v_lo))


def hat(m, at, tilt=(0.0, 0.0, 0.0), look='crepe', band='crepe', scale=1.0):
    """A cattleman's hat: a curled brim and a creased crown, a band round it."""
    s = scale
    profile = [(0.0, 0.006), (0.12, 0.006), (0.205, 0.0), (0.222, 0.016), (0.205, 0.024), (0.108, 0.02), (0.1, 0.06),
               (0.094, 0.12), (0.075, 0.15), (0.03, 0.142), (0.0, 0.13)]
    axis = kit.place(rot=tilt)
    town.lathe(m, [(r * s, z * s) for r, z in profile], at=at, sides=8, look=look, axis=axis)
    town.lathe(m, [(0.112 * s, 0.024 * s), (0.106 * s, 0.052 * s)], at=at, sides=8, look=band, axis=axis)


def memorial(name, seed):
    m = town.Model(name, seed=seed)
    rng = m.rng
    # A little cairn of fieldstones round the post's foot.
    for k in range(6):
        a = 2.0 * math.pi * k / 6 + rng.uniform(-0.2, 0.2)
        r = rng.uniform(0.2, 0.3)
        size = (rng.uniform(0.2, 0.3), rng.uniform(0.16, 0.24), rng.uniform(0.12, 0.2))
        town.box(m, size, at=(r * math.cos(a), r * math.sin(a), size[2] * 0.4),
                 rot=(rng.uniform(-10, 10), rng.uniform(-10, 10), math.degrees(a) + rng.uniform(-20, 20)),
                 look='world_D', drop=('-z',))
    for k in range(2):
        a = 2.0 * math.pi * k / 2 + 0.4
        size = (0.2, 0.17, 0.14)
        town.box(m, size, at=(0.13 * math.cos(a), 0.13 * math.sin(a), 0.22), rot=(rng.uniform(-8, 8), 0.0,
                 math.degrees(a)), look='world_D')
    # The post, leaning a little, with a crossarm; a peg at each arm's end for a hat.
    lean = 1.5
    top = 1.78
    town.board(m, (0.0, 0.0, -0.05), (math.sin(math.radians(lean)) * top, 0.0, top), 0.15, 0.15, look='C', bevel=0.012)
    arm_z = 1.5
    town.board(m, (-0.42, -0.0, arm_z), (0.46, 0.0, arm_z), 0.12, 0.1, look='C', face=(0.0, -1.0, 0.0), lift=0.06)
    for x in (-0.36, 0.4):
        town.box(m, (0.03, 0.03, 0.09), at=(x, -0.11, arm_z + 0.06), look='C', drop=('-z',))
    # The hats: the sheriff's black felt on the left peg, a farmhand's straw on the right, hung at a slant.
    hat(m, (-0.36, -0.11, arm_z + 0.08), tilt=(4.0, -10.0, 15.0), look='crepe', band='crepe')
    hat(m, (0.4, -0.11, arm_z + 0.08), tilt=(-6.0, 12.0, -25.0), look='hay', band='crepe', scale=1.04)
    # The wreath on the post's front: a ring of evergreen sprigs round a core, a crepe bow with tails at its foot.
    wz, wr = 1.06, 0.2
    # The core: a smooth ring of greenery (a torus of 10 x 4), the sprigs stuck in it.
    tb = kit._new_bmesh()
    rings = []
    for i in range(10):
        a = 2.0 * math.pi * i / 10
        radial = Vector((math.cos(a), 0.0, math.sin(a)))
        center = Vector((0.0, -0.12, wz)) + radial * wr
        rings.append([tb.verts.new(center + radial * (0.04 * math.cos(b)) + Vector((0.0, -0.035 * math.sin(b), 0.0)))
                      for b in (math.pi * (0.25 + 0.5 * j) for j in range(4))])
    for i in range(10):
        r0, r1 = rings[i], rings[(i + 1) % 10]
        for j in range(4):
            tb.faces.new((r0[j], r1[j], r1[(j + 1) % 4], r0[(j + 1) % 4]))
    bmesh.ops.recalc_face_normals(tb, faces=tb.faces[:])
    m.emit(tb, Swatch('GrassDeep'), 'foliage', None, smooth=True)
    # Sprigs of evergreen in two rings, overlapping, each a little cupped leaf pointing round the wreath one way.
    sprigs = 34
    for k in range(sprigs):
        a = 2.0 * math.pi * (k + rng.uniform(-0.3, 0.3)) / sprigs
        radial = Vector((math.cos(a), 0.0, math.sin(a)))
        tangent = Vector((-math.sin(a), 0.0, math.cos(a)))
        ring_r = wr + (0.035 if k % 2 else -0.035) + rng.uniform(-0.01, 0.01)
        base = Vector((0.0, -0.155 - rng.uniform(0.0, 0.025), wz)) + radial * ring_r
        tip = base + tangent * rng.uniform(0.09, 0.12) + radial * rng.uniform(-0.02, 0.03) + Vector((0.0, -0.015, 0.0))
        mid = base.lerp(tip, 0.45) + Vector((0.0, -0.012, 0.0))
        side = radial * rng.uniform(0.026, 0.034)
        tb = kit._new_bmesh()
        verts = [tb.verts.new(p) for p in (base, mid - side, tip, mid + side)]
        tb.faces.new(verts)
        m.emit(tb, Swatch(rng.choice(('Fern', 'GrassDeep', 'CloverDark', 'Moss'))), 'foliage', None)
    town.crepe_bow(m, None, (0.0, 0.0, wz - wr - 0.02), scale=0.8, tails=0.32, y=-0.2)
    # Larkspur laid at the foot: two bunches of blue spikes tied with crepe.
    for k, (x, y, yaw) in enumerate(((-0.42, -0.3, 30.0), (0.36, -0.36, -40.0))):
        turn = Matrix.Rotation(math.radians(yaw), 3, 'Z')
        for j in range(3):
            spread = math.radians(-10.0 + 10.0 * j + rng.uniform(-3.0, 3.0))
            d = turn @ Vector((math.cos(spread), math.sin(spread), 0.0))
            n = turn @ Vector((-math.sin(spread), math.cos(spread), 0.0))
            base = Vector((x, y, 0.04)) + Vector((0.0, 0.0, 0.01 * j))
            tip = base + d * rng.uniform(0.36, 0.46) + Vector((0.0, 0.0, 0.06))
            mid = base.lerp(tip, 0.45)
            for p0, p1, w0, w1, swatch in ((base, mid, 0.008, 0.008, 'Stem'), (mid, tip, 0.035, 0.012, 'FlowerBlue')):
                tb = kit._new_bmesh()
                up = Vector((0.0, 0.0, 1.0))
                verts = [tb.verts.new(p0 - n * w0 + up * 0.002), tb.verts.new(p1 - n * w1 + up * 0.03),
                         tb.verts.new(p1 + n * w1 + up * 0.03), tb.verts.new(p0 + n * w0 + up * 0.002)]
                tb.faces.new(verts)
                m.emit(tb, Swatch(swatch), 'foliage', None)
    m.hull((0.8, 0.8, 0.4), at=(0.0, 0.0, 0.18))
    m.hull((0.18, 0.18, top), at=(0.03, 0.0, top * 0.5))
    m.hull((0.95, 0.3, 0.35), at=(0.02, -0.08, arm_z + 0.1))
    obj = m.finish(ao=False, preview=False, **PROPS)
    # The wreath and the flowers are on the foliage master: no wind for them (R = 0), then the baked occlusion.
    lt.set_foliage_colors(obj, wind=0.0, variation='island', seed=seed)
    lt.bake_vertex_ao(obj)
    return obj


models = [
    boardwalk('Boardwalk_4m', 4.0, 501),
    boardwalk('Boardwalk_2m', 2.0, 502),
    steps('Boardwalk_Steps', 503),
    corner('Boardwalk_Corner', 504),
    awning('Awning_4m', 505, 'shakes'),
    awning('Awning_4m_Boards', 506, 'boards'),
    awning_end('Awning_Post', 507),
    hitch_rail('HitchRail', 508),
    notice_board('NoticeBoard', 509),
    memorial('TownMemorial', 510),
]

if lt.want_preview():
    views = {'HitchRail': (-0.6, -1.6, 0.45), 'NoticeBoard': (-0.7, -1.6, 0.3), 'TownMemorial': (-0.6, -1.6, 0.35),
             'Awning_Post': (-1.0, -1.6, 0.4)}
    for obj in models:
        lt.preview([obj], lt.preview_path(PREVIEW, obj.name), view=views.get(obj.name, (-1.0, -1.6, 0.7)), fit=0.9,
                   ground_at='origin')
