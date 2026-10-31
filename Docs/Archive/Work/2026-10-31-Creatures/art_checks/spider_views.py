"""Check renders of SK_Spider (Art/Models/Creatures/Spider.py, the Tripo build by default): builds the spider without
painting its atlas again (the textures on disk are used), poses its rig as ASpiderCreature does and renders each pose
on a ground plane into Saved/ArtPreviews/Creatures/SpiderTripo/:
  side_rest (orthographic, level check: the feet on the ground, the body level), q_rest, front_rest, top_rest,
  q_gait (a stride: one group of feet lifted and carried), side_attack (the bite's rear: nose up 24 deg, the front legs
  raised as SpiderCreature.cpp holds them), q_death (the curl: SpiderCreatureDeath.cpp's dead pose at DeadRide).
    Tools\\artrun.ps1 -Script Tools\\Blender\\spider_tripo_views.py [-ScriptArgs '--clay']
The poses are the C++'s numbers in creature space (x forward, y left: the C++'s Unreal y is to the right)."""
import math
import os
import sys

import bpy
import numpy as np
from mathutils import Matrix, Vector

import looter_creatures as lc
import looter_textures as lt

ARGS = sys.argv[sys.argv.index('--') + 1:] if '--' in sys.argv else []
OUT = os.path.join(lt.PREVIEW_DIR, 'Creatures', 'Fix1031', 'spider')
SPIDER = os.path.join(lt.REPO, 'Art', 'Models', 'Creatures', 'Spider.py')
ONLY = os.environ.get('VIEWS_ONLY', '')

# --- Build (the atlas isn't painted again) ---
lc.paint_atlas = lambda *a, **k: None
ns = {'__file__': SPIDER, '__name__': '__spider__'}
with open(SPIDER, encoding='utf-8') as f:
    exec(compile(f.read(), SPIDER, 'exec'), ns)
rig = next(o for o in bpy.data.objects if o.type == 'ARMATURE')
spider = ns['spider']
for o in list(bpy.data.objects):
    if o.name.startswith(('UCX_', 'USP_', 'UCP_')):
        o.hide_render = True
RIDE = ns['RIDE']
bones = {name: (np.array(h[:]), np.array(t[:])) for name, h, t, _ in ns['bones']}
TB = lc.TO_BLENDER
TBI = TB.inverted()


def M(R=None, t=(0, 0, 0)):
    out = Matrix.Identity(4)
    if R is not None:
        out = Matrix([list(R[0]) + [0], list(R[1]) + [0], list(R[2]) + [0], [0, 0, 0, 1]])
    out.translation = Vector(t)
    return out


def rot(axis, deg):
    return np.array(Matrix.Rotation(math.radians(deg), 3, axis))


def seg(a, b, pole):
    """SegmentFrame: x along a->b, z toward pole (FRotationMatrix::MakeFromXZ), at a."""
    x = (b - a) / np.linalg.norm(b - a)
    z = pole - x * (pole @ x)
    z /= np.linalg.norm(z)
    y = np.cross(z, x)
    return M(np.column_stack([x, y, z]), a)


def knee_pole(up, outward):
    return up + outward * 0.4


def solve(hip, target, upper, lower, pole):
    d = target - hip
    dist = np.linalg.norm(d)
    direction = d / dist
    dist = min(max(dist, abs(upper - lower) + 1.0), (upper + lower) * 0.999)
    along = (upper * upper - lower * lower + dist * dist) / (2.0 * dist)
    height = math.sqrt(max(upper * upper - along * along, 0.0))
    bend = pole - direction * (pole @ direction)
    bend /= np.linalg.norm(bend)
    return hip + direction * along + bend * height, hip + direction * dist


LEGS = [(p, s) for p in range(4) for s in ('l', 'r')]


def rest_leg(pair, s):
    hip, knee = bones[f'femur_{pair}_{s}']
    foot = bones[f'tibia_{pair}_{s}'][1]
    return hip, knee, foot


def pose(body_R, body_t, fang=3.0, abdomen_pitch=0.0, feet=None, spread=None):
    """Sets the rig: the body frame (turn body_R about the thorax, then moved to body_t), the fangs open fang degrees,
    each leg's foot target (creature space, or None: its rest spot) solved toward its pole."""
    thorax = bones['body'][0]
    body = M(body_R, body_t) @ M(None, -thorax)            # rest -> posed, for everything on the body
    world = {}
    for name in ('body', 'head', 'palp_l', 'palp_r'):
        world[name] = body
    pivot = bones['abdomen'][0]
    world['abdomen'] = body @ M(None, pivot) @ M(rot('Y', -abdomen_pitch)) @ M(None, -pivot)
    for side, name in ((1.0, 'fang_l'), (-1.0, 'fang_r')):
        p = bones[name][0]
        # FRotator(Fang * 0.4, -Fang, 0) on the left (Unreal yaw is clockwise seen from above, its y to the right).
        R = rot('Z', side * fang) @ rot('Y', -fang * 0.4)
        world[name] = body @ M(None, p) @ M(R) @ M(None, -p)
    up = np.array(body_R) @ np.array([0.0, 0.0, 1.0])
    centre = np.array(body @ Vector(thorax))
    for pair, s in LEGS:
        hip, knee, foot = rest_leg(pair, s)
        upper, lower = np.linalg.norm(knee - hip), np.linalg.norm(foot - knee)
        rest_pole = knee_pole(np.array([0, 0, 1.0]), _flat(hip - thorax))
        hip_p = np.array(body @ Vector(hip))
        out = _flat(hip_p - centre)
        target = foot if feet is None or (pair, s) not in feet else feet[(pair, s)]
        if callable(target):
            target = target(hip_p, out)
        pole = knee_pole(up, out)
        if spread is not None:
            stretch = np.linalg.norm(target - hip_p)
            rs = np.linalg.norm(foot - hip)
            pole = pole + out * min(max((rs - stretch) / rs, 0.0) * 3.5, 1.0)
        knee_p, foot_p = solve(hip_p, target, upper, lower, pole) if not isinstance(target, tuple) else target
        world[f'femur_{pair}_{s}'] = seg(hip_p, knee_p, pole) @ seg(hip, knee, rest_pole).inverted()
        world[f'tibia_{pair}_{s}'] = seg(knee_p, foot_p, pole) @ seg(knee, foot, rest_pole).inverted()
        world[f'foot_{pair}_{s}'] = world[f'tibia_{pair}_{s}']
    for pb in rig.pose.bones:
        pb.matrix_basis = Matrix.Identity(4)
    bpy.context.view_layer.update()
    order = sorted(rig.pose.bones, key=lambda pb: len(pb.parent_recursive))
    for pb in order:
        T = world.get(pb.name, body)
        pb.matrix = (TB @ T @ TBI) @ rig.data.bones[pb.name].matrix_local
        bpy.context.view_layer.update()
    return world


def _flat(v):
    v = np.array([v[0], v[1], 0.0])
    return v / np.linalg.norm(v)


# --- The scene ---
sc = bpy.context.scene
sc.render.engine = 'BLENDER_EEVEE_NEXT'
sc.render.resolution_x, sc.render.resolution_y = 1600, 1000
sc.render.film_transparent = False
sc.view_settings.view_transform = 'Standard'
world = bpy.data.worlds.new('w')
world.use_nodes = True
world.node_tree.nodes['Background'].inputs['Color'].default_value = (0.55, 0.6, 0.68, 1.0)
world.node_tree.nodes['Background'].inputs['Strength'].default_value = 0.7
sc.world = world
sun = bpy.data.objects.new('sun', bpy.data.lights.new('sun', 'SUN'))
sun.data.energy = 3.5
sun.rotation_euler = (math.radians(50), 0, math.radians(-35))
sc.collection.objects.link(sun)
ground_mat = bpy.data.materials.new('ground')
ground_mat.use_nodes = True
nodes = ground_mat.node_tree.nodes
checker = nodes.new('ShaderNodeTexChecker')
checker.inputs['Scale'].default_value = 12.0
checker.inputs['Color1'].default_value = (0.42, 0.40, 0.36, 1)
checker.inputs['Color2'].default_value = (0.36, 0.34, 0.31, 1)
ground_mat.node_tree.links.new(checker.outputs['Color'], nodes['Principled BSDF'].inputs['Base Color'])
# A slab, so the orthographic side view shows the ground as a line the feet must stand on.
bpy.ops.mesh.primitive_cube_add(size=1.0, location=(0.0, 0.0, -0.01))
ground = bpy.context.active_object
ground.scale = (6.0, 6.0, 0.02)
ground.data.materials.append(ground_mat)
if '--clay' in ARGS:
    clay = bpy.data.materials.new('clay')
    clay.diffuse_color = (0.7, 0.68, 0.64, 1)
    spider.data.materials.clear()
    spider.data.materials.append(clay)
cam = bpy.data.objects.new('cam', bpy.data.cameras.new('cam'))
sc.collection.objects.link(cam)
sc.camera = cam
os.makedirs(OUT, exist_ok=True)


def shot(name, direction, target=(0.0, 0.0, 0.55), ortho=None, distance=6.0, lens=50):
    d = Vector(direction).normalized()
    cam.location = Vector(target) + d * distance
    cam.rotation_euler = (-d).to_track_quat('-Z', 'Y').to_euler()
    if ortho:
        cam.data.type, cam.data.ortho_scale = 'ORTHO', ortho
    else:
        cam.data.type, cam.data.lens = 'PERSP', lens
    sc.render.filepath = os.path.join(OUT, name + '.png')
    bpy.ops.render.render(write_still=True)
    print('SPIDER VIEW', sc.render.filepath)


I = np.eye(3)
thorax = bones['body'][0]
# Blender: the creature's front is -Y, its left +X.
pose(I, thorax)
shot('side_rest', (1.0, 0.0, 0.0), target=(0.0, 0.2, 0.6), ortho=4.2, distance=8)
shot('q_rest', (1.1, -1.4, 0.8), target=(0.0, 0.2, 0.5), distance=6.5)
shot('front_rest', (0.0, -1.0, 0.25), target=(0.0, 0.0, 0.6), distance=6.5)
shot('top_rest', (0.0, 0.0001, 1.0), target=(0.0, 0.2, 0.0), ortho=5.0, distance=8)
shot('face_rest', (0.6, -1.0, 0.35), target=(0.0, -0.55, 0.65), distance=3.0)
ground.hide_render = True
shot('under_rest', (0.5, -1.0, -0.45), target=(0.0, 0.0, 0.5), distance=5.0)
ground.hide_render = False

# A stride: group A (L0 R1 L2 R3) lifted 25 cm mid-step, 5 cm ahead; group B planted 20 cm behind its spots.
groupA = {(0, 'l'), (1, 'r'), (2, 'l'), (3, 'r')}
feet = {}
for pair, s in LEGS:
    foot = rest_leg(pair, s)[2]
    feet[(pair, s)] = foot + (np.array([5.0, 0, 25.0]) if (pair, s) in groupA else np.array([-20.0, 0, 0]))
pose(I, thorax + np.array([0, 0, 1.5]), feet=feet, spread=True)
shot('q_gait', (1.1, -1.4, 0.7), target=(0.0, 0.2, 0.5), distance=6.5)
shot('side_gait', (1.0, 0.0, 0.0), target=(0.0, 0.2, 0.6), ortho=4.2, distance=8)

# The bite's wind-up at its peak: nose up 24, up 16, back 14, fangs 30; the front legs held at the body's
# (Hip.X + 80, Hip.Y * 1.5, 50) (SpiderCreature.cpp); the rest planted on their spots.
R = rot('Y', -24.0)
t = thorax + np.array([-14.0, 0, 16.0])
feet = {}
body = M(R, t) @ M(None, -thorax)
for s in ('l', 'r'):
    hip = rest_leg(0, s)[0] - thorax
    feet[(0, s)] = np.array(body @ Vector(thorax + np.array([hip[0] + 80.0, hip[1] * 1.5, 50.0])))
pose(R, t, fang=30.0, feet=feet, spread=True)
shot('side_attack', (1.0, 0.0, 0.0), target=(0.0, 0.2, 0.7), ortho=4.2, distance=8)
shot('q_attack', (1.1, -1.4, 0.6), target=(0.0, 0.0, 0.6), distance=6.5)

# Dead (SpiderCreatureDeath.cpp DeadPose): the thorax at DeadRide 46, every femur swung toward the body's line and down
# to its knee on the ground, the tibia folded back under the body.
DEAD_RIDE = 46.0
YAW, KNEE_H, TIP_SIDE, TIP_H = (8, 22, 156, 142), (12, 12, 12, 9), (6, 18, 8, 30), (5, 3, 7, 3)
dead = {}
dt = np.array([thorax[0], thorax[1], DEAD_RIDE])
for pair, s in LEGS:
    hip, knee, foot = rest_leg(pair, s)
    side = 1.0 if s == 'l' else -1.0
    upper, lower = np.linalg.norm(knee - hip), np.linalg.norm(foot - knee)
    h = hip - thorax
    yaw = math.radians(YAW[pair])
    flat = np.array([math.cos(yaw), side * math.sin(yaw), 0.0])
    drop = min(max((h[2] - (KNEE_H[pair] - DEAD_RIDE)) / upper, -1.0), 1.0)
    k = h + (flat * math.sqrt(1 - drop * drop) - np.array([0, 0, 1.0]) * drop) * upper
    across = side * TIP_SIDE[pair] - k[1]
    down = (TIP_H[pair] - DEAD_RIDE) - k[2]
    along = lower * lower - across * across - down * down
    f = k + np.array([(-1.0 if flat[0] > 0 else 1.0) * math.sqrt(max(along, 0.0)), across, down])
    dead[(pair, s)] = (dt + k, dt + f)
dead_world = pose(I, dt, fang=20.0, feet=dead)
shot('q_death', (1.1, -1.4, 0.9), target=(0.0, 0.2, 0.35), distance=6.0)
shot('side_death', (1.0, 0.0, 0.0), target=(0.0, 0.2, 0.4), ortho=4.2, distance=8)



# --- Extra poses (2026-10-31 art fix): the extremes the gait reaches ---
def reach_feet(share):
    """Every foot along its resting direction from the hip, share of the leg's full reach away (the ground or below)."""
    out = {}
    for pair, s in LEGS:
        hip, knee, foot = rest_leg(pair, s)
        upper, lower = np.linalg.norm(knee - hip), np.linalg.norm(foot - knee)
        d = foot - hip
        d = d / np.linalg.norm(d)
        out[(pair, s)] = hip + d * (upper + lower) * share
    return out

pose(I, thorax, feet=reach_feet(0.97), spread=True)
shot('x_reach_q', (1.1, -1.4, 0.6), target=(0.0, 0.2, 0.4), distance=7.5)
shot('x_reach_side', (1.0, 0.0, 0.0), target=(0.0, 0.2, 0.4), ortho=5.0, distance=8)
pose(I, thorax, feet=reach_feet(0.5), spread=True)
shot('x_fold_q', (1.1, -1.4, 0.6), target=(0.0, 0.2, 0.5), distance=6.0)
# A long running stride: group A 70 cm ahead and lifted 30, group B 70 cm behind.
feet = {}
for pair, s in LEGS:
    foot = rest_leg(pair, s)[2]
    feet[(pair, s)] = foot + (np.array([70.0, 0, 30.0]) if (pair, s) in groupA else np.array([-70.0, 0, 0]))
pose(I, thorax + np.array([0, 0, 1.5]), feet=feet, spread=True)
shot('x_run_q', (1.1, -1.4, 0.5), target=(0.0, 0.2, 0.5), distance=6.5)
shot('x_run_side', (1.0, 0.0, 0.0), target=(0.0, 0.2, 0.6), ortho=4.6, distance=8)
# As the player sees it: eye height 1.6 m, 3.2 m off its front-right, a wide lens (the game's 90 deg view).
pose(I, thorax + np.array([0, 0, 1.5]), feet=feet, spread=True)
cam.data.type, cam.data.lens = 'PERSP', 18
d = Vector((1.0, -1.6, 0.0)).normalized()
cam.location = Vector((0.0, 0.0, 1.6)) + d * 3.2
cam.rotation_euler = (Vector((0.0, -0.2, 0.45)) - cam.location).to_track_quat('-Z', 'Y').to_euler()
sc.render.filepath = os.path.join(OUT, 'x_player_run.png')
bpy.ops.render.render(write_still=True)
pose(I, thorax)
# --- Checks the game's tests make (Looter.Creatures.Spider.HitZones, ...Anim.SpiderDeathCurl), measured here ---

def hulls():
    """Each UCX_ hit hull as (bone, points cm, planes [(normal, offset)]) in creature space at rest."""
    import bmesh
    out = []
    for o in bpy.data.objects:
        if not o.name.startswith('UCX_'):
            continue
        P = [np.array(TBI @ (o.matrix_world @ v.co)) for v in o.data.vertices]
        bm = bmesh.new()
        for p in P:
            bm.verts.new(p)
        hull = bmesh.ops.convex_hull(bm, input=bm.verts)
        bm.normal_update()
        centre = np.mean(P, axis=0)
        planes = []
        for f in [g for g in hull['geom'] if isinstance(g, bmesh.types.BMFace)]:
            n = np.array(f.normal)
            if n @ (np.array(f.calc_center_median()) - centre) < 0:
                n = -n
            planes.append((n, n @ np.array(f.verts[0].co)))
        bm.free()
        out.append((o['Bone'], np.array(P), planes))
    return out


def depth(planes, p):
    """How far p lies inside a hull (cm; negative outside)."""
    return -max(n @ p - d for n, d in planes)


def apply(T, p):
    return np.array(T @ Vector(p))


H = hulls()
by_bone = {}
for bone, P, planes in H:
    by_bone.setdefault(bone, []).append((P, planes))
print(f'SPIDER CHECK: {len(by_bone)} bones with hit hulls (the test wants 23)')
deepest, where = 0.0, ''
for bone, shapes in by_bone.items():
    if not bone.startswith('femur'):
        continue
    hip = bones[bone][0]
    T = dead_world[bone]
    for P, _ in shapes:
        for p in P:
            if np.linalg.norm(p - hip) < 10.0:
                continue
            posed = apply(T, p)
            hidden = any(depth(pl, apply(dead_world['body'].inverted(), posed)) > 0 for _, pl in by_bone['body'])
            if hidden:
                continue
            for _, pl in by_bone['abdomen']:
                now = depth(pl, apply(dead_world['abdomen'].inverted(), posed))
                rest = max(depth(pl, p), 0.0)
                if now - rest > deepest:
                    deepest, where = now - rest, f'{bone}, {np.linalg.norm(p - hip):.0f} cm from its hip'
print(f'SPIDER CHECK: dead, the femurs reach {deepest:.1f} cm into the abdomen past rest at worst ({where}); '
      f'the test allows 3')
tips = [bones[f'foot_{p}_{s}'][0][2] for p, s in LEGS]
print(f'SPIDER CHECK: the feet at rest {min(tips):.2f}..{max(tips):.2f} cm over the ground')


def enter(planes, origin, direction):
    """Where a ray first enters a convex hull (its distance), or None."""
    t0, t1 = 0.0, 1e9
    for n, d in planes:
        nd, no = n @ direction, n @ origin
        if abs(nd) < 1e-12:
            if no > d:
                return None
            continue
        t = (d - no) / nd
        if nd < 0:
            t0 = max(t0, t)
        else:
            t1 = min(t1, t)
    return t0 if t0 <= t1 else None


# Looter.Creatures.Spider.Shots, at rest (its Unreal y is to the right: creature y = -Unreal y).
body = bones['body'][0]
femur_mid = (bones['femur_0_r'][0] + bones['femur_0_r'][1]) * 0.5
SHOTS = (('head-on', body + np.array([400.0, 0, 10]), body + np.array([0.0, 0, 10]), 'head'),
         ('thorax from above', body + np.array([0, 0, 200.0]), body - np.array([0, 0, 100.0]), 'body'),
         ('abdomen from behind', body + np.array([-400.0, 0, 15]), body + np.array([-50.0, 0, 15]), 'abdomen'),
         ('left fang', body + np.array([400.0, 6.0, -8]), body + np.array([0.0, 6.0, -8]), 'fang_l'),
         ('front right femur from above', femur_mid + np.array([0, 0, 150.0]), femur_mid - np.array([0, 0, 150.0]),
          'femur_0_r'),
         ('well over it', body + np.array([400.0, 0, 150]), body + np.array([-400.0, 0, 150]), None))
for what, a, b, want in SHOTS:
    d = (b - a) / np.linalg.norm(b - a)
    hits = []
    for bone, P, planes in H:
        t = enter(planes, a, d)
        if t is not None and t <= np.linalg.norm(b - a):
            hits.append((t, bone))
    got = min(hits)[1] if hits else None
    print(f'SPIDER CHECK: shot {what}: hits {got} (the test wants {want}) {"ok" if got == want else "WRONG"}')
sizes = {}
for p in spider.data.polygons:
    sizes[p.loop_total] = sizes.get(p.loop_total, 0) + 1
tris = sum(n * (k - 2) for k, n in sizes.items())
print(f'SPIDER CHECK: faces by corners {dict(sorted(sizes.items()))}; {tris} triangles, '
      f'{sizes.get(4, 0) * 2 / tris * 100:.0f}% of them in quads; {len(rig.data.bones)} bones, '
      f'{len(spider.data.materials)} material slot(s) {[m.name for m in spider.data.materials]}')
used = np.zeros(len(spider.data.vertices), bool)
for p in spider.data.polygons:
    used[list(p.vertices)] = True
print(f'SPIDER CHECK: {len(spider.data.vertices)} vertices, {int((~used).sum())} loose (no face)')
