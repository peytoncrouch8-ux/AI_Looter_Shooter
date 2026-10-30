"""The Venom Spitter from Tripo: its atlas painted in our look (parts_spitter_look.py bakes Tripo's paint first). Not a
model (Tools\\models.ps1 skips parts_*.py).

Tripo's paint is soft and grained like wood (fine streaks along the legs, contour swirls on the carapace): the user
turned that look down on the old spider (2026-10-22: "legs like blocky striped poles", the streak noise "read as wood
grain"). So:
  - the limbs (femur, tibia, palp) are repainted from the bake's band pattern alone: each limb's colour along it (its
    pale shafts, dark joints and claw) made crisp, a pale rim before each dark joint, broad soft mottling, a painted light
    along the top and a darker belly (Spider.py's painted chitin);
  - the carapace and face keep Tripo's shapes (grooves, eye rims, plates) with their fine grain damped, lit from above
    and darker underneath; the eyes stay crisp;
  - what Tripo never painted on a shape of ours (the eyes, the venom core, the veins, the collar and waist) is painted.
"""
import math

import numpy as np

import looter_textures as lt
from parts_spitter_look import REGIONS, box, grads, lin

EYE, EYE_ROUGH = 0x0E1A16, 0.18
CORE_DARK, CORE_LIGHT = 0x1F3A10, 0x3F6A1C
VEIN_DARK, VEIN_LIGHT = 0x2A1A10, 0x5A3B22
COLLAR_DARK, COLLAR_LIGHT = 0x3A2616, 0x7A5634
CHITIN_ROUGH, FANG_ROUGH, KNOB_ROUGH = 0.8, 0.42, 0.6
GRAIN_KEEP = 0.4          # share of the carapace's fine detail (under ~6 px) kept: grooves stay, grain fades
TOP_LIGHT = 0.16          # a limb's painted light along its top, and its belly's darkening
BODY_LIGHT = 0.14         # the carapace's, by how far up each texel faces
JOINT_DARK = 0.74         # a limb's texel is a dark joint below this share of its leg's running brightness
LUMA = np.array([0.2126, 0.7152, 0.0722], np.float32)


def _smoothstep(e0, e1, x):
    t = np.clip((x - e0) / (e1 - e0), 0.0, 1.0)
    return t * t * (3.0 - 2.0 * t)


def _smooth1d(a, sigma):
    """A 1-D Gaussian along axis 0 (edges held)."""
    if sigma <= 0:
        return a
    r = int(3 * sigma) + 1
    k = np.exp(-0.5 * (np.arange(-r, r + 1) / sigma) ** 2)
    k /= k.sum()
    pad = np.pad(a, [(r, r)] + [(0, 0)] * (a.ndim - 1), mode='edge')
    if a.ndim == 1:
        return np.convolve(pad, k, mode='valid')
    return np.stack([np.convolve(pad[:, i], k, mode='valid') for i in range(a.shape[1])], axis=1)


def _mix(a, b, t):
    return a + (b - a) * np.asarray(t, np.float32)[..., None]


def fill_missed(colour, covered):
    """Texels no ray reached (the carapace's back, under the sac): the covered colour round them, widely blurred."""
    w = lt.blur(covered.astype(np.float32), 40.0)
    c = lt.blur(colour * covered[..., None], 40.0) / np.maximum(w, 1e-4)[..., None]
    return np.where(covered[..., None], colour, c)


def limb(tile, U, V, seed):
    """A limb strip (U along, V round, 0.5 its top) repainted from its band pattern."""
    prof = tile.mean(axis=0)                                  # colour along the limb
    luma = prof @ LUMA
    running = _smooth1d(luma, 40.0)
    dark = _smoothstep(JOINT_DARK + 0.06, JOINT_DARK - 0.06, _smooth1d(luma, 1.5) / np.maximum(running, 1e-4))
    weight = _smooth1d(dark, 10.0)
    joint = _smooth1d(prof * dark[:, None], 10.0) / np.maximum(weight, 1e-4)[:, None]
    joint = np.where(weight[:, None] > 0.02, joint, prof * 0.45)
    pale = _smooth1d(prof * (1.0 - dark)[:, None], 25.0) / np.maximum(_smooth1d(1.0 - dark, 25.0), 1e-4)[:, None]
    along = _mix(pale, joint * 0.85, dark)
    # A pale rim just before each dark joint (each plate's hard edge).
    ahead = np.maximum.reduce([np.roll(dark, -k) for k in range(1, 9)])
    rim = np.clip(ahead - dark, 0.0, 1.0)
    along = along * (1.0 + 0.18 * rim)[:, None]
    out = np.broadcast_to(along[None, :, :], tile.shape).copy()
    light = 1.0 + TOP_LIGHT * np.cos(2.0 * math.pi * (V - 0.5))
    mottle = 1.0 + 0.06 * lt.noise(U.shape, seed, 22.0, 9.0, octaves=2)
    pores = 1.0 + 0.025 * lt.noise(U.shape, seed + 1, 1.2)
    return out * (light * mottle * pores)[..., None]


def body_dirs(n_rows, n_cols):
    """Each carapace texel's direction from the shell's middle (z up), from the cube-sphere's 3 x 2 UV layout."""
    v = 1.0 - (np.arange(n_rows) + 0.5) / n_rows           # region V, rows top first
    u = (np.arange(n_cols) + 0.5) / n_cols
    U, V = np.meshgrid(u, v)
    col = np.minimum((U * 3).astype(int), 2)
    row = np.minimum((V * 2).astype(int), 1)
    fu, fv = U * 3 - col, V * 2 - row
    face = col + 3 * row
    axis, sign = face // 2, np.where(face % 2 == 0, -1.0, 1.0)
    p = np.zeros(U.shape + (3,))
    a, b = np.tan((fu - 0.5) * math.pi / 2.0), np.tan((fv - 0.5) * math.pi / 2.0)
    for ax in range(3):
        others = [x for x in range(3) if x != ax]
        m = axis == ax
        p[..., ax] = np.where(m, sign, p[..., ax])
        p[..., others[0]] = np.where(m, a, p[..., others[0]])
        p[..., others[1]] = np.where(m, b, p[..., others[1]])
    return p / np.linalg.norm(p, axis=-1, keepdims=True)


def style(colour, covered):
    """Tripo's bake in our look (see the module's docstring). Returns (BC linear, ORM)."""
    bc = fill_missed(colour, covered)
    orm = np.zeros_like(bc)
    orm[..., 0], orm[..., 1] = 1.0, CHITIN_ROUGH
    for region, seed in (('femur', 31), ('tibia', 32), ('palp', 33)):
        r0, r1, c0, c1 = box(region)
        U, V = grads(region)
        bc[r0:r1, c0:c1] = limb(bc[r0:r1, c0:c1], U, V, seed)
    for region in ('body', 'knob', 'fang'):
        r0, r1, c0, c1 = box(region)
        tile = bc[r0:r1, c0:c1]
        big = lt.blur(tile, 6.0)
        calm = big + (tile - big) * GRAIN_KEEP
        # The eyes (dark, glossy) keep every texel.
        luma = tile @ LUMA
        eyes = _smoothstep(0.03, 0.015, luma) if region == 'body' else np.zeros_like(luma)
        tile = _mix(calm, tile, eyes)
        if region == 'body':
            up = body_dirs(r1 - r0, c1 - c0)[..., 2]
            tile = tile * (1.0 + BODY_LIGHT * up)[..., None]
            orm[r0:r1, c0:c1, 1] = CHITIN_ROUGH - 0.55 * eyes
        bc[r0:r1, c0:c1] = tile
    r0, r1, c0, c1 = box('fang')
    orm[r0:r1, c0:c1, 1] = FANG_ROUGH
    r0, r1, c0, c1 = box('knob')
    orm[r0:r1, c0:c1, 1] = KNOB_ROUGH
    return np.clip(bc, 0.0, 1.0), orm


def paint_built(bc, orm):
    """The regions Tripo never painted on a shape of ours: the eyes, the venom core, the veins, the collar and waist."""
    r0, r1, c0, c1 = box('eye')
    bc[r0:r1, c0:c1] = lin(EYE)
    orm[r0:r1, c0:c1] = (1.0, EYE_ROUGH, 0.0)
    for region, dark, light, rough, seed in (('core', CORE_DARK, CORE_LIGHT, 0.5, 11),
                                             ('vein', VEIN_DARK, VEIN_LIGHT, 0.55, 12),
                                             ('collar', COLLAR_DARK, COLLAR_LIGHT, 0.78, 13)):
        r0, r1, c0, c1 = box(region)
        U, V = grads(region)
        if region == 'core':
            t = 0.5 + 0.22 * lt.noise(U.shape, seed, 6.0, octaves=3)
        elif region == 'vein':
            # Lit along its crest (V 0/1, the side out of the gel), dark down its sunk sides.
            t = 0.25 + 0.6 * (0.5 + 0.5 * np.cos(2.0 * math.pi * V)) + 0.06 * lt.noise(U.shape, seed, 4.0)
        else:
            # Dark and thick at the mouth, lighter out along the tongues, a dark rim at their edges; blotches about as
            # wide as long on the sac (the strip's V runs ~5x the ground of its U).
            t = (0.3 + 0.55 * _smoothstep(0.0, 0.8, U) - 0.45 * _smoothstep(0.88, 1.0, U)
                 + 0.09 * lt.noise(U.shape, seed, 12.0, 2.5, octaves=2))
        bc[r0:r1, c0:c1] = _mix(lin(dark), lin(light), np.clip(t, 0.0, 1.0))
        orm[r0:r1, c0:c1] = (1.0, rough, 0.0)
