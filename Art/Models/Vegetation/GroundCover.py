"""Ground cover for the PCG scatter (Docs/TutorialIsland.md), sized to cover a meadow at about one model per 0.8 m²:
GrassClump_A, GrassClump_B, GrassClump_C (grass patches 70-120 cm across, 25-55 cm tall), TallGrass_A (a loose patch,
80-110 cm tall), Flowers_Yellow, Flowers_White, Flowers_Purple (drifts of flowers in grass, 60-80 cm across) and
Clover_A (a mat, about 70 cm across). Scripted models (Art/README.md) built with Tools/Blender/looter_plants.py; the
patch's middle, at ground level, is the origin.

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


def ground_cover(name, seed, build):
    """A ground cover model: build(plant, mat, rng) makes its parts; its occlusion is authored, not baked."""
    rng = random.Random(seed)
    plant = Plant(name, seed)
    build(plant, lt.material('FoliagePalette'), rng)
    return finish(plant, None, ao_blend=1.0, lods=LODS, lod_screens=LOD_SCREENS)


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
    weights = {'GrassClump_A': 10, 'GrassClump_B': 10, 'GrassClump_C': 6, 'TallGrass_A': 2, 'Flowers_Yellow': 2,
               'Flowers_White': 2, 'Flowers_Purple': 1, 'Clover_A': 3}
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
              flowers_purple(), clover_a()]
    report(models)
    if lt.want_preview() and '--patch' in sys.argv:
        meadow(models, lt.preview_path('Vegetation', 'groundcover'))
    else:
        preview_all(models, view=(-0.8, -1.6, 0.8), fit=0.8)
