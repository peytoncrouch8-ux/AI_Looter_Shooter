"""Cliff swallows and their mud nests for Ransom's Rest's cliffs (the 2026-10-14 "make it alive" polish; look sheet 44):
small fast birds that loop out from gourd-shaped mud nests under the overhangs and dart back in. They are pieces the fauna
draws as instances and moves in code (World/FaunaFlock, as AmbientFauna.py's crows): no skeletons, each piece turning
about its own pivot, a whole flock one draw per piece. Stylized realism in the game's textured look: everything on one
painted atlas, SwallowAtlas (Art/Textures/SwallowAtlas, 1024 px, painted by this script every run, deterministic), on
the World master through MI_SwallowAtlas.

A cliff swallow, not the fauna's barn swallow (Swallow_Flying over the water): a square tail, not a fork; a pale buff
rump; a dark cap with a pale forehead; a chestnut face over a dark throat; steel-blue back with pale streaks; a buff
breast and a pale belly. Refined the same day ("slightly more realistic"): a cliff swallow's true size (15 cm, 30 cm
across the wings), a smaller head, a smooth trailing edge with soft feather tips, its small feathers painted lying in
rows (la.plumage), and the nests' mud as rounded pellets instead of crazed tiles.

Models (SM_<name> in /Game/Art/Creatures; no Nanite, no collision; the front faces Blender -Y, Unreal +X):
  CliffSwallow_Flying       the body, head and square tail, its pivot at the body's middle; SOCKET_WingL, SOCKET_WingR
                            at the shoulders. 15 cm bill to tail.
  CliffSwallow_WingInnerL/R the arm of each wing, pivoting at the shoulder; SOCKET_Wrist.
  CliffSwallow_WingOuterL/R the long pointed hand, pivoting at the wrist (sweep it back for a dive, spread it to glide).
                            30 cm across, spread.
  CliffSwallow_Perched      clinging upright at a nest's mouth, wings folded over the tail, its feet gripping the rim at
                            the pivot; SOCKET_Head at the neck. (FaunaFlock's PerchedMesh.)
  CliffSwallow_Head         the head, beak and eyes, its pivot at the neck joint (on SOCKET_Head).
  SwallowNests              four gourds of dried mud pellets in a cluster, each with a short entrance spout, to fix under
                            an overhang: its pivot is the back top edge's middle (where wall meets overhang), the nests
                            hanging below it and out from the wall toward the front; SOCKET_Nest1..4 at the mouths, each
                            facing out of its hole (a swallow leaving flies along the socket's front; one clinging sits on
                            it; a head peeks out from just inside it). 60 cm wide.

    Tools\\artrun.ps1 -Script Art\\Models\\Creatures\\Swallow.py -Preview      (renders into Saved/ArtPreviews/RansomsRest/Animals)
    -ScriptArgs previews=<folder> renders into a folder of Animals (the refined pass's are in Animals/Refined)
"""
import math

import numpy as np
from mathutils import Matrix, Vector

import looter_animals as la
import looter_creatures as lc
import looter_model as lm
import looter_textures as lt

ARGV = la.args()
PREVIEW = '--preview' in ARGV or lt.want_preview()
SET = 'SwallowAtlas'
SIZE = (1024, 1024)
PX_M, NORMAL_STRENGTH = 0.0005, 1.0
ATLAS = {
    'body': (0.0, 0.5, 0.75, 1.0), 'head': (0.5, 0.75, 0.75, 1.0), 'wing': (0.0, 0.5, 0.5, 0.75),
    'tail': (0.5, 0.75, 0.5, 0.75), 'beak': (0.75, 0.8125, 0.9375, 1.0), 'eye': (0.8125, 0.875, 0.9375, 1.0),
    'leg': (0.875, 0.9375, 0.9375, 1.0), 'hole': (0.9375, 1.0, 0.9375, 1.0), 'folded': (0.75, 1.0, 0.75, 0.875),
    'mud': (0.0, 0.5, 0.0, 0.5), 'mud_rim': (0.5, 1.0, 0.0, 0.25), 'wing_under': (0.5, 1.0, 0.25, 0.5),
}


# --- The atlas ---

rgb = lt.rgb


def paint(region, U, V, k):
    shape = U.shape
    seed = 700 + k
    n = lt.noise(shape, seed, 4.0, 4.0, octaves=2)
    if region in ('body', 'head'):
        # Small feathers lying in overlapping rows from the front back (la.plumage), each with its own tone: glossy
        # steel-blue tips on the back, soft buff and white below.
        d, s = lc.body_axes(U, V)
        rows, cols = (17.0, 20.0) if region == 'body' else (13.0, 16.0)
        q = (1.0 - V) * rows + 0.2 * lt.noise(shape, seed + 21, 20.0, 6.0)
        f = la.plumage(q, U, lambda i: np.full(np.shape(i), cols, np.float32), length=1.8, width=0.66, seed=seed)
        t, x, h = f['t'], f['x'], f['h']
        shadow = 1.0 - lt.smooth(0.0, 0.4, f['over'])
        fringe = lt.smooth(-0.14, 0.0, f['margin']) * lt.smooth(0.45, 0.8, t)
        lines, split = la.barbs(t, x, f['row'], f['col'], seed + 5, count=16.0)
        back = lt.mix(rgb(0x18202e), rgb(0x263652), lt.smooth(0.3, 0.95, t) * (0.5 + 0.5 * h))
        belly = lt.mix(rgb(0xe6ddcc), rgb(0xbca88c), lt.smooth(-0.9, -0.2, d))
        top = lt.smooth(-0.15, 0.25, d + 0.12 * (t - 0.5))
        c = lt.mix(belly, back, top)
        if region == 'body':
            # Pale streaks down the back (whitish feather edges), the buff rump patch above the tail, a buff breast
            # with a darker wash at its sides.
            streak = lt.smooth(0.35, 0.8, d) * lt.smooth(0.35, 0.65, V) * (np.abs(s) > 0.25) * (h > 0.55)
            c = lt.mix(c, rgb(0x9a98a0), streak * fringe * 0.8)
            rump = lt.smooth(0.42, 0.34, V) * lt.smooth(0.12, 0.2, V) * lt.smooth(-0.1, 0.3, d)
            c = lt.mix(c, lt.mix(rgb(0xc89c70), rgb(0xdcb68a), t), rump)
            c = lt.mix(c, rgb(0xb59c7c), lt.smooth(0.65, 0.85, V) * lt.smooth(0.0, -0.4, d) * 0.7)
            c = lt.mix(c, rgb(0x8a7a68), lt.smooth(0.4, 0.75, V) * lt.smooth(-0.1, -0.35, d) * lt.smooth(-0.75, -0.45, d) * 0.4)
        else:
            # The pale forehead patch, the chestnut cheeks and throat, a dark patch low on the throat.
            fore = lt.smooth(0.8, 0.9, V) * lt.smooth(0.55, 0.85, d)
            c = lt.mix(c, lt.mix(rgb(0xd8c49e), rgb(0xeadcbc), t), fore)
            cheek = lt.smooth(0.35, 0.6, V) * lt.smooth(0.4, -0.1, d) * lt.smooth(-0.95, -0.4, d)
            c = lt.mix(c, lt.mix(rgb(0x5e2616), rgb(0x7a3420), h), cheek)
            throat = lt.smooth(0.3, 0.5, V) * lt.smooth(-0.75, -0.95, d)
            c = lt.mix(c, rgb(0x2a1a18), throat * 0.85)
        c = c * (1.0 + 0.12 * (h - 0.5) + 0.06 * fringe + 0.04 * lines - 0.1 * split)[..., None]
        c = c * (1.0 - 0.22 * shadow)[..., None]
        # The blue back glossy, its feathers' tips most; the pale underparts soft.
        rough = 0.62 - 0.2 * top * lt.smooth(0.4, 0.95, t) + 0.12 * lt.smooth(0.0, -0.5, d)
        return c, rough, 0.00035 * f['height'], 1.0 - 0.2 * shadow - 0.1 * lt.smooth(-0.3, -0.9, d)
    if region in ('wing', 'wing_under', 'tail', 'folded'):
        count = {'wing': 12, 'wing_under': 12, 'tail': 10, 'folded': 9}[region]
        col = U * count
        fid = np.floor(col)
        fx = (la.frac(col) - 0.5) * 2.0                      # across each feather: -1 .. 1
        line = 1.0 - lt.smooth(0.0, 0.06, np.minimum(la.frac(col), 1.0 - la.frac(col)))
        tone = la.hash01(fid, 3.0 + k) - 0.5
        lines = la.line_noise(V * 40.0 - np.abs(fx) * 9.0 + fid * 31.0, seed + 3)
        rachis = 1.0 - lt.smooth(0.03, 0.08, np.abs(fx))
        if region == 'wing_under':
            # Seen from below: grey-brown flight feathers, pale-edged, the coverts buff-grey.
            c = lt.mix(rgb(0x6a6460), rgb(0x8c8478), lt.smooth(0.3, 0.9, V) * 0.4)
            c = lt.mix(c, rgb(0xa8a092), lt.smooth(0.3, 0.2, V))
            edge = lt.smooth(0.55, 0.9, np.abs(fx))
            c = lt.mix(c, rgb(0xb5ada0), edge * 0.3)
            c = lt.mix(c, rgb(0xc4bcb0), rachis * 0.3)
        else:
            c = lt.mix(rgb(0x1a1e2a), rgb(0x283044), lt.smooth(0.2, 0.0, V) * 0.6)
            # Blue gloss down each vane; each feather's outer edge a shade paler, so the hand reads as feathers, not
            # a board.
            c = lt.mix(c, rgb(0x2c3a58), (1.0 - lt.smooth(0.2, 0.8, np.abs(fx))) * 0.35)
            edge = lt.smooth(0.6, 0.92, np.abs(fx)) * lt.smooth(0.3, 0.6, V)
            c = lt.mix(c, rgb(0x464c62), edge * 0.4)
        if region == 'wing':
            coverts = lt.smooth(0.34, 0.26, V)
            c = lt.mix(c, rgb(0x24324e), coverts * 0.6)
            line = line * (1.0 - coverts)
        c = lt.mix(c, c * 0.55, line * 0.4) * (1.0 + 0.08 * tone + 0.04 * lines + 0.03 * n)[..., None]
        rough = 0.5 + 0.1 * line + 0.06 * lines
        return c, rough, 0.00012 * (1.0 - line) + 0.00012 * rachis + 0.00003 * lines, np.ones(shape, np.float32)
    solids = {'beak': (0x18171a, 0.4), 'eye': (0x0b0a09, 0.08), 'leg': (0x2a2522, 0.6), 'hole': (0x0c0a08, 0.95)}
    if region in solids:
        color, rough = solids[region]
        return np.broadcast_to(rgb(color), shape + (3,)).copy(), np.full(shape, rough, np.float32), \
            np.zeros(shape, np.float32), np.ones(shape, np.float32) * (0.4 if region == 'hole' else 1.0)
    if region in ('mud', 'mud_rim'):
        # Pellets of dried mud, each a rounded lump pressed onto the last (a swallow carries them one beakful at a
        # time), each its own mud: paler and drier, or darker where newer; soft hollows between them, not cracks; a
        # few straw ends.
        # Pellets about 1 cm across on the model: the gourd's V (pole to pole) holds half as many cm per pixel as its U
        # (round it), and the spout's U (along it) far more, so the cells are counted and measured for each.
        cells_n, stretch, size = ((46, 24), (1.0, 0.49), 11.1) if region == 'mud' else ((6, 16), (1.0, 5.8), 85.0)
        cells = lt.worley(shape, cells_n, seed + 3, jitter=0.95, stretch=stretch,
                          x=lt.coords(shape)[0] + 3.0 * lt.noise(shape, seed + 6, 8.0, 8.0),
                          y=lt.coords(shape)[1] + 3.0 * lt.noise(shape, seed + 7, 8.0, 8.0))
        r = np.clip(cells['f1'] / (size * 0.62), 0.0, 1.0)
        lump = np.sqrt(np.clip(1.0 - r * r, 0.0, 1.0))
        seam = 1.0 - lt.smooth(0.0, 2.2, cells['edge'])
        tone = lt.per_cell(cells['id'], cells['count'], seed + 5)
        c = lt.mix(rgb(0x8a7254), rgb(0xa88c68), tone)
        c = lt.mix(c, rgb(0x6a553e), (tone > 0.86) * 0.45)
        c = lt.mix(c, rgb(0x6a5640), seam * 0.12)
        c = c * (1.0 + 0.1 * lump - 0.05)[..., None]
        grit = lt.noise(shape, seed + 8, 1.0, 1.0)
        straw = (1.0 - lt.smooth(0.1, 0.35, np.abs(lt.noise(shape, seed + 9, 0.6, 9.0)))) * (lt.noise(shape, seed + 10, 12.0, 12.0) > 1.2)
        c = lt.mix(c, rgb(0xc4a874), straw * 0.7)
        c = c * (1.0 + 0.05 * n + 0.05 * grit)[..., None]
        height = 0.0009 * lump + 0.00005 * grit
        return c, 0.9 + 0.04 * grit, height, 1.0 - 0.12 * seam - 0.12 * (1.0 - lump)
    raise ValueError(region)


# --- Shapes (creature space: cm, x forward, y left, z up) ---

def finish(part, material, name):
    obj = part.finish(material, sharp=70.0)
    obj.name = name
    obj.data.name = name
    obj['Nanite'] = 0
    obj['Collision'] = 'None'
    la.log(f'{name}: {la.tris(obj)} triangles')
    return obj


def socket(obj, name, point, front=None):
    """A socket at a creature-space point (cm); front: the creature-space direction its front (Unreal +X) faces."""
    s = lm.socket(obj, name, tuple(lc.to_blender(point)))
    if front is not None:
        f = lc.TO_BLENDER.to_3x3() @ Vector(front)
        # A socket's front is the empty's -Y and its top +Z.
        s.rotation_euler = (-f).to_track_quat('Y', 'Z').to_euler()
    return s


def panel(part, region, stations, side, thickness=0.3, u_range=(0.0, 1.0), serrate=0.4, origin=(0.0, 0.0, 0.0),
          under='wing_under'):
    """A wing as a thin two-sided panel out along the side's span (side 1: out to +y, the bird's left). stations:
    [(s, lead, trail, z)]: how far out (cm), the leading and trailing edges' x, and the height. U along the span, V back
    across the chord; the top swells by thickness at the leading edge; the trailing edge is cut into feather tips. The
    underside takes the region `under` (paler, as a swallow's wing is from below)."""
    ox, oy, oz = origin
    span = max(st[0] for st in stations) or 1.0
    chords = (0.0, 0.3, 1.0)
    top, bottom = [], []
    for s, lead, trail, z in stations:
        rt, rb = [], []
        for j in chords:
            x = lead + (trail - lead) * j
            t = thickness * (1.0 - j) ** 1.5
            rt.append(part.vert((ox + x, oy + side * s, oz + z + 0.6 * t + 0.02)))
            rb.append(part.vert((ox + x, oy + side * s, oz + z - 0.4 * t - 0.02)))
        top.append(rt)
        bottom.append(rb)

    def uv(si, j, dv=0.0, du=0.0, reg=None):
        return part.at(reg or region, u_range[0] + (u_range[1] - u_range[0]) * stations[si][0] / span + du,
                       0.9 * chords[j] + dv)
    for i in range(len(stations) - 1):
        for k in range(len(chords) - 1):
            part.face([top[i][k], top[i + 1][k], top[i + 1][k + 1], top[i][k + 1]],
                      [uv(i, k), uv(i + 1, k), uv(i + 1, k + 1), uv(i, k + 1)], out=(0, 0, 1))
            part.face([bottom[i][k], bottom[i + 1][k], bottom[i + 1][k + 1], bottom[i][k + 1]],
                      [uv(i, k, reg=under), uv(i + 1, k, reg=under), uv(i + 1, k + 1, reg=under), uv(i, k + 1, reg=under)],
                      out=(0, 0, -1))
        part.face([top[i][0], top[i + 1][0], bottom[i + 1][0], bottom[i][0]],
                  [uv(i, 0), uv(i + 1, 0), uv(i + 1, 0, 0.03), uv(i, 0, 0.03)], out=(1, 0, 0))
        if serrate > 0.0:
            # A feather's tip behind the trailing edge between each pair of stations, the outer ones longer.
            reach = serrate * (0.6 + 0.8 * stations[i + 1][0] / span)
            mid = (top[i][-1].co + top[i + 1][-1].co) * 0.5 + Vector((-reach, 0.0, 0.0))
            tt = part.vert(mid + Vector((0.0, 0.0, 0.01)))
            tb = part.vert(mid - Vector((0.0, 0.0, 0.01)))
            u_mid = u_range[0] + (u_range[1] - u_range[0]) * (stations[i][0] + stations[i + 1][0]) * 0.5 / span
            part.face([top[i][-1], top[i + 1][-1], tt], [uv(i, 2), uv(i + 1, 2), part.at(region, u_mid, 1.0)], out=(0, 0, 1))
            part.face([bottom[i][-1], bottom[i + 1][-1], tb], [uv(i, 2, reg=under), uv(i + 1, 2, reg=under),
                                                              part.at(under, u_mid, 1.0)], out=(0, 0, -1))
    last = len(stations) - 1
    for k in range(len(chords) - 1):
        part.face([top[-1][k], top[-1][k + 1], bottom[-1][k + 1], bottom[-1][k]],
                  [uv(last, k), uv(last, k + 1), uv(last, k + 1, du=-0.02), uv(last, k, du=-0.02)], out=(0, side, 0))


def fan(part, region, root_x, tip_x, root_w, tip_w, z0, z1, notch=0.0, thickness=0.3, steps=3, tilt=0.0):
    """A tail: a thin two-sided fan from root_x back to tip_x, widening from root_w to tip_w (half widths), from z0 to
    z1, its end squared off with a shallow notch. U across, V along. tilt: degrees, its tip turned down (+) or up."""
    rows = []
    for i in range(steps + 1):
        t = i / steps
        x = root_x + (tip_x - root_x) * t
        w = root_w + (tip_w - root_w) * t
        z = z0 + (z1 - z0) * t
        row = []
        for a in (-1.0, -0.5, 0.0, 0.5, 1.0):
            end = -notch * (1.0 - abs(a)) if i == steps else 0.0
            row.append((x - end, a * w, z, (a + 1.0) * 0.5, t))
        rows.append(row)
    for want, dz in (((0, 0, 1), thickness * 0.5), ((0, 0, -1), -thickness * 0.5)):
        verts = [[part.vert((p[0], p[1], p[2] + dz)) for p in row] for row in rows]
        for i in range(steps):
            for k in range(4):
                quad = [verts[i][k], verts[i][k + 1], verts[i + 1][k + 1], verts[i + 1][k]]
                uvs = [part.at(region, rows[i][k][3], rows[i][k][4]), part.at(region, rows[i][k + 1][3], rows[i][k + 1][4]),
                       part.at(region, rows[i + 1][k + 1][3], rows[i + 1][k + 1][4]),
                       part.at(region, rows[i + 1][k][3], rows[i + 1][k][4])]
                part.face(quad, uvs, out=want)


def head_parts(part, at=(0.0, 0.0, 0.0), rot=None):
    """The head: a broad flat skull, the tiny dark beak, the eyes; at/rot place it (the head's own frame: its neck joint
    at the origin)."""
    R = (rot or Matrix.Identity(3)).to_3x3()
    o = Vector(at)
    T = lambda p: o + R @ Vector(p)
    # (A swallow's head is broad and flat, but small: about as wide as the body's narrowest, 2.8 cm.)
    la.ellipsoid(part, T((1.4, 0.0, 0.6)), (1.75, 1.42, 1.36), 'head', rot=R, segs=14, rings=9,
                 deform=lambda a, s, u: (a, s, u * (0.88 if u > 0 else 1.0)))
    la.ellipsoid(part, T((0.0, 0.0, -0.1)), (1.45, 1.4, 1.42), 'head', rot=R @ Matrix.Rotation(math.radians(55.0), 3, 'Y'),
                 segs=10, rings=6)
    la.tube(part, [T((2.9, 0.0, 0.48)), T((3.4, 0.0, 0.39)), T((3.85, 0.0, 0.26))], [(0.38, 0.45), (0.22, 0.25), (0.03, 0.03)],
            'beak', sides=6, ref=R @ Vector((0, 0, 1)), caps=(False, True))
    for side in (1.0, -1.0):
        la.ellipsoid(part, T((2.1, side * 1.1, 0.9)), (0.27, 0.27, 0.27), 'eye', rot=R @ Matrix.Rotation(math.radians(side * 70.0), 3, 'Z'),
                     segs=8, rings=5, patch=True)


def build_flying(material):
    part = la.Part('CliffSwallow_Flying', ATLAS, SIZE)
    # A compact, deep-chested body tapering to the tail: 14 cm from the bill to the square tail's end.
    la.ellipsoid(part, (-0.5, 0.0, 0.0), (4.5, 1.85, 1.85), 'body', segs=16, rings=10,
                 deform=lambda a, s, u: (a, s * (1.0 - 0.24 * max(0.0, -a) ** 2) * (1.0 + 0.05 * max(0.0, a)),
                                         u * (1.1 if u < 0 else 0.92) * (1.0 - 0.28 * max(0.0, -a) ** 2)))
    head_parts(part, at=(2.6, 0.0, 0.05))
    fan(part, 'tail', -4.4, -8.6, 0.9, 1.85, 0.3, 0.5, notch=0.3, thickness=0.22)
    obj = finish(part, material, 'CliffSwallow_Flying')
    socket(obj, 'WingL', (1.8, 1.6, 0.9))
    socket(obj, 'WingR', (1.8, -1.6, 0.9))
    return obj


ARM = [(0.0, 1.5, -2.6, 0.0), (1.6, 1.7, -3.0, 0.1), (3.2, 1.7, -2.9, 0.14), (4.8, 1.5, -2.7, 0.16)]
# The hand: long, narrow and pointed, swept back to the tip; a cliff swallow spans about 30 cm.
HAND = [(0.0, 1.5, -2.6, 0.0), (1.75, 1.35, -2.55, -0.02), (3.5, 0.95, -2.25, -0.06), (5.1, 0.35, -1.95, -0.1),
        (6.5, -0.6, -2.05, -0.15), (7.7, -1.7, -2.55, -0.19), (8.6, -2.9, -3.3, -0.22)]


def build_wings(material):
    out = []
    for side, sfx in ((1.0, 'L'), (-1.0, 'R')):
        part = la.Part(f'CliffSwallow_WingInner{sfx}', ATLAS, SIZE)
        panel(part, 'wing', ARM, side, 0.35, u_range=(0.0, 0.32), serrate=0.22)
        obj = finish(part, material, f'CliffSwallow_WingInner{sfx}')
        socket(obj, 'Wrist', (0.0, side * ARM[-1][0], ARM[-1][3]))
        out.append(obj)
        part = la.Part(f'CliffSwallow_WingOuter{sfx}', ATLAS, SIZE)
        panel(part, 'wing', HAND, side, 0.28, u_range=(0.32, 1.0), serrate=0.26)
        out.append(finish(part, material, f'CliffSwallow_WingOuter{sfx}'))
    return out


def build_perched(material):
    """Clinging upright at a nest's mouth: the body tipped up, wings folded and crossed over the tail, tail braced down,
    the feet gripping the rim at the pivot."""
    part = la.Part('CliffSwallow_Perched', ATLAS, SIZE)
    up = la.turn(0.0, -58.0, 0.0)                         # nose up
    body_at = Vector((-1.5, 0.0, 2.7))
    la.ellipsoid(part, body_at, (4.1, 1.88, 1.86), 'body', rot=up, segs=14, rings=9,
                 deform=lambda a, s, u: (a, s * (1.0 - 0.22 * max(0.0, -a) ** 2), u * (1.08 if u < 0 else 0.94)))
    R = up.to_3x3()
    for side in (1.0, -1.0):
        # Folded wings along the back, their long tips crossing just past the tail's end.
        start = body_at + R @ Vector((1.4, side * 1.45, 0.85))
        d = (R @ Vector((-1.0, -side * 0.08, 0.25))).normalized()
        rot = d.to_track_quat('X', 'Z').to_matrix()
        la.ellipsoid(part, start + d * 4.3, (5.2, 0.46, 1.3), 'folded', rot=rot, segs=10, rings=7,
                     deform=lambda a, s, u: (a, s * (1.0 - 0.7 * max(0.0, a) ** 1.5), u * (1.0 - 0.7 * max(0.0, a) ** 1.5)))
        # Legs: short and dark, just showing under the belly feathers, gripping forward at the rim.
        foot = Vector((0.3, side * 0.65, 0.15))
        hip = foot + Vector((-0.6, 0.0, 1.2))
        la.tube(part, [hip, foot], [0.2, 0.16], 'leg', sides=5, caps=(False, True))
        for spread in (-0.4, 0.0, 0.4):
            la.tube(part, [foot, foot + Vector((0.9, side * spread, -0.15))], [0.1, 0.06], 'leg', sides=4, caps=(False, True))
    # The tail braced against the mud below the mouth.
    tail_root = body_at + R @ Vector((-3.6, 0.0, -0.2))
    d = Vector((-0.45, 0.0, -1.0)).normalized()
    tail = [tail_root + d * (4.3 * t) for t in (0.0, 1.0)]
    side_v = Vector((0.0, 1.0, 0.0))
    for want, dz in ((d.cross(side_v), 0.12), (-d.cross(side_v), -0.12)):
        n = d.cross(side_v).normalized()
        rows = [[part.vert(tail[0] + side_v * (a * 1.0) + n * dz) for a in (-1.0, 0.0, 1.0)],
                [part.vert(tail[1] + side_v * (a * 2.0) + n * dz) for a in (-1.0, 0.0, 1.0)]]
        for k in range(2):
            part.face([rows[0][k], rows[0][k + 1], rows[1][k + 1], rows[1][k]],
                      [part.at('tail', 0.5 * k, 0.0), part.at('tail', 0.5 * (k + 1), 0.0), part.at('tail', 0.5 * (k + 1), 1.0),
                       part.at('tail', 0.5 * k, 1.0)], out=want)
    neck = body_at + R @ Vector((3.6, 0.0, 0.55))
    obj = finish(part, material, 'CliffSwallow_Perched')
    socket(obj, 'Head', tuple(neck))
    return obj


def build_head(material):
    part = la.Part('CliffSwallow_Head', ATLAS, SIZE)
    head_parts(part)
    return finish(part, material, 'CliffSwallow_Head')


# --- The nests ---

def throat(part, mouth, d, rad, depth):
    """A spout's dark inside: a tube facing inward from the mouth back into the gourd, closed deep inside, and a lip of
    mud joining it to the spout's outside, so the mouth reads as a hole from any side."""
    d = Vector(d).normalized()
    ref = (Vector((0.0, 0.0, 1.0)) - d * d.z).normalized()
    side = d.cross(ref)
    sides = 12
    rings = []
    for k, (off, r) in enumerate(((0.0, rad + 0.25), (0.02, rad), (-depth * 0.5, rad * 0.9), (-depth, rad * 0.75))):
        rings.append([part.vert(mouth + d * off + (ref * math.cos(a) + side * math.sin(a)) * r)
                      for a in (2.0 * math.pi * (j / sides - 0.5) for j in range(sides))])
    for k in range(3):
        region = 'mud_rim' if k == 0 else 'hole'
        for j in range(sides):
            j1 = (j + 1) % sides
            q = (rings[k][j], rings[k + 1][j], rings[k + 1][j1], rings[k][j1])
            center = sum((v.co for v in q), Vector()) / 4.0
            axis = mouth + d * d.dot(center - mouth)
            out = (center - axis) if k == 0 else (axis - center)
            if k == 0:
                out = out + d * 2.0
            part.face(q, [part.at(region, j / sides, 0.1 + 0.3 * k), part.at(region, (j + 1) / sides, 0.1 + 0.3 * k),
                          part.at(region, (j + 1) / sides, 0.4 + 0.3 * k), part.at(region, j / sides, 0.4 + 0.3 * k)], out=out)
    hub = part.vert(mouth - d * (depth + 0.3))
    for j in range(sides):
        j1 = (j + 1) % sides
        part.face((rings[-1][j], hub, rings[-1][j1]), [part.at('hole', j / sides, 0.9), part.at('hole', 0.5, 1.0),
                                                        part.at('hole', (j + 1) / sides, 0.9)], out=d)

NESTS = [((7.0, -18.0, -8.5), (7.4, 6.6, 7.6), (0.95, -0.25, -0.55), 4.2),
         ((8.0, -5.5, -9.5), (8.2, 6.8, 8.6), (1.0, 0.1, -0.45), 4.6),
         ((7.2, 7.0, -8.0), (7.4, 6.4, 7.6), (0.95, 0.3, -0.5), 4.0),
         ((6.4, 18.0, -7.4), (6.6, 5.8, 7.0), (0.9, 0.15, -0.6), 3.6)]


def build_nests(material):
    """Gourds of mud pellets against the wall and up under the overhang, each with a spout; inside every mouth a dark
    throat runs back into the gourd, so a hole reads as a hole."""
    part = la.Part('SwallowNests', ATLAS, SIZE)
    mouths = []
    for k, (c, r, spout, length) in enumerate(NESTS):
        c = Vector(c)

        def squash(p, c=c):
            # Flat where it's stuck to the wall behind (x = 0) and to the overhang above (z = 0), not quite level.
            q = Vector(p)
            q.x = max(q.x, 0.3 + 0.4 * math.sin(q.y * 0.7 + q.z * 0.5))
            q.z = min(q.z, -0.3 - 0.4 * math.sin(q.y * 0.5 + q.x * 0.8))
            return q
        before = len(part.bm.verts)
        la.ellipsoid(part, c, r, 'mud', segs=18, rings=11, u_range=(0.0, 1.0), v_range=(0.0, 1.0))
        part.bm.verts.ensure_lookup_table()
        for v in list(part.bm.verts)[before:]:
            p = squash(v.co)
            # Pellets: a lumpy surface, the same on every run.
            n = (p - c).normalized()
            p += n * 0.35 * math.sin(p.x * 1.7 + 2.0 * k) * math.sin(p.y * 1.9) * math.sin(p.z * 1.6 + k)
            v.co = p
        # The spout: a short tube out of the gourd's front, its lip and its dark throat.
        d = Vector(spout).normalized()
        base = c + Vector((r[0] * 0.55 * d.x, r[1] * 0.4 * d.y, r[2] * 0.55 * d.z))
        mouth = base + d * length
        rad = 2.2
        la.tube(part, [base - d * 1.0, base + d * (length * 0.5), mouth], [(rad + 0.8, rad + 0.7), (rad + 0.5, rad + 0.4),
                                                                         (rad + 0.25, rad + 0.25)], 'mud_rim', sides=12,
                caps=(False, False), ref=(0.0, 0.0, 1.0), u_range=(0.0, 1.0))
        throat(part, mouth, d, rad, length + 2.0)
        mouths.append((mouth, d))
    obj = finish(part, material, 'SwallowNests')
    obj['LODs'] = '50,25'
    obj['LODScreens'] = '0.2,0.08'
    for k, (mouth, d) in enumerate(mouths):
        socket(obj, f'Nest{k + 1}', tuple(mouth), front=tuple(d))
    lt.bake_vertex_ao(obj, samples=16, distance=0.08, ground=False)
    return obj


# --- Build ---

la.paint_atlas(ATLAS, paint, SIZE, PX_M, SET, NORMAL_STRENGTH)
MATERIAL = lt.material(SET)
FLYING = build_flying(MATERIAL)
WINGS = build_wings(MATERIAL)
PERCHED = build_perched(MATERIAL)
HEAD = build_head(MATERIAL)
NESTS_OBJ = build_nests(MATERIAL)
for o in [FLYING, PERCHED, HEAD] + WINGS:
    lt.bake_vertex_ao(o, samples=12, distance=0.02, ground=False)


# --- Previews ---

def socket_at(obj, name):
    s = next(c for c in obj.children if c.name.startswith('SOCKET_' + name))
    return s.matrix_world


def assemble_flying(studio, at, yaw=0.0, up=10.0, sweep=0.0, bank=0.0, pitch=0.0, name='Fly'):
    """A flying swallow put together from its pieces as FaunaFlock does: the wings on the shoulders turned up (degrees),
    the hands swept back about the wrist."""
    base = Matrix.Translation(Vector(at)) @ Matrix.Rotation(math.radians(yaw), 4, 'Z') @ \
        Matrix.Rotation(math.radians(bank), 4, 'Y') @ Matrix.Rotation(math.radians(pitch), 4, 'X')
    objs = [studio.copy(FLYING, name=name + 'Body')]
    objs[0].matrix_world = base
    pieces = {o.name: o for o in WINGS}
    for side, sfx in ((1.0, 'L'), (-1.0, 'R')):
        inner = pieces[f'CliffSwallow_WingInner{sfx}']
        outer = pieces[f'CliffSwallow_WingOuter{sfx}']
        shoulder = socket_at(FLYING, 'Wing' + sfx).translation
        # The wing's span runs along Blender -X for the left wing (creature +y): raising it turns about the forward axis.
        arm = Matrix.Translation(shoulder) @ Matrix.Rotation(math.radians(-up * side), 4, 'Y')
        a = studio.copy(inner, name=name + 'In' + sfx)
        a.matrix_world = base @ arm
        wrist = next(c for c in inner.children if c.name.startswith('SOCKET_Wrist')).location
        hand = Matrix.Translation(wrist) @ Matrix.Rotation(math.radians(sweep * side), 4, 'Z') @ \
            Matrix.Rotation(math.radians(-up * 0.4 * side), 4, 'Y')
        b = studio.copy(outer, name=name + 'Out' + sfx)
        b.matrix_world = base @ arm @ hand
        objs += [a, b]
    return objs


def previews():
    la.backface_culling([MATERIAL])
    studio = la.Studio()
    hide = [FLYING, PERCHED, HEAD, NESTS_OBJ] + WINGS
    la.hide(hide)
    report = {o.name: dict(triangles=la.tris(o), materials=[m.name for m in o.data.materials])
              for o in [FLYING, PERCHED, HEAD, NESTS_OBJ] + WINGS}
    # Under an overhang: a stand-in cliff (never exported), the nests fixed under it, swallows coming and going.
    wall = la.rock_block(studio, (1.6, 0.6, 1.4), (0.0, 0.3, 0.5), seed=4, name='Wall')
    roof = la.rock_block(studio, (1.6, 1.0, 0.4), (0.0, -0.1, 1.4), seed=6, name='Roof')
    at = Vector((0.0, 0.0, 1.2))                           # the nests' pivot: the wall's face under the overhang
    nest = studio.copy(NESTS_OBJ, name='NestsA')
    nest.matrix_world = Matrix.Translation(at)
    objs = [wall, roof, nest]
    # One clinging at the second nest's mouth with its head on, one peeking out of the third.
    s2 = socket_at(NESTS_OBJ, 'Nest2')
    cling = studio.copy(PERCHED, name='Cling')
    cling.matrix_world = Matrix.Translation(at) @ s2 @ Matrix.Rotation(math.radians(180.0), 4, 'Z')
    head = studio.copy(HEAD, name='ClingHead')
    head.matrix_world = cling.matrix_world @ Matrix.Translation(socket_at(PERCHED, 'Head').translation) @ \
        Matrix.Rotation(math.radians(25.0), 4, 'Z')
    s3 = socket_at(NESTS_OBJ, 'Nest3')
    peek = studio.copy(HEAD, name='Peek')
    peek.matrix_world = Matrix.Translation(at) @ s3 @ Matrix.Translation((0.0, 0.014, -0.006)) @ \
        Matrix.Rotation(math.radians(20.0), 4, 'Z')
    objs += [cling, head, peek]
    objs += assemble_flying(studio, (0.28, -0.42, 0.95), yaw=160.0, up=18.0, sweep=8.0, bank=-25.0, name='F1')
    objs += assemble_flying(studio, (-0.35, -0.6, 0.75), yaw=-30.0, up=-14.0, sweep=26.0, bank=20.0, name='F2')
    studio.light(azimuth=60.0, elevation=22.0)
    studio.shoot('Swallows', objs, None, lens=40.0, size=(1600, 1000), eye=Vector((-0.55, -1.75, 0.8)),
                 target=Vector((0.0, -0.15, 0.95)))
    studio.clear()
    # The flight poses side by side: wings up, wings down, gliding, swept for a dive.
    objs = []
    for k, (up, sweep, label) in enumerate(((48.0, 4.0, 'up'), (-38.0, 0.0, 'down'), (6.0, 6.0, 'glide'), (14.0, 48.0, 'dive'))):
        objs += assemble_flying(studio, ((k - 1.5) * 0.36, 0.0, 0.5), yaw=0.0, up=up, sweep=sweep, name='P' + label)
    studio.light(azimuth=30.0, elevation=40.0)
    studio.shoot('Swallow_Flight', objs, (0.0, -0.35, 1.0), lens=50.0, size=(2000, 800), margin=1.15)
    studio.clear()
    # Close: one clinging at a mouth and looking round, one just out of the next and away.
    wall = la.rock_block(studio, (1.0, 0.4, 1.0), (0.0, 0.2, 0.7), seed=4, name='Wall2')
    roof = la.rock_block(studio, (1.0, 0.7, 0.3), (0.0, -0.05, 1.35), seed=6, name='Roof2')
    at = Vector((0.0, 0.0, 1.2))
    nest = studio.copy(NESTS_OBJ, name='NestsB')
    nest.matrix_world = Matrix.Translation(at)
    cling = studio.copy(PERCHED, name='Cling2')
    cling.matrix_world = Matrix.Translation(at) @ socket_at(NESTS_OBJ, 'Nest2') @ Matrix.Rotation(math.radians(180.0), 4, 'Z')
    ch = studio.copy(HEAD, name='Cling2H')
    ch.matrix_world = cling.matrix_world @ Matrix.Translation(socket_at(PERCHED, 'Head').translation) @ \
        Matrix.Rotation(math.radians(-110.0), 4, 'Z')
    peek = studio.copy(HEAD, name='Peek2')
    peek.matrix_world = Matrix.Translation(at) @ socket_at(NESTS_OBJ, 'Nest3') @ Matrix.Translation((0.0, 0.014, -0.006)) @ \
        Matrix.Rotation(math.radians(-25.0), 4, 'Z')
    objs = [wall, roof, nest, cling, ch, peek]
    studio.light(azimuth=50.0, elevation=24.0)
    studio.shoot('Swallow_Close', objs, None, lens=60.0, size=(1600, 1000), eye=Vector((0.12, -0.62, 0.98)),
                 target=Vector((0.02, -0.06, 1.08)))
    studio.clear()
    la.write_report('Swallow', report)


if PREVIEW and __name__ != '__sheet__':
    previews()
