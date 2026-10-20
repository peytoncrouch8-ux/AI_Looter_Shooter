"""SK_Crane: Hollis Crane, the Unpaid (Docs/Story.md: a marshal dead forty years, his town's saint gone dark; Sexton
promised him a fare), the player's body when he's the hero picked, built from the portrait the user picked on
2026-10-19: option A "The Marshal" of Art/Portraits/Crane.py (its OPTIONS["A"], FACE and MACRO are the spec), the way
Ellis.py builds SK_Ellis (Saved/Heroes/CONTRACT.md; Docs/Pipeline.md "Character models"). Boots to hat:
  - the body: MakeHuman's CC0 base shaped as the portrait's old man, gaunt, 1.92 m barefoot (1.95 in his boots), the
    head and shoulders carried forward (crane_common.STOOP: a shape, not a posture); grey-green dead skin, the cheeks
    and temples sunken, the sockets dark; the right eye a cyan slit burning, the left under the ferryman's coin
  - a battered slouch hat (the portrait's, seated by ellis_hat.fit), long lank white hair falling in front of the
    shoulders and down the back, a drooping white mustache and heavy white brows (hair cards, crane_hair)
  - the old linen shirt torn over his heart, the ember glowing cyan in the tear; a collarless band at the neck
  - the long slate coat to the low calf, open, its hem torn into pointed tatters (settled by cloth simulation, the
    tatters torn after); its sleeves with turned cuffs over bare bony hands; a ragged shoulder cape and a low collar;
    the tin star pinned at the cape's left front
  - the toll chain slung from over the right shoulder across the chest to the left hip and up behind, its iron hook
    hanging at the chest on four links; a plain belt; dark wool trousers tucked into tall worn boots
Rig: the UE5 mannequin's 63 deform bones on MakeHuman's joints, the forearms' twist bones, swing chains for the game's
springs (skirt_, cape_, hair_, lantern_: the hook), the hat's bone, the cape's idle arms (crane_rig). Built facing -Y,
turned last to face like the mannequin.

    artrun.ps1 -Script Art\Models\Characters\Crane.py -Threads 6 -ScriptArgs <flags>
    --shape [--shots full+upper+face+skirt]   clay (the hair keeps its atlas), no bake
    --stats                                   the game mesh's triangles per part, no bake
    --rig-check                               the swing bones at rest and bent, in clay: Saved/Concepts/CraneSwing
    --preview                                 the baked look, arms down: Saved/ArtPreviews/Characters/Crane
    --audit <tag>                             the game's poses, every layer checked: Saved/Concepts/CraneAudit/<tag>
    --no-sim  --rebake  --frames N  --size N
"""
import math
import os
import sys

import bpy  # first: as a Python module bmesh and mathutils come with it
import numpy as np
from mathutils import Matrix, Vector

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(os.path.dirname(HERE)))
for p in (os.path.join(REPO, 'Tools', 'Blender'), os.path.join(REPO, 'Art', 'Portraits')):
    if p not in sys.path:
        sys.path.insert(0, p)
import crane_clothes as cw  # noqa: E402
import crane_common as cc  # noqa: E402
import crane_gear as cg  # noqa: E402
import crane_hair as chh  # noqa: E402
import crane_head as ch  # noqa: E402
import crane_looks as cl  # noqa: E402
import crane_rig as cr  # noqa: E402
import looter_bake as bk  # noqa: E402
import looter_textures as lt  # noqa: E402
import looter_garments as gm  # noqa: E402
import looter_looks as lk  # noqa: E402
from crane_common import ARGS, OPT, arg_value, log, slot  # noqa: E402

COAT = ((0.05, 0.065, 0.08), (0.19, 0.23, 0.27))        # the slate coat's dark and light (sRGB; the portrait's, lifted)

cc.LOOKS.update({
    'Skin': lambda n: lk.skin(n, OPT['skin']),
    'Scalp': lambda n: lk.scalp(n, (0.55, 0.55, 0.50)),
    'DeadEye': lambda n: cl.dead_eye(n),
    'EyeGlow': lambda n: lk.glow(n, OPT['glow'], 6.0),
    'Ember': lambda n: lk.glow(n, (0.35, 0.85, 1.0), 2.5),
    'Coin': lambda n: cl.coin(n),
    'Felt': lambda n: lk.felt(n, (0.08, 0.075, 0.065), (0.27, 0.25, 0.21), band_z=cc.HAT.get('band_z')),
    'HatBand': lambda n: lk.leather(n, (0.07, 0.055, 0.045), grain=280.0, dust_amt=0.2, dust_height=0.001),
    'Coat': lambda n: cl.slate(n, *COAT, hem=cc.HEM),
    'Shirt': lambda n: lk.linen(n, OPT['coat']['shirt']),
    'Trousers': lambda n: lk.wool(n, (0.27, 0.25, 0.23)),
    'Boots': lambda n: lk.leather(n, (0.17, 0.15, 0.14), grain=280.0, dust_amt=0.35, dust_height=0.12, mud=0.5,
                                  boot=cc.BOOT_LOOK),
    'Soles': lambda n: lk.stacked(n, (0.11, 0.085, 0.065)),
    'Belt': lambda n: lk.leather(n, (0.26, 0.19, 0.13), grain=280.0, dust_amt=0.25, dust_height=0.001),
    'Iron': lambda n: cl.rusted_iron(n),
    'Hair': lambda n: chh.material(n),
    'Tin': lambda n: lk.metal(n, (0.22, 0.23, 0.21), (0.60, 0.61, 0.58), rough=(0.35, 0.55), tarnish=(0.12, 0.12, 0.11)),
}
)

# (the camera's target height, distance, height over it[, the target's y: the face is carried forward of the feet])
SHOTS = {'full': (0.98, 4.6, 0.25), 'upper': (1.55, 1.8, 0.08), 'face': (1.80, 0.75, 0.0, -0.10), 'feet': (0.22, 1.5, 0.35),
         'hips': (1.0, 1.9, 0.1), 'skirt': (0.62, 2.4, 0.1), 'chest': (1.48, 1.1, 0.05, -0.08), 'star': (1.43, 0.55, 0.0, -0.17, 0.12)}


def preview(path, views=(('front', 0.0), ('three', 35.0), ('side', 90.0), ('back', 180.0)), size=900, samples=24,
            aim=SHOTS['full']):
    """Quick Cycles renders round the figure (shape checks: clay with --shape)."""
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
    sc.cycles.transparent_max_bounces = 48
    if '--shape' in ARGS:
        # clay, but the hair cards keep their strand atlas (in clay they're slabs) and the glows their light
        clay = bpy.data.materials.get('_Clay') or bpy.data.materials.new('_Clay')
        clay.diffuse_color = (0.55, 0.52, 0.48, 1.0)
        clay.use_nodes = True
        clay.node_tree.nodes['Principled BSDF'].inputs['Base Color'].default_value = (0.55, 0.52, 0.48, 1.0)
        for ob in bpy.context.scene.objects:
            if ob.type != 'MESH':
                continue
            for s_ in ob.material_slots:
                if s_.material is not None and s_.material.get('LookKey') not in ('Hair', 'EyeGlow', 'Ember'):
                    s_.material = clay
    cd = bpy.data.cameras.new('PreviewCam')
    cd.lens = 70
    cam = bpy.data.objects.new('PreviewCam', cd)
    sc.collection.objects.link(cam)
    sc.camera = cam
    out = []
    for name, yaw in views:
        a = math.radians(yaw)
        target = Vector((aim[4] if len(aim) > 4 else 0.0, aim[3] if len(aim) > 3 else 0.0, aim[0]))
        cam.location = target + Vector((math.sin(a) * aim[1], -math.cos(a) * aim[1], aim[2]))
        cam.rotation_euler = (target - cam.location).to_track_quat('-Z', 'Y').to_euler()
        sc.render.filepath = f'{path}_{name}.png'
        bpy.ops.render.render(write_still=True)
        out.append(sc.render.filepath)
    bpy.data.objects.remove(cam)
    return out


def build(fig):
    """Every detailed part, in the order each is fitted over the ones under it. Returns {name: object or list}."""
    p = {}
    p['skin'] = ch.build_skin(fig)
    p['eyes'] = ch.build_eyes(fig)
    p['coin'] = ch.build_coin(fig, p['skin'], p['eyes'][0])
    p['trousers'] = cw.build_trousers(fig)
    p['boots'] = cw.build_boots(fig)
    body_ob = fig.b.mesh('_Body')
    body_ob.hide_render = True
    p['_body'] = body_ob
    p['shirt'], p['band'], p['ember'] = cw.build_shirt(fig, body_ob)
    p['belt'] = cw.build_belt(fig, [p['trousers'], p['shirt']])
    log('skin, trousers, boots, shirt, belt')
    used = fig.b.used
    legs = fig.wv[used][fig.w_leg[used] > 0.3]
    drape_pts = np.concatenate([fig.wv[fig.torso_idx], legs, gm.object_points(p['shirt']), gm.object_points(p['trousers'])]
                               + [gm.object_points(o) for o in p['belt']])
    p['coat'] = cw.build_coat(fig, drape_pts, p['belt'])
    p['sleeves'] = cw.build_sleeves(fig, [body_ob])
    if cc.SIM:
        fill = cw.leg_fill(fig, [p['trousers'], p['boots'][0]], cc.HEM - 0.12, fig.hip_z - 0.08, coat=p['coat'])
        cw.settle(fig, p['coat'], [body_ob, p['trousers'], p['shirt'], p['boots'][0], fill] + p['belt'])
        bpy.data.objects.remove(fill)
    else:
        cw.tear_hem(p['coat'])
    p['cuffs'] = cw.build_cuffs(fig, p['sleeves'])
    log('coat, sleeves, cuffs')
    p['chain'] = cg.build_chain(fig, [p['coat'], p['shirt'], p['sleeves']])
    p['hook'] = cg.build_hook(fig)
    p['cape'], _top_r = cw.build_cape(fig, body_ob, p['coat'], p['sleeves'], gm.object_points(p['chain']))
    p['collar'] = cw.build_collar(fig, [p['band'], p['skin']], p['cape'])
    p['star'] = cg.build_star(fig, [p['coat']])
    log('chain, hook, cape, collar, star')
    p['hat'] = ch.build_hat(fig)
    p['hair'] = chh.build(fig, [p['skin'], p['band'], p['collar'], p['cape'], p['coat'], p['shirt']], 'Hair', slot('Hair'),
                          pivot=cc.HAT.get('pivot'))
    p['mustache'] = chh.build_mustache(fig, p['skin'], slot('Hair'))
    p['brows'] = chh.build_brows(fig, p['skin'], slot('Hair'))
    log('hat, hair, mustache, brows')
    for ob in [p['cape'], p['collar'], p['shirt'], p['band'], p['belt'][0], p['boots'][0]] + p['cuffs']:
        cc.set_seams(ob)
    return p


# ------------------------------------------------------------------------------------------- the texture sets, slots

# The texture sets the looks bake into (Art/Textures/<Set>/), the hair's script-made set, the glowing looks.
TEXTURE_SETS = {
    'CraneSkin': ('Skin', 'Scalp', 'DeadEye'),
    'CraneCoat': ('Coat',),
    'CraneGarb': ('Felt', 'HatBand', 'Shirt', 'Trousers'),
    'CraneGear': ('Boots', 'Soles', 'Belt', 'Iron', 'Tin', 'Coin'),
}
MADE_SETS = {'CraneHair': ('Hair',)}
GLOWS = {'EyeGlow': (0x5AD8FF, 4.0), 'Ember': (0x5AD8FF, 5.0)}
# The game's material slots: (master, the looks on it, the master's parameters), several slots sharing a set
GAME_MATERIALS = {
    'CraneSkin': ('CharacterSkin', ('Skin', 'Scalp', 'DeadEye'), {}),
    'CraneCoat': ('CharacterCloth', ('Coat',), {'FuzzStrength': 0.3, 'FuzzLift': 0.1}),
    'CraneGarbCloth': ('CharacterCloth', ('Felt', 'Shirt', 'Trousers'), {'FuzzStrength': 0.3, 'FuzzLift': 0.12}),
    'CraneGarbLeather': ('Character', ('HatBand',), {'Specular': 0.3}),
    # white hair under the engine's Hair shading: a little under white, as Ellis's dark hair was tinted for the sun
    'CraneHair': ('CharacterHair', ('Hair',), {'tint': '#D8D8D8'}),
    'CraneGear': ('Character', ('Boots', 'Soles', 'Belt', 'Iron', 'Tin', 'Coin'), {}),
}


# ----------------------------------------------------------------------------------------------- the game mesh

SMART_UV = ('Eye_', 'Soles', 'HatCrown', 'HatBrim', 'HatBand', 'Coin', 'Buckle', 'Chain', 'Hook', 'Star', 'Cuff',
            'Ember', 'Boots')
# Sheets given thickness after the bake (m); the inner side shares the outer side's UVs.
THICK = {'Coat': 0.004, 'Cape': 0.004, 'Collar': 0.005, 'Belt': 0.004, 'HatBrim': 0.0055, 'HatBand': 0.0015,
         'HatCrown': 0.004, 'Cuff_l': 0.003, 'Cuff_r': 0.003, 'Shirt': 0.0015, 'ShirtBand': 0.002}
# Each detailed part's share of its triangles the game keeps (Blender's Decimate); 1 keeps it whole. The skin goes by
# region (crane_head.decimate_skin); the coin, the chain and the hook are built again low (their high ones are the
# bake's source); the hair cards are the game's as built.
LOW = {'Eye': 0.30, 'Trousers': 0.40, 'Boots': 0.17, 'Soles': 0.6, 'Shirt': 0.26, 'ShirtBand': 0.35, 'Ember': 1.0,
       'Belt': 0.5, 'Buckle': 1.0, 'Coat': 0.15, 'Sleeves': 0.42, 'Cuff': 0.4, 'Cape': 0.15, 'Collar': 0.4,
       'Star': 0.35, 'HatCrown': 0.10, 'HatBrim': 0.22, 'HatBand': 0.4, 'HatBandInner': 1.0}
EDGE_KEEP = {'Coat': 0.012, 'Cape': 0.009, 'Collar': 0.008, 'HatBrim': 0.010, 'ShirtBand': 0.006}
REBUILT = ('Coin', 'Chain', 'Hook', 'HookLinks', 'HookEye')   # built again low, not reduced


def low_ratio(name):
    key = name.split('.')[0]
    for k in sorted(LOW, key=len, reverse=True):
        if key.startswith(k):
            return LOW[k]
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
    """The faces Decimate turned over turned back, and its slivers dissolved (Ellis.py's: Unreal draws a face from its
    front only, so each was a hole)."""
    import bmesh
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


def push_out(objs, under, gap):
    """Each of objs' points back outside the reduced parts under it by gap (reduced layers cut through each other where
    the detailed ones only just cleared)."""
    if not objs or not under:
        return
    tree = gm.bvh_objects(under)
    for ob in objs:
        co = gm._object_coords(ob)
        ob.data.vertices.foreach_set('co', gm.clear(co, tree, gap, 0.02).astype(np.float32).ravel())
        ob.data.update()


def make_lows(fig, highs):
    """The game mesh: a reduced copy of every detailed part (named <part>_low until the detailed ones go), the skin by
    region, the shirt only where it shows, the coin, chain and hook built again low, the hair cards as they are; then
    each outer part pushed back outside the reduced parts under it."""
    import bmesh
    lows = []
    for ob in highs:
        if ob.name.startswith(REBUILT):
            continue
        low = bpy.data.objects.new(ob.name + '_low', ob.data.copy())
        bpy.context.scene.collection.objects.link(low)
        low.matrix_world = ob.matrix_world
        cc.bake_transform(low)
        if ob.name.startswith(('Hair', 'Mustache', 'Brows')):
            lows.append(low)
            continue
        if ob.name == 'Shirt':
            # only the shirt in the coat's V shows (and round the tear): what lies under the coat goes
            bm = bmesh.new()
            bm.from_mesh(low.data)
            hidden = [f for f in bm.faces if (lambda q: q.z < fig.neck[2] - 0.01 and
                                              abs(q.x) > float(cw.opening(fig, q.z)) + 0.045)(f.calc_center_median())]
            bmesh.ops.delete(bm, geom=hidden, context='FACES')
            bmesh.ops.delete(bm, geom=[v for v in bm.verts if not v.link_faces], context='VERTS')
            bm.to_mesh(low.data)
            bm.free()
        if ob.name == 'Crane':
            before, after = ch.decimate_skin(fig, low)
            log(f'skin: {before} -> {after} triangles by region')
            unflip(low, ob)
        elif low_ratio(ob.name) < 1.0:
            m = low.modifiers.new('decimate', 'DECIMATE')
            m.ratio = low_ratio(ob.name)
            co = gm._object_coords(low)
            band = EDGE_KEEP.get(ob.name.split('.')[0])
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
    lows.append(ch.coin_low(fig))
    chain = cg.chain_low(fig)
    chain['LinkPoints'] = cg.LOW_LINK[0] * cg.LOW_LINK[1]
    lows.append(chain)
    lows += cg.build_hook(fig, low=True)
    by = lambda *names: [o for o in lows if o.name.split('.')[0].startswith(names)]
    push_out(by('ShirtBand'), by('Crane'), 0.003)
    push_out(by('Trousers'), by('Shirt'), 0.002)
    push_out(by('Belt'), by('Trousers', 'Shirt'), 0.002)
    push_out(by('Coat'), by('Shirt', 'Belt', 'Buckle', 'Trousers'), 0.003)
    push_out(by('Cuff'), by('Sleeves'), 0.003)
    push_out(by('Boots'), by('Trousers'), 0.003)
    cw.clear_cape(fig, by('Cape'), by('Coat', 'Chain'), by('Sleeves'))
    push_out(by('Collar'), by('Cape', 'ShirtBand'), 0.003)
    push_out(by('Mustache', 'Brows'), by('Crane'), 0.001)
    lift_whole(by('Star'), by('Coat'), 0.0015)
    coin_sight(fig, lows)
    return lows


def lift_whole(objs, under, gap):
    """A rigid piece (the star) moved out along what it lies on until every point is gap clear of the reduced part
    under it (pushing its points one by one would bend it)."""
    if not objs or not under:
        return
    tree = gm.bvh_objects(under)
    for ob in objs:
        co = gm._object_coords(ob)
        worst, along = 0.0, None
        for p in co:
            loc, nrm, _i, _d = tree.find_nearest(Vector(tuple(p)), 0.03)
            if loc is not None:
                d = (Vector(tuple(p)) - loc).dot(nrm)
                if d < gap and gap - d > worst:
                    worst, along = gap - d, np.array(nrm)
        if along is not None:
            ob.data.vertices.foreach_set('co', (co + along * worst).astype(np.float32).ravel())
            ob.data.update()
            log(f'{ob.name}: lifted {worst * 1000:.1f} mm off the reduced coat')


def coin_sight(fig, lows):
    """What a look straight at the coin meets first (the coin must be the first thing over the left eye)."""
    coin = next(o for o in lows if o.name.startswith('Coin'))
    c = gm._object_coords(coin).mean(0)
    fwd = np.array(fig.axes[0])
    seen = []
    for ob in lows:
        if ob.name.startswith(('Crane', 'Eye', 'Coin', 'Brows', 'Hair', 'HatBrim')):
            hit = gm.bvh_objects([ob]).ray_cast(Vector(tuple(c + fwd * 0.10)), Vector(tuple(-fwd)), 0.2)
            if hit[0] is not None:
                seen.append((hit[3], ob.name))
    log('coin: looking at it, ' + ', '.join(f'{n} at {(d - 0.10) * 1000:.1f} mm' for d, n in sorted(seen)))


def look_of(ob):
    return ob.material_slots[0].material['LookKey'] if ob.material_slots and ob.material_slots[0].material else None


def prepare_uvs(objects):
    """Every part gets a usable 'UVMap': MakeHuman's on the shells and the skin, arc lengths on the drapes, smart
    projection on the small hard parts and the remeshed boots. (The hair cards' UVs are their atlas's.)"""
    for ob in objects:
        me = ob.data
        if ob.name.startswith(('Hair', 'Mustache', 'Brows')):
            continue
        if ob.name.startswith(SMART_UV) or 'UVMap' not in me.uv_layers:
            for layer in list(me.uv_layers):
                me.uv_layers.remove(layer)
            bk.smart_uv(ob)
            if 'LinkPoints' in ob or ob.name.startswith('HookLinks'):
                share_link_uvs(ob)
        me.uv_layers.active = me.uv_layers['UVMap']
        me.uv_layers['UVMap'].active_render = True


def share_link_uvs(ob):
    """Every link of a chain takes the first link's UVs (the links are one mesh repeated: one island for all of them;
    each its own, they took most of the gear atlas and left the boots a quarter of it)."""
    me = ob.data
    per = len(me.polygons)
    n = cg.LOW_LINK[0] * cg.LOW_LINK[1]                  # faces (quads) per link
    if per % n:
        return
    uv = np.empty(len(me.loops) * 2, dtype=np.float32)
    me.uv_layers['UVMap'].data.foreach_get('uv', uv)
    uv = uv.reshape(per // n, -1)
    uv[:] = uv[0]
    me.uv_layers['UVMap'].data.foreach_set('uv', uv.ravel())


BAKE_SOURCES = [os.path.abspath(__file__)] + [os.path.join(REPO, *p) for p in (
    ('Tools', 'Blender', 'looter_human.py'), ('Tools', 'Blender', 'looter_garments.py'), ('Tools', 'Blender', 'looter_looks.py'),
    ('Tools', 'Blender', 'looter_bake.py'), ('Art', 'Portraits', 'mh.py'), ('Art', 'Portraits', 'Crane.py'),
    ('Art', 'Portraits', 'bust.py'), ('Tools', 'Blender', 'ellis_hat.py'), ('Tools', 'Blender', 'ellis_hair.py'),
    ('Tools', 'Blender', 'crane_common.py'), ('Tools', 'Blender', 'crane_head.py'), ('Tools', 'Blender', 'crane_clothes.py'),
    ('Tools', 'Blender', 'crane_gear.py'), ('Tools', 'Blender', 'crane_looks.py'), ('Tools', 'Blender', 'crane_hair.py'))]
# (crane_rig.py and crane_audit.py are left out: they shape nothing that's baked)
BAKE_WEIGHTS = {'Coin': 5.0, 'Star': 1.8, 'Ember': 1.0, 'Buckle': 1.4}


def bake_all(lows, highs, size):
    """Packs every texture set's UVs on the game mesh and bakes it from the detailed parts, unless the sets on disk were
    baked from these same scripts at this size (Art/Textures/CraneSkin/bake.digest; --rebake forces it)."""
    stamp = os.path.join(lt.TEXTURE_DIR, 'CraneSkin', 'bake.digest')
    want = bk.digest(BAKE_SOURCES, f'size={size} blender={bpy.app.version_string}')
    have = open(stamp).read().strip() if os.path.exists(stamp) else ''
    out = {}
    for set_name, keys in TEXTURE_SETS.items():
        obs = [o for o in lows if look_of(o) in keys]
        src = [o for o in highs if look_of(o) in keys]
        weights = {o: k for o in obs for p, k in BAKE_WEIGHTS.items() if o.name.startswith(p)}
        bk.pack(obs, weights=weights)
        if have == want and '--rebake' not in ARGS:
            out[set_name] = bk.load_set(set_name)
        if out.get(set_name) is None:
            out[set_name] = bk.bake_set(set_name, obs, size, sources=src, hide=highs)
            log(f'baked {set_name} ({len(obs)} parts from {len(src)})')
        else:
            log(f'{set_name}: kept (baked from these scripts before)')
    for set_name in MADE_SETS:
        if not chh.have_textures():
            chh.make_textures()
        out[set_name] = bk.load_set(set_name)
    os.makedirs(os.path.dirname(stamp), exist_ok=True)
    with open(stamp, 'w') as f:
        f.write(want + '\n')
    return out


def thicken(objects):
    for ob in objects:
        t = THICK.get(ob.name.split('.')[0])
        if t:
            off = 0.0 if ob.name.startswith('HatBrim') else -1.0 if ob.name.startswith('HatCrown') else 1.0
            gm.solidify(ob, t, offset=off, rim=True)


def stats(parts):
    tris = {}
    for ob in parts:
        key = ob.name.split('.')[0]
        key = key[:-2] if key[-2:] in ('_l', '_r') else key
        tris[key] = tris.get(key, 0) + cc.tris_of(ob)
    for k, n in sorted(tris.items(), key=lambda kv: -kv[1]):
        log(f'{k:16s} {n:7d}')
    log(f'{"total":16s} {sum(tris.values()):7d}')


def fetch_makehuman():
    import mh
    if not os.path.exists(os.path.join(mh.DATA, '3dobjs', 'base.obj')):
        import subprocess
        log("fetching MakeHuman's CC0 assets (Art/ThirdParty/MakeHuman/fetch.py)")
        subprocess.run([sys.executable, os.path.join(REPO, 'Art', 'ThirdParty', 'MakeHuman', 'fetch.py')], check=True)


def main():
    fetch_makehuman()
    bpy.ops.wm.read_factory_settings(use_empty=True)
    fig = cc.Figure()
    parts = build(fig)
    bpy.data.objects.remove(parts['_body'])
    os.makedirs(cc.OUT_DIR, exist_ok=True)
    if '--shape' in ARGS:
        for shot in (arg_value('--shots', 'full')).split('+'):
            preview(os.path.join(cc.OUT_DIR, f'Crane_{shot}'), aim=SHOTS[shot])
        return
    highs = [o for o in bpy.context.scene.objects if o.type == 'MESH']
    lows = make_lows(fig, highs)
    log(f'game mesh: {sum(len(p.data.polygons) for p in lows)} faces before thickness')
    prepare_uvs(lows)
    check = '--stats' in ARGS or '--rig-check' in ARGS
    baked = None if check else bake_all(lows, highs, int(arg_value('--size', 2048)))
    for ob in highs:
        me = ob.data
        bpy.data.objects.remove(ob)
        if me.users == 0:
            bpy.data.meshes.remove(me)
    for ob in lows:
        ob.name = ob.name[:-4] if ob.name.endswith('_low') else ob.name
    thicken(lows)
    if '--stats' in ARGS:
        stats(lows)
        return
    arm, skin = cr.rig(fig, lows)
    if '--rig-check' in ARGS:
        cr.rig_check(fig, arm, skin, REPO)
        return
    view = lambda: cr.viewer_materials(skin, baked, TEXTURE_SETS, MADE_SETS, GLOWS)
    if '--audit' in ARGS:
        import crane_audit
        crane_audit.run(fig, arm, skin, view, arg_value('--audit', 'after'), log, REPO)
        return
    if '--preview' in ARGS:
        view()
        cr.pose_for_view(arm)
        for shot in (arg_value('--shots', 'full+face')).split('+'):
            views = (('front', 0.0), ('three', 35.0), ('side', 90.0), ('back', 180.0)) if shot != 'face' else \
                (('front', 0.0), ('three', 35.0))
            preview(os.path.join(cc.OUT_DIR, f'Crane_{shot}'), views=views, samples=48, aim=SHOTS[shot],
                    size=1200 if shot == 'full' else 900)
        for pb in arm.pose.bones:
            pb.rotation_quaternion = (1.0, 0.0, 0.0, 0.0)
        log('preview written')
    cr.game_materials(skin, TEXTURE_SETS, MADE_SETS, GAME_MATERIALS, GLOWS)
    cr.face_like_mannequin(arm, skin)
    log('ready to export')


if __name__ == '__main__':
    main()
