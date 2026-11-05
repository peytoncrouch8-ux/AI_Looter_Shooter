"""The Wardens' Tripo models (Docs/Work/2026-10-31-Wardens/TRIPO_MODELS.md; the GLBs in Art/Tripo/<Name>/) made into game
models the usual way: Art/Models/Tripo/Wardens.py and Art/Models/Props/TripoProps.py build them with this module and
Tools/Blender/looter_export.py exports them (Tools\\models.ps1 -Only Wardens,TripoProps), so Looter.ImportModels brings
them in like any model: MI_Tripo<Name> on the textured masters, the maps in /Game/Art/Textures/Tripo<Name>.

Each model (SPECS): Tripo's mesh as it came (its triangles kept, nothing remeshed), cleaned (looter_tripo.load), turned to
face Blender's -Y (Unreal's +X), scaled uniformly to its real size, its origin put where the game wants it, a slot for its
glowing glass where the game lights it, sockets the game reads, collision hulls (boxes, or fitted to its parts by
looter_tripo_collision; none on held props), no Nanite
(the rule for these; Medium draws the mesh itself) and LODs at 50/25/12%.

Style (the user's call 2026-10-31: everything from Tripo matches the game's look): no raw Tripo materials. The maps go
through the project's masters as material instances (M_World for props and ships, M_Gun for the held guns: its worn edges
and grime; people in Tools/Blender/tripo_people.py on M_Character), and the colour map is graded like the Tripo chests'
(ChestTiers.write_maps: unsharp-masked, saturation pulled down, contrast lifted round middle grey); Tripo's roughness gets a
floor (its glossy patches read wet beside the game's matte painted surfaces); its occlusion (the GLB's ORM red) is kept.
Sizes: people and ships 2K maps, props 1K (the user's caps).

    import looter_tripo_wardens as tw
    for name in tw.names('Tripo'):
        tw.build(name)
"""
import math
import os

import bmesh
import bpy
import numpy as np
from mathutils import Matrix, Vector

import looter_model as lm
import looter_tripo_barge as barge
import looter_tripo_barge_paint as barge_paint
import looter_tripo_zones as zones_paint
import looter_tripo_collision as tc
import looter_tripo_repack as repack
import looter_textures as lt
import looter_tripo as tripo

REPO = lt.REPO
TRIPO_DIR = os.path.join(REPO, 'Art', 'Tripo')
LODS, LOD_SCREENS = '50,25,12', '0.5,0.25,0.12'
# The colour grade (saturation, contrast) and roughness floor by kind; the chests used (0.9-1.0, 1.08).
GRADE = {'prop': (0.86, 1.06), 'ship': (0.84, 1.06), 'gun': (0.9, 1.08), 'person': (0.92, 1.04)}
ROUGH_FLOOR = {'prop': 0.38, 'ship': 0.4, 'gun': 0.28, 'person': 0.35}
# The game's shared lantern glass (looter_buildings._material('glow'), LanternPost.py): the same name and look, so
# the Tripo glass shares MI_LanternGlow (the roosts light and darken it by slot name: RoostKit::GlassSlot).
GLOW_NAME = 'LanternGlow'


def _glass(rgb):
    """Lantern glass as Tripo painted it: pale, low saturation (the iron frame is dark, the brass saturated)."""
    hi, lo = rgb.max(axis=-1), rgb.min(axis=-1)
    return (hi > 0.42) & ((hi - lo) < 0.22 * np.maximum(hi, 1e-3))


def _bulb(rgb):
    """The Great Lantern's lamp: pale warm yellow."""
    r, g, b = rgb[..., 0], rgb[..., 1], rgb[..., 2]
    return (r > 0.62) & (g > 0.55) & (b < 0.85 * g) & (b > 0.25)


# Coordinates in each spec:
# - 'glb': Tripo's own space as Blender imports the GLB (meters, Z up, its odd size) for cuts, regions and sockets found
#   on the mesh;
# - 'ue': the finished model in Unreal (cm, X front, Y right, Z up, from its pivot).
# turn: degrees about Z bringing its front to Blender's -Y; tilt: (axis, degrees) after it.
# size: (what, meters): 'length' along its front, 'width', 'height' or 'max' of the three, measured after turning.
# pivot: 'bottom' (middle of its footprint, lowest point), 'center', or 'post' (the post's axis found between two heights).
# boxes: collision hulls as fractions of its finished box ((front lo, right lo, up lo), (front hi, right hi, up hi)),
# 'box' one round all, None no collision. collision: a looter_tripo_collision function (obj, report) fitting hulls to the
# model's own parts instead (with boxes=[]; never boxes=[] without it: no hulls is mesh collision).
SPECS = {
    # --- Ships (TRIPO_MODELS 11, 12, 14) ---
    'Lamplighter': dict(
        category='Tripo', kind='ship', master='World', maps=2048, turn=-90.0, size=('length', 31.0), pivot='bottom',
        # The hull's own prow lantern goes: the game hangs the Great Lantern (its own model, lit, its flame) there.
        cut=[((1.49, -0.11, -0.475), (1.72, 0.11, -0.12))],
        # Her hull under the deck and the bag (FLIGHT turns the hull's collision off; these are for anything else).
        boxes=[((0.04, 0.03, 0.0), (0.96, 0.97, 0.205)), ((0.03, 0.0, 0.46), (0.97, 1.0, 1.0))],
        # Her paint repainted in our sets zone by zone (looter_tripo_zones_tables.LAMPLIGHTER: planks, canvas, rope, brass, iron).
        paint=zones_paint.hook('Lamplighter'),
        sockets={'Lantern': ('glb', (1.60, 0.0, -0.475))}),
    'DredgeBarge': dict(
        category='Tripo', kind='ship', master='World', maps=2048, turn=90.0, size=('length', 75.0), pivot='bottom',
        # Reshaped for the walkable kit (looter_tripo_barge: one flat deck, the house, the tower on it, bulwark, guard),
        # which adds its own UCX_ boxes and the Door and Gangway sockets.
        rework=barge.rework, slots=barge.slots, paint=barge_paint.paint, boxes=[],
        # The house's and casing's walls on a 2K texture set of their own, TripoDredgeBargeWalls (looter_tripo_repack.own_set,
        # 15 -> 33 px/m; REPACK.md): the faces slots() puts on TripoDredgeBarge_Walls, one rule (barge_paint.walls).
        sets={'Walls': dict(faces=barge_paint.walls, maps=2048, slot='TripoDredgeBarge_Walls')},
        # Its LODs switch sooner (PERF, Landfall: the barge cost 0.3-0.9 ms at range with the shared 0.5/0.25/0.12).
        lod_screens='0.6,0.35,0.18',
        # The two smokestacks' tops (the smog plumes) and the hose drum's underside (the hose hangs from it).
        sockets={'Stack_1': ('stack', (0.265, 0.0)), 'Stack_2': ('stack', (0.572, 0.0)),
                 'HoseDrum': ('glb', (0.22, 0.08, -0.6))}),
    'CompanyTender': dict(
        category='Tripo', kind='ship', master='World', maps=2048, turn=0.0, size=('length', 12.0), pivot='bottom',
        # The hull in slices along its rounded crown (its riflemen stand on it: CompanyTender.cpp RiflemanDeck), the bag,
        # its bow lamp and tail; its wheels none.
        boxes=[], collision=tc.company_tender),
    # --- Ship pieces and the hose (10, 13, 15) ---
    'GreatLantern': dict(
        category='Tripo', kind='prop', master='World', maps=1024, size=('height', 1.5), pivot='bottom', boxes=None,
        glow=dict(test=_bulb, region=((-0.09, -0.09, -0.12), (0.09, 0.09, 0.08))),
        sockets={'Light': ('glow', None)}),
    'GrappleHook': dict(
        category='Tripo', kind='prop', master='World', maps=1024, size=('height', 1.0), pivot='center', boxes=None,
        # The chain eye at its top: where the line ties on.
        sockets={'Eye': ('top', None)}),
    'DredgeHose': dict(
        # Stood on end: its axis is Z (as the engine cylinder WORLD's hose was built from), its middle the pivot.
        category='Tripo', kind='prop', master='World', maps=1024, tilt=('X', 90.0), size=('width', 1.5),
        pivot='center', boxes=None),
    'HoseIntake': dict(
        # Mouth down on the Sink's floor, the narrow end up where the hose comes in.
        category='Tripo', kind='prop', master='World', maps=1024, tilt=('X', 180.0), size=('max', 3.0),
        pivot='bottom', boxes=[], collision=tc.hose_intake, sockets={'Hose': ('topcenter', None)}),
    # --- World pieces (16-18, 20) ---
    'AnchorWinch': dict(
        category='Tripo', kind='prop', master='World', maps=1024, size=('height', 1.2), pivot='bottom', boxes=[],
        collision=tc.anchor_winch, sockets={'Top': ('top', None)}),
    'LanternPost': dict(
        # Arm toward +X (Tripo hung the lantern off its -Y: already the front), pivot at the post's foot.
        category='Tripo', kind='prop', master='World', maps=1024, size=('height', 3.2), pivot=('post', (0.26, 0.5)),
        boxes=[], collision=tc.lantern_post,
        glow=dict(test=_glass, region=((-0.20, -0.62, 0.0), (0.20, -0.36, 0.55))),
        sockets={'Light': ('glow', None), 'Beacon': ('glow', None), 'Wake': ('ue', (170.0, 0.0, 0.0)),
                 'Speaker': ('ue', (0.0, 0.0, 330.0)), 'Bell': ('glb', (0.0, 0.31, 0.45)),
                 # Hob's crows (RoostKit: Perch_1-6, yaw): the post's top, along the arm both ways, the brace.
                 'Perch_1': ('onto', (0.0, 0.0, 0.0)), 'Perch_2': ('onto', (40.0, 0.0, 90.0)),
                 'Perch_3': ('onto', (75.0, 0.0, -90.0)), 'Perch_4': ('onto', (110.0, 0.0, 90.0)),
                 'Perch_5': ('onto', (-40.0, 0.0, 0.0)), 'Perch_6': ('onto', (25.0, 0.0, -90.0))}),
    'LoudspeakerPost': dict(
        category='Tripo', kind='prop', master='World', maps=1024, turn=-90.0, size=('height', 4.0),
        pivot=('post', (0.2, 0.7)), boxes=[], post_box=0.06,
        sockets={'Horns': ('glb', (0.12, 0.0, 0.62))}),
    'BarricadeTable': dict(
        category='Tripo', kind='prop', master='World', maps=1024, turn=0.0, size=('max', 1.6), pivot='bottom',
        boxes=[], collision=tc.barricade_table),
    'BarricadeBoards': dict(
        category='Tripo', kind='prop', master='World', maps=1024, turn=0.0, size=('max', 2.0), pivot='bottom',
        boxes=[], collision=tc.barricade_boards),
    'BarricadeSacks': dict(
        category='Tripo', kind='prop', master='World', maps=1024, size=('max', 1.2), pivot='bottom', boxes=[],
        collision=tc.barricade_sacks),
    # --- Held and story props (7-9, 19): /Game/Art/Props, where the game's paths already point ---
    'DeliaShotgun': dict(
        category='Props', kind='gun', master='Gun', maps=1024, align=('gun', (0.0, -1.0, 0.0)), size=('length', 1.1),
        pivot='center', boxes=None,
        # Along its length from the muzzle (0) to the butt (1): the wrist behind the trigger, the forend under the barrels.
        sockets={'Muzzle': ('muzzle', None), 'Grip': ('along', 0.68), 'Foregrip': ('along', 0.38)}),
    'AbelsHat': dict(
        category='Props', kind='prop', master='World', maps=1024, turn=-90.0, size=('max', 0.45), pivot='bottom',
        boxes='box'),
    'CompanyRifle': dict(
        category='Tripo', kind='gun', master='Gun', maps=1024, align=('gun', (1.0, 1.0, 0.0)), size=('length', 1.1),
        pivot='center', boxes=None,
        sockets={'Muzzle': ('muzzle', None), 'Grip': ('along', 0.62), 'Foregrip': ('along', 0.35)}),
    'HoseNozzle': dict(
        category='Tripo', kind='gun', master='Gun', maps=1024, turn=180.0, size=('length', 0.45), pivot='center',
        boxes=None, sockets={'Muzzle': ('muzzle', None)}),
}


def names(category):
    """The models of one source (its category folder), filtered by LOOTER_TRIPO_ONLY=Name,Name when set."""
    only = [n for n in os.environ.get('LOOTER_TRIPO_ONLY', '').split(',') if n]
    return [n for n, s in SPECS.items() if s['category'] == category and (not only or n in only)]


def log(message):
    print(f'LOOTER: {message}', flush=True)


# --- Maps ---

def write_maps(maps, name, master, size, kind, tint=None, **props):
    """Tripo's maps as the texture set Tripo<name> (Art/Textures/Tripo<name>/T_Tripo<name>_BC/_ORM/_N, at most size
    px): the colour sharpened and graded toward the game's look, occlusion from Tripo's ORM (red) when it made one,
    roughness floored, the normal map turned to DirectX. Returns the material Tripo<name> on master."""
    set_name = 'Tripo' + name
    bc_path, orm_path, n_path = (lt.texture_path(set_name, k) for k in ('BC', 'ORM', 'N'))
    os.makedirs(os.path.dirname(bc_path), exist_ok=True)
    tripo._save_colour(maps['BC'], bc_path, size)
    img = bpy.data.images.load(bc_path, check_existing=False)
    rgb = tripo.pixels(img)[..., :3].astype(np.float32)
    bpy.data.images.remove(img)
    px = rgb.shape[0]
    soft = lt.blur(rgb, 1.2 * px / 1024.0)
    sharp = rgb + 0.85 * (rgb - soft)
    sat, contrast = GRADE[kind]
    luma = (sharp @ np.array([0.299, 0.587, 0.114], np.float32))[..., None]
    sharp = luma + (sharp - luma) * sat
    sharp = 0.5 + (sharp - 0.5) * contrast
    out = np.clip(sharp, 0.0, 1.0)
    mask = bpy.data.images.get(maps['BC'].get('AlphaMask', ''))
    if mask is not None:
        # A mask the material reads from the colour map's alpha (the barge walls' paint-chip weight,
        # looter_tripo_barge_paint: MI_TripoDredgeBarge_Walls' DetailMaskAlpha), kept as it is (no grade); carried
        # beside the colour image, since Blender premultiplies an image's own alpha into its colour.
        out = np.concatenate([out, tripo.pixels(mask, px)[..., :1]], axis=-1)
    lt.write_png(bc_path, lt.to8(out))
    rm = maps.get('RM')
    if rm is not None:
        data = tripo.pixels(rm, min(size, rm.size[0]))[..., :3].astype(np.float32)
        # glTF: green roughness, blue metal; Tripo's ORM files carry their occlusion in red (a bare metal-roughness
        # map's red is unused).
        occlusion = data[..., 0] if rm.name.upper().startswith('ORM') else np.ones_like(data[..., 0])
        metal = data[..., 2]
        rough = np.maximum(data[..., 1], ROUGH_FLOOR[kind] * (1.0 - metal) + 0.2 * metal)
        orm = np.stack([np.clip(occlusion * 0.85 + 0.15, 0.0, 1.0), rough, metal], axis=-1)
    else:
        orm = tripo.guess_orm(tripo.pixels(maps['BC'], min(size, maps['BC'].size[0]))[..., :3])
    lt.write_png(orm_path, lt.to8(orm))
    if maps.get('N') is not None:
        n = tripo.pixels(maps['N'], min(size, maps['N'].size[0]))[..., :3]
        n[..., 1] = 1.0 - n[..., 1]
        lt.write_png(n_path, lt.to8(n))
    elif os.path.exists(n_path):
        os.remove(n_path)
    log(f'{set_name}: maps {px} px, graded {GRADE[kind]}, roughness floor {ROUGH_FLOOR[kind]}, '
        f'occlusion {"Tripo" if rm is not None and rm.name.upper().startswith("ORM") else "none"}')
    return lt.material(set_name, name=set_name, master=master, tint=tint, **props)


# --- Mesh ---

def _arrays(obj):
    me = obj.data
    V = np.empty(len(me.vertices) * 3)
    me.vertices.foreach_get('co', V)
    C = np.empty(len(me.polygons) * 3)
    me.polygons.foreach_get('center', C)
    return V.reshape(-1, 3), C.reshape(-1, 3)


def _triangles(obj):
    return sum(len(p.vertices) - 2 for p in obj.data.polygons)


def _faces_in(centres, box):
    lo, hi = np.array(box[0]), np.array(box[1])
    return np.nonzero(np.all((centres >= lo) & (centres <= hi), axis=1))[0]


def _faces_by_colour(obj, image, faces, test):
    """Of faces, those whose base colour at the UV centre passes test (vectorised; sRGB 0..1)."""
    if len(faces) == 0:
        return []
    px = tripo.pixels(image, min(1024, image.size[0]))[..., :3]
    h, w = px.shape[:2]
    me = obj.data
    uv = np.empty(len(me.loops) * 2)
    me.uv_layers.active.data.foreach_get('uv', uv)
    uv = uv.reshape(-1, 2)
    starts = np.empty(len(me.polygons), dtype=np.int64)
    counts = np.empty(len(me.polygons), dtype=np.int64)
    me.polygons.foreach_get('loop_start', starts)
    me.polygons.foreach_get('loop_total', counts)
    cu = np.array([uv[starts[f]:starts[f] + counts[f]].mean(axis=0) for f in faces])
    x = np.clip(((cu[:, 0] % 1.0) * w).astype(int), 0, w - 1)
    y = np.clip(((1.0 - (cu[:, 1] % 1.0)) * h).astype(int), 0, h - 1)
    return [int(f) for f, ok in zip(faces, test(px[y, x])) if ok]


def _gun_frame(V, hint):
    """The rotation turning a gun lying any way onto the game's: its barrel's line along -Y (toward the muzzle), up +Z.
    The barrel's line is fitted through the middles of slices of its muzzle third (the stock's drop would tilt a fit of the
    whole gun)."""
    c = V.mean(axis=0)
    _, _, vt = np.linalg.svd(V - c, full_matrices=False)
    a = vt[0] * (1.0 if np.dot(vt[0], hint) >= 0 else -1.0)
    t = (V - c) @ a
    t0, t1 = t.min(), t.max()
    mids = []
    for k in range(8):
        lo = t1 - (t1 - t0) * 0.32 * (k + 1) / 8.0
        hi = t1 - (t1 - t0) * 0.32 * k / 8.0
        sel = (t >= lo) & (t <= hi)
        if sel.sum() > 10:
            mids.append(V[sel].mean(axis=0))
    mids = np.array(mids)
    if len(mids) >= 3:
        _, _, vt2 = np.linalg.svd(mids - mids.mean(axis=0), full_matrices=False)
        f = vt2[0] * (1.0 if np.dot(vt2[0], a) >= 0 else -1.0)
    else:
        f = a
    up = np.array([0.0, 0.0, 1.0])
    u = up - f * np.dot(f, up)
    u /= np.linalg.norm(u)
    s = np.cross(f, u)
    src = np.stack([f, u, s], axis=1)
    dst = np.stack([np.array([0.0, -1.0, 0.0]), np.array([0.0, 0.0, 1.0]), np.cross([0.0, -1.0, 0.0], [0.0, 0.0, 1.0])], axis=1)
    return Matrix((dst @ src.T).tolist()).to_4x4()


def ue_to_blender(p):
    """Unreal cm (X front, Y right) to the model's Blender meters (front -Y): the inverse of looter_export.to_unreal."""
    return Vector((-p[1] / 100.0, -p[0] / 100.0, p[2] / 100.0))


def blender_to_ue(p):
    return (-p[1] * 100.0, -p[0] * 100.0, p[2] * 100.0)


def fitted(name):
    """SPECS[name]'s Tripo mesh loaded, cut, its glow faces and mesh-found sockets measured in Tripo's space, then turned,
    sized and pivoted into the model's frame (Blender metres, front -Y). Returns (obj, maps, report, found, glow_faces);
    report['M'] is the transform from Tripo's space."""
    spec = SPECS[name]
    obj, maps = tripo.load(os.path.join(TRIPO_DIR, name, name + '.glb'))
    V, C = _arrays(obj)
    report = {'source tris': len(obj.data.polygons), 'source box': (V.min(0).round(3).tolist(), V.max(0).round(3).tolist())}

    for box in spec.get('cut', ()):
        doomed = _faces_in(C, box)
        tripo._delete_faces(obj, doomed)
        report['cut faces'] = report.get('cut faces', 0) + len(doomed)
        V, C = _arrays(obj)

    glow_faces = []
    if spec.get('glow'):
        g = spec['glow']
        glow_faces = _faces_by_colour(obj, maps['BC'], _faces_in(C, g['region']), g['test'])
        report['glow faces'] = len(glow_faces)

    # Sockets found on the mesh in Tripo's space, kept as points to carry through the same transform.
    found = {}
    for sock, (how, arg) in spec.get('sockets', {}).items():
        if how == 'glb':
            found[sock] = Vector(arg)
        elif how == 'stack':
            near = np.hypot(V[:, 0] - arg[0], V[:, 1] - arg[1]) < 0.05
            if near.any():
                top = V[near][:, 2].max()
                ring = near & (V[:, 2] > top - 0.01)
                found[sock] = Vector((V[ring][:, 0].mean(), V[ring][:, 1].mean(), top))
        elif how == 'glow' and glow_faces:
            found[sock] = Vector(C[glow_faces].mean(axis=0))

    # Turn, tilt or align, then size, then pivot.
    R = Matrix.Rotation(math.radians(spec.get('turn', 0.0)), 4, 'Z')
    if spec.get('tilt'):
        axis, degrees = spec['tilt']
        R = Matrix.Rotation(math.radians(degrees), 4, axis) @ R
    if spec.get('align'):
        R = _gun_frame(V, np.array(spec['align'][1], float)) @ R
    obj.data.transform(R)
    V, _ = _arrays(obj)
    dims = V.max(0) - V.min(0)
    what, meters = spec['size']
    measure = {'length': dims[1], 'width': dims[0], 'height': dims[2], 'max': dims.max()}[what]
    s = meters / measure
    S = Matrix.Scale(s, 4)
    obj.data.transform(S)
    V, _ = _arrays(obj)
    lo, hi = V.min(0), V.max(0)
    pivot = spec['pivot']
    if pivot == 'center':
        origin = (lo + hi) / 2.0
    elif pivot == 'bottom':
        origin = np.array([(lo[0] + hi[0]) / 2.0, (lo[1] + hi[1]) / 2.0, lo[2]])
    else:
        # A post: its axis is the middle of the mesh between two heights (shares of its height), where only the post is.
        f0, f1 = pivot[1]
        z0, z1 = lo[2] + (hi[2] - lo[2]) * f0, lo[2] + (hi[2] - lo[2]) * f1
        band = V[(V[:, 2] > z0) & (V[:, 2] < z1)]
        origin = np.array([np.median(band[:, 0]), np.median(band[:, 1]), lo[2]])
    T = Matrix.Translation(Vector(-origin))
    obj.data.transform(T)
    M = T @ S @ R
    V, _ = _arrays(obj)
    lo, hi = V.min(0), V.max(0)
    report['scale'] = round(s, 4)
    report['size m'] = (hi - lo).round(3).tolist()
    report['M'] = M
    return obj, maps, report, found, glow_faces


def build(name):
    """Builds SPECS[name] in the open scene as the model object <name> at the origin. Returns it and what was measured."""
    spec = SPECS[name]
    obj, maps, report, found, glow_faces = fitted(name)
    M = report['M']
    if spec.get('rework'):
        # The model reshaped by script on Tripo's fitted mesh (the barge: looter_tripo_barge), before its maps, sockets and
        # collision, so every export reproduces it. Glow faces are found by index before it, so a reworked spec has none.
        if glow_faces:
            raise ValueError(f'{name}: a reworked model cannot carry glow faces (their indices move)')
        spec['rework'](obj, report, maps)
        report['reworked tris'] = _triangles(obj)
    own = {}
    for key, extra in spec.get('sets', {}).items():
        # Faces on a texture set of their own, Tripo<name><key> (the barge's walls), cut from the GLB's maps before any
        # repack moves the rest: looter_tripo_repack.own_set; maps[key] carries them to the paint and write_maps.
        faces = extra['faces'](obj)
        if faces is not None and np.asarray(faces).any():
            own[key] = (np.asarray(faces), repack.own_set(obj, maps, name + key, extra['maps'], faces, report=report))
    if spec.get('repack'):
        # The atlas re-packed to fill its square, the maps re-cut from the GLB's 4096 delivery onto it (looter_tripo_repack;
        # Docs/Work/2026-11-04-Art/REPACK.md), on the final mesh, before any paint and write_maps.
        rest = np.ones(len(obj.data.polygons), bool)
        for faces, _ in own.values():
            rest[faces] = False
        maps = repack.repack(obj, maps, name, spec['maps'], polys=np.flatnonzero(rest) if own else None, report=report)
    for key, (_, sub) in own.items():
        if sub:
            maps[key] = sub
    if spec.get('paint'):
        # Texels of maps repainted by script on the final UVs (the barge's house and casing walls in our sets:
        # looter_tripo_barge_paint), after the rework and any repack, so write_maps saves them.
        spec['paint'](obj, report, maps)
    V, _ = _arrays(obj)
    lo, hi = V.min(0), V.max(0)
    report['size m'] = (hi - lo).round(3).tolist()

    # Materials: the graded texture set on its master; lantern glass on the shared LanternGlow.
    mat = write_maps(maps, name, spec['master'], spec['maps'], spec['kind'], tint=spec.get('tint'))
    lt.assign(obj, mat)
    if spec.get('slots'):
        # The reworked model's faces split into material slots on the same texture set.
        spec['slots'](obj, 'Tripo' + name, spec['master'])
    for key, extra in spec.get('sets', {}).items():
        if maps.get(key):
            # The own set's maps, and its slot's material moved onto it (slots made it on the main set). The slot must
            # hold exactly the faces the set was cut for (slots and extra['faces'] apply one rule: a mismatch stops here).
            names = [m.name if m else '' for m in obj.data.materials]
            idx = np.empty(len(obj.data.polygons), np.int64)
            obj.data.polygons.foreach_get('material_index', idx)
            on_slot = set(np.flatnonzero(idx == names.index(extra['slot'])).tolist()) if extra['slot'] in names else set()
            faces = own[key][0]
            cut = set((np.flatnonzero(faces) if faces.dtype == bool else faces).tolist())
            if on_slot != cut:
                raise ValueError(f"{name}: slot {extra['slot']} holds {len(on_slot)} faces, its set was cut for {len(cut)}")
            write_maps(maps[key], name + key, spec['master'], extra['maps'], spec['kind'], tint=spec.get('tint'))
            lt.material('Tripo' + name + key, name=extra['slot'], master=spec['master'], tint=spec.get('tint'))
    if glow_faces:
        glow = lm.material(GLOW_NAME, 0xffc56e, Glow=5.0, Variation=0.04)
        lt.assign(obj, glow, glow_faces)

    obj.name = name
    obj.data.name = name
    obj['Nanite'] = 0
    obj['LODs'] = LODS
    obj['LODScreens'] = spec.get('lod_screens', LOD_SCREENS)

    # Sockets (an empty's -Y is the socket's front: none are turned, so every socket faces the model's front).
    def put(sock, p_blender, yaw=0.0):
        # An Unreal yaw is a turn the other way about Blender's Z (the axes' handedness).
        lm.socket(obj, sock, tuple(p_blender), (0.0, 0.0, -yaw))
        report.setdefault('sockets', {})[sock] = tuple(round(v, 1) for v in blender_to_ue(p_blender))
    moved = report.get('socket_at', {})
    for sock, (how, arg) in spec.get('sockets', {}).items():
        if sock in found:
            p = M @ found[sock]
            if sock in moved:
                # Its piece moved in the rework: the new (x, y) in Unreal cm, the height as found.
                p = ue_to_blender((moved[sock][0], moved[sock][1], p.z * 100.0))
            put(sock, p)
        elif how == 'ue':
            put(sock, ue_to_blender(arg))
        elif how == 'onto':
            # On top of whatever is there (the highest point within 5 cm of that spot), facing yaw.
            pb = ue_to_blender((arg[0], arg[1], 0.0))
            near = np.hypot(V[:, 0] - pb.x, V[:, 1] - pb.y) < 0.05
            put(sock, Vector((pb.x, pb.y, V[near][:, 2].max() if near.any() else hi[2])), arg[2])
        elif how == 'top':
            top = V[V[:, 2] > hi[2] - 0.02 * (hi[2] - lo[2])]
            put(sock, Vector((top[:, 0].mean(), top[:, 1].mean(), hi[2])))
        elif how == 'topcenter':
            put(sock, Vector((0.0, 0.0, hi[2])))
        elif how == 'muzzle':
            tip = V[V[:, 1] < lo[1] + 0.012]
            put(sock, Vector((tip[:, 0].mean(), lo[1], tip[:, 2].mean())))
        elif how == 'along':
            y = lo[1] + (hi[1] - lo[1]) * arg
            band = V[np.abs(V[:, 1] - y) < 0.01]
            put(sock, Vector((band[:, 0].mean(), y, band[:, 2].mean())) if len(band) else Vector((0.0, y, 0.0)))
        elif how == 'glow':
            # No glass found: the lantern's middle (its glass region's centre, carried through).
            region = spec['glow']['region']
            put(sock, M @ Vector([(a + b) / 2.0 for a, b in zip(*region)]))
            log(f'warning: {name}: no glass faces found; {sock} at the glass region\'s middle')

    # Collision.
    boxes = spec.get('boxes')
    if boxes is None:
        obj['Collision'] = 'None'
    else:
        if boxes == 'box':
            boxes = [((0.0, 0.0, 0.0), (1.0, 1.0, 1.0))]
        for flo, fhi in boxes:
            # Fractions along Unreal's X (front, Blender -Y), Y (right, Blender -X) and Z.
            ue_lo = blender_to_ue(Vector((hi[0], hi[1], lo[2])))
            ue_hi = blender_to_ue(Vector((lo[0], lo[1], hi[2])))
            a = [ue_lo[i] + (ue_hi[i] - ue_lo[i]) * flo[i] for i in range(3)]
            b = [ue_lo[i] + (ue_hi[i] - ue_lo[i]) * fhi[i] for i in range(3)]
            pa, pb = ue_to_blender(a), ue_to_blender(b)
            centre = (pa + pb) / 2.0
            size = Vector([abs(pb[i] - pa[i]) for i in range(3)])
            lm.hull_box(obj, size, centre)
        if spec.get('post_box'):
            # The post itself, from its foot to its top.
            w = spec['post_box']
            lm.hull_box(obj, (w, w, hi[2]), (0.0, 0.0, hi[2] / 2.0))
    if spec.get('collision'):
        # Hulls fitted to the model's own parts (looter_tripo_collision; Docs/Work/2026-11-04-Art/COLLISION.md).
        spec['collision'](obj, report)
    report['tris'] = len(obj.data.polygons)
    report['triangles'] = _triangles(obj)
    return obj, report


def describe(name, report):
    """The import log's line: what was measured (bounds in cm, triangles, sockets)."""
    size = [round(v * 100.0, 1) for v in report['size m']]
    extra = {k: v for k, v in report.items() if k not in ('M', 'size m', 'source box')}
    log(f"{name}: {size[1]} long x {size[0]} wide x {size[2]} high cm, {extra}")
