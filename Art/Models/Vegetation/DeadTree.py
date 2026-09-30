"""DeadTree_A (Docs/TutorialIsland.md): a gnarled, bare dead tree whose limbs fork upward like a candelabra, some
snapped off, its top broken. A scripted model (Art/README.md) built with Tools/Blender/looter_plants.py: the trunk's
foot is the origin, a convex hull around the lower trunk is the collision, no Nanite (the importer makes LODs).

Branches grow recursively: each wanders with a few sharp elbows, forks two or three times, and ends in a point or a
splintered break.

    blender -b --factory-startup --python Art/Models/Vegetation/DeadTree.py -- --preview
"""
import math
import random

from mathutils import Vector

import looter_textures as lt
from looter_plants import (UP, Limb, Plant, bezier, cylinder_points, finish, gnarl, lerp, perpendicular, preview_all,
                           random_unit, report, root_flare)


def wander(rng, start, direction, length, step=0.35, kink=0.3, elbows=1, rise=0.08):
    """A crooked centerline: it wanders a little every step and turns sharply at a few elbows."""
    count = max(3, int(math.ceil(length / step)))
    sharp = set(rng.sample(range(1, count), min(elbows, count - 1)))
    d = direction.normalized()
    points = [start]
    for i in range(count):
        turn = kink * (2.2 if i in sharp else 0.6)
        d = (d + random_unit(rng) * turn + UP * rise).normalized()
        points.append(points[-1] + d * (length / count))
    return points


def grow(plant, rng, bark, start, direction, length, radius, level, wind, sides=(6, 5, 3, 3)):
    """A branch and, recursively, its forks. Deeper levels are shorter and thinner; some snap off."""
    last = level >= 3
    broken = level >= 2 and rng.random() < (0.25 if level < 3 else 0.15)
    points = wander(rng, start, direction, length, 0.3 if level < 2 else 0.35, 0.16 + 0.06 * level,
                    2 if level < 2 else 1)
    limb = Limb(points, radius, radius * 0.45 if broken else 0.0, sides[min(level, 3)], (wind, lerp(wind, 0.8, 0.5)))
    plant.tube(limb, bark)
    if last or broken:
        return
    for _ in range(rng.choice((2, 2, 3))):
        t = rng.uniform(0.4, 0.9)
        p, r, w, d = limb.at(t)
        axis = perpendicular(d).normalized()
        angle = math.radians(rng.uniform(28.0, 55.0))
        spin = rng.uniform(0.0, 2.0 * math.pi)
        axis = axis * math.cos(spin) + d.cross(axis) * math.sin(spin)
        child = d * math.cos(angle) + axis.cross(d) * math.sin(angle) + UP * 0.35
        grow(plant, rng, bark, p, child, length * rng.uniform(0.5, 0.68) * (1.0 - t * 0.3), r * rng.uniform(0.55, 0.7),
             level + 1, w, sides)


def dead_tree(name, seed, height, radius, lean, limbs, hull_height=3.0):
    rng = random.Random(seed)
    plant = Plant(name, seed)
    bark = lt.material('BarkOak')

    # A leaning, twisted trunk with a snapped-off top.
    base = Vector((0.0, 0.0, -0.35))
    top = Vector((lean[0], lean[1], height))
    params = [0.0, 0.04, 0.09, 0.15, 0.23] + [0.23 + 0.77 * i / 10 for i in range(1, 11)]
    points = gnarl(bezier(base, base + UP * height * 0.4, top - UP * height * 0.3 + Vector((lean[0], -lean[1], 0.0)),
                          top, params), 0.35, 0.45, seed)
    trunk = Limb(points, radius, radius * 0.38, 9, (0.0, 0.3), taper=0.8)
    plant.tube(trunk, bark, root_flare(0.9, 5, rng.uniform(0.0, 6.28), 1.0))

    for i in range(limbs):
        t = lerp(0.45, 0.88, i / max(limbs - 1, 1)) + rng.uniform(-0.04, 0.04)
        p, r, w, d = trunk.at(t)
        a = i * 2.4 + rng.uniform(-0.4, 0.4)
        out = Vector((math.cos(a), math.sin(a), 0.0))
        grow(plant, rng, bark, p, out * 0.8 + UP * rng.uniform(0.6, 1.1), height * rng.uniform(0.5, 0.65),
             r * rng.uniform(0.65, 0.78), 1, w)
    # One stout limb snapped off low on the trunk.
    p, r, w, d = trunk.at(0.3)
    out = Vector((math.cos(1.0), math.sin(1.0), 0.15))
    plant.tube(Limb([p, p + out * 0.35, p + out * 0.7 + UP * 0.05], r * 0.6, r * 0.45, 6, (w, w)), bark)

    hull_points = []
    for p, r in zip(trunk.points, trunk.radii):
        if 0.0 <= p.z <= hull_height:
            hull_points += cylinder_points(p, p + UP * 0.01, r * 1.05)
    return finish(plant, hull_points, ao_distance=1.0, ao_blend=0.3)


def dead_tree_a():
    """About 6.5 m: a leaning trunk broken at 5 m, four crooked limbs reaching up past it."""
    return dead_tree('DeadTree_A', 71, height=5.0, radius=0.34, lean=(0.35, 0.2), limbs=5)


if __name__ == '__main__':
    models = [dead_tree_a()]
    report(models)
    preview_all(models)
