"""The lootable world (Docs/Polish/BorderlandsComparison.md, item 10): the breakable crates' and barrel's pieces, broken in
advance, and the stumps they leave; and the containers opened for loot: a coffin with a loose lid, a grave dug open, a
mailbox, a footlocker. A scripted model file (Art/README.md); Tools/models.ps1 exports it into /Game/Art/Props.

Breakables (Source/AI_Looter_Shooter/World/BreakableProp: the dressing's Containers.py crates and barrel break into these):
  Break_CrateA_1..7     Crate_A's (0.9 m slatted crate) pieces: its upper slats front, back and ends with the posts' tops,
                        its top planks in pairs, a brace's broken half; each splintered where it broke
  Break_CrateA_Stump    what's left standing: the bottom row of slats and the posts' stubs, splintered on top, its floor
  Break_CrateB_1..7     Crate_B's (1 m plank crate) pieces: its walls' upper halves, the askew lid whole, a clump of straw
  Break_CrateB_Stump    the walls' lower halves, the floor, the straw bed sunk low in it
  Break_BarrelA_1..7    Barrel_A's (0.92 m barrel) pieces: five clusters of staves broken off above the second hoop, the top
                        hoop sprung oval, the head
  Break_BarrelA_Stump   the staves' feet in their two bottom hoops, splintered, the bottom head inside
Every piece and stump is modeled where it sat in the whole prop: its origin is the prop's pivot (the ground at the middle of
its footprint, front -Y), so the game throws each from exactly where it stood. Containers.py's sizes and materials (the
house trim sheet's weathered boards, its iron strip, the straw). No collision: they're thrown and lie a few seconds.

Containers (Source/AI_Looter_Shooter/Loot/Chest: AChest's kinds Coffin, Grave, Mailbox, Footlocker):
  LootCoffin            Graves.py's old pine toe-pincher (1.94 m along its front, 0.4 m to the top), open: six side boards,
                        a floor, old dirt and a rotted shroud inside, iron handles. SOCKET_Lid at the top's middle
  LootCoffin_Lid        its lid with its nails, origin at its middle underneath (SOCKET_Lid): pried and shoved back off
  LootGrave             an old grave dug open, its origin the mound's middle on the ground, its foot -Y (its headboard stands
                        1.1 m beyond its +Y end): spoil heaps either side and at the foot, loose dirt, the coffin in the
                        middle standing 12 cm proud of the ground (a hole can't go into the terrain), dirt in it.
                        SOCKET_Lid at the coffin's top. Starts under the ground under Graves.py's sunken mound, heaving up
  LootGrave_Lid         the grave's coffin lid (long along Y), origin at its middle underneath: shoved onto the heap
  LootGrave_Shovel      a grave-digger's spade, its origin the blade's tip, the T-grip up; stood by a grave that can be dug
  Mailbox               a tin box on a cedar post, its bottom 1.02 m up, its door at the front (-Y); a faded red flag
  Mailbox_Door          its door, origin on its hinge along its foot (SOCKET_Lid): dropped open forward and down
  Footlocker            a 0.8 m plank trunk, iron corner caps, rope handles at its ends, a hasp
  Footlocker_Lid        its lid, origin on the hinge along its back top edge (SOCKET_Lid): swung up 105 degrees
Sockets: SOCKET_Lid, SOCKET_Loot (where loot comes out), SOCKET_Interact (where the prompt points). Collision: the coffin's,
mailbox's and footlocker's bodies and the footlocker's lid have hulls; the grave, the spade, the coffin lids and the door
have none.

Small props: no Nanite, LODs at 50% and 25%. Materials are the shared ones (HouseTrim, Hay, GroundDirt, IronBlack,
PaintBlack, as Containers.py and Graves.py make them) and two of their own: MailboxTin (MetalWorn, galvanized grey) and
RopeHemp (Hay, tan). Run with --preview for pictures in Saved/ArtPreviews/Lootables.
"""
import math
import random

import bmesh
import bpy
from mathutils import Matrix, Vector, noise

import looter_buildings as kit
import looter_model as lm
import looter_props as lp
import looter_textures as lt

TRIM = lp.trim_material()
HAY = lt.material('Hay')
DIRT = lt.material('GroundDirt')
IRON = lt.material('MetalWorn', name='IronBlack', tint=0x2e2c2a)
PAINT_BLACK = lt.material('PaintWorn', name='PaintBlack', tint=0x2a2622)
TIN = lt.material('MetalWorn', name='MailboxTin', tint=0xc9c6bd)
ROPE = lt.material('Hay', name='RopeHemp', tint=0x9a7a50)
TILE = {HAY: 'Hay', DIRT: 'GroundDirt', IRON: 'MetalWorn', PAINT_BLACK: 'PaintWorn', TIN: 'MetalWorn', ROPE: 'Hay'}
PREVIEW = 'Lootables'


# --- Parts ---

def tile(obj, mat, seed=0, along=None):
    """A tileable material on a part, mapped at world scale."""
    lt.assign(obj, mat)
    lt.box_uv(obj, TILE[mat], seed=seed, along=along)
    return obj


def board(outline, thick):
    """A flat piece: outline (x, z) counterclockwise seen from the front, thick across Y, centred on y = 0."""
    obj = lp.mesh_object(kit._prism([outline], thick))
    obj.data.transform(Matrix.Translation((0.0, -thick * 0.5, 0.0)))
    return obj


def extrude(points, z0, z1):
    """A plan outline (x, y) extruded from z0 up to z1 (as Graves.py's)."""
    obj = lp.mesh_object(kit._prism([[(x, -y) for x, y in points]], z1 - z0))
    obj.data.transform(Matrix.Translation((0.0, 0.0, z0)) @ Matrix.Rotation(math.radians(90.0), 4, 'X'))
    return obj


def splintered(rnd, x0, x1, z0, z1, left=0.0, right=0.0, bottom=0.0, top=0.0, step=0.035):
    """A board's outline from (x0, z0) to (x1, z1), counterclockwise from the front, each edge given a depth broken off
    into splinters: alternate deep and shallow teeth pushed into the board along it."""
    def edge(p0, p1, inward, depth):
        count = max(int((p1 - p0).length / step), 2) if depth > 0.0 else 1
        points = []
        for k in range(count):
            p = p0.lerp(p1, k / count)
            if depth > 0.0 and k > 0:
                p = p + inward * depth * (rnd.uniform(0.55, 1.0) if k % 2 else rnd.uniform(0.0, 0.35))
            points.append((p.x, p.y))
        return points
    a, b, c, d = Vector((x0, z0)), Vector((x1, z0)), Vector((x1, z1)), Vector((x0, z1))
    return (edge(a, b, Vector((0.0, 1.0)), bottom) + edge(b, c, Vector((-1.0, 0.0)), right)
            + edge(c, d, Vector((0.0, -1.0)), top) + edge(d, a, Vector((1.0, 0.0)), left))


def slat(rnd, x0, x1, z0, z1, thick, y, along_y=False, seed=0, strip='Siding', **breaks):
    """A slat or wall board standing on edge: from x0 to x1 (along X, or along Y when along_y), z0 to z1, at y (its
    middle across its thickness; x for an end board), splintered where breaks say."""
    obj = board(splintered(rnd, x0, x1, z0, z1, **breaks), thick)
    if along_y:
        lp.place(obj, (y, 0.0, 0.0), (0.0, 0.0, 90.0))
        return lp.grain(obj, strip, axis=(0.0, 1.0, 0.0), seed=seed)
    lp.place(obj, (0.0, y, 0.0))
    return lp.grain(obj, strip, axis=(1.0, 0.0, 0.0), seed=seed)


def plank(rnd, x0, x1, y, width, z, thick, seed=0, **breaks):
    """A plank lying flat (a crate's top), from x0 to x1 along X, width across Y, its top at z."""
    obj = board(splintered(rnd, x0, x1, -width * 0.5, width * 0.5, **breaks), thick)
    lp.place(obj, (0.0, y, z - thick * 0.5), (90.0, 0.0, 0.0))
    return lp.grain(obj, 'Siding', axis=(1.0, 0.0, 0.0), seed=seed)


def post(rnd, size, x, y, z0, z1, broken_low=0.0, broken_high=0.0, seed=0):
    """A square post from z0 to z1, its low or high end broken off at a slant (the most it slants, m)."""
    obj = lp.block((size, size, z1 - z0), (x, y, (z0 + z1) * 0.5))
    mid = (z0 + z1) * 0.5
    for v in obj.data.vertices:
        if v.co.z < mid and broken_low > 0.0:
            v.co.z += rnd.uniform(0.0, broken_low)
        elif v.co.z > mid and broken_high > 0.0:
            v.co.z -= rnd.uniform(0.0, broken_high)
    return lp.grain(obj, 'Beams', axis=(0.0, 0.0, 1.0), seed=seed)


def finish_piece(name, parts, ao=0.25):
    """A piece or stump: merged, shaded, no Nanite, no collision (it's thrown, or a stump nothing stands on)."""
    obj = lp.join(name, parts)
    lp.finish(obj, ao=ao, nanite=False, smooth=40.0)
    obj['LODs'] = '50,25'
    obj['Collision'] = 'None'
    return obj


def finish_model(obj, ao=0.35, collision=None):
    lp.finish(obj, ao=ao, nanite=False, smooth=35.0)
    obj['LODs'] = '50,25'
    if collision:
        obj['Collision'] = collision
    return obj


def finish_lid(name, parts, origin, ao=0.3, collision=None):
    """A lid or door: merged, its origin moved to origin (its hinge, or its middle underneath) with the geometry left
    where it was built."""
    lid = lp.join(name, parts)
    lp.place(lid, (-origin[0], -origin[1], -origin[2]))
    lid.location = origin
    lm.smooth(lid, 35.0)
    lt.bake_vertex_ao(lid, distance=ao, ground=False)
    lid['Nanite'] = 0
    lid['LODs'] = '50,25'
    if collision:
        lid['Collision'] = collision
    return lid


# --- Crate_A: the slatted crate (Containers.py crate_a: 0.9 x 0.65 x 0.6 m) ---

CA_X, CA_Y, CA_Z, CA_POST, CA_T = 0.9, 0.65, 0.6, 0.07, 0.022
CA_SLATS = ((0.02, 0.18), (0.22, 0.38), (0.42, 0.58))


def crate_a_pieces(seed=30):
    rnd = random.Random(seed)
    front, back = -(CA_Y * 0.5 + CA_T * 0.5), CA_Y * 0.5 + CA_T * 0.5
    end = CA_X * 0.5 + CA_T * 0.5
    px, py = CA_X * 0.5 - CA_POST * 0.5, CA_Y * 0.5 - CA_POST * 0.5
    upper = CA_SLATS[1:]
    split = rnd.uniform(0.04, 0.16)
    pieces = []
    # 1, 2: the front's upper slats, broken through near the middle; the right half keeps the brace's top.
    parts = [slat(rnd, -CA_X * 0.5, split, z0, z1, CA_T, front, seed=rnd.randint(0, 999), right=0.07) for z0, z1 in upper]
    parts += [lp.nail((-CA_X * 0.5 + 0.035, front - CA_T * 0.5, (z0 + z1) * 0.5), (0.0, -1.0, 0.0)) for z0, z1 in upper]
    pieces.append(finish_piece('Break_CrateA_1', parts))
    parts = [slat(rnd, split, CA_X * 0.5, z0, z1, CA_T, front, seed=rnd.randint(0, 999), left=0.07) for z0, z1 in upper]
    brace = board(splintered(rnd, -0.26, 0.26, -0.05, 0.05, left=0.06), CA_T)
    lp.place(brace, (0.0, 0.0, 0.0), (0.0, 36.0, 0.0))
    lp.place(brace, (CA_X * 0.25, front - CA_T, 0.42))
    parts.append(lp.grain(brace, 'Siding', seed=rnd.randint(0, 999)))
    parts.append(post(rnd, CA_POST, px, -py, 0.24, CA_Z - 0.02, broken_low=0.05, seed=rnd.randint(0, 999)))
    pieces.append(finish_piece('Break_CrateA_2', parts))
    # 3: the back's upper slats whole, a splinter off each end, the back brace's middle.
    parts = [slat(rnd, -CA_X * 0.5, CA_X * 0.5, z0, z1, CA_T, back, seed=rnd.randint(0, 999), left=0.03, right=0.03)
             for z0, z1 in upper]
    brace = board(splintered(rnd, -0.3, 0.3, -0.05, 0.05, left=0.05, right=0.05), CA_T)
    lp.place(brace, (0.0, 0.0, 0.0), (0.0, -36.0, 0.0))
    lp.place(brace, (0.0, back + CA_T, 0.38))
    parts.append(lp.grain(brace, 'Siding', seed=rnd.randint(0, 999)))
    pieces.append(finish_piece('Break_CrateA_3', parts))
    # 4, 5: each end's upper slats with the two posts' tops behind them.
    for name, side in (('Break_CrateA_4', -1.0), ('Break_CrateA_5', 1.0)):
        half = CA_Y * 0.5 + CA_T
        parts = [slat(rnd, -half, half, z0, z1, CA_T, side * end, along_y=True, seed=rnd.randint(0, 999),
                      bottom=0.03 if z0 < 0.3 else 0.0) for z0, z1 in upper]
        parts.append(post(rnd, CA_POST, side * px, py, 0.22, CA_Z - 0.02, broken_low=0.06, seed=rnd.randint(0, 999)))
        if side < 0.0:
            parts.append(post(rnd, CA_POST, side * px, -py, 0.26, CA_Z - 0.02, broken_low=0.05, seed=rnd.randint(0, 999)))
        parts += [lp.nail((side * (end + CA_T * 0.5), y, (z0 + z1) * 0.5), (side, 0.0, 0.0))
                  for y in (-CA_Y * 0.5 + 0.03, CA_Y * 0.5 - 0.03) for z0, z1 in upper]
        pieces.append(finish_piece(name, parts))
    # 6, 7: the top's four planks, two to a piece, still nailed to each other at a cleat, splintered at the ends.
    for name, pair in (('Break_CrateA_6', (0, 1)), ('Break_CrateA_7', (2, 3))):
        parts = []
        for k in pair:
            y = -CA_Y * 0.5 + 0.02 + (CA_Y + 0.004) * (k + 0.5) / 4
            parts.append(plank(rnd, -CA_X * 0.5 - 0.02, CA_X * 0.5 + 0.02, y, 0.15, CA_Z + CA_T - 0.02, CA_T,
                               seed=rnd.randint(0, 999), left=rnd.choice((0.0, 0.05)), right=0.05))
            parts += [lp.nail((x, y, CA_Z + CA_T - 0.02), (0.0, 0.0, 1.0)) for x in (-CA_X * 0.5 + 0.035, CA_X * 0.5 - 0.035)]
        yc = -CA_Y * 0.5 + 0.02 + (CA_Y + 0.004) * (pair[0] + 1.0) / 4
        cleat = lp.block((0.06, 0.3, 0.02), (rnd.uniform(-0.1, 0.1), yc, CA_Z - 0.02))
        parts.append(lp.grain(cleat, 'Beams', axis=(0.0, 1.0, 0.0), seed=rnd.randint(0, 999)))
        pieces.append(finish_piece(name, parts))
    return pieces


def crate_a_stump(seed=31):
    rnd = random.Random(seed)
    front, back = -(CA_Y * 0.5 + CA_T * 0.5), CA_Y * 0.5 + CA_T * 0.5
    end = CA_X * 0.5 + CA_T * 0.5
    px, py = CA_X * 0.5 - CA_POST * 0.5, CA_Y * 0.5 - CA_POST * 0.5
    z0, z1 = CA_SLATS[0]
    parts = [slat(rnd, -CA_X * 0.5, CA_X * 0.5, z0, z1, CA_T, y, seed=rnd.randint(0, 999), top=0.05) for y in (front, back)]
    parts += [slat(rnd, -CA_Y * 0.5 - CA_T, CA_Y * 0.5 + CA_T, z0, z1, CA_T, x, along_y=True, seed=rnd.randint(0, 999), top=0.05)
              for x in (-end, end)]
    for x in (-px, px):
        for y in (-py, py):
            parts.append(post(rnd, CA_POST, x, y, 0.0, rnd.uniform(0.21, 0.3), broken_high=0.06, seed=rnd.randint(0, 999)))
    floor = lp.block((CA_X - 0.06, CA_Y - 0.06, 0.02), (0.0, 0.0, 0.03))
    parts.append(lp.grain(floor, 'Beams', axis=(1.0, 0.0, 0.0), seed=seed))
    for k in range(4):   # splinters lying in it
        s = lp.block((rnd.uniform(0.1, 0.22), 0.02, 0.01), (rnd.uniform(-0.3, 0.3), rnd.uniform(-0.2, 0.2), 0.045),
                     (0.0, 0.0, rnd.uniform(0.0, 180.0)))
        parts.append(lp.grain(s, 'Siding', axis=(1.0, 0.0, 0.0), seed=rnd.randint(0, 999)))
    return finish_piece('Break_CrateA_Stump', parts, ao=0.3)


# --- Crate_B: the plank packing crate (Containers.py crate_b: 1.0 x 0.62 x 0.55 m) ---

CB_X, CB_Y, CB_Z, CB_WALL, CB_BATTEN = 1.0, 0.62, 0.55, 0.025, 0.055


def wall_board(rnd, x0, x1, z0, z1, y, along_y=False, seed=0, **breaks):
    """A crate wall's piece: the siding strip draws its boards (as crate_b's walls), diced for the occlusion."""
    obj = board(splintered(rnd, x0, x1, z0, z1, step=0.05, **breaks), CB_WALL)
    if along_y:
        lp.place(obj, (y, 0.0, 0.0), (0.0, 0.0, 90.0))
    else:
        lp.place(obj, (0.0, y, 0.0))
    return lp.dice(lp.trim(obj, 'Siding', seed=seed), 0.2)


def batten(rnd, size, center, axis, seed):
    return lp.grain(lp.block(size, center), 'Beams', axis=axis, seed=seed)


def crate_b_pieces(seed=40):
    rnd = random.Random(seed)
    fy = (CB_Y - CB_WALL) * 0.5
    ex = (CB_X - CB_WALL) * 0.5
    bx, by = CB_X * 0.5 - CB_BATTEN * 0.5 + 0.012, CB_Y * 0.5 - CB_BATTEN * 0.5 + 0.012
    split = rnd.uniform(-0.05, 0.15)
    low = 0.27
    pieces = []
    # 1, 2: the front wall's upper half, broken in two; the top frame's batten on the left, a corner batten's top on the right.
    parts = [wall_board(rnd, -CB_X * 0.5, split, low, CB_Z, -fy, seed=rnd.randint(0, 999), right=0.08, bottom=0.05),
             batten(rnd, (CB_X * 0.5 + split - 0.06, 0.02, 0.06), ((split - CB_X * 0.5) * 0.5, -(CB_Y * 0.5 + 0.01), CB_Z - 0.03),
                    (1.0, 0.0, 0.0), rnd.randint(0, 999))]
    pieces.append(finish_piece('Break_CrateB_1', parts))
    parts = [wall_board(rnd, split, CB_X * 0.5, low, CB_Z, -fy, seed=rnd.randint(0, 999), left=0.08, bottom=0.05),
             post(rnd, CB_BATTEN, bx, -by, low - 0.02, CB_Z, broken_low=0.05, seed=rnd.randint(0, 999))]
    pieces.append(finish_piece('Break_CrateB_2', parts))
    # 3: the back wall's upper half and its top batten.
    parts = [wall_board(rnd, -CB_X * 0.5, CB_X * 0.5, low, CB_Z, fy, seed=rnd.randint(0, 999), bottom=0.06),
             batten(rnd, (CB_X - 2.0 * CB_BATTEN, 0.02, 0.06), (0.0, CB_Y * 0.5 + 0.01, CB_Z - 0.03), (1.0, 0.0, 0.0), rnd.randint(0, 999))]
    pieces.append(finish_piece('Break_CrateB_3', parts))
    # 4, 5: the ends' upper halves with their corner battens' tops.
    for name, side in (('Break_CrateB_4', -1.0), ('Break_CrateB_5', 1.0)):
        half = CB_Y * 0.5 - CB_WALL
        parts = [wall_board(rnd, -half, half, low, CB_Z, side * ex, along_y=True, seed=rnd.randint(0, 999), bottom=0.06),
                 post(rnd, CB_BATTEN, side * bx, by, low, CB_Z, broken_low=0.06, seed=rnd.randint(0, 999))]
        if side < 0.0:
            parts.append(post(rnd, CB_BATTEN, side * bx, -by, low + 0.02, CB_Z, broken_low=0.05, seed=rnd.randint(0, 999)))
        pieces.append(finish_piece(name, parts))
    # 6: the lid, whole, as it lay askew on the crate (crate_b's).
    lid = [lp.dice(lp.trim(lp.block((CB_X + 0.04, CB_Y + 0.04, 0.025), (0.0, 0.0, 0.0125)), 'Siding', seed=seed + 5), 0.25)]
    for x in (-0.32, 0.32):
        lid.append(batten(rnd, (0.08, CB_Y - 0.02, 0.03), (x, 0.0, 0.04), (0.0, 1.0, 0.0), rnd.randint(0, 999)))
        lid += [lp.nail((x, y, 0.055), (0.0, 0.0, 1.0)) for y in (-0.2, 0.0, 0.2)]
    lid_obj = lp.join('_lid', lid)
    lp.place(lid_obj, (0.0, 0.0, 0.0), (0.0, -3.5, 14.0))
    lp.place(lid_obj, (0.1, 0.24, CB_Z + 0.004))
    pieces.append(finish_piece('Break_CrateB_6', [lid_obj]))
    # 7: a clump of the packing straw.
    clump = lp.lathe([(0.0, 0.0), (0.15, 0.015), (0.19, 0.07), (0.13, 0.15), (0.0, 0.17)], segments=10)[0]
    lp.rough(clump, 0.03, 9.0, seed)
    lp.place(clump, (-0.12, -0.02, CB_Z - 0.17))
    pieces.append(finish_piece('Break_CrateB_7', [tile(clump, HAY, seed)]))
    return pieces


def crate_b_stump(seed=41):
    rnd = random.Random(seed)
    fy = (CB_Y - CB_WALL) * 0.5
    ex = (CB_X - CB_WALL) * 0.5
    bx, by = CB_X * 0.5 - CB_BATTEN * 0.5 + 0.012, CB_Y * 0.5 - CB_BATTEN * 0.5 + 0.012
    top = 0.29
    parts = [wall_board(rnd, -CB_X * 0.5, CB_X * 0.5, 0.0, top, y, seed=rnd.randint(0, 999), top=0.07) for y in (-fy, fy)]
    half = CB_Y * 0.5 - CB_WALL
    parts += [wall_board(rnd, -half, half, 0.0, top, x, along_y=True, seed=rnd.randint(0, 999), top=0.07) for x in (-ex, ex)]
    for x in (-bx, bx):
        for y in (-by, by):
            parts.append(post(rnd, CB_BATTEN, x, y, 0.0, rnd.uniform(0.22, 0.31), broken_high=0.05, seed=rnd.randint(0, 999)))
    for side in (-1.0, 1.0):
        parts.append(batten(rnd, (CB_X - 2.0 * CB_BATTEN, 0.02, 0.06), (0.0, side * (CB_Y * 0.5 + 0.01), 0.03), (1.0, 0.0, 0.0),
                            rnd.randint(0, 999)))
    floor = lp.block((CB_X - 2.0 * CB_WALL, CB_Y - 2.0 * CB_WALL, 0.03), (0.0, 0.0, 0.05))
    parts.append(lp.dice(lp.trim(floor, 'Siding', seed=seed), 0.25))
    # The straw bed, lower in the box than it lay.
    bm = lp.new_bmesh()
    bmesh.ops.create_grid(bm, x_segments=10, y_segments=6, size=0.5,
                          matrix=Matrix.Diagonal(Vector((CB_X - 0.06, CB_Y - 0.06, 1.0, 1.0))))
    for v in bm.verts:
        v.co.z = 0.15 + 0.04 * noise.noise(Vector((v.co.x * 6.0, v.co.y * 6.0, seed)))
    straw = lp.mesh_object(bm)
    parts.append(tile(straw, HAY, seed))
    return finish_piece('Break_CrateB_Stump', parts, ao=0.3)


# --- Barrel_A: the wooden barrel (Containers.py barrel_a: 0.92 m, staves on four iron hoops) ---

BA_H, BA_END, BA_MID, BA_STAVES, BA_T = 0.92, 0.27, 0.33, 16, 0.022
BA_HOOPS = (0.07, 0.27, 0.65, 0.85)
BA_PROUD = 0.009


def barrel_radius(z):
    return BA_END + (BA_MID - BA_END) * math.sin(math.pi * min(max(z / BA_H, 0.0), 1.0)) ** 0.8


def stave(rnd, k, z_low, z_high, low_ragged=0.0, high_ragged=0.0, rows=7):
    """The k-th of the barrel's sixteen staves from z_low to z_high, a curved shell its thickness deep, its broken ends
    splintered (a point left at its middle or its sides)."""
    da = 2.0 * math.pi / BA_STAVES
    a0, a1 = k * da + 0.004, (k + 1) * da - 0.004
    cols = 3
    bm = lp.new_bmesh()
    grid = []
    for i in range(cols + 1):
        a = a0 + (a1 - a0) * i / cols
        lo = z_low + (rnd.uniform(0.0, low_ragged) * (1.0 if i % 2 else 0.4) if low_ragged > 0.0 else 0.0)
        hi = z_high - (rnd.uniform(0.0, high_ragged) * (1.0 if i % 2 else 0.4) if high_ragged > 0.0 else 0.0)
        column = []
        for j in range(rows + 1):
            z = lo + (hi - lo) * j / rows
            r = barrel_radius(z)
            outer = bm.verts.new((math.cos(a) * r, math.sin(a) * r, z))
            inner = bm.verts.new((math.cos(a) * (r - BA_T), math.sin(a) * (r - BA_T), z))
            column.append((outer, inner))
        grid.append(column)
    for i in range(cols):
        for j in range(rows):
            (o00, i00), (o10, i10) = grid[i][j], grid[i + 1][j]
            (o01, i01), (o11, i11) = grid[i][j + 1], grid[i + 1][j + 1]
            bm.faces.new((o00, o10, o11, o01))
            bm.faces.new((i01, i11, i10, i00))
    for j in range(rows):
        for i, flip in ((0, True), (cols, False)):
            (o0, i0), (o1, i1) = grid[i][j], grid[i][j + 1]
            bm.faces.new((o0, o1, i1, i0) if flip else (i0, i1, o1, o0))
    for i in range(cols):
        for j, flip in ((0, False), (rows, True)):
            (o0, i0), (o1, i1) = grid[i][j], grid[i + 1][j]
            bm.faces.new((o0, i0, i1, o1) if flip else (o1, i1, i0, o0))
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces[:])
    obj = lp.mesh_object(bm)
    lt.assign(obj, TRIM)
    lp.lathe_uv(obj, 'Siding', grain='up', seed=rnd.randint(0, 999), per_column=False)
    return obj


def hoop(z_center, seed, squash=(1.0, 1.0)):
    """An iron hoop round the staves at z_center (its band 5.6 cm), squashed oval when it's sprung off."""
    z0, z1 = z_center - 0.028, z_center + 0.028
    r = barrel_radius(z_center)
    ring = lp.lathe([(r + BA_PROUD, z0), (r + BA_PROUD, z1), (r - 0.004, z1), (r - 0.004, z0)], segments=16, closed=True)[0]
    lt.assign(ring, TRIM)
    lp.lathe_uv(ring, 'Iron', grain='around', seed=seed)
    ring.data.transform(Matrix.Diagonal(Vector((squash[0], squash[1], 1.0, 1.0))))
    return ring


def barrel_head(z, seed):
    """A head of boards across the barrel at z (the top's, or the bottom's inside)."""
    r = barrel_radius(z) - 0.025
    disc = lp.lathe([(0.0, z - 0.03), (r, z - 0.03), (r, z), (0.0, z)], segments=16)[0]
    lt.trim_uv(disc, None, 'Siding', align='world', v_offset=0.3)
    lt.assign(disc, TRIM)
    return disc


def barrel_a_pieces(seed=10):
    rnd = random.Random(seed)
    pieces = []
    groups = ((0, 1, 2), (3, 4, 5), (6, 7, 8), (9, 10, 11), (12, 13, 14, 15))
    for n, group in enumerate(groups):
        parts = [stave(rnd, k, 0.31, BA_H, low_ragged=0.1, high_ragged=0.0) for k in group]
        pieces.append(finish_piece(f'Break_BarrelA_{n + 1}', parts))
    pieces.append(finish_piece('Break_BarrelA_6', [hoop(BA_HOOPS[3], seed + 6, (1.07, 0.93))]))
    head = barrel_head(BA_H, seed + 7)
    pieces.append(finish_piece('Break_BarrelA_7', [head]))
    return pieces


def barrel_a_stump(seed=11):
    rnd = random.Random(seed)
    parts = [stave(rnd, k, 0.0, rnd.uniform(0.36, 0.44), high_ragged=0.08, rows=4) for k in range(BA_STAVES)]
    parts += [hoop(BA_HOOPS[0], seed + 1), hoop(BA_HOOPS[1], seed + 2)]
    parts.append(barrel_head(0.06, seed + 3))
    return finish_piece('Break_BarrelA_Stump', parts, ao=0.3)


# --- The coffin (Graves.py's toe-pincher, its lid loose) ---

COFFIN = [(-0.97, -0.17), (-0.35, -0.31), (0.97, -0.21), (0.97, 0.21), (-0.35, 0.31), (-0.97, 0.17)]   # head at -x
COFFIN_H = 0.4
LID_T = 0.035


def coffin_box(seed, z0=0.0, height=COFFIN_H, outline=COFFIN, shroud=True, dirt_top=None):
    """An open coffin: six side boards on its outline from z0, a floor, old dirt in it up to dirt_top (8 cm over its
    floor unless given: a grave's must stand above the ground, which would cover anything lower) and a rotted shroud."""
    fill = z0 + 0.08 if dirt_top is None else dirt_top
    rnd = random.Random(seed)
    parts = []
    n = len(outline)
    for k in range(n):
        a, b = Vector(outline[k] + (0.0,)), Vector(outline[(k + 1) % n] + (0.0,))
        d = b - a
        mid = (a + b) * 0.5
        normal = Vector((d.y, -d.x, 0.0)).normalized()
        side = lp.block((d.length + 0.02, 0.03, height), (0.0, 0.0, height * 0.5))
        lp.grain(side, 'Siding', axis=(1.0, 0.0, 0.0), seed=seed + k)
        lp.place(side, mid - normal * 0.015 + Vector((0.0, 0.0, z0)), (0.0, 0.0, math.degrees(math.atan2(d.y, d.x))))
        parts.append(side)
    floor = extrude([(x * 0.97, y * 0.94) for x, y in outline], z0, z0 + 0.03)
    parts.append(lp.grain(floor, 'Siding', axis=(1.0, 0.0, 0.0), seed=seed + 9))
    dirt = extrude([(x * 0.85, y * 0.8) for x, y in outline], z0 + 0.03, fill)
    lp.rough(dirt, 0.02, 6.0, seed)
    parts.append(tile(dirt, DIRT, seed))
    if shroud:
        # A rotted shroud: a dark rag lying along it.
        rag = extrude([(x * 0.42 + 0.08 * (1.0 if abs(x) > abs(y) else 0.0), y * 0.34) for x, y in outline], fill, fill + 0.015)
        lp.rough(rag, 0.012, 11.0, seed + 1)
        parts.append(tile(rag, PAINT_BLACK, seed + 1))
    return parts


def coffin_handles(seed, z, outline_y):
    parts = []
    for x in (-0.55, 0.35):
        for sy in (-1.0, 1.0):
            handle = lp.mesh_object(kit._box((0.18, 0.024, 0.035), drop=('+y',)))
            handle.data.transform(Matrix.Rotation(math.radians(180.0 if sy > 0 else 0.0), 4, 'Z'))
            handle.data.transform(Matrix.Translation((x, sy * (outline_y(x) + 0.012), z)))
            parts.append(tile(handle, IRON, seed + 5))
    return parts


def coffin_side_y(x):
    return 0.17 + (x + 0.97) / 0.62 * 0.14 if x < -0.35 else 0.31 - (x + 0.35) / 1.32 * 0.1


def coffin_lid_parts(seed, outline, z):
    """The lid: boards on the coffin's outline a little proud of it, its nails round the edge."""
    rnd = random.Random(seed)
    top = extrude([(x * 1.03, y * 1.06) for x, y in outline], z, z + LID_T)
    parts = [lp.grain(top, 'Siding', axis=(1.0, 0.0, 0.0) if abs(outline[0][0]) > abs(outline[0][1]) else (0.0, 1.0, 0.0),
                      seed=seed + 3)]
    for k, (x, y) in enumerate(outline):
        parts.append(lp.nail((x * 0.95, y * 0.88, z + LID_T), (0.0, 0.0, 1.0), size=0.02))
    return parts


def loot_coffin(seed=612):
    body_parts = coffin_box(seed) + coffin_handles(seed, COFFIN_H * 0.56, coffin_side_y)
    body = lp.join('LootCoffin', body_parts)
    lp.hull_points(body, [(x * 1.03, y * 1.06, z) for x, y in COFFIN for z in (0.0, COFFIN_H)])
    lm.socket(body, 'Lid', (0.0, 0.0, COFFIN_H))
    lm.socket(body, 'Loot', (0.0, 0.0, 0.14))
    lm.socket(body, 'Interact', (0.0, -0.36, 0.32))
    finish_model(body, ao=0.3)
    lid = finish_lid('LootCoffin_Lid', coffin_lid_parts(seed, COFFIN, COFFIN_H), (0.0, 0.0, COFFIN_H), collision='None')
    return [body, lid]


# --- The grave, dug open ---

# The coffin turned to lie along the grave (its head toward +Y, the headboard's end): Graves.py's (x, y) -> (-y, -x).
GRAVE_COFFIN = [(-y, -x) for x, y in COFFIN]
GRAVE_RIM = 0.12      # how far its coffin stands out of the ground
GRAVE_SUNK = 0.12     # how far down its walls go


def heap(length, width, height, center, seed, nx=10, ny=7):
    """A heap of dug dirt (x across, y along), rounded and lumpy, its skirt 5 cm under the ground."""
    rnd = random.Random(seed)
    off = Vector((rnd.uniform(0, 50), rnd.uniform(0, 50), rnd.uniform(0, 50)))
    bm = lp.new_bmesh()
    grid = []
    for i in range(nx + 1):
        row = []
        for j in range(ny + 1):
            u, v = i / nx * 2.0 - 1.0, j / ny * 2.0 - 1.0
            edge = max(abs(u), abs(v)) >= 0.999
            x, y = u * width * 0.5, v * length * 0.5
            shape = max(0.0, 1.0 - u * u) ** 0.8 * max(0.0, 1.0 - v * v) ** 0.5
            z = -0.05 if edge else height * shape + 0.03 * noise.noise(Vector((x * 5.0, y * 5.0, 0.0)) + off)
            if not edge:
                x += 0.3 * width / nx * noise.noise(Vector((x * 6.0, y * 6.0, 1.0)) + off)
                y += 0.3 * length / ny * noise.noise(Vector((x * 6.0, y * 6.0, 7.0)) + off)
            row.append(bm.verts.new((x + center[0], y + center[1], z)))
        grid.append(row)
    for i in range(nx):
        for j in range(ny):
            bm.faces.new((grid[i][j], grid[i + 1][j], grid[i + 1][j + 1], grid[i][j + 1]))
    bmesh.ops.triangulate(bm, faces=bm.faces[:], quad_method='ALTERNATE')
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces[:])
    for f in bm.faces:
        if f.normal.z < 0.0:
            f.normal_flip()
    return tile(lp.mesh_object(bm), DIRT, seed)


def clods(rnd, count, area, center, z_fn):
    parts = []
    for k in range(count):
        x, y = center[0] + rnd.uniform(-area[0], area[0]), center[1] + rnd.uniform(-area[1], area[1])
        s = rnd.uniform(0.05, 0.11)
        c = lp.block((s, s * rnd.uniform(0.7, 1.2), s * 0.6), (x, y, z_fn(x, y) + s * 0.15),
                     (rnd.uniform(-20, 20), rnd.uniform(-20, 20), rnd.uniform(0, 90)))
        lp.rough(c, s * 0.15, 9.0, rnd.randint(0, 999))
        parts.append(tile(c, DIRT, rnd.randint(0, 999)))
    return parts


def loot_grave(seed=640):
    rnd = random.Random(seed)
    parts = coffin_box(seed, z0=-GRAVE_SUNK, height=GRAVE_SUNK + GRAVE_RIM, outline=GRAVE_COFFIN, dirt_top=0.05)
    # The handles' bands along its long sides (now along Y), a little below the rim.
    for x in (-0.55, 0.35):
        for sx in (-1.0, 1.0):
            handle = lp.block((0.024, 0.18, 0.035), (sx * (coffin_side_y(x) + 0.012), -x, GRAVE_RIM - 0.05))
            parts.append(tile(handle, IRON, seed + 5))
    # Loose dirt round it, the spoil heaps either side and at the foot, clods thrown about.
    apron = heap(2.3, 1.2, 0.05, (0.0, 0.0), seed + 1)
    parts.append(apron)
    parts.append(heap(2.0, 0.5, 0.32, (0.64, 0.05), seed + 2))
    parts.append(heap(1.9, 0.46, 0.28, (-0.63, -0.05), seed + 3))
    parts.append(heap(0.45, 0.8, 0.2, (0.0, -1.22), seed + 4))
    parts += clods(rnd, 9, (0.7, 1.1), (0.0, 0.0), lambda x, y: 0.12 if abs(x) > 0.45 else 0.02)
    body = lp.join('LootGrave', parts)
    lm.socket(body, 'Lid', (0.0, 0.0, GRAVE_RIM))
    lm.socket(body, 'Loot', (0.0, 0.0, 0.1))
    lm.socket(body, 'Interact', (0.0, 0.0, 0.3))
    finish_model(body, ao=0.35, collision='None')
    lid = finish_lid('LootGrave_Lid', coffin_lid_parts(seed + 7, GRAVE_COFFIN, GRAVE_RIM), (0.0, 0.0, GRAVE_RIM),
                     collision='None')
    return [body, lid]


def shovel(seed=650):
    """A grave-digger's spade: a dished iron blade with a pointed tip (the origin), its socket, an ash shaft, a T-grip."""
    rnd = random.Random(seed)
    blade_outline = [(-0.105, 0.05), (-0.03, 0.0), (0.03, 0.0), (0.105, 0.05), (0.11, 0.27), (-0.11, 0.27)]
    blade = board(blade_outline, 0.006)
    for v in blade.data.vertices:      # dished: its sides curve forward
        v.co.y -= 0.02 * (v.co.x / 0.11) ** 2
    lp.grain(blade, 'Iron', axis=(0.0, 0.0, 1.0), seed=seed)
    collar = lp.lathe([(0.0, 0.26), (0.03, 0.26), (0.022, 0.36), (0.0, 0.36)], segments=10)[0]
    lt.assign(collar, TRIM)
    lp.lathe_uv(collar, 'Iron', grain='around', seed=seed + 1)
    shaft = lp.lathe([(0.0, 0.33), (0.019, 0.33), (0.017, 1.08), (0.0, 1.08)], segments=10)[0]
    lt.assign(shaft, TRIM)
    lp.lathe_uv(shaft, 'Beams', grain='up', seed=seed + 2)
    grip = lp.block((0.14, 0.034, 0.034), (0.0, 0.0, 1.095), bevel=0.006)
    lp.grain(grip, 'Beams', axis=(1.0, 0.0, 0.0), seed=seed + 3)
    obj = lp.join('LootGrave_Shovel', [blade, collar, shaft, grip])
    return finish_model(obj, ao=0.15, collision='None')


# --- The mailbox ---

MB_LENGTH, MB_WIDTH, MB_WALL, MB_ARCH = 0.46, 0.18, 0.12, 0.09
MB_FRONT, MB_BOTTOM = -0.28, 1.02


def arch_outline(width, wall, arch, steps=8):
    """A mailbox's end: straight sides wall high under a half round top, (x, z) counterclockwise from the front."""
    half = width * 0.5
    points = [(-half, 0.0), (half, 0.0), (half, wall)]
    points += [(half * math.cos(math.pi * k / steps), wall + arch * math.sin(math.pi * k / steps)) for k in range(1, steps)]
    return points + [(-half, wall)]


def mailbox(seed=660):
    rnd = random.Random(seed)
    parts = []
    # The post, 15 cm in the ground, up to the shelf the box sits on, and a brace under the shelf's front.
    top = MB_BOTTOM - 0.03
    stake = lp.block((0.09, 0.09, top + 0.15), (0.0, 0.05, (top - 0.15) * 0.5), bevel=0.008)
    parts.append(lp.grain(stake, 'Beams', axis=(0.0, 0.0, 1.0), seed=seed))
    shelf = lp.block((0.16, 0.38, 0.03), (0.0, -0.04, MB_BOTTOM - 0.015))
    parts.append(lp.grain(shelf, 'Beams', axis=(0.0, 1.0, 0.0), seed=seed + 1))
    brace = lp.block((0.04, 0.3, 0.04), (0.0, -0.07, MB_BOTTOM - 0.12), (-40.0, 0.0, 0.0))
    parts.append(lp.grain(brace, 'Beams', axis=(0.0, 1.0, 0.0), seed=seed + 2))
    # The box: its end's arch run back from the front, galvanized tin; its front dark inside, the door shut over it.
    box = lp.mesh_object(kit._prism([arch_outline(MB_WIDTH, MB_WALL, MB_ARCH)], MB_LENGTH))
    lp.place(box, (0.0, MB_FRONT, MB_BOTTOM))
    tile(box, TIN, seed + 3)
    front = [p.index for p in box.data.polygons if p.normal.y < -0.9]
    lt.assign(box, PAINT_BLACK, front)
    parts.append(box)
    # A faded red flag on its left side, down.
    flag = lp.block((0.008, 0.16, 0.05), (MB_WIDTH * 0.5 + 0.008, -0.02, MB_BOTTOM + 0.09))
    parts.append(lp.grain(flag, 'TrimRed', axis=(0.0, 1.0, 0.0), seed=seed + 4))
    pivot = lp.block((0.012, 0.02, 0.02), (MB_WIDTH * 0.5 + 0.004, 0.05, MB_BOTTOM + 0.09))
    parts.append(tile(pivot, IRON, seed + 5))
    body = lp.join('Mailbox', parts)
    lp.hull_box(body, (0.1, 0.1, MB_BOTTOM), (0.0, 0.05, MB_BOTTOM * 0.5))
    lp.hull_box(body, (MB_WIDTH + 0.02, MB_LENGTH + 0.02, MB_WALL + MB_ARCH), (0.0, MB_FRONT + MB_LENGTH * 0.5,
                                                                             MB_BOTTOM + (MB_WALL + MB_ARCH) * 0.5))
    lm.socket(body, 'Lid', (0.0, MB_FRONT, MB_BOTTOM))
    lm.socket(body, 'Loot', (0.0, MB_FRONT + 0.2, MB_BOTTOM + 0.07))
    lm.socket(body, 'Interact', (0.0, MB_FRONT - 0.02, MB_BOTTOM + 0.1))
    finish_model(body, ao=0.25)
    # The door: the arch a little bigger, in front of the box's mouth, a latch knob at its top.
    door_shape = arch_outline(MB_WIDTH + 0.012, MB_WALL, MB_ARCH + 0.006)
    door = lp.mesh_object(kit._prism([door_shape], 0.012))
    lp.place(door, (0.0, MB_FRONT - 0.012, MB_BOTTOM - 0.003))
    tile(door, TIN, seed + 6)
    knob = lp.block((0.03, 0.02, 0.02), (0.0, MB_FRONT - 0.022, MB_BOTTOM + MB_WALL + MB_ARCH - 0.02))
    door_obj = finish_lid('Mailbox_Door', [door, tile(knob, IRON, seed + 7)], (0.0, MB_FRONT, MB_BOTTOM), ao=0.1,
                          collision='None')
    return [body, door_obj]


# --- The footlocker ---

FL_X, FL_Y, FL_Z, FL_WALL = 0.8, 0.46, 0.4, 0.022


def footlocker(seed=670):
    rnd = random.Random(seed)
    parts = []
    for side in (-1.0, 1.0):
        w = lp.block((FL_X, FL_WALL, FL_Z), (0.0, side * (FL_Y - FL_WALL) * 0.5, FL_Z * 0.5))
        parts.append(lp.dice(lp.trim(w, 'Siding', seed=rnd.randint(0, 999)), 0.2))
        e = lp.block((FL_WALL, FL_Y - 2.0 * FL_WALL, FL_Z), (side * (FL_X - FL_WALL) * 0.5, 0.0, FL_Z * 0.5))
        parts.append(lp.dice(lp.trim(e, 'Siding', seed=rnd.randint(0, 999)), 0.2))
    floor = lp.block((FL_X - 2.0 * FL_WALL, FL_Y - 2.0 * FL_WALL, 0.02), (0.0, 0.0, 0.05))
    parts.append(lp.dice(lp.trim(floor, 'Siding', seed=seed), 0.25))
    # Iron caps on its eight corners, bands round its middle, a hasp on its front.
    for x in (-1.0, 1.0):
        for y in (-1.0, 1.0):
            for z in (0.0, 1.0):
                cap = lp.block((0.07, 0.07, 0.07), (x * (FL_X * 0.5 - 0.03), y * (FL_Y * 0.5 - 0.03), 0.03 + z * (FL_Z - 0.06)))
                parts.append(tile(cap, IRON, rnd.randint(0, 999)))
    for x in (-0.22, 0.22):
        for side in (-1.0, 1.0):
            band = lp.block((0.05, 0.008, FL_Z - 0.1), (x, side * (FL_Y * 0.5 + 0.004), FL_Z * 0.5))
            parts.append(tile(band, IRON, rnd.randint(0, 999)))
    hasp = lp.block((0.06, 0.012, 0.09), (0.0, -(FL_Y * 0.5 + 0.006), FL_Z - 0.05))
    parts.append(tile(hasp, IRON, seed + 1))
    # Rope handles at its ends, hanging in loops from iron eyes.
    for side in (-1.0, 1.0):
        x = side * (FL_X * 0.5 + 0.02)
        loop = [(x, -0.12, 0.3), (x + side * 0.03, -0.08, 0.21), (x + side * 0.035, 0.0, 0.19),
                (x + side * 0.03, 0.08, 0.21), (x, 0.12, 0.3)]
        rope = lp.sweep(loop, lp.ngon(0.012, 6))
        parts.append(tile(rope, ROPE, rnd.randint(0, 999), along='long'))
    body = lp.join('Footlocker', parts)
    lp.hull_box(body, (FL_X + 0.06, FL_Y + 0.02, FL_Z), (0.0, 0.0, FL_Z * 0.5))
    hinge = (0.0, FL_Y * 0.5 + 0.01, FL_Z)
    lm.socket(body, 'Lid', hinge)
    lm.socket(body, 'Loot', (0.0, 0.0, 0.14))
    lm.socket(body, 'Interact', (0.0, -(FL_Y * 0.5 + 0.03), FL_Z - 0.08))
    finish_model(body, ao=0.3)
    # The lid: planks under an iron edge, its bands carried over, the hasp's staple.
    lid = [lp.dice(lp.trim(lp.block((FL_X + 0.02, FL_Y + 0.02, 0.05), (0.0, 0.0, FL_Z + 0.025)), 'Siding', seed=seed + 2), 0.25)]
    for x in (-0.22, 0.22):
        lid.append(tile(lp.block((0.05, FL_Y + 0.03, 0.056), (x, 0.0, FL_Z + 0.026)), IRON, rnd.randint(0, 999)))
    for side in (-1.0, 1.0):
        lid.append(tile(lp.block((FL_X + 0.03, 0.02, 0.03), (0.0, side * (FL_Y * 0.5 + 0.005), FL_Z + 0.012)), IRON,
                        rnd.randint(0, 999)))
    lid.append(tile(lp.block((0.04, 0.014, 0.05), (0.0, -(FL_Y * 0.5 + 0.012), FL_Z - 0.005)), IRON, seed + 3))
    lid_obj = finish_lid('Footlocker_Lid', lid, hinge)
    lp.hull_box(lid_obj, (FL_X + 0.02, FL_Y + 0.02, 0.05), (0.0, -(FL_Y * 0.5 + 0.01), 0.025))
    lid_obj['OpenAngle'] = 105.0
    return [body, lid_obj]


# --- Build ---

models = []
models += crate_a_pieces() + [crate_a_stump()]
models += crate_b_pieces() + [crate_b_stump()]
models += barrel_a_pieces() + [barrel_a_stump()]
models += loot_coffin()
models += loot_grave()
models.append(shovel())
models += mailbox()
models += footlocker()


def previews():
    """Each breakable's pieces drawn a little apart round its stump; the containers shut, beside their opened pose."""
    shown = []

    def copy(obj, at=(0.0, 0.0, 0.0), turn=(0.0, 0.0, 0.0)):
        # Removed again after the picture (a name starting with _ would be skipped by the preview too).
        dup = bpy.data.objects.new('Preview_' + obj.name, obj.data)
        bpy.context.scene.collection.objects.link(dup)
        dup.location = at
        dup.rotation_euler = [math.radians(a) for a in turn]
        shown.append(dup)
        return dup

    by_name = {o.name: o for o in models}
    for prefix, x0 in (('Break_CrateA', -2.2), ('Break_CrateB', 0.0), ('Break_BarrelA', 2.2)):
        copy(by_name[prefix + '_Stump'], (x0, 0.0, 0.0))
        for name, obj in by_name.items():
            if name.startswith(prefix + '_') and not name.endswith('Stump'):
                middle = sum((Vector(c) for c in obj.bound_box), Vector()) / 8.0
                out = Vector((middle.x, middle.y, 0.0))
                copy(obj, (x0 + out.x * 0.6, out.y * 0.6, 0.25 + middle.z * 0.3))
    bpy.context.view_layer.update()
    lt.preview(shown, lt.preview_path(PREVIEW, 'Breakables'), view=(-0.2, -1.6, 0.8), fit=0.9)
    for dup in shown:
        bpy.data.objects.remove(dup)
    shown.clear()
    # The containers: coffin with its lid shoved off, the grave with the spade, the mailbox open, the footlocker open.
    copy(by_name['LootCoffin'], (-2.4, 0.0, 0.0))
    copy(by_name['LootCoffin_Lid'], (-2.4, 0.42, COFFIN_H - 0.08), (0.0, 28.0, 6.0))
    copy(by_name['LootGrave'], (0.0, 0.3, 0.0))
    copy(by_name['LootGrave_Lid'], (-0.62, 0.38, GRAVE_RIM + 0.14), (0.0, -16.0, 8.0))
    copy(by_name['LootGrave_Shovel'], (0.66, 0.3 - 0.2, 0.14), (12.0, -10.0, -25.0))
    copy(by_name['Mailbox'], (1.8, 0.0, 0.0))
    copy(by_name['Mailbox_Door'], (1.8, MB_FRONT, MB_BOTTOM), (100.0, 0.0, 0.0))
    copy(by_name['Footlocker'], (3.0, 0.0, 0.0))
    copy(by_name['Footlocker_Lid'], (3.0, FL_Y * 0.5 + 0.01, FL_Z), (-105.0, 0.0, 0.0))
    bpy.context.view_layer.update()
    lt.preview(shown, lt.preview_path(PREVIEW, 'Containers'), view=(-0.3, -1.6, 0.9), fit=0.9)
    for dup in shown:
        bpy.data.objects.remove(dup)


if lt.want_preview():
    previews()
for obj in models:
    print(f'LOOTABLES: {obj.name}: {lp.tri_count(obj)} triangles, '
          f'{len([c for c in obj.children if c.name.startswith("UCX_")])} hulls, '
          f'materials {", ".join(m.name for m in obj.data.materials)}', flush=True)
