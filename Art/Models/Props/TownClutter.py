"""Town clutter for Ransom's Rest (Docs/Work/2026-11-04-Towns/PLAN.md 4.4-4.5; the PROPS lane, round 1): the stacks,
tools, carts, signs, bills and merged vignettes the DRESSING lane's rules lay along Main Street, Pen Lane, the back lots
and the yards. A scripted model file (Art/README.md); Tools/models.ps1 exports it into /Game/Art/Props (SM_<name>).
Built with Tools/Blender/looter_clutter.py on the house trim sheet used as a palette, as FrontierClutter.py's pieces are
(weathered boards on strip A's 20 cm lanes, hewn battens C, debarked logs B, teal and oxide paint H1/H2, iron H3 folded
into its cleanest stretch, dark glass H4), so most pieces are ONE slot, HouseTrim, at the town's 320 px/m; the
exceptions say so. Big iron, tin and canvas take the tileable sets at 1024 px/m (IronBlack, MailboxTin, Canvas).

Stacks and containers (pivot: the ground under the middle, front -Y):
  CrateStack_A      three nailed crates, 1.0 x 0.8 x 1.5 m: ORRIN BROS. / MILLERS stencilled on the bottom one, THIS SIDE
                    UP on the middle, the small top one askew with a lid board gone and packing straw showing (Hay: 2).
  CrateStack_B      four crates in two courses, 2.0 x 1.0 x 1.05 m (PRUITT'S / RANSOM'S REST, V. C., NAILS) and a sack
                    of ORRIN BROS. OATS slumped over the small one.
  Crate_C           a long crate, 1.6 x 0.56 x 0.5 m, rope handles at its ends (Hay: 2), V. CONSOLIDATED / No. 14 SURVEY.
  BarrelStack_A     five barrels in a pyramid (three and two) on two sleepers, chocked, 2.0 x 0.95 x 1.35 m (the plan's
                    1.5 m is a third row; five keep it under ~3.1k triangles).
  Keg_Trestle       three kegs on a trestle, a wooden tap in the middle one's head (toward -Y), 1.4 x 0.78 x 0.82 m.
  ToolBarrel        an open barrel of long-handled tools heads up (fork, hoe, shovel, rake), a rake leaning on it,
                    1.2 x 1.1 x 1.75 m (the leaning rake's head lies 0.5 m out).
  SackPile_B        four half-empty oats sacks slumped together, hay spilled round them (Hay: 2), 1.3 x 1.0 x 0.45 m.
  LumberPile_Small  sawn boards (WoodPlanks, the walks' and the lumber stack's boards: 2) in five layers on two bearers,
                    one slid off to the ground in front, 1.9 x 1.2 x 0.34 m.
  Sawhorse_Log      a sawbuck holding a peeled log half sawn through, the bow saw left in the kerf, a cut round on the ground
                    and a heap of sawdust (the log on strip B as the chopping block's, WoodEndGrain: 2), 1.76 x 0.75 x 1.05 m.
  BarrelFire        a burn drum on three iron bar legs, rust-brown (DrumBurnt: MetalRust tinted 0x6e5e52 at 512 px/m,
                    its rust patches' scale), holes punched round its foot, charred boards in its ash (Charcoal): 2
                    slots, 0.6 x 0.6 x 1.0 m. SOCKET_Fire at its mouth (0, 0, 0.91): ATownFire's cards and light go there.
Wall pieces (pivot on the wall's face at its foot, the piece toward -Y):
  ToolsLeaning_A    a shovel blade-down, a fork tines-down, a pick on its head, leaning on the wall, 0.9 x 0.6 x 1.5 m.
  Ladder_A          a 3.6 m ladder leaning on the wall, its feet 0.36 m out (the plan's 0.15 m stands it bolt upright:
                    a ladder can't lean so), a rung gone near the top, a red rag tied on a rail.
  Handbills_A, _B   five bills each, nailed over one another on a wall (1.2 x 0.9 m), their free corners curling off it,
                    two-sided: the handbills atlas (Art/Textures/Handbills, looter_posters.py --handbills; MI_Handbills, 1
                    slot, 1024 px/m). Pivot: the cluster's middle on the wall; hang it ~1.45 m up. No collision, no LODs.
Signs (FrontierClutter's blade boards, the same make: hang at SignBracket's SOCKET_Sign and swing about its axis):
  SignBoard_Rooms   ROOMS in teal on a cream board (the boarding house).   SignBoard_Smith  SMITHY in cream on weathered
  SignBoard_Feed    FEED in oxide on weathered planks (the feed store).                    planks (the smithy).
  SignBoard_Baths   BATHS in cream on teal planks (the bathhouse). 0.87 x 0.48 m each, lettered both sides.
Street events (pivot: the middle of the footprint with the tongue or shafts):
  WagonFreight         a freight wagon, 1.9 x 5.2 x 2.0 m: a box of two faded teal boards under a weathered top board,
                       iron stake straps, a spring seat, oxide-red running gear and heavy wheels (rear 1.32 m, front
                       1.04 m), a brake, the tongue down on the ground, two sacks and a PRUITT'S crate in the back. The
                       box's middle is 1.06 m along +Y from the pivot. Nanite, its whole mesh the fallback (as the Dray).
  WagonFreight_Canvas  the same wagon under five ash bows and a canvas cover (Canvas, 1024 px/m: 2 slots), 2.6 m tall.
  WagonFreight_Broken  the same wagon abandoned (Whitlock's yard): up on a block where its front-left wheel came off (the
                       wheel leans on the box), the tongue snapped, side boards gone and hanging, the tailgate down.
  WaterBarrel_Cart     a two-wheeled water cart, 1.5 x 2.2 x 1.45 m: a barrel strapped on two saddles, its bung up, a tap
                       in its back head (+Y), a bucket on the ground under it, the shafts down in front.
Vignettes (several kit pieces merged into one mesh, one draw a material; the plan's dense clusters; built from this
file's pieces and FrontierClutter's by their own seeds, so they match the separate pieces exactly):
  Vignette_ShopFront_A   a lidded barrel with the broom and bucket, a crate with a smaller one askew on it, two flour
                         sacks, a crate of empties: before a store's window on the walk, 2.0 x 1.2 m (HouseTrim, Hay).
  Vignette_ShopFront_B   two barrels, slumped oats sacks with spilled hay, a long FEED crate: the feed store's or the
                         smithy's front, 2.5 x 1.3 m (HouseTrim, Hay).
  Vignette_PorchLived_A  the rocker with its shawl, a crate for a side table with a tin coffee pot and cup, the broom and
                         bucket by the door, 1.8 x 1.1 m (HouseTrim, Hay, MailboxTin).
  Vignette_PorchEmpty_A  a chair tipped on its side, a broken crate, two boards fallen against the wall, dead leaves and a
                         tumbleweed blown into the corner, 2.6 x 1.2 m (HouseTrim, Hay, FoliagePalette: no wind on it).
  Vignette_Alley_A       a barrel lying by the wall, one standing, a broken crate, three boards leaning on the wall, a
                         bucket, 2.4 x 1.2 m (HouseTrim, WoodPlanks).
  Vignette_BackYard_A    a rick of firewood between stakes, the chopping block and its axe, a bucket, 2.0 x 1.8 m
                         (HouseTrim, BarkOak, WoodEndGrain). Free-standing: its pivot is the ground under the group.
                         The others are wall pieces: their pivot is the wall's face at its foot (on the walk's or the
                         porch's floor), the props toward -Y.
Collision: UCX_ hulls round what blocks (each crate, stack, barrel group, the wagons' wheels and box, the carts, the
trestle, the sawbuck, the drum, the tools' and the ladder's slab against the wall; the vignettes keep their pieces'
hulls); none on the signs, the bills, leaves and tumbleweed. Small props: Nanite off, LODs 50% and 25% from 600
triangles; the three wagons Nanite with every triangle in the fallback (what Medium draws).

    Tools\\artrun.ps1 -Script Art\\Models\\Props\\TownClutter.py -Preview    every model, a lineup and the wall pieces on a
                                    long wall (Saved/ArtPreviews/TownProps/); '--only=Name,Name' builds just those;
                                    '--variants' also renders the Boardwalk/VillageProps/Containers variants from the export
    Tools\\models.ps1 -Only TownClutter -Out Intermediate\\ArtExport_TownProps -NoImport          export test
Everything random comes from seeded random.Random and mathutils noise; nothing walks a set of mesh elements.
"""
import importlib.util
import math
import os
import random
import sys

import bmesh
import bpy
from mathutils import Matrix, Vector, noise

import looter_buildings as kit
from looter_buildings import Trim
import looter_clutter as lc
import looter_homes as homes
import looter_props as lp
import looter_ruins as lr
import looter_textures as lt

ARGS = lc.ARGS
ONLY = next((a.split('=', 1)[1].split(',') for a in ARGS if a.startswith('--only=')), None)
UP = Vector((0.0, 0.0, 1.0))
TAU = 2.0 * math.pi
lc.PREVIEW_DIR = os.path.join(lt.PREVIEW_DIR, 'TownProps')
lc.REPORT_PATH = os.path.join(lc.PREVIEW_DIR, 'TownClutter.report.json')


def _sibling(name, module):
    """A model script beside this one as a module (its models are built only when it is run itself)."""
    sys.dont_write_bytecode = True     # no __pycache__ in the art folder
    path = os.path.join(lt.REPO, 'Art', 'Models', 'Props', name + '.py')
    spec = importlib.util.spec_from_file_location(module, path)
    mod = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(mod)
    return mod


FC = _sibling('FrontierClutter', 'frontier_parts')
BL = _sibling('Backlots', 'backlots_parts')    # its firewood sticks (the woodshed's), for the back yard's rick
CANVAS = homes.EXTRA['canvas']()            # MI_Canvas: Polymer tinted 0xd6c9a8 (looter_homes, the barns' tarps)


def P(frame, x, y, z):
    return frame @ Vector((x, y, z))


def D(frame, x, y, z):
    return (frame.to_3x3() @ Vector((x, y, z))).normalized()


def box_points(frame, lo, hi):
    return [frame @ Vector((x, y, z)) for x in (lo[0], hi[0]) for y in (lo[1], hi[1]) for z in (lo[2], hi[2])]


# --- Crates ---

def crate(m, frame, L, D_, H, seed, stencil=(), lid='closed', straw=False, skids=True, missing=None, gone=()):
    """A nailed plank crate in frame (origin: the middle of its foot; length L along X, depth D_ along Y, height H to
    the lid's top): weathered boards on the long sides running its length, end boards between them under a batten
    frame, a lid of boards, two skids, nail heads where the side boards meet the ends. stencil: (text, height, z share,
    strip) lines on the front (-Y). lid 'askew' slides and turns the lid, 'off' leaves it off; missing drops that lid
    board (straw shows); gone: ('front' | 'back' | 'left' | 'right', row) side boards kicked out (a broken crate).
    Returns the box's top (z in the frame) and its hull points."""
    rng = random.Random(seed)
    T, gap = 0.02, 0.004
    zb = 0.045 if skids else 0.0
    body = H - zb - T
    rows = max(2, int(round(body / 0.165)))
    bw = (body - (rows - 1) * gap) / rows
    if skids:
        for y in (-D_ * 0.5 + 0.09, D_ * 0.5 - 0.09):
            lc.batten(m, P(frame, -L * 0.5 + 0.03, y, zb * 0.5), P(frame, L * 0.5 - 0.03, y, zb * 0.5), (0.07, zb),
                      face=D(frame, 0, 0, 1), strip='C', chamfer=0.006)
    for s in (-1.0, 1.0):
        for r in range(rows):
            z = zb + bw * 0.5 + r * (bw + gap)
            if ('front' if s < 0 else 'back', r) in gone:
                continue
            y = s * (D_ * 0.5 - T * 0.5)
            lc.bar(m, P(frame, -L * 0.5, y, z), P(frame, L * 0.5, y, z), bw - rng.uniform(0.0, 0.005), T,
                   face=D(frame, 0, s, 0), strip='A', chamfer=0.005, twist=rng.uniform(-1.2, 1.2), nicks=0.002)
            lc.nail_heads(m, [P(frame, x, s * D_ * 0.5, z + dz) for x in (-L * 0.5 + 0.025, L * 0.5 - 0.025)
                              for dz in (-bw * 0.25, bw * 0.25)], D(frame, 0, s, 0), size=0.01)
    for s in (-1.0, 1.0):
        x = s * (L * 0.5 - T * 0.5)
        for r in range(rows):
            z = zb + bw * 0.5 + r * (bw + gap)
            if ('left' if s < 0 else 'right', r) in gone:
                continue
            lc.bar(m, P(frame, x, -D_ * 0.5 + T, z), P(frame, x, D_ * 0.5 - T, z), bw - rng.uniform(0.0, 0.004), T,
                   face=D(frame, s, 0, 0), strip='A', chamfer=0.004, caps=(False, False))
        xo = s * (L * 0.5 + 0.011)
        for y in (-D_ * 0.5 + 0.035, D_ * 0.5 - 0.035):
            lc.bar(m, P(frame, xo, y, zb), P(frame, xo, y, zb + body), 0.06, 0.022, face=D(frame, s, 0, 0), strip='C',
                   chamfer=0.006)
        for z in (zb + 0.035, zb + body - 0.035):
            lc.bar(m, P(frame, xo, -D_ * 0.5 + 0.068, z), P(frame, xo, D_ * 0.5 - 0.068, z), 0.06, 0.022,
                   face=D(frame, s, 0, 0), strip='C', chamfer=0.006)
        lc.nail_heads(m, [P(frame, xo + s * 0.011, y, z) for y in (-D_ * 0.5 + 0.035, D_ * 0.5 - 0.035)
                          for z in (zb + 0.035, zb + body - 0.035)], D(frame, s, 0, 0), size=0.012)
    # The lid.
    count = max(2, int(round(D_ / 0.15)))
    lw = (D_ - (count - 1) * gap) / count
    if lid == 'askew':
        lid_frame = frame @ lc.matrix_of((rng.uniform(0.04, 0.07), rng.uniform(-0.05, -0.03), 0.0),
                                         yaw=rng.uniform(5.0, 9.0))
    else:
        lid_frame = frame
    z = zb + body + T * 0.5
    for k in range(count):
        if k == missing or lid == 'off':
            continue
        y = -D_ * 0.5 + lw * 0.5 + k * (lw + gap)
        lc.bar(m, P(lid_frame, -L * 0.5 - 0.005, y, z), P(lid_frame, L * 0.5 + 0.005, y, z), lw - rng.uniform(0.0, 0.004),
               T, face=D(lid_frame, 0, 0, 1), strip='A', chamfer=0.005, twist=rng.uniform(-1.0, 1.0))
        lc.nail_heads(m, [P(lid_frame, x, y, z + T * 0.5) for x in (-L * 0.5 + 0.02, L * 0.5 - 0.02)], D(lid_frame, 0, 0, 1),
                      size=0.01)
    if straw or missing is not None or lid == 'off':
        straw_bed(m, frame, L - 2 * T - 0.01, D_ - 2 * T - 0.01, zb + body - 0.06, seed)
    for k, (text, height, share, strip) in enumerate(stencil):
        mapper = lc.plane_mapper(P(frame, 0.0, -D_ * 0.5, zb + body * share), D(frame, 1, 0, 0), D(frame, 0, 0, 1),
                                 lift=0.0016)
        lc.lettering(m, text, height, mapper, strip=strip, width=L * 0.74, tol=0.032, v0=0.02)
    top = zb + body + T
    return top, box_points(frame, (-L * 0.5 - 0.022, -D_ * 0.5, 0.0), (L * 0.5 + 0.022, D_ * 0.5, top))


def straw_bed(m, frame, L, D_, z, seed, lump=0.03):
    """Packing straw (Hay) a hand under a crate's rim, lumpy, as a grid."""
    cols, rows = 8, 5
    cos, faces = [], []
    sc = lt.SETS['Hay']['density'] / float(lt.SETS['Hay']['size'])
    off = Vector((seed * 0.37 % 1.0, seed * 0.61 % 1.0, 0.0))
    for j in range(rows + 1):
        for i in range(cols + 1):
            x = -L * 0.5 + L * i / cols
            y = -D_ * 0.5 + D_ * j / rows
            h = lump * noise.noise(Vector((x * 7.0, y * 7.0, seed * 0.13))) + 0.012 * noise.noise(Vector((x * 23.0, y * 21.0, 1.7)))
            cos.append(P(frame, x, y, z + h))
    for j in range(rows):
        for i in range(cols):
            a = j * (cols + 1) + i
            idx = lc._orient(cos, [a, a + 1, a + cols + 2, a + cols + 1], D(frame, 0, 0, 1))
            faces.append((idx, [((cos[k] - frame.translation).x * sc + off.x, (cos[k] - frame.translation).y * sc + off.y)
                                for k in idx]))
    m.put(cos, faces, key='hay', ao_floor=0.25)


def mound(m, centre, radius, height, seed, strip='B', u0=2.0, key='trim', ring=14, squash=1.25, wobble=0.35):
    """A low ragged heap (sawdust, chips, spilled grain) round centre: high in the middle, its ragged edge sunk a
    little under the ground so it never shows a floating rim on a slope."""
    rng = random.Random(seed)
    c = Vector(centre)
    cos = [c + Vector((0.0, 0.0, height))]
    off = rng.uniform(0.0, 50.0)
    for band, (share, z) in enumerate(((0.55, height * 0.62), (1.0, -0.006))):
        for k in range(ring):
            a = TAU * k / ring
            wob = 1.0 + wobble * noise.noise(Vector((math.cos(a) * 1.7, math.sin(a) * 1.7, off + band * 0.3)))
            rr = radius * share * wob
            cos.append(c + Vector((math.cos(a) * rr * squash, math.sin(a) * rr, z + rng.uniform(-0.002, 0.002))))
    faces = []

    def uv(i):
        d = cos[i] - c
        return (lc.strip_uv(strip, u0 + d.x, 0.2 + d.y) if key == 'trim' else
                (d.x * lc.set_scale(lc.BAR_SETS.get(key, 'Hay')) + 0.3, d.y * lc.set_scale(lc.BAR_SETS.get(key, 'Hay'))))
    for k in range(ring):
        k1 = (k + 1) % ring
        idx = lc._orient(cos, [0, 1 + k, 1 + k1], UP)
        faces.append((idx, [uv(i) for i in idx]))
        idx = lc._orient(cos, [1 + k, 1 + ring + k, 1 + ring + k1, 1 + k1], UP)
        faces.append((idx, [uv(i) for i in idx]))
    m.put(cos, faces, key=key, ao_floor=0.45)


# --- Barrels and kegs ---

def barrel(m, frame, H=0.92, r_end=0.27, r_mid=0.33, staves=16, hoops=(0.07, 0.27, 0.65, 0.85), top='head',
           bottom='head', rows=None, bung=None, tap=False, light=False):
    """A coopered barrel in frame (origin: the middle of its foot, its axis the frame's Z): staves bellied from r_end
    to r_mid, each its own weathered board, a chime at each end round a recessed head of boards, riveted iron hoops at
    the heights in hoops. top 'head' or 'open' (the inside down to its floor, dark); bung (an angle in degrees) puts a
    bung in the belly; tap a wooden tap in the bottom head (a keg lying on its side shows it). Returns r(z)."""
    chime, wall = 0.035, 0.022

    def r(z):
        return r_end + (r_mid - r_end) * math.sin(math.pi * min(max(z / H, 0.0), 1.0)) ** 0.8
    zs = rows or [0.0, 0.05, 0.16, 0.3, 0.46, 0.62, 0.76, 0.87, 1.0]
    zs = [z * H for z in zs]
    at = frame.translation
    rot = frame.to_3x3()
    axis = rot @ UP
    prof = []
    if bottom:
        prof += [(r(chime) - wall, chime), (r(0.0) - wall, 0.0)]
    prof += [(r(z), z) for z in zs]
    if top == 'open':
        inner = [(r(z) - wall, z) for z in reversed(zs[1:-1]) if z > chime + 0.03]
        prof += [(r(H) - wall, H)] + inner + [(r(chime) - wall, chime + 0.004), (0.0, chime + 0.004)]
    else:
        prof += [(r(H) - wall, H), (r(H - chime) - wall, H - chime)]
    lc.turned(m, prof, staves, at=at, rot=rot, strip='A', grain='staves', phase=math.pi / staves)
    for z in hoops:
        z0, z1 = z * H - 0.017, z * H + 0.017
        if light:
            # A stacked barrel's hoops: a low ridge of two bands (its lips are never seen from a step away).
            lc.turned(m, [(r(z0) - 0.003, z0), (r(z * H) + 0.006, z * H), (r(z1) - 0.003, z1)], staves, at=at, rot=rot,
                      strip='H3', grain='around')
        else:
            lc.hoop(m, at, r(z * H), z0, z1, staves, rot=rot)
    if top != 'open':
        lc.disc(m, at + axis * (H - chime), r(H - chime) - wall + 0.002, staves, strip='A', normal=axis)
    if bottom:
        lc.disc(m, at + axis * chime, r(chime) - wall + 0.002, staves, strip='A', normal=-axis)
    if bung is not None:
        a = math.radians(bung)
        out = rot @ Vector((math.cos(a), math.sin(a), 0.0))
        lc.turned(m, [(0.0, -0.01), (0.026, -0.01), (0.026, 0.008), (0.02, 0.014), (0.0, 0.014)], 8,
                  at=at + axis * (H * 0.5) + out * (r(H * 0.5) - 0.004), rot=UP.rotation_difference(out).to_matrix(),
                  strip='C', grain='up')
    if tap:
        # A wooden tap through the bottom head (a lying keg's front), a little off its middle.
        # Local +Y is down on a lying barrel (lying() with roll 0).
        down = rot @ Vector((0.0, 1.0, 0.0))
        c = at + axis * chime + down * (r(chime) * 0.45)
        d = -axis
        lc.turned(m, [(0.0, -0.01), (0.03, -0.01), (0.03, 0.02), (0.019, 0.03), (0.019, 0.1), (0.016, 0.105), (0.0, 0.106)],
                  8, at=c, rot=UP.rotation_difference(d).to_matrix(), strip='C', grain='up')
        spout = c + d * 0.085
        lc.rod(m, [spout, spout + down * 0.07], 0.011, strip='C', sides=6)
        lc.rod(m, [c + d * 0.065, c + d * 0.065 - down * 0.06], 0.008, strip='H3', sides=4)
    return r


def lying(at, yaw=0.0, roll=0.0):
    """A frame laying a barrel on its side: its axis along the yawed +Y (its bottom head toward -Y), its foot's middle
    at at (the barrel's centre is r_mid above the ground there)."""
    return (Matrix.Translation(Vector(at)) @ Matrix.Rotation(math.radians(yaw), 4, 'Z') @
            Matrix.Rotation(math.radians(-90.0), 4, 'X') @ Matrix.Rotation(math.radians(roll), 4, 'Z'))


# --- CrateStack_A, CrateStack_B, Crate_C ---

ORRIN = (('ORRIN BROS.', 0.075, 0.64, 'H2'), ('MILLERS', 0.05, 0.34, 'H2'))


def crate_stack_a(name, seed):
    m = lc.Model(name, seed)
    f0 = lc.matrix_of((0.0, 0.03, 0.0), yaw=2.0)
    top0, h0 = crate(m, f0, 0.95, 0.8, 0.615, seed, stencil=ORRIN)
    f1 = lc.matrix_of((-0.05, 0.08, top0), yaw=-6.0)
    top1, h1 = crate(m, f1, 0.8, 0.6, 0.505, seed + 1, stencil=(('THIS SIDE UP', 0.05, 0.55, 'H2'),))
    f2 = lc.matrix_of((0.09, 0.02, top0 + top1), yaw=17.0)
    _, h2 = crate(m, f2, 0.56, 0.4, 0.365, seed + 2, lid='askew', missing=1, stencil=(('GLASS', 0.045, 0.5, 'H1'),))
    for h in (h0, h1, h2):
        m.hull_points(h)
    return m.done('the ground under the stack\'s middle', 'Three nailed crates, the top one askew with a lid\n'
                  'board gone (straw); ORRIN BROS. stencilled.', ao=0.4)


def crate_stack_b(name, seed):
    m = lc.Model(name, seed)
    fa = lc.matrix_of((-0.5, 0.02, 0.0), yaw=-1.5)
    ta, ha = crate(m, fa, 0.96, 0.84, 0.52, seed, stencil=(("PRUITT'S", 0.07, 0.62, 'H2'), ("RANSOM'S REST", 0.042, 0.3, 'H2')))
    fb = lc.matrix_of((0.49, -0.01, 0.0), yaw=2.5)
    tb, hb = crate(m, fb, 0.94, 0.86, 0.52, seed + 1, stencil=(('V. C.', 0.09, 0.55, 'H1'),))
    fc_ = lc.matrix_of((-0.46, 0.06, ta), yaw=4.0)
    _, hc = crate(m, fc_, 0.86, 0.72, 0.44, seed + 2, stencil=(('NAILS', 0.06, 0.55, 'H2'),))
    fd = lc.matrix_of((0.47, 0.1, tb), yaw=-9.0)
    td, hd = crate(m, fd, 0.6, 0.5, 0.36, seed + 3)
    # A sack slumped over the small crate's top.
    sack = FC.Sack(0.8, 0.46, 0.17, seed * 10 + 7)
    matrix = lc.matrix_of((0.48, 0.06, tb + td + 0.06), yaw=-8.0, pitch=4.0)     # its print reads from the front
    sack.emit(m, matrix, floor=tb + td)
    sack.print_on(m, matrix, [(FC.MILL[0], 0.055, 0.07, 'H2'), ('OATS', 0.085, -0.05, 'H1')], floor=tb + td)
    for h in (ha, hb, hc, hd):
        m.hull_points(h)
    return m.done('the ground under the stack\'s middle', 'Four crates in two courses (PRUITT\'S, V. C.,\n'
                  'NAILS), a sack of oats over the small one.', ao=0.4)


def rope_handle(m, frame, x, z, span=0.16, sag=0.06):
    """A rope handle through two holes in an end: a loop of hemp hanging from two knots, on the Hay set."""
    s = 1.0 if x > 0 else -1.0
    pts = [P(frame, x, -span * 0.5, z), P(frame, x + s * 0.03, -span * 0.35, z - sag * 0.55),
           P(frame, x + s * 0.04, 0.0, z - sag), P(frame, x + s * 0.03, span * 0.35, z - sag * 0.55),
           P(frame, x, span * 0.5, z)]
    lc.rod(m, lc.spline(pts, per=2), 0.011, key='hay', sides=6)
    for y in (-span * 0.5, span * 0.5):
        lc.turned(m, [(0.0, -0.004), (0.02, 0.0), (0.019, 0.02), (0.0, 0.026)], 6, at=P(frame, x, y, z),
                  rot=UP.rotation_difference(D(frame, s, 0, 0)).to_matrix(), key='hay', grain='around')


def crate_c(name, seed):
    m = lc.Model(name, seed)
    f = lc.matrix_of()
    L, Dp, H = 1.52, 0.56, 0.48
    top, h = crate(m, f, L, Dp, H, seed, stencil=(('V. CONSOLIDATED', 0.05, 0.62, 'H2'), ('No. 14   SURVEY', 0.04, 0.32, 'H2')))
    for s in (-1.0, 1.0):
        rope_handle(m, f, s * (L * 0.5 + 0.022), 0.33)
    m.hull_points(h)
    return m.done('the ground under the crate\'s middle', 'A long crate with rope handles, stencilled for\n'
                  'the Company\'s survey.', ao=0.35)


# --- BarrelStack_A, Keg_Trestle, ToolBarrel ---

def chock(m, at, yaw, seed):
    """A sawn wedge under a lying barrel."""
    rng = random.Random(seed)
    f = lc.matrix_of(at, yaw=yaw)
    outline = [(-0.07, 0.0), (0.07, 0.0), (0.07, 0.025), (-0.07, 0.085)]
    lc.prism_part(m, outline, 0.07, f @ Matrix.Translation((0.0, -0.035, 0.0)), uv=Trim('C'))


def barrel_stack_a(name, seed):
    m = lc.Model(name, seed)
    rng = m.rng
    H, re, rm = 0.92, 0.27, 0.33
    for y in (-0.28, 0.28):
        lc.batten(m, (-1.0, y, 0.06), (1.0, y, 0.06), (0.12, 0.12), face=(0.0, 0.0, 1.0), strip='C', chamfer=0.01,
                  twist=rng.uniform(-2, 2))
    base = 0.12
    xs = [-0.67, 0.0, 0.67]
    for k, x in enumerate(xs):
        f = lying((x, -H * 0.5, base + rm), yaw=rng.uniform(-1.5, 1.5), roll=rng.uniform(0.0, 360.0))
        barrel(m, f, H, re, rm, staves=14, rows=[0.0, 0.06, 0.22, 0.5, 0.78, 0.94, 1.0], light=True,
               hoops=(0.08, 0.3, 0.7, 0.92), bung=rng.uniform(60.0, 120.0))
        for y in (-0.28, 0.28):
            for s in (-1.0, 1.0):
                chock(m, (x + s * 0.24, y, base), 180.0 if s < 0 else 0.0, seed + k)
    for k, x in enumerate((-0.335, 0.335)):
        z = base + rm + math.sqrt(0.67 ** 2 - 0.335 ** 2) * 0.99
        f = lying((x + rng.uniform(-0.02, 0.02), -H * 0.5 + rng.uniform(-0.04, 0.04), z), yaw=rng.uniform(-3, 3),
                  roll=rng.uniform(0.0, 360.0))
        barrel(m, f, H, re, rm, staves=14, rows=[0.0, 0.06, 0.22, 0.5, 0.78, 0.94, 1.0], light=True,
               hoops=(0.08, 0.3, 0.7, 0.92), bung=rng.uniform(70.0, 110.0))
    m.hull((1.95, 0.95, base + 2 * rm), at=(0.0, 0.0, (base + 2 * rm) * 0.5))
    top = base + rm + math.sqrt(0.67 ** 2 - 0.335 ** 2) + rm
    m.hull_points([Vector((x, y, z)) for x in (-0.65, 0.65) for y in (-0.47, 0.47) for z in (base + 2 * rm - 0.05,)] +
                  [Vector((x, y, top)) for x in (-0.35, 0.35) for y in (-0.47, 0.47)])
    return m.done('the ground under the stack\'s middle', 'Five barrels in a pyramid on two sleepers,\n'
                  'chocked.', ao=0.45)


def trestle(m, x, half, top, seed, spread=0.2):
    """An A-frame of two splayed legs and a cross bar under a rail (along Y), at x."""
    rng = random.Random(seed)
    for s in (-1.0, 1.0):
        foot = Vector((x + rng.uniform(-0.01, 0.01), s * (half + spread), 0.0))
        head = Vector((x, s * (half - 0.03), top - 0.02))
        lc.batten(m, foot, head, (0.07, 0.05), face=(1.0, 0.0, 0.0), strip='C', chamfer=0.006)
    lc.batten(m, (x, -half - spread * 0.6, top * 0.32), (x, half + spread * 0.6, top * 0.32), (0.08, 0.025),
              face=(1.0, 0.0, 0.0), strip='C', chamfer=0.005)


def keg_trestle(name, seed):
    m = lc.Model(name, seed)
    rng = m.rng
    top = 0.4
    for x in (-0.55, 0.55):
        trestle(m, x, 0.16, top, seed + int(x * 10))
    for y in (-0.15, 0.15):
        lc.batten(m, (-0.7, y, top - 0.035), (0.7, y, top - 0.035), (0.09, 0.07), face=(0.0, 0.0, 1.0), strip='C',
                  chamfer=0.008)
    H, re, rm = 0.56, 0.17, 0.21
    for k, x in enumerate((-0.44, 0.0, 0.44)):
        f = lying((x + rng.uniform(-0.015, 0.015), -H * 0.5 + rng.uniform(-0.02, 0.02), top + 0.195),
                  yaw=rng.uniform(-2.0, 2.0), roll=0.0 if k == 1 else rng.uniform(0.0, 360.0))
        barrel(m, f, H, re, rm, staves=14, rows=[0.0, 0.06, 0.25, 0.5, 0.75, 0.94, 1.0], hoops=(0.09, 0.3, 0.7, 0.91),
               tap=k == 1)
    m.hull((1.42, 0.62, top + 2 * rm - 0.04), at=(0.0, 0.0, (top + 2 * rm - 0.04) * 0.5))
    return m.done('the ground under the trestle\'s middle', 'Three kegs on a trestle, a wooden tap in the\n'
                  'middle one.', ao=0.4)


# --- Tools (shared by ToolBarrel, ToolsLeaning_A, the vignettes) ---

def handle(m, butt, tip, r0=0.017, r1=0.015, sides=7):
    lc.rod(m, [butt, butt.lerp(tip, 0.5), tip], r0, strip='C', sides=sides, radii=[r0, (r0 + r1) * 0.5, r1])


def tool(m, kind, butt, head, seed, face=(0.0, -1.0, 0.0)):
    """A long-handled tool from its handle's butt to where the head goes: 'shovel' (round-point blade on a socket),
    'fork' (three curved tines), 'rake' (a bow rake's iron head), 'hoe', 'pick' (the head across the handle's end). The
    head's flat side faces face. Iron is the trim's iron strip, folded into its cleanest stretch (Model.done)."""
    rng = random.Random(seed)
    butt, head = Vector(butt), Vector(head)
    d = (head - butt).normalized()
    f = Vector(face)
    f = (f - d * f.dot(d)).normalized()
    side = d.cross(f).normalized()
    if kind == 'pick':
        handle(m, butt, head - d * 0.02, 0.016, 0.022)
        # The head: an iron bar curved across the handle's end, a point one way and a chisel the other.
        pts, radii = [], []
        for k in range(9):
            t = -1.0 + 2.0 * k / 8
            pts.append(head + side * (t * 0.3) - d * (0.05 * (1.0 - t * t)) + d * 0.02)
            radii.append(0.016 * (1.0 - 0.75 * abs(t) ** 1.6) + 0.003)
        lc.rod(m, pts, 0.016, strip='H3', sides=6, radii=radii)
        lc.rod(m, [head - d * 0.03, head + d * 0.035], 0.026, strip='H3', sides=8)
        return
    handle(m, butt, head)
    if kind == 'shovel':
        lc.turned(m, [(0.0, 0.0), (0.019, 0.0), (0.024, 0.12), (0.03, 0.17), (0.0, 0.17)], 8, at=head - d * 0.12,
                  rot=UP.rotation_difference(d).to_matrix(), strip='H3', grain='around')
        # A round-point blade: square shoulders, straight sides, a rounded point, dished across.
        rows = []
        ts = (0.0, 0.2, 0.45, 0.65, 0.85, 1.0)
        for t in ts:
            w = 0.108 if t <= 0.45 else max(0.012, 0.108 * math.cos((t - 0.45) / 0.55 * math.pi * 0.5) ** 0.8)
            row = []
            for i in range(5):
                s = -1.0 + 2.0 * i / 4
                dish = 0.026 * (1.0 - s * s) * (1.0 - 0.5 * t)
                row.append(head + d * (0.03 + t * 0.29) + side * (s * w) + f * dish)
            rows.append(row)
        u_iron = lc.IRON_CLEAN[0] + 0.05
        lc.cloth(m, rows, 0.004, strip='H3', facing=f,
                 uv_of=lambda j, i: (u_iron + 0.22 * i / 4, 0.03 + 0.14 * ts[j]))
    elif kind == 'fork':
        lc.turned(m, [(0.0, 0.0), (0.019, 0.0), (0.022, 0.09), (0.0, 0.1)], 8, at=head - d * 0.08,
                  rot=UP.rotation_difference(d).to_matrix(), strip='H3', grain='around')
        lc.rod(m, [head + side * -0.075 + d * 0.03, head + d * 0.035, head + side * 0.075 + d * 0.03], 0.009, strip='H3',
               sides=5)
        for s in (-0.075, 0.0, 0.075):
            base = head + side * s + d * 0.03
            pts = [base + d * (0.09 * k) + f * (0.012 * k * k) + side * (s * 0.08 * k) for k in range(5)]
            lc.rod(m, pts, 0.0065, strip='H3', sides=5, radii=[0.007, 0.0065, 0.006, 0.0045, 0.002])
    elif kind == 'rake':
        lc.rod(m, [head - d * 0.04, head + d * 0.02], 0.02, strip='H3', sides=6)
        for s in (-1.0, 1.0):
            lc.rod(m, [head - d * 0.03, head + side * (s * 0.12) + d * 0.08, head + side * (s * 0.19) + d * 0.09], 0.006,
                   strip='H3', sides=4)
        bar_c = head + d * 0.095
        lc.batten(m, bar_c - side * 0.2, bar_c + side * 0.2, (0.02, 0.018), face=f, strip='H3', chamfer=0.003)
        for k in range(12):
            s = -0.18 + 0.36 * k / 11
            root = bar_c + side * s
            lc.rod(m, [root, root + d * 0.05 + f * 0.02, root + d * 0.075 + f * 0.05], 0.0045, strip='H3', sides=4,
                   radii=[0.005, 0.0045, 0.002], caps=(False, True))
    elif kind == 'hoe':
        lc.rod(m, [head - d * 0.06, head + d * 0.01, head + d * 0.04 + f * 0.05], 0.011, strip='H3', sides=5)
        blade_c = head + d * 0.07 + f * 0.08
        rows = [[blade_c + side * (s * 0.085) + d * (t - 0.04) - f * 0.004 * (1 - s * s) for s in (-1.0, -0.5, 0.0, 0.5, 1.0)]
                for t in (0.0, 0.04, 0.08)]
        u_iron = lc.IRON_CLEAN[0] + 0.1
        lc.cloth(m, rows, 0.004, strip='H3', facing=-d, uv_of=lambda j, i: (u_iron + 0.17 * i / 4, 0.05 + 0.04 * j))


def tool_barrel(name, seed):
    m = lc.Model(name, seed)
    rng = m.rng
    H = 0.92
    r = barrel(m, lc.matrix_of(), H, 0.27, 0.33, staves=16, top='open', bottom=None,
               rows=[0.0, 0.05, 0.16, 0.3, 0.46, 0.62, 0.76, 0.87, 1.0])
    # Handles down in the barrel, heads up, leaning out a little over the rim.
    stood = [('fork', (-0.08, 0.06), (-0.2, 0.18), 1.62), ('hoe', (0.07, 0.09), (0.21, 0.2), 1.55),
             ('shovel', (0.04, -0.07), (0.12, -0.22), 1.5), ('rake', (-0.06, -0.08), (-0.15, -0.21), 1.58)]
    for k, (kind, (x0, y0), (x1, y1), z1) in enumerate(stood):
        butt = Vector((x0, y0, 0.06))
        head = Vector((x1, y1, z1 - 0.32))
        tool(m, kind, butt, head, seed + k, face=(head - butt).cross(UP).normalized() if kind != 'shovel' else (0.0, -1.0, 0.0))
    # A rake leaning on the outside, its head on the ground in front.
    tool(m, 'rake', Vector((0.31, 0.12, 0.88)), Vector((0.62, -0.42, 0.06)), seed + 9, face=(0.0, 0.0, 1.0))
    FC.ring_hull(m, (0.0, 0.0, 0.0), 0.34, 0.0, H)
    return m.done('the ground under the barrel\'s middle', 'An open barrel of long-handled tools, heads up,\n'
                  'and a rake leaning on it.', ao=0.4)


# --- ToolsLeaning_A, Ladder_A (wall pieces: the pivot on the wall's face at its foot, the piece toward -Y) ---

def lean_tool(m, kind, foot, top, length, seed, face=(0.0, -1.0, 0.0), head_len=0.32):
    """A tool leaning on the wall: its head on the ground at foot (x, y), its butt against the wall at height top."""
    butt = Vector((foot[0] + 0.03, -0.022, top))
    tip = Vector((foot[0], foot[1], 0.012))
    d = (tip - butt).normalized()
    head = tip - d * head_len
    tool(m, kind, butt, head, seed, face=face)
    return butt, tip


def tools_leaning_a(name, seed):
    m = lc.Model(name, seed)
    # A shovel blade-down, a fork tines-down, a pick on its head, all leaning on the wall.
    lean_tool(m, 'shovel', (-0.33, -0.27), 1.36, 1.42, seed, face=(0.0, -1.0, 0.0), head_len=0.33)
    lean_tool(m, 'fork', (0.04, -0.26), 1.48, 1.5, seed + 1, face=(0.0, -1.0, 0.0), head_len=0.39)
    pick_foot = Vector((0.4, -0.2, 0.035))
    pick_butt = Vector((0.36, -0.024, 0.9))
    tool(m, 'pick', pick_butt, pick_foot + Vector((0.0, 0.0, 0.02)), seed + 2, face=(1.0, 0.0, 0.0))
    m.hull_points([Vector((x, y, z)) for x in (-0.5, 0.52) for y, z in ((-0.32, 0.0), (0.0, 0.0), (0.0, 1.5), (-0.06, 1.5))])
    return m.done('on the wall at its foot (ground, the wall\'s face); the tools lean on it toward -Y',
                  'A shovel, a fork and a pick leaning on a wall.', ao=0.35)


def ladder_a(name, seed):
    m = lc.Model(name, seed)
    rng = m.rng
    top, foot_y = 3.6, -0.36
    for s in (-1.0, 1.0):
        p0 = Vector((s * 0.245, foot_y, 0.0))
        p1 = Vector((s * 0.215, -0.034, top))
        lc.bar(m, p0, p1, 0.066, 0.036, face=(s, 0.0, 0.0), strip='C', chamfer=0.006, back=0.006,
               bow=0.0, twist=rng.uniform(-1.0, 1.0), segments=4)
    k = 0
    z = 0.3
    while z < top - 0.15:
        t = z / top
        y = foot_y + (-0.034 - foot_y) * t
        half = 0.245 + (0.215 - 0.245) * t
        if k != 8:          # one rung near the top gone, a nail-scarred gap where it was
            lc.rod(m, [(-half - 0.012, y, z), (half + 0.012, y, z)], 0.0165, strip='C', sides=7)
        k += 1
        z += 0.3
    # A rag tied round the left rail to mark it.
    lc.turned(m, [(0.045, 1.38), (0.05, 1.4), (0.05, 1.46), (0.045, 1.48)], 7, at=(-0.243, -0.27, 0.0), strip='H2',
              grain='around')
    m.hull_points([Vector((x, y, z)) for x in (-0.28, 0.28) for y, z in ((foot_y - 0.04, 0.0), (foot_y + 0.04, 0.0),
                                                                        (-0.0, top), (-0.07, top))])
    return m.done('on the wall at its foot (ground, the wall\'s face); its feet stand 0.36 m out, its top on the wall',
                  'A 3.6 m ladder leaning on a wall, a rung gone\nnear the top.', ao=0.3)


# --- Sawhorse_Log, LumberPile_Small ---

BARK = lt.material('BarkOak')


def bark_log(m, p0, p1, r0, r1, sides=10, seed=0):
    """A round of log for the saw: peeled, on the trim's debarked log strip (as the chopping block's), sawn ends on
    WoodEndGrain. (BarkOak's plates, made for trunks, read as a net on a 25 cm log at any density.)"""
    m.cylinder(p0, p1, r0, r1, sides=sides, uv=Trim('B', v=0.0), face=(0.0, -1.0, 0.0), cap_uv=kit.EndGrain(),
               cap_mat='endgrain')


def sawhorse_log(name, seed):
    m = lc.Model(name, seed)
    rng = m.rng
    # A sawbuck: two X-frames of squared legs, a stretcher; the log in the crotches.
    spread, lean_top, head_z = 0.3, 0.13, 0.75
    for x in (-0.45, 0.45):
        for s in (-1.0, 1.0):
            foot = Vector((x + rng.uniform(-0.01, 0.01), s * spread, 0.0))
            head = Vector((x, -s * lean_top, head_z))
            lc.batten(m, foot, head, (0.07, 0.055), face=(1.0 if s > 0 else -1.0, 0.0, 0.0), strip='C', chamfer=0.007)
    # Where the legs cross, and where a log of radius r rests in the V above (touching both legs).
    cross_z = head_z * spread / (spread + lean_top)
    half = math.atan2(spread + lean_top, head_z)
    lc.batten(m, (-0.52, 0.0, 0.26), (0.52, 0.0, 0.26), (0.08, 0.045), face=(0.0, -1.0, 0.0), strip='C', chamfer=0.006)
    lc.nail_heads(m, [(x * 1.07, -0.035, cross_z) for x in (-0.45, 0.45)], (1.0, 0.0, 0.0), size=0.014)
    r = 0.125
    cz = cross_z + (r + 0.035) / math.sin(half)
    bark_log(m, (-0.74, 0.0, cz), (0.62, 0.0, cz + 0.01), r, r * 0.95, sides=12)
    # The kerf: a dark slot across the log's top a third of the way in, the bow saw left in it.
    kx = 0.36
    arc = [Vector((kx, math.sin(a) * (r * 0.966 + 0.0015), cz + 0.0075 + math.cos(a) * (r * 0.966 + 0.0015)))
           for a in (math.radians(-80.0 + 160.0 * k / 8) for k in range(9))]
    lc.tube(m, arc, lc.rect(0.006, 0.002), strip='H4', up=(1.0, 0.0, 0.0), caps=(False, False))
    blade_z = cz - 0.01
    for y in (-0.36, 0.36):
        lc.batten(m, (kx, y, blade_z - 0.03), (kx, y * 0.92, blade_z + 0.42), (0.03, 0.022), face=(1.0, 0.0, 0.0),
                  strip='C', chamfer=0.004)
    lc.batten(m, (kx, -0.34, blade_z + 0.2), (kx, 0.34, blade_z + 0.2), (0.028, 0.018), face=(0.0, 0.0, 1.0), strip='C',
              chamfer=0.004)
    lc.bar(m, (kx, -0.36, blade_z), (kx, 0.36, blade_z), 0.045, 0.002, face=(1.0, 0.0, 0.0), strip='H3', chamfer=0.0)
    for k in range(5):
        y = -0.33 + 0.66 * k / 4
        lc.rod(m, [(kx - 0.004, y, blade_z + 0.41), (kx + 0.004, y + 0.02, blade_z + 0.4)], 0.004, strip='G', sides=4)
    lc.rod(m, [(kx, -0.33, blade_z + 0.405), (kx, 0.33, blade_z + 0.405)], 0.005, strip='G', sides=4)
    lc.rod(m, [(kx, 0.02, blade_z + 0.405), (kx + 0.02, 0.0, blade_z + 0.33)], 0.007, strip='C', sides=5)
    # A round already cut, on the ground, and a little heap of sawdust under the kerf (the debarked log strip's pale
    # wood, in a ragged mound that sinks into the ground at its edge).
    bark_log(m, (0.86, -0.26, 0.0), (0.9, -0.2, 0.245), r * 0.96, r * 0.96, sides=12)
    mound(m, Vector((kx + 0.02, 0.01, 0.0)), 0.15, 0.022, seed, strip='B', u0=2.1)
    m.hull_points([Vector((x, y, z)) for x in (-0.75, 0.63) for y in (-0.33, 0.33) for z in (0.0,)] +
                  [Vector((x, y, cz + r)) for x in (-0.75, 0.63) for y in (-r, r)])
    return m.done('the ground under the sawbuck\'s middle', 'A sawbuck with a log half sawn through, the bow\n'
                  'saw left in the kerf, a round on the ground.', ao=0.4)


def lumber_pile_small(name, seed):
    m = lc.Model(name, seed)
    rng = m.rng
    for x in (-0.6, 0.6):
        m.board((x + rng.uniform(-0.03, 0.03), -0.42, 0.05), (x, 0.42, 0.05), 0.1, 0.1, face=(0.0, 0.0, 1.0),
                uv=Trim('C', lane='each'))
    z = 0.1
    for layer in range(5):
        count = 3 if layer < 4 else 2
        widths = [rng.uniform(0.18, 0.2) for _ in range(count)]
        gap = (0.72 - sum(widths)) / max(count - 1, 1) if count > 1 else 0.0
        y = -0.36 + (0.1 if layer == 4 else 0.0)
        for w in widths:
            length = rng.uniform(1.7, 1.82)
            x = rng.uniform(-0.05, 0.05)
            yaw = rng.uniform(-1.2, 1.2) if layer < 4 else rng.uniform(-6.0, 6.0)
            d = Vector((math.cos(math.radians(yaw)), math.sin(math.radians(yaw)), 0.0))
            c = Vector((x, y + w * 0.5, z + 0.0125))
            m.board(c - d * length * 0.5, c + d * length * 0.5, w, 0.025, face=(0.0, 0.0, 1.0), uv=lr.PlankRow(),
                    mat='planks', cuts=1)
            y += w + gap
        z += 0.025
    # One board slid off, leaning from the pile's edge to the ground at the front.
    foot = Vector((-0.2, -0.62, 0.01))
    rest = Vector((0.55, -0.36, z - 0.01))
    m.board(foot, rest + (rest - foot).normalized() * 0.2, 0.19, 0.025, face=(0.0, -0.4, 1.0), uv=lr.PlankRow(),
            mat='planks')
    m.hull((1.84, 0.8, z), at=(0.0, 0.0, z * 0.5))
    return m.done('the ground under the pile\'s middle', 'Sawn boards stacked on two bearers, one slid off\n'
                  'and leaning to the ground.', ao=0.35)


# --- The new blade signs (FrontierClutter's boards: hang them at SignBracket's SOCKET_Sign) ---

# (name, seed, text, board strip, letters strip, the board's stretch(es) along U, the letters' stretch, cap height).
# Every stretch is clear of its strip's joints and stains (looter_props.JOINT_FREE, looter_clutter.CREAM): 4.86-5.66
# is joint-free on all four of strip A's lanes, so either plank's random lane reads as one board.
SIGNS = [
    ('SignBoard_Rooms', 5121, 'ROOMS', 'G', 'H1', 5.45, 4.7, 0.16),
    ('SignBoard_Smith', 5122, 'SMITHY', 'A', 'G', (4.86, 4.86), 4.18, 0.15),
    ('SignBoard_Feed', 5123, 'FEED', 'A', 'H2', (4.86, 4.86), 4.4, 0.17),
    ('SignBoard_Baths', 5124, 'BATHS', 'H1', 'G', (2.7, 3.6), 5.5, 0.16),
]


def sign(name, seed, text, board, letters, u_board, u_text, height):
    obj = FC.sign_board(name, seed, text, board, letters, u_board, u_text, height=height)
    return obj


# --- BarrelFire ---

DRUM = lt.material('MetalRust', name='DrumBurnt', tint=0x6e5e52)


def barrel_fire(name, seed):
    m = lc.Model(name, seed)
    rng = m.rng
    R, H, legs = 0.29, 0.86, 0.13
    z0 = legs
    # The drum: an old steel drum, its paint burnt off to rust, rolling ridges, a rolled rim, open, an ash bed inside.
    prof = [(0.0, z0 + 0.06), (R - 0.01, z0 + 0.06), (R - 0.01, z0), (R, z0 + 0.01)]
    for zz in (0.2, 0.3, 0.6, 0.7):
        zc = z0 + zz * H
        prof += [(R, zc - 0.02), (R + 0.01, zc - 0.006), (R + 0.01, zc + 0.006), (R, zc + 0.02)]
    prof += [(R, z0 + H - 0.02), (R + 0.006, z0 + H - 0.008), (R + 0.004, z0 + H), (R - 0.006, z0 + H - 0.004),
             (R - 0.006, z0 + H * 0.8), (R - 0.006, z0 + H * 0.58)]
    lc.turned(m, prof, 18, at=(0.0, 0.0, 0.0), key='DrumBurnt', grain='around', set_name='MetalRust', density=512.0)
    # Punched holes round its foot (dark, a hair proud).
    for row, zz in enumerate((0.1, 0.42)):
        for k in range(9):
            a = TAU * (k + 0.5 * row) / 9 + 0.2
            c = Vector((math.cos(a) * (R + 0.0015), math.sin(a) * (R + 0.0015), z0 + zz * H))
            nrm = Vector((math.cos(a), math.sin(a), 0.0))
            side = Vector((-math.sin(a), math.cos(a), 0.0))
            s = 0.026 if row == 0 else 0.02
            corners = [c + side * dx * s + UP * dz * s * 1.2 for dx, dz in ((-1, -1), (1, -1), (1, 1), (-1, 1))]
            uvs = [(0.1 + 0.01 * j, 0.1 + 0.02 * (j // 2)) for j in range(4)]
            idx = lc._orient(corners, [0, 1, 2, 3], nrm)
            mp = dict(zip(range(4), uvs))
            m.put(corners, [(idx, [mp[j] for j in idx])], key='charcoal', smooth=False)
    # Three short legs of iron bar under it, splayed.
    for k in range(3):
        a = TAU * k / 3 + 0.5
        top_p = Vector((math.cos(a) * (R - 0.04), math.sin(a) * (R - 0.04), z0 + 0.02))
        foot = Vector((math.cos(a) * (R + 0.04), math.sin(a) * (R + 0.04), 0.0))
        lc.batten(m, foot, top_p, (0.028, 0.006), face=(math.cos(a), math.sin(a), 0.0), key='DrumBurnt', density=512.0,
                  chamfer=0.001)
    # Inside: ash and charred ends of planks sticking up out of it.
    ash_z = z0 + H * 0.62
    lc.turned(m, [(R - 0.008, ash_z - 0.01), (R - 0.008, ash_z), (R * 0.5, ash_z + 0.02), (0.0, ash_z + 0.03)], 12,
              at=(0.0, 0.0, 0.0), key='charcoal', grain='around', set_name='BarkOak')
    for k in range(4):
        a = TAU * k / 4 + rng.uniform(-0.3, 0.3)
        base = Vector((math.cos(a) * 0.1, math.sin(a) * 0.1, ash_z - 0.05))
        tip = base + Vector((math.cos(a) * 0.07, math.sin(a) * 0.07, rng.uniform(0.28, 0.4)))
        lc.batten(m, base, tip, (0.07, 0.03), face=(math.cos(a), math.sin(a), 0.0), key='charcoal', chamfer=0.006,
                  taper=0.6)
    m.socket('Fire', (0.0, 0.0, z0 + H - 0.08))
    FC.ring_hull(m, (0.0, 0.0, 0.0), R + 0.02, 0.0, z0 + H)
    return m.done('the ground under the drum\'s middle', 'A burn drum on iron legs, holes punched round\n'
                  'its foot, charred boards in it; SOCKET_Fire in its mouth.', ao=0.35)


# --- Wheels and the freight wagon ---

def wheel(m, frame, center, radius, spokes, seed, width=0.06, felloe=0.06, hub=0.085, hub_len=0.24, segs=18,
          paint='H2'):
    """A heavy spoked wheel in frame, its axle along the frame's X at center: an iron tyre on a painted felloe, spokes
    and a hub with iron bands (FrontierClutter's wheel, heavier, and placed by a frame)."""
    rng = random.Random(seed)
    rot = frame.to_3x3() @ Matrix.Rotation(math.radians(90.0), 3, 'Y')
    c = frame @ Vector(center)
    inner = radius - felloe
    lc.turned(m, [(inner, -width * 0.5), (radius - 0.014, -width * 0.5)], segs, at=c, rot=rot, strip=paint,
              grain='around')
    lc.turned(m, [(radius - 0.014, -width * 0.56), (radius, -width * 0.52), (radius, width * 0.52),
                  (radius - 0.014, width * 0.56)], segs, at=c, rot=rot, strip='H3', grain='around')
    lc.turned(m, [(radius - 0.014, width * 0.5), (inner, width * 0.5), (inner, -width * 0.5)], segs, at=c, rot=rot,
              strip=paint, grain='around')
    lc.turned(m, [(0.0, -hub_len * 0.5), (hub * 0.55, -hub_len * 0.5), (hub * 0.62, -hub_len * 0.44),
                  (hub, -hub_len * 0.2), (hub, hub_len * 0.2), (hub * 0.7, hub_len * 0.5), (0.0, hub_len * 0.5)], 10,
              at=c, rot=rot, strip=paint, grain='up')
    for z in (-hub_len * 0.38, hub_len * 0.3):
        lc.hoop(m, c, hub * 0.86 if z < 0 else hub * 0.95, z - 0.012, z + 0.012, 10, rot=rot)
    start = rng.uniform(0.0, math.pi)
    axis_x = frame.to_3x3() @ Vector((1.0, 0.0, 0.0))
    for k in range(spokes):
        a = start + TAU * k / spokes
        dloc = Vector((0.0, math.cos(a), math.sin(a)))
        d = frame.to_3x3() @ dloc
        lc.tube(m, [c + d * (hub * 0.85), c + d * (inner + 0.012)], lc.rect(0.026, 0.036), strip=paint, up=axis_x,
                scales=[1.0, 0.78], caps=(False, False))


def wheel_hull(m, frame, center, radius, width=0.08):
    c = Vector(center)
    m.hull_points([frame @ (c + Vector((dx, math.cos(a) * radius, math.sin(a) * radius)))
                   for a in (k * math.pi / 4 for k in range(8)) for dx in (-width * 0.5, width * 0.5)])


R_REAR, R_FRONT = 0.66, 0.52
Y_REAR, Y_FRONT = 1.0, -1.0
TRACK = 0.8
FLOOR = 0.9
BOX = (0.56, -1.35, 1.65)          # half inner width, front and back of the box
SIDE = 0.56


def freight_wagon(m, frame, seed, canvas=False, broken=False):
    """A freight wagon in frame (x across, front -Y): heavy oxide-red running gear and wheels, a box of faded teal
    side boards under a weathered top board, iron stake straps, a spring seat, a brake, the tongue down on the ground
    in front. broken: up on a block where its front-left wheel came off (the wheel leans on the box), the tongue
    snapped, side boards gone and hanging, the tailgate in the dirt. Returns the hull point lists."""
    rng = random.Random(seed)
    hw, y0, y1 = BOX
    hulls = []
    # Wheels.
    for sx in (-1.0, 1.0):
        wheel(m, frame, (sx * TRACK, Y_REAR, R_REAR), R_REAR, 14, seed + int(sx + 3))
        hulls.append(('wheel', (sx * TRACK, Y_REAR, R_REAR), R_REAR))
        if broken and sx < 0:
            continue
        wheel(m, frame, (sx * TRACK, Y_FRONT, R_FRONT), R_FRONT, 12, seed + int(sx + 7))
        hulls.append(('wheel', (sx * TRACK, Y_FRONT, R_FRONT), R_FRONT))
    # Running gear: axle beds and iron axles, bolsters, the reach, the hounds.
    for y, r in ((Y_REAR, R_REAR), (Y_FRONT, R_FRONT)):
        lc.batten(m, P(frame, -TRACK + 0.14, y, r), P(frame, TRACK - 0.14, y, r), (0.11, 0.12), face=D(frame, 0, -1, 0),
                  strip='H2', chamfer=0.01)
        lc.rod(m, [P(frame, -TRACK - 0.16, y, r), P(frame, TRACK + 0.16, y, r)], 0.026, strip='H3', sides=8)
        top = r + 0.06
        bz = FLOOR - 0.03 - 0.06
        lc.batten(m, P(frame, -hw - 0.1, y, (top + bz) * 0.5), P(frame, hw + 0.1, y, (top + bz) * 0.5), (bz - top, 0.11),
                  face=D(frame, 0, -1, 0), strip='H2', chamfer=0.01)
        for sx in (-1.0, 1.0):
            lc.batten(m, P(frame, sx * (hw + 0.07), y, bz - 0.02), P(frame, sx * (hw + 0.07), y, FLOOR + 0.24), (0.06, 0.05),
                      face=D(frame, sx, 0, 0), strip='H2', chamfer=0.006)
    reach_z = R_FRONT + 0.0
    lc.batten(m, P(frame, 0.0, Y_FRONT - 0.15, reach_z), P(frame, 0.0, Y_REAR + 0.45, R_REAR - 0.02), (0.1, 0.08),
              face=D(frame, 0, 0, 1), strip='H2', chamfer=0.008)
    for sx in (-1.0, 1.0):
        lc.batten(m, P(frame, sx * 0.5, Y_REAR, R_REAR - 0.01), P(frame, sx * 0.04, Y_REAR - 0.9, reach_z + 0.04),
                  (0.07, 0.06), face=D(frame, 0, 0, 1), strip='H2', chamfer=0.006)
    # The box: floor boards along it on two sills, sides of two teal boards under a weathered one, end gates.
    for sx in (-1.0, 1.0):
        lc.batten(m, P(frame, sx * 0.42, y0, FLOOR - 0.08), P(frame, sx * 0.42, y1, FLOOR - 0.08), (0.1, 0.1),
                  face=D(frame, 0, 0, 1), strip='C', chamfer=0.008)
    floor_n = 7
    for k in range(floor_n):
        x = -hw + (2 * hw) * (k + 0.5) / floor_n
        lc.bar(m, P(frame, x, y0, FLOOR - 0.015), P(frame, x, y1, FLOOR - 0.015), 2 * hw / floor_n - 0.006, 0.03,
               face=D(frame, 0, 0, 1), strip='A', chamfer=0.004, twist=rng.uniform(-0.6, 0.6))
    side_boards = [('H1', 0.0), ('H1', 0.19), ('A', 0.38)]
    for sx in (-1.0, 1.0):
        x = sx * (hw + 0.012)
        for k, (strip, z_off) in enumerate(side_boards):
            if broken and sx < 0 and k == 2:
                continue
            z = FLOOR + z_off + 0.09
            if broken and sx < 0 and k == 1:
                # Hanging by its back nails: dropped at the front.
                lc.bar(m, P(frame, x - 0.01, y0 + 0.4, z - 0.32), P(frame, x, y1, z), 0.18, 0.024, face=D(frame, sx, 0, 0),
                       strip=strip, chamfer=0.004)
                continue
            lc.bar(m, P(frame, x, y0 - 0.02, z), P(frame, x, y1 + 0.02, z), 0.18, 0.024, face=D(frame, sx, 0, 0),
                   strip=strip, chamfer=0.004, twist=rng.uniform(-0.5, 0.5), nicks=0.002)
        for k in range(5):
            y = y0 + 0.12 + (y1 - y0 - 0.24) * k / 4
            lc.bar(m, P(frame, x + sx * 0.016, y, FLOOR - 0.04), P(frame, x + sx * 0.016, y, FLOOR + SIDE), 0.04, 0.008,
                   face=D(frame, sx, 0, 0), strip='H3', chamfer=0.0015)
            lc.nail_heads(m, [P(frame, x + sx * 0.02, y, FLOOR + zz) for zz in (0.09, 0.28, 0.47)], D(frame, sx, 0, 0),
                          size=0.014)
    for y, front in ((y0 - 0.012, True), (y1 + 0.012, False)):
        if broken and not front:
            continue
        s = -1.0 if front else 1.0
        for k, (strip, z_off) in enumerate(side_boards):
            z = FLOOR + z_off + 0.09
            lc.bar(m, P(frame, -hw - 0.012, y, z), P(frame, hw + 0.012, y, z), 0.18, 0.024, face=D(frame, 0, s, 0),
                   strip=strip, chamfer=0.004, caps=(False, False))
        for sx in (-1.0, 1.0):
            lc.batten(m, P(frame, sx * (hw - 0.04), y + s * 0.02, FLOOR - 0.02), P(frame, sx * (hw - 0.04), y + s * 0.02,
                      FLOOR + SIDE - 0.02), (0.06, 0.03), face=D(frame, 0, s, 0), strip='C', chamfer=0.005)
    if broken:
        # The tailgate down in the dirt behind.
        tg = frame @ lc.matrix_of((0.15, y1 + 0.6, 0.02), yaw=12.0)
        for k in range(3):
            lc.bar(m, P(tg, -hw, 0.19 * k - 0.19, 0.012), P(tg, hw, 0.19 * k - 0.19, 0.012), 0.18, 0.024,
                   face=D(tg, 0, 0, 1), strip=side_boards[k][0], chamfer=0.004)
    else:
        # The tailgate's chains.
        for sx in (-1.0, 1.0):
            pts = [P(frame, sx * (hw + 0.02), y1 - 0.05, FLOOR + SIDE - 0.02),
                   P(frame, sx * (hw + 0.03), y1 + 0.03, FLOOR + SIDE - 0.12), P(frame, sx * (hw + 0.02), y1 + 0.03,
                                                                              FLOOR + SIDE - 0.2)]
            lc.rod(m, lc.spline(pts, per=3), 0.007, strip='H3', sides=4)
    # The spring seat over the front of the box.
    seat_y, seat_z = y0 + 0.35, FLOOR + SIDE + 0.24
    for sx in (-1.0, 1.0):
        pts = [P(frame, sx * (hw - 0.05), seat_y - 0.12, FLOOR + SIDE), P(frame, sx * (hw - 0.03), seat_y, FLOOR + SIDE + 0.17),
               P(frame, sx * (hw - 0.05), seat_y + 0.12, FLOOR + SIDE)]
        lc.tube(m, lc.spline(pts, per=2), lc.rect(0.04, 0.012), strip='H3', up=D(frame, 1, 0, 0))
    lc.bar(m, P(frame, -hw - 0.05, seat_y, seat_z), P(frame, hw + 0.05, seat_y, seat_z), 0.34, 0.035, face=D(frame, 0, 0, 1),
           strip='A', chamfer=0.008)
    back = frame @ lc.matrix_of((0.0, seat_y + 0.2, seat_z + 0.16), pitch=-12.0)
    lc.bar(m, P(back, -hw - 0.03, 0.0, 0.0), P(back, hw + 0.03, 0.0, 0.0), 0.2, 0.03, face=D(back, 0, -1, 0), strip='H1',
           chamfer=0.006)
    for sx in (-1.0, 1.0):
        lc.rod(m, [P(frame, sx * (hw - 0.02), seat_y + 0.12, seat_z), P(back, sx * (hw - 0.02), 0.0, -0.06)], 0.009,
               strip='H3', sides=5)
    # The brake: a lever by the seat, a beam with shoes before the rear wheels.
    lever_top = FLOOR + SIDE + (0.25 if canvas else 0.55)      # under a cover the lever is kept short of the canvas
    lc.rod(m, [P(frame, hw + 0.06, seat_y + 0.25, FLOOR + 0.1), P(frame, hw + 0.1, seat_y + 0.15, lever_top)], 0.013,
           strip='H3', sides=6)
    by = Y_REAR - R_REAR - 0.06
    lc.batten(m, P(frame, -TRACK - 0.04, by, 0.58), P(frame, TRACK + 0.04, by, 0.58), (0.08, 0.07), face=D(frame, 0, -1, 0),
              strip='C', chamfer=0.006)
    for sx in (-1.0, 1.0):
        lc.batten(m, P(frame, sx * TRACK, by + 0.03, 0.5), P(frame, sx * TRACK, by + 0.05, 0.68), (0.08, 0.07),
                  face=D(frame, 0, 1, 0), strip='C', chamfer=0.006)
    lc.rod(m, [P(frame, hw + 0.08, seat_y + 0.22, FLOOR + 0.12), P(frame, TRACK - 0.1, by - 0.02, 0.6)], 0.008, strip='H3',
           sides=4)
    # The tongue: from the front hounds forward to its tip on the ground, a doubletree across it.
    t0 = Vector((0.0, Y_FRONT - 0.2, R_FRONT - 0.02))
    tip = Vector((0.0, Y_FRONT - 2.45, 0.05))
    if broken:
        snap = t0.lerp(tip, 0.45)
        lc.batten(m, P(frame, *t0), P(frame, *(snap + Vector((0.0, 0.0, 0.03)))), (0.085, 0.075), face=D(frame, 0, 0, 1),
                  strip='C', chamfer=0.008)
        lc.batten(m, P(frame, 0.12, snap.y - 0.2, 0.04), P(frame, -0.05, tip.y, 0.04), (0.08, 0.07), face=D(frame, 0, 0, 1),
                  strip='C', chamfer=0.008)
    else:
        lc.batten(m, P(frame, *t0), P(frame, *tip), (0.085, 0.075), face=D(frame, 0, 0, 1), strip='C', chamfer=0.008,
                  segments=3)
        dt = t0.lerp(tip, 0.24)
        lc.batten(m, P(frame, -0.55, dt.y, dt.z + 0.06), P(frame, 0.55, dt.y, dt.z + 0.06), (0.07, 0.06),
                  face=D(frame, 0, -1, 0), strip='C', chamfer=0.006)
        lc.rod(m, [P(frame, 0.0, tip.y + 0.02, tip.z + 0.04), P(frame, 0.0, tip.y - 0.06, tip.z + 0.02)], 0.03, strip='H3',
               sides=6)
    for sx in (-1.0, 1.0):
        lc.batten(m, P(frame, sx * 0.42, Y_FRONT + 0.25, R_FRONT + 0.02), P(frame, sx * 0.06, Y_FRONT - 0.55, R_FRONT - 0.04),
                  (0.06, 0.05), face=D(frame, 0, 0, 1), strip='H2', chamfer=0.006)
    if broken:
        # Up on a block where the front-left wheel came off; the wheel leans on the box.
        lc.batten(m, P(frame, -TRACK, Y_FRONT, 0.0), P(frame, -TRACK, Y_FRONT, R_FRONT - 0.03), (0.24, 0.2),
                  face=D(frame, 0, -1, 0), strip='B', chamfer=0.02)
        lean = frame @ lc.matrix_of((-0.87, -0.3, 0.0), yaw=7.0, roll=14.0)
        wheel(m, lean, (0.0, 0.0, R_FRONT), R_FRONT, 12, seed + 6)
        hulls.append(('wheel_lean', lean, R_FRONT))
    if canvas:
        cover(m, frame, seed)
    hulls.append(('box', box_points(frame, (-hw - 0.1, y0 - 0.05, FLOOR - 0.15), (hw + 0.1, y1 + 0.05,
                                                                                FLOOR + (2.6 - FLOOR if canvas else SIDE)))))
    return hulls


def add_wagon_hulls(m, frame, hulls):
    for item in hulls:
        if item[0] == 'wheel':
            wheel_hull(m, frame, item[1], item[2])
        elif item[0] == 'wheel_lean':
            wheel_hull(m, item[1], (0.0, 0.0, item[2]), item[2])
        else:
            m.hull_points(item[1])


BOWS = (-1.2, -0.52, 0.16, 0.84, 1.5)
BOW_TOP = 2.6


def bow_shape(t, hw):
    """A bow's line across the box: t from -1 (left side top) to 1 (right side top); straight sides that round over."""
    a = (t + 1.0) * 0.5 * math.pi
    x = -math.cos(a) * (hw + 0.07)
    z = FLOOR + SIDE + (BOW_TOP - FLOOR - SIDE) * math.sin(a) ** 0.55
    return x, z


def cover(m, frame, seed):
    """Five bent ash bows over the box, a canvas cover (MI_Canvas) stretched over them, sagging between, drawn in at
    both ends round a puckered opening."""
    rng = random.Random(seed)
    hw = BOX[0]
    for y in BOWS:
        pts = []
        for k in range(11):
            t = -1.0 + 2.0 * k / 10
            x, z = bow_shape(t, hw)
            pts.append(P(frame, x, y, z - 0.03))
        lc.rod(m, pts, 0.016, strip='C', sides=6)
    # The sheet: rows along the wagon (j), columns across the bows (i).
    ys = []
    for k in range(len(BOWS) - 1):
        for s in range(3):
            ys.append(BOWS[k] + (BOWS[k + 1] - BOWS[k]) * s / 3)
    ys = [BOWS[0] - 0.28, BOWS[0] - 0.12] + ys + [BOWS[-1], BOWS[-1] + 0.12, BOWS[-1] + 0.28]
    cols = 15
    rows = []
    for j, y in enumerate(ys):
        # Sag between bows; gathered in at the two ends.
        nearest = min(abs(y - b) for b in BOWS)
        sag = 0.035 * math.sin(min(nearest / 0.34, 1.0) * math.pi * 0.5)
        over = max(0.0, BOWS[0] - y, y - BOWS[-1]) / 0.28
        gather = 1.0 - 0.32 * over ** 1.4
        row = []
        for i in range(cols + 1):
            t = -1.0 + 2.0 * i / cols
            x, z = bow_shape(t, hw)
            cx, cz = 0.0, FLOOR + SIDE + 0.55
            px = cx + (x - cx) * gather
            pz = cz + (z - cz) * gather if over > 0 else z
            ripple = 0.012 * math.sin(i * 1.9 + j * 0.7 + seed) * (0.4 + over)
            inward = Vector((px, 0.0, pz - (FLOOR + SIDE))).normalized() if over == 0 else Vector((0.0, 0.0, 0.0))
            row.append(P(frame, px - inward.x * (sag + ripple), y, pz - inward.z * (sag + ripple) + 0.012))
        rows.append(row)
    sheet(m, rows, 0.004, CANVAS.name, seed)


def sheet(m, rows, thick, key, seed, density=None):
    """A two-sided thin sheet through a grid of points on a tileable set (world scale, U along the rows' points)."""
    R, C = len(rows), len(rows[0])
    Pp = [[Vector(p) for p in row] for row in rows]
    scale = lc.set_scale(bpy.data.materials[key].get('TextureSet') or 'Polymer', density)
    du = [[0.0] * C for _ in range(R)]
    dv = [[0.0] * C for _ in range(R)]
    for j in range(R):
        for i in range(1, C):
            du[j][i] = du[j][i - 1] + (Pp[j][i] - Pp[j][i - 1]).length
    for i in range(C):
        for j in range(1, R):
            dv[j][i] = dv[j - 1][i] + (Pp[j][i] - Pp[j - 1][i]).length
    N = []
    for j in range(R):
        line = []
        for i in range(C):
            a = Pp[j][min(i + 1, C - 1)] - Pp[j][max(i - 1, 0)]
            b = Pp[min(j + 1, R - 1)][i] - Pp[max(j - 1, 0)][i]
            n = a.cross(b)
            line.append(n.normalized() if n.length > 1e-12 else UP)
        N.append(line)
    off = (seed * 0.13 % 1.0, seed * 0.29 % 1.0)
    cos, faces = [], []
    top, bot = [], []
    for j in range(R):
        top.append([]); bot.append([])
        for i in range(C):
            top[j].append(len(cos)); cos.append(Pp[j][i] + N[j][i] * thick * 0.5)
            bot[j].append(len(cos)); cos.append(Pp[j][i] - N[j][i] * thick * 0.5)
    for j in range(R - 1):
        for i in range(C - 1):
            n = N[j][i] + N[j + 1][i + 1]
            uvs = [((du[jj][ii] + off[0]) * scale, (dv[jj][ii] + off[1]) * scale)
                   for jj, ii in ((j, i), (j, i + 1), (j + 1, i + 1), (j + 1, i))]
            for layer, sign in ((top, 1.0), (bot, -1.0)):
                idx = [layer[j][i], layer[j][i + 1], layer[j + 1][i + 1], layer[j + 1][i]]
                mp = dict(zip(idx, uvs))
                idx = lc._orient(cos, idx, n * sign)
                faces.append((idx, [mp[q] for q in idx]))
    m.put(cos, faces, key=key)


def wagon_freight(name, seed, canvas=False, broken=False):
    m = lc.Model(name, seed)
    frame = Matrix.Identity(4)
    hulls = freight_wagon(m, frame, seed, canvas=canvas, broken=broken)
    if not canvas and not broken:
        # A light load at the back of the box: two sacks and a crate.
        for k, (x, y, yaw) in enumerate(((-0.22, 1.2, 84.0), (0.25, 1.15, 98.0))):
            sk = FC.Sack(0.84, 0.48, 0.19, seed * 10 + k)
            mx = lc.matrix_of((x, y, FLOOR + 0.08), yaw=yaw)
            sk.emit(m, mx, floor=FLOOR)
        top, hb = crate(m, lc.matrix_of((0.05, 0.45, FLOOR), yaw=6.0), 0.78, 0.6, 0.46, seed + 5, skids=False,
                        stencil=(("PRUITT'S", 0.06, 0.55, 'H2'),))
    add_wagon_hulls(m, frame, hulls)
    move = m.center_footprint()
    lt._log(f'CLUTTER {name}: the box\'s middle is {round(0.15 + move.y, 3)} m along Y from the pivot')
    if canvas:
        caption = 'The freight wagon with five ash bows and a\ncanvas cover drawn in at both ends.'
    elif broken:
        caption = 'The freight wagon abandoned: up on a block\nwhere a wheel came off, tongue snapped.'
    else:
        caption = 'A freight wagon: teal box, oxide-red gear and\nwheels, a spring seat, the tongue down.'
    return m.done('the middle of its footprint with the tongue', caption, ao=0.55, nanite=True, fallback=100.0)


def water_barrel_cart(name, seed):
    m = lc.Model(name, seed)
    rng = m.rng
    frame = Matrix.Identity(4)
    R, AX = 0.5, 0.18
    for sx in (-1.0, 1.0):
        wheel(m, frame, (sx * 0.6, AX, R), R, 12, seed + int(sx + 2), width=0.05, felloe=0.05, hub=0.07, hub_len=0.2,
              segs=16)
    lc.rod(m, [(-0.74, AX, R), (0.74, AX, R)], 0.022, strip='H3', sides=8)
    lc.batten(m, (-0.5, AX, R + 0.05), (0.5, AX, R + 0.05), (0.1, 0.09), face=(0.0, -1.0, 0.0), strip='H2', chamfer=0.008)
    rail_z = 0.64
    for sx in (-1.0, 1.0):
        pts = [Vector((sx * 0.42, 0.88, rail_z)), Vector((sx * 0.42, -0.4, rail_z)), Vector((sx * 0.36, -0.82, 0.34)),
               Vector((sx * 0.32, -1.22, 0.06)), Vector((sx * 0.32, -1.3, 0.07))]
        lc.rod(m, lc.spline(pts, per=3), 0.032, strip='C', sides=6,
               radii=[0.036] * 7 + [0.032, 0.03, 0.028, 0.026, 0.025, 0.024])
    for y in (-0.38, 0.86):
        lc.batten(m, (-0.46, y, rail_z), (0.46, y, rail_z), (0.07, 0.06), face=(0.0, 0.0, 1.0), strip='C', chamfer=0.006)
    # Two saddles cut to the barrel, the barrel on them, strapped down; its tap at the back.
    H, re, rm = 1.0, 0.3, 0.36
    rs = 0.346                         # the barrel's girth over the saddles (a quarter of its length from each end)
    base = rail_z + 0.03               # the saddles' foot, on the rails
    cz = base + 0.05 + rs              # 5 cm of saddle under the barrel at its lowest
    for y in (-0.25, 0.25):
        outline = [(-0.36, 0.0), (0.36, 0.0), (0.36, 0.13)]
        for k in range(7):
            a = math.radians(-26.0 + 52.0 * (6 - k) / 6)
            outline.append((math.sin(a) * (rs + 0.002), 0.05 + rs - math.cos(a) * (rs + 0.002)))
        outline.append((-0.36, 0.13))
        lc.prism_part(m, outline, 0.06, Matrix.Translation((0.0, y - 0.03, base)), uv=Trim('C'))
    f = lying((0.0, 0.5, cz), yaw=180.0, roll=0.0)
    barrel(m, f, H, re, rm, staves=16, hoops=(0.08, 0.28, 0.72, 0.92), bung=270.0, tap=True)
    for y in (-0.05, 0.4):
        rr = 0.3 + 0.06 * math.sin(math.pi * (0.5 - y)) ** 0.8 + 0.012
        pts = [Vector((math.sin(a) * rr, y, cz + math.cos(a) * rr)) for a in
               (math.radians(-100.0 + 200.0 * k / 12) for k in range(13))]
        lc.tube(m, pts, lc.rect(0.035, 0.006), strip='H3', up=(0.0, 1.0, 0.0), caps=(False, False))
    # A bucket on the ground under the tap.
    FC.bucket(m, (0.0, 0.7, 0.0), seed_turn=90.0, bail_down=0.4)
    m.hull((1.3, 0.3, 2 * R), at=(0.0, AX, R))
    m.hull_points([Vector((x, y, z)) for x in (-rm, rm) for y in (-0.5, 0.86) for z in (rail_z, cz + rm)])
    m.center_footprint()
    return m.done('the middle of its footprint with the shafts', 'A two-wheeled water cart: a barrel on saddles,\n'
                  'its tap at the back, a bucket under it.', ao=0.45)


# --- Handbills_A, Handbills_B (wall pieces: the pivot is the cluster's middle on the wall's face) ---

HANDBILLS = lt.material('Handbills')     # the handbills atlas (looter_posters.py --handbills), 1024 px/m

# (sheet, its middle x and z on the wall from the pivot (m), its turn in the wall's plane (degrees)), back to front.
HANDBILL_LAYOUTS = {
    'Handbills_A': [('Notice', -0.39, 0.0, -1.5), ('Auction', 0.03, -0.04, 2.5), ('Lost', 0.44, 0.13, -3.5),
                    ('Rooms', 0.43, -0.3, 1.5), ('Torn', -0.03, 0.33, -2.0)],
    'Handbills_B': [('Stage', -0.42, 0.02, 2.0), ('Hands', -0.05, -0.03, -1.5), ('Service', 0.36, 0.12, 3.0),
                    ('Sale', 0.4, -0.25, -2.5), ('Torn', -0.38, -0.33, 4.0)],
}


def handbill_sheet(m, name, cx, cz, turn, layer, seed):
    """One sheet on the wall (the wall's face at y = 0, the sheet toward -Y): its pinned corners flat on the wall, the
    rest lifting a little and the free corners curling off it; two-sided (its back on the atlas's plain paper)."""
    import looter_posters as lpo
    rng = random.Random(seed)
    W, H = (s * 0.001 for s in lpo.HANDBILL_CELLS[name][1])
    pins = [(x * 0.001, y * 0.001) for x, y in lpo.HANDBILL_PINS[name]]
    nx, nz = 6, 8
    a = math.radians(turn)
    ca, sa = math.cos(a), math.sin(a)
    corners = [(0.0, 0.0), (W, 0.0), (0.0, H), (W, H)]
    free = [c for c in corners if not any(math.hypot(c[0] - p[0], c[1] - p[1]) < 0.05 for p in pins)]
    curls = {c: (rng.uniform(0.5, 1.0), rng.uniform(0.28, 0.4) * min(W, H)) for c in free}
    lift0 = 0.0018 + 0.0013 * layer
    torn_top = name == 'Torn'
    cos, uvs_front, uvs_back = [], [], []
    for j in range(nz + 1):
        for i in range(nx + 1):
            s, t = W * i / nx, H * j / nz
            if torn_top and j == 0:
                t += rng.uniform(0.0, 0.02)
            # Off the wall: a slow bulge between the pins, the free corners curling out and in on themselves.
            near = min([math.hypot(s - p[0], t - p[1]) for p in pins] or [1.0])
            out = 0.004 * min(1.0, near / 0.12)
            shrink = (0.0, 0.0)
            for (cx0, cy0), (k, R) in curls.items():
                d = math.hypot(s - cx0, t - cy0)
                if d < R:
                    f = (1.0 - d / R) ** 2
                    out += k * f * R * 0.55
                    shrink = (shrink[0] + (W * 0.5 - cx0) * f * 0.08 * k, shrink[1] + (H * 0.5 - cy0) * f * 0.08 * k)
            if not pins:
                out += 0.01 * t / H
            ls, lt_ = s + shrink[0] - W * 0.5, H * 0.5 - (t + shrink[1])
            x = cx + ls * ca - lt_ * sa
            z = cz + ls * sa + lt_ * ca
            cos.append(Vector((x, -(lift0 + out), z)))
            uvs_front.append(lpo.handbill_uv(name, s * 1000.0, t * 1000.0))
            uvs_back.append(lpo.handbill_uv('Back', (1.0 - i / nx) * 200.0, (j / nz) * 230.0))
    count = len(cos)
    back = [c + Vector((0.0, 0.0004, 0.0)) for c in cos]
    faces = []
    for j in range(nz):
        for i in range(nx):
            q = [j * (nx + 1) + i, j * (nx + 1) + i + 1, (j + 1) * (nx + 1) + i + 1, (j + 1) * (nx + 1) + i]
            fi = lc._orient(cos + back, q, Vector((0.0, -1.0, 0.0)))
            faces.append((fi, [uvs_front[k] for k in fi]))
            bq = [k + count for k in q]
            bi = lc._orient(cos + back, bq, Vector((0.0, 1.0, 0.0)))
            faces.append((bi, [uvs_back[k - count] for k in bi]))
    m.put(cos + back, faces, key='Handbills', smooth=True)


def handbills(name, seed):
    m = lc.Model(name, seed)
    for layer, (sheet, x, z, turn) in enumerate(HANDBILL_LAYOUTS[name]):
        handbill_sheet(m, sheet, x, z, turn, layer, seed + layer)
    return m.done('the middle of the cluster on the wall\'s face (hang it with its middle ~1.45 m up)',
                  'Bills nailed up over each other, their free\ncorners curling (the handbills atlas).', ao=0.04,
                  ground=False, collision='None', lods='')


# --- The vignettes: kit pieces baked into one mesh (one draw a material) ---

def piece(builder, *args, **kw):
    """A kit piece built only to be merged (named '_V...', so never exported on its own; its report line dropped)."""
    obj = builder(*args, **kw)
    lc.REPORT.pop(obj.name, None)
    return obj


def lone_crate(name, seed, L, D_, H, **kw):
    m = lc.Model(name, seed)
    _, h = crate(m, Matrix.Identity(4), L, D_, H, seed, **kw)
    m.hull_points(h)
    return m.done('', '', ao=0.35)


def lone_barrel(name, seed, top='head', lid_on=True, tools=False, **kw):
    m = lc.Model(name, seed)
    barrel(m, Matrix.Identity(4), top=top, **kw)
    if top == 'open' and lid_on:
        # Its lid of three boards laid over it, a little askew.
        f = lc.matrix_of((0.02, -0.03, 0.925), yaw=12.0)
        for k, y in enumerate((-0.18, 0.0, 0.18)):
            lc.bar(m, P(f, -0.3, y, 0.0), P(f, 0.3, y, 0.0), 0.175, 0.022, face=D(f, 0, 0, 1), strip='A', chamfer=0.005)
        lc.bar(m, P(f, 0.0, -0.25, 0.022), P(f, 0.0, 0.25, 0.022), 0.06, 0.022, face=D(f, 0, 0, 1), strip='C',
               chamfer=0.005)
    FC.ring_hull(m, (0.0, 0.0, 0.0), 0.34, 0.0, 0.95)
    return m.done('', '', ao=0.4)


def coffee_set(name, seed):
    """A tin coffee pot and a cup (MailboxTin)."""
    m = lc.Model(name, seed)
    lc.turned(m, [(0.0, 0.0), (0.072, 0.0), (0.075, 0.01), (0.068, 0.12), (0.05, 0.17), (0.046, 0.19), (0.052, 0.2),
                  (0.03, 0.215), (0.0, 0.22)], 12, key='tin', grain='around', density=1024.0)
    lc.rod(m, [(0.05, 0.0, 0.05), (0.11, 0.0, 0.13), (0.12, 0.0, 0.16)], 0.008, key='tin', sides=5,
           radii=[0.012, 0.009, 0.006])
    lc.rod(m, [(-0.055, 0.0, 0.16), (-0.1, 0.0, 0.14), (-0.1, 0.0, 0.07), (-0.065, 0.0, 0.04)], 0.006, key='tin', sides=4)
    lc.turned(m, [(0.0, 0.0), (0.036, 0.0), (0.04, 0.075), (0.037, 0.078), (0.033, 0.006), (0.0, 0.006)], 10,
              at=(0.16, -0.08, 0.0), key='tin', grain='around', density=1024.0)
    return m.done('', '', ao=0.15, collision='None')


def leaf_drift(name, seed, size=(1.1, 0.55), count=46, swatches=('DryTan', 'Straw', 'GrassDry', 'CattailBrown')):
    """Dead leaves blown into a corner: small curled leaves on the ground, more toward the back (on FoliagePalette)."""
    m = lc.Model(name, seed)
    rng = m.rng
    lt.material('FoliagePalette')
    rows = len(lt.PALETTE)
    for k in range(count):
        x = rng.uniform(-size[0] * 0.5, size[0] * 0.5)
        y = -size[1] * (rng.random() ** 1.6)
        a = rng.uniform(0.0, TAU)
        ln, wd = rng.uniform(0.07, 0.12), rng.uniform(0.035, 0.06)
        d = Vector((math.cos(a), math.sin(a), 0.0))
        sd = Vector((-d.y, d.x, 0.0))
        c = Vector((x, y, 0.004 + 0.004 * (k % 3)))
        curl = rng.uniform(0.004, 0.012)
        pts = [c - d * ln * 0.5, c + sd * wd * 0.5 + UP * curl, c + d * ln * 0.5 + UP * curl * 0.5,
               c - sd * wd * 0.5 + UP * curl]
        sw = lt.PALETTE.index(rng.choice(swatches))
        v_lo, v_hi = (sw + 0.15) / rows, (sw + 0.85) / rows
        uv = [(0.05, (v_lo + v_hi) * 0.5), (0.5, v_hi), (0.95, (v_lo + v_hi) * 0.5), (0.5, v_lo)]
        idx = lc._orient(pts, [0, 1, 2, 3], UP)
        mp = dict(zip(range(4), uv))
        m.put(pts, [(idx, [mp[i] for i in idx])], key='FoliagePalette', smooth=True)
    return m.done('', '', ao=0.1, collision='None')


def tumbleweed(name, seed, radius=0.3):
    """A dead tumbleweed caught against a wall: a loose ball of thin bleached twigs (the Hay set)."""
    m = lc.Model(name, seed)
    rng = m.rng
    c = Vector((0.0, 0.0, radius * 0.82))
    # Wiry twigs curling round the ball's surface (arcs of great circles at a few depths), a few spokes to its middle.
    for k in range(46):
        a, b = rng.uniform(0.0, TAU), math.acos(rng.uniform(-1.0, 1.0))
        d = Vector((math.cos(a) * math.sin(b), math.sin(a) * math.sin(b), math.cos(b)))
        t = d.cross(Vector((rng.uniform(-1, 1), rng.uniform(-1, 1), rng.uniform(-1, 1)))).normalized()
        r = radius * rng.uniform(0.7, 1.0)
        span = rng.uniform(0.6, 1.3)
        pts = []
        for j in range(4):
            ang = span * (j / 3 - 0.5)
            p = (d * math.cos(ang) + t * math.sin(ang)) * r
            pts.append(c + Vector((p.x, p.y, p.z * 0.85)))
        lc.rod(m, pts, 0.0035, key='hay', sides=3, radii=[0.004, 0.0035, 0.003, 0.002], caps=(False, False))
    for k in range(8):
        a, b = rng.uniform(0.0, TAU), math.acos(rng.uniform(-1.0, 1.0))
        d = Vector((math.cos(a) * math.sin(b), math.sin(a) * math.sin(b), math.cos(b)))
        lc.rod(m, [c + d * 0.03, c + d * radius * 0.95], 0.005, key='hay', sides=3, radii=[0.006, 0.003],
               caps=(False, False))
    return m.done('', '', ao=0.15, collision='None', lods='')


def firewood_rick(name, seed, length=1.5, height=0.75):
    """A rick of split firewood between two pairs of stakes, on two poles: wedge-split billets (bark and split face
    on BarkOak and the trim's debarked log strip, their sawn ends on WoodEndGrain)."""
    m = lc.Model(name, seed)
    rng = m.rng
    for y in (-0.17, 0.17):
        lc.batten(m, (-length * 0.5 - 0.1, y, 0.04), (length * 0.5 + 0.1, y, 0.04), (0.08, 0.08), face=(0.0, 0.0, 1.0),
                  strip='B', chamfer=0.01)
    for x in (-length * 0.5 - 0.04, length * 0.5 + 0.04):
        for y in (-0.21, 0.21):
            lc.batten(m, (x, y, -0.05), (x + rng.uniform(-0.02, 0.02), y * 1.05, height + 0.12), (0.06, 0.06),
                      face=(0.0, -1.0, 0.0), strip='C', chamfer=0.008)
    # Courses of split sticks packed as the backlots' woodshed packs them (Backlots.py's firewood(): halves, quarters,
    # thirds and the odd round, bark on BarkOak, split faces on the debarked log strip, sawn ends on WoodEndGrain),
    # here with both ends out and each stick a little proud or shy of its neighbours.
    kinds = ['half', 'quarter', 'third', 'half', 'quarter', 'round']
    x0, x1 = -length * 0.5 + 0.02, length * 0.5 - 0.02
    z = 0.09
    for course in range(9):
        x = x0
        tallest = 0.0
        while x < x1 - 0.08:
            kind = rng.choice(kinds)
            radius = rng.uniform(0.07, 0.1) if kind != 'round' else rng.uniform(0.045, 0.06)
            turn = rng.uniform(-math.pi, math.pi)
            pts = BL.stick_outline(kind, radius, turn)
            lo_u, hi_u, lo_w, hi_w = BL.outline_box(pts)
            w = hi_u - lo_u
            if x + w * 0.92 > x1:
                break
            depth = rng.uniform(0.36, 0.42)
            BL.firewood(m, (x - lo_u, -depth * 0.5 + rng.uniform(-0.025, 0.025), z - lo_w - rng.uniform(0.0, 0.015)),
                        depth, radius, kind, turn, rng, back=True)
            x += w * 0.9
            tallest = max(tallest, (hi_w - lo_w) * 0.86)
        z += tallest
        if z > height:
            break
    m.hull((length + 0.2, 0.48, z + 0.05), at=(0.0, 0.0, (z + 0.05) * 0.5))
    return m.done('', '', ao=0.4)


V_MATS = {}


def vignette_shopfront_a(name, seed):
    """Before a store's window on the walk: a lidded barrel with a broom and bucket by it, a crate with a smaller one
    askew on it, two flour sacks against the crate, a crate of empties."""
    parts = [
        (piece(lone_barrel, '_V_Barrel', seed + 1, top='open', lid_on=True), lc.matrix_of((-0.75, -0.4, 0.0), yaw=20.0)),
        (piece(FC.broom_bucket, '_V_Broom', 4207), lc.matrix_of((-0.2, 0.0, 0.0), yaw=0.0)),
        (piece(lone_crate, '_V_Crate1', seed + 3, 0.72, 0.56, 0.48, stencil=(('ORRIN BROS.', 0.05, 0.55, 'H2'),)),
         lc.matrix_of((0.55, -0.34, 0.0), yaw=-4.0)),
        (piece(lone_crate, '_V_Crate2', seed + 4, 0.5, 0.4, 0.34, lid='askew'), lc.matrix_of((0.5, -0.36, 0.48), yaw=9.0)),
        (piece(two_sacks, '_V_Sacks', seed + 5), lc.matrix_of((0.55, -0.78, 0.0))),
        (piece(FC.bottle_crate, '_V_Bottles', 4215), lc.matrix_of((-0.62, -0.95, 0.0), yaw=-14.0)),
    ]
    return lc.merge(name, parts, 'on the wall at its foot (the walk\'s top, the wall\'s face): the props stand out to '
                    '1.2 m toward -Y', 'A store front: a lidded barrel, broom and bucket,\ncrates, flour sacks, a crate '
                    'of empties.', ao=0.45)


def two_sacks(name, seed):
    """Two flour sacks before a crate: one lying, one stood on end leaning back on the crate (the front -Y)."""
    m = lc.Model(name, seed)
    lines = [(FC.MILL[0], 0.055, 0.07, 'H2'), (FC.MILL[1], 0.085, -0.05, 'H1')]
    lying_m = lc.matrix_of((-0.12, -0.18, 0.085), yaw=-8.0)
    s1 = FC.Sack(0.86, 0.48, 0.19, seed * 10 + 1)
    s1.emit(m, lying_m)
    s1.print_on(m, lying_m, lines)
    stood = (Matrix.Translation((0.22, -0.02, 0.42)) @ Matrix.Rotation(math.radians(10.0), 4, 'Z')
             @ Matrix.Rotation(math.radians(9.0), 4, 'X')
             @ Matrix(((0.0, -1.0, 0.0, 0.0), (0.0, 0.0, -1.0, 0.0), (1.0, 0.0, 0.0, 0.0), (0.0, 0.0, 0.0, 1.0))))
    s2 = FC.Sack(0.84, 0.48, 0.19, seed * 10 + 2)
    s2.emit(m, stood)
    s2.print_on(m, stood, lines, upright=True)
    m.hull_points([Vector((x, y, z)) for x in (-0.55, 0.45) for y in (-0.45, 0.08) for z in (0.0, 0.2)] +
                  [Vector((x, y, 0.84)) for x in (0.0, 0.4) for y in (-0.15, 0.05)])
    return m.done('', '', ao=0.3)


def vignette_shopfront_b(name, seed):
    """Before a feed store or a smithy: two barrels, slumped oats sacks, a long crate, tools in a barrel."""
    parts = [
        (piece(lone_barrel, '_V_BarrelA', seed + 1), lc.matrix_of((-0.95, -0.38, 0.0))),
        (piece(lone_barrel, '_V_BarrelB', seed + 2, top='open', lid_on=False, staves=16), lc.matrix_of((-0.3, -0.36, 0.0))),
        (piece(sack_pile_b, '_V_Sacks', seed + 3), lc.matrix_of((0.55, -0.5, 0.0), yaw=-6.0)),
        (piece(lone_crate, '_V_Long', seed + 4, 1.2, 0.46, 0.4, stencil=(('FEED', 0.07, 0.55, 'H2'),)),
         lc.matrix_of((-0.62, -0.97, 0.0), yaw=4.0)),
    ]
    return lc.merge(name, parts, 'on the wall at its foot (ground or walk, the wall\'s face): out to 1.35 m toward -Y',
                    'A feed store\'s front: barrels, slumped sacks\nof oats, a long crate of feed.', ao=0.45)


def vignette_porch_lived(name, seed):
    """A lived-in porch: the rocker with its shawl, a crate for a side table with a coffee pot and cup, a broom and
    bucket by the door."""
    table = piece(lone_crate, '_V_Table', seed + 1, 0.42, 0.34, 0.4)
    parts = [
        (piece(FC.rocking_chair, '_V_Rocker', 4201, shawl=True), lc.matrix_of((-0.55, -0.62, 0.0), yaw=-18.0)),
        (table, lc.matrix_of((0.08, -0.5, 0.0), yaw=7.0)),
        (piece(coffee_set, '_V_Coffee', seed + 2), lc.matrix_of((0.06, -0.52, 0.4), yaw=30.0)),
        (piece(FC.broom_bucket, '_V_Broom', 4207), lc.matrix_of((0.78, 0.0, 0.0), yaw=0.0)),
    ]
    return lc.merge(name, parts, 'on the wall at its foot (the porch floor, the wall\'s face): out to 1.1 m toward -Y',
                    'A lived porch: the rocker and its shawl, a crate\nfor a table with the coffee pot, the broom.',
                    ao=0.4)


def vignette_porch_empty(name, seed):
    """An empty house's porch: a chair tipped on its side, a broken crate, two boards fallen against the wall, dead
    leaves and a tumbleweed blown into the corner."""
    parts = [
        (piece(FC.porch_chair, '_V_Chair', 4203), lc.matrix_of((-0.4, -0.75, 0.24), yaw=20.0, roll=-88.0)),
        (piece(lone_crate, '_V_Broken', seed + 1, 0.6, 0.45, 0.4, lid='off',
               gone=(('front', 1), ('right', 0), ('right', 1))), lc.matrix_of((0.42, -0.42, 0.0), yaw=-12.0)),
        (piece(fallen_against, '_V_Boards', seed + 2), Matrix.Identity(4)),
        (piece(leaf_drift, '_V_Leaves', seed + 3), lc.matrix_of((0.1, -0.02, 0.0))),
        (piece(tumbleweed, '_V_Tumble', seed + 4), lc.matrix_of((1.05, -0.3, 0.0))),
    ]
    obj = lc.merge(name, parts, 'on the wall at its foot (the porch floor, the wall\'s face): out to 1.15 m toward -Y',
                   'An empty porch: a tipped chair, a broken crate,\nboards down, leaves and a tumbleweed.', ao=0.4)
    lt.set_foliage_colors(obj, wind=0.0, variation='island', seed=seed)
    return obj


def fallen_against(name, seed):
    """Two weathered boards that came off the wall, leaning against it."""
    m = lc.Model(name, seed)
    rng = m.rng
    for x, top, foot_y, w in ((-1.05, 1.35, -0.42, 0.19), (-0.78, 1.1, -0.36, 0.17)):
        lc.bar(m, (x + rng.uniform(-0.05, 0.05), foot_y, 0.0), (x + 0.12, -0.03, top), w, 0.022, face=(0.15, -1.0, 0.3),
               strip='A', chamfer=0.005, twist=rng.uniform(-4.0, 4.0), nicks=0.003)
    return m.done('', '', ao=0.3)


def vignette_alley(name, seed):
    """An alley between two buildings: a barrel on its side against the wall, an upright one, a broken crate, boards
    leaning on the wall, a bucket."""
    m = lc.Model('_V_AlleyBoards', seed)
    rng = m.rng
    for k, (x, top) in enumerate(((0.55, 1.7), (0.72, 1.55), (0.86, 1.8))):
        foot = Vector((x, -0.42 - 0.05 * k, 0.0))
        rest = Vector((x + 0.06, -0.03, top))
        m.board(foot, rest, 0.2 - 0.01 * k, 0.025, face=(0.0, -1.0, 0.25), uv=lr.PlankRow(), mat='planks')
    boards = m.done('', '', ao=0.3)
    lc.REPORT.pop(boards.name, None)
    m2 = lc.Model('_V_Bucket', seed + 9)
    FC.bucket(m2, (0.0, 0.0, 0.0), seed_turn=30.0, bail_down=-1.0)
    bucket_obj = m2.done('', '', ao=0.25)
    lc.REPORT.pop(bucket_obj.name, None)
    m3 = lc.Model('_V_Lying', seed + 7)
    barrel(m3, lying((0.0, -0.46, 0.33), yaw=8.0, roll=40.0), bung=90.0)
    m3.hull((1.0, 0.7, 0.66), at=(0.0, 0.0, 0.33))
    lying_obj = m3.done('', '', ao=0.4)
    lc.REPORT.pop(lying_obj.name, None)
    parts = [
        (lying_obj, lc.matrix_of((-0.75, -0.36, 0.0), yaw=90.0)),
        (piece(lone_barrel, '_V_Upright', seed + 1), lc.matrix_of((0.08, -0.36, 0.0))),
        (piece(lone_crate, '_V_Broken', seed + 2, 0.56, 0.42, 0.38, lid='off', gone=(('front', 0), ('left', 1))),
         lc.matrix_of((-0.55, -1.0, 0.0), yaw=24.0)),
        (boards, Matrix.Identity(4)),
        (bucket_obj, lc.matrix_of((0.55, -0.85, 0.0))),
    ]
    return lc.merge(name, parts, 'on the wall at its foot (the alley\'s ground, the wall\'s face): out to 1.2 m '
                    'toward -Y', 'An alley: a barrel lying by the wall, one\nstanding, a broken crate, boards, a bucket.',
                    ao=0.45)


def vignette_backyard(name, seed):
    """A back yard's wood corner: a rick of firewood between stakes, the chopping block with its axe, split billets
    about, a bucket."""
    m2 = lc.Model('_V_Bucket', seed + 9)
    FC.bucket(m2, (0.0, 0.0, 0.0), seed_turn=-20.0, bail_down=1.0)
    bucket_obj = m2.done('', '', ao=0.25)
    lc.REPORT.pop(bucket_obj.name, None)
    parts = [
        (piece(firewood_rick, '_V_Rick', seed + 1), lc.matrix_of((0.0, 0.45, 0.0))),
        (piece(FC.chopping_block, '_V_Block', 4209), lc.matrix_of((0.35, -0.5, 0.0), yaw=-25.0)),
        (bucket_obj, lc.matrix_of((-0.75, -0.45, 0.0))),
    ]
    return lc.merge(name, parts, 'the ground under the group\'s middle (the rick along X behind, toward +Y)',
                    'A back yard\'s wood corner: a firewood rick, the\nchopping block and its axe, a bucket.', ao=0.45)


# --- SackPile_B ---

def sack_pile_b(name, seed):
    m = lc.Model(name, seed)
    rng = m.rng
    lines = [(FC.MILL[0], 0.055, 0.07, 'H2'), ('OATS', 0.085, -0.05, 'H1')]
    placed = [((-0.28, 0.1, 0.075), 88.0, 0.0, 0.16, 0.0, None), ((0.27, 0.12, 0.07), 94.0, 0.0, 0.15, 0.0, None),
              ((-0.02, 0.18, 0.2), 6.0, 5.0, 0.13, 0.14, 'lying'), ((0.4, -0.3, 0.06), -30.0, 8.0, 0.12, 0.0, None)]
    for k, ((x, y, z), yaw, pitch, thick, floor, printed) in enumerate(placed):
        sack = FC.Sack(0.82, 0.48, thick, seed * 10 + k)
        # Slumped: half empty, the top fallen in toward one end.
        for key in sack.top:
            p = sack.top[key]
            t = (p.x / (sack.L * 0.5) + 1.0) * 0.5
            p.z -= thick * 0.35 * t * max(0.0, 1.0 - abs(p.y) / (sack.W * 0.5)) ** 0.6
        matrix = lc.matrix_of((x, y, z), yaw=yaw, pitch=pitch)
        sack.emit(m, matrix, floor=floor, u0=5.86 + 0.02 * k, v0=0.05 + 0.05 * k)
        if printed:
            sack.print_on(m, matrix, lines, floor=floor)
    # Hay spilled round the pile's foot and loose straws.
    mound(m, Vector((0.05, -0.36, 0.0)), 0.26, 0.03, seed, key='hay', ring=16, squash=1.6, wobble=0.6)
    mound(m, Vector((-0.42, -0.5, 0.0)), 0.12, 0.018, seed + 1, key='hay', ring=10, squash=1.3, wobble=0.6)
    for k in range(18):
        a = rng.uniform(0.0, TAU)
        c = Vector((rng.uniform(-0.55, 0.6), rng.uniform(-0.62, -0.2), 0.006))
        d = Vector((math.cos(a), math.sin(a), 0.0))
        lc.rod(m, [c - d * 0.07, c + d * 0.07], 0.0025, key='hay', sides=3)
    m.hull_points([Vector((x, y, z)) for x, y in ((-0.6, 0.45), (0.58, 0.45), (0.7, -0.5), (-0.5, -0.3)) for z in (0.0, 0.3)] +
                  [Vector((x, y, 0.42)) for x, y in ((-0.3, 0.3), (0.25, 0.3), (-0.3, 0.05), (0.25, 0.05))])
    return m.done('the ground under the pile\'s middle', 'Half-empty sacks of oats slumped together, hay\n'
                  'spilled round them.', ao=0.35)


BUILDERS = [
    ('CrateStack_A', lambda: crate_stack_a('CrateStack_A', 5101)),
    ('CrateStack_B', lambda: crate_stack_b('CrateStack_B', 5102)),
    ('Crate_C', lambda: crate_c('Crate_C', 5103)),
    ('BarrelStack_A', lambda: barrel_stack_a('BarrelStack_A', 5104)),
    ('Keg_Trestle', lambda: keg_trestle('Keg_Trestle', 5105)),
    ('ToolBarrel', lambda: tool_barrel('ToolBarrel', 5106)),
    ('ToolsLeaning_A', lambda: tools_leaning_a('ToolsLeaning_A', 5107)),
    ('Ladder_A', lambda: ladder_a('Ladder_A', 5108)),
    ('Sawhorse_Log', lambda: sawhorse_log('Sawhorse_Log', 5109)),
    ('LumberPile_Small', lambda: lumber_pile_small('LumberPile_Small', 5110)),
] + [(s[0], (lambda s=s: sign(*s))) for s in SIGNS] + [
    ('BarrelFire', lambda: barrel_fire('BarrelFire', 5125)),
    ('WagonFreight', lambda: wagon_freight('WagonFreight', 5126)),
    ('WagonFreight_Canvas', lambda: wagon_freight('WagonFreight_Canvas', 5126, canvas=True)),
    ('WagonFreight_Broken', lambda: wagon_freight('WagonFreight_Broken', 5126, broken=True)),
    ('WaterBarrel_Cart', lambda: water_barrel_cart('WaterBarrel_Cart', 5127)),
    ('SackPile_B', lambda: sack_pile_b('SackPile_B', 5111)),
    ('Handbills_A', lambda: handbills('Handbills_A', 5131)),
    ('Handbills_B', lambda: handbills('Handbills_B', 5132)),
    ('Vignette_ShopFront_A', lambda: vignette_shopfront_a('Vignette_ShopFront_A', 5141)),
    ('Vignette_ShopFront_B', lambda: vignette_shopfront_b('Vignette_ShopFront_B', 5142)),
    ('Vignette_PorchLived_A', lambda: vignette_porch_lived('Vignette_PorchLived_A', 5143)),
    ('Vignette_PorchEmpty_A', lambda: vignette_porch_empty('Vignette_PorchEmpty_A', 5144)),
    ('Vignette_Alley_A', lambda: vignette_alley('Vignette_Alley_A', 5145)),
    ('Vignette_BackYard_A', lambda: vignette_backyard('Vignette_BackYard_A', 5146)),
]


def build():
    return {name: make() for name, make in BUILDERS if ONLY is None or name in ONLY}


VIEWS = {   # per piece: view (toward the camera), lens, sun (azimuth, elevation), margin
    'BarrelStack_A': ((-0.55, -1.0, 0.4), 40.0, (40.0, 24.0), 1.06),
    'WagonFreight': ((-1.0, -0.8, 0.42), 35.0, (45.0, 22.0), 1.04),
    'WagonFreight_Canvas': ((-1.0, -0.8, 0.35), 35.0, (45.0, 22.0), 1.04),
    'WagonFreight_Broken': ((-1.0, -0.7, 0.42), 35.0, (45.0, 22.0), 1.04),
    'WaterBarrel_Cart': ((-1.0, -0.6, 0.45), 40.0, (45.0, 24.0), 1.06),
    'BarrelFire': ((-0.6, -1.0, 0.6), 45.0, (40.0, 26.0), 1.15),
    'Sawhorse_Log': ((-0.7, -1.0, 0.5), 40.0, (40.0, 24.0), 1.08),
}
WALL_PIECES = ('ToolsLeaning_A', 'Ladder_A', 'Vignette_ShopFront_A', 'Vignette_ShopFront_B', 'Vignette_PorchLived_A',
               'Vignette_PorchEmpty_A', 'Vignette_Alley_A')
HUNG = {'Handbills_A': 1.45, 'Handbills_B': 1.45}     # wall pieces whose pivot hangs this high on the wall
LINEUP_ROWS = [['WagonFreight', 'WagonFreight_Canvas', 'WagonFreight_Broken', 'WaterBarrel_Cart'],
               ['BarrelStack_A', 'CrateStack_B', 'LumberPile_Small', 'Keg_Trestle', 'Sawhorse_Log', 'Vignette_BackYard_A'],
               ['CrateStack_A', 'Crate_C', 'SackPile_B', 'ToolBarrel', 'BarrelFire']]


def lineup(built, rows, out, view=(-0.25, -1.0, 0.75), size=(2400, 1200)):
    """Pieces at one scale in rows, a 1.8 m figure at the front right."""
    figure = homes.figure()
    shown = []
    y = 0.0
    for row in rows:
        x, depth, placed = 0.0, 0.0, []
        for name in row:
            if name not in built:
                continue
            c = lc.copy_of(built[name], turn=-90.0 if name.startswith('Wagon') or name == 'WaterBarrel_Cart' else 0.0)
            bpy.context.view_layer.update()
            pts = [c.matrix_world @ v.co for v in c.data.vertices]
            lo = Vector((min(p.x for p in pts), min(p.y for p in pts), 0.0))
            hi = Vector((max(p.x for p in pts), max(p.y for p in pts), 0.0))
            c.location = (x - lo.x, y - hi.y, 0.0)
            x += hi.x - lo.x + 0.5
            depth = max(depth, hi.y - lo.y)
            placed.append(c)
        for c in placed:
            c.location.x -= (x - 0.5) * 0.5
        shown += placed
        y -= depth + 0.7
    figure.location = (shown[-1].location.x + 1.3 if shown else 0.0, y + 0.9, 0.0)
    bpy.context.view_layer.update()
    path = lc.shoot(out, shown + [figure], view=view, lens=35.0, size=size, sun=(35.0, 30.0), margin=1.03, bias=0.3)
    lc.clear_copies()
    bpy.data.objects.remove(figure)
    return path


WALL_ROW = ['Vignette_ShopFront_A', 'Handbills_A', 'Vignette_ShopFront_B', 'ToolsLeaning_A', 'Ladder_A',
            'Vignette_PorchLived_A', 'Handbills_B', 'Vignette_PorchEmpty_A', 'Vignette_Alley_A']


def wall_lineup(built):
    """The wall pieces along one long wall, a 1.8 m figure among them."""
    names = [n for n in WALL_ROW if n in built]
    widths = {}
    for n in names:
        xs = [v.co.x for v in built[n].data.vertices]
        widths[n] = max(xs) - min(xs)
    total = sum(widths.values()) + 0.4 * (len(names) - 1)
    wall = FC.backdrop('_WallLong', total + 2.0, 4.0, finish='trim')
    x = -total * 0.5
    shown = []
    for n in names:
        xs = [v.co.x for v in built[n].data.vertices]
        c = lc.copy_of(built[n], (x - min(xs), 0.0, HUNG.get(n, 0.0)))
        shown.append(c)
        x += widths[n] + 0.4
    figure = homes.figure()
    figure.location = (total * 0.5 + 0.6, -0.8, 0.0)
    bpy.context.view_layer.update()
    path = lc.shoot('Lineup_Walls', shown + [figure], [wall], view=(-0.2, -1.0, 0.35), lens=35.0, size=(2400, 900),
                    sun=(35.0, 28.0), margin=1.03, bias=0.4)
    lc.clear_copies()
    bpy.data.objects.remove(figure)
    return path


EXPORT = os.path.join(lt.REPO, 'Intermediate', 'ArtExport_TownProps')


def variants():
    """The variants added to Boardwalk.py, VillageProps.py and Containers.py, from this round's export (run the export
    first): the worn walks chained with the plain one, the steps; the three lamps (B on a building's corner); the
    barrels."""
    def fbx(name):
        return lc.import_fbx(os.path.join(EXPORT, f'SM_{name}.fbx'))
    lt.material('WoodPlanks', name='WoodHoney', tint=0xf2d6a4)       # Boardwalk.py's replaced boards (import_fbx
    lt.material('WoodPlanks')                                         # swaps the session's textured ones in by name)
    shots = {}
    walks = [fbx(n) for n in ('Boardwalk_4m', 'Boardwalk_4m_Worn', 'Boardwalk_4m_Patched', 'Boardwalk_Steps_Worn')]
    if all(w is not None for w in walks):
        for k, w in enumerate(walks[:3]):
            w.location = (4.0 * k - 6.0, 0.0, 0.0)
        walks[3].location = (6.0, 0.0, 0.0)
        bpy.context.view_layer.update()
        wall = FC.backdrop('_WallWalk', 14.0, 3.0, finish='trim', x=0.0)
        wall.location = (0.0, 1.5, 0.0)
        bpy.context.view_layer.update()
        shots['Variants_Boardwalk'] = lc.shoot('Variants_Boardwalk', walks, [wall], view=(-0.3, -1.0, 0.55), lens=35.0,
                                               size=(2200, 900), sun=(30.0, 24.0), margin=1.03, bias=0.3)
        shots['Variants_Boardwalk_Close'] = lc.shoot('Variants_Boardwalk_Close', walks[1:3], [wall] + walks,
                                                     eye=Vector((-0.6, -3.4, 1.65)), target=Vector((0.9, 0.0, 0.3)),
                                                     lens=28.0, size=(1600, 1000), sun=(30.0, 24.0))
        for w in walks:
            w.hide_render = True
    lamps = [fbx(n) for n in ('LampPost', 'LampPost_B', 'LampPost_C')]
    if all(l is not None for l in lamps):
        lamps[0].location = (-2.2, 0.0, 0.0)
        lamps[2].location = (2.2, 0.0, 0.0)
        # B on a building's outside corner 3 m up: two walls meeting at the pivot, the building behind (+X +Y).
        m = kit.Model('_Corner', 1)
        m.box((1.6, 0.12, 4.0), at=(0.8, 0.06, 2.0), uv=Trim('A', world=True))
        m.box((0.12, 1.6, 4.0), at=(0.06, 0.8, 2.0), uv=Trim('A', world=True))
        corner = m.finish(ao=False, preview=False)
        corner.hide_render = True
        lamps[1].location = (0.0, 0.0, 3.0)
        corner.location = (0.0, 0.0, 0.0)
        bpy.context.view_layer.update()
        shots['Variants_Lamps'] = lc.shoot('Variants_Lamps', lamps, [corner], view=(-0.45, -1.0, 0.25), lens=35.0,
                                           size=(1800, 1100), sun=(35.0, 26.0), margin=1.08, bias=0.5)
        for l in lamps:
            l.hide_render = True
    barrels = [fbx(n) for n in ('Barrel_A', 'Barrel_C')]
    if all(b is not None for b in barrels):
        barrels[0].location = (-0.5, 0.0, 0.0)
        barrels[1].location = (0.5, 0.0, 0.0)
        bpy.context.view_layer.update()
        shots['Variants_Barrels'] = lc.shoot('Variants_Barrels', barrels, view=(-0.5, -1.0, 0.6), lens=45.0,
                                             size=(1200, 860), sun=(40.0, 26.0), margin=1.12)
    return shots


def previews(built):
    lc.town_materials()
    lc.stage(samples=32)
    shots = {}
    wall = FC.backdrop('_WallTown', 4.0, 4.2, finish='trim')
    siding = FC.backdrop('_WallSiding', 3.0, 4.2, finish='trim')
    bracket = None
    for name, obj in built.items():
        if name.startswith('SignBoard'):
            if bracket is None:
                bracket = FC.sign_bracket('_SignBracket', 4218)
                lc.REPORT.pop('_SignBracket', None)
                bracket.location = (0.0, 0.0, 3.0)
                bpy.context.view_layer.update()
            board = FC.hang(bracket, obj, swing=4.0)
            shots[name] = lc.shoot(name, [bracket, board], [siding], view=(-1.0, -0.62, 0.02), lens=50.0,
                                   sun=(-60.0, 24.0), margin=1.12, bias=0.5, size=(1200, 860))
            lc.clear_copies()
        elif name in HUNG:
            obj.location = (0.0, 0.0, HUNG[name])
            bpy.context.view_layer.update()
            shots[name] = lc.shoot(name, [obj], [wall], view=(-0.35, -1.0, 0.05), lens=50.0, sun=(30.0, 26.0),
                                   margin=1.15, size=(1200, 900), bias=0.5)
            shots[name + '_Close'] = lc.shoot(name + '_Close', [obj], [wall], view=(-0.15, -1.0, 0.0), lens=50.0,
                                              sun=(30.0, 26.0), margin=0.55, size=(1200, 900), bias=0.55)
            obj.location = (0.0, 0.0, 0.0)
        elif name in WALL_PIECES:
            shots[name] = lc.shoot(name, [obj], [wall], view=(-0.55, -1.0, 0.3), lens=40.0, sun=(30.0, 26.0),
                                   margin=1.1, size=(1000, 1100) if name == 'Ladder_A' else (1200, 860))
        else:
            view, lens, sun, margin = VIEWS.get(name, ((-0.6, -1.0, 0.5), 45.0, (40.0, 24.0), 1.1))
            shots[name] = lc.shoot(name, [obj], view=view, lens=lens, sun=sun, margin=margin, size=(1200, 860))
    if ONLY is None or len(built) > 6:
        shots['Lineup_TownClutter'] = lineup(built, LINEUP_ROWS, 'Lineup_TownClutter')
    if ONLY is None:
        shots['Lineup_Walls'] = wall_lineup(built)
    if '--variants' in ARGS:
        shots.update(variants())
    lt._log('CLUTTER previews: ' + ', '.join(sorted(shots)))
    return shots


if __name__ == '__main__':
    BUILT = build()
    total = sum(info['triangles'] for info in lc.REPORT.values())
    lt._log(f'CLUTTER: {len(lc.REPORT)} models, {total:,} triangles')
    if lt.want_preview():
        lc.write_report()
        previews(BUILT)
