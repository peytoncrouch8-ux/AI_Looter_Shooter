"""Odessa Lark, the Cardsharp: one of the five playable heroes concepted on 2026-10-04, a riverboat gambler who plays
the Reaches' saloons and never loses a hand she means to win. Kept in the backlog (Art/Backlog/README.md): nothing in
the game uses it yet.

The look: a flat-crowned gambler's hat with a card in its band, a wine-red tailcoat cut away at the waist with long
tails, a gold brocade waistcoat with a watch chain, a white high collar and black cravat, black gloves, trousers tucked
into tall heeled riding boots, a card case on the hip, gold earrings.

Ember powers (a design sketch, not in the game): Lucky Streak (the approved ricochets and critical hits), Fifty-Two
Pickup (a cone of razor cards that each cut) and Double or Nothing (for 6 s every shot deals double damage or misses).

The rig is the UE5 mannequin's skeleton (looter_heroes); 1.72 m tall.
"""
import math

import numpy as np

import looter_heroes as lh
from looter_heroes import v3

PALETTE = {
    'coat': (0x7a2f3e, 'fill'), 'coatDark': (0x4e1f2a, 'fill'), 'vest': (0xc9a24e, 'fill'), 'vestDark': (0x8a6a2e, 'fill'),
    'blouse': (0xf1e9dc, 'fill'), 'cravat': (0x2a2428, 'fill'), 'pants': (0x2f2c34, 'fill'), 'boots': (0x221d22, 'fill'),
    'gloves': (0x2a2529, 'fill'), 'skin': (0xb07a5c, 'fill'), 'hair': (0x2a1f22, 'fill'), 'eyes': (0x1e1618, 'fill'),
    'lips': (0x8f2f3a, 'fill'), 'hat': (0x2b2629, 'fill'), 'hatband': (0x7a2f3e, 'fill'), 'gold': (0xe0b44a, 'accent'),
    'card': (0xf7f1e4, 'fill'), 'pip': (0xd23d3d, 'accent'), 'leather': (0x4a3328, 'fill'),
}
HEAD = dict(rx=0.079, ry=0.096, rz=0.112, jaw=0.74, chin=0.006, back=1.02)


def build():
    b = lh.Build(height=1.72, shoulders=0.36, hips=0.22, head=1.12, neck=0.9, hand=1.0, foot=0.95, leg=1.05,
                 arm_angle=45.0)
    hero = lh.Hero('Odessa', b, PALETTE)
    sk = hero.sk
    T = sk.T
    zt = lambda t: sk.hip_z + t * T

    # --- the blouse (the torso), trousers, riding boots, gloves
    body = [(-0.16, 0.14, 0.1, 0.0), (-0.06, 0.166, 0.114, 0.0), (0.05, 0.17, 0.112, 0.0), (0.2, 0.148, 0.1, 0.0),
            (0.36, 0.128, 0.09, 0.0), (0.52, 0.14, 0.1, 0.006), (0.64, 0.156, 0.118, 0.018), (0.76, 0.162, 0.112, 0.012),
            (0.86, 0.164, 0.1, 0.004), (0.94, 0.148, 0.09, 0.0), (0.985, 0.098, 0.072, 0.0), (1.0, 0.06, 0.056, 0.0)]
    hero.add('Blouse', lh.torso(sk, body), 'blouse', bones=lh.TORSO_BONES)
    for side, sx in lh.SIDES:
        leg = lambda t: float(np.interp(t, [-0.12, 0.0, 0.25, 0.5, 0.7, 0.95], [0.096, 0.094, 0.078, 0.058, 0.056, 0.048]))
        hero.add(f'Trousers_{side}', lh.trouser(sk, side, leg, -0.12, 0.95), 'pants', bones=lh.LEG_BONES(side))
        shaft, foot = lh.boot(sk, side, shaft_top=0.9, shaft_r=0.06, cuff=0.004, width=0.046, heel=0.03)
        hero.add(f'BootShaft_{side}', shaft, 'boots', bones=lh.FOOT_BONES(side) | {f'thigh_{side}': 0.5})
        hero.add(f'Boot_{side}', foot, 'boots', bones=lh.FOOT_BONES(side))
        ank = sk.tail(f'calf_{side}')
        heel = lh.box(v3(ank[0], ank[1] + 0.018, 0.025), (0.04, 0.045, 0.05), bevel=0.15)
        hero.add(f'Heel_{side}', heel, 'boots', bones=lh.FOOT_BONES(side))
        for i, (shape, how) in enumerate(lh.hand_shapes(sk, side, glove=1.0, finger_r=0.0088, thumb_r=0.0105)):
            hero.add(f'Glove_{side}_{i}', shape, 'gloves', bones=how[1], power=how[2])
        hero.add(f'GloveCuff_{side}', lh.sleeve(sk, side, lambda t: 0.04 + 0.01 * (t - 0.88) / 0.14, start=0.88, end=1.02),
                 'gloves', bones={f'lowerarm_{side}': 1.0, f'hand_{side}': 1.0})

    # --- the waistcoat over the blouse: a V at the neck, buttoned to a point below the waist
    vest_rows = []
    for t, gap in ((0.9, 0.62), (0.8, 0.46), (0.66, 0.3), (0.52, 0.12), (0.36, 0.05), (0.2, 0.05), (0.08, 0.05)):
        w, d, f = (np.interp(t, [r[0] for r in body], [r[i] for r in body]) for i in (1, 2, 3))
        vest_rows.append(dict(z=zt(t), rx=w + 0.008, ry=d + 0.008, fwd=f, gap=gap))
    vest, vest_edges = lh.garment(sk, vest_rows, n=40, thickness=0.006)
    hero.add('Vest', vest, 'vest', bones=lh.TORSO_BONES)
    for k, t in enumerate((0.48, 0.38, 0.28, 0.18)):
        z = zt(t)
        y = lh.spine_at(sk, z)[1] - np.interp(t, [r[0] for r in body], [r[2] for r in body]) - 0.016
        hero.add(f'Button_{k}', lh.ellipsoid(v3(0.0, y, z), (0.008, 0.005, 0.008), 10, 6), 'gold', rigid='spine_02')
    # A watch chain from a button to the waistcoat pocket.
    chain = [v3(0.0, lh.spine_at(sk, zt(0.3))[1] - 0.112, zt(0.3))]
    for i in range(1, 7):
        u = i / 6
        x = 0.09 * u
        z = zt(0.3) - 0.04 * math.sin(math.pi * u) - 0.02 * u
        y = lh.spine_at(sk, z)[1] - 0.112 + 0.025 * u ** 2
        chain.append(v3(x, y, z))
    hero.add('WatchChain', lh.tube(chain, 0.0028, 6), 'gold', rigid='spine_02')
    hero.add('Watch', lh.ellipsoid(chain[-1] + v3(0.008, 0.006, -0.012), (0.014, 0.006, 0.016), 12, 8), 'gold', rigid='spine_02')

    # --- the tailcoat: open from the collar, cut away at the waist, two long tails behind
    waist = zt(0.18)
    coat_rows = []
    for t, gap in ((0.985, 1.05), (0.94, 0.86), (0.86, 0.74), (0.76, 0.66), (0.64, 0.62), (0.52, 0.64), (0.36, 0.78),
                   (0.2, 1.1)):
        w, d, f = (np.interp(t, [r[0] for r in body], [r[i] for r in body]) for i in (1, 2, 3))
        coat_rows.append(dict(z=zt(t), rx=w + 0.02, ry=d + 0.02, fwd=f, gap=gap))
    coat, edges = lh.garment(sk, coat_rows, n=44, thickness=0.007)
    hero.add('Coat', coat, 'coat', bones=lh.TORSO_BONES)
    for sgn, side in ((1, 'l'), (-1, 'r')):
        hero.add(f'Lapel_{side}', lh.lapel_strip(edges, list(range(0, 6)), [0.014, 0.034, 0.048, 0.05, 0.036, 0.016], sgn),
                 'coatDark', bones=lh.TORSO_BONES)
    tail_top = zt(0.24)
    tail_bottom = sk.knee_z - 0.04
    for side, sgn in (('l', 1), ('r', -1)):
        rows = []
        for i in range(8):
            u = i / 7
            z = tail_top + (tail_bottom - tail_top) * u
            rx = 0.182 + 0.03 * u
            ry = 0.125 + 0.04 * u
            back = 3 * math.pi / 2
            inner, outer = 0.035, 1.05 - 0.55 * u ** 0.8      # the tails narrow as they fall
            u0, u1 = (back + inner, back + outer) if sgn > 0 else (back - outer, back - inner)
            rows.append(dict(z=z, rx=rx, ry=ry, fwd=-0.004, u0=u0, u1=u1))
        tail, _ = lh.garment(sk, rows, n=16, thickness=0.007)
        hero.add(f'Tail_{side}', tail, 'coat', skirt={'stiff': 0.7, 'top': tail_top, 'width': 0.28})
    for side, sx in lh.SIDES:
        sleeve_r = lambda t: float(np.interp(t, [-0.12, -0.05, 0.04, 0.2, 0.5, 0.75, 0.9], [0.046, 0.068, 0.074, 0.064, 0.054, 0.052, 0.05]))
        hero.add(f'Sleeve_{side}', lh.sleeve(sk, side, sleeve_r, -0.12, 0.9), 'coat', bones=lh.ARM_BONES(side))
        hero.add(f'Cuff_{side}', lh.sleeve(sk, side, lambda t: 0.057, start=0.8, end=0.91), 'coatDark',
                 bones={f'lowerarm_{side}': 1.0, f'hand_{side}': 0.3})
        hero.add(f'Ruffle_{side}', lh.sleeve(sk, side, lambda t: 0.05 + 0.012 * (t - 0.9) / 0.06, start=0.9, end=0.96),
                 'blouse', bones={f'lowerarm_{side}': 1.0, f'hand_{side}': 0.4})
    collar = []
    base = sk.head('neck_01')
    for dz, rx, ry in ((-0.02, 0.07, 0.064), (0.03, 0.074, 0.068), (0.06, 0.084, 0.078)):
        collar.append(lh.arc(base + v3(0, 0.006, dz), v3(1, 0, 0), v3(0, -1, 0), rx, ry, math.pi / 2 + 0.95,
                             math.pi / 2 + 2 * math.pi - 0.95, 28))
    hero.add('CoatCollar', lh.thicken(lh.sheet(collar), 0.006), 'coatDark',
             bones={'spine_05': 1.0, 'neck_01': 0.7, 'clavicle_l': 0.3, 'clavicle_r': 0.3})

    # --- belt and card case
    z0, z1 = sk.hip_z - 0.01, sk.hip_z + 0.02
    hero.add('Belt', lh.band(v3(0, 0.002, 0), (0.176, 0.118), z0, z1, 44, 2.4), 'leather', rigid='pelvis')
    hero.add('Buckle', lh.prism(lh.rounded_rect(0.04, 0.032, 0.006), 0.01, v3(0.0, -0.124, (z0 + z1) / 2), v3(1, 0, 0),
                                v3(0, 0, 1), 0.2), 'gold', rigid='pelvis')
    case_c = v3(-0.17, -0.06, z0 - 0.06)
    hero.add('CardCase', lh.box(case_c, (0.03, 0.075, 0.1), axes=(lh.unit(v3(1, 0.35, 0)), lh.unit(v3(-0.35, 1, 0)), None), bevel=0.2),
             'leather', rigid='pelvis')
    hero.add('CardCaseClasp', lh.box(case_c + v3(-0.016, -0.004, 0.03), (0.006, 0.03, 0.016), bevel=0.2), 'gold', rigid='pelvis')

    # --- the head: face, hair with a low bun, earrings, cravat and high collar, the hat with a card in its band
    _, lft, fwd, up = lh.head_frame(sk)
    hero.add('Neck', lh.neck(sk, 0.045, 0.043), 'skin', bones=lh.NECK_BONES)
    hero.add('Head', lh.skull(sk, HEAD), 'skin', rigid='head')
    hero.add('Nose', lh.ellipsoid(lh.on_head(sk, 0.0, HEAD['ry'] * 0.93, -0.014), (0.0085, 0.011, 0.016), 10, 8,
                                  axes=(lft, fwd, up)), 'skin', rigid='head')
    for sx in (1.0, -1.0):
        eye = lh.on_head(sk, sx * 0.031, HEAD['ry'] * 0.86, 0.014)
        eye_ax = (lh.unit(lft + up * 0.18 * sx), fwd, lh.unit(up - lft * 0.18 * sx))
        hero.add(f'Eye_{sx:+.0f}', lh.ellipsoid(eye, (0.019, 0.0065, 0.0085), 12, 8, axes=eye_ax), 'eyes', rigid='head')
        # A winged line along the upper lid.
        lid = [lh.on_head(sk, sx * 0.014, HEAD['ry'] * 0.92, 0.021), lh.on_head(sk, sx * 0.032, HEAD['ry'] * 0.9, 0.025),
               lh.on_head(sk, sx * 0.05, HEAD['ry'] * 0.78, 0.024), lh.on_head(sk, sx * 0.058, HEAD['ry'] * 0.7, 0.03)]
        hero.add(f'Lid_{sx:+.0f}', lh.tube(lid, [0.0026, 0.0032, 0.0028, 0.0016], 6), 'eyes', rigid='head')
        a, c, m = (lh.on_head(sk, sx * 0.016, HEAD['ry'] * 0.92, 0.036), lh.on_head(sk, sx * 0.05, HEAD['ry'] * 0.76, 0.036),
                   lh.on_head(sk, sx * 0.033, HEAD['ry'] * 0.9, 0.045))
        hero.add(f'Brow_{sx:+.0f}', lh.tube([a, m, c], [0.0035, 0.004, 0.003], 6), 'hair', rigid='head')
        ear = lh.on_head(sk, sx * 0.08, -0.008, 0.0)
        hero.add(f'Ear_{sx:+.0f}', lh.ellipsoid(ear, (0.01, 0.018, 0.026), 10, 8, axes=(lft, fwd, up)), 'skin', rigid='head')
        hero.add(f'Earring_{sx:+.0f}', lh.torus(ear - up * 0.034 + lft * sx * 0.004, fwd, 0.011, 0.0022, 14, 5), 'gold', rigid='head')
    hero.add('Lips', lh.ellipsoid(lh.on_head(sk, 0.0, HEAD['ry'] * 0.84, -0.052), (0.02, 0.008, 0.008), 12, 8,
                                  axes=(lft, fwd, up)), 'lips', rigid='head')
    hair = lh.skull(sk, dict(HEAD, rx=HEAD['rx'] + 0.008, ry=HEAD['ry'] + 0.008, rz=HEAD['rz'] + 0.002, jaw=0.9, chin=0.0))
    hair = lh.cut(hair, lambda v: (lh.head_local(sk, v)[:, 1] < 0.045 - 0.4 * np.clip(lh.head_local(sk, v)[:, 2], 0, 1)) &
                  (lh.head_local(sk, v)[:, 2] > -0.06))
    hero.add('Hair', hair, 'hair', rigid='head')
    hero.add('Bun', lh.ellipsoid(lh.on_head(sk, 0.0, -HEAD['ry'] - 0.012, -0.045), (0.044, 0.034, 0.036), 14, 10,
                                 axes=(lft, fwd, up)), 'hair', rigid='head')
    # High collar and a black cravat tied at the throat.
    hero.add('HighCollar', lh.band(v3(0, sk.head('neck_01')[1] - 0.002, 0), (0.054, 0.05), sk.neck_z + 0.0,
                                   sk.neck_z + 0.055, 24), 'blouse', bones={'neck_01': 1.0, 'neck_02': 0.6, 'spine_05': 0.5})
    knot = v3(0.0, sk.head('neck_01')[1] - 0.058, sk.neck_z + 0.03)
    hero.add('CravatKnot', lh.ellipsoid(knot, (0.016, 0.012, 0.014), 10, 8), 'cravat', bones={'neck_01': 1.0, 'spine_05': 0.5})
    for sx in (1.0, -1.0):
        hero.add(f'CravatEnd_{sx:+.0f}', lh.limb(knot + v3(0, -0.004, -0.006), knot + v3(sx * 0.02, -0.012, -0.075), 0.011, 0.016,
                                                  3, 8, hint=(0, -1, 0), flat=0.35), 'cravat', bones={'neck_01': 0.5, 'spine_05': 1.0})
    gambler_hat(hero, sk)
    return hero


def gambler_hat(hero, sk, s=1.04):
    """A flat-crowned gambler: a flat brim turned up a touch at the edge, a low straight crown, a red band with a
    playing card tucked in it."""
    _, lft, fwd, up = lh.head_frame(sk)
    base = lh.on_head(sk, 0.0, 0.002, 0.058 * s)
    tilt = lh.unit(up + fwd * 0.08 + lft * 0.06)
    r_ax = lh.unit(lft - tilt * float(lft @ tilt))
    f_ax = lh.unit(np.cross(tilt, r_ax))
    n = 44
    u = np.linspace(0.0, 2.0 * math.pi, n, endpoint=False)
    rows = []
    for r in (0.086, 0.12, 0.15, 0.162):
        r *= s
        lip = 0.012 * s * max(0.0, (r - 0.15 * s) / (0.012 * s))
        pts = base[None, :] + (np.cos(u) * r)[:, None] * r_ax[None, :] + (np.sin(u) * r * 1.06)[:, None] * f_ax[None, :] + \
            lip * tilt[None, :]
        rows.append(pts)
    hero.add('HatBrim', lh.thicken(lh.loft(rows, cap_start=False, cap_end=False), 0.009), 'hat', rigid='head')
    prof = [(0.088 * s, -0.004), (0.088 * s, 0.07 * s), (0.084 * s, 0.085 * s), (0.0, 0.088 * s)]
    hero.add('HatCrown', lh.lathe(prof, base, axis=tilt, n=n, hint=r_ax, squash=1.1, power=2.2), 'hat', rigid='head')
    hero.add('HatBand', lh.lathe([(0.0905 * s, 0.002), (0.0905 * s, 0.024 * s)], base, axis=tilt, n=n, hint=r_ax,
                                 squash=1.1, power=2.2, cap_start=False, cap_end=False), 'hatband', rigid='head')
    # The card, tucked upright in the band on the left side.
    side = lh.unit(r_ax * 0.8 + f_ax * 0.6)
    pos = base + side * 0.097 * s + tilt * 0.045 * s
    tangent = lh.unit(np.cross(tilt, side))
    card = lh.box(pos, (0.042, 0.06, 0.003), axes=(tangent, lh.unit(tilt * 0.95 + side * 0.15), None))
    hero.add('Card', card, 'card', rigid='head')
    pip = lh.prism([(0.0, 0.012), (0.009, 0.0), (0.0, -0.012), (-0.009, 0.0)], 0.0045, pos + side * 0.0005,
                   tangent, lh.unit(tilt * 0.95 + side * 0.15))
    hero.add('CardPip', pip, 'pip', rigid='head')


hero = build()
hero.finish()
