"""The Gravewind over Gravewind Canyon (Docs/Areas/RansomsRest.md: the Rim, Gravewind Point, Main 6 "The Gravewind";
Effects: "Gravewind wisps, unlit wisp cards along the Rim, fog rising out of the canyon at the deck"). Card meshes
built the way Smoke.py builds SmokePlume, for two unlit, translucent effect materials beside M_Smoke.

  GravewindWisp_A..D  the cold wind spilling off the Rim at dusk: long, thin ribbons (two or three strands each) that
                      lie low over the ground behind the lip, roll over it and curl down and out over the drop. A
                      "Spill" (7.4 m overall, falling 3.4 m), a short "Lick" (4 m), a long "Streamer" that the wind
                      carries out (8.7 m) and a "Curl" that turns sideways as it falls (6.8 m; corners, the flanks).
  CanyonFog_A..C      fog welling up out of the canyon under the deck: a few gently domed strata, densest at the
                      bottom, and a rising curtain near the cliff whose top leans out over the drop. "Welling" (A,
                      37 x 21 x 12.7 m, under the deck's west end), "Drift" (B, 38 x 29 x 7 m, low and wide, farther
                      out, where Abel drifts) and "Corner" (C, 32 x 19 x 11 m, its curtain swept round to wrap a flank).

Placement (the preview's, in the plan's metres, the deck centred at (-150, 12), its walking surface Z = 0): wisps
every 3.6 m along the deck's open west end (pivot on its edge at deck height, turned west +-12 deg) and along the
point's open lips, every 9 m along the Rim beside it; bank A with its back 3 m past the deck's end, base at -12; B
18 m past it, base -7; C off the point's south flank, base -13. Show them in the dusk lighting state only.

Pivots and facing: Blender's -Y is Unreal's +X (Art/README.md), so every card mesh points -Y out over the drop.
  wisps   the point where the ribbon leaves the lip (the lip's edge at ground level): the root lies 1.2-2.6 m behind
          it on the ground (+Y), the tail falls 1.9-3.4 m below it out over the drop.
  fog     the middle of the bank's base (its lowest point, the middle of its footprint), facing -Y.

Vertex color (linear, written through the linear API as Smoke.py does):
  A  opacity: 0 along every card's edges and ends, denser in the middle and toward the bottom.
  R  a noise phase, 0..1, constant per strand or card, so neighbouring strands and layers don't pan in step.
  G  drift weight, 0..1: 0 where a wisp lies on the ground behind the lip (anchored) rising to 1 at its tail; 0 at a
     fog card's base or middle rising to 1 at its top or rim (what may waft with a world-position offset).
  B  1 (unused).
UV0 (UVMap): Smoke's layout, U 0..1 across each card, V 0..1 along it (a wisp from root to tail, a curtain from bottom
to top, a stratum from the cliff side to its outer edge), so M_Smoke itself already draws them sensibly.
UV1 (Meters): the same directions in meters (U across, V along), so the effect materials can tile their noise at a
constant world scale whatever the card's size.

Material Master 'Smoke' (no texture set), slots GravewindWisp and CanyonFog. No Nanite, no collision, one slot each.
The node trees here are only the preview's stand-in for the effect materials (described in the report); the export
reads Master. --preview renders a mock of Gravewind Point at dusk (a cliff, a deck stand-in) with the kit placed into
Saved/ArtPreviews/RansomsRest/Gravewind/ (the views, an overdraw count from the deck, Cards.png) and
RansomsRest/Gravewind_overview.png, and logs how many cards overlap (GRAVEWIND_LAYERS=1 logs that without previews;
GRAVEWIND_PREVIEW=quick renders fewer samples).

    blender -b --factory-startup --python Art/Models/Props/Gravewind.py -- --preview
"""
import math
import os
import random

import bmesh
import bpy
from mathutils import Vector
from mathutils.bvhtree import BVHTree

import looter_textures as lt

WISP_MAT, FOG_MAT = 'GravewindWisp', 'CanyonFog'
LAT, UP = Vector((1.0, 0.0, 0.0)), Vector((0.0, 0.0, 1.0))
TAU = 2.0 * math.pi


def smoothstep(e0, e1, x):
    t = min(max((x - e0) / (e1 - e0), 0.0), 1.0)
    return t * t * (3.0 - 2.0 * t)


def lerp(a, b, t):
    return a + (b - a) * t


def flow(s, w, h):
    """Flow coordinates (s out over the drop, w across, h up) in Blender's axes: out is -Y (Unreal's +X)."""
    return Vector((w, -s, h))


# --- Card meshes ---

class Cards:
    """Vertices with their own UVs and color (no two cards share a vertex), faces, and the order they were made in:
    the faces of a fog bank are made far to near for the view from the deck (bottom stratum first, the curtain last),
    the order they blend in when a mesh draws in its index order."""

    def __init__(self):
        self.pos, self.uv0, self.uv1, self.col, self.faces = [], [], [], [], []

    def vert(self, p, uv0, uv1, col):
        self.pos.append(Vector(p))
        self.uv0.append(tuple(uv0))
        self.uv1.append(None if uv1 is None else tuple(uv1))
        self.col.append(tuple(col))
        return len(self.pos) - 1

    def face(self, *idx):
        self.faces.append(tuple(idx))

    def rest_on_base(self):
        """Moves the pivot to the middle of the footprint at the lowest point (the fog banks' pivot)."""
        lo = Vector((min(p.x for p in self.pos), min(p.y for p in self.pos), min(p.z for p in self.pos)))
        hi = Vector((max(p.x for p in self.pos), max(p.y for p in self.pos), max(p.z for p in self.pos)))
        shift = Vector(((lo.x + hi.x) * 0.5, (lo.y + hi.y) * 0.5, lo.z))
        for p in self.pos:
            p -= shift

    def build(self, name, mat):
        bm = bmesh.new()
        uv0 = bm.loops.layers.uv.new('UVMap')
        uv1 = bm.loops.layers.uv.new('Meters')
        col = bm.loops.layers.color.new('Col')
        verts = [bm.verts.new(p) for p in self.pos]
        for idx in self.faces:
            face = bm.faces.new([verts[i] for i in idx])
            for loop, i in zip(face.loops, idx):
                loop[uv0].uv = self.uv0[i]
                # A stratum's meters are planar, from its final position: across = x, out = -y.
                loop[uv1].uv = self.uv1[i] if self.uv1[i] is not None else (self.pos[i].x, -self.pos[i].y)
                loop[col] = self.col[i]
        mesh = bpy.data.meshes.new(name)
        bm.to_mesh(mesh)
        bm.free()
        obj = bpy.data.objects.new(name, mesh)
        bpy.context.scene.collection.objects.link(obj)
        mesh.materials.append(mat)
        for p in mesh.polygons:
            p.use_smooth = True
        # Written as stored bytes; rewrite through the linear API, which is what the exporter reads back (Smoke.py).
        attr = lt._col_attribute(mesh)
        raw = [tuple(d.color_srgb) for d in attr.data]
        for d, value in zip(attr.data, raw):
            d.color = value
        obj['Nanite'] = 0
        obj['Collision'] = 'None'
        return obj


# --- Wisps ---

class Path:
    """A smooth curve through control points (Catmull-Rom), walked by arc-length fraction."""

    def __init__(self, points, per_segment=32):
        pts = [Vector(p) for p in points]
        ext = [pts[0] + (pts[0] - pts[1])] + pts + [pts[-1] + (pts[-1] - pts[-2])]
        dense = []
        for i in range(1, len(ext) - 2):
            p0, p1, p2, p3 = ext[i - 1], ext[i], ext[i + 1], ext[i + 2]
            for k in range(per_segment):
                t = k / per_segment
                t2, t3 = t * t, t * t * t
                dense.append(0.5 * ((2.0 * p1) + (p2 - p0) * t + (2.0 * p0 - 5.0 * p1 + 4.0 * p2 - p3) * t2
                                    + (3.0 * p1 - p0 - 3.0 * p2 + p3) * t3))
        dense.append(pts[-1].copy())
        self.points = dense
        self.cum = [0.0]
        for a, b in zip(dense, dense[1:]):
            self.cum.append(self.cum[-1] + (b - a).length)
        self.length = self.cum[-1]

    def at(self, u):
        d = min(max(u, 0.0), 1.0) * self.length
        lo, hi = 0, len(self.cum) - 1
        while hi - lo > 1:
            mid = (lo + hi) // 2
            if self.cum[mid] <= d:
                lo = mid
            else:
                hi = mid
        span = self.cum[hi] - self.cum[lo]
        t = 0.0 if span <= 1e-9 else (d - self.cum[lo]) / span
        return self.points[lo].lerp(self.points[hi], t)

    def tangent(self, u, du=0.004):
        return (self.at(u + du) - self.at(u - du)).normalized()

    def lip(self):
        """The arc fraction where the curve passes over the lip (out = 0)."""
        for i, p in enumerate(self.points):
            if -p.y >= 0.0:
                return self.cum[i] / self.length
        return 0.0


def strand(cards, path, spec, rng):
    """One ribbon along the wisp's path: offset sideways, spiralling round it past the lip (the curl), twisting as
    it goes. Three vertices across (alpha 0 at both edges), the middle one lifted a little off the ribbon's plane so
    the cross-section is a shallow V and never goes fully edge-on."""
    rows = spec['rows']
    t0, t1 = spec['t']
    t_lip = path.lip()
    phase = rng.random()
    centers, ts = [], []
    for i in range(rows):
        f = i / (rows - 1)
        t = lerp(t0, t1, f)
        m, tan = path.at(t), path.tangent(t)
        n1 = (LAT - tan * LAT.dot(tan)).normalized()
        n2 = tan.cross(n1)
        offset = lerp(spec['offset'][0], spec['offset'][1], f)
        curl = spec.get('helix', 0.0) * smoothstep(t_lip, t_lip + 0.35, t)
        phi = spec.get('phi', 0.0) + TAU * spec.get('turns', 0.0) * max(t - t_lip, 0.0)
        centers.append(m + n1 * offset + (n1 * math.cos(phi) + n2 * math.sin(phi)) * curl)
        ts.append(t)
    arc = [0.0]
    for a, b in zip(centers, centers[1:]):
        arc.append(arc[-1] + (b - a).length)
    w0, wm, wt = spec['width']
    tw0, tw1 = spec['twist']
    peak = spec['alpha']
    fade_in, fade_out = spec.get('fade', (0.14, 0.5))
    rows_idx = []
    for i, (c, t) in enumerate(zip(centers, ts)):
        f = i / (rows - 1)
        a, b = centers[max(i - 1, 0)], centers[min(i + 1, rows - 1)]
        ts_ = (b - a).normalized()
        n1 = (LAT - ts_ * LAT.dot(ts_)).normalized()
        b1 = ts_.cross(n1)
        theta = math.radians(lerp(tw0, tw1, f ** 1.1))
        side = n1 * math.cos(theta) + b1 * math.sin(theta)
        normal = ts_.cross(side)
        half = w0 + (wm - w0) * smoothstep(0.0, 0.4, f) + (wt - wm) * smoothstep(0.45, 1.0, f)
        alpha = peak * smoothstep(0.0, fade_in, f) * (1.0 - smoothstep(fade_out, 1.0, f))
        alpha *= 0.85 + 0.15 * math.sin(f * 7.0 + phase * TAU)
        drift = smoothstep(t_lip - 0.05, 1.0, t)
        v = arc[i] / arc[-1]
        along = t * path.length
        row = []
        for s in (-1.0, 0.0, 1.0):
            p = c + side * (s * half) if s else c + normal * (spec.get('cup', 0.18) * half)
            row.append(cards.vert(p, ((s + 1.0) * 0.5, v), (s * half, along),
                                  (phase, drift, 1.0, alpha if s == 0.0 else 0.0)))
        rows_idx.append(row)
    for r0, r1 in zip(rows_idx, rows_idx[1:]):
        for j in range(2):
            cards.face(r0[j], r0[j + 1], r1[j + 1], r1[j])


# Each wisp: its path in flow coordinates (s out over the drop from the lip, w across, h up; the root lies on the
# ground behind the lip at s < 0) and its strands. A strand: rows, the part of the path it covers (t), its sideways
# offset at start and end, its half width at root, middle and tail, twist (degrees) at root and tail, the spiral
# round the path past the lip (helix radius, turns, start angle), peak alpha and where it fades in and out.
WISPS = {
    'A': dict(seed=4101, path=[(-2.2, 0.0, 0.17), (-1.2, 0.04, 0.18), (-0.3, 0.1, 0.24), (0.5, 0.15, 0.04),
                               (1.2, 0.2, -0.7), (2.0, 0.3, -1.55), (3.0, 0.5, -2.25), (4.1, 0.7, -2.6),
                               (5.0, 0.9, -2.45)],
              strands=[dict(rows=16, t=(0.0, 1.0), offset=(0.0, 0.0), width=(0.2, 0.62, 1.0), twist=(0.0, 70.0),
                            helix=0.3, turns=0.7, alpha=0.95),
                       dict(rows=12, t=(0.12, 0.88), offset=(0.32, 0.6), width=(0.15, 0.4, 0.62), twist=(80.0, 25.0),
                            helix=0.65, turns=1.1, phi=1.8, alpha=0.75, fade=(0.2, 0.45)),
                       dict(rows=10, t=(0.05, 0.72), offset=(-0.28, -0.5), width=(0.12, 0.28, 0.45),
                            twist=(35.0, 150.0), helix=0.5, turns=1.3, phi=3.6, alpha=0.6, fade=(0.2, 0.4))]),
    'B': dict(seed=4102, path=[(-1.3, 0.0, 0.17), (-0.6, 0.0, 0.18), (0.0, 0.05, 0.2), (0.6, 0.1, -0.12),
                               (1.2, 0.15, -0.75), (1.9, 0.2, -1.25), (2.4, 0.25, -1.45)],
              strands=[dict(rows=12, t=(0.0, 1.0), offset=(0.0, 0.05), width=(0.18, 0.54, 0.8), twist=(0.0, 55.0),
                            helix=0.15, turns=0.5, alpha=0.85),
                       dict(rows=9, t=(0.15, 0.9), offset=(-0.3, -0.45), width=(0.12, 0.3, 0.48), twist=(75.0, 20.0),
                            helix=0.45, turns=1.0, phi=2.4, alpha=0.7, fade=(0.2, 0.45))]),
    'C': dict(seed=4103, path=[(-2.2, 0.0, 0.17), (-1.2, 0.0, 0.19), (-0.3, -0.05, 0.3), (0.7, -0.15, 0.08),
                               (1.7, -0.3, -0.55), (2.9, -0.5, -1.15), (4.1, -0.6, -1.5), (5.2, -0.5, -1.6),
                               (6.0, -0.35, -1.45)],
              strands=[dict(rows=18, t=(0.0, 1.0), offset=(0.0, 0.0), width=(0.18, 0.56, 0.95), twist=(0.0, 60.0),
                            helix=0.3, turns=1.1, alpha=0.85, fade=(0.1, 0.55)),
                       dict(rows=13, t=(0.1, 0.85), offset=(0.3, 0.75), width=(0.12, 0.34, 0.54), twist=(85.0, 10.0),
                            helix=0.6, turns=1.5, phi=2.0, alpha=0.7, fade=(0.18, 0.45)),
                       dict(rows=10, t=(0.3, 1.0), offset=(-0.25, -0.65), width=(0.12, 0.3, 0.58), twist=(30.0, 120.0),
                            helix=0.45, turns=0.9, phi=4.0, alpha=0.55, fade=(0.25, 0.4))]),
    'D': dict(seed=4104, path=[(-1.8, 0.0, 0.17), (-0.9, 0.1, 0.18), (0.0, 0.25, 0.22), (0.7, 0.55, -0.25),
                               (1.3, 1.0, -1.05), (2.0, 1.5, -1.75), (2.9, 1.8, -2.15), (3.8, 1.9, -2.1),
                               (4.4, 1.85, -1.8)],
              strands=[dict(rows=15, t=(0.0, 1.0), offset=(0.0, 0.0), width=(0.2, 0.6, 0.9), twist=(0.0, 95.0),
                            helix=0.45, turns=1.0, alpha=0.9),
                       dict(rows=11, t=(0.1, 0.9), offset=(0.3, 0.5), width=(0.12, 0.34, 0.54), twist=(70.0, 160.0),
                            helix=0.8, turns=1.3, phi=2.6, alpha=0.7, fade=(0.2, 0.45))]),
}


def wisp(key, mat):
    spec = WISPS[key]
    rng = random.Random(spec['seed'])
    path = Path([flow(*p) for p in spec['path']])
    cards = Cards()
    for s in spec['strands']:
        strand(cards, path, s, rng)
    return cards.build(f'GravewindWisp_{key}', mat)


# --- Fog banks ---

def stratum(cards, rng, center, a, b, dome, tilt, peak, segments=18, wave=0.5):
    """A gently domed sheet of fog with a lobed outline (no wasted corners): a polar grid round its middle, alpha
    full in the middle and 0 on the rim, broken up round the rim. tilt is its rise per meter outward (negative: it
    stands higher toward the cliff, welling up there). a and b are its half extents out and across."""
    sc, wc, hc = center
    lobes = [(k, rng.uniform(0.05, 0.11) * 3.0 / k, rng.uniform(0.0, TAU)) for k in (2, 3, 4, 5)]
    patches = [(k, rng.uniform(0.15, 0.3), rng.uniform(0.0, TAU)) for k in (1, 2, 3)]
    p1, p2 = rng.uniform(0.0, TAU), rng.uniform(0.0, TAU)
    phase = rng.random()
    rings = (0.0, 0.3, 0.58, 0.8, 1.0)
    ring_alpha = (1.0, 0.95, 0.68, 0.3, 0.0)
    pts = []
    for ri, f in enumerate(rings):
        for j in range(segments if f > 0.0 else 1):
            phi = TAU * j / segments
            rho = 1.0 + sum(amp * math.sin(k * phi + ph) for k, amp, ph in lobes)
            s = sc + a * f * rho * math.cos(phi)
            w = wc + b * f * rho * math.sin(phi)
            h = (hc + dome * (1.0 - f * f) + tilt * (s - sc)
                 + wave * math.sin(0.33 * s + p1) * math.cos(0.27 * w + p2) * f)
            patch = 1.0 - sum(amp * (0.5 + 0.5 * math.sin(k * phi + ph)) for k, amp, ph in patches) * f
            alpha = peak * ring_alpha[ri] * max(patch, 0.0)
            pts.append((s, w, h, alpha, f))
    s_lo, s_hi = min(p[0] for p in pts), max(p[0] for p in pts)
    w_lo, w_hi = min(p[1] for p in pts), max(p[1] for p in pts)
    idx = []
    for s, w, h, alpha, f in pts:
        uv0 = ((w - w_lo) / (w_hi - w_lo), (s - s_lo) / (s_hi - s_lo))
        idx.append(cards.vert(flow(s, w, h), uv0, None, (phase, f, 1.0, alpha)))
    center_i = idx[0]
    ring = lambda r, j: idx[1 + (r - 1) * segments + (j % segments)]
    for j in range(segments):
        cards.face(center_i, ring(1, j + 1), ring(1, j))
    for r in range(1, len(rings) - 1):
        for j in range(segments):
            cards.face(ring(r, j), ring(r, j + 1), ring(r + 1, j + 1), ring(r + 1, j))


def curtain(cards, rng, s_back, half, height, peak, bow=3.0, skew=0.0, lean=4.0, cols=11, fingers=0.32):
    """Fog rising up the cliff face: a card standing across the bank near its back, its ends swept forward (bow,
    skew), leaning out over the drop as it rises (lean, more toward the top), with an uneven top (fingers of fog
    rising higher than others). Alpha: soft at the foot (it rises out of the bottom stratum), densest low, 0 at the
    top and both ends."""
    rows = (0.0, 0.12, 0.3, 0.5, 0.7, 0.86, 1.0)
    row_alpha = (0.4, 0.9, 0.82, 0.6, 0.34, 0.13, 0.0)
    q = [rng.uniform(0.0, TAU) for _ in range(4)]
    phase = rng.random()
    grid = []
    for j in range(cols):
        u = j / (cols - 1)
        x = 2.0 * u - 1.0
        w = half * x
        s0 = s_back + bow * x * x + skew * x
        reach = 1.0 - fingers * (0.5 + 0.25 * math.sin(5.1 * u + q[0]) + 0.25 * math.sin(9.7 * u + q[1]))
        end = smoothstep(0.0, 0.2, u) * smoothstep(0.0, 0.2, 1.0 - u)
        column, up = [], 0.0
        prev = None
        for k, v in enumerate(rows):
            h = height * reach * v
            s = s0 + lean * v ** 1.6 + 0.8 * math.sin(3.0 * math.pi * u + q[2]) * v
            w_k = w + 0.5 * math.sin(4.0 * u + q[3]) * v
            p = flow(s, w_k, h)
            if prev is not None:
                up += (p - prev).length
            prev = p
            alpha = peak * row_alpha[k] * end * (0.82 + 0.18 * math.sin(7.3 * u + q[1] + v))
            column.append(cards.vert(p, (u, v), (w, up), (phase, v, 1.0, alpha)))
        grid.append(column)
    for c0, c1 in zip(grid, grid[1:]):
        for k in range(len(rows) - 1):
            cards.face(c0[k], c1[k], c1[k + 1], c0[k + 1])


# Each bank: strata (center (out, across, up), half extents out and across, dome height, tilt, peak alpha), then
# its curtains. Strata go in bottom first and the curtain last (far to near from the deck).
BANKS = {
    'A': dict(seed=4201,
              strata=[((0.0, 0.0, 0.0), 11.0, 17.0, 2.0, -0.10, 0.85),
                      ((-2.5, 2.0, 3.6), 7.5, 11.5, 1.5, -0.14, 0.62),
                      ((3.5, -5.5, 6.2), 5.5, 7.5, 1.0, -0.10, 0.48)],
              curtains=[dict(s_back=-10.0, half=14.0, height=12.0, peak=0.8, bow=3.5, lean=5.0)]),
    'B': dict(seed=4202,
              strata=[((0.0, 0.0, 0.0), 13.0, 20.0, 1.8, -0.04, 0.8),
                      ((2.0, -5.0, 3.0), 9.0, 12.5, 1.3, -0.05, 0.6),
                      ((-3.0, 6.5, 5.2), 6.5, 8.5, 1.0, -0.06, 0.46)],
              curtains=[]),
    'C': dict(seed=4203,
              strata=[((0.0, 0.0, 0.0), 9.0, 13.0, 1.6, -0.12, 0.8),
                      ((-1.5, 3.0, 3.4), 6.0, 8.0, 1.2, -0.12, 0.6)],
              curtains=[dict(s_back=-8.0, half=11.0, height=10.0, peak=0.7, bow=2.0, skew=3.5, lean=4.0)]),
}


def fog_bank(key, mat):
    spec = BANKS[key]
    rng = random.Random(spec['seed'])
    cards = Cards()
    for center, a, b, dome, tilt, peak in spec['strata']:
        stratum(cards, rng, center, a, b, dome, tilt, peak)
    for c in spec['curtains']:
        curtain(cards, rng, **c)
    cards.rest_on_base()
    return cards.build(f'CanyonFog_{key}', mat)


# --- Materials ---

SUN_AZIMUTH, SUN_ELEVATION = 252.0, 4.0


def toward_sun():
    """The direction to the dusk sun in the preview's world (x east, y north, z up)."""
    a, e = math.radians(SUN_AZIMUTH), math.radians(SUN_ELEVATION)
    return Vector((math.sin(a) * math.cos(e), math.cos(a) * math.cos(e), math.sin(e)))


# The stand-in's look per material: noise scale across and along (per meter of UV1), threshold, opacity, the cold
# color (shade side), the warm color toward the sun and how tight its glow is, emission strength, the fade where a
# card turns edge-on, how dark the bottom of a bank is, and the distance it fades out over (meters).
LOOKS = {
    WISP_MAT: dict(scale=(2.4, 0.26), cut=(0.34, 0.72), opacity=0.8, cold=0xb0b6d4, warm=0xffc08c, glow=6.0,
                   strength=0.85, edge=2.5, base_dark=1.0, fade=(25.0, 60.0)),
    FOG_MAT: dict(scale=(0.08, 0.08), cut=(0.3, 0.66), opacity=1.0, cold=0xbcc0da, warm=0xffc690, glow=3.0,
                  strength=1.15, edge=2.5, base_dark=0.6, fade=(400.0, 900.0), swirl=0.6),
}


def card_material(name):
    """Master 'Smoke' for the export. The nodes are the preview's stand-in for the effect material: unlit, opacity =
    vertex A x panning noise (UV1 meters, R its phase) x a fade where the card turns edge-on x Opacity; the color
    cold blue-violet, warming toward the sun, a bank darker toward its base."""
    look = LOOKS[name]
    mat = bpy.data.materials.get(name) or bpy.data.materials.new(name)
    mat['Master'] = 'Smoke'
    mat.use_nodes = True
    nodes, links = mat.node_tree.nodes, mat.node_tree.links
    nodes.clear()

    def node(kind, **values):
        n = nodes.new(kind)
        for key, value in values.items():
            setattr(n, key, value)
        return n

    def math_node(op, a, b=None, clamp=False):
        n = node('ShaderNodeMath', operation=op, use_clamp=clamp)
        for i, value in enumerate((a, b)):
            if value is None:
                continue
            if isinstance(value, (int, float)):
                n.inputs[i].default_value = value
            else:
                links.new(value, n.inputs[i])
        return n.outputs[0]

    out = node('ShaderNodeOutputMaterial')
    vcol = node('ShaderNodeVertexColor', layer_name='Col')
    rgb = node('ShaderNodeSeparateColor')
    links.new(vcol.outputs['Color'], rgb.inputs['Color'])
    meters = node('ShaderNodeUVMap', uv_map='Meters')
    xy = node('ShaderNodeSeparateXYZ')
    links.new(meters.outputs['UV'], xy.inputs['Vector'])
    vec = node('ShaderNodeCombineXYZ')
    links.new(math_node('MULTIPLY', xy.outputs['X'], look['scale'][0]), vec.inputs['X'])
    links.new(math_node('MULTIPLY', xy.outputs['Y'], look['scale'][1]), vec.inputs['Y'])
    links.new(math_node('MULTIPLY', rgb.outputs['Red'], 9.0), vec.inputs['Z'])
    noise = node('ShaderNodeTexNoise', noise_dimensions='3D')
    noise.inputs['Scale'].default_value = 1.0
    noise.inputs['Detail'].default_value = 3.0
    noise.inputs['Roughness'].default_value = 0.55
    noise.inputs['Distortion'].default_value = look.get('swirl', 0.0)
    links.new(vec.outputs['Vector'], noise.inputs['Vector'])
    cut = node('ShaderNodeMapRange', interpolation_type='SMOOTHSTEP')
    cut.inputs['From Min'].default_value, cut.inputs['From Max'].default_value = look['cut']
    links.new(noise.outputs['Fac'], cut.inputs['Value'])
    geo = node('ShaderNodeNewGeometry')
    facing = node('ShaderNodeVectorMath', operation='DOT_PRODUCT')
    links.new(geo.outputs['Normal'], facing.inputs[0])
    links.new(geo.outputs['Incoming'], facing.inputs[1])
    edge = math_node('MULTIPLY', math_node('ABSOLUTE', facing.outputs['Value']), look['edge'], clamp=True)
    alpha = math_node('MULTIPLY', vcol.outputs['Alpha'], cut.outputs['Result'])
    alpha = math_node('MULTIPLY', alpha, edge)
    # Gone with distance (the wisps are near detail: full within 25 m, gone by 60 m).
    camera = node('ShaderNodeCameraData')
    far = node('ShaderNodeMapRange', interpolation_type='SMOOTHSTEP')
    far.inputs['From Min'].default_value, far.inputs['From Max'].default_value = look['fade']
    far.inputs['To Min'].default_value, far.inputs['To Max'].default_value = 1.0, 0.0
    links.new(camera.outputs['View Distance'], far.inputs['Value'])
    alpha = math_node('MULTIPLY', alpha, far.outputs['Result'])
    alpha = math_node('MULTIPLY', alpha, look['opacity'], clamp=True)
    # Toward the sun: the view direction (-Incoming) against the direction to the sun.
    sun = node('ShaderNodeVectorMath', operation='DOT_PRODUCT')
    links.new(geo.outputs['Incoming'], sun.inputs[0])
    sun.inputs[1].default_value = -toward_sun()
    glow = math_node('POWER', math_node('MAXIMUM', sun.outputs['Value'], 0.0), look['glow'])
    color = node('ShaderNodeMix', data_type='RGBA')
    color.inputs['A'].default_value = lt.hex_color(look['cold'])
    color.inputs['B'].default_value = lt.hex_color(look['warm'])
    links.new(glow, color.inputs['Factor'])
    # A bank is darker toward its base (the canyon's shade); its height in object space over 10 m.
    coords = node('ShaderNodeTexCoord')
    oz = node('ShaderNodeSeparateXYZ')
    links.new(coords.outputs['Object'], oz.inputs['Vector'])
    lift = node('ShaderNodeMapRange')
    lift.inputs['From Min'].default_value, lift.inputs['From Max'].default_value = 0.0, 10.0
    lift.inputs['To Min'].default_value, lift.inputs['To Max'].default_value = look['base_dark'], 1.0
    links.new(oz.outputs['Z'], lift.inputs['Value'])
    shade = node('ShaderNodeMix', data_type='RGBA', blend_type='MULTIPLY')
    shade.inputs['Factor'].default_value = 1.0
    links.new(color.outputs['Result'], shade.inputs['A'])
    grey = node('ShaderNodeCombineColor')
    for channel in ('Red', 'Green', 'Blue'):
        links.new(lift.outputs['Result'], grey.inputs[channel])
    links.new(grey.outputs['Color'], shade.inputs['B'])
    emission = node('ShaderNodeEmission')
    emission.inputs['Strength'].default_value = look['strength']
    links.new(shade.outputs['Result'], emission.inputs['Color'])
    clear = node('ShaderNodeBsdfTransparent')
    mix = node('ShaderNodeMixShader')
    links.new(alpha, mix.inputs['Fac'])
    links.new(clear.outputs['BSDF'], mix.inputs[1])
    links.new(emission.outputs['Emission'], mix.inputs[2])
    links.new(mix.outputs['Shader'], out.inputs['Surface'])
    if hasattr(mat, 'surface_render_method'):
        mat.surface_render_method = 'BLENDED'
    mat.use_backface_culling = False
    if hasattr(mat, 'use_transparent_shadow'):
        mat.use_transparent_shadow = True
    return mat


# --- Build ---

def triangles(obj):
    return sum(len(p.vertices) - 2 for p in obj.data.polygons)


def card_area(obj):
    """Total card area (m2) and the area weighted by mean vertex alpha (what actually shows)."""
    mesh = obj.data
    alphas = lt._read_col(mesh)[:, 3]
    total = weighted = 0.0
    for p in mesh.polygons:
        total += p.area
        weighted += p.area * sum(alphas[i] for i in p.loop_indices) / p.loop_total
    return total, weighted


def layers(obj, direction, step):
    """How many cards a ray crosses where they show (alpha over 0.05), along direction, over a grid of rays covering
    the mesh: (worst, share of covered rays crossing 2+, 3+)."""
    mesh = obj.data
    bm = bmesh.new()
    bm.from_mesh(mesh)
    col = bm.loops.layers.color.get('Col')
    bmesh.ops.triangulate(bm, faces=bm.faces[:])
    tris, alphas = [], []
    for f in bm.faces:
        tris.append([l.vert.co.copy() for l in f.loops])
        # Byte colors: alpha isn't color managed, so the stored value is the alpha.
        alphas.append([l[col][3] for l in f.loops])
    bm.free()
    verts = [v for t in tris for v in t]
    bvh = BVHTree.FromPolygons(verts, [(3 * i, 3 * i + 1, 3 * i + 2) for i in range(len(tris))])
    d = Vector(direction).normalized()
    u = d.orthogonal().normalized()
    v = d.cross(u).normalized()
    center = sum(verts, Vector()) / len(verts)
    radius = max((p - center).length for p in verts)
    n = int(2.0 * radius / step) + 1
    counts = []
    from mathutils.geometry import barycentric_transform
    for i in range(n):
        for j in range(n):
            origin = center + u * (-radius + i * step) + v * (-radius + j * step) - d * (radius + 1.0)
            hits, travelled = 0, 0.0
            o = origin
            while travelled < 2.0 * radius + 2.0:
                loc, normal, index, dist = bvh.ray_cast(o, d, 2.0 * radius + 2.0 - travelled)
                if loc is None:
                    break
                t = tris[index]
                w = barycentric_transform(loc, t[0], t[1], t[2], Vector((1, 0, 0)), Vector((0, 1, 0)),
                                          Vector((0, 0, 1)))
                if sum(a * b for a, b in zip(w, alphas[index])) > 0.05:
                    hits += 1
                travelled += dist + 1e-3
                o = loc + d * 1e-3
            if hits:
                counts.append(hits)
    if not counts:
        return 0, 0.0, 0.0
    return max(counts), sum(c >= 2 for c in counts) / len(counts), sum(c >= 3 for c in counts) / len(counts)


def report(obj):
    lo = Vector((min(v.co.x for v in obj.data.vertices), min(v.co.y for v in obj.data.vertices),
                 min(v.co.z for v in obj.data.vertices)))
    hi = Vector((max(v.co.x for v in obj.data.vertices), max(v.co.y for v in obj.data.vertices),
                 max(v.co.z for v in obj.data.vertices)))
    total, weighted = card_area(obj)
    # Unreal's axes: X = -Blender Y (out over the drop), Y = -Blender X, Z up.
    lt._log(f'{obj.name}: {triangles(obj)} triangles, {len(obj.data.vertices)} vertices; '
            f'Unreal X {-hi.y:.2f}..{-lo.y:.2f} (out), Y {-hi.x:.2f}..{-lo.x:.2f}, Z {lo.z:.2f}..{hi.z:.2f} m; '
            f'size {hi.y - lo.y:.1f} out x {hi.x - lo.x:.1f} across x {hi.z - lo.z:.1f} up; card area {total:.1f} m2 '
            f'(alpha-weighted {weighted:.1f} m2)')
    return lo, hi


def report_layers(objs):
    views = {'from above': (0.0, 0.0, -1.0), 'from the deck (out, 30 deg down)': (0.0, -0.866, -0.5),
             'from the side': (-1.0, 0.0, -0.1)}
    for obj in objs:
        step = 0.04 if obj.name.startswith('GravewindWisp') else 0.4
        text = '; '.join(f'{name}: worst {w}, 2+ {two:.0%}, 3+ {three:.0%}'
                         for name, (w, two, three) in ((n, layers(obj, d, step)) for n, d in views.items()))
        lt._log(f'{obj.name} layers ({text})')


# --- Preview: Gravewind Point at dusk ---
# The preview's world: x east, y north, z up, the deck's middle at the origin (the plan's (-150, 12)), its walking
# surface 0.4 m over the rock (z = 0). The deck runs x -12.5..12.5 (its west end over the drop), y -9..9; the lip
# under it runs at x = -9.5 (BurialDeck.py), and the canyon floor is 70 m down.

OUT = os.path.join(lt.PREVIEW_DIR, 'RansomsRest', 'Gravewind')
DECK_Z, LIP_X, FLOOR_Z = 0.4, -9.5, -70.0
SKY = [(0.0, 0x15131c), (0.47, 0x29253a), (0.5, 0x9a8aa2), (0.515, 0x857c9e), (0.56, 0x625f90), (0.7, 0x41447a),
       (1.0, 0x252a55)]
SKY_GLOWS = [(0xff8840, 5.0, 0.8), (0xffb468, 40.0, 1.6), (0xfff0d8, 1400.0, 40.0)]
HAZE = 0x7f7a9c
HAZE_SUN = 0xe0a070
# The lip of Gravewind Point and the Rim beside it, south to north: land lies east of it (on its right walking north).
OUTLINE = [(34, -95), (32, -60), (31, -40), (28, -26), (22, -20), (12, -17), (2, -15), (-5, -13.5), (-8.5, -12),
           (-9.5, -9), (-9.7, -4), (-9.6, 0), (-9.5, 5), (-9.3, 9), (-8, 12), (-3, 14), (6, 15.5), (15, 17.5),
           (22, 21), (28, 27), (31, 40), (32, 60), (34, 95)]


def sky_world():
    """The dusk sky: blue-violet overhead, lavender at the horizon, a warm glow round the low sun (the level's dusk,
    Docs/Areas/RansomsRest.md; the same build as the town's day/dusk sheets)."""
    world = bpy.data.worlds.new('_DuskSky')
    world.use_nodes = True
    nodes, links = world.node_tree.nodes, world.node_tree.links
    nodes.clear()
    out = nodes.new('ShaderNodeOutputWorld')
    background = nodes.new('ShaderNodeBackground')
    coords = nodes.new('ShaderNodeTexCoord')
    split = nodes.new('ShaderNodeSeparateXYZ')
    links.new(coords.outputs['Generated'], split.inputs['Vector'])
    remap = nodes.new('ShaderNodeMapRange')
    remap.inputs['From Min'].default_value = -1.0
    links.new(split.outputs['Z'], remap.inputs['Value'])
    ramp = nodes.new('ShaderNodeValToRGB')
    links.new(remap.outputs['Result'], ramp.inputs['Fac'])
    elements = ramp.color_ramp.elements
    elements[0].position, elements[0].color = SKY[0][0], lt.hex_color(SKY[0][1])
    elements[1].position, elements[1].color = SKY[-1][0], lt.hex_color(SKY[-1][1])
    for position, color in SKY[1:-1]:
        elements.new(position).color = lt.hex_color(color)
    color = ramp.outputs['Color']
    dot = nodes.new('ShaderNodeVectorMath')
    dot.operation = 'DOT_PRODUCT'
    links.new(coords.outputs['Generated'], dot.inputs[0])
    dot.inputs[1].default_value = toward_sun()
    clamp = nodes.new('ShaderNodeMath')
    clamp.operation = 'MAXIMUM'
    links.new(dot.outputs['Value'], clamp.inputs[0])
    for glow, power, strength in SKY_GLOWS:
        lift = nodes.new('ShaderNodeMath')
        lift.operation = 'POWER'
        links.new(clamp.outputs['Value'], lift.inputs[0])
        lift.inputs[1].default_value = power
        scale = nodes.new('ShaderNodeMath')
        scale.operation = 'MULTIPLY'
        links.new(lift.outputs['Value'], scale.inputs[0])
        scale.inputs[1].default_value = strength
        add = nodes.new('ShaderNodeMix')
        add.data_type, add.blend_type = 'RGBA', 'ADD'
        links.new(scale.outputs['Value'], add.inputs['Factor'])
        links.new(color, add.inputs['A'])
        add.inputs['B'].default_value = lt.hex_color(glow)
        color = add.outputs['Result']
    links.new(color, background.inputs['Color'])
    links.new(background.outputs['Background'], out.inputs['Surface'])
    return world


def hazed(name, color, rough=0.9, mottle=0.0, mottle_scale=0.15, emit=None, haze=280.0,
          air=(HAZE, 0.85, HAZE_SUN)):
    """An opaque stand-in surface that fades into the canyon's haze with distance (air: its color, brightness and
    color toward the sun)."""
    mat = bpy.data.materials.new(name)
    mat.use_nodes = True
    nodes, links = mat.node_tree.nodes, mat.node_tree.links
    bsdf = next(n for n in nodes if n.type == 'BSDF_PRINCIPLED')
    out = next(n for n in nodes if n.type == 'OUTPUT_MATERIAL')
    bsdf.inputs['Roughness'].default_value = rough
    if mottle:
        noise = nodes.new('ShaderNodeTexNoise')
        noise.inputs['Scale'].default_value = mottle_scale
        noise.inputs['Detail'].default_value = 6.0
        mix = nodes.new('ShaderNodeMix')
        mix.data_type = 'RGBA'
        mix.inputs['A'].default_value = lt.hex_color(color)
        mix.inputs['B'].default_value = [c * (1.0 - mottle) for c in lt.hex_color(color)[:3]] + [1.0]
        links.new(noise.outputs['Fac'], mix.inputs['Factor'])
        links.new(mix.outputs['Result'], bsdf.inputs['Base Color'])
    else:
        bsdf.inputs['Base Color'].default_value = lt.hex_color(color)
    surface = bsdf.outputs['BSDF']
    if emit is not None:
        e = nodes.new('ShaderNodeEmission')
        e.inputs['Color'].default_value = lt.hex_color(emit[0])
        e.inputs['Strength'].default_value = emit[1]
        surface = e.outputs['Emission']
    camera = nodes.new('ShaderNodeCameraData')
    fade = nodes.new('ShaderNodeMath')
    fade.operation = 'MULTIPLY'
    links.new(camera.outputs['View Distance'], fade.inputs[0])
    fade.inputs[1].default_value = -1.0 / haze
    exp = nodes.new('ShaderNodeMath')
    exp.operation = 'EXPONENT'
    links.new(fade.outputs[0], exp.inputs[0])
    amount = nodes.new('ShaderNodeMapRange')
    links.new(exp.outputs[0], amount.inputs['Value'])
    amount.inputs['From Min'].default_value, amount.inputs['From Max'].default_value = 1.0, 0.0
    amount.inputs['To Min'].default_value, amount.inputs['To Max'].default_value = 0.0, 0.92
    geo = nodes.new('ShaderNodeNewGeometry')
    sun = nodes.new('ShaderNodeVectorMath')
    sun.operation = 'DOT_PRODUCT'
    links.new(geo.outputs['Incoming'], sun.inputs[0])
    sun.inputs[1].default_value = -toward_sun()
    glow = nodes.new('ShaderNodeMath')
    glow.operation = 'POWER'
    glow.use_clamp = True
    links.new(sun.outputs['Value'], glow.inputs[0])
    glow.inputs[1].default_value = 4.0
    tint = nodes.new('ShaderNodeMix')
    tint.data_type = 'RGBA'
    tint.inputs['A'].default_value = lt.hex_color(air[0])
    tint.inputs['B'].default_value = lt.hex_color(air[2])
    links.new(glow.outputs[0], tint.inputs['Factor'])
    fog = nodes.new('ShaderNodeEmission')
    links.new(tint.outputs['Result'], fog.inputs['Color'])
    fog.inputs['Strength'].default_value = air[1]
    mix = nodes.new('ShaderNodeMixShader')
    links.new(amount.outputs['Result'], mix.inputs['Fac'])
    links.new(surface, mix.inputs[1])
    links.new(fog.outputs['Emission'], mix.inputs[2])
    links.new(mix.outputs['Shader'], out.inputs['Surface'])
    return mat


def mesh_object(name, verts, faces, mat, collection):
    mesh = bpy.data.meshes.new(name)
    mesh.from_pydata([tuple(v) for v in verts], [], faces)
    mesh.validate()
    obj = bpy.data.objects.new(name, mesh)
    collection.objects.link(obj)
    mesh.materials.append(mat)
    return obj


def box(name, lo, hi, mat, collection):
    (x0, y0, z0), (x1, y1, z1) = lo, hi
    verts = [(x0, y0, z0), (x1, y0, z0), (x1, y1, z0), (x0, y1, z0), (x0, y0, z1), (x1, y0, z1), (x1, y1, z1),
             (x0, y1, z1)]
    faces = [(0, 3, 2, 1), (4, 5, 6, 7), (0, 1, 5, 4), (1, 2, 6, 5), (2, 3, 7, 6), (3, 0, 4, 7)]
    return mesh_object(name, verts, faces, mat, collection)


def wave(x, phases, scale=1.0):
    return sum(math.sin(x * f / scale + p) * a for (f, a), p in zip(((0.21, 1.0), (0.53, 0.5), (1.31, 0.25),
                                                                       (2.9, 0.12)), phases))


def densify(points, spacing):
    out = []
    for a, b in zip(points, points[1:]):
        a, b = Vector((a[0], a[1])), Vector((b[0], b[1]))
        n = max(1, int(round((b - a).length / spacing)))
        out += [a.lerp(b, k / n) for k in range(n)]
    out.append(Vector(points[-1]))
    return out


def mock_point(scene_col):
    """Gravewind Point's tip and the Rim: the land on top, the cliff faces down to the canyon floor, the floor, the
    far wall 300 m off, the plains and a few mesas toward the sunset."""
    rng = random.Random(77)
    lip = densify(OUTLINE, 1.6)
    # A little roughness along the lip, but none under the deck, where the struts' shoes sit on the face.
    phases = [rng.uniform(0.0, TAU) for _ in range(4)]
    rough = []
    for i, p in enumerate(lip):
        k = 0.0 if (p.x < -8.0 and abs(p.y) < 10.0) else 0.35
        rough.append(k * wave(i * 1.6, phases, 0.8))
    normals = []
    for i in range(len(lip)):
        a, b = lip[max(i - 1, 0)], lip[min(i + 1, len(lip) - 1)]
        d = (b - a).normalized()
        normals.append(Vector((-d.y, d.x)))
    lip = [p + n * r for p, n, r in zip(lip, normals, rough)]
    grass = hazed('_Grass', 0x8c7a46, mottle=0.35, mottle_scale=0.08)
    rock = hazed('_Rock', 0x86705c, mottle=0.4, mottle_scale=0.06)
    # The canyon below the Rim lies in shade at dusk: its floor dark, its air deep blue-violet even toward the sun,
    # where the far wall and the plains beyond catch the last light in the haze.
    floor = hazed('_CanyonFloor', 0x3a3640, mottle=0.4, mottle_scale=0.02, haze=260.0,
                  air=(0x403c5e, 0.65, 0x6a5466))
    far = hazed('_FarLand', 0x5e5048, mottle=0.3, mottle_scale=0.01, haze=300.0, air=(0x4a4668, 0.8, 0xd09a78))
    river = hazed('_River', 0x000000, emit=(0x8c90b8, 0.07), haze=260.0, air=(0x403c5e, 0.65, 0x6a5466))
    top = [(p.x, p.y, 0.0) for p in lip] + [(140.0, 95.0, 0.0), (140.0, -95.0, 0.0)]
    land = mesh_object('_Land', top, [tuple(reversed(range(len(top))))], grass, scene_col)
    bm = bmesh.new()
    bm.from_mesh(land.data)
    bmesh.ops.triangulate(bm, faces=bm.faces[:], quad_method='BEAUTY', ngon_method='BEAUTY')
    bm.to_mesh(land.data)
    bm.free()
    depths = (0.0, 1.0, 3.0, 6.0, 10.0, 16.0, 24.0, 34.0, 46.0, 58.0, 66.0, 70.0)
    row_phases = [[rng.uniform(0.0, TAU) for _ in range(4)] for _ in depths]
    verts, faces = [], []
    for k, d in enumerate(depths):
        for i, (p, n) in enumerate(zip(lip, normals)):
            amp = 0.0 if k == 0 else (0.25 + 0.035 * d) * (0.3 if (p.x < -8.0 and abs(p.y) < 10.0 and d < 6.0) else 1.0)
            talus = 9.0 * smoothstep(56.0, 70.0, d)
            off = amp * wave(i * 1.6, row_phases[k], 1.0) + talus
            verts.append((p.x + n.x * off, p.y + n.y * off, -d))
    width = len(lip)
    for k in range(len(depths) - 1):
        for i in range(width - 1):
            faces.append((k * width + i, k * width + i + 1, (k + 1) * width + i + 1, (k + 1) * width + i))
    mesh_object('_Cliff', verts, faces, rock, scene_col)
    mesh_object('_Floor', [(-300, -1500, FLOOR_Z), (60, -1500, FLOOR_Z), (60, 1500, FLOOR_Z), (-300, 1500, FLOOR_Z)],
                [(0, 1, 2, 3)], floor, scene_col)
    # The far wall 300 m off, in buttresses and bays, and the plains running to the sunset from its top.
    wall, wf = [], []
    ys = [-1500.0 + 12.5 * j for j in range(241)]
    for j, y in enumerate(ys):
        bay = 16.0 * math.sin(y * 0.011 + 0.6) + 7.0 * math.sin(y * 0.037 + 2.0) + 3.0 * math.sin(y * 0.11)
        lift = 4.0 * math.sin(y * 0.013 + 1.0) + 2.5 * math.sin(y * 0.041)
        wall += [(-300.0 + bay, y, FLOOR_Z), (-306.0 + bay * 1.1, y, -54.0 + 0.5 * lift),
                 (-318.0 + bay * 0.8, y, -36.0 + lift), (-6000.0, y, -36.0)]
    for j in range(len(ys) - 1):
        for c in range(3):
            wf.append((4 * j + c, 4 * j + c + 1, 4 * (j + 1) + c + 1, 4 * (j + 1) + c))
    mesh_object('_FarWall', wall, wf, far, scene_col)
    # A thin river down the canyon floor.
    rv, rf = [], []
    for j in range(121):
        y = -1500.0 + 25.0 * j
        x = -190.0 + 40.0 * math.sin(y * 0.006 + 0.8) + 12.0 * math.sin(y * 0.021)
        half = 1.6 + 0.6 * math.sin(y * 0.03)
        rv += [(x - half, y, FLOOR_Z + 0.05), (x + half, y, FLOOR_Z + 0.05)]
        if j:
            rf.append((2 * j - 2, 2 * j - 1, 2 * j + 1, 2 * j))
    mesh_object('_River', rv, rf, river, scene_col)
    mesa = hazed('_Mesa', 0x6c5a4e, haze=600.0)
    for k, (x, y, w, d, h) in enumerate(((-1800, -620, 260, 180, 90), (-2600, 260, 420, 220, 120),
                                         (-1400, 820, 160, 120, 60), (-3400, -1300, 520, 300, 150))):
        verts = [(x - w, y - d, -36), (x + w, y - d, -36), (x + w, y + d, -36), (x - w, y + d, -36),
                 (x - w * 0.7, y - d * 0.75, -36 + h), (x + w * 0.7, y - d * 0.75, -36 + h),
                 (x + w * 0.7, y + d * 0.75, -36 + h), (x - w * 0.7, y + d * 0.75, -36 + h)]
        mesh_object(f'_Mesa{k}', verts, [(0, 1, 5, 4), (1, 2, 6, 5), (2, 3, 7, 6), (3, 0, 4, 7), (4, 5, 6, 7)],
                    mesa, scene_col)
    return lip, normals


def mock_deck(scene_col):
    """A stand-in for BurialDeck: the boards, the overhang's struts to the face, side and back rails, eight biers,
    three lantern posts (lit) and a figure for scale near the open west end."""
    wood = hazed('_Boards', 0x6e5a46, mottle=0.3, mottle_scale=0.9)
    dark = hazed('_Timber', 0x4a3c30)
    lamp = hazed('_Lamp', 0x000000, emit=(0xffb050, 18.0))
    figure = hazed('_Figure', 0x2a2624)
    deck = box('_Deck', (-12.5, -9.0, 0.05), (12.5, 9.0, DECK_Z), wood, scene_col)
    # The boards in the deck's own planks (the shared WoodPlanks set), so the foreground isn't a flat slab.
    deck.data.materials.clear()
    deck.data.uv_layers.new(name='UVMap')
    lt.assign(deck, lt.material('WoodPlanks'))
    lt.box_uv(deck, 'WoodPlanks')
    lt._col_attribute(deck.data)
    for y in (-8.6, -4.3, 0.0, 4.3, 8.6):
        a, b = Vector((LIP_X - 0.1, y, -3.5)), Vector((-12.3, y, 0.05))
        mid, half = (a + b) * 0.5, (b - a).length * 0.5
        strut = box(f'_Strut{y}', (-half, -0.1, -0.1), (half, 0.1, 0.1), dark, scene_col)
        strut.location = mid
        strut.rotation_euler = (0.0, math.atan2(b.z - a.z, a.x - b.x), 0.0)
    for side in (-1.0, 1.0):
        y = 8.9 * side
        box(f'_Rail{side}', (-12.3, y - 0.05, 0.95), (12.4, y + 0.05, 1.05), dark, scene_col)
        for x in range(-12, 13, 3):
            box(f'_RailPost{side}{x}', (x - 0.06, y - 0.06, DECK_Z), (x + 0.06, y + 0.06, 1.05), dark, scene_col)
    for x in (-4.8, 1.6):
        for y in (6.3, 2.1, -2.1, -6.3):
            box(f'_Bier{x}{y}', (x - 1.1, y - 0.4, 1.22), (x + 1.1, y + 0.4, 1.3), dark, scene_col)
            for dx in (-0.9, 0.9):
                box(f'_BierLeg{x}{y}{dx}', (x + dx - 0.05, y - 0.3, DECK_Z), (x + dx + 0.05, y + 0.3, 1.22), dark,
                    scene_col)
    for x, y in ((-8.6, 7.6), (-8.6, -7.6), (9.4, 4.6)):
        box(f'_Post{x}{y}', (x - 0.09, y - 0.09, DECK_Z), (x + 0.09, y + 0.09, DECK_Z + 3.2), dark, scene_col)
        box(f'_Arm{x}{y}', (x - 0.06, y - 0.06, DECK_Z + 2.9), (x + 0.06, y + 0.7, DECK_Z + 3.0), dark, scene_col)
        box(f'_Lamp{x}{y}', (x - 0.13, y + 0.5, DECK_Z + 2.35), (x + 0.13, y + 0.76, DECK_Z + 2.7), lamp, scene_col)
    box('_FigureBody', (-10.95, 3.2, DECK_Z), (-10.45, 3.6, DECK_Z + 1.5), figure, scene_col)
    box('_FigureHead', (-10.82, 3.28, DECK_Z + 1.52), (-10.58, 3.52, DECK_Z + 1.8), figure, scene_col)


def yaw_toward(dx, dy):
    """The yaw that turns a card mesh's out direction (-Y) toward (dx, dy)."""
    return math.atan2(dx, -dy)


def place(obj, location, direction, scale, collection, tag):
    inst = obj.copy()
    inst.name = f'_{obj.name}_{tag}'
    collection.objects.link(inst)
    inst.location = location
    inst.rotation_euler = (0.0, 0.0, yaw_toward(*direction))
    inst.scale = (scale, scale, scale)
    inst.hide_render = False
    if hasattr(inst, 'visible_shadow'):
        inst.visible_shadow = False
    return inst


def place_kit(wisps, banks, lip, normals, col):
    """The suggested placement (the report's numbers): wisps every 3-4 m along the deck's open end and the point's
    lips, turned toward the sunset (the Gravewind blows west), sparser along the Rim; banks A under the deck's end,
    B farther out where Abel drifts, C wrapping the south flank."""
    rng = random.Random(91)
    placed = []
    keys = 'ACBDCABDACDB'
    k = 0
    for y in (-7.2, -3.6, 0.4, 4.1, 7.5):
        j = math.radians(rng.uniform(-12.0, 12.0))
        placed.append(place(wisps[keys[k % 12]], (-12.5, y, DECK_Z), (-math.cos(j), math.sin(j)),
                            rng.uniform(0.85, 1.15), col, f'deck{k}'))
        k += 1
    west = Vector((-1.0, 0.0))
    last = None
    for p, n in zip(lip, normals):
        on_point = -9.0 < p.x < 24.0 and abs(p.y) > 10.0
        on_rim = p.x > 26.0 and abs(p.y) > 28.0
        gap = 3.6 if on_point else 9.0
        if not (on_point or on_rim) or (last is not None and (p - last).length < gap):
            continue
        last = p
        d = (n + west * (0.9 if on_point else 0.3)).normalized()
        a = math.radians(rng.uniform(-10.0, 10.0))
        d = Vector((d.x * math.cos(a) - d.y * math.sin(a), d.x * math.sin(a) + d.y * math.cos(a)))
        placed.append(place(wisps[keys[k % 12]], (p.x, p.y, 0.0), d, rng.uniform(0.85, 1.2), col,
                            f'lip{k}'))
        k += 1
    by = {o.name[-1]: o for o in banks}
    placed.append(place(by['A'], (-12.5 - 3.0 - 10.6, 0.0, -12.0), (-1.0, 0.0), 1.0, col, 'under'))
    placed.append(place(by['B'], (-12.5 - 18.0 - 14.3, 6.0, -7.0), (-0.99, -0.14), 1.0, col, 'out'))
    placed.append(place(by['C'], (3.0, -15.0 - 11.5, -13.0), (-0.4, -0.92), 1.0, col, 'south'))
    return placed


def camera(name, location, target, lens):
    cam = bpy.data.objects.new(name, bpy.data.cameras.new(name))
    bpy.context.scene.collection.objects.link(cam)
    cam.location = location
    cam.rotation_euler = (Vector(target) - Vector(location)).to_track_quat('-Z', 'Y').to_euler()
    cam.data.lens = lens
    cam.data.clip_start, cam.data.clip_end = 0.1, 8000.0
    return cam


def render(cam, name, resolution=(1600, 900), samples=64):
    scene = bpy.context.scene
    scene.camera = cam
    scene.render.resolution_x, scene.render.resolution_y = resolution
    scene.eevee.taa_render_samples = samples
    scene.render.filepath = os.path.join(OUT, name + '.png')
    bpy.ops.render.render(write_still=True)
    lt._log(f'preview: {scene.render.filepath}')
    return scene.render.filepath


def world_triangles(objs, with_alpha):
    verts, alphas = [], []
    deps = bpy.context.evaluated_depsgraph_get()
    for obj in objs:
        ev = obj.evaluated_get(deps)
        mesh = ev.to_mesh()
        mesh.calc_loop_triangles()
        m = obj.matrix_world
        a = lt._read_col(mesh)[:, 3] if with_alpha else None
        for tri in mesh.loop_triangles:
            verts += [m @ mesh.vertices[v].co for v in tri.vertices]
            if with_alpha:
                alphas.append([a[l] for l in tri.loops])
        ev.to_mesh_clear()
    bvh = BVHTree.FromPolygons(verts, [(3 * i, 3 * i + 1, 3 * i + 2) for i in range(len(verts) // 3)])
    return bvh, verts, alphas


def overdraw(cam, cards, occluders, name, size=(320, 180)):
    """Counts the translucent layers every pixel of the deck camera pays for (every card surface in front of the
    first opaque one, at any alpha), and those that show (alpha over 0.05). Writes a heat map and logs the shares."""
    from mathutils.geometry import barycentric_transform
    import numpy as np
    scene = bpy.context.scene
    scene.render.resolution_x, scene.render.resolution_y = size
    cbvh, cverts, calpha = world_triangles(cards, True)
    obvh, _, _ = world_triangles(occluders, False)
    frame = cam.data.view_frame(scene=scene)   # top right, bottom right, bottom left, top left
    tr, br, bl, tl = frame
    rot = cam.matrix_world.to_3x3()
    origin = cam.matrix_world.translation
    w, h = size
    paid = np.zeros((h, w), np.int32)
    shown = np.zeros((h, w), np.int32)
    one, two, three = Vector((1, 0, 0)), Vector((0, 1, 0)), Vector((0, 0, 1))
    for py in range(h):
        for px in range(w):
            d = (rot @ (tl + (tr - tl) * ((px + 0.5) / w) + (bl - tl) * ((py + 0.5) / h))).normalized()
            hit = obvh.ray_cast(origin, d, 8000.0)
            limit = hit[3] if hit[0] is not None else 8000.0
            o, travelled = origin.copy(), 0.0
            while travelled < limit:
                loc, _, index, dist = cbvh.ray_cast(o, d, limit - travelled)
                if loc is None:
                    break
                paid[py, px] += 1
                t = cverts[3 * index:3 * index + 3]
                bary = barycentric_transform(loc, t[0], t[1], t[2], one, two, three)
                if sum(a * b for a, b in zip(bary, calpha[index])) > 0.05:
                    shown[py, px] += 1
                travelled += dist + 1e-3
                o = loc + d * 1e-3
    total = float(w * h)
    lt._log(f'overdraw from {name}: cards cover {np.mean(paid > 0):.0%} of the screen; layers paid per pixel '
            f'{paid.sum() / total:.2f} on average, worst {paid.max()}; 2+ layers on {np.mean(paid >= 2):.0%}, 3+ on '
            f'{np.mean(paid >= 3):.0%}, 4+ on {np.mean(paid >= 4):.0%}. Showing (alpha > 0.05): '
            f'{shown.sum() / total:.2f} per pixel, worst {shown.max()}, 3+ on {np.mean(shown >= 3):.0%}')
    colors = np.array([[12, 12, 16], [40, 60, 150], [40, 150, 150], [230, 210, 60], [240, 130, 40], [220, 40, 40]],
                      np.uint8)
    img = colors[np.minimum(paid, 5)]
    img = np.repeat(np.repeat(img, 3, axis=0), 3, axis=1)
    path = os.path.join(OUT, name + '_overdraw.png')
    lt.write_png(path, img)
    lt._log(f'preview: {path} (layers paid: black 0, blue 1, teal 2, yellow 3, orange 4, red 5+)')


def composite(paths, out, columns=2):
    import numpy as np
    tiles = []
    for p in paths:
        img = bpy.data.images.load(p)
        w, h = img.size
        px = np.empty(w * h * 4, np.float32)
        img.pixels.foreach_get(px)
        tiles.append(np.flipud(px.reshape(h, w, 4))[:, :, :3])
        bpy.data.images.remove(img)
    rows = [np.concatenate(tiles[i:i + columns], axis=1) for i in range(0, len(tiles), columns)]
    sheet = np.concatenate(rows, axis=0)
    lt.write_png(out, (np.clip(sheet, 0.0, 1.0) * 255.0 + 0.5).astype(np.uint8))
    lt._log(f'preview: {out}')


def stage():
    scene = bpy.context.scene
    scene.render.engine = 'BLENDER_EEVEE_NEXT'
    ee = scene.eevee
    ee.use_shadows = True
    if hasattr(ee, 'shadow_pool_size'):
        ee.shadow_pool_size = '1024'
    if hasattr(ee, 'use_raytracing'):
        ee.use_raytracing = False
    scene.render.film_transparent = False
    scene.render.image_settings.file_format = 'PNG'
    scene.render.image_settings.color_mode = 'RGB'
    scene.view_settings.view_transform = 'AgX'
    scene.view_settings.look = 'AgX - Medium High Contrast'
    scene.render.resolution_percentage = 100
    sun = bpy.data.objects.new('_Sun', bpy.data.lights.new('_Sun', 'SUN'))
    scene.collection.objects.link(sun)
    sun.data.energy, sun.data.color, sun.data.angle = 4.5, (1.0, 0.58, 0.32), math.radians(1.0)
    sun.rotation_euler = (-toward_sun()).to_track_quat('-Z', 'Y').to_euler()
    # The sky's light on what the low sun only grazes (the boards, the land): soft, cool, from high in the east.
    fill = bpy.data.objects.new('_SkyFill', bpy.data.lights.new('_SkyFill', 'SUN'))
    scene.collection.objects.link(fill)
    fill.data.energy, fill.data.color, fill.data.use_shadow = 0.9, (0.62, 0.62, 0.9), False
    fill.rotation_euler = (-Vector((0.35, 0.1, 0.93)).normalized()).to_track_quat('-Z', 'Y').to_euler()
    return scene


def preview(wisps, banks):
    os.makedirs(OUT, exist_ok=True)
    scene = stage()
    world = sky_world()
    scene.world = world
    scene_col = bpy.data.collections.new('_Scene')
    scene.collection.children.link(scene_col)
    kit_col = bpy.data.collections.new('_Placed')
    scene.collection.children.link(kit_col)
    for obj in wisps + banks:
        obj.hide_render = True
    lip, normals = mock_point(scene_col)
    mock_deck(scene_col)
    placed = place_kit({o.name[-1]: o for o in wisps}, banks, lip, normals, kit_col)
    views = {
        # Three metres back from the open end, looking west and down past it: fog A welling up under the end, B out
        # where Abel drifts, the wisps spilling off the boards, the sun ahead.
        'Dusk_DeckWest': camera('_CamDeck', (-9.5, 2.0, DECK_Z + 1.7), (-36.0, -3.0, -8.5), 20.0),
        # At the open end, looking down into the canyon: the fog welling up under the deck.
        'Dusk_DeckEdge': camera('_CamEdge', (-11.9, -1.5, DECK_Z + 1.7), (-28.0, 3.5, -15.0), 20.0),
        # From the north, out over the canyon at deck height: the strata from the side under the deck's end.
        'Dusk_Side': camera('_CamSide', (-40.0, 58.0, 2.0), (-20.0, 0.0, -8.0), 28.0),
        # The deck's south-west corner, looking north along the open end: the wisps spill and curl from the side.
        'Dusk_Lip': camera('_CamLip', (-11.2, -8.3, DECK_Z + 1.6), (-13.8, 4.0, -1.6), 24.0),
    }
    quick = 'quick' in (os.environ.get('GRAVEWIND_PREVIEW') or '')
    for name, cam in views.items():
        render(cam, name, samples=24 if quick else 96)
    occluders = [o for o in scene_col.objects if o.type == 'MESH']
    overdraw(views['Dusk_DeckWest'], placed, occluders, 'Dusk_DeckWest', size=(160, 90) if quick else (320, 180))
    overview(wisps, banks, scene_col, kit_col, quick)


def fit_camera(name, objs, direction, lens=35.0, aspect=16.0 / 9.0, fit=0.92):
    """A camera looking along -direction that frames objs' bounding sphere across the frame's width."""
    pts = [o.matrix_world @ Vector(c) for o in objs for c in o.bound_box]
    center = sum(pts, Vector()) / len(pts)
    d = Vector(direction).normalized()
    right = d.cross(UP).normalized()
    up = right.cross(d).normalized()
    half_w = max(abs((p - center).dot(right)) for p in pts)
    half_h = max(abs((p - center).dot(up)) for p in pts)
    tan_w = 18.0 / lens
    distance = max(half_w / tan_w, half_h * aspect / tan_w) * fit + max(abs((p - center).dot(d)) for p in pts)
    return camera(name, center + d * distance, center, lens)


def overview(wisps, banks, scene_col, kit_col, quick):
    """The kit one tile each over a plain dark ground: wisps A-D on a lip block from three-quarters (top row), banks
    A-C as seen from the deck's side and bank A from the side (bottom row). Gravewind_overview.png is shaded with the
    stand-in material; Gravewind/Cards.png draws every card edge over it."""
    scene = bpy.context.scene
    scene_col.hide_render = True
    kit_col.hide_render = True
    col = bpy.data.collections.new('_Overview')
    scene.collection.children.link(col)
    plain = bpy.data.worlds.new('_Plain')
    plain.use_nodes = True
    bg = plain.node_tree.nodes['Background']
    bg.inputs['Color'].default_value = lt.hex_color(0x1b1d2a)
    bg.inputs['Strength'].default_value = 1.0
    scene.world = plain
    ledge = hazed('_Ledge', 0x000000, emit=(0x3c3734, 1.0), haze=1e6)
    wire = bpy.data.materials.new('_Wire')
    wire.use_nodes = True
    nodes = wire.node_tree.nodes
    nodes.clear()
    e = nodes.new('ShaderNodeEmission')
    e.inputs['Color'].default_value = lt.hex_color(0xd8f0ff)
    e.inputs['Strength'].default_value = 0.5
    o = nodes.new('ShaderNodeOutputMaterial')
    wire.node_tree.links.new(e.outputs['Emission'], o.inputs['Surface'])
    tiles = []
    bpy.context.view_layer.update()
    for i, obj in enumerate(wisps + banks + [banks[0]]):
        x = 200.0 * i
        inst = place(obj, (x, 0.0, 0.0), (0.0, -1.0), 1.0, col, f'ov{i}')
        frame = inst.copy()
        frame.name = inst.name + '_wire'
        col.objects.link(frame)
        frame.data = inst.data.copy()
        frame.data.materials.clear()
        frame.data.materials.append(wire)
        mod = frame.modifiers.new('wire', 'WIREFRAME')
        mod.thickness = 0.012 if obj in wisps else 0.06
        mod.use_replace = True
        extra = []
        if obj in wisps:
            k = obj.dimensions.y / 7.0
            extra.append(box(f'_Ledge{i}', (x - 2.6 * k, 0.0, -3.8 * k), (x + 2.6 * k, 3.4 * k, 0.0), ledge, col))
            direction = (-1.0, -0.75, 0.55)
        elif i == len(wisps) + len(banks):
            direction = (-1.0, 0.12, 0.1)          # bank A from the side: the strata
        else:
            direction = (0.35, 1.0, 0.8)           # from the deck's side, above and behind the bank
        bpy.context.view_layer.update()
        cam = fit_camera(f'_CamOv{i}', [inst], direction)
        tiles.append((inst, frame, extra, cam))
    shaded, cards = [], []
    size = (640, 360)
    for i, (inst, frame, extra, cam) in enumerate(tiles):
        for o in col.objects:
            o.hide_render = True
        inst.hide_render = False
        for o in extra:
            o.hide_render = False
        shaded.append(render(cam, f'_ov{i}_shaded', size, 24 if quick else 64))
        frame.hide_render = False
        cards.append(render(cam, f'_ov{i}_cards', size, 16 if quick else 32))
    composite(shaded, os.path.join(lt.PREVIEW_DIR, 'RansomsRest', 'Gravewind_overview.png'), columns=4)
    composite(cards, os.path.join(OUT, 'Cards.png'), columns=4)
    for p in shaded + cards:
        os.remove(p)


wisp_mat, fog_mat = card_material(WISP_MAT), card_material(FOG_MAT)
wisps = [wisp(k, wisp_mat) for k in sorted(WISPS)]
banks = [fog_bank(k, fog_mat) for k in sorted(BANKS)]
for obj in wisps + banks:
    report(obj)
if os.environ.get('GRAVEWIND_LAYERS') == '1' or lt.want_preview():
    report_layers(wisps + banks)
if lt.want_preview():
    preview(wisps, banks)

