"""The Foreman, Cyrus Teague (AForemanCreature; Docs/Areas/RansomsRest_Expansion.md, section 6), as the user picked him
on 2026-10-13: option A of Art/Backlog/Creatures/ForemanConcepts.py, "the company man". A mouse-brown overcoat to the
knee buttoned over a black waistcoat, its velvet collar turned up round a stiff white collar and black tie; a bowler with
a carbide lamp; the pick handle carried like a club in his right fist; a canvas satchel of fuse sticks at his left hip on
a strap over his right shoulder; the Unpaid's hunger on a heavy, jowled company face under a black walrus moustache; the
gold coal of a Soulfed Legendary burning through his left breast; the shroud below the coat.

SK_Foreman is built on Unpaid.py's own code (run up to its build, as SK_DeadMiner is) at the rig's size: the actor
scales him to 1.5 times an Unpaid (Foreman::Size). The Unpaid's bones and rest (the code's idle baked in), so the dead
miner's rig code poses him unchanged; his hands are bigger (1.14x) and the right one is a fist round the handle; and he
has Abel's skirt bones for the coat's long skirts, and the dead miner's 'pick' bone at the fist's grip.

Models (in /Game/Art/Creatures):
  SK_Foreman           about 12k triangles, LODs at 50% and 25% (no Nanite); slots GhostForeman (M_Ghost: skin and
                       shroud, the white collar and shirt, the coat, black for the waistcoat, tie, velvet collar, hair,
                       moustache and the bowler) and GhostCoal (shared; the code sets the Soulfed gold).
  SM_ForemanBowler     on the 'hat' bone as SM_UnpaidHat is (its pivot the bone's head, modeled as it sits at rest);
                       slot GhostForeman; SOCKET_Lamp on the crown's front, its front (-Y; Unreal +X) the lamp's forward.
  SM_ForemanLamp       his carbide lamp (the miners', a bigger reflector): its pivot at its mount, the lens along -Y
                       (Unreal +X), SOCKET_Light at the burner. Slot ForemanLamp.
  SM_ForemanPickHandle the pick's hickory handle, its head long gone, two iron bands at the eye end: its pivot where his
                       fist grips it, the handle along +Z (as SM_DeadMinerPick, so the code's UpdatePick holds it the same
                       way). Slot ForemanHandle.
  SM_ForemanSatchel    the powder satchel and its strap, rigid on spine_02 (the chest): its pivot that bone's head,
                       modeled where it hangs at rest (attach it turned by the inverse of the bone's rest rotation, as the
                       code does). The bag at the left hip, its flap thrown back, five fuse sticks standing in it; the
                       strap across his chest, over his right shoulder and down his back. Slot ForemanSatchel.
  SM_ForemanFuseStick  a stick of blasting powder in red paper, 20 cm: its pivot at its middle, the fuse end along +Z;
                       SOCKET_Fuse at the fuse's tip (the sparks). Slot ForemanSatchel.
The parts' rest places are logged as PART lines and written to Intermediate/ForemanModel/Foreman_parts.json.

Bones: SK_Unpaid's (pelvis, spine_01, spine_02, coal, neck, head, jaw, hat, upperarm/lowerarm/hand_l and _r with
thumb/index/middle/ring/pinky_01_l and _r, tail_01..05, tail_l_01/02, tail_r_01/02), plus 'pick' under hand_r (the
fist's grip, along the handle) and Abel's skirt bones under the pelvis: skirt_f_01 (hips to knees, the front), skirt_f_02
(knees to hem), skirt_l_01 and skirt_r_01 (the sides, hips to knees). Unposed, they ride the pelvis.

Hit zones (PA_Foreman): a hull on pelvis, spine_01, spine_02, neck, head, jaw, each upper arm, forearm and hand (the
fingers as the code curls them), tail_01, tail_02 and the four skirt bones; a 7 cm sphere on the coal.

    powershell -NoProfile -File Tools\\artrun.ps1 -Script Art\\Models\\Creatures\\Foreman.py -Preview
Previews go to Saved/ArtPreviews/RansomsRest/Foreman/.
"""
import math
import os
import sys

import bpy
import numpy as np
from mathutils import Matrix, Vector

import looter_ghosts as lg
import looter_miners as mn
import looter_model as lm
import looter_textures as lt

smoothstep, gauss, unit, rot_x, rot_y, wrap = lg.smoothstep, lg.gauss, lg.unit, lg.rot_x, lg.rot_y, lg.wrap
SIDES, SUFFIX = lg.SIDES, lg.SUFFIX
SKIN, SHIRT, COAT_ZONE, ACCENT = 0, 1, 2, 3
ARGV = [a for arg in (sys.argv[sys.argv.index('--') + 1:] if '--' in sys.argv else []) for a in arg.split(',') if a]
OUT = os.path.join(lt.REPO, 'Saved', 'ArtPreviews', 'RansomsRest', 'Foreman')
WORK = os.path.join(lt.REPO, 'Intermediate', 'ForemanModel')
SCALE = 1.5                                  # the actor's size: he is built at 1
SOULFED = 0xffcc00                           # the Soulfed Legendary's coal (CreatureRankSettings), for the previews only

BODY_COLORS = ('#BAC4C6', '#D9D4C7', '#5E5243', '#2E2B2B')     # ghost, white collar, mouse-brown coat, black
LAMP_COLORS = ('#D6D3CA', '#C2A66E', '#5F6264', '#3A3230')     # nickel reflector, brass, iron, leather
HANDLE_COLORS = ('#D8D4CC', '#9C8668', '#6F767B', '#C9CBC6')   # worn steel, hickory, iron bands
SATCHEL_COLORS = ('#8F7F62', '#A9533F', '#3A3532', '#C2A36B')  # canvas and leather, red paper, fuse, brass
TRIANGLES = dict(shirt=600, waistcoat=520, coat=2700, sleeve=380, head=2400, arm=700)
HAND = dict(scale=1.14, length=1.12, thin=1.1, knuckle=1.45, claw=0.2)    # big working hands
HAND_SIZE = 1.08                             # how much farther out of the wrist they grip than the Unpaid's
PICK_GRIP = 0.12                             # the fist holds the handle this far over its butt
HANDLE_LENGTH = 0.86


def log(message):
    print(f'FOREMAN: {message}', flush=True)


# --- Unpaid.py's own code, with his head's shape, his hands and his shroud ---

U = mn.unpaid_library()
mn.fist_right(U, **HAND)
U['TRIANGLES'] = dict(U['TRIANGLES'], head=TRIANGLES['head'], arm=TRIANGLES['arm'])
# The face: the Unpaid's hunger on a heavy, jowled company face; deep sockets, a big nose, the jaw hanging.
U['HEAD_D'] = dict(width=0.154, front=0.099, back=0.108, top=0.12, bottom=0.148, mouth=-0.076, brow=0.017, brow_shelf=0.007,
                   socket=0.022, eyeball=0.0068, nose=0.036, cheek=0.007, zyg=0.006, hollow=0.011, nasolabial=0.0034,
                   lips=0.003, lips_thin=0.0012, chin=0.01, jaw_angle=0.009, nape=0.2, jaw_taper=0.05, socket_dark=0.97,
                   temple=0.009, asym=0.1)
# The shroud from under the knee-length coat (Abel's path and size), torn into more strips below its hem.
U['SHROUD_PATH'] = [(0.0, 0.0, 0.92), (0.0, 0.06, 0.6), (0.0, 0.2, 0.36), (0.0, 0.48, 0.2), (0.0, 0.85, 0.12),
                    (0.0, 1.25, 0.08)]
U['SHROUD_SIZE'] = [(0.0, 0.17, 0.12), (0.25, 0.17, 0.13), (0.5, 0.12, 0.1), (0.75, 0.07, 0.06), (1.0, 0.03, 0.03)]
_shroud = U['shroud']
U['shroud'] = lambda **kw: _shroud(**dict(dict(rows=15, cols=30, strips=9, split=0.3, ends=(0.76, 1.0), seed=5,
                                               fade_from=0.32), **kw))
hunch, hunched = U['hunch'], U['hunched']
surface = lg.surface

# His frame: a broad, heavy man's chest and shoulders (Amos's B torso) on the Unpaid's line of the neck, so the Unpaid's
# head and neck sit on it.
TORSO_B = [(0.90, 0.172, 0.118, 0.000, 2.2), (0.98, 0.168, 0.116, -0.004, 2.2), (1.06, 0.164, 0.114, -0.008, 2.2),
           (1.16, 0.168, 0.116, -0.010, 2.3), (1.26, 0.180, 0.122, -0.010, 2.4), (1.34, 0.192, 0.125, -0.010, 2.6),
           (1.40, 0.199, 0.119, -0.006, 2.8), (1.45, 0.197, 0.107, -0.002, 3.2), (1.475, 0.185, 0.099, 0.000, 3.4),
           (1.50, 0.156, 0.091, 0.000, 3.0), (1.52, 0.116, 0.081, -0.002, 2.6), (1.54, 0.079, 0.071, -0.004, 2.2)]
TORSO = [(z, w, d, yd, p) for (z, w, d, _, p), (_, _, _, yd, _) in zip(TORSO_B, U['TORSO_D'])]
SHOULDER = {s: np.array([s * 0.178, -0.022, 1.436]) for s in SIDES}
COAL_AT = (0.28, 1.33)
# The overcoat: to the knee, its skirts flaring, the back fuller than the front.
LOWER = [(0.36, 0.262, 0.216, 0.06, 2.0), (0.5, 0.25, 0.2, 0.045, 2.0), (0.65, 0.232, 0.178, 0.028, 2.1),
         (0.8, 0.21, 0.152, 0.012, 2.2), (0.92, 0.19, 0.134, 0.004, 2.2), (1.0, 0.178, 0.124, 0.0, 2.2),
         (1.06, 0.172, 0.12, -0.004, 2.2)]
COAT = LOWER + [r for r in TORSO if r[0] > 1.06]
HEM = 0.42
# Abel's skirt bones (Abel.py): the front from the hips to the knees and on to the hem, the sides to the knees.
SKIRT_HINGE = np.array([0.0, -0.06, 0.95])
SKIRT_KNEE = np.array([0.0, -0.13, 0.55])
SKIRT_HEM = np.array([0.0, -0.15, 0.42])
SKIRT_SIDE_HIP = np.array([0.17, 0.004, 0.95])
SKIRT_SIDE_KNEE = np.array([0.2, 0.03, 0.55])


def torso_weights(part, V, shoulders=True):
    """The spine's share by height in the upright frame the cloth was built in (lg.torso_weights)."""
    return lg.torso_weights(part, V[:, 2], V[:, 0], shoulders=shoulders)


def skirt_weights(part, V):
    """Abel's: the coat below the hips, the front and sides on the skirt chain (thighs to the knee, then to the hem), the
    back on tail_01 with the shroud, all of it on the pelvis at the waist."""
    theta = np.arctan2(V[:, 0], -V[:, 1])
    z = V[:, 2]
    down = smoothstep(1.0, 0.86, z)
    front = smoothstep(2.3, 1.95, np.abs(theta))
    sides = smoothstep(math.radians(70.0), math.radians(95.0), np.abs(theta))
    knee = np.maximum(smoothstep(0.6, 0.5, z), sides * smoothstep(0.8, 0.66, z))
    side = smoothstep(math.radians(70.0), math.radians(100.0), np.abs(theta))
    thigh = down * front * (1.0 - knee)
    part.weigh('pelvis', 1.0 - down)
    part.weigh('skirt_f_01', thigh * (1.0 - side))
    part.weigh('skirt_l_01', thigh * side * (V[:, 0] >= 0.0))
    part.weigh('skirt_r_01', thigh * side * (V[:, 0] < 0.0))
    part.weigh('skirt_f_02', down * front * knee)
    part.weigh('tail_01', down * (1.0 - front))
    return part


def body_weights(part, V):
    """Cloth from the chest to the hem: the torso's weights above the hips, the skirt's below (Abel's)."""
    below = smoothstep(1.06, 0.98, V[:, 2])
    upper = torso_weights(lg.Part('_', (V, []), 0), V)
    lower = skirt_weights(lg.Part('_', (V, []), 0), V)
    for bone, w in upper.weights.items():
        part.weigh(bone, w * (1.0 - below))
    for bone, w in lower.weights.items():
        part.weigh(bone, w * below)
    return part


def upright(part):
    """A part built upright, hunched as the Unpaid's frame is (its weights already taken upright)."""
    part.V = hunch(part.V)
    return part


# --- The clothes ---

def build_shirt():
    """The shirt under it all: the hollow chest and the coal's crater (the Unpaid's), its stiff white collar and the
    black tie in the waistcoat's V; the coal."""
    parts = []
    theta = np.linspace(0.0, 2.0 * np.pi, 96, endpoint=False)
    z = np.linspace(1.12, 1.54, 30)
    P = surface(TORSO, z, theta, 0.004)
    dense = lg.closed_loft(P)
    tc, zc = COAL_AT
    pc = surface(TORSO, [zc], np.array([tc]), 0.004)[0, 0]
    nc = lg.outward(tc)
    dense = lg.push(dense, pc, 0.07, -0.022)
    dist = np.linalg.norm(dense[0] - pc, axis=1)
    jitter = 0.5 + 0.5 * lg.pnoise(dense[0], 60.0, 5)
    d_cav = smoothstep(0.06, 0.035, dist) * 0.9
    d_ember = smoothstep(0.03, 0.05, dist) * smoothstep(0.085, 0.062, dist) * (0.4 + 0.6 * jitter)
    low = lg.reduce(dense, TRIANGLES['shirt'])
    near = lg.nearest(dense[0], low[0])
    shirt = lg.Part('Shirt', low, SHIRT, cavity=d_cav[near], ember=d_ember[near])
    parts.append(upright(torso_weights(shirt, low[0])))
    coal_center = hunched(pc - nc * 0.012)
    coal_normal = unit(hunched(pc + nc * 0.05) - hunched(pc))
    # The stiff collar round the Unpaid's neck, standing above the coat's.
    th = np.linspace(0.0, 2.0 * np.pi, 32, endpoint=False)
    rings = np.array([np.stack([rr * np.sin(th), -0.046 - rr * 0.92 * np.cos(th), np.full_like(th, zz)], -1)
                      for zz, rr in ((1.525, 0.056), (1.56, 0.0545), (1.592, 0.053))])
    collar = lg.Part('StiffCollar', (rings.reshape(-1, 3), [lg.grid_faces(3, 32)]), SHIRT, cavity=0.05)
    collar.weigh('neck', 0.5).weigh('spine_02', 0.5)
    parts.append(upright(collar))
    # The tie: its knot at the collar's front, its blade down the shirt to the waistcoat.
    knot = lg.ellipsoid((0.0, -0.1, 1.552), (0.013, 0.009, 0.012), segs=10, rings=6)
    parts.append(upright(lg.Part('Tie', knot, ACCENT, cavity=0.2).weigh('neck', 0.4).weigh('spine_02', 0.6)))
    path = [np.array([0.0, -0.104, 1.54])] + [surface(TORSO, [zr], np.array([0.0]), 0.012)[0, 0] for zr in (1.5, 1.44, 1.38)]
    blade = lg.ribbon(path, [(0.0, 0.009), (0.5, 0.014), (1.0, 0.017)], n=8, cols=3, out=(0.0, -1.0, 0.25), curl=0.15)
    tie = lg.Part('Tie', blade, ACCENT, cavity=0.1)
    parts.append(upright(torso_weights(tie, blade[0], shoulders=False)))
    # The coal, burning where the heart was.
    V, F = lg.ellipsoid(coal_center, (0.056, 0.056, 0.052), segs=12, rings=8)
    V = V + (V - coal_center) * (0.16 * lg.pnoise(V, 30.0, 3))[:, None]
    parts.append(lg.Part('Coal', (V, F), SKIN, slot=1).weigh('coal', 1.0))
    return parts, (coal_center, coal_normal)


def build_waistcoat():
    """The black waistcoat buttoned up under the overcoat, its V open over the shirt and tie; burnt through at the coal.
    Its back is under the coat and left out."""
    def folds(th, zz):
        return 0.012 + 0.002 * np.cos(9 * th) * smoothstep(1.2, 1.0, zz)

    def sd(th, zz):
        at = np.abs(th)
        hem = 0.98 - 0.04 * np.exp(-((at - 0.15) / 0.12) ** 2)
        out = np.maximum(hem - zz, np.where(zz > 1.3, (0.03 + 0.9 * (zz - 1.3)) - at, -1.0))
        out = np.maximum(out, lg.armhole(th, zz, 1.39, 0.36, 0.09))
        out = np.maximum(out, at - 2.0)
        return np.maximum(out, zz - 1.5)
    (V, F), th, zz, edge = lg.shell(TORSO, 0.92, 1.5, sd, folds, rows=70, cols=130, hole=0.068, coal_at=COAL_AT)
    ember = lg.scorch(edge, V, heat=0.65)
    cav = 0.55 * smoothstep(0.025, 0.0, edge)
    low = lg.reduce((V, F), TRIANGLES['waistcoat'])
    near = lg.nearest(V, low[0])
    part = lg.Part('Waistcoat', low, ACCENT, ember=ember[near], cavity=cav[near])
    return [upright(torso_weights(part, low[0], shoulders=False))]


def coat_sd(th, zz):
    """The overcoat's cut: buttoned from the waist to the chest, the V and lapels above, its skirts parting below the
    waist, a back vent below the seat, armholes; to the knee."""
    at = np.abs(th)
    out = HEM + 0.008 * np.sin(5 * th + 0.4) - zz
    front = np.where(zz > 1.36, 0.08 + 1.8 * (zz - 1.36), np.where(zz > 1.0, 0.045, 0.045 + 0.5 * (1.0 - zz)))
    vent = np.where(zz < 0.8, (0.012 + 0.07 * (0.8 - zz)) - np.abs(wrap(th - np.pi)), -1.0)
    out = np.maximum(np.maximum(out, front - at), vent)
    out = np.maximum(out, lg.armhole(th, zz, 1.385, 0.26, 0.062))
    return np.maximum(out, zz - 1.548)


def build_coat(shoulders):
    """The overcoat: its shell (burnt through at the coal), buttons, flap pockets, the velvet collar and the lapels;
    the sleeves to the wrists with turned cuffs."""
    parts = []
    rng = np.random.default_rng(11)
    amps, phases = rng.uniform(0.5, 1.0, 5), rng.uniform(0.0, 2.0 * np.pi, 5)

    def folds(th, zz):
        f = sum(a * np.cos(k * th + p) for a, k, p in zip(amps, (6, 9, 13, 17, 23), phases)) / amps.sum()
        skirt = 0.014 * smoothstep(1.0, 0.55, zz) * (f - 0.3 * np.abs(f))
        chest = 0.002 * np.cos(11 * th) * smoothstep(1.1, 1.3, zz)
        return 0.02 + skirt + chest
    (V, F), th, zz, edge = lg.shell(COAT, HEM - 0.06, 1.548, coat_sd, folds, rows=150, cols=220, hole=0.072,
                                    coal_at=COAL_AT)
    ember = lg.scorch(edge, V, heat=0.65)
    seam = 0.3 * gauss(np.abs(th) - 0.045, 0.01) * (zz < 1.36) + 0.2 * gauss(zz - HEM - 0.02, 0.005)
    cav = np.maximum(0.55 * smoothstep(0.025, 0.0, edge), seam)
    low = lg.reduce((V, F), TRIANGLES['coat'])
    near = lg.nearest(V, low[0])
    coat = lg.Part('Coat', low, COAT_ZONE, ember=ember[near], cavity=cav[near])
    parts.append(upright(body_weights(coat, low[0])))
    # Buttons down the closed front.
    pieces = [lg.ellipsoid(surface(COAT, [zb], np.array([0.05]), grow=0.027)[0, 0], (0.0085, 0.0085, 0.005),
                           np.array([[0, 0, 1.0], [1.0, 0, 0], [0, -1.0, 0]]), 8, 4) for zb in (1.34, 1.25, 1.16, 1.07)]
    buttons = lg.Part('Buttons', lg.merge(pieces), COAT_ZONE, cavity=0.4)
    parts.append(upright(torso_weights(buttons, buttons.V, shoulders=False)))
    # Flap pockets at the hips.
    for s in SIDES:
        t0, t1 = sorted((s * 0.62, s * 1.08))
        ts, zs = np.linspace(t0, t1, 7), np.linspace(0.9, 0.945, 3)
        P = surface(COAT, zs, ts, 0.03)
        Tg, Zg = np.meshgrid(np.linspace(0, 1, 7), np.linspace(0, 1, 3))
        rim = smoothstep(0.25, 0.0, np.minimum(np.minimum(Tg, 1 - Tg) * 2.0, Zg))
        part = lg.Part('Pocket', (P.reshape(-1, 3), [lg.grid_faces(3, 7, closed=False)]), COAT_ZONE,
                       cavity=(0.55 * rim).ravel())
        parts.append(body_weights(part, part.V))
    # The velvet collar turned up and back round his neck, and the lapels laid back along the V.
    ang = np.linspace(-np.pi, np.pi, 48, endpoint=False)
    rows = [(1.555, 0.096, 0.0), (1.537, 0.112, 0.0), (1.505, 0.133, 0.012), (1.468, 0.152, 0.03)]
    rings = np.array([np.stack([r0 * np.sin(ang), -0.044 - r0 * 0.96 * np.cos(ang),
                                z0 - drop * 2.2 * np.cos(ang).clip(0.0, None) ** 2], -1) for z0, r0, drop in rows])
    Vc, Fc, used = lg.cut(rings, np.broadcast_to((0.5 - np.abs(ang))[None, :], rings.shape[:2]) * 0.1)
    part = lg.Part('VelvetCollar', (Vc, Fc), ACCENT, cavity=0.12)
    parts.append(upright(part.weigh('spine_02', 0.75).weigh('neck', 0.25)))
    pieces = []
    for s in SIDES:
        zl = np.linspace(1.36, 1.5, 10)
        path = [surface(COAT, [zr], np.array([s * (0.08 + 1.8 * (zr - 1.36) + 0.05)]), grow=0.03)[0, 0] for zr in zl]
        pieces.append(lg.ribbon(path, [(0.0, 0.006), (0.5, 0.034), (1.0, 0.026)], n=12, cols=3,
                                out=lambda C: lg.outward(np.arctan2(C[:, 0], -C[:, 1])), curl=0.1))
    lapels = lg.Part('Lapels', lg.merge(pieces), COAT_ZONE, cavity=0.1)
    parts.append(upright(torso_weights(lapels, lapels.V)))
    # The sleeves, full to the wrists over the Unpaid's thin arms, turned cuffs.
    for k, s in enumerate(SIDES):
        sfx = SUFFIX[s]
        E, W = U['bent'](s)[:2]
        S = shoulders[s] + np.array([-s * 0.006, 0.0, 0.012])
        piece, line, at_elbow = lg.sleeve_piece(S, E, W, (0.074, 0.066, 0.058), 41 + k, folds=0.07)
        low = lg.reduce(piece, TRIANGLES['sleeve'])
        part = lg.Part(f'Sleeve_{sfx}', low, COAT_ZONE)
        parts.append(lg.weigh_sleeve(part, low[0], line, at_elbow, sfx))
        dv = unit(W - E)
        cuff = lg.torus(W - dv * 0.05, dv, 0.058, 0.014, segs=16, sides=6, squash=1.4)
        parts.append(lg.Part(f'Cuff_{sfx}', cuff, COAT_ZONE, cavity=0.15).weigh(f'lowerarm_{sfx}', 1.0))
    return parts


def build_moustache(head):
    """The walrus moustache: two heavy drooping wings from under the nose over the upper lip and past the mouth's
    corners, lying on the face (the Unpaid's head surface for this face, as built)."""
    spec = U['HEAD_D']
    (V, F), cav, jw = U['head_surface'](spec)
    jw[-1] = 0.5
    V = U['jaw_turn'](V, jw, U['JAW_BIND'])
    dirs = V / np.linalg.norm(V, axis=1, keepdims=True)
    zm = spec['mouth']
    parts = []
    for s in SIDES:
        keys = [(s * 0.002, zm + 0.017), (s * 0.017, zm + 0.012), (s * 0.031, zm + 0.002), (s * 0.039, zm - 0.014),
                (s * 0.042, zm - 0.03)]
        pts = []
        for k, (x, zz) in enumerate(keys):
            want = unit(np.array([x, -0.09, zz]))
            p = V[int(np.argmax(dirs @ want))]
            pts.append(p * (1.0 + (0.007 - 0.002 * k / 4.0) / np.linalg.norm(p)))
        C = lg.spline(pts, 9)
        t = np.linspace(0.0, 1.0, 9)
        depth = np.interp(t, [0.0, 0.35, 1.0], [0.0065, 0.0075, 0.0025])
        height = np.interp(t, [0.0, 0.3, 0.7, 1.0], [0.011, 0.0125, 0.008, 0.003])
        piece = lg.tube(C + head['center'], depth, height, segs=8, up=(0.0, -1.0, 0.0), domes=2, pointed=0.3)
        part = lg.Part('Moustache', piece, ACCENT, cavity=0.35 * np.ones(len(piece[0])))
        parts.append(part.weigh('head', 1.0))
    return parts


# --- The rig ---

def materials():
    body = lg.ghost_slot('GhostForeman', BODY_COLORS, **lg.GHOST_LOOK)
    coal = lg.coal_slot()
    lamp = lg.ghost_slot('ForemanLamp', LAMP_COLORS, **dict(lg.PROP_LOOK, RimStrength=1.2, GlowStrength=0.14))
    handle = lg.ghost_slot('ForemanHandle', HANDLE_COLORS, **lg.PROP_LOOK)
    satchel = lg.ghost_slot('ForemanSatchel', SATCHEL_COLORS, **lg.PROP_LOOK)
    return dict(body=body, coal=coal, lamp=lamp, handle=handle, satchel=satchel)


def build_rig(mats):
    shoulders = {s: hunched(SHOULDER[s]) for s in SIDES}
    shirt_parts, coal = build_shirt()
    parts = shirt_parts + build_waistcoat() + build_coat(shoulders)
    arm_parts, fingers, old_fingers = U['build_arms']()
    head_parts, head = U['build_head']()
    head['hat'] = head['pivot'] + np.array([0.0, 0.0, 0.153])       # his bowler sits on a bigger head
    shroud_parts, tail = U['build_shroud']()
    for p in head_parts:
        if p.name == 'Hair':           # cropped: each of the Unpaid's locks drawn in toward its root (its first row)
            root = p.V[:3].mean(0)
            p.V = root + (p.V - root) * 0.5
    parts += [mn.from_unpaid(p, ACCENT if p.name == 'Hair' else None) for p in arm_parts + head_parts + shroud_parts]
    parts += build_moustache(head)
    rig = lm.armature()
    specs = U['bone_specs'](dict(coal=coal, shoulders=shoulders), old_fingers, head, tail)
    specs.append(('skirt_f_01', tuple(SKIRT_HINGE), tuple(SKIRT_KNEE), 'pelvis'))
    specs.append(('skirt_f_02', tuple(SKIRT_KNEE), tuple(SKIRT_HEM), 'skirt_f_01'))
    for s in SIDES:
        mirror = np.array([s, 1.0, 1.0])
        specs.append((f'skirt_{SUFFIX[s]}_01', tuple(SKIRT_SIDE_HIP * mirror), tuple(SKIRT_SIDE_KNEE * mirror), 'pelvis'))
    lm.bones(rig, [(n, tuple(map(float, h)), tuple(map(float, t)), p) for n, h, t, p in specs])
    U['bake_idle'](rig, fingers)
    G, haft, point = mn.fist_grip(U, -1.0, HAND_SIZE)
    mn.add_bone(rig, 'pick', G, G + haft * 0.1, 'hand_r')
    body, arrays, layout = lg.skinned(rig, 'Foreman', parts, [mats['body'], mats['coal']])
    lg.triangle_report(log, 'Foreman', body, parts)
    return rig, body, head, arrays, layout, parts


SKIRTS = ('skirt_f_01', 'skirt_f_02', 'skirt_l_01', 'skirt_r_01')


def hit_zones(rig, V, faces, W, names):
    """The Unpaid's zones and a hull on each skirt bone; the hands' hulls wrap the fingers as the code curls them."""
    zones = lg.HitZones(U['HULL_BONES'] + SKIRTS, U['NO_HULL'] + ('pick',), U['COAL_REACH'])
    owner = np.array([zones.owner_of(names[i]) for i in np.argmax(W, axis=1)])
    extra = {}
    for sfx in ('l', 'r'):
        mine = owner == f'hand_{sfx}'
        extra[f'hand_{sfx}'] = np.vstack([U['curled'](rig, V[mine], W[mine], names, sfx, c, s)
                                          for c, s in U['HAND_HULL_POSES']])
    zones.build(rig, V, faces, W, names, log, extra)
    return zones


# --- The props ---

def bowler_parts():
    """The bowler in hat space (origin at the brim's middle, front -Y): a hard round crown, a narrow brim curled up at
    the sides and dipping front and back, its edge rolled, a grosgrain band. All black (the accent zone)."""
    cols = 32
    th = np.linspace(0.0, 2.0 * np.pi, cols, endpoint=False)
    zc = [0.0, 0.03, 0.058, 0.082, 0.1, 0.111, 0.117]
    rc = [0.089, 0.092, 0.092, 0.086, 0.072, 0.051, 0.025]
    rings = np.array([np.stack([rr * np.sin(th), -rr * np.cos(th) * 1.1, np.full_like(th, zz)], -1) for zz, rr in zip(zc, rc)])
    n = len(zc) * cols
    crown = lg.Part('Crown', (np.vstack([rings.reshape(-1, 3), [[0.0, 0.0, 0.119]]]),
                              [lg.grid_faces(len(zc), cols), lg.fan(n, np.arange(cols) + n - cols, top=True)]), ACCENT,
                    cavity=np.concatenate([0.1 * smoothstep(0.03, 0.0, rings.reshape(-1, 3)[:, 2]), [0.0]]))
    q = np.linspace(0.0, 1.0, 4)[:, None]
    r = 0.089 + 0.044 * q
    lift = 0.03 * q ** 2.2 * np.sin(th) ** 2 - 0.006 * q ** 2 * np.cos(th) ** 2
    top = np.stack([r * np.sin(th), -r * np.cos(th) * 1.09, lift], -1)
    brim = mn.two_sided_brim(top, 0.005, ACCENT, cavity_edge=0.15)
    edge = top[-1]
    ang = np.linspace(0.0, 2.0 * np.pi, 5, endpoint=False)
    out = np.stack([np.sin(th), -np.cos(th) * 1.09, np.zeros_like(th)], -1)
    out = out / np.linalg.norm(out, axis=1, keepdims=True)
    tube = edge[:, None, :] + (out[:, None, :] * np.cos(ang)[None, :, None] + np.array([0.0, 0.0, 1.0]) * np.sin(ang)[None, :, None]) * 0.0045 \
        + np.array([0.0, 0.0, 0.002])
    i, j = np.meshgrid(np.arange(cols), np.arange(5), indexing='ij')
    F = np.stack([i * 5 + j, ((i + 1) % cols) * 5 + j, ((i + 1) % cols) * 5 + (j + 1) % 5, i * 5 + (j + 1) % 5], -1).reshape(-1, 4)
    rolled = lg.Part('BrimRoll', (tube.reshape(-1, 3), [F]), ACCENT, cavity=0.1)
    band = np.array([np.stack([0.0925 * np.sin(th), -0.0925 * np.cos(th) * 1.1, np.full_like(th, z)], -1)
                     for z in (0.002, 0.013, 0.024)])
    band = lg.Part('Band', (band.reshape(-1, 3), [lg.grid_faces(3, cols)]), ACCENT, cavity=0.55)
    return [crown, brim, rolled, band], (np.array([0.0, -0.103, 0.047]), 2.0)


HAT_TILT = (-3.0, 2.0)


def build_bowler(mats, head):
    parts, (mount, pitch) = bowler_parts()
    Rh = rot_x(HAT_TILT[0]) @ rot_y(HAT_TILT[1])
    pivot = np.asarray(head['hat'], float)
    for p in parts:
        p.V = p.V @ Rh.T + pivot
    obj = lg.static_prop('ForemanBowler', parts, [mats['body']], pivot)
    R = Rh @ rot_x(pitch)
    at = pivot + Rh @ mount
    lg.socket(obj, 'Lamp', at, R)
    log(f'SM_ForemanBowler: {lg.triangles_of(obj)} triangles; SOCKET_Lamp {tuple(np.round(lg.to_ue(at - pivot), 2))} cm '
        f'from its pivot')
    return obj, (at, R)


def build_handle(mats):
    parts = mn.pick_parts(head=False, grip=PICK_GRIP, length=HANDLE_LENGTH, thick=1.15, bands=True)
    obj = lg.static_prop('ForemanPickHandle', parts, [mats['handle']], (0.0, 0.0, 0.0))
    log(f'SM_ForemanPickHandle: {lg.triangles_of(obj)} triangles')
    return obj


STRAP = [(1.32, 0.99), (0.9, 1.08), (0.45, 1.18), (0.0, 1.27), (-0.45, 1.4), (-0.95, 1.52), (-1.5708, 1.56), (-2.3, 1.49),
         (-2.9, 1.32), (-3.4, 1.17), (-3.75, 1.06), (-4.05, 0.99)]
BAG_AT = dict(theta=1.85, z=0.99, grow=0.06, half=0.118)


def stick_parts(base, d, length, rng, fuse_out):
    """A fuse stick from base along d: red paper, its fuse curling out of the top. Returns parts and the fuse's tip."""
    top = base + d * length
    paper = lg.Part('Stick', lg.tube([base, top], 0.0165, segs=10, domes=1), 1, cavity=0.05)
    side = unit(np.cross(d, fuse_out))
    tip = top + d * 0.03 + fuse_out * 0.022 + side * rng.uniform(-0.01, 0.01)
    fuse = lg.tube(lg.spline([top - d * 0.004, top + d * 0.018 + fuse_out * 0.004, tip], 7), 0.0026, segs=5, domes=1)
    return [paper, lg.Part('Fuse', fuse, 2)], tip


def satchel_parts():
    """The powder satchel in rig space at rest (hunched as he is): the canvas bag at his left hip, its flap thrown back
    over its top edge, two brass buckles, five sticks standing in it; the strap from its front corner across his chest,
    over his right shoulder and down his back to its back corner."""
    th = BAG_AT['theta']
    origin = surface(COAT, [BAG_AT['z']], np.array([th]), BAG_AT['grow'])[0, 0]
    y = -lg.outward(th)
    zax = np.array([0.0, 0.0, 1.0])
    x = np.cross(y, zax)
    M = np.column_stack([x, y, zax])
    parts = []
    a = np.linspace(0.0, 2.0 * np.pi, 24, endpoint=False)
    rings = []
    for k, zr in enumerate(np.linspace(-0.21, 0.0, 6)):
        w = BAG_AT['half'] + 0.005 * math.sin(k * 1.3)
        dd = 0.05 - 0.01 * smoothstep(-0.05, 0.0, zr)
        c, s = np.cos(a), np.sin(a)
        rings.append(np.stack([w * np.sign(c) * np.abs(c) ** 0.5, -0.052 + dd * np.sign(s) * np.abs(s) ** 0.5,
                               np.full_like(a, zr)], -1))
    V, F = lg.orient(lg.closed_loft(np.array(rings)))
    parts.append(lg.Part('Bag', (V, F), 0, cavity=0.15 * (np.abs(V[:, 0]) > BAG_AT['half'] - 0.01)))
    qf = np.linspace(0.0, 1.0, 5)[:, None]
    xs = np.linspace(-BAG_AT['half'], BAG_AT['half'], 8)[None, :]
    flap = np.stack([xs + 0.0 * qf, -0.004 + 0.008 * qf + 0.0 * xs, 0.008 - 0.1 * qf + 0.0 * xs], -1)
    parts.append(lg.Part('Flap', (flap.reshape(-1, 3), [lg.grid_faces(5, 8, closed=False)]), 0, cavity=0.12))
    for bx in (-0.065, 0.065):
        parts.append(lg.Part('Buckle', lg.box_piece((bx, -0.104, -0.08), (0.012, 0.003, 0.009)), 3, cavity=0.2))
        parts.append(lg.Part('Tab', lg.box_piece((bx, -0.103, -0.11), (0.011, 0.0025, 0.035)), 0, cavity=0.3))
    rng = np.random.default_rng(3)
    for k in range(5):
        bx = (k - 2) * 0.042 + rng.uniform(-0.005, 0.005)
        d = unit(np.array([rng.uniform(-0.15, 0.15), rng.uniform(-0.15, 0.05), 1.0]))
        sp, _ = stick_parts(np.array([bx, -0.05, -0.12]), d, 0.17 + rng.uniform(0.0, 0.04), rng,
                            unit(np.array([rng.uniform(-1, 1), -0.6, 0.0])))
        parts += sp
    for p in parts:
        p.V = p.V @ M.T + origin
    # The strap on his coat, from the bag's front corner round to its back corner (hunched with him).
    pts = [surface(COAT, [z], np.array([t]), 0.048)[0, 0] for t, z in STRAP]
    pts[0] = origin + M @ np.array([BAG_AT['half'] - 0.02, -0.05, 0.0])
    pts[-1] = origin + M @ np.array([-BAG_AT['half'] + 0.02, -0.05, 0.0])
    strap = lg.ribbon(pts, 0.022, n=72, cols=3, out=lambda C: lg.outward(np.arctan2(C[:, 0], -C[:, 1])), curl=0.02)
    parts.append(lg.Part('Strap', (hunch(strap[0]), strap[1]), 0, cavity=0.1))
    return parts


def build_satchel(mats, rig):
    pivot = np.array(rig.data.bones['spine_02'].head_local)
    obj = lg.static_prop('ForemanSatchel', satchel_parts(), [mats['satchel']], pivot)
    log(f'SM_ForemanSatchel: {lg.triangles_of(obj)} triangles; its pivot spine_02\'s head '
        f'{tuple(np.round(lg.to_ue(pivot), 2))} cm')
    return obj


def build_fuse(mats):
    rng = np.random.default_rng(7)
    parts, tip = stick_parts(np.array([0.0, 0.0, -0.1]), np.array([0.0, 0.0, 1.0]), 0.2, rng, np.array([0.0, -1.0, 0.0]))
    obj = lg.static_prop('ForemanFuseStick', parts, [mats['satchel']], (0.0, 0.0, 0.0))
    lg.socket(obj, 'Fuse', tip)
    log(f'SM_ForemanFuseStick: {lg.triangles_of(obj)} triangles; SOCKET_Fuse {tuple(np.round(lg.to_ue(tip), 2))} cm')
    return obj, tip


# --- Build ---

def build():
    mats = materials()
    rig, body, head, (V, faces, colors, W, names), layout, parts = build_rig(mats)
    bowler = build_bowler(mats, head)
    lamp = mn.build_lamp('ForemanLamp', mats['lamp'], log, reflector=0.058)
    handle = build_handle(mats)
    satchel = build_satchel(mats, rig)
    fuse = build_fuse(mats)
    zones = hit_zones(rig, V, faces, W, names)
    return rig, body, head, bowler, lamp, handle, satchel, fuse, colors, zones


RIG, BODY, HEAD, (BOWLER, LAMP_MOUNT), LAMP, HANDLE, SATCHEL, (FUSE, FUSE_TIP), RAW_COLORS, ZONES = build()
G_PICK, HAFT, POINT = mn.fist_grip(U, -1.0, HAND_SIZE)
PICK_FRAME = mn.tool_frame(HAFT, POINT)


def write_parts():
    G_left = mn.fist_grip(U, 1.0, HAND_SIZE)
    rest = {'SM_ForemanPickHandle (on the pick bone)': mn.part_rest(RIG, G_PICK, PICK_FRAME, 'pick'),
            'SM_ForemanPickHandle (on hand_r)': mn.part_rest(RIG, G_PICK, PICK_FRAME, 'hand_r'),
            'SM_ForemanLamp on SM_ForemanBowler': mn.part_rest(RIG, LAMP_MOUNT[0], LAMP_MOUNT[1], 'hat'),
            'SM_ForemanSatchel': mn.part_rest(RIG, np.array(RIG.data.bones['spine_02'].head_local), np.eye(3), 'spine_02'),
            'SM_ForemanFuseStick (lit in the left fist, fuse up the fist\'s tunnel)':
                mn.part_rest(RIG, G_left[0], mn.tool_frame(G_left[1], G_left[2]), 'hand_l')}
    mn.write_parts(os.path.join(WORK, 'Foreman_parts.json'), rest, log)


write_parts()
TOTAL = sum(lg.triangles_of(o) for o in (BODY, BOWLER, LAMP, HANDLE, SATCHEL))
log(f'the Foreman as drawn (body, bowler, lamp, handle, satchel): {TOTAL} triangles; a fuse stick '
    f'{lg.triangles_of(FUSE)} more')


# --- Previews (Saved/ArtPreviews/RansomsRest/Foreman) ---

# AForemanCreature's fuse (ForemanCreaturePose.cpp): lit in his left fist held up beside his head, the handle low in
# his right; and the dead miner's calm carry.
FUSE_ARMS = {1.0: (mn.ue_dir(0.3, -0.6, 0.75), mn.ue_dir(0.15, -0.25, 0.96)),
             -1.0: (mn.ue_dir(0.3, 0.25, -0.92), mn.ue_dir(0.8, -0.2, 0.55))}
CALM_ARMS = {-1.0: (mn.ue_dir(0.15, 0.28, -0.95), mn.ue_dir(0.2, -0.3, 0.93))}


def want_preview():
    return '--preview' in ARGV or lt.want_preview()


def figure_copy(where, location, turn, arms=None, scale=SCALE, rank=SOULFED, rank_alpha=1.8):
    """A copy of him (no armature) with his bowler, lamp, satchel and handle, at the actor's scale, standing at location
    turned about Z: at rest, or with his arms as the code lays them (arms {side: (upper, fore)}), the handle in his right
    fist and, with the left raised, a lit fuse stick in it."""
    M = mn.figure_matrix(location, turn) @ Matrix.Scale(scale, 4)
    held = {-1.0: (G_PICK, PICK_FRAME)}
    if arms:
        held.update(mn.code_arms(RIG, arms))
        body = mn.posed_copy(BODY, where)
        mn.rest_pose(RIG)
    else:
        body = mn.dup(BODY, where)
        body.modifiers.clear()
    body.matrix_world = M
    mn.preview_look(body, RAW_COLORS, rank=rank, rank_alpha=rank_alpha)
    objs = [body]
    places = [(BOWLER, Matrix.Translation(Vector(BOWLER.location))), (LAMP, mn.frame_matrix(*LAMP_MOUNT)),
              (SATCHEL, Matrix.Translation(Vector(SATCHEL.location))), (HANDLE, mn.frame_matrix(*held[-1.0]))]
    if 1.0 in held:
        places.append((FUSE, mn.frame_matrix(*held[1.0])))
    for src, place in places:
        o = mn.place_copy(src, where, M, place)
        mn.preview_look(o, rank=rank, rank_alpha=rank_alpha)
        objs.append(o)
    return objs


def hide_sources(hidden=True):
    for obj in (BODY, BOWLER, LAMP, HANDLE, SATCHEL, FUSE):
        obj.hide_render = hidden
    for c in RIG.children:
        if c.name.startswith(('UCX_', 'USP_')):
            c.hide_render = hidden


def clear(where):
    for o in list(where.objects):
        bpy.data.objects.remove(o, do_unlink=True)
    bpy.data.collections.remove(where)


def sparks(mc, where, obj):
    """The lit fuse's spitting end, for the previews (the code's sparks): a white-hot flare and a halo at SOCKET_Fuse."""
    M = obj.matrix_world
    tip = np.array(M @ Vector(tuple(FUSE_TIP)))
    k = M.to_scale()[0]
    V, F = mc.uc.ellipsoid(tip, (0.01 * k, 0.01 * k, 0.012 * k), segs=12, rings=8)
    mc.uc.mesh_object('Spark', V, F, mc.flame_material(1.6), where)
    V, F = mc.uc.ellipsoid(tip, (0.05 * k,) * 3, segs=24, rings=14)
    halo = mc.uc.mesh_object('SparkHalo', V, F, mc.halo_material(1.6, color=0xffb050), where)
    halo.visible_shadow = False
    bulb = bpy.data.objects.new('FuseLight', bpy.data.lights.new('FuseLight', 'POINT'))
    bulb.data.energy = 6.0 * k * k
    bulb.data.color = (1.0, 0.62, 0.3)
    bulb.data.shadow_soft_size = 0.02
    bulb.location = Vector(tuple(tip))
    where.objects.link(bulb)


def shot_turn(mc, path):
    """Front, side and back at the actor's 1.5x, at rest, beside a 1.8 m post and a 1.8 m figure; his props below."""
    where = bpy.data.collections.new('Turn')
    bpy.context.scene.collection.children.link(where)
    mc.uc.post(where, (-1.75, 0.0, 0.0))
    mc.mannequin(where, (-1.35, 0.0, 0.0), 0.0)
    for k, t in enumerate((0.0, 90.0, 180.0)):
        figure_copy(where, (-0.55 + 1.05 * k, 0.0, 0.0), t)
    for src, at, s, t in ((LAMP, (2.4, -0.2, 2.2), 3.0, 0.0), (BOWLER, (2.35, -0.2, 1.75), 1.5, 0.0),
                          (FUSE, (1.95, -0.2, 0.7), 2.0, 0.0), (HANDLE, (2.75, -0.2, 0.18), 1.5, -90.0),
                          (SATCHEL, (2.1, 0.5, 1.6), 1.5, -90.0)):
        o = mn.dup(src, where)
        o.matrix_world = Matrix.Translation(Vector(at)) @ Matrix.Rotation(math.radians(t), 4, 'Z') @ Matrix.Scale(s, 4)
        mn.preview_look(o, rank=SOULFED)
    mc.studio(where)
    cam = mc.uc.camera((0.45, -40.0, 1.45 + 40.0 * math.tan(math.radians(5.0))), (0.45, 0.0, 1.45), ortho=6.0)
    mc.render(path, (2400, 1200), 96)
    bpy.data.objects.remove(cam)
    clear(where)


def shot_fuse(mc, path):
    """In the stope under cool ghost light: 'Fire in the hole!', a fuse lit in his raised left fist, the handle low in
    his right, his coal open; a 1.8 m figure for scale."""
    where = bpy.data.collections.new('Fuse')
    bpy.context.scene.collection.children.link(where)
    mc.gallery_stage(where, rubble=True)
    mc.ghost_light(where)
    objs = figure_copy(where, (0.1, 3.0, 0.0), 12.0, arms=FUSE_ARMS, rank_alpha=2.6)
    mn.lamp_glow(mc, where, objs[2], halo=0.03, light=2.5)
    sparks(mc, where, objs[-1])
    mc.mannequin(where, (-1.1, 3.6, 0.0), 0.3)
    eye = Vector((0.9, -1.4, 1.6))
    mc.lens_shot(path, eye, Vector((0.0, 3.0, 1.55)), 30.0, (1600, 1100), 96)
    clear(where)


def shot_golden(mc, path):
    """At the Providence's mouth in the golden light, three-quarter view, the handle carried as the miners' calm does."""
    where = bpy.data.collections.new('Golden')
    bpy.context.scene.collection.children.link(where)
    mc.portal_stage(where)
    mc.golden(where)
    figure_copy(where, (0.0, 1.2, 0.0), -24.0, arms=CALM_ARMS)
    mc.mannequin(where, (-1.3, 1.9, 0.0), -0.2)
    eye = Vector((1.9, -3.4, 1.7))
    mc.lens_shot(path, eye, Vector((-0.2, 1.3, 1.45)), 32.0, (1600, 1100), 96)
    clear(where)


SHOTS = {'turn': ('Foreman_turn.png', shot_turn), 'fuse': ('Foreman_fuse.png', shot_fuse),
         'golden': ('Foreman_golden.png', shot_golden)}


def previews():
    mc = mn.kit()
    os.makedirs(OUT, exist_ok=True)
    hide_sources(True)
    wanted = [k for k in SHOTS if k in ARGV] or list(SHOTS)
    for key in wanted:
        name, shot = SHOTS[key]
        shot(mc, os.path.join(OUT, name))
    hide_sources(False)


if want_preview():
    previews()
