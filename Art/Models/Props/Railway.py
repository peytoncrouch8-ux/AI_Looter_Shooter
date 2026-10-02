"""The railway at Ransom's Rest (zone "Undertaker's Yard and the Depot", Docs/Areas/RansomsRest.md): the track kit, the
buffer stop at the end of the line, the semaphore signal at Stage Gap's mouth, and the station board that hangs by the
depot's door. Small instanced props (Art/README.md): no Nanite, LODs at 50% and 25%; built with looter_buildings and
looter_rail.

The rail contract (shared with the train, Art/Models/Vehicles/Train.py, and the depot's platform):
- the ground under the track is z = 0 at a piece's origin, and the rail heads' tops are at z = 0.25 m;
- the gauge is 1.435 m between the rail heads' inner faces;
- a piece runs from its origin along -Y: a straight of length L ends at (0, -L, 0);
- the platform's walking surface is 0.40 m up, its edge 1.65 m from the track's centreline (Depot.py).

  Track_Straight_6m, Track_Straight_12m   straights; chain them end to origin, same rotation.
  Track_Curve      15 degrees of a 60 m radius (15.71 m along the centreline), turning toward +X: it ends at
                   (2.0445, -15.5291, 0), heading turned 15 degrees (the next piece is rotated 15 degrees about Z, toward
                   +X). Run backwards it turns the other way: rotate it 165 degrees (180 - 15) and put its END on the
                   chain point; the chain carries on from its origin, turned 15 degrees toward -X.
  BufferStop       a timber stop with an oxide-red buffer beam (coupler height, 0.85 m above the rail heads), iron
                   straps and striker, a red target disc and an earth mound behind. Its origin is the end of the line:
                   put it at the last piece's end with that piece's rotation (its beam stands over the last 1.1 m of
                   track and faces back up the line, +Y); at the start of a chain, turn it 180 degrees.
  Signal           a 6.2 m timber semaphore post: ladder, lamp man's platform, lamp (LanternGlow, SOCKET_Light) and
                   finial. SOCKET_Arm is the arm's spindle; its front (-Y) faces the train the signal speaks to.
  SignalArm        the arm on its own, pivot on the spindle: an oxide-red blade with a cream stripe toward +X and the
                   spectacle on the other side. 0 degrees is horizontal (stop); turned about its front axis so the tip
                   rises (+45 or +90 degrees about the socket's forward, counterclockwise seen from the front) it shows
                   clear. Spectacle openings sit in front of the lamp at 0 and 45 degrees.
  StationBoard     the departures board: a framed blackboard (boards painted black, WoodBlack) under an oxide-red
                   header reading DEPARTURES in Rye, with a chalk ledge; the rest is blank for the game's UI. Its origin is the middle of its back, where it
                   meets the wall (the depot's SOCKET_StationBoard); SOCKET_Interact is in front of the slate.

Track: creosote-dark ties every 0.6 m (WoodBlack, the shared painted-wood name, its grain showing; TIE_END_GRAIN = True
would put their sawn ends on WoodEndGrain, a fourth material slot), rails, tie plates, spikes and joint bars in
IronBlack (MetalRust is teal-painted steel and read as painted rails), a shallow bed of Ballast (cinders: the
GroundDirt set, greyed; it repeats a whole number of times on every piece, so joints don't show). Collision is one
low hull per piece over the ties and rails (three on the curve), flat at the rail heads between x = +-1.0 and sloping
to the ground at +-1.75, so a walking capsule steps across without snagging. The track past Stage Gap belongs to the
surround and has no collision: turn collision off on those instances.
"""
import math

from mathutils import Matrix, Vector

import looter_buildings as kit
import looter_rail as rail
from looter_buildings import Trim

lt = kit.lt

# --- The rail contract ---
GAUGE = 1.435                       # between the rail heads' inner faces
RAIL_TOP = 0.25                     # the rail heads' tops above the ground
RAIL_H = 0.13                       # the rail's height: its foot stands on the tie plates
HEAD_W = 0.07
RAIL_X = GAUGE * 0.5 + HEAD_W * 0.5  # each rail's centreline from the track's
PLATE = 0.014
TIE_TOP = RAIL_TOP - RAIL_H - PLATE  # 0.106
TIE = (2.6, 0.22, 0.16)              # length across the track, width along it, depth
TIE_PITCH = 0.6
BALLAST_TOP = 0.09
# The rail's section (across, up from its foot), round from the foot's left edge; the foot's underside isn't drawn.
RAIL_PROFILE = [(0.06, 0.0), (0.06, 0.012), (0.012, 0.03), (0.009, 0.085), (0.035, 0.092), (0.035, RAIL_H),
                (-0.035, RAIL_H), (-0.035, 0.092), (-0.009, 0.085), (-0.012, 0.03), (-0.06, 0.012), (-0.06, 0.0)]
# The ballast's section (across, up): toes buried a little, shoulders, a nearly flat top.
BALLAST = [(-2.12, -0.05), (-1.5, BALLAST_TOP), (-0.75, BALLAST_TOP + 0.004), (0.75, BALLAST_TOP + 0.004),
           (1.5, BALLAST_TOP), (2.12, -0.05)]
# Rails, plates, spikes, joint bars and the stop's ironwork: IronBlack (dark worn steel; MetalRust is teal-painted).
IRON_KEY, IRON_SET = 'ironblack', rail.IRON_SET
METAL_TILE = 3.2                    # the metal sets repeat every 3.2 m at 320 px/m
BALLAST_TILE = 3.0                  # the cinder bed repeats every 3 m: whole repeats on every piece (6, 12 and the curve)
CURVE_RADIUS, CURVE_ANGLE = 60.0, 15.0
TIE_END_GRAIN = False               # True: the ties' sawn ends on WoodEndGrain (a fourth material slot per piece)
TIE_MAT = 'woodblack'               # creosoted ties (and the buffer stop's timbers): WoodBlack


class Path:
    """A piece's centreline: a straight of `length` along -Y, or a curve of `radius` and `angle` (degrees) turning
    toward +X. frame(s) is the matrix at distance s along it: X across the track (toward +X at the start; toward the
    curve's centre), Y back toward the origin, Z up."""

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
        y = Vector((-x.y, x.x, 0.0))       # back along the track: X cross Y is up
        return kit.basis(self.point(s), x, y, Vector((0.0, 0.0, 1.0)))

    def at(self, s, u, z):
        """A point u across the track (toward +X at the start) and z up, at distance s."""
        return self.point(s) + self.across(s) * u + Vector((0.0, 0.0, z))


def _quad_faces(tb, rows, closed=False):
    """Faces between consecutive rows of vertices (each row a list, same length)."""
    for r0, r1 in zip(rows, rows[1:]):
        n = len(r0)
        for j in range(n if closed else n - 1):
            k = (j + 1) % n
            tb.faces.new((r0[j], r0[k], r1[k], r1[j]))


def rails(m, path, segments):
    """Both rails swept along the path in `segments` pieces, capped at both ends, mapped on IronBlack (U along the
    rail, V round its section). RAIL_PROFILE runs counterclockwise looking ahead along the track, so faces between
    a ring and the next one ahead face out."""
    perimeter = [0.0]
    for a, b in zip(RAIL_PROFILE, RAIL_PROFILE[1:]):
        perimeter.append(perimeter[-1] + math.hypot(b[0] - a[0], b[1] - a[1]))
    for side in (-1.0, 1.0):
        tb = kit._new_bmesh()
        uv = tb.loops.layers.uv.active
        rows, coords = [], []
        for i in range(segments + 1):
            s = path.length * i / segments
            rows.append([tb.verts.new(path.at(s, side * RAIL_X + px, RAIL_TOP - RAIL_H + pz)) for px, pz in RAIL_PROFILE])
            coords.append(s)
        _quad_faces(tb, rows)
        # Caps where the rail ends: the next piece's rail butts against it; at a run's end it shows.
        for row, s, sign in ((rows[0], 0.0, 1.0), (rows[-1], path.length, -1.0)):
            face = tb.faces.new(row)
            face.normal_update()
            if face.normal.dot(path.frame(s).to_3x3() @ Vector((0.0, sign, 0.0))) < 0.0:
                face.normal_flip()
        # UVs: U along the rail, V round the section; caps planar.
        index = {v: (i, j) for i, row in enumerate(rows) for j, v in enumerate(row)}
        for face in tb.faces:
            cap = len(face.verts) == len(RAIL_PROFILE)
            for loop in face.loops:
                i, j = index[loop.vert]
                px, pz = RAIL_PROFILE[j]
                if cap:
                    loop[uv].uv = (px / METAL_TILE, pz / METAL_TILE)
                else:
                    loop[uv].uv = (coords[i] / METAL_TILE, perimeter[j] / METAL_TILE)
        m.emit(rail.fill(tb), None, IRON_KEY)


def ballast(m, path, rows_count, rng, bumps=True):
    """The ballast bed under the ties: BALLAST's section swept along the path, a little lumpy except at the ends (so
    pieces meet exactly), capped at both ends; its UVs repeat a whole number of times along any piece."""
    tb = kit._new_bmesh()
    uv = tb.loops.layers.uv.active
    repeats = max(1, round(path.length / BALLAST_TILE))
    rows = []
    for i in range(rows_count + 1):
        s = path.length * i / rows_count
        row = []
        for j, (u, z) in enumerate(BALLAST):
            lump = 0.0
            if bumps and 0 < i < rows_count and 0 < j < len(BALLAST) - 1:
                lump = rng.uniform(-0.012, 0.01)
            row.append(tb.verts.new(path.at(s, u, z + lump)))
        rows.append(row)
    _quad_faces(tb, rows)
    for face in tb.faces:
        face.normal_update()
        if face.normal.z < 0.0:
            face.normal_flip()
    across = [0.0]
    for a, b in zip(BALLAST, BALLAST[1:]):
        across.append(across[-1] + math.hypot(b[0] - a[0], b[1] - a[1]))
    index = {v: (i, j) for i, row in enumerate(rows) for j, v in enumerate(row)}
    for face in tb.faces:
        for loop in face.loops:
            i, j = index[loop.vert]
            loop[uv].uv = ((across[j] - across[-1] * 0.5) / BALLAST_TILE, repeats * i / rows_count)
    # End caps (hidden where pieces meet; they close a run's end).
    for row, s in ((rows[0], 0.0), (rows[-1], path.length)):
        face = tb.faces.new(row)
        face.normal_update()
        center = face.calc_center_median()
        if face.normal.dot(center - path.point(path.length * 0.5)) < 0.0:
            face.normal_flip()
        for loop in face.loops:
            co = loop.vert.co
            loop[uv].uv = ((co - path.point(s)).dot(path.across(s)) / BALLAST_TILE, co.z / BALLAST_TILE)
    m.emit(rail.fill(tb), None, 'ballast')


def tie(m, matrix, rng, k=0):
    """A tie (WoodBlack, the grain across the track), and its two tie plates (IronBlack), each
    with a spike hooked over the rail's foot: outside the rails on even ties, inside on odd ones (a hint, cheaply), all
    placed by the tie's matrix."""
    lx, ly, lz = TIE
    m.emit(kit._box(TIE, drop=('-z', '-x', '+x')), rail.Planks(row=rng.randrange(rail.PLANK_ROWS)), TIE_MAT,
           matrix @ Matrix.Translation((0.0, 0.0, TIE_TOP - lz * 0.5)))
    for sign in (-1.0, 1.0):
        end = kit._new_bmesh()
        corners = [end.verts.new((sign * lx * 0.5, y, TIE_TOP + z)) for y, z in
                   ((-ly * 0.5, -lz), (ly * 0.5, -lz), (ly * 0.5, 0.0), (-ly * 0.5, 0.0))]
        face = end.faces.new(corners)
        face.normal_update()
        if face.normal.x * sign < 0.0:
            face.normal_flip()
        if TIE_END_GRAIN:
            m.emit(end, kit.EndGrain(), 'endgrain', matrix)
        else:
            m.emit(end, rail.Planks(), TIE_MAT, matrix)
    metal = kit.Tile(IRON_SET)
    for side in (-1.0, 1.0):
        x = side * RAIL_X
        m.emit(kit._box((0.2, 0.18, PLATE), drop=('-z',)), metal, IRON_KEY,
               matrix @ Matrix.Translation((x, 0.0, TIE_TOP + PLATE * 0.5)))
        dx = side * (0.072 if k % 2 == 0 else -0.072)
        m.emit(kit._box((0.032, 0.032, 0.024), drop=('-z',)), metal, IRON_KEY,
               matrix @ Matrix.Translation((x + dx, side * 0.045, TIE_TOP + PLATE + 0.012)))


def joint_bars(m, path):
    """Half joint bars on both sides of each rail at both ends: two pieces' halves make one bar at every joint."""
    metal = kit.Tile(IRON_SET)
    for s in (0.15, path.length - 0.15):
        frame = path.frame(s)
        for side in (-1.0, 1.0):
            for dx in (-0.017, 0.017):
                m.emit(kit._box((0.012, 0.3, 0.055)), metal, IRON_KEY,
                       frame @ Matrix.Translation((side * RAIL_X + dx, 0.0, RAIL_TOP - RAIL_H + 0.058)))


def track_hulls(m, path, count):
    """Low hulls over the ties and rails: flat at the rail heads between +-1.0 m, sloping to the ground at +-1.75 m."""
    section = [(-1.75, -0.05), (-1.0, RAIL_TOP), (1.0, RAIL_TOP), (1.75, -0.05)]
    for k in range(count):
        s0, s1 = path.length * k / count, path.length * (k + 1) / count
        m.hull_points([path.at(s, u, z) for s in (s0, s1) for u, z in section])


def track(name, seed, path, rail_segments, ballast_rows, hulls=1):
    m = rail.Model(name, seed=seed)
    rng = m.rng
    count = max(1, round(path.length / TIE_PITCH))
    pitch = path.length / count
    for k in range(count):
        s = pitch * (k + 0.5) + rng.uniform(-0.012, 0.012)
        jitter = Matrix.Translation((rng.uniform(-0.012, 0.012), 0.0, -rng.uniform(0.0, 0.006))) @ \
            Matrix.Rotation(math.radians(rng.uniform(-0.8, 0.8)), 4, 'Z')
        tie(m, path.frame(s) @ jitter, rng, k)
    m.section('ties')
    rails(m, path, rail_segments)
    joint_bars(m, path)
    m.section('rails')
    ballast(m, path, ballast_rows, rng)
    m.section('ballast')
    track_hulls(m, path, hulls)
    return m.finish(preview=False, ao=True, Nanite=0, LODs='50,25')


# --- The buffer stop ---

def buffer_stop():
    m = rail.Model('BufferStop', seed=611)
    rng = m.rng
    metal = kit.Tile(IRON_SET)
    BEAM_Y, BEAM_Z = 0.85, 1.1           # the beam's middle: over the last metre of track, at coupler height
    # The earth mound behind the end of the line: the track's bed rising into a rounded heap.
    tb = kit._new_bmesh()
    uv = tb.loops.layers.uv.active
    sections = [(0.0, 0.0, 0.0), (-0.45, 0.2, 0.08), (-0.9, 0.3, 0.12), (-1.35, 0.26, 0.0), (-1.8, 0.1, -0.3),
                (-2.15, -0.05, -0.75)]
    rows = []
    for y, lift, widen in sections:
        row = []
        for j, (u, z) in enumerate(BALLAST):
            w = u * (1.0 + widen * 0.25) if abs(u) > 1.0 else u * (1.0 + widen * 0.15)
            zz = z + lift if 0 < j < len(BALLAST) - 1 else z
            if y < -1.7 and 0 < j < len(BALLAST) - 1:
                zz = z + lift * 0.6
            row.append(tb.verts.new((w, y, zz)))
        rows.append(row)
    _quad_faces(tb, rows)
    for face in tb.faces:
        face.normal_update()
        if face.normal.z < 0.0:
            face.normal_flip()
    cap = tb.faces.new(rows[-1])
    cap.normal_update()
    if cap.normal.y > 0.0:
        cap.normal_flip()
    for face in tb.faces:
        for loop in face.loops:
            co = loop.vert.co
            loop[uv].uv = (co.x / BALLAST_TILE, (co.y + co.z * 0.5) / BALLAST_TILE)
    m.emit(rail.fill(tb), None, 'ballast')
    m.section('mound')
    # Posts outside the rails, the beam in front of them, raking struts back to sills on the mound.
    for side in (-1.0, 1.0):
        x = side * 1.02
        m.board((x, BEAM_Y - 0.3, -0.25), (x, BEAM_Y - 0.3, BEAM_Z + 0.24), 0.26, 0.26, face=(0.0, -1.0, 0.0),
                uv=rail.Planks(), mat=TIE_MAT)
        m.board((x, BEAM_Y - 0.42, BEAM_Z + 0.08), (x, -1.45, 0.24), 0.22, 0.2, face=(side, 0.0, 0.0),
                uv=rail.Planks(), mat=TIE_MAT)
        m.board((x, BEAM_Y - 0.1, 0.08), (x, -1.75, 0.2), 0.24, 0.18, face=(0.0, 0.0, 1.0), uv=rail.Planks(),
                mat=TIE_MAT)
        # Iron straps where the strut meets the post, and over the beam's ends.
        m.box((0.3, 0.06, 0.3), at=(x, BEAM_Y - 0.44, BEAM_Z + 0.02), rot=(0.0, 0.0, 0.0), uv=metal, mat=IRON_KEY)
        m.box((0.12, 0.36, 0.42), at=(side * 1.25, BEAM_Y, BEAM_Z), uv=metal, mat=IRON_KEY)
        for z in (BEAM_Z - 0.1, BEAM_Z + 0.1):
            m.box((0.05, 0.05, 0.05), at=(side * 0.7, BEAM_Y + 0.17, z), uv=metal, mat=IRON_KEY)
    m.box((2.9, 0.32, 0.38), at=(0.0, BEAM_Y, BEAM_Z), uv=kit.Tile('PaintWorn'), mat='oxide', bevel=0.015)
    m.box((0.5, 0.04, 0.3), at=(0.0, BEAM_Y + 0.18, BEAM_Z), uv=metal, mat=IRON_KEY, bevel=0.008)
    # A cross brace between the posts behind the beam.
    m.board((-1.0, BEAM_Y - 0.3, 0.35), (1.0, BEAM_Y - 0.3, BEAM_Z - 0.15), 0.16, 0.08, uv=rail.Planks(), mat=TIE_MAT)
    # The target: a red disc on a short iron stalk over the beam's middle, facing up the line.
    m.box((0.05, 0.05, 0.42), at=(0.0, BEAM_Y - 0.05, BEAM_Z + 0.4), uv=metal, mat=IRON_KEY)
    disc = kit._disc(0.26, 12, 0.0)
    m.emit(rail.fill(disc), kit.Tile('PaintWorn'), 'oxide', kit.place((0.0, BEAM_Y - 0.02, BEAM_Z + 0.72), (0.0, 0.0, 180.0)))
    back = kit._disc(0.26, 12, 0.0)
    m.emit(rail.fill(back), kit.Tile('PaintWorn'), 'oxide', kit.place((0.0, BEAM_Y - 0.035, BEAM_Z + 0.72)))
    # An iron rim round the disc.
    m.cylinder((0.0, BEAM_Y - 0.045, BEAM_Z + 0.72), (0.0, BEAM_Y - 0.005, BEAM_Z + 0.72), 0.275, sides=12, uv=metal,
               mat=IRON_KEY, caps=(False, False), face=(0.0, 0.0, 1.0))
    m.section('stop')
    m.hull((2.95, 0.7, BEAM_Z + 0.3), at=(0.0, BEAM_Y - 0.1, (BEAM_Z + 0.3) * 0.5))
    m.hull_points([(x, y, z) for x in (-2.0, 2.0) for y, z in ((0.0, 0.0), (-0.9, 0.42), (-2.1, -0.05))]
                  + [(x, y, -0.05) for x in (-2.0, 2.0) for y in (0.0,)])
    return m.finish(preview=False, Nanite=0, LODs='50,25')


# --- The semaphore signal ---

SIGNAL_TOP = 6.2
SPINDLE = (0.0, -0.22, 5.55)        # the arm's pivot, in front of the post near its top
LENS = (-0.36, 0.0)                  # where the spectacle's danger opening sits, from the spindle (across, up)


def signal():
    m = rail.Model('Signal', seed=621)
    rng = m.rng
    post_len = SIGNAL_TOP - 0.3

    def width(z):
        """The post's width at height z: 0.26 at its foot tapering to 0.17 at the top."""
        return 0.26 - 0.09 * min(max((z - 0.3) / post_len, 0.0), 1.0)

    kit.stone_stack(m, (0.0, 0.0, -0.12), (0.64, 0.64), 0.46, taper=0.12, rot=rng.uniform(-5.0, 5.0))

    def taper(co):
        k = width(co.x + post_len * 0.5 + 0.3) / 0.26
        return Vector((co.x, co.y * k, co.z * k))
    m.emit(kit._box((post_len, 0.26, 0.26), cuts=3), Trim('C', lane='each', width=0.26), 'trim',
           kit.place((0.0, 0.0, 0.3 + post_len * 0.5), (0.0, -90.0, 0.0)), shape=taper)
    # The finial: a cap board and a little pyramid in faded oxide paint.
    top_w = width(SIGNAL_TOP)
    m.box((top_w + 0.06, top_w + 0.06, 0.05), at=(0.0, 0.0, SIGNAL_TOP + 0.025), uv='C')
    m.cylinder((0.0, 0.0, SIGNAL_TOP + 0.05), (0.0, 0.0, SIGNAL_TOP + 0.27), (top_w * 0.5 + 0.02) * math.sqrt(2.0),
               0.004, sides=4, uv='H2', phase=-45.0, caps=(False, False))
    m.section('post')
    # The spindle's bearing on the post's front, a stub through it.
    sx_, sy_, sz_ = SPINDLE
    front = -width(sz_) * 0.5
    m.box((0.15, 0.07, 0.26), at=(0.0, front - 0.035, sz_), uv=rail.IRON, bevel=0.01)
    m.cylinder((0.0, front - 0.07, sz_), (0.0, sy_ + 0.03, sz_), 0.035, sides=6, uv=rail.IRON, cap_uv=rail.IRON,
               face=(0.0, 0.0, 1.0))
    # The lamp behind the spectacle: an iron case on a bracket from the post, a round lit lens, a vented top.
    lx, lz = LENS[0], sz_ + LENS[1]
    ly = sy_ + 0.17                    # the case's middle, behind the arm
    m.box((0.2, 0.2, 0.28), at=(lx, ly, lz), uv=rail.IRON, bevel=0.012)
    m.cylinder((lx, ly - 0.1, lz), (lx, ly - 0.13, lz), 0.085, sides=10, uv=rail.IRON, cap_uv=rail.IRON,
               face=(0.0, 0.0, 1.0), caps=(False, False))
    lens = kit._disc(0.075, 10, 0.0)
    m.emit(rail.fill(lens), Trim('H4', fit=True), 'glow', kit.place((lx, ly - 0.12, lz)))
    m.cylinder((lx, ly, lz + 0.14), (lx, ly, lz + 0.24), 0.05, sides=8, uv=rail.IRON, cap_uv=rail.IRON)
    m.cylinder((lx, ly, lz + 0.24), (lx, ly, lz + 0.3), 0.085, 0.02, sides=8, uv=rail.IRON, cap_uv=rail.IRON)
    m.box((lx + width(lz) * 0.5 + 0.1, 0.05, 0.05), at=((lx - width(lz) * 0.5 + 0.1) * 0.5 - 0.05, ly, lz - 0.17),
          uv=rail.IRON)
    m.socket('Light', (lx, ly - 0.13, lz))
    # The operating rod down the post's side to a lever at its foot.
    m.box((0.02, 0.02, sz_ - 1.25), at=(-width(3.0) * 0.5 - 0.05, -0.05, 1.05 + (sz_ - 1.25) * 0.5 - 0.1),
          uv=rail.IRON)
    m.box((0.06, 0.36, 0.05), at=(-width(1.0) * 0.5 - 0.05, -0.18, 0.95), rot=(18.0, 0.0, 0.0), uv=rail.IRON)
    m.box((0.08, 0.1, 0.16), at=(-width(1.0) * 0.5 - 0.05, -0.02, 0.95), uv=rail.IRON)
    m.section('spindle, lamp, rod')
    # A ladder up the back to a lamp man's platform under the lamp.
    lad_y = width(1.0) * 0.5 + 0.17
    for x in (-0.2, 0.2):
        m.board((x, lad_y, 0.0), (x, lad_y, 5.05), 0.06, 0.045, face=(0.0, 1.0, 0.0), uv=Trim('C', lane='each'))
    for z in kit.frange(0.3, 4.9, 0.3):
        m.box((0.4, 0.028, 0.028), at=(0.0, lad_y, z), uv=rail.IRON, drop=('-x', '+x'))
    for z in (1.6, 3.4):
        for x in (-0.2, 0.2):
            m.board((x, lad_y - 0.02, z), (x * 0.4, width(z) * 0.5 - 0.01, z), 0.04, 0.04, face=(1.0, 0.0, 0.0),
                    uv=rail.IRON)
    PLAT = 4.85
    for k in range(4):
        x = -0.62 + 0.15 * (k + 0.5)
        m.board((x, -0.05, PLAT), (x, lad_y + 0.05, PLAT), 0.14, 0.04, face=(0.0, 0.0, 1.0), uv='A')
    m.board((-0.64, lad_y, PLAT - 0.06), (-width(PLAT) * 0.5, lad_y, PLAT - 0.06), 0.08, 0.06, face=(0.0, 1.0, 0.0),
            uv='C')
    m.board((-0.6, -0.02, PLAT - 0.06), (-width(PLAT) * 0.5, -0.02, PLAT - 0.06), 0.08, 0.06, uv='C')
    m.board((-0.12, 0.1, PLAT - 0.6), (-0.55, 0.1, PLAT - 0.05), 0.07, 0.06, face=(0.0, 1.0, 0.0), uv='C')
    m.section('ladder, platform')
    m.hull((0.66, 0.66, 0.4), at=(0.0, 0.0, 0.1))
    m.hull((0.28, 0.28, SIGNAL_TOP - 0.3), at=(0.0, 0.0, 0.3 + (SIGNAL_TOP - 0.3) * 0.5))
    m.socket('Arm', SPINDLE)
    return m.finish(preview=False, Nanite=0, LODs='50,25')


def signal_arm():
    """The arm round its spindle (the origin), in the XZ plane: blade toward +X, spectacle toward -X."""
    m = rail.Model('SignalArm', seed=631)
    paint = kit.Tile('PaintWorn')
    # The blade: oxide red with a cream band near its square end, on a narrower root from the hub.
    for x0, x1, mat in ((0.17, 1.0, 'oxide'), (1.0, 1.12, 'cream'), (1.12, 1.42, 'oxide')):
        h0 = 0.25 - 0.03 * x0 / 1.42
        h1 = 0.25 - 0.03 * x1 / 1.42

        def taper(co, x0=x0, x1=x1, h0=h0, h1=h1):
            t = (co.x - (-(x1 - x0) * 0.5)) / (x1 - x0)
            return Vector((co.x, co.y, co.z * (h0 + (h1 - h0) * t) / 0.25))
        m.emit(kit._box((x1 - x0, 0.03, 0.25)), paint, mat, kit.place(((x0 + x1) * 0.5, 0.0, 0.0)), shape=taper)
    m.box((0.14, 0.035, 0.12), at=(0.1, 0.0, 0.0), uv=paint, mat='oxide')
    # The spectacle: a casting with the danger and clear openings, round the spindle from the lamp's side.
    def circle(cx, cz, r, n=10):
        return [(cx + r * math.cos(-2.0 * math.pi * k / n), cz + r * math.sin(-2.0 * math.pi * k / n)) for k in range(n)]
    lens_b = (LENS[0] * math.cos(math.radians(-45.0)) - LENS[1] * math.sin(math.radians(-45.0)),
              LENS[0] * math.sin(math.radians(-45.0)) + LENS[1] * math.cos(math.radians(-45.0)))
    outline = [(-0.05, -0.08), (-0.47, -0.13), (-0.5, 0.08), (-0.36, 0.36), (-0.16, 0.42), (-0.03, 0.08)]
    plate = kit._prism([outline, circle(LENS[0], LENS[1], 0.085), circle(lens_b[0], lens_b[1], 0.085)], 0.025)
    m.emit(plate, paint, 'oxide', kit.place((0.0, -0.0125, 0.0)))
    m.cylinder((0.0, -0.035, 0.0), (0.0, 0.035, 0.0), 0.065, sides=10, uv=paint, cap_uv=paint, mat='oxide',
               face=(0.0, 0.0, 1.0))
    return m.finish(ground=False, preview=False, Nanite=0, LODs='50,25', Collision='None')


# --- The station board ---

def station_board():
    m = rail.Model('StationBoard', seed=641)
    # Battens against the wall, the slate, a frame round it, the header with its lettering, a chalk ledge.
    for x in (-0.42, 0.42):
        m.board((x, -0.0125, -0.56), (x, -0.0125, 0.52), 0.07, 0.025, uv='C')
    # The slate: boards painted black (WoodBlack: on a panel this size PaintWorn's chips read as spots).
    m.box((1.1, 0.02, 0.78), at=(0.0, -0.035, -0.12), uv=kit.Tile('WoodPlanks'), mat='woodblack')
    for z in (0.305, -0.545):
        m.board((-0.62, -0.05, z), (0.62, -0.05, z), 0.07, 0.04, uv=Trim('C', lane=1))
    for x in (-0.585, 0.585):
        m.board((x, -0.05, -0.51), (x, -0.05, 0.27), 0.07, 0.04, uv=Trim('C', lane=2))
    m.board((-0.65, -0.0425, 0.45), (0.65, -0.0425, 0.45), 0.2, 0.035, uv='H2')
    m.board((-0.68, -0.055, 0.57), (0.68, -0.055, 0.57), 0.04, 0.07, face=(0.0, 0.0, 1.0), uv=Trim('C', lane=0))
    rail.lettering(m, 'DEPARTURES', 0.105, at=(0.0, -0.0635, 0.45), width=1.16, simplify=30.0)
    m.board((-0.6, -0.085, -0.6), (0.6, -0.085, -0.6), 0.075, 0.03, face=(0.0, 0.0, 1.0), uv=Trim('C', lane=3))
    for x in (-0.5, 0.5):
        m.board((x, -0.05, -0.615), (x, -0.11, -0.66), 0.03, 0.025, face=(1.0, 0.0, 0.0), uv='C')
    m.box((0.085, 0.016, 0.016), at=(0.33, -0.095, -0.577), rot=(0.0, 0.0, 12.0), uv=kit.Tile('PaintWorn'), mat='cream')
    m.hull((1.36, 0.1, 1.24), at=(0.0, -0.05, -0.02))
    m.socket('Interact', (0.0, -0.12, -0.1))
    return m.finish(ground=False, preview=False, Nanite=0, LODs='50,25')


# --- Chaining (the level's builder does the same) ---

CURVE_END = Path(radius=CURVE_RADIUS, angle=CURVE_ANGLE).point(CURVE_RADIUS * math.radians(CURVE_ANGLE))


def chain(steps, start=(0.0, 0.0, 0.0), heading=-90.0, name='Chain'):
    """Lays copies of the pieces end to end, the way the level does: steps are (object, how), how being 'ahead' (a
    straight, a curve turning toward +X of the direction of travel, or the stop at the end) or 'back' (a curve run
    backwards: it turns the other way). heading is the direction of travel in degrees from +X (-90 is -Y, a piece's
    own). Returns the copies, the end point and the heading there."""
    import bpy
    q, h = Vector(start), heading
    copies = []
    for k, (obj, how) in enumerate(steps):
        copy = obj.copy()
        copy.name = f'{name}_{k}_{obj.name}'
        bpy.context.scene.collection.objects.link(copy)
        is_curve = obj.name == 'Track_Curve'
        if obj.name.startswith('Track_Straight'):
            end = Vector((0.0, -float(obj.name.split('_')[-1].rstrip('m')), 0.0))
        else:
            end = CURVE_END if is_curve else Vector((0.0, 0.0, 0.0))
        if how == 'back':
            alpha = h - (90.0 + CURVE_ANGLE)
            turn = Matrix.Rotation(math.radians(alpha), 3, 'Z')
            copy.location = q - turn @ end
            copy.rotation_euler = (0.0, 0.0, math.radians(alpha))
            q, h = copy.location.copy(), h - CURVE_ANGLE
        else:
            alpha = h + 90.0
            turn = Matrix.Rotation(math.radians(alpha), 3, 'Z')
            copy.location = q
            copy.rotation_euler = (0.0, 0.0, math.radians(alpha))
            q, h = q + turn @ end, h + (CURVE_ANGLE if is_curve else 0.0)
        copies.append(copy)
    bpy.context.view_layer.update()      # so their world matrices (and a preview's framing) follow
    return copies, q, h


# --- Build ---

straight_6 = track('Track_Straight_6m', 501, Path(length=6.0), 2, 5)
straight_12 = track('Track_Straight_12m', 502, Path(length=12.0), 4, 10)
curve = track('Track_Curve', 503, Path(radius=CURVE_RADIUS, angle=CURVE_ANGLE), 8, 12, hulls=3)
stop = buffer_stop()
signal_post = signal()
arm = signal_arm()
board = station_board()

if lt.want_preview() or __name__ in ('__station__',):
    arm.location = SPINDLE           # shown on its post (models export from their own origins)
if lt.want_preview():
    import bpy
    lt.preview([signal_post, arm], rail.preview_path('Depot', 'Signal'), view=(-0.9, -1.0, 0.25), fit=0.95)
    arm.location = (0.0, 0.0, 0.0)
    lt.preview([arm], rail.preview_path('Depot', 'SignalArm'), view=(-0.3, -1.0, 0.15), fit=0.9)
    lt.preview([board], rail.preview_path('Depot', 'StationBoard'), view=(-0.45, -1.0, 0.12), fit=0.9)
    shots = [(straight_12, (-0.55, -1.0, 0.45), 0.55), (straight_6, (-0.6, -1.0, 0.5), 0.75),
             (curve, (-0.35, -0.6, 1.0), 0.8), (stop, (-0.8, 1.0, 0.45), 0.85)]
    for obj, view, fit in shots:
        lt.preview([obj], rail.preview_path('Depot', obj.name), view=view, fit=fit)
    # A run: 12 m, a curve, 6 m, the curve run backwards (an S-bend), 12 m, the buffer stop at the end.
    run, end, heading = chain([(straight_12, 'ahead'), (curve, 'ahead'), (straight_6, 'ahead'), (curve, 'back'),
                               (straight_12, 'ahead'), (stop, 'ahead')], name='Run')
    lt.preview(run, rail.preview_path('Depot', 'Track_Run'), view=(-0.45, -0.25, 1.0), fit=0.95)
    lt.preview([run[4], run[5]], rail.preview_path('Depot', 'Track_Run_Stop'), view=(0.55, 1.0, 0.38), fit=0.7)
    for o in run:
        bpy.data.objects.remove(o)
