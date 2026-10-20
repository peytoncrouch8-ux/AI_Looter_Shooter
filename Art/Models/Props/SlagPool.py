"""Slag Bomb's molten pool (Gauge's power; Source/AI_Looter_Shooter/Embers/Powers/EmberSlagBomb*.cpp): the slag it leaves
burning for a few seconds. Dark cooling crust plates (iron-black, a little rough: the SlagPool texture set) float on
bright molten cracks, with a white-hot lake in the middle; it reads as molten iron on Medium with no light at all (the
glow is the surface's own emissive: the flat stylized material's Glow, as the lantern glass does it).

  SlagPool        the pool: about 1 m across at scale 1 (mean radius 0.5 m: scale XY by the power's Radius / 50),
                  pivot on the ground in its middle. The melt stands 6 mm above the pivot (7.5 in the middle), the
                  crust plates 3-7 mm above the melt, and a soft rounded lip of crust runs down to 8 mm below the
                  pivot, so a little unevenness under it is swallowed. Spatter droplets lie just outside it.
  SlagPoolCrust   the crust that forms as it cools, on the same pivot: bigger plates over the same cells (and over the
                  white-hot lake). It sits fully under the melt when lowered 0.9 cm (in the model's units: the script
                  logs it) and closes the cracks to hairlines when raised to 0: raising it over the pool's life grows the
                  crust out from the plates' edges into the cracks, and the lake crusts over last. Crust only.
  SlagSplatA, B   two small splashes (also 1 m across at scale 1), the same make: a few plates, more droplets; to break
                  the pool's round outline or mark where a lob fell.

Material slots (names are the Unreal slots, MI_<name>):
  SlagCrust      M_World, the SlagPool set (Art/Textures/SlagPool/make_slag_pool.py), mapped once per 0.2 m of model
  SlagMolten     flat stylized Surface, #FF6A14, Glow 6: the cracks and bays
  SlagWhiteHot   flat stylized Surface, #FFC45C, Glow 9: the lake in the middle (and the cracks round it)
  SlagGlowEdge   flat stylized Surface, #E2440C, Glow 3.5: the hot sides of the inner plates where they leave the melt
  The game can fade the glow by the slots' Glow scalar (and dim Color toward a dark red) as the pool cools:
  Saved/Polish/SLAGPOOL.md.
No Nanite (Medium and High draw the same mesh), no collision, no LODs: SlagPool 3.9k triangles, the crust 2.2k, the
splats 2.0-2.1k. Every run builds the same meshes (seeded; no sets iterated).

    Tools\\artrun.ps1 -Script Art\\Models\\Props\\SlagPool.py -Preview
        renders Saved/ArtPreviews/Props/SlagPool/ (top-down and a low three-quarter, hot and cooled, and the splats, at
        the game's size: 9 m across on uneven dusty ground)
"""
import math
import os
import random

import bmesh
import bpy
from mathutils import Vector

import looter_model as lm
import looter_textures as lt

SET = 'SlagPool'
CRUST, MOLTEN, WHITEHOT, EDGE = range(4)
MATS = [lt.material(SET, name='SlagCrust', master='World'),
        lm.material('SlagMolten', 0xff6a14, Glow=6.0, Variation=0.03),
        lm.material('SlagWhiteHot', 0xffc45c, Glow=9.0, Variation=0.02),
        lm.material('SlagGlowEdge', 0xe2440c, Glow=3.5, Variation=0.03)]
UV_PER_M = 5.0          # the crust texture once per 0.2 m of model (1.8 m in game at 9x)
Z_MELT = 0.006          # the melt's level above the pivot (m, scale 1) ...
DOME = 0.0015           # ... rising this much more toward the middle
Z_EDGE = -0.008         # the lip's foot, below the pivot
REGION = 0.42           # the plates' reach in normalized space (the outline at 0.5): the lip starts there


def cross2(a, b):
    return a.x * b.y - a.y * b.x


def area(poly):
    return 0.5 * sum(cross2(poly[i], poly[(i + 1) % len(poly)]) for i in range(len(poly)))


def smooth(e0, e1, x):
    t = min(max((x - e0) / (e1 - e0), 0.0), 1.0)
    return t * t * (3.0 - 2.0 * t)


class Outline:
    """The pool's outline: R(theta), mean radius 0.5 m, wavy (terms of k lobes)."""

    def __init__(self, rng, terms):
        self.terms = [(k, a, rng.uniform(0.0, 2.0 * math.pi)) for k, a in terms]

    def radius(self, theta):
        return 0.5 * (1.0 + sum(a * math.sin(k * theta + ph) for k, a, ph in self.terms))

    def to_real(self, q):
        """A point of the normalized disc (outline at 0.5) on the real pool."""
        if q.length < 1e-9:
            return q.copy()
        return q * (self.radius(math.atan2(q.y, q.x)) / 0.5)

    def s(self, p):
        """How far out a real point lies: 0 in the middle, 1 on the outline."""
        if p.length < 1e-9:
            return 0.0
        return p.length / self.radius(math.atan2(p.y, p.x))

    def melt_z(self, p):
        return Z_MELT + DOME * (1.0 - min(1.0, self.s(p) / 0.84) ** 2)


def clip(poly, labels, n, d, lab):
    """Sutherland-Hodgman against n.p <= d, keeping each edge's label (the clip line's edges get lab)."""
    out, outl = [], []
    m = len(poly)
    for i in range(m):
        a, b = poly[i], poly[(i + 1) % m]
        da, db = n.dot(a) - d, n.dot(b) - d
        if da <= 0.0:
            out.append(a)
            outl.append(labels[i])
            if db > 0.0:
                out.append(a + (b - a) * (da / (da - db)))
                outl.append(lab)
        elif db <= 0.0:
            out.append(a + (b - a) * (da / (da - db)))
            outl.append(labels[i])
    return out, outl


def cells_of(seeds, region):
    """Voronoi cells of the seeds inside a circle of radius region: (polygon CCW, label per edge: the neighbour's
    index, or -1 on the circle)."""
    k = 72
    circle = [Vector((region * math.cos(2.0 * math.pi * i / k), region * math.sin(2.0 * math.pi * i / k))) for i in range(k)]
    cells = []
    for i, s in enumerate(seeds):
        poly = [Vector((-1.0, -1.0)), Vector((1.0, -1.0)), Vector((1.0, 1.0)), Vector((-1.0, 1.0))]
        labels = [-2] * 4
        for j, t in enumerate(seeds):
            if j != i and poly:
                n = t - s
                poly, labels = clip(poly, labels, n, n.dot((s + t) * 0.5), j)
        for c in range(k):
            a, b = circle[c], circle[(c + 1) % k]
            e = b - a
            n = Vector((e.y, -e.x))
            if poly:
                poly, labels = clip(poly, labels, n, n.dot(a), -1)
        cells.append((poly, labels))
    return cells


def merge_short(poly, labels, eps):
    """Drops edges shorter than eps (their two corners meet in the middle)."""
    poly, labels = list(poly), list(labels)
    changed = True
    while changed and len(poly) > 3:
        changed = False
        for i in range(len(poly)):
            j = (i + 1) % len(poly)
            if (poly[j] - poly[i]).length < eps:
                mid = (poly[i] + poly[j]) * 0.5
                if j == 0:                  # the closing edge: the corner kept is the first
                    poly[0] = mid
                    del poly[-1]
                    del labels[-1]
                else:
                    poly[i] = mid
                    del poly[j]
                    del labels[i]
                changed = True
                break
    return poly, labels


def inset(poly, ws):
    """The polygon (CCW) with each edge moved inward by its own distance."""
    n = len(poly)
    lines = []
    for i in range(n):
        d = (poly[(i + 1) % n] - poly[i]).normalized()
        lines.append((poly[i] + Vector((-d.y, d.x)) * ws[i], d))
    out = []
    for i in range(n):
        p1, d1 = lines[i - 1]
        p2, d2 = lines[i]
        den = cross2(d1, d2)
        if abs(den) < 0.17:
            n1, n2 = Vector((-d1.y, d1.x)), Vector((-d2.y, d2.x))
            m = (n1 + n2).normalized()
            out.append(poly[i] + m * ((ws[i - 1] + ws[i]) * 0.5 / max(m.dot(n2), 0.5)))
        else:
            out.append(p1 + d1 * (cross2(p2 - p1, d2) / den))
    return out


def sound(poly, ref):
    """True when an inset kept every edge's direction and some area."""
    n = len(poly)
    for i in range(n):
        if (poly[(i + 1) % n] - poly[i]).dot(ref[(i + 1) % n] - ref[i]) <= 0.0:
            return False
    return area(poly) > 1e-5


class Wobble:
    """A cell's broken edge: a smooth field over the pool (two waves, 6 and 2.5 cm, in their own directions), so the hot
    plate and its cooled crust, built over the same cell, wander alike."""

    def __init__(self, seed):
        r = random.Random(seed)
        self.waves = []
        for length, share in ((0.06, 0.65), (0.025, 0.35)):
            a = r.uniform(0.0, 2.0 * math.pi)
            self.waves.append((Vector((math.cos(a), math.sin(a))) * (2.0 * math.pi / length), r.uniform(0.0, 6.3), share))

    def at(self, p):
        return sum(share * math.sin(k.dot(p) + ph) for k, ph, share in self.waves)


def refine(poly, values, room, wobble, seg, chamfer, inward_amp):
    """Rounds sharp corners (a chamfer), splits long edges (at most seg) and breaks the line: each point moves along
    the ring's inward normal by the wobble, inward up to inward_amp, outward up to its edge's room (part of the crack,
    so neighbours never meet). values and room hold one entry per edge; each new edge keeps its source edge's value.
    Returns the ring and its edges' values."""
    n = len(poly)
    # vals[k] and edge_room[k] belong to the edge from pts[k] to pts[k + 1] (a chamfer's own edge takes the incoming
    # edge's value), pt_room[k] to the point itself (a corner: the tighter of its two edges)
    pts, vals, pt_room, edge_room = [], [], [], []
    for i in range(n):
        a, b, c = poly[i - 1], poly[i], poly[(i + 1) % n]
        din, dout = (b - a), (c - b)
        turn = math.degrees(math.acos(max(-1.0, min(1.0, din.normalized().dot(dout.normalized())))))
        cut = min(chamfer, din.length * 0.3, dout.length * 0.3)
        tight = min(room[i - 1], room[i])
        if turn > 70.0 and cut > 0.0015:
            pts += [b - din.normalized() * cut, b + dout.normalized() * cut]
            vals += [values[i - 1], values[i]]
            pt_room += [room[i - 1], room[i]]
            edge_room += [tight, room[i]]
        else:
            pts.append(b.copy())
            vals.append(values[i])
            pt_room.append(tight)
            edge_room.append(room[i])
    ring, ring_vals, ring_room = [], [], []
    m = len(pts)
    for k in range(m):
        a, b = pts[k], pts[(k + 1) % m]
        d = b - a
        parts = max(1, int(math.ceil(d.length / seg)))
        for p in range(parts):
            ring.append(a + d * (p / parts))
            ring_vals.append(vals[k])
            ring_room.append(pt_room[k] if p == 0 else edge_room[k])
    normals = ring_normals(ring)
    out = []
    for q, nrm, rm in zip(ring, normals, ring_room):
        f = wobble.at(q)
        out.append(q + nrm * (f * inward_amp if f > 0.0 else f * rm))
    return out, ring_vals


def ring_normals(ring):
    """Inward normals at a CCW ring's points."""
    n = len(ring)
    out = []
    for i in range(n):
        d = (ring[(i + 1) % n] - ring[i - 1])
        out.append(Vector((-d.y, d.x)).normalized())
    return out


class Mesh:
    def __init__(self):
        self.bm = bmesh.new()
        self.uv = self.bm.loops.layers.uv.new('UVMap')

    def v(self, p):
        return self.bm.verts.new(p)

    def face(self, verts, mat, uvs):
        f = self.bm.faces.new(verts)
        f.material_index = mat
        f.smooth = True
        for loop, uv in zip(f.loops, uvs):
            loop[self.uv].uv = uv
        return f

    def finish(self, name):
        used = sorted({f.material_index for f in self.bm.faces})
        remap = {old: new for new, old in enumerate(used)}
        for f in self.bm.faces:
            f.material_index = remap[f.material_index]
        me = bpy.data.meshes.new(name)
        self.bm.normal_update()
        self.bm.to_mesh(me)
        self.bm.free()
        for i in used:
            me.materials.append(MATS[i])
        obj = bpy.data.objects.new(name, me)
        bpy.context.scene.collection.objects.link(obj)
        obj['Nanite'] = 0
        obj['Collision'] = 'None'
        tris = sum(len(p.vertices) - 2 for p in me.polygons)
        lo = [min(v.co[a] for v in me.vertices) for a in range(3)]
        hi = [max(v.co[a] for v in me.vertices) for a in range(3)]
        print(f'LOOTER: {name}: {tris} triangles, materials {", ".join(MATS[i].name for i in used)}; bounds '
              f'{", ".join(f"{a * 100:.1f}..{b * 100:.1f}" for a, b in zip(lo, hi))} cm', flush=True)
        return obj


def plate(mesh, outline, ring, wall_mats, run_v, h, tilt, rng, dome, top_mat=CRUST, uv_off=(0.0, 0.0), rough=0.03):
    """A crust plate on the melt: ring is its outline where it leaves the melt (CCW); its sides rise inward by run_v (per
    point) to a slightly domed top h above the melt (tilted by tilt, a slope vector). When any side glows, the sides
    break at 55% of h: below, wall_mats (per edge: the hot glow or crust), above crust; else one crust side."""
    n = len(ring)
    c = sum(ring, Vector((0.0, 0.0))) / n
    base = outline.melt_z(c)
    inward = ring_normals(ring)
    glows = any(m != CRUST for m in wall_mats)
    M = [ring[i] + inward[i] * (run_v[i] * 0.5) for i in range(n)]
    T = [ring[i] + inward[i] * run_v[i] for i in range(n)]

    def height(p, share):
        # the crust thins toward the rim and sinks into the lip there (no step round the pool's edge)
        sink = 1.0 - 0.9 * smooth(0.66, 0.86, outline.s(p))
        return base + share * (h + tilt.dot(p - c)) * sink
    vB = [mesh.v((p.x, p.y, base - 0.0015)) for p in ring]
    vM = [mesh.v((p.x, p.y, height(p, 0.55))) for p in M] if glows else None
    vT = [mesh.v((p.x, p.y, height(p, 0.93 + rng.uniform(-rough, rough)))) for p in T]
    top = sum(T, Vector((0.0, 0.0))) / n
    vC = mesh.v((top.x, top.y, height(top, 1.0) + dome))
    # side UVs: along the ring (u) and up (v); the top laid on from above
    arc = [0.0]
    for i in range(n):
        arc.append(arc[-1] + (ring[(i + 1) % n] - ring[i]).length)
    k = UV_PER_M
    for i in range(n):
        j = (i + 1) % n
        u0, u1 = arc[i] * k, arc[i + 1] * k
        zb, zt = vB[i].co.z, vT[i].co.z
        zbj, ztj = vB[j].co.z, vT[j].co.z
        if glows:
            zm, zmj = vM[i].co.z, vM[j].co.z
            mesh.face([vB[i], vB[j], vM[j], vM[i]], wall_mats[i],
                      [(u0, zb * k), (u1, zbj * k), (u1, zmj * k + 0.004), (u0, zm * k + 0.004)])
            mesh.face([vM[i], vM[j], vT[j], vT[i]], CRUST,
                      [(u0, zm * k + 0.004), (u1, zmj * k + 0.004), (u1, ztj * k + 0.012), (u0, zt * k + 0.012)])
        else:
            mesh.face([vB[i], vB[j], vT[j], vT[i]], CRUST,
                      [(u0, zb * k), (u1, zbj * k), (u1, ztj * k + 0.012), (u0, zt * k + 0.012)])
        mesh.face([vT[i], vT[j], vC], top_mat,
                  [(T[i].x * k + uv_off[0], T[i].y * k + uv_off[1]), (T[j].x * k + uv_off[0], T[j].y * k + uv_off[1]),
                   (top.x * k + uv_off[0], top.y * k + uv_off[1])])


def base_surface(mesh, outline, segments, rng, core_s):
    """The melt and its lip: rings out from the middle, the white-hot faces inside core_s, the molten ones out to the
    lip, the lip crust, rounded down to Z_EDGE at the outline."""
    ph = rng.uniform(0.0, 2.0 * math.pi)
    ph2 = rng.uniform(0.0, 2.0 * math.pi)
    angles = [2.0 * math.pi * i / segments for i in range(segments)]
    lip_s = [0.83 - 0.025 * (0.5 + 0.5 * math.sin(5.0 * a + ph)) * (0.5 + 0.5 * math.sin(11.0 * a + ph2)) for a in angles]
    inner = [0.08, 0.17, 0.36, 0.6]
    lip_u = [0.35, 0.68, 0.88, 1.0]
    # each row: (s, the lip's share u (None on the melt), where the lip starts at that angle); the band out to a row
    # takes that row's material
    rows, mats = [], []
    for s in inner:
        rows.append([(s, None, ls) for ls in lip_s])
        mats.append(WHITEHOT if s <= core_s + 1e-6 else MOLTEN)
    rows.append([(ls, 0.0, ls) for ls in lip_s])
    mats.append(MOLTEN)
    for u in lip_u:
        rows.append([(ls + u * (1.0 - ls), u, ls) for ls in lip_s])
        mats.append(CRUST)
    k = UV_PER_M

    def point(a, s, u, ls):
        way = Vector((math.cos(a), math.sin(a))) * outline.radius(a)
        p = way * s
        if u is None:
            z = outline.melt_z(p)
        else:
            z0 = outline.melt_z(way * ls)          # the melt's level where the lip starts, rounding down to Z_EDGE
            z = z0 - (z0 - Z_EDGE) * (1.0 - math.sqrt(max(0.0, 1.0 - u * u)))
        return mesh.v((p.x, p.y, z))
    centre = mesh.v((0.0, 0.0, outline.melt_z(Vector((0.0, 0.0)))))
    verts = [[point(a, s, u, ls) for a, (s, u, ls) in zip(angles, row)] for row in rows]

    def uv(v):
        return (v.co.x * k, v.co.y * k)
    for i in range(segments):
        j = (i + 1) % segments
        a, b = verts[0][i], verts[0][j]
        mesh.face([centre, a, b], mats[0], [uv(centre), uv(a), uv(b)])
    for r in range(len(rows) - 1):
        mat = mats[r + 1]
        for i in range(segments):
            j = (i + 1) % segments
            q = [verts[r][i], verts[r + 1][i], verts[r + 1][j], verts[r][j]]
            mesh.face(q, mat, [uv(v) for v in q])


def droplets(mesh, outline, rng, count, size=(0.008, 0.02)):
    """Spatter: small domes of crust just outside the outline, their foot below the ground."""
    for _ in range(count):
        a = rng.uniform(0.0, 2.0 * math.pi)
        rd = rng.uniform(*size)
        dist = outline.radius(a) * rng.uniform(1.03, 1.16) + rd
        c = Vector((math.cos(a) * dist, math.sin(a) * dist))
        hd = rd * rng.uniform(0.35, 0.55)
        squash = rng.uniform(0.7, 1.0)
        turn = rng.uniform(0.0, math.pi)
        n = 8
        base, mid = [], []
        for i in range(n):
            t = 2.0 * math.pi * i / n
            e = Vector((math.cos(t) * rd, math.sin(t) * rd * squash))
            e = Vector((e.x * math.cos(turn) - e.y * math.sin(turn), e.x * math.sin(turn) + e.y * math.cos(turn)))
            wob = 1.0 + rng.uniform(-0.12, 0.12)
            base.append(mesh.v((c.x + e.x * wob, c.y + e.y * wob, -0.004)))
            mid.append(mesh.v((c.x + e.x * 0.72 * wob, c.y + e.y * 0.72 * wob, hd * 0.62)))
        top = mesh.v((c.x, c.y, hd))
        k = UV_PER_M
        for i in range(n):
            j = (i + 1) % n
            q = [base[i], base[j], mid[j], mid[i]]
            mesh.face(q, CRUST, [(v.co.x * k, v.co.y * k + v.co.z * k) for v in q])
            q = [mid[i], mid[j], top]
            mesh.face(q, CRUST, [(v.co.x * k, v.co.y * k) for v in q])


def build(name, seed, terms, dmin, core_seeds, core_r, bays, max_bays, n_drops, segments, flecks=0, crust_name=None,
          seg=0.04):
    """A pool (and its cooling crust as a second model when crust_name is given)."""
    rng = random.Random(seed)
    outline = Outline(rng, terms)
    # seeds of the plates' cells (normalized space): the open core's first, then darts thrown with a spacing that grows
    # outward (small broken plates by the hot middle, broad ones by the rim)
    seeds, open_cells = [], set()
    for i in range(core_seeds):
        a = 2.0 * math.pi * (i + rng.uniform(0.0, 0.6)) / max(core_seeds, 1)
        seeds.append(Vector((math.cos(a), math.sin(a))) * rng.uniform(0.0, core_r * 0.5))
        open_cells.add(i)
    lo = core_r * 1.35 if core_seeds else 0.0
    for _ in range(6000):
        a = rng.uniform(0.0, 2.0 * math.pi)
        r = math.sqrt(rng.uniform((lo / REGION) ** 2, 1.0)) * REGION * 0.97
        q = Vector((math.cos(a) * r, math.sin(a) * r))
        if all((q - s).length > 0.5 * (dmin(r) + dmin(s.length)) for s in seeds[core_seeds:]) and \
                all((q - s).length > dmin(r) * 0.8 for s in seeds[:core_seeds]):
            seeds.append(q)
    cells = cells_of(seeds, REGION)
    # a few bays of melt reaching the rim crust: only smaller rim cells (a broad one read as a hole of light)
    rim = [i for i, (poly, labels) in enumerate(cells) if i not in open_cells and -1 in labels and len(poly) > 2]
    rim_areas = sorted(area(cells[i][0]) for i in rim)
    small = rim_areas[len(rim_areas) // 2] if rim_areas else 0.0
    for i in rim:
        if len(open_cells) - core_seeds < max_bays and area(cells[i][0]) <= small and rng.random() < bays:
            open_cells.add(i)
    mesh = Mesh()
    crust = Mesh() if crust_name else None
    edge_rng = {}

    def crack(i, j, mid_r):
        """The crack's half-width between cells i and j (the same from both sides)."""
        key = (min(i, j), max(i, j))
        if key not in edge_rng:
            edge_rng[key] = random.Random(seed * 7919 + key[0] * 613 + key[1]).random()
        base = 0.0085 + (0.0033 - 0.0085) * smooth(0.05, 0.40, mid_r)
        return base * (0.7 + 0.6 * edge_rng[key])
    plates = 0
    for i, (qpoly, qlabels) in enumerate(cells):
        if len(qpoly) < 3:
            continue
        r_seed = seeds[i].length
        real = [outline.to_real(q) for q in qpoly]
        real, labels = merge_short(real, qlabels, 0.002)
        if len(real) < 3:
            continue
        mid_r = [outline.s((real[e] + real[(e + 1) % len(real)]) * 0.5) * 0.5 for e in range(len(real))]
        hot = r_seed < 0.30 + rng.uniform(-0.04, 0.04)
        h = 0.0032 + (0.0062 - 0.0032) * smooth(0.05, REGION, r_seed) + rng.uniform(-0.0008, 0.0008)
        slope = rng.uniform(0.0, 0.05)
        ang = rng.uniform(0.0, 2.0 * math.pi)
        tilt = Vector((math.cos(ang), math.sin(ang))) * slope
        size = math.sqrt(max(area(real), 1e-6) / math.pi)
        tilt = tilt * min(1.0, 0.3 * h / max(slope * size, 1e-6)) if slope > 0.0 else tilt
        uv_off = (rng.uniform(0.0, 1.0), rng.uniform(0.0, 1.0))
        dome = rng.uniform(0.0008, 0.0018)
        ws, runs, mats = [], [], []
        for e, lab in enumerate(labels):
            if lab == -1:
                ws.append(0.0)
                runs.append(0.002)
                mats.append(CRUST)
            elif lab in open_cells:
                ws.append(0.0018)
                runs.append(0.0042)
                mats.append(EDGE)
            else:
                ws.append(crack(i, lab, mid_r[e]))
                runs.append(0.0045 if hot else 0.0022)
                mats.append(EDGE if hot else CRUST)
        wobble = Wobble(seed * 1009 + i)
        chamfer = min(0.007, 0.22 * size)
        inward_amp = min(0.0022, 0.12 * size)
        if i not in open_cells:
            ring = None
            for scale in (1.0, 0.6, 0.35):
                used = [w * scale for w in ws]
                cand = inset(real, used)
                if sound(cand, real):
                    ring = cand
                    break
            if ring is not None:
                limit = 0.25 * size
                ring, seg_vals = refine(ring, list(zip(mats, runs)), [0.45 * w for w in used], wobble, seg, chamfer,
                                        inward_amp)
                run_v = [min(limit, 0.5 * (seg_vals[k - 1][1] + seg_vals[k][1])) for k in range(len(ring))]
                plate(mesh, outline, ring, [m for m, _ in seg_vals], run_v, h, tilt, rng, dome, uv_off=uv_off)
                plates += 1
        if crust is not None:
            # the cooled crust: the cracks closed to hairlines, its sides wide enough to bury the hot plate's own
            late_w, late_run = [], []
            for e, lab in enumerate(labels):
                w = ws[e] if i not in open_cells else (0.0 if lab == -1 else crack(i, lab, mid_r[e]))
                lw = 0.0 if lab == -1 else max(0.0011, 0.28 * w)
                late_w.append(lw)
                late_run.append(0.0015 if lab == -1 else max(0.0015, 0.85 * (w - lw)))
            cand = inset(real, late_w)
            if sound(cand, real):
                ring, seg_vals = refine(cand, late_run, [0.45 * w for w in late_w], wobble, seg, chamfer, inward_amp)
                run_v = [min(0.3 * size, 0.5 * (seg_vals[k - 1] + seg_vals[k])) for k in range(len(ring))]
                # lower than the hot plate everywhere (its top stays the plate's own, never pierced)
                plate(crust, outline, ring, [CRUST] * len(ring), run_v, h * 0.85, tilt, rng, dome * 0.8,
                      uv_off=uv_off, rough=0.0)
    # flecks of crust drifting on the white-hot lake: broken bits, some long, some round
    turn0 = rng.uniform(0.0, 2.0 * math.pi)
    for f in range(flecks):
        a = turn0 + 2.0 * math.pi * (f + rng.uniform(-0.2, 0.2)) / max(flecks, 1)
        c = Vector((math.cos(a), math.sin(a))) * (core_r * rng.uniform(0.45, 0.85))
        rf = rng.uniform(0.01, 0.017)
        stretch = rng.uniform(0.2, 0.6)
        turn = rng.uniform(0.0, math.pi)
        ring = []
        for k in range(7):
            t = 2.0 * math.pi * k / 7
            r = rf * (1.0 + stretch * math.cos(2.0 * (t - turn))) * rng.uniform(0.8, 1.1)
            ring.append(c + Vector((math.cos(t) * r, math.sin(t) * r)))
        plate(mesh, outline, ring, [EDGE] * 7, [min(0.0015, 0.2 * rf)] * 7, rng.uniform(0.0016, 0.0026),
              Vector((0.0, 0.0)), rng, 0.0006)
    core_s = 0.17 if core_seeds else 0.0
    base_surface(mesh, outline, segments, rng, core_s)
    droplets(mesh, outline, rng, n_drops)
    print(f'LOOTER: {name}: {plates} plates, {len(open_cells)} open cells of {len(cells)}', flush=True)
    obj = mesh.finish(name)
    crust_obj = crust.finish(crust_name) if crust is not None else None
    return obj, crust_obj


POOL, POOL_CRUST = build('SlagPool', 5, [(3, 0.045), (5, 0.03), (8, 0.018), (13, 0.008)],
                         lambda r: 0.05 + 0.065 * (r / REGION), core_seeds=4, core_r=0.06, bays=0.0, max_bays=0,
                         n_drops=7, segments=56, flecks=2, crust_name='SlagPoolCrust', seg=0.04)
# how far the crust must go down to hide under the melt (its highest point under the melt's lowest level, at the rim)
CRUST_DROP = math.ceil((max(v.co.z for v in POOL_CRUST.data.vertices) - Z_MELT + 0.0005) * 1000.0) / 1000.0
print(f'LOOTER: SlagPoolCrust hides under the melt lowered {CRUST_DROP * 100:.1f} cm (model units)', flush=True)
SPLAT_A, _ = build('SlagSplatA', 21, [(2, 0.11), (3, 0.07), (5, 0.05), (9, 0.02)], lambda r: 0.12 + 0.05 * (r / REGION),
                   core_seeds=1, core_r=0.05, bays=0.0, max_bays=0, n_drops=9, segments=36, seg=0.055)
SPLAT_B, _ = build('SlagSplatB', 34, [(2, 0.09), (3, 0.1), (4, 0.05), (7, 0.03)], lambda r: 0.13 + 0.05 * (r / REGION),
                   core_seeds=1, core_r=0.045, bays=0.0, max_bays=0, n_drops=11, segments=36, seg=0.055)


# --- Previews: at the game's size (9x) on uneven, dusty ground in daylight ---

def preview_all():
    scene = bpy.context.scene
    out_dir = os.path.join(lt.PREVIEW_DIR, 'Props', 'SlagPool')
    os.makedirs(out_dir, exist_ok=True)
    added = []
    # uneven ground: gentle swells (a few cm over the pool's 9 m), dusty dirt
    n, size = 120, 40.0
    verts, faces = [], []
    for j in range(n + 1):
        for i in range(n + 1):
            x, y = -size / 2 + size * i / n, -size / 2 + size * j / n
            z = 0.035 * math.sin(x * 0.9 + 0.4) * math.cos(y * 0.7 - 0.3) + 0.02 * math.sin(x * 2.3 + y * 1.7) - 0.01
            verts.append((x, y, z))
    for j in range(n):
        for i in range(n):
            a = j * (n + 1) + i
            faces.append((a, a + 1, a + n + 2, a + n + 1))
    me = bpy.data.meshes.new('_Ground')
    me.from_pydata(verts, [], faces)
    me.uv_layers.new(name='UVMap')
    ground = bpy.data.objects.new('_Ground', me)
    scene.collection.objects.link(ground)
    for p in me.polygons:
        p.use_smooth = True
    lt.assign(ground, lt.material('GroundDirt', name='_PreviewDirt', tint=0xd8cbb4))
    lt.box_uv(ground, 'GroundDirt', texel_density=256)
    added.append(ground)
    # glows as Blender emission (the stylized material's Color x Glow)
    for mat in MATS[1:]:
        bsdf = next(nd for nd in mat.node_tree.nodes if nd.type == 'BSDF_PRINCIPLED')
        bsdf.inputs['Emission Color'].default_value = bsdf.inputs['Base Color'].default_value
        bsdf.inputs['Emission Strength'].default_value = float(mat['Glow'])
    for o in (POOL, POOL_CRUST, SPLAT_A, SPLAT_B):
        o.scale = (9.0, 9.0, 9.0)
    sun = bpy.data.objects.new('_Sun', bpy.data.lights.new('_Sun', 'SUN'))
    sun.data.energy = 4.0
    sun.data.color = (1.0, 0.9, 0.78)
    sun.data.angle = math.radians(2.0)
    toward = Vector((-0.55, -0.7, 0.0)).normalized() * math.cos(math.radians(45.0)) + Vector((0.0, 0.0, math.sin(math.radians(45.0))))
    sun.rotation_euler = (-toward).to_track_quat('-Z', 'Y').to_euler()
    scene.collection.objects.link(sun)
    added.append(sun)
    cam = bpy.data.objects.new('_Cam', bpy.data.cameras.new('_Cam'))
    scene.collection.objects.link(cam)
    added.append(cam)
    scene.camera = cam
    world = lt._preview_world(scene)
    scene.world = world
    scene.render.resolution_x, scene.render.resolution_y = 1600, 900
    scene.render.image_settings.file_format = 'PNG'
    scene.view_settings.view_transform = 'AgX'
    scene.view_settings.look = 'AgX - Medium High Contrast'
    try:
        scene.render.engine = 'BLENDER_EEVEE_NEXT'
    except TypeError:
        scene.render.engine = 'BLENDER_EEVEE'
    scene.eevee.taa_render_samples = 32
    # the game's bloom, roughly: a glare on what's brighter than white
    scene.use_nodes = True
    tree = scene.node_tree
    tree.nodes.clear()
    layers = tree.nodes.new('CompositorNodeRLayers')
    glare = tree.nodes.new('CompositorNodeGlare')
    try:
        glare.glare_type = 'BLOOM'
    except TypeError:
        glare.glare_type = 'FOG_GLOW'
    glare.threshold = 1.0
    glare.size = 7
    comp = tree.nodes.new('CompositorNodeComposite')
    tree.links.new(layers.outputs['Image'], glare.inputs['Image'])
    tree.links.new(glare.outputs['Image'], comp.inputs['Image'])

    def shot(file, at, look, lens, show):
        for o in (POOL, POOL_CRUST, SPLAT_A, SPLAT_B):
            o.hide_render = o not in show
        cam.location = at
        cam.rotation_euler = (Vector(look) - Vector(at)).to_track_quat('-Z', 'Y').to_euler()
        cam.data.lens = lens
        scene.render.filepath = os.path.join(out_dir, file)
        bpy.ops.render.render(write_still=True)
        print(f'LOOTER: preview {scene.render.filepath}', flush=True)

    POOL.location = POOL_CRUST.location = (0.0, 0.0, 0.0)
    POOL_CRUST.location = (0.0, 0.0, -CRUST_DROP * 9.0)
    shot('SlagPool_top.png', (0.0, -0.01, 15.0), (0.0, 0.0, 0.0), 32.0, [POOL, POOL_CRUST])
    shot('SlagPool_three.png', (-4.2, -7.6, 1.7), (0.3, 0.6, 0.0), 30.0, [POOL, POOL_CRUST])
    # cooled (near the end of its time): the crust raised, the glow down to a third
    POOL_CRUST.location = (0.0, 0.0, 0.0)
    for mat in MATS[1:]:
        bsdf = next(nd for nd in mat.node_tree.nodes if nd.type == 'BSDF_PRINCIPLED')
        bsdf.inputs['Emission Strength'].default_value = float(mat['Glow']) * 0.35
    shot('SlagPool_cooled_three.png', (-4.2, -7.6, 1.7), (0.3, 0.6, 0.0), 30.0, [POOL, POOL_CRUST])
    for mat in MATS[1:]:
        bsdf = next(nd for nd in mat.node_tree.nodes if nd.type == 'BSDF_PRINCIPLED')
        bsdf.inputs['Emission Strength'].default_value = float(mat['Glow'])
    SPLAT_A.location = (-3.2, 0.0, 0.0)
    SPLAT_B.location = (3.2, 0.0, 0.0)
    for o in (SPLAT_A, SPLAT_B):
        o.scale = (5.0, 5.0, 5.0)
    shot('SlagSplats.png', (0.0, -9.5, 6.0), (0.0, 0.0, 0.0), 30.0, [SPLAT_A, SPLAT_B])
    # leave the scene as it was built (the export reads it)
    for o in (POOL, POOL_CRUST, SPLAT_A, SPLAT_B):
        o.scale = (1.0, 1.0, 1.0)
        o.location = (0.0, 0.0, 0.0)
        o.hide_render = False
    for mat in MATS[1:]:
        bsdf = next(nd for nd in mat.node_tree.nodes if nd.type == 'BSDF_PRINCIPLED')
        bsdf.inputs['Emission Strength'].default_value = 0.0
    scene.use_nodes = False
    scene.world = None
    bpy.data.worlds.remove(world)
    for o in added:
        data = o.data
        bpy.data.objects.remove(o)
        if isinstance(data, bpy.types.Mesh):
            bpy.data.meshes.remove(data)
        elif isinstance(data, bpy.types.Camera):
            bpy.data.cameras.remove(data)
        elif isinstance(data, bpy.types.Light):
            bpy.data.lights.remove(data)


if lt.want_preview():
    preview_all()
