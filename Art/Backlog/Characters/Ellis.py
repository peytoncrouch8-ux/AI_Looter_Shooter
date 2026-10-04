"""Ellis Ransom, the Revenant: the story's hero (Docs/Story.md) and one of the five playable heroes concepted on
2026-10-04. Kept in the backlog (Art/Backlog/README.md): nothing in the game uses it yet.

The look follows the HUD portrait (Art/Icons/HudPortrait.svg): a wide-brimmed cattleman hat, a red bandana over the
nose and mouth, eyes burning cyan in the brim's shadow (Ellis is dead, raised by Sexton), and a long dusty duster worn
open over a gun belt of brass cartridges, a cross-draw holster, and Pa's lantern clipped to the right hip.

Ember powers (a design sketch, not in the game): Dust Devil (the approved dash), Sundown (time slows round Ellis for
6 s) and Borrowed Time (for 8 s damage is written in Sexton's ledger instead, then paid back; kills strike some off).

The rig is the UE5 mannequin's skeleton (looter_heroes): the mannequin's animations can drive it through an IK
retargeter. 1.80 m tall, the mannequin's height.
"""
import math

import numpy as np

import looter_heroes as lh
from looter_heroes import v3

PALETTE = {
    'coat': (0x8f7052, 'fill'), 'coatDark': (0x624b39, 'fill'), 'shirt': (0x7d93a8, 'fill'),
    'pants': (0x4c4852, 'fill'), 'boots': (0x4a3328, 'fill'), 'leather': (0x6a4a33, 'fill'),
    'brass': (0xd1a54e, 'accent'), 'hat': (0x3f3537, 'fill'), 'hatband': (0x6a4a33, 'fill'),
    'bandana': (0xc8443a, 'accent'), 'skin': (0xc9b8a8, 'fill'), 'hair': (0x3a3032, 'fill'), 'brow': (0x2e2628, 'fill'),
    'eyes': (0x5ac8ff, 'glow'), 'gloves': (0xa47c56, 'fill'), 'glass': (0xffc65c, 'glow'), 'metal': (0x5f6670, 'fill'),
    'grip': (0x5a4030, 'fill'), 'vest': (0x3f3b42, 'fill'),
}
HEAD = dict(rx=0.088, ry=0.104, rz=0.12, jaw=0.8, chin=0.012, back=1.0)


def build():
    b = lh.Build(height=1.80, shoulders=0.42, hips=0.21, head=1.12, neck=0.7, hand=1.12, foot=1.05, arm_angle=45.0)
    hero = lh.Hero('Ellis', b, PALETTE)
    sk = hero.sk
    T = sk.T
    zt = lambda t: sk.hip_z + t * T

    # --- under the coat: shirt, trousers, boots, gloves
    shirt = [(-0.16, 0.14, 0.098, 0.0), (-0.06, 0.163, 0.112, 0.0), (0.05, 0.17, 0.116, 0.0), (0.2, 0.163, 0.112, 0.0),
             (0.36, 0.158, 0.11, 0.004), (0.52, 0.172, 0.118, 0.01), (0.66, 0.188, 0.126, 0.014),
             (0.79, 0.198, 0.124, 0.012), (0.87, 0.2, 0.116, 0.006), (0.94, 0.176, 0.1, 0.002),
             (0.985, 0.12, 0.08, 0.0), (1.0, 0.075, 0.066, 0.0)]
    hero.add('Shirt', lh.torso(sk, shirt), 'shirt', bones=lh.TORSO_BONES)
    for side, sx in lh.SIDES:
        leg = lambda t: float(np.interp(t, [-0.12, 0.0, 0.25, 0.5, 0.7, 0.95], [0.098, 0.096, 0.083, 0.064, 0.064, 0.055]))
        hero.add(f'Trousers_{side}', lh.trouser(sk, side, leg, -0.12, 0.95), 'pants', bones=lh.LEG_BONES(side))
        shaft, foot = lh.boot(sk, side, shaft_top=0.44, shaft_r=0.068, cuff=0.01, width=0.054)
        hero.add(f'BootShaft_{side}', shaft, 'boots', bones=lh.FOOT_BONES(side))
        hero.add(f'Boot_{side}', foot, 'boots', bones=lh.FOOT_BONES(side))
        for i, (shape, how) in enumerate(lh.hand_shapes(sk, side, glove=1.08)):
            hero.add(f'Glove_{side}_{i}', shape, 'gloves', bones=how[1], power=how[2])
        gauntlet = lh.sleeve(sk, side, lambda t: 0.047 + 0.016 * (t - 0.86) / 0.16, start=0.86, end=1.02)
        hero.add(f'Gauntlet_{side}', gauntlet, 'gloves', bones={f'lowerarm_{side}': 1.0, f'hand_{side}': 1.0})

    # --- a dark waistcoat over the shirt, brass buttons, and the shirt's collar round the neck
    vest_rows = []
    for t, gap in ((0.9, 0.55), (0.8, 0.42), (0.66, 0.26), (0.52, 0.1), (0.36, 0.05), (0.2, 0.05), (0.08, 0.05)):
        w, d, f = (np.interp(t, [r[0] for r in shirt], [r[i] for r in shirt]) for i in (1, 2, 3))
        vest_rows.append(dict(z=zt(t), rx=w + 0.008, ry=d + 0.008, fwd=f, gap=gap))
    vest, _ = lh.garment(sk, vest_rows, n=40, thickness=0.006)
    hero.add('Vest', vest, 'vest', bones=lh.TORSO_BONES)
    for k, t in enumerate((0.5, 0.4, 0.3, 0.2)):
        z = zt(t)
        y = lh.spine_at(sk, z)[1] - np.interp(t, [r[0] for r in shirt], [r[2] for r in shirt]) - 0.016
        hero.add(f'VestButton_{k}', lh.ellipsoid(v3(0.0, y, z), (0.008, 0.005, 0.008), 10, 6), 'brass', rigid='spine_02')
    hero.add('ShirtCollar', lh.band(v3(0, sk.head('neck_01')[1] - 0.002, 0), (0.062, 0.058), sk.neck_z - 0.01,
                                    sk.neck_z + 0.045, 24), 'shirt', bones={'neck_01': 1.0, 'neck_02': 0.5, 'spine_05': 0.6})

    # --- the duster: one garment from the collar to the calves, open in a V above the waist and wider below it
    waist = zt(0.2)
    rows = []
    for t, gap in ((0.985, 0.95), (0.94, 0.72), (0.87, 0.56), (0.79, 0.44), (0.66, 0.30), (0.52, 0.2), (0.36, 0.14),
                   (0.2, 0.15)):
        w, d, f = (np.interp(t, [r[0] for r in shirt], [r[i] for r in shirt]) for i in (1, 2, 3))
        rows.append(dict(z=zt(t), rx=w + 0.018, ry=d + 0.018, fwd=f, gap=gap))
    hem_z = 0.40
    for i in range(1, 9):
        u = i / 8
        z = waist + (hem_z - waist) * u
        rows.append(dict(z=z, rx=0.188 + 0.085 * u ** 1.2, ry=0.13 + 0.09 * u ** 1.2, fwd=-0.012 * u, gap=0.17 + 0.42 * u))
    coat, edges = lh.garment(sk, rows, n=48, thickness=0.008)
    hero.add('Coat', coat, 'coat', weights=lh.garment_weights(hero, waist, stiff=0.85, width=0.32))
    top_rows = list(range(0, 8))
    widths = [0.012, 0.03, 0.05, 0.056, 0.05, 0.036, 0.02, 0.01]
    for sgn, side in ((1, 'l'), (-1, 'r')):
        hero.add(f'Lapel_{side}', lh.lapel_strip(edges, top_rows, widths, sgn), 'coatDark', bones=lh.TORSO_BONES)
    for side, sx in lh.SIDES:
        sleeve_r = lambda t: float(np.interp(t, [-0.12, -0.05, 0.04, 0.2, 0.5, 0.75, 0.93], [0.05, 0.078, 0.088, 0.077, 0.065, 0.063, 0.06]))
        hero.add(f'Sleeve_{side}', lh.sleeve(sk, side, sleeve_r, -0.12, 0.93), 'coat', bones=lh.ARM_BONES(side))
        hero.add(f'Cuff_{side}', lh.sleeve(sk, side, lambda t: 0.068, start=0.78, end=0.94), 'coatDark',
                 bones={f'lowerarm_{side}': 1.0, f'hand_{side}': 0.3})
        # A pocket flap on each front panel, just below the belt, seated on the coat's surface.
        z = waist - 0.13
        r = min(rows[8:], key=lambda row: abs(row['z'] - z))
        x = sx * 0.125
        y = -r['ry'] * math.sqrt(max(0.0, 1.0 - (x / r['rx']) ** 2)) + r.get('fwd', 0.0) * -1.0
        normal = lh.unit(v3(x / r['rx'] ** 2, y / r['ry'] ** 2, 0.0))
        tangent = lh.unit(np.cross(v3(0, 0, 1), normal))
        flap = lh.box(v3(x, y, z) + normal * 0.008, (0.105, 0.042, 0.01), axes=(tangent, v3(0, 0, 1), None), bevel=0.15)
        hero.add(f'Pocket_{side}', flap, 'coatDark', weights=lh.garment_weights(hero, waist, stiff=0.85, width=0.32))
    collar = []
    base = sk.head('neck_01')
    for dz, rx, ry in ((-0.025, 0.084, 0.074), (0.02, 0.088, 0.078), (0.05, 0.096, 0.086)):
        collar.append(lh.arc(base + v3(0, 0.006, dz), v3(1, 0, 0), v3(0, -1, 0), rx, ry, math.pi / 2 + 0.9,
                             math.pi / 2 + 2 * math.pi - 0.9, 28))
    hero.add('Collar', lh.thicken(lh.sheet(collar), 0.007), 'coatDark',
             bones={'spine_05': 1.0, 'neck_01': 0.7, 'clavicle_l': 0.3, 'clavicle_r': 0.3})

    # --- gun belt: cartridge loops, buckle, a cross-draw holster, Pa's lantern
    z0, z1 = sk.hip_z - 0.02, sk.hip_z + 0.03
    belt_r = (0.178, 0.124)
    hero.add('Belt', lh.band(v3(0, 0.002, 0), belt_r, z0, z1, 48, 2.4), 'leather', rigid='pelvis')
    for k in range(18):
        a = math.pi / 2 + math.radians(48 + k * (264 / 17))
        p = v3(math.cos(a) * (belt_r[0] + 0.006), -math.sin(a) * (belt_r[1] + 0.006) + 0.002, (z0 + z1) / 2)
        hero.add(f'Cartridge_{k}', lh.limb(p - v3(0, 0, 0.018), p + v3(0, 0, 0.02), 0.0065, 0.0055, 2, 8), 'brass',
                 rigid='pelvis')
    hero.add('Buckle', lh.prism(lh.rounded_rect(0.064, 0.05, 0.009), 0.012, v3(0.0, -0.128, (z0 + z1) / 2), v3(1, 0, 0),
                                v3(0, 0, 1), 0.2), 'brass', rigid='pelvis')
    top = v3(0.095, -0.118, z0 + 0.005)
    down = lh.unit(v3(-0.25, -0.12, -1.0))
    hero.add('Holster', lh.limb(top, top + down * 0.18, 0.032, 0.024, 4, 12, hint=(1, 0, 0), flat=0.55, power=2.6),
             'leather', rigid='pelvis')
    hero.add('RevolverGrip', lh.limb(top + v3(0, -0.004, 0.012), top + v3(-0.05, -0.02, 0.06), 0.017, 0.015, 3, 10, flat=0.7),
             'grip', rigid='pelvis')
    hero.add('RevolverHammer', lh.box(top + v3(-0.004, -0.008, 0.022), (0.022, 0.026, 0.032), bevel=0.2), 'metal',
             rigid='pelvis')
    lan = v3(-0.15, -0.085, z0 - 0.105)
    hero.add('LanternFrame', lh.lathe([(0.0, 0.0), (0.04, 0.0), (0.042, 0.014), (0.034, 0.024), (0.034, 0.085),
                                       (0.042, 0.095), (0.036, 0.11), (0.016, 0.128), (0.0, 0.133)], lan, n=12),
             'brass', rigid='pelvis')
    hero.add('LanternGlass', lh.lathe([(0.0, 0.0), (0.031, 0.0), (0.037, 0.03), (0.031, 0.06), (0.0, 0.06)],
                                      lan + v3(0, 0, 0.024), n=12), 'glass', rigid='pelvis')
    hero.add('LanternRing', lh.torus(lan + v3(0, 0, 0.15), (0, 1, 0), 0.026, 0.0045, 16, 6), 'brass', rigid='pelvis')

    # --- the head: pale skin, hair at the nape, stern brows over burning eyes, the bandana and the hat
    _, lft, fwd, up = lh.head_frame(sk)
    hero.add('Neck', lh.neck(sk, 0.055, 0.052), 'skin', bones=lh.NECK_BONES)
    hero.add('Head', lh.skull(sk, HEAD), 'skin', rigid='head')
    hair = lh.skull(sk, dict(HEAD, rx=HEAD['rx'] + 0.006, ry=HEAD['ry'] + 0.006, rz=HEAD['rz'] - 0.004, jaw=0.95, chin=0.0))
    hair = lh.cut(hair, lambda v: (lh.head_local(sk, v)[:, 1] < 0.02) & (lh.head_local(sk, v)[:, 2] > -0.07))
    hero.add('Hair', hair, 'hair', rigid='head')
    for sx in (1.0, -1.0):
        eye = lh.on_head(sk, sx * 0.035, HEAD['ry'] * 0.9, 0.022)
        hero.add(f'Eye_{sx:+.0f}', lh.ellipsoid(eye, (0.019, 0.007, 0.0085), 12, 8, axes=(lft, fwd, up)), 'eyes', rigid='head')
        a, c = lh.on_head(sk, sx * 0.014, HEAD['ry'] * 0.93, 0.042), lh.on_head(sk, sx * 0.055, HEAD['ry'] * 0.8, 0.052)
        hero.add(f'Brow_{sx:+.0f}', lh.limb(a, c, 0.0075, 0.005, 3, 8, hint=up, flat=0.6), 'brow', rigid='head')
        ear = lh.on_head(sk, sx * 0.089, -0.006, 0.0)
        hero.add(f'Ear_{sx:+.0f}', lh.ellipsoid(ear, (0.012, 0.022, 0.031), 10, 8, axes=(lft, fwd, up)), 'skin', rigid='head')
    kerchief = lh.bandana(sk, HEAD, top=0.008, chin=-0.104, drop=0.085, spread=1.45, offset=(0.006, 0.013), nose=0.022)
    hero.add('Bandana', lh.thicken(kerchief, 0.005, -1.0), 'bandana',
             weights=lambda v: _bandana_weights(hero, v))
    # The cloth's ends wrap round the back of the head to the knot.
    back = lh.face_wrap(sk, HEAD, [0.004, -0.03, -0.062], [0.008, 0.01, 0.012], gap=0.0, n=40)
    back = lh.cut(back, lambda v: lh.head_local(sk, v)[:, 1] < 0.03)
    hero.add('BandanaBack', lh.thicken(back, 0.004, -1.0), 'bandana', rigid='head')
    hero.add('BandanaKnot', lh.ellipsoid(lh.on_head(sk, 0.0, -HEAD['ry'] - 0.01, -0.05), (0.022, 0.016, 0.018), 10, 8,
                                         axes=(lft, fwd, up)), 'bandana', rigid='head')
    hat(hero, sk)
    return hero


def _bandana_weights(hero, verts):
    """The kerchief follows the head; its hanging point eases toward the neck as it falls below the chin."""
    local = lh.head_local(hero.sk, verts)
    k = np.clip((-local[:, 2] - 0.1) / 0.1, 0.0, 1.0) * 0.6
    return {'head': 1.0 - k, 'neck_02': k * 0.6, 'neck_01': k * 0.4}


def hat(hero, sk, s=1.08):
    """A cattleman: a wide brim curled up at the sides and dipped front and back, a creased crown, a dark band."""
    _, lft, fwd, up = lh.head_frame(sk)
    base = lh.on_head(sk, 0.0, 0.006, 0.062 * s)
    tilt = lh.unit(up + fwd * 0.14)
    r_ax, f_ax = lft, lh.unit(np.cross(tilt, lft))
    n = 48
    u = np.linspace(0.0, 2.0 * math.pi, n, endpoint=False)
    rows = []
    for r in (0.09, 0.125, 0.16, 0.195):
        r *= s
        k = (r - 0.09 * s) / (0.105 * s)
        curl = 0.055 * s * (np.abs(np.cos(u)) ** 2.2) * k ** 1.6
        dip = -0.014 * s * (np.abs(np.sin(u)) ** 2) * k
        pts = base[None, :] + (np.cos(u) * r)[:, None] * r_ax[None, :] + (np.sin(u) * r * 1.07)[:, None] * f_ax[None, :] + \
            (curl + dip)[:, None] * tilt[None, :]
        rows.append(pts)
    hero.add('HatBrim', lh.thicken(lh.loft(rows, cap_start=False, cap_end=False), 0.01), 'hat', rigid='head')
    prof = [(0.092 * s, -0.004), (0.094 * s, 0.032 * s), (0.091 * s, 0.074 * s), (0.083 * s, 0.104 * s),
            (0.062 * s, 0.126 * s), (0.03 * s, 0.135 * s), (0.0, 0.137 * s)]
    crown = lh.lathe(prof, base, axis=tilt, n=n, hint=lft, squash=1.12)

    def crease(v):
        rel = v - base[None, :]
        h, x, y = rel @ tilt, rel @ r_ax, rel @ f_ax
        top = np.clip((h - 0.085 * s) / (0.05 * s), 0.0, 1.0)
        dent = 0.036 * s * top * np.exp(-(x / (0.034 * s)) ** 2)
        pinch = 0.02 * s * top * np.exp(-((y - 0.08 * s) / (0.032 * s)) ** 2) * np.clip(np.abs(x) / (0.05 * s), 0, 1)
        return v - tilt[None, :] * dent[:, None] - r_ax[None, :] * (np.sign(x) * pinch)[:, None]

    hero.add('HatCrown', crown.moved(crease), 'hat', rigid='head')
    hero.add('HatBand', lh.lathe([(0.0955 * s, 0.0), (0.0955 * s, 0.028 * s)], base, axis=tilt, n=n, hint=lft, squash=1.12,
                                 cap_start=False, cap_end=False), 'hatband', rigid='head')


hero = build()
hero.finish()
