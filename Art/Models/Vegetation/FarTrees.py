"""The far ridge trees of Ransom's Rest (Docs/Areas/RansomsRest.md: Art needs, "Far trees and rocks"; Performance
plan, "No masked foliage past the boundary"): three opaque, mid-detail trees for the near slopes of the ridges past the
playable boundary, seen only from inside the valley, from about 150 m to over 1 km, and through the sights' zoom (8x
makes a tree 400 m out look 50 m away). The main session places them as one HISM per mesh across the whole ring,
tagged Beyond, with no collision and no shadows. Their partners on the ridges are the far rocks (FarRock_A..D in
Art/Models/Rocks/Outcrops.py).

  FarPine_A      a tall, narrow spire, about 13 m: seven uneven tiers of drooping boughs, some snapped short, on a
                 trunk that runs up through the crown, and a leader bending a little off true with a small whorl
  FarPine_B      a broader, older pine, about 12.5 m (13.6 to its dead leader's top): a bare lower trunk, seven ragged
                 tiers with gaps, leaning a little, its top broken off flat with a short, thick dead snag standing out
                 of it, dark as weathered wood in shade
  FarBroadleaf   a cottonwood, about 11 m: a broken crown of seven separate clumps at different heights on a trunk and
                 limbs that show between them

A conifer's tier is a ring of boughs: each a drooping tip, with a ridge halfway out that arches above the line from the
trunk to the tip, and notches pulled in between them, so the tier's outline is ragged from every side and its top
rounds over instead of lying flat. A snapped bough leaves a stub, its notches falling in: a gap in the tier. Tiers stand
apart enough that sky or slope shows between them at the edges. Each tier is closed: a top from its rim up to the trunk
and an underside from its own copy of the rim up to the trunk a little higher, shaded down and out, so from below (from
the valley; the ridges stand higher) its boughs read dark instead of catching the sky's sheen. The cottonwood's clumps
are once-subdivided octahedra (32 faces: rounder than icosahedra), jittered. The foliage is smooth-shaded with
crown-wide shading normals (mostly out of the whole crown, a quarter out of the part's own middle, as the in-boundary
trees' cards are, looter_plants.crown_shade): a tree lights as one soft volume, never as flat planes.

One material slot for all three, FarTrees (MI_FarTrees: the opaque master M_World on the FoliagePalette texture set, at
M_World's defaults): every part sits on one point of a FoliagePalette swatch (the pines on CloverDark, lower tiers
further down its dark-to-light gradient, their trunks on CattailBrown and the dead snag on a darker point of it; the
cottonwood in late summer's dry colors, like the valley's tinted oaks and birches: olive-drab clumps on GrassOlive, the
top on GrassDry, one small low clump turning on GrassYellow's warm end, the bark on CattailBrown's dark grey-brown end).
Each part's UVs spread only a hair round that point (a planar projection a few hundredths of the swatch):
enough UV area for MikkTSpace tangents, and so little that the sampled mip stays sharp at any distance and never bleeds
into the neighboring swatches. Shading within a part comes from the baked vertex AO (vertex color alpha, as every model
has; RGB carries looter_plants' foliage convention, which M_World ignores), mottled a little per bough.

Per model: Nanite 0, no LODs (three draws in all: one per mesh; the manifest's empty list says so), Collision None. The
pivot is the trunk's foot, +Z up; trunks reach 0.6 m below it to sit on slopes. Seeded: every run builds the same
meshes.

    Tools\\artrun.ps1 -Script Art\\Models\\Vegetation\\FarTrees.py [-Preview [-ScriptArgs shapes,apparent,boundary,ridge]]

-Preview renders Saved/ArtPreviews/RansomsRest/FarTrees/ (see previews()): the three side by side, from below and
beside the in-boundary trees for scale; the 8x sight's view at 400 m beside the in-boundary trees' LOD2; the
in-boundary pines and oak beside their far versions at 150 m; a ridge's face of instances at 300 m and 800 m, in the
golden afternoon and at dusk.
"""
import math
import os
import random
import sys

import bpy
import numpy as np
from mathutils import Euler, Vector

import looter_plants as lp
import looter_textures as lt
from looter_plants import UP, lerp, smoothstep

MAT = lt.material('FoliagePalette', name='FarTrees', master='World')
ROWS = len(lt.PALETTE)
# The planar projection that spreads each part's UVs a hair round its swatch point: two oblique directions, so no
# face of a jittered part lies exactly along the projection (no face without UV area).
E1 = Vector((0.8, 0.5, 0.33)).normalized()
E2 = (Vector((-0.4, 0.6, 0.7)) - E1 * Vector((-0.4, 0.6, 0.7)).dot(E1)).normalized()
SPREAD_U = 0.02        # of the whole texture, across a part
SPREAD_V = 0.1         # of one swatch's height, across a part
GOLDEN_ANGLE = math.pi * (3.0 - math.sqrt(5.0))
BARK = ('CattailBrown', 0.9)
# The dead snag: weathered wood, darker than the living bark, so it reads as a dead top in shade (a pale one stood out
# as a light stick on every old pine across a ridge).
DEAD_WOOD = ('CattailBrown', 0.2)
# The cottonwood wears Ransom's Rest's dry late summer, as the valley's scattered oaks and birches do
# (MI_LeavesOak_Ransom, MI_LeavesBirch_Ransom: olive-drab, a third turning gold); on GrassDeep it stood past the
# boundary as rows of saturated green lollipops. Its clumps sit on olive-drab points (GrassOlive, near the oaks' tinted
# leaves; the top a little drier), and one small low clump is turning: a patch, not a crown. It sits on GrassYellow's
# warm end, between the olive and FlowerYellow in lightness, which the golden light turns old gold. FlowerYellow's
# dull end, on two clumps a tree, made the far forest read bright yellow-orange, brighter than the near oaks (its
# gradient barely darkens toward that end, and no swatch lies between it and the olive). The bark is CattailBrown's
# dark end, a dark grey-brown like the near oaks' bark rather than the pines' orange-red.
LEAVES_TOP = ('GrassDry', 0.14)
LEAVES_HIGH = ('GrassOlive', 0.46)
LEAVES_LOW = ('GrassOlive', 0.38)      # a shade darker, under the high clumps
LEAVES_GOLD = ('GrassYellow', 0.72)
COTTONWOOD_BARK = ('CattailBrown', 0.15)


def swatch_uv(name, u, offset, size):
    """The UV of a point offset (from its part's center) on a part of this size, mapped round the point u of the
    swatch name (u along its dark-to-light gradient, in the middle of its height)."""
    index = lt.PALETTE.index(name)
    a = offset.dot(E1) / max(size, 1e-3)
    b = offset.dot(E2) / max(size, 1e-3)
    return (min(max(u + SPREAD_U * a, 0.02), 0.98), (index + 0.5 + SPREAD_V * max(min(b, 1.5), -1.5)) / ROWS)


class Part:
    """A part of a far tree (a tier, a clump, a trunk): its faces sit round one swatch point. size: how far its
    vertices reach from center, which scales its UVs' spread (never past the clamps, which would flatten faces)."""

    def __init__(self, plant, swatch, u, center, size):
        self.plant, self.swatch, self.u, self.center, self.size = plant, swatch, u, center, size

    def face(self, verts):
        uvs = [swatch_uv(self.swatch, self.u, v.co - self.center, self.size) for v in verts]
        self.plant.face(verts, uvs, MAT)


def ring(count, rng, turn, jitter):
    """count angles round a circle from turn, each nudged by up to jitter of a step."""
    step = 2.0 * math.pi / count
    return [turn + step * (i + rng.uniform(-jitter, jitter)) for i in range(count)]


# --- Wood and spires ---

def tube(plant, rng, points, radii, sides, swatch=BARK, occlusion=0.55, shade=None, broken=False):
    """A tapered tube through points with radii at each (a trunk, a limb, a leader, a snag), open at its start (buried
    in the ground, a trunk or a crown). Its end closes to a point where its radius is 0, else stays open (buried), or
    with broken is closed by a splintered break, its rim staggered. shade(p) gives shading normals (a leader's
    needles); wood keeps its own smooth ones. occlusion: one value or one per point."""
    count = len(points)
    tangents = [(points[min(i + 1, count - 1)] - points[max(i - 1, 0)]).normalized() for i in range(count)]
    side = lp.perpendicular(tangents[0])
    center = sum(points, Vector()) / count
    part = Part(plant, swatch[0], swatch[1], center, max((p - center).length for p in points) + max(radii) + 0.15)
    turn = rng.uniform(0.0, 2.0 * math.pi)
    occ = list(occlusion) if isinstance(occlusion, (list, tuple)) else [occlusion] * count
    rings = []
    for i, (p, r) in enumerate(zip(points, radii)):
        t = tangents[i]
        if i:
            # Parallel transport: the ring's frame turns with the bend, so the tube never twists.
            side = tangents[i - 1].rotation_difference(t) @ side
            side = (side - t * side.dot(t)).normalized()
        other = t.cross(side)
        if r < 1e-4:
            rings.append([plant.vert(p, 0.0, shade(p) if shade else None, occ[i])])
            continue
        made = []
        for a in ring(sides, rng, turn, 0.0):
            q = p + (side * math.cos(a) + other * math.sin(a)) * r
            if broken and i == count - 1:
                q += t * rng.uniform(-0.12, 0.12)
            made.append(plant.vert(q, 0.0, shade(q) if shade else None, occ[i]))
        rings.append(made)
    for i in range(count - 1):
        low, high = rings[i], rings[i + 1]
        for s in range(sides):
            s1 = (s + 1) % sides
            if len(high) == 1:
                part.face((low[s], low[s1], high[0]))
            else:
                part.face((low[s], low[s1], high[s1], high[s]))
    if broken and len(rings[-1]) > 1:
        top = rings[-1]
        middle = plant.vert(points[-1] - tangents[-1] * 0.06, 0.0, None, occ[-1])
        for s in range(sides):
            part.face((top[s], top[(s + 1) % sides], middle))


# --- Conifers: tiers of drooping boughs ---

def tier(plant, part, rng, center, apex_p, under_p, boughs, reach, droop, notch, ragged, shade, occlusion, turn,
         broken=0.0, bulge=0.2):
    """One tier round center: boughs boughs, each a drooping tip about reach meters out (ragged: how much each
    varies; broken: the chance one is snapped to a stub, its notches falling in toward the trunk: a gap), a ridge
    halfway out raised bulge of the drop from apex_p to the tip above the straight line (so it arches over and droops),
    and notches between them pulled in to notch of the reach. The top runs from apex_p over the ridges down to the rim;
    the underside from its own copy of the rim, shaded down and out, up to under_p. Six faces a bough."""
    step = 2.0 * math.pi / boughs
    angles, reaches, snapped = [], [], []
    for j in range(boughs):
        angles.append(turn + step * (j + rng.uniform(-0.15, 0.15)))
        r = reach * (1.0 + ragged * rng.uniform(-1.0, 1.0))
        snap = broken > 0.0 and rng.random() < broken
        reaches.append(r * rng.uniform(0.3, 0.45) if snap else r)
        snapped.append(snap)
    tips, ridges, dips, mottles = [], [], [], []
    for j in range(boughs):
        a, r = angles[j], reaches[j]
        out = Vector((math.cos(a), math.sin(a), 0.0))
        tip = center + out * r - UP * droop * r * rng.uniform(0.8, 1.1)
        ridge = center + out * r * 0.5
        ridge.z = (apex_p.z + tip.z) * 0.5 + (apex_p.z - tip.z) * bulge
        b = a + step * rng.uniform(0.42, 0.58)
        n = reach * notch * rng.uniform(0.85, 1.12) * (0.6 if snapped[j] or snapped[(j + 1) % boughs] else 1.0)
        tips.append(tip)
        ridges.append(ridge)
        dips.append(center + Vector((math.cos(b) * n, math.sin(b) * n, -droop * n * 0.3)))
        # Each bough a little lighter or darker than the next, as clumps of needles catch the light unevenly.
        mottles.append(rng.uniform(0.8, 1.0))

    def under_vert(p, mottle):
        # The underside's rim is its own: shaded down and out, so from below it reads as the boughs' dark underside,
        # never catching the sky's sheen at a grazing look the way the tops' normals would.
        out = Vector((p.x - center.x, p.y - center.y, 0.0))
        out = out.normalized() if out.length > 1e-6 else Vector((1.0, 0.0, 0.0))
        return plant.vert(p, 0.6, (out * 0.5 - UP).normalized(), occlusion(p) * mottle * 0.75)

    # And within a bough each point a little different again: needle masses break up into light and dark.
    dip_mottles = [(mottles[j] + mottles[(j + 1) % boughs]) * 0.5 * rng.uniform(0.85, 1.0) for j in range(boughs)]
    tip_mottles = [m * rng.uniform(0.85, 1.0) for m in mottles]
    ridge_mottles = [m * rng.uniform(0.85, 1.0) for m in mottles]
    top_tips = [plant.vert(p, 0.6, shade(p), occlusion(p) * m) for p, m in zip(tips, tip_mottles)]
    top_ridges = [plant.vert(p, 0.5, shade(p), occlusion(p) * m) for p, m in zip(ridges, ridge_mottles)]
    top_dips = [plant.vert(p, 0.6, shade(p), occlusion(p) * m) for p, m in zip(dips, dip_mottles)]
    under_tips = [under_vert(p, m) for p, m in zip(tips, tip_mottles)]
    under_dips = [under_vert(p, m) for p, m in zip(dips, dip_mottles)]
    apex = plant.vert(apex_p, 0.4, shade(apex_p), occlusion(apex_p))
    under = plant.vert(under_p, 0.3, (shade(under_p) * 0.3 - UP).normalized(), occlusion(under_p) * 0.6)
    for j in range(boughs):
        k, i = (j + 1) % boughs, (j - 1) % boughs
        part.face((apex, top_ridges[j], top_ridges[k]))
        part.face((top_ridges[j], top_dips[i], top_tips[j]))
        part.face((top_ridges[j], top_tips[j], top_dips[j]))
        part.face((top_ridges[j], top_dips[j], top_ridges[k]))
        part.face((under, under_dips[j], under_tips[j]))
        part.face((under, under_tips[j], under_dips[i]))


def conifer(name, seed, height, crown_base, reach, boughs, trunk_r, trunk_sides, trunk_top, lean=(0.0, 0.0),
            spacing=1.3, overlap=1.5, under=0.4, profile=1.0, droop=0.4, notch=0.6, ragged=0.18, broken=0.15,
            bulge=0.2, top_len=2.2, leader=True, bend=(0.3, 0.15), u_range=(0.3, 0.6), offsets=None, snag=0.0,
            local=0.25, uneven=0.25, wobble=0.12):
    """A conifer: tiers (boughs: the boughs of each, bottom first) from crown_base up, then a leader top_len meters
    tall, bending bend (meters, x and y) off true at its tip, with a small whorl; or, with leader False, the last tier
    is the top, top_len above its rim (low: a broken, flattened top), with a dead snag standing snag meters over it.
    Tier rims stand closer toward the top (spacing > 1) and stray uneven of a gap from even; each tier's top reaches
    overlap times the gap to the next rim, its underside under of it; their reach follows a cone to the power profile
    (under 1 rounds the crown out). u_range: the swatch point of the bottom tier and the top one (darker below).
    offsets: per tier, a shift off the axis (meters). local: how much of a vertex's shading normal comes from its own
    tier rather than the whole crown. The trunk runs up through the crown in three bends (wobble meters)."""
    rng = random.Random(seed)
    plant = lp.Plant(name, seed)

    def axis(z):
        f = max(z, 0.0) / height
        return Vector((lean[0] * f ** 1.5, lean[1] * f ** 1.5, z))

    count = len(boughs)
    span = height - top_len - crown_base
    if leader:
        fractions = [1.0 - (1.0 - k / count) ** spacing for k in range(count)] + [1.0]
    else:
        fractions = [1.0 - (1.0 - k / (count - 1)) ** spacing for k in range(count)]
    rims = [crown_base + span * f for f in fractions]
    rims = [z + (rng.uniform(-uneven, uneven) * span / count if 0 < k < count - 1 else 0.0) for k, z in enumerate(rims)]
    crown_mid = axis(crown_base + (height - crown_base) * 0.4)

    def cone_radius(z):
        return reach * min(max((height - z) / (height - crown_base), 0.0), 1.0) ** profile

    def occlusion(p):
        a = axis(p.z)
        across = math.hypot(p.x - a.x, p.y - a.y) / max(cone_radius(p.z), 0.3)
        depth = lerp(0.45, 1.0, smoothstep(0.1, 0.9, across))
        return depth * lerp(0.75, 1.0, smoothstep(crown_base - 0.5, crown_base + (height - crown_base) * 0.4, p.z))

    def shade_for(center):
        local_center = center - UP * 0.9

        def shade(p):
            a = p - local_center
            b = p - crown_mid
            a = a.normalized() if a.length > 1e-6 else UP
            b = b.normalized() if b.length > 1e-6 else UP
            n = a * local + b * (1.0 - local)
            return n.normalized() if n.length > 1e-6 else UP
        return shade

    # The trunk, up through the crown in three bends, so it shows through the gaps between tiers.
    heights = [-0.6, trunk_top * 0.35, trunk_top * 0.7, trunk_top]
    points = [axis(z) + (Vector((rng.uniform(-1.0, 1.0), rng.uniform(-1.0, 1.0), 0.0)) * wobble if 0 < i < 3
                         else Vector()) for i, z in enumerate(heights)]
    tube(plant, rng, points, [trunk_r, trunk_r * 0.85, trunk_r * 0.65, trunk_r * 0.45], trunk_sides,
         occlusion=[0.45, 0.5, 0.55, 0.6])
    turn = rng.uniform(0.0, 2.0 * math.pi)
    for k in range(count):
        z = rims[k]
        last = k == count - 1 and not leader
        gap = top_len if last else rims[k + 1] - z
        offset = Vector(offsets[k] + (0.0,)) if offsets else Vector()
        center = axis(z) + offset
        apex = (axis(height) + offset * 0.5) if last else axis(min(z + gap * overlap, height - 0.3)) + offset * 0.5
        turn += GOLDEN_ANGLE
        reach_k = cone_radius(z) + 0.3
        under_p = axis(z + gap * under) + offset * 0.8
        size = max(reach_k * (1.0 + ragged) * (1.0 + droop), (apex - center).length, (under_p - center).length)
        part = Part(plant, 'CloverDark', lerp(u_range[0], u_range[1], k / max(count - 1, 1)), center, size)
        tier(plant, part, rng, center, apex, under_p, boughs[k], reach_k, droop, notch, ragged, shade_for(center),
             occlusion, turn, broken=broken if k < count - 1 else 0.0, bulge=bulge)
    if leader:
        # The leader: a spire of needles bending a little off true near its tip, a small whorl partway up it.
        z = rims[-1]
        base = axis(z)
        bent = Vector((bend[0], bend[1], 0.0))
        tip = axis(height) + bent
        middle = base.lerp(axis(height), 0.55) + bent * 0.3
        tube(plant, rng, [base, middle, tip], [0.34, 0.17, 0.0], 4, swatch=('CloverDark', u_range[1]),
             occlusion=[0.8, 0.95, 1.0], shade=shade_for(base))
        wz = z + top_len * 0.3
        center = base.lerp(middle, 0.55)
        center.z = wz
        size = max(1.2 * (1.0 + ragged), 0.8)
        part = Part(plant, 'CloverDark', u_range[1], center, size)
        tier(plant, part, rng, center, center + UP * 0.65, center + UP * 0.25, 3, 0.9, droop * 0.6, notch, ragged,
             shade_for(center), occlusion, turn + GOLDEN_ANGLE, bulge=bulge)
    if snag:
        # The broken top's dead snag: blunt and thick (never under 0.38 m across, so it doesn't flicker far off).
        top = axis(height) + (Vector(offsets[-1] + (0.0,)) * 0.5 if offsets else Vector())
        base = top - UP * 0.9 + Vector((0.25, -0.15, 0.0))
        end = top + Vector((-0.2, 0.3, snag))
        tube(plant, rng, [base, base.lerp(end, 0.5), end], [0.26, 0.22, 0.19], 5, swatch=DEAD_WOOD,
             occlusion=[0.5, 0.65, 0.75], broken=True)
    return finish(plant, ao_distance=1.6)


def far_pine_a():
    """The spire: a tall narrow cone of seven uneven tiers of drooping boughs, some snapped short, branched almost to
    the ground, and a leader bending off true near its tip, about 13 m."""
    return conifer('FarPine_A', 811, height=13.0, crown_base=1.5, reach=2.3, boughs=[7, 7, 6, 6, 6, 5, 4],
                   trunk_r=0.3, trunk_sides=4, trunk_top=10.5, lean=(0.15, -0.1), spacing=1.3, overlap=1.35,
                   under=0.42, profile=1.0, droop=0.42, notch=0.66, ragged=0.22, broken=0.12, bulge=0.2, top_len=2.3,
                   leader=True, bend=(0.32, 0.12), u_range=(0.42, 0.77))


def far_pine_b():
    """The old pine: a bare lower trunk under a broad crown of seven ragged tiers of level boughs with gaps, a lean,
    its top broken off flat with a short, thick dead snag standing out of it, about 12.5 m (13.6 to the snag's
    top)."""
    return conifer('FarPine_B', 823, height=12.5, crown_base=4.0, reach=2.85, boughs=[8, 7, 6, 6, 6, 5, 3],
                   trunk_r=0.36, trunk_sides=5, trunk_top=11.2, lean=(0.5, 0.3), spacing=1.0, overlap=1.25,
                   under=0.38, profile=0.75, droop=0.3, notch=0.64, ragged=0.3, broken=0.2, bulge=0.18, top_len=1.0,
                   leader=False, u_range=(0.15, 0.45),
                   offsets=[(0.0, 0.0), (0.2, -0.15), (-0.25, 0.2), (0.3, 0.25), (-0.1, 0.35), (0.25, -0.3),
                            (-0.45, -0.3)],
                   snag=1.0, uneven=0.3, wobble=0.15)


# --- The cottonwood: separate clumps on limbs ---

_OCTA = [Vector(v) for v in ((1, 0, 0), (-1, 0, 0), (0, 1, 0), (0, -1, 0), (0, 0, 1), (0, 0, -1))]
_OCTA_FACES = [(0, 2, 4), (2, 1, 4), (1, 3, 4), (3, 0, 4), (2, 0, 5), (1, 2, 5), (3, 1, 5), (0, 3, 5)]


def octasphere():
    """A once-subdivided octahedron: 18 unit directions and 32 faces wound outward (lists: the same on every run)."""
    dirs = list(_OCTA)
    middles = {}

    def middle(i, j):
        key = (min(i, j), max(i, j))
        if key not in middles:
            middles[key] = len(dirs)
            dirs.append((dirs[i] + dirs[j]).normalized())
        return middles[key]
    faces = []
    for a, b, c in _OCTA_FACES:
        if (dirs[b] - dirs[a]).cross(dirs[c] - dirs[a]).dot(dirs[a]) < 0.0:
            b, c = c, b
        ab, bc, ca = middle(a, b), middle(b, c), middle(c, a)
        faces += [(a, ab, ca), (b, bc, ab), (c, ca, bc), (ab, bc, ca)]
    return dirs, faces


CLUMP_DIRS, CLUMP_FACES = octasphere()


def clump(plant, rng, center, radii, shade_center, occlusion, swatch, u, share=0.3, jitter=0.2, puff=0.18):
    """A clump of leaves on the point u of the swatch: an octasphere turned at random, stretched to radii, its six
    first points puffed out (puff) and every point jittered (an uneven, cauliflower outline), shading mostly as the
    whole crown does (share: how much out of its own middle), its underside more and more by its own (down and out,
    like the conifers' undersides, so a high clump's belly never catches the sky's sheen), mottled point by point like
    leaf masses catching the light unevenly."""
    turn = Euler((rng.uniform(0.0, 6.28), rng.uniform(0.0, 6.28), rng.uniform(0.0, 6.28))).to_matrix()
    part = Part(plant, swatch, u, center, max(radii) * (1.0 + puff + jitter))
    made = []
    for i, d in enumerate(CLUMP_DIRS):
        q = turn @ d
        bulge = 1.0 + (puff if i < 6 else 0.0) + rng.uniform(-jitter, jitter)
        p = center + Vector((q.x * radii[0], q.y * radii[1], q.z * radii[2])) * bulge
        own = (p - center).normalized()
        mine = lerp(share, 0.8, smoothstep(0.0, 0.7, -own.z))
        n = own * mine + (p - shade_center).normalized() * (1.0 - mine)
        made.append(plant.vert(p, 0.8, n.normalized(), occlusion(p) * rng.uniform(0.7, 1.0)))
    for a, b, c in CLUMP_FACES:
        part.face((made[a], made[b], made[c]))


def far_broadleaf():
    """A cottonwood: a short trunk carrying on up through a broken crown of seven separate clumps at different heights
    (one on top, three high round it, three lower and further out), with three limbs from the fork to the high clumps
    and side limbs from them to the low ones showing in the gaps, about 11 m tall and 10.5 m across. Olive-drab, the
    smallest low clump turning old gold, on dark grey-brown bark (see LEAVES_TOP)."""
    rng = random.Random(837)
    plant = lp.Plant('FarBroadleaf', 837)
    center = Vector((0.2, 0.1, 7.4))
    occlusion = lp.crown_occlusion(center, (5.8, 5.6, 4.0), floor=0.45)
    shade_center = center - UP * 1.4
    heading = rng.uniform(0.0, 2.0 * math.pi)

    def around(turn, out, z):
        a = heading + turn + rng.uniform(-0.2, 0.2)
        return Vector((center.x + math.cos(a) * out, center.y + math.sin(a) * out, z))

    top = (around(0.0, 0.5, 9.1), 2.0, 0.8, LEAVES_TOP)
    high = [(around(turn, out, z), size, 0.75, leaves) for turn, out, z, size, leaves in
            ((0.3, 2.7, 8.0, 2.1, LEAVES_HIGH), (2.4, 2.6, 7.7, 2.2, LEAVES_HIGH), (4.4, 2.9, 8.3, 1.95, LEAVES_HIGH))]
    low = [(around(turn, out, z), size, 0.72, leaves) for turn, out, z, size, leaves in
           ((1.35, 3.3, 6.0, 1.85, LEAVES_LOW), (3.4, 3.1, 5.5, 2.0, LEAVES_LOW), (5.4, 3.4, 6.2, 1.7, LEAVES_GOLD))]
    for c, size, flat, (swatch, u) in [top] + high + low:
        clump(plant, rng, c, (size, size * rng.uniform(0.9, 1.05), size * flat), shade_center, occlusion, swatch,
              u + rng.uniform(-0.03, 0.03))
    # The trunk carries on up through the middle into the top clump; the limbs leave it at the fork.
    fork = Vector((0.12, 0.05, 3.0))
    tube(plant, rng, [Vector((0.0, 0.0, -0.6)), fork, top[0] - UP * 0.8], [0.45, 0.34, 0.16], 5,
         swatch=COTTONWOOD_BARK, occlusion=[0.5, 0.5, 0.6])
    limbs = []
    for c, size, flat, _ in high:
        out = Vector((c.x - fork.x, c.y - fork.y, 0.0)).normalized()
        bow = fork.lerp(c, 0.5) + out * 0.4 - UP * 0.2
        limbs.append((fork, bow, c))
        tube(plant, rng, [fork, bow, c], [0.26, 0.18, 0.1], 3, swatch=COTTONWOOD_BARK, occlusion=[0.5, 0.55, 0.6])
    # Side limbs: from partway up the high limb nearest each low clump, out into it.
    for c, size, flat, _ in low:
        start, bow, end = min(limbs, key=lambda limb: (Vector((limb[2].x, limb[2].y, 0.0)) -
                                                       Vector((c.x, c.y, 0.0))).length)
        tube(plant, rng, [start.lerp(bow, 0.75), c], [0.16, 0.08], 3, swatch=COTTONWOOD_BARK, occlusion=[0.55, 0.6])
    # Mostly the crown's own smooth occlusion: a bake alone darkens single vertices where clumps come close, which
    # shows their facets.
    return finish(plant, ao_distance=2.2, ao_blend=0.7)


# --- Finishing ---

def finish(plant, ao_distance, ao_blend=0.5):
    """looter_plants' finish (shading normals, vertex colors, baked AO blended with the guess by ao_blend), then this
    family's export settings: no collision, no Nanite, no LODs."""
    obj = lp.finish(plant, ao_distance=ao_distance, ao_samples=32, ao_blend=ao_blend)
    obj['Collision'] = 'None'
    obj['Nanite'] = 0
    obj['LODs'] = ''       # an empty list in the manifest: no LODs at all (the importer leaves them out)
    return obj


def describe(obj):
    """Triangles, vertices, height and width at scale 1, and the smallest UV area of a triangle (as the exporter's
    quads are cut: from their first corner), which must stay above zero for MikkTSpace."""
    mesh = obj.data
    vs = [obj.matrix_world @ v.co for v in mesh.vertices]
    lo = Vector((min(v.x for v in vs), min(v.y for v in vs), min(v.z for v in vs)))
    hi = Vector((max(v.x for v in vs), max(v.y for v in vs), max(v.z for v in vs)))
    uv = mesh.uv_layers.active.data
    least = math.inf
    for polygon in mesh.polygons:
        corners = [uv[i].uv for i in polygon.loop_indices]
        for k in range(1, len(corners) - 1):
            a, b, c = corners[0], corners[k], corners[k + 1]
            least = min(least, abs((b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x)) * 0.5)
    return dict(tris=lp.triangles(obj), verts=len(mesh.vertices), height=hi.z, bottom=lo.z,
                width=max(hi.x - lo.x, hi.y - lo.y), x=(lo.x, hi.x), y=(lo.y, hi.y), uv_area=least)


models = [far_pine_a(), far_pine_b(), far_broadleaf()]
lp.report(models)
for _m in models:
    _d = describe(_m)
    print(f"LOOTER: {_m.name}: {_d['tris']} triangles, {_d['verts']} vertices, {_d['height']:.2f} m tall (from z "
          f"{_d['bottom']:.2f}), {_d['width']:.2f} m wide (x {_d['x'][0]:.2f}..{_d['x'][1]:.2f}, y "
          f"{_d['y'][0]:.2f}..{_d['y'][1]:.2f}), smallest triangle UV area {_d['uv_area']:.2e}", flush=True)


# --- Previews (with --preview only; never during an export) ---
# The scenes put north at +Y and east at +X, the camera looking north at a ridge's sunny face, as from the valley
# toward Larkspur Ridge. They light it as the game does (layout.json's environment, Day and Dusk): the sun in the
# west-southwest, no shadows past 100 m (none here at all: the far trees cast none, and everything they stand on is
# beyond the shadow range), the exponential height fog's share at each distance, and the masters' occlusion
# (game_material). The cameras are the game's: 90 degrees across at 1920 x 1080 (or the sight's 8x zoom).

PREVIEW = 'RansomsRest/FarTrees'
LIGHTS = {
    'Golden': dict(azimuth=247.5, elevation=15.0, sun=4.6, sun_color=(1.0, 0.8, 0.6), sky=1.0,
                   ground=(0.3, 0.27, 0.22), horizon=(0.86, 0.72, 0.52), zenith=(0.2, 0.33, 0.6),
                   haze=(0.74, 0.7, 0.63), fog=0.84),
    'Dusk': dict(azimuth=252.0, elevation=4.0, sun=2.8, sun_color=(1.0, 0.6, 0.34), sky=0.6,
                 ground=(0.12, 0.1, 0.1), horizon=(0.66, 0.4, 0.3), zenith=(0.12, 0.14, 0.3),
                 haze=(0.15, 0.14, 0.19), fog=1.05),
}
SLOPE = 0x857752       # the ring macro map's mean color: a plain golden-ochre
GAME_LENS = 18.0       # 90 degrees across a 36 mm sensor: Unreal's default field of view


def toward_sun(light):
    a, e = math.radians(light['azimuth']), math.radians(light['elevation'])
    return Vector((math.sin(a) * math.cos(e), math.cos(a) * math.cos(e), math.sin(e)))


def game_material(mat, light_name, toward):
    """A copy of an lt material that shades like the game's masters on Medium: its occlusion (the texture's AO times
    the vertex alpha) darkens the base color by DiffuseAO (0.4) everywhere and, as the AO output does to the sky
    light, fully where the sun doesn't reach (lt's preview multiplies the color by the whole occlusion)."""
    name = f'_Game{light_name}_{mat.name}'
    if name in bpy.data.materials:
        return bpy.data.materials[name]
    copy = mat.copy()
    copy.name = name
    nodes, links = copy.node_tree.nodes, copy.node_tree.links
    bsdf = next(n for n in nodes if n.type == 'BSDF_PRINCIPLED')
    mix = bsdf.inputs['Base Color'].links[0].from_node
    b_input = next(s for s in mix.inputs if s.identifier == 'B_Color')
    ao = b_input.links[0].from_socket
    if bsdf.inputs['Normal'].links:
        normal = bsdf.inputs['Normal'].links[0].from_socket
    else:
        normal = nodes.new('ShaderNodeNewGeometry').outputs['Normal']

    def math_node(op, a, b, c=None, clamp=False):
        node = nodes.new('ShaderNodeMath')
        node.operation = op
        node.use_clamp = clamp
        for socket, value in zip(node.inputs, (a, b, c)):
            if value is None:
                continue
            if isinstance(value, (int, float)):
                socket.default_value = value
            else:
                links.new(value, socket)
        return node.outputs[0]
    # The material's own Specular and RoughnessOffset when it sets them, as M_World reads them (Blender's Specular IOR
    # Level has the same scale: 0.5 is 4%).
    if copy.get('Specular') is not None:
        bsdf.inputs['Specular IOR Level'].default_value = float(copy['Specular'])
    if copy.get('RoughnessOffset'):
        rough = bsdf.inputs['Roughness']
        if rough.links:
            links.new(math_node('ADD', rough.links[0].from_socket, float(copy['RoughnessOffset']), clamp=True), rough)
        else:
            rough.default_value = min(rough.default_value + float(copy['RoughnessOffset']), 1.0)
    dot = nodes.new('ShaderNodeVectorMath')
    dot.operation = 'DOT_PRODUCT'
    links.new(normal, dot.inputs[0])
    dot.inputs[1].default_value = toward
    lit = math_node('MULTIPLY', dot.outputs['Value'], 2.5, clamp=True)
    sky_share = math_node('MULTIPLY_ADD', lit, -0.7, 1.0)          # 1 in shade, 0.3 in full sun
    open_ = math_node('SUBTRACT', 1.0, ao)                         # 1 - ao
    diffuse = math_node('MULTIPLY_ADD', open_, -0.4, 1.0)          # lerp(1, ao, 0.4)
    ambient = math_node('SUBTRACT', 1.0, math_node('MULTIPLY', open_, sky_share))
    links.new(math_node('MULTIPLY', diffuse, ambient), b_input)
    return copy


def game_mesh(obj, light_name, toward):
    """A copy of a model's mesh wearing game_material copies, for instancing in a scene."""
    mesh = obj.data.copy()
    mesh.name = f'_Game{light_name}_{obj.name}'
    for i, mat in enumerate(mesh.materials):
        mesh.materials[i] = game_material(mat, light_name, toward)
    return mesh


def flat_material(name, color):
    mat = bpy.data.materials.get(name) or bpy.data.materials.new(name)
    mat.use_nodes = True
    bsdf = next(n for n in mat.node_tree.nodes if n.type == 'BSDF_PRINCIPLED')
    bsdf.inputs['Base Color'].default_value = lt.hex_color(color)
    bsdf.inputs['Roughness'].default_value = 0.95
    return mat


class Stage:
    """A preview scene: sky, sun, camera, haze and the objects added, all removed by close()."""

    def __init__(self, light_name):
        scene = bpy.context.scene
        self.light_name = light_name
        self.light = LIGHTS[light_name]
        self.toward = toward_sun(self.light)
        self.added = []
        self.meshes = []
        self.hidden = {o: o.hide_render for o in scene.objects}
        for o in scene.objects:
            o.hide_render = True
        self.world = self.sky()
        self.saved_world = scene.world
        scene.world = self.world
        sun = bpy.data.objects.new('_Sun', bpy.data.lights.new('_Sun', 'SUN'))
        sun.data.energy = self.light['sun']
        sun.data.color = self.light['sun_color']
        sun.data.angle = math.radians(0.8)
        sun.data.use_shadow = False
        sun.rotation_euler = (-self.toward).to_track_quat('-Z', 'Y').to_euler()
        self.link(sun)
        self.haze()

    def sky(self):
        """A gradient sky, the light's ground, horizon and zenith colors (as Art/Levels/area_preview.py's golden
        one), lighting the scene at the light's sky strength."""
        light = self.light
        world = bpy.data.worlds.new('_FarTreesSky')
        world.use_nodes = True
        nodes, links = world.node_tree.nodes, world.node_tree.links
        nodes.clear()
        out = nodes.new('ShaderNodeOutputWorld')
        background = nodes.new('ShaderNodeBackground')
        coords = nodes.new('ShaderNodeTexCoord')
        norm = nodes.new('ShaderNodeVectorMath')
        norm.operation = 'NORMALIZE'
        split = nodes.new('ShaderNodeSeparateXYZ')
        ramp = nodes.new('ShaderNodeValToRGB')
        links.new(coords.outputs['Generated'], norm.inputs[0])
        links.new(norm.outputs['Vector'], split.inputs['Vector'])
        links.new(split.outputs['Z'], ramp.inputs['Fac'])
        ramp.color_ramp.interpolation = 'EASE'
        e = ramp.color_ramp.elements
        e[0].position, e[0].color = 0.0, light['ground'] + (1.0,)
        e[1].position, e[1].color = 0.62, light['zenith'] + (1.0,)
        mid = e.new(0.03)
        mid.color = light['horizon'] + (1.0,)
        links.new(ramp.outputs['Color'], background.inputs['Color'])
        background.inputs['Strength'].default_value = light['sky']
        links.new(background.outputs['Background'], out.inputs['Surface'])
        world.mist_settings.start = 60.0
        world.mist_settings.depth = 1200.0
        world.mist_settings.falloff = 'LINEAR'
        return world

    def haze(self):
        """The height fog's share by distance, as the game's (density 0.012, from 60 m): about 11% at 300 m, 15% at
        400 m and 30% at 800 m (the mist pass runs from 60 m over 1200 m, so the sky gets the share at 1.26 km)."""
        scene = bpy.context.scene
        scene.view_layers[0].use_pass_mist = True
        scene.use_nodes = True
        tree = scene.node_tree
        tree.nodes.clear()
        layers = tree.nodes.new('CompositorNodeRLayers')

        def math_node(op, a, b):
            node = tree.nodes.new('CompositorNodeMath')
            node.operation = op
            for socket, value in zip(node.inputs, (a, b)):
                if isinstance(value, (int, float)):
                    socket.default_value = value
                else:
                    tree.links.new(value, socket)
            return node.outputs[0]
        share = math_node('SUBTRACT', 1.0, math_node('POWER', 2.0, math_node('MULTIPLY', layers.outputs['Mist'],
                                                                             -self.light['fog'])))
        mix = tree.nodes.new('CompositorNodeMixRGB')
        tree.links.new(share, mix.inputs[0])
        tree.links.new(layers.outputs['Image'], mix.inputs[1])
        mix.inputs[2].default_value = self.light['haze'] + (1.0,)
        comp = tree.nodes.new('CompositorNodeComposite')
        tree.links.new(mix.outputs[0], comp.inputs['Image'])

    def link(self, obj):
        bpy.context.scene.collection.objects.link(obj)
        self.added.append(obj)
        return obj

    def mesh_of(self, obj):
        mesh = game_mesh(obj, self.light_name, self.toward)
        self.meshes.append(mesh)
        return mesh

    def place(self, mesh, location, yaw=0.0, scale=(1.0, 1.0, 1.0), name='_Tree'):
        obj = bpy.data.objects.new(name, mesh)
        obj.location = location
        obj.rotation_euler = (0.0, 0.0, yaw)
        obj.scale = scale
        obj.visible_shadow = False
        return self.link(obj)

    def camera(self, eye, target, lens=GAME_LENS):
        cam = bpy.data.objects.new('_Camera', bpy.data.cameras.new('_Camera'))
        cam.data.lens = lens
        cam.data.sensor_fit = 'HORIZONTAL'
        cam.data.sensor_width = 36.0
        cam.data.clip_start = 0.5
        cam.data.clip_end = 6000.0
        cam.location = eye
        cam.rotation_euler = (Vector(target) - Vector(eye)).to_track_quat('-Z', 'Y').to_euler()
        self.link(cam)
        bpy.context.scene.camera = cam
        return cam

    def render(self, path, size=(1920, 1080), samples=32):
        scene = bpy.context.scene
        scene.render.engine = 'BLENDER_EEVEE_NEXT'
        scene.eevee.taa_render_samples = samples
        # Medium has no screen-space AO, GI or ray tracing; nothing past 100 m casts a shadow.
        for flag in ('use_gtao', 'use_fast_gi', 'use_raytracing', 'use_shadows'):
            if hasattr(scene.eevee, flag):
                setattr(scene.eevee, flag, False)
        scene.render.resolution_x, scene.render.resolution_y = size
        scene.render.resolution_percentage = 100
        scene.render.film_transparent = False
        scene.render.image_settings.file_format = 'PNG'
        scene.render.image_settings.color_mode = 'RGB'
        scene.view_settings.view_transform = 'AgX'
        scene.view_settings.look = 'AgX - Medium High Contrast'
        scene.render.filepath = path
        os.makedirs(os.path.dirname(path), exist_ok=True)
        bpy.ops.render.render(write_still=True)
        print(f'LOOTER: preview {os.path.normpath(path)}', flush=True)
        return path

    def close(self):
        scene = bpy.context.scene
        for obj in self.added:
            data = obj.data
            bpy.data.objects.remove(obj)
            if isinstance(data, bpy.types.Light):
                bpy.data.lights.remove(data)
            elif isinstance(data, bpy.types.Camera):
                bpy.data.cameras.remove(data)
        for mesh in self.meshes:
            bpy.data.meshes.remove(mesh)
        scene.world = self.saved_world
        bpy.data.worlds.remove(self.world)
        scene.use_nodes = False
        scene.view_layers[0].use_pass_mist = False
        for o, value in self.hidden.items():
            if o.name in scene.objects:
                o.hide_render = value


def heightfield(stage, name, xs, ys, height, color):
    """A ground mesh over the grid xs x ys (meters), height(x, y), in one flat color."""
    verts = [(x, y, height(x, y)) for y in ys for x in xs]
    nx = len(xs)
    faces = [(j * nx + i, j * nx + i + 1, (j + 1) * nx + i + 1, (j + 1) * nx + i)
             for j in range(len(ys) - 1) for i in range(nx - 1)]
    mesh = bpy.data.meshes.new(name)
    mesh.from_pydata(verts, [], faces)
    for polygon in mesh.polygons:
        polygon.use_smooth = True
    mesh.materials.append(flat_material('_' + name, color))
    stage.meshes.append(mesh)
    return stage.link(bpy.data.objects.new(name, mesh))


def terrace(stage, distance, rows):
    """Flat ground out to distance meters and a ridge's face rising behind (32 degrees) from 9 m past it, so trees
    standing at distance are seen against the slope, as past the boundary."""
    foot = distance + 9.0
    rise = math.tan(math.radians(32.0))
    heightfield(stage, '_Terrace', [-600.0 + 25.0 * i for i in range(49)], [-50.0 + 8.0 * j for j in range(rows)],
                lambda x, y: max(y - foot, 0.0) * rise, SLOPE)


def read_png(path):
    """A PNG's pixels, top row first, as stored (sRGB, 0..1)."""
    image = bpy.data.images.load(path, check_existing=False)
    w, h = image.size
    px = np.empty(w * h * 4, dtype=np.float32)
    image.pixels.foreach_get(px)
    bpy.data.images.remove(image)
    return px.reshape(h, w, 4)[::-1, :, :3]


def enlarge(path, out, box, factor=4):
    """A crop of a render (box: x0, y0, x1, y1 in pixels, top left origin), blown up factor times pixel for pixel, so
    what the game's view shows can be seen."""
    px = read_png(path)
    x0, y0, x1, y1 = box
    crop = px[max(y0, 0):y1, max(x0, 0):x1]
    big = np.repeat(np.repeat(crop, factor, axis=0), factor, axis=1)
    lt.write_png(out, lt.to8(big))
    print(f'LOOTER: preview {os.path.normpath(out)}', flush=True)


def screen_box(scene, cam, objs, size):
    """The pixel box (x0, y0, x1, y1; top left origin) the objects' bounds cover in the camera's view."""
    from bpy_extras.object_utils import world_to_camera_view
    xs, ys = [], []
    for obj in objs:
        for corner in obj.bound_box:
            p = world_to_camera_view(scene, cam, obj.matrix_world @ Vector(corner))
            xs.append(p.x * size[0])
            ys.append((1.0 - p.y) * size[1])
    return int(min(xs)), int(min(ys)), int(math.ceil(max(xs))), int(math.ceil(max(ys)))


def lod2(obj, share=0.12):
    """A copy of an in-boundary tree cut to share of its triangles, as the importer's default LOD2 for vegetation:
    Blender's collapse decimation standing in for Unreal's reduction, the full mesh's shading normals carried over (the
    reduction keeps them; Blender's decimation drops custom normals)."""
    copy = obj.copy()
    copy.data = obj.data.copy()
    copy.name = obj.name + '_LOD2'
    bpy.context.scene.collection.objects.link(copy)
    decimate = copy.modifiers.new('LOD2', 'DECIMATE')
    decimate.decimate_type = 'COLLAPSE'
    decimate.ratio = share
    transfer = copy.modifiers.new('Normals', 'DATA_TRANSFER')
    transfer.object = obj
    transfer.use_loop_data = True
    transfer.data_types_loops = {'CUSTOM_NORMAL'}
    transfer.loop_mapping = 'POLYINTERP_NEAREST'
    depsgraph = bpy.context.evaluated_depsgraph_get()
    mesh = bpy.data.meshes.new_from_object(copy.evaluated_get(depsgraph), preserve_all_data_layers=True,
                                           depsgraph=depsgraph)
    copy.modifiers.clear()
    old = copy.data
    copy.data = mesh
    bpy.data.meshes.remove(old)
    print(f'LOOTER: {copy.name}: {lp.triangles(copy)} triangles (of {lp.triangles(obj)})', flush=True)
    return copy


# --- The previews ---

def previews(models):
    argv = sys.argv[sys.argv.index('--') + 1:] if '--' in sys.argv else []
    wanted = [a for a in argv if not a.startswith('--')] or ['shapes', 'apparent', 'boundary', 'ridge']
    by = {m.name: m for m in models}
    others = {}
    if {'shapes', 'apparent', 'boundary'} & set(wanted):
        # The in-boundary trees they stand beside.
        sys.dont_write_bytecode = True  # no __pycache__ in the art folder
        sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
        import Pines
        import Trees
        others = {'Pine_A': Pines.pine_a(), 'Pine_B': Pines.pine_b(), 'Oak_A': Trees.oak_a()}
    if 'shapes' in wanted:
        close(models, 'FarTrees', view=(-0.35, -1.0, 0.18))
        close(models, 'FarTrees_FromBelow', view=(0.3, -1.0, -0.1))
        close([others['Pine_A'], by['FarPine_A'], others['Pine_B'], by['FarPine_B'], others['Oak_A'],
               by['FarBroadleaf']], 'FarTrees_Scale', view=(0.0, -1.0, 0.1), fit=0.56, gap=1.0)
    if 'apparent' in wanted:
        apparent(by, others)
    if 'boundary' in wanted:
        boundary(by, others)
    if 'ridge' in wanted:
        for light in ('Golden', 'Dusk'):
            for distance in (300, 800):
                ridge(by, light, distance)


def close(objs, name, view=(-0.35, -1.0, 0.18), fit=0.6, lens=40.0, gap=1.5):
    """The trees side by side up close (name.png), lit and shaded as the game does: no shadows (the far trees cast
    none, so they never shade themselves either), the masters' occlusion, the golden afternoon."""
    stage = Stage('Golden')
    try:
        heightfield(stage, '_Ground', [-400.0 + 50.0 * i for i in range(17)], [-400.0 + 50.0 * j for j in range(17)],
                    lambda x, y: 0.0, SLOPE)
        placed, x = [], 0.0
        for obj in objs:
            lo, hi = min(c[0] for c in obj.bound_box), max(c[0] for c in obj.bound_box)
            placed.append(stage.place(stage.mesh_of(obj), (x - lo, 0.0, 0.0), name='_' + obj.name))
            x += hi - lo + gap
        for o in placed:
            o.location.x -= (x - gap) * 0.5
        bpy.context.view_layer.update()
        corners = [o.matrix_world @ Vector(c) for o in placed for c in o.bound_box]
        lo = Vector((min(c.x for c in corners), min(c.y for c in corners), 0.0))
        hi = Vector((max(c.x for c in corners), max(c.y for c in corners), max(c.z for c in corners)))
        center, radius = (lo + hi) * 0.5, (hi - lo).length * 0.5
        half = math.atan(math.tan(math.atan(18.0 / lens)) * 9.0 / 16.0)
        stage.camera(center + Vector(view).normalized() * radius * fit / math.sin(half), center, lens=lens)
        stage.render(lt.preview_path(PREVIEW, name))
    finally:
        stage.close()


def apparent(by, others, distance=400.0, zoom=8.0):
    """What the sight's 8x zoom shows of trees 400 m out, as if 50 m away (Apparent_50m.png): the far trees, each
    beside its in-boundary counterpart at LOD2 (lod2()), on a terrace at a ridge's foot in the golden afternoon with
    400 m of fog."""
    stage = Stage('Golden')
    reduced = []
    try:
        terrace(stage, distance, int((distance + 350.0) / 8.0))
        reduced = [lod2(others[name]) for name in ('Pine_A', 'Pine_B', 'Oak_A')]
        row = [reduced[0], by['FarPine_A'], reduced[1], by['FarPine_B'], reduced[2], by['FarBroadleaf']]
        x = -40.0
        for obj in row:
            stage.place(stage.mesh_of(obj), (x, distance, 0.0), yaw=1.0, name='_' + obj.name)
            x += 16.0
        stage.camera(Vector((0.0, 0.0, 1.7)), Vector((0.0, distance, 6.5)), lens=GAME_LENS * zoom)
        stage.render(lt.preview_path(PREVIEW, 'Apparent_50m'))
    finally:
        stage.close()
        for obj in reduced:
            mesh = obj.data
            bpy.data.objects.remove(obj)
            bpy.data.meshes.remove(mesh)


def boundary(by, others, distance=150.0):
    """Each in-boundary tree beside its far version, all 150 m out on a terrace at a ridge's foot in the golden
    afternoon: the game's view (Boundary_150m.png), its trees blown up 4x pixel for pixel (_Pixels), a 6x zoom from
    the same spot (_Zoom), and each tree's mean color on screen, measured against the same render without the trees:
    over every pixel it changes (its look, gaps and edges included) and over its solid core. The in-boundary trees are
    their full meshes, the colors the far trees match."""
    import tempfile
    stage = Stage('Golden')
    scene = bpy.context.scene
    try:
        terrace(stage, distance, 80)
        pairs = [('Pine_A', others['Pine_A']), ('FarPine_A', by['FarPine_A']), ('Pine_B', others['Pine_B']),
                 ('FarPine_B', by['FarPine_B']), ('Oak_A', others['Oak_A']), ('FarBroadleaf', by['FarBroadleaf'])]
        trees = []
        x = -48.0
        for name, obj in pairs:
            trees.append((name, stage.place(stage.mesh_of(obj), (x, distance, 0.0), yaw=1.0, name='_' + name)))
            x += 19.0
        eye, target = Vector((0.0, 0.0, 1.7)), Vector((0.0, distance, 7.0))
        cam = stage.camera(eye, target)
        out = lt.preview_path(PREVIEW, 'Boundary_150m')
        stage.render(out)
        bpy.context.view_layer.update()
        box = screen_box(scene, cam, [o for _, o in trees], (1920, 1080))
        enlarge(out, out.replace('.png', '_Pixels.png'), (box[0] - 12, box[1] - 12, box[2] + 12, box[3] + 12))
        empty = os.path.join(tempfile.gettempdir(), 'FarTrees_boundary_empty.png')
        for _, o in trees:
            o.hide_render = True
        stage.render(empty)
        for _, o in trees:
            o.hide_render = False
        full, bare = read_png(out), read_png(empty)
        os.remove(empty)
        change = np.abs(full - bare).max(axis=2)
        for name, o in trees:
            x0, y0, x1, y1 = screen_box(scene, cam, [o], (1920, 1080))
            region, moved = full[y0:y1 + 1, x0:x1 + 1], change[y0:y1 + 1, x0:x1 + 1]
            report = []
            for label, mask in (('all', moved > 2.5 / 255.0), ('core', moved > 0.12)):
                px = region[mask]
                lin = np.where(px <= 0.04045, px / 12.92, ((px + 0.055) / 1.055) ** 2.4)
                luma = float((lin @ np.array([0.2126, 0.7152, 0.0722])).mean())
                mean = tuple(int(c) for c in np.round(px.mean(axis=0) * 255.0))
                report.append(f'{label} {int(mask.sum())} px sRGB {mean} luminance {luma:.4f}')
            print(f"LOOTER: boundary {name}: {'; '.join(report)}", flush=True)
        stage.camera(eye, target, lens=GAME_LENS * 6.0)
        stage.render(out.replace('.png', '_Zoom.png'))
    finally:
        stage.close()


def ridge_profile(foot, rise=72.0, angle=30.0):
    """A ridge's sunny face: the flat valley floor to foot (meters north of the camera), then a face rising rise
    meters at about angle degrees, rounding over at its crest, with swells across it."""
    run = rise / math.tan(math.radians(angle))

    def height(x, y):
        t = (y - foot) / run
        if t <= 0.0:
            return 0.0
        base = rise * smoothstep(0.0, 1.0, min(t, 1.0)) + max(t - 1.0, 0.0) * run * 0.08
        swell = (5.0 * math.sin(x / 61.0 + 0.4) + 2.5 * math.sin(x / 23.0 + y / 37.0)) * math.sin(min(t, 1.0) * math.pi)
        return base + swell
    return height, run


def scatter(rng, height, foot, run, half_width=260.0, spacing=6.5, tries=16000):
    """Tree spots on the ridge's face: dart throwing at least spacing apart, kept where a low-frequency mask says a
    stand grows (clumps and clearings), from just above the foot to past the crest. (x, y, z, t) with t up the face."""
    from mathutils import noise
    cell = spacing / math.sqrt(2.0)
    grid = {}
    spots = []
    for _ in range(tries):
        x = rng.uniform(-half_width, half_width)
        t = rng.uniform(0.06, 1.12)
        y = foot + t * run
        density = noise.noise(Vector((x / 90.0, y / 70.0, 3.7))) * 0.5 + 0.5
        if rng.random() > smoothstep(0.3, 0.55, density):
            continue
        gx, gy = int(math.floor(x / cell)), int(math.floor(y / cell))
        if any((x - p[0]) ** 2 + (y - p[1]) ** 2 < spacing ** 2
               for i in range(gx - 2, gx + 3) for j in range(gy - 2, gy + 3) for p in grid.get((i, j), ())):
            continue
        spot = (x, y, height(x, y), t)
        grid.setdefault((gx, gy), []).append(spot)
        spots.append(spot)
    return spots


def ridge(by, light_name, distance):
    """A stand of far trees on a ridge's face distance meters out (its foot a little nearer), plain golden-ochre,
    seen from the valley floor: the game's view (Ridge_<distance>m_<light>.png) and a 4x zoom (_Zoom)."""
    rng = random.Random(4100 + distance)
    stage = Stage(light_name)
    try:
        foot = distance - 40.0
        height, run = ridge_profile(foot)
        xs = [-1500.0 + 15.0 * i for i in range(201)]
        ys = [-60.0 + 8.0 * j for j in range(int((foot + run + 400.0) / 8.0))]
        heightfield(stage, '_Ridge', xs, ys, height, SLOPE)
        meshes = {name: stage.mesh_of(by[name]) for name in ('FarPine_A', 'FarPine_B', 'FarBroadleaf')}
        spots = scatter(rng, height, foot, run)
        counts = {name: 0 for name in meshes}
        for x, y, z, t in spots:
            broadleaf = lerp(0.35, 0.06, smoothstep(0.0, 0.8, t))
            pick = rng.random()
            name = 'FarBroadleaf' if pick < broadleaf else ('FarPine_A' if pick < broadleaf + (1.0 - broadleaf) * 0.55
                                                            else 'FarPine_B')
            s = rng.uniform(0.8, 1.25)
            stage.place(meshes[name], (x, y, z), yaw=rng.uniform(0.0, 2.0 * math.pi),
                        scale=(s, s, s * rng.uniform(0.92, 1.1)))
            counts[name] += 1
        print(f'LOOTER: ridge {distance} m {light_name}: {len(spots)} trees {counts}', flush=True)
        eye = Vector((0.0, 0.0, 1.7))
        target = Vector((0.0, foot + run * 0.5, 34.0))
        stage.camera(eye, target)
        out = lt.preview_path(PREVIEW, f'Ridge_{distance}m_{light_name}')
        stage.render(out)
        stage.camera(eye, target, lens=GAME_LENS * 4.0)
        stage.render(out.replace('.png', '_Zoom.png'))
    finally:
        stage.close()


if lt.want_preview():
    previews(models)
