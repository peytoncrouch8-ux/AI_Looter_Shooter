"""The island's conifers (Docs/TutorialIsland.md): Pine_A, a full cone of a pine, and Pine_B, taller and ragged, its
lower trunk bare but for a few dead stubs. Scripted models (Art/README.md), built with Tools/Blender/looter_plants.py:
the trunk's foot is the origin, one convex hull around the lower trunk is the collision, and there are no Nanite
meshes (the importer gives them classic LODs).

A pine is a straight tapering trunk with whorls of branches every metre or so. Each branch carries needle-cluster
cards lying along it, nearly flat with a roof-like fold, so the whorls stack into layered tiers. The cards shade with
normals leaning out of the cone and up, so the tree lights as one soft cone.

    blender -b --factory-startup --python Art/Models/Vegetation/Pines.py -- --preview [Pine_A ...]
"""
import math
import random

from mathutils import Vector

import looter_textures as lt
from looter_plants import (UP, Limb, Plant, bezier, crown_shade, cylinder_points, finish, gnarl, horizontal, lerp,
                           preview_all, report, root_flare, smoothstep)

GOLDEN_ANGLE = math.pi * (3.0 - math.sqrt(5.0))


def rotate_about(v, axis, angle):
    """v turned by angle (radians) about axis (Rodrigues)."""
    axis = axis.normalized()
    return v * math.cos(angle) + axis.cross(v) * math.sin(angle) + axis * axis.dot(v) * (1.0 - math.cos(angle))


def conifer(name, seed, height, radius, crown_base, crown_radius, whorl_gap, per_whorl, card_size, lean=(0.0, 0.0),
            droop=0.25, gaps=0.0, cards_per_meter=1.3, dead_stubs=0, flare=0.5, hull_height=3.0, lods=None,
            lod_screens=None):
    """A conifer: a trunk of height meters, whorls of branches from crown_base up whose length follows a cone of
    crown_radius at the bottom, needle cards along each branch. gaps is the chance a branch is missing (a ragged
    crown); dead_stubs adds bare broken branches below the crown."""
    rng = random.Random(seed)
    plant = Plant(name, seed)
    bark = lt.material('BarkPine')
    needles = lt.material('NeedlesPine')

    base = Vector((0.0, 0.0, -0.35))
    top = Vector((lean[0], lean[1], height))
    params = [0.0, 0.03, 0.07, 0.12] + [0.12 + 0.88 * i / 18 for i in range(1, 19)]
    points = bezier(base, base + UP * height * 0.35, top - UP * height * 0.35 + Vector((lean[0], lean[1], 0.0)) * 0.3,
                    top, params)
    points = gnarl(points, 0.1, 0.3, seed)
    trunk = Limb(points, radius, 0.0, 8, (0.0, 0.4), taper=0.9)

    span = height - crown_base

    def cone_radius(z):
        return crown_radius * max((height - z) / span, 0.0) ** 0.85

    def occlusion(p):
        axis = trunk.at(trunk.param_at_height(p.z))[0]
        across = math.hypot(p.x - axis.x, p.y - axis.y) / max(cone_radius(p.z), 0.35)
        depth = lerp(0.45, 1.0, smoothstep(0.1, 0.9, across))
        return depth * lerp(0.72, 1.0, smoothstep(crown_base - 1.0, crown_base + span * 0.35, p.z))

    plant.tube(trunk, bark, root_flare(flare, 5, rng.uniform(0.0, 6.28), 0.7), occlusion)
    crown_mid = Vector((lean[0] * 0.5, lean[1] * 0.5, crown_base + span * 0.4))

    def branch(start, direction, length, trunk_r, trunk_wind, f):
        """One branch from start: it sags in the middle, like a spruce's, and carries a fan of needle cards that
        slope down and out like a skirt (steeper low on the tree), so the tiers read from the side too."""
        p1 = start + direction * length * 0.35
        p2 = start + direction * length * 0.7 - UP * length * droop * 0.6
        p3 = start + direction * length - UP * length * droop * 0.3
        limb = Limb(bezier(start, p1, p2, p3, [0.0, 0.5, 1.0] if length < 1.5 else [0.0, 0.35, 0.7, 1.0]),
                    max(trunk_r * 0.32, 0.018), 0.0, 3, (trunk_wind, 0.6))
        plant.tube(limb, bark, occlusion=occlusion)
        shade = crown_shade(start - UP * 0.6, crown_mid, 0.45)
        count = max(2, int(round(length * cards_per_meter)))
        size = lerp(card_size[0], card_size[1], min(length / (crown_radius * 0.8), 1.0))
        slope = lerp(0.6, 0.2, f)
        heading = math.atan2(direction.y, direction.x)
        for i in range(count):
            t = lerp(0.12, 0.72, i / max(count - 1, 1))
            p, _, w, d = limb.at(t)
            s = size * rng.uniform(0.85, 1.1)
            yaw = heading + (0.45 if i % 2 else -0.45) * rng.uniform(0.6, 1.2) * (1.0 - t * 0.5)
            up = (horizontal(yaw) - UP * slope * rng.uniform(0.8, 1.2)).normalized()
            facing = rotate_about(UP - up * up.z, up, rng.uniform(-0.35, 0.35))
            plant.card(p - up * s * 0.15, facing, up, s * rng.uniform(0.8, 0.95), s, needles, rng.randrange(4), shade,
                       occlusion, (w, 1.0), 0.18, 0.15, rng.random() < 0.5, 2)

    # Whorls from the crown's base to near the top, closer together as the tree narrows.
    z = crown_base
    turn = rng.uniform(0.0, 2.0 * math.pi)
    while z < height - 0.8:
        f = (z - crown_base) / span
        start, trunk_r, trunk_wind, _ = trunk.at(trunk.param_at_height(z))
        turn += GOLDEN_ANGLE
        count = per_whorl + rng.choice((-1, 0, 0, 1))
        for j in range(count):
            if rng.random() < gaps * (1.0 - f):
                continue
            a = turn + 2.0 * math.pi * j / count + rng.uniform(-0.3, 0.3)
            pitch = lerp(-0.35, 0.4, f) + rng.uniform(-0.1, 0.1)
            direction = horizontal(a) * math.cos(pitch) + UP * math.sin(pitch)
            length = cone_radius(z) * rng.uniform(0.85, 1.1) + 0.25
            branch(start, direction, length, trunk_r, trunk_wind, f)
        z += whorl_gap * lerp(1.0, 0.55, f) * rng.uniform(0.85, 1.15)

    # The leader's tuft: a few cards standing up around the tip.
    tip_shade = crown_shade(top - UP * 1.2, crown_mid, 0.5)
    for j in range(3):
        a = turn + 2.0 * math.pi * j / 3
        s = card_size[0] * 1.1
        plant.card(top - UP * (s * 0.85), horizontal(a), UP + horizontal(a + 1.2, 0.25), s * 0.7, s, needles,
                   rng.randrange(4), tip_shade, occlusion, (0.7, 1.0), 0.15, 0.0, j == 1, 2)

    # Dead stubs on the bare lower trunk: short broken branches, no needles.
    for _ in range(dead_stubs):
        z = rng.uniform(1.4, crown_base - 0.3)
        start, trunk_r, trunk_wind, _ = trunk.at(trunk.param_at_height(z))
        direction = horizontal(rng.uniform(0.0, 2.0 * math.pi)) + UP * rng.uniform(-0.35, 0.05)
        length = rng.uniform(0.3, 0.9)
        stub = Limb([start, start + direction.normalized() * length * 0.5, start + direction.normalized() * length
                     - UP * length * 0.1], max(trunk_r * 0.2, 0.02), 0.0, 3, (trunk_wind, 0.3))
        plant.tube(stub, bark, occlusion=occlusion)

    hull_points = []
    for p, r in zip(trunk.points, trunk.radii):
        if 0.0 <= p.z <= hull_height:
            hull_points += cylinder_points(p, p + UP * 0.01, r * 1.05)
    return finish(plant, hull_points, ao_distance=1.2, ao_blend=0.6, lods=lods, lod_screens=lod_screens)


def pine_a():
    """A full, dense cone of a pine, branched almost to the ground, about 11.5 m."""
    return conifer('Pine_A', 61, height=11.5, radius=0.27, crown_base=1.5, crown_radius=3.1, whorl_gap=0.95,
                   per_whorl=6, card_size=(1.1, 1.9), droop=0.22, cards_per_meter=2.4)


def pine_b():
    """A tall, ragged pine: bare lower trunk with dead stubs, uneven whorls, a slight lean, about 13.5 m."""
    return conifer('Pine_B', 67, height=13.5, radius=0.31, crown_base=4.2, crown_radius=2.9, whorl_gap=1.05,
                   per_whorl=6, card_size=(1.1, 1.9), lean=(0.4, 0.2), droop=0.3, gaps=0.22, dead_stubs=5,
                   cards_per_meter=2.4, hull_height=3.5)


if __name__ == '__main__':
    models = [pine_a(), pine_b()]
    report(models)
    preview_all(models)
