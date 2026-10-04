"""Marshal Hollis Crane, the Unpaid: one of the five playable heroes concepted on 2026-10-04. A lawman dead forty
years who could not cross: his town's saint went dark, so he walks as one of the Unpaid (Docs/Story.md). Sexton never
paid his fare, so one eye still wears the ferryman's coin and the other burns with grave light. Kept in the backlog
(Art/Backlog/README.md): nothing in the game uses it yet.

The look: very tall, gaunt and stooped, grey-green skin over a long skull, a drooping white lawman's mustache and
long white hair, a tarnished coin over the left eye and cyan light in the right socket, an ember glowing through a
tear in his shirt, a long tattered slate coat with a ragged shoulder cape, a tin star, and the toll chain slung from
shoulder to hip with its iron hook.

Ember powers (a design sketch, not in the game): Toll Chain (hook a creature and drag it in, stunned), Grave Ward
(plant a circle of grave light: creatures in it slow, Crane mends) and Last Rites (mark a creature; if it dies within
8 s its soul bursts on its pack, else the mark strikes it hard).

The rig is the UE5 mannequin's skeleton (looter_heroes); 1.95 m tall.
"""
import math

import numpy as np

import looter_heroes as lh
from looter_heroes import v3

PALETTE = {
    'coat': (0x3c4552, 'fill'), 'coatDark': (0x2a3039, 'fill'), 'shirt': (0x5b605a, 'fill'), 'pants': (0x4a4440, 'fill'),
    'boots': (0x2e2a28, 'fill'), 'leather': (0x4f3a2c, 'fill'), 'skin': (0xb4c0b0, 'fill'), 'socket': (0x1c2226, 'fill'),
    'hair': (0xdcd9cf, 'fill'), 'glow': (0x6ee7ff, 'glow'), 'coin': (0xd4b04c, 'accent'), 'star': (0xd8c27a, 'accent'),
    'iron': (0x5a5e64, 'fill'), 'grip': (0x6b5844, 'fill'), 'hat': (0x4a4540, 'fill'), 'hatband': (0x2a3039, 'fill'),
}
HEAD = dict(rx=0.077, ry=0.1, rz=0.112, jaw=0.72, chin=0.018, back=0.97)


def build():
    b = lh.Build(height=1.95, shoulders=0.40, hips=0.19, head=0.98, neck=0.95, arm=1.06, leg=1.05, hand=1.2, foot=1.1,
                 stoop=0.05, arm_angle=45.0)
    hero = lh.Hero('Crane', b, PALETTE)
    sk = hero.sk
    T = sk.T
    zt = lambda t: sk.hip_z + t * T

    # --- a thin frame: shirt, trousers, worn boots, bare bony hands
    body = [(-0.16, 0.13, 0.092, 0.0), (-0.06, 0.15, 0.1, 0.0), (0.05, 0.152, 0.1, 0.0), (0.2, 0.14, 0.094, 0.0),
            (0.36, 0.134, 0.092, 0.0), (0.52, 0.146, 0.1, 0.006), (0.66, 0.16, 0.106, 0.01), (0.79, 0.17, 0.104, 0.008),
            (0.87, 0.172, 0.098, 0.004), (0.94, 0.15, 0.086, 0.0), (0.985, 0.1, 0.07, 0.0), (1.0, 0.062, 0.058, 0.0)]
    hero.add('Shirt', lh.torso(sk, body), 'shirt', bones=lh.TORSO_BONES)
    # The ember that keeps him walking glows through a tear over the heart.
    heart_z = zt(0.7)
    heart = v3(0.05, lh.spine_at(sk, heart_z)[1] - 0.1, heart_z)
    hero.add('Ember', lh.ellipsoid(heart, (0.03, 0.016, 0.036), 14, 10), 'glow', rigid='spine_04')
    tear = [(-0.03, 0.04), (0.0, 0.05), (0.034, 0.03), (0.04, -0.01), (0.02, -0.045), (-0.012, -0.05), (-0.04, -0.015)]
    hero.add('Tear', lh.prism([(x * 1.25, y * 1.25) for x, y in tear], 0.004, heart + v3(0, -0.004, 0), v3(1, 0, 0),
                              v3(0, 0, 1)), 'socket', rigid='spine_04')
    for side, sx in lh.SIDES:
        leg = lambda t: float(np.interp(t, [-0.12, 0.0, 0.25, 0.5, 0.7, 0.95], [0.086, 0.084, 0.07, 0.054, 0.054, 0.047]))
        hero.add(f'Trousers_{side}', lh.trouser(sk, side, leg, -0.12, 0.95), 'pants', bones=lh.LEG_BONES(side))
        shaft, foot = lh.boot(sk, side, shaft_top=0.36, shaft_r=0.06, cuff=0.004, width=0.048, toe_up=0.02)
        hero.add(f'BootShaft_{side}', shaft, 'boots', bones=lh.FOOT_BONES(side))
        hero.add(f'Boot_{side}', foot, 'boots', bones=lh.FOOT_BONES(side))
        for i, (shape, how) in enumerate(lh.hand_shapes(sk, side, glove=0.98, finger_r=0.0082, thumb_r=0.0098,
                                                        palm_scale=(1.0, 0.9, 0.85))):
            hero.add(f'Hand_{side}_{i}', shape, 'skin', bones=how[1], power=how[2])
        hero.add(f'Wrist_{side}', lh.sleeve(sk, side, lambda t: 0.03, start=0.85, end=1.0), 'skin',
                 bones={f'lowerarm_{side}': 1.0, f'hand_{side}': 1.0})

    # --- the long coat: tattered at the hem, open over the chest, and a ragged shoulder cape
    waist = zt(0.2)
    rows = []
    for t, gap in ((0.985, 1.0), (0.94, 0.8), (0.87, 0.64), (0.79, 0.54), (0.66, 0.48), (0.52, 0.42), (0.36, 0.36),
                   (0.2, 0.34)):
        w, d, f = (np.interp(t, [r[0] for r in body], [r[i] for r in body]) for i in (1, 2, 3))
        rows.append(dict(z=zt(t), rx=w + 0.02, ry=d + 0.02, fwd=f, gap=gap))
    hem_z = 0.34
    for i in range(1, 9):
        u = i / 8
        z = waist + (hem_z - waist) * u
        rows.append(dict(z=z, rx=0.172 + 0.07 * u ** 1.2, ry=0.118 + 0.075 * u ** 1.2, fwd=-0.006 * u, gap=0.36 + 0.3 * u))

    def tatters(u, z):
        return 0.03 + 0.07 * np.abs(np.sin(u * 7.3 + 0.6)) ** 3 + 0.025 * np.sin(u * 19.0) ** 2

    coat, edges = lh.garment(sk, rows, n=56, thickness=0.008, hem=tatters)
    hero.add('Coat', coat, 'coat', weights=lh.garment_weights(hero, waist, stiff=0.82, width=0.3))
    for sgn, side in ((1, 'l'), (-1, 'r')):
        hero.add(f'Lapel_{side}', lh.lapel_strip(edges, list(range(0, 7)), [0.01, 0.03, 0.045, 0.05, 0.042, 0.03, 0.016], sgn),
                 'coatDark', bones=lh.TORSO_BONES)
    for side, sx in lh.SIDES:
        sleeve_r = lambda t: float(np.interp(t, [-0.12, -0.05, 0.04, 0.2, 0.5, 0.75, 0.88], [0.04, 0.058, 0.064, 0.058, 0.052, 0.05, 0.047]))
        sleeve = lh.sleeve(sk, side, sleeve_r, -0.12, 0.88)
        hero.add(f'Sleeve_{side}', sleeve, 'coat', bones=lh.ARM_BONES(side))
        hero.add(f'Cuff_{side}', lh.sleeve(sk, side, lambda t: 0.057, start=0.78, end=0.89), 'coatDark',
                 bones={f'lowerarm_{side}': 1.0, f'hand_{side}': 0.3})
    cape_rows = []
    base = sk.head('neck_01')
    for i, (dz, rx, ry) in enumerate(((0.02, 0.085, 0.074), (-0.03, 0.15, 0.11), (-0.09, 0.235, 0.145), (-0.17, 0.29, 0.17),
                                      (-0.25, 0.31, 0.18))):
        c = base + v3(0, 0.01, dz)
        cape_rows.append(dict(z=c[2], rx=rx, ry=ry, fwd=-0.0, gap=0.42 if i else 0.8, power=2.0))

    def cape_tatters(u, z):
        return 0.02 + 0.06 * np.abs(np.sin(u * 6.1 + 2.0)) ** 3 + 0.02 * np.sin(u * 15.0) ** 2

    cape, _ = lh.garment(sk, cape_rows, n=60, thickness=0.007, hem=cape_tatters)
    hero.add('Cape', cape, 'coatDark', bones=lh.TORSO_BONES | {'upperarm_l': 0.7, 'upperarm_r': 0.7, 'spine_05': 1.4,
                                                                'clavicle_l': 1.0, 'clavicle_r': 1.0})
    # The tin star on the left breast.
    star_z = zt(0.76)
    star_c = v3(0.095, lh.spine_at(sk, star_z)[1] - 0.122, star_z)
    hero.add('Star', lh.prism(lh.star(5, 0.032, 0.014), 0.006, star_c, lh.unit(v3(1, 0.45, 0)), v3(0, 0, 1), 0.15), 'star',
             rigid='spine_04')

    # --- the gun belt and an old revolver, and the toll chain from the right shoulder to the left hip
    z0, z1 = sk.hip_z - 0.02, sk.hip_z + 0.025
    hero.add('Belt', lh.band(v3(0, 0.002, 0), (0.166, 0.112), z0, z1, 44, 2.4), 'leather', rigid='pelvis')
    hero.add('Buckle', lh.prism(lh.rounded_rect(0.05, 0.04, 0.006), 0.01, v3(0.0, -0.118, (z0 + z1) / 2), v3(1, 0, 0),
                                v3(0, 0, 1), 0.2), 'iron', rigid='pelvis')
    top = v3(-0.17, -0.03, z0)
    down = lh.unit(v3(-0.05, -0.08, -1.0))
    hero.add('Holster', lh.limb(top, top + down * 0.2, 0.034, 0.026, 4, 12, hint=(1, 0, 0), flat=0.55, power=2.6), 'leather',
             rigid='pelvis')
    hero.add('RevolverGrip', lh.limb(top + v3(0, -0.004, 0.012), top + v3(0.012, 0.03, 0.07), 0.018, 0.016, 3, 10, flat=0.7),
             'grip', rigid='pelvis')
    a = v3(-0.15, lh.spine_at(sk, zt(0.92))[1] - 0.02, zt(0.93))
    c = v3(0.15, -0.12, z0 + 0.01)
    links = []
    count = 20
    for k in range(count):
        u = k / (count - 1)
        p = a + (c - a) * u
        z = p[2]
        row_w = np.interp(z, [zt(t) for t, *_ in body], [w for _, w, _, _ in body]) + 0.03
        row_d = np.interp(z, [zt(t) for t, *_ in body], [d for _, _, d, _ in body]) + 0.03
        x = np.clip(p[0], -row_w * 0.98, row_w * 0.98)
        y = lh.spine_at(sk, z)[1] - row_d * math.sqrt(max(0.0, 1 - (x / row_w) ** 2)) - 0.012
        links.append(v3(x, y, z))
    for k in range(count - 1):
        mid = (links[k] + links[k + 1]) / 2
        axis = lh.unit(links[k + 1] - links[k])
        ring_axis = lh.unit(np.cross(axis, v3(0, -1, 0))) if k % 2 == 0 else v3(0, -1, 0)
        link = lh.torus(mid, ring_axis, 0.016, 0.0042, 12, 6, squash=1.6, hint=axis)
        hero.add(f'Chain_{k}', link, 'iron', bones=lh.TORSO_BONES | {'pelvis': 1.2})
    hook_top = links[-1] + v3(0.0, -0.01, -0.02)
    hook = [hook_top, hook_top + v3(0, -0.004, -0.06), hook_top + v3(0.0, -0.02, -0.1), hook_top + v3(0.0, -0.045, -0.095),
            hook_top + v3(0.0, -0.05, -0.07)]
    hero.add('Hook', lh.tube(hook, [0.008, 0.008, 0.007, 0.006, 0.003], 8), 'iron', rigid='pelvis')

    # --- the head: a long skull, a coin and a burning socket, a drooping mustache, long white hair
    _, lft, fwd, up = lh.head_frame(sk)
    hero.add('Neck', lh.neck(sk, 0.042, 0.04), 'skin', bones=lh.NECK_BONES)
    hero.add('Head', lh.skull(sk, HEAD), 'skin', rigid='head')
    for sx in (1.0, -1.0):
        cheek = lh.on_head(sk, sx * 0.05, HEAD['ry'] * 0.72, -0.012)
        hero.add(f'Cheekbone_{sx:+.0f}', lh.ellipsoid(cheek, (0.022, 0.016, 0.014), 10, 8, axes=(lft, fwd, up)), 'skin', rigid='head')
        sock = lh.on_head(sk, sx * 0.031, HEAD['ry'] * 0.8, 0.02)
        hero.add(f'Socket_{sx:+.0f}', lh.ellipsoid(sock, (0.02, 0.012, 0.016), 12, 8, axes=(lft, fwd, up)), 'socket', rigid='head')
    coin = lh.on_head(sk, 0.031, HEAD['ry'] * 0.9, 0.02)
    coin_axis = lh.unit(fwd + lft * 0.25)
    hero.add('Coin', lh.lathe([(0.0, 0.0), (0.02, 0.0), (0.021, 0.003), (0.02, 0.006), (0.0, 0.006)], coin, axis=coin_axis,
                              n=18), 'coin', rigid='head')
    hero.add('EyeLight', lh.ellipsoid(lh.on_head(sk, -0.031, HEAD['ry'] * 0.86, 0.02), (0.008, 0.005, 0.008), 10, 8,
                                      axes=(lft, fwd, up)), 'glow', rigid='head')
    hero.add('Nose', lh.ellipsoid(lh.on_head(sk, 0.0, HEAD['ry'] * 0.93, -0.012), (0.0085, 0.012, 0.022), 10, 8,
                                  axes=(lft, lh.unit(fwd - up * 0.25), lh.unit(up + fwd * 0.25))), 'skin', rigid='head')
    hero.add('Mouth', lh.ellipsoid(lh.on_head(sk, 0.0, HEAD['ry'] * 0.84, -0.062), (0.017, 0.005, 0.003), 10, 6,
                                   axes=(lft, fwd, up)), 'socket', rigid='head')
    # A horseshoe mustache lying on the lip: out from under the nose, then down past the corners of the mouth.
    for sx in (1.0, -1.0):
        pts = [lh.on_head(sk, sx * 0.003, HEAD['ry'] * 0.95, -0.038), lh.on_head(sk, sx * 0.018, HEAD['ry'] * 0.92, -0.044),
               lh.on_head(sk, sx * 0.03, HEAD['ry'] * 0.84, -0.056), lh.on_head(sk, sx * 0.033, HEAD['ry'] * 0.8, -0.082),
               lh.on_head(sk, sx * 0.032, HEAD['ry'] * 0.76, -0.1)]
        hero.add(f'Mustache_{sx:+.0f}', lh.tube(pts, [0.0065, 0.0075, 0.0065, 0.005, 0.0022], 8), 'hair', rigid='head')
    hair = lh.skull(sk, dict(HEAD, rx=HEAD['rx'] + 0.006, ry=HEAD['ry'] + 0.007, rz=HEAD['rz'] + 0.004, jaw=0.95, chin=0.0))
    hair = lh.cut(hair, lambda v: (lh.head_local(sk, v)[:, 1] < 0.02) & (lh.head_local(sk, v)[:, 2] > -0.045))
    hero.add('Hair', hair, 'hair', rigid='head')
    # Thin white hair falls from under the hat over the collar: flat locks, tapering to points.
    for k in range(14):
        a = math.radians(118 + k * (124 / 13))
        start = lh.on_head(sk, math.cos(a) * HEAD['rx'] * 1.02, math.sin(a) * HEAD['ry'] * 0.95, -0.02 + 0.01 * math.sin(k))
        out = lh.unit(lft * math.cos(a) + fwd * math.sin(a))
        length = 0.13 + 0.06 * math.sin(k * 2.3) ** 2
        pts = [start, start + out * 0.012 - up * 0.05, start + out * 0.022 - up * length * 0.7 - fwd * 0.012,
               start + out * 0.026 - up * length - fwd * 0.024]
        side = lh.unit(np.cross(up, out))
        hero.add(f'Strand_{k}', lh.tube(pts, [(0.004, 0.012), (0.0035, 0.011), (0.003, 0.008), (0.001, 0.003)], 7, hint=out),
                 'hair', bones={'head': 1.0, 'neck_02': 0.6, 'neck_01': 0.5, 'spine_05': 0.3})
    slouch_hat(hero, sk)
    return hero


def slouch_hat(hero, sk, s=1.0):
    """A battered slouch hat: a low dented dome crown and a soft wide brim that sags at the front and back, with a
    tear in its edge."""
    _, lft, fwd, up = lh.head_frame(sk)
    base = lh.on_head(sk, 0.0, -0.004, 0.05 * s)
    tilt = lh.unit(up + fwd * 0.18)
    r_ax = lh.unit(lft - tilt * float(lft @ tilt))
    f_ax = lh.unit(np.cross(tilt, r_ax))
    n = 52
    u = np.linspace(0.0, 2.0 * math.pi, n, endpoint=False)
    rows = []
    for r in (0.085, 0.115, 0.145, 0.175):
        k = (r - 0.085) / 0.09
        sag = -0.05 * (np.abs(np.sin(u)) ** 1.5) * k ** 1.4 - 0.012 * k
        tear = np.where(np.abs(u - 0.9) < 0.12, -0.03 * k ** 2, 0.0)
        wob = 0.006 * np.sin(u * 5.0 + 1.0) * k
        rr = r * (1.0 - 0.18 * (np.abs(u - 0.9) < 0.1) * k ** 3)
        pts = base[None, :] + (np.cos(u) * rr)[:, None] * r_ax[None, :] + (np.sin(u) * rr * 1.05)[:, None] * f_ax[None, :] + \
            (sag + tear + wob)[:, None] * tilt[None, :]
        rows.append(pts)
    hero.add('HatBrim', lh.thicken(lh.loft(rows, cap_start=False, cap_end=False), 0.008), 'hat', rigid='head')
    prof = [(0.088, -0.004), (0.09, 0.03), (0.085, 0.07), (0.068, 0.1), (0.036, 0.115), (0.0, 0.118)]
    crown = lh.lathe(prof, base, axis=tilt, n=n, hint=r_ax, squash=1.08)

    def dent(v):
        rel = v - base[None, :]
        h, x = rel @ tilt, rel @ r_ax
        top = np.clip((h - 0.07) / 0.05, 0.0, 1.0)
        return v - tilt[None, :] * (0.03 * top * np.exp(-(x / 0.04) ** 2))[:, None]

    hero.add('HatCrown', crown.moved(dent), 'hat', rigid='head')
    hero.add('HatBand', lh.lathe([(0.0915, 0.0), (0.0915, 0.022)], base, axis=tilt, n=n, hint=r_ax, squash=1.08,
                                 cap_start=False, cap_end=False), 'hatband', rigid='head')


hero = build()
hero.finish()
