"""Concepts for the Providence's dead miners (Docs/Areas/RansomsRest_Expansion.md, section 6, "Dead miners": a new kind
of the Unpaid on its rig, ADeadMinerCreature), for the user to pick from (2026-10-13). Concept art kept in Art/Backlog
(see its README): nothing in the game uses it. The pick becomes a game model on the Unpaid's rig and bones (Unpaid.py),
its hat and lamp a static mesh on the 'hat' bone as SM_UnpaidHat is, its pick a static mesh on a new 'pick' bone under
hand_r (as Amos's fork rides 'fork').

  A  Canvas and carbide: the day shift as they died, about 1900. A hip-length duck coat over a flannel shirt, braces, a
     neckerchief; a felt hat with its brim pinned up in front or a leather cap, and on it a brass carbide lamp with a
     polished reflector the size of a saucer: the weak spot is a bright disc on the forehead.
  B  The candle men: the oldest of the forty years' dead, Cornish hard-rock men. A long canvas smock belted with rope,
     a bunch of spare candles hung by their wicks from a button; a stiff hard-felt hat with a tallow candle stuck to its
     front in a lump of clay: the weak spot is a standing flame over the brow.
  C  The muckers: the lower drift's men, stripped to undershirts in the heat, sleeves rolled, heavy braces and a belt
     with the lunch pail on it, a bandana over nose and mouth against the dust; a leather cap with an oil-wick "teapot"
     lamp whose big smoky flame stands off the brow on its spout.

Every option is the Unpaid's kin (pale, gaunt, the hungry face, the shroud below the waist, a coal burning through the
clothes on the left of the chest in the rank's color) and carries a pick. Each has three outfits for a pack (tints, a
hat or a coat swapped, the pick dragged or shouldered), so four never look cloned. The head lamp burns a pale warm white
(no rarity color); in the pick swing's wind-up it flares (the ghost shots show one flared).

This file is also the kit for the mountain's other dead (ForemanConcepts.py, LiftAndCellarConcepts.py import it): the
ghost and lamp materials, the body and its garments, hats, lamps, the pick, the stages (the Providence's mouth in the
level's golden late afternoon; a gallery, the winch house and the cold room under cool ghost light), the line-up at
scale, the 30 m silhouettes and the look sheet (Saved/ArtPreviews/RansomsRest/LookSheets/38_Miners.jpg).

Renders go to Saved/ArtPreviews/RansomsRest/Concepts/Miners/: Miners_front.png (all nine outfits from the front beside a
1.8 m post and an Unpaid), per option Miners_A_golden.png (the three outfits at the mine mouth, three-quarter view, beside
a 1.8 m figure) and Miners_A_ghost.png (a pack in a gallery under cool ghost light, the nearest one winding up with its
lamp flared); Lineup.png and Far30m.png (in Concepts/, every new character beside an Unpaid and Abel).

    powershell -NoProfile -File Tools\\artrun.ps1 -Script Art\\Backlog\\Creatures\\MinerConcepts.py -Preview
    ... -ScriptArgs A,golden          only some: options A B C; shots front golden ghost (no shot named: all three)
    ... -ScriptArgs lineup,far,sheet  the line-up, the 30 m strip and the look sheet (run after the other two scripts)
    ... -ScriptArgs A,debug           quick checks into Intermediate/MinerConcepts ('fast' halves any render)
"""
import math
import os
import sys
import time

import bpy
import numpy as np
from mathutils import Vector

if __name__ == '__main__':
    sys.modules.setdefault('MinerConcepts', sys.modules[__name__])      # the other concept scripts import this kit
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import UnpaidConcepts as uc  # noqa: E402  (its toolkit, ghost, stage and the Unpaid; importing renders nothing)
import AmosConcepts as am  # noqa: E402  (zoned parts, the ghost material with its knobs, sleeves, the golden sky)
import looter_textures as lt  # noqa: E402

ARGV = [a for arg in (sys.argv[sys.argv.index('--') + 1:] if '--' in sys.argv else []) for a in arg.split(',') if a]
CONCEPTS = os.path.join(lt.REPO, 'Saved', 'ArtPreviews', 'RansomsRest', 'Concepts')
OUT = os.path.join(CONCEPTS, 'Miners')
SHEET = os.path.join(lt.REPO, 'Saved', 'ArtPreviews', 'RansomsRest', 'LookSheets', '38_Miners.jpg')
WORK = os.path.join(lt.REPO, 'Intermediate', 'MinerConcepts')
FAST = 'fast' in ARGV
OPTIONS = ('A', 'B', 'C')
TITLES = {'A': 'Canvas and carbide', 'B': 'The candle men', 'C': 'The muckers'}
SKIN, CLOTH1, CLOTH2, ACCENT = 0, 1, 2, 3
SIDES = (1.0, -1.0)                         # +1 his left (+X), -1 his right
smoothstep, unit, rotation, wrap, spline = uc.smoothstep, uc.unit, uc.rotation, uc.wrap, uc.spline
ZPart = am.ZPart
rot_x, rot_y, rot_z = am.rot_x, am.rot_y, am.rot_z

# The coal in the rank's color (CreatureRankSettings: Basic's dull red, Restless blue, Soulfed and the bosses' gold).
BASIC = (0xb02a18, 6.0)
RESTLESS = (0x7caaff, 3.0)
SOULFED = (0xffcc00, 3.4)
FLAME = 0xfff0d8                            # the head lamps' flame: a pale, warm white, no rarity color
SKIN_TINT = 0xbac4c6                        # the Unpaid's approved ghost skin
SUN_COLOR = am.SUN_COLOR


def log(message):
    print(f'MINERS: {message}', flush=True)


def g(x, s):
    return np.exp(-(np.asarray(x, float) / s) ** 2)


def outward(theta):
    return np.stack([np.sin(theta), -np.cos(theta), np.zeros_like(theta)], -1)


def surface(profile, z, theta, grow=0.0):
    return uc.torso_points(profile, z, theta, grow)


def perp(v, axis):
    """v with its part along axis removed."""
    v, a = np.asarray(v, float), unit(axis)
    return v - a * np.dot(v, a)


def frame(origin, x, y, z):
    return am.frame_matrix(np.asarray(origin, float), np.asarray(x, float), np.asarray(y, float), np.asarray(z, float))


def translate(t):
    M = np.eye(4)
    M[:3, 3] = t
    return M


def turn4(R):
    M = np.eye(4)
    M[:3, :3] = R
    return M


# --- The figure ---

class Ghost(uc.Figure):
    """A concept in a pose: its parts (each drawn with one of the figure's looks), its coal, its flames (center, size:
    the head lamp's glow) and named points (marks) that the warps carry along."""

    def __init__(self, key, pose, variant=0):
        super().__init__(key, pose)
        self.variant = variant
        self.looks = {}                     # material key -> four zone colors (skin, cloth 1, cloth 2, accent)
        self.flames = []
        self.marks = {}
        self.scale = 1.0
        self.rank = BASIC
        self.banked = False                 # a friend's coal: small, under ash

    def warp(self, fn):
        super().warp(fn)
        self.flames = [(fn(np.atleast_2d(c))[0], s) for c, s in self.flames]
        self.marks = {k: ({j: fn(np.atleast_2d(p))[0] for j, p in v.items()} if isinstance(v, dict)
                          else fn(np.atleast_2d(v))[0]) for k, v in self.marks.items()}


# --- Materials ---

_mats = {}


def flame_material(strength=1.0):
    """The lamp's flame: an emissive surface, white-hot (the game: an emissive card on the lamp)."""
    key = ('flame', round(strength, 3))
    if key not in _mats:
        mat = bpy.data.materials.new('LampFlame')
        mat.use_nodes = True
        nodes, links = mat.node_tree.nodes, mat.node_tree.links
        nodes.clear()
        out = nodes.new('ShaderNodeOutputMaterial')
        emit = nodes.new('ShaderNodeEmission')
        emit.inputs['Color'].default_value = lt.hex_color(FLAME)
        emit.inputs['Strength'].default_value = 22.0 * strength
        links.new(emit.outputs['Emission'], out.inputs['Surface'])
        _mats[key] = mat
    return _mats[key]


def halo_material(strength=1.0, color=FLAME):
    """A soft glow round the flame (what the game's bloom and the lamp card's falloff make): brightest where the ball
    faces the eye, gone at its edge, blended."""
    key = ('halo', round(strength, 3), color)
    if key not in _mats:
        mat = bpy.data.materials.new('LampHalo')
        mat.use_nodes = True
        nodes, links = mat.node_tree.nodes, mat.node_tree.links
        nodes.clear()
        out = nodes.new('ShaderNodeOutputMaterial')
        weight = nodes.new('ShaderNodeLayerWeight')
        weight.inputs['Blend'].default_value = 0.5
        k = uc._math(nodes, links, 'SUBTRACT', 1.0, weight.outputs['Facing'])
        k = uc._math(nodes, links, 'POWER', k, 2.6)
        k = uc._math(nodes, links, 'MULTIPLY', k, 0.85, clamp=True)
        emit = nodes.new('ShaderNodeEmission')
        emit.inputs['Color'].default_value = lt.hex_color(color)
        emit.inputs['Strength'].default_value = 2.2 * strength
        clear = nodes.new('ShaderNodeBsdfTransparent')
        mix = nodes.new('ShaderNodeMixShader')
        links.new(k, mix.inputs['Fac'])
        links.new(clear.outputs['BSDF'], mix.inputs[1])
        links.new(emit.outputs['Emission'], mix.inputs[2])
        links.new(mix.outputs['Shader'], out.inputs['Surface'])
        mat.surface_render_method = 'BLENDED'
        mat.use_backface_culling = True
        _mats[key] = mat
    return _mats[key]


def flat(name, color, rough=0.9, emit=0.0):
    return uc.flat_material(name, color, rough, emit)


def material_for(fig, key, rank, flare):
    """The figure's look for a part: the body's ghost (its ember edge in the rank's color), the props' ghosts (the
    lamp's reflector lit in the flame's white through the ember channel, as M_Ghost's EmberColor would), the coal."""
    if key == 'coal':
        return am.banked_coal_material() if fig.banked else uc.coal_material(rank)
    if key == 'flame':
        return flame_material(flare)
    colors = fig.looks[key]
    name = f'{fig.key}{fig.variant}{key}'
    if key == 'body':
        ember = am.EMBER if fig.banked else rank
        return am.ghost_material(name, colors, ember=ember, edge=0.35 if fig.banked else 0.6)
    if key == 'lamp':
        return am.ghost_material(name, colors, ember=(FLAME, 1.7 * flare), edge=1.0, rim=1.2, glow=0.1)
    if key == 'hat':
        return am.ghost_material(name, colors, ember=rank, rim=1.0, glow=0.08, power=3.6)
    return am.ghost_material(name, colors, ember=rank, rim=1.3, glow=0.08)


def build(part, material, where, location=(0.0, 0.0, 0.0), turn=0.0, scale=1.0, solidify=0.0, shadow=True, **_):
    obj = am.build_part(part, material, where, solidify=solidify, location=location, turn=turn, shadow=shadow)
    obj.scale = (scale, scale, scale)
    if solidify:
        obj.modifiers['Thickness'].thickness = solidify
    return obj


def to_world(p, location, turn, scale):
    return rotation((0.0, 0.0, 1.0), turn) @ (np.asarray(p, float) * scale) + np.asarray(location, float)


def instantiate(fig, where, location=(0.0, 0.0, 0.0), turn=0.0, rank=None, flare=1.0, light=0.0, scale=None):
    """Builds the figure (scaled about its feet), with a halo round each flame (bigger when it flares) and, if asked, a
    small warm point light there (energy in watts at size 1)."""
    rank = rank or fig.rank
    scale = fig.scale if scale is None else scale
    objs = []
    for part, options in fig.parts:
        mat = material_for(fig, options.get('material', 'body'), rank, flare)
        objs.append(build(part, mat, where, location, turn, scale,
                          **{k: v for k, v in options.items() if k != 'material'}))
    for c, size in fig.flames:
        p = to_world(c, location, turn, scale)
        r = size * scale * (1.0 + 0.3 * (flare - 1.0))
        V, F = uc.ellipsoid(p, (r, r, r * 1.15), segs=24, rings=14)
        halo = uc.mesh_object('Halo', V, F, halo_material(flare), where)
        halo.visible_shadow = False
        objs.append(halo)
        if light:
            lamp = bpy.data.objects.new('LampLight', bpy.data.lights.new('LampLight', 'POINT'))
            lamp.data.energy = light * flare * scale * scale
            lamp.data.color = lt.hex_color(FLAME)[:3]
            lamp.data.shadow_soft_size = 0.03
            lamp.location = Vector(p) + Vector((0.0, 0.0, 0.0))
            where.objects.link(lamp)
            objs.append(lamp)
    return objs


# --- The body and its garments (the Unpaid's frame: front -Y, origin on the ground, the pelvis at 1 m) ---

TORSO = uc.TORSO_D                          # the Unpaid's wasted torso
COAL_AT = (0.3, 1.32)                       # the coal: angle round the chest from the front, height (the rig's coal bone)
COAL_R = 0.056                              # the Unpaid's coal
SHOULDER = {s: np.array([s * 0.16, -0.022, 1.428]) for s in SIDES}
PIVOT = np.array([0.0, -0.06, 1.615])       # the top of the neck
UPPER, FORE = 0.316, 0.284                  # the Unpaid's arm bones


def torso_core(fig, profile=TORSO, coal_at=COAL_AT, radius=COAL_R, zone=SKIN, z_lo=0.92):
    """The body under the clothes, its chest sunk into a crater round the coal, the crater's rim scorched and glowing
    (hungry) or barely warm (banked)."""
    theta = np.linspace(0.0, 2.0 * np.pi, 96, endpoint=False)
    z = np.linspace(z_lo, 1.55, 40)
    piece = uc.closed_loft(surface(profile, z, theta))
    tc, zc = coal_at
    pc = surface(profile, [zc], np.array([tc]))[0, 0]
    nc = uc.torso_normal(tc)
    piece = uc.push(piece, pc, radius * 1.3, -0.02)
    dist = np.linalg.norm(piece[0] - pc, axis=1)
    cavity = smoothstep(radius * 1.1, radius * 0.6, dist) * 0.9
    heat = 0.3 if fig.banked else 1.0
    ember = (smoothstep(radius * 0.55, radius * 0.9, dist) * smoothstep(radius * 1.5, radius * 1.1, dist)
             * (0.4 + 0.6 * (0.5 + 0.5 * uc.pnoise(piece[0], 60.0, 5))) * heat)
    fig.add(ZPart('Body', zone).add(piece, cavity=cavity, ember=ember))
    fig.coal = (pc - nc * 0.012, radius)


def shell(profile, z_lo, z_hi, sd_fn, folds_fn, grow=0.0, rows=130, cols=220, hole=0.07, coal_at=COAL_AT):
    """A garment round the body: the profile grown by grow and folded by folds_fn(theta, z), cut where sd_fn > 0 and
    burnt through at the coal (a ragged hole). Returns (V, faces), each vertex's theta and z and its distance from the
    hole's edge."""
    theta = np.linspace(-np.pi, np.pi, cols, endpoint=False)
    z = np.linspace(z_lo, z_hi, rows)
    th, zz = np.meshgrid(theta, z)
    P = surface(profile, z, theta, grow) + outward(theta)[None, :, :] * folds_fn(th, zz)[..., None]
    sd = sd_fn(th, zz)
    edge = np.ones_like(th)
    if hole:
        tc, zc = coal_at
        ang = np.arctan2(zz - zc, (th - tc) * 0.17)
        rh = hole * (1.0 + 0.14 * np.sin(ang * 5 + 1.0) + 0.09 * np.sin(ang * 11.0))
        gap = np.hypot((th - tc) * 0.17, zz - zc)
        sd = np.maximum(sd, rh - gap)
        edge = gap - rh
    V, F, used = uc.cut(P, sd)
    return (V, F), th.ravel()[used], zz.ravel()[used], edge.ravel()[used]


def scorch(edge, V, heat=1.0):
    """The ember edge round the coal's burnt hole."""
    return np.clip(smoothstep(0.03, 0.0, edge) * (0.5 + 0.5 * uc.pnoise(V, 50.0, 2)) * heat, 0.0, 1.0)


def scorch_cavity(edge):
    return 0.55 * smoothstep(0.025, 0.0, edge)


def armhole(th, zz, z0=1.372, w=0.22, h=0.05):
    at = np.abs(th)
    return (1.0 - np.sqrt(((at - np.pi / 2 - 0.03) / w) ** 2 + ((zz - z0) / h) ** 2)) * 0.06


def shirt(fig, zone=CLOTH1, profile=TORSO, z_lo=0.92, neck=0.05, buttons_at=(1.405, 1.30, 1.2, 1.1, 1.0), placket=True,
          collar=True, heat=1.0, folds=1.0, name='Shirt', collar_y=-0.044, coal_at=COAL_AT, hole=0.066):
    """A work shirt hanging off him: loose folds, open at the throat, a placket and its buttons, a band collar."""
    def fold(th, zz):
        f = (0.005 * np.cos(7 * th + 0.5) + 0.003 * np.cos(12 * th + 2.0)) * smoothstep(1.3, 1.0, zz) * folds
        back = 0.003 * np.cos(9 * th + 1.0) * smoothstep(1.45, 1.25, zz) * smoothstep(1.4, 2.2, np.abs(th))
        return 0.005 + f + back + 0.008 * g(zz - 1.0, 0.05) * folds

    def sd(th, zz):
        at = np.abs(th)
        out = np.maximum(z_lo - zz, np.where(zz > 1.43, (neck + 1.25 * (zz - 1.43)) - at, -1.0))
        return np.maximum(np.maximum(out, armhole(th, zz)), zz - 1.548)
    (V, F), th, zz, edge = shell(profile, z_lo, 1.548, sd, fold, hole=hole, coal_at=coal_at)
    cav = np.maximum(0.3 * g(th, 0.011) * (zz < 1.432) * placket, scorch_cavity(edge))
    part = ZPart(name, zone).add((V, F), ember=scorch(edge, V, heat), cavity=cav)
    if placket:
        am.buttons(part, profile, [(0.0, z) for z in buttons_at], grow=0.012, size=0.0062)
    fig.add(part, solidify=0.005)
    if collar:
        fig.add(ZPart('Collar', zone).add(am.collar_band(profile, z0=1.515, height=0.028, gap=0.22,
                                                           r=(0.058, 0.056, 0.055), y0=collar_y)), solidify=0.004)
    return part


def kerchief(fig, zone=ACCENT, knot_y=-0.046):
    """A neckerchief loose round a thin neck: the rolled knot, the triangle over the chest, the knot behind (the
    Unpaid's bandana, tied by a miner)."""
    band = ZPart('Kerchief', zone)
    band.add(uc.torus((0.0, knot_y, 1.566), (0.0, -0.3, 1.0), 0.047, 0.011, squash=0.75))
    rows, cols = 14, 25
    zb = np.linspace(1.545, 1.44, rows)
    width = np.linspace(0.8, 0.03, rows)
    flap = np.array([surface(TORSO, [zr], np.linspace(-w, w, cols), grow=0.03)[0] for zr, w in zip(zb, width)])
    flap[..., 2] += 0.005 * np.sin(np.linspace(0, 3 * np.pi, cols))[None, :] * (1 - width[:, None])
    band.add((flap.reshape(-1, 3), [uc.grid_faces(rows, cols, closed=False)]))
    band.add(uc.ellipsoid((0.0, 0.0, 1.562), (0.018, 0.013, 0.015)))
    fig.add(band, solidify=0.005)


def braces(fig, zone, width=0.03, grow=0.016, profile=TORSO, front_t=0.6, back_t=2.62, z_top=1.535, z_low=1.0):
    """Braces over both shoulders, crossed at the back, buttoned at the waist."""
    part = ZPart('Braces', zone)
    for s in SIDES:
        keys = [(s * front_t, z_low), (s * (front_t - 0.03), 1.16), (s * (front_t - 0.08), 1.3), (s * 0.5, 1.42),
                (s * 0.8, 1.5), (s * 1.5708, z_top), (s * 2.3, 1.5), (s * 2.75, 1.41), (s * 2.95, 1.28),
                (s * 2.85, 1.15), (s * back_t, z_low)]
        am.ribbon_on(part, profile, keys, width, grow, n=70)
        for t in (s * front_t, s * back_t):
            p = surface(profile, [z_low], np.array([t]), grow + 0.006)[0, 0]
            n = uc.torso_normal(t)
            part.add(uc.ellipsoid(p, (0.007, 0.007, 0.0045), np.array([[0, 0, 1.0], np.cross(n, (0, 0, 1.0)), n]), 12, 8),
                     cavity=0.4)
    fig.add(part, solidify=0.004)
    return part


def belt(fig, zone, z=0.985, grow=0.022, profile=TORSO, width=0.042, buckle=True):
    part = ZPart('Belt', zone)
    am.ribbon_on(part, profile, [(t, z) for t in np.linspace(-np.pi, np.pi, 25)], width / 2.0, grow, n=90, curl=0.0)
    if buckle:
        b0 = surface(profile, [z], np.array([0.0]), grow + 0.004)[0, 0]
        part.add(uc.box_piece(b0, (0.026, 0.005, 0.022)), cavity=0.35)
    fig.add(part, solidify=0.004)
    return part


# --- Arms and hands ---

def elbow(S, W, pole, L1=UPPER, L2=FORE):
    """Where the elbow goes for a shoulder S and wrist W, bending toward pole (two-bone IK)."""
    S, W = np.asarray(S, float), np.asarray(W, float)
    d = W - S
    dist = float(np.linalg.norm(d))
    k = d / dist
    dist = min(dist, (L1 + L2) * 0.995)
    a = (L1 * L1 - L2 * L2 + dist * dist) / (2.0 * dist)
    h = math.sqrt(max(L1 * L1 - a * a, 0.0))
    p = unit(perp(pole, k))
    return S + k * a + p * h


def grip(S, G, h, side, elbow_guess, pole, reach=0.092, drop=0.032):
    """An arm whose fist closes round a handle running along h through G: the fingers continue the forearm's line
    (from elbow_guess) square to the handle, the back of the hand turned so the handle runs across the palm. Returns
    the elbow, the wrist and the hand's frame (along the fingers, the back of the hand)."""
    h = unit(h)
    f = unit(perp(np.asarray(G, float) - np.asarray(elbow_guess, float), h))
    u = unit(side * np.cross(h, f))
    W = np.asarray(G, float) - f * reach + u * drop
    return elbow(S, W, pole), W, f, u


def claw_hand(fig, name, E, W, fwd, up, side, curl, spread=0.1, thumb=0.35, claw=0.3, forearm=0.12, zone=SKIN):
    """The Unpaid's long bony hand on a thin forearm (from inside the cuff to the wrist), fused smooth."""
    dv = unit(W - E)
    start = W - dv * max(np.linalg.norm(W - E) - 0.02, forearm)
    limb = [uc.tube([start, start + (W - start) * 0.35, (start + W) * 0.5 + (W - start) * 0.2, W - dv * 0.01],
                    [0.033, 0.031, 0.026, 0.017], [0.036, 0.034, 0.03, 0.024], segs=20, up=up),
            uc.ellipsoid(W - dv * 0.012 + unit(np.cross(dv, up)) * side * -0.018, (0.008, 0.008, 0.008))]
    limb += uc.hand(W, fwd, up, side, curl=curl, spread=spread, thumb=thumb, length=1.26, thin=0.82, claw=claw,
                    knuckle=1.3, scale=1.02)
    fig.add(ZPart(name, zone).add(uc.union(limb, 0.0024, smooth=3)))


FIST = (1.15, 1.2, 1.2, 1.15)
LOOSE_L = (0.35, 0.5, 0.62, 0.75)
LOOSE_R = (0.2, 0.32, 0.42, 0.55)
# The Unpaid's idle arms (figure D): elbows bent, one hand ahead of the other, the fingers curled unevenly.
IDLE_ARM = {1.0: dict(E=np.array([0.205, -0.075, 1.12]), W=np.array([0.205, -0.2, 0.865]),
                      fwd=unit([0.05, -0.55, -1.0]), up=unit([1.0, -0.25, 0.1]), curl=LOOSE_L, spread=0.1, thumb=0.35),
            -1.0: dict(E=np.array([-0.2, -0.04, 1.115]), W=np.array([-0.225, -0.13, 0.84]),
                       fwd=unit([-0.06, -0.35, -1.0]), up=unit([-1.0, -0.15, 0.1]), curl=LOOSE_R, spread=0.14,
                       thumb=0.35)}


def sleeve(fig, s, E, W, kind, zone, seed, shoulder=None, size=1.0):
    """The sleeve on an arm: 'coat' (to the wrist, a cuff), 'smock' (wide, gathered at the wrist), 'rolled' (rolled
    above the elbow) or 'shirt' (to the wrist, buttoned cuff)."""
    S0 = (SHOULDER[s] if shoulder is None else np.asarray(shoulder, float)) + np.array([-s * 0.012, 0.0, 0.008])
    part = ZPart(f'Sleeve{s:+.0f}', zone)
    if kind == 'rolled':
        for piece in am.rolled_sleeve(S0, E, 0.06 * size, 0.053 * size, seed, roll=0.016 * size):
            part.add(piece)
    else:
        radii = {'coat': (0.066, 0.06, 0.052), 'smock': (0.07, 0.068, 0.058), 'shirt': (0.058, 0.052, 0.045)}[kind]
        radii = tuple(r * size for r in radii)
        for piece in am.full_sleeve(S0, E, W, radii, seed, cuff=0.013 if kind != 'smock' else 0.016,
                                    folds=0.1 if kind != 'smock' else 0.14):
            part.add(piece)
    fig.add(part)


# --- The head: the Unpaid's hungry face (figure D's), its thin hair under the hat ---

def hungry_head(fig, R, jaw, spec=None, teeth=True, locks=24, pivot=PIVOT, neck_radius=(0.037, 0.035, 0.034), seed=17):
    spec = spec or uc.HEAD_D
    face = ZPart('Head', SKIN)
    neck = [np.array([0.0, -0.03, 1.49]), np.array([0.0, -0.048, 1.56])]
    uc.head_parts(face, spec, R, pivot, jaw, neck=neck, neck_radius=neck_radius, teeth=teeth)
    center = pivot + R @ np.array([0.0, -0.012, 0.09])
    for s in SIDES:                                            # the cords standing in the thin neck
        face.add(uc.tube(spline([(s * 0.018, -0.07, 1.53), (s * 0.026, -0.072, 1.575),
                                 center + R @ np.array([s * 0.05, 0.006, -0.05])], 10), 0.0075, segs=10))
    face.add(uc.ellipsoid(pivot + np.array([0.0, -0.03, -0.03]), (0.007, 0.009, 0.012)))
    fig.add(face)
    hair = ZPart('Hair', SKIN)
    rng = np.random.default_rng(seed)
    for k in range(locks):
        a = math.pi * rng.uniform(0.4, 1.0) * (1 if k % 2 else -1)
        root = np.array([0.07 * math.sin(a) * 1.02, -0.09 * math.cos(a) * 1.08 + 0.006, rng.uniform(0.03, 0.06)])
        out = np.array([math.sin(a), -math.cos(a), 0.0])
        drop = rng.uniform(0.05, 0.15)
        flick = rng.uniform(-0.004, 0.016)
        side = np.array([math.cos(a), math.sin(a), 0.0]) * rng.uniform(-0.012, 0.012)
        path = [root, root + out * 0.01 + (0.0, 0.0, -drop * 0.35),
                root + out * 0.014 + side * 0.5 + (0.0, 0.006, -drop * 0.7),
                root + out * (0.014 + flick) + side + (0.0, 0.012, -drop)]
        path = [center + R @ p for p in path]
        w0 = rng.uniform(0.008, 0.02)
        uc.ribbon(hair, path, [(0.0, w0), (0.6, w0 * 0.7), (1.0, 0.002)], n=20, cols=4,
                  out=lambda C, c0=center: unit(C - c0), curl=0.35, twist=0.25 * rng.uniform(-1, 1), seed=60 + k)
    for values in hair.attr:
        values[:, 1] = 0.3
    if locks:
        fig.add(hair, solidify=0.003)
    fig.marks['head'] = center
    return center


def dust_mask(fig, R, jaw, zone=ACCENT, spec=None, pivot=PIVOT):
    """A bandana over nose and mouth against the dust: the cloth laid over the face from the bridge of the nose to
    under the chin, round the cheeks to a knot at the back of the head, its point hanging over the throat."""
    spec = spec or uc.HEAD_D
    rows = cols = 128
    (V, faces), _ = uc.head_shape(spec, jaw, rows=rows, cols=cols)
    P = V[:rows * cols].reshape(rows, cols, 3)
    d = unit(P)
    TH = np.arctan2(d[..., 0], -d[..., 1])
    z = P[..., 2]
    at = np.abs(TH)
    top = -0.006 - 0.02 * smoothstep(0.25, 1.1, at) - 0.03 * smoothstep(1.4, 2.2, at)   # over the nose, under the ears
    bottom = np.where(at < 1.6, -1.0, -0.072 - 0.006 * smoothstep(2.0, 3.0, at))
    sd = np.maximum(z - top, bottom - z)
    lift = 0.0055 + 0.003 * g(TH, 0.35) * smoothstep(-0.03, -0.06, z) + 0.004 * smoothstep(-0.11, -0.14, z) * g(TH, 0.9)
    Q = P + d * lift[..., None]
    Q[..., 1] -= 0.004 * g(TH, 0.6) * smoothstep(-0.04, -0.1, z)          # the cloth stands off the chin a little
    Vm, Fm, used = uc.cut(Q, sd)
    center = pivot + R @ np.array([0.0, -0.012, 0.09])
    part = ZPart('Mask', zone)
    folds = 0.3 * (np.sin(TH * 18.0 + z * 40.0) > 0.6).astype(float).ravel()[used] * (at.ravel()[used] < 1.4)
    part.add(uc.transform((Vm, Fm), R, center), cavity=folds * 0.4)
    # The point of the triangle hanging over the throat, and the knot's two tails at the back.
    tip = [np.array([0.0, -0.095, -0.12]), np.array([0.0, -0.088, -0.17]), np.array([0.0, -0.07, -0.215])]
    uc.ribbon(part, [center + R @ p for p in tip], [(0.0, 0.07), (0.5, 0.045), (1.0, 0.004)], n=16, cols=7,
              out=lambda C: np.tile(R @ np.array([0.0, -1.0, 0.1]), (len(C), 1)), curl=0.2)
    for s in SIDES:
        tail = [np.array([s * 0.01, 0.1, -0.055]), np.array([s * 0.03, 0.125, -0.09]), np.array([s * 0.035, 0.13, -0.13])]
        uc.ribbon(part, [center + R @ p for p in tail], [(0.0, 0.016), (1.0, 0.01)], n=10, cols=4,
                  out=lambda C: np.tile(R @ np.array([0.0, 1.0, 0.0]), (len(C), 1)), curl=0.2, twist=0.1 * s)
    part.add(uc.transform(uc.ellipsoid((0.0, 0.104, -0.05), (0.02, 0.014, 0.016)), R, center))
    fig.add(part, solidify=0.004)


# --- Hats (hat space: origin at the brim's middle, where the 'hat' bone's head is; front -Y). Each returns pieces as
# (V, faces, fade, zone, cavity) and the lamp's mount: a point on its front and the mount's pitch (degrees, forward).

def lathe(profile, origin=(0.0, 0.0, 0.0), axis=(0.0, 0.0, 1.0), segs=24, squash=None):
    """A closed surface of revolution: profile [(height along axis, radius)], radius 0 at either end a pole;
    squash scales the cross-section's second axis (an oval)."""
    o, k = np.asarray(origin, float), unit(axis)
    a = unit(np.cross(k, (0.31, 0.17, 0.93) if abs(k[2]) < 0.9 else (1.0, 0.0, 0.0)))
    b = np.cross(k, a)
    ang = np.linspace(0.0, 2.0 * np.pi, segs, endpoint=False)
    rad = np.outer(np.cos(ang), a) + np.outer(np.sin(ang), b) * (squash or 1.0)
    rings = [o + k * hgt + rad * r for hgt, r in profile if r > 1e-7]
    V = list(np.vstack(rings))
    R = len(rings)
    faces = [uc.grid_faces(R, segs)]
    first, last = profile[0], profile[-1]
    V.append(o + k * first[0] if first[1] <= 1e-7 else rings[0].mean(0))
    faces.append(uc.fan(len(V) - 1, np.arange(segs)))
    V.append(o + k * last[0] if last[1] <= 1e-7 else rings[-1].mean(0))
    faces.append(uc.fan(len(V) - 1, np.arange(segs) + (R - 1) * segs, top=True))
    return uc.orient((np.array(V), faces))


def crown_rings(zc, rc, theta, oval=1.1, dent=0.0):
    rings = []
    for z, r in zip(zc, rc):
        rings.append(np.stack([r * np.sin(theta), -r * np.cos(theta) * oval, np.full_like(theta, z)], -1))
    P = np.array(rings)
    if dent:
        P[..., 2] -= dent * np.exp(-(P[..., 0] / 0.025) ** 2) * (P[..., 2] > zc[-1] - 0.02)
    return P


def capped(P, top_z):
    nr, nc = P.shape[:2]
    V = np.vstack([P.reshape(-1, 3), [[0.0, 0.0, top_z]]])
    return V, [uc.grid_faces(nr, nc), uc.fan(nr * nc, np.arange(nc) + (nr - 1) * nc, top=True)]


def band_ring(theta, r, z0, z1, oval):
    band = np.array([np.stack([r * np.sin(theta), -r * np.cos(theta) * oval, np.full_like(theta, z)], -1)
                     for z in np.linspace(z0, z1, 3)])
    return band.reshape(-1, 3), [uc.grid_faces(3, len(theta))]


def hat_felt():
    """A miner's felt: a soft crown dented on top, the brim drooping round the sides and back and pinned up in front,
    where the lamp hooks over it."""
    theta = np.linspace(0.0, 2.0 * np.pi, 120, endpoint=False)
    q = np.linspace(0.0, 1.0, 12)[:, None]
    front = np.exp(-(wrap(theta) / 0.7) ** 2)
    r = 0.087 + 0.083 * q * (1.0 + 0.03 * np.sin(3 * theta + 0.6))
    pull = -0.05 * q ** 1.3 * front                          # the front flap folds up against the crown
    up = 0.085 * q ** 1.1 * front
    droop = -0.024 * q ** 1.6 * (1.0 - front) + 0.003 * q * np.sin(5 * theta + 1.0)
    rr = r + pull
    brim = np.stack([rr * np.sin(theta), -rr * np.cos(theta) * 1.06, up + droop], -1)
    zc = [0.0, 0.03, 0.06, 0.085, 0.1, 0.108]
    rc = [0.088, 0.087, 0.084, 0.077, 0.064, 0.042]
    P = crown_rings(zc, rc, theta, dent=0.016)
    crownV, crownF = capped(P, 0.086)
    out = [(brim.reshape(-1, 3), [uc.grid_faces(12, 120)], 1.0, CLOTH1, 0.0),
           (crownV, crownF, 1.0, CLOTH1, 0.12 * smoothstep(0.035, 0.05, crownV[:, 2]) * smoothstep(0.07, 0.055, crownV[:, 2])),
           band_ring(theta, 0.0895, 0.003, 0.03, 1.1) + (1.0, ACCENT, 0.0)]
    # The lamp hooks over the turned-up flap: its mount at the flap's face, two thirds of the way up.
    k = 8
    j = 0
    mount = brim[k, j] + np.array([0.0, -0.006, 0.0])
    return out, (mount, 8.0)


def hat_cap():
    """A leather miner's cap: a soft round crown over a stitched band, a short stiff peak, and a brass lamp bracket on
    the front of the crown."""
    theta = np.linspace(0.0, 2.0 * np.pi, 96, endpoint=False)
    zc = [0.0, 0.018, 0.04, 0.06, 0.074, 0.083, 0.088]
    rc = [0.089, 0.093, 0.094, 0.088, 0.074, 0.05, 0.02]
    P = crown_rings(zc, rc, theta, oval=1.12)
    P[..., 1] -= 0.006 * (P[..., 2] / 0.088)                  # pulled a little forward
    V, F = capped(P, 0.089)
    seams = 0.25 * np.clip(np.max([g(np.abs(wrap(np.arctan2(V[:, 0], -V[:, 1]) - a)), 0.02) for a in (0.0, 1.57, -1.57, 3.14)],
                                  axis=0), 0, 1) * (V[:, 2] > 0.03)
    out = [(V, F, 1.0, CLOTH1, seams)]
    out.append(band_ring(theta, 0.0905, 0.0, 0.02, 1.12) + (1.0, ACCENT, 0.15))
    a = np.linspace(-1.15, 1.15, 40)
    q = np.linspace(0.0, 1.0, 7)[:, None]
    r = 0.09 + 0.05 * q * np.cos(a * 0.9) ** 0.7
    peak = np.stack([r * np.sin(a), -r * np.cos(a) * 1.12, 0.002 - 0.02 * q ** 1.4 + 0.0 * a], -1)
    out.append((peak.reshape(-1, 3), [uc.grid_faces(7, 40, closed=False)], 1.0, ACCENT, 0.1))
    plate = uc.box_piece((0.0, -0.104, 0.042), (0.014, 0.003, 0.02))
    out.append(plate + (1.0, CLOTH2, 0.2))
    return out, (np.array([0.0, -0.108, 0.044]), 4.0)


def hat_hard():
    """A Cornish miner's hard felt: a tall crown stiffened with resin, a narrow brim rolled up all round, scuffed; the
    candle's clay sits on its front."""
    theta = np.linspace(0.0, 2.0 * np.pi, 96, endpoint=False)
    zc = [0.0, 0.03, 0.06, 0.09, 0.12, 0.145, 0.16, 0.168]          # tall and straight-sided: a pot, not a bowler
    rc = [0.09, 0.0925, 0.094, 0.0945, 0.093, 0.086, 0.07, 0.04]
    P = crown_rings(zc, rc, theta, oval=1.1)
    P[..., 2] += 0.004 * np.sin(theta * 3.0 + 1.0)[None, :] * (np.array(zc)[:, None] / 0.168)    # knocked out of true
    V, F = capped(P, 0.17)
    scuff = 0.18 * (0.5 + 0.5 * np.sin(V[:, 0] * 90.0) * np.sin(V[:, 2] * 140.0 + V[:, 1] * 60.0)) * (V[:, 2] > 0.05)
    out = [(V, F, 1.0, CLOTH1, scuff)]
    q = np.linspace(0.0, 1.0, 8)[:, None]
    r = 0.089 + 0.032 * q
    lift = 0.012 * q ** 2 + 0.002 * np.sin(4 * theta) * q
    brim = np.stack([r * np.sin(theta), -r * np.cos(theta) * 1.08, lift + 0.0 * theta], -1)
    out.append((brim.reshape(-1, 3), [uc.grid_faces(8, 96)], 1.0, CLOTH1, 0.0))
    out.append(band_ring(theta, 0.0925, 0.002, 0.018, 1.1) + (1.0, ACCENT, 0.0))
    return out, (np.array([0.0, -0.103, 0.055]), 0.0)


def hat_bowler():
    """A bowler: a hard round crown, a narrow brim curled up at the sides and dipping front and back, a grosgrain band;
    the lamp on a bracket at its front (the Foreman's, in ForemanConcepts.py)."""
    theta = np.linspace(0.0, 2.0 * np.pi, 112, endpoint=False)
    zc = [0.0, 0.03, 0.058, 0.082, 0.1, 0.111, 0.117]
    rc = [0.088, 0.091, 0.091, 0.085, 0.071, 0.05, 0.024]
    P = crown_rings(zc, rc, theta, oval=1.1)
    V, F = capped(P, 0.119)
    out = [(V, F, 1.0, CLOTH1, 0.0)]
    q = np.linspace(0.0, 1.0, 10)[:, None]
    r = 0.088 + 0.046 * q
    lift = 0.03 * q ** 2.2 * np.sin(theta) ** 2 - 0.005 * q ** 2 * np.cos(theta) ** 2
    brim = np.stack([r * np.sin(theta), -r * np.cos(theta) * 1.09, lift], -1)
    out.append((brim.reshape(-1, 3), [uc.grid_faces(10, 112)], 1.0, CLOTH1, 0.0))
    edge = brim[-1] + np.array([0.0, 0.0, 0.004])
    rim = np.array([brim[-1], edge, brim[-1] * np.array([0.97, 0.97, 1.0]) + np.array([0.0, 0.0, 0.006])])
    out.append((rim.reshape(-1, 3), [uc.grid_faces(3, 112)], 1.0, CLOTH1, 0.15))
    out.append(band_ring(theta, 0.0915, 0.002, 0.024, 1.1) + (1.0, ACCENT, 0.0))
    plate = uc.box_piece((0.0, -0.1, 0.045), (0.013, 0.003, 0.018))
    out.append(plate + (1.0, CLOTH2, 0.2))
    return out, (np.array([0.0, -0.104, 0.047]), 2.0)


HATS = {'felt': hat_felt, 'cap': hat_cap, 'hard': hat_hard, 'bowler': hat_bowler}
HAT_SEAT = {'felt': 0.148, 'cap': 0.14, 'hard': 0.146, 'bowler': 0.148}


def add_hat(fig, kind, R, pivot=PIVOT, tilt=(-4.0, 5.0), seat=None, hats=None):
    """The hat on the head (drawn with the figure's 'hat' look); returns the lamp mount as a matrix (lamp space:
    origin on the hat's front, -Y forward, Z up) in figure space."""
    hats = hats or HATS
    Rh = R @ rot_x(tilt[0]) @ rot_y(tilt[1])
    at = pivot + R @ np.array([0.0, 0.0, HAT_SEAT.get(kind, 0.148) if seat is None else seat])
    pieces, (mount, pitch) = hats[kind]()
    part = ZPart('Hat', CLOTH1)
    for V, F, fade, zone, cav in pieces:
        part.add((np.asarray(V) @ Rh.T + at, F), fade=fade, zone=zone, cavity=cav)
    fig.add(part, solidify=0.005, material='hat')
    M = frame(at, Rh[:, 0], Rh[:, 1], Rh[:, 2])
    fig.marks['hat'] = at
    return M @ translate(mount) @ turn4(rot_x(pitch))


# --- Lamps (lamp space: origin at the mount on the hat, -Y forward, Z up). Each returns its parts as (ZPart, material
# key) and its flame (the flame's middle and the glow's size). ---

def flame_piece(base, direction, length, width, segs=16):
    """A flame: a teardrop along direction from base, fattest a third of the way up, a long soft tip."""
    prof = [(0.0, 0.0), (0.06 * length, 0.5 * width), (0.2 * length, 0.92 * width), (0.36 * length, width),
            (0.56 * length, 0.78 * width), (0.78 * length, 0.42 * width), (0.92 * length, 0.15 * width), (length, 0.0)]
    return lathe(prof, base, direction, segs=segs)


def lamp_carbide(scale=1.0):
    """A brass carbide lamp: the carbide chamber below, the water tank above with its valve lever, a hook over the
    hat, and a polished reflector the size of a saucer round the burner; the jet of flame stands out of its middle."""
    s = scale
    brass, refl, dark = ZPart('LampBrass', CLOTH1), ZPart('LampReflector', SKIN), ZPart('LampDark', CLOTH2)
    brass.add(uc.box_piece((0.0, -0.005 * s, 0.005 * s), (0.008 * s, 0.005 * s, 0.022 * s)))      # the hook plate
    c = np.array([0.0, -0.03 * s, 0.0])
    brass.add(lathe([(-0.04 * s, 0.0), (-0.04 * s, 0.017 * s), (-0.036 * s, 0.02 * s), (-0.004 * s, 0.02 * s),
                     (0.0, 0.022 * s), (0.03 * s, 0.022 * s), (0.034 * s, 0.018 * s), (0.036 * s, 0.0)], c, segs=24))
    dark.add(uc.torus(c, (0.0, 0.0, 1.0), 0.022 * s, 0.0032 * s, segs=24, sides=8))                # the thread ring
    dark.add(uc.tube([c + (0.0, 0.0, 0.034 * s), c + (0.0, 0.0, 0.046 * s)], 0.0045 * s, segs=10, up=(0, 1, 0)))
    dark.add(uc.box_piece(c + (0.007 * s, 0.0, 0.048 * s), (0.012 * s, 0.0025 * s, 0.002 * s)))   # the valve lever
    # The reflector: a shallow dish facing forward, its middle bright with the flame (the ember channel).
    center = np.array([0.0, -0.06 * s, 0.012 * s])
    Rr, depth = 0.05 * s, 0.013 * s
    t = np.linspace(0.06, 1.0, 12)[:, None]
    a = np.linspace(0.0, 2.0 * np.pi, 40, endpoint=False)[None, :]
    P = center + np.stack([np.cos(a) * Rr * t, -depth * t ** 2 + 0.0 * a, np.sin(a) * Rr * t], -1)   # rim forward
    V = np.vstack([P.reshape(-1, 3), [center]])
    F = [uc.grid_faces(12, 40), uc.fan(len(V) - 1, np.arange(40))]
    glow = np.concatenate([(0.12 + 0.88 * (1.0 - t.ravel()) ** 2.2)[:, None].repeat(40, 1).ravel(), [1.0]])
    refl.add((V, F), ember=glow)
    brass.add(uc.torus(center + np.array([0.0, -depth, 0.0]), (0.0, 1.0, 0.0), Rr, 0.0028 * s, segs=48, sides=8))
    dark.add(uc.tube([c + (0.0, -0.016 * s, 0.012 * s), center + np.array([0.0, -0.006 * s, 0.0])], 0.0035 * s, segs=10))
    flame = ZPart('Flame', SKIN).add(flame_piece(center + np.array([0.0, -0.008 * s, 0.0]), unit((0.0, -0.45, 0.89)),
                                                0.042 * s, 0.0085 * s))
    mid = center + np.array([0.0, -0.016 * s, 0.018 * s])
    return [(brass, 'lamp'), (refl, 'lamp'), (dark, 'lamp'), (flame, 'flame')], (mid, 0.05 * s)


def lamp_candle(scale=1.0, lean=0.0):
    """A tallow candle stuck to the hat in a fist of clay, wax run down its side, the wick and a tall flame."""
    s = scale
    clay, tallow, wick = ZPart('Clay', CLOTH1), ZPart('Tallow', SKIN), ZPart('Wick', CLOTH2)
    V, F = uc.ellipsoid((0.0, -0.014 * s, 0.0), (0.028 * s, 0.021 * s, 0.023 * s), segs=24, rings=14)
    V = V + (V - np.array([0.0, -0.014 * s, 0.0])) * (0.25 * uc.pnoise(V, 60.0 / s, 7))[:, None]
    clay.add((V, F), cavity=0.15 + 0.2 * (0.5 + 0.5 * uc.pnoise(V, 120.0 / s, 3)))
    d = unit(rot_y(lean) @ np.array([0.0, -0.3, 0.95]))
    b = np.array([0.0, -0.02 * s, 0.01 * s])
    L = 0.088 * s
    tallow.add(uc.tube([b, b + d * L * 0.5, b + d * L], [0.0108 * s, 0.0105 * s, 0.01 * s], segs=16, up=(1, 0, 0), domes=2))
    for k, (t, side, ln) in enumerate(((0.92, 0.4, 0.03), (0.85, -1.2, 0.045), (0.7, 2.0, 0.025), (0.95, 2.9, 0.02))):
        n = unit(np.cross(d, (1.0, 0.0, 0.0)) * math.cos(side) + np.array([1.0, 0.0, 0.0]) * math.sin(side))
        p0 = b + d * L * t + n * 0.0095 * s
        tallow.add(uc.tube([p0, p0 - d * ln * 0.5 * s + n * 0.002 * s, p0 - d * ln * s + n * 0.001 * s],
                           [0.0034 * s, 0.003 * s, 0.0036 * s], segs=8, up=(1, 0, 0), domes=2))
    tip = b + d * L
    wick.add(uc.tube([tip, tip + (0.0, 0.0, 0.008 * s)], 0.0012 * s, segs=6, up=(1, 0, 0), domes=1))
    flame = ZPart('Flame', SKIN).add(flame_piece(tip + (0.0, 0.0, 0.004 * s), (0.0, 0.0, 1.0), 0.04 * s, 0.009 * s))
    return [(clay, 'lamp'), (tallow, 'lamp'), (wick, 'lamp'), (flame, 'flame')], (tip + np.array([0.0, 0.0, 0.02 * s]),
                                                                                 0.045 * s)


def lamp_teapot(scale=1.0):
    """An oil-wick cap lamp, the little 'teapot': a tin body hooked over the cap's bracket, a hinged lid, a long spout
    carrying the wick forward and up, and a big smoky flame at its end with soot rising off it."""
    s = scale
    tin, dark, brass = ZPart('Tin', SKIN), ZPart('Soot', CLOTH2), ZPart('Brass', ACCENT)
    dark.add(uc.tube(spline([(0.0, 0.006 * s, 0.03 * s), (0.0, -0.004 * s, 0.046 * s), (0.0, -0.02 * s, 0.036 * s),
                             (0.0, -0.024 * s, 0.012 * s)], 12), 0.0024 * s, segs=8, up=(1, 0, 0), domes=1))
    c = np.array([0.0, -0.036 * s, -0.004 * s])
    tin.add(lathe([(-0.016 * s, 0.0), (-0.016 * s, 0.019 * s), (-0.012 * s, 0.024 * s), (0.0, 0.026 * s),
                   (0.008 * s, 0.025 * s), (0.014 * s, 0.021 * s), (0.017 * s, 0.014 * s), (0.02 * s, 0.006 * s),
                   (0.022 * s, 0.0)], c, segs=24))
    brass.add(uc.ellipsoid(c + (0.0, 0.0, 0.023 * s), (0.005 * s, 0.005 * s, 0.004 * s), segs=10, rings=6))
    spout = spline([c + (0.0, -0.018 * s, 0.0), c + (0.0, -0.04 * s, 0.008 * s), c + (0.0, -0.058 * s, 0.024 * s),
                    c + (0.0, -0.066 * s, 0.036 * s)], 16)
    tin.add(uc.tube(spout, np.linspace(0.0085 * s, 0.0045 * s, 16), segs=12, up=(1, 0, 0), domes=1))
    tip = spout[-1]
    flame = ZPart('Flame', SKIN).add(flame_piece(tip + (0.0, -0.002 * s, 0.003 * s), unit((0.0, -0.15, 1.0)),
                                                0.056 * s, 0.0115 * s))
    # Soot curling up off the flame's tip, fading.
    smoke = [tip + (0.0, -0.01 * s, 0.07 * s), tip + (0.004 * s, -0.006 * s, 0.1 * s), tip + (-0.006 * s, 0.004 * s, 0.13 * s),
             tip + (0.002 * s, 0.014 * s, 0.17 * s)]
    uc.ribbon(dark, smoke, [(0.0, 0.004 * s), (0.5, 0.009 * s), (1.0, 0.014 * s)], n=16, cols=4,
              out=(1.0, 0.0, 0.0), curl=0.2, twist=0.4, fade_from=0.1)
    return [(tin, 'lamp'), (dark, 'lamp'), (brass, 'lamp'), (flame, 'flame')], (tip + np.array([0.0, -0.004 * s, 0.026 * s]),
                                                                            0.055 * s)


LAMPS = {'carbide': lamp_carbide, 'candle': lamp_candle, 'teapot': lamp_teapot}


def add_lamp(fig, kind, M, **kw):
    parts, (mid, size) = LAMPS[kind](**kw)
    for part, key in parts:
        part.V = [am.apply(M, V) for V in part.V]
        fig.add(part, material=key, solidify=0.002 if part.name == 'LampReflector' else 0.0)
    fig.flames.append((am.apply(M, mid[None])[0], size))


# --- Props: the pick, the lunch pail, spare candles ---

PICK_LENGTH = 0.86                         # from the butt to the eye


def pick_parts(head=True):
    """A miner's pick in pick space: the butt at the origin, the ash handle up +Z swelling into the iron eye; the head
    across it in the swing's plane, its point to -Y and a narrow chisel to +Y, worn bright at the ends."""
    wood, iron, steel = ZPart('PickHandle', CLOTH1), ZPart('PickIron', CLOTH2), ZPart('PickSteel', ACCENT)
    L = PICK_LENGTH
    wood.add(uc.tube([(0.0, 0.0, 0.0), (0.0, 0.0, 0.04), (0.0, 0.0, 0.35), (0.0, 0.0, 0.7), (0.0, 0.0, L + 0.03)],
                     [0.0205, 0.018, 0.0155, 0.0165, 0.0195], [0.0165, 0.0148, 0.0128, 0.0138, 0.016], segs=14,
                     up=(0, 1, 0), domes=3))
    if not head:
        return [wood]
    iron.add(uc.tube([(0.0, -0.034, L), (0.0, 0.034, L)], 0.026, 0.021, segs=8, up=(0, 0, 1), domes=1))
    point = spline([(0.0, -0.03, L), (0.0, -0.13, L - 0.006), (0.0, -0.24, L - 0.024), (0.0, -0.3, L - 0.045)], 18)
    iron.add(uc.tube(point, np.interp(np.linspace(0, 1, 18), [0, 0.5, 1], [0.019, 0.013, 0.006]),
                     np.interp(np.linspace(0, 1, 18), [0, 0.5, 1], [0.016, 0.011, 0.005]), segs=10, up=(0, 0, 1),
                     domes=3, pointed=0.6))
    chisel = spline([(0.0, 0.03, L), (0.0, 0.13, L - 0.004), (0.0, 0.22, L - 0.018), (0.0, 0.265, L - 0.03)], 18)
    iron.add(uc.tube(chisel, np.interp(np.linspace(0, 1, 18), [0, 0.6, 1], [0.019, 0.011, 0.004]),
                     np.interp(np.linspace(0, 1, 18), [0, 0.6, 1], [0.016, 0.014, 0.016]), segs=10, up=(0, 0, 1), domes=2))
    steel.add(uc.ellipsoid((0.0, -0.288, L - 0.04), (0.005, 0.016, 0.006), segs=10, rings=6))
    steel.add(uc.box_piece((0.0, 0.262, L - 0.029), (0.016, 0.006, 0.0035)))
    return [wood, iron, steel]


def add_pick(fig, G, h, grip_at, point_dir, head=True):
    """The pick with its handle along h through the grip G (grip_at meters from the butt), its point turned toward
    point_dir."""
    z = unit(h)
    y = -unit(perp(point_dir, z))
    M = frame(np.asarray(G, float) - z * grip_at, np.cross(y, z), y, z)
    for part in pick_parts(head):
        part.V = [am.apply(M, V) for V in part.V]
        fig.add(part, material='pick')
    fig.marks['pick_head'] = am.apply(M, np.array([[0.0, 0.0, PICK_LENGTH]]))[0]


def pail_parts():
    """A miner's lunch pail in pail space: the bail's top at the origin, the tin hanging below with its lid and the
    tea cup on top."""
    tin, dark = ZPart('Pail', CLOTH1), ZPart('PailDark', CLOTH2)
    tin.add(lathe([(-0.24, 0.0), (-0.24, 0.066), (-0.236, 0.07), (-0.13, 0.072), (-0.124, 0.076), (-0.118, 0.076),
                   (-0.112, 0.07), (-0.108, 0.048), (-0.088, 0.046), (-0.084, 0.0)], segs=28))
    dark.add(uc.torus((0.0, 0.0, -0.13), (0.0, 0.0, 1.0), 0.072, 0.003, segs=28, sides=6), cavity=0.3)
    arc = [(0.075 * math.cos(a), 0.0, -0.17 + 0.17 * math.sin(a)) for a in np.linspace(0.0, np.pi, 15)]
    dark.add(uc.tube(arc, 0.0025, segs=6, up=(0, 1, 0), domes=1))
    return [tin, dark]


def add_pail(fig, M):
    for part in pail_parts():
        part.V = [am.apply(M, V) for V in part.V]
        fig.add(part, material='pail')


def hanging_candles(fig, top, count=3, seed=2):
    """Spare candles tied by their wicks to a string from a button, hanging against the chest."""
    rng = np.random.default_rng(seed)
    string = ZPart('CandleString', CLOTH2)
    tallow = ZPart('Candles', SKIN)
    knot = np.asarray(top, float)
    string.add(uc.ellipsoid(knot, (0.006, 0.005, 0.006), segs=8, rings=5))
    for k in range(count):
        end = knot + np.array([(k - (count - 1) / 2) * 0.022 + rng.uniform(-0.004, 0.004), -0.012 - 0.006 * k,
                               -0.03 - 0.012 * k])
        string.add(uc.tube([knot, (knot + end) / 2 + (0.0, -0.004, 0.0), end], 0.0014, segs=5, up=(1, 0, 0), domes=1))
        tallow.add(uc.tube([end, end + (0.0, -0.004, -0.11)], [0.0095, 0.0105], segs=12, up=(1, 0, 0), domes=2))
    fig.add(string, material='lamp')
    fig.add(tallow, material='lamp')


# --- The garments of each option ---

COAT = [(0.82, 0.192, 0.14, 0.004, 2.3), (0.90, 0.186, 0.134, 0.004, 2.3), (0.98, 0.178, 0.126, 0.002, 2.3),
        (1.06, 0.17, 0.12, -0.002, 2.3), (1.16, 0.172, 0.12, -0.008, 2.4), (1.26, 0.18, 0.124, -0.014, 2.5),
        (1.34, 0.188, 0.124, -0.02, 2.7), (1.40, 0.194, 0.118, -0.028, 2.9), (1.45, 0.192, 0.11, -0.034, 3.2),
        (1.475, 0.18, 0.102, -0.038, 3.4), (1.50, 0.152, 0.093, -0.042, 3.0), (1.52, 0.114, 0.083, -0.045, 2.6),
        (1.545, 0.082, 0.074, -0.047, 2.2)]


def coat_open(zz):
    """The open coat's fronts (angle either side of the middle): wide enough at the chest to show the shirt and the
    coal, parting further toward the hem."""
    return 0.4 + 0.22 * smoothstep(1.3, 0.85, zz) - 0.1 * smoothstep(1.42, 1.53, zz)


def coat_closed(zz):
    return np.where(zz > 1.36, 0.06 + 1.5 * (zz - 1.36), 0.03 + 0.1 * smoothstep(0.98, 0.84, zz))


def work_coat(fig, closed=False, collar_up=False, zone=CLOTH2, hem=0.835, seed=21, profile=None, collar_y=-0.044,
              coal_at=COAL_AT, heat=1.0, hole=0.074):
    """A hip-length duck work coat hanging off him: boxy, stiff folds, patch pockets; open, or buttoned to the chest;
    its collar turned down or up round his neck."""
    COAT_P = profile or COAT
    rng = np.random.default_rng(seed)
    amps, phases = rng.uniform(0.5, 1.0, 4), rng.uniform(0.0, 2.0 * np.pi, 4)
    opening = coat_closed if closed else coat_open

    def folds(th, zz):
        f = sum(a * np.cos(k * th + p) for a, k, p in zip(amps, (5, 7, 11, 15), phases)) / amps.sum()
        sag = 0.006 * smoothstep(1.3, 0.9, zz) * (1.0 - smoothstep(0.0, 0.9, np.abs(th)))
        return 0.012 + 0.013 * smoothstep(1.15, 0.85, zz) * (f - 0.3 * np.abs(f)) + sag

    def sd(th, zz):
        at = np.abs(th)
        h = hem + 0.008 * np.sin(4 * th + 1.0) + 0.006 * np.sin(11 * th)
        out = np.maximum(h - zz, opening(zz) - at)
        out = np.maximum(out, armhole(th, zz, 1.368, 0.24, 0.055))
        return np.maximum(out, zz - 1.546)
    (V, F), th, zz, edge = shell(COAT_P, hem - 0.02, 1.546, sd, folds, hole=hole, coal_at=coal_at)
    part = ZPart('Coat', zone).add((V, F), ember=scorch(edge, V, heat), cavity=scorch_cavity(edge))
    for s in SIDES:
        t0, t1 = sorted((s * (coat_open(1.3) + 0.08), s * (coat_open(1.3) + 0.42)))
        if s < 0 or closed:
            am.patch(part, COAT_P, t0, t1, 1.24, 1.34, grow=0.016, cavity_edge=0.45)
        u0, u1 = sorted((s * 0.62, s * 1.12))
        am.patch(part, COAT_P, u0, u1, 0.88, 1.0, grow=0.018, cavity_edge=0.45)
    if closed:
        for zb in (1.34, 1.22, 1.1, 0.98):
            p = surface(COAT_P, [zb], np.array([0.035]), grow=0.022)[0, 0]
            part.add(uc.ellipsoid(p, (0.0075, 0.0075, 0.0045), np.array([[0, 0, 1.0], [1.0, 0, 0], [0, -1.0, 0]])),
                     cavity=0.35)
    fig.add(part, solidify=0.008)
    # The collar: turned down onto the shoulders, or turned up round the neck against the cold.
    theta = np.linspace(-np.pi, np.pi, 96, endpoint=False)
    if collar_up:
        rows = [(1.53, 0.092, 0.0), (1.56, 0.088, 0.0), (1.6, 0.086, 0.0), (1.625, 0.09, 0.0)]
    else:
        rows = [(1.552, 0.09, 0.0), (1.535, 0.105, 0.0), (1.505, 0.125, 0.012), (1.47, 0.142, 0.03)]
    rings = []
    for z0, r0, drop in rows:
        front = np.cos(theta).clip(0.0, None) ** 2
        rings.append(np.stack([r0 * np.sin(theta), collar_y - r0 * 0.96 * np.cos(theta), z0 - drop * 2.2 * front], -1))
    rings = np.array(rings)
    gap = 0.5 if collar_up else 0.42
    sdc = np.broadcast_to((gap - np.abs(theta))[None, :], rings.shape[:2]) * 0.1
    Vc, Fc, used = uc.cut(rings, sdc)
    fig.add(ZPart('CoatCollar', zone).add((Vc, Fc), cavity=0.12), solidify=0.008)


SMOCK = [(0.6, 0.218, 0.164, 0.012, 2.2), (0.72, 0.206, 0.152, 0.01, 2.2), (0.84, 0.19, 0.138, 0.006, 2.2),
         (0.94, 0.172, 0.122, 0.002, 2.2), (1.0, 0.162, 0.112, 0.0, 2.2), (1.06, 0.166, 0.116, -0.002, 2.3),
         (1.16, 0.172, 0.12, -0.008, 2.4), (1.26, 0.178, 0.122, -0.014, 2.5), (1.34, 0.186, 0.122, -0.02, 2.7),
         (1.40, 0.19, 0.116, -0.028, 2.9), (1.45, 0.188, 0.108, -0.034, 3.2), (1.475, 0.177, 0.1, -0.038, 3.4),
         (1.50, 0.15, 0.091, -0.042, 3.0), (1.52, 0.113, 0.081, -0.045, 2.6), (1.545, 0.08, 0.072, -0.047, 2.2)]


def smock(fig, zone=CLOTH2, rope=ACCENT, hem=0.62, seed=5):
    """The miner's smock ('slop'): a long loose canvas smock gathered at a yoke, belted with a rope, falling in deep
    folds to mid-thigh where its hem is ragged; a short slit at the neck."""
    rng = np.random.default_rng(seed)
    amps, phases = rng.uniform(0.5, 1.0, 5), rng.uniform(0.0, 2.0 * np.pi, 5)

    def folds(th, zz):
        f = sum(a * np.cos(k * th + p) for a, k, p in zip(amps, (7, 11, 13, 17, 23), phases)) / amps.sum()
        below = smoothstep(0.98, 0.7, zz)
        above = smoothstep(1.02, 1.3, zz) * smoothstep(1.42, 1.3, zz)
        blouse = 0.012 * g(zz - 1.05, 0.04)
        return 0.008 + 0.022 * below * (f - 0.3 * np.abs(f)) + 0.006 * above * f + blouse

    def sd(th, zz):
        at = np.abs(th)
        h = hem + 0.025 * np.sin(5 * th + 1.0) * np.sin(13 * th) + 0.012 * np.sin(9 * th + 2.0)
        out = h - zz
        out = np.maximum(out, np.where(zz > 1.44, 0.035 + 0.5 * (zz - 1.44) - at, -1.0))
        out = np.maximum(out, armhole(th, zz, 1.37, 0.24, 0.055))
        return np.maximum(out, zz - 1.546)
    (V, F), th, zz, edge = shell(SMOCK, hem - 0.04, 1.546, sd, folds, rows=150, hole=0.072)
    yoke = 0.32 * g(zz - 1.4, 0.004) * (np.abs(th) < 2.6)
    fade = 1.0 - 0.0 * zz
    part = ZPart('Smock', zone).add((V, F), ember=scorch(edge, V), cavity=np.maximum(scorch_cavity(edge), yoke), fade=fade)
    fig.add(part, solidify=0.007)
    # The rope belt round the waist, its knot on his left with the ends hanging.
    ring = surface(SMOCK, [1.0], np.linspace(-np.pi, np.pi, 64), grow=0.02)[0]
    ring = np.vstack([ring, ring[:1]])
    cord = ZPart('Rope', rope)
    cord.add(uc.tube(ring, 0.011, segs=10, up=(0, 0, 1), domes=0))
    k0 = surface(SMOCK, [1.0], np.array([0.75]), grow=0.03)[0, 0]
    cord.add(uc.ellipsoid(k0, (0.02, 0.016, 0.018), segs=12, rings=8))
    for dx, ln in ((0.008, 0.17), (-0.012, 0.13)):
        cord.add(am.tube([k0, k0 + (dx, -0.012, -ln * 0.5), k0 + (dx * 1.6, -0.006, -ln)], [0.01, 0.009, 0.008], segs=8,
                         domes=2))
    fig.add(cord)


def henley(fig, zone=CLOTH1, heat=1.0):
    """An undershirt (a henley) clinging to the ribs with sweat, the placket's three buttons open."""
    def fold(th, zz):
        ribs = 0.0015 * np.cos(zz * 160.0) * smoothstep(1.2, 1.3, zz) * smoothstep(1.42, 1.34, zz) * smoothstep(1.2, 0.3, np.abs(th))
        return 0.003 + 0.0035 * np.cos(9 * th + 0.4) * smoothstep(1.15, 0.95, zz) + 0.006 * g(zz - 1.0, 0.05) + ribs

    def sd(th, zz):
        at = np.abs(th)
        out = np.maximum(0.92 - zz, np.where(zz > 1.46, (0.04 + 0.6 * (zz - 1.46)) - at, -1.0))
        return np.maximum(np.maximum(out, armhole(th, zz)), zz - 1.548)
    (V, F), th, zz, edge = shell(TORSO, 0.92, 1.548, sd, fold, hole=0.066)
    part = ZPart('Henley', zone).add((V, F), ember=scorch(edge, V, heat),
                                      cavity=np.maximum(scorch_cavity(edge), 0.3 * g(th, 0.01) * (zz > 1.38)))
    am.buttons(part, TORSO, [(0.0, z) for z in (1.4, 1.435, 1.47)], grow=0.008, size=0.0052)
    fig.add(part, solidify=0.004)
    fig.add(ZPart('Neckband', zone).add(am.collar_band(TORSO, z0=1.52, height=0.018, gap=0.16, r=(0.055, 0.054, 0.053),
                                                         y0=-0.045)), solidify=0.004)


# --- The options' outfits: three a pack. Looks are zone colors (skin and shroud, cloth 1, cloth 2, accent). ---

LAMP_LOOK = {'carbide': (0xbdb8ac, 0xc8ad74, 0x6b6e70, 0x5a4638),     # the reflector's silver, brass, dark iron, leather
             'candle': (0xe9e0c6, 0x8f7c64, 0x2e2a27, 0xb9aa8c),      # tallow, clay, the wick and string, hemp
             'teapot': (0xb3b4ae, 0xb7b0a4, 0x3c3836, 0xc2a36b)}      # tin, the spout's tin, soot, brass
PICK_LOOK = (0xd8d4cc, 0xa89777, 0x8b9196, 0xc9cbc6)                 # -, ash, iron, bright steel
PAIL_LOOK = (0xb8b8b0, 0xaeb0aa, 0x5c5e5e, 0xc2a36b)

VARIANTS = {
    'A': [dict(name='tan duck, slate flannel, felt hat', body=(SKIN_TINT, 0x8f9cab, 0xa08d6c, 0x9c5a48),
               hat=('felt', (0x6a5a4a, 0x6a5a4a, 0x7d6c58, 0x3f3530)), closed=False, collar_up=False, hold='drag'),
          dict(name='faded denim, leather cap', body=(SKIN_TINT, 0xc9c0aa, 0x72849a, 0x3d3836),
               hat=('cap', (0x5a4638, 0x5a4638, 0xb39a6a, 0x3d332c)), closed=True, collar_up=False, hold='shoulder'),
          dict(name='oxblood oilcloth, grey flannel, collar up', body=(SKIN_TINT, 0x9a958c, 0x7a4f42, 0x6f6a55),
               hat=('felt', (0x7a766d, 0x7a766d, 0x7d6c58, 0x4a4540)), closed=False, collar_up=True, hold='drag')],
    'B': [dict(name='unbleached smock, grey flannel', body=(SKIN_TINT, 0xa7a9a3, 0xd2c8b0, 0x8d7656),
               hat=('hard', (0x4a4644, 0x4a4644, 0x3a3634, 0x2e2b29)), hold='shoulder', lean=0.0),
          dict(name='grey smock, red flannel', body=(SKIN_TINT, 0x8e6a5a, 0x9da3a1, 0x6b5a46),
               hat=('hard', (0x2f2c2c, 0x2f2c2c, 0x3a3634, 0x5a4a3c)), hold='drag', lean=12.0),
          dict(name='iron-stained smock, oat shirt', body=(SKIN_TINT, 0xc0b49a, 0xb08a5e, 0x4a4440),
               hat=('hard', (0x6b5a48, 0x6b5a48, 0x3a3634, 0x3a3230)), hold='shoulder', lean=-8.0)],
    'C': [dict(name='oat henley, leather braces, red bandana', body=(SKIN_TINT, 0xcfc6b0, 0x5c4a3c, 0x9a4e42),
               hat=('cap', (0x5a4638, 0x5a4638, 0xb39a6a, 0x3d332c)), hold='drag', pail='belt'),
          dict(name='blue-grey undershirt, blue bandana', body=(SKIN_TINT, 0x9aa4ad, 0x3f3a36, 0x5f6c80),
               hat=('cap', (0x3d3a37, 0x3d3a37, 0xb39a6a, 0x2e2b29)), hold='shoulder', pail='hand'),
          dict(name='rust union suit, cream bandana', body=(SKIN_TINT, 0xb59a7e, 0x6a5a3e, 0xd8d0bc),
               hat=('cap', (0x6e5a44, 0x6e5a44, 0xb39a6a, 0x4a3e34)), hold='drag', pail=None)],
}
LAMP_OF = {'A': 'carbide', 'B': 'candle', 'C': 'teapot'}
# The eventual game model's triangles (the Unpaid's measured 6.6k body with the vest swapped for each option's clothes,
# its 1.05k head kept; the hat with its lamp and the pick are their own static meshes).
BUDGET = {'A': dict(body=7.6, hat=1.1, pick=0.45), 'B': dict(body=8.0, hat=0.75, pick=0.45),
          'C': dict(body=6.9, hat=0.85, pick=0.45, pail=0.3)}


def estimate(budget):
    total = sum(budget.values())
    return total, ', '.join(f'{k} {v:.1f}k' for k, v in budget.items())


# --- Poses: the arms, the pick and the body's bend ---

def hold_pick(fig, hold, sleeve_kind, sleeve_zone, pose):
    """The arms by the pick's hold: 'drag' (carried low in the right fist by the neck, the head forward and the handle
    trailing back), 'shoulder' (on the right shoulder, the head behind him), or the swing's wind-up (both fists, the
    head high over his right shoulder). Returns the left hand's frame if it is free."""
    S = SHOULDER
    if pose == 'windup':
        B = np.array([-0.06, -0.24, 1.47])
        h = unit((-0.22, 0.42, 0.88))
        hands = {1.0: grip(S[1.0], B + h * 0.07, h, 1.0, (0.06, -0.3, 1.2), (0.2, -0.4, -1.0)),
                 -1.0: grip(S[-1.0], B + h * 0.36, h, -1.0, (-0.36, -0.04, 1.5), (-1.0, 0.1, -0.2))}
        for s, (E, W, f, u) in hands.items():
            sleeve(fig, s, E, W, sleeve_kind, sleeve_zone, 41 + (s > 0))
            claw_hand(fig, f'Hand{s:+.0f}', E, W, f, u, s, FIST, 0.04, 0.8, 0.15)
        add_pick(fig, B + h * 0.36, h, 0.36, (0.0, -1.0, 0.0))
        return None
    if hold == 'drag':
        G = np.array([-0.235, -0.15, 0.85])
        h = unit((0.02, -0.75, 0.66))
        E, W, f, u = grip(S[-1.0], G, h, -1.0, (-0.2, -0.04, 1.12), (-0.6, 0.6, 0.0))
        add_pick(fig, G, h, 0.7, (0.0, -0.6, -1.0))
    else:
        G = np.array([-0.135, -0.165, 1.26])
        h = unit(np.array([-0.17, 0.0, 1.505]) - G)
        E, W, f, u = grip(S[-1.0], G, h, -1.0, (-0.25, -0.08, 1.12), (-0.5, 0.2, -1.0))
        add_pick(fig, G, h, 0.13, (0.0, 0.35, -1.0))
    sleeve(fig, -1.0, E, W, sleeve_kind, sleeve_zone, 41)
    claw_hand(fig, 'Hand-1', E, W, f, u, -1.0, FIST, 0.04, 0.8, 0.15)
    return 'free'


def left_arm(fig, sleeve_kind, sleeve_zone, carry=None):
    """The left arm hanging as the Unpaid's, or its fist round the lunch pail's bail."""
    a = IDLE_ARM[1.0]
    if carry == 'pail':
        G = np.array([0.215, -0.14, 0.86])
        h = np.array([0.0, -1.0, 0.0])          # the bail through the fist, the thumb forward
        E, W, f, u = grip(SHOULDER[1.0], G, h, 1.0, (0.21, -0.07, 1.12), (1.0, 0.3, 0.0))
        sleeve(fig, 1.0, E, W, sleeve_kind, sleeve_zone, 42)
        claw_hand(fig, 'Hand+1', E, W, f, u, 1.0, FIST, 0.04, 0.8, 0.15)
        add_pail(fig, frame(G + np.array([0.0, 0.0, 0.004]), (1.0, 0.0, 0.0), (0.0, 1.0, 0.0), (0.0, 0.0, 1.0)))
        return
    sleeve(fig, 1.0, a['E'], a['W'], sleeve_kind, sleeve_zone, 42)
    claw_hand(fig, 'Hand+1', a['E'], a['W'], a['fwd'], a['up'], 1.0, a['curl'], a['spread'], a['thumb'])


SHROUD_SIZE = [(0.0, 0.135, 0.092), (0.12, 0.142, 0.104), (0.3, 0.118, 0.1), (0.5, 0.088, 0.08), (0.75, 0.056, 0.052),
               (1.0, 0.028, 0.028)]


def shroud_tail(fig, path=None, size=None, seed=4, strips=7, top=1.04):
    path = path or [(0.0, 0.005, top), (0.0, 0.04, 0.84), (0.0, 0.17, 0.62), (0.0, 0.42, 0.45), (0.0, 0.75, 0.33),
                    (0.0, 1.1, 0.27)]
    fig.add(uc.shroud(ZPart('Shroud', SKIN), path, size or SHROUD_SIZE, strips=strips, split=0.16, ends=(0.74, 1.0),
                      spread=0.1, seed=seed, fade_from=0.22, folds=0.34, narrow=0.55, wave=0.035), solidify=0.006)


def miner(key, v=0, pose='idle'):
    """A dead miner of option key in outfit v: 'idle' (the rig's rest: hanging with the pick) or 'windup'."""
    spec = VARIANTS[key][v]
    fig = Ghost(key, pose, v)
    windup = pose == 'windup'
    fig.looks = {'body': spec['body'], 'hat': spec['hat'][1], 'lamp': LAMP_LOOK[LAMP_OF[key]], 'pick': PICK_LOOK,
                 'pail': PAIL_LOOK}
    torso_core(fig)
    if key == 'A':
        shirt(fig, CLOTH1)
        braces(fig, ACCENT if v != 1 else CLOTH1, width=0.015)
        kerchief(fig, ACCENT)
        work_coat(fig, closed=spec['closed'], collar_up=spec['collar_up'], seed=21 + v)
        sleeves = ('coat', CLOTH2)
    elif key == 'B':
        shirt(fig, CLOTH1, neck=0.04, buttons_at=(1.42, 1.48), collar=False)
        kerchief(fig, ACCENT, knot_y=-0.05)
        smock(fig, seed=5 + v)
        sleeves = ('smock', CLOTH2)
        hanging_candles(fig, surface(SMOCK, [1.37], np.array([-0.42]), 0.03)[0, 0], seed=2 + v)
    else:
        henley(fig, CLOTH1)
        braces(fig, CLOTH2, width=0.019, grow=0.012, front_t=0.55)
        belt(fig, CLOTH2, z=0.985, grow=0.02)
        sleeves = ('rolled', CLOTH1)
        if spec.get('pail') == 'belt':
            add_pail(fig, frame(surface(TORSO, [0.985], np.array([-1.35]), 0.04)[0, 0] + np.array([0.0, 0.0, 0.02]),
                                (0.3, 0.95, 0.0), (-0.95, 0.3, 0.0), (0.0, 0.0, 1.0)))
    free = hold_pick(fig, spec['hold'], sleeves[0], sleeves[1], pose)
    if free:
        left_arm(fig, sleeves[0], sleeves[1], 'pail' if spec.get('pail') == 'hand' else None)
    if windup:
        R, jaw = rot_x(-6.0), math.radians(32.0 if key != 'C' else 8.0)
    else:
        R, jaw = rot_y(-5.0) @ rot_x(-14.0), math.radians(18.0 if key != 'C' else 5.0)
    hungry_head(fig, R, jaw, teeth=key != 'C', locks=16 if spec['hat'][0] == 'cap' else 24, seed=17 + v)
    if key == 'C':
        dust_mask(fig, R, jaw)
    M = add_hat(fig, spec['hat'][0], R)
    if key == 'B':
        add_lamp(fig, 'candle', M, lean=spec.get('lean', 0.0))
    else:
        add_lamp(fig, LAMP_OF[key], M)
    shroud_tail(fig, seed=4 + v)
    fig.add(ZPart('Coal', SKIN).add(uc.coal_piece(*fig.coal)), material='coal')
    hunch, lean = (4.0, -9.0) if windup else (9.0, 8.0)
    fig.warp(uc.lean(math.radians(hunch), pivot_z=1.33, width=0.08))
    fig.warp(uc.lean(math.radians(lean)))
    return fig


_cache = {}


def figure(key, v=0, pose='idle'):
    if (key, v, pose) not in _cache:
        t0 = time.time()
        _cache[(key, v, pose)] = miner(key, v, pose)
        log(f'{key}{v + 1} {pose}: {_cache[(key, v, pose)].triangles()} triangles (concept density), built in '
            f'{time.time() - t0:.1f} s')
    return _cache[(key, v, pose)]


# --- Stages ---

def mannequin(where, location, turn=0.0):
    """A plain 1.8 m figure for scale."""
    V, F = uc.mannequin()
    obj = uc.mesh_object('Figure', V, F, flat('Mannequin', 0x7b8590, 0.7), where)
    obj.location = location
    obj.rotation_euler = (0.0, 0.0, turn)
    return obj


def box_uv(V):
    return np.column_stack([V[:, 0] + 0.6 * V[:, 1], V[:, 2] + 0.4 * V[:, 1]])


def rock(where, center, radii, seed, mat, turn=0.0):
    V, F = uc.ellipsoid(center, radii, segs=40, rings=22)
    c = np.asarray(center, float)
    V = V + (V - c) * (0.22 * uc.pnoise(V, 1.6 / max(radii), seed) + 0.08 * uc.pnoise(V, 5.0 / max(radii), seed + 1))[:, None]
    V = (V - c) @ rotation((0, 0, 1), turn).T + c
    return uc.mesh_object('Rock', V, F, mat, where, uvs=box_uv(V) * 0.5)


def timber(where, a, b, size, mat, name='Timber'):
    """A squared timber from a to b (size: its width and depth)."""
    a, b = np.asarray(a, float), np.asarray(b, float)
    d = unit(b - a)
    x = unit(np.cross(d, (0.0, 0.0, 1.0) if abs(d[2]) < 0.9 else (1.0, 0.0, 0.0)))
    y = np.cross(d, x)
    V, F = uc.box_piece((a + b) / 2.0, (size[0] / 2.0, size[1] / 2.0, np.linalg.norm(b - a) / 2.0),
                        np.column_stack([x, y, d]))
    uv = np.column_stack([V @ d * 0.6, (V @ x + V @ y) * 0.6])
    obj = uc.mesh_object(name, V, F, mat, where, uvs=uv)
    obj.data.polygons.foreach_set('use_smooth', np.zeros(len(obj.data.polygons), dtype=bool))
    return obj


def portal_stage(where, seed=3):
    """The Providence's mouth in the golden late afternoon: dusty ground and spoil, granite boulders, the timbered
    portal set into the high ridge's cliff behind, its dark adit, a tramway's rails coming out of it."""
    rng = np.random.default_rng(seed)
    uc.ground(where, 0xd8c6a2, radius=300.0)
    granite = lt.material('RockGranite', name='MinerGranite', tint=0xcfc8bc)
    cliff = lt.material('RockCliff', name='MinerCliff', tint=0xc2b8a8)
    wood = lt.material('WoodPlanks', name='MinerTimber', tint=0xa69680)
    # The ridge's foot: a cliff face across the back, the portal cut into it.
    xs = np.linspace(-26.0, 26.0, 140)
    zs = np.linspace(-0.5, 16.0, 60)
    X, Z = np.meshgrid(xs, zs)
    Y = 9.0 + 0.9 * np.sin(X * 0.35) + 0.6 * np.sin(X * 1.3 + 1.0) + 0.25 * Z + 0.3 * np.sin(Z * 0.9 + X * 0.2)
    Y = Y + 1.6 * smoothstep(4.0, 9.0, np.abs(X))
    hole = (np.abs(X) < 1.25) & (Z < 2.55)
    Y = np.where(hole, Y + 3.0, Y)
    P = np.stack([X, Y, Z], -1)
    sd = np.where(hole, 1.0, -1.0)
    V, F, used = uc.cut(P, sd, closed=False)
    uc.mesh_object('Cliff', V, F, cliff, where, uvs=np.column_stack([V[:, 0], V[:, 2]]) * 0.35)
    dark = flat('AditDark', 0x0b0a09, 1.0)
    V, F = uc.box_piece((0.0, 11.0, 1.3), (1.25, 2.0, 1.3))
    uc.mesh_object('Adit', V, F, dark, where)
    for x in (-1.3, 1.3):
        timber(where, (x, 9.0, -0.1), (x, 9.0, 2.75), (0.22, 0.22), wood)
    timber(where, (-1.75, 8.95, 2.78), (1.75, 8.95, 2.78), (0.26, 0.26), wood)
    for k in range(6):
        timber(where, (-1.6, 9.1 + 0.02 * k, 2.95 + 0.0 * k), (1.6, 9.1, 2.95), (0.05, 0.2), wood, 'Lagging')
    # The tramway coming out of the mouth, and the spoil.
    steel = flat('Rail', 0x6e6a64, 0.5)
    for x in (-0.45, 0.45):
        timber(where, (x, 10.5, 0.06), (x * 1.0 + 0.6, -6.0, 0.06), (0.04, 0.06), steel, 'Rail')
    for y in np.arange(-6.0, 10.0, 0.7):
        x0 = 0.6 * (10.5 - y) / 16.5
        timber(where, (x0 - 0.75, y, 0.03), (x0 + 0.75, y, 0.03), (0.16, 0.08), wood, 'Sleeper')
    rock(where, (6.5, 6.0, -0.6), (4.5, 3.0, 2.2), 11, granite, 0.4)
    for k in range(14):
        x = rng.uniform(-9.0, 9.0)
        y = rng.uniform(-3.0, 8.0)
        if abs(x) < 2.6 and y < 7.0:
            x += 3.2 * np.sign(x or 1.0)
        r = rng.uniform(0.12, 0.55)
        rock(where, (x, y, r * 0.35), (r * 1.3, r, r * 0.8), 20 + k, granite, rng.uniform(0, 3))
    am.ridges(where)


def golden(where, sun_strength=4.4):
    am.golden_sky()
    sun = uc.sun(am.SUN, strength=sun_strength, color=SUN_COLOR, where=where)
    sun.data.angle = math.radians(2.5)
    return sun


def studio(where, floor=0xc4b59c):
    uc.ground(where, floor, plain=True)
    am.golden_sky(0.85)
    uc.sun((-0.42, -0.85, 0.36), strength=4.0, color=SUN_COLOR, where=where)


def cool_world(strength=1.0, colors=((0.0, 0x0b111c), (0.46, 0x1a273d), (0.52, 0x2e4466), (1.0, 0x1e2d48))):
    world = bpy.data.worlds.get('GhostLight') or bpy.data.worlds.new('GhostLight')
    world.use_nodes = True
    nodes, links = world.node_tree.nodes, world.node_tree.links
    nodes.clear()
    out = nodes.new('ShaderNodeOutputWorld')
    bg = nodes.new('ShaderNodeBackground')
    bg.inputs['Strength'].default_value = strength
    coords = nodes.new('ShaderNodeTexCoord')
    norm = nodes.new('ShaderNodeVectorMath')
    norm.operation = 'NORMALIZE'
    links.new(coords.outputs['Generated'], norm.inputs[0])
    split = nodes.new('ShaderNodeSeparateXYZ')
    links.new(norm.outputs['Vector'], split.inputs['Vector'])
    remap = nodes.new('ShaderNodeMapRange')
    remap.inputs['From Min'].default_value = -1.0
    links.new(split.outputs['Z'], remap.inputs['Value'])
    ramp = nodes.new('ShaderNodeValToRGB')
    links.new(remap.outputs['Result'], ramp.inputs['Fac'])
    els = ramp.color_ramp.elements
    els[0].position, els[0].color = colors[0][0], lt.hex_color(colors[0][1])
    els[1].position, els[1].color = colors[-1][0], lt.hex_color(colors[-1][1])
    for p, c in colors[1:-1]:
        els.new(p).color = lt.hex_color(c)
    links.new(ramp.outputs['Color'], bg.inputs['Color'])
    links.new(bg.outputs['Background'], out.inputs['Surface'])
    bpy.context.scene.world = world


def area_light(where, location, target, size, energy, color=(0.62, 0.76, 1.0), shape='SQUARE'):
    light = bpy.data.objects.new('Area', bpy.data.lights.new('Area', 'AREA'))
    light.data.energy = energy
    light.data.size = size
    light.data.shape = shape
    light.data.color = color
    light.location = location
    light.rotation_euler = (Vector(target) - Vector(location)).to_track_quat('-Z', 'Y').to_euler()
    where.objects.link(light)
    return light


def gallery_stage(where, length=20.0, width=4.4, height=3.0, seed=5, rubble=False):
    """A gallery of the Providence under cool ghost light: rough rock walls and roof, squared timber sets every 2.5 m
    with lagging over their caps, a rail line down the middle; the far end opening to cold light (the way out)."""
    rng = np.random.default_rng(seed)
    rockm = lt.material('RockCliff', name='GalleryRock', tint=0x9a958c)
    floor = lt.material('GroundStony', name='GalleryFloor', tint=0x8c867a)
    wood = lt.material('WoodPlanks', name='GalleryTimber', tint=0x8f806c)
    y0, y1 = -6.0, length
    ys = np.linspace(y0, y1, 160)
    # The floor, a little uneven.
    xs = np.linspace(-width / 2 - 0.3, width / 2 + 0.3, 40)
    X, Y = np.meshgrid(xs, ys)
    Z = 0.03 * np.sin(X * 3.0 + Y * 1.7) + 0.02 * np.sin(Y * 4.1)
    P = np.stack([X, Y, Z], -1)
    F = uc.grid_faces(len(ys), len(xs), closed=False)
    uc.mesh_object('Floor', P.reshape(-1, 3), [F[:, ::-1]], floor, where, uvs=P.reshape(-1, 3)[:, :2] * 0.5)
    # The walls and roof: one rough section of rock round the gallery (walls up to the roof's spring, then an arch).
    rx, spring = width / 2 + 0.25, height * 0.6
    sec = ([(-rx, z) for z in np.linspace(-0.1, spring, 10)[:-1]]
           + [(rx * math.cos(t), spring + (height + 0.55 - spring) * math.sin(t)) for t in np.linspace(np.pi, 0.0, 30)]
           + [(rx, z) for z in np.linspace(spring, -0.1, 10)[1:]])
    sec = np.array(sec)
    tang = np.gradient(sec, axis=0)
    nrm = unit(np.column_stack([tang[:, 1], -tang[:, 0]]))             # away from the gallery's middle
    nrm = nrm * np.sign(np.sum(nrm * (sec - (0.0, spring * 0.5)), axis=1))[:, None]
    u = np.cumsum(np.concatenate([[0.0], np.linalg.norm(np.diff(sec, axis=0), axis=1)]))
    U, Yg = np.meshgrid(u, ys)
    bump = (0.12 * np.sin(U * 2.1 + Yg * 0.9) + 0.08 * np.sin(U * 4.3 - Yg * 2.3) + 0.06 * np.sin(Yg * 5.0 + U * 1.3)
            + 0.05 * uc.pnoise(np.column_stack([U.ravel(), Yg.ravel(), np.zeros(U.size)]), 1.7, seed).reshape(U.shape))
    P = np.stack([sec[None, :, 0] + nrm[None, :, 0] * bump, Yg, sec[None, :, 1] + nrm[None, :, 1] * bump], -1)
    F = uc.grid_faces(len(ys), len(sec), closed=False)
    uc.mesh_object('Rock', P.reshape(-1, 3), [F], rockm, where, uvs=np.column_stack([U.ravel() * 0.5, Yg.ravel() * 0.5]))
    # Timber sets and lagging, the rails and sleepers.
    for y in np.arange(-4.0, length - 3.0, 2.5):
        for x in (-width / 2 + 0.25, width / 2 - 0.25):
            timber(where, (x, y, -0.05), (x + rng.uniform(-0.04, 0.04), y, height - 0.18), (0.2, 0.2), wood)
        timber(where, (-width / 2, y, height - 0.08), (width / 2, y, height - 0.08), (0.22, 0.22), wood)
        for k in range(-3, 4):
            timber(where, (k * 0.55, y - 1.25, height + 0.08), (k * 0.55, y + 1.25, height + 0.08), (0.18, 0.05), wood,
                   'Lagging')
    steel = flat('GalleryRail', 0x5e5a55, 0.5)
    for x in (-0.38, 0.38):
        timber(where, (x, y0, 0.07), (x, y1, 0.07), (0.035, 0.05), steel, 'Rail')
    for y in np.arange(y0, y1, 0.65):
        timber(where, (-0.62, y, 0.035), (0.62, y, 0.035), (0.14, 0.07), wood, 'Sleeper')
    if rubble:
        # The fall: the roof come down across the stope's far end in a slope of broken rock, lit cold from above.
        granite = lt.material('RockGranite', name='Rubble', tint=0x9a948a)
        rock(where, (0.0, length - 1.5, 0.6), (width * 0.62, 3.2, height * 0.95), 49, granite, 0.0)
        for k in range(46):
            r = rng.uniform(0.15, 0.6)
            rock(where, (rng.uniform(-width / 2, width / 2), length - 6.0 + rng.uniform(0.0, 4.0) + r, r * 0.4 + rng.uniform(0, 1.2)),
                 (r * 1.3, r, r * 0.8), 50 + k, granite, rng.uniform(0, 3))
        area_light(where, (0.0, length - 7.0, height - 0.3), (0.0, length - 3.0, 0.8), 3.0, 420.0)
    # The far end's cold light, and a faint cold wash down the gallery's length.
    area_light(where, (0.0, length - 1.0, height * 0.7), (0.0, 0.0, 1.0), 3.5, 1400.0)
    for y in (1.0, 6.5, 12.0):
        area_light(where, (0.0, y, height - 0.3), (0.0, y + 0.01, 0.0), 1.5, 45.0)


def ghost_light(where, fill=(1.5, -6.0, 2.8), fill_energy=420.0):
    cool_world()
    area_light(where, fill, (0.0, 4.0, 1.2), 4.0, fill_energy, color=(0.58, 0.72, 1.0))


# --- Rendering ---

def render(path, resolution, samples=64, exposure=0.0):
    if FAST:
        path = os.path.join(WORK, 'fast_' + os.path.basename(path))
        resolution, samples = (resolution[0] // 2, resolution[1] // 2), 12
    os.makedirs(os.path.dirname(path), exist_ok=True)
    uc.setup_render(resolution, samples)
    bpy.context.scene.view_settings.exposure = exposure
    uc.render(path, resolution, samples=samples)
    bpy.context.scene.view_settings.exposure = 0.0
    return path


def lens_shot(path, eye, target, lens, resolution, samples=64, exposure=0.0, focus=None, fstop=None):
    cam = uc.camera(Vector(eye), Vector(target), lens=lens)
    cam.data.clip_end = 5000.0
    if focus is not None:
        cam.data.dof.use_dof = True
        cam.data.dof.focus_distance = focus
        cam.data.dof.aperture_fstop = fstop
    render(path, resolution, samples, exposure)
    bpy.data.objects.remove(cam)
    return path


def bounds_of(fig, turn=0.0):
    lo, hi = fig.bounds(turn)
    return lo * fig.scale, hi * fig.scale


# --- Shots ---

def shot_front():
    """All nine outfits from the front, beside a 1.8 m post and an Unpaid, in the golden light from the front left."""
    uc.reset()
    where = uc.collection('Front')
    unpaid = uc.figure('D', 'idle')
    ulo, uhi = unpaid.bounds(0.0)
    figs = [figure(k, v) for k in OPTIONS for v in range(3)]
    items = [(-0.08, 0.08), (ulo[0], uhi[0])] + [(bounds_of(f)[0][0], bounds_of(f)[1][0]) for f in figs]
    xs, total = uc.lineup(items, gap=0.34)
    uc.post(where, (xs[0], 0.0, 0.0))
    uc.label('1.8 m', (xs[0] + 0.1, -0.05, 1.78), 0.075, align='LEFT')
    uc.instantiate(unpaid, where, location=(xs[1], 0.0, 0.0))
    uc.label('an Unpaid', (xs[1] + (ulo[0] + uhi[0]) / 2, -0.6, 0.05), 0.085)
    for x, fig, (lo, hi) in zip(xs[2:], figs, items[2:]):
        instantiate(fig, where, (x, 0.0, 0.0))
        uc.label(f'{fig.key}{fig.variant + 1}', (x + (lo + hi) / 2, -0.6, 0.05), 0.1)
    studio(where)
    width = total + 0.5
    uc.camera((0.0, -30.0, 1.1 + 30.0 * math.tan(math.radians(6.0))), (0.0, 0.0, 1.1), ortho=width)
    render(os.path.join(OUT, 'Miners_front.png'), (2400, int(2400 * 2.45 / width)), 96)


def shot_golden(key):
    """The three outfits of an option at the Providence's mouth, three-quarter view, a 1.8 m figure among them."""
    uc.reset()
    where = uc.collection('Golden')
    portal_stage(where)
    golden(where)
    place = ((-1.2, 1.5, -0.3), (0.1, 0.5, -0.22), (1.35, 2.1, -0.42))
    for v, (x, y, t) in enumerate(place):
        instantiate(figure(key, v), where, (x, y, 0.0), t)
    mannequin(where, (-2.25, 2.5, 0.0), -0.2)
    eye = Vector((1.55, -3.1, 1.42))
    lens_shot(os.path.join(OUT, f'Miners_{key}_golden.png'), eye, Vector((-0.2, 1.5, 1.12)), 30.0, (1600, 1100), 96,
              focus=(eye - Vector((0.1, 0.6, 1.2))).length, fstop=5.6)


def shot_ghost(key):
    """A pack in a gallery under cool ghost light: the nearest winding up, its lamp flared (the weak spot); one of the
    others Restless (the blue coal)."""
    uc.reset()
    where = uc.collection('Ghost')
    gallery_stage(where)
    ghost_light(where)
    # Turned well to his left, so the swing's plane (and the pick's head) shows side-on and the lamp three-quarter.
    instantiate(figure(key, 0, 'windup'), where, (-0.45, 1.35, 0.0), 1.05, flare=2.6, light=4.0)
    instantiate(figure(key, 1), where, (0.95, 4.4, 0.0), 0.2, rank=RESTLESS, light=2.0)
    instantiate(figure(key, 2), where, (-1.0, 7.2, 0.0), 0.15, light=2.0)
    eye = Vector((1.2, -3.3, 1.3))
    lens_shot(os.path.join(OUT, f'Miners_{key}_ghost.png'), eye, Vector((-0.1, 3.0, 1.35)), 30.0, (1600, 1100), 96,
              exposure=0.4)


def shot_debug(key, pose='idle', v=0):
    uc.reset()
    where = uc.collection('Debug')
    fig = figure(key, v, pose)
    instantiate(fig, where, flare=2.6 if pose == 'windup' else 1.0)
    studio(where, 0x9d9484)
    c = Vector((0.0, -0.1, 1.15))
    paths = []
    for name, d in (('front', (0.0, -1.0, 0.05)), ('three', (0.8, -1.0, 0.1)), ('side', (1.0, 0.0, 0.05)),
                    ('back', (0.3, 1.0, 0.1))):
        cam = uc.camera(c + Vector(d).normalized() * 3.8, c, lens=50.0)
        paths.append(os.path.join(WORK, f'debug_{key}{v + 1}_{pose}_{name}.png'))
        uc.render(paths[-1], (560, 900), samples=12)
        bpy.data.objects.remove(cam)
    uc.compose(paths, os.path.join(WORK, f'debug_{key}{v + 1}_{pose}.png'))
    # And the head and lamp close.
    head = Vector(fig.marks['head'])
    cam = uc.camera(head + Vector((0.45, -1.0, 0.1)).normalized() * 0.9, head + Vector((0.0, 0.0, 0.04)), lens=70.0)
    uc.render(os.path.join(WORK, f'debug_{key}{v + 1}_{pose}_head.png'), (700, 700), samples=16)
    bpy.data.objects.remove(cam)


# --- The line-up of every new character beside an Unpaid and Abel, at scale, and at 30 m ---

def cast():
    """Everyone for the line-up, in order: (label, builder(where, location, turn)) with each one's width. The
    Foreman's options and Hiram and Thaddeus come from their scripts."""
    import ForemanConcepts as fc
    import LiftAndCellarConcepts as lc
    import AbelConcepts as ab
    unpaid = uc.figure('D', 'idle')
    abel = ab.figure('A', 'fight')
    people = [('an Unpaid', unpaid, lambda w, l, t: uc.instantiate(unpaid, w, location=l, turn=t), 1.0),
              ('Abel (1.3x)', abel, lambda w, l, t: ab.instantiate(abel, w, location=l, turn=t, light=False), ab.SCALE)]
    for k in OPTIONS:
        f = figure(k, 0)
        people.append((f'Miner {k}', f, lambda w, l, t, f=f: instantiate(f, w, l, t), 1.0))
    for k in fc.OPTIONS:
        f = fc.figure(k)
        people.append((f'Foreman {k}', f, lambda w, l, t, f=f: instantiate(f, w, l, t), f.scale))
    for name in ('Hiram', 'Thaddeus'):
        f = lc.figure(name, 'stand')
        people.append((name, f, lambda w, l, t, f=f: instantiate(f, w, l, t), 1.0))
    return people, ab


def shot_lineup():
    uc.reset()
    where = uc.collection('Lineup')
    people, ab = cast()
    ab._props.clear()
    turn = math.radians(-24.0)
    items = [(-0.08, 0.08)]
    for _, f, _, s in people:
        lo, hi = f.bounds(turn)
        items.append((lo[0] * s, hi[0] * s))
    xs, total = uc.lineup(items, gap=0.32)
    uc.post(where, (xs[0], 0.0, 0.0))
    uc.label('1.8 m', (xs[0] + 0.1, -0.05, 1.78), 0.075, align='LEFT')
    for x, (name, f, make, s), (lo, hi) in zip(xs[1:], people, items[1:]):
        make(where, (x, 0.0, 0.0), turn)
        uc.label(name, (x + (lo + hi) / 2, -0.9, 0.05), 0.2)
    studio(where)
    width = total + 0.5
    uc.camera((0.0, -40.0, 1.5 + 40.0 * math.tan(math.radians(6.0))), (0.0, 0.0, 1.5), ortho=width)
    render(os.path.join(CONCEPTS, 'Lineup.png'), (3000, int(3000 * 3.75 / width)), 96)


FAR = 30.0          # meters
GAME_FOCAL_PX = 960.0      # 1080p at a 90 degree horizontal field of view: 960 px of focal length


def shot_far():
    """The line-up 30 m away, at the size a 1080p screen shows it (90 degree field of view), in the golden light on
    dusty ground: each figure is about 58 px tall. The sheet enlarges it without smoothing."""
    uc.reset()
    where = uc.collection('Far')
    people, ab = cast()
    ab._props.clear()
    uc.ground(where, 0xd8c6a2, radius=400.0)
    golden(where)
    am.ridges(where)
    spacing = 1.25
    n = len(people)
    for k, (name, f, make, s) in enumerate(people):
        make(where, ((k - (n - 1) / 2) * spacing, FAR, 0.0), math.radians(-20.0))
    width_px = 560
    lens = GAME_FOCAL_PX * 36.0 / width_px
    cam = uc.camera(Vector((0.0, 0.0, 1.6)), Vector((0.0, FAR, 1.45)), lens=lens)
    cam.data.sensor_fit = 'HORIZONTAL'
    cam.data.sensor_width = 36.0
    cam.data.clip_end = 5000.0
    path = os.path.join(CONCEPTS, 'Far30m.png')
    os.makedirs(CONCEPTS, exist_ok=True)
    uc.setup_render((width_px, 116), 64)
    # Rendered at the game's own pixel size: no supersampled detail the screen wouldn't show.
    bpy.context.scene.render.filter_size = 1.2
    uc.render(path, (width_px, 116), samples=64)
    bpy.context.scene.render.filter_size = 1.5
    bpy.data.objects.remove(cam)


# --- The look sheet (composed in Blender: the renders on emissive planes, the words in Chakra Petch) ---

FONT = os.path.join(lt.REPO, 'Content', 'UI', 'Fonts', 'ChakraPetch-Bold.ttf')
FONT_REGULAR = os.path.join(lt.REPO, 'Content', 'UI', 'Fonts', 'ChakraPetch-Regular.ttf')
INK, ORANGE, PAPER = 0xe8e2d6, 0xf2a64a, 0x16171a


def _plane(name, x, y, w, h, z, mat):
    mesh = bpy.data.meshes.new('_Sheet_' + name)
    mesh.from_pydata([(x, -y - h, z), (x + w, -y - h, z), (x + w, -y, z), (x, -y, z)], [], [(0, 1, 2, 3)])
    uv = mesh.uv_layers.new(name='UVMap')
    for loop, coord in zip(uv.data, ((0.0, 0.0), (1.0, 0.0), (1.0, 1.0), (0.0, 1.0))):
        loop.uv = coord
    mesh.materials.append(mat)
    return bpy.data.objects.new('_Sheet_' + name, mesh)


def _emission(name, color=None, image=None, interpolation='Cubic'):
    mat = bpy.data.materials.new('_Sheet_' + name)
    mat.use_nodes = True
    nodes, links = mat.node_tree.nodes, mat.node_tree.links
    nodes.clear()
    out = nodes.new('ShaderNodeOutputMaterial')
    emit = nodes.new('ShaderNodeEmission')
    if image is not None:
        tex = nodes.new('ShaderNodeTexImage')
        tex.image = image
        tex.interpolation = interpolation
        links.new(tex.outputs['Color'], emit.inputs['Color'])
    else:
        emit.inputs['Color'].default_value = lt.hex_color(color)
    links.new(emit.outputs['Emission'], out.inputs['Surface'])
    return mat


def _text(name, body, x, y, size, color, bold=True, z=0.5):
    curve = bpy.data.curves.new('_Sheet_' + name, 'FONT')
    curve.body = body
    try:
        curve.font = bpy.data.fonts.load(FONT if bold else FONT_REGULAR, check_existing=True)
    except Exception:
        pass
    curve.size = size
    curve.align_x = 'LEFT'
    curve.align_y = 'TOP_BASELINE'
    curve.materials.append(_emission(name, color))
    obj = bpy.data.objects.new('_Sheet_' + name, curve)
    obj.location = (x, -y, z)
    return obj


class Sheet:
    """A look sheet laid out top to bottom: headings, lines of text, full-width images and rows of images, each with
    its caption, 1 unit = 1 px."""

    def __init__(self, width=2000, pad=40, gap=18):
        self.width, self.pad, self.gap = width, pad, gap
        self.y = pad
        self.objs = []
        self.n = 0

    def _id(self):
        self.n += 1
        return str(self.n)

    def title(self, text, size=80):
        self.objs.append(_text(self._id(), text, self.pad, self.y + size * 0.86, size, ORANGE))
        self.y += size * 1.2

    def heading(self, text, size=62):
        self.y += 26
        self.objs.append(_plane(self._id(), self.pad, self.y, self.width - 2 * self.pad, 3, 0.2,
                                _emission('rule', 0x3a3b40)))
        self.y += 22
        self.objs.append(_text(self._id(), text, self.pad, self.y + size * 0.86, size, ORANGE))
        self.y += size * 1.25

    def lines(self, text, size=38, color=INK, bold=False, step=1.3):
        for line in text.split('\n'):
            self.objs.append(_text(self._id(), line, self.pad, self.y + size * 0.9, size, color, bold=bold))
            self.y += size * step
        self.y += 8

    def image(self, path, caption='', nearest=False, height=None):
        self.row([(path, None, caption)], nearest=nearest)

    def row(self, tiles, nearest=False):
        """Images side by side over the full width, each (path, label, caption), all scaled to one height."""
        imgs = [bpy.data.images.load(p, check_existing=True) for p, _, _ in tiles]
        inner = self.width - 2 * self.pad - self.gap * (len(tiles) - 1)
        ratios = [im.size[0] / im.size[1] for im in imgs]
        h = inner / sum(ratios)
        x = self.pad
        label_h = 64 if any(label for _, label, _ in tiles) else 0
        bottom = 0.0
        for (path, label, caption), im, r in zip(tiles, imgs, ratios):
            w = h * r
            if label:
                self.objs.append(_text(self._id(), label, x, self.y + 50, 50, ORANGE))
            self.objs.append(_plane(self._id(), x, self.y + label_h, w, h, 0.0,
                                    _emission('img', image=im, interpolation='Closest' if nearest else 'Cubic')))
            yy = self.y + label_h + h + 8
            for line in caption.split('\n') if caption else []:
                self.objs.append(_text(self._id(), line, x, yy + 36, 36, INK, bold=False))
                yy += 46
            bottom = max(bottom, yy)
            x += w + self.gap
        self.y = bottom + 30

    def render(self, out_jpg):
        height = int(self.y + self.pad)
        scene = bpy.data.scenes.new('_Sheet')
        for o in self.objs:
            scene.collection.objects.link(o)
        ground = _plane('ground', 0, 0, self.width, height, -0.5, _emission('paper', PAPER))
        scene.collection.objects.link(ground)
        cam = bpy.data.objects.new('_SheetCam', bpy.data.cameras.new('_SheetCam'))
        cam.data.type = 'ORTHO'
        cam.data.ortho_scale = self.width
        cam.data.sensor_fit = 'HORIZONTAL'
        cam.location = (self.width * 0.5, -height * 0.5, 100.0)
        cam.data.clip_start, cam.data.clip_end = 1.0, 500.0
        scene.collection.objects.link(cam)
        scene.camera = cam
        scene.render.engine = 'BLENDER_EEVEE_NEXT'
        scene.eevee.taa_render_samples = 16
        scene.render.resolution_x, scene.render.resolution_y = self.width, height
        scene.render.resolution_percentage = 100
        scene.view_settings.view_transform = 'Standard'
        scene.view_settings.look = 'None'
        scene.render.film_transparent = False
        world = bpy.data.worlds.new('_SheetWorld')
        world.use_nodes = True
        world.node_tree.nodes['Background'].inputs['Color'].default_value = lt.hex_color(PAPER)
        scene.world = world
        scene.render.image_settings.file_format = 'JPEG'
        scene.render.image_settings.quality = 88
        scene.render.image_settings.color_mode = 'RGB'
        os.makedirs(os.path.dirname(out_jpg), exist_ok=True)
        scene.render.filepath = out_jpg
        bpy.ops.render.render(write_still=True, scene=scene.name)
        log(f'look sheet {out_jpg} ({self.width} x {height})')


def enlarge(path, factor, out):
    """An image enlarged by a whole factor without smoothing (every pixel a block), as a PNG."""
    src = bpy.data.images.load(path, check_existing=False)
    w, h = src.size
    px = np.empty(w * h * 4, np.float32)
    src.pixels.foreach_get(px)
    big = np.repeat(np.repeat(px.reshape(h, w, 4), factor, axis=0), factor, axis=1)
    img = bpy.data.images.new('Enlarged', w * factor, h * factor, alpha=False)
    img.pixels.foreach_set(big.ravel())
    img.filepath_raw = out
    img.file_format = 'PNG'
    os.makedirs(os.path.dirname(out), exist_ok=True)
    img.save()
    bpy.data.images.remove(src)
    bpy.data.images.remove(img)
    return out


def sheet():
    import ForemanConcepts as fc
    import LiftAndCellarConcepts as lc
    s = Sheet()
    s.title('The Providence\'s dead: pick one of each')
    s.lines('Dead miners (A, B or C) and the Foreman (A, B or C), plus one look each for Hiram Coyle and\n'
            'Thaddeus Bright. All on the Unpaid\'s rig and proportions (the Foreman at 1.5x); concept renders, nothing\n'
            'in the game yet. Triangle counts are estimates for the game models.')
    s.image(os.path.join(CONCEPTS, 'Lineup.png'),
            'Everyone at scale beside an Unpaid, Abel (1.3x) and a 1.8 m post. Miners and Foremen in their first outfit.')
    far = enlarge(os.path.join(CONCEPTS, 'Far30m.png'), 3, os.path.join(WORK, 'Far30m_x3.png'))
    s.image(far, 'The same 30 m away, at the size a 1080p screen draws them (enlarged, pixels kept): head lamps, picks,\n'
                 'the bowlers, coats and satchels set them apart from the Unpaid and Abel. Left to right: Unpaid, Abel,\n'
                 'Miners A B C, Foremen A B C, Hiram, Thaddeus.', nearest=True)
    s.heading('Dead miners: pick A, B or C (three outfits each, so a pack never looks cloned)')
    s.image(os.path.join(OUT, 'Miners_front.png'), 'All nine outfits from the front, beside a 1.8 m post and an Unpaid.')
    captions = {'A': 'Duck coats and flannel, felt hats or leather caps; a carbide lamp with a saucer reflector.',
                'B': 'Old Cornish men: long smocks, rope belts, hard felt hats with a candle in clay.',
                'C': 'Stripped to undershirts, bandanas over the mouth, lunch pails; an oil-wick teapot lamp.'}
    for k in OPTIONS:
        total, parts = estimate(BUDGET[k])
        s.row([(os.path.join(OUT, f'Miners_{k}_golden.png'), f'{k}  {TITLES[k]}   ~{total:.1f}k tris',
                captions[k] + '\nAt the mine mouth, golden light; a 1.8 m figure at left.'),
               (os.path.join(OUT, f'Miners_{k}_ghost.png'), '',
                f'In a gallery, cool ghost light: the near one winds up,\nits lamp flared (the weak spot). ({parts})')])
    fc.sheet_section(s)
    lc.sheet_section(s)
    s.render(SHEET)


def main():
    keys = [a for a in ARGV if a in OPTIONS] or list(OPTIONS)
    if 'debug' in ARGV:
        poses = [p for p in ('idle', 'windup') if p in ARGV] or ['idle']
        vs = [int(a[1:]) - 1 for a in ARGV if a.startswith('v') and a[1:].isdigit()] or [0]
        for key in keys:
            for pose in poses:
                for v in vs:
                    shot_debug(key, pose, v)
        return
    if 'build' in ARGV:
        for key in keys:
            for v in range(3):
                figure(key, v)
            figure(key, 0, 'windup')
        return
    shots = [a for a in ARGV if a in ('front', 'golden', 'ghost', 'lineup', 'far', 'sheet')]
    if not shots:
        if '--preview' not in ARGV and not lt.want_preview():
            for key in keys:
                figure(key, 0)
            return
        shots = ['front', 'golden', 'ghost']
    if 'front' in shots:
        shot_front()
    for key in keys:
        if 'golden' in shots:
            shot_golden(key)
        if 'ghost' in shots:
            shot_ghost(key)
    if 'lineup' in shots:
        shot_lineup()
    if 'far' in shots:
        shot_far()
    if 'sheet' in shots:
        sheet()


if __name__ == '__main__':
    main()
