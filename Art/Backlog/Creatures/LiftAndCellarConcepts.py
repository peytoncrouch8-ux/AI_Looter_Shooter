"""Concepts for the mountain story's two friendly dead (Docs/Areas/RansomsRest_Expansion.md, section 4), one look each,
for the user to approve (2026-10-13). Concept art kept in Art/Backlog (see its README): nothing in the game uses it. Each
becomes a game model on the Unpaid's rig at size 1 with a pose table, as Amos did (Art/Models/Creatures/Amos.py: the
thighs on skirt_f_01/02, so he can sit; a living man's face; the coal banked low under ash).

  Hiram Coyle, the ore lift's dead engineman ("Dead Lift"; he hasn't left his winch in four years). About sixty, square
     and soot-creased, big grey mutton chops, kind. In the dead miners' costume set: the duck work coat open over a
     hickory shirt, braces, a sweat rag at his neck, dark duck trousers; an engineer's cap with his goggles pushed up on
     it. He sits on his stool by the cold boiler, forearms on his knees, looking up at whoever's come: "Ain't had a
     passenger in four years."
  Thaddeus Bright, Tilly's dead father ("Knocking Below"), the undertaker of Bright & Daughter. Long and thin and
     stooped, bald under a fringe of grey, wire spectacles low on his nose, a carpenter's pencil behind his ear; a
     collarless shirt with its sleeves rolled and held with garters, a black waistcoat, a long leather apron to below the
     knee. At his bench in the cold room he holds his cloth tape out between his hands, measuring a coffin that isn't
     there.

Renders go to Saved/ArtPreviews/RansomsRest/Concepts/Hiram/ and /Thaddeus/: Friends_front.png (both from the front and
the side beside a 1.8 m post and figure, in Concepts/Hiram/), and for each the golden shot (Hiram on his stool in the
winch house, the sun in through the open landing; Thaddeus at his trestles, the low sun through the cellar's window) and
the ghost shot (the same places at night under cool ghost light).

    powershell -NoProfile -File Tools\\artrun.ps1 -Script Art\\Backlog\\Creatures\\LiftAndCellarConcepts.py -Preview
    ... -ScriptArgs Hiram,golden      only some: Hiram Thaddeus; shots front golden ghost
    ... -ScriptArgs Hiram,debug       quick checks into Intermediate/MinerConcepts
"""
import math
import os
import sys
import time

import bpy
import numpy as np
from mathutils import Vector

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import MinerConcepts as mc  # noqa: E402  (the kit)

uc, am, lt = mc.uc, mc.am, mc.lt
smoothstep, unit, rotation, wrap, spline = uc.smoothstep, uc.unit, uc.rotation, uc.wrap, uc.spline
ZPart, rot_x, rot_y, rot_z = mc.ZPart, mc.rot_x, mc.rot_y, mc.rot_z
g, surface, frame = mc.g, mc.surface, mc.frame
ARGV = mc.ARGV
OUT = {'Hiram': os.path.join(mc.CONCEPTS, 'Hiram'), 'Thaddeus': os.path.join(mc.CONCEPTS, 'Thaddeus')}
NAMES = ('Hiram', 'Thaddeus')
SKIN, CLOTH1, CLOTH2, ACCENT = 0, 1, 2, 3
SIDES = (1.0, -1.0)
STOOL = 0.56                                 # the stool's top over the winch house's floor


def log(message):
    print(f'FRIENDS: {message}', flush=True)


# Their faces, on Amos's living-face machinery (AmosConcepts.FACES): tired and patient, never the Unpaid's hunger.
am.FACES['H'] = dict(width=0.152, front=0.099, back=0.108, top=0.12, bottom=0.136, mouth=-0.072, brow=0.0145, bags=0.0018,
                     nose=0.03, cheek=0.0072, hollow=0.0016, jaw=0.0078, chin=0.0095, taper=0.07, naso=0.0024, smile=0.45,
                     worry=0.2, squint=0.65, heavy=0.65, age=0.9, jowl=0.0018, brow_hair=0.0052, droop=0.45, lips=0.85,
                     beard='chops', hair='short', hair_zone=SKIN)
am.FACES['T'] = dict(width=0.14, front=0.097, back=0.106, top=0.124, bottom=0.138, mouth=-0.073, brow=0.012, bags=0.0022,
                     nose=0.034, cheek=0.0058, hollow=0.0042, jaw=0.0045, chin=0.008, taper=0.1, naso=0.0026, smile=0.2,
                     worry=0.55, squint=0.35, heavy=0.72, age=1.0, jowl=0.0012, brow_hair=0.0036, droop=0.6, lips=0.65,
                     beard='stubble', hair='thin', hair_zone=SKIN)


# --- Hiram's engineer's cap with his goggles on it (hat space; the 'hat' look: glass, cap cloth, brass, leather) ---

def cap_engineer():
    """An engineer's cap: a soft crown standing straight up from the band and bellying over it, pleated into a flat
    top, a short stiff peak; his driving goggles pushed up on its front, their strap round the crown."""
    theta = np.linspace(0.0, 2.0 * np.pi, 96, endpoint=False)
    zc = [0.0, 0.02, 0.045, 0.065, 0.078, 0.084]
    rc = [0.09, 0.093, 0.099, 0.101, 0.096, 0.08]
    P = mc.crown_rings(zc, rc, theta, oval=1.1)
    P[..., 2] += 0.004 * np.sin(theta * 12.0)[None, :] * (np.array(zc)[:, None] > 0.06)       # the pleats
    V, F = mc.capped(P, 0.082)
    out = [(V, F, 1.0, CLOTH1, 0.15 * (V[:, 2] > 0.07) * (0.5 + 0.5 * np.sin(np.arctan2(V[:, 0], V[:, 1]) * 12.0)))]
    out.append(mc.band_ring(theta, 0.0915, 0.0, 0.018, 1.1) + (1.0, ACCENT, 0.1))
    a = np.linspace(-1.1, 1.1, 40)
    q = np.linspace(0.0, 1.0, 7)[:, None]
    r = 0.09 + 0.052 * q * np.cos(a * 0.9) ** 0.7
    peak = np.stack([r * np.sin(a), -r * np.cos(a) * 1.1, 0.002 - 0.024 * q ** 1.3 + 0.0 * a], -1)
    out.append((peak.reshape(-1, 3), [uc.grid_faces(7, 40, closed=False)], 1.0, ACCENT, 0.12))
    for s in SIDES:                              # the goggles: brass cups, their lenses, on the crown's front
        c = np.array([s * 0.036, -0.107, 0.05])
        d = unit(np.array([s * 0.25, -1.0, 0.25]))
        out.append(am.tube([c - d * 0.012, c + d * 0.01], [0.021, 0.019], segs=20, domes=1) + (1.0, CLOTH2, 0.15))
        xl = unit(np.cross(d, (0, 0, 1.0)))
        lens = uc.ellipsoid(c + d * 0.011, (0.016, 0.016, 0.003), np.array([xl, np.cross(d, xl), d]), 20, 8)
        out.append(lens + (1.0, SKIN, 0.0))
    out.append(am.tube([(-0.012, -0.112, 0.052), (0.0, -0.115, 0.054), (0.012, -0.112, 0.052)], 0.004, segs=8, domes=1)
               + (1.0, CLOTH2, 0.2))
    out.append(mc.band_ring(theta, 0.099, 0.038, 0.06, 1.1) + (1.0, ACCENT, 0.2))       # the goggles' strap
    return out, (np.array([0.0, -0.11, 0.05]), 0.0)


HATS = dict(mc.HATS, engineer=cap_engineer)
mc.HAT_SEAT['engineer'] = 0.15


# --- Thaddeus's spectacles and pencil (in head space; carried on the 'hat' bone in the game) ---

def spectacles(fig, R, center):
    part = ZPart('Spectacles', CLOTH2)
    glass = ZPart('Lenses', SKIN)
    for s in SIDES:
        c = np.array([s * 0.031, -0.106, -0.012])
        V, F = uc.torus(c, (0.0, -1.0, 0.0), 0.016, 0.0013, segs=40, sides=6)
        V = c + (V - c) * np.array([1.0, 1.0, 0.8])
        part.add(uc.transform((V, F), R, center), cavity=0.2)
        lens = uc.ellipsoid(c, (0.0158, 0.0012, 0.0126), segs=24, rings=8)
        glass.add(uc.transform(lens, R, center))
        temple = [c + np.array([s * 0.016, 0.0, 0.002]), np.array([s * 0.068, -0.06, 0.004]),
                  np.array([s * 0.073, 0.0, 0.0]), np.array([s * 0.07, 0.016, -0.02])]
        part.add(uc.transform(am.tube(spline(temple, 14), 0.0012, segs=6, domes=1), R, center))
    part.add(uc.transform(am.tube(spline([(-0.015, -0.108, -0.01), (0.0, -0.112, -0.004), (0.015, -0.108, -0.01)], 8),
                                  0.0013, segs=6, domes=1), R, center))
    fig.add(part, material='hat')
    fig.add(glass, material='hat')
    pencil = ZPart('Pencil', ACCENT)
    p = [np.array([-0.08, -0.02, 0.02]), np.array([-0.083, 0.04, -0.012])]
    pencil.add(uc.transform(am.tube(p, 0.0042, segs=6, domes=1), R, center))
    fig.add(pencil, material='hat')


# --- Garments ---

def sweat_rag(fig, zone, y0=-0.004):
    rag = ZPart('Rag', zone)
    rag.add(uc.torus((0.0, y0, 1.548), (0.0, -0.25, 1.0), 0.066, 0.012, squash=0.8))
    rag.add(uc.ellipsoid((0.0, -0.075, 1.53), (0.018, 0.013, 0.015)))
    for s in SIDES:
        tail = [(0.0, -0.08, 1.525), (s * 0.014, -0.09, 1.49), (s * 0.022, -0.094, 1.45)]
        uc.ribbon(rag, tail, [(0.0, 0.016), (1.0, 0.024)], n=14, cols=4, out=(0.0, -1.0, 0.1), curl=0.15, twist=0.1 * s)
    fig.add(rag, solidify=0.004)


def waistcoat(fig, P, zone):
    def folds(th, zz):
        return 0.011 + 0.002 * np.cos(9 * th) * smoothstep(1.2, 1.0, zz)

    def sd(th, zz):
        at = np.abs(th)
        hem = 0.97 - 0.04 * np.exp(-((at - 0.15) / 0.12) ** 2)
        out = np.maximum(hem - zz, np.where(zz > 1.28, (0.035 + 0.85 * (zz - 1.28)) - at, -1.0))
        out = np.maximum(out, mc.armhole(th, zz, 1.39, 0.36, 0.09))
        return np.maximum(out, zz - 1.5)
    (V, F), th, zz, edge = mc.shell(P, 0.92, 1.5, sd, folds, hole=am.HOLE_RADIUS, coal_at=am.COAL_AT)
    part = ZPart('Waistcoat', zone).add((V, F), ember=mc.scorch(edge, V, 0.3), cavity=mc.scorch_cavity(edge))
    am.buttons(part, P, [(0.02, z) for z in (1.25, 1.18, 1.11, 1.04, 0.97)], grow=0.018, size=0.0055)
    fig.add(part, solidify=0.005)


def apron(fig, P, zone, hem=0.46):
    """The undertaker's long leather apron: a bib over the chest, hung from a strap round his neck, tied round the
    waist, falling straight from the waist to below the knee over the shroud; worn shiny at the front, a pocket on it."""
    zs = np.linspace(hem, 1.37, 70)
    us = np.linspace(-1.0, 1.0, 26)
    Z, U = np.meshgrid(zs, us, indexing='ij')
    w = np.interp(Z, [hem, 0.7, 1.0, 1.08, 1.2, 1.37], [0.215, 0.205, 0.19, 0.15, 0.115, 0.105])
    X = U * w
    front = np.array([surface(P, [min(max(z, 0.95), 1.4)], np.array([0.0]))[0, 0] for z in zs])
    y_front = np.interp(Z, zs, front[:, 1]) - 0.026
    wrap_k = np.where(Z > 1.0, 1.6, 1.0)
    Y = y_front + wrap_k * X ** 2
    Y = Y - 0.012 * smoothstep(1.0, 0.6, Z)                                                 # it hangs forward a little
    Y = Y + 0.006 * np.sin(U * 9.0 + 1.3) * smoothstep(1.0, 0.65, Z) + 0.004 * np.sin(U * 15.0 + Z * 6.0) * smoothstep(0.95, 0.6, Z)
    Pg = np.stack([X, Y, Z], -1)
    edge = np.minimum(1.0 - np.abs(U), (Z - hem) / 0.1)
    cav = 0.4 * smoothstep(0.06, 0.0, edge)
    part = ZPart('Apron', zone).add((Pg.reshape(-1, 3), [uc.grid_faces(len(zs), len(us), closed=False)]), cavity=cav.ravel())
    # The pocket on the bib, the neck strap and the waist ties.
    pk = (np.abs(U) < 0.42) & (Z > 1.18) & (Z < 1.3)
    pocket = Pg + np.array([0.0, -0.005, 0.0])
    sdp = np.where(pk, -1.0, 1.0)
    Vp, Fp, used = uc.cut(pocket, sdp, closed=False)
    part.add((Vp, Fp), cavity=0.25)
    for s in SIDES:
        top = Pg[-1, 0 if s < 0 else -1]
        strap = [top, top + np.array([s * 0.005, 0.02, 0.06]), np.array([s * 0.058, 0.0, 1.535]), np.array([s * 0.04, 0.05, 1.56]),
                 np.array([0.0, 0.07, 1.555])]
        uc.ribbon(part, strap, 0.009, n=24, cols=3, out=lambda C: unit(np.column_stack([C[:, 0], C[:, 1] + 0.02, 0.3 + 0.0 * C[:, 0]])),
                  curl=0.05)
    ties = [surface(P, [1.02], np.array([t]), 0.035)[0, 0] for t in np.linspace(-1.6, -3.0, 6)]
    uc.ribbon(part, ties, 0.009, n=20, cols=3, out=lambda C: mc.outward(np.arctan2(C[:, 0], -C[:, 1])), curl=0.05)
    ties = [surface(P, [1.02], np.array([t]), 0.035)[0, 0] for t in np.linspace(1.6, 3.0, 6)]
    uc.ribbon(part, ties, 0.009, n=20, cols=3, out=lambda C: mc.outward(np.arctan2(C[:, 0], -C[:, 1])), curl=0.05)
    knot = surface(P, [1.02], np.array([np.pi]), 0.045)[0, 0]
    part.add(uc.ellipsoid(knot, (0.018, 0.012, 0.012)))
    for s in SIDES:
        uc.ribbon(part, [knot, knot + np.array([s * 0.02, 0.01, -0.06]), knot + np.array([s * 0.03, 0.012, -0.14])],
                  [(0.0, 0.009), (1.0, 0.007)], n=12, cols=3, out=(0.0, 1.0, 0.0), curl=0.05)
    fig.add(part, solidify=0.006)


# --- Arms: big working hands (Amos's), bare forearms under rolled sleeves ---

def arm(fig, s, S, W, fwd, up, curl, pole, sleeve_kind, zone, spread=0.06, thumb=0.35, garter=None, L=(0.31, 0.28)):
    E = mc.elbow(S, W, pole, *L)
    up = unit(mc.perp(up, fwd))
    if sleeve_kind == 'rolled':
        part = ZPart(f'Sleeve{s:+.0f}', zone)
        for piece in am.rolled_sleeve(S + np.array([-s * 0.012, 0.0, 0.008]), E, 0.063, 0.056, 31 + (s > 0)):
            part.add(piece)
        fig.add(part)
        am.hand_part(fig, f'Hand{s:+.0f}', W, unit(fwd), up, s, curl, spread, thumb, forearm=E - unit(E - S) * 0.012)
        if garter is not None:
            c = S + (E - S) * 0.55
            fig.add(ZPart(f'Garter{s:+.0f}', garter).add(uc.torus(c, E - S, 0.064, 0.007, squash=1.3)))
    else:
        mc.sleeve(fig, s, E, W, sleeve_kind, zone, 41 + (s > 0), shoulder=S, size=1.06)
        am.hand_part(fig, f'Hand{s:+.0f}', W, unit(fwd), up, s, curl, spread, thumb, scale=1.1)
    return E


# --- The figures ---

def hiram(pose='sit'):
    """Hiram on his stool ('sit'), or standing at ease ('stand', for the line-up)."""
    key = 'H'
    fig = mc.Ghost('Hiram', pose)
    fig.banked = True
    fig.looks = {'body': (mc.SKIN_TINT, 0x8e99a8, 0x7d7262, 0x3c3734), 'hat': (0xc9d2ce, 0x4c5361, 0xa58e62, 0x2c2a29)}
    P = am.TORSO['B']
    legs = 'sit' if pose == 'sit' else 'down'
    mc.torso_core(fig, P, am.COAL_AT, radius=am.COAL_RADIUS, z_lo=1.0)
    mc.shirt(fig, CLOTH1, profile=P, collar_y=-0.006, coal_at=am.COAL_AT, hole=am.HOLE_RADIUS, heat=0.3,
             buttons_at=(1.3, 1.2, 1.1))
    mc.braces(fig, ACCENT, width=0.016, grow=0.018, profile=P, front_t=0.62)
    sweat_rag(fig, CLOTH1)
    mc.work_coat(fig, closed=False, profile=am.COAT_B, collar_y=0.002, coal_at=am.COAL_AT, heat=0.3,
                 hole=am.HOLE_RADIUS + 0.006, seed=31)
    K = am.trousers(fig, P, legs, ACCENT)
    if pose == 'sit':
        spec = dict(lean=22.0, lean_z=1.07, lean_w=0.05, hunch=9.0)
    else:
        spec = dict(lean=2.0, hunch=5.0)
    warp = am.warp_of(spec)
    fig.warp(warp)
    S = {s: am.at(warp, am.SHOULDER['B'][s]) for s in SIDES}
    pivot = am.at(warp, am.PIVOT['B'])
    if pose == 'sit':
        # Forearms on his thighs, the big hands hanging loose over his knees.
        for s in SIDES:
            W = np.array([s * 0.125, -0.47, 0.985])
            arm(fig, s, S[s], W, (-s * 0.12, -0.35, -0.93), (s * 0.15, -0.95, 0.25), (0.45, 0.55, 0.65, 0.72),
                (s * 0.6, 0.1, -1.0), 'coat', CLOTH2, thumb=0.3)
        R = rot_z(-14.0) @ rot_x(-19.0)
    else:
        for s in SIDES:
            W = S[s] + np.array([s * 0.07, -0.08, -0.56])
            arm(fig, s, S[s], W, (s * 0.05, -0.3, -1.0), (s * 1.0, -0.15, 0.1), (0.4, 0.5, 0.6, 0.68), (s * 0.5, 0.5, 0.0),
                'coat', CLOTH2)
        R = rot_z(-6.0) @ rot_x(-3.0)
    neck = [am.at(warp, (0.0, am.PIVOT['B'][1] + 0.01, 1.47)), am.at(warp, (0.0, am.PIVOT['B'][1] - 0.002, 1.55))]
    center = am.head_parts(fig, key, R, pivot, neck, neck_radius=(0.064, 0.05, 0.046))
    fig.marks['head'] = center
    mc.add_hat(fig, 'engineer', R, pivot=pivot, tilt=(6.0, -3.0), hats=HATS)
    am.leg_wisps(fig, K, legs)
    fig.add(ZPart('Coal', SKIN).add(uc.coal_piece(*fig.coal)), material='coal')
    return fig


def thaddeus(pose='measure'):
    """Thaddeus at his bench, holding his tape out between his hands over a coffin that isn't there."""
    key = 'T'
    fig = mc.Ghost('Thaddeus', pose)
    fig.banked = True
    fig.looks = {'body': (mc.SKIN_TINT, 0xd6d1c4, 0x3e3c3f, 0x8b7356), 'hat': (0xd9e2e2, 0x8f8f86, 0x9a8a6a, 0x6e5e44)}
    P = am.TORSO['A']
    mc.torso_core(fig, P, am.COAL_AT, radius=am.COAL_RADIUS, z_lo=0.93)
    mc.shirt(fig, CLOTH1, profile=P, neck=0.035, buttons_at=(1.42,), collar=True, collar_y=-0.006, coal_at=am.COAL_AT,
             hole=am.HOLE_RADIUS, heat=0.3)
    waistcoat(fig, P, CLOTH2)
    apron(fig, P, ACCENT)
    path = [(0.0, 0.03, 1.0), (0.0, 0.06, 0.72), (0.0, 0.17, 0.5), (0.0, 0.42, 0.36), (0.0, 0.75, 0.28), (0.0, 1.1, 0.24)]
    mc.shroud_tail(fig, path=path, seed=8, strips=8)
    for part, _ in fig.parts[-1:]:
        for V, a in zip(part.V, part.attr):
            a[:, 0] = np.minimum(a[:, 0], 0.7)              # lace rather than a sheet, as Amos's
    spec = dict(lean=10.0, hunch=15.0)
    warp = am.warp_of(spec)
    fig.warp(warp)
    S = {s: am.at(warp, am.SHOULDER['A'][s]) for s in SIDES}
    pivot = am.at(warp, am.PIVOT['A'])
    hands = {}
    targets = {-1.0: (np.array([-0.235, -0.38, 1.02]), (0.42, -0.85, -0.3), (-0.15, 0.1, 1.0)),
               1.0: (np.array([0.245, -0.37, 1.035]), (-0.42, -0.85, -0.28), (0.15, 0.1, 1.0))}
    for s in SIDES:
        W, fwd, up = targets[s]
        arm(fig, s, S[s], W, fwd, up, (0.95, 1.05, 1.1, 1.05), (s * 0.7, 0.3, -0.6), 'rolled', CLOTH1, spread=0.03,
            thumb=0.75, garter=CLOTH2)
        hands[s] = am.grip_point(W, unit(fwd), unit(mc.perp(up, fwd)), s)[0]
    # The tape, stretched between his fingers with a little sag; its tail and its leather case hanging from his left
    # hand, the case's ring over his right thumb.
    tape = ZPart('Tape', CLOTH1)
    a, b = hands[-1.0], hands[1.0]
    mid = (a + b) / 2 + np.array([0.0, -0.01, -0.025])
    pts = spline([a, mid, b], 40)
    uc.ribbon(tape, pts, 0.008, n=40, cols=3, out=(0.0, 0.0, 1.0), curl=0.02)
    for V, at_ in zip(tape.V, tape.attr):
        u = np.linalg.norm(V - a, axis=1)
        at_[:, 1] = 0.5 * (np.sin(u * 2.0 * np.pi / 0.0254) > 0.75)             # the inch marks
    tail = [b, b + np.array([0.02, -0.01, -0.1]), b + np.array([0.0, 0.02, -0.22]), b + np.array([-0.03, 0.01, -0.3])]
    uc.ribbon(tape, spline(tail, 30), 0.008, n=30, cols=3, out=(0.0, -1.0, 0.0), curl=0.02, twist=0.3)
    fig.add(tape, solidify=0.002)
    case = ZPart('TapeCase', ACCENT)
    c = a + np.array([0.0, 0.0, -0.06])
    case.add(uc.ellipsoid(c, (0.032, 0.011, 0.032), segs=24, rings=10), cavity=0.2)
    case.add(uc.torus(a + np.array([0.0, 0.0, -0.02]), (0.0, 1.0, 0.0), 0.012, 0.002, segs=16, sides=6))
    fig.add(case)
    R = rot_z(8.0) @ rot_x(14.0)
    neck = [am.at(warp, (0.0, am.PIVOT['A'][1] + 0.01, 1.47)), am.at(warp, (0.0, am.PIVOT['A'][1] - 0.002, 1.55))]
    center = am.head_parts(fig, key, R, pivot, neck, neck_radius=(0.058, 0.045, 0.042))
    fig.marks['head'] = center
    spectacles(fig, R, center)
    fig.add(ZPart('Coal', SKIN).add(uc.coal_piece(*fig.coal)), material='coal')
    return fig


_cache = {}


def figure(name, pose=None):
    pose = pose or ('sit' if name == 'Hiram' else 'measure')
    if name == 'Thaddeus':
        pose = 'measure'
    if (name, pose) not in _cache:
        t0 = time.time()
        _cache[(name, pose)] = hiram(pose) if name == 'Hiram' else thaddeus(pose)
        log(f'{name} {pose}: {_cache[(name, pose)].triangles()} triangles (concept density), built in {time.time() - t0:.1f} s')
    return _cache[(name, pose)]


BUDGET = {'Hiram': dict(body=10.4, cap=0.9, head_share=3.2), 'Thaddeus': dict(body=10.6, spectacles=0.35, head_share=3.2)}


def estimate(name):
    b = BUDGET[name]
    total = sum(v for k, v in b.items() if k != 'head_share')
    return total, ', '.join(f'{k} {v:.1f}k' for k, v in b.items() if k != 'head_share') + f'; its head {b["head_share"]:.1f}k'


# --- Stages ---

def boards(where, origin, length, across, n, depth, mat, name='Boards', seed=1):
    """n boards side by side from the corner origin, each running along length, together spanning across; each a
    little out of true."""
    rng = np.random.default_rng(seed)
    o, L, A = np.asarray(origin, float), np.asarray(length, float), np.asarray(across, float)
    w = np.linalg.norm(A) / n
    lu, au = unit(L), unit(A)
    nu = unit(np.cross(lu, au))
    for k in range(n):
        c = o + A * (k + 0.5) / n + L * 0.5 + nu * rng.uniform(-0.006, 0.006)
        R = np.column_stack([au, nu, lu]) @ rotation((0, 1, 0), rng.uniform(-0.01, 0.01))
        V, F = uc.box_piece(c, (w * 0.485, depth / 2.0, np.linalg.norm(L) / 2.0), R)
        uv = np.column_stack([V @ lu * 0.5, V @ au * 0.5 + k * 0.37])
        obj = uc.mesh_object(name, V, F, mat, where, uvs=uv)
        obj.data.polygons.foreach_set('use_smooth', np.zeros(len(obj.data.polygons), dtype=bool))


def cylinder(where, base, axis, radius, length, mat, segs=40, name='Cyl'):
    V, F = mc.lathe([(0.0, radius), (length, radius)], base, axis, segs=segs)
    k = unit(axis)
    uv = np.column_stack([np.arctan2(V[:, 1] - base[1], V[:, 0] - base[0]) * radius, V @ k])
    obj = uc.mesh_object(name, V, F, mat, where, uvs=uv)
    return obj


def winch_house(where, seed=3):
    """The ore lift's winch house: a plank floor, board walls at the back and right, open on the landing side to the
    west; the vertical boiler and its stack, the winch drum on its timber frame with the cable wound on it, the brake
    lever, the coal bin, Hiram's stool and his oilcan."""
    rng = np.random.default_rng(seed)
    wood = lt.material('WoodPlanks', name='WinchBoards', tint=0xb0a08a)
    dark = lt.material('WoodPlanks', name='WinchTimber', tint=0x857563)
    iron = lt.material('MetalRust', name='BoilerIron', tint=0x9c8f84)
    worn = lt.material('MetalWorn', name='WinchIron', tint=0x8a8a86)
    boards(where, (-3.0, -3.0, -0.05), (0.0, 5.6, 0.0), (5.6, 0.0, 0.0), 22, 0.05, wood, 'Floor')
    boards(where, (-3.0, 2.62, 0.0), (0.0, 0.0, 3.2), (5.6, 0.0, 0.0), 26, 0.04, wood, 'BackWall', seed=2)
    boards(where, (2.62, -3.0, 0.0), (0.0, 0.0, 3.2), (0.0, 5.6, 0.0), 26, 0.04, wood, 'SideWall', seed=3)
    for x in (-2.9, 0.0, 2.55):
        mc.timber(where, (x, 2.5, 0.0), (x, 2.5, 3.3), (0.18, 0.18), dark)
    mc.timber(where, (-3.0, 2.5, 3.25), (2.6, 2.5, 3.25), (0.2, 0.2), dark)
    mc.timber(where, (2.5, -3.0, 3.25), (2.5, 2.6, 3.25), (0.2, 0.2), dark)
    # The boiler: a riveted upright drum, its dome, the firebox door, the gauge, the stack up through the roof.
    bx, by = 1.35, 1.45
    cylinder(where, (bx, by, 0.0), (0, 0, 1), 0.52, 1.85, iron, name='Boiler')
    V, F = uc.ellipsoid((bx, by, 1.85), (0.52, 0.52, 0.22), segs=40, rings=12)
    uc.mesh_object('Dome', V, F, iron, where, uvs=mc.box_uv(V))
    cylinder(where, (bx, by, 1.9), (0, 0, 1), 0.13, 2.0, iron, segs=24, name='Stack')
    for z in (0.08, 0.62, 1.2, 1.78):
        V, F = uc.torus((bx, by, z), (0, 0, 1), 0.525, 0.018, segs=48, sides=8)
        uc.mesh_object('Band', V, F, worn, where, uvs=mc.box_uv(V))
    V, F = uc.box_piece((bx - 0.3, by - 0.42, 0.45), (0.2, 0.06, 0.17), rotation((0, 0, 1), 0.6))
    uc.mesh_object('Firebox', V, F, worn, where, uvs=mc.box_uv(V))
    V, F = uc.ellipsoid((bx - 0.43, by - 0.25, 1.35), (0.07, 0.025, 0.07), rotation((0, 0, 1), 1.05).T, 24, 8)
    uc.mesh_object('Gauge', V, F, mc.flat('GaugeFace', 0xd8d0bc, 0.6), where)
    # The winch: the drum on its frame, the cable wound on it and running out to the landing, the brake lever.
    dx, dy = -1.0, 1.7
    cylinder(where, (dx - 0.6, dy, 0.75), (1, 0, 0), 0.34, 1.2, worn, name='Drum')
    for k in range(9):
        V, F = uc.torus((dx - 0.5 + k * 0.125, dy, 0.75), (1, 0, 0), 0.355, 0.022, segs=40, sides=8)
        uc.mesh_object('Cable', V, F, mc.flat('Cable', 0x4b4643, 0.5), where)
    for x in (dx - 0.68, dx + 0.68):
        mc.timber(where, (x, dy - 0.4, 0.0), (x, dy, 0.82), (0.16, 0.16), dark)
        mc.timber(where, (x, dy + 0.4, 0.0), (x, dy, 0.82), (0.16, 0.16), dark)
    mc.timber(where, (dx - 0.2, dy - 0.6, 0.0), (dx - 0.15, dy - 0.85, 1.25), (0.05, 0.05), worn, 'Brake')
    V, F = uc.ellipsoid((dx - 0.15, dy - 0.86, 1.27), (0.035, 0.035, 0.035))
    uc.mesh_object('Knob', V, F, worn, where, uvs=mc.box_uv(V))
    # The coal bin, nearly empty (the boiler's stone cold), and a scatter of coal.
    for o, L in (((2.0, -0.6, 0.0), (0.0, 1.2, 0.0)), ((1.2, -0.6, 0.0), (0.0, 1.2, 0.0)), ((1.2, -0.62, 0.0), (0.8, 0.0, 0.0))):
        boards(where, o, L, (0.0, 0.0, 0.55), 4, 0.04, dark, 'Bin')
    coal = mc.flat('CoalLumps', 0x1f1d1c, 0.5)
    for k in range(18):
        p = (rng.uniform(1.3, 1.9), rng.uniform(-0.5, 0.5), rng.uniform(0.02, 0.08))
        mc.rock(where, p, (0.05, 0.04, 0.035), 60 + k, coal, rng.uniform(0, 3))
    # Hiram's stool, and his long-spouted oilcan on the floor by it.
    sx, sy = 0.15, 0.55
    V, F = mc.lathe([(STOOL - 0.045, 0.0), (STOOL - 0.045, 0.17), (STOOL, 0.17), (STOOL, 0.0)], (sx, sy, 0.0), (0, 0, 1), 32)
    uc.mesh_object('Seat', V, F, dark, where, uvs=mc.box_uv(V))
    for k in range(3):
        a = k * 2.0 * np.pi / 3 + 0.3
        foot = np.array([sx + 0.24 * math.cos(a), sy + 0.24 * math.sin(a), 0.0])
        mc.timber(where, foot, (sx + 0.1 * math.cos(a), sy + 0.1 * math.sin(a), STOOL - 0.04), (0.04, 0.04), dark, 'Leg')
    V, F = mc.lathe([(0.0, 0.0), (0.0, 0.07), (0.07, 0.075), (0.1, 0.05), (0.11, 0.0)], (sx + 0.42, sy - 0.15, 0.0), (0, 0, 1), 24)
    uc.mesh_object('Oilcan', V, F, worn, where, uvs=mc.box_uv(V))
    mc.timber(where, (sx + 0.42, sy - 0.15, 0.1), (sx + 0.42, sy - 0.42, 0.3), (0.012, 0.012), worn, 'Spout')
    return (sx, sy)


def cold_room(where, seed=4):
    """Bright & Daughter's cold room under the workshop: a stone vault, flags underfoot, two coffins waiting on their
    racks against the back wall, his two trestles standing empty before him, a small window high in the west wall at
    the street's level."""
    rng = np.random.default_rng(seed)
    stone = lt.material('StoneWall', name='CellarStone', tint=0xb8b0a4)
    flags = lt.material('GroundStony', name='CellarFlags', tint=0xa49c90)
    wood = lt.material('WoodPlanks', name='CellarWood', tint=0x9b8a74)
    pale = lt.material('WoodPlanks', name='CoffinPine', tint=0xc9b796)
    V = np.array([(-2.4, -3.0, 0.0), (2.4, -3.0, 0.0), (2.4, 2.0, 0.0), (-2.4, 2.0, 0.0)])
    uc.mesh_object('Floor', V, [np.array([[0, 1, 2, 3]])], flags, where, uvs=V[:, :2] * 0.6)
    # The vault: walls up to the springing, then a barrel arch across x; the back wall; the west wall with its window.
    sec = [(-2.2, z) for z in np.linspace(0.0, 1.7, 8)[:-1]] + \
          [(2.2 * math.cos(t), 1.7 + 0.9 * math.sin(t)) for t in np.linspace(np.pi, 0.0, 24)] + \
          [(2.2, z) for z in np.linspace(1.7, 0.0, 8)[1:]]
    sec = np.array(sec)
    ys = np.linspace(-3.5, 1.8, 40)
    P = np.stack([np.broadcast_to(sec[None, :, 0], (len(ys), len(sec))), np.broadcast_to(ys[:, None], (len(ys), len(sec))),
                  np.broadcast_to(sec[None, :, 1], (len(ys), len(sec)))], -1).copy()
    win = (P[..., 0] < -2.1) & (np.abs(P[..., 1] + 0.4) < 0.45) & (P[..., 2] > 1.75) & (P[..., 2] < 2.25)
    Vv, Fv, used = uc.cut(P, np.where(win, 1.0, -1.0), closed=False)
    u = np.cumsum(np.concatenate([[0.0], np.linalg.norm(np.diff(sec, axis=0), axis=1)]))
    uv = np.column_stack([np.broadcast_to(u[None, :], P.shape[:2]).ravel()[used], P[..., 1].ravel()[used]]) * 0.6
    uc.mesh_object('Vault', Vv, Fv, stone, where, uvs=uv)
    # The back wall: a whole rectangle just behind the vault's end (the vault hides what stands past it).
    Vb = np.array([(-2.4, 1.78, 0.0), (2.4, 1.78, 0.0), (2.4, 1.78, 2.75), (-2.4, 1.78, 2.75)])
    uc.mesh_object('BackWall', Vb, [np.array([[0, 3, 2, 1]])], stone, where, uvs=np.column_stack([Vb[:, 0], Vb[:, 2]]) * 0.6)
    for y in (-0.85, 0.05):                              # the window's bars
        mc.timber(where, (-2.2, y + 0.0, 1.75), (-2.2, y, 2.25), (0.025, 0.025), mc.flat('Bar', 0x3a3633, 0.6), 'Bar')
    for z in (1.92, 2.08):
        mc.timber(where, (-2.2, -0.85, z), (-2.2, 0.05, z), (0.025, 0.025), mc.flat('Bar', 0x3a3633, 0.6), 'Bar')
    # A coffin waiting on its rack along the back wall, and another stood on end against the wall, its lid on.
    prof = np.array([(-0.95, 0.12), (-0.5, 0.27), (0.95, 0.17)])
    outline = [(px, py) for px, py in prof] + [(px, -py) for px, py in prof[::-1]]

    def coffin(M):
        ring = np.array([(px, py, 0.0) for px, py in outline])
        V, F = uc.orient(uc.closed_loft(np.array([ring, ring + (0.0, 0.0, 0.3), ring * (0.97, 0.97, 1.0) + (0.0, 0.0, 0.34)])))
        V = V @ M[:3, :3].T + M[:3, 3]
        obj = uc.mesh_object('Coffin', V, F, pale, where, uvs=mc.box_uv(V))
        obj.data.polygons.foreach_set('use_smooth', np.zeros(len(obj.data.polygons), dtype=bool))
    x = -1.1
    for dx in (-0.7, 0.7):
        mc.timber(where, (x + dx, 1.45, 0.0), (x + dx, 1.45, 0.62), (0.07, 0.07), wood, 'Rack')
    mc.timber(where, (x - 0.8, 1.45, 0.64), (x + 0.8, 1.45, 0.64), (0.5, 0.06), wood, 'Rack')
    coffin(mc.frame((x, 1.45, 0.67), (1.0, 0.0, 0.0), (0.0, 1.0, 0.0), (0.0, 0.0, 1.0)))
    lean = math.radians(8.0)
    up = np.array([0.0, math.sin(lean), math.cos(lean)])          # its head end leaning back to the wall
    coffin(mc.frame((1.45, 1.6, 0.97), up, (-1.0, 0.0, 0.0), np.cross(up, (-1.0, 0.0, 0.0))))   # its lid to the room
    # His trestles, empty.
    for x in (-0.48, 0.48):
        y0 = -0.75
        for s in (-1.0, 1.0):
            mc.timber(where, (x - 0.2 * s, y0 - 0.22, 0.0), (x, y0 - 0.02, 0.7), (0.05, 0.05), wood, 'Leg')
            mc.timber(where, (x - 0.2 * s, y0 + 0.22, 0.0), (x, y0 + 0.02, 0.7), (0.05, 0.05), wood, 'Leg')
        mc.timber(where, (x, y0 - 0.35, 0.73), (x, y0 + 0.35, 0.73), (0.09, 0.07), wood, 'Beam')
    # A candle stub on the near trestle's end, cold.
    V, F = mc.lathe([(0.0, 0.0), (0.0, 0.02), (0.06, 0.018), (0.065, 0.0)], (0.48, -1.05, 0.765), (0, 0, 1), 16)
    uc.mesh_object('Stub', V, F, mc.flat('Tallow', 0xd9cfb8, 0.8), where)
    # Sawdust and shavings swept against the walls: a few low heaps.
    dust = lt.material('GroundStony', name='Sawdust', tint=0xd8c8a8)
    for k in range(5):
        p = (rng.choice((-1.9, 1.9)) + rng.uniform(-0.15, 0.15), rng.uniform(-2.5, 1.0), -0.03)
        mc.rock(where, p, (0.25, 0.35, 0.06), 80 + k, dust, rng.uniform(0, 3))


def golden_through(where, toward, strength=4.4):
    am.golden_sky(0.7)
    sun = uc.sun(toward, strength=strength, color=mc.SUN_COLOR, where=where)
    sun.data.angle = math.radians(1.5)
    return sun


def moonlight(where, toward, strength=1.2, world=0.8):
    mc.cool_world(strength=world)
    sun = uc.sun(toward, strength=strength, color=(0.62, 0.74, 1.0), where=where)
    sun.data.angle = math.radians(1.0)
    return sun


# --- Shots ---

def shot_front():
    """Both from the front and the side, standing at ease, beside a 1.8 m post and a 1.8 m figure."""
    uc.reset()
    where = uc.collection('Front')
    figs = [(figure('Hiram', 'stand'), 0.0), (figure('Hiram', 'stand'), math.pi / 2), (figure('Thaddeus'), 0.0),
            (figure('Thaddeus'), math.pi / 2)]
    items = [(-0.08, 0.08), (-0.25, 0.25)] + [(f.bounds(t)[0][0], f.bounds(t)[1][0]) for f, t in figs]
    xs, total = uc.lineup(items, gap=0.45)
    uc.post(where, (xs[0], 0.0, 0.0))
    uc.label('1.8 m', (xs[0] + 0.1, -0.05, 1.78), 0.075, align='LEFT')
    mc.mannequin(where, (xs[1], 0.0, 0.0))
    for x, (f, t), (lo, hi), name in zip(xs[2:], figs, items[2:], ('Hiram', 'Hiram, side', 'Thaddeus', 'Thaddeus, side')):
        mc.instantiate(f, where, (x, 0.0, 0.0), t)
        uc.label(name, (x + (lo + hi) / 2, -0.7, 0.05), 0.1)
    mc.studio(where)
    width = total + 0.6
    uc.camera((0.0, -30.0, 1.1 + 30.0 * math.tan(math.radians(6.0))), (0.0, 0.0, 1.1), ortho=width)
    mc.render(os.path.join(OUT['Hiram'], 'Friends_front.png'), (2400, int(2400 * 2.45 / width)), 96)


SIT_TURN = math.radians(-30.0)


def hiram_scene(where):
    sx, sy = winch_house(where)
    fig = figure('Hiram', 'sit')
    seat_local = np.array([0.0, 0.025, am.SEAT_Z])
    loc = np.array([sx, sy, STOOL]) - rotation((0, 0, 1), SIT_TURN) @ seat_local
    return fig, tuple(loc)


def shot_hiram(mode):
    uc.reset()
    where = uc.collection('Hiram')
    fig, loc = hiram_scene(where)
    toward = am.bearing(247.5, 15.0)
    if mode == 'golden':
        golden_through(where, toward)
        mc.instantiate(fig, where, loc, SIT_TURN)
        exposure = 0.0
    else:
        moonlight(where, toward, strength=0.9, world=0.55)
        mc.area_light(where, (-2.6, -2.2, 2.0), (0.2, 0.6, 1.0), 2.5, 45.0)
        mc.instantiate(fig, where, loc, SIT_TURN)
        exposure = 0.6
    eye = Vector((-1.75, -2.55, 1.3))
    mc.lens_shot(os.path.join(OUT['Hiram'], f'Hiram_{mode}.png'), eye, Vector((0.3, 0.55, 0.95)), 30.0, (1600, 1100), 96,
                 exposure=exposure)


def shot_thaddeus(mode):
    uc.reset()
    where = uc.collection('Thaddeus')
    cold_room(where)
    fig = figure('Thaddeus')
    toward = (-0.86, -0.25, 0.44)                # in through the high window in the west wall
    if mode == 'golden':
        golden_through(where, toward, strength=5.0)
        exposure = 0.3
    else:
        moonlight(where, toward, strength=2.4, world=1.0)
        mc.area_light(where, (1.6, -2.6, 2.1), (-0.2, 0.3, 1.0), 2.0, 160.0)
        exposure = 0.6
    mc.instantiate(fig, where, (0.0, -0.05, 0.0), 0.0)
    eye = Vector((1.25, -2.9, 1.45))
    mc.lens_shot(os.path.join(OUT['Thaddeus'], f'Thaddeus_{mode}.png'), eye, Vector((-0.05, -0.35, 1.1)), 30.0,
                 (1600, 1100), 96, exposure=exposure)


def shot_debug(name, pose=None):
    uc.reset()
    where = uc.collection('Debug')
    fig = figure(name, pose)
    mc.instantiate(fig, where)
    mc.studio(where, 0x9d9484)
    c = Vector((0.0, -0.15, 1.0))
    paths = []
    for view, d in (('front', (0.0, -1.0, 0.05)), ('three', (0.8, -1.0, 0.1)), ('side', (1.0, 0.0, 0.05)),
                    ('back', (0.3, 1.0, 0.1))):
        cam = uc.camera(c + Vector(d).normalized() * 3.6, c, lens=50.0)
        paths.append(os.path.join(mc.WORK, f'debug_{name}_{fig.pose}_{view}.png'))
        uc.render(paths[-1], (560, 900), samples=12)
        bpy.data.objects.remove(cam)
    uc.compose(paths, os.path.join(mc.WORK, f'debug_{name}_{fig.pose}.png'))
    head = Vector(fig.marks['head'])
    cam = uc.camera(head + Vector((0.35, -1.0, 0.05)).normalized() * 0.8, head + Vector((0.0, 0.0, 0.02)), lens=70.0)
    uc.render(os.path.join(mc.WORK, f'debug_{name}_{fig.pose}_head.png'), (700, 700), samples=16)
    bpy.data.objects.remove(cam)


def sheet_section(s):
    s.heading('Hiram Coyle and Thaddeus Bright: one look each (friends; their coals banked low)')
    s.image(os.path.join(OUT['Hiram'], 'Friends_front.png'),
            'Standing at ease, front and side, beside a 1.8 m figure and post (Hiram sits in the game).')
    th, tp = estimate('Hiram')
    s.row([(os.path.join(OUT['Hiram'], 'Hiram_golden.png'), f'Hiram Coyle, the dead engineman   ~{th:.1f}k tris',
            'On his stool by the cold boiler, the sun in through\nthe landing: duck coat, sweat rag, goggles on his cap.'),
           (os.path.join(OUT['Hiram'], 'Hiram_ghost.png'), '',
            f'The winch house at night, cool ghost light.\n({tp})')])
    tt, tq = estimate('Thaddeus')
    s.row([(os.path.join(OUT['Thaddeus'], 'Thaddeus_golden.png'), f'Thaddeus Bright, the undertaker   ~{tt:.1f}k tris',
            'Measuring a coffin that isn\'t there, the low sun in\nthrough the cellar window: apron, garters, spectacles.'),
           (os.path.join(OUT['Thaddeus'], 'Thaddeus_ghost.png'), '',
            f'The cold room by night, cool ghost light.\n({tq})')])


def main():
    names = [a for a in ARGV if a in NAMES] or list(NAMES)
    if 'debug' in ARGV:
        for name in names:
            poses = [p for p in ('sit', 'stand') if p in ARGV] if name == 'Hiram' else [None]
            for pose in poses or [None]:
                shot_debug(name, pose)
        return
    shots = [a for a in ARGV if a in ('front', 'golden', 'ghost')]
    if not shots:
        if '--preview' not in ARGV and not lt.want_preview():
            for name in names:
                figure(name)
            return
        shots = ['front', 'golden', 'ghost']
    if 'front' in shots:
        shot_front()
    for name in names:
        for mode in ('golden', 'ghost'):
            if mode in shots:
                (shot_hiram if name == 'Hiram' else shot_thaddeus)(mode)


if __name__ == '__main__':
    main()
