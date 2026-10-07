"""Gauge, the clockwork rail-hand: one of the five playable heroes concepted on 2026-10-04. A steam automaton the
railroad built to lay track through the Reaches, woken when a sliver of a saint's ember was sealed in its firebox.
Kept in the backlog (Art/Backlog/README.md): nothing in the game uses it yet.

The look: massive and top-heavy, iron plates and brass bands, a boiler chest with a pressure gauge for a heart, a
firebox door glowing in its belly, two smokestacks behind the shoulders, domed pauldrons, a small round head with one
headlamp eye, a grille mouth and a steam whistle, piston-rod limbs and huge three-fingered riveted hands.

Ember powers (a design sketch, not in the game): Slag Bomb (the approved bomb, lobbed from its firebox), Iron Hide (the
plates lock: 60% less damage for 6 s, slower, biters burn) and Rail Spike (a spike driven from the forearm through every
creature in a line).

The rig is the UE5 mannequin's skeleton (looter_heroes); 1.92 m tall. The parts are rigid where iron would be.
"""
import math

import numpy as np

import looter_heroes as lh
from looter_heroes import v3

PALETTE = {
    'iron': (0x5d6a78, 'fill'), 'ironDark': (0x3b434d, 'fill'), 'brass': (0xc9a050, 'accent'), 'copper': (0xb06a45, 'fill'),
    'paint': (0xa8433a, 'fill'), 'fire': (0xff8a1c, 'glow'), 'lamp': (0xfff0b8, 'glow'), 'dial': (0xefe6d0, 'fill'),
    'needle': (0xc8322c, 'accent'), 'soot': (0x2a2d33, 'fill'), 'rubber': (0x34302e, 'fill'),
}


def build():
    b = lh.Build(height=1.92, shoulders=0.56, hips=0.27, head=0.82, neck=0.32, arm=1.04, leg=0.95, torso=1.06, hand=1.4,
                 foot=1.3, arm_angle=45.0)
    hero = lh.Hero('Gauge', b, PALETTE)
    sk = hero.sk
    T = sk.T
    zt = lambda t: sk.hip_z + t * T

    # --- the boiler: a barrel chest over a riveted girdle, a domed top between the shoulders
    shell = [(-0.12, 0.17, 0.13, 0.0), (0.0, 0.2, 0.15, 0.0), (0.14, 0.19, 0.145, 0.0), (0.26, 0.18, 0.14, 0.0),
             (0.36, 0.22, 0.17, 0.01), (0.5, 0.26, 0.2, 0.025), (0.66, 0.275, 0.21, 0.03), (0.8, 0.27, 0.2, 0.025),
             (0.9, 0.24, 0.175, 0.015), (0.97, 0.18, 0.13, 0.01), (1.02, 0.1, 0.08, 0.0)]
    hero.add('Boiler', lh.torso(sk, shell, n=36, power=2.2), 'iron', bones=lh.TORSO_BONES)
    for t, (w, d) in ((0.33, (0.215, 0.165)), (0.62, (0.28, 0.215)), (0.02, (0.205, 0.155))):
        z = zt(t)
        hero.add(f'Band_{t}', lh.band(v3(0, lh.spine_at(sk, z)[1] - 0.012, 0), (w + 0.006, d + 0.006), z - 0.014, z + 0.014,
                                      40, 2.2), 'brass', bones=lh.TORSO_BONES)
    # Rivets round the bands.
    for t, (w, d) in ((0.33, (0.215, 0.165)), (0.62, (0.28, 0.215))):
        z = zt(t)
        for k in range(16):
            a = k / 16 * 2 * math.pi
            p = v3(math.cos(a) * (w + 0.012), lh.spine_at(sk, z)[1] - 0.012 - math.sin(a) * (d + 0.012), z)
            hero.add(f'Rivet_{t}_{k}', lh.ellipsoid(p, (0.007, 0.007, 0.007), 8, 5), 'brass', bones=lh.TORSO_BONES)
    # The red chest plate and the pressure gauge over the heart.
    chest_z = zt(0.74)
    front_y = lh.spine_at(sk, chest_z)[1] - 0.231
    plate = lh.prism(lh.rounded_rect(0.3, 0.19, 0.04), 0.014, v3(0.0, front_y - 0.004, chest_z), v3(1, 0, 0), v3(0, 0, 1), 0.08)
    # Bent round the boiler: the plate's sides follow the barrel back.
    plate = plate.moved(lambda v: v + v3(0, 1, 0) * (0.204 - np.sqrt(np.maximum(0.204 ** 2 - (v[:, 0:1] * 0.204 / 0.275) ** 2, 0.0))))
    hero.add('ChestPlate', plate, 'paint', rigid='spine_04')
    gauge_c = v3(0.06, front_y - 0.011 + (0.204 - math.sqrt(0.204 ** 2 - (0.06 * 0.204 / 0.275) ** 2)), chest_z + 0.01)
    hero.add('GaugeRim', lh.lathe([(0.0, 0.0), (0.06, 0.0), (0.064, 0.01), (0.058, 0.022), (0.05, 0.022)], gauge_c,
                                  axis=(0, -1, 0), n=28, cap_end=False), 'brass', rigid='spine_04')
    hero.add('GaugeFace', lh.lathe([(0.0, 0.0), (0.05, 0.0), (0.05, 0.016), (0.0, 0.016)], gauge_c, axis=(0, -1, 0), n=28),
             'dial', rigid='spine_04')
    needle = lh.prism([(-0.003, -0.004), (0.003, -0.004), (0.0015, 0.04), (-0.0015, 0.04)], 0.004,
                      gauge_c + v3(0, -0.019, 0), lh.unit(v3(math.cos(0.6), 0, math.sin(0.6))), lh.unit(v3(-math.sin(0.6), 0, math.cos(0.6))))
    hero.add('GaugeNeedle', needle, 'needle', rigid='spine_04')
    for k in range(8):
        a = -2.3 + k * (4.6 / 7)
        p = gauge_c + v3(math.sin(a) * 0.038, -0.018, math.cos(a) * 0.038)
        hero.add(f'GaugeTick_{k}', lh.box(p, (0.004, 0.003, 0.009), axes=(lh.unit(v3(math.cos(a), 0, -math.sin(a))), v3(0, -1, 0), None)),
                 'soot', rigid='spine_04')
    # The firebox door in the belly: a ring, a grate of bars, the fire behind it.
    belly_z = zt(0.42)
    belly_y = lh.spine_at(sk, belly_z)[1] - 0.012 - 0.18
    door_c = v3(0.0, belly_y - 0.004, belly_z)
    hero.add('Fire', lh.ellipsoid(door_c + v3(0, 0.012, 0), (0.085, 0.02, 0.07), 20, 10), 'fire', rigid='spine_02')
    hero.add('DoorRing', lh.torus(door_c - v3(0, 0.004, 0), (0, -1, 0), 0.085, 0.014, 36, 10, squash=0.82, hint=(1, 0, 0)),
             'ironDark', rigid='spine_02')
    for k in range(5):
        x = -0.056 + k * 0.028
        hgt = 0.12 * math.sqrt(max(0.0, 1 - (x / 0.085) ** 2))
        hero.add(f'Grate_{k}', lh.box(door_c + v3(x, -0.008, 0.0), (0.009, 0.012, hgt), bevel=0.2), 'soot', rigid='spine_02')
    hero.add('DoorLatch', lh.box(door_c + v3(0.095, -0.012, 0.0), (0.03, 0.016, 0.022), bevel=0.2), 'brass', rigid='spine_02')
    # Two smokestacks behind the shoulders, sooty at the lip.
    for sx in (1.0, -1.0):
        base = v3(sx * 0.11, lh.spine_at(sk, zt(0.8))[1] + 0.17, zt(0.8))
        top = base + v3(sx * 0.02, 0.03, 0.36)
        stack = lh.lathe([(0.0, 0.0), (0.045, 0.0), (0.042, 0.25), (0.06, 0.32), (0.064, 0.36), (0.05, 0.36), (0.04, 0.34),
                          (0.0, 0.34)], base, axis=lh.unit(top - base), n=18)
        hero.add(f'Stack_{sx:+.0f}', stack, 'ironDark', rigid='spine_05')
        hero.add(f'StackBand_{sx:+.0f}', lh.lathe([(0.048, 0.06), (0.048, 0.085)], base, axis=lh.unit(top - base), n=18,
                                                  cap_start=False, cap_end=False), 'copper', rigid='spine_05')
        hero.add(f'StackSoot_{sx:+.0f}', lh.lathe([(0.052, 0.335), (0.04, 0.338)], base, axis=lh.unit(top - base), n=18,
                                                  cap_start=False, cap_end=False), 'soot', rigid='spine_05')
        hero.socket(f'Stack_{"l" if sx > 0 else "r"}', 'spine_05', base + lh.unit(top - base) * 0.37)

    # --- the hips: a short skirt of riveted plates over the pelvis
    for k in range(10):
        a = k / 10 * 2 * math.pi + 0.31
        out = v3(math.cos(a), -math.sin(a) * 0.78, 0.0)
        c = v3(math.cos(a) * 0.205, -math.sin(a) * 0.16, zt(-0.06))
        down = lh.unit(v3(0, 0, -1) + out * 0.25)
        plate = lh.box(c, (0.12, 0.17, 0.016), axes=(lh.unit(np.cross(down, out)), down, None), bevel=0.15)
        hero.add(f'Tasset_{k}', plate, 'ironDark', skirt={'stiff': 0.6, 'top': zt(0.0), 'width': 0.36})

    # --- arms: domed pauldrons, piston-rod limbs, ball joints, huge hands
    for side, sx in lh.SIDES:
        sh, el, wr = sk.head(f'upperarm_{side}'), sk.tail(f'upperarm_{side}'), sk.tail(f'lowerarm_{side}')
        d1 = lh.unit(el - sh)
        pauld = lh.lathe([(0.0, 0.0), (0.13, 0.0), (0.135, 0.02), (0.115, 0.07), (0.07, 0.11), (0.0, 0.125)],
                         sh + v3(sx * 0.0, 0.0, -0.035), axis=lh.unit(v3(sx * 0.45, 0, 1)), n=28, squash=0.92)
        hero.add(f'Pauldron_{side}', pauld, 'iron', bones={f'clavicle_{side}': 1.0, f'upperarm_{side}': 0.8, 'spine_05': 0.3})
        hero.add(f'PauldronRim_{side}', lh.torus(sh + v3(0, 0, -0.035) + lh.unit(v3(sx * 0.45, 0, 1)) * 0.012,
                                                 lh.unit(v3(sx * 0.45, 0, 1)), 0.132, 0.011, 32, 8, squash=0.92),
                 'brass', bones={f'clavicle_{side}': 1.0, f'upperarm_{side}': 0.8, 'spine_05': 0.3})
        hero.add(f'UpperArm_{side}', lh.limb(sh + d1 * 0.05, el - d1 * 0.03, 0.068, 0.06, 4, 18, hint=(0, -1, 0)), 'ironDark',
                 rigid=f'upperarm_{side}')
        hero.add(f'Elbow_{side}', lh.ellipsoid(el, (0.068, 0.068, 0.068), 16, 10), 'brass',
                 bones={f'upperarm_{side}': 0.6, f'lowerarm_{side}': 1.0})
        d2 = lh.unit(wr - el)
        fore = lh.limb(el + d2 * 0.04, wr - d2 * 0.005, 0.07, 0.078, 5, 18, hint=(0, -1, 0), bulge=0.1, at=0.55, power=2.3)
        hero.add(f'Forearm_{side}', fore, 'iron', rigid=f'lowerarm_{side}')
        hero.add(f'ForearmBand_{side}', lh.limb(wr - d2 * 0.07, wr - d2 * 0.035, 0.084, 0.084, 1, 18), 'brass',
                 rigid=f'lowerarm_{side}')
        piston_a = el + d2 * 0.06 + v3(0, 0.07, 0)
        hero.add(f'Piston_{side}', lh.limb(piston_a, piston_a + d2 * 0.17, 0.014, 0.014, 2, 8), 'copper', rigid=f'lowerarm_{side}')
        hero.add(f'PistonRod_{side}', lh.limb(piston_a - d1 * 0.12, piston_a + d2 * 0.02, 0.007, 0.007, 2, 6), 'brass',
                 rigid=f'upperarm_{side}')
        # The hand: a riveted mitt with three thick fingers and a thumb.
        p = sk.palm[side]
        a_, n_, s_, q = p['a'], p['n'], p['s'], p['q']
        palm = lh.box(p['wrist'] + a_ * 0.055 * q, (0.11 * q, 0.105 * q, 0.05 * q), axes=(a_, s_, n_), bevel=0.25)
        hero.add(f'Palm_{side}', palm, 'iron', bones={f'hand_{side}': 1.0, f'middle_metacarpal_{side}': 0.3})
        for finger in ('index', 'middle', 'ring'):
            names = [f'{finger}_0{i}_{side}' for i in (1, 2, 3)]
            pts = [sk.head(names[0]) - a_ * 0.01 * q] + [sk.tail(nm) for nm in names]
            if finger == 'ring':
                pts = [x - s_ * 0.008 * q for x in pts]
            hero.add(f'Finger_{side}_{finger}', lh.tube(pts, [0.017 * q, 0.016 * q, 0.015 * q, 0.013 * q], 10, hint=s_, power=2.4),
                     'ironDark', bones={nm: 1.0 for nm in names} | {f'{finger}_metacarpal_{side}': 0.5})
            for nm in names[:2]:
                hero.add(f'Knuckle_{nm}', lh.ellipsoid(sk.tail(nm), (0.016 * q,) * 3, 10, 6), 'brass', rigid=nm)
        names = [f'thumb_0{i}_{side}' for i in (1, 2, 3)]
        pts = [sk.head(names[0])] + [sk.tail(nm) for nm in names]
        hero.add(f'Thumb_{side}', lh.tube(pts, [0.02 * q, 0.017 * q, 0.016 * q, 0.014 * q], 10, hint=n_),
                 'ironDark', bones={nm: 1.0 for nm in names} | {f'hand_{side}': 0.5})

    # --- legs: piston thighs, knee balls, shins with rods, broad feet with toe plates
    for side, sx in lh.SIDES:
        hip, knee, ank = sk.head(f'thigh_{side}'), sk.tail(f'thigh_{side}'), sk.tail(f'calf_{side}')
        d1, d2 = lh.unit(knee - hip), lh.unit(ank - knee)
        hero.add(f'Hip_{side}', lh.ellipsoid(hip, (0.095, 0.095, 0.095), 16, 10), 'ironDark', bones={'pelvis': 0.7, f'thigh_{side}': 1.0})
        hero.add(f'Thigh_{side}', lh.limb(hip + d1 * 0.04, knee - d1 * 0.04, 0.098, 0.082, 5, 20, hint=(0, -1, 0), bulge=0.06),
                 'iron', rigid=f'thigh_{side}')
        hero.add(f'Knee_{side}', lh.ellipsoid(knee + v3(0, -0.02, 0), (0.08, 0.08, 0.08), 16, 10), 'brass',
                 bones={f'thigh_{side}': 0.5, f'calf_{side}': 1.0})
        hero.add(f'KneeCap_{side}', lh.lathe([(0.0, 0.0), (0.06, 0.0), (0.05, 0.03), (0.0, 0.04)], knee + v3(0, -0.07, 0.0),
                                             axis=(0, -1, 0), n=16), 'iron', rigid=f'calf_{side}')
        hero.add(f'Shin_{side}', lh.limb(knee + d2 * 0.06, ank + d2 * 0.02, 0.078, 0.07, 5, 20, hint=(0, -1, 0), bulge=0.12, at=0.3,
                                         power=2.3), 'ironDark', rigid=f'calf_{side}')
        rod_a = knee + d2 * 0.1 + v3(0, 0.075, 0)
        hero.add(f'ShinPiston_{side}', lh.limb(rod_a, rod_a + d2 * 0.22, 0.016, 0.016, 2, 8), 'copper', rigid=f'calf_{side}')
        shaft, foot = lh.boot(sk, side, shaft_top=0.25, shaft_r=0.08, cuff=0.01, width=0.075, heel=0.05, sole=0.02)
        hero.add(f'Ankle_{side}', shaft, 'iron', bones=lh.FOOT_BONES(side))
        hero.add(f'Foot_{side}', foot, 'ironDark', bones=lh.FOOT_BONES(side))
        toe = sk.tail(f'ball_{side}')
        hero.add(f'ToeCap_{side}', lh.ellipsoid(toe + v3(0, 0.035, 0.035), (0.08, 0.06, 0.04), 16, 8), 'brass',
                 bones={f'ball_{side}': 1.0, f'foot_{side}': 0.4})

    # --- the head: a small iron dome, one headlamp eye, a grille mouth, a steam whistle on top
    _, lft, fwd, up = lh.head_frame(sk)
    c = sk.head_center
    hero.add('Neck', lh.neck(sk, 0.06, 0.055), 'ironDark', bones=lh.NECK_BONES)
    hero.add('NeckRing', lh.torus(sk.head('neck_01') + v3(0, 0, 0.01), (0, 0, 1), 0.078, 0.016, 28, 8), 'brass', rigid='spine_05')
    hero.add('HeadDome', lh.ellipsoid(c + up * 0.01, (0.1, 0.1, 0.095), 24, 14, axes=(lft, fwd, up), power=2.2), 'iron', rigid='head')
    hero.add('HeadBrim', lh.torus(c - up * 0.005, up, 0.102, 0.012, 32, 8), 'brass', rigid='head')
    hero.add('LampHood', lh.lathe([(0.0, 0.0), (0.04, 0.0), (0.043, 0.03), (0.037, 0.036), (0.0, 0.036)], c + fwd * 0.074 + up * 0.016,
                                  axis=fwd, n=20), 'ironDark', rigid='head')
    hero.add('Lamp', lh.lathe([(0.0, 0.0), (0.031, 0.0), (0.033, 0.005), (0.0, 0.01)], c + fwd * 0.11 + up * 0.016, axis=fwd, n=20),
             'lamp', rigid='head')
    hero.add('LampRim', lh.torus(c + fwd * 0.111 + up * 0.016, fwd, 0.036, 0.0055, 24, 6), 'brass', rigid='head')
    for k in range(4):
        z = -0.04 - k * 0.011
        hero.add(f'Grille_{k}', lh.box(c + fwd * 0.093 + up * z, (0.07 - k * 0.008, 0.012, 0.006), axes=(lft, fwd, None)),
                 'soot', rigid='head')
    hero.add('Jaw', lh.box(c + fwd * 0.07 - up * 0.058, (0.14, 0.06, 0.05), axes=(lft, fwd, None), bevel=0.25), 'ironDark',
             rigid='head')
    hero.add('Whistle', lh.lathe([(0.0, 0.0), (0.016, 0.0), (0.016, 0.04), (0.026, 0.05), (0.026, 0.07), (0.012, 0.08), (0.0, 0.08)],
                                 c + up * 0.1, axis=up, n=16), 'brass', rigid='head')
    for sx in (1.0, -1.0):
        hero.add(f'EarBolt_{sx:+.0f}', lh.lathe([(0.0, 0.0), (0.032, 0.0), (0.032, 0.018), (0.02, 0.026), (0.0, 0.026)],
                                                c + lft * sx * 0.094, axis=lft * sx, n=16), 'copper', rigid='head')
    return hero


hero = build()
hero.finish()
