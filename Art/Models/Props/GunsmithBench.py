"""The gunsmith's bench (the shared contract's G3 bench, AGunsmithBench): a plain frontier workbench where the player
refits guns, in Skyreach's village and in Ransom's Rest, until Ozias takes it over when the Gilded Lily comes. A scripted
model (Art/README.md) in the frontier kit's look (Backlots.py, Ruins.py): the house trim sheet, WoodPlanks and the
brief's shared material names, put together with looter_buildings parts (boards, beams) and looter_props parts (blocks,
sweeps, lathes) in one looter_ruins Model.

  GunsmithBench   a thick, worn top of four planks (2.0 x 0.8 m, its surface 0.95 m up) on hewn legs with aprons, low
                  stretchers and a slatted shelf (a crate, a cartridge box and two rough stock blanks on it); a cast-iron
                  bench vise in oxide-red paint on the front edge at the left end; the back posts run up to a board
                  backboard (1.94 m) capped with a narrow shelf (two tins), and on the backboard hang a slotted rack of
                  three files, a rasp and four screwdrivers, a hacksaw, a ball-peen hammer and a brass hammer on pegs, a
                  hand brace and a coil of wire. On the top: a low parts box with six compartments (cartridges, pins, a
                  spring, small parts) at the back right, a brass oil can, a cleaning rod, a barrel blank, a loose
                  spring, pins and a cartridge, oil stains, and a red shop rag over the front edge at the right end.
                  The middle of the top is left clear for the gun (about 1.0 x 0.4 m).

The pivot is on the ground under the middle of the top; the front (where the player stands) faces -Y. 2.03 x 1.0 m with
the vise and the rag, 1.98 m high to the cap shelf (2.07 m with its tins). About 7.5k triangles.

Sockets (a socket's front is its empty's -Y, its top the empty's +Z; Art/README.md):
  Interact   the front, centered, 1.0 m up, facing out (the actor's forward in Unreal).
  Gun        the middle of the clear space, 3 cm above the top (the middle of a gun lying there), turned so a gun put
             on it with its forward (+X in Unreal, the muzzle) and up lies on its left side: muzzle to the right as seen
             from the front (Unreal -Y, like AWeaponRack's gun), its top toward the backboard, its grip and magazine
             toward the player. The middle of the gun's bounds belongs here (a gun's own origin is the back of its
             receiver).
  Box        the parts box, the middle of its rim, facing out (turned with the box).

Collision: three UCX_ boxes (the top and its frame down to the ground, the backboard with its posts, cap and tools, the
vise); the small things on the top have none. Nanite with a fallback of every triangle, like the dray and the woodshed;
baked vertex occlusion and oil stains in its alpha.

    artrun.ps1 -Script Art\\Models\\Props\\GunsmithBench.py -Preview      previews (with a 1.8 m figure and a stand-in gun
                                                                         on SOCKET_Gun) into Saved/ArtPreviews/Props/
    artrun.ps1 -Export GunsmithBench -Family GunsmithBench               export test into Intermediate/ArtExport_GunsmithBench
"""
import math
import os
import random

import bmesh
import bpy
from mathutils import Matrix, Vector, noise

import looter_buildings as kit
from looter_buildings import Tile, Trim
import looter_model as lm
import looter_props as lp
import looter_ruins as lr
import looter_textures as lt

# The brief's shared material names (MI_<name> in Unreal): one name, one look, in every model that uses it.
TRIM = lr.material('trim')
PLANKS = lr.material('planks')
IRON = lr.material('ironblack')
BRASS = lr.material('brass')
PAINT_OXIDE = lt.material('PaintWorn', name='PaintOxide', tint=0x8c3e2c)      # the vise (Backlots.py's DANGER red)

TOP = 0.95                       # the top's surface
TOP_T = 0.07                     # its planks' thickness
HX = 1.0                         # half its length
FRONT, BACK = -0.42, 0.38        # its front and back edges
LEG_X = 0.86                     # the legs' and posts' middles
LEG_FRONT_Y, LEG_BACK_Y = -0.30, 0.325
LEG = 0.11
BOARD_Y = 0.38                   # the backboard's front face (behind the posts)
BOARD_TOP = 1.94
CAP_TOP = BOARD_TOP + 0.035
TOOL_Y = BOARD_Y - 0.035         # where the hanging tools lie against the backboard
RACK_Z = 1.2                     # the slotted tool rack's middle
SINK = 0.003                     # loose things settle into the worn top a little
GUN_AT = (0.0, -0.1, TOP + 0.03)
BOX_AT, BOX_YAW = (0.62, 0.15), -6.0
BOX_SIZE = (0.34, 0.24, 0.05)
VISE_AT = (-0.72, FRONT, TOP)
PREVIEW_DIR = os.path.join(lt.REPO, 'Saved', 'ArtPreviews', 'Props')
STEEL = Tile('MetalWorn')
PAINT = Tile('PaintWorn')


def turned(profile, segments, mat, tex, grain='around', seed=0, center=(0.0, 0.0, 0.0), rotation=(0.0, 0.0, 0.0)):
    """A turned part (a lathe around Z; profile [(radius, z), ...]) in any material, mapped on a tileable set or a trim
    strip (tex), then turned and moved into place."""
    part, _ = lp.lathe(profile, segments=segments)
    lt.assign(part, mat)
    lp.lathe_uv(part, tex, grain=grain, seed=seed)
    lr.split_poles(part, grain)
    lp.place(part, center, rotation)
    return part


def steel_block(size, center, rotation=(0.0, 0.0, 0.0), mat=IRON, seed=0, bevel=0.0):
    part = lp.block(size, center, rotation, bevel=bevel)
    lt.assign(part, mat)
    lt.box_uv(part, 'MetalWorn' if mat is not PAINT_OXIDE else 'PaintWorn', seed=seed)
    return part


def rod(points, radius, key='ironblack', sides=5, radii=None, seed=0, caps=(True, True)):
    """A metal rod or wire along points (black iron or brass)."""
    return lr.tube(points, radius, sides=sides, key=key, set_name='MetalWorn', radii=radii, seed=seed, caps=caps)


def peg(m, x, z, length=0.06, radius=0.008):
    """A wooden peg standing out of the backboard."""
    m.cylinder((x, BOARD_Y + 0.005, z), (x, BOARD_Y - length, z), radius, sides=6, uv='C')


# --- The bench: top, frame, shelf, backboard ---

def top_planks(m):
    """Four thick planks, a little uneven, the front one's edge worn round, nailed down at the side aprons."""
    rnd = m.rng
    for k in range(4):
        y = FRONT + 0.1 + 0.2 * k
        x0, x1 = -HX - rnd.uniform(0.0, 0.015), HX + rnd.uniform(0.0, 0.015)
        length = x1 - x0
        part = lp.block((length, 0.196, TOP_T), (0.0, 0.0, 0.0), bevel=0.012 if k == 0 else 0.006)
        lr.PlankRow().apply(part, rnd)
        lt.assign(part, PLANKS)
        lp.slice_at(part, (1.0, 0.0, 0.0), [-length * 0.5 + length * i / 10 for i in range(1, 10)])
        lp.rough(part, 0.0015, 4.0, rnd.randint(0, 999))
        lp.place(part, ((x0 + x1) * 0.5, y, TOP - TOP_T * 0.5))
        m.add(part)
        for x in (-LEG_X, LEG_X):
            m.add(lp.nail((x + rnd.uniform(-0.01, 0.01), y + rnd.uniform(-0.04, 0.04), TOP), (0.0, 0.0, 1.0), size=0.018))


def frame(m):
    """Hewn legs (the back pair runs on up as the backboard's posts), aprons under the top, low stretchers carrying a
    slatted shelf."""
    beams = Trim('C', lane='each')
    for x in (-LEG_X, LEG_X):
        m.board((x, LEG_FRONT_Y, 0.0), (x, LEG_FRONT_Y, TOP - TOP_T), LEG, LEG, uv=beams, bevel=0.008, cuts=4)
        m.board((x, LEG_BACK_Y, 0.0), (x, LEG_BACK_Y, BOARD_TOP), LEG, LEG, uv=beams, bevel=0.008, cuts=9)
    apron_z = TOP - TOP_T - 0.05
    for y, side in ((LEG_FRONT_Y, -1.0), (LEG_BACK_Y, 1.0)):
        m.board((-LEG_X, y, apron_z), (LEG_X, y, apron_z), 0.1, 0.035, face=(0.0, side, 0.0), uv='C', cuts=4)
        m.board((-LEG_X, y, 0.21), (LEG_X, y, 0.21), 0.07, 0.045, face=(0.0, side, 0.0), uv='C', cuts=4)
    for x in (-LEG_X, LEG_X):
        side = 1.0 if x > 0.0 else -1.0
        m.board((x, LEG_FRONT_Y, apron_z), (x, LEG_BACK_Y, apron_z), 0.1, 0.035, face=(side, 0.0, 0.0), uv='C', cuts=1)
        m.board((x, LEG_FRONT_Y, 0.14), (x, LEG_BACK_Y, 0.14), 0.08, 0.05, face=(side, 0.0, 0.0), uv='C', cuts=1)
    # The shelf: seven short boards across the long stretchers, gaps between them.
    rnd = m.rng
    for k in range(7):
        x = -0.645 + 0.215 * k + rnd.uniform(-0.006, 0.006)
        m.board((x, LEG_FRONT_Y - 0.035, 0.2575), (x, LEG_BACK_Y + 0.035, 0.2575), 0.19, 0.025, face=(0.0, 0.0, 1.0),
                uv=lr.PlankRow(), mat='planks', cuts=2)


def backboard(m):
    """Five weathered boards behind the back posts up to their tops, a cap shelf over them, the slotted tool rack."""
    rnd = m.rng
    rise = (BOARD_TOP - TOP) / 5
    for k in range(5):
        z = TOP + rise * (k + 0.5)
        m.board((-0.915 - rnd.uniform(0.0, 0.01), BOARD_Y + 0.0125, z), (0.915 + rnd.uniform(0.0, 0.01), BOARD_Y + 0.0125, z),
                rise - 0.004, 0.025, uv='A', cuts=8)
        for x in (-LEG_X, LEG_X):
            m.add(lp.nail((x, BOARD_Y + 0.025, z + rnd.uniform(-0.03, 0.03)), (0.0, 1.0, 0.0), size=0.018))
    m.board((-0.98, 0.34, BOARD_TOP + 0.0175), (0.98, 0.34, BOARD_TOP + 0.0175), 0.2, 0.035, face=(0.0, 0.0, 1.0),
            uv=lr.PlankRow(), mat='planks', cuts=8)
    # The rack the files and screwdrivers stand in, handles up: a batten standing out from the boards.
    m.board((-0.79, BOARD_Y - 0.04, RACK_Z), (-0.08, BOARD_Y - 0.04, RACK_Z), 0.08, 0.035, face=(0.0, 0.0, 1.0), uv='C',
            cuts=3)
    for x in (-0.772, -0.1):
        m.add(lp.grain(lp.block((0.03, 0.06, 0.07), (x, BOARD_Y - 0.03, RACK_Z - 0.05)), 'Beams',
                       axis=(0.0, 0.0, 1.0), seed=rnd.randint(0, 999)))


# --- The vise ---

def vise(m):
    """A cast-iron bench vise in oxide-red paint bolted to the front edge: a swivel base, the body with its anvil face,
    the fixed jaw over the edge and the moving jaw out in front on its slide, steel jaw plates, the screw's boss and hub
    and a T-handle slid through and hanging down."""
    o = Vector(VISE_AT)
    rnd = random.Random(61)

    def painted(part, seed, smooth=None):
        # Chipped paint at about three times the set's density: chips a few centimetres across, dark iron under them.
        lt.assign(part, PAINT_OXIDE)
        lt.box_uv(part, 'PaintWorn', texel_density=960, seed=seed)
        lp.place(part, tuple(o))
        m.add(part, smooth=smooth)

    def casting(size, at, bevel, flare=None, seed=0):
        """A cast part: a chamfered box whose lower half narrows (flare: the bottom's share of the top's width)."""
        part = lp.block(size, at, bevel=bevel)
        if flare is not None:
            for v in part.data.vertices:
                if v.co.z < at[2]:
                    v.co.x *= flare
        painted(part, seed)

    def steel_box(size, at, bevel=0.0):
        m.box(size, at=tuple(o + Vector(at)), uv=STEEL, mat='ironblack', bevel=bevel)

    painted(turned([(0.0, -SINK), (0.08, -SINK), (0.08, 0.012), (0.066, 0.022), (0.0, 0.022)], 12, PAINT_OXIDE,
                   'PaintWorn', seed=1, center=(0.0, 0.1, 0.0)), 1, smooth=True)
    for k in range(3):
        a = math.radians(30.0 + 120.0 * k)
        m.add(turned([(0.0, 0.009), (0.011, 0.009), (0.011, 0.021), (0.0, 0.021)], 6, IRON, 'MetalWorn', seed=2 + k,
                     center=tuple(o + Vector((math.cos(a) * 0.072, 0.1 + math.sin(a) * 0.072, 0.0)))))
    casting((0.11, 0.2, 0.08), (0.0, 0.1, 0.062), 0.014, 0.85, seed=2)           # the body
    steel_box((0.09, 0.065, 0.016), (0.0, 0.16, 0.11))                           # its anvil face
    casting((0.165, 0.05, 0.11), (0.0, 0.025, 0.097), 0.008, 0.72, seed=3)       # the fixed jaw, flaring at the top
    casting((0.165, 0.05, 0.075), (0.0, -0.067, 0.1125), 0.008, 0.7, seed=4)     # the moving jaw's head
    casting((0.085, 0.046, 0.13), (0.0, -0.068, 0.015), 0.012, 0.8, seed=5)      # and its body, round the screw
    steel_box((0.14, 0.008, 0.034), (0.0, -0.004, 0.13))                         # the jaw plates, on the jaws' tops
    steel_box((0.14, 0.008, 0.034), (0.0, -0.038, 0.13))
    steel_box((0.06, 0.15, 0.04), (0.0, 0.015, 0.05))                            # the slide
    for x in (-0.045, 0.045):                                                    # the plates' screws, facing
        for y, turn in ((-0.008, 90.0), (-0.034, -90.0)):                        # each other across the gap
            m.add(turned([(0.0, 0.0), (0.006, 0.0), (0.005, 0.002), (0.0, 0.002)], 6, IRON, 'MetalWorn', seed=9,
                         center=tuple(o + Vector((x, y, 0.13))), rotation=(turn, 0.0, 0.0)))
    # The screw: a boss on the moving jaw, the hub, and the T-handle slid to one side, dropped.
    painted(turned([(0.0, 0.0), (0.026, 0.0), (0.026, 0.012), (0.02, 0.022), (0.0, 0.022)], 10, PAINT_OXIDE, 'PaintWorn',
                   seed=3, center=(0.0, -0.09, 0.0), rotation=(90.0, 0.0, 0.0)), 6, smooth=True)
    m.add(turned([(0.0, 0.0), (0.016, 0.0), (0.016, 0.03), (0.012, 0.034), (0.0, 0.034)], 8, IRON, 'MetalWorn', seed=4,
                 center=tuple(o + Vector((0.0, -0.11, 0.0))), rotation=(90.0, 0.0, 0.0)), smooth=True)
    hub = o + Vector((0.0, -0.128, 0.0))
    d = Vector((math.cos(math.radians(-24.0)), 0.0, math.sin(math.radians(-24.0))))
    ends = (hub - d * 0.07, hub + d * 0.15)
    m.add(rod(list(ends), 0.0065, sides=6, seed=5), smooth=True)
    for k, end in enumerate(ends):
        m.add(turned([(0.0, -0.013), (0.009, -0.0105), (0.013, 0.0), (0.009, 0.0105), (0.0, 0.013)], 6, IRON,
                     'MetalWorn', seed=6 + k, center=tuple(end), rotation=(0.0, 90.0 - 24.0, rnd.uniform(0.0, 60.0))),
              smooth=True)


# --- The tools on the backboard ---

def file_tool(m, x, length, width, thick, handle, seed, rasp=False):
    """A file (or a rasp) standing in the rack: its blade hangs below the batten, its wooden handle and brass ferrule
    above it."""
    top = RACK_Z + 0.0175
    blade = lp.block((width, thick, length), (0.0, 0.0, -length * 0.5))
    for v in blade.data.vertices:
        t = -v.co.z / length
        v.co.x *= 1.0 - (0.35 if rasp else 0.55) * t
        v.co.y *= 1.0 - 0.3 * t
    lt.assign(blade, IRON)
    lt.box_uv(blade, 'MetalWorn', seed=seed)
    lp.place(blade, (x, TOOL_Y, top + 0.004))
    m.add(blade)
    m.add(turned([(0.0, 0.0), (0.0105, 0.0), (0.0105, 0.012), (0.0, 0.012)], 6, BRASS, 'MetalWorn', seed=seed,
                 center=(x, TOOL_Y, top)), smooth=True)
    m.add(turned([(0.0, 0.0), (0.009, 0.0), (0.012, 0.015), (0.013, handle * 0.6), (0.0115, handle * 0.92),
                  (0.0, handle)], 6, TRIM, 'Beams', grain='up', seed=seed, center=(x, TOOL_Y, top + 0.012)), smooth=True)


def screwdriver(m, x, shaft, seed):
    top = RACK_Z + 0.0175
    m.add(rod([(x, TOOL_Y, top + 0.01), (x, TOOL_Y, top - shaft)], 0.0032, seed=seed), smooth=True)
    m.add(steel_block((0.0075, 0.0018, 0.016), (x, TOOL_Y, top - shaft - 0.007), seed=seed))
    m.add(turned([(0.0, 0.0), (0.008, 0.0), (0.0125, 0.012), (0.014, 0.05), (0.0125, 0.082), (0.0, 0.09)], 6, TRIM,
                 'Beams', grain='up', seed=seed, center=(x, TOOL_Y, top)), smooth=True)


def hammer(m, c, z, profile, head_r, handle, mat, seed):
    """A hammer hung by its head on two pegs, its handle hanging down between them."""
    for dx in (-0.026, 0.026):
        peg(m, c + dx, z)
    zc = z + 0.0068 + head_r            # on the pegs' flat tops (six-sided: 0.8 cm x cos 30)
    m.add(turned(profile, 8, mat, 'MetalWorn', seed=seed, center=(c, TOOL_Y, zc), rotation=(0.0, 90.0, 0.0)),
          smooth=True)
    grip = lp.sweep([(c, TOOL_Y, zc - head_r * 0.5), (c, TOOL_Y, zc - handle)], lp.ngon(0.0105, 6, squash=1.4),
                    scales=[0.85, 1.05])
    m.add(lp.grain(grip, 'Beams', axis=(0.0, 0.0, -1.0), seed=seed), smooth=True)


def hand_brace(m, c, seed):
    """A carpenter's brace hung on a peg by its crank: the head pad, the crank with its wooden grip, the chuck and a bit."""
    pts = [(0.0, 1.715), (0.0, 1.66), (0.075, 1.615), (0.075, 1.47), (0.0, 1.425), (0.0, 1.37)]
    m.add(rod([(c + x, TOOL_Y, z) for x, z in pts], 0.0055, sides=6, seed=seed), smooth=True)
    m.add(turned([(0.0, 1.705), (0.012, 1.705), (0.028, 1.72), (0.03, 1.742), (0.022, 1.758), (0.0, 1.763)], 8, TRIM,
                 'Beams', grain='up', seed=seed, center=(c, TOOL_Y, 0.0)), smooth=True)
    m.add(turned([(0.0, 1.49), (0.011, 1.49), (0.015, 1.505), (0.016, 1.545), (0.015, 1.585), (0.011, 1.6), (0.0, 1.6)],
                 8, TRIM, 'Beams', grain='up', seed=seed + 1, center=(c + 0.075, TOOL_Y, 0.0)), smooth=True)
    m.add(turned([(0.0, 1.38), (0.011, 1.38), (0.016, 1.356), (0.015, 1.322), (0.008, 1.3), (0.0, 1.298)], 8, IRON,
                 'MetalWorn', seed=seed + 2, center=(c, TOOL_Y, 0.0)), smooth=True)
    m.add(rod([(c, TOOL_Y, 1.302), (c, TOOL_Y, 1.21)], 0.003, seed=seed + 3), smooth=True)
    # The peg sits under the crank's upper bend.
    t = 0.035 / 0.075
    under = 1.66 - 0.045 * t - 0.0055 / math.cos(math.atan2(0.045, 0.075)) - 0.008
    peg(m, c + 0.035, under)


def frame_saw(m, x0, x1, seed):
    """A hacksaw on a peg: an iron frame whose back bends down into a wooden pistol grip, the blade along the bottom
    from the grip to the front post."""
    zt, zb = 1.69, 1.6
    mid = (x0 + x1) * 0.5
    m.add(rod([(x0 + 0.01, TOOL_Y, zb + 0.004), (x0, TOOL_Y, zt - 0.02), (x0 + 0.03, TOOL_Y, zt), (mid, TOOL_Y, zt + 0.01),
               (x1 - 0.02, TOOL_Y, zt), (x1, TOOL_Y, zt - 0.02), (x1, TOOL_Y, zb)], 0.0055, sides=6, seed=seed),
          smooth=True)
    m.add(steel_block((x1 - x0, 0.0015, 0.016), (mid, TOOL_Y, zb + 0.004), seed=seed))
    for x in (x0 + 0.01, x1):                       # the wing nuts holding the blade
        m.add(steel_block((0.012, 0.008, 0.022), (x, TOOL_Y, zb + 0.004), seed=seed + 1))
    # The grip leans back from the frame's back post, like a pistol's.
    grip = lp.block((0.03, 0.026, 0.11), (0.0, 0.0, -0.055), bevel=0.008)
    for v in grip.data.vertices:
        if v.co.z < -0.06:
            v.co.x *= 1.25
    lp.grain(grip, 'Beams', axis=(0.0, 0.0, 1.0), seed=seed)
    lp.place(grip, (x0 + 0.004, TOOL_Y, zb + 0.016), (0.0, 22.0, 0.0))
    m.add(grip, smooth=False)
    peg(m, mid, zt + 0.01 - 0.0055 - 0.008)


def wire_coil(m, c, z, seed):
    """A coil of iron wire hung on a peg: two loops sagging from it."""
    peg(m, c, z)
    radius = 0.075
    for k in range(2):
        pts = []
        for j in range(13):
            a = 2.0 * math.pi * j / 12
            pts.append((c + radius * 0.82 * math.sin(a) + 0.006 * k, TOOL_Y + 0.004 - 0.005 * k,
                        z - 0.008 - 0.004 - radius * (1.0 - math.cos(a)) * (1.0 + 0.05 * k)))
        m.add(rod(pts, 0.0028, sides=5, seed=seed + k, caps=(False, False)), smooth=True)


def tools(m):
    rnd = random.Random(71)
    for k, (x, length, width, thick, handle, rasp) in enumerate((
            (-0.735, 0.24, 0.028, 0.008, 0.11, True), (-0.675, 0.22, 0.022, 0.005, 0.1, False),
            (-0.615, 0.2, 0.02, 0.005, 0.1, False), (-0.555, 0.16, 0.014, 0.004, 0.09, False))):
        file_tool(m, x, length, width, thick, handle, 80 + k, rasp)
    for k, (x, shaft) in enumerate(((-0.46, 0.17), (-0.385, 0.13), (-0.31, 0.11), (-0.235, 0.08))):
        screwdriver(m, x, shaft, 90 + k)
    frame_saw(m, -0.66, -0.3, 95)
    # A ball-peen hammer and a gunsmith's brass hammer.
    hammer(m, 0.03, 1.6, [(0.0, -0.05), (0.012, -0.05), (0.014, -0.044), (0.014, -0.008), (0.01, 0.0), (0.0095, 0.014),
                          (0.012, 0.028), (0.013, 0.038), (0.011, 0.046), (0.006, 0.05), (0.0, 0.051)], 0.014, 0.3,
           IRON, 100)
    hammer(m, 0.21, 1.6, [(0.0, -0.04), (0.012, -0.04), (0.015, -0.036), (0.015, 0.036), (0.012, 0.04), (0.0, 0.04)],
           0.015, 0.25, BRASS, 101)
    hand_brace(m, 0.42, 102)
    wire_coil(m, 0.68, 1.64, 103)
    # Two tins on the cap shelf.
    m.add(turned([(0.0, -SINK), (0.045, -SINK), (0.045, 0.084), (0.042, 0.09), (0.0, 0.09)], 10, IRON, 'MetalWorn',
                 seed=104, center=(-0.62, 0.33, CAP_TOP)), smooth=True)
    m.add(turned([(0.0, -SINK), (0.03, -SINK), (0.03, 0.058), (0.022, 0.066), (0.022, 0.074), (0.0, 0.074)], 10, BRASS,
                 'MetalWorn', seed=105, center=(-0.49, 0.335, CAP_TOP), rotation=(0.0, 0.0, rnd.uniform(0.0, 40.0))),
          smooth=True)


# --- On the top ---

def cartridge(center, yaw=0.0, standing=False, seed=0):
    """A brass rifle cartridge: case, shoulder, neck and a dark bullet nose."""
    profile = [(0.0, 0.0), (0.0058, 0.0), (0.0058, 0.032), (0.0042, 0.037), (0.0042, 0.043), (0.0032, 0.052), (0.0, 0.056)]
    if standing:            # center: the middle of its base
        return turned(profile, 6, BRASS, 'MetalWorn', seed=seed, center=center, rotation=(0.0, 0.0, yaw))
    # Lying, center is its middle, its nose toward yaw.
    d = Vector((math.cos(math.radians(yaw)), math.sin(math.radians(yaw)), 0.0))
    return turned(profile, 6, BRASS, 'MetalWorn', seed=seed, center=tuple(Vector(center) - d * 0.028),
                  rotation=(0.0, 90.0, yaw))


def spring(center, length, radius, wire, turns, yaw=0.0, seed=0):
    """A coil spring lying along X (turned by yaw)."""
    n = turns * 5 + 1
    pts = []
    for i in range(n):
        a = 2.0 * math.pi * turns * i / (n - 1)
        pts.append(Vector((-length * 0.5 + length * i / (n - 1), radius * math.cos(a), radius * math.sin(a))))
    part = rod(pts, wire, sides=5, seed=seed)
    lp.place(part, center, (0.0, 0.0, yaw))
    return part


def pin(center, length, radius, yaw, seed=0):
    d = Vector((math.cos(math.radians(yaw)), math.sin(math.radians(yaw)), 0.0)) * (length * 0.5)
    return rod([Vector(center) - d, Vector(center) + d], radius, seed=seed)


def parts_box(m):
    """A low wooden tote with six compartments and a raised middle divider to carry it by: cartridges, pins, a spring
    and small parts in it. Its walls are low so a player standing at the bench sees into it."""
    rnd = random.Random(81)
    sx, sy, sz = BOX_SIZE
    wall = 0.012
    floor_z = 0.01
    parts = [lp.grain(lp.block((sx, sy, floor_z), (0.0, 0.0, floor_z * 0.5)), 'Beams', axis=(1.0, 0.0, 0.0), seed=1)]
    for side in (-1.0, 1.0):
        parts.append(lp.grain(lp.block((sx, wall, sz), (0.0, side * (sy - wall) * 0.5, sz * 0.5), bevel=0.002), 'Beams',
                              axis=(1.0, 0.0, 0.0), seed=rnd.randint(0, 999)))
        parts.append(lp.grain(lp.block((wall, sy - 2.0 * wall, sz), (side * (sx - wall) * 0.5, 0.0, sz * 0.5),
                                       bevel=0.002), 'Beams', axis=(0.0, 1.0, 0.0), seed=rnd.randint(0, 999)))
        parts.append(lp.grain(lp.block((0.008, sy - 2.0 * wall, sz - 0.01), (side * sx / 6.0, 0.0, (sz - 0.01) * 0.5)),
                              'Beams', axis=(0.0, 1.0, 0.0), seed=rnd.randint(0, 999)))
    # The middle divider rises in the middle into a handle.
    handle = lp.block((sx - 2.0 * wall, 0.01, 0.105), (0.0, 0.0, 0.0525))
    lp.slice_at(handle, (1.0, 0.0, 0.0), (-0.08, 0.08))
    for v in handle.data.vertices:
        if v.co.z > 0.08 and abs(v.co.x) > 0.09:
            v.co.z = sz + 0.006
    parts.append(lp.grain(handle, 'Beams', axis=(1.0, 0.0, 0.0), seed=rnd.randint(0, 999)))
    for x in (-sx * 0.5 + 0.03, sx * 0.5 - 0.03):
        for side in (-1.0, 1.0):
            parts.append(lp.nail((x, side * sy * 0.5, sz * 0.55), (0.0, side, 0.0), size=0.011))
    # The compartments: x -1, 0, 1 (left to right), y front (-) and back (+). Round things are smooth-shaded.
    cx, cy = sx / 3.0, sy * 0.25
    lie = floor_z + 0.0058
    round_parts = []
    for k in range(4):                                    # front left: cartridges lying in a row, a fifth across them
        round_parts.append(cartridge((-cx + 0.003 + rnd.uniform(-0.003, 0.003), -cy - 0.03 + 0.0135 * k, lie),
                                     yaw=rnd.uniform(-6.0, 6.0) + (180.0 if k % 2 else 0.0), seed=k))
    round_parts.append(cartridge((-cx - 0.01, -cy - 0.01, lie + 0.0112), yaw=75.0, seed=4))
    for k in range(6):                                    # back left: cartridges standing in two rows
        round_parts.append(cartridge((-cx - 0.024 + 0.024 * (k % 3), cy - 0.014 + 0.028 * (k // 3), floor_z),
                                     standing=True, seed=10 + k))
    for k in range(6):                                    # front middle: pins
        round_parts.append(pin((rnd.uniform(-0.028, 0.028), -cy + rnd.uniform(-0.02, 0.02), floor_z + 0.0025 + 0.001 * k),
                               rnd.uniform(0.022, 0.034), 0.0025, rnd.uniform(0.0, 180.0), seed=20 + k))
    round_parts.append(spring((cx, -cy - 0.012, floor_z + 0.009), 0.07, 0.0075, 0.0012, 5, yaw=8.0, seed=30))
    round_parts.append(pin((cx + 0.01, -cy + 0.026, floor_z + 0.0025), 0.03, 0.0025, 12.0, seed=31))
    for k, (x, y) in enumerate(((-0.026, -0.03), (0.022, 0.02), (-0.012, 0.03))):   # back middle: a trigger, a
        parts.append(steel_block((0.03, 0.008, 0.016), (x, cy + y, floor_z + 0.004),   # hammer and a sear
                                 (90.0, 0.0, rnd.uniform(-35.0, 35.0)), seed=40 + k))
    parts.append(steel_block((0.04, 0.012, 0.012), (cx, cy + 0.015, floor_z + 0.006), (0.0, 0.0, 25.0), seed=44))
    parts.append(steel_block((0.05, 0.01, 0.022), (cx - 0.005, cy - 0.025, floor_z + 0.005), (90.0, 0.0, -10.0), seed=45))
    frame_matrix = Matrix.Translation((BOX_AT[0], BOX_AT[1], TOP - SINK)) @ Matrix.Rotation(math.radians(BOX_YAW), 4, 'Z')
    for p, smooth in [(p, None) for p in parts] + [(p, True) for p in round_parts]:
        p.data.transform(frame_matrix)
        m.add(p, smooth=smooth)


def oil_can(m, x, y, seed):
    """A brass pump oiler: a squat body, a cone shoulder and a long thin spout."""
    m.add(turned([(0.0, -SINK), (0.046, -SINK), (0.05, 0.006), (0.05, 0.03), (0.046, 0.04), (0.03, 0.052), (0.014, 0.06),
                  (0.012, 0.072), (0.0, 0.074)], 12, BRASS, 'MetalWorn', seed=seed, center=(x, y, TOP)), smooth=True)
    pts = [(0.0, 0.0, 0.068), (0.014, -0.026, 0.108), (0.034, -0.07, 0.15), (0.05, -0.11, 0.174)]
    m.add(rod([(x + a, y + b, TOP + c) for a, b, c in pts], 0.006, key='brass', radii=[0.006, 0.0045, 0.0032, 0.0022],
              seed=seed), smooth=True)


def cleaning_rod(m, x0, x1, y, seed):
    """A brass cleaning rod with a wooden T-handle and a jag at its tip."""
    h = 0.011
    m.add(rod([(x0, y, TOP + h - SINK), (x1, y, TOP + 0.0045 - SINK)], 0.0045, key='brass', seed=seed), smooth=True)
    m.add(turned([(0.0, -0.045), (0.009, -0.045), (0.011, -0.03), (0.011, 0.03), (0.009, 0.045), (0.0, 0.045)], 6, TRIM,
                 'Beams', grain='up', seed=seed, center=(x0 - 0.005, y, TOP + h - SINK), rotation=(90.0, 0.0, 0.0)),
          smooth=True)
    m.add(turned([(0.0, 0.0), (0.006, 0.002), (0.006, 0.03), (0.0, 0.032)], 6, BRASS, 'MetalWorn', seed=seed,
                 center=(x1, y, TOP + 0.0045 - SINK), rotation=(0.0, 90.0, 0.0)), smooth=True)


def loose_parts(m):
    rnd = random.Random(91)
    # A barrel blank: a turned steel bar waiting to be bored.
    m.cylinder((-0.46, 0.17, TOP + 0.016 - SINK), (0.08, 0.13, TOP + 0.016 - SINK), 0.016, sides=8, uv=STEEL,
               mat='ironblack')
    m.add(spring((0.25, 0.215, TOP + 0.0105 - SINK), 0.06, 0.009, 0.0015, 5, yaw=-14.0, seed=1), smooth=True)
    for k, (x, y) in enumerate(((0.14, 0.19), (0.17, 0.165), (0.115, 0.155))):
        m.add(pin((x, y, TOP + 0.0025 - SINK * 0.5), rnd.uniform(0.025, 0.04), 0.0025, rnd.uniform(0.0, 180.0), seed=k),
              smooth=True)
    m.add(cartridge((0.33, 0.12, TOP + 0.0058 - SINK * 0.5), yaw=200.0, seed=7), smooth=True)


def rag(m, x0, x1, seed):
    """A red shop rag lying crumpled on the top and hanging over the front edge: two thin layers (each one's back is
    hidden), folds and lumps, a ragged hem. It is the vise's oxide paint, whose dark chips read as grease on cloth (no
    extra material)."""
    rnd = random.Random(seed)
    path = [(-0.17, TOP + 0.006), (-0.3, TOP + 0.006), (-0.403, TOP + 0.006), (-0.422, TOP - 0.003),
            (-0.428, TOP - 0.03), (-0.43, TOP - 0.1), (-0.427, TOP - 0.175)]
    lengths = [0.0]
    for a, b in zip(path, path[1:]):
        lengths.append(lengths[-1] + math.hypot(b[0] - a[0], b[1] - a[1]))
    total = lengths[-1]

    def along(s):
        for i in range(len(path) - 1):
            if s <= lengths[i + 1] or i == len(path) - 2:
                t = (s - lengths[i]) / max(lengths[i + 1] - lengths[i], 1e-6)
                (ya, za), (yb, zb) = path[i], path[i + 1]
                ty, tz = (yb - ya), (zb - za)
                n = math.hypot(ty, tz)
                return Vector((0.0, ya + (yb - ya) * t, za + (zb - za) * t)), Vector((0.0, tz / n, -ty / n))
    rows, cols = 12, 8
    scale = 400.0 / lt.SETS['PaintWorn']['size']
    u0, v0 = rnd.uniform(0.0, 2.5), rnd.uniform(0.0, 2.5)
    offset = Vector((rnd.uniform(0, 50), rnd.uniform(0, 50), rnd.uniform(0, 50)))
    middle = (x0 + x1) * 0.5
    grid = []
    for j in range(rows + 1):
        s = total * j / rows
        base, out = along(s)
        row = []
        for i in range(cols + 1):
            u = i / cols
            x = x0 + (x1 - x0) * u + 0.01 * noise.noise(Vector((s * 9.0, u * 3.0, 0.0)) + offset)
            # Below the edge the cloth falls in folds that deepen as it hangs, gathers in a little, and hangs lower at
            # its right corner; on the top it lies crumpled.
            drop = min(max((TOP - 0.004 - base.z) / 0.16, 0.0), 1.0)
            if drop > 0.0:
                x = middle + (x - middle) * (1.0 - 0.1 * drop)
                fold = (0.5 + 0.5 * math.sin(u * math.pi * 4.0 + s * 3.0 + 0.7)) * 0.024 * min(drop * 2.5, 1.0)
                lump = 0.0
            else:
                fold = (0.5 + 0.5 * math.sin(u * math.pi * 2.0 + s * 9.0)) * 0.006
                lump = abs(noise.noise(Vector((x * 12.0, base.y * 12.0, 0.3)) + offset)) * 0.03
            p = base + Vector((x - base.x, 0.0, 0.0)) + out * (fold + lump)
            p.z -= drop * drop * u * 0.035
            if j == rows:
                p.z += 0.015 * noise.noise(Vector((x * 20.0, 0.0, 1.0)) + offset)       # the ragged hem
            row.append((p, out, s))
        grid.append(row)
    bm = lp.new_bmesh()
    uv_layer = bm.loops.layers.uv.active
    # Rows run along the drape (away from the bench's middle, then down) and columns along +X, so a quad in that order
    # faces into the bench: the outer layer is wound the other way round, the inner one (3 mm under it) as it is.
    for layer in (0, 1):
        verts = [[bm.verts.new(p - out * (0.003 * layer)) for p, out, s in row] for row in grid]
        for j in range(rows):
            for i in range(cols):
                quad = [verts[j][i], verts[j][i + 1], verts[j + 1][i + 1], verts[j + 1][i]]
                coords = [grid[j][i], grid[j][i + 1], grid[j + 1][i + 1], grid[j + 1][i]]
                if layer == 0:
                    quad.reverse()
                    coords.reverse()
                face = bm.faces.new(quad)
                for loop, (p, out, s) in zip(face.loops, coords):
                    loop[uv_layer].uv = ((p.x + u0) * scale, (s + v0) * scale)
    part = lp.mesh_object(bm)
    lt.assign(part, PAINT_OXIDE)
    m.add(part, smooth=True)


def shelf_things(m):
    """On the shelf: a crate, an iron cartridge box with a brass hasp, and two rough-sawn stock blanks."""
    rnd = random.Random(111)
    shelf = 0.27
    # The crate (Backlots.py's stacked crates): siding boards on corner posts, battens round the top.
    sx, sy, sz = 0.4, 0.3, 0.22
    frame_matrix = Matrix.Translation((-0.45, 0.02, shelf)) @ Matrix.Rotation(math.radians(4.0), 4, 'Z')
    parts = []
    body = lp.block((sx - 0.02, sy - 0.02, sz - 0.01), (0.0, 0.0, (sz - 0.01) * 0.5))
    lt.assign(body, TRIM)
    lt.trim_uv(body, None, 'Siding', align='world', cut=True, u_offset=rnd.uniform(0.0, 6.4), v_offset=0.05)
    parts.append(body)
    for x in (-1.0, 1.0):
        for y in (-1.0, 1.0):
            post = lp.block((0.045, 0.045, sz), (x * (sx * 0.5 - 0.02), y * (sy * 0.5 - 0.02), sz * 0.5))
            parts.append(lp.grain(post, 'Beams', axis=(0.0, 0.0, 1.0), seed=rnd.randint(0, 999)))
    for y in (-1.0, 1.0):
        bat = lp.block((sx, 0.025, 0.06), (0.0, y * (sy * 0.5 + 0.004), sz - 0.04))
        parts.append(lp.grain(bat, 'Beams', axis=(1.0, 0.0, 0.0), seed=rnd.randint(0, 999)))
    for p in parts:
        p.data.transform(frame_matrix)
        m.add(p)
    # The cartridge box: black iron, a lid lip, a bail handle and a brass hasp.
    bx, by = -0.07, 0.04
    m.add(steel_block((0.28, 0.15, 0.15), (bx, by, shelf + 0.075), seed=1, bevel=0.006))
    m.add(steel_block((0.29, 0.16, 0.03), (bx, by, shelf + 0.15), seed=2, bevel=0.005))
    m.add(rod([(bx - 0.06, by, shelf + 0.165), (bx - 0.05, by, shelf + 0.2), (bx + 0.05, by, shelf + 0.2),
               (bx + 0.06, by, shelf + 0.165)], 0.005, seed=3), smooth=True)
    m.add(steel_block((0.035, 0.008, 0.05), (bx, by - 0.08, shelf + 0.135), mat=BRASS, seed=4))
    # Two stock blanks: rough-sawn walnut outlines, butt to the right, stacked.
    outline = [(0.0, 0.0), (0.36, 0.006), (0.5, 0.0), (0.7, -0.03), (0.7, 0.13), (0.55, 0.078), (0.42, 0.066),
               (0.3, 0.052), (0.0, 0.046)]
    for k in range(2):
        blank = lp.sweep([(0.0, 0.0, 0.0), (0.0, 0.0, 0.042)], [(y, x) for x, y in outline])
        lp.grain(blank, 'Beams', axis=(1.0, 0.0, 0.0), seed=rnd.randint(0, 999))
        lp.place(blank, (-0.35, -0.05, 0.0), (0.0, 0.0, 0.0))
        lp.place(blank, (0.45 + 0.015 * k, -0.02 * k, shelf + 0.042 * k), (0.0, 0.0, (-5.0, 7.0)[k]))
        m.add(blank)


def stains(obj):
    """Oil and grease worked into the top: under and around the vise, where guns lie, round the parts box and the rag."""
    spots = ((-0.7, -0.3, 0.24, 0.85), (0.0, -0.12, 0.45, 0.5), (0.3, 0.05, 0.2, 0.45), (0.68, -0.3, 0.2, 0.6),
             (-0.5, 0.24, 0.12, 0.7))

    def weight(p, n):
        if n.z < 0.8 or abs(p.z - TOP) > 0.008 or abs(p.x) > HX - 0.005 or not FRONT < p.y < BACK:
            return 0.0
        w = 0.0
        for cx, cy, r, s in spots:
            d = math.hypot(p.x - cx, (p.y - cy) * 1.3) / r
            if d < 1.0:
                w = max(w, s * (1.0 - d * d) * (0.55 + 0.45 * noise.noise(Vector((p.x * 9.0, p.y * 9.0, 0.5)))))
        return w
    lr.darken(obj, weight, strength=0.42)


def build():
    m = lr.Model('GunsmithBench', seed=17)
    top_planks(m)
    m.section('top')
    frame(m)
    m.section('frame and shelf')
    backboard(m)
    m.section('backboard')
    vise(m)
    m.section('vise')
    tools(m)
    m.section('tools')
    parts_box(m)
    m.section('parts box')
    oil_can(m, -0.5, 0.24, 121)
    cleaning_rod(m, -0.38, 0.46, 0.31, 122)
    loose_parts(m)
    rag(m, 0.64, 0.88, 123)
    m.section('loose things')
    shelf_things(m)
    m.section('shelf')

    m.socket('Interact', (0.0, FRONT - 0.03, 1.0))
    m.socket('Gun', GUN_AT, (0.0, 90.0, 90.0))
    m.socket('Box', (BOX_AT[0], BOX_AT[1], TOP - SINK + BOX_SIZE[2]), (0.0, 0.0, BOX_YAW))

    m.hull((2.0 * HX, BACK - FRONT, TOP), at=(0.0, (FRONT + BACK) * 0.5, TOP * 0.5))
    m.hull((1.96, 0.2, CAP_TOP - TOP), at=(0.0, 0.34, (TOP + CAP_TOP) * 0.5))
    m.hull((0.25, 0.35, 0.235), at=(VISE_AT[0] + 0.035, FRONT + 0.025, TOP + 0.0375))

    obj = m.finish(ao=False, preview=False, fallback=100.0)
    # Round parts are smooth-shaded; their flat ends and sharp turns stay crisp.
    obj.data.set_sharp_from_angle(angle=math.radians(75.0))
    if '--no-ao' not in kit._args():
        lt.bake_vertex_ao(obj, ground=True, distance=0.6)
        stains(obj)
    return obj


# --- Previews (only with --preview): beside a 1.8 m figure, a stand-in gun lying on SOCKET_Gun ---

def figure():
    """A 1.8 m figure for scale: a plain clay mannequin standing at its origin, facing -Y (TownGate.py's)."""
    bm = bmesh.new()

    def limb(p0, p1, r0, r1, segments=10):
        p0, p1 = Vector(p0), Vector(p1)
        d = p1 - p0
        rot = Vector((0.0, 0.0, 1.0)).rotation_difference(d).to_matrix().to_4x4()
        bmesh.ops.create_cone(bm, cap_ends=True, segments=segments, radius1=r0, radius2=r1, depth=d.length,
                              matrix=Matrix.Translation((p0 + p1) * 0.5) @ rot)

    def blob(c, r, scale=(1.0, 1.0, 1.0)):
        bmesh.ops.create_uvsphere(bm, u_segments=12, v_segments=8, radius=r,
                                  matrix=Matrix.Translation(c) @ Matrix.Diagonal((*scale, 1.0)))
    for sx in (-1.0, 1.0):
        blob((sx * 0.1, -0.05, 0.045), 0.06, (0.95, 2.1, 0.75))
        limb((sx * 0.1, 0.0, 0.06), (sx * 0.1, 0.0, 0.5), 0.05, 0.062)
        limb((sx * 0.1, 0.0, 0.5), (sx * 0.1, 0.0, 0.93), 0.064, 0.088)
        blob((sx * 0.2, 0.0, 1.42), 0.068)
        limb((sx * 0.21, 0.0, 1.42), (sx * 0.24, 0.012, 1.12), 0.05, 0.044)
        limb((sx * 0.24, 0.012, 1.12), (sx * 0.255, -0.02, 0.86), 0.042, 0.034)
        blob((sx * 0.258, -0.022, 0.81), 0.045, (0.8, 0.9, 1.2))
    blob((0.0, 0.0, 0.96), 0.17, (1.12, 0.72, 0.62))
    limb((0.0, 0.0, 0.96), (0.0, 0.0, 1.45), 0.15, 0.19, segments=12)
    limb((0.0, 0.0, 1.46), (0.0, 0.0, 1.58), 0.055, 0.05)
    blob((0.0, -0.01, 1.68), 0.11, (0.9, 1.0, 1.09))
    bmesh.ops.scale(bm, vec=(1.0, 0.68, 1.0), verts=[v for v in bm.verts if 0.94 < v.co.z < 1.47 and abs(v.co.x) < 0.2])
    return _clay_object('_Human', bm, 0x8a847a)


def stand_in_gun():
    """A rough rifle in the socket's own frame (the muzzle along its -Y, its top along +Z, 5 cm thick along X), the
    middle of its bounds at the origin: how a gun lies on SOCKET_Gun."""
    bm = bmesh.new()

    def box(size, at):
        bmesh.ops.create_cube(bm, size=1.0, matrix=Matrix.Translation(at) @ Matrix.Diagonal((*size, 1.0)))
    box((0.05, 0.5, 0.11), (0.0, 0.07, 0.0))             # the receiver and stock
    box((0.03, 0.045, 0.11), (0.0, -0.03, -0.1))          # the grip
    box((0.03, 0.07, 0.15), (0.0, 0.15, -0.12))           # the magazine (behind the grip: a bullpup)
    box((0.036, 0.16, 0.05), (0.0, 0.02, 0.085))          # the sight
    box((0.024, 0.36, 0.024), (0.0, -0.36, 0.02))         # the barrel
    lo = Vector((min(v.co.x for v in bm.verts), min(v.co.y for v in bm.verts), min(v.co.z for v in bm.verts)))
    hi = Vector((max(v.co.x for v in bm.verts), max(v.co.y for v in bm.verts), max(v.co.z for v in bm.verts)))
    bmesh.ops.translate(bm, vec=-(lo + hi) * 0.5, verts=bm.verts)
    return _clay_object('_StandInGun', bm, 0x2c3e5a)


def _clay_object(name, bm, color):
    mesh = bpy.data.meshes.new(name)
    bm.to_mesh(mesh)
    bm.free()
    obj = bpy.data.objects.new(name, mesh)
    bpy.context.scene.collection.objects.link(obj)
    mat = bpy.data.materials.new(name + 'Clay')
    mat.use_nodes = True
    bsdf = next(n for n in mat.node_tree.nodes if n.type == 'BSDF_PRINCIPLED')
    bsdf.inputs['Base Color'].default_value = lt.hex_color(color)
    bsdf.inputs['Roughness'].default_value = 0.8
    mesh.materials.append(mat)
    return obj


def previews(obj):
    scene = bpy.context.scene
    scene.render.engine = 'BLENDER_EEVEE_NEXT'
    scene.eevee.taa_render_samples = 48
    if hasattr(scene.eevee, 'use_shadows'):
        scene.eevee.use_shadows = True
    if hasattr(scene.eevee, 'use_raytracing'):
        scene.eevee.use_raytracing = True
        scene.eevee.ray_tracing_method = 'SCREEN'
    scene.render.film_transparent = False
    scene.render.image_settings.file_format = 'PNG'
    scene.render.image_settings.color_mode = 'RGB'
    scene.view_settings.view_transform = 'AgX'
    scene.view_settings.look = 'AgX - Medium High Contrast'
    scene.world = lt._preview_world(scene)
    for o in scene.objects:
        if o.name.startswith('UCX_'):
            o.hide_render = True
    added = []
    mesh = bpy.data.meshes.new('_Ground')
    mesh.from_pydata([(-60.0, -60.0, 0.0), (60.0, -60.0, 0.0), (60.0, 60.0, 0.0), (-60.0, 60.0, 0.0)], [], [(0, 1, 2, 3)])
    mesh.uv_layers.new(name='UVMap')
    ground = bpy.data.objects.new('_Ground', mesh)
    scene.collection.objects.link(ground)
    lt.assign(ground, lt.material('GroundDirt'))
    lt.box_uv(ground, 'GroundDirt')
    col = mesh.color_attributes.new('Col', 'BYTE_COLOR', 'CORNER')
    col.data.foreach_set('color', [1.0] * (4 * len(mesh.loops)))
    sun = bpy.data.objects.new('_Sun', bpy.data.lights.new('_Sun', 'SUN'))
    scene.collection.objects.link(sun)
    sun.data.energy, sun.data.color, sun.data.angle = 4.2, (1.0, 0.88, 0.72), math.radians(2.0)
    added += [ground, sun]
    person = figure()
    gun = stand_in_gun()
    added += [person, gun]
    bpy.context.view_layer.update()
    socket = next(c for c in obj.children if c.name.startswith('SOCKET_Gun'))
    gun.matrix_world = socket.matrix_world.copy()
    cam = bpy.data.objects.new('_Camera', bpy.data.cameras.new('_Camera'))
    scene.collection.objects.link(cam)
    added.append(cam)
    scene.camera = cam

    def shoot(name, eye, target, lens=35.0, size=(1400, 1000), sun_from=(-0.55, -0.8, 0.75), show_person=False):
        person.hide_render = not show_person
        toward = Vector(sun_from).normalized()
        sun.rotation_euler = (-toward).to_track_quat('-Z', 'Y').to_euler()
        cam.location = Vector(eye)
        cam.rotation_euler = (Vector(target) - Vector(eye)).to_track_quat('-Z', 'Y').to_euler()
        cam.data.lens, cam.data.clip_start, cam.data.clip_end = lens, 0.02, 500.0
        scene.render.resolution_x, scene.render.resolution_y = size
        scene.render.resolution_percentage = 100
        path = os.path.join(PREVIEW_DIR, name + '.png')
        os.makedirs(os.path.dirname(path), exist_ok=True)
        scene.render.filepath = path
        bpy.ops.render.render(write_still=True)
        lt._log(f'preview: {path}')

    person.location = (1.55, -0.55, 0.0)
    person.rotation_euler = (0.0, 0.0, math.radians(-20.0))
    shoot('GunsmithBench', (-1.9, -3.9, 2.0), (0.25, 0.0, 0.95), show_person=True)
    shoot('GunsmithBench_Top', (0.05, -1.2, 1.95), (0.0, 0.02, TOP), lens=32.0)
    shoot('GunsmithBench_Board', (-0.15, -1.25, 1.5), (0.0, 0.36, 1.45), lens=30.0)
    shoot('GunsmithBench_Vise', (-0.35, -0.95, 1.25), (-0.68, -0.4, 0.98), lens=45.0)
    # The parts box as a player standing at the bench sees it (eyes 1.6 m up).
    shoot('GunsmithBench_Box', (BOX_AT[0] - 0.3, BOX_AT[1] - 0.75, 1.6), (BOX_AT[0], BOX_AT[1], TOP), lens=50.0,
          size=(1200, 900))
    shoot('GunsmithBench_Rag', (0.45, -1.05, 1.15), (0.76, -0.38, 0.9), lens=45.0, size=(1200, 900))
    shoot('GunsmithBench_Shelf', (0.35, -1.7, 0.55), (0.0, 0.0, 0.4), lens=32.0)
    shoot('GunsmithBench_Back', (2.6, 3.4, 1.9), (0.0, 0.2, 0.9), sun_from=(0.6, 0.8, 0.7))
    for o in added:
        bpy.data.objects.remove(o)


model = build()
if lt.want_preview():
    previews(model)
