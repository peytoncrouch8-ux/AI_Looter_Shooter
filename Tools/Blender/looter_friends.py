"""The mountain's friendly dead (Art/Models/Creatures/Hiram.py and Thaddeus.py): what both build from. Amos.py's own code
run as a library (its living face, ears, shirt, trousers and shroud), its face rebuilt here with a beard and hair of
their own ('chops' or 'stubble'; 'short' or 'thin'), the head and neck placed and turned as each one rests; arms laid to
their rest, sleeved or rolled; the rig from their parts; a pose table in Amos_poses.json's layout.

Every model script under Art/Models runs on its own (Tools/models.ps1), so what they share lives here, beside
looter_ghosts.py."""
import math
import os

import numpy as np

import looter_ghosts as lg
import looter_model as lm
import looter_textures as lt

smoothstep, gauss, unit, wrap = lg.smoothstep, lg.gauss, lg.unit, lg.wrap
SIDES, SUFFIX = lg.SIDES, lg.SUFFIX
AMOS = os.path.join(lt.REPO, 'Art', 'Models', 'Creatures', 'Amos.py')


def amos_library():
    """Amos.py's source run up to its build into a namespace of its own: nothing of Amos is made. Its functions read
    their globals from it (FACE, TORSO, COAL_AT, KNEES, SHROUD_PATH, ...), so a model sets its own (configure)."""
    source = open(AMOS, encoding='utf-8').read()
    stop = '\nRIG, BODY, HAT, FORK, RAW_COLORS = build()'
    ns = {'__name__': 'amos_library', '__file__': AMOS}
    exec(compile(source[:source.index(stop)], AMOS, 'exec'), ns)
    return ns


def configure(A, **values):
    """Sets Amos.py's globals and what follows from them."""
    A.update(values)
    A['MOUTH_Z'] = A['FACE']['mouth']
    A['HEAD_CENTER'] = A['PIVOT'] + np.array([0.0, -0.012, 0.09])
    A['SHROUD_LENGTH'] = float(np.linalg.norm(np.diff(lg.spline(A['SHROUD_PATH'], 300), axis=0), axis=1).sum())


# --- The face: Amos's, with a beard and hair of its own ---

def chops_mask(TH, z, zm):
    """Mutton chops: thick from the sideburns down over the cheeks to the jaw's line, broadest at the jaw, joined over the
    upper lip by a heavy moustache; the chin shaved."""
    at = np.abs(TH)
    cheek = smoothstep(0.62, 0.8, at) * smoothstep(1.62, 1.45, at)
    top = 0.012 + 0.01 * smoothstep(1.2, 1.5, at)
    bottom = -0.118 + 0.02 * smoothstep(0.9, 0.62, at)
    side = cheek * smoothstep(top + 0.004, top - 0.006, z) * smoothstep(bottom - 0.006, bottom + 0.008, z)
    lip = gauss(TH, 0.3) * smoothstep(zm + 0.003, zm + 0.008, z) * smoothstep(zm + 0.03, zm + 0.02, z)
    reach = (smoothstep(0.24, 0.42, at) * smoothstep(0.8, 0.62, at) * smoothstep(zm - 0.026, zm - 0.012, z)
             * smoothstep(zm + 0.02, zm + 0.008, z))
    return np.clip(np.maximum(side, np.maximum(lip, 0.8 * reach)), 0.0, 1.0)


def head_surface(A, beard='stubble', hair='short', hair_zone=0, rows=150, cols=168):
    """Amos's head (face_radius, eye_fields, the lines of age, crow's feet, brows) with a beard and hair of its own:
    beard 'stubble' (Amos's shading) or 'chops' (grown out of the surface: mutton chops and a moustache); hair 'short'
    (under a cap, cut over the collar) or 'thin' (bald on top, a fringe round the back and over the ears). Returns
    ((V, faces), cavity, jaw weight, keep, tint zone) as Amos.py's does."""
    F = A['FACE']
    phi, th = A['face_grid'](rows, cols)
    PH, TH = np.meshgrid(phi, th, indexing='ij')
    d = np.stack([np.sin(PH) * np.sin(TH), -np.sin(PH) * np.cos(TH), np.cos(PH)], -1)
    r = A['face_radius'](d)
    z = d[..., 2] * r
    at = np.abs(TH)
    zm = A['MOUTH_Z']
    eyes, eye_cav = A['eye_fields'](TH, z)
    creases = sum(gauss(z - zk, 0.0028) * (1.0 + 0.35 * np.sin(TH * 9.0 + k * 1.7))
                  for k, zk in enumerate((0.05, 0.062, 0.074))) * gauss(TH, 0.6)
    R = eyes - 0.0011 * F['age'] * creases
    crow = np.zeros_like(TH)
    FACE_R = A['FACE_R']
    for sgn in (1.0, -1.0):
        s_arc = (sgn * TH - 0.53) * FACE_R
        for ang in (-0.6, -0.2, 0.2, 0.55):
            along = math.cos(ang) * s_arc + math.sin(ang) * z
            across = -math.sin(ang) * s_arc + math.cos(ang) * z
            crow = crow + gauss(across, 0.0016) * smoothstep(0.003, 0.009, along) * smoothstep(0.034, 0.016, along)
    R = R - (0.0007 + 0.0007 * F['squint']) * crow
    line = (0.026 + 0.004 * F['worry'] * gauss(at - 0.12, 0.11) - 0.011 * ((at - 0.4) / 0.32) ** 2
            - 0.003 * F['droop'] * smoothstep(0.45, 0.75, at))
    brows = gauss(z - line, 0.0055) * smoothstep(0.07, 0.14, at) * smoothstep(0.8, 0.68, at)
    R = R + F['brow_hair'] * brows * (1.0 + 0.12 * np.sin(TH * 34.0 + z * 160.0))
    comb2 = np.sin(TH * 24.0 + 1.6 * np.sin(z * 60.0))
    if hair == 'thin':
        # Bald: a fringe from the temples round the back, over the ears, its top edge low and ragged.
        low = -0.05 + 0.02 * smoothstep(2.4, 1.3, at) + 0.005 * np.sin(TH * 17.0)
        high = 0.034 + 0.01 * smoothstep(1.4, 2.6, at) + 0.004 * np.sin(TH * 13.0 + 1.0)
        hm = smoothstep(1.15, 1.35, at) * smoothstep(low - 0.006, low + 0.008, z) * smoothstep(high + 0.006, high - 0.006, z)
        R = R + hm * 0.0045 * (1.0 + 0.15 * comb2)
    else:
        # Cut short under the cap: over the ears and the collar, a little longer at the nape.
        low = (-0.035 + 0.025 * smoothstep(2.4, 1.3, at) - 0.016 * smoothstep(1.7, 2.7, at)
               + 0.004 * np.sin(TH * 23.0) * np.sin(TH * 9.0 + 1.0))
        front = 0.95 + 0.3 * smoothstep(0.045, -0.02, z)
        hm = smoothstep(front - 0.16, front + 0.08, at) * smoothstep(low - 0.006, low + 0.008, z)
        R = R + hm * 0.0065 * (1.0 + 0.12 * comb2)
    chops = chops_mask(TH, z, zm) if beard == 'chops' else np.zeros_like(TH)
    if beard == 'chops':
        tuft = 1.0 + 0.18 * np.sin(TH * 70.0 + z * 260.0) * np.sin(TH * 31.0 - z * 120.0)
        R = R + chops * (0.011 + 0.005 * smoothstep(-0.05, -0.1, z)) * tuft
    P = d * (r + R)[..., None]
    P[..., 2] += 0.0022 * F['smile'] * smoothstep(0.12, 0.34, at) * gauss(z - zm, 0.014) * smoothstep(1.0, 0.5, at)
    zz = P[..., 2]
    cav = eye_cav
    cav = np.maximum(cav, 0.42 * smoothstep(0.15, 0.6, brows))
    cav = np.maximum(cav, 0.7 * (gauss(TH - 0.06, 0.02) + gauss(TH + 0.06, 0.02)) * gauss(z + 0.0485, 0.0042))
    cav = np.maximum(cav, 0.6 * gauss(TH, 0.27) * gauss(z - zm, 0.0024) * (1.0 - smoothstep(0.27, 0.36, at)))
    cav = np.maximum(cav, 0.18 * gauss(TH, 0.2) * gauss(z - zm + 0.0165, 0.003))
    cav = np.maximum(cav, 0.24 * (gauss(TH - 0.33, 0.028) + gauss(TH + 0.33, 0.028)) * gauss(z - zm - 0.001, 0.0035) * F['smile'])
    nl = 0.21 + 0.22 * np.clip((-0.045 - z) / 0.04, 0.0, 1.0)
    window = smoothstep(-0.041, -0.049, z) * smoothstep(-0.088, -0.074, z)
    cav = np.maximum(cav, 0.26 * gauss(at - nl, 0.028) * window)
    cav = np.maximum(cav, 0.22 * np.clip(crow, 0.0, 1.0))
    cav = np.maximum(cav, 0.13 * F['age'] * np.clip(creases, 0.0, 1.0))
    # A few days' growth where no beard is (Amos's).
    lb = A['beard_line'](at)
    lip_band = gauss(TH, 0.36) * smoothstep(zm + 0.0095, zm + 0.004, z) * smoothstep(zm - 0.012, zm - 0.005, z)
    grow = (smoothstep(lb + 0.008, lb - 0.01, z) * smoothstep(1.7, 1.5, at)
            + gauss(TH, 0.38) * smoothstep(zm + 0.026, zm + 0.016, z) * smoothstep(zm + 0.003, zm + 0.0095, z))
    speck = 0.5 + 0.5 * np.sin(TH * 210.0 + z * 530.0) * np.sin(TH * 97.0 - z * 830.0)
    cav = np.maximum(cav, np.clip(grow, 0.0, 1.0) * (1.0 - lip_band) * (0.11 + 0.08 * speck) * (1.0 - chops))
    tufts = 0.3 * np.sin(TH * 95.0 + z * 310.0) * np.sin(TH * 41.0 - z * 170.0)
    hairy = hm + tufts * smoothstep(0.05, 0.45, hm) * smoothstep(0.98, 0.6, hm)
    cav = np.where(hairy > 0.5, 0.1 + 0.05 * (1.0 - comb2), cav)
    combed = 0.12 + 0.1 * (0.5 + 0.5 * np.sin(TH * 60.0 + z * 400.0))
    cav = np.where(chops > 0.5, combed, cav)
    zone = np.where((hairy > 0.5) | (chops > 0.5), hair_zone, 0).astype(float)
    face = np.maximum(smoothstep(1.35, 0.95, at), 0.4 * smoothstep(1.9, 1.5, at)) * smoothstep(-0.16, -0.12, z) * smoothstep(0.085, 0.06, z)
    keep = face * (0.4 + 0.6 * np.clip(1.6 * (gauss(TH - A['ET'], 0.22) + gauss(TH + A['ET'], 0.22)) * gauss(z, 0.025)
                                       + gauss(TH, 0.5) * gauss(z - zm, 0.022) + gauss(TH, 0.12) * gauss(z + 0.03, 0.03), 0.0, 1.0))
    MOUTH_HALF = A['MOUTH_HALF']
    centre_col = int(np.argmin(np.abs(th)))
    i_m = int(np.argmin(np.abs(zz[:, centre_col] - zm)))
    slit = np.where(np.abs(th) < MOUTH_HALF)[0]
    j0, j1 = slit.min(), slit.max()
    inner = np.arange(j0 + 1, j1)
    inside = smoothstep(MOUTH_HALF + 0.16, MOUTH_HALF, at)
    sharp = (np.arange(rows)[:, None] > i_m).astype(float) * np.ones_like(at)
    soft = smoothstep(zm + 0.004, zm - 0.03, zz) * smoothstep(2.3, 1.5, at)
    jw = inside * sharp + (1.0 - inside) * soft
    band = (np.abs(np.arange(rows)[:, None] - i_m) <= 2) & (at <= MOUTH_HALF + 0.06)
    keep = np.where(band, 1.0, keep)
    V = P.reshape(-1, 3)
    n = rows * cols
    dup = n + np.arange(len(inner))
    V = np.vstack([V, P[i_m, inner], P[0].mean(0) + (0.0, 0.0, 0.002), P[-1].mean(0) - (0.0, 0.0, 0.002)])
    jw_v = np.concatenate([jw.ravel(), np.ones(len(inner)), [0.0, 1.0]])
    jw_v[i_m * cols + inner] = 0.0
    cav_v = np.concatenate([cav.ravel(), np.full(len(inner), 0.6), [0.0, 0.0]])
    cav_v[i_m * cols + inner] = 0.6
    keep_v = np.concatenate([keep.ravel(), np.ones(len(inner)), [0.0, 0.0]])
    zone_v = np.concatenate([zone.ravel(), np.zeros(len(inner)), [0.0, 0.0]])
    Fg = lg.grid_faces(rows, cols)
    lower = (Fg[:, 0] // cols == i_m)
    swap = {int(i_m * cols + j): int(k) for j, k in zip(inner, dup)}
    for f in np.where(lower)[0]:
        Fg[f] = [swap.get(int(v), int(v)) if v // cols == i_m else v for v in Fg[f]]
    top, bottom = n + len(inner), n + len(inner) + 1
    piece = lg.orient((V, [Fg, lg.fan(top, np.arange(cols)), lg.fan(bottom, np.arange(cols) + n - cols)]))
    return piece, cav_v, jw_v, keep_v, zone_v


def build_head(A, pivot, R, neck_base, triangles, beard='stubble', hair='short', hair_zone=0, seat=0.153):
    """The head as it rests: built in head space (Amos.py's build_head: the face, the ears, the mouth and teeth, the jaw
    closed), turned by R about the top of the neck (pivot) and put there; the neck from neck_base (two points up out of
    the collar, rig space) into it. Returns the parts and the head's frame: pivot, center (head space's origin), R, the
    hat's seat (seat over the pivot along the head's up), the hinge."""
    parts = []
    hs_center = np.array([0.0, -0.012, 0.09])
    center = pivot + R @ hs_center

    def place(V):
        return (np.asarray(V, float)) @ R.T + center
    (V, F), cav, jw, keep, zone = head_surface(A, beard, hair, hair_zone)
    opened = A['jaw_turn'](V, jw, A['JAW_BUILD'])
    low = lg.reduce((opened, F), triangles, keep=keep * 0.35, keep_factor=3.0)      # a fuller keep starves the skull
    near = lg.nearest(opened, low[0])
    jw_low = jw[near]
    Vr = A['jaw_turn'](low[0], jw_low, A['JAW_REST'] - A['JAW_BUILD'])
    head = lg.Part('Head', (place(Vr), low[1]), zone[near], cavity=cav[near])
    parts.append(head.weigh('head', 1.0 - jw_low).weigh('jaw', jw_low))
    for s in SIDES:
        (Ve, Fe), c = A['ear_piece'](s)
        parts.append(lg.Part('Ear', (place(Ve), Fe), 0, cavity=c).weigh('head', 1.0))
    zm = A['MOUTH_Z']
    sock = lg.ellipsoid((0.0, -0.06, zm - 0.006), (0.024, 0.026, 0.017), segs=12, rings=7)
    sw = smoothstep(zm + 0.004, zm - 0.01, sock[0][:, 2])
    parts.append(lg.Part('Mouth', (place(A['jaw_turn'](sock[0], sw, A['JAW_REST'])), sock[1]), 0, cavity=1.0)
                 .weigh('head', 1.0 - sw).weigh('jaw', sw))
    for upper in (True, False):
        a = np.linspace(-0.55, 0.55, 9)
        zt = (zm + 0.0045, zm - 0.0005) if upper else (zm - 0.0015, zm - 0.0065)
        rr = 0.03 if upper else 0.028
        pts = np.array([[[rr * math.sin(x), -0.072 - 0.012 * math.cos(x) - (0.0015 if upper else 0.0), zt_] for x in a]
                        for zt_ in zt])
        Vt = pts.reshape(-1, 3)
        Vt = Vt if upper else A['jaw_turn'](Vt, np.ones(len(Vt)), A['JAW_REST'])
        parts.append(lg.Part('Teeth', (place(Vt), [lg.grid_faces(2, len(a), closed=False)]), 0, cavity=0.2)
                     .weigh('head' if upper else 'jaw', 1.0))
    # The neck: up out of the collar (neck_base), into the head from under it.
    C = lg.spline([neck_base[0], neck_base[1], center + R @ np.array([0.0, 0.018, -0.07])], 10)

    def rfn(s, a):
        cords = 0.07 * (gauss(wrap(a - 0.95), 0.25) + gauss(wrap(a + 0.95), 0.25)) * smoothstep(0.25, 0.55, s)
        apple = 0.09 * gauss(wrap(a), 0.22) * gauss(s - 0.55, 0.12)
        return np.interp(s, [0.0, 0.45, 1.0], [0.06, 0.049, 0.045]) * (1.0 + cords + apple)
    neck = lg.tube_fn(C, rfn, segs=16, up=R @ np.array([0.0, -1.0, 0.0]))
    s_neck = lg.polyline_distance(neck[0], C)[1]
    part = lg.Part('Neck', neck, 0, cavity=0.05)
    part.weigh('spine_02', 0.6 * smoothstep(0.25, 0.0, s_neck))
    part.weigh('head', smoothstep(0.6, 1.0, s_neck))
    part.weigh('neck', 1.0 - 0.6 * smoothstep(0.25, 0.0, s_neck) - smoothstep(0.6, 1.0, s_neck))
    parts.append(part)
    frame = dict(pivot=np.asarray(pivot, float), center=center, R=R, hat=pivot + R @ np.array([0.0, 0.0, seat]),
                 hinge=center + R @ A['HINGE'])
    return parts, frame


def head_bones(head, neck_base):
    """The neck, head, jaw and hat bones on the head's frame."""
    R, P, C = head['R'], head['pivot'], head['center']
    return [('neck', neck_base, P, 'spine_02'),
            ('head', P, P + R @ np.array([0.0, 0.0, 0.17]), 'neck'),
            ('jaw', head['hinge'], C + R @ np.array([0.0, -0.08, -0.11]), 'head'),
            ('hat', head['hat'], head['hat'] + R @ np.array([0.0, 0.0, 0.1]), 'head')]


# --- Arms ---

def rest_arm(S, W, f, u, pole, lengths=(0.29, 0.265)):
    """An arm's rest: the shoulder S, the wrist W (pulled in if out of reach), the elbow toward pole; the hand's frame."""
    E, W, reached = lg.two_bone(S, W, lengths[0], lengths[1], pole)
    f = unit(f)
    u = unit(np.asarray(u, float) - f * np.dot(u, f))
    return dict(S=np.asarray(S, float), E=E, W=W, f=f, u=u)


def build_arm(A, s, arm, curls, spread=0.06, thumb=0.35, sleeve='coat', sleeve_zone=1, radii=(0.072, 0.064, 0.056),
              sleeve_tris=360, hand_tris=900, hand_scale=1.1, seed=41, garter=None):
    """A sleeve (a coat's to the wrist with a turned cuff, or a shirt's rolled past the elbow over a bare forearm) and the
    forearm and big hand, fused and reduced; the fingers on their own bones. Returns the parts and the fingers' lines."""
    sfx = SUFFIX[s]
    S, E, W, f, u = arm['S'], arm['E'], arm['W'], arm['f'], arm['u']
    parts = []
    dv = unit(W - E)
    if sleeve == 'coat':
        piece, line, at_elbow = lg.sleeve_piece(S + np.array([-s * 0.006, 0.0, 0.01]), E, W, radii, seed, folds=0.07)
        low = lg.reduce(piece, sleeve_tris)
        parts.append(lg.weigh_sleeve(lg.Part(f'Sleeve_{sfx}', low, sleeve_zone), low[0], line, at_elbow, sfx))
        cuff = lg.torus(W - dv * 0.048, dv, radii[2], 0.013, segs=16, sides=6, squash=1.4)
        parts.append(lg.Part(f'Cuff_{sfx}', cuff, sleeve_zone, cavity=0.15).weigh(f'lowerarm_{sfx}', 1.0))
        start = E + dv * 0.05
        limb = [lg.tube([start, (start + W) * 0.5, W - dv * 0.01], [0.034, 0.03, 0.026], [0.037, 0.034, 0.031], segs=16,
                        up=u)]
    else:
        piece, line = A['rolled_sleeve'](S, E, 0.063, 0.056, seed)
        low = lg.reduce(piece, sleeve_tris)
        part = lg.Part(f'Sleeve_{sfx}', low, sleeve_zone)
        d, along = lg.polyline_distance(low[0], line)
        top = smoothstep(0.25, 0.0, along)
        parts.append(part.weigh('spine_02', 0.45 * top).weigh(f'upperarm_{sfx}', 1.0 - 0.45 * top))
        if garter is not None:
            c = S + (E - S) * 0.55
            ring = lg.torus(c, E - S, 0.064, 0.0065, segs=16, sides=5, squash=1.3)
            parts.append(lg.Part(f'Garter_{sfx}', ring, garter, cavity=0.2).weigh(f'upperarm_{sfx}', 1.0))
        start = E - unit(E - S) * 0.012
        L = np.linalg.norm(W - start)
        C = [start, start + dv * L * 0.3, start + dv * L * 0.65, W - dv * 0.01]
        limb = [lg.tube(C, [0.039, 0.041, 0.033, 0.026], [0.041, 0.043, 0.036, 0.031], segs=18, up=u)]
    hand, lines = lg.hand_pieces(W, f, u, s, curls, spread, thumb=thumb, length=1.02, thin=1.12, claw=0.0, knuckle=1.2,
                                 scale=hand_scale)
    low = lg.reduce(lg.union(limb + hand, 0.0024, smooth=3), hand_tris)
    part = lg.Part(f'Hand_{sfx}', low, 0)
    parts.append(lg.weigh_hand(part, low[0], E, W, lines, sfx, elbow_from=S))
    return parts, lines


def arm_bones(arms, fingers):
    specs = []
    for s in SIDES:
        sfx = SUFFIX[s]
        a = arms[s]
        specs.append((f'upperarm_{sfx}', a['S'], a['E'], 'spine_02'))
        specs.append((f'lowerarm_{sfx}', a['E'], a['W'], f'upperarm_{sfx}'))
        specs.append((f'hand_{sfx}', a['W'], a['W'] + a['f'] * 0.08, f'lowerarm_{sfx}'))
        for name, pts, radius in fingers[s]:
            specs.append((lg.finger_bone(name, sfx), pts[0], pts[1], f'hand_{sfx}'))
    return specs


def tail_bones(tail, stops=(0.0, 0.2, 0.4, 0.6, 0.8, 1.0), first_parent='pelvis'):
    specs = []
    C = tail['path']
    prev = first_parent
    for k in range(5):
        a, b = lg.sample(C, stops[k]), lg.sample(C, stops[k + 1])
        specs.append((f'tail_{k + 1:02d}', a, b, prev))
        prev = f'tail_{k + 1:02d}'
    for s in SIDES:
        sfx = SUFFIX[s]
        p = tail['sides'][s]
        specs.append((f'tail_{sfx}_01', p[0], p[1], 'tail_02'))
        specs.append((f'tail_{sfx}_02', p[1], p[2], f'tail_{sfx}_01'))
    return specs


def make_rig(specs):
    rig = lm.armature()
    lm.bones(rig, [(n, tuple(map(float, h)), tuple(map(float, t)), p) for n, h, t, p in specs])
    return rig


def speaker(obj, R, mouth):
    """SOCKET_Speaker on a prop riding the head: at the mouth, facing the head's front."""
    return lg.socket(obj, 'Speaker', mouth, R)
