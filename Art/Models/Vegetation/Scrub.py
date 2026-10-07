"""Scrub for the dry slopes of Ransom's Rest (Docs/Areas/RansomsRest.md: the golden late-summer palette, the ridges,
the Hogback, Ransom's Point): the shrubs and dry bunchgrass that break up the big ochre ridge and bluff faces,
scattered over them by PCG in the thousands. Scripted models (Art/README.md) built with Tools/Blender/looter_plants.py;
each model's origin is the middle of its foot at ground level, and its stems and blades reach a little below it so it
sits on a slope.

  Sagebrush_A    the iconic western shrub: a dense, rounded mound about 0.9 m tall and 1.05 m across of silvery
                 grey-green leaves, four lobes, short gnarled trunks at its foot, late-summer flower stalks on top
  Sagebrush_B    a low, lopsided one, about 0.7 m tall and 1.35 m across, five lobes, with a dead side of bare twigs
  Sagebrush_C    a small, young one, about 0.55 m, three lobes
  Rabbitbrush_A  a rounded olive mound on short, slender stems, about 0.6 m tall and 0.7 m across, its top crowded with
                 yellow-gold flower clusters: the warm accent among the sage, tying it into the golden grass
  Juniper_A      a gnarled two-stemmed juniper, about 2.2 m: an irregular crown of shaggy, dark blue-green foliage clumps
                 from knee height up, on twisted limbs, and a dead silver limb out of one side (for creases and gullies)
  Juniper_B      a squat, wind-sheared one, about 1.5 m, leaning, its crown swept to one side and flat on top (for
                 ridge lines)
  DryTuft_A      straw-golden bunchgrass, a fountain of blades about 35 cm with seed stalks to about 50 cm
  DryTuft_B      a low, older tuft, about 30 cm, part flattened, grey weathered blades among the straw

How they're made. Everything is opaque geometry on the FoliagePalette texture (no alpha, next to no overdraw). A
shrub is a dense, rounded mound sitting nearly on the ground, its outline a few overlapping lobes (a lobed envelope):
shoots of three short leaves (one triangle each) laid over the envelope like shingles (see cover()), pointing up and
out along its surface and lifted only a little off it, so they cover it and soften its outline instead of spiking out
of it; under them a darker core following the envelope a step inside, the shaded foliage seen in the gaps and what the
far LODs keep of the mound. (A core at the surface under sparse leaves read as a rock; leaves radiating out of the
envelope read as agave.) Short trunks show at the foot; flower stalks and dead twigs stand a little out of the top and
side. A juniper is bark tubes (BarkOak) into foliage pads made the same way: each pad a dark, rounded core under
bunches of short scale-leaf sprays drooping down and out over it, so the crown reads as shaggy dark clumps with dark
gaps between them. Tufts are blades and seed stalks. Shading normals are crown-wide (out of the whole plant blended
with out of the nearest lobe), so a shrub lights as one soft volume, each leaf's tipped just off its own plane (see
off_plane); blades lean up, as the meadow's grass does.

Seen from 30 to 150 m, a whole shrub covers a few pixels, so the usual blade mapping (a swatch's whole gradient along
a blade and its whole height across it) would sample the palette's smallest mips, which mix every swatch. So, as the
far trees do (FarTrees.py), every part sits on one point of a swatch, its UVs spread only a hair round it in the part's
own frame (lengthwise for leaves, blades and twigs, around for tubes, an oblique projection for the closed parts):
enough UV area for tangents, and a sharp mip at any distance. Light and dark come from choosing that point (a swatch
runs dark at U 0 to light at U 1), the baked occlusion and the normals.

Materials (one slot each, the juniper two):
  FoliageSage     sage's silvery grey-green is in no swatch (they are all yellow-greens and straws): this instance of
                  the palette is tinted SAGE_TINT, and the sage sits on the near-neutral FlowerWhite swatch, which the
                  tint turns grey-green (its core on the swatch's dark end); its stems on DryTan (olive-brown under the
                  tint), its flower stalks on DryTan's light end (khaki), dead twigs on FlowerWhite's dark end (grey)
  FoliageJuniper  the same for the juniper's dark blue-green (JUNIPER_TINT, sprays and cores on FlowerWhite, the cores
                  at its dark end), its bark BarkOak
  FoliagePalette  the rabbitbrush (GrassOlive, GrassDry, FlowerYellow, FlowerCenter) and the tufts (Straw, DryTan,
                  GrassDry)

Vertex colors (looter_plants' foliage convention). R, the wind weight, runs from 0 at the ground: woody stems reach
0.25-0.3 where they meet the foliage, cores 0.25-0.42, leaves the core's weight at their bases plus 0.3 at their tips,
flower stalks, florets and seed heads to 0.9-1; the junipers are stiffer (trunks to 0.12, limbs to 0.35, pads
0.35-0.8). G, the wind phase, is shared by a stem and everything on it (a mound and its leaves, a limb and its pad, a
stalk and its seed head), so nothing drifts off its stem; each blade of a tuft has its own. A is the baked occlusion
blended with an authored guess (dark inside and under a crown, on the cores and at a blade's root).

Per model: Nanite 0, LODs (below), no collision for shrubs and tufts (Collision 'none'), one convex hull round the
lower trunks for each juniper. Seeded: every run builds the same meshes.

    Tools\\artrun.ps1 -Script Art\\Models\\Vegetation\\Scrub.py [-Preview [-ScriptArgs closeups,overview,lods,slope]]

-Preview renders Saved/ArtPreviews/RansomsRest/Scrub/ (see previews()): each model close up in the area's golden
late-afternoon light (the sun at bearing 247.5, 15 degrees up), every LOD side by side (LODs.png), a 30 x 30 m patch of
a 27 degree stand-in slope scattered with the kit beside the same slope bare, from 20 m and 120 m away (each model
drawn at the LOD Unreal would pick there), and Saved/ArtPreviews/RansomsRest/Scrub_overview.png, the kit side by side
with a person for scale.
"""
import math
import os
import random
import sys

import bmesh
import bpy
import numpy as np
from mathutils import Vector, noise

import looter_plants as lp
import looter_textures as lt
from looter_plants import UP, horizontal, lerp, perpendicular, random_unit, smoothstep

GOLDEN_ANGLE = math.pi * (3.0 - math.sqrt(5.0))

# --- Materials ---

# Tints of the palette's FlowerWhite swatch (c8c8b8 at U 0 to f4f2ea at U 1, a near-neutral grey): sage's grey-green,
# about 757b59 to 8b916d where the leaves sit, and the juniper's dark blue-green, about 475646 to 536354.
SAGE_TINT = 0x8d9475
JUNIPER_TINT = 0x55675a
SAGE = lt.material('FoliagePalette', name='FoliageSage', tint=SAGE_TINT)
JUNIPER = lt.material('FoliagePalette', name='FoliageJuniper', tint=JUNIPER_TINT)
PALETTE = lt.material('FoliagePalette')
BARK = lt.material('BarkOak')

# Swatch points: (swatch, U) or (swatch, U range picked from).
SAGE_CORE = ('FlowerWhite', 0.1)
SAGE_LEAF = ('FlowerWhite', (0.15, 0.8))
SAGE_TWIG = ('DryTan', 0.12)
SAGE_DEAD = ('FlowerWhite', 0.1)
SAGE_STALK = ('DryTan', 0.85)
JUNIPER_CORE = ('FlowerWhite', 0.1)
JUNIPER_LEAF = ('FlowerWhite', (0.15, 0.7))

# LODs (triangle percentages) and the screen sizes they switch at. Unreal's screen size is about 1.8 x the bounds'
# radius / distance at a 90 degree field of view. These are seen across a slope from 30 to 150 m, so each keeps a
# mid LOD that still holds its canopy's outline from about 35 to 90 m and only then thins to a sketch of it.
SHRUB_LODS = ('50,22,10', '0.1,0.035,0.013')        # sage (radius ~0.7 m): LOD1 from ~12 m, LOD2 ~35 m, LOD3 ~94 m
SMALL_SHRUB_LODS = ('50,25,12', '0.08,0.03,0.012')  # Sagebrush_C (~0.42 m): from ~9 m, ~25 m, ~62 m
ACCENT_LODS = ('50,25,12', '0.065,0.024,0.009')     # rabbitbrush (~0.45 m): from ~12 m, ~33 m, ~89 m, its gold kept far
TUFT_LODS = ('50,25', '0.06,0.025')                 # tufts (~0.28 m): LOD1 from ~8 m, LOD2 from ~20 m
JUNIPER_LODS = ('50,25,12', '0.2,0.08,0.03')        # junipers (~1.5 m): from ~13 m, ~33 m, ~89 m

# The UV spread round a part's swatch point, in texture widths per meter of the part's own layout: along U (a hair of
# the swatch's dark-to-light gradient) and across V (in the middle of the swatch's height, never near its edges).
K_U = 0.06
K_V = 0.025
V_SPREAD = 0.3      # at most this share of a swatch's height off its middle
ROWS = len(lt.PALETTE)
# An oblique projection for the closed canopies (two directions no face lies exactly along).
E1 = Vector((0.8, 0.5, 0.33)).normalized()
E2 = (Vector((-0.4, 0.6, 0.7)) - E1 * Vector((-0.4, 0.6, 0.7)).dot(E1)).normalized()


def octasphere(levels=1):
    """An octahedron subdivided levels times onto the sphere: 18 vertices and 32 faces once, 66 and 128 twice."""
    verts = [Vector(v) for v in ((1, 0, 0), (-1, 0, 0), (0, 1, 0), (0, -1, 0), (0, 0, 1), (0, 0, -1))]
    faces = [(0, 2, 4), (2, 1, 4), (1, 3, 4), (3, 0, 4), (2, 0, 5), (1, 2, 5), (3, 1, 5), (0, 3, 5)]
    for _ in range(levels):
        middles = {}

        def middle(a, b):
            key = (min(a, b), max(a, b))
            if key not in middles:
                middles[key] = len(verts)
                verts.append((verts[a] + verts[b]).normalized())
            return middles[key]
        split = []
        for a, b, c in faces:
            ab, bc, ca = middle(a, b), middle(b, c), middle(c, a)
            split += [(a, ab, ca), (ab, b, bc), (ca, bc, c), (ab, bc, ca)]
        faces = split
    return verts, faces


SPHERES = {0: octasphere(0), 1: octasphere(1), 2: octasphere(2)}


def pick(rng, value):
    """A number, or one picked from a (low, high) range."""
    return rng.uniform(*value) if isinstance(value, tuple) else value


def off_plane(normal, face_normal, least=0.2):
    """normal (a crown-wide shading normal) tipped just enough off a flat part's plane (face_normal) to stand at least
    least out of it, on its own side. A shading normal lying in a part's plane leaves no tangent plane to measure the
    part's corner angles in: MikkTSpace's angle rounds to zero and Unreal warns of a zero tangent."""
    s = normal.dot(face_normal)
    if abs(s) >= least:
        return normal
    return (normal + face_normal * ((least if s >= 0.0 else -least) - s)).normalized()


def pal_uv(swatch, u, a=0.0, b=0.0):
    """A UV on the swatch at u along its gradient, moved a hair by a part's own coordinates a (along U) and b (across
    V), in meters."""
    index = lt.PALETTE.index(swatch)
    # The point stays far enough inside the swatch for the spread to fit (clamping the spread would flatten a part's
    # UVs to a line: no UV area, no tangents).
    u = min(max(u, 0.1), 0.88)
    du = max(min(a * K_U, 0.08), -0.08)
    dv = max(min(b * K_V * ROWS, V_SPREAD), -V_SPREAD)
    return (u + du, (index + 0.5 + dv) / ROWS)


# --- One model's mesh, with wind phase groups ---

class Shrub(lp.Plant):
    """looter_plants' Plant, with a wind phase group per vertex: new_group() starts a stem's, and everything built
    until the next shares its phase."""

    def __init__(self, name, seed=0):
        super().__init__(name, seed)
        self.groups = []
        self.group = 0
        self.count = 1

    def new_group(self):
        self.group = self.count
        self.count += 1
        return self.group

    def vert(self, co, wind=0.0, normal=None, occlusion=1.0):
        self.groups.append(self.group)
        return super().vert(co, wind, normal, occlusion)

    def out_face(self, verts, uvs, mat, outward):
        """A face, wound so its front faces outward (the occlusion bake lets rays through faces seen from behind)."""
        a, b, c = verts[0].co, verts[1].co, verts[2].co
        if (b - a).cross(c - a).dot(outward) < 0.0:
            verts, uvs = list(reversed(verts)), list(reversed(uvs))
        return self.face(verts, uvs, mat)


def finish(plant, hull_points=None, ao_distance=0.5, ao_blend=0.5, lods=None):
    """looter_plants.finish, then the wind weights and phases written exactly: R from each vertex's own weight (finish
    looks them up by position, and coincident points could swap), G one random value per phase group."""
    obj = lp.finish(plant, hull_points, ao_distance=ao_distance, ao_samples=24, ao_blend=ao_blend,
                    lods=lods[0] if lods else None, lod_screens=lods[1] if lods else None)
    mesh = obj.data
    col = mesh.color_attributes['Col']
    values = np.empty(4 * len(mesh.loops), dtype=np.float32)
    col.data.foreach_get('color', values)
    values = values.reshape(-1, 4)
    loop_vert = np.empty(len(mesh.loops), dtype=np.int64)
    mesh.loops.foreach_get('vertex_index', loop_vert)
    rng = random.Random(plant.seed * 7 + 3)
    phases = np.array([rng.random() for _ in range(plant.count)], dtype=np.float32)
    values[:, 0] = np.clip(np.array(plant.winds, dtype=np.float32)[loop_vert], 0.0, 1.0)
    values[:, 1] = phases[np.array(plant.groups, dtype=np.int64)[loop_vert]]
    col.data.foreach_set('color', values.ravel())
    mesh.update()
    return obj


# --- Parts ---

def ptube(plant, limb, mat, swatch, u, u_tip=None, occlusion=None):
    """limb as a tube on one point of a palette swatch (a twig, a stem): U runs from u to u_tip along it, V a hair round
    it. Its base is open (buried in the ground or a mass); a zero tip radius closes it to a point, else a small cap."""
    pts, radii, winds, sides = limb.points, limb.radii, limb.winds, limb.sides
    u_tip = u if u_tip is None else u_tip
    count = len(pts)
    tangents = [(pts[min(i + 1, count - 1)] - pts[max(i - 1, 0)]).normalized() for i in range(count)]
    side = perpendicular(tangents[0])
    rings = []
    for i in range(count):
        t = tangents[i]
        if i:
            side = tangents[i - 1].rotation_difference(t) @ side
            side = (side - t * side.dot(t)).normalized()
        other = t.cross(side)
        o = occlusion(pts[i]) if occlusion else 1.0
        if radii[i] < 1e-4:
            rings.append([plant.vert(pts[i], winds[i], None, o)])
            continue
        rings.append([plant.vert(pts[i] + (side * math.cos(2.0 * math.pi * s / sides)
                                           + other * math.sin(2.0 * math.pi * s / sides)) * radii[i], winds[i], None, o)
                      for s in range(sides)])
    around = 2.0 * math.pi * max(radii[0], 0.005)

    def uv(i, s):
        along = limb.params[i] * limb.length
        return pal_uv(swatch, lerp(u, u_tip, limb.params[i]), along, (s / sides - 0.5) * around)
    for i in range(count - 1):
        lower, upper = rings[i], rings[i + 1]
        for s in range(sides):
            s1 = (s + 1) % sides
            if len(upper) == 1:
                plant.face((lower[s], lower[s1], upper[0]), [uv(i, s), uv(i, s + 1), uv(i + 1, s + 0.5)], mat)
            else:
                plant.face((lower[s], lower[s1], upper[s1], upper[s]),
                           [uv(i, s), uv(i, s + 1), uv(i + 1, s + 1), uv(i + 1, s)], mat)
    if len(rings[-1]) > 1:
        tip = plant.vert(pts[-1] + tangents[-1] * radii[-1] * 0.5, winds[-1], None, 0.9)
        tip_uv = pal_uv(swatch, u_tip, limb.length + 0.05, 0.0)
        ring = rings[-1]
        for s in range(sides):
            plant.face((ring[s], ring[(s + 1) % sides], tip), [uv(count - 1, s), uv(count - 1, s + 1), tip_uv], mat)


def ribbon(plant, mat, points, widths, sides, normals, swatch, u_range, winds, occlusions):
    """A flat strip along points (a blade, a slender stem), widths[i] across along sides[i] (0: a point), each vertex
    shaded with normals[i] (None keeps the mesh's own): U runs along it over u_range, V a hair across."""
    length = 0.0
    lengths = [0.0]
    for a, b in zip(points, points[1:]):
        length += (b - a).length
        lengths.append(length)
    rows = []
    normals = list(normals)
    for i, p in enumerate(points):
        if normals[i] is not None:
            along = points[min(i + 1, len(points) - 1)] - points[max(i - 1, 0)]
            flat = sides[i].cross(along)
            if flat.length > 1e-6:
                normals[i] = off_plane(normals[i], flat.normalized())
        t = lengths[i] / max(length, 1e-6)
        u = lerp(u_range[0], u_range[1], t)
        if widths[i] > 0.0:
            half = sides[i] * widths[i] * 0.5
            rows.append([(plant.vert(p - half, winds[i], normals[i], occlusions[i]),
                          pal_uv(swatch, u, lengths[i], -widths[i] * 0.5)),
                         (plant.vert(p + half, winds[i], normals[i], occlusions[i]),
                          pal_uv(swatch, u, lengths[i], widths[i] * 0.5))])
        else:
            rows.append([(plant.vert(p, winds[i], normals[i], occlusions[i]), pal_uv(swatch, u, lengths[i], 0.0))])
    for lower, upper in zip(rows, rows[1:]):
        if len(upper) == 1:
            corners = [lower[0], lower[1], upper[0]]
        elif len(lower) == 1:
            corners = [lower[0], upper[1], upper[0]]
        else:
            corners = [lower[0], lower[1], upper[1], upper[0]]
        plant.face([v for v, _ in corners], [uv for _, uv in corners], mat)


def blade(plant, mat, root, heading, height, width, lean, swatch, u_range, rng, segments=2, shade=None, twist=0.3,
          wind=(0.0, 1.0), occlusion=(0.6, 1.0), taper=0.8, end_width=0.0):
    """A blade (or a slender stem) from root rising height meters and bending toward heading by lean (a share of its
    height), as looter_plants.blade bends it, narrowing from width to end_width (0: a point); on one stretch of a
    swatch (u_range, root to tip). Returns its tip."""
    heading = Vector((heading.x, heading.y, 0.0)).normalized()
    side = UP.cross(heading)
    a = rng.uniform(-twist, twist)
    side = (side * math.cos(a) + heading * math.sin(a)).normalized()
    points, widths, normals, winds, occlusions = [], [], [], [], []
    for i in range(segments + 1):
        t = i / segments
        p = root + heading * (lean * height * t * t) + UP * (height * t * (1.0 - 0.35 * lean * lean * t))
        points.append(p)
        widths.append(width * (1.0 - t) ** taper if i < segments else end_width)
        normals.append(shade(p) if shade else UP)
        winds.append(lerp(wind[0], wind[1], t ** 1.5))
        occlusions.append(lerp(occlusion[0], occlusion[1], t ** 0.8))
    ribbon(plant, mat, points, widths, [side] * len(points), normals, swatch, u_range, winds, occlusions)
    return points[-1]


def shoot(plant, mat, base, up, rng, blades, length, width, spread, swatch, u, shade, occlusion, wind, droop=0.0):
    """A shoot: blades narrow leaves (one triangle each) fanned out of one point around up, each tilted up to spread
    (radians) off it and drooping by droop, facing every way: packed over a canopy they give it a fine, brushy texture
    and outline."""
    up = up.normalized()
    side = perpendicular(up)
    turn = rng.uniform(0.0, 2.0 * math.pi)
    for k in range(blades):
        a = turn + 2.0 * math.pi * (k + rng.uniform(-0.3, 0.3)) / blades
        out = side * math.cos(a) + up.cross(side) * math.sin(a)
        tilt = spread * rng.uniform(0.35, 1.0)
        direction = (up * math.cos(tilt) + out * math.sin(tilt) - UP * droop).normalized()
        facing = (out * math.cos(1.2) + up.cross(out) * math.sin(1.2) * rng.choice((-1.0, 1.0))).normalized()
        tuft_leaf(plant, mat, base, direction, facing, length * rng.uniform(0.7, 1.1), width * rng.uniform(0.8, 1.2),
                  swatch, u + rng.uniform(-0.08, 0.08), shade, occlusion, wind)


def tuft_leaf(plant, mat, base, direction, facing, length, width, swatch, u, shade, occlusion, wind):
    """A narrow leaf (or a tuft of them) as one flat triangle from base along direction, its flat side toward facing.
    On one point of a swatch, a hair lighter at its tip."""
    d = direction.normalized()
    n = facing - d * facing.dot(d)
    n = n.normalized() if n.length > 1e-4 else perpendicular(d)
    side = d.cross(n)
    corners = [(base - side * width * 0.5, 0.0, -0.5), (base + side * width * 0.5, 0.0, 0.5),
               (base + d * length, 1.0, 0.0)]
    verts = [plant.vert(p, lerp(wind[0], wind[1], along), off_plane(shade(p), n),
                        occlusion(p) * lerp(0.85, 1.0, along)) for p, along, across in corners]
    uvs = [pal_uv(swatch, u + 0.03 * along, length * along, width * across) for p, along, across in corners]
    plant.out_face(verts, uvs, mat, n)


def surface_normal(surface, d, center):
    """The outward normal of a canopy's surface (a function of direction, as lobed() returns) in direction d."""
    d = d.normalized()
    t1 = perpendicular(d)
    t2 = d.cross(t1)
    p = surface(d)
    n = (surface(d + t1 * 0.04) - p).cross(surface(d + t2 * 0.04) - p)
    if n.length < 1e-9:
        return (p - center).normalized()
    n = n.normalized()
    return n if n.dot(p - center) >= 0.0 else -n


def cover(plant, mat, rng, surface, shade, center, count, low, length, width, swatch, u_range, occlusion, wind,
          flow, lift=(0.1, 0.35), leaves=3, fan=0.9, sink=0.3, skip=None):
    """Leaves laid over a canopy's surface like shingles: count shoots spread over it (directions above low), each
    leaves short leaves fanned (fan radians) in the surface's own plane round the way flow(point, normal) points them
    (up it for sage, down it for drooping juniper sprays), lifted off it only lift radians and facing out, their bases
    sunk a share sink of their length. They cover the canopy and soften its outline, never spike out of it (a leaf's tip
    stands about length x sin(lift) proud). skip(point) leaves out spots (buried in a neighbour)."""
    for d in lp.spiral(count, low, rng.uniform(0.0, 2.0 * math.pi)):
        d = (d + random_unit(rng) * 0.2).normalized()
        base = surface(d)
        if skip and skip(base):
            continue
        n = surface_normal(surface, d, center)
        f = flow(base, n)
        f = f - n * f.dot(n)
        f = f.normalized() if f.length > 1e-3 else perpendicular(n)
        size = pick(rng, length)
        turn = rng.uniform(-0.3, 0.3)
        u = pick(rng, u_range)
        for k in range(leaves):
            a = turn + (k / max(leaves - 1, 1) - 0.5) * fan + rng.uniform(-0.15, 0.15)
            along = f * math.cos(a) + n.cross(f) * math.sin(a)
            tilt = pick(rng, lift)
            direction = along * math.cos(tilt) + n * math.sin(tilt)
            facing = (n * math.cos(tilt) - along * math.sin(tilt) + random_unit(rng) * 0.15).normalized()
            leaf_length = size * rng.uniform(0.8, 1.15)
            tuft_leaf(plant, mat, base - direction * leaf_length * sink - n * 0.01, direction, facing, leaf_length,
                      leaf_length * width * rng.uniform(0.85, 1.15), swatch, u + rng.uniform(-0.06, 0.06), shade,
                      occlusion, (wind(base), min(wind(base) + 0.3, 1.0)))


def lobed(plant, mat, center, radii, rng, swatch, u, shade_center, occlusion, wind, lobes=7, levels=2,
          lobe_width=0.35, bottom=0.8, swell=0.38, build=True):
    """A canopy: a sphere of radii (subdivided levels times) swelling into lobes, soft bumps toward random directions
    mostly up and out, so it reads as a cauliflower of foliage masses rather than one ball; its underside pulled up
    toward a floor bottom of its height below the middle. Shaded out of the crown (shade_center) blended with out of its
    nearest lobe, so each lobe rounds a little in the light. Returns surface(d), the point on it in direction d, and
    shade(p); with build False it makes no mesh (an envelope to grow shoots on)."""
    verts, faces = SPHERES[levels]
    offset = Vector((rng.uniform(0.0, 50.0), rng.uniform(0.0, 50.0), rng.uniform(0.0, 50.0)))
    bumps = []
    for k in range(lobes):
        z = rng.uniform(-0.25, 0.9)
        a = 2.0 * math.pi * (k + rng.uniform(-0.3, 0.3)) / lobes
        bumps.append((Vector((math.cos(a) * math.sqrt(1.0 - z * z), math.sin(a) * math.sqrt(1.0 - z * z), z)),
                      rng.uniform(0.7, 1.0)))
    floor_z = center.z - radii[2] * bottom

    def surface(d):
        d = d.normalized()
        bump = max(s * math.exp(-(1.0 - d.dot(b)) / lobe_width) for b, s in bumps)
        r = 0.7 + swell * bump + 0.05 * noise.noise(d * 2.5 + offset)
        p = center + Vector((d.x * radii[0], d.y * radii[1], d.z * radii[2])) * r
        if p.z < floor_z:
            p.z = lerp(p.z, floor_z, 0.85)
        return p

    def shade(p):
        crown = (p - shade_center).normalized()
        d = p - center
        d = Vector((d.x / radii[0], d.y / radii[1], d.z / radii[2])).normalized()
        lobe = max(bumps, key=lambda b: d.dot(b[0]))[0]
        return (crown * 0.65 + lobe * 0.35).normalized()

    if not build:
        return surface, shade
    made = []
    for v in verts:
        p = surface(v)
        made.append(plant.vert(p, wind(p), shade(p), occlusion(p)))
    for face in faces:
        vs = [made[i] for i in face]
        middle = (vs[0].co + vs[1].co + vs[2].co) / 3.0
        uvs = [pal_uv(swatch, u, (v.co - center).dot(E1), (v.co - center).dot(E2)) for v in vs]
        plant.out_face(vs, uvs, mat, middle - center)
    return surface, shade


def shell(plant, mat, center, surface, levels, swatch, u, shade, occlusion, wind):
    """A closed mesh through surface(d) (a point for each direction), a subdivided octahedron's directions, on one
    point of a swatch."""
    verts, faces = SPHERES[levels]
    made = []
    for v in verts:
        p = surface(v)
        made.append(plant.vert(p, wind(p), shade(p), occlusion(p)))
    for face in faces:
        vs = [made[i] for i in face]
        middle = (vs[0].co + vs[1].co + vs[2].co) / 3.0
        uvs = [pal_uv(swatch, u, (v.co - center).dot(E1), (v.co - center).dot(E2)) for v in vs]
        plant.out_face(vs, uvs, mat, middle - center)


def clump_layout(rng, center, radii, count, size, low, outliers=1, floor=0.12):
    """Foliage masses over a crown (looter_plants.layout_clumps), a few pushed out past its outline so it is never a
    smooth dome, none sunk below floor."""
    clumps = lp.layout_clumps(rng, center, radii, count, size, low=low, jitter=0.3)
    order = sorted(range(len(clumps)), key=lambda i: clumps[i][0].z)
    for i in rng.sample(order[:max(len(order) * 2 // 3, 1)], min(outliers, len(order))):
        c, r = clumps[i]
        out = Vector((c.x - center.x, c.y - center.y, 0.0))
        if out.length > 1e-3:
            clumps[i] = (c + out.normalized() * r * rng.uniform(0.3, 0.5), r * rng.uniform(0.75, 0.9))
    return [(Vector((c.x, c.y, max(c.z, floor + r * 0.6))), r) for c, r in clumps]


# --- Sagebrush ---

def spike(plant, mat, root, heading, height, tilt, swatch, u, shade, wind=(0.4, 1.0)):
    """A flower stalk: a thin stem bending a little toward heading, its top 45% a slim panicle, widest a third of the
    way up it (five triangles)."""
    heading = Vector((heading.x, heading.y, 0.0)).normalized()
    side = UP.cross(heading).normalized()
    points, widths, normals, winds, occlusions = [], [], [], [], []
    for t, w in ((0.0, 0.006), (0.55, 0.009), (0.72, 0.024), (1.0, 0.0)):
        p = root + heading * (tilt * height * t * t) + UP * (height * t)
        points.append(p)
        widths.append(w)
        normals.append(shade(p))
        winds.append(lerp(wind[0], wind[1], t))
        occlusions.append(lerp(0.75, 1.0, t))
    ribbon(plant, mat, points, widths, [side] * 4, normals, swatch, (u - 0.05, u), winds, occlusions)


def sagebrush(name, seed, center, radii, lobes, shoots, stalks=0, dead=0, stems=3, stem_radius=(0.028, 0.042),
              leaf_size=(0.13, 0.19), lods=SHRUB_LODS):
    """A sagebrush: a dense, rounded mound sitting almost on the ground, its outline a few overlapping lobes (a lobed
    canopy of radii round center), made of shoots of short grey-green leaves laid up and out along its surface like
    shingles (shoots of three), silvery at their tips. Short, gnarled trunks show under it at the foot; late-summer
    flower stalks stand a little out of its top and dead, bare twigs out of one side."""
    rng = random.Random(seed)
    plant = Shrub(name, seed)
    center, radii = Vector(center), Vector(radii)
    occlusion = lp.crown_occlusion(center, radii, floor=0.5)
    shade_center = center - UP * radii.z * 0.7
    plant.new_group()

    def wind(p):
        return lerp(0.25, 0.42, smoothstep(center.z - radii.z, center.z + radii.z, p.z))
    # The trunks: short and twisted, splaying from the foot into the core's underside, so only their feet show.
    for k in range(stems):
        a = 2.0 * math.pi * (k + rng.uniform(-0.3, 0.3)) / stems
        start = horizontal(a, rng.uniform(0.0, 0.04)) - UP * 0.16
        end = center + horizontal(a, rng.uniform(0.25, 0.5) * radii.x) - UP * radii.z * 0.35
        radius = pick(rng, stem_radius)
        stem = lp.curved_limb(start, end, horizontal(a, 0.6) + UP, 0.02, 3, radius, 0.0, (0.0, 0.25), rng, 0.05, 0.12,
                              0.35, seed * 17 + k)
        stem.radii = [radius * (1.0 - t ** 3) for t in stem.params]
        ptube(plant, stem, SAGE, *SAGE_TWIG, occlusion=lambda p: occlusion(p) * 0.8)
    # The canopy's envelope, and under the leaves a core that follows its lobes a little inside it: the mound's shaded
    # foliage, seen in the gaps between the leaves (and what the far LODs keep of it). Darker and a step below the
    # leaves, it doesn't read as a rock, as a brown core at the surface with sparse leaves did, nor as a basket round a
    # ball, as a small one did.
    surface, shade = lobed(plant, SAGE, center, radii, rng, SAGE_CORE[0], pick(rng, SAGE_CORE[1]), shade_center,
                           lambda p: occlusion(p) * 0.6, wind, lobes, 1, lobe_width=0.45, swell=0.45, bottom=0.85,
                           build=False)
    shell(plant, SAGE, center, lambda d: center + (surface(d) - center) * 0.86, 1, SAGE_CORE[0], SAGE_CORE[1], shade,
          lambda p: occlusion(p) * 0.55, wind)

    def flow(p, n):
        # Up along the surface; outward from the middle where the surface is level.
        return UP + Vector((p.x - center.x, p.y - center.y, 0.0)) * 0.8
    cover(plant, SAGE, rng, surface, shade, center, shoots, -0.8, leaf_size, 0.6, SAGE_LEAF[0], SAGE_LEAF[1],
          occlusion, wind, flow, lift=(0.06, 0.28))

    # Flower stalks: thin and upright out of the top, a slim panicle on each.
    for k in range(stalks):
        d = (random_unit(rng) * Vector((0.8, 0.8, 0.2)) + UP).normalized()
        a = math.atan2(d.y, d.x)
        spike(plant, SAGE, surface(d) - UP * 0.03, horizontal(a), rng.uniform(0.12, 0.2), rng.uniform(0.05, 0.3),
              SAGE_STALK[0], SAGE_STALK[1], shade)

    # Dead twigs: bare and grey, through the foliage on one side, a little proud of it.
    side = rng.uniform(0.0, 2.0 * math.pi)
    height = center.z + radii.z
    for k in range(dead):
        plant.new_group()
        a = side + rng.uniform(-0.6, 0.6)
        start = horizontal(a, rng.uniform(0.03, 0.08)) - UP * 0.1
        end = horizontal(a, max(radii.x, radii.y) * rng.uniform(0.85, 1.05)) + UP * rng.uniform(height * 0.55,
                                                                                               height * 0.9)
        twig = lp.curved_limb(start, end, horizontal(a, 0.3) + UP, 0.15, 3, rng.uniform(0.012, 0.016), 0.0,
                              (0.0, 0.5), rng, 0.05, 0.22, 0.25, seed * 31 + k)
        ptube(plant, twig, SAGE, *SAGE_DEAD, occlusion=lambda p: occlusion(p))
        p, rr, w, dd = twig.at(rng.uniform(0.6, 0.85))
        out = (dd + random_unit(rng) * 0.9 + UP * 0.3).normalized()
        fork = lp.Limb([p, p + out * 0.05, p + out * rng.uniform(0.09, 0.13) + UP * 0.02], rr * 0.8, 0.0, 3,
                       (w, 0.55))
        ptube(plant, fork, SAGE, *SAGE_DEAD, occlusion=lambda p: occlusion(p))
    return finish(plant, None, ao_distance=0.45, ao_blend=0.5, lods=lods)


def sagebrush_a():
    """A full, rounded mound about 1.05 m across and 0.8 m tall, four overlapping lobes, flower stalks a little out of
    its top to 0.9 m."""
    return sagebrush('Sagebrush_A', 301, (0.0, 0.0, 0.36), (0.5, 0.46, 0.4), 4, 120, stalks=8,
                     leaf_size=(0.12, 0.17))


def sagebrush_b():
    """Low and lopsided, about 0.6 m tall and 1.3 m across, its crown pushed off its foot, five lobes and a dead side
    of bare twigs."""
    return sagebrush('Sagebrush_B', 311, (0.1, -0.04, 0.28), (0.6, 0.44, 0.3), 5, 110, stalks=4, dead=2,
                     leaf_size=(0.12, 0.17))


def sagebrush_c():
    """Small and young, about 0.6 m across and 0.45 m tall: three lobes, a thinner cover, two dead twigs through it."""
    return sagebrush('Sagebrush_C', 323, (0.03, 0.02, 0.22), (0.27, 0.25, 0.21), 3, 56, stalks=2, dead=2, stems=2,
                     stem_radius=(0.016, 0.024), leaf_size=(0.09, 0.13), lods=SMALL_SHRUB_LODS)


# --- Rabbitbrush ---

def flower_head(plant, mat, center, radius, up, rng, swatch, u, wind, shade, rim_count=4):
    """A rabbitbrush flower cluster: a rounded dome of tiny florets (a crown, a rim, an underside), on one point of a
    swatch, its crown a hair lighter."""
    side = perpendicular(up)
    other = up.cross(side)
    turn = rng.uniform(0.0, 2.0 * math.pi)
    rim = []
    for k in range(rim_count):
        a = turn + 2.0 * math.pi * k / rim_count + rng.uniform(-0.25, 0.25)
        p = center + (side * math.cos(a) + other * math.sin(a)) * radius * rng.uniform(0.8, 1.1) - up * radius * 0.05
        rim.append((plant.vert(p, wind, shade(p), 0.9), pal_uv(swatch, u, (p - center).dot(side),
                                                                (p - center).dot(other))))
    top_p = center + up * radius * rng.uniform(0.3, 0.42)
    bottom_p = center - up * radius * 0.45
    top = (plant.vert(top_p, wind, shade(top_p), 1.0), pal_uv(swatch, u + 0.04, 0.0, 0.0))
    bottom = (plant.vert(bottom_p, wind, (shade(bottom_p) - up * 0.5).normalized(), 0.7),
              pal_uv(swatch, u - 0.04, 0.01, 0.01))
    for k in range(rim_count):
        k1 = (k + 1) % rim_count
        corners = [top, rim[k], rim[k1]]
        plant.out_face([v for v, _ in corners], [uv for _, uv in corners], mat, up)
        corners = [bottom, rim[k1], rim[k]]
        plant.out_face([v for v, _ in corners], [uv for _, uv in corners], mat, -up)


def rabbitbrush_a():
    """A rounded shrub about 0.7 m tall and 0.75 m across: a green mound of erect olive leaves on short, slender stems,
    shaded inside, its top crowded with small yellow-gold flower clusters."""
    rng = random.Random(331)
    plant = Shrub('Rabbitbrush_A', 331)
    mat = PALETTE
    center = Vector((0.0, 0.0, 0.3))
    radii = Vector((0.37, 0.34, 0.3))
    occlusion = lp.crown_occlusion(center, radii, floor=0.5)
    shade_center = center - UP * 0.25
    plant.new_group()

    def wind(p):
        return lerp(0.25, 0.45, smoothstep(0.1, 0.65, p.z))
    # Stems: slender, splaying out of the foot into the mound, only their feet showing.
    stem_shade = lp.crown_shade(center, shade_center, 0.3)
    for k in range(5):
        plant.new_group()
        a = 2.0 * math.pi * (k + rng.uniform(-0.3, 0.3)) / 5
        root = horizontal(a, rng.uniform(0.0, 0.04)) - UP * 0.08
        top = center + horizontal(a, rng.uniform(0.1, 0.2)) - UP * radii.z * 0.4
        points = lp.bezier(root, root + UP * top.z * 0.45, top - UP * top.z * 0.3, top, lp.steps(2))
        ribbon(plant, mat, points, [0.016, 0.013, 0.01], [horizontal(a + math.pi * 0.5)] * 3,
               [stem_shade(p) for p in points], 'GrassDry', (0.25, 0.4), [0.0, 0.15, 0.3], [0.55, 0.65, 0.8])
    # The mound: erect, narrow leaves laid up along it over a darker olive core a step inside (as the sage's); its top
    # is the flowers'.
    plant.new_group()
    surface, shade = lobed(plant, mat, center, radii, rng, 'GrassOlive', 0.15, shade_center,
                           lambda p: occlusion(p) * 0.65, wind, lobes=4, levels=1, bottom=0.95, swell=0.35, build=False)
    shell(plant, mat, center, lambda d: center + (surface(d) - center) * 0.86, 1, 'GrassOlive', 0.25, shade,
          lambda p: occlusion(p) * 0.6, wind)
    cover(plant, mat, rng, surface, shade, center, 58, -0.8, (0.12, 0.17), 0.45, 'GrassOlive', (0.45, 0.9),
          occlusion, wind, lambda p, n: UP, lift=(0.15, 0.4), fan=0.7,
          skip=lambda p: p.z > center.z + radii.z * 0.6)
    # The flowers crowd the top: low, cushioned clusters (what far LODs keep), and between them fluffy sprays of
    # florets standing a little proud.
    for k, d in enumerate(lp.spiral(44, 0.1, rng.uniform(0.0, 2.0 * math.pi))):
        plant.new_group()
        d = (d + random_unit(rng) * 0.2).normalized()
        if d.z < 0.1:
            continue
        head = surface(d) + d * 0.015
        swatch, u = ('FlowerCenter', rng.uniform(0.8, 0.9)) if rng.random() < 0.3 else \
            ('FlowerYellow', rng.uniform(0.12, 0.35))
        flower_shade = lp.crown_shade(head, shade_center, 0.35)
        if k % 4 == 0:
            flower_head(plant, mat, head - d * 0.01, rng.uniform(0.04, 0.055), (d * 0.5 + UP).normalized(), rng,
                        swatch, u, 0.8, flower_shade, rim_count=5)
        else:
            length = rng.uniform(0.045, 0.065)
            shoot(plant, mat, head - d * 0.015, d * 0.4 + UP, rng, 3, length, length * 0.4, 0.8, swatch, u,
                  flower_shade, lambda p: 1.0, (0.75, 0.9))
    return finish(plant, None, ao_distance=0.4, ao_blend=0.5, lods=ACCENT_LODS)


# --- Junipers ---

def juniper(name, seed, stems, crown_center, crown_radii, clumps, clump_size, sprays, snag, shear=(0.0, 0.0),
            flat_top=None, outliers=2, hull_height=1.2, spray_size=(0.18, 0.26), low=-0.7, floor=0.35):
    """A juniper: gnarled, twisted trunks (stems: dicts of root, lean, fork, radius, fork_radius), a limb from a trunk
    into each foliage pad of an irregular crown that reaches down near the ground (sheared toward shear at the top, cut
    flat at flat_top), each pad drooping sprays round a small dark core, and a dead silver limb (snag: its height on the
    first trunk and its bearing, counterclockwise from +X)."""
    rng = random.Random(seed)
    plant = Shrub(name, seed)
    crown_center = Vector(crown_center)
    pads = clump_layout(rng, crown_center, crown_radii, clumps, clump_size, low, outliers, floor=floor)
    bottom = crown_center.z - crown_radii[2]
    sheared = []
    for c, r in pads:
        f = smoothstep(bottom, crown_center.z + crown_radii[2], c.z)
        c = c + Vector((shear[0], shear[1], 0.0)) * f
        if flat_top is not None and c.z + r * 0.5 > flat_top:
            c = Vector((c.x, c.y, flat_top - r * 0.5))
        sheared.append((c, r))
    pads = sorted(sheared, key=lambda p: p[0].z)
    shade_center = crown_center - UP * crown_radii[2] * 0.5
    occlusion = lp.crown_occlusion(crown_center, crown_radii, floor=0.4)

    # Each pad belongs to the trunk whose fork is nearest it; each trunk runs on as a leader into its highest pad.
    forks = [Vector((s['root'][0] + s['lean'][0], s['root'][1] + s['lean'][1], s['fork'])) for s in stems]
    owner = [min(range(len(stems)), key=lambda k: (forks[k].xy - c.xy).length) for c, r in pads]
    leaders = {}
    for index in range(len(pads)):
        leaders[owner[index]] = index   # pads run low to high: the last one each trunk owns is its highest
    pad_groups = [plant.new_group() for _ in pads]

    # The trunks: short, twisting, leaning apart.
    trunks = []
    for index, stem in enumerate(stems):
        base = Vector((stem['root'][0], stem['root'][1], -0.3))
        fork = forks[index]
        points = lp.bezier(base, base + UP * stem['fork'] * 0.45, fork - (fork - base).normalized() * stem['fork'] * 0.3,
                           fork, [0.0, 0.06, 0.14, 0.25, 0.38, 0.52, 0.68, 0.84, 1.0])
        top_c, top_r = pads[leaders[index]] if index in leaders else (fork + UP * 0.5, 0.2)
        leader_end = top_c + UP * top_r * 0.1
        count = max(2, int((leader_end - fork).length / 0.35))
        points += lp.bezier(fork, fork + (fork - points[-2]).normalized() * 0.5, leader_end - UP * 0.3,
                            leader_end, lp.steps(count))[1:]
        points = lp.gnarl(points, stem.get('gnarl', 0.16), 1.1, seed * 3 + index)
        limb = lp.Limb(points, stem['radius'], 0.0, 7 if index == 0 else 6, (0.0, 0.12))
        limb.radii = [lerp(stem['radius'], stem['fork_radius'], smoothstep(-0.3, stem['fork'], p.z)) if p.z <= stem['fork']
                      else stem['fork_radius'] * max(1.0 - (p.z - stem['fork']) / max(points[-1].z - stem['fork'], 0.1),
                                                     0.0) ** 0.8 for p in points]
        limb.radii[-1] = 0.0
        # The leader sways with its pad above the fork; the trunk below barely moves.
        plant.group = pad_groups[leaders[index]] if index in leaders else 0
        plant.tube(limb, BARK, lp.root_flare(0.7, 4, rng.uniform(0.0, 6.28), 0.45), occlusion)
        trunks.append(limb)

    # A limb into each other pad from its trunk, leaving lower for lower pads, twisting on the way.
    for index, (c, r) in enumerate(pads):
        if leaders.get(owner[index]) == index:
            continue
        plant.group = pad_groups[index]
        trunk = trunks[owner[index]]
        fork_z = stems[owner[index]]['fork']
        share = smoothstep(pads[0][0].z, pads[-1][0].z, c.z)
        height = lerp(fork_z * 0.55, fork_z + (c.z - fork_z) * 0.3, share) + rng.uniform(-0.1, 0.1)
        start, trunk_r, trunk_wind, _ = trunk.at(trunk.param_at_height(min(height, trunk.points[-1].z - 0.05)))
        out = horizontal(math.atan2(c.y - start.y, c.x - start.x)) + UP * lerp(0.3, 0.9, share)
        end = c + (c - start).normalized() * r * 0.2
        limb = lp.curved_limb(start, end, out, 0.15, 4, max(trunk_r * rng.uniform(0.55, 0.7), 0.025), 0.0,
                              (trunk_wind, 0.35), rng, 0.12, 0.4, 0.35, seed * 31 + index)
        plant.tube(limb, BARK, occlusion=occlusion)

    # The pads: each a rounded, shaggy tuft, its dark core (the shade between the sprays, and what the far LODs keep
    # of it) under bunches of short scale-leaf sprays drooping down and out over it, none where a neighbour buries it.
    for index, (c, r) in enumerate(pads):
        plant.group = pad_groups[index]
        others = [o for j, o in enumerate(pads) if j != index]

        def wind(p, c=c, r=r):
            return lerp(0.35, 0.5, smoothstep(c.z - r, c.z + r, p.z))

        def flow(p, n, c=c):
            return Vector((p.x - c.x, p.y - c.y, 0.0)) * 2.0 - UP * 0.6

        def buried(p, others=others):
            return any((p - oc).length < orad * 0.72 for oc, orad in others)
        surface, shade = lobed(plant, JUNIPER, c, (r * 0.8, r * 0.8, r * 0.64), rng, JUNIPER_CORE[0],
                               pick(rng, JUNIPER_CORE[1]), shade_center, lambda p: occlusion(p) * 0.6, wind, lobes=3,
                               levels=1, bottom=0.75, swell=0.3)
        cover(plant, JUNIPER, rng, surface, shade, c, int(round(sprays * (r / clump_size[1]) ** 2)), -0.65, spray_size,
              0.4, JUNIPER_LEAF[0], JUNIPER_LEAF[1], occlusion, wind, flow, lift=(0.08, 0.35), leaves=4, fan=0.8,
              skip=buried)

    # The dead limb: bare, silvery, reaching up and out past the crown on one side, snapped at its tip, one fork.
    plant.new_group()
    height, bearing = snag
    start, trunk_r, trunk_wind, _ = trunks[0].at(trunks[0].param_at_height(height))
    reach = max(crown_radii[0], crown_radii[1]) * 1.05
    end = start + horizontal(math.radians(bearing), reach) + UP * crown_radii[2] * 0.9
    dead = lp.curved_limb(start, end, horizontal(math.radians(bearing)) + UP * 0.6, 0.1, 4, trunk_r * 0.55,
                          trunk_r * 0.12, (trunk_wind, 0.2), rng, 0.15, 0.3, 0.35, seed * 53)
    plant.tube(dead, BARK, occlusion=lambda p: 0.95)
    p, rr, w, d = dead.at(0.6)
    out = (d + horizontal(math.radians(bearing + 70.0)) * 0.8 + UP * 0.4).normalized()
    plant.tube(lp.Limb([p, p + out * 0.25, p + out * 0.5 + UP * 0.08], rr * 0.6, 0.0, 3, (w, 0.25)), BARK,
               occlusion=lambda p: 0.95)

    hull_points = []
    for trunk in trunks:
        for p, r in zip(trunk.points, trunk.radii):
            if 0.0 <= p.z <= hull_height and r > 0.0:
                hull_points += lp.cylinder_points(p, p + UP * 0.01, r * 1.05)
    return finish(plant, hull_points, ao_distance=0.9, ao_blend=0.55, lods=JUNIPER_LODS)


def juniper_a():
    """About 2.2 m tall and 2.3 m across: two gnarled stems leaning apart from one foot, a bushy, irregular crown of
    shaggy clumps from knee height up, a dead limb out of one side."""
    return juniper('Juniper_A', 341, [
        dict(root=(0.04, 0.0), lean=(0.16, 0.06), fork=0.6, radius=0.13, fork_radius=0.085, gnarl=0.14),
        dict(root=(-0.08, 0.03), lean=(-0.36, 0.12), fork=0.48, radius=0.1, fork_radius=0.065, gnarl=0.16)],
        (0.0, 0.05, 1.4), (1.1, 1.0, 1.05), 15, (0.36, 0.5), 26, (0.4, 250.0), outliers=2, low=-0.85, floor=0.3)


def juniper_b():
    """About 1.5 m tall and 2.7 m long: one thick, leaning, twisted trunk, its crown low and swept east (+X) by the
    wind, flat on top, the dead limb on the windward (west) side. Turn its +X downwind (or down the ridge line)."""
    return juniper('Juniper_B', 353, [
        dict(root=(-0.05, 0.0), lean=(0.3, -0.05), fork=0.4, radius=0.15, fork_radius=0.1, gnarl=0.18)],
        (0.25, 0.0, 0.9), (1.05, 0.75, 0.62), 11, (0.32, 0.45), 24, (0.3, 190.0), shear=(0.5, 0.05),
        flat_top=1.6, outliers=2, hull_height=0.9, low=-0.85, floor=0.25)


# --- Dry bunchgrass ---

def dry_tuft(name, seed, blades, height, width, spread, lean, colors, stalks, stalk_height, flattened=0.0,
             segments=3):
    """A bunchgrass tuft: blades rooted in a tight disc of radius spread, arching out (the outer ones lean more, some
    lie flattened), on stretches of the colors swatches ((swatch, u range, weight)), and seed stalks with slim heads
    standing over it."""
    rng = random.Random(seed)
    plant = Shrub(name, seed)
    mat = PALETTE
    pool = [(s, u) for s, u, w in colors for _ in range(w)]
    shade = lp.up_normal(Vector((0.0, 0.0, 0.0)), spread + height * lean, 0.5)
    for i in range(blades):
        plant.new_group()
        r = spread * math.sqrt((i + 0.5) / blades) * rng.uniform(0.7, 1.1)
        a = GOLDEN_ANGLE * i + rng.uniform(-0.4, 0.4)
        root = horizontal(a, r) - UP * 0.025
        edge = r / max(spread, 1e-3)
        flat = rng.random() < flattened
        h = height * rng.uniform(0.7, 1.1) * lerp(1.05, 0.75, edge) * (0.75 if flat else 1.0)
        bend = lean * lerp(0.4, 1.3, edge) * rng.uniform(0.7, 1.2) * (2.0 if flat else 1.0)
        swatch, u = rng.choice(pool)
        u0 = rng.uniform(*u)
        blade(plant, mat, root, horizontal(a + rng.uniform(-0.45, 0.45)), h, width * rng.uniform(0.8, 1.2),
              min(bend, 0.95), swatch, (u0 - 0.12, u0), rng, segments, shade, twist=0.4, occlusion=(0.5, 1.0))
    for k in range(stalks):
        plant.new_group()
        a = rng.uniform(0.0, 2.0 * math.pi)
        root = horizontal(a, spread * 0.5 * math.sqrt(rng.random())) - UP * 0.02
        spike(plant, mat, root, horizontal(a), stalk_height * rng.uniform(0.85, 1.1), rng.uniform(0.1, 0.35), 'Straw',
              rng.uniform(0.85, 0.95), shade, wind=(0.0, 0.95))
    return finish(plant, None, ao_blend=1.0, lods=TUFT_LODS)


def dry_tuft_a():
    """A fountain of straw-golden blades about 35 cm tall and 55 cm across, seven seed stalks to about 50 cm."""
    return dry_tuft('DryTuft_A', 361, 44, 0.36, 0.019, 0.07, 0.55,
                    [('Straw', (0.6, 0.85), 5), ('DryTan', (0.55, 0.85), 3), ('GrassDry', (0.8, 0.95), 2),
                     ('DryTan', (0.2, 0.3), 1)], 7, 0.5)


def dry_tuft_b():
    """An older tuft about 25-30 cm tall and 60 cm across, a third of its blades lying flattened, grey, weathered
    ones among the straw, four seed stalks to about 35 cm."""
    return dry_tuft('DryTuft_B', 367, 40, 0.27, 0.02, 0.08, 0.6,
                    [('Straw', (0.45, 0.75), 3), ('DryTan', (0.35, 0.7), 3), ('DryTan', (0.12, 0.22), 3),
                     ('GrassDry', (0.65, 0.9), 1)], 4, 0.35, flattened=0.33)


def build():
    return [sagebrush_a(), sagebrush_b(), sagebrush_c(), rabbitbrush_a(), juniper_a(), juniper_b(), dry_tuft_a(),
            dry_tuft_b()]


# --- Previews ---

SUN_BEARING, SUN_UP = 247.5, 15.0
OUT = os.path.join(lt.PREVIEW_DIR, 'RansomsRest', 'Scrub')
SLOPE_DEG = 27.0
# The patch: 30 m across (x) and 30 m along the slope, from 3 m up the toe; the bare copy mirrors it across x = 0.
PATCH_X = 16.0
PATCH_Y = (3.0, 3.0 + 30.0 * math.cos(math.radians(SLOPE_DEG)))
CREASE_X = -6.0     # the gully, in the patch's own x (x' = |x| - 16)
ROCKS = [(-10.5, 12.0, (2.6, 1.9, 1.3), 1), (4.0, 21.0, (3.4, 2.2, 1.6), 2), (9.5, 7.0, (1.6, 1.3, 0.9), 3),
         (-1.0, 25.5, (2.2, 1.6, 1.1), 4), (5.5, 22.8, (1.1, 0.9, 0.6), 5)]


def toward_sun():
    b, e = math.radians(SUN_BEARING), math.radians(SUN_UP)
    return Vector((math.sin(b) * math.cos(e), math.cos(b) * math.cos(e), math.sin(e)))


def golden_world():
    """The afternoon sky: warm haze at the horizon, blue above."""
    world = bpy.data.worlds.new('_GoldenWorld')
    world.use_nodes = True
    nodes, links = world.node_tree.nodes, world.node_tree.links
    nodes.clear()
    out = nodes.new('ShaderNodeOutputWorld')
    background = nodes.new('ShaderNodeBackground')
    background.inputs['Strength'].default_value = 0.9
    coords = nodes.new('ShaderNodeTexCoord')
    split = nodes.new('ShaderNodeSeparateXYZ')
    ramp = nodes.new('ShaderNodeValToRGB')
    links.new(coords.outputs['Generated'], split.inputs['Vector'])
    links.new(split.outputs['Z'], ramp.inputs['Fac'])
    e = ramp.color_ramp.elements
    e[0].position, e[0].color = 0.0, lt.hex_color(0xdcb68a)
    e[1].position, e[1].color = 0.9, lt.hex_color(0x6a8cc0)
    mid = e.new(0.12)
    mid.color = lt.hex_color(0xc4bfb2)
    high = e.new(0.45)
    high.color = lt.hex_color(0x8ea8c8)
    links.new(ramp.outputs['Color'], background.inputs['Color'])
    links.new(background.outputs['Background'], out.inputs['Surface'])
    return world


def ochre_material():
    """The stand-in ground: ochre soil mottled at two scales, as the area's slopes read on Medium."""
    mat = bpy.data.materials.get('_Ochre')
    if mat:
        return mat
    mat = bpy.data.materials.new('_Ochre')
    mat.use_nodes = True
    nodes, links = mat.node_tree.nodes, mat.node_tree.links
    bsdf = next(n for n in nodes if n.type == 'BSDF_PRINCIPLED')
    bsdf.inputs['Roughness'].default_value = 1.0
    coords = nodes.new('ShaderNodeTexCoord')
    big = nodes.new('ShaderNodeTexNoise')
    big.inputs['Scale'].default_value = 0.06
    big.inputs['Detail'].default_value = 3.0
    fine = nodes.new('ShaderNodeTexNoise')
    fine.inputs['Scale'].default_value = 1.4
    fine.inputs['Detail'].default_value = 4.0
    links.new(coords.outputs['Object'], big.inputs['Vector'])
    links.new(coords.outputs['Object'], fine.inputs['Vector'])
    mix = nodes.new('ShaderNodeMix')
    mix.data_type = 'FLOAT'
    mix.inputs['Factor'].default_value = 0.35
    links.new(big.outputs['Fac'], mix.inputs['A'])
    links.new(fine.outputs['Fac'], mix.inputs['B'])
    ramp = nodes.new('ShaderNodeValToRGB')
    ramp.color_ramp.elements[0].position, ramp.color_ramp.elements[0].color = 0.3, lt.hex_color(0x6a5232)
    ramp.color_ramp.elements[1].position, ramp.color_ramp.elements[1].color = 0.7, lt.hex_color(0x9a7c4c)
    links.new(mix.outputs['Result'], ramp.inputs['Fac'])
    links.new(ramp.outputs['Color'], bsdf.inputs['Base Color'])
    return mat


def new_object(name, verts, faces, mat):
    mesh = bpy.data.meshes.new(name)
    mesh.from_pydata(verts, [], faces)
    mesh.materials.append(mat)
    for polygon in mesh.polygons:
        polygon.use_smooth = True
    obj = bpy.data.objects.new(name, mesh)
    bpy.context.scene.collection.objects.link(obj)
    return obj


def ground_plane(size=60.0):
    s = size
    return new_object('_Ground', [(-s, -s, 0.0), (s, -s, 0.0), (s, s, 0.0), (-s, s, 0.0)], [(0, 1, 2, 3)],
                      ochre_material())


def render(objects, path, cam, target, lens=35.0, resolution=(1280, 720), samples=64):
    """Renders objects from cam toward target in Ransom's Rest's golden light (AgX, as lt.preview)."""
    scene = bpy.context.scene
    shown = set(objects)
    hidden = {o: o.hide_render for o in scene.objects}
    for o in scene.objects:
        o.hide_render = o not in shown
    camera = bpy.data.objects.new('_Cam', bpy.data.cameras.new('_Cam'))
    camera.data.lens = lens
    camera.data.sensor_fit = 'HORIZONTAL'
    camera.data.sensor_width = 36.0
    camera.data.clip_start = 0.05
    camera.data.clip_end = 3000.0
    camera.location = cam
    camera.rotation_euler = (Vector(target) - Vector(cam)).to_track_quat('-Z', 'Y').to_euler()
    scene.collection.objects.link(camera)
    sun = bpy.data.objects.new('_Sun', bpy.data.lights.new('_Sun', 'SUN'))
    sun.data.energy = 6.0
    sun.data.color = (1.0, 0.8, 0.6)
    sun.data.angle = math.radians(1.5)
    sun.rotation_euler = (-toward_sun()).to_track_quat('-Z', 'Y').to_euler()
    scene.collection.objects.link(sun)
    world = golden_world()
    scene.camera = camera
    scene.world = world
    scene.render.resolution_x, scene.render.resolution_y = resolution
    scene.render.resolution_percentage = 100
    scene.render.film_transparent = False
    scene.render.image_settings.file_format = 'PNG'
    scene.render.image_settings.color_mode = 'RGB'
    scene.render.filepath = path
    scene.view_settings.view_transform = 'AgX'
    scene.view_settings.look = 'AgX - Medium High Contrast'
    scene.render.engine = 'BLENDER_EEVEE_NEXT'
    scene.eevee.taa_render_samples = samples
    os.makedirs(os.path.dirname(path), exist_ok=True)
    bpy.ops.render.render(write_still=True)
    print(f'LOOTER: preview: {path}', flush=True)
    for o in (camera, sun):
        data = o.data
        bpy.data.objects.remove(o)
        if isinstance(data, bpy.types.Camera):
            bpy.data.cameras.remove(data)
        else:
            bpy.data.lights.remove(data)
    bpy.data.worlds.remove(world)
    for o, value in hidden.items():
        if o.name in scene.objects:
            o.hide_render = value


def place(src, location, yaw=0.0, scale=1.0, mesh=None):
    """A copy of src (sharing its mesh, or mesh) at location."""
    copy = src.copy()
    if mesh is not None:
        copy.data = mesh
    copy.location = location
    copy.rotation_euler = (0.0, 0.0, yaw)
    copy.scale = (scale, scale, scale)
    bpy.context.scene.collection.objects.link(copy)
    return copy


def bounds_radius(obj):
    """The bounds' sphere radius Unreal's LOD screen size uses (about: the farthest vertex from the box's middle)."""
    co = np.empty(3 * len(obj.data.vertices))
    obj.data.vertices.foreach_get('co', co)
    co = co.reshape(-1, 3)
    middle = (co.min(axis=0) + co.max(axis=0)) * 0.5
    return float(np.linalg.norm(co - middle, axis=1).max())


def lod_meshes(obj):
    """obj's mesh and stand-ins for its LODs (Blender's collapse at each LODs share, custom normals carried over from
    the full mesh), with the screen size each starts at: [(screen, mesh)], from LOD0."""
    result = [(1.0, obj.data)]
    if not obj.get('LODs'):
        return result
    shares = [float(s) / 100.0 for s in str(obj['LODs']).split(',')]
    screens = [float(s) for s in str(obj['LODScreens']).split(',')]
    for share, screen in zip(shares, screens):
        copy = obj.copy()
        copy.data = obj.data.copy()
        copy.name = '_LOD_' + obj.name
        bpy.context.scene.collection.objects.link(copy)
        decimate = copy.modifiers.new('LOD', 'DECIMATE')
        decimate.ratio = share
        transfer = copy.modifiers.new('Normals', 'DATA_TRANSFER')
        transfer.object = obj
        transfer.use_loop_data = True
        transfer.data_types_loops = {'CUSTOM_NORMAL'}
        transfer.loop_mapping = 'POLYINTERP_NEAREST'
        bpy.context.view_layer.objects.active = copy
        bpy.ops.object.modifier_apply(modifier='LOD')
        try:
            bpy.ops.object.modifier_apply(modifier='Normals')
        except RuntimeError as error:
            print(f'LOOTER: preview: {obj.name} LOD normals not carried over ({error})', flush=True)
            copy.modifiers.remove(copy.modifiers['Normals'])
        print(f'LOOTER: preview: {obj.name} LOD stand-in {share:.0%}: {lp.triangles(copy)} triangles', flush=True)
        result.append((screen, copy.data))
        bpy.data.objects.remove(copy)
    return result


def lod_for(lods, radius, distance, lens):
    """The mesh Unreal would draw at distance through a lens on a 36 mm, 16:9 frame (its screen size: the bounds'
    radius over distance times the projection's larger scale, 16/9 / tan(half the horizontal field of view))."""
    tan_half = 18.0 / lens
    screen = (16.0 / 9.0) / tan_half * radius / max(distance, 1e-3)
    chosen = lods[0][1]
    for start, mesh in lods:
        if screen <= start:
            chosen = mesh
    return chosen


def local_x(x):
    """The patch's own x: both halves of the slope (scrubbed at x < 0, bare at x > 0) mirror each other across x = 0."""
    return abs(x) - PATCH_X


def slope_height(x, y):
    """The stand-in slope: a valley floor (y < 0) rising at SLOPE_DEG past a soft toe, a gully down CREASE_X and a
    slow swell, the same on both halves."""
    k = 2.5
    rise = math.tan(math.radians(SLOPE_DEG)) * k * math.log1p(math.exp(min(y / k, 50.0)))
    lx = local_x(x)
    crease = -1.6 * math.exp(-((lx - CREASE_X) / 3.2) ** 2) * smoothstep(0.0, 8.0, y)
    swell = 1.4 * noise.noise(Vector((lx / 22.0, y / 22.0, 0.37))) * smoothstep(-2.0, 8.0, y)
    return rise + crease + swell


def slope_ground():
    xs = np.arange(-90.0, 90.01, 1.0)
    ys = np.arange(-160.0, 70.01, 1.0)
    verts = [(x, y, slope_height(x, y)) for y in ys for x in xs]
    nx = len(xs)
    faces = [(j * nx + i, j * nx + i + 1, (j + 1) * nx + i + 1, (j + 1) * nx + i) for j in range(len(ys) - 1)
             for i in range(nx - 1)]
    return new_object('_Slope', verts, faces, ochre_material())


def rock(name, location, size, seed, mat):
    """A stand-in outcrop: a lumpy, flattened rock sunk into the slope (preview only)."""
    bm = bmesh.new()
    bmesh.ops.create_icosphere(bm, subdivisions=2, radius=1.0)
    offset = Vector((seed * 3.1, seed * 1.7, seed * 2.3))
    for v in bm.verts:
        d = v.co.normalized()
        lump = 1.0 + 0.22 * noise.noise(d * 1.6 + offset) + 0.08 * noise.noise(d * 4.0 + offset)
        flat = 0.75 if d.z > 0.55 else 1.0
        v.co = Vector((d.x * size[0], d.y * size[1], d.z * size[2] * flat)) * lump * 0.5
    mesh = bpy.data.meshes.new(name)
    bm.to_mesh(mesh)
    bm.free()
    mesh.materials.append(mat)
    obj = bpy.data.objects.new(name, mesh)
    bpy.context.scene.collection.objects.link(obj)
    obj.location = location
    obj.rotation_euler = (math.radians(SLOPE_DEG * 0.6), 0.0, seed * 1.3)
    bpy.context.view_layer.update()
    lt.box_uv(obj, 'RockCliff', seed=seed)
    return obj


def slope_scene(models):
    """The slope and its rocks (both halves), and the scrub on the left patch: (ground objects, [(model, location,
    yaw, scale)])."""
    by = {m.name: m for m in models}
    rng = random.Random(29)
    # The area's rock is warm ochre-tan (the cliff kit wears the area's colors), not the set's pale grey: a preview-only
    # tint, never exported.
    cliff = lt.material('RockCliff', name='_RockCliffWarm', tint=0xc49a6a)
    scenery = [slope_ground()]
    rocks = []
    for k, (lx, y, size, seed) in enumerate(ROCKS):
        for sign in (-1.0, 1.0):
            x = sign * (lx + PATCH_X)
            z = slope_height(x, y) + size[2] * 0.12
            scenery.append(rock(f'_Rock{k}{"L" if sign < 0 else "R"}', (x, y, z), size, seed, cliff))
        rocks.append((lx, y, max(size[0], size[1]) * 0.5))
    placed = []
    taken = []   # (local x, y, radius)

    def free(lx, y, radius):
        if abs(lx) > PATCH_X - 1.0 or not (PATCH_Y[0] <= y <= PATCH_Y[1]):
            return False
        for ox, oy, orad in rocks + taken:
            if math.hypot(lx - ox, y - oy) < radius + orad:
                return False
        return True

    def add(name, lx, y, radius, scale=None, yaw=None):
        x = -(lx + PATCH_X)
        scale = scale or rng.uniform(0.85, 1.2)
        placed.append((by[name], Vector((x, y, slope_height(x, y) - 0.04)), yaw if yaw is not None else
                       rng.uniform(0.0, 2.0 * math.pi), scale))
        taken.append((lx, y, radius * scale))

    # Junipers down the gully, one on the patch's top edge (its ridge line).
    y = PATCH_Y[0] + 3.0
    while y < PATCH_Y[1] - 1.0:
        lx = CREASE_X + rng.uniform(-1.3, 1.3)
        if free(lx, y, 1.4):
            add(rng.choice(['Juniper_A', 'Juniper_A', 'Juniper_B']), lx, y, 1.3)
        y += rng.uniform(3.5, 5.5)
    for lx in (8.0, 12.5):
        if free(lx, PATCH_Y[1] - 1.2, 1.2):
            add('Juniper_B', lx, PATCH_Y[1] - 1.2, 1.1, yaw=rng.uniform(-0.3, 0.3))
    # Sage everywhere, patchy (a jittered grid thinned by slow noise); rabbitbrush along the gully's margins and the
    # toe; tufts between.
    for kind, spacing, keep, radius in (('sage', 1.45, 0.9, 0.38), ('rabbit', 5.0, 0.9, 0.35),
                                        ('tuft', 1.2, 0.85, 0.18)):
        y = PATCH_Y[0]
        while y < PATCH_Y[1]:
            lx = -PATCH_X
            while lx < PATCH_X:
                px = lx + rng.uniform(-0.5, 0.5) * spacing
                py = y + rng.uniform(-0.5, 0.5) * spacing
                patchy = 0.55 + 0.9 * noise.noise(Vector((px / 9.0, py / 9.0, 4.2)))
                near_gully = math.exp(-((px - CREASE_X) / 4.0) ** 2)
                if kind == 'sage':
                    chance = keep * min(max(patchy, 0.25), 1.2) * (1.0 - 0.5 * near_gully)
                    name = rng.choice(['Sagebrush_A'] * 4 + ['Sagebrush_B'] * 3 + ['Sagebrush_C'] * 3)
                elif kind == 'rabbit':
                    chance = keep * (0.35 + near_gully + smoothstep(PATCH_Y[0] + 6.0, PATCH_Y[0], py))
                    name = 'Rabbitbrush_A'
                else:
                    chance = keep * min(max(1.3 - patchy, 0.35), 1.0)
                    name = rng.choice(['DryTuft_A', 'DryTuft_B'])
                if rng.random() < chance and free(px, py, radius):
                    add(name, px, py, radius)
                lx += spacing
            y += spacing
    counts = {}
    for model, _, _, _ in placed:
        counts[model.name] = counts.get(model.name, 0) + 1
    print('LOOTER: preview: slope patch ' + ', '.join(f'{k} {v}' for k, v in sorted(counts.items())) +
          f' ({len(placed)} on {30 * 30} m2)', flush=True)
    return scenery, placed


def slope_views(models):
    """The patch from 20 m and from 120 m (the game's 90 degree view, and a 3x zoom showing it beside the bare copy),
    each model drawn at the LOD Unreal would pick at its distance."""
    scenery, placed = slope_scene(models)
    lods = {m.name: (bounds_radius(m), lod_meshes(m)) for m in models}
    patch_mid = (-PATCH_X, (PATCH_Y[0] + PATCH_Y[1]) * 0.5)
    views = [
        ('Slope_20m', (patch_mid[0] - 3.0, -2.5), (patch_mid[0], patch_mid[1] - 1.0), 18.0),
        ('Slope_120m', (0.0, -112.0), (0.0, patch_mid[1] + 3.0), 18.0),
        ('Slope_120m_Zoom', (0.0, -112.0), (0.0, patch_mid[1] + 1.0), 60.0),
    ]
    for name, (cx, cy), (tx, ty), lens in views:
        cam = Vector((cx, cy, slope_height(cx, cy) + 1.7))
        target = Vector((tx, ty, slope_height(tx, ty) + 1.0))
        copies = []
        for model, location, yaw, scale in placed:
            radius, meshes = lods[model.name]
            mesh = lod_for(meshes, radius * scale, (location - cam).length, lens)
            copies.append(place(model, location, yaw, scale, mesh))
        bpy.context.view_layer.update()
        render(copies + scenery, os.path.join(OUT, name + '.png'), cam, target, lens=lens, resolution=(1920, 1080),
               samples=48)
        for copy in copies:
            bpy.data.objects.remove(copy)


def closeups(models, wanted):
    ground = ground_plane()
    for model in models:
        if wanted and model.name not in wanted and 'closeups' not in wanted:
            continue
        size = max(model.dimensions.x, model.dimensions.y)
        height = max(c[2] for c in model.bound_box)
        distance = max(2.3 * height, 1.3 * size, 1.1)
        view = Vector((-0.42, -1.0, 0.32)).normalized()
        target = Vector((0.0, 0.0, height * 0.42))
        copy = place(model, (0.0, 0.0, 0.0))
        render([copy, ground], os.path.join(OUT, model.name + '.png'), target + view * distance, target, lens=35.0)
        bpy.data.objects.remove(copy)
    bpy.data.objects.remove(ground)


def overview(models):
    """The kit side by side in lt.preview's neutral light, with a person-height figure for scale."""
    import importlib.util
    spec = importlib.util.spec_from_file_location('groundcover', os.path.join(lt.REPO, 'Art', 'Models', 'Vegetation',
                                                                              'GroundCover.py'))
    groundcover = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(groundcover)
    order = ['DryTuft_B', 'DryTuft_A', 'Sagebrush_C', 'Sagebrush_B', 'Sagebrush_A', 'Rabbitbrush_A', 'Juniper_B',
             'Juniper_A']
    by = {m.name: m for m in models}
    shown, x = [], 0.0
    for name in order:
        model = by[name]
        copy = lp.planted(model)
        lo = min(c[0] for c in model.bound_box)
        hi = max(c[0] for c in model.bound_box)
        copy.location.x = x - lo
        shown.append(copy)
        x += (hi - lo) + 0.3
    figure = groundcover.person()
    figure.location = (x + 0.3, 0.3, 0.0)
    shown.append(figure)
    bpy.context.view_layer.update()
    lt.preview(shown, os.path.join(lt.PREVIEW_DIR, 'RansomsRest', 'Scrub_overview.png'), view=(-0.2, -1.0, 0.2),
               fit=0.52)
    for copy in shown:
        mesh = copy.data
        bpy.data.objects.remove(copy)
        bpy.data.meshes.remove(mesh)


def lod_grid(models):
    """LODs.png: each model (a row) at LOD0 to its last LOD (left to right; Blender's collapse standing in for
    Unreal's reduction), close up in the golden light, to judge what each LOD keeps."""
    rows, y = [], 0.0
    for model in sorted(models, key=lambda m: m.dimensions.z):
        depth = max(model.dimensions.x, model.dimensions.y)
        rows.append((model, y + depth * 0.5))
        y += depth * 0.8 + 0.4
    copies = []
    for model, row_y in rows:
        for k, (_, mesh) in enumerate(lod_meshes(model)):
            copies.append(place(model, (k * 3.0 - 4.5, row_y, 0.0), 0.0, 1.0, mesh))
    ground = ground_plane(80.0)
    bpy.context.view_layer.update()
    target = Vector((0.0, y * 0.42, 0.3))
    render(copies + [ground], os.path.join(OUT, 'LODs.png'), Vector((0.0, -y * 0.55 - 3.0, y * 0.75 + 2.0)), target,
           lens=30.0, resolution=(1600, 1600))
    for copy in copies:
        bpy.data.objects.remove(copy)
    bpy.data.objects.remove(ground)


def previews(models):
    argv = sys.argv[sys.argv.index('--') + 1:] if '--' in sys.argv else []
    wanted = [p for a in argv if not a.startswith('--') for p in a.split(',') if p]
    names = {m.name for m in models}
    every = not wanted
    if every or 'closeups' in wanted or names & set(wanted):
        closeups(models, [w for w in wanted if w in names or w == 'closeups'])
    if every or 'overview' in wanted:
        overview(models)
    if every or 'lods' in wanted:
        lod_grid(models)
    if every or 'slope' in wanted:
        slope_views(models)


if __name__ == '__main__':
    models = build()
    lp.report(models)
    for obj in models:
        print(f'LOOTER: {obj.name}: {obj.dimensions.x:.2f} x {obj.dimensions.y:.2f} x '
              f'{max(c[2] for c in obj.bound_box):.2f} m above ground, bounds radius {bounds_radius(obj):.2f} m',
              flush=True)
    if lt.want_preview():
        previews(models)
