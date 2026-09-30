"""Undergrowth (Docs/TutorialIsland.md): Bush_A, Bush_B, Bush_C (0.9-1.8 m), Fern_A, Reeds_A (the pond's edge), Stump_A
and Log_A (fallen, mossy). Scripted models (Art/README.md) built with Tools/Blender/looter_plants.py (bark tubes, leaf
cards, fronds and reeds); each model's origin is where it meets the ground.

Bushes, ferns and reeds are walk-through: no collision hulls (Collision 'none'). The stump and the log have one convex
hull each.

    blender -b --factory-startup --python Art/Models/Vegetation/Undergrowth.py -- --preview [Bush_A ...]
"""
import math
import random

from mathutils import Vector

import looter_textures as lt
from looter_plants import (UP, Limb, Plant, blade, crown_occlusion, curved_limb, cylinder_points, finish, gnarl,
                           horizontal, layout_clumps, leaf_clump, lerp, perpendicular, petal, preview_all, report,
                           root_flare, swatch, up_normal)

# LODs (triangle percentages) and the screen sizes they switch at. Unreal's screen size is about 1.8 x radius /
# distance at a 90 degree field of view, so these small models would drop to their LODs within a couple of meters at
# the importer's defaults (0.45, 0.16, made for trees).
SMALL_LODS = ('40,15', '0.2,0.07')    # bushes (about 0.7 m radius): LOD1 from about 6 m, LOD2 from 18 m
PLANT_LODS = ('50,20', '0.25,0.08')   # fern and reeds: from about 5 m and 14 m
WOOD_LODS = ('50,20', '0.3,0.1')      # stump and log: from about 4 m and 11 m (the log, longer, later)


# --- Bushes: a few stems into leaf clumps ---

def bush(name, seed, leaves, crown_center, crown_radii, clumps, clump_size, card_size, cards_per_clump, low=-0.7):
    """A bush: leaf clumps over a squat crown, each fed by a thin stem from the ground, cards filling the clumps
    through (a bush has no hollow middle to show)."""
    rng = random.Random(seed)
    plant = Plant(name, seed)
    bark = lt.material('BarkOak')
    crown_center = Vector(crown_center)
    crown = layout_clumps(rng, crown_center, crown_radii, clumps, clump_size, low=low, jitter=0.25)
    occlusion = crown_occlusion(crown_center, crown_radii, floor=0.45)
    shade_center = crown_center - UP * crown_radii[2] * 0.5
    for index, (center, radius) in enumerate(crown):
        a = math.atan2(center.y, center.x)
        start = horizontal(a, rng.uniform(0.05, 0.18)) - UP * 0.1
        stem = curved_limb(start, center, horizontal(a, 0.4) + UP, 0.05, 3, rng.uniform(0.018, 0.03), 0.0, (0.0, 0.4),
                           rng, 0.05, 0.3, 0.3, seed * 17 + index)
        plant.tube(stem, bark, occlusion=occlusion)
    for index, (center, radius) in enumerate(crown):
        others = [c for j, c in enumerate(crown) if j != index]
        count = int(round(cards_per_clump * (radius / clump_size[1]) ** 2))
        leaf_clump(plant, rng, center, radius, shade_center, leaves, count, card_size, others, occlusion,
                   shell=(0.3, 0.95), fold=0.12, wind=(0.5, 1.0))
    return finish(plant, None, ao_distance=0.5, ao_blend=0.6, lods=SMALL_LODS[0], lod_screens=SMALL_LODS[1])


def bush_a():
    """A round bush, about 1 m tall and 1.3 m across."""
    return bush('Bush_A', 201, lt.material('LeavesOak'), (0.0, 0.0, 0.5), (0.62, 0.58, 0.45), 6, (0.34, 0.46),
                (0.55, 0.72), 40)


def bush_b():
    """A low, spreading bush, about 0.9 m tall and 1.8 m across, lighter leaves."""
    return bush('Bush_B', 211, lt.material('LeavesBirch'), (0.0, 0.0, 0.42), (0.9, 0.75, 0.42), 8, (0.34, 0.46),
                (0.5, 0.68), 36)


def bush_c():
    """A tall, upright shrub (hazel or elder), about 1.8 m, its base more open."""
    return bush('Bush_C', 223, lt.material('LeavesOak'), (0.0, 0.0, 0.98), (0.66, 0.6, 0.82), 9, (0.34, 0.46),
                (0.6, 0.78), 34, low=-0.8)


# --- Fern: a rosette of arching fronds ---

def frond(plant, mat, rng, heading, length, rise, shade, pairs=11):
    """A fern frond: a rachis arching out and down from the crown, pinnae in pairs along it, longest a third of the
    way out and angled toward the tip."""
    heading = heading.normalized()
    side = UP.cross(heading).normalized()
    roll = rng.uniform(-0.25, 0.25)
    points = []
    for i in range(pairs + 2):
        t = i / (pairs + 1)
        # Up steeply out of the crown, then arching over, the tip lower than the arch but off the ground.
        points.append(heading * (length * t) + UP * (length * rise * math.sin(t * math.pi * 0.8) * (1.0 - 0.25 * t)))
    # The rachis, a thin strip.
    last = len(points) - 1
    rachis = []
    for i, p in enumerate(points):
        t = i / last
        for s in (-1.0, 1.0):
            rachis.append(plant.vert(p + side * (s * 0.006 * (1.0 - t)), lerp(0.1, 1.0, t), shade(p),
                                     lerp(0.6, 1.0, t)))
    for i in range(last):
        t0, t1 = i / last, (i + 1) / last
        plant.face((rachis[2 * i], rachis[2 * i + 1], rachis[2 * i + 3], rachis[2 * i + 2]),
                   [swatch('Stem', t0, 0.0), swatch('Stem', t0, 1.0), swatch('Stem', t1, 1.0),
                    swatch('Stem', t1, 0.0)], mat)
    for i in range(1, pairs + 1):
        t = i / (pairs + 1)
        p = points[i]
        along = (points[i + 1] - points[i - 1]).normalized()
        normal = (UP - along * along.dot(UP)).normalized()
        normal = (normal * math.cos(roll) + along.cross(normal) * math.sin(roll)).normalized()
        across = normal.cross(along).normalized()
        # Lanceolate: the pinnae grow quickly to their longest a quarter of the way out, then taper to the tip.
        size = length * 0.2 * (1.0 - t) ** 0.7 * min(1.0, t * 4.0) + 0.025
        for s in (-1.0, 1.0):
            d = (across * s + along * 0.35).normalized()
            petal(plant, mat, p, d - normal * 0.12, size, size * 0.42, 'Fern', normal, wind=lerp(0.2, 1.0, t),
                  occlusion=lerp(0.6, 1.0, t), cup=-0.1)
    # The frond ends in a leaflet of its own rather than a bare stalk.
    tip_dir = (points[-1] - points[-2]).normalized()
    normal = (UP - tip_dir * tip_dir.dot(UP)).normalized()
    petal(plant, mat, points[-2], tip_dir, (points[-1] - points[-2]).length + 0.03, 0.02, 'Fern', normal, wind=1.0)


def fern_a():
    """A fern, about 0.55 m tall and 1.3 m across: eight arching fronds, their leaflets packed close."""
    rng = random.Random(233)
    plant = Plant('Fern_A', 233)
    mat = lt.material('FoliagePalette')
    shade = up_normal(Vector((0.0, 0.0, 0.0)), 0.6, 0.5)
    for i in range(8):
        a = 2.0 * math.pi * i / 8 + rng.uniform(-0.25, 0.25)
        frond(plant, mat, rng, horizontal(a), rng.uniform(0.62, 0.8), rng.uniform(0.55, 0.8), shade, pairs=17)
    return finish(plant, None, ao_blend=1.0, lods=PLANT_LODS[0], lod_screens=PLANT_LODS[1])


# --- Reeds: tall blades and cattails ---

def reeds_a():
    """Reeds for the pond's edge, 1-1.6 m: narrow blades, a few bent over, and four cattails."""
    rng = random.Random(241)
    plant = Plant('Reeds_A', 241)
    mat = lt.material('FoliagePalette')
    shade = up_normal(Vector((0.0, 0.0, 0.0)), 0.4, 0.35)
    for i in range(24):
        a = rng.uniform(0.0, 2.0 * math.pi)
        root = horizontal(a, 0.16 * math.sqrt(rng.random())) - UP * 0.05
        lean = rng.uniform(0.1, 0.35) if rng.random() > 0.12 else rng.uniform(0.5, 0.7)
        # Green in the growing season, with some older blades and dry tips among them.
        blade(plant, mat, root, horizontal(a + rng.uniform(-0.4, 0.4)), rng.uniform(1.0, 1.6), rng.uniform(0.022, 0.03),
              lean, rng.choice(['GrassOlive', 'GrassDeep', 'GrassFresh', 'Reed']), rng, 3, shade, twist=0.5,
              tip='GrassDry' if rng.random() < 0.35 else None)
    for i in range(4):
        a = 2.0 * math.pi * i / 4 + rng.uniform(-0.5, 0.5)
        root = horizontal(a, rng.uniform(0.02, 0.1)) - UP * 0.05
        top = blade(plant, mat, root, horizontal(a), rng.uniform(1.3, 1.6), 0.012, rng.uniform(0.05, 0.12), 'Stem', rng,
                    2, shade, taper=0.3, wind=(0.0, 0.9))
        # The cattail's head, a brown sausage just under the stem's tip.
        axis = (top - root).normalized()
        cattail(plant, mat, top - axis * 0.28, axis, 0.2, 0.024)
    return finish(plant, None, ao_blend=1.0, lods=PLANT_LODS[0], lod_screens=PLANT_LODS[1])


def cattail(plant, mat, base, axis, length, radius, sides=6):
    """A cattail head: a closed, rounded cylinder along axis."""
    side = perpendicular(axis)
    other = axis.cross(side)
    shade = up_normal(Vector((0.0, 0.0, 0.0)), 0.4, 0.35)
    bottom = plant.vert(base - axis * radius * 0.5, 0.9, shade(base), 0.9)
    top = plant.vert(base + axis * (length + radius * 0.5), 1.0, shade(base), 1.0)
    rings = []
    for k, t in enumerate((0.0, 1.0)):
        ring = []
        for s in range(sides):
            a = 2.0 * math.pi * s / sides
            p = base + axis * (length * t) + (side * math.cos(a) + other * math.sin(a)) * radius
            normal = (side * math.cos(a) + other * math.sin(a) + UP * 0.8).normalized()
            ring.append(plant.vert(p, lerp(0.9, 1.0, t), normal, 0.95))
        rings.append(ring)
    uv = swatch('CattailBrown', 0.5, 0.5)
    for s in range(sides):
        s1 = (s + 1) % sides
        plant.face((rings[0][s], rings[0][s1], rings[1][s1], rings[1][s]), [uv] * 4, mat)
        plant.face((bottom, rings[0][s1], rings[0][s]), [uv] * 3, mat)
        plant.face((top, rings[1][s], rings[1][s1]), [uv] * 3, mat)


# --- Stumps and logs ---

def saw_cut(wood, seed=0):
    """A cap for Plant.tube: the saw cut, T_WoodEndGrain's log end (growth rings inside its bark rim) fitted to the
    cut's outline and turned by seed, as lt.cap_uv maps caps."""
    turn = random.Random(seed).uniform(0.0, 2.0 * math.pi)

    def cap(plant, ring, center, direction):
        # Its own vertices, so the cut meets the bark at a hard edge.
        rim = [plant.vert(v.co.copy(), 0.0, direction, 0.9) for v in ring]
        hub = plant.vert(center + direction * 0.01, 0.0, direction, 0.9)
        side = perpendicular(direction)
        other = direction.cross(side)
        side, other = side * math.cos(turn) + other * math.sin(turn), other * math.cos(turn) - side * math.sin(turn)
        scale = 0.48 / max((v.co - center).length for v in ring)

        def uv(p):
            d = p - center
            return (0.5 + d.dot(side) * scale, 0.5 + d.dot(other) * scale)
        for s in range(len(rim)):
            s1 = (s + 1) % len(rim)
            plant.face((hub, rim[s], rim[s1]), [uv(hub.co), uv(rim[s].co), uv(rim[s1].co)], wood)
    return cap


def stump_a():
    """A felled tree's stump, 0.5 m, roots gripping the ground and a slanted saw cut on top."""
    rng = random.Random(251)
    plant = Plant('Stump_A', 251)
    bark = lt.material('BarkOak')
    wood = lt.material('WoodEndGrain')
    radius = 0.32
    points = [Vector((0.0, 0.0, z)) for z in (-0.3, -0.1, 0.05, 0.2, 0.36, 0.5)]
    points = gnarl(points, 0.03, 1.5, 251)
    trunk = Limb(points, radius * 1.05, radius, 10, (0.0, 0.0))
    plant.tube(trunk, bark, root_flare(1.3, 5, 0.4, 0.45), cap=saw_cut(wood, 251))
    for i in range(5):
        # Roots between the flare's lobes: thick where they leave the stump, diving into the ground.
        a = (2.0 * math.pi * i + math.pi - 0.4) / 5 + rng.uniform(-0.15, 0.15)
        start = horizontal(a, radius * 0.55) + UP * 0.2
        end = horizontal(a, radius * rng.uniform(1.9, 2.3)) - UP * 0.16
        root = curved_limb(start, end, horizontal(a) - UP * 0.1, 0.05, 6, rng.uniform(0.12, 0.15), 0.035, (0.0, 0.0),
                           rng, 0.03, 0.18, 0.3, 251 + i)
        plant.tube(root, bark)
    hull_points = cylinder_points(Vector((0.0, 0.0, 0.0)), Vector((0.0, 0.0, 0.5)), radius * 1.15)
    return finish(plant, hull_points, ao_distance=0.4, ao_blend=0.4, lods=WOOD_LODS[0], lod_screens=WOOD_LODS[1])


def log_a():
    """A fallen, mossy log, 3.4 m long and half a meter thick, lying along X and sunk a little into the ground: sawn
    at one end, snapped at the other, two broken branch stubs and a mat of moss along its top."""
    rng = random.Random(263)
    plant = Plant('Log_A', 263)
    bark = lt.material('BarkOak')
    wood = lt.material('WoodEndGrain')
    moss = lt.material('FoliagePalette')
    r0, r1 = 0.27, 0.22
    points = [Vector((x, 0.0, lerp(r0, r1, (x + 1.7) / 3.4) * 0.82)) for x in [-1.7 + 0.34 * i for i in range(11)]]
    points = gnarl(points, 0.05, 0.6, 263)
    log = Limb(points, r0, r1, 9, (0.0, 0.0))
    plant.tube(log, bark, cap=None)
    # The sawn end faces -X: a short tube out of that end closes it with the cut.
    back = Limb([points[0], points[0] + (points[0] - points[1]).normalized() * 0.02], r0, r0, 9, (0.0, 0.0))
    plant.tube(back, bark, cap=saw_cut(wood, 263))
    for t, a in ((0.35, 1.0), (0.7, -0.6)):
        p, r, _, d = log.at(t)
        out = (Vector((0.0, math.sin(a), math.cos(a))) + d * 0.4).normalized()
        stub = Limb([p, p + out * r * 1.4, p + out * r * 2.1], r * 0.35, r * 0.25, 5, (0.0, 0.0))
        plant.tube(stub, bark)

    # Moss: a thin shell over the top, creeping further down one side here and there, its ragged edges tucked
    # against the bark; darker at the edges than on the sunny top.
    rings = []
    for i in range(12):
        t = lerp(0.1, 0.84, i / 11)
        p, r, _, d = log.at(t)
        side = d.cross(UP).normalized()
        grow = math.sin(math.pi * i / 11) ** 0.5
        left = -lerp(0.35, 1.3, grow) * rng.uniform(0.7, 1.1)
        right = lerp(0.35, 1.2, grow) * rng.uniform(0.7, 1.1)
        ring = []
        for k in range(5):
            a = lerp(left, right, k / 4)
            n = UP * math.cos(a) + side * math.sin(a)
            lift = 1.0 if k in (0, 4) or i in (0, 11) else 1.045
            up_share = max(math.cos(a), 0.0)  # 1 on top of the log, 0 at its sides
            ring.append((plant.vert(p + n * r * lift, 0.0, (n + UP).normalized(), lerp(0.75, 1.0, up_share)),
                         swatch('Moss', 0.25 if lift == 1.0 else lerp(0.45, 0.9, up_share), k / 4)))
        rings.append(ring)
    for i in range(len(rings) - 1):
        for k in range(4):
            quad = (rings[i][k], rings[i][k + 1], rings[i + 1][k + 1], rings[i + 1][k])
            plant.face([v for v, _ in quad], [uv for _, uv in quad], moss)

    hull_points = []
    for p, r in zip(log.points, log.radii):
        hull_points += cylinder_points(p - Vector((0.005, 0.0, 0.0)), p + Vector((0.005, 0.0, 0.0)), r * 1.02)
    return finish(plant, hull_points, ao_distance=0.5, ao_blend=0.4, lods=WOOD_LODS[0], lod_screens=WOOD_LODS[1])


if __name__ == '__main__':
    models = [bush_a(), bush_b(), bush_c(), fern_a(), reeds_a(), stump_a(), log_a()]
    report(models)
    preview_all(models, view=(-0.8, -1.6, 0.55), fit=0.85)
