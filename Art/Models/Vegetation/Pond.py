"""Pond plants for the tutorial island (Docs/TutorialIsland.md): LilyPads_A and Reeds_B. Scripted models (Art/README.md)
built with Tools/Blender/looter_plants.py, on the FoliagePalette like the other ground cover: opaque leaves and petals
UV-mapped onto its swatches. Walk-through (no collision), no Nanite, the plant LODs.

  LilyPads_A  a floating cluster about 1.5 m across: seven notched pads in three greens (one yellowing), two white
              water lilies with yellow middles and a closed bud. Flat at z = 0 (the water's surface, the origin at
              the cluster's middle), each pad a few millimeters higher than the last so overlaps don't flicker.
  Reeds_B     a flag-iris sedge clump, 1.2-1.6 m, unlike Reeds_A's thin blades and cattails: fans of broad sword
              leaves, arching a little, a few dry tips, and two yellow flowers on stems. The origin is its foot.

    blender -b --factory-startup --python Art/Models/Vegetation/Pond.py -- --preview [LilyPads_A Reeds_B]
"""
import math
import random

from mathutils import Vector

import looter_textures as lt
from looter_plants import UP, Plant, blade, finish, horizontal, petal, preview_all, report, swatch, up_normal

PLANT_LODS = ('50,20', '0.25,0.08')   # as the fern and Reeds_A: LOD1 from about 5 m, LOD2 from 14 m


def lily_pad(plant, mat, center, radius, turn, color, z, rng):
    """A floating pad: a fan round its middle with the notch where its stem meets it; the rim curls up a little."""
    notch = math.radians(rng.uniform(12.0, 20.0))
    rim = 12
    hub = plant.vert(Vector((center.x, center.y, z + 0.002)), 0.05, UP, 0.95)
    ring = []
    for k in range(rim):
        a = turn + notch * 0.5 + (2.0 * math.pi - notch) * k / (rim - 1)
        r = radius * (1.0 - 0.06 * math.sin(a * 3.0 + turn))
        lift = 0.006 + 0.006 * (0.5 + 0.5 * math.sin(a * 2.0 + rng.uniform(0.0, 6.0)))
        ring.append((plant.vert(Vector((center.x + math.cos(a) * r, center.y + math.sin(a) * r, z + lift)), 0.08, UP, 1.0),
                     k / (rim - 1)))
    for (v0, s0), (v1, s1) in zip(ring, ring[1:]):
        plant.face((hub, v0, v1), [swatch(color, 0.0, 0.5), swatch(color, 1.0, s0), swatch(color, 1.0, s1)], mat)


def water_lily(plant, mat, center, size, turn):
    """An open water lily: eight outer petals lying back, six inner ones cupped up, a yellow middle."""
    for ring, (count, length, width, tilt, offset) in enumerate(((8, 0.07, 0.032, 18.0, 0.0), (6, 0.05, 0.026, 52.0, 0.5))):
        for k in range(count):
            a = turn + 2.0 * math.pi * (k + offset) / count
            out = horizontal(a)
            t = math.radians(tilt)
            direction = (out * math.cos(t) + UP * math.sin(t)).normalized()
            normal = (UP - direction * UP.dot(direction)).normalized()
            petal(plant, mat, center + UP * (0.004 + ring * 0.006), direction, length * size, width * size, 'FlowerWhite',
                  normal, wind=0.15, cup=0.12)
    for k in range(5):
        a = turn + 2.0 * math.pi * k / 5
        direction = (horizontal(a) * 0.4 + UP).normalized()
        normal = (horizontal(a) - direction * horizontal(a).dot(direction)).normalized()
        petal(plant, mat, center + UP * 0.012, direction, 0.024 * size, 0.012 * size, 'FlowerCenter', normal, wind=0.15)


def bud(plant, mat, center, size, turn):
    """A closed bud: four petals folded up into a point."""
    for k in range(4):
        a = turn + 2.0 * math.pi * k / 4
        direction = (horizontal(a) * 0.25 + UP).normalized()
        normal = (horizontal(a) - direction * horizontal(a).dot(direction)).normalized()
        petal(plant, mat, center + UP * 0.004, direction, 0.06 * size, 0.035 * size, 'FlowerWhite', normal, wind=0.15,
              cup=-0.1)


def lily_pads_a():
    rng = random.Random(311)
    plant = Plant('LilyPads_A', 311)
    mat = lt.material('FoliagePalette')
    pads = [((0.0, 0.0), 0.22, 'Clover'), ((0.36, 0.14), 0.18, 'CloverDark'), ((-0.3, 0.22), 0.2, 'Clover'),
            ((-0.18, -0.34), 0.17, 'GrassDeep'), ((0.24, -0.3), 0.15, 'CloverDark'), ((-0.55, -0.05), 0.13, 'GrassYellow'),
            ((0.6, -0.12), 0.12, 'Clover')]
    for k, ((x, y), r, color) in enumerate(pads):
        lily_pad(plant, mat, Vector((x, y, 0.0)), r, rng.uniform(0.0, 2.0 * math.pi), color, 0.001 * k, rng)
    water_lily(plant, mat, Vector((0.08, 0.05, 0.012)), 1.0, 0.3)
    water_lily(plant, mat, Vector((-0.28, -0.3, 0.012)), 0.8, 1.1)
    bud(plant, mat, Vector((0.33, 0.16, 0.012)), 0.9, 0.4)
    return finish(plant, None, ao_blend=1.0, lods=PLANT_LODS[0], lod_screens=PLANT_LODS[1])


def iris_flower(plant, mat, top, size, turn):
    """A yellow flag iris: three falls drooping out and three standards held up."""
    for k in range(3):
        a = turn + 2.0 * math.pi * k / 3
        fall = (horizontal(a) - UP * 0.35).normalized()
        petal(plant, mat, top, fall, 0.07 * size, 0.045 * size, 'FlowerYellow', (UP - fall * UP.dot(fall)).normalized(),
              wind=1.0, cup=0.15, round=True)
        stand = (horizontal(a + math.pi / 3.0) * 0.35 + UP).normalized()
        out = horizontal(a + math.pi / 3.0)
        petal(plant, mat, top, stand, 0.05 * size, 0.03 * size, 'FlowerYellow',
              (out - stand * out.dot(stand)).normalized(), wind=1.0, cup=0.1)


def reeds_b():
    rng = random.Random(321)
    plant = Plant('Reeds_B', 321)
    mat = lt.material('FoliagePalette')
    shade = up_normal(Vector((0.0, 0.0, 0.0)), 0.35, 0.3)
    # Fans: each a few sword leaves in one plane, overlapping at the foot, the outer ones arching out.
    for f in range(6):
        a = 2.0 * math.pi * f / 6 + rng.uniform(-0.3, 0.3)
        base = horizontal(a + 1.3, rng.uniform(0.03, 0.14)) - UP * 0.04
        fan_dir = horizontal(a)
        for k in range(5):
            spread = (k - 2) * 0.22
            heading = horizontal(a + spread)
            height = rng.uniform(1.15, 1.55) * (1.0 - 0.12 * abs(k - 2))
            lean = 0.08 + 0.12 * abs(k - 2) + rng.uniform(0.0, 0.08)
            color = rng.choice(['Fern', 'GrassDeep', 'GrassFresh', 'GrassDeep', 'Fern', 'GrassOlive'])
            blade(plant, mat, base + fan_dir * (0.008 * (k - 2)), heading, height, rng.uniform(0.05, 0.066), lean, color,
                  rng, 3, shade, twist=0.15, taper=0.25, tip='DryTan' if rng.random() < 0.15 else None)
    # Two flower stems standing out of the fans.
    for k, (a, h) in enumerate(((0.6, 1.35), (3.5, 1.22))):
        root = horizontal(a, 0.06) - UP * 0.04
        top = blade(plant, mat, root, horizontal(a), h, 0.012, 0.05, 'Stem', rng, 2, shade, taper=0.3, wind=(0.0, 0.9))
        iris_flower(plant, mat, top, 1.4, a + k)
    return finish(plant, None, ao_blend=1.0, lods=PLANT_LODS[0], lod_screens=PLANT_LODS[1])


models = [lily_pads_a(), reeds_b()]
report(models)
preview_all(models, view=(-0.8, -1.6, 0.9), fit=0.8)
