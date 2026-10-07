"""Three concepts for Abel Ransom, the Keeper (Docs/Story.md; Docs/Areas/RansomsRest.md, "The boss" and build step 22):
Ellis's Pa, a farmer and the keeper of Saint Ada's lantern, shot by the gang on the path below the lookout. A week dead,
he walks the burial boards at dusk, because a keeper doesn't lie still while his saint is dark. He is the area's boss
and then a friend, so his face carries grief and dignity, never a monster's hunger. Concept art kept in Art/Backlog
(see its README): nothing in the game uses it.

  A  The Sunday keeper: the black frock coat he wore to walk the dead to the boards, buttoned to the throat, a keeper's
     stole with the sun-ring at its ends, a flat-crowned wide hat; a trimmed beard. Upright, formal, mourning.
  B  The farmer-keeper: the canvas duster and shoulder cape he ran up the bluff path in, open over his work shirt, a
     black crepe band on his arm and the brass sun-ring on his lapel; a creased cattleman's hat, a heavy moustache.
  C  The storm keeper: a caped greatcoat whose skirts have become the Gravewind itself, streaming off him in long torn
     tails; the scarf and the hair stream too, the hat's brim blown up off his face; a long beard.

Every option is Abel as the doc fixes him: the Unpaid rig at 1.3 times (built here at the rig's own size and scaled
1.3), his ghost lantern (the Keeper's Lantern of Art/Models/Props/BurialDeck.py, gone to spirit, its flame a pale ghost
light) in his left hand, held up before the coal while he fights, and the spectral twin of his Ranchhand pump
(Heirloom: a classic walnut-stocked pump in Art/Models/Weapons/Ranchhand.py's proportions) in his right. His coal is
the boss's orange (CreatureRankSettings' Boss tag, #FFCC00). He floats: no legs. Drawn as the game would draw him: the
masked, dithered ghost material of the Unpaid concepts (UnpaidConcepts.py, whose stage and toolkit this script borrows),
a fresnel rim, the coal and the lantern emissive.

Renders go to Saved/ArtPreviews/RansomsRest/Concepts/Abel/: per option the hero at dusk on the burial deck's open
edge, backlit by the setting sun (Abel_A_dusk.png), the same in the golden afternoon (_day), a turnaround beside a
1.8 m post and an Unpaid (_turnaround), a face close-up (_face) and the kneeling grief with the coal open (_kneel); and
Abel_compare.png, the three side by side in the same light and at the same scale.

    powershell -NoProfile -File Tools\\artrun.ps1 -Script Art\\Backlog\\Creatures\\AbelConcepts.py -ScriptArgs A,dusk
    ... -ScriptArgs A,B,C,dusk,day,turn,face,kneel,compare      (no shot named: all of them)
    ... -ScriptArgs A,debug                                      quick checks into Intermediate/AbelConcepts
"""
import math
import os
import sys
import time

import bpy
import numpy as np
from mathutils import Matrix, Vector

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import UnpaidConcepts as uc  # noqa: E402  (its stage, toolkit and ghost material; importing it renders nothing)
import looter_textures as lt  # noqa: E402

OUT = os.path.join(lt.REPO, 'Saved', 'ArtPreviews', 'RansomsRest', 'Concepts', 'Abel')
WORK = os.path.join(lt.REPO, 'Intermediate', 'AbelConcepts')
ARGV = [a for arg in (sys.argv[sys.argv.index('--') + 1:] if '--' in sys.argv else []) for a in arg.split(',') if a]
OPTIONS = ('A', 'B', 'C')
SHOTS = ('dusk', 'day', 'turn', 'face', 'kneel', 'compare')
TITLES = {'A': 'A  The Sunday keeper', 'B': 'B  The farmer-keeper', 'C': 'C  The storm keeper'}
SCALE = 1.3                                 # the doc: the Unpaid rig at 1.3 times
BOSS_COAL = (0xffcc00, 3.4)                 # the Boss rank's tag color, as the coal burns it
EMBER = (0xffcc00, 1.6)                     # the coal sunk to an ember (the scene at zero)
FLAME = 0xfff0d6                            # the ghost light: a pale, warm white, no rarity color
# Zone colors (skin and shroud, shirt, coat, accent): pale and faded, they are ghosts.
TINTS = {'A': (0xbac4c6, 0xdcd8cc, 0x3c3b40, 0xd3c9ae),      # white shirt, black frock coat, bone stole
         'B': (0xbac4c6, 0xb9b29e, 0x86725a, 0x34302f),      # oatmeal work shirt, canvas duster, black crepe
         'C': (0xc2c9c8, 0x8a8f8b, 0x525957, 0x7e4c45)}      # grey waistcoat, slate greatcoat, oxblood scarf
GUN_TINT = (0xb4bcc0, 0xa08a72, 0x8f979b, 0xcfc4ad)          # steel, walnut, dark steel, bead
SKIN, SHIRT, COAT, ACCENT = 0, 1, 2, 3
SIDES = (1.0, -1.0)
smoothstep, unit, rotation, wrap, spline = uc.smoothstep, uc.unit, uc.rotation, uc.wrap, uc.spline


def log(message):
    print(f'ABEL: {message}', flush=True)


def g(x, s):
    return np.exp(-(np.asarray(x, float) / s) ** 2)


# --- The body under the clothes: a farmer of about 1.8 m in the rig's frame, upright, broad in the shoulder ---

TORSO = [(0.90, 0.165, 0.110, 0.000, 2.2), (0.98, 0.160, 0.106, 0.000, 2.2), (1.06, 0.155, 0.104, -0.004, 2.2),
         (1.16, 0.163, 0.110, -0.008, 2.3), (1.26, 0.178, 0.120, -0.010, 2.4), (1.34, 0.190, 0.124, -0.010, 2.6),
         (1.40, 0.198, 0.118, -0.006, 2.8), (1.45, 0.196, 0.106, -0.002, 3.2), (1.475, 0.184, 0.098, 0.000, 3.4),
         (1.50, 0.155, 0.090, 0.000, 3.0), (1.52, 0.115, 0.080, -0.002, 2.6), (1.54, 0.078, 0.070, -0.004, 2.2)]
SHOULDER = {s: np.array([s * 0.175, -0.005, 1.445]) for s in SIDES}
PIVOT = np.array([0.0, -0.03, 1.62])        # the top of the neck, where the head turns
COAL_AT = (0.24, 1.33)                      # the coal: angle round the chest from the front, height


def surface(profile, z, theta, grow=0.0):
    return uc.torso_points(profile, z, theta, grow)


def outward(theta):
    return np.stack([np.sin(theta), -np.cos(theta), np.zeros_like(theta)], -1)


# --- The head: grief and dignity, a face that reads when he speaks ---

HEAD = dict(width=0.15, front=0.097, back=0.108, top=0.12, bottom=0.14, mouth=-0.075, brow=0.013, brow_shelf=0.006,
            socket=0.019, eyeball=0.0072, nose=0.04, cheek=0.006, zyg=0.005, hollow=0.007, nasolabial=0.0028,
            lips=0.0034, lips_thin=0.0012, chin=0.008, jaw_angle=0.006, nape=0.22, jaw_taper=0.05, socket_dark=0.9,
            mouth_depth=0.03, temple=0.006, asym=0.06)


def beard_line(at):
    """Where a full beard starts on the face: just under the cheekbones, up the jaw to the sideburns."""
    return -0.036 - 0.012 * g(at - 0.8, 0.3) + 0.062 * smoothstep(1.15, 1.5, at)


def head_shape(spec, jaw_open, grief=1.0, stubble=0.0, rows=144, cols=144):
    """UnpaidConcepts.head_shape's radial head, made a living man's grieving face: eyes in their sockets under heavy
    lids, looking a little down; brows drawn up and together at their inner ends, the forehead creased over them; a
    long nose with its wings; the corners of the mouth pulled down. Returns the piece, cavity and the grid (P, d, TH, z,
    mouth) the beard and hair grow from."""
    phi = np.linspace(0.0, np.pi, rows + 2)[1:-1]
    th = np.linspace(-np.pi, np.pi, cols, endpoint=False)
    PH, TH = np.meshgrid(phi, th, indexing='ij')
    d = np.stack([np.sin(PH) * np.sin(TH), -np.sin(PH) * np.cos(TH), np.cos(PH)], -1)
    a = spec['width'] / 2.0
    b = np.where(d[..., 1] < 0, spec['front'], spec['back'])
    c = np.where(d[..., 2] > 0, spec['top'], spec['bottom'])
    r = 1.0 / np.sqrt((d[..., 0] / a) ** 2 + (d[..., 1] / b) ** 2 + (d[..., 2] / c) ** 2)
    z = d[..., 2] * r
    at = np.abs(TH)
    zm0 = spec['mouth']
    zm = zm0 - 0.006 * grief * smoothstep(0.1, 0.36, at)            # the corners pulled down
    r = r * (1.0 - spec['nape'] * smoothstep(-0.03, -0.11, z) * smoothstep(1.9, 2.8, at))
    r = r * (1.0 - spec['jaw_taper'] * smoothstep(zm0 + 0.01, -spec['bottom'], z) * smoothstep(2.2, 1.2, at)
             * np.abs(np.sin(TH)))
    nose = (spec['nose'] * smoothstep(0.018, -0.04, z) * smoothstep(-0.062, -0.044, z)
            * g(TH, 0.07 + 0.08 * smoothstep(-0.01, -0.048, z)))
    wings = 0.0055 * (g(TH - 0.1, 0.045) + g(TH + 0.1, 0.045)) * g(z + 0.046, 0.0065)
    lop = 1.0 + spec['asym'] * (TH < 0)
    brow_z = 0.022 + 0.008 * grief * g(TH, 0.32)
    r = r + (spec['brow'] * g(TH, 0.62) * g(z - brow_z, 0.011)
             + 0.0025 * grief * g(TH, 0.12) * g(z - 0.03, 0.009)        # knitted between the brows
             - 0.0012 * grief * g(TH, 0.3) * sum(g(z - zk, 0.0026) for zk in (0.05, 0.062, 0.074))   # creases
             + nose + wings + 0.004 * g(at - 0.17, 0.06) * g(z + 0.043, 0.008)
             + spec['cheek'] * g(at - 0.68, 0.22) * g(z + 0.02, 0.015)
             - spec['hollow'] * g(at - 0.62, 0.25) * g(z + 0.055, 0.022)
             + spec['lips'] * g(TH, 0.42) * g(z - zm, 0.022)
             - 0.0035 * g(TH, 0.3) * g(z - zm, 0.003)
             + spec['chin'] * g(TH, 0.32) * g(z + spec['bottom'] - 0.016, 0.013)
             - spec['temple'] * g(at - 1.05, 0.22) * g(z - 0.04, 0.025))
    # The eyes: sockets under the brow, the eyeballs set in them, a fold of upper lid and a lower lid.
    eye = lambda w, zc, s: (g(TH - 0.36, w) + g(TH + 0.36, w)) * g(z - zc, s)       # noqa: E731
    r = r + (-spec['socket'] * lop * (g(TH - 0.36, 0.16) + g(TH + 0.36, 0.16)) * g(z - 0.002, 0.016)
             + spec['eyeball'] * lop * eye(0.072, -0.001, 0.0085)
             + 0.0027 * eye(0.09, 0.0064, 0.0026) + 0.0016 * eye(0.085, -0.0082, 0.0024)
             + spec['brow_shelf'] * eye(0.2, 0.017, 0.006))
    # The brows: hair on the ridge, their inner ends raised.
    line = 0.027 + 0.009 * grief * g(at - 0.13, 0.11) - 0.011 * ((at - 0.36) / 0.3) ** 2
    brows = g(z - line, 0.0045) * smoothstep(0.09, 0.16, at) * smoothstep(0.67, 0.56, at)
    r = r + 0.003 * brows
    nl = 0.2 + 0.22 * np.clip((-0.045 - z) / 0.035, 0.0, 1.0)
    r = r + (spec['zyg'] * g(z + 0.018, 0.009) * smoothstep(0.55, 0.75, at) * smoothstep(1.55, 1.3, at)
             + spec['jaw_angle'] * g(at - 1.22, 0.2) * g(z + 0.085, 0.02)
             - spec['nasolabial'] * g(at - nl, 0.045) * smoothstep(-0.04, -0.05, z) * smoothstep(-0.088, -0.078, z)
             + spec['lips_thin'] * g(TH, 0.3) * (g(z - zm - 0.007, 0.0035) + g(z - zm + 0.008, 0.0035)))
    P = d * r[..., None]
    # Cavity: the sockets' shadow round the lighter eyeballs, the irises looking a little down, the brows, the
    # nostrils, the mouth line, the creases and the folds.
    sock = np.clip(1.8 * (g(TH - 0.36, 0.14) + g(TH + 0.36, 0.14)) * g(z - 0.002, 0.015), 0.0, 1.0) * spec['socket_dark']
    ball = np.clip(1.6 * eye(0.055, -0.001, 0.0065), 0.0, 1.0)
    cav = sock * (1.0 - 0.45 * ball)
    cav = np.maximum(cav, 0.75 * np.clip((g(TH - 0.352, 0.026) + g(TH + 0.352, 0.026)) * g(z + 0.0028, 0.0042), 0, 1))
    cav = np.maximum(cav, 0.6 * brows * (0.8 + 0.2 * np.sin(TH * 150.0 + z * 420.0)))
    cav = np.maximum(cav, 0.6 * (g(TH - 0.055, 0.024) + g(TH + 0.055, 0.024)) * g(z + 0.052, 0.0042))
    cav = np.maximum(cav, 0.5 * g(TH, 0.26) * g(z - zm, 0.0035))
    cav = np.maximum(cav, 0.3 * g(at - nl, 0.03) * smoothstep(-0.04, -0.05, z) * smoothstep(-0.088, -0.078, z))
    cav = np.maximum(cav, 0.22 * grief * g(TH, 0.3) * sum(g(z - zk, 0.002) for zk in (0.05, 0.062, 0.074)))
    if stubble:
        line_b = beard_line(at)
        cav = np.maximum(cav, stubble * smoothstep(line_b + 0.01, line_b - 0.012, z) * smoothstep(1.7, 1.55, at)
                         * (1.0 - g(TH, 0.36) * smoothstep(zm + 0.011, zm + 0.004, z) * smoothstep(zm - 0.012, zm - 0.004, z)))
    if jaw_open > 1e-3:
        w = smoothstep(zm0 + 0.003, zm0 - 0.012, P[..., 2]) * smoothstep(2.4, 1.6, at)
        hinge = np.array([0.0, 0.006, -0.03])
        ang = jaw_open * w
        y, zz = P[..., 1] - hinge[1], P[..., 2] - hinge[2]
        P = P.copy()
        P[..., 1] = hinge[1] + y * np.cos(ang) - zz * np.sin(ang)
        P[..., 2] = hinge[2] + y * np.sin(ang) + zz * np.cos(ang)
        band = np.clip(4.0 * w * (1.0 - w), 0.0, 1.0) * smoothstep(0.62, 0.26, at)
        P = P - d * (band * spec['mouth_depth'] * min(jaw_open / 0.4, 1.0))[..., None]
        cav = np.maximum(cav, smoothstep(0.15, 0.6, band) * 0.97)
    rings = P.shape[0]
    V = np.vstack([P.reshape(-1, 3), P[0].mean(0) + (0.0, 0.0, 0.002), P[-1].mean(0) - (0.0, 0.0, 0.002)])
    n = rings * cols
    faces = [uc.grid_faces(rings, cols), uc.fan(n, np.arange(cols)), uc.fan(n + 1, np.arange(cols) + n - cols)]
    return uc.orient((V, faces)), np.concatenate([cav.ravel(), [0.0, 0.0]]), (P, d, TH, z, zm0)


def grow_sheet(grid, mask, thick, strands=95.0, cut=0.04, seed=3, comb=0.22):
    """A sheet grown out of the face along its radii where mask is up (beard, moustache, hair), thick as asked, with
    strands combed down it. Returns ((V, faces), cavity)."""
    P, d, TH, z, zm = grid
    rng = np.random.default_rng(seed)
    comb = 1.0 + comb * np.sin(TH * strands + 3.0 * np.sin(z * 120.0)) * smoothstep(0.2, 0.6, mask)
    jitter = 1.0 + 0.1 * rng.uniform(-1.0, 1.0, TH.shape[1])[None, :]
    B = P + d * (thick * comb * jitter * smoothstep(cut, 0.45, mask))[..., None]
    V, F, used = uc.cut(B, cut - mask)
    return (V, F), 0.28 + 0.25 * (1.0 - comb.ravel()[used])


def beard_piece(grid, style):
    """The beard and moustache: the mouth stays clear (it reads when he talks), and so does the nose."""
    P, d, TH, z, zm = grid
    at = np.abs(TH)
    lips = g(TH, 0.36) * smoothstep(zm + 0.011, zm + 0.004, z) * smoothstep(zm - 0.012, zm - 0.004, z)
    nose = g(TH, 0.13) * smoothstep(-0.064, -0.05, z)
    if style in ('full', 'long'):
        line = beard_line(at)
        m = smoothstep(line + 0.012, line - 0.014, z) * smoothstep(1.7, 1.56, at)
        chin = smoothstep(zm - 0.01, zm - 0.09, z) if style == 'long' else smoothstep(zm - 0.02, zm - 0.07, z)
        t = (0.0035 + 0.0035 * smoothstep(line, line - 0.04, z)
             + (0.026 if style == 'long' else 0.01) * g(TH, 0.5 if style == 'long' else 0.45) * chin)
        tache = g(TH, 0.4) * smoothstep(zm + 0.026, zm + 0.016, z) * smoothstep(zm + 0.002, zm + 0.008, z)
        t = np.maximum(t, 0.0075 * tache)
        m = np.maximum(m, tache)
    elif style == 'walrus':
        top = zm + 0.027 - 0.006 * (at / 0.45) ** 2
        bot = zm - 0.002 - 0.018 * smoothstep(0.22, 0.46, at)
        m = smoothstep(top + 0.003, top - 0.006, z) * smoothstep(bot - 0.004, bot + 0.012, z) * smoothstep(0.53, 0.45, at)
        t = 0.0095 * (1.0 - 0.5 * smoothstep(0.26, 0.5, at))
        m = m * (1.0 - nose)
        return grow_sheet(grid, m, t, strands=70.0, comb=0.1)    # it hangs over the upper lip, past the corners
    else:
        return None
    m = m * (1.0 - lips) * (1.0 - nose)
    return grow_sheet(grid, m, t)


def hair_sheet(grid, style):
    """The hair under the hat: cropped close round the back and sides, down to the nape and the sideburns."""
    P, d, TH, z, zm = grid
    at = np.abs(TH)
    low = -0.04 + 0.03 * smoothstep(2.4, 1.3, at)
    front = 0.95 + 0.3 * smoothstep(0.045, -0.02, z)          # the hairline: the temples, then back to the sideburns
    m = smoothstep(front - 0.16, front + 0.08, at) * smoothstep(low - 0.012, low + 0.012, z) * smoothstep(0.095, 0.07, z)
    t = {'short': 0.005, 'medium': 0.008, 'long': 0.009}[style]
    return grow_sheet(grid, m, t, strands=120.0, seed=5)


def hair_locks(part, center, R, style, seed=17, wind=0.0):
    """Locks falling from under the hair's edge at the back and sides: middling or long, streaming back with wind."""
    if style == 'short':
        return
    rng = np.random.default_rng(seed)
    count, drops = {'medium': (20, (0.04, 0.09)), 'long': (26, (0.12, 0.24))}[style]
    for k in range(count):
        a = math.pi * rng.uniform(0.5, 1.0) * (1 if k % 2 else -1)
        low = -0.04 + 0.03 * smoothstep(2.4, 1.3, abs(a))
        root = np.array([0.068 * math.sin(a), -0.094 * math.cos(a) + 0.008, low + 0.012])
        out = np.array([math.sin(a), -math.cos(a), 0.0])
        drop = rng.uniform(*drops)
        side = np.array([math.cos(a), math.sin(a), 0.0]) * rng.uniform(-0.01, 0.01)
        back = np.array([0.0, 1.0, 0.3]) * wind * drop
        path = [root, root + out * 0.006 + (0.0, 0.0, -drop * 0.35) + back * 0.3,
                root + out * 0.01 + side * 0.5 + (0.0, 0.006, -drop * 0.7) + back * 0.7,
                root + out * 0.014 + side + (0.0, 0.012, -drop) + back * 1.2]
        path = [center + R @ p for p in path]
        w0 = rng.uniform(0.008, 0.016)
        uc.ribbon(part, path, [(0.0, w0), (0.6, w0 * 0.75), (1.0, 0.002)], n=20, cols=4,
                  out=lambda C, c0=center: unit(C - c0), curl=0.35, twist=0.25 * rng.uniform(-1, 1), seed=60 + k)
    for values in part.attr:
        values[:, 1] = 0.3


def head_parts(fig, R, jaw, beard, hair, wind=0.0):
    """The head on its neck (into the collar), its beard and hair; returns the head's middle."""
    center = PIVOT + R @ np.array([0.0, -0.012, 0.09])
    face = uc.Part('Head', SKIN)
    piece, cav, grid = head_shape(HEAD, jaw, stubble=0.22 if beard == 'walrus' else 0.0)
    face.add(uc.transform(piece, R, center), cavity=cav)
    for s in SIDES:
        ear = uc.ellipsoid((s * (HEAD['width'] / 2.0 - 0.003), 0.014, -0.006), (0.009, 0.018, 0.028), segs=16, rings=10)
        face.add(uc.transform(ear, R, center))
    face.add(uc.tube([(0.0, -0.012, 1.48), (0.0, -0.022, 1.55), center + R @ np.array([0.0, 0.018, -0.07])],
                     [0.05, 0.047, 0.046], segs=20))
    zm = HEAD['mouth']
    face.add(uc.transform(uc.ellipsoid((0.0, -0.035, zm - 0.012), (0.032, 0.04, 0.026), segs=20, rings=12), R, center),
             cavity=1.0)
    fig.add(face)
    (V, F), hcav = hair_sheet(grid, hair)
    fig.add(uc.Part('HairCap', SKIN).add((V @ R.T + center, F), cavity=hcav))
    grown = beard_piece(grid, beard)
    if grown is not None:
        (V, F), bcav = grown
        fig.add(uc.Part('Beard', SKIN).add((V @ R.T + center, F), cavity=bcav))
    if beard == 'long':
        # The beard's fall below the chin: a long spade of combed hair, its point blown back by the wind.
        path = [np.array(p) for p in ((0.0, -0.07, -0.11), (0.0, -0.09, -0.155), (0.0, -0.084 + 0.04 * wind, -0.2),
                                      (0.0, -0.064 + 0.08 * wind, -0.24 + 0.01 * wind))]
        C = spline([center + R @ p for p in path], 30)

        def rfn(s, a):
            return (0.028 * (1.0 - s) ** 0.9 + 0.003) * (1.0 + 0.2 * np.cos(17.0 * a + 5.0 * s))
        V, F = uc.tube_fn(C, rfn, segs=48, up=tuple(R @ np.array([0.0, -1.0, 0.0])))
        V = center + ((V - center) @ R) * np.array([1.0, 0.7, 1.0]) @ R.T      # flattened front to back
        fig.add(uc.Part('BeardLocks', SKIN).add((V, F), cavity=0.36))
    locks = uc.Part('Hair', SKIN)
    hair_locks(locks, center, R, hair, wind=wind)
    if locks.V:
        fig.add(locks, solidify=0.003)
    return center


# --- Hats (hat space: origin at the brim's middle, front -Y) ---

def hat_flat():
    """A keeper's Sunday hat: a low flat crown and a wide flat brim with its edge rolled up a little."""
    theta = np.linspace(0.0, 2.0 * np.pi, 96, endpoint=False)
    q = np.linspace(0.0, 1.0, 12)[:, None]
    r = 0.086 + 0.13 * q
    lift = 0.012 * smoothstep(0.75, 1.0, q) ** 2 + 0.003 * q * np.sin(2 * theta)
    brim = np.stack([r * np.sin(theta), -r * np.cos(theta) * 1.05, lift + 0.0 * theta], -1)
    zc = [0.0, 0.03, 0.06, 0.082, 0.09, 0.093]
    rc = [0.087, 0.086, 0.085, 0.083, 0.077, 0.065]
    crown = np.array([np.stack([rr * np.sin(theta), -rr * np.cos(theta) * 1.1, np.full_like(theta, zz)], -1)
                      for zz, rr in zip(zc, rc)])
    V = np.vstack([crown.reshape(-1, 3), [[0.0, 0.0, 0.094]]])
    felt = [(brim.reshape(-1, 3), [uc.grid_faces(12, 96)]),
            (V, [uc.grid_faces(len(zc), 96), uc.fan(len(zc) * 96, np.arange(96) + (len(zc) - 1) * 96, top=True)])]
    band = np.array([np.stack([0.088 * np.sin(theta), -0.088 * np.cos(theta) * 1.1, np.full_like(theta, z)], -1)
                     for z in (0.003, 0.014, 0.025)])
    return felt, (band.reshape(-1, 3), [uc.grid_faces(3, 96)])


def hat_cattleman():
    """A working hat: a tall crown pinched in front with a crease down the middle, the brim curled up at the sides and
    dipping front and back, battered."""
    theta = np.linspace(0.0, 2.0 * np.pi, 96, endpoint=False)
    q = np.linspace(0.0, 1.0, 12)[:, None]
    r = 0.086 + 0.112 * q * (1.0 + 0.03 * np.sin(3 * theta + 0.6))
    lift = 0.045 * q ** 1.8 * np.sin(theta) ** 2 - 0.012 * q ** 2 * np.cos(theta) ** 2 + 0.004 * q * np.sin(5 * theta)
    brim = np.stack([r * np.sin(theta), -r * np.cos(theta) * 1.08, lift], -1)
    zc = np.array([0.0, 0.03, 0.06, 0.09, 0.11, 0.12])
    rc = np.array([0.088, 0.087, 0.084, 0.078, 0.068, 0.052])
    pinch = 1.0 - 0.14 * np.exp(-((np.abs(wrap(theta)) - 0.5) / 0.28) ** 2)
    rings = []
    for z, rr in zip(zc, rc):
        k = 1.0 + (pinch - 1.0) * (z / 0.12)
        rings.append(np.stack([rr * k * np.sin(theta), -rr * k * np.cos(theta) * 1.12, np.full_like(theta, z)], -1))
    for s, z in ((0.6, 0.118), (0.28, 0.104)):
        x, y = 0.052 * s * np.sin(theta), -0.052 * s * np.cos(theta) * 1.12
        rings.append(np.stack([x, y, z - 0.022 * np.exp(-(x / 0.022) ** 2)], -1))
    P = np.array(rings)
    V = np.vstack([P.reshape(-1, 3), [[0.0, 0.0, 0.08]]])
    felt = [(brim.reshape(-1, 3), [uc.grid_faces(12, 96)]),
            (V, [uc.grid_faces(len(rings), 96), uc.fan(len(rings) * 96, np.arange(96) + (len(rings) - 1) * 96, top=True)])]
    band = np.array([np.stack([0.0895 * np.sin(theta), -0.0895 * np.cos(theta) * 1.12, np.full_like(theta, z)], -1)
                     for z in (0.003, 0.018, 0.032)])
    return felt, (band.reshape(-1, 3), [uc.grid_faces(3, 96)])


def hat_storm():
    """A wide slouch hat in the Gravewind: the front of its brim blown up off his face, the back pressed down."""
    theta = np.linspace(0.0, 2.0 * np.pi, 96, endpoint=False)
    q = np.linspace(0.0, 1.0, 14)[:, None]
    r = 0.086 + 0.135 * q * (1.0 + 0.04 * np.sin(4 * theta + 1.0))
    front = np.exp(-(wrap(theta) / 0.9) ** 2)
    lift = (0.075 * q ** 1.6 * front - 0.03 * q ** 1.7 * (1.0 - front) + 0.006 * q * np.sin(7 * theta + 0.5)
            + 0.004 * q ** 2 * np.sin(13 * theta))
    brim = np.stack([r * np.sin(theta), -r * np.cos(theta) * 1.06, lift], -1)
    zc = [0.0, 0.03, 0.06, 0.085, 0.1, 0.106]
    rc = [0.088, 0.087, 0.084, 0.078, 0.064, 0.045]
    rings = [np.stack([rr * np.sin(theta), -rr * np.cos(theta) * 1.08, np.full_like(theta, zz) - 0.008 * (zz / 0.106)
                       * np.cos(theta)], -1) for zz, rr in zip(zc, rc)]
    P = np.array(rings)
    V = np.vstack([P.reshape(-1, 3), [[0.0, 0.0, 0.104]]])
    felt = [(brim.reshape(-1, 3), [uc.grid_faces(14, 96)]),
            (V, [uc.grid_faces(len(rings), 96), uc.fan(len(rings) * 96, np.arange(96) + (len(rings) - 1) * 96, top=True)])]
    band = np.array([np.stack([0.0895 * np.sin(theta), -0.0895 * np.cos(theta) * 1.08, np.full_like(theta, z)], -1)
                     for z in (0.003, 0.012, 0.021)])
    return felt, (band.reshape(-1, 3), [uc.grid_faces(3, 96)])


HATS = {'A': hat_flat, 'B': hat_cattleman, 'C': hat_storm}


# --- The figure ---

class Abel(uc.Figure):
    """A concept in a pose: the Unpaid concepts' parts, plus his props (lantern and gun, as matrices in figure space)."""

    def __init__(self, key, pose):
        super().__init__(key, pose)
        self.props = []              # (kind, 4x4 matrix)
        self.ember = False


def pose_arms(pose):
    """Elbows, wrists and hand frames (direction along the fingers, back of the hand) and finger curls."""
    if pose == 'kneel':
        # The lantern set down on the boards at his side, his right hand over his eyes.
        E = {1.0: np.array([0.26, -0.13, 1.18]), -1.0: np.array([-0.2, -0.27, 1.26])}
        W = {1.0: np.array([0.29, -0.3, 0.96]), -1.0: np.array([-0.06, -0.26, 1.6])}
        fwd = {1.0: unit([0.02, -0.55, -0.85]), -1.0: unit([0.55, -0.25, 0.8])}
        up = {1.0: unit([0.95, -0.1, 0.2]), -1.0: unit([0.0, -1.0, 0.3])}
        curls = {1.0: (1.1, 1.15, 1.15, 1.1), -1.0: (0.25, 0.3, 0.32, 0.35)}
        # Lift or lower the left wrist until, leaned and sunk, its grip is at the standing lantern's grip height.
        for _ in range(6):
            z = kneeling(grip_point(W[1.0], fwd[1.0], up[1.0], 1.0)[0])[2]
            W[1.0] = W[1.0] + np.array([0.0, 0.0, 0.427 * 1.0 - z])
    else:
        # Fighting: the lantern arm drawn across his chest, guarding the coal, the lantern hanging from his fist before
        # him; the pump low in his right hand, ready.
        E = {1.0: np.array([0.27, -0.1, 1.27]), -1.0: np.array([-0.235, -0.04, 1.18])}
        W = {1.0: np.array([0.01, -0.235, 1.355]), -1.0: np.array([-0.25, -0.17, 0.975])}
        fwd = {1.0: unit([-0.85, -0.45, 0.12]), -1.0: unit([0.12, -0.5, -0.85])}
        up = {1.0: unit([0.05, -0.45, 0.9]), -1.0: unit([-0.95, -0.15, 0.2])}
        curls = {1.0: (1.1, 1.15, 1.15, 1.1), -1.0: (0.95, 1.0, 1.05, 1.1)}
    return E, W, fwd, up, curls


def arm_parts(fig, E, W, fwd, up, curls, sleeve_r=(0.062, 0.056, 0.048), cuff=True, wind=0.0, seed=41):
    """Coat sleeves from the shoulders to the wrists, cuffs, and the hands (big working hands)."""
    for k, s in enumerate(SIDES):
        S = SHOULDER[s]
        d = unit(E[s] - S)
        C = spline([S + np.array([-s * 0.012, 0.0, 0.01]), (S + E[s]) * 0.5 + np.cross(d, [0.0, 0.0, 1.0]) * 0.006,
                    E[s], (E[s] + W[s]) * 0.5, W[s] - unit(W[s] - E[s]) * 0.02], 40)
        rng = np.random.default_rng(seed + k)
        starts = rng.uniform(0.0, 2.0 * np.pi, 4)
        twists = rng.uniform(0.8, 1.8, 4) * rng.choice((-1.0, 1.0), 4)

        def rfn(t, a, starts=starts, twists=twists):
            base = np.interp(t, [0.0, 0.5, 1.0], sleeve_r)
            f = sum(0.09 * np.exp(-(wrap(a - p - tw * t) / 0.35) ** 2) for p, tw in zip(starts, twists))
            elbow = 0.07 * np.exp(-((t - 0.5) / 0.08) ** 2) * (0.6 + 0.4 * np.cos(3 * a))
            return base * (1.0 + f * smoothstep(0.05, 0.3, t) + elbow)
        sleeve = uc.tube_fn(C, rfn, segs=28)
        part = uc.Part(f'Sleeve{s:+.0f}', COAT).add(sleeve)
        if cuff:
            dv = unit(W[s] - E[s])
            part.add(uc.torus(W[s] - dv * 0.035, dv, sleeve_r[2] - 0.004, 0.012, squash=1.4))
        fig.add(part)
        dv = unit(W[s] - E[s])
        limb = [uc.tube([W[s] - dv * 0.09, W[s] - dv * 0.01], [0.03, 0.025], [0.034, 0.03], segs=18, up=up[s])]
        limb += uc.hand(W[s], fwd[s], up[s], s, curl=curls[s], spread=0.04, thumb=0.75 if curls[s][0] > 0.8 else 0.35,
                        length=1.05, thin=1.15, claw=0.0, knuckle=1.25, scale=1.14)
        fig.add(uc.Part(f'Hand{s:+.0f}', SKIN).add(uc.union(limb, 0.0024, smooth=3)))


def grip_point(W, fwd, up, side):
    """Where a fist's grip sits: inside the curled fingers."""
    f = unit(fwd)
    u = unit(np.asarray(up, float) - f * np.dot(up, f))
    return W + f * 0.075 * 1.14 - u * 0.03 * 1.14, f, u, np.cross(f, u) * side


def frame_matrix(origin, x, y, z):
    M = Matrix.Identity(4)
    for i, axis in enumerate((x, y, z)):
        for j in range(3):
            M[j][i] = float(axis[j])
    M.translation = Vector(origin)
    return M


def add_props(fig, W, fwd, up, pose):
    G, f, u, t = grip_point(W[1.0], fwd[1.0], up[1.0], 1.0)
    # The lantern hangs plumb from the bail's grip in his fist, the grip across his fingers.
    across = unit(np.array([t[0], t[1], 0.0]))
    fig.props.append(('lantern', frame_matrix(G, across, np.cross((0.0, 0.0, 1.0), across), (0.0, 0.0, 1.0))
                      @ Matrix.Translation(Vector((0.0, 0.0, -0.427)))))
    G, f, u, t = grip_point(W[-1.0], fwd[-1.0], up[-1.0], -1.0)
    barrel = unit([0.06, -1.0, -0.42])
    vup = unit(np.array([0.0, 0.0, 1.0]) - barrel * barrel[2])
    fig.props.append(('gun', frame_matrix(G, barrel, np.cross(vup, barrel), vup)))


def shroud_tail(fig, path, size, zone=SKIN, strips=7, seed=4, fade_from=0.28, ends=(0.74, 1.0), wave=0.035,
                folds=0.34, split=0.18, rows=64, cols=72):
    fig.add(uc.shroud(uc.Part('Shroud', zone), path, size, rows=rows, cols=cols, strips=strips, split=split, ends=ends,
                      spread=0.1, seed=seed, fade_from=fade_from, folds=folds, narrow=0.55, wave=wave), solidify=0.006)


def coat_surface(profile, z_lo, z_hi, sd_fn, folds_fn, grow, rows=150, cols=240):
    """A coat panel round the body: the profile grown by grow, displaced by folds_fn(theta, z), cut where sd_fn > 0.
    Returns (V, faces), the kept theta and z per vertex, and the distance to the coal's hole."""
    theta = np.linspace(-np.pi, np.pi, cols, endpoint=False)
    z = np.linspace(z_lo, z_hi, rows)
    th, zz = np.meshgrid(theta, z)
    P = np.zeros(th.shape + (3,))
    for i, zr in enumerate(z):
        P[i] = surface(profile, [zr], theta, grow)[0]
    P = P + outward(theta)[None, :, :] * folds_fn(th, zz)[..., None]
    tc, zc = COAL_AT
    ang = np.arctan2(zz - zc, (th - tc) * 0.17)
    rh = 0.062 * (1.0 + 0.12 * np.sin(ang * 5 + 1.0) + 0.08 * np.sin(ang * 11.0))
    hole = np.hypot((th - tc) * 0.17, zz - zc)
    sd = np.maximum(sd_fn(th, zz), rh - hole)
    V, F, used = uc.cut(P, sd)
    edge = (hole - rh).ravel()[used]
    return (V, F), th.ravel()[used], zz.ravel()[used], edge


def ember_of(edge, V, seed=2):
    return np.clip(smoothstep(0.03, 0.0, edge) * (0.5 + 0.5 * uc.pnoise(V, 50.0, seed)), 0.0, 1.0)


def torso_core(fig, zone=SHIRT, crater=True):
    """The body under the coat, in the shirt: a smooth torso with the coal's crater on the left of the chest."""
    theta = np.linspace(0.0, 2.0 * np.pi, 96, endpoint=False)
    z = np.linspace(0.92, 1.55, 40)
    piece = uc.closed_loft(surface(TORSO, z, theta))
    tc, zc = COAL_AT
    pc = surface(TORSO, [zc], np.array([tc]))[0, 0]
    nc = uc.torso_normal(tc)
    cavity, ember = 0.0, 0.0
    if crater:
        piece = uc.push(piece, pc, 0.062, -0.022)
        dist = np.linalg.norm(piece[0] - pc, axis=1)
        cavity = smoothstep(0.055, 0.03, dist) * 0.9
        ember = smoothstep(0.03, 0.05, dist) * smoothstep(0.08, 0.058, dist) * (0.4 + 0.6 * (0.5 + 0.5 * uc.pnoise(piece[0], 60.0, 5)))
    fig.add(uc.Part('Body', zone).add(piece, cavity=cavity, ember=ember))
    fig.coal = (pc - nc * 0.01, 0.05)
    return pc, nc


def sun_ring(part, center, normal, up, radius=0.022, rays=10):
    """The keepers' mark: a ring with short rays, worked in dark thread or brass, lying on a surface."""
    n = unit(normal)
    u = unit(np.asarray(up, float) - n * np.dot(up, n))
    v = np.cross(n, u)
    part.add(uc.torus(center, n, radius, 0.0028, segs=40, sides=8, squash=0.6), cavity=0.2)
    for k in range(rays):
        a = 2.0 * np.pi * k / rays
        dirv = u * math.cos(a) + v * math.sin(a)
        p = center + dirv * radius * 1.55
        part.add(uc.ellipsoid(p, (radius * 0.38, 0.0022, 0.0018), np.array([dirv, np.cross(n, dirv), n])), cavity=0.2)


# --- Option A: the Sunday keeper ---

COAT_A = [(0.42, 0.236, 0.186, 0.048, 2.0), (0.55, 0.222, 0.168, 0.036, 2.0), (0.70, 0.205, 0.148, 0.022, 2.1),
          (0.85, 0.184, 0.126, 0.01, 2.2), (0.98, 0.168, 0.113, 0.002, 2.2), (1.06, 0.160, 0.108, -0.004, 2.2)] + \
         [row for row in TORSO if row[0] > 1.06]


def figure_A(pose):
    fig = Abel('A', pose)
    kneel = pose == 'kneel'
    pc, nc = torso_core(fig)
    # The frock coat: fitted to the waist, its skirts flaring to the knee and parting in front, a vent at the back,
    # buttoned to the chest, its V filled by the shirt and cravat.
    rng = np.random.default_rng(11)
    amps, phases = rng.uniform(0.5, 1.0, 5), rng.uniform(0.0, 2.0 * np.pi, 5)

    def folds(th, zz):
        f = sum(a * np.cos(k * th + p) for a, k, p in zip(amps, (6, 9, 13, 17, 23), phases)) / amps.sum()
        skirt = 0.014 * smoothstep(1.0, 0.55, zz) * (f - 0.3 * np.abs(f))
        chest = 0.002 * np.cos(11 * th) * smoothstep(1.1, 1.3, zz)
        return 0.016 + skirt + chest

    def sd(th, zz):
        at = np.abs(th)
        hem = 0.44 + 0.006 * np.sin(5 * th + 0.4) + 0.01 * np.cos(th)
        out = hem - zz
        out = np.maximum(out, np.where(zz > 1.38, (0.1 + 1.7 * (zz - 1.38)) - at, -1.0))         # the V
        out = np.maximum(out, np.where(zz < 1.04, (0.025 + 0.42 * (1.04 - zz)) - at, -1.0))      # the skirts part
        out = np.maximum(out, np.where(zz < 0.82, (0.012 + 0.07 * (0.82 - zz)) - np.abs(wrap(th - np.pi)), -1.0))
        arm = np.sqrt(((at - np.pi / 2 - 0.04) / 0.44) ** 2 + ((zz - 1.405) / 0.1) ** 2)
        out = np.maximum(out, (1.0 - arm) * 0.06)
        return np.maximum(out, zz - 1.548)

    (V, F), th, zz, edge = coat_surface(COAT_A, 0.42, 1.548, sd, folds, 0.0)
    coat = uc.Part('Coat', COAT).add((V, F), ember=ember_of(edge, V))
    # Buttons down the closed front, and the waist seam's two buttons at the back.
    for zb in (1.36, 1.29, 1.22, 1.15, 1.08):
        p = surface(COAT_A, [zb], np.array([0.035]), grow=0.02)[0, 0]
        coat.add(uc.ellipsoid(p, (0.0075, 0.0075, 0.0045), np.array([[0, 0, 1.0], [1.0, 0, 0], [0, -1.0, 0]])), cavity=0.35)
    for s in SIDES:
        p = surface(COAT_A, [1.05], np.array([np.pi - s * 0.2]), grow=0.02)[0, 0]
        coat.add(uc.ellipsoid(p, (0.008, 0.0045, 0.008)), cavity=0.35)
    fig.add(coat, solidify=0.008)
    # Lapels along the V, laid back over the chest, and the turned-down collar.
    lapels = uc.Part('Lapels', COAT)
    for s in SIDES:
        zs = np.linspace(1.38, 1.53, 12)
        edge_th = s * (0.1 + 1.7 * (zs - 1.38))
        path = [surface(COAT_A, [zr], np.array([e + s * 0.05]), grow=0.024)[0, 0] for zr, e in zip(zs, edge_th)]
        uc.ribbon(lapels, path, [(0.0, 0.006), (0.5, 0.03), (1.0, 0.024)], n=24, cols=4, out=lambda C: outward(
            np.arctan2(C[:, 0], -C[:, 1])), curl=0.1)
    fig.add(lapels, solidify=0.005)
    theta = np.linspace(0.0, 2.0 * np.pi, 72, endpoint=False)
    collar = np.array([np.stack([rr * np.sin(theta), -0.012 - rr * 0.92 * np.cos(theta), np.full_like(theta, zz)], -1)
                       for zz, rr in ((1.53, 0.086), (1.556, 0.074), (1.575, 0.066))])
    fig.add(uc.Part('Collar', COAT).add(uc.closed_loft(collar)))
    # The shirt's stand collar and the black cravat at his throat.
    shirt = uc.Part('ShirtCollar', SHIRT)
    sc = np.array([np.stack([rr * np.sin(theta), -0.016 - rr * 0.9 * np.cos(theta), np.full_like(theta, zz)], -1)
                   for zz, rr in ((1.545, 0.058), (1.565, 0.056), (1.585, 0.055))])
    shirt.add(uc.closed_loft(sc))
    fig.add(shirt)
    cravat = uc.Part('Cravat', COAT)
    cravat.add(uc.ellipsoid((0.0, -0.082, 1.52), (0.022, 0.012, 0.016)))
    for s in SIDES:
        cravat.add(uc.ellipsoid((s * 0.026, -0.082, 1.518), (0.024, 0.009, 0.014), uc.rotation((0, 1, 0), s * 0.35)))
    cravat.add(uc.tube([(0.0, -0.08, 1.5), (0.004, -0.096, 1.45), (0.0, -0.11, 1.4)], [0.012, 0.016, 0.014], segs=12))
    fig.add(cravat)
    # The keeper's stole: a long bone-white band round his neck, falling down both sides of the coat front to below the
    # waist, the sun-ring worked at each end in black thread, the ends fringed.
    stole = uc.Part('Stole', ACCENT)
    marks = uc.Part('StoleMarks', COAT)
    for s in SIDES:
        top = [(0.0, 0.07, 1.552), (s * 0.06, 0.03, 1.575), (s * 0.095, -0.03, 1.55)]
        down = []
        for zr in (1.48, 1.38, 1.26, 1.14, 1.02, 0.9, 0.78, 0.66):
            th0 = s * (0.62 - 0.08 * smoothstep(1.48, 1.0, zr))
            p = surface(COAT_A, [zr], np.array([th0]), grow=0.03)[0, 0]
            down.append(p + np.array([0.0, -0.012 * smoothstep(1.06, 0.7, zr), 0.0]))
        path = top + down
        uc.ribbon(stole, path, [(0.0, 0.03), (0.25, 0.042), (0.8, 0.05), (1.0, 0.056)], n=60, cols=5,
                  out=lambda C: outward(np.arctan2(C[:, 0], -C[:, 1])), curl=0.08)
        end = down[-2] + (down[-1] - down[-2]) * 0.35
        sun_ring(marks, end + np.array([0.0, -0.006, 0.0]), (0.0, -1.0, 0.0), (0.0, 0.0, 1.0), radius=0.018)
        for k in range(9):        # the fringe
            x = (k - 4) / 4.0 * 0.05
            p0 = down[-1] + np.array([x, -0.002, 0.0])
            stole.add(uc.tube([p0, p0 + (0.0, -0.002, -0.04)], [0.0025, 0.002], segs=6))
    fig.add(stole, solidify=0.004)
    fig.add(marks)
    E, W, fwd, up, curls = pose_arms(pose)
    arm_parts(fig, E, W, fwd, up, curls, sleeve_r=(0.064, 0.058, 0.05))
    R = (uc.rotation((1, 0, 0), math.radians(-46)) if kneel else
         uc.rotation((0, 0, 1), math.radians(-6)) @ uc.rotation((1, 0, 0), math.radians(-4)))
    center = head_parts(fig, R, math.radians(4), 'full', 'short')
    hat(fig, 'A', R, center)
    shroud_tail(fig, [(0.0, 0.0, 0.92), (0.0, 0.06, 0.6), (0.0, 0.2, 0.36), (0.0, 0.48, 0.2), (0.0, 0.85, 0.12),
                      (0.0, 1.25, 0.08)],
                [(0.0, 0.17, 0.12), (0.25, 0.17, 0.13), (0.5, 0.12, 0.1), (0.75, 0.07, 0.06), (1.0, 0.03, 0.03)],
                strips=10, seed=5, fade_from=0.2, split=0.12)
    finish(fig, pose, W, fwd, up)
    return fig


# --- Option B: the farmer-keeper ---

COAT_B = [(0.26, 0.255, 0.225, 0.085, 2.0), (0.40, 0.248, 0.21, 0.07, 2.0), (0.60, 0.232, 0.185, 0.045, 2.0),
          (0.80, 0.21, 0.156, 0.022, 2.1), (0.95, 0.192, 0.136, 0.008, 2.2), (1.06, 0.182, 0.126, 0.0, 2.2),
          (1.16, 0.182, 0.126, -0.006, 2.3), (1.26, 0.192, 0.134, -0.008, 2.4), (1.34, 0.202, 0.136, -0.008, 2.6),
          (1.40, 0.21, 0.13, -0.004, 2.8), (1.45, 0.207, 0.118, -0.002, 3.2), (1.475, 0.194, 0.108, 0.0, 3.4),
          (1.50, 0.164, 0.098, 0.0, 3.0), (1.52, 0.122, 0.088, -0.002, 2.6), (1.54, 0.084, 0.078, -0.004, 2.2)]


def opening_B(zz):
    return 0.25 + 0.2 * smoothstep(1.2, 0.4, zz) - 0.05 * smoothstep(1.4, 1.52, zz)


def cape(keys, front_open, seed, folds=0.05, rows=48, cols=192, wind=0.0):
    """A shoulder cape (UnpaidConcepts.drape), open at the front by front_open radians, blown back by wind."""
    P, depth = uc.drape(keys, rows=rows, cols=cols, folds=folds, seed=seed, harmonics=(7, 11, 16), jag=0.02)
    theta = np.linspace(0.0, 2.0 * np.pi, cols, endpoint=False)
    if wind:
        top = P[0:1, :, 2]
        k = np.clip(top - P[..., 2], 0.0, None)
        P[..., 1] += wind * k ** 1.3 * 1.6
        P[..., 2] += wind * k ** 1.3 * 0.5
    sd = np.broadcast_to((front_open - np.abs(wrap(theta)))[None, :], P.shape[:2]) * 0.1
    V, F, used = uc.cut(P, sd)
    return V, F


def figure_B(pose):
    fig = Abel('B', pose)
    kneel = pose == 'kneel'
    torso_core(fig)
    # The work shirt's collar, open at the throat, and its placket of buttons down the front.
    theta = np.linspace(0.0, 2.0 * np.pi, 72, endpoint=False)
    shirt = uc.Part('ShirtCollar', SHIRT)
    for s in SIDES:
        path = [(s * 0.05, 0.04, 1.565), (s * 0.075, -0.03, 1.56), (s * 0.05, -0.085, 1.53), (s * 0.018, -0.105, 1.49)]
        uc.ribbon(shirt, path, [(0.0, 0.016), (0.6, 0.022), (1.0, 0.012)], n=20, cols=4,
                  out=lambda C: outward(np.arctan2(C[:, 0], -C[:, 1])), curl=0.2)
    for zb in (1.44, 1.36, 1.28, 1.2, 1.12):
        p = surface(TORSO, [zb], np.array([0.0]), grow=0.004)[0, 0]
        shirt.add(uc.ellipsoid(p, (0.006, 0.006, 0.004), np.array([[0, 0, 1.0], [1.0, 0, 0], [0, -1.0, 0]])), cavity=0.3)
    fig.add(shirt, solidify=0.004)
    # The duster: long canvas hanging open from the collar, its skirts split at the back for riding, stiff folds.
    rng = np.random.default_rng(21)
    amps, phases = rng.uniform(0.5, 1.0, 4), rng.uniform(0.0, 2.0 * np.pi, 4)

    def folds(th, zz):
        f = sum(a * np.cos(k * th + p) for a, k, p in zip(amps, (5, 7, 11, 15), phases)) / amps.sum()
        return 0.014 + 0.022 * smoothstep(1.1, 0.4, zz) * (f - 0.3 * np.abs(f))

    def sd(th, zz):
        at = np.abs(th)
        hem = 0.27 + 0.015 * np.sin(4 * th + 1.0) + 0.008 * np.sin(11 * th)
        out = hem - zz
        out = np.maximum(out, opening_B(zz) - at)
        out = np.maximum(out, np.where(zz < 0.95, (0.015 + 0.1 * (0.95 - zz)) - np.abs(wrap(th - np.pi)), -1.0))
        arm = np.sqrt(((at - np.pi / 2 - 0.04) / 0.46) ** 2 + ((zz - 1.405) / 0.105) ** 2)
        out = np.maximum(out, (1.0 - arm) * 0.06)
        return np.maximum(out, zz - 1.548)

    (V, F), th, zz, edge = coat_surface(COAT_B, 0.26, 1.548, sd, folds, 0.0)
    fig.add(uc.Part('Coat', COAT).add((V, F)), solidify=0.009)
    # Wide lapels rolled back along the opening, and the collar.
    lapels = uc.Part('Lapels', COAT)
    for s in SIDES:
        zs = np.linspace(1.05, 1.535, 16)
        path = [surface(COAT_B, [zr], np.array([s * (opening_B(zr) + 0.07)]), grow=0.025)[0, 0] for zr in zs]
        uc.ribbon(lapels, path, [(0.0, 0.01), (0.55, 0.05), (0.85, 0.055), (1.0, 0.03)], n=28, cols=4,
                  out=lambda C: outward(np.arctan2(C[:, 0], -C[:, 1])), curl=0.12)
    fig.add(lapels, solidify=0.006)
    collar = np.array([np.stack([rr * np.sin(theta), 0.004 - rr * 0.95 * np.cos(theta), np.full_like(theta, zz_)], -1)
                       for zz_, rr in ((1.52, 0.1), (1.55, 0.092), (1.585, 0.088))])
    sdc = np.broadcast_to((0.75 - np.abs(wrap(theta)))[None, :], collar.shape[:2]) * 0.1
    V, F, used = uc.cut(collar, sdc)
    fig.add(uc.Part('Collar', COAT).add((V, F)), solidify=0.008)
    # The shoulder cape over it, open in front.
    keys = [(0.0, 0.105, 1.535, 0.17, 1.445, 0.215, 1.27), (0.6, 0.12, 1.54, 0.215, 1.45, 0.262, 1.25),
            (1.2, 0.125, 1.545, 0.255, 1.43, 0.3, 1.22), (1.6, 0.12, 1.545, 0.255, 1.43, 0.3, 1.22),
            (2.2, 0.11, 1.55, 0.2, 1.445, 0.25, 1.25), (np.pi, 0.1, 1.55, 0.16, 1.445, 0.2, 1.27)]
    V, F = cape(keys, 0.32, seed=7)
    fig.add(uc.Part('Cape', COAT).add((V, F)), solidify=0.008)
    E, W, fwd, up, curls = pose_arms(pose)
    arm_parts(fig, E, W, fwd, up, curls, sleeve_r=(0.07, 0.064, 0.058))
    # The black crepe band on his left arm, and the keepers' brass sun-ring pinned to his right lapel.
    S = SHOULDER[1.0]
    p = S + (E[1.0] - S) * 0.42
    fig.add(uc.Part('Crepe', ACCENT).add(uc.torus(p, E[1.0] - S, 0.071, 0.013, squash=1.8)))
    badge = uc.Part('Badge', SHIRT)
    pb = surface(COAT_B, [1.42], np.array([-(opening_B(1.42) + 0.1)]), grow=0.036)[0, 0]
    sun_ring(badge, pb, uc.torso_normal(-(opening_B(1.42) + 0.1)), (0.0, 0.0, 1.0), radius=0.017, rays=12)
    fig.add(badge)
    R = (uc.rotation((1, 0, 0), math.radians(-46)) if kneel else
         uc.rotation((0, 0, 1), math.radians(-6)) @ uc.rotation((1, 0, 0), math.radians(-4)))
    center = head_parts(fig, R, math.radians(3), 'walrus', 'medium')
    hat(fig, 'B', R, center)
    shroud_tail(fig, [(0.0, 0.0, 0.95), (0.0, 0.08, 0.5), (0.0, 0.3, 0.22), (0.0, 0.62, 0.1), (0.0, 1.0, 0.06),
                      (0.0, 1.45, 0.05)],
                [(0.0, 0.2, 0.15), (0.25, 0.19, 0.15), (0.5, 0.13, 0.1), (0.75, 0.07, 0.06), (1.0, 0.03, 0.03)],
                strips=11, seed=8, fade_from=0.22, split=0.14)
    finish(fig, pose, W, fwd, up)
    return fig


# --- Option C: the storm keeper ---

COAT_C = [(0.92, 0.19, 0.13, 0.01, 2.2), (1.06, 0.178, 0.122, 0.0, 2.2)] + \
         [(z, w + 0.012, d + 0.012, y, p) for z, w, d, y, p in TORSO if z > 1.06]


def figure_C(pose):
    fig = Abel('C', pose)
    kneel = pose == 'kneel'
    wind = 0.0 if kneel else 1.0
    torso_core(fig)

    # The greatcoat, buttoned double down the front, its collar turned up against the wind.
    def folds(th, zz):
        return 0.012 + 0.003 * np.cos(9 * th + 0.5) * smoothstep(1.2, 0.95, zz)

    def sd(th, zz):
        at = np.abs(th)
        out = 0.94 - zz
        arm = np.sqrt(((at - np.pi / 2 - 0.04) / 0.46) ** 2 + ((zz - 1.405) / 0.105) ** 2)
        out = np.maximum(out, (1.0 - arm) * 0.06)
        return np.maximum(out, zz - 1.548)

    (V, F), th, zz, edge = coat_surface(COAT_C, 0.92, 1.548, sd, folds, 0.0)
    coat = uc.Part('Coat', COAT).add((V, F), ember=ember_of(edge, V))
    for zb in (1.4, 1.3, 1.2, 1.1):
        for tb in (-0.3, 0.3):
            p = surface(COAT_C, [zb], np.array([tb]), grow=0.02)[0, 0]
            if np.hypot((tb - COAL_AT[0]) * 0.17, zb - COAL_AT[1]) > 0.075:
                coat.add(uc.ellipsoid(p, (0.008, 0.008, 0.005)), cavity=0.35)
    fig.add(coat, solidify=0.009)
    theta = np.linspace(0.0, 2.0 * np.pi, 72, endpoint=False)
    collar = np.array([np.stack([rr * np.sin(theta), 0.012 - rr * 0.95 * np.cos(theta), np.full_like(theta, zz_)], -1)
                       for zz_, rr in ((1.52, 0.104), (1.56, 0.1), (1.61, 0.104), (1.645, 0.112))])
    sdc = np.broadcast_to((0.42 - np.abs(wrap(theta)))[None, :], collar.shape[:2]) * 0.1
    V, F, used = uc.cut(collar, sdc)
    fig.add(uc.Part('Collar', COAT).add((V, F)), solidify=0.01)
    # Three shoulder capes, each shorter over the last, lifting in the Gravewind.
    for k, hem in enumerate((1.12, 1.2, 1.28)):
        spread = 1.0 + 0.12 * (2 - k)
        keys = [(0.0, 0.11, 1.53, 0.17, 1.445, 0.2 * spread, hem), (0.6, 0.12, 1.535, 0.215, 1.45, 0.25 * spread, hem - 0.02),
                (1.2, 0.125, 1.54, 0.255, 1.43, 0.29 * spread, hem - 0.04), (1.6, 0.12, 1.54, 0.255, 1.43, 0.29 * spread, hem - 0.04),
                (2.2, 0.11, 1.545, 0.2, 1.445, 0.24 * spread, hem - 0.02), (np.pi, 0.1, 1.545, 0.16, 1.445, 0.2 * spread, hem)]
        V, F = cape(keys, 0.12, seed=30 + k, folds=0.06, wind=0.35 * wind)
        fig.add(uc.Part(f'Cape{k}', COAT).add((V, F)), solidify=0.007)
    E, W, fwd, up, curls = pose_arms(pose)
    arm_parts(fig, E, W, fwd, up, curls, sleeve_r=(0.068, 0.062, 0.056))
    # The scarf round his neck, both ends streaming back.
    scarf = uc.Part('Scarf', ACCENT)
    scarf.add(uc.torus((0.0, 0.006, 1.535), (0.0, 0.0, 1.0), 0.104, 0.02, squash=1.3))
    for k, s in enumerate(SIDES):
        L = 1.0 if wind else 0.4
        path = [(s * 0.06, 0.09, 1.535), (s * 0.09, 0.22, 1.53 - 0.1 * (1 - wind)),
                (s * 0.07 + 0.03, 0.22 + 0.4 * L, 1.5 + 0.05 * s * wind - 0.35 * (1 - wind)),
                (s * 0.12 + 0.05, 0.22 + 0.85 * L, 1.47 - 0.04 * s * wind - 0.5 * (1 - wind))]
        uc.ribbon(scarf, path, [(0.0, 0.07), (0.8, 0.075), (1.0, 0.06)], n=40, cols=5, out=(s, 0.0, 0.4),
                  curl=0.15, twist=0.3 * s, wave=0.05 * wind, ragged=0.15, fade_from=0.75, seed=70 + k)
    fig.add(scarf, solidify=0.004)
    R = (uc.rotation((1, 0, 0), math.radians(-46)) if kneel else
         uc.rotation((0, 0, 1), math.radians(-6)) @ uc.rotation((1, 0, 0), math.radians(-2)))
    center = head_parts(fig, R, math.radians(4), 'long', 'long', wind=wind)
    hat(fig, 'C', R, center)
    # Below the coat the greatcoat's skirts are the Gravewind: long torn tails streaming back off him, fading to fog.
    if kneel:
        path = [(0.0, 0.0, 1.0), (0.0, 0.1, 0.62), (0.0, 0.35, 0.36), (0.0, 0.75, 0.2), (0.0, 1.2, 0.12), (0.0, 1.7, 0.1)]
    else:
        path = [(0.0, 0.0, 1.0), (0.0, 0.14, 0.7), (0.0, 0.48, 0.52), (0.0, 0.95, 0.46), (0.0, 1.5, 0.5), (0.0, 2.1, 0.58)]
    shroud_tail(fig, path, [(0.0, 0.2, 0.14), (0.15, 0.24, 0.18), (0.45, 0.22, 0.16), (0.75, 0.12, 0.1),
                            (1.0, 0.05, 0.05)], zone=COAT, strips=10, seed=12, fade_from=0.45, ends=(0.7, 1.0),
                wave=0.06, folds=0.4, split=0.22, rows=72, cols=96)
    finish(fig, pose, W, fwd, up)
    return fig


def hat(fig, key, R, center):
    felt, band = HATS[key]()
    Rh = R @ uc.rotation((1, 0, 0), math.radians(-3)) @ uc.rotation((0, 1, 0), math.radians(3))
    at = PIVOT + R @ np.array([0.0, 0.0, 0.153])
    part = uc.Part('Hat', COAT)
    for V, F in felt:
        part.add((V @ Rh.T + at, F))
    fig.add(part, solidify=0.007)
    fig.add(uc.Part('HatBand', ACCENT if key == 'B' else COAT).add((band[0] @ Rh.T + at, band[1])), solidify=0.004)


KNEEL_LEAN, KNEEL_DROP = math.radians(28.0), 0.55
kneel_lean = uc.lean(KNEEL_LEAN, pivot_z=1.0, width=0.1)
UNLAPPED = ('Sleeve', 'Hand', 'Head', 'Beard', 'Brows', 'HairCap', 'Hair', 'BeardLocks', 'Hat', 'HatBand', 'Coal',
            'Crepe', 'Badge', 'Scarf')


def kneel_lap(V, hip=1.0, thigh=0.46):
    """Kneeling, sat back on his heels: the front of the coat below the hips swings forward over the thighs into a lap,
    and past the knees it falls to the boards."""
    V = np.asarray(V, float).copy()
    front = smoothstep(0.08, -0.02, V[:, 1])
    a = -math.radians(76.0) * smoothstep(hip, hip - 0.2, V[:, 2]) * front
    y, z = V[:, 1], V[:, 2] - hip
    y2, z2 = y * np.cos(a) - z * np.sin(a), hip + y * np.sin(a) + z * np.cos(a)
    past = np.clip(-thigh - y2, 0.0, None) * front
    V[:, 1] = y2 + past * 0.45
    V[:, 2] = z2 - past * 1.6
    return V


def kneel_sink(V):
    """Down onto the boards: everything drops, and what would go through them pools on them, spreading out."""
    V = np.asarray(V, float).copy()
    V[:, 2] -= KNEEL_DROP
    below = V[:, 2] < 0.012
    depth = 0.012 - V[:, 2]
    r = np.hypot(V[:, 0], V[:, 1] - 0.05) + 1e-6
    spread = np.where(below, depth * 0.95, 0.0)
    V[:, 0] += V[:, 0] / r * spread
    V[:, 1] += (V[:, 1] - 0.05) / r * spread
    V[:, 2] = np.where(below, 0.012 + 0.012 * np.tanh(depth * 6.0), V[:, 2])
    return V


def kneeling(p):
    return kneel_sink(kneel_lean(np.atleast_2d(p)))[0]


def finish(fig, pose, W, fwd, up):
    """The coal, the props, and for the kneel: lean, bow and sink to the boards, the coat pooling on them."""
    fig.add(uc.Part('Coal', 0).add(uc.coal_piece(*fig.coal)), material='coal')
    if pose == 'kneel':
        fig.warp(kneel_lean)
        for part, _ in fig.parts:
            if not part.name.startswith(UNLAPPED):
                part.V = [kneel_lap(V) for V in part.V]
        fig.warp(kneel_sink)
        # The lantern stands on the boards under his left hand's grip; the pump lies on the boards on his right.
        G = kneeling(grip_point(W[1.0], fwd[1.0], up[1.0], 1.0)[0])
        fig.props.append(('lantern', Matrix.Translation(Vector((G[0], G[1], 0.0)))))
        fig.props.append(('gun', frame_matrix((-0.5, -0.36, 0.035), unit([1.0, -0.3, 0.0]), unit([0.3, 1.0, 0.0]),
                                              (0.0, 0.0, 1.0)) @ Matrix.Rotation(math.radians(85.0), 4, 'X')))
        return
    add_props(fig, W, fwd, up, pose)


def instantiate(fig, where, location=(0.0, 0.0, 0.0), turn=0.0, scale=SCALE, coal=BOSS_COAL, light=True):
    """Builds the figure (scaled about its feet), its lantern and gun; returns the objects and the flame's position."""
    ghost = uc.ghost_material(TINTS[fig.key], coal)
    ember = uc.coal_material(EMBER if fig.ember else coal)
    objs = []
    for part, options in fig.parts:
        obj = uc.build(part, ember if options.get('material') == 'coal' else ghost, where, location=location, turn=turn,
                       **{k: v for k, v in options.items() if k != 'material'})
        obj.scale = (scale, scale, scale)
        objs.append(obj)
    M = Matrix.Translation(Vector(location)) @ Matrix.Rotation(turn, 4, 'Z') @ Matrix.Scale(scale, 4)
    flame = None
    for kind, P in fig.props:
        obj = PROPS[kind]().copy()
        where.objects.link(obj)
        obj.matrix_world = M @ P
        obj.hide_render = False
        objs.append(obj)
        if kind == 'lantern':
            flame = obj.matrix_world @ Vector((0.0, 0.0, 0.122))
            if light:
                lamp = bpy.data.objects.new('GhostLight', bpy.data.lights.new('GhostLight', 'POINT'))
                lamp.data.energy = 30.0 * scale * scale
                lamp.data.color = lt.hex_color(FLAME)[:3]
                lamp.data.shadow_soft_size = 0.06
                lamp.location = flame
                where.objects.link(lamp)
    return objs, flame


FIGURES = {'A': figure_A, 'B': figure_B, 'C': figure_C}
_cache = {}


def figure(key, pose='fight'):
    if (key, pose) not in _cache:
        t0 = time.time()
        _cache[(key, pose)] = FIGURES[key](pose)
        log(f'{key} {pose}: {_cache[(key, pose)].triangles()} triangles (concept density), built in '
            f'{time.time() - t0:.1f} s')
    return _cache[(key, pose)]


# --- Props: the ghost lantern and the spectral pump ---

_props = {}


def prop_material(name, color, rim=0.6, glow=0.12):
    """A ghost prop's surface: the pale tint, the fresnel rim and a faint glow, as the ghost material draws a body."""
    mat = bpy.data.materials.get(name) or bpy.data.materials.new(name)
    mat.use_nodes = True
    nodes, links = mat.node_tree.nodes, mat.node_tree.links
    nodes.clear()
    out = nodes.new('ShaderNodeOutputMaterial')
    bsdf = nodes.new('ShaderNodeBsdfPrincipled')
    links.new(bsdf.outputs['BSDF'], out.inputs['Surface'])
    bsdf.inputs['Base Color'].default_value = lt.hex_color(color)
    bsdf.inputs['Roughness'].default_value = 0.7
    facing = nodes.new('ShaderNodeLayerWeight')
    facing.inputs['Blend'].default_value = 0.35
    k = uc._math(nodes, links, 'POWER', facing.outputs['Facing'], 2.8)
    k = uc._math(nodes, links, 'MULTIPLY', k, rim)
    rim_c = uc._vscale(nodes, links, uc._color(nodes, uc.RIM), k)
    emit = uc._vadd(nodes, links, rim_c, uc._vscale(nodes, links, uc._color(nodes, color), glow))
    links.new(emit, bsdf.inputs['Emission Color'])
    bsdf.inputs['Emission Strength'].default_value = 1.0
    mat.surface_render_method = 'DITHERED'
    mat.use_backface_culling = False
    return mat


def flame_material():
    mat = bpy.data.materials.get('GhostFlame') or bpy.data.materials.new('GhostFlame')
    mat.use_nodes = True
    bsdf = next(n for n in mat.node_tree.nodes if n.type == 'BSDF_PRINCIPLED')
    bsdf.inputs['Base Color'].default_value = lt.hex_color(FLAME)
    bsdf.inputs['Emission Color'].default_value = lt.hex_color(FLAME)
    bsdf.inputs['Emission Strength'].default_value = 7.0
    return mat


def deck_models():
    """BurialDeck.py's deck, bier, keeper's lantern post and the Keeper's Lantern, built once (no AO bake) and kept
    out of the renders: scenes place copies."""
    if 'deck' in _props:
        return _props['deck']
    path = os.path.join(lt.REPO, 'Art', 'Models', 'Props', 'BurialDeck.py')
    saved = sys.argv
    sys.argv = [saved[0], '--', '--only=BurialDeck,Bier,KeeperLanternPost,KeepersLantern', '--no-ao']
    ns = {'__name__': 'burial_deck', '__file__': path}
    exec(compile(open(path, encoding='utf-8').read(), path, 'exec'), ns)
    sys.argv = saved
    keep = bpy.data.collections.new('DeckModels')
    bpy.context.scene.collection.children.link(keep)
    found = {}
    models = [obj for obj in bpy.data.objects
              if obj.parent is None and obj.name in ('BurialDeck', 'Bier', 'KeeperLanternPost', 'KeepersLantern')]
    for obj in models:
        sockets = {c.name.split('.')[0][len('SOCKET_'):]: (Vector(c.location), c.rotation_euler.z)
                   for c in obj.children if c.name.startswith('SOCKET_')}
        for c in list(obj.children):
            bpy.data.objects.remove(c, do_unlink=True)
        for coll in list(obj.users_collection):
            coll.objects.unlink(obj)
        keep.objects.link(obj)
        obj.hide_render = True
        found[obj.name] = (obj, sockets)
    keep.hide_render = True
    _props['deck'] = found
    return found


def ghost_lantern():
    """The Keeper's Lantern gone to spirit: its own shape in pale ghost brass and iron, the globe a pale flame."""
    if 'lantern' not in _props:
        src = deck_models()['KeepersLantern'][0]
        obj = src.copy()
        obj.data = src.data.copy()
        obj.name = 'GhostLantern'
        slots = {'BrassWorn': prop_material('GhostBrass', 0xcabf9f, rim=0.8),
                 'LanternGlow': flame_material(),
                 'IronBlack': prop_material('GhostIron', 0x7d8285, rim=0.8),
                 'HouseTrim': prop_material('GhostGrip', 0x9a8774, rim=0.6)}
        for k, mat in enumerate(obj.data.materials):
            obj.data.materials[k] = slots.get(mat.name.split('.')[0], slots['BrassWorn'])
        _props['lantern'] = obj
    return _props['lantern']


def gun_parts():
    """The spectral twin of his Ranchhand pump (Heirloom), in gun space: X along the barrel from the wrist of the
    stock, Z up. A classic walnut pump in the Ranchhand's proportions (a 20 cm receiver, a 70 cm barrel)."""
    zones = {'steel': 0, 'wood': 1, 'dark': 2}
    parts = {k: uc.Part(f'Gun{k}', z) for k, z in zones.items()}

    def loft(sections, segs=24):
        """Rounded sections along x: (x, half width, half height, z middle, power)."""
        a = np.linspace(0.0, 2.0 * np.pi, segs, endpoint=False)
        rings = []
        for x, w, h, zc, p in sections:
            c, s = np.cos(a), np.sin(a)
            e = 2.0 / p
            rings.append(np.stack([np.full_like(a, x), w * np.sign(s) * np.abs(s) ** e,
                                   zc + h * np.sign(c) * np.abs(c) ** e], -1))
        P = np.array(rings)
        V = np.vstack([P.reshape(-1, 3), P[0].mean(0), P[-1].mean(0)])
        R = len(sections)
        return uc.orient((V, [uc.grid_faces(R, segs), uc.fan(R * segs, np.arange(segs)),
                              uc.fan(R * segs + 1, np.arange(segs) + (R - 1) * segs, top=True)]))
    # The stock: a slim wrist swelling to the comb and a tall butt, dropping at the heel.
    parts['wood'].add(loft([(0.05, 0.016, 0.022, -0.004, 2.4), (0.0, 0.017, 0.021, -0.008, 2.4),
                            (-0.06, 0.018, 0.026, -0.016, 2.4), (-0.14, 0.02, 0.038, -0.03, 2.6),
                            (-0.24, 0.021, 0.05, -0.044, 2.8), (-0.33, 0.022, 0.06, -0.054, 3.0),
                            (-0.345, 0.021, 0.059, -0.055, 3.0)]))
    parts['dark'].add(loft([(-0.343, 0.022, 0.06, -0.055, 3.0), (-0.36, 0.022, 0.06, -0.055, 3.0)]))
    # The receiver, a rounded block, with the trigger guard under it.
    parts['steel'].add(loft([(0.04, 0.019, 0.03, 0.0, 3.0), (0.06, 0.022, 0.034, 0.002, 3.4),
                             (0.22, 0.022, 0.034, 0.002, 3.4), (0.245, 0.02, 0.03, 0.004, 3.0)]))
    parts['steel'].add(uc.torus((0.085, 0.0, -0.045), (0.0, 1.0, 0.0), 0.022, 0.0035, squash=1.0))
    parts['dark'].add(uc.tube([(0.09, 0.0, -0.03), (0.092, 0.0, -0.05)], [0.003, 0.0025], segs=6))
    # The barrel over the magazine tube, a bead at the muzzle; the grooved walnut pump on the tube.
    parts['steel'].add(uc.tube([(0.24, 0.0, 0.014), (0.8, 0.0, 0.014)], 0.0115, segs=20))
    parts['dark'].add(uc.tube([(0.24, 0.0, -0.014), (0.66, 0.0, -0.014)], 0.0105, segs=18))
    parts['steel'].add(uc.ellipsoid((0.795, 0.0, 0.029), (0.003, 0.003, 0.003), segs=8, rings=6))
    xs = np.linspace(0.34, 0.54, 30)
    C = np.stack([xs, np.zeros_like(xs), np.full_like(xs, -0.014)], -1)

    def pump(t, a):
        return 0.022 * (1.0 - 0.06 * (np.cos(2.0 * np.pi * t * 9.0) > 0.3) * smoothstep(0.05, 0.12, t)
                        * smoothstep(0.95, 0.88, t))
    parts['wood'].add(uc.tube_fn(C, pump, segs=20, up=(0.0, 0.0, 1.0)))
    return parts


def ghost_gun():
    if 'gun' not in _props:
        keep = bpy.data.collections.get('PropModels') or bpy.data.collections.new('PropModels')
        if keep.name not in bpy.context.scene.collection.children:
            bpy.context.scene.collection.children.link(keep)
        mat = uc.ghost_material(GUN_TINT, BOSS_COAL)
        parts = gun_parts()
        pieces = []
        for part in parts.values():
            pieces.append(uc.build(part, mat, keep))
        bpy.ops.object.select_all(action='DESELECT')
        for obj in pieces:
            obj.select_set(True)
        bpy.context.view_layer.objects.active = pieces[0]
        bpy.ops.object.join()
        gun = bpy.context.view_layer.objects.active
        gun.name = 'GhostGun'
        gun.hide_render = True
        _props['gun'] = gun
    return _props['gun']


PROPS = {'lantern': ghost_lantern, 'gun': ghost_gun}


# --- Debug ---

def shot_debug(key):
    uc.reset()
    _props.clear()
    where = uc.collection('Debug')
    fig = figure(key, 'fight')
    instantiate(fig, where, light=False)
    uc.studio(where, 0x9d9484)
    uc.sun((-0.45, -0.8, 0.45), strength=3.6)
    lo, hi = fig.bounds()
    c = Vector((0.0, 0.0, 1.05 * SCALE))
    for name, d in (('front', (0.0, -1.0, 0.05)), ('three', (0.8, -1.0, 0.1)), ('side', (1.0, 0.0, 0.05))):
        cam = uc.camera(c + Vector(d).normalized() * 7.0, c, lens=50.0)
        uc.render(os.path.join(WORK, f'debug_{key}_{name}.png'), (800, 1000), samples=16)
        bpy.data.objects.remove(cam)
    head = Vector((0.0, -0.05, 1.72)) * SCALE
    cam = uc.camera(head + Vector((0.45, -1.0, 0.08)).normalized() * 1.1, head, lens=85.0)
    uc.render(os.path.join(WORK, f'debug_{key}_head.png'), (900, 900), samples=16)


# --- The burial deck on Gravewind Point, at dusk or in the golden afternoon ---

LIGHTS = {
    # sun toward (elevation), its color and strength; sky ramp (height 0..1: color); sun glow; canyon fog
    'dusk': dict(sun=(-0.36, -0.93, 0.1), color=(1.0, 0.56, 0.3), strength=3.2,
                 ramp=((0.44, 0x241a22), (0.5, 0xff9a52), (0.53, 0xee7c4e), (0.6, 0xa4536a), (0.76, 0x463e6e),
                       (0.95, 0x1b1d40)),
                 glow=(0xffc27e, 24.0, 1.3), fog=(0.006, (1.0, 0.76, 0.6)), ridge=0x2a1c25, haze_mix=0.1,
                 sky=0.9),
    'day': dict(sun=(0.45, 0.55, 0.62), color=(1.0, 0.84, 0.6), strength=4.6,
                ramp=((0.44, 0x6b5a4a), (0.5, 0xf0d0a0), (0.55, 0xd9d4c4), (0.68, 0x8eb0d8), (0.95, 0x4a78b6)),
                glow=(0xfff0d0, 10.0, 0.45), fog=(0.003, (1.0, 0.94, 0.86)), ridge=0x8d6f58, haze_mix=0.16,
                sky=1.0),
}


def world_sky(mode):
    L = LIGHTS[mode]
    world = bpy.data.worlds.new(f'AbelSky_{mode}')
    world.use_nodes = True
    nodes, links = world.node_tree.nodes, world.node_tree.links
    nodes.clear()
    out = nodes.new('ShaderNodeOutputWorld')
    bg = nodes.new('ShaderNodeBackground')
    bg.inputs['Strength'].default_value = L['sky']
    coords = nodes.new('ShaderNodeTexCoord')
    norm = nodes.new('ShaderNodeVectorMath')
    norm.operation = 'NORMALIZE'
    links.new(coords.outputs['Generated'], norm.inputs[0])
    split = nodes.new('ShaderNodeSeparateXYZ')
    links.new(norm.outputs['Vector'], split.inputs['Vector'])
    height = uc._math(nodes, links, 'MULTIPLY_ADD', split.outputs['Z'], 0.5)
    height.node.inputs[2].default_value = 0.5
    ramp = nodes.new('ShaderNodeValToRGB')
    els = ramp.color_ramp.elements
    stops = L['ramp']
    els[0].position, els[0].color = stops[0][0], lt.hex_color(stops[0][1])
    els[1].position, els[1].color = stops[-1][0], lt.hex_color(stops[-1][1])
    for position, color in stops[1:-1]:
        els.new(position).color = lt.hex_color(color)
    links.new(height, ramp.inputs['Fac'])
    # The glow round the sun: a tight core and a wide halo.
    dot = nodes.new('ShaderNodeVectorMath')
    dot.operation = 'DOT_PRODUCT'
    links.new(norm.outputs['Vector'], dot.inputs[0])
    dot.inputs[1].default_value = tuple(Vector(L['sun']).normalized())
    near = uc._math(nodes, links, 'MAXIMUM', dot.outputs['Value'], 0.0)
    color, core, halo = L['glow']
    k = uc._math(nodes, links, 'ADD', uc._math(nodes, links, 'MULTIPLY', uc._math(nodes, links, 'POWER', near, 900.0), core),
                 uc._math(nodes, links, 'MULTIPLY', uc._math(nodes, links, 'POWER', near, 9.0), halo))
    sky = uc._vadd(nodes, links, ramp.outputs['Color'], uc._vscale(nodes, links, uc._color(nodes, color), k))
    links.new(sky, bg.inputs['Color'])
    links.new(bg.outputs['Background'], out.inputs['Surface'])
    bpy.context.scene.world = world


def mix_hex(a, b, t):
    ca, cb = [(a >> s) & 0xff for s in (16, 8, 0)], [(b >> s) & 0xff for s in (16, 8, 0)]
    return sum(int(round(x + (y - x) * t)) << s for x, y, s in zip(ca, cb, (16, 8, 0)))


def ridges(where, mode, seed=9):
    """The far side of Gravewind Canyon: mesas and ridges in layers to the west, each farther one grayer with the haze
    (they cast no shadows: the low sun stands over them)."""
    rng = np.random.default_rng(seed)
    horizon = LIGHTS[mode]['ramp'][1][1]
    for k, (dist, top, spread) in enumerate(((420.0, 62.0, 0.6), (760.0, 105.0, 0.9), (1150.0, 150.0, 1.2))):
        mat = uc.flat_material(f'Ridge_{mode}_{k}', mix_hex(LIGHTS[mode]['ridge'], horizon,
                                                            LIGHTS[mode]['haze_mix'] * (1 + 1.6 * k)), 1.0)
        xs = np.linspace(-1500.0, 1500.0, 320)
        h = top * (0.55 + 0.18 * np.sin(xs / (180.0 * spread) + rng.uniform(0, 6))
                   + 0.14 * np.sin(xs / (61.0 * spread) + rng.uniform(0, 6))
                   + 0.08 * np.sin(xs / (23.0 * spread) + rng.uniform(0, 6)))
        h = np.where(h > top * 0.72, top * 0.72 + (h - top * 0.72) * 0.12, h)        # flat mesa tops
        h += rng.uniform(-1.5, 1.5, len(xs))
        V = np.vstack([np.stack([xs, np.full_like(xs, -dist), np.full_like(xs, -120.0)], -1),
                       np.stack([xs, np.full_like(xs, -dist), h], -1)])
        n = len(xs)
        F = np.array([(k, k + 1, n + k + 1, n + k) for k in range(n - 1)])
        obj = uc.mesh_object('Ridge', V, [F], mat, where)
        obj.data.polygons.foreach_set('use_smooth', np.zeros(len(obj.data.polygons), dtype=bool))
        obj.visible_shadow = False


def canyon_fog(where, mode):
    density, color = LIGHTS[mode]['fog']
    mat = bpy.data.materials.new(f'CanyonFog_{mode}')
    mat.use_nodes = True
    nodes, links = mat.node_tree.nodes, mat.node_tree.links
    nodes.clear()
    out = nodes.new('ShaderNodeOutputMaterial')
    vol = nodes.new('ShaderNodeVolumePrincipled')
    vol.inputs['Color'].default_value = (*color, 1.0)
    vol.inputs['Anisotropy'].default_value = 0.65
    # Thinning toward its top, so the fog rises out of the canyon softly instead of lying flat as a sea.
    coords = nodes.new('ShaderNodeTexCoord')
    split = nodes.new('ShaderNodeSeparateXYZ')
    links.new(coords.outputs['Object'], split.inputs['Vector'])
    ramp = nodes.new('ShaderNodeMapRange')
    ramp.inputs['From Min'].default_value = -1.0
    ramp.inputs['From Max'].default_value = -28.0
    ramp.inputs['To Min'].default_value = 0.0
    ramp.inputs['To Max'].default_value = density
    links.new(split.outputs['Z'], ramp.inputs['Value'])
    links.new(ramp.outputs['Result'], vol.inputs['Density'])
    links.new(vol.outputs['Volume'], out.inputs['Volume'])
    V, F = uc.box_piece((0.0, -560.0, -41.0), (900.0, 547.0, 40.0))
    obj = uc.mesh_object('CanyonFog', V, F, mat, where)
    obj.visible_shadow = False


def stage(mode, where):
    """The deck at its surface (z 0), its biers and lantern posts in their places, the sky, the sun, the canyon."""
    models = deck_models()
    deck, sockets = models['BurialDeck']
    shift = Vector((0.0, 0.0, -0.4))
    for name, obj in (('BurialDeck', deck),):
        copy = obj.copy()
        where.objects.link(copy)
        copy.location = shift
        copy.hide_render = False
    for name, (loc, rz) in sockets.items():
        kind = 'Bier' if name.startswith('Bier') else 'KeeperLanternPost'
        copy = models[kind][0].copy()
        where.objects.link(copy)
        copy.location = loc + shift
        copy.rotation_euler = (0.0, 0.0, rz)
        copy.hide_render = False
    world_sky(mode)
    L = LIGHTS[mode]
    uc.sun(L['sun'], strength=L['strength'], color=L['color'], where=where)
    ridges(where, mode)
    canyon_fog(where, mode)
    scene = bpy.context.scene
    for attr, value in (('volumetric_end', 1600.0), ('volumetric_tile_size', '4'), ('volumetric_samples', 96),
                        ('use_volumetric_shadows', True), ('volumetric_shadow_samples', 32)):
        if hasattr(scene.eevee, attr):
            setattr(scene.eevee, attr, value)


FAST = 'fast' in ARGV          # quick looks: half size, few samples, into Intermediate/AbelConcepts


def render(path, resolution, samples):
    if FAST:
        path = os.path.join(WORK, 'fast_' + os.path.basename(path))
        resolution, samples = (resolution[0] // 2, resolution[1] // 2), 12
    uc.render(path, resolution, samples=samples)


def lens_shot(path, location, target, lens, resolution, samples=64, focus=None, fstop=None):
    cam = uc.camera(Vector(location), Vector(target), lens=lens)
    cam.data.clip_end = 8000.0          # the canyon's far rim is a kilometer out
    if focus is not None:
        cam.data.dof.use_dof = True
        cam.data.dof.focus_distance = focus
        cam.data.dof.aperture_fstop = fstop
    render(path, resolution, samples)
    bpy.data.objects.remove(cam)


def shot_path(key, name):
    return os.path.join(OUT, f'Abel_{key}_{name}.png')


AT_EDGE = (-5.3, -10.5)       # where he stands for the hero shots, two meters in from the deck's open edge


def shot_hero(key, mode):
    """Three-quarter view on the deck's open edge, the sun setting (or lower in the golden afternoon) behind him."""
    uc.reset()
    _props.clear()
    where = uc.collection('Hero')
    stage(mode, where)
    fig = figure(key, 'fight')
    instantiate(fig, where, location=(*AT_EDGE, 0.0), turn=math.pi)
    target = Vector((AT_EDGE[0], AT_EDGE[1], 1.5))
    lens_shot(shot_path(key, mode), target + Vector((2.2, 4.3, -0.4)), target, 33.0, (1600, 1000), samples=64)


def shot_face(key):
    """His face at dusk, the ghost light under it and the sunset behind."""
    uc.reset()
    _props.clear()
    where = uc.collection('Face')
    stage('dusk', where)
    fig = figure(key, 'fight')
    instantiate(fig, where, location=(*AT_EDGE, 0.0), turn=math.pi)
    head = Vector((AT_EDGE[0], AT_EDGE[1] + 0.05 * SCALE, (PIVOT[2] + 0.09) * SCALE))
    eye = head + Vector((0.42, 1.0, -0.05)).normalized() * 1.05
    # The ghost light's warmth on his face from below (his lantern), as the eye would see it at this distance.
    fill = bpy.data.objects.new('LanternFill', bpy.data.lights.new('LanternFill', 'AREA'))
    fill.data.energy = 9.0
    fill.data.size = 0.25
    fill.data.color = lt.hex_color(FLAME)[:3]
    fill.location = head + Vector((-0.1, 0.45, -0.42))
    fill.rotation_euler = (head - fill.location).to_track_quat('-Z', 'Y').to_euler()
    where.objects.link(fill)
    lens_shot(shot_path(key, 'face'), eye, head + Vector((0.0, 0.0, -0.03)), 85.0, (1200, 1200), samples=64,
              focus=(head - eye).length - 0.11, fstop=8.0)


def shot_kneel(key):
    """At zero: on his knees on the boards, the lantern set down, his hand over his eyes, the coal open and sinking."""
    uc.reset()
    _props.clear()
    where = uc.collection('Kneel')
    stage('dusk', where)
    fig = figure(key, 'kneel')
    spot = (1.4, -9.6)
    instantiate(fig, where, location=(*spot, 0.0), turn=math.pi + math.radians(14))
    target = Vector((spot[0], spot[1], 0.95))
    lens_shot(shot_path(key, 'kneel'), target + Vector((-1.9, 4.6, 0.55)), target, 35.0, (1600, 1000), samples=64)


def shot_turnaround(key):
    """Front, side and back beside a 1.8 m post and an Unpaid, orthographic."""
    uc.reset()
    _props.clear()
    where = uc.collection('Turn')
    fig = figure(key, 'fight')
    turns = (0.0, math.pi / 2.0, math.pi)
    spans = [tuple(np.array([fig.bounds(t)[0][0], fig.bounds(t)[1][0]]) * SCALE) for t in turns]
    unpaid = uc.figure('D', 'idle')
    ulo, uhi = unpaid.bounds(math.radians(-25))
    xs, total = uc.lineup([(-0.08, 0.08), (ulo[0], uhi[0])] + spans, gap=0.6)
    uc.post(where, (xs[0], 0.0, 0.0))
    uc.label('1.8 m', (xs[0] + 0.1, -0.05, 1.78), 0.075, align='LEFT')
    uc.instantiate(unpaid, where, location=(xs[1], 0.0, 0.0), turn=math.radians(-25))
    uc.label('an Unpaid', (xs[1] + (ulo[0] + uhi[0]) / 2, -0.6, 0.06), 0.1)
    for x, t, name, (lo, hi) in zip(xs[2:], turns, ('front', 'side', 'back'), spans):
        instantiate(fig, where, location=(x, 0.0, 0.0), turn=t, light=False)
        uc.label(name, (x + (lo + hi) / 2, -0.6, 0.06), 0.1)
    uc.label(TITLES[key], (xs[0] - 0.1, -1.5, 2.95), 0.13, align='LEFT')
    uc.studio(where)
    uc.sun((-0.4, -0.85, 0.45), strength=3.8)
    width = total + 0.7
    height = 3.25
    cam = uc.camera((0.0, -30.0, 1.45 + 30.0 * math.tan(math.radians(7.0))), (0.0, 0.0, 1.45), ortho=width)
    render(shot_path(key, 'turnaround'), (2400, int(2400 * height / width)), 48)


def shot_compare():
    """The three in the same light and at the same scale, an Unpaid and a 1.8 m post beside them."""
    uc.reset()
    _props.clear()
    where = uc.collection('Compare')
    turn = math.radians(-28.0)
    figs = [figure(k, 'fight') for k in OPTIONS]
    unpaid = uc.figure('D', 'idle')
    ulo, uhi = unpaid.bounds(turn)
    items = [(-0.08, 0.08), (ulo[0], uhi[0])]
    for fig in figs:
        lo, hi = fig.bounds(turn)
        items.append((lo[0] * SCALE, hi[0] * SCALE))
    xs, total = uc.lineup(items, gap=0.55)
    uc.post(where, (xs[0], 0.0, 0.0))
    uc.label('1.8 m', (xs[0] + 0.1, -0.05, 1.78), 0.075, align='LEFT')
    uc.instantiate(unpaid, where, location=(xs[1], 0.0, 0.0), turn=turn)
    uc.label('an Unpaid', (xs[1] + (ulo[0] + uhi[0]) / 2, -0.75, 0.05), 0.09)
    for x, fig, (lo, hi) in zip(xs[2:], figs, items[2:]):
        instantiate(fig, where, location=(x, 0.0, 0.0), turn=turn, light=False)
        uc.label(TITLES[fig.key], (x + (lo + hi) / 2, -0.75, 0.05), 0.09)
    uc.studio(where)
    uc.sun((-0.45, -0.85, 0.42), strength=3.8)
    width = total + 0.6
    cam = uc.camera((0.0, -30.0, 1.4 + 30.0 * math.tan(math.radians(7.0))), (0.0, 0.0, 1.4), ortho=width)
    render(os.path.join(OUT, 'Abel_compare.png'), (2600, int(2600 * 3.1 / width)), 64)


def main():
    keys = [a for a in ARGV if a in OPTIONS] or list(OPTIONS)
    if 'debug' in ARGV:
        for key in keys:
            if key in FIGURES:
                shot_debug(key)
        paths = [os.path.join(WORK, f'debug_{k}_{v}.png') for k in keys for v in ('front', 'three', 'side')]
        uc.compose([p for p in paths if os.path.exists(p)], os.path.join(WORK, 'debug_strip.png'))
        return
    shots = [a for a in ARGV if a in SHOTS] or list(SHOTS)
    for key in keys:
        for shot in shots:
            if shot in ('dusk', 'day'):
                shot_hero(key, shot)
            elif shot == 'turn':
                shot_turnaround(key)
            elif shot == 'face':
                shot_face(key)
            elif shot == 'kneel':
                shot_kneel(key)
    if 'compare' in shots:
        shot_compare()


if __name__ == '__main__':
    main()
