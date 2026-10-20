"""The other four heroes' first-person arms (Odessa, Crane, Gauge, Pike), as SK_FP_Arms is Ellis's: Art/Models/Characters/
FPArms<Hero>.py each calls build(<Hero>) here. Everything about the rig is Ellis's (Tools/Blender/fp_arms_base.py): the
same 46 bones on the mannequin's names, the root at the eye facing ahead, the same bind pose, so the hands code
(FFirstPersonHandsRig) binds each unchanged and every grip number holds; each writes its own
Intermediate/FPArms/FPArms_<Hero>_rig.json (FPArms.py's keys and values).

What differs is what's drawn round the bones, matched to each hero's third-person model (Art/Models/Characters/<Hero>.py,
its looks and colours read from it):
  Odessa  black kid gloves (slimmer fingers, three stitched points, a short flared cuff), the white shirt cuff showing
          2.8 cm, the wine tailcoat's fitted sleeve under its darker silk turned-back cuff
  Crane   bare grey-green dead hands (bony: thin fingers, swollen joints, tendons, nails gone yellow-grey), the slate
          coat's loose sleeve and its torn turned-back cuff
  Pike    bare weathered hands (heavy, dirt in the creases and under the nails), the khaki drill field coat's sleeve,
          its turned-back cuff with a buttoned tab and a brass button
  Gauge   iron: black enamel casings, brass bands, rivets and hinge pins, three riveted fingers (thumb on thumb_*, the
          two on index_* and middle_*; ring and pinky bones present, unweighted), sized near Ellis's sleeves so they
          don't fill the screen (Tools/Blender/fp_arms_gauge.py)

Each mesh: FP_Arms_<Hero> -> SK_FP_Arms_<Hero>, two material slots (Gauge one), its texture sets FPArms<Hero><Set>
(Tools/Blender/fp_arms_paint.py: the hero's own looks baked, the hand's detail painted over).

    Tools\\artrun.ps1 -Script Art\\Models\\Characters\\FPArms<Hero>.py -Threads 6 [-Preview] [-ScriptArgs keep-textures]
"""
import ast
import importlib
import json
import math
import os
import sys

import bpy
import numpy as np
from mathutils import Matrix

import looter_textures as lt
from fp_arms_base import A, C, WORK, log, smoothstep, unit, v3
import fp_arms_cloth as cl
import fp_arms_hands as hd
import fp_arms_paint as pt

ARGV = [a for arg in (sys.argv[sys.argv.index('--') + 1:] if '--' in sys.argv else []) for a in arg.split(',') if a]
REPO = lt.REPO
CHAR = os.path.join(REPO, 'Art', 'Models', 'Characters')
PORTRAITS = os.path.join(REPO, 'Art', 'Portraits')
BLENDER_DIR = os.path.join(REPO, 'Tools', 'Blender')
OWN = [os.path.join(BLENDER_DIR, f) for f in ('fp_arms_base.py', 'fp_arms_hands.py', 'fp_arms_cloth.py',
                                               'fp_arms_paint.py', 'fp_arms_heroes.py', 'fp_arms_gauge.py')]
SHARED = [A['__file__'], os.path.join(BLENDER_DIR, 'looter_looks.py'), os.path.join(BLENDER_DIR, 'looter_bake.py')]
# The arms' size against the guns the code should draw each hero at (UFirstPersonHandsComponent::ArmsScale; Ellis
# 0.88): Saved/FPArms/WIRING.md. The look sheet draws them so.
ARMS_SCALE = {'Ellis': 0.88, 'Odessa': 0.84, 'Crane': 0.88, 'Gauge': 0.88, 'Pike': 0.86}


# ------------------------------------------------------------------------------------- the models' own numbers
def _tree(path):
    with open(path, encoding='utf-8') as fh:
        return ast.parse(fh.read())


def model_literal(path, name, default):
    """A top-level NAME = <literal> in a model script (its colours), else default (logged)."""
    for node in _tree(path).body:
        if isinstance(node, ast.Assign) and any(isinstance(t, ast.Name) and t.id == name for t in node.targets):
            try:
                return ast.literal_eval(node.value)
            except ValueError:
                break
    log(f'note: {name} not read from {os.path.relpath(path, REPO)}; using {default}')
    return default


def portrait_option(path, key, field, default):
    """OPTIONS[key][field] of a portrait script (the picked look's skin), else default (logged)."""
    for node in ast.walk(_tree(path)):
        if isinstance(node, ast.Assign) and any(isinstance(t, ast.Name) and t.id == 'OPTIONS' for t in node.targets) \
                and isinstance(node.value, ast.Dict):
            for k, v in zip(node.value.keys, node.value.values):
                if isinstance(k, ast.Constant) and k.value == key and isinstance(v, ast.Dict):
                    for k2, v2 in zip(v.keys, v.values):
                        if isinstance(k2, ast.Constant) and k2.value == field:
                            return ast.literal_eval(v2)
    log(f'note: OPTIONS[{key}][{field}] not read from {os.path.relpath(path, REPO)}; using {default}')
    return default


def look_args(path, dict_name, key, default):
    """The literal arguments after the name of a look's builder in a model's LOOKS table ('Coat': lambda n:
    pl.drill(n, dark, light)): (args, kwargs), else default (logged)."""
    for node in _tree(path).body:
        if isinstance(node, ast.Assign) and any(isinstance(t, ast.Name) and t.id == dict_name for t in node.targets) \
                and isinstance(node.value, ast.Dict):
            for k, v in zip(node.value.keys, node.value.values):
                if isinstance(k, ast.Constant) and k.value == key and isinstance(v, ast.Lambda) and \
                        isinstance(v.body, ast.Call):
                    try:
                        args = [ast.literal_eval(a) for a in v.body.args[1:]]
                        kwargs = {kw.arg: ast.literal_eval(kw.value) for kw in v.body.keywords}
                        return args, kwargs
                    except ValueError:
                        break
    log(f'note: {dict_name}[{key}] not read from {os.path.relpath(path, REPO)}; using {default}')
    return default


def _module(name):
    if BLENDER_DIR not in sys.path:
        sys.path.insert(0, BLENDER_DIR)
    return importlib.import_module(name)


# ------------------------------------------------------------------------------------------------- the heroes
class Built:
    """One hero's right arm, as built: objects by texture set, the hand's own data for its paint, weights to give."""

    def __init__(self, hero):
        self.hero = hero
        self.sets = []              # [(set name, [objects], master, params, slot)]
        self.hand = None            # (object, hand-frame co, hand-frame normals, HandSpec, paint style)
        self.hand_frame = []        # objects built in the hand frame (moved into the rest pose after weighting)
        self.weights = {}           # object -> {bone: weights}
        self.cloth = []             # (set name, [objects]) the weave goes on
        self.sources = []           # files the looks' colours come from (the bake's digest)
        self.fit = {}               # set name -> BakedSet.adjust's numbers (the look brought to first person)
        self.rivets = {}            # set name -> ([objects], [(centre, normal, radius)]): rivets painted on (Gauge's)
        self.fix_ao = set()         # sets whose occlusion is baked again with a margin for its size (fp_arms_paint)

    def add_set(self, name, objects, master, slot, fit=None, rivets=None, fix_ao=False, **params):
        self.sets.append((name, objects, master, params, slot))
        if fit:
            self.fit[name] = fit
        if rivets:
            self.rivets[name] = rivets
        if fix_ao:
            self.fix_ao.add(name)

    def objects(self):
        return [o for _, objs, _, _, _ in self.sets for o in objs]


def _hand_with_normals(spec, name, look):
    obj, co = hd.hand_mesh(spec, name)
    me = obj.data
    nrm = np.array([v.normal[:] for v in me.vertices])
    me.materials.append(look)
    return obj, co, nrm


def _zones(obj, co, spec, tint, amount):
    """A bare hand's 'zones' colour (looter_looks.skin multiplies its colour by it): white, the knuckles tinted."""
    k = np.zeros(len(co))
    for ch in spec.chains[:4]:
        for j in (1, 2):
            p = ch.joints[j] + v3(0, 0, ch.radii[j]) + 0.0 * C
            k = np.maximum(k, np.exp(-np.sum((co - p) ** 2, axis=1) / (1.3 * C) ** 2))
    col = 1.0 - (1.0 - np.asarray(tint)[None, :]) * (k * amount)[:, None]
    me = obj.data
    attr = me.color_attributes.new('zones', 'FLOAT_COLOR', 'POINT')
    data = np.concatenate([col, np.ones((len(co), 1))], axis=1).astype(np.float32)
    attr.data.foreach_set('color', data.ravel())


def build_odessa(rest, b):
    ol = _module('odessa_looks')
    b.sources += [os.path.join(BLENDER_DIR, 'odessa_looks.py'), os.path.join(BLENDER_DIR, 'odessa_kit.py')]
    looks = {k: ol.LOOKS[k](f'FPArmsOdessa_{k}Look') for k in ('Gloves', 'Coat', 'CoatTrim', 'Shirt')}
    # Her glove (Odessa.py: black kid, its girth 1.12 over her bare hand): slimmer than Ellis's roper gloves on the same
    # bones, a narrower palm, a slim wrist; the palm's underside kept on his (the grip point).
    spec = hd.HandSpec(finger=0.94, thumb=0.95, tip=0.96, palm_top=0.97, palm_bottom=1.42, heel=0.35, outline=0.95,
                       knuckle=0.9, wrist=(2.75, 2.62, 0.72), tris=2400)
    hand, co, nrm = _hand_with_normals(spec, 'Glove_r', looks['Gloves'])
    b.weights[hand] = hd.hand_weights(co, spec)
    # its short flared cuff (odessa_clothes: to 6.8 cm up the wrist, flaring 1.0 x 0.8 cm, 4 mm room)
    flare = hd.Flare(x0=0.3, top=7.1, top_back=0.3, narrow=(3.05, 2.35), wide=(4.05, 3.2), zc=(-0.25, -0.15),
                     power=1.4, t=0.2)
    gcuff, gd = flare.mesh('GloveCuff_r')
    gcuff.data.materials.append(looks['Gloves'])
    b.weights[gcuff] = hd.cuff_weights(gd['h'], gd['part'])
    # the shirt's white cuff over the glove's, 5.5 to 9.8 cm up (2.8 cm of it shows under the coat's sleeve)
    shirt, sd = cl.ShirtCuff(x0=-5.5, x1=-9.9, ry=4.35, rz=3.6, zc=-0.15).mesh('ShirtCuff_r')
    shirt.data.materials.append(looks['Shirt'])
    b.weights[shirt] = {'lowerarm': np.ones(len(sd['co']))}
    b.hand_frame += [hand, gcuff, shirt]
    # the tailcoat's fitted sleeve from 8.3 cm up, under its darker silk cuff to 13.5
    sspec = cl.SleeveSpec(x_start=-8.3, end=(4.62, 3.88),
                          knots=[(4.6, 4.85, 4.3), (lambda p: p.elbow_v - 2.0, 5.15, 4.75),
                                 (lambda p: p.elbow_v + 1.0, 5.55, 5.25), (lambda p: p.length, 5.85, 5.6)],
                          folds=0.55, rim=5.2)
    sleeve, lining, sdat = cl.build_sleeve(rest, sspec)
    for o in (sleeve, lining):
        o.data.materials.append(looks['Coat'])
    b.weights[sleeve] = cl.sleeve_weights(sdat['v'], sdat['path'])
    b.weights[lining] = {'upperarm': np.ones(len(lining.data.vertices))}
    cuff, cdat = cl.build_cuff(sspec, sdat['path'], cl.CuffSpec(length=5.2, below=0.35, room=0.3, flare=0.22, t=0.28))
    cuff.data.materials.append(looks['CoatTrim'])
    b.weights[cuff] = {'lowerarm': np.ones(len(cuff.data.vertices))}
    # kid leather up close: the look's creases (made for her whole figure across a room) a third as deep, a soft sheen
    b.add_set('FPArmsOdessaGlove', [hand, gcuff], 'Character', 'FPArmsOdessa_Glove',
              fit=dict(normal=0.3, soften=0.35, rough_add=0.12))
    b.add_set('FPArmsOdessaCoat', [sleeve, lining, cuff, shirt], 'CharacterCloth', 'FPArmsOdessa_Coat',
              fit=dict(normal=0.8), FuzzStrength=0.35, FuzzLift=0.12)
    b.cloth.append(('FPArmsOdessaCoat', [sleeve, cuff, shirt]))
    b.hand = (hand, co, nrm, spec, dict(glove=True, thread=(0.085, 0.08, 0.088), scuff=(0.2, 0.19, 0.2),
                                         scuff_amount=0.3, cavity=0.4, polish=0.1, thread_rough=0.5))


def build_crane(rest, b):
    cl_ = _module('crane_looks')
    lk = _module('looter_looks')
    crane_py, common_py = os.path.join(CHAR, 'Crane.py'), os.path.join(BLENDER_DIR, 'crane_common.py')
    portrait = os.path.join(PORTRAITS, 'Crane.py')
    b.sources += [crane_py, common_py, portrait, os.path.join(BLENDER_DIR, 'crane_looks.py')]
    coat = model_literal(crane_py, 'COAT', ((0.05, 0.065, 0.08), (0.19, 0.23, 0.27)))
    hem = model_literal(common_py, 'HEM', 0.30)
    skin = portrait_option(portrait, 'A', 'skin', (0.50, 0.58, 0.48))
    looks = {'Skin': lk.skin('FPArmsCrane_SkinLook', skin), 'Coat': cl_.slate('FPArmsCrane_CoatLook', *coat, hem=hem)}
    # Bare dead hands, a gaunt old man's: thin bony fingers on Ellis's long bones, swollen joints, knuckles standing up,
    # the tendons showing on the back, a thin wrist and forearm up inside the sleeve.
    spec = hd.HandSpec(finger=0.8, thumb=0.84, tip=0.9, palm_top=0.85, palm_bottom=1.4, heel=0.3, cup=0.42,
                       outline=0.95, knuckle=1.0, lift=0.14, bulge=0.13, tendons=0.17, wrist=(2.45, 2.35, 0.7),
                       forearm=(-15.0, 2.95, 0.8), pads=0.85, tris=2500)
    hand, co, nrm = _hand_with_normals(spec, 'Hand_r', looks['Skin'])
    _zones(hand, co, spec, (0.86, 0.9, 0.86), 0.5)
    b.weights[hand] = hd.hand_weights(co, spec)
    b.hand_frame.append(hand)
    # the coat's sleeve from 1.2 cm up the wrist (crane_clothes.SLEEVE_END), loose on him, under a torn turned cuff
    # 6.5 cm long whose fold reaches the wrist
    sspec = cl.SleeveSpec(x_start=-1.2, end=(2.85, 2.25),
                          knots=[(5.4, 4.25, 3.9), (lambda p: p.elbow_v - 2.0, 5.3, 4.95),
                                 (lambda p: p.elbow_v + 1.0, 5.9, 5.6), (lambda p: p.length, 6.3, 6.0)],
                          folds=1.15, rim=6.5)
    sleeve, lining, sdat = cl.build_sleeve(rest, sspec)
    for o in (sleeve, lining):
        o.data.materials.append(looks['Coat'])
    b.weights[sleeve] = cl.sleeve_weights(sdat['v'], sdat['path'])
    b.weights[lining] = {'upperarm': np.ones(len(lining.data.vertices))}
    # (its fold 0.4 cm up the wrist, a little shorter than his model's: on a gun's fore-end a cuff to the knuckles hid
    # the whole hand)
    cuff, cdat = cl.build_cuff(sspec, sdat['path'], cl.CuffSpec(length=6.5, below=0.8, room=0.35, flare=0.35, t=0.32,
                                                                 ragged=0.9, seed=11, segs=40))
    cuff.data.materials.append(looks['Coat'])
    b.weights[cuff] = {'lowerarm': np.ones(len(cuff.data.vertices))}
    b.add_set('FPArmsCraneSkin', [hand], 'CharacterSkin', 'FPArmsCrane_Skin', fit=dict(normal=0.38, soften=0.5))
    b.add_set('FPArmsCraneCoat', [sleeve, lining, cuff], 'CharacterCloth', 'FPArmsCrane_Coat', fit=dict(normal=0.85),
              FuzzStrength=0.3, FuzzLift=0.1)
    b.cloth.append(('FPArmsCraneCoat', [sleeve, cuff]))
    b.hand = (hand, co, nrm, spec, dict(glove=False, crease_tint=(0.62, 0.66, 0.6), cavity=1.0,
                                         knuckle=((0.84, 0.88, 0.84), 0.45), dirt=((0.2, 0.2, 0.16), 0.35),
                                         nail=(0.5, 0.52, 0.42), lunula=(0.58, 0.6, 0.52), free=(0.66, 0.64, 0.52),
                                         fold_color=(0.3, 0.33, 0.28), nail_rough=0.48))


def build_pike(rest, b):
    pl = _module('pike_looks')
    lk = _module('looter_looks')
    pike_py, portrait = os.path.join(CHAR, 'Pike.py'), os.path.join(PORTRAITS, 'Pike.py')
    b.sources += [pike_py, portrait, os.path.join(BLENDER_DIR, 'pike_looks.py')]
    skin = portrait_option(portrait, 'A', 'skin', (0.74, 0.60, 0.52))
    coat_a, coat_k = look_args(pike_py, 'LOOKS', 'Coat', ([(0.40, 0.355, 0.235), (0.58, 0.52, 0.36)], {}))
    brass_a, brass_k = look_args(pike_py, 'LOOKS', 'Brass', ([(0.58, 0.42, 0.19), (0.86, 0.68, 0.36)], {}))
    looks = {'Skin': lk.skin('FPArmsPike_SkinLook', skin), 'Coat': pl.drill('FPArmsPike_CoatLook', *coat_a, **coat_k),
             'Brass': lk.metal('FPArmsPike_BrassLook', *brass_a, **brass_k)}
    # Bare weathered hands, a stocky man's: heavy fingers, a deep palm, broad wrist and forearm
    spec = hd.HandSpec(finger=1.0, thumb=1.0, tip=0.97, palm_top=1.2, palm_bottom=1.5, heel=0.5, outline=1.03,
                       knuckle=0.96, lift=0.08, bulge=0.05, tendons=0.05, wrist=(3.0, 2.85, 0.74),
                       forearm=(-15.0, 3.55, 0.82), pads=1.08, tris=2500)
    hand, co, nrm = _hand_with_normals(spec, 'Hand_r', looks['Skin'])
    _zones(hand, co, spec, (0.95, 0.84, 0.82), 0.6)
    b.weights[hand] = hd.hand_weights(co, spec)
    b.hand_frame.append(hand)
    # the field coat's sleeve from 1.2 cm up (pike_clothes.SLEEVE_END), its 4.2 cm turned cuff with the tab and button
    sspec = cl.SleeveSpec(x_start=-1.2, end=(3.45, 2.75),
                          knots=[(3.6, 4.85, 4.45), (lambda p: p.elbow_v - 2.0, 5.4, 5.05),
                                 (lambda p: p.elbow_v + 1.0, 5.9, 5.6), (lambda p: p.length, 6.25, 5.95)],
                          folds=0.9, rim=4.2)
    sleeve, lining, sdat = cl.build_sleeve(rest, sspec)
    for o in (sleeve, lining):
        o.data.materials.append(looks['Coat'])
    b.weights[sleeve] = cl.sleeve_weights(sdat['v'], sdat['path'])
    b.weights[lining] = {'upperarm': np.ones(len(lining.data.vertices))}
    cs = cl.CuffSpec(length=4.2, below=0.4, room=0.35, flare=0.2, t=0.3, tab=(-0.5, 0.45, 3.55, 1.9))
    cuff, cdat = cl.build_cuff(sspec, sdat['path'], cs)
    cuff.data.materials.append(looks['Coat'])
    tab, (bat, bn) = cl.build_tab(sdat['path'], cdat, cs)
    tab.data.materials.append(looks['Coat'])
    button = cl.build_button(bat, bn)
    button.data.materials.append(looks['Brass'])
    for o in (cuff, tab, button):
        b.weights[o] = {'lowerarm': np.ones(len(o.data.vertices))}
    b.add_set('FPArmsPikeSkin', [hand], 'CharacterSkin', 'FPArmsPike_Skin', fit=dict(normal=0.38, soften=0.5))
    b.add_set('FPArmsPikeCoat', [sleeve, lining, cuff, tab, button], 'CharacterCloth', 'FPArmsPike_Coat',
              fit=dict(normal=0.85), FuzzStrength=0.25, FuzzLift=0.1)
    b.cloth.append(('FPArmsPikeCoat', [sleeve, cuff, tab]))
    b.hand = (hand, co, nrm, spec, dict(glove=False, crease_tint=(0.7, 0.58, 0.52), cavity=1.0,
                                         knuckle=((1.04, 0.9, 0.86), 0.55), dirt=((0.3, 0.24, 0.17), 0.45),
                                         nail=(0.78, 0.62, 0.55), lunula=(0.86, 0.76, 0.7), free=(0.8, 0.74, 0.6),
                                         fold_color=(0.45, 0.33, 0.27), nail_rough=0.36))


def build_gauge(rest, b):
    import fp_arms_gauge as ga
    ga.build(rest, b)


BUILDERS = {'Odessa': build_odessa, 'Crane': build_crane, 'Gauge': build_gauge, 'Pike': build_pike}


# ------------------------------------------------------------------------------------------------- the build
def rig_json(arm, rest, hero):
    """FPArms.py's rig_json for this hero's arms (the same bones, so the same numbers), into FPArms_<Hero>_rig.json."""
    mirror = A['mirror']

    def ue(p):
        p = np.asarray(p)
        return [round(float(-p[1] * 100.0), 3), round(float(-p[0] * 100.0), 3), round(float(p[2] * 100.0), 3)]

    def ue_dir(d):
        d = unit(np.asarray(d))
        return [round(float(-d[1]), 5), round(float(-d[0]), 5), round(float(d[2]), 5)]

    bones = [dict(name=bone.name, parent=bone.parent.name if bone.parent else 'root', head=ue(bone.head_local),
                  tail=ue(bone.tail_local)) for bone in arm.data.bones]
    hands = {}
    for side, m in (('r', lambda p: p), ('l', mirror)):
        R = rest.R
        hands[side] = dict(
            Wrist=ue(m(rest.wrist)), FingerDir=ue_dir(m(R[:, 0])), ThumbDir=ue_dir(m(R[:, 1])),
            PalmNormal=ue_dir(m(-R[:, 2])), GripPoint=ue(m(rest.to_world(A['GRIP_POINT'] * C))),
            HandleAxis=ue_dir(m(R @ A['HANDLE_AXIS'])),
            RestCurlDegrees=dict(knuckle=A['REST_CURL'][0], middle=A['REST_CURL'][1], end=A['REST_CURL'][2]))
    out = dict(note=f'SK_FP_Arms_{hero} rest (bind) pose, Unreal component space (cm), root at the eye: the same skeleton '
                    'as SK_FP_Arms (Ellis). Curl a finger by turning each of its bones toward PalmNormal (about the axis '
                    'across the finger). A held handle runs along HandleAxis through GripPoint (the middle of a 4 cm '
                    'handle in the closed fist).',
               bones=bones, hands=hands, upperarm_cm=round(rest.upper * 100, 2), lowerarm_cm=round(rest.fore * 100, 2))
    os.makedirs(WORK, exist_ok=True)
    path = os.path.join(WORK, f'FPArms_{hero}_rig.json')
    with open(path, 'w') as fh:
        json.dump(out, fh, indent=1)
    log(f'rig written to {os.path.relpath(path, REPO)}')


LAST = None                    # the last build's parts (the look sheet reads the hand's spec from it)


def build(hero):
    """Builds hero's arms in the scene: returns (armature, the skinned mesh, the rest pose)."""
    global LAST
    rest = A['Rest']()
    b = Built(hero)
    LAST = b
    BUILDERS[hero](rest, b)
    # weights while the hand-frame parts are still in the hand frame, then everything into the rest pose
    for obj, W in b.weights.items():
        hd.set_weights(obj, W, 'r')
    for obj in b.hand_frame:
        obj.data.transform(rest.matrix())
        obj.data.update()
    # the texture sets: the looks baked (or the files kept), the hand painted over
    first = b.sets[0][0]
    stamp_path = os.path.join(lt.TEXTURE_DIR, first, 'bake.digest')
    baked, stamp = pt.bake_sets([(n, objs) for n, objs, _, _, _ in b.sets], b.objects(), stamp_path,
                                OWN + SHARED + b.sources, force='rebake' in ARGV, keep='keep-textures' in ARGV,
                                fix_ao=b.fix_ao)
    if baked is not None:
        for set_name, objs, _, _, _ in b.sets:
            if set_name in b.fit:
                baked[set_name].adjust(objs, **b.fit[set_name])
        if b.hand is not None:
            hand, co, nrm, spec, style = b.hand
            set_name = next(n for n, objs, _, _, _ in b.sets if hand in objs)
            pt.paint_hand(baked[set_name], hand, co, nrm, spec, style)
        for set_name, objs in b.cloth:
            pt.paint_cloth(baked[set_name], objs)
        for set_name, (objs, rivets) in b.rivets.items():
            whole = next(o for n, o, _, _, _ in b.sets if n == set_name)
            pt.paint_rivets(baked[set_name], objs, rivets, others=[o for o in whole if o not in objs])
        for bs in baked.values():
            bs.save()
        pt.write_digest(stamp)
    # the game's materials on the masters each part kind uses on the hero's own model
    for set_name, objs, master, params, slot in b.sets:
        mat = lt.material(set_name, name=slot, master=master, **params)
        for o in objs:
            o.data.materials.clear()
            o.data.materials.append(mat)
            for name in ('Seams', 'zones'):
                attr = o.data.color_attributes.get(name)
                if attr is not None:
                    o.data.color_attributes.remove(attr)
    right = b.objects()
    left = [A['mirrored_copy'](o, o.name[:-2] + '_l') for o in right]
    arm = A['build_armature'](rest)
    arm.name = arm.data.name = f'FP_Arms_{hero}_Rig'
    bpy.ops.object.select_all(action='DESELECT')
    for p in right + left:
        p.select_set(True)
    bpy.context.view_layer.objects.active = right[0]
    bpy.ops.object.join()
    body = bpy.context.view_layer.objects.active
    body.name = body.data.name = f'FP_Arms_{hero}'
    body.parent = arm
    body.modifiers.new('Armature', 'ARMATURE').object = arm
    body['Nanite'] = 0
    lt.bake_vertex_ao(body, samples=24, distance=0.04, ground=False, strength=0.45)
    tris = sum(len(p.vertices) - 2 for p in body.data.polygons)
    log(f'SK_FP_Arms_{hero}: {tris} triangles, {len(body.data.vertices)} vertices, {len(arm.data.bones)} bones, '
        f'materials {", ".join(m.name for m in body.data.materials)}')
    rig_json(arm, rest, hero)
    return arm, body, rest


def preview(hero, body):
    """The rest pose from the front and the hand close, into Saved/ArtPreviews/Characters/FPArms_<Hero>/."""
    folder = os.path.join(lt.PREVIEW_DIR, 'Characters', f'FPArms_{hero}')
    lt.preview([body], os.path.join(folder, 'rest.png'), view=(-0.4, 1.0, 0.6), ground=False, lens=50.0, fit=0.9)


def main(hero):
    """For Art/Models/Characters/FPArms<Hero>.py: builds the hero's arms (and previews them with --preview)."""
    arm, body, rest = build(hero)
    if lt.want_preview():
        preview(hero, body)
    return arm, body, rest
