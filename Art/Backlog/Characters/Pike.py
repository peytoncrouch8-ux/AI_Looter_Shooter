"""Wendell Pike, the Surveyor: one of the five playable heroes concepted on 2026-10-04. The railroad's old surveyor,
who has walked every mile the line will ever run and reads country like a ledger. Kept in the backlog
(Art/Backlog/README.md): nothing in the game uses it yet.

The look: short and stocky, a brown bowler with brass goggles strapped round it, round spectacles, bushy grey
mutton chops joined to a big mustache, a teal neckerchief, a khaki field coat full of pockets under a cartridge belt,
olive trousers in wrapped puttees, a huge canvas pack with a folded tripod, a map tube and a bedroll, a scoped long
rifle strapped beside it, and a brass spyglass at his hip.

Ember powers (a design sketch, not in the game): Spyglass (the approved marking through walls), Tripwire (two stakes
and a wire that trips and stuns whatever crosses) and Long Shot (for 8 s damage grows with range; marked creatures are
always critically hit).

The rig is the UE5 mannequin's skeleton (looter_heroes); 1.68 m tall.
"""
import math

import numpy as np

import looter_heroes as lh
from looter_heroes import v3

PALETTE = {
    'coat': (0xb8a57a, 'fill'), 'coatDark': (0x8f7d58, 'fill'), 'shirt': (0xe5dccb, 'fill'), 'pants': (0x6f7350, 'fill'),
    'puttee': (0xa99f86, 'fill'), 'boots': (0x5a3d2b, 'fill'), 'leather': (0x6a4a33, 'fill'), 'skin': (0xd6a07f, 'fill'),
    'whiskers': (0xb8b2a6, 'fill'), 'hat': (0x5a4535, 'fill'), 'hatband': (0x3a2c24, 'fill'), 'brass': (0xd1a54e, 'accent'),
    'glass': (0x9fd6e0, 'fill'), 'kerchief': (0x2f8f93, 'accent'), 'canvas': (0x8f8463, 'fill'), 'wood': (0x9a7650, 'fill'),
    'gunmetal': (0x4d5258, 'fill'), 'eyes': (0x2a2224, 'fill'), 'bedroll': (0x7a4a3c, 'fill'),
}
HEAD = dict(rx=0.09, ry=0.1, rz=0.112, jaw=0.9, chin=0.006, back=1.0)


def build():
    b = lh.Build(height=1.68, shoulders=0.43, hips=0.24, head=1.14, neck=0.62, arm=0.98, leg=0.92, torso=1.06, hand=1.1,
                 foot=1.05, arm_angle=45.0)
    hero = lh.Hero('Pike', b, PALETTE)
    sk = hero.sk
    T = sk.T
    zt = lambda t: sk.hip_z + t * T

    # --- a stout body: shirt, trousers, puttees and ankle boots, bare hands
    body = [(-0.16, 0.15, 0.11, 0.0), (-0.06, 0.176, 0.126, 0.0), (0.05, 0.186, 0.136, 0.01), (0.2, 0.19, 0.148, 0.026),
            (0.36, 0.187, 0.15, 0.03), (0.52, 0.19, 0.14, 0.02), (0.66, 0.198, 0.132, 0.012), (0.79, 0.2, 0.126, 0.008),
            (0.87, 0.198, 0.118, 0.004), (0.94, 0.175, 0.1, 0.0), (0.985, 0.12, 0.08, 0.0), (1.0, 0.078, 0.068, 0.0)]
    hero.add('Shirt', lh.torso(sk, body), 'shirt', bones=lh.TORSO_BONES)
    for side, sx in lh.SIDES:
        leg = lambda t: float(np.interp(t, [-0.12, 0.0, 0.25, 0.5, 0.65, 0.95], [0.104, 0.102, 0.088, 0.068, 0.066, 0.056]))
        hero.add(f'Trousers_{side}', lh.trouser(sk, side, leg, -0.12, 0.7), 'pants', bones=lh.LEG_BONES(side))
        putt = lh.trouser(sk, side, lambda t: 0.06 + 0.008 * math.sin((t - 0.55) * 60.0) ** 2 * 0.5, 0.55, 0.98)
        hero.add(f'Puttee_{side}', putt, 'puttee', bones=lh.LEG_BONES(side))
        for k in range(5):
            t = 0.6 + k * 0.075
            hero.add(f'PutteeWrap_{side}_{k}', lh.trouser(sk, side, lambda tt: 0.064, t, t + 0.012, n=16), 'coatDark',
                     bones={f'calf_{side}': 1.0})
        shaft, foot = lh.boot(sk, side, shaft_top=0.16, shaft_r=0.062, cuff=0.004, width=0.056, toe_up=0.008)
        hero.add(f'BootShaft_{side}', shaft, 'boots', bones=lh.FOOT_BONES(side))
        hero.add(f'Boot_{side}', foot, 'boots', bones=lh.FOOT_BONES(side))
        for i, (shape, how) in enumerate(lh.hand_shapes(sk, side, glove=1.04, finger_r=0.0105, thumb_r=0.0122)):
            hero.add(f'Hand_{side}_{i}', shape, 'skin', bones=how[1], power=how[2])

    # --- the field coat to mid-thigh, open at the throat, buttoned below, its hem parting over the legs
    rows = []
    for t, gap in ((0.985, 0.85), (0.94, 0.6), (0.87, 0.42), (0.79, 0.3), (0.66, 0.08), (0.52, 0.04), (0.36, 0.04),
                   (0.2, 0.05), (0.05, 0.08)):
        w, d, f = (np.interp(t, [r[0] for r in body], [r[i] for r in body]) for i in (1, 2, 3))
        rows.append(dict(z=zt(t), rx=w + 0.018, ry=d + 0.018, fwd=f, gap=gap))
    waist = zt(0.05)
    hem_z = sk.knee_z + 0.16
    for i in range(1, 5):
        u = i / 4
        z = waist + (hem_z - waist) * u
        rows.append(dict(z=z, rx=0.204 + 0.03 * u, ry=0.158 + 0.03 * u, fwd=0.0, gap=0.1 + 0.22 * u))
    coat, edges = lh.garment(sk, rows, n=48, thickness=0.008)
    hero.add('Coat', coat, 'coat', weights=lh.garment_weights(hero, waist, stiff=0.75, width=0.36))
    for sgn, side in ((1, 'l'), (-1, 'r')):
        hero.add(f'Lapel_{side}', lh.lapel_strip(edges, [0, 1, 2, 3], [0.012, 0.034, 0.04, 0.02], sgn), 'coatDark',
                 bones=lh.TORSO_BONES)
    for k, t in enumerate((0.6, 0.48, 0.36, 0.24)):
        p, n_, _ = lh.garment_surface(sk, rows, 0.0, zt(t))
        hero.add(f'Button_{k}', lh.ellipsoid(p + n_ * 0.006, (0.009, 0.006, 0.009), 10, 6), 'leather', rigid='spine_02')
    # Pockets: two on the chest, two big ones on the skirt.
    for sx in (1.0, -1.0):
        for z, w, h, name in ((zt(0.7), 0.085, 0.09, 'Chest'), (zt(-0.05), 0.12, 0.12, 'Hip')):
            p, n_, tan = lh.garment_surface(sk, rows, sx * (0.095 if name == 'Chest' else 0.12), z)
            hero.add(f'{name}Pocket_{sx:+.0f}', lh.box(p + n_ * 0.006, (w, h, 0.012), axes=(tan, v3(0, 0, 1), None), bevel=0.12),
                     'coat', weights=lh.garment_weights(hero, waist, stiff=0.75, width=0.36))
            hero.add(f'{name}Flap_{sx:+.0f}', lh.box(p + n_ * 0.014 + v3(0, 0, h * 0.42), (w + 0.01, h * 0.32, 0.01),
                                                     axes=(tan, v3(0, 0, 1), None), bevel=0.15),
                     'coatDark', weights=lh.garment_weights(hero, waist, stiff=0.75, width=0.36))
    for side, sx in lh.SIDES:
        sleeve_r = lambda t: float(np.interp(t, [-0.12, -0.05, 0.04, 0.2, 0.5, 0.75, 0.9], [0.05, 0.08, 0.088, 0.078, 0.066, 0.064, 0.06]))
        hero.add(f'Sleeve_{side}', lh.sleeve(sk, side, sleeve_r, -0.12, 0.9), 'coat', bones=lh.ARM_BONES(side))
        hero.add(f'Cuff_{side}', lh.sleeve(sk, side, lambda t: 0.066, start=0.82, end=0.92), 'coatDark',
                 bones={f'lowerarm_{side}': 1.0, f'hand_{side}': 0.3})

    # --- the cartridge belt over the coat, the spyglass at the hip
    z0, z1 = zt(0.26), zt(0.33)
    bw = float(np.interp(0.3, [r[0] for r in body], [r[1] for r in body])) + 0.03
    bd = float(np.interp(0.3, [r[0] for r in body], [r[2] for r in body])) + 0.03
    hero.add('Belt', lh.band(v3(0, lh.spine_at(sk, (z0 + z1) / 2)[1] - 0.028, 0), (bw, bd), z0, z1, 48, 2.3), 'leather',
             rigid='spine_01')
    for k in range(4):
        a = math.pi / 2 + (-1.15 - 0.35 * k if k < 2 else 1.15 + 0.35 * (k - 2))
        p = v3(math.cos(a) * (bw + 0.012), lh.spine_at(sk, (z0 + z1) / 2)[1] - 0.028 - math.sin(a) * (bd + 0.012), (z0 + z1) / 2 - 0.01)
        out = lh.unit(v3(math.cos(a), -math.sin(a), 0))
        hero.add(f'Pouch_{k}', lh.box(p + out * 0.014, (0.06, 0.07, 0.035), axes=(lh.unit(np.cross(v3(0, 0, 1), out)), v3(0, 0, 1), None),
                                      bevel=0.2), 'leather', rigid='spine_01')
    hero.add('Buckle', lh.prism(lh.rounded_rect(0.055, 0.06, 0.008), 0.012,
                                v3(0.0, lh.spine_at(sk, (z0 + z1) / 2)[1] - 0.028 - bd - 0.006, (z0 + z1) / 2), v3(1, 0, 0),
                                v3(0, 0, 1), 0.2), 'brass', rigid='spine_01')
    glass_top = v3(-0.215, -0.05, z0 - 0.02)
    glass_dir = lh.unit(v3(-0.08, -0.12, -1.0))
    hero.add('Spyglass', lh.tube([glass_top + glass_dir * t for t in (0.0, 0.07, 0.071, 0.14, 0.141, 0.2)],
                                 [0.024, 0.024, 0.021, 0.021, 0.018, 0.018], 14), 'brass', rigid='pelvis')
    hero.add('SpyglassStrap', lh.band(glass_top - v3(0, 0, 0.0) + glass_dir * 0.04, (0.027, 0.027), -0.008, 0.008, 14), 'leather',
             rigid='pelvis')

    # --- the pack: canvas body with a flap, bedroll under it, map tube on top, tripod and rifle strapped beside it
    back_z = zt(0.62)
    back_y = lh.spine_at(sk, back_z)[1] + 0.13
    pack_c = v3(0.0, back_y + 0.11, back_z)
    hero.add('Pack', lh.box(pack_c, (0.36, 0.2, 0.42), bevel=0.22), 'canvas', bones={'spine_03': 0.6, 'spine_04': 1.0, 'spine_05': 0.6})
    hero.add('PackFlap', lh.box(pack_c + v3(0, 0.0, 0.17), (0.37, 0.215, 0.12), bevel=0.25), 'coatDark',
             bones={'spine_04': 1.0, 'spine_05': 0.8})
    for sx in (1.0, -1.0):
        hero.add(f'PackBuckle_{sx:+.0f}', lh.box(pack_c + v3(sx * 0.1, 0.105, 0.1), (0.035, 0.012, 0.05), bevel=0.2), 'brass',
                 rigid='spine_04')
    roll_c = pack_c + v3(0, 0.0, -0.27)
    hero.add('Bedroll', lh.limb(roll_c - v3(0.21, 0, 0), roll_c + v3(0.21, 0, 0), 0.065, 0.065, 4, 16, hint=(0, 0, 1)), 'bedroll',
             bones={'spine_02': 0.6, 'spine_03': 1.0})
    for sx in (1.0, -1.0):
        hero.add(f'BedrollStrap_{sx:+.0f}', lh.torus(roll_c + v3(sx * 0.13, 0, 0), (1, 0, 0), 0.068, 0.006, 20, 5), 'leather',
                 rigid='spine_03')
    tube_c = pack_c + v3(0, 0.02, 0.27)
    hero.add('MapTube', lh.limb(tube_c - v3(0.26, 0, 0), tube_c + v3(0.26, 0, 0), 0.042, 0.042, 4, 16, hint=(0, 0, 1)), 'leather',
             bones={'spine_05': 1.0, 'spine_04': 0.5})
    for sx in (1.0, -1.0):
        hero.add(f'MapCap_{sx:+.0f}', lh.limb(tube_c + v3(sx * 0.255, 0, 0), tube_c + v3(sx * 0.28, 0, 0), 0.046, 0.046, 1, 16,
                                              hint=(0, 0, 1)), 'brass', rigid='spine_05')
    # The folded tripod on the left of the pack, the rifle on the right with its scope.
    for k in range(3):
        off = v3(0.205 + 0.016 * (k - 1), 0.07 + 0.018 * (k % 2), 0.0)
        a, c = pack_c + off + v3(0, 0, -0.28), pack_c + off + v3(0.01, 0.0, 0.42)
        hero.add(f'TripodLeg_{k}', lh.limb(a, c, 0.012, 0.01, 3, 8), 'wood', bones={'spine_03': 0.6, 'spine_04': 1.0, 'spine_05': 0.6})
    hero.add('TripodHead', lh.lathe([(0.0, 0.0), (0.032, 0.0), (0.03, 0.03), (0.0, 0.034)], pack_c + v3(0.212, 0.08, 0.42), n=14),
             'brass', rigid='spine_05')
    rifle_a, rifle_b = pack_c + v3(-0.225, 0.04, -0.3), pack_c + v3(-0.205, 0.07, 0.6)
    hero.add('RifleStock', lh.limb(rifle_a, rifle_a + (rifle_b - rifle_a) * 0.35, 0.03, 0.02, 3, 10, flat=0.55, hint=(1, 0, 0)),
             'wood', bones={'spine_03': 0.6, 'spine_04': 1.0, 'spine_05': 0.6})
    hero.add('RifleBarrel', lh.limb(rifle_a + (rifle_b - rifle_a) * 0.33, rifle_b, 0.013, 0.009, 4, 8), 'gunmetal',
             bones={'spine_04': 0.6, 'spine_05': 1.0})
    scope_a = rifle_a + (rifle_b - rifle_a) * 0.42 + v3(0, 0.03, 0)
    hero.add('RifleScope', lh.limb(scope_a, scope_a + (rifle_b - rifle_a) * 0.28, 0.016, 0.016, 2, 12), 'brass',
             bones={'spine_04': 0.6, 'spine_05': 1.0})
    # Pack straps over the shoulders and down the chest.
    for sx in (1.0, -1.0):
        pts = []
        for t in (0.98, 0.93, 0.86, 0.76, 0.64, 0.5):
            w, d, f = (np.interp(t, [r[0] for r in body], [r[i] for r in body]) for i in (1, 2, 3))
            x = sx * (0.12 if t > 0.9 else 0.11)
            if t > 0.9:
                p = v3(x, lh.spine_at(sk, zt(t))[1] + (0.03 if t > 0.95 else -0.02), zt(t) + 0.045)
            else:
                p, n_, _ = lh.garment_surface(sk, rows, x, zt(t))
                p = p + n_ * 0.008
            pts.append(p)
        pts.insert(0, v3(sx * 0.12, back_y + 0.02, zt(0.9)))
        hero.add(f'Strap_{sx:+.0f}', lh.tube(pts, [(0.006, 0.026)] * len(pts), 8, hint=(0, 0, 1)), 'leather', bones=lh.TORSO_BONES)

    # --- the head: ruddy face, round nose, spectacles, mutton chops and mustache, neckerchief, bowler with goggles
    _, lft, fwd, up = lh.head_frame(sk)
    hero.add('Neck', lh.neck(sk, 0.06, 0.056), 'skin', bones=lh.NECK_BONES)
    hero.add('Head', lh.skull(sk, HEAD), 'skin', rigid='head')
    hero.add('Nose', lh.ellipsoid(lh.on_head(sk, 0.0, HEAD['ry'] * 1.0, -0.012), (0.017, 0.016, 0.019), 12, 8, axes=(lft, fwd, up)),
             'skin', rigid='head')
    for sx in (1.0, -1.0):
        eye = lh.on_head(sk, sx * 0.032, HEAD['ry'] * 0.9, 0.018)
        hero.add(f'Eye_{sx:+.0f}', lh.ellipsoid(eye, (0.009, 0.005, 0.008), 10, 6, axes=(lft, fwd, up)), 'eyes', rigid='head')
        hero.add(f'Lens_{sx:+.0f}', lh.torus(eye + fwd * 0.012, fwd, 0.02, 0.0028, 20, 5), 'brass', rigid='head')
        hero.add(f'LensGlass_{sx:+.0f}', lh.lathe([(0.0, 0.0), (0.019, 0.0), (0.019, 0.002), (0.0, 0.002)], eye + fwd * 0.011, axis=fwd,
                                                  n=16), 'glass', rigid='head')
        hero.add(f'Temple_{sx:+.0f}', lh.tube([eye + fwd * 0.012 + lft * sx * 0.02, lh.on_head(sk, sx * 0.085, 0.02, 0.022)],
                                              0.0022, 5), 'brass', rigid='head')
        brow = [lh.on_head(sk, sx * 0.015, HEAD['ry'] * 0.95, 0.042), lh.on_head(sk, sx * 0.034, HEAD['ry'] * 0.93, 0.048),
                lh.on_head(sk, sx * 0.052, HEAD['ry'] * 0.82, 0.044)]
        hero.add(f'Brow_{sx:+.0f}', lh.tube(brow, [0.007, 0.008, 0.006], 7), 'whiskers', rigid='head')
        ear = lh.on_head(sk, sx * 0.09, -0.006, 0.0)
        hero.add(f'Ear_{sx:+.0f}', lh.ellipsoid(ear, (0.013, 0.023, 0.032), 10, 8, axes=(lft, fwd, up)), 'skin', rigid='head')
        # Mutton chops: down the cheek from the ear to the jaw, joined to the mustache.
        chop = [lh.on_head(sk, sx * 0.082, 0.03, 0.01), lh.on_head(sk, sx * 0.08, 0.05, -0.03), lh.on_head(sk, sx * 0.07, 0.07, -0.06),
                lh.on_head(sk, sx * 0.05, 0.09, -0.065), lh.on_head(sk, sx * 0.02, HEAD['ry'] * 0.98, -0.04)]
        hero.add(f'Chop_{sx:+.0f}', lh.tube(chop, [0.016, 0.022, 0.024, 0.018, 0.012], 10), 'whiskers', rigid='head')
    hero.add('Bridge', lh.tube([lh.on_head(sk, 0.012, HEAD['ry'] * 1.02, 0.018), lh.on_head(sk, -0.012, HEAD['ry'] * 1.02, 0.018)],
                               0.0024, 5), 'brass', rigid='head')
    stache = [lh.on_head(sk, -0.045, HEAD['ry'] * 0.88, -0.05), lh.on_head(sk, -0.02, HEAD['ry'] * 1.0, -0.036),
              lh.on_head(sk, 0.0, HEAD['ry'] * 1.03, -0.034), lh.on_head(sk, 0.02, HEAD['ry'] * 1.0, -0.036),
              lh.on_head(sk, 0.045, HEAD['ry'] * 0.88, -0.05)]
    hero.add('Mustache', lh.tube(stache, [0.009, 0.014, 0.015, 0.014, 0.009], 10), 'whiskers', rigid='head')
    # The neckerchief, knotted at the front.
    hero.add('Neckerchief', lh.band(v3(0, sk.head('neck_01')[1] - 0.004, 0), (0.07, 0.066), sk.neck_z - 0.005, sk.neck_z + 0.04, 28),
             'kerchief', bones={'neck_01': 1.0, 'spine_05': 0.6})
    knot = v3(0.0, sk.head('neck_01')[1] - 0.075, sk.neck_z + 0.01)
    hero.add('KerchiefKnot', lh.ellipsoid(knot, (0.022, 0.014, 0.018), 10, 8), 'kerchief', bones={'neck_01': 1.0, 'spine_05': 0.6})
    for sx in (1.0, -1.0):
        hero.add(f'KerchiefEnd_{sx:+.0f}', lh.limb(knot + v3(0, -0.004, -0.008), knot + v3(sx * 0.03, -0.014, -0.075), 0.014, 0.02, 3, 8,
                                                    hint=(0, -1, 0), flat=0.35), 'kerchief', bones={'spine_05': 1.0, 'neck_01': 0.4})
    bowler(hero, sk)
    return hero


def bowler(hero, sk, s=1.0):
    """A brown bowler: a round crown, a short brim curled up at the sides, a dark band, and brass goggles strapped
    round it."""
    _, lft, fwd, up = lh.head_frame(sk)
    base = lh.on_head(sk, 0.0, -0.002, 0.055)
    tilt = lh.unit(up + fwd * 0.05)
    r_ax = lh.unit(lft - tilt * float(lft @ tilt))
    f_ax = lh.unit(np.cross(tilt, r_ax))
    n = 44
    u = np.linspace(0.0, 2.0 * math.pi, n, endpoint=False)
    rows = []
    for r in (0.094, 0.11, 0.125, 0.133):
        k = (r - 0.094) / 0.039
        curl = 0.03 * (np.abs(np.cos(u)) ** 2) * k ** 2 + 0.006 * k ** 3
        pts = base[None, :] + (np.cos(u) * r)[:, None] * r_ax[None, :] + (np.sin(u) * r * 1.08)[:, None] * f_ax[None, :] + \
            curl[:, None] * tilt[None, :]
        rows.append(pts)
    hero.add('HatBrim', lh.thicken(lh.loft(rows, cap_start=False, cap_end=False), 0.008), 'hat', rigid='head')
    prof = [(0.095, -0.004), (0.097, 0.03), (0.094, 0.06), (0.082, 0.09), (0.058, 0.11), (0.028, 0.12), (0.0, 0.122)]
    hero.add('HatCrown', lh.lathe(prof, base, axis=tilt, n=n, hint=r_ax, squash=1.1), 'hat', rigid='head')
    hero.add('HatBand', lh.lathe([(0.0985, 0.0), (0.0985, 0.02)], base, axis=tilt, n=n, hint=r_ax, squash=1.1, cap_start=False,
                                 cap_end=False), 'hatband', rigid='head')
    hero.add('GoggleStrap', lh.lathe([(0.0995, 0.026), (0.0995, 0.044)], base, axis=tilt, n=n, hint=r_ax, squash=1.1,
                                     cap_start=False, cap_end=False), 'leather', rigid='head')
    for sx in (1.0, -1.0):
        c = base + tilt * 0.04 + f_ax * 0.112 + r_ax * sx * 0.034
        out = lh.unit(f_ax * 0.95 + tilt * 0.3)
        hero.add(f'Goggle_{sx:+.0f}', lh.lathe([(0.0, 0.0), (0.026, 0.0), (0.028, 0.012), (0.024, 0.024), (0.0, 0.024)], c, axis=out,
                                               n=18), 'brass', rigid='head')
        hero.add(f'GoggleGlass_{sx:+.0f}', lh.lathe([(0.0, 0.0), (0.02, 0.0), (0.0, 0.004)], c + out * 0.024, axis=out, n=16), 'glass',
                 rigid='head')


hero = build()
hero.finish()
