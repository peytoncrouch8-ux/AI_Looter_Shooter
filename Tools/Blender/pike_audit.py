"""Pike's audit (Pike.py --audit <tag>, as Ellis's ellis_audit.py on looter_audit.py): every layer checked against the
one over it in the game's poses (the swing chains where BodySwing's springs would rest them), the poses in clay, and
the baked look in daylight as a turntable and close-ups, into Saved/Concepts/PikeAudit/<tag>/ (findings.json). Not in
the bake's digest."""
import json
import math
import os

import bpy
import numpy as np

import looter_audit as la

# (inner, outer): the inner layer must stay under the outer one. (Not the collar and the neckerchief: in front the
# neckerchief lies over the collar's ends, round the back under the collar, by design.)
PAIRS = (('Shirt', 'Waistcoat'), ('Waistcoat', 'Coat'), ('Shirt', 'Coat'), ('Trousers', 'Skirt'), ('Waistcoat', 'Skirt'),
         ('Coat', 'Belt'), ('Skirt', 'Belt'), ('Trousers', 'Puttee'), ('Puttee', 'Boots'), ('Trousers', 'Boots'),
         ('Coat', 'Strap'), ('Coat', 'Bandolier'), ('Coat', 'Kerchief'), ('Pike', 'Kerchief'),
         ('Coat', 'Cuff'), ('Pike', 'Cuff'), ('Coat', 'Lapel'), ('Coat', 'Pocket'), ('Skirt', 'Pocket'), ('Pocket', 'Flap'),
         ('Coat', 'Collar'), ('Pike', 'Collar'), ('Pike', 'Whiskers'), ('Pike', 'Coat'))
# (part, solid): the part must not be inside the solid (or behind the sheet)
SOLIDS = (('Spyglass', 'Trousers'), ('Spyglass', 'Skirt'), ('Coat', 'Rucksack'), ('Skirt', 'Bedroll'), ('Coat', 'Bedroll'),
          ('Strap', 'Rucksack'), ('Bandolier', 'Rucksack'), ('Pike', 'Skirt'), ('Tripod', 'Coat'), ('MapTube', 'HatBrim'),
          ('Tripod', 'HatBrim'), ('HatBrim', 'Rucksack'))
GROUNDED = ('Skirt', 'Spyglass', 'Kerchief')
POSES = ('rest', 'idle', 'aim', 'stride', 'crouch', 'grip_a', 'grip_b', 'aim_high', 'crouch_aim', 'carry')


def family(name):
    return name.split('.')[0].split('_')[0]


def parts_named(fig, fam):
    names = [n for n in fig.rest if family(n) == fam]
    co, faces, W, off = [], [], [], 0
    for n in names:
        c, f, w = fig.rest[n]
        co.append(c)
        faces += [[i + off for i in p] for p in f]
        W.append(w)
        off += len(c)
    if not co:
        return None
    return np.concatenate(co), faces, np.concatenate(W)


def chains(fig):
    return {c['name']: [f'{c["name"]}_{k + 1:02d}' for k in range(len(c['joints']) - 1)] for c in fig.swing}


def posed_parts(fig, arm, pose):
    la.pose(arm, la.POSES[pose])
    la.settle_swing(arm, chains(fig), la.POSES[pose])
    mats = la.skinning(arm, fig.rig_names)
    cache = {}

    def get(fam):
        if fam not in cache:
            p = parts_named(fig, fam)
            cache[fam] = None if p is None else (la.deform(p[0], p[2], mats), p[1])
        return cache[fam]
    return get


BONES = ('pelvis', 'spine_01', 'spine_02', 'spine_03', 'spine_04', 'spine_05', 'neck_01', 'neck_02', 'head') + tuple(
    f'{b}_{s}' for b in ('clavicle', 'upperarm', 'lowerarm', 'hand', 'thigh', 'calf', 'foot') for s in 'lr')


def check(fig, arm, pose):
    get = posed_parts(fig, arm, pose)
    pb = arm.pose.bones
    bones = (np.array([tuple(pb[b].head) for b in BONES]), np.array([tuple(pb[b].tail) for b in BONES]))
    found = []
    for inner, outer in PAIRS:
        a, b = get(inner), get(outer)
        if a is None or b is None:
            continue
        idx, deep = la.poke_through(a, b, outward=la.away_from_bones(a[0], bones))
        if len(idx):
            found.append(('through', inner, outer, int(len(idx)), float(deep.max() * 100),
                          [round(float(v) * 100, 1) for v in a[0][idx].mean(0)]))
    for part, solid in SOLIDS:
        a, b = get(part), get(solid)
        if a is None or b is None:
            continue
        idx, deep = la.inside(a[0], b)
        if len(idx):
            found.append(('inside', part, solid, int(len(idx)), float(deep.max() * 100),
                          [round(float(v) * 100, 1) for v in a[0][idx].mean(0)]))
    for part in GROUNDED:
        a = get(part)
        if a is None:
            continue
        idx = np.nonzero(a[0][:, 2] < 0.0)[0]
        if len(idx):
            found.append(('under', part, 'the ground', int(len(idx)), float(-a[0][idx, 2].min() * 100),
                          [round(float(v) * 100, 1) for v in a[0][idx].mean(0)]))
    return found


def one_sided(M, fig, arm, skin, baked, path):
    """The baked look drawn one-sided as Unreal draws it (a face seen from behind is see-through: Cycles has no back
    face culling, so each material turns transparent on its back faces), at the game's idle: the front and both hips
    close (a hole shows the sky through him)."""
    M.viewer_materials(skin, baked)
    for slot in skin.material_slots:
        mat = slot.material
        if mat is None or mat.get('_one_sided'):
            continue
        nt = mat.node_tree
        out = next(n for n in nt.nodes if n.type == 'OUTPUT_MATERIAL')
        src = out.inputs['Surface'].links[0].from_socket
        geo = nt.nodes.new('ShaderNodeNewGeometry')
        mix = nt.nodes.new('ShaderNodeMixShader')
        clear = nt.nodes.new('ShaderNodeBsdfTransparent')
        nt.links.new(geo.outputs['Backfacing'], mix.inputs[0])
        nt.links.new(src, mix.inputs[1])
        nt.links.new(clear.outputs[0], mix.inputs[2])
        nt.links.new(mix.outputs[0], out.inputs['Surface'])
        mat['_one_sided'] = True
    la.cycles(int(os.environ.get('AUDIT_SAMPLES', 32)))
    la.light('day')
    if os.environ.get('AUDIT_SUN'):
        # another sun: 'tilt,turn' degrees (the sun's own X and Z turns; 65,35 a low sun from his left front) under a
        # dimmer sky, as a late afternoon in the game
        tilt, turn = (float(v) for v in os.environ['AUDIT_SUN'].split(','))
        sun = next(o for o in bpy.data.objects if o.name.startswith('_AuditSun'))
        sun.rotation_euler = (math.radians(tilt), 0.0, math.radians(turn))
        bpy.context.scene.world.node_tree.nodes['Background'].inputs['Strength'].default_value = 0.35
    # the arms as the game's idle holds them, or closer in (AUDIT_ARM degrees off the body: the hands by the skirt)
    steps = la._arms_down(float(os.environ['AUDIT_ARM'])) if os.environ.get('AUDIT_ARM') else la.POSES['idle']
    if os.environ.get('AUDIT_POSE'):
        steps = la.POSES[os.environ['AUDIT_POSE']]
    elif os.environ.get('AUDIT_FORE'):
        # the arms forward of the plain idle ('upper,fore': each one's forward share; the game's idle in the hero
        # shots holds the hands in front of the hips, the forearms leaning forward)
        up, fore = (float(v) for v in os.environ['AUDIT_FORE'].split(','))
        s = math.sin(math.radians(float(os.environ.get('AUDIT_ARM', 10.0))))
        steps = [('point', 'upperarm_l', (s, -up, -1.0)), ('point', 'lowerarm_l', (s * 0.6, -fore, -1.0)),
                 ('point', 'upperarm_r', (-s, -up, -1.0)), ('point', 'lowerarm_r', (-s * 0.6, -fore, -1.0))]
    la.pose(arm, steps)
    pbs = arm.pose.bones
    skirt = [n for c, names in chains(fig).items() if c.startswith('skirt') for n in names]
    before = {n: pbs[n].tail.copy() for n in skirt}
    la.settle_swing(arm, chains(fig), steps)
    for n in skirt:
        d = pbs[n].tail - before[n]
        if d.length > 0.01:
            print(f'PIKE one-sided: {n} tip pushed {d.length * 100:.1f} cm ({d.x * 100:.1f}, {d.y * 100:.1f}, '
                  f'{d.z * 100:.1f}) by the springs and limbs', flush=True)
    for side in 'lr':
        print(f'PIKE one-sided: hand_{side} {tuple(round(v, 3) for v in pbs["hand_" + side].head)} to '
              f'{tuple(round(v, 3) for v in pbs["middle_01_" + side].head)}', flush=True)
    shots = {'onesided_front': ((0.0, -0.1, 0.95), (0.0, -2.6, 0.1), 85.0, (1200, 1400)),
             'onesided_hip_left': ((0.15, -0.1, 0.82), (0.45, -1.3, 0.05), 85.0, (1200, 1200)),
             'onesided_hip_left_side': ((0.18, -0.02, 0.82), (1.3, -0.35, 0.05), 85.0, (1200, 1200)),
             'onesided_hip_right': ((-0.15, -0.1, 0.82), (-0.45, -1.3, 0.05), 85.0, (1200, 1200)),
             'onesided_spyglass': ((0.21, 0.06, 0.85), (1.2, 0.55, 0.1), 85.0, (1200, 1200))}
    for name, (target, off, lens, size) in shots.items():
        la.shoot(path(name), target, off, lens, size)
    la.reset(arm)


def run(M, fig, arm, skin, baked, tag):
    out = os.path.join(M.REPO, 'Saved', 'Concepts', 'PikeAudit', tag)
    os.makedirs(out, exist_ok=True)
    path = lambda n: os.path.join(out, n + '.png')
    if os.environ.get('AUDIT_ONESIDED'):
        one_sided(M, fig, arm, skin, baked, path)
        M.log(f'audit: one-sided renders in {out}')
        return
    poses = os.environ.get('AUDIT_POSES', ','.join(POSES)).split(',')
    report = {}
    for pose in poses:
        report[pose] = check(fig, arm, pose)
        for kind, a, b, n, deep, where in report[pose]:
            M.log(f'audit {pose}: {a} {kind} {b}: {n} points, {deep:.1f} cm deep at most, about {where} cm')
        if not report[pose]:
            M.log(f'audit {pose}: nothing through')
    with open(os.path.join(out, 'findings.json'), 'w') as f:
        json.dump(report, f, indent=1)
    if os.environ.get('AUDIT_RENDERS') == '0':
        la.reset(arm)
        return
    la.clay()
    bpy.context.scene.world = bpy.data.worlds.get('Preview') or bpy.data.worlds.new('Preview')
    bpy.context.scene.world.color = (0.30, 0.33, 0.38)
    skin.color = (0.62, 0.58, 0.53, 1.0)
    for ob in arm.children:
        if ob.name.startswith(('USP_', 'UCP_', 'UCX_')):
            ob.hide_render = True
    views = {'front': (0.0, -4.0, 0.3), 'side': (4.0, 0.0, 0.3), 'back': (0.0, 4.0, 0.3), 'three': (2.5, -3.2, 0.5)}
    for pose in [p for p in poses if p != 'rest']:
        la.pose(arm, la.POSES[pose])
        la.settle_swing(arm, chains(fig), la.POSES[pose])
        for v, off in views.items():
            la.shoot(path(f'clay_{pose}_{v}'), (0.0, 0.0, 0.82 if 'crouch' not in pose else 0.62), off, 85.0, (900, 1200))
        hips = (0.0, -0.05, 0.85 if 'crouch' not in pose else 0.62)
        la.shoot(path(f'clay_{pose}_hips_left'), hips, (1.6, -0.6, 0.1), 85.0, (900, 900))
    if os.environ.get('AUDIT_CLAY_ONLY'):
        la.reset(arm)
        return
    M.viewer_materials(skin, baked)
    la.cycles(int(os.environ.get('AUDIT_SAMPLES', 40)))
    la.light('day')
    la.pose(arm, la.POSES['idle'])
    la.settle_swing(arm, chains(fig), la.POSES['idle'])
    for k in range(8):
        a = math.radians(45.0 * k)
        la.shoot(path(f'turn_{k * 45:03d}'), (0.0, 0.0, 0.88), (math.sin(a) * 5.8, -math.cos(a) * 5.8, 0.35), 85.0)
    pb = arm.pose.bones
    eye = (fig.eyes[0] + fig.eyes[1]) * 0.5
    shots = {
        'face_front': (eye, (0.0, -0.9, 0.0), 100.0, (1200, 1200)),
        'face_three': (eye, (0.55, -0.7, 0.05), 100.0, (1200, 1200)),
        'face_below_brim': (eye, (0.0, -0.75, -0.30), 100.0, (1200, 1200)),
        'hand_l': (tuple(pb['hand_l'].tail), (0.2, -0.7, 0.05), 100.0, (1200, 1200)),
        'hand_r': (tuple(pb['hand_r'].tail), (-0.2, -0.7, 0.05), 100.0, (1200, 1200)),
        'chest': ((0.0, -0.1, 1.25), (0.15, -1.1, 0.05), 85.0, (1200, 1200)),
        'belt_spyglass': ((0.12, -0.05, 0.92), (0.9, -1.2, 0.1), 85.0, (1200, 1200)),
        'boots': ((0.0, -0.05, 0.22), (0.55, -1.4, 0.2), 85.0, (1600, 1200)),
        'pack': ((0.0, 0.2, 1.25), (0.5, 1.6, 0.25), 85.0, (1200, 1400)),
        'collar_back': ((0.0, 0.05, 1.50), (0.0, 1.0, 0.15), 100.0, (1200, 1200)),
    }
    for name, (target, off, lens, size) in shots.items():
        la.shoot(path(name), target, off, lens, size)
    la.light('dusk')
    la.shoot(path('dusk_three'), (0.0, 0.0, 0.88), (3.6, -4.6, 0.4), 85.0)
    la.reset(arm)
    M.log(f'audit: renders and findings in {out}')
