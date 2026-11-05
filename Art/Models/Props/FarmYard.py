"""Farmyard implements for Ransom's Rest's two farms (Docs/Work/2026-11-04-Towns/PLAN.md 4.5, "FarmYard.py"; the PROPS
lane, round 2): the Ransom farmyard between the farmhouse, barn and well (the kettle, churn, trough and rake) and Amos
Whitlock's empty barn yard (plough, harrow, trough, grindstone; TownClutter's WagonFreight_Broken). A scripted model
file (Art/README.md); Tools/models.ps1 exports it into /Game/Art/Props (SM_<name>). Built with looter_clutter on the
house trim sheet as a palette (weathered wood, oxide paint, iron) as the town's clutter is, the big iron on IronBlack
(MetalWorn, 1024 px/m) and the plough's share and mouldboard on SteelWorn (MetalWorn tinted 0x8e8c88, new), the
grindstone's wheel on Sandstone (RockCliff tinted 0xd9c9a6, new); TownClutter.py's wheels, sheets, barrels and heaps
are reused.

  Plough        a walking plough parked on its share: an oxide-red beam, a clevis at its nose, a gauge wheel, the steel
                share, landside and a twisted mouldboard turning the furrow to the right, two long handles with a
                rung, raked back to 1.0 m. 0.6 x 2.2 x 1.0 m; its beam runs along Y, the nose toward -Y.
  DragHarrow    a timber drag harrow lying flat on its iron teeth: a 1.6 m square of five bars joined by three cross
                bars, a draw chain at its front edge. 1.6 x 1.6 x 0.3 m.
  HayRake       a horse dump rake: two tall iron-spoked wheels (1.3 m) in faded oxide, the axle bar across, a row of
                curved iron tines, a spring seat on its stem, the shafts down in front. 2.8 x 1.6 x 1.3 m.
  FeedTrough    a V trough of two boards on X legs, the ends boarded, hay and spilt feed in it. 2.4 x 0.6 x 0.5 m.
  ButterChurn   a tall dash churn: tapering staves in three hoops, its lid, the dasher's handle standing up through
                it. 0.45 x 0.45 x 1.05 m.
  WashKettle    a cast-iron wash kettle on three stubby legs hung by a chain from a tripod of three poles over a cold
                fire pit (a ring of fieldstones, ash, charred ends). SOCKET_Fire at the pit (the user left Delia's
                kettle unlit, D3; the socket is there should that change). 1.2 x 1.2 x 1.3 m.
  Grindstone    a sandstone wheel on its iron axle in a four-legged timber frame, a crank at one end, a water pan under
                the wheel. 1.0 x 0.5 x 1.0 m.

Pivots: the ground under each model's middle, front -Y. Collision: UCX hulls round what stands (the kettle's tripod and
pit, the churn, the trough, the grindstone, the plough's beam and handles, the rake's wheels and axle); the harrow is a
low hull the player steps over. Small props (Nanite off), LODs 50% and 25% from 600 triangles; the rake keeps Nanite
with all its triangles in the fallback (its wheels' spokes pass 2.5k).

    Tools\\artrun.ps1 -Script Art\\Models\\Props\\FarmYard.py -Preview      every model and a lineup into
                                    Saved/ArtPreviews/TownProps/; '--only=Name,Name' builds just those
    Tools\\models.ps1 -Only FarmYard -Out Intermediate\\ArtExport_TownProps -NoImport          export test
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
from looter_buildings import Trim
import looter_clutter as lc
import looter_homes as homes
import looter_textures as lt

ARGS = lc.ARGS
ONLY = next((a.split('=', 1)[1].split(',') for a in ARGS if a.startswith('--only=')), None)
UP = Vector((0.0, 0.0, 1.0))
TAU = 2.0 * math.pi


def _sibling(name, module):
    sys.dont_write_bytecode = True
    path = os.path.join(lt.REPO, 'Art', 'Models', 'Props', name + '.py')
    spec = importlib.util.spec_from_file_location(module, path)
    mod = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(mod)
    return mod


TC = _sibling('TownClutter', 'town_clutter_parts')
FC = TC.FC
lc.PREVIEW_DIR = os.path.join(lt.PREVIEW_DIR, 'TownProps')
lc.REPORT_PATH = os.path.join(lc.PREVIEW_DIR, 'FarmYard.report.json')
IRON = lt.material('MetalWorn', name='IronBlack', tint=0x2e2c2a)
SANDSTONE = lt.material('RockCliff', name='Sandstone', tint=0xd9c9a6)
STEEL = lt.material('MetalWorn', name='SteelWorn', tint=0x8e8c88)   # the plough's share and mouldboard: worn bright steel
P, D = TC.P, TC.D


def iron_sheet(m, rows, thick, seed):
    TC.sheet(m, rows, thick, 'IronBlack', seed, density=1024.0)


# --- Plough ---

def plough(name, seed):
    m = lc.Model(name, seed)
    rng = m.rng
    # The beam: from its nose (the clevis) back over the share to the standard and on to the handles.
    nose = Vector((0.0, -1.08, 0.5))
    knee = Vector((0.0, -0.3, 0.62))
    heel = Vector((0.0, 0.25, 0.42))
    lc.batten(m, nose, knee, (0.09, 0.07), face=(1.0, 0.0, 0.0), strip='H2', chamfer=0.01)
    lc.batten(m, knee, heel, (0.09, 0.07), face=(1.0, 0.0, 0.0), strip='H2', chamfer=0.01)
    # The standard down to the share, the share and landside on the ground, the mouldboard curling to the right.
    lc.batten(m, (0.0, -0.3, 0.62), (0.0, -0.18, 0.06), (0.08, 0.05), face=(1.0, 0.0, 0.0), key='SteelWorn',
              density=1024.0, chamfer=0.006)
    lc.bar(m, (0.0, -0.52, 0.02), (0.02, 0.15, 0.03), 0.07, 0.016, face=(-1.0, 0.0, 0.0), key='SteelWorn', density=1024.0,
           chamfer=0.003)
    # The share: a pointed blade along the ground at the mouldboard's foot, its point forward (-Y) at the landside.
    share = [[Vector((0.0, -0.6, 0.006)), Vector((0.0, -0.52, 0.006)), Vector((0.0, -0.36, 0.008)), Vector((0.0, -0.2, 0.01))],
             [Vector((0.02, -0.56, 0.018)), Vector((0.1, -0.46, 0.016)), Vector((0.2, -0.32, 0.016)),
              Vector((0.26, -0.18, 0.018))]]
    TC.sheet(m, share, 0.01, 'SteelWorn', seed + 1, density=1024.0)
    # The mouldboard: a twisted plate that lifts the furrow slice off the share and turns it over to the right. Each
    # section across it (from its foot on the share up to its top edge) leans further out the further back it is,
    # and is dished; its front edge stands nearly upright at the landside.
    rows = []
    for j in range(7):
        u = j / 6
        y = -0.5 + 0.66 * u
        lean = math.radians(18.0 + 62.0 * u ** 1.3)
        foot = Vector((0.02 + 0.24 * u ** 1.5, y, 0.02))
        height = 0.34 - 0.06 * u
        row = []
        for i in range(6):
            v = i / 5
            reach = height * v
            dish = 0.035 * math.sin(math.pi * v) * (0.5 + 0.5 * u)
            row.append(foot + Vector((math.sin(lean) * reach - dish * math.cos(lean), 0.04 * v * u,
                                      math.cos(lean) * reach + dish * math.sin(lean) * 0.5)))
        rows.append(row)
    TC.sheet(m, rows, 0.008, 'SteelWorn', seed + 2, density=1024.0)
    # The handles: two long bars from the beam's heel and the mouldboard's back, raked up and out; a rung and a brace.
    for s, base in ((-1.0, Vector((-0.02, 0.2, 0.42))), (1.0, Vector((0.2, 0.12, 0.2)))):
        top = Vector((s * 0.3, 1.05, 1.0))
        mid = base.lerp(top, 0.5) + Vector((0.0, 0.0, 0.04))
        lc.rod(m, lc.spline([base, mid, top], per=3), 0.022, strip='C', sides=7,
               radii=[0.026, 0.025, 0.024, 0.023, 0.022, 0.021, 0.02])
        lc.turned(m, [(0.0, 0.0), (0.024, 0.0), (0.026, 0.08), (0.02, 0.1), (0.0, 0.11)], 7, at=top,
                  rot=UP.rotation_difference((top - mid).normalized()).to_matrix(), strip='C', grain='up')
    lc.rod(m, [(-0.19, 0.72, 0.78), (0.2, 0.7, 0.7)], 0.014, strip='C', sides=6)
    lc.rod(m, [(0.0, 0.25, 0.42), (0.1, 0.55, 0.6)], 0.01, strip='H3', sides=5)
    # The clevis at the nose and a small gauge wheel under it.
    lc.rod(m, [nose + Vector((0.0, 0.02, 0.0)), nose + Vector((0.0, -0.08, -0.04)), nose + Vector((0.0, -0.12, 0.03))],
           0.011, strip='H3', sides=5)
    lc.rod(m, [nose + Vector((0.0, 0.05, -0.01)), Vector((0.04, -0.98, 0.16))], 0.014, strip='H3', sides=5)
    TC.wheel(m, Matrix.Identity(4), (0.06, -0.98, 0.15), 0.15, 8, seed + 3, width=0.035, felloe=0.03, hub=0.03,
             hub_len=0.08, segs=12, paint='H3')
    m.hull_points([Vector((x, y, z)) for x in (-0.06, 0.24) for y, z in ((-1.15, 0.0), (-1.15, 0.6), (0.3, 0.0),
                                                                       (0.3, 0.7))])
    m.hull_points([Vector((x, y, z)) for x in (-0.33, 0.33) for y, z in ((0.3, 0.4), (1.1, 0.95), (1.1, 1.05), (0.3, 0.5))])
    m.center_footprint()
    return m.done('the ground under its middle (its beam along Y, the nose toward -Y)', 'A walking plough parked on its '
                  'share: red beam,\niron mouldboard, long handles.', ao=0.4)


# --- DragHarrow ---

def drag_harrow(name, seed):
    m = lc.Model(name, seed)
    rng = m.rng
    teeth_h = 0.12
    z = teeth_h + 0.05
    xs = [-0.72 + 1.44 * k / 4 for k in range(5)]
    for k, x in enumerate(xs):
        lc.batten(m, (x + rng.uniform(-0.01, 0.01), -0.8, z), (x, 0.8, z + rng.uniform(-0.01, 0.01)), (0.09, 0.1),
                  face=(0.0, 0.0, 1.0), strip='C', chamfer=0.01, twist=rng.uniform(-2.0, 2.0))
        for j in range(7):
            y = -0.7 + 1.4 * (j + 0.5 * (k % 2)) / 7
            # A square iron tooth through the bar, its point in the dirt, its head on top.
            lc.batten(m, (x, y, z + 0.06), (x + 0.01, y + 0.03, 0.0), (0.016, 0.016), face=(0.0, -1.0, 0.0), strip='H3',
                      chamfer=0.002, taper=0.4)
    for y in (-0.62, 0.0, 0.62):
        lc.batten(m, (-0.82, y, z + 0.085), (0.82, y + rng.uniform(-0.02, 0.02), z + 0.085), (0.07, 0.06),
                  face=(0.0, 0.0, 1.0), strip='C', chamfer=0.008)
    lc.nail_heads(m, [(x, y, z + 0.116) for x in xs for y in (-0.62, 0.0, 0.62)], (0.0, 0.0, 1.0), size=0.02)
    # The draw chain from the front corners to a ring, lying on the ground in front.
    ring = Vector((0.0, -1.0, 0.012))
    for s in (-1.0, 1.0):
        a = Vector((s * 0.72, -0.82, z))
        pts = [a, a.lerp(ring, 0.5) + Vector((0.0, 0.0, -0.06)), ring]
        pts = lc.spline(pts, per=5)
        for k in range(len(pts) - 1):
            lc.rod(m, [pts[k], pts[k + 1]], 0.008, strip='H3', sides=4, caps=(False, False))
    lc.turned(m, [(0.035, -0.006), (0.045, 0.0), (0.035, 0.006), (0.025, 0.0)], 10, at=ring, strip='H3', grain='around')
    m.hull((1.66, 1.64, z + 0.12), at=(0.0, 0.0, (z + 0.12) * 0.5))
    return m.done('the ground under its middle', 'A timber drag harrow lying on its iron teeth,\nits draw chain in front.',
                  ao=0.4)


# --- HayRake ---

def hay_rake(name, seed):
    m = lc.Model(name, seed)
    rng = m.rng
    R = 0.65
    frame = Matrix.Identity(4)
    for s in (-1.0, 1.0):
        TC.wheel(m, frame, (s * 1.32, 0.15, R), R, 14, seed + int(s + 2), width=0.05, felloe=0.035, hub=0.07,
                 hub_len=0.18, segs=22, paint='H2')
    lc.rod(m, [(-1.42, 0.15, R), (1.42, 0.15, R)], 0.028, strip='H3', sides=8)
    lc.batten(m, (-1.2, 0.05, R + 0.08), (1.2, 0.05, R + 0.08), (0.11, 0.09), face=(0.0, -1.0, 0.0), strip='H2',
              chamfer=0.01)
    # The tines: curved iron rods from the tine bar under the axle, curling forward to the ground.
    count = 22
    for k in range(count):
        x = -1.15 + 2.3 * k / (count - 1)
        root = Vector((x, 0.12, R - 0.06))
        pts = [root, Vector((x, 0.32, R - 0.3)), Vector((x, 0.38, 0.22)), Vector((x, 0.26, 0.04)),
               Vector((x, 0.14, 0.015))]
        lc.rod(m, lc.spline(pts, per=2), 0.008, strip='H3', sides=4, radii=[0.009] * 6 + [0.008, 0.007, 0.004])
    lc.batten(m, (-1.18, 0.12, R - 0.06), (1.18, 0.12, R - 0.06), (0.05, 0.04), face=(0.0, -1.0, 0.0), strip='H3',
              chamfer=0.004)
    # The seat on its spring stem, the dump lever, the shafts down in front.
    stem = [Vector((0.0, 0.05, R + 0.12)), Vector((0.0, 0.3, R + 0.35)), Vector((0.0, 0.42, R + 0.52))]
    lc.tube(m, lc.spline(stem, per=3), lc.rect(0.05, 0.012), strip='H3', up=(1.0, 0.0, 0.0))
    seat_c = Vector((0.0, 0.46, R + 0.55))
    lc.turned(m, [(0.0, -0.01), (0.17, -0.005), (0.2, 0.02), (0.19, 0.04), (0.16, 0.03), (0.0, 0.02)], 14, at=seat_c,
              rot=Matrix.Rotation(math.radians(-6.0), 3, 'X'), strip='H2', grain='around')
    lc.rod(m, [(0.35, 0.08, R + 0.1), (0.38, 0.3, R + 0.62)], 0.012, strip='H3', sides=5)
    for s in (-1.0, 1.0):
        pts = [Vector((s * 0.36, 0.05, R + 0.08)), Vector((s * 0.33, -0.5, 0.42)), Vector((s * 0.3, -1.15, 0.1)),
               Vector((s * 0.29, -1.38, 0.05))]
        lc.rod(m, lc.spline(pts, per=3), 0.03, strip='C', sides=6, radii=[0.034] * 4 + [0.031, 0.029, 0.027, 0.025, 0.024,
                                                                                        0.023])
    lc.rod(m, [(-0.33, -0.55, 0.4), (0.33, -0.55, 0.4)], 0.02, strip='C', sides=6)
    for s in (-1.0, 1.0):
        TC.wheel_hull(m, frame, (s * 1.32, 0.15, R), R)
    m.hull((2.4, 0.5, R + 0.15), at=(0.0, 0.2, (R + 0.15) * 0.5))
    m.center_footprint()
    return m.done('the ground under its middle (the axle along X, the shafts toward -Y)', 'A horse dump rake: tall red '
                  'iron wheels, a row\nof curved tines, a spring seat.', ao=0.45, nanite=True, fallback=100.0)


# --- FeedTrough ---

def feed_trough(name, seed):
    m = lc.Model(name, seed)
    rng = m.rng
    L, top = 2.4, 0.5
    floor = 0.22
    for s in (-1.0, 1.0):
        # Each side two boards: their feet meet in the V, their tops lean out.
        a = Vector((0.0, s * 0.02, floor))
        b = Vector((0.0, s * 0.27, top))
        out = Vector((0.0, s * (top - floor), -(0.25))).normalized()
        for k in range(2):
            p, q = a.lerp(b, k / 2), a.lerp(b, (k + 1) / 2)
            mid = (p + q) * 0.5 + out * 0.015
            lc.bar(m, Vector((-L * 0.5, mid.y, mid.z)), Vector((L * 0.5, mid.y, mid.z)), (q - p).length - 0.004, 0.03,
                   face=out, strip='A', chamfer=0.006, twist=rng.uniform(-1.0, 1.0), segments=3, nicks=0.003)
    for s in (-1.0, 1.0):
        x = s * (L * 0.5 - 0.06)
        outline = [(-0.3, top + 0.02), (-0.03, floor - 0.03), (0.03, floor - 0.03), (0.3, top + 0.02)]
        lc.prism_part(m, [(y, z) for y, z in outline], 0.03, kit.basis((x - s * 0.015, 0.0, 0.0), (0.0, s, 0.0),
                      (-s, 0.0, 0.0), (0.0, 0.0, 1.0)), uv=Trim('A', rotate=True))
        # The X legs.
        for t in (-1.0, 1.0):
            lc.batten(m, (x + s * 0.05, t * 0.3, 0.0), (x + s * 0.05, -t * 0.16, top - 0.06), (0.07, 0.045),
                      face=(s, 0.0, 0.0), strip='C', chamfer=0.006)
    lc.batten(m, (-L * 0.5 + 0.12, 0.0, 0.12), (L * 0.5 - 0.12, 0.0, 0.12), (0.07, 0.05), face=(0.0, -1.0, 0.0),
              strip='C', chamfer=0.006)
    # Hay and spilt feed in the V, a little over its ends.
    rows = []
    for j in range(4):
        y = -0.2 + 0.4 * j / 3
        row = []
        for i in range(9):
            x = -L * 0.5 + 0.12 + (L - 0.24) * i / 8
            z = floor + 0.06 + 0.16 * abs(y) / 0.2 * 0.6 + 0.03 * noise.noise(Vector((x * 4.0, y * 6.0, 2.0)))
            row.append((x, y, z))
        rows.append(row)
    sc = lt.SETS['Hay']['density'] / float(lt.SETS['Hay']['size'])
    cos, faces = [], []
    for row in rows:
        cos += [Vector(p) for p in row]
    for j in range(3):
        for i in range(8):
            a = j * 9 + i
            idx = lc._orient(cos, [a, a + 1, a + 10, a + 9], UP)
            faces.append((idx, [(cos[k].x * sc, cos[k].y * sc) for k in idx]))
    m.put(cos, faces, key='hay', ao_floor=0.3)
    TC.mound(m, Vector((0.6, -0.42, 0.0)), 0.18, 0.02, seed, key='hay', squash=1.5)
    m.hull((L, 0.62, top + 0.02), at=(0.0, 0.0, (top + 0.02) * 0.5))
    return m.done('the ground under its middle', 'A V trough of two boards on X legs, hay in it\nand spilt at its foot.',
                  ao=0.4)


# --- ButterChurn ---

def butter_churn(name, seed):
    m = lc.Model(name, seed)
    H = 0.78
    r_bot, r_top = 0.2, 0.15
    rows = [0.0, 0.2, 0.45, 0.7, 1.0]
    lc.vessel(m, (0.0, 0.0, 0.0), r_bot, r_top, H, 14, wall=0.016, floor=0.04, hoops=(0.06, H * 0.5, H - 0.07),
              bottom=True)
    # The lid sitting down in the mouth, the dasher's handle up through it.
    lc.turned(m, [(0.0, H - 0.01), (r_top - 0.02, H - 0.01), (r_top - 0.01, H + 0.012), (0.06, H + 0.025), (0.0, H + 0.03)],
              14, at=(0.0, 0.0, 0.0), strip='A', grain='around')
    lc.rod(m, [(0.0, 0.0, H - 0.2), (0.012, 0.0, H + 0.27)], 0.016, strip='C', sides=7)
    lc.turned(m, [(0.0, 0.0), (0.02, 0.0), (0.024, 0.02), (0.0, 0.03)], 7, at=(0.012, 0.0, H + 0.26), strip='C', grain='up')
    FC.ring_hull(m, (0.0, 0.0, 0.0), r_bot + 0.01, 0.0, H + 0.03)
    return m.done('the ground under its middle', 'A tall dash churn in three hoops, its lid and\nthe dasher\'s handle.',
                  ao=0.3)


# --- WashKettle ---

def wash_kettle(name, seed):
    m = lc.Model(name, seed)
    rng = m.rng
    # A ring of fieldstones round a cold pit: ash, two charred ends.
    for k in range(9):
        a = TAU * k / 9 + rng.uniform(-0.15, 0.15)
        r = 0.38 + rng.uniform(-0.03, 0.03)
        c = Vector((math.cos(a) * r, math.sin(a) * r, 0.0))
        size = rng.uniform(0.14, 0.2)
        lc.turned(m, [(0.0, -0.02), (size * 0.5, -0.02), (size * 0.58, size * 0.25), (size * 0.42, size * 0.55),
                      (0.0, size * 0.62)], 6, at=c, rot=Matrix.Rotation(rng.uniform(0.0, 6.0), 3, 'Z')
                  @ Matrix.Rotation(math.radians(rng.uniform(-12.0, 12.0)), 3, 'X'), strip='D', grain='around',
                  phase=rng.uniform(0.0, 1.0))
    lc.turned(m, [(0.33, -0.01), (0.33, 0.012), (0.18, 0.025), (0.0, 0.03)], 12, at=(0.0, 0.0, 0.0), key='charcoal',
              grain='around', set_name='BarkOak')
    for k in range(2):
        a = rng.uniform(0.0, TAU)
        d = Vector((math.cos(a), math.sin(a), 0.0))
        lc.batten(m, d * 0.3 + UP * 0.03, -d * 0.05 + UP * 0.07, (0.07, 0.05), face=(0.0, 0.0, 1.0), key='charcoal',
                  chamfer=0.008, taper=0.6)
    m.socket('Fire', (0.0, 0.0, 0.06))
    # The tripod: three peeled poles leaning in to a lashing over the pit.
    apex = Vector((0.0, 0.0, 1.26))
    for k in range(3):
        a = TAU * k / 3 + 0.4
        foot = Vector((math.cos(a) * 0.62, math.sin(a) * 0.62, -0.05))
        tip = apex + (apex - foot).normalized() * 0.08
        lc.rod(m, [foot, apex, tip], 0.035, strip='B', sides=7, radii=[0.038, 0.032, 0.03])
    lc.turned(m, [(0.045, -0.04), (0.05, -0.02), (0.05, 0.02), (0.045, 0.04)], 8, at=apex, strip='G', grain='around')
    # The chain and the kettle on it.
    kz = 0.24
    links = 9
    for k in range(links):
        t0 = k / links
        a = apex.lerp(Vector((0.0, 0.0, kz + 0.52)), t0)
        b = apex.lerp(Vector((0.0, 0.0, kz + 0.52)), (k + 1) / links)
        lc.rod(m, [a, b], 0.008, strip='H3', sides=4, caps=(False, False))
    prof = [(0.0, 0.03), (0.12, 0.03), (0.24, 0.07), (0.3, 0.16), (0.31, 0.26), (0.29, 0.36), (0.27, 0.41), (0.3, 0.43),
            (0.3, 0.45), (0.27, 0.44), (0.25, 0.38), (0.0, 0.38)]
    lc.turned(m, prof, 18, at=(0.0, 0.0, kz - 0.03), key='ironblack', grain='around', set_name='MetalWorn',
              density=1024.0)
    lc.disc(m, Vector((0.0, 0.0, kz + 0.352)), 0.252, 18, strip='H4')
    for k in range(3):
        a = TAU * k / 3 + 0.2
        top = Vector((math.cos(a) * 0.17, math.sin(a) * 0.17, kz + 0.05))
        lc.rod(m, [top, top + Vector((math.cos(a) * 0.03, math.sin(a) * 0.03, -0.13))], 0.018, key='ironblack', sides=6,
               set_name='MetalWorn', density=1024.0, radii=[0.02, 0.012])
    bail = [Vector((math.cos(t) * 0.31, 0.0, kz + 0.4 + math.sin(t) * 0.12)) for t in
            (math.radians(a) for a in (0, 30, 60, 90, 120, 150, 180))]
    lc.rod(m, bail, 0.007, strip='H3', sides=4)
    for s in (-1.0, 1.0):
        m.box((0.03, 0.04, 0.06), at=(s * 0.305, 0.0, kz + 0.39), uv='H3')
    m.hull_points([Vector((math.cos(a) * 0.32, math.sin(a) * 0.32, z)) for a in (k * TAU / 8 for k in range(8))
                   for z in (kz - 0.12, kz + 0.43)])
    m.hull_points([Vector((math.cos(a) * 0.5, math.sin(a) * 0.5, z)) for a in (k * TAU / 8 for k in range(8))
                   for z in (0.0, 0.14)])
    return m.done('the ground under the pit\'s middle (SOCKET_Fire in the pit)', 'A cast-iron wash kettle hung from a '
                  'tripod over\na cold fire pit.', ao=0.45)


# --- Grindstone ---

def grindstone(name, seed):
    m = lc.Model(name, seed)
    rng = m.rng
    R, W = 0.3, 0.1
    cz = 0.68
    # The frame: four splayed legs, two side rails, two cross pieces.
    for sx in (-1.0, 1.0):
        for sy in (-1.0, 1.0):
            foot = Vector((sx * 0.45, sy * 0.24, 0.0))
            head = Vector((sx * 0.38, sy * 0.15, cz - 0.04))
            lc.batten(m, foot, head, (0.06, 0.06), face=(sx, 0.0, 0.0), strip='C', chamfer=0.007)
        lc.batten(m, (sx * 0.42, -0.2, 0.25), (sx * 0.42, 0.2, 0.25), (0.05, 0.04), face=(sx, 0.0, 0.0), strip='C',
                  chamfer=0.005)
    for sy in (-1.0, 1.0):
        lc.batten(m, (-0.46, sy * 0.15, cz - 0.02), (0.46, sy * 0.15, cz - 0.02), (0.08, 0.06), face=(0.0, sy, 0.0),
                  strip='C', chamfer=0.008)
    # The stone wheel on its axle (along X), worn hollow at its rim.
    rot = Matrix.Rotation(math.radians(90.0), 3, 'Y')
    prof = [(0.0, -W * 0.5), (R * 0.9, -W * 0.5), (R, -W * 0.38), (R - 0.006, 0.0), (R, W * 0.38), (R * 0.9, W * 0.5),
            (0.0, W * 0.5)]
    lc.turned(m, prof, 22, at=(0.0, 0.0, cz + 0.02), rot=rot, key='Sandstone', grain='around', set_name='RockCliff',
              density=640.0)
    lc.rod(m, [(-0.56, 0.0, cz + 0.02), (0.5, 0.0, cz + 0.02)], 0.015, strip='H3', sides=6)
    crank = [Vector((-0.56, 0.0, cz + 0.02)), Vector((-0.56, 0.0, cz - 0.13)), Vector((-0.66, 0.0, cz - 0.13))]
    lc.rod(m, crank, 0.012, strip='H3', sides=5)
    lc.rod(m, [(-0.62, 0.0, cz - 0.13), (-0.74, 0.0, cz - 0.13)], 0.018, strip='C', sides=6)
    # The water pan under the wheel's rim.
    lc.vessel(m, (0.0, 0.0, cz - R - 0.1), 0.14, 0.16, 0.1, 10, wall=0.01, floor=0.015, hoops=(), water=0.07, bottom=True)
    lc.batten(m, (-0.43, -0.14, cz - R - 0.12), (0.43, -0.14, cz - R - 0.12), (0.05, 0.03), face=(0.0, 0.0, 1.0),
              strip='C', chamfer=0.005)
    lc.batten(m, (-0.43, 0.14, cz - R - 0.12), (0.43, 0.14, cz - R - 0.12), (0.05, 0.03), face=(0.0, 0.0, 1.0),
              strip='C', chamfer=0.005)
    m.hull((0.98, 0.52, cz + R + 0.05), at=(0.0, 0.0, (cz + R + 0.05) * 0.5))
    return m.done('the ground under its middle (the wheel\'s axle along X)', 'A sandstone grindstone in a timber frame,\n'
                  'its crank, a water pan under the rim.', ao=0.4)


BUILDERS = [
    ('Plough', lambda: plough('Plough', 5301)),
    ('DragHarrow', lambda: drag_harrow('DragHarrow', 5302)),
    ('HayRake', lambda: hay_rake('HayRake', 5303)),
    ('FeedTrough', lambda: feed_trough('FeedTrough', 5304)),
    ('ButterChurn', lambda: butter_churn('ButterChurn', 5305)),
    ('WashKettle', lambda: wash_kettle('WashKettle', 5306)),
    ('Grindstone', lambda: grindstone('Grindstone', 5307)),
]


def build():
    return {name: make() for name, make in BUILDERS if ONLY is None or name in ONLY}


VIEWS = {
    'Plough': ((-1.0, -0.55, 0.45), 40.0, (45.0, 24.0), 1.06),
    'HayRake': ((-0.7, -1.0, 0.45), 35.0, (40.0, 22.0), 1.04),
    'ButterChurn': ((-0.6, -1.0, 0.4), 50.0, (40.0, 24.0), 1.15),
    'WashKettle': ((-0.6, -1.0, 0.45), 40.0, (40.0, 24.0), 1.06),
}


def previews(built):
    lc.town_materials()
    lc.stage(samples=32)
    shots = {}
    for name, obj in built.items():
        view, lens, sun, margin = VIEWS.get(name, ((-0.6, -1.0, 0.55), 45.0, (40.0, 24.0), 1.08))
        shots[name] = lc.shoot(name, [obj], view=view, lens=lens, sun=sun, margin=margin, size=(1200, 860))
    if ONLY is None:
        shots['Lineup_FarmYard'] = TC.lineup(built, [['HayRake', 'Plough', 'DragHarrow'],
                                                     ['WashKettle', 'FeedTrough', 'Grindstone', 'ButterChurn']],
                                             'Lineup_FarmYard', size=(2200, 1100))
    lt._log('FARMYARD previews: ' + ', '.join(sorted(shots)))
    return shots


if __name__ == '__main__':
    BUILT = build()
    total = sum(info['triangles'] for info in lc.REPORT.values())
    lt._log(f'FARMYARD: {len(lc.REPORT)} models, {total:,} triangles')
    if lt.want_preview():
        lc.write_report()
        previews(BUILT)
