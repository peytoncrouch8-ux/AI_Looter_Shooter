"""The dredge barge's house dressed (round 2 of the rework, the coordinator 2026-11-04: "the house reads as a plain box and
the door is lost in it"): everything here stands OUTSIDE the house's walls (nothing inside: the kit's hall fills it, its
.Hull test traces z 2350 and 3100), and none of it over the arena (x -300..3300, |y| <= 1100) below z 2600 but the door's
frame and steps (x -400..-320). looter_tripo_barge.rework calls rail_copies() on Tripo's mesh before its cuts and
dress() with its other new faces; the numbers it shares (DECK, BEAM, HOUSE_AFT, HOUSE_FORE, ROOF, DOOR_W, DOOR_H) are
read off it when they run.

- Ribs (pilasters) RIB_W wide, RIB_PROUD proud up the outer walls from the deck to the roof's rim, the bays between them
  ~360 cm (the asked ~300 for the ribs and ~400 for the portholes meet there: one porthole centred in each bay); none
  over the door or the stern's machine (STERN_CLEAR).
- Portholes: a rim (PORT_R..PORT_R + PORT_RIM, PORT_PROUD proud) round dark glass (the Tripo cabin's window glass, the
  'glass' paint), centred at PORT_Z in every bay.
- The catwalk ring at WALK_Z, WALK_W wide, WALK_T thick, open at the stern where Tripo's machine stands against the aft
  wall, with Tripo's walk railing (cut from the midship block's roof walk, RAIL_SRC, before the clear) laid end to end
  along its outer edge, squeezed to RAIL_HIGH.
- The door: a frame FRAME_W wide, FRAME_PROUD proud round the opening, a lintel plate over it, two steps in front of it
  (STEP_H high, STEP_D deep each, their tops under DECK + 30; the kit walks through the door with W alone).
- Vents: VENTS, short mushroom stubs on the roof away from the stacks and the tower.
Paint (looter_tripo_rework.Paint): ribs, rims, frame, steps' risers, the catwalk's edges and the vents in 'hull', glass
in 'glass', the catwalk's and steps' tops in 'deck', the lintel plate in 'roof'.
"""
import math

import bmesh
from mathutils import Vector

import looter_tripo_barge as bg
from looter_tripo_rework import B, U, bisect, over, spanning, steep, ue_centre

RIB_BAYS = {'side': 8, 'end': 8}       # bays per side wall (2900 long) and per end wall (2475 wide): ~362 and ~309 cm
RIB_W, RIB_PROUD = 20.0, 15.0
PORT_Z, PORT_R, PORT_RIM, PORT_PROUD, PORT_SIDES = 2850.0, 40.0, 8.0, 4.0, 16
STERN_CLEAR = 320.0                   # |y| on the aft wall kept free: Tripo's stern machine (y -249..280 up to z 3010)
WALK_Z, WALK_W, WALK_T = 2950.0, 80.0, 15.0
RAIL_SRC_X, RAIL_SRC_Y, RAIL_SRC_Z = (1100.0, 1500.0), (870.0, 1010.0), (3230.0, 3420.0)   # Tripo's walk railing
RAIL_HIGH, RAIL_IN = 100.0, 35.0      # the railing's height on the catwalk; its line inside the catwalk's outer edge
FRAME_W, FRAME_PROUD = 30.0, 20.0
LINTEL_W, LINTEL_H, LINTEL_PROUD, LINTEL_GAP = 360.0, 40.0, 25.0, 10.0
STEP_H, STEP_D, STEP_HALF = 14.5, 40.0, 170.0
VENTS = ((-2850.0, 650.0), (-2850.0, -650.0), (-650.0, 550.0))
VENT_R, VENT_H, VENT_CAP_R, VENT_CAP_H, VENT_SIDES = 25.0, 110.0, 40.0, 12.0, 12


def log(message):
    print(f'LOOTER: DredgeBarge house: {message}', flush=True)


# --- Tripo's railing, re-used along the catwalk ---

def _source(bm, tag):
    """Cuts Tripo's walk railing out on both sides (RAIL_SRC_*); returns {side: faces} (still Tripo's, in place)."""
    x0, x1 = RAIL_SRC_X
    z0, z1 = RAIL_SRC_Z
    outer = lambda ps: max(abs(p[1]) for p in ps) > RAIL_SRC_Y[0] - 40.0
    bisect(bm, spanning(bm, tag, 0, 2, z0, lambda ps: over(0, x0, x1)(ps) and outer(ps) and steep(ps)), 2, z0)
    for x in (x0, x1):
        bisect(bm, spanning(bm, tag, 0, 0, x, lambda ps: min(p[2] for p in ps) > z0 - 1.0 and outer(ps)), 0, x)
    faces = {}
    for s in (-1, 1):
        faces[s] = [f for f in bm.faces if f[tag] == 0 and x0 < ue_centre(f)[0] < x1 and z0 < ue_centre(f)[2] < z1
                    and RAIL_SRC_Y[0] < s * ue_centre(f)[1] < RAIL_SRC_Y[1]]
    return faces


def _runs():
    """The catwalk's railing runs: (start (x, y), along (x, y), length, outward (x, y)) at the line RAIL_IN inside its
    outer edge."""
    xo_f, xo_a = bg.HOUSE_FORE + WALK_W - RAIL_IN, bg.HOUSE_AFT - WALK_W + RAIL_IN
    yo = bg.BEAM + WALK_W - RAIL_IN
    runs = [((xo_a, s * yo), (1.0, 0.0), xo_f - xo_a, (0.0, float(s))) for s in (-1, 1)]
    runs.append(((xo_f, -yo), (0.0, 1.0), 2 * yo, (1.0, 0.0)))
    for s in (-1, 1):
        runs.append(((xo_a, s * STERN_CLEAR), (0.0, float(s)), yo - STERN_CLEAR, (-1.0, 0.0)))
    return runs


def rail_copies(bm, tag, part):
    """Lays copies of Tripo's walk railing end to end along the catwalk's runs (tagged part); returns their face count."""
    src = _source(bm, tag)
    x0, x1 = RAIL_SRC_X
    z0, z1 = RAIL_SRC_Z
    seg = x1 - x0
    zs = RAIL_HIGH / (z1 - z0)
    line = {s: sum(s * U(v.co)[1] for f in src[s] for v in f.verts) / max(1, sum(len(f.verts) for f in src[s]))
            for s in (-1, 1)}
    made = 0
    for start, along, length, out in _runs():
        n = max(1, round(length / seg))
        squeeze = length / (n * seg)
        # The side runs take their own side's railing; the end runs the +Y one, turned so its outside faces out.
        s = int(out[1]) if out[1] else 1
        for i in range(n):
            faces = src[s]
            geom = list(faces) + list({e for f in faces for e in f.edges}) + list({v for f in faces for v in f.verts})
            dup = bmesh.ops.duplicate(bm, geom=geom)['geom']
            new_faces = [g for g in dup if isinstance(g, bmesh.types.BMFace)]
            for g in dup:
                if isinstance(g, bmesh.types.BMVert):
                    x, y, z = U(g.co)
                    u = ((x - x0) + i * seg) * squeeze
                    w = s * y - line[s]                      # outward of the railing's own line
                    g.co = B((start[0] + along[0] * u + out[0] * w, start[1] + along[1] * u + out[1] * w,
                              WALK_Z + (z - z0) * zs))
            # The source's frame (along +x, out s*y) and the run's: when one mirrors the other, the winding turns.
            if (along[0] * out[1] - along[1] * out[0]) * s < 0.0:
                bmesh.ops.reverse_faces(bm, faces=new_faces)
            for f in new_faces:
                f[tag] = part
                f.normal_update()
            made += len(new_faces)
    log(f'railing: Tripo\'s walk railing x {x0:.0f}..{x1:.0f}, z {z0:.0f}..{z1:.0f} ({len(src[1])} + {len(src[-1])} '
        f'faces), {made} faces laid along the catwalk, {RAIL_HIGH:.0f} high')
    return made


# --- The dressing ---

def dress(build):
    """Ribs, portholes, the catwalk, the door's frame, lintel plate and steps, the roof's vents. Returns the UCX boxes
    it wants (name, x0, x1, y0, y1, z0, z1)."""
    boxes = []
    _ribs_and_ports(build)
    boxes += _catwalk(build)
    boxes += _door(build)
    for x, y in VENTS:
        _vent(build, x, y)
    log(f'ribs, portholes, catwalk, door frame, lintel, steps, {len(VENTS)} vents')
    return boxes


def _wall_spots():
    """Per outer wall: (origin (x, y), along (x, y), length, outward (x, y), bays, skip(u) for a rib or porthole)."""
    dh = bg.DOOR_W / 2.0 + FRAME_W
    lw = LINTEL_W / 2.0
    lintel_top = bg.DECK + bg.DOOR_H + FRAME_W + LINTEL_GAP + LINTEL_H
    walls = []
    for s in (-1, 1):
        walls.append(((bg.HOUSE_AFT, s * bg.BEAM), (1.0, 0.0), bg.HOUSE_FORE - bg.HOUSE_AFT, (0.0, float(s)),
                      RIB_BAYS['side'], lambda u, z: False))
    # The fore face: no rib over the door; portholes over it stay clear of the lintel plate (they are higher).
    walls.append(((bg.HOUSE_FORE, -bg.BEAM), (0.0, 1.0), 2 * bg.BEAM, (1.0, 0.0), RIB_BAYS['end'],
                  lambda u, z: abs(u - bg.BEAM) < max(dh, lw) + RIB_W and z < lintel_top + 1.0))
    walls.append(((bg.HOUSE_AFT, -bg.BEAM), (0.0, 1.0), 2 * bg.BEAM, (-1.0, 0.0), RIB_BAYS['end'],
                  lambda u, z: abs(u - bg.BEAM) < STERN_CLEAR))
    return walls


def _ribs_and_ports(build):
    ribs = ports = 0
    for origin, along, length, out, bays, skip in _wall_spots():
        a, o = Vector((along[0], along[1], 0.0)), Vector((out[0], out[1], 0.0))
        base = Vector((origin[0], origin[1], 0.0))
        bay = length / bays
        for k in range(1, bays):
            u = bay * k
            if skip(u, bg.DECK):
                continue
            c = base + a * u
            _box(build, c - a * (RIB_W / 2.0), a, RIB_W, o, RIB_PROUD, bg.DECK, bg.ROOF + bg.COAMING_H, 'hull')
            ribs += 1
        for k in range(bays):
            u = bay * (k + 0.5)
            if skip(u, PORT_Z - PORT_R - PORT_RIM):
                continue
            _porthole(build, base + a * u + Vector((0.0, 0.0, PORT_Z)), a, o)
            ports += 1
    log(f'{ribs} ribs ({RIB_W:.0f} wide, {RIB_PROUD:.0f} proud), {ports} portholes (r {PORT_R:.0f}, z {PORT_Z:.0f})')


def _box(build, corner, a, la, o, lo, z0, z1, key, faces='front sides top bottom'):
    """A block standing out of a wall: corner on the wall, la along a, lo out along o, from z0 to z1."""
    up = Vector((0.0, 0.0, 1.0))
    c0 = Vector((corner[0], corner[1], z0))
    p = lambda u, w, z: c0 + a * u + o * w + up * (z - z0)
    quads = {'front': ([p(0, lo, z0), p(la, lo, z0), p(la, lo, z1), p(0, lo, z1)], o),
             'side0': ([p(0, 0, z0), p(0, lo, z0), p(0, lo, z1), p(0, 0, z1)], -a),
             'side1': ([p(la, 0, z0), p(la, lo, z0), p(la, lo, z1), p(la, 0, z1)], a),
             'top': ([p(0, 0, z1), p(la, 0, z1), p(la, lo, z1), p(0, lo, z1)], up),
             'bottom': ([p(0, 0, z0), p(la, 0, z0), p(la, lo, z0), p(0, lo, z0)], -up)}
    names = faces.replace('sides', 'side0 side1').split()
    for name in names:
        pts, n = quads[name]
        build.face([tuple(q) for q in pts], tuple(n), key)


def _porthole(build, centre, a, o):
    """A round porthole on a wall (centre on the wall's face): a rim PORT_PROUD proud round dark glass set 1 cm out."""
    up = Vector((0.0, 0.0, 1.0))
    ring = lambda r, w: [centre + (a * math.cos(2 * math.pi * i / PORT_SIDES) + up * math.sin(2 * math.pi * i / PORT_SIDES))
                         * r + o * w for i in range(PORT_SIDES)]
    r0, r1 = PORT_R, PORT_R + PORT_RIM
    back, front, inner, glass = ring(r1, 0.0), ring(r1, PORT_PROUD), ring(r0, PORT_PROUD), ring(r0, 1.0)
    for i in range(PORT_SIDES):
        j = (i + 1) % PORT_SIDES
        mid = (front[i] + front[j]) / 2.0 - centre - o * PORT_PROUD
        build.face([tuple(back[i]), tuple(back[j]), tuple(front[j]), tuple(front[i])], tuple(mid), 'hull')
        build.face([tuple(inner[i]), tuple(inner[j]), tuple(front[j]), tuple(front[i])], tuple(o), 'hull')
        build.face([tuple(glass[i]), tuple(glass[j]), tuple(inner[j]), tuple(inner[i])], tuple(-mid), 'hull')
    build.face([tuple(p) for p in glass], tuple(o), 'glass')


def _catwalk(build):
    """The ring at WALK_Z round the house (open at the stern machine): tops, undersides, outer edges and ends."""
    t, w = WALK_T, WALK_W
    z0, z1 = WALK_Z - t, WALK_Z
    xf, xa, yo = bg.HOUSE_FORE + w, bg.HOUSE_AFT - w, bg.BEAM + w
    boxes = []

    def slab(name, x0, x1, y0, y1, outer):
        build.rect((x0, y0, z1), (1, 0, 0), (0, 1, 0), x1 - x0, y1 - y0, (0, 0, 1), 'deck')
        build.rect((x0, y0, z0), (1, 0, 0), (0, 1, 0), x1 - x0, y1 - y0, (0, 0, -1), 'hull')
        for side in outer:
            if side == '+x':
                build.rect((x1, y0, z0), (0, 1, 0), (0, 0, 1), y1 - y0, t, (1, 0, 0), 'hull')
            elif side == '-x':
                build.rect((x0, y0, z0), (0, 1, 0), (0, 0, 1), y1 - y0, t, (-1, 0, 0), 'hull')
            elif side == '+y':
                build.rect((x0, y1, z0), (1, 0, 0), (0, 0, 1), x1 - x0, t, (0, 1, 0), 'hull')
            elif side == '-y':
                build.rect((x0, y0, z0), (1, 0, 0), (0, 0, 1), x1 - x0, t, (0, -1, 0), 'hull')
        boxes.append((name, x0, x1, y0, y1, z0, z1))

    for s in (-1, 1):
        y0, y1 = sorted((s * bg.BEAM, s * yo))
        slab(f'catwalk {"+Y" if s > 0 else "-Y"}', xa, xf, y0, y1, ('+x', '-x', '+y' if s > 0 else '-y'))
    slab('catwalk fore', bg.HOUSE_FORE, xf, -bg.BEAM, bg.BEAM, ('+x',))
    for s in (-1, 1):
        y0, y1 = sorted((s * STERN_CLEAR, s * bg.BEAM))
        slab(f'catwalk aft {"+Y" if s > 0 else "-Y"}', xa, bg.HOUSE_AFT, y0, y1, ('-x', '-y' if s > 0 else '+y'))
    # Its railing's collision (the railing itself is Tripo's, laid by rail_copies).
    rails = [(f'catwalk rail {"+Y" if s > 0 else "-Y"}', xa, xf, *sorted((s * (yo - 10.0), s * yo))) for s in (-1, 1)]
    rails.append(('catwalk rail fore', xf - 10.0, xf, -yo, yo))
    rails += [(f'catwalk rail aft {"+Y" if s > 0 else "-Y"}', xa, xa + 10.0, *sorted((s * STERN_CLEAR, s * yo)))
              for s in (-1, 1)]
    boxes += [(n, x0, x1, y0, y1, z1, z1 + RAIL_HIGH) for n, x0, x1, y0, y1 in rails]
    return boxes


def _door(build):
    """The door's frame (jambs and head), the lintel plate over it, two steps in front of the threshold."""
    x = bg.HOUSE_FORE
    dh, head = bg.DOOR_W / 2.0, bg.DECK + bg.DOOR_H
    a, o = Vector((0.0, 1.0, 0.0)), Vector((1.0, 0.0, 0.0))
    for y0 in (-dh - FRAME_W, dh):
        _box(build, (x, y0), a, FRAME_W, o, FRAME_PROUD, bg.DECK, head + FRAME_W, 'hull', 'front sides top')
    _box(build, (x, -dh), a, 2 * dh, o, FRAME_PROUD, head, head + FRAME_W, 'hull', 'front bottom top')
    lz = head + FRAME_W + LINTEL_GAP
    _box(build, (x, -LINTEL_W / 2.0), a, LINTEL_W, o, LINTEL_PROUD, lz, lz + LINTEL_H, 'roof')
    # Two steps down from the threshold's front to the deck: the upper one against the wall.
    boxes = [('door jamb -Y', x, x + FRAME_PROUD, -dh - FRAME_W, -dh, bg.DECK, head + FRAME_W),
             ('door jamb +Y', x, x + FRAME_PROUD, dh, dh + FRAME_W, bg.DECK, head + FRAME_W)]
    for k, top in enumerate((bg.DECK + 2 * STEP_H, bg.DECK + STEP_H)):
        x0, x1 = x + k * STEP_D, x + (k + 1) * STEP_D
        build.rect((x0, -STEP_HALF, top), (1, 0, 0), (0, 1, 0), STEP_D, 2 * STEP_HALF, (0, 0, 1), 'deck')
        build.rect((x1, -STEP_HALF, bg.DECK), (0, 1, 0), (0, 0, 1), 2 * STEP_HALF, top - bg.DECK, (1, 0, 0), 'hull')
        for s in (-1, 1):
            build.face([(x0, s * STEP_HALF, bg.DECK), (x1, s * STEP_HALF, bg.DECK), (x1, s * STEP_HALF, top),
                        (x0, s * STEP_HALF, top)], (0, s, 0), 'hull')
        boxes.append((f'door step {k + 1}', x0, x1, -STEP_HALF, STEP_HALF, bg.DECK, top))
    return boxes


def _vent(build, x, y):
    """A mushroom vent on the roof: a short pipe and a cap."""
    z0, z1 = bg.ROOF, bg.ROOF + VENT_H
    pts = lambda r, z: [(x + r * math.cos(2 * math.pi * i / VENT_SIDES), y + r * math.sin(2 * math.pi * i / VENT_SIDES), z)
                        for i in range(VENT_SIDES)]
    lo, hi = pts(VENT_R, z0), pts(VENT_R, z1)
    cap_lo, cap_hi = pts(VENT_CAP_R, z1), pts(VENT_CAP_R, z1 + VENT_CAP_H)
    for i in range(VENT_SIDES):
        j = (i + 1) % VENT_SIDES
        out = ((lo[i][0] + lo[j][0]) / 2.0 - x, (lo[i][1] + lo[j][1]) / 2.0 - y, 0.0)
        build.face([lo[i], lo[j], hi[j], hi[i]], out, 'hull')
        build.face([cap_lo[i], cap_lo[j], cap_hi[j], cap_hi[i]], out, 'hull')
        build.face([cap_lo[i], cap_lo[j], hi[j], hi[i]], (0.0, 0.0, -1.0), 'hull')
    build.face(cap_hi, (0.0, 0.0, 1.0), 'roof')
