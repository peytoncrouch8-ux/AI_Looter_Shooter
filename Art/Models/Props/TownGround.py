"""Ground detail for the towns of Ransom's Rest (Docs/Work/2026-11-04-Towns/PLAN.md 4.3, "Mesh strips"; the PROPS
lane): the pieces with height that the terrain's wear term can't paint, for the DRESSING lane's rules to lay along wall
feet, posts, ruts and yards. A scripted model file (Art/README.md); Tools/models.ps1 exports it into /Game/Art/Props
(SM_<name>).

Weeds (FoliagePalette on M_WorldFoliage, one slot; opaque blades and leaves, no alpha; wind by vertex colour, the
looter_plants convention: R the sway weight from 0 at the root, G a phase per blade, A the occlusion):
  WeedFringe_A   short dry grass at a lived house's wall foot: a 2.0 x 0.45 m strip of tufts 0.2-0.45 m tall, thicker
                 against the wall, greener at the drip line, a few seed stalks and plantain leaves in front. Pivot: the
                 middle of the strip's wall edge on the ground; the strip runs along X (x = -1 to +1) out to y = -0.45.
                 The ends thin out, so strips laid end to end (or overlapping) show no seam.
  WeedFringe_B   an empty house's: the same strip gone to tall weeds 0.25-0.8 m: two curly docks (rosettes of broad
                 leaves, rusty seed stalks), a woolly mullein (pale rosette, a tall spike), two thistles (spiny leaves,
                 purple and brown heads) in dry grass. Same pivot and footprint.
  WeedFringe_C   a ring of dry grass 0.6 m across round a post's foot (a 9 cm hole in the middle for a post up to
                 16 cm square), two taller stalks. Pivot: the post's middle on the ground.
Every weed is walk-through and shadowless (Collision 'None'; the dressing sets no shadow), Nanite off, LODs 50% and
25% (WeedFringe: from ~13 m and ~25 m; the plan's cull is 24 m on Medium).

Planks and litter (flat, no collision; the walks' and the trim's wood, the GroundDirt and Hay sets):
  PlankCrossing_A  five 2.4 m planks laid across a rut (1.2 x 2.4 m), bowed into it, mud squeezed up along their edges
                   and splashed on them. The planks run along Y (across the rut, which runs along X). Pivot: the middle
                   on the ground. WoodPlanks (each plank one plank of the walks' texture) and GroundDirt: 2 slots.
  PlankCrossing_B  four 3.0 m planks (1.0 x 3.0 m), one snapped in the middle and its halves sunk into the mud.
  StrawLitter_A    trodden straw on the ground, a ragged 1.6 x 1.2 m mat a few centimetres thick, loose straws round its
                   edge (Hay: 1 slot). Pivot: the middle.
  FallenBoards_A   four weathered boards fallen in a jumble (2.2 x 1.0 m), one split, nails standing out of one (the
                   trim's weathered siding and iron: 1 slot). Pivot: the middle.
The wall-edge and flat pieces sink their edges a centimetre or two under the pivot, so they sit on gentle slopes.

    Tools\\artrun.ps1 -Script Art\\Models\\Props\\TownGround.py -Preview     every model, a lineup and the fringes on a
                                    wall foot (Saved/ArtPreviews/TownProps/); '--only=Name,Name' builds just those
    Tools\\models.ps1 -Only TownGround -Out Intermediate\\ArtExport_TownProps -NoImport          export test
Everything random comes from seeded random.Random and mathutils noise; nothing walks a set of mesh elements.
"""
import importlib.util
import math
import os
import random
import sys

import bpy
from mathutils import Matrix, Vector, noise

import looter_buildings as kit
import looter_clutter as lc
import looter_homes as homes
import looter_plants as pl
import looter_textures as lt
from looter_plants import UP, Plant, blade, horizontal, lerp, petal, up_normal

ARGS = lc.ARGS
ONLY = next((a.split('=', 1)[1].split(',') for a in ARGS if a.startswith('--only=')), None)
TAU = 2.0 * math.pi
lc.PREVIEW_DIR = os.path.join(lt.PREVIEW_DIR, 'TownProps')
lc.REPORT_PATH = os.path.join(lc.PREVIEW_DIR, 'TownGround.report.json')
FOLIAGE = lt.material('FoliagePalette')
WEED_LODS = ('50,25', '0.14,0.075')


def _sibling(name, module):
    sys.dont_write_bytecode = True
    path = os.path.join(lt.REPO, 'Art', 'Models', 'Props', name + '.py')
    spec = importlib.util.spec_from_file_location(module, path)
    mod = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(mod)
    return mod


# --- Weeds ---

def away(rng, spread=1.2):
    """A heading away from the wall (toward -Y), within spread radians either way."""
    return horizontal(-math.pi * 0.5 + rng.uniform(-spread, spread))


def tuft(plant, rng, root, count, height, width, spread, lean, colors, tips=None, dry=0.0, wall=True, segments=2):
    """A tuft of blades rooted round root, leaning out (away from the wall when wall)."""
    shade = up_normal(root, spread + height * lean)
    for i in range(count):
        r = spread * math.sqrt(rng.random())
        a = rng.uniform(0.0, TAU)
        p = root + horizontal(a, r) - UP * 0.015
        if wall:
            p.y = min(p.y, -0.015)
            heading = away(rng, 1.25) if rng.random() < 0.75 else horizontal(a)
            if heading.y > 0.0:
                heading.y = -heading.y
        else:
            heading = horizontal(a + rng.uniform(-0.4, 0.4))
        edge = r / max(spread, 1e-3)
        h = height * rng.uniform(0.65, 1.1) * lerp(1.0, 0.8, edge)
        blade(plant, FOLIAGE, p, heading, h, width * rng.uniform(0.8, 1.2), lean * lerp(0.5, 1.3, edge) *
              rng.uniform(0.7, 1.2), rng.choice(colors), rng, segments, shade, tip=tips if rng.random() < dry else None,
              occlusion=(0.55, 1.0))


def seed(plant, center, direction, length, width, swatch, u, wind):
    """One seed or husk: a small diamond on one point of a swatch (u: 0 its dark root end, 1 its pale tip), so a seed
    head reads as pale grain, not as the swatch's dark root."""
    d = direction.normalized()
    side = d.cross(UP if abs(d.z) < 0.95 else Vector((1.0, 0.0, 0.0))).normalized()
    n = side.cross(d).normalized()
    pts = [center, center + d * length * 0.45 + side * width * 0.5, center + d * length,
           center + d * length * 0.45 - side * width * 0.5]
    verts = [plant.vert(p, wind, (n + UP).normalized(), 0.95) for p in pts]
    uvs = [pl.swatch(swatch, u - 0.04, 0.5), pl.swatch(swatch, u, 1.0), pl.swatch(swatch, u + 0.04, 0.5),
           pl.swatch(swatch, u, 0.0)]
    plant.face(verts, uvs, FOLIAGE)


def seed_stalk(plant, rng, root, height, head='Straw', stem='Stem', lean=0.15, wall=True, u=0.8, seeds=9):
    """A bare stalk ending in a narrow head of seeds (pale grass heads, rusty dock, brown husks)."""
    heading = away(rng, 0.9) if wall else horizontal(rng.uniform(0.0, TAU))
    blade(plant, FOLIAGE, root, heading, height, 0.006, lean, stem, rng, 3, None, taper=0.2, wind=(0.0, 1.0),
          occlusion=(0.6, 1.0))
    flat = Vector((heading.x, heading.y, 0.0)).normalized()

    def at(t):
        return root + flat * (lean * height * t * t) + UP * (height * t * (1.0 - 0.35 * lean * lean * t))
    for k in range(seeds):
        t = 0.74 + 0.25 * k / (seeds - 1)
        out = horizontal(rng.uniform(0.0, TAU))
        seed(plant, at(t), (out * 0.45 + UP).normalized(), 0.028 - 0.012 * k / seeds, 0.01, head,
             u + rng.uniform(-0.08, 0.08), t ** 1.5)


def rosette(plant, rng, root, count, length, width, color, droop=0.25, round_=True, curl=0.0):
    """A ground rosette: broad leaves fanned out round root, lying low (dock, mullein, plantain, thistle)."""
    turn = rng.uniform(0.0, TAU)
    for k in range(count):
        a = turn + TAU * k / count + rng.uniform(-0.25, 0.25)
        d = (horizontal(a) + UP * rng.uniform(0.25, 0.6)).normalized()
        n = (UP - d * d.z).normalized()
        petal(plant, FOLIAGE, root + UP * 0.01, d, length * rng.uniform(0.8, 1.15), width * rng.uniform(0.85, 1.1),
              color, (n + UP * 0.5).normalized(), wind=0.25, occlusion=0.75, cup=curl or -droop, round=round_)


def dock(plant, rng, root, height):
    """Curly dock gone to seed: a rosette of long leaves, two tall stalks thick with rusty seed clusters."""
    rosette(plant, rng, root, 7, 0.26, 0.08, 'GrassOlive', droop=0.18)
    for s in range(2):
        h = height * rng.uniform(0.8, 1.0)
        heading = away(rng, 0.8)
        lean = rng.uniform(0.08, 0.2)
        blade(plant, FOLIAGE, root, heading, h, 0.011, lean, 'Stem', rng, 3, None, taper=0.3, occlusion=(0.55, 1.0))
        flat = Vector((heading.x, heading.y, 0.0)).normalized()
        for k in range(10):
            t = 0.45 + 0.53 * k / 9
            c = root + flat * (lean * h * t * t) + UP * (h * t * (1.0 - 0.35 * lean * lean * t))
            for j in range(3):
                out = horizontal(rng.uniform(0.0, TAU))
                seed(plant, c, (out * 0.7 + UP * 0.5).normalized(), 0.032 * (1.0 - 0.35 * (k / 9)), 0.016,
                     rng.choice(('CattailBrown', 'FruitRed', 'DryTan')), rng.uniform(0.45, 0.7), t ** 1.4)


def mullein(plant, rng, root, height):
    """Woolly mullein: a pale rosette of big soft leaves and one thick spike, its few flowers long gone brown."""
    rosette(plant, rng, root, 8, 0.24, 0.12, 'GrassOlive', droop=0.08)
    heading = away(rng, 0.3)
    blade(plant, FOLIAGE, root, heading, height, 0.03, 0.04, 'GrassOlive', rng, 3, None, taper=0.5,
          occlusion=(0.6, 1.0), tip='DryTan')
    for k in range(12):
        t = 0.55 + 0.43 * k / 11
        c = root + UP * (height * t)
        out = horizontal(TAU * k * 0.38)
        petal(plant, FOLIAGE, c + out * 0.012, (out + UP * 0.4).normalized(), 0.03, 0.022,
              'FlowerYellow' if k % 4 == 1 else 'CattailBrown', (out + UP * 0.3).normalized(), wind=t ** 1.4,
              occlusion=0.95, round=True)
    for k in range(4):
        t = 0.15 + 0.1 * k
        c = root + UP * (height * t)
        out = horizontal(TAU * k / 4 + 0.5)
        petal(plant, FOLIAGE, c, (out + UP * 0.6).normalized(), 0.12, 0.05, 'GrassOlive', (out * -0.2 + UP).normalized(),
              wind=t, occlusion=0.85, round=True)


def thistle(plant, rng, root, height):
    """A thistle: spiny pointed leaves up a stalk, two or three heads, purple and gone to brown."""
    heading = away(rng, 0.9)
    lean = rng.uniform(0.05, 0.15)
    blade(plant, FOLIAGE, root, heading, height, 0.012, lean, 'Stem', rng, 3, None, taper=0.4, occlusion=(0.55, 1.0))
    flat = Vector((heading.x, heading.y, 0.0)).normalized()

    def at(t):
        return root + flat * (lean * height * t * t) + UP * (height * t * (1.0 - 0.35 * lean * lean * t))
    for k in range(7):
        t = 0.08 + 0.62 * k / 6
        out = horizontal(rng.uniform(0.0, TAU))
        petal(plant, FOLIAGE, at(t), (out + UP * 0.25).normalized(), 0.13 * (1.0 - 0.5 * t), 0.035, 'GrassOlive',
              (out * -0.3 + UP).normalized(), wind=t, occlusion=0.8, cup=0.15)
    for k in range(rng.randint(2, 3)):
        t = 0.88 + 0.12 * k / 2
        c = at(t) + horizontal(rng.uniform(0.0, TAU), 0.03 * k)
        color = 'FlowerPurple' if k == 0 else 'CattailBrown'
        for j in range(6):
            out = horizontal(TAU * j / 6 + rng.uniform(-0.3, 0.3))
            petal(plant, FOLIAGE, c, (out * 0.4 + UP).normalized(), 0.032, 0.02, color, (out + UP).normalized(),
                  wind=0.95, occlusion=1.0, round=True)
        for j in range(5):
            out = horizontal(TAU * j / 5)
            petal(plant, FOLIAGE, c, (out * 0.8 - UP * 0.4).normalized(), 0.028, 0.02, 'GrassOlive',
                  (out + UP * 0.2).normalized(), wind=0.95, occlusion=0.9)


def plantain(plant, rng, root):
    rosette(plant, rng, root, 5, 0.09, 0.045, 'GrassDeep', droop=0.05)


def strip_spots(rng, count, length=2.0, depth=0.45, wall_bias=1.6):
    """Root points along a wall-foot strip: spread along X, crowded toward the wall (y near 0), thinning at both
    ends (a fade factor per point)."""
    out = []
    for k in range(count):
        x = -length * 0.5 + length * (k + rng.uniform(0.1, 0.9)) / count
        y = -0.03 - (depth - 0.05) * rng.random() ** wall_bias
        edge = min(1.0, (length * 0.5 - abs(x)) / 0.3)
        out.append((Vector((x, y, 0.0)), max(0.25, edge)))
    return out


def weed_fringe_a(name, seed):
    rng = random.Random(seed)
    plant = Plant(name, seed)
    for root, fade in strip_spots(rng, 26):
        wet = root.y > -0.18                     # the drip line: a little green at the wall
        colors = ('GrassDry', 'Straw', 'GrassYellow', 'GrassOlive') if wet else ('GrassDry', 'Straw', 'DryTan')
        tuft(plant, rng, root, int(9 * fade) + 3, rng.uniform(0.2, 0.42) * (0.6 + 0.4 * fade), 0.012, 0.085, 0.3, colors,
             tips='Straw', dry=0.5)
    for k in range(5):
        x = rng.uniform(-0.8, 0.8)
        seed_stalk(plant, rng, Vector((x, rng.uniform(-0.25, -0.05), -0.01)), rng.uniform(0.38, 0.5),
                   head=rng.choice(('Straw', 'DryTan')))
    for k in range(4):
        plantain(plant, rng, Vector((rng.uniform(-0.85, 0.85), rng.uniform(-0.42, -0.3), 0.0)))
    return finish_weeds(plant, 'the middle of the wall edge on the ground (the strip runs along X out to -Y)',
                        'Short dry grass at a lived house\'s wall foot,\na few seed stalks and plantain.')


def weed_fringe_b(name, seed):
    rng = random.Random(seed)
    plant = Plant(name, seed)
    for root, fade in strip_spots(rng, 22):
        tuft(plant, rng, root, int(8 * fade) + 3, rng.uniform(0.3, 0.55) * (0.6 + 0.4 * fade), 0.013, 0.09, 0.35,
             ('GrassDry', 'Straw', 'DryTan', 'GrassYellow'), tips='DryTan', dry=0.6)
    dock(plant, rng, Vector((-0.62, -0.16, 0.0)), 0.78)
    dock(plant, rng, Vector((0.55, -0.12, 0.0)), 0.66)
    mullein(plant, rng, Vector((0.05, -0.14, 0.0)), 0.8)
    thistle(plant, rng, Vector((-0.2, -0.3, 0.0)), 0.55)
    thistle(plant, rng, Vector((0.82, -0.28, 0.0)), 0.48)
    for k in range(4):
        seed_stalk(plant, rng, Vector((rng.uniform(-0.9, 0.9), rng.uniform(-0.3, -0.05), -0.01)),
                   rng.uniform(0.5, 0.7), head='CattailBrown')
    return finish_weeds(plant, 'the middle of the wall edge on the ground (the strip runs along X out to -Y)',
                        'An empty house\'s wall foot gone to tall weeds:\ndock, mullein, thistle, dry grass.')


def weed_fringe_c(name, seed):
    rng = random.Random(seed)
    plant = Plant(name, seed)
    ring = 9
    for k in range(ring):
        a = TAU * k / ring + rng.uniform(-0.2, 0.2)
        r = rng.uniform(0.13, 0.22)
        root = horizontal(a, r)
        tuft(plant, rng, root, 6, rng.uniform(0.18, 0.38), 0.011, 0.06, 0.35, ('GrassDry', 'Straw', 'GrassYellow',
             'GrassOlive'), tips='Straw', dry=0.5, wall=False)
    for k in range(2):
        a = rng.uniform(0.0, TAU)
        seed_stalk(plant, rng, horizontal(a, 0.12) - UP * 0.01, rng.uniform(0.45, 0.6), wall=False)
    return finish_weeds(plant, 'the post\'s middle on the ground (a 9 cm hole for the post)',
                        'Dry grass in a ring round a post\'s foot, two\ntaller stalks.')


def finish_weeds(plant, pivot, caption):
    obj = pl.finish(plant, None, ao_distance=0.35, ao_blend=0.5, lods=WEED_LODS[0], lod_screens=WEED_LODS[1])
    obj['Collision'] = 'None'
    lc.report(obj, pivot, caption)
    return obj


# --- Planks, straw and boards ---

def mud_rim(m, points, width, seed, rise=0.024, sink=-0.012):
    """Mud squeezed up along a line on the ground (points along it, the mud on its left): a lumpy ridge from the line
    (where it laps onto the plank's side) out to width, sinking under the ground at its outer edge."""
    rng = random.Random(seed)
    pts = [Vector(p) for p in points]
    cos, faces = [], []
    sc = lt.SETS['GroundDirt']['density'] / float(lt.SETS['GroundDirt']['size'])
    off = (rng.uniform(0.0, 3.0), rng.uniform(0.0, 3.0))
    rows = []
    for i, p in enumerate(pts):
        t = (pts[min(i + 1, len(pts) - 1)] - pts[max(i - 1, 0)]).normalized()
        left = Vector((-t.y, t.x, 0.0))
        row = []
        for j, (s, z) in enumerate(((0.0, rise), (0.35, rise * 0.75), (0.7, rise * 0.3), (1.0, sink))):
            lump = 0.008 * noise.noise(Vector((p.x * 9.0, p.y * 9.0, j * 1.3 + seed * 0.01)))
            w = width * (1.0 + 0.35 * noise.noise(Vector((p.x * 5.0, p.y * 5.0, 7.0 + seed * 0.01))))
            q = p + left * (s * w) + UP * (z + (lump if 0 < j < 3 else 0.0))
            row.append(len(cos))
            cos.append(q)
        rows.append(row)
    for i in range(len(rows) - 1):
        for j in range(3):
            idx = [rows[i][j], rows[i + 1][j], rows[i + 1][j + 1], rows[i][j + 1]]
            o = lc._orient(cos, idx, UP)
            faces.append((o, [((cos[k].x + off[0]) * sc, (cos[k].y + off[1]) * sc) for k in o]))
    m.put(cos, faces, key='dirt', ao_floor=0.35)


def splash(m, at, size, seed):
    """A flat splash of dried mud on a plank's top."""
    rng = random.Random(seed)
    c = Vector(at)
    sc = lt.SETS['GroundDirt']['density'] / float(lt.SETS['GroundDirt']['size'])
    cos = [c]
    n = 8
    for k in range(n):
        a = TAU * k / n
        r = size * (0.5 + 0.5 * rng.random())
        cos.append(c + Vector((math.cos(a) * r, math.sin(a) * r * 0.7, -0.0012)))
    faces = []
    for k in range(n):
        idx = lc._orient(cos, [0, 1 + k, 1 + (k + 1) % n], UP)
        faces.append((idx, [((cos[i].x + 0.4) * sc, (cos[i].y + 0.7) * sc) for i in idx]))
    m.put(cos, faces, key='dirt', smooth=False, ao_floor=0.5)


def plank_crossing(name, seed, count, length, width, broken=None):
    m = lc.Model(name, seed)
    rng = m.rng
    T = 0.04
    gap = 0.012
    pw = (width - (count - 1) * gap) / count
    for k in range(count):
        x = -width * 0.5 + pw * 0.5 + k * (pw + gap) + rng.uniform(-0.01, 0.01)
        yaw = rng.uniform(-2.0, 2.0)
        d = Vector((math.sin(math.radians(yaw)), math.cos(math.radians(yaw)), 0.0))
        c = Vector((x, rng.uniform(-0.05, 0.05), T * 0.5 - 0.006))
        sag = rng.uniform(0.012, 0.025)
        if broken == k:
            # Snapped in the middle: each half sinks its broken end into the rut's mud.
            for s in (-1.0, 1.0):
                outer = c + d * (s * length * 0.5)
                inner = c + d * (s * 0.04) - UP * 0.03
                lc.bar(m, outer if s < 0 else inner, inner if s < 0 else outer, pw, T, face=(0.0, 0.0, 1.0),
                       key='planks', chamfer=0.006, twist=rng.uniform(-5.0, 5.0), caps=(True, True), nicks=0.004)
            continue
        lc.bar(m, c - d * length * 0.5, c + d * length * 0.5, pw, T, face=(0.0, 0.0, 1.0), key='planks', chamfer=0.006,
               sag=sag, twist=rng.uniform(-2.0, 2.0), segments=4, nicks=0.002)
        for j in range(rng.randint(1, 3)):
            t = rng.uniform(-0.4, 0.4)
            bend = 1.0 - (2.0 * t) ** 2
            splash(m, c + d * (t * length) + Vector((rng.uniform(-pw * 0.3, pw * 0.3), 0.0, T * 0.5 - sag * bend + 0.0015)),
                   rng.uniform(0.04, 0.09), seed + 31 * k + j)
    # Mud squeezed up along the two outer edges and round the ends.
    for s in (-1.0, 1.0):
        x = s * (width * 0.5 + 0.004)
        line = [Vector((x, -length * 0.5 + length * i / 8, 0.0)) for i in range(9)]
        if s > 0:
            line = list(reversed(line))
        mud_rim(m, line, 0.12, seed + int(s + 2))
    for s in (-1.0, 1.0):
        y = s * (length * 0.5 + 0.01)
        line = [Vector((-width * 0.5 + width * i / 6, y, 0.0)) for i in range(7)]
        if s < 0:
            line = list(reversed(line))
        mud_rim(m, line, 0.1, seed + int(s + 5), rise=0.016)
    return m.done('the middle on the ground (the planks run along Y, across a rut along X)',
                  f'{count} planks laid across a rut, bowed into it,\nmud squeezed up round them.', ao=0.25,
                  collision='None')


def straw_litter(name, seed):
    m = lc.Model(name, seed)
    rng = m.rng
    sc = lt.SETS['Hay']['density'] / float(lt.SETS['Hay']['size'])
    rings = [(0.0, 0.022), (0.35, 0.026), (0.7, 0.016), (0.9, 0.006), (1.0, -0.01)]
    n = 18
    cos = [Vector((0.0, 0.0, 0.02))]
    off = rng.uniform(0.0, 50.0)
    for j, (share, z) in enumerate(rings[1:]):
        for k in range(n):
            a = TAU * k / n
            wob = 1.0 + 0.3 * noise.noise(Vector((math.cos(a) * 1.4, math.sin(a) * 1.4, off)))
            r = share * wob
            h = z + 0.008 * noise.noise(Vector((math.cos(a) * 3.0 * share, math.sin(a) * 3.0 * share, off + 9.0)))
            cos.append(Vector((math.cos(a) * r * 0.8, math.sin(a) * r * 0.6, h if j < 3 else z)))
    faces = []

    def uv(i):
        return ((cos[i].x + off * 0.01) * sc, (cos[i].y + 0.2) * sc)
    for k in range(n):
        idx = lc._orient(cos, [0, 1 + k, 1 + (k + 1) % n], UP)
        faces.append((idx, [uv(i) for i in idx]))
    for j in range(len(rings) - 2):
        for k in range(n):
            k1 = (k + 1) % n
            a, b = 1 + j * n + k, 1 + j * n + k1
            idx = lc._orient(cos, [a, a + n, b + n, b], UP)
            faces.append((idx, [uv(i) for i in idx]))
    m.put(cos, faces, key='hay', ao_floor=0.4)
    # Loose straws scattered over the mat and round its edge.
    for k in range(40):
        a = rng.uniform(0.0, TAU)
        r = 0.88 * math.sqrt(rng.random()) + 0.12 * (k % 2)
        c = Vector((math.cos(a) * r * 0.85, math.sin(a) * r * 0.65, 0.0))
        z = 0.004 + max(0.0, 0.02 * (1.0 - r))
        d = horizontal(rng.uniform(0.0, TAU))
        ln = rng.uniform(0.08, 0.18)
        lc.rod(m, [c - d * ln * 0.5 + UP * z, c + d * ln * 0.5 + UP * (z + rng.uniform(-0.004, 0.006))], 0.0022, key='hay',
               sides=3, caps=(False, False))
    return m.done('the middle on the ground', 'Trodden straw on the ground, loose straws\nround its edge.', ao=0.2,
                  collision='None')


def fallen_boards(name, seed):
    m = lc.Model(name, seed)
    rng = m.rng
    T = 0.022
    # (start, end, width): the first two on the ground crossed, the third over them, the split one, a batten.
    boards = [((-1.0, -0.22), (0.9, 0.06), 0.19), ((-0.7, 0.38), (0.95, -0.28), 0.18),
              ((-0.35, -0.46), (0.75, 0.34), 0.17)]
    tops = []
    for k, ((x0, y0), (x1, y1), w) in enumerate(boards):
        z = T * 0.5 + (0.03 if k == 2 else 0.0)
        lc.bar(m, (x0, y0, z - 0.008), (x1, y1, z + (0.012 if k == 2 else 0.0)), w, T, face=(0.0, 0.0, 1.0), strip='A',
               chamfer=0.005, twist=rng.uniform(-3.0, 3.0), nicks=0.004, segments=3)
        tops.append(((x0, y0), (x1, y1), z))
    # A short split piece, and a nail-bristling batten end.
    lc.bar(m, (0.4, -0.38, T * 0.5), (0.98, -0.5, T * 0.5), 0.09, T, face=(0.0, 0.0, 1.0), strip='A', chamfer=0.004,
           taper=0.6, nicks=0.004)
    lc.bar(m, (-1.05, 0.05, 0.02), (-0.4, 0.42, 0.02), 0.05, 0.04, face=(0.0, 0.0, 1.0), strip='C', chamfer=0.006)
    for k in range(3):
        t = 0.15 + 0.3 * k
        base = Vector((-1.05, 0.05, 0.04)).lerp(Vector((-0.4, 0.42, 0.04)), t)
        lc.rod(m, [base, base + Vector((rng.uniform(-0.01, 0.01), rng.uniform(-0.01, 0.01), 0.045))], 0.0025,
               strip='H3', sides=4)
    (x0, y0), (x1, y1), z = tops[2]
    for k in range(2):
        t = 0.08 + 0.84 * k
        base = Vector((x0 + (x1 - x0) * t, y0 + (y1 - y0) * t, z + T * 0.5 + 0.03))
        lc.rod(m, [base, base + Vector((0.006, -0.004, 0.035))], 0.0025, strip='H3', sides=4)
    return m.done('the middle on the ground', 'Weathered boards fallen in a jumble, one split,\nnails standing out of '
                  'a batten.', ao=0.25, collision='None')


BUILDERS = [
    ('WeedFringe_A', lambda: weed_fringe_a('WeedFringe_A', 5201)),
    ('WeedFringe_B', lambda: weed_fringe_b('WeedFringe_B', 5202)),
    ('WeedFringe_C', lambda: weed_fringe_c('WeedFringe_C', 5203)),
    ('PlankCrossing_A', lambda: plank_crossing('PlankCrossing_A', 5204, 5, 2.4, 1.2)),
    ('PlankCrossing_B', lambda: plank_crossing('PlankCrossing_B', 5205, 4, 3.0, 1.0, broken=2)),
    ('StrawLitter_A', lambda: straw_litter('StrawLitter_A', 5206)),
    ('FallenBoards_A', lambda: fallen_boards('FallenBoards_A', 5207)),
]


def build():
    return {name: make() for name, make in BUILDERS if ONLY is None or name in ONLY}


def previews(built):
    fc = _sibling('FrontierClutter', 'frontier_parts')
    lc.town_materials()
    lc.stage(samples=32)
    shots = {}
    wall = fc.backdrop('_WallTown', 5.0, 2.4, finish='trim')
    post = None
    for name, obj in built.items():
        if name.startswith('WeedFringe_C'):
            m = lc.Model('_Post', 1)
            lc.batten(m, (0.0, 0.0, -0.1), (0.0, 0.0, 1.1), (0.14, 0.14), face=(0.0, -1.0, 0.0), strip='C', chamfer=0.012)
            post = m.done('', '')
            lc.REPORT.pop('_Post', None)
            shots[name] = lc.shoot(name, [obj], [post], view=(-0.6, -1.0, 0.55), lens=45.0, sun=(40.0, 24.0), margin=1.1,
                                   size=(1200, 860))
        elif name.startswith('WeedFringe'):
            shots[name] = lc.shoot(name, [obj], [wall], view=(-0.45, -1.0, 0.35), lens=40.0, sun=(35.0, 24.0),
                                   margin=1.06, size=(1400, 800))
        else:
            shots[name] = lc.shoot(name, [obj], view=(-0.55, -1.0, 0.7), lens=45.0, sun=(40.0, 26.0), margin=1.08,
                                   size=(1200, 860))
    # The fringes laid end to end along a wall's foot, a fallen-board yard in front.
    if ONLY is None:
        staged = []
        for k, x in enumerate((-3.0, -1.0, 1.0, 3.0)):
            n = 'WeedFringe_A' if k < 2 else 'WeedFringe_B'
            if n in built:
                staged.append(lc.copy_of(built[n], (x, 0.0, 0.0)))
        for n, at, turn in (('PlankCrossing_A', (-1.6, -2.4, 0.0), 90.0), ('StrawLitter_A', (0.9, -1.6, 0.0), 0.0),
                            ('FallenBoards_A', (2.4, -2.2, 0.0), 20.0)):
            if n in built:
                staged.append(lc.copy_of(built[n], at, turn))
        figure = homes.figure()
        figure.location = (0.2, -2.6, 0.0)
        bpy.context.view_layer.update()
        shots['Lineup_TownGround'] = lc.shoot('Lineup_TownGround', staged + [figure], [wall], view=(-0.25, -1.0, 0.45),
                                              lens=32.0, size=(2200, 1000), sun=(35.0, 24.0), margin=1.04, bias=0.3)
        lc.clear_copies()
        bpy.data.objects.remove(figure)
    lt._log('GROUND previews: ' + ', '.join(sorted(shots)))
    return shots


if __name__ == '__main__':
    BUILT = build()
    total = sum(info['triangles'] for info in lc.REPORT.values())
    lt._log(f'GROUND: {len(lc.REPORT)} models, {total:,} triangles')
    if lt.want_preview():
        lc.write_report()
        previews(BUILT)
