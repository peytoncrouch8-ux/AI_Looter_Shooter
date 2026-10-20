"""SK_Odessa: Odessa Lark, the Cardsharp, a playable hero (Docs/Story.md "The heroes"), built from the portrait the user
picked on 2026-10-19: option A, The Dealer, of Art/Portraits/Odessa.py (its OPTIONS["A"], FACE, MACRO and builders are
the spec; the painted bust Saved/ArtPreviews/Portraits/Odessa/OdessaA.png). Boots to hat:
  - the body: MakeHuman's CC0 base shaped as the portrait's slight woman about thirty, 1.72 m barefoot, standing 3.5 cm
    up on riding heels; her arms a little longer than MakeHuman's (ARM_STRETCH: her span her height, and the hands reach
    where the mannequin's do); the portrait's face: high brows, the half smile drawn up at her left corner, the lids a
    little lowered over eyes that glow cyan
  - the portrait's flat-crowned black gambler with a wine silk band and the ace of spades tucked in it (a plain pip)
  - dark hair combed back and pinned in a knot at the nape; gold drop earrings
  - a white starched high collar and a black silk cravat (a band, a knot, its fall over the waistcoat, a gold stick pin)
  - a gold brocade waistcoat closing under the cravat, four gold buttons, a watch chain to the pocket on her right
  - the wine-red tailcoat: fitted, open in front with darker silk lapels and a rolled collar, cut away at the waist in a
    curve back to long tails (to the back of the knee, split up the back to the waist seam, two gold buttons at its top),
    fitted sleeves with darker turned-back cuffs over the shirt's white cuffs
  - black gloves with short flared cuffs; charcoal trousers tucked into tall black riding boots with riding heels
  - a slim hip belt with a gold buckle, the oxblood card case at her right hip
The card in her hand stays in the portrait: her hands hold the game's guns.
Rig: the UE5 mannequin's 63 deform bones on MakeHuman's joints, the forearms' twist bones, the tails' six swing chains
(skirt_sl, bl, bli, bri, br, sr: each tail's outer edge, middle and inner edge, each ending in an unweighted _end) and
`hat`, which the game tips back while aiming. Built facing -Y, exported turned to face -X: +Y in Unreal, as the mannequin.
Ellis.py is the worked example (its notes and lessons: Docs/Pipeline.md "Character models"); the builders are in
Tools/Blender/odessa_*.py.

    blender -b --factory-startup --python Art/Models/Characters/Odessa.py -- [--shape] [--stats] [--preview]
        [--shots full+upper+face+back] [--rig-check] [--audit <tag>] [--rebake] [--size 2048]
or Tools\\artrun.ps1 -Script Art\\Models\\Characters\\Odessa.py -Threads 6 -ScriptArgs --preview
"""
import importlib.util
import math
import os
import sys

import bpy  # first: as a Python module bmesh and mathutils come with it
import bmesh
import numpy as np
from mathutils import Matrix, Vector

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(os.path.dirname(HERE)))
for p in (os.path.join(REPO, 'Tools', 'Blender'), os.path.join(REPO, 'Art', 'Portraits')):
    if p not in sys.path:
        sys.path.insert(0, p)
import bust  # noqa: E402
import looter_bake as bk  # noqa: E402
import looter_garments as gm  # noqa: E402
import looter_human as hu  # noqa: E402
import looter_model as lm  # noqa: E402
import looter_textures as lt  # noqa: E402
import mh  # noqa: E402
import odessa_kit as kit  # noqa: E402
import odessa_looks as looks  # noqa: E402
import odessa_head as head  # noqa: E402
import odessa_clothes as cl  # noqa: E402
import odessa_coat as cm  # noqa: E402
from odessa_kit import ARGS, arg_value, log, slot  # noqa: E402

PORTRAIT = os.path.join(REPO, 'Art', 'Portraits', 'Odessa.py')
HEIGHT = 1.72            # barefoot, the top of the head (m)
LIFT = 0.035             # the riding heels and soles raise the body this much
ARM_STRETCH = 1.065      # her upper arms and forearms this much longer than MakeHuman's (0.479 m -> 0.51, Ellis's)


def load_portrait():
    """Art/Portraits/Odessa.py as a module of its own name (it shares this script's): the spec's numbers, builders."""
    spec = importlib.util.spec_from_file_location('OdessaPortrait', PORTRAIT)
    mod = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(mod)
    return mod


# ------------------------------------------------------------------------------------------------------ the body

def shape_face(fig, op, opt):
    """The portrait's face touches that come after mh.build (MakeHuman space, at rest): the high brows (its
    LATE_TARGETS) and the half smile (op.smirk: her left mouth corner drawn up and back)."""
    h = fig.b.h
    for rel, w in op.LATE_TARGETS.items():
        ix, d = mh.load_target(rel)
        h['verts'][ix] += d * w
        h['morphed'][ix] += d * w
    amount = opt.get('smirk', 0.0)
    if amount > 0:
        rest = h['morphed']
        w_oris = sum(fig.b.mh_weight(b) for b in h['bones'] if b.startswith('oris'))
        lips = rest[w_oris > 0.3]
        corner = lips[np.argmax(lips[:, 0])]
        # (the portrait's falloff, 2.2 cm, creased the cheek into a line that read as a scar in the game's light:
        # wider and softer here, the corner raised as far)
        d = np.linalg.norm((rest - corner) * np.array([1.0, 1.3, 1.0]), axis=1)
        k = np.clip(1.0 - d / 0.32, 0, 1) ** 1.3 * amount * 0.8
        move = k[:, None] * np.array([0.015, 0.04, -0.015])
        h['verts'] += move
        h['morphed'] += move


def stretch_arms(body, s):
    """Lengthens each upper arm and forearm by s along its own bone (MakeHuman space, its vertices by their weights,
    its joints with them): the elbow moves out by the upper arm's share, the wrist and hand by both."""
    h = body.h
    bones = h['bones']
    for S in 'LR':
        A = bones[f'upperarm01.{S}']['head'].copy()
        B = bones[f'lowerarm01.{S}']['head'].copy()
        C = bones[f'wrist.{S}']['head'].copy()
        dU, dL = (B - A) * (s - 1.0), (C - B) * (s - 1.0)
        hand = [n for n in bones if n == f'wrist.{S}' or _under(bones, n, f'wrist.{S}')]
        upper = (f'upperarm01.{S}', f'upperarm02.{S}')
        lower = (f'lowerarm01.{S}', f'lowerarm02.{S}')
        tu = lambda p: np.clip(((p - A) @ (B - A)) / ((B - A) @ (B - A)), 0.0, 1.0)
        tl = lambda p: np.clip(((p - B) @ (C - B)) / ((C - B) @ (C - B)), 0.0, 1.0)
        wu = sum(body.mh_weight(b) for b in upper)
        wl = sum(body.mh_weight(b) for b in lower)
        wh = sum(body.mh_weight(b) for b in hand)
        v = h['verts']
        disp = (wu * tu(v))[:, None] * dU + wl[:, None] * (dU + tl(v)[:, None] * dL) + wh[:, None] * (dU + dL)
        h['verts'] = v + disp
        h['morphed'] = h['morphed'] + disp
        for name in list(bones):
            for end in ('head', 'tail'):
                p = bones[name][end]
                if name in upper:
                    q = p + tu(p[None])[0] * dU
                elif name in lower:
                    q = p + dU + tl(p[None])[0] * dL
                elif name in hand:
                    q = p + dU + dL
                else:
                    continue
                bones[name] = dict(bones[name], **{end: q})


def _under(bones, name, root):
    p = bones[name]['parent']
    while p:
        if p == root:
            return True
        p = bones[p]['parent']
    return False


class Figure:
    """The shaped body and everything measured from it that the clothes are fitted to."""

    def __init__(self):
        self.op = load_portrait()
        self.opt = self.op.OPTIONS['A']
        face = {**self.op.FACE, **self.opt.get('face', {})}
        self.b = hu.Body(self.op.MACRO, face, HEIGHT, LIFT)
        h = self.b.h
        shape_face(self, self.op, self.opt)
        stretch_arms(self.b, ARM_STRETCH)
        self.balls = bust.close_lids(h, self.opt['lids'])
        self.hf = bust.head_frame(h)
        self.wv = self.b.world_verts()
        self.names, self.Wv = self.b.vertex_weights()
        self.col = {n: i for i, n in enumerate(self.names)}
        W = lambda *bones: sum(self.b.mh_weight(x) for x in bones)
        sides = ('L', 'R')
        self.w_arm = sum(W(f'shoulder01.{s}', f'upperarm01.{s}', f'upperarm02.{s}', f'lowerarm01.{s}', f'lowerarm02.{s}')
                         for s in sides)
        self.w_hand = sum(W(f'wrist.{s}', *[f'finger{k}-{i}.{s}' for k in range(1, 6) for i in (1, 2, 3)],
                            *[f'metacarpal{k}.{s}' for k in range(1, 5)]) for s in sides)
        self.w_head = self.Wv[:, self.col['head']] + self.Wv[:, self.col['neck_02']] + 0.6 * self.Wv[:, self.col['neck_01']]
        self.w_leg = sum(self.Wv[:, self.col[f'{p}_{s}']] for p in ('thigh', 'calf', 'foot', 'ball') for s in 'lr')
        self.w_foot = sum(self.Wv[:, self.col[f'{p}_{s}']] for p in ('foot', 'ball') for s in 'lr')
        self.faces = self.b.body_faces
        self.fuv = [self.b.h['uv'][fu] for fu, g in zip(self.b.h['fuv'], self.b.h['groups']) if g and g.startswith('body')]
        sk = self.b.skeleton()
        self.sk = sk
        self.neck = sk['neck_01'][0]
        self.shoulder_z = sk['clavicle_l'][1][2]
        self.hip = sk['pelvis'][0][2]
        used = self.b.used
        torso = used[(self.w_arm[used] < 0.3) & (self.w_hand[used] < 0.1) & (self.w_head[used] < 0.3)]
        tz = self.wv[torso]
        chest = tz[(tz[:, 2] > self.shoulder_z - 0.20) & (tz[:, 2] < self.shoulder_z)]
        self.centre = np.array([0.0, (chest[:, 1].min() + chest[:, 1].max()) * 0.5])
        self.torso_idx = torso
        # the natural waist: the torso's narrowest, between the hips and the ribs
        best = None
        for z in np.arange(self.hip + 0.06, self.hip + 0.26, 0.005):
            sl = tz[np.abs(tz[:, 2] - z) < 0.006]
            sl = sl[np.abs(sl[:, 0]) < 0.25]
            if len(sl) > 8:
                wdt = sl[:, 0].max() - sl[:, 0].min()
                if best is None or wdt < best[0]:
                    best = (wdt, z)
        self.waist = float(best[1])
        g = self.head_point
        self.eyes = [g(e) for e in self.hf['eyes']]
        self.eye_z = (self.eyes[0][2] + self.eyes[1][2]) * 0.5
        hv = self.hf['verts']
        nose = hv[np.argmax(hv[:, 2])]
        self.mouth_mh = nose + np.array([0.0, -0.37, -0.20])
        ankle, ball = sk['foot_l'][0], sk['ball_l'][0]
        f = gm.unit(np.array([ball[0] - ankle[0], ball[1] - ankle[1], 0.0]))
        self.ball_s = float((ball - ankle) @ f)
        feet = self.wv[used][self.w_foot[used] > 0.3]
        toe_y = float(feet[:, 1].min()) - 0.03
        kit.BOOT_CAP = (toe_y + 0.05, 0.0205)            # the boots' toe cap stitched 5 cm behind the tip, the welt
        arm = np.linalg.norm(sk['upperarm_l'][1] - sk['upperarm_l'][0]) + np.linalg.norm(sk['lowerarm_l'][1] - sk['lowerarm_l'][0])
        log(f'body: {len(used)} vertices, scale {self.b.scale:.3f}, eyes at {self.eye_z:.3f} m, waist {self.waist:.3f}, '
            f'hips {self.hip:.3f}, arm {arm:.3f} m (shoulder to wrist), chest centre y {self.centre[1]:.3f}')

    def head_point(self, p):
        return self.b.to_world(np.asarray(p, dtype=np.float64)[None])[0]

    def face_mask(self, vert_mask):
        return [bool(vert_mask[f].all()) for f in self.faces]

    def to_portrait(self, ob):
        """Moves an object the portrait built (MakeHuman metres, no size or lift) into this figure's space."""
        s = self.b.scale
        ob.matrix_world = Matrix.Translation((0.0, 0.0, -s * 0.1 * self.b.floor + LIFT)) @ Matrix.Scale(s, 4) @ ob.matrix_world

    def head_tree(self):
        return head.skin_tree(self)


# ------------------------------------------------------------------------------------------------------ previews

SHOTS = {'full': (0.90, 4.2, 0.25), 'upper': (1.40, 1.7, 0.06), 'face': (1.60, 0.85, 0.02), 'feet': (0.22, 1.5, 0.35),
         'hips': (0.92, 1.8, 0.1)}
VIEWS = (('front', 0.0), ('three', 35.0), ('side', 90.0), ('back', 180.0))


def preview(path, views=VIEWS, size=900, samples=24, aim=SHOTS['full']):
    """Quick Cycles renders round the figure (shape checks in clay with --shape)."""
    sc = bpy.context.scene
    sc.render.engine = 'CYCLES'
    sc.cycles.device = 'CPU'
    sc.cycles.samples = samples
    sc.cycles.use_denoising = True
    sc.render.resolution_x = int(size * 0.75)
    sc.render.resolution_y = size
    sc.render.film_transparent = False
    try:
        sc.view_settings.view_transform = 'AgX'
    except Exception:
        pass
    world = bpy.data.worlds.get('Preview') or bpy.data.worlds.new('Preview')
    sc.world = world
    world.use_nodes = True
    world.node_tree.nodes['Background'].inputs['Color'].default_value = (0.32, 0.36, 0.42, 1.0)
    world.node_tree.nodes['Background'].inputs['Strength'].default_value = 0.6
    if 'Sun' not in bpy.data.objects:
        ld = bpy.data.lights.new('Sun', 'SUN')
        ld.energy = 3.5
        ld.angle = math.radians(8)
        lo = bpy.data.objects.new('Sun', ld)
        sc.collection.objects.link(lo)
        lo.rotation_euler = (math.radians(50), 0.0, math.radians(-35))
    if '--parts' in ARGS:
        # each part its own colour (Workbench), to tell the layers apart
        sc.render.engine = 'BLENDER_WORKBENCH'
        sc.display.render_aa = '8'
        sh = sc.display.shading
        sh.light, sh.color_type, sh.show_cavity, sh.cavity_type = 'STUDIO', 'RANDOM', True, 'WORLD'
    elif '--shape' in ARGS:
        clay = bpy.data.materials.get('_Clay') or bpy.data.materials.new('_Clay')
        clay.diffuse_color = (0.55, 0.52, 0.48, 1.0)
        bpy.context.view_layer.material_override = clay
    cd = bpy.data.cameras.new('PreviewCam')
    cd.lens = 70
    cam = bpy.data.objects.new('PreviewCam', cd)
    sc.collection.objects.link(cam)
    sc.camera = cam
    out = []
    for name, yaw in views:
        a = math.radians(yaw)
        target = Vector((0.0, 0.0, aim[0]))
        cam.location = target + Vector((math.sin(a) * aim[1], -math.cos(a) * aim[1], aim[2]))
        cam.rotation_euler = (target - cam.location).to_track_quat('-Z', 'Y').to_euler()
        sc.render.filepath = f'{path}_{name}.png'
        bpy.ops.render.render(write_still=True)
        out.append(sc.render.filepath)
    bpy.data.objects.remove(cam)
    return out


# ------------------------------------------------------------------------------------------- the game mesh

# (the hat's parts too: their cylindrical map laid the flat brim on a line, all its texels along one stripe)
SMART_UV = ('Eye_', 'Soles', 'HatCrown', 'HatBrim', 'HatBand', 'HatCard', 'VestButton', 'CoatButton', 'StickPin',
            'CravatKnot', 'Earring', 'EarDrop', 'Buckle', 'CardCase', 'WatchFob', 'Knot', 'Gauntlet', 'ShirtCuff',
            'CoatCuff')
# The game mesh's share of each detailed part's triangles (Decimate, collapse; 1 keeps it whole): the face, hands, hat
# brim, the coat's edges and the collar get the triangles; small hard parts stay whole (reduced, they shred).
LOW = {'Odessa': 1.0, 'Eye': 0.35, 'Hair': 0.12, 'Knot': 0.25, 'Earring': 0.6, 'EarDrop': 0.5, 'HatCrown': 0.09,
       'HatBrim': 0.22, 'HatBand': 0.6, 'HatBandInner': 1.0, 'HatCard': 1.0, 'Coat': 0.16, 'CoatTails': 0.15,
       'Lapel': 1.0, 'CoatCollar': 0.24, 'CoatButton': 0.5, 'CoatCuff': 0.4, 'ShirtCuff': 0.4, 'Gauntlet': 0.45,
       'Gloves': 0.36, 'Vest': 0.35, 'VestButton': 0.35, 'Shirt': 0.5, 'Collar': 0.35, 'CravatBand': 0.3,
       'CravatKnot': 0.6, 'CravatFall': 1.0, 'StickPin': 0.6, 'WatchFob': 0.6, 'Trousers': 0.35, 'Boots': 0.2,
       'Soles': 0.7, 'Belt': 0.45, 'Buckle': 1.0, 'CardCase': 0.6}
kit.LOW.update(LOW)
kit.EDGE_KEEP.update({'HatBrim': 0.010, 'Collar': 0.008, 'CravatBand': 0.006, 'Hair': 0.004})
# Sheets given thickness after the bake (more are set by their builders): the crown's felt inward, the brim both ways
kit.THICK.update({'HatBrim': 0.005, 'HatCrown': 0.004, 'HatBand': 0.0015, 'Gauntlet_l': 0.0025, 'Gauntlet_r': 0.0025})


def low_ratio(name):
    key = name.split('.')[0]
    for k in sorted(kit.LOW, key=len, reverse=True):
        if key.startswith(k):
            return kit.LOW[k]
    return 1.0


def near_open_edge(ob, co, band):
    from mathutils import kdtree
    count = {}
    for p in ob.data.polygons:
        vs = list(p.vertices)
        for k in range(len(vs)):
            e = (min(vs[k], vs[(k + 1) % len(vs)]), max(vs[k], vs[(k + 1) % len(vs)]))
            count[e] = count.get(e, 0) + 1
    rim = sorted({v for e, c in count.items() if c == 1 for v in e})
    if not rim:
        return np.zeros(len(co), dtype=bool)
    tree = kdtree.KDTree(len(rim))
    for i, v in enumerate(rim):
        tree.insert(co[v], i)
    tree.balance()
    return np.array([tree.find(p)[2] < band for p in co])


def unflip(low, high):
    """The faces Decimate turned over turned back to face as the detailed part does there, its slivers dissolved
    (Unreal draws a face from its front only: each was a hole; Ellis.py's)."""
    tree = gm.bvh_objects([high])
    bm = bmesh.new()
    bm.from_mesh(low.data)
    bmesh.ops.dissolve_degenerate(bm, dist=1e-6, edges=bm.edges[:])
    bm.normal_update()
    bad = []
    for f in bm.faces:
        loc, nrm, _i, _d = tree.find_nearest(f.calc_center_median(), 0.05)
        if loc is None or f.normal.dot(nrm) >= 0.0:
            continue
        around = Vector()
        for e in f.edges:
            for g in e.link_faces:
                if g is not f:
                    around += g.normal * g.calc_area()
        if around.length == 0.0 or f.normal.dot(around) < 0.0:
            bad.append(f)
    if bad:
        bmesh.ops.reverse_faces(bm, faces=bad)
    bm.to_mesh(low.data)
    bm.free()
    low.data.update()


# Reduced layers pushed back outside the reduced layers under them (outer, its unders, the gap)
CLEAR = (('Vest', ('Shirt',), 0.003), ('CravatFall', ('Shirt', 'Vest'), 0.002), ('CravatBand', ('Collar',), 0.002),
         ('Coat', ('Vest', 'Shirt', 'ShirtCuff'), 0.004), ('CoatTails', ('Vest', 'Belt', 'Buckle', 'Trousers', 'CardCase'), 0.004),
         ('Coat', ('CoatTails',), 0.002), ('Lapel', ('Coat',), 0.002), ('CoatCollar', ('Collar', 'CravatBand', 'Coat'), 0.003),
         ('CoatCuff', ('Coat',), 0.002), ('ShirtCuff', ('Gauntlet',), 0.002), ('Gauntlet', ('Gloves',), 0.002),
         ('Boots', ('Trousers',), 0.003), ('Belt', ('Trousers', 'Vest'), 0.003), ('Collar', ('Odessa',), 0.004))


def part_base(name):
    """A part's name without _low, Blender's .001 and the _l/_r of a side."""
    key = name.split('.')[0]
    key = key[:-4] if key.endswith('_low') else key
    return key[:-2] if key.endswith(('_l', '_r')) else key


def make_lows(fig, highs):
    """The game mesh: a reduced copy of every detailed part (named <part>_low until the detailed ones go): the head by
    region, the cloth's open edges spared; parts built apart for the game (CUSTOM_LOW) and bake-only sources skipped."""
    lows = []
    for ob in highs:
        key = ob.name.split('.')[0]
        if key in kit.CUSTOM_LOW:
            made = kit.CUSTOM_LOW[key]
            if made is not None:
                made.name = ob.name + '_low'
                made.hide_render = False
                lows.append(made)
            continue
        if key.startswith('_'):
            continue
        low = bpy.data.objects.new(ob.name + '_low', ob.data.copy())
        bpy.context.scene.collection.objects.link(low)
        low.matrix_world = ob.matrix_world
        kit.bake_transform(low)
        if key == 'Shirt':
            # only the shirt in the waistcoat's V shows
            bm = bmesh.new()
            bm.from_mesh(low.data)
            nz = fig.neck[2]
            hw = lambda z: cl.VEST_VEE * max(0.0, 1.0 - (nz - z) / cl.VEST_DEPTH) ** 0.8 + 0.02
            hidden = [f for f in bm.faces if (lambda q: abs(q.x) > hw(q.z))(f.calc_center_median())]
            bmesh.ops.delete(bm, geom=hidden, context='FACES')
            bm.to_mesh(low.data)
            bm.free()
        ratio = low_ratio(ob.name)
        if key == 'Odessa':
            before, after = head.decimate_skin(fig, low)
            log(f'skin: {before} -> {after} triangles by region')
            unflip(low, ob)
        elif ratio < 1.0:
            m = low.modifiers.new('decimate', 'DECIMATE')
            m.ratio = ratio
            co = gm._object_coords(low)
            band = next((v for k, v in sorted(kit.EDGE_KEEP.items(), key=lambda kv: -len(kv[0])) if key.startswith(k)), None)
            spare = near_open_edge(low, co, band) if band else np.zeros(len(co), dtype=bool)
            if spare.any():
                g = low.vertex_groups.new(name='keep')
                for i in np.nonzero(spare)[0]:
                    g.add([int(i)], 1.0, 'REPLACE')
                m.vertex_group = 'keep'
                m.invert_vertex_group = True
                m.vertex_group_factor = 1.0
            gm.apply_modifiers(low, keep=())
            if 'keep' in low.vertex_groups:
                low.vertex_groups.remove(low.vertex_groups['keep'])
            unflip(low, ob)
        lows.append(low)
    by = lambda *names: [o for o in lows if part_base(o.name) in names]
    for outer, inner, gap in CLEAR:
        under = by(*inner)
        if not under:
            continue
        tree = gm.bvh_objects(under)
        for ob in by(outer):
            co = gm._object_coords(ob)
            new = gm.clear(co, tree, gap, 0.02)
            ob.data.vertices.foreach_set('co', new.astype(np.float32).ravel())
            ob.data.update()
    return lows


def look_of(ob):
    return ob.material_slots[0].material['LookKey'] if ob.material_slots and ob.material_slots[0].material else None


def prepare_uvs(objects):
    """Every part gets a usable 'UVMap': MakeHuman's on the shells, arc lengths on the grids, smart projection on the
    small hard parts. ('Strand' layers stay: the hair's look reads them.)"""
    for ob in objects:
        me = ob.data
        if ob.name.startswith(SMART_UV) or 'UVMap' not in me.uv_layers:
            for layer in list(me.uv_layers):
                if layer.name != 'Strand':
                    me.uv_layers.remove(layer)
            bk.smart_uv(ob)
        me.uv_layers.active = me.uv_layers['UVMap']
        me.uv_layers['UVMap'].active_render = True


BAKE_SOURCES = [os.path.abspath(__file__)] + [os.path.join(REPO, *p) for p in (
    ('Tools', 'Blender', 'looter_human.py'), ('Tools', 'Blender', 'looter_garments.py'), ('Tools', 'Blender', 'looter_looks.py'),
    ('Tools', 'Blender', 'looter_bake.py'), ('Art', 'Portraits', 'mh.py'), ('Art', 'Portraits', 'bust.py'),
    ('Art', 'Portraits', 'Odessa.py'), ('Tools', 'Blender', 'odessa_kit.py'), ('Tools', 'Blender', 'odessa_looks.py'),
    ('Tools', 'Blender', 'odessa_head.py'), ('Tools', 'Blender', 'odessa_clothes.py'),
    ('Tools', 'Blender', 'odessa_coat.py'))]


def bake_all(lows, highs, size):
    """Packs every texture set's UVs on the game mesh and bakes it from the detailed parts; skipped when the sets on
    disk were baked from these same scripts at this size (Art/Textures/OdessaSkin/bake.digest; --rebake forces it)."""
    stamp = os.path.join(lt.TEXTURE_DIR, 'OdessaSkin', 'bake.digest')
    want = bk.digest(BAKE_SOURCES, f'size={size} blender={bpy.app.version_string}')
    have = open(stamp).read().strip() if os.path.exists(stamp) else ''
    out = {}
    for set_name, keys in looks.TEXTURE_SETS.items():
        obs = [o for o in lows if look_of(o) in keys]
        src = [o for o in highs if look_of(o) in keys]
        weights = {o: 1.8 for o in obs if o.name.startswith(('HatCard', 'Gloves', 'CravatKnot', 'StickPin'))}
        bk.pack(obs, weights=weights)
        if have == want and '--rebake' not in ARGS:
            out[set_name] = bk.load_set(set_name)
        if out.get(set_name) is None:
            out[set_name] = bk.bake_set(set_name, obs, size, sources=src, hide=highs)
            log(f'baked {set_name} ({len(obs)} parts from {len(src)})')
        else:
            log(f'{set_name}: kept (baked from these scripts before)')
    os.makedirs(os.path.dirname(stamp), exist_ok=True)
    with open(stamp, 'w') as f:
        f.write(want + '\n')
    return out


def thicken(objects):
    for ob in objects:
        t = kit.THICK.get(ob.name.split('.')[0])
        if t:
            off = 0.0 if ob.name.startswith('HatBrim') else -1.0 if ob.name.startswith('HatCrown') else 1.0
            gm.solidify(ob, t, offset=off, rim=True)


def game_materials(skin):
    """The looks swapped for the game's: a textured material per GAME_MATERIALS slot (MI_<Slot> on its master, the maps
    of the set its looks bake into), Glow for the glows."""
    swap = {}
    for slot_name, (master, keys, params) in looks.GAME_MATERIALS.items():
        sets = {s for s, ks in looks.TEXTURE_SETS.items() if set(keys) & set(ks)}
        assert len(sets) == 1, f'{slot_name}: its looks must bake into one set, not {sets}'
        mat = lt.material(sets.pop(), name=slot_name, master=master, **params)
        for k in keys:
            swap[f'Odessa_{k}'] = mat
    for k, (hex_value, strength) in looks.GLOWS.items():
        swap[f'Odessa_{k}'] = lm.material(f'Odessa{k}', hex_value, 'Glow', Glow=strength)
    for i, s_ in enumerate(skin.material_slots):
        if s_.material is not None and s_.material.name in swap:
            skin.material_slots[i].material = swap[s_.material.name]
    bk.select([skin])
    bpy.ops.object.material_slot_remove_unused()


def viewer_materials(skin, baked):
    """The baked maps on a Principled BSDF (base colour, normal, roughness and metal from the ORM, its occlusion),
    emission for the glows: the previews' and the audit's look."""
    mats = {}
    for set_name, keys in looks.TEXTURE_SETS.items():
        imgs = baked[set_name]['images']
        mat = bpy.data.materials.new(f'View_{set_name}')
        mat.use_nodes = True
        nt = mat.node_tree
        nt.nodes.clear()
        out = nt.nodes.new('ShaderNodeOutputMaterial')
        b = nt.nodes.new('ShaderNodeBsdfPrincipled')
        nt.links.new(b.outputs[0], out.inputs['Surface'])
        col = nt.nodes.new('ShaderNodeTexImage')
        col.image = imgs['COLOR']
        orm = nt.nodes.new('ShaderNodeTexImage')
        orm.image = imgs['ORM']
        sep = nt.nodes.new('ShaderNodeSeparateColor')
        nt.links.new(orm.outputs['Color'], sep.inputs[0])
        # occlusion into the colour (the viewer has no other place for it)
        ao = nt.nodes.new('ShaderNodeMix')
        ao.data_type, ao.blend_type = 'RGBA', 'MULTIPLY'
        ao.inputs[0].default_value = 0.8
        nt.links.new(col.outputs['Color'], ao.inputs[6])
        nt.links.new(sep.outputs[0], ao.inputs[7])
        nt.links.new(ao.outputs[2], b.inputs['Base Color'])
        nt.links.new(sep.outputs[1], b.inputs['Roughness'])
        nt.links.new(sep.outputs[2], b.inputs['Metallic'])
        nrm = nt.nodes.new('ShaderNodeTexImage')
        nrm.image = imgs['NORMAL']
        nm = nt.nodes.new('ShaderNodeNormalMap')
        nt.links.new(nrm.outputs['Color'], nm.inputs['Color'])
        nt.links.new(nm.outputs['Normal'], b.inputs['Normal'])
        if set_name == 'OdessaSkin':
            b.inputs['Subsurface Weight'].default_value = 0.15
            b.inputs['Subsurface Radius'].default_value = (1.0, 0.45, 0.35)
            b.inputs['Subsurface Scale'].default_value = 0.004
        if set_name in ('OdessaCoat', 'OdessaGarb'):
            b.inputs['Sheen Weight'].default_value = 0.4
        for k in keys:
            mats[f'Odessa_{k}'] = mat
    for k in looks.GLOWS:
        mats[f'Odessa_{k}'] = bpy.data.materials.get(f'Odessa_{k}')
    for i, s_ in enumerate(skin.material_slots):
        if s_.material is not None and s_.material.name in mats:
            skin.material_slots[i].material = mats[s_.material.name]


VIEW_POSE = {'upperarm_l': ((0.0, 1.0, 0.0), 34.0), 'upperarm_r': ((0.0, 1.0, 0.0), -34.0),
             'lowerarm_l': ((1.0, 0.0, 0.0), 14.0), 'lowerarm_r': ((1.0, 0.0, 0.0), 14.0)}


def pose_for_view(arm):
    """A relaxed stance for the previews: the arms let down from the bind pose's A toward the sides."""
    from mathutils import Quaternion
    for bone, (axis, deg) in VIEW_POSE.items():
        pb = arm.pose.bones[bone]
        m = pb.bone.matrix_local.to_3x3()
        local_axis = m.inverted() @ Vector(axis)
        pb.rotation_mode = 'QUATERNION'
        pb.rotation_quaternion = Quaternion(local_axis.normalized(), math.radians(deg))
    bpy.context.view_layer.update()


def face_like_mannequin(arm, skin):
    """The finished figure turned a quarter about Z into its data (bones' rest, vertices, the hit zones), the last
    step before the export: in Unreal SK_Odessa then faces +Y with her left side at +X, as SKM_Manny_Simple does."""
    turn = Matrix.Rotation(math.radians(-90.0), 4, 'Z')
    bk.select([arm], arm)
    arm.data.transform(turn)
    skin.data.transform(turn)
    for ob in arm.children:
        if ob is not skin:
            ob.matrix_world = turn @ ob.matrix_world
    bpy.context.view_layer.update()
    toe = arm.data.bones['ball_l'].head_local - arm.data.bones['foot_l'].head_local
    log(f'turned to face -X here (+Y in Unreal): foot_l to ball_l {tuple(round(c, 3) for c in toe.normalized())}')


def fetch_makehuman():
    if not os.path.exists(os.path.join(mh.DATA, '3dobjs', 'base.obj')):
        import subprocess
        log('fetching MakeHuman\'s CC0 assets (Art/ThirdParty/MakeHuman/fetch.py)')
        subprocess.run([sys.executable, os.path.join(REPO, 'Art', 'ThirdParty', 'MakeHuman', 'fetch.py')], check=True)


# ---------------------------------------------------------------------------------------------------------- main

def build(fig):
    """Every detailed part, boots to hat (the order matters: each layer is fitted over the ones under it)."""
    skin, eyes = head.build_skin(fig, fig.op, fig.opt)
    brows = head.build_brows(fig)
    trousers = cl.build_trousers(fig)
    boots = cl.build_boots(fig)
    vest, buttons = cl.build_vest(fig)
    shirt = cl.build_shirt(fig)
    collar = cl.build_collar(fig)
    log('trousers, boots, waistcoat, shirt, collar')
    belt = cl.build_belt(fig, [trousers, vest])
    body_ob = fig.b.mesh('_Body')
    body_ob.hide_render = True
    torso_pts = fig.wv[fig.torso_idx]
    legs = fig.wv[fig.b.used][fig.w_leg[fig.b.used] > 0.3]
    gloves, tops = cl.build_gloves(fig)
    fig.cuff_tops = tops
    shirt_cuffs = cl.build_shirt_cuffs(fig, tops)
    coat = cm.build_coat(fig, vest, body_ob)
    belt_pts = np.concatenate([gm.object_points(o) for o in belt])
    under = np.concatenate([torso_pts, legs, gm.object_points(vest), gm.object_points(trousers), belt_pts])
    tails = cm.build_tails(fig, under, coat)
    coat_cuffs = cm.build_coat_cuffs(fig, coat)
    lapels = cm.build_lapels(fig, coat)
    coat_collar = cm.build_coat_collar(fig, coat)
    coat_buttons = cm.build_coat_buttons(fig, tails)
    log('belt, card case, tailcoat, sleeves, gloves, cuffs, lapels, collar')
    cravat = cl.build_cravat(fig, collar, [shirt, vest])
    chain = cl.build_watch_chain(fig, vest)
    hair = head.build_hair(fig, fig.op)
    earrings = head.build_earrings(fig)
    hat = head.build_hat(fig, fig.op, fig.opt)
    log('cravat, watch chain, hair, earrings, hat')
    sewn = [coat, tails, collar, vest, coat_collar] + lapels + belt[:1] + coat_cuffs + shirt_cuffs + cravat[:1]
    sewn += cravat[2:3] + [g for g in gloves if g.name.startswith('Gauntlet')] + boots[:1] + hat[:3]
    for ob in sewn:
        if 'Seams' not in ob.data.color_attributes:
            kit.set_seams(ob)
    bpy.data.objects.remove(body_ob)
    return dict(skin=skin, eyes=eyes, hat=hat, coat=coat)


def main():
    fetch_makehuman()
    bpy.ops.wm.read_factory_settings(use_empty=True)
    fig = Figure()
    parts = build(fig)
    out = os.path.join(REPO, 'Saved', 'ArtPreviews', 'Characters', 'Odessa')
    os.makedirs(out, exist_ok=True)
    for ob in [o for o in bpy.context.scene.objects if o.name.startswith('_WatchChainLow')]:
        ob.hide_render = True
    if '--shape' in ARGS:
        for shot in arg_value('--shots', 'full').split('+'):
            views = VIEWS if shot != 'face' else (('front', 0.0), ('three', 35.0), ('side', 90.0))
            preview(os.path.join(out, f'Odessa_clay_{shot}'), views=views, aim=SHOTS[shot])
        return
    highs = [o for o in bpy.context.scene.objects if o.type == 'MESH' and not o.name.startswith('_')]
    lows = make_lows(fig, highs)
    log(f'game mesh: {sum(len(p.data.polygons) for p in lows)} faces before thickness')
    prepare_uvs(lows)
    check = '--stats' in ARGS or '--rig-check' in ARGS
    baked = None if check else bake_all(lows, highs, int(arg_value('--size', 2048)))
    for ob in highs + [o for o in bpy.context.scene.objects if o.name.startswith('_WatchChainLow')]:
        if ob.name in bpy.data.objects and ob not in lows:
            me = ob.data
            bpy.data.objects.remove(ob)
            bpy.data.meshes.remove(me)
    for ob in lows:
        ob.name = ob.name[:-4] if ob.name.endswith('_low') else ob.name
    thicken(lows)
    if check:
        tris = {}
        for ob in lows:
            key = part_base(ob.name)
            tris[key] = tris.get(key, 0) + sum(len(p.vertices) - 2 for p in ob.data.polygons)
        for k, n in sorted(tris.items(), key=lambda kv: -kv[1]):
            log(f'{k:16s} {n:7d}')
        log(f'{"total":16s} {sum(tris.values()):7d}')
    if '--stats' in ARGS:
        return
    import odessa_rig as rig
    arm, skin = rig.rig(fig, lows)
    if '--rig-check' in ARGS:
        rig.rig_check(fig, arm, skin)
        return
    if '--audit' in ARGS:
        import odessa_audit
        odessa_audit.run(fig, arm, skin, baked, arg_value('--audit', 'after'), viewer_materials, log, REPO)
        return
    if '--preview' in ARGS:
        viewer_materials(skin, baked)
        pose_for_view(arm)
        for shot in arg_value('--shots', 'full+face').split('+'):
            views = VIEWS if shot != 'face' else (('front', 0.0), ('three', 35.0), ('side', 90.0))
            preview(os.path.join(out, f'Odessa_{shot}'), views=views, aim=SHOTS[shot], samples=32)
        for pb in arm.pose.bones:
            pb.rotation_quaternion = (1.0, 0.0, 0.0, 0.0)
        bpy.context.view_layer.update()
        log('preview written')
    game_materials(skin)
    face_like_mannequin(arm, skin)
    log('ready to export')


if __name__ == '__main__':
    main()
