"""The Gravemother's Barrow fight, its earth (Saved/Gravemother/CONTRACT.md, BARROW-ART, 2026-10-19; sizes from
Saved/Concepts/Gravemother/Barrow_FX.png): what she tears up, throws and ploughs. Fresh grave earth (GraveEarth: the
GravemotherEarth set, #6E5C49, darker than the Sink's sun-baked floor so it reads on the floor and the wall), with field
stones (GraveFieldstone), old coffin wood (GraveCoffinWood), bone (BoneDry, the den's) and roots (GraveRoots) in it.

  GraveClod_S, _M, _L     clods of grave earth, field stones, a coffin plank and a bone stuck in them (S 67, M 90,
                          L 121 cm across): L is the clod she lobs, S the loose ones. Pivot at the middle; one convex hull
                          (what a shot in her grip hits; the game sets its channels).
  GraveRubble             what a clod leaves: a heaped pile about 1.1 m high and 1.8 m across, stones, a plank, a bone,
                          loose clods round it; convex hulls round its mass (crouching cover, 10 s). Pivot at its base
                          middle, on the ground.
  GraveBerm               one segment of the earth she throws up beside the furrow: 2 m between segments (it reaches
                          2.6 m: neighbours overlap), about 1.7 m wide and 0.9 m high, X (Unreal) along its length, its
                          steeper face to Unreal -Y (turn it toward the furrow); three convex hulls (cover, 20 s).
  GraveMound              a spawn mound at its full bulge (170 x 160 x 47 cm; the game scales it up over its 1 s), its
                          crust lifting in plates; a convex hull (a mound shot while it bulges); MI_Decal_MoundCracks goes
                          over it.
  GraveMoundBurst         the mound dug out: a ring of broken clods and crust round a dark hole (190 x 184 x 29 cm);
                          no collision.
  GraveDebris_1..5        chunks a burst or the plough throws: a clump of earth, a field stone, a coffin splinter, a bone,
                          a root end; pivot at the middle, no collision.

None has Nanite (they're spawned, moved, scaled and sunk away): LODs at 50% and 25%. The pieces that lie on the ground
have baked vertex occlusion.

    Tools\\artrun.ps1 -Script Art\\Models\\Props\\GraveFight.py -Preview -Threads 6
        (Saved/ArtPreviews/Creatures/Gravemother/GraveFight_*.png)
"""
import math
import os
import random

import bmesh
import bpy
from mathutils import Euler, Matrix, Vector, noise

import looter_barrow as lb
import looter_textures as lt

lb.ensure_set(lb.EARTH_SET, lb.paint_earth, always=True)
E, ST, WOOD, BONE, ROOT, HOLE = range(6)
MATS = [lb.earth(), lb.fieldstone(), lb.coffin_wood(), lb.bone(), lb.roots(),
        lt.material(lb.EARTH_SET, name='GraveHole', tint=0x3a332c, DiffuseAO=1.0)]
# Each material's UV repeat per metre (its set's px per metre over its size).
UV = {E: 0.5, ST: 320.0 / 1024.0, WOOD: 320.0 / 1024.0, BONE: 1.0, ROOT: 1.0 / lb.ROOT_REPEAT_M, HOLE: 0.5}
UP = Vector((0.0, 0.0, 1.0))


def rand_dir(rng, up_bias=0.0):
    v = Vector((rng.uniform(-1, 1), rng.uniform(-1, 1), rng.uniform(-1, 1) + up_bias))
    return v.normalized() if v.length > 1e-6 else UP


def rot(rng):
    return Euler((rng.uniform(0, 6.3), rng.uniform(0, 6.3), rng.uniform(0, 6.3))).to_matrix()


def stone(bm, at, r, rng, flat=0.7):
    """A field stone: a faceted, rounded lump of granite."""
    verts = lb.blob(bm, at, (r * 1.2, r, r * flat), rot(rng), seed=rng.randint(0, 999), lump=0.18, subdiv=1, mat=ST)
    for f in {f for v in verts for f in v.link_faces}:
        f.smooth = False


def plank(bm, at, length, width, thick, R, rng, splinter=True):
    """A piece of an old coffin board, its far end broken into splinters."""
    n = 6
    rows = []
    for k in range(n + 1):
        t = k / n
        x = (t - 0.5) * length
        ring = []
        for y, z in ((-0.5, -0.5), (0.5, -0.5), (0.5, 0.5), (-0.5, 0.5)):
            yy, zz = y * width, z * thick
            if splinter and t > 0.7:
                # The broken end: its corners pulled into splinters of uneven length.
                f = (t - 0.7) / 0.3
                yy *= 1.0 - f * rng.uniform(0.3, 0.9)
                zz *= 1.0 - f * 0.5
            ring.append(bm.verts.new(Vector(at) + R @ Vector((x, yy, zz))))
        rows.append(ring)
    for k in range(n):
        for j in range(4):
            j1 = (j + 1) % 4
            bm.faces.new((rows[k][j], rows[k + 1][j], rows[k + 1][j1], rows[k][j1])).material_index = WOOD
    bm.faces.new(list(reversed(rows[0]))).material_index = WOOD
    tip = bm.verts.new(Vector(at) + R @ Vector((length * 0.5 + rng.uniform(0.02, 0.08) * length, 0.0, 0.0)))
    for j in range(4):
        bm.faces.new((rows[n][j], rows[n][(j + 1) % 4], tip)).material_index = WOOD


def bone_piece(bm, at, length, r, R, rng):
    """A bone: a shaft with a knob at one end, broken at the other."""
    pts = [Vector(at) + R @ Vector((x * length, 0.0, math.sin(x * 2.0) * r * 0.3)) for x in (-0.5, -0.25, 0.0, 0.25, 0.5)]
    radii = [r * 1.55, r * 1.0, r * 0.82, r * 0.9, r * 1.05]
    lb.tube(bm, pts, radii, mat=BONE, sides=9, uv_per=UV[BONE], wobble=0.08, seed=rng.randint(0, 99))
    lb.blob(bm, pts[0] + R @ Vector((-r * 0.4, r * 0.5, 0.0)), (r * 1.3, r * 1.1, r * 1.0), R, seed=rng.randint(0, 99),
            lump=0.12, subdiv=2, mat=BONE)
    lb.blob(bm, pts[0] + R @ Vector((-r * 0.4, -r * 0.6, 0.0)), (r * 1.1, r * 1.0, r * 0.9), R, seed=rng.randint(0, 99),
            lump=0.12, subdiv=2, mat=BONE)


def root_bit(bm, at, length, r, R, rng):
    pts = [Vector(at) + R @ Vector((x * length, math.sin(x * 4.0 + 1.0) * r * 1.2, math.cos(x * 3.0) * r * 0.8))
           for x in (-0.5, -0.3, -0.1, 0.1, 0.3, 0.5)]
    lb.tube(bm, pts, [r * 1.15, r * 1.05, r, r * 0.9, r * 0.75, r * 0.55], mat=ROOT, sides=7, uv_per=UV[ROOT],
            wobble=0.15, seed=rng.randint(0, 99))
    tip = pts[-1]
    d = (pts[-1] - pts[-2]).normalized()
    for k in range(3):
        lb.tube(bm, [tip - d * r * 0.5, tip + (d + rand_dir(rng) * 0.5).normalized() * r * rng.uniform(1.0, 2.2)],
                [r * 0.3, 0.002], mat=ROOT, sides=4, uv_per=UV[ROOT])


def earth_lump(bm, at, radii, rng, lump=0.3, subdiv=2, flat_below=None, R=None, freq=1.5):
    return lb.blob(bm, at, radii, R or rot(rng), seed=rng.randint(0, 999), lump=lump, subdiv=subdiv, freq=freq, mat=E,
                   flat_below=flat_below)


def finish(name, bm, nanite=False, lods='50,25', screens='0.4,0.15', collision=None, ao_ground=None, hulls=()):
    """The piece as a model: cube UVs per material at its set's scale, materials compacted, hulls under it, its
    occlusion baked (ao_ground: the ground's height under it, None for pieces that fly)."""
    bm.normal_update()
    for mat, per in UV.items():
        if mat == ROOT:                     # the roots keep their tubes' UVs (the bark's grain along them)
            continue
        lb.box_uv_faces(bm, [f for f in bm.faces if f.material_index == mat], per)
    bmesh.ops.triangulate(bm, faces=[f for f in bm.faces if len(f.verts) > 4])
    used = sorted({f.material_index for f in bm.faces})
    remap = {old: new for new, old in enumerate(used)}
    for f in bm.faces:
        f.material_index = remap[f.material_index]
    obj = lb.obj_from_bm(name, bm, [MATS[i] for i in used], smooth=None, sharp=None)
    obj['Nanite'] = 0
    if lods:
        obj['LODs'] = lods
        obj['LODScreens'] = screens
    if collision:
        obj['Collision'] = collision
    for i, points in enumerate(hulls):
        hb = bmesh.new()
        for p in points:
            hb.verts.new(p)
        made = bmesh.ops.convex_hull(hb, input=hb.verts)
        inside = [v for v in made['geom_interior'] + made['geom_unused'] if isinstance(v, bmesh.types.BMVert)]
        bmesh.ops.delete(hb, geom=list(set(inside)), context='VERTS')
        h = lb.obj_from_bm(f'UCX_{name}_{i:02d}', hb, [], smooth=False)
        h.parent = obj
        h.display_type = 'WIRE'
    if ao_ground is not None:
        lt.bake_vertex_ao(obj, samples=24, distance=0.6, ground=True, children=False, strength=0.85)
    return obj


def surface_verts(bm, mat=E):
    return [v.co.copy() for v in bm.verts if any(f.material_index == mat for f in v.link_faces)]


# --- Clods ---


def clod(name, R, seed):
    """A clod of grave earth torn up whole: a lumpy mass, field stones half in it, a coffin plank and a bone through it."""
    rng = random.Random(seed)
    bm = lb.new_bm()
    earth_lump(bm, (0.0, 0.0, 0.0), (R, R * 0.86, R * 0.8), rng, lump=0.24, subdiv=4 if R > 0.45 else 3, freq=0.95)
    for k in range(4):
        d = rand_dir(rng, 0.3)
        earth_lump(bm, d * R * 0.62, (R * 0.48, R * 0.42, R * 0.38), rng, lump=0.26, subdiv=3 if R > 0.45 else 2,
                   freq=1.1)
    for k in range(6):
        d = rand_dir(rng)
        stone(bm, d * R * 0.86, R * rng.uniform(0.13, 0.22), rng)
    ang = rng.uniform(0, 6.3)
    Rp = Matrix.Rotation(ang, 3, 'Z') @ Matrix.Rotation(0.5, 3, 'Y')
    plank(bm, Rp @ Vector((R * 0.55, 0.0, R * 0.15)), R * 1.65, R * 0.34, R * 0.085, Rp, rng)
    Rb = Matrix.Rotation(ang + 2.4, 3, 'Z') @ Matrix.Rotation(-0.35, 3, 'Y')
    bone_piece(bm, Rb @ Vector((R * 0.62, 0.0, 0.0)), R * 0.95, R * 0.07, Rb, rng)
    pts = [v for v in surface_verts(bm)]
    return finish(name, bm, hulls=[[p * 0.97 for p in pts]])


clods = [clod('GraveClod_S', 0.29, 1), clod('GraveClod_M', 0.4, 2), clod('GraveClod_L', 0.53, 3)]

# --- The rubble pile ---


def rubble(seed=7):
    rng = random.Random(seed)
    bm = lb.new_bm()
    earth_lump(bm, (0.0, 0.0, 0.22), (0.78, 0.68, 0.86), rng, lump=0.18, subdiv=4, flat_below=0.0, freq=1.0)
    lobes = []
    for k in range(7):
        a = 2.0 * math.pi * k / 7 + rng.uniform(-0.3, 0.3)
        d = rng.uniform(0.5, 0.78)
        rr = rng.uniform(0.26, 0.42)
        c = Vector((math.cos(a) * d, math.sin(a) * d, rr * 0.25))
        earth_lump(bm, c, (rr * 1.2, rr, rr * 0.8), rng, lump=0.26, subdiv=3, flat_below=0.0, freq=1.1)
        lobes.append(c)
    # Clods heaped on it.
    for k in range(10):
        a, h = rng.uniform(0, 6.3), rng.uniform(0.25, 0.85)
        r = 0.72 * math.sqrt(max(1.0 - (h / 1.05) ** 2, 0.05))
        earth_lump(bm, (math.cos(a) * r, math.sin(a) * r, h), (0.16, 0.13, 0.11), rng, lump=0.3, subdiv=2)
    for k in range(8):
        a, h = rng.uniform(0, 6.3), rng.uniform(0.1, 0.8)
        r = 0.74 * math.sqrt(max(1.0 - (h / 1.05) ** 2, 0.05))
        stone(bm, (math.cos(a) * r, math.sin(a) * r, h), rng.uniform(0.07, 0.14), rng)
    Rp = Matrix.Rotation(0.9, 3, 'Z') @ Matrix.Rotation(-0.55, 3, 'Y')
    plank(bm, (0.32, -0.25, 0.62), 0.95, 0.19, 0.035, Rp, rng)
    Rb = Matrix.Rotation(2.6, 3, 'Z') @ Matrix.Rotation(0.3, 3, 'Y')
    bone_piece(bm, (-0.42, 0.38, 0.55), 0.36, 0.022, Rb, rng)
    root_bit(bm, (-0.2, -0.55, 0.45), 0.42, 0.022, Matrix.Rotation(-0.6, 3, 'Z') @ Matrix.Rotation(0.4, 3, 'X'), rng)
    # Loose clods round its foot.
    for k in range(14):
        a = rng.uniform(0, 6.3)
        d = rng.uniform(0.95, 1.3)
        r = rng.uniform(0.045, 0.1)
        earth_lump(bm, (math.cos(a) * d, math.sin(a) * d, r * 0.4), (r * 1.2, r, r * 0.75), rng, lump=0.3, subdiv=1)
    core = [p for p in surface_verts(bm) if math.hypot(p.x, p.y) < 0.92 and p.z > -0.02]
    left = [p for p in core if p.x < 0.15]
    right = [p for p in core if p.x > -0.15]
    base = [Vector((x, y, 0.0)) for x, y in ((0.0, 0.0),)]
    return finish('GraveRubble', bm, ao_ground=0.0, hulls=[left + base, right + base])


# --- A berm segment ---


def berm(seed=3, length=2.6, width=1.7, height=0.9):
    """Earth thrown up beside the furrow: a long ragged ridge, steeper on its furrow side (Blender +X: Unreal -Y), its
    ends rounding down so the next segment's end, 2 m on, overlaps it; clods and stones lying on it."""
    rng = random.Random(seed)
    bm = lb.new_bm()
    n, m = 30, 21
    rows = []
    for i in range(n):
        s = i / (n - 1)
        y = (s - 0.5) * length
        ends = lb.lc.smoothstep(0.0, 0.1, s) * lb.lc.smoothstep(1.0, 0.9, s)
        ends = 0.45 + 0.55 * ends
        row = []
        for j in range(m):
            u = -1.0 + 2.0 * j / (m - 1)
            steep = 0.6 if u > 0 else 1.35
            prof = max(0.0, 1.0 - abs(u) ** (2.0 if u > 0 else 1.6)) ** steep
            nz = noise.noise(Vector((y * 1.4 + seed, u * 1.6, 0.5)))
            fine = noise.noise(Vector((y * 5.0, u * 4.0, seed + 0.3)))
            z = height * ends * prof * (1.0 + 0.2 * nz + 0.08 * fine)
            # Thrown earth lies in rough clumps: a ripple along it.
            z += 0.035 * noise.noise(Vector((y * 9.0, u * 6.0, 2.0 + seed))) * prof
            x = u * width * 0.5 * (1.0 + 0.1 * nz)
            row.append(bm.verts.new((x, y, z - 0.03)))
        rows.append(row)
    ridge = [v.co.copy() for row in rows for v in row]          # the ridge itself, for its hulls
    for i in range(n - 1):
        for j in range(m - 1):
            face = bm.faces.new((rows[i][j], rows[i][j + 1], rows[i + 1][j + 1], rows[i + 1][j]))
            face.material_index, face.smooth = E, True
    bm.normal_update()
    for f in bm.faces:
        if f.normal.z < 0:
            f.normal_flip()
    # Clods and stones lying on it, more slid to its foot on the far side.
    for k in range(26):
        s, u = rng.uniform(0.08, 0.92), rng.uniform(-0.85, 0.85)
        y = (s - 0.5) * length
        prof = max(0.0, 1.0 - abs(u) ** (2.0 if u > 0 else 1.6)) ** (0.6 if u > 0 else 1.35)
        r = rng.uniform(0.05, 0.13)
        z = height * prof * 0.95 + r * 0.2
        earth_lump(bm, (u * width * 0.5, y, z), (r * 1.25, r, r * 0.75), rng, lump=0.28, subdiv=2)
    for k in range(6):
        s, u = rng.uniform(0.1, 0.9), rng.uniform(-0.7, 0.7)
        prof = max(0.0, 1.0 - abs(u) ** 1.8)
        stone(bm, (u * width * 0.5, (s - 0.5) * length, height * prof * 0.9), rng.uniform(0.05, 0.1), rng)
    root_bit(bm, (0.25, 0.3, height * 0.75), 0.4, 0.018, Matrix.Rotation(1.2, 3, 'Z') @ Matrix.Rotation(0.3, 3, 'Y'), rng)
    hulls = []
    for k in range(3):
        lo, hi = -length / 2 + k * length / 3 - 0.05, -length / 2 + (k + 1) * length / 3 + 0.05
        part = [p for p in ridge if lo <= p.y <= hi]
        hulls.append([Vector((p.x, p.y, max(p.z, 0.0))) for p in part])
    return finish('GraveBerm', bm, ao_ground=0.0, hulls=hulls)


# --- Spawn mounds ---


def mound(seed=3):
    """A spawn mound at its full bulge: a dome of grave earth, its crust breaking into plates lifting from its top,
    crumbs and pebbles shed round its skirt."""
    rng = random.Random(seed)
    bm = lb.new_bm()
    r, h = 0.82, 0.47
    earth_lump(bm, (0.0, 0.0, -0.02), (r, r * 0.94, h), rng, lump=0.16, subdiv=4, flat_below=0.0, R=Matrix.Identity(3))
    # Crust plates lifting: thin flat lumps tilted out from the top.
    for k in range(11):
        a, d = rng.uniform(0, 6.3), r * rng.uniform(0.12, 0.7)
        z = h * math.sqrt(max(1.0 - (d / r) ** 2, 0.0)) - 0.015
        n = Vector((math.cos(a) * d / r * 0.8, math.sin(a) * d / r * 0.8, 1.0)).normalized()
        a2 = n.orthogonal().normalized()
        R = Matrix((a2, n.cross(a2), n)).transposed() @ Matrix.Rotation(rng.uniform(0.1, 0.35), 3, 'X')
        pr = rng.uniform(0.09, 0.17)
        verts = lb.blob(bm, (math.cos(a) * d, math.sin(a) * d, z), (pr * 1.3, pr, 0.028), R, seed=rng.randint(0, 999),
                        lump=0.25, subdiv=1, mat=E)
        for f in {f for v in verts for f in v.link_faces}:
            f.smooth = False
    for k in range(14):
        a = rng.uniform(0, 6.3)
        d = r * rng.uniform(0.9, 1.15)
        s = rng.uniform(0.04, 0.08)
        earth_lump(bm, (math.cos(a) * d, math.sin(a) * d, s * 0.3), (s * 1.2, s, s * 0.7), rng, lump=0.3, subdiv=1)
    for k in range(5):
        a = rng.uniform(0, 6.3)
        d = r * rng.uniform(0.5, 0.95)
        stone(bm, (math.cos(a) * d, math.sin(a) * d, h * math.sqrt(max(1 - (d / r) ** 2, 0)) * 0.9), 0.035, rng)
    dome = [p for p in surface_verts(bm) if math.hypot(p.x, p.y) < r * 1.02]
    return finish('GraveMound', bm, ao_ground=0.0, hulls=[[Vector((p.x, p.y, max(p.z, 0.0))) for p in dome]])


def mound_burst(seed=4):
    """The mound dug out: a low rim of thrown-up earth, its crust's broken plates tipped on it, a dark hole in the
    middle (a shallow dish: the floor under it is solid), clumps thrown round."""
    rng = random.Random(seed)
    bm = lb.new_bm()
    for k in range(15):
        a = 2.0 * math.pi * k / 15 + rng.uniform(-0.15, 0.15)
        d = rng.uniform(0.52, 0.66)
        rr = rng.uniform(0.2, 0.3)
        R = Matrix.Rotation(a, 3, 'Z')
        earth_lump(bm, (math.cos(a) * d, math.sin(a) * d, -0.05), (rr * 0.75, rr * 1.35, 0.3), rng, lump=0.3,
                   subdiv=2, flat_below=0.0, R=R)
    for k in range(10):
        a, d = rng.uniform(0, 6.3), rng.uniform(0.42, 0.8)
        z = rng.uniform(0.1, 0.22)
        n = Vector((math.cos(a), math.sin(a), rng.uniform(0.6, 1.6))).normalized()
        a2 = n.orthogonal().normalized()
        R = Matrix((a2, n.cross(a2), n)).transposed()
        pr = rng.uniform(0.09, 0.16)
        verts = lb.blob(bm, (math.cos(a) * d, math.sin(a) * d, z), (pr * 1.3, pr, 0.03), R, seed=rng.randint(0, 999),
                        lump=0.25, subdiv=1, mat=E)
        for f in {f for v in verts for f in v.link_faces}:
            f.smooth = False
    # The hole: a dark dish inside the rim, sloping down to its middle.
    rings, sides = 6, 24
    centre = bm.verts.new((0.0, 0.0, 0.012))
    grid = []
    for i in range(1, rings + 1):
        t = i / rings
        row = []
        for j in range(sides):
            a = 2.0 * math.pi * j / sides
            rr = 0.5 * t * (1.0 + 0.12 * noise.noise(Vector((math.cos(a) * 2, math.sin(a) * 2, 1.5))))
            row.append(bm.verts.new((math.cos(a) * rr, math.sin(a) * rr, 0.012 + 0.09 * t ** 2.2)))
        grid.append(row)
    for j in range(sides):
        face = bm.faces.new((centre, grid[0][j], grid[0][(j + 1) % sides]))
        face.material_index, face.smooth = HOLE, True
    for i in range(rings - 1):
        for j in range(sides):
            j1 = (j + 1) % sides
            face = bm.faces.new((grid[i][j], grid[i + 1][j], grid[i + 1][j1], grid[i][j1]))
            face.material_index, face.smooth = (HOLE if i < 3 else E), True
    for k in range(16):
        a = rng.uniform(0, 6.3)
        d = rng.uniform(0.75, 1.0)
        s = rng.uniform(0.035, 0.08)
        earth_lump(bm, (math.cos(a) * d, math.sin(a) * d, s * 0.3), (s * 1.2, s, s * 0.7), rng, lump=0.3, subdiv=1)
    return finish('GraveMoundBurst', bm, ao_ground=0.0, collision='None')


# --- Debris ---


def debris():
    out = []
    rng = random.Random(21)
    bm = lb.new_bm()
    earth_lump(bm, (0.0, 0.0, 0.0), (0.125, 0.105, 0.085), rng, lump=0.32, subdiv=2)
    stone(bm, (0.06, 0.03, 0.05), 0.025, rng)
    out.append(finish('GraveDebris_1', bm, lods='50', screens='0.1', collision='None'))
    bm = lb.new_bm()
    verts = lb.blob(bm, (0.0, 0.0, 0.0), (0.085, 0.07, 0.068), rot(rng), seed=4, lump=0.2, subdiv=2, mat=ST)
    for f in {f for v in verts for f in v.link_faces}:
        f.smooth = False
    out.append(finish('GraveDebris_2', bm, lods='50', screens='0.1', collision='None'))
    bm = lb.new_bm()
    plank(bm, (0.0, 0.0, 0.0), 0.36, 0.11, 0.03, Matrix.Rotation(0.15, 3, 'Z'), rng)
    out.append(finish('GraveDebris_3', bm, lods='50', screens='0.1', collision='None'))
    bm = lb.new_bm()
    bone_piece(bm, (0.0, 0.0, 0.0), 0.24, 0.022, Matrix.Rotation(0.2, 3, 'Z'), rng)
    out.append(finish('GraveDebris_4', bm, lods='50', screens='0.1', collision='None'))
    bm = lb.new_bm()
    root_bit(bm, (0.0, 0.0, 0.0), 0.26, 0.03, Matrix.Rotation(0.3, 3, 'Z'), rng)
    out.append(finish('GraveDebris_5', bm, lods='50', screens='0.1', collision='None'))
    return out


pieces = clods + [rubble(), berm(), mound(), mound_burst()] + debris()
for obj in pieces:
    bpy.context.view_layer.update()
    dims = obj.dimensions * 100.0
    tris = sum(len(p.vertices) - 2 for p in obj.data.polygons)
    lb.log(f'{obj.name}: {dims.x:.0f} x {dims.y:.0f} x {dims.z:.0f} cm, {tris} triangles, '
           f'{len([c for c in obj.children if c.name.startswith("UCX_")])} hulls, '
           f'materials {", ".join(m.name for m in obj.data.materials)}')

if lt.want_preview():
    out = lb.PREVIEW_DIR
    by = {o.name: o for o in pieces}
    shots = []
    # Clods and debris, floating at their middles over the floor.
    x = -1.3
    for name in ('GraveClod_S', 'GraveClod_M', 'GraveClod_L'):
        o = by[name]
        o.location = (x, 0.0, o.dimensions.z * 0.5 + 0.02)
        x += o.dimensions.x * 0.5 + 0.55
    for i in range(1, 6):
        by[f'GraveDebris_{i}'].location = (-1.0 + i * 0.4, -0.9, 0.12)
    group = [by[n] for n in ('GraveClod_S', 'GraveClod_M', 'GraveClod_L')] + [by[f'GraveDebris_{i}'] for i in range(1, 6)]
    shots.append(lb.render(group, os.path.join(out, 'GraveFight_clods.png'), (0.3, -3.6, 1.6), (0.35, -0.2, 0.3),
                           lens=40.0, size=(1500, 800), sun_dir=(-0.5, -0.8, 0.7)))
    # The rubble and a row of berms (laid every 2 m, turned a little, as the game lays them).
    by['GraveRubble'].location = (-1.6, 0.6, 0.0)
    shots.append(lb.render([by['GraveRubble']], os.path.join(out, 'GraveFight_rubble.png'), (-0.4, -2.6, 1.3),
                           (-1.6, 0.6, 0.45), lens=40.0, size=(1500, 800), sun_dir=(-0.5, -0.8, 0.7)))
    berm_obj = by['GraveBerm']
    row = [berm_obj]
    for k in range(1, 4):
        c = berm_obj.copy()
        bpy.context.scene.collection.objects.link(c)
        c.location = (0.0, k * 2.0, 0.0)
        c.rotation_euler = (0.0, 0.0, math.radians((-4, 3, -2)[k - 1]))
        c.scale = (1.0, 1.0, (1.06, 0.92, 1.1)[k - 1])
        row.append(c)
    shots.append(lb.render(row, os.path.join(out, 'GraveFight_berms.png'), (4.2, -2.4, 2.2), (0.0, 3.0, 0.3),
                           lens=35.0, size=(1500, 800), sun_dir=(-0.5, -0.8, 0.7)))
    for c in row[1:]:
        bpy.data.objects.remove(c)
    by['GraveMound'].location = (-1.0, 0.0, 0.0)
    by['GraveMoundBurst'].location = (1.1, 0.0, 0.0)
    shots.append(lb.render([by['GraveMound'], by['GraveMoundBurst']], os.path.join(out, 'GraveFight_mounds.png'),
                           (0.0, -3.3, 1.9), (0.0, 0.0, 0.15), lens=40.0, size=(1500, 800), sun_dir=(-0.5, -0.8, 0.7)))
    lb.sheet(shots, os.path.join(out, 'GraveFight_sheet.png'), cols=2)
    for o in pieces:
        o.location = (0.0, 0.0, 0.0)
        o.rotation_euler = (0.0, 0.0, 0.0)
