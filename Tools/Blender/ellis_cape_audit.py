"""Ellis's shoulder cape and turned-up collar checked in the game's poses (round 6, the user: "Cape clippings ... don't
stop until you get it right", 2026-10-15). Ellis.py is built and rigged WITHOUT its bake (the texture sets on disk are
never touched: any edit to Ellis.py changes the bake's digest), then, per pose (looter_audit.POSES: the game's idle,
the gun carried low, aimed level, high and low, gripped, a crouch, a stride, a run's arm swing, the head turned, up and
down), the cape and collar against every layer they meet and close clay views of the shoulders, arms and collar from
the front, three-quarter, side and back, each part tinted apart and every face's back culled (the game's materials
are one-sided: a sheet seen from behind is a hole).

    Tools\\artrun.ps1 -Script Tools\\Blender\\ellis_cape_audit.py -ScriptArgs --audit,<tag>[,--poses,idle+aim]
    into Saved/Concepts/EllisRound6/cape/<tag>/: findings.json, <pose>_<view>.png, sheet_<pose>.png, holes_<pose>.png
    (down into the collar against a magenta sky: magenta between the neck and the collar is a hole), sheet_weights.png
    (the cape coloured by what it rides: red the arms, blue its chains, green the chest and shoulders)

Checks (cm): crossing faces between two parts (BVH overlap: a real cut, either way; those out of the collar's ring
counted apart); points of an inner layer out through the one over it (ellis_audit.PAIRS and the cape's own); cape
points inside the sleeves' tubes round the upper arms; how near the brim comes to the collar.
Env: CAPE_POSES (a+b), CAPE_SHOW (only these parts drawn, a+b), CAPE_RENDERS=0 (numbers only), CAPE_SWEEP
('f=0.2,0.3;bulk=4,6;cap=9': the springs' cape shares, the cape's bulk, the upper arms' cap, tried without renders),
CAPE_FOLLOW ('a=1.0;f=0.2'), CAPE_KEEPUNDER=strict, CAPE_MIRROR=r5 (BodySwing as round 5 shipped it), CAPE_SRC (another
version of Ellis.py, run as if it were the one in the repo), CAPE_PROFILE (the parts' radii round the neck),
CAPE_OPEN_WHERE (where the crossings out of the collar's ring are)."""
import json
import math
import os
import sys

import bpy
import numpy as np
from mathutils import Vector
from mathutils.bvhtree import BVHTree

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
for p in (HERE, os.path.join(REPO, 'Art', 'Models', 'Characters')):
    if p not in sys.path:
        sys.path.insert(0, p)

import looter_audit as la  # noqa: E402
import ellis_audit as ea  # noqa: E402

POSES = ('rest', 'idle', 'carry', 'aim', 'aim_high', 'aim_low', 'grip_a', 'crouch', 'crouch_aim', 'stride', 'swing',
         'look_l', 'look_r', 'look_up', 'look_down', 'look_up_tip')
# Crossing faces between two parts (either may cut the other)
CROSS = (('Cape', 'Sleeves'), ('Cape', 'Duster'), ('Cape', 'Collar'), ('Cape', 'Hair'), ('Cape', 'Bandana'),
         ('Cape', 'Gauntlet'), ('Cape', 'Gloves'), ('Collar', 'HatBrim'), ('Collar', 'HatCrown'), ('Collar', 'Hair'),
         ('Collar', 'Bandana'), ('Collar', 'Ellis'), ('Collar', 'Duster'), ('Collar', 'Sleeves'), ('SunPin', 'Bandana'),
         ('SunPin', 'HatBrim'))
# Points of the first through the second (looter_audit.poke_through), beyond ellis_audit.PAIRS
PAIRS = (('Sleeves', 'Cape'), ('Duster', 'Cape'), ('Cape', 'Collar'), ('Bandana', 'Collar'), ('Hair', 'Collar'),
         ('Ellis', 'Collar'), ('Duster', 'Collar'), ('Gauntlet', 'Cape'))
TINT = (('Cape', (0.86, 0.55, 0.24)), ('Collar', (0.96, 0.82, 0.36)), ('SunPin', (0.9, 0.9, 0.2)),
        ('Duster', (0.52, 0.49, 0.45)), ('Sleeves', (0.36, 0.55, 0.86)), ('Gauntlet', (0.70, 0.62, 0.52)),
        ('Gloves', (0.78, 0.68, 0.56)), ('Concho', (0.9, 0.8, 0.3)), ('BandanaKnot', (0.85, 0.22, 0.16)),
        ('Bandana', (0.85, 0.22, 0.16)), ('Hair', (0.22, 0.30, 0.22)), ('Hat', (0.45, 0.31, 0.20)),
        ('Ellis', (0.86, 0.76, 0.70)), ('Eye', (0.3, 0.8, 1.0)), ('Vest', (0.28, 0.28, 0.30)),
        ('Shirt', (0.8, 0.8, 0.75)))


def tint_of(name):
    for prefix, rgb in TINT:
        if name.startswith(prefix):
            return rgb
    return (0.6, 0.6, 0.6)


def posed_all(fig, arm, pose):
    """{part name: (posed points, faces)} for every part, the swing chains settled as the game's springs rest them."""
    la.pose(arm, la.POSES[pose])
    la.settle_swing(arm, ea.chains(fig), la.POSES[pose])
    mats = la.skinning(arm, fig.rig_names)
    return {n: (la.deform(c, W, mats), f) for n, (c, f, W) in fig.rest.items()}


def joined(parts, prefix):
    names = ['Ellis'] if prefix == 'Ellis' else [n for n in parts if n.split('.')[0].startswith(prefix)]
    co, faces, off = [], [], 0
    for n in names:
        c, f = parts[n]
        co.append(c)
        faces += [[i + off for i in p] for p in f]
        off += len(c)
    return (np.concatenate(co), faces) if co else None


def crossings(a, b, neck=None, collar=None):
    """Face pairs of a and b that cut each other, and where (the mean of the cut faces' centres, cm); with neck (the
    posed neck_01 head), also how many lie out of the collar's cover (below its foot, or out past its wall)."""
    ta = BVHTree.FromPolygons([tuple(map(float, p)) for p in a[0]], a[1])
    tb = BVHTree.FromPolygons([tuple(map(float, p)) for p in b[0]], b[1])
    pairs = ta.overlap(tb)
    if not pairs:
        return (0, None) if neck is None else (0, None, 0)
    cen = np.array([a[0][a[1][i]].mean(0) for i, _ in pairs])
    where = [round(float(v) * 100, 1) for v in cen.mean(0)]
    if neck is None:
        return len(pairs), where
    if collar is None:
        out = (cen[:, 2] < neck[2] - 0.035) | (np.linalg.norm(cen[:, :2] - neck[:2], axis=1) > 0.18)
    else:
        # covered: inside the collar's ring (nearer the neck's axis than its wall) and no lower than its foot
        tc = BVHTree.FromPolygons([tuple(map(float, p)) for p in collar[0]], collar[1])
        out = np.zeros(len(cen), dtype=bool)
        for i, c in enumerate(cen):
            d = Vector((c[0] - neck[0], c[1] - neck[1], 0.0))
            if d.length < 1e-6:
                continue
            hit = tc.ray_cast(Vector(tuple(c)) + d.normalized() * 0.002, d.normalized(), 0.25)
            out[i] = hit[0] is None
    if out.any() and os.environ.get('CAPE_OPEN_WHERE'):
        pts = cen[out] * 100
        print(f'ELLIS open crossings: {len(pts)}, z {pts[:, 2].min():.1f}..{pts[:, 2].max():.1f}, x {pts[:, 0].min():.1f}..'
              f'{pts[:, 0].max():.1f}, y {pts[:, 1].min():.1f}..{pts[:, 1].max():.1f}', flush=True)
    return len(pairs), where, int(out.sum())


def inside_arm(fig, arm, cape, sleeves):
    """Cape points inside the sleeves' tube round each upper arm (from a fifth of the way down it to the elbow; the
    shoulder's top is the cape's own), and the deepest (cm)."""
    tree = BVHTree.FromPolygons([tuple(map(float, p)) for p in sleeves[0]], sleeves[1])
    pb = arm.pose.bones
    n, deep, where = 0, 0.0, []
    for s in 'lr':
        a, b = np.array(pb[f'upperarm_{s}'].head), np.array(pb[f'lowerarm_{s}'].head)
        ab = b - a
        t = np.clip(((cape[0] - a) @ ab) / (ab @ ab), 0.0, 1.0)
        q = a + t[:, None] * ab
        d = np.linalg.norm(cape[0] - q, axis=1)
        for i in np.nonzero((t > 0.2) & (d < 0.14))[0]:
            hit = tree.ray_cast(Vector(tuple(q[i])), Vector(tuple((cape[0][i] - q[i]) / max(d[i], 1e-9))), 0.25)
            if hit[0] is not None and hit[3] > d[i] + 0.002:
                n += 1
                deep = max(deep, hit[3] - d[i])
                where.append(cape[0][i])
    return n, deep * 100, ([round(float(v) * 100, 1) for v in np.mean(where, 0)] if where else None)


def shares(fig, W):
    """Per row of a weight matrix (fig.rig_names' columns): the upper arms', the cape chains' and the rest's shares."""
    names = fig.rig_names
    arm = [j for j, n in enumerate(names) if n.startswith(('upperarm_', 'lowerarm_'))]
    chain = [j for j, n in enumerate(names) if n.startswith('cape_')]
    return W[:, arm].sum(1), W[:, chain].sum(1)


def diag_cross(fig, parts, pose, log):
    """What the cape's points that cut the sleeves ride: their upper-arm and chain shares, their heights (cm)."""
    co, faces = parts['Cape']
    sl = joined(parts, 'Sleeves')
    ta = BVHTree.FromPolygons([tuple(map(float, p)) for p in co], faces)
    tb = BVHTree.FromPolygons([tuple(map(float, p)) for p in sl[0]], sl[1])
    pairs = ta.overlap(tb)
    idx = sorted({v for i, _ in pairs for v in faces[i]})
    if not idx:
        return
    # which part of which sleeve is cut: its points' biggest bone
    Ws = np.concatenate([fig.rest[n][2] for n in fig.rest if n.split('.')[0].startswith('Sleeves')])
    sv = sorted({v for _, j in pairs for v in sl[1][j]})
    names = fig.rig_names
    big = [names[k] for k in np.argmax(Ws[sv], 1)]
    count = {}
    for b in big:
        key = b.replace('_twist_01', '').replace('_twist_02', '')
        count[key] = count.get(key, 0) + 1
    log(f'cape diag {pose}: the sleeves cut at ' + ', '.join(f'{k} {v}' for k, v in sorted(count.items(), key=lambda kv: -kv[1])))
    W = fig.rest['Cape'][2][idx]
    a, c = shares(fig, W)
    rest = fig.rest['Cape'][0][idx]
    log(f'cape diag {pose}: {len(idx)} cutting points: arm share {a.min():.2f}..{a.max():.2f} (mean {a.mean():.2f}), '
        f'chains {c.min():.2f}..{c.max():.2f} (mean {c.mean():.2f}), rest heights {rest[:, 2].min() * 100:.1f}..'
        f'{rest[:, 2].max() * 100:.1f} cm, x {rest[:, 0].min() * 100:.1f}..{rest[:, 0].max() * 100:.1f}')


def check(fig, arm, parts, pose):
    found = []
    neck = np.array(arm.pose.bones['neck_01'].head)
    for a, b in CROSS:
        pa, pb = joined(parts, a), joined(parts, b)
        if pa is None or pb is None:
            continue
        n, where, out = crossings(pa, pb, neck, joined(parts, 'Collar'))
        if n:
            # (the 'deep' place holds how many are out in the open, not under the collar)
            found.append(('cross', a, b, n, float(out), where))
    bones = (np.array([tuple(arm.pose.bones[b].head) for b in ea.BONES]),
             np.array([tuple(arm.pose.bones[b].tail) for b in ea.BONES]))
    for inner, outer in PAIRS:
        a, b = joined(parts, inner), joined(parts, outer)
        if a is None or b is None:
            continue
        idx, deep = la.poke_through(a, b, outward=la.away_from_bones(a[0], bones))
        if len(idx):
            found.append(('through', inner, outer, int(len(idx)), float(deep.max() * 100),
                          [round(float(v) * 100, 1) for v in a[0][idx].mean(0)]))
    n, deep, where = inside_arm(fig, arm, joined(parts, 'Cape'), joined(parts, 'Sleeves'))
    if n:
        found.append(('inside', 'Cape', 'Sleeves', n, deep, where))
    # how near the brim comes to the collar (cm; the HAT agent's bar: 2.7 or more with the head turned and tipped)
    brim, collar = joined(parts, 'HatBrim'), joined(parts, 'Collar')
    if brim is not None and collar is not None:
        tb = BVHTree.FromPolygons([tuple(map(float, p)) for p in brim[0]], brim[1])
        best, at = 1.0, None
        for p in collar[0]:
            loc, _n, _i, d = tb.find_nearest(Vector(tuple(p)), 0.2)
            if loc is not None and d < best:
                best, at = d, p
        if at is not None:
            found.append(('gap', 'HatBrim', 'Collar', 0, best * 100, [round(float(v) * 100, 1) for v in at]))
    return found


def objects_for(fig):
    """A clay object per part (its faces from the rest, its points set per pose), tinted apart."""
    obs = {}
    for name, (co, faces, _W) in fig.rest.items():
        me = bpy.data.meshes.new('_Cape_' + name)
        me.from_pydata([tuple(map(float, p)) for p in co], [], [tuple(f) for f in faces])
        me.update()
        ob = bpy.data.objects.new('_Cape_' + name, me)
        bpy.context.scene.collection.objects.link(ob)
        ob.color = (*tint_of(name), 1.0)
        show = os.environ.get('CAPE_SHOW')        # only these parts (prefixes, '+' between), to see what's under
        ob.hide_render = bool(show) and not name.startswith(tuple(show.split('+')))
        obs[name] = ob
    return obs


def weight_views(fig, arm, obs, out, log):
    """The cape at the idle coloured by what it rides (red the arms, blue its chains, green the chest and shoulders),
    the other parts their tints: weights_<view>.png."""
    sc = bpy.context.scene
    for name, ob in obs.items():
        if name == 'Cape':
            a, c = shares(fig, fig.rest[name][2])
            rgb = np.stack([a, np.clip(1.0 - a - c, 0.0, 1.0), c], 1)
        else:
            rgb = np.tile(np.array(tint_of(name)), (len(ob.data.vertices), 1))
        attr = ob.data.color_attributes.get('Look') or ob.data.color_attributes.new('Look', 'FLOAT_COLOR', 'POINT')
        attr.data.foreach_set('color', np.concatenate([rgb, np.ones((len(rgb), 1))], 1).astype(np.float32).ravel())
        ob.data.color_attributes.active_color = attr
    place(obs, posed_all(fig, arm, 'idle'))
    sc.display.shading.color_type = 'VERTEX'
    pb = arm.pose.bones
    chest = np.array(pb['spine_05'].head)
    paths = [la.shoot(os.path.join(out, f'weights_{v}.png'), tuple(chest), off, 60.0, (800, 800))
             for v, off in {'front': (0.0, -1.25, 0.12), 'three': (0.8, -0.96, 0.25), 'side': (-1.25, 0.0, 0.12),
                            'back': (0.0, 1.25, 0.25)}.items()]
    sheet(paths, os.path.join(out, 'sheet_weights.png'), cols=4, scale=0.5)
    sc.display.shading.color_type = 'OBJECT'


def place(obs, parts):
    for name, ob in obs.items():
        co = parts[name][0]
        ob.data.vertices.foreach_set('co', co.astype(np.float32).ravel())
        ob.data.update()


def sheet(paths, out, cols=4, scale=0.5):
    """The renders side by side on one picture (Blender's images, no other tool)."""
    imgs = []
    for p in paths:
        im = bpy.data.images.load(p)
        w, h = im.size
        px = np.empty(w * h * 4, dtype=np.float32)
        im.pixels.foreach_get(px)
        px = px.reshape(h, w, 4)
        step = int(round(1 / scale))
        imgs.append(px[::step, ::step])
        bpy.data.images.remove(im)
    h = max(i.shape[0] for i in imgs)
    w = max(i.shape[1] for i in imgs)
    rows = (len(imgs) + cols - 1) // cols
    canvas = np.ones((rows * h, cols * w, 4), dtype=np.float32)
    for k, im in enumerate(imgs):
        r, c = k // cols, k % cols
        y0 = (rows - 1 - r) * h                      # Blender's pixels run bottom up
        canvas[y0 + h - im.shape[0]:y0 + h, c * w:c * w + im.shape[1]] = im
    img = bpy.data.images.new('_sheet', canvas.shape[1], canvas.shape[0], alpha=True)
    img.pixels.foreach_set(canvas.ravel())
    img.filepath_raw = out
    img.file_format = 'PNG'
    img.save()
    bpy.data.images.remove(img)


def sweep(fig, arm, poses, spec, log):
    """Tries the game's cape follow shares (looter_audit.CAPE_FOLLOW) and limb caps over the poses without renders:
    spec 'a=0.6,0.8;s=0.85,1.0;cap=9,12;bulk=0,2' (cap: the upper arms' clearance cap, bulk: the cape's, cm). Logs each try's cuts."""
    import itertools
    keys, values = [], []
    for part in spec.split(';'):
        k, v = part.split('=')
        keys.append(k)
        values.append([float(x) for x in v.split(',')])
    limbs = list(la.LIMBS)
    for combo in itertools.product(*values):
        for k, v in zip(keys, combo):
            if k == 'cap':
                la.LIMBS = tuple((a, b, v if a.startswith('upperarm_') else m) for a, b, m in limbs)
            elif k == 'bulk':
                la.BULK['cape'] = v / 100.0                  # (cm) the cape's chains keep this much more off every limb
            else:
                la.CAPE_FOLLOW[k] = v
        cut, inside, hands = 0, 0, 0
        per = []
        for pose in poses:
            parts = posed_all(fig, arm, pose)
            n, _ = crossings(joined(parts, 'Cape'), joined(parts, 'Sleeves'))
            m = inside_arm(fig, arm, joined(parts, 'Cape'), joined(parts, 'Sleeves'))[0]
            h = sum(crossings(joined(parts, 'Cape'), joined(parts, p))[0] for p in ('Gauntlet', 'Gloves'))
            cut, inside, hands = cut + n, inside + m, hands + h
            per.append(f'{pose} {n}/{m}/{h}')
        log(f'cape sweep {dict(zip(keys, combo))}: cut {cut}, inside {inside}, hands {hands} ({", ".join(per)})')
    la.LIMBS = tuple(limbs)


def profile(fig, log, parts=('Ellis', 'Bandana', 'Hair', 'Duster', 'Cape', 'Collar', 'Sleeves', 'Vest', 'Shirt')):
    """Round the neck at the collar's heights: each part's outermost radius from the neck's axis (cm) every 30 degrees
    (0 the front, + the figure's left), at rest: how close the collar can sit."""
    from mathutils.bvhtree import BVHTree
    neck = fig.neck
    trees = {}
    for p in parts:
        j = joined({n: (c, f) for n, (c, f, _w) in fig.rest.items()}, p)
        if j is not None:
            trees[p] = BVHTree.FromPolygons([tuple(map(float, q)) for q in j[0]], j[1])
    for dz in (-0.03, -0.01, 0.01, 0.03, 0.05, 0.07, 0.09, 0.11):
        z = neck[2] + dz
        row = []
        for deg in range(0, 360, 30):
            a = math.radians(deg)
            d = Vector((math.sin(a), -math.cos(a), 0.0))
            cells = []
            for p, t in trees.items():
                # from outside in: the first hit is the part's outermost surface there
                o = Vector((neck[0], neck[1], z)) + d * 0.4
                hit = t.ray_cast(o, -d, 0.4)
                cells.append(f'{p[:2]}{(0.4 - hit[3]) * 100:4.1f}' if hit[0] is not None else f'{p[:2]}  - ')
            row.append(f'{deg:3d}: ' + ' '.join(cells))
        log(f'profile neck{dz * 100:+.0f} cm:\n    ' + '\n    '.join(row))


def facing(fig, log, parts=('Cape', 'Collar', 'Duster', 'Bandana', 'HatBrim')):
    """How each thick part's faces look: out from the neck's axis or in, by layer (the outer layer should look out,
    the inner in: the game's materials are one-sided)."""
    c = fig.neck
    for p in parts:
        j = joined({n: (co, f) for n, (co, f, _w) in fig.rest.items()}, p)
        if j is None:
            continue
        co, faces = j
        tree = BVHTree.FromPolygons([tuple(map(float, q)) for q in co], faces)
        bad, where = 0, []
        for f in faces:
            pts = co[f]
            n = np.cross(pts[1] - pts[0], pts[-1] - pts[0])
            if np.linalg.norm(n) < 1e-12:
                continue
            n = Vector(tuple(n / np.linalg.norm(n)))
            o = Vector(tuple(pts.mean(0)))
            # a face looking into its own solid meets its other side within the thickness along its normal
            if tree.ray_cast(o + n * 1e-4, n, 0.008)[0] is not None and tree.ray_cast(o - n * 1e-4, -n, 0.008)[0] is None:
                bad += 1
                where.append(pts.mean(0))
        log(f'facing {p}: {bad} of {len(faces)} faces look into the part (its back shows outside)'
            + (f', about {np.round(np.mean(where, 0) * 100, 1)} cm' if where else ''))


def run(fig, arm, skin, baked, tag, viewer_materials, log, repo):
    facing(fig, log)
    for n in ('BandanaKnot', 'Hair'):
        if n in fig.rest:
            co = fig.rest[n][0]
            r = np.linalg.norm(co[:, :2] - fig.neck[:2], axis=1)
            back = co[np.abs(np.degrees(np.arctan2(co[:, 0] - fig.neck[0], -(co[:, 1] - fig.neck[1])))) > 85]
            log(f'{n}: z {co[:, 2].min() * 100:.1f}..{co[:, 2].max() * 100:.1f} cm, {r.min() * 100:.1f}..{r.max() * 100:.1f} '
                f'cm from the neck axis; behind 85 degrees its lowest {back[:, 2].min() * 100 if len(back) else 0:.1f} cm')
    if os.environ.get('CAPE_PROFILE'):
        profile(fig, log)
        return
    out = os.path.join(repo, 'Saved', 'Concepts', 'EllisRound6', 'cape', tag)
    os.makedirs(out, exist_ok=True)
    want = os.environ.get('CAPE_POSES', '')
    if '--poses' in sys.argv:
        want = sys.argv[sys.argv.index('--poses') + 1]
    poses = [p for p in POSES if not want or p in want.split('+')]
    for part in filter(None, os.environ.get('CAPE_FOLLOW', '').split(';')):
        k, v = part.split('=')
        la.CAPE_FOLLOW[k] = float(v)
    if os.environ.get('CAPE_MIRROR') == 'r5':
        # the game's springs as round 5 shipped them (BodySwing today): the cape's front chains take 0.6 of the upper
        # arm's turn and any other 0.4, from the bind pose's arm (the side the name's 7th letter), KeepUnder level
        os.environ['CAPE_KEEPUNDER'] = 'strict'
        la.cape_follow = lambda chain: ((f'upperarm_{"l" if chain[6] == "l" else "r"}', 0.6 if chain[5] == 'f' else 0.4)
                                        if chain.startswith('cape_') and len(chain) >= 7 else None)
        la.IDLE_ARM_BONE = 'no_such_bone_{}'
        la.CAPE_FOLLOW = {'f': 0.6, 'b': 0.4}
        la.BULK.pop('cape', None)
    if os.environ.get('CAPE_KEEPUNDER') == 'strict':
        # round 5's KeepUnder (level with the head at most), to compare
        strict = la._keep_under
        la._keep_under = lambda head, tip, length, target_z=None: strict(head, tip, length)
    log(f'cape audit: follow {la.CAPE_FOLLOW}, KeepUnder {os.environ.get("CAPE_KEEPUNDER", "to the target")}')
    if os.environ.get('CAPE_SWEEP'):
        sweep(fig, arm, poses, os.environ['CAPE_SWEEP'], log)
        return
    renders = os.environ.get('CAPE_RENDERS', '1') != '0'
    la.clay()
    sc = bpy.context.scene
    sc.world = bpy.data.worlds.get('Preview') or bpy.data.worlds.new('Preview')
    sc.world.color = (0.30, 0.33, 0.38)
    sc.display.shading.show_backface_culling = True     # the game's one-sided materials: a sheet's back is a hole
    skin.hide_render = True
    for ob in arm.children:
        if ob.name.startswith(('USP_', 'UCP_')):
            ob.hide_render = True
    obs = objects_for(fig)
    report = {}
    tally = {}
    for pose in poses:
        parts = posed_all(fig, arm, pose)
        found = check(fig, arm, parts, pose)
        diag_cross(fig, parts, pose, log)
        report[pose] = found
        for kind, a, b, n, deep, where in found:
            if kind == 'gap':
                log(f'cape audit {pose}: the brim comes within {deep:.1f} cm of the collar, at {where} cm')
                key = 'brim to collar, nearest (cm)'
                tally[key] = round(min(tally.get(key, 99.0), deep), 1)
                continue
            log(f'cape audit {pose}: {a} {kind} {b}: {n} {"face pairs" if kind == "cross" else "points"}'
                + (f', {deep:.1f} cm deep at most' if kind != 'cross' else f' ({int(deep)} out from under the collar)')
                + f', about {where} cm')
            key = f'{a} {kind} {b}'
            tally[key] = tally.get(key, 0) + n
            if kind == 'cross':
                tally[key + ' (open)'] = tally.get(key + ' (open)', 0) + int(deep)
        if not renders:
            continue
        place(obs, parts)
        pb = arm.pose.bones
        chest = np.array(pb['spine_05'].head) + np.array((0.0, 0.0, 0.02))
        neck = np.array(pb['head'].head)
        d = 1.25
        views = {'front': (0.0, -d, 0.12), 'three': (d * 0.64, -d * 0.77, 0.25), 'side': (-d, 0.0, 0.12),
                 'back': (0.0, d, 0.25), 'bthree': (-d * 0.6, d * 0.8, 0.45)}
        paths = []
        for v, off in views.items():
            paths.append(la.shoot(os.path.join(out, f'{pose}_{v}.png'), tuple(chest), off, 60.0, (800, 800)))
        for side, sx in (('r', -1.0), ('l', 1.0)):
            # the shoulder's top by the collar's foot, where the sleeve's top lies under the cape's
            target = np.array(pb[f'clavicle_{side}'].tail) * 0.6 + np.array(pb['neck_01'].head) * 0.4 + np.array((0, 0, 0.07))
            paths.append(la.shoot(os.path.join(out, f'{pose}_shoulder_{side}.png'), tuple(target), (sx * 0.35, -0.1, 0.3),
                                  50.0, (800, 800)))
        for hand, off in (('hand_r', (-0.15, -0.6, 0.1)), ('hand_l', (0.15, -0.6, 0.1))):
            # the hands and forearms by the cape's front panels
            target = (np.array(pb[hand].head) + np.array(pb['spine_05'].head)) * 0.5
            paths.append(la.shoot(os.path.join(out, f'{pose}_{hand}.png'), tuple(target), off, 50.0, (800, 800)))
        for v, off in {'collar_back': (-0.42, 0.62, 0.30), 'collar_side': (0.75, -0.05, 0.05),
                       'collar_below': (0.35, -0.55, -0.35)}.items():
            paths.append(la.shoot(os.path.join(out, f'{pose}_{v}.png'), tuple(neck - np.array((0, 0, 0.06))), off,
                                  60.0, (800, 800)))
        # looking down into the collar against a magenta sky: any magenta between the neck and the collar is a hole
        # (a back face, culled as the game's one-sided materials are)
        sc.world.color = (0.9, 0.05, 0.75)
        holes = []
        for v, off in {'down_back': (-0.18, 0.42, 0.62), 'down_front': (0.42, -0.5, 0.5), 'down_side': (0.62, 0.12, 0.5),
                       'down_top': (0.04, 0.16, 0.75)}.items():
            holes.append(la.shoot(os.path.join(out, f'{pose}_{v}.png'), tuple(neck - np.array((0, 0, 0.06))), off,
                                  50.0, (800, 800)))
        sc.world.color = (0.30, 0.33, 0.38)
        sheet(paths, os.path.join(out, f'sheet_{pose}.png'), cols=4, scale=0.5)
        sheet(holes, os.path.join(out, f'holes_{pose}.png'), cols=4, scale=0.5)
    if renders:
        weight_views(fig, arm, obs, out, log)
    with open(os.path.join(out, 'findings.json'), 'w') as f:
        json.dump({'poses': report, 'totals': tally}, f, indent=1)
    log('cape audit totals: ' + '; '.join(f'{k} {v}' for k, v in sorted(tally.items())))
    for ob in obs.values():
        me = ob.data
        bpy.data.objects.remove(ob)
        bpy.data.meshes.remove(me)
    skin.hide_render = False
    la.reset(arm)
    log(f'cape audit: renders and findings in {out}')


def main():
    src = os.environ.get('CAPE_SRC')
    if src:
        # another version of the model's script (an older one to compare), run as if it were Ellis.py where it lives
        import types
        real = os.path.join(REPO, 'Art', 'Models', 'Characters', 'Ellis.py')
        Ellis = types.ModuleType('Ellis')
        Ellis.__file__ = real
        sys.modules['Ellis'] = Ellis
        with open(src) as f:
            code = compile(f.read(), real, 'exec')
        exec(code, Ellis.__dict__)
    else:
        import Ellis
    # never bake (the texture sets on disk stay as committed): the clay needs no looks
    Ellis.bake_all = lambda lows, highs, size: None
    ea.run = run
    Ellis.main()


if __name__ == '__main__':
    main()
