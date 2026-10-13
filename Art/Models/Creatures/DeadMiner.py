"""The Providence's dead miners (ADeadMinerCreature; Docs/Areas/RansomsRest_Expansion.md, section 6), as the user picked
them on 2026-10-13: option A of Art/Backlog/Creatures/MinerConcepts.py, "canvas and carbide", all three outfits. The
Unpaid's kin in the clothes they died in about 1900: a hip-length duck coat hanging open over a flannel shirt, braces, a
neckerchief; the Unpaid's hungry face and shroud and its coal; a felt hat with its brim pinned up in front (or a leather
cap) carrying a brass carbide lamp with a polished reflector, the weak spot; a pick in the right fist.

SK_DeadMiner is built on Unpaid.py's own code (run here up to its build, so nothing of it is made): the same bones, the
same rest pose (the Unpaid's idle) and the same skeleton laid out the same way, so ADeadMinerCreature poses it with the
Unpaid's rig code unchanged. What differs: the clothes; the right hand's fingers closed into a fist round the pick's haft
(its finger bones follow them; the code's idle Curl leaves them closed); and one more bone, 'pick', under hand_r, at the
fist's grip along the haft.

Models (in /Game/Art/Creatures):
  SK_DeadMiner       about 7.9k triangles, LODs at 50% and 25% (no Nanite); slots DeadMiner_A (M_Ghost: outfit A's four
                     zone colors; the tints MI_DeadMiner_B and _C go on this slot, the hat with it) and GhostCoal (shared
                     with SK_Unpaid). Zones (vertex R): 0 skin and shroud, 1/3 the shirt, 2/3 the coat, 1 the accents
                     (hat, neckerchief, braces).
  SM_DeadMinerHat    the felt, its brim pinned up in front, on the 'hat' bone as SM_UnpaidHat is (its pivot the bone's
                     head, modeled as it sits at rest: attach turned by the inverse of the bone's rest rotation); slot
                     DeadMiner_A (felt and band in the accent zone, so the tints color it); SOCKET_Lamp on the pinned-up
                     flap, its front (-Y; Unreal +X) the lamp's forward.
  SM_DeadMinerCap    the leather cap for outfit B, the same way; SOCKET_Lamp on its crown's front.
  SM_DeadMinerLamp   the carbide lamp in its own axes: its pivot at its mount (the hook's back), its lens along -Y (Unreal
                     +X), Z up; SOCKET_Light at the burner in the reflector's middle (the glow, the light and the hit
                     sphere go there). Slot DeadMinerLamp: the reflector's silver, brass, iron, the strap's leather. No
                     flame of its own: the code's glow cards are the flame, so a lamp shot out goes dark.
  SM_DeadMinerPick   the pick in its own axes: its pivot where the fist grips the haft, a hand's width over the butt; the
                     haft along +Z to the head (0.76 m), the point along -Y (Unreal +X), the chisel +Y. Slot DeadMinerPick.
  SM_DeadMinerPail   the lunch pail: its pivot at the bail's top (where a fist holds it), the pail hanging below (-Z).
                     Slot DeadMinerPail.
The parts' rest places (the pick in the fist, the lamp on each hat, the pail's grip in the left hand) are logged as PART
lines and written to Intermediate/DeadMinerModel/DeadMiner_parts.json in Unreal's component space.

Hit zones (PA_DeadMiner), on the Unpaid's scheme: a hull on pelvis, spine_01, spine_02, neck, head, jaw, each upper arm,
forearm and hand (the right fist's fingers as the code curls them), tail_01, tail_02; a 7 cm sphere on the coal.

    powershell -NoProfile -File Tools\\artrun.ps1 -Script Art\\Models\\Creatures\\DeadMiner.py -Preview
Previews go to Saved/ArtPreviews/RansomsRest/DeadMiner/.
"""
import json
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
OUT = os.path.join(lt.REPO, 'Saved', 'ArtPreviews', 'RansomsRest', 'DeadMiner')
WORK = os.path.join(lt.REPO, 'Intermediate', 'DeadMinerModel')

# The three outfits (skin and shroud, shirt, coat, accents: the felt or the cap, neckerchief, braces). A is the model's
# own slot; B and C are the tints MI_DeadMiner_B and MI_DeadMiner_C (made as MI_Ghost_B and _C are). B wears the cap.
CLOTHES = {'A': ('#BAC4C6', '#8F9CAB', '#A08D6C', '#6A5A4A'),     # slate flannel, tan duck, brown felt and kerchief
           'B': ('#BAC4C6', '#C9C0AA', '#72849A', '#3D3836'),     # oat shirt, faded denim, black leather cap and kerchief
           'C': ('#BAC4C6', '#9A958C', '#7A4F42', '#6F6A60')}     # grey flannel, oxblood oilcloth, grey felt
LAMP_COLORS = ('#D0CCC0', '#C2A66E', '#5F6264', '#4A3E34')         # the reflector's silver, brass, iron, leather
PICK_COLORS = ('#C9CBC6', '#A89777', '#8B9196', '#C9CBC6')         # bright steel, ash, iron
PAIL_COLORS = ('#B3B4AE', '#B3B4AE', '#5C5E5E', '#C2A36B')         # tin, tin, its dark rim and bail, brass
TRIANGLES = dict(shirt=700, coat=1500, sleeve=300)
PICK_GRIP = 0.1                            # the fist holds the haft this far over its butt
PICK_LENGTH = 0.86                           # from the butt to the eye


def log(message):
    print(f'DEADMINER: {message}', flush=True)


# --- Unpaid.py's own code, run up to its build: its bones, rest, head, arms, shroud and hit zones ---

U = mn.unpaid_library()
mn.fist_right(U)                             # the right fist closed round the pick's haft, its finger bones with it
TORSO = U['TORSO_D']
COAL_AT = (0.3, 1.32)
hunch, hunched = U['hunch'], U['hunched']
surface, from_unpaid = lg.surface, mn.from_unpaid


# --- The clothes (built in the Unpaid's frame before its hunch, as Unpaid.py builds its own) ---

COAT = [(0.82, 0.192, 0.14, 0.004, 2.3), (0.90, 0.186, 0.134, 0.004, 2.3), (0.98, 0.178, 0.126, 0.002, 2.3),
        (1.06, 0.17, 0.12, -0.002, 2.3), (1.16, 0.172, 0.12, -0.008, 2.4), (1.26, 0.18, 0.124, -0.014, 2.5),
        (1.34, 0.188, 0.124, -0.02, 2.7), (1.40, 0.194, 0.118, -0.028, 2.9), (1.45, 0.192, 0.11, -0.034, 3.2),
        (1.475, 0.18, 0.102, -0.038, 3.4), (1.50, 0.152, 0.093, -0.042, 3.0), (1.52, 0.114, 0.083, -0.045, 2.6),
        (1.545, 0.082, 0.074, -0.047, 2.2)]
HEM = 0.835


def coat_open(zz):
    """The open coat's fronts (angle either side of the middle): wide enough at the chest to show the shirt and the coal,
    parting further toward the hem."""
    return 0.4 + 0.22 * smoothstep(1.3, 0.85, zz) - 0.1 * smoothstep(1.42, 1.53, zz)


def build_torso():
    """The shirt with the coal's crater, its collar too big for the neck, the coal; the duck coat hanging open, its
    collar turned down, its pockets, its sleeves to the wrist with cuffs; the braces in the coat's opening; the
    neckerchief."""
    parts = []
    S = {s: hunched(U['SHOULDER'][s]) for s in SIDES}
    # The shirt (the Unpaid's): loose folds round the tails, the hollow chest, the crater.
    theta = np.linspace(0.0, 2.0 * np.pi, 96, endpoint=False)
    z = np.linspace(0.90, 1.54, 40)
    P = surface(TORSO, z, theta)
    tails = smoothstep(1.0, 0.9, z)[:, None]
    folds = (np.cos(5 * theta + 0.7) * 0.6 + np.cos(9 * theta + 2.1) * 0.4)[None, :]
    P[..., 0] *= 1.0 + 0.06 * tails * folds
    P[..., 1] *= 1.0 + 0.06 * tails * folds
    P[0, :, 2] += 0.016 * np.sin(3 * theta + 1.0) + 0.009 * np.sin(7 * theta)
    dense = lg.closed_loft(P)
    dense = lg.push(dense, (0.0, -0.11, 1.4), (0.03, 0.04, 0.09), -0.012)
    for s in SIDES:
        dense = lg.push(dense, (s * 0.06, -0.09, 1.46), 0.032, -0.008)
    tc, zc = COAL_AT
    pc = surface(TORSO, [zc], np.array([tc]))[0, 0]
    nc = U['torso_normal'](tc)
    dense = lg.push(dense, pc, 0.068, -0.02)
    dist = np.linalg.norm(dense[0] - pc, axis=1)
    jitter = 0.5 + 0.5 * lg.pnoise(dense[0], 60.0, 5)
    d_cav = smoothstep(0.06, 0.035, dist) * 0.9
    d_ember = smoothstep(0.03, 0.05, dist) * smoothstep(0.085, 0.062, dist) * (0.4 + 0.6 * jitter)
    low = lg.reduce(dense, TRIANGLES['shirt'])
    near = lg.nearest(dense[0], low[0])
    shirt = lg.Part('Shirt', (hunch(low[0]), low[1]), SHIRT, cavity=d_cav[near], ember=d_ember[near])
    U['torso_weights'](shirt, low[0][:, 2], low[0][:, 0])
    parts.append(shirt)
    coal_center = hunched(pc - nc * 0.012)
    coal_normal = unit(hunched(pc + nc * 0.05) - hunched(pc))
    collar = np.array([np.stack([0.05 * np.sin(theta[::3]), -0.043 - 0.047 * np.cos(theta[::3]), np.full(32, zz)], -1)
                       for zz in (1.52, 1.55, 1.578)])
    part = lg.Part('Collar', (hunch(collar.reshape(-1, 3)), [lg.grid_faces(3, 32)]), SHIRT)
    parts.append(part.weigh('neck', 0.5).weigh('spine_02', 0.5))

    # The coat: boxy duck canvas hanging off him, stiff folds deepening toward the hem, open in front; the coal has
    # burnt through its left breast where it laps over.
    rng = np.random.default_rng(22)
    amps, phases = rng.uniform(0.5, 1.0, 4), rng.uniform(0.0, 2.0 * np.pi, 4)

    def coat_folds(th, zz):
        f = sum(a * np.cos(k * th + p) for a, k, p in zip(amps, (5, 7, 11, 15), phases)) / amps.sum()
        sag = 0.006 * smoothstep(1.3, 0.9, zz) * (1.0 - smoothstep(0.0, 0.9, np.abs(th)))
        return 0.012 + 0.013 * smoothstep(1.15, 0.85, zz) * (f - 0.3 * np.abs(f)) + sag

    def coat_sd(th, zz):
        h = HEM + 0.008 * np.sin(4 * th + 1.0) + 0.006 * np.sin(11 * th)
        out = np.maximum(h - zz, coat_open(zz) - np.abs(th))
        out = np.maximum(out, lg.armhole(th, zz, 1.368, 0.24, 0.055))
        return np.maximum(out, zz - 1.546)
    (V, F), th, zz, edge = lg.shell(COAT, HEM - 0.02, 1.546, coat_sd, coat_folds, rows=110, cols=180, hole=0.074,
                                    coal_at=COAL_AT)
    ember = lg.scorch(edge, V)
    seam = 0.25 * gauss(zz - (HEM + 0.025), 0.004) + 0.3 * gauss(np.abs(th) - coat_open(zz) - 0.035, 0.008)
    cav = np.maximum(0.55 * smoothstep(0.025, 0.0, edge), seam)
    low = lg.reduce((V, F), TRIANGLES['coat'])
    near = lg.nearest(V, low[0])
    coat = lg.Part('Coat', (hunch(low[0]), low[1]), COAT_ZONE, ember=ember[near], cavity=cav[near])
    parts.append(U['torso_weights'](coat, low[0][:, 2], low[0][:, 0]))
    # Patch pockets at the hips, and one on the right breast.
    for s, (t0, t1, z0, z1, grow) in [(1.0, (0.66, 1.1, 0.88, 1.0, 0.032)), (-1.0, (-1.1, -0.66, 0.88, 1.0, 0.032)),
                                      (-1.0, (-0.98, -0.6, 1.24, 1.33, 0.026))]:
        ts, zs = np.linspace(t0, t1, 6), np.linspace(z0, z1, 5)
        Pp = surface(COAT, zs, ts, grow)
        Tg, Zg = np.meshgrid(np.linspace(0, 1, len(ts)), np.linspace(0, 1, len(zs)))
        rim = smoothstep(0.22, 0.0, np.minimum(np.minimum(Tg, 1 - Tg), np.minimum(Zg, 1 - Zg)))
        part = lg.Part('Pocket', (hunch(Pp.reshape(-1, 3)), [lg.grid_faces(len(zs), len(ts), closed=False)]), COAT_ZONE,
                       cavity=(0.5 * rim).ravel())
        parts.append(U['torso_weights'](part, Pp.reshape(-1, 3)[:, 2], Pp.reshape(-1, 3)[:, 0], shoulders=False))
    # The collar turned down onto the shoulders, its points on the chest.
    ang = np.linspace(-np.pi, np.pi, 40, endpoint=False)
    rows = [(1.552, 0.09, 0.0), (1.535, 0.105, 0.0), (1.505, 0.125, 0.012), (1.47, 0.142, 0.03)]
    rings = np.array([np.stack([r0 * np.sin(ang), -0.044 - r0 * 0.96 * np.cos(ang),
                                z0 - drop * 2.2 * np.cos(ang).clip(0.0, None) ** 2], -1) for z0, r0, drop in rows])
    Vc, Fc, used = lg.cut(rings, np.broadcast_to((0.42 - np.abs(ang))[None, :], rings.shape[:2]) * 0.1)
    part = lg.Part('CoatCollar', (hunch(Vc), Fc), COAT_ZONE, cavity=0.12)
    parts.append(part.weigh('spine_02', 0.7).weigh('neck', 0.3))
    # The sleeves to the wrists over the Unpaid's thin forearms, cuffs turned back.
    for k, s in enumerate(SIDES):
        sfx = SUFFIX[s]
        E, W = U['bent'](s)[:2]
        piece, line, at_elbow = lg.sleeve_piece(S[s] + np.array([-s * 0.012, 0.0, 0.008]), E, W, (0.064, 0.058, 0.05),
                                                31 + k, folds=0.08)
        low = lg.reduce(piece, TRIANGLES['sleeve'])
        part = lg.Part(f'Sleeve_{sfx}', low, COAT_ZONE)
        parts.append(lg.weigh_sleeve(part, low[0], line, at_elbow, sfx))
        dv = unit(W - E)
        cuff = lg.torus(W - dv * 0.045, dv, 0.05, 0.013, segs=14, sides=6, squash=1.3)
        parts.append(lg.Part(f'Cuff_{sfx}', cuff, COAT_ZONE, cavity=0.15).weigh(f'lowerarm_{sfx}', 1.0))
    # The braces up the shirt in the coat's opening, and their buttons at the waist.
    for s in SIDES:
        keys = [(s * 0.6, 0.99), (s * 0.57, 1.14), (s * 0.52, 1.28), (s * 0.47, 1.4), (s * 0.55, 1.47)]
        path = [surface(TORSO, [zr], np.array([t]), 0.013)[0, 0] for t, zr in keys]
        piece = lg.ribbon(path, 0.016, n=12, cols=3, out=lambda C: lg.outward(np.arctan2(C[:, 0], -C[:, 1])), curl=0.05)
        part = lg.Part('Brace', (hunch(piece[0]), piece[1]), ACCENT)
        parts.append(U['torso_weights'](part, piece[0][:, 2], piece[0][:, 0], shoulders=False))
        p = surface(TORSO, [0.99], np.array([s * 0.6]), 0.019)[0, 0]
        n = U['torso_normal'](s * 0.6)
        button = lg.ellipsoid(p, (0.007, 0.007, 0.0045), np.array([[0, 0, 1.0], np.cross(n, (0, 0, 1.0)), n]), 8, 4)
        parts.append(lg.Part('BraceButton', button, ACCENT, cavity=0.4).weigh('pelvis', 1.0))
    # The neckerchief (the Unpaid's bandana, a miner's knot).
    knot = lg.torus((0.0, -0.046, 1.566), (0.0, -0.3, 1.0), 0.047, 0.011, segs=14, sides=5, squash=0.75)
    parts.append(lg.Part('Kerchief', (hunch(knot[0]), knot[1]), ACCENT).weigh('neck', 0.6).weigh('spine_02', 0.4))
    nrows, ncols = 6, 9
    zb = np.linspace(1.545, 1.43, nrows)
    width = np.linspace(0.9, 0.06, nrows)
    flap = np.array([surface(TORSO, [zr], np.linspace(-w, w, ncols), grow=0.03)[0] for zr, w in zip(zb, width)])
    flap[..., 2] += 0.005 * np.sin(np.linspace(0, 3 * np.pi, ncols))[None, :] * (1 - width[:, None])
    flat = flap.reshape(-1, 3)
    part = lg.Part('Kerchief', (hunch(flat), [lg.grid_faces(nrows, ncols, closed=False)]), ACCENT)
    parts.append(part.weigh('spine_02', 1.0 - 0.4 * smoothstep(1.5, 1.545, flat[:, 2]))
                 .weigh('neck', 0.4 * smoothstep(1.5, 1.545, flat[:, 2])))
    back = lg.ellipsoid((0.0, 0.0, 1.562), (0.018, 0.013, 0.015), segs=8, rings=5)
    parts.append(lg.Part('Kerchief', (hunch(back[0]), back[1]), ACCENT).weigh('neck', 1.0))
    # The coal, burning where the heart was (the Unpaid's).
    V, F = lg.ellipsoid(coal_center, (0.056, 0.056, 0.052), segs=12, rings=8)
    d = V - coal_center
    V = V + d * (0.16 * lg.pnoise(V, 30.0, 3))[:, None]
    parts.append(lg.Part('Coal', (V, F), SKIN, slot=1).weigh('coal', 1.0))
    return parts, dict(coal=(coal_center, coal_normal), shoulders=S)


# --- The rig ---

def materials():
    body = lg.ghost_slot('DeadMiner_A', CLOTHES['A'], **lg.GHOST_LOOK)
    coal = lg.coal_slot()
    lamp = lg.ghost_slot('DeadMinerLamp', LAMP_COLORS, **dict(lg.PROP_LOOK, RimStrength=1.2, GlowStrength=0.14))
    pick = lg.ghost_slot('DeadMinerPick', PICK_COLORS, **lg.PROP_LOOK)
    pail = lg.ghost_slot('DeadMinerPail', PAIL_COLORS, **lg.PROP_LOOK)
    return dict(body=body, coal=coal, lamp=lamp, pick=pick, pail=pail)


def build_rig(mats):
    torso_parts, torso = build_torso()
    arm_parts, fingers, old_fingers = U['build_arms']()
    head_parts, head = U['build_head']()
    shroud_parts, tail = U['build_shroud']()
    parts = torso_parts + [from_unpaid(p) for p in arm_parts + head_parts + shroud_parts]
    rig = lm.armature()
    lm.bones(rig, U['bone_specs'](torso, old_fingers, head, tail))
    U['bake_idle'](rig, fingers)
    G, haft, point = mn.fist_grip(U, -1.0)
    mn.add_bone(rig, 'pick', G, G + haft * 0.1, 'hand_r')
    body, arrays, layout = lg.skinned(rig, 'DeadMiner', parts, [mats['body'], mats['coal']])
    lg.triangle_report(log, 'DeadMiner', body, parts)
    return rig, body, head, arrays, layout, parts


# --- The props ---

two_sided_brim = mn.two_sided_brim


def hat_felt():
    """The felt in hat space (origin at the brim's middle, front -Y): a soft crown dented on top, the brim drooping round
    the sides and back and pinned up in front, a band. Returns parts and the lamp mount (point, pitch)."""
    cols = 32
    theta = np.linspace(0.0, 2.0 * np.pi, cols, endpoint=False)
    q = np.linspace(0.0, 1.0, 4)[:, None]
    front = np.exp(-(wrap(theta) / 0.7) ** 2)
    r = 0.087 + 0.083 * q * (1.0 + 0.03 * np.sin(3 * theta + 0.6)) - 0.05 * q ** 1.3 * front
    lift = 0.085 * q ** 1.1 * front - 0.024 * q ** 1.6 * (1.0 - front) + 0.003 * q * np.sin(5 * theta + 1.0)
    top = np.stack([r * np.sin(theta), -r * np.cos(theta) * 1.06, lift], -1)
    brim = two_sided_brim(top, 0.007, ACCENT)
    zc = [0.0, 0.03, 0.06, 0.085, 0.1, 0.108]
    rc = [0.088, 0.087, 0.084, 0.077, 0.064, 0.042]
    tc = np.linspace(0.0, 2.0 * np.pi, 24, endpoint=False)
    rings = np.array([np.stack([rr * np.sin(tc), -rr * np.cos(tc) * 1.1, np.full_like(tc, zz)], -1) for zz, rr in zip(zc, rc)])
    rings[..., 2] -= 0.016 * np.exp(-(rings[..., 0] / 0.025) ** 2) * (rings[..., 2] > 0.088)
    n = len(zc) * 24
    crown = lg.Part('Crown', (np.vstack([rings.reshape(-1, 3), [[0.0, 0.0, 0.086]]]),
                              [lg.grid_faces(len(zc), 24), lg.fan(n, np.arange(24) + n - 24, top=True)]), ACCENT,
                    cavity=np.concatenate([0.12 * smoothstep(0.035, 0.05, rings.reshape(-1, 3)[:, 2]), [0.25]]))
    band = np.array([np.stack([0.0905 * np.sin(tc), -0.0905 * np.cos(tc) * 1.1, np.full_like(tc, z)], -1)
                     for z in (0.003, 0.016, 0.029)])
    band = lg.Part('Band', (band.reshape(-1, 3), [lg.grid_faces(3, 24)]), ACCENT, cavity=0.5)
    mount = top[2, 0] + np.array([0.0, -0.004, 0.0])
    return [brim, crown, band], (mount, 8.0)


def hat_cap():
    """The leather cap in hat space: a soft round crown over a stitched band, a short stiff peak; the lamp's bracket is
    the lamp's own hook."""
    tc = np.linspace(0.0, 2.0 * np.pi, 28, endpoint=False)
    zc = [0.0, 0.018, 0.04, 0.06, 0.074, 0.083, 0.088]
    rc = [0.089, 0.093, 0.094, 0.088, 0.074, 0.05, 0.02]
    rings = np.array([np.stack([rr * np.sin(tc), -rr * np.cos(tc) * 1.12, np.full_like(tc, zz)], -1) for zz, rr in zip(zc, rc)])
    rings[..., 1] -= 0.006 * (rings[..., 2] / 0.088)
    n = len(zc) * 28
    flat = rings.reshape(-1, 3)
    seams = 0.25 * np.clip(np.max([gauss(np.abs(wrap(np.arctan2(flat[:, 0], -flat[:, 1]) - a)), 0.05)
                                   for a in (0.0, 1.57, -1.57, 3.14)], axis=0), 0, 1) * (flat[:, 2] > 0.03)
    crown = lg.Part('Crown', (np.vstack([flat, [[0.0, -0.006, 0.089]]]),
                              [lg.grid_faces(len(zc), 28), lg.fan(n, np.arange(28) + n - 28, top=True)]), ACCENT,
                    cavity=np.concatenate([seams, [0.2]]))
    band = np.array([np.stack([0.0905 * np.sin(tc), -0.0905 * np.cos(tc) * 1.12, np.full_like(tc, z)], -1)
                     for z in (0.0, 0.01, 0.02)])
    band = lg.Part('Band', (band.reshape(-1, 3), [lg.grid_faces(3, 28)]), ACCENT, cavity=0.4)
    a = np.linspace(-1.15, 1.15, 14)
    qq = np.linspace(0.0, 1.0, 3)[:, None]
    r = 0.09 + 0.05 * qq * np.cos(a * 0.9) ** 0.7
    top = np.stack([r * np.sin(a), -r * np.cos(a) * 1.12, 0.002 - 0.02 * qq ** 1.4 + 0.0 * a], -1)
    under = top - np.array([0.0, 0.0, 0.005])
    V = np.vstack([top.reshape(-1, 3), under.reshape(-1, 3)])
    F = [lg.grid_faces(3, 14, closed=False)[:, ::-1], lg.grid_faces(3, 14, closed=False) + 42]
    edge = np.arange(13)
    F.append(np.stack([28 + edge, 28 + edge + 1, 42 + 28 + edge + 1, 42 + 28 + edge], 1)[:, ::-1])
    peak = lg.Part('Peak', (V, F), ACCENT, cavity=np.concatenate([np.full(42, 0.2), np.full(42, 0.45)]))
    return [crown, band, peak], (np.array([0.0, -0.105, 0.046]), 4.0)


HATS = {'DeadMinerHat': hat_felt, 'DeadMinerCap': hat_cap}
HAT_TILT = (-4.0, 5.0)                       # SM_UnpaidHat's: tipped back a little and to one side


def build_hat(name, mats, head):
    """A hat on the 'hat' bone as it sits at rest; SOCKET_Lamp at its mount. Returns the object and the mount's frame
    (rig space) at rest."""
    parts, (mount, pitch) = HATS[name]()
    Rh = rot_x(HAT_TILT[0]) @ rot_y(HAT_TILT[1])
    pivot = np.asarray(head['hat'], float)
    for p in parts:
        p.V = p.V @ Rh.T + pivot
    obj = lg.static_prop(name, parts, [mats['body']], pivot)
    R = Rh @ rot_x(pitch)
    at = pivot + Rh @ mount
    lg.socket(obj, 'Lamp', at, R)
    log(f'SM_{name}: {lg.triangles_of(obj)} triangles; SOCKET_Lamp {tuple(np.round(lg.to_ue(at - pivot), 2))} cm from its '
        f'pivot, its lens {pitch + HAT_TILT[0]:.0f} degrees down from level')
    return obj, (at, R)


def build_lamp(mats):
    return mn.build_lamp('DeadMinerLamp', mats['lamp'], log)


def build_pick(mats):
    obj = lg.static_prop('DeadMinerPick', mn.pick_parts(grip=PICK_GRIP, length=PICK_LENGTH), [mats['pick']], (0.0, 0.0, 0.0))
    log(f'SM_DeadMinerPick: {lg.triangles_of(obj)} triangles')
    return obj


def pail_parts():
    """The lunch pail in pail space: the bail's top at the origin, the tin hanging below with its lid and the tea cup
    on top, the wire bail over it."""
    tin = lg.lathe([(-0.24, 0.0), (-0.24, 0.066), (-0.236, 0.07), (-0.13, 0.072), (-0.124, 0.076), (-0.118, 0.076),
                    (-0.112, 0.07), (-0.108, 0.048), (-0.088, 0.046), (-0.084, 0.0)], segs=16)
    rim = lg.torus((0.0, 0.0, -0.13), (0.0, 0.0, 1.0), 0.072, 0.003, segs=16, sides=4)
    arc = [(0.075 * math.cos(a), 0.0, -0.17 + 0.17 * math.sin(a)) for a in np.linspace(0.0, np.pi, 11)]
    bail = lg.tube(arc, 0.0025, segs=5, up=(0.0, 1.0, 0.0), domes=1)
    V = tin[0]
    return [lg.Part('Pail', tin, 1, cavity=0.25 * smoothstep(-0.2, -0.24, V[:, 2])),
            lg.Part('Rim', rim, 2, cavity=0.3), lg.Part('Bail', bail, 2)]


def build_pail(mats):
    obj = lg.static_prop('DeadMinerPail', pail_parts(), [mats['pail']], (0.0, 0.0, 0.0))
    log(f'SM_DeadMinerPail: {lg.triangles_of(obj)} triangles')
    return obj


# --- Build ---

def build():
    mats = materials()
    rig, body, head, (V, faces, colors, W, names), layout, parts = build_rig(mats)
    hats = {name: build_hat(name, mats, head) for name in HATS}
    lamp = build_lamp(mats)
    pick = build_pick(mats)
    pail = build_pail(mats)
    U['hit_zones'](rig, V, faces, W, names)
    return rig, body, head, hats, lamp, pick, pail, colors, layout, parts, mats


RIG, BODY, HEAD, HATS_BUILT, LAMP, PICK, PAIL, RAW_COLORS, LAYOUT, PARTS, MATS = build()
G_PICK, HAFT, POINT = mn.fist_grip(U, -1.0)
PICK_FRAME = mn.tool_frame(HAFT, POINT)      # pick space (x, y = -point, z = haft) in rig space at rest


def write_parts():
    rest = {'SM_DeadMinerPick (on the pick bone)': mn.part_rest(RIG, G_PICK, PICK_FRAME, 'pick'),
            'SM_DeadMinerPick (on hand_r)': mn.part_rest(RIG, G_PICK, PICK_FRAME, 'hand_r')}
    for name, (obj, (at, R)) in HATS_BUILT.items():
        rest[f'SM_DeadMinerLamp on SM_{name}'] = mn.part_rest(RIG, at, R, 'hat')
    rest['SM_DeadMinerPail (its bail in the left fist, hanging plumb)'] = mn.part_rest(RIG, mn.fist_grip(U, 1.0)[0],
                                                                                       np.eye(3), 'hand_l')
    mn.write_parts(os.path.join(WORK, 'DeadMiner_parts.json'), rest, log)


write_parts()
totals = {'SK_DeadMiner': lg.triangles_of(BODY), 'SM_DeadMinerHat': lg.triangles_of(HATS_BUILT['DeadMinerHat'][0]),
          'SM_DeadMinerLamp': lg.triangles_of(LAMP), 'SM_DeadMinerPick': lg.triangles_of(PICK),
          'SM_DeadMinerPail': lg.triangles_of(PAIL)}
log(f'a dead miner as drawn (body, felt, lamp, pick; the pail besides): '
    f'{sum(v for k, v in totals.items() if k != "SM_DeadMinerPail")} triangles; with the cap instead '
    f'{totals["SK_DeadMiner"] + lg.triangles_of(HATS_BUILT["DeadMinerCap"][0]) + totals["SM_DeadMinerLamp"] + totals["SM_DeadMinerPick"]}')


# --- Previews (Saved/ArtPreviews/RansomsRest/DeadMiner) ---

# ADeadMinerCreature's calm (DeadMinerCreaturePose.cpp's CarryUpper and CarryFore): the right arm laid along these.
CARRY = dict(upper=mn.ue_dir(0.15, 0.28, -0.95), fore=mn.ue_dir(0.2, -0.3, 0.93))


def want_preview():
    return '--preview' in ARGV or lt.want_preview()


def figure_copy(where, outfit, location, turn, hat_name, rank=0xb02a18, carry=False):
    """A copy of the body (no armature) in an outfit's tint, its hat, lamp and pick where they sit, standing at location
    turned about Z (degrees): at rest, or with the right arm and the pick as the code's calm carries them."""
    M = mn.figure_matrix(location, turn)
    swap = {'DeadMiner_A': CLOTHES[outfit]}
    grip, frame = G_PICK, PICK_FRAME
    if carry:
        grip, frame = mn.code_carry(RIG, CARRY['upper'], CARRY['fore'])[:2]
        body = mn.posed_copy(BODY, where)
        mn.rest_pose(RIG)
    else:
        body = mn.dup(BODY, where)
        body.modifiers.clear()
    body.matrix_world = M
    mn.preview_look(body, RAW_COLORS, swap, rank=rank)
    objs = [body]
    hat, (at, R) = HATS_BUILT[hat_name]
    for src, place in ((hat, Matrix.Translation(Vector(hat.location))), (LAMP, mn.frame_matrix(at, R)),
                       (PICK, mn.frame_matrix(grip, frame))):
        o = mn.place_copy(src, where, M, place)
        mn.preview_look(o, swap=swap, rank=rank)
        objs.append(o)
    return objs


def hide_sources(hidden=True):
    for obj in [BODY, LAMP, PICK, PAIL] + [h for h, _ in HATS_BUILT.values()]:
        obj.hide_render = hidden
    for c in RIG.children:
        if c.name.startswith(('UCX_', 'USP_')):
            c.hide_render = hidden


def clear(where):
    for o in list(where.objects):
        bpy.data.objects.remove(o, do_unlink=True)
    bpy.data.collections.remove(where)


def shot_pack(mc, path):
    """The pack at the Providence's mouth in the golden light, picks carried as the code's calm does: the three outfits
    (the felt, the cap, the felt) and a 1.8 m figure."""
    where = bpy.data.collections.new('Pack')
    bpy.context.scene.collection.children.link(where)
    mc.portal_stage(where)
    mc.golden(where)
    for outfit, (x, y, t), hat in (('A', (-1.2, 1.5, -17.0), 'DeadMinerHat'), ('B', (0.1, 0.5, -13.0), 'DeadMinerCap'),
                                   ('C', (1.35, 2.1, -24.0), 'DeadMinerHat')):
        figure_copy(where, outfit, (x, y, 0.0), t, hat, carry=True)
    mc.mannequin(where, (-2.25, 2.5, 0.0), -0.2)
    eye = Vector((1.55, -3.1, 1.42))
    mc.lens_shot(path, eye, Vector((-0.2, 1.5, 1.12)), 30.0, (1600, 1100), 96, focus=(eye - Vector((0.1, 0.6, 1.2))).length,
                 fstop=5.6)
    clear(where)


def shot_turn(mc, path):
    """Front, side and back of outfit A at rest (the bind pose) beside a 1.8 m post; the lamp (2.5x), the cap, the pail
    and the pick."""
    where = bpy.data.collections.new('Turn')
    bpy.context.scene.collection.children.link(where)
    mc.uc.post(where, (-1.3, 0.0, 0.0))
    for k, t in enumerate((0.0, 90.0, 180.0)):
        figure_copy(where, 'A', (-0.55 + 0.75 * k, 0.0, 0.0), t, 'DeadMinerHat')
    for src, at in ((LAMP, (1.75, -0.2, 1.0)), (PAIL, (1.75, -0.2, 0.55)), (HATS_BUILT['DeadMinerCap'][0], (2.25, -0.2, 1.05))):
        o = mn.dup(src, where)
        o.matrix_world = Matrix.Translation(Vector(at)) @ (Matrix.Scale(2.5, 4) if src is LAMP else Matrix.Identity(4))
        mn.preview_look(o, swap={'DeadMiner_A': CLOTHES['B']})
    o = mn.dup(PICK, where)
    o.matrix_world = Matrix.Translation(Vector((2.3, -0.2, 0.15))) @ Matrix.Rotation(math.radians(-90), 4, 'Z')
    mn.preview_look(o)
    mc.studio(where)
    cam = mc.uc.camera((0.45, -30.0, 1.05 + 30.0 * math.tan(math.radians(6.0))), (0.45, 0.0, 1.05), ortho=4.4)
    mc.render(path, (2200, 1100), 96)
    bpy.data.objects.remove(cam)
    clear(where)


def shot_gallery(mc, path):
    """In a Providence gallery under cool ghost light, coming on with their lamps lit: the three outfits."""
    where = bpy.data.collections.new('Gallery')
    bpy.context.scene.collection.children.link(where)
    mc.gallery_stage(where, rubble=True)
    mc.ghost_light(where)
    for outfit, (x, y, t), hat in (('A', (-0.75, 3.4, 14.0), 'DeadMinerHat'), ('B', (0.4, 2.2, -6.0), 'DeadMinerCap'),
                                   ('C', (1.0, 4.8, -20.0), 'DeadMinerHat')):
        objs = figure_copy(where, outfit, (x, y, 0.0), t, hat, carry=True)
        mn.lamp_glow(mc, where, objs[2], light=1.2)
    eye = Vector((0.25, -1.6, 1.5))
    mc.lens_shot(path, eye, Vector((0.1, 3.2, 1.2)), 32.0, (1600, 1100), 96)
    clear(where)


SHOTS = {'pack': ('DeadMiner_pack.png', shot_pack), 'turn': ('DeadMiner_turn.png', shot_turn),
         'gallery': ('DeadMiner_gallery.png', shot_gallery)}


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
