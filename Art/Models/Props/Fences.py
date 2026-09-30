"""Field boundaries for the tutorial island: a rail fence kit and a dry-stone wall kit. A scripted model (see
Art/README.md); the props are built from parts, UV-mapped on the house trim sheet or a tileable, and merged.

  FenceRail     3 m post-and-rail segment: posts at x = 0 and x = 1.5 m, two weathered rails from x = 0 to 3 m
  FencePost     a single post: ends a run of FenceRail (put it at the last segment's x = 3 m)
  FenceBroken   a FenceRail whose top rail has snapped (one end hangs to the ground), a leaning post, a rail lying in
                front; the player can jump the gap
  StoneWall     3 m dry-stone wall segment, 0.9 m tall, with upright coping stones along the top
  StoneWallEnd  0.9 m finished end of a wall

Chaining: every segment's pivot is on the ground at its start (x = 0, the middle of the post or of the wall's
width) and it runs along +X to x = 3 m, where the next segment starts. Turn the next segment at the joint for a
corner. Fence rails end inside the next segment's first post; finish a run with a FencePost at its end. The stone wall
is shaped and textured so its x = 3 m end matches its x = 0 start (and StoneWallEnd's start), so walls chain without a
seam; put StoneWallEnd at a run's end, or turned 180 degrees at its start. Posts and walls reach 15 cm below the
pivot so they sit in uneven ground.

The fence pieces are small (under 500 triangles, in long runs) and have no Nanite; the walls keep it.
"""
import math
import random

import bmesh
from mathutils import Vector, noise

import looter_textures as lt
import looter_props as lp


# --- Rail fence ---

POST_HEIGHT = 1.22
RAIL_HEIGHTS = (0.5, 0.98)


def post(x, seed, lean=1.5, height=POST_HEIGHT):
    """A hewn post, its top cut at a slant to shed rain, leaning a little."""
    rnd = random.Random(seed)
    part = lp.block((0.14, 0.14, height + 0.15), (0.0, 0.0, (height - 0.15) * 0.5), bevel=0.014)
    slant = rnd.choice((-1.0, 1.0))
    for v in part.data.vertices:
        if v.co.z > height - 0.1:
            v.co.z -= 0.045 * (0.5 + slant * v.co.y / 0.14)
    lp.grain(part, 'Siding', axis=(0.0, 0.0, 1.0), seed=seed)
    lp.slice_at(part, (0.0, 0.0, 1.0), (0.03, 0.4))            # vertices just above the ground, for the AO
    lp.rough(part, 0.006, 6.0, seed)
    return lp.place(part, (x, 0.0, 0.0), (rnd.uniform(-lean, lean), rnd.uniform(-lean, lean), rnd.uniform(-5.0, 5.0)))


def rail(p0, p1, seed, sag=0.025, segments=6, broken_end=False, posts=()):
    """A split rail from p0 to p1: an uneven six-sided section, bowed and sagging a little between its ends. posts
    (x positions) get rings just beside them, so the rail's shading darkens only where it enters a post."""
    rnd = random.Random(seed)
    p0, p1 = Vector(p0), Vector(p1)
    bow = rnd.uniform(-0.015, 0.015)
    ts = {i / segments for i in range(segments + 1)}
    span = p1.x - p0.x
    for x in posts:
        for side in (-0.1, 0.1):
            t = (x + side - p0.x) / span if abs(span) > 1e-6 else -1.0
            if 0.02 < t < 0.98:
                ts.add(t)
    ts = sorted(t for t in ts if not any(abs(t - (x - p0.x) / span) < 0.03 for x in posts) or t in (0.0, 1.0))
    points = []
    for t in ts:
        points.append(p0.lerp(p1, t) + Vector((0.0, bow, -sag)) * math.sin(math.pi * t))
    profile = [(a * 0.6, b) for a, b in lp.ngon(0.055, 6, jitter=0.12, seed=seed)]   # deeper than thick, like split wood
    part = lp.sweep(points, profile)
    lp.grain(part, 'Siding', axis=p1 - p0, seed=seed + 7)
    if broken_end:
        # A splintered end: the last ring's corners pulled out unevenly along the rail.
        direction = (p1 - p0).normalized()
        for v in part.data.vertices:
            if (v.co - p1).dot(direction) > -0.08 and (v.co - p1).length < 0.12:
                v.co += direction * rnd.uniform(-0.1, 0.04)
    lp.rough(part, 0.004, 5.0, seed)
    return part


def fence_rail(name, seed):
    rnd = random.Random(seed)
    parts = [post(0.0, seed + 1), post(1.5, seed + 2)]
    for k, z in enumerate(RAIL_HEIGHTS):
        z += rnd.uniform(-0.02, 0.02)
        parts.append(rail((-0.03, 0.0, z), (3.03, 0.0, z), seed + 10 + k, posts=(0.0, 1.5, 3.0)))
    obj = lp.join(name, parts)
    lp.hull_box(obj, (3.07, 0.16, POST_HEIGHT), (1.5, 0.0, POST_HEIGHT * 0.5))
    return lp.finish(obj, ao=0.4, nanite=False)


def fence_post(name, seed):
    obj = lp.join(name, [post(0.0, seed, lean=1.0)])
    lp.hull_box(obj, (0.16, 0.16, POST_HEIGHT), (0.0, 0.0, POST_HEIGHT * 0.5))
    return lp.finish(obj, ao=0.4, nanite=False)


def fence_broken(name, seed):
    parts = [post(0.0, seed + 1)]
    leaning = post(0.0, seed + 2, lean=0.0)
    lp.place(leaning, (0.0, 0.0, 0.0), (7.0, -4.0, 0.0))     # pushed over a little
    lp.place(leaning, (1.5, 0.0, 0.0))
    parts.append(leaning)
    # The bottom rail is whole; the top rail snapped at 2.15 m: the long piece still rests in the posts, the short one
    # hangs from the next post down to the ground.
    low, high = RAIL_HEIGHTS
    parts.append(rail((-0.03, 0.0, low), (3.03, 0.0, low), seed + 10, posts=(0.0, 1.5, 3.0)))
    parts.append(rail((-0.03, 0.0, high + 0.02), (2.12, 0.0, high - 0.06), seed + 11, sag=0.03, segments=5,
                      broken_end=True, posts=(0.0, 1.5)))
    parts.append(rail((3.03, 0.0, high), (2.28, -0.05, 0.05), seed + 12, sag=0.0, segments=2, broken_end=True))
    # A loose rail lying in the grass in front.
    parts.append(rail((0.35, -0.5, 0.04), (1.7, -0.78, 0.05), seed + 13, sag=0.0, segments=3))
    obj = lp.join(name, parts)
    lp.hull_box(obj, (2.25, 0.16, POST_HEIGHT), (1.1, 0.0, POST_HEIGHT * 0.5))
    lp.hull_box(obj, (0.9, 0.16, 0.56), (2.6, 0.0, 0.28))    # only the bottom rail here: jump over it
    return lp.finish(obj, ao=0.4, nanite=False)


# --- Dry-stone wall ---

STONE = lt.material('StoneWall')
WALL_LENGTH = 3.0
WALL_DENSITY = 1024.0 / WALL_LENGTH   # px per metre: the texture repeats exactly once per segment, so joints match
BODY_TOP = 0.72


def wall_profile(width_scale=1.0):
    """The wall's section (across, up): battered sides, a rounded top; from the buried foot around."""
    half_base, half_top = 0.36, 0.25
    pts = [(-half_base, -0.15), (-half_base, 0.02), (-0.33, 0.25), (-0.29, 0.5), (-half_top, BODY_TOP - 0.06),
           (-0.12, BODY_TOP), (0.12, BODY_TOP), (half_top, BODY_TOP - 0.06), (0.29, 0.5), (0.33, 0.25),
           (half_base, 0.02), (half_base, -0.15)]
    return [(a * width_scale, b) for a, b in pts]


def stones(x, y, z):
    """How far the wall's face bulges at (x, y, z): lumpy, not flat (the texture draws the stones themselves).
    Periodic along x (every wall length), so a wall's end matches the next one's start."""
    def lumps(px):
        return noise.noise(Vector((px * 2.6, y * 2.6 + 11.0, z * 3.0))) + 0.5 * noise.noise(Vector((px * 6.0, y * 6.0, z * 6.0 + 5.0)))
    blend = min(max((x - (WALL_LENGTH - 0.6)) / 0.6, 0.0), 1.0)
    return 0.03 * (lumps(x) * (1.0 - blend) + lumps(x - WALL_LENGTH) * blend)


def wall_body(x0, x1, end_round=None):
    """The body of a wall from x0 to x1, its sides pushed out where the stones are; end_round (x) rounds the wall off
    in plan after that point."""
    steps = max(2, int(round((x1 - x0) / 0.1)))
    profile = wall_profile()
    bm = lp.new_bmesh()
    rings = []
    for i in range(steps + 1):
        x = x0 + (x1 - x0) * i / steps
        width = 1.0
        if end_round is not None and x > end_round:
            t = (x - end_round) / (x1 - end_round)
            width = max(math.sqrt(max(1.0 - t * t, 0.0)), 0.18)
        ring = []
        for a, b in profile:
            a *= width
            if b > -0.1:
                # Stones stand out sideways on the faces and upward on the top.
                n = Vector((0.0, a, (b - 0.3) * 0.6 if b > BODY_TOP - 0.1 else 0.0)).normalized()
                p = Vector((x, a, b)) + n * stones(x, a, b)
            else:
                p = Vector((x, a, b))
            ring.append(bm.verts.new(p))
        rings.append(ring)
    n = len(profile)
    for r0, r1 in zip(rings, rings[1:]):
        for j in range(n - 1):
            bm.faces.new((r0[j], r0[j + 1], r1[j + 1], r1[j]))
        bm.faces.new((r0[n - 1], r0[0], r1[0], r1[n - 1]))
    bm.faces.new(rings[0])
    bm.faces.new(rings[-1])
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces)
    return lp.mesh_object(bm)


def coping(x0, x1, seed, end_stone=False):
    """Coping: flat stones set on edge along the top, tall and short in turn ("cock and hen"), leaning a little."""
    rnd = random.Random(seed)
    parts = []
    x = x0 + 0.08
    count = 0
    while x < x1 - 0.06:
        thick = rnd.uniform(0.06, 0.11)
        height = rnd.uniform(0.2, 0.27) if count % 2 == 0 else rnd.uniform(0.12, 0.17)
        width = rnd.uniform(0.36, 0.46)
        stone = lp.block((thick, width, height), (0.0, 0.0, height * 0.5))
        lp.rough(stone, 0.02, 8.0, rnd.randint(0, 999))
        lp.place(stone, (x, rnd.uniform(-0.03, 0.03), BODY_TOP - 0.06),
              (rnd.uniform(-5.0, 5.0), rnd.uniform(-12.0, 12.0), rnd.uniform(-8.0, 8.0)))
        parts.append(stone)
        x += thick + rnd.uniform(0.02, 0.05)
        count += 1
    if end_stone:
        stone = lp.block((0.3, 0.46, 0.26), (0.0, 0.0, 0.13), bevel=0.03)
        lp.rough(stone, 0.015, 7.0, seed + 5)
        lp.place(stone, (x1 - 0.22, 0.0, BODY_TOP - 0.07), (0.0, 3.0, 4.0))
        parts.append(stone)
    return parts


def stone_wall(name, seed, end=False):
    length = 0.9 if end else WALL_LENGTH
    body = wall_body(0.0, length, end_round=0.45 if end else None)
    parts = [body] + coping(0.0, length - (0.12 if end else 0.0), seed, end_stone=end)
    for part in parts:
        lt.assign(part, STONE)
    obj = lp.join(name, parts)
    lt.box_uv(obj, 'StoneWall', texel_density=WALL_DENSITY)
    top = BODY_TOP + 0.2
    profile = [(-0.36, -0.15), (0.36, -0.15), (-0.36, 0.02), (0.36, 0.02), (-0.25, top), (0.25, top)]
    x_end = length if not end else length - 0.05
    lp.hull_points(obj, [Vector((x, a, b)) for x in (0.0, x_end) for a, b in profile])
    return lp.finish(obj, ao=0.5, fallback=100, smooth=35.0)


models = [
    fence_rail('FenceRail', 10),
    fence_post('FencePost', 20),
    fence_broken('FenceBroken', 30),
    stone_wall('StoneWall', 40),
    stone_wall('StoneWallEnd', 50, end=True),
]

if lt.want_preview():
    for model in models:
        lt.preview([model], lt.preview_path('RocksProps', model.name))
