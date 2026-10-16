"""Ellis's hat and head as the game draws them (round 6, 2026-10-15): Ellis.py's build to the rigged game mesh (its
--rig-check path, no bake), then, in place of the rig check, the head from the game's angles (7 sides x 3 heights, two
close-ups) with back faces culled as Unreal culls them (Ellis.py's clay renders draw both sides, so they never showed the
sky through the hat), in loud colours on magenta with the see-through pixels counted inside the hat's outline, and in
roughly the game's colours; the hat tipped as when aiming; sections through the head; and in numbers the skull's
closure, the sweatband's fit, the tipped hat's lift, the brim against hair and collar with the head turned
(HEAD_TESTS). --pivots also sweeps the hat bone's place (ellis_hat.PIVOT).
    Tools\artrun.ps1 -Script Tools\Blender\ellis_hat_check.py -ScriptArgs --rig-check,--tag,<tag>[,--pivots]
into Saved/Concepts/EllisHat/<tag>/.
"""
import math
import os
import sys

REPO = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
for p in (os.path.join(REPO, 'Art', 'Models', 'Characters'), os.path.join(REPO, 'Tools', 'Blender'),
          os.path.join(REPO, 'Art', 'Portraits')):
    if p not in sys.path:
        sys.path.insert(0, p)
import bpy  # noqa: E402
import numpy as np  # noqa: E402
from mathutils import Matrix, Vector  # noqa: E402
from mathutils.bvhtree import BVHTree  # noqa: E402

import Ellis as E  # noqa: E402

gm = E.gm
TAG = E.arg_value('--tag', 'check')
OUT = os.path.join(REPO, 'Saved', 'Concepts', 'EllisHat', TAG)
LOOK = {  # roughly the game's colours (the baked looks' base colours, a little lifted for Workbench)
    'Ellis': (0.62, 0.48, 0.40), 'Eye_': (0.3, 0.85, 1.0), 'HatCrown': (0.26, 0.21, 0.17), 'HatBrim': (0.26, 0.21, 0.17),
    'HatBandInner': (0.16, 0.09, 0.06), 'HatBand': (0.20, 0.11, 0.07), 'Hair': (0.07, 0.055, 0.05),
    'Bandana': (0.60, 0.10, 0.07), 'Collar': (0.33, 0.25, 0.17), 'Cape': (0.33, 0.25, 0.17),
    'Duster': (0.33, 0.25, 0.17), 'SunPin': (0.7, 0.55, 0.25)}
CHECK = {  # loud colours for the hole check (background magenta)
    'Ellis': (0.85, 0.62, 0.50), 'Eye_': (0.3, 0.85, 1.0), 'HatCrown': (0.85, 0.55, 0.15), 'HatBrim': (0.95, 0.85, 0.20),
    'HatBandInner': (0.20, 0.35, 0.95), 'HatBand': (0.75, 0.15, 0.10), 'Hair': (0.20, 0.75, 0.30),
    'Bandana': (0.95, 0.30, 0.30), 'Collar': (0.45, 0.45, 0.50), 'Cape': (0.55, 0.55, 0.60)}


def colour_of(name, table):
    for k in sorted(table, key=len, reverse=True):
        if name.startswith(k):
            return table[k]
    return (0.45, 0.42, 0.40)


def part_objects(fig):
    obs = {}
    for name, (co, faces, _w) in fig.rest.items():
        me = bpy.data.meshes.new('P_' + name)
        me.from_pydata([tuple(p) for p in co], [], [tuple(f) for f in faces])
        me.update()
        ob = bpy.data.objects.new('P_' + name, me)
        bpy.context.scene.collection.objects.link(ob)
        ob['part'] = name
        obs[name] = ob
    return obs


def tip_hat(fig, obs, deg):
    """The hat's parts turned deg on the hat bone (front up), as the game does (rigid: they're 100% hat)."""
    h, t = (np.asarray(x) for x in fig.hat)
    axis = gm.unit(np.cross(gm.unit(t - h), (0.0, 0.0, 1.0)))
    R = np.array(Matrix.Rotation(math.radians(deg), 3, Vector(tuple(axis))))
    for name, ob in obs.items():
        if name.startswith('Hat'):
            co = fig.rest[name][0]
            ob.data.vertices.foreach_set('co', ((co - h) @ R.T + h).astype(np.float32).ravel())
            ob.data.update()


def setup(sc, background):
    sc.render.engine = 'BLENDER_WORKBENCH'
    sc.render.resolution_x, sc.render.resolution_y = 800, 800
    sc.display.render_aa = '8'
    sc.view_settings.view_transform = 'Standard'
    sh = sc.display.shading
    sh.light = 'STUDIO'
    sh.color_type = 'OBJECT'
    sh.show_backface_culling = True              # as Unreal: a face seen from behind isn't drawn
    sh.show_cavity = False
    sh.show_object_outline = False
    sh.show_specular_highlight = False
    world = bpy.data.worlds.get('Probe') or bpy.data.worlds.new('Probe')
    world.color = background
    sc.world = world


def holes(path, box=None):
    """Background pixels enclosed by the figure (not reachable from the picture's edge): see-through holes; within box
    (x0, x1, y0, y1 in pixels from the bottom left) if given."""
    img = bpy.data.images.load(path)
    w, h = img.size
    px = np.array(img.pixels[:], dtype=np.float32).reshape(h, w, 4)
    bpy.data.images.remove(img)
    bg = (px[..., 0] > 0.9) & (px[..., 1] < 0.1) & (px[..., 2] > 0.9)
    reach = np.zeros_like(bg)
    reach[0, :], reach[-1, :], reach[:, 0], reach[:, -1] = bg[0, :], bg[-1, :], bg[:, 0], bg[:, -1]
    for _ in range(3000):
        grow = reach.copy()
        grow[1:] |= reach[:-1]
        grow[:-1] |= reach[1:]
        grow[:, 1:] |= reach[:, :-1]
        grow[:, :-1] |= reach[:, 1:]
        grow &= bg
        if (grow == reach).all():
            break
        reach = grow
    hole = bg & ~reach
    if box is not None:
        x0, x1, y0, y1 = box
        hole = hole[max(0, y0):max(0, y1), max(0, x0):max(0, x1)]
    return int(hole.sum())


def shots(fig, obs, tag, table, background, views, only=None, count_holes=False):
    sc = bpy.context.scene
    setup(sc, background)
    for name, ob in obs.items():
        ob.color = (*colour_of(name, table), 1.0)
        ob.hide_render = only is not None and not name.startswith(only)
    cam = bpy.data.objects.get('ProbeCam')
    if cam is None:
        cam = bpy.data.objects.new('ProbeCam', bpy.data.cameras.new('ProbeCam'))
        sc.collection.objects.link(cam)
    cam.data.lens = 85
    sc.camera = cam
    head = fig.sk['head'][0]
    target = Vector((head[0], head[1], fig.eye_z + 0.03))
    report = []
    for vname, ang, height, dist in views:
        a = math.radians(ang)
        cam.location = target + Vector((math.sin(a) * dist, -math.cos(a) * dist, height))
        cam.rotation_euler = (target - cam.location).to_track_quat('-Z', 'Y').to_euler()
        path = os.path.join(OUT, f'{tag}_{vname}.png')
        sc.render.filepath = path
        bpy.ops.render.render(write_still=True)
        if count_holes:
            # only those inside the hat's outline on the picture (between hair and collar don't count here)
            from bpy_extras.object_utils import world_to_camera_view
            bpy.context.view_layer.update()
            hats = [coords(ob) for n, ob in obs.items() if n.startswith('Hat') and not ob.hide_render]
            box = None
            if hats:
                pts = np.concatenate(hats)
                uv = np.array([world_to_camera_view(sc, cam, Vector(tuple(p)))[:2] for p in pts[::3]])
                W = sc.render.resolution_x
                box = (int(uv[:, 0].min() * W), int(uv[:, 0].max() * W) + 1, int(uv[:, 1].min() * W),
                       int(uv[:, 1].max() * W) + 1)
            report.append(f'{vname} {holes(path, box)}')
    if count_holes:
        E.log(f'hat check {tag}: enclosed see-through pixels: ' + ', '.join(report))


SIDES = (('front', 0), ('q3', 45), ('side', 90), ('bq', 135), ('back', 180), ('rbq', -135), ('rside', -90))
HEIGHTS = (('level', 0.0, 1.2), ('below', -0.55, 1.1), ('above', 0.55, 1.1))
VIEWS = [(f'{h}_{s}', a, z, d) for h, z, d in HEIGHTS for s, a in SIDES] + [('close_q3', 30, 0.05, 0.75),
                                                                           ('close_bq', 150, 0.0, 0.75)]


def section(fig, obs, path, plane_point, plane_normal, axis_u, axis_v, centre, half=(0.17, 0.15), scale=2400):
    """Where each part crosses a plane, drawn: the head (salmon), hair (green), crown (orange), lining (blue), brim
    (yellow), band (red), collar (grey); 1 cm grid."""
    W, H = int(2 * half[0] * scale), int(2 * half[1] * scale)
    img = np.ones((H, W, 4), dtype=np.float32) * np.array([0.1, 0.1, 0.12, 1.0], dtype=np.float32)
    for k in range(-30, 31):
        x = int(W / 2 + k * 0.01 * scale)
        y = int(H / 2 + k * 0.01 * scale)
        if 0 <= x < W:
            img[:, x, :3] = 0.22 if k % 5 else 0.32
        if 0 <= y < H:
            img[y, :, :3] = 0.22 if k % 5 else 0.32
    n = np.asarray(plane_normal, dtype=np.float64)
    p0 = np.asarray(plane_point, dtype=np.float64)
    u, v = np.asarray(axis_u, dtype=np.float64), np.asarray(axis_v, dtype=np.float64)
    layers = []
    if getattr(fig, 'skin_high', None):
        layers.append((np.array([0.45, 0.30, 0.25], dtype=np.float32), fig.skin_high[0], fig.skin_high[1]))
    for name, ob in obs.items():
        if ob.hide_render and not name.startswith(('Ellis', 'Hair', 'Hat')):
            continue
        layers.append((np.array(colour_of(name, CHECK), dtype=np.float32), np.array([vv.co[:] for vv in ob.data.vertices]),
                       [list(p.vertices) for p in ob.data.polygons]))
    for col, co, polys in layers:
        d = (co - p0) @ n
        for ids in polys:
            pts = []
            for k in range(len(ids)):
                a, b = ids[k], ids[(k + 1) % len(ids)]
                if (d[a] > 0) != (d[b] > 0):
                    t = d[a] / (d[a] - d[b])
                    pts.append(co[a] + (co[b] - co[a]) * t)
            if len(pts) < 2:
                continue
            P, Q = pts[0], pts[1]
            for s in np.linspace(0, 1, 24):
                q = P + (Q - P) * s - centre
                x = int(W / 2 + (q @ u) * scale)
                y = int(H / 2 + (q @ v) * scale)
                if 0 <= x < W and 0 <= y < H:
                    img[max(0, y - 1):y + 1, max(0, x - 1):x + 1, :3] = col
    im = bpy.data.images.new(os.path.basename(path), W, H)
    im.pixels.foreach_set(img.ravel())
    im.filepath_raw = path
    im.file_format = 'PNG'
    im.save()
    bpy.data.images.remove(im)


def measure(fig, obs):
    """The band's fit and the skull's closure, in numbers."""
    skin = obs['Ellis']
    co = np.array([v.co[:] for v in skin.data.vertices])
    faces = [list(p.vertices) for p in skin.data.polygons]
    count = {}
    for f in faces:
        for k in range(len(f)):
            e = (min(f[k], f[(k + 1) % len(f)]), max(f[k], f[(k + 1) % len(f)]))
            count[e] = count.get(e, 0) + 1
    rim = [e for e, c in count.items() if c == 1]
    hi = [e for e in rim if co[list(e)][:, 2].mean() > fig.eye_z - 0.02]
    E.log(f'hat check: skin open edges above the eyes: {len(hi)} (of {len(rim)}); '
          f'z {min((co[list(e)][:, 2].mean() for e in hi), default=0):.3f}..{max((co[list(e)][:, 2].mean() for e in hi), default=0):.3f}')
    skin_tree = BVHTree.FromPolygons([tuple(p) for p in co], faces)
    head = fig.sk['head'][0]
    c = np.array([head[0], head[1] + 0.0, fig.eye_z + 0.04])
    miss = 0
    rng = np.random.default_rng(1)
    for _ in range(400):
        d = gm.unit(rng.normal(size=3))
        d[2] = abs(d[2])
        if skin_tree.ray_cast(Vector(tuple(c)), Vector(tuple(d)), 0.3)[0] is None:
            miss += 1
    E.log(f'hat check: rays from inside the skull up and out that leave without hitting the skin: {miss} of 400')
    trees = {k: BVHTree.FromPolygons([tuple(v.co) for v in obs[k].data.vertices], [list(p.vertices) for p in obs[k].data.polygons])
             for k in obs if k.startswith(('Hair', 'Ellis', 'HatCrown', 'HatBand', 'HatBrim', 'HatBandInner'))}
    high_tree = None
    if getattr(fig, 'skin_high', None):
        high_tree = BVHTree.FromPolygons([tuple(p) for p in fig.skin_high[0]], fig.skin_high[1])
    band = np.array([v.co[:] for v in obs['HatBandInner' if 'HatBandInner' in obs else 'HatBand'].data.vertices])
    if getattr(fig, 'hat_fit', None):
        hf = fig.hat_fit
        E.log(f'hat check: fit: crown raised {hf["raise_by"] * 100:.0f}%, crown scale {hf["k"].min():.3f}..{hf["k"].max():.3f}; '
              f'F {np.round(hf["F"] * 100, 1)} B {np.round(hf["B"] * 100, 1)} cm; plane tilt '
              f'{math.degrees(math.atan2(hf["f"][2], -hf["f"][1])):.1f} deg (front up +)')
    bc = band.mean(0)
    ang = np.degrees(np.arctan2(band[:, 0] - bc[0], -(band[:, 1] - bc[1])))
    rows = []
    for a0 in range(-180, 180, 15):
        sel = np.abs(((ang - a0 + 180) % 360) - 180) < 7.5
        if not sel.any():
            continue
        pts = band[sel]
        r = np.linalg.norm(pts[:, :2] - bc[:2], axis=1)
        low = pts[np.argsort(pts[:, 2] + (r - r.min()) * 2.0)[0]]      # the inner bottom edge
        d = np.array([bc[0] - low[0], bc[1] - low[1], 0.0])
        d = gm.unit(d)
        o = Vector(tuple(low))
        hs = skin_tree.ray_cast(o, Vector(tuple(d)), 0.3)
        hd = high_tree.ray_cast(o, Vector(tuple(d)), 0.3) if high_tree else (None,)
        hh = [trees[k].ray_cast(o, Vector(tuple(d)), 0.3) for k in trees if k.startswith('Hair')]
        hh = [x for x in hh if x[0] is not None]
        gs = hs[3] * 100 if hs[0] is not None else float('nan')
        gd = hd[3] * 100 if hd[0] is not None else float('nan')
        gh = min(x[3] for x in hh) * 100 if hh else float('nan')
        rows.append(f'{a0:+4d}:z{low[2] * 100:.1f} skin {gs:.1f} detailed {gd:.1f} hair {gh:.1f}')
    E.log('hat check: band bottom (angle from the front, + his left): height cm, gap to skin / hair cm')
    for k in range(0, len(rows), 4):
        E.log('hat check:   ' + ' | '.join(rows[k:k + 4]))
    cr = obs['HatCrown']
    cco = np.array([v.co[:] for v in cr.data.vertices])
    cent = cco.mean(0)
    inward = sum(1 for p in cr.data.polygons if Vector(tuple(cent - np.array(p.center))).dot(p.normal) > 0)
    E.log(f'hat check: crown faces {len(cr.data.polygons)}, facing inward {inward}; crown z {cco[:, 2].min():.3f}..{cco[:, 2].max():.3f}; '
          f'skull top {co[:, 2].max():.3f}; brows ~ {fig.eye_z + 0.02:.3f}; hat bone {np.round(np.asarray(fig.hat[0]) * 100, 1)} > '
          f'{np.round(np.asarray(fig.hat[1]) * 100, 1)} cm')
    if getattr(fig, 'hat_fit', None) and 'HatBandInner' in obs:
        s = fig.hat_fit
        rel = cco - s['O']
        h = rel @ s['u']
        a = np.degrees(np.arctan2(rel @ s['x'], rel @ s['f']))
        feet = [h[np.abs(((a - a0 + 180) % 360) - 180) < 10].min() * 100 for a0 in range(-180, 180, 30)]
        E.log('hat check: the crown\'s foot over the band plane by angle (cm): ' + ' '.join(f'{x:.1f}' for x in feet))
        fa = np.sort(np.unique(np.round(a[h < 0.002], 1)))
        gaps = np.diff(np.concatenate([fa, fa[:1] + 360]))
        E.log(f'hat check: the crown\'s foot has {len(fa)} points round, the widest step {gaps.max():.0f} degrees')
        ctree = BVHTree.FromPolygons([tuple(v.co) for v in cr.data.vertices], [list(p.vertices) for p in cr.data.polygons])
        sw = coords(obs['HatBandInner'])
        dmin = min(ctree.find_nearest(Vector(tuple(p)), 0.1)[3] for p in sw)
        E.log(f'hat check: the sweatband\'s nearest to the crown\'s felt {dmin * 1000:.1f} mm')
    tris = {}
    for k, ob in obs.items():
        key = k.split('.')[0]
        if key.startswith('Hat'):
            tris[key] = sum(len(p.vertices) - 2 for p in ob.data.polygons)
    E.log(f'hat check: hat triangles {tris}, total {sum(tris.values())}')


def rotate_parts(fig, obs, names, pivot, axis, deg, base=None):
    R = np.array(Matrix.Rotation(math.radians(deg), 3, Vector(tuple(axis))))
    for name in names:
        co = fig.rest[name][0] if base is None else base[name]
        obs[name].data.vertices.foreach_set('co', ((co - pivot) @ R.T + pivot).astype(np.float32).ravel())
        obs[name].data.update()


def coords(ob):
    return np.array([v.co[:] for v in ob.data.vertices])


def brim_collar(fig, obs, tag):
    """The brim's least distance to the collar (cm) and its points inside the collar's shell, the hat as posed."""
    col = obs['Collar']
    tree = BVHTree.FromPolygons([tuple(v.co) for v in col.data.vertices], [list(p.vertices) for p in col.data.polygons])
    brim = coords(obs['HatBrim'])
    best = 1.0
    for p in brim:
        loc, _n, _i, d = tree.find_nearest(Vector(tuple(p)), 0.5)
        if loc is not None:
            best = min(best, d)
    return f'{tag}: brim to collar {best * 100:.1f} cm'


def foot_gaps(fig, obs):
    """How far the sweatband's foot stands off the detailed head (cm), by side, as posed."""
    if not getattr(fig, 'skin_high', None):
        return ''
    tree = BVHTree.FromPolygons([tuple(p) for p in fig.skin_high[0]], fig.skin_high[1])
    inner = coords(obs['HatBandInner'])
    s = fig.hat_seat
    foot = inner[len(inner) // 2:]                       # gm.grid's rows top down: the second row is the foot
    rest = fig.rest['HatBandInner'][0][len(inner) // 2:]
    rel = rest - s['O']
    ang = np.degrees(np.arctan2(rel @ s['x'], rel @ s['f']))
    d = []
    for p in foot:
        loc, nrm, _i, dist = tree.find_nearest(Vector(tuple(p)), 0.5)
        d.append((Vector(tuple(p)) - loc).dot(nrm))
    d = np.array(d) * 100
    out = []
    for lo, hi, nm in ((0, 30, 'front'), (30, 75, 'temple'), (75, 120, 'side'), (120, 150, 'backq'), (150, 181, 'back')):
        sel = (np.abs(ang) >= lo) & (np.abs(ang) < hi)
        if sel.any():
            out.append(f'{nm} {d[sel].min():.1f}-{d[sel].max():.1f}')
    return ', '.join(out)


def probe(fig, arm, skin):
    os.makedirs(OUT, exist_ok=True)
    skin.hide_render = True
    for ob in bpy.context.scene.objects:
        if ob.type == 'MESH':
            ob.hide_render = True
    obs = part_objects(fig)
    measure(fig, obs)
    head = fig.sk['head'][0]
    band = np.array([v.co[:] for v in obs['HatBand'].data.vertices])
    bc = band.mean(0)
    section(fig, obs, os.path.join(OUT, 'section_mid.png'), (0.0, 0, 0), (1.0, 0, 0), (0, -1.0, 0), (0, 0, 1.0), bc)
    section(fig, obs, os.path.join(OUT, 'section_across.png'), (0, bc[1], 0), (0, 1.0, 0), (1.0, 0, 0), (0, 0, 1.0), bc)
    shots(fig, obs, 'check', CHECK, (1.0, 0.0, 1.0), VIEWS, count_holes=True)
    shots(fig, obs, 'look', LOOK, (0.55, 0.62, 0.70), [v for v in VIEWS if v[0].startswith(('level', 'below', 'close'))])
    shots(fig, obs, 'nohair', CHECK, (1.0, 0.0, 1.0), [v for v in VIEWS if v[0] in ('level_back', 'level_bq', 'below_back',
                                                                                  'below_bq', 'level_side', 'below_front')],
          only=('Ellis', 'Eye_', 'Hat'), count_holes=True)
    shots(fig, obs, 'skull', CHECK, (1.0, 0.0, 1.0), [('above_back', 180, 0.55, 1.1), ('above_side', 90, 0.55, 1.1),
                                                      ('above_front', 0, 0.55, 1.1)], only=('Ellis', 'Eye_'), count_holes=True)
    tip = float(E.arg_value('--tip', E.HAT_TIP))
    tip_hat(fig, obs, tip)
    shots(fig, obs, 'tipped', LOOK, (0.55, 0.62, 0.70), [('level_side', 90, 0.0, 1.2), ('level_front', 0, 0.0, 1.2),
                                                         ('level_q3', 45, 0.0, 1.2), ('below_front', 0, -0.55, 1.1),
                                                         ('level_back', 180, 0.0, 1.2)])
    shots(fig, obs, 'tipped_check', CHECK, (1.0, 0.0, 1.0), [('level_side', 90, 0.0, 1.2), ('below_front', 0, -0.55, 1.1),
                                                             ('below_q3', 45, -0.55, 1.1), ('level_back', 180, 0.0, 1.2)],
          count_holes=True)
    section(fig, obs, os.path.join(OUT, 'section_mid_tipped.png'), (0.0, 0, 0), (1.0, 0, 0), (0, -1.0, 0), (0, 0, 1.0), bc)
    E.log(f'hat check: the hat tipped {tip:.0f}: {E.hat_contact(fig, tip)}')
    if 'HatBandInner' in obs:
        E.log(f'hat check: tipped, the sweatband\'s foot off the head (cm): {foot_gaps(fig, obs)}')
    hat_names = [n for n in obs if n.startswith('Hat')]
    tipped = {n: coords(obs[n]) for n in hat_names}
    rotate_parts(fig, obs, hat_names, np.zeros(3), (0, 0, 1), 0.0)
    if 'HatBandInner' in obs:
        E.log(f'hat check: at rest, the sweatband\'s foot off the head (cm): {foot_gaps(fig, obs)}')
    pivot = np.asarray(fig.sk['head'][0])
    res = [brim_collar(fig, obs, 'rest')]
    for name, axis, deg in E.HEAD_TESTS + (('pitch_up', (1.0, 0.0, 0.0), -20.0),):
        rotate_parts(fig, obs, hat_names, pivot, axis, deg)
        res.append(brim_collar(fig, obs, name))
        rotate_parts(fig, obs, hat_names, pivot, axis, deg, base=tipped)
        res.append(brim_collar(fig, obs, name + '+tip'))
    rotate_parts(fig, obs, hat_names, np.zeros(3), (0, 0, 1), 0.0)
    E.log('hat check: head turned (and with the hat tipped): ' + '; '.join(res))
    if '--pivots' in E.ARGS and 'HatBandInner' in obs:
        s = fig.hat_seat
        keep = fig.hat
        axis = gm.unit(np.cross(s['f'], (0.0, 0.0, 1.0)))
        edge = fig.rest['HatBrim'][0]
        rel = edge - s['O']
        ea = np.degrees(np.arctan2(rel @ s['x'], rel @ s['f']))
        er = np.linalg.norm(rel[:, :2], axis=1)
        picks = [int(np.argmax(er * (np.abs(ea - a0) < 4))) for a0 in (0.0, -30.0, -60.0)]
        sweep = [(dy, dz) for dy in (0.0, 0.02, 0.03, 0.04, 0.05) for dz in (0.0, -0.03, -0.04, -0.05, -0.06)]
        for dy, dz in sweep:
            pv = np.asarray(keep[0]) + s['f'] * dy + s['u'] * dz
            fig.hat = (pv, pv + s['f'] * 0.2)
            rotate_parts(fig, obs, hat_names, pv, axis, tip)
            now = coords(obs['HatBrim'])
            rise = ' '.join(f'{(now[i, 2] - edge[i, 2]) * 100:.1f}' for i in picks)
            E.log(f'hat check: pivot fwd {dy * 100:.0f} down {-dz * 100:.0f} cm: brim edge rises (front, right-front, '
                  f'right) {rise} cm; foot {foot_gaps(fig, obs)}; {E.hat_contact(fig, tip)}')
        fig.hat = keep
        rotate_parts(fig, obs, hat_names, np.zeros(3), (0, 0, 1), 0.0)
    E.log(f'hat check: written to {OUT}')


_make_lows = E.make_lows


def make_lows(fig, highs, hair_under):
    for ob in highs:
        if ob.name == 'Ellis':
            co = gm._object_coords(ob)
            d = np.minimum(np.linalg.norm(co - fig.eyes[0], axis=1), np.linalg.norm(co - fig.eyes[1], axis=1))
            tris = sum(len(p.vertices) - 2 for p in ob.data.polygons)
            E.log(f'hat check: the detailed skin {len(co)} vertices, {tris} triangles; within 3 cm of an eye '
                  f'{int((d < 0.03).sum())} vertices; above the eyes {int((co[:, 2] > fig.eye_z).sum())}')
            fig.skin_high = (co, [list(p.vertices) for p in ob.data.polygons])
    lows = _make_lows(fig, highs, hair_under)
    for ob in lows:
        if ob.name.startswith('Ellis'):
            co = gm._object_coords(ob)
            E.log(f'hat check: the game skin {len(co)} vertices, {sum(len(p.vertices) - 2 for p in ob.data.polygons)} '
                  f'triangles; above the eyes {int((co[:, 2] > fig.eye_z).sum())} vertices')
    return lows


E.make_lows = make_lows
E.rig_check = probe
E.main()
