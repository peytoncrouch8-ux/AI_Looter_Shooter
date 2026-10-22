"""Odessa's head (Art/Models/Characters/Odessa.py calls these): the face and neck with the portrait's painted zones,
the glowing eyes, the brows (bake sources only: strips just over the skin whose colour the skin's bake takes), the hair
combed back into a knot at the nape (a thin shell over the scalp read along its 'Strand' UV, and the coiled knot), gold
drop earrings, and the portrait's flat-crowned gambler (Art/Portraits/Odessa.py build_hat) seated on her head with the
card in its band; the game head reduced region by region.

The hat's seat is ellis_hat.fit's method (copied: its constants are Ellis's cattleman fit): the band's foot on a plane
from over the brows at the front to over the occipital bump at the back, the crown's foot fitted to the head's section
in that plane plus the room for the hair, the felt and the sweatband; the hat bone's pivot in the back of the skull."""
import math

import bpy  # first: bmesh and mathutils come with it
import bmesh
import numpy as np
from mathutils import Matrix, Vector
from mathutils.bvhtree import BVHTree

import looter_garments as gm
import mh
import odessa_kit as kit
from odessa_kit import log, slot

# ----------------------------------------------------------------------------------------------- the face's paint

LIPS = (0.70, 0.42, 0.44)       # the portrait's lips (multiplied into the skin), a shade warmer in daylight
SUNK = 0.55                     # the portrait's sunken eyes eased (1 the Revenant's; A had 0.7 under painted light)


def paint_zones(fig, op, opt):
    """The 'zones' colours per MakeHuman vertex (the portrait's repaint_face at rest): bust.zones' sunken eyes eased to
    SUNK, the lips tinted, the scalp darkened under the hair, a dark line along each upper lid (her lashes)."""
    import bust
    h, hf = fig.b.h, fig.hf
    n = len(h['verts'])
    used = np.arange(n)
    verts = h['verts']
    c = bust.zones(h, verts, list(hf['eyes']), used)
    c[:, :3] = 1.0 - (1.0 - c[:, :3]) * SUNK
    hw = h['weights']('head')
    front = verts[hw > 0.5]
    lip = front[np.argmax(front[:, 2])] + np.array([0.0, -0.37, -0.20])     # under the nose's tip
    k = np.clip(1.0 - np.linalg.norm((verts - lip) * np.array([0.75, 2.4, 1.2]), axis=1) / 0.30, 0.0, 1.0) ** 0.6
    c[:, :3] *= 1.0 - k[:, None] * (1.0 - np.array(LIPS))
    sc = np.clip(above_hairline(hf, h['morphed']) / 0.06 + 0.7, 0, 1) * (hw > 0.5)
    c[:, :3] *= 1.0 - sc[:, None] * (1.0 - np.array([0.10, 0.07, 0.06]))
    for bone in ('orbicularis03.L', 'orbicularis03.R'):
        c[:, :3] *= 1.0 - (np.clip((h['weights'](bone) - 0.35) / 0.4, 0, 1) * 0.6)[:, None]
    return c


def build_skin(fig, op, opt):
    """The visible skin: the head and the neck down under the collar (the clothes cover the rest; the gloves are their
    own shell of the hands), with the painted zones, and the glowing eyeballs."""
    keep = fig.w_head > 0.45
    ob = fig.b.mesh('Odessa', keep=lambda i, f: bool(keep[f].all()))
    ob.data.materials.append(slot('Skin'))
    zones = paint_zones(fig, op, opt)
    idx = np.array([d.value for d in ob.data.attributes['mh_index'].data])
    attr = ob.data.color_attributes.new('zones', 'FLOAT_COLOR', 'POINT')
    attr.data.foreach_set('color', zones[idx].astype(np.float32).ravel())
    eyes = []
    for side in ('l', 'r'):
        c, r = fig.balls[side]
        bm = bmesh.new()
        kit.uv_sphere(bm, 20, 12, r * 0.1 * fig.b.scale)
        me = bpy.data.meshes.new(f'Eye_{side}')
        bm.to_mesh(me)
        bm.free()
        me.shade_smooth()
        eo = bpy.data.objects.new(f'Eye_{side}', me)
        bpy.context.scene.collection.objects.link(eo)
        eo.location = Vector(fig.head_point(c))
        eo.data.materials.append(slot('EyeGlow'))
        eyes.append(eo)
    log(f'skin: {len(ob.data.polygons)} faces, painted')
    return ob, eyes


def skin_tree(fig):
    if getattr(fig, '_head_tree', None) is None:
        keep = fig.w_head > 0.45
        faces = [list(f) for f in fig.faces if keep[f].all()]
        fig._head_tree = BVHTree.FromPolygons([tuple(p) for p in fig.wv], faces)
    return fig._head_tree


def build_brows(fig):
    """High arched groomed brows (the portrait's curve: the arch two-thirds out), as strips 1 mm over the skin: bake
    sources only, so the skin's texture takes them crisp (painted by vertex they were a smudge). Their 'Strand' UV runs
    along them for the hairs' strokes."""
    tree = skin_tree(fig)
    hf = fig.hf
    pieces = []
    for e in hf['eyes']:
        sx = 1.0 if e[0] > 0 else -1.0
        rows = [[], [], []]
        uvs = []
        for t in np.linspace(0.0, 1.0, 14):
            x = e[0] + sx * (-0.20 + 0.46 * t)
            y = e[1] + 0.17 + 0.075 * math.sin(t * math.pi * 0.8) - 0.04 * t * t
            p0 = fig.head_point([x, y, hf['front'] + 0.4])
            p1 = fig.head_point([x, y, hf['front'] - 1.0])
            d = gm.unit(p1 - p0)
            hit = tree.ray_cast(Vector(tuple(p0)), Vector(tuple(d)), 0.2)
            if hit[0] is None:
                continue
            p, nrm = np.array(hit[0]), np.array(hit[1])
            # the brow's width: full at its head, the arch, tapering to a fine tail
            w = (0.0062 * (1.0 - t) + 0.0022 * t) * (0.7 + 0.3 * math.sin(math.pi * min(1.0, t * 1.4)))
            up = fig.head_point([x, y + 0.1, hf['front']]) - fig.head_point([x, y, hf['front']])
            up = gm.unit(up - nrm * (up @ nrm))
            for k, s in enumerate((-0.5, 0.0, 0.5)):
                rows[k].append(p + nrm * (0.0010 + 0.0004 * (k == 1)) + up * w * s)
            uvs.append(t)
        R = np.array(rows)
        piece = gm.grid(R[..., 0], R[..., 1], R[..., 2])
        ob = gm.mesh(f'Brow_{"l" if sx > 0 else "r"}', piece, slot('Hair'))
        strand_uv(ob, lambda co, R=R: _strip_uv(co, R))
        pieces.append(ob)
        kit.CUSTOM_LOW[ob.name] = None                   # a bake source only: no game part
    return pieces


def _strip_uv(co, R):
    """A strip's points (u across, v along) from its rows (3, k, 3)."""
    mid = R[1]
    along = np.concatenate([[0.0], np.cumsum(np.linalg.norm(np.diff(mid, axis=0), axis=1))])
    out = np.zeros((len(co), 2))
    for i, p in enumerate(co):
        d = np.linalg.norm(mid - p, axis=1)
        j = int(np.argmin(d))
        side = np.linalg.norm(R[0, j] - p) - np.linalg.norm(R[2, j] - p)
        out[i] = (side * 0.5, along[j])
    return out


def strand_uv(ob, fn):
    """A 'Strand' UV layer from fn(points) -> (n, 2) per vertex."""
    me = ob.data
    co = gm._object_coords(ob)
    uv = fn(co)
    layer = me.uv_layers.get('Strand') or me.uv_layers.new(name='Strand')
    loops = np.empty(len(me.loops), dtype=np.int64)
    me.loops.foreach_get('vertex_index', loops)
    layer.data.foreach_set('uv', uv[loops].astype(np.float32).ravel())


# ------------------------------------------------------------------------------------------------------ the hair

HAIR_OFF = 0.0045       # the combed hair's depth over the scalp (m): inside the hat's room for it (HAT_ROOM_HAIR)
HAIR_EDGE = 0.0006      # ... at its edge (the hairline), feathered over the first 2.5 cm
HAIR_TUCK = 0.0015      # the hairline's edge tucked under the skin (the reduced skin's chords sit inside the detailed)
KNOT = (0.032, 0.030, 0.030)    # the knot's half sizes (m): across, up, out from the nape
# The game's hairline. The portrait's (Art/Portraits/Odessa.py above_hairline) takes every point at the head's sides
# (|x| > 0.66 dm) lower than 2.5 cm over the eyes for the ears: under the game's hat brim that left the sides of her head
# bald from the temples round over the ears (2026-10-22: hair only behind the ears and a few tips at the temples).
# Here only the ear itself is cut out (its top sits ~0.5 cm over the eyes) and the line comes down in front of it to a
# short sideburn; the brow's high line and the nape are the portrait's.
EAR_SIDE = 0.66         # dm off the middle: the head's sides (the ear sticks out past it)
EAR_TOP = 0.08          # dm over the eyes: the ear's cut-out ends here; above it the sides are scalp
TEMPLE = (20.0, 44.0)   # degrees round from the front over which the brow's high line falls to the temple
TEMPLE_Y = 0.12         # dm over the eyes: the line at the temples
SIDEBURN = 0.04         # dm over the eyes: where the line meets the ear's front
SIDEBURN_FROM = (55.0, 72.0)    # degrees round from the front over which the line drops to the sideburn
EAR_FROM = (76.0, 86.0)         # ... and over which it rises again to the ear's top (the ear: from ~78 degrees round)


def above_hairline(hf, v):
    """How far each rest-space point v sits above Odessa's game hairline (decimetres; negative below it, -1 on the
    ears): the portrait's line, the sides filled down to the ear and a sideburn in front of it."""
    ey = hf['eye_y']
    cz = (hf['verts'][:, 2].max() + hf['verts'][:, 2].min()) * 0.5
    a = np.abs(np.degrees(np.arctan2(v[:, 0], v[:, 2] - cz)))
    # high on the brow, low at the temples (the portrait's 28-58 degrees to ey + 0.14 left the temples under the brim
    # bare: the line reaches the temple sooner and lower)
    s = gm.smoothstep(TEMPLE[0], TEMPLE[1], a)
    line = ey + 0.58 - (0.58 - TEMPLE_Y) * s
    t = gm.smoothstep(SIDEBURN_FROM[0], SIDEBURN_FROM[1], a)
    line = line + t * (ey + SIDEBURN - line)
    t = gm.smoothstep(EAR_FROM[0], EAR_FROM[1], a)
    line = line + t * (ey + EAR_TOP - line)
    line = np.where(a < 105, line, ey - 0.50)
    ear = (np.abs(v[:, 0]) > EAR_SIDE) & (v[:, 1] < ey + EAR_TOP) & (a > EAR_FROM[0])
    return np.where(ear, -1.0, v[:, 1] - line)


def to_mh(fig, P):
    """World points back into MakeHuman's rest space (decimetres, +y up, +z forward): Body.to_world undone."""
    b = fig.b
    w = (np.asarray(P, dtype=np.float64) - np.array([0.0, 0.0, b.lift])) / (0.1 * b.scale)
    return np.stack([w[:, 0], w[:, 2] + b.floor, -w[:, 1]], axis=1)


def knot_place(fig):
    """The knot's centre and its axis out of the head (the portrait's: low at the nape under the hat, its centre a
    little behind the scalp)."""
    hf = fig.hf
    hv = hf['verts']
    by = hf['eye_y'] - 0.30
    near = hv[(np.abs(hv[:, 1] - by) < 0.1) & (np.abs(hv[:, 0]) < 0.25)]
    surf = fig.head_point(np.array([0.0, by, near[:, 2].min()]))
    centre_mh = np.array([0.0, by, (hf['front'] + hf['back']) * 0.5])
    axis = gm.unit(surf - fig.head_point(centre_mh))
    return surf + axis * (KNOT[2] * 0.5), axis


def build_hair(fig, op):
    """The hair combed back from the hairline and pinned in a knot at the nape: a shell over the scalp HAIR_OFF deep
    (feathered to HAIR_EDGE at the hairline, its edge tucked a hair under the skin so the skin draws the line), its
    'Strand' UV the comb's lines (meridians round the axis from the head's middle to the knot); and the knot, coiled,
    its strands round its own axis."""
    h, hf = fig.b.h, fig.hf
    ah = above_hairline(hf, h['morphed'])            # decimetres above the hairline (negative below, the ears -1)
    hw = h['weights']('head')
    # MakeHuman's scalp is coarse (its faces ~1.5 cm: the hairline came out in steps): a shell over the scalp and a
    # margin below the hairline, subdivided twice, then cut at the hairline at the finer faces and feathered to it
    region = (ah > -0.16) & (hw > 0.5)
    piece, used = gm.shell(fig.wv, fig.faces, fig.fuv, fig.face_mask(region), HAIR_OFF, iters=4)
    ob = gm.mesh('Hair', piece, slot('Hair'))
    m = ob.modifiers.new('fine', 'SUBSURF')
    m.levels = 2
    m.uv_smooth = 'PRESERVE_BOUNDARIES'
    gm.apply_modifiers(ob, keep=())
    co = gm._object_coords(ob)
    ahv = above_hairline(hf, to_mh(fig, co))
    ears = ahv <= -0.99
    bm = bmesh.new()
    bm.from_mesh(ob.data)
    bm.verts.ensure_lookup_table()
    gone = [f for f in bm.faces if sum(ahv[v.index] < 0.0 or ears[v.index] for v in f.verts) >= 3]
    bmesh.ops.delete(bm, geom=gone, context='FACES')
    loose = [v for v in bm.verts if not v.link_faces]
    bmesh.ops.delete(bm, geom=loose, context='VERTS')
    bm.verts.ensure_lookup_table()
    # the hairline smoothed along itself (the cut's steps), then each point's depth set over the skin: HAIR_OFF over
    # the scalp, feathered to HAIR_EDGE at the line, the line itself tucked HAIR_TUCK under the skin
    edge = [v for v in bm.verts if v.is_boundary]
    for _ in range(6):
        new = {}
        for v in edge:
            nb = [e.other_vert(v) for e in v.link_edges if e.is_boundary]
            if len(nb) == 2:
                new[v] = v.co * 0.5 + (nb[0].co + nb[1].co) * 0.25
        for v, p in new.items():
            v.co = p
    bm.to_mesh(ob.data)
    bm.free()
    ob.data.update()
    co = gm._object_coords(ob)
    ahv = above_hairline(hf, to_mh(fig, co))
    edge_mask = gm.boundary([list(p.vertices) for p in ob.data.polygons], len(co))
    tree = skin_tree(fig)
    feather = gm.smoothstep(0.0, 0.25, np.maximum(ahv, 0.0))
    for i, p in enumerate(co):
        loc, nrm, _f, _d = tree.find_nearest(Vector(tuple(p)), 0.05)
        if loc is None:
            continue
        loc, nrm = np.array(loc), np.array(nrm)
        depth = (p - loc) @ nrm
        want = -HAIR_TUCK if edge_mask[i] else HAIR_EDGE + (HAIR_OFF - HAIR_EDGE) * feather[i]
        if feather[i] < 0.999 or edge_mask[i]:
            co[i] = p + nrm * (want - depth)
    ob.data.vertices.foreach_set('co', co.astype(np.float32).ravel())
    ob.data.update()
    piece = gm.Piece(co, [list(p.vertices) for p in ob.data.polygons])
    knot_c, knot_a = knot_place(fig)
    centre = fig.head_point(np.array([0.0, hf['eye_y'] + 0.25, (hf['front'] + hf['back']) * 0.5]))
    axis = gm.unit(knot_c - centre)
    ref = gm.unit(np.cross(axis, [0.0, 0.0, 1.0]))
    ref2 = np.cross(axis, ref)

    def comb(co):
        q = co - centre
        r = np.linalg.norm(q, axis=1)
        polar = np.arccos(np.clip((q @ axis) / np.maximum(r, 1e-9), -1.0, 1.0))
        az = np.arctan2(q @ ref2, q @ ref)
        # the strands' seam (where u wraps) on the meridian from the knot straight down to the nape's hairline, a few
        # centimetres under the knot (up the back of the head it showed as a ragged line)
        az = np.where(az > math.pi * 0.5, az - 2 * math.pi, az)
        return np.stack([az * 0.10, polar * 0.10], 1)
    strand_uv(ob, comb)
    knot = build_knot(fig, knot_c, knot_a)
    log(f'hair: shell {len(piece.faces)} faces over the scalp, knot {len(knot.data.polygons)} faces at '
        f'({knot_c[0] * 100:.1f}, {knot_c[1] * 100:.1f}, {knot_c[2] * 100:.1f}) cm')
    return [ob, knot]


def build_knot(fig, c, axis):
    """The knot: an oval coil of hair, its grooves a spiral from the middle out (the portrait's coils, wound)."""
    bm = bmesh.new()
    kit.uv_sphere(bm, 40, 24, 1.0)
    me = bpy.data.meshes.new('Knot')
    bm.to_mesh(me)
    bm.free()
    v = np.array([p.co[:] for p in me.vertices])
    # local frame: x across, y up, z out of the head
    up = np.array([0.0, 0.0, 1.0])
    across = gm.unit(np.cross(up, axis))
    upl = np.cross(axis, across)
    rho = np.linalg.norm(v[:, :2], axis=1)
    psi = np.arctan2(v[:, 1], v[:, 0])
    groove = np.sin(2 * math.pi * rho * 3.0 - psi) * gm.smoothstep(0.05, 0.25, rho)
    flat = np.where(v[:, 2] < 0.0, 0.6, 1.0)                       # its back pressed flat onto the nape
    loc = np.stack([v[:, 0] * KNOT[0], v[:, 1] * KNOT[1], v[:, 2] * KNOT[2] * flat], 1)
    loc *= (1.0 + 0.06 * groove)[:, None]
    world = c + np.outer(loc[:, 0], across) + np.outer(loc[:, 1], upl) + np.outer(loc[:, 2], axis)
    me.vertices.foreach_set('co', world.astype(np.float32).ravel())
    me.update()
    me.shade_smooth()
    ob = bpy.data.objects.new('Knot', me)
    bpy.context.scene.collection.objects.link(ob)
    ob.data.materials.append(slot('Hair'))
    # strands wound round its axis: v along the coil, u across it
    strand_uv(ob, lambda co: np.stack([(rho - psi / (2 * math.pi) / 3.0) * 0.06, psi * rho * 0.04], 1))
    return ob


# ------------------------------------------------------------------------------------------------------ earrings

def build_earrings(fig):
    """Gold drops: a bead at each lobe (the lowest of the head's outermost points) and a teardrop under it."""
    h = fig.b.h
    m = h['morphed']
    w = h['weights']('head')
    parts = []
    for sx, s in ((1.0, 'l'), (-1.0, 'r')):
        pts = m[(w > 0.5) & (m[:, 0] * sx > 0)]
        far = pts[np.abs(pts[:, 0]) > np.abs(pts[:, 0]).max() * 0.85]
        top = fig.head_point(far[np.argmin(far[:, 1])] + np.array([sx * -0.04, -0.03, 0.0]))
        sc = fig.b.scale
        parts.append(kit.ellipsoid(f'Earring_{s}', top, (0.0034 * sc,) * 3, (0, 0, 1), 12, 8, slot('Gold')))
        parts.append(kit.ellipsoid(f'EarDrop_{s}', top + np.array([0.0, 0.0, -0.014 * sc]),
                                   (0.0045 * sc, 0.0045 * sc, 0.0090 * sc), (0, 0, 1), 14, 10, slot('Gold')))
    return parts


# ------------------------------------------------------------------------------------------------------ the hat

BROW = 0.018            # the brows over the eyes' centres (m)
BROW_UP = 0.014         # the sweatband's foot over the brows at the front: worn low (the portrait's "hat low")
BACK_UP = 0.010         # ... and over the occipital bump at the back
ROOM_FRONT = 0.002      # the sweatband off the skin on the bare forehead
ROOM_HAIR = 0.0075      # ... and over the combed hair (the sides and back, from the temples): HAIR_OFF and 3 mm
FELT = 0.004            # the crown's felt (THICK: the inner shell 4 mm in)
SWEAT = 0.0035          # the sweatband's face inside the felt's
SWEAT_H = 0.035         # the sweatband's height up the crown's inside
BRIM_KEEP = 0.7         # how much of the crown's narrowing the brim's edge does not follow
CROWN_AIR = 0.008       # the least room between the skull and the crown's inside
CARD_AT = 50.0          # the card in the band: degrees round from the front toward her left (the portrait's)
PIVOT = (0.04, -0.03)   # the hat bone's head from the back of the sweatband's foot: forward, up (m)


def _hit(tree, origin, direction, reach=0.3):
    loc = tree.ray_cast(Vector(tuple(origin)), Vector(tuple(gm.unit(direction))), reach)[0]
    return None if loc is None else np.array(loc)


def seat(fig):
    """Where the hat sits: the band plane's centre O and axes (x her left, f forward, u up the crown), and the head's
    section in that plane (ring_ang from the front, + toward her left; ring_r from O to the skin)."""
    if getattr(fig, 'hat_seat', None):
        return fig.hat_seat
    tree = skin_tree(fig)
    hy = fig.sk['head'][0][1]
    inside = lambda z: np.array([0.0, hy, z])
    F = _hit(tree, inside(fig.eye_z + BROW + BROW_UP), (0.0, -1.0, 0.0))
    c = inside(fig.eye_z + 0.03)
    best = None
    for deg in np.arange(140.0, 221.0, 2.0):
        a = math.radians(deg)
        p = _hit(tree, c, (0.0, -math.cos(a), math.sin(a)))
        if p is not None and (best is None or p[1] > best[1]):
            best = p
    B = best + np.array([0.0, 0.0, BACK_UP])
    x = np.array([1.0, 0.0, 0.0])
    f = gm.unit(F - B)
    u = np.cross(f, x)
    O = (F + B) * 0.5
    ang = np.linspace(-math.pi, math.pi, 192, endpoint=False)
    r = []
    for a in ang:
        p = _hit(tree, O, f * math.cos(a) + x * math.sin(a))
        r.append(np.linalg.norm(p - O) if p is not None else np.nan)
    r = np.array(r)
    good = ~np.isnan(r)
    r = np.interp(ang, ang[good], r[good], period=2 * math.pi)
    k = np.array([1, 2, 3, 2, 1], dtype=np.float64)
    r = np.convolve(np.concatenate([r[-2:], r, r[:2]]), k / k.sum(), mode='valid')
    fig.hat_seat = dict(O=O, x=x, f=f, u=u, F=F, B=B, ring_ang=ang, ring_r=r)
    return fig.hat_seat


def room(ang):
    return ROOM_FRONT + (ROOM_HAIR - ROOM_FRONT) * gm.smoothstep(math.radians(35), math.radians(60), np.abs(ang))


def ring_at(s, ang):
    return np.interp(ang, s['ring_ang'], s['ring_r'], period=2 * math.pi)


def _frame_of(ring):
    O = ring.mean(0)
    _, _, vt = np.linalg.svd(ring - O)
    u = vt[2] if vt[2][2] > 0 else -vt[2]
    f = ring[0] - O
    f = gm.unit(f - u * (f @ u))
    x = np.cross(u, f)
    return O, x, f, u


def _local(P, frame):
    O, x, f, u = frame
    Q = P - O
    return np.stack([Q @ x, Q @ f, Q @ u], 1)


def _world(L, frame):
    O, x, f, u = frame
    return O + np.outer(L[:, 0], x) + np.outer(L[:, 1], f) + np.outer(L[:, 2], u)


def _faces_out(ob, axis_point, axis):
    me = ob.data
    score = 0.0
    for p in me.polygons:
        c = np.array(p.center) - axis_point
        c = c - axis * (c @ axis)
        score += float(np.array(p.normal) @ c) * p.area
    if score < 0.0:
        bm = bmesh.new()
        bm.from_mesh(me)
        bmesh.ops.reverse_faces(bm, faces=bm.faces[:])
        bm.to_mesh(me)
        bm.free()
        me.update()


def build_hat(fig, op, opt):
    """The portrait's gambler (op.build_hat: a low straight crown with a flat top, a wide flat brim rolled up at its
    edge, a wine silk band) moved into the figure's space and seated (fit), with a sweatband inside; then the card
    tucked in the band (CARD_AT). The crown's felt and the brim get their thickness after the bake (THICK)."""
    before = set(bpy.data.objects.keys())
    op.build_hat(fig.b.h, fig.hf, opt)
    made = [o for o in bpy.data.objects if o.name not in before]
    parts = []
    for ob in made:
        if ob.name.startswith('HatCard'):
            me = ob.data
            bpy.data.objects.remove(ob)
            bpy.data.meshes.remove(me)
            continue
        for m in list(ob.modifiers):
            ob.modifiers.remove(m)
        fig.to_portrait(ob)
        kit.bake_transform(ob)
        ob.data.materials.clear()
        parts.append(ob)
    crown = next(o for o in parts if o.name.startswith('HatCrown'))
    brim = next(o for o in parts if o.name.startswith('HatBrim'))
    band = next(o for o in parts if o.name.startswith('HatBand'))
    crown.name, brim.name, band.name = 'HatCrown', 'HatBrim', 'HatBand'
    fit(fig, crown, brim, band)
    inner = sweatband(fig, crown)
    crown.data.materials.append(slot('Felt'))
    brim.data.materials.append(slot('Felt'))
    band.data.materials.append(slot('HatBand'))
    inner.data.materials.append(slot('SweatBand'))
    for ob in (crown, brim, band, inner):
        kit.uv_cylinder(ob)
    card = hat_card(fig, band)
    fig.hat_pivot = pivot(fig)
    log(f'hat: seated (crown raised {fig.hat_fit["raise_by"] * 100:.0f}%, its inside {fig.hat_fit["air"] * 1000:.1f} mm '
        f'off the skull at least); the card at {CARD_AT:.0f} degrees')
    return [crown, brim, band, inner] + card


def fit(fig, crown, brim, band):
    """Seats the hat (ellis_hat.fit's method): the crown's foot fitted column by column to the head's section in the
    band plane plus the room, the felt and the sweatband; the brim's spokes follow at their root and BRIM_KEEP less at
    the edge; the crown raised if its top came within CROWN_AIR of the skull; all of it moved onto seat()."""
    s = seat(fig)
    cc = gm._object_coords(crown)
    src = _frame_of(cc[:96])
    dst = (s['O'], s['x'], s['f'], s['u'])
    base = _local(cc[:96], src)
    psi = np.arctan2(base[:, 0], base[:, 1])
    want = ring_at(s, psi) + room(psi) + SWEAT + FELT
    k = want / np.linalg.norm(base[:, :2], axis=1)
    L = _local(cc, src)
    col = np.arange(len(L)) % 96
    top = len(L) - 1 if len(L) % 96 == 1 else None
    kk = k[col]
    if top is not None:
        kk[top] = k.mean()
    L[:, :2] *= kk[:, None]
    tree = skin_tree(fig)
    raise_by, worst = 0.0, 1.0
    for _ in range(12):
        Lr = L.copy()
        Lr[:, 2] *= 1.0 + raise_by
        P = _world(Lr, dst)
        worst = 1.0
        for p in P[Lr[:, 2] > 0.05]:
            loc, nrm, _i, d = tree.find_nearest(Vector(tuple(p)), 0.1)
            if loc is not None:
                worst = min(worst, (Vector(tuple(p)) - loc).dot(nrm) - FELT)
        if worst >= CROWN_AIR:
            break
        raise_by += 0.02
    L[:, 2] *= 1.0 + raise_by
    crown.data.vertices.foreach_set('co', _world(L, dst).astype(np.float32).ravel())
    crown.data.update()
    bb = gm._object_coords(band)
    Lb = _local(bb, src)
    Lb[:, :2] *= k[np.arange(len(Lb)) % 96][:, None]
    band.data.vertices.foreach_set('co', _world(Lb, dst).astype(np.float32).ravel())
    band.data.update()
    br = gm._object_coords(brim)
    Lr = _local(br, src).reshape(-1, 96, 3)
    rad = np.linalg.norm(Lr[..., :2], axis=2)
    r0, r1 = rad[0], rad[-1]
    shrink = r0 * (1.0 - k)
    r1n = r1 - (1.0 - BRIM_KEEP) * shrink
    t = (rad - r0) / np.maximum(r1 - r0, 1e-6)
    new = r0 * k + t * (r1n - r0 * k)
    Lr[..., :2] *= (new / np.maximum(rad, 1e-9))[..., None]
    brim.data.vertices.foreach_set('co', _world(Lr.reshape(-1, 3), dst).astype(np.float32).ravel())
    brim.data.update()
    _faces_out(crown, s['O'], s['u'])
    _faces_out(band, s['O'], s['u'])
    fig.hat_fit = dict(s, raise_by=raise_by, k=k, air=worst, L=L, top=top, dst=dst)


def sweatband(fig, crown):
    """A leather strip on the crown's inside, from its foot SWEAT_H up, facing the head."""
    hf = fig.hat_fit
    L, top, dst = hf['L'], hf['top'], hf['dst']
    Lc = L[:len(L) - (top is not None)].reshape(-1, 96, 3)
    rows = []
    for hgt in (0.0, SWEAT_H):
        pts = []
        for j in range(0, 96, 2):
            c = Lc[:6, j]
            p = np.array([np.interp(hgt, c[:, 2], c[:, 0]), np.interp(hgt, c[:, 2], c[:, 1]), hgt])
            rad = np.linalg.norm(p[:2])
            p[:2] *= (rad - FELT - SWEAT) / rad
            pts.append(p)
        rows.append(_world(np.array(pts), dst))
    R = np.array(rows[::-1])
    inner = gm.mesh('HatBandInner', gm.grid(R[..., 0], R[..., 1], R[..., 2], wrap=True))
    s = seat(fig)
    _faces_out(inner, s['O'], s['u'])
    bm = bmesh.new()
    bm.from_mesh(inner.data)
    bmesh.ops.reverse_faces(bm, faces=bm.faces[:])
    bm.to_mesh(inner.data)
    bm.free()
    inner.data.update()
    return inner


CARD_W, CARD_H, CARD_T = 0.063, 0.088, 0.0008


def hat_card(fig, band):
    """The card tucked in the band at CARD_AT toward her left (the portrait's: the ace of spades, a plain pip): its
    lower third behind the band against the crown, leaning out 14 degrees, canted 18 degrees back. The board a closed
    slab (its game part), the pips faces just over its front (bake sources only, so its texture takes them)."""
    s = fig.hat_fit
    co = gm._object_coords(band)
    L = _local(co, s['dst'])
    ang = np.arctan2(L[:, 0], L[:, 1])
    a = math.radians(CARD_AT)
    near = np.abs((ang - a + math.pi) % (2 * math.pi) - math.pi) < math.radians(6)
    foot = L[near][np.argmin(L[near][:, 2])]
    r = np.linalg.norm(foot[:2]) + 0.0025
    out = np.array([math.sin(a), math.cos(a), 0.0])
    tan = np.array([math.cos(a), -math.sin(a), 0.0])
    lean = math.radians(14.0)
    upc = np.array([0.0, 0.0, math.cos(lean)]) + out * math.sin(lean)
    cant = math.radians(18.0)
    ax = tan * math.cos(cant) + upc * math.sin(cant)
    az = upc * math.cos(cant) - tan * math.sin(cant)
    # its foot on the band just over the brim (the portrait's: centred 4.5 cm up), leaning out from there
    bottom = out * (r + 0.001) + np.array([0.0, 0.0, foot[2] + 0.005])
    centre = bottom + az * (CARD_H * 0.5)
    O, x, f, u = s['dst']
    to_w = lambda p: O + x * p[0] + f * p[1] + u * p[2]
    dir_w = lambda d: x * d[0] + f * d[1] + u * d[2]
    # the frame in world space (the seat's (x, f, u) is left-handed: a cross product taken in it came out turned, and
    # the card was built inside out: black, its occlusion baked 0)
    AX, AZ, OUT = dir_w(ax), dir_w(az), dir_w(out)
    N = np.cross(AX, AZ)                                    # a counterclockwise outline's face: out from the crown
    if N @ OUT < 0:
        AX, N = -AX, -N
    frame = (to_w(centre), AX, -N, AZ)                      # the slab's -y is its face
    outline = kit.rounded_rect(CARD_W, CARD_H, 0.0045)
    board = kit.rounded_slab('HatCard', outline, CARD_T, frame, slot('Card'))
    kit.LOW['HatCard'] = 1.0
    pips = []
    op = fig.op
    for cx, cz, sz, rot in ((0.0, 0.0, 0.0125, 1), (-CARD_W / 2 + 0.0075, CARD_H / 2 - 0.0115, 0.0042, 1),
                            (CARD_W / 2 - 0.0075, -CARD_H / 2 + 0.0115, 0.0042, -1)):
        for k, poly in enumerate(op.pip_shapes('spade')):
            pts = [(cx + px * sz * rot, cz + pz * sz * rot) for px, pz in poly]
            verts = [frame[0] + frame[1] * px - frame[2] * (CARD_T * 0.5 + 0.0002) + frame[3] * pz for px, pz in pts]
            ob = gm.mesh(f'HatPip_{len(pips)}', gm.Piece(np.array(verts), [list(range(len(verts)))]), slot('InkBlack'),
                         smooth=False)
            kit.CUSTOM_LOW[ob.name] = None
            pips.append(ob)
    return [board] + pips


def pivot(fig):
    """The hat bone (head, tail; world): inside the back of the skull, PIVOT from the back of the sweatband's foot, so
    the band slides round the head as the game tips it (ellis_hat.pivot's)."""
    s = seat(fig)
    back = s['O'] - s['f'] * (ring_at(s, np.array([math.pi]))[0] + room(np.array([math.pi]))[0])
    head = back + s['f'] * PIVOT[0] + s['u'] * PIVOT[1]
    return head, head + s['f'] * 0.2


# ------------------------------------------------------------------------------------------- the game head's reduction

# Each region's share (ellis_hat.decimate_skin's way: one Decimate per region, the rest spared): the lids whole, the
# mouth and its corners well kept (her half smile), the face she shows, the skull under the crown and the neck under
# the high collar.
SKIN_LOW = {'eyes': 1.0, 'mouth': 0.5, 'face': 0.28, 'skull': 0.15, 'hidden': 0.10, 'seen': 0.20}
EYE_KEEP = 0.017


def decimate_skin(fig, low):
    s = seat(fig)
    mouth_c = fig.head_point(fig.mouth_mh)

    def regions(co):
        d = np.minimum(np.linalg.norm(co - fig.eyes[0], axis=1), np.linalg.norm(co - fig.eyes[1], axis=1))
        eyes = d < EYE_KEEP
        skull = ((co - s['O']) @ s['u'] > 0.012) & ~eyes
        mouth = (np.linalg.norm((co - mouth_c) * np.array([1.0, 1.0, 1.6]), axis=1) < 0.032) & ~eyes & ~skull
        hidden = (co[:, 2] < fig.neck[2] + 0.035) & ~eyes & ~skull & ~mouth
        face = (co[:, 1] < fig.sk['head'][0][1] - 0.025) & ~(eyes | skull | mouth | hidden)
        return {'eyes': eyes, 'mouth': mouth, 'skull': skull, 'hidden': hidden, 'face': face,
                'seen': ~(eyes | skull | mouth | hidden | face)}

    before = sum(len(p.vertices) - 2 for p in low.data.polygons)
    for name in ('skull', 'hidden', 'seen', 'face', 'mouth', 'eyes'):
        share = SKIN_LOW[name]
        if share >= 1.0:
            continue
        co = gm._object_coords(low)
        reg = regions(co)[name]
        tris = sum(len(p.vertices) - 2 for p in low.data.polygons)
        mine = sum(len(p.vertices) - 2 for p in low.data.polygons if reg[list(p.vertices)].all())
        if not mine:
            continue
        m = low.modifiers.new('decimate', 'DECIMATE')
        m.ratio = (tris - mine * (1.0 - share)) / tris
        m.use_symmetry = True
        m.symmetry_axis = 'X'
        g = low.vertex_groups.new(name='keep')
        g.add([int(i) for i in np.nonzero(~reg)[0]], 1.0, 'REPLACE')
        m.vertex_group = 'keep'
        m.invert_vertex_group = True
        m.vertex_group_factor = 1.0
        gm.apply_modifiers(low, keep=())
        low.vertex_groups.remove(low.vertex_groups['keep'])
    return before, sum(len(p.vertices) - 2 for p in low.data.polygons)
