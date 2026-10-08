"""Style Lab export, the last step: turns what the earlier steps left in Saved/StyleLab/work (the models, the level
build's placements, the terrain) into what the page loads, in Saved/StyleLab/export (the shared contract):

    manifest.json        models (pack, node, triangles, bounds, sockets, bones), one record per material name, the
                         texture sets, the terrain's files and M_Terrain's rules
    scene.json           instances (the game's own build, run under mock_unreal.py), lights, PCG's scatter as rules,
                         chimney smoke, creatures (the spawners' and the story's)
    models/pack_N.glb    the models, one top-level node each, at most 14 MB a pack
    tex/<Set>_{BC,N,ORM}.webp

    /home/user/bpyenv/bin/python Tools/StyleLab/export/lab_export.py

Run export_all.sh for the whole export; this step alone needs terrain_export.py's and scene_build.py's outputs.
"""
import glob
import json
import math
import os
import random
import sys
import time

sys.dont_write_bytecode = True
import labcommon as lc  # noqa: E402

lc.add_paths()

import numpy as np  # noqa: E402
from PIL import Image  # noqa: E402

import glbtool  # noqa: E402

OUT = lc.OUT
MODELS = os.path.join(lc.WORK, 'models')
TEX = os.path.join(lc.REPO, 'Art', 'Textures')
PACK_LIMIT = 13_000_000     # bytes: under 14 MB however the limit counts a megabyte
STARTED = time.time()
CORE = [-200.0, -200.0, 200.0, 200.0]
MARGIN_UE = 2500.0   # instances this far outside the region still count as in it (cm)


def log(message):
    print(f'STYLELAB: [{time.time() - STARTED:5.1f} s] {message}', flush=True)


def load_json(path):
    with open(path, encoding='utf-8') as f:
        return json.load(f)


LAYOUT = load_json(os.path.join(lc.REPO, 'Art', 'Levels', 'RansomsRest', 'layout.json'))
COMPUTED = load_json(os.path.join(lc.REPO, 'Art', 'Levels', 'RansomsRest', 'layout_computed.json'))
LEVEL = LAYOUT['level']


def poster_models():
    """The wanted posters and Calder's note are decals in the game (AWantedPoster: M_PosterDecal over the art's atlas
    T_Posters_BC, its cells in World/WantedPoster.h FWantedPosterAtlas, 1 m of wall per atlas width). The lab gets them
    as thin cards: Poster_Wanted (0.5 x 0.703 m) and Poster_Note (0.25 x 0.297 m), their picture facing the model's
    +z (out of the wall, the actor's +X), centred on the origin, 2 mm proud; UVs are the atlas cell (v = 0 at the top),
    material PosterDecal."""
    for name, (u0, v0, u1, v1) in (('Poster_Wanted', (0.0, 0.0, 0.5, 0.703125)),
                                   ('Poster_Note', (0.0, 0.703125, 0.25, 1.0))):
        w, h = (u1 - u0), (v1 - v0)
        pos = np.array([[-w / 2, h / 2, 0.002], [w / 2, h / 2, 0.002], [w / 2, -h / 2, 0.002], [-w / 2, -h / 2, 0.002]],
                       np.float32)
        nrm = np.tile(np.array([0, 0, 1], np.float32), (4, 1))
        uv = np.array([[u0, v0], [u1, v0], [u1, v1], [u0, v1]], np.float32)
        col = np.ones((4, 4), np.float32)
        idx = np.array([0, 2, 1, 0, 3, 2], np.uint16)
        blobs = [pos.tobytes(), nrm.tobytes(), uv.tobytes(), col.tobytes(), idx.tobytes()]
        views, binary = [], b''
        for b in blobs:
            views.append({'buffer': 0, 'byteOffset': len(binary), 'byteLength': len(b)})
            binary += b + b'\0' * ((4 - len(b) % 4) % 4)
        acc = [{'bufferView': 0, 'componentType': 5126, 'count': 4, 'type': 'VEC3',
                'min': pos.min(0).tolist(), 'max': pos.max(0).tolist()},
               {'bufferView': 1, 'componentType': 5126, 'count': 4, 'type': 'VEC3'},
               {'bufferView': 2, 'componentType': 5126, 'count': 4, 'type': 'VEC2'},
               {'bufferView': 3, 'componentType': 5126, 'count': 4, 'type': 'VEC4'},
               {'bufferView': 4, 'componentType': 5123, 'count': 6, 'type': 'SCALAR'}]
        gltf = {'asset': {'version': '2.0', 'generator': 'Style Lab exporter'}, 'scene': 0,
                'scenes': [{'nodes': [0]}], 'nodes': [{'name': name, 'mesh': 0}],
                'meshes': [{'name': name, 'primitives': [{'attributes': {'POSITION': 0, 'NORMAL': 1, 'TEXCOORD_0': 2,
                                                                         'COLOR_0': 3}, 'indices': 4,
                                                          'material': 0}]}],
                'materials': [{'name': 'PosterDecal', 'doubleSided': False,
                               'pbrMetallicRoughness': {'metallicFactor': 0.0, 'roughnessFactor': 0.9}}],
                'accessors': acc, 'bufferViews': views, 'buffers': [{'byteLength': len(binary)}]}
        glbtool.write(os.path.join(MODELS, name + '.glb'), gltf, binary)
        rec = {'tris': 2, 'skinned': False, 'bones': [], 'source': 'Synth/WantedPoster.h',
               'bounds': lc.r([[-w / 2, -h / 2, 0.0], [w / 2, h / 2, 0.002]], 4),
               'ueBounds': lc.r([[0.0, -w * 50, -h * 50], [0.2, w * 50, h * 50]], 2),
               'materials': {}, 'slots': ['PosterDecal'], 'sockets': {},
               'note': 'a decal in the game (AWantedPoster); a card here'}
        lc.write_json(os.path.join(MODELS, name + '.json'), rec, indent=1)


poster_models()


def model_db():
    db = {}
    for path in sorted(glob.glob(os.path.join(MODELS, '*.json'))):
        name = os.path.basename(path)[:-5]
        if name.startswith('_'):
            continue
        db[name] = load_json(path)
    return db


DB = model_db()


# --- Transforms ---

def to_three(m):
    """A UE world matrix (4x4, cm, columns) to three's position (m), Euler XYZ (radians) and scale."""
    m = np.asarray(m, dtype=np.float64).reshape(4, 4)
    cols = m[:3, :3]
    s = np.linalg.norm(cols, axis=0)
    rot = cols / np.where(s > 1e-12, s, 1.0)
    if np.linalg.det(rot) < 0.0:
        s[1] = -s[1]
        rot[:, 1] = -rot[:, 1]
    t = m[:3, 3]
    x = rot[:, 0]
    if abs(x[2]) < 1e-6 and abs(rot[2, 2] - 1.0) < 1e-6:
        # Yaw alone (most of the level): rotation.y = -yaw, as the contract writes it.
        euler = [0.0, -math.atan2(x[1], x[0]), 0.0]
    else:
        euler = lc.euler_xyz(lc.ue_matrix_to_three(rot.tolist()))
    return (lc.r(lc.ue_to_three(t), 3), lc.r(euler, 5), lc.r(lc.ue_scale_to_three(s.tolist()), 4))


def ue_matrix(x, y, z, yaw=0.0, scale=1.0):
    m = np.eye(4)
    c, s = math.cos(math.radians(yaw)), math.sin(math.radians(yaw))
    m[:3, :3] = np.array([[c, -s, 0], [s, c, 0], [0, 0, 1]]) * scale
    m[:3, 3] = [x, y, z]
    return m


HEIGHTS = None


def ground_three(x3, z3):
    """The ground's height (three y) under three (x, z), from heights.bin."""
    global HEIGHTS
    if HEIGHTS is None:
        HEIGHTS = np.fromfile(os.path.join(OUT, 'terrain', 'heights.bin'), '<f4').reshape(1024, 1024)
    n = 1024
    fx = np.clip((x3 - CORE[0]) / (CORE[2] - CORE[0]) * n - 0.5, 0, n - 1.001)
    fz = np.clip((z3 - CORE[1]) / (CORE[3] - CORE[1]) * n - 0.5, 0, n - 1.001)
    i, j = int(fz), int(fx)
    tz, tx = fz - i, fx - j
    h = HEIGHTS
    return float((h[i, j] * (1 - tx) + h[i, j + 1] * tx) * (1 - tz) + (h[i + 1, j] * (1 - tx) + h[i + 1, j + 1] * tx) * tz)


def ground_ue(x, y):
    return ground_three(-y / 100.0, x / 100.0) * 100.0


def in_region(x, y, margin=MARGIN_UE):
    g = lc.REGION_UE
    return g['x0'] - margin <= x <= g['x1'] + margin and g['y0'] - margin <= y <= g['y1'] + margin


# --- What each placed thing is ---

def tag_of(model, folder, cls):
    n = model
    if cls == 'Chest' or n.startswith(('Strongbox', 'SupplyCrate', 'Ammo')):
        return 'loot'
    if n.startswith(('Cliff',)):
        return 'cliff'
    if n.startswith(('Outcrop_', 'DenRock', 'Rock_', 'Boulder_', 'FarRock', 'PebbleCluster')):
        return 'rock'
    if n.startswith('Grave_') or n.startswith('Cairn'):
        return 'grave'
    if n.startswith(('Fence', 'StoneWall', 'SinkWarning', 'HitchRail')):
        return 'fence'
    if n.startswith(('DeadTree', 'Apple', 'Far', 'Pine', 'Oak', 'Birch', 'FallenPine', 'Stump', 'Log_')):
        return 'tree'
    if n.startswith(('Sagebrush', 'Rabbitbrush', 'DryTuft', 'Juniper', 'Bush_', 'Fern', 'Larkspur', 'Reeds')):
        return 'scrub'
    if n.startswith(('Grass', 'TallGrass', 'Clover', 'Flowers')):
        return 'grass'
    if n.startswith(('SmokePlume', 'Waterfall', 'GravewindWisp', 'CanyonFog')):
        return 'effect'
    if n.startswith('Poster'):
        return 'decal'
    if n.startswith(('Web_', 'EggSac', 'Cocoon')):
        return 'prop'
    big = DB.get(model, {}).get('bounds')
    if folder.endswith('/Buildings') and big:
        (x0, y0, z0), (x1, y1, z1) = big
        if max(x1 - x0, z1 - z0) > 4.0 and y1 - y0 > 2.5:
            return 'building'
    return 'prop'


BIG_TAGS = ('building', 'cliff', 'rock', 'tree')


def is_big(model, scale):
    b = DB.get(model, {}).get('bounds')
    if not b:
        return False
    size = max(abs(b[1][i] - b[0][i]) * abs(scale[i]) for i in range(3))
    return size >= 6.0


# --- The scene ---

class Scene:
    def __init__(self):
        self.instances = []       # dicts (model, p, r, s, tag, id, overrides)
        self.lights = []
        self.smoke = []
        self.creatures = []
        self.scatter = []
        self.no_trees = []
        self.skipped = {}

    def add(self, model, matrix, tag=None, label='', folder='', cls='', overrides=None, keep=None):
        if model not in DB:
            self.skipped[model] = self.skipped.get(model, 0) + 1
            return
        m = np.asarray(matrix, dtype=np.float64).reshape(4, 4)
        p, r, s = to_three(m)
        tag = tag or tag_of(model, folder, cls)
        if keep is None:
            keep = in_region(m[0, 3], m[1, 3]) or tag in BIG_TAGS or is_big(model, s)
        if not keep:
            return
        self.instances.append({'model': model, 'p': p, 'r': r, 's': s, 'tag': tag, 'id': label,
                               'overrides': overrides or {}})


def slot_overrides(model, materials):
    """{slot index: MI name} -> {original material name: new material name} (MI_ dropped)."""
    slots = DB.get(model, {}).get('slots', [])
    out = {}
    for slot, mi in (materials or {}).items():
        k = int(slot)
        if 0 <= k < len(slots):
            new = mi[3:] if mi.startswith('MI_') else mi
            if new != slots[k]:
                out[slots[k]] = new
    return out


def build_scene():
    raw = load_json(os.path.join(lc.WORK, 'scene_raw.json'))
    if raw.get('failed'):
        raise SystemExit('the level build failed under the mock:\n' + raw['failed'])
    sc = Scene()
    placements = COMPUTED['placements']
    swaps = {k[3:]: v[3:] for k, v in LEVEL.get('swaps', {}).items()}
    for actor in raw['actors']:
        cls, label, folder = actor['class'], actor['label'], actor['folder']
        if cls in ('DuskScenery', 'ColdOpenSet', 'BossSeal'):
            continue   # the Gravewind's wisps and the canyon fog show at dusk only; the cold open is a cutscene
        for mesh in actor['meshes']:
            if mesh['mesh'].startswith('RansomsRest_') or mesh.get('hidden'):
                continue   # the terrain's pieces: terrain_export.py's
            if mesh['mesh'] == 'SmokePlume':
                p = lc.ue_to_three(np.asarray(mesh['world']).reshape(4, 4)[:3, 3])
                sc.smoke.append({'p': lc.r(p, 3), 'note': f'chimney: {label.replace("Smoke_", "")}'})
                continue
            sc.add(mesh['mesh'], mesh['world'], label=label, folder=folder, cls=cls,
                   overrides=slot_overrides(mesh['mesh'], mesh['materials']))
        for inst in actor['instances']:
            ov = slot_overrides(inst['mesh'], inst['materials'])
            for k, mtx in enumerate(inst['matrices']):
                sc.add(inst['mesh'], mtx, label=f'{label}_{k}', folder=folder, cls=cls, overrides=ov)
        world = np.asarray(actor['world']).reshape(4, 4)
        props = actor.get('props', {})
        if cls == 'TrainStation' and isinstance(props.get('building_mesh'), dict) and not actor['meshes']:
            name = props['building_mesh'].get('asset', '')[3:]
            sc.add(name, world, tag='building', label=label, folder=folder, cls=cls)
        elif cls == 'KeepersLantern':
            for part in ('Web_Snare', 'KeepersLantern'):
                sc.add(part, world, tag='prop', label=f'{label}_{part}', folder=folder, cls=cls, keep=True)
        elif cls == 'HouseLights':
            lamp = props.get('lamp', {}).get('props', {})
            sc.lights.append({'type': 'point', 'p': lc.r(lc.ue_to_three(world[:3, 3]), 3), 'color': '#ffb468',
                              'intensity': float(props.get('day_lamp', lamp.get('intensity', 2.0))),
                              'duskIntensity': float(props.get('dusk_lamp', 5.5)),
                              'range': round(float(lamp.get('attenuation_radius', 1000.0)) / 100.0, 2),
                              'units': 'candela (Unreal); range in metres',
                              'note': f'lamp in {props.get("house", {}).get("actor", label)}'})
        elif cls == 'LanternFlame':
            sc.lights.append({'type': 'point', 'p': lc.r(lc.ue_to_three(world[:3, 3] + [0, 0, 40]), 3),
                              'color': '#ffa040', 'intensity': 4.0, 'range': 8.0, 'units': 'candela',
                              'note': 'the lit keeper\'s lantern at the depot (after Main 6)'})
        elif cls == 'WantedPoster':
            note = str(props.get('variant', '')).endswith('CALDER_NOTE')
            scaled = world.copy()
            k = float(props.get('scale', 1.0) or 1.0)
            scaled[:3, :3] = world[:3, :3] * k
            sc.add('Poster_Note' if note else 'Poster_Wanted', scaled, tag='decal', label=label, folder=folder,
                   cls=cls)
        elif cls == 'EncounterSpawner':
            creatures_of_spawner(sc, actor, world, props)
        elif cls in ('HobBird', 'MisterSexton', 'AmosWhitlock', 'AbelOnBoard'):
            story_character(sc, cls, actor, world, props)
    # The train waits at the depot (its cars are the C++ Train's: placed here at their layout spots).
    for key, model in (('hearseCar', 'HearseCar'), ('passengerCar', 'PassengerCar'), ('locomotive', 'Locomotive_B')):
        spot = placements.get(key)
        if spot:
            x, y, z = spot['location']
            sc.add(model, ue_matrix(x, y, z, spot['yaw']), tag='building', label=key, folder='Train')
    # The Sink's egg sacs' and the den's ground spiders already come from their spawners; the scrub's listed points.
    scrub_points(sc)
    sc.scatter = scatter_rules()
    sc.no_trees = no_tree_rects(raw)
    # Instances keep the area's swaps on what PCG scatters too (level.swaps): the listed scrub.
    for inst in sc.instances:
        if not inst['overrides']:
            mats = DB[inst['model']].get('slots', [])
            inst['overrides'] = {m: swaps[m] for m in mats if m in swaps} if inst['tag'] in ('scrub', 'tree', 'grass') \
                and inst['id'].startswith('Scrub_') else {}
    log(f'scene: {len(sc.instances)} instances, {len(sc.lights)} lights, {len(sc.smoke)} smoke, '
        f'{len(sc.creatures)} creatures; not exported: {sc.skipped}')
    return sc, raw


def no_tree_rects(raw):
    """The NoTrees boxes the tree layers keep out of (build_area.py no_tree_zones: level.noTreeZones, each zone polygon's
    bounding box scaled by its share about its middle; a 64 cm TriggerBox scaled to that size, whose bounds PCG's
    Difference reads), in three metres [x0, z0, x1, z1]. Cross-checked against the boxes the mocked build placed."""
    placed = {a['label']: np.asarray(a['world']).reshape(4, 4) for a in raw['actors'] if a['class'] == 'TriggerBox'}
    zones = {z['id']: z['polygon'] for z in LAYOUT.get('zones', [])}
    out = []
    for zone_id, share in LEVEL.get('noTreeZones', []):
        xs, ys = [p[0] for p in zones[zone_id]], [p[1] for p in zones[zone_id]]
        cx, cy = (min(xs) + max(xs)) / 2.0, (min(ys) + max(ys)) / 2.0
        sx, sy = (max(xs) - min(xs)) * share, (max(ys) - min(ys)) * share
        box = placed.get(f'NoTrees_{zone_id}')
        if box is not None:
            # The mock's box: its middle and its 64 cm cube's scale.
            half = np.abs(np.diag(box[:3, :3])) * 32.0
            if abs(box[0, 3] - cx) > 1 or abs(box[1, 3] - cy) > 1 or abs(half[0] - sx / 2) > 1 or abs(half[1] - sy / 2) > 1:
                log(f'warning: NoTrees_{zone_id} differs from the layout\'s rule')
        out.append({'id': zone_id, 'share': share,
                    'rect': lc.r([-(cy + sy / 2) / 100.0, (cx - sx / 2) / 100.0, -(cy - sy / 2) / 100.0,
                                  (cx + sx / 2) / 100.0], 3)})
    return out


CREATURE_MODELS = {'SpiderCreature': ('Spider', 1.0, None), 'UnpaidCreature': ('Unpaid', 1.0, None),
                   'GravemotherCreature': ('Spider', 1.8, 'Pale'), 'SlimeCreature': ('Slime', 1.0, None)}


def creatures_of_spawner(sc, actor, world, props):
    sid = props.get('spawner_id', actor['label'])
    groups = props.get('groups') or []
    rng = random.Random(f'spawner {sid}')
    radius = float(props.get('spawn_radius', 400.0) or 400.0)
    points = [np.asarray(p['matrix']).reshape(4, 4) for p in props.get('spawn_points') or [] if isinstance(p, dict)
              and 'matrix' in p]
    spots = []
    total = sum(int(g.get('count', 1) or 1) for g in groups) or 1
    cx, cy = world[0, 3], world[1, 3]
    heights = {}
    for i in range(total):
        if i < len(points):
            local = world @ points[i]
            spots.append((local[0, 3], local[1, 3]))
            heights[i] = local[2, 3]   # the build's own spot, on the ground it traced (a lair's floor in its rock)
            continue
        for _ in range(60):
            a, d = rng.uniform(0, 2 * math.pi), radius * math.sqrt(rng.random()) * 0.7
            x, y = cx + d * math.cos(a), cy + d * math.sin(a)
            if all(math.hypot(x - sx, y - sy) > 220.0 for sx, sy in spots):
                break
        spots.append((x, y))
    k = 0
    for g in groups:
        cls = (g.get('creature_class') or {}).get('class', 'SpiderCreature')
        model, scale, variant = CREATURE_MODELS.get(cls, ('Spider', 1.0, None))
        for _ in range(int(g.get('count', 1) or 1)):
            x, y = spots[k]
            z = heights.get(k)
            k += 1
            if not in_region(x, y, 0.0):
                continue
            ground = ground_ue(x, y)
            # A spawn point stands on what the build traced (a lair's floor inside its rock); one far over the
            # terrain is a spot in the air (a marker's height), so the creature stands on the ground under it.
            z = ground if z is None or z < ground - 50.0 or z > ground + 250.0 else z
            yaw = rng.uniform(-math.pi, math.pi)
            entry = {'model': model + (f'__{variant}' if variant else ''), 'p': lc.r(lc.ue_to_three((x, y, z)), 3),
                     'yaw': round(yaw, 4), 'spawner': sid, 'rank': str(g.get('rank', '')).split('.')[-1]}
            if scale != 1.0:
                entry['scale'] = scale
            sc.creatures.append(entry)


def story_character(sc, cls, actor, world, props):
    model = {'HobBird': 'Hob', 'MisterSexton': 'MisterSexton', 'AmosWhitlock': 'Amos', 'AbelOnBoard': 'Abel'}[cls]
    at = world[:3, 3]
    yaw = math.atan2(world[1, 0], world[0, 0])
    if cls == 'HobBird':
        # Hob's perches: the first inside the region (Main 3's sign over Bright & Daughter, Main 4's chapel hood).
        for perch in props.get('perches') or []:
            loc = perch.get('location')
            if loc and in_region(loc[0], loc[1], 0.0):
                at, yaw = np.asarray(loc), math.radians(float(perch.get('yaw', 0.0)))
                break
    if not in_region(at[0], at[1], 0.0):
        return
    sc.creatures.append({'model': model, 'p': lc.r(lc.ue_to_three(at), 3), 'yaw': round(-yaw, 4),
                         'note': f'{cls} ({actor["label"]})'})


def scrub_points(sc):
    """The scrub the generator lists (layout_computed.json scrub): the Sink's floor tufts and sage, the rim's and
    crests' junipers, the sage groups; deterministic size and heading as PCG draws them (its ranges)."""
    scrub = COMPUTED.get('scrub', {})
    rng = random.Random(97)
    tables = (('pitTufts', (('DryTuft_A', 1), ('DryTuft_B', 3)), (0.85, 1.2), None),
              ('pitSage', (('Sagebrush_C', 1),), (0.6, 1.4), None),
              ('rimJunipers', (('Juniper_A', 2), ('Juniper_B', 1)), (0.9, 1.4), 30.0),
              ('crestJunipers', (('Juniper_B', 1),), (0.85, 1.3), 5.0),
              ('sageGroups', (('Sagebrush_A', 4), ('Sagebrush_B', 3), ('Sagebrush_C', 3)), (0.6, 1.4), None))
    for key, meshes, scale, turn in tables:
        names = [n for n, w in meshes for _ in range(w)]
        for i, pt in enumerate(scrub.get(key, [])):
            x, y = pt[0], pt[1]
            model = names[rng.randrange(len(names))]
            s = rng.uniform(*scale)
            if turn is not None:
                base = pt[2] if len(pt) > 2 and key == 'crestJunipers' else 0.0
                yaw = base + rng.uniform(-turn, turn)
            else:
                yaw = rng.uniform(-180.0, 180.0)
            far = not in_region(x, y)
            if far and key != 'crestJunipers':
                continue
            z = ground_ue(x, y) - 4.0
            sc.add(model, ue_matrix(x, y, z, yaw, s), tag='scrub', label=f'Scrub_{key}_{i}', keep=True)


# The layers build_island_scatter.py builds with trees=True, whose points take the "No trees" Difference (the NoTrees
# boxes): Trees, Meadow trees and Crease pines.
NO_TREE_RULES = ('trees', 'meadowTrees', 'creasePines')


def scatter_rules():
    """PCG's layers (Tools/Unreal/build_island_scatter.py build_graph and scrub_layers) as data. Each layer: candidates
    every `cell` metres over the core (moved up to jitter x cell), kept where the mask's value x a random number in
    [0, 1) >= keep, on ground whose normal's up part is between flatMin and flatMax (slopeMax = acos(flatMin), radians),
    off roads/buildings/dressing (avoidTags); `models` are picked by `weights`, uniformly scaled within `scale`, upright
    or tilted with the slope (`upright`), sunk `sink` metres, culled past `cull` metres (Medium's distance)."""
    swaps = {k[3:]: v[3:] for k, v in LEVEL.get('swaps', {}).items()}
    scatter_tex, masks_tex, scrub_tex = 'tex/Terrain_Scatter.webp', 'tex/Terrain_Masks.webp', 'tex/Terrain_Scrub.webp'
    layers = scrub_spec = COMPUTED.get('scrub', {}).get('layers', {})
    out = []

    def rule(name, models, weights, tex, channel, cell, keep, flat=0.85, flat_max=1.0, scale=(0.85, 1.2),
             upright=True, sink=0.0, cull=None, jitter=0.5, avoid=('road', 'building', 'water'), note='',
             density_scaling=False):
        models = [m for m in models if m in DB]
        if channel == 'A':
            # The maps' alpha channels are files of their own (<name>_A.webp, grayscale: read R).
            tex, channel = tex.replace('.webp', '_A.webp'), 'R'
        out.append({'name': name, 'models': models, 'weights': weights[:len(models)], 'maskTex': tex, 'mask': channel,
                    'maskRect': CORE, 'cell': cell, 'perM2': round(1.0 / (cell * cell), 4), 'keep': keep,
                    'jitter': jitter, 'flatMin': round(flat, 4), 'flatMax': round(flat_max, 4),
                    'slopeMax': round(math.acos(max(-1.0, min(1.0, flat))), 4),
                    'slopeMaxDeg': round(math.degrees(math.acos(max(-1.0, min(1.0, flat)))), 1),
                    'scale': list(scale), 'upright': upright, 'sink': sink, 'cull': cull,
                    'mediumDensity': 0.4 if density_scaling else 1.0, 'avoidTags': list(avoid),
                    'noTrees': name in NO_TREE_RULES, 'note': note})

    rule('trees', ['Pine_A', 'Pine_B', 'Oak_A', 'Oak_B', 'Birch_A', 'Birch_B', 'DeadTree_A'], [1, 1, 3, 3, 3, 2, 0.4],
         scatter_tex, 'R', 6.0, 0.12, flat=0.8, sink=0.15, note='stands: a noise (2.5x scale) splits pines from '
         'broadleaf at 0.5; few in Ransom\'s Rest\'s valley (R is mostly the creases\' pines over 37 degrees)')
    rule('meadowTrees', ['Oak_A', 'Oak_B', 'Birch_A', 'Birch_B', 'Pine_A', 'Oak_C'], [3, 3, 2, 1, 1, 3], masks_tex,
         'B', 13.0, 0.8, flat=0.85, scale=(0.9, 1.25), sink=0.15, note='lone trees where the grass grows')
    rule('bushes', ['Bush_A', 'Bush_B', 'Bush_C'], [1, 1, 1], scatter_tex, 'R', 4.2, 0.1, scale=(0.75, 1.25),
         sink=0.05, cull=90.0)
    rule('forestFloor', ['Fern_A', 'Stump_A', 'Log_A'], [8, 1, 1], scatter_tex, 'R', 2.6, 0.3, scale=(0.8, 1.2),
         upright=False, cull=50.0)
    rule('grass', ['GrassClump_A', 'GrassClump_B', 'GrassClump_C', 'TallGrass_A', 'Clover_A'], [4, 3, 3, 2, 1],
         masks_tex, 'B', 0.9, 0.12, scale=(0.8, 1.25), upright=False, cull=45.0, density_scaling=True,
         avoid=('road', 'building', 'water', 'dressing'), note='a patch every 0.8 m2; fit 45 degrees to the slope')
    rule('flowers', ['Flowers_Yellow', 'Flowers_White', 'Flowers_Purple'], [1, 1, 1], scatter_tex, 'B', 2.3, 0.3,
         scale=(0.85, 1.2), upright=False, cull=45.0, density_scaling=True, avoid=('road', 'building', 'water',
                                                                                      'dressing'))
    rule('pebbles', ['PebbleCluster_A'], [1], scatter_tex, 'A', 1.7, 0.1, flat=0.7, scale=(0.8, 1.3), upright=False,
         cull=30.0, density_scaling=True)
    rule('rocks', ['Rock_A', 'Rock_B', 'Rock_C', 'Rock_D'], [1, 1, 1, 1], scatter_tex, 'A', 5.0, 0.15, flat=0.7,
         scale=(0.7, 1.4), sink=0.04, cull=90.0)
    rule('boulders', ['Boulder_A', 'Boulder_B', 'Boulder_C'], [1, 1, 1], scatter_tex, 'A', 12.0, 0.15, flat=0.7,
         scale=(0.8, 1.3), sink=0.1)
    rule('creasePines', ['Pine_A', 'Pine_B'], [1, 1], scatter_tex, 'R', 7.0, 0.15, flat=0.64, flat_max=0.8,
         sink=0.4, note='the ridges\' creases, 37-50 degrees')
    for key, title, models, weights, scale, cull in (
            ('junipers', 'junipers', ['Juniper_A', 'Juniper_B'], [2, 1], (0.9, 1.4), 252.0),
            ('rabbitbrush', 'rabbitbrush', ['Rabbitbrush_A'], [1], (0.8, 1.25), 162.0),
            ('sage', 'sagebrush', ['Sagebrush_A', 'Sagebrush_B', 'Sagebrush_C'], [4, 3, 3], (0.6, 1.4), 162.0),
            ('bigSage', 'bigSagebrush', ['Sagebrush_A', 'Sagebrush_B'], [1, 1], (1.0, 1.8), 252.0),
            ('tufts', 'dryTufts', ['DryTuft_A', 'DryTuft_B'], [1, 1], (0.85, 1.2), 40.0)):
        spec = scrub_spec.get(key)
        if not spec:
            continue
        lo, hi = spec.get('slopes', [0.0, 50.2])
        rule(title, models, weights, scrub_tex, spec['channel'], spec['cell'] / 100.0, spec['keep'],
             flat=math.cos(math.radians(hi)), flat_max=math.cos(math.radians(lo)) if lo > 0 else 1.0, scale=scale,
             sink=0.04, cull=cull, jitter=spec.get('jitter', 0.5), density_scaling=(key == 'tufts'),
             note='junipers face west within 30 degrees' if key == 'junipers' else '')
    for r in out:
        r['materialSwaps'] = {m: swaps[m] for model in r['models'] for m in DB[model].get('slots', []) if m in swaps}
    del layers
    return out


# --- Materials ---

def srgb(c):
    return [12.92 * v if v <= 0.0031308 else 1.055 * v ** (1 / 2.4) - 0.055 for v in c]


def linear(c):
    return [v / 12.92 if v <= 0.04045 else ((v + 0.055) / 1.055) ** 2.4 for v in c]


def hexc(c):
    return '#' + ''.join(f'{int(round(max(0.0, min(1.0, v)) * 255)):02x}' for v in c[:3])


def hex_to_linear(h):
    h = str(h).lstrip('#')
    return linear([int(h[i:i + 2], 16) / 255.0 for i in (0, 2, 4)])


SET_INFO = {}


def set_info(name):
    """A texture set's files and its mean colour (sRGB, alpha weighted), roughness and metal (the ORM's means)."""
    if name in SET_INFO:
        return SET_INFO[name]
    folder = os.path.join(TEX, name)
    bc = os.path.join(folder, f'T_{name}_BC.png')
    if not os.path.exists(bc):
        SET_INFO[name] = None
        return None
    n = os.path.join(folder, f'T_{name}_N.png')
    orm = os.path.join(folder, f'T_{name}_ORM.png')
    if name == 'SpiderBody_Pale':
        n, orm = (os.path.join(TEX, 'SpiderBody', f'T_SpiderBody_{k}.png') for k in ('N', 'ORM'))
    im = Image.open(bc)
    a = np.asarray(im.convert('RGBA').resize((256, 256), Image.BILINEAR), np.float32) / 255.0
    alpha = im.mode in ('RGBA', 'LA') and float(np.asarray(im.convert('RGBA'))[..., 3].min()) < 250
    w = (a[..., 3] > 0.5) if alpha else np.ones(a.shape[:2], bool)
    if not w.any():
        w[:] = True
    mean = a[..., :3][w].mean(0)
    info = {'bc': bc, 'n': n if os.path.exists(n) else None, 'orm': orm if os.path.exists(orm) else None,
            'mean': mean.tolist(), 'alpha': bool(alpha), 'size': im.size[0]}
    if info['orm']:
        o = np.asarray(Image.open(info['orm']).convert('RGB').resize((128, 128)), np.float32) / 255.0
        info['ormMean'] = o.reshape(-1, 3).mean(0).tolist()
    SET_INFO[name] = info
    return info


ALL_MATERIALS = {}
for _rec in DB.values():
    for _name, _mat in _rec.get('materials', {}).items():
        ALL_MATERIALS.setdefault(_name, _mat)

ROLE_BY_SET = {'HouseTrim': 'wood', 'WoodPlanks': 'wood', 'WoodEndGrain': 'wood', 'GunWood': 'wood', 'Hay': 'grass',
               'MetalRust': 'metal', 'MetalWorn': 'metal', 'PaintWorn': 'metal', 'ScreenMesh': 'metal',
               'StoneWall': 'stone', 'RockGranite': 'stone', 'RockCliff': 'rock', 'GroundGrass': 'grass',
               'GroundDirt': 'ground', 'BarkOak': 'bark', 'BarkBirch': 'bark', 'BarkPine': 'bark',
               'LeavesOak': 'foliage', 'LeavesBirch': 'foliage', 'NeedlesPine': 'foliage', 'FoliagePalette': 'foliage',
               'SpiderBody': 'creature', 'SpiderBody_Pale': 'creature', 'Webs': 'fabric', 'Posters': 'paper',
               'Polymer': 'gun'}
ROLE_WORDS = (('glow', ('Glow', 'Flame', 'Ember', 'Reticle')), ('glass', ('Glass', 'Lens', 'Window')),
              ('bone', ('Bone', 'Skull', 'Rib', 'Antler', 'Horn', 'Tooth', 'Teeth')),
              ('skin', ('Skin', 'Flesh', 'Face', 'Hand', 'Lip', 'Eye')),
              ('fabric', ('Cloth', 'Coat', 'Crepe', 'Fabric', 'Hat', 'Wool', 'Silk', 'Web', 'Ribbon', 'Rope', 'Leather',
                          'Felt', 'Shirt', 'Scarf', 'Veil', 'Canvas', 'Sack', 'Burlap', 'Vest', 'Trouser', 'Boot',
                          'Strap', 'Belt', 'Bandana', 'Feather', 'Cocoon', 'Shroud')),
              ('paper', ('Paper', 'Poster', 'Page', 'Ledger', 'Card', 'Book', 'Letter')),
              ('metal', ('Iron', 'Metal', 'Steel', 'Brass', 'Tin', 'Rust', 'Gun', 'Bell', 'Nail', 'Chain', 'Silver',
                         'Gold', 'Copper', 'Lantern')),
              ('wood', ('Wood', 'Plank', 'Board', 'Post', 'Coffin', 'Barrel', 'Crate', 'Log', 'Shake', 'Beam',
                        'Walnut', 'Oak', 'Pine')),
              ('stone', ('Stone', 'Granite', 'Brick', 'Marble', 'Plaster', 'Mortar')),
              ('rock', ('Rock', 'Cliff', 'Boulder', 'Pebble')),
              ('foliage', ('Leaf', 'Leaves', 'Needle', 'Moss', 'Bush', 'Fern', 'Sage', 'Juniper', 'Flower', 'Petal')),
              ('grass', ('Grass', 'Hay', 'Straw', 'Tuft')),
              ('water', ('Water',)), ('smoke', ('Smoke', 'Fog', 'Mist', 'Wisp')))
MASTER_MAP = {'World': 'World', 'WorldFoliage': 'WorldFoliage', 'Terrain': 'Terrain', 'Water': 'Water', 'Gun': 'Gun',
              'Glass': 'Glass', 'Smoke': 'Smoke', 'Waterfall': 'Waterfall', 'Gel': 'Glass', 'Ghost': 'Smoke',
              'GravewindWisp': 'Smoke', 'CanyonFog': 'Smoke', 'Backdrop': 'Flat'}


def role_of(name, master, set_name, kind, glow, source_category):
    if master in ('Glass',):
        return 'glass'
    if master in ('Water', 'Waterfall'):
        return 'water'
    if master == 'Smoke':
        return 'smoke'
    if master == 'Gun':
        return 'gun'
    if kind == 'Glow' or glow > 0.5:
        return 'glow'
    if set_name in ROLE_BY_SET:
        return ROLE_BY_SET[set_name]
    for role, words in ROLE_WORDS:
        if any(w.lower() in name.lower() for w in words):
            return role
    if source_category in ('Creatures',):
        return 'creature'
    if source_category in ('Characters',):
        return 'fabric'
    return 'other'


MATERIAL_SOURCE = {}
for _model, _rec in DB.items():
    for _name in _rec.get('materials', {}):
        MATERIAL_SOURCE.setdefault(_name, _rec.get('source', '').split('/')[0])


def material_record(name, src):
    props = src.get('props', {})
    game_master = props.get('Master')
    kind = props.get('Kind', 'Surface')
    if game_master:
        master = MASTER_MAP.get(game_master, 'World')
    else:
        master = 'Glow' if kind == 'Glow' else 'Flat'
    set_name = props.get('TextureSet') if game_master and game_master not in ('Glass', 'Gel', 'Backdrop') else None
    info = set_info(set_name) if set_name else None
    if set_name and info is None:
        set_name = None
    tint_lin = hex_to_linear(props['Tint']) if isinstance(props.get('Tint'), str) else [1.0, 1.0, 1.0]
    base_lin = src.get('base', [0.5, 0.5, 0.5, 1.0])[:3]
    if info:
        color_lin = [m * t for m, t in zip(linear(info['mean']), tint_lin)]
    elif game_master in ('Glass', 'Gel', 'Backdrop'):
        color_lin = tint_lin if isinstance(props.get('Tint'), str) else base_lin
    else:
        color_lin = base_lin
    glow = float(props.get('Glow', 0.0) or 0.0)
    if kind == 'Glow' and glow <= 0.0:
        glow = 1.0
    rough = float(src.get('roughness', 0.8))
    metal = float(src.get('metallic', 0.0))
    if info and info.get('ormMean'):
        rough, metal = info['ormMean'][1], info['ormMean'][2]
    two_sided = game_master in ('WorldFoliage', 'Glass', 'Gel', 'Smoke', 'Waterfall', 'GravewindWisp', 'CanyonFog',
                                'Ghost') or kind == 'Foliage'
    rec = {'master': master, 'set': set_name, 'tint': hexc(srgb(tint_lin)), 'tintLinear': lc.r(tint_lin, 4),
           'uvScale': float(props.get('UVScale', 1.0) or 1.0), 'color': hexc(srgb(color_lin)),
           'roughness': round(rough, 3), 'metallic': round(metal, 3), 'glow': round(glow, 3),
           'glowColor': hexc(srgb(color_lin if glow > 0 else [1.0, 0.7, 0.3])),
           'twoSided': bool(two_sided), 'alphaMask': bool(info and info['alpha'] and game_master == 'WorldFoliage'
                                                          or (info and info['alpha'] and set_name in ('Webs', 'Posters'))),
           'wind': bool(game_master == 'WorldFoliage' or float(props.get('Wind', 0) or 0) > 0),
           'role': role_of(name, master, set_name, kind, glow, MATERIAL_SOURCE.get(name, '')),
           'gameMaster': game_master or f'Stylized{kind}'}
    if game_master in ('Glass', 'Gel'):
        rec['opacity'] = float(props.get('Opacity', src.get('alpha', 0.3)))
    if game_master == 'Backdrop':
        rec['unlit'] = True
    extras = {k: v for k, v in props.items() if k not in ('Master', 'TextureSet', 'Tint', 'UVScale', 'Kind', 'Glow',
                                                          'DetailSets')}
    if extras:
        rec['params'] = extras
    return rec


def area_material(name, spec, base_name):
    """One of level.materials: its parent's record with the instance's values (vector Tint multiplies)."""
    parent = spec['parent'][3:]
    rec = dict(materials_for(parent) or {})
    if not rec:
        return None
    params = dict(rec.get('params', {}))
    vectors, scalars = spec.get('vectors', {}), spec.get('scalars', {})
    if 'Tint' in vectors:
        # The instance's Tint replaces its parent's (a material instance's parameter override).
        t = vectors['Tint'][:3]
        info = set_info(rec['set']) if rec.get('set') else None
        base = linear(info['mean']) if info else [a / max(b, 1e-4) for a, b in
                                                  zip(hex_to_linear(rec['color']), rec['tintLinear'])]
        rec['tintLinear'] = lc.r(t, 4)
        rec['tint'] = hexc(srgb(rec['tintLinear']))
        rec['color'] = hexc(srgb([a * b for a, b in zip(base, t)]))
    for k, v in vectors.items():
        if k != 'Tint':
            params[k] = lc.r(v, 4)
    for k, v in scalars.items():
        if k == 'Glow':
            rec['glow'] = v
        else:
            params[k] = v
    if 'TintVariation' in vectors:
        params['note'] = ('TintVariation x VariationAmount: each tree takes a random share of TintVariation instead of '
                          'Tint (late summer turning)')
    rec['params'] = params
    rec['gameInstance'] = f'MI_{name} (parent {spec["parent"]})'
    return rec


_MAT_CACHE = {}


def materials_for(name):
    if name in _MAT_CACHE:
        return _MAT_CACHE[name]
    rec = None
    spec = LEVEL.get('materials', {}).get('MI_' + name)
    if spec:
        rec = area_material(name, spec, spec['parent'][3:])
    elif name in ALL_MATERIALS:
        rec = material_record(name, ALL_MATERIALS[name])
    elif name == 'PosterDecal':
        info = set_info('Posters')
        rec = {'master': 'World', 'set': 'Posters' if info else None, 'tint': '#ffffff', 'tintLinear': [1, 1, 1],
               'uvScale': 1.0, 'color': hexc(info['mean']) if info else '#c8b48c', 'roughness': 0.9, 'metallic': 0.0,
               'glow': 0.0, 'glowColor': '#000000', 'twoSided': False, 'alphaMask': bool(info and info['alpha']),
               'wind': False, 'role': 'paper', 'gameMaster': 'PosterDecal (deferred decal)',
               'note': 'the posters\' atlas; the game draws it as a decal, the lab as a card (Poster_Wanted, '
                       'Poster_Note): no normal or ORM map'}
    elif name == 'SpiderBody_Pale':
        # The Gravemother's hide (MI_SpiderBody_Pale): SpiderBody's normal and ORM maps with the pale colour map.
        rec = dict(materials_for('SpiderBody'), set='SpiderBody_Pale', gameInstance='MI_SpiderBody_Pale')
        rec['color'] = hexc(set_info('SpiderBody_Pale')['mean'])
    _MAT_CACHE[name] = rec
    return rec


def terrain_materials():
    steep = LEVEL['materials'].get('MI_RansomsRestMacro_Steep', {})
    common = {'master': 'Terrain', 'tint': '#ffffff', 'tintLinear': [1, 1, 1], 'uvScale': 1.0, 'roughness': 0.9,
              'metallic': 0.0, 'glow': 0.0, 'glowColor': '#000000', 'twoSided': False, 'alphaMask': False,
              'wind': False, 'role': 'ground', 'gameMaster': 'Terrain'}
    macro = set_info_custom('tex/Terrain_Macro.webp', os.path.join(TEX, 'RansomsRestMacro', 'T_RansomsRestMacro_BC.png'))
    ring = set_info_custom('tex/Terrain_RingMacro.webp',
                           os.path.join(TEX, 'RansomsRestRingMacro', 'T_RansomsRestRingMacro_BC.png'))
    out = {
        'Terrain': dict(common, set=None, color=macro, macro='tex/Terrain_Macro.webp',
                        macroSelect='tex/Terrain_Macro_A.webp', macroRect=CORE,
                        params=dict(steep.get('scalars', {}), SteepTint=steep.get('vectors', {}).get('SteepTint')),
                        gameInstance='MI_RansomsRestMacro_Steep'),
        'TerrainRing': dict(common, set=None, color=ring, macro='tex/Terrain_RingMacro.webp',
                            macroSelect='tex/Terrain_RingMacro_A.webp',
                            macroRect=[-550.0, -550.0, 550.0, 550.0],
                            params=dict(steep.get('scalars', {}), SteepTint=steep.get('vectors', {}).get('SteepTint')),
                            gameInstance='MI_RansomsRestRingMacro_Steep'),
        'Water': {'master': 'Water', 'set': None, 'tint': '#ffffff', 'tintLinear': [1, 1, 1], 'uvScale': 1.0,
                  'color': '#2f5b63', 'roughness': 0.08, 'metallic': 0.0, 'glow': 0.0, 'glowColor': '#000000',
                  'twoSided': False, 'alphaMask': False, 'wind': False, 'role': 'water', 'gameMaster': 'Water'},
    }
    tints = (0x575e68, 0x7d8a8f, 0xa3adb5)   # area_model.py BACKDROP_TINTS, nearest first
    for i, t in enumerate(tints):
        c = '#%06x' % t
        out[f'Backdrop{i}'] = {'master': 'Flat', 'set': None, 'tint': c, 'tintLinear': lc.r(hex_to_linear(c), 4),
                               'uvScale': 1.0, 'color': c, 'roughness': 1.0, 'metallic': 0.0, 'glow': 0.0,
                               'glowColor': '#000000', 'twoSided': False, 'alphaMask': False, 'wind': False,
                               'role': 'other', 'gameMaster': 'Backdrop', 'unlit': True,
                               'note': 'unlit silhouette layer past the ring (nearest 0); the haze fogs it'}
    return out


def set_info_custom(rel, png):
    a = np.asarray(Image.open(png).convert('RGB').resize((256, 256)), np.float32) / 255.0
    return hexc(a.reshape(-1, 3).mean(0))


# --- Textures ---

BIG_SETS = ('GroundGrass', 'GroundDirt', 'RockCliff', 'HouseTrim', 'WoodPlanks', 'StoneWall', 'RockGranite',
            'MetalRust')


def write_sets(names):
    out = {}
    os.makedirs(os.path.join(OUT, 'tex'), exist_ok=True)
    for name in sorted(names):
        info = set_info(name)
        if info is None:
            continue
        entry = {'mean': hexc(info['mean']), 'alpha': info['alpha'], 'normal': 'DirectX'}
        for kind, path, size in (('bc', info['bc'], 1024 if name in BIG_SETS else 512), ('n', info['n'], 512),
                                 ('orm', info['orm'], 512)):
            if not path:
                continue
            rel = f'tex/{name}_{kind.upper()}.webp'
            im = Image.open(path)
            if kind == 'bc' and info['alpha']:
                # Cut-out sets (leaves, needles, webs, posters): RGB and alpha resized apart (Pillow resizes RGBA
                # premultiplied, which blackens the colour the source carries under alpha 0, and mip-mapped alpha
                # tests then fringe every leaf card dark), saved losslessly with that colour kept (exact).
                rgb, alpha = im.convert('RGB'), im.convert('RGBA').getchannel('A')
                if im.size[0] > size:
                    rgb, alpha = rgb.resize((size, size), Image.LANCZOS), alpha.resize((size, size), Image.LANCZOS)
                merged = Image.merge('RGBA', (*rgb.split(), alpha))
                merged.save(os.path.join(OUT, rel), 'WEBP', lossless=True, quality=100, method=6, exact=True)
            else:
                im = im.convert('RGB')
                if im.size[0] > size:
                    im = im.resize((size, size), Image.LANCZOS)
                im.save(os.path.join(OUT, rel), 'WEBP', quality=90 if kind != 'n' else 92, method=6)
            entry[kind] = rel
        out[name] = entry
    # The terrain's strata noise (M_Terrain's StrataNoise and band darkness).
    noise = os.path.join(TEX, 'MacroNoise', 'T_MacroNoise_M.png')
    if os.path.exists(noise):
        Image.open(noise).convert('L').resize((512, 512), Image.LANCZOS).save(
            os.path.join(OUT, 'tex', 'MacroNoise.webp'), 'WEBP', quality=92, method=6)
    log(f'textures: {len(out)} sets')
    return out


# --- Packs ---

EXTRA_MODELS = ['Spider', 'Unpaid', 'UnpaidHat', 'Abel', 'AbelHat', 'AbelLantern', 'AbelPump', 'Amos', 'AmosHat',
                'AmosFork', 'Hob', 'Slime', 'MisterSexton', 'SextonLedger', 'SextonStand', 'AmmoAssaultRifle',
                'AmmoPistol', 'AmmoSMG', 'AmmoShotgun', 'AmmoSniper', 'SupplyCrate', 'SupplyCrate_Lid', 'Strongbox',
                'Strongbox_Lid', 'Strongbox_Wheel', 'Gun_Bullpup', 'Gun_Ranchhand', 'SmokePlume', 'EggSac_A',
                'EggSac_B', 'EggSac_C', 'EggSac_Burst', 'KeepersLantern', 'Web_Snare']
VARIANTS = {'Spider__Pale': ('Spider', {'SpiderBody': 'SpiderBody_Pale'})}


def plan_models(sc):
    """Every model the page needs, with its material renames: the scene's (an instance's swaps; a model used with two
    different sets of swaps gets a variant <Model>__<n>), the scatter's, and the extras."""
    by_model = {}
    for inst in sc.instances:
        key = json.dumps(inst['overrides'], sort_keys=True)
        by_model.setdefault(inst['model'], {}).setdefault(key, 0)
        by_model[inst['model']][key] += 1
    plan = {}       # node name -> (source model, renames)
    rename_of = {}  # (model, key) -> node name
    for model in sorted(by_model):
        keys = sorted(by_model[model].items(), key=lambda kv: (-kv[1], kv[0]))
        for i, (key, _) in enumerate(keys):
            node = model if i == 0 else f'{model}__{i}'
            plan[node] = (model, json.loads(key))
            rename_of[(model, key)] = node
    for inst in sc.instances:
        inst['model'] = rename_of[(inst['model'], json.dumps(inst['overrides'], sort_keys=True))]
        del inst['overrides']
    for rule in sc.scatter:
        for model in rule['models']:
            if model not in plan:
                plan[model] = (model, {m: rule['materialSwaps'][m] for m in DB[model].get('slots', [])
                                       if m in rule['materialSwaps']})
    for model in EXTRA_MODELS:
        if model in DB and model not in plan:
            plan[model] = (model, {})
    for node, (model, renames) in VARIANTS.items():
        if model in DB:
            plan[node] = (model, renames)
    return plan


def renamed_glb(model, renames, node):
    """A model's GLB with its materials renamed and its top node (and mesh) named node; written to the work folder."""
    gltf, binary = glbtool.read(os.path.join(MODELS, model + '.glb'))
    for mat in gltf.get('materials', []):
        if mat.get('name') in renames:
            mat['name'] = renames[mat['name']]
    scene = gltf['scenes'][gltf.get('scene', 0)]
    if len(scene['nodes']) != 1:
        raise RuntimeError(f'{model}.glb has {len(scene["nodes"])} top-level nodes')
    top = scene['nodes'][0]
    # three's GLTFLoader makes every object name in a file unique (a second "body" bone becomes "body_1"), so a pack's
    # names must not repeat: each model's inner nodes and meshes carry its node name (Spider's bone femur_0_l is
    # Spider_femur_0_l, the pale variant's Spider__Pale_femur_0_l). The manifest maps them back (bones / gameBones).
    joints = {j for skin in gltf.get('skins', []) for j in skin['joints']}
    bones = []
    for i, n in enumerate(gltf['nodes']):
        if i == top:
            n['name'] = node
        elif i in joints:
            bones.append((n.get('name', f'bone{i}'), f'{node}_{n.get("name", f"bone{i}")}'))
            n['name'] = bones[-1][1]
        else:
            n['name'] = f'{node}_{n.get("name", f"node{i}").replace(model + "_", "", 1)}'
    for i, mesh in enumerate(gltf.get('meshes', [])):
        mesh['name'] = f'{node}_Mesh{i}'
    path = os.path.join(lc.WORK, 'packed', node + '.glb')
    os.makedirs(os.path.dirname(path), exist_ok=True)
    glbtool.write(path, gltf, binary)
    return path, [m.get('name') for m in gltf.get('materials', [])], bones


def write_packs(plan):
    for old in glob.glob(os.path.join(OUT, 'models', 'pack_*.glb')):
        os.remove(old)
    order = sorted(plan, key=lambda n: (DB[plan[n][0]].get('source', ''), n))
    packs, models, used_materials = [], {}, set()
    merger, count = glbtool.Merger(), 0

    def flush():
        nonlocal merger, count
        if count:
            rel = f'models/pack_{len(packs)}.glb'
            merger.save(os.path.join(OUT, rel))
            packs.append(rel)
            log(f'{rel}: {count} models, {os.path.getsize(os.path.join(OUT, rel)) / 2 ** 20:.2f} MB')
        merger, count = glbtool.Merger(), 0

    for node in order:
        model, renames = plan[node]
        path, mats, bone_names = renamed_glb(model, renames, node)
        size = os.path.getsize(path)
        if count and merger.size() + size > PACK_LIMIT:
            flush()
        merger.add(path)
        count += 1
        used_materials.update(mats)
        rec = DB[model]
        socks = rec.get('sockets', {})
        entry = {'pack': len(packs), 'node': node, 'tris': rec['tris'], 'bounds': rec['bounds'],
                 'sockets': {k: v['three'] for k, v in socks.items()},
                 'socketFrames': {k: {'p': v['three'], 'forward': v['threeForward'], 'up': v['threeUp']}
                                  for k, v in socks.items()},
                 'skinned': rec['skinned'], 'bones': rec['bones'], 'materials': sorted(set(mats)),
                 'source': ('Source/AI_Looter_Shooter/World/WantedPoster.h (a decal in the game)'
                            if rec.get('source', '').startswith('Synth') else 'Art/Models/' + rec.get('source', ''))}
        if rec.get('decimatedFrom'):
            entry['decimatedFrom'] = rec['decimatedFrom']
        if rec['skinned']:
            # bones: the node names in the GLB (what three finds); gameBones: the game's names, in the same order.
            glb_of = dict(bone_names)
            entry['bones'] = [glb_of.get(b, b) for b in rec['bones']]
            entry['gameBones'] = list(rec['bones'])
            entry['boneRest'] = {glb_of.get(b, b): dict(v, parent=glb_of.get(v.get('parent'), v.get('parent')))
                                 for b, v in rec.get('boneRest', {}).items()}
        if model != node:
            entry['variantOf'] = model
        if rec.get('forward'):
            entry['forward'] = rec['forward']
        if rec.get('parts'):
            entry['parts'] = rec['parts']
        models[node] = entry
    flush()
    return packs, models, used_materials


def main():
    sc, raw = build_scene()
    plan = plan_models(sc)
    packs, models, used = write_packs(plan)
    materials = {}
    for name in sorted(used):
        rec = materials_for(name)
        if rec is None:
            log(f'warning: no record for material {name}')
            rec = {'master': 'Flat', 'set': None, 'tint': '#ffffff', 'tintLinear': [1, 1, 1], 'uvScale': 1.0,
                   'color': '#808080', 'roughness': 0.8, 'metallic': 0.0, 'glow': 0.0, 'glowColor': '#000000',
                   'twoSided': False, 'alphaMask': False, 'wind': False, 'role': 'other'}
        materials[name] = rec
    terrain_mats = terrain_materials()
    rock = materials_for('RockCliff_Ransom')
    materials.update(terrain_mats)
    if rock:
        materials['RockCliff_Ransom'] = rock
    sets = {m['set'] for m in materials.values() if m.get('set')} | {'GroundGrass', 'GroundDirt', 'RockCliff'}
    texture_sets = write_sets(sets)
    for name, m in materials.items():
        if m.get('set') and m['set'] not in texture_sets:
            m['set'] = None
    manifest = {
        'version': 1,
        'generated': 'Tools/StyleLab/export (export_all.sh); Ransom\'s Rest from the game\'s own scripts',
        'axes': 'three metres: x = -Y/100, y = Z/100, z = X/100 (UE cm, X north, Y east); a UE yaw y is rotation.y = '
                '-y in radians; models face +z (UE +X)',
        'region': {'ue': lc.REGION_UE, 'three': lc.REGION_THREE},
        'packs': packs,
        'models': {k: models[k] for k in sorted(models)},
        'materials': {k: materials[k] for k in sorted(materials)},
        'textureSets': texture_sets,
        'vertexColors': 'COLOR_0 is linear RGBA; World/Gun: A = baked occlusion; WorldFoliage: R = wind weight, '
                        'G = phase, A = occlusion; terrain: A = baked occlusion',
        'mapConvention': 'every lab map (macro, masks, scatter, scrub, heights): row 0 at z0, column 0 at x0 of its '
                         'rect; load with flipY = false and sample at ((x - x0)/(x1 - x0), (z - z0)/(z1 - z0))',
        'terrain': terrain_block(),
    }
    lc.write_json(os.path.join(OUT, 'manifest.json'), manifest, indent=1)
    scene = {'version': 1, 'instances': sc.instances, 'lights': sc.lights, 'scatter': sc.scatter,
             'noTrees': sc.no_trees, 'smoke': sc.smoke,
             'creatures': sc.creatures,
             'notes': {'instances': 'everything the game\'s build (Tools/Unreal/build_area.py RansomsRest, run under '
                                    'mock_unreal.py) places in the region, and the big things past it (buildings, '
                                    'cliffs, rocks, far trees); id = the actor\'s label',
                       'lights': 'HouseLights lamps: intensity is the day lamp in candela, duskIntensity at dusk',
                       'creatures': 'the encounter spawners\' creatures (spread over their spawn radius, '
                                    'deterministic) and the story\'s characters standing in the region; the '
                                    'Gravemother is Spider__Pale at scale 1.8',
                       'scatter': scatter_rules.__doc__.strip(),
                       'noTrees': 'the boxes the rules with noTrees: true keep out of (build_area.py no_tree_zones: '
                                  'level.noTreeZones, each zone\'s bounding box scaled by its share about its middle), '
                                  'rect = [x0, z0, x1, z1] in three metres'}}
    lc.write_json(os.path.join(OUT, 'scene.json'), scene)
    log(f'manifest: {len(models)} models in {len(packs)} packs, {len(materials)} materials, '
        f'{len(texture_sets)} texture sets; scene: {len(sc.instances)} instances')


def terrain_block():
    return {
        'near': 'models/terrain_near.glb',
        'far': ['models/terrain_far.glb'],
        'nodes': {'near': ['Terrain_Near'], 'far': ['Terrain_Far', 'Terrain_Water', 'Terrain_Ring',
                                                    'Terrain_CanyonWall', 'Terrain_Backdrop_0', 'Terrain_Backdrop_1',
                                                    'Terrain_Backdrop_2']},
        'heights': {'file': 'terrain/heights.bin', 'w': 1024, 'h': 1024, 'rect': CORE,
                    'format': 'float32 little-endian, metres (three y), row-major; sample (r, c) is at x = x0 + (c + '
                              '0.5)(x1 - x0)/w, z = z0 + (r + 0.5)(z1 - z0)/h; ray-cast from the drawn triangles'},
        'macro': 'tex/Terrain_Macro.webp', 'macroRect': CORE,
        'macroSelect': 'tex/Terrain_Macro_A.webp',
        'macroSelectNote': 'grayscale (read R): the macro map\'s alpha, M_Terrain\'s detail selector: about 0 grass and '
                           'soil, 1 rock; dirt roads about 0.4, footpaths 0.25, scree 0.65. Every map is RGB or '
                           'grayscale with no alpha: a browser loses the colour under alpha 0',
        'ringMacro': 'tex/Terrain_RingMacro.webp', 'ringMacroSelect': 'tex/Terrain_RingMacro_A.webp',
        'ringMacroRect': [-550.0, -550.0, 550.0, 550.0],
        'masks': 'tex/Terrain_Masks.webp', 'masksA': 'tex/Terrain_Masks_A.webp',
        'masksNote': 'over macroRect, RGB: R = road/bare dirt (macro selector band 0.16-0.5, or no grass and no rock), '
                     'G = rock/cliff (selector 0.5-0.8, or slope 40-55 degrees), B = grass density (the scatter '
                     'mask\'s G); masksA (grayscale) = scrub (the scrub mask\'s strongest channel)',
        'scatterMask': 'tex/Terrain_Scatter.webp (RGB: R trees, G grass, B flowers) + tex/Terrain_Scatter_A.webp '
                       '(pebbles and rocks); linear, lossless',
        'scrubMask': 'tex/Terrain_Scrub.webp (RGB: R sagebrush, G dry tufts, B rabbitbrush) + '
                     'tex/Terrain_Scrub_A.webp (junipers); linear, lossless',
        'noise': 'tex/MacroNoise.webp',
        'uv': 'TEXCOORD_0 = the macro uv over macroRect (ringMacroRect on Terrain_Ring) in the map convention; '
              'TEXCOORD_1 = (x, z) in metres for the detail layers; COLOR_0.a = baked occlusion',
        'layers': [{'name': 'grass', 'set': 'GroundGrass', 'tile': 2.0, 'meanLuma': 0.3},
                   {'name': 'dirt', 'set': 'GroundDirt', 'tile': 2.0, 'meanLuma': 0.3,
                    'note': 'not used by M_Terrain itself (the macro map paints the roads); offered for the masks\' R'},
                   {'name': 'rock', 'set': 'RockCliff', 'tile': 4.0, 'meanLuma': 0.35}],
        'rules': TERRAIN_RULES,
    }


TERRAIN_RULES = (
    "M_Terrain (Tools/Unreal/build_world_materials.py build_terrain) with Ransom's Rest's MI_RansomsRestMacro_Steep. "
    "1) Macro: the macro map's RGB on TEXCOORD_0 is the ground's colour; its alpha in the game, here macroSelect "
    "(Terrain_Macro_A, grayscale), is Select, which picks the detail: 0 grass/soil, 1 rock. 2) Detail: Detail = lerp(GroundGrass BC tiled every 2 m (GrassScale 0.5 on metre UVs), RockCliff BC "
    "tiled every 4 m (RockScale 0.25), Select); Color = Macro * lerp(1, luma(Detail) / lerp(0.30, 0.35, Select), 0.6) "
    "(DetailStrength 0.6, GrassMeanLuma 0.3, RockMeanLuma 0.35): the detail only modulates the macro's brightness. "
    "3) Steep faces: Steep = 1 - smoothstep(cos 55 deg, cos 40 deg, normal.up) (SteepStart 40, SteepFull 55). There "
    "the rock map is laid on from the side (triplanar on the world X and Y planes, weights |n.xy|^4 normalised, UV = "
    "(along * 0.25, -height * 0.25) metres plus a slow wave and StrataWarp 1.5 m of macro-noise warp over StrataWave "
    "45 m), and Color = lerp(Color, Macro * SteepTint (0.8, 0.74, 0.68) * Banding * lerp(1, luma(SideRock) / 0.35, "
    "SteepDetail 1.15), Steep); Banding: noise B from MacroNoise at ((x + y) / 120 m, height / 25 m), b = (B - 0.5) * "
    "2 * SteepVariation (0.6): darker by 0.5 * max(-b, 0), redder toward (1.1, 0.9, 0.78) by max(b, 0). 4) Occlusion: "
    "Color *= lerp(1, COLOR_0.a, 0.45) (DiffuseAO), and the AO output is COLOR_0.a. 5) Normal: lerp(GroundGrass N, "
    "RockCliff N, Select) at strength 0.8 * (1 - Steep) (NormalStrength 0.8; DirectX maps: flip green for OpenGL), "
    "and on steep faces the side-laid rock normal at SteepNormalStrength 0.6. 6) Roughness = lerp(0.92, 0.82, "
    "max(Select, Steep)); metallic 0. The ring (TerrainRing) is the same material over its own macro map. The "
    "rendered grass, flowers and scrub come from the scatter rules (scene.json), not the material.")


if __name__ == '__main__':
    main()
