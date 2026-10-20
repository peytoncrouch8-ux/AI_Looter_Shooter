"""Hollis Crane's long white hair, drooping mustache and brows as hair cards (Art/Models/Characters/Crane.py), the way
Ellis's hair is made (Tools/Blender/ellis_hair.py, Docs/Pipeline.md "Hair cards"): layered strips of a script-made
strand atlas, not baked.

  - The atlas: ellis_hair's painter (make_textures: a sheet across the top quarter, eight strips below), run with
    Crane's colours into his own set, Art/Textures/CraneHair/T_CraneHair_BC|N|ORM.png: old men's white, yellowed and
    greyer at the roots, a few darker grey strands (ellis_hair's "near black" ones at 0.6 of the white), dull with grave
    dust. ellis_hair's colours and set name are module globals read when it paints: swapped for the call (_as_crane).
  - The cards: roots under the hat's sweatband (ellis_hat.band_ring), round from temple to temple; lank and long: by the
    temples to the collarbones, at the sides onto the cape's shoulders, down the back over the collar and the cape to the
    shoulder blades. Grown over the skin, the shirt's band, the collar, the cape and the coat (all of them surfaces it
    lies on: Ellis's collar and cape were walls his hair stayed inside). Pressed within the band's room under it; points
    the tipped brim would sweep moved in under the crown (ellis_hair.under_brim).
  - The mustache: cards from under the nose drooping past the corners of the mouth to the jaw (the portrait's "droop");
    the brows: short cards along the brow ridge, the outer ends drooping.
"""
import math
import os
from contextlib import contextmanager

import bpy
import numpy as np
from mathutils import Vector

import crane_common as cc
import ellis_hair as eh
import looter_garments as gm
import looter_textures as lt
from crane_common import log

SET = 'CraneHair'
# sRGB: the roots greyer and dirtier, the lengths a yellowed white, the tips paler (ellis_hair's ROOT, MID, TIP, DUST)
COLOURS = dict(ROOT=np.array([0.50, 0.49, 0.45]), MID=np.array([0.74, 0.72, 0.65]), TIP=np.array([0.80, 0.78, 0.71]),
               DUST=np.array([0.55, 0.52, 0.46]))


@contextmanager
def _as_crane():
    """ellis_hair's painter and material with Crane's set and colours (its globals, swapped back after)."""
    keep = {k: getattr(eh, k) for k in ('SET',) + tuple(COLOURS)}
    eh.SET = SET
    for k, v in COLOURS.items():
        setattr(eh, k, v)
    try:
        yield
    finally:
        for k, v in keep.items():
            setattr(eh, k, v)


def have_textures():
    return all(os.path.exists(lt.texture_path(SET, k)) for k in ('BC', 'N', 'ORM'))


def make_textures():
    with _as_crane():
        return eh.make_textures(look=False)


def material(name='Crane_Hair'):
    if not have_textures():
        make_textures()
    with _as_crane():
        return eh.material(name)


# ------------------------------------------------------------------------------------------------------- the cards

# Layers from the scalp out: (name, cards, width m, lift at the root, lift below, length range (of the drop to the
# tips' height), strip kind, rows, quads across, twist degrees, wander, flick out range, sweep).
CARDS = dict(
    seed=11,
    span=(40.0, 320.0),
    under=0.012,
    sheet=dict(span=(96.0, 264.0), cols=30, rows=12, lift=(0.0015, 0.004), length=0.97),
    layers=(
        ('main', 34, 0.046, 0.0025, 0.007, (0.90, 1.03), 'dense', 10, 2, 8.0, 0.03, (0.0, 0.0), 0.06),
        ('outer', 40, 0.032, 0.0035, 0.010, (0.62, 0.98), 'clump', 10, 2, 14.0, 0.05, (0.0, 0.05), 0.10),
        ('wisp', 22, 0.017, 0.0040, 0.012, (0.70, 1.06), 'wisp', 9, 1, 20.0, 0.05, (0.0, 0.0), 0.05),
    ),
    # (no fly-aways: on the shoulders they lay out level, thin black lines across the cape)
    # below the jaw the hair parts over the shoulders: what grows from in front of SPLIT degrees falls forward in front
    # of the shoulder, what grows behind falls back over the cape (on the shoulders' top it slid out sideways and
    # fanned level, round 1)
    split=118.0, fwd=0.85, back=0.8, side=150.0,
    # the tips' drop below the eyes: by the temples, at the sides, at the back (m)
    tips=dict(temple=0.25, side=0.31, back=0.40),
)


def _tip_drop(deg, tips):
    """How far below the eyes the hair ends at angle deg round the head (0 the face): the temples', the sides' (90
    degrees) and the back's, between."""
    a = min(deg, 360.0 - deg)
    return float(np.interp(a, [40.0, 90.0, 180.0], [tips['temple'], tips['side'], tips['back']]))


def grow_long(tree, root, length, rows, clear, out_bias, rng, wob, down, split_z, part, axis, side=True):
    """A long card's centre line (ellis_hair.grow's way: falls along down, slides over what's in tree, kept clear(t)
    off it, wandering), parting below split_z: part (m per m, + forward) leans it to the front or the back of the
    shoulder it falls onto, coming in over 6 cm; and there a card from the side of the head (side) never steps out away
    from the body's axis (axis: x, y), so on the shoulder's top it slides off to the front or the back, not out along
    the slope (round 2: the side hair
    fanned out level across the cape's shoulders)."""
    pts = [np.array(root, dtype=np.float64)]
    p = Vector(tuple(root))
    step = length / (rows - 1)
    w = Vector(tuple(rng.normal(size=3))) * wob
    for k in range(rows - 1):
        t = (k + 1) / (rows - 1)
        d = Vector(tuple(down)) + w * t
        if p.z < split_z:
            d = d + Vector((0.0, -part, 0.0)) * min(1.0, (split_z - p.z) / 0.06)
        d.normalize()
        hit = tree.find_nearest(p, 0.2)
        if hit[0] is not None and (p - hit[0]).length < clear(t) * 3.0:
            n = hit[1]
            if d.dot(n) < 0.0:
                d = d - n * d.dot(n)
            d = (d + n * out_bias * 0.25).normalized()
        if p.z < split_z and side:                    # (the back's go out over the collar and down the cape)
            r = Vector((p.x - axis[0], p.y - axis[1], 0.0))
            if r.length > 1e-6:
                r.normalize()
                out = d.dot(r)
                if out > 0.0:
                    d = d - r * out
                    if d.length < 1e-4:
                        d = Vector((0.0, -part, -1.0))
                    d.normalize()
        p = p + d * step
        hit = tree.find_nearest(p, 0.2)
        if hit[0] is not None and (p - hit[0]).dot(hit[1]) < clear(t):
            p = hit[0] + hit[1] * clear(t)
        pts.append(np.array(p[:]))
    return np.array(pts)


def build(fig, under_obs, name, mat, pivot=None, **params):
    """The long hair as one object of cards (UVs on the CraneHair atlas, custom normals out of the hair's volume).
    under_obs: the skin first (the scalp), then what the hair lies on."""
    P = dict(CARDS, **params)
    rng = np.random.default_rng(P['seed'])
    tree = gm.bvh_objects(under_obs)
    skin_tree = gm.bvh_objects(under_obs[:1])
    hf = fig.hf
    hv = hf['verts']
    band_y = hf['eye_y'] + 0.47
    cz = (hv[:, 2].max() + hv[:, 2].min()) * 0.5 - 0.1
    head_c = fig.head_point(np.array([0.0, band_y - 0.3, cz]))
    ring = eh.band_ring(fig, head_c, skin_tree, log)
    eye_z = fig.eye_z
    cards = eh._Cards()
    span0, span1 = P['span']
    counts = {}

    def root_at(deg, lift):
        a = math.radians(deg)
        z, room = ring(a)
        z += P['under']
        dvec = eh.direction(a)
        o = head_c + dvec * 0.3
        o[2] = z
        hit = skin_tree.ray_cast(Vector(tuple(o)), Vector(tuple(-dvec)), 0.5)
        if hit[0] is None:
            hit = skin_tree.find_nearest(Vector(tuple(o)))
        lift = min(lift, max(0.0006, room - 0.001))
        return np.array(hit[0]) + np.array(hit[1]) * lift, z, lift

    tip_z = lambda deg: eye_z - _tip_drop(deg, P['tips'])
    log(f'{name}: tips at {tip_z(span0):.3f} m by the temples, {tip_z(90.0):.3f} at the sides, {tip_z(180.0):.3f} at '
        f'the back')

    def normals_for(Pg, out):
        rad = Pg - head_c
        rad /= np.maximum(np.linalg.norm(rad, axis=-1, keepdims=True), 1e-9)
        n = 0.7 * rad + 0.3 * out[:, None, :]
        return n / np.maximum(np.linalg.norm(n, axis=-1, keepdims=True), 1e-9)

    split_z = eye_z - 0.10                        # the jaw's line: below it the hair parts over the shoulders
    part = lambda deg: P['fwd'] if min(deg, 360.0 - deg) < P['split'] else -P['back']
    sh = P['sheet']
    cols = []
    for c in range(sh['cols']):
        deg = sh['span'][0] + (sh['span'][1] - sh['span'][0]) * c / (sh['cols'] - 1)
        lo_lift, hi_lift = sh['lift']
        root, zr, lo_lift = root_at(deg, lo_lift)
        L = max(0.03, (zr - tip_z(deg)) * 1.05 * sh['length'])
        clear = (lambda lo, hi: (lambda t: lo + (hi - lo) * float(lt.smooth(0.05, 0.3, t))))(lo_lift, hi_lift)
        cols.append(grow_long(tree, root, L, sh['rows'], clear, 0.25, rng, 0.0, (0.0, 0.0, -1.0), split_z, -P['back'], head_c[:2],
                              side=min(deg, 360.0 - deg) < P['side']))
    G = np.stack(cols, axis=1)
    out_all = G - head_c
    out_all /= np.linalg.norm(out_all, axis=-1, keepdims=True)
    t = np.linspace(0.0, 1.0, G.shape[0])
    U, V = eh.sheet_uv(np.linspace(0.0, 1.0, G.shape[1])[None, :].repeat(G.shape[0], 0), t[:, None].repeat(G.shape[1], 1))
    cards.add_grid(G, U, V, out_all)
    counts['sheet'] = sh['cols']

    for (layer, n, width, lift0, lift1, lscale, kind, rows, across, twist, wob, flick, sweep) in P['layers']:
        strips = eh.strips_of(kind)
        step = (span1 - span0) / n
        shift = 0.5 if layer == 'outer' else 0.0
        for c in range(n):
            deg = span0 + (step * (c + 0.5 + shift + rng.uniform(-0.3, 0.3))) % (span1 - span0)
            if layer in ('wisp', 'fly'):
                deg = span0 + (span1 - span0) * rng.uniform(0.02, 0.98)
            root, zr, l0 = root_at(deg, lift0)
            L = max(0.03, (zr - tip_z(deg)) * 1.04 * rng.uniform(*lscale))
            clear = (lambda lo, hi: (lambda t: lo + (hi - lo) * float(lt.smooth(0.08, 0.4, t))))(l0, lift1)
            a = math.radians(deg)
            down = np.array([math.cos(a), math.sin(a), 0.0]) * rng.uniform(-sweep, sweep) + np.array([0.0, 0.0, -1.0])
            # (no flick: long hair hangs; flicked out on the shoulders it fanned level)
            st = grow_long(tree, root, L, rows, clear, 0.3 + 0.2 * rng.random(), rng, wob, down, split_z, part(deg), head_c[:2],
                           side=min(deg, 360.0 - deg) < P['side'])
            tw = rng.uniform(-twist, twist)
            tang, side, out = eh.frames(st, head_c, tw)
            w = width * rng.uniform(0.8, 1.2) * (1.0 - 0.3 * np.linspace(0.0, 1.0, rows))
            cup = np.linalg.norm((st - head_c)[:, :2], axis=1)
            Pg, sfrac = eh.card_grid(st, side, out, w, across, cup)
            k = int(rng.choice(strips))
            sf = sfrac[::-1] if rng.random() < 0.5 else sfrac
            tt = np.linspace(0.0, 1.0, rows)
            U, V = eh.strip_uv(k, sf[None, :].repeat(rows, 0), tt[:, None].repeat(across + 1, 1))
            cards.add_grid(Pg, U, V, normals_for(Pg, out))
        counts[layer] = n

    verts = np.array(cards.verts)
    pressed = 0
    for i, q in enumerate(verts):
        z0, room = ring(float(eh.round_angle(q, head_c)[0]))
        if z0 - 0.002 <= q[2] <= z0 + 0.035:
            loc, nrm, _i, _d = skin_tree.find_nearest(Vector(tuple(q)), 0.05)
            if loc is None:
                continue
            allowed = max(0.0006, room - 0.0008)
            d = (Vector(tuple(q)) - loc).dot(nrm)
            if d > allowed:
                verts[i] = q - np.array(nrm) * (d - allowed)
                pressed += 1
    verts, moved = eh.under_brim(verts, pivot, head_c, skin_tree, log)
    # nothing left inside what it lies on (a curled card edge, a flick back in)
    inside = 0
    for i, q in enumerate(verts):
        loc, nrm, _i, _d = tree.find_nearest(Vector(tuple(q)), 0.03)
        if loc is not None and (Vector(tuple(q)) - loc).dot(nrm) < 0.0015 and q[2] < head_c[2] - 0.05:
            verts[i] = np.array(loc + nrm * 0.0015)
            inside += 1
    cards.verts = [tuple(q) for q in verts]
    log(f'{name}: {pressed} card points pressed in under the band, {moved} moved out of the tipped brim\'s way, '
        f'{inside} lifted off what they lie on')
    ob = _object(name, cards, mat)
    log(f'{name}: hair cards ' + ', '.join(f'{k} {v}' for k, v in counts.items()) + f'; {cc.tris_of(ob)} triangles')
    fig.hair_centre = head_c
    return ob


def _object(name, cards, mat):
    me = bpy.data.meshes.new(name)
    me.from_pydata(cards.verts, [], cards.faces)
    layer_uv = me.uv_layers.new(name='UVMap')
    layer_uv.data.foreach_set('uv', np.asarray(cards.uv, dtype=np.float32).ravel())
    me.validate(clean_customdata=False)
    me.update()
    me.shade_smooth()
    me.normals_split_custom_set_from_vertices([tuple(n) for n in cards.normals])
    ob = bpy.data.objects.new(name, me)
    bpy.context.scene.collection.objects.link(ob)
    ob.data.materials.append(mat)
    return ob


def _head_dir(fig, d):
    """A direction in the head's rest space (MakeHuman axes) as the posed head carries it, in the figure's space."""
    return gm.unit(np.array(cc.mh.to_blender(fig.g_head[:3, :3] @ np.asarray(d, float))))


def _on_face(fig, tree, x, y, lift):
    """The face's surface at head-local (x, y) (decimetres), from the front, lifted off it; (point, normal)."""
    hv = fig.hf['verts']
    front = hv[:, 2].max()
    p, q = fig.head_point(np.array([x, y, front + 0.4])), fig.head_point(np.array([x, y, front - 1.0]))
    d = gm.unit(q - p)
    hit = tree.ray_cast(Vector(tuple(p)), Vector(tuple(d)), float(np.linalg.norm(q - p)))
    if hit[0] is None:
        return None
    return np.array(hit[0]) + np.array(hit[1]) * lift, np.array(hit[1])


def build_mustache(fig, skin, mat, name='Mustache'):
    """The white mustache drooping past the corners of the mouth to the jaw (the portrait's build_mustache "droop"): a
    horseshoe of cards rooted along the upper lip's arch under the nose, the middle ones short over the lip, the outer
    ones long and combed down past the mouth's corners along the jaw; two layers, the top one shorter; grown over the
    skin a few millimetres off it."""
    rng = np.random.default_rng(13)
    tree = gm.bvh_objects([skin])
    hv = fig.hf['verts']
    nose = hv[np.argmax(hv[:, 2])]
    centre = (fig.eyes[0] + fig.eyes[1]) * 0.5 - _head_dir(fig, (0.0, 0.0, 1.0)) * 0.09
    cards = eh._Cards()
    count = 0
    for layer, n, lift, length, width in (('under', 10, 0.0012, (0.024, 0.060), 0.012), ('over', 7, 0.0028, (0.020, 0.045), 0.010)):
        for c in range(2 * n):
            sx = 1.0 if c < n else -1.0
            ax = ((c % n) + 0.5 + rng.uniform(-0.2, 0.2)) / n
            x = sx * 0.30 * ax
            y = nose[1] - 0.19 - 0.12 * ax ** 2 - 0.02 * rng.random()
            got = _on_face(fig, tree, x, y, lift)
            if got is None:
                continue
            root, _n = got
            comb = _head_dir(fig, (sx * (0.15 + 0.35 * ax), -1.0, 0.30 - 0.20 * ax))
            L = (length[0] + (length[1] - length[0]) * ax ** 1.6) * (0.9 + 0.2 * rng.random())
            rows = 5
            st = eh.grow(tree, root, L, rows, lambda t: lift + 0.0022 * t, 0.15, rng, 0.02, down=tuple(comb))
            tang, side, out = eh.frames(st, centre, rng.uniform(-8.0, 8.0))
            w = width * (0.9 + 0.2 * rng.random()) * (1.0 - 0.6 * np.linspace(0.0, 1.0, rows))
            Pg, sfrac = eh.card_grid(st, side, out, w, 2, np.full(rows, 0.05))
            k = int(rng.choice(eh.strips_of('dense' if layer == 'under' else 'clump')))
            U, V = eh.strip_uv(k, sfrac[None, :].repeat(rows, 0), np.linspace(0.0, 1.0, rows)[:, None].repeat(3, 1))
            cards.add_grid(Pg, U, V, np.repeat(out[:, None, :], 3, 1))
            count += 1
    ob = _object(name, cards, mat)
    log(f'{name}: {count} cards, {cc.tris_of(ob)} triangles')
    return ob


def build_brows(fig, skin, mat, name='Brows'):
    """Long wiry white brows on the heavy ridge (the portrait's build_brows): two layers of short cards a side lying
    along the brow from its inner end outward, the outer ends drooping as old men's do."""
    rng = np.random.default_rng(3)
    tree = gm.bvh_objects([skin])
    cards = eh._Cards()
    for e in fig.hf['eyes']:
        sx = math.copysign(1.0, e[0])
        head_c = fig.head_point(np.array([0.0, e[1], e[2] - 0.9]))
        for layer, n, lift, width in (('under', 3, 0.0010, 0.014), ('over', 2, 0.0022, 0.011)):
            for k in range(n):
                t = (k + 0.3 + 0.4 * rng.random()) / n
                x = e[0] + sx * (-0.24 + 0.30 * t)
                y = e[1] + 0.20 + 0.03 * math.sin(t * math.pi * 0.8) - 0.03 * (1 - t)
                got = _on_face(fig, tree, x, y, lift)
                if got is None:
                    continue
                root, _n = got
                comb = _head_dir(fig, (sx * 1.0, -0.15 - 0.35 * t, 0.1))
                L = 0.026 + 0.012 * rng.random()
                rows = 4
                st = eh.grow(tree, root, L, rows, lambda tt: lift + 0.0015 * tt, 0.2, rng, 0.02, down=tuple(comb))
                tang, side, out = eh.frames(st, head_c, 0.0)
                w = width * (1.0 - 0.45 * np.linspace(0.0, 1.0, rows))
                Pg, sfrac = eh.card_grid(st, side, out, w, 1, np.full(rows, 0.06))
                kk = int(rng.choice(eh.strips_of('dense')))
                U, V = eh.strip_uv(kk, sfrac[None, :].repeat(rows, 0), np.linspace(0.0, 1.0, rows)[:, None].repeat(2, 1))
                cards.add_grid(Pg, U, V, np.repeat(out[:, None, :], 2, 1))
    ob = _object(name, cards, mat)
    log(f'{name}: {cc.tris_of(ob)} triangles')
    return ob


def chain_joints(co, centre, top, back=math.pi, width=math.radians(25)):
    return eh.chain_joints(co, centre, top, back, width)
