"""The Meadow Wolf's bristles on the Tripo build (parts_spider_build.py runs it), as the scripted spider has them
(Spider.py's numbers: FEMUR_SPINES, TIBIA_SPINES, LOOSE_HAIRS, BODY_HAIRS, HAIR_LENGTH, HAIR_RADIUS; lc.bristle, four
vertices each, on the atlas's solid bristle patch): spines in pairs under the tibia and metatarsus, a row on the femur,
short loose hairs leaning toward the claw, and coarse hairs combed back over the carapace and the abdomen. Each sits
on its part's rebuilt surface (its ring's radius at that place), so the hit hulls still hold them. Seeded: every run
lays the same hairs."""
import math

import numpy as np
from mathutils import Vector

import looter_creatures as lc


def _station(limb, u):
    """A limb's centre, frame and radii at its region place u (interpolated between its stations)."""
    U = limb['U']
    k = int(np.clip(np.searchsorted(U, u) - 1, 0, len(U) - 2))
    f = 0.0 if U[k + 1] == U[k] else float(np.clip((u - U[k]) / (U[k + 1] - U[k]), 0.0, 1.0))
    lerp = lambda A: A[k] * (1.0 - f) + A[k + 1] * f
    tangent = limb['C'][k + 1] - limb['C'][k]
    return lerp(limb['C']), lerp(limb['top']), lerp(limb['side']), lerp(limb['R']), tangent / np.linalg.norm(tangent)


def _radius_at(R, around):
    """A ring's radius at an angle from its top (degrees), between its rays."""
    sides = len(R)
    x = (math.radians(around) + math.pi) / (2.0 * math.pi) * sides
    j = int(math.floor(x)) % sides
    f = x - math.floor(x)
    return R[j] * (1.0 - f) + R[(j + 1) % sides] * f


def leg_hair(m, limb, u, around, length, lean, radius):
    centre, top, side, R, tangent = _station(limb, u)
    a = math.radians(around)
    normal = top * math.cos(a) + side * math.sin(a)
    normal /= np.linalg.norm(normal)
    lean = math.radians(lean)
    base = centre + normal * _radius_at(R, around) * 0.82
    lc.bristle(m, Vector(base), Vector(normal * math.cos(lean) + tangent * math.sin(lean)), length, radius, 'bristle')


def legs(hairs, limbs, ns, seed=1847):
    """Spines and loose hairs on every femur and tibia (limbs: {bone: limb})."""
    rng = np.random.default_rng(seed)
    length, radius = ns['HAIR_LENGTH'], ns['HAIR_RADIUS']
    for bone in sorted(limbs):
        limb = limbs[bone]
        m = hairs.setdefault(bone, lc.Mesh(ns['ATLAS']))
        femur = bone.startswith('femur')
        spines = ns['FEMUR_SPINES'] if femur else ns['TIBIA_SPINES']
        for t, around, share in spines:
            for a in ((around, -around) if 0.0 < around < 180.0 else (around,)):
                leg_hair(m, limb, t, a + rng.uniform(-8, 8), length * share * rng.uniform(0.9, 1.1), 52.0, radius)
        count, low, high = ns['LOOSE_HAIRS']['femur' if femur else 'tibia']
        for _ in range(count):
            leg_hair(m, limb, rng.uniform(0.05, 0.92), rng.uniform(-115, 115), length * rng.uniform(low, high), 57.0,
                     radius)


def shell(m, grid, count, where, ns, seed):
    """Coarse hairs combed back over a shell grid (centre, dirs, r, fwd): count of them where where(d, s, v) holds
    (d 1 on the back, s -1..1 side to side, v 0 rear .. 1 front: the painter's axes)."""
    rng = np.random.default_rng(seed)
    centre, dirs, r, fwd = grid['centre'], grid['dirs'], grid['r'], grid['fwd']
    P = centre + dirs * r[..., None]
    rings, segs = r.shape
    length, radius = ns['HAIR_LENGTH'], ns['HAIR_RADIUS']
    placed, tries, used = 0, 0, set()     # one hair a grid point (two on one point lay one inside the other)
    while placed < count and tries < count * 50:
        tries += 1
        i = int(rng.integers(1, rings - 1))
        j = int(rng.integers(0, segs))
        if (i, j) in used:
            continue
        u = j / segs
        d, s = math.cos(2.0 * math.pi * (u - 0.5)), math.sin(2.0 * math.pi * (u - 0.5))
        if not where(d, s, (i + 1) / (rings + 1)):
            continue
        p = P[i, j]
        n = np.cross(P[i, (j + 1) % segs] - P[i, j - 1], P[i + 1, j] - P[i - 1, j])
        n /= np.linalg.norm(n)
        if n @ (p - centre) < 0:
            n = -n
        lc.bristle(m, Vector(p - n * 0.5), Vector(n * 0.45 - fwd * 0.9), length * rng.uniform(0.7, 1.3), radius,
                   'bristle')
        used.add((i, j))
        placed += 1
