"""The Providence Mining Company's ore lift (Docs/Areas/RansomsRest_Expansion.md, section 4 "Dead Lift", section 6 "The
ore lift"): a single-cable aerial lift from the Old Quarry up Larkspur Ridge's face to the crest. Scripted models
(Art/README.md) on looter_mountain (looter_homes, looter_town, looter_buildings).

Every model is built with x east and y north, the cable running north (+Y) up the ridge, so its front (-Y) faces south
down the line. Every sheave stands WEST of the cable on a cantilever (a "gallows"), because the cage hangs EAST of it:
its hanger leaves the grip sideways and comes down past the sheaves, so it passes the tower and docks at both ends
without touching a timber.

  RR_LiftHouse  the winch house (10 x 8 m) on the quarry floor and, against its north side, the bottom station: a plank
                landing at the house's floor height (0.3 m) round a notch where the cage docks, and the gantry with the
                bottom sheave over it. One room, its north wall open on the landing west of x -0.6. Inside: the vertical
                boiler on its stone base (firebox door, gauge, whistle, the stack through the roof), the winch (a 1.1 m
                drum with the cable wound on it, a gear, the pinion shaft with its flywheel, a small steam engine, a band
                brake and its lever), the coal bin (empty), the speaking tube's mouth, Hiram's stool, lamps; the
                footlocker's place. The drum's cable runs out through the open side up to the bottom sheave.
                Pivot: the middle of the house's footprint on the ground (the floor stands 0.3 m up on fieldstone).
  RR_LiftTower  the one tower on the ridge face: a tapering four-legged timber trestle west of the cable, X-braced,
                with a ladder to a railed platform and a cantilever carrying two saddle sheaves in tandem under the
                cable. Its legs reach 2.5 m below its pivot for the slope. Pivot: the ground under the sheaves' tops,
                on the cable's line; SOCKET_Cable is the top of the saddle (12.0 m up).
  RR_LiftHead   the top station on its granite shoulder: a gallows head frame of heavy timber with the big return
                sheave (1.6 m) the cable goes over and down into a counterweight box hung in a guide frame behind; a
                plank landing round the cage's notch; the bell pull, the speaking tube's mouth and the company's tool box
                (the carbine's place). Not a works. Pivot: the ground under the sheave on the cable's line.
  RR_LiftCage   an open ore cage for one (1.4 x 1.4 m inside): a plank floor in an iron frame, a waist-high rail with
                bars under it on three sides, its gate a chain hooked across the fourth (no collision there), its yoke
                and the hanger arm up to the grip on the cable. Pivot: the floor's middle; its gate faces -Y (Unreal's
                +X). SOCKET_Grip is the grip on the cable, 0.9 m behind the gate's side (+Y) and 4.2 m up: the cage
                hangs plumb east of the cable, its gate facing east, away from the cable, toward both landings, so it
                never turns on the ride. SOCKET_Rider is the floor's middle.
  RR_LiftCable  one metre of the cable (5 cm, IronBlack) along its own -Y (Unreal +X) from its pivot, cut into ten
                rings: a spline mesh (forward axis X) per span, or scaled along X to a span's length.

The code's sockets (Main's contract, 2026-10-13): RR_LiftHouse Cage, BellPull, Winch, SpeakingTube; RR_LiftTower
Sheave; RR_LiftHead Cage, BellPull, SpeakingTube, ToolBox; RR_LiftCage Grip, Rider. A station's SOCKET_Cage is where the
cage's pivot goes when it docks there, turned as the cage is (yaw 90 in the station's frame: the gate east); its floor
is then level with that landing. Where the cable goes: RR_LiftHouse SOCKET_CableEnd (the bottom sheave's top) ->
RR_LiftTower SOCKET_Sheave (the saddle's top) -> RR_LiftHead SOCKET_CableEnd (the return sheave's top, its south side);
the grip rides that line. Turn the house so its +Y from SOCKET_CableEnd points at the tower, and the head so its -Y
points at it. Other sockets: the house's Boiler, Brake, Crank, ClampPin (the bottom sheave), CoalBin, HiramStool,
Footlocker, LandingStep, Smoke, Whistle, Light, Light_2; the head's LandingStep, Light; the tower's Light.
  --preview  renders the lift line on a stand-in ridge, the house outside and in (roof off), the head and the tower into
             Saved/ArtPreviews/RansomsRest/Mountain/.
"""
import math

import bmesh
import bpy
from mathutils import Matrix, Vector

import looter_buildings as kit
import looter_homes as homes
import looter_mountain as mtn
import looter_textures as lt
import looter_town as town
from looter_buildings import Opening, Space, Tile, Trim, place

FLOOR = 0.3                     # the house's floor and the bottom landing over the ground
X0, X1, Y0, Y1 = -5.0, 5.0, -4.0, 4.0
EAVE = FLOOR + 3.4
PITCH = 32.0
LINE_X = -3.4                   # the cable's line through the bottom station (the house's frame)
SHEAVE_Y = 7.2                  # the bottom sheave's middle
SHEAVE_R = 0.45
SHEAVE_TOP = 4.5                # the bottom sheave's top over the ground
CAGE_DROP = 4.2                 # from the grip down to the cage's floor
CAGE_OFF = 0.9                  # the cage's middle east of the cable
CAGE_HALF = 0.7                 # half the cage's width and depth
CAGE_TURN = (0.0, 0.0, 90.0)    # the cage's own frame in the line's: its gate (-Y) faces east, toward both landings
DRUM = (LINE_X, 1.5, FLOOR + 1.0)
DRUM_R = 0.55


def yaw_facing(fx, fy):
    return math.degrees(math.atan2(fx, -fy))


# --- Walls with both faces (the house is entered) ---

def board_wall(m, p0, p1, outline, items, thick=0.05, battens=True):
    """A board-and-batten wall from p0 to p1 (outside on the right walking p0 -> p1), seen from both sides: upright
    boards (the trim sheet's weathered siding turned), battens on the outside, the openings' casings and windows."""
    space = kit.wall_space(p0, p1, FLOOR)
    length = (Vector(p1) - Vector(p0)).length
    m.panel(outline, kit.holes(items), thick, Trim('A', world=True, rotate=True, u=m.rng.uniform(0.0, 6.4)), space=space,
            around=items, slices=0)
    if battens:
        homes.battens(m, space, outline, 0.0, length, homes.keep_out(items), look='A', z0=0.0)
    for o in items:
        if o.kind == 'window':
            town.window(m, space, o, thick)
    return space, length


def frame_post(m, x, y, z0, z1, s=0.2):
    mtn.post(m, x, y, z0, z1, size=s)


# --- The winch house and the bottom station ---

def boiler(m):
    """A vertical boiler on a fieldstone base by the east wall: the shell in IronBlack with riveted bands, the firebox
    door on its west side, a gauge, a safety valve and the whistle on its crown, the stack up through the roof."""
    cx, cy = 3.2, 1.3
    base = FLOOR + 0.45
    kit.stone_stack(m, (cx, cy, FLOOR - 0.05), (1.7, 1.7), 0.5, taper=0.02)
    town.cylinder(m, (cx, cy, base), (cx, cy, base + 2.2), 0.62, sides=16, look='iron', face=(-1.0, 0.0, 0.0))
    town.cylinder(m, (cx, cy, base + 2.2), (cx, cy, base + 2.42), 0.62, 0.32, sides=16, look='iron',
                  face=(-1.0, 0.0, 0.0))
    for z in (base + 0.12, base + 1.1, base + 2.12):
        town.cylinder(m, (cx, cy, z - 0.03), (cx, cy, z + 0.03), 0.635, sides=16, look='iron', caps=(False, False),
                      face=(-1.0, 0.0, 0.0))
    # Firebox door (west), its latch; an ash-pit door under it.
    door = (cx - 0.6, cy, base + 0.42)
    mtn.iron_box(m, (0.08, 0.5, 0.42), at=door, bevel=0.01)
    mtn.iron_box(m, (0.05, 0.12, 0.05), at=(door[0] - 0.06, cy + 0.17, door[2]))
    mtn.iron_box(m, (0.06, 0.44, 0.16), at=(cx - 0.6, cy, base + 0.09))
    # The gauge on a pipe, the safety valve, the whistle; the steam pipe to the engine; the stack.
    town.cylinder(m, (cx - 0.45, cy - 0.3, base + 1.7), (cx - 0.62, cy - 0.42, base + 1.95), 0.02, sides=6, look='brass')
    mtn.ring(m, [(0.0, 0.0), (0.075, 0.0), (0.075, 0.03), (0.0, 0.03)], (cx - 0.64, cy - 0.44, base + 1.98),
             (-0.7, -0.5, 0.0), sides=10, look='brass')
    town.cylinder(m, (cx + 0.25, cy + 0.15, base + 2.38), (cx + 0.25, cy + 0.15, base + 2.6), 0.05, sides=8, look='brass')
    town.cylinder(m, (cx - 0.2, cy - 0.15, base + 2.38), (cx - 0.2, cy - 0.15, base + 2.55), 0.03, sides=8, look='iron')
    town.cylinder(m, (cx - 0.2, cy - 0.15, base + 2.55), (cx - 0.2, cy - 0.15, base + 2.8), 0.045, 0.03, sides=10,
                  look='brass')
    m.socket('Whistle', (cx - 0.2, cy - 0.15, base + 2.85))
    stack_top = 8.1
    town.cylinder(m, (cx, cy, base + 2.4), (cx, cy, stack_top), 0.2, sides=12, look='iron', caps=(False, False),
                  face=(-1.0, 0.0, 0.0))
    town.cylinder(m, (cx, cy, stack_top), (cx, cy, stack_top + 0.35), 0.2, 0.32, sides=12, look='iron', caps=(False, True))
    m.socket('Smoke', (cx, cy, stack_top + 0.4))
    pipe = [(cx - 0.4, cy + 0.3, base + 2.3), (cx - 0.9, cy + 0.3, FLOOR + 2.7), (-0.6, cy + 0.3, FLOOR + 2.7),
            (-0.6, -0.2, FLOOR + 2.7), (-0.6, -0.2, FLOOR + 1.3)]
    for p0, p1 in zip(pipe, pipe[1:]):
        mtn.rod(m, p0, p1, r=0.045, sides=8, caps=(False, False))
    m.socket('Boiler', (door[0] - 0.3, cy, door[2]), (0.0, 0.0, yaw_facing(-1.0, 0.0)))
    m.hull((1.7, 1.7, base + 2.45 - FLOOR + 0.05), at=(cx, cy, (FLOOR - 0.05 + base + 2.45) * 0.5))
    return stack_top


def winch_engine(m):
    """The winch: the drum (axis along X) on two timber bearers, the cable wound on it, a gear on its east end, the
    pinion shaft with its flywheel, a small steam engine turning it, the band brake and its lever, the crank's seat."""
    dx, dy, dz = DRUM
    half = 0.8
    for x in (dx - half - 0.18, dx + half + 0.18):
        mtn.beam(m, (x, dy - 0.9, FLOOR + 0.12), (x, dy + 0.9, FLOOR + 0.12), 0.22, 0.24, face=(1.0, 0.0, 0.0))
        for y in (dy - 0.6, dy + 0.6):
            mtn.beam(m, (x, y, FLOOR + 0.2), (x, dy, dz - 0.1), 0.18, 0.18, face=(1.0, 0.0, 0.0))
        town.box(m, (0.26, 0.4, 0.2), at=(x, dy, dz - 0.05), look='C')
        mtn.iron_box(m, (0.3, 0.22, 0.08), at=(x, dy, dz + 0.08))
    mtn.rod(m, (dx - half - 0.4, dy, dz), (dx + half + 0.45, dy, dz), r=0.05, sides=8)
    town.cylinder(m, (dx - half, dy, dz), (dx + half, dy, dz), DRUM_R, sides=16, look='C', face=(0.0, 0.0, 1.0))
    for x in (dx - half, dx + half):
        mtn.sheave(m, (x, dy, dz), (1.0, 0.0, 0.0), DRUM_R + 0.14, width=0.04, spokes=0, groove=False, hub=0.08)
    town.cylinder(m, (dx - half + 0.06, dy, dz), (dx + half - 0.06, dy, dz), DRUM_R + 0.03, sides=16, look='iron',
                  caps=(False, False), face=(0.0, 0.0, 1.0))
    mtn.sheave(m, (dx + half + 0.32, dy, dz), (1.0, 0.0, 0.0), 0.8, width=0.08, spokes=6, groove=False)
    # The pinion shaft (on the north side), its flywheel and the engine's crank.
    py, pz = dy + 1.15, dz + 0.55
    mtn.rod(m, (dx - half - 0.5, py, pz), (dx + half + 1.6, py, pz), r=0.045, sides=8)
    mtn.sheave(m, (dx + half + 0.32, py, pz), (1.0, 0.0, 0.0), 0.22, width=0.08, spokes=0, groove=False, hub=0.06)
    mtn.sheave(m, (dx + half + 1.25, py, pz), (1.0, 0.0, 0.0), 0.72, width=0.1, spokes=6, groove=False)
    for x in (dx - half - 0.3, dx + half + 0.75):
        mtn.beam(m, (x, py, FLOOR), (x, py, pz - 0.08), 0.2, 0.2, face=(1.0, 0.0, 0.0))
        mtn.iron_box(m, (0.24, 0.2, 0.1), at=(x, py, pz - 0.03))
    ex = dx + half + 1.25
    town.box(m, (0.5, 1.5, 0.5), at=(ex + 0.55, py - 1.0, FLOOR + 0.25), look='C')
    town.cylinder(m, (ex + 0.55, py - 1.55, FLOOR + 0.78), (ex + 0.55, py - 0.75, FLOOR + 0.78), 0.2, sides=12, look='iron',
                  face=(0.0, 0.0, 1.0))
    for y in (py - 1.55, py - 0.75):
        mtn.ring(m, [(0.0, -0.03), (0.24, -0.03), (0.24, 0.03), (0.0, 0.03)], (ex + 0.55, y, FLOOR + 0.78), (0.0, 1.0, 0.0),
                 sides=12)
    mtn.rod(m, (ex + 0.55, py - 0.75, FLOOR + 0.78), (ex + 0.55, py - 0.35, FLOOR + 0.78), r=0.025)
    mtn.rod(m, (ex + 0.55, py - 0.35, FLOOR + 0.78), (ex + 0.12, py, pz + 0.3), r=0.03)
    # The band brake on the drum's west flange and its lever; the crank's square seat at the pinion shaft's west end.
    bx = dx - half - 0.06
    mtn.ring(m, [(DRUM_R + 0.15, -0.035), (DRUM_R + 0.18, -0.035), (DRUM_R + 0.18, 0.035), (DRUM_R + 0.15, 0.035),
                 (DRUM_R + 0.15, -0.035)], (bx, dy, dz), (1.0, 0.0, 0.0), sides=16)
    lever_foot = Vector((bx - 0.2, dy - 0.75, FLOOR))
    lever_top = Vector((bx - 0.2, dy - 0.95, FLOOR + 1.75))
    mtn.beam(m, tuple(lever_foot), tuple(lever_top), 0.08, 0.08, face=(-1.0, 0.0, 0.0))
    mtn.iron_box(m, (0.1, 0.12, 0.3), at=(bx - 0.2, dy - 0.76, FLOOR + 0.15))
    mtn.rod(m, (bx - 0.2, dy - 0.82, FLOOR + 0.6), (bx - 0.05, dy - DRUM_R - 0.16, dz - 0.25), r=0.018)
    m.socket('Brake', tuple(lever_top + Vector((0.0, -0.25, -0.1))), (0.0, 0.0, yaw_facing(0.0, -1.0)))
    seat = (dx - half - 0.52, py, pz)
    mtn.iron_box(m, (0.06, 0.07, 0.07), at=seat)
    m.socket('Crank', (seat[0] - 0.3, py, pz), (0.0, 0.0, yaw_facing(-1.0, 0.0)))
    m.socket('Winch', (dx, dy - DRUM_R - 0.75, FLOOR + 1.0), (0.0, 0.0, yaw_facing(0.0, -1.0)))
    m.hull((2 * half + 0.6, 1.9, dz + 0.75 - FLOOR), at=(dx, dy, (dz + 0.75 + FLOOR) * 0.5))
    m.hull((2 * half + 2.3, 0.5, pz + 0.75 - FLOOR), at=(dx + 0.7, py, (pz + 0.75 + FLOOR) * 0.5))
    m.hull((0.6, 1.6, 1.05), at=(ex + 0.55, py - 1.0, FLOOR + 0.52))


def gantry(m):
    """The bottom station north of the house: the plank landing round the cage's notch on posts, its steps, the gallows
    gantry west of the cable with the bottom sheave on a cantilever, its clamp, and the bell pull."""
    rng = m.rng
    # The landing: planks along X from the house's north wall to y 8.1, but none where the cage docks.
    notch = (LINE_X + CAGE_OFF - CAGE_HALF - 0.06, LINE_X + CAGE_OFF + CAGE_HALF + 0.06,
             SHEAVE_Y + 0.1 - CAGE_HALF - 0.06)
    for y0, y1, xa, xb in ((Y1, notch[2], X0, -0.2), (notch[2], SHEAVE_Y + 0.9, notch[1], -0.2),
                           (notch[2], SHEAVE_Y + 0.9, X0, notch[0])):
        mtn.deck(m, xa, xb, y0, y1, FLOOR)
        m.hull((xb - xa, y1 - y0, 0.3), at=((xa + xb) * 0.5, (y0 + y1) * 0.5, FLOOR - 0.15))
    for x in (X0 + 0.1, -2.4, -0.3):
        for y in (Y1 + 0.2, 6.1, SHEAVE_Y + 0.8):
            mtn.post(m, x, y, -0.5, FLOOR - 0.05, size=0.16)
    for y in (Y1 + 0.2, 6.1, SHEAVE_Y + 0.8):
        mtn.beam(m, (X0, y, FLOOR - 0.12), (-0.2, y, FLOOR - 0.12), 0.14, 0.12, face=(0.0, 0.0, -1.0))
    # A step down off the landing's east side (x = -0.2), toward +X.
    town.box(m, (0.32, 1.2, 0.05), at=(0.0, 5.5, FLOOR * 0.5), look='planks')
    m.hull((0.34, 1.2, FLOOR * 0.5), at=(0.0, 5.5, FLOOR * 0.25))
    # A railing on the landing's west edge and round the notch's back.
    for (ax, ay), (bx, by) in (((X0 + 0.05, Y1 + 0.1), (X0 + 0.05, SHEAVE_Y + 0.85)),):
        homes.railing(m, (ax, ay, FLOOR), (bx, by, FLOOR), height=1.0, every=0.3)
        mtn.hull_between(m, (ax, ay, FLOOR), (bx, by, FLOOR), 0.12, 1.05)
    # The gallows: two posts west of the cable, a cap along Y, the cantilever east to the sheave's bearing, braces.
    gx = LINE_X - 1.15
    top = SHEAVE_TOP + 0.75
    for y in (SHEAVE_Y - 0.65, SHEAVE_Y + 0.65):
        mtn.post(m, gx, y, -0.6, top, size=0.3)
        m.hull((0.3, 0.3, top + 0.6), at=(gx, y, (top - 0.6) * 0.5))
        mtn.beam(m, (gx - 1.4, y, -0.1), (gx - 0.1, y, 2.4), 0.2, 0.2, face=(0.0, y - SHEAVE_Y, 0.0))
    mtn.beam(m, (gx, SHEAVE_Y - 1.0, top + 0.15), (gx, SHEAVE_Y + 1.0, top + 0.15), 0.3, 0.32, face=(-1.0, 0.0, 0.0))
    arm_z = SHEAVE_TOP - SHEAVE_R - 0.35
    mtn.beam(m, (gx - 0.15, SHEAVE_Y, arm_z), (LINE_X - 0.15, SHEAVE_Y, arm_z), 0.32, 0.36)
    mtn.beam(m, (gx + 0.1, SHEAVE_Y, arm_z - 1.2), (gx + 0.85, SHEAVE_Y, arm_z - 0.12), 0.18, 0.2)
    mtn.beam(m, (gx + 0.12, SHEAVE_Y, top + 0.0), (LINE_X - 0.25, SHEAVE_Y, arm_z + 0.2), 0.16, 0.2)
    center = Vector((LINE_X, SHEAVE_Y, SHEAVE_TOP - SHEAVE_R))
    town.box(m, (0.22, 0.42, 0.34), at=(LINE_X - 0.2, SHEAVE_Y, center.z - 0.02), look='iron')
    mtn.rod(m, (LINE_X - 0.32, SHEAVE_Y, center.z), (LINE_X + 0.1, SHEAVE_Y, center.z), r=0.04)
    town.box(m, (0.08, 0.36, 0.4), at=(LINE_X - 0.2, SHEAVE_Y, arm_z + 0.3), look='iron')
    mtn.sheave(m, tuple(center), (1.0, 0.0, 0.0), SHEAVE_R, width=0.12, spokes=6)
    # The clamp on the bearing and its pin's hole (the pin is the mission's).
    mtn.iron_box(m, (0.06, 0.16, 0.16), at=(LINE_X - 0.33, SHEAVE_Y, center.z + 0.12))
    m.socket('ClampPin', (LINE_X - 0.62, SHEAVE_Y, center.z + 0.12), (0.0, 0.0, yaw_facing(-1.0, 0.0)))
    m.hull((1.5, 0.4, 0.4), at=((gx + LINE_X) * 0.5, SHEAVE_Y, arm_z))
    m.socket('CableEnd', (LINE_X, SHEAVE_Y, SHEAVE_TOP))
    m.socket('Cage', (LINE_X + CAGE_OFF, SHEAVE_Y + 0.1, SHEAVE_TOP + 0.02 - CAGE_DROP), CAGE_TURN)
    m.socket('LandingStep', (notch[1] + 0.45, SHEAVE_Y + 0.1, FLOOR), (0.0, 0.0, yaw_facing(-1.0, 0.0)))
    # The bell on a bracket off the south post, its pull rope down to a handle.
    by = SHEAVE_Y - 0.65
    mtn.beam(m, (gx, by, 2.8), (gx + 0.55, by - 0.3, 2.8), 0.1, 0.1, face=(0.0, 0.0, 1.0))
    mtn.bell(m, (gx + 0.5, by - 0.28, 2.74), radius=0.12, rope=1.35)
    m.socket('BellPull', (gx + 0.77, by - 0.28, 1.25), (0.0, 0.0, yaw_facing(0.0, -1.0)))


def lift_house():
    m = mtn.Model('RR_LiftHouse', seed=6103)
    rng = m.rng
    homes.plinth(m, X0, X1, Y0, Y1, FLOOR, out=0.06)
    # The floor: planks along X on the plinth.
    mtn.deck(m, X0, X1, Y0, Y1, FLOOR)
    m.hull((X1 - X0, Y1 - Y0, FLOOR + 0.35), at=(0.0, 0.0, (FLOOR - 0.35) * 0.5))
    m.section('floor')
    wall_h = EAVE - FLOOR
    rise = (Y1 - Y0) * 0.5 * math.tan(math.radians(PITCH))
    DOOR = Opening('door', 3.7, 0.0, 1.1, 2.1)
    south = [DOOR, Opening('window', 1.0, 1.15, 0.9, 1.0, panes=(2, 2)),
             Opening('window', 6.3, 1.15, 0.9, 1.0, panes=(2, 2))]
    s_out = [(0.0, 0.0), (DOOR.x, 0.0), (DOOR.x, DOOR.h), (DOOR.x + DOOR.w, DOOR.h), (DOOR.x + DOOR.w, 0.0),
             (10.0, 0.0), (10.0, wall_h), (0.0, wall_h)]
    board_wall(m, (X0, Y0), (X1, Y0), s_out, south)
    east = [Opening('window', 3.4, 2.0, 1.0, 0.8, panes=(2, 1))]
    board_wall(m, (X1, Y0), (X1, Y1), [(0.0, 0.0), (8.0, 0.0), (8.0, wall_h), (4.0, wall_h + rise), (0.0, wall_h)], east)
    # The north wall: boarded east of the opening (x -0.6), with a window; open over the landing to the west, under a
    # header, the gable's triangle boarded above.
    north = [Opening('window', 2.2, 1.15, 0.9, 1.0, panes=(2, 2))]
    board_wall(m, (X1, Y1), (-0.6, Y1), [(0.0, 0.0), (5.6, 0.0), (5.6, wall_h), (0.0, wall_h)], north)
    head = FLOOR + 2.9
    mtn.beam(m, (X0, Y1 - 0.1, head + 0.15), (-0.6, Y1 - 0.1, head + 0.15), 0.3, 0.22, face=(0.0, 1.0, 0.0))
    over = kit.wall_space((-0.6, Y1), (X0, Y1), head + 0.3)
    m.panel([(0.0, 0.0), (4.4, 0.0), (4.4, EAVE - head - 0.3), (0.0, EAVE - head - 0.3)], (), 0.05,
            Trim('A', world=True, rotate=True), space=over, slices=0)
    board_wall(m, (X0, Y1), (X0, Y0), [(0.0, 0.0), (8.0, 0.0), (8.0, wall_h), (4.0, wall_h + rise), (0.0, wall_h)], [])
    # The frame inside: corner and middle posts, a plate, tie beams at the eaves, a ridge beam and rafters.
    for x in (X0 + 0.1, -2.5, -0.6, 2.5, X1 - 0.1):
        for y in (Y0 + 0.13, Y1 - 0.13):
            frame_post(m, x, y, FLOOR, EAVE)
    for x in (X0 + 0.1, X1 - 0.1):
        frame_post(m, x, 0.0, FLOOR, EAVE + rise - 0.2)
    for x in (-2.5, 0.0, 2.5):
        mtn.beam(m, (x, Y0 + 0.05, EAVE - 0.1), (x, Y1 - 0.05, EAVE - 0.1), 0.2, 0.16, face=(1.0, 0.0, 0.0))
        frame_post(m, x, 0.0, EAVE, EAVE + rise - 0.25, s=0.14)
    mtn.beam(m, (X0, 0.0, EAVE + rise - 0.2), (X1, 0.0, EAVE + rise - 0.2), 0.2, 0.18, face=(0.0, -1.0, 0.0))
    m.section('walls')
    # The roof: shakes on a board deck, a hole round the boiler's stack, rafters under the deck.
    front, back = kit.gable(m, X0, X1, Y0, Y1, EAVE, PITCH, overhang=0.4, rake=0.3, deck=0.06, sag=0.03)
    stack = (3.2, 1.3)
    skip = lambda s: (lambda x, y: (s.world((x, y, 0.0)) - Vector((stack[0], stack[1], s.world((x, y, 0.0)).z))).length < 0.42)
    kit.shingles(m, front, piece=(2.2, 3.4), skip=skip(front))
    kit.shingles(m, back, piece=(2.2, 3.4), skip=skip(back))
    kit.ridge_cap(m, front, back, uv='C', width=0.2)
    for slope in (front, back):
        x = 0.5
        while x < slope.width - 0.3:
            # Rafters from the wall plate up (their sawn ends hidden on the plate and at the ridge beam).
            town.board(m, (x, 0.5, -0.06 - 0.06), (x, slope.length - 0.15, -0.06 - 0.06), 0.12, 0.06, look='C',
                       face=(1.0, 0.0, 0.0), space=slope, drop=('-x', '+x'), cuts=0)
            x += 0.9
    homes.gable_hull(m, X0, X1, Y0, Y1, EAVE, EAVE, 0.0, EAVE + rise, overhang=0.4, rake=0.3, pitch_front=PITCH)
    m.section('roof')
    # Walls' collision: the south wall round its door, the east and west walls, the north wall east of the opening.
    t = 0.12
    m.hull((DOOR.x, t, EAVE + 0.4), at=(X0 + DOOR.x * 0.5, Y0, (EAVE + 0.4) * 0.5))
    m.hull((10.0 - DOOR.x - DOOR.w, t, EAVE + 0.4), at=(X0 + DOOR.x + DOOR.w + (10.0 - DOOR.x - DOOR.w) * 0.5, Y0,
                                                        (EAVE + 0.4) * 0.5))
    m.hull((DOOR.w, t, EAVE - FLOOR - DOOR.h), at=(X0 + DOOR.x + DOOR.w * 0.5, Y0, (EAVE + FLOOR + DOOR.h) * 0.5))
    for x in (X0, X1):
        m.hull((t, 8.0, EAVE + 0.4), at=(x, 0.0, (EAVE + 0.4) * 0.5))
    m.hull((5.6, t, EAVE + 0.4), at=(2.2, Y1, (EAVE + 0.4) * 0.5))
    m.hull((4.4, t, EAVE - head), at=(-2.8, Y1, (EAVE + head) * 0.5))
    # The door stands open against the wall inside.
    hinge = Vector((X0 + DOOR.x + DOOR.w, Y0 + 0.1, FLOOR))
    for k in range(5):
        x = hinge.x + 0.11 + 0.22 * k
        mtn.plank(m, (x, hinge.y + 0.06, FLOOR + 0.02), (x, hinge.y + 0.06, FLOOR + DOOR.h - 0.03), 0.21, 0.04,
                  face=(0.0, 1.0, 0.0), look='A')
    for z in (0.35, DOOR.h - 0.4):
        mtn.plank(m, (hinge.x + 0.05, hinge.y + 0.11, FLOOR + z), (hinge.x + 1.05, hinge.y + 0.11, FLOOR + z), 0.14,
                  0.03, face=(0.0, 1.0, 0.0), look='A')
    town.box(m, (DOOR.w + 0.1, 0.4, 0.15), at=(X0 + DOOR.x + DOOR.w * 0.5, Y0 - 0.25, FLOOR * 0.5), look='D')
    m.hull((DOOR.w + 0.1, 0.4, FLOOR * 0.5 + 0.05), at=(X0 + DOOR.x + DOOR.w * 0.5, Y0 - 0.25, FLOOR * 0.25))
    m.section('door')
    stack_top = boiler(m)
    m.section('boiler')
    winch_engine(m)
    m.section('winch')
    # The drum's cable out through the open side and up to the bottom sheave's south side.
    dx, dy, dz = DRUM
    tangent = Vector((LINE_X, SHEAVE_Y - SHEAVE_R * 0.42, SHEAVE_TOP - SHEAVE_R * 0.1))
    start = Vector((LINE_X, dy + 0.05, dz + DRUM_R + 0.02))
    mtn.rod(m, tuple(start), tuple(tangent), r=0.025, sides=6, caps=(False, False))
    gantry(m)
    m.section('gantry')
    # The coal bin (empty, a few lumps and the shovel), the speaking tube, Hiram's stool, the lamps.
    bx0, bx1, by0, by1 = 3.85, 4.85, -3.75, -1.95
    for (ax, ay), (cx, cy) in (((bx0, by0), (bx0, by1)), ((bx0, by1), (bx1, by1)), ((bx0, by0), (bx1, by0))):
        length = math.hypot(cx - ax, cy - ay)
        for k in range(4):
            z = FLOOR + 0.12 + 0.22 * k
            mtn.plank(m, (ax, ay, z), (cx, cy, z), 0.2, 0.04, face=(-(cy - ay), cx - ax, 0.0), look='planks')
    for k in range(6):
        s = rng.uniform(0.07, 0.13)
        town.box(m, (s, s * 0.8, s * 0.6), at=(rng.uniform(bx0 + 0.2, bx1 - 0.15), rng.uniform(by0 + 0.2, by1 - 0.2),
                                               FLOOR + s * 0.3), rot=(0, 0, rng.uniform(0, 90)), look='charcoal')
    mtn.log(m, (bx0 - 0.15, by1 + 0.05, FLOOR), (bx0 - 0.05, by1 + 0.15, FLOOR + 1.05), r=0.02, sides=6)
    mtn.iron_box(m, (0.26, 0.04, 0.3), at=(bx0 - 0.16, by1 + 0.04, FLOOR + 0.12), rot=(0.0, -6.0, 0.0))
    m.hull((bx1 - bx0, by1 - by0, 0.95), at=((bx0 + bx1) * 0.5, (by0 + by1) * 0.5, FLOOR + 0.47))
    m.socket('CoalBin', (bx0 - 0.45, (by0 + by1) * 0.5, FLOOR), (0.0, 0.0, yaw_facing(-1.0, 0.0)))
    mouth = mtn.tube_mouth(m, (0.4, Y1 - 0.2, FLOOR), facing=(0.0, -1.0, 0.0), rise=1.4)
    town.cylinder(m, (0.4, Y1 - 0.2, FLOOR + 1.4), (0.4, Y1 - 0.2, EAVE + 1.0), 0.035, sides=8, look='iron',
                  caps=(False, False), face=(0.0, -1.0, 0.0))
    m.socket('SpeakingTube', tuple(mouth + Vector((0.0, -0.25, -0.05))), (0.0, 0.0, yaw_facing(0.0, -1.0)))
    sx, sy = 2.1, 0.0
    town.lathe(m, [(0.0, 0.47), (0.17, 0.47), (0.17, 0.5), (0.0, 0.5)], at=(sx, sy, FLOOR), sides=10, look='C')
    for k in range(3):
        a = math.radians(30.0 + 120.0 * k)
        mtn.log(m, (sx + 0.2 * math.cos(a), sy + 0.2 * math.sin(a), FLOOR), (sx + 0.1 * math.cos(a), sy + 0.1 * math.sin(a),
                                                                            FLOOR + 0.47), r=0.022, sides=5)
    m.socket('HiramStool', (sx, sy, FLOOR + 0.5), (0.0, 0.0, yaw_facing(-0.6, -1.0)))
    m.socket('Footlocker', (-3.6, Y0 + 0.5, FLOOR), (0.0, 0.0, yaw_facing(0.0, 1.0)))
    mtn.lantern(m, (0.0, -0.2, EAVE - 0.55), name='Light')
    mtn.rod(m, (0.0, -0.2, EAVE - 0.3), (0.0, -0.2, EAVE - 0.18), r=0.008, sides=4)
    mtn.lantern(m, (LINE_X - 0.6, SHEAVE_Y - 0.65 - 0.2, 2.2), name='Light_2')
    m.section('dressing')
    # The roof deck's world-mapped bands leave a pair of coincident vertices on its eave (a sliver without a tangent
    # frame, tangentcheck's ZERO): weld what coincides before the export's triangulation.
    bmesh.ops.remove_doubles(m.bm, verts=m.bm.verts[:], dist=1e-4)
    obj = m.finish(view=(-0.8, -1.4, 0.6))
    return obj


# --- The tower ---

TOWER_TOP = 12.0


def lift_tower():
    """The tower: a tapering trestle west of the cable (legs at x -0.9..-2.9 at the foot, -1.0..-2.0 at the top; y +-1.6
    to +-0.7), braced, a ladder up its west side to a railed platform, the cantilever and the saddle's two sheaves."""
    m = mtn.Model('RR_LiftTower', seed=6201)
    rng = m.rng
    below = 2.5
    top = TOWER_TOP - 0.95
    feet = {(-1, -1): (-2.9, -1.6), (1, -1): (-0.9, -1.6), (1, 1): (-0.9, 1.6), (-1, 1): (-2.9, 1.6)}
    heads = {(-1, -1): (-2.0, -0.7), (1, -1): (-1.0, -0.7), (1, 1): (-1.0, 0.7), (-1, 1): (-2.0, 0.7)}

    def leg_at(key, z):
        (fx, fy), (hx, hy) = feet[key], heads[key]
        t = (z + below) / (top + below)
        return Vector((fx + (hx - fx) * t, fy + (hy - fy) * t, z))
    for key in feet:
        mtn.beam(m, tuple(leg_at(key, -below)), tuple(leg_at(key, top)), 0.3, 0.3)
        town.box(m, (0.7, 0.7, 0.3), at=tuple(leg_at(key, -below) + Vector((0.0, 0.0, 0.1))), look='D')
    levels = [-below + 0.4, 1.8, 5.2, 8.4, top - 0.1]
    pairs = [((-1, -1), (1, -1)), ((1, -1), (1, 1)), ((1, 1), (-1, 1)), ((-1, 1), (-1, -1))]
    for z0, z1 in zip(levels, levels[1:]):
        for a, b in pairs:
            mtn.beam(m, tuple(leg_at(a, z1)), tuple(leg_at(b, z1)), 0.18, 0.14,
                     face=tuple((leg_at(a, z1) + leg_at(b, z1)) * 0.5 - Vector((-1.5, 0.0, z1))))
            for p, q in ((leg_at(a, z0), leg_at(b, z1)), (leg_at(b, z0), leg_at(a, z1))):
                mtn.beam(m, tuple(p), tuple(q), 0.14, 0.1,
                         face=tuple((leg_at(a, z0) + leg_at(b, z0)) * 0.5 - Vector((-1.5, 0.0, z0))))
    for key in feet:
        p = leg_at(key, -below)
        q = leg_at(key, top)
        m.hull_points([p + Vector((dx, dy, 0.0)) for dx in (-0.16, 0.16) for dy in (-0.16, 0.16)] +
                      [q + Vector((dx, dy, 0.0)) for dx in (-0.16, 0.16) for dy in (-0.16, 0.16)])
    # The platform on the trestle's top, its railing, the ladder up the west side.
    pz = top
    for k in range(9):
        y = -1.0 + 2.0 * (k + 0.5) / 9
        mtn.plank(m, (-2.35, y, pz + 0.025), (-0.6, y, pz + 0.025), 2.0 / 9 - 0.01, 0.05, look='planks')
    m.hull((1.8, 2.0, 0.3), at=(-1.47, 0.0, pz - 0.1))
    for (ax, ay), (bx, by) in (((-2.35, -1.0), (-2.35, 1.0)), ((-2.35, 1.0), (-0.6, 1.0)), ((-0.6, -1.0), (-2.35, -1.0))):
        homes.railing(m, (ax, ay, pz + 0.05), (bx, by, pz + 0.05), height=1.0, every=0.35)
        mtn.hull_between(m, (ax, ay, pz + 0.05), (bx, by, pz + 0.05), 0.1, 1.05)
    lx = -3.05
    for y in (-0.25, 0.25):
        mtn.beam(m, (lx - 0.0, y, -0.3), (lx + 0.62, y, pz + 1.0), 0.08, 0.06, face=(-1.0, 0.0, 0.0))
    rung = 0.0
    while rung < pz + 0.9:
        t = (rung + 0.3) / (pz + 1.3)
        x = lx + 0.62 * t
        mtn.rod(m, (x, -0.25, rung), (x, 0.25, rung), r=0.02, sides=5)
        rung += 0.32
    # The cantilever from the trestle's top east to the cable's line, and the saddle: two sheaves in tandem on a rocker.
    arm_z = TOWER_TOP - 0.62
    for y in (-0.55, 0.55):
        mtn.beam(m, (-2.2, y, arm_z - 0.35), (0.0, y * 0.4, arm_z - 0.1), 0.26, 0.24, face=(0.0, -1.0 if y < 0 else 1.0, 0.0))
        mtn.beam(m, (-1.0, y, top - 1.6), (-0.4, y * 0.5, arm_z - 0.3), 0.16, 0.16, face=(0.0, -1.0 if y < 0 else 1.0, 0.0))
    mtn.iron_box(m, (0.16, 1.4, 0.18), at=(-0.18, 0.0, arm_z))
    for y in (-0.42, 0.42):
        c = Vector((0.0, y, TOWER_TOP - 0.3))
        mtn.sheave(m, tuple(c), (1.0, 0.0, 0.0), 0.3, width=0.1, spokes=5)
        mtn.rod(m, (-0.26, y, c.z), (0.08, y, c.z), r=0.03)
        mtn.iron_box(m, (0.1, 0.16, 0.36), at=(-0.18, y, c.z - 0.12))
    m.hull((2.4, 1.4, 0.6), at=(-1.1, 0.0, arm_z - 0.2))
    m.socket('Sheave', (0.0, 0.0, TOWER_TOP))
    mtn.lantern(m, (-0.7, 0.85, pz + 1.15), name='Light')
    return m.finish(view=(-1.2, -1.0, 0.45))


# --- The head ---

HEAD_R = 0.8
HEAD_C = Vector((0.0, 0.0, 4.4))         # the return sheave's middle
SPAN = 46.0                              # the span's slope arriving from the tower


def lift_head():
    m = mtn.Model('RR_LiftHead', seed=6301)
    rng = m.rng
    s = math.radians(SPAN)
    tangent = HEAD_C + HEAD_R * Vector((0.0, -math.sin(s), math.cos(s)))
    dock = tangent + Vector((0.0, -0.45, -0.45 * math.tan(s)))
    deck = dock.z - CAGE_DROP
    # The gallows: two heavy posts west of the cable either side of the sheave's plane, a cap, the stub axle's bearing
    # on a cross piece, braces down the back (north) and the front; the guide frame and the counterweight behind.
    gx = -0.95
    top = HEAD_C.z + 0.8
    for y in (-0.7, 0.7):
        mtn.post(m, gx, y, -0.5, top, size=0.34)
        m.hull((0.34, 0.34, top + 0.5), at=(gx, y, (top - 0.5) * 0.5))
        mtn.beam(m, (gx - 1.9, y + (1.6 if y > 0 else -0.4), -0.2), (gx - 0.15, y, top - 1.4), 0.24, 0.24,
                 face=(0.0, y, 0.0))
    mtn.beam(m, (gx, -1.0, top + 0.17), (gx, 1.0, top + 0.17), 0.34, 0.36, face=(-1.0, 0.0, 0.0))
    mtn.beam(m, (gx, 0.0, HEAD_C.z - 0.35), (-0.25, 0.0, HEAD_C.z - 0.35), 0.4, 0.5)
    mtn.beam(m, (gx + 0.1, 0.0, HEAD_C.z - 1.6), (-0.4, 0.0, HEAD_C.z - 0.58), 0.2, 0.24)
    town.box(m, (0.26, 0.56, 0.5), at=(-0.25, 0.0, HEAD_C.z - 0.05), look='iron')
    mtn.rod(m, (-0.42, 0.0, HEAD_C.z), (0.12, 0.0, HEAD_C.z), r=0.06)
    mtn.sheave(m, tuple(HEAD_C), (1.0, 0.0, 0.0), HEAD_R, width=0.16, spokes=8)
    m.hull((1.0, 0.6, 0.9), at=(-0.5, 0.0, HEAD_C.z - 0.3))
    # The cable over the top and down the back into the counterweight.
    back = HEAD_C + Vector((0.0, HEAD_R, 0.0))
    box_top = 3.0
    mtn.rod(m, tuple(back), (0.0, HEAD_R, box_top), r=0.025, sides=6, caps=(False, False))
    for y in (HEAD_R - 0.65, HEAD_R + 0.65):
        mtn.post(m, 0.75, y, -0.5, 3.6, size=0.18)
        mtn.post(m, -0.55, y, -0.5, 3.6, size=0.18)
    for z in (3.65,):
        mtn.beam(m, (-0.75, HEAD_R - 0.65, z), (0.95, HEAD_R - 0.65, z), 0.14, 0.14)
        mtn.beam(m, (-0.75, HEAD_R + 0.65, z), (0.95, HEAD_R + 0.65, z), 0.14, 0.14, face=(0.0, 1.0, 0.0))
    for k in range(5):
        z = box_top - 0.15 - 0.22 * k
        for (ax, ay), (bx, by) in (((-0.42, HEAD_R - 0.42), (0.42, HEAD_R - 0.42)), ((0.42, HEAD_R + 0.42), (-0.42, HEAD_R + 0.42)),
                                   ((-0.42, HEAD_R + 0.42), (-0.42, HEAD_R - 0.42)), ((0.42, HEAD_R - 0.42), (0.42, HEAD_R + 0.42))):
            mtn.plank(m, (ax, ay, z), (bx, by, z), 0.2, 0.05, face=(-(by - ay), bx - ax, 0.0), look='planks')
    for k in range(5):
        sz = rng.uniform(0.18, 0.3)
        town.box(m, (sz, sz * 0.8, sz * 0.6), at=(rng.uniform(-0.25, 0.25), HEAD_R + rng.uniform(-0.25, 0.25), box_top - 0.05),
                 rot=(rng.uniform(-20, 20), rng.uniform(-20, 20), rng.uniform(0, 90)), look='granite', bevel=0.02)
    mtn.iron_box(m, (0.9, 0.06, 0.06), at=(0.0, HEAD_R, box_top + 0.03))
    m.hull((1.5, 1.5, 3.6), at=(0.1, HEAD_R, 1.3))
    # The landing round the notch the cage docks in (the notch open to the south, down the line).
    cx0, cx1 = CAGE_OFF - CAGE_HALF - 0.06, CAGE_OFF + CAGE_HALF + 0.06
    cy0, cy1 = dock.y - CAGE_HALF - 0.06, dock.y + CAGE_HALF + 0.06
    parts = [((-0.45, 2.7), (cy1, 2.2)), ((cx1, 2.7), (cy0, cy1))]
    for (xa, xb), (ya, yb) in parts:
        mtn.deck(m, xa, xb, ya, yb, deck)
        m.hull((xb - xa, yb - ya, 0.3), at=((xa + xb) * 0.5, (ya + yb) * 0.5, deck - 0.15))
    for x in (-0.35, 1.3, 2.6):
        for y in (cy1 + 0.1, 2.1):
            mtn.post(m, x, y, -1.6, deck - 0.05, size=0.16)
    for x in (cx1 + 0.1, 2.6):
        mtn.post(m, x, cy0 + 0.1, -2.2, deck - 0.05, size=0.16)
    homes.railing(m, (2.65, cy0 + 0.05, deck), (2.65, 2.1, deck), height=1.0, every=0.32)
    mtn.hull_between(m, (2.65, cy0 + 0.05, deck), (2.65, 2.1, deck), 0.1, 1.05)
    homes.railing(m, (cx1 + 0.05, cy0 + 0.02, deck), (2.65, cy0 + 0.02, deck), height=1.0, every=0.32)
    mtn.hull_between(m, (cx1 + 0.05, cy0 + 0.02, deck), (2.65, cy0 + 0.02, deck), 0.1, 1.05)
    m.socket('Cage', tuple(dock + Vector((CAGE_OFF, 0.0, -CAGE_DROP))), CAGE_TURN)
    m.socket('CableEnd', tuple(tangent))
    m.socket('LandingStep', (cx1 + 0.45, dock.y, deck), (0.0, 0.0, yaw_facing(-1.0, 0.0)))
    # The bell pull on its post, the speaking tube's mouth, the tool box, a lamp.
    mtn.post(m, 2.45, 1.6, -0.5, 3.0, size=0.16)
    mtn.beam(m, (2.45, 1.6, 2.85), (2.0, 1.6, 2.85), 0.1, 0.1, face=(0.0, 0.0, 1.0))
    mtn.bell(m, (2.05, 1.6, 2.79), radius=0.12, rope=1.15)
    m.socket('BellPull', (2.32, 1.6, deck + 0.95), (0.0, 0.0, yaw_facing(-1.0, 0.0)))
    m.hull((0.18, 0.18, 3.5), at=(2.45, 1.6, 1.25))
    mouth = mtn.tube_mouth(m, (-0.35, 2.55, -0.3), facing=(0.0, -1.0, 0.0), rise=deck + 1.7)
    m.socket('SpeakingTube', tuple(mouth + Vector((0.0, -0.25, -0.05))), (0.0, 0.0, yaw_facing(0.0, -1.0)))
    tb = (1.55, 1.75)
    town.box(m, (1.0, 0.5, 0.42), at=(tb[0], tb[1], deck + 0.21), look='C')
    town.box(m, (1.04, 0.54, 0.06), at=(tb[0], tb[1], deck + 0.45), look='woodblack')
    for x in (tb[0] - 0.38, tb[0] + 0.38):
        mtn.iron_box(m, (0.05, 0.56, 0.5), at=(x, tb[1], deck + 0.24))
    mtn.iron_box(m, (0.08, 0.03, 0.1), at=(tb[0], tb[1] - 0.28, deck + 0.38))
    town.text(m, 'P.M.CO.', (tb[0], tb[1] - 0.252, deck + 0.22), height=0.1, look='cream', lift=0.0)
    m.hull((1.04, 0.54, 0.48), at=(tb[0], tb[1], deck + 0.24))
    m.socket('ToolBox', (tb[0], tb[1] - 0.6, deck), (0.0, 0.0, yaw_facing(0.0, -1.0)))
    mtn.lantern(m, (gx + 0.35, -0.75, 2.4), name='Light')
    return m.finish(view=(-1.0, -1.3, 0.4)), deck


# --- The cage and the cable ---

def lift_cage():
    m = mtn.Model('RR_LiftCage', seed=6401)
    cx = CAGE_OFF
    fl = -CAGE_DROP
    h = CAGE_HALF
    # The grip on the cable, its two rollers; the hanger arm out east and down to the yoke.
    mtn.iron_box(m, (0.1, 0.32, 0.1), at=(0.0, 0.0, 0.06), bevel=0.01)
    for y in (-0.12, 0.12):
        mtn.sheave(m, (0.0, y, 0.04), (1.0, 0.0, 0.0), 0.06, width=0.05, spokes=0, hub=0.02)
    path = [(0.0, 0.0, 0.11), (0.38, 0.0, 0.11), (0.45, 0.0, 0.0), (0.45, 0.0, -1.5), (cx, 0.0, fl + 2.45)]
    for p0, p1 in zip(path, path[1:]):
        m.board(p0, p1, 0.09, 0.05, face=(0.0, -1.0, 0.0), uv=Tile('MetalWorn'), mat='iron')
    yoke = fl + 2.4
    mtn.iron_box(m, (2 * h, 0.08, 0.08), at=(cx, 0.0, yoke))
    for sx in (-1.0, 1.0):
        for sy in (-1.0, 1.0):
            mtn.rod(m, (cx + sx * (h - 0.05), 0.0, yoke), (cx + sx * (h - 0.04), sy * (h - 0.04), fl + 2.22), r=0.012)
    # The cage: an iron frame, a plank floor, corner posts, a top frame; a waist rail with bars below it on its west,
    # north and south sides; its gate (east, away from the cable, toward both landings) a chain across the opening.
    mtn.deck(m, cx - h, cx + h, -h, h, fl)
    for side in ('west', 'north', 'south'):
        if side == 'west':
            p0, p1 = Vector((cx - h, -h, 0.0)), Vector((cx - h, h, 0.0))
        else:
            sy = 1.0 if side == 'north' else -1.0
            p0, p1 = Vector((cx - h, sy * h, 0.0)), Vector((cx + h, sy * h, 0.0))
        mid = (p0 + p1) * 0.5
        size = (0.06, 2 * h, 0.0) if side == 'west' else (2 * h, 0.06, 0.0)
        for z, t in ((fl - 0.04, 0.08), (fl + 1.05, 0.05), (fl + 2.22, 0.05)):
            mtn.iron_box(m, (size[0] if side == 'west' else 2 * h, size[1] if side == 'west' else 0.05, t),
                         at=(mid.x, mid.y, z))
        for k in range(1, 7):
            p = p0.lerp(p1, k / 7.0)
            mtn.rod(m, (p.x, p.y, fl), (p.x, p.y, fl + 1.05), r=0.011, sides=5)
        m.hull((max(abs(p1.x - p0.x), 0.06), max(abs(p1.y - p0.y), 0.06), 1.1), at=(mid.x, mid.y, fl + 0.55))
    for z, t in ((fl - 0.04, 0.08), (fl + 2.22, 0.05)):
        mtn.iron_box(m, (0.06, 2 * h, t), at=(cx + h, 0.0, z))
    for sx in (-1.0, 1.0):
        for sy in (-1.0, 1.0):
            mtn.iron_box(m, (0.05, 0.05, 2.26), at=(cx + sx * h, sy * h, fl + 1.1))
    for k in range(9):                          # the gate's chain, hooked across the opening at waist height
        y = -h + 0.08 + (2 * h - 0.16) * k / 8
        z = fl + 1.0 - 0.1 * math.sin(math.pi * k / 8)
        mtn.iron_box(m, (0.02, 0.14, 0.035), at=(cx + h + 0.01, y, z), rot=(30.0 * (1 if k % 2 else -1), 0.0, 0.0))
    m.hull((2 * h, 2 * h, 0.1), at=(cx, 0.0, fl - 0.05))
    # Pivot on the floor's middle, the gate toward -Y (Unreal's +X): the cable's frame turned a quarter.
    mtn.transform_model(m, Matrix.Rotation(math.radians(-90.0), 4, 'Z') @ Matrix.Translation((-cx, 0.0, -fl)))
    m.socket('Grip', (0.0, cx, -fl))
    m.socket('Rider', (0.0, 0.0, 0.0))
    return m.finish(Nanite=0, LODs='50,25')


def lift_cable():
    """One metre of cable along -Y from the pivot (Unreal +X): 8 sides, 10 rings, U round it, V along it."""
    m = mtn.Model('RR_LiftCable', seed=6501)
    tb = kit._new_bmesh()
    uv = tb.loops.layers.uv.active
    r, sides, rings = 0.025, 8, 10
    rows = [[tb.verts.new((r * math.cos(2 * math.pi * k / sides), -1.0 * i / rings, r * math.sin(2 * math.pi * k / sides)))
             for k in range(sides + 1)] for i in range(rings + 1)]
    for i in range(rings):
        for k in range(sides):
            f = tb.faces.new((rows[i][k], rows[i + 1][k], rows[i + 1][k + 1], rows[i][k + 1]))
            for loop in f.loops:
                ii = next(j for j, row in enumerate(rows) if loop.vert in row)
                kk = rows[ii].index(loop.vert)
                loop[uv].uv = (kk / sides * 0.05, ii / rings * 0.32)
    tb.normal_update()
    if sum((f.normal.x * f.calc_center_median().x + f.normal.z * f.calc_center_median().z) for f in tb.faces) < 0:
        bmesh.ops.reverse_faces(tb, faces=tb.faces[:])
    m.emit(tb, None, 'iron')
    obj = m.finish(Nanite=0, Collision='None', ao=False)
    for p in obj.data.polygons:
        p.use_smooth = True
    return obj


house = lift_house()
tower = lift_tower()
head, head_deck = lift_head()
cage = lift_cage()
cable = lift_cable()
print(f'ORELIFT: head landing at {head_deck:.3f} m; bottom landing {FLOOR} m; grip to cage floor {CAGE_DROP} m',
      flush=True)


def socket_world(obj, name):
    s = next(c for c in obj.children if c.name.startswith('SOCKET_' + name))
    return obj.matrix_world @ s.location


if lt.want_preview():
    scene, sun = mtn.stage()
    human = homes.figure()
    # The house outside and in (its roof and the walls' tops cut away at the eaves).
    human.location = (1.5, -5.2, 0.0)
    mtn.show_only([house, human])
    mtn.shot(scene, sun, [house], 'LiftHouse', view=(-0.75, -1.0, 0.42), sun_dir=(30.0, 20.0), figure=human)
    cut = mtn.cut_copy(house, z=EAVE - 0.4)
    mtn.show_only([cut, human])
    human.location = (1.6, -1.6, FLOOR)
    mtn.shot(scene, sun, [cut], 'LiftHouse_Inside', view=(0.35, -0.75, 1.0), sun_dir=(20.0, 55.0), figure=human, lens=30.0)
    bpy.data.objects.remove(cut)
    # The head with the cage docked; the tower; the whole line on a stand-in ridge.
    quarter = Matrix.Rotation(math.radians(CAGE_TURN[2]), 4, 'Z')

    def hang(grip):
        """The cage's matrix with its grip on the cable at grip (the line running north)."""
        return Matrix.Translation(Vector(grip) + Vector((CAGE_OFF, 0.0, -CAGE_DROP))) @ quarter
    cage.matrix_world = head.matrix_world @ Matrix.Translation(socket_world(head, 'Cage')) @ quarter
    human.location = (2.2, -0.3, head_deck)
    mtn.show_only([head, cage, human])
    bpy.context.view_layer.update()
    mtn.shot(scene, sun, [head, cage], 'LiftHead', view=(1.0, -1.2, 0.35), sun_dir=(-30.0, 22.0), figure=human)
    human.location = (-4.0, -2.5, 0.0)
    cage.matrix_world = hang((0.0, -3.0, TOWER_TOP - 2.6 + CAGE_DROP - 1.6))
    mtn.show_only([tower, cage, human])
    bpy.context.view_layer.update()
    mtn.shot(scene, sun, [tower, cage], 'LiftTower', view=(0.9, -1.0, 0.25), sun_dir=(-40.0, 20.0), figure=human)
    # The line: the house at (28, 110, 1), the tower at (23, 126) with its saddle at +30, the head at (27, 146, 47).
    ridge = bpy.data.meshes.new('_Ridge')
    pts = [(-30.0, 95.0, 1.0), (60.0, 95.0, 1.0), (60.0, 112.0, 1.0), (-30.0, 112.0, 1.0), (60.0, 120.0, 8.0),
           (-30.0, 120.0, 8.0), (60.0, 132.0, 30.0), (-30.0, 132.0, 30.0), (60.0, 143.0, 46.0), (-30.0, 143.0, 46.0),
           (60.0, 170.0, 47.0), (-30.0, 170.0, 47.0)]
    ridge.from_pydata(pts, [], [(0, 1, 2, 3), (3, 2, 4, 5), (5, 4, 6, 7), (7, 6, 8, 9), (9, 8, 10, 11)])
    ridge_obj = bpy.data.objects.new('_Ridge', ridge)
    scene.collection.objects.link(ridge_obj)
    ridge.materials.append(mtn.EXTRA['granite']())
    lt.box_uv(ridge_obj, 'RockGranite', texel_density=40)
    tower_at = Vector((23.0, 126.0, 30.0 - TOWER_TOP))
    house.location = (28.0, 110.0, 1.0)
    bpy.context.view_layer.update()
    end = socket_world(house, 'CableEnd')
    turn = math.atan2(-(tower_at.x - end.x), tower_at.y - end.y)
    house.rotation_euler = (0.0, 0.0, turn)
    bpy.context.view_layer.update()
    end = socket_world(house, 'CableEnd')
    tower.location = tower_at
    head.location = (27.0, 146.0, 47.0)
    bpy.context.view_layer.update()
    top_end = socket_world(head, 'CableEnd')
    head.rotation_euler = (0.0, 0.0, math.atan2(-(top_end.x - tower_at.x), top_end.y - tower_at.y))
    bpy.context.view_layer.update()
    spans = []
    a, b, c = end, socket_world(tower, 'Sheave'), socket_world(head, 'CableEnd')
    for p, q in ((a, b), (b, c)):
        span = bpy.data.objects.new('_Span', cable.data)
        scene.collection.objects.link(span)
        d = q - p
        span.matrix_world = (Matrix.Translation(p) @ (Vector((0.0, -1.0, 0.0)).rotation_difference(d)).to_matrix().to_4x4()
                             @ Matrix.Diagonal((3.0, d.length, 3.0, 1.0)))
        spans.append(span)
    cage.matrix_world = hang(b.lerp(c, 0.45))
    human.location = (30.0, 102.0, 1.0)
    for o in scene.objects:
        if o.name == '_Ground':
            o.hide_render = True
    mtn.show_only([house, tower, head, cage, ridge_obj, human] + spans)
    bpy.context.view_layer.update()
    mtn.shot(scene, sun, [house, tower, head, cage], 'LiftLine', view=(1.0, -0.75, 0.32), sun_dir=(-35.0, 18.0),
             lens=35.0, margin=1.04, height_bias=0.5)
    for o in [ridge_obj] + spans:
        bpy.data.objects.remove(o)
    for o in (house, tower, head, cage):
        o.location = (0.0, 0.0, 0.0)
        o.rotation_euler = (0.0, 0.0, 0.0)
        o.matrix_world = Matrix.Identity(4)
mtn.save_reports('OreLift')
