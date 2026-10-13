"""The Providence's ore tramway (Docs/Areas/RansomsRest_Expansion.md, section 6: the Diggings and the mine): a
narrow-gauge track kit and the ore car that ran on it, out of the mine's adit, across Larkspur Bench to the lift head,
and down the mine's galleries and decline. The station's rail kit (Railway.py) is standard gauge on ballast, so it
can't be reused: this is its small cousin, with the same materials (WoodBlack ties, IronBlack rails). Small instanced
props (Art/README.md): no Nanite, LODs at 50% and 25%; built with looter_buildings through looter_mountain.

The tramway contract (the mine's layout and the level's builder chain pieces with it):
- the ground under the track is z = 0 at a piece's origin and the rail heads' tops are at RAIL_TOP = 0.125 m;
- the gauge is 0.6 m between the rail heads' inner faces;
- a piece runs from its origin along -Y: a straight of length L ends at (0, -L, 0);
- ties every 0.5 m, sunk 3 cm, so they sit on a mine's dirt floor or the Bench's turf without ballast.

  RR_TramTrack_4m, RR_TramTrack_2m   straights; chain them end to origin, same rotation (they also run pitched down a
                                     decline: rotate about their own X).
  RR_TramTrack_Curve   15 degrees of a 10 m radius (2.62 m along the centreline), turning toward +X: it ends at
                       (0.3407, -2.5882, 0), heading turned 15 degrees toward +X. Run backwards (rotated 165 degrees,
                       its END on the chain point) it turns the other way.
  RR_TramTrack_End     a timber bumper across the rails at the end of a line (its pivot on the last piece's end, the
                       same rotation; it stands over the last 0.8 m, its face toward +Y, up the line).
  RR_OreCar            a wooden ore car with iron bands on four iron wheels, a little ore in its bottom: 1.5 m long, its
                       pivot on the ground at its middle, its length along Y (front -Y), its wheels on the gauge's rail
                       heads when it stands at a track's origin line. Tip it on its side by rotating about Y for the
                       runaway in the decline's sump.

Collision: one low hull per track piece (flat at the rail heads between x = +-0.5, sloping to the ground at +-0.8), so a
walking capsule steps across; the end stop and the car have box hulls.
  --preview  renders the kit into Saved/ArtPreviews/RansomsRest/Mountain/.
"""
import math

from mathutils import Matrix, Vector

import looter_buildings as kit
import looter_mountain as mtn
import looter_textures as lt
import looter_town as town
from looter_buildings import Tile, Trim

GAUGE = 0.6
HEAD_W = 0.04
RAIL_H = 0.065
RAIL_TOP = 0.125
RAIL_X = GAUGE * 0.5 + HEAD_W * 0.5
TIE = (1.1, 0.13, 0.09)               # across, along, deep
TIE_TOP = RAIL_TOP - RAIL_H            # 0.06: the tie's top (sunk 3 cm into the ground)
TIE_PITCH = 0.5
CURVE_RADIUS, CURVE_ANGLE = 10.0, 15.0
PROFILE = [(0.032, 0.0), (0.032, 0.008), (0.008, 0.018), (0.006, 0.046), (0.02, 0.05), (0.02, RAIL_H),
           (-0.02, RAIL_H), (-0.02, 0.05), (-0.006, 0.046), (-0.008, 0.018), (-0.032, 0.008), (-0.032, 0.0)]
METAL_TILE = 3.2


class Path:
    """A piece's centreline (Railway.py's): a straight along -Y, or a curve turning toward +X."""

    def __init__(self, length=None, radius=None, angle=None):
        self.radius = radius
        self.theta = math.radians(angle) if angle else 0.0
        self.length = radius * self.theta if radius else length

    def point(self, s):
        if not self.radius:
            return Vector((0.0, -s, 0.0))
        phi = s / self.radius
        return Vector((self.radius * (1.0 - math.cos(phi)), -self.radius * math.sin(phi), 0.0))

    def across(self, s):
        phi = s / self.radius if self.radius else 0.0
        return Vector((math.cos(phi), math.sin(phi), 0.0))

    def frame(self, s):
        x = self.across(s)
        y = Vector((-x.y, x.x, 0.0))
        return kit.basis(self.point(s), x, y, Vector((0.0, 0.0, 1.0)))

    def at(self, s, u, z):
        return self.point(s) + self.across(s) * u + Vector((0.0, 0.0, z))


def rails(m, path, segments):
    """Both rails swept along the path, capped, U along the rail and V round its section (IronBlack)."""
    perimeter = [0.0]
    for a, b in zip(PROFILE, PROFILE[1:]):
        perimeter.append(perimeter[-1] + math.hypot(b[0] - a[0], b[1] - a[1]))
    for side in (-1.0, 1.0):
        tb = kit._new_bmesh()
        uv = tb.loops.layers.uv.active
        rows, coords = [], []
        for i in range(segments + 1):
            s = path.length * i / segments
            rows.append([tb.verts.new(path.at(s, side * RAIL_X + px, TIE_TOP + pz)) for px, pz in PROFILE])
            coords.append(s)
        for r0, r1 in zip(rows, rows[1:]):
            for j in range(len(r0) - 1):
                tb.faces.new((r0[j], r0[j + 1], r1[j + 1], r1[j]))
        for row, s, sign in ((rows[0], 0.0, 1.0), (rows[-1], path.length, -1.0)):
            face = tb.faces.new(row)
            face.normal_update()
            if face.normal.dot(path.frame(s).to_3x3() @ Vector((0.0, sign, 0.0))) < 0.0:
                face.normal_flip()
        tb.normal_update()
        index = {v: (i, j) for i, row in enumerate(rows) for j, v in enumerate(row)}
        for face in tb.faces:
            cap = len(face.verts) == len(PROFILE)
            for loop in face.loops:
                i, j = index[loop.vert]
                px, pz = PROFILE[j]
                loop[uv].uv = ((px / METAL_TILE, pz / METAL_TILE) if cap else
                               (coords[i] / METAL_TILE, perimeter[j] / METAL_TILE))
        tb.normal_update()
        import bmesh
        bmesh.ops.triangulate(tb, faces=[f for f in tb.faces if len(f.verts) > 4], quad_method='BEAUTY',
                              ngon_method='BEAUTY')
        m.emit(tb, None, 'iron')


def tie(m, matrix, rng, k):
    """A tie (WoodBlack, grain across the track) with an iron spike head by each rail's foot."""
    lx, ly, lz = TIE
    m.emit(kit._box(TIE, drop=('-z',)), town.PlankRow(), 'woodblack', matrix @ Matrix.Translation((0.0, 0.0, TIE_TOP - lz * 0.5)))
    for side in (-1.0, 1.0):
        x = side * RAIL_X
        dx = side * (0.042 if k % 2 == 0 else -0.042)
        m.emit(kit._box((0.022, 0.022, 0.018), drop=('-z',)), Tile('MetalWorn'), 'iron',
               matrix @ Matrix.Translation((x + dx, side * 0.03, TIE_TOP + 0.009)))


def track_hull(m, path, count):
    section = [(-0.8, -0.03), (-0.5, RAIL_TOP), (0.5, RAIL_TOP), (0.8, -0.03)]
    for k in range(count):
        s0, s1 = path.length * k / count, path.length * (k + 1) / count
        m.hull_points([path.at(s, u, z) for s in (s0, s1) for u, z in section])


def track(name, seed, path, segments, hulls=1):
    m = mtn.Model(name, seed=seed)
    rng = m.rng
    count = max(1, round(path.length / TIE_PITCH))
    pitch = path.length / count
    for k in range(count):
        s = pitch * (k + 0.5) + rng.uniform(-0.015, 0.015)
        jitter = (Matrix.Translation((rng.uniform(-0.015, 0.015), 0.0, -rng.uniform(0.0, 0.01))) @
                  Matrix.Rotation(math.radians(rng.uniform(-2.0, 2.0)), 4, 'Z'))
        tie(m, path.frame(s) @ jitter, rng, k)
    rails(m, path, segments)
    track_hull(m, path, hulls)
    return m.finish(Nanite=0, LODs='50,25')


def end_stop():
    """A timber bumper over the last 0.8 m of a line: two short posts outside the rails, a beam across at buffer
    height with an iron face, braces back to a sill, all spiked to two ties."""
    m = mtn.Model('RR_TramTrack_End', seed=711)
    # Over the last 0.8 m of the line (+Y of its pivot, the line's end): posts outside the rails, the beam's face up
    # the line, braces back to the ground just past the end.
    for x in (-0.48, 0.48):
        mtn.post(m, x, 0.2, -0.25, 0.62, size=0.16)
        mtn.beam(m, (x, -0.28, 0.02), (x, 0.16, 0.52), 0.1, 0.12, face=(x, 0.0, 0.0))
        mtn.beam(m, (x, 0.75, -0.02), (x, -0.32, -0.02), 0.12, 0.14, face=(0.0, 0.0, 1.0))
    mtn.beam(m, (-0.62, 0.3, 0.45), (0.62, 0.3, 0.45), 0.22, 0.2, face=(0.0, 1.0, 0.0))
    mtn.iron_box(m, (0.9, 0.02, 0.14), at=(0.0, 0.41, 0.45))
    for x in (-0.3, 0.3):
        mtn.bolt(m, (x, 0.42, 0.45), (0.0, 1.0, 0.0))
    m.hull((1.3, 0.75, 0.62), at=(0.0, 0.05, 0.31))
    return m.finish(Nanite=0, LODs='50,25')


def ore_car():
    """A wooden ore car: a flared plank box with iron corner angles and bands, on two timber sills with four iron
    wheels on the gauge, a coupling link at each end, a little ore in its bottom."""
    m = mtn.Model('RR_OreCar', seed=731)
    rng = m.rng
    L, W0, W1 = 1.5, 0.84, 0.94          # length; width at the floor and at the top
    FLOOR, TOP = 0.46, 1.06
    WHEEL_R = 0.17
    AXLE_Y = 0.42
    axle_z = RAIL_TOP + WHEEL_R
    # Wheels on their axles, under the sills.
    for y in (-AXLE_Y, AXLE_Y):
        mtn.rod(m, (-RAIL_X - 0.06, y, axle_z), (RAIL_X + 0.06, y, axle_z), r=0.03, sides=8)
        for side in (-1.0, 1.0):
            c = Vector((side * RAIL_X, y, axle_z))
            mtn.sheave(m, c, (1.0, 0.0, 0.0), WHEEL_R, width=0.06, spokes=5, groove=False, hub=0.045)
            # The flange on the rail's inner side.
            mtn.ring(m, [(WHEEL_R - 0.02, -0.01), (WHEEL_R + 0.022, -0.01), (WHEEL_R + 0.022, 0.006), (WHEEL_R - 0.02, 0.006),
                         (WHEEL_R - 0.02, -0.01)], c - Vector((side * 0.03, 0.0, 0.0)), (1.0, 0.0, 0.0), sides=16)
            mtn.iron_box(m, (0.08, 0.16, 0.12), at=(side * (RAIL_X - 0.08), y, axle_z + 0.04))
    # Two sills along the car and the end beams.
    for x in (-0.26, 0.26):
        mtn.beam(m, (x, -L * 0.5 - 0.06, FLOOR - 0.08), (x, L * 0.5 + 0.06, FLOOR - 0.08), 0.14, 0.12, face=(x, 0.0, 0.0))
    for y in (-L * 0.5 - 0.02, L * 0.5 + 0.02):
        mtn.beam(m, (-W0 * 0.5, y, FLOOR - 0.06), (W0 * 0.5, y, FLOOR - 0.06), 0.16, 0.12, face=(0.0, y, 0.0))
        mtn.iron_box(m, (0.1, 0.12, 0.08), at=(0.0, y + math.copysign(0.09, y), FLOOR - 0.06))
        mtn.rod(m, (0.0, y + math.copysign(0.14, y), FLOOR - 0.06), (0.0, y + math.copysign(0.26, y), FLOOR - 0.08),
                r=0.018)
    # The box: a plank floor, flared sides and ends of planks laid level, iron angles up the corners, two bands round.
    town.box(m, (W0, L, 0.05), at=(0.0, 0.0, FLOOR + 0.025), look='planks')
    rows = 4
    for k in range(rows):
        z0 = FLOOR + 0.05 + (TOP - FLOOR - 0.05) * k / rows
        z1 = FLOOR + 0.05 + (TOP - FLOOR - 0.05) * (k + 1) / rows
        zc = (z0 + z1) * 0.5
        w = W0 + (W1 - W0) * (zc - FLOOR) / (TOP - FLOOR)
        h = z1 - z0 - 0.006
        for side in (-1.0, 1.0):
            mtn.plank(m, (side * w * 0.5, -L * 0.5, zc), (side * w * 0.5, L * 0.5, zc), h, 0.045,
                      face=(side, 0.0, -0.08), look='A')
        for y in (-L * 0.5 + 0.02, L * 0.5 - 0.02):
            mtn.plank(m, (-w * 0.5 + 0.04, y, zc), (w * 0.5 - 0.04, y, zc), h, 0.045, face=(0.0, math.copysign(1.0, y),
                                                                                                0.0), look='A')
    for sx in (-1.0, 1.0):
        for sy in (-1.0, 1.0):
            p0 = Vector((sx * (W0 * 0.5 + 0.02), sy * (L * 0.5 + 0.0), FLOOR + 0.02))
            p1 = Vector((sx * (W1 * 0.5 + 0.02), sy * (L * 0.5 + 0.0), TOP + 0.01))
            mtn.iron_box(m, (0.07, 0.07, (p1 - p0).length), at=tuple((p0 + p1) * 0.5),
                         rot=(0.0, -sx * math.degrees(math.atan2(W1 - W0, 2 * (TOP - FLOOR))), 0.0))
    for z in (FLOOR + 0.18, TOP - 0.1):
        w = W0 + (W1 - W0) * (z - FLOOR) / (TOP - FLOOR) + 0.1
        for side in (-1.0, 1.0):
            mtn.iron_box(m, (0.014, L + 0.06, 0.06), at=(side * w * 0.5, 0.0, z))
        for y in (-L * 0.5 - 0.035, L * 0.5 + 0.035):
            mtn.iron_box(m, (w, 0.014, 0.06), at=(0.0, y, z))
    # A push handle at one end; ore in its bottom: broken granite.
    mtn.rod(m, (-0.3, L * 0.5 + 0.08, TOP - 0.02), (0.3, L * 0.5 + 0.08, TOP - 0.02), r=0.018)
    for x in (-0.3, 0.3):
        mtn.rod(m, (x, L * 0.5 + 0.08, TOP - 0.02), (x, L * 0.5 + 0.02, TOP - 0.1), r=0.016)
    for k in range(9):
        s = rng.uniform(0.12, 0.22)
        town.box(m, (s, s * rng.uniform(0.7, 1.0), s * 0.6),
                 at=(rng.uniform(-0.25, 0.25), rng.uniform(-0.5, 0.5), FLOOR + 0.08 + s * 0.2),
                 rot=(rng.uniform(-25, 25), rng.uniform(-25, 25), rng.uniform(0, 90)), look='granite', bevel=0.02)
    m.hull((W1 + 0.1, L + 0.1, TOP - RAIL_TOP), at=(0.0, 0.0, (TOP + RAIL_TOP) * 0.5))
    m.hull((0.9, 1.0, RAIL_TOP + 0.35), at=(0.0, 0.0, (RAIL_TOP + 0.35) * 0.5))
    m.socket('Load', (0.0, 0.0, FLOOR + 0.1))
    return m.finish(Nanite=0, LODs='50,25')


CURVE_END = Path(radius=CURVE_RADIUS, angle=CURVE_ANGLE).point(CURVE_RADIUS * math.radians(CURVE_ANGLE))

straight_4 = track('RR_TramTrack_4m', 701, Path(length=4.0), 2)
straight_2 = track('RR_TramTrack_2m', 702, Path(length=2.0), 1)
curve = track('RR_TramTrack_Curve', 703, Path(radius=CURVE_RADIUS, angle=CURVE_ANGLE), 6, hulls=2)
stop = end_stop()
car = ore_car()
print(f'TRAMWAY: curve end {tuple(round(c, 4) for c in CURVE_END)}', flush=True)

if lt.want_preview():
    import bpy
    scene, sun = mtn.stage()
    human = mtn.homes.figure()
    # A little run: a straight, the curve, a straight, the stop; a car on the straight.
    pieces = []
    for obj, at, rot in ((straight_4, (0.0, 0.0, 0.0), 0.0), (curve, (0.0, -4.0, 0.0), 0.0),
                         (straight_2, tuple(Vector((0.0, -4.0, 0.0)) + CURVE_END), CURVE_ANGLE),
                         (stop, tuple(Vector((0.0, -4.0, 0.0)) + CURVE_END + Matrix.Rotation(math.radians(CURVE_ANGLE), 3, 'Z') @ Vector((0.0, -2.0, 0.0))), CURVE_ANGLE)):
        copy = obj.copy()
        copy.name = '_Run_' + obj.name
        scene.collection.objects.link(copy)
        copy.location = at
        copy.rotation_euler = (0.0, 0.0, math.radians(rot))
        pieces.append(copy)
    car.location = (0.0, -1.6, 0.0)
    human.location = (1.6, -1.0, 0.0)
    bpy.context.view_layer.update()
    for o in (straight_4, straight_2, curve, stop):
        o.hide_render = True
    mtn.shot(scene, sun, pieces + [car], 'Tramway', view=(-0.8, -1.0, 0.55), lens=40.0, figure=human)
    car.location = (0.0, 0.0, 0.0)
    for p in pieces:
        bpy.data.objects.remove(p)
    for o in (straight_4, straight_2, curve, stop):
        o.hide_render = False
mtn.save_reports('Tramway')
