"""The Providence's dead (Art/Models/Creatures/DeadMiner.py and Foreman.py): what both build from. Unpaid.py's own code
run as a library (its bones, rest, head, arms, shroud and hit zones, so their skeletons are the Unpaid's and the code's rig
poses them unchanged); the carbide lamp, the pick and its handle, two-sided brims; where a part rests on its bone, as a
table for the code; and the previews' copies, looks and the code's calm carry of the pick.

Every model script under Art/Models runs on its own (Tools/models.ps1), so what they share lives here, beside
looter_ghosts.py."""
import json
import math
import os
import sys

import bpy
import numpy as np
from mathutils import Matrix, Vector

import looter_ghosts as lg
import looter_textures as lt

unit, smoothstep = lg.unit, lg.smoothstep
UNPAID = os.path.join(lt.REPO, 'Art', 'Models', 'Creatures', 'Unpaid.py')
FIST = (1.15, 1.2, 1.2, 1.15)                # a fist's finger curls (the Unpaid's idle hand's, closed round a haft)


# --- Unpaid.py as a library ---

def unpaid_library():
    """Unpaid.py's source run up to its build into a namespace of its own: nothing of the Unpaid is made. Its functions
    read their globals from that namespace, so a model can swap them (its hands, its head's shape, its shroud's path)."""
    source = open(UNPAID, encoding='utf-8').read()
    stop = '\nrig, body, head, torso, raw_colors = build_rig()'
    ns = {'__name__': 'unpaid_library', '__file__': UNPAID}
    exec(compile(source[:source.index(stop)], UNPAID, 'exec'), ns)
    return ns


def fist_right(U, thumb=0.85, claw=0.15, **hand):
    """The right hand closed into a fist round a haft: the idle hand's fingers closed (FIST), the thumb over them. The
    Unpaid's build_arms and its skeleton both read CURLS and SPREAD (the older rest's finger lines are the fist's, turned
    back by the code's idle), so the finger bones follow. hand: more of hand_pieces' shape (scale, length, ...), both
    hands."""
    U['CURLS'] = {1.0: U['CURLS'][1.0], -1.0: FIST}
    U['SPREAD'] = {1.0: U['SPREAD'][1.0], -1.0: 0.04}
    original = U['hand_pieces']

    def hand_pieces(W, fwd, up, side, curl, spread, **kw):
        kw.update(hand)
        if side < 0:
            kw['thumb'], kw['claw'] = thumb, claw
        return original(W, fwd, up, side, curl, spread, **kw)
    U['hand_pieces'] = hand_pieces


def from_unpaid(p, zone=None):
    """An Unpaid.py part as a looter_ghosts part (its one zone for every vertex, or zone instead)."""
    q = lg.Part(p.name, (p.V, p.F), p.zone if zone is None else zone, slot=p.slot, cavity=p.cavity, ember=p.ember,
                fade=p.fade, strip_uv=p.strip_uv)
    q.weights = dict(p.weights)
    return q


def add_bone(rig, name, head, tail, parent):
    bpy.ops.object.select_all(action='DESELECT')
    bpy.context.view_layer.objects.active = rig
    rig.select_set(True)
    bpy.ops.object.mode_set(mode='EDIT')
    bone = rig.data.edit_bones.new(name)
    bone.head, bone.tail = tuple(map(float, head)), tuple(map(float, tail))
    bone.use_connect = False
    bone.parent = rig.data.edit_bones[parent]
    bpy.ops.object.mode_set(mode='OBJECT')


def fist_grip(U, side, scale=1.0):
    """A fist's grip at rest (the idle hand): where a haft passes through it, the haft's way along the fist's tunnel
    (toward his front), and the way square to it, down (a pick's point). scale: the hand's size over the Unpaid's."""
    E, W, f, u = U['bent'](side)
    f = unit(f)
    u = unit(u - f * np.dot(u, f))
    G = W + (f * 0.082 - u * 0.034) * scale
    t = unit(np.cross(f, u) * -side)
    if t[1] > 0.0:
        t = -t
    point = unit(lg.perp((0.0, 0.0, -1.0), t))
    return G, t, point


def tool_frame(haft, point):
    """A tool's own axes in rig space: its x, y (minus the point's way: a model's front is -Y) and z (the haft)."""
    haft = unit(haft)
    point = unit(lg.perp(point, haft))
    return np.column_stack([np.cross(-point, haft), -point, haft])


# --- Props ---

def two_sided_brim(top, under_drop, zone, cavity_edge=0.25, name='Brim'):
    """A brim as a thin solid: its top grid (rows from the crown out, closed round), the underside under it, the outer
    edge closing them."""
    R, C = top.shape[:2]
    under = top - np.array([0.0, 0.0, under_drop])
    V = np.vstack([top.reshape(-1, 3), under.reshape(-1, 3)])
    F = [lg.grid_faces(R, C)[:, ::-1], lg.grid_faces(R, C) + R * C]
    outer = np.arange(C)
    F.append(np.stack([(R - 1) * C + outer, (R - 1) * C + (outer + 1) % C, R * C + (R - 1) * C + (outer + 1) % C,
                       R * C + (R - 1) * C + outer], 1)[:, ::-1])
    q = np.concatenate([np.repeat(np.linspace(0, 1, R), C)] * 2)
    return lg.Part(name, (V, F), zone, cavity=np.concatenate([cavity_edge * smoothstep(0.75, 1.0, q[:R * C]),
                                                             0.35 + 0.0 * q[R * C:]]))


LENS = np.array([0.0, -0.066, 0.012])        # the burner's tip in the reflector's middle (lamp space): SOCKET_Light


def lamp_parts(reflector=0.05):
    """The carbide lamp in lamp space (its mount at the origin, -Y forward, Z up): the hook plate, the carbide chamber
    and the water tank with its thread ring and valve lever, the burner tube, and the reflector (its radius) polished
    bright to its middle. Zones: 0 the reflector's silver, 1 brass, 2 iron, 3 the strap's leather."""
    brass, dark, strap = [], [], []
    brass.append(lg.box_piece((0.0, -0.005, 0.004), (0.008, 0.005, 0.021)))
    c = np.array([0.0, -0.031, 0.0])
    brass.append(lg.lathe([(-0.042, 0.0), (-0.042, 0.017), (-0.038, 0.02), (-0.004, 0.02), (0.0, 0.022), (0.03, 0.022),
                           (0.034, 0.018), (0.036, 0.0)], c, segs=14))
    dark.append(lg.torus(c, (0.0, 0.0, 1.0), 0.022, 0.0032, segs=14, sides=4))
    dark.append(lg.tube([c + (0.0, 0.0, 0.034), c + (0.0, 0.0, 0.046)], 0.0045, segs=6, domes=1))
    dark.append(lg.box_piece(c + (0.007, 0.0, 0.048), (0.012, 0.0025, 0.002)))
    center = np.array([0.0, -0.06, 0.012])
    Rr, depth = reflector, 0.013 * reflector / 0.05
    t = np.linspace(0.08, 1.0, 5)[:, None]
    a = np.linspace(0.0, 2.0 * np.pi, 20, endpoint=False)[None, :]
    P = center + np.stack([np.cos(a) * Rr * t, -depth * t ** 2 + 0.0 * a, np.sin(a) * Rr * t], -1)     # rim forward
    back = P + np.array([0.0, 0.003, 0.0])
    V = np.vstack([P.reshape(-1, 3), [center], back.reshape(-1, 3), [center + (0.0, 0.003, 0.0)]])
    n = 5 * 20
    F = [lg.grid_faces(5, 20), lg.fan(n, np.arange(20)),
         lg.grid_faces(5, 20)[:, ::-1] + n + 1, lg.fan(2 * n + 1, np.arange(20) + n + 1)[:, ::-1]]
    rim = np.arange(20)
    F.append(np.stack([4 * 20 + rim, 4 * 20 + (rim + 1) % 20, n + 1 + 4 * 20 + (rim + 1) % 20, n + 1 + 4 * 20 + rim], 1))
    shine = np.concatenate([(0.45 * smoothstep(0.3, 1.0, t.ravel()))[:, None].repeat(20, 1).ravel(), [0.0],
                            np.full(n + 1, 0.3)])
    brass.append(lg.torus(center + np.array([0.0, -depth, 0.0]), (0.0, 1.0, 0.0), Rr, 0.0028, segs=24, sides=4))
    dark.append(lg.tube([c + (0.0, -0.016, 0.012), LENS], 0.0035, segs=6, domes=1))
    strap.append(lg.box_piece((0.0, 0.004, 0.004), (0.011, 0.002, 0.026)))
    out = []
    for zone, pieces in ((1, brass), (2, dark), (3, strap)):
        for piece in pieces:
            out.append(lg.Part('Lamp', piece, zone))
    out.append(lg.Part('Reflector', (V, F), 0, cavity=shine))
    return out


def build_lamp(name, material, log, reflector=0.05):
    obj = lg.static_prop(name, lamp_parts(reflector), [material], (0.0, 0.0, 0.0))
    lg.socket(obj, 'Light', LENS)
    log(f'SM_{name}: {lg.triangles_of(obj)} triangles; SOCKET_Light {tuple(np.round(lg.to_ue(LENS), 2))} cm from its pivot')
    return obj


def pick_parts(head=True, grip=0.1, length=0.86, thick=1.0, bands=False):
    """A pick in pick space: its grip at the origin, the haft along +Z from its butt (grip under the grip) to the eye
    (length from the butt), swelling toward the head; with its head, the point to -Y and a narrow chisel to +Y, worn
    bright at their ends; without, the eye end bound with two iron bands (a pick handle). Zones: 0 bright steel, 1 the
    wood, 2 iron."""
    z0, zh = -grip, length - grip
    wood = lg.tube([(0.0, 0.0, z0), (0.0, 0.0, z0 + 0.04), (0.0, 0.0, z0 + 0.4), (0.0, 0.0, zh - 0.12), (0.0, 0.0, zh + 0.03)],
                   np.array([0.0205, 0.018, 0.0155, 0.0165, 0.0195]) * thick,
                   np.array([0.0165, 0.0148, 0.0128, 0.0138, 0.016]) * thick, segs=10, up=(0.0, 1.0, 0.0), domes=2)
    parts = [lg.Part('Haft', wood, 1)]
    if bands:
        for zb in (zh - 0.02, zh - 0.07):
            parts.append(lg.Part('Band', lg.torus((0.0, 0.0, zb), (0.0, 0.0, 1.0), 0.0185 * thick, 0.0035, segs=12,
                                                  sides=4, squash=1.6), 2, cavity=0.25))
    if not head:
        return parts
    iron = [lg.tube([(0.0, -0.034, zh), (0.0, 0.034, zh)], 0.026, 0.021, segs=8, up=(0.0, 0.0, 1.0), domes=1)]
    point = lg.spline([(0.0, -0.03, zh), (0.0, -0.13, zh - 0.006), (0.0, -0.24, zh - 0.024), (0.0, -0.3, zh - 0.045)], 7)
    iron.append(lg.tube(point, np.interp(np.linspace(0, 1, 7), [0, 0.5, 1], [0.019, 0.013, 0.006]),
                        np.interp(np.linspace(0, 1, 7), [0, 0.5, 1], [0.016, 0.011, 0.005]), segs=8, up=(0.0, 0.0, 1.0),
                        domes=2, pointed=0.6))
    chisel = lg.spline([(0.0, 0.03, zh), (0.0, 0.13, zh - 0.004), (0.0, 0.22, zh - 0.018), (0.0, 0.265, zh - 0.03)], 7)
    iron.append(lg.tube(chisel, np.interp(np.linspace(0, 1, 7), [0, 0.6, 1], [0.019, 0.011, 0.004]),
                        np.interp(np.linspace(0, 1, 7), [0, 0.6, 1], [0.016, 0.014, 0.016]), segs=8, up=(0.0, 0.0, 1.0),
                        domes=1))
    for piece in iron:
        parts.append(lg.Part('Iron', piece, 2, cavity=0.08))
    parts.append(lg.Part('Steel', lg.ellipsoid((0.0, -0.288, zh - 0.04), (0.005, 0.016, 0.006), segs=6, rings=4), 0))
    parts.append(lg.Part('Steel', lg.box_piece((0.0, 0.262, zh - 0.029), (0.016, 0.006, 0.0035)), 0))
    return parts


# --- Where the parts rest, for the code ---

def part_rest(rig, point, R, bone_name):
    """A part's rest place: its pivot in component space (cm) and from its bone's head, and its turn from its own axes
    to the component's (FQuat X, Y, Z, W)."""
    head = np.array(rig.data.bones[bone_name].head_local)
    return dict(bone=bone_name, at=(np.round(lg.to_ue(point), 2) + 0.0).tolist(),
                from_bone=(np.round(lg.to_ue(np.asarray(point) - head), 2) + 0.0).tolist(),
                turn=(np.round(lg.quat_ue(R), 5) + 0.0).tolist())


def write_parts(path, rest, log):
    for k, v in rest.items():
        log(f'PART {k}: at {tuple(v["at"])} cm (from {v["bone"]}\'s head {tuple(v["from_bone"])}), turn FQuat '
            f'{tuple(v["turn"])}')
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, 'w', encoding='utf-8', newline='\n') as fh:
        json.dump(dict(units='Unreal component space at size 1: cm; x forward, y right, z up; turn = FQuat (X, Y, Z, W) '
                             'from the part\'s own axes to the component\'s, at rest',
                       parts=rest), fh, indent=1)
    log(f'parts\' rest places written to {os.path.relpath(path, lt.REPO)}')


# --- Previews ---

def kit():
    """The concept kit (stages, lights, renders, the look sheet's composer)."""
    sys.path.insert(0, os.path.join(lt.REPO, 'Art', 'Backlog', 'Creatures'))
    import MinerConcepts as mc
    return mc


def dup(src, where):
    """A copy of a model for a preview: its own mesh, no parent, shown (the sources are hidden while they render)."""
    o = src.copy()
    o.data = src.data.copy()
    o.parent = None
    o.hide_render = False
    where.objects.link(o)
    return o


def preview_look(obj, raw=None, swap=None, rank=0xb02a18, rank_alpha=1.8):
    """M_Ghost's preview materials on a copy: each slot's own look and colors (swap: {slot name: colors} for a tint),
    the coal in the rank's color."""
    lg.ghost_colors(obj, raw)
    for k, mat in enumerate(obj.data.materials):
        if mat.name.startswith('Preview'):
            continue
        if mat.get('Coal'):
            obj.data.materials[k] = lg.preview_material('Coal', dict(EmberStrength=2.1), None, slot_coal=True,
                                                        rank=rank, rank_alpha=rank_alpha)
        else:
            look, colors, grain = lg.slot_look(mat)
            if swap and mat.name in swap:
                colors = swap[mat.name]
            obj.data.materials[k] = lg.preview_material(mat.name, look, colors, rank=rank, rank_alpha=rank_alpha)


def ue_dir(x, y, z):
    """A direction in Unreal's component axes (X forward, Y right, Z up) in Blender's."""
    return unit(np.array([-y, -x, z], float))


def code_arms(rig, arms):
    """Poses the rig as ADeadMinerCreature's LayArm does, arms {side: (upper, fore)} (directions in Blender's axes): the
    upper arm's least turn from its rest way to upper, the forearm's whole turn its own least turn to fore, the hand with
    it. Returns, per side, what's held as UpdatePick places it: its grip 80% of the way from the wrist to the middle
    knuckle, its haft along the forearm, the point square to it (Unreal's Right x Haft), as (grip, frame)."""
    bones = rig.data.bones
    total = {}
    for side, (upper, fore) in arms.items():
        sfx = lg.SUFFIX[side]
        S, E, W = (np.array(bones[n].head_local) for n in (f'upperarm_{sfx}', f'lowerarm_{sfx}', f'hand_{sfx}'))
        total[f'upperarm_{sfx}'] = lg.turn_between(E - S, upper)
        total[f'lowerarm_{sfx}'] = total[f'hand_{sfx}'] = lg.turn_between(W - E, fore)
    T, J = lg.Poser(rig, {}).apply(dict(total=total))
    held = {}
    for side in arms:
        sfx = lg.SUFFIX[side]
        wrist = J[f'hand_{sfx}']
        grip = wrist + 0.8 * (J[f'middle_01_{sfx}'] - wrist)
        haft = unit(J[f'hand_{sfx}'] - J[f'lowerarm_{sfx}'])
        held[side] = (grip, tool_frame(haft, np.cross((1.0, 0.0, 0.0), haft)))
    return held


def code_carry(rig, upper, fore, side=-1.0):
    """code_arms for one arm: (grip, frame)."""
    return code_arms(rig, {side: (upper, fore)})[side]


def rest_pose(rig):
    for pb in rig.pose.bones:
        pb.matrix_basis = Matrix.Identity(4)
    bpy.context.view_layer.update()


def posed_copy(body, where):
    """A copy of the skin as the rig poses it now (no armature)."""
    posed = lg.posed_vertices(body)
    o = dup(body, where)
    o.modifiers.clear()
    o.data.vertices.foreach_set('co', posed.astype(np.float32).ravel())
    o.data.update()
    return o


def place_copy(src, where, M, place):
    """A copy of a prop at place (rig space at rest: a 4x4 or the prop's own location) in a figure standing at M."""
    o = dup(src, where)
    o.matrix_world = M @ place
    return o


def lamp_glow(mc, where, lamp, halo=0.03, light=0.0, flare=1.0):
    """What the code lights a carbide lamp with, for the previews: a small white-hot jet at the burner (leaning up as a
    carbide flame does), a soft halo round it and, if asked, a warm point light (watts at size 1)."""
    M = lamp.matrix_world
    k = M.to_scale()[0]
    p = np.array(M @ Vector(tuple(LENS)))
    ahead = np.array((M.to_3x3() @ Vector((0.0, -1.0, 0.0))).normalized())
    V, F = mc.flame_piece(p, unit(ahead + np.array([0.0, 0.0, 0.7])), 0.024 * k, 0.0075 * k)
    out = [mc.uc.mesh_object('LampFlame', V, F, mc.flame_material(flare), where)]
    r = halo * k * (1.0 + 0.3 * (flare - 1.0))
    V, F = mc.uc.ellipsoid(p + ahead * 0.008 * k, (r, r, r * 1.15), segs=24, rings=14)
    out.append(mc.uc.mesh_object('LampHalo', V, F, mc.halo_material(flare), where))
    out[-1].visible_shadow = False
    out[0].visible_shadow = False
    if light:
        bulb = bpy.data.objects.new('LampLight', bpy.data.lights.new('LampLight', 'POINT'))
        bulb.data.energy = light * flare * k * k
        bulb.data.color = lt.hex_color(mc.FLAME)[:3]
        bulb.data.shadow_soft_size = 0.03
        bulb.location = Vector(tuple(p + ahead * 0.03 * k))
        where.objects.link(bulb)
        out.append(bulb)
    return out


def figure_matrix(location, turn):
    return Matrix.Translation(Vector(location)) @ Matrix.Rotation(math.radians(turn), 4, 'Z')


def frame_matrix(point, R):
    return Matrix.Translation(Vector(tuple(point))) @ Matrix(np.asarray(R).tolist()).to_4x4()
