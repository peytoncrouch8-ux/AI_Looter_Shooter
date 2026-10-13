"""The Providence Diggings (Docs/Areas/RansomsRest_Expansion.md, section 4 "Back Pay", section 6 "The Providence"): the
mine's timbered portal and tag board, the ore bin, the stone powder house and the smithy. Scripted models (Art/README.md)
on looter_mountain (looter_homes, looter_town, looter_buildings). The company shut the mine four years ago after the
fall: the forge is cold, the powder house locked, the portal's doors hang open.

  RR_MinePortal          the adit's mouth set into Larkspur High's cliff band: a fully lagged entry of timber sets from
                         y -3.4 to 0 (the rock pieces take over at y -1: Art/Models/Mine/Mine.py), a heavy front set with
                         PROVIDENCE MINING CO. on its header, log cribbing wings holding the cut slope back either side
                         (x to +-4.6), a brow of logs over the cap, the two plank doors hanging open, plank walks either
                         side of the track's way; and in front of the west wing the tag board, IN and OUT, its hooks, the
                         shift's tags that came up on OUT. Pivot: the mine's origin (Mine.layout.json: the adit's mouth on
                         its floor), its front (-Y) south: place it with the same transform as the mine's pieces. The
                         clear opening is 3.6 m wide (between the posts) and 2.95 m high (under the caps).
                         SOCKET_TagBoard (the board's face), SOCKET_Mouth (the floor at the mouth), SOCKET_Light.
  RR_TagBoard_TagsIn     the eleven's brass tags on the IN side's hooks (Back Pay's artifact); RR_TagBoard_TagsOut the same
                         eleven on OUT's empty hooks (the "pay" ending). Both pivot where the portal does: same transform.
  RR_OreBin              a timber ore bin on posts with a top deck at 3.6 m (a grizzly of rails over its mouth), a 6 m
                         trestle north at the deck's height for the track from the portal, the chute and its gate south
                         over the lower track, ore spilt under it. Pivot: the middle of the bin's foot on the lower ground;
                         the trestle's north end wants ground about 3.6 m higher (SOCKET_TrackTop), the lower tramway
                         leaves under the chute (SOCKET_TrackBottom).
  RR_PowderHouse         a 3 x 4 m house of dry fieldstone under a low board-and-shake roof, its iron-strapped door
                         padlocked, POWDER on the board over it. Pivot: the middle of its footprint, the door south.
  RR_Smithy              an open-fronted smithy shed (5 x 4 m): a stone forge and hood with its stack through the shake
                         roof, the anvil on its stump, the bellows, a quench barrel, a grindstone, tongs and hammers on the
                         wall. Pivot: the middle of its footprint, the open side south. SOCKET_Smoke, SOCKET_Anvil,
                         SOCKET_Light.
  --preview  renders them into Saved/ArtPreviews/RansomsRest/Mountain/.
"""
import math

import bmesh
import bpy
from mathutils import Matrix, Vector

import looter_buildings as kit
import looter_homes as homes
import looter_mountain as mtn
import looter_ruins as lr
import looter_textures as lt
import looter_town as town
from looter_buildings import Opening, Space, Tile, Trim, place


def yaw_facing(fx, fy):
    return math.degrees(math.atan2(fx, -fy))


# --- The portal ---

PY0, PY1 = -3.4, 0.0                    # the entry's front and back along Y
POST_X, POST, CAP_Z, CAP_H = 1.825, 0.25, 2.95, 0.28
BOARD = (-4.3, -4.5)                    # the tag board's middle on the ground (west of the mouth, in front of the wing)
# (x, z) on the board's face: IN has twelve hooks, OUT twenty (the shift's nine that came up and eleven left empty)
HOOKS_IN = [(-0.68 + 0.17 * c, 0.78 - 0.2 * r) for r in range(3) for c in range(4)]
HOOKS_OUT = [(0.1 + 0.15 * c, 0.78 - 0.18 * r) for r in range(4) for c in range(5)]


def board_frame():
    """The tag board's frame: its face toward the approach (south-east), x along it, z up from the ground."""
    turn = math.radians(25.0)
    return Space(Matrix.Translation((BOARD[0], BOARD[1], 0.0)) @ Matrix.Rotation(turn, 4, 'Z'))


def tag(m, space, at, seed, number=None):
    """A brass tag (a disc 4 cm across, a hole for its hook) hung at `at` on the board's face."""
    import random
    rnd = random.Random(seed)
    x, z = at
    tb = kit._new_bmesh()
    bmesh.ops.create_cone(tb, cap_ends=True, cap_tris=False, segments=10, radius1=0.02, radius2=0.02, depth=0.003)
    m.emit(tb, Tile('MetalWorn'), 'brass', Matrix.Translation((x, -0.035, z - 0.035)) @
           Matrix.Rotation(math.radians(90.0), 4, 'X') @ Matrix.Rotation(math.radians(rnd.uniform(-10, 10)), 4, 'Y'), space)


def mine_portal():
    m = mtn.Model('RR_MinePortal', seed=8101)
    rng = m.rng
    # The entry: sets every 1.1 m, lagged on the roof and walls, the front set heavier.
    for k, y in enumerate((-2.85, -1.75, -0.65)):
        for sx in (-1.0, 1.0):
            mtn.post(m, sx * POST_X, y, -0.12, CAP_Z, size=POST)
        mtn.beam(m, (-2.15, y, CAP_Z + CAP_H * 0.5), (2.15, y, CAP_Z + CAP_H * 0.5), CAP_H, POST + 0.02)
    for sx in (-1.0, 1.0):
        mtn.post(m, sx * 2.0, PY0 + 0.2, -0.3, 3.05, size=0.36)
        m.hull((0.36, 0.36, 3.35), at=(sx * 2.0, PY0 + 0.2, 1.37))
    mtn.beam(m, (-2.65, PY0 + 0.2, 3.25), (2.65, PY0 + 0.2, 3.25), 0.4, 0.4)
    m.hull((5.3, 0.4, 0.4), at=(0.0, PY0 + 0.2, 3.25))
    # Roof lagging (planks along Y over the caps) and wall lagging (planks along Y outside the posts).
    top = CAP_Z + CAP_H
    for k in range(20):
        x = -2.05 + 4.1 * (k + 0.5) / 20
        if rng.random() < 0.06:
            continue
        mtn.plank(m, (x, PY0 + 0.4, top + 0.025), (x, PY1, top + 0.025), 4.1 / 20 - 0.015, 0.05, face=(0.0, 0.0, 1.0),
                  look='A')
    for sx in (-1.0, 1.0):
        for k in range(14):
            z = 0.12 + (top - 0.15) * (k + 0.5) / 14
            mtn.plank(m, (sx * (POST_X + POST * 0.5 + 0.03), PY0 + 0.45, z), (sx * (POST_X + POST * 0.5 + 0.03), PY1, z),
                      (top - 0.15) / 14 - 0.012, 0.05, face=(sx, 0.0, 0.0), look='A')
        m.hull((0.3, PY1 - PY0, top), at=(sx * (POST_X + 0.15), (PY0 + PY1) * 0.5, top * 0.5))
    m.hull((4.6, PY1 - PY0, 0.4), at=(0.0, (PY0 + PY1) * 0.5, top + 0.2))
    m.section('entry')
    # The wings: log cribs either side holding the cut slope (logs along X at the front, along Y at the ends).
    for sx in (-1.0, 1.0):
        x0, x1 = sx * 2.25, sx * 4.6
        z = -0.4
        k = 0
        while z < 3.5:
            r = rng.uniform(0.15, 0.18)
            if k % 2 == 0:
                for y in (PY0 + 0.05, PY0 + 2.2):
                    mtn.log(m, (x0 - sx * 0.1, y, z + r), (x1 + sx * 0.25, y, z + r), r=r, out=(0.0, -1.0, 0.0), sides=8)
            else:
                for x in (x0 + sx * 0.15, x1 - sx * 0.05):
                    mtn.log(m, (x, PY0 - 0.25, z + r), (x, PY0 + 2.55, z + r), r=r, out=(sx, 0.0, 0.0), sides=8)
            z += r * 1.65
            k += 1
        m.hull((abs(x1 - x0) + 0.3, 2.9, 3.9), at=((x0 + x1) * 0.5, PY0 + 1.15, 1.55))
        # Rock and earth fill showing between the logs at the back.
        town.box(m, (abs(x1 - x0), 2.0, 3.4), at=((x0 + x1) * 0.5, PY0 + 1.15, 1.4), look='granite')
    # The brow: logs laid across over the cap, holding the overburden.
    z = 3.5
    for k in range(5):
        r = rng.uniform(0.15, 0.19)
        mtn.log(m, (-4.7, PY0 + 0.35 + 0.05 * k, z + r), (4.7, PY0 + 0.35 + 0.05 * k, z + r + rng.uniform(-0.05, 0.05)), r=r,
                sides=8)
        z += r * 1.85
    m.hull((9.4, 0.5, z - 3.45), at=(0.0, PY0 + 0.45, (z + 3.45) * 0.5))
    m.section('wings')
    # The header: a black board on the front cap, PROVIDENCE MINING CO. in cream, and a smaller No. 1 ADIT.
    hz = 3.82
    town.box(m, (3.6, 0.06, 0.56), at=(0.0, PY0 - 0.02, hz), look='woodblack')
    town.text(m, 'PROVIDENCE MINING CO.', (0.0, PY0 - 0.055, hz + 0.05), height=0.2, width=3.3, look='cream', lift=0.0)
    town.text(m, 'No. 1 ADIT', (0.0, PY0 - 0.055, hz - 0.18), height=0.1, look='cream', lift=0.0)
    # The doors: two plank leaves, the west one swung right back against the crib, the east one hanging half open.
    for sx, swing in ((-1.0, 100.0), (1.0, 55.0)):
        hinge = Vector((sx * (POST_X - 0.02), PY0 + 0.0, 0.05))
        d = Matrix.Rotation(math.radians(swing) * sx, 3, 'Z') @ Vector((-sx, 0.0, 0.0))     # swung out (south)
        for k in range(8):
            p = hinge + d * (0.11 + 0.22 * k)
            mtn.plank(m, (p.x, p.y, 0.1), (p.x, p.y, 2.75 - rng.uniform(0.0, 0.05)), 0.21, 0.05,
                      face=tuple(Vector((d.y, -d.x, 0.0)) * (-sx)), look='A')
        for z in (0.45, 2.3):
            q0, q1 = hinge + d * 0.1, hinge + d * 1.7
            mtn.plank(m, (q0.x, q0.y, z), (q1.x, q1.y, z), 0.16, 0.04, face=tuple(Vector((d.y, -d.x, 0.0)) * (-sx)), look='A',
                      lift=0.045)
    # Plank walks either side of the track's way, from the mouth out over the threshold.
    for x0, x1 in ((-1.65, -0.85), (0.85, 1.65)):
        mtn.deck(m, x0, x1, PY0 - 0.3, PY1 - 0.1, 0.05, along='y')
    m.socket('Mouth', (0.0, 0.0, 0.0))
    mtn.lantern(m, (-2.0 + 0.32, PY0 - 0.05, 2.45), name='Light')
    m.section('front')
    # The tag board: two posts, a little roof, IN and OUT, a hook on each place, the tags that came up on OUT.
    sp = board_frame()
    for x in (-0.95, 0.95):
        mtn.beam(m, (x, 0.05, -0.4), (x, 0.05, 2.25), 0.12, 0.12, space=sp)
    town.box(m, (1.7, 0.05, 1.15), at=(0.0, 0.05, 1.42), look='woodblack', space=sp)
    for x in (-0.45, 0.45):
        town.box(m, (0.7, 0.03, 0.18), at=(x, 0.01, 2.13), look='cream', space=sp)
    town.text(m, 'IN', (-0.45, -0.006, 2.13), height=0.12, look='black', lift=0.0, space=sp)
    town.text(m, 'OUT', (0.45, -0.006, 2.13), height=0.12, look='black', lift=0.0, space=sp)
    town.box(m, (0.04, 0.03, 1.05), at=(0.0, 0.01, 1.42), look='C', space=sp)
    for x, z in HOOKS_IN + HOOKS_OUT:
        town.box(m, (0.012, 0.05, 0.012), at=(x, -0.02, z + 1.0), look='iron', space=sp)
    for k, (x, z) in enumerate(HOOKS_OUT[:9]):
        tag(m, sp, (x, z + 1.0), 8110 + k)
    roof = Space(sp.matrix @ Matrix.Translation((0.0, -0.05, 2.35)) @ Matrix.Rotation(math.radians(-25.0), 4, 'X'))
    town.box(m, (2.1, 0.55, 0.04), at=(0.0, 0.0, 0.0), look='E', space=roof)
    lr.hull_along(m, sp.world((-1.0, 0.05, 0.0)), sp.world((1.0, 0.05, 0.0)), 2.3, thick=0.15)
    face = sp.world((0.0, -0.45, 0.0))
    m.socket('TagBoard', tuple(face), (0.0, 0.0, 25.0))
    m.section('tag board')
    return m.finish(view=(-0.6, -1.0, 0.35))


def tags(name, hooks, seed):
    m = mtn.Model(name, seed=seed)
    sp = board_frame()
    for k, (x, z) in enumerate(hooks[:11]):
        tag(m, sp, (x, z + 1.0), seed + k)
    return m.finish(Nanite=0, Collision='None')


# --- The ore bin ---

BIN_TOP = 3.6


def ore_bin():
    m = mtn.Model('RR_OreBin', seed=8201)
    rng = m.rng
    H = 2.0
    # Posts round the bin and its walls of upright planks from 1.2 m to the deck, braces under it.
    for x in (-H, 0.0, H):
        for y in (-H, 0.0, H):
            if x == 0.0 and y == 0.0:
                continue
            mtn.post(m, x, y, -0.4, BIN_TOP - 0.05, size=0.3)
            m.hull((0.3, 0.3, BIN_TOP + 0.35), at=(x, y, (BIN_TOP - 0.45) * 0.5))
    for (p0, p1) in (((-H, -H, 0.0), (H, -H, 0.0)), ((H, -H, 0.0), (H, H, 0.0)), ((H, H, 0.0), (-H, H, 0.0)),
                     ((-H, H, 0.0), (-H, -H, 0.0))):
        d = (Vector(p1) - Vector(p0)).normalized()
        out = Vector((d.y, -d.x, 0.0))
        L = 2.0 * H
        n = int(L / 0.22)
        for k in range(n):
            p = Vector(p0) + d * (L * (k + 0.5) / n) + out * 0.17
            mtn.plank(m, (p.x, p.y, 1.2), (p.x, p.y, BIN_TOP - 0.1), L / n - 0.012, 0.05, face=tuple(out), look='A')
        mid = (Vector(p0) + Vector(p1)) * 0.5 + out * 0.17
        m.hull((L if abs(d.x) > 0.5 else 0.1, L if abs(d.y) > 0.5 else 0.1, BIN_TOP - 1.2), at=(mid.x, mid.y, (BIN_TOP + 1.2) * 0.5))
        for z in (1.25, BIN_TOP - 0.2):
            q0, q1 = Vector(p0) + out * 0.2, Vector(p1) + out * 0.2
            mtn.beam(m, (q0.x, q0.y, z), (q1.x, q1.y, z), 0.2, 0.12, face=tuple(out))
        for a, b in ((p0, p1),):
            q0 = Vector(a) + out * 0.2
            q1 = Vector(b) + out * 0.2
            mtn.beam(m, (q0.x, q0.y, 0.2), ((q0.x + q1.x) * 0.5, (q0.y + q1.y) * 0.5, 1.15), 0.14, 0.12, face=tuple(out))
            mtn.beam(m, (q1.x, q1.y, 0.2), ((q0.x + q1.x) * 0.5, (q0.y + q1.y) * 0.5, 1.15), 0.14, 0.12, face=tuple(out))
    # The top deck round a grizzly of iron bars over the bin's mouth; the trestle north at the same height.
    for x0, x1, y0, y1 in ((-H - 0.3, H + 0.3, -H - 0.3, -0.8), (-H - 0.3, H + 0.3, 0.8, H + 0.3), (-H - 0.3, -0.8, -0.8, 0.8),
                           (0.8, H + 0.3, -0.8, 0.8)):
        mtn.deck(m, x0, x1, y0, y1, BIN_TOP, along='y')
        m.hull((x1 - x0, y1 - y0, 0.25), at=((x0 + x1) * 0.5, (y0 + y1) * 0.5, BIN_TOP - 0.125))
    for k in range(9):
        x = -0.8 + 1.6 * (k + 0.5) / 9
        mtn.iron_box(m, (0.05, 1.7, 0.08), at=(x, 0.0, BIN_TOP - 0.06))
    m.hull((1.6, 1.6, 0.12), at=(0.0, 0.0, BIN_TOP - 0.06))
    TR = 6.0
    for y in (H + 2.0, H + 4.0, H + TR):
        for x in (-0.9, 0.9):
            mtn.post(m, x, y, -1.6, BIN_TOP - 0.2, size=0.24)
            m.hull((0.24, 0.24, BIN_TOP + 1.4), at=(x, y, (BIN_TOP - 1.8) * 0.5))
        mtn.beam(m, (-1.1, y, BIN_TOP - 0.3), (1.1, y, BIN_TOP - 0.3), 0.2, 0.24, face=(0.0, -1.0, 0.0))
        mtn.beam(m, (-0.9, y, -0.2), (0.9, y, BIN_TOP - 0.6), 0.14, 0.12, face=(0.0, -1.0, 0.0))
    for x in (-0.6, 0.6):
        mtn.beam(m, (x, H, BIN_TOP - 0.14), (x, H + TR + 0.2, BIN_TOP - 0.14), 0.18, 0.2, face=(1.0, 0.0, 0.0))
    mtn.deck(m, -1.1, 1.1, H + 0.3, H + TR + 0.3, BIN_TOP, along='y')
    m.hull((2.2, TR, 0.25), at=(0.0, H + 0.3 + TR * 0.5, BIN_TOP - 0.125))
    lr.railing(m, (-1.05, H + 0.35, BIN_TOP), (-1.05, H + TR + 0.25, BIN_TOP), height=1.0, post_every=2.0, mid=0.5,
               post=0.1, rail=(0.1, 0.05))
    lr.hull_along(m, (-1.05, H + 0.35, BIN_TOP), (-1.05, H + TR + 0.25, BIN_TOP), 1.05, thick=0.1)
    m.socket('TrackTop', (0.0, H + TR + 0.3, BIN_TOP), (0.0, 0.0, 180.0))
    # The chute south over the lower track: a plank trough sloping down, an iron gate and its lever.
    c0, c1 = Vector((0.0, -H - 0.1, 1.5)), Vector((0.0, -H - 1.4, 1.05))
    d = (c1 - c0).normalized()
    for sx in (-1.0, 1.0):
        mtn.plank(m, tuple(c0 + Vector((sx * 0.4, 0.0, 0.1))), tuple(c1 + Vector((sx * 0.4, 0.0, 0.1))), 0.35, 0.05,
                  face=(sx, 0.0, 0.0), look='A')
    mtn.plank(m, tuple(c0 - Vector((0.0, 0.0, 0.05))), tuple(c1 - Vector((0.0, 0.0, 0.05))), 0.8, 0.06,
              face=tuple(Vector((0.0, d.z, -d.y)) * -1.0), look='A')
    mtn.iron_box(m, (0.8, 0.04, 0.4), at=(0.0, -H - 1.38, 1.25))
    mtn.beam(m, (0.55, -H - 0.3, 1.4), (0.75, -H - 0.9, 2.4), 0.06, 0.06, face=(1.0, 0.0, 0.0))
    m.socket('Chute', tuple(c1 - Vector((0.0, 0.0, 0.2))), (0.0, 0.0, 0.0))
    m.socket('TrackBottom', (0.0, -H - 1.4, 0.0), (0.0, 0.0, 0.0))
    for k in range(10):
        s = rng.uniform(0.12, 0.28)
        town.box(m, (s, s * 0.8, s * 0.55), at=(rng.uniform(-0.9, 0.9), -H - 1.6 + rng.uniform(-0.6, 0.6), s * 0.2),
                 rot=(rng.uniform(-20, 20), rng.uniform(-20, 20), rng.uniform(0, 90)), look='granite', bevel=0.02)
    return m.finish(view=(-0.8, -1.0, 0.45))


# --- The powder house ---

def powder_house():
    m = mtn.Model('RR_PowderHouse', seed=8301)
    X0, X1, Y0, Y1 = -1.5, 1.5, -2.0, 2.0
    T, WALL = 0.45, 2.65
    DOOR = (1.0, 2.0, 1.0, 1.9)     # x along the front, its width, height
    B = -0.3                        # the walls' foot below the ground
    rise = (Y1 - Y0) * 0.5 * math.tan(math.radians(18.0))
    gable = [(0.0, B), (4.0, B), (4.0, WALL), (2.0, WALL + rise), (0.0, WALL)]
    walls = [((X0, Y0), (X1, Y0), [(0.0, B), (DOOR[0], B), (DOOR[0], DOOR[3]), (DOOR[0] + DOOR[2], DOOR[3]),
                                   (DOOR[0] + DOOR[2], B), (3.0, B), (3.0, WALL), (0.0, WALL)]),
             ((X1, Y0), (X1, Y1), gable),
             ((X1, Y1), (X0, Y1), [(0.0, B), (3.0, B), (3.0, WALL), (0.0, WALL)]),
             ((X0, Y1), (X0, Y0), gable)]
    for k, (p0, p1, outline) in enumerate(walls):
        sp = kit.wall_space(p0, p1, 0.0)
        holes = [kit.rect(1.75, WALL + rise * 0.25, 0.5, 0.16)] if k in (1, 3) else []     # vents in the gables
        town.panel(m, outline, holes, T, Trim('D', world=True, u=m.rng.uniform(0, 6.4)), space=sp, slices=0, back=False)
    m.hull((3.1, 4.1, WALL + 0.3), at=(0.0, 0.0, (WALL - 0.3) * 0.5))
    # Quoins of dressed stone at the corners.
    for x, y in ((X0, Y0), (X1, Y0), (X1, Y1), (X0, Y1)):
        z = -0.25
        k = 0
        while z < WALL - 0.1:
            h = 0.3 if k % 2 == 0 else 0.22
            sx = 1.0 if x > 0 else -1.0
            sy = 1.0 if y > 0 else -1.0
            town.box(m, (0.5 if k % 2 == 0 else 0.3, 0.3 if k % 2 == 0 else 0.5, h - 0.02),
                     at=(x - sx * (0.2 if k % 2 == 0 else 0.1), y - sy * (0.1 if k % 2 == 0 else 0.2), z + h * 0.5), look='D')
            z += h
            k += 1
    # The roof: a board deck and shakes, low.
    front, back = kit.gable(m, X0 - 0.05, X1 + 0.05, Y0, Y1, WALL, 18.0, overhang=0.3, rake=0.25, deck=0.06, sag=0.02)
    kit.shingles(m, front, piece=(2.0, 3.0))
    kit.shingles(m, back, piece=(2.0, 3.0))
    kit.ridge_cap(m, front, back, uv='C', width=0.18)
    homes.gable_hull(m, X0, X1, Y0, Y1, WALL, WALL, 0.0, WALL + rise, overhang=0.3, rake=0.25, pitch_front=18.0)
    # The door: heavy planks, iron straps, a hasp with its padlock; the sign over it.
    sp = kit.wall_space((X0, Y0), (X1, Y0), 0.0)
    x0, w, h = DOOR[0], DOOR[2], DOOR[3]
    for k in range(5):
        x = x0 + w * (k + 0.5) / 5
        mtn.plank(m, (x, T * 0.4, 0.0), (x, T * 0.4, h - 0.02), w / 5 - 0.008, 0.06, face=(0.0, -1.0, 0.0), look='A', space=sp)
    for z in (0.3, 1.0, 1.65):
        mtn.iron_box(m, (w + 0.05, 0.02, 0.08), at=(x0 + w * 0.5, T * 0.4 - 0.04, z), space=sp)
    mtn.iron_box(m, (0.12, 0.04, 0.04), at=(x0 + w - 0.1, T * 0.4 - 0.07, 1.0), space=sp)
    mtn.iron_box(m, (0.07, 0.03, 0.09), at=(x0 + w - 0.06, T * 0.4 - 0.1, 0.92), space=sp)
    town.box(m, (1.4, 0.05, 0.36), at=(1.5, -0.03, h + 0.32), look='cream', space=sp)
    town.text(m, 'POWDER', (1.5, -0.056, h + 0.32), height=0.2, look='oxide', lift=0.0, space=sp)
    m.socket('Door', (X0 + x0 + w * 0.5, Y0 - 0.6, 0.0), (0.0, 0.0, 0.0))
    return m.finish(view=(-0.7, -1.0, 0.4))


# --- The smithy ---

def smithy():
    m = mtn.Model('RR_Smithy', seed=8401)
    rng = m.rng
    X0, X1, Y0, Y1 = -2.5, 2.5, -2.0, 2.0
    FRONT, BACK = 2.6, 2.1
    for x in (X0, 0.0, X1):
        mtn.post(m, x, Y0 + 0.08, -0.3, FRONT, size=0.18)
        mtn.post(m, x, Y1 - 0.08, -0.3, BACK, size=0.18)
        m.hull((0.18, 0.18, FRONT + 0.3), at=(x, Y0 + 0.08, (FRONT - 0.3) * 0.5))
    sp_b = kit.wall_space((X1, Y1), (X0, Y1), 0.0)
    m.panel([(0.0, 0.0), (5.0, 0.0), (5.0, BACK), (0.0, BACK)], [], 0.05, Trim('A', world=True, rotate=True), space=sp_b,
            slices=0)
    homes.battens(m, sp_b, [(0.0, 0.0), (5.0, 0.0), (5.0, BACK), (0.0, BACK)], 0.0, 5.0, [(1.6, 3.4, -1.0, 9.0)], look='A')
    m.hull((5.1, 0.1, BACK), at=(0.0, Y1, BACK * 0.5))
    for p0, p1, h0, h1 in (((X0, Y1), (X0, Y0), BACK, FRONT), ((X1, Y0), (X1, Y1), FRONT, BACK)):
        sp = kit.wall_space(p0, p1, 0.0)
        outline = [(0.0, 0.0), (4.0, 0.0), (4.0, h1), (0.0, h0)]
        m.panel(outline, [], 0.05, Trim('A', world=True, rotate=True), space=sp, slices=0)
        homes.battens(m, sp, outline, 0.0, 4.0, [], look='A')
        m.hull((0.1 if abs(p0[0] - p1[0]) < 0.1 else 4.0, 4.0 if abs(p0[0] - p1[0]) < 0.1 else 0.1, FRONT),
               at=((p0[0] + p1[0]) * 0.5, (p0[1] + p1[1]) * 0.5, FRONT * 0.5))
    for y, z in ((Y0 + 0.08, FRONT), (Y1 - 0.08, BACK)):
        mtn.beam(m, (X0 - 0.2, y, z + 0.08), (X1 + 0.2, y, z + 0.08), 0.16, 0.16)
    # The roof: shakes sloping back from the open front to the back wall.
    # Up from the back eave (north, low) to the front (south, high): x along the eave from east to west keeps its top up.
    run = Vector((0.0, -4.0, FRONT - BACK))
    slope = kit.Slope(Vector((X1 + 0.25, Y1 + 0.4, BACK + 0.16 - 0.4 * (FRONT - BACK) / 4.0)), Vector((-1.0, 0.0, 0.0)),
                      run.normalized(), X1 - X0 + 0.5, run.length + 0.8, 0.03)
    # The deck as one slab of planks (each face mapped flat on its own axis: the thin top edge keeps a clean tangent).
    m.emit(kit._box((slope.width, slope.length, 0.06), 0.0, int(slope.width / 1.1), ()), homes.Boards(rotate=False),
           'planks', place((slope.width * 0.5, slope.length * 0.5, -0.03)), slope)
    stack = (-1.0, 1.35)
    kit.shingles(m, slope, piece=(2.0, 3.2), skip=lambda x, y: (Vector(slope.world((x, y, 0.0)).xy) - Vector(stack)).length < 0.5)
    homes.slope_hull(m, slope)
    # The forge: a stone hearth against the back wall, its hood, the stack through the roof; the bellows beside it.
    fx, fy = stack
    kit.stone_stack(m, (fx, fy, -0.1), (1.5, 1.1), 0.9, taper=0.02)
    for k in range(5):
        s = rng.uniform(0.08, 0.14)
        town.box(m, (s, s * 0.8, s * 0.5), at=(fx + rng.uniform(-0.35, 0.35), fy + rng.uniform(-0.3, 0.2), 0.82),
                 rot=(0, 0, rng.uniform(0, 90)), look='charcoal')
    town.cylinder(m, (fx, fy + 0.1, 1.75), (fx, fy + 0.1, 1.15), 0.18, 0.6, sides=8, look='iron', face=(0.0, -1.0, 0.0))
    kit.stone_stack(m, (fx, fy + 0.15, 1.7), (0.55, 0.55), 2.2, taper=0.05)
    homes.flue_cap(m, fx, fy + 0.15, 3.9, size=(0.5, 0.5), socket='Smoke')
    m.hull((1.5, 1.1, 0.9), at=(fx, fy, 0.35))
    m.hull((0.6, 0.6, 2.3), at=(fx, fy + 0.15, 2.85))
    bx, by = fx + 1.15, fy - 0.1
    town.box(m, (0.5, 0.9, 0.06), at=(bx, by, 0.85), rot=(5.0, 0.0, 0.0), look='C')
    town.box(m, (0.46, 0.85, 0.06), at=(bx, by, 0.6), look='C')
    town.box(m, (0.48, 0.8, 0.2), at=(bx, by, 0.72), look='crepe')
    mtn.rod(m, (bx, by + 0.45, 0.85), (bx, by + 0.9, 1.2), r=0.02)
    for x in (bx - 0.2, bx + 0.2):
        mtn.post(m, x, by - 0.3, 0.0, 0.58, size=0.06)
        mtn.post(m, x, by + 0.3, 0.0, 0.58, size=0.06)
    # The anvil on its stump, the quench barrel, the grindstone, the tools on the back wall.
    ax, ay = 0.4, -0.2
    mtn.log(m, (ax, ay, -0.05), (ax, ay, 0.55), r=0.24, sides=10)
    mtn.iron_box(m, (0.55, 0.16, 0.12), at=(ax, ay, 0.62))
    mtn.iron_box(m, (0.32, 0.12, 0.08), at=(ax, ay, 0.72))
    mtn.iron_box(m, (0.6, 0.18, 0.08), at=(ax + 0.02, ay, 0.8))
    town.cylinder(m, (ax + 0.32, ay, 0.8), (ax + 0.5, ay, 0.8), 0.045, 0.01, sides=6, look='iron', face=(0.0, 0.0, 1.0))
    m.hull((0.6, 0.5, 0.85), at=(ax, ay, 0.42))
    m.socket('Anvil', (ax, ay - 0.6, 0.0), (0.0, 0.0, 0.0))
    homes.barrel(m, (1.5, 0.5, 0.0), height=0.8, radius=0.28)
    gx, gy = -1.9, -0.9
    mtn.sheave(m, (gx, gy, 0.75), (0.0, 1.0, 0.0), 0.4, width=0.1, spokes=0, groove=False, hub=0.06)
    town.cylinder(m, (gx, gy - 0.05, 0.75), (gx, gy + 0.05, 0.75), 0.36, sides=14, look='granite', face=(0.0, 0.0, 1.0))
    for y in (gy - 0.2, gy + 0.2):
        mtn.beam(m, (gx - 0.35, y, 0.0), (gx, y, 0.72), 0.06, 0.06, face=(0.0, y - gy, 0.0))
        mtn.beam(m, (gx + 0.35, y, 0.0), (gx, y, 0.72), 0.06, 0.06, face=(0.0, y - gy, 0.0))
    mtn.rod(m, (gx, gy - 0.3, 0.75), (gx, gy + 0.3, 0.75), r=0.02)
    rack = kit.wall_space((X1, Y1), (X0, Y1), 0.0)
    town.box(m, (1.6, 0.05, 0.1), at=(1.2, -0.05, 1.6), look='C', space=rack)
    for k in range(5):
        x = 0.6 + 0.28 * k
        mtn.iron_box(m, (0.03, 0.03, 0.42), at=(x, -0.09, 1.38), space=rack)
        mtn.iron_box(m, (0.08, 0.04, 0.05), at=(x, -0.09, 1.16), space=rack)
    mtn.lantern(m, (0.0, Y0 + 0.3, FRONT - 0.15), name='Light')
    return m.finish(view=(-0.6, -1.0, 0.4))


portal = mine_portal()
tags_in = tags('RR_TagBoard_TagsIn', HOOKS_IN, 8150)
tags_out = tags('RR_TagBoard_TagsOut', HOOKS_OUT[9:], 8170)
bin_ = ore_bin()
powder = powder_house()
smith = smithy()

if lt.want_preview():
    scene, sun = mtn.stage()
    human = homes.figure()
    human.location = (2.8, -5.0, 0.0)
    mtn.show_only([portal, tags_in, human])
    bpy.context.view_layer.update()
    mtn.shot(scene, sun, [portal], 'MinePortal', view=(-0.35, -1.0, 0.22), sun_dir=(30.0, 20.0), figure=human)
    homes.light(scene, sun, homes.golden_sun(40.0, 22.0))
    homes.shoot(scene, mtn.os.path.join(mtn.PREVIEW_DIR, 'TagBoard.png'), (-2.9, -6.2, 1.7), (-4.3, -4.5, 1.45), 30.0,
                homes.SHOT, -0.15)
    for obj, name, view, figure in ((bin_, 'OreBin', (-0.8, -1.0, 0.4), (2.5, -3.5, 0.0)),
                                    (powder, 'PowderHouse', (-0.6, -1.0, 0.35), (2.0, -3.0, 0.0)),
                                    (smith, 'Smithy', (-0.5, -1.0, 0.45), (2.9, -2.8, 0.0))):
        human.location = figure
        mtn.show_only([obj, human])
        mtn.shot(scene, sun, [obj], name, view=view, sun_dir=(30.0, 22.0), figure=human)
mtn.save_reports('Diggings')
