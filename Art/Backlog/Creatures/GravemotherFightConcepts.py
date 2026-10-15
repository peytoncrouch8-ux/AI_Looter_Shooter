"""Four concept sheets for the Gravemother's fight (2026-10-15): the user asked for a "sprite sheet" of the boss fight
(her, her projectiles, her ground charge, the attack particles) to give it the character of a real boss fight, 3 to 5
directions to choose from before any of it becomes game assets. Concept art kept in Art/Backlog (see its README): the
game's build never reads this file.

What the fight has to keep (Docs/Areas/RansomsRest.md, the Gravemother row): a Legendary spider at 1.8x on the game's
spider rig (Art/Models/Creatures/Spider.py, built here by that file through GravemotherConcepts.py's stage), x12
health, crits on the head and the abdomen (the existing hit hulls), a charge with a long telegraph and a ground crack,
4 spiderlings called at 66% and 33%, no climbing, no leaps, the den under Den Rock on the Sink's east wall. Everything
drawn is something the game can build at 1080p Medium without Lumen: meshes, emissive surfaces (Nanite can't draw
additive glow, so glows are emissive meshes plus soft sprite cards), decals, ribbons and simple particles. No
volumes, no smoke walls.

  A  Bone Mother   the user's Bone hide, unchanged, made a mother: four spiderlings ride her abdomen and drop off at
                   66% (four more climb on from the den before 33%). Milk Spit: a fan of three milky venom globs that
                   leave slowing, burning puddles. Grave Plough: a chalk crack runs from her feet to the player (the
                   2 s tell), then she ploughs along it. FX: globs, splash crowns, chalk crack, dust cards, bone chips.
  B  Vigil         the Bone hide lit from inside: the ghost-light of the dead she has eaten shows through her heart
                   mark and chevrons, and nine chapel candles stand in wax on her back (her phase meter: 9, 6, 3).
                   Corpse-Lights: wisps that drift from her flames and burst into rings of cold flame; the flames lean
                   toward the player first, like a keeper's lantern. Procession: a line of ghost-flames lights the
                   charge's path, and the crack she leaves glows.
  C  Shroud        the Ivory hide with its oxblood hourglass (her abdomen crit), wrapped in burial silk. Shroud Shot:
                   she turns her hourglass to the player and fires a silk wad that opens into a net and sticks to the
                   floor. Drag-Line: she anchors three lines to the den and slings herself forward (shoot the lines to
                   cut the charge short); trip-lines stay on the floor. Egg sacs in the den's web hatch the
                   spiderlings, so shooting them first means fewer.
  D  Barrow        the bold redesign: she has lived under the churchyard so long it grew onto her: a toppled
                   headstone bound to her abdomen by roots (armour: the crit is the soft rear under it), grave mud on
                   her legs, a mud-grey hide. Grave Clod: she tears up earth, bones and coffin wood and lobs it (shoot
                   it in her grip to stun her). Furrow: the earth heaves toward the player, then she ploughs it,
                   throwing up berms that stay as cover. At 33% the headstone cracks off and her pale Bone abdomen is
                   bare; spiderlings burst from mounds.

Each sheet has six renders, into Saved/ArtPreviews/RansomsRest/Concepts/GravemotherFight/GMF_<X>_<shot>.png:
  hero     she in the Sink at golden hour before the den, the fight's opening pose (1960 x 1000)
  spit     the projectile, over the player's shoulder: tell, flight and landing in one frame
  tell     the charge's telegraph, from the side
  charge   the charge itself and the crack or trail it leaves
  fx       the FX parts laid out on the floor like a sprite sheet
  special  the fight detail that gives her character (the brood, the candles, the sacs, the headstone)
and GMF_<X>.json: the sheet's text (below, SHEETS) and the callout points projected into each render. The sheets,
Saved/Concepts/Gravemother/Gravemother_Fight_<X>_<Name>.png, and the overview are assembled from these afterwards with
System.Drawing (outside Blender). The emission masks for B go to Intermediate/GravemotherFightConcepts/.

Hides: Bone is the game's T_SpiderBody_Pale_BC; Ivory and Ash are GravemotherConcepts.py's C and B maps in
Intermediate/GravemotherConcepts (painted again there if missing, never into Art/Textures). The figure is that file's
1.8 m clay stand-in for the player.

    powershell -NoProfile -File Tools\\artrun.ps1 -Script Art\\Backlog\\Creatures\\GravemotherFightConcepts.py
    ... -ScriptArgs A,B,hero,spit       some sheets and shots (none named: all of them)
    ... -ScriptArgs A,hero,quick        half size, few samples
    ... -ScriptArgs poses               the pose test strip (GMF_poses.png)
"""
import json
import math
import os
import random
import sys
import time

import bmesh
import bpy
import numpy as np
from bpy_extras.object_utils import world_to_camera_view
from mathutils import Euler, Matrix, Quaternion, Vector, noise

import looter_creatures as lc
import looter_textures as lt

REPO = lt.REPO
STAGE_PY = os.path.join(REPO, 'Art', 'Backlog', 'Creatures', 'GravemotherConcepts.py')
OLD_MAPS = os.path.join(REPO, 'Intermediate', 'GravemotherConcepts')
MAP_DIR = os.path.join(REPO, 'Intermediate', 'GravemotherFightConcepts')
OUT = os.path.join(REPO, 'Saved', 'ArtPreviews', 'RansomsRest', 'Concepts', 'GravemotherFight')
ARGV = [a for arg in (sys.argv[sys.argv.index('--') + 1:] if '--' in sys.argv else []) for a in arg.split(',') if a]
QUICK = 'quick' in ARGV
SHEET_KEYS = ('A', 'B', 'C', 'D')
SHOT_KEYS = ('hero', 'spit', 'tell', 'charge', 'fx', 'special')
SCALE, LING = 1.8, 0.45
HERO_SIZE, PANEL_SIZE = (1960, 1000), (970, 560)


def log(*a):
    print('GMFIGHT:', *a, flush=True)


# --- The stage and the spider, borrowed: GravemotherConcepts.py without its main() ---

def borrow_stage():
    source = open(STAGE_PY, encoding='utf-8').read()
    head, mark, tail = source.rpartition('\nmain()')
    if not mark or tail.strip():
        raise RuntimeError(f'{STAGE_PY} no longer ends with main(): update borrow_stage')
    ns = {'__name__': 'gravemother_stage', '__file__': STAGE_PY}
    exec(compile(head, STAGE_PY, 'exec'), ns)
    return ns


G = borrow_stage()
SCENE = bpy.context.scene
SPIDER = G['build_spider']()          # Spider.py's namespace; it paints nothing, and its objects are hidden
SRC = SPIDER['spider']
ATLAS = SPIDER['ATLAS']
RIDE, FEMUR, TIBIA = SPIDER['RIDE'], SPIDER['FEMUR'], SPIDER['TIBIA']
BONES = {name: (Vector(head), Vector(tail)) for name, head, tail, parent in SPIDER['bones']}
TOB = lc.TO_BLENDER
TOB_INV = TOB.inverted()
LEGS = [(pair, s, side) for pair in range(4) for side, s in ((1.0, 'l'), (-1.0, 'r'))]

# Creature-space landmarks (cm at 1x, x forward, y left, z up from the ground).
HEAD_C = SPIDER['lifted'](SPIDER['HEAD']['center'])
ABD = SPIDER['ABDOMEN']
ABD_C = SPIDER['lifted'](ABD['center'])
ABD_FWD = Vector(ABD['fwd']).normalized()
SPINNERETS = ABD_C - ABD_FWD * ABD['radii'][0]
MOUTH = Vector((66.0, 0.0, 48.0))


def vertex_groups():
    """Per vertex of the spider's mesh: its bone (each part moves rigidly with one), and per polygon the same."""
    me = SRC.data
    names = [g.name for g in SRC.vertex_groups]
    groups = np.array([v.groups[0].group if len(v.groups) else -1 for v in me.vertices], np.int64)
    co = np.empty(len(me.vertices) * 3, np.float32)
    me.vertices.foreach_get('co', co)
    starts = np.empty(len(me.polygons), np.int64)
    me.polygons.foreach_get('loop_start', starts)
    loops = np.empty(len(me.loops), np.int64)
    me.loops.foreach_get('vertex_index', loops)
    return names, groups, co.reshape(-1, 3), groups[loops[starts]]


GROUP_NAMES, VGROUP, REST_CO, POLY_GROUP = vertex_groups()


# --- Posing: rigid parts moved by bone matrices solved in creature space ---

def T(v):
    return Matrix.Translation(Vector(v))


def about(pivot, rot):
    return T(pivot) @ rot @ T(-Vector(pivot))


def pitch_up(deg):
    """Nose up by deg (the front, +x, toward +z)."""
    return Matrix.Rotation(math.radians(-deg), 4, 'Y')


def roll(deg):
    return Matrix.Rotation(math.radians(deg), 4, 'X')


def yaw(deg):
    return Matrix.Rotation(math.radians(deg), 4, 'Z')


def frame(x, n):
    x = x.normalized()
    n = (n - x * n.dot(x)).normalized()
    return Matrix((x, n.cross(x), n)).transposed()


class Pose:
    """A pose in creature space. Angles in degrees, lengths in cm. steps: leg -> (dx, dy out, dz) added to the rest
    foot; reach: leg -> (dx, dy out, dz) from the posed hip (a raised leg); spread scales the rest feet round the body;
    ground is the feet's height."""

    def __init__(self, pitch=0.0, rollz=0.0, turn=0.0, lift=0.0, shift=0.0, head=0.0, head_turn=0.0, abd=0.0,
                 abd_turn=0.0, fangs=0.0, palps=0.0, steps=None, reach=None, spread=1.0, ground=0.0):
        self.__dict__.update(locals())
        del self.__dict__['self']
        self.steps, self.reach = steps or {}, reach or {}


def pose_matrices(p):
    """{bone: creature-space matrix} and the solved feet."""
    B = T((p.shift, 0.0, p.lift)) @ about((0.0, 0.0, RIDE), yaw(p.turn) @ pitch_up(p.pitch) @ roll(p.rollz))
    M = {'body': B}
    M['head'] = B @ about(BONES['head'][0], yaw(p.head_turn) @ pitch_up(p.head))
    M['abdomen'] = B @ about(BONES['abdomen'][0], yaw(p.abd_turn) @ pitch_up(-p.abd))
    for s, side in (('l', 1.0), ('r', -1.0)):
        base = BONES[f'fang_{s}'][0]
        M[f'fang_{s}'] = M['head'] @ about(base, roll(side * p.fangs) @ pitch_up(p.fangs * 0.6))
        start = BONES[f'palp_{s}'][0]
        M[f'palp_{s}'] = M['head'] @ about(start, yaw(side * p.palps * 0.3) @ pitch_up(p.palps))
    feet = {}
    for pair, s, side in LEGS:
        leg = f'{pair}_{s}'
        H0, K0 = BONES[f'femur_{leg}']
        F0 = BONES[f'tibia_{leg}'][1]
        H = B @ H0
        if leg in p.reach:
            dx, dy, dz = p.reach[leg]
            target = H + Vector((dx, side * dy, dz))
        else:
            dx, dy, dz = p.steps.get(leg, (0.0, 0.0, 0.0))
            target = Vector((F0.x * p.spread + dx, F0.y * p.spread + side * dy, p.ground + dz))
        pole = B.to_3x3() @ SPIDER['knee_pole'](H0)
        K, F = lc.solve_two_bone(H, target, FEMUR[pair], TIBIA[pair], pole)
        n0, n1 = (K0 - H0).cross(F0 - K0), (K - H).cross(F - K)
        Rf = frame(K - H, n1) @ frame(K0 - H0, n0).transposed()
        Rt = frame(F - K, n1) @ frame(F0 - K0, n0).transposed()
        M[f'femur_{leg}'] = T(H) @ Rf.to_4x4() @ T(-H0)
        M[f'tibia_{leg}'] = T(K) @ Rt.to_4x4() @ T(-K0)
        feet[leg] = F
    return M, feet


def posed_mesh(name, M):
    me = SRC.data.copy()
    me.name = name
    co = REST_CO.copy()
    for gi, gname in enumerate(GROUP_NAMES):
        if gname not in M:
            continue
        mb = TOB @ M[gname] @ TOB_INV
        sel = VGROUP == gi
        R = np.array(mb.to_3x3())
        co[sel] = co[sel] @ R.T + np.array(mb.translation)
    me.vertices.foreach_set('co', co.astype(np.float32).ravel())
    me.update()
    return me


STAND = Pose()
THREAT = Pose(pitch=16, lift=16, shift=-12, abd=18, head=8, fangs=24, palps=10,
              reach={'0_l': (100, 90, 115), '0_r': (100, 90, 115), '1_l': (115, 95, 45), '1_r': (115, 95, 45)},
              steps={'2_l': (-10, 10, 0), '2_r': (-10, 10, 0), '3_l': (-15, 0, 0), '3_r': (-15, 0, 0)})
SHRIEK = Pose(pitch=28, lift=28, shift=-18, abd=30, head=12, fangs=30, palps=14,
              reach={'0_l': (55, 85, 175), '0_r': (55, 85, 175), '1_l': (95, 115, 115), '1_r': (95, 115, 115)},
              steps={'2_l': (-15, 15, 0), '2_r': (-15, 15, 0), '3_l': (-25, 0, 0), '3_r': (-25, 0, 0)})
CHARGE = Pose(pitch=-6, lift=-14, abd=8, head=-6, fangs=16, palps=-15,
              steps={'0_l': (60, 5, 0), '0_r': (-10, 0, 30), '1_l': (-35, 0, 0), '1_r': (45, 0, 25),
                     '2_l': (35, 0, 22), '2_r': (-30, 0, 0), '3_l': (-50, 0, 0), '3_r': (25, 0, 30)})
RAKE = Pose(pitch=8, lift=-8, shift=-16, abd=14, head=-8, fangs=12, reach={'0_l': (95, 45, 30)},
            steps={'0_r': (35, 0, 0), '1_l': (10, 0, 0), '1_r': (10, 0, 0)})
SPIT = Pose(pitch=12, lift=8, head=16, fangs=32, palps=12, abd=6, steps={'0_l': (15, 15, 0), '0_r': (15, 15, 0)})
CLOD = Pose(pitch=22, lift=22, shift=-14, abd=22, head=10, fangs=20, palps=6,
            reach={'0_l': (120, -2, 140), '0_r': (120, -2, 140)},
            steps={'1_l': (25, 10, 0), '1_r': (25, 10, 0), '3_l': (-15, 0, 0), '3_r': (-15, 0, 0)})
TURNED = Pose(pitch=-8, lift=-4, abd=30, head=-10, fangs=6, steps={'3_l': (-10, 20, 0), '3_r': (-10, 20, 0)})
DIG = Pose(pitch=-5, lift=-14, shift=-10, abd=20, head=-12, fangs=10,
           steps={'0_l': (45, -15, -28), '0_r': (45, -15, -28), '1_l': (20, 0, 0), '1_r': (20, 0, 0)})
HEAVY = Pose(pitch=4, lift=-10, abd=4, head=2, fangs=14, palps=10, reach={'0_l': (110, 40, 70)},
             steps={'0_r': (20, 0, 0)})
VIGIL = Pose(pitch=4, lift=6, head=8, abd=20, fangs=14, palps=6, reach={'0_l': (95, 85, 100), '0_r': (100, 60, 55)})
SHROUDED = Pose(pitch=14, lift=12, shift=-10, abd=26, abd_turn=-14, head=6, fangs=22, palps=8,
                reach={'0_l': (100, 90, 110), '0_r': (105, 80, 70)},
                steps={'2_l': (-10, 10, 0), '2_r': (-10, 10, 0), '3_l': (-15, 0, 0), '3_r': (-15, 0, 0)})
SLING = Pose(pitch=4, lift=-16, shift=-22, abd=10, head=-4, fangs=18, spread=1.08,
             steps={'0_l': (40, 10, 0), '0_r': (40, 10, 0), '3_l': (-30, 0, 0), '3_r': (-30, 0, 0)})
CLING = Pose(lift=-20, spread=0.6, ground=-12, palps=-10)
DROP = Pose(lift=8, spread=1.15, ground=-40, fangs=10, palps=5)
RUN = CHARGE


# --- Objects of one shot (removed after its render) ---

SHOT = []
CARDS = []


def link(o, shadow=True):
    SCENE.collection.objects.link(o)
    o.visible_shadow = shadow
    SHOT.append(o)
    return o


def clear_shot():
    for o in SHOT:
        data = o.data
        bpy.data.objects.remove(o, do_unlink=True)
        if isinstance(data, bpy.types.Mesh) and data.users == 0:
            bpy.data.meshes.remove(data)
    SHOT.clear()
    CARDS.clear()


def finish_bm(name, bm, mat, smooth=True, shadow=True):
    me = bpy.data.meshes.new(name)
    bm.to_mesh(me)
    bm.free()
    if not me.uv_layers:
        me.uv_layers.new(name='UVMap')
    G['white_col'](me)
    if mat is not None:
        me.materials.append(mat)
    me.polygons.foreach_set('use_smooth', [smooth] * len(me.polygons))
    return link(bpy.data.objects.new(name, me), shadow)


FXA = {'fx': (0.0, 1.0, 0.0, 1.0)}


def wm():
    """A part built straight in world metres (or creature cm for attach), with lc's ellipsoid and tube."""
    return lc.Mesh(FXA)


def done(m, name, mat, smooth=True, shadow=True, box=None):
    o = finish_bm(name, m.bm, mat, smooth, shadow)
    if box:
        bpy.context.view_layer.update()
        lt.box_uv(o, box)
    return o


def spider(name, pose, at=None, toward=None, scale=SCALE, mat=None, W=None, second=None):
    """A posed copy of the spider. Returns its record: obj, bone matrices M, world matrix W, solved feet."""
    M, feet = pose_matrices(pose)
    me = posed_mesh(name, M)
    me.materials.clear()
    me.materials.append(mat or SPIDER['MATERIAL'])
    if second is not None:
        # The abdomen in a second material (set after the slots: clearing slots remaps the indices).
        me.materials.append(second)
        me.polygons.foreach_set('material_index', (POLY_GROUP == GROUP_NAMES.index('abdomen')).astype(np.int32))
        me.update()
    o = link(bpy.data.objects.new(name, me))
    if W is None:
        W = Matrix.LocRotScale(Vector((at[0], at[1], at[2] if len(at) > 2 else 0.0)),
                               Euler((0.0, 0.0, G['heading_to'](at, toward))), Vector((scale,) * 3))
    o.matrix_world = W
    return dict(obj=o, M=M, W=W, feet=feet, scale=scale)


def cpt(rec, bone, p):
    """A creature-space point (cm, rest) on a bone, in world metres."""
    return rec['W'] @ (TOB @ (rec['M'][bone] @ Vector(p)))


def cdir(rec, bone, v):
    return (rec['W'].to_3x3() @ TOB.to_3x3() @ rec['M'][bone].to_3x3() @ Vector(v)).normalized()


def attach(rec, bone, m, name, mat, shadow=True, smooth=True, box=None):
    """A part built in creature cm (rest), moved with a bone of a placed spider."""
    bmesh.ops.transform(m.bm, matrix=rec['W'] @ TOB @ rec['M'][bone], verts=m.bm.verts)
    return done(m, name, mat, smooth, shadow, box)


def foot(rec, leg):
    return rec['W'] @ (TOB @ rec['feet'][leg])


def ell_point(center, radii, fwd, a, phi, out=1.0, deform=None):
    """A point on an ellipsoid (creature cm; a -1 rear .. 1 front along fwd, phi degrees round from the top, + toward
    her left) and its normal."""
    fwd = Vector(fwd).normalized()
    up = (Vector((0.0, 0.0, 1.0)) - fwd * fwd.z).normalized()
    left = up.cross(fwd)
    r = math.sqrt(max(1.0 - a * a, 0.0))
    s, u = r * math.sin(math.radians(phi)), r * math.cos(math.radians(phi))
    da, ds, du = deform(a, s, u) if deform else (a, s, u)
    rx, ry, rz = radii
    p = Vector(center) + (fwd * da * rx + left * ds * ry + up * du * rz) * out
    n = (fwd * (a / rx) + left * (s / ry) + up * (u / rz)).normalized()
    return p, n


def ab_point(a, phi, out=1.0):
    return ell_point(ABD_C, ABD['radii'], ABD_FWD, a, phi, out, ABD['deform'])


# --- Geometry helpers (world metres) ---

def basis(fwd, up=(0.0, 0.0, 1.0)):
    """Columns: x along fwd, z toward up."""
    f = Vector(fwd).normalized()
    u = Vector(up)
    u = u - f * u.dot(f)
    if u.length < 1e-4:
        u = Vector((0.0, 0.0, 1.0)) if abs(f.z) < 0.9 else Vector((1.0, 0.0, 0.0))
        u = u - f * u.dot(f)
    u.normalize()
    return Matrix((f, u.cross(f), u)).transposed()


def fbm(p, octaves=3):
    total, amp = 0.0, 1.0
    for _ in range(octaves):
        total += amp * noise.noise(p)
        p, amp = p * 2.1, amp * 0.5
    return total


def blobs(name, specs, mat, shadow=True, box=None, subdiv=2, smooth=True):
    """Lumpy ellipsoids in one mesh: specs of (center, radii, R (3x3 or None), seed, lumpiness)."""
    bm = bmesh.new()
    for center, radii, R, seed, lump in specs:
        made = bmesh.ops.create_icosphere(bm, subdivisions=subdiv, radius=1.0)
        off = Vector((seed * 1.7, seed * 2.3, seed * 0.9))
        R = R or Matrix.Identity(3)
        for v in made['verts']:
            d = v.co.copy()
            k = 1.0 + lump * fbm(d * 1.6 + off, 2)
            v.co = Vector(center) + R @ Vector((d.x * radii[0], d.y * radii[1], d.z * radii[2])) * k
    o = finish_bm(name, bm, mat, smooth, shadow)
    if box:
        bpy.context.view_layer.update()
        lt.box_uv(o, box, seed=7)
    return o


def ribbon(name, pts, widths, mat, across=None, lift=0.0, shadow=True):
    """A strip along pts: flat on the ground (across None) or turned by the across vectors. UV x along, y across."""
    n = len(pts)
    bm = bmesh.new()
    uv = bm.loops.layers.uv.new('UVMap')
    left, right = [], []
    for k in range(n):
        t = (pts[min(k + 1, n - 1)] - pts[max(k - 1, 0)])
        if across is None:
            t2 = Vector((t.x, t.y, 0.0)).normalized()
            side = Vector((-t2.y, t2.x, 0.0))
        else:
            tn = t.normalized()
            side = (across[k] - tn * across[k].dot(tn)).normalized()
        p = pts[k] + Vector((0.0, 0.0, lift))
        left.append(bm.verts.new(p + side * widths[k] * 0.5))
        right.append(bm.verts.new(p - side * widths[k] * 0.5))
    for k in range(n - 1):
        f = bm.faces.new((right[k], right[k + 1], left[k + 1], left[k]))
        for loop, c in zip(f.loops, ((k / (n - 1), 0.0), ((k + 1) / (n - 1), 0.0), ((k + 1) / (n - 1), 1.0),
                                     (k / (n - 1), 1.0))):
            loop[uv].uv = c
    return finish_bm(name, bm, mat, True, shadow)


def tube(name, pts, radii, mat, sides=8, shadow=True):
    m = wm()
    lc.tube(m, pts, radii, 'fx', sides=sides)
    return done(m, name, mat, shadow=shadow)


def tubes(name, paths, mat, sides=6, shadow=True):
    """Many thin tubes in one mesh: paths of (points, radii)."""
    m = wm()
    for pts, radii in paths:
        if len(pts) >= 2:
            lc.tube(m, pts, radii, 'fx', sides=sides)
    return done(m, name, mat, shadow=shadow)


def jag(p0, p1, seed, amp, step=0.2):
    """A crack's wandering line from p0 to p1 on the ground."""
    d = p1 - p0
    length = d.length
    n = max(int(length / step), 3)
    t_hat = d / length
    side = Vector((-t_hat.y, t_hat.x, 0.0))
    pts = []
    for k in range(n + 1):
        t = k / n
        o = amp * (noise.noise(Vector((t * length * 0.55, seed * 3.1, 0.5)))
                   + 0.4 * noise.noise(Vector((t * length * 2.6, seed * 1.3, 2.5))))
        o *= min(1.0, t * 6.0) if k else 0.0
        pts.append(p0 + d * t + side * o)
    return pts


def arc(p0, p1, apex, n=24, bend=0.0):
    d = p1 - p0
    side = Vector((-d.y, d.x, 0.0)).normalized()
    out = []
    for k in range(n + 1):
        t = k / n
        p = p0.lerp(p1, t)
        p.z += apex * 4.0 * t * (1.0 - t)
        out.append(p + side * bend * 4.0 * t * (1.0 - t))
    return out


def fan(name, at, radius_fn, mat, z=0.012, steps=72, dome=0.0, shadow=False):
    """A flat irregular disc (a decal stand-in) round at: radius_fn(theta) in metres."""
    bm = bmesh.new()
    uv = bm.loops.layers.uv.new('UVMap')
    c = bm.verts.new(Vector(at) + Vector((0.0, 0.0, z + dome)))
    ring = []
    for k in range(steps):
        th = 2.0 * math.pi * k / steps
        r = radius_fn(th)
        ring.append(bm.verts.new(Vector(at) + Vector((math.cos(th) * r, math.sin(th) * r, z))))
    for k in range(steps):
        f = bm.faces.new((c, ring[k], ring[(k + 1) % steps]))
        for loop in f.loops:
            loop[uv].uv = (0.5, 0.5)
    return finish_bm(name, bm, mat, True, shadow)


def splat_radius(base, seed, fingers=9, reach=0.7, rough=0.25):
    rng = random.Random(seed)
    spikes = [(rng.uniform(0, 2 * math.pi), rng.uniform(0.4, 1.0)) for _ in range(fingers)]

    def fn(th):
        r = 1.0 + rough * noise.noise(Vector((math.cos(th) * 2.0, math.sin(th) * 2.0, seed * 0.37)))
        for a, k in spikes:
            r += reach * k * max(0.0, math.cos(th - a)) ** 40
        return base * r
    return fn


def card(name, at, size, mat, roll_angle=None, aspect=1.0, shadow=False):
    """A soft sprite card: it is turned to face the camera before the render (face_cards)."""
    bm = bmesh.new()
    uv = bm.loops.layers.uv.new('UVMap')
    s = size * 0.5
    vs = [bm.verts.new(Vector((x * s * aspect, y * s, 0.0))) for x, y in ((-1, -1), (1, -1), (1, 1), (-1, 1))]
    f = bm.faces.new(vs)
    for loop, c in zip(f.loops, ((0, 0), (1, 0), (1, 1), (0, 1))):
        loop[uv].uv = c
    o = finish_bm(name, bm, mat, True, shadow)
    o.location = Vector(at)
    CARDS.append((o, random.uniform(0, 2 * math.pi) if roll_angle is None else roll_angle))
    return o


def face_cards(cam):
    for o, angle in CARDS:
        q = (cam.location - o.location).normalized().to_track_quat('Z', 'Y') @ Quaternion((0.0, 0.0, 1.0), angle)
        o.rotation_mode = 'QUATERNION'
        o.rotation_quaternion = q


def dust(name, at, radius, count, height, mat, seed, size=(0.7, 1.3), lift=0.15):
    rng = random.Random(seed)
    for i in range(count):
        a = rng.uniform(0, 2 * math.pi)
        r = radius * math.sqrt(rng.random())
        h = rng.uniform(lift, max(height, lift + 0.01))
        p = Vector(at) + Vector((math.cos(a) * r, math.sin(a) * r, h))
        card(f'{name}{i}', p, rng.uniform(*size) * radius, mat)


def dust_line(name, pts, radius, mat, seed, every=3, height=0.6):
    for k in range(0, len(pts), every):
        dust(f'{name}{k}_', pts[k], radius * (0.6 + 0.4 * k / len(pts)), 2, height, mat, seed + k)


# --- Materials ---

MATS = {}


def fresh(name):
    mat = bpy.data.materials.new(name)
    mat.use_nodes = True
    nt = mat.node_tree
    nt.nodes.clear()
    out = nt.nodes.new('ShaderNodeOutputMaterial')
    return mat, nt, out


def mnode(nt, op, a, b=None, c=None, clamp=False):
    n = nt.nodes.new('ShaderNodeMath')
    n.operation = op
    n.use_clamp = clamp
    for i, v in enumerate((a, b, c)):
        if v is None:
            continue
        if isinstance(v, (int, float)):
            n.inputs[i].default_value = v
        else:
            nt.links.new(v, n.inputs[i])
    return n.outputs[0]


def radial(nt, power=2.0):
    """1 at a card's middle, 0 at its edge."""
    tc = nt.nodes.new('ShaderNodeTexCoord')
    sub = nt.nodes.new('ShaderNodeVectorMath')
    sub.operation = 'SUBTRACT'
    sub.inputs[1].default_value = (0.5, 0.5, 0.0)
    nt.links.new(tc.outputs['UV'], sub.inputs[0])
    ln = nt.nodes.new('ShaderNodeVectorMath')
    ln.operation = 'LENGTH'
    nt.links.new(sub.outputs[0], ln.inputs[0])
    f = mnode(nt, 'MULTIPLY_ADD', ln.outputs['Value'], -2.0, 1.0, clamp=True)
    return mnode(nt, 'POWER', f, power)


def card_noise(nt, scale=3.0, detail=4.0):
    tc = nt.nodes.new('ShaderNodeTexCoord')
    info = nt.nodes.new('ShaderNodeObjectInfo')
    tex = nt.nodes.new('ShaderNodeTexNoise')
    tex.noise_dimensions = '4D'
    tex.inputs['Scale'].default_value = scale
    tex.inputs['Detail'].default_value = detail
    nt.links.new(tc.outputs['UV'], tex.inputs['Vector'])
    nt.links.new(mnode(nt, 'MULTIPLY', info.outputs['Random'], 50.0), tex.inputs['W'])
    return tex.outputs['Fac']


def mat_glow(name, color, strength, power=2.2):
    """Additive glow card: a soft halo (an emissive sprite in the game)."""
    if name in MATS:
        return MATS[name]
    mat, nt, out = fresh(name)
    f = radial(nt, power)
    em = nt.nodes.new('ShaderNodeEmission')
    em.inputs['Color'].default_value = lt.hex_color(color)
    nt.links.new(mnode(nt, 'MULTIPLY', f, strength), em.inputs['Strength'])
    add = nt.nodes.new('ShaderNodeAddShader')
    nt.links.new(nt.nodes.new('ShaderNodeBsdfTransparent').outputs[0], add.inputs[0])
    nt.links.new(em.outputs[0], add.inputs[1])
    nt.links.new(add.outputs[0], out.inputs['Surface'])
    mat.surface_render_method = 'BLENDED'
    mat.use_backface_culling = False
    MATS[name] = mat
    return mat


def mat_dust(name, color, opacity=0.6, power=1.4, scale=3.0, glow=0.0):
    """Lit soft dust or smoke card."""
    if name in MATS:
        return MATS[name]
    mat, nt, out = fresh(name)
    f = radial(nt, power)
    nz = card_noise(nt, scale)
    shaped = mnode(nt, 'MULTIPLY', f, mnode(nt, 'MULTIPLY_ADD', nz, 2.2, -0.75, clamp=True))
    alpha = mnode(nt, 'MULTIPLY', shaped, opacity, clamp=True)
    bsdf = nt.nodes.new('ShaderNodeBsdfPrincipled')
    bsdf.inputs['Base Color'].default_value = lt.hex_color(color)
    bsdf.inputs['Roughness'].default_value = 1.0
    if glow:
        bsdf.inputs['Emission Color'].default_value = lt.hex_color(color)
        bsdf.inputs['Emission Strength'].default_value = glow
    mix = nt.nodes.new('ShaderNodeMixShader')
    nt.links.new(alpha, mix.inputs['Fac'])
    nt.links.new(nt.nodes.new('ShaderNodeBsdfTransparent').outputs[0], mix.inputs[1])
    nt.links.new(bsdf.outputs[0], mix.inputs[2])
    nt.links.new(mix.outputs[0], out.inputs['Surface'])
    mat.surface_render_method = 'BLENDED'
    mat.use_backface_culling = False
    MATS[name] = mat
    return mat


def mat_trail(name, color, strength, power=1.6):
    """A fading emissive ribbon or tube (a projectile's trail): bright at its head (UV x = 1), soft at its edges."""
    if name in MATS:
        return MATS[name]
    mat, nt, out = fresh(name)
    tc = nt.nodes.new('ShaderNodeTexCoord')
    sep = nt.nodes.new('ShaderNodeSeparateXYZ')
    nt.links.new(tc.outputs['UV'], sep.inputs[0])
    along = mnode(nt, 'POWER', sep.outputs['X'], power)
    lw = nt.nodes.new('ShaderNodeLayerWeight')
    lw.inputs['Blend'].default_value = 0.5
    edge = mnode(nt, 'POWER', mnode(nt, 'SUBTRACT', 1.0, lw.outputs['Facing'], clamp=True), 1.5)
    em = nt.nodes.new('ShaderNodeEmission')
    em.inputs['Color'].default_value = lt.hex_color(color)
    nt.links.new(mnode(nt, 'MULTIPLY', mnode(nt, 'MULTIPLY', along, edge), strength), em.inputs['Strength'])
    add = nt.nodes.new('ShaderNodeAddShader')
    nt.links.new(nt.nodes.new('ShaderNodeBsdfTransparent').outputs[0], add.inputs[0])
    nt.links.new(em.outputs[0], add.inputs[1])
    nt.links.new(add.outputs[0], out.inputs['Surface'])
    mat.surface_render_method = 'BLENDED'
    mat.use_backface_culling = False
    MATS[name] = mat
    return mat


def mat_surface(name, color, rough=0.6, metal=0.0, glow_color=None, glow=0.0, coat=0.0, sss=0.0, sheen=0.0,
                alpha=1.0, cull=False):
    if name in MATS:
        return MATS[name]
    mat, nt, out = fresh(name)
    bsdf = nt.nodes.new('ShaderNodeBsdfPrincipled')
    bsdf.inputs['Base Color'].default_value = lt.hex_color(color)
    bsdf.inputs['Roughness'].default_value = rough
    bsdf.inputs['Metallic'].default_value = metal
    if glow:
        bsdf.inputs['Emission Color'].default_value = lt.hex_color(glow_color or color)
        bsdf.inputs['Emission Strength'].default_value = glow
    if coat:
        bsdf.inputs['Coat Weight'].default_value = coat
        bsdf.inputs['Coat Roughness'].default_value = 0.05
    if sss:
        bsdf.inputs['Subsurface Weight'].default_value = sss
        bsdf.inputs['Subsurface Scale'].default_value = 0.04
    if sheen:
        bsdf.inputs['Sheen Weight'].default_value = sheen
    if alpha < 1.0:
        bsdf.inputs['Alpha'].default_value = alpha
        mat.surface_render_method = 'DITHERED'
    nt.links.new(bsdf.outputs[0], out.inputs['Surface'])
    mat.use_backface_culling = cull
    mat.diffuse_color = lt.hex_color(color)
    MATS[name] = mat
    return mat


def mat_emit(name, color, strength):
    if name in MATS:
        return MATS[name]
    mat, nt, out = fresh(name)
    em = nt.nodes.new('ShaderNodeEmission')
    em.inputs['Color'].default_value = lt.hex_color(color)
    em.inputs['Strength'].default_value = strength
    nt.links.new(em.outputs[0], out.inputs['Surface'])
    MATS[name] = mat
    return mat


def mat_silk(name, color=0xf6f5f1, tatter=0.45):
    """Burial silk: pale, soft sheen, torn into tatters toward the strip's end (UV x = 1) and its edges."""
    if name in MATS:
        return MATS[name]
    mat, nt, out = fresh(name)
    tc = nt.nodes.new('ShaderNodeTexCoord')
    sep = nt.nodes.new('ShaderNodeSeparateXYZ')
    nt.links.new(tc.outputs['UV'], sep.inputs[0])
    mapping = nt.nodes.new('ShaderNodeMapping')
    mapping.inputs['Scale'].default_value = (14.0, 3.0, 1.0)
    nt.links.new(tc.outputs['UV'], mapping.inputs['Vector'])
    tex = nt.nodes.new('ShaderNodeTexNoise')
    tex.inputs['Scale'].default_value = 1.0
    tex.inputs['Detail'].default_value = 6.0
    nt.links.new(mapping.outputs[0], tex.inputs['Vector'])
    end = mnode(nt, 'MULTIPLY', mnode(nt, 'SUBTRACT', sep.outputs['X'], 0.55, clamp=True), 1.2)
    edge = mnode(nt, 'MULTIPLY', mnode(nt, 'ABSOLUTE', mnode(nt, 'SUBTRACT', sep.outputs['Y'], 0.5)), 0.5)
    keep = mnode(nt, 'GREATER_THAN', mnode(nt, 'SUBTRACT', tex.outputs['Fac'], mnode(nt, 'ADD', end, edge)),
                 0.5 - tatter)
    bsdf = nt.nodes.new('ShaderNodeBsdfPrincipled')
    bsdf.inputs['Base Color'].default_value = lt.hex_color(color)
    bsdf.inputs['Roughness'].default_value = 0.55
    bsdf.inputs['Sheen Weight'].default_value = 0.8
    bsdf.inputs['Subsurface Weight'].default_value = 0.3
    bsdf.inputs['Subsurface Scale'].default_value = 0.02
    nt.links.new(mnode(nt, 'MULTIPLY', keep, 0.92), bsdf.inputs['Alpha'])
    nt.links.new(bsdf.outputs[0], out.inputs['Surface'])
    mat.surface_render_method = 'DITHERED'
    mat.use_backface_culling = False
    MATS[name] = mat
    return mat


def textured(set_name, name, tint=None):
    if name not in MATS:
        MATS[name] = lt.material(set_name, name=name, tint=tint)
    return MATS[name]


# --- Hides ---

PALE_BC = os.path.join(lt.TEXTURE_DIR, 'SpiderBody_Pale', 'T_SpiderBody_Pale_BC.png')


def old_map(key):
    """GravemotherConcepts.py's B (Ash) or C (Ivory) color map, painted into Intermediate if it isn't there."""
    path = os.path.join(OLD_MAPS, key, f'T_SpiderBody_{key}_BC.png')
    if not os.path.exists(path):
        G['paint_maps'](SPIDER, ['Check', key])
    return path


def glow_mask(name, heart=False, eyes=False):
    """An emission mask in the spider's atlas: the heart mark's outline and the chevrons on the abdomen (as
    Spider.py's painter draws them), and the eyes."""
    size = 1024
    img = np.zeros((size, size), np.float32)
    if heart:
        sl, U, V = lc.region_grid(ATLAS['abdomen'], size)
        d, s = lc.body_axes(U, V)
        width, middle, half = SPIDER['BONE']['heart_shape']
        mark = ((np.abs(s) < width * np.clip(1 - ((V - middle) / half) ** 2, 0, 1)) & (d > 0)).astype(np.float32)
        soft = lt.blur(mark, 2.5)
        m = np.clip(np.clip(4.0 * soft * (1.0 - soft), 0, 1) + soft * 0.4, 0, 1)
        for k in range(4):
            vk = 0.5 - k * 0.11
            line = 1 - lt.smooth(0.012, 0.03, np.abs(V - (vk - 0.22 * np.abs(s))))
            m = np.maximum(m, line * (np.abs(s) < 0.42) * (d > 0) * (0.75 - 0.12 * k))
        img[sl] = np.maximum(img[sl], lt.blur(m, 1.2))
    if eyes:
        sl, _, _ = lc.region_grid(ATLAS['eye'], size)
        img[sl] = 1.0
    path = os.path.join(MAP_DIR, f'T_{name}_Glow.png')
    lt.write_png(path, lt.to8(np.stack([img] * 3, -1)))
    return path


def hide(name, bc=None, tint=None, mask=None, glow_color=None, glow=0.0):
    """MI_SpiderBody's look with its color map swapped (and a tint, and an emission mask for the lit looks)."""
    if name in MATS:
        return MATS[name]
    mat = lt.material('SpiderBody', name=name, tint=tint)
    nt = mat.node_tree
    if bc:
        for node in nt.nodes:
            if node.type == 'TEX_IMAGE' and node.image and os.path.basename(node.image.filepath) == 'T_SpiderBody_BC.png':
                image = bpy.data.images.load(bc, check_existing=True)
                image.colorspace_settings.name = 'sRGB'
                node.image = image
    if mask:
        bsdf = next(n for n in nt.nodes if n.type == 'BSDF_PRINCIPLED')
        tc = nt.nodes.new('ShaderNodeTexCoord')
        tex = nt.nodes.new('ShaderNodeTexImage')
        tex.image = bpy.data.images.load(mask, check_existing=True)
        tex.image.colorspace_settings.name = 'Non-Color'
        nt.links.new(tc.outputs['UV'], tex.inputs['Vector'])
        mix = nt.nodes.new('ShaderNodeMix')
        mix.data_type = 'RGBA'
        mix.blend_type = 'MULTIPLY'
        mix.inputs['Factor'].default_value = 1.0
        nt.links.new(tex.outputs['Color'], mix.inputs['A'])
        mix.inputs['B'].default_value = lt.hex_color(glow_color)
        nt.links.new(mix.outputs['Result'], bsdf.inputs['Emission Color'])
        bsdf.inputs['Emission Strength'].default_value = glow
    MATS[name] = mat
    return mat


# --- The set: the borrowed Sink and den, plus coffins and fallen blocks on the floor ---

def coffin(name, at, turn, mat, tilt=0.0, lid_off=False):
    """An old six-sided coffin, 2 m long."""
    prof = [(-1.0, 0.0), (-0.62, 0.32), (0.55, 0.22), (1.0, 0.0), (0.55, -0.22), (-0.62, -0.32)]
    bm = bmesh.new()
    low = [bm.verts.new(Vector((x * 1.0, y * 1.0, 0.0))) for x, y in prof]
    high = [bm.verts.new(Vector((x * 1.0, y * 1.0, 0.42))) for x, y in prof]
    bm.faces.new(list(reversed(low)))
    bm.faces.new(high)
    for k in range(len(prof)):
        bm.faces.new((low[k], low[(k + 1) % 6], high[(k + 1) % 6], high[k]))
    R = Matrix.Rotation(turn, 4, 'Z') @ Matrix.Rotation(tilt, 4, 'X')
    bmesh.ops.transform(bm, matrix=T(at) @ R, verts=bm.verts)
    o = finish_bm(name, bm, mat, smooth=False)
    SHOT.remove(o)
    bpy.context.view_layer.update()
    lt.box_uv(o, 'WoodPlanks', seed=3)
    return o


def sink_ring():
    """The rest of the Sink's wall (the borrowed set has only its east side, with the den): a plain bedded rock face
    round the west of the pit, so no shot sees past the pit. It casts no shadow, so the floor keeps the golden light."""
    rock = lt.material('RockCliff', MossAmount=0.6)
    center, radius = G['PIT_C'], G['PIT_R']
    a0, a1 = math.radians(72.0), math.radians(288.0)
    na, nz = 140, 34
    bm = bmesh.new()
    rows = []
    for i in range(na + 1):
        a = a0 + (a1 - a0) * i / na
        d = Vector((math.cos(a), math.sin(a), 0.0))
        col = []
        for j in range(nz + 1):
            z = -0.5 + 14.5 * j / nz
            p = Vector((a * radius * 0.16, z * 0.16, 4.2))
            bed = z / 2.4 + 0.3 * noise.noise(Vector((a * radius * 0.09, 3.0, 1.0)))
            frac = bed - math.floor(bed)
            out = 0.7 * fbm(p, 3) - 0.45 * lc.smoothstep(0.62, 0.9, frac) + 0.2 - 0.25 * lc.smoothstep(2.0, 0.0, z)
            col.append(bm.verts.new(center + d * (radius + 0.4 - out) + Vector((0.0, 0.0, z))))
        rows.append(col)
    for i in range(na):
        for j in range(nz):
            bm.faces.new((rows[i][j], rows[i][j + 1], rows[i + 1][j + 1], rows[i + 1][j]))
    o = finish_bm('_SinkRing', bm, rock, smooth=True, shadow=False)
    SHOT.remove(o)
    bpy.context.view_layer.update()
    lt.box_uv(o, 'RockCliff')
    return o


def build_stage():
    G['build_set']()
    sink_ring()
    sun, rim = G['build_light']()
    wood = textured('WoodPlanks', 'GMF_Coffin', tint=0x8a7a68)
    stone = textured('RockGranite', 'GMF_Block')
    props = [coffin('_CoffinA', (-15.5, 7.2, 0.0), 0.5, wood), coffin('_CoffinB', (-21.0, -5.8, 0.0), 2.1, wood, 0.12),
             coffin('_CoffinC', (2.2, -6.8, 0.0), -0.4, wood)]
    for i, (at, size, turn) in enumerate((((-9.0, 9.4, 0.25), (1.3, 0.8, 0.7), 0.4), ((-23.0, 3.0, 0.2), (1.0, 1.4, 0.8), 1.1),
                                          ((-3.5, -9.5, 0.3), (1.5, 0.9, 0.9), -0.3))):
        bm = bmesh.new()
        bmesh.ops.create_cube(bm, size=1.0)
        bmesh.ops.bevel(bm, geom=bm.edges[:], offset=0.06, segments=2, affect='EDGES')
        bmesh.ops.transform(bm, matrix=T(at) @ Matrix.Rotation(turn, 4, 'Z') @ Matrix.Rotation(0.15 * i, 4, 'X')
                            @ Matrix.Diagonal(Vector(size)).to_4x4(), verts=bm.verts)
        o = finish_bm(f'_Block{i}', bm, stone, smooth=False)
        SHOT.remove(o)
        bpy.context.view_layer.update()
        lt.box_uv(o, 'RockGranite', seed=i)
        props.append(o)
    G['SET'].extend(props)
    return sun, rim


def setup_compositor():
    SCENE.use_nodes = True
    tree = SCENE.node_tree
    tree.nodes.clear()
    rl = tree.nodes.new('CompositorNodeRLayers')
    comp = tree.nodes.new('CompositorNodeComposite')
    glare = tree.nodes.new('CompositorNodeGlare')
    try:
        glare.glare_type = 'BLOOM'
    except TypeError:
        glare.glare_type = 'FOG_GLOW'
    for attr, value in (('quality', 'HIGH'), ('threshold', 1.2), ('mix', 0.0), ('size', 7)):
        if hasattr(glare, attr):
            setattr(glare, attr, value)
    tree.links.new(rl.outputs['Image'], glare.inputs['Image'])
    tree.links.new(glare.outputs['Image'], comp.inputs['Image'])


FIG = None


def figure(at, toward, z=0.0):
    o = FIG.copy()
    link(o)
    o.parent = None
    o.hide_render = False
    o.location = Vector((at[0], at[1], z))
    o.rotation_euler = (0.0, 0.0, G['heading_to'](at, toward))
    return o


# --- Shared FX builders ---

def glob(name, at, r, vel, mat, seed, drops=4):
    vel = Vector(vel).normalized()
    specs = [(at, (r * 1.35, r, r * 0.95), basis(vel), seed, 0.12)]
    side = basis(vel) @ Vector((0.0, 1.0, 0.0))
    for k in range(1, drops + 1):
        p = Vector(at) - vel * r * (1.4 + 1.25 * k) + side * r * 0.35 * math.sin(k * 2.1 + seed)
        rr = r * max(0.42 - 0.07 * k, 0.12)
        specs.append((p, (rr * 1.3, rr, rr), basis(vel), seed + k, 0.08))
    return blobs(name, specs, mat, shadow=True)


def splash(name, at, r, mat, seed, count=14, decal=None):
    rng = random.Random(seed)
    specs = []
    for i in range(count):
        a = 2 * math.pi * i / count + rng.uniform(-0.2, 0.2)
        dist = r * rng.uniform(0.35, 0.95)
        h = r * rng.uniform(0.25, 1.15)
        d = Vector((math.cos(a), math.sin(a), 1.4 + rng.uniform(-0.4, 0.6))).normalized()
        p = Vector(at) + Vector((math.cos(a) * dist, math.sin(a) * dist, h))
        rr = r * rng.uniform(0.05, 0.1)
        specs.append((p, (rr * 2.8, rr, rr), basis(d), seed + i, 0.1))
    blobs(name, specs, mat, shadow=True)
    if decal is not None:
        fan(name + 'Splat', at, splat_radius(r * 0.9, seed, fingers=11, reach=0.8), decal, z=0.014)


def puddle(name, at, r, mat, seed, bubbles=6, bubble_mat=None):
    fan(name, at, splat_radius(r, seed, fingers=5, reach=0.3, rough=0.3), mat, z=0.012, dome=0.02)
    if bubbles:
        rng = random.Random(seed)
        specs = []
        for i in range(bubbles):
            a, d = rng.uniform(0, 2 * math.pi), r * math.sqrt(rng.random()) * 0.75
            b = r * rng.uniform(0.04, 0.09)
            specs.append((Vector(at) + Vector((math.cos(a) * d, math.sin(a) * d, 0.02)), (b, b, b * 0.7), None, i, 0.0))
        blobs(name + 'Bub', specs, bubble_mat or mat, shadow=False)


def chunks(name, pts, mat, seed, size=0.12, lift=0.0, airborne=0.0, box='GroundDirt'):
    rng = random.Random(seed)
    specs = []
    for p in pts:
        r = size * rng.uniform(0.5, 1.3)
        q = Vector(p) + Vector((0.0, 0.0, lift + rng.uniform(0, airborne)))
        R = Euler((rng.uniform(-0.8, 0.8), rng.uniform(-0.8, 0.8), rng.uniform(0, 6.3))).to_matrix()
        specs.append((q, (r * 1.2, r, r * 0.7), R, rng.randint(0, 999), 0.3))
    return blobs(name, specs, mat, box=box, subdiv=1, smooth=False)


def crack(name, p0, p1, width, seed, glow_mat=None, branches=4, lip_mat=None, crest=None):
    """A ground crack (a decal plus lips of broken earth): the dark split, its branches and the chunks pushed up along
    its edges. crest(t) lifts it onto a heave's top."""
    rng = random.Random(seed)
    dark = mat_surface('GMF_CrackDark', 0x120e0b, rough=0.95)
    pts = jag(p0, p1, seed, width * 1.4)
    n = len(pts)
    if crest:
        for k, p in enumerate(pts):
            p.z = crest(k / (n - 1))
    ws = [width * math.sin(math.pi * min(max(k / (n - 1), 0.03), 0.97)) ** 0.5 * (0.75 + 0.25 * noise.noise(
        Vector((k * 0.4, seed, 1.0)))) for k in range(n)]
    ribbon(name, pts, ws, dark, lift=0.012)
    if glow_mat:
        ribbon(name + 'Glow', pts, [w * 0.42 for w in ws], glow_mat, lift=0.016, shadow=False)
    lines = [(pts, ws)]
    for b in range(branches):
        k = rng.randint(2, n - 3)
        t = (pts[k + 1] - pts[k - 1]).normalized()
        ang = rng.choice((-1, 1)) * rng.uniform(0.6, 1.1)
        d = Matrix.Rotation(ang, 3, 'Z') @ t
        q1 = pts[k] + d * rng.uniform(0.5, 1.6)
        q1.z = pts[k].z
        bp = jag(pts[k], q1, seed + 10 + b, width * 0.8, step=0.15)
        bw = [ws[k] * 0.6 * (1 - j / (len(bp) - 1)) for j in range(len(bp))]
        ribbon(f'{name}B{b}', bp, bw, dark, lift=0.013)
        if glow_mat:
            ribbon(f'{name}BG{b}', bp, [w * 0.4 for w in bw], glow_mat, lift=0.017, shadow=False)
        lines.append((bp, bw))
    edge = []
    for line, lw in lines:
        for k in range(1, len(line) - 1, 2):
            t = (line[k + 1] - line[k - 1])
            side = Vector((-t.y, t.x, 0.0)).normalized()
            for sgn in (1, -1):
                edge.append(line[k] + side * sgn * (lw[k] * 0.5 + width * rng.uniform(0.25, 0.6)))
    chunks(name + 'Lip', edge, lip_mat or textured('GroundDirt', 'GMF_Dirt'), seed, size=width * 0.32, lift=0.02)
    return pts


def ridge(name, pts, half_w, heights, mat, seed, cols=11):
    """A heave of earth swept along pts: a soft mound across, heights per point."""
    bm = bmesh.new()
    n = len(pts)
    grid = []
    for k in range(n):
        t = (pts[min(k + 1, n - 1)] - pts[max(k - 1, 0)])
        side = Vector((-t.y, t.x, 0.0)).normalized()
        row = []
        t = k / (n - 1)
        ends = lc.smoothstep(0.0, 0.12, t) * lc.smoothstep(1.0, 0.88, t)      # no cut-off walls at the ends
        for j in range(cols):
            u = -1.0 + 2.0 * j / (cols - 1)
            h = heights[k] * ends * max(0.0, math.cos(u * math.pi / 2)) ** 1.3
            h *= 1.0 + 0.3 * noise.noise(Vector((k * 0.35, j * 0.6, seed)))
            jitter = side * half_w[k] * 0.08 * noise.noise(Vector((k * 0.5, j, seed + 3.0)))
            row.append(bm.verts.new(pts[k] + side * u * half_w[k] * 1.15 + jitter + Vector((0.0, 0.0, h - 0.03))))
        grid.append(row)
    for k in range(n - 1):
        for j in range(cols - 1):
            bm.faces.new((grid[k][j + 1], grid[k + 1][j + 1], grid[k + 1][j], grid[k][j]))
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces)
    o = finish_bm(name, bm, mat)
    bpy.context.view_layer.update()
    lt.box_uv(o, 'GroundDirt', seed=seed)
    return o


def flame(m, base, h, axis=(0.0, 0.0, 1.0), width=0.36):
    """A candle or ghost flame: a teardrop pinched to a point at its top (into an lc.Mesh, any units)."""
    axis = Vector(axis).normalized()

    def tear(a, s, u):
        k = (1.0 - 0.78 * max(a, 0.0) ** 0.8) * (1.0 + 0.12 * max(-a, 0.0))
        return a, s * k, u * k
    lc.ellipsoid(m, Vector(base) + axis * h * 0.42, (h * 0.55, h * width * 0.5, h * width * 0.5), 'fx', fwd=axis,
                 up=(1.0, 0.0, 0.0), segs=12, rings=10, deform=tear, patch=True)


def web(name, center, normal, radius, mat, seed, spokes=12, rings=6, sag=0.12, strand=0.012, domed=0.0):
    """A web of strands round center in the plane facing normal (spokes and rings; domed pushes the middle out)."""
    rng = random.Random(seed)
    R = basis(normal)
    u_axis, v_axis = R @ Vector((0.0, 1.0, 0.0)), R @ Vector((0.0, 0.0, 1.0))
    nrm = Vector(normal).normalized()
    ends = []
    for k in range(spokes):
        a = 2 * math.pi * k / spokes + rng.uniform(-0.12, 0.12)
        ends.append((a, radius * rng.uniform(0.75, 1.05)))

    def at(a, r):
        dome = domed * (1.0 - (r / radius) ** 2)
        return Vector(center) + (u_axis * math.cos(a) + v_axis * math.sin(a)) * r + nrm * dome
    paths = [([at(a, 0.0), at(a, r * 0.5), at(a, r)], [strand * 1.3, strand, strand * 0.8]) for a, r in ends]
    for j in range(1, rings + 1):
        f = j / (rings + 0.6)
        for k in range(spokes):
            a0, r0 = ends[k]
            a1, r1 = ends[(k + 1) % spokes]
            if a1 < a0:
                a1 += 2 * math.pi
            p0, p1 = at(a0, r0 * f), at(a1, r1 * f)
            mid = at((a0 + a1) / 2, (r0 + r1) / 2 * f * (1.0 - sag * 0.6))
            paths.append(([p0, mid, p1], [strand * 0.8] * 3))
    return tubes(name, paths, mat, sides=4, shadow=False)


def egg_sac(name, at, h, mat, strand_mat, seed, hang_to=None, torn=False):
    rng = random.Random(seed)
    specs = [(at, (h * 0.33, h * 0.31, h * 0.5), Euler((rng.uniform(-0.15, 0.15), rng.uniform(-0.15, 0.15), 0)).to_matrix(),
              seed, 0.08)]
    blobs(name, specs, mat, subdiv=3)
    paths = []
    for k in range(6):
        tilt = rng.uniform(-0.6, 0.6)
        pts = []
        for j in range(19):
            a = 2 * math.pi * j / 18
            pts.append(Vector(at) + Vector((math.cos(a) * h * 0.345, math.sin(a) * h * 0.325,
                                            h * (0.5 * math.sin(a) * tilt + (k - 2.5) * 0.12))))
        paths.append((pts, [h * 0.012] * len(pts)))
    if hang_to is not None:
        top = Vector(at) + Vector((0.0, 0.0, h * 0.48))
        paths.append(([top, top.lerp(Vector(hang_to), 0.5), Vector(hang_to)], [h * 0.02] * 3))
    tubes(name + 'Wrap', paths, strand_mat, sides=4)
    if torn:
        dark = mat_surface('GMF_SacInside', 0x2a1d17, rough=0.5, coat=0.6)
        fan(name + 'Hole', Vector(at) + Vector((0.0, 0.0, h * 0.49)), splat_radius(h * 0.22, seed, 7, 0.6),
            dark, z=0.0)


def wisp(name, at, r, core, halo, trail=None, trail_mat=None):
    blobs(name, [(at, (r, r, r), None, 3, 0.15)], core, shadow=False)
    card(name + 'Halo', at, r * 7.0, halo)
    card(name + 'Halo2', at, r * 3.0, halo)
    if trail:
        n = len(trail)
        tube(name + 'Trail', trail, [r * (0.12 + 0.7 * (k / (n - 1)) ** 1.5) for k in range(n)], trail_mat, sides=10,
             shadow=False)


# --- The Gravemother, dressed per look ---

def bone_hide():
    return hide('GMF_Bone', PALE_BC)


def gm_hide(sheet):
    if sheet == 'A':
        return bone_hide()
    if sheet == 'B':
        return hide('GMF_Vigil', PALE_BC, mask=glow_mask('Vigil', heart=True, eyes=True), glow_color=0xeef3b5, glow=8.0)
    if sheet == 'C':
        return hide('GMF_Ivory', old_map('C'))
    return hide('GMF_Barrow', old_map('B'), tint=0x8f8a82)


def ling(name, pose, at, toward, mat=None):
    return spider(name, pose, at, toward, LING, mat or SPIDER['MATERIAL'])


def ling_eyes():
    return hide('GMF_LingEyes', None, mask=glow_mask('LingEyes', eyes=True), glow_color=0xeef3b5, glow=12.0)


def riders(rec, count=4):
    """A: her brood riding on her abdomen, clinging, heads every way."""
    spots = ((0.18, -30, 30), (-0.42, 34, -40), (-0.15, -78, 80), (-0.8, 8, 170))[:count]
    out = []
    for i, (a, phi, turn) in enumerate(spots):
        p, n = ab_point(a, phi, 1.0)
        P = cpt(rec, 'abdomen', p)
        N = cdir(rec, 'abdomen', n)
        fwd = cdir(rec, 'abdomen', (math.cos(math.radians(turn)), math.sin(math.radians(turn)), 0.0))
        fwd = (fwd - N * fwd.dot(N)).normalized()
        left = N.cross(fwd)
        R = Matrix((left, -fwd, N)).transposed().to_4x4()
        W = T(P - N * 0.03) @ R @ Matrix.Scale(LING, 4)
        out.append(spider(f'Rider{i}', CLING, mat=SPIDER['MATERIAL'], W=W))
    return out


CANDLES = ((0.32, -8, 30, 6.5), (0.2, 14, 22, 5.5), (0.42, 20, 38, 7.0),       # (a, phi, height cm, radius cm)
           (-0.22, -40, 26, 6.0), (-0.35, -26, 40, 7.5), (-0.1, -52, 18, 5.0),
           (-0.3, 38, 34, 7.0), (-0.48, 22, 24, 6.0), (-0.12, 50, 20, 5.0))


def candles(rec, lit=9, lean_to=None, smoke=False):
    """B: chapel candles stuck in wax on her abdomen. The first `lit` burn; flames lean toward lean_to (the tell)."""
    wax, wick = mat_surface('GMF_Wax', 0xe6dcc4, rough=0.42, sss=0.6), mat_surface('GMF_Wick', 0x1b1612, rough=0.8)
    fire = mat_emit('GMF_Flame', 0xfff6d8, 14.0)
    halo = mat_glow('GMF_FlameHalo', 0xe9f0b0, 2.0, power=3.0)
    smoke_mat = mat_dust('GMF_Smoke', 0xc4c0b8, 0.9, power=1.1)
    mw, mk, mf = wm(), wm(), wm()
    flames = []
    for i, (a, phi, h, r) in enumerate(CANDLES):
        p, n = ab_point(a, phi, 0.97)
        axis = (n * 0.3 + Vector((0.0, 0.0, 1.0)) * 0.7).normalized()
        base = p - axis * 5.0
        top = p + axis * h
        lc.tube(mw, [base, base.lerp(top, 0.5) + Vector((0.4, 0.3, 0.0)), top], [r, r * 1.03, r * 0.96], 'fx', sides=14)
        lc.ellipsoid(mw, top - axis * 1.0, (r * 0.35, r * 1.02, r * 1.02), 'fx', fwd=axis, segs=14, rings=6)
        lc.ellipsoid(mw, p + n * 1.0, (r * 0.9, r * 2.6, r * 2.4), 'fx', fwd=n, segs=16, rings=8)
        for j in range(2):
            ang = phi * 0.1 + j * 2.4 + i
            side = basis(axis) @ Vector((0.0, math.cos(ang), math.sin(ang)))
            s0 = top + side * r * 0.95
            lc.tube(mw, [s0, s0 - axis * h * 0.25 + side * 0.8, s0 - axis * h * (0.35 + 0.15 * j) + side * 1.0],
                    [r * 0.28, r * 0.24, r * 0.33], 'fx', sides=8)
        lc.tube(mk, [top, top + axis * r * 0.5], [0.5, 0.4], 'fx', sides=5)
        if i < lit:
            fa = axis
            if lean_to is not None:
                lean = (rec['W'] @ TOB).inverted() @ Vector(lean_to) - rec['M']['abdomen'] @ top
                lean = rec['M']['abdomen'].inverted().to_3x3() @ lean.normalized()
                fa = (axis * 0.45 + lean * 0.55).normalized()
            fh = r * (2.6 if lean_to is None else 3.4)
            flame(mf, top + axis * r * 0.35, fh, fa)
            flames.append(top + axis * r * 0.35 + fa * fh * 0.45)
        elif smoke:
            flames.append(None)
    attach(rec, 'abdomen', mw, 'Candles', wax)
    attach(rec, 'abdomen', mk, 'Wicks', wick)
    if mf.bm.verts:
        attach(rec, 'abdomen', mf, 'Flames', fire, shadow=False)
    else:
        mf.bm.free()
    for i, f in enumerate(flames):
        if f is not None:
            card(f'FlameHalo{i}', cpt(rec, 'abdomen', f), 0.6, halo)
    if smoke:
        for i, (a, phi, h, r) in enumerate(CANDLES[lit:]):
            p, n = ab_point(a, phi, 0.97)
            top = cpt(rec, 'abdomen', p + Vector((0.0, 0.0, h + 6.0)))
            for j in range(4):
                card(f'Smoke{i}_{j}', top + Vector((0.05 * j, 0.03 * j, 0.25 + 0.35 * j)), 0.4 + 0.25 * j, smoke_mat)
    return [cpt(rec, 'abdomen', f) for f in flames if f is not None]


def shroud(rec, anchors=None, train=True):
    """C: burial silk wrapped over her abdomen and trailing behind, drag-lines from her spinnerets to anchors."""
    silk = mat_silk('GMF_Silk')
    strand = mat_surface('GMF_Strand', 0xf2efe8, rough=0.35, glow=0.25, glow_color=0xffffff)
    rng = random.Random(11)
    # Built in world space: the silk lies on her abdomen as posed, and its loose ends fall straight to the floor.
    s = rec['scale'] * 0.01
    back = -cdir(rec, 'body', (1.0, 0.0, 0.0))
    back = Vector((back.x, back.y, 0.0)).normalized()
    left = Vector((0.0, 0.0, 1.0)).cross(-back)
    # Wrappings: bands wound round her abdomen, their loose ends hanging off her flanks and streaming back.
    bands = ((0.45, 1, 34), (0.2, -1, 32), (-0.05, 1, 38), (-0.3, -1, 34), (-0.55, 1, 30), (0.05, -1, 28),
             (-0.65, -1, 30), (0.3, 1, 28), (0.62, -1, 26), (-0.4, 1, 26))
    for i, (a, sgn, w) in enumerate(bands):
        pts = []
        for phi in range(-25, 101, 8):
            p, _ = ab_point(a + 0.05 * math.sin(phi * 0.05 + i) - 0.1 * max(phi - 60, 0) / 40, sgn * phi, 1.03)
            pts.append(cpt(rec, 'abdomen', p))
        last = pts[-1]
        hang = (28.0 + 14.0 * (i % 3)) * s
        for k in range(1, 6):
            q = last + back * k * 6.0 * s + left * sgn * k * 1.5 * s
            q.z = max(last.z - hang * k / 5, 0.05)
            pts.append(q)
        widths = [w * s * (1.0 + 0.25 * noise.noise(Vector((k * 0.3, i, 0.0)))) for k in range(len(pts))]
        ribbon(f'Shroud{i}', pts, widths, silk, across=[-back] * len(pts))
    # The train: three long strips from the top of her abdomen back over her spinnerets and along the floor.
    for i, y in enumerate((-14.0, 2.0, 16.0) if train else ()):
        pts = [cpt(rec, 'abdomen', ab_point(0.1 - k * 0.1, y * 0.9, 1.04)[0]) for k in range(11)]
        p = pts[-1]
        side = left * y * 0.6 * s
        for k in range(1, 4):
            q = p + back * 22.0 * s * k + side * k
            q.z = p.z * (1.0 - k / 3.0) + 0.03 + 0.01 * i
            pts.append(q)
        for k in range(8):
            q = pts[-1] + back * 30.0 * s + side + left * rng.uniform(-5, 5) * s
            q.z = 0.03 + 0.01 * i
            pts.append(q)
        n = len(pts)
        ribbon(f'Train{i}', pts, [40.0 * s * (1.0 + 0.6 * k / n) for k in range(n)], silk, across=[left] * n)
    if anchors:
        s = cpt(rec, 'abdomen', SPINNERETS)
        tubes('DragLines', [([s, s.lerp(Vector(a), 0.5), Vector(a)], [0.02, 0.016, 0.02]) for a in anchors], strand,
              sides=5, shadow=False)
        glint = mat_glow('GMF_Glint', 0xffffff, 6.0, power=3.0)
        for i, a in enumerate(anchors):
            for t in (0.3, 0.62):
                card(f'Glint{i}{t}', s.lerp(Vector(a), t), 0.2, glint, roll_angle=0.6)


def headstone_mesh(m, base, w, h, th, R):
    """An arched headstone (creature cm): its foot centred on base, broad face along R's x axis."""
    prof = [(-w / 2, 0.0), (w / 2, 0.0), (w / 2, h - w / 2)]
    for k in range(1, 12):
        a = math.pi * k / 12
        prof.append((math.cos(a) * w / 2, h - w / 2 + math.sin(a) * w / 2))
    prof.append((-w / 2, h - w / 2))
    bm = m.bm
    front = [bm.verts.new(Vector(base) + R @ Vector((x, -th / 2, z))) for x, z in prof]
    back = [bm.verts.new(Vector(base) + R @ Vector((x, th / 2, z))) for x, z in prof]
    uv = m.at('fx', 0.5, 0.5)
    for face in (list(reversed(front)), back):
        f = bm.faces.new(face)
        for loop in f.loops:
            loop[m.uv].uv = uv
    k = len(prof)
    for i in range(k):
        f = bm.faces.new((front[i], back[i], back[(i + 1) % k], front[(i + 1) % k]))
        for loop in f.loops:
            loop[m.uv].uv = uv


STONE_W, STONE_H, STONE_T = 80.0, 116.0, 17.0


def stone_frame():
    base, n = ab_point(-0.12, 0.0, 0.8)
    R = (Matrix.Rotation(math.radians(-90.0), 3, 'Z') @ Matrix.Rotation(math.radians(26.0), 3, 'X')
         @ Matrix.Rotation(math.radians(7.0), 3, 'Y'))
    return base, R


def barrow(rec, stone=True):
    """D: the headstone on her back, roots binding it, grave mud on her back and legs."""
    granite = textured('RockGranite', 'GMF_Granite', tint=0xb8b4ac)
    bark = textured('BarkOak', 'GMF_Roots', tint=0x9a8670)
    mud = textured('GroundDirt', 'GMF_Mud', tint=0xa69a8a)
    groove = mat_surface('GMF_Groove', 0x34312d, rough=0.9)
    base, R = stone_frame()
    if stone:
        m = wm()
        headstone_mesh(m, base, STONE_W, STONE_H, STONE_T, R)
        attach(rec, 'abdomen', m, 'Headstone', granite, smooth=False, box='RockGranite')
        # The carving: an arched border groove and three worn lines of letters, and a crack across the face.
        g = wm()
        face = base + R @ Vector((0.0, STONE_T / 2 + 0.2, 0.0))
        half, arch = STONE_W / 2 - 7.0, STONE_H - STONE_W / 2
        border = [face + R @ Vector((math.cos(math.pi * k / 16) * half, 0.0, arch + math.sin(math.pi * k / 16) * half))
                  for k in range(17)]
        border = [face + R @ Vector((half, 0.0, 22.0))] + border + [face + R @ Vector((-half, 0.0, 22.0))]
        lc.tube(g, border, [0.7] * len(border), 'fx', sides=5)
        rng_l = random.Random(9)
        for z, wl in ((arch + 8.0, 36.0), (arch - 8.0, 46.0), (arch - 22.0, 30.0)):
            x = -wl / 2
            while x < wl / 2:                     # worn letters: short strokes of uneven length
                stroke = rng_l.uniform(2.0, 4.5)
                lc.tube(g, [face + R @ Vector((x, 0.0, z)), face + R @ Vector((x + stroke, 0.0, z + rng_l.uniform(-1, 1)))],
                        [0.9, 0.9], 'fx', sides=5)
                x += stroke + rng_l.uniform(1.2, 3.0)
        cr = [face + R @ Vector((x, 0.0, z)) for x, z in ((-40, 96), (-22, 84), (-15, 70), (2, 62), (9, 46), (27, 36),
                                                         (40, 26))]
        lc.tube(g, cr, [1.2, 1.7, 1.4, 1.6, 1.3, 1.1, 0.6], 'fx', sides=5)
        attach(rec, 'abdomen', g, 'Carving', groove)
    # Roots from the stone's foot, gripping round her abdomen; two over the stone's face.
    rng = random.Random(5)
    roots = wm()
    for k in range(9):
        a0 = rng.uniform(-0.35, 0.15)
        sgn = 1 if k % 2 else -1
        pts = []
        for j in range(9):
            t = j / 8
            p, _ = ab_point(a0 + 0.25 * t * math.sin(k), sgn * (5 + t * (95 + 10 * (k % 3))), 1.02 + 0.01 * j)
            pts.append(p)
        lc.tube(roots, pts, [3.6 - 2.6 * (j / 8) for j in range(9)], 'fx', sides=7)
    if stone:
        for sgn in (1, -1):
            top = base + R @ Vector((sgn * 30.0, STONE_T / 2 + 2.0, 80.0))
            mid = base + R @ Vector((sgn * 12.0, STONE_T / 2 + 2.5, 40.0))
            low, _ = ab_point(0.15, sgn * 25.0, 1.03)
            lc.tube(roots, [top, mid, base + R @ Vector((sgn * 4.0, STONE_T / 2 + 2.0, 6.0)), low], [1.6, 2.4, 3.2, 2.6],
                    'fx', sides=7)
    attach(rec, 'abdomen', roots, 'Roots', bark, box='BarkOak')
    # Grave mud: clumps round the stone's foot and on her back, crusts on her legs' lower joints.
    specs = []
    for k in range(7):
        p, n = ab_point(rng.uniform(-0.5, 0.25), rng.uniform(-45, 45), 0.98)
        R2 = basis(n, (1.0, 0.0, 0.0))
        specs.append((p, (9.0, 20.0, 16.0), R2, k, 0.35))
    m = wm()
    m.bm.free()
    mudb = bmesh.new()
    for center, radii, R2, seed, lump in specs:
        made = bmesh.ops.create_icosphere(mudb, subdivisions=2, radius=1.0)
        for v in made['verts']:
            d = v.co.copy()
            kk = 1.0 + lump * fbm(d * 1.6 + Vector((seed, seed * 2, 0)), 2)
            v.co = center + R2 @ Vector((d.x * radii[0], d.y * radii[1], d.z * radii[2])) * kk
    m.bm = mudb
    attach(rec, 'abdomen', m, 'BackMud', mud, box='GroundDirt')
    for pair, s, side in LEGS:
        leg = f'{pair}_{s}'
        K0, F0 = BONES[f'tibia_{leg}']
        mm = wm()
        mm.bm.free()
        lb = bmesh.new()
        rl = random.Random(pair * 7 + (3 if side > 0 else 0))
        d = (F0 - K0).normalized()
        R3 = basis(d)
        for j, t in enumerate((0.5, 0.6, 0.7, 0.8, 0.9)):
            # Caked mud: overlapping uneven lumps a little thicker than the leg, thickest low down.
            leg_r = SPIDER['LEG_RADIUS'] * (0.84 - 0.62 * t ** 1.1)
            c = K0.lerp(F0, t) + R3 @ Vector((0.0, rl.uniform(-1.5, 1.5), rl.uniform(-1.5, 1.5)))
            made = bmesh.ops.create_icosphere(lb, subdivisions=2, radius=1.0)
            rr = leg_r * rl.uniform(1.35, 1.7) * (0.8 + 0.4 * t)
            for v in made['verts']:
                dd = v.co.copy()
                kk = 1.0 + 0.4 * fbm(dd * 1.9 + Vector((pair, j, side)), 2)
                v.co = c + R3 @ Vector((dd.x * rr * 2.6, dd.y * rr, dd.z * rr)) * kk
        mm.bm = lb
        attach(rec, f'tibia_{leg}', mm, f'LegMud{leg}', mud, box='GroundDirt')


def gravemother(sheet, pose, at, toward, phase=1, lean_to=None, anchors=None, train=True):
    """She in a sheet's look, dressed. phase: 1 the fight's start; 2 after a call (A: no riders, B: six candles lit
    and three smoking); 3 (D) the headstone gone and her pale abdomen bare."""
    second = bone_hide() if (sheet == 'D' and phase == 3) else None
    rec = spider(f'GM_{sheet}', pose, at, toward, SCALE, gm_hide(sheet), second=second)
    rec['extra'] = {}
    if sheet == 'A' and phase == 1:
        rec['extra']['riders'] = riders(rec)
    elif sheet == 'B':
        rec['extra']['flames'] = candles(rec, lit=9 if phase == 1 else 6, lean_to=lean_to, smoke=phase != 1)
    elif sheet == 'C':
        shroud(rec, anchors, train)
    elif sheet == 'D':
        barrow(rec, stone=phase != 3)
    return rec


def crit_marks(rec, sheet):
    head = cpt(rec, 'head', HEAD_C + Vector((16.0, 0.0, 10.0)))
    if sheet == 'D':
        abd = cpt(rec, 'abdomen', ABD_C + Vector((-50.0, 0.0, 6.0)))
    elif sheet == 'C':
        abd = cpt(rec, 'abdomen', ABD_C + Vector((-8.0, 0.0, 36.0)))
    else:
        abd = cpt(rec, 'abdomen', ABD_C + Vector((4.0, 0.0, 34.0)))
    return [('head', head), ('abdomen', abd)]


# --- Light and render ---

RESULTS = {}


def light(kind, sun, rim):
    rim.hide_render = kind != 'shade'
    sun.data.energy = 4.6


def shoot(sheet, shot, cam_loc, target, lens, size, samples, marks=(), exposure=0.0):
    cam = G['camera'](cam_loc, target, lens)
    face_cards(cam)
    G['setup_render'](samples if not QUICK else 12)
    SCENE.view_settings.exposure = exposure
    SCENE.render.resolution_x, SCENE.render.resolution_y = size
    SCENE.render.resolution_percentage = 50 if QUICK else 100
    path = os.path.join(OUT, f'GMF_{sheet}_{shot}.png')
    SCENE.render.filepath = path
    t0 = time.time()
    bpy.ops.render.render(write_still=True)
    log(f'rendered {path} in {time.time() - t0:.1f} s')
    points = []
    for key, p in marks:
        c = world_to_camera_view(SCENE, cam, Vector(p))
        points.append(dict(key=key, x=round(c.x * size[0], 1), y=round((1.0 - c.y) * size[1], 1), depth=round(c.z, 2)))
    RESULTS.setdefault(sheet, {})[shot] = dict(file=os.path.basename(path), w=size[0], h=size[1], marks=points)


def dust_mat():
    """Dust a shade lighter than the floor it rises from, so it reads against it."""
    return mat_dust('GMF_Dust', 0xd6c4a0, 0.8)


def chalk():
    """A's crack: the Sink's floor split down to the chalky bone-white layer under it."""
    return mat_surface('GMF_Chalk', 0xd9d1c1, rough=0.9)


# --- The shots. The Sink: the den's mouth at x 8 facing west, the floor west of it; sun from the west-south-west. ---

# From the floor's middle, a little above a player's eye: she stands with the den's dark mouth behind her.
HERO_AT, HERO_TOWARD = (-1.0, 1.0), (-10.0, -3.6)
HERO_CAM, HERO_TARGET, HERO_LENS = (-11.0, 2.7, 2.4), (-1.4, 0.8, 1.55), 32.0
PLAYER = (-13.6, -2.2)


def shot_hero(sheet, sun, rim):
    light('golden', sun, rim)
    pose = {'A': THREAT, 'B': VIGIL, 'C': SHROUDED, 'D': HEAVY}[sheet]
    anchors = [(7.8, 2.6, 3.2), (7.6, -2.4, 3.6), (7.9, 0.4, 4.3)] if sheet == 'C' else None
    # C turns her flank to the camera, so her silk train shows.
    rec = gravemother(sheet, pose, HERO_AT, (-6.0, -9.0) if sheet == 'C' else HERO_TOWARD, anchors=anchors)
    marks = crit_marks(rec, sheet)
    if sheet == 'A':
        marks.append(('brood', rec['extra']['riders'][2]['W'].translation.copy()))
    elif sheet == 'B':
        marks.append(('candles', rec['extra']['flames'][4]))
    elif sheet == 'C':
        marks.append(('spinnerets', cpt(rec, 'abdomen', SPINNERETS)))
        web('DenWebL', (7.7, 2.7, 3.3), (-1.0, 0.0, 0.0), 1.5, mat_surface('GMF_Strand', 0xf2efe8), 3, sag=0.2)
        web('DenWebR', (7.7, -2.8, 3.0), (-1.0, 0.0, 0.0), 1.2, mat_surface('GMF_Strand', 0xf2efe8), 4, sag=0.2)
    elif sheet == 'D':
        base, R = stone_frame()
        marks.append(('headstone', cpt(rec, 'abdomen', base + R @ Vector((0.0, 0.0, 62.0)))))
    dust('HeroDust', foot(rec, '2_r'), 0.9, 5, 0.5, dust_mat(), 3)
    shoot(sheet, 'hero', HERO_CAM, HERO_TARGET, HERO_LENS, HERO_SIZE, 64, marks)


SPIT_CAM, SPIT_TARGET = (-15.6, -7.4, 2.8), (-5.2, -0.2, 1.2)


def shot_spit(sheet, sun, rim):
    light('golden', sun, rim)
    fig_at = (-10.6, -1.8)
    figure(fig_at, HERO_AT)
    land = [Vector((fig_at[0] + 1.5, fig_at[1] + 1.7, 0.0)), Vector((fig_at[0] - 1.2, fig_at[1] - 1.9, 0.0)),
            Vector((fig_at[0] + 0.4, fig_at[1] + 0.8, 0.0))]
    marks = []
    if sheet == 'A':
        rec = gravemother('A', SPIT, (-1.0, 0.8), fig_at)
        mouth = cpt(rec, 'head', MOUTH)
        milk = mat_surface('GMF_Milk', 0xe6e2cf, rough=0.12, coat=1.0, glow=0.35, glow_color=0xd9e59a, sss=0.3)
        pool = mat_surface('GMF_MilkPool', 0xd8d4c0, rough=0.08, coat=1.0, glow=0.5, glow_color=0xd9e59a)
        trail = mat_trail('GMF_MilkTrail', 0xe8edc0, 2.5)
        paths = [arc(mouth, land[0], 2.2, bend=0.6), arc(mouth, land[1], 2.6, bend=-0.8), arc(mouth, land[2], 2.4)]
        # One landed (splash and puddle), one about to land, one just spat.
        splash('Splash0', land[0], 0.55, milk, 1, decal=pool)
        puddle('Puddle0', land[0], 0.85, pool, 2)
        dust('SplashDust', land[0], 0.7, 4, 0.5, dust_mat(), 5)
        for i, (path, t) in enumerate(((paths[1], 0.86), (paths[2], 0.35))):
            k = int(t * (len(path) - 1))
            vel = path[k + 1] - path[k]
            glob(f'Glob{i}', path[k], 0.27, vel, milk, 10 + i)
            tube(f'Trail{i}', path[max(k - 9, 0):k + 1], [0.03 + 0.15 * (j / 9) for j in range(len(path[max(k - 9, 0):k + 1]))],
                 trail, sides=10, shadow=False)
        marks = [('tell', mouth)]
    elif sheet == 'B':
        rec = gravemother('B', VIGIL, (-1.0, 0.8), fig_at, lean_to=Vector((fig_at[0], fig_at[1], 1.4)))
        core, halo = mat_emit('GMF_WispCore', 0xfdfbe8, 18.0), mat_glow('GMF_WispHalo', 0xe3eea6, 3.5)
        trail = mat_trail('GMF_WispTrail', 0xe3eea6, 5.0)
        start = rec['extra']['flames'][2]
        paths = [arc(start, land[0], 3.0, bend=1.4), arc(start, land[1], 3.4, bend=-1.6), arc(start, land[2], 2.6,
                                                                                              bend=0.4)]
        for i, (path, t) in enumerate(((paths[1], 0.8), (paths[2], 0.45))):
            k = int(t * (len(path) - 1))
            wisp(f'Wisp{i}', path[k], 0.16, core, halo, trail=path[max(k - 10, 0):k + 1], trail_mat=trail)
        flame_ring('Ring', land[0], 1.1, 5)
        marks = [('tell', rec['extra']['flames'][4])]
    elif sheet == 'C':
        rec = gravemother('C', TURNED, (-1.0, 0.8), (8.0, 3.0), train=False)
        spin = cpt(rec, 'abdomen', SPINNERETS)
        strand = mat_surface('GMF_Strand', 0xf2efe8, rough=0.35, glow=0.25, glow_color=0xffffff)
        sac = mat_surface('GMF_SilkBall', 0xe9e4d8, rough=0.6, sheen=0.8, sss=0.4)
        path = arc(spin, land[0], 2.0, bend=0.3)
        k = int(0.55 * (len(path) - 1))
        silk_wad('Wad', path[k], 0.24, path[k + 1] - path[k], sac, strand, 3)
        tubes('Unspool', [(path[max(k - 8, 0):k + 1], [0.008] * len(path[max(k - 8, 0):k + 1]))], strand, sides=4)
        p2 = arc(spin, land[1], 1.8)[20]
        web('Net', p2, (p2 - spin).normalized(), 0.8, strand, 9, spokes=11, rings=4, domed=-0.35, strand=0.014)
        web_patch('Patch', land[2] + Vector((0.3, 0.3, 0.0)), 1.5, strand, 4)
        marks = [('tell', cpt(rec, 'abdomen', ABD_C + Vector((-8.0, 0.0, 36.0))))]
    else:
        rec = gravemother('D', CLOD, (-1.0, 0.8), fig_at)
        held = (foot(rec, '0_l') + foot(rec, '0_r')) * 0.5 + Vector((0.0, 0.0, 0.25))
        clod('Held', held, 0.55, 1)
        path = arc(held, land[1], 2.4, bend=-0.5)
        k = int(0.82 * (len(path) - 1))
        clod('Flying', path[k], 0.42, 2)
        dust_trail = mat_dust('GMF_DustThin', 0xd6c4a0, 0.55)
        for j in range(max(k - 8, 0), k, 2):
            card(f'Trail{j}', path[j], 0.35 + 0.05 * (k - j), dust_trail)
        clod_impact('Impact', land[0], 4)
        marks = [('tell', held)]
    shoot(sheet, 'spit', SPIT_CAM, SPIT_TARGET, 30.0, PANEL_SIZE, 48, marks)


def silk_wad(name, at, r, vel, mat, strand, seed):
    blobs(name, [(at, (r * 1.1, r, r), basis(vel), seed, 0.15)], mat, subdiv=3)
    rng = random.Random(seed)
    paths = []
    for k in range(7):
        ax = Vector((rng.uniform(-1, 1), rng.uniform(-1, 1), rng.uniform(-1, 1))).normalized()
        R = basis(ax)
        pts = [Vector(at) + R @ Vector((0.0, math.cos(a) * r * 1.06, math.sin(a) * r * 1.06))
               for a in (2 * math.pi * j / 16 for j in range(17))]
        paths.append((pts, [r * 0.03] * len(pts)))
    tubes(name + 'Wind', paths, strand, sides=4)


def web_patch(name, at, radius, strand, seed):
    """C: a shroud shot's landing: a sticky web on the floor, anchored up to the rocks round it."""
    web(name, Vector(at) + Vector((0.0, 0.0, 0.03)), (0.0, 0.0, 1.0), radius, strand, seed, spokes=14, rings=6,
        sag=0.25, strand=0.014)
    sheen = mat_surface('GMF_WebSheen', 0xe9e6dc, rough=0.25, alpha=0.35)
    fan(name + 'Sheen', at, splat_radius(radius * 0.8, seed, 6, 0.3), sheen, z=0.02)
    rng = random.Random(seed)
    paths = []
    for k in range(5):
        a = 2 * math.pi * k / 5 + rng.uniform(-0.3, 0.3)
        p0 = Vector(at) + Vector((math.cos(a) * radius * 0.9, math.sin(a) * radius * 0.9, 0.03))
        p1 = Vector(at) + Vector((math.cos(a) * radius * 1.6, math.sin(a) * radius * 1.6, rng.uniform(0.3, 0.8)))
        paths.append(([p0, p0.lerp(p1, 0.5), p1], [0.01] * 3))
    tubes(name + 'Anchors', paths, strand, sides=4)


def flame_ring(name, at, radius, seed, count=11):
    """B: a corpse-light's landing: a ring of cold flame and a scorch."""
    rng = random.Random(seed)
    fire, halo = mat_emit('GMF_ColdFlame', 0xd4ec96, 7.0), mat_glow('GMF_WispHalo', 0xe3eea6, 3.5)
    scorch = mat_surface('GMF_Scorch', 0x5c5249, rough=0.95)
    fan(name + 'Scorch', at, splat_radius(radius * 1.15, seed, 8, 0.4), scorch, z=0.011)
    m = wm()
    for k in range(count):
        a = 2 * math.pi * k / count + rng.uniform(-0.15, 0.15)
        p = Vector(at) + Vector((math.cos(a) * radius, math.sin(a) * radius, 0.0))
        flame(m, p, rng.uniform(0.35, 0.75), (rng.uniform(-0.15, 0.15), rng.uniform(-0.15, 0.15), 1.0), width=0.45)
        if k % 2 == 0:
            card(f'{name}Halo{k}', p + Vector((0.0, 0.0, 0.3)), 0.9, halo)
    done(m, name, fire, shadow=False)


def clod(name, at, r, seed):
    """D: a clod of grave earth with stones, a coffin plank and a bone in it."""
    rng = random.Random(seed)
    blobs(name, [(at, (r, r * 0.85, r * 0.8), None, seed, 0.3)], textured('GroundDirt', 'GMF_Dirt'), box='GroundDirt',
          subdiv=3)
    stones = [(Vector(at) + Vector((rng.uniform(-1, 1), rng.uniform(-1, 1), rng.uniform(-1, 1))).normalized() * r * 0.85,
               (r * 0.22, r * 0.18, r * 0.15), None, k, 0.3) for k in range(5)]
    blobs(name + 'Stones', stones, textured('RockGranite', 'GMF_Granite', tint=0xb8b4ac), box='RockGranite', subdiv=1,
          smooth=False)
    bm = bmesh.new()
    bmesh.ops.create_cube(bm, size=1.0)
    bmesh.ops.transform(bm, matrix=T(Vector(at) + Vector((r * 0.5, 0.0, r * 0.45))) @ Matrix.Rotation(0.7, 4, 'Y')
                        @ Matrix.Rotation(0.4, 4, 'Z') @ Matrix.Diagonal((r * 1.6, r * 0.32, r * 0.08, 1.0)), verts=bm.verts)
    o = finish_bm(name + 'Plank', bm, textured('WoodPlanks', 'GMF_Coffin', tint=0x8a7a68), smooth=False)
    bpy.context.view_layer.update()
    lt.box_uv(o, 'WoodPlanks')
    bone = mat_surface('GMF_BoneChip', 0xd9d1bf, rough=0.6)
    tube(name + 'Bone', [Vector(at) + Vector((-r * 0.9, -r * 0.3, r * 0.3)), Vector(at) + Vector((-r * 0.2, -r * 0.6, r * 0.75))],
         [r * 0.09, r * 0.07], bone, sides=8)


def clod_impact(name, at, seed):
    """D: where a clod lands: a dust burst, debris in the air, and the rubble pile it leaves."""
    rng = random.Random(seed)
    dirt = textured('GroundDirt', 'GMF_Dirt')
    pile = [(Vector(at) + Vector((rng.uniform(-0.6, 0.6), rng.uniform(-0.6, 0.6), 0.05)),
             (rng.uniform(0.2, 0.45),) * 2 + (rng.uniform(0.12, 0.25),), None, k, 0.35) for k in range(7)]
    blobs(name + 'Pile', pile, dirt, box='GroundDirt')
    chunks(name + 'Debris', [Vector(at) + Vector((math.cos(a) * d, math.sin(a) * d, 0.0)) for a, d in
                             ((rng.uniform(0, 6.3), rng.uniform(0.6, 1.6)) for _ in range(14))], dirt, seed, size=0.09,
           lift=0.3, airborne=1.4)
    dust(name + 'Dust', at, 1.3, 10, 1.4, mat_dust('GMF_DustThick', 0xcdb68e, 0.9), seed, size=(0.8, 1.5))


TELL_CAM = {'A': ((-6.6, -9.2, 4.8), (-5.4, 0.3, 0.3), 24.0), 'B': ((-6.6, -9.2, 4.8), (-5.4, 0.3, 0.3), 24.0),
            'C': ((-5.4, -8.8, 3.6), (2.6, 0.4, 1.5), 26.0), 'D': ((-6.6, -9.2, 4.8), (-5.4, 0.3, 0.3), 24.0)}


def shot_tell(sheet, sun, rim):
    light('golden', sun, rim)
    fig_at = (-14.0, 0.6)
    figure(fig_at, (0.0, 0.0))
    start, end = Vector((-4.6, 0.0, 0.0)), Vector((-13.8, 0.5, 0.0))
    marks = []
    if sheet == 'A':
        rec = gravemother('A', RAKE, (-1.0, 0.0), (-14.0, 0.0))
        grown = start.lerp(end, 0.78)
        crack('Line', start, grown, 0.34, 21, lip_mat=chalk())
        rake = foot(rec, '0_l')
        dust('Rake', Vector((rake.x - 0.4, rake.y, 0.0)), 0.8, 6, 0.6, dust_mat(), 2)
        for i in range(3):
            q = start.lerp(grown, 0.3 + 0.3 * i)
            dust(f'Spurt{i}', q, 0.4, 2, 0.4, dust_mat(), 30 + i)
        marks = [('line', grown)]
    elif sheet == 'B':
        rec = gravemother('B', RAKE, (-1.0, 0.0), (-14.0, 0.0), lean_to=Vector((fig_at[0], fig_at[1], 1.0)))
        fire, halo = mat_emit('GMF_ColdFlame', 0xd4ec96, 7.0), mat_glow('GMF_WispHalo', 0xe3eea6, 3.5)
        m = wm()
        pts = jag(start, end, 4, 0.2, step=0.75)
        for k, p in enumerate(pts[:-3]):
            h = 0.45 * (1.0 - 0.6 * k / len(pts))
            flame(m, p, h, (0.0, 0.0, 1.0), width=0.5)
            card(f'Halo{k}', p + Vector((0.0, 0.0, h * 0.4)), 0.8, halo)
        done(m, 'PathFlames', fire, shadow=False)
        marks = [('line', pts[len(pts) - 4])]
    elif sheet == 'C':
        anchors = [(7.7, 2.4, 3.1), (7.8, -2.2, 3.4), (7.9, 0.2, 4.4)]
        rec = gravemother('C', SLING, (2.4, 0.0), (-14.0, 0.0), anchors=anchors)
        dust('Brace', foot(rec, '0_l'), 0.6, 4, 0.4, dust_mat(), 6)
        dust('Brace2', foot(rec, '0_r'), 0.6, 4, 0.4, dust_mat(), 7)
        marks = [('line', Vector(anchors[2]).lerp(cpt(rec, 'abdomen', SPINNERETS), 0.5))]
    else:
        rec = gravemother('D', DIG, (-1.0, 0.0), (-14.0, 0.0))
        pts = jag(start, end, 8, 0.25, step=0.3)
        n = len(pts)
        upto = int(n * 0.8)
        ridge('Heave', pts[:upto], [0.85 - 0.3 * k / upto for k in range(upto)],
              [0.55 * (1.0 - (k / upto) ** 2) + 0.08 for k in range(upto)], textured('GroundDirt', 'GMF_Dirt'), 3)
        crack('HeaveCrack', pts[0], pts[upto - 1], 0.22, 9, crest=lambda t: 0.55 * (1.0 - t * t) + 0.12)
        for i in range(4):
            dust(f'Jet{i}', pts[int(upto * (0.2 + 0.2 * i))], 0.5, 3, 0.9, dust_mat(), 40 + i)
        for s in ('0_l', '0_r'):
            dust(f'Dig{s}', foot(rec, s), 0.8, 5, 0.7, mat_dust('GMF_DustThick', 0xcdb68e, 0.9), 8)
        marks = [('line', pts[upto - 1])]
    cam, target, lens = TELL_CAM[sheet]
    shoot(sheet, 'tell', cam, target, lens, PANEL_SIZE, 48, marks)


CHARGE_CAM, CHARGE_TARGET = (-11.6, -9.4, 4.4), (-5.0, 0.2, 0.5)


def shot_charge(sheet, sun, rim):
    light('golden', sun, rim)
    figure((-14.6, 3.0), (-8.0, 0.0))
    at, start = (-8.2, 0.2), Vector((-0.6, 0.0, 0.0))
    behind = Vector((at[0] + 1.6, at[1], 0.0))
    rec = gravemother(sheet, CHARGE, at, (-20.0, 0.4), anchors=None)
    for s in ('0_l', '1_r', '2_l', '3_l'):
        dust(f'Kick{s}', foot(rec, s), 0.6, 3, 0.5, dust_mat(), 17 + int(s[0]))
    if sheet == 'A':
        pts = crack('Plough', start, behind, 0.4, 5, lip_mat=chalk())
        bone = mat_surface('GMF_BoneChip', 0xd9d1bf, rough=0.6)
        rng = random.Random(3)
        chips = [(Vector((rng.uniform(behind.x, start.x), rng.uniform(-1.2, 1.2), rng.uniform(0.02, 1.2))),
                  (0.09, 0.025, 0.025), Euler((0, rng.uniform(0, 3), rng.uniform(0, 6))).to_matrix(), k, 0.05)
                 for k in range(16)]
        blobs('BoneChips', chips, bone)
        dust_line('Wake', pts, 1.0, dust_mat(), 3, every=4, height=0.9)
    elif sheet == 'B':
        pts = crack('Burn', start, behind, 0.3, 6, glow_mat=mat_emit('GMF_CrackGlow', 0xeef3b5, 9.0))
        fire, halo = mat_emit('GMF_ColdFlame', 0xd4ec96, 7.0), mat_glow('GMF_WispHalo', 0xe3eea6, 3.5)
        m = wm()
        for k in range(2, len(pts) - 2, 4):
            flame(m, pts[k], 0.25 + 0.2 * random.Random(k).random(), (0.0, 0.0, 1.0), width=0.5)
            card(f'Halo{k}', pts[k] + Vector((0.0, 0.0, 0.2)), 0.7, halo)
        done(m, 'Gutter', fire, shadow=False)
        dust_line('Wake', pts, 0.8, dust_mat(), 3, every=5, height=0.7)
    elif sheet == 'C':
        anchors = [(7.7, 2.4, 3.1), (7.8, -2.2, 3.4)]
        s = cpt(rec, 'abdomen', SPINNERETS)
        strand = mat_surface('GMF_Strand', 0xf2efe8, rough=0.35, glow=0.25, glow_color=0xffffff)
        tubes('Trailing', [([s, s.lerp(Vector(a), 0.5) - Vector((0, 0, 0.5)), Vector(a)], [0.018] * 3) for a in anchors],
              strand, sides=5)
        pts = crack('Drag', start, behind, 0.24, 7)
        rng = random.Random(4)
        trip = []
        for k in range(5):
            x = behind.x + 1.2 + k * 1.3
            p0 = Vector((x + rng.uniform(-0.3, 0.3), -1.6 - rng.uniform(0, 0.6), 0.0))
            p1 = Vector((x + rng.uniform(-0.6, 0.6), 1.6 + rng.uniform(0, 0.6), 0.0))
            p0.z, p1.z = rng.uniform(0.1, 0.25), rng.uniform(0.1, 0.25)
            trip.append(([p0, p0.lerp(p1, 0.5) - Vector((0, 0, 0.05)), p1], [0.024] * 3))
        tubes('TripLines', trip, strand, sides=4)
        dust_line('Wake', pts, 0.9, dust_mat(), 3, every=5, height=0.6)
    else:
        pts = jag(start, behind, 12, 0.15, step=0.3)
        n = len(pts)
        dirt = textured('GroundDirt', 'GMF_Dirt')
        for sgn in (1, -1):
            side_pts = [p + Vector((0.0, sgn * 1.25, 0.0)) for p in pts]
            ridge(f'Berm{sgn}', side_pts, [0.85] * n, [0.85 * math.sin(math.pi * min(k / (n - 1) * 1.6, 1.0)) ** 0.5
                                                      for k in range(n)], dirt, 5 + sgn)
        crack('FurrowCrack', start, behind, 0.18, 13)
        chunks('Thrown', [p + Vector((random.Random(k).uniform(-2, 2), random.Random(k + 1).uniform(-2.2, 2.2), 0.0))
                          for k, p in enumerate(pts[::2])], dirt, 8, size=0.14, lift=0.2, airborne=1.0)
        dust_line('Wake', pts, 1.2, mat_dust('GMF_DustThick', 0xcdb68e, 0.9), 3, every=4, height=1.2)
    shoot(sheet, 'charge', CHARGE_CAM, CHARGE_TARGET, 28.0, PANEL_SIZE, 48, [('head', cpt(rec, 'head', HEAD_C))])


FX_COLS, FX_ROWS = (-4.4, -1.5, 1.5, 4.4), (1.9, -1.4)


def shot_fx(sheet, sun, rim):
    """The FX parts laid out on the floor, four by two, labelled in the sheet (the marks)."""
    shade = sheet == 'B'
    light('shade' if shade else 'golden', sun, rim)
    O = Vector((60.0, 0.0, 0.0)) if shade else Vector((0.0, -150.0, 0.0))
    cells = [O + Vector((x, y, 0.0)) for y in FX_ROWS for x in FX_COLS]
    marks = []
    builders = FX_PARTS[sheet]
    for i, (key, build) in enumerate(builders):
        build(cells[i], i)
        marks.append((key, cells[i] + Vector((0.0, -0.95, 0.0))))
    shoot(sheet, 'fx', O + Vector((0.0, -8.8, 5.6)), O + Vector((0.0, 0.1, 0.3)), 30.0, PANEL_SIZE, 48, marks,
          exposure=0.4 if shade else 0.0)


def _milk():
    return (mat_surface('GMF_Milk', 0xe6e2cf, rough=0.12, coat=1.0, glow=0.35, glow_color=0xd9e59a, sss=0.3),
            mat_surface('GMF_MilkPool', 0xd8d4c0, rough=0.08, coat=1.0, glow=0.5, glow_color=0xd9e59a))


def _strand():
    return mat_surface('GMF_Strand', 0xf2efe8, rough=0.35, glow=0.25, glow_color=0xffffff)


def _wisp_mats():
    return mat_emit('GMF_WispCore', 0xfdfbe8, 18.0), mat_glow('GMF_WispHalo', 0xe3eea6, 3.5), \
        mat_trail('GMF_WispTrail', 0xe3eea6, 5.0)


FX_PARTS = {
    'A': [
        ('glob', lambda p, i: glob('FxGlob', p + Vector((0.4, 0.0, 0.9)), 0.24, (1.0, 0.0, -0.3), _milk()[0], i, drops=5)),
        ('trail', lambda p, i: tube('FxTrail', arc(p + Vector((-1.0, 0.0, 0.4)), p + Vector((1.0, 0.0, 0.4)), 0.8),
                                    [0.02 + 0.13 * (k / 24) ** 1.5 for k in range(25)],
                                    mat_trail('GMF_MilkTrail', 0xe8edc0, 2.5), sides=10, shadow=False)),
        ('splash', lambda p, i: splash('FxSplash', p, 0.6, _milk()[0], i, decal=_milk()[1])),
        ('puddle', lambda p, i: puddle('FxPuddle', p, 0.85, _milk()[1], i)),
        ('dust', lambda p, i: dust('FxDust', p + Vector((0.0, 0.0, 0.3)), 0.8, 14, 1.0, mat_dust('GMF_DustPuff', 0xe6dcc6, 1.0, power=1.0), i)),
        ('crack', lambda p, i: crack('FxCrack', p + Vector((-1.1, -0.4, 0.0)), p + Vector((1.1, 0.4, 0.0)), 0.26, i,
                                     branches=2, lip_mat=chalk())),
        ('chips', lambda p, i: blobs('FxChips', [(p + Vector((random.Random(k).uniform(-0.6, 0.6),
                                                              random.Random(k + 9).uniform(-0.5, 0.5),
                                                              random.Random(k + 5).uniform(0.05, 0.9))),
                                                  (0.11, 0.03, 0.03), Euler((0, k, k * 2.1)).to_matrix(), k, 0.05)
                                                 for k in range(12)], mat_surface('GMF_BoneChip', 0xd9d1bf, rough=0.6))),
        ('ling', lambda p, i: spider('FxLing', STAND, (p.x, p.y), (p.x - 3.0, p.y - 6.0), LING)),
    ],
    'B': [
        ('wisp', lambda p, i: wisp('FxWisp', p + Vector((0.0, 0.0, 0.9)), 0.18, *_wisp_mats()[:2])),
        ('trail', lambda p, i: wisp('FxWispT', p + Vector((0.9, 0.0, 0.8)), 0.14, *_wisp_mats()[:2],
                                    trail=arc(p + Vector((-1.1, 0.0, 0.3)), p + Vector((0.9, 0.0, 0.8)), 0.4, bend=0.3),
                                    trail_mat=_wisp_mats()[2])),
        ('flame', lambda p, i: (lambda m: (flame(m, p, 0.9, width=0.42), done(m, 'FxFlame', mat_emit('GMF_ColdFlame', 0xd4ec96, 7.0),
                                                                             shadow=False),
                                           card('FxFlameHalo', p + Vector((0, 0, 0.4)), 1.4, _wisp_mats()[1])))(wm())),
        ('ring', lambda p, i: flame_ring('FxRing', p, 0.8, i, count=9)),
        ('scorch', lambda p, i: fan('FxScorch', p, splat_radius(0.9, i, 8, 0.4),
                                    mat_surface('GMF_Scorch', 0x5c5249, rough=0.95), z=0.011)),
        ('crack', lambda p, i: crack('FxCrack', p + Vector((-1.1, -0.4, 0.0)), p + Vector((1.1, 0.4, 0.0)), 0.26, i,
                                     branches=2, glow_mat=mat_emit('GMF_CrackGlow', 0xeef3b5, 9.0))),
        ('candle', lambda p, i: fx_candle(p)),
        ('smoke', lambda p, i: [card(f'FxSmoke{k}', p + Vector((0.08 * k, 0.0, 0.3 + 0.32 * k)), 0.5 + 0.2 * k,
                                     mat_dust('GMF_Smoke', 0xc4c0b8, 0.9, power=1.1)) for k in range(5)]),
    ],
    'C': [
        ('wad', lambda p, i: silk_wad('FxWad', p + Vector((0.0, 0.0, 0.8)), 0.3, (1.0, 0.0, 0.0),
                                      mat_surface('GMF_SilkBall', 0xe9e4d8, rough=0.6, sheen=0.8, sss=0.4), _strand(), i)),
        ('net', lambda p, i: web('FxNet', p + Vector((0.0, 0.0, 0.9)), (0.3, -1.0, 0.25), 0.95, _strand(), i, spokes=11,
                                 rings=4, domed=-0.4, strand=0.014)),
        ('patch', lambda p, i: web_patch('FxPatch', p, 1.0, _strand(), i)),
        ('strand', lambda p, i: (tubes('FxStrand', [([p + Vector((-1.1, 0.0, 0.15)), p + Vector((0.0, 0.0, 1.0)),
                                                       p + Vector((1.1, 0.0, 1.85))], [0.02] * 3)], _strand()),
                                 card('FxGlint', p + Vector((0.3, 0.0, 1.24)), 0.4, mat_glow('GMF_Glint', 0xffffff, 6.0, 3.0),
                                      roll_angle=0.6))),
        ('sac', lambda p, i: egg_sac('FxSac', p + Vector((0.0, 0.0, 0.62)), 1.1,
                                     mat_surface('GMF_Sac', 0xe4ddcb, rough=0.5, sss=0.5, sheen=0.5), _strand(), i)),
        ('tatters', lambda p, i: [ribbon(f'FxTatter{k}', [p + Vector((-1.0 + 0.22 * j, -0.5 + 0.45 * k + 0.12 * math.sin(j + k),
                                                                     0.03 + 0.06 * max(0.0, math.sin(j * 1.3 + k))))
                                                         for j in range(10)], [0.34] * 10, mat_silk('GMF_Silk'))
                                  for k in range(3)]),
        ('splash', lambda p, i: splash('FxBlood', p, 0.55, mat_surface('GMF_Oxblood', 0x6e1e12, rough=0.15, coat=1.0), i,
                                       decal=mat_surface('GMF_OxPool', 0x5e170e, rough=0.1, coat=1.0))),
        ('dust', lambda p, i: dust('FxDust', p + Vector((0.0, 0.0, 0.3)), 0.8, 14, 1.0, mat_dust('GMF_DustPuff', 0xe6dcc6, 1.0, power=1.0), i)),
    ],
    'D': [
        ('clod', lambda p, i: clod('FxClod', p + Vector((0.0, 0.0, 0.8)), 0.5, i)),
        ('burst', lambda p, i: dust('FxBurst', p, 1.0, 10, 1.3, mat_dust('GMF_DustThick', 0xcdb68e, 0.9), i,
                                    size=(0.8, 1.5))),
        ('debris', lambda p, i: chunks('FxDebris', [p + Vector((math.cos(k) * 0.7, math.sin(k * 1.3) * 0.5, 0.0))
                                                    for k in range(12)], textured('GroundDirt', 'GMF_Dirt'), i,
                                       size=0.1, lift=0.2, airborne=1.2)),
        ('rubble', lambda p, i: blobs('FxRubble', [(p + Vector((math.cos(k * 1.9) * 0.45, math.sin(k * 1.9) * 0.4, 0.05)),
                                                    (0.32, 0.3, 0.2), None, k, 0.35) for k in range(6)],
                                      textured('GroundDirt', 'GMF_Dirt'), box='GroundDirt')),
        ('heave', lambda p, i: fx_heave(p, i)),
        ('berm', lambda p, i: fx_berm(p, i)),
        ('roots', lambda p, i: tubes('FxRoots', [([p + Vector((math.cos(k) * 0.2, math.sin(k) * 0.2, 0.0)),
                                                   p + Vector((math.cos(k) * 0.6, math.sin(k) * 0.6, 0.5)),
                                                   p + Vector((math.cos(k + 0.6) * 0.9, math.sin(k + 0.6) * 0.9, 0.9))],
                                                  [0.06, 0.045, 0.02]) for k in range(5)],
                                     textured('BarkOak', 'GMF_Roots', tint=0x9a8670), sides=7)),
        ('mound', lambda p, i: spawn_mound('FxMound', p, i, with_ling=False)),
    ],
}


def fx_heave(p, i):
    pts = jag(p + Vector((-1.2, 0.0, 0.0)), p + Vector((1.2, 0.0, 0.0)), i, 0.1, 0.2)
    n = len(pts)
    ridge('FxHeave', pts, [0.6] * n, [0.3 * (1 - k / (n - 1)) + 0.04 for k in range(n)],
          textured('GroundDirt', 'GMF_Dirt'), i)
    crack('FxHeaveCrack', pts[0], pts[-1], 0.16, i, branches=1, crest=lambda t: 0.3 * (1 - t) + 0.08)


def fx_berm(p, i):
    pts = jag(p + Vector((-1.2, 0.2, 0.0)), p + Vector((1.2, 0.2, 0.0)), i, 0.1, 0.2)
    ridge('FxBerm', pts, [0.7] * len(pts), [0.55] * len(pts), textured('GroundDirt', 'GMF_Dirt'), i)


def fx_candle(p):
    wax, wick = mat_surface('GMF_Wax', 0xe6dcc4, rough=0.42, sss=0.6), mat_surface('GMF_Wick', 0x1b1612, rough=0.8)
    m = wm()
    top = p + Vector((0.0, 0.0, 0.7))
    lc.tube(m, [p, top], [0.13, 0.12], 'fx', sides=16)
    lc.ellipsoid(m, p + Vector((0.0, 0.0, 0.02)), (0.05, 0.32, 0.3), 'fx', fwd=(0, 0, 1), segs=16, rings=8)
    for j in range(3):
        a = j * 2.1
        s = top + Vector((math.cos(a) * 0.12, math.sin(a) * 0.12, 0.0))
        lc.tube(m, [s, s - Vector((0, 0, 0.15 + 0.08 * j))], [0.035, 0.045], 'fx', sides=8)
    done(m, 'FxCandle', wax)
    f = wm()
    flame(f, top + Vector((0.0, 0.0, 0.04)), 0.32)
    done(f, 'FxCandleFlame', mat_emit('GMF_Flame', 0xfff6d8, 14.0), shadow=False)
    card('FxCandleHalo', top + Vector((0.0, 0.0, 0.18)), 0.9, mat_glow('GMF_FlameHalo', 0xe9f0b0, 3.0))


def spawn_mound(name, at, seed, with_ling=True, toward=(-14.0, 0.0)):
    """D: a mound of earth bulging as a spiderling digs out of it."""
    dirt = textured('GroundDirt', 'GMF_Dirt')
    blobs(name, [(at, (0.9, 0.85, 0.32), None, seed, 0.25)], dirt, box='GroundDirt', subdiv=3)
    hole = mat_surface('GMF_Hole', 0x15100c, rough=1.0)
    fan(name + 'Hole', Vector(at) + Vector((0.0, 0.0, 0.3)), splat_radius(0.32, seed, 6, 0.5), hole, z=0.0)
    rng = random.Random(seed)
    chunks(name + 'Spray', [Vector(at) + Vector((rng.uniform(-0.9, 0.9), rng.uniform(-0.9, 0.9), 0.0)) for _ in range(9)],
           dirt, seed, size=0.08, lift=0.4, airborne=1.0)
    dust(name + 'Dust', at, 0.8, 4, 0.8, dust_mat(), seed)
    if with_ling:
        p = Vector(at)
        spider(name + 'Ling', Pose(pitch=28, lift=10, reach={'0_l': (60, 40, 70), '0_r': (60, 40, 70)}),
               (p.x, p.y, 0.05), toward, LING)


SPECIAL_CAM = {'A': ((-9.6, -4.4, 1.1), (-1.6, 0.4, 2.0), 30.0), 'B': ((-7.6, 4.2, 4.0), (2.4, 0.2, 1.4), 30.0),
               'C': ((-9.0, -0.2, 2.1), (3.0, -1.2, 2.0), 30.0), 'D': ((4.2, -8.2, 3.9), (-1.6, 0.4, 1.0), 28.0)}


def shot_special(sheet, sun, rim):
    marks = []
    if sheet == 'A':
        light('golden', sun, rim)
        rec = gravemother('A', SHRIEK, HERO_AT, (-12.0, -4.0), phase=2)
        abd = cpt(rec, 'abdomen', ABD_C)
        left, fwd = cdir(rec, 'body', (0.0, 1.0, 0.0)), cdir(rec, 'body', (1.0, 0.0, 0.0))
        left, fwd = Vector((left.x, left.y, 0.0)).normalized(), Vector((fwd.x, fwd.y, 0.0)).normalized()
        rng = random.Random(2)
        # The brood shaken off her back: four spiderlings in the air beside her, legs flung out.
        for i, (sgn, along, h) in enumerate(((1, 0.3, 1.9), (-1, -0.4, 1.2), (1, -0.9, 0.7), (-1, 0.7, 2.3))):
            P = abd + left * sgn * 2.0 + fwd * along
            P.z = h
            R = Euler((rng.uniform(-0.7, 0.7), rng.uniform(-0.7, 0.7), rng.uniform(0, 6.3))).to_matrix().to_4x4()
            spider(f'Falling{i}', DROP, mat=SPIDER['MATERIAL'], W=T(P) @ R @ Matrix.Scale(LING, 4))
        for i, at in enumerate(((-4.6, -3.0), (-5.6, 2.6))):
            ling(f'Running{i}', RUN, at, (-14.0, -6.0 + 9 * i))
            dust(f'RunDust{i}', Vector((at[0] + 0.5, at[1], 0.0)), 0.5, 3, 0.4, dust_mat(), 60 + i)
        dust('ShakeDust', Vector((HERO_AT[0], HERO_AT[1], 0.0)), 2.6, 12, 0.9, dust_mat(), 7, size=(0.5, 0.9))
        marks = crit_marks(rec, 'A')
    elif sheet == 'B':
        light('shade', sun, rim)
        rec = gravemother('B', SHRIEK, (2.6, 0.2), (-12.0, 2.0), phase=2)
        eyes = ling_eyes()
        for i, (at, tw) in enumerate((((5.6, -1.6), (-10, -6)), ((6.4, 1.8), (-10, 7)), ((-1.6, -3.4), (-12, -8)),
                                      ((-2.4, 3.0), (-12, 7)))):
            ling(f'Pour{i}', RUN, at, tw, mat=eyes)
        marks = [('gutter', rec['extra']['flames'][0])]
    elif sheet == 'C':
        light('golden', sun, rim)
        rec = gravemother('C', SHRIEK, (1.0, -4.6), (-10.0, -8.0), anchors=[(7.8, 2.6, 3.4), (7.6, -2.6, 3.8)])
        strand = _strand()
        sac_mat = mat_surface('GMF_Sac', 0xe4ddcb, rough=0.5, sss=0.5, sheen=0.5)
        web('DenWebL', (7.7, 2.8, 3.3), (-1.0, 0.0, 0.0), 1.7, strand, 3, sag=0.2)
        web('DenWebR', (7.7, -2.9, 3.0), (-1.0, 0.0, 0.0), 1.4, strand, 4, sag=0.2)
        sacs = [((6.7, 2.6, 2.5), (7.6, 3.0, 4.5)), ((6.6, -2.3, 2.1), (7.5, -2.8, 4.3)), ((7.0, 0.7, 3.0), (7.8, 1.0, 4.6)),
                ((6.4, -0.8, 1.2), (7.5, -1.0, 4.5))]
        for i, (at, top) in enumerate(sacs):
            egg_sac(f'Sac{i}', at, 1.25 if i != 2 else 1.05, sac_mat, strand, 20 + i, hang_to=top)
        egg_sac('SacTorn', (4.6, 3.2, 0.52), 1.15, sac_mat, strand, 31, torn=True)
        splash('Hatch', Vector((4.6, 3.2, 0.0)), 0.7, mat_surface('GMF_SacFluid', 0xcfc7a8, rough=0.1, coat=1.0), 5,
               decal=mat_surface('GMF_SacPool', 0xbdb594, rough=0.1, coat=1.0))
        ling('Hatchling', Pose(pitch=-10, lift=-6), (3.4, 2.2), (-10.0, -2.0))
        marks = [('sacs', Vector(sacs[0][0])), ('sacs2', Vector(sacs[1][0]))]
    else:
        light('golden', sun, rim)
        rec = gravemother('D', SHRIEK, (-1.4, 0.6), (-12.0, -3.0), phase=3)
        granite = textured('RockGranite', 'GMF_Granite', tint=0xb8b4ac)
        # The headstone, cracked off her back in two.
        for i, (at, rot, lo, hi) in enumerate((((2.0, -0.4, 0.2), (1.25, 0.0, 1.9), 0.0, 52.0),
                                              ((0.4, -2.2, 0.16), (1.5708, 0.0, 0.9), 52.0, STONE_H))):
            m = wm()
            prof_h = hi - lo
            headstone_mesh(m, Vector((0.0, 0.0, -lo)), STONE_W, STONE_H, STONE_T, Matrix.Identity(3))
            bm = m.bm
            geom = bm.verts[:] + bm.edges[:] + bm.faces[:]
            bmesh.ops.bisect_plane(bm, geom=geom, plane_co=(0, 0, 0), plane_no=(0.15, 0.0, 1.0), clear_inner=True)
            geom = bm.verts[:] + bm.edges[:] + bm.faces[:]
            bmesh.ops.bisect_plane(bm, geom=geom, plane_co=(0, 0, prof_h), plane_no=(0.15, 0.0, 1.0), clear_outer=True)
            bmesh.ops.holes_fill(bm, edges=[e for e in bm.edges if e.is_boundary])
            bmesh.ops.transform(bm, matrix=Matrix.Scale(0.01 * SCALE, 4), verts=bm.verts)
            bmesh.ops.transform(bm, matrix=T(at) @ Euler(rot).to_matrix().to_4x4(), verts=bm.verts)
            done(m, f'Broken{i}', granite, smooth=False, box='RockGranite')
        dust('Fall', Vector((1.2, -1.4, 0.0)), 1.2, 6, 0.8, mat_dust('GMF_DustThick', 0xcdb68e, 0.9), 4)
        for i, at in enumerate(((-5.0, -1.6), (-4.6, 3.6), (-2.6, -4.4))):
            spawn_mound(f'Mound{i}', (at[0], at[1], 0.0), 50 + i, toward=(-14.0, at[1]))
        marks = [('bare', cpt(rec, 'abdomen', ABD_C + Vector((-10.0, 0.0, 36.0))))]
    cam, target, lens = SPECIAL_CAM[sheet]
    shoot(sheet, 'special', cam, target, lens, PANEL_SIZE, 48, marks, exposure=0.5 if sheet == 'B' else 0.0)


def shot_poses(sun, rim):
    """A test strip: every pose on the bare spider, for checking the solver."""
    light('golden', sun, rim)
    poses = [('stand', STAND), ('threat', THREAT), ('shriek', SHRIEK), ('charge', CHARGE), ('rake', RAKE),
             ('spit', SPIT), ('clod', CLOD), ('dig', DIG), ('sling', SLING)]
    for i, (name, pose) in enumerate(poses):
        spider(f'P_{name}', pose, (-40.0 + i * 7.0, -60.0), (-40.0 + i * 7.0 - 8.0, -80.0), 1.0, bone_hide())
    shoot('poses', 'strip', (-12.0, -82.0, 3.0), (-12.0, -60.0, 0.7), 24.0, (2400, 700), 24)


SHOTS = {'hero': shot_hero, 'spit': shot_spit, 'tell': shot_tell, 'charge': shot_charge, 'fx': shot_fx,
         'special': shot_special}


# --- The sheet's words (the assembly reads them from GMF_<X>.json) ---

SHEETS = {
    'A': dict(
        name='Bone Mother', file='Gravemother_Fight_A_BoneMother.png',
        pitch='Your Bone pick, made a mother: she carries her brood on her back and feeds you to them.',
        hero='The Bone hide as the game has it; four spiderlings ride her abdomen; front legs up in the threat display '
             'that opens the fight.',
        marks={'head': ('HEAD', 'crit: the eyes'), 'abdomen': ('ABDOMEN', 'crit: the ghost heart'),
               'brood': ('BROOD', 'the next 4 spiderlings'), 'tell': ('TELL', 'globs swell in her fangs'),
               'line': ('THE LINE', 'the crack she will run')},
        panels={
            'spit': ('Milk Spit  |  projectile', [
                ('Does', 'A fan of 3 milky venom globs. Each leaves a chalk-white puddle that slows and burns for 4 s.'),
                ('Tell', 'Head rears back, fangs spread, the globs swell between them (1 s).'),
                ('Answer', 'Strafe out of the fan. A head crit in the tell chokes the spit.')]),
            'tell': ('Grave Plough  |  the tell (2 s)', [
                ('Does', 'She rakes the floor twice; a chalk crack runs from her feet toward you: the line she will run.'),
                ('Tell', 'Raking legs, a dust spurt each stroke, the crack growing at you.'),
                ('Answer', 'Step off the line. Never stand between her and a wall.')]),
            'charge': ('Grave Plough  |  the charge', [
                ('Does', '25 m at a run along the crack, ploughing a furrow and throwing dirt and bone. Heavy hit, '
                         'knockdown.'),
                ('Tell', 'Only the crack: she never leaves its line.'),
                ('Answer', 'Sidestep. She skids and stalls 2 s at the end: shoot the abdomen.')]),
            'fx': ('FX parts', [
                ('Meshes', 'glob and droplets, splash crown, bone chips, crack chunks, spiderling (0.45x).'),
                ('Cards', 'dust puffs (lit, soft).'),
                ('Decals', 'puddle and splat, the chalk crack.')]),
            'special': ('Shed the Brood  |  66% and 33%', [
                ('Does', 'She rears and shakes; the 4 riders drop and scatter. Before 33%, 4 more climb from the den '
                         'onto her back.'),
                ('Tell', 'A shriek and a rear-up (1.5 s).'),
                ('Answer', 'Kill the climbers before they reach her. A bare back means the brood is spent.')])},
        fx_labels={'glob': 'Milk glob', 'trail': 'Glob trail', 'splash': 'Splash crown', 'puddle': 'Puddle decal',
                   'dust': 'Dust puff', 'crack': 'Chalk crack', 'chips': 'Bone chips', 'ling': 'Spiderling 0.45x'},
        key=[('BAB2A1', 'Bone hide'), ('524D46', 'Ghost-lines'), ('E6E2CF', 'Milk venom'), ('D9E59A', 'Venom glow'),
             ('D6C4A0', 'Dust'), ('6B4A2E', 'Spiderlings')]),
    'B': dict(
        name='Vigil', file='Gravemother_Fight_B_Vigil.png',
        pitch='She hoards the chapel\'s candles in wax on her back; the ghost-light she has eaten burns through her '
              'Bone hide.',
        hero='Bone hide with the heart and chevrons lit from inside; nine candles in three clusters; candle-lit eyes.',
        marks={'head': ('HEAD', 'crit: lit eyes'), 'abdomen': ('HEART', 'abdomen crit, glowing'),
               'candles': ('CANDLES', '9 flames: her phase meter'), 'tell': ('TELL', 'flames lean at you'),
               'line': ('THE LINE', 'lit flames mark her path'), 'gutter': ('CANDLES', '6 lit, 3 guttered out')},
        panels={
            'spit': ('Corpse-Lights  |  projectile', [
                ('Does', 'Three wisps peel off her flames and drift at you in slow curving arcs; each bursts into a '
                         'ring of cold flame (3 s).'),
                ('Tell', 'Her flames lean toward you and flare white (1 s), like a keeper\'s lantern.'),
                ('Answer', 'Shoot wisps out of the air (one hit each) or outrun them. Never stand in a ring.')]),
            'tell': ('Procession  |  the tell (2 s)', [
                ('Does', 'Ghost-flames light one by one along the floor toward you: the path she will run.'),
                ('Tell', 'The lit line grows; her own flames lean along it.'),
                ('Answer', 'Get off the line early: it shows the whole run.')]),
            'charge': ('Procession  |  the charge', [
                ('Does', 'She runs the lit line; the floor cracks and burns with cold light for 4 s behind her.'),
                ('Tell', 'Only the lit line.'),
                ('Answer', 'Sidestep. While her candles relight (3 s) the heart is bare: shoot it.')]),
            'fx': ('FX parts', [
                ('Emissive meshes', 'wisp cores, flames, the crack\'s glowing seam.'),
                ('Cards', 'soft halos, smoke. Ribbons: wisp trails.'),
                ('Decals', 'scorch, the crack.')]),
            'special': ('Guttering  |  66% and 33%', [
                ('Does', 'Three candles gutter out each phase and 4 spiderlings with candle-lit eyes pour from the den. '
                         'Fewer candles, brighter heart: wisps come in fives.'),
                ('Tell', 'A shriek; the dying candles smoke.'),
                ('Answer', 'Count her flames: 9, 6, 3. Kill the brood, then the heart.')])},
        fx_labels={'wisp': 'Wisp core + halo', 'trail': 'Wisp trail', 'flame': 'Cold flame', 'ring': 'Flame ring',
                   'scorch': 'Scorch decal', 'crack': 'Glow crack', 'candle': 'Candle', 'smoke': 'Smoke cards'},
        key=[('BAB2A1', 'Bone hide'), ('EEF3B5', 'Ghost-light'), ('FFF6D8', 'Flame core'), ('E6DCC4', 'Wax'),
             ('5C5249', 'Scorch'), ('C4C0B8', 'Smoke')]),
    'C': dict(
        name='Shroud', file='Gravemother_Fight_C_Shroud.png',
        pitch='Ivory and oxblood, wrapped in the burial silk she spins: the den is her web and the Sink her larder.',
        hero='The Ivory hide with the oxblood hourglass; silk wrappings trailing from her back; drag-lines to the den.',
        marks={'head': ('HEAD', 'crit'), 'abdomen': ('HOURGLASS', 'abdomen crit'),
               'spinnerets': ('SPINNERETS', 'the silk comes from here'), 'tell': ('TELL', 'the hourglass turns to you'),
               'sacs': ('EGG SACS', 'shoot them before 66%'), 'sacs2': ('', ''),
               'line': ('DRAG-LINES', 'shoot them to cut the charge')},
        panels={
            'spit': ('Shroud Shot  |  projectile', [
                ('Does', 'A silk wad that opens into a net and lands as a 5 m sticky patch: slows 60%, roots 1 s on a '
                         'hit.'),
                ('Tell', 'She turns away; the hourglass faces you (1.2 s).'),
                ('Answer', 'Punish the turn: shoot the hourglass. Dodge sideways; shoot a patch\'s anchors to clear it.')]),
            'tell': ('Drag-Line  |  the tell (2 s)', [
                ('Does', 'She backs toward the den, anchors three drag-lines and pulls them taut like a sling.'),
                ('Tell', 'The lines glint and creak as they stretch.'),
                ('Answer', 'Shoot the anchor lines: each one shortens the charge; all three cancel it.')]),
            'charge': ('Drag-Line  |  the charge', [
                ('Does', 'She slings forward in a straight line, trailing silk. Trip-lines left on the floor slow you '
                         'for 6 s.'),
                ('Tell', 'Only the taut lines.'),
                ('Answer', 'Sidestep; cut or jump the trip-lines.')]),
            'fx': ('FX parts', [
                ('Meshes', 'silk wad, net, egg sac, strands (thin tubes).'),
                ('Cards', 'dust, silk tatters, strand glints.'),
                ('Decals', 'web patch, oxblood splash.')]),
            'special': ('Brood Sacs  |  66% and 33%', [
                ('Does', 'Egg sacs hang in the den\'s web (shootable props). At 66% and 33% she shrieks and every whole '
                         'sac hatches, up to 4 spiderlings.'),
                ('Tell', 'The sacs twitch and swell before each phase.'),
                ('Answer', 'Shoot the sacs first: every burst sac is one fewer spiderling.')])},
        fx_labels={'wad': 'Silk wad', 'net': 'Net (mid-air)', 'patch': 'Web patch', 'strand': 'Taut line',
                   'sac': 'Egg sac', 'tatters': 'Silk tatters', 'splash': 'Oxblood splash', 'dust': 'Dust puff'},
        key=[('C8BB9D', 'Ivory hide'), ('531B13', 'Oxblood'), ('EEEAE0', 'Silk'), ('FFFFFF', 'Glint'),
             ('D6C4A0', 'Dust'), ('E4DDCB', 'Egg sac')]),
    'D': dict(
        name='Barrow', file='Gravemother_Fight_D_Barrow.png',
        pitch='The bold one: she has lived under the churchyard so long it grew onto her. A headstone for a shell, '
              'roots and grave mud for armour.',
        hero='Mud-grey hide, a toppled headstone bound to her abdomen by roots, grave mud caked on her legs; low and '
             'heavy.',
        marks={'head': ('HEAD', 'crit'), 'abdomen': ('ABDOMEN', 'crit: the soft rear'),
               'headstone': ('HEADSTONE', 'armour: shots glance off'), 'tell': ('TELL', 'the clod in her grip'),
               'line': ('THE HEAVE', 'her path, splitting toward you'), 'bare': ('BARE ABDOMEN', 'all of it a crit after 33%')},
        panels={
            'spit': ('Grave Clod  |  projectile', [
                ('Does', 'She tears up a clod of grave earth (stones, bones, coffin wood) and lobs it. It bursts into '
                         'dust and leaves a rubble pile.'),
                ('Tell', 'Front legs dig in and lift the clod overhead (1.2 s).'),
                ('Answer', 'Shoot the clod in her grip: it bursts on her and stuns her.')]),
            'tell': ('Furrow  |  the tell (2 s)', [
                ('Does', 'She digs in; the earth heaves and splits in a line toward you.'),
                ('Tell', 'A bulging crack crawling across the floor, dust jetting from it.'),
                ('Answer', 'Step out of the heave line.')]),
            'charge': ('Furrow  |  the charge', [
                ('Does', 'Head low, she ploughs the line and throws up a berm of earth on both sides.'),
                ('Tell', 'Only the heave line.'),
                ('Answer', 'The berms stay 20 s: cover against her clods.')]),
            'fx': ('FX parts', [
                ('Meshes', 'clod, rubble, debris, berms, roots, mounds.'),
                ('Cards', 'dust bursts.'),
                ('Decals', 'the heave crack, scuffs.')]),
            'special': ('Unearthed  |  66% and 33%', [
                ('Does', 'She slams the floor and 4 spiderlings dig out of mounds. At 33% the headstone cracks off: her '
                         'pale Bone abdomen is bare and she runs faster.'),
                ('Tell', 'A slam; the mounds bulge (1 s).'),
                ('Answer', 'Shoot the mounds as they bulge. After 33% the whole abdomen is a crit.')])},
        fx_labels={'clod': 'Grave clod', 'burst': 'Dust burst', 'debris': 'Debris', 'rubble': 'Rubble pile',
                   'heave': 'Heave crack', 'berm': 'Berm', 'roots': 'Roots', 'mound': 'Spawn mound'},
        key=[('8C8070', 'Mud-grey hide'), ('C9B392', 'Grave mud'), ('8E8B86', 'Granite'), ('5B4330', 'Roots'),
             ('CDB68E', 'Dust'), ('BAB2A1', 'Bare bone')]),
}


def write_sheet(sheet):
    path = os.path.join(OUT, f'GMF_{sheet}.json')
    data = {}
    if os.path.exists(path):
        with open(path, encoding='utf-8') as f:
            data = json.load(f)
    shots = data.get('shots', {})
    shots.update(RESULTS.get(sheet, {}))
    data = dict(SHEETS[sheet], letter=sheet, shots=shots)
    with open(path, 'w', encoding='utf-8') as f:
        json.dump(data, f, indent=1)
    log('wrote', path)


def main():
    global FIG
    os.makedirs(OUT, exist_ok=True)
    os.makedirs(MAP_DIR, exist_ok=True)
    t0 = time.time()
    FIG = G['mannequin']()
    sun, rim = build_stage()
    setup_compositor()
    if 'poses' in ARGV:
        shot_poses(sun, rim)
        clear_shot()
        return
    sheets = [k for k in SHEET_KEYS if k in ARGV] or list(SHEET_KEYS)
    shots = [s for s in SHOT_KEYS if s in ARGV] or list(SHOT_KEYS)
    for sheet in sheets:
        for shot in shots:
            SHOTS[shot](sheet, sun, rim)
            clear_shot()
        write_sheet(sheet)
    log(f'all done in {time.time() - t0:.0f} s')


main()
