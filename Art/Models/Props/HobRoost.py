"""Hob's roost (the user's call, 2026-10-13): the game's ONE save, respawn and fast-travel point on every map, replacing the
respawn graves. Hob's crows keep a roost on every map Ellis can reach (a frontier valley now; an airship, a riverboat
casino and a sky island later), and carry Ellis between them. It must read as the same landmark everywhere: one
silhouette the player learns, from 60 m and more, with a beacon burning at its top once it's found (the game's code
lights it: World/RoostBeacon, a soft column and flame on SOCKET_Beacon, in one colour that is no loot colour).

A scripted model (Art/README.md) in Ransom's Rest's shared materials, no new texture sets, no tin or MetalRust. Three
design directions are built here (the look sheet 41_Roost.jpg shows them; the user picks one):

  A  The Keeper's Crook   a hewn square post with a forged iron crook hooking out east from its head, the fire basket
                          hung from the crook's end on three chains; a level crossbar east-west and a short one
                          north-south above it, one knee brace; the nest on the post's head beside the crook. 3,102 tris.
  B  The Crow's Mast      a weathered mast in two pieces (the lower mast, and a topmast fished to its back with two iron
                          bands); a long yard lashed across it cockbilled (tilted 17 degrees, as a ship's yards are in
                          mourning: no cross and no telegraph pole from any side); a big ragged nest on a short yard
                          round the topmast (a stork's nest on a pole: the "crow's nest"); a crosstree, and the iron
                          cresset (a fire basket) at the masthead, 5 m up. Recommended: it belongs on a deck as much as
                          on a hill. 4,558 tris.
  C  The Crow Tree        a dead oak snag forking near the top, its branches cut back to perches, the nest in the fork,
                          the cresset spiked on the taller leader. 2,922 tris.

Every direction carries the same furniture: a cairn of granite round its foot; the nest (a bowl woven of grass and
twigs, oak sticks round it and out past its rim, black feathers in it); rope lashings with Hob's feathers caught in
them; charms on twine (knucklebones, a crow's skull); at eye height a board with the roost's mark burnt into it (a
crow's foot, the same on every roost) and a horseshoe nailed up over it; and the small iron bell, a model of its own so
the game can swing and ring it:

  HobRoost        the roost (DIRECTION below picks which; --direction=A|B|C overrides it for a run). Its pivot is the
                  post's foot on the ground, its front (where the player wakes) toward -Y.
  HobRoostBell    the iron bell (20 cm across the lip, 27 cm to the clapper), its pivot the top of its hanger loop,
                  hanging down -Z: it goes on the roost's SOCKET_Bell. 692 tris; no collision, no Nanite, LODs 50/25%.

Materials (8; all shared names, MI_<name>): HouseTrim (the timbers on the hewn beam strip, the board, its nails),
IronBlack (bands, cresset, crook, horseshoe, hooks), RopeHemp, BarkOak (sticks, C's trunk), HobFeathers (Hob.py's: the
feathers, the burnt mark, the cold coals), BoneDry (DenDressing.py's), RockGranite (the cairn) and NestTwigs, the one new
instance: the Hay set tinted to old grass and twigs.

Sockets (a socket's front is its empty's -Y, its top +Z; Art/README.md). B's, in Blender metres (Unreal: x = -y,
y = -x, cm); the script prints every direction's as it builds:
  Beacon      (0.05, 0.21, 4.76)  the middle of the fire basket, top up: where the code stands the far light.
  Bell        (0.82, -0.05, 2.08) the bell's pivot, under its hook toward the yard's low east end.
  Perch_1..6  where the code perches crows (the fauna's Crow_Perched pivot is its feet), each across its bar as a
              bird grips one, turned outward: 1 and 2 on the yard (2.71 m, 2.35 m), 3 and 4 on the nest bed's ends
              (3.20 m), 5 on the nest's rim (3.63 m), 6 on the crosstree (4.13 m). The nest's sticks keep clear of them.
  Wake        (0, -1.7, 0) on the ground in front of the post, facing away from it: where Ellis stands on arrival or
              wakes after a death.
  Speaker     (0.04, -0.38, 3.64) the nest's front rim, facing front: where Hob's lines come from (and where Hob sits).

Collision: a slim hull round the post only, from the ground to the masthead, and a low one (0.3 m, 0.74 m round) over
the cairn at its foot; the yards, nest, bell and charms have none (players, creatures and bullets pass the perches).
Nanite on with every triangle in the fallback (Medium draws it). Budget: under 5,000 triangles.

    Tools\\artrun.ps1 -Script Art\\Models\\Props\\HobRoost.py -Preview     every direction, the renders and the look sheet
                                     (Tools/Blender/roost_sheet.py: Saved/ArtPreviews/RansomsRest/Roost/,
                                     LookSheets/41_Roost.jpg); --quick only each direction's golden view
    Tools\\artrun.ps1 -Export HobRoost -Family RR_Roost                 export test of DIRECTION
Everything random comes from seeded random.Random and mathutils noise, nothing iterates a set of mesh elements, and
the lumps (bones, coals) are built here rather than with bmesh's UV sphere (whose faces come out in a different order
on every run), so every export is the same apart from the FBX's ids and time stamps.
"""
import math
import random

import bmesh
from mathutils import Matrix, Vector, noise

import looter_buildings as kit
import looter_props as lp
import looter_ruins as lr
import looter_textures as lt
from looter_buildings import Trim

ARGS = kit._args()
# The direction the game uses (the user's pick from the look sheet; B is the recommendation until then).
DIRECTION = next((a.split('=', 1)[1].upper() for a in ARGS if a.startswith('--direction=')), 'B')
PREVIEW = lt.want_preview()
UP = Vector((0.0, 0.0, 1.0))
SEED = 4141
# The mast's wood: one face of the trim sheet's hewn beams per timber, weathered dark (a strong silhouette in the gold).
MAST_WOOD = next((a.split('=', 1)[1] for a in ARGS if a.startswith('--wood=')), 'Beams')

# --- Materials: the shared names (MI_<name> in Unreal), each made exactly as its first family makes it ---

ROPE = lt.material('Hay', name='RopeHemp', tint=0x9a7a50)                                       # looter_mountain
BONE = lt.material('Polymer', name='BoneDry', tint=0xe6d9bf, RoughnessOffset=0.3, DiffuseAO=0.85)  # DenDressing.py
BONE_DENSITY = 1536.0
FEATHERS = lt.material('Polymer', name='HobFeathers', tint=0x16161a, RoughnessOffset=0.15)        # Hob.py
BARK = lt.material('BarkOak')                                                                    # the oak trees
# New here: the nest's woven bowl, the straw set darkened to old grass and fine twigs (no new texture).
NEST = lt.material('Hay', name='NestTwigs', tint=0x5c5044)


def yaw_of(facing):
    """The socket rotation (degrees about Z) whose front (-Y) points along the horizontal direction facing."""
    return math.degrees(math.atan2(facing[0], -facing[1]))


def flat(v):
    return Vector((v[0], v[1], 0.0))


def tiled(part, mat, set_name, seed, density=None):
    """A part on a tileable set at world scale."""
    lt.assign(part, mat)
    lt.box_uv(part, set_name, texel_density=density, seed=seed)
    return part


# --- Timber ---

def timber(points, radii, sides=8, strip='Beams', seed=0, jitter=0.05, up=(0.0, 0.0, 1.0)):
    """A round timber (a mast, a yard, a pole) along points, radius radii[i] at each, a little out of round, its grain
    along it on one face of the trim sheet's hewn beams (or a tileable set). up: a direction never along the timber
    (it orients the rings, so they never twist)."""
    profile = lp.ngon(1.0, sides, jitter=jitter, seed=seed, start=math.pi / sides)
    part = lp.sweep([tuple(p) for p in points], profile, up=up, scales=list(radii))
    axis = (Vector(points[-1]) - Vector(points[0])).normalized()
    lp.grain(part, strip, axis=tuple(axis), seed=seed)
    return part


def stick(points, radius, seed, sides=3, taper=0.6):
    """A twig or branch of the nest (oak bark), tapering to its tip, the bark's furrows running along it."""
    count = len(points)
    radii = [radius * (1.0 - (1.0 - taper) * i / max(count - 1, 1)) for i in range(count)]
    part = lr.tube(points, radius, sides=sides, key='trim', set_name='BarkOak', radii=radii, seed=seed)
    lt.assign(part, BARK)
    # The tube maps U along it and V round it; the bark's furrows run along V, so turn the mapping a quarter.
    for loop in part.data.uv_layers.active.data:
        u, v = loop.uv
        loop.uv = (v, u)
    return part


def rope(points, radius=0.011, sides=3, seed=0, closed=False):
    """Hemp rope or twine along points (closed: a loop, the first point repeated at the end)."""
    pts = list(points) + ([points[0]] if closed else [])
    part = lr.tube(pts, radius, sides=sides, key='trim', set_name='Hay', caps=(not closed, not closed), seed=seed)
    lt.assign(part, ROPE)
    return part


def iron_bar(points, radius, seed, sides=4, radii=None, caps=(True, True)):
    """Forged black iron (a crook, a cresset's bar, a hook)."""
    return lr.tube(points, radius, sides=sides, key='ironblack', set_name='MetalWorn', radii=radii, caps=caps, seed=seed)


def hull_outline(circles, count=28, z=0.0):
    """The convex outline round several circles ((x, y), r) in plan, as a closed loop of points at height z."""
    pts = []
    for k in range(count):
        a = 2.0 * math.pi * k / count
        d = Vector((math.cos(a), math.sin(a), 0.0))
        c, r = max(circles, key=lambda cr: Vector((cr[0][0], cr[0][1], 0.0)).dot(d) + cr[1])
        pts.append(Vector((c[0], c[1], z)) + d * r)
    return pts


def band(loop, width, thick, seed):
    """A flat iron band standing on edge along a closed loop (counter-clockwise from above), its face outward."""
    bm = lp.new_bmesh()
    count = len(loop)
    rings = []
    for i in range(count):
        p = Vector(loop[i])
        t = (Vector(loop[(i + 1) % count]) - Vector(loop[i - 1])).normalized()
        out = Vector((t.y, -t.x, 0.0)).normalized()
        half = UP * (width * 0.5)
        rings.append([bm.verts.new(p - out * 0.002 - half), bm.verts.new(p + out * thick - half),
                      bm.verts.new(p + out * thick + half), bm.verts.new(p - out * 0.002 + half)])
    for i in range(count):
        a, b = rings[i], rings[(i + 1) % count]
        for j in range(4):
            bm.faces.new((a[j], a[(j + 1) % 4], b[(j + 1) % 4], b[j]))
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces)
    part = lp.mesh_object(bm)
    return tiled(part, lr.material('ironblack'), 'MetalWorn', seed)


def iron_ring(center, radius, width, thick, seed, count=16):
    """An iron band round a round timber at center (its axis upright)."""
    return band([Vector(center) + Vector((math.cos(a) * radius, math.sin(a) * radius, 0.0))
                 for a in (2.0 * math.pi * k / count for k in range(count))], width, thick, seed)


def helix(center, radius, z0, z1, turns, seed, rope_r=0.010, per_turn=9, phase=0.0):
    """Turns of rope round an upright timber (a whipping, a seizing)."""
    count = max(2, int(turns * per_turn))
    pts = [(center[0] + math.cos(phase + 2.0 * math.pi * turns * i / count) * radius,
            center[1] + math.sin(phase + 2.0 * math.pi * turns * i / count) * radius,
            z0 + (z1 - z0) * i / count) for i in range(count + 1)]
    return rope(pts, rope_r, seed=seed)


def lashing(mast_c, mast_r, mast_dir, yard_c, yard_r, yard_dir, seed, rope_r=0.011, count=16):
    """A diagonal lashing where a yard crosses a timber: two turns round both, crossing in an X on the yard's face (the
    callers whip the timber above and below with helix()). mast_c and yard_c: the two axes' closest points."""
    parts = []
    a, b = Vector(mast_c), Vector(yard_c)
    da, db = Vector(mast_dir).normalized(), Vector(yard_dir).normalized()
    n = (b - a).normalized()
    for k, sign in enumerate((1.0, -1.0)):
        d = (da + db * sign).normalized()
        e = n
        # Each timber cut by the plane (d, e) is an ellipse: its radius along e, stretched along d.
        ka = 1.0 / max(math.sqrt(max(1.0 - d.dot(da) ** 2, 1e-4)), 1e-3)
        kb = 1.0 / max(math.sqrt(max(1.0 - d.dot(db) ** 2, 1e-4)), 1e-3)
        pts = []
        for i in range(count):
            ang = 2.0 * math.pi * i / count
            ud, ue = math.cos(ang), math.sin(ang)
            u = d * ud + e * ue
            best = None
            for c, r, kk in ((a, mast_r + rope_r, ka), (b, yard_r + rope_r, kb)):
                # The support point, in direction u, of the ellipse with semi-axes A (along d) and B (along e).
                A, B = r * kk, r
                norm = math.sqrt((A * ud) ** 2 + (B * ue) ** 2) or 1.0
                p = c + d * (A * A * ud / norm) + e * (B * B * ue / norm)
                if best is None or p.dot(u) > best[0]:
                    best = (p.dot(u), p)
            pts.append(tuple(best[1]))
        parts.append(rope(pts, rope_r, seed=seed + k, closed=True))
    return parts


# --- The nest, feathers, charms ---

def feather(root, direction, across, length, width, seed, curl=0.12):
    """A black feather (Hob's crows'): a thin vane with a raised shaft, both faces, its quill at root pointing along
    direction, its vanes spread along across (the left one narrower), curving a little toward its face."""
    rnd = random.Random(seed)
    d = Vector(direction).normalized()
    side = Vector(across)
    side = (side - d * side.dot(d)).normalized()
    face = side.cross(d).normalized()
    # A flight feather: a bare quill, then the vane, widest past its middle, narrowing to a rounded point.
    stations = (0.0, 0.16, 0.5, 0.86)
    halves = (0.1, 0.78, 1.0, 0.66)
    left, right = 0.4 * rnd.uniform(0.85, 1.05), 0.6 * rnd.uniform(0.95, 1.1)
    bend = curl * rnd.uniform(0.6, 1.3)
    bm = lp.new_bmesh()
    rows = []
    for t, hw in zip(stations, halves):
        c = Vector(root) + d * (length * t) + face * (bend * length * t * t)
        w = width * hw
        rows.append((bm.verts.new(c - side * (w * left)), bm.verts.new(c + face * 0.0018),
                     bm.verts.new(c + side * (w * right)), bm.verts.new(c - face * 0.0018)))
    tip = bm.verts.new(Vector(root) + d * length + face * (bend * length))
    for r0, r1 in zip(rows, rows[1:]):
        bm.faces.new((r0[0], r0[1], r1[1], r1[0]))
        bm.faces.new((r0[1], r0[2], r1[2], r1[1]))
        bm.faces.new((r0[2], r0[3], r1[3], r1[2]))
        bm.faces.new((r0[3], r0[0], r1[0], r1[3]))
    last = rows[-1]
    for j in range(4):
        bm.faces.new((last[j], last[(j + 1) % 4], tip))
    bm.faces.new((rows[0][3], rows[0][2], rows[0][1], rows[0][0]))
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces)
    part = lp.mesh_object(bm)
    return tiled(part, FEATHERS, 'Polymer', seed)


def blob(center, radii, seed, rot=(0.0, 0.0, 0.0), segs=6, rings=4, mat=None, set_name='Polymer', density=BONE_DENSITY,
         lumpy=0.12):
    """A small lumpy ellipsoid (a knucklebone, a skull's vault, a coal), on bone unless mat says otherwise. Built here,
    poles, rings and faces in a fixed order: bmesh's own UV sphere welds its poles in hash order, so its faces came out
    in a different order on every run."""
    bm = lp.new_bmesh()
    top = bm.verts.new((0.0, 0.0, 1.0))
    bands = []
    for j in range(1, rings):
        t = math.pi * j / rings
        bands.append([bm.verts.new((math.cos(2.0 * math.pi * i / segs) * math.sin(t),
                                    math.sin(2.0 * math.pi * i / segs) * math.sin(t), math.cos(t))) for i in range(segs)])
    bottom = bm.verts.new((0.0, 0.0, -1.0))
    for i in range(segs):
        i1 = (i + 1) % segs
        bm.faces.new((top, bands[0][i], bands[0][i1]))
        for upper, lower in zip(bands, bands[1:]):
            bm.faces.new((upper[i], lower[i], lower[i1], upper[i1]))
        bm.faces.new((bottom, bands[-1][i1], bands[-1][i]))
    off = Vector((seed * 1.7, seed * 0.3, seed * 2.9))
    for v in bm.verts:
        k = 1.0 + lumpy * noise.noise(v.co * 1.6 + off)
        v.co = Vector((v.co.x * radii[0], v.co.y * radii[1], v.co.z * radii[2])) * k
    part = lp.mesh_object(bm)
    lp.place(part, center, rot)
    return tiled(part, mat or BONE, set_name, seed, density=density if mat is None else None)


def crow_skull(at, facing, seed, scale=1.0):
    """A crow's skull: the vault, the long beak, hung by a twine through the eye sockets."""
    f = flat(facing).normalized()
    yaw = math.degrees(math.atan2(f.y, f.x))
    parts = [blob(at, (0.03 * scale, 0.022 * scale, 0.022 * scale), seed, rot=(0.0, 0.0, yaw), segs=8, rings=5)]
    # The beak: a cone to one point (a tube's tip ring would be all slivers), drooping a little.
    beak, _ = lp.lathe([(0.0, 0.0), (0.011 * scale, 0.0), (0.008 * scale, 0.03 * scale), (0.0, 0.068 * scale)], segments=5)
    pitch = 90.0 + 10.0
    lp.place(beak, tuple(Vector(at) + f * (0.016 * scale)), (pitch, 0.0, yaw + 90.0))
    parts.append(tiled(beak, BONE, 'Polymer', seed + 1, density=BONE_DENSITY))
    return parts


def charm_string(top, length, seed, items=('knuckle', 'knuckle', 'skull'), sway=(0.02, 0.01)):
    """Twine hanging from top with charms tied along it: knucklebones, a crow's skull at the end, or a feather."""
    rnd = random.Random(seed)
    top = Vector(top)
    end = top + Vector((sway[0], sway[1], -length))
    parts = [rope([tuple(top), tuple(top.lerp(end, 0.5) + Vector((sway[1], -sway[0], 0.0))), tuple(end)], 0.0045,
                  sides=3, seed=seed)]
    for k, item in enumerate(items):
        t = (k + 1) / len(items)
        p = top.lerp(end, t)
        if item == 'knuckle':
            parts.append(blob(p, (0.02, 0.015, 0.026), seed + 10 + k, rot=(rnd.uniform(-20, 20), rnd.uniform(-20, 20),
                                                                            rnd.uniform(0, 180))))
        elif item == 'skull':
            facing = Vector((math.cos(rnd.uniform(0, 6.28)), math.sin(rnd.uniform(0, 6.28)), 0.0))
            parts += crow_skull(p - UP * 0.02, facing, seed + 20 + k)
        elif item == 'feather':
            parts.append(feather(p, (rnd.uniform(-0.2, 0.2), rnd.uniform(-0.2, 0.2), -1.0), (1.0, 0.3, 0.0),
                                 rnd.uniform(0.2, 0.26), 0.032, seed + 30 + k))
    return parts


def nest(center, r_out, depth, seed, sticks=34, rim_feathers=6, avoid=None, clear=()):
    """A crows' nest of sticks: a ragged bowl woven of grass and twigs, oak sticks round its wall and poking out every
    way past its rim and under it, black feathers tucked in. center: the middle of its base. avoid: an upright timber's
    ((x, y), r) the nest is built round (sticks keep out of it). clear: perch points the sticks and feathers keep away
    from, so a crow there never stands in them."""
    rnd = random.Random(seed)
    c = Vector(center)
    # The bowl: its outer wall, the rim, the inner wall down to the cup.
    profile = [(0.0, 0.0), (r_out * 0.45, -0.03), (r_out * 0.84, depth * 0.3), (r_out, depth * 0.7), (r_out * 0.96, depth),
               (r_out * 0.78, depth * 1.02), (r_out * 0.52, depth * 0.6), (0.0, depth * 0.5)]
    shell, bands = lp.lathe(profile, segments=16)
    wall = [i for i, b in enumerate(bands) if b < 6]
    cup = [i for i, b in enumerate(bands) if b >= 6]
    # Woven of grass and fine twigs, the straw's strands running round the wall (the cup across: one material, as
    # nobody sees in from below).
    lt.assign(shell, NEST)
    lp.lathe_uv(shell, 'Hay', faces=wall, grain='up', seed=seed)
    lp.lathe_uv(shell, 'Hay', faces=cup, seed=seed + 1)
    lr.split_poles(shell, 'around')
    # Ragged: every wall and rim vertex pushed about, the rim most.
    off = Vector((seed * 0.7, seed * 1.9, 3.0))
    for v in shell.data.vertices:
        r = math.hypot(v.co.x, v.co.y)
        if r > 1e-4:
            k = noise.noise(Vector((v.co.x * 4.0, v.co.y * 4.0, v.co.z * 4.0)) + off)
            v.co.x *= 1.0 + 0.12 * k
            v.co.y *= 1.0 + 0.12 * k
            v.co.z += 0.06 * noise.noise(Vector((v.co.x * 6.0, v.co.y * 6.0, 1.0)) + off) * max(v.co.z / depth, 0.0)
    lp.place(shell, tuple(c))
    parts = [shell]

    def clear_of_mast(p):
        for q in clear:
            # A perched crow fills about 0.3 m round its feet and 0.4 m up.
            if math.hypot(p.x - q[0], p.y - q[1]) < 0.3 and -0.12 < p.z - q[2] < 0.42:
                return False
        if avoid is None:
            return True
        (mx, my), mr = avoid
        return math.hypot(p.x - mx, p.y - my) > mr + 0.03

    # Sticks round the wall (woven, nearly level), out past the rim and hanging under it (the ragged silhouette).
    for k in range(sticks):
        a = 2.0 * math.pi * (k + rnd.uniform(-0.4, 0.4)) / sticks
        out = Vector((math.cos(a), math.sin(a), 0.0))
        tang = Vector((-math.sin(a), math.cos(a), 0.0))
        if k % 7 == 3:
            # Hanging under the bowl's lower edge.
            z = depth * rnd.uniform(0.05, 0.3)
            p0 = c + out * (r_out * rnd.uniform(0.6, 0.82)) + UP * z
            p2 = p0 + (out * rnd.uniform(0.5, 0.9) + tang * rnd.uniform(-0.4, 0.4) - UP * rnd.uniform(0.5, 1.0)).normalized() \
                * rnd.uniform(0.25, 0.45)
            pts = [p0, p0.lerp(p2, 0.5) + out * 0.02, p2]
        elif k % 3 == 2:
            # Out past the rim, leaning out (a few up).
            z = depth * rnd.uniform(0.55, 0.95)
            r0 = r_out * rnd.uniform(0.55, 0.8)
            length = rnd.uniform(0.35, 0.7)
            p0 = c + out * r0 + UP * z
            p2 = p0 + (out * rnd.uniform(0.75, 1.0) + tang * rnd.uniform(-0.5, 0.5) + UP * rnd.uniform(-0.15, 0.55)).normalized() * length
            p1 = p0.lerp(p2, 0.5) + UP * rnd.uniform(-0.03, 0.03)
            pts = [p0, p1, p2]
        else:
            # Round the wall: a chord across the bowl's outside, bent to it.
            z = depth * rnd.uniform(0.15, 0.85)
            rr = r_out * (0.86 + 0.14 * min(z / depth / 0.78, 1.0)) + 0.012
            span = rnd.uniform(0.5, 0.95)
            tilt = rnd.uniform(-0.12, 0.12)
            pts = []
            for j in range(3):
                aa = a + span * (j - 1)
                pts.append(c + Vector((math.cos(aa) * rr * (1.03 if j == 1 else 1.0), math.sin(aa) * rr * (1.03 if j == 1 else 1.0),
                                       z + tilt * (j - 1) * 0.3)))
            # The ends run on past the bowl a little.
            pts[0] = pts[1] + (pts[0] - pts[1]) * 1.25
            pts[2] = pts[1] + (pts[2] - pts[1]) * 1.25
        radius = rnd.uniform(0.014, 0.022)
        # Checked at the ends and between them.
        if not all(clear_of_mast(p) for p in pts + [pts[0].lerp(pts[-1], 0.25), pts[0].lerp(pts[-1], 0.75)]):
            continue
        parts.append(stick([tuple(p) for p in pts], radius, seed + 100 + k))
    for k in range(rim_feathers):
        a = 2.0 * math.pi * (k + rnd.uniform(-0.3, 0.3)) / rim_feathers + 0.4
        out = Vector((math.cos(a), math.sin(a), 0.0))
        root = c + out * (r_out * 0.86) + UP * (depth * 0.8)
        direction = out * rnd.uniform(0.4, 0.9) + UP * rnd.uniform(0.2, 0.8) + Vector((-out.y, out.x, 0.0)) * rnd.uniform(-0.4, 0.4)
        length, width = rnd.uniform(0.2, 0.28), rnd.uniform(0.03, 0.038)
        if not clear_of_mast(root) or not clear_of_mast(root + direction.normalized() * length):
            continue
        parts.append(feather(tuple(root), tuple(direction), (-out.y, out.x, 0.2), length, width, seed + 200 + k))
    return parts


def cresset(center, r0, r1, height, seed, bars=8):
    """The beacon's iron fire basket: a pan on a socket over the timber's top, bars flaring up and out from the pan's
    rim to a ring, curling out at their tips like a flame, and a few cold coals in the pan (the code lights it)."""
    rnd = random.Random(seed)
    c = Vector(center)
    parts = []
    pan, _ = lr.lathe([(0.0, -0.14), (0.088, -0.14), (0.092, -0.02), (r0 * 1.05, 0.0), (r0 * 1.12, 0.035), (r0 * 0.96, 0.04),
                       (0.0, 0.02)], 10, 'ironblack', set_name='MetalWorn', seed=seed, center=tuple(c))
    parts.append(pan)
    for k in range(bars):
        a = 2.0 * math.pi * (k + 0.5) / bars
        out = Vector((math.cos(a), math.sin(a), 0.0))
        pts = [c + out * (r0 * 1.06) + UP * 0.03, c + out * (r0 + (r1 - r0) * 0.45) + UP * (height * 0.45),
               c + out * (r1 * 0.98) + UP * (height * 0.86), c + out * (r1 * 1.12) + UP * (height * 1.02),
               c + out * (r1 * 1.2) + UP * (height * 0.96)]
        parts.append(iron_bar([tuple(p) for p in pts], 0.011, seed + k, radii=[0.012, 0.011, 0.01, 0.008, 0.005],
                              caps=(True, True)))
    for z, r in ((height * 0.45, r0 + (r1 - r0) * 0.45 + 0.008), (height * 0.86, r1 * 0.98 + 0.008)):
        ring = [tuple(c + Vector((math.cos(t) * r, math.sin(t) * r, z))) for t in (2.0 * math.pi * k / 12 for k in range(13))]
        parts.append(iron_bar(ring, 0.009, seed + 20 + int(z * 100), caps=(False, False)))
    # Cold coals: seen only from above, so few and plain.
    for k in range(3):
        a = 2.0 * math.pi * k / 3 + rnd.uniform(-0.3, 0.3)
        rr = r0 * rnd.uniform(0.2, 0.55)
        parts.append(blob(tuple(c + Vector((math.cos(a) * rr, math.sin(a) * rr, 0.045))), (0.05, 0.04, 0.032), seed + 40 + k,
                          rot=(0.0, 0.0, rnd.uniform(0, 180)), segs=5, rings=3, mat=FEATHERS,
                          set_name='Polymer', lumpy=0.25))
    return parts


def horseshoe(at, axis_c, radius, seed):
    """An iron horseshoe nailed to a round timber (axis through axis_c, upright) at `at`, its prongs up for luck,
    bent round the timber."""
    a_c = Vector((axis_c[0], axis_c[1], 0.0))
    out = flat(Vector(at) - a_c).normalized()
    side = Vector((-out.y, out.x, 0.0))
    pts = []
    for k in range(11):
        t = math.radians(-150.0 + 300.0 * k / 10)
        x, z = math.sin(t) * 0.06, -math.cos(t) * 0.065
        # Bent round the timber's face.
        ang = x / (radius + 0.006)
        p = a_c + (out * math.cos(ang) + side * math.sin(ang)) * (radius + 0.006) + UP * (Vector(at).z + z)
        pts.append(tuple(p))
    # A flat shoe: wide in its own plane, thin off the timber.
    part = lp.sweep(pts, [(-0.012, -0.004), (0.012, -0.004), (0.012, 0.004), (-0.012, 0.004)], up=tuple(out))
    return [tiled(part, lr.material('ironblack'), 'MetalWorn', seed)]


def cairn(radius, seed, count=9, height=(0.18, 0.34), avoid_r=0.14):
    """Granite stones packed round a post's foot: a ring of big ones, and (from nine stones up) a few smaller ones
    wedged on them against the post, so the foot reads as built, not fallen."""
    rnd = random.Random(seed)
    parts = []
    outer = count if count < 10 else count - count // 3
    for k in range(count):
        if k < outer:
            a = 2.0 * math.pi * (k + rnd.uniform(-0.25, 0.25)) / outer
            r = rnd.uniform(avoid_r + 0.16, radius)
            s = rnd.uniform(*height)
            z = -0.06
        else:
            a = 2.0 * math.pi * (k - outer + 0.5 + rnd.uniform(-0.2, 0.2)) / (count - outer)
            r = avoid_r + rnd.uniform(0.06, 0.14)
            s = rnd.uniform(height[0] * 0.7, height[0] * 1.1)
            z = rnd.uniform(0.06, 0.12)
        size = (s * rnd.uniform(1.2, 1.7), s * rnd.uniform(1.0, 1.4), s)
        parts.append(lr.round_stone(size, (math.cos(a) * r, math.sin(a) * r, z),
                                    (rnd.uniform(-8, 8), rnd.uniform(-8, 8), math.degrees(a) + rnd.uniform(-30, 30)),
                                    seed=seed + k, tris=46 if k < outer else 30, key='granite'))
    return parts


def brand_board(axis_xy, mast_r, z, seed, width=0.24, height=0.32, thick=0.03):
    """A weathered board nailed to a round post's front at height z, the roost's mark burnt into it: a crow's foot
    (three toes forward, one back). The same mark on every roost, and a fair map icon for them."""
    ax = Vector((axis_xy[0], axis_xy[1], 0.0))
    yc = ax.y - mast_r * 0.97 - thick * 0.5
    board = lp.block((width, thick, height), (ax.x, yc, z), bevel=0.006)
    lp.grain(board, 'Siding', axis=(0.0, 0.0, 1.0), seed=seed)
    parts = [board]
    face = yc - thick * 0.5
    for dx, dz in ((-0.088, 0.125), (0.088, 0.128), (-0.09, -0.124), (0.086, -0.127)):
        parts.append(lp.nail((ax.x + dx, face, z + dz), (0.0, -1.0, 0.0), size=0.018))
    foot = Vector((ax.x, face - 0.0015, z - 0.035))
    for (dx, dz), w in (((0.0, 0.118), 0.018), ((-0.072, 0.09), 0.016), ((0.072, 0.09), 0.016), ((0.0, -0.07), 0.016)):
        length = math.hypot(dx, dz)
        angle = math.degrees(math.atan2(dx, dz))
        mid = foot + Vector((dx * 0.5, 0.0, dz * 0.5))
        stroke = lp.block((w, 0.004, length + w * 0.7), tuple(mid), (0.0, angle, 0.0), bevel=0.0015)
        parts.append(tiled(stroke, FEATHERS, 'Polymer', seed + int(angle)))
    return parts


def bell_hanger(top, drop, seed):
    """The bell's iron hanger under a yard: a rod down from the yard's underside and a hook at its end for the bell's
    loop (SOCKET_Bell goes at drop below top)."""
    t = Vector(top)
    pts = [t, t - UP * (drop * 0.5) + Vector((0.006, 0.0, 0.0)), t - UP * drop]
    hook = [tuple(t - UP * drop + Vector((0.0, 0.012 * math.sin(a), 0.012 * math.cos(a) - 0.012)))
            for a in (k * math.pi / 5 for k in range(8))]
    return [iron_bar([tuple(p) for p in pts], 0.006, seed), iron_bar(hook, 0.005, seed + 1)]


# --- The directions ---

class Roost:
    """One direction's model being put together: an lr.Model with the sockets as named points, and the facts the
    renders need."""

    def __init__(self, name, seed):
        self.m = lr.Model(name, seed=seed)
        self.name = name
        self.perches = []

    def add(self, parts, smooth=None):
        for p in parts if isinstance(parts, (list, tuple)) else [parts]:
            self.m.add(p, smooth=smooth)

    def perch(self, at, facing):
        self.perches.append((tuple(at), yaw_of(facing)))

    def finish(self, beacon, bell, wake, speaker, post_hull, foot_hull):
        m = self.m
        m.socket('Beacon', tuple(beacon))
        m.socket('Bell', tuple(bell))
        for k, (at, yaw) in enumerate(self.perches[:6]):
            m.socket(f'Perch_{k + 1}', at, (0.0, 0.0, yaw))
        m.socket('Wake', tuple(wake))
        m.socket('Speaker', tuple(speaker[0]), (0.0, 0.0, yaw_of(speaker[1])))
        m.hull_points([tuple(p) for p in post_hull])
        m.hull_points([tuple(p) for p in foot_hull])
        obj = m.finish(ao=True, ground=True, preview=False, fallback=100.0, ao_distance=0.7)
        print(f'ROOST: {obj.name}: sockets', ', '.join(
            f"{c.name[7:]} ({c.location.x:.3f}, {c.location.y:.3f}, {c.location.z:.3f}) yaw {math.degrees(c.rotation_euler.z):.0f}"
            for c in sorted(obj.children, key=lambda o: o.name) if c.name.startswith('SOCKET_')), flush=True)
        return obj


def prism_hull(center_xy, radius, z0, z1, sides=8):
    """The corners of an upright octagonal prism (a hull round a post or a cairn)."""
    pts = []
    for z in (z0, z1):
        for k in range(sides):
            a = 2.0 * math.pi * (k + 0.5) / sides
            pts.append((center_xy[0] + math.cos(a) * radius, center_xy[1] + math.sin(a) * radius, z))
    return pts


def on_top(points, radii, s):
    """The top of a round timber (points along its axis, radii at them) s metres along it from its first point."""
    pts = [Vector(p) for p in points]
    acc = 0.0
    for i in range(len(pts) - 1):
        seg = (pts[i + 1] - pts[i]).length
        if acc + seg >= s or i == len(pts) - 2:
            t = min(max((s - acc) / seg, 0.0), 1.0)
            return pts[i].lerp(pts[i + 1], t) + UP * (radii[i] + (radii[i + 1] - radii[i]) * t)
        acc += seg


def length_of(points):
    return sum((Vector(b) - Vector(a)).length for a, b in zip(points, points[1:]))


def yard_frame(mast_at, mast_r, yard_r, yaw, side):
    """Where a yard crosses a timber: its direction (yaw degrees from +X), the side it's lashed on (+1 toward the
    direction's left, -1 its right) and the point on its axis next to the timber."""
    u = Vector((math.cos(math.radians(yaw)), math.sin(math.radians(yaw)), 0.0))
    n = Vector((-u.y, u.x, 0.0)) * side
    return u, n, Vector(mast_at) + n * (mast_r + yard_r)


def direction_b(name):
    """B, the Crow's Mast."""
    r = Roost(name, SEED + 2)
    rnd = random.Random(SEED + 2)
    # The lower mast, sunk 0.2 m in the cairn, leaning a touch east; the topmast fished to its back.
    low_pts = [(0.0, 0.0, -0.2), (0.005, 0.0, 0.8), (0.014, 0.003, 1.8), (0.024, 0.006, 2.6), (0.03, 0.008, 3.12)]
    low_r = [0.13, 0.125, 0.118, 0.11, 0.105]
    r.add(timber(low_pts, low_r, sides=9, strip=MAST_WOOD, seed=SEED + 3, up=(0.0, 1.0, 0.0)))
    tm0 = (0.026, 0.198, 2.42)
    tm1 = (0.05, 0.21, 4.52)
    top_pts = [tm0, (0.034, 0.202, 3.1), (0.042, 0.206, 3.8), tm1]
    top_r = [0.082, 0.08, 0.075, 0.07]
    r.add(timber(top_pts, top_r, sides=8, strip=MAST_WOOD, seed=SEED + 4, up=(0.0, 1.0, 0.0)))
    # Two iron bands round the doubling, and one low on the mast over the cairn.
    for z in (2.56, 2.98):
        low_at = Vector((0.024 + (z - 2.6) * 0.012, 0.006, z))
        top_at = Vector((0.03 + (z - 2.42) * 0.011, 0.2, z))
        r.add(band(hull_outline([((low_at.x, low_at.y), 0.112), ((top_at.x, top_at.y), 0.083)], count=26, z=z), 0.05,
                   0.008, SEED + int(z * 10)))
    r.add(iron_ring((0.004, 0.0, 0.52), 0.127, 0.06, 0.008, SEED + 7))
    # The yard, lashed across the mast's front and cockbilled (tilted, as a ship's yards are in mourning): its west
    # end high, its east end low. The tilt is the roost's signature: no cross, no telegraph pole.
    yard_z = 2.45
    u, n, p = yard_frame((0.021, 0.005, yard_z), 0.112, 0.05, 8.0, -1.0)
    tilt = math.radians(17.0)
    ut = (u * math.cos(tilt) - UP * math.sin(tilt)).normalized()
    y0 = p - ut * 1.05
    y2 = p + ut * 1.15
    bow = Vector((0.0, 0.0, 0.012))
    yard_pts = [y0, p.lerp(y0, 0.5) + bow, p, p.lerp(y2, 0.5) + bow, y2]
    yard_r = [0.036, 0.046, 0.052, 0.046, 0.034]
    r.add(timber([tuple(q) for q in yard_pts], yard_r, sides=7, strip=MAST_WOOD, seed=SEED + 8))
    r.add(lashing((0.021, 0.005, yard_z), 0.112, (0.0, 0.0, 1.0), tuple(p), 0.052, tuple(ut), SEED + 9))
    r.add(helix((0.021, 0.005), 0.122, yard_z + 0.1, yard_z + 0.16, 2.0, SEED + 10))
    r.add(helix((0.021, 0.005), 0.122, yard_z - 0.17, yard_z - 0.11, 2.0, SEED + 11))
    # The nest's bed: a short yard across the topmast (nearly north-south) and a chock stick under the nest.
    bed_z = 3.17
    u2, n2, p2 = yard_frame((0.036, 0.203, bed_z), 0.08, 0.042, 96.0, 1.0)
    b0, b2 = p2 - u2 * 1.14, p2 + u2 * 1.12
    bed_pts, bed_r = [b0, p2 + UP * 0.006, b2], [0.032, 0.044, 0.03]
    r.add(timber([tuple(q) for q in bed_pts], bed_r, sides=6, strip=MAST_WOOD, seed=SEED + 12))
    r.add(lashing((0.036, 0.203, bed_z), 0.08, (0.0, 0.0, 1.0), tuple(p2), 0.044, tuple(u2), SEED + 13))
    chock_a = Vector((0.036, 0.203, bed_z + 0.02)) - n2 * 0.13
    r.add(stick([tuple(chock_a - Vector((0.5, 0.06, 0.0))), tuple(chock_a), tuple(chock_a + Vector((0.48, 0.05, 0.01)))],
                0.026, SEED + 14, sides=5, taper=0.8))
    # The nest, round the topmast on its bed: big enough to read as a dark mass from far off. Its sticks keep clear of
    # the crows' places on the bed's ends and its rim, and of Hob's on its front rim.
    nest_c = Vector((0.04, 0.2, bed_z + 0.04))
    nest_r, nest_d = 0.66, 0.42
    side2 = Vector((-u2.y, u2.x, 0.0))
    bed_perches = [(on_top(bed_pts, bed_r, length_of(bed_pts) - 0.07), -side2 + u2 * 0.4),
                   (on_top(bed_pts, bed_r, 0.06), side2 - u2 * 0.4)]
    rim_out = Vector((math.cos(math.radians(-30.0)), math.sin(math.radians(-30.0)), 0.0))
    rim_perch = (nest_c + rim_out * (nest_r * 0.9) + UP * nest_d, rim_out)
    speaker = (nest_c + Vector((0.0, -nest_r * 0.88, nest_d + 0.01)), (0.0, -1.0, 0.0))
    clear = [tuple(q) for q, _ in bed_perches + [rim_perch, speaker]]
    r.add(nest(tuple(nest_c), nest_r, nest_d, SEED + 15, sticks=42, rim_feathers=7, avoid=((0.04, 0.204), 0.075),
               clear=clear))
    # The crosstree under the masthead, across the yard's line, and the cresset on top.
    cross_z = 4.1
    u3, n3, p3 = yard_frame((0.047, 0.208, cross_z), 0.072, 0.035, 100.0, -1.0)
    c0, c2 = p3 - u3 * 0.5, p3 + u3 * 0.46
    cross_pts, cross_r = [c0, p3, c2], [0.03, 0.036, 0.029]
    r.add(timber([tuple(q) for q in cross_pts], cross_r, sides=6, strip=MAST_WOOD, seed=SEED + 16))
    r.add(lashing((0.047, 0.208, cross_z), 0.072, (0.0, 0.0, 1.0), tuple(p3), 0.036, tuple(u3), SEED + 17, rope_r=0.009))
    cres_c = Vector((0.05, 0.21, 4.56))
    r.add(cresset(tuple(cres_c), 0.13, 0.27, 0.4, SEED + 18))
    # Furniture at eye height: the roost's mark burnt into a board, the horseshoe over it, a whipping of twine with
    # feathers in it.
    r.add(brand_board((0.008, 0.0), 0.121, 1.3, SEED + 22))
    r.add(horseshoe((0.0, -0.12, 1.68), (0.012, 0.0), 0.12, SEED + 19))
    r.add(helix((0.01, 0.002), 0.124, 1.86, 1.94, 3.0, SEED + 20, rope_r=0.006))
    for k, (ang, z, tilt) in enumerate(((-100.0, 1.9, 0.5), (-70.0, 1.88, 0.35), (-125.0, 1.91, 0.7), (160.0, 1.89, 0.4))):
        a = math.radians(ang)
        out = Vector((math.cos(a), math.sin(a), 0.0))
        root = Vector((0.01, 0.002, z)) + out * 0.125
        r.add(feather(tuple(root), tuple(out * 0.35 + UP * tilt + Vector((-out.y, out.x, 0.0)) * rnd.uniform(-0.3, 0.3)),
                      (-out.y, out.x, 0.0), rnd.uniform(0.2, 0.27), 0.032, SEED + 30 + k))
    # Feathers in the yard's lashing.
    for k, (ang, lift) in enumerate(((-80.0, 0.8), (-110.0, 0.6), (-55.0, 0.9))):
        a = math.radians(ang)
        out = Vector((math.cos(a), math.sin(a), 0.0))
        root = p + out * 0.06 + UP * 0.06
        r.add(feather(tuple(root), tuple(out * 0.4 + UP * lift), (-out.y, out.x, 0.1), rnd.uniform(0.22, 0.3), 0.034,
                      SEED + 40 + k))
    # Charms on twine from the yard's high west end; the bell's hook toward its low east end.
    r.add(charm_string(tuple(on_top(yard_pts, yard_r, 0.14) - UP * 0.075), 0.4, SEED + 50))
    r.add(charm_string(tuple(on_top(yard_pts, yard_r, 0.55) - UP * 0.09), 0.24, SEED + 55, items=('feather',), sway=(0.0, 0.01)))
    bell_top = on_top(yard_pts, yard_r, 1.05 + 0.82) - UP * 0.078
    r.add(bell_hanger(tuple(bell_top), 0.1, SEED + 60))
    bell = bell_top - UP * 0.1
    # The perches: the yard's ends, the nest bed's ends, the nest's rim, the crosstree's end.
    side = Vector((-u.y, u.x, 0.0))
    yard_len = length_of(yard_pts)
    r.perch(on_top(yard_pts, yard_r, 0.32), -side + u * -0.4)
    r.perch(on_top(yard_pts, yard_r, yard_len - 0.62), side + u * 0.45)
    for at, facing in bed_perches + [rim_perch]:
        r.perch(at, facing)
    side3 = Vector((-u3.y, u3.x, 0.0))
    r.perch(on_top(cross_pts, cross_r, 0.07), side3 - u3 * 0.5)
    r.add(cairn(0.66, SEED + 21, count=12))
    post = [(x, y, z) for x, y, z in prism_hull((0.02, 0.04), 0.2, 0.0, 3.1)] + \
        [(x, y, z) for x, y, z in prism_hull((0.045, 0.205), 0.11, 3.1, 4.6)]
    foot = prism_hull((0.0, 0.0), 0.74, 0.0, 0.3)
    obj = r.finish(cres_c + UP * 0.2, bell, (0.0, -1.7, 0.0), speaker, post, foot)
    return obj, r


def direction_a(name):
    """A, the Keeper's Crook."""
    r = Roost(name, SEED + 1)
    rnd = random.Random(SEED + 1)
    m = r.m
    post_top = 3.8
    w = 0.26
    # The hewn post (the trim sheet's beam faces), sunk in the cairn, a little out of true.
    matrix = kit.toward((0.0, 0.0, -0.2), (0.02, 0.01, post_top), (0.0, -1.0, 0.0)) @ Matrix.Translation(((post_top + 0.2) * 0.5, 0.0, 0.0))
    m.emit(kit._box((post_top + 0.2, w, w), 0.012, 3), Trim('C', lane='each', width=w), 'trim', matrix)
    # A cap board on the post's head.
    m.box((w + 0.06, w + 0.06, 0.05), at=(0.02, 0.01, post_top + 0.02), uv=Trim('C', lane='each', width=w + 0.06), bevel=0.01)
    # The crossbars: the long one east-west low, the short one north-south above (never a cross from the front).
    def hewn(p0, p1, size, seed):
        length = (Vector(p1) - Vector(p0)).length
        mat = kit.toward(p0, p1, (0.0, 0.0, 1.0)) @ Matrix.Translation((length * 0.5, 0.0, 0.0))
        m.emit(kit._box((length, size, size), 0.008, 1), Trim('C', lane='each', width=size), 'trim', mat)
    bar1_z, bar2_z = 2.45, 3.2
    a0, a1 = Vector((-0.85, -0.2, bar1_z + 0.02)), Vector((1.1, -0.2, bar1_z - 0.03))
    hewn(a0, a1, 0.11, SEED + 21)
    b0, b1 = Vector((0.2, -0.66, bar2_z)), Vector((0.2, 0.8, bar2_z + 0.03))
    hewn(b0, b1, 0.1, SEED + 22)
    # One knee brace under the long bar's longer arm.
    hewn((0.13, -0.2, bar1_z - 0.66), (0.66, -0.2, bar1_z - 0.06), 0.075, SEED + 23)
    # Iron straps where the bars cross the post.
    m.box((0.3, 0.012, 0.05), at=(0.02, -0.265, bar1_z + 0.09), uv=Trim('H3', fit=True))
    m.box((0.012, 0.3, 0.05), at=(0.14, 0.0, bar2_z + 0.085), uv=Trim('H3', fit=True))
    # The crook: a square iron bar up out of the cap, round in a wide hook forward, the beacon's fire basket hanging
    # from its end on three chains (the keepers' lantern crook, grown to a landmark).
    # It hooks out east, so from the front it reads as a crook, not a handle.
    crook = [(0.02, 0.01, post_top - 0.15), (0.02, 0.01, post_top + 0.3), (0.02, 0.01, post_top + 0.6)]
    R, cz = 0.42, post_top + 0.6
    for k in range(1, 10):
        t = math.pi * k / 9
        crook.append((0.02 + R - R * math.cos(t), 0.01, cz + R * math.sin(t)))
    tip = Vector((0.02 + 2 * R, 0.01, cz - 0.1))
    crook.append(tuple(tip))
    r.add(iron_bar(crook, 0.03, SEED + 24, radii=[0.034, 0.033, 0.032] + [0.03] * 9 + [0.024]))
    # A scroll curled off the crook's back (the smith's flourish), and a stay from the post to the hook.
    scroll = [(0.02 - 0.06 * math.sin(a), 0.01, post_top + 0.38 + 0.06 * math.cos(a) * (1.0 - a / 9.0))
              for a in (k * 0.4 for k in range(14))]
    r.add(iron_bar(scroll, 0.012, SEED + 25, caps=(True, True)))
    r.add(iron_bar([(0.05, 0.01, post_top + 0.15), (0.44, 0.01, post_top + 0.62)], 0.014, SEED + 26))
    # The fire basket on its chains (three bars from a ring under the hook's end).
    ring_at = tip - UP * 0.1
    r.add(iron_bar([tuple(tip), tuple(ring_at)], 0.01, SEED + 27))
    basket_c = ring_at - UP * 0.55
    r.add(cresset(tuple(basket_c), 0.13, 0.24, 0.4, SEED + 28, bars=8))
    for k in range(3):
        t = 2.0 * math.pi * k / 3 + 0.5
        r.add(iron_bar([tuple(ring_at), tuple(basket_c + Vector((math.cos(t) * 0.22, math.sin(t) * 0.22, 0.35)))], 0.006,
                       SEED + 29 + k))
    beacon = basket_c + UP * 0.2
    # The nest on the post's head beside the crook, on the cap and the upper bar.
    nest_c = Vector((-0.1, 0.1, post_top + 0.03))
    a_rim = Vector((math.cos(math.radians(160.0)), math.sin(math.radians(160.0)), 0.0))
    r.add(nest(tuple(nest_c), 0.52, 0.34, SEED + 31, sticks=38, rim_feathers=6, avoid=((0.02, 0.01), 0.04),
               clear=[tuple(nest_c + a_rim * 0.47 + UP * 0.34), tuple(nest_c + Vector((0.0, -0.47, 0.35)))]))
    # Furniture.
    r.add(brand_board((0.02, 0.0), 0.134, 1.3, SEED + 34))
    r.add(horseshoe((0.02, -0.14, 1.68), (0.02, 0.0), 0.13, SEED + 32))
    for k, (dx, z) in enumerate(((-0.06, 1.95), (0.07, 1.98), (0.0, 2.04))):
        r.add(feather((0.02 + dx, -0.135, z), (dx * 3.0, -0.5, 0.8), (1.0, 0.0, 0.0), rnd.uniform(0.2, 0.26), 0.032, SEED + 33 + k))
    r.add(charm_string(tuple(a0 + Vector((0.14, 0.0, -0.06))), 0.34, SEED + 40))
    r.add(charm_string(tuple(b1 - Vector((0.0, 0.1, 0.05))), 0.24, SEED + 45, items=('feather', 'knuckle')))
    bell_top = a1 - Vector((0.12, 0.0, 0.06))
    r.add(bell_hanger(tuple(bell_top), 0.1, SEED + 50))
    bell = bell_top - UP * 0.1
    r.perch(a0.lerp(a1, 0.12) + UP * 0.055, (-0.3, -1.0, 0.0))
    r.perch(a0.lerp(a1, 0.88) + UP * 0.055, (0.3, 1.0, 0.0))
    r.perch(b0.lerp(b1, 0.06) + UP * 0.05, (-1.0, -0.3, 0.0))
    r.perch(b0.lerp(b1, 0.94) + UP * 0.05, (1.0, 0.4, 0.0))
    rim_out = Vector((math.cos(math.radians(160.0)), math.sin(math.radians(160.0)), 0.0))
    r.perch(nest_c + rim_out * 0.47 + UP * 0.34, rim_out)
    r.perch(Vector((0.02 + R, 0.01, cz + R + 0.03)), (0.0, -1.0, 0.0))
    speaker = (nest_c + Vector((0.0, -0.47, 0.35)), (0.0, -1.0, 0.0))
    post = prism_hull((0.01, 0.0), 0.2, 0.0, post_top + 0.05)
    foot = prism_hull((0.0, 0.0), 0.62, 0.0, 0.3)
    r.add(cairn(0.6, SEED + 60))
    obj = r.finish(beacon, bell, (0.0, -1.7, 0.0), speaker, post, foot)
    return obj, r


def direction_c(name):
    """C, the Crow Tree."""
    r = Roost(name, SEED + 3)
    rnd = random.Random(SEED + 3)
    # The trunk: a dead oak, its bark on, flaring at its foot, forking at 3.4 m.
    trunk = [(0.0, 0.0, -0.25), (0.0, 0.0, 0.2), (0.04, 0.02, 1.0), (0.02, 0.05, 1.8), (0.08, 0.04, 2.6), (0.1, 0.06, 3.35)]
    tr = [0.3, 0.24, 0.19, 0.17, 0.155, 0.14]
    part = timber(trunk, tr, sides=11, strip='BarkOak', seed=SEED + 70, jitter=0.12, up=(0.0, 1.0, 0.0))
    lp.rough(part, 0.025, 2.5, SEED + 70)
    lt.assign(part, BARK)
    r.add(part)
    fork = Vector(trunk[-1])
    lead1 = [tuple(fork - UP * 0.15), (0.06, 0.1, 3.9), (0.02, 0.16, 4.35), (0.05, 0.18, 4.62)]
    lead2 = [tuple(fork - UP * 0.15), (0.35, 0.02, 3.75), (0.62, -0.06, 4.0), (0.78, -0.1, 4.12)]
    for pts, rr, s in ((lead1, [0.12, 0.1, 0.085, 0.075], 71), (lead2, [0.11, 0.08, 0.06, 0.045], 72)):
        part = timber(pts, rr, sides=8, strip='BarkOak', seed=SEED + s, jitter=0.1, up=(1.0, 0.0, 0.0) if s == 71 else (0.0, 0.0, 1.0))
        lp.rough(part, 0.015, 3.0, SEED + s)
        lt.assign(part, BARK)
        r.add(part)
    # Branches cut back to perches, their ends sawn.
    branches = [((0.02, 0.02, 2.2), -15.0, 0.95, 0.25, 0.07), ((0.06, 0.05, 2.75), 150.0, 0.8, 0.3, 0.06),
                ((0.06, 0.04, 2.55), 75.0, 0.6, 0.35, 0.055), ((0.09, 0.06, 3.15), -110.0, 0.7, 0.2, 0.05)]
    ends = []
    for k, (at, yaw, length, rise, rad) in enumerate(branches):
        a = math.radians(yaw)
        d = Vector((math.cos(a), math.sin(a), rise)).normalized()
        p0 = Vector(at)
        p2 = p0 + d * length
        p1 = p0.lerp(p2, 0.5) + UP * 0.05
        part = timber([tuple(p0), tuple(p1), tuple(p2)], [rad, rad * 0.85, rad * 0.72], sides=6, strip='BarkOak',
                      seed=SEED + 80 + k, jitter=0.1)
        lt.assign(part, BARK)
        r.add(part)
        ends.append((p0, p2, d, rad * 0.72))
    # Twine bands round the trunk with feathers and charms in them.
    r.add(helix((0.03, 0.03), 0.18, 1.82, 1.9, 3.0, SEED + 90, rope_r=0.007))
    for k, ang in enumerate((-100.0, -60.0, -135.0)):
        a = math.radians(ang)
        out = Vector((math.cos(a), math.sin(a), 0.0))
        r.add(feather(tuple(Vector((0.03, 0.03, 1.87)) + out * 0.18), tuple(out * 0.35 + UP * 0.6), (-out.y, out.x, 0.0),
                      rnd.uniform(0.2, 0.27), 0.032, SEED + 91 + k))
    r.add(brand_board((0.03, 0.03), 0.19, 1.28, SEED + 94))
    r.add(horseshoe((0.03, -0.155, 1.66), (0.03, 0.03), 0.18, SEED + 95))
    # The nest in the fork; the cresset on the taller leader.
    nest_c = Vector((0.12, 0.08, 3.38))
    c_rim = Vector((math.cos(math.radians(-20.0)), math.sin(math.radians(-20.0)), 0.0))
    r.add(nest(tuple(nest_c), 0.58, 0.38, SEED + 96, sticks=40, rim_feathers=6, avoid=((0.06, 0.1), 0.1),
               clear=[tuple(nest_c + c_rim * 0.52 + UP * 0.38), tuple(nest_c + Vector((-0.05, -0.52, 0.39)))]))
    cres_c = Vector(lead1[-1]) + UP * 0.02
    r.add(cresset(tuple(cres_c), 0.12, 0.25, 0.38, SEED + 97))
    # Charms and the bell from the long branch.
    p0, p2, d, rad = ends[0]
    r.add(charm_string(tuple(p0.lerp(p2, 0.55) - UP * (rad + 0.01)), 0.34, SEED + 98))
    bell_top = p0.lerp(p2, 0.85) - UP * (rad + 0.005)
    r.add(bell_hanger(tuple(bell_top), 0.1, SEED + 99))
    bell = bell_top - UP * 0.1
    p0b, p2b, db, radb = ends[1]
    r.add(charm_string(tuple(p0b.lerp(p2b, 0.7) - UP * (radb + 0.01)), 0.22, SEED + 101, items=('feather', 'knuckle')))
    for k, (p0, p2, d, rad) in enumerate(ends):
        side = Vector((-d.y, d.x, 0.0)).normalized()
        at = p0.lerp(p2, 0.6 if k == 0 else 0.75) + UP * rad * 1.1
        r.perch(at, side * (1.0 if k % 2 else -1.0) + flat(d) * 0.4)
    lead2_end = Vector(lead2[-1])
    r.perch(lead2_end - Vector((0.08, -0.02, 0.0)) + UP * 0.05, (0.5, -1.0, 0.0))
    rim_out = Vector((math.cos(math.radians(-20.0)), math.sin(math.radians(-20.0)), 0.0))
    r.perch(nest_c + rim_out * 0.52 + UP * 0.38, rim_out)
    speaker = (nest_c + Vector((-0.05, -0.52, 0.39)), (0.0, -1.0, 0.0))
    post = prism_hull((0.04, 0.03), 0.24, 0.0, 3.4) + [(0.04, 0.18, 4.6), (0.1, 0.12, 4.6)]
    foot = prism_hull((0.0, 0.0), 0.55, 0.0, 0.28)
    r.add(cairn(0.58, SEED + 102, count=6, avoid_r=0.26))
    obj = r.finish(cres_c + UP * 0.2, bell, (0.0, -1.7, 0.0), speaker, post, foot)
    return obj, r


def roost_bell():
    """The iron bell, hanging from its loop's top (the pivot), its clapper under the lip."""
    m = lr.Model('HobRoostBell', seed=SEED + 300)
    body, _ = lr.lathe([(0.0, -0.034), (0.034, -0.036), (0.054, -0.06), (0.06, -0.11), (0.07, -0.165), (0.09, -0.208),
                        (0.1, -0.228), (0.097, -0.236), (0.082, -0.234), (0.0, -0.22)], 16, 'ironblack', set_name='MetalWorn',
                       seed=SEED + 301)
    m.add(body, smooth=True)
    # A raised band round its shoulder and one above its lip.
    for z, rr in ((-0.075, 0.058), (-0.19, 0.088)):
        ring = [(math.cos(a) * rr, math.sin(a) * rr, z) for a in (2.0 * math.pi * k / 16 for k in range(17))]
        m.add(lr.tube(ring, 0.004, sides=4, key='ironblack', set_name='MetalWorn', caps=(False, False), seed=SEED + 302), smooth=True)
    loop = [(0.016 * math.sin(a), 0.0, -0.018 + 0.016 * math.cos(a)) for a in (2.0 * math.pi * k / 12 for k in range(13))]
    m.add(lr.tube(loop, 0.0055, sides=5, key='ironblack', set_name='MetalWorn', caps=(False, False), seed=SEED + 303), smooth=True)
    m.add(lr.tube([(0.0, 0.0, -0.21), (0.0, 0.0, -0.24)], 0.004, sides=4, key='ironblack', set_name='MetalWorn', seed=SEED + 304))
    clapper, _ = lr.lathe([(0.0, -0.236), (0.012, -0.241), (0.016, -0.252), (0.012, -0.262), (0.0, -0.266)], 8, 'ironblack',
                          set_name='MetalWorn', seed=SEED + 305)
    m.add(clapper, smooth=True)
    return m.finish(ao=True, ground=False, preview=False, fallback=None, ao_distance=0.1, Nanite=0, LODs='50,25',
                    Collision='None')


BUILDERS = {'A': direction_a, 'B': direction_b, 'C': direction_c}
TITLES = {'A': "The Keeper's Crook", 'B': "The Crow's Mast", 'C': 'The Crow Tree'}

# Blender runs the script as __main__ (models.ps1, artrun.ps1); a checking script may import its parts without building.
if __name__ == '__main__':
    if PREVIEW:
        BUILT = {key: BUILDERS[key]('HobRoost_' + key) for key in ('A', 'B', 'C')}
        BELL = roost_bell()
        import roost_sheet
        roost_sheet.render(BUILT, BELL, TITLES, DIRECTION)
    else:
        ROOST, _ = BUILDERS[DIRECTION]('HobRoost')
        BELL = roost_bell()
