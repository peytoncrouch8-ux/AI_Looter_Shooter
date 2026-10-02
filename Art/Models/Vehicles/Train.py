"""The train for Ransom's Rest (Docs/Areas/RansomsRest.md: The train; Stations and the train; Track and train
collision): a small frontier railway in stylized realism, every body put together from the same parts kit
(Tools/Blender/looter_train.py) in the shared materials. A scripted model file (Art/README.md).

  Locomotive_B          the game's locomotive: a Western 2-6-2T, 8.8 m over the couplers: balloon stack, bell, sand dome,
                        box headlamp, cowcatcher, cordwood
  Locomotive_A          the option not picked (0-6-2T, straight stack, coal bunker, oil headlamp): built for previews only,
                        never exported
  PassengerCar          12.3 m clerestory coach in faded teal and cream, end platforms with railings and steps
  HearseCar             Tilly's funeral car, 10.3 m: black and brass, etched windows onto a coffin rack, crepe, brass lamps
  HearseCarDoor         its side door; origin on the hinge (SOCKET_Door): opens 180 degrees outward, flat on the side
  TrainWheelSet_Driver  a 1.04 m spoked driver pair with crank pins (SOCKET_Axle_N on the locomotives)
  TrainWheelSet_Carriage  a 0.84 m spoked pair with outside journals (bogies, leading and trailing axles)
  TrainCouplingRod      the drivers' coupling rod, one mesh for both sides (SOCKET_Rod_L, SOCKET_Rod_R)

Every body: SOCKET_Axle_1.. front to back, SOCKET_Coupler_Front / _Back on the coupler planes (bodies couple where
these meet), one box hull from the ground to the roof reaching the coupler planes, Nanite off with three LODs.
Locomotives: SOCKET_Smoke (stack top), SOCKET_Light (headlamp glass), SOCKET_Firebox (the lit firebox door in the cab),
SOCKET_Whistle, and SOCKET_Bell on B. Hearse car: SOCKET_Door, SOCKET_Arrival (on the platform at the door),
SOCKET_Light_1..4 (its four lamps). Lit glass is the LanternGlow slot, mapped on the trim sheet's dark glass strip, so
swapping that slot for HouseTrim shows unlit lamps while the train is cold.

THE RAIL CONTRACT (with Art/Models/Props/Railway.py and Art/Models/Buildings/Depot.py): ground z = 0, rail heads at
z = 0.25, 1.435 m between the rails' inner faces, every body's origin on the ground under its middle with its front
toward -Y, wheel treads on z = 0.25, the platform 0.40 m high with its edge 1.65 m out (nothing below 0.6 m reaches
past 1.55 m from the centreline).

    blender -b --factory-startup --python Art/Models/Vehicles/Train.py -- --preview
"""
import math
import os

import bpy
from mathutils import Matrix, Vector

import looter_model as lm
import looter_textures as lt
import looter_props as lp
import looter_train as tk

# --- The locomotives (one kit, two looks) ---

FOOT = 1.36                     # footplate top
BOILER_Z, BOILER_R = 2.12, 0.62
SMOKEBOX_R = 0.68
SPACING = 1.45                  # driver axle spacing (the coupling rod's)
DRIVERS = (-2.35, -0.9, 0.55)
TRAIL = 2.6                     # the trailing carriage axle (both)
LEAD = -3.45                    # the leading carriage axle (Locomotive_B)
BEAM = 4.05                     # buffer beams' outer faces at y = -BEAM and +BEAM
CAB_F, CAB_B = 0.5, 2.25        # cab front and back walls (outer faces)
CAB_W = 1.3
EAVE, CROWN = 3.45, 3.72
STACK_Y = -3.38
PILOT_TIP = -5.0                # Locomotive_B's cowcatcher reaches out to here
ROUND, DOOR_ROUND = 32, 36      # segments round the boiler and smokebox, and on the smokebox door (no facets close up)
# Every body's LODs (triangle shares) and the screen sizes they start at: LOD3 is under 1k triangles from about 120 m
# (the locomotives) and 145 m (the cars), so Ransom's Point, 240 m off, sees only it.
BODY_LODS, BODY_SCREENS = '50,25,10', '0.5,0.25,0.08'
DRIVER_Z = tk.RAIL_TOP + tk.DRIVER_R
CARRIAGE_Z = tk.RAIL_TOP + tk.CARRIAGE_R
ROOF_R = (CAB_W ** 2 + (CROWN - EAVE) ** 2) / (2.0 * (CROWN - EAVE))
ROOF_C = CROWN - ROOF_R


def roof_z(x, lift=0.0):
    """The cab roof's underside (lift > 0: a surface above it) over x."""
    return ROOF_C + math.sqrt((ROOF_R + lift) ** 2 - x * x)


def wall_top(x0, x1, steps=6):
    """Points along the roof's underside from x1 back to x0 (a wall's top edge, counterclockwise order)."""
    return [(x1 - (x1 - x0) * i / steps, roof_z(x1 - (x1 - x0) * i / steps)) for i in range(1, steps)]


def mirror(points, face):
    """An outline drawn for the '+x' (or '-y') wall, for the opposite wall: mirrored and still counterclockwise."""
    return points if face in ('+x', '-y') else [(-u, v) for u, v in reversed(points)]


def circle(cx, cz, r, n=16):
    return [(cx + math.cos(2.0 * math.pi * i / n) * r, cz + math.sin(2.0 * math.pi * i / n) * r) for i in range(n)]


def carriage_truck(k, y):
    """Outside frames and journal boxes for a single carriage axle under a locomotive (leading or trailing)."""
    for s in (-1.0, 1.0):
        x = s * 0.99
        k.beam((x, y - 0.62, CARRIAGE_Z + 0.17), (x, y + 0.62, CARRIAGE_Z + 0.17), 0.06, 0.09, 'iron')
        k.box((0.2, 0.26, 0.26), (x + s * 0.02, y, CARRIAGE_Z), 'iron', bevel=0.02)
        k.box((0.09, 0.62, 0.08), (x, y, CARRIAGE_Z + 0.27), 'iron')
        for dy in (-0.58, 0.58):
            k.beam((x, y + dy, CARRIAGE_Z + 0.2), (x * 0.86, y + dy, FOOT - 0.05), 0.05, 0.05, 'iron',
                   up=(0.0, 1.0, 0.0))


def locomotive(name, western):
    """A small tank engine, 8.8 m over the couplers. Locomotive_A: 0-6-2T with a straight stack, a coal bunker, an oil
    headlamp and buffers at both ends. Locomotive_B (western=True): 2-6-2T, the same kit with a flared balloon stack,
    a bell, a sand dome, a big box headlamp, a red slatted cowcatcher with the coupler on it, and cordwood in the bunker."""
    k = tk.Kit(name, 12 if western else 11)
    # Frames, axleboxes, springs and brakes, all black iron.
    for s in (-1.0, 1.0):
        k.box((0.05, 7.7, 0.77), (s * 0.56, 0.0, 0.935), 'iron')
        for i, y in enumerate(DRIVERS):
            k.box((0.12, 0.3, 0.34), (s * 0.6, y, DRIVER_Z), 'iron', bevel=0.015)
            k.box((0.08, 0.95, 0.09), (s * 0.6, y, 1.22), 'iron')
            k.box((0.1, 0.07, 0.34), (s * 0.75, y + 0.585, DRIVER_Z - 0.04), 'iron', rot=(8.0, 0.0, 0.0))
            k.beam((s * 0.62, y + 0.62, FOOT - 0.06), (s * 0.75, y + 0.6, DRIVER_Z + 0.12), 0.04, 0.05, 'iron',
                   up=(0.0, 1.0, 0.0))
    carriage_truck(k, TRAIL)
    if western:
        carriage_truck(k, LEAD)
    k.box((1.0, 0.75, 0.48), (0.0, -3.35, 1.1), 'iron', bevel=0.03)               # the inside cylinders
    # Footplate, valances and buffer beams (oxide-red timber).
    k.box((2.64, 8.1, 0.04), (0.0, 0.0, FOOT - 0.02), 'iron')
    for s in (-1.0, 1.0):
        k.box((0.03, 8.1, 0.14), (s * 1.315, 0.0, FOOT - 0.09), 'iron')
    for d in (-1.0, 1.0):
        k.box((2.64, 0.2, 0.48), (0.0, d * (BEAM - 0.1), 1.12), 'woxide', bevel=0.015)
    tk.end_gear(k, BEAM, 1.0)
    if western:
        tk.end_gear(k, -BEAM, -1.0, buffers=False)
        cowcatcher(k)
    else:
        tk.end_gear(k, -BEAM, -1.0)
    # Smokebox, door, boiler, bands and the firebox's end in the cab.
    k.cyl((0.0, -3.75, BOILER_Z), (0.0, -2.95, BOILER_Z), SMOKEBOX_R, 'iron', sides=ROUND)
    k.lathe([(0.6, 0.0), (0.585, 0.025), (0.48, 0.05), (0.25, 0.07), (0.0, 0.078)], 'iron', (0.0, -3.75, BOILER_Z),
            (0.0, -4.75, BOILER_Z), sides=DOOR_ROUND)
    for dz in (-0.27, 0.27):
        k.box((0.86, 0.03, 0.06), (0.0, -3.81, BOILER_Z + dz), 'iron')
        k.box((0.06, 0.08, 0.1), (-0.5, -3.79, BOILER_Z + dz), 'iron')
    k.cyl((0.0, -3.82, BOILER_Z), (0.0, -3.92, BOILER_Z), 0.03, 'brass', sides=6)
    k.box((0.26, 0.03, 0.03), (0.0, -3.9, BOILER_Z), 'brass', rot=(0.0, 30.0, 0.0))
    k.cyl((0.0, -2.95, BOILER_Z), (0.0, 0.8, BOILER_Z), BOILER_R, 'iron', sides=ROUND, caps=(False, True))
    for y in (-2.94, -2.0, -0.3, 0.38):
        k.cyl((0.0, y, BOILER_Z), (0.0, y + 0.06, BOILER_Z), BOILER_R + 0.012, 'brass', sides=ROUND, caps=(False, False))
    k.box((1.24, 0.32, 0.76), (0.0, 0.64, FOOT + 0.38), 'iron')                       # firebox below the barrel
    # Domes, valves, whistle, stack.
    k.lathe([(0.34, 2.62), (0.31, 2.74), (0.28, 2.8), (0.27, 2.96), (0.25, 3.06), (0.17, 3.14), (0.0, 3.17)], 'brass',
            (0.0, -1.2, 0.0), sides=24)
    k.lathe([(0.13, 2.7), (0.1, 2.8), (0.07, 2.92), (0.09, 3.02), (0.0, 3.04)], 'brass', (0.0, 0.12, 0.0), sides=12)
    k.lathe([(0.03, 2.7), (0.03, 2.95), (0.055, 2.97), (0.055, 3.12), (0.035, 3.14), (0.0, 3.16)], 'brass',
            (0.0, 0.42, 0.0), sides=8)
    k.socket('Whistle', (0.0, 0.42, 3.17))
    if western:
        k.lathe([(0.32, 2.66), (0.26, 2.78), (0.2, 2.86), (0.19, 3.12), (0.25, 3.34), (0.42, 3.6), (0.5, 3.8),
                 (0.5, 3.9), (0.46, 3.96), (0.0, 3.93)], 'iron', (0.0, STACK_Y, 0.0), sides=24)
        k.cyl((0.0, STACK_Y, 3.78), (0.0, STACK_Y, 3.86), 0.506, 'brass', sides=24, caps=(False, False))
        k.lathe([(0.24, 2.68), (0.2, 2.76), (0.17, 2.84), (0.17, 2.92), (0.12, 3.0), (0.0, 3.02)], 'brass',
                (0.0, -2.05, 0.0), sides=16)                                                # the sand dome
        bell(k, (0.0, -2.72, 2.74))
    else:
        k.lathe([(0.3, 2.66), (0.25, 2.78), (0.205, 2.86), (0.2, 3.8), (0.0, 3.8)], 'iron', (0.0, STACK_Y, 0.0), sides=20)
        k.lathe([(0.2, 3.78), (0.24, 3.84), (0.255, 3.94), (0.2, 3.98), (0.17, 3.86), (0.0, 3.86)], 'brass',
                (0.0, STACK_Y, 0.0), sides=20)
    k.socket('Smoke', (0.0, STACK_Y, 3.98))
    # Side tanks, beading, fillers and number plates; sandboxes and their pipes; boiler handrails.
    for s in (-1.0, 1.0):
        k.box((0.52, 2.4, 1.04), (s * 1.04, -1.35, FOOT + 0.52), 'iron', bevel=0.04)
        k.box((0.035, 2.36, 0.035), (s * 1.3, -1.35, FOOT + 1.02), 'brass')
        k.box((0.035, 2.36, 0.035), (s * 1.3, -1.35, FOOT + 0.04), 'brass')
        k.cyl((s * 1.0, -2.25, FOOT + 1.03), (s * 1.0, -2.25, FOOT + 1.1), 0.13, 'brass', sides=10)
        k.box((0.02, 0.66, 0.32), (s * 1.31, -1.35, FOOT + 0.56), 'brass', bevel=0.012)
        k.text('No 4', 0.2, (s * 1.32, -1.35, FOOT + 0.56), (90.0, 0.0, s * 90.0), 'iron', extrude=0.006)
        k.box((0.22, 0.3, 0.3), (s * 1.0, -2.85, FOOT + 0.15), 'iron', bevel=0.02)
        k.tube([(s * 1.0, -2.9, FOOT), (s * 0.96, -2.95, 1.0), (s * 0.76, -2.92, 0.55)], 0.018, 'iron')
        k.tube([(s * 0.7, -3.45, 2.5), (s * 0.7, 0.42, 2.5)], 0.016, 'brass')
        for y in (-3.2, -2.0, -0.8):
            k.tube([(s * 0.5, y, 2.47), (s * 0.7, y, 2.5)], 0.013, 'brass')
    # The cab: weathered boards, oxide-red window frames, a black iron roof.
    cab_walls(k)
    # The backhead: firebox door (lit), gauge glasses, a pressure gauge.
    for dx in (-0.21, 0.21):
        k.box((0.05, 0.04, 0.34), (dx, 0.82, 1.76), 'brass')
    for dz in (-0.15, 0.15):
        k.box((0.47, 0.04, 0.05), (0.0, 0.82, 1.76 + dz), 'brass')
    k.box((0.36, 0.02, 0.24), (0.0, 0.81, 1.76), 'glow')
    k.socket('Firebox', (0.0, 0.84, 1.76), (0.0, 0.0, 180.0))
    for dx in (-0.26, 0.26):
        k.cyl((dx, 0.86, 2.2), (dx, 0.86, 2.5), 0.025, 'brass', sides=6)
    k.lathe([(0.0, 0.0), (0.09, 0.0), (0.09, 0.03), (0.0, 0.035)], 'brass', (0.0, 0.8, 2.62), (0.0, 1.8, 2.62),
            sides=10)
    # Bunker with its load, ladder and the back.
    k.box((2.5, 1.63, 1.09), (0.0, 3.035, FOOT + 0.545), 'iron', bevel=0.03)
    k.box((2.54, 0.035, 0.035), (0.0, 3.85, FOOT + 1.08), 'brass')
    for s in (-1.0, 1.0):                                                             # coal rails round the top
        k.box((0.03, 1.5, 0.16), (s * 1.22, 3.06, FOOT + 1.17), 'iron')
    k.box((2.44, 0.03, 0.16), (0.0, 3.8, FOOT + 1.17), 'iron')
    if western:
        cordwood(k)
    else:
        heap = k.lathe([(0.0, 0.34), (0.45, 0.29), (0.8, 0.17), (1.0, 0.06), (1.06, 0.0)], 'iron', (0.0, 0.0, 0.0),
                       sides=10)
        heap.data.transform(Matrix.Diagonal((1.08, 0.7, 1.0, 1.0)))
        lp.rough(heap, 0.045, 4.0, 5)
        lp.place(heap, (0.0, 3.03, FOOT + 1.06))
    for s in (-1.0, 1.0):
        k.box((0.04, 0.04, 1.0), (s * 0.22, 3.88, FOOT + 0.5), 'iron')
    for z in (FOOT + 0.25, FOOT + 0.55, FOOT + 0.85):
        k.box((0.48, 0.04, 0.035), (0.0, 3.9, z), 'iron')
    # Headlamp.
    if western:
        box_headlamp(k, (0.0, -3.97, SMOKEBOX_R + BOILER_Z - 0.1))
    else:
        tk.oil_lamp(k, (0.0, -3.62, BOILER_Z + SMOKEBOX_R - 0.03), size=0.28, socket='Light')
    # Running gear sockets, front to back: carriage axles (leading, trailing) and drivers.
    axles = ([(LEAD, 'Carriage')] if western else []) + [(y, 'Driver') for y in DRIVERS] + [(TRAIL, 'Carriage')]
    for i, (y, kind) in enumerate(axles):
        k.socket(f'Axle_{i + 1}', (0.0, y, DRIVER_Z if kind == 'Driver' else CARRIAGE_Z))
    k.socket('Rod_L', (tk.ROD_X, DRIVERS[1] - tk.CRANK, DRIVER_Z))
    k.socket('Rod_R', (-tk.ROD_X, DRIVERS[1], DRIVER_Z - tk.CRANK))
    front = -(BEAM + tk.REACH)
    k.socket('Coupler_Front', (0.0, front, tk.COUPLER_Z))
    k.socket('Coupler_Back', (0.0, BEAM + tk.REACH, tk.COUPLER_Z), (0.0, 0.0, 180.0))
    # One box: nobody climbs on, squeezes past or walks under it. A cowcatcher reaches past the coupler.
    y0 = PILOT_TIP - 0.03 if western else front
    y1 = BEAM + tk.REACH
    k.hull((2.8, y1 - y0, 4.0), (0.0, (y0 + y1) * 0.5, 2.0))
    model = k.finish(lods=BODY_LODS, screens=BODY_SCREENS, ao=0.6)
    model['_axles'] = ','.join(kind for _, kind in axles)
    return model


def cab_walls(k):
    t = 0.06
    front = ([(-CAB_W, FOOT), (CAB_W, FOOT), (CAB_W, EAVE)] + wall_top(-CAB_W, CAB_W) + [(-CAB_W, EAVE)])
    windows = [tk.window_outline(x0, 2.52, 0.4, 0.62, arch=0.12) for x0 in (-1.1, 0.7)]
    k.panel([front, circle(0.0, BOILER_Z, BOILER_R + 0.02, ROUND)] + windows, t, 'woxide', tk.frame('-y', (0.0, CAB_F, 0.0)))
    for w in windows:
        tk.window(k, '-y', (0.0, CAB_F, 0.0), w, t, frame_mat='brass', frame_w=0.045)
    side = [(CAB_F + t, FOOT), (0.92, FOOT), (0.92, 2.82), (1.0, 2.93), (1.54, 2.93), (1.62, 2.82), (1.62, FOOT),
            (CAB_B, FOOT), (CAB_B, EAVE), (CAB_F + t, EAVE)]
    side_window = tk.window_outline(1.8, 2.45, 0.32, 0.6, arch=0.1)
    for face, x in (('+x', CAB_W), ('-x', -CAB_W)):
        outline, hole = mirror(side, face), mirror(side_window, face)
        k.panel([outline, hole], t, 'woxide', tk.frame(face, (x, 0.0, 0.0)))
        tk.window(k, face, (x, 0.0, 0.0), hole, t, frame_mat='brass', frame_w=0.04)
        # Black iron corner angles and eave strip on the red cab.
        k.box((0.05, 0.07, EAVE - FOOT), (x, CAB_B - 0.035, (EAVE + FOOT) * 0.5), 'iron')
        k.box((0.05, 0.07, EAVE - FOOT), (x, CAB_F + 0.035, (EAVE + FOOT) * 0.5), 'iron')
        k.box((0.05, CAB_B - CAB_F, 0.07), (x, (CAB_F + CAB_B) * 0.5, EAVE - 0.035), 'iron')
        for y in (0.88, 1.66):
            tk.grab_rail(k, x, y, 1.55, 2.8, 'brass', stand=0.05)
        tk.corner_steps(k, math.copysign(1.36, x), 1.27, FOOT, levels=(1.02, 0.7), width=0.52)
    back = [(-(CAB_W - t), FOOT), (CAB_W - t, FOOT), (CAB_W - t, roof_z(CAB_W - t))] + \
        wall_top(-(CAB_W - t), CAB_W - t) + [(-(CAB_W - t), roof_z(CAB_W - t))]
    back_windows = [tk.window_outline(x0, 2.6, 0.42, 0.55, arch=0.12) for x0 in (-0.8, 0.38)]
    k.panel([back] + back_windows, t, 'woxide', tk.frame('+y', (0.0, CAB_B, 0.0)))
    for w in back_windows:
        tk.window(k, '+y', (0.0, CAB_B, 0.0), w, t, frame_mat='brass', frame_w=0.045, sill=False)
    k.box((2.5, CAB_B - CAB_F - 0.1, 0.04), (0.0, (CAB_F + CAB_B) * 0.5, FOOT + 0.02), 'trim', strip='Beams', axis='y')
    # The roof: an iron shell over the walls, overhanging all round, and a vent.
    xs = [1.4 - 2.8 * i / 10 for i in range(11)]
    profile = [(x, roof_z(x, 0.05)) for x in xs] + [(x, roof_z(x)) for x in reversed(xs)]
    roof = lp.sweep([(0.0, CAB_F - 0.14, 0.0), (0.0, CAB_B + 0.14, 0.0)], profile)
    k.map(roof, 'iron')
    k.box((0.5, 0.6, 0.08), (0.0, (CAB_F + CAB_B) * 0.5, CROWN + 0.08), 'iron', bevel=0.02)


def cowcatcher(k):
    """A slatted pilot under the front beam, raked forward to a point 9 cm over the rails, in oxide-red timber."""
    tip_y, top_y, z_top, z_low = PILOT_TIP, -BEAM - 0.02, 0.9, 0.34

    def bottom(x):
        return -4.32 + (tip_y + 4.32) * (1.0 - abs(x) / 1.24), z_low
    k.beam((-1.26, top_y, z_top - 0.04), (1.26, top_y, z_top - 0.04), 0.12, 0.1, 'woxide', up=(0.0, 1.0, 0.0))
    for a, b in (((-1.24, -4.32), (0.0, tip_y)), ((0.0, tip_y), (1.24, -4.32))):
        k.beam((a[0], a[1], z_low), (b[0], b[1], z_low), 0.08, 0.08, 'woxide')
    for i in range(13):
        x = -1.14 + 2.28 * i / 12
        y_b, z_b = bottom(x)
        k.beam((x, top_y, z_top - 0.06), (x, y_b + 0.02, z_b + 0.03), 0.065, 0.04, 'woxide', up=(0.0, -1.0, 0.0))
    k.beam((0.0, top_y, z_top - 0.06), (0.0, tip_y + 0.03, z_low + 0.04), 0.09, 0.07, 'woxide', up=(0.0, -1.0, 0.0))


def bell(k, at):
    """A brass bell hung in an iron yoke on the boiler (SOCKET_Bell at its mouth)."""
    x, y, z = at
    for s in (-1.0, 1.0):
        k.beam((s * 0.2, y, z - 0.05), (s * 0.2, y, z + 0.5), 0.05, 0.05, 'iron', up=(0.0, 1.0, 0.0))
    k.beam((-0.23, y, z + 0.5), (0.23, y, z + 0.5), 0.06, 0.06, 'iron', up=(0.0, 1.0, 0.0))
    k.lathe([(0.0, 0.0), (0.17, 0.0), (0.16, 0.035), (0.125, 0.12), (0.105, 0.21), (0.065, 0.27), (0.0, 0.285)], 'brass',
            (x, y, z + 0.16), sides=14)
    k.socket('Bell', (x, y, z + 0.18))


def box_headlamp(k, at):
    """The Western headlamp: a big black box lamp on the smokebox, brass-bound, with a round lens (SOCKET_Light)."""
    x, y, z = at
    k.box((0.56, 0.56, 0.05), (x, y, z + 0.025), 'iron')
    for s in (-1.0, 1.0):
        k.beam((x + s * 0.2, -3.78, z - 0.3), (x + s * 0.2, y - 0.2, z), 0.04, 0.03, 'iron', up=(1.0, 0.0, 0.0))
    k.box((0.52, 0.5, 0.5), (x, y, z + 0.3), 'iron', bevel=0.02)
    k.cyl((x - 0.29, y, z + 0.6), (x + 0.29, y, z + 0.6), 0.06, 'iron', sides=4, spin=45.0)
    k.box((0.6, 0.58, 0.04), (x, y, z + 0.57), 'iron')
    k.cyl((x, y, z + 0.6), (x, y, z + 0.82), 0.06, 'iron', sides=8)
    k.cyl((x, y, z + 0.8), (x, y, z + 0.86), 0.1, 'brass', sides=8)
    for dz in (0.07, 0.53):
        k.box((0.54, 0.52, 0.03), (x, y, z + dz), 'brass')
    k.lathe([(0.22, 0.0), (0.225, 0.035), (0.19, 0.055), (0.16, 0.05)], 'brass', (x, y - 0.25, z + 0.3),
            (x, y - 1.25, z + 0.3), sides=16)
    k.lathe([(0.17, 0.0), (0.1, 0.03), (0.0, 0.038)], 'glow', (x, y - 0.25, z + 0.3), (x, y - 1.25, z + 0.3),
            sides=16)
    k.socket('Light', (x, y - 0.3, z + 0.3))


def cordwood(k):
    """Split logs stacked across the bunker (a wood burner's fuel)."""
    rnd = k.rnd
    z = FOOT + 1.09
    for row in range(3):
        count = 7 - row * 2
        for i in range(count):
            y = 3.03 + (i - (count - 1) * 0.5) * 0.19 + rnd.uniform(-0.02, 0.02)
            r = rnd.uniform(0.075, 0.09)
            length = rnd.uniform(2.0, 2.25)
            dx = rnd.uniform(-0.06, 0.06)
            k.cyl((-length * 0.5 + dx, y, z + r), (length * 0.5 + dx, y, z + r), r, 'trim', sides=6, strip='Logs',
                  grain='up', spin=rnd.uniform(0.0, 60.0))
        z += 0.15


# --- The cars (one kit: underframe, bogies, end platforms, roof) ---

CAR_FLOOR = 1.26                # floor top (end platforms too)
CAR_W = 1.4                     # side walls' outer faces
CAR_EAVE = 3.4
WALL = 0.06
HOOD = [(1.48, 3.4), (1.25, 3.53), (0.95, 3.63), (0.62, 3.7), (0.3, 3.74), (0.0, 3.75)]
CLERE_X, CLERE_TOP = 0.62, 3.98
UPPER = [(0.72, 3.98), (0.5, 4.06), (0.25, 4.11), (0.0, 4.125)]


def hood_z(x):
    x = abs(x)
    for (x0, z0), (x1, z1) in zip(HOOD, HOOD[1:]):
        if x1 <= x <= x0:
            return z0 + (z1 - z0) * (x0 - x) / (x0 - x1)
    return HOOD[-1][1] if x < HOOD[-1][0] else HOOD[0][1]


def upper_z(x):
    x = abs(x)
    for (x0, z0), (x1, z1) in zip(UPPER, UPPER[1:]):
        if x1 <= x <= x0:
            return z0 + (z1 - z0) * (x0 - x) / (x0 - x1)
    return UPPER[-1][1]


def closed_profile(half, thick):
    """A roof shell's cross-section (x, z), outer surface across then inner surface back."""
    outer = [(x, z) for x, z in half] + [(-x, z) for x, z in reversed(half[:-1])]
    return outer + [(x, z - thick) for x, z in reversed(outer)]


def end_wall_top(x_edge):
    """A bulkhead's top edge under the roof, from x_edge over to -x_edge (counterclockwise order)."""
    pts = [(x_edge, hood_z(x_edge) - 0.05)]
    for x, z in HOOD:
        if CLERE_X < x < x_edge:
            pts.append((x, z - 0.05))
    pts += [(CLERE_X, hood_z(CLERE_X) - 0.05), (CLERE_X, CLERE_TOP - 0.04)]
    pts += [(x, z - 0.04) for x, z in UPPER[1:]]
    return pts + [(-x, z) for x, z in reversed(pts[:-1])]


def car_roof(k, body, beam, mat, deck_glass=True):
    """The clerestory roof: a low arched hood over the whole car (and its end platforms), the raised clerestory over
    the body with little deck lights along its sides, and its end walls."""
    hood = lp.sweep([(0.0, -beam - 0.06, 0.0), (0.0, beam + 0.06, 0.0)], closed_profile(HOOD, 0.05))
    k.map(hood, mat)
    upper = lp.sweep([(0.0, -body, 0.0), (0.0, body, 0.0)], closed_profile(UPPER, 0.04))
    k.map(upper, mat)
    z_low = hood_z(CLERE_X) - 0.02
    for s in (-1.0, 1.0):
        k.box((0.05, 2.0 * body, CLERE_TOP - z_low), (s * (CLERE_X - 0.025), 0.0, (CLERE_TOP + z_low) * 0.5), mat)
        if deck_glass:
            n = int(2.0 * body / 0.62)
            for i in range(n):
                y = -body + (i + 0.5) * 2.0 * body / n
                x = s * (CLERE_X + 0.002)
                c = [(x, y - s * 0.17, 3.79), (x, y + s * 0.17, 3.79), (x, y + s * 0.17, 3.94), (x, y - s * 0.17, 3.94)]
                tk.quad(k, c, 'trim', 'Glass')
    # The bulkheads close the clerestory's ends up to the roof, and the shells' own end caps close the rest.


def car_underframe(k, body, beam, bogies, paint, sill_mat=None):
    """Side sills, end beams with their buffers and couplers, truss rods under the floor, the floor itself and the two
    bogies. Returns the axle positions, front to back."""
    sill_mat = sill_mat or paint
    for s in (-1.0, 1.0):
        k.box((0.1, 2.0 * beam, 0.22), (s * 1.35, 0.0, CAR_FLOOR - 0.11), sill_mat)
    for d in (-1.0, 1.0):
        k.box((2.8, 0.2, 0.42), (0.0, d * (beam - 0.1), CAR_FLOOR - 0.21), paint, bevel=0.015)
        tk.end_gear(k, d * beam, d)
    k.box((2.62, 2.0 * body, 0.06), (0.0, 0.0, CAR_FLOOR - 0.03), 'trim', strip='Beams', axis='y')
    axles = []
    for y in bogies:
        axles += tk.bogie(k, y)
        k.box((2.5, 0.28, 0.16), (0.0, y, 1.03), 'iron')
    queen = 1.25
    for x in (-0.45, 0.45):
        y0, y1 = bogies[0], bogies[1]
        k.tube([(x, y0, 1.08), (x, -queen, 0.66), (x, queen, 0.66), (x, y1, 1.08)], 0.02, 'iron')
        for y in (-queen, queen):
            k.box((0.07, 0.07, 0.48), (x, y, 0.9), 'iron')
        k.cyl((x, -0.15, 0.66), (x, 0.15, 0.66), 0.035, 'iron', sides=6)
    return sorted(axles, key=lambda a: a[1])


def end_platform(k, body, beam, d, door_paint, rail='iron', lamp=None):
    """An end platform from the body's end (y = d * body) to the end beam: plank floor, railings round the end with an
    opening for the coupled car, corner steps both sides, grab handles, and the end door in the bulkhead."""
    depth = beam - body
    yc = d * (body + depth * 0.5)
    k.box((2.74, depth, 0.05), (0.0, yc, CAR_FLOOR - 0.025), 'trim', strip='Siding', axis='x')
    y_rail = d * (beam - 0.05)
    y_ret = d * (beam - 0.32)
    for s in (-1.0, 1.0):
        tk.railing(k, [(s * 0.45, y_rail, CAR_FLOOR), (s * 1.33, y_rail, CAR_FLOOR), (s * 1.33, y_ret, CAR_FLOOR)],
                   1.0, rail)
        steps_y = d * (body + (depth - 0.3) * 0.5)
        tk.corner_steps(k, s * 1.5, steps_y, CAR_FLOOR - 0.02, levels=(0.98, 0.7), width=0.46)
        tk.grab_rail(k, s * (CAR_W - 0.04), d * (body + 0.06), 1.45, 2.75, 'brass', stand=0.0)
    # The bulkhead with its door (a panel door, a little window, a brass knob).
    face = '-y' if d < 0 else '+y'
    xe = CAR_W - WALL
    outline = [(-xe, CAR_FLOOR), (-0.38, CAR_FLOOR), (-0.38, 3.18), (0.38, 3.18), (0.38, CAR_FLOOR), (xe, CAR_FLOOR)]
    outline += end_wall_top(xe)
    k.panel([mirror(outline, face)], WALL, door_paint, tk.frame(face, (0.0, d * body, 0.0)), world=False)
    m = tk.frame(face, (0.0, d * body, 0.0))
    door = mirror([(-0.36, CAR_FLOOR), (0.36, CAR_FLOOR), (0.36, 3.16), (-0.36, 3.16)], face)
    window = mirror(tk.window_outline(-0.22, 2.3, 0.44, 0.62, arch=0.1), face)
    k.panel([door, window], 0.04, door_paint, m @ Matrix.Translation((0.0, 0.03, 0.0)), world=False)
    tk.window(k, face, (0.0, d * body, 0.0), window, 0.08, frame_mat='brass', frame_w=0.03, frame_t=0.012,
              sill=False, glass_at=0.6)
    knob = m @ Vector((0.27, 0.01, 2.0))
    k.cyl(knob, knob + Vector((0.0, d * 0.06, 0.0)), 0.03, 'brass', sides=6)
    if lamp is not None:
        lamp(d)


def coffin(k, x, y, z, length=1.86):
    """A toe-pinch coffin of dark timber with a lid and brass handles, lying along Y with its head toward +Y."""
    h = length * 0.5
    outline = [(-0.17, -h), (0.17, -h), (0.27, h * 0.4), (0.2, h), (-0.2, h), (-0.27, h * 0.4)]
    tk.slab(k, [(x + a, y + b) for a, b in outline], z, z + 0.32, 'trim', strip='Beams', axis=(0.0, 1.0, 0.0))
    lid = [(a * 1.06, b * 1.02) for a, b in outline]
    tk.slab(k, [(x + a, y + b) for a, b in lid], z + 0.32, z + 0.38, 'trim', strip='Beams', axis=(0.0, 1.0, 0.0))
    for s in (-1.0, 1.0):
        k.box((0.03, 0.5, 0.035), (x + s * 0.245, y, z + 0.18), 'brass')               # a brass rail each side


def coffin_rack(k, side, y0, y1, coffins=()):
    """A two-tier rack along one wall inside (side +1 or -1), from y0 to y1, with a brass rail and posts; coffins lie on
    it at the (tier, y) pairs given."""
    x_wall = side * (CAR_W - WALL)
    x_edge = side * (CAR_W - WALL - 0.6)
    for z in sorted({2.2} | {(1.6, 2.2)[tier] for tier, _ in coffins}):
        k.box((0.6, y1 - y0, 0.05), ((x_wall + x_edge) * 0.5, (y0 + y1) * 0.5, z - 0.025), 'trim', strip='Beams',
              axis='y')
    k.tube([(x_edge, y0, 2.32), (x_edge, y1, 2.32)], 0.015, 'brass')
    for y in (y0 + 0.04, y1 - 0.04):
        k.box((0.05, 0.05, 2.3 - CAR_FLOOR), (x_edge, y, (2.3 + CAR_FLOOR) * 0.5), 'trim', strip='Beams', axis='z')
    for tier, y in coffins:
        coffin(k, (x_wall + x_edge) * 0.5, y, (1.6, 2.2)[tier])


def curtains(k, outline, matrix):
    """Black crepe curtains just inside a window, drawn back to both sides and tied with small brass holders: the
    middle stays open (the glass is clear), so the coffins on the rack show through. matrix places the window's frame
    (u along the wall, v up) at the curtains' depth."""
    us = [p[0] for p in outline]
    u0, u1 = min(us), max(us)
    z0 = min(p[1] for p in outline) + 0.01
    z1 = max(p[1] for p in outline) - 0.06
    tie = z0 + (z1 - z0) * 0.42
    for edge, d in ((u0, 1.0), (u1, -1.0)):
        def at(du, z):
            return (edge + d * du, z)
        tk.flat(k, [at(0.0, z0), at(0.15, z0), at(0.11, tie - 0.07), at(0.085, tie), at(0.12, tie + 0.14),
                    at(0.22, z1 - 0.16), at(0.27, z1), at(0.0, z1)], 'crepe', matrix)
        holder = matrix @ Vector((edge + d * 0.08, -0.012, tie))
        k.box((0.04, 0.03, 0.04), holder, 'brass')
    # A gathered pelmet across the head.
    tk.flat(k, [at_ for at_ in ((u0, z1 - 0.07), (u1, z1 - 0.07), (u1, z1 + 0.04), (u0, z1 + 0.04))], 'crepe', matrix)


def swag(k, x, y0, y1, z, drop=0.28, side=1.0):
    """Black crepe draped from the eave (a sagging strip a little proud of the wall), over the brass head rail."""
    pts = []
    for i in range(7):
        t = i / 6.0
        sag = drop * math.sin(math.pi * t) ** 0.8
        pts.append((x + side * (0.03 + 0.035 * math.sin(math.pi * t)), y0 + (y1 - y0) * t, z - sag))
    cloth = lp.sweep(pts, [(-0.012, -0.09), (0.012, -0.09), (0.012, 0.09), (-0.012, 0.09)], up=(0.0, 0.0, 1.0))
    k.map(cloth, 'crepe')


def rosette(k, x, y, z, side=1.0):
    """Where two swags hang: a brass rosette (it reads against the black body) over the crepe's knot and tail."""
    k.box((0.014, 0.13, 0.36), (x + side * 0.015, y, z - 0.2), 'crepe', rot=(4.0, 0.0, 0.0))
    k.lathe([(0.0, 0.0), (0.085, 0.0), (0.065, 0.032), (0.0, 0.045)], 'brass', (x + side * 0.01, y, z),
            (x + side, y, z), sides=8)


def crepe_run(k, x, bays, z, side):
    """Swags over each bay (y0, y1), with one rosette at every bay end (shared where bays meet)."""
    ends = []
    for a, b in bays:
        swag(k, x, a - 0.12, b + 0.12, z, side=side)
        for y in (a - 0.12, b + 0.12):
            if all(abs(y - e) > 0.35 for e in ends):
                ends.append(y)
    for y in ends:
        rosette(k, x + side * 0.035, y, z - 0.01, side=side)


def carriage_lamp(k, x, y, z, side, name):
    """A brass carriage lamp on a bracket off the car's side (side: which way it stands out, +1/-1 in x)."""
    k.beam((x, y, z + 0.1), (x + side * 0.2, y, z + 0.1), 0.04, 0.04, 'brass', up=(0.0, 1.0, 0.0))
    k.beam((x, y, z - 0.12), (x + side * 0.2, y, z + 0.06), 0.03, 0.03, 'brass', up=(0.0, 1.0, 0.0))
    tk.oil_lamp(k, (x + side * 0.24, y, z - 0.05), facing=(side, 0.0, 0.0), size=0.22, socket=name)


# --- Tilly's hearse car ---

HB, HE = 3.9, 4.8               # hearse body half length, end beams
H_BOGIES = (-2.95, 2.95)
DOOR_Y0, DOOR_Y1 = -1.3, -0.05  # the side doorway (its hinge at DOOR_Y1)
DOOR_TOP = 3.24
PIVOT = (CAR_W + 0.07, DOOR_Y1, CAR_FLOOR + 0.01)
H_WINDOWS = [(-3.55, -2.7), (-2.4, -1.55), (1.55, 2.4), (2.7, 3.55)]


def hearse_side(k, face):
    """One side of the hearse car: black panelling with brass beading, four etched-glass windows (clear in the middle,
    frosted borders and corner fans: the coffin rack shows through), the doorway (+X) or the fixed twin door (-X)."""
    s = 1.0 if face == '+x' else -1.0
    x = s * CAR_W
    outline = [(-HB, CAR_FLOOR), (HB, CAR_FLOOR), (HB, CAR_EAVE), (-HB, CAR_EAVE)]
    if s > 0:
        outline = [(-HB, CAR_FLOOR), (DOOR_Y0, CAR_FLOOR), (DOOR_Y0, DOOR_TOP), (DOOR_Y1, DOOR_TOP),
                   (DOOR_Y1, CAR_FLOOR), (HB, CAR_FLOOR), (HB, CAR_EAVE), (-HB, CAR_EAVE)]
    holes = [tk.window_outline(a, 2.02, b - a, 0.98, arch=0.12) for a, b in H_WINDOWS]
    m = tk.frame(face, (x, 0.0, 0.0))
    k.panel([mirror(outline, face)] + [mirror(h, face) for h in holes], WALL, 'wblack', m, world=False)
    glass = m @ Matrix.Translation((0.0, 0.03, 0.0))
    for h in holes:
        hm = mirror(h, face)
        curtains(k, hm, glass)
        tk.window(k, face, (x, 0.0, 0.0), hm, WALL, frame_mat='brass', frame_w=0.035, frame_t=0.015, glass=False,
                  sill=False)
    k.section('  wall and windows')
    # Brass beading: belt rails at the sills and heads, and the lower panels' divisions; Tilly's name in brass on the
    # platform side's lower panels, split by the door.
    for z in (2.0, 3.04, 1.42):
        for a, b in ((-HB + 0.05, DOOR_Y0 - 0.05), (DOOR_Y1 + 0.05, HB - 0.05)) if s > 0 else ((-HB + 0.05, HB - 0.05),):
            k.box((0.014, b - a, 0.025), (x + s * 0.007, (a + b) * 0.5, z), 'brass')
    divisions = [-3.55, -1.55, 1.55, 3.55, 0.6] if s > 0 else [-3.55, -2.7, -2.4, -1.55, -0.68, 1.55, 2.4, 2.7, 3.55]
    for y in divisions:
        k.box((0.014, 0.025, 0.56), (x + s * 0.007, y, 1.71), 'brass')
    if s > 0:
        k.text('BRIGHT &', 0.24, (x + 0.004, -2.55, 1.68), (90.0, 0.0, 90.0), 'brass')
        k.text('DAUGHTER', 0.24, (x + 0.004, 2.55, 1.68), (90.0, 0.0, 90.0), 'brass')
    for y in (-HB + 0.04, HB - 0.04):
        k.box((0.08, 0.08, CAR_EAVE - CAR_FLOOR), (x - s * 0.03, y, (CAR_EAVE + CAR_FLOOR) * 0.5), 'wblack')
    k.section('  beading')
    if s > 0:
        # The doorway: brass-edged casing, a threshold, the hinge pintles, and steps down to the platform.
        for y in (DOOR_Y0 - 0.03, DOOR_Y1 + 0.03):
            k.box((0.04, 0.06, DOOR_TOP - CAR_FLOOR), (x + 0.01, y, (DOOR_TOP + CAR_FLOOR) * 0.5), 'brass')
        k.box((0.04, DOOR_Y1 - DOOR_Y0 + 0.12, 0.06), (x + 0.01, (DOOR_Y0 + DOOR_Y1) * 0.5, DOOR_TOP + 0.03), 'brass')
        k.box((0.12, DOOR_Y1 - DOOR_Y0, 0.04), (x - 0.04, (DOOR_Y0 + DOOR_Y1) * 0.5, CAR_FLOOR - 0.01), 'brass')
        for z in (1.62, 2.86):
            k.cyl((PIVOT[0], PIVOT[1], z), (PIVOT[0], PIVOT[1], z + 0.14), 0.022, 'iron', sides=6)
            k.box((0.08, 0.03, 0.05), (x + 0.04, PIVOT[1] + 0.02, z + 0.07), 'iron')
        tk.corner_steps(k, 1.5, (DOOR_Y0 + DOOR_Y1) * 0.5, CAR_FLOOR - 0.02, levels=(0.98, 0.7), width=0.9)
    else:
        # The fixed twin of the platform side's door, and its steps.
        door = [(DOOR_Y0, CAR_FLOOR + 0.01), (DOOR_Y1, CAR_FLOOR + 0.01), (DOOR_Y1, DOOR_TOP), (DOOR_Y0, DOOR_TOP)]
        k.panel([mirror(door, face)], 0.03, 'wblack', m @ Matrix.Translation((0.0, -0.03, 0.0)), world=False)
        for y in (DOOR_Y0 - 0.03, DOOR_Y1 + 0.03):
            k.box((0.04, 0.06, DOOR_TOP - CAR_FLOOR), (x - 0.01, y, (DOOR_TOP + CAR_FLOOR) * 0.5), 'brass')
        for z in (1.75, 2.85):
            k.box((0.015, 1.0, 0.07), (x - 0.04, (DOOR_Y0 + DOOR_Y1) * 0.5 + 0.05, z), 'iron')
        tk.corner_steps(k, -1.5, (DOOR_Y0 + DOOR_Y1) * 0.5, CAR_FLOOR - 0.02, levels=(0.98, 0.7), width=0.9)
    k.section('  door')
    # Crepe swags over the window bays, rosettes between them.
    bays = H_WINDOWS if s > 0 else H_WINDOWS[:2] + [(DOOR_Y0, DOOR_Y1 + 1.35)] + H_WINDOWS[2:]
    crepe_run(k, x, bays, CAR_EAVE - 0.06, s)


def hearse_car():
    k = tk.Kit('HearseCar', 31)
    axles = car_underframe(k, HB, HE, H_BOGIES, 'wblack')
    k.section('underframe')
    for face in ('+x', '-x'):
        hearse_side(k, face)
        k.section('side ' + face)
    for d in (-1.0, 1.0):
        end_platform(k, HB, HE, d, 'wblack', rail='iron')
    k.section('end platforms')
    car_roof(k, HB, HE, 'iron')
    k.section('roof')
    # Brass roof rail and urns along the clerestory, Tilly's name over the door.
    for s in (-1.0, 1.0):
        xr = s * 0.68
        k.tube([(xr, -HB + 0.1, 4.13), (xr, HB - 0.1, 4.13)], 0.015, 'brass', sides=5)
        for i in range(1, 5):
            y = -HB + 0.1 + (2.0 * HB - 0.2) * i / 5
            k.tube([(xr, y, upper_z(0.68) - 0.01), (xr, y, 4.13)], 0.012, 'brass', sides=4)
        for y in (-HB + 0.1, HB - 0.1):
            k.lathe([(0.0, 0.0), (0.06, 0.0), (0.04, 0.05), (0.085, 0.13), (0.06, 0.21), (0.035, 0.24), (0.0, 0.3)],
                    'brass', (xr, y, upper_z(0.68) - 0.01), sides=6)
    k.section('roof rail')
    # Inside: the floor, coffin racks along both walls, three coffins.
    coffin_rack(k, 1.0, -3.55, -1.5, coffins=[(1, -2.55)])
    coffin_rack(k, 1.0, 1.5, 3.55, coffins=[(1, 2.5)])
    coffin_rack(k, -1.0, -3.55, -1.5, coffins=[(0, -2.5)])
    coffin_rack(k, -1.0, 1.5, 3.55)
    k.section('racks')
    # Brass lamps: two flanking the door, two at the back as tail lamps.
    carriage_lamp(k, CAR_W, DOOR_Y0 - 0.125, 2.6, 1.0, 'Light_1')
    carriage_lamp(k, CAR_W, 1.425, 2.6, 1.0, 'Light_2')
    for i, s in enumerate((1.0, -1.0)):
        tk.oil_lamp(k, (s * 1.22, HB + 0.16, 2.55), facing=(0.0, 1.0, 0.0), size=0.2, socket=f'Light_{3 + i}')
        k.box((0.06, 0.1, 0.18), (s * 1.22, HB + 0.07, 2.5), 'brass')
    k.section('lamps')
    for i, a in enumerate(axles):
        k.socket(f'Axle_{i + 1}', a)
    k.socket('Coupler_Front', (0.0, -(HE + tk.REACH), tk.COUPLER_Z))
    k.socket('Coupler_Back', (0.0, HE + tk.REACH, tk.COUPLER_Z), (0.0, 0.0, 180.0))
    k.socket('Door', PIVOT)
    k.socket('Arrival', (2.35, (DOOR_Y0 + DOOR_Y1) * 0.5, tk.PLATFORM_Z), (0.0, 0.0, 90.0))
    k.hull((2.9, 2.0 * (HE + tk.REACH), 4.3), (0.0, 0.0, 2.15))
    model = k.finish(lods=BODY_LODS, screens=BODY_SCREENS, ao=0.7)
    model['_axles'] = 'Carriage,Carriage,Carriage,Carriage'
    return model


def hearse_door():
    """The hearse car's side door: black panels, an etched-glass window, brass mouldings and handle, long strap hinges.
    Its origin is the hinge pivot (the car's SOCKET_Door): it opens by turning 180 degrees outward about its up axis,
    folding flat against the plain panel behind the doorway. No collision."""
    k = tk.Kit('HearseCarDoor', 32)
    x0, x1 = CAR_W - 0.015, CAR_W + 0.035
    ya, yb = DOOR_Y0 + 0.015, DOOR_Y1 - 0.015
    za, zb = CAR_FLOOR + 0.01, DOOR_TOP - 0.02
    m = tk.frame('+x', (x1, 0.0, 0.0))
    window = tk.window_outline(ya + 0.2, 2.2, yb - ya - 0.4, 0.82, arch=0.14)
    leaf = [(ya, za), (yb, za), (yb, zb), (ya, zb)]
    k.panel([leaf, window], x1 - x0, 'wblack', m, world=False)
    curtains(k, window, m @ Matrix.Translation((0.0, 0.035, 0.0)))
    tk.window(k, '+x', (x1, 0.0, 0.0), window, x1 - x0, frame_mat='brass', frame_w=0.035, frame_t=0.012, glass=False)
    for z0, z1 in ((za + 0.12, 1.98), ):
        k.panel([tk.expand([(ya + 0.15, z0), (yb - 0.15, z0), (yb - 0.15, z1), (ya + 0.15, z1)], 0.0),
                 [(ya + 0.19, z0 + 0.04), (yb - 0.19, z0 + 0.04), (yb - 0.19, z1 - 0.04), (ya + 0.19, z1 - 0.04)]],
                0.012, 'brass', m @ Matrix.Translation((0.0, -0.012, 0.0)), world=False)
    for z in (1.66, 2.9):
        k.box((0.014, 0.9, 0.07), (x1 + 0.007, yb - 0.45, z), 'iron')
        k.cyl((PIVOT[0], PIVOT[1], z - 0.08), (PIVOT[0], PIVOT[1], z - 0.0), 0.024, 'iron', sides=6)
        k.box((PIVOT[0] - x1, 0.03, 0.06), ((PIVOT[0] + x1) * 0.5, PIVOT[1] - 0.02, z - 0.04), 'iron')
    k.box((0.03, 0.04, 0.2), (x1 + 0.03, ya + 0.12, 2.0), 'brass')
    k.cyl((x1, ya + 0.12, 1.98), (x1 + 0.05, ya + 0.12, 1.98), 0.03, 'brass', sides=6)
    model = k.finish(lods='50,20', screens='0.3,0.1', ao=0.3, ground=False, collision=False)
    # The pivot becomes the origin; the geometry stays where it hangs on the car.
    lp.place(model, (-PIVOT[0], -PIVOT[1], -PIVOT[2]))
    model.location = PIVOT
    return model


# --- The passenger car ---

PB, PE = 4.8, 5.8               # passenger car body half length, end beams
P_BOGIES = (-3.75, 3.75)
P_WINDOWS = [(-4.8 + 0.35 + i * 1.2, -4.8 + 0.35 + i * 1.2 + 0.7) for i in range(8)]


def passenger_side(k, face):
    """One side of the coach: faded teal lower panels and letterboard, a cream window band with eight round-topped
    windows of dark glass in teal frames, cream beading on the panels."""
    s = 1.0 if face == '+x' else -1.0
    x = s * CAR_W
    m = tk.frame(face, (x, 0.0, 0.0))
    k.panel([mirror([(-PB, CAR_FLOOR), (PB, CAR_FLOOR), (PB, 2.0), (-PB, 2.0)], face)], WALL, 'wteal', m, world=False)
    holes = [tk.window_outline(a, 2.08, b - a, 0.86, arch=0.12) for a, b in P_WINDOWS]
    band = [(-PB, 2.0), (PB, 2.0), (PB, 3.06), (-PB, 3.06)]
    k.panel([mirror(band, face)] + [mirror(h, face) for h in holes], WALL, 'wcream', m, world=False)
    k.panel([mirror([(-PB, 3.06), (PB, 3.06), (PB, CAR_EAVE), (-PB, CAR_EAVE)], face)], WALL, 'wteal', m, world=False)
    for h in holes:
        tk.window(k, face, (x, 0.0, 0.0), mirror(h, face), WALL, frame_mat='wteal', frame_w=0.05, frame_t=0.02)
    for z in (1.31, 2.0, 3.06):
        k.box((0.016, 2.0 * PB - 0.1, 0.035), (x + s * 0.008, 0.0, z), 'cream')
    for a, b in P_WINDOWS[:-1]:
        k.box((0.016, 0.035, 0.62), (x + s * 0.008, b + 0.25, 1.65), 'cream')
    k.box((0.016, 0.035, 0.62), (x + s * 0.008, P_WINDOWS[0][0] - 0.25, 1.65), 'cream')
    k.box((0.016, 0.035, 0.62), (x + s * 0.008, P_WINDOWS[-1][1] + 0.25, 1.65), 'cream')
    for y in (-PB + 0.04, PB - 0.04):
        k.box((0.08, 0.08, CAR_EAVE - CAR_FLOOR), (x - s * 0.03, y, (CAR_EAVE + CAR_FLOOR) * 0.5), 'wteal')
    k.text('No 12', 0.2, (x + s * 0.004, P_WINDOWS[3][1] + 0.25 if s > 0 else P_WINDOWS[4][0] - 0.25, 1.66),
           (90.0, 0.0, s * 90.0), 'cream')


def passenger_car():
    k = tk.Kit('PassengerCar', 41)
    axles = car_underframe(k, PB, PE, P_BOGIES, 'wteal')
    k.section('underframe')
    for face in ('+x', '-x'):
        passenger_side(k, face)
    k.section('sides')
    for d in (-1.0, 1.0):
        end_platform(k, PB, PE, d, 'wteal', rail='iron')
    k.section('end platforms')
    car_roof(k, PB, PE, 'iron')
    k.section('roof')
    for i, a in enumerate(axles):
        k.socket(f'Axle_{i + 1}', a)
    k.socket('Coupler_Front', (0.0, -(PE + tk.REACH), tk.COUPLER_Z))
    k.socket('Coupler_Back', (0.0, PE + tk.REACH, tk.COUPLER_Z), (0.0, 0.0, 180.0))
    k.hull((2.9, 2.0 * (PE + tk.REACH), 4.3), (0.0, 0.0, 2.15))
    model = k.finish(lods=BODY_LODS, screens=BODY_SCREENS, ao=0.6)
    model['_axles'] = 'Carriage,Carriage,Carriage,Carriage'
    return model


# --- Running gear (separate models: the game spins the wheel sets and moves the rods) ---

def running_gear():
    driver = tk.wheel_set('TrainWheelSet_Driver', tk.DRIVER_R, 10, 'oxide', 'iron', cranks=(math.pi, -math.pi * 0.5),
                          seed=21)
    carriage = tk.wheel_set('TrainWheelSet_Carriage', tk.CARRIAGE_R, 8, 'iron', 'iron', journal=1.0, seed=22)
    rod = tk.coupling_rod('TrainCouplingRod', SPACING, seed=23)
    return driver, carriage, rod


driver, carriage, rod = running_gear()
# The user picked Locomotive_B (2026-10-02). Locomotive_A is still built for the previews, to compare, but never
# exported; each body has its own seed, so leaving it out changes nothing else.
loco_a = locomotive('Locomotive_A', western=False) if lt.want_preview() else None
hearse = hearse_car()
door = hearse_door()
coach = passenger_car()
loco_b = locomotive('Locomotive_B', western=True)


def attached_models(body):
    wheels = {'Driver': driver, 'Carriage': carriage}
    models = {f'Axle_{i + 1}': wheels[kind] for i, kind in enumerate(body['_axles'].split(','))}
    models.update({'Rod_L': rod, 'Rod_R': rod, 'Door': door})
    return models


def coupler_y(body, end):
    """The local y of a body's coupler plane ('Front' or 'Back')."""
    return next(c for c in body.children if c.name.startswith('SOCKET_Coupler_' + end)).location.y


def socket_parts(body):
    """(model, matrix) for every separate model at the body's sockets, the body at the origin."""
    bpy.context.view_layer.update()
    models = attached_models(body)
    found = []
    for child in body.children:
        key = child.name[len('SOCKET_'):].split('.')[0] if child.name.startswith('SOCKET_') else None
        if key in models:
            found.append((models[key], child.matrix_world.copy()))
    return found


def overview(loco, name):
    """The train coupled at the platform: the locomotive, the passenger car, the hearse car, on plain track."""
    train = [loco, coach, hearse]
    y = 0.0
    for i, body in enumerate(train):
        if i:
            y += coupler_y(train[i - 1], 'Back') - coupler_y(body, 'Front')
        body.location = (0.0, y, 0.0)
    bpy.context.view_layer.update()
    y0, y1 = coupler_y(loco, 'Front') - 1.5, y + coupler_y(hearse, 'Back') + 1.5
    track = tk.preview_track(y0, y1)
    copies = [c for body in train for c in tk.attach(body, attached_models(body))]
    shown = train + [track] + copies
    path = tk.render(shown, name, view=(1.0, -0.62, 0.32), lens=38.0, fit=0.6, resolution=(1920, 1080),
                     folder=os.path.dirname(tk.PREVIEW_DIR) if name == 'Train_overview' else None)
    tk.remove(copies + [track])
    for body in train:
        body.location = (0.0, 0.0, 0.0)
    return path


if lt.want_preview():
    import sys
    # Optional names after '--' limit the renders: model names, 'overview', 'lods' (commas or spaces between them).
    words = [w for a in (sys.argv[sys.argv.index('--') + 1:] if '--' in sys.argv else []) for w in a.split(',')]
    only = [w for w in words if w and not w.startswith('--')]
    door.hide_render = True
    if not only or 'overview' in only:
        overview(loco_a, 'Train_overview')
        overview(loco_b, 'Train_overview_B')
    if not only or 'lods' in only:
        for body in (loco_a, loco_b, coach, hearse):
            counts, _ = tk.lod_sheet(body, socket_parts(body), f'{body.name}_LODs')
            print(f'TRAIN: LODs {body.name}: {counts}', flush=True)
        for part in (driver, carriage, rod, door):
            counts = []
            for share in (1.0, 0.5, 0.2):
                copy = tk.decimated(part, share, 'PV_count')
                counts.append(lp.tri_count(copy))
                tk.remove([copy])
            print(f'TRAIN: LODs {part.name}: {counts}', flush=True)
    for body in (loco_a, hearse, coach, loco_b):
        if only and body.name not in only:
            continue
        half = max(abs(v.co.y) for v in body.data.vertices)
        track = tk.preview_track(-half - 0.6, half + 0.6, platform=(-half, half) if body is hearse else None)
        copies = tk.attach(body, attached_models(body))
        shown = [body, track] + copies
        tk.render(shown, f'{body.name}_front', view=(0.95, -1.3, 0.42), lens=45.0, fit=0.62)
        tk.render(shown, f'{body.name}_side', view=(1.0, -0.04, 0.12), lens=50.0, fit=0.62, resolution=(1600, 700))
        if body in (loco_a, loco_b):
            tk.closeup(shown, f'{body.name}_closeup', (0.4, -1.2, 1.9), (1.0, -0.9, 0.35), 8.5, lens=40.0)
        if body is hearse:
            tk.closeup(shown, 'HearseCar_door', (1.4, -0.4, 2.0), (1.0, -0.6, 0.2), 6.5, lens=40.0)
            # The door standing open (turned 180 degrees about its hinge, as the game opens it).
            shut = next(c for c in copies if c.name.startswith('PV_HearseCarDoor'))
            shut.matrix_world = shut.matrix_world @ Matrix.Rotation(math.radians(180.0), 4, 'Z')
            tk.closeup(shown, 'HearseCar_door_open', (1.4, -0.2, 1.8), (1.0, -0.35, 0.18), 7.5, lens=40.0)
        tk.remove(copies + [track])
