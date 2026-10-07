"""Ground cover for the PCG scatter (Docs/TutorialIsland.md), sized to cover a meadow at about one model per 0.8 m²:
GrassClump_A, GrassClump_B, GrassClump_C (grass patches 70-120 cm across, 25-55 cm tall), TallGrass_A (a loose patch,
80-110 cm tall), Flowers_Yellow, Flowers_White, Flowers_Purple (drifts of flowers in grass, 60-80 cm across) and
Clover_A (a mat, about 70 cm across). For Ransom's Rest, blue larkspur along its fences (Docs/Areas/RansomsRest.md):
Larkspur_A (a clump of five spikes 60-90 cm tall) and Larkspur_B (four spikes strung along 1.2 m of X, for a fence's
foot), placed along fence lines rather than by the meadow scatter. Scripted models (Art/README.md) built with
Tools/Blender/looter_plants.py; the patch's middle, at ground level, is the origin.

A patch is a few tufts spread over it rather than one clump, so neighbouring patches blend into an uneven meadow.
Everything is opaque geometry on one material, FoliagePalette: blades, petals and leaflets are UV-mapped onto its
gradient swatches (lt.PALETTE; U runs from a part's root to its tip), so there is no alpha and next to no overdraw,
150-300 triangles a patch. Wind and occlusion run from each root to its tip. Shading normals lean up, out of each
tuft's middle, so the grass lights like the ground it stands on with a little roundness. There is no collision.

    blender -b --factory-startup --python Art/Models/Vegetation/GroundCover.py -- --preview [GrassClump_A ...]
renders each model, and with --patch Saved/ArtPreviews/Vegetation/groundcover.png: a piece of meadow scattered at
that density, a person-height figure for scale.
"""
import math
import random
import sys

import bmesh
import bpy
from mathutils import Matrix, Vector

import looter_textures as lt
from looter_plants import UP, Plant, blade, finish, horizontal, lerp, petal, preview_all, report, swatch, up_normal

# One LOD (half the triangles) before the cull distance, from about 11 m for a patch (Unreal's screen size is about
# 1.8 x radius / distance at a 90 degree field of view, and a patch's bounds are about 0.6 m in radius).
LODS = '50'
LOD_SCREENS = '0.1'
GOLDEN_ANGLE = math.pi * (3.0 - math.sqrt(5.0))


def spots(rng, count, radius):
    """count points spread evenly but irregularly over a disc of radius (a jittered golden spiral)."""
    turn = rng.uniform(0.0, 2.0 * math.pi)
    return [horizontal(turn + GOLDEN_ANGLE * i + rng.uniform(-0.35, 0.35),
                       radius * math.sqrt((i + 0.5) / count) * rng.uniform(0.8, 1.1)) for i in range(count)]


def tuft(plant, mat, rng, count, height, width, spread, lean, colors, segments=2, center=Vector((0.0, 0.0, 0.0)),
         tips=None, dry=0.0):
    """A tuft of count blades rooted in a disc of radius spread: taller in the middle, leaning out at the edge.
    colors are swatches picked at random; dry is the share of blades with a tips swatch on their top."""
    radius = spread + height * lean
    shade = up_normal(center, radius)
    for i in range(count):
        r = spread * math.sqrt(rng.random())
        a = rng.uniform(0.0, 2.0 * math.pi)
        root = center + horizontal(a, r) - UP * 0.02
        edge = r / max(spread, 1e-3)
        heading = horizontal(a + rng.uniform(-0.5, 0.5))
        h = height * rng.uniform(0.65, 1.1) * lerp(1.0, 0.75, edge)
        blade(plant, mat, root, heading, h, width * rng.uniform(0.8, 1.2),
              lean * lerp(0.4, 1.3, edge) * rng.uniform(0.7, 1.2), rng.choice(colors), rng, segments, shade,
              tip=tips if rng.random() < dry else None)


def grass_patch(plant, mat, rng, radius, tufts, blades, height, width, lean, colors, segments=3, tips=None, dry=0.0,
                spread=0.11):
    """A patch of grass radius meters across: blades split over tufts spread across it, each tuft a little taller or
    shorter than the next, so the patch's top is uneven."""
    for i, center in enumerate(spots(rng, tufts, radius - spread)):
        count = blades // tufts + (1 if i < blades % tufts else 0)
        tuft(plant, mat, rng, count, height * rng.uniform(0.75, 1.15), width, spread, lean, colors, segments, center,
             tips, dry)


def ground_cover(name, seed, build, lods=LODS, lod_screens=LOD_SCREENS):
    """A ground cover model: build(plant, mat, rng) makes its parts; its occlusion is authored, not baked. A build may
    return the pieces that sway with a stem (see share_phase)."""
    rng = random.Random(seed)
    plant = Plant(name, seed)
    phases = build(plant, lt.material('FoliagePalette'), rng)
    obj = finish(plant, None, ao_blend=1.0, lods=lods, lod_screens=lod_screens)
    if phases:
        share_phase(obj, phases)
    return obj


def share_phase(obj, phases):
    """Gives pieces of mesh their stem's wind phase (vertex color G, a random value per connected piece): phases lists
    (stem's first vertex, first vertex of its pieces, the vertex after them). A floret is a piece of its own, and with a
    phase of its own it would sway out of step with its stem and drift off it."""
    import numpy as np
    mesh = obj.data
    col = mesh.color_attributes['Col']
    values = np.empty(4 * len(mesh.loops), dtype=np.float32)
    col.data.foreach_get('color', values)
    values = values.reshape(-1, 4)
    loop_vert = np.empty(len(mesh.loops), dtype=np.int64)
    mesh.loops.foreach_get('vertex_index', loop_vert)
    first_loop = np.zeros(len(mesh.vertices), dtype=np.int64)
    first_loop[loop_vert[::-1]] = np.arange(len(loop_vert))[::-1]
    owner = np.full(len(mesh.vertices), -1, dtype=np.int64)
    for stem, start, end in phases:
        owner[start:end] = stem
    follows = owner[loop_vert] >= 0
    values[follows, 1] = values[first_loop[owner[loop_vert[follows]]], 1]
    col.data.foreach_set('color', values.ravel())
    mesh.update()


# --- Grass ---

def grass_clump_a():
    """Short, fresh grass: a patch about 80 cm across and 25-30 cm tall, 36 blades."""
    return ground_cover('GrassClump_A', 101, lambda plant, mat, rng: grass_patch(
        plant, mat, rng, 0.4, 9, 36, 0.3, 0.045, 0.55, ['GrassFresh', 'GrassFresh', 'GrassDeep']))


def grass_clump_b():
    """Meadow grass, olive and yellowing with a few dry tips: a patch about a meter across, 35-45 cm tall, 42 blades."""
    return ground_cover('GrassClump_B', 103, lambda plant, mat, rng: grass_patch(
        plant, mat, rng, 0.44, 11, 42, 0.48, 0.042, 0.6, ['GrassOlive', 'GrassFresh', 'GrassYellow'], tips='GrassDry',
        dry=0.3))


def grass_clump_c():
    """Wild grass, arching blades with some straw: a patch about 110 cm across, 45-55 cm tall, 40 blades."""
    return ground_cover('GrassClump_C', 107, lambda plant, mat, rng: grass_patch(
        plant, mat, rng, 0.48, 9, 40, 0.6, 0.045, 0.6, ['GrassDeep', 'GrassOlive', 'GrassFresh', 'Straw'], segments=4,
        tips='GrassDry', dry=0.25))


def tall_grass(plant, mat, rng):
    grass_patch(plant, mat, rng, 0.45, 6, 28, 0.85, 0.036, 0.35, ['GrassOlive', 'GrassYellow', 'GrassFresh'],
                segments=4, tips='GrassDry', dry=0.4, spread=0.12)
    # Seed stalks standing above the blades, each with a slim seed head.
    for spot in spots(rng, 6, 0.22):
        a = rng.uniform(0.0, 2.0 * math.pi)
        top = blade(plant, mat, spot - UP * 0.02, horizontal(a), rng.uniform(0.85, 0.95), 0.013, rng.uniform(0.1, 0.2),
                    'Straw', rng, 2, up_normal(spot, 0.3), taper=0.2, wind=(0.0, 0.95))
        head_dir = (horizontal(a, 0.35) + UP).normalized()
        petal(plant, mat, top - head_dir * 0.01, head_dir, rng.uniform(0.1, 0.14), 0.024, 'Straw',
              horizontal(a + 1.57), occlusion=1.0)


def tall_grass_a():
    """Tall meadow grass: a loose patch about 90 cm across and 80-110 cm tall, with seed stalks."""
    return ground_cover('TallGrass_A', 109, tall_grass)


# --- Flowers: drifts of flowers in short grass ---

def flower_drift(plant, mat, rng, count, height, radius=0.38):
    """Short grass over a drift radius meters across, and count flower stems rising out of it here and there;
    returns each stem's top and the direction its flower faces."""
    grass_patch(plant, mat, rng, radius, 7, 22, 0.22, 0.03, 0.5, ['GrassFresh', 'Clover', 'GrassDeep'], segments=2,
                spread=0.08)
    tops = []
    for spot in spots(rng, count, radius - 0.05):
        a = rng.uniform(0.0, 2.0 * math.pi)
        lean = rng.uniform(0.1, 0.3)
        top = blade(plant, mat, spot - UP * 0.02, horizontal(a), height * rng.uniform(0.8, 1.1), 0.008, lean, 'Stem',
                    rng, 2, up_normal(spot, 0.3), taper=0.1, wind=(0.0, 0.95))
        tops.append((top, (horizontal(a, lean * 0.8) + UP).normalized()))
    return tops


def flower_head(plant, mat, rng, top, facing, petals, length, width, color, cup=0.0, center=None, round=False):
    """A flower facing up-ish: petals around top, and optionally a raised center of another swatch."""
    side = facing.cross(Vector((1.0, 0.0, 0.0)) if abs(facing.x) < 0.9 else UP).normalized()
    other = facing.cross(side)
    turn = rng.uniform(0.0, 2.0 * math.pi)
    for i in range(petals):
        a = turn + 2.0 * math.pi * i / petals
        d = side * math.cos(a) + other * math.sin(a)
        petal(plant, mat, top, d, length * rng.uniform(0.9, 1.1), width, color, facing, cup=cup, round=round)
    if center:
        r = length * 0.3
        ring = [top + (side * math.cos(2 * math.pi * k / 5) + other * math.sin(2 * math.pi * k / 5)) * r
                + facing * 0.004 for k in range(5)]
        hub = plant.vert(top + facing * r * 0.5, 1.0, facing, 1.0)
        rim = [plant.vert(p, 1.0, facing, 0.95) for p in ring]
        for k in range(5):
            plant.face((hub, rim[k], rim[(k + 1) % 5]), [swatch(center, 0.2, 0.5), swatch(center, 0.9, 0.0),
                                                         swatch(center, 0.9, 1.0)], mat)


def flowers_yellow():
    """Buttercups: seven glossy yellow cups over short grass, a drift about 70 cm across, 35-45 cm tall."""
    def build(plant, mat, rng):
        for top, facing in flower_drift(plant, mat, rng, 7, 0.4):
            flower_head(plant, mat, rng, top, facing, 5, 0.042, 0.038, 'FlowerYellow', cup=0.45, round=True)
    return ground_cover('Flowers_Yellow', 113, build)


def flowers_white():
    """Ox-eye daisies: six white-rayed flowers with yellow middles over short grass, about 70 cm across, 45-55 cm."""
    def build(plant, mat, rng):
        for top, facing in flower_drift(plant, mat, rng, 6, 0.5):
            flower_head(plant, mat, rng, top, facing, 9, 0.048, 0.017, 'FlowerWhite', cup=0.1, center='FlowerCenter')
    return ground_cover('Flowers_White', 127, build)


def flowers_purple():
    """Purple spikes (wild lupine or vetch) over short grass: six stems, about 70 cm across, 45-55 cm tall."""
    def build(plant, mat, rng):
        for top, facing in flower_drift(plant, mat, rng, 6, 0.5):
            # The spike: three tiers of four small petals up the stem's tip, getting smaller toward it.
            a = math.atan2(facing.y, facing.x)
            for tier in range(3):
                p = top - facing * (0.1 - tier * 0.04)
                size = 0.038 * (1.0 - tier * 0.25)
                for k in range(4):
                    d = (horizontal(a + k * math.pi * 0.5 + tier * 0.7) + facing * 0.6).normalized()
                    petal(plant, mat, p, d, size, size * 0.8, 'FlowerPurple', (d + UP).normalized(), cup=0.3)
    return ground_cover('Flowers_Purple', 131, build)


# --- Clover ---

def clover_a():
    """A clover mat about 70 cm across: trefoil leaves in a few clusters, low to the ground, and three white heads."""
    def build(plant, mat, rng):
        for cluster in spots(rng, 6, 0.33):
            shade = up_normal(cluster, 0.15, 0.3)
            for _ in range(4):
                a = rng.uniform(0.0, 2.0 * math.pi)
                r = 0.1 * math.sqrt(rng.random())
                height = rng.uniform(0.025, 0.06) * lerp(1.2, 0.8, r / 0.1)
                center = cluster + horizontal(a, r) + UP * height
                color = rng.choice(['Clover', 'Clover', 'CloverDark'])
                turn = rng.uniform(0.0, 2.0 * math.pi)
                normal = (UP + horizontal(a, 0.3)).normalized()
                for k in range(3):
                    d = horizontal(turn + k * 2.0 * math.pi / 3)
                    petal(plant, mat, center, d - normal * normal.dot(d), rng.uniform(0.045, 0.055), 0.045, color,
                          normal, wind=0.8, occlusion=lerp(0.7, 1.0, height / 0.07), cup=-0.15, round=True)
                # The leaf's stalk, a sliver down to the ground.
                blade(plant, mat, cluster + horizontal(a, r * 0.7) - UP * 0.01, horizontal(a), height + 0.01, 0.006,
                      0.3, 'Stem', rng, 1, shade, taper=0.2, wind=(0.0, 0.8))
        for spot in spots(rng, 3, 0.3):
            top = blade(plant, mat, spot - UP * 0.01, horizontal(rng.uniform(0.0, 6.28)), 0.14, 0.006, 0.2, 'Stem', rng,
                        2, up_normal(spot, 0.15, 0.3), taper=0.2, wind=(0.0, 0.9))
            flower_head(plant, mat, rng, top, UP, 7, 0.02, 0.013, 'FlowerWhite', cup=0.9)
    return ground_cover('Clover_A', 137, build)


# --- Larkspur: Ransom's Rest's blue accent along its fences (Docs/Areas/RansomsRest.md, "The palette") ---

# A thin line of blue along a fence has to hold from 20 to 40 m, and a reduction may thin the small florets, so larkspur
# keeps its whole mesh to 35-45 m (screen size 0.03; the bounds radii are 0.6 and 0.78 m) and halves past it.
LARKSPUR_LODS = '50'
LARKSPUR_SCREENS = '0.03'
LARKSPUR_GRASS = ['GrassYellow', 'GrassDry', 'Straw', 'GrassOlive']


def floret(plant, mat, rng, center, facing, radius, color, wind, normal, tip=0.9, occlusion=1.0):
    """A larkspur floret as a cup open toward facing, four triangles round a sunken throat: the long top sepal (over
    the spur), two side sepals a little below level, and the short lower pair as one point. Its swatch runs from the
    throat (U 0.4, a little darker) to the sepal tips (U tip)."""
    side = UP.cross(facing)
    side = side.normalized() if side.length > 1e-6 else Vector((1.0, 0.0, 0.0))
    up = facing.cross(side)
    turn = rng.uniform(-0.35, 0.35)
    hub = plant.vert(center, wind, normal, occlusion * 0.75)
    rim = []
    for k, (a, reach) in enumerate(((90.0, 1.35), (195.0, 1.05), (270.0, 0.8), (345.0, 1.05))):
        a = math.radians(a + rng.uniform(-12.0, 12.0)) + turn
        reach *= rng.uniform(0.9, 1.1)
        rim.append(plant.vert(center + (side * math.cos(a) + up * math.sin(a)) * radius * reach + facing * radius * 0.5,
                              wind, normal, occlusion))
    for k in range(4):
        k1 = (k + 1) % 4
        plant.face((hub, rim[k], rim[k1]), [swatch(color, 0.4, 0.5), swatch(color, tip, 0.12 + 0.25 * k),
                                            swatch(color, tip, 0.12 + 0.25 * k1)], mat)


def bud(plant, mat, center, out, axis, length, color, wind, normal, occlusion=0.9):
    """A closed bud hugging the stem: a pointed oval along axis, two triangles facing out."""
    side = axis.cross(out).normalized() * length * 0.2
    swell = center + axis * length * 0.25 + out * length * 0.12
    bottom = plant.vert(center - axis * length * 0.3, wind, normal, occlusion * 0.85)
    right = plant.vert(swell + side, wind, normal, occlusion)
    top = plant.vert(center + axis * length * 0.7, wind, normal, occlusion)
    left = plant.vert(swell - side, wind, normal, occlusion)
    plant.face((bottom, right, top), [swatch(color, 0.05, 0.5), swatch(color, 0.3, 0.9), swatch(color, 0.55, 0.5)], mat)
    plant.face((bottom, top, left), [swatch(color, 0.05, 0.5), swatch(color, 0.55, 0.5), swatch(color, 0.3, 0.1)], mat)


def raceme_core(plant, mat, at, axis, sway, shade, start, end, width, turn):
    """The spike's shaded inner florets: two narrow crossed diamonds along it from start to end (stem parameters),
    widest a third of the way up, a deeper, shaded blue, mostly hidden by the florets. Up close they fill the gaps
    between the florets with shade; from afar they keep the spike a solid streak, and they are big enough to outlast a
    reduced LOD."""
    middle = lerp(start, end, 0.35)
    for k in range(2):
        out = horizontal(turn + k * math.pi * 0.5)
        out = (out - axis(middle) * out.dot(axis(middle))).normalized()
        normal = (shade(at(middle)) + out * 0.5).normalized()
        bottom = plant.vert(at(start), sway(start), normal, 0.4)
        left = plant.vert(at(middle) - out * width * 0.5, sway(middle), normal, 0.5)
        top = plant.vert(at(end), sway(end), normal, 0.6)
        right = plant.vert(at(middle) + out * width * 0.5, sway(middle), normal, 0.5)
        plant.face((bottom, right, top), [swatch('FlowerBlue', 0.2, 0.5), swatch('FlowerBlue', 0.5, 0.9),
                                          swatch('FlowerBlue', 0.35, 0.5)], mat)
        plant.face((bottom, top, left), [swatch('FlowerBlue', 0.2, 0.5), swatch('FlowerBlue', 0.35, 0.5),
                                         swatch('FlowerBlue', 0.5, 0.1)], mat)


def larkspur_spike(plant, mat, rng, root, heading, height, lean, shade, florets, phases, leaf=False):
    """One larkspur stem rising height meters from root: slender and bare below (a three-lobed leaf low on it with
    leaf), its top 40-45% a close spike of open florets round a shaded core, facing out a golden angle apart, smaller
    toward the top: pale blue, the last one opening violet, ending in two dark violet buds. Each floret takes the stem's
    wind weight where it sits, and (phases) the stem's wind phase, so the spike sways as one."""
    first = len(plant.winds)
    blade(plant, mat, root, heading, height, 0.009, lean, 'Stem', rng, 3, shade, taper=0.15, wind=(0.0, 0.95),
          occlusion=(0.6, 1.0))
    pieces = len(plant.winds)
    flat = Vector((heading.x, heading.y, 0.0)).normalized()

    def at(t):
        """The stem's centerline, bent as blade() bends it."""
        return root + flat * (lean * height * t * t) + UP * (height * t * (1.0 - 0.35 * lean * lean * t))

    def axis(t):
        return (flat * (2.0 * lean * height * t) + UP * (height * (1.0 - 0.7 * lean * lean * t))).normalized()

    def sway(t):
        return 0.95 * t ** 1.5

    buds = 2
    count = florets + buds
    start = rng.uniform(0.55, 0.6)
    turn = rng.uniform(0.0, 2.0 * math.pi)
    raceme_core(plant, mat, at, axis, sway, shade, start + 0.03, 0.9, 0.022, turn + 0.4)
    for i in range(count):
        s = i / (count - 1)
        t = start + (0.975 - start) * s ** 0.9
        along = axis(t)
        out = horizontal(turn + GOLDEN_ANGLE * i)
        out = (out - along * out.dot(along)).normalized()
        if i < florets:
            # Florets are drawn a little large (stylized), so a spike still shows as a streak of color at 30 m. They
            # shade out from the spike as much as up, so the low sun catches the side facing it.
            radius = lerp(0.027, 0.019, s) * rng.uniform(0.9, 1.1)
            facing = (out + along * 0.25).normalized()
            center = at(t) + out * (0.007 + radius * 0.55)
            purple = i == florets - 1
            floret(plant, mat, rng, center, facing, radius, 'FlowerPurple' if purple else 'FlowerBlue', sway(t),
                   (shade(center) + facing * 0.8).normalized(), tip=0.3 if purple else 0.95)
        else:
            center = at(t) + out * 0.006
            bud(plant, mat, center, out, along, lerp(0.026, 0.018, (i - florets) / (buds - 1)), 'FlowerPurple',
                sway(t), (shade(center) + out * 0.4).normalized())
    if leaf:
        t = rng.uniform(0.12, 0.3)
        base = at(t)
        a = rng.uniform(0.0, 2.0 * math.pi)
        size = rng.uniform(0.07, 0.09)
        for k in (-1, 0, 1):
            d = (horizontal(a + k * 0.55) + UP * rng.uniform(0.5, 0.8)).normalized()
            petal(plant, mat, base, d, size * (1.0 if k == 0 else 0.8), size * 0.16, 'Stem',
                  (UP - d * d.z).normalized(), wind=sway(t), occlusion=0.8)
    phases.append((first, pieces, len(plant.winds)))


def larkspur_a():
    """Blue larkspur, a clump: five spikes 60-90 cm tall, rooted a hand apart and leaning a little apart, over a
    little golden grass about 60 cm across. The spikes rise above the meadow's grass, so their color shows over it
    from afar."""
    def build(plant, mat, rng):
        grass_patch(plant, mat, rng, 0.32, 5, 15, 0.34, 0.032, 0.55, LARKSPUR_GRASS, segments=2, tips='Straw',
                    dry=0.35, spread=0.09)
        phases = []
        heights = [0.9, 0.84, 0.77, 0.7, 0.63]
        rng.shuffle(heights)
        for i, (spot, height) in enumerate(zip(spots(rng, 5, 0.15), heights)):
            a = rng.uniform(0.0, 2.0 * math.pi)
            heading = (spot.normalized() if spot.length > 1e-3 else horizontal(a)) + horizontal(a, 0.6)
            larkspur_spike(plant, mat, rng, spot - UP * 0.02, heading, height * rng.uniform(0.95, 1.05),
                           rng.uniform(0.04, 0.1), up_normal(Vector((0.0, 0.0, 0.0)), 0.3),
                           int(round(lerp(7, 10, (height - 0.63) / 0.27))), phases, leaf=i == 0)
        return phases
    return ground_cover('Larkspur_A', 139, build, LARKSPUR_LODS, LARKSPUR_SCREENS)


def larkspur_b():
    """Blue larkspur in a strip for a fence's foot: four spikes 60-90 cm tall strung along 1.1 m of X, in short golden
    grass along 1.2 m (about 50 cm wide). Turn X along the fence."""
    def build(plant, mat, rng):
        for k in range(6):
            center = Vector((lerp(-0.6, 0.6, (k + rng.uniform(0.2, 0.8)) / 6.0), rng.uniform(-0.09, 0.09), 0.0))
            tuft(plant, mat, rng, 3, rng.uniform(0.28, 0.4), 0.032, 0.08, 0.55, LARKSPUR_GRASS, 2, center,
                 tips='Straw', dry=0.35)
        phases = []
        heights = [0.88, 0.79, 0.7, 0.62]
        rng.shuffle(heights)
        for k, height in enumerate(heights):
            x = lerp(-0.55, 0.55, (k + rng.uniform(0.15, 0.85)) / 4.0)
            root = Vector((x, rng.uniform(-0.08, 0.08), -0.02))
            larkspur_spike(plant, mat, rng, root, horizontal(rng.uniform(0.0, 2.0 * math.pi)),
                           height * rng.uniform(0.95, 1.05), rng.uniform(0.03, 0.09),
                           up_normal(Vector((x, 0.0, 0.0)), 0.3), int(round(lerp(7, 10, (height - 0.62) / 0.26))),
                           phases, leaf=k == 1)
        return phases
    return ground_cover('Larkspur_B', 149, build, LARKSPUR_LODS, LARKSPUR_SCREENS)


# --- The meadow preview ---

def person(height=1.8):
    """A plain figure of a person's height for scale in previews (never exported: only preview runs make it)."""
    bm = bmesh.new()
    bmesh.ops.create_cone(bm, cap_ends=True, segments=12, radius1=0.2, radius2=0.18, depth=height - 0.28,
                          matrix=Matrix.Translation((0.0, 0.0, (height - 0.28) * 0.5)))
    bmesh.ops.create_uvsphere(bm, u_segments=12, v_segments=8, radius=0.13,
                              matrix=Matrix.Translation((0.0, 0.0, height - 0.14)))
    mesh = bpy.data.meshes.new('ReferencePerson')
    bm.to_mesh(mesh)
    bm.free()
    for polygon in mesh.polygons:
        polygon.use_smooth = True
    mat = bpy.data.materials.new('ReferencePerson')
    mat.diffuse_color = (0.45, 0.45, 0.47, 1.0)
    mesh.materials.append(mat)
    obj = bpy.data.objects.new('ReferencePerson', mesh)
    bpy.context.scene.collection.objects.link(obj)
    return obj


def meadow(models, out_png, size=4.5, spacing=0.9, seed=5):
    """groundcover.png: a piece of meadow as the PCG scatter would make it (a jittered grid, one model per
    spacing² m²), with a person-height figure for scale."""
    rng = random.Random(seed)
    # Larkspur grows along fences, not out in the meadow.
    weights = {'GrassClump_A': 10, 'GrassClump_B': 10, 'GrassClump_C': 6, 'TallGrass_A': 2, 'Flowers_Yellow': 2,
               'Flowers_White': 2, 'Flowers_Purple': 1, 'Clover_A': 3, 'Larkspur_A': 0, 'Larkspur_B': 0}
    pool = [m for m in models for _ in range(weights.get(m.name, 1))]
    copies = []
    steps = int(size * 2.0 / spacing)
    for i in range(steps):
        for j in range(steps):
            copy = rng.choice(pool).copy()
            copy.location = (-size + (i + 0.5) * spacing + rng.uniform(-0.3, 0.3),
                             -size + (j + 0.5) * spacing + rng.uniform(-0.3, 0.3), 0.0)
            copy.rotation_euler = (0.0, 0.0, rng.uniform(0.0, 2.0 * math.pi))
            s = rng.uniform(0.9, 1.1)
            copy.scale = (s, s, s)
            bpy.context.scene.collection.objects.link(copy)
            copies.append(copy)
    figure = person()
    figure.location = (0.9, 1.4, 0.0)
    bpy.context.view_layer.update()  # the placements' matrices, which lt.preview frames
    lt.preview(copies + [figure], out_png, view=(-0.4, -1.6, 0.45), fit=0.4)
    for copy in copies:
        bpy.data.objects.remove(copy)
    mesh, mat = figure.data, figure.data.materials[0]
    bpy.data.objects.remove(figure)
    bpy.data.meshes.remove(mesh)
    bpy.data.materials.remove(mat)


if __name__ == '__main__':
    models = [grass_clump_a(), grass_clump_b(), grass_clump_c(), tall_grass_a(), flowers_yellow(), flowers_white(),
              flowers_purple(), clover_a(), larkspur_a(), larkspur_b()]
    report(models)
    if lt.want_preview() and '--patch' in sys.argv:
        meadow(models, lt.preview_path('Vegetation', 'groundcover'))
    else:
        preview_all(models, view=(-0.8, -1.6, 0.8), fit=0.8)
