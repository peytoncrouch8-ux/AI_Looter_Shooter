"""The island's broadleaf trees (Docs/TutorialIsland.md): Oak_A, Oak_B, Birch_A, Birch_B and Apple_A (the conifers
are in Pines.py, the dead tree in DeadTree.py). Scripted models (Art/README.md) built with
Tools/Blender/looter_plants.py: a tree's origin is where its trunk meets the ground, and one convex hull around the
trunk is its collision. They aren't Nanite: the importer gives them classic LODs.

A tree is laid out crown first. Its crown is a few leaf clumps (spheres) in a readable, slightly stylized silhouette;
limbs grow from the trunk into each clump and leaf cards cover the clumps, shading as soft volumes (see looter_plants).
Wind weights: 0 at the roots, about 0.3 up the trunk, 0.6 at branch tips, 0.7 to 1 across a leaf card.

    blender -b --factory-startup --python Art/Models/Vegetation/Trees.py -- --preview [Oak_A Birch_B ...]
renders Saved/ArtPreviews/Vegetation/<Name>.png for each tree here (or the ones named); '-- --preview --overview'
renders overview.png instead: every tree, pines and dead tree included, side by side.
"""
import math
import os
import random
import sys

from mathutils import Vector

import looter_textures as lt
from looter_plants import (UP, Limb, Plant, bezier, crown_occlusion, curved_limb, cylinder_points, finish, gnarl,
                           horizontal, layout_clumps, leaf_clump, lerp, lineup, preview_all, random_unit, report,
                           root_flare, smoothstep, steps, swatch)


# --- Broadleaf trees: a trunk, limbs into leaf clumps, cards over the clumps ---

def broadleaf(name, seed, stems, hull_height=2.6, lods=None, lod_screens=None, extra=None):
    """A broadleaf tree (oak, birch, apple) of one or more stems (dicts of grow_broadleaf's settings), with a
    collision hull around the lower trunks. extra(plant, rng, crowns) adds anything else (fruit)."""
    rng = random.Random(seed)
    plant = Plant(name, seed)
    trunks, crowns = [], []
    for index, stem in enumerate(stems):
        trunk, crown = grow_broadleaf(plant, rng, seed * 13 + index, **stem)
        trunks.append(trunk)
        crowns.append(crown)
    if extra:
        extra(plant, rng, crowns)
    hull_points = []
    for trunk in trunks:
        for p, r in zip(trunk.points, trunk.radii):
            if 0.0 <= p.z <= hull_height:
                hull_points += cylinder_points(p, p + UP * 0.01, r * 1.05)
    return finish(plant, hull_points, ao_distance=1.5, ao_blend=0.6, lods=lods, lod_screens=lod_screens)


def grow_broadleaf(plant, rng, seed, bark, leaves, fork, radius, fork_radius, lean, crown_center, crown_radii, clumps,
                   clump_size, card_size, cards_per_clump, root=(0.0, 0.0), flare=0.8, lobes=5, trunk_sides=10,
                   limb_sides=6, branch_sides=4, gnarl_amount=0.25, limb_rise=0.7, side_branches=2, limb_low=0.55,
                   hanging=0.0, droop=0.0, rows=1, clump_low=-0.35):
    """One stem of a broadleaf tree: a trunk from root to the fork height (leaning by lean), one limb into every leaf
    clump of its crown (the trunk itself carries on into the highest), side branches inside the clumps, and leaf
    cards over them. Returns the trunk (a Limb) and the crown's clumps."""
    crown_center = Vector(crown_center)
    crown = layout_clumps(rng, crown_center, crown_radii, clumps, clump_size, low=clump_low)
    crown.sort(key=lambda c: c[0].z)
    top_center, top_radius = crown[-1]
    shade_center = crown_center - UP * crown_radii[2] * 0.35
    occlusion = crown_occlusion(crown_center, crown_radii)

    # The trunk rises to the fork, leaning, and carries on as the leader into the highest clump.
    base = Vector((root[0], root[1], -0.35))
    fork_point = Vector((root[0] + lean[0], root[1] + lean[1], fork))
    leader_end = top_center + UP * top_radius * 0.2
    params = [0.0, 0.04, 0.09, 0.15, 0.23, 0.33, 0.45, 0.58, 0.72, 0.86, 1.0]
    lower = bezier(base, base + UP * fork * 0.4, fork_point - (fork_point - base).normalized() * fork * 0.3, fork_point,
                   params)
    upper_count = max(2, int((leader_end - fork_point).length / 0.6))
    upper = bezier(fork_point, fork_point + (fork_point - lower[-2]).normalized() * 1.0, leader_end - UP * 1.2,
                   leader_end, steps(upper_count))[1:]
    points = gnarl(lower + upper, gnarl_amount, 0.35, seed)
    trunk = Limb(points, radius, 0.0, trunk_sides, (0.0, 0.3))
    # Thick to the fork, then thinning fast along the leader.
    trunk.radii = [lerp(radius, fork_radius, smoothstep(0.0, fork, p.z)) if p.z <= fork else
                   fork_radius * max(1.0 - (p.z - fork) / max(leader_end.z - fork, 0.1), 0.0) ** 0.8 for p in points]
    trunk.radii[-1] = 0.0
    plant.tube(trunk, bark, root_flare(flare, lobes, rng.uniform(0.0, 6.28)), occlusion)

    for index, (center, clump_radius) in enumerate(crown[:-1]):
        # Lower clumps branch off lower on the trunk and leave it flatter; each limb heads out toward its clump, then
        # curves up into it.
        share = smoothstep(crown[0][0].z, top_center.z, center.z)
        height = lerp(fork * limb_low, fork + (leader_end.z - fork) * 0.3, share) + rng.uniform(-0.2, 0.2)
        start, trunk_r, trunk_wind, _ = trunk.at(trunk.param_at_height(height))
        out = (horizontal(math.atan2(center.y - start.y, center.x - start.x))
               + UP * lerp(limb_rise * 0.5, limb_rise * 1.4, share))
        end = center + (center - start).normalized() * clump_radius * 0.25
        limb = curved_limb(start, end, out, 0.12, limb_sides, trunk_r * rng.uniform(0.66, 0.85), 0.0,
                           (trunk_wind, 0.45), rng, gnarl_amount * 1.6, 0.5, 0.35, seed * 31 + index)
        plant.tube(limb, bark, occlusion=occlusion)
        for b in range(side_branches):
            s_start, s_r, s_wind, s_dir = limb.at(rng.uniform(0.45, 0.8))
            target = center + (random_unit(rng) + UP * 0.4).normalized() * clump_radius * rng.uniform(0.45, 0.75)
            branch = curved_limb(s_start, target, s_dir + random_unit(rng) * 0.5, 0.05, branch_sides, s_r * 0.6, 0.0,
                                 (s_wind, 0.6), rng, gnarl_amount * 0.6, 0.45, 0.3, seed * 97 + index * 7 + b)
            plant.tube(branch, bark, occlusion=occlusion)

    for index, (center, clump_radius) in enumerate(crown):
        others = [c for j, c in enumerate(crown) if j != index]
        count = int(round(cards_per_clump * (clump_radius / clump_size[1]) ** 2))
        leaf_clump(plant, rng, center, clump_radius, shade_center, leaves, count, card_size, others, occlusion,
                   droop, rows, hanging=hanging)
    return trunk, crown


# --- The trees ---

def oak_a():
    """A broad, round oak: a short thick trunk splitting into spreading limbs, about 8.5 m."""
    return broadleaf('Oak_A', 11, [dict(
        bark=lt.material('BarkOak'), leaves=lt.material('LeavesOak'), fork=2.6, radius=0.4, fork_radius=0.3,
        lean=(0.18, 0.06), crown_center=(0.2, 0.1, 5.6), crown_radii=(4.6, 4.3, 2.5), clumps=9, clump_size=(1.8, 2.3),
        card_size=(1.7, 2.2), cards_per_clump=30, limb_rise=0.45, limb_low=0.75)])


def oak_b():
    """An older, lopsided oak leaning off its roots, its crown pushed to one side, about 9.5 m."""
    return broadleaf('Oak_B', 23, [dict(
        bark=lt.material('BarkOak'), leaves=lt.material('LeavesOak'), fork=3.0, radius=0.46, fork_radius=0.33,
        lean=(0.6, -0.25), crown_center=(1.0, -0.4, 6.4), crown_radii=(4.9, 3.9, 2.8), clumps=10,
        clump_size=(1.8, 2.4), card_size=(1.7, 2.2), cards_per_clump=30, limb_rise=0.4, limb_low=0.65,
        gnarl_amount=0.35, flare=0.9)])


def birch(name, seed, stems):
    return broadleaf(name, seed, [dict(
        bark=lt.material('BarkBirch'), leaves=lt.material('LeavesBirch'), flare=0.35, lobes=4, trunk_sides=8,
        limb_sides=5, branch_sides=3, gnarl_amount=0.12, limb_rise=1.1, side_branches=2, hanging=0.55, droop=0.18,
        rows=2, card_size=(1.0, 1.3), cards_per_clump=20, clump_low=-0.65, **stem) for stem in stems], hull_height=2.2)


def birch_a():
    """A slender birch with an airy, narrow crown and hanging twigs, about 11 m."""
    return birch('Birch_A', 37, [dict(fork=4.6, radius=0.17, fork_radius=0.12, lean=(0.25, 0.1),
                                      crown_center=(0.3, 0.1, 7.3), crown_radii=(2.4, 2.3, 3.4), clumps=14,
                                      clump_size=(0.95, 1.25), limb_low=0.8)])


def birch_b():
    """Two birch stems from one root, leaning apart, about 10-12 m."""
    return birch('Birch_B', 41, [
        dict(root=(-0.12, 0.0), fork=4.8, radius=0.15, fork_radius=0.1, lean=(-0.7, 0.15),
             crown_center=(-0.9, 0.2, 7.9), crown_radii=(2.0, 1.9, 3.4), clumps=10, clump_size=(0.9, 1.2),
             limb_low=0.85),
        dict(root=(0.12, 0.02), fork=3.8, radius=0.13, fork_radius=0.09, lean=(0.6, -0.2),
             crown_center=(0.9, -0.3, 6.5), crown_radii=(1.9, 1.8, 2.9), clumps=8, clump_size=(0.85, 1.15),
             limb_low=0.85)])


def apple_a():
    """A small orchard apple: a short trunk, a round crown and a few red apples, about 4.5 m."""
    palette = lt.material('FoliagePalette')
    apple_uv = swatch('FruitRed', 0.7, 0.5)

    def apples(plant, rng, crowns):
        # Fruit hangs at the surface of each clump's lower half, where it shows; a little oversized to read.
        for center, radius in crowns[0]:
            for _ in range(rng.randint(2, 4)):
                d = (random_unit(rng) * Vector((1.0, 1.0, 0.5)) - UP * 0.35).normalized()
                plant.ball(center + d * radius * rng.uniform(0.92, 1.02), rng.uniform(0.06, 0.07), palette, apple_uv,
                           wind=0.8, occlusion=0.95, segments=5, rings=3)

    return broadleaf('Apple_A', 53, [dict(
        bark=lt.material('BarkOak'), leaves=lt.material('LeavesBirch', name='LeavesApple', tint=0x9fb08a),
        fork=1.1, radius=0.15, fork_radius=0.11, lean=(0.12, 0.05), crown_center=(0.1, 0.0, 3.0),
        crown_radii=(2.3, 2.2, 1.5), clumps=7, clump_size=(1.0, 1.3), card_size=(1.0, 1.25), cards_per_clump=22,
        flare=0.5, trunk_sides=8, limb_sides=5, branch_sides=3, gnarl_amount=0.15, limb_rise=0.8, limb_low=0.9)],
        hull_height=1.2, extra=apples)


if __name__ == '__main__':
    models = [oak_a(), oak_b(), birch_a(), birch_b(), apple_a()]
    report(models)
    if '--overview' in sys.argv:
        # Every tree side by side for scale (Saved/ArtPreviews/Vegetation/overview.png).
        sys.dont_write_bytecode = True  # no __pycache__ in the art folder
        sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
        import DeadTree
        import Pines
        others = [Pines.pine_a(), Pines.pine_b(), DeadTree.dead_tree_a()]
        lineup([models[4], models[0], models[1], models[2], models[3]] + others,
               lt.preview_path('Vegetation', 'overview'))
    else:
        preview_all(models)
