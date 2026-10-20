"""Gauge's parts (Art/Models/Characters/Gauge.py): the railroad's clockwork track-layer as hard-surface pieces, each a
closed solid riding one mannequin bone (or a fixed blend of two), built twice from the same numbers: the game mesh
(Kit.hi False: few segments, two-step bevels) and the detailed bake source (Kit.hi True: round bevels, rivets, the
boiler's plate seams, the nameplate's letters, the crest's wheel, the waist's bellows), cast onto it by looter_bake.

The look is the portrait's (Art/Portraits/Gauge.py OPTIONS["C"], Company Iron): the head is its numbers; the body is the
game's own around a human's joints (the portrait's boiler ran down past the hips, where the thighs must swing).
Space: metres, Z up, facing -Y, its left +X, at the skeleton's bind pose. Looks are named <Region><Kind> (HeadEnamel,
BodyBrass, ...): a region is a texture set (Gauge.py TEXTURE_SETS).
"""
import math

import numpy as np
from mathutils import Quaternion, Vector

from gauge_geo import (R, S, T, arc, box, cyl, dome_shell, frame, housing, join, lathe, loft, perp, prism, rivets, ring,
                       sheet, smoothstep, sphere, spline, superellipse, text, torus, tube, unit)
from gauge_geo import Shape as gg_shape

X, Y, Z = (1.0, 0.0, 0.0), (0.0, 1.0, 0.0), (0.0, 0.0, 1.0)
FWD = np.array([0.0, -1.0, 0.0])
HEAD_Z = 1.616            # the head pivot the body's heights below are written for (Kit.z shifts them to the skeleton's)
RIVET = 0.0045            # a rivet's head (m)
# The boiler (the chest): (z, half width, half depth, centre y), bottom to top; a superellipse (power 3.2) through each,
# smooth between. Its widest, 0.51 m, at the elbows' height; its shoulders round in under the collar. The upper part
# (from the chest band up) rides spine_05 with the shoulders, the lower one (the firebox) spine_03.
BOILER = [(1.110, 0.192, 0.163, 0.020), (1.140, 0.212, 0.175, 0.020), (1.190, 0.236, 0.188, 0.019),
          (1.260, 0.250, 0.197, 0.017), (1.350, 0.255, 0.200, 0.015), (1.440, 0.250, 0.194, 0.012),
          (1.510, 0.238, 0.182, 0.008), (1.555, 0.212, 0.165, 0.000), (1.585, 0.165, 0.135, -0.012),
          (1.604, 0.100, 0.090, -0.025)]
POWER = 3.2
CHEST_BAND = 1.438        # the brass band where the upper and lower boiler meet (it hides the seam)
SEAMS_T = (-math.pi / 4, -3 * math.pi / 4, math.pi / 4, 3 * math.pi / 4)   # the plates' upright seams (the portrait's
                          # rivet rows at x +-0.2 down the chest), as the superellipse's angle
SEAMS_UPPER, SEAMS_LOWER = (1.578,), (1.375, 1.17)
GROOVE = 0.0018           # a plate seam's depth (m)
PAULDRON_ARM = 0.7        # the pauldrons' share of the upper arm's turn (the rest on the collarbone): raised to aim,
                          # an arm would go through a pauldron fixed to the shoulders (at half, 4.9 cm through its skirt
                          # aiming high); blended, a pauldron shrinks ~5-8 % in the turned plane
PAULDRON_TILT = 18.0      # the pauldrons' outward lean at the game's idle (the portrait's 20)
PAULDRON_INNER = 0.72     # the pauldrons' inner halves drawn in to this (gauge_parts.build_arm's squeeze)
IDLE_ARM = 10.0           # the idle's arms hang this far off the body (looter_audit._arms_down): the pauldrons are made
                          # for it, and carried into the bind pose by their share of the upper arm


class Kit:
    """The parts at one level of detail, as the builders add them: dicts of name, look, bones ({bone: weight or
    fn(points) -> weights}), shape, group (the audit's), only ('hi', 'lo' or None)."""

    def __init__(self, sk, hi):
        self.sk = {k: (np.asarray(v[0], dtype=np.float64), np.asarray(v[1], dtype=np.float64), v[2])
                   for k, v in sk.items()}
        self.hi = hi
        self.parts = []
        self.notes = {}
        self.group = ''
        self.H = self.sk['head'][0]
        self.dz = self.H[2] - HEAD_Z

    def n(self, lo, hi):
        return hi if self.hi else lo

    def z(self, z):
        return z + self.dz

    def add(self, name, look, bones, shape, only=None, sharp=None):
        if (only == 'hi' and not self.hi) or (only == 'lo' and self.hi) or not len(shape.F):
            return
        self.parts.append(dict(name=name, look=look, bones=bones, shape=shape, group=self.group, sharp=sharp))


# ------------------------------------------------------------------------------------------------------------ head

LAMP_Z = 0.185            # the headlamp's centre over the head pivot
LAMP_R = 0.052


def face_y(z):
    """The cranium's front at height z over the pivot (it leans back toward the crown: the box's top is 0.9 deep)."""
    return 0.005 - 0.112 * (1.0 - 0.10 * (z - 0.064) / 0.216)


def side_x(z):
    return 0.112 * (1.0 - 0.14 * (z - 0.064) / 0.216)


def build_head(K):
    """The portrait's squared head (Company Iron): the headlamp in a brass bezel under a level brow with the GAUGE
    nameplate, cheek guards, a heavy jaw with the grille mouth glowing behind its bars, bolts for ears; on a short iron
    neck (neck_02) that lifts it 5 cm clear of the collar, so the jaw clears the boiler with the head 25 degrees down."""
    K.group = 'Head'
    H = K.H
    B = {'head': 1.0}
    seg = K.n(2, 4)
    at = lambda x, y, z: T(H + np.array([x, y, z]))
    fy = face_y(LAMP_Z)
    K.add('Cranium', 'HeadEnamel', B, box((0.112, 0.112, 0.108), (0.86, 0.90), 0.022, seg).at(at(0, 0.005, 0.172)))
    K.add('Ridge', 'HeadEnamel', B, box((0.016, 0.09, 0.012), b=0.007, seg=seg).at(at(0, 0.01, 0.286)))
    K.add('Jaw', 'HeadEnamel', B, box((0.118, 0.04, 0.034), (0.92, 1.0), 0.014, seg).at(at(0, -0.084, 0.092) @ R(X, -4)))
    K.add('MouthGlow', 'Fire', B, box((0.074, 0.0015, 0.012)).at(at(0, -0.1245, 0.092)), only='lo')
    K.add('Bars', 'HeadIron', B, join(cyl(0.0045, 0.03, K.n(8, 12)).at(at(x, -0.1285, 0.077))
                                      for x in np.linspace(-0.066, 0.066, 9)))
    K.add('Lips', 'HeadBrass', B, join(box((0.082, 0.006, 0.0042), b=0.002, seg=K.n(1, 2)).at(at(0, -0.127, z))
                                       for z in (0.0755, 0.1085)))
    lamp = lambda dy: at(0, fy + dy, LAMP_Z) @ R(X, 90)          # +z out of the face (-Y)
    K.add('Socket', 'HeadDark', B, cyl(LAMP_R + 0.006, 0.012, K.n(32, 48)).at(lamp(0.005)))
    K.add('Bezel', 'HeadBrass', B, torus(LAMP_R + 0.004, 0.0085, K.n(32, 64), K.n(6, 12)).at(lamp(-0.007)))
    K.add('Lens', 'Lamp', B, lathe([(LAMP_R * math.cos(a), 0.009 * math.sin(a))
                                    for a in np.radians(np.linspace(0.0, 90.0, K.n(4, 8)))], K.n(32, 48)).at(lamp(-0.006)),
          only='lo')
    K.add('Brow', 'HeadEnamel', B, join(box((0.064, 0.03, 0.015), b=0.006, seg=seg).at(
        at(s * 0.056, fy - 0.016, LAMP_Z + 0.064) @ R(X, 14)) for s in (-1, 1)))
    K.add('Nameplate', 'HeadBrass', B, box((0.084, 0.004, 0.016), b=0.002, seg=K.n(1, 2)).at(
        at(0, fy - 0.052, LAMP_Z + 0.058) @ R(X, -4)))
    K.add('Letters', 'HeadBlack', B, text('GAUGE', 0.029, 0.0012).at(at(0, fy - 0.0565, LAMP_Z + 0.0575) @ R(X, 86)),
          only='hi')
    K.add('Cheeks', 'HeadEnamel', B, join(box((0.014, 0.022, 0.062), b=0.006, seg=seg).at(
        at(s * 0.098, fy - 0.012, 0.174) @ R(Z, -s * 8)) for s in (-1, 1)))
    K.add('Ears', 'HeadIron', B, join(cyl(0.032, 0.022, K.n(20, 40), b=0.003).at(at(s * 0.101, 0.0, 0.18) @ R(Y, s * 90))
                                      for s in (-1, 1)))
    K.add('EarNuts', 'HeadBrass', B, join(cyl(0.019, 0.014, 6, b=0.002, seg=K.n(1, 2)).at(
        at(s * 0.121, 0.0, 0.18) @ R(Y, s * 90)) for s in (-1, 1)))
    K.add('BackPlate', 'HeadEnamel', B, box((0.08, 0.007, 0.075), b=0.004, seg=seg).at(at(0, 0.113, 0.17) @ R(X, 3)))
    if K.hi:
        pts, nrm = [], []
        for s in (-1, 1):
            for z in (0.135, 0.174, 0.213):                       # down the cheek guards
                pts.append(H + (s * 0.098, fy - 0.0345, z))
                nrm.append((0.0, -1.0, 0.0))
            for z in np.linspace(0.09, 0.25, 6):                    # the sides' upright seam
                pts.append(H + (s * (side_x(z) + 0.0004), 0.045, z))
                nrm.append((s, 0.0, 0.0))
            for y in np.linspace(-0.07, 0.08, 6):                   # along under the crown
                pts.append(H + (s * (side_x(0.255) + 0.0004), y, 0.255))
                nrm.append((s, 0.0, 0.06))
            for a in np.linspace(0.0, 2 * math.pi, 6, endpoint=False):   # round each ear
                pts.append(H + (s * 0.1232, 0.025 * math.cos(a), 0.18 + 0.025 * math.sin(a)))
                nrm.append((s, 0.0, 0.0))
            for z in (0.074, 0.11):                                 # the jaw's sides
                pts.append(H + (s * (0.118 * (0.92 if z > 0.092 else 1.0) + 0.0004), -0.084, z))
                nrm.append((s, 0.0, 0.0))
        for x in np.linspace(-0.072, 0.072, 7):                     # the crown's front, over the brow
            pts.append(H + (x, face_y(0.271) - 0.0008, 0.271))
            nrm.append((0.0, -0.95, 0.3))
        for x in (-0.068, 0.068):                                   # the back plate
            for z in (0.11, 0.17, 0.23):
                pts.append(H + (x, 0.1195 + 0.003 * (z - 0.17) / 0.06, z))
                nrm.append((0.0, 1.0, 0.0))
        K.add('HeadRivets', 'HeadBrass', B, rivets(pts, nrm, RIVET), only='hi')
    # the neck: an iron column on neck_02, a brass ring round it
    K.add('Neck', 'HeadIron', {'neck_02': 1.0}, cyl(0.058, 0.13, K.n(24, 48), b=0.004).at(at(0, 0.01, -0.055)))
    K.add('NeckRing', 'HeadBrass', {'neck_02': 1.0}, torus(0.059, 0.006, K.n(24, 48), K.n(6, 10)).at(at(0, 0.01, 0.035)))


# ------------------------------------------------------------------------------------------------------- the boiler

class Barrel:
    """The boiler's surface: superellipse rings through BOILER's rows (Kit.z's heights), shrink scaling them."""

    def __init__(self, K, shrink=1.0):
        rows = np.array(BOILER)
        self.zs = np.array([K.z(z) for z in rows[:, 0]])
        self.hw, self.hd, self.cy = rows[:, 1] * shrink, rows[:, 2] * shrink, rows[:, 3]

    def dims(self, z):
        return spline(self.zs, self.hw, z), spline(self.zs, self.hd, z), spline(self.zs, self.cy, z)

    def ring(self, t, z, off=0.0):
        hw, hd, cy = self.dims(z)
        return superellipse(hw + off, hd + off, cy, z, t, POWER)

    def front_y(self, x, z):
        hw, hd, cy = self.dims(z)
        return float(cy - hd * max(0.0, 1.0 - abs(x / hw) ** POWER) ** (1.0 / POWER))

    def out2d(self, t, z):
        """The ring's outward normal in plan at angles t."""
        e = 1e-4
        d = self.ring(t + e, z) - self.ring(t - e, z)
        n = np.stack([d[:, 1], -d[:, 0], np.zeros(len(d))], 1)
        return n / np.maximum(np.linalg.norm(n, axis=1, keepdims=True), 1e-12)

    def normal(self, t, z):
        e = 1e-4
        dt = self.ring(t + e, z) - self.ring(t - e, z)
        dz = self.ring(t, z + e) - self.ring(t, z - e)
        n = np.cross(dt, dz)
        n /= np.maximum(np.linalg.norm(n, axis=1, keepdims=True), 1e-12)
        return np.where(((n[:, :2] * self.out2d(t, z)[:, :2]).sum(1) < 0)[:, None], -n, n)

    def arc(self, z, n=4000):
        """(t, arc length) round the ring at z, from the back (t -pi)."""
        t = np.linspace(-math.pi, math.pi, n + 1)
        p = self.ring(t, z)
        s = np.concatenate([[0.0], np.cumsum(np.linalg.norm(np.diff(p, axis=0), axis=1))])
        return t, s


def _dedupe(vals, gap=0.0008):
    out = []
    for v in sorted(vals):
        if not out or v - out[-1] > gap:
            out.append(v)
    return np.array(out)


def barrel_section(K, bar, z0, z1, rows_lo, seams_z=()):
    """The boiler from z0 to z1, closed. The game mesh: 48 round, rows_lo high. The detail: ~1 cm squares, every plate
    seam (SEAMS_T upright, seams_z round) a groove GROOVE deep, rivets along each seam's lap."""
    if not K.hi:
        t = np.linspace(-math.pi, math.pi, 48, endpoint=False)
        return loft([bar.ring(t, z) for z in rows_lo]), None
    zm = 0.5 * (z0 + z1)
    tt, ss = bar.arc(zm)
    L = ss[-1]
    near = (-0.006, -0.003, -0.0015, 0.0, 0.0015, 0.003, 0.006)
    seam_s = [float(np.interp(t_, tt, ss)) for t_ in SEAMS_T]
    cols = list(np.arange(0.0, L, 0.011)) + [(s0 + d) % L for s0 in seam_s for d in near]
    cols = _dedupe(cols)
    t = np.interp(cols, ss, tt)
    rows = [z for z in list(np.arange(z0, z1, 0.006)) + [z1] + [zs + d for zs in seams_z for d in near] if z0 <= z <= z1]
    rows = _dedupe(rows, 0.0008)
    dv = np.min([np.minimum(np.abs(cols - s0), L - np.abs(cols - s0)) for s0 in seam_s], 0)
    rings = []
    for z in rows:
        dh = min([abs(z - zs) for zs in seams_z] + [1.0])
        d = GROOVE * np.maximum(np.exp(-(dv / 0.0018) ** 2), math.exp(-(dh / 0.0018) ** 2))
        rings.append(bar.ring(t, z) - bar.out2d(t, z) * d[:, None])
    # the laps' rivets: 8 mm to one side of each seam, every 3.5 cm
    pts, nrm = [], []
    for s0 in seam_s:
        tr = np.array([np.interp((s0 + 0.008) % L, ss, tt)])
        for z in np.arange(z0 + 0.02, min(z1, K.z(1.55)) - 0.01, 0.035):
            pts.append(bar.ring(tr, z, 0.0004)[0])
            nrm.append(bar.normal(tr, z)[0])
    for zs in seams_z:
        tr = np.interp(np.arange(0.0, L, 0.035), ss, tt)
        pts += list(bar.ring(tr, zs - 0.008, 0.0004))
        nrm += list(bar.normal(tr, zs - 0.008))
    return loft(rings), rivets(pts, nrm, RIVET)


def barrel_band(bar, z, h, grow, n):
    """A brass band round the boiler at z, h half-high, standing grow proud, its edges chamfered, closed."""
    t = np.linspace(-math.pi, math.pi, n, endpoint=False)
    prof = [(-0.004, -h), (grow - 0.0015, -h), (grow, -h + 0.0025), (grow, h - 0.0025), (grow - 0.0015, h), (-0.004, h)]
    return loft([bar.ring(t, z + dz, off) for off, dz in prof], closed=True)


def band_rivets(bar, z, grow, count):
    t = np.linspace(-math.pi, math.pi, count, endpoint=False)
    return rivets(bar.ring(t, z, grow + 0.0003), bar.out2d(t, z), RIVET * 1.1)


def bend_onto(shape, bar, x0, z0):
    """A flat fitting laid on the boiler's front: each point moved back by how far the boiler curves away there."""
    V = shape.V.copy()
    f0 = bar.front_y(x0, z0)
    V[:, 1] += np.array([bar.front_y(x, z) - f0 for x, z in zip(V[:, 0], V[:, 2])])
    shape.V = V
    return shape


def build_torso(K):
    """The boiler chest in two plates' courses (upper on spine_05, lower on spine_03) under the chest band, the
    pressure gauge for a heart on the left breast, the crest on the right, the firebox in the belly; the collar the
    neck rises from, with the furnace's light in its seam; the drum across the back with the two stacks and the steam
    whistle; the bellows waist (spine_01); the girdle and belt (pelvis) and the side tassets over the hips."""
    up, lo = Barrel(K), Barrel(K, 0.99)                        # the lower course tucks inside the upper one
    Z_ = K.z
    seg = K.n(2, 4)
    n = K.n(48, 96)
    S5, S3 = {'spine_05': 1.0}, {'spine_03': 1.0}
    K.group = 'Torso'
    sh, rv = barrel_section(K, up, Z_(1.426), Z_(1.604), [Z_(z) for z in (1.426, 1.455, 1.49, 1.52, 1.545, 1.563, 1.578,
                                                                         1.59, 1.598, 1.604)], [Z_(z) for z in SEAMS_UPPER])
    K.add('BoilerUpper', 'BodyEnamel', S5, sh)
    if rv is not None:
        K.add('BoilerUpperRivets', 'BodyBrass', S5, rv, only='hi')
    K.add('ChestBand', 'BodyBrass', S5, barrel_band(up, Z_(CHEST_BAND), 0.019, 0.007, n))
    if K.hi:
        K.add('ChestBandRivets', 'BodyBrass', S5, band_rivets(up, Z_(CHEST_BAND), 0.007, 30), only='hi')
    # the collar: an iron drum round the neck, its brass ring, and the furnace's light in the seam under the head
    cy = K.H[1] + 0.01                                  # round the neck (its back clears the head looking up 25)
    K.add('Collar', 'BodyIron', S5, cyl(0.105, 0.068, K.n(32, 64), b=0.006, seg=seg).at(T((0.0, cy, Z_(1.556)))))
    K.add('CollarRing', 'BodyBrass', S5, torus(0.106, 0.0075, K.n(32, 64), K.n(6, 10)).at(T((0.0, cy, Z_(1.617)))))
    K.add('NeckGlow', 'Fire', S5, torus(0.084, 0.006, 32, 4).at(T((0.0, cy, Z_(1.624)))), only='lo')
    # the pressure gauge on the left breast: brass case and bezel, the ivory dial (its ticks, red arc and needle are the
    # look's), a pipe down into the boiler
    x, z, rd = 0.1, Z_(1.53), 0.056
    fy = up.front_y(x, z)
    face = lambda dy: T((x, fy + dy, z)) @ R(X, 90)
    K.add('GaugeCase', 'BodyBrass', S5, cyl(rd + 0.008, 0.056, n, b=0.004, seg=seg).at(face(0.03)))
    K.add('GaugeBezel', 'BodyBrass', S5, torus(rd + 0.004, 0.0068, K.n(36, 96), K.n(6, 12)).at(face(-0.026)))
    K.add('GaugeDial', 'BodyDial', S5, cyl(rd + 0.002, 0.002, n).at(face(-0.0245)))
    K.add('GaugePipe', 'BodyBrass', S5, tube([(x, fy - 0.01, z - rd - 0.006), (x + 0.05, fy + 0.002, z - rd - 0.024),
                                              (x + 0.11, fy + 0.03, z - 0.08)], 0.006, K.n(8, 12)))
    K.notes['dial'] = (x, z, rd)
    # the company's crest on the right breast: a brass-rimmed shield of red enamel, an eight-spoked driving wheel on it
    # (a grid laid on the boiler: a flat shield bent by its corners cut into the shoulder's rounding, 2026-10-19)
    x, z = -0.105, Z_(1.505)
    fy = up.front_y(x, z)
    k = 1.0 + 0.008 / 0.06
    front_y = np.vectorize(up.front_y)

    def shield(scale, lift):
        def P(u, v):                                   # u across -1..1, v up (m, the shield's own)
            w = np.where(v > -0.006, 0.06, 0.06 * np.clip((v + 0.075) / 0.069, 0.03, 1.0)) * scale
            xs, zs = x + u * w, z + v * scale
            return np.stack([xs, front_y(xs, zs) - lift, zs], -1)
        return P
    vs = np.unique(np.concatenate([np.linspace(-0.075, 0.06, K.n(8, 28)), [-0.006]]))
    us = np.linspace(-1.0, 1.0, K.n(7, 24))
    forward = lambda p: np.broadcast_to(np.array([0.0, -1.0, 0.0]), p.shape)
    K.add('CrestBack', 'BodyBrass', S5, sheet(shield(k, 0.003), us, vs, 0.009, forward))
    K.add('Crest', 'BodyRed', S5, sheet(shield(1.0, 0.008), us, vs, 0.009, forward))
    if K.hi:
        wheel = torus(0.03, 0.004, 48, 8) + cyl(0.008, 0.006, 16).at(T((0, 0, -0.003)))
        wheel += join(box((0.03, 0.0022, 0.002)).at(R(Z, 45 * i)) for i in range(4))
        for a in np.linspace(0.0, 2 * math.pi, 8, endpoint=False):          # the rim's bolts
            wheel += sphere(0.0035, 8, 4).at(T((0.03 * math.cos(a), 0.03 * math.sin(a), 0.003)))
        K.add('CrestWheel', 'BodyBrass', S5, bend_onto(wheel.at(T((x, fy - 0.012, z - 0.004)) @ R(X, 90)), up, x, z),
              only='hi')
    # the drum across the back (a tank: the stacks rise from it), its bands and rims
    # (0.34 m long: its ends stay clear of the pauldrons' inner backs as the arms swing)
    zd, yd, hl = Z_(1.555), 0.17, 0.17
    K.add('Drum', 'BodyEnamel', S5, cyl(0.10, 2 * hl, K.n(28, 64), b=0.01, seg=seg).at(T((-hl, yd, zd)) @ R(Y, 90)))
    K.add('DrumBands', 'BodyBrass', S5, join(ring(0.098, 0.1035, 0.02, K.n(28, 64)).at(T((xx - 0.01, yd, zd)) @ R(Y, 90))
                                             for xx in (-hl + 0.03, hl - 0.03)))
    K.add('DrumRims', 'BodyBrass', S5, join(torus(0.094, 0.006, K.n(24, 64), K.n(5, 8)).at(T((xx, yd, zd)) @ R(Y, 90))
                                            for xx in (-hl - 0.0005, hl + 0.0005)))
    if K.hi:
        pts, nrm = [], []
        for xx, sgn in ((-hl - 0.0015, -1.0), (hl + 0.0015, 1.0)):
            for a in np.linspace(0.0, 2 * math.pi, 10, endpoint=False):
                pts.append((xx, yd + 0.07 * math.cos(a), zd + 0.07 * math.sin(a)))
                nrm.append((sgn, 0.0, 0.0))
        for xx in (-hl + 0.03, hl - 0.03):
            for a in np.linspace(0.0, 2 * math.pi, 14, endpoint=False):
                pts.append((xx, yd + 0.1038 * math.cos(a), zd + 0.1038 * math.sin(a)))
                nrm.append((0.0, math.cos(a), math.sin(a)))
        K.add('DrumRivets', 'BodyBrass', S5, rivets(pts, nrm, RIVET), only='hi')
    # the stacks: iron, splayed 9 degrees, a brass band, the crown flared out to a brass lip, dark in the flue
    for s in (-1.0, 1.0):
        side = 'l' if s > 0 else 'r'
        base = np.array([s * 0.14, 0.14, Z_(1.55)])
        d = np.array([s * math.sin(math.radians(9.0)), 0.0, math.cos(math.radians(9.0))])
        F = frame(base, d, (1.0, 0.0, 0.0))
        ns = K.n(20, 48)
        # the crown a flared wall round the mouth (the flue dark at the tube's top inside it)
        K.add(f'Stack_{side}', 'BodyIron', S5, cyl(0.04, 0.33, ns).at(F) +
              lathe([(0.037, 0.0), (0.0405, 0.0), (0.063, 0.044), (0.059, 0.044)], ns, cap=False, closed=True).at(
                  F @ T((0, 0, 0.325))))
        K.add(f'StackBand_{side}', 'BodyBrass', S5, cyl(0.0435, 0.018, ns, b=0.002, seg=K.n(1, 2)).at(F @ T((0, 0, 0.22))))
        K.add(f'StackLip_{side}', 'BodyBrass', S5, ring(0.056, 0.066, 0.012, ns).at(F @ T((0, 0, 0.363))))
        K.add(f'StackFlue_{side}', 'BodyDark', S5, cyl(0.039, 0.003, ns).at(F @ T((0, 0, 0.329))))
        if K.hi:
            a = np.linspace(0.0, 2 * math.pi, 8, endpoint=False)
            loc = np.stack([0.0438 * np.cos(a), 0.0438 * np.sin(a), np.full(8, 0.229)], 1)
            M = F[:3, :3]
            K.add(f'StackRivets_{side}', 'BodyBrass', S5,
                  rivets(loc @ M.T + F[:3, 3], np.stack([np.cos(a), np.sin(a), np.zeros(8)], 1) @ M.T, RIVET * 0.8),
                  only='hi')
    # the steam whistle on the drum's left end: a brass bell on a stand-pipe, its slit, a cap, a pull lever
    w = np.array([0.15, 0.24, Z_(1.618)])
    nw = K.n(16, 32)
    K.add('Whistle', 'BodyBrass', S5, cyl(0.011, 0.06, nw).at(T(w)) + cyl(0.022, 0.075, nw, b=0.003).at(T(w + (0, 0, 0.055))) +
          lathe([(0.022 * math.cos(a), 0.016 * math.sin(a)) for a in np.radians(np.linspace(0, 90, K.n(4, 8)))], nw).at(
              T(w + (0, 0, 0.13))))
    K.add('WhistleSlit', 'BodyDark', S5, cyl(0.0228, 0.007, nw).at(T(w + (0, 0, 0.068))))
    K.add('WhistleLever', 'BodyIron', S5, box((0.03, 0.004, 0.004), b=0.0015, seg=1).at(T(w + (0.03, 0.0, 0.03)) @ R(Y, -20)))

    # --- the lower course: the firebox in the belly, the waist band under it (spine_03)
    K.group = 'Belly'
    sh, rv = barrel_section(K, lo, Z_(1.11), Z_(1.452), [Z_(z) for z in np.linspace(1.11, 1.452, 10)],
                            [Z_(z) for z in SEAMS_LOWER])
    K.add('BoilerLower', 'BodyEnamel', S3, sh)
    if rv is not None:
        K.add('BoilerLowerRivets', 'BodyBrass', S3, rv, only='hi')
    K.add('WaistBand', 'BodyBrass', S3, barrel_band(lo, Z_(1.122), 0.012, 0.006, n))
    if K.hi:
        K.add('WaistBandRivets', 'BodyBrass', S3, band_rivets(lo, Z_(1.122), 0.006, 26), only='hi')
    z = Z_(1.262)
    fy = lo.front_y(0.0, z)
    K.add('FireFrame', 'BodyDark', S3, box((0.118, 0.014, 0.082), b=0.006, seg=seg).at(T((0.0, fy - 0.003, z))))
    K.add('FireDoor', 'BodyIron', S3, box((0.09, 0.007, 0.064), b=0.005, seg=seg).at(T((0.0, fy - 0.022, z))))
    K.add('Louvres', 'Fire', S3, join(box((0.0065, 0.002, 0.026)).at(T((-0.044 + 0.022 * i, fy - 0.0295, z + 0.012)))
                                      for i in range(5)), only='lo')
    K.add('FireKnob', 'BodyBrass', S3, cyl(0.011, 0.018, K.n(12, 24), b=0.003, seg=K.n(1, 2)).at(
        T((0.068, fy - 0.027, z - 0.024)) @ R(X, 90)))
    K.add('FireHinges', 'BodyBrass', S3, join(cyl(0.008, 0.03, K.n(10, 20), b=0.002, seg=1).at(
        T((-0.092, fy - 0.026, z + dz - 0.015))) for dz in (0.04, -0.04)))
    if K.hi:
        pts = [(xx, fy - 0.0295, z + zz) for xx in (-0.08, 0.08) for zz in np.linspace(-0.05, 0.05, 4)]
        pts += [(xx, fy - 0.0175, z + zz) for xx in (-0.108, 0.108) for zz in np.linspace(-0.07, 0.07, 5)]
        K.add('FireRivets', 'BodyBrass', S3, rivets(pts, [(0.0, -1.0, 0.0)] * len(pts), RIVET * 0.9), only='hi')

    # --- the waist: an iron bellows the boiler rocks on (spine_01)
    K.group = 'Waist'
    t = np.linspace(-math.pi, math.pi, K.n(40, 160), endpoint=False)
    # (its foot inside the girdle's top, no lower: crouched, the thighs come up under it)
    zs = [Z_(1.062), Z_(1.09), Z_(1.12), Z_(1.15)] if not K.hi else list(np.arange(Z_(1.062), Z_(1.15) + 1e-6, 0.00225))
    folds = lambda zz: 0.0035 * math.sin(2 * math.pi * (zz - Z_(1.062)) / 0.018) if K.hi else 0.0
    # (narrow: the boiler pitched 15 degrees on spine_03 brought its bottom band 1.7 cm into a 0.178 waist)
    K.add('Waist', 'BodyIron', {'spine_01': 1.0}, loft([superellipse(0.165 + folds(zz), 0.132 + folds(zz), 0.022, zz, t, POWER)
                                                      for zz in zs]))

    # --- the girdle round the hips (pelvis): high at the front, where the thighs rise in a crouch, low at the sides and
    # back; its brass belt; the side tassets over the hip joints ride the thighs going down
    K.group = 'Hips'
    top = Z_(1.075)
    hem_deg = np.array([0.0, 60.0, 85.0, 110.0, 180.0])
    hem_z = np.array([1.042, 1.040, 0.95, 0.925, 0.92])
    gw = lambda zz: 0.205 + 0.04 * smoothstep(Z_(1.075), Z_(1.0), zz)
    gd = lambda zz: 0.168 + 0.017 * smoothstep(Z_(1.075), Z_(0.92), zz)
    t = np.linspace(-math.pi, math.pi, K.n(40, 160), endpoint=False)
    theta =np.degrees(np.abs(np.angle(np.exp(1j * (t + math.pi / 2)))))      # 0 at the front, 180 at the back
    hem = np.array([Z_(h) for h in np.interp(theta, hem_deg, hem_z)])
    ss = np.linspace(0.0, 1.0, K.n(6, 30))

    def girdle_ring(s_, off):
        zz = top + s_ * (hem - top)
        c, sn = np.cos(t), np.sin(t)
        return np.stack([(gw(zz) + off) * np.sign(c) * np.abs(c) ** (2 / POWER),
                         0.018 + (gd(zz) + off) * np.sign(sn) * np.abs(sn) ** (2 / POWER), zz], 1)
    rings = [girdle_ring(s_, 0.0) for s_ in ss[::-1]] + [girdle_ring(s_, -0.012) for s_ in ss]
    K.add('Girdle', 'BodyEnamel', {'pelvis': 1.0}, loft(rings, closed=True))
    zb = Z_(1.058)
    t2 = np.linspace(-math.pi, math.pi, n, endpoint=False)
    prof = [(-0.004, -0.013), (0.0045, -0.013), (0.006, -0.0105), (0.006, 0.0105), (0.0045, 0.013), (-0.004, 0.013)]
    belt = lambda off, dz: superellipse(gw(zb + dz) + off, gd(zb + dz) + off, 0.018, zb + dz, t2, POWER)
    K.add('Belt', 'BodyBrass', {'pelvis': 1.0}, loft([belt(o, d) for o, d in prof], closed=True))
    if K.hi:
        tr = np.linspace(-math.pi, math.pi, 30, endpoint=False)
        p = superellipse(gw(zb) + 0.0063, gd(zb) + 0.0063, 0.018, zb, tr, POWER)
        K.add('BeltRivets', 'BodyBrass', {'pelvis': 1.0}, rivets(p, np.stack([p[:, 0], p[:, 1] - 0.018, 0 * p[:, 0]], 1),
                                                               RIVET * 1.1), only='hi')
    for s in (-1.0, 1.0):
        side = 'l' if s > 0 else 'r'
        K.group = f'Tasset_{side}'
        za, zb_ = Z_(1.055), Z_(0.82)
        rr = lambda v: 0.262 + 0.04 * smoothstep(za, zb_, v)

        def P(u, v):
            a = u if s > 0 else math.pi - u
            return np.stack([rr(v) * np.cos(a), 0.018 + rr(v) * np.sin(a) * 0.9, v], -1)
        us = np.linspace(-0.42, 0.42, K.n(9, 48))
        vs = np.linspace(za, zb_, K.n(5, 36))
        shp = sheet(P, us, vs, 0.014, lambda p: (p - np.array([0.0, 0.018, 0.0])) * np.array([1.0, 1.0, 0.0]))
        # on the hips alone: ridden down onto the thighs they stretched like rubber in a crouch; they hang clear of
        # the thighs' swing instead
        bones = {'pelvis': 1.0}
        K.add(f'Tasset_{side}', 'LegsEnamel', bones, shp)
        if K.hi:
            pts = [P(np.array(u), np.array(v)) for u in np.linspace(-0.37, 0.37, 7) for v in (za - 0.014,)]
            pts += [P(np.array(u), np.array(v)) for u in (-0.37, 0.37) for v in np.linspace(za - 0.05, zb_ + 0.02, 5)]
            pts = np.array(pts)
            nrm = np.stack([pts[:, 0], (pts[:, 1] - 0.018), 0 * pts[:, 0]], 1)
            K.add(f'TassetRivets_{side}', 'LegsBrass', bones, rivets(pts, nrm, RIVET), only='hi')


def seam_strap(F, a, radii, z0, z1, width=0.014, rise=0.0016, step=0.032):
    """A lap seam down a casing (the detail only): a strap rise proud of the casing (radii [(z, r), ...], its profile)
    at angle a round the frame's z axis, from z0 to z1, riveted every step."""
    zr = np.array(radii)
    r_at = lambda z: np.interp(z, zr[:, 0], zr[:, 1])

    def P(u, v):
        rr = r_at(v) + rise
        ang = a + u / rr
        return np.stack([rr * np.cos(ang), rr * np.sin(ang), v], -1)
    sh = sheet(P, np.linspace(-width / 2, width / 2, 4), np.linspace(z0, z1, 24), rise + 0.001,
               lambda p: p * np.array([1.0, 1.0, 0.0]))
    zs = np.arange(z0 + step / 2, z1 - step / 3, step)
    rad = np.array([math.cos(a), math.sin(a), 0.0])
    rv = rivets([rad * (r_at(z) + rise) + (0.0, 0.0, z) for z in zs], [rad] * len(zs), RIVET * 0.85)
    return sh.at(F), rv.at(F)


def add_seam(K, name, region, bones, F, a, radii, z0, z1):
    """seam_strap on a casing: the strap in the casing's enamel, its rivets brass (the detail only)."""
    if K.hi:
        strap, rv = seam_strap(F, a, radii, z0, z1)
        K.add(name, f'{region}Enamel', bones, strap, only='hi')
        K.add(name + 'Rivets', f'{region}Brass', bones, rv, only='hi')


def angle_of(F, v):
    """The angle round a frame's z axis that points along v."""
    return math.atan2(np.dot(v, F[:3, 1]), np.dot(v, F[:3, 0]))


# ------------------------------------------------------------------------------------------------------------ arms

def _quat(a, b):
    return Vector(tuple(unit(a))).rotation_difference(Vector(tuple(unit(b))))


def idle_arm(s):
    """The upper arm's way at the game's idle (looter_audit._arms_down)."""
    sx = 1.0 if s == 'l' else -1.0
    a = math.radians(IDLE_ARM)
    return unit((sx * math.sin(a), -0.04, -math.cos(a)))


def build_arm(K, s):
    """A domed pauldron on the shoulder (half on the collarbone, half on the upper arm; made for the idle's arm and
    carried up into the bind pose by that half), the shoulder's ball, a piston-rod upper arm in an enamel casing, a
    brass elbow ball between iron hinge caps, a heavy forearm in two casings (the wrist's on lowerarm_twist_02, so the
    hand's twist turns it; the band over their seam on lowerarm_twist_01) with a piston, the brass wrist cuff, and the
    three-fingered hand."""
    sk = K.sk
    sx = 1.0 if s == 'l' else -1.0
    S0, E = sk[f'upperarm_{s}'][0], sk[f'upperarm_{s}'][1]
    W = sk[f'lowerarm_{s}'][1]
    du, dl = unit(E - S0), unit(W - E)
    L1, L2 = np.linalg.norm(E - S0), np.linalg.norm(W - E)
    seg = K.n(2, 4)
    n = K.n(20, 48)
    UA, LA = {f'upperarm_{s}': 1.0}, {f'lowerarm_{s}': 1.0}
    T1, T2 = {f'lowerarm_twist_01_{s}': 1.0}, {f'lowerarm_twist_02_{s}': 1.0}
    out = perp((sx, 0.0, 0.0), du)
    back_u, back_l = perp((0.0, 1.0, 0.0), du), perp((0.0, 1.0, 0.0), dl)
    Fu, Fl = frame(S0, du, FWD), frame(E, dl, FWD)

    # --- the pauldron, made at the idle and carried into the bind pose
    K.group = f'Pauldron_{s}'
    q = Quaternion().slerp(_quat(idle_arm(s), du), PAULDRON_ARM)
    carry = T(S0) @ np.array(q.to_matrix().to_4x4()) @ T(-S0)
    C = S0 + np.array([-sx * 0.005, 0.0, 0.045])
    Mp = carry @ T(C) @ R(Y, sx * PAULDRON_TILT)
    PB = {f'clavicle_{s}': 1.0 - PAULDRON_ARM, f'upperarm_{s}': PAULDRON_ARM}
    rx, ry, rz = 0.175, 0.182, 0.17
    nd = K.n(32, 80)
    sc = S(1.0, ry / rx, 1.0)

    def squeeze(sh):
        """The inner half (toward the neck) drawn in to PAULDRON_INNER: a full round dome's inner skirt came down
        across the gauge and the crest at the idle; drawn in, it stays inside the boiler's shoulder."""
        V = sh.V.copy()
        m = V[:, 0] * sx < 0
        V[m, 0] *= PAULDRON_INNER
        return gg_shape(V, sh.F)
    K.add(f'Pauldron_{s}', f'ArmsEnamel', PB, squeeze(dome_shell(rx, ry, rz, 0.012, nd, K.n(9, 20))).at(Mp))
    K.add(f'PauldronRim_{s}', 'ArmsBrass', PB, squeeze(torus(rx + 0.002, 0.012, nd, K.n(5, 10)).at(sc)).at(Mp))
    K.add(f'PauldronSkirt_{s}', 'ArmsEnamel', PB, squeeze(join(
        lathe([(r0 - 0.01, z0), (r0, z0), (r1, z1), (r1 - 0.01, z1)], nd, cap=False, closed=True).at(sc)
        for r0, z0, r1, z1 in ((rx, -0.004, rx * 1.07, -0.06), (rx * 1.0, -0.055, rx * 1.09, -0.11)))).at(Mp))
    if K.hi:
        pts, nrm = [], []
        for ph, cnt in ((24.0, 20), (52.0, 12)):                    # rows round the dome
            for a in np.linspace(0.0, 2 * math.pi, cnt, endpoint=False):
                c, sn = math.cos(math.radians(ph)), math.sin(math.radians(ph))
                p = np.array([rx * c * math.cos(a), ry * c * math.sin(a), rz * sn])
                pts.append(p)
                nrm.append(p / np.array([rx * rx, ry * ry, rz * rz]))
        for ph in np.radians([70.0, 80.0]):
            for a in np.radians([0.0, 90.0, 180.0, 270.0]):
                p = np.array([rx * math.cos(ph) * math.cos(a), ry * math.cos(ph) * math.sin(a), rz * math.sin(ph)])
                pts.append(p)
                nrm.append(p / np.array([rx * rx, ry * ry, rz * rz]))
        for a in np.linspace(0.0, 2 * math.pi, 24, endpoint=False):  # round the skirt
            p = np.array([rx * 1.045 * math.cos(a), ry * 1.045 * math.sin(a), -0.036])
            pts.append(p)
            nrm.append((math.cos(a), math.sin(a), 0.1))
        pts = np.array(pts)
        nrm = np.array(nrm, dtype=np.float64)
        inner = pts[:, 0] * sx < 0
        pts[inner, 0] *= PAULDRON_INNER
        nrm[inner, 0] /= PAULDRON_INNER
        M = Mp[:3, :3]
        K.add(f'PauldronRivets_{s}', 'ArmsBrass', PB, rivets(pts @ M.T + Mp[:3, 3], nrm @ M.T, RIVET * 1.2), only='hi')

    # --- the upper arm
    K.group = f'UpperArm_{s}'
    K.add(f'ShoulderBall_{s}', 'ArmsIron', UA, sphere(0.072, K.n(14, 40), K.n(7, 20)).at(T(S0)))
    # the casings end clear of the elbow (7 cm, more than the other casing's radius): bent to aim, they met in it
    ua_r = [(0.05, 0.067), (L1 * 0.5, 0.065), (L1 - 0.07, 0.058)]
    K.add(f'UpperArm_{s}', 'ArmsEnamel', UA, housing(ua_r, n, 0.008, seg).at(Fu))
    add_seam(K, f'UpperArmSeam_{s}', 'Arms', UA, Fu, angle_of(Fu, out), ua_r, 0.07, L1 - 0.105)
    K.add(f'UpperArmBand_{s}', 'ArmsBrass', UA, cyl(0.0665, 0.02, n, b=0.002, seg=K.n(1, 2)).at(Fu @ T((0, 0, L1 - 0.09))))
    pa = S0 + du * 0.09 + back_u * 0.07
    K.add(f'UpperArmPiston_{s}', 'ArmsIron', UA, cyl(0.013, 0.12, K.n(10, 20), b=0.002, seg=1).at(frame(pa, du)))
    K.add(f'UpperArmRod_{s}', 'ArmsBrass', UA, cyl(0.0065, L1 - 0.22, K.n(8, 16)).at(frame(pa + du * 0.1, du)))
    hinge = unit(np.cross(du, dl))
    K.add(f'ElbowCaps_{s}', 'ArmsIron', UA, join(cyl(0.037, 0.018, n, b=0.003, seg=K.n(1, 2)).at(
        frame(E + hinge * sg * 0.052, hinge * sg)) for sg in (-1.0, 1.0)))
    if K.hi:
        pts, nrm = [], []
        for zz in (0.075, L1 - 0.12):
            r = float(np.interp(zz, *np.array(ua_r).T)) + 0.0003
            for a in np.linspace(0.0, 2 * math.pi, 10, endpoint=False):
                pts.append(np.array([r * math.cos(a), r * math.sin(a), zz]))
                nrm.append(np.array([math.cos(a), math.sin(a), 0.0]))
        pts, nrm = np.array(pts), np.array(nrm)
        K.add(f'UpperArmRivets_{s}', 'ArmsBrass', UA, rivets(pts @ Fu[:3, :3].T + Fu[:3, 3], nrm @ Fu[:3, :3].T, RIVET),
              only='hi')

    # --- the elbow and forearm
    K.group = f'Forearm_{s}'
    K.add(f'Elbow_{s}', 'ArmsBrass', LA, sphere(0.063, K.n(18, 40), K.n(9, 20)).at(T(E)))
    fa_r = [(0.066, 0.064), (L2 * 0.32, 0.078), (L2 * 0.5, 0.074)]
    # (the wrist's casing and cuff stop 3 cm short of the wrist: the hand bent 30 degrees on a gun's grip met them)
    fb_r = [(L2 * 0.47, 0.073), (L2 * 0.84, 0.066)]
    a_out = angle_of(Fl, perp((sx, 0.0, 0.0), dl))
    K.add(f'ForearmA_{s}', 'ArmsEnamel', LA, housing(fa_r, n, 0.008, seg).at(Fl))
    add_seam(K, f'ForearmASeam_{s}', 'Arms', LA, Fl, a_out, fa_r, 0.07, L2 * 0.45)
    K.add(f'ForearmBand_{s}', 'ArmsBrass', T1, cyl(0.0785, 0.024, n, b=0.003, seg=K.n(1, 2)).at(Fl @ T((0, 0, L2 * 0.475 - 0.012))))
    K.add(f'ForearmB_{s}', 'ArmsEnamel', T2, housing(fb_r, n, 0.006, seg).at(Fl))
    add_seam(K, f'ForearmBSeam_{s}', 'Arms', T2, Fl, a_out, fb_r, L2 * 0.51, L2 * 0.77)
    K.add(f'WristCuff_{s}', 'ArmsBrass', T2, cyl(0.07, 0.026, n, b=0.004, seg=K.n(1, 2)).at(Fl @ T((0, 0, L2 * 0.80))))
    pf = E + dl * 0.07 + back_l * 0.077
    K.add(f'ForearmPiston_{s}', 'ArmsIron', LA, cyl(0.012, 0.11, K.n(10, 20), b=0.002, seg=1).at(frame(pf, dl)))
    if K.hi:
        pts, nrm = [], []
        for zz, r in ((z_, float(np.interp(z_, *np.array(fa_r).T)) + 0.0003) for z_ in (0.085, L2 * 0.42)):
            for a in np.linspace(0.0, 2 * math.pi, 10, endpoint=False):
                pts.append(np.array([r * math.cos(a), r * math.sin(a), zz]))
                nrm.append(np.array([math.cos(a), math.sin(a), 0.0]))
        K.add(f'ForearmRivets_{s}', 'ArmsBrass', LA, rivets(np.array(pts) @ Fl[:3, :3].T + Fl[:3, 3],
                                                           np.array(nrm) @ Fl[:3, :3].T, RIVET), only='hi')
        pts, nrm = [], []
        for zz in (L2 * 0.58, L2 * 0.74):
            for a in np.linspace(0.0, 2 * math.pi, 8, endpoint=False):
                r = float(np.interp(zz, *np.array(fb_r).T))
                pts.append(np.array([(r + 0.0003) * math.cos(a), (r + 0.0003) * math.sin(a), zz]))
                nrm.append(np.array([math.cos(a), math.sin(a), 0.0]))
        K.add(f'ForearmBRivets_{s}', 'ArmsBrass', T2, rivets(np.array(pts) @ Fl[:3, :3].T + Fl[:3, 3],
                                                            np.array(nrm) @ Fl[:3, :3].T, RIVET), only='hi')
    build_hand(K, s)


def build_hand(K, s):
    """Three riveted fingers (thumb, and two on index_* and middle_*), block segments on brass hinge pins, a boxy palm
    with a brass knuckle bar, an iron wrist ball."""
    K.group = f'Hand_{s}'
    sk = K.sk
    W = sk[f'hand_{s}'][0]
    I0, M0 = sk[f'index_01_{s}'][0], sk[f'middle_01_{s}'][0]
    knuck = 0.5 * (I0 + M0)
    along = unit(knuck - W)
    lat = perp(M0 - I0, along)
    back = np.cross(along, lat)
    thumb = sk[f'thumb_03_{s}'][1]
    if (thumb - W) @ back > 0:                         # the thumb is on the palm's side
        back = -back
    if np.cross(lat, along) @ back < 0:
        lat = -lat
    Fh = np.eye(4)
    Fh[:3, 0], Fh[:3, 1], Fh[:3, 2], Fh[:3, 3] = lat, along, back, W
    Lp = np.linalg.norm(knuck - W)
    xc = (knuck - W) @ lat
    HB = {f'hand_{s}': 1.0}
    seg = K.n(2, 3)
    K.notes.setdefault('finger_gap', []).append(float(abs((M0 - I0) @ lat)))
    K.add(f'Palm_{s}', 'ArmsEnamel', HB, box((0.052, Lp * 0.5 - 0.004, 0.031), (1.0, 1.0), 0.012, seg).at(
        Fh @ T((xc, Lp * 0.5 + 0.008, 0.0))))
    K.add(f'Knuckles_{s}', 'ArmsBrass', HB, cyl(0.0135, 0.104, K.n(10, 20), b=0.003, seg=1).at(
        Fh @ T((xc - 0.052, Lp - 0.004, 0.013)) @ R(Y, 90)))
    K.add(f'WristBall_{s}', 'ArmsIron', HB, sphere(0.048, K.n(16, 32), K.n(8, 16)).at(T(W)))
    if K.hi:
        pts = [(xc + dx, Lp * a, 0.0312) for dx in (-0.032, 0.032) for a in (0.3, 0.72)]
        pts = np.array(pts) @ Fh[:3, :3].T + W
        K.add(f'PalmRivets_{s}', 'ArmsBrass', HB, rivets(pts, [back] * len(pts), RIVET * 1.1), only='hi')
    for chain, width, thick in (('thumb', 0.042, 0.038), ('index', 0.041, 0.036), ('middle', 0.041, 0.036)):
        for i in (1, 2, 3):
            bone = f'{chain}_0{i}_{s}'
            h, t_ = sk[bone][0], sk[bone][1]
            ax = unit(t_ - h)
            nb = perp(back, ax)
            lb = np.cross(ax, nb)
            F = np.eye(4)
            F[:3, 0], F[:3, 1], F[:3, 2], F[:3, 3] = lb, ax, nb, h
            L = np.linalg.norm(t_ - h)
            tip = i == 3
            K.add(f'Finger_{chain}_0{i}_{s}', 'ArmsEnamel', {bone: 1.0},
                  box((width / 2, L / 2 + (0.006 if tip else 0.004), thick / 2), (1.0, 1.0), 0.009 if tip else 0.006,
                      K.n(1, 3)).at(F @ T((0.0, L / 2 + (0.003 if tip else 0.0), 0.0))))
            if i > 1:
                prev = f'{chain}_0{i - 1}_{s}'
                K.add(f'Pin_{chain}_0{i}_{s}', 'ArmsBrass', {prev: 1.0},
                      cyl(0.0095, width + 0.006, K.n(8, 16), b=0.002, seg=1).at(F @ T((-(width + 0.006) / 2, 0.0, 0.0)) @ R(Y, 90)))
            if K.hi:
                K.add(f'FingerRivet_{chain}_0{i}_{s}', 'ArmsBrass', {bone: 1.0},
                      rivets([h + ax * L * 0.5 + nb * (thick / 2 + 0.0003)], [nb], RIVET * 0.9), only='hi')


# ------------------------------------------------------------------------------------------------------------ legs

def build_leg(K, s):
    """An iron hip ball, a piston-rod thigh in an enamel casing with a brass band and a piston, a brass knee ball under
    an enamel knee cap, a shin casing with a raised front plate, an iron ankle ball, and a broad enamel foot on an iron
    sole with a brass toe cap (on ball_*)."""
    sk = K.sk
    sx = 1.0 if s == 'l' else -1.0
    Hp, Kn = sk[f'thigh_{s}'][0], sk[f'thigh_{s}'][1]
    A, Bl = sk[f'calf_{s}'][1], sk[f'foot_{s}'][1]
    Tp = sk[f'ball_{s}'][1]
    dt, dc = unit(Kn - Hp), unit(A - Kn)
    Lt, Lc = np.linalg.norm(Kn - Hp), np.linalg.norm(A - Kn)
    Ft, Fc = frame(Hp, dt, FWD), frame(Kn, dc, FWD)
    n = K.n(22, 48)
    seg = K.n(2, 4)
    TH, CA = {f'thigh_{s}': 1.0}, {f'calf_{s}': 1.0}
    K.group = f'Thigh_{s}'
    # the thigh's casing domes over the hip joint (a dome turns about its own centre: no gap opens at the hip in a
    # stride or a crouch, and its crown meets the belt)
    rh = 0.10
    K.add(f'Thigh_{s}', 'LegsEnamel', TH, lathe(arc(0.0, 0.0, rh, -90, 0, K.n(6, 14)) + [(0.106, Lt * 0.32)] +
                                               arc(0.092 - 0.009, Lt - 0.045 - 0.009, 0.009, 0, 90, seg + 1), n).at(Ft))
    K.add(f'ThighBand_{s}', 'LegsBrass', TH, cyl(0.0985, 0.022, n, b=0.003, seg=K.n(1, 2)).at(Ft @ T((0, 0, Lt - 0.13))))
    add_seam(K, f'ThighSeam_{s}', 'Legs', TH, Ft, 0.0, [(0.0, 0.10), (Lt * 0.32, 0.106), (Lt - 0.054, 0.092)], 0.12,
             Lt - 0.145)
    side = perp((sx, 0.6, 0.0), dt)
    pt = Hp + dt * 0.14 + side * 0.106
    K.add(f'ThighPiston_{s}', 'LegsIron', TH, cyl(0.014, 0.14, K.n(10, 20), b=0.002, seg=1).at(frame(pt, dt)))
    K.add(f'ThighRod_{s}', 'LegsBrass', TH, cyl(0.007, Lt - 0.33, K.n(8, 16)).at(frame(pt + dt * 0.12, dt)))
    if K.hi:
        pts, nrm = [], []
        th_r = lambda z: float(np.interp(z, [0.0, Lt * 0.32, Lt - 0.054], [0.10, 0.106, 0.092])) + 0.0003
        for zz, r in ((0.13, th_r(0.13)), (Lt * 0.55, th_r(Lt * 0.55))):
            for a in np.linspace(0.0, 2 * math.pi, 14, endpoint=False):
                pts.append(np.array([r * math.cos(a), r * math.sin(a), zz]))
                nrm.append(np.array([math.cos(a), math.sin(a), 0.0]))
        K.add(f'ThighRivets_{s}', 'LegsBrass', TH, rivets(np.array(pts) @ Ft[:3, :3].T + Ft[:3, 3],
                                                         np.array(nrm) @ Ft[:3, :3].T, RIVET), only='hi')
    K.group = f'Shin_{s}'
    K.add(f'Knee_{s}', 'LegsBrass', CA, sphere(0.072, K.n(18, 40), K.n(9, 20)).at(T(Kn)))
    fc = perp(FWD, dc)
    latk = unit(np.cross(dc, fc))

    def Pk(u, v):                                   # a guard curved round the knee's hinge: u across, v up from level
        return Kn + latk * u[..., None] + 0.104 * (np.cos(v)[..., None] * fc - np.sin(v)[..., None] * dc)
    # thick, down onto the knee's ball (thin, it stood off the ball like a crescent)
    K.add(f'KneeCap_{s}', 'LegsEnamel', CA, sheet(Pk, np.linspace(-0.074, 0.074, K.n(6, 30)),
                                                  np.linspace(-0.8, 0.72, K.n(8, 34)), 0.034,
                                                  lambda p: (p - Kn) - np.outer((p - Kn).reshape(-1, 3) @ latk, latk).reshape(p.shape)))
    if K.hi:
        pts = np.array([Pk(np.array(u), np.array(v)) for u in (-0.06, 0.06) for v in (-0.6, -0.2, 0.2, 0.58)])
        K.add(f'KneeRivets_{s}', 'LegsBrass', CA, rivets(pts, [unit(p - Kn - latk * ((p - Kn) @ latk)) for p in pts],
                                                        RIVET), only='hi')
    # the shin's casing starts as far below the knee as the thigh's is wide: crouched (the knee at 90 degrees) the two
    # casings met 4.8 cm deep behind it; the knee guard covers the gap in front
    sh_r = [(0.10, 0.09), (Lc * 0.32, 0.097), (Lc - 0.05, 0.074)]
    K.add(f'Shin_{s}', 'LegsEnamel', CA, housing(sh_r, n, 0.009, seg).at(Fc))
    add_seam(K, f'ShinSeam_{s}', 'Legs', CA, Fc, angle_of(Fc, perp((sx, 0.0, 0.0), dc)), sh_r, 0.13, Lc - 0.08)
    K.add(f'ShinBand_{s}', 'LegsBrass', CA, cyl(0.0925, 0.022, n, b=0.003, seg=K.n(1, 2)).at(Fc @ T((0, 0, 0.11))))
    lat = np.cross(dc, fc)
    Fp = np.eye(4)
    Fp[:3, 0], Fp[:3, 1], Fp[:3, 2] = lat, fc, dc
    Fp[:3, 3] = Kn + dc * (Lc * 0.47) + fc * 0.088
    K.add(f'ShinPlate_{s}', 'LegsEnamel', CA, box((0.056, 0.013, Lc * 0.28), (0.85, 1.0), 0.006, seg).at(Fp))
    pc = Kn + dc * 0.12 - fc * 0.088
    K.add(f'CalfPiston_{s}', 'LegsIron', CA, cyl(0.013, 0.13, K.n(10, 20), b=0.002, seg=1).at(frame(pc, dc)))
    K.add(f'CalfRod_{s}', 'LegsBrass', CA, cyl(0.0065, Lc - 0.31, K.n(8, 16)).at(frame(pc + dc * 0.12, dc)))
    if K.hi:
        pts = [Fp @ np.array([x, 0.0123, z, 1.0]) for x in (-0.036, 0.036) for z in np.linspace(-Lc * 0.24, Lc * 0.24, 4)]
        pts = np.array(pts)[:, :3]
        K.add(f'ShinRivets_{s}', 'LegsBrass', CA, rivets(pts, [fc] * len(pts), RIVET), only='hi')

    # --- the foot (foot_*) and its toe cap (ball_*), standing on the ground
    K.group = f'Foot_{s}'
    FO, BA = {f'foot_{s}': 1.0}, {f'ball_{s}': 1.0}
    K.add(f'Ankle_{s}', 'LegsIron', FO, sphere(0.064, K.n(16, 36), K.n(8, 18)).at(T(A)))
    f = unit(np.array([Bl[0] - A[0], Bl[1] - A[1], 0.0]))
    lt = np.array([-f[1], f[0], 0.0])                         # +x for a foot facing -y
    o = np.array([A[0], A[1], 0.0])
    ub = (Bl - o) @ f                                          # the ball's place along the foot
    ut = (Tp - o) @ f + 0.02
    nr = K.n(16, 40)
    a = np.linspace(-math.pi, math.pi, nr, endpoint=False)

    def section(u, hw, top, bot=0.014, p=4.0):
        c, sn = np.cos(a), np.sin(a)
        mid, hh = 0.5 * (top + bot), 0.5 * (top - bot)
        xx = hw * np.sign(c) * np.abs(c) ** (2 / p)
        zz = mid + hh * np.sign(sn) * np.abs(sn) ** (2 / p)
        return o + np.outer(np.full(nr, u), f) + np.outer(xx, lt) + np.outer(zz, (0.0, 0.0, 1.0))
    us = np.linspace(-0.085, ub + 0.008, K.n(9, 30))
    hw = np.interp(us, [-0.085, 0.0, ub], [0.056, 0.064, 0.07])
    tp = np.interp(us, [-0.085, -0.05, 0.0, 0.06, ub], [0.08, 0.105, 0.12, 0.098, 0.07])
    rounds = [(-0.093, 0.7, 0.85), (-0.089, 0.9, 0.96)]        # the heel rounded off
    rings = [section(u, np.interp(u, us, hw) * k, 0.014 + (np.interp(u, us, tp) - 0.014) * kz) for u, k, kz in rounds]
    rings += [section(u, w_, tp_) for u, w_, tp_ in zip(us, hw, tp)]
    K.add(f'Foot_{s}', 'LegsEnamel', FO, loft(rings))
    ut_ = np.linspace(ub - 0.012, ut, K.n(7, 24))
    tw = np.interp(ut_, [ub, ut], [0.072, 0.056])
    tt = np.interp(ut_, [ub, ut - 0.03, ut], [0.074, 0.056, 0.04])
    rings = [section(u, w_, t_) for u, w_, t_ in zip(ut_, tw, tt)]
    rings += [section(ut + 0.008, 0.047, 0.034, 0.017), section(ut + 0.012, 0.032, 0.029, 0.02)]
    K.add(f'Toe_{s}', 'LegsBrass', BA, loft(rings))
    sole = lambda u0, u1, w0, w1: loft([section(u, w_, 0.016, 0.0, 8.0) for u, w_ in
                                       zip(np.linspace(u0, u1, K.n(6, 24)), np.linspace(w0, w1, K.n(6, 24)))])
    K.add(f'Sole_{s}', 'LegsDark', FO, sole(-0.092, ub, 0.062, 0.076))
    K.add(f'ToeSole_{s}', 'LegsDark', BA, sole(ub - 0.004, ut + 0.012, 0.076, 0.05))
    if K.hi:
        pts, nrm = [], []
        for u in np.linspace(ub + 0.005, ut - 0.015, 4):
            for sg in (-1.0, 1.0):
                w_ = np.interp(u, [ub, ut], [0.072, 0.056])
                pts.append(o + f * u + lt * sg * (w_ + 0.0003) + np.array([0, 0, 0.035]))
                nrm.append(lt * sg)
        K.add(f'ToeRivets_{s}', 'LegsBrass', BA, rivets(pts, nrm, RIVET), only='hi')
        pts, nrm = [], []
        for u in np.linspace(-0.07, ub - 0.02, 5):
            for sg in (-1.0, 1.0):
                w_ = np.interp(u, us, hw)
                pts.append(o + f * u + lt * sg * (w_ + 0.0003) + np.array([0, 0, 0.04]))
                nrm.append(lt * sg)
        K.add(f'FootRivets_{s}', 'LegsBrass', FO, rivets(pts, nrm, RIVET), only='hi')


def build(sk, hi):
    """Every part at one level of detail (Kit)."""
    K = Kit(sk, hi)
    build_head(K)
    build_torso(K)
    for s in 'lr':
        build_arm(K, s)
        build_leg(K, s)
    return K
